// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include <math.h>

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif




inline BOOL FGetBit( const void * const pv, const INT ibitOffset );
inline void SetBit( void * const pv, const INT ibitOffset );
inline void FlipBit( void * const pv, const INT ibitOffset );


UINT IbitNewChecksumFormatFlag( const PAGETYPE pagetype );
BOOL    FPageHasNewChecksumFormat( const void * const pv, const PAGETYPE pagetype );
void        SetNewChecksumFormatFlag( void * const pv, const PAGETYPE pagetype );


BOOL            FPageHasLongChecksum( const PAGETYPE pagetype );

ULONG   ShortChecksumFromPage( const void * const pv );
void                SetShortChecksumOnPage( void * const pv, const ULONG checksum );

PAGECHECKSUM    LongChecksumFromPage( const void * const pv, const ULONG cb );
void            SetLongChecksumOnPage( void * const pv, const ULONG cb, const PAGECHECKSUM checksum );

PAGECHECKSUM    ChecksumFromPage( const void * const pv, const ULONG cb, const PAGETYPE pagetype );
void            SetChecksumOnPage( void * const pv, const ULONG cb, const PAGETYPE pagetype, const PAGECHECKSUM checksum );

ULONG   CbBlockSize( const ULONG cb );

PAGECHECKSUM    ComputePageChecksum(
    const void* const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const ULONG pgno,
    const BOOL fNew = fFalse );


void TryFixPage(
    void * const pv,
    const UINT cb,
    const PAGETYPE pagetype,
    const BOOL fCorrectError,
    BOOL * const pfCorrectableError,
    INT * const pibitCorrupted,
    PAGECHECKSUM checksumExpected,
    const PAGECHECKSUM checksumActual );

typedef XECHECKSUM( *PFNCHECKSUMNEWFORMAT )( const unsigned char * const, const ULONG, const ULONG, BOOL );
typedef ULONG( *PFNCHECKSUMOLDFORMAT )( const unsigned char * const, const ULONG );


XECHECKSUM LongChecksumFromShortChecksum( const ULONG xorChecksum, const ULONG pgno );

ULONG ChecksumSelectOldFormat( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormatSlowly( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormat64Bit( const unsigned char * const pb, const ULONG cb );


ULONG ChecksumOldFormatSSE( const unsigned char * const pb, const ULONG cb );
ULONG ChecksumOldFormatSSE2( const unsigned char * const pb, const ULONG cb );


XECHECKSUM ChecksumSelectNewFormat( const unsigned char * const pb, const ULONG cb, const ULONG pgno, BOOL fHeaderBlock = fTrue );


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



static void BitRoutinesUnitTest( unsigned char * const pb, UINT cbPageTest )
{
    pb[3] = 0;
    
    AssertRTL( !FGetBit( pb, 25 ) );
    SetBit( pb, 25 );
    AssertRTL( FGetBit( pb, 25 ) );
    FlipBit( pb, 25 );
    AssertRTL( !FGetBit( pb, 25 ) );
    FlipBit( pb, 25 );
    AssertRTL( FGetBit( pb, 25 ) );

    AssertRTL( FPageHasLongChecksum( databasePage ) );
    AssertRTL( !FPageHasLongChecksum( databaseHeader ) );
    AssertRTL( !FPageHasLongChecksum( logfileHeader ) );

    DWORD * const pdw = (DWORD *)pb;
    pdw[9] = 0;

    AssertRTL( !FPageHasNewChecksumFormat( pb, databaseHeader ) );
    AssertRTL( !FPageHasNewChecksumFormat( pb, logfileHeader ) );
    AssertRTL( !FPageHasNewChecksumFormat( pb, databasePage ) );
    SetNewChecksumFormatFlag( pb, databasePage );
    AssertRTL( !FPageHasNewChecksumFormat( pb, databaseHeader ) );
    AssertRTL( !FPageHasNewChecksumFormat( pb, logfileHeader ) );
    AssertRTL( FPageHasNewChecksumFormat( pb, databasePage ) );

    SetShortChecksumOnPage( pb, 0x12345678 );
    AssertRTL( 0x12345678 == ShortChecksumFromPage( pb ) );
    AssertRTL( 0x12345678 == ChecksumFromPage( pb, cbPageTest, databaseHeader ) );

    SetChecksumOnPage( pb, cbPageTest, databaseHeader, 0x87654321 );
    AssertRTL( 0x87654321 == ShortChecksumFromPage( pb ) );
    AssertRTL( 0x87654321 == ChecksumFromPage( pb, cbPageTest, databaseHeader ) );

    SetLongChecksumOnPage( pb, cbPageTest, 0x89ABCDEFBADF00D );
    AssertRTL( 0x89ABCDEFBADF00D == LongChecksumFromPage( pb, cbPageTest ) );
    AssertRTL( 0x89ABCDEFBADF00D == ChecksumFromPage( pb, cbPageTest, databasePage ) );

    SetChecksumOnPage( pb, cbPageTest, databasePage, 0xBADF00D89ABCDEF );
    AssertRTL( 0xBADF00D89ABCDEF == LongChecksumFromPage( pb, cbPageTest ) );
    AssertRTL( 0xBADF00D89ABCDEF == ChecksumFromPage( pb, cbPageTest, databasePage ) );
}

static void XORChecksumUnitTest( const unsigned char * const pb )
{
    const ULONG dwChecksum4KB           = ChecksumOldFormatSlowly( pb, 4096 );
    const ULONG dwChecksum4KBSSE        = FSSEInstructionsAvailable()  ? ChecksumOldFormatSSE( pb, 4096 ) : dwChecksum4KB;
    const ULONG dwChecksum4KBSSE2       = FSSE2InstructionsAvailable()  ? ChecksumOldFormatSSE2( pb, 4096 ) : dwChecksum4KB;
    const ULONG dwChecksum4KBSelect     = ChecksumSelectOldFormat( pb, 4096 );
    const ULONG dwChecksum4KB64Bit      = ChecksumOldFormat64Bit( pb, 4096 );

    AssertRTL( dwChecksum4KB == dwChecksum4KBSSE );
    AssertRTL( dwChecksum4KB == dwChecksum4KBSSE2 );
    AssertRTL( dwChecksum4KB == dwChecksum4KB64Bit );
    AssertRTL( dwChecksum4KB == dwChecksum4KBSelect );

    const ULONG dwChecksum8KB           = ChecksumOldFormatSlowly( pb, 8192 );
    const ULONG dwChecksum8KBSSE        = FSSEInstructionsAvailable()  ? ChecksumOldFormatSSE( pb, 8192 ) : dwChecksum8KB;
    const ULONG dwChecksum8KBSSE2       = FSSE2InstructionsAvailable()  ? ChecksumOldFormatSSE2( pb, 8192 ) : dwChecksum8KB;
    const ULONG dwChecksum8KBSelect     = ChecksumSelectOldFormat( pb, 8192 );
    const ULONG dwChecksum8KB64Bit      = ChecksumOldFormat64Bit( pb, 8192 );

    AssertRTL( dwChecksum8KB == dwChecksum8KBSSE );
    AssertRTL( dwChecksum8KB == dwChecksum8KBSSE2 );
    AssertRTL( dwChecksum8KB == dwChecksum8KB64Bit );
    AssertRTL( dwChecksum8KB == dwChecksum8KBSelect );
}

static void ECCChecksumUnitTest( const unsigned char * const pb )
{
    const ULONG pgno                    = 1234;
    
    const XECHECKSUM checksum4KB            = ChecksumNewFormatSlowly( pb, 4096, pgno );
    const XECHECKSUM checksum4KBSSE         = FSSEInstructionsAvailable()  ? ChecksumNewFormatSSE( pb, 4096, pgno ) : checksum4KB;
    const XECHECKSUM checksum4KBSSE2        = FSSE2InstructionsAvailable()  ? ChecksumNewFormatSSE2<ParityMaskFuncDefault>( pb, 4096, pgno ) : checksum4KB;
    const XECHECKSUM checksum4KBSelect      = ChecksumSelectNewFormat( pb, 4096, pgno );
    const XECHECKSUM checksum4KB64Bit       = ChecksumNewFormat64Bit( pb, 4096, pgno );
    const XECHECKSUM checksum4KBSSE2_Popcnt = FPopcntAvailable()  ? ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>( pb, 4096, pgno ) : checksum4KB;
    const XECHECKSUM checksum4KBAVX         = FAVXEnabled() ? ChecksumNewFormatAVX( pb, 4096, pgno ) : checksum4KB;

    Enforce( checksum4KB == checksum4KBSSE );
    Enforce( checksum4KB == checksum4KBSSE2 );
    Enforce( checksum4KB == checksum4KB64Bit );
    Enforce( checksum4KB == checksum4KBSelect );
    Enforce( checksum4KB == checksum4KBSSE2_Popcnt );
    Enforce( checksum4KB == checksum4KBAVX );

    const XECHECKSUM checksum8KB            = ChecksumNewFormatSlowly( pb, 8192, pgno );
    const XECHECKSUM checksum8KBSSE         = FSSEInstructionsAvailable()  ? ChecksumNewFormatSSE( pb, 8192, pgno ) : checksum8KB;
    const XECHECKSUM checksum8KBSSE2        = FSSE2InstructionsAvailable()  ? ChecksumNewFormatSSE2<ParityMaskFuncDefault>( pb, 8192, pgno ) : checksum8KB;
    const XECHECKSUM checksum8KBSelect      = ChecksumSelectNewFormat( pb, 8192, pgno );
    const XECHECKSUM checksum8KB64Bit       = ChecksumNewFormat64Bit( pb, 8192, pgno );
    const XECHECKSUM checksum8KBSSE2_Popcnt = FPopcntAvailable()  ? ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>( pb, 8192, pgno ) : checksum8KB;
    const XECHECKSUM checksum8KBAVX         = FAVXEnabled() ? ChecksumNewFormatAVX( pb, 8192, pgno ) : checksum8KB;

    Enforce( checksum8KB == checksum8KBSSE );
    Enforce( checksum8KB == checksum8KBSSE2 );
    Enforce( checksum8KB == checksum8KB64Bit );
    Enforce( checksum8KB == checksum8KBSelect );
    Enforce( checksum8KB == checksum8KBSSE2_Popcnt );
    Enforce( checksum8KB == checksum8KBAVX );
}


extern PFNCHECKSUMNEWFORMAT pfnChecksumNewFormat;

static void TestECCChecksumSelection()
{
    if ( FAVXEnabled() )
    {
        Enforce( pfnChecksumNewFormat == ChecksumNewFormatAVX );
    }
    else if ( FPopcntAvailable() )
    {
        Enforce( pfnChecksumNewFormat == ChecksumNewFormatSSE2<ParityMaskFuncPopcnt> );
    }
    else if ( FSSE2InstructionsAvailable() )
    {
        Enforce( pfnChecksumNewFormat == ChecksumNewFormatSSE2<ParityMaskFuncDefault> );
    }
    else if ( FSSEInstructionsAvailable() )
    {
        Enforce( pfnChecksumNewFormat == ChecksumNewFormatSSE );
    }
    else if ( sizeof(DWORD_PTR) == sizeof(ULONG) * 2 )
    {
        Enforce( pfnChecksumNewFormat == ChecksumNewFormat64Bit );
    }
    else
    {
        Enforce( pfnChecksumNewFormat == ChecksumNewFormatSlowly );
    }
}

static void TestChecksumOnePage( unsigned char * const pb, const INT cb, const PAGETYPE pagetype )
{
    SetPageChecksum( pb, cb, pagetype, 0x12 );
    
    PAGECHECKSUM checksumExpected;
    PAGECHECKSUM checksumActual;

    ChecksumPage( pb, cb, pagetype, 0x12, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    if( databasePage == pagetype )
    {
        ChecksumPage( pb, cb, pagetype, 0x13, &checksumExpected, &checksumActual );
        AssertRTL( checksumExpected != checksumActual );
    }

    BOOL fCorrectableError;
    INT ibitCorrupted;
    
    ChecksumAndPossiblyFixPage( pb, cb, pagetype, 0x12, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( checksumExpected == checksumActual );
}

static void TestFixOnePage( unsigned char * const pb, const INT cb, const PAGETYPE pagetype, const INT ibitToCorrupt )
{
    Assert( ibitToCorrupt >= 0 );
    Assert( ibitToCorrupt < ( cb * 8 ) );

    const ULONG pgno = 9876;
    
    
    SetPageChecksum( pb, cb, pagetype, pgno );
    AssertRTL( FPageHasNewChecksumFormat( pb, pagetype ) );

    PAGECHECKSUM checksumExpected;
    PAGECHECKSUM checksumActual;

    ChecksumPage( pb, cb, pagetype, pgno, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    const PAGECHECKSUM checksumOriginal = checksumExpected;

    
    FlipBit( pb, ibitToCorrupt );
    
    ChecksumPage( pb, cb, pagetype, pgno, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected != checksumActual );

    
    BOOL fCorrectableError;
    INT ibitCorrupted;
    
    ChecksumAndPossiblyFixPage( pb, cb, pagetype, pgno, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( checksumExpected == checksumActual );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitToCorrupt == ibitCorrupted );

    ChecksumPage( pb, cb, pagetype, pgno, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );
    AssertRTL( checksumOriginal == checksumExpected );
    AssertRTL( FPageHasNewChecksumFormat( pb, pagetype ) );
}

static void TestFailToFixOnePage( unsigned char * const pb, const INT cb, const PAGETYPE pagetype, const INT ibitToCorrupt )
{
    Assert( ibitToCorrupt >= 0 );
    Assert( ibitToCorrupt < ( cb * 8 ) );

    const ULONG pgno = 888888;
    
    
    SetPageChecksum( pb, cb, pagetype, pgno );

    PAGECHECKSUM checksumExpected;
    PAGECHECKSUM checksumActual;

    ChecksumPage( pb, cb, pagetype, pgno,  &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    
    FlipBit( pb, ibitToCorrupt );
    
    ChecksumPage( pb, cb, pagetype, pgno, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected != checksumActual );

    
    BOOL fCorrectableError;
    INT ibitCorrupted;
    
    ChecksumAndPossiblyFixPage( pb, cb, pagetype, pgno, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( checksumExpected != checksumActual );
    AssertRTL( !fCorrectableError );
}

static void TestCorruptOnePage( unsigned char * const pb, const INT cb, const PAGETYPE pagetype )
{
    const ULONG pgno = 1;
    
    
    SetPageChecksum( pb, cb, pagetype, pgno );

    PAGECHECKSUM checksumExpected;
    PAGECHECKSUM checksumActual;

    ChecksumPage( pb, cb, pagetype, pgno, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );


    const INT           idw     = 365;
    ULONG * const       pdw     = (ULONG *)pb;
    const ULONG         dwOrig  = pdw[idw];
    const ULONG         dwNew   = ( 4096 == cb ) ? ( dwOrig ^ 0xBADF00D ) : ~dwOrig;
    AssertRTL( dwOrig != dwNew );
    pdw[idw] = dwNew;
    
    ChecksumPage( pb, cb, pagetype, pgno, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected != checksumActual );
    
    
    BOOL fCorrectableError;
    INT ibitCorrupted;
    
    ChecksumAndPossiblyFixPage( pb, cb, pagetype, pgno, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( checksumExpected != checksumActual );
    AssertRTL( !fCorrectableError );
}

static void TestSetAndChecksum( unsigned char * const pb )
{
    
    TestChecksumOnePage( pb, 8192, databasePage );
    TestChecksumOnePage( pb, 8192, databaseHeader );
    TestChecksumOnePage( pb, 8192, logfileHeader );

    TestChecksumOnePage( pb, 4096, databasePage );
    TestChecksumOnePage( pb, 4096, databaseHeader );
    TestChecksumOnePage( pb, 4096, logfileHeader );


    TestFixOnePage( pb, 8192, databasePage, 100 );
    TestFixOnePage( pb, 8192, databasePage, 129 );
    TestFixOnePage( pb, 8192, databasePage, 3097 );
    TestFixOnePage( pb, 8192, databasePage, ( 8192 * 8 ) - 1 );


    
#ifdef CHECKSUM_TEST_ALL_BITS
    for( ULONG ibit = 40 * 8; ibit < ( 32 * 1024 * 8 ); ibit++ )
    {
#else
    for( ULONG i = 0; i < 16; i++ )
    {
        ULONG ibit = ( rand() % ( 32 * 1024 - 40 ) ) * (  8   ) + ( 40 * 8 ) + ( rand() % 8 );
#endif
        TestFixOnePage( pb, 32 * 1024, databasePage, ibit );
    }

    TestFixOnePage( pb, 4096, databasePage, 64 );
    TestFixOnePage( pb, 4096, databasePage, 29000 );
    TestFixOnePage( pb, 4096, databasePage, 30009 );
    TestFixOnePage( pb, 4096, databasePage, ( 4096 * 8 ) - 1 );

    
    TestFixOnePage( pb, 4096, databasePage, ( ( rand() % 4000  ) + 40 ) * 8 );


    TestFailToFixOnePage( pb, 8192, databasePage, 0 );
    TestFailToFixOnePage( pb, 8192, databasePage, IbitNewChecksumFormatFlag( databasePage ) );

    TestFailToFixOnePage( pb, 4096, databasePage, 1 );
    TestFailToFixOnePage( pb, 4096, databasePage, IbitNewChecksumFormatFlag( databasePage ) );
    

    TestFailToFixOnePage( pb, 8192, databaseHeader, 100 );
    TestFailToFixOnePage( pb, 8192, logfileHeader, 129 );
    TestFailToFixOnePage( pb, 8192, databaseHeader, 3097 );
    TestFailToFixOnePage( pb, 8192, logfileHeader, IbitNewChecksumFormatFlag( databasePage ) );
    TestFailToFixOnePage( pb, 8192, databaseHeader, ( 8192 * 8 ) - 1 );

    TestFailToFixOnePage( pb, 4096, logfileHeader, 32 );
    TestFailToFixOnePage( pb, 4096, databaseHeader, 29000 );
    TestFailToFixOnePage( pb, 4096, logfileHeader, 30009 );
    TestFailToFixOnePage( pb, 4096, databaseHeader, IbitNewChecksumFormatFlag( databasePage ) );
    TestFailToFixOnePage( pb, 4096, logfileHeader, ( 4096 * 8 ) - 1 );


    TestCorruptOnePage( pb, 8192, databasePage );
    TestCorruptOnePage( pb, 8192, databaseHeader );
    TestCorruptOnePage( pb, 8192, logfileHeader );

    TestCorruptOnePage( pb, 4096, databasePage );
    TestCorruptOnePage( pb, 4096, databaseHeader );
    TestCorruptOnePage( pb, 4096, logfileHeader );
}

static void TestDehydratedPageChecksum( unsigned char * const pb )
{
    for ( INT cb = 4096; cb <= g_cbPageMax; cb += 4096 )
    {
        TestChecksumOnePage( pb, cb, databasePage );
        TestFixOnePage( pb, cb, databasePage, (cb * CHAR_BIT) - 1 );
    }
}

VOID KnownPageUnitTest( __out_bcount( cbSize ) unsigned char * const pb, __in_range( 32 * 1024, 32 * 1024 ) INT cbSize )
{
    INT ib;

    AssertRTLPREFIX(cbSize >= g_cbPage);
    for( ib = 0; (ib < g_cbPageDefault) && (ib < cbSize); ++ib )
    {
        pb[ib] = (unsigned char)ib;
    }

    PAGECHECKSUM checksumExpected;
    PAGECHECKSUM checksumActual;


    const PAGECHECKSUM checksumDatabasePage = 0x0000000004042290;

    SetPageChecksum( pb, 8192, databasePage, 9876 );
    ChecksumPage( pb, 8192, databasePage, 9876, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );
    AssertRTL( checksumDatabasePage == checksumExpected );


    const DWORD pgno        = 0x12345678;
    const DWORD xorChecksum = 0x89123456;
    
    DWORD * const pdw = (DWORD *)pb;
    pdw[0] = xorChecksum;
    pdw[1] = pgno;
    AssertRTL( LongChecksumFromShortChecksum( xorChecksum, pgno ) == LongChecksumFromPage( pb, 8192 ) );


    const PAGECHECKSUM checksumDatabaseHeader = 0x9f9b9f93;

    SetPageChecksum( pb, 8192, databaseHeader, 0 );
    ChecksumPage( pb, 8192, databaseHeader, 0, &checksumExpected, &checksumActual );
    AssertRTL( checksumExpected == checksumActual );
    AssertRTL( checksumDatabaseHeader == checksumExpected );

{
    const UINT cbPageTest = 1024 * 32;
    for( UINT i = 0; i < cbPageTest; ++i )
    {
        pb[ i ] = ( unsigned char )( i + i / 13 );
    }

    const PGNO pgnoLarge = 0x98765431;
    SetPageChecksum( pb, cbPageTest, databasePage, pgnoLarge );

    BOOL fCorrectableError = 0;
    INT ibitCorrupted = 0;
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );

    const PAGECHECKSUM checksumDatabasePage32k( 0xf9ce063147b6b865, 0x69af69afcdd590e4, 0x770988f62ea1a9e2, 0x7cb77cb77b839a0e );
    AssertRTL( !fCorrectableError );
    AssertRTL( -1 == ibitCorrupted );
    AssertRTL( checksumDatabasePage32k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    const INT ibitFlip0 = CHAR_BIT * 40 + 1;
    const INT ibitFlip1 = CHAR_BIT * 1024 * 8 + 2;
    const INT ibitFlip2 = CHAR_BIT * 1024 * 16 + 3;
    const INT ibitFlip3 = CHAR_BIT * 1024 * 24 + 4;

    FlipBit( pb, ibitFlip0 );
    FlipBit( pb, ibitFlip1 );
    FlipBit( pb, ibitFlip2 );
    FlipBit( pb, ibitFlip3 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip0 == ibitCorrupted );
    AssertRTL( checksumDatabasePage32k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    FlipBit( pb, ibitFlip0 );
    FlipBit( pb, ibitFlip1 );
    FlipBit( pb, ibitFlip2 );
    FlipBit( pb, ibitFlip3 );

    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fFalse, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip0 == ibitCorrupted );
    AssertRTL( checksumExpected != checksumActual );

    FlipBit( pb, ibitFlip0 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fFalse, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip1 == ibitCorrupted );
    AssertRTL( checksumExpected != checksumActual );

    FlipBit( pb, ibitFlip1 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fFalse, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip2 == ibitCorrupted );
    AssertRTL( checksumExpected != checksumActual );

    FlipBit( pb, ibitFlip2 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fFalse, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip3 == ibitCorrupted );
    AssertRTL( checksumExpected != checksumActual );

    FlipBit( pb, ibitFlip3 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fFalse, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( checksumDatabasePage32k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );
}

{
    const UINT cbPageTest = 1024 * 16;
    const PGNO pgnoLarge = 0x98765431;
    SetPageChecksum( pb, cbPageTest, databasePage, pgnoLarge );

    BOOL fCorrectableError = 0;
    INT ibitCorrupted = 0;
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );

    const PAGECHECKSUM checksumDatabasePage16k( 0x7b807b80651e03fc, 0x7c347c34bc66a14b, 0x6c7e6c7ecb3a6c94, 0x5d105d19e99a841 );
    AssertRTL( !fCorrectableError );
    AssertRTL( -1 == ibitCorrupted );
    AssertRTL( checksumDatabasePage16k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    const INT ibitFlip0 = CHAR_BIT * 40 + 1;
    const INT ibitFlip1 = CHAR_BIT * 1024 * 4 + 2;
    const INT ibitFlip2 = CHAR_BIT * 1024 * 8 + 3;
    const INT ibitFlip3 = CHAR_BIT * 1024 * 12 + 4;

    FlipBit( pb, ibitFlip0 );
    FlipBit( pb, ibitFlip1 );
    FlipBit( pb, ibitFlip2 );
    FlipBit( pb, ibitFlip3 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip0 == ibitCorrupted );
    AssertRTL( checksumDatabasePage16k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );
}

{
    const UINT cbPageTest = 1024 * 28;
    const PGNO pgnoLarge = 0x98765431;
    SetPageChecksum( pb, cbPageTest, databasePage, pgnoLarge );

    BOOL fCorrectableError = 0;
    INT ibitCorrupted = 0;
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );

    const PAGECHECKSUM checksumDatabasePage28k( 0xf9d80627e9c2387e, 0x69af69afcdd590e4, 0x770988f62ea1a9e2, 0x1f311f31b6717993 );
    AssertRTL( !fCorrectableError );
    AssertRTL( -1 == ibitCorrupted );
    AssertRTL( checksumDatabasePage28k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );

    const INT ibitFlip0 = CHAR_BIT * 40 + 1;
    const INT ibitFlip1 = CHAR_BIT * 1024 * 8 + 2;
    const INT ibitFlip2 = CHAR_BIT * 1024 * 16 + 3;
    const INT ibitFlip3 = CHAR_BIT * 1024 * 24 + 4;

    FlipBit( pb, ibitFlip0 );
    FlipBit( pb, ibitFlip1 );
    FlipBit( pb, ibitFlip2 );
    FlipBit( pb, ibitFlip3 );
    ChecksumAndPossiblyFixPage( pb, cbPageTest, databasePage, pgnoLarge, fTrue, &checksumExpected, &checksumActual, &fCorrectableError, &ibitCorrupted );
    AssertRTL( fCorrectableError );
    AssertRTL( ibitFlip0 == ibitCorrupted );
    AssertRTL( checksumDatabasePage28k == checksumActual );
    AssertRTL( checksumExpected == checksumActual );
}

}

INT IbitFirstBitToTest()
{
    return 0;
}

INT CbChunkSizeToTestRemainingBlocks()
{
    return sizeof( XECHECKSUM );
}

INT CbChunkSizeToTestFirstBlock()
{
    return ( max( sizeof( PAGECHECKSUM ), IbitNewChecksumFormatFlag( databasePage ) / 8 + 1 ) + CbChunkSizeToTestRemainingBlocks() );
}

INT IbitNextBitToTest( const INT ibitCurrent, const INT cbPageSize )
{
    const INT ibCurrent = ibitCurrent / 8;
    INT ibitNext = ibitCurrent + 1;
    INT ibNext = ibitNext / 8;

    AssertRTL( ibNext <= cbPageSize );


    if ( ibNext == ibCurrent )
    {
        return ibitNext;
    }

    const INT cbBlock = CbBlockSize( cbPageSize );
    const INT iBlockCurrent = ibCurrent / cbBlock;
    const INT ibNextFirstInBlock = ( ibNext / cbBlock ) * cbBlock;
    const INT ibNextFromBlock = ibNext % cbBlock;
    const INT cbChunkFirst = CbChunkSizeToTestFirstBlock();
    const INT cbChunkRemaining = CbChunkSizeToTestRemainingBlocks();
    const INT cbChunk = ( iBlockCurrent == 0 ) ? cbChunkFirst : cbChunkRemaining;


    if ( ibNextFromBlock == cbChunk )
    {

        ibNext = ibNextFirstInBlock + cbBlock;
        if ( ibNext >= cbPageSize )
        {
            AssertRTL( ibNext == cbPageSize );
            return -1;
        }
    }

    ibitNext = 8 * ibNext;

    return ibitNext;
}

VOID AssertPagesAreNotSame( const unsigned char * const pb1, const unsigned char * const pb2, const INT cb )
{
    AssertRTL( memcmp( pb1, pb2, cb ) != 0 );
}

VOID AssertPagesAreSame( const unsigned char * const pb1, const unsigned char * const pb2, const INT cb )
{
    if ( memcmp( pb1, pb2, cb ) == 0 )
    {
        return;
    }

    INT ibFirstMismatch = -1;

    for ( INT ib = 0; ib < cb ; ib++ )
    {
        if ( pb1[ ib ] == pb2[ ib ] )
        {
            continue;
        }

        wprintf( L"Byte at position %d is different. 0x%x != 0x%x.\n", ib, (INT)pb1[ ib ], (INT)pb2[ ib ] );
        if ( ibFirstMismatch == -1 )
        {
            ibFirstMismatch = ib;
        }
    }

    AssertRTL( ibFirstMismatch != -1 );
    AssertRTL( fFalse );
}

VOID ExtensiveKnownPageUnitTest( __out_bcount( cbSizeMax ) unsigned char * const pb, __in_range( g_cbPageMin, g_cbPageMax ) INT cbSizeMin, __in_range( g_cbPageMin, g_cbPageMax ) INT cbSizeMax )
{
    wprintf( L"ExtensiveKnownPageUnitTest starting.\n" );

    const ULONG pgno = 1234;
    unsigned char* const pbExpanded = (unsigned char*)PvOSMemoryPageAlloc( 3 * cbSizeMax, NULL );
    AssertRTL( pbExpanded != NULL );
    unsigned char* const pbData = pbExpanded + cbSizeMax;


    OSMemoryPageProtect( pbExpanded, cbSizeMax );
    OSMemoryPageProtect( pbExpanded + 2 * cbSizeMax, cbSizeMax );


    for ( INT cbSize = cbSizeMin; cbSize <= cbSizeMax; cbSize *= 2 )
    {

        for( INT ib = 0; ib < cbSize; ib++ )
        {
            pb[ib] = (unsigned char)ib;
        }


        SetPageChecksum( pb, cbSize, databasePage, pgno );
        memcpy( pbData, pb, cbSize );


        OSMemoryPageProtect( pb, g_cbPageMax );
        OSMemoryPageProtect( pbData + cbSize, cbSizeMax - cbSize );


        BOOL fFixErrors = fFalse;
        do
        {
            PAGECHECKSUM checksumStoredInHeader;
            PAGECHECKSUM checksumComputedOffData;
            PAGECHECKSUM checksumOriginal;
            BOOL fCorrectableError;
            INT ibitCorrupted;
            INT cFixed = 0;
            INT cUnfixed = 0;
            INT cFixable1 = 0;
            INT cFixable2 = 0;
            INT cFixableNone = 0;
            INT cbit = 0;

            wprintf( L"Testing page size %d with ECC correction %ws.\n", cbSize, fFixErrors ? L"ON" : L"OFF" );

            wprintf( L"Backup page must be intact.\n" );
            AssertPagesAreSame( pbData, pb, cbSize );

            wprintf( L"Original page must checksum correctly.\n" );
            OSMemoryPageProtect( pbData, cbSize );
            ChecksumAndPossiblyFixPage(
                        pbData,
                        cbSize,
                        databasePage,
                        pgno,
                        fFixErrors,
                        &checksumStoredInHeader,
                        &checksumComputedOffData,
                        &fCorrectableError,
                        &ibitCorrupted );
            AssertRTL( checksumComputedOffData == checksumStoredInHeader );
            AssertRTL( !fCorrectableError );
            AssertRTL( ibitCorrupted == -1 );
            AssertRTL( FGetBit( pbData, (INT)IbitNewChecksumFormatFlag( databasePage ) ) );
            AssertPagesAreSame( pbData, pb, cbSize );
            checksumOriginal = checksumComputedOffData;
            OSMemoryPageUnprotect( pbData, cbSize );

            wprintf( L"Going through relevant bits and performing single bit flips.\n" );
            for ( INT ibit = IbitFirstBitToTest(); ibit >= 0; ibit = IbitNextBitToTest( ibit, cbSize ) )
            {
                cbit++;


                const INT fBitBelongsToFirstChecksum = ( ibit / 8 ) < sizeof( XECHECKSUM );
                const INT fBitIsChecksumFormatFlag = ibit == (INT)IbitNewChecksumFormatFlag( databasePage );
                const INT fBitFixableSingleBitError = !( fBitBelongsToFirstChecksum || fBitIsChecksumFormatFlag );


                FlipBit( pbData, ibit );
                if ( !fFixErrors )
                {
                    OSMemoryPageProtect( pbData, cbSize );
                }
                ChecksumAndPossiblyFixPage(
                            pbData,
                            cbSize,
                            databasePage,
                            pgno,
                            fFixErrors,
                            &checksumStoredInHeader,
                            &checksumComputedOffData,
                            &fCorrectableError,
                            &ibitCorrupted );
                if ( !fFixErrors )
                {
                    OSMemoryPageUnprotect( pbData, cbSize );
                }

                if ( checksumComputedOffData == checksumStoredInHeader )
                {
                    AssertRTL( fFixErrors );
                    AssertRTL( fBitFixableSingleBitError );
                    AssertRTL( checksumComputedOffData == checksumOriginal );
                    AssertRTL( fCorrectableError );
                    AssertRTL( ibitCorrupted == ibit );
                    AssertPagesAreSame( pbData, pb, cbSize );
                    cFixed++;
                }
                else
                {
                    AssertRTL( !fFixErrors || !fBitFixableSingleBitError );
                    AssertRTL( fCorrectableError || !fBitFixableSingleBitError );
                    AssertRTL( ( ibitCorrupted == -1 ) || fCorrectableError );
                    AssertRTL( !fCorrectableError || ( ibitCorrupted == ibit ) );
                    AssertPagesAreNotSame( pbData, pb, cbSize );
                    FlipBit( pbData, ibit );
                    AssertPagesAreSame( pbData, pb, cbSize );
                    cUnfixed++;
                }
            }


            AssertRTL( ( cFixed + cUnfixed ) == cbit );
            if ( fFixErrors )
            {
                AssertRTL( cFixed > 0 );
                AssertRTL( cUnfixed > 0 );
            }
            else
            {
                AssertRTL( cFixed == 0 );
                AssertRTL( cUnfixed == cbit );
            }

            const INT cBlocks = cbSize / CbBlockSize( cbSize );
            AssertRTL( cbit == (INT)( 8 * ( CbChunkSizeToTestFirstBlock() + ( cBlocks - 1 ) * CbChunkSizeToTestRemainingBlocks() ) ) );
            AssertPagesAreSame( pbData, pb, cbSize );

            wprintf( L"Going through relevant bits and performing two bit flips.\n" );
            cFixed = 0;
            cUnfixed = 0;
            cFixable1 = 0;
            cFixable2 = 0;
            cFixableNone = 0;
            cbit = 0;
            for ( INT ibit1 = IbitFirstBitToTest(); ibit1 >= 0; ibit1 = IbitNextBitToTest( ibit1, cbSize ) )
            {

                for ( INT ibit2 = IbitNextBitToTest( ibit1, cbSize ); ibit2 >= 0; ibit2 = IbitNextBitToTest( ibit2, cbSize ) )
                {
                    cbit++;


                    const INT fBit1BelongsToFirstChecksum = ( ibit1 / 8 ) < sizeof( XECHECKSUM );
                    const INT fBit1IsChecksumFormatFlag = ibit1 == (INT)IbitNewChecksumFormatFlag( databasePage );
                    const INT fBit1FixableSingleBitError = !( fBit1BelongsToFirstChecksum || fBit1IsChecksumFormatFlag );
                    const INT fBit2BelongsToFirstChecksum = ( ibit2 / 8 ) < sizeof( XECHECKSUM );
                    const INT fBit2IsChecksumFormatFlag = ibit2 == (INT)IbitNewChecksumFormatFlag( databasePage );
                    const INT fBit2FixableSingleBitError = !( fBit2BelongsToFirstChecksum || fBit2IsChecksumFormatFlag );


                    FlipBit( pbData, ibit1 );
                    FlipBit( pbData, ibit2 );

                    if ( !fFixErrors )
                    {
                        OSMemoryPageProtect( pbData, cbSize );
                    }
                    ChecksumAndPossiblyFixPage(
                                pbData,
                                cbSize,
                                databasePage,
                                pgno,
                                fFixErrors,
                                &checksumStoredInHeader,
                                &checksumComputedOffData,
                                &fCorrectableError,
                                &ibitCorrupted );
                    if ( !fFixErrors )
                    {
                        OSMemoryPageUnprotect( pbData, cbSize );
                    }

                    if ( checksumComputedOffData == checksumStoredInHeader )
                    {
                        AssertRTL( !FIsSmallPage( cbSize ) );
                        AssertRTL( fFixErrors );
                        AssertRTL( fBit1FixableSingleBitError && fBit2FixableSingleBitError );
                        AssertRTL( checksumComputedOffData == checksumOriginal );
                        AssertRTL( fCorrectableError );
                        AssertRTL( ibitCorrupted == ibit1 );
                        AssertPagesAreSame( pbData, pb, cbSize );
                        cFixed++;
                    }
                    else
                    {
                        AssertPagesAreNotSame( pbData, pb, cbSize );

                        if ( ibitCorrupted == ibit1 )
                        {
                            AssertRTL( fCorrectableError );
                            AssertRTL( fBit1FixableSingleBitError );
                            AssertRTL( !FIsSmallPage( cbSize ) );
                            cFixable1++;
                        }
                        else if ( ibitCorrupted == ibit2 )
                        {
                            AssertRTL( fCorrectableError );
                            AssertRTL( !fBit1FixableSingleBitError );
                            AssertRTL( fBit2FixableSingleBitError );
                            cFixable2++;
                        }
                        else
                        {
                            AssertRTL( ibitCorrupted == -1 );
                            AssertRTL( !fCorrectableError );
                            cFixableNone++;
                        }

                        if ( !fFixErrors )
                        {
                            FlipBit( pbData, ibit1 );
                            FlipBit( pbData, ibit2 );
                        }
                        else
                        {
                            if ( ibitCorrupted == ibit1 )
                            {
                                FlipBit( pbData, ibit2 );
                            }
                            else if ( ibitCorrupted == ibit2 )
                            {
                                FlipBit( pbData, ibit1 );
                            }
                            else
                            {
                                FlipBit( pbData, ibit1 );
                                FlipBit( pbData, ibit2 );
                            }
                        }

                        cUnfixed++;
                    }

                    AssertPagesAreSame( pbData, pb, cbSize );
                }
            }


            AssertRTL( cFixableNone > 0 );
            AssertRTL( ( cFixed + cUnfixed ) == cbit );
            AssertRTL( ( cFixable1 + cFixable2 + cFixableNone ) == cUnfixed );
            if ( fFixErrors )
            {
                AssertRTL( ( cFixed > 0 ) || FIsSmallPage( cbSize ) );
                AssertRTL( cUnfixed > 0 );
            }
            else
            {
                AssertRTL( cFixed == 0 );
                AssertRTL( cUnfixed == cbit );
            }

            AssertPagesAreSame( pbData, pb, cbSize );

            fFixErrors = !fFixErrors;
        }
        while ( fFixErrors );

        OSMemoryPageUnprotect( pb, g_cbPageMax );
        OSMemoryPageUnprotect( pbData + cbSize, cbSizeMax - cbSize );

        wprintf( L"\n" );
    }

    OSMemoryPageUnprotect( pbExpanded, cbSizeMax );
    OSMemoryPageUnprotect( pbExpanded + 2 * cbSizeMax, cbSizeMax );

    wprintf( L"ExtensiveKnownPageUnitTest done.\n" );
}

ERR ErrChecksumUnitTest( INT cIterations )
{
    ERR err = JET_errSuccess;

    wprintf(L"\nProcessor Capabilities: %s%s%s%s\n",
        FSSEInstructionsAvailable() ? L"SSE " : L"",
        FSSE2InstructionsAvailable() ? L"SSE2 " : L"",
        FPopcntAvailable() ? L"POPCNT " : L"",
        FAVXEnabled() ? L"AVX " : L"");
    
    unsigned char* pb = ( unsigned char* )PvOSMemoryPageAlloc( g_cbPageMax, NULL );
    if( NULL == pb )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    BitRoutinesUnitTest( pb, 8192 );
    KnownPageUnitTest( pb, g_cbPageMax );
    ExtensiveKnownPageUnitTest( pb, g_cbPageMin, g_cbPageMax );

    srand( (UINT)HrtHRTCount() );
    for ( INT i = 0; i < cIterations; i++ )
    {
        SHORT* pw = (SHORT *)pb;
        for ( INT j = 0; j < g_cbPageMax / sizeof(SHORT); j++ )
        {
            pw[j] = (SHORT)rand();
            pw[j] |= (SHORT)( rand() << 15 );
        }

        XORChecksumUnitTest( pb );
        ECCChecksumUnitTest( pb );
        TestSetAndChecksum( pb );
        TestDehydratedPageChecksum( pb );
        TestECCChecksumSelection();
    }

    OSMemoryPageFree( pb );
    return err;
}

ERR ErrChecksumPerfTest( PFNCHECKSUMNEWFORMAT pfnChecksumMethod, UINT cbPageTest, UINT cbDataset )
{
    const INT cSamples = 1000000;
    Assert( cbDataset % cbPageTest == 0);

    wprintf( L"Computing %ld checksums for %ldk pages...\n", cSamples, cbPageTest / 1024);
    wprintf( L"Dataset size: %ldk\n", cbDataset / 1024 );
    unsigned char* pb = ( unsigned char* )PvOSMemoryPageAlloc( cbDataset, NULL );
    HRT* pSamples = ( HRT* )PvOSMemoryPageAlloc( cSamples * sizeof(QWORD), NULL );
    
    if( NULL == pb || NULL == pSamples )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    for( UINT ib = 0; ib < cbDataset; ++ib )
    {
        pb[ib] = (unsigned char)rand();
    }

    HRT hrtStart, hrtEnd;
    unsigned char* pbT = pb;
    unsigned cbT = cbPageTest;
    if ( !FIsSmallPage( cbPageTest ) )
    {
        cbT = cbPageTest / cxeChecksumPerPage;
        for ( INT i = 0; i < cSamples; i++ )
        {
            hrtStart = HrtHRTCount();
            pfnChecksumMethod( pbT + cbT * 1, cbT, 0, fFalse );
            pfnChecksumMethod( pbT + cbT * 2, cbT, 0, fFalse );
            pfnChecksumMethod( pbT + cbT * 3, cbT, 0, fFalse );
            pfnChecksumMethod( pbT, cbT, 0, fTrue );
            hrtEnd = HrtHRTCount();
            pSamples[i] = hrtEnd - hrtStart;
            pbT += (cbPageTest * 10);
            if ( pbT >= pb + cbDataset )
                pbT = pb;
        }
    }
    else
    {
        for ( INT i = 0; i < cSamples; i++ )
        {
            hrtStart = HrtHRTCount();
            pfnChecksumMethod( pbT, cbT, 0, fTrue );
            hrtEnd = HrtHRTCount();
            pSamples[i] = hrtEnd - hrtStart;
            pbT += (cbPageTest * 10);
            if ( pbT >= pb + cbDataset )
                pbT = pb;
        }
    }

    double variance = 0.0, avg = 0.0;
    HRT sample_min = QWORD(-1), sample_max = 0;
    for ( INT i = 0; i < cSamples; i++ )
    {
        sample_min = min( pSamples[i], sample_min );
        sample_max = max( pSamples[i], sample_max );
        avg += pSamples[i];
    }
    avg /= cSamples;
    for ( INT i = 0; i < cSamples; i++ )
        variance += ( pSamples[i] - avg ) * ( pSamples[i] - avg );
    variance /= cSamples;

    wprintf( L"HRT Freq: %I64d\n", HrtHRTFreq() );
    wprintf( L"Average checksum time (timer-cycles, micro-secs): %.1lf, %.1lf\n" , avg, avg * 1000.0 * 1000.0 / HrtHRTFreq() );
    wprintf( L"Min = %I64d, Max = %I64d\n", sample_min, sample_max );
    wprintf( L"Std. Dev. (timer-cycles) = %.1lf\n", sqrt( variance ) );

    OSMemoryPageFree( pb );
    OSMemoryPageFree( pSamples );
    return JET_errSuccess;
}

#define CHECKSUM_PERF_TEST( method, cbPage, cbDataset ) \
    wprintf( L"\nTesting perf for %hs\n", #method ); \
    ErrChecksumPerfTest( (method), (cbPage), (cbDataset) );



JETUNITTEST( CHECKSUM, FunctionalValidation )
{
    CallSx( ErrChecksumUnitTest( 5 ), JET_errOutOfMemory );
}

JETUNITTESTEX( CHECKSUM, ExtendedFunctionalValidation, JetSimpleUnitTest::dwDontRunByDefault )
{
    CallSx( ErrChecksumUnitTest( 100 * 1000 ), JET_errOutOfMemory );
}

JETUNITTESTEX( CHECKSUM, Perf, JetSimpleUnitTest::dwDontRunByDefault )
{
    CHECKSUM_PERF_TEST( ChecksumNewFormat64Bit, 32768, 100 * 1024 * 1024 );
    CHECKSUM_PERF_TEST( ChecksumNewFormatSSE, 32768, 100 * 1024 * 1024 );
    CHECKSUM_PERF_TEST( ChecksumNewFormatSSE2<ParityMaskFuncDefault>, 32768, 100 * 1024 * 1024 );

    if ( FPopcntAvailable() )
    {
        wprintf( L"\nPOPCNT supported !" );
        CHECKSUM_PERF_TEST( ChecksumNewFormatSSE2<ParityMaskFuncPopcnt>, 32768, 100 * 1024 * 1024 );
    }
    else
    {
        wprintf( L"\nPOPCNT not supported !\nChecksumNewFormatSSE2<ParityMaskFuncPopcnt> will not run.\n" );
    }

    if ( FAVXEnabled() )
    {
        wprintf( L"\nAVX supported !" );
        CHECKSUM_PERF_TEST( ChecksumNewFormatAVX, 32768, 100 * 1024 * 1024 );
    }
    else
    {
        wprintf( L"\nAVX not supported !\nChecksumNewFormatAVX will not run.\n" );
    }
}

