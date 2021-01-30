// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "checksumstd.hxx"

inline XECHECKSUM MakeChecksumFromECCXORAndPgno(
    const ULONG eccChecksum,
    const ULONG xorChecksum,
    const ULONG pgno )
{
    const XECHECKSUM low    = xorChecksum ^ pgno;
    const XECHECKSUM high   = (XECHECKSUM)eccChecksum << 32;
    return ( high | low );
}

#if ( defined _AMD64_ || defined _X86_ ) && !defined _CHPE_X86_ARM64_

#include <intrin.h>
#include <emmintrin.h>

typedef unsigned __int64 XECHECKSUM;

typedef XECHECKSUM  (*PFNCHECKSUMNEWFORMAT)( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG   (*PFNCHECKSUMOLDFORMAT)( const unsigned char * const, const ULONG );


inline __m128i operator^( const __m128i dq0, const __m128i dq1 )
{
    return _mm_xor_si128( dq0, dq1 );
}

inline __m128i operator^=( __m128i& dq0, const __m128i dq1 )
{
    return dq0 = _mm_xor_si128( dq0, dq1 );
}

inline __m256i operator^( const __m256i qq0, const __m256i qq1)
{
    return _mm256_castpd_si256( _mm256_xor_pd( _mm256_castsi256_pd( qq0 ), _mm256_castsi256_pd( qq1 ) ) );
}

inline __m256i operator^=( __m256i& qq0, const __m256i qq1 )
{
    return qq0 = _mm256_castpd_si256( _mm256_xor_pd( _mm256_castsi256_pd( qq0 ), _mm256_castsi256_pd( qq1 ) ) );
}

inline LONG lParityMaskPopcnt( const LONG dw )
{
    INT popcnt = (INT) _mm_popcnt_u32( dw );
    return -(popcnt & 0x01);
}

inline LONG lParityMaskAVX( const __m256i qq )
{
    const __m256i qq1 = _mm256_permute2f128_si256( qq, qq, 0x01 );
    const __m128i dq2 = _mm256_castsi256_si128( qq1 ^ qq );
    const __m128i dq3 = _mm_shuffle_epi32( dq2, 0x4e);
    const __m128i dq4 = dq3 ^ dq2;

#if ( defined _X86_ )
    const __m128i dq5 = _mm_shuffle_epi32( dq4, 0x1b );
    const __m128i dq6 = dq5 ^ dq4;
    INT popcnt = _mm_popcnt_u32( _mm_cvtsi128_si32( dq6 ) );
#else
    INT popcnt = (INT) _mm_popcnt_u64( _mm_cvtsi128_si64( dq4 ) );
#endif

    return -(popcnt & 0x01);
}

extern __declspec( align( 128 ) ) const unsigned char g_bECCLookupTable[ 256 ] =
{
    0x00, 0x70, 0x61, 0x11, 0x52, 0x22, 0x33, 0x43, 0x43, 0x33, 0x22, 0x52, 0x11, 0x61, 0x70, 0x00,
    0x34, 0x44, 0x55, 0x25, 0x66, 0x16, 0x07, 0x77, 0x77, 0x07, 0x16, 0x66, 0x25, 0x55, 0x44, 0x34,
    0x25, 0x55, 0x44, 0x34, 0x77, 0x07, 0x16, 0x66, 0x66, 0x16, 0x07, 0x77, 0x34, 0x44, 0x55, 0x25,
    0x11, 0x61, 0x70, 0x00, 0x43, 0x33, 0x22, 0x52, 0x52, 0x22, 0x33, 0x43, 0x00, 0x70, 0x61, 0x11,
    0x16, 0x66, 0x77, 0x07, 0x44, 0x34, 0x25, 0x55, 0x55, 0x25, 0x34, 0x44, 0x07, 0x77, 0x66, 0x16,
    0x22, 0x52, 0x43, 0x33, 0x70, 0x00, 0x11, 0x61, 0x61, 0x11, 0x00, 0x70, 0x33, 0x43, 0x52, 0x22,
    0x33, 0x43, 0x52, 0x22, 0x61, 0x11, 0x00, 0x70, 0x70, 0x00, 0x11, 0x61, 0x22, 0x52, 0x43, 0x33,
    0x07, 0x77, 0x66, 0x16, 0x55, 0x25, 0x34, 0x44, 0x44, 0x34, 0x25, 0x55, 0x16, 0x66, 0x77, 0x07,
    0x07, 0x77, 0x66, 0x16, 0x55, 0x25, 0x34, 0x44, 0x44, 0x34, 0x25, 0x55, 0x16, 0x66, 0x77, 0x07,
    0x33, 0x43, 0x52, 0x22, 0x61, 0x11, 0x00, 0x70, 0x70, 0x00, 0x11, 0x61, 0x22, 0x52, 0x43, 0x33,
    0x22, 0x52, 0x43, 0x33, 0x70, 0x00, 0x11, 0x61, 0x61, 0x11, 0x00, 0x70, 0x33, 0x43, 0x52, 0x22,
    0x16, 0x66, 0x77, 0x07, 0x44, 0x34, 0x25, 0x55, 0x55, 0x25, 0x34, 0x44, 0x07, 0x77, 0x66, 0x16,
    0x11, 0x61, 0x70, 0x00, 0x43, 0x33, 0x22, 0x52, 0x52, 0x22, 0x33, 0x43, 0x00, 0x70, 0x61, 0x11,
    0x25, 0x55, 0x44, 0x34, 0x77, 0x07, 0x16, 0x66, 0x66, 0x16, 0x07, 0x77, 0x34, 0x44, 0x55, 0x25,
    0x34, 0x44, 0x55, 0x25, 0x66, 0x16, 0x07, 0x77, 0x77, 0x07, 0x16, 0x66, 0x25, 0x55, 0x44, 0x34,
    0x00, 0x70, 0x61, 0x11, 0x52, 0x22, 0x33, 0x43, 0x43, 0x33, 0x22, 0x52, 0x11, 0x61, 0x70, 0x00,
};

inline ULONG lECCLookup8bit(const ULONG byte)
{
    return g_bECCLookupTable[ byte & 0xff ];
}

XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatAVX;
    Unused( pfn );

    Assert( 32 == sizeof( __m256i ) );
    Assert( 4 == sizeof( ULONG ) );

    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    Assert( 0 == ( ( uintptr_t )pb & ( 256 - 1 ) ) );

    const ULONG cqq = cb / 32;

    ULONG p = 0;

    __m256i qq0 = _mm256_setzero_si256();
    __m256i qq1 = _mm256_setzero_si256();
    __m256i qq2 = _mm256_setzero_si256();
    __m256i qq3 = _mm256_setzero_si256();
    {
        ULONG idxp = 0xfc000000;

        ULONG i = 0;
        __m256i qqL0;

        const __m256i* pqq = ( const __m256i* )pb;
        if ( fHeaderBlock )
        {
            const __m128i* pdq = (const __m128i*) pqq;
            qqL0 = _mm256_castsi128_si256( _mm_unpackhi_epi64( _mm_setzero_si128(), pdq[ 0 ] ) );
            qqL0 = _mm256_insertf128_si256(qqL0, pdq[1], 1);
            goto Start;
        }

        do
        {
            qqL0 = pqq[ i + 0 ];
Start:
            _mm_prefetch( ( char *)&pqq[ i + 16 ], _MM_HINT_NTA );

            const __m256i qqL1 = pqq[ i + 1 ];
            const __m256i qqL2 = pqq[ i + 2 ];
            const __m256i qqL3 = pqq[ i + 3 ];

            qq0 ^= qqL0;
            qq1 ^= qqL1;
            qq2 ^= qqL2;
            qq3 ^= qqL3;
            const __m256i qqLAcc = qqL0 ^ qqL1 ^ qqL2 ^ qqL3;

            _mm_prefetch( ( char *)&pqq[ i + 16 + 2], _MM_HINT_NTA );

            p ^= idxp & lParityMaskAVX( qqLAcc );
            
            idxp += 0xfc000400;

            i += 4;

            __assume( 8 < cqq );
        }
        while ( i < cqq );
    }

    

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

    ULONG r = 0;
    UINT byteT = dwAcc;
    ULONG byte0 = 0;
    ULONG idxr = 0xfff80000;
    for ( ULONG i = 0; i < 4; i++ )
    {
        r ^= idxr & -INT( _mm_popcnt_u32( byteT & 0xff ) & 0x01 );
        byte0 ^= byteT;
        byteT >>= 8;
        idxr += 0xfff80008;
    }

    const LONG bits = lECCLookup8bit( byte0 );
    r |= ( bits & 0x07 );
    r |= ( ( bits << 12 ) & 0x00070000 );

    _mm256_zeroall();

    const ULONG mask = ( cb << 19 ) - 1;

    const ULONG ecc = p & 0xfc00fc00 & mask | q & 0x03000300 | q_ & 0x00e000e0 | r & 0x001f001f;
    const ULONG xor = dwAcc;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}
    
#else

XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    Enforce( fFalse );
    return MakeChecksumFromECCXORAndPgno( 0, 0, pgno );
}

#endif
