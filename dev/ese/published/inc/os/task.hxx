// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TASK_HXX_INCLUDED
#define _OS_TASK_HXX_INCLUDED



////////////////////////////////////////////////
//
//  Generic Task Manager
//
//      This has 2 levels of context information: per-thread and per-task.
//
//      Per-thread context is established at init time.  It is bound to each worker thread and is passed
//      to each user-defined task function.  The purpose of this is to provide a generic, local context
//      in which each thread will operate regardless of the task being done (e.g. passing JET_SESIDs to
//      each thread because the tasks will all be background database operations).
//
//      Per-task contexts are the obvious "completion key" mechanism.  At post-time, the user passes
//      a specific completion context (usually so the routine can idenfity what work is being done).

#include "collection.hxx"

class TaskInfo
{
public:
    TaskInfo () {}
    ~TaskInfo() {}

public:
    virtual void NotifyPost () = 0;
    virtual void NotifyDispatch () = 0;
    virtual void NotifyEnd () = 0;
};


/*
In order to add Task Information reporting to a specific task, the steps to follow are:
1. define a global TaskInfoGenericStats class with the desired parameters. This class will be used for all
tasks that can and will be monitored together

EX:  TaskInfoGenericStats g_TaskInfoGenericStats( "SCRUB", 10 * 1000, 30 * 1000);

2. for each task you want to monitor, create a new TaskInfoGeneric object which will maintain his runtime
caracteristics and use the TaskInfoGenericStats for the type of task (check the allocation as well :))

EX:  TaskInfoGeneric * pTaskInfo = new TaskInfoGeneric( &g_TaskInfoGenericStats );

3. use the task info object when posting the task (rather then the default NULL TaskInfo param)

EX:  errPostScrub = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, pSCRUBTASK, pTaskInfo);

*/

// the TaskInfoGenericStats events won't be reported more often then this time:
const TICK  dtickTaskAbnormalLatencyEvent       = (60 * 1000);

// this is what the default "abnormal" task dispatch time is considered
const TICK  dtickTaskMaxDispatchDefault     = (1 * 60 * 1000);

// this is what the default "abnormal" task execution time is considered
const TICK  dtickTaskMaxExecuteDefault      = (10 * 60 * 1000);


class TaskInfoGenericStats;

class TaskInfoGeneric: public TaskInfo
{
    friend class TaskInfoGenericStats;

public:
    TaskInfoGeneric (TaskInfoGenericStats * pTaskInfoGenericStats) :TaskInfo(),
        m_pTaskInfoGenericStats( pTaskInfoGenericStats ),
        m_dwPostTick(0),
        m_dwDispatchTick(0),
        m_dwEndTick(0)
        {}

    ~TaskInfoGeneric() {}

public:
    virtual void NotifyPost ();
    virtual void NotifyDispatch ();
    virtual void NotifyEnd();

private:
    // initial tick when the task was posted
    DWORD   m_dwPostTick;

    // tick when the task was dispatched
    DWORD   m_dwDispatchTick;

    // tick when the task finished
    DWORD   m_dwEndTick;

    // task statistics to report to
    TaskInfoGenericStats * m_pTaskInfoGenericStats;
};


class TaskInfoGenericStats
{
public:
    TaskInfoGenericStats (_In_ PCSTR szTaskType, DWORD msMaxDispatch = dtickTaskMaxDispatchDefault, DWORD msMaxExecute = dtickTaskMaxExecuteDefault):
        m_szTaskType( szTaskType ),
        m_msMaxDispatch( msMaxDispatch ),
        m_msMaxExecute( msMaxExecute ),
        m_cTasks( 0 ),
        m_dwLastReportDispatch( 0 ),
        m_dwLastReportExecute( 0 ),
        m_dwLastReportEndTick( 0 ),
        m_cLastReportNotReported( 0 ),
        m_critStats( CLockBasicInfo( CSyncBasicInfo( "TaskInfoStats" ), 0, 0 ) ) {}

    ~TaskInfoGenericStats() {}

public:
    void ReportTaskInfo(const TaskInfoGeneric & taskInfo);

private:
    CCriticalSection  m_critStats;

    const CHAR *  m_szTaskType;
    DWORD   m_msMaxDispatch;
    DWORD   m_msMaxExecute;

    DWORD   m_dwLastReportDispatch;
    DWORD   m_dwLastReportExecute;
    DWORD   m_dwLastReportEndTick;
    DWORD   m_cLastReportNotReported;


    // Total (all m_cTasks tasks) and average time to
    // - dispatch a task (m_dwDispatchTick - m_dwPostTick)
    // - execute a task (m_dwEndTick - m_dwDispatchTick)
    // - complete a task (m_dwEndTick - m_dwPostTick)
    DWORD   m_cTasks;
    QWORD   m_qwDispatchTickTotal;
    QWORD   m_qwExecuteTickTotal;
    QWORD   m_qwTotalTickTotal;

};


class CTaskManager
{
    public:

        typedef VOID (*PfnCompletion)(  const DWORD     dwError,
                                        const DWORD_PTR dwThreadContext,
                                        const DWORD     dwCompletionKey1,
                                        const DWORD_PTR dwCompletionKey2 );

    private:

        class CTaskNode
        {
            public:

                CTaskNode() {}
                ~CTaskNode() {}

                static SIZE_T OffsetOfILE() { return OffsetOf( CTaskNode, m_ile ); }

            public:

                //  task information

                PfnCompletion   m_pfnCompletion;
                DWORD           m_dwCompletionKey1;
                DWORD_PTR       m_dwCompletionKey2;

                //  task-pool context

                CInvasiveList< CTaskNode, OffsetOfILE >::CElement m_ile;
        };

        typedef CInvasiveList< CTaskNode, CTaskNode::OffsetOfILE > TaskList;


        struct THREADCONTEXT
        {
            THREAD          thread;             //  thread handle
            CTaskManager    *ptm;               //  back-pointer for thread to get at CTaskManager data
            DWORD_PTR       dwThreadContext;    //  per-thread user-defined context
        };

        struct COMPLETIONPACKETINFO // cpi
        {
            //  required completion information

            BOOL                fDequeued;          //  whether we successfully got a completion packet from GQCS (whether it was a success or failure packet)
            DWORD               gle;                //  error associated with the completion packet
            PfnCompletion       pfnCompletion;      //  completion packet completion function
            DWORD               dwCompletionKey1;   //
            DWORD_PTR           dwCompletionKey2;   //
            BOOL                fGQCSSuccess;       //  return value of GQCS / success (mostly for debugging, should use fDequeued instead; see comments in TMIDispatch)
            TICK                tickStartWait;      //  tick time before calling GQCS
            TICK                tickComplete;       //  time time on return from GQCS
#ifndef RTM
            //  debugging status information

            TICK                dtickIdle;          //  tick time before triggering the idle callback
            PfnCompletion       pfnIdle;            //  the completion function for the idle callback
#endif
        };

    public:

        //  init / term

        CTaskManager();
        ~CTaskManager();

        ERR ErrTMInit(  const ULONG                     cThread,
                        const DWORD_PTR *const          rgThreadContext     = NULL,
                        const BOOL                      fForceMaxThreads    = fFalse );
        VOID TMTerm();

        //  methods for posting and/or associating completion functions

        ERR ErrTMPost(  PfnCompletion       pfnCompletion,
                        const DWORD         dwCompletionKey1,
                        const DWORD_PTR     dwCompletionKey2 );

        BOOL FTMRegisterFile( VOID *hFile, CTaskManager::PfnCompletion pfnCompletion );

        VOID TMRequestIdleCallback( TICK            dtickIdle,
                                    PfnCompletion   pfnIdle );

    private:

        static DWORD TMDispatch( DWORD_PTR dwContext );
        VOID TMIDispatch( const DWORD_PTR dwThreadContext );

        BOOL    FTMIFileIOCompletion( CTaskManager::PfnCompletion pfnCompletion );

        static VOID TMITermTask(    const DWORD     dwError,
                                    const DWORD_PTR dwThreadContext,
                                    const DWORD     dwCompletionKey1,
                                    const DWORD_PTR dwCompletionKey2 );
        ERR     ErrAddThreadCheck( const BOOL fForceMaxThreads );   //  increase the number of activce threads in thread pool if necessary
        BOOL    FNeedNewThread() const;

    private:

        volatile ULONG                  m_cThreadMax;                   //  Max number of active threads
        volatile ULONG                  m_cPostedTasks;                 //  number of currently posted ( and eventually running) tasks
        volatile ULONG                  m_cmsLastActivateThreadTime;    //  when is the next time to check
                                                                        //  whether we should add more threads in the thread pool
        CCriticalSection                m_critActivateThread;           //  critical section to activate thread
        volatile ULONG                  m_cTasksThreshold;              //  tasks threshold above which we will consider
                                                                        //  creation of new thread in the thread pool
        volatile ULONG                  m_cThread;                      //  number of active threads
        THREADCONTEXT                   *m_rgThreadContext;             //  per-thread context for each active thread

        // These four variables are only used when IO Completion Ports aren't supported.
        TaskList                        m_ilTask;                       //  list of task-nodes
        CCriticalSection                m_critTask;                     //  protect the task-node list
        CSemaphore                      m_semTaskDispatch;              //  dispatch tasks via Acquire/Release
        CTaskNode                       **m_rgpTaskNode;                //  extra task-nodes reserved for TMTerm

        VOID                            *m_hIOCPTaskDispatch;           //  I/O completion port for dispatching tasks
        BOOL                            m_fIOCPHasRegisteredFile;       //  was a file registered with the IOCP?

        PfnCompletion                   m_pfnFileIOCompletion;          //  completion function specific to File I/O completions

#ifndef RTM
        //  IIRC we only have one completion thread, but this is by no means a requirement of the system or GQCS and
        //  if we ever want to parallelize IO completion this will need to be thread safe, so we just leave it thread
        //  safe today.  And just in case I misunderstood all our task mgr usage cases. ;-)
        volatile DWORD                  m_irrcpiLast;                   //  An "index" (unwrapped) for last completion packet info saved in m_rgcpiLast
        COMPLETIONPACKETINFO            m_rgcpiLast[10];                //  A round-robin buffer for the last 10 completion packets
#endif

    public:
        ULONG CPostedTasks() const      { return m_cPostedTasks; }
    };

class CGPTaskManager
{
    private:
        volatile BOOL       m_fInit;
        volatile ULONG      m_cPostedTasks;
        CAutoResetSignal    m_asigAllDone;
        CReaderWriterLock   m_rwlPostTasks;
        CTaskManager*       m_ptaskmanager;

    public:
        typedef DWORD (*PfnCompletion)( VOID* pvParam );

    private:
        typedef
            struct { PfnCompletion pfnCompletion; VOID *pvParam; CGPTaskManager* pThis; TaskInfo *pTaskInfo; }
            TMCallWrap, *PTMCallWrap;

    private:
        static DWORD __stdcall TMIDispatchGP( VOID* pvParam );
        static VOID TMIDispatch(    const DWORD     dwError,
                                    const DWORD_PTR dwThreadContext,
                                    const DWORD     dwCompletionKey1,
                                    const DWORD_PTR dwCompletionKey2 );

    public:
        CGPTaskManager();
        ~CGPTaskManager();

        ERR ErrTMInit( const ULONG cThread = 0 );
        ERR ErrTMPost(  PfnCompletion   pfnCompletion,
                        VOID *          pvParam     = NULL,
                        TaskInfo *      pTaskInfo   = NULL );
        VOID TMTerm();
        ULONG CPostedTasks() { return m_cPostedTasks - 1; }
};


BOOL FOSTaskIsTaskThread( void );

#endif  //  _OS_TASK_HXX_INCLUDED


