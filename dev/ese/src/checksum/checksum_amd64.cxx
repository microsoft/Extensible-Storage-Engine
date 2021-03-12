// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
// There is a header collision between windows.h and emmintrin.h
// To compile this code we put it in a file by itself and only include emmintrin.h
//
// When the bug is fixed, move this code to checksum.cxx
//

#include "checksumstd.hxx"

#if 1

#include <intrin.h>
#if ( defined _M_AMD64 || defined _M_IX86  )
#include <emmintrin.h>
#endif

#else

//  UNDONE: Exchange's ancient compiler doesn't support SSE intrinsics, so we have to use inline
//  assembly and emitted code. When they move to a more modern compiler, we can get rid of this
//  code.

#define ChecksumOldFormatSSE2_Wrapper   ChecksumOldFormatSSE2
#define ChecksumNewFormatSSE2_Wrapper   ChecksumNewFormatSSE2

#endif

//  ChecksumNewFormatSlowly is an implementation of the standard NAND Flash ECC Algorithm (HSIAO code)
//  This file contains different variants of the same algorithm designed for different cpus
//  Here is a simple explanation of the algorithm:
//      A HSIAO code can be thought of as an index to a flipped bit. For example, lets generate a code for 8-bits.
//      So we need 3-bits of code to detect a potential bit-flip. Twice that to correct 1-bit and detect 2-bit flips (Hamming principle).
//      Basically 3-bits of index, and index'
//      Here is how an HSIAO code can be generated for 8-bits.
//      x = XOR bits into the ecc code bit shown to the right.
//      - = ignore.

//      7 6 5 4 3 2 1 0         7 6 5 4 3 2 1 0
//      - x - x - x - x = e0    x - x - x - x - = e0'
//      - - x x - - x x = e1    x x - - x x - - = e1'
//      - - - - x x x x = e2    x x x x - - - - = e2'
//
//      If any bit changes, we recompute the checksum and XOR with the the original checksum. If (e2, e1, e0) = ~(e2', e1', e0'), we know
//      that it was a 1-bit flip, and (e2, e1, e0) will give us the index to the changed bit.

typedef unsigned __int64 XECHECKSUM;

typedef XECHECKSUM  (*PFNCHECKSUMNEWFORMAT)( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG   (*PFNCHECKSUMOLDFORMAT)( const unsigned char * const, const ULONG );

//  ================================================================
inline void CachePrefetch( const void * const p )
//  ================================================================
{
#ifdef _M_IX86 
    _asm
    {
        mov eax,p

        _emit 0x0f  // PrefetchNTA
        _emit 0x18
        _emit 0x00
    }
#endif
}

//  ================================================================
ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Use the prefetch instruction to bring memory into the processor
//  cache prior to use
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE;

    Unused( pfn );

    const ULONG * pdw   = (ULONG *)pb;
    const INT cDWords           = 8;
    const INT cbStep            = cDWords * sizeof( ULONG );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );

    //  touching this memory puts the page in the processor TLB (needed
    //  for prefetch) and brings in the first cacheline (cacheline 0)

    ULONG   dwChecksum = 0x89abcdef ^ pdw[0];

    while ( ( cbT -= cbStep ) >= 0 )
    {
#if (defined _M_AMD64 || defined _M_IX86  )
#if 1
        _mm_prefetch ( (char *)(pdw + 16), _MM_HINT_NTA );
#else
        CachePrefetch ( pdw + 16 );
#endif
#endif
        dwChecksum  ^= pdw[0]
                    ^ pdw[1]
                    ^ pdw[2]
                    ^ pdw[3]
                    ^ pdw[4]
                    ^ pdw[5]
                    ^ pdw[6]
                    ^ pdw[7];
        pdw += cDWords;
    }

    return dwChecksum;
}

#if 1

//  ================================================================
ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  SSE2 supports 128-bit XOR operations. XOR the page using the SSE2
//  instructions and collapse the checksum at the end.
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2;

    Unused( pfn );

#if (defined _M_AMD64 || defined _M_IX86  )

    __m128i owChecksum              = _mm_setzero_si128();
    const   __m128i * pow           = (__m128i *)pb;
    const INT cm128is               = 8;
    const INT cbStep                = cm128is * sizeof( __m128i );
    __int64 cbT                     = cb;
    Assert( 0 == ( cbT % cbStep ) );

    //  checksum the first eight bytes twice to remove the checksum

    const ULONG * pdw       = (ULONG *)pb;
    ULONG   ulChecksum      = 0x89abcdef ^ pdw[0];

    while ( ( cbT -= cbStep ) >= 0 )
    {
        _mm_prefetch ( (char *)(pow + 16), _MM_HINT_NTA );

                __m128i owChecksumT1    = _mm_xor_si128 ( pow[0], pow[1] );
        const   __m128i owChecksumT2    = _mm_xor_si128 ( pow[2], pow[3] );
                __m128i owChecksumT3    = _mm_xor_si128 ( pow[4], pow[5] );
        const   __m128i owChecksumT4    = _mm_xor_si128 ( pow[6], pow[7] );

        owChecksumT1            = _mm_xor_si128 ( owChecksumT1, owChecksumT2 );
        owChecksumT3            = _mm_xor_si128 ( owChecksumT3, owChecksumT4 );

        owChecksum              = _mm_xor_si128 ( owChecksum, owChecksumT1 );
        owChecksum              = _mm_xor_si128 ( owChecksum, owChecksumT3 );

        pow += cm128is;
    }

    ulChecksum  ^=
          owChecksum.m128i_i32[0]
        ^ owChecksum.m128i_i32[1]
        ^ owChecksum.m128i_i32[2]
        ^ owChecksum.m128i_i32[3];

    return ulChecksum;

#else

    //  we should never get here

    return 0x12345678;

#endif
}

#endif

#if 1
#else

//================================================================
static __declspec( naked ) ULONG __stdcall ChecksumOldFormatSSE2_Emitted( const unsigned char * const pb, const ULONG cb )
//================================================================
//
// Exchange's (older) C compiler doesn't support SSE2 intrinsics
//
// The code uses x86 calling convention, it is NOT compatible with AMD64 calling convention
// Under x86, all the xmm regs are caller saved, this is NOT the case for AMD64.
//
// Following is the assembly source, it generates the machine code to be emitted
//
/****************************************************************
.586
.xmm
.model flat

;================================================
; DWORD DwXORChecksumESESSE2( const BYTE * const pb, const DWORD cb )
;   pb should be 128-Byte aligned
;   cb should be multiple of 128
;================================================
PUBLIC  _DwXORChecksumESESSE2@8

_TEXT   SEGMENT
_DwXORChecksumESESSE2@8 PROC NEAR
    mov eax, [esp+4]            ; edx = pb
    mov ecx, [esp+8]            ; ecx = cb
    mov edx, 128                ; edx = stride, cacheline size

    movd xmm0, [eax]            ; first dword is checksum itself
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    pxor xmm4, xmm4
    pxor xmm5, xmm5
    pxor xmm6, xmm6
    pxor xmm7, xmm7

;================================
; Use 8 xmm regs, total 128bytes
;================================
    align 16
xorloop:
    prefetchnta [eax+edx*4] ; prefetch distance can be tuned

    pxor xmm0, [eax+16*0]
    pxor xmm1, [eax+16*1]
    pxor xmm2, [eax+16*2]
    pxor xmm3, [eax+16*3]
    pxor xmm4, [eax+16*4]
    pxor xmm5, [eax+16*5]
    pxor xmm6, [eax+16*6]
    pxor xmm7, [eax+16*7]

    add eax, edx
    sub ecx, edx
    ja xorloop

;================================
; Consolidate 128 Bytes to 16 Bytes
;================================
    pxor xmm0, xmm1
    pxor xmm2, xmm3
    pxor xmm4, xmm5
    pxor xmm6, xmm7
    pxor xmm0, xmm2
    pxor xmm4, xmm6
    pxor xmm0, xmm4

;================================
; Consolidate 16 Bytes to 4 Bytes
;================================
    pshufd xmm1, xmm0, 04eh
    pxor xmm0, xmm1
    pshufd xmm2, xmm0, 0b1h
    pxor xmm0, xmm2

    movd eax, xmm0
    xor eax, 089abcdefh     ; 89abcdefH, just our seed

    ret 8
_DwXORChecksumSSE2Asm@8 ENDP
_TEXT   ENDS

END
****************************************************************/
{
#pragma push_macro( "emit2" )
#pragma push_macro( "emit3" )
#pragma push_macro( "emit4" )
#pragma push_macro( "emit5" )

#define emit2(b1,b2)                __asm _emit b1      __asm _emit b2
#define emit3(b1,b2,b3)         emit2(b1,b2)            __asm _emit b3
#define emit4(b1, b2, b3, b4)       emit3(b1,b2,b3)     __asm _emit b4
#define emit5(b1, b2, b3, b4, b5)   emit4(b1,b2,b3,b4)  __asm _emit b5

    __asm mov eax, [esp+4]
    __asm mov ecx, [esp+8]
    __asm mov edx, 0x80

    emit4( 0x66, 0x0f, 0x6e, 0x00 );            // movd xmm0,dword ptr [eax]
    emit4( 0x66, 0x0f, 0xef, 0xc9 );            // pxor xmm1,xmm1
    emit4( 0x66, 0x0f, 0xef, 0xd2 );            // pxor xmm2,xmm2
    emit4( 0x66, 0x0f, 0xef, 0xdb );            // pxor xmm3,xmm3
    emit4( 0x66, 0x0f, 0xef, 0xe4 );            // pxor xmm4,xmm4
    emit4( 0x66, 0x0f, 0xef, 0xed );            // pxor xmm5,xmm5
    emit4( 0x66, 0x0f, 0xef, 0xf6 );            // pxor xmm6,xmm6
    emit4( 0x66, 0x0f, 0xef, 0xff );            // pxor xmm7,xmm7

    __asm align 16
xorloop:
    emit4( 0x0f, 0x18, 0x04, 0x90 );            // prefetchtnta byte ptr [eax+edx*4]

    emit4( 0x66, 0x0f, 0xef, 0x00 );            // pxor xmm0,oword ptr [eax]
    emit5( 0x66, 0x0f, 0xef, 0x48, 0x10 );      // pxor xmm1,oword ptr [eax+0x10]
    emit5( 0x66, 0x0f, 0xef, 0x50, 0x20 );      // pxor xmm2,oword ptr [eax+0x20]
    emit5( 0x66, 0x0f, 0xef, 0x58, 0x30 );      // pxor xmm3,oword ptr [eax+0x30]
    emit5( 0x66, 0x0f, 0xef, 0x60, 0x40 );      // pxor xmm4,oword ptr [eax+0x40]
    emit5( 0x66, 0x0f, 0xef, 0x68, 0x50 );      // pxor xmm5,oword ptr [eax+0x50]
    emit5( 0x66, 0x0f, 0xef, 0x70, 0x60 );      // pxor xmm6,oword ptr [eax+0x60]
    emit5( 0x66, 0x0f, 0xef, 0x78, 0x70 );      // pxor xmm7,oword ptr [eax+0x70]

    __asm add eax, edx
    __asm sub ecx, edx
    __asm ja xorloop

    emit4( 0x66, 0x0f, 0xef, 0xc1 );            // pxor xmm0,xmm1
    emit4( 0x66, 0x0f, 0xef, 0xd3 );            // pxor xmm2,xmm3
    emit4( 0x66, 0x0f, 0xef, 0xe5 );            // pxor xmm4,xmm5
    emit4( 0x66, 0x0f, 0xef, 0xf7 );            // pxor xmm6,xmm7
    emit4( 0x66, 0x0f, 0xef, 0xc2 );            // pxor xmm0,xmm2
    emit4( 0x66, 0x0f, 0xef, 0xe6 );            // pxor xmm4,xmm6
    emit4( 0x66, 0x0f, 0xef, 0xc4 );            // pxor xmm0,xmm4

    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x4e );      // pshufd xmm1,xmm0,0x4e
    emit4( 0x66, 0x0f, 0xef, 0xc1 );            // pxor xmm0,xmm1
    emit5( 0x66, 0x0f, 0x70, 0xd0, 0xb1 );      // pshufd xmm2,xmm0,0xb1
    emit4( 0x66, 0x0f, 0xef, 0xc2 );            // pxor xmm0,xmm2
    emit4( 0x66, 0x0f, 0x7e, 0xc0 );            // movd eax,xmm0

    __asm xor eax, 0x89abcdef
    __asm ret 0x8
}


//  ================================================================
ULONG ChecksumOldFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  ChecksumOldFormatSSE2_Emitted is a __stdcall function, but that might not be
//  the calling convention we are using. This standard wrapper presents
//  a function with the expected signature
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2_Wrapper;
    return ChecksumOldFormatSSE2_Emitted( pb, cb );
}

#endif

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


//  ================================================================
__declspec( align( 128 ) ) static const signed char g_bParityLookupTable[ 256 ] =
//  ================================================================
{
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0x0x
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0x1x
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0x2x
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0x3x
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0x4x
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0x5x
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0x6x
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0x7x
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0x8x
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0x9x
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0xax
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0xbx
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0xcx
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0xdx
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1, // 0xex
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0, // 0xfx
};


//  ================================================================
inline LONG lParityMask( const ULONG dw )
//  ================================================================
//
// Compute a 32-bit parity mask of a 32-bit integer
//
//-

{
    const ULONG dw1 = dw >> 16;
    const ULONG dw2 = dw ^ dw1;

    const ULONG dw3 = dw2 >> 8;
    const ULONG dw4 = dw2 ^ dw3;

    return g_bParityLookupTable[ dw4 & 0xff ];
}

//  ================================================================
static ULONG DwXORChecksumBasic( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Plain old unrolled-loop checksum that should work on any processor
//
//-
{
    const ULONG * pdw   = (ULONG *)pb;
    const INT cDWords           = 8;
    const INT cbStep            = cDWords * sizeof( ULONG );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );

    //  remove the first QWORD, as it is the checksum itself

    ULONG   dwChecksum = pdw[0] ^ pdw[1];

    while ( ( cbT -= cbStep ) >= 0 )
    {
        dwChecksum  ^= pdw[0]
                    ^ pdw[1]
                    ^ pdw[2]
                    ^ pdw[3]
                    ^ pdw[4]
                    ^ pdw[5]
                    ^ pdw[6]
                    ^ pdw[7];
        pdw += cDWords;
    }

    return dwChecksum;
}


//  ================================================================
XECHECKSUM ChecksumNewFormatBasic( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  optmized ECC function for 32-bit CPU, 32-byte cache line, no prefetching
//  the data is organized into an array of bits that is 32*4=128 bits wide
//  each line of CPU cache will hold 2 rows of data
//
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatBasic;

    Unused( pfn );

    // data size sanity check
    //
    Assert( 4 == sizeof( ULONG ) );

    // cb should be an order of 2 and between 1k and 8k
    //
    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    // pb should align to 32 (2 rows of CPU cache)
    //
    Assert( 0 == ( ( uintptr_t )pb & ( 32 - 1 ) ) );

    //================================================
    // long* is easier to use than char*
    //
    const ULONG* pdw = ( ULONG* )pb;
    const ULONG cdw = cb / sizeof( *pdw );

    //================================================
    // compute ECC bits shown as '1'
    // 1111 1111 1000 0000 1111 1111 1000 0000
    //
    ULONG p = 0;

    // p0, p1, p2, p3 hold accumulated column parity bits (32*4=128 bits)
    //
    unsigned p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    {
        // combine 16-bit index and ~index together into a 32-bit double index
        //
        ULONG idxp = 0xff800000;

        // loop control variable
        //
        ULONG i = 0;

        // treat the first 64-bit of the data always as 0
        //
        ULONG pT0 = 0;
        ULONG pT1 = 0;

        if ( fHeaderBlock )
        {
            goto Start;
        }

        // unroll the loop to 32-byte cache line boundary
        // one iteration will process 2 rows of data in one cache line.
        //
        do
        {
            // read one row (first half of the cache line) from data (32*4=128 bits)
            //
            pT0 = pdw[ i + 0 ];
            pT1 = pdw[ i + 1 ];
Start:
            const ULONG pT2 = pdw[ i + 2 ];
            const ULONG pT3 = pdw[ i + 3 ];

            // update accumulated column parity bits
            //
            p0 ^= pT0;
            p1 ^= pT1;
            p2 ^= pT2;
            p3 ^= pT3;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( pT0 ^ pT1 ^ pT2 ^ pT3 );

            // update double index
            //
            idxp += 0xff800080;

            // read another row (second half of the cache line) from data (32*4=128 bits)
            //
            const ULONG pT4 = pdw[ i + 4 ];
            const ULONG pT5 = pdw[ i + 5 ];
            const ULONG pT6 = pdw[ i + 6 ];
            const ULONG pT7 = pdw[ i + 7 ];

            // update accumulated column parity bits
            //
            p0 ^= pT4;
            p1 ^= pT5;
            p2 ^= pT6;
            p3 ^= pT7;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( pT4 ^ pT5 ^ pT6 ^ pT7 );

            // update double index
            //
            idxp += 0xff800080;

            // update loop control variable
            //
            i += 8;

            // inform compiler the loop won't quit in 1st iteration.
            //
            __assume( 8 < cdw );
        }while ( i < cdw );
    }

    //================================================
    // reduce 128 column parity bits (p0, p1, p2, p3) into 32 bits
    //

    // compute ECC bits shown as '1'
    // 0000 0000 0110 0000 0000 0000 0110 0000
    //
    p |= 0x00400000 & lParityMask( p0 ^ p1 );
    p |= 0x00000040 & lParityMask( p2 ^ p3 );

    const ULONG r0 = p0 ^ p2;
    const ULONG r1 = p1 ^ p3;

    p |= 0x00200000 & lParityMask( r0 );
    p |= 0x00000020 & lParityMask( r1 );

    // r2 holds 32-bit reduced column parity bits
    //
    const ULONG r2 = r0 ^ r1;

    //================================================
    // compute ECC bits shown as '1'
    // 0000 0000 0001 1111 0000 0000 0001 1111
    //
    ULONG r = 0;

    // double index
    //
    ULONG idxr = 0xffff0000;

    // loop through every column parity bit
    //
    for ( ULONG i = 1; i; i += i )
    {
        const LONG mask = -!!( r2 & i );

        r ^= mask & idxr;
        idxr += 0xffff0001;
    }

    //================================================
    // mask some high bits out according to the data size
    //
    const ULONG mask = ( cb << 19 ) - 1;

    // assemble final ECC
    //
    const ULONG eccChecksum = p & 0xffe0ffe0 & mask | r & 0x001f001f;
    const ULONG xorChecksum = DwXORChecksumBasic( pb, cb );
    return MakeChecksumFromECCXORAndPgno( eccChecksum, xorChecksum, pgno );
}

//  ================================================================
XECHECKSUM ChecksumNewFormatSlowly( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  optmized ECC function for 32-bit CPU, 32-byte cache line, no prefetching
//  the data is organized into an array of bits that is 32*4=128 bits wide
//  each line of CPU cache will hold 2 rows of data
//
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSlowly;

    Unused( pfn );

    //  make sure lParityMask is working

    AssertRTL( 0 == lParityMask( 0 ) );
    AssertRTL( 0xFFFFFFFF == lParityMask( 1 ) );

    // data size sanity check
    //
    Assert( 4 == sizeof( ULONG ) );

    // cb should be an order of 2 and between 1k and 8k
    //
    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    // pb should align to 32 (2 rows of CPU cache)
    //
    Assert( 0 == ( ( uintptr_t )pb & ( 32 - 1 ) ) );

    //================================================
    // long* is easier to use than char*
    //
    const ULONG* pdw = ( ULONG* )pb;
    const ULONG cdw = cb / sizeof( *pdw );

    //================================================
    // compute ECC bits shown as '1'
    // 1111 1111 1000 0000 1111 1111 1000 0000
    //
    ULONG p = 0;

    // p0, p1, p2, p3 hold accumulated column parity bits (32*4=128 bits)
    //
    ULONG p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    {
        // combine 16-bit index and ~index together into a 32-bit double index
        //
        ULONG idxp = 0xff800000;

        // loop control variable
        //
        ULONG i = 0;

        // treat first and second 32-bit of the data always as 0
        ULONG pT0 = 0;
        ULONG pT1 = 0;

        // skip first 8 byte when checksum region is at the beginning of a page
        if ( fHeaderBlock )
        {
            goto Start;
        }

        // unroll the loop to 32-byte cache line boundary
        // one iteration will process 2 rows of data in one cache line.
        //
        do
        {
            // read one row (first half of the cache line) from data (32*4=128 bits)
            //
            pT0 = pdw[ i + 0 ];
            pT1 = pdw[ i + 1 ];
Start:
            const ULONG pT2 = pdw[ i + 2 ];
            const ULONG pT3 = pdw[ i + 3 ];

            // update accumulated column parity bits
            //
            p0 ^= pT0;
            p1 ^= pT1;
            p2 ^= pT2;
            p3 ^= pT3;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( pT0 ^ pT1 ^ pT2 ^ pT3 );

            // update double index
            //
            idxp += 0xff800080;

            // read another row (second half of the cache line) from data (32*4=128 bits)
            //
            const ULONG pT4 = pdw[ i + 4 ];
            const ULONG pT5 = pdw[ i + 5 ];
            const ULONG pT6 = pdw[ i + 6 ];
            const ULONG pT7 = pdw[ i + 7 ];

            // update accumulated column parity bits
            //
            p0 ^= pT4;
            p1 ^= pT5;
            p2 ^= pT6;
            p3 ^= pT7;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( pT4 ^ pT5 ^ pT6 ^ pT7 );

            // update double index
            //
            idxp += 0xff800080;

            // update loop control variable
            //
            i += 8;

            // inform compiler the loop won't quit in 1st iteration.
            //
            __assume( 8 < cdw );
        }       while ( i < cdw );
    }

    //================================================
    // reduce 128 column parity bits (p0, p1, p2, p3) into 32 bits
    //

    // compute ECC bits shown as '1'
    // 0000 0000 0110 0000 0000 0000 0110 0000
    //
    p |= 0x00400000 & lParityMask( p0 ^ p1 );
    p |= 0x00000040 & lParityMask( p2 ^ p3 );

    const ULONG r0 = p0 ^ p2;
    const ULONG r1 = p1 ^ p3;

    p |= 0x00200000 & lParityMask( r0 );
    p |= 0x00000020 & lParityMask( r1 );

    // r2 holds 32-bit reduced column parity bits
    //
    const ULONG r2 = r0 ^ r1;

    //================================================
    // compute ECC bits shown as '1'
    // 0000 0000 0001 1111 0000 0000 0001 1111
    //
    ULONG r = 0;

    // double index
    //
    ULONG idxr = 0xffff0000;

    // loop through every column parity bit
    //
    for ( ULONG i = 1; i; i += i )
    {
        const LONG mask = -!!( r2 & i );

        r ^= mask & idxr;
        idxr += 0xffff0001;
    }

    //================================================
    // mask some high bits out according to the data size
    //
    const ULONG mask = ( cb << 19 ) - 1;

    // assemble final checksum
    //
    const ULONG ecc = p & 0xffe0ffe0 & mask | r & 0x001f001f;
    const ULONG xor = r2;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}


//  ================================================================
XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock  )
//  ================================================================
//
//  optmized ECC function for 32-bit CPU, 32-byte cache line, prefetching
//  the data is organized into an array of bits that is 32*4=128 bits wide
//  each line of CPU cache will hold 2 rows of data
//
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE;

    Unused( pfn );

    // data size sanity check
    //
    Assert( 4 == sizeof( ULONG ) );

    // cb should be an order of 2 and between 1k and 8k
    //
    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    // pb should align to 32 (2 rows of CPU cache)
    //
    Assert( 0 == ( ( uintptr_t )pb & ( 32 - 1 ) ) );

    //================================================
    // long* is easier to use than char*
    //
    const ULONG* pdw = ( ULONG* )pb;
    const ULONG cdw = cb / sizeof( *pdw );

    //================================================
    // compute ECC bits shown as '1'
    // 1111 1111 1000 0000 1111 1111 1000 0000
    //
    ULONG p = 0;

    // p0, p1, p2, p3 hold accumulated column parity bits (32*4=128 bits)
    //
    unsigned p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    {
        // combine 16-bit index and ~index together into a 32-bit double index
        //
        ULONG idxp = 0xff800000;

        // loop control variable
        //
        ULONG i = 0;

        // treat the first and second 32-bit of the data always as 0
        //
        ULONG pT0 = 0;
        ULONG pT1 = 0;

        // skip first 8 byte when checksum region is at the beginning of a page
        if ( fHeaderBlock )
        {
            goto Start;
        }

        // unroll the loop to 32-byte cache line boundary
        // one iteration will process 2 rows of data in one cache line.
        //
        do
        {
            // read one row (first half of the cache line) from data (32*4=128 bits)
            //
            pT0 = pdw[ i + 0 ];
            pT1 = pdw[ i + 1 ];
Start:

#if (defined _M_AMD64 || defined _M_IX86  )
#if 1
            _mm_prefetch( ( char *)&( pdw[ i + 32 ] ), _MM_HINT_NTA );
#else
            CachePrefetch( ( char *)&( pdw[ i + 32 ] ) );
#endif
#endif

            const ULONG pT2 = pdw[ i + 2 ];
            const ULONG pT3 = pdw[ i + 3 ];

            // update accumulated column parity bits
            //
            p0 ^= pT0;
            p1 ^= pT1;
            p2 ^= pT2;
            p3 ^= pT3;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( pT0 ^ pT1 ^ pT2 ^ pT3 );

            // update double index
            //
            idxp += 0xff800080;

            // read another row (second half of the cache line) from data (32*4=128 bits)
            //
            const ULONG pT4 = pdw[ i + 4 ];
            const ULONG pT5 = pdw[ i + 5 ];
            const ULONG pT6 = pdw[ i + 6 ];
            const ULONG pT7 = pdw[ i + 7 ];

            // update accumulated column parity bits
            //
            p0 ^= pT4;
            p1 ^= pT5;
            p2 ^= pT6;
            p3 ^= pT7;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( pT4 ^ pT5 ^ pT6 ^ pT7 );

            // update double index
            //
            idxp += 0xff800080;

            // update loop control variable
            //
            i += 8;

            // inform compiler the loop won't quit in 1st iteration.
            //
            __assume( 8 < cdw );
        }
        while ( i < cdw );
    }

    //================================================
    // reduce 128 column parity bits (p0, p1, p2, p3) into 32 bits
    //

    // compute ECC bits shown as '1'
    // 0000 0000 0110 0000 0000 0000 0110 0000
    //
    p |= 0x00400000 & lParityMask( p0 ^ p1 );
    p |= 0x00000040 & lParityMask( p2 ^ p3 );

    const ULONG r0 = p0 ^ p2;
    const ULONG r1 = p1 ^ p3;

    p |= 0x00200000 & lParityMask( r0 );
    p |= 0x00000020 & lParityMask( r1 );

    // r2 holds 32-bit reduced column parity bits
    //
    const ULONG r2 = r0 ^ r1;

    //================================================
    // compute ECC bits shown as '1'
    // 0000 0000 0001 1111 0000 0000 0001 1111
    //
    ULONG r = 0;

    // double index
    //
    ULONG idxr = 0xffff0000;

    // loop through every column parity bit
    //
    for ( ULONG i = 1; i; i += i )
    {
        const LONG mask = -!!( r2 & i );

        r ^= mask & idxr;
        idxr += 0xffff0001;
    }

    //================================================
    // mask some high bits out according to the data size
    //
    const ULONG mask = ( cb << 19 ) - 1;

    // assemble final checksum
    //
    const ULONG ecc = p & 0xffe0ffe0 & mask | r & 0x001f001f;
    const ULONG xor = r2;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}


#if 1

enum ChecksumParityMaskFunc
{
    ParityMaskFuncDefault = 0,
    ParityMaskFuncPopcnt,
};

#if ( defined _M_AMD64 || defined _M_IX86  ) && !defined _CHPE_X86_ARM64_

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

//  ================================================================
inline LONG lParityMask( const __m128i dq )
//  ================================================================
//
//  Compute a 32-bit parity mask of a 128-bit integer
//
{
    const __m128i dq1 = _mm_shuffle_epi32( dq, 0x4e );
    const __m128i dq2 = dq ^ dq1;

    const __m128i dq3 = _mm_shuffle_epi32( dq2, 0x1b );
    const __m128i dq4 = dq2 ^ dq3;

    return lParityMask( _mm_cvtsi128_si32( dq4 ) );
}

//  ================================================================
inline LONG lParityMaskPopcnt( const __m128i dq )
//  ================================================================
//
//  Compute a 32-bit parity mask of a 128-bit integer using POPCNT instruction
//  This instruction is part of SSE4.2 on intel processors (Core2, Core i3/5/7)
//  AMD architectures K10 and newer support it even though they might not support SSE4
//
{
    const __m128i dq1 = _mm_shuffle_epi32( dq, 0x4e);
    const __m128i dq2 = dq ^ dq1;

#if ( defined _M_IX86  )
    // reduce to 32-bits
    const __m128i dq3 = _mm_shuffle_epi32( dq2, 0x1b );
    const __m128i dq4 = dq2 ^ dq3;
    INT popcnt = _mm_popcnt_u32( _mm_cvtsi128_si32( dq4 ) );
#else
    INT popcnt = (INT) _mm_popcnt_u64( _mm_cvtsi128_si64( dq2 ) );
#endif

    return -(popcnt & 0x01);
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

//  A table containing 3-bit parity values for a byte.
//  Contains 6-bits of data (r and r') required to compute an ECC checksum for a byte.
//  ================================================================
extern __declspec( align( 128 ) ) const unsigned char g_bECCLookupTable[ 256 ];
//  ================================================================

inline ULONG lECCLookup8bit(const ULONG byte)
{
    return g_bECCLookupTable[ byte & 0xff ];
}

//  ================================================================
template <ChecksumParityMaskFunc TParityMaskFunc>
XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  Optimized ECC function for P4 (SSE2 + 64B L2 + Prefetching)
//  the data is organized into an array of bits that is 128*4=512 bits wide
//  each line of CPU cache will hold 1 rows of data
//
//  Divides the ECC-parity bits into 3 sets:
//  p = row parity 7-bits (index to a 512-bit row)
//  q = word parity 4-bits (index to a 32-bit word in a row)
//  r = column parity 5-bits (index to a bit in 32-bit word column)
//  Each set is composed of index, index` as explained above in the algo description
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2<TParityMaskFunc>;

    Unused( pfn );

    // data size sanity check
    //
    Assert( 16 == sizeof( __m128i ) );
    Assert( 4 == sizeof( ULONG ) );

    // cb should be an order of 2 and between 1k and 8k
    //
    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    // pb should align to 128 (2 rows of CPU cache)
    //
    Assert( 0 == ( ( uintptr_t )pb & ( 128 - 1 ) ) );

    // get double quadword count of the data block
    //
    const ULONG cdq = cb / 16;

    //================================================
    // compute ECC bits shown as '1' (p and p')
    // 1111 1110 0000 0000 1111 1110 0000 0000
    //
    ULONG p = 0;

    // dq0, dq1, dq2, dq3 hold accumulated column parity bits (128*4=512 bits)
    // for calculating q and r later
    //
    __m128i dq0 = _mm_setzero_si128();
    __m128i dq1 = _mm_setzero_si128();
    __m128i dq2 = _mm_setzero_si128();
    __m128i dq3 = _mm_setzero_si128();
    {
        // combine 16-bit index and index' together into a 32-bit double index
        //
        ULONG idxp = 0xfe000000;

        // loop control variable
        //
        ULONG i = 0;
        __m128i dqL0;


        // treat the first and second 32-bit of the data always as 0
        //
        const __m128i* pdq = ( const __m128i* )pb;
        if ( fHeaderBlock )
        {
            dqL0 = _mm_unpackhi_epi64( _mm_setzero_si128(), pdq[ 0 ] );
            goto Start;
        }

        // unroll the loop to 128-byte cache line boundary
        // one iteration will process 2 rows of data in one cache line.
        //
        do
        {
            // read one row (first half of the cache line) from data (128*4=512 bits)
            //
            dqL0 = pdq[ i + 0 ];
Start:
            // Each iteration of the loop consumes 2 64-byte cache lines, so prefetch 2 lines every iteration
            // This would have the effect of pre-caching the next 8 cache-lines.
            //
            _mm_prefetch( ( char *)&pdq[ i + 32 ], _MM_HINT_NTA );
            _mm_prefetch( ( char *)&pdq[ i + 32 + 4], _MM_HINT_NTA );

            const __m128i dqL1 = pdq[ i + 1 ];
            const __m128i dqL2 = pdq[ i + 2 ];
            const __m128i dqL3 = pdq[ i + 3 ];
            LONG parityL, parityH;

            // update accumulated column parity bits for calculating q and r later
            //
            dq0 ^= dqL0;
            dq1 ^= dqL1;
            dq2 ^= dqL2;
            dq3 ^= dqL3;
            const __m128i dqLAcc = dqL0 ^ dqL1 ^ dqL2 ^ dqL3;

            // compute row parity and distribute it according to its index
            //
            if ( TParityMaskFunc == ParityMaskFuncPopcnt )
            {
                parityL = lParityMaskPopcnt( dqLAcc );
            }
            else
            {
                parityL = lParityMask( dqLAcc );
            }
            p ^= idxp & parityL;

            // the magic numbers distribute the parity according to the algorithm
            // decrements the upper bits and increments the lower bits
            idxp += 0xfe000200; // update double index

            // read another row (second cache line) from data (128*4=512 bits)
            //
            const __m128i dqH0 = pdq[ i + 4 ];
            const __m128i dqH1 = pdq[ i + 5 ];
            const __m128i dqH2 = pdq[ i + 6 ];
            const __m128i dqH3 = pdq[ i + 7 ];

            // update accumulated column parity bits
            //
            dq0 ^= dqH0;
            dq1 ^= dqH1;
            dq2 ^= dqH2;
            dq3 ^= dqH3;
            const __m128i dqHAcc = dqH0 ^ dqH1 ^ dqH2 ^ dqH3;

            // compute row parity and distribute it according to its index
            //
            if ( TParityMaskFunc == ParityMaskFuncPopcnt )
            {
                parityH = lParityMaskPopcnt( dqHAcc );
            }
            else
            {
                parityH = lParityMask( dqHAcc );
            }
            p ^= idxp & parityH;
            idxp += 0xfe000200; // update double index

            // update loop control variable
            //
            i += 8;

            // inform compiler the loop won't quit in 1st iteration.
            //
            __assume( 8 < cdq );
        }
        while ( i < cdq );
    }

    //================================================
    // Compute q and q' bits. Assume the block has 32-bit wide rows now (instead of earlier 512-bits).
    // Assume 128x4 XORs are 32x16 XORs.
    //
    __declspec( align( 128 ) ) __m128i arydq[ 4 ];
    ULONG* pdw = ( ULONG* )arydq;

    arydq[ 0 ] = dq0;
    arydq[ 1 ] = dq1;
    arydq[ 2 ] = dq2;
    arydq[ 3 ] = dq3;

    // compute ECC bits shown as '1'
    // 0000 0001 1110 0000 0000 0001 1110 0000
    //
    ULONG q = 0;

    // dw0 holds 32-bit reduced column parity bits
    //
    ULONG dw0 = 0;

    // double index
    //
    ULONG idxq = 0xffe00000;

    for ( ULONG i = 0; i < 16; ++i )
    {
        ULONG dwT = pdw[ i ];
        dw0 ^= dwT;

        if ( TParityMaskFunc == ParityMaskFuncPopcnt )
            q ^= idxq & lParityMaskPopcnt( dwT );
        else
            q ^= idxq & lParityMask( dwT );
        idxq += 0xffe00020;
    }

    //================================================
    // compute ECC bits shown as '1' (r and r')
    // 0000 0000 0001 1000 0000 0000 0001 1000
    // Assume the block has 32-bit wide rows now
    //
    ULONG r = 0;
    ULONG byteT = dw0;
    ULONG byte0 = 0;
    ULONG idxr = 0xfff80000;
    for ( ULONG i = 0; i < 4; i++ ) // compiler should unroll hopefully
    {
        if ( TParityMaskFunc == ParityMaskFuncPopcnt)
        {
            r ^= idxr & lParityMaskPopcnt( byteT & 0xff );
        }
        else
        {
            const char parity = g_bParityLookupTable[ byteT & 0xff ];
            r ^= idxr & (LONG) parity;
        }
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

    //================================================
    // mask some high bits out according to the data size
    //
    const ULONG mask = ( cb << 19 ) - 1;

    // assemble final checksum
    //
    const ULONG ecc = p & 0xfe00fe00 & mask | q & 0x01e001e0 | r & 0x001f001f;
    const ULONG xor = dw0;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}

template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncDefault>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );
template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );

// Imported from checksum_avx.lib. Need this because the lib has to be built with /arch:AVX switch
XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );

#else

//  ================================================================
template <ChecksumParityMaskFunc TParityMaskFunc>
XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  stub used for IA64 builds (function ptr never called)
//
//-
{
    Enforce( fFalse );  // Should never be selected by SelectChecksumNewFormat() if it isn't available
    return MakeChecksumFromECCXORAndPgno( 0, 0, pgno );
}

template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncDefault>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );
template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );

#endif
#endif

//  ================================================================
XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormat64Bit;

    Unused( pfn );

    //  the data will be organized into an array of bits that is 32 bits wide
    //
    const ULONG*    rgdwRow = ( ULONG* )pb;
    const ULONG cColumn     = 32;
    const ULONG cRow        = cb / ( cColumn / 8 );

    //  this will hold the column parity bits which we will accumulate while
    //  computing the row values of the ECC
    //
    ULONG dwColumnParity = 0;

    //  the first and second row (8-byte in total) will be the ECC and XOR checksum,
    //  we cannot include them in the computation, so, we treat them as always 0
    //
    ULONG p = 0, q = 0;
    ULONG iRow = 0;

    //  compute the remainder of the row values of the ECC from the block
    //
    for ( iRow = fHeaderBlock ? 2 : 0; iRow < cRow; ++iRow )
    {
        //  read this row from the block
        //
        const ULONG dwRow = rgdwRow[ iRow ];

        //  accumulate the column parity for this row
        //
        dwColumnParity ^= dwRow;

        //  expand the row parity value into a mask
        //
        const LONG maskRow = lParityMask( dwRow );

        //  incorporate this row's parity into the row values of the ECC
        //
        p ^= maskRow & iRow;
        q ^= maskRow & ~iRow;
    }

    //  compute the column values of the ECC from the accumulated column parity bits
    //
    ULONG r = 0, s = 0;
    ULONG iColumn = 0;
    for ( iColumn = 0; iColumn < cColumn; ++iColumn )
    {
        //  expand the desired bit from the column parity values into a mask
        //
        const LONG maskColumn = -( !!( dwColumnParity & ( 1 << iColumn ) ) );

        //  incorporate this column's parity into the column values of the ECC
        //
        r ^= maskColumn & iColumn;
        s ^= maskColumn & ~iColumn;
    }

    //  the ECC will be encoded as a 32 bit word that will look like this:
    //
    //    3         22   11         00   0
    //    1         10   65         54   0
    //    qqqqqqqqqqqsssssppppppppppprrrrr
    //
    //  For an 8k block size: 32bit ECC code = 22bit row parity, 10bit column parity
    //  Legend:
    //  q = row-parity'
    //  p = row-parity
    //  s = column-parity'
    //  r = column-parity
    //
    //  the upper bits of p and q may not be used depending on the size of the
    //  block.  and 8kb block will use all the bits.  each block size below
    //  that will use one fewer bit in both p and q.  any unused bits in the
    //  ECC will always be zero
    //
    //  the ECC is used to detect errors using the XOR of the expected value of
    //  the ECC and the actual value of the ECC as follows:
    //
    //    all bits are clear:              no error
    //    one bit is set:                  one bit flip in the ECC was detected
    //    count of bits set = used bits
    //        (=16 for 8k block):          one bit flip in the block was detected
    //    random bits are set:             more than one bit flip was detected
    //
    //  if a bit flip in the ECC was detected then the ECC should simply be
    //  corrected to the expected value of the ECC
    //
    //  if a bit flip in the block was detected then the offset of that bit in
    //  the block is equal to the least significant 16 bits of the XORed ECC
    //  values
    //
    const ULONG ulEcc = ( ( q & ( cRow - 1 ) ) << 21 )  |
                            ( ( s & ( cColumn - 1 ) ) << 16 )   |
                            ( ( p & ( cRow - 1 ) ) << 5 )       |
                            ( ( r & ( cColumn - 1 ) ) << 0 )    |
                            0;
    const ULONG ulXor = dwColumnParity;
    return MakeChecksumFromECCXORAndPgno( ulEcc, ulXor, pgno );
}


#if 1
#else

//================================================================
static __declspec( naked ) XECHECKSUM __stdcall ChecksumSSE2_Emitted( const unsigned char* pb, const ULONG cb )
//================================================================
//
// we need to be 100% correct about the number of PUSHes inside the assembly routine
// since we access local variable/buffers based on ESP
//
// update STKOFF if local variable/buffers is changed
//
//
//================================================
// latency and throughput for some instructions on P4
//================================================
//  movd xmm, r32           6   2   MMX_MISC, MMX_SHFT
//  movd r32, xmm           10  1   FP_MOVE, FP_MISC
//  movdqa xmm, xmm     6   1   FP_MOVE
//  pshufd xmm, xmm, imm8   4   2   MMX_SHFT
//  psrldq xmm, imm8            4   2   MMX_SHFT
//  pxor                        2   2   MMX_ALU
//  SAL/SAR/SHL/SHR     4   1
//  SETcc                   4   1.5 ALU
//  CALL                    5   1   ALU, MEM_STORE
//
//================================================
//
//-
{
#define STKOFF ( 4 * 6 )

#pragma push_macro( "emit2" )
#pragma push_macro( "emit3" )
#pragma push_macro( "emit4" )
#pragma push_macro( "emit5" )

#define emit2(b1,b2)                        __asm _emit b1              __asm _emit b2
#define emit3(b1,b2,b3)                     emit2(b1,b2)                __asm _emit b3
#define emit4(b1, b2, b3, b4)               emit3(b1,b2,b3)             __asm _emit b4
#define emit5(b1, b2, b3, b4, b5)           emit4(b1,b2,b3,b4)          __asm _emit b5
#define emit6(b1, b2, b3, b4, b5, b6)       emit5(b1,b2,b3,b4, b5)      __asm _emit b6
#define emit7(b1, b2, b3, b4, b5, b6, b7 )  emit6(b1,b2,b3,b4, b5, b6)  __asm _emit b7

    __asm mov eax, [ esp + 4 ]          // pb
    __asm mov ecx, [ esp + 8 ]          // cb

    __asm mov edx, esp              // save esp in edx
    __asm and esp, 0xfffffff0           // align esp to 16-byte boundary
    __asm sub esp, 16 * 4               // 64-byte local buffer

    // treat first 8-Byte always as 0
    emit4( 0x66, 0x0f, 0xef, 0xc0 );    // pxor xmm0,xmm0
    emit4( 0x66, 0x0f, 0x6d, 0x00 );    // punpckhqdq xmm0,oword ptr [eax]

    __asm push edx                  // edx(esp)
    __asm push ebx                  //
    __asm push ebp                  // save callee saved regs
    __asm push esi                  //
    __asm push edi                  //

    //================================================
    // cb-byte to 64-byte
    //
    __asm push ecx
    __asm shr ecx, 7                        // count
    __asm xor ebx, ebx                  // ecc
    __asm mov edx, 0xfe000000           // double index
    __asm mov ebp, 0xfe000200           // double index delta

    emit4( 0x66, 0x0f, 0xef, 0xe4 );        // pxor xmm4,xmm4
    emit4( 0x66, 0x0f, 0xef, 0xed );        // pxor xmm5,xmm5
    emit4( 0x66, 0x0f, 0xef, 0xf6 );        // pxor xmm6,xmm6
    emit4( 0x66, 0x0f, 0xef, 0xff );        // pxor xmm7,xmm7
    __asm jmp loop_page_to_64B_start

loop_page_to_64B:
    emit4( 0x66, 0x0f, 0x6f, 0x00 );        // movdqa xmm0,oword ptr [eax]
loop_page_to_64B_start:
    emit5( 0x66, 0x0f, 0x6f, 0x48, 0x10 );  // movdqa xmm1,oword ptr [eax+0x10]
    emit5( 0x66, 0x0f, 0x6f, 0x50, 0x20 );  // movdqa xmm2,oword ptr [eax+0x20]
    emit5( 0x66, 0x0f, 0x6f, 0x58, 0x30 );  // movdqa xmm3,oword ptr [eax+0x30]

    emit7( 0x0f, 0x18, 0x80, 0x00, 0x02, 0x00, 0x00 );      // prefetchnta byte ptr [eax+0x200]

    emit4( 0x66, 0x0f, 0xef, 0xe0 );        // pxor xmm4,xmm0
    emit4( 0x66, 0x0f, 0xef, 0xe9 );        // pxor xmm5,xmm1
    emit4( 0x66, 0x0f, 0xef, 0xf2 );        // pxor xmm6,xmm2
    emit4( 0x66, 0x0f, 0xef, 0xfb );        // pxor xmm7,xmm3
    emit4( 0x66, 0x0f, 0xef, 0xc1 );        // pxor xmm0,xmm1
    emit4( 0x66, 0x0f, 0xef, 0xd3 );        // pxor xmm2,xmm3
    emit4( 0x66, 0x0f, 0xef, 0xc2 );        // pxor xmm0,xmm2

    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x4e );  // pshufd xmm1,xmm0,0x4e
    emit4( 0x66, 0x0f, 0xef, 0xc1 );        // pxor xmm0,xmm1
    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x1b );  // pshufd xmm1,xmm0,0x1b
    emit4( 0x66, 0x0f, 0xef, 0xc1 );        // pxor xmm0,xmm1

    emit4( 0x66, 0x0f, 0x7e, 0xc6 );        // movd esi, xmm0
    __asm mov edi, esi
    __asm shr edi, 16
    __asm xor esi, edi
    __asm mov edi, esi
    __asm shr edi, 8
    __asm xor esi, edi
    __asm and esi, 0xff

    __asm movsx esi, [ g_bParityLookupTable + esi ]
    __asm and esi, edx
    __asm add edx, ebp
    __asm xor ebx, esi

    emit5( 0x66, 0x0f, 0x6f, 0x40, 0x40 );  // movdqa xmm0,oword ptr [eax+0x40]
    emit5( 0x66, 0x0f, 0x6f, 0x48, 0x50 );  // movdqa xmm1,oword ptr [eax+0x50]
    emit5( 0x66, 0x0f, 0x6f, 0x50, 0x60 );  // movdqa xmm2,oword ptr [eax+0x60]
    emit5( 0x66, 0x0f, 0x6f, 0x58, 0x70 );  // movdqa xmm3,oword ptr [eax+0x70]

    emit4( 0x66, 0x0f, 0xef, 0xe0 );        // pxor xmm4,xmm0
    emit4( 0x66, 0x0f, 0xef, 0xe9 );        // pxor xmm5,xmm1
    emit4( 0x66, 0x0f, 0xef, 0xf2 );        // pxor xmm6,xmm2
    emit4( 0x66, 0x0f, 0xef, 0xfb );        // pxor xmm7,xmm3
    emit4( 0x66, 0x0f, 0xef, 0xc1 );        // pxor xmm0,xmm1
    emit4( 0x66, 0x0f, 0xef, 0xd3 );        // pxor xmm2,xmm3
    emit4( 0x66, 0x0f, 0xef, 0xc2 );        // pxor xmm0,xmm2

    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x4e );  // pshufd xmm1,xmm0,0x4e
    emit4( 0x66, 0x0f, 0xef, 0xc1 );        // pxor xmm0,xmm1
    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x1b );  // pshufd xmm1,xmm0,0x1b
    emit4( 0x66, 0x0f, 0xef, 0xc1 );        // pxor xmm0,xmm1

    emit4( 0x66, 0x0f, 0x7e, 0xc6 );        // movd esi, xmm0
    __asm mov edi, esi
    __asm shr edi, 16
    __asm xor esi, edi
    __asm mov edi, esi
    __asm shr edi, 8
    __asm xor esi, edi
    __asm and esi, 0xff

    __asm movsx esi, [ g_bParityLookupTable + esi ]
    __asm and esi, edx
    __asm add edx, ebp
    __asm xor ebx, esi

    __asm sub eax, -128
    __asm dec ecx
    __asm jnz loop_page_to_64B

    //================================================
    // 64-byte to 4-byte
    //
    emit6( 0x66, 0x0f, 0x7f, 0x64, 0x24, STKOFF + 0x00 );   // movdqa oword ptr [esp+0x18],xmm4
    emit6( 0x66, 0x0f, 0x7f, 0x6c, 0x24, STKOFF + 0x10 );   // movdqa oword ptr [esp+0x28],xmm5
    emit6( 0x66, 0x0f, 0x7f, 0x74, 0x24, STKOFF + 0x20 );   // movdqa oword ptr [esp+0x38],xmm6
    emit6( 0x66, 0x0f, 0x7f, 0x7c, 0x24, STKOFF + 0x30 );   // movdqa oword ptr [esp+0x48],xmm7

    __asm pop ecx
    __asm and ebx, 0xfe00fe00
    __asm shl ecx, 19
    __asm dec ecx
    __asm and ebx, ecx
    __asm push ebx                          // save 1st part ECC

    __asm lea eax, [ esp + STKOFF ]             // address
    __asm mov ecx, 16                       // count
    __asm xor edx, edx                      // dw
    __asm xor ebx, ebx                      // ecc
    __asm mov ebp, 0xffe00000               // double index

loop_64B_to_4B:
    __asm mov esi, [ eax ]
    __asm xor edx, esi

    __asm mov edi, esi
    __asm shr edi, 16
    __asm xor esi, edi
    __asm mov edi, esi
    __asm shr edi, 8
    __asm xor esi, edi
    __asm and esi, 0xff

    __asm movsx esi, [ g_bParityLookupTable + esi ]
    __asm and esi, ebp
    __asm add ebp, 0xffe00020
    __asm xor ebx, esi

    __asm add eax, 4
    __asm dec ecx
    __asm jnz loop_64B_to_4B

    __asm and ebx, 0x01e001e0
    __asm push ebx                  // save 2nd part ECC

    //================================================
    // last 32-bit
    // edx = xor checksum
    //
    __asm xor ecx, ecx              // ecc
    __asm mov ebx, 0xffff0000       // double index
    __asm mov eax, 1                    // bit mask

loop_4B_to_End:
    __asm mov esi, eax
    __asm and esi, edx
    __asm neg esi
    __asm sbb esi, esi
    __asm and esi, ebx
    __asm add ebx, 0xffff0001
    __asm xor ecx, esi
    __asm add eax, eax
    __asm jnz loop_4B_to_End

    //================================================
    // Compose final result
    // eax = ecc checksum
    //
    __asm pop eax
    __asm pop ebx
    __asm and ecx, 0x001f001f
    __asm or eax, ecx
    __asm or eax, ebx

    __asm pop edi               //
    __asm pop esi                   //
    __asm pop ebp               // restore callee saved regs
    __asm pop ebx               //
    __asm pop esp               //
    __asm ret 8
}


//  ================================================================
XECHECKSUM ChecksumNewFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb, const ULONG pgno )
//  ================================================================
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2_Wrapper;

    XECHECKSUM checksum = ChecksumSSE2_Emitted( pb, cb );

    //  The emitted code returns a 64-bit number with the ECC as the low DWORD and XOR as the high DWORD

    const ULONG checksumXOR = (ULONG)(checksum >> 32);
    const ULONG checksumECC = (ULONG)(checksum & 0xffffffff);

    return MakeChecksumFromECCXORAndPgno( checksumECC, checksumXOR, pgno );
}

#endif
