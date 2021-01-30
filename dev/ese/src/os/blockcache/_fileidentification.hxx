// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


template< class I >
class TFileIdentification
    :   public I
{
    public:

        TFileIdentification();

        virtual ~TFileIdentification();

        void Cleanup() { PurgeVolumeHandleCache(); }

    public:

        ERR ErrGetFileId(   _In_z_  const WCHAR* const  wszPath,
                            _Out_   VolumeId* const     pvolumeid,
                            _Out_   FileId* const       pfileid ) override;

        ERR ErrGetFileKeyPath(  _In_z_                                  const WCHAR* const  wszPath,
                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath ) override;

        ERR ErrGetFilePathById( _In_                                    const VolumeId  volumeid,
                                _In_                                    const FileId    fileid,
                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszAnyAbsPath,
                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszKeyPath ) override;

    private:


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

        ERR ErrMakeKeyPath( _In_z_                                  WCHAR* const    wszAnyAbsPath,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszKeyPath );

        ERR ErrOpenVolumeById(  _In_    const VolumeId              volumeid,
                                _Out_   CVolumeHandleCacheEntry**   pvhce );

        ERR ErrOpenVolumeByIdCacheMiss( _In_                const VolumeId      volumeid,
                                        _Deref_out_opt_z_   const WCHAR** const pwszPath,
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
    PurgeVolumeHandleCache();
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
ERR TFileIdentification<I>::ErrGetFileKeyPath(  _In_z_                                  const WCHAR* const  wszPath,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath )
{
    ERR                         err                                 = JET_errSuccess;
    const DWORD                 cwchDirPathMax                      = OSFSAPI_MAX_PATH;
    WCHAR                       wszDirPath[ cwchDirPathMax ]        = { 0 };
    DWORD                       cwchDirPath                         = 0;
    WCHAR*                      wszFileName                         = NULL;
    WCHAR                       wchFileNameSave                     = 0;
    HANDLE                      hDirectory                          = NULL;
    VolumeId                    volumeid                            = volumeidInvalid;
    FileId                      fileid                              = fileidInvalid;
    CVolumeHandleCacheEntry*    pvhce                               = NULL;
    const DWORD                 cwchFinalPathMax                    = OSFSAPI_MAX_PATH;
    WCHAR                       wszFinalPath[ cwchFinalPathMax ]    = { 0 };
    DWORD                       cwchFinalPath                       = 0;
    const DWORD                 cwchVolumePathMax                   = OSFSAPI_MAX_PATH;
    const DWORD                 cbVolumePathMax                     = cwchVolumePathMax * sizeof( WCHAR );
    WCHAR                       wszVolumePath[ cwchVolumePathMax ]  = { 0 };

    wszKeyPath[ 0 ] = 0;


    cwchDirPath = GetFullPathNameW( wszPath, cwchDirPathMax, wszDirPath, &wszFileName );
    if ( cwchDirPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchDirPath >= cwchDirPathMax )
    {
        Call( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    if ( wszFileName )
    {
        wchFileNameSave = wszFileName[ 0 ];
        wszFileName[ 0 ] = 0;
    }


    err = ErrGetFileId( wszDirPath, &hDirectory, &volumeid, &fileid );
    err = err == JET_errFileNotFound ? ErrERRCheck( JET_errInvalidPath ) : err;
    Call( err );


    Call( ErrOpenVolumeById( volumeid, &pvhce ) );


    cwchFinalPath = GetFinalPathNameByHandleW( hDirectory, wszFinalPath, cwchFinalPathMax, VOLUME_NAME_NONE );
    if ( cwchFinalPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchFinalPath >= cwchFinalPathMax )
    {
        Call( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrOSStrCbCopyW( wszVolumePath, cbVolumePathMax, pvhce->WszPath() ) );
    Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, wszFinalPath ) );
    Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, L"\\" ) );
    if ( wszFileName )
    {
        wszFileName[ 0 ] = wchFileNameSave;
        Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, wszFileName ) );
    }


    Call( ErrMakeKeyPath( wszVolumePath, wszKeyPath ) );

HandleError:
    if ( hDirectory && hDirectory != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hDirectory );
    }
    if ( err < JET_errSuccess )
    {
        wszKeyPath[ 0 ] = 0;
    }
    return err;
}
        
template< class I >
ERR TFileIdentification<I>::ErrGetFilePathById( _In_                                    const VolumeId  volumeid,
                                                _In_                                    const FileId    fileid,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszAnyAbsPath,
                                                _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszKeyPath )
{
    ERR                         err                                 = JET_errSuccess;
    CVolumeHandleCacheEntry*    pvhce                               = NULL;
    FILE_ID_DESCRIPTOR          fileIdDescriptor                    = { 0 };
    HANDLE                      hFile                               = NULL;
    const DWORD                 cwchAnyAbsPathMax                   = OSFSAPI_MAX_PATH;
    const DWORD                 cbAnyAbsPathMax                     = cwchAnyAbsPathMax * sizeof( WCHAR );
    const WCHAR                 wszPrefix[]                         = L"\\\\?\\";
    const DWORD                 cwchPrefix                          = _countof( wszPrefix ) - 1;
    const DWORD                 cwchFinalPathMax                    = cwchPrefix + OSFSAPI_MAX_PATH;
    WCHAR                       wszFinalPath[ cwchFinalPathMax ]    = { 0 };
    DWORD                       cwchFinalPath                       = 0;
    const DWORD                 cwchVolumePathMax                   = OSFSAPI_MAX_PATH;
    const DWORD                 cbVolumePathMax                     = cwchVolumePathMax * sizeof( WCHAR );
    WCHAR                       wszVolumePath[ cwchVolumePathMax ]  = { 0 };

    wszAnyAbsPath[ 0 ] = 0;
    wszKeyPath[ 0 ] = 0;


    if ( volumeid == volumeidInvalid )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( fileid == fileidInvalid )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }


    err = ErrOpenVolumeById( volumeid, &pvhce );
    Call( err == JET_errInvalidPath ? ErrERRCheck( JET_errFileNotFound ) : err );


    fileIdDescriptor.dwSize = sizeof( fileIdDescriptor );
    fileIdDescriptor.Type = ObjectIdType;
    *((FileId*)fileIdDescriptor.ExtendedFileId.Identifier) = fileid;

    hFile = OpenFileById(   pvhce->Handle(),
                            &fileIdDescriptor,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        if ( GetLastError() == ERROR_INVALID_PARAMETER )
        {
            Call( ErrERRCheck( JET_errFileNotFound ) );
        }

        Call( ErrGetLastError() );
    }


    cwchFinalPath = GetFinalPathNameByHandleW( hFile, wszFinalPath, cwchFinalPathMax, VOLUME_NAME_DOS );
    if ( cwchFinalPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchFinalPath >= cwchFinalPathMax )
    {
        Call( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrOSStrCbCopyW( wszAnyAbsPath, cbAnyAbsPathMax, wszFinalPath + cwchPrefix ) );


    cwchFinalPath = GetFinalPathNameByHandleW( hFile, wszFinalPath, cwchFinalPathMax, VOLUME_NAME_NONE );
    if ( cwchFinalPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchFinalPath >= cwchFinalPathMax )
    {
        Call( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    Call( ErrOSStrCbCopyW( wszVolumePath, cbVolumePathMax, pvhce->WszPath() ) );
    Call( ErrOSStrCbAppendW( wszVolumePath, cbVolumePathMax, wszFinalPath ) );


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


    *phFile = CreateFileW(  wszPath,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                            NULL );

    if ( *phFile == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }


    fSuccess = GetFileInformationByHandleEx(    *phFile,
                                                FileIdInfo,
                                                &fileIdInfo,
                                                sizeof( fileIdInfo ) );
    if ( !fSuccess )
    {
        Call( ErrGetLastError() );
    }


    *pvolumeid = (VolumeId)fileIdInfo.VolumeSerialNumber;
    *pfileid = *((FileId*)fileIdInfo.FileId.Identifier);


    if ( *pvolumeid == volumeidInvalid )
    {
        Call( ErrERRCheck( JET_errInternalError ) );
    }
    if ( *pfileid == fileidInvalid )
    {
        Call( ErrERRCheck( JET_errInternalError ) );
    }


    FILE_ID_128 fileId;
    memset( fileId.Identifier, 0, _cbrg( fileId.Identifier ) );
    *((FileId*)fileId.Identifier) = *pfileid;
    if ( memcmp( fileIdInfo.FileId.Identifier, fileId.Identifier, sizeof( fileId.Identifier) ) )
    {
        Call( ErrERRCheck( JET_errInternalError ) );
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
ERR TFileIdentification<I>::ErrMakeKeyPath( _In_z_                                  WCHAR* const    wszAnyAbsPath,
                                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const    wszKeyPath )
{
    ERR         err             = JET_errSuccess;
    const DWORD cwchKeyPathMax  = OSFSAPI_MAX_PATH;
    DWORD       cwchKeyPath     = 0;

    wszKeyPath[ 0 ] = 0;


    cwchKeyPath = LCMapStringEx(    LOCALE_NAME_INVARIANT,
                                    LCMAP_UPPERCASE,
                                    wszAnyAbsPath,
                                    LOSStrLengthW( wszAnyAbsPath ) + 1,
                                    wszKeyPath,
                                    cwchKeyPathMax,
                                    NULL,
                                    NULL,
                                    0 );
    if ( cwchKeyPath == 0 )
    {
        Call( ErrGetLastError() );
    }
    if ( cwchKeyPath >= cwchKeyPathMax )
    {
        Call( ErrERRCheck( JET_errBufferTooSmall ) );
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
                                                        _Deref_out_opt_z_   const WCHAR** const pwszPath,
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


    hFindVolume = FindFirstVolumeW( wszVolume, cwchVolumeMax );

    if ( hFindVolume == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }

    do {

        DWORD volumeSerialNumber;
        BOOL fSucceeded = GetVolumeInformationW( wszVolume, NULL, cwchVolumeMax, &volumeSerialNumber, NULL, NULL, NULL, 0 );
        if ( !fSucceeded )
        {
            continue;
        }


        if ( volumeSerialNumber == (DWORD)volumeid )
        {
            DWORD cwchVolume = LOSStrLengthW( wszVolume );
            if ( wszVolume[ cwchVolume - 1 ] == L'\\' )
            {
                cwchVolume = cwchVolume - 1;
                wszVolume[ cwchVolume ] = 0;
            }

            Alloc( wszPath = new WCHAR[ cwchVolume + 1 ] );
            Call( ErrOSStrCbCopyW( wszPath, ( cwchVolume + 1 ) * sizeof( WCHAR ), wszVolume ) );
            break;
        }

    } while ( FindNextVolumeW( hFindVolume, wszVolume, cwchVolumeMax ) );


    if ( !wszPath )
    {
        Call( ErrERRCheck( JET_errInvalidPath ) );
    }


    hVolume = CreateFileW(  wszPath,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );

    if ( hVolume == INVALID_HANDLE_VALUE )
    {
        Call( ErrGetLastError() );
    }


    *pwszPath = wszPath;
    wszPath = NULL;
    *phVolume = hVolume;
    hVolume = NULL;

HandleError:
    if ( hVolume && hVolume != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hVolume );
    }
    delete wszPath;
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


class CFileIdentification : public TFileIdentification<IFileIdentification>
{
    public:

        CFileIdentification()
            : TFileIdentification<IFileIdentification>()
        {
        }

        virtual ~CFileIdentification() {}
};
