// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Source code in this file must use VEX encoded instructions strictly !!!
//  /arch:AVX compiler option in the sources file guarantees that.
//  Using any leagcy-encoded SSE instructions here will incur a heavy performance penalty (~70 cycles)
//  because of state transitions in the CPU register file backing the XMM/YMM registers.

#include "checksumstd.hxx"

//  ================================================================
inline XECHECKSUM MakeChecksumFromECCXORAndPgno(
    const ULONG eccChecksum,
    const ULONG xorChecksum,
    const ULONG pgno )
//  ================================================================
{
    const XECHECKSUM low    = xorChecksum ^ pgno;
    const XECHECKSUM high   = (XECHECKSUM)eccChecksum << 32;
    return ( high | low );
}

#if ( defined _M_AMD64 || defined _M_IX86 ) && !defined _CHPE_X86_ARM64_

#include <intrin.h>
#include <emmintrin.h>

typedef unsigned __int64 XECHECKSUM;

typedef XECHECKSUM  (*PFNCHECKSUMNEWFORMAT)( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG   (*PFNCHECKSUMOLDFORMAT)( const unsigned char * const, const ULONG );


//  ================================================================
inline __m128i operator^( const __m128i dq0, const __m128i dq1 )
//  ================================================================
{
    return _mm_xor_si128( dq0, dq1 );
}

//  ================================================================
inline __m128i operator^=( __m128i& dq0, const __m128i dq1 )
//  ================================================================
{
    return dq0 = _mm_xor_si128( dq0, dq1 );
}

inline __m256i operator^( const __m256i qq0, const __m256i qq1)
{
    return _mm256_castpd_si256( _mm256_xor_pd( _mm256_castsi256_pd( qq0 ), _mm256_castsi256_pd( qq1 ) ) );
}

//  ================================================================
inline __m256i operator^=( __m256i& qq0, const __m256i qq1 )
//  ================================================================
{
    return qq0 = _mm256_castpd_si256( _mm256_xor_pd( _mm256_castsi256_pd( qq0 ), _mm256_castsi256_pd( qq1 ) ) );
}

//  ================================================================
inline LONG lParityMaskPopcnt( const LONG dw )
//  ================================================================
//
//  Compute a 32-bit parity mask of a 32-bit integer using POPCNT instruction
//  This instruction is part of SSE4.2 on intel processors (Core2, Core i3/5/7)
//  AMD architectures K10 and newer support it even though they might not support SSE4
//
{
    INT popcnt = (INT) _mm_popcnt_u32( dw );
    return -(popcnt & 0x01);
}

//  ================================================================
inline LONG lParityMaskAVX( const __m256i qq )
//  ================================================================
//
//  Computes a 32-bit parity mask of a 256-bit integer using vperm2f128, vpermilpd, POPCNT instructions
//  These instruction are part of AVX and SSE4.2 instruction sets respectively.
//
{
    const __m256i qq1 = _mm256_permute2f128_si256( qq, qq, 0x01 );  // swaps lo and hi 128-bit integers
    const __m128i dq2 = _mm256_castsi256_si128( qq1 ^ qq ); // reduce to 128-bits
    const __m128i dq3 = _mm_shuffle_epi32( dq2, 0x4e);
    const __m128i dq4 = dq3 ^ dq2;  // reduce to 64-bits

#if ( defined _M_IX86  )
    // reduce to 32-bits
    const __m128i dq5 = _mm_shuffle_epi32( dq4, 0x1b );
    const __m128i dq6 = dq5 ^ dq4;
    INT popcnt = _mm_popcnt_u32( _mm_cvtsi128_si32( dq6 ) );
#else
    INT popcnt = (INT) _mm_popcnt_u64( _mm_cvtsi128_si64( dq4 ) );
#endif

    return -(popcnt & 0x01);
}

//  A table containing 3-bit parity values for a byte.
//  Contains 6-bits of data (r and r') required to compute an ECC checksum for a byte.
//  ================================================================
extern __declspec( align( 128 ) ) const unsigned char g_bECCLookupTable[ 256 ] =
//  ================================================================
{
    0x00, 0x70, 0x61, 0x11, 0x52, 0x22, 0x33, 0x43, 0x43, 0x33, 0x22, 0x52, 0x11, 0x61, 0x70, 0x00,  // 0x0x
    0x34, 0x44, 0x55, 0x25, 0x66, 0x16, 0x07, 0x77, 0x77, 0x07, 0x16, 0x66, 0x25, 0x55, 0x44, 0x34,  // 0x1x
    0x25, 0x55, 0x44, 0x34, 0x77, 0x07, 0x16, 0x66, 0x66, 0x16, 0x07, 0x77, 0x34, 0x44, 0x55, 0x25,  // 0x2x
    0x11, 0x61, 0x70, 0x00, 0x43, 0x33, 0x22, 0x52, 0x52, 0x22, 0x33, 0x43, 0x00, 0x70, 0x61, 0x11,  // 0x3x
    0x16, 0x66, 0x77, 0x07, 0x44, 0x34, 0x25, 0x55, 0x55, 0x25, 0x34, 0x44, 0x07, 0x77, 0x66, 0x16,  // 0x4x
    0x22, 0x52, 0x43, 0x33, 0x70, 0x00, 0x11, 0x61, 0x61, 0x11, 0x00, 0x70, 0x33, 0x43, 0x52, 0x22,  // 0x5x
    0x33, 0x43, 0x52, 0x22, 0x61, 0x11, 0x00, 0x70, 0x70, 0x00, 0x11, 0x61, 0x22, 0x52, 0x43, 0x33,  // 0x6x
    0x07, 0x77, 0x66, 0x16, 0x55, 0x25, 0x34, 0x44, 0x44, 0x34, 0x25, 0x55, 0x16, 0x66, 0x77, 0x07,  // 0x7x
    0x07, 0x77, 0x66, 0x16, 0x55, 0x25, 0x34, 0x44, 0x44, 0x34, 0x25, 0x55, 0x16, 0x66, 0x77, 0x07,  // 0x8x
    0x33, 0x43, 0x52, 0x22, 0x61, 0x11, 0x00, 0x70, 0x70, 0x00, 0x11, 0x61, 0x22, 0x52, 0x43, 0x33,  // 0x9x
    0x22, 0x52, 0x43, 0x33, 0x70, 0x00, 0x11, 0x61, 0x61, 0x11, 0x00, 0x70, 0x33, 0x43, 0x52, 0x22,  // 0xax
    0x16, 0x66, 0x77, 0x07, 0x44, 0x34, 0x25, 0x55, 0x55, 0x25, 0x34, 0x44, 0x07, 0x77, 0x66, 0x16,  // 0xbx
    0x11, 0x61, 0x70, 0x00, 0x43, 0x33, 0x22, 0x52, 0x52, 0x22, 0x33, 0x43, 0x00, 0x70, 0x61, 0x11,  // 0xcx
    0x25, 0x55, 0x44, 0x34, 0x77, 0x07, 0x16, 0x66, 0x66, 0x16, 0x07, 0x77, 0x34, 0x44, 0x55, 0x25,  // 0xdx
    0x34, 0x44, 0x55, 0x25, 0x66, 0x16, 0x07, 0x77, 0x77, 0x07, 0x16, 0x66, 0x25, 0x55, 0x44, 0x34,  // 0xex
    0x00, 0x70, 0x61, 0x11, 0x52, 0x22, 0x33, 0x43, 0x43, 0x33, 0x22, 0x52, 0x11, 0x61, 0x70, 0x00,  // 0xfx
};

inline ULONG lECCLookup8bit(const ULONG byte)
{
    return g_bECCLookupTable[ byte & 0xff ];
}

XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  Optimized ECC function for SandyBridge or higher (AVX + 64B L2 + Prefetching)
//  the data is organized into an array of bits that is 256*4=1024 bits wide
//  Each line of CPU cache will hold 1 row of data
//  Divides the ECC-parity bits into 4 sets:
//  p  = row parity 6-bits (index to a 1024-bit row)
//  q  = 2-bits: index to a 256-bit word in a row)
//  q_ = 3-bits: index to a 32-bit word in 256-bits.
//  r = column parity 5-bits (index to a bit in a 32-bit word column)
//  Each set is composed of index, index` as explained above in the algo description
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatAVX;
    Unused( pfn );

    // data size sanity check
    //
    Assert( 32 == sizeof( __m256i ) );
    Assert( 4 == sizeof( ULONG ) );

    // cb should be an order of 2 and between 1k and 8k
    //
    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    // pb should align to 256 (4 rows of CPU cache)
    //
    Assert( 0 == ( ( uintptr_t )pb & ( 256 - 1 ) ) );

    // get quad quadword count of the data block
    //
    const ULONG cqq = cb / 32;

    //================================================
    // compute ECC bits shown as '1' (p and p')
    // 1111 1100 0000 0000 1111 1100 0000 0000
    //
    ULONG p = 0;

    // dq0, dq1, dq2, dq3 hold accumulated column parity bits (256*4=1024 bits)
    // for calculating q, q_, and r later
    //
    __m256i qq0 = _mm256_setzero_si256();
    __m256i qq1 = _mm256_setzero_si256();
    __m256i qq2 = _mm256_setzero_si256();
    __m256i qq3 = _mm256_setzero_si256();
    {
        // combine 16-bit index and index' together into a 32-bit double index
        //
        ULONG idxp = 0xfc000000;

        // loop control variable
        //
        ULONG i = 0;
        __m256i qqL0;

        // treat the first 64-bit of the page always as 0
        //
        const __m256i* pqq = ( const __m256i* )pb;
        if ( fHeaderBlock )
        {
            const __m128i* pdq = (const __m128i*) pqq;
            qqL0 = _mm256_castsi128_si256( _mm_unpackhi_epi64( _mm_setzero_si128(), pdq[ 0 ] ) );
            qqL0 = _mm256_insertf128_si256(qqL0, pdq[1], 1);
            goto Start;
        }

        // don't unroll the loop. cpu front-end is the bottleneck. Small loops fitting in the uops tracecache of the cpu perform better.
        // one iteration will process 1 row of data in 2 cache lines.
        //
        do
        {
            // read one row (2 cachelines) from data (256*4=1024 bits)
            //
            qqL0 = pqq[ i + 0 ];
Start:
            // Each iteration of the loop consumes 4 64-byte cache lines, so prefetch 4 lines every iteration
            // This would have the effect of pre-caching the next 16 cache-lines.
            // Caching starts 16 elements into the buffer. This number has been manually tuned by running the Checksum.Perf test with different values.
            // It has significant impact on the performance of this function. Should be optimized again for architectures with significantly different memory latencies (relative to core clock).
            _mm_prefetch( ( char *)&pqq[ i + 16 ], _MM_HINT_NTA );

            const __m256i qqL1 = pqq[ i + 1 ];
            const __m256i qqL2 = pqq[ i + 2 ];
            const __m256i qqL3 = pqq[ i + 3 ];

            // update accumulated column parity bits for calculating q, q_, and r later
            //
            qq0 ^= qqL0;
            qq1 ^= qqL1;
            qq2 ^= qqL2;
            qq3 ^= qqL3;
            const __m256i qqLAcc = qqL0 ^ qqL1 ^ qqL2 ^ qqL3;

            _mm_prefetch( ( char *)&pqq[ i + 16 + 2], _MM_HINT_NTA );

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMaskAVX( qqLAcc );
            
            // the magic numbers distribute the parity according to the algorithm
            // decrements the upper bits and increments the lower bits
            idxp += 0xfc000400;

            // update loop control variable
            //
            i += 4;

            // inform compiler the loop won't quit in 1st iteration.
            //
            __assume( 8 < cqq );
        }
        while ( i < cqq );
    }

    //================================================
    // Compute q and q' bits. Assume the block has 256-bit wide rows now (instead of earlier 1024-bits).
    //
    
    // compute ECC bits shown as '1'
    // 0000 0011 0000 0000 0000 0011 0000 0000
    //

    // For computing q_ and r later
    // Collapse 256x4 XORs into 32x8 XORs
    __m256i qqAcc = qq0 ^ qq1 ^ qq2 ^ qq3;
    __declspec( align( 256 ) ) __m256i aryqq[1];
    aryqq[0] = qqAcc;

    ULONG q = 0;
    ULONG idxq = 0xff000000;
    
    q ^= idxq & lParityMaskAVX( qq0 );
    idxq += 0xff000100;
    q ^= idxq & lParityMaskAVX( qq1 );
    idxq += 0xff000100;
    q ^= idxq & lParityMaskAVX( qq2 );
    idxq += 0xff000100;
    q ^= idxq & lParityMaskAVX( qq3 );
    
    //================================================
    // Compute ECC bits shown as '1' (q_ and q_'). Assume the block has 64-bit wide rows now.
    // 0000 0000 1110 0000 0000 0000 1110 0000
    //
    ULONG q_ = 0;
    ULONG idxq_ = 0xffe00000;
    UINT *pdw = ( UINT* )aryqq;
    UINT dwAcc = 0;
    for ( ULONG i = 0; i < 8; i++ )
    {
        const UINT dwT = pdw[i];
        dwAcc ^= dwT;
        q_ ^= idxq_ & -( _mm_popcnt_u32( dwT ) & 0x01 );
        idxq_ += 0xffe00020;
    }

    //================================================
    // compute ECC bits shown as '1' (r and r')
    // 0000 0000 0001 1000 0000 0000 0001 1000
    //
    ULONG r = 0;
    UINT byteT = dwAcc;
    ULONG byte0 = 0;
    ULONG idxr = 0xfff80000;
    for ( ULONG i = 0; i < 4; i++ ) // compiler should unroll hopefully
    {
        r ^= idxr & -INT( _mm_popcnt_u32( byteT & 0xff ) & 0x01 );
        byte0 ^= byteT;
        byteT >>= 8;
        idxr += 0xfff80008;
    }

    //================================================
    // Lookup pre-calculated ECC bits shown as '1'
    // and combine them into r and r'
    // 0000 0000 0000 0111 0000 0000 0000 0111
    //
    const LONG bits = lECCLookup8bit( byte0 );
    r |= ( bits & 0x07 );   // move r into position (bits 0,1,2 of the lookup)
    r |= ( ( bits << 12 ) & 0x00070000 );   // move r' into position (bits 4,5,6 of the lookup)

    // Reset all YMM registers to 0
    // Any code that uses YMM registers should leave them in a reset state to avoid a register file stall (~70 cycle penalty)
    // when someone tries to access XMM counterparts of those registers.
    // DONT use AVX after zeroing !
    _mm256_zeroall();

    //================================================
    // mask some high bits out according to the data size
    //
    const ULONG mask = ( cb << 19 ) - 1;

    // assemble final checksum
    //
    const ULONG ecc = p & 0xfc00fc00 & mask | q & 0x03000300 | q_ & 0x00e000e0 | r & 0x001f001f;
    const ULONG xor = dwAcc;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}
    
#else

//  ================================================================
XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  stub used for non x86 builds (function ptr never called)
//
//-
{
    Enforce( fFalse );  // Should never be selected by SelectChecksumNewFormat() if it isn't available
    return MakeChecksumFromECCXORAndPgno( 0, 0, pgno );
}

#endif
