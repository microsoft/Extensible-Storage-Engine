// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "stdafx.h"

#pragma once

namespace Internal
{
    namespace Ese
    {
        namespace BlockCache
        {
            namespace Interop
            {
                template< class TM, class TN >
                class CFileSystemWrapper : public CWrapper<TM, TN>
                {
                    public:

                        CFileSystemWrapper( TM^ fs ) : CWrapper( fs ) { }

                    public:

                        ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                                            QWORD* const        pcbFreeForUser,
                                            QWORD* const        pcbTotalForUser = NULL,
                                            QWORD* const        pcbFreeOnDisk = NULL ) override;


                        ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                                DWORD* const        pcbSize ) override;

                        ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                                    DWORD* const        pcbSize ) override;

                        ERR ErrGetLastError( const DWORD error ) override;

                        ERR ErrPathRoot(    const WCHAR* const                                              wszPath,
                                            __out_bcount( OSFSAPI_MAX_PATH * sizeof( WCHAR ) ) WCHAR* const wszAbsRootPath ) override;

                        void PathVolumeCanonicalAndDiskId(  const WCHAR* const                                  wszVolumePath,
                                                            __out_ecount( cchVolumeCanonicalPath ) WCHAR* const wszVolumeCanonicalPath,
                                                            __in const DWORD                                    cchVolumeCanonicalPath,
                                                            __out_ecount( cchDiskId ) WCHAR* const              wszDiskId,
                                                            __in const DWORD                                    cchDiskId,
                                                            __out DWORD *                                       pdwDiskNumber ) override;

                        ERR ErrPathComplete(    _In_z_ const WCHAR* const                           wszPath,
                                                _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const   wszAbsPath ) override;
                        ERR ErrPathParse(   const WCHAR* const                                          wszPath,
                                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFolder,
                                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileBase,
                                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileExt ) override;
                        const WCHAR * const WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const override;
                        ERR ErrPathBuild(   __in_z const WCHAR* const                                   wszFolder,
                                            __in_z const WCHAR* const                                   wszFileBase,
                                            __in_z const WCHAR* const                                   wszFileExt,
                                            __out_bcount_z(cbPath) WCHAR* const                         wszPath,
                                            __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG  cbPath ) override;
                        ERR ErrPathFolderNorm(  __inout_bcount(cbSize)  PWSTR const wszFolder,
                                                DWORD                               cbSize ) override;
                        BOOL FPathIsRelative(_In_ PCWSTR wszPath ) override;

                        ERR ErrPathExists(  _In_ PCWSTR     wszPath,
                                            _Out_opt_ BOOL* pfIsDirectory ) override;

                        ERR ErrPathFolderDefault(   _Out_z_bytecap_(cbSize) PWSTR const wszFolder,
                                                    _In_ DWORD                          cbSize,
                                                    _Out_ BOOL *                        pfIsDefaultDirectory ) override;


                        ERR ErrFolderCreate( const WCHAR* const wszPath ) override;
                        ERR ErrFolderRemove( const WCHAR* const wszPath ) override;

                        ERR ErrFileFind(    const WCHAR* const      wszFind,
                                            IFileFindAPI** const    ppffapi ) override;

                        ERR ErrFileDelete( const WCHAR* const wszPath ) override;
                        ERR ErrFileMove(    const WCHAR* const  wszPathSource,
                                            const WCHAR* const  wszPathDest,
                                            const BOOL          fOverwriteExisting  = fFalse ) override;
                        ERR ErrFileCopy(    const WCHAR* const  wszPathSource,
                                            const WCHAR* const  wszPathDest,
                                            const BOOL          fOverwriteExisting  = fFalse ) override;

                        ERR ErrFileCreate(  _In_z_ const WCHAR* const               wszPath,
                                            _In_   const IFileAPI::FileModeFlags    fmf,
                                            _Out_  IFileAPI** const                 ppfapi ) override;

                        ERR ErrFileOpen(    _In_z_ const WCHAR* const               wszPath,
                                            _In_   const IFileAPI::FileModeFlags    fmf,
                                            _Out_  IFileAPI** const                 ppfapi ) override;

                    protected:

                        virtual ERR ErrWrap( IFile^% f, IFileAPI** const ppfapi );
                };

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrDiskSpace( const WCHAR * const wszPath,
                                                                    QWORD * const       pcbFreeForUser,
                                                                    QWORD * const       pcbTotalForUser,
                                                                    QWORD * const       pcbFreeOnDisk )

                {
                    ERR     err                 = JET_errSuccess;
                    Int64   bytesFreeForUser    = 0;
                    Int64   bytesTotalForUser   = 0;
                    Int64   bytesFreeOnDisk     = 0;

                    if ( pcbFreeForUser )
                    {
                        *pcbFreeForUser = 0;
                    }
                    if ( pcbTotalForUser )
                    {
                        *pcbTotalForUser = 0;
                    }
                    if ( pcbFreeOnDisk )
                    {
                        *pcbFreeOnDisk = 0;
                    }

                    ExCall( I()->DiskSpace( gcnew String( wszPath ),
                                            bytesFreeForUser,
                                            bytesTotalForUser,
                                            bytesFreeOnDisk ) );

                    if ( pcbFreeForUser )
                    {
                        *pcbFreeForUser = bytesFreeForUser;
                    }
                    if ( pcbTotalForUser )
                    {
                        *pcbTotalForUser = bytesTotalForUser;
                    }
                    if ( pcbFreeOnDisk )
                    {
                        *pcbFreeOnDisk = bytesFreeOnDisk;
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( pcbFreeForUser )
                        {
                            *pcbFreeForUser = 0;
                        }
                        if ( pcbTotalForUser )
                        {
                            *pcbTotalForUser = 0;
                        }
                        if ( pcbFreeOnDisk )
                        {
                            *pcbFreeOnDisk = 0;
                        }
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileSectorSize(    const WCHAR* const  wszPath,
                                                                            DWORD* const        pcbSize )
                {
                    ERR err         = JET_errSuccess;
                    int sizeInBytes = 0;

                    *pcbSize = 0;

                    ExCall( sizeInBytes = I()->FileSectorSize( gcnew String( wszPath ) ) );

                    *pcbSize = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbSize = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileAtomicWriteSize(   const WCHAR* const  wszPath,
                                                                                DWORD* const        pcbSize )
                {
                    ERR err         = JET_errSuccess;
                    int sizeInBytes = 0;

                    *pcbSize = 0;

                    ExCall( sizeInBytes = I()->FileAtomicWriteSize( gcnew String( wszPath ) ) );

                    *pcbSize = sizeInBytes;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbSize = 0;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrGetLastError( const DWORD error )
                {
                    EsentErrorException^ ex = I()->GetLastError( error );

                    return ex == nullptr ? JET_errSuccess : (ERR)(int)ex->Error;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathRoot(  const WCHAR* const                                              wszPath,
                                                                    __out_bcount( OSFSAPI_MAX_PATH * sizeof( WCHAR ) ) WCHAR* const wszAbsRootPath )
                {
                    ERR     err         = JET_errSuccess;
                    String^ absRootPath = nullptr;

                    wszAbsRootPath[ 0 ] = L'\0';

                    ExCall( absRootPath = I()->PathRoot( gcnew String( wszPath ) ) );

                    pin_ptr<const Char> wszAbsRootPathT = PtrToStringChars( absRootPath );
                    Call( ErrOSStrCbCopyW( wszAbsRootPath, OSFSAPI_MAX_PATH * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszAbsRootPathT ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        wszAbsRootPath[ 0 ] = L'\0';
                    }
                    return err;
                }

                template< class TM, class TN >
                inline void CFileSystemWrapper<TM,TN>::PathVolumeCanonicalAndDiskId(    const WCHAR* const                                  wszVolumePath,
                                                                                        __out_ecount( cchVolumeCanonicalPath ) WCHAR* const wszVolumeCanonicalPath,
                                                                                        __in const DWORD                                    cchVolumeCanonicalPath,
                                                                                        __out_ecount( cchDiskId ) WCHAR* const              wszDiskId,
                                                                                        __in const DWORD                                    cchDiskId,
                                                                                        __out DWORD *                                       pdwDiskNumber )
                {
                    String^ volumeCanonicalPath = nullptr;
                    String^ diskId              = nullptr;
                    int     diskNumber          = 0;

                    I()->PathVolumeCanonicalAndDiskId(  gcnew String( wszVolumePath ),
                                                        volumeCanonicalPath,
                                                        diskId,
                                                        diskNumber );

                    pin_ptr<const Char> wszVolumeCanonicalPathT = PtrToStringChars( volumeCanonicalPath );
                    OSStrCbCopyW( wszVolumeCanonicalPath, cchVolumeCanonicalPath * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszVolumeCanonicalPathT );
                    pin_ptr<const Char> wszDiskIdT = PtrToStringChars( diskId );
                    OSStrCbCopyW( wszDiskId, cchDiskId * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszDiskIdT );
                    *pdwDiskNumber = diskNumber;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathComplete(  _In_z_ const WCHAR* const                           wszPath,
                                                                        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const   wszAbsPath )
                {
                    ERR     err     = JET_errSuccess;
                    String^ absPath = nullptr;

                    wszAbsPath[ 0 ] = L'\0';

                    ExCall( absPath = I()->PathComplete( gcnew String( wszPath ) ) );

                    pin_ptr<const Char> wszAbsPathT = PtrToStringChars( absPath );
                    Call( ErrOSStrCbCopyW( wszAbsPath, OSFSAPI_MAX_PATH * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszAbsPathT ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        wszAbsPath[ 0 ] = L'\0';
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathParse( const WCHAR* const                                          wszPath,
                                                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFolder,
                                                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileBase,
                                                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileExt )
                {
                    ERR     err         = JET_errSuccess;
                    String^ folder      = nullptr;
                    String^ fileBase    = nullptr;
                    String^ fileExt     = nullptr;

                    if ( wszFolder )
                    {
                        wszFolder[ 0 ] = L'\0';
                    }
                    if ( wszFileBase )
                    {
                        wszFileBase[ 0 ] = L'\0';
                    }
                    if ( wszFileExt )
                    {
                        wszFileExt[ 0 ] = L'\0';
                    }

                    ExCall( I()->PathParse( gcnew String( wszPath ), folder, fileBase, fileExt ) );

                    if ( wszFolder && folder != nullptr )
                    {
                        pin_ptr<const Char> wszFolderT = PtrToStringChars( folder );
                        Call( ErrOSStrCbCopyW( wszFolder, OSFSAPI_MAX_PATH * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszFolderT ) );
                    }
                    if ( wszFileBase && fileBase != nullptr )
                    {
                        pin_ptr<const Char> wszFileBaseT = PtrToStringChars( fileBase );
                        Call( ErrOSStrCbCopyW( wszFileBase, OSFSAPI_MAX_PATH * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszFileBaseT ) );
                    }
                    if ( wszFileExt && fileExt != nullptr )
                    {
                        pin_ptr<const Char> wszFileExtT = PtrToStringChars( fileExt );
                        Call( ErrOSStrCbCopyW( wszFileExt, OSFSAPI_MAX_PATH * sizeof( WCHAR ), (STRSAFE_LPCWSTR)wszFileExtT ) );
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( wszFolder )
                        {
                            wszFolder[ 0 ] = L'\0';
                        }
                        if ( wszFileBase )
                        {
                            wszFileBase[ 0 ] = L'\0';
                        }
                        if ( wszFileExt )
                        {
                            wszFileExt[ 0 ] = L'\0';
                        }
                    }
                    return err;
                }

                template< class TM, class TN >
                inline const WCHAR * const CFileSystemWrapper<TM,TN>::WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const
                {

                    String^ optionalFullPath = wszOptionalFullPath ? gcnew String( wszOptionalFullPath ) : nullptr;
                    String^ pathFileName = I()->PathFileName( optionalFullPath );

                    if ( pathFileName->Length == 0 )
                    {
                        return L"";
                    }

                    if ( optionalFullPath->EndsWith( pathFileName ) )
                    {
                        return wszOptionalFullPath + optionalFullPath->Length - pathFileName->Length;
                    }

                    return L"UNKNOWN.PTH";
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathBuild( __in_z const WCHAR* const                                   wszFolder,
                                                                    __in_z const WCHAR* const                                   wszFileBase,
                                                                    __in_z const WCHAR* const                                   wszFileExt,
                                                                    __out_bcount_z(cbPath) WCHAR* const                         wszPath,
                                                                    __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG  cbPath )
                {
                    ERR     err     = JET_errSuccess;
                    String^ path    = nullptr;

                    if ( cbPath )
                    {
                        wszPath[ 0 ] = L'\0';
                    }

                    ExCall( path = I()->PathBuild(  gcnew String( wszFolder ),
                                                    gcnew String( wszFileBase ),
                                                    gcnew String( wszFileExt ) ) );

                    pin_ptr<const Char> wszPathT = PtrToStringChars( path );
                    Call( ErrOSStrCbCopyW( wszPath, cbPath, (STRSAFE_LPCWSTR)wszPathT ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( cbPath )
                        {
                            wszPath[ 0 ] = L'\0';
                        }
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathFolderNorm(    __inout_bcount( cbSize ) PWSTR const    wszFolder,
                                                                            DWORD                                   cbSize )
                {
                    ERR     err         = JET_errSuccess;
                    String^ folderIn    = gcnew String( wszFolder );
                    String^ folderOut   = nullptr;

                    ExCall( folderOut = I()->PathFolderNorm( folderIn ) );

                    pin_ptr<const Char> wszFolderT = PtrToStringChars( folderOut );
                    Call( ErrOSStrCbCopyW( wszFolder, cbSize, (STRSAFE_LPCWSTR)wszFolderT ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline BOOL CFileSystemWrapper<TM,TN>::FPathIsRelative( _In_ PCWSTR wszPath )
                {
                    return I()->PathIsRelative( gcnew String( wszPath ) ) ? fTrue : fFalse;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathExists(    _In_ PCWSTR     wszPath,
                                                                        _Out_opt_ BOOL* pfIsDirectory )
                {
                    ERR     err         = JET_errSuccess;
                    bool    isDirectory = false;

                    if ( pfIsDirectory )
                    {
                        *pfIsDirectory = fFalse;
                    }

                    ExCall( isDirectory = I()->PathExists( gcnew String( wszPath ) ) );
                    if ( pfIsDirectory )
                    {
                        *pfIsDirectory = isDirectory ? fTrue : fFalse;
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( pfIsDirectory )
                        {
                            *pfIsDirectory = fFalse;
                        }
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrPathFolderDefault( _Out_z_bytecap_(cbFolder) PWSTR const   wszFolder,
                                                                            _In_ DWORD                              cbFolder,
                                                                            _Out_ BOOL *                            pfCanProcessUseRelativePaths )
                {
                    ERR     err                         = JET_errSuccess;
                    String^ folder                      = nullptr;
                    bool    fCanProcessUseRelativePaths = false;

                    if ( cbFolder )
                    {
                        wszFolder[ 0 ] = L'\0';
                    }
                    *pfCanProcessUseRelativePaths = fFalse;

                    ExCall( folder = I()->PathFolderDefault( fCanProcessUseRelativePaths ) );

                    pin_ptr<const Char> wszFolderT = PtrToStringChars( folder );
                    Call( ErrOSStrCbCopyW( wszFolder, cbFolder, (STRSAFE_LPCWSTR)wszFolderT ) );
                    *pfCanProcessUseRelativePaths = fCanProcessUseRelativePaths ? fTrue : fFalse;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        if ( cbFolder )
                        {
                            wszFolder[ 0 ] = L'\0';
                        }
                        *pfCanProcessUseRelativePaths = fFalse;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFolderCreate( const WCHAR* const wszPath )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FolderCreate( gcnew String( wszPath ) ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFolderRemove( const WCHAR* const wszPath )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FolderRemove( gcnew String( wszPath ) ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileFind(  const WCHAR* const      wszFind,
                                                                    IFileFindAPI** const    ppffapi )
                {
                    ERR         err         = JET_errSuccess;
                    IFileFind^  ff          = nullptr;

                    *ppffapi = NULL;

                    ExCall( ff = I()->FileFind( gcnew String( wszFind ) ) );

                    Call( FileFind::ErrWrap( ff, ppffapi ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete ff;
                        delete *ppffapi;
                        *ppffapi = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileDelete( const WCHAR* const wszPath )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FileDelete( gcnew String( wszPath ) ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileMove(  const WCHAR* const  wszPathSource,
                                                                    const WCHAR* const  wszPathDest,
                                                                    const BOOL          fOverwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FileMove(  gcnew String( wszPathSource ),
                                            gcnew String( wszPathDest ),
                                            fOverwriteExisting ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileCopy(  const WCHAR* const  wszPathSource,
                                                                    const WCHAR* const  wszPathDest,
                                                                    const BOOL          fOverwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    ExCall( I()->FileCopy(  gcnew String( wszPathSource ),
                                            gcnew String( wszPathDest ),
                                            fOverwriteExisting ) );

                HandleError:
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileCreate(    _In_z_  const WCHAR* const              wszPath,
                                                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                                                        _Out_   IFileAPI** const                ppfapi )
                {
                    ERR     err     = JET_errSuccess;
                    IFile^  f       = nullptr;

                    *ppfapi = NULL;

                    ExCall( f = I()->FileCreate( gcnew String( wszPath ), (FileModeFlags)fmf ) );

                    Call( ErrWrap( f, ppfapi ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete f;
                        delete *ppfapi;
                        *ppfapi = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM,TN>::ErrFileOpen(  _In_z_  const WCHAR* const              wszPath,
                                                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                                                    _Out_   IFileAPI** const                ppfapi )
                {
                    ERR     err     = JET_errSuccess;
                    IFile^  f       = nullptr;

                    *ppfapi = NULL;

                    ExCall( f = I()->FileOpen(  gcnew String( wszPath ), (FileModeFlags)fmf ) );

                    Call( ErrWrap( f, ppfapi ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        delete f;
                        delete *ppfapi;
                        *ppfapi = NULL;
                    }
                    return err;
                }

                template< class TM, class TN >
                inline ERR CFileSystemWrapper<TM, TN>::ErrWrap( _In_ IFile^% f, _Out_ IFileAPI** const ppfapi )
                {
                    return File::ErrWrap( f, ppfapi );
                }
            }
        }
    }
}
