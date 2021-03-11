// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"


class ResMgrEmulatorLRUKTestTest : public UNITTEST
{
    private:
        static ResMgrEmulatorLRUKTestTest s_instance;

    protected:
        ResMgrEmulatorLRUKTestTest() {}
    public:
        ~ResMgrEmulatorLRUKTestTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrInitInvalidParameters_();
        ERR ErrInitValidParameters_();
        ERR ErrCacheAlreadyCachedPages_();
        ERR ErrTouchUncachedPages_();
        ERR ErrSupercoldUncachedPages_();
        ERR ErrEvictUncachedPages_();
        ERR ErrEvictNextOnEmptyCache_();
        ERR ErrBasicCacheEvict_();
        ERR ErrBasicCacheTouchEvict_();
        ERR ErrSupercoldWorks_();
        ERR ErrBasicLru2_();
        ERR ErrEvictWithinCorrelationInterval_();
        ERR ErrKeepHistory_();
        ERR ErrExplicitEvictionDoesntKeepHistory_();
        ERR ErrSupercoldDoesntKeepHistory_();
        ERR ErrBasicLruKMax_();
        ERR ErrLruKMaxWithMoreTouches_();
        ERR ErrBoostFromCorrelatedTouchOnTouch_();
        ERR ErrBoostFromCorrelatedTouchOnReCache_();
        ERR ErrBasicIntegrationLruKMax_();
};

ResMgrEmulatorLRUKTestTest ResMgrEmulatorLRUKTestTest::s_instance;

const char * ResMgrEmulatorLRUKTestTest::SzName() const         { return "ResMgrEmulatorLRUKTestTest"; };
const char * ResMgrEmulatorLRUKTestTest::SzDescription() const  { return "LRU-K tests for PageEvictionEmulator."; }
bool ResMgrEmulatorLRUKTestTest::FRunUnderESE98() const         { return true; }
bool ResMgrEmulatorLRUKTestTest::FRunUnderESENT() const         { return true; }
bool ResMgrEmulatorLRUKTestTest::FRunUnderESE97() const         { return true; }

ERR ResMgrEmulatorLRUKTestTest::ErrTest()
{
    ERR err = JET_errSuccess;

    TestCall( ErrInitInvalidParameters_() );
    TestCall( ErrInitValidParameters_() );
    TestCall( ErrCacheAlreadyCachedPages_() );
    TestCall( ErrTouchUncachedPages_() );
    TestCall( ErrSupercoldUncachedPages_() );
    TestCall( ErrEvictUncachedPages_() );
    TestCall( ErrEvictNextOnEmptyCache_() );
    TestCall( ErrBasicCacheEvict_() );
    TestCall( ErrBasicCacheTouchEvict_() );
    TestCall( ErrSupercoldWorks_() );
    TestCall( ErrBasicLru2_() );
    TestCall( ErrEvictWithinCorrelationInterval_() );
    TestCall( ErrKeepHistory_() );
    TestCall( ErrExplicitEvictionDoesntKeepHistory_() );
    TestCall( ErrSupercoldDoesntKeepHistory_() );
    TestCall( ErrBasicLruKMax_() );
    TestCall( ErrLruKMaxWithMoreTouches_() );
    TestCall( ErrBoostFromCorrelatedTouchOnTouch_() );
    TestCall( ErrBoostFromCorrelatedTouchOnReCache_() );
    TestCall( ErrBasicIntegrationLruKMax_() );

HandleError:
    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrInitInvalidParameters_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheck( ( lruktest.ErrInit( 0, 1.0 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errInvalidParameter ) );
    TestCheck( ( lruktest.ErrInit( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax + 1, 1.0 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errInvalidParameter ) );
    TestCheck( ( lruktest.ErrInit( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax, 0.0001 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errInvalidParameter ) );

    TestCheckErr( lruktest.ErrInit( 1, 1.0 ) );
    lruktest.Term();

    TestCheckErr( lruktest.ErrInit( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax, 1.0 ) );
    lruktest.Term();

    TestCheckErr( lruktest.ErrInit( 1, 1.0 ) );
    lruktest.Term();
    

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrInitValidParameters_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 1.0 ) );
    lruktest.Term();

    TestCheckErr( lruktest.ErrInit( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax, 1.0 ) );
    lruktest.Term();

    TestCheckErr( lruktest.ErrInit( 1, 0.0 ) );
    lruktest.Term();

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );
    lruktest.Term();

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrCacheAlreadyCachedPages_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );
    TestCheck( ( lruktest.ErrCacheResource( 10, 10, true ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );

    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheck( ( lruktest.ErrCacheResource( 12, 10, true ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );

    TestCheckErr( lruktest.ErrEvictResource( 12 ) );
    TestCheck( ( lruktest.ErrCacheResource( 10, 10, true ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheck( ( lruktest.ErrCacheResource( 12, 10, true ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrTouchUncachedPages_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheck( ( lruktest.ErrTouchResource( 10, 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );
    TestCheckErr( lruktest.ErrTouchResource( 10, 10 ) );
    TestCheckErr( lruktest.ErrTouchResource( 10, 10 ) );

    TestCheck( ( lruktest.ErrTouchResource( 12, 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheckErr( lruktest.ErrTouchResource( 12, 10 ) );
    TestCheckErr( lruktest.ErrTouchResource( 12, 10 ) );

    TestCheckErr( lruktest.ErrEvictResource( 12 ) );
    TestCheck( ( lruktest.ErrTouchResource( 12, 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrTouchResource( 10, 10 ) );
    TestCheckErr( lruktest.ErrEvictResource( 10 ) );
    TestCheck( ( lruktest.ErrTouchResource( 10, 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheckErr( lruktest.ErrTouchResource( 12, 10 ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );
    TestCheckErr( lruktest.ErrTouchResource( 10, 10 ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrSupercoldUncachedPages_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheck( ( lruktest.ErrMarkResourceAsSuperCold( 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 10 ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 10 ) );

    TestCheck( ( lruktest.ErrMarkResourceAsSuperCold( 12 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 12 ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 12 ) );

    TestCheckErr( lruktest.ErrEvictResource( 12 ) );
    TestCheck( ( lruktest.ErrMarkResourceAsSuperCold( 12 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 10 ) );
    TestCheckErr( lruktest.ErrEvictResource( 10 ) );
    TestCheck( ( lruktest.ErrMarkResourceAsSuperCold( 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 12 ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 10 ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrEvictUncachedPages_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheck( ( lruktest.ErrEvictResource( 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );
    TestCheckErr( lruktest.ErrEvictResource( 10 ) );
    TestCheck( ( lruktest.ErrEvictResource( 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 10, true ) );

    TestCheckErr( lruktest.ErrEvictResource( 12 ) );
    TestCheck( ( lruktest.ErrEvictResource( 12 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
    TestCheckErr( lruktest.ErrEvictResource( 10 ) );
    TestCheck( ( lruktest.ErrEvictResource( 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrEvictNextOnEmptyCache_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

    TestCheckErr( lruktest.ErrCacheResource( 12, 10, true ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 10 ) );
    TestCheck( key == 12 );
    TestCheck( ( lruktest.ErrEvictNextResource( &key, 10 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

    TestCheckErr( lruktest.ErrCacheResource( 10, 20, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 30, true ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 10 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 12 );
    TestCheck( ( lruktest.ErrEvictNextResource( &key, 40 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBasicCacheEvict_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 20, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 0 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 11 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 12 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 10 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 40 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBasicCacheTouchEvict_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 20, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 0 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 11 );

    TestCheckErr( lruktest.ErrTouchResource( 12, 50 ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 60 ) );
    TestCheck( key == 10 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 60 ) );
    TestCheck( key == 12 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 60 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrSupercoldWorks_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 1, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 10, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 20, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 0 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 40 ) );
    TestCheck( key == 11 );

    TestCheckErr( lruktest.ErrTouchResource( 12, 50 ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 12 ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 60 ) );
    TestCheck( key == 12 );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 60 ) );
    TestCheck( key == 10 );

    TestCheckErr( lruktest.ErrCacheResource( 10, 70, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 80, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 90, true ) );

    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 11 ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 11 ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 100 ) );
    TestCheck( key == 11 );
    
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 100 ) );
    TestCheck( key == 10 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 100 ) );
    TestCheck( key == 12 );

    TestCheckErr( lruktest.ErrCacheResource( 10, 110, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 120, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 130, true ) );

    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 11 ) );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 11 ) );
    TestCheckErr( lruktest.ErrTouchResource( 11, 140 ) );   
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 150 ) );
    TestCheck( key == 10 );
    
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 150 ) );
    TestCheck( key == 12 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 150 ) );
    TestCheck( key == 11 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 150 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBasicLru2_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 2, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 20, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 40, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 50, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 60, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 70 ) );
    TestCheck( key == 0 );


    TestCheckErr( lruktest.ErrTouchResource( 12, 80 ) );
    TestCheckErr( lruktest.ErrTouchResource( 13, 90 ) );
    TestCheckErr( lruktest.ErrTouchResource( 14, 100 ) );


    TestCheckErr( lruktest.ErrCacheResource( 17, 110, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 17 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 14 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 12 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 13 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 150 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrEvictWithinCorrelationInterval_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 2, 0.025 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 20, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 40, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 50, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 60, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 70 ) );
    TestCheck( key == 0 );


    TestCheckErr( lruktest.ErrTouchResource( 14, 80 ) );
    TestCheckErr( lruktest.ErrTouchResource( 13, 90 ) );
    TestCheckErr( lruktest.ErrTouchResource( 12, 100 ) );


    TestCheckErr( lruktest.ErrCacheResource( 17, 110, true ) );


    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 17 ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 17 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 11 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 14 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 13 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 12 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 120 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrKeepHistory_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 2, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 20, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 40, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 50, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 60, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 70 ) );
    TestCheck( key == 0 );


    TestCheckErr( lruktest.ErrTouchResource( 12, 80 ) );
    TestCheckErr( lruktest.ErrTouchResource( 13, 90 ) );
    TestCheckErr( lruktest.ErrTouchResource( 14, 100 ) );


    TestCheckErr( lruktest.ErrCacheResource( 17, 110, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 17 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 14 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 12 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 13 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 120 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );


    TestCheckErr( lruktest.ErrCacheResource( 11, 130, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 140, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 150, false ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 160, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 170, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 180, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 13 );

    
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 12 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 14 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 150 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrExplicitEvictionDoesntKeepHistory_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 2, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 20, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 40, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 50, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 60, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 70 ) );
    TestCheck( key == 0 );


    TestCheckErr( lruktest.ErrTouchResource( 12, 80 ) );
    TestCheckErr( lruktest.ErrTouchResource( 13, 90 ) );
    TestCheckErr( lruktest.ErrTouchResource( 14, 100 ) );


    TestCheckErr( lruktest.ErrCacheResource( 17, 110, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 17 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 14 );
    TestCheckErr( lruktest.ErrEvictResource( 12 ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 13 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 120 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );


    TestCheckErr( lruktest.ErrCacheResource( 11, 130, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 140, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 150, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 160, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 170, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 180, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 12 );

    
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 13 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 14 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 150 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrSupercoldDoesntKeepHistory_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheckErr( lruktest.ErrInit( 2, 0.001 ) );

    TestCheckErr( lruktest.ErrCacheResource( 0, 5, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 10, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 20, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 30, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 40, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 50, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 11, 60, true ) );

    TestCheckErr( lruktest.ErrEvictNextResource( &key, 70 ) );
    TestCheck( key == 0 );


    TestCheckErr( lruktest.ErrTouchResource( 12, 80 ) );
    TestCheckErr( lruktest.ErrTouchResource( 13, 90 ) );
    TestCheckErr( lruktest.ErrTouchResource( 14, 100 ) );


    TestCheckErr( lruktest.ErrCacheResource( 17, 110, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 17 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 14 );
    TestCheckErr( lruktest.ErrMarkResourceAsSuperCold( 12 ) );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 12 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 120 ) );
    TestCheck( key == 13 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 120 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );


    TestCheckErr( lruktest.ErrCacheResource( 11, 130, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 12, 140, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 14, 150, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 13, 160, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 16, 170, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 15, 180, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 12 );

    
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 15 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 16 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 11 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 13 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 190 ) );
    TestCheck( key == 14 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 150 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBasicLruKMax_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheck( ( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax == 5 ) );
    TestCheckErr( lruktest.ErrInit( 5, 0.001 ) );


    TICK tick = 10;
    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        TestCheckErr( lruktest.ErrCacheResource( pgno, tick, true ) );
        tick += 10;
    }


    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        const QWORD cTouches = ( 50 - pgno ) / 10;

        for ( QWORD iTouch = 0; iTouch < cTouches; iTouch++ )
        {
            TestCheckErr( lruktest.ErrTouchResource( pgno, tick ) );
            tick += 10;
        }
    }


    for ( QWORD pgnoT = 41; pgnoT >= 1; )
    {
        for ( QWORD pgno = pgnoT; pgno < ( pgnoT + 10 ); pgno++ )
        {
            TestCheckErr( lruktest.ErrEvictNextResource( &key, tick ) );
            TestCheck( key == pgno );
        }

        if ( pgnoT == 1 )
        {
            break;
        }
        else
        {
            pgnoT -= 10;
        }
    }

    TestCheck( ( lruktest.ErrEvictNextResource( &key, tick ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrLruKMaxWithMoreTouches_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheck( ( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax == 5 ) );
    TestCheckErr( lruktest.ErrInit( 5, 0.001 ) );


    TestCheckErr( lruktest.ErrCacheResource( 1, 0, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 2, 100, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 3, 200, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 4, 215, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 5, 1000, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 6, 2000, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 7, 3000, true ) );


    TestCheckErr( lruktest.ErrTouchResource( 1, 230 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 330 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 430 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 530 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 630 ) );


    TestCheckErr( lruktest.ErrTouchResource( 2, 210 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 310 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 410 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 510 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 610 ) );


    TestCheckErr( lruktest.ErrTouchResource( 3, 250 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 350 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 450 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 550 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 650 ) );


    TestCheckErr( lruktest.ErrTouchResource( 4, 315 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 415 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 515 ) );


    TestCheckErr( lruktest.ErrTouchResource( 5, 1010 ) );
    TestCheckErr( lruktest.ErrTouchResource( 5, 1110 ) );


    TestCheckErr( lruktest.ErrTouchResource( 6, 2010 ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 7 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 6 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 5 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 4 );


    TestCheckErr( lruktest.ErrCacheResource( 4, 4000, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 2 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 4 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 1 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 3 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 5000 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBoostFromCorrelatedTouchOnTouch_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheck( ( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax == 5 ) );
    TestCheckErr( lruktest.ErrInit( 5, 0.010 ) );


    TestCheckErr( lruktest.ErrCacheResource( 1, 0, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 2, 100, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 3, 200, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 4, 215, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 5, 1000, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 6, 2000, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 7, 3000, true ) );


    TestCheckErr( lruktest.ErrTouchResource( 1, 230 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 330 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 430 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 530 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 630 ) );


    TestCheckErr( lruktest.ErrTouchResource( 2, 210 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 310 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 410 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 510 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 610 ) );


    TestCheckErr( lruktest.ErrTouchResource( 3, 250 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 350 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 450 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 550 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 650 ) );


    TestCheckErr( lruktest.ErrTouchResource( 4, 315 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 415 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 515 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 524 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 533 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 535 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 615 ) );


    TestCheckErr( lruktest.ErrTouchResource( 5, 1010 ) );
    TestCheckErr( lruktest.ErrTouchResource( 5, 1110 ) );


    TestCheckErr( lruktest.ErrTouchResource( 6, 2010 ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 7 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 6 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 5 );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 2 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 1 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 4 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 3 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 4000 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBoostFromCorrelatedTouchOnReCache_()
{
    ERR err = JET_errSuccess;
    QWORD key = 0;

    printf( "\t%s\r\n", __FUNCTION__ );

    CLRUKTestResourceUtilityManager<QWORD, TICK> lruktest;

    TestCheck( ( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax == 5 ) );
    TestCheckErr( lruktest.ErrInit( 5, 0.010 ) );


    TestCheckErr( lruktest.ErrCacheResource( 1, 0, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 2, 100, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 3, 200, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 4, 215, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 5, 1000, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 6, 2000, true ) );
    TestCheckErr( lruktest.ErrCacheResource( 7, 3000, true ) );


    TestCheckErr( lruktest.ErrTouchResource( 1, 230 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 330 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 430 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 530 ) );
    TestCheckErr( lruktest.ErrTouchResource( 1, 630 ) );


    TestCheckErr( lruktest.ErrTouchResource( 2, 210 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 310 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 410 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 510 ) );
    TestCheckErr( lruktest.ErrTouchResource( 2, 610 ) );


    TestCheckErr( lruktest.ErrTouchResource( 3, 250 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 350 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 450 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 550 ) );
    TestCheckErr( lruktest.ErrTouchResource( 3, 650 ) );


    TestCheckErr( lruktest.ErrTouchResource( 4, 315 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 415 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 515 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 524 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 533 ) );
    TestCheckErr( lruktest.ErrTouchResource( 4, 535 ) );


    TestCheckErr( lruktest.ErrTouchResource( 5, 1010 ) );
    TestCheckErr( lruktest.ErrTouchResource( 5, 1110 ) );


    TestCheckErr( lruktest.ErrTouchResource( 6, 2010 ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 7 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 6 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 5 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 4000 ) );
    TestCheck( key == 4 );


    TestCheckErr( lruktest.ErrCacheResource( 4, 4000, true ) );


    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 2 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 1 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 4 );
    TestCheckErr( lruktest.ErrEvictNextResource( &key, 5000 ) );
    TestCheck( key == 3 );

    TestCheck( ( lruktest.ErrEvictNextResource( &key, 5000 ) == CLRUKTestResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    lruktest.Term();

    return err;
}

ERR ResMgrEmulatorLRUKTestTest::ErrBasicIntegrationLruKMax_()
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );
    
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKTest algorithm;


    BFTRACE rgbftrace[ 224 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 10;
    const TICK tickBegin = tick;

    TestCheck( ( CLRUKTestResourceUtilityManager<QWORD, TICK>::KMax == 5 ) );


    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    rgbftrace[ iTrace ].bfinit.K = 5;
    rgbftrace[ iTrace ].bfinit.csecCorrelatedTouch = 0.010;
    iTrace++;
    tick += 10;


    for ( PGNO pgno = 1; iTrace < 51; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 10;
    }


    for ( PGNO pgno = 1; pgno <= 50; pgno++ )
    {
        const ULONG cTouches = ( 50 - pgno ) / 10;

        for ( ULONG iTouch = 0; iTouch < cTouches; iTouch++ )
        {
            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidTouch;
            BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
            pbftouch->ifmp = pgno % 2;
            pbftouch->pgno = pgno;
            pbftouch->pctPri = 100;
            pbftouch->fUseHistory = fTrue;

            tick += 10;
            iTrace++;
        }
    }


    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSuperCold;
    BFTRACE::BFSuperCold_* pbfsupercold = &rgbftrace[ iTrace ].bfsupercold;
    pbfsupercold->ifmp = 25 % 2;
    pbfsupercold->pgno = 25;

    tick += 10;
    iTrace++;


    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 35 % 2;
    pbfevict->pgno = 35;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonShrink;

    tick += 10;
    iTrace++;


    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 25 % 2;
    pbfevict->pgno = 25;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonAvailPool;

    tick += 10;
    iTrace++;

    for ( PGNO pgnoT = 41; pgnoT >= 1; )
    {
        for ( PGNO pgno = pgnoT; pgno < ( pgnoT + 10 ); pgno++ )
        {
            if ( ( pgno == 25 ) || ( pgno == 35 ) )
            {
                continue;
            }

            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidEvict;
            pbfevict = &rgbftrace[ iTrace ].bfevict;
            pbfevict->ifmp = pgno % 2;
            pbfevict->pgno = pgno;
            pbfevict->pctPri = 100;
            pbfevict->fCurrentVersion = fTrue;
            pbfevict->bfef = bfefReasonAvailPool;
 
            tick += 10;
            iTrace++;
            }

        if ( pgnoT == 1 )
        {
            break;
        }
        else
        {
            pgnoT -= 10;
        }
    }


    for ( PGNO pgno = 1; iTrace < 212; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = ( pgno != 5 );
        pbfcache->fNewPage = !pbfcache->fUseHistory;
        pgno++;
        tick += 10;
    }


    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 5 % 2;
    pbfevict->pgno = 5;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonAvailPool;

    tick += 10;
    iTrace++;

    for ( PGNO pgno = 1; pgno <= 10; pgno++ )
    {
        if ( pgno == 5 )
        {
            continue;
        }

        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;

        tick += 10;
        iTrace++;
    }


    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;


    rgbftrace[ iTrace ].traceid = bftidInvalid;
    iTrace++;

    TestCheck( iTrace == _countof( rgbftrace ) );

    const TICK tickEnd = tick;


    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );


    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 50;
    pbfftlc->rgpgnoMax[ 1 ] = 50;


    TestCall( emulator.ErrSetEvictNextOnShrink( false ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 50 );
    TestCheck( stats.cRequestedUnique == 50 );
    TestCheck( stats.cRequested == 160 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 59 );
    TestCheck( stats.cFaultsSim == 59 );
    TestCheck( stats.cTouchesReal == 100 );
    TestCheck( stats.cTouchesSim == 100 );
    TestCheck( stats.cCaches == 60 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cEvictionsReal == 60 );
    TestCheck( stats.cEvictionsSim == 60 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.cSuperColdedReal == 1 );
    TestCheck( stats.cSuperColdedSim == 1 );
    TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

