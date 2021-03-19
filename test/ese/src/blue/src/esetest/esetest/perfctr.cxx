// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//================================================================
// Performance counters helper functions.
//================================================================
//

#include <process.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <loadperf.h>
#include <float.h>
#include <tchar.h>
#include <stdlib.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include "ese_common.hxx"
#include "eseloadlibrary.hxx"

#define wszPerfDataHelper               L"pdh.dll"
const wchar_t * const g_mwszzPerfDataHelperBrokenCoreSystem         = wszPerfDataHelper L"\0";

#define ESETEST_PCHELPER_INIT_COUNTERS      2
#define ESETEST_PCHELPER_MAX_COUNTERS       1000

static const WCHAR wszEsentPerfCountersPath[]   =   L"SYSTEM\\CurrentControlSet\\Services\\" WESE_FLAVOUR L"\\Performance";

typedef struct{
    CRITICAL_SECTION*           pcs;
    PDH_HQUERY                  hQuery;
    size_t                      cCounters;
    size_t                      cCountersAllocated;
    PDH_HCOUNTER*               phCounters;
    BOOL                        fCollecting;
    double*                     pMostRecent;
    double*                     pDataMins;
    double*                     pTimeMins;
    double*                     pDataMaxs;
    double*                     pTimeMaxs;
    double*                     pDataAvgs;
    double*                     pDataVars;
    double*                     pBuffer;
    ULONG64                     cCollected;
    DWORD                       dwPeriod;
    JET_GRBIT*                  pgrbitStats;
    PWSTR                       wszLogFile;
    HANDLE                      hStopThread;
} EsePerfCounterQuery;

#ifndef BUILD_ENV_IS_WPHONE

// Global 'functors' to help with the loadlibrary's.

NTOSFuncStd( pfnPdhOpenQueryW, g_mwszzPerfDataHelperBrokenCoreSystem, PdhOpenQueryW, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhCloseQuery, g_mwszzPerfDataHelperBrokenCoreSystem, PdhCloseQuery, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhMakeCounterPathW, g_mwszzPerfDataHelperBrokenCoreSystem, PdhMakeCounterPathW, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhRemoveCounter, g_mwszzPerfDataHelperBrokenCoreSystem, PdhRemoveCounter, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhCollectQueryData, g_mwszzPerfDataHelperBrokenCoreSystem, PdhCollectQueryData, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhGetFormattedCounterValue, g_mwszzPerfDataHelperBrokenCoreSystem, PdhGetFormattedCounterValue, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhGetRawCounterValue, g_mwszzPerfDataHelperBrokenCoreSystem, PdhGetRawCounterValue, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );

// PdhAddEnglishCounterW actually does language-neutral counters.
NTOSFuncStd( pfnPdhAddCounterW, g_mwszzPerfDataHelperBrokenCoreSystem, PdhAddCounterW, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );
NTOSFuncStd( pfnPdhAddEnglishCounterW, g_mwszzPerfDataHelperBrokenCoreSystem, PdhAddEnglishCounterW, oslfExpectedOnWin5x | oslfNotExpectedOnCoreSystem );

// Prototypes

BOOL
PerfCountersExist()
{
    BOOL fSuccess;
    HANDLE hQuery = PerfCountersCreateQuery();
    fSuccess = PerfCountersAddLnCounterA( hQuery,
                                            NULL,
                                            SzEsetestEseperfDatabase(),
                                            "Database Cache Size",
                                            "esetest" );
    PerfCountersDestroyQuery( hQuery );
    return fSuccess;
}

BOOL
PerfCountersInstallA(
    __in_opt PCSTR  szLibraryPath,
    __in_opt PCSTR  szIniFilePath
)
{
    WCHAR* wszLibraryPath       = EsetestWidenString( __FUNCTION__, szLibraryPath );
    WCHAR* wszIniFilePath       = EsetestWidenString( __FUNCTION__, szIniFilePath );
    BOOL fReturn                = PerfCountersInstallW( wszLibraryPath,
                                                        wszIniFilePath );
    delete []wszLibraryPath;
    delete []wszIniFilePath;

    return fReturn;
}

BOOL
PerfCountersInstallW(
    __in_opt PCWSTR wszLibraryPath,
    __in_opt PCWSTR wszIniFilePath
)
{
    LONG lError;
    DWORD dwError;
    DWORD dwDisposition;
    HKEY hKey = NULL;
    DWORD dwBuffer;
    size_t cbCount;
    WCHAR wszILibraryPath[ MAX_PATH ];
    WCHAR wszLibraryFile[ MAX_PATH ];
    WCHAR wszIniFile[ MAX_PATH ];
    BOOL fReturn = FALSE;

    // Try to open/create the key.
    if( ERROR_SUCCESS != ( lError = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                                                        wszEsentPerfCountersPath,
                                                        0,
                                                        NULL,
                                                        REG_OPTION_NON_VOLATILE,
                                                        KEY_WRITE,
                                                        NULL,                                                   
                                                        &hKey,
                                                        &dwDisposition ) ) ){
        OutputError( "%s(): RegCreateKeyExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        hKey = NULL;
        goto Cleanup;
    }

    // Defaults and path manipulation.
    if ( NULL == wszLibraryPath ||  L'\0' == wszLibraryPath[ 0 ] ){
        GetCurrentDirectoryW( MAX_PATH, wszILibraryPath );
    }
    else{
        StringCchPrintfW( wszILibraryPath, MAX_PATH, L"%s", wszLibraryPath );
    }
    StringCchPrintfW( wszLibraryFile, MAX_PATH, L"%s\\%s", wszILibraryPath, WszEsetestEseperfDll() );
    if ( NULL == wszIniFilePath ||  L'\0' == wszIniFilePath[ 0 ] ){
        StringCchPrintfW( wszIniFile, MAX_PATH, L"lame \"%s\\%s\"", wszILibraryPath, WszEsetestEseperfIni() );
    }
    else{
        StringCchPrintfW( wszIniFile, MAX_PATH, L"lame \"%s\\%s\"", wszIniFilePath, WszEsetestEseperfIni() );
    }

    // Try to load the performance counters.
    if( ERROR_SUCCESS != ( dwError = LoadPerfCounterTextStringsW( wszIniFile,
                                                                    TRUE ) ) ){
        OutputError( "%s(): LoadPerfCounterTextStrings() failed with %lu!" CRLF, __FUNCTION__, dwError );
        goto Cleanup;
    }

    // We have to create these extra registry values.
    if( ERROR_SUCCESS != ( lError = RegSetValueExW( hKey,
                                                    g_wszRegKeyOpenPerfData,
                                                    0,
                                                    REG_SZ,
                                                    ( const BYTE* )( g_wszRegKeyOpenPerfDataVal ),
                                                    sizeof( g_wszRegKeyOpenPerfDataVal ) ) ) ){
        OutputError( "%s(): RegSetValueExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        goto Cleanup;
    }
    if( ERROR_SUCCESS != ( lError = RegSetValueExW( hKey,
                                                    g_wszRegKeyCollectPerfData,
                                                    0,
                                                    REG_SZ,
                                                    ( const BYTE* )g_wszRegKeyCollectPerfDataVal,
                                                    sizeof( g_wszRegKeyCollectPerfDataVal ) ) ) ){
        OutputError( "%s(): RegSetValueExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        goto Cleanup;
    }
    if( ERROR_SUCCESS != ( lError = RegSetValueExW( hKey,
                                                    g_wszRegKeyClosePerfData,
                                                    0,
                                                    REG_SZ,
                                                    ( const BYTE* )g_wszRegKeyClosePerfDataVal,
                                                    sizeof( g_wszRegKeyClosePerfDataVal ) ) ) ){
        OutputError( "%s(): RegSetValueExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        goto Cleanup;
    }
    dwBuffer = 1;
    if( ERROR_SUCCESS != ( lError = RegSetValueExW( hKey,
                                                    g_wszRegKeyAdvancedPerfCtrs,
                                                    0,
                                                    REG_DWORD,
                                                    ( const BYTE* )&dwBuffer,
                                                    sizeof( dwBuffer ) ) ) ){
        OutputError( "%s(): RegSetValueExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        goto Cleanup;
    }
    StringCbLengthW( wszLibraryFile, MAX_PATH * sizeof( WCHAR ), ( size_t* )&cbCount );
    if( ERROR_SUCCESS != ( lError = RegSetValueExW( hKey,
                                                    g_wszRegKeyPerfLibrary,
                                                    0,
                                                    REG_EXPAND_SZ,
                                                    ( const BYTE* )wszLibraryFile,
                                                    ( DWORD )( cbCount + sizeof( WCHAR ) ) ) ) ){
        OutputError( "%s(): RegSetValueExW() failed with %ld!" CRLF, __FUNCTION__, lError );
        goto Cleanup;
    }

    fReturn = TRUE;
    
Cleanup:
    if ( NULL != hKey ) RegCloseKey( hKey );
    
    return fReturn;
}

BOOL
PerfCountersUninstall()
{
    DWORD dwError;
    WCHAR wszCmdLine[ MAX_PATH ];
    BOOL fReturn = FALSE;

    // Try to unload the performance counters.
    StringCchPrintfW( wszCmdLine, MAX_PATH, L"lame \"%s\"", WESE_FLAVOUR );
    dwError = UnloadPerfCounterTextStringsW( wszCmdLine, TRUE );    
    if( ERROR_SUCCESS != dwError && ERROR_BADKEY != dwError ){      
        OutputError( "%s(): UnloadPerfCounterTextStringsW() failed with %lu!" CRLF, __FUNCTION__, dwError );
        goto Cleanup;
    }

    fReturn = TRUE;
    
Cleanup:
    return fReturn;
}

HANDLE
PerfCountersCreateQuery()
{
    EsePerfCounterQuery* pcqReturn;
    BOOL fSuccess = FALSE;
    PDH_STATUS pdhStatus;
    pcqReturn = nullptr;
    
    JET_ERR err = pfnPdhOpenQueryW.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhOpenQueryW() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    err = pfnPdhCloseQuery.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhCloseQuery() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    // Allocate new query object.
    pcqReturn = new EsePerfCounterQuery;
    if ( !pcqReturn ){
        OutputError( "%s(): failed to allocate new query object!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // Initialize memory and resources.
    pcqReturn->pcs                  = NULL;
    pcqReturn->hQuery               = INVALID_HANDLE_VALUE;
    pcqReturn->cCounters            = 0;
    pcqReturn->cCountersAllocated   = 0;
    pcqReturn->phCounters           = NULL;
    pcqReturn->fCollecting          = FALSE;
    pcqReturn->pMostRecent          = NULL;
    pcqReturn->pDataMins            = NULL;
    pcqReturn->pTimeMins            = NULL;
    pcqReturn->pDataMaxs            = NULL;
    pcqReturn->pTimeMaxs            = NULL;
    pcqReturn->pDataAvgs            = NULL;
    pcqReturn->pDataVars            = NULL;
    pcqReturn->pBuffer              = NULL;
    pcqReturn->cCollected           = 0;
    pcqReturn->dwPeriod             = 0;
    pcqReturn->pgrbitStats          = NULL;
    pcqReturn->wszLogFile           = NULL;
    pcqReturn->hStopThread          = NULL;

    if ( ( pcqReturn->pcs = new CRITICAL_SECTION ) == NULL ){
        OutputError( "%s(): failed to allocate new critical section!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    InitializeCriticalSection( pcqReturn->pcs );
    if ( ERROR_SUCCESS != ( pdhStatus = pfnPdhOpenQueryW( NULL, 0, &pcqReturn->hQuery ) ) ){
        OutputError( "%s(): PdhOpenQueryW() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
        goto Cleanup;
    }
    if ( ( pcqReturn->phCounters = new PDH_HCOUNTER[ ESETEST_PCHELPER_INIT_COUNTERS ] ) == NULL ){
        OutputError( "%s(): failed to allocate new PDH_COUNTER array!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    pcqReturn->cCountersAllocated = ESETEST_PCHELPER_INIT_COUNTERS;
    fSuccess = TRUE;

Cleanup:
    if ( !fSuccess && pcqReturn ){
        if ( pcqReturn->pcs ) DeleteCriticalSection( pcqReturn->pcs );
        delete pcqReturn->pcs;
        if ( pcqReturn->hQuery != INVALID_HANDLE_VALUE ) pfnPdhCloseQuery( pcqReturn->hQuery );
        delete []pcqReturn->phCounters;
        delete pcqReturn;       
        pcqReturn = NULL;
    }
    return ( HANDLE )pcqReturn;
}

BOOL
IPerfCountersAddCounter(
    _In_ EsePerfCounterQuery*   pcq,
    _In_ PCWSTR                 wszComputerName,
    _In_ PCWSTR                 wszPerfObject,
    _In_ PCWSTR                 wszPerfCounter,
    _In_ PCWSTR                 wszInstance,
    _In_ BOOL                   fLangNeutral
)
{
    if ( !pcq || pcq->fCollecting ){
        OutputError( "%s(): invalid query or query collection under way!" CRLF, __FUNCTION__ );
        return FALSE;
    }
    const size_t cchCounterPath = _MAX_PATH;
    WCHAR wszCounterPath[ cchCounterPath ];
    DWORD cchusedCounterPath = cchCounterPath;
    PDH_HCOUNTER hCounter = INVALID_HANDLE_VALUE;
    BOOL fSuccess = FALSE;
    PDH_STATUS pdhStatus;

    JET_ERR err = pfnPdhMakeCounterPathW.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhMakeCounterPathW() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    err = pfnPdhAddCounterW.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhAddCounterW() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    if ( fLangNeutral )
    {
        err = pfnPdhAddEnglishCounterW.ErrIsPresent();
        if ( err < 0 )
        {
            OutputWarning( "%s(): PdhAddEnglishCounterW() is not present on this system: %d" CRLF, __FUNCTION__, err );
            goto Cleanup;
        }
    }

    err = pfnPdhRemoveCounter.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhRemoveCounter() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    // Prepare struct to add counter.
    PDH_COUNTER_PATH_ELEMENTS_W pcpeCounterElements;
    pcpeCounterElements.szMachineName       = const_cast< WCHAR* >( wszComputerName );
    pcpeCounterElements.szObjectName        = const_cast< WCHAR* >( wszPerfObject );
    pcpeCounterElements.szInstanceName      = const_cast< WCHAR* >( wszInstance );
    pcpeCounterElements.szParentInstance    = NULL;
    pcpeCounterElements.dwInstanceIndex = ( DWORD )-1;
    pcpeCounterElements.szCounterName       = const_cast< WCHAR* >( wszPerfCounter );
    if ( ERROR_SUCCESS != ( pdhStatus = pfnPdhMakeCounterPathW( &pcpeCounterElements, wszCounterPath, &cchusedCounterPath,  0 ) ) ){
        OutputError( "%s(): PdhMakeCounterPathW() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
        goto Cleanup;
    }

    if ( fLangNeutral )
    {
        pdhStatus = pfnPdhAddEnglishCounterW( pcq->hQuery, wszCounterPath, 0, &hCounter );
    }
    else
    {
        pdhStatus = pfnPdhAddCounterW( pcq->hQuery, wszCounterPath, 0, &hCounter );
    }
    if( ERROR_SUCCESS != pdhStatus ){
        OutputError( "%s(): pfnPdhAddCounterW() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
        goto Cleanup;
    }

    // The counter is already added to the low level query. Let's put it in our query now.
    size_t cCounters = pcq->cCounters;
    size_t cCountersAllocated = pcq->cCountersAllocated;
    if( ++cCounters > ESETEST_PCHELPER_MAX_COUNTERS ){
        OutputError( "%s(): maximum number of counters exceeded!" CRLF, __FUNCTION__ );
        goto Cleanup;   
    }
    if ( cCounters > cCountersAllocated ){
        // We need more memory. Let's allocate 30% + 1 more.
        cCountersAllocated = ( ( 13 * cCounters ) / 10 + 1 );
        PDH_HCOUNTER* pNewCounters = new PDH_HCOUNTER[ cCountersAllocated ];
        if ( NULL == pNewCounters ){
            OutputError( "%s(): failed to allocate memory for new counters!" CRLF, __FUNCTION__ );
            goto Cleanup;
        }
        memcpy( pNewCounters, pcq->phCounters, pcq->cCounters * sizeof( *pNewCounters ) );
        delete []pcq->phCounters;
        pcq->phCounters = pNewCounters;
    }

    // We already have room for the new counter.
    pcq->phCounters[ cCounters - 1 ] = hCounter;
    pcq->cCounters = cCounters;
    pcq->cCountersAllocated = cCountersAllocated;
    fSuccess = TRUE;
Cleanup:
    if ( !fSuccess && ( hCounter != INVALID_HANDLE_VALUE ) ) pfnPdhRemoveCounter( hCounter );
    return fSuccess;
}

BOOL
PerfCountersAddLnCounterA(
    _In_ HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    _In_ PCSTR      szPerfObject,
    _In_ PCSTR      szPerfCounter,
    _In_ PCSTR      szInstance
)
{
    WCHAR* wszComputerName      = EsetestWidenString( __FUNCTION__, szComputerName );
    WCHAR* wszPerfObject        = EsetestWidenString( __FUNCTION__, szPerfObject );
    WCHAR* wszPerfCounter       = EsetestWidenString( __FUNCTION__, szPerfCounter );
    WCHAR* wszInstance          = EsetestWidenString( __FUNCTION__, szInstance );
    BOOL fReturn                = PerfCountersAddLnCounterW( hQuery,
                                                                wszComputerName,
                                                                wszPerfObject,
                                                                wszPerfCounter,
                                                                wszInstance );
    delete []wszComputerName;
    delete []wszPerfObject;
    delete []wszPerfCounter;
    delete []wszInstance;

    return fReturn;
}

BOOL
PerfCountersAddLnCounterW(
    _In_ HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    _In_ PCWSTR     wszPerfObject,
    _In_ PCWSTR     wszPerfCounter,
    _In_ PCWSTR     wszInstance
)
{
    return IPerfCountersAddCounter( ( EsePerfCounterQuery* )hQuery,
                                    wszComputerName,
                                    wszPerfObject,
                                    wszPerfCounter,
                                    wszInstance,
                                    TRUE );
}

BOOL
PerfCountersAddCounterA(
    _In_ HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    _In_ PCSTR      szPerfObject,
    _In_ PCSTR      szPerfCounter,
    _In_ PCSTR      szInstance
)
{
    WCHAR* wszComputerName      = EsetestWidenString( __FUNCTION__, szComputerName );
    WCHAR* wszPerfObject        = EsetestWidenString( __FUNCTION__, szPerfObject );
    WCHAR* wszPerfCounter       = EsetestWidenString( __FUNCTION__, szPerfCounter );
    WCHAR* wszInstance          = EsetestWidenString( __FUNCTION__, szInstance );
    BOOL fReturn                = PerfCountersAddCounterW( hQuery,
                                                            wszComputerName,
                                                            wszPerfObject,
                                                            wszPerfCounter,
                                                            wszInstance );
    delete []wszComputerName;
    delete []wszPerfObject;
    delete []wszPerfCounter;
    delete []wszInstance;

    return fReturn;
}

BOOL
PerfCountersAddCounterW(
    _In_ HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    _In_ PCWSTR     wszPerfObject,
    _In_ PCWSTR     wszPerfCounter,
    _In_ PCWSTR     wszInstance
)
{
    return IPerfCountersAddCounter( ( EsePerfCounterQuery* )hQuery,
                                    wszComputerName,
                                    wszPerfObject,
                                    wszPerfCounter,
                                    wszInstance,
                                    FALSE );
}

BOOL
IPerfCountersGetCounterValues(
    _In_ EsePerfCounterQuery*   pcq,
    __out_opt double*           pData,
    _In_ BOOL                   fRetry
)
{
    BOOL fSuccess                           = FALSE;
    BOOL fDoRetry                           = FALSE;
    PDH_FMT_COUNTERVALUE pdhFormatted;
    PDH_STATUS pdhStatus;

    JET_ERR err = pfnPdhCollectQueryData.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhCollectQueryData() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    err = pfnPdhGetFormattedCounterValue.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhGetFormattedCounterValue() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    // Perform the actual collection.
    if( ERROR_SUCCESS != ( pdhStatus = pfnPdhCollectQueryData( pcq->hQuery ) ) ){
        OutputError( "%s(): PdhCollectQueryData() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
        goto Cleanup;
    }

    // Go through all the counters, formatting their values.
    fSuccess = TRUE;
    for ( size_t i = 0; i < pcq->cCounters ; i++ ){
        if( ERROR_SUCCESS == ( pdhStatus = pfnPdhGetFormattedCounterValue( pcq->phCounters[ i ],
                                                                         PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                                                         NULL,
                                                                        &pdhFormatted ) ) ){
            if ( pData ) pData[ i ] = pdhFormatted.doubleValue;
        }
        else{
            if ( !fRetry ){
                fDoRetry = TRUE;
            }
            else{
                OutputWarning( "%s(): PdhGetFormattedCounterValue() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
                fSuccess = FALSE;
            }
        }       
    }

    // It's ok to return this when it's the first time we are retrieving
    // a rate-type counter.
    if ( fDoRetry ){
        Sleep( pcq->dwPeriod );
        return IPerfCountersGetCounterValues( pcq, pData, TRUE );
    }

Cleanup:
    return fSuccess;
}

BOOL
IPerfCountersGetCounterValuesRaw(
    _In_ EsePerfCounterQuery*   pcq,
    __out_opt ULONG64*          pData1,
    __out_opt ULONG64*          pData2
)
{
    BOOL fSuccess                           = FALSE;
    DWORD dwType;
    PDH_RAW_COUNTER pdhRaw;
    PDH_STATUS pdhStatus;

    JET_ERR err = pfnPdhCollectQueryData.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhCollectQueryData() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }

    err = pfnPdhGetRawCounterValue.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhGetRawCounterValue() is not present on this system: %d" CRLF, __FUNCTION__, err );
        goto Cleanup;
    }


    // Perform the actual collection.
    if( ERROR_SUCCESS != ( pdhStatus = pfnPdhCollectQueryData( pcq->hQuery ) ) ){
        OutputError( "%s(): PdhCollectQueryData() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
        goto Cleanup;
    }

    // Go through all the counters, formatting their values.
    fSuccess = TRUE;
    for ( size_t i = 0; i < pcq->cCounters ; i++ ){
        if( ERROR_SUCCESS == ( pdhStatus = pfnPdhGetRawCounterValue( pcq->phCounters[ i ],
                                                                    &dwType,
                                                                    &pdhRaw ) ) ){      
            if ( PDH_CSTATUS_VALID_DATA != pdhRaw.CStatus &&
                    PDH_CSTATUS_NEW_DATA != pdhRaw.CStatus ){
                OutputWarning( "%s(): PdhGetRawCounterValue() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
                fSuccess = FALSE;
            }
            else{
                if ( pData1 ) pData1[ i ] = ( ULONG64 )pdhRaw.FirstValue;
                if ( pData2 ) pData2[ i ] = ( ULONG64 )pdhRaw.SecondValue;
            }
        }
        else{
            OutputWarning( "%s(): PdhGetRawCounterValue() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
            fSuccess = FALSE;
        }
    }

Cleanup:
    return fSuccess;
}

BOOL
PerfCountersGetCounterValues(
    _In_ HANDLE         hQuery,
    __out_opt double*   pData
)
{
    BOOL fSuccess       = FALSE;

    if ( !hQuery ){
        OutputError( "%s(): invalid query!" CRLF, __FUNCTION__ );
        return FALSE;
    }

    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;

    // Acquire critical section.
    EnterCriticalSection( pcq->pcs );
    if ( pcq->fCollecting ){
        OutputError( "%s(): background data collection already under way!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // We are OK to collect new data now.
    fSuccess = IPerfCountersGetCounterValues( pcq, pData, FALSE );
    
Cleanup:
    LeaveCriticalSection( pcq->pcs );
    return fSuccess;
}

BOOL
PerfCountersGetCounterValuesRaw(
    _In_ HANDLE         hQuery,
    __out_opt ULONG64*  pData1,
    __out_opt ULONG64*  pData2
)
{
    BOOL fSuccess       = FALSE;

    if ( !hQuery ){
        OutputError( "%s(): invalid query!" CRLF, __FUNCTION__ );
        return FALSE;
    }

    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;

    // Acquire critical section.
    EnterCriticalSection( pcq->pcs );
    if ( pcq->fCollecting ){
        OutputError( "%s(): background data collection already under way!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // We are OK to collect new data now.
    fSuccess = IPerfCountersGetCounterValuesRaw( pcq, pData1, pData2 );
    
Cleanup:
    LeaveCriticalSection( pcq->pcs );
    return fSuccess;
}

VOID __cdecl
IPerfCountersBackgroundCollection(
    __in_opt PVOID  hQuery
)
{
    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;
    DWORD dwInitialTime;
    DWORD dwCurrentTime, dwCurrentTime2;
    ULONG64 qwTime, qwTimePrev;
    double dTime;
    double dSample, dCoeff1, dCoeff2;

    // Should we log the results?
    HANDLE hFile = INVALID_HANDLE_VALUE;
    if ( pcq->wszLogFile ){     
        hFile = CreateFileW( pcq->wszLogFile,
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
        if( INVALID_HANDLE_VALUE != hFile ){
            if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_END ) ) {
                OutputWarning( "%s(): failed to seek to the end of the file!" CRLF, __FUNCTION__ );
                CloseHandleP( &hFile );
                hFile = INVALID_HANDLE_VALUE;
            }
        }
        else{
            OutputWarning( "%s(): failed to open file!" CRLF, __FUNCTION__ );
        }
    }

    // Flag that the thread has been started.
    SetEvent( pcq->hStopThread );
    dwInitialTime = dwCurrentTime = dwCurrentTime2 = GetTickCount();
    dTime = 0.0;
    qwTime = qwTimePrev = 0;
    goto Collect;

    do{
        // If the sampling period has already expired, do not go to sleep.
        dwCurrentTime2 = GetTickCount();
        if ( ( dwCurrentTime2 - dwCurrentTime ) < pcq->dwPeriod ){
            Sleep( pcq->dwPeriod - ( dwCurrentTime2 - dwCurrentTime ) );
        }
        dwCurrentTime = GetTickCount();
Collect:

        // Avoid time overflow.
        qwTime += ( ( ULONG64 )( dwCurrentTime - dwInitialTime ) - ( qwTime & 0xFFFFFFFFLL ) );
        dTime = ( ( double )( qwTime ) ) / 1000.0;
        dCoeff1 = qwTime ? ( ( (  double )qwTimePrev ) / ( ( double )qwTime ) ) : 0;
        dCoeff2 = qwTime ? ( ( ( double )( qwTime - qwTimePrev ) ) / ( ( double )qwTime ) ) : 1;

        // Actually collect the data.
        if ( IPerfCountersGetCounterValues( pcq, pcq->pBuffer, FALSE ) )
            {
            // Perform calculations.
            EnterCriticalSection( pcq->pcs );       
            DoubleArrayCopy( pcq->pMostRecent, pcq->pBuffer, pcq->cCounters );
            if ( pcq->cCollected < ( ( ULONG64 ) -1 ) ){
                pcq->cCollected++;
            }
            for ( size_t i = 0; i < pcq->cCounters ; i++ ){
                dSample = pcq->pBuffer[ i ];
                // Min.
                if ( pcq->pgrbitStats[ i ] & ESETEST_bitPCHelperCollectMin ){
                    if ( dSample < pcq->pDataMins[ i ] ){
                        pcq->pDataMins[ i ] = dSample;
                        pcq->pTimeMins[ i ] = dTime;
                    }
                }
                // Max.
                if ( pcq->pgrbitStats[ i ] & ESETEST_bitPCHelperCollectMax ){
                    if (dSample > pcq->pDataMaxs[ i ] ){
                        pcq->pDataMaxs[ i ] = dSample;
                        pcq->pTimeMaxs[ i ] = dTime;
                    }
                }

                // Avg.
                if ( pcq->pgrbitStats[ i ] & ESETEST_bitPCHelperCollectAvg ){
                    // This is a floating-point version of the following discrete recursive formula:
                    // X[ i ] = ( ( i - 1 ) / i ) * X[ i - 1 ] + ( 1 / i ) * x[ i ], where X is the avg. and x is the sample.
                    pcq->pDataAvgs[ i ] = dCoeff1 * pcq->pDataAvgs[ i ] + dCoeff2 * dSample;

                    // Var.
                    if ( pcq->pgrbitStats[ i ] & ESETEST_bitPCHelperCollectVar ){
                        // This is a floating-point version of the following discrete recursive formula:
                        // V[ i ] = ( ( i - 1 ) / i ) * V[ i - 1 ] + ( 1 / i ) * ( x[ i ] - X[ i ] )^2, where X is the avg., x is the sample and V is the variance.
                        double dDeltaSqr = dSample - pcq->pDataAvgs[ i ];
                        dDeltaSqr *= dDeltaSqr;
                        pcq->pDataVars[ i ] = dCoeff1 * pcq->pDataVars[ i ] + dCoeff2 * dDeltaSqr;
                    }
                }
            }
            LeaveCriticalSection( pcq->pcs );
            }

        qwTimePrev = qwTime;

        // Write to the file.
        if ( INVALID_HANDLE_VALUE != hFile ){
            DWORD dw;
            CHAR szBuffer[ 32 ];
            sprintf_s( szBuffer, 32, "%#.6e\t", dTime );
            if ( !WriteFile( hFile, szBuffer, ( DWORD )strlen( szBuffer ), &dw, NULL ) ){
                OutputWarning( "%s(): failed to write to file!" CRLF, __FUNCTION__ );
            }
            for ( size_t i = 0; i < pcq->cCounters ; i++ ){
                sprintf_s( szBuffer, 32, "%#.6e\t", pcq->pBuffer[ i ] );
                if ( !WriteFile( hFile, szBuffer, ( DWORD )strlen( szBuffer ), &dw, NULL )){
                    OutputWarning( "%s(): failed to write to file!" CRLF, __FUNCTION__ );
                }
            }
            if ( !WriteFile( hFile, CRLF, ( DWORD )strlen( CRLF ), &dw, NULL ) ){
                OutputWarning( "%s(): failed to write to file!" CRLF, __FUNCTION__ );
            }
        }
    } while( WAIT_OBJECT_0 == WaitForSingleObject( pcq->hStopThread, 0 ) );

    // Close the file, if it was opened.
    CloseHandleP( &hFile );

    // Flag that the thread doesn't need to manipulate resources anymore.
    SetEvent( pcq->hStopThread );   
    _endthread();
}

BOOL
PerfCountersStartCollectingStatsA(
    _In_ HANDLE             hQuery,
    _In_ DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCSTR          szLogFile
)
{
    WCHAR* wszLogFile   = EsetestWidenString( __FUNCTION__, szLogFile );
    BOOL fReturn        = PerfCountersStartCollectingStatsW( hQuery, dwPeriod, pgrbitStats, wszLogFile );
    delete []wszLogFile;

    return fReturn;
}

BOOL
PerfCountersStartCollectingStatsW(
    _In_ HANDLE             hQuery,
    _In_ DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCWSTR         wszLogFile
)
{
    BOOL fSuccess       = FALSE;

    if ( !hQuery ){
        OutputError( "%s(): invalid query!" CRLF, __FUNCTION__ );
        return FALSE;
    }

    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;

    // Acquire critical section.
    EnterCriticalSection( pcq->pcs );
    if ( pcq->fCollecting ){
        OutputError( "%s(): background data collection already under way!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // We are OK to start background collection now.    
    pcq->pMostRecent = new double[ 8 * pcq->cCounters ];
    if ( NULL == pcq->pMostRecent ){
        OutputError( "%s(): failed to allocate memory for background collection (data)!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    pcq->pgrbitStats = new JET_GRBIT[ pcq->cCounters ];
    if ( NULL == pcq->pgrbitStats ){
        OutputError( "%s(): failed to allocate memory for background collection (grbit)!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    pcq->pDataMins = pcq->pMostRecent + pcq->cCounters;
    pcq->pTimeMins = pcq->pDataMins + pcq->cCounters;
    pcq->pDataMaxs = pcq->pTimeMins + pcq->cCounters;
    pcq->pTimeMaxs = pcq->pDataMaxs + pcq->cCounters;
    pcq->pDataAvgs = pcq->pTimeMaxs + pcq->cCounters;
    pcq->pDataVars = pcq->pDataAvgs + pcq->cCounters;
    pcq->pBuffer = pcq->pDataVars + pcq->cCounters;
    DoubleArraySet( pcq->pMostRecent, pcq->cCounters, 0.0 );
    DoubleArraySet( pcq->pDataMins, pcq->cCounters, DBL_MAX );
    DoubleArraySet( pcq->pTimeMins, pcq->cCounters, 0.0 );
    DoubleArraySet( pcq->pDataMaxs, pcq->cCounters, -DBL_MAX );
    DoubleArraySet( pcq->pTimeMaxs, pcq->cCounters, 0.0 );
    DoubleArraySet( pcq->pDataAvgs, pcq->cCounters, 0.0 );
    DoubleArraySet( pcq->pDataVars, pcq->cCounters, 0.0 );
    DoubleArraySet( pcq->pBuffer, pcq->cCounters, 0.0 );
    pcq->cCollected = 0;
    pcq->dwPeriod = dwPeriod;
    if ( pgrbitStats ){
        for ( size_t i = 0; i < pcq->cCounters ; i++ ){
            // We can't calculate the variance without the average.
            AssertM( ( pgrbitStats[ i ] & ( ESETEST_bitPCHelperCollectAvg | ESETEST_bitPCHelperCollectVar ) ) !=
                        ESETEST_bitPCHelperCollectVar );
            pcq->pgrbitStats[ i ] = pgrbitStats[ i ];
        }
    }
    else{
        memset( pcq->pgrbitStats, -1, pcq->cCounters * sizeof( *pcq->pgrbitStats ) );
    }

    // Should we log the results?
    size_t cStrLen;
    if ( wszLogFile && ( ( cStrLen = wcslen( wszLogFile ) ) != 0 ) ){
        pcq->wszLogFile = new WCHAR[ cStrLen + 1 ];
        if ( NULL == pcq->wszLogFile ){
            OutputError( "%s(): failed to allocate memory for log file name!" CRLF, __FUNCTION__ );
            goto Cleanup;
        }
        wcscpy( pcq->wszLogFile, wszLogFile );
    }

    // Start background thread to collect data.
    pcq->hStopThread = CreateEventW( NULL, TRUE, FALSE, NULL);
    if ( NULL == pcq->hStopThread ){
        OutputError( "%s(): failed to create event!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    if ( -1L == _beginthread( IPerfCountersBackgroundCollection, 0, hQuery ) ){
        OutputError( "%s(): failed to start background collection thread!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }
    WaitForSingleObject( pcq->hStopThread, INFINITE );
    pcq->fCollecting = TRUE;
    fSuccess = TRUE;

Cleanup:
    if ( !fSuccess ){
        delete []pcq->pMostRecent;
        pcq->pMostRecent = NULL;
        delete []pcq->wszLogFile;
        pcq->wszLogFile = NULL;
        delete []pcq->pgrbitStats;
        pcq->pgrbitStats = NULL;
        CloseHandleP( &pcq->hStopThread );
    }
    LeaveCriticalSection( pcq->pcs );
    return fSuccess;
}

BOOL
PerfCountersRetrieveStats(
    _In_ HANDLE         hQuery,
    __out_opt double*   pMostRecent,
    __out_opt double*   pDataMin,
    __out_opt double*   pTimeMin,
    __out_opt double*   pDataMax,
    __out_opt double*   pTimeMax,
    __out_opt double*   pDataAvg,
    __out_opt double*   pDataVar,
    __out_opt ULONG64*  cCollected
)
{
    BOOL fSuccess       = FALSE;

    if ( !hQuery ){
        OutputError( "%s(): invalid query!" CRLF, __FUNCTION__ );
        return FALSE;
    }

    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;

    // Acquire critical section.
    EnterCriticalSection( pcq->pcs );
    if ( !pcq->fCollecting ){
        OutputError( "%s(): cannot retrieve statistics if no background collection is under way!" CRLF, __FUNCTION__ );
        goto Cleanup;
    }

    // Just copy everything in case the pointers are valid.
    if ( pMostRecent ){
        memcpy( pMostRecent, pcq->pMostRecent, pcq->cCounters * sizeof( *pMostRecent ) );
    }
    if ( pDataMin ){
        memcpy( pDataMin, pcq->pDataMins, pcq->cCounters * sizeof( *pDataMin ) );
    }
    if ( pTimeMin ){
        memcpy( pTimeMin, pcq->pTimeMins, pcq->cCounters * sizeof( *pTimeMin ) );
    }
    if ( pDataMax ){
        memcpy( pDataMax, pcq->pDataMaxs, pcq->cCounters * sizeof( *pDataMax ) );
    }
    if ( pTimeMax ){
        memcpy( pTimeMax, pcq->pTimeMaxs, pcq->cCounters * sizeof( *pTimeMax ) );
    }
    if ( pDataAvg ){
        memcpy( pDataAvg, pcq->pDataAvgs, pcq->cCounters * sizeof( *pDataAvg ) );
    }
    if ( pDataVar ){
        memcpy( pDataVar, pcq->pDataVars, pcq->cCounters * sizeof( *pDataVar ) );
    }
    if ( cCollected ){
        *cCollected = pcq->cCollected;
    }
    fSuccess = TRUE;

Cleanup:
    LeaveCriticalSection( pcq->pcs );
    return fSuccess;
}

BOOL
PerfCountersStopCollectingStats(
    _In_ HANDLE hQuery
)
{
    BOOL fSuccess       = FALSE;

    if ( !hQuery ){
        OutputError( "%s(): invalid query!" CRLF, __FUNCTION__ );
        return FALSE;
    }

    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;

    // Acquire critical section.
    EnterCriticalSection( pcq->pcs );
    if ( !pcq->fCollecting ){
        goto Cleanup;
    }
    LeaveCriticalSection( pcq->pcs );

    // Tell background thread to stop and free memory and resources.
    ResetEvent( pcq->hStopThread );
    WaitForSingleObject( pcq->hStopThread, INFINITE );  
    EnterCriticalSection( pcq->pcs );
    pcq->fCollecting = FALSE;
    CloseHandleP( &pcq->hStopThread );
    delete []pcq->pMostRecent;
    pcq->pMostRecent = NULL;
    delete []pcq->wszLogFile;
    pcq->wszLogFile = NULL; 
    delete []pcq->pgrbitStats;
    pcq->pgrbitStats = NULL;
    fSuccess = TRUE;

Cleanup:
    LeaveCriticalSection( pcq->pcs );
    return fSuccess;
}

BOOL
PerfCountersDestroyQuery(
    _In_ HANDLE hQuery
)
{   
    if ( !hQuery ){
        return FALSE;
    }

    JET_ERR err = pfnPdhRemoveCounter.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhRemoveCounter() is not present on this system: %d" CRLF, __FUNCTION__, err );
        return FALSE;
    }

    err = pfnPdhCloseQuery.ErrIsPresent();
    if ( err < 0 )
    {
        OutputWarning( "%s(): PdhCloseQuery() is not present on this system: %d" CRLF, __FUNCTION__, err );
        return FALSE;
    }

    EsePerfCounterQuery* pcq = ( EsePerfCounterQuery* )hQuery;  
    if ( pcq->fCollecting ){
        if ( !PerfCountersStopCollectingStats( hQuery ) ){
            OutputWarning( "%s(): failed to stop collecting statistics!" CRLF, __FUNCTION__ );
        }
    }

    // Remove counters. Is this really necessary?
    if( pcq->hQuery != INVALID_HANDLE_VALUE ){
        for ( size_t i = 0; i < pcq->cCounters ; i++ ){
            if ( pcq->phCounters[ i ] != INVALID_HANDLE_VALUE ){
                pfnPdhRemoveCounter( pcq->phCounters[ i ] );
                pcq->phCounters[ i ] = INVALID_HANDLE_VALUE;
            }
        }
    }
    
    // Free resrouces and memory.
    PDH_STATUS pdhStatus;
    if ( ERROR_SUCCESS != ( pdhStatus = pfnPdhCloseQuery( pcq->hQuery ) ) ){
        OutputWarning( "%s(): PdhCloseQuery() failed with %#x!" CRLF, __FUNCTION__, pdhStatus );
    }
    DeleteCriticalSection( pcq->pcs );  
    delete pcq->pcs;
    delete []pcq->phCounters;
    delete pcq;

    return TRUE;
}

#else
//Windows Phone
BOOL
PerfCountersExist()
{
    return FALSE;
}

BOOL
PerfCountersInstallA(
    __in_opt PCSTR  szLibraryPath,
    __in_opt PCSTR  szIniFilePath
)
{
    return FALSE;
}

BOOL
PerfCountersInstallW(
    __in_opt PCWSTR wszLibraryPath,
    __in_opt PCWSTR wszIniFilePath
)
{
    return FALSE;
}

BOOL
PerfCountersUninstall()
{
    return FALSE;
}

HANDLE
PerfCountersCreateQuery()
{
    return NULL;
}

BOOL
PerfCountersAddLnCounterA(
    _In_ HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    _In_ PCSTR      szPerfObject,
    _In_ PCSTR      szPerfCounter,
    _In_ PCSTR      szInstance
)
{
    return FALSE;
}

BOOL
PerfCountersAddLnCounterW(
    _In_ HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    _In_ PCWSTR     wszPerfObject,
    _In_ PCWSTR     wszPerfCounter,
    _In_ PCWSTR     wszInstance
)
{
    return FALSE;
}

BOOL
PerfCountersAddCounterA(
    _In_ HANDLE     hQuery,
    __in_opt PCSTR  szComputerName,
    _In_ PCSTR      szPerfObject,
    _In_ PCSTR      szPerfCounter,
    _In_ PCSTR      szInstance
)
{
    return FALSE;
}

BOOL
PerfCountersAddCounterW(
    _In_ HANDLE     hQuery,
    __in_opt PCWSTR wszComputerName,
    _In_ PCWSTR     wszPerfObject,
    _In_ PCWSTR     wszPerfCounter,
    _In_ PCWSTR     wszInstance
)
{
    return FALSE;
}

BOOL
PerfCountersGetCounterValues(
    _In_ HANDLE         hQuery,
    __out_opt double*   pData
)
{
    return FALSE;
}

BOOL
PerfCountersGetCounterValuesRaw(
    _In_ HANDLE         hQuery,
    __out_opt ULONG64*  pData1,
    __out_opt ULONG64*  pData2
)
{
    return FALSE;
}

BOOL
PerfCountersStartCollectingStatsA(
    _In_ HANDLE             hQuery,
    _In_ DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCSTR          szLogFile
)
{
    return FALSE;
}

BOOL
PerfCountersStartCollectingStatsW(
    _In_ HANDLE             hQuery,
    _In_ DWORD              dwPeriod,
    __in_opt JET_GRBIT*     pgrbitStats,
    __in_opt PCWSTR         wszLogFile
)
{
    return FALSE;
}

BOOL
PerfCountersRetrieveStats(
    _In_ HANDLE         hQuery,
    __out_opt double*   pMostRecent,
    __out_opt double*   pDataMin,
    __out_opt double*   pTimeMin,
    __out_opt double*   pDataMax,
    __out_opt double*   pTimeMax,
    __out_opt double*   pDataAvg,
    __out_opt double*   pDataVar,
    __out_opt ULONG64*  cCollected
)
{
    return FALSE;
}

BOOL
PerfCountersStopCollectingStats(
    _In_ HANDLE hQuery
)
{
    return FALSE;
}

BOOL
PerfCountersDestroyQuery(
    _In_ HANDLE hQuery
)
{
    return FALSE;
}
#endif

