// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//================================================================
// Event logging helper functions.
//================================================================
//

#include "ese_common.hxx"
#include "eseloadlibrary.hxx"
#include <process.h>
#include <stdlib.h>

#define ESETEST_ELHELPER_STRING_BUFFER_SIZE     1024
#define ESETEST_ELHELPER_MAXIMUM_WAIT_TIME      1

#define wszAdvapi32             L"advapi32.dll"
const wchar_t * const g_mwszzAdvapi32CoreSystemBroken       = wszAdvapi32 L"\0";

typedef struct{
    PFNEVENTLOGGING pfnCallback;
    HANDLE              hEventLog;
    PWSTR*              pwszEventSources;
    size_t              cEventSources;
    DWORD               dwTimeMin;
    DWORD               dwTimeMax;
    PWORD               pEventTypes;
    size_t              cEventTypes;
    PWORD               pEventCategories;
    size_t              cEventCategories;
    PDWORD              pEventIds;  
    size_t              cEventIds;  
    HANDLE              hFile;
    PVOID               pUserData;
    PBYTE               pBuffer;
    DWORD               cbBufferSize;
    HANDLE              hObjects[ 2 ];

    // Won't be used after initialization. For debugging purposes only.
    PWSTR               wszEventLog;
    SYSTEMTIME          stTimeMin;
    SYSTEMTIME          stTimeMax;
    PWSTR               wszLogFile;
    BOOL                fThreadActive;
} EseEventLoggingQuery;

#ifndef BUILD_ENV_IS_WPHONE 

// Global 'functors' to help with the loadlibrary's.

NTOSFuncPtr( g_pfnOpenEventLogW, g_mwszzAdvapi32CoreSystemBroken, OpenEventLogW, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( g_pfnReadEventLogW, g_mwszzAdvapi32CoreSystemBroken, ReadEventLogW, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( g_pfnCloseEventLog, g_mwszzAdvapi32CoreSystemBroken, CloseEventLog, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( g_pfnNotifyChangeEventLog, g_mwszzAdvapi32CoreSystemBroken, NotifyChangeEventLog, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );


// Function implementations.

VOID
IEventLoggingPrintToFile(
    _In_ HANDLE hFile,
    _In_ PSTR   szString
)
{
    DWORD dw;
    if ( !WriteFile( hFile, szString, ( DWORD )strlen( szString ), &dw, NULL ) ){
        OutputWarning( "%s(): failed to write to file!" CRLF, __FUNCTION__ );
    }
}

BOOL
IEventLoggingProcessEvents(
    EseEventLoggingQuery* hQuery
)
{
    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fReturn;
    DWORD cbRead;
    DWORD cbNeed;   
    PBYTE pBufferAux, pBuffer2;
    EVENTLOGRECORD* pelg;
    PWSTR wszEventSource;
    DWORD dwTimeGenerated;
    WORD wEventType;
    WORD wEventCategory;
    DWORD dwEventId;
    WORD cStrings;
    DWORD cbRawData;        
    PWSTR* rgpwszStrings = NULL;
    PWSTR rgpwszStringsBuffer;
    PVOID pvRawData;
    SYSTEMTIME stTimeGenerated;
    BOOL fResult = FALSE;
    HMODULE hModule;

    JET_ERR err = g_pfnReadEventLogW.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): ReadEventLogW() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    // Process all the events up to EOF.
    while ( ERROR_HANDLE_EOF != dwLastError ){
        // We've seen access denied here. Retry 10 times.
        for ( DWORD iRetry = 0 ; iRetry < 10 ; iRetry++ ){
            // Read as many entries as it fits in our buffer.
            fReturn = g_pfnReadEventLogW( hQuery->hEventLog,
                                        EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                                        0,
                                        hQuery->pBuffer,
                                        hQuery->cbBufferSize,
                                        &cbRead,
                                        &cbNeed );
            if ( fReturn || GetLastError() != ERROR_ACCESS_DENIED ){
                break;
            }
            Sleep( 1000 );
        }
        
        if ( fReturn ){
            pBufferAux = hQuery->pBuffer;

            // Process all the events returned.
            while ( ( ( DWORD )( pBufferAux - hQuery->pBuffer ) ) < cbRead ){
                pelg = ( EVENTLOGRECORD* )pBufferAux;
                wszEventSource = ( WCHAR* )( pelg + 1 );
                dwTimeGenerated = pelg->TimeGenerated;
                wEventType = pelg->EventType;
                wEventCategory = pelg->EventCategory;
                dwEventId = (pelg->EventID & 0x0000FFFF);   // lower 16 bits contain the ID

                // It's time to filter out some events.
                // First, by event ID.
                if ( hQuery->pEventIds ){
                    BOOL fFound = FALSE;
                    for ( size_t i = 0 ; i < hQuery->cEventIds ; i++ ){
                        if ( hQuery->pEventIds[ i ] == dwEventId ){ 
                            fFound = TRUE;
                            break;
                        }
                    }
                    if ( !fFound ){
                        goto NextEvent;
                    }
                }
                
                // Now, by time generated.
                if ( ! (( dwTimeGenerated >= hQuery->dwTimeMin ) &&
                        ( dwTimeGenerated <= hQuery->dwTimeMax ) ) ){
                        goto NextEvent;
                }

                // Now, by event source.
                if ( hQuery->pwszEventSources ){
                    BOOL fFound = FALSE;
                    for ( size_t i = 0 ; i < hQuery->cEventSources ; i++ ){
                        if ( !wcscmp( wszEventSource, hQuery->pwszEventSources[ i ] ) ){
                            fFound = TRUE;
                            break;
                        }
                    }
                    if ( !fFound ){
                        goto NextEvent;
                    }
                }

                // Now, by event type.
                if ( hQuery->pEventTypes ){
                    BOOL fFound = FALSE;
                    for ( size_t i = 0 ; i < hQuery->cEventTypes ; i++ ){
                        if ( hQuery->pEventTypes[ i ] == wEventType ){
                            fFound = TRUE;
                            break;
                        }
                    }
                    if ( !fFound ){
                        goto NextEvent;
                    }
                }

                // Now, by event category.
                if ( hQuery->pEventCategories ){
                    BOOL fFound = FALSE;
                    for ( size_t i = 0 ; i < hQuery->cEventCategories ; i++ ){
                        if ( hQuery->pEventCategories[ i ] == wEventCategory ){
                            fFound = TRUE;
                            break;
                        }
                    }
                    if ( !fFound ){
                        goto NextEvent;
                    }
                }

                // At this point, we know that the event matches our constraints.               
                cStrings = pelg->NumStrings;
                rgpwszStrings = NULL;
                if ( cStrings ){
                    rgpwszStrings = new PWSTR[ cStrings ];                  
                    if ( rgpwszStrings ){
                        rgpwszStrings[ 0 ] = ( WCHAR* )( pBufferAux + pelg->StringOffset );
                        for ( WORD i = 1 ; i < cStrings ; i++ ){
                            rgpwszStringsBuffer = rgpwszStrings[ i - 1 ];
                            rgpwszStrings[ i ] = rgpwszStringsBuffer + wcslen( rgpwszStringsBuffer ) + 1;
                        }                   
                    }
                    else{
                        OutputError( "%s(): failed to allocate buffer for rgpwszStrings!" CRLF, __FUNCTION__ );
                        goto Cleanup;
                    }
                }
                cbRawData = pelg->DataLength;
                pvRawData = cbRawData ? ( pBufferAux + pelg->DataOffset ) : NULL;

                // Compute the time generated in SYSTEMTIME format.
                SecondsSince1970ToSystemTime( dwTimeGenerated, &stTimeGenerated );

                // Callback.
                if ( hQuery->pfnCallback ){
                    hQuery->pfnCallback( hQuery->wszEventLog,
                                            wszEventSource,
                                            &stTimeGenerated,
                                            wEventType,
                                            wEventCategory,
                                            dwEventId,
                                            rgpwszStrings,
                                            cStrings,
                                            pvRawData,
                                            cbRawData,
                                            hQuery->pUserData );
                }

                // Write to the file.
                if ( INVALID_HANDLE_VALUE != hQuery->hFile ){
                    CHAR szBuffer[ ESETEST_ELHELPER_STRING_BUFFER_SIZE ];
                    IEventLoggingPrintToFile( hQuery->hFile, CRLF );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Event log: %S" CRLF, hQuery->wszEventLog );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Event source: %S" CRLF, wszEventSource );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Time generated: %04u/%02u/%02u %02u:%02u:%02u" CRLF,
                                stTimeGenerated.wYear, stTimeGenerated.wMonth, stTimeGenerated.wDay,
                                stTimeGenerated.wHour, stTimeGenerated.wMinute, stTimeGenerated.wSecond );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Event type: %u" CRLF, wEventType );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Event category: %u" CRLF, wEventCategory );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Event ID: %u" CRLF, ( WORD )dwEventId );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Raw data size: %lu" CRLF, cbRawData );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    sprintf_s( szBuffer,
                                ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                "Number of strings: %u" CRLF, cStrings );
                    IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                    for ( WORD i = 0 ; i < cStrings ; i++ ){
                        sprintf_s( szBuffer,
                                    ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                    "\tString[ %u ]: %S" CRLF, i + 1, rgpwszStrings[ i ] );
                        IEventLoggingPrintToFile( hQuery->hFile, szBuffer );                    
                    }
                    hModule = EventLoggingModuleFromEventSource( wszEventSource );
                    if ( hModule ){
                        PWSTR wszFormattedMsg = EventLoggingFormatMessage( hModule,
                                                                            dwEventId,
                                                                            GetSystemDefaultLCID(),
                                                                            rgpwszStrings );
                        if ( wszFormattedMsg ){
                            sprintf_s( szBuffer,
                                        ESETEST_ELHELPER_STRING_BUFFER_SIZE,
                                        "Formatted message: %S" CRLF, wszFormattedMsg );
                            IEventLoggingPrintToFile( hQuery->hFile, szBuffer );
                            if ( NULL != LocalFree( wszFormattedMsg ) ){
                                OutputWarning( "%s(): LocalFree() failed" CRLF, __FUNCTION__ );
                            }
                        }
                    }
                    IEventLoggingPrintToFile( hQuery->hFile, CRLF );
                }

            NextEvent:
                delete []rgpwszStrings;
                rgpwszStrings = NULL;
                pBufferAux += pelg->Length;
            }           
        }
        else{
            dwLastError = GetLastError();           
            switch ( dwLastError ){
                case ERROR_INSUFFICIENT_BUFFER:
                    pBuffer2 = new BYTE[ cbNeed ];
                    if ( NULL == pBuffer2 ){
                        OutputError( "%s(): failed to allocate buffer for pBuffer2!" CRLF, __FUNCTION__ );
                        goto Cleanup;
                    }
                    delete []hQuery->pBuffer;
                    hQuery->pBuffer = pBuffer2;
                    hQuery->cbBufferSize = cbNeed;
                    break;

                case ERROR_HANDLE_EOF:
                    break;

                default:
                    OutputError( "%s(): ReadEventLogW() failed with %lu!" CRLF,
                                    __FUNCTION__, dwLastError );
                    goto Cleanup;
                    break;
            }
        }
    }

    fResult = TRUE;
    
Cleanup:
    delete []rgpwszStrings;
    return fResult;
}

VOID __cdecl
IEventLoggingBackgroundListening(
    __in_opt PVOID  hQuery
)
{
    EseEventLoggingQuery* elq = ( EseEventLoggingQuery* )hQuery;
    DWORD dwTime, dwDeltaTime, dwReturn;

    dwTime = GetTickCount();
    while ( 1 ){
        dwDeltaTime = GetTickCount() - dwTime;
        if ( !IEventLoggingProcessEvents( elq ) ){
            OutputError( "%s(): IEventLoggingProcessEvents() failed!" CRLF, __FUNCTION__ );
        }
        dwReturn = WaitForMultipleObjects( 2, elq->hObjects, FALSE,
                                            ( dwDeltaTime >= ESETEST_ELHELPER_MAXIMUM_WAIT_TIME ) ?
                                                0 :
                                                ( ESETEST_ELHELPER_MAXIMUM_WAIT_TIME - dwDeltaTime ) );
        dwTime = GetTickCount();
        // We'll process event if either we timed-out or we've been notified.
        // Ideally, we shouldn't need to check for timeouts, but NotifyChangeEventLog() will not call
        //  PulseEvent() twice in a five-second interval. So, if we process events and then new ones
        //  are generated, we'll miss them.
        if ( ( WAIT_TIMEOUT == dwReturn ) || ( ( dwReturn - WAIT_OBJECT_0 ) == 1 ) ){
            if ( !IEventLoggingProcessEvents( elq ) ){
                OutputError( "%s(): IEventLoggingProcessEvents() failed!" CRLF, __FUNCTION__ );
            }
        }
        else{
            break;
        }       
    }

    // Flag that the thread doesn't need to manipulate resources anymore.
    ResetEvent( elq->hObjects[ 0 ] );   
    _endthread();
}

HANDLE
EventLoggingCreateQuery(
    __in_opt PFNEVENTLOGGING                        pfnCallback,
    __in_opt PCWSTR                             wszEventLog,
    __in_ecount_opt( cEventSources ) PCWSTR*    pwszEventSources,
    _In_ size_t                                 cEventSources,
    __in_opt PSYSTEMTIME                            pTimeMin,
    __in_opt PSYSTEMTIME                            pTimeMax,
    __in_ecount_opt( cEventTypes ) PWORD        pEventTypes,
    _In_ size_t                                 cEventTypes,
    __in_ecount_opt( cEventCategories ) PWORD   pEventCategories,
    _In_ size_t                                 cEventCategories,
    __in_ecount_opt( cEventIds ) PDWORD         pEventIds,
    _In_ size_t                                 cEventIds,
    __in_opt PCWSTR                             wszLogFile,
    __in_opt PVOID                              pUserData
)
{
    BOOL fSuccess = FALSE;

    // First thing, we allocate the object.
    EseEventLoggingQuery* hQuery = new EseEventLoggingQuery;
    if ( NULL == hQuery ){
        OutputError( "%s(): failed to allocate new query object" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // Initialize the structure with default values, in case we bail out.
    hQuery->pfnCallback                 = pfnCallback;
    hQuery->hEventLog                   = NULL;
    hQuery->pwszEventSources            = NULL;
    hQuery->cEventSources               = 0;
    hQuery->dwTimeMin                   = 0;
    hQuery->dwTimeMax                   = ULONG_MAX;
    hQuery->pEventTypes                 = NULL;
    hQuery->cEventTypes                 = 0;
    hQuery->pEventCategories            = NULL;
    hQuery->cEventCategories            = 0;
    hQuery->pEventIds                   = NULL;
    hQuery->cEventIds                   = 0;
    hQuery->hFile                       = INVALID_HANDLE_VALUE;
    hQuery->pUserData                   = pUserData;
    hQuery->pBuffer                     = NULL;
    hQuery->cbBufferSize                = 0;
    hQuery->wszEventLog                 = NULL;
    hQuery->wszLogFile                  = NULL;
    hQuery->fThreadActive               = FALSE;


    JET_ERR err = g_pfnOpenEventLogW.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): OpenEventLogW() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    err = g_pfnNotifyChangeEventLog.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): NotifyChangeEventLog() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    // Initialize with real values.

    // Event log.
    if ( NULL == wszEventLog ){
        hQuery->wszEventLog                 = EsetestCopyWideString( __FUNCTION__, L"Application" );
    }
    else{
        hQuery->wszEventLog                 = EsetestCopyWideString( __FUNCTION__, wszEventLog );
    }
    if ( NULL == hQuery->wszEventLog ){
        OutputError( "%s(): failed to allocate hQuery->wszEventLog!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    hQuery->hEventLog = g_pfnOpenEventLogW( NULL, hQuery->wszEventLog );
    if ( NULL == hQuery->hEventLog ){
        OutputError( "%s(): OpenEventLogW() failed with %lu!" CRLF, __FUNCTION__, GetLastError() );
        goto Cleanup;
    }

    // Event sources filter.
    if ( NULL != pwszEventSources ){
        if ( cEventSources ){
            hQuery->pwszEventSources = new PWSTR[ cEventSources ];
            if ( NULL == hQuery->pwszEventSources ){
                OutputError( "%s(): failed to allocate hQuery->pwszEventSources!" CRLF, __FUNCTION__ );
                goto Cleanup;
            }
            for ( size_t i = 0 ; i < cEventSources ; i++ ){
                if ( NULL ==
                    ( hQuery->pwszEventSources[ i ] = EsetestCopyWideString( __FUNCTION__,
                                                                            pwszEventSources[ i ] ) ) ){
                    OutputError( "%s(): failed to allocate hQuery->pwszEventSources[ %lu ]!" CRLF, __FUNCTION__, i );
                    goto Cleanup;
                }
            }
            hQuery->cEventSources = cEventSources;
        }
    }

    // Time filters.
    if ( NULL != pTimeMin ){
        hQuery->dwTimeMin = SystemTimeToSecondsSince1970( pTimeMin );
    }
    SecondsSince1970ToSystemTime( hQuery->dwTimeMin, &hQuery->stTimeMin );
    if ( NULL != pTimeMax ){
        hQuery->dwTimeMax = SystemTimeToSecondsSince1970( pTimeMax );
    }
    SecondsSince1970ToSystemTime( hQuery->dwTimeMax, &hQuery->stTimeMax );

    // Event type filters.
    if ( NULL != pEventTypes ){
        if ( cEventTypes ){
            hQuery->pEventTypes = new WORD[ cEventTypes ];
            if ( NULL == hQuery->pEventTypes ){
                OutputError( "%s(): failed to allocate hQuery->pEventTypes!" CRLF, __FUNCTION__ );
                goto Cleanup;
            }
            memcpy( hQuery->pEventTypes, pEventTypes, sizeof( WORD ) * ( cEventTypes ) );
            hQuery->cEventTypes = cEventTypes;
        }
    }

    // Event category filters.
    if ( NULL != pEventCategories ){
        if ( cEventCategories ){
            hQuery->pEventCategories = new WORD[ cEventCategories ];
            if ( NULL == hQuery->pEventCategories ){
                OutputError( "%s(): failed to allocate hQuery->pEventCategories!" CRLF, __FUNCTION__ );
                goto Cleanup;
            }
            memcpy( hQuery->pEventCategories, pEventCategories, sizeof( WORD ) * ( cEventCategories ) );
            hQuery->cEventCategories = cEventCategories;
        }
    }

    // Event ID filters.
    if ( NULL != pEventIds ){
        if ( cEventIds ){
            hQuery->pEventIds = new DWORD[ cEventIds ];
            if ( NULL == hQuery->pEventIds ){
                OutputError( "%s(): failed to allocate hQuery->pEventIds!" CRLF, __FUNCTION__ );
                goto Cleanup;
            }
            memcpy( hQuery->pEventIds, pEventIds, sizeof( DWORD ) * ( cEventIds ) );
            hQuery->cEventIds = cEventIds;
        }
    }

    // File to log.
    if ( wszLogFile ){
        hQuery->wszLogFile = EsetestCopyWideString( __FUNCTION__, wszLogFile );
        if ( NULL == hQuery->wszLogFile ){
            OutputError( "%s(): failed to allocate hQuery->wszLogFile!" CRLF, __FUNCTION__ );
            goto Cleanup;
        }
        hQuery->hFile = CreateFileW( wszLogFile,
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL );
        if( INVALID_HANDLE_VALUE != hQuery->hFile ){
            if( INVALID_SET_FILE_POINTER == SetFilePointer( hQuery->hFile, 0, NULL, FILE_END ) ) {
                OutputWarning( "%s(): failed to seek to the end of the file!" CRLF, __FUNCTION__ );
                CloseHandleP( &hQuery->hFile );
                hQuery->hFile = INVALID_HANDLE_VALUE;
            }
        }
        else{
            OutputWarning( "%s(): failed to open file!" CRLF, __FUNCTION__ );
        }
    }

    // Buffer to receive the event log records.
    hQuery->pBuffer = new BYTE[ sizeof( EVENTLOGRECORD ) ];
    if ( NULL == hQuery->pBuffer ){
        OutputError( "%s(): failed to allocate hQuery->pBuffer!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    hQuery->cbBufferSize = sizeof( EVENTLOGRECORD );

    // Synchronization objects.
    hQuery->hObjects[ 0 ] = CreateEventW( NULL, TRUE, FALSE, NULL );
    hQuery->hObjects[ 1 ] = CreateEventW( NULL, TRUE, FALSE, NULL );
    if ( ( NULL == hQuery->hObjects[ 0 ] ) || ( NULL == hQuery->hObjects[ 1 ] ) ){
        OutputError( "%s(): CreateEventW() failed!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    else{
        // Notification event for new log records.
        if ( !g_pfnNotifyChangeEventLog( hQuery->hEventLog, hQuery->hObjects[ 1 ] ) ){
            OutputError( "%s(): NotifyChangeEventLog() failed!" CRLF, __FUNCTION__ );
            goto Cleanup;
        }
    }

    // At this point, everything is ready to go. We can even return some events.
    if ( !IEventLoggingProcessEvents( hQuery ) ){
        OutputError( "%s(): IEventLoggingProcessEvents() failed!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // Finally, create the background thread to listen for new events.
    if ( -1L == _beginthread( IEventLoggingBackgroundListening, 0, hQuery ) ){
        OutputError( "%s(): failed to start background listening thread!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    hQuery->fThreadActive = TRUE;

    // We got it.
    fSuccess = TRUE;

Cleanup:
    if ( !fSuccess && ( NULL != hQuery ) ){
        EventLoggingDestroyQuery( ( HANDLE )hQuery );
        hQuery = NULL;
    }
    return ( HANDLE )hQuery;
}

BOOL
EventLoggingDestroyQuery(
    _In_ HANDLE hQuery
)
{
    if ( !hQuery ){
        OutputError( "%s(): invalid query!" CRLF, __FUNCTION__ );
        return FALSE;
    }
    JET_ERR err = g_pfnCloseEventLog.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): CloseEventLog() is not present on this system: %d" CRLF, __FUNCTION__, err );
        return FALSE;
    }

    EseEventLoggingQuery* elq = ( EseEventLoggingQuery* )hQuery;

    // Tell background thread to stop.
    if ( elq->fThreadActive ){
        SetEvent( elq->hObjects[ 0 ] );
        while ( WAIT_TIMEOUT != WaitForSingleObject( elq->hObjects[ 0 ], 1 ) );
        elq->fThreadActive = FALSE;
    }

    // Free memory and resources.

    // Event log handle.
    if ( NULL != elq->hEventLog ){
        (void) g_pfnCloseEventLog( elq->hEventLog );
    }

    // Event sources filter.
    for ( size_t i = 0 ; i < elq->cEventSources ; i++ ){
        delete []elq->pwszEventSources[ i ];
    }
    delete []elq->pwszEventSources;

    // Event type filters.
    delete []elq->pEventTypes;

    // Event category filters.
    delete []elq->pEventCategories; 

    // Event ID filters.
    delete []elq->pEventIds;

    // File to log.
    CloseHandleP( &elq->hFile );

    // Buffer.
    delete []elq->pBuffer;

    // Synchronization objects.
    CloseHandleP( &elq->hObjects[ 0 ] );
    CloseHandleP( &elq->hObjects[ 1 ] );

    // Strings.
    delete []elq->wszEventLog;
    delete []elq->wszLogFile;

    // Object itself.
    delete elq;
    
    return TRUE;
}

PWSTR
EventLoggingFormatMessage(
    _In_ HMODULE        hModule,
    _In_ DWORD          dwEventId,
    _In_ LCID           dwLandIg,
    __in_opt PWSTR*     pwszStrings
)
{
    DWORD dwReturn;
    PWSTR wszFormattedMsg;
    DWORD fIgnoreInserts = 0;

    // Since we don't pass the size, we may AV if the string provided by the event generator
    //  is malformatted.
IfExcept:
    __try{
        dwReturn = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_ARGUMENT_ARRAY |
                                    FORMAT_MESSAGE_FROM_HMODULE |
                                    FORMAT_MESSAGE_FROM_SYSTEM | 
                                    fIgnoreInserts,
                                    hModule,
                                    dwEventId,
                                    dwLandIg,
                                    ( LPWSTR )&wszFormattedMsg,
                                    0,
                                    ( va_list* )pwszStrings );
        if ( dwReturn ){
            return wszFormattedMsg;
        }
        else{
            OutputError( "%s(): FormatMessageW() failed with %lu" CRLF,
                            __FUNCTION__, GetLastError() );
            return NULL;
        }
    }
     __except ( ( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION )  ? 
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ){
        if ( !fIgnoreInserts ){
            fIgnoreInserts = FORMAT_MESSAGE_IGNORE_INSERTS;
            goto IfExcept; 
        }
        else{
            return NULL;
        }
    }
}

HMODULE
EventLoggingModuleFromEventSource(
    _In_ PCWSTR     wszEventSource
)
{
    // Try to match with one of the supported modules.
    if ( !wcscmp( WszEsetestEseEventSource(), wszEventSource ) ){
        return HmodEsetestEseDll();
    }
    else if ( !wcscmp( WszEsetestEsebackEventSource(), wszEventSource ) ){
        return HmodEsetestEsebackDll();
    }
    return NULL;    
}

VOID
EventLoggingPrintEvent(
    _In_ PCWSTR                         wszEventLog,
    _In_ PCWSTR                         wszEventSource,
    _In_ PSYSTEMTIME                    pTimeGenerated,
    _In_ WORD                           wEventType,
    _In_ WORD                           wEventCategory,
    _In_ DWORD                          dwEventId,
    _In_ DWORD                          dwLangId,
    __in_ecount_opt( cStrings ) PWSTR*  pwszStrings,
    _In_ WORD                           cStrings,
    __in_bcount_opt( cbRawData ) PVOID  pRawData,
    _In_ DWORD                          cbRawData
)
{
    HMODULE hModule;
    PWSTR wszFormattedMsg;
        
    tprintf( CRLF );
    tprintf( "Event log: %S" CRLF, wszEventLog );   
    tprintf( "Event source: %S" CRLF, wszEventSource );
    tprintf( "Time generated: %04u/%02u/%02u %02u:%02u:%02u" CRLF,
                pTimeGenerated->wYear, pTimeGenerated->wMonth, pTimeGenerated->wDay,
                pTimeGenerated->wHour, pTimeGenerated->wMinute, pTimeGenerated->wSecond );
    tprintf( "Event type: %u" CRLF, wEventType );
    tprintf( "Event category: %u" CRLF, wEventCategory );
    tprintf( "Event ID: %u" CRLF, ( WORD )dwEventId );
    tprintf( "Raw data size: %lu" CRLF, cbRawData );
    tprintf( "Number of strings: %u" CRLF, cStrings );
    for ( WORD i = 0 ; i < cStrings ; i++ ){
        tprintf( "\tString[ %u ]: %S" CRLF, i + 1, pwszStrings[ i ] );
    }
    hModule = EventLoggingModuleFromEventSource( wszEventSource );
    if ( hModule ){
        wszFormattedMsg = EventLoggingFormatMessage( hModule, dwEventId, dwLangId, pwszStrings );
        if ( wszFormattedMsg ){
            tprintf( "Formatted message: %S" CRLF, wszFormattedMsg );
            if ( NULL != LocalFree( wszFormattedMsg ) ){
                OutputWarning( "%s(): LocalFree() failed" CRLF, __FUNCTION__ );
            }
        }
    }
    tprintf( CRLF );
}

#else
//Windows Phone
HANDLE
EventLoggingCreateQuery(
    __in_opt PFNEVENTLOGGING                        pfnCallback,
    __in_opt PCWSTR                             wszEventLog,
    __in_ecount_opt( cEventSources ) PCWSTR*    pwszEventSources,
    _In_ size_t                                 cEventSources,
    __in_opt PSYSTEMTIME                            pTimeMin,
    __in_opt PSYSTEMTIME                            pTimeMax,
    __in_ecount_opt( cEventTypes ) PWORD        pEventTypes,
    _In_ size_t                                 cEventTypes,
    __in_ecount_opt( cEventCategories ) PWORD   pEventCategories,
    _In_ size_t                                 cEventCategories,
    __in_ecount_opt( cEventIds ) PDWORD         pEventIds,
    _In_ size_t                                 cEventIds,
    __in_opt PCWSTR                             wszLogFile,
    __in_opt PVOID                              pUserData
)
{
    return NULL;
}

BOOL
EventLoggingDestroyQuery(
    _In_ HANDLE hQuery
)
{
    return FALSE;
}

PWSTR
EventLoggingFormatMessage(
    _In_ HMODULE        hModule,
    _In_ DWORD          dwEventId,
    _In_ LCID           dwLandIg,
    __in_opt PWSTR*     pwszStrings
)
{
    return NULL;
}

HMODULE
EventLoggingModuleFromEventSource(
    _In_ PCWSTR     wszEventSource
)
{   
    return NULL;    
}

VOID
EventLoggingPrintEvent(
    _In_ PCWSTR                         wszEventLog,
    _In_ PCWSTR                         wszEventSource,
    _In_ PSYSTEMTIME                    pTimeGenerated,
    _In_ WORD                           wEventType,
    _In_ WORD                           wEventCategory,
    _In_ DWORD                          dwEventId,
    _In_ DWORD                          dwLangId,
    __in_ecount_opt( cStrings ) PWSTR*  pwszStrings,
    _In_ WORD                           cStrings,
    __in_bcount_opt( cbRawData ) PVOID  pRawData,
    _In_ DWORD                          cbRawData
)
{   
}

#endif
