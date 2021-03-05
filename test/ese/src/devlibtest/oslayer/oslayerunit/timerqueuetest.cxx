// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "osunitstd.hxx"

typedef __success(return >= 0) LONG NTSTATUS;

CSemaphore g_semStress( CSyncBasicInfo( "SchedulingTimerStress::g_semStress" ) );
POSTIMERTASK g_posttStressTask = NULL;
DWORD g_fScheduled = 0;
DWORD g_fRunning = 0;
DWORD g_cExecuted = 0;
DWORD g_cCancelled = 0;
const DWORD g_dtickRuntime = 5000;




POSTIMERTASK    g_rgposttTesters[5];

#define OSTestTimerPreamble()           \
    ERR err = JET_errSuccess;           \
    COSLayerPreInit oslayer;            \
    OSTestCall( ErrOSInit() );          \
    srand( TickOSTimeCurrent() );       \
    g_fRandomizedDelays = fFalse;       \

#define OSTestTimerCleanup( cTimers )   \
    for( INT i = 0; i < cTimers; i++ )  \
        {                               \
        if ( g_rgposttTesters[i] )      \
            {                           \
            OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[i] ) ) );   \
            OSTimerTaskDelete( g_rgposttTesters[i] );                               \
            g_rgposttTesters[i] = NULL;                                             \
        }                               \
    }                                   \
    wprintf(L"OSTerm()ing\n" );         \
    OSTerm();                           \

TICK g_dtickRandSleepMax = 1300;
VOID RandSleep()
{
    TICK tickSleep = 0;
    if ( g_dtickRandSleepMax == (ULONG)-1 )
    {
        return;
    }
    if ( g_dtickRandSleepMax == 0 )
    {
        Sleep( 0 );
        return;
    }
    switch( rand() % 20 )
    {
        case 19:
            tickSleep = rand() % g_dtickRandSleepMax;
            break;
        case 18:
            tickSleep = rand() % min( 300, g_dtickRandSleepMax );
            break;
        case 17:
        case 16:
        case 15:
        case 14:
        case 13:
            tickSleep = rand() % min( 50, g_dtickRandSleepMax );
            break;
        case 12:
        case 11:
            tickSleep = rand() % min( 14, g_dtickRandSleepMax );
            break;
        default:
            tickSleep = rand() % min( 3, g_dtickRandSleepMax );
            break;
        case 0:
            return;
    }
    Sleep( tickSleep );
}

VOID ScheduleTimerStress( const TICK dtickDelay, VOID* const pvRuntimeContext )
{
    Enforce( g_semStress.CAvail() <= 1 );
    if ( !g_semStress.FTryAcquire() )
    {
        Enforce( g_semStress.CAvail() <= 1 );
        return;
    }

    Enforce( g_semStress.CAvail() == 0 );

    const BOOL fSelfScheduling = ( pvRuntimeContext != NULL );
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fScheduled, 1 ) == 0 );

    Enforce( g_posttStressTask != NULL );
    OSTimerTaskScheduleTask(
        g_posttStressTask,
        !fSelfScheduling ? (VOID*)AtomicRead( (LONG*)&g_cCancelled ) : pvRuntimeContext,
        dtickDelay,
        0 );
}

VOID CancelTimerStress( PfnTimerTask pfnTask )
{
    Enforce( g_posttStressTask != NULL );
    OSTimerTaskCancelTask( g_posttStressTask );

    AtomicExchange( (LONG*)&g_fScheduled, 0 );

    const DWORD cCancelled = AtomicIncrement( (LONG*)&g_cCancelled );

    Enforce( g_semStress.CAvail() <= 1 );
    (void)g_semStress.FTryAcquire();
    Enforce( g_semStress.CAvail() == 0 );

    Enforce( g_semStress.CAvail() == 0 );
    g_semStress.Release();
    Enforce( g_semStress.CAvail() <= 1 );
}

#include "NTSecAPI.h"

NTSTATUS
WINAPI
NtQueryTimerResolution (
    _Out_ PULONG MaximumTime,
    _Out_ PULONG MinimumTime,
    _Out_ PULONG CurrentTime
    );
NTSTATUS
WINAPI
NtSetTimerResolution (
    _In_ ULONG DesiredTime,
    _In_ BOOLEAN SetResolution,
    _Out_ PULONG ActualTime
    );
const wchar_t * const g_mwszzNtdllLibs = L"ntdll.dll" L"\0";
NTOSFuncNtStd( g_pfnNtQueryTimerResolution, g_mwszzNtdllLibs, NtQueryTimerResolution, oslfExpectedOnWin5x );
NTOSFuncNtStd( g_pfnNtSetTimerResolution, g_mwszzNtdllLibs, NtSetTimerResolution, oslfExpectedOnWin5x );

JET_ERR ErrSchedulingTimerStress_( const PfnTimerTask pfnTask )
{
    AtomicExchange( (LONG*)&g_fScheduled, 0 );
    AtomicExchange( (LONG*)&g_fRunning, 0 );
    AtomicExchange( (LONG*)&g_cCancelled, 1 );
    AtomicExchange( (LONG*)&g_cExecuted, 0 );

    Enforce( g_posttStressTask == NULL );
    Enforce( ErrOSTimerTaskCreate( pfnTask, NULL, &g_posttStressTask ) == JET_errSuccess );

    Enforce( g_semStress.CAvail() == 0 );
    g_semStress.Release();
    Enforce( g_semStress.CAvail() <= 1 );

    ULONG Min, Max, Current, Effective;
    g_pfnNtQueryTimerResolution( &Max, &Min, &Current );
    wprintf( L"            Timer resolutions - Min %d, Max %d, Current %d\n", Min, Max, Current );
    Effective = ( rand() % 2 ) ? Min : Current;
    wprintf( L"            Overriding timer resolution to %d\n", Effective );
    g_pfnNtSetTimerResolution( Effective, true, &Effective );

    const TICK tickInitial = GetTickCount();
    while ( ( GetTickCount() - tickInitial ) < g_dtickRuntime )
    {
        ScheduleTimerStress( ( ( rand() % 2 ) == 0 ) ? 0 : 1, 0 );

        if ( ( rand() % 2 ) == 0 )
        {
            UtilSleep( 1 );
        }

        CancelTimerStress( pfnTask );
    }

    g_pfnNtSetTimerResolution( Current, false, &Current );

    Enforce( g_semStress.CAvail() == 1 );
    Enforce( g_semStress.FTryAcquire() );
    Enforce( g_semStress.CAvail() == 0 );

    const DWORD cCancelled = (DWORD)AtomicRead( (LONG*)&g_cCancelled );
    const DWORD cExecuted = (DWORD)AtomicRead( (LONG*)&g_cExecuted );
    wprintf( L"            Cancelled = %d, Executed = %d\n", cCancelled, cExecuted );

    Enforce( (DWORD)AtomicRead( (LONG*)&g_fRunning ) == 0 );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_fScheduled ) == 0 );

    Enforce( g_posttStressTask != NULL );
    OSTimerTaskDelete( g_posttStressTask );
    g_posttStressTask = NULL;

    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == cCancelled );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_cExecuted ) == cExecuted );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_fRunning ) == 0 );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_fScheduled ) == 0 );

    return JET_errSuccess;
}



        
VOID OSTimerTestGroupCtxSemaphoreReleaser( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore *psemaphore = (CSemaphore*)pvGroupContext;

    psemaphore->Release();
}

VOID OSTimerTestGroupCtxSemaphoreReleaserRuntimeCtxIncrementerRandSleep( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore * const psemaphore = (CSemaphore*)pvGroupContext;
    LONG * const pul = (LONG*) pvRuntimeContext;

    RandSleep();

    if ( pul )
    {
        AtomicIncrement( pul );
    }
    psemaphore->Release();
}

VOID OSTimerTestGroupCtxSemaphoreTryReleaser( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore *psemaphore = (CSemaphore*)pvGroupContext;

    if ( psemaphore->FTryAcquire() )
    {
        psemaphore->Release();
    }

    psemaphore->Release();
}

VOID OSTimerTestRuntimeCtxSemaphoreReleaser( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore *psemaphore = (CSemaphore*)pvRuntimeContext;

    psemaphore->Release();
}

VOID OSTimerTestRuntimeCtxSemaphoreTryReleaser( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore *psemaphore = (CSemaphore*)pvRuntimeContext;

    if ( psemaphore->FTryAcquire() )
    {
        psemaphore->Release();
    }

    psemaphore->Release();
}

const TICK g_dtickSleepyNapTimeMax = 500;

VOID OSTimerTestGroupCtxSlowSleepySemaphoreReleaser( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore *psemaphore = (CSemaphore*)pvGroupContext;

    Sleep( g_dtickSleepyNapTimeMax );

    psemaphore->Release();
}

VOID OSTimerTestGroupCtxSemaphoreReleaserThenNapTime( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    CSemaphore *psemaphore = (CSemaphore*)pvGroupContext;

    psemaphore->Release();

    Sleep( g_dtickSleepyNapTimeMax );
}

VOID SchedulingOsTimerStressTask( VOID* const pvGroupContext, VOID* const pvRuntimeContext )
{
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fRunning, 1 ) == 0 );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == (DWORD)pvRuntimeContext );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_fScheduled ) == 1 );

    Enforce( g_semStress.CAvail() == 0 );
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fScheduled, 0 ) == 1 );
    g_semStress.Release();
    Enforce( g_semStress.CAvail() <= 1 );

    if ( ( rand() % 2 ) == 0 )
    {
        UtilSleep( 1 );
    }

    AtomicIncrement( (LONG*)&g_cExecuted );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == (DWORD)pvRuntimeContext );
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fRunning, 0 ) == 1 );
}

VOID SelfSchedulingOsTimerStressTask( VOID* const pvGroupContext, VOID* const pvRuntimeContext )
{
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fRunning, 1 ) == 0 );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == (DWORD)pvRuntimeContext );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_fScheduled ) == 1 );

    Enforce( g_semStress.CAvail() == 0 );
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fScheduled, 0 ) == 1 );
    g_semStress.Release();
    Enforce( g_semStress.CAvail() <= 1 );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == (DWORD)pvRuntimeContext );

    if ( ( rand() % 2 ) == 0 )
    {
        UtilSleep( 1 );
    }

    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == (DWORD)pvRuntimeContext );

    ScheduleTimerStress( ( ( rand() % 2 ) == 0 ) ? 0 : 1, pvRuntimeContext );

    if ( ( rand() % 2 ) == 0 )
    {
        UtilSleep( 1 );
    }

    AtomicIncrement( (LONG*)&g_cExecuted );
    Enforce( (DWORD)AtomicRead( (LONG*)&g_cCancelled ) == (DWORD)pvRuntimeContext );
    Enforce( (DWORD)AtomicExchange( (LONG*)&g_fRunning, 0 ) == 1 );
}


BOOL            g_fRandomizedDelays = fFalse;
TICK            g_dtickDelayMax = 0;
TICK            g_dtickFuzzMax = 0;
ULONG           g_cRuns = 0;

VOID OSTimerTestSelfRescheduler( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    g_cRuns++;

    if ( g_cRuns == 1 )
    {
        srand( TickOSTimeCurrent() );
    }

    const TICK dtickDelay   = g_fRandomizedDelays ? ( 1 + rand() % g_dtickDelayMax ) : g_dtickDelayMax;
    const TICK dtickFuzz    = g_fRandomizedDelays ? ( 1 + rand() % g_dtickFuzzMax ) : g_dtickFuzzMax;


    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, dtickDelay, dtickFuzz );
}

VOID OSTimerTestCrossScheduler( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    g_cRuns++;


    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 100, 10 );
}


VOID OSTimerTestSelfReschedulerAndReleaser( VOID * pvGroupContext, VOID * pvRuntimeContext )
{
    g_cRuns++;

    CSemaphore *psemaphore = (CSemaphore*)( pvRuntimeContext ? pvRuntimeContext : pvGroupContext );

    psemaphore->Release();

    if ( g_cRuns == 1 )
    {
        srand( TickOSTimeCurrent() );
    }

    const TICK dtickDelay   = g_fRandomizedDelays ? ( 1 + rand() % g_dtickDelayMax ) : g_dtickDelayMax;
    const TICK dtickFuzz    = g_fRandomizedDelays ? ( 1 + rand() % g_dtickFuzzMax ) : g_dtickFuzzMax;


    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, dtickDelay, dtickFuzz );
}



JET_ERR ErrOSTimerTestRun( const TICK dtickRunTime, const TICK dtickInitialDelay, const TICK dtickInitialFuzz, const TICK dtickSubsequentDelayMax, const TICK dtickSubsequentFuzzMax )
{
    g_cRuns = 0;

    g_dtickDelayMax = dtickSubsequentDelayMax;
    g_dtickFuzzMax = dtickSubsequentFuzzMax;

    wprintf(L"\tBeginning test run ( runtime = %d ... dtickInitial %d/%d ... SubsequentMax = %d/%d )\n", dtickRunTime,
            dtickInitialDelay, dtickInitialFuzz, g_dtickDelayMax, g_dtickFuzzMax );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, dtickInitialDelay, dtickInitialFuzz );

    wprintf(L"\tSleeping for %d.%03d seconds ...\n", dtickRunTime / 1000, dtickRunTime % 1000 );
    UtilSleep( dtickRunTime );
    wprintf(L"\tWaking ...\n" );

    OSTimerTaskCancelTask( g_rgposttTesters[0] );

    return JET_errSuccess;
}

INT CSemaphoreCollect( CSemaphore * psemaphore )
{
    INT cAcquired = 0;

    while( psemaphore->FTryAcquire() )
    {
        cAcquired++;
    }

    UtilSleep( 16 );
    Expected( psemaphore->FTryAcquire() == fFalse );    
    Expected( psemaphore->CAvail() == 0 );

    return cAcquired;
}

INT CSemaphoreWaitCollect( CSemaphore * psemaphore, const TICK dtickMaxInterimWait, const INT cExpected )
{
    INT cAcquired = 0;

    Assert( dtickMaxInterimWait > 10 );

#ifdef DEBUG
    const BOOL fQuickOut = fFalse;
#else
    const BOOL fQuickOut = fTrue;
#endif

    while( fTrue )
    {
        TICK tickStartNextWait = TickOSTimeCurrent();

        BOOL fIncrementAcquired = fFalse;
        while( !( fIncrementAcquired = psemaphore->FTryAcquire() ) )
        {
            UtilSleep( dtickMaxInterimWait / 10 );
            if( DtickDelta( tickStartNextWait, TickOSTimeCurrent() ) > (INT)dtickMaxInterimWait )
            {
                Assert( !fIncrementAcquired );
                break;
            }
        }
        if( !fIncrementAcquired )
        {
            break;
        }
        cAcquired++;
        if( fQuickOut && cAcquired == cExpected )
        {
            break;
        }
    }

    UtilSleep( 16 );
    Expected( psemaphore->FTryAcquire() == fFalse );    
    Expected( psemaphore->CAvail() == 0 );

    return cAcquired;
}



CUnitTest( OSTimerTaskTestCancelWinsOverScheduledTask, 0, "Tests that a timer task that is scheduled far in the future will lose out (without getting run) when cancel is called before the scheduled tick delay." );
ERR OSTimerTaskTestCancelWinsOverScheduledTask::ErrTest()
{
    OSTestTimerPreamble();
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestSelfRescheduler, NULL, &g_rgposttTesters[0]  ) );

    OSTestCall( ErrOSTimerTestRun( 2000, 3000, 1500, 1000, 1000 ) );
    OSTestCheck( g_cRuns == 0 );
    RandSleep();
    OSTestCheck( g_cRuns == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskOneShotTimerFiresOnceGroupCtx, 0, "Simple test for one-shot timers work right using group context." );
ERR OSTimerTaskOneShotTimerFiresOnceGroupCtx::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "OneShotSemaphore" ) );
    OSTestTimerPreamble();

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaser, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    printf( "\t%s\n", __FUNCTION__ );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );


    semTimerShot.Acquire();

    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskOneShotTimerFiresOnceRuntimeCtx, 0, "Simple test for one-shot timers work right using runtime context." );
ERR OSTimerTaskOneShotTimerFiresOnceRuntimeCtx::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "OneShotSemaphore" ) );
    OSTestTimerPreamble();

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestRuntimeCtxSemaphoreReleaser, NULL, &g_rgposttTesters[0] ) );


    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    printf( "\t%s\n", __FUNCTION__ );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], &semTimerShot, 0, 0 );


    semTimerShot.Acquire();

    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTwoTaskTimerFiresTwiceGroupCtx, 0, "Simple test for two one-shot timers work right using group context." );
ERR OSTimerTaskTwoTaskTimerFiresTwiceGroupCtx::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "TwoShotSemaphore" ) );
    OSTestTimerPreamble();

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaser, &semTimerShot, &g_rgposttTesters[0] ) );
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreTryReleaser, &semTimerShot, &g_rgposttTesters[1] ) );


    printf( "\t%s\n", __FUNCTION__ );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
    OSTimerTaskScheduleTask( g_rgposttTesters[1], NULL, 0, 0 );


    semTimerShot.Acquire();
    semTimerShot.Acquire();

    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTimerTaskCancelTask( g_rgposttTesters[1] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 2 );
    return err;
}

CUnitTest( OSTimerTaskTwoTaskTimerFiresTwiceRuntimeCtx, 0, "Simple test for two one-shot timers work right using runtime context." );
ERR OSTimerTaskTwoTaskTimerFiresTwiceRuntimeCtx::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "TwoShotSemaphore" ) );
    OSTestTimerPreamble();

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestRuntimeCtxSemaphoreReleaser, NULL, &g_rgposttTesters[0] ) );
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestRuntimeCtxSemaphoreTryReleaser, NULL, &g_rgposttTesters[1] ) );


    printf( "\t%s\n", __FUNCTION__ );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], &semTimerShot, 0, 0 );
    OSTimerTaskScheduleTask( g_rgposttTesters[1], &semTimerShot, 0, 0 );


    semTimerShot.Acquire();
    semTimerShot.Acquire();

    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTimerTaskCancelTask( g_rgposttTesters[1] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 2 );
    return err;
}

CUnitTest( OSTimerTaskTwoTaskTimerFiresTwiceMixedCtx, 0, "Simple test for two one-shot timers work right using both group and runtime context." );
ERR OSTimerTaskTwoTaskTimerFiresTwiceMixedCtx::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "TwoShotSemaphore" ) );
    OSTestTimerPreamble();

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaser, &semTimerShot, &g_rgposttTesters[0] ) );
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestRuntimeCtxSemaphoreTryReleaser, NULL, &g_rgposttTesters[1] ) );


    printf( "\t%s\n", __FUNCTION__ );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], &semTimerShot , 0, 0 );
    OSTimerTaskScheduleTask( g_rgposttTesters[1], &semTimerShot, 0, 0 );


    semTimerShot.Acquire();
    semTimerShot.Acquire();

    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTimerTaskCancelTask( g_rgposttTesters[1] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 2 );
    return err;
}

CUnitTest( OSTimerTaskTestReasonableRuns, 0, "Tests that a timer task gets several appropriate # of runs within the tick delays and runtime." );
ERR OSTimerTaskTestReasonableRuns::ErrTest()
{
    OSTestTimerPreamble();
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestSelfRescheduler, NULL, &g_rgposttTesters[0]  ) );

    OSTestCall( ErrOSTimerTestRun( 2100, 100, 500, 100, 500 ) );

    OSTestCheck( g_cRuns >= 3 && g_cRuns <= 21 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}



BOOL FOSPeriodicTimerTestRun( POSTIMERTASK postt, CSemaphore * psemCallback, const BOOL fWithInitialDelay, const ULONG iPeriodicSpeedSlowness )
{
    BOOL        fSuccess = fTrue;
    BOOL        fCancelNeeded = fFalse;


    g_dtickDelayMax = 100 * iPeriodicSpeedSlowness;
    g_dtickFuzzMax = 0;
    g_cRuns = 0;

    const TICK  dtickCheckFuzziness = g_dtickDelayMax / 10;

    #define Check( fCheck )                 \
        if ( !( fSuccess = ( fCheck ) ) )   \
            {                               \
            goto HandleError;               \
        }

    const TICK dtickInitial = fWithInitialDelay ? ( g_dtickDelayMax * 2 ) : 0;

    wprintf( L"[%d:%dx5=%d-%d ms]", dtickInitial, g_dtickDelayMax, ( ( 0 * g_dtickDelayMax ) + dtickInitial ), ( ( 5 * g_dtickDelayMax ) + dtickInitial ) );

    const TICK tickStart = TickOSTimeCurrent();
    TICK tickFinal;

    OSTimerTaskScheduleTask( postt, NULL, dtickInitial, 0 );
    fCancelNeeded = fTrue;


    for( INT i = 0; i < 6; i++ )
    {
        psemCallback->Acquire();
        const TICK tickEnd = tickFinal = TickOSTimeCurrent();

        wprintf( L"(%d,%d,%d)", 
                ( ( i * g_dtickDelayMax ) + dtickInitial - dtickCheckFuzziness ), 
                DtickDelta( tickStart, tickEnd ), 
                ( ( i * g_dtickDelayMax ) + dtickInitial + dtickCheckFuzziness )  );

        Check( (INT)( ( i * g_dtickDelayMax ) + dtickInitial - dtickCheckFuzziness ) < DtickDelta( tickStart, tickEnd ) );
        Check( (INT)( ( i * g_dtickDelayMax ) + dtickInitial + dtickCheckFuzziness ) > DtickDelta( tickStart, tickEnd ) );
    }


    OSTimerTaskCancelTask( postt );
    fCancelNeeded = fFalse;
    
    Check( g_cRuns >= 6 && g_cRuns < 7  );

    wprintf( L"\tSucceeded on run %d: Initial: %d ms, Subsequent: %d ms delay, ran correctly for %d runs over %d ms with a %d ms inaccuracy\n", iPeriodicSpeedSlowness, dtickInitial, g_dtickDelayMax, g_cRuns, DtickDelta( tickStart, tickFinal ), dtickCheckFuzziness );

HandleError:

    if ( fCancelNeeded )
    {

        OSTimerTaskCancelTask( postt );
        fCancelNeeded = fFalse;
    }

    return fSuccess;
}

CUnitTest( OSTimerTaskTestPeriodicEmulation, 0, "Tests that a timer task that regularly reschedules gets approximate periodic callbacks of right frequency." );
ERR OSTimerTaskTestPeriodicEmulation::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "PeriodicTimer" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestSelfReschedulerAndReleaser, &semTimerShot, &g_rgposttTesters[0]  ) );

    const ULONG cRuns = 100;
    for( ULONG i = 0; i < cRuns; i++ )
    {

        OSTestCheck( i < ( cRuns - 1 ) );


        wprintf( L"\n\t[%d]", i );
        if ( FOSPeriodicTimerTestRun( g_rgposttTesters[0], &semTimerShot, fFalse, 1+i/4 ) )
        {
            break;
        }
    }

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestPeriodicEmulationWithInitial, 0, "Tests that a timer task that regularly reschedules gets approximate periodic callbacks of right frequency, and with an initial backoff." );
ERR OSTimerTaskTestPeriodicEmulationWithInitial::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "PeriodicTimer" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestSelfReschedulerAndReleaser, &semTimerShot, &g_rgposttTesters[0]  ) );

    const ULONG cRuns = 100;
    for( ULONG i = 0; i < cRuns; i++ )
    {

        OSTestCheck( i < ( cRuns - 1 ) );


        wprintf( L"\n\t[%d]", i );
        if ( FOSPeriodicTimerTestRun( g_rgposttTesters[0], &semTimerShot, fTrue, 1+i/4 ) )
        {
            break;
        }
    }

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestOneShotWithExternalReschedule, 0, "Tests that a one-shot timer that gets repeatedly rescheduled (but not cancelled) only runs once after the last reschedule." );
ERR OSTimerTaskTestOneShotWithExternalReschedule::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "OneShotTimer" ) );
    const TICK dtickTimeout = 1000;
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaser, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    const TICK tickInitial = GetTickCount();
    while ( ( GetTickCount() - tickInitial ) <= ( 2 * dtickTimeout ) )
    {
        Sleep( dtickTimeout / 10 );
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, dtickTimeout, 0 );
    }


    OSTestCheck( semTimerShot.CAvail() == 0 );


    semTimerShot.Acquire();


    Sleep( 2 * dtickTimeout );
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestConcurrentBackedUpCallbacksStatesWorkFine, 0, "Tests that two concurrently running callbacks put the state to a good value." );
ERR OSTimerTaskTestConcurrentBackedUpCallbacksStatesWorkFine::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "ReleaseNappyCallback" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaserThenNapTime, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    ULONG cAcquired = 0;
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    UtilSleep( 100 );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    wprintf( L"Done.\n" );


    UtilSleep( 100 );

    while( cAcquired < 2 )
    {
        semTimerShot.Acquire();
        cAcquired++;
    }
    OSTestCheck( 2 == cAcquired );
    Assert( semTimerShot.FTryAcquire() == 0 );
    Assert( semTimerShot.CAvail() == 0 );
    wprintf( L"\tHad %d callbacks.\n", cAcquired );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestConcurrentDoubleBackedUpCallbacksStatesWorkFine, 0, "Tests that two concurrently running callbacks put the state to a good value." );
ERR OSTimerTaskTestConcurrentDoubleBackedUpCallbacksStatesWorkFine::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "SlowNappyCallback" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaserThenNapTime, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    ULONG cAcquired = 0;
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    semTimerShot.Acquire();
    cAcquired++;

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
    C_ASSERT( g_dtickSleepyNapTimeMax / 4 > 30 );
    UtilSleep( g_dtickSleepyNapTimeMax / 4 );
    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    UtilSleep( g_dtickSleepyNapTimeMax / 4 );
    OnDebug( OSTestCheck( FOSTimerTaskActive( g_rgposttTesters[0] ) ) );

    wprintf( L"Done.\n" );


    UtilSleep( g_dtickSleepyNapTimeMax + 100 );

    cAcquired = cAcquired + CSemaphoreCollect( &semTimerShot );
    OSTestCheck( 2 == cAcquired );
    Assert( semTimerShot.FTryAcquire() == 0 );
    Assert( semTimerShot.CAvail() == 0 );
    wprintf( L"\tHad %d callbacks.\n", cAcquired );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestConcurrentTripleBackedUpCallbacksStatesWorkFine, 0, "Tests that two concurrently running callbacks put the state to a good value." );
ERR OSTimerTaskTestConcurrentTripleBackedUpCallbacksStatesWorkFine::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "SlowNappyCallback" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaserThenNapTime, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    ULONG cAcquired = 0;
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    semTimerShot.Acquire();
    cAcquired++;

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
    C_ASSERT( g_dtickSleepyNapTimeMax / 4 > 30 );
    UtilSleep( g_dtickSleepyNapTimeMax / 4 );
    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    UtilSleep( g_dtickSleepyNapTimeMax / 4 );
    OnDebug( OSTestCheck( FOSTimerTaskActive( g_rgposttTesters[0] ) ) );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
    OnDebug( OSTestCheck( FOSTimerTaskActive( g_rgposttTesters[0] ) ) );

    if( ( rand() % 2 ) == 0 )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
        OnDebug( OSTestCheck( FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }

    wprintf( L"Done.\n" );


    UtilSleep( g_dtickSleepyNapTimeMax + 100 );

    cAcquired = cAcquired + CSemaphoreCollect( &semTimerShot );
    OSTestCheck( 2 == cAcquired );
    Assert( semTimerShot.FTryAcquire() == 0 );
    Assert( semTimerShot.CAvail() == 0 );
    wprintf( L"\tHad %d callbacks.\n", cAcquired );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestScheduleSmallStress, 0x0, "Tests 'concurrent' or at least competitive schedules against the callback going off are all state / thread safe and consistent." );
ERR OSTimerTaskTestScheduleSmallStress::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "StressCallbacksReleases" ) );
    const TICK dtickScheduleMax = 40;
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaser, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    wprintf( L"\tStart schedule race:" );
    BOOL fCancelled = fFalse;
    for( TICK dtickSchedule = 0; dtickSchedule < dtickScheduleMax; dtickSchedule++ )
    {
        fCancelled = fFalse;
        OSTimerTaskScheduleTask( g_rgposttTesters[0], (void*)fTrue, dtickSchedule, 0 );
        if ( ( rand() % 8 ) == 3 )
        {
            OSTimerTaskCancelTask( g_rgposttTesters[0] );
            fCancelled = fTrue;
        }
        UtilSleep( dtickSchedule );
        if ( !fCancelled && ( rand() % 8 ) == 3 )
        {
            OSTimerTaskCancelTask( g_rgposttTesters[0] );
            fCancelled = fTrue;
        }
        if ( ( dtickSchedule % 10 ) == 2 )
        {
            OnDebug( wprintf( L"." ) );
        }
    }
    wprintf( L"Done.\n" );


    UtilSleep( 2 * dtickScheduleMax );
    ULONG cCallbacks = CSemaphoreCollect( &semTimerShot );
    wprintf( L"\tHad %d callbacks.\n", cCallbacks );


    Sleep( 2 * dtickScheduleMax );
    OSTestCheck( semTimerShot.CAvail() == 0 );


    if ( !fCancelled )
    {
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
    }
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}


CUnitTest( OSTimerTaskTestScheduleSmallStressAgainstSlowCallback, 0, "Tests 'concurrent' or at least competitive schedules against the callback (even when it is _slow_ and so multiple callbacks back up) going off are all state / thread safe and consistent." );
ERR OSTimerTaskTestScheduleSmallStressAgainstSlowCallback::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "SlowCallbacksReleases" ) );
    const TICK dtickScheduleMax = 20;
    const TICK dtickRandSleepMaxSaved = g_dtickRandSleepMax;
    OSTestTimerPreamble();

    g_dtickRandSleepMax = dtickScheduleMax * 2;

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaserRuntimeCtxIncrementerRandSleep, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    wprintf( L"\tStart schedule race:" );
    for( TICK dtickSchedule = 0; dtickSchedule < dtickScheduleMax; dtickSchedule++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, dtickSchedule, 0 );
        UtilSleep( dtickSchedule );
        if ( ( dtickSchedule % 10 ) == 2 )
        {
            OnDebug( wprintf( L"." ) );
        }
    }
    wprintf( L"Done.\n" );


    UtilSleep( 2 * dtickScheduleMax + g_dtickRandSleepMax );
    ULONG cCallbacks = CSemaphoreCollect( &semTimerShot );
    wprintf( L"\tHad %d callbacks.\n", cCallbacks );


    Sleep( 2 * dtickScheduleMax );
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    g_dtickRandSleepMax = dtickRandSleepMaxSaved;
    return err;
}




CUnitTest( OSTimerTaskTestCanMoveUp, 0, "Tests that rescheduling a timer earlier takes nearly immediate effect both in timing and context." );
ERR OSTimerTaskTestCanMoveUp::ErrTest()
{
    CSemaphore semGlobal( CSyncBasicInfo( "GlobalContext" ) );
    CSemaphore semRuntime( CSyncBasicInfo( "RuntimeContext" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestSelfReschedulerAndReleaser, &semGlobal, &g_rgposttTesters[0]  ) );

    TICK dtickCheckFuzziness = 30;
    g_dtickDelayMax = 50000;
    g_cRuns = 0;

    
    const TICK tickStart = TickOSTimeCurrent();

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 300, 0 );

    Sleep( 40 );

    OSTestCheck( semGlobal.CAvail() == 0 );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], &semRuntime, 14, 0 );
    
    OSTestCheck( semGlobal.CAvail() == 0 );
    semRuntime.Acquire();

    const TICK tickEnd = TickOSTimeCurrent();

    OSTestCheck( (INT)( 40 + 14 - dtickCheckFuzziness ) < DtickDelta( tickStart, tickEnd ) );
    OSTestCheck( (INT)( 40 + 14 + dtickCheckFuzziness * 2 ) > DtickDelta( tickStart, tickEnd ) );

    RandSleep();

    OSTestCheck( semGlobal.CAvail() == 0 );
    OSTestCheck( semRuntime.CAvail() == 0 );
    
    OSTimerTaskCancelTask( g_rgposttTesters[0] );


    dtickCheckFuzziness = 20;

    const TICK tickStart2 = TickOSTimeCurrent();

    OSTimerTaskScheduleTask( g_rgposttTesters[0], &semRuntime, 300, 0 );

    OSTestCheck( semGlobal.CAvail() == 0 );

    OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );

    OSTestCheck( semRuntime.CAvail() == 0 );
    semGlobal.Acquire();

    const TICK tickEnd2 = TickOSTimeCurrent();

    OSTestCheck( (INT)0 <= DtickDelta( tickStart2, tickEnd2 ) );
    OSTestCheck( (INT)40 > DtickDelta( tickStart2, tickEnd2 ) );

    RandSleep();

    OSTestCheck( semGlobal.CAvail() == 0 );
    OSTestCheck( semRuntime.CAvail() == 0 );
    
    OSTimerTaskCancelTask( g_rgposttTesters[0] );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

DWORD DwRandNonZero()
{
    UINT r;
    C_ASSERT( sizeof( r ) == sizeof( DWORD ) );
    OnDebug( errno_t crterr = ) rand_s( &r );
    Assert( crterr == 0 );
    return UlFunctionalMax( r, 1 );
}

void GetUniqueNonZeroRandomValues( _Out_ DWORD * pdwValueA, _Out_ DWORD * pdwValueB, _Out_ DWORD * pdwValueC )
{
    do
    {
        *pdwValueA = DwRandNonZero();
        *pdwValueB = DwRandNonZero();
        *pdwValueC = DwRandNonZero();
    }
    while( *pdwValueA == *pdwValueB ||
            *pdwValueA == *pdwValueC ||
            *pdwValueB == *pdwValueC );
}

CUnitTest( OSTimerTaskTestRescheduleBeforeExecuteSupplantsRuntimeContext, 0, "Tests that a the runtime context which is supposed to be delivered to the next callback can be supplanted by an subsequent call to schedule." );
ERR OSTimerTaskTestRescheduleBeforeExecuteSupplantsRuntimeContext::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "OneShotTimer" ) );
    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    OSTestCheck( semTimerShot.CAvail() == 0 );
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaser, &semTimerShot, &g_rgposttTesters[0] ) );

    ULONG_PTR idPreviousRunCancelled;
    DWORD idStackTrash, idWillNeverGetToRun, idWillGetToRun;
    
    GetUniqueNonZeroRandomValues( &idStackTrash, &idWillNeverGetToRun, &idWillGetToRun );

    idPreviousRunCancelled = idStackTrash;
    wprintf( L"\t\tSchedule[1]: 0x%x, 0x%x (expecting NULL) -->", idWillNeverGetToRun, idPreviousRunCancelled );
    OSTimerTaskScheduleTask( g_rgposttTesters[0], (void*)idWillNeverGetToRun, 100, 0, (const void**)&idPreviousRunCancelled );
    wprintf( L" 0x%x.\n", idPreviousRunCancelled );
    OSTestCheck( idPreviousRunCancelled != idStackTrash );
    OSTestCheck( idPreviousRunCancelled != idWillNeverGetToRun );
    OSTestCheck( idPreviousRunCancelled == NULL );

    idPreviousRunCancelled = idStackTrash;
    wprintf( L"\t\tSchedule[2]: 0x%x, 0x%x (expecting cancel = 0x%x) -->", idWillNeverGetToRun, idWillGetToRun, idWillNeverGetToRun );
    OSTimerTaskScheduleTask( g_rgposttTesters[0], (void*)idWillGetToRun , 14, 0, (const void**)&idPreviousRunCancelled );
    wprintf( L" 0x%x.\n", idPreviousRunCancelled );
    OSTestCheck( idPreviousRunCancelled != idStackTrash );
    OSTestCheck( idPreviousRunCancelled != idWillGetToRun );
    OSTestCheck( idPreviousRunCancelled == idWillNeverGetToRun );

    OSTestCheck( semTimerShot.CAvail() == 0 || semTimerShot.CAvail() == 1  );

    Sleep( 32 );

    if ( semTimerShot.CAvail() == 1 )
    {
        Sleep( 50 );
        OSTestCheck( semTimerShot.CAvail() == 1 );
    }

    OSTestCheck( semTimerShot.CAvail() == 1 );
    semTimerShot.Acquire();

    idPreviousRunCancelled = idStackTrash;
    wprintf( L"\t\tSchedule[2]: 0x%x, 0x%x (expecting no cancel of previous / NULL, as we're after firing) -->", idWillNeverGetToRun, idWillGetToRun );
    OSTimerTaskScheduleTask( g_rgposttTesters[0], (void*)fTrue , 0, 0, (const void**)&idPreviousRunCancelled );
    wprintf( L" 0x%x.\n", idPreviousRunCancelled );
    OSTestCheck( idPreviousRunCancelled != idStackTrash );
    OSTestCheck( idPreviousRunCancelled != idWillNeverGetToRun );
    OSTestCheck( idPreviousRunCancelled != idWillGetToRun );
    OSTestCheck( idPreviousRunCancelled == NULL );

    semTimerShot.Acquire();


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskTestConcurrentCancelStopsCallbackAndScrubsRuntimeContext, 0, "Tests that two concurrently running callbacks put the state to a good value." );
ERR OSTimerTaskTestConcurrentCancelStopsCallbackAndScrubsRuntimeContext::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "ReleaseNappyCallback" ) );
    OSTestTimerPreamble();
    ULONG rgulSignals[4];

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaserThenNapTime, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    ULONG cAcquired = 0;
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskScheduleTask( g_rgposttTesters[0], &rgulSignals[0], 0, 0 );


    UtilSleep( 100 );


    ULONG * pul = NULL;
    OSTimerTaskScheduleTask( g_rgposttTesters[0], &rgulSignals[1], 0, 0, (const void**)&pul );
    OSTestCheck( pul == NULL );


    pul = NULL;
    OSTimerTaskCancelTask( g_rgposttTesters[0], (const void**)&pul );
    OSTestCheck( pul == &rgulSignals[1] );
    UtilSleep( 100 );


    while( cAcquired < 1 )
    {
        semTimerShot.Acquire();
        cAcquired++;
    }
    OSTestCheck( 1 == cAcquired );
    Assert( semTimerShot.FTryAcquire() == 0 );
    Assert( semTimerShot.CAvail() == 0 );
    wprintf( L"\tHad %d callbacks.\n", cAcquired );


    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}






const WCHAR * WszBool( _In_ const BOOL fBool )
{
    return fBool ? L"fTrue" : L"fFalse";
}

JET_ERR ErrStressSchedule( POSTIMERTASK postt, CSemaphore * psemCtx, _In_ const BOOL fCheckedRuntimeContext, _In_ const TICK dtickSlop )
{
    ERR err = JET_errSuccess;
    TICK dtickScheduleMax = 200;

    wprintf( L"\n\t\t    fCheckedRuntimeContext(=%ws), dtickSlop(=%d):", 
                    WszBool( fCheckedRuntimeContext ), dtickSlop );
    for( LONG dtickDeltaDelta = -2; dtickDeltaDelta < 3; dtickDeltaDelta++ )
    {
        const ULONG crunMax = 201;
        const LONG crunCheckNoOverrun = -9;
        LONG rgcruns[ 201 * 2] = { 0 };
        rgcruns[crunMax] = crunCheckNoOverrun;
        C_ASSERT( crunMax < _countof( rgcruns ) );
        ULONG csched = 0;
        ULONG csupplanted = 0;
        wprintf( L"\n\t\t 	   dtickDeltaDelta(=%d): ", dtickDeltaDelta );
        for( TICK dtickSchedule = 0; dtickSchedule < dtickScheduleMax; dtickSchedule++ )
        {
            const INT dtickScheduleFinal = dtickSchedule + dtickDeltaDelta;
            LONG * pcrunCancelled = NULL;
            OSTimerTaskScheduleTask( postt, 
                                        &(rgcruns[csched]),
                                        max( dtickScheduleFinal, 0 ),
                                        dtickSlop,
                                        fCheckedRuntimeContext ? (const void**)&pcrunCancelled : NULL );
            if ( pcrunCancelled )
            {
                csupplanted++;
                AtomicIncrement( pcrunCancelled );
            }
            csched++;
            UtilSleep( dtickSchedule + dtickSlop );
            if ( ( dtickSchedule % 10 ) == 2 )
            {
                OnDebug( wprintf( L"." ); );
            }
        }
        wprintf( L" enter collect callbacks phase ..." );

        ULONG cruns = CSemaphoreWaitCollect( psemCtx, 
                            dtickScheduleMax * 2 + dtickDeltaDelta * 2 + dtickSlop * 2 + g_dtickRandSleepMax,
                            fCheckedRuntimeContext ? ( csched - csupplanted ) : 0 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( postt ) ) );

        wprintf( L" %d/%d runs.", cruns, csupplanted );
        if ( fCheckedRuntimeContext )
        {
            wprintf( L" (checking runs - " );
            OSTestCheck( rgcruns[crunMax] == crunCheckNoOverrun );
            for( ULONG isched = 0; isched < csched; isched++ )
            {
                OSTestCheck( rgcruns[isched] == 1 );
            }
            OSTestCheck( cruns + csupplanted == csched );
            wprintf( L" all runs accounted for!). Runs %d", CSemaphoreCollect( psemCtx ) );
        }
    }

HandleError:
    return err;
}

CUnitTest( OSTimerTaskTestVariousDelaysLongStress, bitExplicitOnly, "Tests that a one-shot timer that gets repeatedly rescheduled (but not cancelled) only runs once after the last reschedule." );
ERR OSTimerTaskTestVariousDelaysLongStress::ErrTest()
{
    CSemaphore semTimerShot( CSyncBasicInfo( "CallbacksCountForLongStress" ) );
    const TICK dtickTimeout = 1000;
    const TICK dtickRandSleepMaxSaved = g_dtickRandSleepMax;

    OSTestTimerPreamble();

    printf( "\t%s\n", __FUNCTION__ );

    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestGroupCtxSemaphoreReleaserRuntimeCtxIncrementerRandSleep, &semTimerShot, &g_rgposttTesters[0] ) );


    RandSleep();
    OSTestCheck( semTimerShot.CAvail() == 0 );


    wprintf( L"\tStart schedule race:" );
    for( BOOL fCheckedRuntimeContext = fFalse; fCheckedRuntimeContext < 2; fCheckedRuntimeContext++ )
    {
        for( BOOL fSloppySleep = fFalse; fSloppySleep < 2; fSloppySleep++ )
        {
#ifdef DEBUG
            TICK rgtickCallbackMaxSleep[] = { (TICK)-1, 500 };
#else
            TICK rgtickCallbackMaxSleep[] = { (TICK)-1, 0, 1, 2, 32, 100, 1300 };
#endif
            for( INT i = 0; i < _countof( rgtickCallbackMaxSleep ); i++ )
            {
                g_dtickRandSleepMax = rgtickCallbackMaxSleep[i];
                wprintf( L"\n\t\tg_dtickRandSleepMax(=%d):", g_dtickRandSleepMax );
                for( TICK dtickSlop = 0; dtickSlop < 12; dtickSlop = dtickSlop * 2 + 1 )
                {
                    if ( fSloppySleep == fFalse && dtickSlop != 0 )
                    {
                        continue;
                    }

                    err = ErrStressSchedule( g_rgposttTesters[0], &semTimerShot, fCheckedRuntimeContext, dtickSlop );
                    OSTestCall( err );
                }
            }
        }
    }
    wprintf( L"\n\tDone.\n" );


    Sleep( 2 * dtickTimeout );
    OSTestCheck( semTimerShot.CAvail() == 0 );


    OSTimerTaskCancelTask( g_rgposttTesters[0] );
    OSTestCheck( semTimerShot.CAvail() == 0 );

HandleError:
    g_dtickRandSleepMax = dtickRandSleepMaxSaved;
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskConcurrentRescheduleStress, bitExplicitOnly, "Tests that a timer task can cancel a task concurrent w/ the task trying to reschedule itself and the cancel always wins." );
ERR OSTimerTaskConcurrentRescheduleStress::ErrTest()
{
    OSTestTimerPreamble();
    Call( ErrOSTimerTaskCreate( (PfnTimerTask)OSTimerTestSelfRescheduler, NULL, &g_rgposttTesters[0]  ) );

    g_cRuns = 0;

    g_dtickDelayMax = 1000;
    g_dtickFuzzMax = 1000;

    g_fRandomizedDelays = fTrue;

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For immediate schedule/cancel, got %d runs\n", g_cRuns );
    OSTestCheck( g_cRuns < 50 );

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
        UtilSleep( 0 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        UtilSleep( 0 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For Sleep(0) schedule/cancel, got %d runs\n", g_cRuns );

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
        UtilSleep( 1 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        UtilSleep( 1 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For Sleep(1) schedule/cancel, got %d runs\n", g_cRuns );
    OSTestCheck( g_cRuns > 20 );

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, 0 );
        UtilSleep( i ? rand() % i : 0 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        UtilSleep( i ? rand() % i : 0 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For Sleep(rand()) schedule/cancel, got %d runs\n", g_cRuns );
    OSTestCheck( g_cRuns > 20 );

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, i ? rand() % i : 0, 0 );
        UtilSleep( i ? rand() % i : 0 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        UtilSleep( i ? rand() % i : 0 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For Sleep(rand()) schedule/cancel, with rand initial wait, got %d runs\n", g_cRuns );
    OSTestCheck( g_cRuns > 1 );
    OSTestCheck( g_cRuns < 99 );

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, 0, i ? rand() % i : 0 );
        UtilSleep( i ? rand() % i : 0 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        UtilSleep( i ? rand() % i : 0 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For Sleep(rand()) schedule/cancel, with rand subsequent wait, got %d runs\n", g_cRuns );

    g_cRuns = 0;
    for( ULONG i = 0; i < 100; i++ )
    {
        OSTimerTaskScheduleTask( g_rgposttTesters[0], NULL, i ? rand() % i : 0, i ? rand() % i : 0 );
        UtilSleep( i ? rand() % i : 0 );
        OSTimerTaskCancelTask( g_rgposttTesters[0] );
        UtilSleep( i ? rand() % i : 0 );
        OnDebug( OSTestCheck( !FOSTimerTaskActive( g_rgposttTesters[0] ) ) );
    }
    wprintf( L"RESULTS: For Sleep(rand()) schedule/cancel, with rand initial and subsequent waits, got %d runs\n", g_cRuns );
    OSTestCheck( g_cRuns > 1 );
    OSTestCheck( g_cRuns < 99 );

HandleError:
    OSTestTimerCleanup( 1 );
    return err;
}

CUnitTest( OSTimerTaskSchedulingTimerStress, 0, "SchedulingTimerStress." );
ERR OSTimerTaskSchedulingTimerStress::ErrTest()
{
    printf( "\t%s\n", __FUNCTION__ );

    COSLayerPreInit oslayer;
    Enforce( ErrOSInit() == JET_errSuccess );
    const JET_ERR err = ErrSchedulingTimerStress_( SchedulingOsTimerStressTask );
    OSTerm();

    return err;
}

CUnitTest( OSTimerTaskSelfSchedulingTimerStress, 0, "OSTimerTaskSelfSchedulingTimerStress." );
ERR OSTimerTaskSelfSchedulingTimerStress::ErrTest()
{
    printf( "\t%s\n", __FUNCTION__ );

    COSLayerPreInit oslayer;
    Enforce( ErrOSInit() == JET_errSuccess );
    const JET_ERR err = ErrSchedulingTimerStress_( SelfSchedulingOsTimerStressTask );
    OSTerm();

    return err;
}
