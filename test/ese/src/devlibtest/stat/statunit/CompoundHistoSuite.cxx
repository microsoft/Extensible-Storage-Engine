// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

#define CallTest( func )    \
            {   \
            if ( CStats::ERR::errSuccess != ( err = (func) ) )      \
                {                               \
                wprintf(L" [line=%d] Call( %S ) failed w/ %d\n", __LINE__, #func, err );    \
                if ( fBailOnError ) goto HandleError;   \
                STATAssertSz( fFalse, "HaltTest" );     \
            }   \
        }
#define TestTest( ftest )   \
            if ( !(ftest) )         \
                {                               \
                wprintf(L" [line=%d] Test( %S ) failed w/ %d\n", __LINE__, #ftest, (DWORD)(ftest) );    \
                if ( fBailOnError ) goto HandleError;   \
                STATAssertSz( fFalse, "HaltTest" );     \
            }
#define TestSame( valueExpected, valueTest )    \
            {   \
            auto valueTestT = (valueTest);  \
            if ( (valueExpected) != valueTestT )        \
                {                                       \
                wprintf(L" [line=%d] Test( %I64d /* %S */ == %I64d /* %S */ ) failed\n", \
                            __LINE__, (QWORD)valueExpected, #valueExpected, (QWORD)valueTestT, #valueTest );    \
                if ( fBailOnError ) goto HandleError;   \
                STATAssertSz( fFalse, "HaltTest" );     \
            }   \
        }
#define TestFail( errExpected, errCall )        \
            {   \
            auto errCallT = (errCall);      \
            if ( (errExpected) != errCallT )        \
                {                               \
                err = errCallT;     \
                wprintf(L" [line=%d] Call( %S ) failed w/ %d, but we expected %d / %S.\n", __LINE__, #errCall, err, errExpected, #errExpected );    \
                if ( fBailOnError ) goto HandleError;   \
                STATAssertSz( fFalse, "HaltTest" );     \
            }   \
            else    \
                {   \
                err = (CStats::ERR)0x0; \
            }   \
        }

// This test performs these operations
//      Insert pattern: 60, 70, 80, 89, 90 x 2, 100  (but not in this order, in two passes)
//      Queries values: Anywhere from 55 to 106.
//          Both for getting sample values, and sample hits.
//
CStats::ERR TestSimplePerfectHistogramBehavior( CStats * const phistoTest )
{
    CStats::ERR err = CStats::ERR::errSuccess;

    wprintf( L"---Begin Test on %p ---\n", phistoTest );

    ULONG fBailOnError = fFalse;

    SAMPLE qwSample = 0;
    CHITS chits = 0;

    //  Test empty class

    CallTest( (CStats::ERR)ErrTestZerodHisto( phistoTest, TestZeroFlags( fHasExplicitBuckets | fNoPercentileSupport ) ) );

    //  1st pass of inserts!

    CallTest( phistoTest->ErrAddSample( 100 ) );
    CallTest( phistoTest->ErrAddSample( 70 ) );
    CallTest( phistoTest->ErrAddSample( 90 ) );

    // Comparing floating point for equality is a pain.  Check
    // that we're within .001% of the expected vale.
    // Precalculated approximate answer, taken from Excel.
    double dblStdDevExpected = 12.47219129;
    TestTest( dblStdDevExpected * 1.000001 > phistoTest->DblStdDev() );
    TestTest( dblStdDevExpected * 0.999999 < phistoTest->DblStdDev() );

    //  Do basic sample enumeration.

    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 70, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 90, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 100, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrReset() );

    //  2nd pass of inserts!

    CallTest( phistoTest->ErrAddSample( 60 ) );
    CallTest( phistoTest->ErrAddSample( 80 ) );
    CallTest( phistoTest->ErrAddSample( 89 ) );
    CallTest( phistoTest->ErrAddSample( 90 ) );

    // Precalculated approximate answer, taken from Excel.
    dblStdDevExpected = 12.70224149;
    TestTest( dblStdDevExpected * 1.000001 > phistoTest->DblStdDev() );
    TestTest( dblStdDevExpected * 0.999999 < phistoTest->DblStdDev() );
    // Diag / debugging 
    //((CCompoundHistogram*)phistoTest)->Dump();

    //  Do 2nd basic sample enumeration (seeing that things have changed).

    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 60, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 70, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrReset() );

    //  Complicated cases.

    //  Check a multi-bucket get hits.
    CallTest( phistoTest->ErrGetSampleHits( 80, &chits ) );
    TestSame( 3, chits );
    CallTest( phistoTest->ErrReset() );

    //  Check empty buckets before.
    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 55, &chits ) );
    TestSame( 0, chits );
    //  Check get all hits.
    CallTest( phistoTest->ErrGetSampleHits( 100, &chits ) );
    TestSame( 7, chits );
    //  Check no remaining hits, and hits zeroed.
    chits = 42; // dirty value
    TestFail( CStats::ERR::wrnOutOfSamples, phistoTest->ErrGetSampleHits( 101, &chits ) );
    TestSame( 0, chits );

    //  Check get all hits (without preceding empty bucket get).
    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 100, &chits ) );
    TestSame( 7, chits );
    //  Check get all hits (beyond the range).
    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 104, &chits ) );
    TestSame( 7, chits );

    //  Check complete list of all hits for each insert sample.
    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 60, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleHits( 70, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleHits( 80, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleHits( 89, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleHits( 90, &chits ) );
    TestSame( 2, chits );
    CallTest( phistoTest->ErrGetSampleHits( 99, &chits ) ); // note: This one inserted to look for no hits between 90 and 99 (inclusive)
    TestSame( 0, chits );
    CallTest( phistoTest->ErrGetSampleHits( 100, &chits ) );
    TestSame( 1, chits );
    TestFail( CStats::ERR::wrnOutOfSamples, phistoTest->ErrGetSampleHits( 105, &chits ) );

    //  Check complete list of all sample values expected.
    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 60, qwSample );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 70, qwSample );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 80, qwSample );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 89, qwSample );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 90, qwSample );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 100, qwSample );
    TestFail( CStats::ERR::wrnOutOfSamples, phistoTest->ErrGetSampleValues( &qwSample ) );

HandleError:

    phistoTest->ErrReset();
    return err;
}

//  ================================================================
CUnitTest( CompoundHistogramTest, 0, "Tests a three segmented compound histo with a moving split to ensure we don't miss any values at the separations." );
ERR CompoundHistogramTest::ErrTest()
//  ================================================================
{
    wprintf( L"\tTesting Compound Histogram support ...\n");

    CStats::ERR err = CStats::ERR::errSuccess;

    ULONG fBailOnError = fFalse;

    SAMPLE qwMax            = 0xffffffffffffffff;

    BOOL fPrintStatus = fTrue;

    if ( fPrintStatus )
        wprintf(L" Check the baseline test ...\n");

    CStats * phistoBaseline = new CPerfectHistogramStats();
    CallTest( TestSimplePerfectHistogramBehavior( phistoBaseline ) );

    if ( fPrintStatus )
        wprintf(L" Testing CCompoundHistogram...\n");
    
    /* for debugging explicit splits ...
    ULONG iDebugOne = 55;
    ULONG iDebugTwo = 102;
    CCompoundHistogram * pchDebug = new CCompoundHistogram();
    pchDebug->ErrAddHistoRange( iDebugOne, new CPerfectHistogramStats() );
    pchDebug->ErrAddHistoRange( iDebugTwo, new CPerfectHistogramStats() );
    pchDebug->ErrAddHistoRange( qwMax, new CPerfectHistogramStats() );
    wprintf( L"    Explicit Splits: %d, %d -> %p\n", iDebugOne, iDebugTwo, pchDebug );
    pchDebug->Zero();
    TestCall( TestSimplePerfectHistogramBehavior( pchDebug ) );
    delete pchDebug;
    //*/
    
    for( ULONG iFirstSplit = 55; iFirstSplit < 106; iFirstSplit++ )
    {
        for( ULONG iSecondSplit = iFirstSplit + 1; iSecondSplit < 106; iSecondSplit++ )
        {
            CCompoundHistogram * pCH = new CCompoundHistogram();
            pCH->ErrAddHistoRange( iFirstSplit, new CPerfectHistogramStats( phsfReadSyncExternalControlled ) );
            pCH->ErrAddHistoRange( iSecondSplit, new CPerfectHistogramStats( phsfReadSyncExternalControlled ) );
            pCH->ErrAddHistoRange( qwMax, new CPerfectHistogramStats( phsfReadSyncExternalControlled ) );
            wprintf( L"    Splits: %d, %d -> %p\n", iFirstSplit, iSecondSplit, pCH );

            pCH->Zero();

            CallTest( TestSimplePerfectHistogramBehavior( pCH ) );

            delete pCH;
        }
    }

HandleError:

    if ( err != CStats::ERR::errSuccess )
    {
        wprintf(L"Failed test!\n");
    }
    else
    {
        if ( fPrintStatus )
            wprintf(L" Stat library unit test completed (successfully)!\n");
    }

    return err == CStats::ERR::errSuccess ? JET_errSuccess : -1;
}


