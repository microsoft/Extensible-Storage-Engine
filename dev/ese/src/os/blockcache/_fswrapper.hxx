// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TFileSystemWrapper:  wrapper of an implementation of IFileSystemAPI or its derivatives.
//
//  This is a utility class used to enable test dependency injection and as the basis for building the file system
//  filter used to implement the block cache.

template< class I >
class TFileSystemWrapper  //  fsw
    :   public I
{
    public:  //  specialized API

        TFileSystemWrapper( _Inout_ I** const ppi );

        virtual ~TFileSystemWrapper();

    public:  //  IFileSystemAPI

        ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                            QWORD* const        pcbFreeForUser,
                            QWORD* const        pcbTotalForUser = NULL,
                            QWORD* const        pcbFreeOnDisk = NULL );


        ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                DWORD* const        pcbSize );

        ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                    DWORD* const        pcbSize );

        ERR ErrGetLastError( const DWORD error );

        ERR ErrPathRoot(    const WCHAR* const                                              wszPath,
                            __out_bcount( OSFSAPI_MAX_PATH * sizeof( WCHAR ) ) WCHAR* const wszAbsRootPath );

        void PathVolumeCanonicalAndDiskId(  const WCHAR* const                                  wszVolumePath,
                                            __out_ecount( cchVolumeCanonicalPath ) WCHAR* const wszVolumeCanonicalPath,
                                            _In_ const DWORD                                    cchVolumeCanonicalPath,
                                            __out_ecount( cchDiskId ) WCHAR* const              wszDiskId,
                                            _In_ const DWORD                                    cchDiskId,
                                            _Out_ DWORD *                                       pdwDiskNumber );

        ERR ErrPathComplete(    _In_z_ const WCHAR* const                           wszPath,
                                _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const   wszAbsPath );
        ERR ErrPathParse(   const WCHAR* const                                          wszPath,
                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFolder,
                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileBase,
                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileExt );
        const WCHAR * const WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const;
        ERR ErrPathBuild(   __in_z const WCHAR* const                                   wszFolder,
                            __in_z const WCHAR* const                                   wszFileBase,
                            __in_z const WCHAR* const                                   wszFileExt,
                            __out_bcount_z(cbPath) WCHAR* const                         wszPath,
                            __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG  cbPath );
        ERR ErrPathFolderNorm(  __inout_bcount(cbSize)  PWSTR const wszFolder,
                                DWORD                               cbSize );
        BOOL FPathIsRelative(_In_ PCWSTR wszPath );

        ERR ErrPathExists(  _In_ PCWSTR     wszPath,
                            _Out_opt_ BOOL* pfIsDirectory );

        ERR ErrPathFolderDefault(   _Out_z_bytecap_(cbSize) PWSTR const wszFolder,
                                    _In_ DWORD                          cbSize,
                                    _Out_ BOOL *                        pfCanProcessUseRelativePaths );


        ERR ErrFolderCreate( const WCHAR* const wszPath );
        ERR ErrFolderRemove( const WCHAR* const wszPath );

        ERR ErrFileFind(    const WCHAR* const      wszFind,
                            IFileFindAPI** const    ppffapi );

        ERR ErrFileDelete( const WCHAR* const wszPath );
        ERR ErrFileMove(    const WCHAR* const  wszPathSource,
                            const WCHAR* const  wszPathDest,
                            const BOOL          fOverwriteExisting  = fFalse );
        ERR ErrFileCopy(    const WCHAR* const  wszPathSource,
                            const WCHAR* const  wszPathDest,
                            const BOOL          fOverwriteExisting  = fFalse );

        ERR ErrFileCreate(  _In_z_ const WCHAR* const               wszPath,
                            _In_   const IFileAPI::FileModeFlags    fmf,
                            _Out_  IFileAPI** const                 ppfapi );

        ERR ErrFileOpen(    _In_z_ const WCHAR* const               wszPath,
                            _In_   const IFileAPI::FileModeFlags    fmf,
                            _Out_  IFileAPI** const                 ppfapi );

    protected:

        virtual ERR ErrWrapFile( _Inout_ IFileAPI** const ppfapiInner, _Out_ IFileAPI** const ppfapi );

    protected:

        I* const m_piInner;
};

template< class I >
TFileSystemWrapper<I>::TFileSystemWrapper( _Inout_ I** const ppi )
    :   m_piInner( *ppi )
{
    *ppi = NULL;
}

template< class I >
TFileSystemWrapper<I>::~TFileSystemWrapper()
{
    delete m_piInner;
}

template< class I >
ERR TFileSystemWrapper<I>::ErrDiskSpace(    const WCHAR * const wszPath,
                                            QWORD * const       pcbFreeForUser,
                                            QWORD * const       pcbTotalForUser,
                                            QWORD * const       pcbFreeOnDisk )

{
    return m_piInner->ErrDiskSpace( wszPath, pcbFreeForUser, pcbTotalForUser, pcbFreeOnDisk );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileSectorSize(   const WCHAR* const  wszPath,
                                                DWORD* const        pcbSize )
{
    return m_piInner->ErrFileSectorSize( wszPath, pcbSize );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileAtomicWriteSize(  const WCHAR* const  wszPath,
                                                    DWORD* const        pcbSize )
{
    return m_piInner->ErrFileAtomicWriteSize( wszPath, pcbSize );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrGetLastError( const DWORD error )
{
    return m_piInner->ErrGetLastError( error );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathRoot( const WCHAR* const                                              wszPath,
                                        __out_bcount( OSFSAPI_MAX_PATH * sizeof( WCHAR ) ) WCHAR* const wszAbsRootPath )
{
    return m_piInner->ErrPathRoot( wszPath, wszAbsRootPath );
}

template< class I >
void TFileSystemWrapper<I>::PathVolumeCanonicalAndDiskId(   const WCHAR* const                                  wszVolumePath,
                                                            __out_ecount( cchVolumeCanonicalPath ) WCHAR* const wszVolumeCanonicalPath,
                                                            _In_ const DWORD                                    cchVolumeCanonicalPath,
                                                            __out_ecount( cchDiskId ) WCHAR* const              wszDiskId,
                                                            _In_ const DWORD                                    cchDiskId,
                                                            _Out_ DWORD *                                       pdwDiskNumber )
{
    m_piInner->PathVolumeCanonicalAndDiskId( wszVolumePath, wszVolumeCanonicalPath, cchVolumeCanonicalPath, wszDiskId, cchDiskId, pdwDiskNumber );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathComplete( _In_z_ const WCHAR* const                           wszPath,
                                            _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const   wszAbsPath )
{
    return m_piInner->ErrPathComplete( wszPath, wszAbsPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathParse(    const WCHAR* const                                          wszPath,
                                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFolder,
                                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileBase,
                                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const   wszFileExt )
{
    return m_piInner->ErrPathParse( wszPath, wszFolder, wszFileBase, wszFileExt );
}

template< class I >
const WCHAR * const TFileSystemWrapper<I>::WszPathFileName( _In_z_ const WCHAR * const wszOptionalFullPath ) const
{
    return m_piInner->WszPathFileName( wszOptionalFullPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathBuild(    __in_z const WCHAR* const                                   wszFolder,
                                            __in_z const WCHAR* const                                   wszFileBase,
                                            __in_z const WCHAR* const                                   wszFileExt,
                                            __out_bcount_z(cbPath) WCHAR* const                         wszPath,
                                            __in_range(cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW) ULONG  cbPath )
{
    return m_piInner->ErrPathBuild( wszFolder, wszFileBase, wszFileExt, wszPath, cbPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathFolderNorm(   __inout_bcount( cbSize ) PWSTR const    wszFolder,
                                                DWORD                                   cbSize )
{
    return m_piInner->ErrPathFolderNorm( wszFolder, cbSize );
}

template< class I >
BOOL TFileSystemWrapper<I>::FPathIsRelative( _In_ PCWSTR wszPath )
{
    return m_piInner->FPathIsRelative( wszPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathExists(   _In_ PCWSTR     wszPath,
                                            _Out_opt_ BOOL* pfIsDirectory )
{
    return m_piInner->ErrPathExists( wszPath, pfIsDirectory );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrPathFolderDefault(    _Out_z_bytecap_(cbFolder) PWSTR const   wszFolder,
                                                    _In_ DWORD                              cbFolder,
                                                    _Out_ BOOL *                            pfCanProcessUseRelativePaths )
{
    return m_piInner->ErrPathFolderDefault( wszFolder, cbFolder, pfCanProcessUseRelativePaths );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFolderCreate( const WCHAR* const wszPath )
{
    return m_piInner->ErrFolderCreate( wszPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFolderRemove( const WCHAR* const wszPath )
{
    return m_piInner->ErrFolderRemove( wszPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileFind( const WCHAR* const      wszFind,
                                        IFileFindAPI** const    ppffapi )
{
    return m_piInner->ErrFileFind( wszFind, ppffapi );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileDelete( const WCHAR* const wszPath )
{
    return m_piInner->ErrFileDelete( wszPath );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileMove( const WCHAR* const  wszPathSource,
                                        const WCHAR* const  wszPathDest,
                                        const BOOL          fOverwriteExisting )
{
    return m_piInner->ErrFileMove( wszPathSource, wszPathDest, fOverwriteExisting );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileCopy( const WCHAR* const  wszPathSource,
                                        const WCHAR* const  wszPathDest,
                                        const BOOL          fOverwriteExisting )
{
    return m_piInner->ErrFileCopy( wszPathSource, wszPathDest, fOverwriteExisting );
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileCreate(   _In_z_  const WCHAR* const              wszPath,
                                            _In_    const IFileAPI::FileModeFlags   fmf,
                                            _Out_   IFileAPI** const                ppfapi )
{
    ERR         err     = JET_errSuccess;
    IFileAPI*   pfapi   = NULL;

    *ppfapi = NULL;

    Call( m_piInner->ErrFileCreate( wszPath, fmf, &pfapi ) );

    Call( ErrWrapFile( &pfapi, ppfapi ) );

HandleError:
    delete pfapi;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemWrapper<I>::ErrFileOpen( _In_z_  const WCHAR* const              wszPath,
                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                        _Out_   IFileAPI** const                ppfapi )
{
    ERR         err = JET_errSuccess;
    IFileAPI*   pfapi = NULL;

    *ppfapi = NULL;

    Call( m_piInner->ErrFileOpen( wszPath, fmf, &pfapi ) );

    Call( ErrWrapFile( &pfapi, ppfapi ) );

HandleError:
    delete pfapi;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemWrapper<I>::ErrWrapFile( _Inout_ IFileAPI** const ppfapiInner, _Out_ IFileAPI** const ppfapi )
{
    ERR         err     = JET_errSuccess;
    IFileAPI*   pfapi   = NULL;

    *ppfapi = NULL;

    if ( *ppfapiInner )
    {
        Alloc( pfapi = new CFileWrapper( ppfapiInner ) );
    }

    *ppfapi = pfapi;
    pfapi = NULL;

HandleError:
    delete pfapi;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

//  CFileSystemWrapper:  concrete TFileSystemWrapper<IFileSystemAPI>.

class CFileSystemWrapper : public TFileSystemWrapper<IFileSystemAPI>
{
    public:  //  specialized API

        CFileSystemWrapper( _Inout_ IFileSystemAPI** const ppfsapi )
            :   TFileSystemWrapper<IFileSystemAPI>( ppfsapi )
        {
        }

        virtual ~CFileSystemWrapper() {}
};
