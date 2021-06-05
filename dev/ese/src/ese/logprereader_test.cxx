// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // ENABLE_JET_UNIT_TEST


//  ================================================================
//  LogPrereader test class and tests
//  ================================================================

//  Implementation of LogPrereaderTest.

ERR LogPrereaderTest::ErrLGPIPrereadPage( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf )
{
    //  For the test class, we will pretend that every page that is a multiple of 3 (dbid 0) or 5 (dbid 1) causes
    //  the IO to split, unless we pass bfprfCombinableOnly in.
    //  Also, page 13 will return JET_errOutOfMemory and page 17 will return errBFPageCached.

    Assert( FLGPDBEnabled( dbid ) );

    Expected( dbid == 0 || dbid == 1 );

    const PGNO pgnoMod = ( dbid == 0 ) ? 3 : 5;

    if ( pgno == 13 )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }
    else if ( pgno == 17 )
    {
        return ErrERRCheck( errBFPageCached );
    }
    else if ( ( pgno % pgnoMod ) == 0 && ( bfprf & bfprfCombinableOnly ) )
    {
        return ErrERRCheck( errDiskTilt );
    }
    else
    {
        //  Binary search won't work here because we're nulling pages out.
        const size_t ipg = IpgLGPIGetUnsorted( dbid, pgno );
        Assert( ipg != CArray<PGNO>::iEntryNotFound );

        //  Null it out so that the test can verify that it was successfully preread.
        const ERR errSetEntry = ErrLGPISetEntry( dbid, ipg, pgnoNull );
        Assert( errSetEntry == JET_errSuccess );

        return JET_errSuccess;
    }
}

ERR LogPrereaderTest::ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid )
{
    return JET_errSuccess;
}

LogPrereaderTest::LogPrereaderTest() :
    LogPrereaderBase()
{
}

size_t LogPrereaderTest::CpgLGPGetArrayPgnosSize( const DBID dbid ) const
{
    return CpgLGPIGetArrayPgnosSize( dbid );
}

VOID LogPrereaderTest::LGPAddRgPgnoRef( const DBID dbid, const PGNO* const rgpgno, const CPG cpg )
{
    for ( CPG ipg = 0; ipg < cpg; ipg++ )
    {
        CallS( ErrLGPAddPgnoRef( dbid, rgpgno[ ipg ] ) );
    }
}

BOOL LogPrereaderTest::FLGPTestPgnosPresent( const DBID dbid, const PGNO* const rgpgno, const CPG cpg )
{
    BOOL fAllMatch = ( cpg == (CPG)CpgLGPIGetArrayPgnosSize( dbid ) );

    Assert( FLGPDBEnabled( dbid ) );
    
    for ( CPG ipg = 0; ipg < cpg && fAllMatch; ipg++ )
    {
        fAllMatch = fAllMatch && ( rgpgno[ ipg ] == PgnoLGPIGetEntry( dbid, ipg ) );
    }

    return fAllMatch;
}


// test that the initial state is consistent
JETUNITTEST( LogPrereader, InitialStateConsistent )
{
    LogPrereaderTest logprereader;

    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test term without init
JETUNITTEST( LogPrereader, TermNoInit )
{
    LogPrereaderTest logprereader;

    logprereader.LGPTerm();
    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test init/term
JETUNITTEST( LogPrereader, InitTerm )
{
    LogPrereaderTest logprereader;

    logprereader.LGPInit( 2, 5 );
    CHECK( logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
    
    logprereader.LGPTerm();
    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test disable without enable
JETUNITTEST( LogPrereader, DisableNoEnable )
{
    LogPrereaderTest logprereader;

    logprereader.LGPInit( 2, 5 );
    
    logprereader.LGPDBDisable( 0 );
    CHECK( logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
    
    logprereader.LGPTerm();
    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test enable without disable
JETUNITTEST( LogPrereader, EnableNoDisable )
{
    LogPrereaderTest logprereader;

    logprereader.LGPInit( 2, 5 );
    
    logprereader.LGPDBEnable( 1 );
    CHECK( logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( logprereader.FLGPDBEnabled( 1 ) );
    
    logprereader.LGPTerm();
    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test enable/disable
JETUNITTEST( LogPrereader, EnableDisable )
{
    LogPrereaderTest logprereader;

    logprereader.LGPInit( 2, 5 );
    
    logprereader.LGPDBEnable( 0 );
    CHECK( logprereader.FLGPEnabled() );
    CHECK( logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
    
    logprereader.LGPDBDisable( 0 );
    CHECK( logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
    
    logprereader.LGPTerm();
    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test oprations on disabled dbid
JETUNITTEST( LogPrereader, OpsOnDisabledDb )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;

    logprereader.LGPInit( 2, 1 );
    
    logprereader.LGPDBEnable( 0 );
    
    CHECK( logprereader.ErrLGPAddPgnoRef( 0, 10 ) == JET_errSuccess );
    CHECK( logprereader.ErrLGPAddPgnoRef( 1, 10 ) == JET_errSuccess );
    CHECK( logprereader.ErrLGPAddPgnoRef( 0, 20 ) == JET_errSuccess );
    CHECK( logprereader.ErrLGPAddPgnoRef( 1, 20 ) == JET_errSuccess );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 2 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );
    
    logprereader.LGPSortPages();
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 2 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 10, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 2 );
    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 10, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 0 );

    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 2 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    CHECK( logprereader.FLGPEnabled() );
    CHECK( logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );

    logprereader.LGPDBDisable( 0 );
    CHECK( logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 0 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    logprereader.LGPTerm();
    CHECK( !logprereader.FLGPEnabled() );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( !logprereader.FLGPDBEnabled( 1 ) );
}

// test enabling twice
JETUNITTEST( LogPrereader, EnablingTwice )
{
    LogPrereaderTest logprereader;

    logprereader.LGPInit( 1, 1 );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    
    logprereader.LGPDBEnable( 0 );
    CHECK( logprereader.FLGPDBEnabled( 0 ) );
    CHECK( logprereader.ErrLGPAddPgnoRef( 0, 10 ) == JET_errSuccess );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 1 );

    CHECK( logprereader.FLGPDBEnabled( 0 ) );
    logprereader.LGPDBEnable( 0 );
    CHECK( logprereader.FLGPDBEnabled( 0 ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 0 );

    logprereader.LGPDBDisable( 0 );
    CHECK( !logprereader.FLGPDBEnabled( 0 ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 0 );
    
    logprereader.LGPTerm();
}

// test no pages
JETUNITTEST( LogPrereader, NoPages )
{
    LogPrereaderTest logprereader;

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    CHECK( logprereader.FLGPTestPgnosPresent( 0, NULL, 0 ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, NULL, 0 ) );

    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test single-page success
JETUNITTEST( LogPrereader, SinglePageSuccess )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = { 10 };
    const PGNO rgpgnoSorted[]   = { 10 };
    const PGNO rgpgnoIssued[]   = { xx };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 10, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 1 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test single-page cached
JETUNITTEST( LogPrereader, SinglePageCached )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;

    const PGNO rgpgnoUnsorted[] = { 17 };
    const PGNO rgpgnoSorted[]   = { 17 };
    const PGNO rgpgnoIssued[]   = { 17 };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 17, &cpgRead ) == errBFPageCached );
    CHECK( cpgRead == 0 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test single-page out-of-memory
JETUNITTEST( LogPrereader, SinglePageOutOfMemory )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;

    const PGNO rgpgnoUnsorted[] = { 13 };
    const PGNO rgpgnoSorted[]   = { 13 };
    const PGNO rgpgnoIssued[]   = { 13 };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 13, &cpgRead ) == JET_errOutOfMemory );
    CHECK( cpgRead == 0 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );

    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test single-page success almost tilt
JETUNITTEST( LogPrereader, SinglePageSuccessAlmostTilt )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = { 3 };
    const PGNO rgpgnoSorted[]   = { 3 };
    const PGNO rgpgnoIssued[]   = { xx };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 3, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 1 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test multi-page tilt at both ends, starting at the middle, with pgnoNull pages
JETUNITTEST( LogPrereader, MultiPageTiltBothStartMiddleWithPgnoNull )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = { 4, xx,  6, xx,  3,  5 };
    const PGNO rgpgnoSorted[]   = { 3,  4,  5,  6 };
    const PGNO rgpgnoIssued[]   = { 3, xx, xx,  6 };
    const CPG cpgUnsorted = _countof( rgpgnoUnsorted );
    const CPG cpgSorted = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpgUnsorted );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpgSorted ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 4, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 2 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpgSorted ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test multi-page tilt at both ends, starting at the left
JETUNITTEST( LogPrereader, MultiPageTiltBothStartLeft )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = {  4,  6,  3,  5 };
    const PGNO rgpgnoSorted[]   = {  3,  4,  5,  6 };
    const PGNO rgpgnoIssued[]   = { xx, xx, xx,  6 };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 3, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 3 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}


// test multi-page tilt at both ends, starting at the right
JETUNITTEST( LogPrereader, MultiPageTiltBothStartRight )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = { 4,  6,  3,  5 };
    const PGNO rgpgnoSorted[]   = { 3,  4,  5,  6 };
    const PGNO rgpgnoIssued[]   = { 3, xx, xx, xx };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );
    logprereader.LGPDBEnable( 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 6, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 3 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test multi-page tilt at both ends, starting at the middle of an almost-tilted page
JETUNITTEST( LogPrereader, MultiPageTiltBothStartAlmostTilt )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted0[]    = { 66, 71, 65, 65, 64, 69, 62, 67, 68, 65, 70, 63, 71, 66, 69, 69, 69, 65 };
    const PGNO rgpgnoSorted0[]      = { 62, 63, 64, 65, 66, 67, 68, 69, 70, 71 };
    const PGNO rgpgnoIssued0[]      = { 62, 63, xx, xx, xx, xx, xx, 69, 70, 71 };
    const CPG cpgUnsorted0  = _countof( rgpgnoUnsorted0 );
    const CPG cpgSorted0    = _countof( rgpgnoSorted0 );

    const PGNO rgpgnoUnsorted1[]    = { 37, 39, 34, 33, 32, 35, 36, 30, 38, 31, 40 };
    const PGNO rgpgnoSorted1[]      = { 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40 };
    const PGNO rgpgnoIssued1[]      = { 30, xx, xx, xx, xx, xx, xx, xx, xx, xx, 40 };
    const CPG cpgUnsorted1  = _countof( rgpgnoUnsorted1 );
    const CPG cpgSorted1    = _countof( rgpgnoSorted1 );

    logprereader.LGPInit( 2, 2 );
    logprereader.LGPDBEnable( 0 );
    logprereader.LGPDBEnable( 1 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted0, cpgUnsorted0 );
    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted1, cpgUnsorted1 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted0, cpgUnsorted0 ) );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted1, cpgUnsorted1 ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted0, cpgSorted0 ) );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted1, cpgSorted1 ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 66, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 5 );
    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 35, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 9 );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued0, cpgSorted0 ) );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued1, cpgSorted1 ) );

    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == cpgSorted0 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == cpgSorted1 );

    logprereader.LGPDBDisable( 0 );
    logprereader.LGPDBDisable( 1 );
    logprereader.LGPTerm();
}

// test multi-page errBFPageCached at the right, and JET_errOutOfMemory at the left.
JETUNITTEST( LogPrereader, MultiPageCachedAndOutOfMemory )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = { 10, 15, 17, 18, 16, 14, 13 };
    const PGNO rgpgnoSorted[]   = { 10, 13, 14, 15, 16, 17, 18 };
    const PGNO rgpgnoIssued[]   = { 10, 13, xx, xx, xx, 17, 18 };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 1, 1 );

    //  Starting in the middle.
    logprereader.LGPDBEnable( 0 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 15, &cpgRead ) == JET_errOutOfMemory );
    CHECK( cpgRead == 3 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued, cpg ) );

    //  Starting at the left.
    logprereader.LGPDBEnable( 0 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    const PGNO rgpgnoIssued2[]  = { 10, 13, xx, 15, 16, 17, 18 };
    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 14, &cpgRead ) == JET_errOutOfMemory );
    CHECK( cpgRead == 1 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued2, cpg ) );

    //  Starting at the right.
    logprereader.LGPDBEnable( 0 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 0, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoSorted, cpg ) );

    const PGNO rgpgnoIssued3[]  = { 10, 13, 14, 15, xx, 17, 18 };
    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 0, 16, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 1 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 0 ) == cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 0, rgpgnoIssued3, cpg ) );
    
    logprereader.LGPDBDisable( 0 );
    logprereader.LGPTerm();
}

// test multi-page errBFPageCached first
JETUNITTEST( LogPrereader, MultiPageCachedFirst )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;

    const PGNO rgpgnoUnsorted[] = { 15, 17, 16, 14, 13, 22 };
    const PGNO rgpgnoSorted[]   = { 13, 14, 15, 16, 17, 22 };
    const PGNO rgpgnoIssued[]   = { 13, 14, 15, 16, 17, 22 };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 2, 1 );
    logprereader.LGPDBEnable( 1 );

    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 17, &cpgRead ) == errBFPageCached );
    CHECK( cpgRead == 0 );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 1 );
    logprereader.LGPTerm();
}

// test multi-page JET_errOutOfMemory first.
JETUNITTEST( LogPrereader, MultiPageOutOfMemoryFirst )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;

    const PGNO rgpgnoUnsorted[] = { 15, 17, 16, 14, 13, 22 };
    const PGNO rgpgnoSorted[]   = { 13, 14, 15, 16, 17, 22 };
    const PGNO rgpgnoIssued[]   = { 13, 14, 15, 16, 17, 22 };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 2, 1 );
    logprereader.LGPDBEnable( 1 );

    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 13, &cpgRead ) == JET_errOutOfMemory );
    CHECK( cpgRead == 0 );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued, cpg ) );
    
    logprereader.LGPDBDisable( 1 );
    logprereader.LGPTerm();
}

// test multi-page success.
JETUNITTEST( LogPrereader, MultiPageSuccess )
{
    LogPrereaderTest logprereader;
    CPG cpgRead;
    const PGNO xx = pgnoNull;

    const PGNO rgpgnoUnsorted[] = { 34, 33, 32, 31 };
    const PGNO rgpgnoSorted[]   = { 31, 32, 33, 34 };
    const PGNO rgpgnoIssued[]   = { xx, xx, xx, xx };
    const CPG cpg = _countof( rgpgnoSorted );

    logprereader.LGPInit( 2, 1 );

    //  starting at the middle-left.
    logprereader.LGPDBEnable( 1 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 32, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 4 );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued, cpg ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == cpg );

    //  starting at the middle-right.
    logprereader.LGPDBEnable( 1 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 33, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 4 );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued, cpg ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == cpg );

    //  starting at the left.
    logprereader.LGPDBEnable( 1 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 31, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 4 );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued, cpg ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == cpg );

    //  starting at the right.
    logprereader.LGPDBEnable( 1 );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == 0 );

    logprereader.LGPAddRgPgnoRef( 1, rgpgnoUnsorted, cpg );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoUnsorted, cpg ) );

    logprereader.LGPSortPages();
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoSorted, cpg ) );

    CHECK( logprereader.ErrLGPPrereadExtendedPageRange( 1, 34, &cpgRead ) == JET_errSuccess );
    CHECK( cpgRead == 4 );
    CHECK( logprereader.FLGPTestPgnosPresent( 1, rgpgnoIssued, cpg ) );
    CHECK( logprereader.CpgLGPGetArrayPgnosSize( 1 ) == cpg );
    
    logprereader.LGPDBDisable( 1 );
    logprereader.LGPTerm();
}

