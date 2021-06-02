// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"

//  ================================================================
class BasicInitTermLockTest : public UNITTEST
//  ================================================================
{
    private:
        static BasicInitTermLockTest s_instance;

    protected:
        BasicInitTermLockTest() {}
    public:
        ~BasicInitTermLockTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

BasicInitTermLockTest BasicInitTermLockTest::s_instance;

const char * BasicInitTermLockTest::SzName() const      { return "BasicCInitTermLock"; };
const char * BasicInitTermLockTest::SzDescription() const   { return 
            "Tests the synchronization libraries' CInitTermLock in a basic way (i.e. single threaded).";
        }
bool BasicInitTermLockTest::FRunUnderESE98() const      { return true; }
bool BasicInitTermLockTest::FRunUnderESENT() const      { return true; }
bool BasicInitTermLockTest::FRunUnderESE97() const      { return true; }


#include "sync.hxx"


CInitTermLock   g_clock;


//  ================================================================
ERR BasicInitTermLockTest::ErrTest()
//  ================================================================
{

    wprintf( L"\tTesting basic CInitTermLock() support ...\n");

    ERR err = JET_errSuccess;
    
    //  test trying to init, and failing to finish

    TestCheck( CInitTermLock::ERR::errInitBegun == g_clock.ErrInitBegin() );
    g_clock.InitFinish( CInitTermLock::fFailedInit );

    //  test init (and succeeding init)

    TestCheck( CInitTermLock::ERR::errInitBegun == g_clock.ErrInitBegin() );
    g_clock.InitFinish( CInitTermLock::fSuccessfulInit);
    TestCheck( g_clock.CConsumers() == 1 ); 

    //  test ref count, skipped init

    TestCheck( CInitTermLock::ERR::errSuccess == g_clock.ErrInitBegin() );
    TestCheck( g_clock.CConsumers() == 2 );

    //  test ref count, skipped term

    TestCheck( CInitTermLock::ERR::errSuccess == g_clock.ErrTermBegin() );
    TestCheck( g_clock.CConsumers() == 1 );

    //  test term

    TestCheck( CInitTermLock::ERR::errTermBegun == g_clock.ErrTermBegin() );
    g_clock.TermFinish();

    return JET_errSuccess;  

HandleError:

    wprintf(L"Failed test!\n");

    return err;
}


