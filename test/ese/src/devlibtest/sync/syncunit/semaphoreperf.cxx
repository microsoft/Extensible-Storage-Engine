// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"
#include "sync.hxx"
#include <time.h>

//  Interface class.

class ISlimSemaphore
{
    protected:
        ISlimSemaphore( const LONG cInitial );

    public:
        static const DWORD cmsecDuration = 10000;

    public:
        virtual void Enter() = 0;
        virtual void Leave() = 0;
};

ISlimSemaphore::ISlimSemaphore( const LONG cInitial )
{
}


//  SlimSemaphoreBroken implementation.

class SlimSemaphoreBroken : public ISlimSemaphore
{
    public:
        SlimSemaphoreBroken( const LONG cInitial );
        void Enter();
        void Leave();
};

SlimSemaphoreBroken::SlimSemaphoreBroken( const LONG cInitial ) :
    ISlimSemaphore( cInitial )
{
}

void SlimSemaphoreBroken::Enter()
{
}

void SlimSemaphoreBroken::Leave()
{
}


//  SlimSemaphoreHeavy implementation.

class SlimSemaphoreHeavy : public ISlimSemaphore
{
    private:
        HANDLE m_hSemaphore;

    public:
        SlimSemaphoreHeavy( const LONG cInitial );
        ~SlimSemaphoreHeavy();
        void Enter();
        void Leave();
};

SlimSemaphoreHeavy::SlimSemaphoreHeavy( const LONG cInitial ) :
    ISlimSemaphore( cInitial ),
    m_hSemaphore( NULL )
{
    m_hSemaphore = CreateSemaphoreExW( NULL, cInitial, cInitial, NULL, 0, SEMAPHORE_ALL_ACCESS );
    TestAssert( m_hSemaphore != NULL );
}

SlimSemaphoreHeavy::~SlimSemaphoreHeavy()
{
    const DWORD dwClose = CloseHandle( m_hSemaphore );
    TestAssert( dwClose != 0 );
}

void SlimSemaphoreHeavy::Enter()
{
    WaitForSingleObject( m_hSemaphore, INFINITE );
}

void SlimSemaphoreHeavy::Leave()
{
    ReleaseSemaphore( m_hSemaphore, 1, NULL );
}


//  SlimSemaphoreLightSpin implementation.

class SlimSemaphoreLightSpin : public ISlimSemaphore
{
    private:
        HANDLE m_hEvent;
        volatile LONG m_cResources;
        LONG m_cInitial;
        volatile LONG m_critSec;
        volatile LONG m_fAck;

    private:
        inline void EnterCriticalSection_();
        inline void LeaveCriticalSection_();

    public:
        SlimSemaphoreLightSpin( const LONG cInitial );
        ~SlimSemaphoreLightSpin();
        void Enter();
        void Leave();
};

inline void SlimSemaphoreLightSpin::EnterCriticalSection_()
{
    while ( InterlockedCompareExchange( &m_critSec, 1, 0 ) == 1 )
    {
        Sleep( 0 );
    }
}

inline void SlimSemaphoreLightSpin::LeaveCriticalSection_()
{
    InterlockedExchange( &m_critSec, 0 );
}

SlimSemaphoreLightSpin::SlimSemaphoreLightSpin( const LONG cInitial ) :
    ISlimSemaphore( cInitial ),
    m_hEvent( NULL ),
    m_cResources( cInitial ),
    m_cInitial( cInitial ),
    m_critSec( 0 ),
    m_fAck( 0 )
{
    TestAssert( m_cInitial > 0 );
    m_hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
    TestAssert( m_hEvent != NULL );
}

SlimSemaphoreLightSpin::~SlimSemaphoreLightSpin()
{
    const DWORD dwClose = CloseHandle( m_hEvent );
    TestAssert( dwClose != 0 );
    TestAssert( m_cResources == m_cInitial );
    TestAssert( m_critSec == 0 );
    TestAssert( m_fAck == 0 );
}

void SlimSemaphoreLightSpin::Enter()
{
    if ( InterlockedDecrement( &m_cResources ) >= 0 )
    {
        return;
    }
    
    WaitForSingleObject( m_hEvent, INFINITE );

    InterlockedExchange( &m_fAck, 1 );
}

void SlimSemaphoreLightSpin::Leave()
{
    if ( InterlockedIncrement( &m_cResources ) > 0 )
    {
        return;
    }
    
    EnterCriticalSection_();
    SetEvent( m_hEvent );
    while ( InterlockedExchangeAdd( &m_fAck, 0 ) == 0 )
    {
        Sleep( 0 );
    }
    InterlockedExchange( &m_fAck, 0 );
    LeaveCriticalSection_();
}


//  SlimSemaphoreLightEvents implementation.

class SlimSemaphoreLightEvents : public ISlimSemaphore
{
    private:
        HANDLE m_hEvent;
        volatile LONG m_cResources;
        LONG m_cInitial;
        HANDLE m_hCritSec;
        HANDLE m_hAck;

    private:
        inline void EnterCriticalSection_();
        inline void LeaveCriticalSection_();

    public:
        SlimSemaphoreLightEvents( const LONG cInitial );
        ~SlimSemaphoreLightEvents();
        void Enter();
        void Leave();
};

inline void SlimSemaphoreLightEvents::EnterCriticalSection_()
{
    WaitForSingleObject( m_hCritSec, INFINITE );
}

inline void SlimSemaphoreLightEvents::LeaveCriticalSection_()
{
    SetEvent( m_hCritSec );
}

SlimSemaphoreLightEvents::SlimSemaphoreLightEvents( const LONG cInitial ) :
    ISlimSemaphore( cInitial ),
    m_hEvent( NULL ),
    m_cResources( cInitial ),
    m_cInitial( cInitial ),
    m_hCritSec( NULL ),
    m_hAck( NULL )
{
    TestAssert( m_cInitial > 0 );
    m_hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
    TestAssert( m_hEvent != NULL );
    m_hCritSec = CreateEventW( NULL, FALSE, TRUE, NULL );
    TestAssert( m_hCritSec != NULL );
    m_hAck = CreateEventW( NULL, FALSE, FALSE, NULL );
    TestAssert( m_hAck != NULL );
}

SlimSemaphoreLightEvents::~SlimSemaphoreLightEvents()
{
    DWORD dwClose = CloseHandle( m_hEvent );
    TestAssert( dwClose != 0 );
    TestAssert( m_cResources == m_cInitial );
    TestAssert( WaitForSingleObject( m_hCritSec, 0 ) == WAIT_OBJECT_0 );
    dwClose = CloseHandle( m_hCritSec );
    TestAssert( dwClose != 0 );
    TestAssert( WaitForSingleObject( m_hAck, 0 ) == WAIT_TIMEOUT );
    dwClose = CloseHandle( m_hAck );
    TestAssert( dwClose != 0 );
}

void SlimSemaphoreLightEvents::Enter()
{
    if ( InterlockedDecrement( &m_cResources ) >= 0 )
    {
        return;
    }
    
    WaitForSingleObject( m_hEvent, INFINITE );
    SetEvent( m_hAck );
}

void SlimSemaphoreLightEvents::Leave()
{
    if ( InterlockedIncrement( &m_cResources ) > 0 )
    {
        return;
    }
    
    EnterCriticalSection_();
    SetEvent( m_hEvent );
    WaitForSingleObject( m_hAck, INFINITE );
    LeaveCriticalSection_();
}


//  SlimSemaphoreLightKSemaphore implementation.

class SlimSemaphoreLightKSemaphore : public ISlimSemaphore
{
    private:
        volatile LONG m_cResources;
        LONG m_cInitial;
        HANDLE m_hKSemaphore;

    public:
        SlimSemaphoreLightKSemaphore( const LONG cInitial );
        ~SlimSemaphoreLightKSemaphore();
        void Enter();
        void Leave();
};

SlimSemaphoreLightKSemaphore::SlimSemaphoreLightKSemaphore( const LONG cInitial ) :
    ISlimSemaphore( cInitial ),
    m_cResources( cInitial ),
    m_cInitial( cInitial ),
    m_hKSemaphore( NULL )
{
    TestAssert( m_cInitial > 0 );
    m_hKSemaphore = CreateSemaphoreExW( NULL, 0, cInitial, NULL, 0, SEMAPHORE_ALL_ACCESS );
    TestAssert( m_hKSemaphore != NULL );
}

SlimSemaphoreLightKSemaphore::~SlimSemaphoreLightKSemaphore()
{
    TestAssert( m_cResources == m_cInitial );
    TestAssert( WaitForSingleObject( m_hKSemaphore, 0 ) == WAIT_TIMEOUT );
    const DWORD dwClose = CloseHandle( m_hKSemaphore );
    TestAssert( dwClose != 0 );
}

void SlimSemaphoreLightKSemaphore::Enter()
{
    if ( InterlockedDecrement( &m_cResources ) >= 0 )
    {
        return;
    }
    
    WaitForSingleObject( m_hKSemaphore, INFINITE );
}

void SlimSemaphoreLightKSemaphore::Leave()
{
    if ( InterlockedIncrement( &m_cResources ) > 0 )
    {
        return;
    }

    ReleaseSemaphore( m_hKSemaphore, 1, NULL );
}


//  SlimSemaphoreLightEseCSemaphore implementation.

class SlimSemaphoreLightEseCSemaphore : public ISlimSemaphore
{
    private:
        LONG m_cInitial;
        CSemaphore m_eseCSemaphore;

    public:
        SlimSemaphoreLightEseCSemaphore( const LONG cInitial );
        ~SlimSemaphoreLightEseCSemaphore();
        void Enter();
        void Leave();
};

SlimSemaphoreLightEseCSemaphore::SlimSemaphoreLightEseCSemaphore( const LONG cInitial ) :
    ISlimSemaphore( cInitial ),
    m_cInitial( cInitial ),
    m_eseCSemaphore( CSyncBasicInfo( "SlimSemaphoreLightEseCSemaphore" ) )
{
    TestAssert( m_cInitial > 0 );
    m_eseCSemaphore.Release( m_cInitial );
}

SlimSemaphoreLightEseCSemaphore::~SlimSemaphoreLightEseCSemaphore()
{
    TestAssert( m_eseCSemaphore.CAvail() == m_cInitial );
}

void SlimSemaphoreLightEseCSemaphore::Enter()
{
    m_eseCSemaphore.Acquire();
}

void SlimSemaphoreLightEseCSemaphore::Leave()
{
    m_eseCSemaphore.Release();
}


//  ISlimSemaphore factory.
class SlimSemaphoreFactory
{
    public:
        typedef enum
        {
            smtypBroken,
            smtypHeavy,
            smtypLightSpin,
            smtypLightEvents,
            smtypLightKSemaphore,
            smtypLightEseCSemaphore
        }
        SlimSemaphoreType;

    public:
        static ISlimSemaphore* Create( const SlimSemaphoreFactory::SlimSemaphoreType smtyp, const LONG cInitial );
        static void Destroy( ISlimSemaphore* const pISlimSemaphore );
};

ISlimSemaphore* SlimSemaphoreFactory::Create( const SlimSemaphoreFactory::SlimSemaphoreType smtyp, const LONG cInitial )
{
    switch ( smtyp )
    {
        case SlimSemaphoreFactory::smtypBroken:
            return new SlimSemaphoreBroken( cInitial );

        case SlimSemaphoreFactory::smtypHeavy:
            return new SlimSemaphoreHeavy( cInitial );

        case SlimSemaphoreFactory::smtypLightSpin:
            return new SlimSemaphoreLightSpin( cInitial );

        case SlimSemaphoreFactory::smtypLightEvents:
            return new SlimSemaphoreLightEvents( cInitial );

        case SlimSemaphoreFactory::smtypLightKSemaphore:
            return new SlimSemaphoreLightKSemaphore( cInitial );

        case SlimSemaphoreFactory::smtypLightEseCSemaphore:
            return new SlimSemaphoreLightEseCSemaphore( cInitial );

        default:
            TestAssertSz( false, "Unrecognized SlimSemaphoreType!" );
    }
}

void SlimSemaphoreFactory::Destroy( ISlimSemaphore* const pISlimSemaphore )
{
    delete pISlimSemaphore;
}


//  Test class.

class SlimSemaphoreTest
{
    private:
        ISlimSemaphore* m_pSemaphore;
        HANDLE* m_hThreads;
        LONG m_cInitial;
        LONG m_cParallelThreads;
        DWORD m_cmsecRuntime;
        volatile LONG m_lResource;
        volatile LONGLONG m_cResources;
        volatile LONGLONG m_cLessThanMin;
        volatile LONGLONG m_cMin;
        volatile LONGLONG m_cMax;
        volatile LONGLONG m_cGreaterThanMax;
        bool m_fSuceeded;
        double m_dblThroughput;
        double m_dblKernelTime;
        double m_dblUserTime;
        double m_dblTotalTime;
        volatile LONG m_fShouldRun;

    private:
        static ULONGLONG TickFromFileTime_( const FILETIME ft );
        static ULONGLONG DTickFromFileTimes_( const FILETIME ft1, const FILETIME ft2 );
        static DWORD WINAPI ConsumerThread_( LPVOID pvContext );
        void ConsumeResource_();
        void ReleaseResource_();

    public:
        SlimSemaphoreTest( const SlimSemaphoreFactory::SlimSemaphoreType smtyp, const LONG cInitial, const LONG cParallelThreads, const DWORD cmsecRuntime );
        ~SlimSemaphoreTest();
        bool FSuceeded( double* const pdblThroughput, double* const pdblKernelTime, double* const pdblUserTime, double* const pdblTotalTime ) const;
};

ULONGLONG SlimSemaphoreTest::TickFromFileTime_( const FILETIME ft )
{
    ULARGE_INTEGER tick;
    
    tick.u.LowPart = ft.dwLowDateTime;
    tick.u.HighPart = ft.dwHighDateTime;
    
    return tick.QuadPart;
}
    
ULONGLONG SlimSemaphoreTest::DTickFromFileTimes_( const FILETIME ft1, const FILETIME ft2 )
{
    return TickFromFileTime_( ft2 ) - TickFromFileTime_( ft1 );
}

DWORD WINAPI SlimSemaphoreTest::ConsumerThread_( LPVOID pvContext )
{ 
    SlimSemaphoreTest* const pSemaphore = (SlimSemaphoreTest*)pvContext;
    
    srand( (unsigned)time(NULL) );
    while ( InterlockedExchangeAdd( &pSemaphore->m_fShouldRun, 0 ) != 0 )
        {
        pSemaphore->m_pSemaphore->Enter();      
        pSemaphore->ConsumeResource_();
        Sleep( rand() % 2 );
        pSemaphore->ReleaseResource_();
        pSemaphore->m_pSemaphore->Leave();
    }
    
    return 0;
}
    
void SlimSemaphoreTest::ConsumeResource_()
{
    const LONG lResource = InterlockedDecrement( &m_lResource );
    if ( lResource == 0 )
    {
        InterlockedIncrement64( &m_cMin );
    }
    else if ( lResource < 0 )
    {
        InterlockedIncrement64( &m_cLessThanMin );
    }
}

void SlimSemaphoreTest::ReleaseResource_()
{
    const LONG lResource = InterlockedIncrement( &m_lResource );
    if ( lResource == m_cInitial )
    {
        InterlockedIncrement64( &m_cMax );
    }
    else if ( lResource > m_cInitial )
    {
        InterlockedIncrement64( &m_cGreaterThanMax );
    }
    InterlockedIncrement64( &m_cResources );
}

SlimSemaphoreTest::SlimSemaphoreTest( const SlimSemaphoreFactory::SlimSemaphoreType smtyp, const LONG cInitial, const LONG cParallelThreads, const DWORD cmsecRuntime ) : 
    m_pSemaphore( NULL ),
    m_hThreads( NULL ),
    m_cInitial ( cInitial ),
    m_cParallelThreads( cParallelThreads ),
    m_cmsecRuntime( cmsecRuntime),
    m_lResource( m_cInitial ),
    m_cResources( 0 ),
    m_cLessThanMin( 0 ),
    m_cMin( 0 ),
    m_cMax( 0 ),
    m_cGreaterThanMax( 0 ),
    m_fSuceeded( false ),
    m_dblThroughput( 0.0 ),
    m_dblKernelTime( 0.0 ),
    m_dblUserTime( 0.0 ),
    m_dblTotalTime( 0.0 ),
    m_fShouldRun( 0 )
{
    m_pSemaphore = SlimSemaphoreFactory::Create( smtyp, cInitial );
    TestAssert( m_pSemaphore != NULL );
    m_hThreads = new HANDLE[cParallelThreads];
    TestAssert( m_hThreads != NULL );

    InterlockedExchange( &m_fShouldRun, 1 );

    for ( LONG iThread = 0 ; iThread < cParallelThreads ; iThread++ )
    {
        m_hThreads[iThread] = CreateThread( NULL, 0, SlimSemaphoreTest::ConsumerThread_, this, 0, NULL );
        TestAssert( m_hThreads[iThread] != NULL );
    }
    
    FILETIME ftDummy;
    FILETIME  ftKernelTime1, ftUserTime1;
    FILETIME  ftKernelTime2, ftUserTime2;
    const HANDLE hProcess = GetCurrentProcess();
    TestAssert( GetProcessTimes( hProcess, &ftDummy, &ftDummy, &ftKernelTime1, &ftUserTime1 ) != 0 );
    Sleep( m_cmsecRuntime );
    TestAssert( GetProcessTimes( hProcess, &ftDummy, &ftDummy, &ftKernelTime2, &ftUserTime2 ) != 0 );
    m_dblKernelTime = ( 100.0 * (double)DTickFromFileTimes_( ftKernelTime1, ftKernelTime2 ) ) / (double)m_cResources;
    m_dblUserTime = ( 100.0 * (double)DTickFromFileTimes_( ftUserTime1, ftUserTime2 ) ) / (double)m_cResources;
    m_dblTotalTime = m_dblKernelTime + m_dblUserTime;

    InterlockedExchange( &m_fShouldRun, 0 );

    for ( LONG iThread = 0 ; iThread < cParallelThreads ; iThread++ )
    {
        const DWORD dwWait = WaitForSingleObject( m_hThreads[iThread], INFINITE );
        TestAssert( dwWait == WAIT_OBJECT_0 );
        const DWORD dwClose = CloseHandle( m_hThreads[iThread] );
        TestAssert( dwClose != 0 );
    }

    m_fSuceeded = ( m_lResource == m_cInitial ) &&
                    ( m_cResources > 0 ) &&
                    ( m_cLessThanMin == 0 ) &&
                    ( m_cMin > 0 ) &&
                    ( m_cMax > 0 ) &&
                    ( m_cGreaterThanMax == 0 );
    
    m_dblThroughput = ( (double)m_cResources / (double)m_cParallelThreads ) / 1000000.0;

    wprintf( L"      m_lResource = %ld\n", m_lResource );
    wprintf( L"     m_cResources = %I64d\n", m_cResources );
    wprintf( L"   m_cLessThanMin = %I64d\n", m_cLessThanMin );
    wprintf( L"           m_cMin = %I64d\n", m_cMin );
    wprintf( L"           m_cMax = %I64d\n", m_cMax );
    wprintf( L"m_cGreaterThanMax = %I64d\n", m_cGreaterThanMax );
    wprintf( L"       Throughput = %.6f Mops/thread\n", m_dblThroughput );
    wprintf( L"  m_dblKernelTime = %.3f nsec/op\n", m_dblKernelTime );
    wprintf( L"    m_dblUserTime = %.3f nsec/op\n", m_dblUserTime );
    wprintf( L"   m_dblTotalTime = %.3f nsec/op\n", m_dblTotalTime );
}

SlimSemaphoreTest::~SlimSemaphoreTest()
{
    delete[] m_hThreads;
    SlimSemaphoreFactory::Destroy( m_pSemaphore );
}

bool SlimSemaphoreTest::FSuceeded( double* const pdblThroughput, double* const pdblKernelTime, double* const pdblUserTime, double* const pdblTotalTime ) const
{
    TestAssert( pdblThroughput != NULL );
    TestAssert( pdblKernelTime != NULL );
    TestAssert( pdblUserTime != NULL );
    TestAssert( pdblTotalTime != NULL );
    *pdblThroughput = m_dblThroughput;
    *pdblKernelTime = m_dblKernelTime;
    *pdblUserTime = m_dblUserTime;
    *pdblTotalTime = m_dblTotalTime;

    return m_fSuceeded;
}


//  Helper functions.

bool FRunTest( const SlimSemaphoreFactory::SlimSemaphoreType smtyp, const LONG cInitial, const LONG cParallelThreads, const DWORD cmsecRuntime, double* const pdblThroughput, double* const pdblKernelTime, double* const pdblUserTime, double* const pdblTotalTime )
{
    wprintf( L"Starting test: %ld resource(s), %ld parallel thread(s), %lu msec...\n", cInitial, cParallelThreads, cmsecRuntime );
    SlimSemaphoreTest test( smtyp, cInitial, cParallelThreads, cmsecRuntime );

    double dblThroughput, dblKernelTime, dblUserTime, dblTotalTime;
    const bool fPassed = test.FSuceeded( &dblThroughput, &dblKernelTime, &dblUserTime, &dblTotalTime );
    wprintf( L"Test %ls!!!\n", fPassed ? L"PASSED" : L"FAILED" );

    *pdblThroughput = dblThroughput;
    *pdblKernelTime = dblKernelTime;
    *pdblUserTime = dblUserTime;
    *pdblTotalTime = dblTotalTime;

    return fPassed;
}


bool FRunTests( const SlimSemaphoreFactory::SlimSemaphoreType smtyp, const bool fExpected )
{
    const INT cTest = 5;
    double* rgThroughput = new double[cTest];
    double* rgKernelTime = new double[cTest];
    double* rgUserTime = new double[cTest];
    double* rgTotalTime = new double[cTest];
    INT iTest = 0;
    bool fPassed = true;
    SYSTEM_INFO sysinfo ; 
    GetSystemInfo( &sysinfo ); 
    const UINT inumCores = sysinfo.dwNumberOfProcessors; 
        
    wprintf( L"\nRunning tests for: %d on: %d cores\n", (INT)smtyp, inumCores);
    fPassed = FRunTest( smtyp, 2*inumCores, 2*inumCores, ISlimSemaphore::cmsecDuration, &( rgThroughput[iTest] ), &( rgKernelTime[iTest] ), &( rgUserTime[iTest] ), &( rgTotalTime[iTest] ) ) && fPassed;
    iTest++;
    fPassed = FRunTest( smtyp, 2*inumCores, 3*inumCores, ISlimSemaphore::cmsecDuration, &( rgThroughput[iTest] ), &( rgKernelTime[iTest] ), &( rgUserTime[iTest] ), &( rgTotalTime[iTest] ) ) && fPassed;
    iTest++;
    fPassed = FRunTest( smtyp, 2*inumCores, 20*inumCores, ISlimSemaphore::cmsecDuration, &( rgThroughput[iTest] ), &( rgKernelTime[iTest] ), &( rgUserTime[iTest] ), &( rgTotalTime[iTest] ) ) && fPassed;
    iTest++;
    TestAssert( iTest <= cTest );

    wprintf( L"\nSummary:\n" );
    wprintf( L"Throughput: " );
    for ( INT iTestT = 0 ; iTestT < iTest ; iTestT++ )
    {
        wprintf( L"%.6f  ", rgThroughput[iTestT] );
    }
    wprintf( L"\n" );
    wprintf( L"Kernel time: " );
    for ( INT iTestT = 0 ; iTestT < iTest ; iTestT++ )
    {
        wprintf( L"%.3f  ", rgKernelTime[iTestT] );
    }
    wprintf( L"\n" );
    wprintf( L"User time: " );
    for ( INT iTestT = 0 ; iTestT < iTest ; iTestT++ )
    {
        wprintf( L"%.3f  ", rgUserTime[iTestT] );
    }
    wprintf( L"\n" );
    wprintf( L"Total time: " );
    for ( INT iTestT = 0 ; iTestT < iTest ; iTestT++ )
    {
        wprintf( L"%.3f  ", rgTotalTime[iTestT] );
    }
    wprintf( L"\n" );
    wprintf( L"Expected: %d\n", fExpected );
    wprintf( L"Returned: %d\n\n", fPassed );

    delete[] rgThroughput;
    delete[] rgKernelTime;
    delete[] rgUserTime;
    delete[] rgTotalTime;

    return fPassed == fExpected;
}

//  Test fixture.

class CSemaphorePerfTest : public UNITTEST
{
    private:
        static CSemaphorePerfTest s_instance;

    protected:
        CSemaphorePerfTest()
        {
        }

    public:
        ~CSemaphorePerfTest()
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

CSemaphorePerfTest CSemaphorePerfTest::s_instance;

const char * CSemaphorePerfTest::SzName() const
{
    return "CSemaphoreFunctionalAndPerformance";
};

const char * CSemaphorePerfTest::SzDescription() const
{
    return "Validates the CSemaphore synchronization object (multi-threaded) and tests for performance.";
}

bool CSemaphorePerfTest::FRunUnderESE98() const
{
    return true;
}

bool CSemaphorePerfTest::FRunUnderESENT() const
{
    return true;
}

bool CSemaphorePerfTest::FRunUnderESE97() const
{
    return true;
}

ERR CSemaphorePerfTest::ErrTest()
{
    const BOOL fOsSyncInit = FOSSyncPreinit();

    TestAssert( fOsSyncInit );

    wprintf( L"\tTesting semaphore performance...\n" );
//  TestAssert( FRunTests( SlimSemaphoreFactory::smtypBroken, false ) );
//  TestAssert( FRunTests( SlimSemaphoreFactory::smtypHeavy, true ) );
//  TestAssert( FRunTests( SlimSemaphoreFactory::smtypLightSpin, true ) );
//  TestAssert( FRunTests( SlimSemaphoreFactory::smtypLightEvents, true ) );
//  TestAssert( FRunTests( SlimSemaphoreFactory::smtypLightKSemaphore, true ) );
    TestAssert( FRunTests( SlimSemaphoreFactory::smtypLightEseCSemaphore, true ) );

    OSSyncPostterm();

    return JET_errSuccess;
}


class CSemaphoreFairnessTest : public UNITTEST
{
private:
    static CSemaphoreFairnessTest s_instance;

public:
    const char * SzName() const;
    const char * SzDescription() const;
    bool FRunUnderESE98() const;
    bool FRunUnderESENT() const;
    bool FRunUnderESE97() const;
    ERR ErrTest();
    void TestCase( const LONG cThreads );
};

CSemaphoreFairnessTest CSemaphoreFairnessTest::s_instance;

const char * CSemaphoreFairnessTest::SzName() const
{
    return "CSemaphoreFairnessTest";
};

const char * CSemaphoreFairnessTest::SzDescription() const
{
    return "Tests the CSemaphore for fairness.";
}

bool CSemaphoreFairnessTest::FRunUnderESE98() const
{
    return true;
}

bool CSemaphoreFairnessTest::FRunUnderESENT() const
{
    return true;
}

bool CSemaphoreFairnessTest::FRunUnderESE97() const
{
    return true;
}

struct CSemaphoreFairnessTestContext
{
    HANDLE hThread;
    CSemaphore *pSemaphore;
    BOOL bAggressive;
    volatile ULONG *pulExit;
    volatile ULONG ulAcquire;
};

static DWORD WINAPI FairnessTestThread( LPVOID pvContext )
{
    CSemaphoreFairnessTestContext *pContext = (CSemaphoreFairnessTestContext*)pvContext;

    while ( !InterlockedCompareExchange( pContext->pulExit, 0, 0 ) )
    {
        if ( pContext->bAggressive )
        {
            pContext->pSemaphore->Acquire();
            InterlockedIncrement( &pContext->ulAcquire );
            Sleep(1);
            pContext->pSemaphore->Release();
        }
        else
        {
            Sleep(1);
            pContext->pSemaphore->Acquire();
            InterlockedIncrement( &pContext->ulAcquire );
            pContext->pSemaphore->Release();
        }
    }

    return ERROR_SUCCESS;
}

ERR CSemaphoreFairnessTest::ErrTest()
{
    TestAssert( FOSSyncPreinit() );

    TestCase( 2 );
    TestCase( 3 );
    TestCase( 4 );
    TestCase( 6 );
    TestCase( 8 );

    OSSyncPostterm();

    return JET_errSuccess;
}

void CSemaphoreFairnessTest::TestCase( const LONG cThreads )
{
    CSemaphore semaphore( CSyncBasicInfo( "CSemaphoreFairnessTest" ) );
    volatile ULONG ulExit = 0;

    CSemaphoreFairnessTestContext *pContexts = new CSemaphoreFairnessTestContext[cThreads];
    for ( LONG i = 0; i < cThreads; i++ )
    {
        CSemaphoreFairnessTestContext *pContext = &pContexts[i];

        memset( pContext, 0, sizeof(*pContext) );
        pContext->pSemaphore = &semaphore;
        pContext->pulExit = &ulExit;
        pContext->bAggressive = i % 2;
        pContext->hThread = CreateThread( NULL, 0, FairnessTestThread, pContext, 0, NULL );
        TestAssert( pContext->hThread );
    }

    semaphore.Release( 1 );
    Sleep( 3 * 1000 );
    InterlockedExchange( &ulExit, 1 );

    ULONG ulAcquireMin = ULONG_MAX;
    ULONG ulAcquireMax = 0;
    for ( LONG i = 0; i < cThreads; i++ )
    {
        CSemaphoreFairnessTestContext *pContext = &pContexts[i];

        TestAssert( WaitForSingleObject( pContext->hThread, INFINITE ) == WAIT_OBJECT_0 );
        TestAssert( CloseHandle( pContext->hThread ) );

        if ( pContext->ulAcquire < ulAcquireMin )
        {
            ulAcquireMin = pContext->ulAcquire;
        }

        if ( pContext->ulAcquire > ulAcquireMax )
        {
            ulAcquireMax = pContext->ulAcquire;
        }

        wprintf( L"\t%lu", pContext->ulAcquire );
    }

    wprintf( L"\n" );

    TestAssert( ulAcquireMax < ulAcquireMin * 4 );

    delete[] pContexts;
}


