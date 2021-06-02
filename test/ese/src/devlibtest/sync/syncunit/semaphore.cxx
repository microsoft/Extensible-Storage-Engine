// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"
#include "sync.hxx"

// --------------------------------------------------------------------------
//
//  Helpers
//

#define SyncBasicTestInit               \
    ERR err = JET_errSuccess;           \
    const BOOL fOsSyncInit = FOSSyncPreinit();  \
    TestAssert( fOsSyncInit );

// Note: Technically not needed right now, but be a good citizen
#define SyncBasicTestTerm               \
    OSSyncPostterm();


DWORD WINAPI AcquireForever( LPVOID lpParameter )
{
    CSemaphore* const pSemaphore = (CSemaphore*)lpParameter;

    pSemaphore->Acquire();

    return ERROR_SUCCESS;
}

DWORD WINAPI WaitForever( LPVOID lpParameter )
{
    CSemaphore* const pSemaphore = (CSemaphore*)lpParameter;

    pSemaphore->Wait();

    return ERROR_SUCCESS;
}

DWORD WINAPI AcquireForeverSpin( LPVOID lpParameter )
{
    CSemaphore* const pSemaphore = (CSemaphore*)lpParameter;

    while ( !pSemaphore->FAcquire( 0 ) );

    return ERROR_SUCCESS;
}

DWORD WINAPI WaitForeverSpin( LPVOID lpParameter )
{
    CSemaphore* const pSemaphore = (CSemaphore*)lpParameter;

    while ( !pSemaphore->FWait( 0 ) );

    return ERROR_SUCCESS;
}

static DWORD g_dwTimeout = 0;

DWORD WINAPI AcquireOneTimeout( LPVOID lpParameter )
{
    CSemaphore* const pSemaphore = (CSemaphore*)lpParameter;

    return pSemaphore->FAcquire( g_dwTimeout );
}

DWORD WINAPI WaitOneTimeout( LPVOID lpParameter )
{
    CSemaphore* const pSemaphore = (CSemaphore*)lpParameter;

    return pSemaphore->FWait( g_dwTimeout );
}

// --------------------------------------------------------------------------
//
//  Tests
//

CUnitTest( SyncSemaphoreConcurrentlyCorrectCWaitAndAvail, 0x0, "" );
ERR SyncSemaphoreConcurrentlyCorrectCWaitAndAvail::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    psemaphore->Release( 2 );
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 2 );

    psemaphore->Acquire();
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 1 );

    HANDLE rghThreads[2];
    rghThreads[0] = CreateThread( NULL, 0, AcquireForever, psemaphore, 0, NULL );
    rghThreads[1] = CreateThread( NULL, 0, AcquireForever, psemaphore, 0, NULL );

    while ( psemaphore->CWait() == 0 );

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == 1 );

    psemaphore->Release();

    while ( psemaphore->CWait() != 0 );

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == 0 );

    const DWORD dwWait = WaitForMultipleObjectsEx( 2, rghThreads, TRUE, INFINITE, FALSE );
    TestCheck( dwWait == WAIT_OBJECT_0 );

    TestAssert( CloseHandle( rghThreads[0] ) );
    TestAssert( CloseHandle( rghThreads[1] ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphorePerformsConcurrentAcquireRelease, 0x0, "" );
ERR SyncSemaphorePerformsConcurrentAcquireRelease::ErrTest()
{
    SyncBasicTestInit;

    const DWORD cThreads = 60;
    const DWORD cThreadsRelease = 30;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    HANDLE rghThreads[cThreads];
    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        rghThreads[iThread] = CreateThread( NULL, 0, AcquireForever, psemaphore, 0, NULL );
    }

    while ( psemaphore->CWait() != cThreads )
    {
        TestAssert( psemaphore->CAvail() == 0 );
    }

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == cThreads );

    psemaphore->Release();

    while ( psemaphore->CWait() != ( cThreads - 1 ) );

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == ( cThreads - 1 ) );

    psemaphore->Release( cThreadsRelease );

    while ( psemaphore->CWait() != ( cThreads - 1 - cThreadsRelease ) );

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == ( cThreads - 1 - cThreadsRelease ) );

    psemaphore->ReleaseAllWaiters();

    while ( psemaphore->CWait() != 0 );

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == 0 );

    const DWORD dwWait = WaitForMultipleObjectsEx( cThreads, rghThreads, TRUE, INFINITE, FALSE );
    TestCheck( dwWait == WAIT_OBJECT_0 );
    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        TestAssert( CloseHandle( rghThreads[iThread] ) );
    }
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphorePerformsConcurrentTestFAcquireRelease, 0x0, "" );
ERR SyncSemaphorePerformsConcurrentTestFAcquireRelease::ErrTest()
{
    SyncBasicTestInit;

    const DWORD cThreads = 60;
    const DWORD cThreadsRelease = 30;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    HANDLE rghThreads[cThreads];
    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        rghThreads[iThread] = CreateThread( NULL, 0, AcquireForeverSpin, psemaphore, 0, NULL );
    }

    TestCheck( psemaphore->CAvail() == 0 );

    psemaphore->Release();

    while ( psemaphore->CAvail() != 0 );

    TestCheck( psemaphore->CAvail() == 0 );

    psemaphore->Release( cThreadsRelease );

    while ( psemaphore->CAvail() != 0 );

    TestCheck( psemaphore->CAvail() == 0 );

    psemaphore->Release( cThreads - 1 - cThreadsRelease );

    while ( psemaphore->CAvail() != 0 );

    TestCheck( psemaphore->CAvail() == 0 );
    TestCheck( psemaphore->CWait() == 0 );

    const DWORD dwWait = WaitForMultipleObjectsEx( cThreads, rghThreads, TRUE, INFINITE, FALSE );
    TestCheck( dwWait == WAIT_OBJECT_0 );
    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        TestAssert( CloseHandle( rghThreads[iThread] ) );
    }
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphoreMultiThreadedAcquireTimesOutWithoutRelease, 0x0, "" );
ERR SyncSemaphoreMultiThreadedAcquireTimesOutWithoutRelease::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    g_dwTimeout = 2000;
    DWORD dwDelay = GetTickCount();
    HANDLE hThread = CreateThread( NULL, 0, AcquireOneTimeout, psemaphore, 0, NULL );

    while ( psemaphore->CWait() == 0 );

    TestCheck( psemaphore->CAvail() == 0 );

    DWORD dwWait = WaitForSingleObject( hThread, INFINITE );
    dwDelay = GetTickCount() - dwDelay;

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    TestCheck( dwWait == WAIT_OBJECT_0 );
    TestCheck( dwDelay >= ( g_dwTimeout - 16 ) );

    DWORD dwExitCode = 0;
    TestAssert( GetExitCodeThread( hThread, &dwExitCode ) );

    TestCheck( (BOOL)dwExitCode == fFalse );

    TestAssert( CloseHandle( hThread ) );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphoreMultiThreadedAcquireDoesNotTimeoutIfReleased, 0x0, "" );
ERR SyncSemaphoreMultiThreadedAcquireDoesNotTimeoutIfReleased::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    g_dwTimeout = 10 * 60 * 1000;
    HANDLE hThread = CreateThread( NULL, 0, AcquireOneTimeout, psemaphore, 0, NULL );

    while ( psemaphore->CWait() == 0 );

    TestCheck( psemaphore->CAvail() == 0 );

    DWORD dwDelay = GetTickCount();
    psemaphore->Release();

    DWORD dwWait = WaitForSingleObject( hThread, INFINITE );
    dwDelay = GetTickCount() - dwDelay;

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    TestCheck( dwWait == WAIT_OBJECT_0 );
    TestCheck( dwDelay < 10000 );

    DWORD dwExitCode = 0;
    TestAssert( GetExitCodeThread( hThread, &dwExitCode ) );

    TestCheck( (BOOL)dwExitCode == fTrue );

    TestAssert( CloseHandle( hThread ) );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

//  The spawned threads wait forever by doing psemaphore->Wait(). Note that Wait() does _not_ acquire 
//  resources. Those threads just sit there waiting for the object to be signaled. So, once the main 
//  thread releases 3000 resources, all 6000 waiting threads get unblocked (yet, as I said, they don't 
//  acquire) and finish, and the expected final state is then between CWait() == 0 and CAvail() == 3000.
//
//  Note: that even releasing one resource would suffice to unblock them all.

ERR SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResourcesWorker( const DWORD cThreadGroups )
{
    SyncBasicTestInit;

    const DWORD cThreadGroup = 60;
    const DWORD cThreads = cThreadGroup * cThreadGroups;
    const DWORD cThreadsRelease = cThreads / 2;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    INT cAvail = psemaphore->CAvail();
    INT cWait = psemaphore->CWait();
    TestCheck( cAvail == 0 );
    TestCheck( cWait == 0 );

    HANDLE rghThreads[6001];
    TestCheck( cThreads <= _countof( rghThreads ) );
    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        rghThreads[iThread] = CreateThread( NULL, 0, WaitForever, psemaphore, 0, NULL );
        TestAssert( rghThreads[iThread] != NULL );
    }

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( cAvail == 0 );
    TestCheck( cWait >= 0 && (DWORD)cWait <= cThreads );

    while ( psemaphore->CWait() != (INT)cThreads )
    {
        cAvail = psemaphore->CAvail();
        cWait = psemaphore->CWait();
        TestCheck( cAvail >= 0 && (DWORD)cAvail <= cThreadsRelease );
        TestCheck( cWait >= 0 && (DWORD)cWait <= cThreads );
    }

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( cAvail == 0 );
    TestCheck( (DWORD)cWait == cThreads );

    psemaphore->Release( cThreadsRelease );

    while ( true )
    {
        DWORD cThreadUnfinished = 0;
        for ( DWORD iThreadGroup = 0; iThreadGroup < cThreadGroups; iThreadGroup++ )
        {
            cAvail = psemaphore->CAvail();
            cWait = psemaphore->CWait();
            TestCheck( cAvail >= 0 && (DWORD)cAvail <= cThreadsRelease );
            TestCheck( cWait >= 0 && (DWORD)cWait <= ( cThreads - cThreadsRelease ) );

            DWORD dwWait = WAIT_TIMEOUT;
            if ( ( dwWait = WaitForMultipleObjectsEx( cThreadGroup, &rghThreads[ iThreadGroup * cThreadGroup ], TRUE, 0, FALSE ) ) == WAIT_TIMEOUT )
            {
                cThreadUnfinished++;
            }
            else
            {
                TestAssert( dwWait == WAIT_OBJECT_0 );
            }
        }

        if ( cThreadUnfinished == 0 )
        {
            break;
        }
    }

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( (DWORD)cAvail == cThreadsRelease );
    TestCheck( cWait == 0 );

    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        TestAssert( CloseHandle( rghThreads[iThread] ) );
    }

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( (DWORD)cAvail == cThreadsRelease );
    TestCheck( cWait == 0 );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResources, 0x0, "" );
ERR SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResources::ErrTest()
{
    return SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResourcesWorker( 1 ); // 1 group = 6 threads ...
}

CUnitTest( SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResourcesStress, bitExplicitOnly, "" );
ERR SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResourcesStress::ErrTest()
{
    return SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResourcesWorker( 100 ); // ~6000 threads
}


//  FWait() is similar to previous SyncSemaphoreCheckWaitDoesNotPermanentlyAcquireResources() above. The 
//  only difference is that it takes a timeout and it returns false/true to indicate whether it got 
//  unblocked before the timeout expired. This test is actually very similar to the test above, but it 
//  only releases one resource and in the end expects CWait() == 0 or CAvail() == 1.
//
//  Note: The test asserts TestAssert( cAvail == 1 || cAvail == 0 ) while there are threads potentially 
//  running, this is because even though Wait()/FWait() don't _permanently_ acquire resources, they may 
//  TEMPORARILY do so and immediately release it and that is why you could see CAvail() == 0 in some spots.
//
//  But regardless, the final state should always be CWait() == 0 or CAvail() == 1.

ERR SyncSemaphoreFWaitDoesNotPermanentlyAcquireResourcesWorker( const DWORD cThreadGroups )
{
    SyncBasicTestInit;

    const DWORD cThreadGroup = 60;
    const DWORD cThreads = cThreadGroup * cThreadGroups;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    INT cAvail = psemaphore->CAvail();
    INT cWait = psemaphore->CWait();
    TestCheck( cWait == 0 );
    TestCheck( cAvail == 0 );

    HANDLE rghThreads[6001];
    TestCheck( cThreads <= _countof( rghThreads ) );
    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        rghThreads[iThread] = CreateThread( NULL, 0, WaitForeverSpin, psemaphore, 0, NULL );
        TestAssert( rghThreads[iThread] != NULL );
    }

    TestCheck( psemaphore->CAvail() == 0 );

    psemaphore->Release();

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( cAvail == 1 || cAvail == 0 );
    TestCheck( cWait >= 0 && (DWORD)cWait <= ( cThreads - 1 ) );

    while ( true )
    {
        DWORD cThreadUnfinished = 0;
        for ( DWORD iThreadGroup = 0; iThreadGroup < cThreadGroups; iThreadGroup++ )
        {
            cAvail = psemaphore->CAvail();
            cWait = psemaphore->CWait();
            TestCheck( cAvail == 1 || cAvail == 0 );
            TestCheck( cWait >= 0 && (DWORD)cWait <= ( cThreads - 1 ) );

            DWORD dwWait = WAIT_TIMEOUT;
            if ( ( dwWait = WaitForMultipleObjectsEx( cThreadGroup, &rghThreads[ iThreadGroup * cThreadGroup ], TRUE, 0, FALSE ) ) == WAIT_TIMEOUT )
            {
                cThreadUnfinished++;
            }
            else
            {
                TestAssert( dwWait == WAIT_OBJECT_0 );
            }
        }

        if ( cThreadUnfinished == 0 )
        {
            break;
        }
    }

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( cAvail == 1 );
    TestCheck( cWait == 0 );

    for ( DWORD iThread = 0; iThread < cThreads; iThread++ )
    {
        TestAssert( CloseHandle( rghThreads[iThread] ) );
    }

    cAvail = psemaphore->CAvail();
    cWait = psemaphore->CWait();
    TestCheck( cAvail == 1 );
    TestCheck( cWait == 0 );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphoreFWaitDoesNotPermanentlyAcquireResources, 0x0, "" );
ERR SyncSemaphoreFWaitDoesNotPermanentlyAcquireResources::ErrTest()
{
    return SyncSemaphoreFWaitDoesNotPermanentlyAcquireResourcesWorker( 1 ); // 1 group = 6 threads ...
}

CUnitTest( SyncSemaphoreFWaitDoesNotPermanentlyAcquireResourcesStress, bitExplicitOnly, "" );
ERR SyncSemaphoreFWaitDoesNotPermanentlyAcquireResourcesStress::ErrTest()
{
    return SyncSemaphoreFWaitDoesNotPermanentlyAcquireResourcesWorker( 100 ); // ~6000 threads
}


CUnitTest( SyncSemaphoreMultiThreadedWaitTimesOutWithoutRelease, 0x0, "" );
ERR SyncSemaphoreMultiThreadedWaitTimesOutWithoutRelease::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    g_dwTimeout = 2000;
    DWORD dwDelay = GetTickCount();
    HANDLE hThread = CreateThread( NULL, 0, WaitOneTimeout, psemaphore, 0, NULL );

    while ( psemaphore->CWait() == 0 );

    TestCheck( psemaphore->CAvail() == 0 );

    DWORD dwWait = WaitForSingleObject( hThread, INFINITE );
    dwDelay = GetTickCount() - dwDelay;

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    TestCheck( dwWait == WAIT_OBJECT_0 );
    TestCheck( dwDelay >= ( g_dwTimeout - 16 ) );

    DWORD dwExitCode = 0;
    TestAssert( GetExitCodeThread( hThread, &dwExitCode ) );

    TestCheck( (BOOL)dwExitCode == fFalse );

    TestAssert( CloseHandle( hThread ) );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphoreMultiThreadedWaitDoesNotTimeoutIfReleased, 0x0, "" );
ERR SyncSemaphoreMultiThreadedWaitDoesNotTimeoutIfReleased::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "CSemaphore test." ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    g_dwTimeout = 60 * 10 * 1000;
    HANDLE hThread = CreateThread( NULL, 0, WaitOneTimeout, psemaphore, 0, NULL );

    while ( psemaphore->CWait() == 0 );

    TestCheck( psemaphore->CAvail() == 0 );

    DWORD dwDelay = GetTickCount();
    psemaphore->Release();

    DWORD dwWait = WaitForSingleObject( hThread, INFINITE );
    dwDelay = GetTickCount() - dwDelay;

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 1 );

    TestCheck( dwWait == WAIT_OBJECT_0 );
    TestCheck( dwDelay < 10000 );

    DWORD dwExitCode = 0;
    TestAssert( GetExitCodeThread( hThread, &dwExitCode ) );

    TestCheck( (BOOL)dwExitCode == fTrue );

    TestAssert( CloseHandle( hThread ) );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphorePerformsBasicUncontendedSemaphoreReleaseAndAcquire, 0x0, "" );
ERR SyncSemaphorePerformsBasicUncontendedSemaphoreReleaseAndAcquire::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore* psemaphore = new CSemaphore( CSyncBasicInfo( "SyncPerformsBasicUncontendedSemaphoreReleaseAndAcquire" ) );

    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

    psemaphore->Release( 2 );
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 2 );

    psemaphore->Acquire();
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 1 );

    psemaphore->Acquire();
    TestCheck( psemaphore->CWait() == 0 );
    TestCheck( psemaphore->CAvail() == 0 );

HandleError:
    delete psemaphore;
    SyncBasicTestTerm;
    return err;
}

CUnitTest( SyncSemaphorePerformsBasicUncontendedSemaphoreReleaseAndAcquireWithSemaphoreOnStack, 0x0, "" );
ERR SyncSemaphorePerformsBasicUncontendedSemaphoreReleaseAndAcquireWithSemaphoreOnStack::ErrTest()
{
    SyncBasicTestInit;

    CSemaphore sem( CSyncBasicInfo( "SyncPerformsBasicUncontendedSemaphoreReleaseAndAcquireV2" ) );

    TestCheck( sem.CWait() == 0 );
    TestCheck( sem.CAvail() == 0 );

    sem.Release( 2 );
    TestCheck( sem.CWait() == 0 );
    TestCheck( sem.CAvail() == 2 );

    sem.Acquire();
    TestCheck( sem.CWait() == 0 );
    TestCheck( sem.CAvail() == 1 );

    sem.Acquire();
    TestCheck( sem.CWait() == 0 );
    TestCheck( sem.CAvail() == 0 );

HandleError:
    SyncBasicTestTerm;
    return err;
}
