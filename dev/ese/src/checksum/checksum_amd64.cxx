// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "checksumstd.hxx"

#if 1

#include <intrin.h>
#if ( defined _AMD64_ || defined _X86_ )
#include <emmintrin.h>
#endif

#else


#define ChecksumOldFormatSSE2_Wrapper   ChecksumOldFormatSSE2
#define ChecksumNewFormatSSE2_Wrapper   ChecksumNewFormatSSE2

#endif



typedef unsigned __int64 XECHECKSUM;

typedef XECHECKSUM  (*PFNCHECKSUMNEWFORMAT)( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG   (*PFNCHECKSUMOLDFORMAT)( const unsigned char * const, const ULONG );

inline void CachePrefetch( const void * const p )
{
#ifdef _X86_
    _asm
    {
        mov eax,p

        _emit 0x0f
        _emit 0x18
        _emit 0x00
    }
#endif
}

ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE;

    Unused( pfn );

    const ULONG * pdw   = (ULONG *)pb;
    const INT cDWords           = 8;
    const INT cbStep            = cDWords * sizeof( ULONG );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );


    ULONG   dwChecksum = 0x89abcdef ^ pdw[0];

    while ( ( cbT -= cbStep ) >= 0 )
    {
#if (defined _AMD64_ || defined _X86_ )
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

ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2;

    Unused( pfn );

#if (defined _AMD64_ || defined _X86_ )

    __m128i owChecksum              = _mm_setzero_si128();
    const   __m128i * pow           = (__m128i *)pb;
    const INT cm128is               = 8;
    const INT cbStep                = cm128is * sizeof( __m128i );
    __int64 cbT                     = cb;
    Assert( 0 == ( cbT % cbStep ) );


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


    return 0x12345678;

#endif
}

#endif

#if 1
#else

static __declspec( naked ) ULONG __stdcall ChecksumOldFormatSSE2_Emitted( const unsigned char * const pb, const ULONG cb )

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

    emit4( 0x66, 0x0f, 0x6e, 0x00 );
    emit4( 0x66, 0x0f, 0xef, 0xc9 );
    emit4( 0x66, 0x0f, 0xef, 0xd2 );
    emit4( 0x66, 0x0f, 0xef, 0xdb );
    emit4( 0x66, 0x0f, 0xef, 0xe4 );
    emit4( 0x66, 0x0f, 0xef, 0xed );
    emit4( 0x66, 0x0f, 0xef, 0xf6 );
    emit4( 0x66, 0x0f, 0xef, 0xff );

    __asm align 16
xorloop:
    emit4( 0x0f, 0x18, 0x04, 0x90 );

    emit4( 0x66, 0x0f, 0xef, 0x00 );
    emit5( 0x66, 0x0f, 0xef, 0x48, 0x10 );
    emit5( 0x66, 0x0f, 0xef, 0x50, 0x20 );
    emit5( 0x66, 0x0f, 0xef, 0x58, 0x30 );
    emit5( 0x66, 0x0f, 0xef, 0x60, 0x40 );
    emit5( 0x66, 0x0f, 0xef, 0x68, 0x50 );
    emit5( 0x66, 0x0f, 0xef, 0x70, 0x60 );
    emit5( 0x66, 0x0f, 0xef, 0x78, 0x70 );

    __asm add eax, edx
    __asm sub ecx, edx
    __asm ja xorloop

    emit4( 0x66, 0x0f, 0xef, 0xc1 );
    emit4( 0x66, 0x0f, 0xef, 0xd3 );
    emit4( 0x66, 0x0f, 0xef, 0xe5 );
    emit4( 0x66, 0x0f, 0xef, 0xf7 );
    emit4( 0x66, 0x0f, 0xef, 0xc2 );
    emit4( 0x66, 0x0f, 0xef, 0xe6 );
    emit4( 0x66, 0x0f, 0xef, 0xc4 );

    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x4e );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );
    emit5( 0x66, 0x0f, 0x70, 0xd0, 0xb1 );
    emit4( 0x66, 0x0f, 0xef, 0xc2 );
    emit4( 0x66, 0x0f, 0x7e, 0xc0 );

    __asm xor eax, 0x89abcdef
    __asm ret 0x8
}


ULONG ChecksumOldFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2_Wrapper;
    return ChecksumOldFormatSSE2_Emitted( pb, cb );
}

#endif

inline XECHECKSUM MakeChecksumFromECCXORAndPgno(
    const ULONG eccChecksum,
    const ULONG xorChecksum,
    const ULONG pgno )
{
    const XECHECKSUM low    = xorChecksum ^ pgno;
    const XECHECKSUM high   = (XECHECKSUM)eccChecksum << 32;
    return ( high | low );
}


__declspec( align( 128 ) ) static const signed char g_bParityLookupTable[ 256 ] =
{
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    -1, 0, 0, -1, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0, -1,
    0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0,
};


inline LONG lParityMask( const ULONG dw )

{
    const ULONG dw1 = dw >> 16;
    const ULONG dw2 = dw ^ dw1;

    const ULONG dw3 = dw2 >> 8;
    const ULONG dw4 = dw2 ^ dw3;

    return g_bParityLookupTable[ dw4 & 0xff ];
}

static ULONG DwXORChecksumBasic( const unsigned char * const pb, const ULONG cb )
{
    const ULONG * pdw   = (ULONG *)pb;
    const INT cDWords           = 8;
    const INT cbStep            = cDWords * sizeof( ULONG );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );


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


XECHECKSUM ChecksumNewFormatBasic( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatBasic;

    Unused( pfn );

    Assert( 4 == sizeof( ULONG ) );

    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    Assert( 0 == ( ( uintptr_t )pb & ( 32 - 1 ) ) );

    const ULONG* pdw = ( ULONG* )pb;
    const ULONG cdw = cb / sizeof( *pdw );

    ULONG p = 0;

    unsigned p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    {
        ULONG idxp = 0xff800000;

        ULONG i = 0;

        ULONG pT0 = 0;
        ULONG pT1 = 0;

        if ( fHeaderBlock )
        {
            goto Start;
        }

        do
        {
            pT0 = pdw[ i + 0 ];
            pT1 = pdw[ i + 1 ];
Start:
            const ULONG pT2 = pdw[ i + 2 ];
            const ULONG pT3 = pdw[ i + 3 ];

            p0 ^= pT0;
            p1 ^= pT1;
            p2 ^= pT2;
            p3 ^= pT3;

            p ^= idxp & lParityMask( pT0 ^ pT1 ^ pT2 ^ pT3 );

            idxp += 0xff800080;

            const ULONG pT4 = pdw[ i + 4 ];
            const ULONG pT5 = pdw[ i + 5 ];
            const ULONG pT6 = pdw[ i + 6 ];
            const ULONG pT7 = pdw[ i + 7 ];

            p0 ^= pT4;
            p1 ^= pT5;
            p2 ^= pT6;
            p3 ^= pT7;

            p ^= idxp & lParityMask( pT4 ^ pT5 ^ pT6 ^ pT7 );

            idxp += 0xff800080;

            i += 8;

            __assume( 8 < cdw );
        }while ( i < cdw );
    }


    p |= 0x00400000 & lParityMask( p0 ^ p1 );
    p |= 0x00000040 & lParityMask( p2 ^ p3 );

    const ULONG r0 = p0 ^ p2;
    const ULONG r1 = p1 ^ p3;

    p |= 0x00200000 & lParityMask( r0 );
    p |= 0x00000020 & lParityMask( r1 );

    const ULONG r2 = r0 ^ r1;

    ULONG r = 0;

    ULONG idxr = 0xffff0000;

    for ( ULONG i = 1; i; i += i )
    {
        const LONG mask = -!!( r2 & i );

        r ^= mask & idxr;
        idxr += 0xffff0001;
    }

    const ULONG mask = ( cb << 19 ) - 1;

    const ULONG eccChecksum = p & 0xffe0ffe0 & mask | r & 0x001f001f;
    const ULONG xorChecksum = DwXORChecksumBasic( pb, cb );
    return MakeChecksumFromECCXORAndPgno( eccChecksum, xorChecksum, pgno );
}

XECHECKSUM ChecksumNewFormatSlowly( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSlowly;

    Unused( pfn );


    AssertRTL( 0 == lParityMask( 0 ) );
    AssertRTL( 0xFFFFFFFF == lParityMask( 1 ) );

    Assert( 4 == sizeof( ULONG ) );

    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    Assert( 0 == ( ( uintptr_t )pb & ( 32 - 1 ) ) );

    const ULONG* pdw = ( ULONG* )pb;
    const ULONG cdw = cb / sizeof( *pdw );

    ULONG p = 0;

    ULONG p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    {
        ULONG idxp = 0xff800000;

        ULONG i = 0;

        ULONG pT0 = 0;
        ULONG pT1 = 0;

        if ( fHeaderBlock )
        {
            goto Start;
        }

        do
        {
            pT0 = pdw[ i + 0 ];
            pT1 = pdw[ i + 1 ];
Start:
            const ULONG pT2 = pdw[ i + 2 ];
            const ULONG pT3 = pdw[ i + 3 ];

            p0 ^= pT0;
            p1 ^= pT1;
            p2 ^= pT2;
            p3 ^= pT3;

            p ^= idxp & lParityMask( pT0 ^ pT1 ^ pT2 ^ pT3 );

            idxp += 0xff800080;

            const ULONG pT4 = pdw[ i + 4 ];
            const ULONG pT5 = pdw[ i + 5 ];
            const ULONG pT6 = pdw[ i + 6 ];
            const ULONG pT7 = pdw[ i + 7 ];

            p0 ^= pT4;
            p1 ^= pT5;
            p2 ^= pT6;
            p3 ^= pT7;

            p ^= idxp & lParityMask( pT4 ^ pT5 ^ pT6 ^ pT7 );

            idxp += 0xff800080;

            i += 8;

            __assume( 8 < cdw );
        }       while ( i < cdw );
    }


    p |= 0x00400000 & lParityMask( p0 ^ p1 );
    p |= 0x00000040 & lParityMask( p2 ^ p3 );

    const ULONG r0 = p0 ^ p2;
    const ULONG r1 = p1 ^ p3;

    p |= 0x00200000 & lParityMask( r0 );
    p |= 0x00000020 & lParityMask( r1 );

    const ULONG r2 = r0 ^ r1;

    ULONG r = 0;

    ULONG idxr = 0xffff0000;

    for ( ULONG i = 1; i; i += i )
    {
        const LONG mask = -!!( r2 & i );

        r ^= mask & idxr;
        idxr += 0xffff0001;
    }

    const ULONG mask = ( cb << 19 ) - 1;

    const ULONG ecc = p & 0xffe0ffe0 & mask | r & 0x001f001f;
    const ULONG xor = r2;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}


XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock  )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE;

    Unused( pfn );

    Assert( 4 == sizeof( ULONG ) );

    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    Assert( 0 == ( ( uintptr_t )pb & ( 32 - 1 ) ) );

    const ULONG* pdw = ( ULONG* )pb;
    const ULONG cdw = cb / sizeof( *pdw );

    ULONG p = 0;

    unsigned p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    {
        ULONG idxp = 0xff800000;

        ULONG i = 0;

        ULONG pT0 = 0;
        ULONG pT1 = 0;

        if ( fHeaderBlock )
        {
            goto Start;
        }

        do
        {
            pT0 = pdw[ i + 0 ];
            pT1 = pdw[ i + 1 ];
Start:

#if (defined _AMD64_ || defined _X86_ )
#if 1
            _mm_prefetch( ( char *)&( pdw[ i + 32 ] ), _MM_HINT_NTA );
#else
            CachePrefetch( ( char *)&( pdw[ i + 32 ] ) );
#endif
#endif

            const ULONG pT2 = pdw[ i + 2 ];
            const ULONG pT3 = pdw[ i + 3 ];

            p0 ^= pT0;
            p1 ^= pT1;
            p2 ^= pT2;
            p3 ^= pT3;

            p ^= idxp & lParityMask( pT0 ^ pT1 ^ pT2 ^ pT3 );

            idxp += 0xff800080;

            const ULONG pT4 = pdw[ i + 4 ];
            const ULONG pT5 = pdw[ i + 5 ];
            const ULONG pT6 = pdw[ i + 6 ];
            const ULONG pT7 = pdw[ i + 7 ];

            p0 ^= pT4;
            p1 ^= pT5;
            p2 ^= pT6;
            p3 ^= pT7;

            p ^= idxp & lParityMask( pT4 ^ pT5 ^ pT6 ^ pT7 );

            idxp += 0xff800080;

            i += 8;

            __assume( 8 < cdw );
        }
        while ( i < cdw );
    }


    p |= 0x00400000 & lParityMask( p0 ^ p1 );
    p |= 0x00000040 & lParityMask( p2 ^ p3 );

    const ULONG r0 = p0 ^ p2;
    const ULONG r1 = p1 ^ p3;

    p |= 0x00200000 & lParityMask( r0 );
    p |= 0x00000020 & lParityMask( r1 );

    const ULONG r2 = r0 ^ r1;

    ULONG r = 0;

    ULONG idxr = 0xffff0000;

    for ( ULONG i = 1; i; i += i )
    {
        const LONG mask = -!!( r2 & i );

        r ^= mask & idxr;
        idxr += 0xffff0001;
    }

    const ULONG mask = ( cb << 19 ) - 1;

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

#if ( defined _AMD64_ || defined _X86_ ) && !defined _CHPE_X86_ARM64_

inline __m128i operator^( const __m128i dq0, const __m128i dq1 )
{
    return _mm_xor_si128( dq0, dq1 );
}

inline __m128i operator^=( __m128i& dq0, const __m128i dq1 )
{
    return dq0 = _mm_xor_si128( dq0, dq1 );
}

inline LONG lParityMask( const __m128i dq )
{
    const __m128i dq1 = _mm_shuffle_epi32( dq, 0x4e );
    const __m128i dq2 = dq ^ dq1;

    const __m128i dq3 = _mm_shuffle_epi32( dq2, 0x1b );
    const __m128i dq4 = dq2 ^ dq3;

    return lParityMask( _mm_cvtsi128_si32( dq4 ) );
}

inline LONG lParityMaskPopcnt( const __m128i dq )
{
    const __m128i dq1 = _mm_shuffle_epi32( dq, 0x4e);
    const __m128i dq2 = dq ^ dq1;

#if ( defined _X86_ )
    const __m128i dq3 = _mm_shuffle_epi32( dq2, 0x1b );
    const __m128i dq4 = dq2 ^ dq3;
    INT popcnt = _mm_popcnt_u32( _mm_cvtsi128_si32( dq4 ) );
#else
    INT popcnt = (INT) _mm_popcnt_u64( _mm_cvtsi128_si64( dq2 ) );
#endif

    return -(popcnt & 0x01);
}

inline LONG lParityMaskPopcnt( const LONG dw )
{
    INT popcnt = (INT) _mm_popcnt_u32( dw );
    return -(popcnt & 0x01);
}

extern __declspec( align( 128 ) ) const unsigned char g_bECCLookupTable[ 256 ];

inline ULONG lECCLookup8bit(const ULONG byte)
{
    return g_bECCLookupTable[ byte & 0xff ];
}

template <ChecksumParityMaskFunc TParityMaskFunc>
XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2<TParityMaskFunc>;

    Unused( pfn );

    Assert( 16 == sizeof( __m128i ) );
    Assert( 4 == sizeof( ULONG ) );

    Assert( 0 == ( cb & ( cb -1 ) ) );
    Assert( 1024 <= cb && cb <= 8192 );

    Assert( 0 == ( ( uintptr_t )pb & ( 128 - 1 ) ) );

    const ULONG cdq = cb / 16;

    ULONG p = 0;

    __m128i dq0 = _mm_setzero_si128();
    __m128i dq1 = _mm_setzero_si128();
    __m128i dq2 = _mm_setzero_si128();
    __m128i dq3 = _mm_setzero_si128();
    {
        ULONG idxp = 0xfe000000;

        ULONG i = 0;
        __m128i dqL0;


        const __m128i* pdq = ( const __m128i* )pb;
        if ( fHeaderBlock )
        {
            dqL0 = _mm_unpackhi_epi64( _mm_setzero_si128(), pdq[ 0 ] );
            goto Start;
        }

        do
        {
            dqL0 = pdq[ i + 0 ];
Start:
            _mm_prefetch( ( char *)&pdq[ i + 32 ], _MM_HINT_NTA );
            _mm_prefetch( ( char *)&pdq[ i + 32 + 4], _MM_HINT_NTA );

            const __m128i dqL1 = pdq[ i + 1 ];
            const __m128i dqL2 = pdq[ i + 2 ];
            const __m128i dqL3 = pdq[ i + 3 ];
            LONG parityL, parityH;

            dq0 ^= dqL0;
            dq1 ^= dqL1;
            dq2 ^= dqL2;
            dq3 ^= dqL3;
            const __m128i dqLAcc = dqL0 ^ dqL1 ^ dqL2 ^ dqL3;

            if ( TParityMaskFunc == ParityMaskFuncPopcnt )
            {
                parityL = lParityMaskPopcnt( dqLAcc );
            }
            else
            {
                parityL = lParityMask( dqLAcc );
            }
            p ^= idxp & parityL;

            idxp += 0xfe000200;

            const __m128i dqH0 = pdq[ i + 4 ];
            const __m128i dqH1 = pdq[ i + 5 ];
            const __m128i dqH2 = pdq[ i + 6 ];
            const __m128i dqH3 = pdq[ i + 7 ];

            dq0 ^= dqH0;
            dq1 ^= dqH1;
            dq2 ^= dqH2;
            dq3 ^= dqH3;
            const __m128i dqHAcc = dqH0 ^ dqH1 ^ dqH2 ^ dqH3;

            if ( TParityMaskFunc == ParityMaskFuncPopcnt )
            {
                parityH = lParityMaskPopcnt( dqHAcc );
            }
            else
            {
                parityH = lParityMask( dqHAcc );
            }
            p ^= idxp & parityH;
            idxp += 0xfe000200;

            i += 8;

            __assume( 8 < cdq );
        }
        while ( i < cdq );
    }

    __declspec( align( 128 ) ) __m128i arydq[ 4 ];
    ULONG* pdw = ( ULONG* )arydq;

    arydq[ 0 ] = dq0;
    arydq[ 1 ] = dq1;
    arydq[ 2 ] = dq2;
    arydq[ 3 ] = dq3;

    ULONG q = 0;

    ULONG dw0 = 0;

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

    ULONG r = 0;
    ULONG byteT = dw0;
    ULONG byte0 = 0;
    ULONG idxr = 0xfff80000;
    for ( ULONG i = 0; i < 4; i++ )
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

    const LONG bits = lECCLookup8bit( byte0 );
    r |= ( bits & 0x07 );
    r |= ( ( bits << 12 ) & 0x00070000 );

    const ULONG mask = ( cb << 19 ) - 1;

    const ULONG ecc = p & 0xfe00fe00 & mask | q & 0x01e001e0 | r & 0x001f001f;
    const ULONG xor = dw0;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}

template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncDefault>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );
template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );

XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );

#else

template <ChecksumParityMaskFunc TParityMaskFunc>
XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    Enforce( fFalse );
    return MakeChecksumFromECCXORAndPgno( 0, 0, pgno );
}

template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncDefault>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );
template XECHECKSUM ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock );

#endif
#endif

XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormat64Bit;

    Unused( pfn );

    const ULONG*    rgdwRow = ( ULONG* )pb;
    const ULONG cColumn     = 32;
    const ULONG cRow        = cb / ( cColumn / 8 );

    ULONG dwColumnParity = 0;

    ULONG p = 0, q = 0;
    ULONG iRow = 0;

    for ( iRow = fHeaderBlock ? 2 : 0; iRow < cRow; ++iRow )
    {
        const ULONG dwRow = rgdwRow[ iRow ];

        dwColumnParity ^= dwRow;

        const LONG maskRow = lParityMask( dwRow );

        p ^= maskRow & iRow;
        q ^= maskRow & ~iRow;
    }

    ULONG r = 0, s = 0;
    ULONG iColumn = 0;
    for ( iColumn = 0; iColumn < cColumn; ++iColumn )
    {
        const LONG maskColumn = -( !!( dwColumnParity & ( 1 << iColumn ) ) );

        r ^= maskColumn & iColumn;
        s ^= maskColumn & ~iColumn;
    }

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

static __declspec( naked ) XECHECKSUM __stdcall ChecksumSSE2_Emitted( const unsigned char* pb, const ULONG cb )
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

    __asm mov eax, [ esp + 4 ]
    __asm mov ecx, [ esp + 8 ]

    __asm mov edx, esp
    __asm and esp, 0xfffffff0
    __asm sub esp, 16 * 4

    emit4( 0x66, 0x0f, 0xef, 0xc0 );
    emit4( 0x66, 0x0f, 0x6d, 0x00 );

    __asm push edx
    __asm push ebx
    __asm push ebp
    __asm push esi
    __asm push edi

    __asm push ecx
    __asm shr ecx, 7
    __asm xor ebx, ebx
    __asm mov edx, 0xfe000000
    __asm mov ebp, 0xfe000200

    emit4( 0x66, 0x0f, 0xef, 0xe4 );
    emit4( 0x66, 0x0f, 0xef, 0xed );
    emit4( 0x66, 0x0f, 0xef, 0xf6 );
    emit4( 0x66, 0x0f, 0xef, 0xff );
    __asm jmp loop_page_to_64B_start

loop_page_to_64B:
    emit4( 0x66, 0x0f, 0x6f, 0x00 );
loop_page_to_64B_start:
    emit5( 0x66, 0x0f, 0x6f, 0x48, 0x10 );
    emit5( 0x66, 0x0f, 0x6f, 0x50, 0x20 );
    emit5( 0x66, 0x0f, 0x6f, 0x58, 0x30 );

    emit7( 0x0f, 0x18, 0x80, 0x00, 0x02, 0x00, 0x00 );

    emit4( 0x66, 0x0f, 0xef, 0xe0 );
    emit4( 0x66, 0x0f, 0xef, 0xe9 );
    emit4( 0x66, 0x0f, 0xef, 0xf2 );
    emit4( 0x66, 0x0f, 0xef, 0xfb );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );
    emit4( 0x66, 0x0f, 0xef, 0xd3 );
    emit4( 0x66, 0x0f, 0xef, 0xc2 );

    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x4e );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );
    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x1b );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );

    emit4( 0x66, 0x0f, 0x7e, 0xc6 );
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

    emit5( 0x66, 0x0f, 0x6f, 0x40, 0x40 );
    emit5( 0x66, 0x0f, 0x6f, 0x48, 0x50 );
    emit5( 0x66, 0x0f, 0x6f, 0x50, 0x60 );
    emit5( 0x66, 0x0f, 0x6f, 0x58, 0x70 );

    emit4( 0x66, 0x0f, 0xef, 0xe0 );
    emit4( 0x66, 0x0f, 0xef, 0xe9 );
    emit4( 0x66, 0x0f, 0xef, 0xf2 );
    emit4( 0x66, 0x0f, 0xef, 0xfb );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );
    emit4( 0x66, 0x0f, 0xef, 0xd3 );
    emit4( 0x66, 0x0f, 0xef, 0xc2 );

    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x4e );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );
    emit5( 0x66, 0x0f, 0x70, 0xc8, 0x1b );
    emit4( 0x66, 0x0f, 0xef, 0xc1 );

    emit4( 0x66, 0x0f, 0x7e, 0xc6 );
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

    emit6( 0x66, 0x0f, 0x7f, 0x64, 0x24, STKOFF + 0x00 );
    emit6( 0x66, 0x0f, 0x7f, 0x6c, 0x24, STKOFF + 0x10 );
    emit6( 0x66, 0x0f, 0x7f, 0x74, 0x24, STKOFF + 0x20 );
    emit6( 0x66, 0x0f, 0x7f, 0x7c, 0x24, STKOFF + 0x30 );

    __asm pop ecx
    __asm and ebx, 0xfe00fe00
    __asm shl ecx, 19
    __asm dec ecx
    __asm and ebx, ecx
    __asm push ebx

    __asm lea eax, [ esp + STKOFF ]
    __asm mov ecx, 16
    __asm xor edx, edx
    __asm xor ebx, ebx
    __asm mov ebp, 0xffe00000

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
    __asm push ebx

    __asm xor ecx, ecx
    __asm mov ebx, 0xffff0000
    __asm mov eax, 1

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

    __asm pop eax
    __asm pop ebx
    __asm and ecx, 0x001f001f
    __asm or eax, ecx
    __asm or eax, ebx

    __asm pop edi
    __asm pop esi
    __asm pop ebp
    __asm pop ebx
    __asm pop esp
    __asm ret 8
}


XECHECKSUM ChecksumNewFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb, const ULONG pgno )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2_Wrapper;

    XECHECKSUM checksum = ChecksumSSE2_Emitted( pb, cb );


    const ULONG checksumXOR = (ULONG)(checksum >> 32);
    const ULONG checksumECC = (ULONG)(checksum & 0xffffffff);

    return MakeChecksumFromECCXORAndPgno( checksumECC, checksumXOR, pgno );
}

#endif
