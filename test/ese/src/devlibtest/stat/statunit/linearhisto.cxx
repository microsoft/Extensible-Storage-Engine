// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

#include "stat.hxx"

//  ================================================================
class LinearHistogramTest : public UNITTEST
//  ================================================================
{
    private:
        static LinearHistogramTest s_instance;

    protected:
        LinearHistogramTest() {}
    public:
        ~LinearHistogramTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

LinearHistogramTest LinearHistogramTest::s_instance;

const char * LinearHistogramTest::SzName() const        { return "Linear"; };
const char * LinearHistogramTest::SzDescription() const { return 
            "Tests the statistic libraries' linear histogram support.";
        }
bool LinearHistogramTest::FRunUnderESE98() const        { return true; }
bool LinearHistogramTest::FRunUnderESENT() const        { return true; }
bool LinearHistogramTest::FRunUnderESE97() const        { return true; }


//  ================================================================
ERR LinearHistogramTest::ErrTest()
//  ================================================================
{
    wprintf( L"\tTesting Linear Histogram support ...\n");

    #define CallTest( func )        if ( CLinearHistogramStats::ERR::errSuccess != ( err = (func) ) )      \
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

    CLinearHistogramStats::ERR err = CLinearHistogramStats::ERR::errSuccess;

    ULONG iTest = 0;
    BOOL fResult = fTrue; // assumed innocent, until proven guilty.
    ULONG fBailOnError = fFalse;

    SAMPLE qwMax            = 0xffffffffffffffff;

    BOOL fPrintStatus = fTrue;

    if ( fPrintStatus )
        wprintf(L" Testing CLinearHistogram...\n");

    CLinearHistogramStats linear30histo( 30 );
    CStats * pLHS = &linear30histo;
    SAMPLE qwSample = 0;
    CHITS chits = 0;

    pLHS->Zero();

    if ( fPrintStatus )
        wprintf(L" Stat library unit test completed (successfully)!\n");

    err = CLinearHistogramStats::ERR::errSuccess;

    //  Test empty class

    CallTest( ErrTestZerodHisto( pLHS ) );

    //  Basic tests

    CallTest( pLHS->ErrAddSample( 100 ) );
    CallTest( pLHS->ErrAddSample( 90 ) );
    CallTest( pLHS->ErrAddSample( 80 ) );
    CallTest( pLHS->ErrAddSample( 120 ) );

    //  Test the samples are properly distributed to the proper multiples of 30

    CallTest( pLHS->ErrGetSampleHits( 0, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( 30, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( 60, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( 90, &chits ) );
    TestTest( 2 == chits );
    CallTest( pLHS->ErrGetSampleHits( 120, &chits ) );
    TestTest( 2 == chits );
    chits = 42; // dirty value
    TestTest( CStats::ERR::wrnOutOfSamples == pLHS->ErrGetSampleHits( 121, &chits ) );
    TestTest( 0 == chits );
    chits = 42; // dirty value
    TestTest( CStats::ERR::wrnOutOfSamples == pLHS->ErrGetSampleHits( qwMax, &chits ) );
    TestTest( 0 == chits );

    CallTest( pLHS->ErrReset() );

    //  Testing mode

    TestTest( 90 == linear30histo.Mode() );

    pLHS->Zero();

    //  Test edge cases of the linear divisions ...

    CallTest( pLHS->ErrAddSample( 1 ) );
    CallTest( pLHS->ErrGetSampleHits( 0, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( 1, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( 29, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( 30, &chits ) );
    TestTest( 1 == chits );
    chits = 42; // dirty value
    TestTest( CStats::ERR::wrnOutOfSamples == pLHS->ErrGetSampleHits( 31, &chits ) );
    TestTest( 0 == chits );
    
    CallTest( pLHS->ErrReset() );

    CallTest( pLHS->ErrAddSample( 30 ) );
    CallTest( pLHS->ErrGetSampleHits( 30, &chits ) );
    TestTest( 2 == chits );

    CallTest( pLHS->ErrReset() );

    CallTest( pLHS->ErrAddSample( 31 ) );
    CallTest( pLHS->ErrGetSampleValues( &qwSample ) );
    TestTest( 30 == qwSample );
    CallTest( pLHS->ErrGetSampleHits( 30, &chits ) );
    TestTest( 2 == chits );

    CallTest( pLHS->ErrGetSampleValues( &qwSample ) );
    TestTest( 60 == qwSample );
    CallTest( pLHS->ErrGetSampleHits( 60, &chits ) );
    TestTest( 1 == chits );

    CallTest( pLHS->ErrReset() );

    pLHS->Zero();

    //  Test catch all case
    //

    CallTest( pLHS->ErrAddSample( 0 ) );
    for ( ULONG i = 0; i < 30; i++ )
    {
        pLHS->ErrAddSample( (SAMPLE)rand() * (SAMPLE)rand() * (SAMPLE)rand() + (SAMPLE)2 );
    }
    CallTest( pLHS->ErrAddSample( qwMax ) );

    CallTest( pLHS->ErrReset() );
    CallTest( pLHS->ErrGetSampleHits( 0, &chits ) );
    TestTest( 1 == chits );
    CallTest( pLHS->ErrGetSampleHits( 1, &chits ) );
    TestTest( 0 == chits );
    CallTest( pLHS->ErrGetSampleHits( qwMax, &chits ) );
    TestTest( 31 == chits );


HandleError:

    if ( err != CLinearHistogramStats::ERR::errSuccess )
    {
        wprintf(L"Failed test!\n");
    }

    return err == CLinearHistogramStats::ERR::errSuccess ? JET_errSuccess : -1;
}


