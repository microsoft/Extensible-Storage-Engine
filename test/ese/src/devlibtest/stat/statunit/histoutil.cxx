// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

CStats::ERR ErrTestZerodHisto( CStats * pHistoClass, const enum TestZeroFlags fZeroTestFlags )
{
    CStats::ERR err = CStats::ERR::errSuccess;

    SAMPLE qwSample = 0;
    CHITS chits = 0;
    ULONG pct = 0;

    ULONG iTest = 0;
    BOOL fResult = fTrue;
    ULONG fBailOnError = fFalse;

    #define CallTest( func )        if ( CStats::ERR::errSuccess != ( err = (func) ) )      \
                                        {                               \
                                        wprintf(L" [iTest=%d,line=%d] Call( %S ) failed w/ %d\n", iTest, __LINE__, #func, err );    \
                                        fResult = fFalse;               \
                                        if ( fBailOnError ) goto HandleError;   \
                                        STATAssertSz( fFalse, "HaltTest" );     \
                                    }
    #define TestTest( ftest )       if ( !(ftest) )         \
                                        {                               \
                                        wprintf(L" [iTest=%d,line=%d] Test( %S ) failed w/ %d\n", iTest, __LINE__, #ftest, (DWORD)(ftest) );    \
                                        fResult = fFalse;               \
                                        if ( fBailOnError ) goto HandleError;   \
                                        STATAssertSz( fFalse, "HaltTest" );     \
                                    }

    pHistoClass->Zero();


    TestTest( pHistoClass->Min() == 0xFFFFFFFFFFFFFFFF );
    TestTest( pHistoClass->Ave() == 0 );
    TestTest( pHistoClass->Max() == 0 );
    TestTest( pHistoClass->C() == 0 );
    TestTest( pHistoClass->Total() == 0 );
    TestTest( pHistoClass->DblAve() == 0 );


    if ( !( fZeroTestFlags & fHasExplicitBuckets ) )
    {
        TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetSampleHits( 0, &chits ) );
        TestTest( 0 == chits );
        CallTest( pHistoClass->ErrReset() );

        TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetSampleHits( 1, &chits ) );
        TestTest( 0 == chits );
        CallTest( pHistoClass->ErrReset() );

        TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetSampleHits( 100, &chits ) );
        TestTest( 0 == chits );
        CallTest( pHistoClass->ErrReset() );
    }

    TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetSampleHits( 0xffffffffffffffff, &chits ) );
    TestTest( 0 == chits );
    CallTest( pHistoClass->ErrReset() );


    if ( !( fZeroTestFlags & fNoPercentileSupport ) )
    {
        pct = 0;
        TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetPercentileHits( &pct, &qwSample ) );
        TestTest( 0 == qwSample );
        CallTest( pHistoClass->ErrReset() );

        pct = 20;
        TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetPercentileHits( &pct, &qwSample ) );
        TestTest( 0 == qwSample );
        CallTest( pHistoClass->ErrReset() );

        pct = 100;
        TestTest( CStats::ERR::wrnOutOfSamples == pHistoClass->ErrGetPercentileHits( &pct, &qwSample ) );
        TestTest( 0 == qwSample );
        CallTest( pHistoClass->ErrReset() );
    }

    pHistoClass->Zero();

HandleError:

    return err;
}
