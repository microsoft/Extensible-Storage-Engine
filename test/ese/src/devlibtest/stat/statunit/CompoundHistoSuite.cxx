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

CStats::ERR TestSimplePerfectHistogramBehavior( CStats * const phistoTest )
{
    CStats::ERR err = CStats::ERR::errSuccess;

    wprintf( L"---Begin Test on %p ---\n", phistoTest );

    ULONG fBailOnError = fFalse;

    SAMPLE qwSample = 0;
    CHITS chits = 0;


    CallTest( (CStats::ERR)ErrTestZerodHisto( phistoTest, TestZeroFlags( fHasExplicitBuckets | fNoPercentileSupport ) ) );


    CallTest( phistoTest->ErrAddSample( 100 ) );
    CallTest( phistoTest->ErrAddSample( 70 ) );
    CallTest( phistoTest->ErrAddSample( 90 ) );


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


    CallTest( phistoTest->ErrAddSample( 60 ) );
    CallTest( phistoTest->ErrAddSample( 80 ) );
    CallTest( phistoTest->ErrAddSample( 89 ) );
    CallTest( phistoTest->ErrAddSample( 90 ) );


    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 60, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrGetSampleValues( &qwSample ) );
    TestSame( 70, qwSample );
    CallTest( phistoTest->ErrGetSampleHits( qwSample, &chits ) );
    TestSame( 1, chits );
    CallTest( phistoTest->ErrReset() );


    CallTest( phistoTest->ErrGetSampleHits( 80, &chits ) );
    TestSame( 3, chits );
    CallTest( phistoTest->ErrReset() );

    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 55, &chits ) );
    TestSame( 0, chits );
    CallTest( phistoTest->ErrGetSampleHits( 100, &chits ) );
    TestSame( 7, chits );
    chits = 42;
    TestFail( CStats::ERR::wrnOutOfSamples, phistoTest->ErrGetSampleHits( 101, &chits ) );
    TestSame( 0, chits );

    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 100, &chits ) );
    TestSame( 7, chits );
    CallTest( phistoTest->ErrReset() );
    CallTest( phistoTest->ErrGetSampleHits( 104, &chits ) );
    TestSame( 7, chits );

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
    CallTest( phistoTest->ErrGetSampleHits( 99, &chits ) );
    TestSame( 0, chits );
    CallTest( phistoTest->ErrGetSampleHits( 100, &chits ) );
    TestSame( 1, chits );
    TestFail( CStats::ERR::wrnOutOfSamples, phistoTest->ErrGetSampleHits( 105, &chits ) );

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

CUnitTest( CompoundHistogramTest, 0, "Tests a three segmented compound histo with a moving split to ensure we don't miss any values at the separations." );
ERR CompoundHistogramTest::ErrTest()
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


