// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"

class LeakDetectionTest : public UNITTEST
{
    private:
        static LeakDetectionTest s_instance;

    protected:
        LeakDetectionTest() {}
    public:
        ~LeakDetectionTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

LeakDetectionTest LeakDetectionTest::s_instance;

const char * LeakDetectionTest::SzName() const      { return "SyncOwnershipLeakDetection"; };
const char * LeakDetectionTest::SzDescription() const   { return 
            "Tests the synchronization libraries' capability to detect a leaked lock.";
        }
bool LeakDetectionTest::FRunUnderESE98() const      { return true; }
bool LeakDetectionTest::FRunUnderESENT() const      { return true; }
bool LeakDetectionTest::FRunUnderESE97() const      { return true; }


#include "sync.hxx"


class TestClassWithLock
{

    public:
        TestClassWithLock() :
            m_testLock( CLockBasicInfo( CSyncBasicInfo( "TestClassWithLock::m_testLock" ), 0, 0 ) )
        { 
        };
            
        ~TestClassWithLock() { };
        
    public:
        CCriticalSection        m_testLock;
};

ERR LeakDetectionTest::ErrTest()
{
    TestClassWithLock * pTestClass = NULL;

#ifdef SYNC_DEADLOCK_DETECTION
    wprintf( L"\tTesting detecting leave from a missing unlock ...\n");

    SetExpectedAsserts( 1 );
    
    pTestClass = new TestClassWithLock();

    pTestClass->m_testLock.Enter();

    delete pTestClass;

    AssertExpectedAsserts();
#endif 
    wprintf( L"\tTesting detection will not fire for unused lock ...\n");

    SetExpectedAsserts( 0 );
    
    pTestClass = new TestClassWithLock();

    delete pTestClass;

    AssertExpectedAsserts();

    wprintf( L"\tTesting detection will not fire for properly used lock ...\n");

    SetExpectedAsserts( 0 );
    
    pTestClass = new TestClassWithLock();

    pTestClass->m_testLock.Enter();
    pTestClass->m_testLock.Leave();

    delete pTestClass;

    AssertExpectedAsserts();

    ResetExpectedAsserts();

    return JET_errSuccess;
}


