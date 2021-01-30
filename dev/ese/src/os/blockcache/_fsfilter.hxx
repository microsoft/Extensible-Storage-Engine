// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

class CFileFilter;
class CFileFilterReference;
static SIZE_T CFileFilterReferenceOffsetOfILE();


class CFilePathTableEntry
{
    public:

        class COpenFile;

    public:

        CFilePathTableEntry( _Inout_z_ const WCHAR** const pwszKeyPath );

        ~CFilePathTableEntry();

        const WCHAR* WszKeyPath() const { return m_wszKeyPath; }
        COpenFile* Pof() const { return m_pof;  }
        UINT UiHash() const { return CFilePathTableEntry::UiHash( m_wszKeyPath ); }

        void AddRef( _In_ int dref = 1 ) { m_cref += dref; }
        BOOL FRelease( _In_ int dref = 1 ) { m_cref -= dref;  return m_cref == 0; }

        ERR ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem );
        void AddNextOwner();
        void RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem );

        static UINT UiHash( _In_z_ const WCHAR* const wszKeyPath );

        ERR ErrOpenFile( _Inout_ CFileFilter** const ppff, _In_ ICache* const pc, _Out_ COpenFile** const ppof );
        void RemoveFile( _In_ COpenFile* const pof );
        void DisconnectFile( _In_ COpenFile* const pof );
        void TransferFiles( _In_ CFilePathTableEntry* const pfpteDest );

        ERR ErrAccessCheck( _In_ const IFileAPI::FileModeFlags  fmf,
                            _In_ const BOOL                     fCreate,
                            _In_ const BOOL                     fCacheOpen );
        ERR ErrDeleteCheck();

    public:


        class COpenFile
        {
            public:

                COpenFile(  _In_    CFilePathTableEntry* const  pfpte,
                            _Inout_ CFileFilter** const         ppff,
                            _In_    ICache* const               pc )
                    :   m_pfpte( pfpte ),
                        m_pff( *ppff ),
                        m_pc( pc )
                {
                    *ppff = NULL;
                }

                ~COpenFile()
                {
                    delete m_pff;
                }

                CFilePathTableEntry* Pfpte() const { return m_pfpte; }
                CFileFilter* Pff() const { return m_pff; }
                ICache* Pc() const { return m_pc;  }
                int CReference() const { return m_ilReferences.Count();  }

                void AddReference( _In_ CFileFilterReference* const pffr )
                {
                    m_ilReferences.InsertAsNextMost( pffr );
                }

                BOOL FRemoveReference( _In_ CFileFilterReference* const pffr )
                {
                    m_ilReferences.Remove( pffr );
                    return m_ilReferences.FEmpty();
                }

                void SetPfpte( _In_ CFilePathTableEntry* const pfpte )
                {
                    m_pfpte = pfpte;
                }

                ERR ErrAccessCheck( _In_ const IFileAPI::FileModeFlags  fmf,
                                    _In_ const BOOL                     fCreate,
                                    _In_ const BOOL                     fCacheOpen );
                ERR ErrDeleteCheck();

                static SIZE_T OffsetOfILE() { return OffsetOf( COpenFile, m_ile ); }

            private:

                typename CInvasiveList< COpenFile, OffsetOfILE >::CElement                  m_ile;
                CFilePathTableEntry*                                                        m_pfpte;
                CFileFilter*                                                                m_pff;
                ICache*                                                                     m_pc;
                CCountedInvasiveList<CFileFilterReference, CFileFilterReferenceOffsetOfILE> m_ilReferences;
        };

    private:


        class CWaiter
        {
            public:

                CWaiter( _In_ CSemaphore* const psem )
                    :   m_psem( psem )
                {
                }

                CSemaphore* Psem() { return m_psem; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CWaiter, m_ile ); }

            private:

                typename CInvasiveList< CWaiter, OffsetOfILE >::CElement    m_ile;
                CSemaphore* const                                           m_psem;
        };

    private:

        const WCHAR* const                                  m_wszKeyPath;
        int                                                 m_cref;
        CSemaphore*                                         m_psemOwner;
        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>        m_ilWaiters;
        CInvasiveList<COpenFile, COpenFile::OffsetOfILE>    m_ilOpenFiles;
        COpenFile*                                          m_pof;
};

INLINE CFilePathTableEntry::CFilePathTableEntry( _Inout_z_ const WCHAR** const pwszKeyPath )
    :   m_wszKeyPath( *pwszKeyPath ),
        m_cref( 0 ),
        m_psemOwner( NULL ),
        m_pof( NULL )
{
    *pwszKeyPath = NULL;
}

INLINE CFilePathTableEntry::~CFilePathTableEntry()
{
    delete[] m_wszKeyPath;
}

INLINE ERR CFilePathTableEntry::ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem )
{
    ERR         err     = JET_errSuccess;
    CWaiter*    pwaiter = NULL;

    if ( !m_psemOwner )
    {
        m_psemOwner = psem;
        m_psemOwner->Release();
    }
    else
    {
        Alloc( pwaiter = new CWaiter( psem ) );

        m_ilWaiters.InsertAsNextMost( pwaiter );
        pwaiter = NULL;
    }

HandleError:
    delete pwaiter;
    return err;
}

INLINE void CFilePathTableEntry::AddNextOwner()
{
    CWaiter* pwaiter = m_ilWaiters.PrevMost();
    if ( pwaiter != NULL )
    {
        m_ilWaiters.Remove( pwaiter );
        m_psemOwner = pwaiter->Psem();
        m_psemOwner->Release();
        delete pwaiter;
    }
}

INLINE void CFilePathTableEntry::RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem )
{
    CWaiter* pwaiter = m_ilWaiters.PrevMost();
    while ( pwaiter != NULL )
    {
        CWaiter* const pwaiterNext = m_ilWaiters.Next( pwaiter );
        if ( pwaiter->Psem() == psem )
        {
            m_ilWaiters.Remove( pwaiter );
            delete pwaiter;
        }
        pwaiter = pwaiterNext;
    }

    if ( m_psemOwner == psem )
    {
        m_psemOwner = NULL;
    }
}

INLINE UINT CFilePathTableEntry::UiHash( _In_z_ const WCHAR* const wszKeyPath )
{
    const INT cbKeyPath = wszKeyPath ? LOSStrLengthW( wszKeyPath ) * sizeof( wszKeyPath[ 0 ] ) : 0;

    unsigned __int64 ui64Hash = Ui64FNVHash( wszKeyPath, cbKeyPath );

    return (UINT)( ( ui64Hash >> 32 ) | ( ui64Hash & dwMax ) );
}

INLINE ERR CFilePathTableEntry::ErrOpenFile(    _Inout_ CFileFilter** const ppff, 
                                                _In_    ICache* const       pc,
                                                _Out_   COpenFile** const   ppof )
{
    ERR         err = JET_errSuccess;
    COpenFile*  pof = NULL;

    *ppof = NULL;

    Alloc( pof = new COpenFile( this, ppff, pc ) );

    m_ilOpenFiles.InsertAsPrevMost( pof );

    *ppof = pof;

    m_pof = pof;
    pof = NULL;

HandleError:
    delete pof;
    if ( err < JET_errSuccess )
    {
        *ppof = NULL;
    }
    return err;
}

INLINE void CFilePathTableEntry::RemoveFile( _In_ COpenFile* const pof )
{
    if ( !pof )
    {
        return;
    }

    DisconnectFile( pof );

    m_ilOpenFiles.Remove( pof );
}

INLINE void CFilePathTableEntry::DisconnectFile( _In_ COpenFile* const pof )
{
    m_pof = m_pof == pof ? NULL : m_pof;
}

INLINE void CFilePathTableEntry::TransferFiles( _In_ CFilePathTableEntry* const pfpteDest )
{
    while ( COpenFile* pof = m_ilOpenFiles.NextMost() )
    {
        m_ilOpenFiles.Remove( pof );
        BOOL fRelease = FRelease( pof->CReference() );
        Assert( !fRelease );
        pof->SetPfpte( pfpteDest );
        pfpteDest->m_ilOpenFiles.InsertAsPrevMost( pof );
        pfpteDest->m_pof = pof;
        pfpteDest->AddRef( pof->CReference() );
    }
    m_pof = NULL;
}

INLINE ERR CFilePathTableEntry::ErrAccessCheck( _In_ const IFileAPI::FileModeFlags  fmf,
                                                _In_ const BOOL                     fCreate,
                                                _In_ const BOOL                     fCacheOpen )
{
    ERR err = JET_errSuccess;

    for (   COpenFile* pof = m_ilOpenFiles.PrevMost();
            pof;
            pof = m_ilOpenFiles.Next( pof ) )
    {
        Call( pof->ErrAccessCheck( fmf, fCreate, fCacheOpen ) );
    }

HandleError:
    return err;
}

INLINE ERR CFilePathTableEntry::ErrDeleteCheck()
{
    ERR err = JET_errSuccess;

    for (   COpenFile* pof = m_ilOpenFiles.PrevMost();
            pof;
            pof = m_ilOpenFiles.Next( pof ) )
    {
        Call( pof->ErrDeleteCheck() );
    }

HandleError:
    return err;
}


class CFilePathHashKey
{
    public:

        CFilePathHashKey()
            :   m_wszKeyPath( NULL ),
                m_uiHash( 0 )
        {
        }

        CFilePathHashKey( _In_z_ const WCHAR* const wszKeyPath )
            :   m_wszKeyPath( wszKeyPath ),
                m_uiHash( CFilePathTableEntry::UiHash( wszKeyPath ) )
        {
        }

        CFilePathHashKey( _In_ CFilePathTableEntry* const pfpte )
            :   CFilePathHashKey( pfpte->WszKeyPath() )
        {
        }

        CFilePathHashKey( _In_ const CFilePathHashKey& src )
        {
            *this = src;
        }

        const CFilePathHashKey& operator=( _In_ const CFilePathHashKey& src )
        {
            m_wszKeyPath = src.m_wszKeyPath;
            m_uiHash = src.m_uiHash;

            return *this;
        }

        const WCHAR* WszKeyPath() const { return m_wszKeyPath; }
        UINT UiHash() const { return m_uiHash; }

    private:

        const WCHAR*    m_wszKeyPath;
        UINT            m_uiHash;
};


class CFilePathHashEntry
{
    public:

        CFilePathHashEntry()
            :   m_pfpte( NULL ),
                m_uiHash( 0 )
        {
        }

        CFilePathHashEntry( _In_ CFilePathTableEntry* const pfpte )
            :   m_pfpte( pfpte ),
                m_uiHash( pfpte->UiHash() )
        {
        }

        CFilePathHashEntry( _In_ const CFilePathHashEntry& src )
        {
            *this = src;
        }

        const CFilePathHashEntry& operator=( _In_ const CFilePathHashEntry& src )
        {
            m_pfpte = src.m_pfpte;
            m_uiHash = src.m_uiHash;

            return *this;
        }

        CFilePathTableEntry* Pfpte() const { return m_pfpte; }
        UINT UiHash() const { return m_uiHash; }

    private:

        CFilePathTableEntry*    m_pfpte;
        UINT                    m_uiHash;
};


typedef CDynamicHashTable<CFilePathHashKey, CFilePathHashEntry> CFilePathHash;


template< class I >
class TFileSystemFilter
    :   public TFileSystemWrapper<I>
{
    public:

        TFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ IFileSystemAPI** const          ppfsapi,
                            _In_    IFileIdentification* const      pfident,
                            _In_    ICacheTelemetry* const          pctm,
                            _In_    ICacheRepository * const        pcrep );

        virtual ~TFileSystemFilter();

        void AddFileReference(  _In_ CFileFilterReference* const            pffr,
                                _In_ CFilePathTableEntry::COpenFile* const  pof );

        void RemoveFileReference(   _In_ CFileFilterReference* const            pffr,
                                    _In_ CFilePathTableEntry::COpenFile* const  pof );

    public:


        ERR ErrFileOpenById(    _In_    const VolumeId                  volumeid,
                                _In_    const FileId                    fileid,
                                _In_    const FileSerial                fileserial,
                                _In_    const IFileAPI::FileModeFlags   fmf,
                                _Out_   IFileAPI** const                ppfapi ) override;

        ERR ErrFileRename(  _In_    IFileAPI* const    pfapi,
                            _In_z_  const WCHAR* const wszPathDest,
                            _In_    const BOOL         fOverwriteExisting ) override;

    public:

        ERR ErrFileDelete( const WCHAR* const wszPath );
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

    public:

        ERR ErrWrapFile( _Inout_ IFileAPI** const ppfapiInner, _Out_ IFileAPI** const ppfapi ) override;

    private:

        void ReleaseFile(   _In_opt_    CFilePathTableEntry* const              pfpte,
                            _In_opt_    CSemaphore* const                       psem    = NULL,
                            _In_opt_    CFilePathTableEntry::COpenFile* const   pof     = NULL,
                            _In_opt_    CFileFilterReference* const             pffr    = NULL );

        ERR ErrLockFile(    _In_z_                                  const WCHAR* const          wszPath,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const                wszAbsPath,
                            _In_                                    CSemaphore* const           psem,
                            _Out_                                   CFilePathTableEntry** const ppfpte );
        ERR ErrLockFiles(   _In_z_                                  const WCHAR* const          wszPathSrc, 
                            _In_z_                                  const WCHAR* const          wszPathDest,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const                wszAbsPathSrc,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const                wszAbsPathDest,
                            _In_                                    CSemaphore* const           psem,
                            _Out_                                   CFilePathTableEntry** const ppfpteSrc,
                            _Out_                                   CFilePathTableEntry** const ppfpteDest );
        ERR ErrLockFile(    _In_z_  const WCHAR* const          wszKeyPath,
                            _In_    CSemaphore* const           psem,
                            _Out_   CFilePathTableEntry** const ppfpte );
        ERR ErrGetFilePath( _In_z_                                  const WCHAR* const  wszPath,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszAbsPath,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath );

        ERR ErrPrepareToDelete( _In_ CFilePathTableEntry* const pfpte );

        ERR ErrFileCreateCacheMiss( _In_z_  const WCHAR* const              wszAnyAbsPath,
                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                    _In_    CFilePathTableEntry* const      pfpte,
                                    _Out_   CFileFilterReference** const    ppffr );

        ERR ErrFileConfigure(   _In_        CFileFilter* const          pff,
                                _In_z_      const WCHAR* const          wszAnyAbsPath );

        ERR ErrFileOpenInternal(    _In_z_  const WCHAR* const              wszAnyAbsPath,
                                    _In_z_  const WCHAR* const              wszKeyPath,
                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                    _In_    const BOOL                      fCacheOpen,
                                    _Out_   CFileFilterReference** const    ppffr );
        ERR ErrFileOpenCacheMiss(   _In_z_  const WCHAR* const              wszAnyAbsPath,
                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                    _In_    const BOOL                      fCacheOpen,
                                    _In_    CFilePathTableEntry* const      pfpte,
                                    _Out_   CFileFilterReference** const    ppffr );
        ERR ErrFileOpenAndConfigure(    _In_z_  const WCHAR* const              wszAnyAbsPath,
                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                        _In_    CFilePathTableEntry* const      pfpte,
                                        _Out_   CFileFilter** const             ppff,
                                        _Out_   ICache** const                  ppc,
                                        _Out_   BOOL* const                     pfAttached );
        ERR ErrFileOpenCacheHit(    _In_    const IFileAPI::FileModeFlags           fmf,
                                    _In_    const BOOL                              fCreate,
                                    _In_    const BOOL                              fCacheOpen,
                                    _In_    CFilePathTableEntry::COpenFile* const   pof,
                                    _Out_   CFileFilterReference** const            ppffr );
                
        void ReportCachedFileNotFoundById(  _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial );

        ERR ErrGetConfiguredCache(  _In_    CFileFilter* const  pff,
                                    _In_z_  const WCHAR* const  wszKeyPath,
                                    _Out_   ICache** const      ppc );
        void ReportCacheOpenFailure( _In_ ICachedFileConfiguration* const pcfconfig, _In_ const ERR err );
        ERR ErrGetCache(    _In_    CFileFilter* const          pff,
                            _In_z_  const WCHAR* const          wszKeyPath,
                            _Out_   ICache** const              ppc,
                            _Out_   BOOL* const                 pfAttached );

        ERR ErrGetConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig );
        ERR ErrGetConfiguration();
        static ERR ErrGetConfiguration_( _In_ TFileSystemFilter<I>* const pfsf ) { return pfsf->ErrGetConfiguration(); }

        ERR ErrEnsureInitFilePathTable() { return m_initOnceFilePathTable.Init( ErrInitFilePathTable_, this ); };
        ERR ErrInitFilePathTable();
        static ERR ErrInitFilePathTable_( _In_ TFileSystemFilter<I>* const pfsf ) { return pfsf->ErrInitFilePathTable(); }
        void TermFilePathTable();

    private:

        IFileSystemConfiguration* const                                                     m_pfsconfig;
        IFileIdentification* const                                                          m_pfident;
        ICacheTelemetry* const                                                              m_pctm;
        ICacheRepository* const                                                             m_pcrep;
        CInitOnce< ERR, decltype( &ErrInitFilePathTable_ ), TFileSystemFilter<I>* const >   m_initOnceFilePathTable;
        CFilePathHash                                                                       m_filePathHash;
        IBlockCacheConfiguration*                                                           m_pbcconfig;
        CInitOnce< ERR, decltype( &ErrGetConfiguration_ ), TFileSystemFilter<I>* const >    m_initOnceAttach;
};

template< class I >
TFileSystemFilter<I>::TFileSystemFilter(    _In_    IFileSystemConfiguration* const pfsconfig,
                                            _Inout_ IFileSystemAPI** const          ppfsapi,
                                            _In_    IFileIdentification* const      pfident,
                                            _In_    ICacheTelemetry* const          pctm,
                                            _In_    ICacheRepository * const        pcrep )
    :   TFileSystemWrapper<I>( (I** const)ppfsapi ),
        m_pfsconfig( pfsconfig ),
        m_pfident( pfident ),
        m_pctm( pctm ),
        m_pcrep( pcrep ),
        m_filePathHash( 0 ),
        m_pbcconfig( NULL )
{
}

template< class I >
TFileSystemFilter<I>::~TFileSystemFilter()
{
    delete m_pbcconfig;
    TermFilePathTable();
}

template< class I >
void TFileSystemFilter<I>::AddFileReference(    _In_ CFileFilterReference* const            pffr,
                                                _In_ CFilePathTableEntry::COpenFile* const  pof )
{
    pof->Pfpte()->AddRef();
    pof->AddReference( pffr );
}

template< class I >
void TFileSystemFilter<I>::RemoveFileReference( _In_ CFileFilterReference* const            pffr,
                                                _In_ CFilePathTableEntry::COpenFile* const  pof )
{

    if ( !pffr->FCacheOpen() && pof->Pc() )
    {
        VolumeId    volumeid    = volumeidInvalid;
        FileId      fileid      = fileidInvalid;
        FileSerial  fileserial  = fileserialInvalid;

        CallS( pof->Pff()->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );
        CallS( pof->Pc()->ErrClose( volumeid, fileid, fileserial ) );
    }


    ReleaseFile( pof->Pfpte(), NULL, pof, pffr );
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenById(  _In_    const VolumeId                  volumeid,
                                            _In_    const FileId                    fileid,
                                            _In_    const FileSerial                fileserial,
                                            _In_    const IFileAPI::FileModeFlags   fmf,
                                            _Out_   IFileAPI** const                ppfapi )
{
    ERR                     err                                 = JET_errSuccess;
    const DWORD             cwchAnyAbsPathMax                   = OSFSAPI_MAX_PATH;
    WCHAR                   wszAnyAbsPath[ cwchAnyAbsPathMax ]  = { 0 };
    const DWORD             cwchKeyPathMax                      = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]        = { 0 };
    CFileFilterReference*   pffr                                = NULL;
    VolumeId                volumeidActual                      = volumeidInvalid;
    FileId                  fileidActual                        = fileidInvalid;
    FileSerial              fileserialActual                    = fileserialInvalid;
    const int               cAttemptMax                         = 10;
    int                     cAttempt                            = 0;

    *ppfapi = NULL;


    if ( volumeid == volumeidInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( fileid == fileidInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( fileserial == fileserialInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    while ( volumeidActual != volumeid || fileidActual != fileid )
    {

        if ( ++cAttempt >= cAttemptMax )
        {
            Call( ErrERRCheck( JET_errInternalError ) );
        }


        delete *ppfapi;
        *ppfapi = NULL;


        Call( m_pfident->ErrGetFilePathById( volumeid, fileid, wszAnyAbsPath, wszKeyPath ) );


        Call( ErrFileOpenInternal( wszAnyAbsPath, wszKeyPath, fmf, fTrue, &pffr ) );


        Call( pffr->ErrGetPhysicalId( &volumeidActual, &fileidActual, &fileserialActual ) );
    }


    if ( fileserialActual != fileserial )
    {
        Call( ErrERRCheck( JET_errFileNotFound ) );
    }


    *ppfapi = pffr;
    pffr = NULL;

HandleError:
    delete pffr;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;

        if ( err == JET_errFileNotFound || err == JET_errInvalidPath )
        {
            ReportCachedFileNotFoundById( volumeid, fileid, fileserial );
            err = ErrERRCheck( JET_errFileNotFound );
        }
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileRename(    _In_    IFileAPI* const    pfapi,
                                            _In_z_  const WCHAR* const wszPathDest,
                                            _In_    const BOOL         fOverwriteExisting )
{
    ERR                     err                                     = JET_errSuccess;
    const DWORD             cwchPathSrcMax                          = OSFSAPI_MAX_PATH;
    WCHAR                   wszPathSrc[ cwchPathSrcMax ]            = { 0 };
    const DWORD             cwchAbsPathSrcMax                       = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPathSrc[ cwchAbsPathSrcMax ]      = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileRename" ) );
    CFilePathTableEntry*    pfpteSrc                                = NULL;
    const DWORD             cwchAbsPathDestMax                      = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPathDest[ cwchAbsPathDestMax ]    = { 0 };
    CFilePathTableEntry*    pfpteDest                               = NULL;


    if ( TFileSystemWrapper<I>::FPathIsRelative( wszPathDest ) )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }


    Call( pfapi->ErrPath( wszPathSrc ) );
    Call( ErrLockFiles( wszPathSrc, wszPathDest, wszAbsPathSrc, wszAbsPathDest, &sem, &pfpteSrc, &pfpteDest ) );


    Call( pfpteDest->ErrAccessCheck( fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone, fTrue, fFalse ) );


    Call( pfapi->ErrRename( wszAbsPathDest, fOverwriteExisting ) );


    pfpteSrc->TransferFiles( pfpteDest );

HandleError:
    ReleaseFile( pfpteSrc, &sem );
    ReleaseFile( pfpteDest, &sem );
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileDelete( const WCHAR* const wszPath )
{
    ERR                     err                             = JET_errSuccess;
    const DWORD             cwchAbsPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileDelete" ) );
    CFilePathTableEntry*    pfpte                           = NULL;


    Call( ErrLockFile( wszPath, wszAbsPath, &sem, &pfpte ) );


    Call( ErrPrepareToDelete( pfpte ) );


    Call( TFileSystemWrapper<I>::ErrFileDelete( wszAbsPath ) );


    pfpte->DisconnectFile( pfpte->Pof() );

HandleError:
    ReleaseFile( pfpte, &sem );
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileMove(  const WCHAR* const  wszPathSource,
                                        const WCHAR* const  wszPathDest,
                                        const BOOL          fOverwriteExisting )
{
    ERR                     err                                     = JET_errSuccess;
    const DWORD             cwchAbsPathSrcMax                       = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPathSrc[ cwchAbsPathSrcMax ]      = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileMove" ) );
    CFilePathTableEntry*    pfpteSrc                                = NULL;
    const DWORD             cwchAbsPathDestMax                      = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPathDest[ cwchAbsPathDestMax ]    = { 0 };
    CFilePathTableEntry*    pfpteDest                               = NULL;


    Call( ErrLockFiles( wszPathSource, wszPathDest, wszAbsPathSrc, wszAbsPathDest, &sem, &pfpteSrc, &pfpteDest ) );


    Call( ErrPrepareToDelete( pfpteSrc ) );


    Call( pfpteDest->ErrAccessCheck( fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone, fTrue, fFalse ) );


    Call( TFileSystemWrapper<I>::ErrFileMove( wszAbsPathSrc, wszAbsPathDest, fOverwriteExisting ) );


    pfpteSrc->TransferFiles( pfpteDest );

HandleError:
    ReleaseFile( pfpteSrc, &sem );
    ReleaseFile( pfpteDest, &sem );
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileCopy(  const WCHAR* const  wszPathSource,
                                        const WCHAR* const  wszPathDest,
                                        const BOOL          fOverwriteExisting )
{
    ERR                     err                                     = JET_errSuccess;
    const DWORD             cwchAbsPathSrcMax                       = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPathSrc[ cwchAbsPathSrcMax ]      = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileCopy" ) );
    CFilePathTableEntry*    pfpteSrc                                = NULL;
    const DWORD             cwchAbsPathDestMax                      = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPathDest[ cwchAbsPathDestMax ]    = { 0 };
    CFilePathTableEntry*    pfpteDest                               = NULL;


    Call( ErrLockFiles( wszPathSource, wszPathDest, wszAbsPathSrc, wszAbsPathDest, &sem, &pfpteSrc, &pfpteDest ) );


    Call( pfpteSrc->ErrAccessCheck( IFileAPI::fmfReadOnly, fFalse, fFalse ) );


    Call( pfpteDest->ErrAccessCheck( fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone, fTrue, fFalse ) );


    Call( TFileSystemWrapper<I>::ErrFileCopy( wszAbsPathSrc, wszAbsPathDest, fOverwriteExisting ) );

HandleError:
    ReleaseFile( pfpteSrc, &sem );
    ReleaseFile( pfpteDest, &sem );
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileCreate(    _In_z_ const WCHAR* const               wszPath,
                                            _In_   const IFileAPI::FileModeFlags    fmf,
                                            _Out_  IFileAPI** const                 ppfapi )
{
     
    ERR                     err                             = JET_errSuccess;
    const DWORD             cwchAbsPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileCreate" ) );
    CFilePathTableEntry*    pfpte                           = NULL;
    CFileFilterReference*   pffr                            = NULL;

    *ppfapi = NULL;


    Call( ErrLockFile( wszPath, wszAbsPath, &sem, &pfpte ) );
    

    if ( !pfpte->Pof() )
    {
        Call( ErrFileCreateCacheMiss( wszAbsPath, fmf, pfpte, &pffr ) );
    }


    else
    {

        Call( ErrFileOpenCacheHit( fmf, fTrue, fFalse, pfpte->Pof(), &pffr ) );


        if ( DwCreationDispositionFromFileModeFlags( fTrue, fmf ) == CREATE_ALWAYS )
        {
            TraceContextScope tcScope;
            Call( pfpte->Pof()->Pff()->ErrSetSize( *tcScope, 0, fFalse, qosIONormal ) );
        }
    }


    *ppfapi = pffr;
    pffr = NULL;

HandleError:
    ReleaseFile( pfpte, &sem );
    delete pffr;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpen(  _In_z_ const WCHAR* const               wszPath,
                                        _In_   const IFileAPI::FileModeFlags    fmf,
                                        _Out_  IFileAPI** const                 ppfapi )
{
    ERR                     err                             = JET_errSuccess;
    const DWORD             cwchAbsPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    const DWORD             cwchKeyPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]    = { 0 };
    CFileFilterReference*   pffr                            = NULL;

    *ppfapi = NULL;


    Call( ErrGetFilePath( wszPath, wszAbsPath, wszKeyPath ) );


    Call( ErrFileOpenInternal( wszAbsPath, wszKeyPath, fmf, fFalse, &pffr ) );


    *ppfapi = pffr;
    pffr = NULL;

HandleError:
    delete pffr;
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrWrapFile( _Inout_ IFileAPI** const ppfapiInner, _Out_ IFileAPI** const ppfapi )
{
    ERR         err     = JET_errSuccess;
    IFileAPI*   pfapi   = NULL;

    *ppfapi = NULL;

    if ( *ppfapiInner )
    {
        Alloc( pfapi = new CFileFilter( ppfapiInner,
                                        (IFileSystemFilter*)this,
                                        m_pfsconfig, 
                                        m_pctm,
                                        volumeidInvalid,
                                        fileidInvalid,
                                        NULL,
                                        NULL,
                                        NULL ) );
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

template< class I >
void TFileSystemFilter<I>::ReleaseFile( _In_opt_    CFilePathTableEntry* const              pfpte,
                                        _In_opt_    CSemaphore* const                       psem,
                                        _In_opt_    CFilePathTableEntry::COpenFile* const   pof,
                                        _In_opt_    CFileFilterReference* const             pffr )
{
    BOOL    fDeleteFilePathTableEntry   = fFalse;
    BOOL    fDeleteOpenFile             = fFalse;

    if ( !pfpte )
    {
        return;
    }

    CFilePathHashKey        key( pfpte );
    CFilePathHash::CLock    lock;
    m_filePathHash.WriteLockKey( key, &lock );

    if ( pof )
    {
        if ( fDeleteOpenFile = pof->FRemoveReference( pffr ) )
        {
            pfpte->RemoveFile( pof );
        }
    }

    pfpte->RemoveAsOwnerOrWaiter( psem );

    if ( fDeleteFilePathTableEntry = pfpte->FRelease() )
    {
        CFilePathHash::ERR errFilePathHash = m_filePathHash.ErrDeleteEntry( &lock );
        Assert( errFilePathHash == CFilePathHash::ERR::errSuccess );
    }
    else
    {
        pfpte->AddNextOwner();
    }

    m_filePathHash.WriteUnlockKey( &lock );

    if ( fDeleteOpenFile )
    {
        delete pof;
    }
    if ( fDeleteFilePathTableEntry )
    {
        delete pfpte;
    }
}

template< class I >
ERR TFileSystemFilter<I>::ErrLockFile(  _In_z_                                  const WCHAR* const          wszPath,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const                wszAbsPath,
                                        _In_                                    CSemaphore* const           psem,
                                        _Out_                                   CFilePathTableEntry** const ppfpte )
{
    ERR                     err                             = JET_errSuccess;
    const DWORD             cwchKeyPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]    = { 0 };

    *ppfpte = NULL;

    Call( ErrGetFilePath( wszPath, wszAbsPath, wszKeyPath ) );
    Call( ErrLockFile( wszKeyPath, psem, ppfpte ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        ReleaseFile( *ppfpte, psem );
        *ppfpte = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrLockFiles( _In_z_                                  const WCHAR* const          wszPathSrc, 
                                        _In_z_                                  const WCHAR* const          wszPathDest,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const                wszAbsPathSrc,
                                        _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const                wszAbsPathDest,
                                        _In_                                    CSemaphore* const           psem,
                                        _Out_                                   CFilePathTableEntry** const ppfpteSrc,
                                        _Out_                                   CFilePathTableEntry** const ppfpteDest )
{
    ERR                     err                                     = JET_errSuccess;
    const DWORD             cwchKeyPathSrcMax                       = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPathSrc[ cwchKeyPathSrcMax ]      = { 0 };
    const DWORD             cwchKeyPathDestMax                      = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPathDest[ cwchKeyPathDestMax ]    = { 0 };
    WCHAR*                  pwszKeyPath1                            = wszKeyPathSrc;
    CFilePathTableEntry**   ppfpte1                                 = ppfpteSrc;
    WCHAR*                  pwszKeyPath2                            = wszKeyPathDest;
    CFilePathTableEntry**   ppfpte2                                 = ppfpteDest;

    Call( ErrGetFilePath( wszPathSrc, wszAbsPathSrc, wszKeyPathSrc ) );
    Call( ErrGetFilePath( wszPathDest, wszAbsPathDest, wszKeyPathDest ) );

    if ( LOSStrCompareW( wszKeyPathSrc, wszKeyPathDest ) > 0 )
    {
        pwszKeyPath1 = wszKeyPathDest;
        ppfpte1 = ppfpteDest;
        pwszKeyPath2 = wszKeyPathSrc;
        ppfpte2 = ppfpteSrc;
    }

    Call( ErrLockFile( pwszKeyPath1, psem, ppfpte1 ) );
    Call( ErrLockFile( pwszKeyPath2, psem, ppfpte2 ) );

HandleError:
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrLockFile(  _In_z_  const WCHAR* const          wszKeyPath,
                                        _In_    CSemaphore* const           psem,
                                        _Out_   CFilePathTableEntry** const ppfpte )
{
    ERR                     err             = JET_errSuccess;
    CFilePathHashKey        key( wszKeyPath );
    CFilePathHashEntry      entry;
    CFilePathHash::CLock    lock;
    CFilePathHash::ERR      errFilePathHash = CFilePathHash::ERR::errSuccess;
    BOOL                    fLocked         = fFalse;
    CFilePathTableEntry*    pfpteExisting   = NULL;
    int                     cwchKeyPath     = 0;
    WCHAR*                  wszKeyPathCopy  = NULL;
    CFilePathTableEntry*    pfpteNew        = NULL;
    BOOL                    fRemove         = fFalse;

    *ppfpte = NULL;

    Call( ErrEnsureInitFilePathTable() );

    m_filePathHash.WriteLockKey( key, &lock );
    fLocked = fTrue;

    errFilePathHash = m_filePathHash.ErrRetrieveEntry( &lock, &entry );
    if ( errFilePathHash == CFilePathHash::ERR::errSuccess )
    {
        pfpteExisting = entry.Pfpte();
    }
    else
    {
        Assert( errFilePathHash == CFilePathHash::ERR::errEntryNotFound );
        cwchKeyPath = LOSStrLengthW( wszKeyPath ) + 1;
        Alloc( wszKeyPathCopy = new WCHAR[ cwchKeyPath ] );
        Call( ErrOSStrCbCopyW( wszKeyPathCopy, cwchKeyPath * sizeof( WCHAR ), wszKeyPath ) );

        Alloc( pfpteExisting = pfpteNew = new CFilePathTableEntry( const_cast<const WCHAR** const>( &wszKeyPathCopy ) ) );

        entry = CFilePathHashEntry( pfpteNew );
        errFilePathHash = m_filePathHash.ErrInsertEntry( &lock, entry );
        if ( errFilePathHash == CFilePathHash::ERR::errOutOfMemory )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( errFilePathHash == CFilePathHash::ERR::errSuccess );
        fRemove = fTrue;
    }

    Call( pfpteExisting->ErrAddAsOwnerOrWaiter( psem ) );

    pfpteExisting->AddRef();
    fRemove = fFalse;

    m_filePathHash.WriteUnlockKey( &lock );
    fLocked = fFalse;

    *ppfpte = pfpteExisting;
    psem->Wait();

HandleError:
    if ( fRemove )
    {
        errFilePathHash = m_filePathHash.ErrDeleteEntry( &lock );
        Assert( errFilePathHash == CFilePathHash::ERR::errSuccess );
        delete pfpteNew;
    }
    if ( fLocked )
    {
        m_filePathHash.WriteUnlockKey( &lock );
    }
    delete[] wszKeyPathCopy;
    if ( err < JET_errSuccess )
    {
        *ppfpte = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrGetFilePath(   _In_z_                                  const WCHAR* const  wszPath,
                                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszAbsPath,
                                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )  WCHAR* const        wszKeyPath )
{
    ERR err = JET_errSuccess;

    wszAbsPath[ 0 ] = 0;
    wszKeyPath[ 0 ] = 0;


    Call( TFileSystemWrapper<I>::ErrPathComplete( wszPath, wszAbsPath ) );


    Call( m_pfident->ErrGetFileKeyPath( wszAbsPath, wszKeyPath ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        wszAbsPath[ 0 ] = 0;
        wszKeyPath[ 0 ] = 0;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrPrepareToDelete( _In_ CFilePathTableEntry* const pfpte )
{
    ERR         err         = JET_errSuccess;
    VolumeId    volumeid    = volumeidInvalid;
    FileId      fileid      = fileidInvalid;
    FileSerial  fileserial  = fileserialInvalid;


    if ( pfpte->Pof() && pfpte->Pof()->Pc() )
    {
        Call( pfpte->Pof()->Pff()->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );
        Call( pfpte->Pof()->Pc()->ErrClose( volumeid, fileid, fileserial ) );
    }


    Call( pfpte->ErrDeleteCheck() );

HandleError:
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileCreateCacheMiss(   _In_z_  const WCHAR* const              wszAnyAbsPath,
                                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                                    _In_    CFilePathTableEntry* const      pfpte,
                                                    _Out_   CFileFilterReference** const    ppffr )
{
    ERR                             err         = JET_errSuccess;
    CFileFilter*                    pff         = NULL;
    BOOL                            fCreated    = fFalse;
    CFilePathTableEntry::COpenFile* pof         = NULL;
    ICache*                         pc          = NULL;

    *ppffr = NULL;


    Call( TFileSystemWrapper<I>::ErrFileCreate( wszAnyAbsPath, fmf, (IFileAPI**)&pff ) );
    fCreated = fTrue;


    Call( ErrFileConfigure( pff, wszAnyAbsPath ) );


    Call( ErrGetConfiguredCache( pff, pfpte->WszKeyPath(), &pc ) );


    Call( pfpte->ErrOpenFile( &pff, pc, &pof ) );


    Alloc( *ppffr = new CFileFilterReference( this, pof, fmf, fFalse ) );
    pof = NULL;

HandleError:
    delete pof;
    delete pff;
    if ( err < JET_errSuccess )
    {
        delete *ppffr;
        *ppffr = NULL;
        if ( fCreated )
        {
            CallS( TFileSystemWrapper<I>::ErrFileDelete( wszAnyAbsPath ) );
        }
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileConfigure( _In_        CFileFilter* const          pff,
                                            _In_z_      const WCHAR* const          wszAnyAbsPath )
{
    ERR         err         = JET_errSuccess;
    VolumeId    volumeid    = volumeidInvalid;
    FileId      fileid      = fileidInvalid;

    Call( m_pfident->ErrGetFileId( wszAnyAbsPath, &volumeid, &fileid ) );
    pff->SetFileId( volumeid, fileid );

HandleError:
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenInternal(  _In_z_  const WCHAR* const              wszAnyAbsPath,
                                                _In_z_  const WCHAR* const              wszKeyPath,
                                                _In_    const IFileAPI::FileModeFlags   fmf,
                                                _In_    const BOOL                      fCacheOpen,
                                                _Out_   CFileFilterReference** const    ppffr )
{
    ERR                     err         = JET_errSuccess;
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileOpenInternal" ) );
    CFilePathTableEntry*    pfpte       = NULL;

    *ppffr = NULL;


    Call( ErrLockFile( wszKeyPath, &sem, &pfpte ) );


    if ( !pfpte->Pof() )
    {
        Call( ErrFileOpenCacheMiss( wszAnyAbsPath, fmf, fCacheOpen, pfpte, ppffr ) );
    }


    else
    {

        Call( ErrFileOpenCacheHit( fmf, fFalse, fCacheOpen, pfpte->Pof(), ppffr ) );


        if ( DwCreationDispositionFromFileModeFlags( fFalse, fmf ) == TRUNCATE_EXISTING )
        {
            TraceContextScope tcScope;
            Call( pfpte->Pof()->Pff()->ErrSetSize( *tcScope, 0, fFalse, qosIONormal ) );
        }
    }

HandleError:
    ReleaseFile( pfpte, &sem );
    if ( err < JET_errSuccess )
    {
        delete *ppffr;
        *ppffr = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenCacheMiss( _In_z_  const WCHAR* const              wszAnyAbsPath,
                                                _In_    const IFileAPI::FileModeFlags   fmf,
                                                _In_    const BOOL                      fCacheOpen,
                                                _In_    CFilePathTableEntry* const      pfpte,
                                                _Out_   CFileFilterReference** const    ppffr )
{
    ERR                             err             = JET_errSuccess;
    CFileFilter*                    pff             = NULL;
    ICache*                         pc              = NULL;
    BOOL                            fAttached       = fFalse;
    const IFileAPI::FileModeFlags   fmfReadOnlyMask = IFileAPI::fmfReadOnly | IFileAPI::fmfReadOnlyClient | IFileAPI::fmfReadOnlyPermissive;
    CFilePathTableEntry::COpenFile* pof             = NULL;

    *ppffr = NULL;


    Call( ErrFileOpenAndConfigure( wszAnyAbsPath, fmf, pfpte, &pff, &pc, &fAttached ) );


    if ( fAttached && (fmf & fmfReadOnlyMask) != 0 )
    {
        delete pff;
        pff = NULL;

        Call( ErrFileOpenAndConfigure( wszAnyAbsPath, fmf & ~fmfReadOnlyMask, pfpte, &pff, &pc, &fAttached ))
    }


    Call( pfpte->ErrOpenFile( &pff, pc, &pof ) );


    Alloc( *ppffr = new CFileFilterReference( this, pof, fmf, fCacheOpen ) );
    pof = NULL;

HandleError:
    delete pof;
    delete pff;
    if ( err < JET_errSuccess )
    {
        delete *ppffr;
        *ppffr = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenAndConfigure(  _In_z_  const WCHAR* const              wszAnyAbsPath,
                                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                                    _In_    CFilePathTableEntry* const      pfpte,
                                                    _Out_   CFileFilter** const             ppff,
                                                    _Out_   ICache** const                  ppc,
                                                    _Out_   BOOL* const                     pfAttached )
{
    ERR             err         = JET_errSuccess;
    CFileFilter*    pff         = NULL;
    ICache*         pc          = NULL;
    BOOL            fAttached   = fFalse;

    *ppff = NULL;
    *ppc = NULL;
    *pfAttached = fFalse;


    Call( TFileSystemWrapper<I>::ErrFileOpen( wszAnyAbsPath, fmf, (IFileAPI**)&pff ) );


    Call( ErrFileConfigure( pff, wszAnyAbsPath ) );


    Call( ErrGetCache( pff, pfpte->WszKeyPath(), &pc, &fAttached ) );


    *ppff = pff;
    pff = NULL;
    *ppc = pc;
    *pfAttached = fAttached;

HandleError:
    delete pff;
    if ( err < JET_errSuccess )
    {
        delete *ppff;
        *ppff = NULL;
        *ppc = NULL;
        *pfAttached = fFalse;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenCacheHit(  _In_    const IFileAPI::FileModeFlags           fmf,
                                                _In_    const BOOL                              fCreate,
                                                _In_    const BOOL                              fCacheOpen,
                                                _In_    CFilePathTableEntry::COpenFile* const   pof,
                                                _Out_   CFileFilterReference** const            ppffr )
{
    ERR err = JET_errSuccess;

    *ppffr = NULL;


    Call( pof->Pfpte()->ErrAccessCheck( fmf, fCreate, fCacheOpen ) );


    Alloc( *ppffr = new CFileFilterReference( this, pof, fmf, fCacheOpen ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete *ppffr;
        *ppffr = NULL;
    }
    return err;
}

template<class I>
void TFileSystemFilter<I>::ReportCachedFileNotFoundById(    _In_ const VolumeId     volumeid,
                                                            _In_ const FileId       fileid,
                                                            _In_ const FileSerial   fileserial )
{
    const ULONG     cwsz                = 3;
    const WCHAR*    rgpwsz[ cwsz ]      = { 0 };
    DWORD           irgpwsz             = 0;
    WCHAR           wszVolumeId[ 64 ]   = { 0 };
    WCHAR           wszFileId[ 64 ]     = { 0 };
    WCHAR           wszFileSerial[ 64 ] = { 0 };

    OSStrCbFormatW( wszVolumeId, sizeof( wszVolumeId ), L"0x%08x", volumeid );
    OSStrCbFormatW( wszFileId, sizeof( wszFileId ), L"0x%016I64x", fileid );
    OSStrCbFormatW( wszFileSerial, sizeof( wszFileSerial ), L"0x%08x", fileserial );

    rgpwsz[ irgpwsz++ ] = wszVolumeId;
    rgpwsz[ irgpwsz++ ] = wszFileId;
    rgpwsz[ irgpwsz++ ] = wszFileSerial;

    m_pfsconfig->EmitEvent( eventError,
                            BLOCK_CACHE_CATEGORY,
                            BLOCK_CACHE_CACHED_FILE_ID_MISMATCH_ID,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );
}

template< class I >
ERR TFileSystemFilter<I>::ErrGetConfiguredCache(    _In_    CFileFilter* const  pff,
                                                    _In_z_  const WCHAR* const  wszKeyPath,
                                                    _Out_   ICache** const      ppc )
{
    ERR                             err                                                 = JET_errSuccess;
    IBlockCacheConfiguration*       pbcconfig                                           = NULL;
    ICachedFileConfiguration*       pcfconfig                                           = NULL;
    const DWORD                     cwchAbsPathCachingFileMax                           = OSFSAPI_MAX_PATH;
    WCHAR                           wszAbsPathCachingFile[ cwchAbsPathCachingFileMax ]  = { 0 };
    const DWORD                     cwchKeyPathCachingFileMax                           = OSFSAPI_MAX_PATH;
    WCHAR                           wszKeyPathCachingFile[ cwchKeyPathCachingFileMax ]  = { 0 };
    ICacheConfiguration*            pcconfig                                            = NULL;
    ICache*                         pc                                                  = NULL;

    *ppc = NULL;


    Call( ErrGetConfiguration( &pbcconfig ) );
    Call( pbcconfig->ErrGetCachedFileConfiguration( wszKeyPath, &pcfconfig ) );


    if ( pcfconfig->FCachingEnabled() )
    {

        pcfconfig->CachingFilePath( wszAbsPathCachingFile );
        Call( m_pfident->ErrGetFileKeyPath( wszAbsPathCachingFile, wszKeyPathCachingFile ) );
        Call( pbcconfig->ErrGetCacheConfiguration( wszKeyPathCachingFile, &pcconfig ) );


        err = m_pcrep->ErrOpen( this, m_pfsconfig, &pcconfig, &pc );
        if ( err < JET_errSuccess )
        {
            ReportCacheOpenFailure( pcfconfig, err );
            err = JET_errSuccess;
        }
    }


    *ppc = pc;


    pff->SetCacheState( &pcfconfig, &pc, NULL );

HandleError:
    delete pc;
    delete pcconfig;
    delete pcfconfig;
    if ( err < JET_errSuccess )
    {
        *ppc = NULL;
    }
    return err;
}

template<class I>
void TFileSystemFilter<I>::ReportCacheOpenFailure( _In_ ICachedFileConfiguration* const pcfconfig, _In_ const ERR err )
{
    const ULONG     cwsz                                = 2;
    const WCHAR*    rgpwsz[ cwsz ]                      = { 0 };
    DWORD           irgpwsz                             = 0;
    WCHAR           wszCachingFile[ OSFSAPI_MAX_PATH ]  = { 0 };
    WCHAR           wszError[ 64 ]                      = { 0 };

    pcfconfig->CachingFilePath( wszCachingFile );
    OSStrCbFormatW( wszError, sizeof( wszError ), L"%i (0x%08x)", err, err );

    rgpwsz[ irgpwsz++ ] = wszCachingFile;
    rgpwsz[ irgpwsz++ ] = wszError;

    m_pfsconfig->EmitEvent( eventWarning,
                            BLOCK_CACHE_CATEGORY,
                            BLOCK_CACHE_CACHING_FILE_OPEN_FAILURE_ID,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );
}

template< class I >
ERR TFileSystemFilter<I>::ErrGetCache(  _In_    CFileFilter* const  pff,
                                        _In_z_  const WCHAR* const  wszKeyPath,
                                        _Out_   ICache** const      ppc,
                                        _Out_   BOOL* const         pfAttached )
{
    ERR                         err         = JET_errSuccess;
    CCachedFileHeader*          pcfh        = NULL;
    IBlockCacheConfiguration*   pbcconfig   = NULL;
    ICachedFileConfiguration*   pcfconfig   = NULL;
    ICache*                     pc          = NULL;

    *ppc = NULL;
    *pfAttached = fFalse;


    err = CCachedFileHeader::ErrLoad( m_pfsconfig, pff, &pcfh );


    if ( err >= JET_errSuccess )
    {
        Call( ErrGetConfiguration( &pbcconfig ) );
        Call( pbcconfig->ErrGetCachedFileConfiguration( wszKeyPath, &pcfconfig ) );
        Call( m_pcrep->ErrOpenById( this,
                                    m_pfsconfig,
                                    pbcconfig,
                                    pcfh->VolumeidCache(),
                                    pcfh->FileidCache(),
                                    pcfh->RgbUniqueIdCache(),
                                    &pc ) );

        *ppc = pc;
        *pfAttached = fTrue;

        pff->SetCacheState( &pcfconfig, &pc, &pcfh );
    }


    else if ( err == JET_errReadVerifyFailure )
    {

        err = JET_errSuccess;


        Call( ErrGetConfiguredCache( pff, wszKeyPath, ppc ) );
    }

HandleError:
    delete pc;
    delete pcfconfig;
    delete pcfh;
    if ( err < JET_errSuccess )
    {
        *ppc = NULL;
        *pfAttached = fFalse;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrGetConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig )
{
    ERR err = JET_errSuccess;

    *ppbcconfig = NULL;

    Call( m_initOnceAttach.Init( ErrGetConfiguration_, this ) );

    *ppbcconfig = m_pbcconfig;

HandleError:
    if ( err < JET_errSuccess )
    {
        *ppbcconfig = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrGetConfiguration()
{
    return m_pfsconfig->ErrGetBlockCacheConfiguration( &m_pbcconfig );
}

template< class I >
ERR TFileSystemFilter<I>::ErrInitFilePathTable()
{
    ERR err = JET_errSuccess;

    if ( m_filePathHash.ErrInit( 5.0, 1.0 ) == CFilePathHash::ERR::errOutOfMemory )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

template< class I >
void TFileSystemFilter<I>::TermFilePathTable()
{
    if ( m_initOnceFilePathTable.FIsInit() )
    {
        m_filePathHash.Term();
    }
}

INLINE typename CFilePathHash::NativeCounter CFilePathHash::CKeyEntry::Hash( const CFilePathHashKey& key )
{
    return CFilePathHash::NativeCounter( key.UiHash() );
}

INLINE typename CFilePathHash::NativeCounter CFilePathHash::CKeyEntry::Hash() const
{
    return CFilePathHash::NativeCounter( m_entry.UiHash() );
}

INLINE BOOL CFilePathHash::CKeyEntry::FEntryMatchesKey( const CFilePathHashKey& key ) const
{
    if ( m_entry.UiHash() != key.UiHash() )
    {
        return fFalse;
    }

    if ( LOSStrCompareW( m_entry.Pfpte()->WszKeyPath(), key.WszKeyPath() ) != 0 )
    {
        return fFalse;
    }

    return fTrue;
}

INLINE void CFilePathHash::CKeyEntry::SetEntry( const CFilePathHashEntry& entry )
{
    m_entry = entry;
}

INLINE void CFilePathHash::CKeyEntry::GetEntry( CFilePathHashEntry * const pentry ) const
{
    *pentry = m_entry;
}


class CFileSystemFilter : public TFileSystemFilter<IFileSystemFilter>
{
    public:

        CFileSystemFilter(  _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ IFileSystemAPI** const          ppfsapi,
                            _In_    IFileIdentification* const      pfident,
                            _In_    ICacheTelemetry* const          pctm,
                            _In_    ICacheRepository * const        pcrep )
            :   TFileSystemFilter<IFileSystemFilter>( pfsconfig, ppfsapi, pfident, pctm, pcrep )
        {
        }

        virtual ~CFileSystemFilter() {}
};


class CFileFilterReference : public CFileFilterWrapper
{
    public:

        CFileFilterReference(   _In_ TFileSystemFilter<IFileSystemFilter>* const    pfsf,
                                _In_ CFilePathTableEntry::COpenFile* const          pof,
                                _In_ const IFileAPI::FileModeFlags                  fmf,
                                _In_ const BOOL                                     fCacheOpen )
            :   CFileFilterWrapper( pof->Pff(), fCacheOpen ? IOMode::iomRaw : IOMode::iomEngine ),
                m_pfsf( pfsf ),
                m_pof( pof ),
                m_fmf( fmf ),
                m_fCacheOpen( fCacheOpen )
        {
            m_pfsf->AddFileReference( this, m_pof );
        }

        virtual ~CFileFilterReference()
        {
            m_pfsf->RemoveFileReference( this, m_pof );
        }

        BOOL FCacheOpen() const { return m_fCacheOpen; }

    public:

        FileModeFlags Fmf() const override { return m_fmf; }

    private:

        friend SIZE_T CFileFilterReferenceOffsetOfILE();

        TFileSystemFilter<IFileSystemFilter>* const                                         m_pfsf;
        CFilePathTableEntry::COpenFile* const                                               m_pof;
        const IFileAPI::FileModeFlags                                                       m_fmf;
        const BOOL                                                                          m_fCacheOpen;
        CInvasiveList< CFileFilterReference, CFileFilterReferenceOffsetOfILE >::CElement    m_ile;
};

static SIZE_T CFileFilterReferenceOffsetOfILE() { return OffsetOf( CFileFilterReference, m_ile ); }

INLINE ERR CFilePathTableEntry::COpenFile::ErrAccessCheck(  _In_ const IFileAPI::FileModeFlags  fmf,
                                                            _In_ const BOOL                     fCreate,
                                                            _In_ const BOOL                     fCacheOpen )
{
    ERR err = JET_errSuccess;


    for (   CFileFilterReference* ffr = m_ilReferences.PrevMost();
            ffr != NULL && err >= JET_errSuccess;
            ffr = m_ilReferences.Next( ffr ) )
    {
        
        if ( ffr->FCacheOpen() )
        {
            continue;
        }


        if ( fCacheOpen )
        {
            continue;
        }


        DWORD dwDesiredAccess = DwDesiredAccessFromFileModeFlags( fmf );


        DWORD dwShareMode = DwShareModeFromFileModeFlags( ffr->Fmf() );


        if ( ( dwDesiredAccess & GENERIC_READ ) && !( dwShareMode & FILE_SHARE_READ ) )
        {
            err = ErrERRCheck( JET_errFileAccessDenied );
        }

        if ( ( dwDesiredAccess & GENERIC_WRITE ) && !( dwShareMode & FILE_SHARE_WRITE ) )
        {
            err = ErrERRCheck( JET_errFileAccessDenied );
        }

        if ( ( dwDesiredAccess & DELETE ) && !( dwShareMode & FILE_SHARE_DELETE ) )
        {
            err = ErrERRCheck( JET_errFileAccessDenied );
        }


        DWORD dwCreationDisposition = DwCreationDispositionFromFileModeFlags( fCreate, fmf );


        if ( dwCreationDisposition == CREATE_NEW )
        {
            err = ErrERRCheck( JET_errFileAlreadyExists );
        }
    }

    return err;
}

INLINE ERR CFilePathTableEntry::COpenFile::ErrDeleteCheck()
{
    ERR err = JET_errSuccess;


    for (   CFileFilterReference* ffr = m_ilReferences.PrevMost();
            ffr != NULL && err >= JET_errSuccess;
            ffr = m_ilReferences.Next( ffr ) )
    {

        DWORD dwShareMode = DwShareModeFromFileModeFlags( ffr->Fmf() );


        if ( !( dwShareMode & FILE_SHARE_DELETE ) )
        {
            err = ErrERRCheck( JET_errFileAccessDenied );
        }
    }

    return err;
}
