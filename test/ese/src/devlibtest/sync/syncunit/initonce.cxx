// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"

class BasicInitOnceTest : public UNITTEST
{
    private:
        static BasicInitOnceTest s_instance;

    protected:
        BasicInitOnceTest() {}
    public:
        ~BasicInitOnceTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

BasicInitOnceTest BasicInitOnceTest::s_instance;

const char * BasicInitOnceTest::SzName() const      { return "BasicCInitOnce"; };
const char * BasicInitOnceTest::SzDescription() const   { return 
            "Tests the synchronization libraries' CInitOnce in a basic way (i.e. single threaded).";
        }
bool BasicInitOnceTest::FRunUnderESE98() const      { return true; }
bool BasicInitOnceTest::FRunUnderESENT() const      { return true; }
bool BasicInitOnceTest::FRunUnderESE97() const      { return true; }


#include "sync.hxx"

INT g_cInitCalled = 0;
INT InitFunction( INT value )
{
    g_cInitCalled++;
    return -value;
}

ERR BasicInitOnceTest::ErrTest()
{
    ERR err = JET_errSuccess;
    wprintf( L"\tTesting basic CInitOnce() support ...\n");
    CInitOnce< INT, decltype(&InitFunction), INT > initOnce;

    TestCheck( !initOnce.FIsInit() );

    TestCheck( initOnce.Init( InitFunction, 1 ) == -1 );
    TestCheck( initOnce.Init( InitFunction, 2 ) == -1 );
    TestCheck( initOnce.Init( InitFunction, 3 ) == -1 );
    TestCheck( g_cInitCalled == 1 );
    TestCheck( initOnce.FIsInit() );

    initOnce.Reset();
    TestCheck( !initOnce.FIsInit() );
    g_cInitCalled = 0;

    TestCheck( initOnce.Init( InitFunction, 3 ) == -3 );
    TestCheck( initOnce.Init( InitFunction, 2 ) == -3 );
    TestCheck( initOnce.Init( InitFunction, 1 ) == -3 );

    initOnce.Reset();
    g_cInitCalled = 0;
    initOnce.Reset();

    TestCheck( initOnce.Init( InitFunction, 2 ) == -2 );
    TestCheck( initOnce.Init( InitFunction, 3 ) == -2 );
    TestCheck( initOnce.Init( InitFunction, 1 ) == -2 );

    initOnce.Reset();
    g_cInitCalled = 0;

    return JET_errSuccess;  

HandleError:

    wprintf(L"Failed test!\n");

    return err;
}


