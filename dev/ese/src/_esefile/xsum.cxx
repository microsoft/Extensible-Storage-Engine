// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#include "esefile.hxx"

#include <limits.h>
#include <memory.h>
#include <stddef.h>
#include "xsum.hxx"

#include <intrin.h>

#ifdef _IA64_

extern "C" {
    void __lfetch( INT Level,void const *Address );
    #pragma intrinsic( __lfetch )
}

#define MD_LFHINT_NONE  0x00
#define MD_LFHINT_NT1   0x01
#define MD_LFHINT_NT2   0x02
#define MD_LFHINT_NTA   0x03

#endif



inline BOOL FGetBit( const void * const pv, const INT ibitOffset );


static UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype );
static BOOL     FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype );


static BOOL         FPageHasLongChecksum( const PAGETYPE pagetype );
static ULONG    ShortChecksumFromPage( const void * const pv );
static PAGECHECKSUM LongChecksumFromPage( const void * const pv );
static PAGECHECKSUM ChecksumFromPage( const void * const pv, const PAGETYPE pagetype );
static PAGECHECKSUM ComputePageChecksum(
    const void* const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    const BOOL fNew = fFalse );



extern INT g_cbPage;

static BOOL FIsSmallPage( ULONG cbPage )    { return cbPage <= 1024 * 8; }
static BOOL FIsSmallPage( void )                    { return FIsSmallPage( g_cbPage ); }


typedef XECHECKSUM (*PFNCHECKSUMNEWFORMAT)( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG   (*PFNCHECKSUMOLDFORMAT)( const unsigned char * const, const ULONG );


static XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno );

static ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb );

static PFNCHECKSUMOLDFORMAT pfnChecksumOldFormat = ChecksumSelectOldFormat;


static XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );



static XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
static XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
static XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
static XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue);



static PFNCHECKSUMNEWFORMAT pfnChecksumNewFormat = ChecksumSelectNewFormat;


static XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno )
{
    Assert( 0 != pgno );
    XECHECKSUM checksum = ( ( XECHECKSUM )pgno ) << 32 | xorChecksum;
    return checksum;
}


static ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumSelectOldFormat;

    if( FSSEInstructionsAvailable() )
    {
        if( FSSE2InstructionsAvailable() )
        {
            pfn = ChecksumOldFormatSSE2;
        }
        else
        {
            pfn = ChecksumOldFormatSSE;
        }
    }
    else if( sizeof( void * ) == sizeof( ULONG ) * 2 )
    {
        pfn = ChecksumOldFormat64Bit;
    }
    else
    {
        pfn = ChecksumOldFormat;
    }

    pfnChecksumOldFormat = pfn;

    return (*pfn)( pb, cb );
}


static ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat;

    Unused( pfn );

    const ULONG * pdw           = (ULONG *)pb;
    INT cbT                     = cb;


    ULONG   dwChecksum = 0x89abcdef ^ pdw[0];

    do
    {
        dwChecksum  ^= pdw[0]
                    ^ pdw[1]
                    ^ pdw[2]
                    ^ pdw[3]
                    ^ pdw[4]
                    ^ pdw[5]
                    ^ pdw[6]
                    ^ pdw[7];
        cbT -= 32;
        pdw += 8;
    } while ( cbT );

    return dwChecksum;
}


static ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat64Bit;

    Unused( pfn );

    const unsigned __int64* pqw         = (unsigned __int64 *)pb;
    unsigned __int64        qwChecksum  = 0;
    ULONG                   cbT         = cb;


    qwChecksum ^= pqw[0] & 0x00000000FFFFFFFF;

    do
    {
#ifdef _IA64_
        __lfetch( MD_LFHINT_NTA, (unsigned char *)(pqw + 4) );
#endif
        qwChecksum ^= pqw[0];
        qwChecksum ^= pqw[1];
        qwChecksum ^= pqw[2];
        qwChecksum ^= pqw[3];
        cbT -= ( 4 * sizeof( unsigned __int64 ) );
        pqw += 4;
    } while ( cbT );

    const unsigned __int64 qwUpper = ( qwChecksum >> ( sizeof( ULONG ) * 8 ) );
    const unsigned __int64 qwLower = qwChecksum & 0x00000000FFFFFFFF;
    qwChecksum = qwUpper ^ qwLower;

    const ULONG ulChecksum = static_cast<ULONG>( qwChecksum ) ^ 0x89abcdef;
    return ulChecksum;
}

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

static ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE;

    Unused( pfn );

    const ULONG * pdw   = (ULONG *)pb;
    INT cbT                     = cb;


    ULONG   dwChecksum = 0x89abcdef ^ pdw[0];

    do
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
        cbT -= 32;
        pdw += 8;
    } while ( cbT );

    return dwChecksum;
}

#if 1

static ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2;

    Unused( pfn );

#if (defined _AMD64_ || defined _X86_ )

    __m128i owChecksum              = _mm_setzero_si128();
    const   __m128i * pow           = (__m128i *)pb;
    const   __m128i * const powMax  = (__m128i *)(pb + cb);


    const ULONG * pdw       = (ULONG *)pb;
    ULONG   ulChecksum      = 0x89abcdef ^ pdw[0];

    do
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

        pow += 8;
    } while ( pow < powMax );

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


static ULONG ChecksumOldFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2_Wrapper;
    return ChecksumOldFormatSSE2_Emitted( pb, cb );
}

#endif

static XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumSelectNewFormat;

    if( FSSEInstructionsAvailable() )
    {
        if( FSSE2InstructionsAvailable() )
        {
            pfn = ChecksumNewFormatSSE2;
        }
        else
        {
            pfn = ChecksumNewFormatSSE;
        }
    }
    else if( sizeof( void * ) == sizeof( ULONG ) * 2 )
    {
        pfn = ChecksumNewFormat64Bit;
    }
    else
    {
        pfn = ChecksumNewFormat;
    }

    pfnChecksumNewFormat = pfn;

    return (*pfn)( pb, cb, pgno, fHeaderBlock );
}


inline XECHECKSUM MakeChecksumFromECCXORAndPgno(
    const ULONG eccChecksum,
    const ULONG xorChecksum,
    const ULONG pgno )
{
    const XECHECKSUM low    = xorChecksum ^ pgno;
    const XECHECKSUM high   = ( XECHECKSUM )eccChecksum << 32;
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
    INT cbT                     = cb;


    ULONG   dwChecksum = pdw[0] ^ pdw[1];

    do
    {
        dwChecksum  ^= pdw[0]
                    ^ pdw[1]
                    ^ pdw[2]
                    ^ pdw[3]
                    ^ pdw[4]
                    ^ pdw[5]
                    ^ pdw[6]
                    ^ pdw[7];
        cbT -= 32;
        pdw += 8;
    } while ( cbT );

    return dwChecksum;
}


static XECHECKSUM ChecksumNewFormatBasic( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
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

static XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormat;

    Unused( pfn );

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


static XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock  )
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
#if (defined _AMD64_ || defined _X86_ )

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


static XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2;

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
            const __m128i dqL1 = pdq[ i + 1 ];
            const __m128i dqL2 = pdq[ i + 2 ];
            const __m128i dqL3 = pdq[ i + 3 ];

            dq0 ^= dqL0;
            dq1 ^= dqL1;
            dq2 ^= dqL2;
            dq3 ^= dqL3;

            p ^= idxp & lParityMask( dqL0 ^ dqL1 ^ dqL2 ^dqL3 );

            idxp += 0xfe000200;

            const __m128i dqH0 = pdq[ i + 4 ];
            const __m128i dqH1 = pdq[ i + 5 ];
            const __m128i dqH2 = pdq[ i + 6 ];
            const __m128i dqH3 = pdq[ i + 7 ];

            dq0 ^= dqH0;
            dq1 ^= dqH1;
            dq2 ^= dqH2;
            dq3 ^= dqH3;

            p ^= idxp & lParityMask( dqH0 ^ dqH1 ^ dqH2 ^dqH3 );

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

        q ^= idxq & lParityMask( dwT );
        idxq += 0xffe00020;
    }

    ULONG r = 0;

    ULONG idxr = 0xffff0000;

    for ( ULONG i = 1; i; i += i )
    {
        const LONG bit = ( dw0 & i ) ? -1 : 0;
        r ^= bit & idxr;
        idxr += 0xffff0001;
    }

    const ULONG mask = ( cb << 19 ) - 1;

    const ULONG ecc = p & 0xfe00fe00 & mask | q & 0x01e001e0 | r & 0x001f001f;
    const ULONG xor = dw0;
    return MakeChecksumFromECCXORAndPgno( ecc, xor, pgno );
}

#else

XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    Assert( fFalse );
    return MakeChecksumFromECCXORAndPgno( 0, 0, pgno );
}


#endif
#endif

static XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
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

static __declspec( naked ) PAGECHECKSUM __stdcall ChecksumSSE2_Emitted( const unsigned char* pb, const ULONG cb )
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


static PAGECHECKSUM ChecksumNewFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb, const ULONG pgno )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2_Wrapper;

    Unused( pfn );

    PAGECHECKSUM checksum = ChecksumSSE2_Emitted( pb, cb );


    const ULONG checksumXOR = (ULONG)(checksum >> 32);
    const ULONG checksumECC = (ULONG)(checksum & 0xffffffff);

    return MakeChecksumFromECCXORAndPgno( checksumECC, checksumXOR, pgno );
}

#endif

inline BOOL FGetBit( const void * const pv, const INT ibitOffset )
{
    const unsigned char * const pb  = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    return ( pb[ibyte] & bitMask ) ? fTrue : fFalse;
}

static UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype )
{
    if( databasePage == pagetype )
    {
        return ( 9 * 32 ) + 13;
    }
    return ( UINT )-1;
}


static BOOL FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype )
{
    if( databasePage == pagetype )
    {
        const INT ibit = IbitNewChecksumFormatFlag( pagetype );
        return FGetBit( pv, ibit );
    }
    return 0;
}

static BOOL FPageHasLongChecksum( const PAGETYPE pagetype )
{
    return ( databasePage == pagetype );
}

static ULONG ShortChecksumFromPage( const void * const pv )
{
    return *( const ULONG* )pv;
}



static PAGECHECKSUM LongChecksumFromPage( const void * const pv )
{
    const PGHDR2* const pPH = ( const PGHDR2* )pv;
    if ( FIsSmallPage() )
    {
        return PAGECHECKSUM( pPH->checksum );
    }
    else
    {
        return PAGECHECKSUM( pPH->checksum, pPH->rgChecksum[ 0 ], pPH->rgChecksum[ 1 ], pPH->rgChecksum[ 2 ] );
    }
}


static PAGECHECKSUM ChecksumFromPage( const void * const pv, const PAGETYPE pagetype )
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        return LongChecksumFromPage( pv );
    }
    return ShortChecksumFromPage( pv );
}


static PAGECHECKSUM ComputePageChecksum(
    const void* const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    const BOOL fNew )
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        if( FPageHasNewChecksumFormat( pv, pagetype ) )
        {
            PAGECHECKSUM pgChecksum;
            const unsigned char* const pch = ( unsigned char* )pv;

            unsigned cbT = cb;
            if ( !FIsSmallPage( cb ) )
            {
                cbT = cb / cxeChecksumPerPage;
                pgChecksum.rgChecksum[ 1 ] = pfnChecksumNewFormat( pch + cbT * 1, cbT, pgno, fFalse );
                pgChecksum.rgChecksum[ 2 ] = pfnChecksumNewFormat( pch + cbT * 2, cbT, pgno, fFalse );
                pgChecksum.rgChecksum[ 3 ] = pfnChecksumNewFormat( pch + cbT * 3, cbT, pgno, fFalse );

                if ( fNew )
                {
                    PGHDR2* const pPgHdr2 = ( PGHDR2* )pv;
                    pPgHdr2->rgChecksum[ 0 ] = pgChecksum.rgChecksum[ 1 ];
                    pPgHdr2->rgChecksum[ 1 ] = pgChecksum.rgChecksum[ 2 ];
                    pPgHdr2->rgChecksum[ 2 ] = pgChecksum.rgChecksum[ 3 ];
                }
            }

            pgChecksum.rgChecksum[ 0 ] = pfnChecksumNewFormat( pch, cbT, pgno, fTrue );
            return pgChecksum;
        }
        else
        {
            return LongChecksumFromShortChecksum( (*pfnChecksumOldFormat)((unsigned char *)pv, cb), pgno );
        }
    }

    return (*pfnChecksumOldFormat)((unsigned char *)pv, cb);
}


static ULONG DwECCChecksumFromPagechecksum( const XECHECKSUM checksum )
{
    return (ULONG)( checksum >> 32 );
}


static ULONG DwXORChecksumFromPagechecksum( const XECHECKSUM checksum )
{
    return (ULONG)( checksum & 0xffffffff );
}


static INT CbitSet( const ULONG dw )
{
    INT cbit = 0;
    for( INT ibit = 0; ibit < 32; ++ibit )
    {
        if( dw & ( 1 << ibit ) )
        {
            ++cbit;
        }
    }
    return cbit;
}

static BOOL FECCErrorIsCorrectable( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
{
    const ULONG dwEcc = DwECCChecksumFromPagechecksum( xeChecksumActual ) ^ DwECCChecksumFromPagechecksum( xeChecksumExpected );
    const ULONG dwXor = DwXORChecksumFromPagechecksum( xeChecksumActual ) ^ DwXORChecksumFromPagechecksum( xeChecksumExpected );

    Assert( xeChecksumActual != xeChecksumExpected );

    const ULONG ulMask  = ( ( cb << 3 ) - 1 );
    const ULONG ulX     = ( ( dwEcc >> 16 ) ^ dwEcc ) & ulMask;


    if ( ulMask == ulX )
    {

        if( 1 == CbitSet( dwXor ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}

static UINT IbitCorrupted( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
{
    Assert( xeChecksumExpected != xeChecksumActual );
    Assert( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) );

    const ULONG dwEcc = DwECCChecksumFromPagechecksum( xeChecksumActual ) ^ DwECCChecksumFromPagechecksum( xeChecksumExpected );
    Assert( 0 != dwEcc );

    return ( UINT )( dwEcc & 0xffff );
}

inline void FlipBit( void * const pv, const INT ibitOffset )
{
    unsigned char * const   pb      = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    pb[ibyte] ^= bitMask;
}



enum XECHECKSUMERROR { xeChecksumNoError = 0, xeChecksumCorrectableError = -13, xeChecksumFatalError = -29, };

static XECHECKSUMERROR FTryFixBlock(
    const UINT cb,
    UINT* const pibitCorrupted,
    const XECHECKSUM xeChecksumExpected,
    const XECHECKSUM xeChecksumActual )
{
    if ( xeChecksumExpected == xeChecksumActual )
    {
        return xeChecksumNoError;
    }

    if ( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) )
    {
        const UINT ibitCorrupted = IbitCorrupted( cb, xeChecksumExpected, xeChecksumActual );

        if ( ( *pibitCorrupted != ibitCorrupted ) && ( ibitCorrupted < 8 * cb ) )
        {
            *pibitCorrupted = ibitCorrupted;
            return xeChecksumCorrectableError;
        }
    }

    return xeChecksumFatalError;
}

static void TryFixPage(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const BOOL fCorrectError,
    BOOL * const pfCorrectableError,
    INT * const pibitCorrupted,
    const PAGECHECKSUM checksumExpected,
    const PAGECHECKSUM checksumActual )
{
    Assert( checksumActual != checksumExpected );

    const BOOL fSmallPage = FIsSmallPage( cb );
    const UINT cbT = fSmallPage ? cb : cb / cxeChecksumPerPage;
    const UINT cblk = fSmallPage ? 1 : cxeChecksumPerPage;
    XECHECKSUMERROR rgErr[ cxeChecksumPerPage ] = { xeChecksumNoError, };

    UINT rgibitCorrupted[ cxeChecksumPerPage ] = { IbitNewChecksumFormatFlag( pagetype ), UINT_MAX, UINT_MAX, UINT_MAX, };
    UINT ibitCorrupted = UINT_MAX;

    for ( UINT iblk = 0; iblk < cblk; ++iblk )
    {
        rgErr[ iblk ] = FTryFixBlock( cbT, &rgibitCorrupted[ iblk ], checksumExpected.rgChecksum[ iblk ], checksumActual.rgChecksum[ iblk ] );
        switch ( rgErr[ iblk ] )
        {
            case xeChecksumNoError:
                break;

            case xeChecksumCorrectableError:
                if ( UINT_MAX == ibitCorrupted )
                {
                    ibitCorrupted = CHAR_BIT * cbT * iblk + rgibitCorrupted[ iblk ];

                    UINT ibitStart = CHAR_BIT * offsetof( PGHDR2, rgChecksum );
                    UINT ibitLength = CHAR_BIT * sizeof( checksumExpected.rgChecksum[ 0 ] );
                    if ( !fSmallPage &&
                        0 == iblk &&
                        ibitStart <= ibitCorrupted  &&
                        ibitCorrupted < ibitStart + ibitLength * ( cxeChecksumPerPage - 1 ) )
                    {
                        UINT ibit = ibitLength + ( ibitCorrupted - ibitStart );
                        FlipBit( ( void* )&checksumExpected, ibit );
                    }
                }
                break;

            case xeChecksumFatalError:
                *pfCorrectableError = fFalse;
                return;
        }
    }

    *pfCorrectableError = fTrue;
    *pibitCorrupted = ibitCorrupted;
    Assert( (UINT)*pibitCorrupted < ( 8 * cb ) );

    for ( UINT iblk = 0; fCorrectError && iblk < cblk; ++iblk )
    {
        unsigned char* pch = ( unsigned char* )pv;
        if ( xeChecksumCorrectableError == rgErr[ iblk ] )
        {
            FlipBit( &pch[ cbT * iblk ], rgibitCorrupted[ iblk ]);
        }
    }

    return;
}


void ChecksumAndPossiblyFixPage(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    const BOOL fCorrectError,
    PAGECHECKSUM * const pchecksumExpected,
    PAGECHECKSUM * const pchecksumActual,
    BOOL * const pfCorrectableError,
    INT * const pibitCorrupted )
{
    *pfCorrectableError = fFalse;
    *pibitCorrupted     = -1;

    *pchecksumExpected  = ChecksumFromPage( pv, pagetype );
    *pchecksumActual    = ComputePageChecksum( pv, cb, pagetype, pgno );

    const BOOL fNewChecksumFormat = FPageHasNewChecksumFormat( pv, pagetype );
    if( *pchecksumActual != *pchecksumExpected && fNewChecksumFormat )
    {
        TryFixPage( pv, cb, pagetype, fCorrectError, pfCorrectableError, pibitCorrupted, *pchecksumExpected, *pchecksumActual );
        Assert( ( *pfCorrectableError && *pibitCorrupted != -1 ) || ( !*pfCorrectableError && *pibitCorrupted == -1 ) );

        if ( fCorrectError && *pfCorrectableError )
        {
            *pchecksumExpected = ChecksumFromPage( pv, pagetype );
            *pchecksumActual = ComputePageChecksum( pv, cb, pagetype, pgno, fTrue );
        }
    }
}


