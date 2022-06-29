// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

#include "stat.hxx"

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

class EmbeddedMultiHistos
{
private:
    static const INT cHistoSections = 6;

    BYTE rgbHistoUp[     CbCFixedLinearHistogram( cHistoSections ) ];
    BYTE rgbHistoDown[   CbCFixedLinearHistogram( cHistoSections ) ];
    BYTE rgbHistoTop[    CbCFixedLinearHistogram( cHistoSections ) ];
    BYTE rgbHistoBottom[ CbCFixedLinearHistogram( cHistoSections ) ];
    BYTE rgbHistoStrange[CbCFixedLinearHistogram( cHistoSections ) ];
    BYTE rgbHistoCharm[  CbCFixedLinearHistogram( cHistoSections ) ];

public:
    EmbeddedMultiHistos()
    {
        new( rgbHistoUp )     CFixedLinearHistogram( sizeof( rgbHistoUp ),     20, 10, cHistoSections, flhfFullRoundUp );
        new( rgbHistoDown )   CFixedLinearHistogram( sizeof( rgbHistoDown ),   20, 10, cHistoSections, flhfFullRoundDown );
        new( rgbHistoTop )    CFixedLinearHistogram( sizeof( rgbHistoTop ),    20, 10, cHistoSections, flhfFullRoundUp | flhfSoftMax );
        new( rgbHistoBottom ) CFixedLinearHistogram( sizeof( rgbHistoBottom ), 20, 10, cHistoSections, flhfFullRoundUp | flhfSoftMin );
        new( rgbHistoStrange )CFixedLinearHistogram( sizeof( rgbHistoStrange ),20, 10, cHistoSections, flhfFullRoundUp | flhfHardMin );
        new( rgbHistoCharm )  CFixedLinearHistogram( sizeof( rgbHistoCharm ),  20, 10, cHistoSections, flhfFullRoundUp | flhfSoftMax );
    }

    CStats * PhistoUp()     { return (CStats*)rgbHistoUp; }
    CStats * PhistoDown()   { return (CStats*)rgbHistoDown; }
    CStats * PhistoTop()    { return (CStats*)rgbHistoTop; }
    CStats * PhistoBottom() { return (CStats*)rgbHistoBottom; }
    CStats * PhistoStrange(){ return (CStats*)rgbHistoStrange; }
    CStats * PhistoCharm()  { return (CStats*)rgbHistoCharm; }

    CStats * Phisto( const INT ihisto )
    {
        switch( ihisto )
        {
            case 0: return PhistoUp();
            case 1: return PhistoDown();
            case 2: return PhistoTop();
            case 3: return PhistoBottom();
            case 4: return PhistoStrange();
            case 5: return PhistoCharm();
        }
        STATAssert( fFalse );
        return NULL;
    }
    INT C() const { return 6; }
};

//  ================================================================
CUnitTest( FixedLinearHistogramRoundingAndCappingTest, 0, "Testing the rounding and capping of the fixed lineary histogram." );
ERR FixedLinearHistogramRoundingAndCappingTest::ErrTest()
//  ================================================================
{
    ULONG fBailOnError = fFalse;
    CStats::ERR err;
    wprintf( L"\tTesting Embedded Fixed Linear Histogram support ... size = %d\n", sizeof( EmbeddedMultiHistos ) );
    EmbeddedMultiHistos  sMultiStats;

    SAMPLE qwSample;
    CHITS cHits;

    for( INT ihisto = 0; ihisto < sMultiStats.C(); ihisto++ )
    {
        sMultiStats.Phisto( ihisto )->Zero();
    }

    //  Add all the valid samples

    for( INT ihisto = 0; ihisto < sMultiStats.C(); ihisto++ )
    {
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 21 ) );
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 20 ) );
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 29 ) );
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 30 ) );

        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 50 ) );
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 51 ) );
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 59 ) );
        CallTest( sMultiStats.Phisto( ihisto )->ErrAddSample( 60 ) );

        // DblStdDev() is not implemented for this class, so returns NotANumber.
        TestTest( isnan( sMultiStats.Phisto( ihisto )->DblStdDev() ) );
    }

    //  Check the samples are all expected

    for( INT ihisto = 0; ihisto < sMultiStats.C(); ihisto++ )
    {
        CallTest( sMultiStats.Phisto( ihisto )->ErrGetSampleValues( &qwSample ) );  TestSame( 20, qwSample );
        CallTest( sMultiStats.Phisto( ihisto )->ErrGetSampleValues( &qwSample ) );  TestSame( 30, qwSample );
        CallTest( sMultiStats.Phisto( ihisto )->ErrGetSampleValues( &qwSample ) );  TestSame( 50, qwSample );
        CallTest( sMultiStats.Phisto( ihisto )->ErrGetSampleValues( &qwSample ) );  TestSame( 60, qwSample );
        TestFail( CStats::ERR::wrnOutOfSamples, sMultiStats.Phisto( ihisto )->ErrGetSampleValues( &qwSample ) );

        CallTest( sMultiStats.Phisto( ihisto )->ErrReset() );
    }

    //  Test that the rounding up case works ...

    CallTest( sMultiStats.PhistoUp()->ErrGetSampleValues( &qwSample ) );        TestSame( 20, qwSample );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleHits( qwSample, &cHits ) );       TestSame( 1, cHits );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleValues( &qwSample ) );        TestSame( 30, qwSample );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleHits( qwSample, &cHits ) );       TestSame( 3, cHits );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleValues( &qwSample ) );        TestSame( 50, qwSample );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleHits( qwSample, &cHits ) );       TestSame( 1, cHits );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleValues( &qwSample ) );        TestSame( 60, qwSample );
    CallTest( sMultiStats.PhistoUp()->ErrGetSampleHits( qwSample, &cHits ) );       TestSame( 3, cHits );

    //  Test that the rounding down case works ...

    CallTest( sMultiStats.PhistoDown()->ErrGetSampleValues( &qwSample ) );      TestSame( 20, qwSample );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleHits( qwSample, &cHits ) );     TestSame( 3, cHits );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleValues( &qwSample ) );      TestSame( 30, qwSample );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleHits( qwSample, &cHits ) );     TestSame( 1, cHits );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleValues( &qwSample ) );      TestSame( 50, qwSample );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleHits( qwSample, &cHits ) );     TestSame( 3, cHits );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleValues( &qwSample ) );      TestSame( 60, qwSample );
    CallTest( sMultiStats.PhistoDown()->ErrGetSampleHits( qwSample, &cHits ) );     TestSame( 1, cHits );

    return JET_errSuccess;

HandleError:

    return err == CStats::ERR::errSuccess ? JET_errSuccess : -1;
}


//  ================================================================
CUnitTest( FixedLinearHistogramBasicTest, 0, "Testing just the basics of linear fixed histo." );
ERR FixedLinearHistogramBasicTest::ErrTest()
//  ================================================================
{
    wprintf( L"\tTesting Fixed Linear Histogram support ...\n");

    CStats::ERR err = CStats::ERR::errSuccess;

    ULONG fBailOnError = fFalse;

    SAMPLE qwMax            = 0xffffffffffffffff;

    BOOL fPrintStatus = fTrue;

    if ( fPrintStatus )
        wprintf(L" Testing CFixedLinearHistogram...\n");

    BYTE rgbFixedHistoInline[ CbCFixedLinearHistogram( 11 ) ];
    CStats * phisto = new( rgbFixedHistoInline )CFixedLinearHistogram( sizeof( rgbFixedHistoInline ), 0, 10, 11, flhfFullRoundUp );

    SAMPLE qwSample = 0;
    CHITS chits = 0;

    phisto->Zero();

    if ( fPrintStatus )
        wprintf(L" Stat library unit test completed (successfully)!\n");

    err = CStats::ERR::errSuccess;

    //  Test empty class

    CallTest( ErrTestZerodHisto( phisto, fHasExplicitBuckets | fNoPercentileSupport ) );

    //  Basic tests

    CallTest( phisto->ErrAddSample( 90 ) );
    CallTest( phisto->ErrAddSample( 40 ) );
    CallTest( phisto->ErrAddSample( 50 ) );
    CallTest( phisto->ErrAddSample( 100 ) );

    //  Test the samples are properly distributed to the proper multiples of 30

    CallTest( phisto->ErrGetSampleHits( 0, &chits ) );
    TestTest( 0 == chits );
    CallTest( phisto->ErrGetSampleHits( 10, &chits ) );
    TestTest( 0 == chits );
    CallTest( phisto->ErrGetSampleHits( 20, &chits ) );
    TestTest( 0 == chits );
    CallTest( phisto->ErrGetSampleHits( 50, &chits ) );
    TestTest( 2 == chits );
    CallTest( phisto->ErrGetSampleHits( 100, &chits ) );
    TestTest( 2 == chits );
    chits = 42; // dirty value
    TestTest( CStats::ERR::wrnOutOfSamples == phisto->ErrGetSampleHits( 121, &chits ) );
    TestTest( 0 == chits );
    chits = 42; // dirty value
    TestTest( CStats::ERR::wrnOutOfSamples == phisto->ErrGetSampleHits( qwMax, &chits ) );
    TestTest( 0 == chits );

    CallTest( phisto->ErrReset() );

    phisto->Zero();

    //  Test edge cases of the linear divisions ...

    CallTest( phisto->ErrAddSample( 1 ) );
    CallTest( phisto->ErrGetSampleHits( 0, &chits ) );
    TestTest( 0 == chits );
    CallTest( phisto->ErrGetSampleHits( 1, &chits ) );
    TestTest( 0 == chits );
    CallTest( phisto->ErrGetSampleHits( 9, &chits ) );
    TestTest( 0 == chits );
    CallTest( phisto->ErrGetSampleHits( 10, &chits ) );
    TestTest( 1 == chits );
    chits = 42; // dirty value
    TestFail( CStats::ERR::wrnOutOfSamples, phisto->ErrGetSampleHits( 31, &chits ) );
    TestTest( 0 == chits );
    
    CallTest( phisto->ErrReset() );

    CallTest( phisto->ErrAddSample( 9 ) );
    CallTest( phisto->ErrGetSampleHits( 30, &chits ) );
    TestTest( 2 == chits );

    CallTest( phisto->ErrReset() );

    CallTest( phisto->ErrAddSample( 11 ) );
    CallTest( phisto->ErrGetSampleValues( &qwSample ) );
    TestTest( 10 == qwSample );
    CallTest( phisto->ErrGetSampleHits( 10, &chits ) );
    TestTest( 2 == chits );

    CallTest( phisto->ErrGetSampleValues( &qwSample ) );
    TestTest( 20 == qwSample );
    CallTest( phisto->ErrGetSampleHits( 20, &chits ) );
    TestTest( 1 == chits );

    CallTest( phisto->ErrReset() );

    phisto->Zero();

    //  Test catch all case
    //

    CallTest( phisto->ErrAddSample( 0 ) );
    for ( ULONG i = 0; i < 30; i++ )
    {
        phisto->ErrAddSample( (SAMPLE)rand() * (SAMPLE)rand() * (SAMPLE)rand() + (SAMPLE)2 );
    }
    TestFail( CStats::ERR::errInvalidParameter, phisto->ErrAddSample( qwMax ) );


HandleError:

    if ( err != CStats::ERR::errSuccess )
    {
        wprintf(L"Failed test!\n");
    }

    return err == CStats::ERR::errSuccess ? JET_errSuccess : -1;
}


