// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


class PIB;
class TaskInfo;

class TASKMGR
{
    public:

        typedef VOID (*TASK)( PIB * const ppib, const ULONG_PTR ul );

    private:


        struct TASKMGRCONTEXT
        {
            TASK        pfnTask;
            ULONG_PTR   ul;
            TaskInfo *  pTaskInfo;
        };

    public:

        TASKMGR();
        ~TASKMGR();

        ERR ErrInit( INST *const pinst, const INT cThread );
        ERR ErrTerm();
        ERR ErrPostTask( const TASK pfnTask, const ULONG_PTR ul, TaskInfo * pTaskInfo = NULL );

    private:

        static VOID Dispatch(   const DWORD     dwError,
                                const DWORD_PTR dwThreadContext,
                                const DWORD     dwCompletionKey1,
                                const DWORD_PTR dwCompletionKey2 );

    private:

        ULONG               m_cContext;
        DWORD_PTR           *m_rgdwContext;

        volatile BOOL       m_fInit;
        CMeteredSection     m_cmsPost;

        CTaskManager        m_tm;
};



