// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


LOCAL const BOOL            fCanUseIOCP         = fTrue;

LOCAL const CHAR * const    szCritTaskList      = "CTaskManager::m_critTask";
LOCAL const CHAR * const    szCritActiveThread  = "CTaskManager::m_critActiveThread";

LOCAL const CHAR * const    szSemTaskDispatch   = "CTaskManager::m_semTaskDispatch";

LOCAL const CHAR * const    szAsigAllDone       = "CGPTaskManager::m_asigAllDone";

LOCAL const CHAR * const    szRwlPostTasks      = "CGPTaskManager::m_rwlPostTasks";


////////////////////////////////////////////////
//
//  Generic Task Manager
//

//  ctor

CTaskManager::CTaskManager()
    :       m_critTask( CLockBasicInfo( CSyncBasicInfo( szCritTaskList ), rankCritTaskList, 0 ) ),
            m_critActivateThread( CLockBasicInfo( CSyncBasicInfo( szCritActiveThread ), rankCritTaskList, 0 ) ),
            m_semTaskDispatch( CSyncBasicInfo( szSemTaskDispatch ) ),
            m_rgpTaskNode( NULL ),
            m_cThread( 0 ),
            m_cThreadMax( 0 ),
            m_cPostedTasks( 0 ),
            m_cmsLastActivateThreadTime( 0 ),
            m_cTasksThreshold( 0 ),
            m_rgThreadContext( NULL ),
            m_hIOCPTaskDispatch( NULL ),
            m_fIOCPHasRegisteredFile( fFalse ),
#ifndef RTM
            m_irrcpiLast( 0 ),
#endif
            m_pfnFileIOCompletion( NULL )
{
#ifndef RTM
    memset( m_rgcpiLast, 0, _countof(m_rgcpiLast)*sizeof(m_rgcpiLast[0]) );
#endif
}


//  dtor

CTaskManager::~CTaskManager()
{
}


//  initialize the task manager
//  NOTE: this is not thread-safe with respect to TMTerm or itself

ERR CTaskManager::ErrTMInit(    const ULONG                     cThread,
                                const DWORD_PTR *const          rgThreadContext,
                                const BOOL                      fForceMaxThreads )
{
    ERR err;

    Assert( 0 == m_cThread );
    Assert( !m_rgThreadContext );
    Assert( m_ilTask.FEmpty() );
    Assert( 0 == m_semTaskDispatch.CAvail() );
    Assert( !m_rgpTaskNode );
    Assert( !m_hIOCPTaskDispatch );

    //  allocate THREAD handles

    m_cThread = 0;
    Alloc( m_rgThreadContext = new THREADCONTEXT[cThread] );
    memset( m_rgThreadContext, 0, sizeof( THREADCONTEXT ) * cThread );

    if ( !fCanUseIOCP )
    {
        ULONG iThread;

        //  allocate extra task-nodes for TMTerm (prevents out-of-memory)
        //  NOTE: they must be allocated separately so they can be freed separately

        Alloc( m_rgpTaskNode = new CTaskNode*[cThread] );
        memset( m_rgpTaskNode, 0, cThread * sizeof( CTaskNode * ) );
        for ( iThread = 0; iThread < cThread; iThread++ )
        {
            Alloc( m_rgpTaskNode[iThread] = new CTaskNode() );
        }
    }
    else
    {
        //  create an I/O completion port for posting tasks

        Alloc( m_hIOCPTaskDispatch = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 ) );
    }

    Assert( cThread < 1000 );
    m_cThreadMax = cThread;

    //  prepare the thread context

    for ( size_t iThread = 0; iThread < cThread; iThread++ )
    {
        m_rgThreadContext[iThread].dwThreadContext = rgThreadContext ? rgThreadContext[iThread] : NULL;
        m_rgThreadContext[iThread].ptm = this;
    }

    Assert( 0 == m_cThread );

    //  defer thread creation unless maximum is
    //  explicitly requested

    if ( fForceMaxThreads )
    {
        Call( ErrAddThreadCheck( fForceMaxThreads ) );
    }

    Assert( fForceMaxThreads ? cThread == m_cThread : 0 == m_cThread );

    return JET_errSuccess;

HandleError:

    //  cleanup

    TMTerm();

    return err;
}


//  cleanup the task manager
//  NOTE: this is not thread-safe with respect to ErrTMInit or itself

VOID CTaskManager::TMTerm()
{
    ULONG iThread;
    ULONG cThread;

    // stop growing of the threads' number in the thread pool

    m_critActivateThread.Enter();
    m_cTasksThreshold   = 0xffffffff;
    m_cThreadMax        = 0;
    m_critActivateThread.Leave();

    //  capture the number of active threads

    cThread = m_cThread;

    if ( NULL != m_rgThreadContext )
    {
        //  post a set of fake tasks that will cause the worker threads to exit gracefully

        for ( iThread = 0; iThread < cThread; iThread++ )
        {
            if ( fCanUseIOCP )
            {
                ERR     errPost;

                //  try to post a task to the I/O completion port

                Assert( m_hIOCPTaskDispatch );
                while ( ( errPost = ErrTMPost( TMITermTask, 0, 0 ) ) != JET_errSuccess )
                {
                    //  if we couldn't post the task, something must be
                    //  horribly wrong (kernel has exhausted paged pool?),
                    //  but we need this to succeed in order to terminate
                    //  properly, so no choice but to keep retrying forever
                    //
                    CallS( errPost );
                    UtilSleep( 1000 );
                }
            }
            else
            {
                //  verify the extra task-node

                Assert( m_rgpTaskNode );
                Assert( m_rgpTaskNode[iThread] );

                //  initialize the extra task-node

                m_rgpTaskNode[iThread]->m_pfnCompletion = TMITermTask;

                //  post the task

                m_critTask.Enter();
                m_ilTask.InsertAsNextMost( m_rgpTaskNode[iThread] );
                m_critTask.Leave();
                m_semTaskDispatch.Release();

                //  prevent cleanup from freeing the extra task-node

                m_rgpTaskNode[iThread] = NULL;
            }
        }

        //  wait for each thread to exit

        for ( iThread = 0; iThread < cThread; iThread++ )
        {
            if ( m_rgThreadContext[iThread].thread )
            {
                UtilThreadEnd( m_rgThreadContext[iThread].thread );
            }
        }

        //  cleanup the thread array

        delete[] m_rgThreadContext;
    }
    m_rgThreadContext = NULL;
    m_cThread = 0;
    m_cTasksThreshold = 0;
    m_cmsLastActivateThreadTime = 0;

    //  term the task-list (it should be empty at this point)

    Assert( m_ilTask.FEmpty() );
    if ( !m_ilTask.FEmpty() )
    {
        //  lock the task list

        m_critTask.Enter();

        while ( !m_ilTask.FEmpty() )
        {
            CTaskNode *ptn = m_ilTask.PrevMost();
            m_ilTask.Remove( ptn );
            delete ptn;
        }

        m_critTask.Leave();
    }

    Assert( 0 == m_semTaskDispatch.CAvail() );
    while ( m_semTaskDispatch.FTryAcquire() )
    {
    }

    //  cleanup the extra task-nodes

    if ( m_rgpTaskNode )
    {
        for ( iThread = 0; iThread < m_cThread; iThread++ )
        {
            delete m_rgpTaskNode[iThread];
        }
        delete [] m_rgpTaskNode;
    }
    m_rgpTaskNode = NULL;

    //  cleanup the I/O completion port

    if ( m_hIOCPTaskDispatch )
    {
        const BOOL fCloseOk = CloseHandle( m_hIOCPTaskDispatch );
        Assert( fCloseOk );
    }
    m_hIOCPTaskDispatch = NULL;
}


//  post a task

ERR CTaskManager::ErrTMPost(    CTaskManager::PfnCompletion pfnCompletion,
                                const DWORD                 dwCompletionKey1,
                                const DWORD_PTR             dwCompletionKey2 )
{
    ERR err = JET_errSuccess;

    //  verify input

    Assert( pfnCompletion );

    //  Increment the number of eventually posted tasks

    AtomicIncrement( (LONG *)&m_cPostedTasks );

    //  check to see if we need to create a service thread

    if ( ErrAddThreadCheck( fFalse ) < JET_errSuccess )
    {
        if ( m_cThread == 0 )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }

    //  post the task based on the method we have available to us

    if ( fCanUseIOCP )
    {

        //  we will be posting a task to the I/O completion port

        Assert( m_hIOCPTaskDispatch );

        ETTaskManagerPost( this, pfnCompletion, dwCompletionKey1, (void*)dwCompletionKey2 );

        //  post the task to the I/O completion port

        BOOL fPostedOk = PostQueuedCompletionStatus(    m_hIOCPTaskDispatch,
                                                        dwCompletionKey1,
                                                        DWORD_PTR( pfnCompletion ),
                                                        (OVERLAPPED*)dwCompletionKey2 );
        if ( !fPostedOk )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }
    else
    {

        //  allocate a task-list node using the given context

        CTaskNode *ptn = new CTaskNode();
        if ( !ptn )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        ptn->m_pfnCompletion = pfnCompletion;
        ptn->m_dwCompletionKey1 = dwCompletionKey1;
        ptn->m_dwCompletionKey2 = dwCompletionKey2;

        ETTaskManagerPost( this, pfnCompletion, dwCompletionKey1, (void*)dwCompletionKey2 );

        //  post the task

        m_critTask.Enter();
        m_ilTask.InsertAsNextMost( ptn );
        m_critTask.Leave();
        m_semTaskDispatch.Release();
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        AtomicDecrement( (LONG *)&m_cPostedTasks );
    }

    return err;
}


//  special API to allow files to register with the task manager's I/O completion port and
//  create a dedicated IO thread

BOOL CTaskManager::FTMRegisterFile( VOID *hFile, CTaskManager::PfnCompletion pfnCompletion )
{
    HANDLE hIOCP;

    //  verify input

    Assert( hFile && (hFile != INVALID_HANDLE_VALUE));
    Assert( pfnCompletion );
    //  can't change our mind for now ... this works because all completion port IO goes through
    //  the IO Manager and it indirects different callbacks (instead of having a different callbacks
    //  for different file types) to their appropriate code.
    Assert( m_pfnFileIOCompletion == NULL || m_pfnFileIOCompletion == pfnCompletion );

    //  if we can't use I/O completion ports then we don't need to do anything

    if ( !fCanUseIOCP )
    {
        return fTrue;
    }

    //  we should have an I/O completion port allocated

    Assert( m_hIOCPTaskDispatch );

    //  register the file handle with the I/O completion port
    //
    //  notes on how this works:
    //
    //      When async-file-I/O completes, it will post a completion packet which will wake up a
    //      CTaskManager thread.  The thread will interpret the parameters of the completion
    //      as if they came from ErrTMPost via PostQueuedCompletionStatus.  Therefore, we must
    //      make sure that the I/O completion looks and acts like one from ErrTMPost.
    //
    //      ErrTMPost interprets the completion packet like this:
    //
    //          number of bytes transferred ==> dwCompletionKey1 (DWORD)
    //          completion key              ==> pfnCompletion    (DWORD_PTR)
    //          ptr to overlapped structure ==> dwCompletionKey2 (DWORD_PTR)
    //
    //      We make the "completion key" a function pointer by passing in the address of the
    //      specified I/O handler (pfnCompletion).  The number of bytes transferred and the
    //      overlapped ptr are specified at I/O-issue time.

    hIOCP = CreateIoCompletionPort( hFile, m_hIOCPTaskDispatch, DWORD_PTR( pfnCompletion ), 0 );
    Assert( NULL == hIOCP || hIOCP == m_hIOCPTaskDispatch );
    if ( NULL != hIOCP )
    {
        m_pfnFileIOCompletion = pfnCompletion;
        Assert( FTMIFileIOCompletion( pfnCompletion ) );

        m_fIOCPHasRegisteredFile = fTrue;

        if ( m_cThread < m_cThreadMax )
        {
            //  We are not able to perform gracefully growing of the threads number
            //  because the task can be issued without using ErrTMPost
            //  and we have no control over real number of posted tasks
            //  Thus we will try to create Max number of threads
            ERR err = JET_errSuccess;
            m_critActivateThread.Enter();
            for ( ; m_cThread < m_cThreadMax && JET_errSuccess <= err; m_cThread ++ )
            {
                //  create the thread

                err = ErrUtilThreadCreate(  PUTIL_THREAD_PROC( TMDispatch ),
                                            OSMemoryPageReserveGranularity(),
                                            priorityNormal,
                                            &m_rgThreadContext[m_cThread].thread,
                                            DWORD_PTR( m_rgThreadContext + m_cThread ) );
            }
            AtomicExchange( (LONG *)&m_cTasksThreshold, -1 );
            m_critActivateThread.Leave();
        }
    }

    return BOOL( NULL != hIOCP );
}

//  special API to check a pfn belongs to the task manager's File I/O completion routine

BOOL CTaskManager::FTMIFileIOCompletion( CTaskManager::PfnCompletion pfnCompletion )
{
    return m_pfnFileIOCompletion == pfnCompletion;
}


//  register a one-time idle callback that is tripped if a task thread waits
//  for more than the given period of time for work.  this callback is
//  guaranteed to be called if the thread does go idle.  this callback can only
//  be requested by the task thread itself

VOID CTaskManager::TMRequestIdleCallback(   TICK            dtickIdle,
                                            PfnCompletion   pfnIdle )
{
    //  only task threads should call this function

    Assert( Postls()->fIsTaskThread );
    
    //  do not want to wack a idle task that is not our own

    Assert( Postls()->pfnTaskThreadIdle == NULL || Postls()->pfnTaskThreadIdle == pfnIdle );
    
    //  set task thread idle callback for this task thread
    
    Postls()->dtickTaskThreadIdle   = dtickIdle;
    Postls()->pfnTaskThreadIdle     = pfnIdle;
}


//  dispatch a task (wrapper for TMIDispatch)

DWORD CTaskManager::TMDispatch( DWORD_PTR dwContext )
{
    THREADCONTEXT *ptc;

    //  extract the context

    Assert( 0 != dwContext );
    ptc = (THREADCONTEXT *)dwContext;

    //  run the internal dispatcher

    ptc->ptm->TMIDispatch( ptc->dwThreadContext );

    return 0;
}

//  main task dispatcher (thread-body for workers)

VOID CTaskManager::TMIDispatch( const DWORD_PTR dwThreadContext )
{
    COMPLETIONPACKETINFO     cpi = { 0 };

    //  we should be a task thread

    Assert( !Postls()->fIsTaskThread );
    Postls()->fIsTaskThread = fTrue;

    //  task loop

    while ( Postls()->fIsTaskThread )
    {

        //  initialize out the cpi info so we don't get dirty reads

        cpi.fDequeued               = fFalse;
        cpi.pfnCompletion           = NULL;
        cpi.dwCompletionKey1        = 0;
        cpi.dwCompletionKey2        = 0;
        cpi.fGQCSSuccess            = fFalse;
        cpi.gle                     = (DWORD)errCodeInconsistency;      // not a gle, but will do
        cpi.tickStartWait           = TickOSTimeCurrent();
        cpi.tickComplete            = (TICK)-1;
        OnNonRTM( cpi.dtickIdle     = Postls()->dtickTaskThreadIdle );
        OnNonRTM( cpi.pfnIdle       = Postls()->pfnTaskThreadIdle );

        if ( fCanUseIOCP )
        {

            //  wait for a task to appear on the I/O completion port

            const DWORD dwErrorFault = ErrFaultInjection( 47064 );
            cpi.fGQCSSuccess = dwErrorFault ?
                                    ( fFalse ) :
                                    ( GetQueuedCompletionStatus( m_hIOCPTaskDispatch,
                                                &(cpi.dwCompletionKey1),
                                                (DWORD_PTR*)&(cpi.pfnCompletion),
                                                (OVERLAPPED**)&(cpi.dwCompletionKey2),
                                                Postls()->dtickTaskThreadIdle ? Postls()->dtickTaskThreadIdle : INFINITE ) );
            if ( dwErrorFault ) //  fake fault injection problem
            {
                SetLastError( dwErrorFault );
            }

            //  upgrade success to the gle

            if ( cpi.fGQCSSuccess )
            {
                SetLastError( ERROR_SUCCESS );
            }

            //  track the gle (first thing!)

            cpi.gle             = GetLastError();

            cpi.tickComplete    = TickOSTimeCurrent();  // purposefully second

            //  wait timeout indicates to fire the idle task, not a real failure

            if ( cpi.gle == WAIT_TIMEOUT )
            {
                //  fire a timeout event

                cpi.pfnCompletion       = Postls()->pfnTaskThreadIdle;
                cpi.dwCompletionKey1    = 0;
                cpi.dwCompletionKey2    = 0;
                // leaving GLE for tracking b/c idle thread doesn't utilize GLE ...
                //cpi.gle               = ERROR_SUCCESS;

                Assert( !FTMIFileIOCompletion( cpi.pfnCompletion ) );   // that wouldn't be right

                //  correct the number of posted tasks

                AtomicIncrement( (LONG *)&m_cPostedTasks );
            }

            //  success is a fickle thing ...

            //  GetQueuedCompletionStatus/GQCS has a fun contract ...
            //      - #1 You can successfully dequeue a success
            //      - #2 You can successfully dequeue a failure
            //              GQCS actually returns failure for this, but this is "success" for us.
            //      - #3 Or you can unsuccessfully dequeue anything
            //
            //  In all successful dequeue cases we must have a valid pfnCompletion.
            //  In File IO completion dequeues we must have a dwCompletionKey2 / OVERLAPPED *.

            //  first we process if we have a valid pfnCompletion

            if ( cpi.fGQCSSuccess )
            {
                //  Case #1: Successfully dequeued a success

                AssertRTL( cpi.pfnCompletion );                         // this would be weird
                cpi.fDequeued = ( cpi.pfnCompletion != NULL );          // defense in depth
            }
            else
            {
                //  failure doesn't necessarily mean failure ...

                if ( cpi.pfnCompletion )
                {
                    //  Case #2: Successfully dequeued a failure

                    cpi.fDequeued = fTrue;
                }
                else
                {
                    //  Case #3: Well, ok in this case it does ... GQCS itself failed

                    //  should we log an event?  what would the be the point ... admin can't do 
                    //  anything and we move on.

                    AssertSzRTL( dwErrorFault /* i.e. assert we had fault injection */, "What is happening here?  Check m_rgcpiLast and the current cpi." );
                    cpi.fDequeued = fFalse;
                    AssertRTL( cpi.gle != ERROR_SUCCESS );
                }
            }

            //  second we check that a File IO completion is setup correctly

            if ( cpi.fDequeued && FTMIFileIOCompletion( cpi.pfnCompletion ) )
            {
                cpi.fDequeued = ( /* OVERLAPPED* */cpi.dwCompletionKey2 != NULL );  // defense in depth
                //  technically, according to GQCS documentation:
                //      http://msdn.microsoft.com/en-us/library/aa364986(VS.85).aspx
                //  this can happen and we should check if the OVERLAPPED * is NULL for success.
                //  However, the documentation isn't accurate b/c a non-IO "posted" completion can come 
                //  back with a NULL OVERLAPPED * ... so we're seeing if we can use the pfnCompletion / 
                //  lpCompletionKey can be used as a proxy for this.  If this goes off, remove it the,
                //  previous line of code does the right thing.
                AssertRTL( cpi.fDequeued );                                         // this would be weird (IMO ;-)
            }

            //  minor validation

            if ( !cpi.fDequeued )
            {
                Expected( cpi.gle != ERROR_SUCCESS );
            }
            if ( !cpi.fGQCSSuccess )    // GQCS success
            {
                //  I can't see how we would get failures of any other kind of completion besides our
                //  idle time out and an File IO completion ...
                Expected( cpi.pfnCompletion == NULL ||
                            // will this hold?
                            ( cpi.pfnCompletion == Postls()->pfnTaskThreadIdle ) ||
                            ( FTMIFileIOCompletion( cpi.pfnCompletion ) && cpi.fDequeued ) );
            }
            // idle thread call should be a successful dequeue
            Assert( cpi.pfnCompletion == NULL || cpi.pfnCompletion != Postls()->pfnTaskThreadIdle || cpi.fDequeued );

            //  "end scope" of cpi.fGQCSSuccess
            #define fGQCSSuccess    fGQCSSuccess_DO_NOT_USE_SEE_COMMENTS
        }
        else
        {
            CTaskNode *ptn;

            //  wait for a task to be posted (disable deadlock timeout)

            if ( m_semTaskDispatch.FAcquire( Postls()->dtickTaskThreadIdle ? Postls()->dtickTaskThreadIdle : cmsecInfiniteNoDeadlock ) )
            {
                //  get the next task-node

                m_critTask.Enter();
                ptn = m_ilTask.PrevMost();
                Assert( ptn );
                m_ilTask.Remove( ptn );
                m_critTask.Leave();

                //  extract the task node's parameters

                cpi.pfnCompletion       = ptn->m_pfnCompletion;
                cpi.dwCompletionKey1    = ptn->m_dwCompletionKey1;
                cpi.dwCompletionKey2    = ptn->m_dwCompletionKey2;
                cpi.fDequeued           = fTrue;
                cpi.gle                 = ERROR_SUCCESS;
                cpi.tickComplete        = TickOSTimeCurrent();

                //  cleanup up the task node

                delete ptn;
            }
            else
            {
                //  fire a timeout event

                cpi.pfnCompletion       = Postls()->pfnTaskThreadIdle;
                cpi.dwCompletionKey1    = 0;
                cpi.dwCompletionKey2    = 0;
                cpi.fDequeued           = fTrue;
                cpi.gle                 = ERROR_SUCCESS;
                cpi.tickComplete        = TickOSTimeCurrent();

                //  correct the number of posted tasks

                AtomicIncrement( (LONG *)&m_cPostedTasks );
            }
        }

        //  update the last completion packet tracking

        OnNonRTM( const DWORD irrcpi = AtomicIncrement( (LONG*)&m_irrcpiLast ) );
        OnNonRTM( m_rgcpiLast[irrcpi%_countof(m_rgcpiLast)] = cpi );


        AssertRTL( !cpi.fDequeued || cpi.pfnCompletion != NULL );

        if ( cpi.fDequeued )
        {
            //  we can't add this to the if check for defense in depth, because it may
            //  mean we have dequeued something and the queue state will go wack.  This
            //  should be impossible due to GQCS results processing above.

            Enforce( cpi.pfnCompletion );

            if ( cpi.pfnCompletion == Postls()->pfnTaskThreadIdle )
            {
                //  servicing the idle task (or equivalent of it), reset our timeout callback
            
                Postls()->dtickTaskThreadIdle   = 0;
                Postls()->pfnTaskThreadIdle     = NULL;
            }

            //  note: success does not imply gle == 0, we can have successfully dequeued a "failure" IO packet

            SetLastError( cpi.gle );    // not needed anymore, but defense in depth

            //  do not trace this completion if it's an I/O one.
            //  enable I/O traces if that is what you are interested in.

            if ( !FTMIFileIOCompletion( cpi.pfnCompletion ) )
            {
                ETTaskManagerRun( this,
                              cpi.pfnCompletion,
                              cpi.dwCompletionKey1,
                              (void*)cpi.dwCompletionKey2,
                              cpi.gle,
                              (void*)dwThreadContext );
            }

            //  run the task

            cpi.pfnCompletion( cpi.gle, dwThreadContext, cpi.dwCompletionKey1, cpi.dwCompletionKey2 );

            //  correct the number of posted tasks
            //
            //  NOTE: count may go negative if it's possible
            //  for completions to occur even though a task wasn't
            //  posted, so we cannot assert against a negative
            //  count if a file was registered with the IOCP

            const LONG count = AtomicDecrement( (LONG *)&m_cPostedTasks );
            Assert( count >= 0 || m_fIOCPHasRegisteredFile );
        }

        //  else / !cpi.fDequeued ...  if we didn't achieve a successful dequeue we loop like nothing 
        //  happened ... we're assured by the Completion Port devs that nothing has been lost and that 
        //  we can just start waiting again on GQCS and it might work out better the next time.  
        //  Curiosity ... should we sleep and give it a "breather"?

    } // while fIsTaskThread (this is cleverly canceled by TMITermTask being posted to the completion port).

}


//  used a bogus completion to pop threads off of the I/O completion port

VOID CTaskManager::TMITermTask( const DWORD     dwError,
                                const DWORD_PTR dwThreadContext,
                                const DWORD     dwCompletionKey1,
                                const DWORD_PTR dwCompletionKey2 )
{
    Expected( dwError == ERROR_SUCCESS );   // not important though ...

    //  this thread is no longer processing tasks

    Assert( Postls()->fIsTaskThread );
    Postls()->fIsTaskThread = fFalse;
}


//  should we try and create a new thread?

BOOL CTaskManager::FNeedNewThread() const
{
    BOOL fNeedNewThread         = fFalse;

    const ULONG tickCurr        = TickOSTimeCurrent();
    const ULONG tickThreshold   = m_cThread * 1000 / CUtilProcessProcessor();
    const ULONG tickNext        = m_cmsLastActivateThreadTime + tickThreshold;

    if (    0 == m_cThread
        || (
                m_cThread < m_cThreadMax
            &&  !m_fIOCPHasRegisteredFile   //  can't trust m_cPostedTasks if it's possible for completions to occur without posting a task
            &&  m_cPostedTasks > m_cTasksThreshold
            &&  TickCmp( tickCurr, tickNext ) >= 0 ) )
    {
        fNeedNewThread = fTrue;
    }

    return fNeedNewThread;
}


//  Check if need to activate one more thread in the thread pool

ERR CTaskManager::ErrAddThreadCheck( const BOOL fForceMaxThreads )
{
    ERR err = JET_errSuccess;

    if( FNeedNewThread()
        && m_critActivateThread.FTryEnter() )
    {
        if( FNeedNewThread() )
        {
            do
            {
                //  create the thread
                //
                err = ErrUtilThreadCreate(
                                PUTIL_THREAD_PROC( TMDispatch ),
                                OSMemoryPageReserveGranularity(),
                                priorityNormal,
                                &m_rgThreadContext[m_cThread].thread,
                                DWORD_PTR( m_rgThreadContext + m_cThread ) );
                if ( err < JET_errSuccess )
                    break;

                //  increase the thread count AFTER we have successfully created the thread

                AtomicIncrement( (LONG *)&m_cThread );

                //  if we have reached the max number of threads

                if ( m_cThread == m_cThreadMax )
                {
                    //  disable further attempts to activate threads

                    m_cTasksThreshold = 0xffffffff;
                }
                else
                {
                    //  set new activate, time & threshold

                    Assert( m_cThread < m_cThreadMax );
                    m_cmsLastActivateThreadTime = TickOSTimeCurrent();
                    m_cTasksThreshold           = m_cThread * 8;
                }
            }
            while ( m_cThread < m_cThreadMax && fForceMaxThreads );
        }

        //  Leave the critical section
        m_critActivateThread.Leave();
    }
    return err;
}


////////////////////////////////////////////////
//
//  Task Manager for Win2000
//

NTOSFuncStd( g_pfnQueueUserWorkItem, g_mwszzThreadpoolLibs, QueueUserWorkItem, oslfExpectedOnWin5x );

//  ctor

CGPTaskManager::CGPTaskManager()
    :   m_fInit( fFalse ),
        m_cPostedTasks( 0 ),
        m_asigAllDone( CSyncBasicInfo( szAsigAllDone ) ),
        m_rwlPostTasks( CLockBasicInfo( CSyncBasicInfo( szRwlPostTasks ), rankRwlPostTasks, 0 ) ),
        m_ptaskmanager( NULL )
{
}

CGPTaskManager::~CGPTaskManager()
{
    TMTerm();
}

ERR CGPTaskManager::ErrTMInit( const ULONG cThread )
{
    ERR err = JET_errSuccess;

    //  reset

    m_fInit             = fFalse;
    m_cPostedTasks      = 0;
    m_asigAllDone.Reset();
    m_ptaskmanager      = NULL;

    //  If QueueUserWorkItem is not supported use CTaskManager with
    //  advised number of threads.
    //
    if ( g_pfnQueueUserWorkItem.ErrIsPresent() < JET_errSuccess )
    {
        Alloc( m_ptaskmanager = new CTaskManager );
        Call( m_ptaskmanager->ErrTMInit( cThread ? cThread : CUtilProcessProcessor() ) );
    }

    //  initialise with a dummy refcount (allows
    //  term code to determine whether it has to
    //  wait for outstanding tasks or not)
    //
    m_cPostedTasks = 1;

    //  we are now init
    //
    m_fInit = fTrue;

HandleError:
    if ( err < JET_errSuccess )
    {
        TMTerm();
    }
    return err;
}

VOID CGPTaskManager::TMTerm()
{
    //  only term if we are init
    //
    if ( m_fInit && !FUtilProcessAbort() )
    {
        //  block anyone from posting more tasks
        //
        m_rwlPostTasks.EnterAsWriter();
        m_fInit = fFalse;
        m_rwlPostTasks.LeaveAsWriter();

        //  remove the dummy refcount.  if there are other counts left then wait
        //  until all outstanding tasks have completed
        //
        if ( AtomicDecrement( (LONG *)&m_cPostedTasks ) )
        {
            m_asigAllDone.Wait();
        }

        //  release our CTaskManager, if in use
        //
        if ( m_ptaskmanager )
        {
            m_ptaskmanager->TMTerm();
            delete m_ptaskmanager;
        }
    }
}

ERR CGPTaskManager::ErrTMPost( PfnCompletion pfnCompletion, VOID *pvParam, TaskInfo *pTaskInfo )
{
    ERR         err         = JET_errSuccess;
    PTMCallWrap ptmCallWrap = NULL;

    //  Enter post task lock
    //
    m_rwlPostTasks.EnterAsReader();

    if ( m_fInit )
    {
        //  Increment the number of eventually posted tasks
        //
        const LONG  cTasksOutstanding   = AtomicIncrement( (LONG *)&m_cPostedTasks );

        //  must be at least a dummy refcount and a refcount for this task
        //
        Assert( cTasksOutstanding > 1 );

        //  wrap the Call
        ptmCallWrap = new TMCallWrap;
        if ( NULL != ptmCallWrap )
        {
            ptmCallWrap->pfnCompletion  = pfnCompletion;
            ptmCallWrap->pvParam        = pvParam;
            ptmCallWrap->pThis          = this;
            ptmCallWrap->pTaskInfo      = pTaskInfo;

            if (pTaskInfo)
            {
                pTaskInfo->NotifyPost();
            }

            ETGPTaskManagerPost( this, pfnCompletion, pvParam, pTaskInfo );

            //  choose the proper dispatch function based on used thread pool manager
            if ( g_pfnQueueUserWorkItem.ErrIsPresent() < JET_errSuccess )
            {
                err = m_ptaskmanager->ErrTMPost( TMIDispatch, 0, DWORD_PTR( ptmCallWrap ) );
            }
            else if ( !g_pfnQueueUserWorkItem( CGPTaskManager::TMIDispatchGP, ptmCallWrap, 0 ) )
            {
                err = ErrERRCheck( JET_errOutOfMemory );
            }
        }
        else
        {
            err = ErrERRCheck( JET_errOutOfMemory );
        }

        if ( err < JET_errSuccess )
        {
            //  couldn't post task, free the ptmCallWrap and update task count accordingly
            //
            delete ptmCallWrap;
            ptmCallWrap = NULL;
            const LONG  cTasksRemaining     = AtomicDecrement( (LONG *)&m_cPostedTasks );

            //  must still be a dummy refcount remaining
            //
            Assert( cTasksRemaining >= 1 );
        }
    }
    else
    {

        //  the task manager is not initialized so the task must be dropped

        err = ErrERRCheck( JET_errTaskDropped );
    }

    // leave post task lock
    //
    m_rwlPostTasks.LeaveAsReader();

    return err;
}

DWORD __stdcall CGPTaskManager::TMIDispatchGP( VOID *pvParam )
{
    PTMCallWrap     ptmCallWrap     = PTMCallWrap( pvParam );
    const BOOL      fIsTaskThread   = Postls()->fIsTaskThread;

    //  All tasks must be executed in TaskThread environment
    if ( !fIsTaskThread )
    {
        Postls()->fIsTaskThread = fTrue;
    }

    //  check input parameters
    Assert( NULL != ptmCallWrap );

    if (ptmCallWrap->pTaskInfo)
    {
        ptmCallWrap->pTaskInfo->NotifyDispatch();
    }

    //  expect to have a clean sync state as a pre-check

    //  can assert stronger statements here in than in JET API as we're guaranteed 
    //  to be at the top of a stack and not in a callback within the JET API.
    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    ETGPTaskManagerRun( ptmCallWrap->pThis, ptmCallWrap->pfnCompletion, ptmCallWrap->pvParam, ptmCallWrap->pTaskInfo );

    extern BOOL g_fCatchExceptions;
    if ( g_fCatchExceptions && g_pfnQueueUserWorkItem.ErrIsPresent() >= JET_errSuccess )
    {
        TRY
        {
            ptmCallWrap->pfnCompletion( ptmCallWrap->pvParam );
        }
        EXCEPT( ExceptionFail( "In worker thread." ) )
        {
            AssertPREFIX( !"This code path should be impossible (the exception-handler should have terminated the process)." );
        }
        ENDEXCEPT
    }
    else
    {
        ptmCallWrap->pfnCompletion( ptmCallWrap->pvParam );
    }

    //  expect to have a clean sync state when done as well

    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    if ( !fIsTaskThread )
    {
        Postls()->fIsTaskThread = fFalse;
    }

    CGPTaskManager * const  pThis           = ptmCallWrap->pThis;

    if (ptmCallWrap->pTaskInfo)
    {
        ptmCallWrap->pTaskInfo->NotifyEnd();
    }

    //  must free PTMCallWrap before decrementing refcount, otherwise
    //  the process may terminate and kill this task before we can finish
    //
    delete ptmCallWrap;
    ptmCallWrap = NULL;

    const LONG              cTasks          = AtomicDecrement( (LONG *)&( pThis->m_cPostedTasks ) );

    if ( 0 == cTasks )
    {
        //  the terminating thread is waiting
        //  for us to finish
        //
        Assert( !pThis->m_fInit );
        pThis->m_asigAllDone.Set();
    }
    else
    {
        //  must still be a dummy refcount remaining
        //
        Assert( cTasks >= 1 );
    }

    return 0;
}

VOID CGPTaskManager::TMIDispatch(   const DWORD     dwError,
                                    const DWORD_PTR dwThreadContext,
                                    const DWORD     dwCompletionKey1,
                                    const DWORD_PTR dwCompletionKey2 )
{
    // check input parameters
    Expected( dwError == ERROR_SUCCESS );
    Assert( NULL == dwThreadContext );
    Assert( 0 == dwCompletionKey1 );
    TMIDispatchGP( (VOID *)dwCompletionKey2 );
}

//  are we on a task thread.

BOOL FOSTaskIsTaskThread( void )
{
    return Postls()->fIsTaskThread;
}

//  post-terminate task subsystem

void OSTaskPostterm()
{
    //  nop
}

//  pre-init task subsystem

BOOL FOSTaskPreinit()
{
    return fTrue;
}


//  terminates the task subsystem

void OSTaskTerm()
{
    //  nop
}

//  init task subsystem

ERR ErrOSTaskInit()
{
    ERR err = JET_errSuccess;

    return err;
}

void TaskInfoGeneric::NotifyPost ()
{
    m_dwPostTick = TickOSTimeCurrent( );
    m_dwDispatchTick = m_dwEndTick = 0;
}

void TaskInfoGeneric::NotifyDispatch ()
{
    m_dwDispatchTick = TickOSTimeCurrent( );
}

void TaskInfoGeneric::NotifyEnd()
{
    m_dwEndTick = TickOSTimeCurrent( );
    m_pTaskInfoGenericStats->ReportTaskInfo( *this );
    delete this;
}


void TaskInfoGenericStats::ReportTaskInfo(const TaskInfoGeneric & taskInfo)
{

    Assert( taskInfo.m_dwDispatchTick >= taskInfo.m_dwPostTick);
    Assert( taskInfo.m_dwEndTick >= taskInfo.m_dwDispatchTick);
    Assert( taskInfo.m_dwEndTick >= taskInfo.m_dwPostTick);
    const DWORD dwDispatchTick   = taskInfo.m_dwDispatchTick - taskInfo.m_dwPostTick;
    const DWORD dwExecuteTick    = taskInfo.m_dwEndTick - taskInfo.m_dwDispatchTick;
    const DWORD dwTotalTick      = taskInfo.m_dwEndTick - taskInfo.m_dwPostTick;

    Assert( dwTotalTick >= dwDispatchTick);
    Assert( dwTotalTick >= dwExecuteTick);


    // Before allowing the total statistics here consider
    // using AtomicAdd usage such that we don't enter/leave
    // the CritSection for all task
#ifdef NEVER
    m_critStats.Enter();

    // add the current task to the total
    m_cTasks++;

    m_qwDispatchTickTotal += dwDispatchTick;
    m_qwExecuteTickTotal += dwExecuteTick;
    m_qwTotalTickTotal += dwTotalTick;

    m_critStats.Leave();
#endif

    // check if the current task as abnormal
    if (dwDispatchTick >= m_msMaxDispatch ||
        dwExecuteTick >= m_msMaxExecute)
    {
        m_critStats.Enter();

        // try to don't spam the EventLog with a reporting euristic
        if ( ( taskInfo.m_dwEndTick < m_dwLastReportEndTick + dtickTaskAbnormalLatencyEvent ) &&
            ( dwDispatchTick < m_dwLastReportDispatch * 3 / 2) &&
            ( dwExecuteTick < m_dwLastReportExecute * 3 / 2) )
        {
                m_cLastReportNotReported++;
        }
        else
        {
            // report

            WCHAR szMaxDispatch[32];
            WCHAR szMaxExecute[32];
            WCHAR szTaskType[32];

            OSStrCbFormatW( szMaxDispatch, sizeof( szMaxDispatch ), L"%d", dwDispatchTick / 1000 );
            OSStrCbFormatW( szMaxExecute, sizeof( szMaxExecute ), L"%d", dwExecuteTick / 1000 );
            OSStrCbFormatW( szTaskType, sizeof( szTaskType ), L"%hs", m_szTaskType );

            // check if this is the first report OR
            // if there were no other abnormal tasks since the last report
            if (0 == m_dwLastReportEndTick || 0 == m_cLastReportNotReported)
            {
                const WCHAR *rgszT[3];
                rgszT[0] = szTaskType;
                rgszT[1] = szMaxDispatch;
                rgszT[2] = szMaxExecute;

                UtilReportEvent(
                        eventWarning,
                        PERFORMANCE_CATEGORY,
                        TASK_INFO_STATS_TOO_LONG_ID,
                        3,
                        rgszT,
                        0,
                        NULL );

            }
            else
            {
                const WCHAR *rgszT[5];
                WCHAR szCountLastReport[32];
                WCHAR szTimeLastReport[32];

                OSStrCbFormatW( szCountLastReport, sizeof( szCountLastReport ), L"%d", m_cLastReportNotReported );
                OSStrCbFormatW( szTimeLastReport, sizeof( szTimeLastReport ), L"%d", (taskInfo.m_dwEndTick - m_dwLastReportEndTick)  / 1000 );

                rgszT[0] = szTaskType;
                rgszT[1] = szMaxDispatch;
                rgszT[2] = szMaxExecute;
                rgszT[3] = szCountLastReport;
                rgszT[4] = szTimeLastReport;

                UtilReportEvent(
                        eventWarning,
                        PERFORMANCE_CATEGORY,
                        TASK_INFO_STATS_TOO_LONG_AGAIN_ID,
                        5,
                        rgszT,
                        0,
                        NULL );

            }

            m_dwLastReportDispatch = dwDispatchTick;
            m_dwLastReportExecute = dwExecuteTick;
            m_cLastReportNotReported = 0;
            m_dwLastReportEndTick = taskInfo.m_dwEndTick;
        }

        m_critStats.Leave();
    }
}



