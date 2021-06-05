// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

#include "stat.hxx"

//  ================================================================
class MovingAverageTest : public UNITTEST
//  ================================================================
{
    private:
        static MovingAverageTest s_instance;

    protected:
        MovingAverageTest() {}
    public:
        ~MovingAverageTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

MovingAverageTest MovingAverageTest::s_instance;

const char * MovingAverageTest::SzName() const          { return "MovingAverage"; };
const char * MovingAverageTest::SzDescription() const   { return  "Tests the statistic libraries' moving average support."; }
bool MovingAverageTest::FRunUnderESE98() const          { return true; }
bool MovingAverageTest::FRunUnderESENT() const          { return true; }
bool MovingAverageTest::FRunUnderESE97() const          { return true; }


//  ================================================================
ERR MovingAverageTest::ErrTest()
//  ================================================================
{
    wprintf( L"\tTesting Moving Average support ...\n");

    #define TestTest( ftest )       if ( !(ftest) )         \
                                        {                               \
                                        wprintf(L" [line=%d] Test( %S ) failed w/ %d\n", __LINE__, #ftest, (DWORD)(ftest) );    \
                                        STATAssertSz( fFalse, "HaltTest" );     \
                                    }

    printf( "\tSingle-sample moving average...\n" );

    CMovingAverage< ULONG, 1 > avgOneSample( 10 );

    TestTest( 10 == avgOneSample.GetLastSample() );
    TestTest( 10 == avgOneSample.GetAverage() );

    avgOneSample.AddSample( 12 );
    TestTest( 12 == avgOneSample.GetLastSample() );
    TestTest( 12 == avgOneSample.GetAverage() );

    avgOneSample.AddSample( 15 );
    TestTest( 15 == avgOneSample.GetLastSample() );
    TestTest( 15 == avgOneSample.GetAverage() );

    avgOneSample.Reset( 20 );
    TestTest( 20 == avgOneSample.GetLastSample() );
    TestTest( 20 == avgOneSample.GetAverage() );

    avgOneSample.AddSample( 30 );
    TestTest( 30 == avgOneSample.GetLastSample() );
    TestTest( 30 == avgOneSample.GetAverage() );

    avgOneSample.AddSample( 5 );
    TestTest( 5 == avgOneSample.GetLastSample() );
    TestTest( 5 == avgOneSample.GetAverage() );


    printf( "\tTwo-sample moving average...\n" );

    CMovingAverage< LONG, 2 > avgTwoSamples( 100 ); // 100, 100

    TestTest( 100 == avgTwoSamples.GetLastSample() );
    TestTest( 100 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 50 );  // 50, 100
    TestTest( 50 == avgTwoSamples.GetLastSample() );
    TestTest( 75 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 10 );  // 50, 10
    TestTest( 10 == avgTwoSamples.GetLastSample() );
    TestTest( 30 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 60 );  // 60, 10
    TestTest( 60 == avgTwoSamples.GetLastSample() );
    TestTest( 35 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 80 );  // 60, 80
    TestTest( 80 == avgTwoSamples.GetLastSample() );
    TestTest( 70 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( -30 ); // -30, 80
    TestTest( -30 == avgTwoSamples.GetLastSample() );
    TestTest( 25 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( -10 ); // -30, -10
    TestTest( -10 == avgTwoSamples.GetLastSample() );
    TestTest( -20 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 110 ); // 110, -10
    TestTest( 110 == avgTwoSamples.GetLastSample() );
    TestTest( 50 == avgTwoSamples.GetAverage() );

    avgTwoSamples.Reset( -30 ); // -30, -30
    TestTest( -30 == avgTwoSamples.GetLastSample() );
    TestTest( -30 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( -40 ); // -40, -30
    TestTest( -40 == avgTwoSamples.GetLastSample() );
    TestTest( -35 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 240 ); // -40, 240
    TestTest( 240 == avgTwoSamples.GetLastSample() );
    TestTest( 100 == avgTwoSamples.GetAverage() );

    avgTwoSamples.AddSample( 60 );  // 60, 240
    TestTest( 60 == avgTwoSamples.GetLastSample() );
    TestTest( 150 == avgTwoSamples.GetAverage() );


    printf( "\tFive-sample moving average...\n" );

    CMovingAverage< INT, 5 > avgFiveSamples( -1000 );   // -1000, -1000, -1000, -1000, -1000

    TestTest( -1000 == avgFiveSamples.GetLastSample() );
    TestTest( -1000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 0 );      // 0, -1000, -1000, -1000, -1000
    TestTest( 0 == avgFiveSamples.GetLastSample() );
    TestTest( -800 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 500 );    // 0, 500, -1000, -1000, -1000
    TestTest( 500 == avgFiveSamples.GetLastSample() );
    TestTest( -500 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 5000 );   // 0, 500, 5000, -1000, -1000
    TestTest( 5000 == avgFiveSamples.GetLastSample() );
    TestTest( 700 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( -10000 ); // 0, 500, 5000, -10000, -1000
    TestTest( -10000 == avgFiveSamples.GetLastSample() );
    TestTest( -1100 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 1000 );   // 0, 500, 5000, -10000, 1000
    TestTest( 1000 == avgFiveSamples.GetLastSample() );
    TestTest( -700 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 10000 );  // 10000, 500, 5000, -10000, 1000
    TestTest( 10000 == avgFiveSamples.GetLastSample() );
    TestTest( 1300 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 5000 );   // 10000, 5000, 5000, -10000, 1000
    TestTest( 5000 == avgFiveSamples.GetLastSample() );
    TestTest( 2200 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( -100000 );// 10000, 5000, -100000, -10000, 1000
    TestTest( -100000 == avgFiveSamples.GetLastSample() );
    TestTest( -18800 == avgFiveSamples.GetAverage() );

    avgFiveSamples.Reset( -10000 );     // -10000, -10000, -10000, -10000, -10000
    TestTest( -10000 == avgFiveSamples.GetLastSample() );
    TestTest( -10000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 0 );      // 0, -10000, -10000, -10000, -10000
    TestTest( 0 == avgFiveSamples.GetLastSample() );
    TestTest( -8000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 0 );      // 0, 0, -10000, -10000, -10000
    TestTest( 0 == avgFiveSamples.GetLastSample() );
    TestTest( -6000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 0 );      // 0, 0, 0, -10000, -10000
    TestTest( 0 == avgFiveSamples.GetLastSample() );
    TestTest( -4000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 0 );      // 0, 0, 0, 0, -10000
    TestTest( 0 == avgFiveSamples.GetLastSample() );
    TestTest( -2000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 0 );      // 0, 0, 0, 0, 0
    TestTest( 0 == avgFiveSamples.GetLastSample() );
    TestTest( 0 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 10000 );  // 10000, 0, 0, 0, 0
    TestTest( 10000 == avgFiveSamples.GetLastSample() );
    TestTest( 2000 == avgFiveSamples.GetAverage() );

    avgFiveSamples.AddSample( 1000 );   // 10000, 1000, 0, 0, 0
    TestTest( 1000 == avgFiveSamples.GetLastSample() );
    TestTest( 2200 == avgFiveSamples.GetAverage() );

    return JET_errSuccess;
}


