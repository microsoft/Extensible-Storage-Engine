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

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)

#include <stdio.h>

// Unsynchronized access. Be careful.
EseRecoveryWriterConfig g_eseRecoveryWriterConfig;

#ifdef DEBUG
bool g_fVerbose = false;
#else
bool g_fVerbose = false;
#endif
EseRecoveryWriter* EseRecoveryWriter::m_pExWriter = NULL;
long        g_lRefEseWriter = 0;


ULONG_PTR GetLogFileSizeAndExtensionFromDirectoryAndBaseName(
    _In_ PCWSTR szLogPath,
    _In_ PCWSTR szEseBaseName,
    __out_ecount(cchMax) PWSTR const szLogExtension,
    _In_ const DWORD cchMax
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


// Initialize the writer
HRESULT STDMETHODCALLTYPE EseRecoveryWriter::Initialize()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;

    if ( !m_fVssInitialized )
    {
        CallHr( __super::Initialize(
            EseRecoveryWriterId,        // WriterID
            EseRecoveryWriterName,          // wszWriterName
            VSS_UT_USERDATA,        // ut
            VSS_ST_OTHER              // st
        ) );
        m_fVssInitialized = true;
    }

    // subscribe for events
    if ( !m_fVssSubscribed )
    {
        CallHr( Subscribe() );
        m_fVssSubscribed = true;
    }

Cleanup:
    DBGV( wprintf( L"Leaving %hs (returning %#x).\n", __FUNCTION__, hr ) );
    return hr;
}

// Initialize the writer
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

// OnIdentify is called as a result of the requestor calling GatherWriterMetadata
bool STDMETHODCALLTYPE EseRecoveryWriter::OnIdentify(
    _In_ IVssCreateWriterMetadata *pMetadata
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr;
    bool filesToAdd = true;
    PCWSTR szComponent = L"FilesComponent";

    //
    //  Set the backup schema
    // REVIEW: What should it be? Just VSS_BS_COPY?
    //
    const DWORD dwBackupSchema = VSS_BS_COPY
        | VSS_BS_DIFFERENTIAL
        | VSS_BS_INCREMENTAL
        | VSS_BS_EXCLUSIVE_INCREMENTAL_DIFFERENTIAL
        | VSS_BS_WRITER_SUPPORTS_NEW_TARGET
        | VSS_BS_WRITER_SUPPORTS_RESTORE_WITH_MOVE
        | VSS_BS_WRITER_SUPPORTS_PARALLEL_RESTORES;

    CallHr( pMetadata->SetBackupSchema( dwBackupSchema ) );

    // Set the restore method to restore if can replace
    // REVIEW: Should it be something else?
    CallHr( pMetadata->SetRestoreMethod(
        VSS_RME_RESTORE_IF_CAN_REPLACE,
        NULL,
        NULL,
        VSS_WRE_ALWAYS,
        false ) );


    // add simple FileGroup component
    CallHr( pMetadata->AddComponent(
        VSS_CT_FILEGROUP,
        NULL,       // logical path
        szComponent,
        L"recovered_database",  // caption
        NULL,       // pbIcon
        0,          // cbIcon
        false,      // bRestoreMetadata
        true,       // bNotifyOnBackupComplete
        true,       // bSelectable
        true,       // bSelectableForRestore
        VSS_CF_BACKUP_RECOVERY | VSS_CF_APP_ROLLBACK_RECOVERY   // dwComponentFlags
    ) );

    if ( g_eseRecoveryWriterConfig.m_szDatabaseFileFullPath == NULL )
    {
        DBGV( wprintf( L"Warning: g_eseRecoveryWriterConfig->m_szDatabaseFileFullPath is NULL.\n" ) );
        filesToAdd = false;
    }

    if ( filesToAdd )
    {
        DBGV( wprintf( L"Adding directory '%s' (file=*) to the file group\n", g_eseRecoveryWriterConfig.m_szDatabasePath ) );

        // add the files to the group   
        CallHr( pMetadata->AddFilesToFileGroup(
            NULL,
            szComponent,
            g_eseRecoveryWriterConfig.m_szDatabasePath, // path
            L"*",   // filespec, all files
            false,  // recursive
            NULL    // alternate location
            // dwBackupTypeMask (VSS_FSBT_ALL_BACKUP_REQUIRED | VSS_FSBT_ALL_SNAPSHOT_REQUIRED)
        ) );

        if ( g_eseRecoveryWriterConfig.m_szLogDirectory != NULL )
        {
            // No sense adding it to the set if it's in the same location.
            if ( _wcsicmp( g_eseRecoveryWriterConfig.m_szDatabasePath, g_eseRecoveryWriterConfig.m_szLogDirectory ) != 0 )
            {
                DBGV( wprintf( L"Adding '%s' to the file group\n", g_eseRecoveryWriterConfig.m_szLogDirectory ) );

                CallHr( pMetadata->AddFilesToFileGroup(
                    NULL,
                    szComponent,
                    g_eseRecoveryWriterConfig.m_szLogDirectory,
                    L"*",   // all files
                    false,
                    NULL
                    // dwBackupTypeMask (VSS_FSBT_ALL_BACKUP_REQUIRED | VSS_FSBT_ALL_SNAPSHOT_REQUIRED)
                ) );
            }
        }

        if ( g_eseRecoveryWriterConfig.m_szSystemDirectory != NULL )
        {
            // No sense adding it to the set if it's in the same location.
            if ( _wcsicmp( g_eseRecoveryWriterConfig.m_szDatabasePath, g_eseRecoveryWriterConfig.m_szSystemDirectory ) != 0 &&
                    _wcsicmp( g_eseRecoveryWriterConfig.m_szLogDirectory, g_eseRecoveryWriterConfig.m_szSystemDirectory ) != 0 )
            {
                DBGV( wprintf( L"Adding '%s' to the file group\n", g_eseRecoveryWriterConfig.m_szSystemDirectory ) );

                CallHr( pMetadata->AddFilesToFileGroup(
                    NULL,
                    szComponent,
                    g_eseRecoveryWriterConfig.m_szSystemDirectory,
                    L"*",   // all files
                    false,
                    NULL
                    // dwBackupTypeMask (VSS_FSBT_ALL_BACKUP_REQUIRED | VSS_FSBT_ALL_SNAPSHOT_REQUIRED)
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

// This function is called as a result of the requestor calling PrepareForBackup
// this indicates to the writer that a backup sequence is being initiated
bool STDMETHODCALLTYPE EseRecoveryWriter::OnPrepareBackup(
    _In_ IVssWriterComponents*
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called after a requestor calls DoSnapshotSet
// time-consuming actions related to Freeze can be performed here
bool STDMETHODCALLTYPE EseRecoveryWriter::OnPrepareSnapshot()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called after a requestor calls DoSnapshotSet
// here the writer is expected to freeze its store
bool STDMETHODCALLTYPE EseRecoveryWriter::OnFreeze()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called after a requestor calls DoSnapshotSet
// here the writer is expected to thaw its store
bool STDMETHODCALLTYPE EseRecoveryWriter::OnThaw()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called after a requestor calls DoSnapshotSet
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

    // First, let's figure out if this is an auto-recovery snapshot
    if ( ( __super::GetContext() & VSS_VOLSNAP_ATTR_AUTORECOVER ) == 0 )
    {
        // this is not an auto-recovery snapshot, so we won't be able to do anything with it.
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
    WCHAR szOutPaths[ _countof( szInPaths ) ][ MAX_PATH ];
    WCHAR szRelativePathFromRoot[ _MAX_PATH ];
    WCHAR szLogPath[ _MAX_PATH ];
    WCHAR szSystemPath[ _MAX_PATH ];

    // convert each path to a corresponding snapshot device path
    for ( size_t i = 0; i < _countof( szInPaths ); i++ )
    {
        WCHAR szVolume[ _MAX_PATH ];
        PCWSTR szDeviceName = NULL;
        PCWSTR szInPath = szInPaths[ i ];

        // Log and system directories are optional, after all
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

        // REVIEW: Does szDeviceName need to be freed?
        CallHr( GetSnapshotDeviceName( szVolume, &szDeviceName ) );
        
        // Now we need to tell the caller where in the mounted path to look.
        hfile = CreateFileW(
            szInPath,
            STANDARD_RIGHTS_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,     // Allows opening a directory
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
        if ( !fRc )
        {
            const DWORD dwGle = GetLastError();
            DBGV( wprintf( L"Failed GetFinalPathNameByHandleW( %s ): returned %d.\n", szInPath, dwGle ) );
            hr = HRESULT_FROM_WIN32( dwGle );
            goto Cleanup;
        }

        // now we can construct the full path
        CallHr( StringCchPrintfW( szOutPaths[ i ], _countof( szOutPaths[ i ]), L"%s%s", szDeviceName, szRelativePathFromRoot ) );
    }

    if ( g_eseRecoveryWriterConfig.m_szLogDirectory == NULL )
    {
        // If the log path wasn't specified, then copy the database full path, and strip off everything
        // after the last '\'.
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
        // If the system path wasn't specified, then copy the log full path.
        StringCchCopyW( szSystemPath, _countof( szLogPath ), szLogPath );
    }
    else
    {
        StringCchCopyW( szSystemPath, _countof( szSystemPath ), szOutPaths[ 2 ] );
    }

    // ok, now we have the transformed database, log and system paths
    DBGV( wprintf( L"Snapshot database file: %s\n", szOutPaths[ 0 ] ) );
    DBGV( wprintf( L"Snapshot log directory: %s\n", szLogPath ) );
    DBGV( wprintf( L"Snapshot system directory: %s\n", szSystemPath ) );

    if ( LOSStrLengthW( g_eseRecoveryWriterConfig.m_szEseBaseName ) > 0 )
    {
        hr = RecoverEseDatabase( szInPaths[ 0 ], szOutPaths[ 0 ], szLogPath, szSystemPath );
        if ( FAILED( hr ) )
        {
            // error already logged
            goto Cleanup;
        }
    }

    // cool! We are done now.

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

// This function is called to abort the writer's backup sequence.
// This should only be called between OnPrepareBackup and OnPostSnapshot
bool STDMETHODCALLTYPE EseRecoveryWriter::OnAbort()
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called as a result of the requestor calling BackupComplete
bool STDMETHODCALLTYPE EseRecoveryWriter::OnBackupComplete(
    _In_ IVssWriterComponents*
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called at the end of the backup process.  This may happen as a result
// of the requestor shutting down, or it may happen as a result of abnormal termination 
// of the requestor.
bool STDMETHODCALLTYPE EseRecoveryWriter::OnBackupShutdown(
    _In_ VSS_ID
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called as a result of the requestor calling PreRestore
// This will be called immediately before files are restored
bool STDMETHODCALLTYPE EseRecoveryWriter::OnPreRestore(
    _In_ IVssWriterComponents *
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

// This function is called as a result of the requestor calling PreRestore
// This will be called immediately after files are restored
bool STDMETHODCALLTYPE EseRecoveryWriter::OnPostRestore(
    _In_ IVssWriterComponents*
)
{
    DBGV( wprintf( L"Entering %hs (and returning true).\n", __FUNCTION__ ) );
    return true;
}

#pragma prefast(pop) // SDK headers.


HRESULT
EseRecoveryWriter::RecoverEseDatabase(
    _In_ PCWSTR szOldDbName,
    _In_ PCWSTR szNewDbName,
    _In_ PCWSTR szLogPath,
    _In_ PCWSTR szSystemPath
)
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    JET_ERR                 err                     = JET_errSuccess;
    IFileSystemAPI*         pfsapi                  = NULL;
    IFileAPI*               pfapi                   = NULL;
    JET_INSTANCE            instance                = 0;
    WCHAR                   wszDefaultTempDB[MAX_PATH];
    WCHAR                   wszTempPath[MAX_PATH];

    JET_RSTINFO_W           rstInfo                 = { 0 };
    JET_RSTMAP_W            rstMap                  = { 0 };
    HRESULT                 hr                      = S_OK;

    CallEse( ErrOSFSCreate( &pfsapi ) );

    CallEse( pfsapi->ErrGetTempFolder( wszTempPath, _countof( wszTempPath ) ) );
    CallEse( pfsapi->ErrGetTempFileName( wszTempPath, L"ewe", wszDefaultTempDB ) );

    // turn on legacy config if implemented (or use fast config if ever created)
    JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, wszDefaultTempDB );

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
        //  Set file size.
        JetSetSystemParameterW( &instance, 0, JET_paramLogFileSize, cbLogFileSize / 1024, NULL );

        // Due to implementation details in ESE, we need to make sure that a <basename>tmp.<log-extension> exists
        // if we are recovering with the possibility of lost logs.
        if ( g_eseRecoveryWriterConfig.m_fIgnoreLostLogs )
        {
            WCHAR szTempLogFileName[MAX_PATH] = { L'\0' };
            StringCchPrintfW( szTempLogFileName, _countof( szTempLogFileName ), L"%ls\\%lstmp.%ls", szLogPath, g_eseRecoveryWriterConfig.m_szEseBaseName, szLogExtension );

            err = pfsapi->ErrFileCreate( szTempLogFileName, IFileAPI::fmfNone, &pfapi );
            if ( err < JET_errSuccess && err != JET_errFileAlreadyExists )
            {
                DBGV( wprintf( L"Failed to create temporary log file %s: Error %d.\n", szTempLogFileName, err ) );
            }
            delete pfapi;
            pfapi = NULL;
        }
    }

    rstInfo.cbStruct = sizeof( rstInfo );
    rstInfo.crstmap = 1;
    rstInfo.rgrstmap = &rstMap;

    rstMap.szDatabaseName = const_cast<WCHAR*>( szOldDbName );
    rstMap.szNewDatabaseName = const_cast<WCHAR*>( szNewDbName );

    // Soft recovery.
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
    if ( instance != 0 )
    {
        JetTerm2( instance, SUCCEEDED(hr) ? JET_bitTermComplete : JET_bitTermAbrupt );
    }

    delete pfapi;
    delete pfsapi;

    if ( FAILED( hr ) )
    {
        __super::SetWriterFailure( hr );
    }

    return hr;
}

// static functions.

/*++

Routine Description:

    This function creates the Exchange Writer object.

Arguments:

    None

Return Value:

    hr

--*/
HRESULT
EseRecoveryWriter::CreateWriter()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
    HRESULT hr = S_OK;
    
//  ec = InitializeEventLogging();
//  if ( ecNone != ec)
//  {
//      // No event logging is possible here.
//      goto Error;
//  }

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

/*++

Routine Description:

    This function destroys the exchange writer object.

Arguments:

    None

Return Value:

    hr

--*/
VOID
EseRecoveryWriter::DestroyWriter()
{
    DBGV( wprintf( L"Entering %hs.\n", __FUNCTION__ ) );
//  EseRecoveryWriter::UninitializeEventLogging();
    EseRecoveryWriter::DeleteSingleton();
    DBGV( wprintf( L"Leaving %hs.\n", __FUNCTION__ ) );
}




//
//  DLL Main Function
//
extern BOOL FOSPreinit();
extern void OSPostterm();

extern "C" {

BOOL
WINAPI
DllMain(
        HINSTANCE hinstDll,
        DWORD dwReason,
        LPVOID /*pvReserved*/
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

        if ( ! FOSPreinit() )
        {
            fReturn = FALSE;
        }

        if ( fReturn && S_OK != EseRecoveryWriter::CreateWriter() )
        {
            fReturn = FALSE;
        }
    }
    else if ( dwReason == DLL_PROCESS_DETACH ) // On all detaches.
    {
        EseRecoveryWriter::DestroyWriter();

        InterlockedDecrement( &g_lRefEseWriter );

        //  terminate OS Layer
        OSPostterm();
    }

    DBGV( wprintf( L"Leaving %hs (returning %d).\n", __FUNCTION__, fReturn ) );
    return fReturn;

}

} // extern "C"
