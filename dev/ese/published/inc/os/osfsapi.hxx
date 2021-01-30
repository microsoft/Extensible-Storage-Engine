// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OSFSAPI_HXX_INCLUDED
#define _OSFSAPI_HXX_INCLUDED

#include "osdiskapi.hxx"
#include "osfileapi.hxx"
#include "osfsconfig.hxx"




class IFileFindAPI
{
    public:


        virtual ~IFileFindAPI() {}




        virtual ERR ErrNext() = 0;




        virtual ERR ErrIsFolder( BOOL* const pfFolder ) = 0;


        virtual ERR ErrPath(
                             __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const wszAbsFoundPath ) = 0;


        virtual ERR ErrSize(
            _Out_ QWORD* const pcbSize,
            _In_ const IFileAPI::FILESIZE file ) = 0;


        virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly ) = 0;
};



class IFileSystemAPI
{
    public:


        IFileSystemAPI() {}


        virtual ~IFileSystemAPI() {}




        enum { cchPathMax = OSFSAPI_MAX_PATH };



        virtual ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                                    QWORD* const        pcbFreeForUser,
                                    QWORD* const        pcbTotalForUser = NULL,
                                    QWORD* const        pcbFreeOnDisk = NULL ) = 0;


        virtual ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                        DWORD* const        pcbSize ) = 0;


        virtual ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize ) = 0;

        
        virtual ERR ErrGetLastError( const DWORD error ) = 0;


        
        virtual ERR ErrPathRoot(    const WCHAR* const                                          wszPath,
                                    __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszAbsRootPath ) = 0;


        virtual void PathVolumeCanonicalAndDiskId(  const WCHAR* const                                  wszVolumePath,
                                                    __out_ecount(cchVolumeCanonicalPath) WCHAR* const   wszVolumeCanonicalPath,
                                                    __in const DWORD                                    cchVolumeCanonicalPath,
                                                    __out_ecount(cchDiskId) WCHAR* const                wszDiskId,
                                                    __in const DWORD                                    cchDiskId,
                                                    __out DWORD *                                       pdwDiskNumber ) = 0;


        virtual ERR ErrPathComplete(     _In_z_ const WCHAR* const  wszPath,
                                        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const       wszAbsPath  = NULL ) = 0;


        virtual ERR ErrPathParse(   const WCHAR* const  wszPath,
                                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFolder,
                                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFileBase,
                                        __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszFileExt ) = 0;


        virtual const WCHAR * const WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const = 0;


        ERR ErrPathBuild(   __in_z const WCHAR* const                           wszFolder,
                            __in_z const WCHAR* const                           wszFileBase,
                            __in_z const WCHAR* const                           wszFileExt,
                            __out_bcount_z(cbOSFSAPI_MAX_PATHW) WCHAR* const    wszPath )
        {
            return ErrPathBuild( wszFolder, wszFileBase, wszFileExt, wszPath, cbOSFSAPI_MAX_PATHW );
        }

        virtual ERR ErrPathBuild(   __in_z const WCHAR* const   wszFolder,
                                    __in_z const WCHAR* const   wszFileBase,
                                    __in_z const WCHAR* const   wszFileExt,
                                    __out_bcount_z(cbPath) WCHAR* const wszPath,
                                    __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG cbPath ) = 0;



        virtual ERR ErrPathFolderNorm(  __inout_bcount(cbSize)  PWSTR const wszFolder,
                                                                DWORD           cbSize ) = 0;


        virtual BOOL FPathIsRelative(_In_ PCWSTR wszPath ) = 0;


        virtual ERR ErrPathExists(   _In_ PCWSTR wszPath,
                                    _Out_opt_ BOOL* pfIsDirectory ) = 0;

        virtual ERR ErrPathFolderDefault( _Out_z_bytecap_(cbFolder) PWSTR const  wszFolder,
                                          _In_ DWORD            cbFolder,
                                          _Out_ BOOL            *pfCanProcessUseRelativePaths ) = 0;



        virtual ERR ErrFolderCreate( const WCHAR* const wszPath ) = 0;


        virtual ERR ErrFolderRemove( const WCHAR* const wszPath ) = 0;




        virtual ERR ErrFileFind(    const WCHAR* const      wszFind,
                                    IFileFindAPI** const    ppffapi ) = 0;




        virtual ERR ErrFileDelete( const WCHAR* const wszPath ) = 0;


        virtual ERR ErrFileMove(    const WCHAR* const  wszPathSource,
                                    const WCHAR* const  wszPathDest,
                                    const BOOL          fOverwriteExisting  = fFalse ) = 0;


        virtual ERR ErrFileCopy(    const WCHAR* const  wszPathSource,
                                    const WCHAR* const  wszPathDest,
                                    const BOOL          fOverwriteExisting  = fFalse ) = 0;


        virtual ERR ErrFileCreate(  _In_z_ const WCHAR* const               wszPath,
                                    _In_   const IFileAPI::FileModeFlags    fmf,
                                    _Out_  IFileAPI** const                 ppfapi ) = 0;


        virtual ERR ErrFileOpen(    _In_z_ const WCHAR* const               wszPath,
                                    _In_   const IFileAPI::FileModeFlags    fmf,
                                    _Out_  IFileAPI** const                 ppfapi ) = 0;
};


ERR ErrOSFSCreate( __out IFileSystemAPI** const ppfsapi );

ERR ErrOSFSCreate(  __in IFileSystemConfiguration * const pfsconfig,
                    __out IFileSystemAPI** const ppfsapi );

#endif

