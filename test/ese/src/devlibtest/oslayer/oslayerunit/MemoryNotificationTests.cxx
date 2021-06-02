// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "osunitstd.hxx"

DWORD g_cNotifications = 0;

void NotificationCallback(
    const DWORD_PTR )
{
    InterlockedIncrement( &g_cNotifications );
}

// This does not write all bytes of the memory region requested.
// Instead it writes a single byte to each page (4096) of memory requested.
// This forces the Memory Manager to back up the request with memory.
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

    // use up all the memory - may not work on 32-bit systems with lots of RAM
    hHeap = HeapCreate( 0, 0, 0 );
    SIZE_T cbAlloc = (SIZE_T)OSMemoryAvailable();
    PVOID pvAlloc = HeapAlloc( hHeap, 0, cbAlloc );

    if ( pvAlloc == NULL )
    {
        // This can easily fail if we're running a WOW64 process on a 4 GB+ machine.
        goto HandleError;
    }

    InsecureZeroMemory( pvAlloc, cbAlloc );

    for ( i=0; i<100; i++ )
    {
        if ( g_cNotifications >= 2 )
        {
            // can only be notified once in one loop
            OSTestCheck( i > 0 );
            break;
        }

        Sleep( 100 );

        // test the re-register path
        OSTestCheckErr( ErrOSRegisterMemoryNotification( pNotification ) );

        cbAlloc = (SIZE_T)OSMemoryAvailable();
        pvAlloc = HeapAlloc( hHeap, 0, cbAlloc );
        if ( pvAlloc == NULL )
        {
            // This can easily fail if we're running a WOW64 process on a 4 GB+ machine.
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

