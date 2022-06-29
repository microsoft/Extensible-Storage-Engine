// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

//  ================================================================
class SegmentedHistogramTest : public UNITTEST
//  ================================================================
{
    private:
        static SegmentedHistogramTest s_instance;

    protected:
        SegmentedHistogramTest() {}
    public:
        ~SegmentedHistogramTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

SegmentedHistogramTest SegmentedHistogramTest::s_instance;

const char * SegmentedHistogramTest::SzName() const     { return "Segmented"; };
const char * SegmentedHistogramTest::SzDescription() const  { return 
            "Tests the statistic libraries' segmented histogram support.";
        }
bool SegmentedHistogramTest::FRunUnderESE98() const     { return true; }
bool SegmentedHistogramTest::FRunUnderESENT() const     { return true; }
bool SegmentedHistogramTest::FRunUnderESE97() const     { return true; }

SAMPLE g_rgHistoSamplesEx1 [] = { 0, 1, 5, 8, 10, 12, 16, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 
                                                    4000, 5000, 10000, 20000, 30000, 60000, 90000, 0xffffffff, 0xffffffffffffffff };

class CSegmentedHistoInPlaceTest
{
    private:

//      const SAMPLE    rgHistoSamples [28] = { 0, 1, 5, 8, 10, 12, 16, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 
//                                                  4000, 5000, 10000, 20000, 30000, 60000, 90000, 0xffffffff, 0xffffffffffffffff };
//      static SAMPLE   rgHistoSamples [28] = { 0, 1, 5, 8, 10, 12, 16, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 
//                                                  4000, 5000, 10000, 20000, 30000, 60000, 90000, 0xffffffff, 0xffffffffffffffff };
        BYTE                    rgbHistoStorage[CbCSegmentedHistogram(g_rgHistoSamplesEx1)];
        CSegmentedHistogram *   phisto;

    public:
        CSegmentedHistoInPlaceTest()
        {
            phisto = new( rgbHistoStorage ) CSegmentedHistogram( g_rgHistoSamplesEx1, _countof(g_rgHistoSamplesEx1), rgbHistoStorage, sizeof(rgbHistoStorage) );
        }

        BOOL FRunInPlaceTest()
        {
            return CSegmentedHistogram::ERR::errSuccess == phisto->ErrAddSample( 19 );
        }

};

class CDoubleInPlace
{
    private:

        BYTE                        rgbHistoStorage[CbCSegmentedHistogram(g_rgHistoSamplesEx1)];
        ULONG                       ulMisalignBuffer;
        BYTE                        rgbHistoStorage2[CbCSegmentedHistogram(g_rgHistoSamplesEx1)];
        CSegmentedHistoInPlaceTest  histoInPlace;

        CSegmentedHistogram *   phisto;
        CSegmentedHistogram *   phisto2;


    public:
        CDoubleInPlace()
        {
            phisto = new( rgbHistoStorage ) CSegmentedHistogram( g_rgHistoSamplesEx1, _countof(g_rgHistoSamplesEx1), rgbHistoStorage, sizeof(rgbHistoStorage) );
            phisto2 = new( rgbHistoStorage2 ) CSegmentedHistogram( g_rgHistoSamplesEx1, _countof(g_rgHistoSamplesEx1), rgbHistoStorage2, sizeof(rgbHistoStorage2) );
        }

        BOOL FRunDoubleInPlaceTest()
        {
            return histoInPlace.FRunInPlaceTest() &&
                    CStats::ERR::errSuccess == phisto->ErrAddSample( 19 ) &&
                    CStats::ERR::errSuccess == phisto2->ErrAddSample( 19 );
        }
};


//  ================================================================
ERR SegmentedHistogramTest::ErrTest()
//  ================================================================
{
    void * pvHistoStorage = NULL;
    CSegmentedHistoInPlaceTest inplace;
    CDoubleInPlace inplacedouble;

    wprintf( L"\tTesting Segmented Histogram support ...\n");

    #define CallTest( func )        if ( CSegmentedHistogram::ERR::errSuccess != ( err = (func) ) )      \
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

    CSegmentedHistogram::ERR err = CSegmentedHistogram::ERR::errSuccess;

    ULONG iTest = 0;
    BOOL fResult = fTrue; // assumed innocent, until proven guilty.
    ULONG fBailOnError = fFalse;

    SAMPLE qwMax            = 0xffffffffffffffff;

    BOOL fPrintStatus = fTrue;

    if ( fPrintStatus )
        wprintf(L" Testing CSegmentedHistogram...\n");

    //  improperly ctor'd histo ...
    //CSegmentedHistogram * pSHSBad = new CSegmentedHistogram();
    // Tested, does not compile.


    SAMPLE rgLimitedSegments [] = { 0, 10, 20, 40, 100 };

    BYTE rgHisto[CbCSegmentedHistogram( rgLimitedSegments )];
    CSegmentedHistogram * pSHS = new (rgHisto) CSegmentedHistogram( rgLimitedSegments, _countof( rgLimitedSegments ), rgHisto, sizeof(rgHisto) );

    pSHS->Zero();

    if ( fPrintStatus )
        wprintf(L" Stat library unit test completed (successfully)!\n");

    err = CSegmentedHistogram::ERR::errSuccess;

    //  Test empty class

    CallTest( ErrTestZerodHisto( pSHS, TestZeroFlags( fHasExplicitBuckets | fNoPercentileSupport ) ) );

    //  Test basics

    CallTest( pSHS->ErrAddSample( 100 ) );
    CallTest( pSHS->ErrAddSample( 90 ) );
    CallTest( pSHS->ErrAddSample( 80 ) );


    CallTest( pSHS->ErrReset() );
    CHITS chits = 0;
    CallTest( pSHS->ErrGetSampleHits( 40, &chits ) );
    TestTest( 0 == chits );
    CallTest( pSHS->ErrGetSampleHits( 100, &chits ) );
    TestTest( 3 == chits );
    chits = 42; // dirty value
    TestTest( CStats::ERR::wrnOutOfSamples == pSHS->ErrGetSampleHits( 101, &chits ) );
    TestTest( 0 == chits );

    //  Testing mode
    //
    CallTest( pSHS->ErrAddSample( 34 ) );
    CallTest( pSHS->ErrAddSample( 34 ) );
    CallTest( pSHS->ErrAddSample( 34 ) );
    CallTest( pSHS->ErrAddSample( 24 ) );
    CallTest( pSHS->ErrAddSample( 24 ) );
    CallTest( pSHS->ErrAddSample( 0 ) );
    CallTest( pSHS->ErrAddSample( 3 ) );

    // DblStdDev() is not implemented for this class, so returns NotANumber.
    TestTest( isnan(pSHS->DblStdDev() ) );

    TestTest( 40 == pSHS->Mode() );

    pSHS = NULL;

    //  2nd set of tests, using a malloc()'d chunk of memory
    //

    ULONG cbHisto = CbCSegmentedHistogram( rgLimitedSegments );
    pvHistoStorage = malloc( cbHisto );
    if ( pvHistoStorage == NULL )
    {
        if ( fPrintStatus )
            wprintf( L"Test %d failed to alloc %d bytes\n", iTest, cbHisto );
        return JET_errOutOfMemory;
    }
    pSHS = new (pvHistoStorage) CSegmentedHistogram( rgLimitedSegments, _countof( rgLimitedSegments ), pvHistoStorage, cbHisto );

    //  Testing we can do in-place "alloc-less" creation / initialization
    //

    if ( !inplace.FRunInPlaceTest() )
    {
        wprintf(L" [iTest=%d,line=%d] failed w/ FRunInPlaceTest()\n", iTest, __LINE__ );
        fResult = fFalse;               \
        if ( fBailOnError ) goto HandleError;   \
        STATAssertSz( fFalse, "HaltTest" );     \
    }
    
    if ( !inplacedouble.FRunDoubleInPlaceTest() )
    {
        wprintf(L" [iTest=%d,line=%d] failed w/ FRunDoubleInPlaceTest()\n", iTest, __LINE__ );
        fResult = fFalse;               \
        if ( fBailOnError ) goto HandleError;   \
        STATAssertSz( fFalse, "HaltTest" );     \
    }

    //  Test catch all case
    //

    SAMPLE rgThreeCoveringSegments [] = { 0, 1, qwMax };

    BYTE rgHisto2[CbCSegmentedHistogram( rgThreeCoveringSegments )];

    pSHS = new (rgHisto) CSegmentedHistogram( rgThreeCoveringSegments, _countof( rgThreeCoveringSegments ), rgHisto2, sizeof(rgHisto2) );

    CallTest( pSHS->ErrAddSample( 0 ) );
    for ( ULONG i = 0; i < 30; i++ )
    {
        pSHS->ErrAddSample( (SAMPLE)rand() * (SAMPLE)rand() * (SAMPLE)rand() + (SAMPLE)2 );
    }
    CallTest( pSHS->ErrAddSample( qwMax ) );

    CallTest( pSHS->ErrReset() );
    CallTest( pSHS->ErrGetSampleHits( 0, &chits ) );
    TestTest( 1 == chits );
    CallTest( pSHS->ErrGetSampleHits( 1, &chits ) );
    TestTest( 0 == chits );
    CallTest( pSHS->ErrGetSampleHits( qwMax, &chits ) );
    TestTest( 31 == chits );


HandleError:

    if ( err != CSegmentedHistogram::ERR::errSuccess )
    {
        wprintf(L"Failed test!\n");
    }

    if ( pvHistoStorage )
    {
        free( pvHistoStorage );
    }

    return err == CSegmentedHistogram::ERR::errSuccess ? JET_errSuccess : -1;
}


