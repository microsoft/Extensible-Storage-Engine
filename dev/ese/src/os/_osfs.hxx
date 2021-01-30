// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef __OSFS_HXX_INCLUDED
#define __OSFS_HXX_INCLUDED



class COSFileSystem
    :   public IFileSystemAPI
{
    public:


        COSFileSystem( IFileSystemConfiguration * const pfsconfig );


        ERR ErrGetLastError( const DWORD error = GetLastError() );


        ERR ErrPathRoot(    const WCHAR* const  wszPath,
                            __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) WCHAR* const       wszAbsRootPath );

        void PathVolumeCanonicalAndDiskId(  const WCHAR* const wszAbsRootPath,
                                            __out_ecount(cchVolumeCanonicalPath) WCHAR* const wszVolumeCanonicalPath,
                                            __in const DWORD cchVolumeCanonicalPath,
                                            __out_ecount(cchDiskId) WCHAR* const wszDiskId,
                                            __in const DWORD cchDiskId,
                                            __out DWORD *pdwDiskNumber );


        ERR ErrCommitFSChange( const WCHAR* const wszPath );


        ERR ErrOSFSGetDeviceHandle( __in PCWSTR wszAbsVolumeRootPath, __out HANDLE* phDevice );


        void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;


        IFileSystemConfiguration* const Pfsconfig() const { return m_pfsconfig; }

    public:

        virtual ~COSFileSystem();

        ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                            QWORD* const        pcbFreeForUser,
                            QWORD* const        pcbTotalForUser = NULL,
                            QWORD* const        pcbFreeOnDisk = NULL ) override;


        ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                DWORD* const        pcbSize ) override;

        ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                    DWORD* const        pcbSize ) override;

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
                                    _Out_ BOOL *                        pfCanProcessUseRelativePaths ) override;


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

        ERR ErrFileCreate(  _In_z_ const WCHAR* const       wszPath,
                            _In_   IFileAPI::FileModeFlags  fmf,
                            _Out_  IFileAPI** const         ppfapi ) override;

        ERR ErrFileOpen(    _In_z_ const WCHAR* const       wszPath,
                            _In_   IFileAPI::FileModeFlags  fmf,
                            _Out_  IFileAPI** const         ppfapi ) override;

    public:


        VOID ReportDiskError( const MessageId msgid,
            const WCHAR * const wszPath,
            const WCHAR * wszAbsRootPath,
            JET_ERR err,
            const DWORD error );
        VOID ReportFileErrorInternal( const MessageId msgid,
            const WCHAR * const wszSrcPath,
            const WCHAR * const wszDstPath,
            JET_ERR err,
            const DWORD error );
        VOID ReportFileError( const MessageId msgid,
            const WCHAR * const wszSrcPath,
            const WCHAR * const wszDstPath,
            JET_ERR err,
            const DWORD error );
        VOID ReportFileErrorWithFilter( const MessageId msgid,
            const WCHAR * const wszSrcPath,
            const WCHAR * const wszDstPath,
            JET_ERR err,
            const DWORD error );

    private:

        ERR ErrSetPrivilege( const WCHAR *  wszPriv,
                             const BOOL     fEnable,
                             BOOL * const   pfJustChanged = NULL );
        ERR ErrCommitFSChangeSlowly( const WCHAR* const wszPath );
        ERR ErrCommitFSChangeSlowly( const HANDLE hFile );


        ERR ErrPathVolumeCanonical( const WCHAR* const wszAbsRootPath,
                                    __out_ecount(cchVolumeCanonicalPath) WCHAR* const wszVolumeCanonicalPath,
                                    __in const DWORD cchVolumeCanonicalPath );


        ERR ErrDiskId(  const WCHAR* const wszVolumeCanonicalPath,
                        __out_ecount(cchDiskId) WCHAR* const wszDiskId,
                        __in const DWORD cchDiskId,
                        __out DWORD *pdwDiskNumber );

    private:

        typedef WINBASEAPI BOOL WINAPI PfnGetVolumePathName( LPCWSTR, LPWSTR, DWORD );
        typedef WINBASEAPI BOOL WINAPI PfnGetVolumeNameForVolumeMountPoint( LPCWSTR, LPWSTR, DWORD );

        class CVolumePathCacheEntry;
        CVolumePathCacheEntry * GetVolInfoCacheEntry( __in PCWSTR wszTargetPath );

        class CVolumePathCacheEntry
        {
            public:

                CVolumePathCacheEntry( __in PCWSTR wszPathKey )
                {
                    OSStrCbCopyW( m_wszKeyPath, sizeof( m_wszKeyPath ), wszPathKey );
                    ResetCaches();
                }
                ~CVolumePathCacheEntry() {}

                static SIZE_T OffsetOfILE() { return OffsetOf( CVolumePathCacheEntry, m_ile ); }

                enum { dtickRefresh = 100 * 1000 };

                void ResetCaches( void )
                {
                    m_wszVolumePath[0] = L'\0';
                    m_wszVolumeCanonicalPath[0] = L'\0';
                    m_wszDiskId[0] = L'\0';
                    m_cbVolSecSize = 0;
                    m_dwDiskNumber = 0;
                    m_tickRefresh = TickOSTimeCurrent() + CVolumePathCacheEntry::dtickRefresh;
                }

                BOOL FGetVolPath( _Out_z_bytecap_(cbVolumePath) PWSTR wszVolumePath, ULONG cbVolumePath )
                {
                    Assert( wszVolumePath && cbVolumePath );
                    if ( FVPCEStale() || m_wszVolumePath[0] == L'\0' )
                    {
                        return fFalse;
                    }
                    OSStrCbCopyW( wszVolumePath, cbVolumePath, m_wszVolumePath );
                    return fTrue;
                }

                BOOL FGetVolCanonicalPath( _Out_z_bytecap_(cbVolumeCanonicalPath) PWSTR wszVolumeCanonicalPath, ULONG cbVolumeCanonicalPath )
                {
                    Assert( wszVolumeCanonicalPath && cbVolumeCanonicalPath );
                    if ( FVPCEStale() || m_wszVolumeCanonicalPath[0] == L'\0' )
                    {
                        return fFalse;
                    }
                    OSStrCbCopyW( wszVolumeCanonicalPath, cbVolumeCanonicalPath, m_wszVolumeCanonicalPath );
                    return fTrue;
                }

                BOOL FGetDiskId( _Out_z_bytecap_(cbDiskId) PWSTR wszDiskId, ULONG cbDiskId )
                {
                    Assert( wszDiskId && cbDiskId );
                    if ( FVPCEStale() || m_wszDiskId[0] == L'\0' )
                    {
                        return fFalse;
                    }
                    OSStrCbCopyW( wszDiskId, cbDiskId, m_wszDiskId );
                    return fTrue;
                }

                BOOL FGetSecSize( __out ULONG * pcbSecSize )
                {
                    Assert( pcbSecSize );
                    if ( FVPCEStale() || m_cbVolSecSize == 0 )
                    {
                        return fFalse;
                    }
                    *pcbSecSize = m_cbVolSecSize;
                    return fTrue;
                }

                DWORD DwDiskNumber() const { return m_dwDiskNumber; }

                void SetVolPath( __in PCWSTR wszVolumePath )
                {
                    Assert( wszVolumePath );
                    if( FVPCEStale() )
                    {
                        ResetCaches();
                    }
                    OSStrCbCopyW( m_wszVolumePath, sizeof( m_wszVolumePath ), wszVolumePath );
                }

                void SetVolCanonicalPath( __in PCWSTR wszVolumeCanonicalPath )
                {
                    Assert( wszVolumeCanonicalPath );
                    if( FVPCEStale() )
                    {
                        ResetCaches();
                    }
                    OSStrCbCopyW( m_wszVolumeCanonicalPath, sizeof( m_wszVolumeCanonicalPath ), wszVolumeCanonicalPath );
                }

                void SetDiskId( __in PCWSTR wszDiskId )
                {
                    Assert( wszDiskId );
                    if( FVPCEStale() )
                    {
                        ResetCaches();
                    }
                    OSStrCbCopyW( m_wszDiskId, sizeof( m_wszDiskId ), wszDiskId );
                }

                void SetSecSize( __in DWORD cbSecSize )
                {
                    Assert( ( cbSecSize >= 512 ) && FPowerOf2( cbSecSize ) );
                    if( FVPCEStale() )
                    {
                        ResetCaches();
                    }
                    m_cbVolSecSize = cbSecSize;
                }

                void SetDiskNumber( __in const DWORD dwDiskNumber )
                {
                    m_dwDiskNumber = dwDiskNumber;
                }

            private:
                inline BOOL FVPCEStale( void )
                {
                    return ( TickCmp( TickOSTimeCurrent(), m_tickRefresh ) >= 0 );
                }

            private:

                CInvasiveList< CVolumePathCacheEntry, OffsetOfILE >::CElement
                        m_ile;

                WCHAR   m_wszKeyPath[ IFileSystemAPI::cchPathMax ];
                TICK    m_tickRefresh;

                WCHAR   m_wszVolumePath[ IFileSystemAPI::cchPathMax ];
                WCHAR   m_wszVolumeCanonicalPath[ IFileSystemAPI::cchPathMax ];
                WCHAR   m_wszDiskId[ IFileSystemAPI::cchPathMax ];
                DWORD   m_cbVolSecSize;
                DWORD   m_dwDiskNumber;

            friend CVolumePathCacheEntry * COSFileSystem::GetVolInfoCacheEntry( __in PCWSTR wszTargetPath );

        };

    private:

        IFileSystemConfiguration * const    m_pfsconfig;
        CCriticalSection                    m_critVolumePathCache;
        CInvasiveList< CVolumePathCacheEntry, CVolumePathCacheEntry::OffsetOfILE >
                                            m_ilVolumePathCache;
};


class COSFileFind
    :   public IFileFindAPI
{
    public:


        COSFileFind();


        ERR ErrInit(    COSFileSystem* const    posfs,
                        const WCHAR* const      wszFindPath );


        void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

    public:

        virtual ~COSFileFind();

        ERR ErrNext() override;

        ERR ErrIsFolder( BOOL* const pfFolder ) override;
        ERR ErrPath( _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) WCHAR* const wszAbsPath ) override;
        ERR ErrSize(    _Out_ QWORD* const pcbSize,
                        _In_ const IFileAPI::FILESIZE filesize ) override;
        ERR ErrIsReadOnly( BOOL* const pfReadOnly ) override;

    private:

        COSFileSystem*  m_posfs;
        WCHAR           m_wszFindPath[ IFileSystemAPI::cchPathMax ];

        WCHAR           m_wszAbsFindPath[ IFileSystemAPI::cchPathMax ];
        BOOL            m_fBeforeFirst;
        ERR             m_errFirst;
        HANDLE          m_hFileFind;

        ERR             m_errCurrent;
        BOOL            m_fFolder;
        WCHAR           m_wszAbsFoundPath[ IFileSystemAPI::cchPathMax ];
        QWORD           m_cbSize;
        BOOL            m_fReadOnly;
};



class COSVolume
    :   public IVolumeAPI
{

    public:
        static SIZE_T OffsetOfILE() { return OffsetOf( COSVolume, m_ile ); }
    private:
        CInvasiveList< COSVolume, OffsetOfILE >::CElement m_ile;


    public:

        COSVolume();
        ERR ErrInitVolume( __in_z const WCHAR * const wszVolPath, __in_z const WCHAR * const wszVolCanonicalPath );

    public:

        virtual ~COSVolume();



    public:
        void AddRef();
        void Release();
        virtual ULONG CRef() const;

    public:
        void AssertValid();
        BOOL FIsVolume( __in_z const WCHAR * const wszTargetVolume ) const;

    public:



        virtual ERR ErrDiskSpace(   const WCHAR* const  wszPath,
                                    QWORD* const        pcbFreeForUser,
                                    QWORD* const        pcbTotalForUser = NULL,
                                    QWORD* const        pcbFreeOnDisk = NULL );



        virtual ERR ErrFileSectorSize(  const WCHAR* const  wszPath,
                                        DWORD* const        pcbSize );


        virtual ERR ErrFileAtomicWriteSize( const WCHAR* const  wszPath,
                                            DWORD* const        pcbSize );

        
        virtual BOOL FDiskFixed();

        
        virtual ERR ErrDiskId( ULONG_PTR* const pulDiskId ) const;


        virtual BOOL FSeekPenalty() const
        {
            return m_posd->FSeekPenalty();
        }

    public:



        const WCHAR * WszVolPath() const;


        const WCHAR * WszVolCanonicalPath() const;


        ERR ErrGetDisk( COSDisk ** pposd );

    private:

        const static UINT DRIVE_UNINIT_ESE = 0xFFFFFF01;
        C_ASSERT( DRIVE_UNINIT_ESE != DRIVE_UNKNOWN );
        C_ASSERT( DRIVE_UNINIT_ESE != DRIVE_FIXED );

        ULONG       m_cref;
        WCHAR       m_wszVolPath[IFileSystemAPI::cchPathMax];
        WCHAR       m_wszVolCanonicalPath[IFileSystemAPI::cchPathMax];
        COSDisk *   m_posd;
        UINT        m_eosDiskType;



    friend ERR ErrOSVolumeConnect(
            __in COSFileSystem * const      posfs,
            __in_z const WCHAR * const      wszFilePath,
            __out IVolumeAPI **             ppvolapi );
    friend void OSVolumeDisconnect(
            __inout IVolumeAPI *            pvolapi );

    friend COSDisk * PosdEDBGGetDisk( _In_ const COSVolume * const posv );

};

class OSFSRETRY
{
    public:
        OSFSRETRY( IFileSystemConfiguration* const pfsconfig );

        BOOL FRetry( const JET_ERR err );

    private:
        IFileSystemConfiguration* const m_pfsconfig;
        BOOL                            m_fInitialized;
        TICK                            m_tickEnd;
};


ERR ErrOSVolumeConnect(
    __in COSFileSystem * const      posfs,
    __in_z const WCHAR * const      wszFilePath,
    __out IVolumeAPI **             ppvolapi
    );

void OSVolumeDisconnect(
    __inout IVolumeAPI *            pvolapi );

inline COSVolume * PosvFromVolAPI( __inout IVolumeAPI *                     pvolapi )
{
    return (COSVolume*) pvolapi;
}


enum OSDiskMappingMode {
    eOSDiskInvalidMode = 0,
    eOSDiskLastDiskOnEarthMode,
    eOSDiskOneDiskPerVolumeMode,
    eOSDiskOneDiskPerPhysicalDisk,
    eOSDiskMonogamousMode,
};


INLINE OSDiskMappingMode GetDiskMappingMode();
INLINE extern void SetDiskMappingMode( const OSDiskMappingMode diskMode );



DWORD DwDesiredAccessFromFileModeFlags( const IFileAPI::FileModeFlags fmf );
DWORD DwShareModeFromFileModeFlags( const IFileAPI::FileModeFlags fmf );
DWORD DwCreationDispositionFromFileModeFlags( const BOOL fCreate, const IFileAPI::FileModeFlags fmf );
DWORD DwFlagsAndAttributesFromFileModeFlags( const IFileAPI::FileModeFlags fmf );

#endif


