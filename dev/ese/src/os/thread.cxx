// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#include <process.h>


//  Thread Local Storage

//  Internal TLS structure

#include "_tls.hxx"


//  external globals

extern BOOL g_fProcessExit;

//  global capabilities

//  Global TLS List

BOOL g_fcsTlsGlobalInit;
CRITICAL_SECTION g_csTlsGlobal;
_TLS* g_ptlsGlobal;
_TLS* g_ptlsCleanGlobal;

//  Allocated TLS Entry

//NOTE: Cannot initialise this variable because the code that allocates
//      TLS and uses this variable to store the index executes before
//      CRTInit, which would subsequently re-initialise the variable
//      with the value specified here
//DWORD g_dwTlsIndex  = dwTlsInvalid;
static DWORD g_dwTlsIndex;

static void OSThreadITlsFree( _TLS * const ptls );


//  registers the given Internal TLS structure as the TLS for this context

static BOOL FOSThreadITlsRegister( _TLS* ptls )
{
    BOOL    fAllocatedTls   = fFalse;

    //  we are the first to register TLS

    EnterCriticalSection( &g_csTlsGlobal );
    
    if ( NULL == g_ptlsGlobal )
    {
        //  allocate a new TLS entry

        g_dwTlsIndex = TlsAlloc();
        if ( dwTlsInvalid == g_dwTlsIndex )
        {
            LeaveCriticalSection( &g_csTlsGlobal );
            return fFalse;
        }

        fAllocatedTls = fTrue;
    }

    Assert( dwTlsInvalid != g_dwTlsIndex );

    //  save the pointer to the given TLS
    const BOOL  fTLSPointerSet  = TlsSetValue( g_dwTlsIndex, ptls );
    if ( !fTLSPointerSet )
    {
        //  free TLS entry if we allocated one
        if ( fAllocatedTls )
        {
            Assert( NULL == g_ptlsGlobal );

            const BOOL  fTLSFreed   = TlsFree( g_dwTlsIndex );
#if !defined(_M_AMD64)
            Assert( fTLSFreed );        //  leak the TLS entry if we fail
#else
            Assert( fTLSFreed || g_fProcessExit );      //  leak the TLS entry if we fail
#endif

            g_dwTlsIndex = dwTlsInvalid;
        }

        LeaveCriticalSection( &g_csTlsGlobal );
        return fFalse;
    }

    //  add this TLS into the global list

    ptls->ptlsNext = g_ptlsGlobal;
    if ( ptls->ptlsNext )
    {
        ptls->ptlsNext->pptlsNext = &ptls->ptlsNext;
    }
    ptls->pptlsNext = &g_ptlsGlobal;
    g_ptlsGlobal = ptls;

    //  try to cleanup two entries in the global TLS list

    for ( INT i = 0; i < 2; i++ )
    {
        //  we have a TLS to clean

        _TLS* ptlsClean = g_ptlsCleanGlobal ? g_ptlsCleanGlobal : g_ptlsGlobal;

        if ( ptlsClean )
        {
            //  set the next TLS to clean

            g_ptlsCleanGlobal = ptlsClean->ptlsNext;

            //  we can cleanup this TLS if the thread has exited

            DWORD dwExitCode;
            if (    ptlsClean->hThread &&
                    GetExitCodeThread( ptlsClean->hThread, &dwExitCode ) &&
                    dwExitCode != STILL_ACTIVE )
            {
                //  free this TLS

                OSThreadITlsFree( ptlsClean );
            }
        }
    }

    LeaveCriticalSection( &g_csTlsGlobal );

    return fTrue;
}

//  unregisters the given Internal TLS structure as the TLS for this context

static void OSThreadITlsUnregister( _TLS* ptls )
{
    //  there should be TLSs registered

    EnterCriticalSection( &g_csTlsGlobal );
    Assert( g_ptlsGlobal != NULL );

    //  make sure that the clean pointer is not pointing at this TLS

    if ( g_ptlsCleanGlobal == ptls )
    {
        g_ptlsCleanGlobal = ptls->ptlsNext;
    }

    //  remove our TLS from the global TLS list

    if( ptls->ptlsNext )
    {
        ptls->ptlsNext->pptlsNext = ptls->pptlsNext;
    }
    *( ptls->pptlsNext ) = ptls->ptlsNext;

    //  we are the last to unregister our TLS

    if ( g_ptlsGlobal == NULL )
    {
        //  deallocate TLS entry

        Assert( dwTlsInvalid != g_dwTlsIndex );

        const BOOL  fTLSFreed = TlsFree( g_dwTlsIndex );
#if !defined(_M_AMD64)
            Assert( fTLSFreed );        //  leak the TLS entry if we fail
#else
            Assert( fTLSFreed || g_fProcessExit );      //  leak the TLS entry if we fail
#endif

        g_dwTlsIndex = dwTlsInvalid;
    }

    LeaveCriticalSection( &g_csTlsGlobal );
}

//  allocates TLS for the current thread

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

    //  we don't yet have any TLS

    _TLS* ptls  = ( NULL != g_ptlsGlobal ?
                        reinterpret_cast<_TLS *>( TlsGetValue( g_dwTlsIndex ) ) :
                        NULL );

    if ( NULL == ptls )
    {
        //  allocate memory for this thread's TLS

        ULONG cb_TLS = sizeof( _TLS ) + sizeof( OSTLS ) + g_cbUserTLSSize;
        cb_TLS = roundup( cb_TLS, 32 );
        ptls = (_TLS*) LocalAlloc( LMEM_ZEROINIT, cb_TLS );

        if ( !ptls )
        {
            return fFalse;
        }

        //  initialize internal TLS fields

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

        //  register our TLS

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

//  releases the specified TLS

static void OSThreadITlsFree( _TLS * const ptls )
{
    //  unregister our TLS

    OSThreadITlsUnregister( ptls );

    //  close our thread handle

    if ( ptls->hThread )
    {
        SetHandleInformation( ptls->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        const BOOL  fCloseOK    = CloseHandle( ptls->hThread );
        Assert( fCloseOK );
    }

    //  free our TLS storage

    BOOL fFreedTLSOK = !LocalFree( ptls );
    Assert( fFreedTLSOK );  //  leak the TLS block if we fail
}


//  returns the pointer to the current context's true local storage / the
//  internal _TLS.  if the thread does not yet have TLS, allocate it.

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

//  returns the pointer to the External OS Layer portion of the provided 
//  internal _TLS.

OSTLS* const PostlsFromIntTLS( _TLS * const ptls )
{
    Assert( ptls );
    return &( ptls->ostls );
}

//  returns the pointer to the External OS Layer portion of the current 
//  context's local storage from the internal _TLS.

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

//  returns the pointer to the USER portion of the provided internal _TLS.

TLS* const PtlsFromIntTLS( _TLS * const ptls )
{
    Assert( ptls );
    return (TLS*)( ptls->rgUserTLS );
}

//  returns the pointer to the USER portion of the current context's local
//  storage from the internal _TLS.

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


//  Thread Management

//  suspends execution of the current context for the specified interval
//    0 == yield to any ready thread at same priority
//    1 == yield to any ready thread
//    2+ == sleep for at least a quantum

void UtilSleep( const DWORD cmsec )
{
    //  we should never sleep more than this arbitrary interval

    const DWORD cmsecWait = min( cmsec, 60 * 1000 );

    //  sleep

    OnThreadWaitBegin();
    SleepEx( cmsecWait, FALSE );
    OnThreadWaitEnd();
}

//  thread base function (used by ErrUtilThreadCreate)

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

    //  expect to have a clean sync state as a pre-check

    //  can assert stronger statements here in UtilThreadIThreadBase than in JET API as we're guaranteed 
    //  to be at the top of a stack and not in a callback within the JET API.
    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    ETThreadStart( p_thread, p_thread->pfnStart, (void*)p_thread->dwParam );

    //  call thread function, catching any exceptions that might happen
    //  if requested

    if ( g_fCatchExceptions )
    {
        TRY
        {
            dwExitCode = ( p_thread->pfnStart )( p_thread->dwParam );
        }
        EXCEPT( ExceptionFail( p_thread->szStart ) )
        {
            AssertPREFIX( !"This code path should be impossible (the exception-handler should have terminated the process)." );
            dwExitCode = 0;     //  UNDONE: is there a more appropriate error code?
        }
        ENDEXCEPT
    }
    else
    {
        dwExitCode = ( p_thread->pfnStart )( p_thread->dwParam );
    }

    //  expect to have a clean sync state when done as well

    CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );

    //  declare this thread as done

    p_thread->fFinish = fTrue;

    //  cleanup our context if we are ending ourself

    if ( p_thread->fEndSelf )
    {
        UtilThreadEnd( THREAD( p_thread ) );
    }

    //  exit with code from thread function

    return dwExitCode;
}

//  thread priority table

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

//  creates a thread with the specified attributes

const ERR ErrUtilThreadICreate(
    const PUTIL_THREAD_PROC     pfnStart,
    const DWORD                 cbStack,
    const EThreadPriority       priority,
    THREAD* const               pThread,
    const DWORD_PTR             dwParam,
    const _TCHAR* const         szStart )
{
    ERR err = JET_errSuccess;

    //  allocate memory to pass thread args

    _THREAD* const p_thread = (_THREAD*)LocalAlloc( 0, sizeof( _THREAD ) );
    if ( NULL == p_thread )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  setup thread args

    p_thread->pfnStart      = pfnStart;
    p_thread->dwParam       = dwParam;
    p_thread->szStart       = szStart;
    p_thread->hThread       = NULL;
    p_thread->idThread      = 0;
    p_thread->fFinish       = fFalse;
    p_thread->fEndSelf      = fFalse;

    //  create the thread in suspended animation

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

    //  set the thread priority, ignoring any errors encountered because we may
    //  not be allowed to set the priority for a legitimate reason (e.g. quotas)

    SetThreadPriority( p_thread->hThread, rgthreadpriority[ priority ] );
    if ( OSSyncGetProcessorCountMax() == 1 )
    {
        SetThreadPriorityBoost( p_thread->hThread, TRUE );
    }

    ETThreadCreate( p_thread, p_thread->pfnStart, (void*)p_thread->dwParam );

    //  activate the thread

    ResumeThread( p_thread->hThread );

    //  return the handle to the thread

    *pThread = THREAD( p_thread );

HandleError:
    if ( err < JET_errSuccess )
    {
        UtilThreadEnd( THREAD( p_thread ) );
        *pThread = THREAD( NULL );

        if ( err == JET_errOutOfMemory || err == JET_errOutOfThreads )
        {
            //  indicate a low memory error (unfortunately, w/o an instance)
            //
            OSUHAEmitFailureTag( NULL, HaDbFailureTagMemory, L"671ee677-ee67-4d7f-aa04-9e86ac635f6d" );
        }
    }
    return err;
}

//  waits for the specified thread to exit and returns its return value

const DWORD UtilThreadEnd( const THREAD Thread )
{
    DWORD dwExitCode = 0;

    //  we have a thread object

    _THREAD* const p_thread = (_THREAD*)Thread;

    if ( p_thread )
    {
        //  the thread was created successfully

        if ( p_thread->hThread )
        {
            //  we are trying to end ourself

            if ( p_thread->idThread == GetCurrentThreadId() )
            {
                //  indicate that we are ending ourself

                p_thread->fEndSelf = fTrue;

                //  do not actually end the thread until it has finished

                if ( !p_thread->fFinish )
                {
                    return 0;
                }
            }

            //  we are trying to end another thread

            else
            {
                //  wait for the specified thread to either finish or terminate

                WaitForSingleObjectEx( p_thread->hThread, INFINITE, FALSE );

                //  fetch the thread's exit code

                GetExitCodeThread( p_thread->hThread, &dwExitCode );
            }

            //  cleanup the thread's context

            SetHandleInformation( p_thread->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
            CloseHandle( p_thread->hThread );
        }
        LocalFree( p_thread );
    }

    //  return the thread's exit code

    return dwExitCode;
}


//  returns the current thread's ID

const DWORD DwUtilThreadId()
{
    return GetCurrentThreadId();
}


//  post-terminate thread subsystem

void OSThreadPostterm()
{
    //  purge our TLS list

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

//  pre-init thread subsystem

BOOL FOSThreadPreinit()
{
    //  reset global TLS list

    if ( !InitializeCriticalSectionAndSpinCount( &g_csTlsGlobal, 0 ) )
    {
        return fFalse;
    }
    g_fcsTlsGlobalInit = fTrue;
    g_ptlsGlobal = NULL;
    g_dwTlsIndex = dwTlsInvalid;

    return fTrue;
}


//  terminate thread subsystem

void OSThreadTerm()
{
    //  nop
}

//  init thread subsystem

ERR ErrOSThreadInit()
{
    //  nop

    return JET_errSuccess;
}

//  Adjusts the thread's IO priority
//  Can't be nested. Each call to begin must be matched with one call to end.
//

void UtilThreadBeginLowIOPriority(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN); // ignore errors
}

void UtilThreadEndLowIOPriority(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END); // ignore errors
}
