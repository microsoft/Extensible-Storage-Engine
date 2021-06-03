// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/*******************************************************************

Each database page and database header page contains a 8-byte checksum.
The checksum is the first 8 bytes on the page. The checksum incorporates
the page number so that getting back the wrong page will generate a
checksum error.

There are two different formats of page checksums, called (imaginatively
enough) old and new:

- Old:  this is the format used by 5.5 through Exchange 2003 SP1, and
        Windows 2000 through Longhorn. The first DWORD of the checksum
        is an XOR of all DWORDs on the page (except the checksum) along
        with the seed value of 0x89abcdef. The second DWORD is the pgno
        of the page. XOR is used because it's very easy to optimize on
        processors with wide words lengths.

        The old checksum format is actually a 4-byte format, we expand
        it to 8-bytes for pages by making the pgno check part of checksum
        validation. Database and logfile headers don't have a pgno, so
        they in fact have a 4-byte checksum.

- New:  XOR checksums can only detect problems, not correct them. ECC
        checksums can correct single-bit errors in pages, which are actually
        a significant percentage of problems we see. Calculating and verifying
        ECC checksums is more expensive though.

        The new checksum format uses an ECC checksum to allow correction of
        single-bit errors in pages.

In an I/O intensive scenario checksum calculation and verification can become
a performance problem. To optimize checksumming as much as possible we take
advantage of different processor features (64-bit words, SSE instructions, SSE2
instructions). This leads to several implementations of the checksum routine,
for different architectures. All checksum calls are indirected through a function
pointer, which is set at runtime, depending on the capabilities of the processor
we are running on.

In order to determine which type of checksum should be used, there is a
bit in the database header/database page which tells us what type of
checksum to use. We always have to consider that a corruption could flip
that bit, so the actual flow of checksumming a page is a little complex to
deal with that.

*******************************************************************/

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
} // extern "C"

#define MD_LFHINT_NONE  0x00
#define MD_LFHINT_NT1   0x01
#define MD_LFHINT_NT2   0x02
#define MD_LFHINT_NTA   0x03

#endif  //  _IA64_

//  ****************************************************************
//  page checksum routines
//  ****************************************************************

//  manipulate bits on the page

inline BOOL FGetBit( const void * const pv, const INT ibitOffset );

//  get and set the new checksum format flag

static UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype );
static BOOL     FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype );

//  get and set the checksum

static BOOL         FPageHasLongChecksum( const PAGETYPE pagetype );
static ULONG    ShortChecksumFromPage( const void * const pv );
static PAGECHECKSUM LongChecksumFromPage( const void * const pv );
static PAGECHECKSUM ChecksumFromPage( const void * const pv, const PAGETYPE pagetype );
static PAGECHECKSUM ComputePageChecksum(
    const void* const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    // set fNew to compute new ECC for a page (R/W wrt the large page!!)
    // reset fNew to computer ECC for verification purpose (R/O wrt the page)
    const BOOL fNew = fFalse );


//

extern INT g_cbPage;

static BOOL FIsSmallPage( ULONG cbPage )    { return cbPage <= 1024 * 8; }
static BOOL FIsSmallPage( void )                    { return FIsSmallPage( g_cbPage ); }


typedef XECHECKSUM (*PFNCHECKSUMNEWFORMAT)( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG   (*PFNCHECKSUMOLDFORMAT)( const unsigned char * const, const ULONG );

//  ****************************************************************
//  XOR checksum routines
//  ****************************************************************

static XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno );

static ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb );
static ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb );

static PFNCHECKSUMOLDFORMAT pfnChecksumOldFormat = ChecksumSelectOldFormat;

//  ****************************************************************
//  ECC checksum routines
//  ****************************************************************

static XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );


//  see comments in checksum_amd64.cxx to see why these are in a separate file

static XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
static XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
static XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
static XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue);



static PFNCHECKSUMNEWFORMAT pfnChecksumNewFormat = ChecksumSelectNewFormat;


//  ================================================================
static XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno )
//  ================================================================
{
    Assert( 0 != pgno );
    XECHECKSUM checksum = ( ( XECHECKSUM )pgno ) << 32 | xorChecksum;
    return checksum;
}


//  ================================================================
static ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Decide which checksum mechanism to use and set the function pointer
//  to it. Subsequent calls to the checksum code will call the correct
//  function directly
//
//  We check for SSE2 instructions before checking to see if this is
//  a 64-bit machine, so AMD64 machines will end up using ChecksumOldFormatSSE2.
//  That is good as the SSE2 implementation will be faster than the plain
//  64-bit implementation.
//
//  Here is a mapping of processor type to function:
//
//  P2/P3       : ChecksumOldFormatSSE
//  P4          : ChecksumOldFormatSSE2
//  AMD64       : ChecksumOldFormatSSE2
//  IA64        : ChecksumOldFormat64Bit
//  Cyrix etc.  : ChecksumOldFormat
//
//-
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


//  ================================================================
static ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Plain old unrolled-loop checksum that should work on any processor
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat;

    Unused( pfn );

    const ULONG * pdw           = (ULONG *)pb;
    INT cbT                     = cb;

    //  remove the first unsigned long, as it is the checksum itself

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


//  ================================================================
static ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Do checksumming 64 bits at a time (the native word size)
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat64Bit;

    Unused( pfn );

    const unsigned __int64* pqw         = (unsigned __int64 *)pb;
    unsigned __int64        qwChecksum  = 0;
    ULONG                   cbT         = cb;

    //  checksum the first four bytes twice to remove the checksum

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

//  ================================================================
inline void CachePrefetch( const void * const p )
//  ================================================================
{
#ifdef _X86_
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
static ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb )
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
    INT cbT                     = cb;

    //  touching this memory puts the page in the processor TLB (needed
    //  for prefetch) and brings in the first cacheline (cacheline 0)

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

//  ================================================================
static ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  SSE2 supports 128-bit XOR operations. XOR the page using the SSE2
//  instructions and collapse the checksum at the end.
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSSE2;

    Unused( pfn );

#if (defined _AMD64_ || defined _X86_ )

    __m128i owChecksum              = _mm_setzero_si128();
    const   __m128i * pow           = (__m128i *)pb;
    const   __m128i * const powMax  = (__m128i *)(pb + cb);

    //  checksum the first eight bytes twice to remove the checksum

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

    //  we should never get here

    return 0x12345678;

#endif
}

#endif  //  1

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
static ULONG ChecksumOldFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb )
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

#endif  //  1

//  ================================================================
static XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  Decide which checksum mechanism to use and set the function pointer
//  to it. Subsequent calls to the checksum code will call the correct
//  function directly
//
//  We check for SSE2 instructions before checking to see if this is
//  a 64-bit machine, so AMD64 machines will end up using ChecksumNewFormatSSE2.
//  That is good as the SSE2 implementation will be faster than the plain
//  64-bit implementation.
//
//  Here is a mapping of processor type to function:
//
//  P2/P3       : ChecksumNewFormatSSE
//  P4          : ChecksumNewFormatSSE2
//  AMD64       : ChecksumNewFormatSSE2
//  IA64        : ChecksumNewFormat64Bit
//  Cyrix etc.  : ChecksumNewFormat
//
//-
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


//  ================================================================
inline XECHECKSUM MakeChecksumFromECCXORAndPgno(
    const ULONG eccChecksum,
    const ULONG xorChecksum,
    const ULONG pgno )
//  ================================================================
{
    const XECHECKSUM low    = xorChecksum ^ pgno;
    const XECHECKSUM high   = ( XECHECKSUM )eccChecksum << 32;
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
    INT cbT                     = cb;

    //  remove the first QWORD, as it is the checksum itself

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


//  ================================================================
static XECHECKSUM ChecksumNewFormatBasic( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
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
static XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  optmized ECC function for 32-bit CPU, 32-byte cache line, no prefetching
//  the data is organized into an array of bits that is 32*4=128 bits wide
//  each line of CPU cache will hold 2 rows of data
//
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormat;

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
static XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock  )
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

#if (defined _AMD64_ || defined _X86_ )
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
#if (defined _AMD64_ || defined _X86_ )

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
static XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  optmized ECC function for P4 (SSE2 + 128B L2 + Prefetching)
//  the data is organized into an array of bits that is 128*4=512 bits wide
//  each line of CPU cache will hold 2 rows of data
//
//-
{
    // ensure correct signature of the function
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2;

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
    // compute ECC bits shown as '1'
    // 1111 1110 0000 0000 1111 1110 0000 0000
    //
    ULONG p = 0;

    // dq0, dq1, dq2, dq3 hold accumulated column parity bits (128*4=512 bits)
    //
    __m128i dq0 = _mm_setzero_si128();
    __m128i dq1 = _mm_setzero_si128();
    __m128i dq2 = _mm_setzero_si128();
    __m128i dq3 = _mm_setzero_si128();
    {
        // combine 16-bit index and ~index together into a 32-bit double index
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
            _mm_prefetch( ( char *)&pdq[ i + 32 ], _MM_HINT_NTA );
            const __m128i dqL1 = pdq[ i + 1 ];
            const __m128i dqL2 = pdq[ i + 2 ];
            const __m128i dqL3 = pdq[ i + 3 ];

            // update accumulated column parity bits
            //
            dq0 ^= dqL0;
            dq1 ^= dqL1;
            dq2 ^= dqL2;
            dq3 ^= dqL3;

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( dqL0 ^ dqL1 ^ dqL2 ^dqL3 );

            // update double index
            //
            idxp += 0xfe000200;

            // read another row (second half of the cache line) from data (128*4=512 bits)
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

            // compute row parity and distribute it according to its index
            //
            p ^= idxp & lParityMask( dqH0 ^ dqH1 ^ dqH2 ^dqH3 );

            // update double index
            //
            idxp += 0xfe000200;

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
    // reduce 512 column parity bits (dq0, dq1, dq2, dq3) into 32 bits
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

        q ^= idxq & lParityMask( dwT );
        idxq += 0xffe00020;
    }

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
        const LONG bit = ( dw0 & i ) ? -1 : 0;
        r ^= bit & idxr;
        idxr += 0xffff0001;
    }

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

#else

//  ================================================================
XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
//  ================================================================
//
//  stub used for IA64 builds (function ptr never called)
//
//-
{
    Assert( fFalse );
    return MakeChecksumFromECCXORAndPgno( 0, 0, pgno );
}


#endif
#endif  //  1

//  ================================================================
static XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
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
    //  the upper bits of p and q may not be used depending on the size of the
    //  block.  and 8kb block will use all the bits.  each block size below
    //  that will use one fewer bit in both p and q.  any unused bits in the
    //  ECC will always be zero
    //
    //  the ECC is used to detect errors using the XOR of the expected value of
    //  the ECC and the actual value of the ECC as follows:
    //
    //    all bits are clear:     no error
    //    one bit is set:         one bit flip in the ECC was detected
    //    all used bits are set:  one bit flip in the block was detected
    //    random bits are set:    more than one bit flip was detected
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
static __declspec( naked ) PAGECHECKSUM __stdcall ChecksumSSE2_Emitted( const unsigned char* pb, const ULONG cb )
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
static PAGECHECKSUM ChecksumNewFormatSSE2_Wrapper( const unsigned char * const pb, const ULONG cb, const ULONG pgno )
//  ================================================================
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormatSSE2_Wrapper;

    Unused( pfn );

    PAGECHECKSUM checksum = ChecksumSSE2_Emitted( pb, cb );

    //  The emitted code returns a 64-bit number with the ECC as the low DWORD and XOR as the high DWORD

    const ULONG checksumXOR = (ULONG)(checksum >> 32);
    const ULONG checksumECC = (ULONG)(checksum & 0xffffffff);

    return MakeChecksumFromECCXORAndPgno( checksumECC, checksumXOR, pgno );
}

#endif  //  1

//  ================================================================
inline BOOL FGetBit( const void * const pv, const INT ibitOffset )
//  ================================================================
{
    const unsigned char * const pb  = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    return ( pb[ibyte] & bitMask ) ? fTrue : fFalse;
}

//  ================================================================
static UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype )
//  ================================================================
{
    if( databasePage == pagetype )
    {
        //  for database pages, the page flags are stored in the 10th
        //  unsigned long. The format bit is 0x1000, which is the 13th bit.
        return ( 9 * 32 ) + 13;
    }
    return ( UINT )-1;
}


//  ================================================================
static BOOL FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype )
//  ================================================================
//
//  Returns fTrue if the ECC format bit for the page is set. Database pages
//  and header pages have different formats, so we have to store the bit
//  in different places
//
//-
{
    if( databasePage == pagetype )
    {
        const INT ibit = IbitNewChecksumFormatFlag( pagetype );
        return FGetBit( pv, ibit );
    }
    return 0;
}

//  ================================================================
static BOOL FPageHasLongChecksum( const PAGETYPE pagetype )
//  ================================================================
{
    return ( databasePage == pagetype );
}

//  ================================================================
static ULONG ShortChecksumFromPage( const void * const pv )
//  ================================================================
{
    return *( const ULONG* )pv;
}



//  ================================================================
static PAGECHECKSUM LongChecksumFromPage( const void * const pv )
//  ================================================================
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


//  ================================================================
static PAGECHECKSUM ChecksumFromPage( const void * const pv, const PAGETYPE pagetype )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        return LongChecksumFromPage( pv );
    }
    return ShortChecksumFromPage( pv );
}


//  ================================================================
static PAGECHECKSUM ComputePageChecksum(
    const void* const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    const BOOL fNew )
//  ================================================================
{
    if( FPageHasLongChecksum( pagetype ) )
    {
        if( FPageHasNewChecksumFormat( pv, pagetype ) )
        {
            // large pages (16/32kiB) always have new checksum format
            PAGECHECKSUM pgChecksum;
            const unsigned char* const pch = ( unsigned char* )pv;

            // divide a large page into 4 blocks, first block is header block which hosts the page header
            unsigned cbT = cb;
            if ( !FIsSmallPage( cb ) )
            {
                cbT = cb / cxeChecksumPerPage;
                pgChecksum.rgChecksum[ 1 ] = pfnChecksumNewFormat( pch + cbT * 1, cbT, pgno, fFalse );
                pgChecksum.rgChecksum[ 2 ] = pfnChecksumNewFormat( pch + cbT * 2, cbT, pgno, fFalse );
                pgChecksum.rgChecksum[ 3 ] = pfnChecksumNewFormat( pch + cbT * 3, cbT, pgno, fFalse );

                // write checksums into designated location in header block
                // so checksum for header block can protect them as well
                if ( fNew )
                {
                    // cast RO ( const void* ) to RW ( PGHDR2* )
                    PGHDR2* const pPgHdr2 = ( PGHDR2* )pv;
                    pPgHdr2->rgChecksum[ 0 ] = pgChecksum.rgChecksum[ 1 ];
                    pPgHdr2->rgChecksum[ 1 ] = pgChecksum.rgChecksum[ 2 ];
                    pPgHdr2->rgChecksum[ 2 ] = pgChecksum.rgChecksum[ 3 ];
                }
            }

            // whole small page or header block for large page
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


//  ================================================================
static ULONG DwECCChecksumFromPagechecksum( const XECHECKSUM checksum )
//  ================================================================
{
    return (ULONG)( checksum >> 32 );
}


//  ================================================================
static ULONG DwXORChecksumFromPagechecksum( const XECHECKSUM checksum )
//  ================================================================
{
    return (ULONG)( checksum & 0xffffffff );
}


//  ================================================================
static INT CbitSet( const ULONG dw )
//  ================================================================
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

//  ================================================================
static BOOL FECCErrorIsCorrectable( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
//  ================================================================
{
    const ULONG dwEcc = DwECCChecksumFromPagechecksum( xeChecksumActual ) ^ DwECCChecksumFromPagechecksum( xeChecksumExpected );
    const ULONG dwXor = DwXORChecksumFromPagechecksum( xeChecksumActual ) ^ DwXORChecksumFromPagechecksum( xeChecksumExpected );

    Assert( xeChecksumActual != xeChecksumExpected );   //  nothing to correct?!

    const ULONG ulMask  = ( ( cb << 3 ) - 1 );
    const ULONG ulX     = ( ( dwEcc >> 16 ) ^ dwEcc ) & ulMask;

    // ulX has all bits set, correctable error

    if ( ulMask == ulX )
    {

        //  we can only have a single-bit error if the XOR checksum shows only one bit incorrect
        //  (of course multiple bits could be corrupted, but this check provides an extra level of
        //  safety)
        if( 1 == CbitSet( dwXor ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}

//  ================================================================
static UINT IbitCorrupted( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
//  ================================================================
{
    Assert( xeChecksumExpected != xeChecksumActual );                               //  nothing to correct?!
    Assert( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) );   //  not correctable?!

    const ULONG dwEcc = DwECCChecksumFromPagechecksum( xeChecksumActual ) ^ DwECCChecksumFromPagechecksum( xeChecksumExpected );
    Assert( 0 != dwEcc );

    return ( UINT )( dwEcc & 0xffff );
}

//  ================================================================
inline void FlipBit( void * const pv, const INT ibitOffset )
//  ================================================================
{
    unsigned char * const   pb      = (unsigned char *)pv;
    const INT   ibyte       = ibitOffset / 8;
    const INT   ibitInByte  = ibitOffset % 8;
    const unsigned char     bitMask     = (unsigned char)( 1 << ibitInByte );

    pb[ibyte] ^= bitMask;
}



//  ================================================================
enum XECHECKSUMERROR { xeChecksumNoError = 0, xeChecksumCorrectableError = -13, xeChecksumFatalError = -29, };

static XECHECKSUMERROR FTryFixBlock(
    const UINT cb,
    UINT* const pibitCorrupted,         // [in] bit offset of XECHECKSUM flag, [out] bit offset of proposed correction
    const XECHECKSUM xeChecksumExpected,
    const XECHECKSUM xeChecksumActual )
//  ================================================================
{
    if ( xeChecksumExpected == xeChecksumActual )
    {
        return xeChecksumNoError;
    }

    if ( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) )
    {
        const UINT ibitCorrupted = IbitCorrupted( cb, xeChecksumExpected, xeChecksumActual );

        // *pibitCorrupted is the offset of the BIT we can NOT flip
        if ( ( *pibitCorrupted != ibitCorrupted ) && ( ibitCorrupted < 8 * cb ) )
        {
            *pibitCorrupted = ibitCorrupted;
            return xeChecksumCorrectableError;
        }
    }

    return xeChecksumFatalError;
}

//  ================================================================
static void TryFixPage(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const BOOL fCorrectError,
    BOOL * const pfCorrectableError,
    INT * const pibitCorrupted,
    const PAGECHECKSUM checksumExpected,
    const PAGECHECKSUM checksumActual )
//  ================================================================
{
    Assert( checksumActual != checksumExpected );

    const BOOL fSmallPage = FIsSmallPage( cb );
    const UINT cbT = fSmallPage ? cb : cb / cxeChecksumPerPage;
    const UINT cblk = fSmallPage ? 1 : cxeChecksumPerPage;
    XECHECKSUMERROR rgErr[ cxeChecksumPerPage ] = { xeChecksumNoError, };

    UINT rgibitCorrupted[ cxeChecksumPerPage ] = { IbitNewChecksumFormatFlag( pagetype ), UINT_MAX, UINT_MAX, UINT_MAX, };
    UINT ibitCorrupted = UINT_MAX;

    // work out correction
    for ( UINT iblk = 0; iblk < cblk; ++iblk )
    {
        rgErr[ iblk ] = FTryFixBlock( cbT, &rgibitCorrupted[ iblk ], checksumExpected.rgChecksum[ iblk ], checksumActual.rgChecksum[ iblk ] );
        switch ( rgErr[ iblk ] )
        {
            case xeChecksumNoError:
                break;

            case xeChecksumCorrectableError:
                // save the location of first correctable error
                if ( UINT_MAX == ibitCorrupted )
                {
                    ibitCorrupted = CHAR_BIT * cbT * iblk + rgibitCorrupted[ iblk ];

                    // does this correctable error actually hit other checksums
                    UINT ibitStart = CHAR_BIT * offsetof( PGHDR2, rgChecksum );
                    UINT ibitLength = CHAR_BIT * sizeof( checksumExpected.rgChecksum[ 0 ] );
                    if ( !fSmallPage &&                 // large page
                        0 == iblk &&                    // header block
                        ibitStart <= ibitCorrupted  &&  // hit one of the other checksums
                        ibitCorrupted < ibitStart + ibitLength * ( cxeChecksumPerPage - 1 ) )
                    {
                        // fix the other hit checksum
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

    // when multiple correctables, just report first one
    *pfCorrectableError = fTrue;
    *pibitCorrupted = ibitCorrupted;
    Assert( (UINT)*pibitCorrupted < ( 8 * cb ) );

    // carry out correction in page
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


//  ================================================================
void ChecksumAndPossiblyFixPage(
    void * const pv,                    //  pointer to the page
    const UINT cb,              //  size of the page (normally g_cbPage)
    const PAGETYPE pagetype,            //  type of the page
    const ULONG pgno,
    const BOOL fCorrectError,           //  fTrue if ECC should be used to correct errors
    PAGECHECKSUM * const pchecksumExpected, //  set to the checksum the page should have
    PAGECHECKSUM * const pchecksumActual,   //  set the the actual checksum. if actual != expected, JET_errReadVerifyFailure is returned
    BOOL * const pfCorrectableError,    //  set to fTrue if ECC could correct the error (set even if fCorrectError is fFalse)
    INT * const pibitCorrupted )        //  offset of the corrupted bit (meaningful only if *pfCorrectableError is fTrue)
//  ================================================================
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

        // no point in re-computing  the checksum if we haven't done any changes
        if ( fCorrectError && *pfCorrectableError )
        {
            *pchecksumExpected = ChecksumFromPage( pv, pagetype );
            *pchecksumActual = ComputePageChecksum( pv, cb, pagetype, pgno, fTrue );
        }
    }
}


