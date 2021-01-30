// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#include <process.h>




#include "_tls.hxx"



extern BOOL g_fProcessExit;



BOOL g_fcsTlsGlobalInit;
CRITICAL_SECTION g_csTlsGlobal;
_TLS* g_ptlsGlobal;
_TLS* g_ptlsCleanGlobal;


static DWORD g_dwTlsIndex;

static void OSThreadITlsFree( _TLS * const ptls );



static BOOL FOSThreadITlsRegister( _TLS* ptls )
{
    BOOL    fAllocatedTls   = fFalse;


    EnterCriticalSection( &g_csTlsGlobal );
    
    if ( NULL == g_ptlsGlobal )
    {

        g_dwTlsIndex = TlsAlloc();
        if ( dwTlsInvalid == g_dwTlsIndex )
        {
            LeaveCriticalSection( &g_csTlsGlobal );
            return fFalse;
        }

        fAllocatedTls = fTrue;
    }

    Assert( dwTlsInvalid != g_dwTlsIndex );

    const BOOL  fTLSPointerSet  = TlsSetValue( g_dwTlsIndex, ptls );
    if ( !fTLSPointerSet )
    {
        if ( fAllocatedTls )
        {
            Assert( NULL == g_ptlsGlobal );

            const BOOL  fTLSFreed   = TlsFree( g_dwTlsIndex );
#if !defined(_AMD64_)
            Assert( fTLSFreed );
#else
            Assert( fTLSFreed || g_fProcessExit );
#endif

            g_dwTlsIndex = dwTlsInvalid;
        }

        LeaveCriticalSection( &g_csTlsGlobal );
        return fFalse;
    }


    ptls->ptlsNext = g_ptlsGlobal;
    if ( ptls->ptlsNext )
    {
        ptls->ptlsNext->pptlsNext = &ptls->ptlsNext;
    }
    ptls->pptlsNext = &g_ptlsGlobal;
    g_ptlsGlobal = ptls;


    for ( INT i = 0; i < 2; i++ )
    {

        _TLS* ptlsClean = g_ptlsCleanGlobal ? g_ptlsCleanGlobal : g_ptlsGlobal;

        if ( ptlsClean )
        {

            g_ptlsCleanGlobal = ptlsClean->ptlsNext;


            DWORD dwExitCode;
            if (    ptlsClean->hThread &&
                    GetExitCodeThread( ptlsClean->hThread, &dwExitCode ) &&
                    dwExitCode != STILL_ACTIVE )
            {

                OSThreadITlsFree( ptlsClean );
            }
        }
    }

    LeaveCriticalSection( &g_csTlsGlobal );

    return fTrue;
}


static void OSThreadITlsUnregister( _TLS* ptls )
{

    EnterCriticalSection( &g_csTlsGlobal );
    Assert( g_ptlsGlobal != NULL );


    if ( g_ptlsCleanGlobal == ptls )
    {
        g_ptlsCleanGlobal = ptls->ptlsNext;
    }


    if( ptls->ptlsNext )
    {
        ptls->ptlsNext->pptlsNext = ptls->pptlsNext;
    }
    *( ptls->pptlsNext ) = ptls->ptlsNext;


    if ( g_ptlsGlobal == NULL )
    {

        Assert( dwTlsInvalid != g_dwTlsIndex );

        const BOOL  fTLSFreed = TlsFree( g_dwTlsIndex );
#if !defined(_AMD64_)
            Assert( fTLSFreed );
#else
            Assert( fTLSFreed || g_fProcessExit );
#endif

        g_dwTlsIndex = dwTlsInvalid;
    }

    LeaveCriticalSection( &g_csTlsGlobal );
}


LOCAL ULONG g_cbUserTLSSize = 0;

void OSPrepreinitSetUserTLSSize( const ULONG cbUserTLSSize )
{
    Assert( ( g_cbUserTLSSize == 0 ) || ( g_cbUserTLSSize == cbUserTLSSize ) );
    g_cbUserTLSSize = cbUserTLSSize;
}

BOOL FOSThreadITlsAlloc()
{
#ifdef OS_LAYER_VIOLATIONS
    Expected( g_cbUserTLSSize == ESE_USER_TLS_SIZE );
#endif


    _TLS* ptls  = ( NULL != g_ptlsGlobal ?
                        reinterpret_cast<_TLS *>( TlsGetValue( g_dwTlsIndex ) ) :
                        NULL );

    if ( NULL == ptls )
    {

        ULONG cb_TLS = sizeof( _TLS ) + sizeof( OSTLS ) + g_cbUserTLSSize;
        cb_TLS = roundup( cb_TLS, 32 );
        ptls = (_TLS*) LocalAlloc( LMEM_ZEROINIT, cb_TLS );

        if ( !ptls )
        {
            return fFalse;
        }


        ptls->dwThreadId = GetCurrentThreadId();
        if ( DuplicateHandle(   GetCurrentProcess(),
                                GetCurrentThread(),
                                GetCurrentProcess(),
                                &ptls->hThread,
                                THREAD_QUERY_INFORMATION,
                                FALSE,
                                0 ) )
        {
            SetHandleInformation( ptls->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
        }


        if ( !FOSThreadITlsRegister( ptls ) )
        {
            if ( ptls->hThread )
            {
                SetHandleInformation( ptls->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
                CloseHandle( ptls->hThread );
            }
            LocalFree( ptls );
            return fFalse;
        }
    }

    return fTrue;
}


static void OSThreadITlsFree( _TLS * const ptls )
{

    OSThreadITlsUnregister( ptls );


    if ( ptls->hThread )
    {
        SetHandleInformation( ptls->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        const BOOL  fCloseOK    = CloseHandle( ptls->hThread );
        Assert( fCloseOK );
    }


    BOOL fFreedTLSOK = !LocalFree( ptls );
    Assert( fFreedTLSOK );
}



_TLS* const _Ptls()
{
    DWORD dwSavedGLE = GetLastError();

    _TLS * ptls = ( NULL != g_ptlsGlobal ?
                        reinterpret_cast<_TLS *>( TlsGetValue( g_dwTlsIndex ) ) :
                        NULL );

    if ( NULL == ptls )
    {
        while ( !FOSThreadITlsAlloc() )
        {
            Sleep( 2 );
        }

        Assert( dwTlsInvalid != g_dwTlsIndex );
        ptls = reinterpret_cast<_TLS *>( TlsGetValue( g_dwTlsIndex ) );
    }

    AssertPREFIX( NULL != ptls );

    SetLastError( dwSavedGLE );
    return ptls;
}


OSTLS* const PostlsFromIntTLS( _TLS * const ptls )
{
    Assert( ptls );
    return &( ptls->ostls );
}


OSTLS* const Postls()
{
    OnDebug( DWORD dwSavedGLE = GetLastError() );

    AssertPREFIX( NULL != _Ptls() );
    OSTLS * postls = PostlsFromIntTLS( _Ptls() );

    Assert( dwSavedGLE == GetLastError() );
    return postls;
}

OSTLS* PostlsFromTLS( TLS* ptls )
{
    Assert( ptls != NULL );
    _TLS* _ptls = CONTAINING_RECORD( ptls, _TLS, rgUserTLS );
    Assert( _ptls->dwThreadId == GetCurrentThreadId() );
    return &_ptls->ostls;
}


TLS* const PtlsFromIntTLS( _TLS * const ptls )
{
    Assert( ptls );
    return (TLS*)( ptls->rgUserTLS );
}


TLS* const Ptls()
{
    OnDebug( DWORD dwSavedGLE = GetLastError() );
#ifdef OS_LAYER_VIOLATIONS
    Assert( g_cbUserTLSSize == ESE_USER_TLS_SIZE );
#else
    Assert( g_cbUserTLSSize > 0 );
#endif

    AssertPREFIX( NULL != _Ptls() );
    TLS * ptls = PtlsFromIntTLS( _Ptls() );

    Assert( dwSavedGLE == GetLastError() );
    return ptls;
}




void UtilSleep( const DWORD cmsec )
{

    const DWORD cmsecWait = min( cmsec, 60 * 1000 );


    OnThreadWaitBegin();
    SleepEx( cmsecWait, FALSE );
    OnThreadWaitEnd();
}


struct _THREAD
{
    PUTIL_THREAD_PROC   pfnStart;
    DWORD_PTR           dwParam;
    const _TCHAR*       szStart;
    HANDLE              hThread;
    DWORD               idThread;
    BOOL                fFinish;
    BOOL                fEndSelf;
};

BOOL g_fCatchExceptions = fTrue;

void COSLayerPreInit::SetCatchExceptionsOnBackgroundThreads( const BOOL fCatch )
{
    g_fCatchExceptions = fCatch;
}

DWORD WINAPI UtilThreadIThreadBase( _THREAD* const p_thread )
{
    DWORD   dwExitCode;


    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    ETThreadStart( p_thread, p_thread->pfnStart, (void*)p_thread->dwParam );


    if ( g_fCatchExceptions )
    {
        TRY
        {
            dwExitCode = ( p_thread->pfnStart )( p_thread->dwParam );
        }
        EXCEPT( ExceptionFail( p_thread->szStart ) )
        {
            AssertPREFIX( !"This code path should be impossible (the exception-handler should have terminated the process)." );
            dwExitCode = 0;
        }
        ENDEXCEPT
    }
    else
    {
        dwExitCode = ( p_thread->pfnStart )( p_thread->dwParam );
    }


    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );


    p_thread->fFinish = fTrue;


    if ( p_thread->fEndSelf )
    {
        UtilThreadEnd( THREAD( p_thread ) );
    }


    return dwExitCode;
}


const INT rgthreadpriority[] =
{
    THREAD_PRIORITY_IDLE,
    THREAD_PRIORITY_LOWEST,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST,
    THREAD_PRIORITY_TIME_CRITICAL
};


const ERR ErrUtilThreadICreate(
    const PUTIL_THREAD_PROC     pfnStart,
    const DWORD                 cbStack,
    const EThreadPriority       priority,
    THREAD* const               pThread,
    const DWORD_PTR             dwParam,
    const _TCHAR* const         szStart )
{
    ERR err = JET_errSuccess;


    _THREAD* const p_thread = (_THREAD*)LocalAlloc( 0, sizeof( _THREAD ) );
    if ( NULL == p_thread )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }


    p_thread->pfnStart      = pfnStart;
    p_thread->dwParam       = dwParam;
    p_thread->szStart       = szStart;
    p_thread->hThread       = NULL;
    p_thread->idThread      = 0;
    p_thread->fFinish       = fFalse;
    p_thread->fEndSelf      = fFalse;


    p_thread->hThread = HANDLE( CreateThread(   NULL,
                                                cbStack,
                                                LPTHREAD_START_ROUTINE( UtilThreadIThreadBase ),
                                                (void*) p_thread,
                                                CREATE_SUSPENDED,
                                                &p_thread->idThread ) );

    if ( !p_thread->hThread )
    {
        Error( ErrERRCheck( JET_errOutOfThreads ) );
    }
    SetHandleInformation( p_thread->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );


    SetThreadPriority( p_thread->hThread, rgthreadpriority[ priority ] );
    if ( OSSyncGetProcessorCountMax() == 1 )
    {
        SetThreadPriorityBoost( p_thread->hThread, TRUE );
    }

    ETThreadCreate( p_thread, p_thread->pfnStart, (void*)p_thread->dwParam );


    ResumeThread( p_thread->hThread );


    *pThread = THREAD( p_thread );

HandleError:
    if ( err < JET_errSuccess )
    {
        UtilThreadEnd( THREAD( p_thread ) );
        *pThread = THREAD( NULL );

        if ( err == JET_errOutOfMemory || err == JET_errOutOfThreads )
        {
            OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"671ee677-ee67-4d7f-aa04-9e86ac635f6d" );
        }
    }
    return err;
}


const DWORD UtilThreadEnd( const THREAD Thread )
{
    DWORD dwExitCode = 0;


    _THREAD* const p_thread = (_THREAD*)Thread;

    if ( p_thread )
    {

        if ( p_thread->hThread )
        {

            if ( p_thread->idThread == GetCurrentThreadId() )
            {

                p_thread->fEndSelf = fTrue;


                if ( !p_thread->fFinish )
                {
                    return 0;
                }
            }


            else
            {

                WaitForSingleObjectEx( p_thread->hThread, INFINITE, FALSE );


                GetExitCodeThread( p_thread->hThread, &dwExitCode );
            }


            SetHandleInformation( p_thread->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
            CloseHandle( p_thread->hThread );
        }
        LocalFree( p_thread );
    }


    return dwExitCode;
}



const DWORD DwUtilThreadId()
{
    return GetCurrentThreadId();
}



void OSThreadPostterm()
{

    while ( g_ptlsGlobal )
    {
        OSThreadITlsFree( g_ptlsGlobal );
    }

    if( g_fcsTlsGlobalInit )
    {
        DeleteCriticalSection( &g_csTlsGlobal );
        g_fcsTlsGlobalInit = fFalse;
    }
}


BOOL FOSThreadPreinit()
{

    if ( !InitializeCriticalSectionAndSpinCount( &g_csTlsGlobal, 0 ) )
    {
        return fFalse;
    }
    g_fcsTlsGlobalInit = fTrue;
    g_ptlsGlobal = NULL;
    g_dwTlsIndex = dwTlsInvalid;

    return fTrue;
}



void OSThreadTerm()
{
}


ERR ErrOSThreadInit()
{

    return JET_errSuccess;
}


void UtilThreadBeginLowIOPriority(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
}

void UtilThreadEndLowIOPriority(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
}
