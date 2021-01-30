// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"



TASKMGR::TASKMGR()
{
    m_cContext      = 0;
    m_rgdwContext   = NULL;

    m_fInit         = fFalse;
}



TASKMGR::~TASKMGR()
{
}



ERR TASKMGR::ErrInit( INST *const pinst, const INT cThread )
{
    ERR err;


    Assert( NULL != pinst );
    if ( cThread <= 0 || cThread > 1000 )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }


    Assert( !m_fInit );


    Assert( 0 == m_cContext );
    Assert( NULL == m_rgdwContext );


    Assert( sizeof( PIB * ) <= sizeof( DWORD_PTR ) );
    m_cContext = 0;
    Alloc( m_rgdwContext = new DWORD_PTR[cThread] );
    memset( m_rgdwContext, 0, sizeof( DWORD_PTR ) * cThread );


    while ( (INT)m_cContext < cThread )
    {
        PIB *ppib;


        Call( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
        ppib->grbitCommitDefault = JET_bitCommitLazyFlush;
        ppib->SetFSystemCallback();


        m_rgdwContext[m_cContext++] = DWORD_PTR( ppib );
    }


    Call( m_tm.ErrTMInit( m_cContext, m_rgdwContext ) );


{
#ifdef DEBUG
    const BOOL fInitBefore =
#endif
    AtomicExchange( (LONG *)&m_fInit, fTrue );
    Assert( !fInitBefore );
}

    return JET_errSuccess;

HandleError:


    CallS( ErrTerm() );

    return err;
}



ERR TASKMGR::ErrTerm()
{
    ULONG iThread;

    if ( m_fInit )
    {
#ifdef DEBUG
        const BOOL fInitBefore =
#endif
        AtomicExchange( (LONG *)&m_fInit, fFalse );
        Assert( fInitBefore );

        m_cmsPost.Partition();
    }

    m_tm.TMTerm();

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



ERR TASKMGR::ErrPostTask( const TASK pfnTask, const ULONG_PTR ul, TaskInfo *pTaskInfo )
{
    ERR                 err;
    TASKMGRCONTEXT *    ptmc;
    INT                 iGroup;


    AllocR( ptmc = new TASKMGRCONTEXT );


    ptmc->pfnTask = pfnTask;
    ptmc->ul = ul;
    ptmc->pTaskInfo = pTaskInfo;


    iGroup = m_cmsPost.Enter();

    if ( m_fInit )
    {
        if ( NULL != pTaskInfo )
        {
            pTaskInfo->NotifyPost();
        }


        err = m_tm.ErrTMPost( Dispatch, 0, DWORD_PTR( ptmc ) );
        CallSx( err, JET_errOutOfMemory );
    }
    else
    {


        err = ErrERRCheck( JET_errTaskDropped );
    }


    m_cmsPost.Leave( iGroup );

    if ( err < JET_errSuccess )
    {


        delete ptmc;
    }

    return err;
}



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


    Assert( 0 != dwThreadContext );
    Assert( 0 == dwCompletionKey1 );
    Assert( 0 != dwCompletionKey2 );


    ppib = (PIB *)dwThreadContext;


    ptmc = (TASKMGRCONTEXT *)dwCompletionKey2;
    pfnTask = ptmc->pfnTask;
    ul = ptmc->ul;
    pTaskInfo = ptmc->pTaskInfo;
    delete ptmc;

    if (pTaskInfo)
    {
        pTaskInfo->NotifyDispatch();
    }

    Assert( NULL != pfnTask );
    pfnTask( ppib, ul );

    if (pTaskInfo)
    {
        pTaskInfo->NotifyEnd();
    }

}


