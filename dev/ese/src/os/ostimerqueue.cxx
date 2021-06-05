// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


//  helper meta-functions ...

//  note: use these post increment, i.e. expect cRun >= 1, and should be a 64-bit value (to avoid div by zero) ...
#define WeightedAvePrevRuns( cRun, cPrevAve )   ( ( ((double)( (cRun) - 1 )) / (double)(cRun) ) * (cPrevAve) )
#define WeightedAveNewRun( cRun, cNewAve )      ( ( ((double)1) / (double)(cRun) ) * (cNewAve) )

NTOSFuncPtr( g_pfnCreateThreadpoolTimer, g_mwszzThreadpoolLibs, CreateThreadpoolTimer, oslfExpectedOnWin6 | oslfRequired );
NTOSFuncVoid( g_pfnSetThreadpoolTimer, g_mwszzThreadpoolLibs, SetThreadpoolTimer, oslfExpectedOnWin6 | oslfRequired );
NTOSFuncVoid( g_pfnWaitForThreadpoolTimerCallbacks, g_mwszzThreadpoolLibs, WaitForThreadpoolTimerCallbacks, oslfExpectedOnWin6 | oslfRequired );
NTOSFuncVoid( g_pfnCloseThreadpoolTimer, g_mwszzThreadpoolLibs, CloseThreadpoolTimer, oslfExpectedOnWin6 | oslfRequired );

//  our "table" used to match task function pointers with timer handles

class COSTimerTaskEntry
{
    public:

        static SIZE_T OffsetOfILE() { return OffsetOf( COSTimerTaskEntry, m_ile ); }

    private:

        //  state of each task function pointer
        
        enum State
        {
            stateInvalid = 0,
            stateInactive,  // or initialized - but not currently scheduled
            stateScheduled,
            stateRunning,
            stateCancelling,
            stateCancelled,
            stateDeleted,
        };

        //      basic
        //
        CInvasiveList< COSTimerTaskEntry, OffsetOfILE >::CElement   m_ile;

        State               m_state;
        LONG                m_cInCallback;                  // number of threads in thunk callback / OSTimerTaskIThreadpoolTimerCompletion

        //      task info
        //
        TICK                m_dtickDelay;
        TICK                m_dtickSlop;
        PTP_TIMER           m_pOSThreadpoolTimer;
#ifdef DEBUG
        BOOL                m_fThreadpoolTimerUp;
#endif

        //      task user details
        //
        PfnTimerTask const  m_pfnTask;
        VOID * const        m_pvTaskGroupContext;
        VOID *              m_pvTaskRuntimeContext;

        //      control
        //
        CCriticalSection    m_critSchedule;                 // controls main scheduling state variables (e.g. m_state, m_idSchedule, m_idRun, ??)
        INT64               m_idSchedule;                   // unique ID incremented every schedule call
        INT64               m_idRun;                        // the matching ID of run (set to m_idSchedule when callback starts)
        CSemaphore          m_semExec;                      // forces there to be a single thread operating the user's callback function (not NT can have multiple in OSTimerTaskIThreadpoolTimerCompletion)
        DWORD               m_tidExec;                      // TID of the thread that has right to run user callback

        //      stats
        //
        DWORD               m_tidExecLast;
        TICK                m_tickExecStartLast;
        TICK                m_tickExecEndLast;
        QWORD               m_cRuns;                        // number of runs of timer-task's callback function
        QWORD               m_cSkippedRuns;                 // number of runs the thunk callback skipped the user m_pfnTask callback (thunk callback = OSTimerTaskIThreadpoolTimerCompletion)
#ifdef DEBUG
        QWORD               m_cSupplantedRuns;              // number of runs that a 2nd schedule timer task call supplanted a previously scheduled / "timered" task (ONLY accurate if m_pvRuntimeContext is non-NULL)
        TICK                m_tickLastSkippedRun;           // time of last callback skipped due to previous callbacks having same m_idRun
        TICK                m_tickLastSupplantedRun;        // time of last scheduled task supplanted by subsequent call to schedule
#endif
        TICK                m_tickRunAverage;
#ifdef DEBUG
        TICK                m_tickCreated;                  // time of ErrOSTimerTaskICreate for this timer-task
#endif
        TICK                m_tickLastSchedule;             // time of last schedule event / call OSTimerTaskScheduleITask
        TICK                m_tickLastReSchedule;           // time of schedule from a callback (not could be from a different timer-task)
        TICK                m_tickLastSelfSchedule;         // time of schedule from our own timer-task callback (so a self-reschedule)
        TICK                m_tickLastQuiesce;              // time of last callback quiescing state to inactive
#ifdef DEBUG
        TICK                m_tickCancelling;               // time of the cancel timer-task starts
        TICK                m_tickCancelled;                // time of the cancel timer-task completes
        TICK                m_tickDeleted;                  // time of delete task / call OSTimerTaskIDelete
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
    //  check for a clean tear down
    Assert( m_state == COSTimerTaskEntry::stateDeleted );
}

void WINAPI COSTimerTaskEntry::OSTimerTaskIThreadpoolTimerCompletion(
        _Inout_     PTP_CALLBACK_INSTANCE       Instance,
        _Inout_     PVOID                       pvContext,
        _Inout_     PTP_TIMER                   Timer )
{
    COSTimerTaskEntry * const ptte = (COSTimerTaskEntry*)pvContext;

    //  expect to have a clean sync state as a pre-check

    //  can assert stronger statements here in than in JET API as we're guaranteed 
    //  to be at the top of a stack and not in a callback within the JET API.
    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    //  track entry (before any locks are even waited for)

    AtomicIncrement( &(ptte->m_cInCallback) );
    
    //  set that this is now a timer-task thread

    Postls()->posttExecuting = ptte;

    //  only one instance of a task is allowed to execute at a time

    ptte->m_semExec.Acquire();

    //  lock and modulate scheduling state

    ptte->m_critSchedule.Enter();

    const __int64 idschedStart = ptte->m_idSchedule;    //  saved in var just for debugging really
    if ( ptte->m_idRun == idschedStart )
    {
        //  this means we have actually already done a run through the callback for 
        //  the latest request to schedule the timer task, so we skip this callback.
        //  note: mostly we skip because we can't necessarily fix the state afterwards.
        
        Assert( ptte->m_pvTaskRuntimeContext == NULL );
        OnDebug( ptte->m_tickLastSkippedRun = TickOSTimeCurrent() );
        ptte->m_cSkippedRuns++;
        ptte->m_critSchedule.Leave();
        goto QuitThread;
    }

    //  it isn't valid to be here if we are in the inactive, self cancel pending,
    //  or canceled states

    Assert( ptte->m_state == COSTimerTaskEntry::stateScheduled ||
                ptte->m_state == COSTimerTaskEntry::stateCancelling );

    //  dequeue our runtime context

    void * const pvTaskRuntimeContext = ptte->m_pvTaskRuntimeContext;
    ptte->m_pvTaskRuntimeContext = NULL;
    ptte->m_idRun = ptte->m_idSchedule;

    //  set state to running

    ptte->m_state = COSTimerTaskEntry::stateRunning;

    ptte->m_critSchedule.Leave();

    //  begin tracking info

    OSTrace( JET_tracetagTimerQueue, OSFormat( "OSTimerTaskIThreadpoolTimerCompletion( %p ) %d ms since last run.\n", ptte->m_pfnTask, DtickDelta( ptte->m_tickExecEndLast, TickOSTimeCurrent() ) ) );
    ETTimerTaskRun( ptte, ptte->m_pfnTask, ptte->m_pvTaskGroupContext, pvTaskRuntimeContext, ptte->m_cRuns );

    ptte->m_tidExec = GetCurrentThreadId();
    const TICK tickStart = TickOSTimeCurrent();

    //  execute the task

    ptte->m_pfnTask( ptte->m_pvTaskGroupContext, pvTaskRuntimeContext );

    Assert( ptte->m_state == COSTimerTaskEntry::stateRunning ||
            ptte->m_state == COSTimerTaskEntry::stateCancelling );

    //  end tracking info

    const TICK tickEnd = TickOSTimeCurrent();

    //  record this task's tracking stats

    ptte->m_tidExecLast         = ptte->m_tidExec;
    ptte->m_tickExecStartLast   = tickStart;
    ptte->m_tickExecEndLast     = tickEnd;
    ptte->m_cRuns++;
    ptte->m_tickRunAverage      = (TICK) (  WeightedAvePrevRuns( ptte->m_cRuns, ptte->m_tickRunAverage ) +
                                            WeightedAveNewRun( ptte->m_cRuns, DtickDelta( tickStart, tickEnd ) ) );

    //  update state, release, and cleanup

    ptte->m_critSchedule.Enter();

    if ( ptte->m_state == COSTimerTaskEntry::stateRunning &&
            ptte->m_idRun == ptte->m_idSchedule )
    {
        //  if the state stayed in the running through the whole run, we should set it back to inactive

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

    //  track callback oslayer level exit

    AtomicDecrement( &(ptte->m_cInCallback) );

    //  expect to have a clean sync state when done as well

    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    //  no longer a timer-task thread

    Postls()->posttExecuting = NULL;
}

typedef CInvasiveList< COSTimerTaskEntry, COSTimerTaskEntry::OffsetOfILE > COSTimerTaskEntryList;

COSTimerTaskEntryList g_ilTimerTaskList;

CCriticalSection g_critTimerTaskList( CLockBasicInfo( CSyncBasicInfo( _T( "g_critTimerTaskList" ) ), rankTimerTaskList, 0 ) );


//  creates a task timer object
//    (see header)
//
ERR COSTimerTaskEntry::ErrOSTimerTaskICreate(
    PfnTimerTask const              pfnTask,
    const void * const              pvTaskGroupContext,
    POSTIMERTASK *                  pposttTimerHandle )
{
    ERR                             err = JET_errSuccess;
    COSTimerTaskEntry *             ptte = NULL;

    //  protect the state of the timer queue

    g_critTimerTaskList.Enter();

    //  validate parameters

    if ( pfnTask == NULL )
    {
        AssertSz( fFalse, "pfnTask should've been specified" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  check for the specified task in our task "table"

    for ( ptte = g_ilTimerTaskList.PrevMost(); ptte; ptte = g_ilTimerTaskList.Next( ptte ) )
    {
        if ( ptte->m_pfnTask == pfnTask && ptte->m_pvTaskGroupContext == pvTaskGroupContext )
        {
            //  found the matching task + group context, so this is task is already created
            break;
        }
    }

    //  if we found it, that's a problem

    if ( ptte )
    {
        AssertSz( fFalse, "This is a usage error to try to schedule the same task." );
        ptte = NULL;
        Error( ErrERRCheck( JET_errTaskDropped ) );
    }

    Alloc( ptte = new COSTimerTaskEntry( pfnTask, pvTaskGroupContext ) );

    Assert( ptte->m_pOSThreadpoolTimer == NULL );

    //  create threadpool timer

    Alloc( ptte->m_pOSThreadpoolTimer = g_pfnCreateThreadpoolTimer( COSTimerTaskEntry::OSTimerTaskIThreadpoolTimerCompletion, ptte, NULL ) );

    ptte->m_state = COSTimerTaskEntry::stateInactive;

    g_ilTimerTaskList.InsertAsPrevMost( ptte );

    //  tracking

    OSTrace( JET_tracetagTimerQueue, OSFormat( "CreateThreadpoolTimer() -> %p \n", ptte->m_pOSThreadpoolTimer ) );

    ptte->m_tickExecEndLast = TickOSTimeCurrent(); // init so we have rational tracing ...
    OnDebug( ptte->m_tickCreated = ptte->m_tickExecEndLast );
    OnDebug( ptte->m_fThreadpoolTimerUp = fTrue );

    //  transfer to out parameter

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
            OnDebug( ptte->m_tickDeleted = (TICK)-1 );  // never really created, so can't delete it - use this as signal
            delete ptte;
        }
    }

    g_critTimerTaskList.Leave();

    return err;
}

//  schedules a task for asynchronous execution at a given time with a fuzzy slop delay.
//    (see header)
//
VOID COSTimerTaskEntry::OSTimerTaskScheduleITask(
    const void * const              pvRuntimeContext,
    const TICK                      dtickMinDelay,
    const TICK                      dtickSlopDelay,
    const void **                   ppvRuntimeContextCancelled )
{
    COSTimerTaskEntry * const       ptte = this;

    //  validate (and clear) the arguments

    Assert( dtickMinDelay != INFINITE );
    Assert( dtickMinDelay < INT_MAX );

    if ( ppvRuntimeContextCancelled )
    {
        *ppvRuntimeContextCancelled = NULL;
    }

    const BOOL fCallbackReScheduling = ( Postls()->posttExecuting != NULL );    // rescheduling from inside _A_ timer-task callback of some sort
    const BOOL fSelfReScheduling = ( ptte->m_tidExec == DwUtilThreadId() ); // rescheduling same timer-task from inside this same timer-task

    if ( fCallbackReScheduling )
    {
    }
    
    //  we synchronize two attempts to reschedule the task

    m_critSchedule.Enter();

    //  check to make sure we are not quiescing this task

    if ( ptte->m_state == COSTimerTaskEntry::stateCancelling )
    {
        //  silently drops as someone has called cancel

        m_critSchedule.Leave();
        return;
    }

    //  update scheduling stats

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

    //  calculate the relative time to put into the FILETIME

    //  all FILETIMEs are in 100 ns increments, couldn't be ns / nano-seconds as that'd be convenient.
    //  negative values are interpreted as relative times, at least for the SetThreadpoolTimer() API.
    //      sub-note: MSDN SetThreadpoolTimer() has interesting info on relative vs. absolute ... maybe 
    //      we should use absolute?  Also will our timer even wake up in these always on-sleep modes?

    LARGE_INTEGER dftMinDelay;
    dftMinDelay.QuadPart = -( ((LONGLONG)dtickMinDelay) * (LONGLONG)10000 );
    Assert( dftMinDelay.QuadPart <= 0 );

    //  enqueue and set scheduled state

    ptte->m_dtickDelay = dtickMinDelay;
    ptte->m_dtickSlop = dtickSlopDelay;

    //  if the callback is concurrently executing, we don't want to set the state to scheduled (as the
    //  running state is more important), but we will still increment m_idSchedule and the mismatching 
    //  value with m_idRun at the end of the callback will tell us to migrate back to stateScheduled 
    //  instead of stateInactive (at the callbacks completion state change, just below m_cRuns increment).
    if ( ptte->m_state != COSTimerTaskEntry::stateRunning )
    {
        ptte->m_state = COSTimerTaskEntry::stateScheduled;
    }
    //  the ID is just an atomically incrementing ID, generated and incremented here in schedule, and 
    //  consumed for the m_idRun in the timer callback thunk.
    ptte->m_idSchedule++;

    //  note: it is not easy to tell if we truly supplanted a run if the client is actually using 
    //  NULL runtime context values ... so for simplicity / to avoid a complicated comparison of the
    //  ptte->m_idSchedule and the ptte->m_idRun, and since I for now only needed this in the light
    //  of non-NULL runtime contexts, so that's all I did ... 
    if ( ptte->m_pvTaskRuntimeContext )
    {
        //  put it under DEBUG for the above reason (only incremented with non-NULL pvTaskRuntimeContext).
        OnDebug( ptte->m_tickLastSupplantedRun = TickOSTimeCurrent() );
        OnDebug( ptte->m_cSupplantedRuns++ );
        if ( ppvRuntimeContextCancelled )
        {
            *ppvRuntimeContextCancelled = ptte->m_pvTaskRuntimeContext;
        }
    }
    ptte->m_pvTaskRuntimeContext = (void*)pvRuntimeContext;

    //  schedule

    OSTrace( JET_tracetagTimerQueue, OSFormat( "Scheduling SetThreadpoolTimer() for between %d and %d ms out ...\n",
                dtickMinDelay, dtickMinDelay + dtickSlopDelay ) );
    ETTimerTaskSchedule( ptte, ptte->m_pfnTask, ptte->m_pvTaskGroupContext, ptte->m_pvTaskRuntimeContext, ptte->m_dtickDelay, ptte->m_dtickSlop );

    g_pfnSetThreadpoolTimer( ptte->m_pOSThreadpoolTimer, (FILETIME*)&dftMinDelay, 0, dtickSlopDelay );

    OSTrace( JET_tracetagTimerQueue, OSFormat( "SetThreadpoolTimer() Done.\n" ) );

    //  cleanup
 
    m_critSchedule.Leave();
}


//  cancels (and quiesces) the outstanding task that was scheduled with ErrOSTimerScheduleTaskSloppy()
//    (see header)
//
//  We follow the shutdown pattern established in this blog article:
//      http://blogs.msdn.com/b/jiangyue/archive/2012/02/07/graceful-completion-of-thread-pool-wait-callback.aspx
//
VOID COSTimerTaskEntry::OSTimerTaskCancelITask( _Out_opt_ const void ** ppvRuntimeContextCancelled )
{
    COSTimerTaskEntry * const ptte = this;

    AssertSz( ptte->m_tidExec != DwUtilThreadId(), "Can not call OSTimerTaskCancelITask from within the callback (case: ptte->m_tidExec != DwUtilThreadId())" );
    AssertSz( Postls()->posttExecuting == NULL, "Can not call OSTimerTaskCancelITask from within the callback (case: Postls()->posttExecuting == NULL)" );

    //  Store ptte->m_state in a temporary var, so it won't get changed in Assert below during comparing.
    OnDebug( const State stateT = ptte->m_state );
    Assert( stateT == COSTimerTaskEntry::stateInactive ||
            stateT == COSTimerTaskEntry::stateScheduled ||
            stateT == COSTimerTaskEntry::stateRunning );

    //  validate arguments

    Assert( ptte->m_pOSThreadpoolTimer );
    Assert( ptte->m_fThreadpoolTimerUp );

    //  quiesce the callback from the threadpool if running or scheduled
    //

    OSTrace( JET_tracetagTimerQueue, OSFormat( "OSTimerTaskCancelITask beginning ThreadpoolTimer Shutdown...\n" ) );

    //  ensure no new callbacks will be able to reschedule threadpool timers

    ptte->m_critSchedule.Enter();

    OnDebug( ptte->m_tickCancelling = TickOSTimeCurrent() );
    ptte->m_state = COSTimerTaskEntry::stateCancelling;
    ptte->m_idRun = ptte->m_idSchedule; // this forces faster cancelling (user callback will not be called)
    if( ppvRuntimeContextCancelled )
    {
        *ppvRuntimeContextCancelled = ptte->m_pvTaskRuntimeContext;
    }
    ptte->m_pvTaskRuntimeContext = NULL;

    //  note: you can not keep the critical section over the WaitForThreadpoolTimerCallbacks() or 
    //  you can deadlock with the callback which also takes the same lock

    ptte->m_critSchedule.Leave();

    //  wait for any callbacks to execute (so they'll all have picked up g_fQuiesceThreadpoolTimer we just set)

    g_pfnWaitForThreadpoolTimerCallbacks( ptte->m_pOSThreadpoolTimer, TRUE );

    //  initiate immediate scheduling for any future tasks

    g_pfnSetThreadpoolTimer( ptte->m_pOSThreadpoolTimer, NULL, 0, 0 );

    //  wait for the callbacks to execute (again)

    g_pfnWaitForThreadpoolTimerCallbacks( ptte->m_pOSThreadpoolTimer, TRUE );

    Assert( ptte->m_cInCallback == 0 );

    //  quiesce state

    ptte->m_critSchedule.Enter();   // shouldn't be necessary, but for future resilience, good idea

    OnDebug( ptte->m_tickCancelled = TickOSTimeCurrent() );
    OSTrace( JET_tracetagTimerQueue, OSFormat( "ThreadpoolTimer Quiesced...\n" ) );
    //OnDebug( UtilSleep( ( rand() % 4 == 0 ) ? ( rand() % 200 ) : 0 ) );

    ptte->m_state = COSTimerTaskEntry::stateCancelled;

    ptte->m_critSchedule.Leave();

    ETTimerTaskCancel( ptte, ptte->m_pfnTask, ptte->m_pvTaskGroupContext );

    //  this function can't actually fail that I can see, which is great for a cleanup code path!
}

//  deletes a task timer object
//    (see header)
//
VOID COSTimerTaskEntry::OSTimerTaskIDelete( POSTIMERTASK postt )
{
    COSTimerTaskEntry * const       ptte = (COSTimerTaskEntry *)postt;

    //  validate arguments

    Assert( ptte );

    AssertSz( ptte->m_tidExec != DwUtilThreadId() && Postls()->posttExecuting == NULL, "Can not call OSTimerTaskIDelete from within the callback" );
    Assert( ptte->m_cInCallback == 0 ); //  shouldn't be anyone even before the semaphore is grabbed

    Expected( ptte->m_state == COSTimerTaskEntry::stateCancelled );

    OnDebug( ptte->m_fThreadpoolTimerUp = fFalse );
    //OnDebug( UtilSleep( ( rand() % 4 == 0 ) ? ( rand() % 200 ) : 0 ) );

    g_critTimerTaskList.Enter();

    //  close / release the threadpool timer

    g_pfnCloseThreadpoolTimer( ptte->m_pOSThreadpoolTimer );
    ptte->m_pOSThreadpoolTimer = NULL;
    ptte->m_state = COSTimerTaskEntry::stateDeleted;
    OnDebug( ptte->m_tickDeleted = TickOSTimeCurrent() );

    OSTrace( JET_tracetagTimerQueue, OSFormat( "ThreadpoolTimer Closed/Released.\n" ) );

    Assert( ptte->m_pOSThreadpoolTimer == NULL );
    Assert( !ptte->m_fThreadpoolTimerUp );

    //  remove

    g_ilTimerTaskList.Remove( ptte );
    delete ptte;

    g_critTimerTaskList.Leave();

    //  this function can't actually fail that I can see, which is great for a cleanup code path!
}

//  creates a task timer object
//    (see header)
//
ERR ErrOSTimerTaskCreate(
    PfnTimerTask const              pfnTask,
    const void * const              pvTaskGroupContext,
    POSTIMERTASK *                  pposttTimerHandle )
{
    return COSTimerTaskEntry::ErrOSTimerTaskICreate( pfnTask, pvTaskGroupContext, pposttTimerHandle );
}

//  schedules a task for asynchronous execution at a given time with a fuzzy slop delay.
//    (see header)
//
VOID OSTimerTaskScheduleTask(
    POSTIMERTASK                    postt,
    const void * const              pvRuntimeContext,
    const TICK                      dtickMinDelay,
    const TICK                      dtickSlopDelay,
    const void **                   ppvRuntimeContextCancelled )
{
    ((COSTimerTaskEntry *)postt)->OSTimerTaskScheduleITask( pvRuntimeContext, dtickMinDelay, dtickSlopDelay, ppvRuntimeContextCancelled );
}

//  cancels (and quiesces) the outstanding task that was scheduled with OSTimerTaskScheduleTask()
//    (see header)
//
VOID OSTimerTaskCancelTask( POSTIMERTASK postt, _Out_opt_ const void ** ppvRuntimeContextCancelled )
{
    ((COSTimerTaskEntry *)postt)->OSTimerTaskCancelITask( ppvRuntimeContextCancelled );
}

//  deletes a task timer object
//    (see header)
//
VOID OSTimerTaskDelete( POSTIMERTASK postt )
{
    COSTimerTaskEntry::OSTimerTaskIDelete( postt );
}

#ifdef DEBUG
//  checks if a timer task is active (either running or scheduled).
//
//  note: DO NOT make this a retail function, almost assuredly outside of
//  Assert()s there is no way to use this safely.
//
BOOL FOSTimerTaskActive( POSTIMERTASK postt )
{
    return ((COSTimerTaskEntry *)postt)->FOSTimerTaskIActive();
}
#endif

//  post- terminate timer queue subsystem

void OSTimerQueuePostterm()
{
    //  nop
}

//  pre-init timer queue subsystem

BOOL FOSTimerQueuePreinit()
{
    //  nop

    Assert( g_ilTimerTaskList.FEmpty() );

    return fTrue;
}


//  terminate timer queue subsystem

void OSTimerQueueTerm()
{
    //  validate our timer task "table" is empty

    g_critTimerTaskList.Enter();

    Assert( g_ilTimerTaskList.FEmpty() );
    Assert( g_ilTimerTaskList.PrevMost() == NULL ); // double check

    g_critTimerTaskList.Leave();

}

//  init timer queue subsystem

ERR ErrOSTimerQueueInit()
{
    ERR err = JET_errSuccess;

//HandleError:
    if ( err < JET_errSuccess )
    {
        OSTimerQueueTerm();
    }
    return err;
}

