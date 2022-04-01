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
                template< class TM, class TN, class TW >
                public ref class FileSystemBase : Base<TM,TN,TW>, IFileSystem
                {
                    public:

                        FileSystemBase( TM^ fs ) : Base( fs ) { }

                        FileSystemBase( TN** const ppfsapi ) : Base( ppfsapi ) {}

                        FileSystemBase( TN* const pfsapi ) : Base( pfsapi ) {}

                    public:

                        virtual void DiskSpace(
                            String^ path,
                            [Out] Int64% bytesFreeForUser,
                            [Out] Int64% bytesTotalForUser,
                            [Out] Int64% bytesFreeOnDisk );

                        virtual int FileSectorSize( String^ path );

                        virtual int FileAtomicWriteSize( String^ path );

                        virtual EsentErrorException^ GetLastError( int error );

                        virtual String^ PathRoot( String^ path );

                        virtual void PathVolumeCanonicalAndDiskId( 
                            String^ volumePath,
                            [Out] String^% volumeCanonicalPath,
                            [Out] String^% diskId,
                            [Out] int% diskNumber );

                        virtual  String^ PathComplete( String^ path );

                        virtual void PathParse( String^ path, [Out] String^% folder, [Out] String^% fileBase, [Out] String^% fileExt );

                        virtual String^ PathFileName( String^ path );

                        virtual String^ PathBuild( String^ folder, String^ fileBase, String^ fileExt );

                        virtual String^ PathFolderNorm( String^ folder );

                        virtual bool PathIsRelative( String^ path );

                        virtual bool PathExists( String^ path );

                        virtual String^ PathFolderDefault( [Out] bool% canProcessUseRelativePaths );

                        virtual String^ GetTempFolder();

                        virtual String^ GetTempFileName( String^ folder, String^ prefix );

                        virtual void FolderCreate( String^ path );

                        virtual void FolderRemove( String^ path );

                        virtual IFileFind^ FileFind( String^ find );

                        virtual void FileDelete( String^ path );

                        virtual void FileMove( String^ pathSource, String^ pathDest, bool overwriteExisting );

                        virtual void FileCopy( String^ pathSource, String^ pathDest, bool overwriteExisting );

                        virtual IFile^ FileCreate( String^ path, FileModeFlags fileModeFlags );

                        virtual IFile^ FileOpen( String^ path, FileModeFlags fileModeFlags );

                    protected:

                        virtual IFile^ Wrap( IFileAPI** ppfapi );
                };

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::DiskSpace(
                    String^ path,
                    [Out] Int64% bytesFreeForUser,
                    [Out] Int64% bytesTotalForUser,
                    [Out] Int64% bytesFreeOnDisk )
                {
                    ERR     err             = JET_errSuccess;
                    QWORD   cbFreeForUser   = 0;
                    QWORD   cbTotalForUser  = 0;
                    QWORD   cbFreeOnDisk    = 0;

                    bytesFreeForUser = 0;
                    bytesTotalForUser = 0;
                    bytesFreeOnDisk = 0;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrDiskSpace( (const WCHAR*)wszPath,
                                            &cbFreeForUser,
                                            &cbTotalForUser,
                                            &cbFreeOnDisk ) );

                    bytesFreeForUser = cbFreeForUser;
                    bytesTotalForUser = cbTotalForUser;
                    bytesFreeOnDisk = cbFreeOnDisk;

                    return;

                HandleError:
                    bytesFreeForUser = 0;
                    bytesTotalForUser = 0;
                    bytesFreeOnDisk = 0;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline int FileSystemBase<TM,TN,TW>::FileSectorSize( String^ path )
                {
                    ERR     err     = JET_errSuccess;
                    DWORD   cbSize  = 0;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFileSectorSize( (const WCHAR*)wszPath, &cbSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline int FileSystemBase<TM,TN,TW>::FileAtomicWriteSize( String^ path )
                {
                    ERR     err     = JET_errSuccess;
                    DWORD   cbSize  = 0;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFileAtomicWriteSize( (const WCHAR*)wszPath, &cbSize ) );

                    return cbSize;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline EsentErrorException^ FileSystemBase<TM,TN,TW>::GetLastError( int error )
                {
                    return EseException( Pi->ErrGetLastError( error ) );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::PathRoot( String^ path )
                {
                    ERR     err                             = JET_errSuccess;
                    WCHAR   wszRootPath[ OSFSAPI_MAX_PATH ] = { 0 };

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrPathRoot( (const WCHAR*)wszPath, wszRootPath ) );

                    return gcnew String( wszRootPath );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::PathVolumeCanonicalAndDiskId(
                    String^ volumePath,
                    [Out] String^% volumeCanonicalPath,
                    [Out] String^% diskId,
                    [Out] int% diskNumber )
                {
                    WCHAR   wszVolumeCanonicalPath[ OSFSAPI_MAX_PATH ]  = { 0 };
                    WCHAR   wszDiskId[ OSFSAPI_MAX_PATH ]               = { 0 };
                    DWORD   dwDiskNumber                                = 0;

                    pin_ptr<const Char> wszVolumePath = PtrToStringChars( volumePath );
                    Pi->PathVolumeCanonicalAndDiskId(   (const WCHAR*)wszVolumePath,
                                                        wszVolumeCanonicalPath,
                                                        _countof( wszVolumeCanonicalPath ),
                                                        wszDiskId,
                                                        _countof( wszDiskId ),
                                                        &dwDiskNumber );

                    volumeCanonicalPath = gcnew String( wszVolumeCanonicalPath );
                    diskId = gcnew String( wszDiskId );
                    diskNumber = dwDiskNumber;
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::PathComplete( String^ path )
                {
                    ERR     err                             = JET_errSuccess;
                    WCHAR   wszFullPath[ OSFSAPI_MAX_PATH ] = { 0 };

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrPathComplete( (const WCHAR*)wszPath, wszFullPath ) );

                    return gcnew String( wszFullPath );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::PathParse(
                    String^ path,
                    [Out] String^% folder,
                    [Out] String^% fileBase,
                    [Out] String^% fileExt )
                {
                    ERR     err                             = JET_errSuccess;
                    WCHAR   wszFolder[ OSFSAPI_MAX_PATH ]   = { 0 };
                    WCHAR   wszFileBase[ OSFSAPI_MAX_PATH ] = { 0 };
                    WCHAR   wszFileExt[ OSFSAPI_MAX_PATH ]  = { 0 };

                    folder = nullptr;
                    fileBase = nullptr;
                    fileExt = nullptr;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrPathParse( (const WCHAR*)wszPath,
                                            wszFolder,
                                            wszFileBase,
                                            wszFileExt ) );

                    folder = gcnew String( wszFolder );
                    fileBase = gcnew String( wszFileBase );
                    fileExt = gcnew String( wszFileExt );

                    return;

                HandleError:
                    folder = nullptr;
                    fileBase = nullptr;
                    fileExt = nullptr;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::PathFileName( String^ path )
                {
                    ERR     err                                 = JET_errSuccess;
                    WCHAR   wszPathStorage[ OSFSAPI_MAX_PATH ]  = { 0 };
                    WCHAR*  wszPath                             = NULL;

                    if ( path )
                    {
                        pin_ptr<const Char> wszPathT = PtrToStringChars( path );
                        Call( ErrOSStrCbCopyW( wszPathStorage, _cbrg( wszPathStorage ), (STRSAFE_LPCWSTR)wszPathT ) );
                        wszPath = wszPathStorage;
                    }

                    return gcnew String( Pi->WszPathFileName( wszPath ) );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::PathBuild( String^ folder, String^ fileBase, String^ fileExt )
                {
                    ERR     err                         = JET_errSuccess;
                    WCHAR   wszPath[ OSFSAPI_MAX_PATH ] = { 0 };

                    pin_ptr<const Char> wszFolder = PtrToStringChars( folder );
                    pin_ptr<const Char> wszFileBase = PtrToStringChars( fileBase );
                    pin_ptr<const Char> wszFileExt = PtrToStringChars( fileExt );
                    Call( Pi->ErrPathBuild( (const WCHAR*)wszFolder,
                                            (const WCHAR*)wszFileBase,
                                            (const WCHAR*)wszFileExt,
                                            wszPath,
                                            _cbrg( wszPath ) ) );

                    return gcnew String( wszPath );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::PathFolderNorm( String^ folder )
                {
                    ERR     err                             = JET_errSuccess;
                    WCHAR   wszFolder[ OSFSAPI_MAX_PATH ]   = { 0 };

                    pin_ptr<const Char> wszFolderT = PtrToStringChars( folder );
                    Call( ErrOSStrCbCopyW( wszFolder, _cbrg( wszFolder ), (STRSAFE_LPCWSTR)wszFolderT ) );

                    Call( Pi->ErrPathFolderNorm( wszFolder, _cbrg( wszFolder ) ) );

                    return gcnew String( wszFolder );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline bool FileSystemBase<TM,TN,TW>::PathIsRelative( String^ path )
                {
                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    return Pi->FPathIsRelative( (const WCHAR*)wszPath ) ? true : false;
                }

                template< class TM, class TN, class TW >
                inline bool FileSystemBase<TM,TN,TW>::PathExists( String^ path )
                {
                    ERR     err         = JET_errSuccess;
                    BOOL    fDirectory  = fFalse;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrPathExists( (const WCHAR*)wszPath, &fDirectory ) );

                    return fDirectory ? true : false;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::PathFolderDefault( [Out] bool% canProcessUseRelativePaths )
                {
                    ERR         err                             = JET_errSuccess;
                    WCHAR       wszFolder[ OSFSAPI_MAX_PATH ]   = { 0 };
                    BOOL        fCanProcessUseRelativePaths     = fFalse;

                    canProcessUseRelativePaths = false;

                    Call( Pi->ErrPathFolderDefault( wszFolder, _cbrg( wszFolder ), &fCanProcessUseRelativePaths ) );

                    canProcessUseRelativePaths = fCanProcessUseRelativePaths ? true : false;
                    return gcnew String( wszFolder );

                HandleError:
                    canProcessUseRelativePaths = false;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::GetTempFolder()
                {
                    ERR         err                             = JET_errSuccess;
                    WCHAR       wszFolder[ OSFSAPI_MAX_PATH ]   = { 0 };

                    Call( Pi->ErrGetTempFolder( wszFolder, OSFSAPI_MAX_PATH ) );

                    return gcnew String( wszFolder );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline String^ FileSystemBase<TM,TN,TW>::GetTempFileName( String^ folder, String^ prefix )
                {
                    ERR         err                             = JET_errSuccess;
                    WCHAR       wszFileName[ OSFSAPI_MAX_PATH ] = { 0 };

                    pin_ptr<const Char> wszFolder = PtrToStringChars( folder );
                    pin_ptr<const Char> wszPrefix = PtrToStringChars( prefix );
                    Call( Pi->ErrGetTempFileName( (const PWSTR)wszFolder, (const PWSTR)wszPrefix, wszFileName ) );

                    return gcnew String( wszFileName );

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::FolderCreate( String^ path )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFolderCreate( (const WCHAR*)wszPath ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::FolderRemove( String^ path )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFolderRemove( (const WCHAR*)wszPath ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline IFileFind^ FileSystemBase<TM,TN,TW>::FileFind( String^ find )
                {
                    ERR             err     = JET_errSuccess;
                    IFileFindAPI*   pffapi  = NULL;

                    pin_ptr<const Char> wszFind = PtrToStringChars( find );
                    Call( Pi->ErrFileFind( (const WCHAR*)wszFind, &pffapi ) );

                    return pffapi ? gcnew Interop::FileFind( &pffapi ) : nullptr;

                HandleError:
                    delete pffapi;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::FileDelete( String^ path )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFileDelete( (const WCHAR*)wszPath ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::FileMove( String^ pathSource, String^ pathDest, bool overwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPathSource = PtrToStringChars( pathSource );
                    pin_ptr<const Char> wszPathDest = PtrToStringChars( pathDest );
                    Call( Pi->ErrFileMove(  (const WCHAR*)wszPathSource,
                                            (const WCHAR*)wszPathDest,
                                            overwriteExisting ? fTrue : fFalse ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline void FileSystemBase<TM,TN,TW>::FileCopy( String^ pathSource, String^ pathDest, bool overwriteExisting )
                {
                    ERR err = JET_errSuccess;

                    pin_ptr<const Char> wszPathSource = PtrToStringChars( pathSource );
                    pin_ptr<const Char> wszPathDest = PtrToStringChars( pathDest );
                    Call( Pi->ErrFileCopy(  (const WCHAR*)wszPathSource,
                                            (const WCHAR*)wszPathDest,
                                            overwriteExisting ? fTrue : fFalse ) );

                    return;

                HandleError:
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline IFile^ FileSystemBase<TM,TN,TW>::FileCreate( String^ path, FileModeFlags fileModeFlags )
                {
                    ERR         err     = JET_errSuccess;
                    IFileAPI*   pfapi   = NULL;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFileCreate(    (const WCHAR*)wszPath,
                                                (IFileAPI::FileModeFlags)fileModeFlags,
                                                &pfapi ) );

                    return Wrap( &pfapi );

                HandleError:
                    delete pfapi;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline IFile^ FileSystemBase<TM,TN,TW>::FileOpen( String^ path, FileModeFlags fileModeFlags )
                {
                    ERR         err     = JET_errSuccess;
                    IFileAPI*   pfapi   = NULL;

                    pin_ptr<const Char> wszPath = PtrToStringChars( path );
                    Call( Pi->ErrFileOpen(  (const WCHAR*)wszPath,
                                            (IFileAPI::FileModeFlags)fileModeFlags,
                                            &pfapi ) );

                    return Wrap( &pfapi );

                HandleError:
                    delete pfapi;
                    throw EseException( err );
                }

                template< class TM, class TN, class TW >
                inline IFile^ FileSystemBase<TM, TN, TW>::Wrap( IFileAPI** ppfapi )
                {
                    return *ppfapi ? gcnew File( ppfapi ) : nullptr;
                }
            }
        }
    }
}
