// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"



#define WeightedAvePrevRuns( cRun, cPrevAve )   ( ( ((double)( (cRun) - 1 )) / (double)(cRun) ) * (cPrevAve) )
#define WeightedAveNewRun( cRun, cNewAve )      ( ( ((double)1) / (double)(cRun) ) * (cNewAve) )

NTOSFuncPtr( g_pfnCreateThreadpoolTimer, g_mwszzThreadpoolLibs, CreateThreadpoolTimer, oslfExpectedOnWin6 | oslfRequired );
NTOSFuncVoid( g_pfnSetThreadpoolTimer, g_mwszzThreadpoolLibs, SetThreadpoolTimer, oslfExpectedOnWin6 | oslfRequired );
NTOSFuncVoid( g_pfnWaitForThreadpoolTimerCallbacks, g_mwszzThreadpoolLibs, WaitForThreadpoolTimerCallbacks, oslfExpectedOnWin6 | oslfRequired );
NTOSFuncVoid( g_pfnCloseThreadpoolTimer, g_mwszzThreadpoolLibs, CloseThreadpoolTimer, oslfExpectedOnWin6 | oslfRequired );


class COSTimerTaskEntry
{
    public:

        static SIZE_T OffsetOfILE() { return OffsetOf( COSTimerTaskEntry, m_ile ); }

    private:

        
        enum State
        {
            stateInvalid = 0,
            stateInactive,
            stateScheduled,
            stateRunning,
            stateCancelling,
            stateCancelled,
            stateDeleted,
        };

        CInvasiveList< COSTimerTaskEntry, OffsetOfILE >::CElement   m_ile;

        State               m_state;
        LONG                m_cInCallback;

        TICK                m_dtickDelay;
        TICK                m_dtickSlop;
        PTP_TIMER           m_pOSThreadpoolTimer;
#ifdef DEBUG
        BOOL                m_fThreadpoolTimerUp;
#endif

        PfnTimerTask const  m_pfnTask;
        VOID * const        m_pvTaskGroupContext;
        VOID *              m_pvTaskRuntimeContext;

        CCriticalSection    m_critSchedule;
        INT64               m_idSchedule;
        INT64               m_idRun;
        CSemaphore          m_semExec;
        DWORD               m_tidExec;

        DWORD               m_tidExecLast;
        TICK                m_tickExecStartLast;
        TICK                m_tickExecEndLast;
        QWORD               m_cRuns;
        QWORD               m_cSkippedRuns;
#ifdef DEBUG
        QWORD               m_cSupplantedRuns;
        TICK                m_tickLastSkippedRun;
        TICK                m_tickLastSupplantedRun;
#endif
        TICK                m_tickRunAverage;
#ifdef DEBUG
        TICK                m_tickCreated;
#endif
        TICK                m_tickLastSchedule;
        TICK                m_tickLastReSchedule;
        TICK                m_tickLastSelfSchedule;
        TICK                m_tickLastQuiesce;
#ifdef DEBUG
        TICK                m_tickCancelling;
        TICK                m_tickCancelled;
        TICK                m_tickDeleted;
#endif
        

    public:

        static ERR ErrOSTimerTaskICreate(
            PfnTimerTask const              pfnTask,
            const void * const              pvTaskGroupContext,
            POSTIMERTASK *                  pposttTimerHandle );

        VOID OSTimerTaskScheduleITask(
            const void * const              pvRuntimeContext,
            const TICK                      dtickMinDelay,
            const TICK                      dtickSlopDelay,
            const void **                   pvRuntimeContextCancelled  );

        VOID OSTimerTaskCancelITask( _Out_opt_ const void ** ppvRuntimeContextCancelled );
    
        static VOID OSTimerTaskIDelete( POSTIMERTASK postt );

#ifdef DEBUG
        BOOL FOSTimerTaskIActive() const
        {
            return m_state == stateScheduled || m_state == stateRunning;
        }
#endif

    private:

        COSTimerTaskEntry(
            PfnTimerTask        pfnTask,
            const VOID * const  pvTaskGroupContext );
        ~COSTimerTaskEntry();

    private:

        static VOID WINAPI OSTimerTaskIThreadpoolTimerCompletion(
            _Inout_     PTP_CALLBACK_INSTANCE       Instance,
            _Inout_     PVOID                       pvContext,
            _Inout_     PTP_TIMER                   Timer );


};

COSTimerTaskEntry::COSTimerTaskEntry(
                    PfnTimerTask        pfnTask,
                    const VOID * const      pvTaskGroupContext ) :
        m_state( stateInvalid ),
        m_cInCallback( 0 ),
        m_dtickDelay( 0 ),
        m_dtickSlop( 0 ),
        m_pOSThreadpoolTimer( NULL ),
        m_pfnTask( pfnTask ),
        m_pvTaskGroupContext( (VOID *)pvTaskGroupContext ),
        m_pvTaskRuntimeContext( NULL ),
        m_semExec( CSyncBasicInfo( "COSTimerTaskEntry::m_semExec" ) ),
        m_critSchedule( CLockBasicInfo( CSyncBasicInfo( _T( "m_critSchedule" ) ), rankTimerTaskEntry, 0 ) ),
        m_idSchedule( 0 ),
        m_idRun( 0 ),
        m_tidExec( DWORD( ~0 ) ),
#ifdef DEBUG
        m_fThreadpoolTimerUp( fFalse ),
#endif
        m_tidExecLast( DWORD( ~0 ) ),
        m_tickExecStartLast( 0 ),
        m_tickExecEndLast( 0 ),
        m_cRuns( 0 ),
        m_cSkippedRuns( 0 ),
#ifdef DEBUG
        m_cSupplantedRuns( 0 ),
        m_tickLastSkippedRun( 0 ),
        m_tickLastSupplantedRun( 0 ),
        m_tickCreated( 0 ),
#endif
        m_tickLastSchedule( 0 ),
        m_tickLastReSchedule( 0 ),
        m_tickLastSelfSchedule( 0 ),
        m_tickLastQuiesce( 0 ),
#ifdef DEBUG
        m_tickCancelling( 0 ),
        m_tickCancelled( 0 ),
        m_tickDeleted( 0 ),
#endif
        m_tickRunAverage( 0 )
{
    m_semExec.Release();
}

COSTimerTaskEntry::~COSTimerTaskEntry()
{
    Assert( m_state == COSTimerTaskEntry::stateDeleted );
}

void WINAPI COSTimerTaskEntry::OSTimerTaskIThreadpoolTimerCompletion(
        _Inout_     PTP_CALLBACK_INSTANCE       Instance,
        _Inout_     PVOID                       pvContext,
        _Inout_     PTP_TIMER                   Timer )
{
    COSTimerTaskEntry * const ptte = (COSTimerTaskEntry*)pvContext;


    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );


    AtomicIncrement( &(ptte->m_cInCallback) );
    

    Postls()->posttExecuting = ptte;


    ptte->m_semExec.Acquire();


    ptte->m_critSchedule.Enter();

    const __int64 idschedStart = ptte->m_idSchedule;
    if ( ptte->m_idRun == idschedStart )
    {
        
        Assert( ptte->m_pvTaskRuntimeContext == NULL );
        OnDebug( ptte->m_tickLastSkippedRun = TickOSTimeCurrent() );
        ptte->m_cSkippedRuns++;
        ptte->m_critSchedule.Leave();
        goto QuitThread;
    }


    Assert( ptte->m_state == COSTimerTaskEntry::stateScheduled ||
                ptte->m_state == COSTimerTaskEntry::stateCancelling );


    void * const pvTaskRuntimeContext = ptte->m_pvTaskRuntimeContext;
    ptte->m_pvTaskRuntimeContext = NULL;
    ptte->m_idRun = ptte->m_idSchedule;


    ptte->m_state = COSTimerTaskEntry::stateRunning;

    ptte->m_critSchedule.Leave();


    OSTrace( JET_tracetagTimerQueue, OSFormat( "OSTimerTaskIThreadpoolTimerCompletion( %p ) %d ms since last run.\n", ptte->m_pfnTask, DtickDelta( ptte->m_tickExecEndLast, TickOSTimeCurrent() ) ) );
    ETTimerTaskRun( ptte, ptte->m_pfnTask, ptte->m_pvTaskGroupContext, pvTaskRuntimeContext, ptte->m_cRuns );

    ptte->m_tidExec = GetCurrentThreadId();
    const TICK tickStart = TickOSTimeCurrent();


    ptte->m_pfnTask( ptte->m_pvTaskGroupContext, pvTaskRuntimeContext );

    Assert( ptte->m_state == COSTimerTaskEntry::stateRunning ||
            ptte->m_state == COSTimerTaskEntry::stateCancelling );


    const TICK tickEnd = TickOSTimeCurrent();


    ptte->m_tidExecLast         = ptte->m_tidExec;
    ptte->m_tickExecStartLast   = tickStart;
    ptte->m_tickExecEndLast     = tickEnd;
    ptte->m_cRuns++;
    ptte->m_tickRunAverage      = (TICK) (  WeightedAvePrevRuns( ptte->m_cRuns, ptte->m_tickRunAverage ) +
                                            WeightedAveNewRun( ptte->m_cRuns, DtickDelta( tickStart, tickEnd ) ) );


    ptte->m_critSchedule.Enter();

    if ( ptte->m_state == COSTimerTaskEntry::stateRunning &&
            ptte->m_idRun == ptte->m_idSchedule )
    {

        ptte->m_tickLastQuiesce = TickOSTimeCurrent();
        ptte->m_state = COSTimerTaskEntry::stateInactive;
    }
    else if ( ptte->m_state == COSTimerTaskEntry::stateRunning &&
            ptte->m_idRun != ptte->m_idSchedule )
    {
        ptte->m_state = COSTimerTaskEntry::stateScheduled;
    }
    Assert( ptte->m_state != COSTimerTaskEntry::stateRunning );

    ptte->m_critSchedule.Leave();

    ptte->m_tidExec = DWORD( ~0 );

QuitThread:

    ptte->m_semExec.Release();


    AtomicDecrement( &(ptte->m_cInCallback) );


    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );


    Postls()->posttExecuting = NULL;
}

typedef CInvasiveList< COSTimerTaskEntry, COSTimerTaskEntry::OffsetOfILE > COSTimerTaskEntryList;

COSTimerTaskEntryList g_ilTimerTaskList;

CCriticalSection g_critTimerTaskList( CLockBasicInfo( CSyncBasicInfo( _T( "g_critTimerTaskList" ) ), rankTimerTaskList, 0 ) );


ERR COSTimerTaskEntry::ErrOSTimerTaskICreate(
    PfnTimerTask const              pfnTask,
    const void * const              pvTaskGroupContext,
    POSTIMERTASK *                  pposttTimerHandle )
{
    ERR                             err = JET_errSuccess;
    COSTimerTaskEntry *             ptte = NULL;


    g_critTimerTaskList.Enter();


    if ( pfnTask == NULL )
    {
        AssertSz( fFalse, "pfnTask should've been specified" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    for ( ptte = g_ilTimerTaskList.PrevMost(); ptte; ptte = g_ilTimerTaskList.Next( ptte ) )
    {
        if ( ptte->m_pfnTask == pfnTask && ptte->m_pvTaskGroupContext == pvTaskGroupContext )
        {
            break;
        }
    }


    if ( ptte )
    {
        AssertSz( fFalse, "This is a usage error to try to schedule the same task." );
        ptte = NULL;
        Error( ErrERRCheck( JET_errTaskDropped ) );
    }

    Alloc( ptte = new COSTimerTaskEntry( pfnTask, pvTaskGroupContext ) );

    Assert( ptte->m_pOSThreadpoolTimer == NULL );


    Alloc( ptte->m_pOSThreadpoolTimer = g_pfnCreateThreadpoolTimer( COSTimerTaskEntry::OSTimerTaskIThreadpoolTimerCompletion, ptte, NULL ) );

    ptte->m_state = COSTimerTaskEntry::stateInactive;

    g_ilTimerTaskList.InsertAsPrevMost( ptte );


    OSTrace( JET_tracetagTimerQueue, OSFormat( "CreateThreadpoolTimer() -> %p \n", ptte->m_pOSThreadpoolTimer ) );

    ptte->m_tickExecEndLast = TickOSTimeCurrent();
    OnDebug( ptte->m_tickCreated = ptte->m_tickExecEndLast );
    OnDebug( ptte->m_fThreadpoolTimerUp = fTrue );


    *pposttTimerHandle = (POSTIMERTASK*)ptte;
    ptte = NULL;
    Assert( JET_errSuccess == err );

HandleError:

    if ( err < JET_errSuccess )
    {
        if ( ptte )
        {
            Assert( ptte->m_pOSThreadpoolTimer == NULL );
            ptte->m_state = COSTimerTaskEntry::stateDeleted;
            OnDebug( ptte->m_tickDeleted = (TICK)-1 );
            delete ptte;
        }
    }

    g_critTimerTaskList.Leave();

    return err;
}

VOID COSTimerTaskEntry::OSTimerTaskScheduleITask(
    const void * const              pvRuntimeContext,
    const TICK                      dtickMinDelay,
    const TICK                      dtickSlopDelay,
    const void **                   ppvRuntimeContextCancelled )
{
    COSTimerTaskEntry * const       ptte = this;


    Assert( dtickMinDelay != INFINITE );
    Assert( dtickMinDelay < INT_MAX );

    if ( ppvRuntimeContextCancelled )
    {
        *ppvRuntimeContextCancelled = NULL;
    }

    const BOOL fCallbackReScheduling = ( Postls()->posttExecuting != NULL );
    const BOOL fSelfReScheduling = ( ptte->m_tidExec == DwUtilThreadId() );

    if ( fCallbackReScheduling )
    {
    }
    

    m_critSchedule.Enter();


    if ( ptte->m_state == COSTimerTaskEntry::stateCancelling )
    {

        m_critSchedule.Leave();
        return;
    }


    m_tickLastSchedule = TickOSTimeCurrent();
    if ( fCallbackReScheduling )
    {
        m_tickLastReSchedule = m_tickLastSchedule;
    }
    if ( fSelfReScheduling )
    {
        m_tickLastSelfSchedule = m_tickLastSchedule;
    }

    Expected( ptte->m_state == COSTimerTaskEntry::stateInactive ||
                ptte->m_state == COSTimerTaskEntry::stateScheduled ||
                ptte->m_state == COSTimerTaskEntry::stateRunning ||
                ptte->m_state == COSTimerTaskEntry::stateCancelling ||
                ptte->m_state == COSTimerTaskEntry::stateCancelled );



    LARGE_INTEGER dftMinDelay;
    dftMinDelay.QuadPart = -( ((LONGLONG)dtickMinDelay) * (LONGLONG)10000 );
    Assert( dftMinDelay.QuadPart <= 0 );


    ptte->m_dtickDelay = dtickMinDelay;
    ptte->m_dtickSlop = dtickSlopDelay;

    if ( ptte->m_state != COSTimerTaskEntry::stateRunning )
    {
        ptte->m_state = COSTimerTaskEntry::stateScheduled;
    }
    ptte->m_idSchedule++;

    if ( ptte->m_pvTaskRuntimeContext )
    {
        OnDebug( ptte->m_tickLastSupplantedRun = TickOSTimeCurrent() );
        OnDebug( ptte->m_cSupplantedRuns++ );
        if ( ppvRuntimeContextCancelled )
        {
            *ppvRuntimeContextCancelled = ptte->m_pvTaskRuntimeContext;
        }
    }
    ptte->m_pvTaskRuntimeContext = (void*)pvRuntimeContext;


    OSTrace( JET_tracetagTimerQueue, OSFormat( "Scheduling SetThreadpoolTimer() for between %d and %d ms out ...\n",
                dtickMinDelay, dtickMinDelay + dtickSlopDelay ) );
    ETTimerTaskSchedule( ptte, ptte->m_pfnTask, ptte->m_pvTaskGroupContext, ptte->m_pvTaskRuntimeContext, ptte->m_dtickDelay, ptte->m_dtickSlop );

    g_pfnSetThreadpoolTimer( ptte->m_pOSThreadpoolTimer, (FILETIME*)&dftMinDelay, 0, dtickSlopDelay );

    OSTrace( JET_tracetagTimerQueue, OSFormat( "SetThreadpoolTimer() Done.\n" ) );

 
    m_critSchedule.Leave();
}


VOID COSTimerTaskEntry::OSTimerTaskCancelITask( _Out_opt_ const void ** ppvRuntimeContextCancelled )
{
    COSTimerTaskEntry * const ptte = this;

    AssertSz( ptte->m_tidExec != DwUtilThreadId(), "Can not call OSTimerTaskCancelITask from within the callback (case: ptte->m_tidExec != DwUtilThreadId())" );
    AssertSz( Postls()->posttExecuting == NULL, "Can not call OSTimerTaskCancelITask from within the callback (case: Postls()->posttExecuting == NULL)" );

    OnDebug( const State stateT = ptte->m_state );
    Assert( stateT == COSTimerTaskEntry::stateInactive ||
            stateT == COSTimerTaskEntry::stateScheduled ||
            stateT == COSTimerTaskEntry::stateRunning );


    Assert( ptte->m_pOSThreadpoolTimer );
    Assert( ptte->m_fThreadpoolTimerUp );


    OSTrace( JET_tracetagTimerQueue, OSFormat( "OSTimerTaskCancelITask beginning ThreadpoolTimer Shutdown...\n" ) );


    ptte->m_critSchedule.Enter();

    OnDebug( ptte->m_tickCancelling = TickOSTimeCurrent() );
    ptte->m_state = COSTimerTaskEntry::stateCancelling;
    ptte->m_idRun = ptte->m_idSchedule;
    if( ppvRuntimeContextCancelled )
    {
        *ppvRuntimeContextCancelled = ptte->m_pvTaskRuntimeContext;
    }
    ptte->m_pvTaskRuntimeContext = NULL;


    ptte->m_critSchedule.Leave();


    g_pfnWaitForThreadpoolTimerCallbacks( ptte->m_pOSThreadpoolTimer, TRUE );


    g_pfnSetThreadpoolTimer( ptte->m_pOSThreadpoolTimer, NULL, 0, 0 );


    g_pfnWaitForThreadpoolTimerCallbacks( ptte->m_pOSThreadpoolTimer, TRUE );

    Assert( ptte->m_cInCallback == 0 );


    ptte->m_critSchedule.Enter();

    OnDebug( ptte->m_tickCancelled = TickOSTimeCurrent() );
    OSTrace( JET_tracetagTimerQueue, OSFormat( "ThreadpoolTimer Quiesced...\n" ) );

    ptte->m_state = COSTimerTaskEntry::stateCancelled;

    ptte->m_critSchedule.Leave();

    ETTimerTaskCancel( ptte, ptte->m_pfnTask, ptte->m_pvTaskGroupContext );

}

VOID COSTimerTaskEntry::OSTimerTaskIDelete( POSTIMERTASK postt )
{
    COSTimerTaskEntry * const       ptte = (COSTimerTaskEntry *)postt;


    Assert( ptte );

    AssertSz( ptte->m_tidExec != DwUtilThreadId() && Postls()->posttExecuting == NULL, "Can not call OSTimerTaskIDelete from within the callback" );
    Assert( ptte->m_cInCallback == 0 );

    Expected( ptte->m_state == COSTimerTaskEntry::stateCancelled );

    OnDebug( ptte->m_fThreadpoolTimerUp = fFalse );

    g_critTimerTaskList.Enter();


    g_pfnCloseThreadpoolTimer( ptte->m_pOSThreadpoolTimer );
    ptte->m_pOSThreadpoolTimer = NULL;
    ptte->m_state = COSTimerTaskEntry::stateDeleted;
    OnDebug( ptte->m_tickDeleted = TickOSTimeCurrent() );

    OSTrace( JET_tracetagTimerQueue, OSFormat( "ThreadpoolTimer Closed/Released.\n" ) );

    Assert( ptte->m_pOSThreadpoolTimer == NULL );
    Assert( !ptte->m_fThreadpoolTimerUp );


    g_ilTimerTaskList.Remove( ptte );
    delete ptte;

    g_critTimerTaskList.Leave();

}

ERR ErrOSTimerTaskCreate(
    PfnTimerTask const              pfnTask,
    const void * const              pvTaskGroupContext,
    POSTIMERTASK *                  pposttTimerHandle )
{
    return COSTimerTaskEntry::ErrOSTimerTaskICreate( pfnTask, pvTaskGroupContext, pposttTimerHandle );
}

VOID OSTimerTaskScheduleTask(
    POSTIMERTASK                    postt,
    const void * const              pvRuntimeContext,
    const TICK                      dtickMinDelay,
    const TICK                      dtickSlopDelay,
    const void **                   ppvRuntimeContextCancelled )
{
    ((COSTimerTaskEntry *)postt)->OSTimerTaskScheduleITask( pvRuntimeContext, dtickMinDelay, dtickSlopDelay, ppvRuntimeContextCancelled );
}

VOID OSTimerTaskCancelTask( POSTIMERTASK postt, _Out_opt_ const void ** ppvRuntimeContextCancelled )
{
    ((COSTimerTaskEntry *)postt)->OSTimerTaskCancelITask( ppvRuntimeContextCancelled );
}

VOID OSTimerTaskDelete( POSTIMERTASK postt )
{
    COSTimerTaskEntry::OSTimerTaskIDelete( postt );
}

#ifdef DEBUG
BOOL FOSTimerTaskActive( POSTIMERTASK postt )
{
    return ((COSTimerTaskEntry *)postt)->FOSTimerTaskIActive();
}
#endif


void OSTimerQueuePostterm()
{
}


BOOL FOSTimerQueuePreinit()
{

    Assert( g_ilTimerTaskList.FEmpty() );

    return fTrue;
}



void OSTimerQueueTerm()
{

    g_critTimerTaskList.Enter();

    Assert( g_ilTimerTaskList.FEmpty() );
    Assert( g_ilTimerTaskList.PrevMost() == NULL );

    g_critTimerTaskList.Leave();

}


ERR ErrOSTimerQueueInit()
{
    ERR err = JET_errSuccess;

    if ( err < JET_errSuccess )
    {
        OSTimerQueueTerm();
    }
    return err;
}

