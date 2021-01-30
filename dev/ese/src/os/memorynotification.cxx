// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

typedef struct _MEMORY_NOTIFICATION
{
    _MEMORY_NOTIFICATION()
    {
        hMemory = NULL;
        hRegister = NULL;
        CallbackThreadId = 0;
        pfnCallback = NULL;
        dwContext = 0;
    }

    ~_MEMORY_NOTIFICATION()
    {
        Assert( hMemory == NULL );
        Assert( hRegister == NULL );
    }

    HANDLE      hMemory;
    HANDLE      hRegister;

    ULONG       CallbackThreadId;

    PfnMemNotification  pfnCallback;
    DWORD_PTR           dwContext;
} MEMORY_NOTIFICATION;

VOID CALLBACK MemoryNotificationCallback(
    PVOID   lpParameter,
    BOOLEAN)
{
    MEMORY_NOTIFICATION * const pNotification = (MEMORY_NOTIFICATION *)lpParameter;

    if ( AtomicCompareExchange( (LONG *)&pNotification->CallbackThreadId, 0, GetCurrentThreadId() ) != 0 )
    {
        return;
    }

    pNotification->pfnCallback( pNotification->dwContext );

    AtomicExchange( (LONG *)&pNotification->CallbackThreadId, 0 );
}

ERR ErrOSCreateLowMemoryNotification(
    PfnMemNotification const            pfnCallback,
    DWORD_PTR const                     dwContext,
    __out HMEMORY_NOTIFICATION * const  ppNotification )
{
    MEMORY_NOTIFICATION *pNotification;
    ERR err;

    Alloc( pNotification = new MEMORY_NOTIFICATION );
    pNotification->pfnCallback = pfnCallback;
    pNotification->dwContext = dwContext;

    *ppNotification = pNotification;

    pNotification->hMemory = CreateMemoryResourceNotification( LowMemoryResourceNotification );
    if ( pNotification->hMemory == NULL )
    {
        Call( ErrOSErrFromWin32Err( GetLastError() ) );
    }

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );

    *ppNotification = NULL;

    delete pNotification;
    return err;
}

ERR ErrOSRegisterMemoryNotification(
    __in HMEMORY_NOTIFICATION pvNotification )
{
    MEMORY_NOTIFICATION * const pNotification = (MEMORY_NOTIFICATION *)pvNotification;

    Assert( pNotification->CallbackThreadId != GetCurrentThreadId() );

    if ( pNotification->hRegister != NULL )
    {
        UnregisterWaitEx(
            pNotification->hRegister,
            INVALID_HANDLE_VALUE );
        pNotification->hRegister = NULL;
    }

    if ( !RegisterWaitForSingleObject(
            &pNotification->hRegister,
            pNotification->hMemory,
            MemoryNotificationCallback,
            pNotification,
            INFINITE,
            WT_EXECUTEONLYONCE ) )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    return JET_errSuccess;
}

ERR ErrOSQueryMemoryNotification(
    HMEMORY_NOTIFICATION const  pvNotification,
    __out BOOL * const          pfLowMemory )
{
    ERR err = JET_errSuccess;
    const MEMORY_NOTIFICATION * const pNotification = (MEMORY_NOTIFICATION *)pvNotification;

    if ( !QueryMemoryResourceNotification( pNotification->hMemory, pfLowMemory ) )
    {
        Call( ErrOSErrFromWin32Err( GetLastError() ) );
    }

HandleError:
    return err;
}

VOID OSUnregisterAndDestroyMemoryNotification(
    HMEMORY_NOTIFICATION const  pvNotification )
{
    MEMORY_NOTIFICATION * const pNotification = (MEMORY_NOTIFICATION *)pvNotification;

    Assert( pNotification->CallbackThreadId != GetCurrentThreadId() );

    if ( pNotification->hRegister != NULL )
    {
        UnregisterWaitEx( pNotification->hRegister, INVALID_HANDLE_VALUE );
        pNotification->hRegister = NULL;
    }

    if ( pNotification->hMemory != NULL )
    {
        CloseHandle( pNotification->hMemory );
        pNotification->hMemory = NULL;
    }

    delete pNotification;
}

