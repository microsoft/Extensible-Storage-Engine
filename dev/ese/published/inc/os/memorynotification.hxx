// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_MEMORY_NOTIFICATION_HXX_
#define _OS_MEMORY_NOTIFICATION_HXX_

typedef VOID *  HMEMORY_NOTIFICATION;

typedef void (*PfnMemNotification)( const DWORD_PTR dwContext );

ERR ErrOSCreateLowMemoryNotification(
    PfnMemNotification const            pfnCallback,
    DWORD_PTR const                     dwContext,
    __out HMEMORY_NOTIFICATION * const  ppNotification );

ERR ErrOSRegisterMemoryNotification(
    __in HMEMORY_NOTIFICATION           pvNotification );

ERR ErrOSQueryMemoryNotification(
    HMEMORY_NOTIFICATION const  pvNotification,
    __out BOOL * const          pfLowMemory );

VOID OSUnregisterAndDestroyMemoryNotification(
    HMEMORY_NOTIFICATION const  pvNotification );

#endif // _OS_MEMORY_NOTIFICATION_HXX_
