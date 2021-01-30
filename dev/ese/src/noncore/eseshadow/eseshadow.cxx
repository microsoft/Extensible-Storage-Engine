// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>


#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with atlbase/atlcomcli, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with atlbase/atlcomcli, someone else owns that.")
#pragma prefast(disable:33006, "Do not bother us with atlbase/atlcomcli, someone else owns that.")
#include <atlbase.h>
#pragma prefast(pop)

#include <stdio.h>
#include "eseshadow.h"
#include "esewriter.hxx"

#define STRSAFE_NO_DEPRECATE
#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)

#ifndef GUID_NULL
struct __declspec(uuid("00000000-0000-0000-0000-000000000000")) GUID_NULL;
#define GUID_NULL __uuidof(struct GUID_NULL)
#endif

const VSS_ID guidSystemWriter = { 
    0xe8132975,
    0x6f93,
    0x4464,
    {0xa5, 0x3e, 0x10, 0x50, 0x25, 0x3a, 0xe2, 0x20}
};




class EseShadowInformation
{
public:
    IVssBackupComponents*   m_pvbc;
    bool            m_ComInitialized;
    VSS_ID          m_vssIdSnapshotSet;
    VSS_ID          m_vssIdDbSnapshot;
    VSS_ID          m_vssIdLogSnapshot;
    VSS_ID          m_vssIdSystemSnapshot;

    PWSTR           m_databaseFile;
    PWSTR           m_logFilePath;
    PWSTR           m_systemFilePath;

    EseShadowInformation()
    {
        m_pvbc = NULL;
        m_ComInitialized = false;
        m_vssIdSnapshotSet = GUID_NULL;
        m_vssIdDbSnapshot = GUID_NULL;
        m_vssIdLogSnapshot = GUID_NULL;
        m_vssIdSystemSnapshot = GUID_NULL;
        m_databaseFile = NULL;
        m_logFilePath = NULL;
        m_systemFilePath = NULL;
    }

    ~EseShadowInformation()
    {
        if ( m_ComInitialized )
        {
            CoUninitialize();
        }
        delete[] m_databaseFile;
        m_databaseFile = NULL;
        delete[] m_logFilePath;
        m_logFilePath = NULL;
        delete[] m_systemFilePath;
        m_systemFilePath = NULL;
    }
};


HRESULT __stdcall
EseShadowInit(
    __out EseShadowContext* pcontext
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    IVssBackupComponents* pvbc = NULL;

    EseShadowInformation* pesi = new EseShadowInformation();
    if ( pesi == NULL )
    {
        DBGV( wprintf( L"%hs: Failed allocating EseShadowInformation.\n", __FUNCTION__ ) );
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if ( SUCCEEDED( hr ) )
    {
        pesi->m_ComInitialized = true;
    }

    CallHr( CreateVssBackupComponents( &pvbc ) );
    CallHr( pvbc->InitializeForBackup() );

    CallHr( pvbc->SetContext( VSS_CTX_BACKUP | VSS_VOLSNAP_ATTR_ROLLBACK_RECOVERY ) );

    pesi->m_pvbc = pvbc;

    CallHr( EseRecoveryWriter::GetSingleton()->Initialize() );

    *pcontext = static_cast<EseShadowContext>( pesi );

    
Cleanup:
    if ( FAILED( hr ) )
    {
        delete pesi;
    }
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

HRESULT __stdcall
EseShadowTerm(
    __in EseShadowContext context
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    EseShadowInformation* pesi = static_cast<EseShadowInformation*>( context );

    if ( pesi->m_pvbc != NULL )
    {
        pesi->m_pvbc->Release();
        pesi->m_pvbc = NULL;
    }

    if ( g_eseRecoveryWriterConfig.m_szDatabasePath != NULL )
    {
        free( (void*) g_eseRecoveryWriterConfig.m_szDatabasePath );
        g_eseRecoveryWriterConfig.m_szDatabasePath = NULL;
    }

    delete pesi;

    EseRecoveryWriter::GetSingleton()->Uninitialize();

    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

HRESULT VssIdToString(
    __in VSS_ID const& vssId,
    __out_ecount(cch) LPWSTR szStr,
    DWORD cch
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    RPC_WSTR strGuid = NULL;
    HRESULT hr = S_OK;

    CallHr( UuidToStringW( &vssId, &strGuid ) );
    CallHr( StringCchPrintfW( szStr, cch, L"{%s}", strGuid ) );

Cleanup:
    if (strGuid != NULL) {
        RpcStringFreeW(&strGuid);
    }
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

static PWSTR WcsDupNew(
    __in PCWSTR szOld
)
{
    const size_t cchOld = wcslen( szOld ) + 1;
    const size_t cbOld = cchOld * sizeof( szOld[ 0 ] );
    PWSTR szNew = new WCHAR[ cchOld ];
    if ( szNew == NULL )
    {
        goto Cleanup;
    }
    memcpy( szNew, szOld, cbOld );
Cleanup:
    return szNew;
}

static HRESULT __stdcall
EseShadowIMountShadow(
    __in IVssBackupComponents* pvbc,
    __in const VSS_ID   vssIdVolume,
    __in PCWSTR     fileName,
    __out_ecount( cchOutPath ) PWSTR szOutPath,
    DWORD cchOutPath
)
{
    DBGV( wprintf( L"Entering %hs( %s ).\n", __FUNCTION__, fileName ) );
    HRESULT hr = S_OK;

    VSS_SNAPSHOT_PROP prop;
    BOOL fFreeProp = FALSE;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    WCHAR fullPath[ MAX_PATH ];
    BOOL fRc;

    VSS_SNAPSHOT_PROP* pProp = NULL;

    szOutPath[ 0 ] = L'\0';

    CallHr( pvbc->GetSnapshotProperties( vssIdVolume, &prop ) );
    fFreeProp = TRUE;
    pProp = &prop;

    WCHAR szID[50];
    CallHr( VssIdToString( vssIdVolume, szID, ARRAYSIZE( szID ) ) );

    if ( pProp->m_pwszSnapshotDeviceObject != NULL )
    {
        DBGV( wprintf( L"Device name is %s.\n", pProp->m_pwszSnapshotDeviceObject ) );
        CallHr( StringCchCopyW( szOutPath, cchOutPath, pProp->m_pwszSnapshotDeviceObject ) );
        hr = S_OK;
    }

    hfile = CreateFileW( fileName,
        STANDARD_RIGHTS_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );
    if ( hfile == INVALID_HANDLE_VALUE )
    {
        const DWORD dwGle = GetLastError();
        DBGV( wprintf( L"Failed to open %s: Error %d.\n", fileName, dwGle ) );
        hr = HRESULT_FROM_WIN32( dwGle );
        goto Cleanup;
    }

    fRc = GetFinalPathNameByHandleW( hfile, fullPath, _countof( fullPath ), VOLUME_NAME_NONE );
    if (!fRc )
    {
        const DWORD dwGle = GetLastError();
        DBGV( wprintf( L"Failed GetFinalPathNameByHandleW( %s ): returned %d.\n", fileName, dwGle ) );
        hr = HRESULT_FROM_WIN32( dwGle );
        goto Cleanup;
    }

    CallHr( StringCchCatW( szOutPath, cchOutPath, fullPath ) );

Cleanup:
    if ( hfile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hfile );
        hfile = INVALID_HANDLE_VALUE;
    }
    if (fFreeProp) {
        VssFreeSnapshotProperties(&prop);
    }

    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

HRESULT __stdcall
EseShadowMountShadow(
    __in EseShadowContext context,
    __out_ecount( cchOutDatabasePath ) PWSTR szOutDatabasePath,
    DWORD cchOutDatabasePath,
    __out_ecount( cchOutLogPath ) PWSTR szOutLogPath,
    DWORD cchOutLogPath,
    __out_ecount( cchOutSystemPath ) PWSTR szOutSystemPath,
    DWORD cchOutSystemPath
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    EseShadowInformation* pesi  = static_cast<EseShadowInformation*>( context );
    IVssBackupComponents* pvbc  = pesi->m_pvbc;

    CallHr( EseShadowIMountShadow(
        pvbc,
        pesi->m_vssIdDbSnapshot,
        pesi->m_databaseFile,
        szOutDatabasePath,
        cchOutDatabasePath
    ) );

    CallHr( EseShadowIMountShadow(
        pvbc,
        pesi->m_vssIdLogSnapshot,
        pesi->m_logFilePath,
        szOutLogPath,
        cchOutLogPath
    ) );

    CallHr( EseShadowIMountShadow(
        pvbc,
        pesi->m_vssIdSystemSnapshot,
        pesi->m_systemFilePath,
        szOutSystemPath,
        cchOutSystemPath) );

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );

    if ( FAILED( hr ) )
    {
        pesi->m_vssIdDbSnapshot = GUID_NULL;
        pesi->m_vssIdLogSnapshot = GUID_NULL;
        pesi->m_vssIdSystemSnapshot = GUID_NULL;
    }
    
    return hr;
}

HRESULT __stdcall
EseShadowMountSimpleShadow(
    _In_ EseShadowContext context,
    _Out_cap_( cchOutDatabasePath ) PWSTR szOutDatabasePath,
    DWORD cchOutDatabasePath
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    EseShadowInformation* pesi  = static_cast<EseShadowInformation*>( context );
    IVssBackupComponents* pvbc  = pesi->m_pvbc;

    CallHr( EseShadowIMountShadow(
        pvbc,
        pesi->m_vssIdDbSnapshot,
        pesi->m_databaseFile,
        szOutDatabasePath,
        cchOutDatabasePath
    ) );

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );

    if ( FAILED( hr ) )
    {
        pesi->m_vssIdDbSnapshot = GUID_NULL;
    }
    
    return hr;
}


HRESULT __stdcall
EseShadowICreateShadow(
    __in IVssBackupComponents* pvbc,
    __in PCWSTR szDatabaseFile,
    __in_opt PCWSTR szLogDirectory,
    __in_opt PCWSTR szSystemDirectory,
    __out VSS_ID* pvssIdSnapshotSet,
    __out VSS_ID* pvssIdDbSnapshot,
    __out_opt VSS_ID* pvssIdLogSnapshot,
    __out_opt VSS_ID* pvssIdSystemSnapshot
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );

    HRESULT hr = S_OK;
    CComPtr<IVssAsync> pAsync;
    CComPtr<IVssExamineWriterMetadata> pMetadata;
    CComPtr<IVssWMComponent> pComponent;
    BOOL fFreeWriterMetadata = FALSE;
    BOOL fFreeWriterStatus = FALSE;
    WCHAR szDbDrive[MAX_PATH] = L"";
    WCHAR szLogDrive[MAX_PATH] = L"";
    WCHAR szSystemDrive[MAX_PATH] = L"";

    if ( pvssIdLogSnapshot != NULL )
    {
        *pvssIdLogSnapshot = GUID_NULL;
    }

    if ( pvssIdSystemSnapshot != NULL )
    {
        *pvssIdSystemSnapshot = GUID_NULL;
    }

    CallHr( pvbc->SetBackupState( true, false, VSS_BT_FULL, true ) );

    CallHr( pvbc->DisableWriterClasses( &guidSystemWriter, 1 ) );

    CallHr( pvbc->GatherWriterMetadata( &pAsync ) );
    CallHr( pAsync->Wait() );
    pAsync.Detach();

    fFreeWriterMetadata = TRUE;

    UINT cWriters;
    CallHr( pvbc->GetWriterMetadataCount(&cWriters) );

    CallHr( pvbc->StartSnapshotSet( pvssIdSnapshotSet ) );

    for ( UINT iWriter = 0; iWriter < cWriters; iWriter++) {
        VSS_ID idInstance;
        VSS_ID idInstanceT;
        VSS_ID idWriter;
        BSTR bstrWriterName;
        VSS_USAGE_TYPE vssUsageType;
        VSS_SOURCE_TYPE vssSourceType;
        BOOL bIncludeWriter = FALSE;
        PVSSCOMPONENTINFO pInfo;

        pMetadata.Release();

        CallHr( pvbc->GetWriterMetadata(iWriter, &idInstance, &pMetadata ) );
        CallHr( pMetadata->GetIdentity(
            &idInstanceT,
            &idWriter,
            &bstrWriterName,
            &vssUsageType,
            &vssSourceType ) );

        ASSERT( memcmp( &idInstance, &idInstanceT, sizeof(VSS_ID) ) == 0 );

        DBGV(
            WCHAR szID[ 50 ];
            CallHr( VssIdToString( idWriter, szID, _countof( szID ) ) );
            wprintf( L"Looking at writer GUID %s.\n", szID );
        );

        if ( memcmp( &idWriter, &EseRecoveryWriterId, sizeof(VSS_ID) ) == 0
#ifdef ENABLE_EXTERNAL_ESE_WRITERS
#ifdef ESENT
            || memcmp( &idWriter, &AdamWriterId, sizeof(VSS_ID) ) == 0
            || memcmp( &idWriter, &AdWriterId, sizeof(VSS_ID) ) == 0
#else
            || memcmp( &idWriter, &StoreWriterId, sizeof(VSS_ID) ) == 0
#endif
#endif
        )
        {
            WCHAR szId[50];
            (void) VssIdToString( idWriter, szId, _countof( szId ) );
            
            DBGV( wprintf( L"Found a suitable writer to invoke: %s\n", szId ) );
            bIncludeWriter = TRUE;
        }

        if (!bIncludeWriter)
        {
            DBGV( wprintf( L"%hs: No writer found.\n", __FUNCTION__ ) );
            continue;
        }

        unsigned cIncludeFiles, cExcludeFiles, cComponents;
        CallHr( pMetadata->GetFileCounts( &cIncludeFiles, &cExcludeFiles, &cComponents ) );
        for ( unsigned iComponent = 0; iComponent < cComponents; iComponent++ )
        {
            CallHr( pMetadata->GetComponent( iComponent, &pComponent ) );
            CallHr( pComponent->GetComponentInfo( &pInfo ) );

            CallHr( pvbc->AddComponent(
                idInstance,
                idWriter,
                pInfo->type,
                pInfo->bstrLogicalPath,
                pInfo->bstrComponentName
                ));

            pComponent->FreeComponentInfo(pInfo);
        }
    }
    pvbc->FreeWriterMetadata();
    fFreeWriterMetadata = FALSE;


    BOOL fRc = GetVolumePathNameW( szDatabaseFile, szDbDrive, _countof( szDbDrive ) );
    if ( !fRc )
    {
        DWORD status = GetLastError();
        DBGV( wprintf( L"Failed GetVolumePathNameW( %s ): returned %d.\n", szDatabaseFile, status ) );
        hr = HRESULT_FROM_WIN32( status );
        goto Cleanup;
    }

    CallHr( pvbc->AddToSnapshotSet(szDbDrive, GUID_NULL, pvssIdDbSnapshot ) );

    if ( szLogDirectory )
    {
        fRc = GetVolumePathNameW( szLogDirectory, szLogDrive, _countof( szLogDrive ) );
        if ( !fRc )
        {
            DWORD status = GetLastError();
            DBGV( wprintf( L"Failed GetVolumePathNameW( %s ): returned %d.\n", szLogDirectory, status ) );
            hr = HRESULT_FROM_WIN32( status );
            goto Cleanup;
        }

        if (_wcsicmp(szDbDrive, szLogDrive) == 0)
        {
            *pvssIdLogSnapshot = *pvssIdDbSnapshot;
        }
        else
        {
            CallHr( pvbc->AddToSnapshotSet( szLogDrive, GUID_NULL, pvssIdLogSnapshot ) );
        }
    }
    else
    {
        *pvssIdLogSnapshot = *pvssIdDbSnapshot;
    }

    if ( szSystemDirectory )
    {
        fRc = GetVolumePathNameW( szSystemDirectory, szSystemDrive, _countof( szSystemDrive ) );
        if ( !fRc )
        {
            DWORD status = GetLastError();
            DBGV( wprintf( L"Failed GetVolumePathNameW( %s ): returned %d.\n", szSystemDrive, status ) );
            hr = HRESULT_FROM_WIN32( status );
            goto Cleanup;
        }

        if (_wcsicmp(szDbDrive, szSystemDrive) == 0)
        {
            *pvssIdSystemSnapshot = *pvssIdDbSnapshot;
        }
        else if (_wcsicmp(szLogDrive, szSystemDrive) == 0)
        {
            *pvssIdSystemSnapshot = *pvssIdLogSnapshot;
        }
        else
        {
            CallHr( pvbc->AddToSnapshotSet( szSystemDrive, GUID_NULL, pvssIdSystemSnapshot ) );
        }
    }
    else
    {
        *pvssIdSystemSnapshot = *pvssIdLogSnapshot;
    }

    CallHr( pvbc->PrepareForBackup( &pAsync ) );
    CallHr( pAsync->Wait() );
    pAsync.Detach();

    CallHr( pvbc->DoSnapshotSet( &pAsync ) );
    CallHr( pAsync->Wait());
    pAsync.Detach();

    CallHr( pvbc->GatherWriterStatus( &pAsync ) );
    CallHr( pAsync->Wait());
    pAsync.Detach();
    fFreeWriterStatus = TRUE;

    CallHr( pvbc->GetWriterStatusCount(&cWriters) );
    for ( UINT iWriter = 0; iWriter < cWriters; iWriter++ )
    {
        VSS_ID idInstance;
        VSS_ID idWriter;
        BSTR bstrWriterName;
        VSS_WRITER_STATE vssWriterState;
        HRESULT hrStatus;
        CallHr( pvbc->GetWriterStatus( iWriter, &idInstance, &idWriter, &bstrWriterName, &vssWriterState, &hrStatus ) );
        CallHr( hrStatus );
    }
    pvbc->FreeWriterStatus();
    fFreeWriterStatus = FALSE;

    CallHr( pvbc->BackupComplete( &pAsync ) );
    CallHr( pAsync->Wait() );

    WCHAR szID[50];
    CallHr( VssIdToString( *pvssIdSnapshotSet, szID, ARRAYSIZE(szID ) ) );

Cleanup:

    if (fFreeWriterMetadata) {
        pvbc->FreeWriterMetadata();
    }
    if (fFreeWriterStatus) {
        pvbc->FreeWriterStatus();
    }

    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

HRESULT __stdcall
EseShadowCreateShadow(
    __in EseShadowContext context,
    __in PCWSTR szDatabaseFile,
    __in_opt PCWSTR szLogDirectory,
    __in_opt PCWSTR szSystemDirectory,
    __in_opt PCWSTR szEseBaseName,
    __in BOOL fIgnoreMissingDb,
    __in BOOL fIgnoreLostLogs
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;

    EseShadowInformation* pesi = static_cast<EseShadowInformation*>( context );
    IVssBackupComponents* pvbc = NULL;
    WCHAR szID[50];

    if ( pesi == NULL )
    {
        DBGV( wprintf( L"EseShadowPurgeShadow() called with a NULL context!" ) );
        goto Cleanup;
    }
    pvbc = pesi->m_pvbc;

    g_eseRecoveryWriterConfig.m_fIgnoreMissingDb = fIgnoreMissingDb;
    g_eseRecoveryWriterConfig.m_fIgnoreLostLogs = fIgnoreLostLogs;

    size_t databasePathLen = szDatabaseFile ? wcslen( szDatabaseFile ) + 1 : 0;
    if ( databasePathLen > 0 )
    {
        pesi->m_databaseFile = new WCHAR[ databasePathLen ];
        if ( pesi->m_databaseFile == NULL )
        {
            DBGV( wprintf( L"EseShadowCreateShadow ran out of memory!\n" ) );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy_s( pesi->m_databaseFile, databasePathLen, szDatabaseFile );
        g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath = pesi->m_databaseFile;
        if ( szEseBaseName != NULL )
        {
        StringCchCopyW(
            g_eseRecoveryWriterConfig.m_szEseBaseName,
            _countof( g_eseRecoveryWriterConfig.m_szEseBaseName ),
            szEseBaseName );
        }

        g_eseRecoveryWriterConfig.m_szDatabasePath = WcsDupNew( g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath );

        wchar_t* pchLastBackslash = const_cast<wchar_t*>( wcsrchr( g_eseRecoveryWriterConfig.m_szDatabasePath, L'\\' ) );
        if ( pchLastBackslash != NULL )
        {
            *pchLastBackslash = L'\0';
        }

        databasePathLen = g_eseRecoveryWriterConfig.m_szDatabasePath ? wcslen( g_eseRecoveryWriterConfig.m_szDatabasePath ) + 1 : 0;
    }
    else
    {
        DBGV( wprintf( L"Need to provide a database name!\n" ) );
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    size_t logPathLength = 0;
    PCWSTR szLogPath = NULL;
    if ( szLogDirectory )
    {
        logPathLength = wcslen( szLogDirectory ) + 1;
        szLogPath = szLogDirectory;
    }
    else
    {
        logPathLength = databasePathLen;
        szLogPath = g_eseRecoveryWriterConfig.m_szDatabasePath;
    }

    if ( logPathLength > 0 )
    {
        pesi->m_logFilePath = new WCHAR[ logPathLength ];
        if ( pesi->m_logFilePath == NULL )
        {
            DBGV( wprintf( L"EseShadowCreateShadow ran out of memory!\n" ) );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy_s( pesi->m_logFilePath, logPathLength, szLogPath );
        g_eseRecoveryWriterConfig.m_szLogDirectory = pesi->m_logFilePath;
    }

    size_t systemPathLength = 0;
    PCWSTR szSystemPath = NULL;
    if ( szSystemDirectory )
    {
        systemPathLength = wcslen( szSystemDirectory ) + 1;
        szSystemPath = szSystemDirectory;
    }
    else
    {
        systemPathLength = logPathLength;
        szSystemPath = g_eseRecoveryWriterConfig.m_szLogDirectory;
    }

    if ( systemPathLength > 0 )
    {
        pesi->m_systemFilePath = new WCHAR[ systemPathLength ];
        if ( pesi->m_systemFilePath == NULL )
        {
            DBGV( wprintf( L"EseShadowCreateShadow ran out of memory!\n" ) );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy_s( pesi->m_systemFilePath, systemPathLength, szSystemPath );
        g_eseRecoveryWriterConfig.m_szSystemDirectory = pesi->m_systemFilePath;
    }

    CallHr( EseShadowICreateShadow(
        pvbc,
        szDatabaseFile,
        szLogDirectory,
        szSystemDirectory,
        &pesi->m_vssIdSnapshotSet,
        &pesi->m_vssIdDbSnapshot,
        &pesi->m_vssIdLogSnapshot,
        &pesi->m_vssIdSystemSnapshot
    ) );

    CallHr( VssIdToString( pesi->m_vssIdSnapshotSet, szID, _countof( szID ) ) );
    DBGV( wprintf( L"VSS Snapshot ID = %s\n", szID ) );

    CallHr( VssIdToString( pesi->m_vssIdDbSnapshot, szID, _countof( szID ) ) );
    DBGV( wprintf( L"VSS Database dir ID = %s\n", szID ) );

    CallHr( VssIdToString( pesi->m_vssIdLogSnapshot, szID, _countof( szID ) ) );
    DBGV( wprintf( L"VSS Log dir ID = %s\n", szID ) );

    CallHr( VssIdToString( pesi->m_vssIdSystemSnapshot, szID, _countof( szID ) ) );
    DBGV( wprintf( L"VSS System dir ID = %s\n", szID ) );

    DBGV( wprintf( L"EseShadowICreateShadow was successful!\n" ) );

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

HRESULT __stdcall
EseShadowCreateSimpleShadow(
    __in EseShadowContext context,
    __in PCWSTR szArbitraryFile
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;

    EseShadowInformation* pesi = static_cast<EseShadowInformation*>( context );
    IVssBackupComponents* pvbc = NULL;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    WCHAR szID[50];

    if ( pesi == NULL )
    {
        DBGV( wprintf( L"EseShadowPurgeShadow() called with a NULL context!" ) );
        goto Cleanup;
    }
    pvbc = pesi->m_pvbc;

    size_t databasePathLen = szArbitraryFile ? wcslen( szArbitraryFile ) + 1 : 0;

    if ( databasePathLen > 0 )
    {
        pesi->m_databaseFile = new WCHAR[ databasePathLen ];
        if ( pesi->m_databaseFile == NULL )
        {
            DBGV( wprintf( L"EseShadowCreateShadow ran out of memory!\n" ) );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        wcscpy_s( pesi->m_databaseFile, databasePathLen, szArbitraryFile );
        g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath = pesi->m_databaseFile;

        g_eseRecoveryWriterConfig.m_szDatabasePath = WcsDupNew( g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath );

        wchar_t* pchLastBackslash = const_cast<wchar_t*>( wcsrchr( g_eseRecoveryWriterConfig.m_szDatabasePath, L'\\' ) );
        if ( pchLastBackslash != NULL )
        {
            *pchLastBackslash = L'\0';
        }

    }
    else
    {
        DBGV( wprintf( L"Need to provide a database name!\n" ) );
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    hfile = CreateFileW( pesi->m_databaseFile,
        STANDARD_RIGHTS_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );
    if ( hfile == INVALID_HANDLE_VALUE )
    {
        const DWORD dwGle = GetLastError();
        DBGV( wprintf( L"Failed to open %s: Error %d.\n", pesi->m_databaseFile, dwGle ) );
        hr = HRESULT_FROM_WIN32( dwGle );
        goto Cleanup;
    }

    CallHr( EseShadowICreateShadow(
        pvbc,
        szArbitraryFile,
        NULL,
        NULL,
        &pesi->m_vssIdSnapshotSet,
        &pesi->m_vssIdDbSnapshot,
        &pesi->m_vssIdLogSnapshot,
        &pesi->m_vssIdSystemSnapshot
    ) );

    CallHr( VssIdToString( pesi->m_vssIdSnapshotSet, szID, _countof( szID ) ) );
    DBGV( wprintf( L"VSS Snapshot ID = %s\n", szID ) );

    CallHr( VssIdToString( pesi->m_vssIdDbSnapshot, szID, _countof( szID ) ) );
    DBGV( wprintf( L"VSS Database dir ID = %s\n", szID ) );

    DBGV( wprintf( L"EseShadowICreateShadow was successful!\n" ) );

Cleanup:
    if ( hfile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hfile );
        hfile = INVALID_HANDLE_VALUE;
    }
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}



HRESULT __stdcall
EseShadowPurgeShadow(
    __in EseShadowContext pcontext
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    HRESULT hrT = S_OK;
    EseShadowInformation* pesi = (EseShadowInformation*) pcontext;
    IVssBackupComponents* pvbc = NULL;

    LONG nSnapshots = 0;
    VSS_ID nonDeletedId = { 0 };

    if ( pesi == NULL )
    {
        DBGV( wprintf( L"EseShadowPurgeShadow() called with a NULL context!" ) );
        goto Cleanup;
    }
    pvbc = pesi->m_pvbc;

    hrT = pvbc->DeleteSnapshots( pesi->m_vssIdSnapshotSet, VSS_OBJECT_SNAPSHOT_SET, FALSE, &nSnapshots, &nonDeletedId );
    if ( SUCCEEDED( hr ) )
    {
        hr = hrT;
    }

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}


