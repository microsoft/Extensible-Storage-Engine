// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"
#include "sync.hxx"


class AtomicAddTest
{
    private:
        struct ADD_THREAD_DATA
        {
            volatile QWORD * pqwValue;
            LONG cAdditions;
            QWORD qwAddition;
        };

        HANDLE* m_hThreads;
        ADD_THREAD_DATA * m_rgThreadData;
        
        LONG m_cParallelThreads;
        bool m_fSuceeded;
        volatile QWORD m_qwValue;
        LONG m_cAdditions;
        
    private:
        static DWORD WINAPI AddThread_( LPVOID pvContext );
        bool FSuceeded() const;

    public:

        static bool FRunTest(); 

        AtomicAddTest( const QWORD qwValue, const LONG cAddition, const LONG cParallelThreads );
        ~AtomicAddTest();
};

bool AtomicAddTest::FRunTest()
{
    AtomicAddTest atomicAddTest( 666, 7000, 5 );

    return atomicAddTest.FSuceeded();
}

DWORD WINAPI AtomicAddTest::AddThread_( LPVOID pvContext )
{
    ADD_THREAD_DATA * pData = (ADD_THREAD_DATA*)pvContext;

    for ( LONG i = 0; i < pData->cAdditions; ++i )
    {
        AtomicAdd( (QWORD *const)pData->pqwValue, pData->qwAddition );
    }

    return 0;
}

AtomicAddTest::AtomicAddTest( const QWORD qwValue, const LONG cAdditions, const LONG cParallelThreads ) : 
    m_hThreads( NULL ),
    m_cParallelThreads( cParallelThreads ),
    m_fSuceeded( false ),
    m_cAdditions( cAdditions ),
    m_qwValue( qwValue )
{
    QWORD qwExpectedValue = m_qwValue;
    
    m_hThreads = new HANDLE[cParallelThreads];
    TestAssert( m_hThreads != NULL );

    m_rgThreadData = new ADD_THREAD_DATA[cParallelThreads];
    TestAssert( m_rgThreadData != NULL );

    for ( LONG iThread = 0 ; iThread < cParallelThreads ; iThread++ )
    {
        m_rgThreadData[iThread].cAdditions = m_cAdditions;
        m_rgThreadData[iThread].pqwValue = &m_qwValue;
        m_rgThreadData[iThread].qwAddition = ( iThread * 0x100000001 );
        
        m_hThreads[iThread] = CreateThread( NULL, 0, AtomicAddTest::AddThread_, &(m_rgThreadData[iThread]), 0, NULL );
        TestAssert( m_hThreads[iThread] != NULL );

        qwExpectedValue += ( iThread * 0x100000001 * cAdditions );
    }

    for ( LONG iThread = 0 ; iThread < cParallelThreads ; iThread++ )
    {
        const DWORD dwWait = WaitForSingleObject( m_hThreads[iThread], INFINITE );
        TestAssert( dwWait == WAIT_OBJECT_0 );
        const DWORD dwClose = CloseHandle( m_hThreads[iThread] );
        TestAssert( dwClose != 0 );
    }

    m_fSuceeded = ( qwExpectedValue == m_qwValue );
}

AtomicAddTest::~AtomicAddTest()
{
    delete[] m_hThreads;
    m_hThreads = NULL;
    delete[] m_rgThreadData;
    m_rgThreadData = NULL;
}

bool AtomicAddTest::FSuceeded() const
{
    return m_fSuceeded;
}


class CAtomicsMultiThreadedTest : public UNITTEST
{
    private:
        static CAtomicsMultiThreadedTest s_instance;

    protected:
        CAtomicsMultiThreadedTest()
        {
        }

    public:
        ~CAtomicsMultiThreadedTest()
        {
        }

    public:
        const char * SzName() const;
        const char * SzDescription() const;
        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;
        ERR ErrTest();
};

CAtomicsMultiThreadedTest CAtomicsMultiThreadedTest::s_instance;

const char * CAtomicsMultiThreadedTest::SzName() const
{
    return "AtomicsFunctionalAndPerformance";
};

const char * CAtomicsMultiThreadedTest::SzDescription() const
{
    return "Validates the Atomics primitives in a multi-threaded.";
}

bool CAtomicsMultiThreadedTest::FRunUnderESE98() const
{
    return true;
}

bool CAtomicsMultiThreadedTest::FRunUnderESENT() const
{
    return true;
}

bool CAtomicsMultiThreadedTest::FRunUnderESE97() const
{
    return true;
}

ERR CAtomicsMultiThreadedTest::ErrTest()
{
    const BOOL fOsSyncInit = FOSSyncPreinit();

    TestAssert( fOsSyncInit );

    wprintf( L"\tTesting AtomicAdd multi-threaded functionality...\n" );

    TestAssert( AtomicAddTest::FRunTest() );

    OSSyncPostterm();

    return JET_errSuccess;
}

