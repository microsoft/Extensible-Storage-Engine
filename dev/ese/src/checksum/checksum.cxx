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

#include "checksumstd.hxx"

//  ****************************************************************
//  XOR checksum routines
//  ****************************************************************

typedef ULONG( *PFNCHECKSUMOLDFORMAT )( const unsigned char * const, const ULONG );

ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormatSlowly( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb );

//  see comments in checksum_amd64.cxx to see why these are in a separate file

ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb );

static PFNCHECKSUMOLDFORMAT pfnChecksumOldFormat = ChecksumSelectOldFormat;

ULONG ChecksumOldFormat( const unsigned char * const pb, const ULONG cb )
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat;
    Unused( pfn );

    return pfnChecksumOldFormat( pb, cb );
}

//  ****************************************************************
//  ECC checksum routines
//  ****************************************************************

typedef XECHECKSUM( *PFNCHECKSUMNEWFORMAT )( const unsigned char * const, const ULONG, const ULONG, BOOL );

XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );

//  see comments in checksum_amd64.cxx to see why these are in a separate file

enum ChecksumParityMaskFunc
{
    ParityMaskFuncDefault = 0,
    ParityMaskFuncPopcnt,
};

XECHECKSUM ChecksumNewFormatSlowly( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
XECHECKSUM ChecksumNewFormat64Bit( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
XECHECKSUM ChecksumNewFormatSSE( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );
template <ChecksumParityMaskFunc TParityMaskFunc> XECHECKSUM ChecksumNewFormatSSE2( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue);
XECHECKSUM ChecksumNewFormatAVX( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );

PFNCHECKSUMNEWFORMAT pfnChecksumNewFormat = ChecksumSelectNewFormat;

XECHECKSUM ChecksumNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumNewFormat;
    Unused( pfn );

    return pfnChecksumNewFormat( pb, cb, pgno, fHeaderBlock );
}


//  ================================================================
ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb )
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
//  Cyrix etc.  : ChecksumOldFormatSlowly
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumSelectOldFormat;

#if (defined(_M_IX86) && defined(_CHPE_X86_ARM64_)) || (defined(_M_AMD64) && defined(_ARM64EC_))
    pfn = ChecksumOldFormatSlowly;
#else
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
        pfn = ChecksumOldFormatSlowly;
    }
#endif

    pfnChecksumOldFormat = pfn;

    return (*pfn)( pb, cb );
}


//  ================================================================
ULONG ChecksumOldFormatSlowly( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Plain old unrolled-loop checksum that should work on any processor
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormatSlowly;

    Unused( pfn );

    const ULONG * pdw   = (ULONG *)pb;
    const INT cDWords           = 8;
    const INT cbStep            = cDWords * sizeof( ULONG );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );

    //  remove the first unsigned long, as it is the checksum itself

    ULONG   dwChecksum = 0x89abcdef ^ pdw[0];

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
ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb )
//  ================================================================
//
//  Do checksumming 64 bits at a time (the native word size)
//
//-
{
    PFNCHECKSUMOLDFORMAT pfn = ChecksumOldFormat64Bit;

    Unused( pfn );

    const unsigned __int64* pqw = (unsigned __int64 *)pb;
    unsigned __int64 qwChecksum = 0;
    const INT cQWords           = 4;
    const INT cbStep            = cQWords * sizeof( unsigned __int64 );
    __int64 cbT                 = cb;
    Assert( 0 == ( cbT % cbStep ) );

    //  checksum the first four bytes twice to remove the checksum

    qwChecksum ^= pqw[0] & 0x00000000FFFFFFFF;

    while ( ( cbT -= cbStep ) >= 0 )
    {
        qwChecksum ^= pqw[0];
        qwChecksum ^= pqw[1];
        qwChecksum ^= pqw[2];
        qwChecksum ^= pqw[3];
        pqw += cQWords;
    }
    
    const unsigned __int64 qwUpper = ( qwChecksum >> ( sizeof( ULONG ) * 8 ) );
    const unsigned __int64 qwLower = qwChecksum & 0x00000000FFFFFFFF;
    qwChecksum = qwUpper ^ qwLower;

    const ULONG ulChecksum = static_cast<ULONG>( qwChecksum ) ^ 0x89abcdef;
    return ulChecksum;
}


//  ================================================================
XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock )
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
//  P2/P3                       : ChecksumNewFormatSSE
//  P4                          : ChecksumNewFormatSSE2<ParityMaskFuncDefault>
//  AMD64                       : ChecksumNewFormatSSE2<ParityMaskFuncDefault>
//    - Nehalem/Barcelona       : ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>
//    - SandyBridge/Bulldozer   : ChecksumNewFormatAVX
//  Cyrix etc.                  : ChecksumNewFormatSlowly
//
//-
{
    PFNCHECKSUMNEWFORMAT pfn = ChecksumSelectNewFormat;

#if (defined(_M_IX86)  && defined(_CHPE_X86_ARM64_)) || (defined(_M_AMD64) && defined(_ARM64EC_))
    pfn = ChecksumNewFormatSlowly;
#else
    if( FAVXEnabled() && FPopcntAvailable() )
    {
        pfn = ChecksumNewFormatAVX;
    }
    else if( FSSEInstructionsAvailable() )
    {
        if( FSSE2InstructionsAvailable() )
        {
            if( FPopcntAvailable() )
            {
                pfn = ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>;
            }
            else
            {
                pfn = ChecksumNewFormatSSE2<ParityMaskFuncDefault>;
            }
        }
        else
        {
            pfn = ChecksumNewFormatSSE;
        }
    }
    else if( sizeof( DWORD_PTR ) == sizeof( ULONG ) * 2 )
    {
        pfn = ChecksumNewFormat64Bit;
    }
    else
    {
        pfn = ChecksumNewFormatSlowly;
    }
#endif

    pfnChecksumNewFormat = pfn;

    return (*pfn)( pb, cb, pgno, fHeaderBlock );
}


//  ================================================================
ULONG DwECCChecksumFromXEChecksum( const XECHECKSUM checksum )
//  ================================================================
{
    return (ULONG)( checksum >> 32 );
}


//  ================================================================
ULONG DwXORChecksumFromXEChecksum( const XECHECKSUM checksum )
//  ================================================================
{
    return (ULONG)( checksum & 0xffffffff );
}


//  ================================================================
INT CbitSet( const ULONG dw )
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
BOOL FECCErrorIsCorrectable( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
//  ================================================================
{
    Assert( xeChecksumActual != xeChecksumExpected );   //  nothing to correct?!

    const DWORD dwECCChecksumExpected = DwECCChecksumFromXEChecksum( xeChecksumExpected );
    const DWORD dwECCChecksumActual = DwECCChecksumFromXEChecksum( xeChecksumActual );

    if ( FECCErrorIsCorrectable( cb, dwECCChecksumExpected, dwECCChecksumActual ) )
    {
        const ULONG dwXor = DwXORChecksumFromXEChecksum( xeChecksumActual ) ^ DwXORChecksumFromXEChecksum( xeChecksumExpected );

        //  we can only have a single-bit error if the XOR checksum shows only one bit incorrect
        //  (of course multiple bits could be corrupted, but this check provides an extra level of
        //  safety)
        if ( 1 == CbitSet( dwXor ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}


//  ================================================================
UINT IbitCorrupted( const UINT cb, const XECHECKSUM xeChecksumExpected, const XECHECKSUM xeChecksumActual )
//  ================================================================
{
    Assert( xeChecksumExpected != xeChecksumActual );                               //  nothing to correct?!
    Assert( FECCErrorIsCorrectable( cb, xeChecksumExpected, xeChecksumActual ) );   //  not correctable?!

    const DWORD dwECCChecksumExpected = DwECCChecksumFromXEChecksum( xeChecksumExpected );
    const DWORD dwECCChecksumActual = DwECCChecksumFromXEChecksum( xeChecksumActual );
    Assert( dwECCChecksumExpected != dwECCChecksumActual ); //  nothing to correct?!

    return IbitCorrupted( cb, dwECCChecksumExpected, dwECCChecksumActual );
}

//  ================================================================
BOOL FECCErrorIsCorrectable( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual )
//  ================================================================
{
    const ULONG dwEcc = dwECCChecksumActual ^ dwECCChecksumExpected;

    const ULONG ulMask = ( ( cb << 3 ) - 1 );
    const ULONG ulX = ( ( dwEcc >> 16 ) ^ dwEcc ) & ulMask;

    // ulX has all bits set, correctable error

    return ulMask == ulX;
}

//  ================================================================
UINT IbitCorrupted( const UINT cb, const ULONG dwECCChecksumExpected, const ULONG dwECCChecksumActual )
//  ================================================================
{
    const ULONG dwEcc = dwECCChecksumActual ^ dwECCChecksumExpected;

    if ( dwEcc == 0 )
    {
        return ulMax;
    }

    const UINT ibitCorrupted    = (UINT)( dwEcc & 0xffff );
    const UINT ibCorrupted      = ibitCorrupted / 8;

    if ( ibCorrupted >= cb )
    {
        return ulMax;
    }

    return ibitCorrupted;
}
