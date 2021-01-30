// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_TASK_HXX_INCLUDED
#define _OS_TASK_HXX_INCLUDED




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




const TICK  dtickTaskAbnormalLatencyEvent       = (60 * 1000);

const TICK  dtickTaskMaxDispatchDefault     = (1 * 60 * 1000);

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
    DWORD   m_dwPostTick;

    DWORD   m_dwDispatchTick;

    DWORD   m_dwEndTick;

    TaskInfoGenericStats * m_pTaskInfoGenericStats;
};


class TaskInfoGenericStats
{
public:
    TaskInfoGenericStats (__in PCSTR szTaskType, DWORD msMaxDispatch = dtickTaskMaxDispatchDefault, DWORD msMaxExecute = dtickTaskMaxExecuteDefault):
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


                PfnCompletion   m_pfnCompletion;
                DWORD           m_dwCompletionKey1;
                DWORD_PTR       m_dwCompletionKey2;


                CInvasiveList< CTaskNode, OffsetOfILE >::CElement m_ile;
        };

        typedef CInvasiveList< CTaskNode, CTaskNode::OffsetOfILE > TaskList;


        struct THREADCONTEXT
        {
            THREAD          thread;
            CTaskManager    *ptm;
            DWORD_PTR       dwThreadContext;
        };

        struct COMPLETIONPACKETINFO
        {

            BOOL                fDequeued;
            DWORD               gle;
            PfnCompletion       pfnCompletion;
            DWORD               dwCompletionKey1;
            DWORD_PTR           dwCompletionKey2;
            BOOL                fGQCSSuccess;
            TICK                tickStartWait;
            TICK                tickComplete;
#ifndef RTM

            TICK                dtickIdle;
            PfnCompletion       pfnIdle;
#endif
        };

    public:


        CTaskManager();
        ~CTaskManager();

        ERR ErrTMInit(  const ULONG                     cThread,
                        const DWORD_PTR *const          rgThreadContext     = NULL,
                        const BOOL                      fForceMaxThreads    = fFalse );
        VOID TMTerm();


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
        ERR     ErrAddThreadCheck( const BOOL fForceMaxThreads );
        BOOL    FNeedNewThread() const;

    private:

        volatile ULONG                  m_cThreadMax;
        volatile ULONG                  m_cPostedTasks;
        volatile ULONG                  m_cmsLastActivateThreadTime;
        CCriticalSection                m_critActivateThread;
        volatile ULONG                  m_cTasksThreshold;
        volatile ULONG                  m_cThread;
        THREADCONTEXT                   *m_rgThreadContext;

        TaskList                        m_ilTask;
        CCriticalSection                m_critTask;
        CSemaphore                      m_semTaskDispatch;
        CTaskNode                       **m_rgpTaskNode;

        VOID                            *m_hIOCPTaskDispatch;
        BOOL                            m_fIOCPHasRegisteredFile;

        PfnCompletion                   m_pfnFileIOCompletion;

#ifndef RTM
        volatile DWORD                  m_irrcpiLast;
        COMPLETIONPACKETINFO            m_rgcpiLast[10];
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

#endif


