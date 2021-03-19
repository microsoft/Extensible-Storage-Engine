// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
//  OS File System and related support ...
//
//

//  This file contains 3 basic objects / sub-systems.
//
//      OS File System
//      OS File Find
//      OS Volume (global)
//

#include "osstd.hxx"
#include <winioctl.h>
#include <errno.h>

// Prototypes of functions used in this file.
VOID CalculateCurrentProcessIsPackaged();
LOCAL ERR ErrOSFSInitDefaultPath();

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsVolumeFlagsInformation,// 10
    FileFsSectorSizeInformation, // 11
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _FILE_FS_SECTOR_SIZE_INFORMATION {
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_FS_SECTOR_SIZE_INFORMATION, *PFILE_FS_SECTOR_SIZE_INFORMATION;

typedef __success(return >= 0) LONG NTSTATUS;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
} DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

__kernel_entry NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile (
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FS_INFORMATION_CLASS FsInformationClass
    );


DWORD g_cbAtomicOverride = 0;

#ifdef DEBUG
//
//  This leaves enough from for a max path to be set while in the debugger, or
//  via the registry "OSPath Trap" in DEBUG section.
//  Note: This does substring matching, so for instance ".edb" would catch
//  databases.
//  BTW, it is amazing how much this goes off, perhaps we should remove it from
//  ErrPathComplete()/ErrPathBuild()/ErrPathParse().  Or make ourselves a little
//  more efficient.
//
WCHAR g_rgwchTrapOSPath[_MAX_PATH] = L"";

#define OSTrapPath( wsz )   \
                if( g_rgwchTrapOSPath[0] != L'\0' && \
                    wsz != NULL && \
                    wsz[0] != L'\0' && \
                    wcsstr( wsz, g_rgwchTrapOSPath ) ) \
                { \
                    AssertSz( !"OS layer path trap." , __FUNCTION__ ); \
                }
#else
#define OSTrapPath( wsz )
#endif

OSFSRETRY::OSFSRETRY( IFileSystemConfiguration* const pfsconfig )
    :   m_pfsconfig( pfsconfig ),
        m_fInitialized( fFalse )
{
}

BOOL OSFSRETRY::FRetry( const JET_ERR err )
{
    ULONG cmsecAccessDeniedRetryPeriod = m_pfsconfig->DtickAccessDeniedRetryPeriod();

    if ( JET_errFileAccessDenied == err && cmsecAccessDeniedRetryPeriod )
    {
        // initialize on first retry
        if ( !m_fInitialized )
        {
            m_fInitialized = fTrue;
            m_tickEnd = TickOSTimeCurrent() + cmsecAccessDeniedRetryPeriod;
        }

        // just a tick to let someone else run
        UtilSleep( 2 );

        // retry for limited time
        // Cmp order is important because m_EndTick may have wrapped
        return TickCmp( TickOSTimeCurrent(), m_tickEnd ) <= 0;
    }
    else
    {
        // no retry
        return fFalse;
    }
}


#if defined( USE_HAPUBLISH_API )
HaDbFailureTag OSFSHaTagOfErr( const ERR err )
{
    switch ( err )
    {
        case JET_errOutOfMemory:
            return HaDbFailureTagMemory;

        case JET_errDiskReadVerificationFailure:
            return HaDbFailureTagRepairable;

        case JET_errDiskFull:
            return HaDbFailureTagSpace;

        case JET_errFileAccessDenied:
            return HaDbFailureTagRemount;

        case JET_errFileSystemCorruption:
            return HaDbFailureTagFileSystemCorruption;

        default:
            return HaDbFailureTagIoHard;
    }
}
#endif

VOID COSFileSystem::ReportDiskError(
    const MessageId         msgid,
    const WCHAR * const     wszPath,
    const WCHAR *           wszAbsRootPath,
    JET_ERR                 err,
    const DWORD             error )
{
    const WCHAR*            rgpwsz[ 5 ];
    DWORD                   irgpwsz                     = 0;
    WCHAR                   wszAbsPath[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszError[ 64 ];
    WCHAR                   wszSystemError[ 64 ];
    WCHAR*                  wszSystemErrorDescription   = NULL;

    if ( ErrPathComplete( wszPath, wszAbsPath ) < JET_errSuccess )
    {
        OSStrCbCopyW(wszAbsPath, sizeof( wszAbsPath ), wszPath);
    }

    OSStrCbFormatW( wszError, sizeof( wszError ), L"%i (0x%08x)", err, err );

    OSStrCbFormatW( wszSystemError, sizeof( wszSystemError ), L"%u (0x%08x)", error, error );

    FormatMessageW( (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                    NULL,
                    error,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                    LPWSTR( &wszSystemErrorDescription ),
                    0,
                    NULL );

    rgpwsz[ irgpwsz++ ]   = wszAbsRootPath;
    rgpwsz[ irgpwsz++ ]   = wszAbsPath;
    rgpwsz[ irgpwsz++ ]   = wszError;
    rgpwsz[ irgpwsz++ ]   = wszSystemError;
    rgpwsz[ irgpwsz++ ]   = wszSystemErrorDescription ? wszSystemErrorDescription : L"";

    Pfsconfig()->EmitEvent( eventError,
                            GENERAL_CATEGORY,
                            msgid,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
    Pfsconfig()->EmitEvent( OSFSHaTagOfErr( err ),
                            Ese2HaId( GENERAL_CATEGORY ),
                            Ese2HaId( msgid ),
                            irgpwsz,
                            rgpwsz,
                            HaDbIoErrorMeta,
                            wszAbsPath );
#endif

    LocalFree( wszSystemErrorDescription );
}

VOID COSFileSystem::ReportFileErrorInternal(
    const MessageId         msgid,
    __in_z const WCHAR * const      wszSrcPath,
    __in_z_opt const WCHAR * const  wszDstPath,
    JET_ERR                 err,
    const DWORD             error )
{
    const WCHAR*            rgpwsz[ 5 ];
    DWORD                   irgpwsz  = 0;
    WCHAR                   wszAbsSrcPath[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszAbsDstPath[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszError[ 64 ];
    WCHAR                   wszSystemError[ 64 ];
    WCHAR*                  wszSystemErrorDescription    = NULL;

    if ( ErrPathComplete( wszSrcPath, wszAbsSrcPath ) < JET_errSuccess )
    {
        OSStrCbCopyW(wszAbsSrcPath, sizeof( wszAbsSrcPath ), wszSrcPath);
    }
    if ( (wszDstPath) && (ErrPathComplete( wszDstPath, wszAbsDstPath ) < JET_errSuccess) )
    {
        OSStrCbCopyW(wszAbsDstPath, sizeof( wszAbsDstPath ), wszDstPath);
    }

    OSStrCbFormatW( wszError, sizeof( wszError ), L"%i (0x%08x)", err, err );

    OSStrCbFormatW( wszSystemError, sizeof( wszSystemError ), L"%u (0x%08x)", error, error );

    FormatMessageW( (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                    NULL,
                    error,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                    LPWSTR( &wszSystemErrorDescription ),
                    0,
                    NULL );

    // Handles messages with and without a root path
    rgpwsz[ irgpwsz++ ]   = wszAbsSrcPath;
    if (wszDstPath)
    {
        rgpwsz[ irgpwsz++ ]   = wszAbsDstPath;
    }
    rgpwsz[ irgpwsz++ ]   = wszError;
    rgpwsz[ irgpwsz++ ]   = wszSystemError;
    rgpwsz[ irgpwsz++ ]   = wszSystemErrorDescription ? wszSystemErrorDescription : L"";

    Pfsconfig()->EmitEvent( eventError,
                            GENERAL_CATEGORY,
                            msgid,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )
    Pfsconfig()->EmitEvent( OSFSHaTagOfErr( err ),
                            Ese2HaId( GENERAL_CATEGORY ),
                            Ese2HaId( msgid ),
                            irgpwsz,
                            rgpwsz,
                            HaDbIoErrorMeta,
                            wszAbsSrcPath );
#endif

    LocalFree( wszSystemErrorDescription );

    OSDiagTrackFileSystemError( err, error );
}

VOID COSFileSystem::ReportFileError(
    const MessageId         msgid,
    __in_z const WCHAR * const      wszSrcPath,
    __in_z_opt const WCHAR * const  wszDstPath,
    JET_ERR                 err,
    const DWORD             error )
{
    if ( err < JET_errSuccess )
    {
        ReportFileErrorInternal( msgid, wszSrcPath, wszDstPath, err, error );
    }
}

VOID COSFileSystem::ReportFileErrorWithFilter(
    const MessageId         msgid,
    __in_z const WCHAR * const      wszSrcPath,
    __in_z_opt const WCHAR * const  wszDstPath,
    JET_ERR                 err,
    const DWORD             error )
{
    const DWORD rgdwIgnorableErrors[] = {
        ERROR_FILE_NOT_FOUND,
        ERROR_PATH_NOT_FOUND,
        // add new filtered error code here
    };

    for( INT idwError = 0; idwError < _countof( rgdwIgnorableErrors ); ++idwError )
    {
        if( error == rgdwIgnorableErrors[ idwError ] )
        {
            return;
        }
    }
    
    ReportFileError( msgid, wszSrcPath, wszDstPath, err, error );
}

COSFileSystem::COSFileSystem( IFileSystemConfiguration * const pfsconfig )
    :   m_pfsconfig( pfsconfig ),
        m_critVolumePathCache( CLockBasicInfo( CSyncBasicInfo( _T( "COSFileSystem::m_critVolumePathCache" ) ), 0, 0 ) )
{
}

NTOSFuncNtStd( g_pfnNtQueryVolumeInformationFile, g_mwszzNtdllLibs, NtQueryVolumeInformationFile, oslfExpectedOnWin5x );

ERR COSFileSystem::ErrGetLastError( const DWORD error )
{
    //  map Win32 errors to JET API errors
    return ErrOSErrFromWin32Err( error, JET_errDiskIO );
}

COSFileSystem::CVolumePathCacheEntry * COSFileSystem::GetVolInfoCacheEntry( _In_ PCWSTR wszTargetPath )
{
    CVolumePathCacheEntry*  pvpce       = NULL;

    Assert( m_critVolumePathCache.FOwner() );

    for ( pvpce = m_ilVolumePathCache.PrevMost(); pvpce != NULL; pvpce = m_ilVolumePathCache.Next( pvpce ) )
    {
        if ( !LOSStrCompareW( pvpce->m_wszKeyPath, wszTargetPath ) )
        {
            return pvpce;
        }
    }
    return NULL;
}

ERR COSFileSystem::ErrPathRoot( const WCHAR* const  wszPath,
    // UNDONE_BANAPI:
                                __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszAbsRootPath )
{
    ERR                     err             = JET_errSuccess;
    WCHAR                   wszAbsPath[ IFileSystemAPI::cchPathMax ];
    DWORD                   dwAttr          = INVALID_FILE_ATTRIBUTES;
    HANDLE                  hFile           = INVALID_HANDLE_VALUE;
    WCHAR*                  wszAbsPathT     = wszAbsPath;
    DWORD                   cwchAbsPathT    = 0;
    WCHAR                   wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszFileBase[ IFileSystemAPI::cchPathMax ];
    WCHAR                   wszFileExt[ IFileSystemAPI::cchPathMax ];
    CVolumePathCacheEntry*  pvpce           = NULL;
    WCHAR                   wszRootPath[ IFileSystemAPI::cchPathMax ];

    //  get the absolute path for the given path

    Call( ErrPathComplete( wszPath, wszAbsPath ) );

    //  if the absolute path is a reparse point then it may be a symbolic link to another volume

    dwAttr = GetFileAttributesW( wszAbsPath );
    if ( dwAttr == INVALID_FILE_ATTRIBUTES )
    {
        err = ErrGetLastError( GetLastError() );
        Call( err == JET_errFileNotFound || err == JET_errInvalidPath ? JET_errSuccess : err );
    }

    if ( dwAttr != INVALID_FILE_ATTRIBUTES && !!( dwAttr & FILE_ATTRIBUTE_REPARSE_POINT ) )
    {
        hFile = CreateFileW(    wszAbsPath,
                                0,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            err = ErrGetLastError( GetLastError() );
            Call( err == JET_errFileNotFound || err == JET_errInvalidPath ? JET_errSuccess : err );
        }

        if ( hFile != INVALID_HANDLE_VALUE )
        {
            cwchAbsPathT = GetFinalPathNameByHandleW( hFile, wszAbsPathT, _countof( wszAbsPath ), VOLUME_NAME_DOS );
            if ( cwchAbsPathT == 0 )
            {
                Call( ErrGetLastError() );
            }
            else if ( cwchAbsPathT >= _countof( wszAbsPath ) )
            {
                Alloc( wszAbsPathT = new WCHAR[cwchAbsPathT] );
                if ( !GetFinalPathNameByHandleW( hFile, wszAbsPathT, _countof( wszAbsPath ), VOLUME_NAME_DOS ) )
                {
                    Call( ErrGetLastError() );
                }
            }

            const WCHAR* const wszPrefix = L"\\\\?\\";
            DWORD cwchOffset = 0;
            if ( wcsncmp( wszAbsPathT, wszPrefix, LOSStrLengthW( wszPrefix ) ) == 0 )
            {
                cwchOffset += LOSStrLengthW( wszPrefix );
            }

            if ( wmemmove_s( wszAbsPath, _countof( wszAbsPath ), wszAbsPathT + cwchOffset, cwchAbsPathT - cwchOffset ) )
            {
                Call( ErrERRCheck( JET_errInvalidPath ) );
            }

            CloseHandle( hFile );
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    //  get the folder of the absolute path

    Call( ErrPathParse( wszAbsPath, wszFolder, wszFileBase, wszFileExt ) );

    //  GetVolumePathName is *really* expensive, so we will use a cache of
    //  folder names to volume paths to avoid the call
    //
    //  NOTE:  we expire entries after a short period in an attempt to react
    //  to dynamic volume path changes.  it isn't perfect but it is reasonable

    m_critVolumePathCache.Enter();
    pvpce = GetVolInfoCacheEntry( wszFolder );
    if ( pvpce != NULL && pvpce->FGetVolPath( wszAbsRootPath, cbOSFSAPI_MAX_PATHW ) )
    {
        m_critVolumePathCache.Leave();
        Error( JET_errSuccess );
    }
    m_critVolumePathCache.Leave();

    //  compute the root path in this absolute path

    if ( !GetVolumePathNameW( wszAbsPath, wszRootPath, IFileSystemAPI::cchPathMax ) )
    {
        //   if we encounter ERROR_INVALID_NAME for the GetVolumePathName
        
        //   call above, this is really an invalid path.
        
        if ( GetLastError() == ERROR_INVALID_NAME )
        {
            Error( ErrERRCheck( JET_errInvalidPath ) );
        }
        
        Call( ErrGetLastError() );
    }

    //  compute the absolute path for the root path

    Call( ErrPathComplete( wszRootPath, wszAbsRootPath ) );

    //  populate the folder to volume path cache with the new result

    m_critVolumePathCache.Enter();
    pvpce = GetVolInfoCacheEntry( wszFolder );
    if ( pvpce == NULL )
    {
        //  if we fail to allocate then we will simply not cache the result
        //
        if ( pvpce = new CVolumePathCacheEntry( wszFolder ) )
        {
            m_ilVolumePathCache.InsertAsPrevMost( pvpce );
        }
    }
    if ( pvpce != NULL )
    {
        pvpce->SetVolPath( wszAbsRootPath );
    }
    m_critVolumePathCache.Leave();

HandleError:
    if ( err < JET_errSuccess )
    {
        OSStrCbCopyW( wszAbsRootPath, cbOSFSAPI_MAX_PATHW, L"" );
    }
    if ( wszAbsPathT != wszAbsPath )
    {
        delete[] wszAbsPathT;
    }
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    return err;
}

void COSFileSystem::PathVolumeCanonicalAndDiskId(   const WCHAR* const wszAbsRootPath,
                                                    __out_ecount(cchVolumeCanonicalPath) WCHAR* const wszVolumeCanonicalPath,
                                                    _In_ const DWORD cchVolumeCanonicalPath,
                                                    __out_ecount(cchDiskId) WCHAR* const wszDiskId,
                                                    _In_ const DWORD cchDiskId,
                                                    _Out_ DWORD *pdwDiskNumber )
{
    Assert( wszAbsRootPath != NULL );

    //  For NTFS, try to use the volume GUID, fallback to the regular root path if it can't retrieve it.

    const WCHAR * wszVolumeCanonicalPathToUse = wszAbsRootPath;
    const WCHAR * wszDiskIdToUse = wszAbsRootPath;

    //  Try the cached paths first.

    BOOL fGetVolCanonicalPath = fFalse;
    BOOL fGetDiskId = fFalse;

    m_critVolumePathCache.Enter();
    CVolumePathCacheEntry* pvpce = GetVolInfoCacheEntry( wszAbsRootPath );
    if ( pvpce == NULL ||
        !( fGetVolCanonicalPath = pvpce->FGetVolCanonicalPath( wszVolumeCanonicalPath, sizeof( WCHAR ) * cchVolumeCanonicalPath ) ) ||
        !( fGetDiskId = pvpce->FGetDiskId( wszDiskId, sizeof( WCHAR ) * cchDiskId ) ) )
    {
        m_critVolumePathCache.Leave();

        //  Call the expensive code, if needed.

        if ( fGetVolCanonicalPath )
        {
            wszVolumeCanonicalPathToUse = wszVolumeCanonicalPath;
        }
        else
        {
            if ( ErrPathVolumeCanonical( wszAbsRootPath, wszVolumeCanonicalPath, cchVolumeCanonicalPath ) == JET_errSuccess )
            {
                wszVolumeCanonicalPathToUse = wszVolumeCanonicalPath;
            }
        }

        if ( fGetDiskId )
        {
            wszDiskIdToUse = wszDiskId;
            *pdwDiskNumber = pvpce->DwDiskNumber();
        }
        else
        {
            if ( ErrDiskId( wszVolumeCanonicalPathToUse, wszDiskId, cchDiskId, pdwDiskNumber ) == JET_errSuccess )
            {
                wszDiskIdToUse = wszDiskId;
            }
        }

        //  Enter the lock again. Pehaps someone has just inserted it.

        m_critVolumePathCache.Enter();
        pvpce = GetVolInfoCacheEntry( wszAbsRootPath );
        if ( pvpce == NULL )
        {
            pvpce = new CVolumePathCacheEntry( wszAbsRootPath );
            if ( pvpce != NULL )
            {
                m_ilVolumePathCache.InsertAsPrevMost( pvpce );
            }
        }

        if ( pvpce != NULL )
        {
            //  We'll override it if it's already there. Treat it as we are refreshing with more recent
            //  information, although we don't expect this to change, except in cases where we've previously
            //  failed in retrieving this information due to, for example, out of memory conditions and
            //  ended up falling back to the default vanilla volume path.
            pvpce->SetVolCanonicalPath( wszVolumeCanonicalPathToUse );
            pvpce->SetDiskId( wszDiskIdToUse );
            pvpce->SetDiskNumber( *pdwDiskNumber );
        }
    }
    else
    {
        wszVolumeCanonicalPathToUse = wszVolumeCanonicalPath;
        wszDiskIdToUse = wszDiskId;
        *pdwDiskNumber = pvpce->DwDiskNumber();
    }
    m_critVolumePathCache.Leave();

    //  We may need to copy to the buffer.

    Assert( wszVolumeCanonicalPathToUse == wszAbsRootPath || wszVolumeCanonicalPathToUse == wszVolumeCanonicalPath );
    Assert( wszDiskIdToUse == wszAbsRootPath || wszDiskIdToUse == wszDiskId );
    if ( wszVolumeCanonicalPathToUse == wszAbsRootPath )
    {
        OSStrCbCopyW( wszVolumeCanonicalPath, sizeof( WCHAR ) * cchVolumeCanonicalPath, wszAbsRootPath );
    }
    if ( wszDiskIdToUse == wszAbsRootPath )
    {
        OSStrCbCopyW( wszDiskId, sizeof( WCHAR ) * cchDiskId, wszAbsRootPath );
    }
}

//  CONSIDER:  make this function even cheaper
//
//  We talked to NTFS and they said that this actually flushes the NTFS Journal twice.
//  We can make it flush just once by simply calling SetFileTime on a file opened with
//  FILE_FLAG_WRITE_THROUGH and with GENERIC_WRITE access.  We should set the Create Time
//  to a unique value each time (using GetSystemTimeAsFileTime())
//  What about ReFS?
//
ERR COSFileSystem::ErrCommitFSChange( const WCHAR* const wszPath )
{
    ERR         err                         = JET_errSuccess;
    WCHAR       wszAbsPath[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszFileBase[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszFileExt[ IFileSystemAPI::cchPathMax ];
    WCHAR       wszFlushFile[ IFileSystemAPI::cchPathMax ];
    INT         iRetry                      = 0;
    const INT   cRetry                      = 100;
    HANDLE      hFlushFile                  = INVALID_HANDLE_VALUE;


    //  create a dummy file using write through to force NTFS to flush its journal.
    //  this will force any previous file meta-data operations to be committed

    Call( ErrPathComplete( wszPath, wszAbsPath ) );
    Call( ErrPathParse( wszAbsPath, wszFolder, wszFileBase, wszFileExt ) );
    Call( ErrPathBuild( wszFolder, L"$FlushFS", L"Now", wszFlushFile, sizeof( wszFlushFile ) ) );

    for ( iRetry = 0; iRetry < cRetry; iRetry++ )
    {
        if ( iRetry > 0 )
        {
            UtilSleep( 2 );
        }
        hFlushFile = CreateFileW(   wszFlushFile,
                                    0,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL,
                                    CREATE_ALWAYS,
                                    (   FILE_ATTRIBUTE_HIDDEN |
                                        FILE_ATTRIBUTE_SYSTEM |
                                        FILE_ATTRIBUTE_TEMPORARY |
                                        FILE_FLAG_DELETE_ON_CLOSE |
                                        FILE_FLAG_WRITE_THROUGH |
                                        FILE_FLAG_NO_BUFFERING ),
                                    NULL );
        err = ErrGetLastError();
        if ( hFlushFile != INVALID_HANDLE_VALUE ||
                err != JET_errFileAccessDenied )
        {
            //  Either we succeeded or we failed with an error that does not
            //  encourage retry
            break;
        }
    }

    //  if for some reason we were not able to create the dummy file then we
    //  will fallback to the old method which is much slower but always works

    if ( hFlushFile == INVALID_HANDLE_VALUE )
    {
        Call( ErrCommitFSChangeSlowly( wszAbsPath ) );
    }

HandleError:
    if ( hFlushFile && hFlushFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFlushFile );
    }
    return err;
}

COSFileSystem::~COSFileSystem()
{
    //  free our Volume Path Cache

    m_critVolumePathCache.Enter();
    while ( !m_ilVolumePathCache.FEmpty() )
    {
        CVolumePathCacheEntry* const pvpce = m_ilVolumePathCache.PrevMost();

        m_ilVolumePathCache.Remove( pvpce );
        delete pvpce;
    }
    m_critVolumePathCache.Leave();
}

ERR COSFileSystem::ErrDiskSpace(
    const WCHAR * const wszPath,
    QWORD * const           pcbFreeForUser,
    QWORD * const           pcbTotalForUser,
    QWORD * const           pcbFreeOnDisk )

{
    ERR     err     = JET_errSuccess;
    DWORD   error   = ERROR_SUCCESS;
    WCHAR   wszAbsRootPath[ IFileSystemAPI::cchPathMax ];

    //  get the root path for the specified path

    Call( ErrPathRoot( wszPath, wszAbsRootPath ) );

    //  RFS:  bad path

    if ( !RFSAlloc( OSFileDiskSpace ) )
    {
        error = ERROR_PATH_NOT_FOUND;
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }

    //  get the sector size for the root path

    if ( !GetDiskFreeSpaceExW( wszAbsRootPath, (PULARGE_INTEGER)pcbFreeForUser, (PULARGE_INTEGER)pcbTotalForUser, (PULARGE_INTEGER)pcbFreeOnDisk ) )
    {
        error = GetLastError();
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }

HandleWin32Error:
    if ( err < JET_errSuccess )
    {
        ReportDiskError( OSFS_DISK_SPACE_ERROR_ID, wszPath, wszAbsRootPath, err, error );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( NULL != pcbFreeForUser )
            *pcbFreeForUser = 0;
        if ( NULL != pcbTotalForUser )
            *pcbTotalForUser = 0;
        if ( NULL != pcbFreeOnDisk )
            *pcbFreeOnDisk = 0;
    }
    return err;
}

ERR COSFileSystem::ErrOSFSGetDeviceHandle(
    _In_    PCWSTR  wszAbsVolumeRootPath,
    _Out_   HANDLE* phDevice )
{
    ERR     err     = JET_errSuccess;
    DWORD   error   = ERROR_SUCCESS;
    const WCHAR wszDevPrefix [] = L"\\\\.\\";
    WCHAR   wszDeviceName[ IFileSystemAPI::cchPathMax + 4 ]; // 4 = LOSStrLengthW( wszDevPrefix )
    wszDeviceName[0] = L'\0';

    Assert( phDevice );
    *phDevice = INVALID_HANDLE_VALUE;

#if 0
{
    //  caller should've provided the absolute root path of a drive/volume.
    WCHAR wszAbsVolumeRootPathCheck[ IFileSystemAPI::cchPathMax ] = L"";
    if ( JET_errSuccess == ErrPathRoot( wszAbsVolumeRootPath, wszAbsVolumeRootPathCheck ) )
    {
        Assert( 0 == _wcsicmp( wszAbsVolumeRootPath, wszAbsVolumeRootPathCheck ) );
    }
}
#endif

    //
    //  The device name is different from the absolute root path of a drive/volume...
    //

    if ( !GetVolumeNameForVolumeMountPointW( wszAbsVolumeRootPath,
                    wszDeviceName,
                    sizeof(wszDeviceName)/sizeof(wszDeviceName[0]) ) )
    {
        error   = GetLastError();
        Assert( error );
        err = ErrGetLastError( error );
    }
    else
    {
        error = ERROR_SUCCESS;
        err = JET_errSuccess;
    }

    //  possibly fall back to manual path munging

    if ( error != ERROR_SUCCESS )
    {
        //  No Win32 API present or we failed with volume API, attempt manual path munging...
        if( wszAbsVolumeRootPath[0] == L'\\' &&
            wszAbsVolumeRootPath[1] == L'\\' )
        {
            // This handles \\?\Volume{guid}\xxx type paths.
            OSStrCbCopyW( wszDeviceName, sizeof(wszDeviceName), wszAbsVolumeRootPath );
        }
        else
        {
            OSStrCbCopyW( wszDeviceName, sizeof(wszDeviceName), wszDevPrefix );
            OSStrCbAppendW( wszDeviceName, sizeof(wszDeviceName), wszAbsVolumeRootPath );
        }
        err = JET_errSuccess;
    }

    //  Whether calculated w/ GetVolumeNameForVolumeMountPoint() or manually, CreatFileW()
    //  expects the trailing backslash to be removed.
    ULONG cchDeviceName = LOSStrLengthW(wszDeviceName);
    if ( wszDeviceName[cchDeviceName-1] == L'\\' )
    {
        wszDeviceName[cchDeviceName-1] = L'\0';
        Assert( wszDeviceName[LOSStrLengthW(wszDeviceName)-1] != L'\\' );
    }

    //
    //  Open the device handle.
    //

    *phDevice = CreateFileW( wszDeviceName,
                            FILE_READ_ATTRIBUTES,
                            FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            (   FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_BACKUP_SEMANTICS  ),
                            NULL);
    // note: no retry, can't imagine anti-virus can cause us trouble opening up 
    // the disk device itself, like all the other file operations ...
    if ( *phDevice == INVALID_HANDLE_VALUE )
    {
        error   = GetLastError();
        Assert( error );
        Call( ErrGetLastError( error ) );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        ReportDiskError( OSFS_OPEN_DEVICE_ERROR_ID, wszAbsVolumeRootPath, wszDeviceName, err, error );
    }

    return(err);
}


ERR COSFileSystem::ErrFileSectorSize(   const WCHAR* const  wszPath,
                                        DWORD* const        pcbSize )
{
    ERR     err     = JET_errSuccess;
    DWORD   error   = ERROR_SUCCESS;
    WCHAR   wszAbsRootPath[ IFileSystemAPI::cchPathMax ]; wszAbsRootPath[0] = L'\0';
    HANDLE  hDevice = INVALID_HANDLE_VALUE;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    CVolumePathCacheEntry*  pvpce       = NULL;

    Assert( pcbSize );
    *pcbSize = 0;

    //  get the root path for the specified path

    Call( ErrPathRoot( wszPath, wszAbsRootPath ) );

    //  RFS:  bad path

    if ( !RFSAlloc( OSFileISectorSize ) )
    {
        error = ERROR_PATH_NOT_FOUND;
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }

    m_critVolumePathCache.Enter();
    pvpce = GetVolInfoCacheEntry( wszAbsRootPath );
    if ( pvpce && pvpce->FGetSecSize( pcbSize ) )
    {
        m_critVolumePathCache.Leave();
        Error( JET_errSuccess );
    }
    m_critVolumePathCache.Leave();

    g_cbAtomicOverride = (DWORD)UlConfigOverrideInjection( 43383, g_cbAtomicOverride );

    if ( g_cbAtomicOverride != 0 )
    {
        *pcbSize = g_cbAtomicOverride;
        Assert( err == JET_errSuccess );
    }

    //  get the sector size for the root path

    if ( *pcbSize == 0 )
    {
        hFile = CreateFileW( wszPath,
                             FILE_READ_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             (  FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_BACKUP_SEMANTICS  ),
                             NULL );
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            // issue query for real sector size.
            // first try new win8 API which works in lowbox
            IO_STATUS_BLOCK                 status_block = {0};
            FILE_FS_SECTOR_SIZE_INFORMATION sector_info = {0};

            if ( g_pfnNtQueryVolumeInformationFile.ErrIsPresent() == JET_errSuccess &&
                 NT_SUCCESS( g_pfnNtQueryVolumeInformationFile(
                             hFile,
                             &status_block,
                             &sector_info,
                             sizeof(sector_info),
                             FileFsSectorSizeInformation ) ) )
            {
                *pcbSize = sector_info.FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
            }

            CloseHandle( hFile );
            hFile = INVALID_HANDLE_VALUE;
        }
    }

    if ( *pcbSize == 0 )
    {
        ULONG cbWritten = 0;
        STORAGE_PROPERTY_QUERY      QueryDesc;
        STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR StorAlignmentDesc = { 0 };

        // the query result's "version" (and also its size) as of Vista Beta2 ...
        #define STORAGE_ACCESS_ALIGNMENT_PROPERTY_MIN_VER  28

        // configure the query params ...
        memset( &QueryDesc, 0, sizeof(QueryDesc) );
        QueryDesc.QueryType = PropertyStandardQuery;
        QueryDesc.PropertyId = StorageAccessAlignmentProperty;

        //
        //  Getting the real sector size ...

        // Disk manufacturers are starting to produce disks with more than 512 bytes
        // per physical sector.  Unfortunately for the 20 years DOS/Windows has been
        // around, no API has reported anything but 512 bytes, ergo it would be too
        // risky to change the normal APIs to return the real sector size.  This means
        // we have to crack out the real sector size with an IOCTL_ and
        // DeviceIoControl().
        //
        // The problem with DeviceIoControl() is that it seems to be equivalent to
        // talking straight to the driver.  And due to experimentation, the drivers
        // seem to vary greatly to the degree they maintain a good contract.  For
        // instance the WinXP SP2 inbox pciide.sys driver seems to return success for
        // this particular DeviceIoControl() call, even though it does nothing to
        // actually fill out the structure.
        //
        // In theory we can query if the device driver can handle this particular IOCTL_
        // and particular query property ID via this query type:
        //   QueryDesc.QueryType = PropertyExistsQuery;
        // ... if that sounds like a good plan, you forgot to read about pciide.sys
        // above.  So we will just make the query directly.
        //
        // Further fun, a widely deployed Intel storage driver reports 3072 / 3 KB when 
        // a 4 KB sector-sized disk is attached due to some math error in their driver 
        // for Win7 RTM and earlier.  This is causing ESE to fail (throwing an error
        // from below) when such disks are attached to Win7 systems.  We haven't 
        // decided what to do about this yet ... my temptation is to insist intel fix, 
        // (note: it is fixed in Sp1) as I've seen lots of bad behaviors introduced  
        // by good intentions while not fixing code at the right layer.  Leading to 
        // bugs later.
        //
        // And another vendor was accidentally returning 512 KB (_kilobytes_!), which
        // is a power of 2, but probably larger than we could ever performantly handle.
        // 
        // Ergo this code must be very defensive, and try to query the real sector
        // size in a best effort way ...

        // but first we need a handle to the device ...
        err = ErrOSFSGetDeviceHandle( wszAbsRootPath, &hDevice );
        if ( err < JET_errSuccess )
        {
            // We're going to pretend this isn't a critical failure.
            err = JET_errSuccess;
        }
        else if ( !DeviceIoControl( hDevice,
                                    IOCTL_STORAGE_QUERY_PROPERTY,
                                    &QueryDesc,
                                    sizeof(QueryDesc),
                                    &StorAlignmentDesc,
                                    sizeof(StorAlignmentDesc),
                                    &cbWritten,
                                    NULL ) )
        {
            // probably not supported ... and so then we ignore it, and hope for the best ...
            error = ERROR_SUCCESS;
            err = JET_errSuccess;
        }
        else
        {

            // Even on success, we have to be defensive the response makes sense ...

            ULONG cbMinFieldRequired = OffsetOf( STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR, BytesPerPhysicalSector )
                                    + sizeof( StorAlignmentDesc.BytesPerPhysicalSector );

            if ( ( cbWritten >= cbMinFieldRequired ) &&
                 ( StorAlignmentDesc.Size >= cbMinFieldRequired ) &&
                 ( StorAlignmentDesc.Version >= STORAGE_ACCESS_ALIGNMENT_PROPERTY_MIN_VER ) &&
                 ( StorAlignmentDesc.BytesPerPhysicalSector != 0 ) // just means not filled in ...
                 )
            {
                //  ok, the alignment desc seems to be reasonably filled out ...

                // The BytesPerPhysicalSector element is what describes the disk devices true
                // sector size. The BytesPerPhysicalSector element is what describes the disk devices true
                // sector size / atomic write size.
                //
                *pcbSize = StorAlignmentDesc.BytesPerPhysicalSector;

            }
        }
    }

    if ( *pcbSize == 0 )
    {
        *pcbSize = 512;
    }

    if ( ( *pcbSize < 512 ) ||
        !FPowerOf2( *pcbSize ) )
    {
#if defined( USE_HAPUBLISH_API )
        //  indicate that this is a configuration error
        //
        Pfsconfig()->EmitFailureTag( HaDbFailureTagConfiguration, L"502bb7a1-11eb-470d-aed2-866d30d7a7f9" );
#endif

        // ESE does not support physical sector sizes of less than 512 or non-
        // powers of 2.  This is more likely a ridiculous driver issue anyway.
        Error( ErrERRCheck( JET_errSectorSizeNotSupported ) );
    }

    //
    //  Sector size is tractable, cache result.
    //
    m_critVolumePathCache.Enter();
    pvpce = GetVolInfoCacheEntry( wszAbsRootPath );
    if ( pvpce == NULL )
    {
        //  if we fail to allocate then we will simply not cache the result
        //
        if ( pvpce = new CVolumePathCacheEntry( wszAbsRootPath ) )
        {
            m_ilVolumePathCache.InsertAsPrevMost( pvpce );
        }
    }
    if ( pvpce )
    {
        pvpce->SetSecSize( *pcbSize );
    }
    m_critVolumePathCache.Leave();

HandleWin32Error:
    if ( err < JET_errSuccess )
    {
        ReportDiskError( OSFS_SECTOR_SIZE_ERROR_ID, wszPath, wszAbsRootPath, err, error );
    }

HandleError:

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    if ( hDevice != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hDevice );
    }
    if ( err < JET_errSuccess )
    {
        *pcbSize = 0;
    }
    return err;
}

ERR COSFileSystem::ErrFileAtomicWriteSize(  const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize )
{
    // The atomic write size is the chunk size the disk device can write without 
    // affecting adjacent data.
    //
    // Most modern disk drives checksum (or even ECC these days c 2006) each 
    // physical sector they write, and this can make a sector unretrievable if 
    // the computer suffers a power outage in the middle of writing a sector, 
    // because the sector will have 1/2 a write there, and neither the old nor 
    // new checksum likely matches the data.  The disk then refuses to return
    // this data, this makes the sector unretrievable.
    //
    // The reason this is important is ESE could issue a write for a section of 
    // the transaction log, that commits a transaction to disk.  If that section 
    // ends before the end of the true sector size, and we append more trx log data 
    // directly, the 2nd write could make the sector unretrievalbe on a power 
    // outage, and effectively "uncommit" the previous data that we claimed (to 
    // the ESE client) was durably committed.
    //
    // ESE deals with this issue, by using a shadow sector in the transaction
    // log for any write that is less than the physical sector size / appending 
    // to previously committed data within a sector.
    //
    return ErrFileSectorSize( wszPath, pcbSize );
}

ERR COSFileSystem::ErrPathComplete( _In_z_ const WCHAR* const   wszPath,
    // UNDONE_BANAPI: Must add count, will require lots of touching ...
                                    _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const       wszAbsPath )
{
    ERR err = JET_errSuccess;

    OSTrapPath( wszPath );
    const WCHAR* wszPathToComplete = wszPath;
    WCHAR wszRedirectedRelativePath[ OSFSAPI_MAX_PATH ];

    //  RFS:  bad path

    if ( !RFSAlloc( OSFilePathComplete ) )
    {
        Call( ErrERRCheck( JET_errInvalidPath ) );
    }

    if ( FPathIsRelative( wszPath ) )
    {
        BOOL fCanUseRelativePaths = fFalse;
        Call( ErrPathFolderDefault( wszRedirectedRelativePath, sizeof( wszRedirectedRelativePath ), &fCanUseRelativePaths ) );

        // The output should have a trailing backslash already.

        Assert( L'\\' == wszRedirectedRelativePath[ LOSStrLengthW( wszRedirectedRelativePath ) - 1 ] );

        if ( fCanUseRelativePaths )
        {
            Assert( 0 == LOSStrCompareW( L".\\", wszRedirectedRelativePath ) );
            Assert( wszPathToComplete == wszPath );
        }
        else
        {
            Assert( LOSStrLengthW( wszRedirectedRelativePath ) > 2 );

            // Prepend the special default directory.

            OSStrCbAppendW( wszRedirectedRelativePath, sizeof( wszRedirectedRelativePath ), wszPath );
            wszPathToComplete = wszRedirectedRelativePath;
        }
    }

    if ( !_wfullpath( wszAbsPath, wszPathToComplete, IFileSystemAPI::cchPathMax ) )
    {
        Call( ErrERRCheck( JET_errInvalidPath ) );
    }

    OSTrapPath( wszAbsPath );

    return JET_errSuccess;

HandleError:
    return err;
}

ERR COSFileSystem::ErrPathParse(    const WCHAR* const  wszPath,
    // UNDONE_BANAPI:
                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFolder,
                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFileBase,
                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFileExt )
{
    WCHAR wszFolderT[ IFileSystemAPI::cchPathMax ];
    errno_t err;

    OSTrapPath( wszPath );

    if ( wszFolder )
    {
        *wszFolder      = L'\0';
    }
    if ( wszFileBase )
    {
        *wszFileBase    = L'\0';
    }
    if ( wszFileExt )
    {
        *wszFileExt     = L'\0';
    }
    
    err = _wsplitpath_s( wszPath,
                    wszFolder, wszFolder ? OSFSAPI_MAX_PATH : 0,
                    wszFolderT, sizeof(wszFolderT)/sizeof(WCHAR),
                    wszFileBase, wszFileBase ? OSFSAPI_MAX_PATH : 0,
                    wszFileExt, wszFileExt ? OSFSAPI_MAX_PATH : 0 );

#ifdef DEBUG
    switch( err )
    {
        case 0:
            // success.
            break;

        default:
            AssertSzRTL( fFalse, "Unexpected errno %d from _wsplitpath_s.", err );
            break;
    }
#endif

    if ( wszFolder )
    {
        //  wszFolder already contains the drive, so just append
        //  the directory to form the complete folder location
        // UNDONE_BANAPI:
        OSStrCbAppendW( wszFolder, OSFSAPI_MAX_PATH*sizeof(WCHAR), wszFolderT );
        Assert( LOSStrLengthW( wszFolder ) < IFileSystemAPI::cchPathMax );
    }

    OSTrapPath( wszFolder );
    OSTrapPath( wszFileBase );
    OSTrapPath( wszFileExt );

    return JET_errSuccess;
}

const WCHAR * const COSFileSystem::WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const
{
    if ( NULL == wszOptionalFullPath || wszOptionalFullPath[ 0 ] == '\0' )
    {
        return L"";
    }
    const WCHAR * const wszLastDelimiter = wcsrchr( wszOptionalFullPath, wchPathDelimiter );
    if ( NULL == wszLastDelimiter )
    {
        return wszOptionalFullPath;
    }
    if ( wszLastDelimiter + 1 >= wszOptionalFullPath + LOSStrLengthW( wszOptionalFullPath ) )
    {
        ExpectedSz( FNegTest( fInvalidUsage ), "Path with delimiter at very end of string." );
        return L"UNKNOWN.PTH";
    }

    return wszLastDelimiter + 1;
}

ERR COSFileSystem::ErrPathBuild(    __in_z const WCHAR* const   wszFolder,
                                    __in_z const WCHAR* const   wszFileBase,
                                    __in_z const WCHAR* const   wszFileExt,
                                    __out_bcount_z(cbPath) WCHAR* const wszPath,
                                    __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG cbPath )
{
    *wszPath = L'\0';

    OSTrapPath( wszFolder );
    OSTrapPath( wszFileBase );
    OSTrapPath( wszFileExt );

    if (0 != _wmakepath_s( wszPath, cbPath / sizeof(WCHAR), NULL, wszFolder, wszFileBase, wszFileExt ) )
    {
        return ErrERRCheck( JET_errInvalidPath );
    }

    OSTrapPath( wszPath );

    return JET_errSuccess;
}

ERR COSFileSystem::ErrPathFolderNorm(   __inout_bcount(cbSize) PWSTR const  wszFolder,
                                                             DWORD          cbSize )
{
    ERR err = JET_errSuccess;
    DWORD cch = LOSStrLengthW(wszFolder);

    OSTrapPath( wszFolder );

    // if somehow we were given an empty folder to normalize, it is a
    // bad, bad path!
    if ( 0 == cch )
    {
        AssertSz( fFalse, "Unexpected empty folder name to be normalized." );
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }

    Assert( ((cch+1)*sizeof(WCHAR)) <= cbSize );

    if ( wszFolder[cch-1] != wchPathDelimiter )
    {
        if ( ((cch+2) * sizeof(WCHAR)) > cbSize )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }

        wszFolder[cch] = wchPathDelimiter;
        wszFolder[cch+1] = L'\0';
    }
    // else folder path already had folder delimiter ...


HandleError:
    return(err);
}

// Returns whether the input path is absolute or relative.
// Note that a leading backslash (e.g. \windows\win.ini) is still relative.

BOOL COSFileSystem::FPathIsRelative(
    _In_ PCWSTR wszPath )
{
    if ( NULL == wszPath )
    {
        return fFalse;
    }

    if ( iswalpha( wszPath[ 0 ] ) && L':' == wszPath[ 1 ] && L'\\' == wszPath[ 2 ] )
    {
        // It's of the form 'd:\'
        return fFalse;
    }

    if ( L'\\' == wszPath[ 0 ] && L'\\' == wszPath[ 1 ] )
    {
        // It's either:
        // -UNC-style paths are absolute. (Includes the \\?\ style of paths.)
        return fFalse;
    }

    return fTrue;
}

//  Returns whether the specified file/path exists.
ERR COSFileSystem::ErrPathExists(
    _In_ PCWSTR wszPath,
    _Out_opt_ BOOL* pfIsDirectory )
{
    JET_ERR err = JET_errSuccess;
    WIN32_FILE_ATTRIBUTE_DATA filedata;

    BOOL fSucceeded = GetFileAttributesExW( wszPath, GetFileExInfoStandard, &filedata );

    if ( fSucceeded )
    {
        if ( pfIsDirectory )
        {
            *pfIsDirectory = !!( filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
        }
    }
    else
    {
        err = ErrOSErrFromWin32Err( GetLastError(), JET_errFileNotFound );
        // maintain error compatibility with the existing FindFirstFile based
        // implementation
        if ( err == JET_errInvalidPath )
        {
            err = ErrERRCheck( JET_errFileNotFound );
        }
    }

    return err;
}

LOCAL WCHAR* g_wszDefaultPath;
const size_t g_cchDefaultPath = IFileSystemAPI::cchPathMax;
const size_t g_cbDefaultPath = g_cchDefaultPath * sizeof( *g_wszDefaultPath );
LOCAL BOOL g_fCanProcessUseRelativePaths = TRUE;

// Retrieves the default location that files should be stored.
// The "." directory (current working dir) is not writeable in new Windows UI.

ERR COSFileSystem::ErrPathFolderDefault(
    _Out_z_bytecap_(cbFolder) PWSTR const   wszFolder,
    _In_ DWORD          cbFolder,
    _Out_ BOOL          *pfCanProcessUseRelativePaths )
{
    ERR     err     = JET_errSuccess;

    *pfCanProcessUseRelativePaths = g_fCanProcessUseRelativePaths;

    Call( ErrOSFSInitDefaultPath() );
    Assert( g_wszDefaultPath );
    OSStrCbCopyW( wszFolder, cbFolder, g_wszDefaultPath );

    // If the process is packaged, then the directory shouldn't be the current
    // working directory.

    Assert( FUtilProcessIsPackaged() || 0 == wcscmp( wszFolder, L".\\" ) );

HandleError:
    return err;
}

ERR COSFileSystem::ErrFolderCreate( const WCHAR* const wszPath )
{
    ERR     err     = JET_errSuccess;
    DWORD   error   = ERROR_SUCCESS;

    OSTrapPath( wszPath );

    WCHAR           wszAbsPath[ OSFSAPI_MAX_PATH ];

    //  RFS:  access denied

    if ( !RFSAlloc( OSFileCreateDirectory ) )
    {
        error = ERROR_ACCESS_DENIED;
        Call( ErrGetLastError( error ) );
    }

    Call( ErrPathComplete( wszPath, wszAbsPath ) );

    if ( !CreateDirectoryW( wszAbsPath, NULL ) )
    {
        error = GetLastError();
        Call( ErrGetLastError( error ) );
    }

HandleError:
    ReportFileError( OSFS_CREATE_FOLDER_ERROR_ID, wszPath, NULL, err, error );
    return err;
}

ERR COSFileSystem::ErrFolderRemove( const WCHAR* const wszPath )
{
    ERR     err     = JET_errSuccess;
    DWORD   error   = ERROR_SUCCESS;

    OSTrapPath( wszPath );

    WCHAR           wszAbsPath[ OSFSAPI_MAX_PATH ];

    //  RFS:  access denied

    if ( !RFSAlloc( OSFileRemoveDirectory ) )
    {
        error = ERROR_ACCESS_DENIED;
        Call( ErrGetLastError( error ) );
    }

    Call( ErrPathComplete( wszPath, wszAbsPath ) );

    if ( !RemoveDirectoryW( wszAbsPath ) )
    {
        error = GetLastError();
        Call( ErrGetLastError( error ) );
    }

    //(void)ErrCommitFSChange( wszPath );

HandleError:
    if ( err == JET_errFileNotFound )
    {
        err = ErrERRCheck( JET_errInvalidPath );
    }
    ReportFileErrorWithFilter( OSFS_REMOVE_FOLDER_ERROR_ID, wszPath, NULL, err, error );
    return err;
}

ERR COSFileSystem::ErrFileFind( const WCHAR* const      wszFind,
                                IFileFindAPI** const    ppffapi )
{
    ERR             err     = JET_errSuccess;
    COSFileFind*    posff   = NULL;

    OSTrapPath( wszFind );

    WCHAR           wszAbsFind[ OSFSAPI_MAX_PATH ];
    Call( ErrPathComplete( wszFind, wszAbsFind ) );

    //  allocate the file find iterator

    Alloc( posff = new COSFileFind );

    //  initialize the file find iterator

    Call( posff->ErrInit( this, wszAbsFind ) );

    //  return the interface to our file find iterator

    *ppffapi = posff;
    return JET_errSuccess;

HandleError:
    delete posff;
    *ppffapi = NULL;
    return err;
}

ERR COSFileSystem::ErrFileDelete( const WCHAR* const wszPath )
{
    ERR         err         = JET_errSuccess;
    DWORD       error       = ERROR_SUCCESS;
    OSFSRETRY   OsfsRetry( Pfsconfig() );

    OSTrapPath( wszPath );

    WCHAR           wszAbsPath[ OSFSAPI_MAX_PATH ];

    //  RFS:  access denied

    if ( !RFSAlloc( OSFileDelete ) )
    {
        error = ERROR_ACCESS_DENIED;
        Call( ErrGetLastError( error ) );
    }

    Call( ErrPathComplete( wszPath, wszAbsPath ) );

    do
    {
        err = JET_errSuccess;
        error = ERROR_SUCCESS;
        if ( !DeleteFileW( wszAbsPath ) )
        {
            error = GetLastError();
            err = ErrGetLastError( error );
        }
    }
    while ( OsfsRetry.FRetry( err ) );
    Call( err );

    //(void)ErrCommitFSChange( wszAbsPath );

HandleError:
    //  we eat any file not found errors
    if ( err == JET_errFileNotFound )
    {
        err = JET_errSuccess;
    }
    ReportFileError( OSFS_DELETE_FILE_ERROR_ID, wszPath, NULL, err, error );
    return err;
}

ERR COSFileSystem::ErrFileMove( const WCHAR* const  wszPathSource,
                                const WCHAR* const  wszPathDest,
                                const BOOL          fOverwriteExisting )
{
    ERR         err             = JET_errSuccess;
    DWORD       error           = ERROR_SUCCESS;

    OSTrapPath( wszPathSource );
    OSTrapPath( wszPathDest );

    WCHAR           wszAbsPathSource[ OSFSAPI_MAX_PATH ];
    WCHAR           wszAbsPathDest[ OSFSAPI_MAX_PATH ];
    OSFSRETRY       OsfsRetryMoveEx( Pfsconfig() );

    Call( ErrPathComplete( wszPathSource, wszAbsPathSource ) );
    Call( ErrPathComplete( wszPathDest, wszAbsPathDest ) );

    //  RFS:  pre-move error

    if ( !RFSAlloc( OSFileMove ) )
    {
        error = ERROR_ACCESS_DENIED;
        Call( ErrGetLastError( error ) );
    }

    do
    {
        err     = JET_errSuccess;
        error   = ERROR_SUCCESS;

        if ( !MoveFileExW(  wszAbsPathSource,
                            wszAbsPathDest,
                            (   MOVEFILE_COPY_ALLOWED |
                                MOVEFILE_WRITE_THROUGH |
                                ( fOverwriteExisting ? MOVEFILE_REPLACE_EXISTING : 0 ) ) ) )
        {
            error   = GetLastError();
            err     = ErrGetLastError( error );
        }
    }
    while ( OsfsRetryMoveEx.FRetry( err ) );
    Call( err );

    //(void)ErrCommitFSChange( wszPathDest );

    //  RFS:  post-move error

    if ( !RFSAlloc( OSFileMove ) )
    {
        error = ERROR_IO_DEVICE;
        Call( ErrGetLastError( error ) );
    }

HandleError:
    ReportFileError( OSFS_MOVE_FILE_ERROR_ID, wszPathSource, wszPathDest, err, error );
    return err;
}

ERR COSFileSystem::ErrFileCopy( const WCHAR* const  wszPathSource,
                                const WCHAR* const  wszPathDest,
                                const BOOL          fOverwriteExisting )
{
    ERR             err                 = JET_errSuccess;
    DWORD           error               = ERROR_SUCCESS;
    OSFSRETRY       OsfsRetry( Pfsconfig() );
    WCHAR           wszAbsPathSource[ OSFSAPI_MAX_PATH ];
    WCHAR           wszAbsPathDest[ OSFSAPI_MAX_PATH ];

    OSTrapPath( wszPathSource );
    OSTrapPath( wszPathDest );

    Call( ErrPathComplete( wszPathSource, wszAbsPathSource ) );
    Call( ErrPathComplete( wszPathDest, wszAbsPathDest ) );

    //  RFS:  pre-copy error

    if ( !RFSAlloc( OSFileCopy ) )
    {
        error = ERROR_ACCESS_DENIED;
        Call( ErrGetLastError( error ) );
    }
    do
    {
        err = JET_errSuccess;
        error = ERROR_SUCCESS;
        if ( !CopyFileExW( wszAbsPathSource, wszAbsPathDest, NULL, NULL, NULL, fOverwriteExisting ? 0 : COPY_FILE_FAIL_IF_EXISTS ) )
        {
            error = GetLastError();
            err = ErrGetLastError( error );
        }
    }
    while ( OsfsRetry.FRetry( err ) );
    Call( err );

    (void)ErrCommitFSChange( wszAbsPathDest );

    //  RFS:  post-copy error

    if ( !RFSAlloc( OSFileCopy ) )
    {
        error = ERROR_IO_DEVICE;
        Call( ErrGetLastError( error ) );
    }

HandleError:
    ReportFileError( OSFS_COPY_FILE_ERROR_ID, wszPathSource, wszPathDest, err, error );
    return err;
}

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

ERR COSFileSystem::ErrFileCreate(   _In_z_ const WCHAR* const       wszPath,
                                    _In_ IFileAPI::FileModeFlags    fmf,
                                    _Out_ IFileAPI** const          ppfapi )
{
    ERR                         err                             = JET_errSuccess;
    DWORD                       error                           = ERROR_SUCCESS;
    const BOOL                  fCached                         = fmf & IFileAPI::fmfCached;
    const BOOL                  fLossyWriteBack                 = fmf & IFileAPI::fmfLossyWriteBack;
    const BOOL                  fTemporary                      = fmf & IFileAPI::fmfTemporary;
    const BOOL                  fStorageHardwareWriteBack       = fmf & IFileAPI::fmfStorageWriteBack;
    COSFile*                    posf                            = NULL;
    COSVolume *                 posv                            = NULL;
    BOOL                        fDisableManageVolumePrivilege   = fFalse;
    WCHAR                       wszAbsPath[ IFileSystemAPI::cchPathMax ]; wszAbsPath[0] = L'\0';
    HANDLE                      hFile                           = INVALID_HANDLE_VALUE;
    DWORD                       cbIOSize                        = 0;
    DWORD                       cbSectorSize                    = 0;
    BY_HANDLE_FILE_INFORMATION  bhfi                            = { 0 };
    QWORD                       cbFileSize                      = 0;
    BOOL                        fIsCompressed                   = fFalse;
    HANDLE                      hEvent                          = NULL;
    OSFSRETRY                   OsfsRetry( Pfsconfig() );

    Assert( 0 == ( fmf & IFileAPI::fmfReadOnly ) );
#ifndef FORCE_STORAGE_WRITEBACK
    AssertSz( !fCached || !fStorageHardwareWriteBack, "Can't use storage write back with caching together %#x", fmf );
    ExpectedSz( !fLossyWriteBack || !fStorageHardwareWriteBack, "Can't use storage write back with regular write back together %#x", fmf );
#endif

    OSTrapPath( wszPath );

    //  allocate the file object

    Alloc( posf = new COSFile );

    //  RFS:  pre-creation error

    if ( !RFSAlloc( OSFileCreate ) )
    {
        error = ERROR_ACCESS_DENIED;
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }

    //  if we are trying to create a temporary file then attempt to enable
    //  the volume management privilege so that we can use SetFileValidData
    //  to extend the file cheaply

    if ( fTemporary )
    {
        //  Make a best-effort attempt at enabling SeManageVolumePrivilege.
        (void) ErrSetPrivilege( WIDEN( SE_MANAGE_VOLUME_NAME ), fTrue, &fDisableManageVolumePrivilege );
    }

    Call( ErrPathComplete( wszPath, wszAbsPath ) );

    //  create the file, retrying for a limited time on access denied

    do
    {
        err     = JET_errSuccess;
        error   = ERROR_SUCCESS;

        hFile = CreateFileW(    wszAbsPath,
                                DwDesiredAccessFromFileModeFlags( IFileAPI::fmfNone ),
                                DwShareModeFromFileModeFlags( IFileAPI::fmfNone ),
                                NULL,
                                DwCreationDispositionFromFileModeFlags( fTrue, fmf ),
                                DwFlagsAndAttributesFromFileModeFlags( fmf ),
                                NULL );

        if ( hFile == INVALID_HANDLE_VALUE )
        {
            error   = GetLastError();
            err     = ErrGetLastError( error );
        }
    }
    while ( OsfsRetry.FRetry( err ) );
    CallJ( err, HandleWin32Error );

    SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

    //  RFS:  post-creation error

    if ( !RFSAlloc( OSFileCreate ) )
    {
        error = ERROR_IO_DEVICE;
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }

    //  get the file's properties

    if ( !GetFileInformationByHandle( hFile, &bhfi ) )
    {
        error = GetLastError();
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }
    cbFileSize      = ( QWORD( bhfi.nFileSizeHigh ) << 32 ) + bhfi.nFileSizeLow;
    fIsCompressed   = !!( bhfi.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED );

    //  Get a volume and disk this file ...

    IVolumeAPI * pvolapi = NULL;
    Call( ErrOSVolumeConnect( this, wszAbsPath, &pvolapi ) );
    Assert( pvolapi );
    posv = PosvFromVolAPI( pvolapi );
    ASSERT_VALID( posv );
    Assert( posv->CRef() >= 1 );

    Call( ErrFileSectorSize( wszAbsPath, &cbSectorSize ) );
    if ( cbSectorSize > 4096 )
    {
        cbSectorSize = 4096;
    }

    //  See MSDN docs on "File Buffering". It says:
    //  As previously discussed, an application must meet certain requirements when working with files
    //  opened with FILE_FLAG_NO_BUFFERING. The following specifics apply:
    //      o File access sizes must be for a number of bytes that is an integer multiple of the volume
    //          sector size. For example, if the sector size is 512 bytes, an application can request
    //          reads and writes of 512, 1,024, 1,536, or 2,048 bytes, but not of 335, 981, or 7,171 bytes.
    //      o File access buffer addresses for read and write operations should be sector-aligned, which
    //          means aligned on addresses in memory that are integer multiples of the volume sector size.
    //
    cbIOSize = !fCached ? cbSectorSize : 1;

    //  if the file is compressed then the kernel forces caching on so we can
    //  not perform atomic writes.  for our purposes, this is not acceptable so
    //  we will immediately decompress the file if it was created as compressed

    if ( fIsCompressed && !fTemporary )
    {
        USHORT      usCompressionState  = COMPRESSION_FORMAT_NONE;
        OVERLAPPED  overlapped          = { 0 };
        DWORD       cbTransferred       = 0;

        //  decompress the newly created, zero-length file

        Alloc( hEvent = CreateEventW( NULL, FALSE, FALSE, NULL ) );
        overlapped.hEvent = hEvent;
        if (    (   !DeviceIoControl(   hFile,
                                        FSCTL_SET_COMPRESSION,
                                        &usCompressionState,
                                        sizeof( usCompressionState ),
                                        NULL,
                                        0,
                                        &cbTransferred,
                                        &overlapped ) &&
                    GetLastError() != ERROR_IO_PENDING ) ||
                (   !GetOverlappedResult(   hFile,
                                            &overlapped,
                                            &cbTransferred,
                                            TRUE ) ) )
        {
            error = GetLastError();
            CallJ( ErrGetLastError( error ), HandleWin32Error );
        }

        //  reopen the file to ensure caching is disabled

        SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( hFile );
        OSVolumeDisconnect( posv );
        delete posf;
        CloseHandle( hEvent );

        if ( ( err = ErrFileOpen( wszAbsPath, ( fCached ? IFileAPI::fmfCached : IFileAPI::fmfNone ) | ( fLossyWriteBack ? IFileAPI::fmfLossyWriteBack : IFileAPI::fmfNone ), ppfapi ) ) < JET_errSuccess )
        {
            DeleteFileW( wszAbsPath );
        }
        return err;
    }

    if ( !( bhfi.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ) )
    {
        //  currently this is configured through ErrSetSparseness().
        Assert( 0 == ( fmf & IFileAPI::fmfSparse ) );
    }
    else
    {
        //  fmfSparse may be set later via ErrSetSparseness(), but we should inform the user 
        //  that the file is sparse already.
        fmf |= IFileAPI::fmfSparse;
    }

    //  make sure the create file operation is committed
    //
    //  NOTE:  if we used FILE_FLAG_WRITE_THROUGH then the change is already
    //  committed.  if we didn't then we don't care if the file is committed
    //  because it is a temp file.  so, we don't need to call ErrCommitFSChange
    //  here

    //(void)ErrCommitFSChange( wszAbsPath );

    //  initialize the file object

    Call( posf->ErrInitFile( this, posv, wszAbsPath, hFile, cbFileSize, fmf, cbIOSize, cbSectorSize ) );
    OSTrace( JET_tracetagFile, OSFormat( "ErrFileCreate( %ws, 0x%x ) -> 0x%p{0x%p} )", wszAbsPath, fmf, posf, hFile ) );
    hFile = INVALID_HANDLE_VALUE;
    posv = NULL;

    //  return the interface to our file object

    *ppfapi = posf;

HandleWin32Error:
    ReportFileError( OSFS_CREATE_FILE_ERROR_ID, wszAbsPath, NULL, err, error );

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
            CloseHandle( hFile );
            DeleteFileW( wszAbsPath );
        }
        if ( posv )
        {
            OSVolumeDisconnect( posv );
        }
        delete posf;
        *ppfapi = NULL;
    }
    if ( hEvent )
    {
        CloseHandle( hEvent );
    }
    if ( fDisableManageVolumePrivilege )
    {
        CallS( ErrSetPrivilege( WIDEN( SE_MANAGE_VOLUME_NAME ), fFalse ) );
    }
    return err;
}

ERR COSFileSystem::ErrFileOpen( _In_z_ const WCHAR* const       wszPath,
                                _In_ IFileAPI::FileModeFlags    fmf,
                                _Out_ IFileAPI** const          ppfapi )
{
    ERR                         err                 = JET_errSuccess;
    DWORD                       error               = ERROR_SUCCESS;
    const BOOL                  fReadOnly           = fmf & IFileAPI::fmfReadOnly;
    const BOOL                  fCached             = fmf & IFileAPI::fmfCached;
    const BOOL                  fLossyWriteBack         = fmf & IFileAPI::fmfLossyWriteBack;
    const BOOL                  fStorageHardwareWriteBack = fmf & IFileAPI::fmfStorageWriteBack;
    COSFile*                    posf                = NULL;
    COSVolume *                 posv                = NULL;
    WCHAR                       wszAbsPath[ IFileSystemAPI::cchPathMax ]; wszAbsPath[0] = L'\0';
    HANDLE                      hFile               = INVALID_HANDLE_VALUE;
    DWORD                       cbIOSize            = 0;
    DWORD                       cbSectorSize        = 0;
    BY_HANDLE_FILE_INFORMATION  bhfi                = { 0 };
    QWORD                       cbFileSize          = 0;
    BOOL                        fIsCompressed       = fFalse;
    HANDLE                      hEvent              = NULL;
    OSFSRETRY                   OsfsRetry( Pfsconfig() );

    Assert( 0 == ( fmf & IFileAPI::fmfTemporary ) );
    Assert( 0 == ( fmf & IFileAPI::fmfOverwriteExisting ) );
#ifndef FORCE_STORAGE_WRITEBACK
    AssertSz( !fCached || !fStorageHardwareWriteBack, "Can't use storage write back with caching together %#x", fmf );
    ExpectedSz( !fLossyWriteBack || !fStorageHardwareWriteBack, "Can't use storage write back with lossy write back together %#x", fmf );
    ExpectedSz( !fReadOnly || !fStorageHardwareWriteBack, "Can't use read only and storage write back together %#x", fmf );
#endif

    OSTrapPath( wszPath );

    //  allocate the file object

    Alloc( posf = new COSFile );

    //  RFS:  access denied

    if ( !RFSAlloc( OSFileOpen ) )
    {
        error = ERROR_ACCESS_DENIED;
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }

    Call( ErrPathComplete( wszPath, wszAbsPath ) );

    //  create the file, retrying for a limited time on access denied

    const DWORD dwDesiredAccess = DwDesiredAccessFromFileModeFlags( fmf );
    const DWORD dwShareMode = DwShareModeFromFileModeFlags( fmf );
    const DWORD dwCreationDisposition = DwCreationDispositionFromFileModeFlags( fFalse, fmf );
    const DWORD dwFlagsAndAttributes = DwFlagsAndAttributesFromFileModeFlags( fmf );

    do
    {
        err     = JET_errSuccess;
        error   = ERROR_SUCCESS;
        
        OSTrace( JET_tracetagFile, OSFormat( "OS CreateFileW( path, %#x, %#x, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | %hs | %hs, NULL )",
                                    dwDesiredAccess,
                                    dwShareMode,
                                    ( ( dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH ) == 0 ? "0" : "FILE_FLAG_WRITE_THROUGH" ),
                                    ( ( dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS ) != 0 ) ? "FILE_FLAG_RANDOM_ACCESS" : "FILE_FLAG_NO_BUFFERING" ) );

        hFile = CreateFileW(    wszAbsPath,
                                dwDesiredAccess,
                                dwShareMode,
                                NULL,
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
                                NULL );

        if ( hFile == INVALID_HANDLE_VALUE )
        {
            error   = GetLastError();
            err     = ErrGetLastError( error );
        }
    }
    while ( OsfsRetry.FRetry( err ) );
    CallJ( err, HandleWin32Error );

    SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

    //  get the file's properties

    if ( !GetFileInformationByHandle( hFile, &bhfi ) )
    {
        error = GetLastError();
        CallJ( ErrGetLastError( error ), HandleWin32Error );
    }
    cbFileSize      = ( QWORD( bhfi.nFileSizeHigh ) << 32 ) + bhfi.nFileSizeLow;
    fIsCompressed   = !!( bhfi.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED );

    //  Get a volume and disk this file ...

    IVolumeAPI * pvolapi = NULL;
    Call( ErrOSVolumeConnect( this, wszAbsPath, &pvolapi ) );
    Assert( pvolapi );
    posv = PosvFromVolAPI( pvolapi );
    ASSERT_VALID( posv );
    Assert( posv->CRef() >= 1 );

    Call( ErrFileSectorSize( wszAbsPath, &cbSectorSize ) );
    if ( cbSectorSize > 4096 )
    {
        FireWall( "SectorSizeTooBig" );
        cbSectorSize = 4096;
    }

    //  See MSDN docs on "File Buffering". It says:
    //  As previously discussed, an application must meet certain requirements when working with files
    //  opened with FILE_FLAG_NO_BUFFERING. The following specifics apply:
    //      o File access sizes must be for a number of bytes that is an integer multiple of the volume
    //          sector size. For example, if the sector size is 512 bytes, an application can request
    //          reads and writes of 512, 1,024, 1,536, or 2,048 bytes, but not of 335, 981, or 7,171 bytes.
    //      o File access buffer addresses for read and write operations should be sector-aligned, which
    //          means aligned on addresses in memory that are integer multiples of the volume sector size.
    //
    cbIOSize = ( dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING ) ? cbSectorSize : 1;
 
    //  if the file is compressed then the kernel forces caching on so we can
    //  not perform atomic writes.  for our purposes, this is not acceptable so
    //  we will immediately decompress the file if it is currently compressed.
    //  if the file is too big to convert then we will simply fail the open.
    //
    //  If this operation is being performed during setup, then no decompression
    //  will be attempted.  Instead, decompression will happen the next time
    //  the OS is cycled.
    //
    const QWORD cbFileSizeDecompressMax = 128 * 1024 * 1024;

    if ( !fReadOnly && fIsCompressed && !FOSSetupRunning() )
    {
        USHORT      usCompressionState  = COMPRESSION_FORMAT_NONE;
        OVERLAPPED  overlapped          = { 0 };
        DWORD       cbTransferred       = 0;

        //  decompress the file

        Alloc( hEvent = CreateEventW( NULL, FALSE, FALSE, NULL ) );
        overlapped.hEvent = hEvent;
        if (    cbFileSize >= cbFileSizeDecompressMax ||
                (   !DeviceIoControl(   hFile,
                                        FSCTL_SET_COMPRESSION,
                                        &usCompressionState,
                                        sizeof( usCompressionState ),
                                        NULL,
                                        0,
                                        &cbTransferred,
                                        &overlapped ) &&
                    GetLastError() != ERROR_IO_PENDING ) ||
                (   !GetOverlappedResult(   hFile,
                                            &overlapped,
                                            &cbTransferred,
                                            TRUE ) ) )
        {
            const WCHAR*    rgpwsz[ 2 ];
            DWORD           irgpwsz         = 0;
            WCHAR           wszError[ 64 ];

            OSStrCbFormatW( wszError, sizeof( wszError ), L"%i (0x%08x)", JET_errFileCompressed, JET_errFileCompressed );

            rgpwsz[ irgpwsz++ ]   = wszAbsPath;
            rgpwsz[ irgpwsz++ ]   = wszError;

            Pfsconfig()->EmitEvent( eventError,
                                    GENERAL_CATEGORY,
                                    OSFS_OPEN_COMPRESSED_FILE_RW_ERROR_ID,
                                    irgpwsz,
                                    rgpwsz,
                                    JET_EventLoggingLevelMin );

#if defined( USE_HAPUBLISH_API )            
            Pfsconfig()->EmitEvent( HaDbFailureTagConfiguration,
                                    Ese2HaId( GENERAL_CATEGORY ),
                                    Ese2HaId( OSFS_OPEN_COMPRESSED_FILE_RW_ERROR_ID ),
                                    irgpwsz,
                                    rgpwsz,
                                    HaDbIoErrorMeta,
                                    wszAbsPath );
#endif

            Call( ErrERRCheck( JET_errFileCompressed ) );
        }

        //  reopen the file to ensure caching is disabled

        SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
        CloseHandle( hFile );
        OSVolumeDisconnect( posv );
        delete posf;
        CloseHandle( hEvent );

        return ErrFileOpen( wszAbsPath, fmf, ppfapi );
    }

    if ( !( bhfi.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ) )
    {
        //  currently this is configured through ErrSetSparseness().
        Assert( 0 == ( fmf & IFileAPI::fmfSparse ) );
    }
    else
    {
        //  fmfSparse may be set later via ErrSetSparseness(), but we should inform the user 
        //  that the file is sparse already.
        fmf |= IFileAPI::fmfSparse;
    }

    //  initialize the file object

    Call( posf->ErrInitFile( this, posv, wszAbsPath, hFile, cbFileSize, fmf, cbIOSize, cbSectorSize ) );
    OSTrace( JET_tracetagFile, OSFormat( "ErrFileOpen( %ws, 0x%x ) -> 0x%p{0x%p} )", wszAbsPath, fmf, posf, hFile ) );
    hFile = INVALID_HANDLE_VALUE;
    posv = NULL;

    //  return the interface to our file object

    *ppfapi = posf;

HandleWin32Error:
    ReportFileErrorWithFilter( ( fReadOnly ? OSFS_OPEN_FILE_RO_ERROR_ID : OSFS_OPEN_FILE_RW_ERROR_ID ), wszAbsPath, NULL, err, error );

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            SetHandleInformation( hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
            CloseHandle( hFile );
        }
        if ( posv )
        {
            OSVolumeDisconnect( posv );
        }
        delete posf;
        *ppfapi = NULL;
    }
    if ( hEvent )
    {
        CloseHandle( hEvent );
    }
    return err;
}

ERR COSFileSystem::ErrSetPrivilege( const WCHAR *   wszPriv,
                                    const BOOL      fEnable,
                                    BOOL * const    pfJustChanged )
{
    ERR                         err                         = JET_errSuccess;
    TOKEN_PRIVILEGES            tp                          = { 0 };
    HANDLE                      hToken                      = NULL;
    TOKEN_PRIVILEGES            tpPrev                      = { 0 };
    DWORD                       cbPrev                      = 0;

    // init out param

    if ( pfJustChanged )
    {
        *pfJustChanged = fFalse;
    }

    //  initialize the defer loaded functionality

    NTOSFuncStd( pfnOpenProcessToken, g_mwszzProcessTokenLibs, OpenProcessToken, oslfExpectedOnWin5x | oslfStrictFree );
    NTOSFuncStd( pfnAdjustTokenPrivileges, g_mwszzAdjPrivLibs, AdjustTokenPrivileges, oslfExpectedOnWin5x | oslfStrictFree );
    NTOSFuncStd( pfnLookupPrivilegeValueW, g_mwszzLookupPrivLibs, LookupPrivilegeValueW, oslfExpectedOnWin5x | oslfStrictFree );

    Call( pfnOpenProcessToken.ErrIsPresent() );
    Call( pfnAdjustTokenPrivileges.ErrIsPresent() );
    Call( pfnLookupPrivilegeValueW.ErrIsPresent() );

    //  create the token privileges block

    tp.PrivilegeCount               = 1;
    tp.Privileges[ 0 ].Attributes   = fEnable ? SE_PRIVILEGE_ENABLED : 0;

    //  retrieve privilege LUID

    if ( !pfnLookupPrivilegeValueW( NULL, wszPriv, &tp.Privileges[ 0 ].Luid ) )
    {
        Call( ErrGetLastError() );
    }

    //  open process token and adjust our privileges

    if ( !pfnOpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) ||
         !pfnAdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( tpPrev ), &tpPrev, &cbPrev ) )
    {
        Call( ErrGetLastError() );
    }

    if ( pfJustChanged )
    {
        *pfJustChanged = tpPrev.PrivilegeCount != 0;
    }

HandleError:
    if ( hToken )
    {
        CloseHandle( hToken );
    }
    return err;
}

ERR COSFileSystem::ErrCommitFSChangeSlowly( const WCHAR* const wszPath )
{
    ERR         err                         = JET_errSuccess;
    WCHAR       wszAbsRootPath[ IFileSystemAPI::cchPathMax ];
    HANDLE      hRootPath                   = INVALID_HANDLE_VALUE;
    BOOL        fDisableBackupPrivilege     = fFalse;

    //  get the root path for the specified path

    Call( ErrPathRoot( wszPath, wszAbsRootPath ) );

    //  open the root path.  if we get access denied then try to enable backup
    //  privilege to bypass the access check and try again

    hRootPath = CreateFileW(    wszAbsRootPath,
                                GENERIC_WRITE,  //  required by FlushFileBuffers()
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL );
    if ( hRootPath == INVALID_HANDLE_VALUE && ErrGetLastError() == JET_errFileAccessDenied )
    {
        if ( ErrSetPrivilege( WIDEN( SE_BACKUP_NAME ), fTrue, &fDisableBackupPrivilege ) >= JET_errSuccess )
        {
            hRootPath = CreateFileW(    wszAbsRootPath,
                                        GENERIC_WRITE,  //  required by FlushFileBuffers()
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_BACKUP_SEMANTICS,
                                        NULL );
        }
        else
        {
            SetLastError( ERROR_ACCESS_DENIED );
        }
    }
    if ( hRootPath == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    //  commit all FS changes made to this root path
    //
    //  NOTE:  this call is VERY, VERY EXPENSIVE when performed on the root of
    //  a volume.  use only as a last resort

    Call( ErrCommitFSChangeSlowly( hRootPath ) );

HandleError:
    if ( hRootPath && hRootPath != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hRootPath );
    }
    if ( fDisableBackupPrivilege )
    {
        CallS( ErrSetPrivilege( WIDEN( SE_BACKUP_NAME ), fFalse ) );
    }
    return err;
}

ERR COSFileSystem::ErrCommitFSChangeSlowly( const HANDLE hFile )
{
    ERR err = JET_errSuccess;

    //  flush file buffers will force all changes made to the given file and
    //  its meta-data to disk

    if ( !FlushFileBuffers( hFile ) )
    {
        Error( ErrGetLastError() );
    }

HandleError:
    return err;
}

ERR COSFileSystem::ErrPathVolumeCanonical(   const WCHAR* const wszVolumePath,
                                            __out_ecount(cchVolumeCanonicalPath) WCHAR* const wszVolumeCanonicalPath,
                                            _In_ const DWORD cchVolumeCanonicalPath )
{
    if ( GetVolumeNameForVolumeMountPointW( wszVolumePath, wszVolumeCanonicalPath, cchVolumeCanonicalPath ) == 0 )
    {
        return ErrGetLastError();
    }

    return JET_errSuccess;
}

ERR COSFileSystem::ErrDiskId(   const WCHAR* const wszVolumeCanonicalPath,
                                __out_ecount(cchDiskId) WCHAR* const wszDiskId,
                                _In_ const DWORD cchDiskId,
                                _Out_ DWORD *pdwDiskNumber )
{
    ERR err                 = JET_errSuccess;
    HANDLE hVolume          = INVALID_HANDLE_VALUE;
    HANDLE hDisk            = INVALID_HANDLE_VALUE;
    BYTE* pbBufferVolume    = NULL;
    BYTE* pbBufferDisk      = NULL;

    //  CreateFile does not like volume paths terminated with '\'.

    WCHAR wszVolumeCanonicalPathT[ IFileSystemAPI::cchPathMax ];
    OSStrCbCopyW( wszVolumeCanonicalPathT, sizeof( wszVolumeCanonicalPathT ), wszVolumeCanonicalPath );

    const size_t cchVolumePathT = LOSStrLengthW( wszVolumeCanonicalPathT );
    Assert( cchVolumePathT > 0 );

    if ( cchVolumePathT == 0 )
    {
        AssertSz( fFalse, "cchVolumePathT should never be zero." );
        Call( ErrERRCheck( JET_errInternalError ) );
    }

    if ( ( wszVolumeCanonicalPathT[ cchVolumePathT - 1 ] == L'\\' ) )
    {
        wszVolumeCanonicalPathT[ cchVolumePathT - 1 ] = L'\0';
    }

    //  Volume handle.

    hVolume = CreateFileW( wszVolumeCanonicalPathT,
                            0,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    
    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    BOOL fGetVolumeExtents = fFalse;

    //  We need to loop until there's enough space to retrieve valid data.

    size_t cbBufferVolume = sizeof( VOLUME_DISK_EXTENTS );  //  let's start with enough for 1 extent only.

    do
    {
        DWORD cbWritten;

        delete[] pbBufferVolume;
        pbBufferVolume = new BYTE[ cbBufferVolume ];
        Alloc( pbBufferVolume );
        
        fGetVolumeExtents = DeviceIoControl( hVolume,
                                                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                                NULL, 0,
                                                pbBufferVolume, cbBufferVolume,
                                                &cbWritten,
                                                NULL );

        if ( !fGetVolumeExtents )
        {
            const DWORD error = GetLastError();
            if ( error == ERROR_MORE_DATA )
            {
                cbBufferVolume += sizeof( DISK_EXTENT );
            }
            else
            {
                Call( ErrGetLastError( error ) );
            }
        }
    }
    while( !fGetVolumeExtents );

    const VOLUME_DISK_EXTENTS* const pvde = (VOLUME_DISK_EXTENTS*)pbBufferVolume;

    //  We are not interested in volumes with no physical disk extents.

    if ( pvde->NumberOfDiskExtents < 1 )
    {
        Call( ErrERRCheck( errNotFound ) );
    }

    //  We are only interested in the first extent.

    *pdwDiskNumber = pvde->Extents[ 0 ].DiskNumber;

    //  Now, the disk.

    WCHAR wszDiskPath[ IFileSystemAPI::cchPathMax ];
    OSStrCbFormatW( wszDiskPath, sizeof( wszDiskPath ), L"\\\\.\\PhysicalDrive%u", *pdwDiskNumber );

    //  Disk handle.

    hDisk = CreateFileW( wszDiskPath,
                            0,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
    
    if ( hDisk == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    BOOL fGetDriveLayout = fFalse;

    //  Again, a similar tedious loop.

    size_t cbBufferDisk = sizeof( DRIVE_LAYOUT_INFORMATION_EX ) + sizeof( PARTITION_INFORMATION_EX );   //  let's start with enough for 2 partitions.

    do
    {
        DWORD cbWritten;

        delete[] pbBufferDisk;
        pbBufferDisk = new BYTE[ cbBufferDisk ];
        Alloc( pbBufferDisk );

        fGetDriveLayout = DeviceIoControl( hVolume,
                                            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                            NULL, 0,
                                            pbBufferDisk, cbBufferDisk,
                                            &cbWritten,
                                            NULL );

        if ( !fGetDriveLayout )
        {
            const DWORD error = GetLastError();
            if ( error == ERROR_INSUFFICIENT_BUFFER )
            {
                cbBufferDisk += sizeof( PARTITION_INFORMATION_EX );
            }
            else
            {
                Call( ErrGetLastError( error ) );
            }
        }
    }
    while( !fGetDriveLayout );

    //  Generate the disk ID.

    const DRIVE_LAYOUT_INFORMATION_EX* const pdli = (DRIVE_LAYOUT_INFORMATION_EX*)pbBufferDisk;

    switch ( pdli->PartitionStyle )
    {
        case PARTITION_STYLE_MBR:
            OSStrCbFormatW( wszDiskId, cchDiskId, L"MBR:%08X", pdli->Mbr.Signature );
            break;

        case PARTITION_STYLE_GPT:
        {
            const GUID guid = pdli->Gpt.DiskId;
            OSStrCbFormatW( wszDiskId, cchDiskId, L"GPT:%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                            guid.Data1,
                            guid.Data2,
                            guid.Data3,
                            guid.Data4[ 0 ], guid.Data4[ 1 ],
                            guid.Data4[ 2 ], guid.Data4[ 3 ], guid.Data4[ 4 ], guid.Data4[ 5 ], guid.Data4[ 6 ], guid.Data4[ 7 ] );
        }
            break;

        default:
            AssertSz( fFalse, "We should not have a partition of this type (%u)!", pdli->PartitionStyle );
            Call( ErrERRCheck( JET_errInternalError ) );
            break;
    }

HandleError:

    delete[] pbBufferVolume;
    delete[] pbBufferDisk;

    if ( hDisk != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hDisk );
    }

    if ( hVolume != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hVolume );
    }
    
    return err;
}


COSFileFind::COSFileFind()
    :   m_posfs( NULL ),
        m_hFileFind( INVALID_HANDLE_VALUE ),
        m_fBeforeFirst( fTrue ),
        m_errFirst( JET_errFileNotFound ),
        m_errCurrent( JET_errFileNotFound )
{
}

ERR COSFileFind::ErrInit(   COSFileSystem* const    posfs,
                            const WCHAR* const      wszFindPath )
{
    ERR     err             = JET_errSuccess;
    WCHAR   wszAbsFindPath[ IFileSystemAPI::cchPathMax ];
    WCHAR   wszT[ IFileSystemAPI::cchPathMax ];
    WCHAR*  pchEnd;
    BOOL    fExpectFolder   = fFalse;

    OSTrapPath( wszFindPath );

    //  reference the file system object that created this File Find iterator

    m_posfs = posfs;

    //  compute the full path of our search criteria

    CallJ( m_posfs->ErrPathComplete( wszFindPath, wszAbsFindPath ), DeferredInvalidPath )

    //  copy our original search criteria (the check above
    //  should ensure the string is a valid length, but be
    //  defensive just in case)

    Assert( LOSStrLengthW( wszFindPath ) < IFileSystemAPI::cchPathMax );
    OSStrCbCopyW( m_wszFindPath, sizeof( m_wszFindPath ), wszFindPath );
    m_wszFindPath[ IFileSystemAPI::cchPathMax - 1 ] = L'\0';

    //  we are searching for a specific folder

    pchEnd = wszAbsFindPath + LOSStrLengthW( wszAbsFindPath ) - 1;
    if (    pchEnd > wszAbsFindPath &&
            ( *pchEnd == L'\\' || *pchEnd == L'/' ) )
    {
        //  strip the trailing delimiter from the path only if the path is not a drive
        AssertPREFIX( pchEnd < wszAbsFindPath + _countof( wszAbsFindPath ) );
        if ( LOSStrLengthW( wszAbsFindPath ) <= 1 || *( pchEnd - 1 ) != L':' )
        {
            *pchEnd = L'\0';
        }
        //  remember that we expect to see a folder

        fExpectFolder = fTrue;
    }

    //  compute the absolute path of the folder we are searching

    CallJ( m_posfs->ErrPathParse( wszAbsFindPath, m_wszAbsFindPath, wszT, wszT ), DeferredInvalidPath );

    //  look for the first file or folder that matches our search criteria

    WIN32_FIND_DATAW wfd;
    m_hFileFind = FindFirstFileW( wszAbsFindPath, &wfd );

    //  we found something

    if ( m_hFileFind != INVALID_HANDLE_VALUE )
    {

        //  setup the iterator to move first on the file or folder that
        //  we found

        WCHAR   wszFile[ IFileSystemAPI::cchPathMax ];
        WCHAR   wszExt[ IFileSystemAPI::cchPathMax ];

        m_fFolder   = !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
        CallJ( m_posfs->ErrPathParse( wfd.cFileName, wszT, wszFile, wszExt ), DeferredInvalidPath );
        CallJ( m_posfs->ErrPathBuild( m_wszAbsFindPath, wszFile, wszExt, m_wszAbsFoundPath, _cbrg( m_wszAbsFoundPath ) ), DeferredInvalidPath );
        m_cbSize    = ( QWORD( wfd.nFileSizeHigh ) << 32 ) + wfd.nFileSizeLow;
        m_fReadOnly = !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY );

        m_errFirst = JET_errSuccess;

        //  if we should have found a folder but did not then setup the
        //  iterator to find nothing and return invalid path

        if ( fExpectFolder && !m_fFolder )
        {
            m_errFirst = JET_errInvalidPath;
        }
    }

    //  we didn't find something

    else
    {
        //  setup the iterator to move first onto the resulting error

        //  if we encounter ERROR_INVALID_NAME for the FindFirstFileW
        
        //  call above, this is really an invalid path.
        
        if ( GetLastError() == ERROR_INVALID_NAME )
        {
            m_errFirst = JET_errFileNotFound;
        }
        else
        {
            m_errFirst = m_posfs->ErrGetLastError();
        }

        //  if the path was invalid then we did not find any files

        if ( m_errFirst == JET_errInvalidPath )
        {
            m_errFirst = JET_errFileNotFound;
        }

        //  if we failed for some reason other than not finding a file or
        //  folder that match our search criteria then fail the creation
        //  of the File Find iterator with that error

        if ( m_errFirst != JET_errFileNotFound )
        {
            Call( ErrERRCheck( m_errFirst ) );
        }

        //  the search criteria exactly matches the root of a volume

        WCHAR * wszAbsRootPath  = wszT;
        if (    m_posfs->ErrPathRoot( wszAbsFindPath, wszAbsRootPath ) == JET_errSuccess &&
                !_wcsnicmp( wszAbsFindPath,
                            wszAbsRootPath,
                            max( LOSStrLengthW( wszAbsFindPath ), LOSStrLengthW( wszAbsRootPath ) - 1 ) ) )
        {
            //  get the attributes of the root

            const DWORD dwFileAttributes = GetFileAttributesW( wszAbsFindPath );

            //  we got the attributes of the root

            if ( dwFileAttributes != -1 )
            {
                //  setup the iterator to move first onto this root

                m_fFolder   = !!( dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
                OSStrCbCopyW( m_wszAbsFoundPath, sizeof( m_wszAbsFoundPath ), wszAbsFindPath );
                m_cbSize    = 0;
                m_fReadOnly = !!( dwFileAttributes & FILE_ATTRIBUTE_READONLY );

                m_errFirst  = JET_errSuccess;
            }

            //  we failed to get the attributes of the root

            else
            {
                //  setup the iterator to move first onto the resulting error

                m_errFirst = m_posfs->ErrGetLastError();
            }
        }
    }

    return JET_errSuccess;

HandleError:
    return err;

DeferredInvalidPath:
    //  if the path was invalid then we did not find any files
    m_errFirst = err == JET_errInvalidPath ? JET_errFileNotFound : err;
    return JET_errSuccess;
}

COSFileFind::~COSFileFind()
{
    if ( m_posfs )
    {
        //  unreference our file system object

        m_posfs = NULL;
    }
    if ( m_hFileFind != INVALID_HANDLE_VALUE )
    {
        FindClose( m_hFileFind );
        m_hFileFind = INVALID_HANDLE_VALUE;
    }
    m_fBeforeFirst  = fTrue;
    m_errFirst      = JET_errFileNotFound;
    m_errCurrent    = JET_errFileNotFound;
}

ERR COSFileFind::ErrNext()
{
    ERR err = JET_errSuccess;

    //  we have yet to move first

    if ( m_fBeforeFirst )
    {
        m_fBeforeFirst = fFalse;

        //  setup the iterator to be on the results of the move first that we
        //  did in ErrInit()

        m_errCurrent = m_errFirst;
    }

    //  we can potentially see more files or folders

    else if ( m_hFileFind != INVALID_HANDLE_VALUE )
    {
        WIN32_FIND_DATAW wfd;

        //  we found another file or folder

        if ( FindNextFileW( m_hFileFind, &wfd ) )
        {
            //  setup the iterator to be on the file or folder that we found

            WCHAR   wszT[ IFileSystemAPI::cchPathMax ];
            WCHAR   wszFile[ IFileSystemAPI::cchPathMax ];
            WCHAR   wszExt[ IFileSystemAPI::cchPathMax ];

            m_fFolder   = !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
            Call( m_posfs->ErrPathParse( wfd.cFileName, wszT, wszFile, wszExt ) );
            Call( m_posfs->ErrPathBuild( m_wszAbsFindPath, wszFile, wszExt, m_wszAbsFoundPath, _cbrg( m_wszAbsFoundPath ) ) );
            m_cbSize    = ( QWORD( wfd.nFileSizeHigh ) << 32 ) + wfd.nFileSizeLow;
            m_fReadOnly = !!( wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY );

            m_errCurrent = JET_errSuccess;
        }

        //  we didn't find another file or folder

        else
        {
            //  setup the iterator to be on the resulting error

            m_errCurrent = m_posfs->ErrGetLastError();
        }
    }

    //  we cannot potentially see any more files or folders

    else
    {
        //  setup the iterator to be after last

        m_errCurrent = JET_errFileNotFound;
    }

    //  RFS:  file not found

    if ( !RFSAlloc( OSFileFindNext ) )
    {
        m_errCurrent = JET_errFileNotFound;
    }

    //  check the error state of the iterator's current entry

    if ( m_errCurrent < JET_errSuccess )
    {
        Call( ErrERRCheck( m_errCurrent ) );
    }

    return JET_errSuccess;

HandleError:
    m_errCurrent = err;
    return err;
}

ERR COSFileFind::ErrIsFolder( BOOL* const pfFolder )
{
    ERR err = JET_errSuccess;

    if ( m_errCurrent < JET_errSuccess )
    {
        Call( ErrERRCheck( m_errCurrent ) );
    }

    *pfFolder = m_fFolder;
    return JET_errSuccess;

HandleError:
    *pfFolder = fFalse;
    return err;
}

ERR COSFileFind::ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsFoundPath )
{
    ERR err = JET_errSuccess;

    if ( m_errCurrent < JET_errSuccess )
    {
        Call( ErrERRCheck( m_errCurrent ) );
    }

    // UNDONE_BANAPI:
    OSStrCbCopyW( wszAbsFoundPath, OSFSAPI_MAX_PATH*sizeof(WCHAR), m_wszAbsFoundPath );

    OSTrapPath( wszAbsFoundPath );

    return JET_errSuccess;

HandleError:
    // UNDONE_BANAPI:
    wszAbsFoundPath[0] = L'\0';
    return err;
}

ERR COSFileFind::ErrSize(
    _Out_ QWORD* const pcbSize,
    _In_ const IFileAPI::FILESIZE filesize )
{
    ERR err = JET_errSuccess;

    if ( m_errCurrent < JET_errSuccess )
    {
        Call( ErrERRCheck( m_errCurrent ) );
    }

    *pcbSize = m_cbSize;
    return JET_errSuccess;

HandleError:
    *pcbSize = 0;
    return err;
}

ERR COSFileFind::ErrIsReadOnly( BOOL* const pfReadOnly )
{
    ERR err = JET_errSuccess;

    if ( m_errCurrent < JET_errSuccess )
    {
        Call( ErrERRCheck( m_errCurrent ) );
    }

    *pfReadOnly = m_fReadOnly;
    return JET_errSuccess;

HandleError:
    *pfReadOnly = fFalse;
    return err;
}


//  initializes an interface to the default OS File System

CDefaultFileSystemConfiguration::CDefaultFileSystemConfiguration()
    :   //m_cbZeroExtend( 1024 * 1024 ),
        m_dtickAccessDeniedRetryPeriod( 100 * 1000 ),
        //m_cIOMaxOutstanding( 1024 ),
        //m_cIOMaxOutstandingBackground( 32 ),
        m_dtickHungIOThreshhold( 60 * 1024 ),
        m_grbitHungIOActions( JET_bitHungIOEvent ),
        m_cbMaxReadSize( 384 * 1024 ),
        m_cbMaxWriteSize( 384 * 1024 ),
        m_cbMaxReadGapSize( 384 * 1024 ),
        m_permillageSmoothIo( 0 ),
        m_fBlockCacheEnabled( fFalse )
{
}

//ULONG CDefaultFileSystemConfiguration::CbZeroExtend()
//{
//    return m_cbZeroExtend;
//}

ULONG CDefaultFileSystemConfiguration::DtickAccessDeniedRetryPeriod()
{
    return m_dtickAccessDeniedRetryPeriod;
}

//ULONG CDefaultFileSystemConfiguration::CIOMaxOutstanding()
//{
//    return m_cIOMaxOutstanding;
//}
//
//ULONG CDefaultFileSystemConfiguration::CIOMaxOutstandingBackground()
//{
//    return m_cIOMaxOutstandingBackground;
//}

ULONG CDefaultFileSystemConfiguration::DtickHungIOThreshhold()
{
    return m_dtickHungIOThreshhold;
}

DWORD CDefaultFileSystemConfiguration::GrbitHungIOActions()
{
    return m_grbitHungIOActions;
}

ULONG CDefaultFileSystemConfiguration::CbMaxReadSize()
{
    return m_cbMaxReadSize;
}

ULONG CDefaultFileSystemConfiguration::CbMaxWriteSize()
{
    return m_cbMaxWriteSize;
}

ULONG CDefaultFileSystemConfiguration::CbMaxReadGapSize()
{
    return m_cbMaxReadGapSize;
}

ULONG CDefaultFileSystemConfiguration::PermillageSmoothIo()
{
    return m_permillageSmoothIo;
}

BOOL CDefaultFileSystemConfiguration::FBlockCacheEnabled()
{
    return m_fBlockCacheEnabled;
}

ERR CDefaultFileSystemConfiguration::ErrGetBlockCacheConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig )
{
    ERR err = JET_errSuccess;

    Alloc( *ppbcconfig = new CDefaultBlockCacheConfiguration() );

HandleError:
    return err;
}

void CDefaultFileSystemConfiguration::EmitEvent(    const EEventType    type,
                                                    const CategoryId    catid,
                                                    const MessageId     msgid,
                                                    const DWORD         cString,
                                                    const WCHAR *       rgpwszString[],
                                                    const LONG          lEventLoggingLevel )
{
}

void CDefaultFileSystemConfiguration::EmitEvent(    int                 haTag,
                                                    const CategoryId    catid,
                                                    const MessageId     msgid,
                                                    const DWORD         cString,
                                                    const WCHAR *       rgpwszString[],
                                                    int                 haCategory,
                                                    const WCHAR*        wszFilename,
                                                    unsigned _int64     qwOffset,
                                                    DWORD               cbSize )
{
}

void CDefaultFileSystemConfiguration::EmitFailureTag(   const int           haTag,
                                                        const WCHAR* const  wszGuid,
                                                        const WCHAR* const  wszAdditional )
{
}

const void* const CDefaultFileSystemConfiguration::PvTraceContext()
{
    return NULL;
}

CDefaultFileSystemConfiguration g_fsconfigDefault;
CFileIdentification g_fident;
CCacheTelemetry g_ctm;
CCacheRepository g_crep( &g_fident, &g_ctm );

ERR ErrOSFSCreate( _Out_ IFileSystemAPI** const ppfsapi )
{
    return ErrOSFSCreate( NULL, ppfsapi );
}

ERR ErrOSFSCreate(  _In_ IFileSystemConfiguration * const   pfsconfig,
                    _Out_ IFileSystemAPI** const            ppfsapi )
{
    ERR                             err         = JET_errSuccess;
    IFileSystemConfiguration* const pfsconfigT  = pfsconfig ? pfsconfig : &g_fsconfigDefault;
    IFileSystemAPI*                 pfsapi      = NULL;

    //  create the file system

    Alloc( pfsapi = new COSFileSystem( pfsconfigT ) );

    *ppfsapi = pfsapi;
    pfsapi = NULL;

    //  if the block cache is enabled then wrap this in its file system filter

    if ( pfsconfigT->FBlockCacheEnabled() )
    {
        Alloc( pfsapi = new CFileSystemFilter( pfsconfigT, ppfsapi, &g_fident, &g_ctm, &g_crep ) );

        *ppfsapi = pfsapi;
        pfsapi = NULL;
    }

HandleError:
    delete pfsapi;
    if ( err < JET_errSuccess )
    {
        delete *ppfsapi;
        *ppfsapi = NULL;
    }
    return err;
}


// A volume is sort of an oddness ... it is 1/2 way between a OS File System and an OS Disk, but
// today ultimately our concept of a volume is a construction of the Operating System itself, so
// we are going to put it in the file system component.


// =============================================================================================
//  OS Volume Subsystem
// =============================================================================================
//
//

//  The List of all volumes attached or connected to the engine
//
CSXWLatch g_sxwlOSVolume( CLockBasicInfo( CSyncBasicInfo( "OS Volume SXWL" ), rankOSVolumeSXWL, 0 ) );

CInvasiveList< COSVolume, COSVolume::OffsetOfILE >  g_ilVolumeList;

LOCAL OSDiskMappingMode g_diskMode = eOSDiskInvalidMode;
INLINE OSDiskMappingMode GetDiskMappingMode()
{

    return g_diskMode;
}

INLINE void SetDiskMappingMode( const OSDiskMappingMode diskMode )
{
    g_diskMode = diskMode;
}

// --------------------------------------------
// COSVolume / OS Volume
//

//  ctor/dtor

COSVolume::COSVolume() :
    m_cref( 0 ),
    m_posd( NULL ),
    m_eosDiskType( DRIVE_UNINIT_ESE )
{
    Assert( 0 == CRef() );
    m_wszVolPath[0] = L'\0';
    m_wszVolCanonicalPath[0] = L'\0';
}

COSVolume::~COSVolume()
{
    Assert( 0 == CRef() );
    Assert( NULL == m_posd );
}

ERR COSVolume::ErrInitVolume( __in_z const WCHAR * const wszVolPath, __in_z const WCHAR * const wszVolCanonicalPath )
{
    if ( ( ( 1 + LOSStrLengthW( wszVolPath ) ) * 2 ) >= sizeof( m_wszVolPath ) ||
            ( ( 1 + LOSStrLengthW( wszVolCanonicalPath ) ) * 2 ) >= sizeof( m_wszVolCanonicalPath ) )
    {
        AssertSz( fFalse, "We expect this to be protected by the out lying layer, but protect anyway" );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    OSStrCbCopyW( m_wszVolPath, sizeof(m_wszVolPath), wszVolPath );
    OSStrCbCopyW( m_wszVolCanonicalPath, sizeof(m_wszVolCanonicalPath), wszVolCanonicalPath );

    return JET_errSuccess;
}

void COSVolume::AssertValid()
{
    Assert( NULL != this );
    // We only call this on initialized volumes, a few things should be verifiable.
    Assert( CRef() );
    Assert( m_wszVolPath[0] != L'\0' );
    Assert( m_wszVolCanonicalPath[0] != L'\0' );
    Assert( m_posd );
}

BOOL COSVolume::FIsVolume( __in_z const WCHAR * const wszTargetVolume ) const
{
    return ( 0 == LOSStrCompareW( m_wszVolCanonicalPath, wszTargetVolume ) );
}

//  returns the amount of free/total disk space on the drive hosting the specified path

ERR COSVolume::ErrDiskSpace(    const WCHAR* const  wszPath,
                                    QWORD* const        pcbFreeForUser,
                                    QWORD* const        pcbTotalForUser,
                                    QWORD* const        pcbFreeOnDisk )
{
    AssertSz( fFalse, __FUNCTION__ " NYI!" );
    return ErrERRCheck( JET_wrnNyi );
}


//  returns the sector size for the specified path
ERR COSVolume::ErrFileSectorSize(   const WCHAR* const  wszPath,
                                    DWORD* const        pcbSize )
{
    AssertSz( fFalse, __FUNCTION__ " NYI!" );
    return ErrERRCheck( JET_wrnNyi );
}

//  returns the atomic write size for the specified path

ERR COSVolume::ErrFileAtomicWriteSize(  const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize )
{
    AssertSz( fFalse, __FUNCTION__ "NYI!" );
    return ErrERRCheck( JET_wrnNyi );
}

#ifndef OS_LAYER_VIOLATIONS //  for unit testing only ...
//  We use this to test other pretend system drives.
WCHAR g_rgwchSysDriveTesting[IFileSystemAPI::cchPathMax] = { 0 };
#endif

BOOL COSVolume::FDiskFixed()
{

    if ( m_eosDiskType == DRIVE_UNINIT_ESE )
    {
        UINT eosDiskType = GetDriveTypeW( m_wszVolPath );
        Assert( eosDiskType != DRIVE_UNINIT_ESE );      //  very unlikely the OS would pick this enum for anything
        if ( eosDiskType == DRIVE_UNKNOWN )
        {
            eosDiskType = GetDriveTypeW( m_wszVolCanonicalPath );
            Assert( eosDiskType != DRIVE_UNINIT_ESE );  //  very unlikely the OS would pick this enum for anything
        }
        if ( eosDiskType == DRIVE_FIXED )
        {
            //  TURNS OUT that simple USB disks show up as DRIVE_FIXED ... so we're going to pick
            //  the safest option possible and only let files running of the OS's windows / system
            //  drive be considered local for now.

            eosDiskType = DRIVE_UNKNOWN;        //  presumed innocent ...

            WCHAR wszWinDir[IFileSystemAPI::cchPathMax];
            if ( GetWindowsDirectoryW( wszWinDir, _countof(wszWinDir) ) )
            {
#ifndef OS_LAYER_VIOLATIONS //  for unit testing only ...
                //  "config" injection of a string, so we can test the ErrOSVolumeConnect() 
                if ( g_rgwchSysDriveTesting[0] != L'\0' )
                {
                    OSStrCbCopyW( wszWinDir, sizeof(wszWinDir), g_rgwchSysDriveTesting );
                }
#endif

                IFileSystemAPI * pfsapi = NULL;
                if( ErrOSFSCreate( &pfsapi ) >= JET_errSuccess )
                {
                    IVolumeAPI * posvDir = NULL;
                    Assert( eOSDiskOneDiskPerPhysicalDisk == GetDiskMappingMode() );

                    if( ErrOSVolumeConnect( (COSFileSystem*)pfsapi, wszWinDir, &posvDir ) >= JET_errSuccess )
                    {
                        ULONG_PTR ulpDiskIdDir = NULL;
                        //  SOMEONE, unncessarily made this return an ERR that is always success, fine handle it.
                        if( posvDir->ErrDiskId( &ulpDiskIdDir ) >= JET_errSuccess )
                        {
                            ULONG_PTR ulpDiskIdTarget = NULL;
                            if( ErrDiskId( &ulpDiskIdTarget ) >= JET_errSuccess )
                            {
                                eosDiskType = ( ulpDiskIdTarget == ulpDiskIdDir ) ? DRIVE_FIXED : DRIVE_UNKNOWN;
                            }
                        }
                        OSVolumeDisconnect( posvDir );
                    }
                    delete pfsapi;
                }
            }

#if defined(DEBUG) && defined(OS_LAYER_VIOLATIONS)  //  in debug ese.dll/esent.dll
            //  Override with mostly fixed (66%), so we can test both paths and to allow us to consider an occasional
            //  network path as fixed, we'll see what that yields in random IO AVs elsewhere in ESE.
            eosDiskType = ( rand() % 3 ) ? DRIVE_FIXED : DRIVE_UNKNOWN;
#endif
        }

        eosDiskType = (UINT)UlConfigOverrideInjection( 46860, eosDiskType );

        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Volume=%ws, eosDiskType = %d, fFixed = %d", m_wszVolPath, eosDiskType, eosDiskType == DRIVE_FIXED ) );
        if ( eosDiskType != DRIVE_FIXED )
        {
            //  not a true IO problem, but want heightened awareness ...
            OSTrace( JET_tracetagIOProblems, OSFormat( "WARNING: Volume=%ws is not detected as fixed, MemoryMapping operations will all be forcibly COW'd", m_wszVolPath ) );
        }

        Assert( eosDiskType != DRIVE_UNINIT_ESE );

        m_eosDiskType = eosDiskType;
    }

    return ( m_eosDiskType == DRIVE_FIXED );
}

ERR COSVolume::ErrDiskId( ULONG_PTR* const pulDiskId ) const
{
    ERR err = JET_errSuccess;
    
    Assert( pulDiskId != NULL );

    Call( m_posd->ErrDiskId( pulDiskId ) );

HandleError:

    return err;
}

void COSVolume::AddRef()
{
    Assert( g_sxwlOSVolume.FOwnExclusiveLatch() || g_sxwlOSVolume.FOwnWriteLatch() );
    m_cref++;
}

void COSVolume::Release()
{
    Assert( g_sxwlOSVolume.FOwnExclusiveLatch() || g_sxwlOSVolume.FOwnWriteLatch() );
    m_cref--;
}

//  returns the number of referrers

ULONG COSVolume::CRef() const
{
    return m_cref;
}

const WCHAR * COSVolume::WszVolPath() const
{
    return m_wszVolPath;
}

const WCHAR * COSVolume::WszVolCanonicalPath() const
{
    return m_wszVolCanonicalPath;
}

//  this is the name for the one disk in single disk mode.

const WCHAR * const wszLastDiskOnEarth = L"ThereCanBeOnlyOne";

ERR COSVolume::ErrGetDisk( COSDisk ** pposd )
{
    Assert( m_posd != NULL );
    return ErrOSDiskConnect( m_posd->WszDiskPathId(), m_posd->DwDiskNumber(), (IDiskAPI**)pposd );
}


// --------------------------------------------
//  OS Volume Global Connect / Disconnect
//

ERR ErrOSVolumeICreate(
    __in_z const WCHAR * const  wszVolPath,
    __in_z const WCHAR * const  wszVolCanonicalPath,
    _Out_ COSVolume **          pposv,
    __in_opt IDiskAPI * const   pdiskapi
    )
{
    ERR     err = JET_errSuccess;

    COSVolume * posv = NULL;

    Assert( g_sxwlOSVolume.FOwnExclusiveLatch() );

    CSXWLatch::ERR errSXW = g_sxwlOSVolume.ErrUpgradeExclusiveLatchToWriteLatch();
    if ( CSXWLatch::ERR::errWaitForWriteLatch == errSXW )
    {
        g_sxwlOSVolume.WaitForWriteLatch();
        errSXW = CSXWLatch::ERR::errSuccess;
    }
    Assert( errSXW == CSXWLatch::ERR::errSuccess );

    Call( ErrFaultInjection( 17376 ) );
    Alloc( posv = new COSVolume() );

    Call( posv->ErrInitVolume( wszVolPath, wszVolCanonicalPath ) );

    //  From this point on, must not fail.

    posv->AddRef();

    g_ilVolumeList.InsertAsPrevMost( posv );

    Assert( 1 == posv->CRef() );

    *pposv = posv;
    err = JET_errSuccess;

HandleError:

    if ( err < JET_errSuccess )
    {
        delete posv;
        Assert( NULL == *pposv );
    }

    g_sxwlOSVolume.DowngradeWriteLatchToExclusiveLatch();

    return err;
}

ERR ErrOSVolumeIRetrieve( _In_ PCWSTR wszVolPath, _Out_ COSVolume ** pposv )
{
    COSVolume * posv = NULL;

    Assert( g_sxwlOSVolume.FOwnExclusiveLatch() || g_sxwlOSVolume.FOwnWriteLatch() );

    *pposv = NULL;

    for ( posv = g_ilVolumeList.PrevMost(); posv != NULL; posv = g_ilVolumeList.Next( posv ) )
    {
        if ( posv->FIsVolume( wszVolPath ) )
        {
            *pposv = posv;
            return JET_errSuccess;
        }
    }

    return errNotFound;
}

ERR ErrOSVolumeConnect(
    _In_ COSFileSystem * const      posfs,
    __in_z const WCHAR * const      wszFilePath,
    _Out_ IVolumeAPI **             ppvolapi
    )
{
    ERR err                                 = JET_errSuccess;
    COSVolume* posv                         = NULL;
    WCHAR wszVolumeCanonicalPath[OSFSAPI_MAX_PATH];
    WCHAR wszDiskId[OSFSAPI_MAX_PATH];
    DWORD dwDiskNumber;

    Assert( wszFilePath );
    Assert( ppvolapi );

    COSVolume * posvCreated = NULL;
    *ppvolapi = NULL;

    //  Retrieve root path from file path name.

    WCHAR wszVolumePath[OSFSAPI_MAX_PATH];

    CallR( ErrFaultInjection( 17380 ) );
    CallR( posfs->ErrPathRoot( wszFilePath, wszVolumePath ) );

    //  Retrieve the volume canonical path and disk ID.

    posfs->PathVolumeCanonicalAndDiskId(    wszVolumePath,
                                            wszVolumeCanonicalPath, _countof( wszVolumeCanonicalPath ),
                                            wszDiskId, _countof( wszDiskId ),
                                            &dwDiskNumber );

    //  Decide on the volume to use. Possibly override what we had determined.

    const WCHAR * wszVolumeCanonicalPathToUse = wszVolumeCanonicalPath;
    
    switch( GetDiskMappingMode() )
    {
        case eOSDiskLastDiskOnEarthMode:
        case eOSDiskOneDiskPerVolumeMode:
        case eOSDiskOneDiskPerPhysicalDisk:
            break;

        case eOSDiskMonogamousMode:
            wszVolumeCanonicalPathToUse = wszFilePath;
            break;

        default:
            AssertSz( fFalse, "Code problem" );
            Call( ErrERRCheck( JET_errInternalError ) );
            break;
    }

    g_sxwlOSVolume.AcquireExclusiveLatch();

    CallSx( err = ErrOSVolumeIRetrieve( wszVolumeCanonicalPathToUse, (COSVolume**)ppvolapi ), errNotFound );
    Assert( JET_errSuccess == err || NULL == *ppvolapi &&
                JET_errSuccess != err || NULL != *ppvolapi );

    const WCHAR * wszDiskIdToUse = wszDiskId;

    if ( *ppvolapi )
    {
        COSVolume * const posvAcquired = ((COSVolume*)*ppvolapi);
        posvAcquired->AddRef();
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Add Ref Volume p=%p, Path=%ws, CanonicalPath=%ws, Cref=%d", posvAcquired, posvAcquired->WszVolPath(), posvAcquired->WszVolCanonicalPath(), posvAcquired->CRef() ) );
        posv = PosvFromVolAPI( *ppvolapi );
        Assert( posv->m_posd != NULL );
        wszDiskIdToUse = posv->m_posd->WszDiskPathId();
    }
    else
    {
        Call( ErrOSVolumeICreate( wszVolumePath, wszVolumeCanonicalPathToUse, &posvCreated, NULL ) );
        Assert( NULL == posvCreated->m_posd );
        *ppvolapi = posvCreated;
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Created Volume p=%p, Path=%ws, CanonicalPath=%ws", posvCreated, posvCreated->WszVolPath(), posvCreated->WszVolCanonicalPath() ) );
        Assert( *ppvolapi );
        posv = PosvFromVolAPI( *ppvolapi );

        switch( GetDiskMappingMode() )
        {
            case eOSDiskLastDiskOnEarthMode:
                wszDiskIdToUse = wszLastDiskOnEarth;
                break;

            case eOSDiskOneDiskPerVolumeMode:
            case eOSDiskMonogamousMode:
                wszDiskIdToUse = posv->WszVolCanonicalPath();
                break;

            case eOSDiskOneDiskPerPhysicalDisk:
                break;

            default:
                AssertSz( fFalse, "Code problem" );
                Call( ErrERRCheck( JET_errInternalError ) );
                break;
        }
    }

    Assert( wszDiskIdToUse != NULL );

    IDiskAPI** const ppidiskapi = (IDiskAPI**)&( posv->m_posd );
    OnDebug( const IDiskAPI* const pidiskapi = *ppidiskapi; );
    Call( ErrOSDiskConnect( wszDiskIdToUse, dwDiskNumber, ppidiskapi ) );
    Assert( ( pidiskapi == NULL && *ppidiskapi != NULL ) || pidiskapi == *ppidiskapi );

    err = JET_errSuccess;

HandleError:

    if ( err < JET_errSuccess )
    {
        if ( *ppvolapi )
        {
            // We've already incremented the ref count.
            ((COSVolume*)(*ppvolapi))->Release();
        }

        //  We created .
        if ( posvCreated )
        {
            Assert( NULL == posvCreated->m_posd );
            g_ilVolumeList.Remove( posvCreated );
            delete posvCreated;
        }

        *ppvolapi = NULL;
    }

    g_sxwlOSVolume.ReleaseExclusiveLatch();

    return err;
}

void OSVolumeDisconnect(
    __inout IVolumeAPI *            pvolapi
    )
{
    Assert( pvolapi );

    COSVolume * posv = (COSVolume*)pvolapi;

    g_sxwlOSVolume.AcquireWriteLatch();

    Assert( g_ilVolumeList.FMember( posv ) );

    OSDiskDisconnect( posv->m_posd, NULL );

    posv->Release();

    if ( 0 == posv->CRef() )
    {
        g_ilVolumeList.Remove( posv );
        posv->m_posd = NULL;
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Deleted Volume p=%p, Path=%ws, CanonicalPath=%ws", posv, posv->WszVolPath(), posv->WszVolCanonicalPath() ) );
        delete posv;
    }
    else
    {
        ASSERT_VALID( posv );
        ASSERT_VALID( posv->m_posd );
        //  Should remain at least one reference to the disk as the volume is still open.
        Assert( posv->m_posd->CRef() );
        OSTrace( JET_tracetagDiskVolumeManagement, OSFormat( "Del Ref Volume p=%p, Path=%ws, CanonicalPath=%ws, Cref=%d", posv, posv->WszVolPath(), posv->WszVolCanonicalPath(), posv->CRef() ) );
    }

    g_sxwlOSVolume.ReleaseWriteLatch();
}


//  FileModeFlags translation
//
//  We basically allow 3 + 1 external modes of concurrent permissioning / operation:
//  Parts of this are a layering violation, but it is the only way to explain our modes.
//      Mode A: RW / Exclusive mode [Default = fmfNone]
//          In this mode we get all _READ, _WRITE AND DELETE access and specify share of 0, so it 
//          is fully read-write and excludes all other modes.  This is the default behavior and
//          how we open all our files in do-time, the database, current edb.log, etc.
//      Mode B: Classic RO Mode / fmfReadOnly
//          In this mode we are only getting _READ access, and we must also allow the _SHARE_READ 
//          flag to alow us to operate concurrently with Mode D / HA.
//          Note: Most simple utilities, extemporaneous file header reads operate off such files.
//      Mode C: Read-Flush Mode / fmfReadOnlyClient
//          In this mode we don't really want to do true write IO, but do want to be able to call flush
//          file buffers (which does require _WRITE access). So we open it with _WRITE and _READ, but
//          not DELETE access.  We also use same sharing as Mode B, so that we can interop cleanly with
//          Mode D.
//          Note: This is primarily used in log recovery redo to force flushes to logs before we consume
//          them in the database file.
//      Mode D: Read-Only Permissive Mode / JetGetLogFileInfo / also used external by HA (primarily for HA)
//          In this mode used by HA external to ESE, they open the file with _READ access, and
//          with BOTH _SHARE_READ AND _SHARE_WRITE, the later being required to operate while ESE
//          is has a file open in Mode C / Read-Flush.
//
//  Note: You can not use only _SHARE_READ to interop with Mode C, because it opens the file with GENERIC_WRITE
//  access.
//
//  Note: Also using _SHARE_WRITE in Mode C does not allow concurrent operation with Mode A for two reasons
//  actually, (1) mainly because Mode A passes 0 for dwShareMode, meaning no concurrent access with
//  anyone, but also (2) Mode A gets DELETE access and Mode C doesn't specify _SHARE_DELETE as well.
//

DWORD DwDesiredAccessFromFileModeFlags( const IFileAPI::FileModeFlags fmf )
{
    return ( fmf & IFileAPI::fmfReadOnly ) ?
        ( GENERIC_READ ) :                          //  Mode B: Classic RO mode.
        ( fmf & IFileAPI::fmfReadOnlyClient ) ?
        ( GENERIC_READ | GENERIC_WRITE ) :          //  Mode C: Newer Read-Flush mode.
        ( fmf & IFileAPI::fmfReadOnlyPermissive ) ?
        ( GENERIC_READ ) :                          //  Mode D: ReadOnly-Permissive mode.
        ( GENERIC_READ | GENERIC_WRITE | DELETE );  //  Mode A: RW / Exclusive mode [Default = fmfNone]
}

DWORD DwShareModeFromFileModeFlags( const IFileAPI::FileModeFlags fmf )
{
    return ( fmf & IFileAPI::fmfReadOnly ) ?
        ( FILE_SHARE_READ ) :                       //  Mode B: Classic RO mode.
        ( fmf & IFileAPI::fmfReadOnlyClient ) ?
        ( FILE_SHARE_READ ) :                       //  Mode C: Newer Read-Flush mode.
        ( fmf & IFileAPI::fmfReadOnlyPermissive ) ?
        ( FILE_SHARE_READ | FILE_SHARE_WRITE ) :    //  Mode D: ReadOnly-Permissive mode.
        ( 0 );                                      //  Mode A: RW / Exclusive mode [Default = fmfNone]
}

DWORD DwCreationDispositionFromFileModeFlags( const BOOL fCreate, const IFileAPI::FileModeFlags fmf )
{
    return fCreate ?
        ( ( fmf & IFileAPI::fmfOverwriteExisting ) ? CREATE_ALWAYS : CREATE_NEW ) :
        ( ( fmf & IFileAPI::fmfOverwriteExisting ) ? TRUNCATE_EXISTING : OPEN_EXISTING );
}

DWORD DwFlagsAndAttributesFromFileModeFlags( const IFileAPI::FileModeFlags fmf )
{
    return 
        FILE_ATTRIBUTE_NORMAL |
        ( ( fmf & IFileAPI::fmfTemporary ) ? FILE_FLAG_DELETE_ON_CLOSE : 0 ) |
        FILE_FLAG_OVERLAPPED |
        ( ( ( fmf & IFileAPI::fmfLossyWriteBack ) || ( fmf & IFileAPI::fmfStorageWriteBack ) ) ? 0 : FILE_FLAG_WRITE_THROUGH ) |
        ( ( fmf & IFileAPI::fmfCached ) ? FILE_FLAG_RANDOM_ACCESS : FILE_FLAG_NO_BUFFERING );
}


#ifdef ESENT

// Defined in $(BASE_INC_PATH)
#include <appmodel.h>
#include <statemanager.h>

#else

typedef void * HSTATE;
#define INVALID_STATE_HANDLE NULL


typedef enum tag_STATE_PERSIST_ATTRIB
{
    STATE_PERSIST_UNDEFINED = 0,
    STATE_PERSIST_LOCAL,
    STATE_PERSIST_ROAMING,
    STATE_PERSIST_TEMP,
    STATE_PERSIST_LAST
}
STATE_PERSIST_ATTRIB;

WINBASEAPI
// 2012/03/23 Exchange does not yet have SALv2:
// _Check_return_ _Success_(return != INVALID_STATE_HANDLE)
HSTATE
WINAPI
OpenState(
    );

WINBASEAPI
// 2012/03/23 Exchange does not yet have SALv2:
// _Success_(return != FALSE)
BOOL
WINAPI
CloseState(
    _In_ HSTATE hState
    );


WINBASEAPI
// 2012/03/23 Exchange does not yet have SALv2:
// _Check_return_ _Success_(return != FALSE)
BOOL
WINAPI
GetStateFolder(
    _In_ HSTATE hState,
    _In_ STATE_PERSIST_ATTRIB persistAttrib,
    _Out_writes_opt_(*pPathCch) LPWSTR pPath,
    _Inout_ PUINT32 pPathCch
    );
#endif

template< typename Ret >
INLINE Ret HandleFailedWithGLE()
{
    SetLastError( ErrorThunkNotSupported() );
    return Ret( INVALID_HANDLE_VALUE );
}

#define NTOSFuncHandle( g_pfn, mszzDlls, func, oslf )               \
            FunctionLoader<decltype(&func)> g_pfn( HandleFailedWithGLE, mszzDlls, #func, oslf );

LOCAL ERR ErrOSFSInitDefaultPath()
{
    ERR err = JET_errSuccess;
    WCHAR* wszDefaultPath = NULL;
    HSTATE hstate = INVALID_STATE_HANDLE;
    NTOSFuncStd( pfnCloseState, g_mwszzAppModelStateLibs, CloseState, oslfExpectedOnWin8 | oslfStrictFree );

    if ( !g_wszDefaultPath )
    {
        Alloc( wszDefaultPath = new WCHAR[ g_cchDefaultPath ] );

        if ( FUtilProcessIsPackaged() )
        {
            NTOSFuncStd( pfnGetStateFolder, g_mwszzAppModelStateLibs, GetStateFolder, oslfExpectedOnWin8 | oslfStrictFree );
            NTOSFuncHandle( pfnOpenState, g_mwszzAppModelStateLibs, OpenState, oslfExpectedOnWin8 | oslfStrictFree );

            // The State-functions should be present if the OS supports Packaged Processes.

            Assert( pfnGetStateFolder.ErrIsPresent() == JET_errSuccess );
            Assert( pfnOpenState.ErrIsPresent() == JET_errSuccess );
            Assert( pfnCloseState.ErrIsPresent() == JET_errSuccess );

            Assert( g_cbDefaultPath % sizeof( g_wszDefaultPath[ 0 ] ) == 0 );

            hstate = pfnOpenState();

            if ( hstate == INVALID_STATE_HANDLE )
            {
                goto HandleError;
            }
            UINT32 cchFolderRequired = g_cchDefaultPath;
            const BOOL fRc = pfnGetStateFolder( hstate, STATE_PERSIST_LOCAL, wszDefaultPath, &cchFolderRequired );
            if ( !fRc )
            {
                err = ErrOSErrFromWin32Err( GetLastError() );
                goto HandleError;
            }

            OnDebug( IFileSystemAPI * pfsapi = NULL );
            OnDebug( (void) ErrOSFSCreate( &pfsapi ) );

            Assert( NULL == pfsapi || !pfsapi->FPathIsRelative( wszDefaultPath ) );
            OnDebug( delete pfsapi );

            // Append a trailing backslash:

            const size_t ichLastChar = cchFolderRequired;

            AssertSz( ichLastChar > 0, "GetStateFolder() should have returned a non-null string." );
            if ( ichLastChar > 0 && wszDefaultPath[ ichLastChar - 1 ] != L'\\' )
            {
                // Append the character and ensure NULL-termination.

                wszDefaultPath[ ichLastChar - 1 ] = L'\\';
                wszDefaultPath[ ichLastChar ] = L'\0';
            }

            g_fCanProcessUseRelativePaths = fFalse;
        }
        else
        {
            OSStrCbCopyW( wszDefaultPath, g_cbDefaultPath, L".\\" );
        }

        WCHAR* wszOldValue = (WCHAR*) AtomicCompareExchangePointer( (void**) &g_wszDefaultPath, NULL, wszDefaultPath );
        if ( wszOldValue == NULL )
        {
            // This means we've successfully transferred our buffer to the global
            // variable.

            wszDefaultPath = NULL;
        }
        else
        {
            // Some other thread beat us to it. Leave wszDefaultPath as non-NULL,
            // and the HandleError block below will free it.
        }
    }

HandleError:
    if ( INVALID_STATE_HANDLE != hstate )
    {
        Assert( pfnCloseState.ErrIsPresent() == JET_errSuccess );
        if ( pfnCloseState.ErrIsPresent() >= 0 )
        {
            pfnCloseState( hstate );
        }
    }

    // If the function was successful, then wszDefaultPath was pased off to g_wszDefaultPath.

    delete [] wszDefaultPath;

    return err;
}



//  post-terminate file subsystem

void OSFSPostterm()
{
    //  The proper location for this is OSFSTerm(), but the unit tests do not
    //  always call OSFSTerm(), but they do call OSFSPostterm().

    delete [] g_wszDefaultPath;
    g_wszDefaultPath = NULL;
}

//  pre-init file subsystem

BOOL FOSFSPreinit()
{
    //  nop

    return fTrue;
}

ERR ErrOSFSInit()
{
    ERR err = JET_errSuccess;

    CalculateCurrentProcessIsPackaged();

    // Calculate the State directory.

    Call( ErrOSFSInitDefaultPath() );

HandleError:
    return err;
}

VOID OSFSTerm()
{
    delete [] g_wszDefaultPath;
    g_wszDefaultPath = NULL;
}
