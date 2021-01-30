// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "esewriter.hxx"

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with vss headers, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with vss headers, someone else owns that.")
#pragma prefast(disable:33006, "Do not bother us with vss headers, someone else owns that.")
#include <vss.h>
#include <vswriter.h>
#pragma prefast(pop)

#ifdef ESENT
#include "esent_x.h"
#else
#include "jet.h"
#endif

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)

#include <stdio.h>

EseRecoveryWriterConfig g_eseRecoveryWriterConfig;

#ifdef DEBUG
bool g_fVerbose = false;
#else
bool g_fVerbose = false;
#endif
EseRecoveryWriter* EseRecoveryWriter::m_pExWriter = NULL;
long        g_lRefEseWriter = 0;


ULONG_PTR GetLogFileSizeAndExtensionFromDirectoryAndBaseName(
    __in PCWSTR szLogPath,
    __in PCWSTR szEseBaseName,
    __out_ecount(cchMax) PWSTR const szLogExtension,
    __in const DWORD cchMax
)
{
    ULONG_PTR cbFile = 0;
    *szLogExtension = L'\0';

    WCHAR path[ _MAX_PATH ];
    WIN32_FIND_DATAW findData;

    StringCchPrintfW( path, _countof( path ), L"%s\\%s*.%s", szLogPath, szEseBaseName, L"log" );
    HANDLE hfind = FindFirstFileW( path, &findData );
    if ( hfind == INVALID_HANDLE_VALUE )
    {
        StringCchPrintfW( path, _countof( path ), L"%s\\%s*.%s", szLogPath, szEseBaseName, L"jtx" );
        hfind = FindFirstFileW( path, &findData );
        if ( hfind != INVALID_HANDLE_VALUE )
        {
            StringCchCopyW( szLogExtension, cchMax, L"jtx" );
        }
    }
    else
    {
        StringCchCopyW( szLogExtension, cchMax, L"log" );
    }

    if ( hfind != INVALID_HANDLE_VALUE )
    {
        cbFile = findData.nFileSizeLow;
        DBGV( wprintf( L"%hs() FindFirstFile say that '%s' is %Id bytes.\n",
            __FUNCTION__, findData.cFileName, cbFile) );
    }

    goto Cleanup;

Cleanup:
    if ( hfind != INVALID_HANDLE_VALUE )
    {
        FindClose( hfind );
    }

    return cbFile;
}


EseRecoveryWriter::EseRecoveryWriter() :
    m_fVssInitialized( false ),
    m_fVssSubscribed( false )
{
}

EseRecoveryWriter::~EseRecoveryWriter()
{
    Uninitialize();
}


HRESULT STDMETHODCALLTYPE EseRecoveryWriter::Initialize()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;

    if ( !m_fVssInitialized )
    {
        CallHr( __super::Initialize(
            EseRecoveryWriterId,
            EseRecoveryWriterName,
            VSS_UT_USERDATA,
            VSS_ST_OTHER
        ) );
        m_fVssInitialized = true;
    }

    if ( !m_fVssSubscribed )
    {
        CallHr( Subscribe() );
        m_fVssSubscribed = true;
    }

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

HRESULT STDMETHODCALLTYPE EseRecoveryWriter::Uninitialize()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    if ( m_fVssSubscribed )
    {
        CallHr( Unsubscribe() );
        m_fVssSubscribed = false;
    }

Cleanup:
    if ( FAILED( hr ) )
        {
        __super::SetWriterFailure( hr );
        }

    return hr;
}

#pragma prefast(push)
#pragma prefast(disable:28204, "Mismatched annotations; the prototypes are part of the SDK.")

bool STDMETHODCALLTYPE EseRecoveryWriter::OnIdentify(
    _In_ IVssCreateWriterMetadata *pMetadata
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr;
    bool filesToAdd = true;
    PCWSTR szComponent = L"FilesComponent";

    const DWORD dwBackupSchema = VSS_BS_COPY
        | VSS_BS_DIFFERENTIAL
        | VSS_BS_INCREMENTAL
        | VSS_BS_EXCLUSIVE_INCREMENTAL_DIFFERENTIAL
        | VSS_BS_WRITER_SUPPORTS_NEW_TARGET
        | VSS_BS_WRITER_SUPPORTS_RESTORE_WITH_MOVE
        | VSS_BS_WRITER_SUPPORTS_PARALLEL_RESTORES;

    CallHr( pMetadata->SetBackupSchema( dwBackupSchema ) );

    CallHr( pMetadata->SetRestoreMethod(
        VSS_RME_RESTORE_IF_CAN_REPLACE,
        NULL,
        NULL,
        VSS_WRE_ALWAYS,
        false ) );


    CallHr( pMetadata->AddComponent(
        VSS_CT_FILEGROUP,
        NULL,
        szComponent,
        L"recovered_database",
        NULL,
        0,
        false,
        true,
        true,
        true,
        VSS_CF_BACKUP_RECOVERY | VSS_CF_APP_ROLLBACK_RECOVERY
    ) );

    if ( g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath == NULL )
    {
        DBGV( wprintf( L"Warning: g_eseRecoveryWriterConfig->m_szDatabaseFileFullPath is NULL.\n" ) );
        filesToAdd = false;
    }

    if ( filesToAdd )
    {
        DBGV( wprintf( L"Adding directory '%s' (file=*) to the file group\n", g_eseRecoveryWriterConfig.m_szDatabasePath ) );

        CallHr( pMetadata->AddFilesToFileGroup(
            NULL,
            szComponent,
            g_eseRecoveryWriterConfig.m_szDatabasePath,
            L"*",
            false,
            NULL
        ) );

        if ( g_eseRecoveryWriterConfig.m_szLogDirectory != NULL )
        {
            if ( _wcsicmp( g_eseRecoveryWriterConfig.m_szDatabasePath, g_eseRecoveryWriterConfig.m_szLogDirectory ) != 0 )
            {
                DBGV( wprintf( L"Adding '%s' to the file group\n", g_eseRecoveryWriterConfig.m_szLogDirectory ) );

                CallHr( pMetadata->AddFilesToFileGroup(
                    NULL,
                    szComponent,
                    g_eseRecoveryWriterConfig.m_szLogDirectory,
                    L"*",
                    false,
                    NULL
                ) );
            }
        }

        if ( g_eseRecoveryWriterConfig.m_szSystemDirectory != NULL )
        {
            if ( _wcsicmp( g_eseRecoveryWriterConfig.m_szDatabasePath, g_eseRecoveryWriterConfig.m_szSystemDirectory ) != 0 &&
                    _wcsicmp( g_eseRecoveryWriterConfig.m_szLogDirectory, g_eseRecoveryWriterConfig.m_szSystemDirectory ) != 0 )
            {
                DBGV( wprintf( L"Adding '%s' to the file group\n", g_eseRecoveryWriterConfig.m_szSystemDirectory ) );

                CallHr( pMetadata->AddFilesToFileGroup(
                    NULL,
                    szComponent,
                    g_eseRecoveryWriterConfig.m_szSystemDirectory,
                    L"*",
                    false,
                    NULL
                ) );
            }
        }

    }

Cleanup:
    if ( FAILED( hr ) )
        {
        __super::SetWriterFailure( hr );
        }

    DBGV( wprintf( L"Leaving %hs (returning hr=%#x).\n", __FUNCTION__, hr ) );
    return SUCCEEDED( hr );
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnPrepareBackup(
    _In_ IVssWriterComponents*
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnPrepareSnapshot()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnFreeze()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnThaw()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool
STDMETHODCALLTYPE
EseRecoveryWriter::OnPostSnapshot(
    _In_ IVssWriterComponents *
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    HANDLE hfile = INVALID_HANDLE_VALUE;

    DBGV( wprintf( L"OnPostSnapshot() begin.\n" ) );

    if ( ( __super::GetContext() & VSS_VOLSNAP_ATTR_AUTORECOVER ) == 0 )
    {
        DBGV( wprintf( L"%hs(): Not an auto-recovery snapshot. Doing nothing.\n", __FUNCTION__ ) );
        hr = S_OK;
        goto Cleanup;
    }

    if ( NULL == g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath )
    {
        DBGV( wprintf( L"%hs(): The input database name was not set!\n", __FUNCTION__ ) );
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    PCWSTR szInPaths[] = {
        g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath,
        g_eseRecoveryWriterConfig.m_szLogDirectory,
        g_eseRecoveryWriterConfig.m_szSystemDirectory,
    };
    WCHAR szOutPaths[ _countof( szInPaths ) ][MAX_PATH];
    WCHAR szRelativePathFromRoot[ _MAX_PATH ];
    WCHAR szLogPath[ _MAX_PATH ];
    WCHAR szSystemPath[ _MAX_PATH ];

    for ( size_t i = 0; i < _countof( szInPaths ); i++ )
    {
        WCHAR szVolume[ _MAX_PATH ];
        PCWSTR szDeviceName = NULL;
        PCWSTR szInPath = szInPaths[ i ];

        if ( szInPath == NULL )
        {
            continue;
        }

        BOOL fRc = GetVolumePathNameW( szInPath, szVolume, _countof( szVolume ) );
        if ( !fRc )
        {
            DWORD status = GetLastError();
            DBGV( wprintf( L"Failed GetVolumePathNameW( %s ): returned %d.\n", szInPath, status ) );
            hr = HRESULT_FROM_WIN32( status );
            goto Cleanup;
        }

        CallHr( GetSnapshotDeviceName( szVolume, &szDeviceName ) );
        
        hfile = CreateFileW(
            szInPath,
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
            DBGV( wprintf( L"Failed to open %s: Error %d.\n", szInPath, dwGle ) );
            hr = HRESULT_FROM_WIN32( dwGle );
            goto Cleanup;
        }

        fRc = GetFinalPathNameByHandleW( hfile, szRelativePathFromRoot, _countof( szRelativePathFromRoot ), VOLUME_NAME_NONE );
        if (!fRc )
        {
            const DWORD dwGle = GetLastError();
            DBGV( wprintf( L"Failed GetFinalPathNameByHandleW( %s ): returned %d.\n", szInPath, dwGle ) );
            hr = HRESULT_FROM_WIN32( dwGle );
            goto Cleanup;
        }

        CallHr( StringCchPrintfW( szOutPaths[ i ], _countof( szOutPaths[ i ]), L"%s%s", szDeviceName, szRelativePathFromRoot ) );
    }

    if ( g_eseRecoveryWriterConfig.m_szLogDirectory == NULL )
    {
        StringCchCopyW( szLogPath, _countof( szLogPath ), szOutPaths[ 0 ] );
        wchar_t* pchLastBackslash = const_cast<wchar_t*>( wcsrchr( szLogPath, L'\\' ) );
        if ( pchLastBackslash != NULL )
        {
            *pchLastBackslash = L'\0';
        }
    }
    else
    {
        StringCchCopyW( szLogPath, _countof( szLogPath ), szOutPaths[ 1 ] );
    }

    if ( g_eseRecoveryWriterConfig.m_szSystemDirectory == NULL )
    {
        StringCchCopyW( szSystemPath, _countof( szLogPath ), szLogPath );
    }
    else
    {
        StringCchCopyW( szSystemPath, _countof( szSystemPath ), szOutPaths[ 2 ] );
    }

    DBGV( wprintf( L"Snapshot database file: %s\n", szOutPaths[ 0 ] ) );
    DBGV( wprintf( L"Snapshot log directory: %s\n", szLogPath ) );
    DBGV( wprintf( L"Snapshot system directory: %s\n", szSystemPath ) );

    if ( wcslen( g_eseRecoveryWriterConfig.m_szEseBaseName ) > 0 )
    {
        hr = RecoverEseDatabase( szInPaths[ 0 ], szOutPaths[ 0 ], szLogPath, szSystemPath );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }


Cleanup:
    if ( hfile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hfile );
        hfile = INVALID_HANDLE_VALUE;
    }

    if ( FAILED( hr ) )
        {
        __super::SetWriterFailure( hr );
        }

    DBGV( wprintf( L"Leaving %hs (returning hr=%#x).\n", __FUNCTION__, hr ) );

    return SUCCEEDED( hr );
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnAbort()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnBackupComplete(
    _In_ IVssWriterComponents*
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnBackupShutdown(
    _In_ VSS_ID
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnPreRestore(
    _In_ IVssWriterComponents *
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

bool STDMETHODCALLTYPE EseRecoveryWriter::OnPostRestore(
    _In_ IVssWriterComponents*
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

#pragma prefast(pop)


HRESULT
EseRecoveryWriter::RecoverEseDatabase(
    __in PCWSTR szOldDbName,
    __in PCWSTR szNewDbName,
    __in PCWSTR szLogPath,
    __in PCWSTR szSystemPath
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    DWORD                   dwErr                   = JET_errSuccess;
    JET_ERR                 err                     = JET_errSuccess;
    JET_INSTANCE            instance                = 0;
    WCHAR                    szDefaultTempDB[MAX_PATH];
    WCHAR                    szTempPath[MAX_PATH];

    JET_RSTINFO_W           rstInfo = {0};
    JET_RSTMAP_W            rstMap = {0};
    HRESULT                 hr = S_OK;

    if ( GetTempPathW(_countof(szTempPath), szTempPath) == 0) {
        dwErr = GetLastError();
        DBGV( wprintf( L"GetTempPath() failed with 0x%x\n", dwErr ) );
        hr = HRESULT_FROM_WIN32( dwErr );
        __super::SetWriterFailure( hr );
        goto Cleanup;
    }

    if ( GetTempFileNameW( szTempPath, L"ewe", 0, szDefaultTempDB) == 0)
    {
        dwErr = GetLastError();
        DBGV( wprintf( L"GetTempFileName() failed with 0x%x\n", dwErr ) );
        hr = HRESULT_FROM_WIN32( dwErr );
        __super::SetWriterFailure( hr );
        goto Cleanup;
    }

    JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, szDefaultTempDB );

    ULONG cbPage;
    CallEse( JetGetDatabaseFileInfoW( szNewDbName, &cbPage, sizeof( cbPage ), JET_DbInfoPageSize ) );
    JetSetSystemParameterW( &instance, 0, JET_paramDatabasePageSize, cbPage, NULL );

    if ( szLogPath != NULL )
    {
        JetSetSystemParameterW( &instance, 0, JET_paramLogFilePath, 0, szLogPath );
    }

    if ( szSystemPath != NULL )
    {
        JetSetSystemParameterW( &instance, 0, JET_paramSystemPath, 0, szSystemPath );
    }

    JetSetSystemParameterW( &instance, 0, JET_paramBaseName, 0, g_eseRecoveryWriterConfig.m_szEseBaseName );
    JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"on" );

    WCHAR szLogExtension[4] = { L'\0' };
    ULONG_PTR cbLogFileSize = GetLogFileSizeAndExtensionFromDirectoryAndBaseName( szLogPath, g_eseRecoveryWriterConfig.m_szEseBaseName, szLogExtension, _countof( szLogExtension ) );

    if ( cbLogFileSize != 0 )
    {
        JetSetSystemParameterW( &instance, 0, JET_paramLogFileSize, cbLogFileSize / 1024, NULL );

        if ( g_eseRecoveryWriterConfig.m_fIgnoreLostLogs )
        {
            WCHAR szTempLogFileName[MAX_PATH] = { L'\0' };
            StringCchPrintfW( szTempLogFileName, _countof( szTempLogFileName ), L"%ls\\%lstmp.%ls", szLogPath, g_eseRecoveryWriterConfig.m_szEseBaseName, szLogExtension );
            HANDLE hfile = CreateFileW( szTempLogFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL );
            if ( hfile == INVALID_HANDLE_VALUE )
            {
                const DWORD dwGle = GetLastError();
                if ( dwGle != ERROR_FILE_EXISTS )
                {
                    DBGV( wprintf( L"Failed to create temporary log file %s: Error %d.\n", szTempLogFileName, dwGle ) );
                }
            }
            else
            {
                CloseHandle( hfile );
            }
        }
    }

    rstInfo.cbStruct = sizeof( rstInfo );
    rstInfo.crstmap = 1;
    rstInfo.rgrstmap = &rstMap;

    rstMap.szDatabaseName = const_cast<WCHAR*>( szOldDbName );
    rstMap.szNewDatabaseName = const_cast<WCHAR*>( szNewDbName );

    JET_GRBIT grbit = JET_bitNil;
    if ( g_eseRecoveryWriterConfig.m_fIgnoreMissingDb )
    {
        grbit |= ( JET_bitReplayIgnoreMissingDB | JET_bitReplayMissingMapEntryDB );
    }
    if ( g_eseRecoveryWriterConfig.m_fIgnoreLostLogs )
    {
        grbit |= JET_bitReplayIgnoreLostLogs;
    }

    CallEse( JetInit3W( &instance, &rstInfo, grbit ) );

    DBGV( wprintf( L"RecoverJetDB successfully replayed the logs.\n" ) );

Cleanup:
    if (instance != 0)
    {
        JetTerm2( instance, SUCCEEDED(hr) ? JET_bitTermComplete : JET_bitTermAbrupt );
    }

    if ( FAILED( hr ) )
        {
        __super::SetWriterFailure( hr );
        }

    return hr;
}



HRESULT
EseRecoveryWriter::CreateWriter()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    

    if ( NULL == EseRecoveryWriter::GetSingleton() )
    {
        hr = E_OUTOFMEMORY;
        DBGV( wprintf( L"%hs() failed with hr=%#x\n", __FUNCTION__, hr ) );
        goto Error;
    }

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
    
Error:
    goto Cleanup;
}


VOID
EseRecoveryWriter::DestroyWriter()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    EseRecoveryWriter::DeleteSingleton();
    DBGV( wprintf( L"Leaving %hs.\n", __FUNCTION__ ) );
}




extern "C" {

BOOL
WINAPI
DllMain(
        HINSTANCE hinstDll,
        DWORD dwReason,
        LPVOID 
        )
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    BOOL    fReturn = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {

        if ( InterlockedIncrement( &g_lRefEseWriter ) == 1 )
        {
            DisableThreadLibraryCalls( hinstDll );
        }

        if ( S_OK != EseRecoveryWriter::CreateWriter() )
        {
            fReturn = FALSE;
        }
    }
    else if ( dwReason == DLL_PROCESS_DETACH )
    {
        EseRecoveryWriter::DestroyWriter();

        InterlockedDecrement( &g_lRefEseWriter );
    }

    DBGV( wprintf( L"Leaving %hs (returning %d).\n", __FUNCTION__, fReturn ) );
    return fReturn;

}

}
