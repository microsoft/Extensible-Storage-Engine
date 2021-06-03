// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


//  ctor

TASKMGR::TASKMGR()
{
    m_cContext      = 0;
    m_rgdwContext   = NULL;

    m_fInit         = fFalse;
}


//  dtor

TASKMGR::~TASKMGR()
{
}


//  initialize the TASKMGR

ERR TASKMGR::ErrInit( INST *const pinst, const INT cThread )
{
    ERR err;

    //  verify input

    Assert( NULL != pinst );
    if ( cThread <= 0 || cThread > 1000 )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    //  the task manager should not be initialized yet

    Assert( !m_fInit );

    //  verify the state of the TASKMGR

    Assert( 0 == m_cContext );
    Assert( NULL == m_rgdwContext );

    //  allocate an array of session handles (the per-thread contexts)

    Assert( sizeof( PIB * ) <= sizeof( DWORD_PTR ) );
    m_cContext = 0;
    Alloc( m_rgdwContext = new DWORD_PTR[cThread] );
    memset( m_rgdwContext, 0, sizeof( DWORD_PTR ) * cThread );

    //  open all sessions

    while ( (INT)m_cContext < cThread )
    {
        PIB *ppib;

        //  open a session

        Call( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
        ppib->grbitCommitDefault = JET_bitCommitLazyFlush;
        ppib->SetFSystemCallback();

        //  bind it to the per-thread context array (AFTER the session has been successfully opened)

        m_rgdwContext[m_cContext++] = DWORD_PTR( ppib );
    }

    //  initialize the real task manager

    Call( m_tm.ErrTMInit( m_cContext, m_rgdwContext ) );

    //  turn the task manager "on"

{
#ifdef DEBUG
    const BOOL fInitBefore =
#endif  //  DEBUG
    AtomicExchange( (LONG *)&m_fInit, fTrue );
    Assert( !fInitBefore );
}

    return JET_errSuccess;

HandleError:

    //  cleanup

    CallS( ErrTerm() );

    return err;
}


//  terminate the TASKMGR
//  NOTE: does not need to return an error-code (left-over from previous version)

ERR TASKMGR::ErrTerm()
{
    ULONG iThread;

    if ( m_fInit )
    {
        //  turn the task manager "off"
        //
#ifdef DEBUG
        const BOOL fInitBefore =
#endif  //  DEBUG
        AtomicExchange( (LONG *)&m_fInit, fFalse );
        Assert( fInitBefore );

        //  wait for everyone currently in the middle of posting a task to finish
        //
        m_cmsPost.Partition();
    }

    //  term the real task manager
    //
    m_tm.TMTerm();

    //  cleanup the per-thread contexts
    //
    if ( NULL != m_rgdwContext )
    {
        for ( iThread = 0; iThread < m_cContext; iThread++ )
        {
            Assert( NULL != m_rgdwContext );
            Assert( 0 != m_rgdwContext[iThread] );
            PIBEndSession( (PIB *)m_rgdwContext[iThread] );
        }

        delete[] m_rgdwContext;
        m_rgdwContext = NULL;
    }
    m_cContext = 0;

    return JET_errSuccess;
}


//  post a task

ERR TASKMGR::ErrPostTask( const TASK pfnTask, const ULONG_PTR ul, TaskInfo *pTaskInfo )
{
    ERR                 err;
    TASKMGRCONTEXT *    ptmc;
    INT                 iGroup;

    //  allocate a private context

    AllocR( ptmc = new TASKMGRCONTEXT );

    //  populate the context

    ptmc->pfnTask = pfnTask;
    ptmc->ul = ul;
    ptmc->pTaskInfo = pTaskInfo;

    //  enter the task-post metered section

    iGroup = m_cmsPost.Enter();

    if ( m_fInit )
    {
        if ( NULL != pTaskInfo )
        {
            pTaskInfo->NotifyPost();
        }

        //  pass the task down to the real task manager (will be dispatched via TASKMGR::Dispatch)

        err = m_tm.ErrTMPost( Dispatch, 0, DWORD_PTR( ptmc ) );
        CallSx( err, JET_errOutOfMemory );  //  we should only see JET_errSuccess or JET_errOutOfMemory
    }
    else
    {

        //  the task manager is not initialized so the task must be dropped

        err = ErrERRCheck( JET_errTaskDropped );
    }

    //  leave the task-post metered section

    m_cmsPost.Leave( iGroup );

    if ( err < JET_errSuccess )
    {

        //  cleanup the task context

        delete ptmc;
    }

    return err;
}


//  dispatch a task

VOID TASKMGR::Dispatch( const DWORD     dwError,
                        const DWORD_PTR dwThreadContext,
                        const DWORD     dwCompletionKey1,
                        const DWORD_PTR dwCompletionKey2 )
{
    PIB             *ppib;
    TASKMGRCONTEXT  *ptmc;
    TASK            pfnTask;
    ULONG_PTR       ul;
    TaskInfo *      pTaskInfo;

    //  verify input

    Assert( 0 != dwThreadContext );
    Assert( 0 == dwCompletionKey1 );
    Assert( 0 != dwCompletionKey2 );

    //  extract the session

    ppib = (PIB *)dwThreadContext;

    //  extract the task's context and clean it up

    ptmc = (TASKMGRCONTEXT *)dwCompletionKey2;
    pfnTask = ptmc->pfnTask;
    ul = ptmc->ul;
    pTaskInfo = ptmc->pTaskInfo;
    delete ptmc;

    if (pTaskInfo)
    {
        pTaskInfo->NotifyDispatch();
    }
    //  run the task

    Assert( NULL != pfnTask );
    pfnTask( ppib, ul );

    if (pTaskInfo)
    {
        pTaskInfo->NotifyEnd();
    }

}


