// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

//  ================================================================
class StatBasicTest : public UNITTEST
//  ================================================================
{
    private:
        static StatBasicTest s_instance;

    protected:
        StatBasicTest() {}
    public:
        ~StatBasicTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

StatBasicTest StatBasicTest::s_instance;

const char * StatBasicTest::SzName() const      { return "Stat Basic Test"; };
const char * StatBasicTest::SzDescription() const   { return 
            "Tests the statistic libraries' basic facilities (min/max, perfect histo) - this is the original test from ESE code.";
        }
bool StatBasicTest::FRunUnderESE98() const      { return true; }
bool StatBasicTest::FRunUnderESENT() const      { return true; }
bool StatBasicTest::FRunUnderESE97() const      { return true; }



//  ================================================================
ERR StatBasicTest::ErrTest()
//  ================================================================
{
    wprintf( L"\tTesting basic stats support ...\n");

    #define Validate( expr )      if ( !(expr) )                      \
                                      {                               \
                                      wprintf(L" [iTest=%d,line=%d] Failure checking: %S\n", iTest, __LINE__, #expr );    \
                                      fResult = fFalse;               \
                                      if ( fBailOnError ) goto HandleError;   \
                                      STATAssertSz( fFalse, "HaltTest" );     \
                                      }

    #define Validate2( expr, val )  if ( !(expr) )                      \
                                        {                               \
                                        wprintf(L" [iTest=%d,line=%d] Failure checking (val=%I64d): %S\n", iTest, __LINE__, val , #expr);   \
                                        fResult = fFalse;               \
                                        if ( fBailOnError ) goto HandleError;   \
                                        STATAssertSz( fFalse, "HaltTest" );     \
                                    }

    #define CallTest( func )        if ( CStats::ERR::errSuccess != ( err = (func) ) )      \
                                        {                               \
                                        wprintf(L" [iTest=%d,line=%d] Call( %S ) failed w/ %d\n", iTest, __LINE__, #func, err );    \
                                        fResult = fFalse;               \
                                        if ( fBailOnError ) goto HandleError;   \
                                        STATAssertSz( fFalse, "HaltTest" );     \
                                    }

    BOOL fPrintStatus = fTrue;
    CStats::ERR err = CStats::ERR::errSuccess;
    ULONG iTest = 0;
    CStats::ERR csErr = CStats::ERR::errSuccess;
    BOOL fResult = fTrue; // assumed innocent, until proven guilty.
    ULONG fBailOnError = fFalse;

    SAMPLE qwSmall      = 10;
    SAMPLE qwMiddleMinus    = 0x7FFFFFFFFFFFFFF1;
    SAMPLE qwMiddlePlus = 0x8000000000000002;
    SAMPLE qwBig        = 0xFFFFFFFFFFFFFFF5;

    if ( fPrintStatus )
        wprintf(L" Beginning Stat library unit test ...\n");


    if ( fPrintStatus )
        wprintf(L" Testing CPerfectHistogramStats...\n");

    CPerfectHistogramStats * pPHS = new CPerfectHistogramStats();
    if ( pPHS == NULL )
    {
        if ( fPrintStatus )
            wprintf(L"Test %d failed to alloc %d bytes\n", iTest, sizeof(CPerfectHistogramStats));
        fResult = fFalse;
        goto HandleError;
    }

    pPHS->AssertValid();    // we can assert valid outside the lock b/c we know we're single threaded here.

    //  Test empty class

    iTest = 1;

    CallTest( ErrTestZerodHisto( pPHS ) );

    //  Test basics ...

    iTest = 2;
    CallTest( pPHS->ErrAddSample( qwSmall ) );
    Validate( pPHS->C() == 1 );
    Validate( pPHS->Min() == qwSmall );
    Validate( pPHS->Ave() == qwSmall );
    Validate( pPHS->Max() == qwSmall );
    Validate( pPHS->Total() == qwSmall );
    pPHS->AssertValid();
    CallTest( pPHS->ErrAddSample( qwMiddleMinus ) );
    Validate( pPHS->C() == 2 );
    Validate( pPHS->Min() == qwSmall );
    Validate( pPHS->Ave() == ( ( qwSmall + qwMiddleMinus ) / 2 ) );
    Validate2( pPHS->Max() == qwMiddleMinus, pPHS->Max() );
    Validate( pPHS->Total() == ( qwSmall + qwMiddleMinus ) );
    CallTest( pPHS->ErrAddSample( qwMiddlePlus ) );
    pPHS->AssertValid();
    Validate( pPHS->C() == 3 );
    Validate( pPHS->Min() == qwSmall );
    Validate( pPHS->Ave() == ( ( qwSmall + qwMiddleMinus + qwMiddlePlus ) / 3 ) );
    Validate( pPHS->Max() == qwMiddlePlus );
    Validate( pPHS->Total() == ( qwSmall + qwMiddleMinus + qwMiddlePlus ) );
    CallTest( pPHS->ErrAddSample( qwBig ) );
    Validate( pPHS->C() == 4 );
    //  Total and Ave likely went wacky on this case.
    Validate( pPHS->Min() == qwSmall );
    Validate( pPHS->Max() == qwBig );
    pPHS->AssertValid();

    iTest = 3;
    pPHS->Zero();
    CallTest( pPHS->ErrAddSample( qwMiddlePlus ) );
    Validate( pPHS->C() == 1 );
    Validate( pPHS->Min() == qwMiddlePlus );
    Validate( pPHS->Ave() == qwMiddlePlus );
    Validate( pPHS->Max() == qwMiddlePlus );
    Validate( pPHS->Total() == qwMiddlePlus );
    CallTest( pPHS->ErrAddSample( qwMiddleMinus ) );
    Validate( pPHS->C() == 2 );
    Validate( pPHS->Min() == qwMiddleMinus );
    Validate( pPHS->Ave() == ( ( qwMiddlePlus + qwMiddleMinus ) / 2 ) );
    Validate2( pPHS->Max() == qwMiddlePlus, pPHS->Max() );
    Validate( pPHS->Total() == ( qwMiddlePlus + qwMiddleMinus ) );
    CallTest( pPHS->ErrAddSample( qwBig ) );
    Validate( pPHS->C() == 3 );
    //  Total and Ave likely went wacky on this case.
    Validate( pPHS->Min() == qwMiddleMinus );
    Validate( pPHS->Max() == qwBig );
    CallTest( pPHS->ErrAddSample( qwSmall ) );
    Validate( pPHS->C() == 4 );
    Validate( pPHS->Min() == qwSmall );
    Validate( pPHS->Max() == qwBig );
    pPHS->AssertValid();

    iTest = 4;
    CallTest( pPHS->ErrAddSample( qwMiddleMinus ) );
    CallTest( pPHS->ErrAddSample( qwMiddlePlus ) );
    Validate( pPHS->C() == 6 );
    pPHS->AssertValid();

    iTest = 5;
    CHITS cSamples;
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrGetSampleHits( qwSmall, &cSamples ) );
    Validate( cSamples == 1 );
    CallTest( pPHS->ErrGetSampleHits( qwMiddleMinus, &cSamples ) );
    Validate( cSamples == 2 );
    CallTest( pPHS->ErrGetSampleHits( qwMiddlePlus, &cSamples ) );
    Validate( cSamples == 2 );
    CallTest( pPHS->ErrGetSampleHits( qwBig, &cSamples ) );
    Validate( cSamples == 1 );
    pPHS->AssertValid();

    iTest = 6;
    CallTest( pPHS->ErrReset() );
    SAMPLE rgSamples[5] = { 42, 42, 42, 42, 42 };
    CHITS rgcSamples[5] = { 42, 42, 42, 42, 42 };
    for ( ULONG iS = 0; iS < sizeof(rgSamples)/sizeof(rgSamples[0]); iS++ )
    {
        csErr = pPHS->ErrGetSampleValues( &(rgSamples[iS]) );
        Validate( csErr == CStats::ERR::errSuccess || iS == 4 );
        if ( csErr != CStats::ERR::errSuccess ) break;
        csErr = pPHS->ErrGetSampleHits( rgSamples[iS], &(rgcSamples[iS]) );
        Validate( csErr == CStats::ERR::errSuccess );
    }
    Validate( rgSamples[0] == qwSmall && rgcSamples[0] == 1 );
    Validate( rgSamples[1] == qwMiddleMinus && rgcSamples[1] == 2 );
    Validate( rgSamples[2] == qwMiddlePlus && rgcSamples[2] == 2 );
    Validate( rgSamples[3] == qwBig && rgcSamples[3] == 1 );
    Validate( rgSamples[4] == 42 );
    pPHS->AssertValid();

    iTest = 7;
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrGetSampleHits( qwMiddleMinus, &cSamples ) );
    Validate( cSamples == 3 );
    CallTest( pPHS->ErrGetSampleHits( qwBig, &cSamples ) );
    Validate( cSamples == 3 );
    pPHS->AssertValid();

    iTest = 8;
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrGetSampleHits( 0x0, &cSamples ) );
    Validate( cSamples == 0 );
    pPHS->AssertValid();

    CallTest( pPHS->ErrReset() );
    pPHS->AssertValid();

    iTest = 9;
    if ( fPrintStatus )
        PrintStats( pPHS );
    if ( fPrintStatus )
        PrintStats( pPHS, 0x2000000000000000 );

    iTest = 10;
    pPHS->Zero();
    CallTest( pPHS->ErrAddSample( 0x0 ) );
    SAMPLE qwSample = 42;
    CallTest( pPHS->ErrGetSampleValues( &qwSample ) );
    Validate( 0x0 == qwSample );
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrGetSampleHits( 0x0, &cSamples ) );
    Validate( 1 == cSamples );
    CallTest( pPHS->ErrReset() );

    CallTest( pPHS->ErrAddSample( 1 ) );
    CallTest( pPHS->ErrGetSampleValues( &qwSample ) );
    Validate( 0x0 == qwSample );
    CallTest( pPHS->ErrGetSampleValues( &qwSample ) );
    Validate( 1 == qwSample );
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrGetSampleHits( 1, &cSamples ) );
    Validate( 2 == cSamples );
    cSamples = 42; // dirty value
    csErr = pPHS->ErrGetSampleHits( 2, &cSamples );
    Validate( csErr == CStats::ERR::wrnOutOfSamples );
    Validate( 0 == cSamples );
    CallTest( pPHS->ErrReset() );

    iTest = 11;
    pPHS->Zero();
    ULONG ulPercentile = 50;
    csErr = pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample );
    Validate( csErr == CStats::ERR::wrnOutOfSamples );
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrAddSample( 0x2A ) );
    ulPercentile = 0;
    qwSample = 42; // dirty value
    csErr = pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample );
    Validate( csErr == CStats::ERR::errInvalidParameter );
    ulPercentile = 1;
    CallTest( pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample ) );
    Validate( 100 == ulPercentile );
    Validate( 0x2A == qwSample );
    ulPercentile = 50;
    CallTest( pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample ) );
    Validate( 100 == ulPercentile );
    Validate( 0x2A == qwSample );
    ulPercentile = 100;
    CallTest( pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample ) );
    Validate( 100 == ulPercentile );
    Validate( 0x2A == qwSample );
    ulPercentile = 101;
    csErr = pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample );
    Validate( csErr == CStats::ERR::errInvalidParameter );
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrAddSample( 0x1A ) );
    CallTest( pPHS->ErrAddSample( 0x1A ) );
    CallTest( pPHS->ErrAddSample( 0x3A ) );
    ulPercentile = 50;
    CallTest( pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample ) );
    Validate( 50 == ulPercentile );
    Validate( 0x1A == qwSample );
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrAddSample( 0x2A ) );
    CallTest( pPHS->ErrAddSample( 0x2A ) );
    CallTest( pPHS->ErrAddSample( 0x3A ) );
    CallTest( pPHS->ErrAddSample( 0x3A ) );
    CallTest( pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample ) );
    Validate( 62 == ulPercentile );
    Validate( 0x2A == qwSample );
    CallTest( pPHS->ErrReset() );
    CallTest( pPHS->ErrAddSample( 0x4A ) );
    ulPercentile = 100;
    CallTest( pPHS->ErrGetPercentileHits( &ulPercentile, &qwSample ) );
    Validate( 100 == ulPercentile );
    Validate( 0x4A == qwSample );
    CallTest( pPHS->ErrReset() );

    iTest = 12;

    // specific, simple cases.
    Validate( UlBound( 35, 40, 55 ) == 40 );    // too low
    Validate( UlBound( 35, 20, 30 ) == 30 );    // too high
    Validate( UlBound( 35, 34, 36 )  == 35 );   // just right

    iTest = 13; // test mode
    pPHS->Zero();
    CallTest( pPHS->ErrAddSample( 34 ) );
    CallTest( pPHS->ErrAddSample( 34 ) );
    CallTest( pPHS->ErrAddSample( 35 ) );
    CallTest( pPHS->ErrAddSample( 35 ) );
    CallTest( pPHS->ErrAddSample( 35 ) );
    CallTest( pPHS->ErrAddSample( 2 ) );
    Validate( 35 == pPHS->Mode() );

    iTest = 100;

    delete pPHS;

    if ( fPrintStatus )
        wprintf(L" Stat library unit test completed (successfully)!\n");

    //  Unknown error case got pass the checks, when it should have thrown.
    Validate( fResult );

    return JET_errSuccess;  

HandleError:

    wprintf(L"Failed test!\n");
    if ( pPHS )
        delete pPHS;

    return err == CStats::ERR::errSuccess ? JET_errSuccess : -1;
}


