// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSFSAPI_HXX_INCLUDED
#define _OSFSAPI_HXX_INCLUDED

#include "osdiskapi.hxx"
#include "osfileapi.hxx"
#include "osfsconfig.hxx"

//  File System API Notes

//  This abstraction exists to allow multiple file systems to be addressed in a
//  compatible manner
//
//  All paths may be absolute or relative unless otherwise specified.  All paths
//  are case insensitive and can be segmented by '\\' or '/' or both.  All
//  other characters take on their usual meaning for the implemented OS.  This
//  is intended to allow OS dependent paths to be passed down from the topmost
//  levels to these functions and still function properly
//
//  The maximum path length under any circumstance is specified by cchPathMax.
//  Buffers of at least this size must be given for all out parameters that will
//  contain paths and paths that are in parameters must not exceed this size
//
//  All file I/O is performed in blocks and is unbuffered and write-through
//
//  All file sharing is enforced as if the file were protected by a Reader /
//  Writer lock.  A file can be opened with multiple handles for Read Only
//  access but a file can only be opened with one handle for Read / Write
//  access.  A file can never be opened for both Read Only and Read / Write
//  access

//  File Find API

class IFileFindAPI  //  ffapi
{
    public:

        //  dtor

        virtual ~IFileFindAPI() {}


        //  Iterator

        //  moves to the next file or folder in the set of files or folders
        //  found by this File Find iterator.  if there are no more files or
        //  folders to be found, JET_errFileNotFound is returned

        virtual ERR ErrNext() = 0;


        //  Property Access

        //  returns the folder attribute of the current file or folder.  if
        //  there is no current file or folder, JET_errFileNotFound is returned

        virtual ERR ErrIsFolder( BOOL* const pfFolder ) = 0;

        //  retrieves the absolute path of the current file or folder.  if
        //  there is no current file or folder, JET_errFileNotFound is returned

        virtual ERR ErrPath( // UNDONE_BANAPI:
                             __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const wszAbsFoundPath ) = 0;

        //  returns the size of the current file or folder.  if there is no
        //  current file or folder, JET_errFileNotFound is returned

        virtual ERR ErrSize(
            _Out_ QWORD* const pcbSize,
            _In_ const IFileAPI::FILESIZE file ) = 0;

        //  returns the read only attribute of the current file or folder.  if
        //  there is no current file or folder, JET_errFileNotFound is returned

        virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly ) = 0;
};


//  File System API

class IFileSystemAPI  //  fsapi
{
    public:

        //  ctor

        IFileSystemAPI() {}

        //  dtor

        virtual ~IFileSystemAPI() {}


        //  Properties

        //  maximum path size

        enum { cchPathMax = OSFSAPI_MAX_PATH };
        //static const int cchPathMax = OSFSAPI_MAX_PATH;


        //  returns the amount of free/total disk space on the drive hosting the specified path

        virtual ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                                    QWORD* const        pcbFreeForUser,
                                    QWORD* const        pcbTotalForUser = NULL,
                                    QWORD* const        pcbFreeOnDisk = NULL ) = 0;

        //  returns the sector size for the specified path

        virtual ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                        DWORD* const        pcbSize ) = 0;

        //  returns the atomic write size for the specified path

        virtual ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize ) = 0;

        //  Error Conversion
        
        virtual ERR ErrGetLastError( const DWORD error ) = 0;

        //  Path Manipulation

        //  computes the root path of the specified path
        
        virtual ERR ErrPathRoot(    const WCHAR* const                                          wszPath,
                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszAbsRootPath ) = 0;

        //  computes the volume canonical path and disk ID of the specified root path

        virtual void PathVolumeCanonicalAndDiskId(  const WCHAR* const                                  wszVolumePath,
                                                    __out_ecount(cchVolumeCanonicalPath) WCHAR* const   wszVolumeCanonicalPath,
                                                    __in const DWORD                                    cchVolumeCanonicalPath,
                                                    __out_ecount(cchDiskId) WCHAR* const                wszDiskId,
                                                    __in const DWORD                                    cchDiskId,
                                                    __out DWORD *                                       pdwDiskNumber ) = 0;

        //  computes the absolute path of the specified path, returning
        //  JET_errInvalidPath if the path is not found.  the absolute path is
        //  returned in the specified buffer if provided

        virtual ERR ErrPathComplete(     _In_z_ const WCHAR* const  wszPath,
                                        // UNDONE_BANAPI:
                                        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const       wszAbsPath  = NULL ) = 0;

        //  breaks the given path into folder, filename, and extension
        //  components

        virtual ERR ErrPathParse(   const WCHAR* const  wszPath,
                                        // UNDONE_BANAPI:
                                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFolder,
                                        // UNDONE_BANAPI:
                                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFileBase,
                                        // UNDONE_BANAPI:
                                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFileExt ) = 0;

        //  returns the portion of the passed in string representing just the filename.ext piece.

        virtual const WCHAR * const WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const = 0;

        //  composes a path from folder, filename, and extension components

        ERR ErrPathBuild(   __in_z const WCHAR* const                           wszFolder,
                            __in_z const WCHAR* const                           wszFileBase,
                            __in_z const WCHAR* const                           wszFileExt,
                            // UNDONE_BANAPI:
                            __out_bcount_z(cbOSFSAPI_MAX_PATHW) WCHAR* const    wszPath )
        {
            return ErrPathBuild( wszFolder, wszFileBase, wszFileExt, wszPath, cbOSFSAPI_MAX_PATHW );
        }

        virtual ERR ErrPathBuild(   __in_z const WCHAR* const   wszFolder,
                                    __in_z const WCHAR* const   wszFileBase,
                                    __in_z const WCHAR* const   wszFileExt,
                                    // UNDONE_BANAPI:
                                    __out_bcount_z(cbPath) WCHAR* const wszPath,
                                    __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG cbPath ) = 0;


        //  appends a folder delimiter (in NT L"\") if it doesn't already have a
        //  folder delimiter at the end of the string.

        virtual ERR ErrPathFolderNorm(  __inout_bcount(cbSize)  PWSTR const wszFolder,
                                                                DWORD           cbSize ) = 0;

        //  Returns whether the input path is absolute or relative.
        //  Note that a leading backslash (e.g. \windows\win.ini) is still relative.

        virtual BOOL FPathIsRelative(_In_ PCWSTR wszPath ) = 0;

        //  Returns whether the specified file/path exists.

        virtual ERR ErrPathExists(   _In_ PCWSTR wszPath,
                                    _Out_opt_ BOOL* pfIsDirectory ) = 0;

        // Retrieves the default location that files should be stored.
        // The "." directory (current working dir) is not writeable by Windows Store apps.
        virtual ERR ErrPathFolderDefault( _Out_z_bytecap_(cbFolder) PWSTR const  wszFolder,
                                          _In_ DWORD            cbFolder,
                                          _Out_ BOOL            *pfCanProcessUseRelativePaths ) = 0;

        //  Folder Control

        //  creates the specified folder.  if the folder already exists,
        //  JET_errFileAccessDenied will be returned

        virtual ERR ErrFolderCreate( const WCHAR* const wszPath ) = 0;

        //  removes the specified folder.  if the folder is not empty,
        //  JET_errFileAccessDenied will be returned.  if the folder does
        //  not exist, JET_errInvalidPath will be returned

        virtual ERR ErrFolderRemove( const WCHAR* const wszPath ) = 0;


        //  Folder Enumeration

        //  creates a File Find iterator that will traverse the absolute paths
        //  of all files and folders that match the specified path with
        //  wildcards.  if the iterator cannot be created, JET_errOutOfMemory
        //  will be returned

        virtual ERR ErrFileFind(    const WCHAR* const      wszFind,
                                    IFileFindAPI** const    ppffapi ) = 0;


        //  File Control

        //  deletes the file specified.  if the file cannot be deleted,
        //  JET_errFileAccessDenied is returned

        virtual ERR ErrFileDelete( const WCHAR* const wszPath ) = 0;

        //  moves the file specified from a source path to a destination path, overwriting
        //  any file at the destination path if requested.  if the file cannot be moved,
        //  JET_errFileAccessDenied is returned

        virtual ERR ErrFileMove(    const WCHAR* const  wszPathSource,
                                    const WCHAR* const  wszPathDest,
                                    const BOOL          fOverwriteExisting  = fFalse ) = 0;

        //  copies the file specified from a source path to a destination path, overwriting
        //  any file at the destination path if requested.  if the file cannot be copied,
        //  JET_errFileAccessDenied is returned

        virtual ERR ErrFileCopy(    const WCHAR* const  wszPathSource,
                                    const WCHAR* const  wszPathDest,
                                    const BOOL          fOverwriteExisting  = fFalse ) = 0;

        //  creates the file specified and returns its handle.  if the file cannot
        //  be created, JET_errFileAccessDenied or JET_errDiskFull will be returned

        virtual ERR ErrFileCreate(  _In_z_ const WCHAR* const               wszPath,
                                    _In_   const IFileAPI::FileModeFlags    fmf,
                                    _Out_  IFileAPI** const                 ppfapi ) = 0;

        //  opens the file specified with the specified access privileges and returns
        //  its handle.  if the file cannot be opened, JET_errFileAccessDenied is returned

        virtual ERR ErrFileOpen(    _In_z_ const WCHAR* const               wszPath,
                                    _In_   const IFileAPI::FileModeFlags    fmf,
                                    _Out_  IFileAPI** const                 ppfapi ) = 0;
};

//  initializes an interface to the OS File System

ERR ErrOSFSCreate( __out IFileSystemAPI** const ppfsapi );

ERR ErrOSFSCreate(  __in IFileSystemConfiguration * const pfsconfig,
                    __out IFileSystemAPI** const ppfsapi );

#endif  //  _OSFSAPI_HXX_INCLUDED

