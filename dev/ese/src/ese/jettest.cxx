// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

#ifdef ENABLE_JET_UNIT_TEST

//  ****************************************************************
//  JetUnitTestFailure
//  ****************************************************************

//  ================================================================
JetUnitTestFailure::JetUnitTestFailure(
    const char * const szFile,
    const INT          line,
    const char * const szCondition ) :
//  ================================================================
        m_szFile( szFile ),
        m_line( line ),
        m_szCondition( szCondition )
{
}

//  ================================================================
JetUnitTestFailure::~JetUnitTestFailure()
//  ================================================================
{
}

//  ================================================================
const char * JetUnitTestFailure::SzFile() const
//  ================================================================
{
    return m_szFile;
}

//  ================================================================
const char * JetUnitTestFailure::SzCondition() const
//  ================================================================
{
    return m_szCondition;
}

//  ================================================================
INT JetUnitTestFailure::Line() const
//  ================================================================
{
    return m_line;
}

//  ****************************************************************
//  JetUnitTestResult
//  ****************************************************************

//  ================================================================
JetUnitTestResult::JetUnitTestResult( ) :
//  ================================================================
    m_szTest( NULL ),
    m_failures( 0 )
{
}

//  ================================================================
JetUnitTestResult::~JetUnitTestResult()
//  ================================================================
{
}

//  ================================================================
void JetUnitTestResult::Start( const char * const szTest )
//  ================================================================
{
    m_szTest = szTest;
    printf( "%s: ", m_szTest );
}

//  ================================================================
void JetUnitTestResult::Finish()
//  ================================================================
{
    printf( "%d failures\n", m_failures );
}

//  ================================================================
void JetUnitTestResult::AddFailure( const JetUnitTestFailure& failure )
//  ================================================================
{
    ++m_failures;
    printf( "\n\tFAILURE: test '%s', file %s, line %d: %s\n", m_szTest, failure.SzFile(), failure.Line(), failure.SzCondition() );
}

//  ================================================================
INT JetUnitTestResult::Failures() const
//  ================================================================
{
    return m_failures;
}

//  ****************************************************************
//  JetUnitTest
//  ****************************************************************

JetUnitTest * JetUnitTest::s_ptestHead = NULL;

//  ================================================================
bool FTestNameMatches( const char * const szTestName, const char * const szTestToRun )
//  ================================================================
{
    const INT cchTestToRun = strlen(szTestToRun);
    if( JetUnitTest::chWildCard == szTestToRun[cchTestToRun-1] )
    {
        // substring matching
        return ( 0 == _strnicmp( szTestName, szTestToRun, cchTestToRun-1 ) );
    }
    // exact matching
    return ( 0 == _stricmp( szTestName, szTestToRun ) );
}

//  ================================================================
void JetUnitTest::PrintTests()
//  ================================================================
{
    const JetUnitTest * ptest = s_ptestHead;

    printf( "Available tests are:\n" );
    while( ptest )
    {
        printf( "\t%s\n", ptest->m_szName );
        ptest = ptest->m_ptestNext;
    }
}

//  ================================================================
INT JetUnitTest::RunTests( const char * const szTest, const IFMP ifmpTest )
//  ================================================================
{
    bool fBFInitd = false;

    if ( szTest == NULL )
    {
        wprintf( L" Try running with '-?' to get help and a list of tests.\n" );
        return 2453453; //  pretend there are many failures...
    }

    if( 0 == _stricmp( szTest, "-?" ) ||
        0 == _stricmp( szTest, "/?" ) )
    {
        JetUnitTest::PrintTests();
        return 1;
    }

    bool fDefaultRun = true;
    const char szWildCard[] = { JetUnitTest::chWildCard, '\0' };
    if( 0 != _stricmp( szTest, szWildCard ) )
    {
        fDefaultRun = false;
    }

    (void)FNegTestSet( fLeakingUnflushedIos );

    bool fTestFound = false;
    INT failures = 0;
    INT cSkipped = 0;
    for( JetUnitTest * ptest = s_ptestHead ; ptest; ptest = ptest->m_ptestNext )
    {
        if( FTestNameMatches( ptest->m_szName, "FMP.NewAndWriteLatch" ) )
        {
            (void)FNegTestUnset( fLeakingUnflushedIos );
        }
        if( FTestNameMatches( ptest->m_szName, szTest ) )
        {
            //  Found a matching test ...
            //
            if( ptest->FRunByDefault() || !fDefaultRun )
            {
                // if a test database was specified, only run DB tests
                // else only run non-DB tests
                // i.e. force the two kinds of runs to be mutually exculsive

                if ( ifmpTest == ifmpNil || ifmpTest == JET_dbidNil )
                {
                    if ( ptest->FNeedsDB() )
                    {
                        // Don't count as a failure, skip this test, unittest didn't qualify ...
                        //JetUnitTestFailure failure( __FILE__, __LINE__, "Critical failure due to client not passing Test database to operate on." );
                        //result.AddFailure( failure );
                        cSkipped++;
                        continue;
                    }
                }
                else
                {
                    if ( ptest->FNeedsDB() )
                    {
                        Assert( ifmpNil != ifmpTest );
                        Assert( JET_dbidNil != ifmpTest );
                        ptest->SetTestIfmp( ifmpTest );
                    }
                    else
                    {
                        // Don't count as a failure, skip this test, unittest didn't qualify ...
                        //JetUnitTestFailure failure( __FILE__, __LINE__, "Critical failure due to client not passing Test database to operate on." );
                        //result.AddFailure( failure );
                        cSkipped++;
                        continue;
                    }
                }

                fTestFound = true;

                //  Get unit test results

                JetUnitTestResult result;
                result.Start( ptest->m_szName );

                //  Init Buffer Manager if necessary

                if ( ptest->FNeedsBF() != fBFInitd )
                {
                    //  We use an oscillating BF init state to avoid re-initing BF over and 
                    //  over again.
                    if ( ptest->FNeedsBF() )
                    {
                        //  We can use the max page size, because the buffer manager now handles multiple page
                        //  sizes.
                        if ( ErrBFInit( g_cbPageMax ) < JET_errSuccess )
                        {
                            JetUnitTestFailure failure( __FILE__, __LINE__, "Critical failure due to BF init failure" );
                            result.AddFailure( failure );
                            continue;
                        }
                        fBFInitd = true;
                    }
                    else
                    {
                        Assert( fBFInitd );
                        JetConfigureProcessForCrashDump( JET_bitDumpUnitTest                    |
                                                         JET_bitDumpCacheIncludeDirtyPages      |
                                                         JET_bitDumpCacheIncludeCachedPages     |
                                                         JET_bitDumpCacheIncludeCorruptedPages  |
                                                         JET_bitDumpMaximum );
                        BFTerm();
                        fBFInitd = false;
                    }
                }

                //  Run the actual test
                //

                ptest->Run( &result );

                //  Complete the test results

                result.Finish();
                failures += result.Failures();
            }
        }
        if( FTestNameMatches( ptest->m_szName, "FMP.NewAndWriteLatch" ) )
        {
            (void)FNegTestSet( fLeakingUnflushedIos );
        }
    }

    if( !fTestFound )
    {
        printf( "No test matched '%s'\n", szTest );
        PrintTests();
        failures = 1;
    }

    if( fBFInitd )
    {
        BFTerm();
        fBFInitd = false;
    }

    (void)FNegTestUnset( fLeakingUnflushedIos );

    printf( "\nTests skipped: %ld\n", cSkipped );
    return failures;
}



//  ================================================================
JetUnitTest::JetUnitTest( const char * const szName ) :
//  ================================================================
    m_ptestNext( NULL ),
    m_szName( szName )
{
    // Append the test to the list so they are run in the order
    // they appear in the compilation unit
    JetUnitTest ** pptest = &s_ptestHead;
    while( *pptest )
    {
        JetUnitTest * ptestNext = (*pptest);
        pptest = &(ptestNext->m_ptestNext);
    }
    *pptest = this;
}

//  ================================================================
JetUnitTest::~JetUnitTest()
//  ================================================================
{
}

//  ================================================================
bool JetUnitTest::FNeedsBF()
//  ================================================================
{
    return fFalse;
}

//  ================================================================
bool JetUnitTest::FNeedsDB()
//  ================================================================
{
    return fFalse;
}

//  ================================================================
bool JetUnitTest::FRunByDefault()
//  ================================================================
{
    return fTrue;
}

//  ================================================================
void JetUnitTest::SetTestIfmp( const IFMP ifmpTest )
//  ================================================================
{
    ExpectedSz( fFalse, "Setting Test Database / IFMP on test that does not require Test Database" );
}


//  ****************************************************************
//  JetSimpleUnitTest
//  ****************************************************************

//  ================================================================
JetSimpleUnitTest::JetSimpleUnitTest( const char * const szName ) :
//  ================================================================
    JetUnitTest( szName ),
    m_dwFacilities( 0 ),
    m_presult( NULL )
{
}

//  ================================================================
JetSimpleUnitTest::JetSimpleUnitTest( const char * const szName, const DWORD dwFacilities ) :
//  ================================================================
    JetUnitTest( szName ),
    m_dwFacilities( dwFacilities ),
    m_presult( NULL )
{
}


//  ================================================================
JetSimpleUnitTest::~JetSimpleUnitTest()
//  ================================================================
{
}

//  ================================================================
void JetSimpleUnitTest::Run( JetUnitTestResult * const presult )
//  ================================================================
{
    m_presult = presult;
    Run_();
}

//  ================================================================
void JetSimpleUnitTest::Fail_( const char * const szFile, const INT line, const char * const szCondition )
//  ================================================================
{
    JetUnitTestFailure failure( szFile, line, szCondition );
    m_presult->AddFailure( failure );
#ifdef DEBUG
    AssertSz( fFalse, "Unit tests should not fail (you know, generally speaking)" );
#else // !DEBUG
#ifdef RTM
    EnforceSz( fFalse, "UnitTestFailure" );
#else // !RTM
    AssertSzRTL( fFalse, "Unit tests should not fail (you know, generally speaking)" );
#endif // RTM
#endif // DEBUG
}


//  ****************************************************************
//  JetSimpleDbUnitTest
//  ****************************************************************

//  ================================================================
JetSimpleDbUnitTest::JetSimpleDbUnitTest( const char * const szName, const DWORD dwFacilities ) :
//  ================================================================
    JetSimpleUnitTest( szName, dwFacilities ),
    m_dwFacilities( dwFacilities ),
    m_presult( NULL )
{
}


//  ================================================================
JetSimpleDbUnitTest::~JetSimpleDbUnitTest()
//  ================================================================
{
}

//  ================================================================
void JetSimpleDbUnitTest::Run( JetUnitTestResult * const presult )
//  ================================================================
{
    m_presult = presult;
    Run_();
}

//  ================================================================
void JetSimpleDbUnitTest::Fail_( const char * const szFile, const INT line, const char * const szCondition )
//  ================================================================
{
    JetUnitTestFailure failure( szFile, line, szCondition );
    m_presult->AddFailure( failure );
}

//  ================================================================
void JetSimpleDbUnitTest::SetTestIfmp( const IFMP ifmpTest )
//  ================================================================
{
    m_ifmp = ifmpTest;
}

//  ================================================================
IFMP JetSimpleDbUnitTest::IfmpTest()
//  ================================================================
{
    return m_ifmp;
}


//  ****************************************************************
//  JetTestFixture
//  ****************************************************************

//  ================================================================
JetTestFixture::JetTestFixture() :
//  ================================================================
    m_presult(NULL)
{
}

//  ================================================================
JetTestFixture::~JetTestFixture()
//  ================================================================
{
}

//  ================================================================
bool JetTestFixture::SetUp( JetUnitTestResult * const presult )
//  ================================================================
{
    m_presult = presult;
    return SetUp_();
}

//  ================================================================
void JetTestFixture::TearDown()
//  ================================================================
{
    TearDown_();
}

//  ================================================================
void JetTestFixture::Fail_( const char * const szFile, const INT line, const char * const szCondition )
//  ================================================================
{
    JetUnitTestFailure failure( szFile, line, szCondition );
    m_presult->AddFailure( failure );
}

#endif // ENABLE_JET_UNIT_TEST

