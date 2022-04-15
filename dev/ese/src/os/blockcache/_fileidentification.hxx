// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TFileIdentification:  core implementation of IFileIdentification and its derivatives.
//
//  This class provides the capability for mapping file paths to physical identifiers and back.  It also provides a
//  stable path that can be used to uniquely identify a file.

template< class I >
class TFileIdentification  //  fident
    :   public I
{
    public:

        TFileIdentification();

        virtual ~TFileIdentification();

        void Cleanup() { PurgeVolumeHandleCache(); }

    public:  //  IFileIdentification

        ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                            _Out_   VolumeId* const     pvolumeid,
                            _Out_   FileId* const       pfileid ) override;

        ERR ErrGetFileKeyPath(  _In_z_                                                  const WCHAR* const  wszPath,
                                _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const        wszKeyPath ) override;

        ERR ErrGetFilePathById( _In_                                                    const VolumeId  volumeid,
                                _In_                                                    const FileId    fileid,
                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )                  WCHAR* const    wszAnyAbsPath,
                                _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const    wszKeyPath ) override;

    private:

        //  A volume handle cache entry.

        class CVolumeHandleCacheEntry
        {
            public:

                CVolumeHandleCacheEntry( const VolumeId volumeid, const WCHAR* const wszPath, const HANDLE handle )
                    :   m_volumeid( volumeid ),
                        m_wszPath( wszPath ),
                        m_handle( handle )
                {
                }

                ~CVolumeHandleCacheEntry()
                {
                    if ( m_handle && m_handle != INVALID_HANDLE_VALUE )
                    {
                        CloseHandle( m_handle );
                    }
                    delete[] m_wszPath;
                }

                VolumeId Volumeid() { return m_volumeid; }
                const WCHAR* WszPath() { return m_wszPath; }
                HANDLE Handle() { return m_handle; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CVolumeHandleCacheEntry, m_ile ); }

            private:

                typename CInvasiveList< CVolumeHandleCacheEntry, OffsetOfILE >::CElement    m_ile;
                const VolumeId                                                              m_volumeid;
                const WCHAR* const                                                          m_wszPath;
                const HANDLE                                                                m_handle;
        };

    private:

        ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                            _Out_   HANDLE* const       phFile,
                            _Out_   VolumeId* const     pvolumeid,
                            _Out_   FileId* const       pfileid );

        ERR ErrMakeKeyPath( _In_z_                                                  WCHAR* const    wszAnyAbsPath,
                            _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const    wszKeyPath );

        ERR ErrOpenVolumeById(  _In_    const VolumeId              volumeid,
                                _Out_   CVolumeHandleCacheEntry**   pvhce );

        ERR ErrOpenVolumeByIdCacheMiss( _In_                const VolumeId      volumeid,
                                        _Outptr_result_z_   const WCHAR** const pwszPath,
                                        _Out_               HANDLE* const       phVolume );

        ERR ErrGetLastError();

        void PurgeVolumeHandleCache();
        CVolumeHandleCacheEntry* PvhceFindVolumeByID( _In_ const VolumeId volumeid );
        ERR ErrAddVolumeByID(   _In_    const VolumeId                  volumeid,
                                _In_z_  const WCHAR* const              wszPath,
                                _In_    const HANDLE                    hVolume,
                                _Out_   CVolumeHandleCacheEntry** const ppvhce );

    private:

        CReaderWriterLock                                                               m_rwlVolumeHandleCache;
        CInvasiveList<CVolumeHandleCacheEntry, CVolumeHandleCacheEntry::OffsetOfILE>    m_ilVolumeHandleCache;
};

template< class I >
TFileIdentification<I>::TFileIdentification()
    :   m_rwlVolumeHandleCache( CLockBasicInfo( CSyncBasicInfo( "TFileIdentification<I>::m_rwlVolumeHandleCache" ), rankFileIdentification, 0 ) )
{
}

template< class I >
TFileIdentification<I>::~TFileIdentification()
{
    Cleanup();
}

template< class I >
ERR TFileIdentification<I>::ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                                            _Out_   VolumeId* const     pvolumeid,
                                            _Out_   FileId* const       pfileid )
{
    ERR     err     = JET_errSuccess;
    HANDLE  hFile   = NULL;

    *pvolumeid = volumeidInvalid;
    *pfileid = fileidInvalid;

    //  get the file id by path

    Call( ErrGetFileId( wszPath, &hFile, pvolumeid, pfileid ) );

HandleError:
    if ( hFile && hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    if ( err < JET_errSuccess )
    {
        *pvolumeid = volumeidInvalid;
        *pfileid = fileidInvalid;
    }
    return err;
}

template< class I >
ERR TFileIdentification<I>::ErrGetFileKeyPath(  _In_z_                                                  const WCHAR* const  wszPath,
                                                _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const        wszKeyPath )
{
    ERR                         err                                         = JET_errSuccess;
    const DWORD                 cwchContainerPathMax                        = IFileSystemAPI::cchPathMax;
    WCHAR                       wszContainerPath[ cwchContainerPathMax ]    = { 0 };
    DWORD                       cwchContainerPath                           = 0;
    WCHAR*                      wszFileName                                 = NULL;
    WCHAR*                      wszTarget                                   = NULL;
    WCHAR                       wchTargetSave                               = 0;
    HANDLE                      hContainer                                  = NULL;
    VolumeId                    volumeid                                    = volumeidInvalid;
    FileId                      fileid                                      = fileidInvalid;
    CVolumeHandleCacheEntry*    pvhce                                       = NULL;
    const DWORD                 cwchFinalPathMax                            = IFileSystemAPI::cchPathMax;
    WCHAR                       wszFinalPath[ cwchFinalPathMax ]            = { 0 };
    DWORD                       cwchFinalPath                               = 0;
    const DWORD                 cwchVolumePathMax                           = IFileIdentification::cwchKeyPathMax;
    const DWORD                 cbVolumePathMax                             = cwchVolumePathMax * sizeof( WCHAR );
    WCHAR                       wszVolumePath[ cwchVolumePathMax ]          = { 0 };

    wszKeyPath[ 0 ] = 0;

    //  compute the absolute path and split it into container and target, if present

    cwchContainerPath = GetFullPathNameW( wszPath, cwchContainerPathMax, wszContainerPath, &wszFileName );
    if ( cwchContainerPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchContainerPath >= cwchContainerPathMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    if ( wszFileName )
    {
        wszTarget = wszFileName;
        wchTargetSave = wszTarget[0];
        wszTarget[0] = 0;
    }
    else if ( GetFileAttributesW( wszContainerPath ) == INVALID_FILE_ATTRIBUTES )
    {
        for (   wszTarget = wszContainerPath + cwchContainerPath - 1;
                &wszTarget[ -1 ] >= wszContainerPath && wszTarget[ -1 ] != L'\\';
                wszTarget-- )
        {
        }

        wchTargetSave = wszTarget[0];
        wszTarget[0] = 0;
    }

    //  if this is a raw device path then use it as the key path

    if ( wcscmp( wszContainerPath, L"\\\\.\\" ) == 0 )
    {
        wszTarget[0] = wchTargetSave;
        Call( ErrMakeKeyPath( wszContainerPath, wszKeyPath ) );
        Error( JET_errSuccess );
    }

    //  get the volume id of the container which must exist.  the target if any doesn't need to exist

    err = ErrGetFileId( wszContainerPath, &hContainer, &volumeid, &fileid );
    err = err == JET_errFileNotFound ? ErrERRCheck( JET_errInvalidPath ) : err;
    Call( err );

    //  get the volume handle for the volume id

    Call( ErrOpenVolumeById( volumeid, &pvhce ) );

    //  generate an absolute path to the target using the volume guid root path to remove ambiguity
    //  created by possible multiple paths to the same volume

    cwchFinalPath = GetFinalPathNameByHandleW( hContainer, wszFinalPath, cwchFinalPathMax, VOLUME_NAME_NONE );
    if ( cwchFinalPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchFinalPath >= cwchFinalPathMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrOSStrCbCopyW( wszVolumePath, cbVolumePathMax, pvhce->WszPath() ) );
    Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, wszFinalPath ) );
    Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, L"\\" ) );
    if ( wszTarget )
    {
        wszTarget[0] = wchTargetSave;
        Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, wszTarget ) );
    }

    //  generate an unambiguous file key path

    Call( ErrMakeKeyPath( wszVolumePath, wszKeyPath ) );

HandleError:
    if ( hContainer && hContainer != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hContainer );
    }
    if ( err < JET_errSuccess )
    {
        wszKeyPath[ 0 ] = 0;
    }
    return err;
}
        
template< class I >
ERR TFileIdentification<I>::ErrGetFilePathById( _In_                                                    const VolumeId  volumeid,
                                                _In_                                                    const FileId    fileid,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )                  WCHAR* const    wszAnyAbsPath,
                                                _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const    wszKeyPath )
{
    ERR                         err                                 = JET_errSuccess;
    CVolumeHandleCacheEntry*    pvhce                               = NULL;
    FILE_ID_DESCRIPTOR          fileIdDescriptor                    = { 0 };
    HANDLE                      hFile                               = NULL;
    const DWORD                 cwchAnyAbsPathMax                   = IFileSystemAPI::cchPathMax;
    const DWORD                 cbAnyAbsPathMax                     = cwchAnyAbsPathMax * sizeof( WCHAR );
    const WCHAR                 wszPrefix[]                         = L"\\\\?\\";
    const DWORD                 cwchPrefix                          = _countof( wszPrefix ) - 1;
    const DWORD                 cwchFinalPathMax                    = cwchPrefix + IFileSystemAPI::cchPathMax;
    WCHAR                       wszFinalPath[ cwchFinalPathMax ]    = { 0 };
    DWORD                       cwchFinalPath                       = 0;
    const DWORD                 cwchVolumePathMax                   = IFileIdentification::cwchKeyPathMax;
    const DWORD                 cbVolumePathMax                     = cwchVolumePathMax * sizeof( WCHAR );
    WCHAR                       wszVolumePath[ cwchVolumePathMax ]  = { 0 };

    wszAnyAbsPath[ 0 ] = 0;
    wszKeyPath[ 0 ] = 0;

    //  defend against illegal values for the volumeid and fileid

    if ( volumeid == volumeidInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( fileid == fileidInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  get the volume handle for the volume id

    err = ErrOpenVolumeById( volumeid, &pvhce );
    Call( err == JET_errInvalidPath ? ErrERRCheck( JET_errFileNotFound ) : err );

    //  open the file by id

    fileIdDescriptor.dwSize = sizeof( fileIdDescriptor );
    fileIdDescriptor.Type = ObjectIdType;
    *((FileId*)fileIdDescriptor.ExtendedFileId.Identifier) = fileid;

    hFile = OpenFileById(   pvhce->Handle(),
                            &fileIdDescriptor,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS );

    if ( !hFile || hFile == INVALID_HANDLE_VALUE )
    {
        if ( GetLastError() == ERROR_INVALID_PARAMETER )
        {
            Error( ErrERRCheck( JET_errFileNotFound ) );
        }

        Call( ErrGetLastError() );
    }

    //  compute an absolute path to the file.  there may be more than one.  also remove the "\\?\" prefix

    cwchFinalPath = GetFinalPathNameByHandleW( hFile, wszFinalPath, cwchFinalPathMax, VOLUME_NAME_DOS );
    if ( cwchFinalPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchFinalPath >= cwchFinalPathMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrOSStrCbCopyW( wszAnyAbsPath, cbAnyAbsPathMax, wszFinalPath + cwchPrefix ) );

    //  generate an absolute path to the file using the volume guid root path to remove ambiguity
    //  created by possible multiple paths to the same volume

    cwchFinalPath = GetFinalPathNameByHandleW( hFile, wszFinalPath, cwchFinalPathMax, VOLUME_NAME_NONE );
    if ( cwchFinalPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchFinalPath >= cwchFinalPathMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrOSStrCbCopyW( wszVolumePath, cbVolumePathMax, pvhce->WszPath() ) );
    Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, wszFinalPath ) );

    //  generate an absolute path and an unambiguous file key path

    Call( ErrMakeKeyPath( wszVolumePath, wszKeyPath ) );

HandleError:
    if ( hFile && hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    if ( err < JET_errSuccess )
    {
        wszAnyAbsPath[ 0 ] = 0;
        wszKeyPath[ 0 ] = 0;
    }
    return err;
}

template< class I >
ERR TFileIdentification<I>::ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                                            _Out_   HANDLE* const       phFile,
                                            _Out_   VolumeId* const     pvolumeid,
                                            _Out_   FileId* const       pfileid )
{
    ERR             err         = JET_errSuccess;
    BOOL            fSuccess    = FALSE;
    FILE_ID_INFO    fileIdInfo  = { 0 };

    *phFile = NULL;
    *pvolumeid = volumeidInvalid;
    *pfileid = fileidInvalid;

    //  open the file by path

    *phFile = CreateFileW(  wszPath,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL );

    if ( !*phFile || *phFile == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    //  get the file id of the opened file

    fSuccess = GetFileInformationByHandleEx(    *phFile,
                                                FileIdInfo,
                                                &fileIdInfo,
                                                sizeof( fileIdInfo ) );
    if ( !fSuccess )
    {
        Call( ErrGetLastError() );
    }

    //  return the volume id and file id

    *pvolumeid = (VolumeId)fileIdInfo.VolumeSerialNumber;
    *pfileid = *((FileId*)fileIdInfo.FileId.Identifier);

    //  defend against illegal values for the volumeid and fileid

    if ( *pvolumeid == volumeidInvalid )
    {
        Error( ErrBlockCacheInternalError( wszPath, "InvalidVolumeId" ) );
    }
    if ( *pfileid == fileidInvalid )
    {
        Error( ErrBlockCacheInternalError( wszPath, "InvalidFileId" ) );
    }

    //  defend against truncation of the file id because we cannot handle that

    FILE_ID_128 fileId;
    memset( fileId.Identifier, 0, _cbrg( fileId.Identifier ) );
    *((FileId*)fileId.Identifier) = *pfileid;
    if ( memcmp( fileIdInfo.FileId.Identifier, fileId.Identifier, sizeof( fileId.Identifier) ) )
    {
        Error( ErrBlockCacheInternalError( wszPath, "TruncatedFileId" ) );
    }


HandleError:
    if ( err < JET_errSuccess )
    {
        if ( *phFile && *phFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle( *phFile );
        }
        *phFile = NULL;
        *pvolumeid = volumeidInvalid;
        *pfileid = fileidInvalid;
    }
    return err;
}

template< class I >
ERR TFileIdentification<I>::ErrMakeKeyPath( _In_z_                                                  WCHAR* const    wszAnyAbsPath,
                                            _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const    wszKeyPath )
{
    ERR     err         = JET_errSuccess;
    DWORD   cwchKeyPath = 0;

    wszKeyPath[ 0 ] = 0;

    //  convert the path to upper case to allow it to be compared for equality as would be done by
    //  the file system (NTFS, case preserving, case insensitive)

    cwchKeyPath = LCMapStringEx(    LOCALE_NAME_INVARIANT,
                                    LCMAP_UPPERCASE,
                                    wszAnyAbsPath,
                                    LOSStrLengthW( wszAnyAbsPath ) + 1,
                                    wszKeyPath,
                                    IFileIdentification::cwchKeyPathMax,
                                    NULL,
                                    NULL,
                                    0 );
    if ( cwchKeyPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchKeyPath > IFileIdentification::cwchKeyPathMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        wszKeyPath[ 0 ] = 0;
    }
    return err;
}


template< class I >
ERR TFileIdentification<I>::ErrOpenVolumeById(  _In_    const VolumeId              volumeid,
                                                _Out_   CVolumeHandleCacheEntry**   ppvhce )
{
    ERR             err     = JET_errSuccess;
    const WCHAR*    wszPath = NULL;
    HANDLE          hVolume = NULL;

    *ppvhce = NULL;

    if ( !( *ppvhce = PvhceFindVolumeByID( volumeid ) ) )
    {
        Call( ErrOpenVolumeByIdCacheMiss( volumeid, &wszPath, &hVolume ) );
        Call( ErrAddVolumeByID( volumeid, wszPath, hVolume, ppvhce ) );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        *ppvhce = NULL;
    }
    return err;
}

template< class I >
ERR TFileIdentification<I>::ErrOpenVolumeByIdCacheMiss( _In_                const VolumeId      volumeid,
                                                        _Outptr_result_z_   const WCHAR** const pwszPath,
                                                        _Out_               HANDLE* const       phVolume )
{
    ERR         err                         = JET_errSuccess;
    const DWORD cwchVolumeMax               = MAX_PATH;
    WCHAR       wszVolume[ cwchVolumeMax ]  = { 0 };
    HANDLE      hFindVolume                 = NULL;
    WCHAR*      wszPath                     = NULL;
    HANDLE      hVolume                     = NULL;

    *pwszPath = NULL;
    *phVolume = NULL;

    //  walk all volumes in the system and try to find one that matches the volume id

    hFindVolume = FindFirstVolumeW( wszVolume, cwchVolumeMax );

    if ( !hFindVolume || hFindVolume == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    do {
        //  try to determine the serial number for this volume.  note that this might fail if there isn't one!

        DWORD volumeSerialNumber;
        BOOL fSucceeded = GetVolumeInformationW( wszVolume, NULL, cwchVolumeMax, &volumeSerialNumber, NULL, NULL, NULL, 0 );
        if ( !fSucceeded )
        {
            continue;
        }

        //  if we got the serial number and it matches the volume id then we have a match

        if ( volumeSerialNumber == (DWORD)volumeid )
        {
            DWORD cwchVolume = LOSStrLengthW( wszVolume );
            if ( wszVolume[ cwchVolume - 1 ] == L'\\' )
            {
                cwchVolume = cwchVolume - 1;
                wszVolume[ cwchVolume ] = 0;
            }

            Alloc( wszPath = new WCHAR[ (size_t)cwchVolume + 1 ] );
            Call( ErrOSStrCbCopyW( wszPath, ( (size_t)cwchVolume + 1 ) * sizeof( WCHAR ), wszVolume ) );
            break;
        }

    } while ( FindNextVolumeW( hFindVolume, wszVolume, cwchVolumeMax ) );

    //  if we did not find the volume then fail

    if ( !wszPath )
    {
        Error( ErrERRCheck( JET_errInvalidPath ) );
    }

    //  open a handle to this volume

    hVolume = CreateFileW(  wszPath,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );

    if ( !hVolume || hVolume == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    //  return the volume we found

    *pwszPath = wszPath;
    wszPath = NULL;
    *phVolume = hVolume;
    hVolume = NULL;

HandleError:
    if ( hVolume && hVolume != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hVolume );
    }
    delete[] wszPath;
    if ( hFindVolume && hFindVolume != INVALID_HANDLE_VALUE )
    {
        FindVolumeClose( hFindVolume );
    }
    if ( err < JET_errSuccess )
    {
        delete[] *pwszPath;
        *pwszPath = NULL;
        if ( *phVolume && *phVolume != INVALID_HANDLE_VALUE )
        {
            CloseHandle( *phVolume );
        }
        *phVolume = NULL;
    }
    return err;
}

template< class I >
ERR TFileIdentification<I>::ErrGetLastError()
{
    return ErrOSErrFromWin32Err( GetLastError(), JET_errDiskIO );
}

template< class I >
void TFileIdentification<I>::PurgeVolumeHandleCache()
{
    while ( !m_ilVolumeHandleCache.FEmpty() )
    {
        CVolumeHandleCacheEntry* const pvhce = m_ilVolumeHandleCache.PrevMost();

        m_ilVolumeHandleCache.Remove( pvhce );
        delete pvhce;
    }
}

template< class I >
typename TFileIdentification<I>::CVolumeHandleCacheEntry* TFileIdentification<I>::PvhceFindVolumeByID( _In_ const VolumeId volumeid )
{
    CVolumeHandleCacheEntry* pvhceCached = NULL;

    m_rwlVolumeHandleCache.EnterAsReader();

    for (   pvhceCached = m_ilVolumeHandleCache.PrevMost();
            pvhceCached != NULL && pvhceCached->Volumeid() != volumeid;
            pvhceCached = m_ilVolumeHandleCache.Next( pvhceCached ) )
    {
    }

    m_rwlVolumeHandleCache.LeaveAsReader();

    return pvhceCached;
}

template< class I >
typename ERR TFileIdentification<I>::ErrAddVolumeByID(  _In_    const VolumeId                                          volumeid,
                                                        _In_z_  const WCHAR* const                                      wszPath,
                                                        _In_    const HANDLE                                            hVolume,
                                                        _Out_   TFileIdentification<I>::CVolumeHandleCacheEntry** const ppvhce )
{
    ERR                         err             = JET_errSuccess;
    CVolumeHandleCacheEntry*    pvhceNew        = NULL;
    CVolumeHandleCacheEntry*    pvhceCached     = NULL;

    *ppvhce = NULL;

    Alloc( pvhceNew = new CVolumeHandleCacheEntry( volumeid, wszPath, hVolume ) );

    m_rwlVolumeHandleCache.EnterAsWriter();

    for (   pvhceCached = m_ilVolumeHandleCache.PrevMost();
            pvhceCached != NULL && pvhceCached->Volumeid() != volumeid;
            pvhceCached = m_ilVolumeHandleCache.Next( pvhceCached ) )
    {
    }

    if ( pvhceCached == NULL )
    {
        m_ilVolumeHandleCache.InsertAsPrevMost( pvhceNew );
        pvhceCached = pvhceNew;
        pvhceNew = NULL;
    }

    m_rwlVolumeHandleCache.LeaveAsWriter();

    *ppvhce = pvhceCached;

HandleError:
    delete pvhceNew;
    if ( !pvhceNew && !pvhceCached )
    {
        CloseHandle( hVolume );
    }
    if ( err < JET_errSuccess )
    {
        delete *ppvhce;
        *ppvhce = NULL;
    }
    return err;
}

//  CFileIdentification:  concrete TFileIdentification<IFileIdentification>

class CFileIdentification : public TFileIdentification<IFileIdentification>
{
    public:  //  specialized API

        CFileIdentification()
            : TFileIdentification<IFileIdentification>()
        {
        }

        virtual ~CFileIdentification() {}
};
