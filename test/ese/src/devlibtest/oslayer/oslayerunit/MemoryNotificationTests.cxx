// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "osunitstd.hxx"

DWORD g_cNotifications = 0;

void NotificationCallback(
    const DWORD_PTR )
{
    InterlockedIncrement( &g_cNotifications );
}

void
InsecureZeroMemory(
    _Out_writes_bytes_all_(cnt) void* pvStart,
    _In_ SIZE_T cb
    )
{
    for ( BYTE* pb = (BYTE*) pvStart; (DWORD_PTR) pb < ( (DWORD_PTR) pvStart + cb); pb += OSMemoryPageCommitGranularity() )
    {
        *pb = 0x42;
    }
}

CUnitTest( OslayerNotifiesOnLowMemory, bitExplicitOnly, "Test for low memory notification" );
ERR OslayerNotifiesOnLowMemory::ErrTest()
{
    ERR err = JET_errSuccess;
    HMEMORY_NOTIFICATION pNotification = NULL;
    HANDLE hHeap = NULL;
    BOOL fLowMemory;
    INT i;

    COSLayerPreInit oslayer;
    OSTestCall( ErrOSInit() );

    printf( "Test low memory notification\n" );
    OSTestCheckErr( ErrOSCreateLowMemoryNotification(
            NotificationCallback,
            0,
            &pNotification ) );
    OSTestCheck( pNotification != NULL );

    OSTestCheckErr( ErrOSRegisterMemoryNotification( pNotification ) );

    OSTestCheckErr( ErrOSQueryMemoryNotification( pNotification, &fLowMemory ) );
    OSTestCheck( !fLowMemory );

    hHeap = HeapCreate( 0, 0, 0 );
    SIZE_T cbAlloc = (SIZE_T)OSMemoryAvailable();
    PVOID pvAlloc = HeapAlloc( hHeap, 0, cbAlloc );

    if ( pvAlloc == NULL )
    {
        goto HandleError;
    }

    InsecureZeroMemory( pvAlloc, cbAlloc );

    for ( i=0; i<100; i++ )
    {
        if ( g_cNotifications >= 2 )
        {
            OSTestCheck( i > 0 );
            break;
        }

        Sleep( 100 );

        OSTestCheckErr( ErrOSRegisterMemoryNotification( pNotification ) );

        cbAlloc = (SIZE_T)OSMemoryAvailable();
        pvAlloc = HeapAlloc( hHeap, 0, cbAlloc );
        if ( pvAlloc == NULL )
        {
            goto HandleError;
        }

        InsecureZeroMemory( pvAlloc, cbAlloc );
    }
    OSTestCheck( g_cNotifications >= 1 );

    HeapDestroy( hHeap );
    hHeap = NULL;

    for ( i=0; i<100; i++ )
    {
        OSTestCheckErr( ErrOSQueryMemoryNotification( pNotification, &fLowMemory ) );
        if ( !fLowMemory )
        {
            break;
        }
        Sleep( 100 );
    }
    OSTestCheckErr( ErrOSQueryMemoryNotification( pNotification, &fLowMemory ) );
    OSTestCheck( !fLowMemory );

HandleError:

    OSUnregisterAndDestroyMemoryNotification( pNotification );
    pNotification = NULL;

    if ( hHeap != NULL )
    {
        HeapDestroy( hHeap );
        hHeap = NULL;
    }

    OSTerm();
    return err;
}

