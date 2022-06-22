// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

class CFileFilter;
class CFileFilterReference;
static SIZE_T CFileFilterReferenceOffsetOfILE();

//  A file path table entry.

class CFilePathTableEntry  //  fpte
{
    public:

        class COpenFile;

    public:

        CFilePathTableEntry( _Inout_z_ const WCHAR** const pwszKeyPath );

        ~CFilePathTableEntry();

        const WCHAR* WszKeyPath() const { return m_wszKeyPath; }
        COpenFile* Pof() const { return m_pof;  }
        UINT UiHash() const { return CFilePathTableEntry::UiHash( m_wszKeyPath ); }

        void AddRef( _In_ int dref = 1 ) { AtomicExchangeAdd( (LONG*)&m_cref, dref ); }
        BOOL FRelease( _In_ int dref = 1 ) { return AtomicExchangeAdd( (LONG*)&m_cref, -dref ) == dref; }

        BOOL FOwned() const { return m_psemOwner != NULL; }
        ERR ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem );
        void AddNextOwner();
        void RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem );

        static UINT UiHash( _In_z_ const WCHAR* const wszKeyPath );

        ERR ErrOpenFile( _Inout_ CFileFilter** const ppff, _Out_ COpenFile** const ppof );
        void DisconnectFile( _In_ COpenFile* const pof );
        void TransferFiles( _In_ CFilePathTableEntry* const pfpteDest );

        ERR ErrAccessCheck( _In_ const IFileAPI::FileModeFlags  fmf,
                            _In_ const BOOL                     fCreate,
                            _In_ const BOOL                     fCacheOpen );
        ERR ErrDeleteCheck();

    public:

        //  Context for an open file.

        class COpenFile
        {
            public:

                COpenFile(  _In_    CFilePathTableEntry* const  pfpte,
                            _Inout_ CFileFilter** const         ppff )
                    :   m_pfpte( pfpte ),
                        m_pff( *ppff ),
                        m_critReferences( CLockBasicInfo( CSyncBasicInfo( "CFilePathTableEntry::COpenFile::m_critReferences" ), rankFileFilterReferences, 0 ) )
                {
                    *ppff = NULL;
                }

                ~COpenFile()
                {
                    delete m_pff;
                }

                CFilePathTableEntry* Pfpte() const { return m_pfpte; }
                CFileFilter* Pff() const { return m_pff; }
                ICache* Pc() const { return Pff()->Pc(); }
                int CReference() const { return m_ilReferences.Count();  }

                void AddReference( _In_ CFileFilterReference* const pffr )
                {
                    m_critReferences.Enter();
                    m_ilReferences.InsertAsNextMost( pffr );
                    m_critReferences.Leave();
                }

                void RemoveReference( _In_ CFileFilterReference* const pffr )
                {
                    m_critReferences.Enter();
                    m_ilReferences.Remove( pffr );
                    m_critReferences.Leave();
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
                CCriticalSection                                                            m_critReferences;
                CCountedInvasiveList<CFileFilterReference, CFileFilterReferenceOffsetOfILE> m_ilReferences;
        };

    private:

        //  Wait context for cache miss collisions on the file path table.

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
        volatile int                                        m_cref;
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
    while ( m_ilOpenFiles.PrevMost() )
    {
        COpenFile* pof = m_ilOpenFiles.PrevMost();
        m_ilOpenFiles.Remove( pof );
        delete pof;
    }
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
                                                _Out_   COpenFile** const   ppof )
{
    ERR         err = JET_errSuccess;
    COpenFile*  pof = NULL;

    *ppof = NULL;

    Alloc( pof = new COpenFile( this, ppff ) );

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

//  File path hash table key.

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

//  File path hash table entry.

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

//  File path hash table.

typedef CDynamicHashTable<CFilePathHashKey, CFilePathHashEntry> CFilePathHash;

//  TFileSystemFilter:  core implementation of IFileSystemFilter and its derivatives.
//
//  This class provides the core of the file system filter used to implement the block cache.  It is responsible for
//  intercepting relevant IFileSystemAPI calls, detecting files that are or should be cached, and managing the caches
//  for these files.  This class also provides the ability for the cache to open cached files for write back such that
//  the engine can successfully access files without being aware of the cache.

template< class I >
class TFileSystemFilter  //  fsf
    :   public TFileSystemWrapper<I>
{
    public:  //  specialized API

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

        typedef void (*PfnDetachFileStatus)(    _In_        const int       i,
                                                _In_        const int       c,
                                                _In_opt_    const DWORD_PTR keyDetachFileStatus );

        ERR ErrFileDetach(  _In_z_      const WCHAR* const                              wszFilePath,
                            _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                            _In_opt_    const DWORD_PTR                                 keyDetachFileStatus );

    public:  //  IFileSystemFilter

        //  configuration

        ERR ErrFileOpenById(    _In_    const VolumeId                  volumeid,
                                _In_    const FileId                    fileid,
                                _In_    const FileSerial                fileserial,
                                _In_    const IFileAPI::FileModeFlags   fmf,
                                _Out_   IFileAPI** const                ppfapi ) override;

        ERR ErrFileRename(  _In_    IFileAPI* const    pfapi,
                            _In_z_  const WCHAR* const wszPathDest,
                            _In_    const BOOL         fOverwriteExisting ) override;

    public:  //  IFileSystemAPI

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

    public:  //  CFileSystemWrapper

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
                            _In_    const BOOL                  fCacheOpen,
                            _Out_   CFilePathTableEntry** const ppfpte );
        ERR ErrGetFilePath( _In_z_                                                  const WCHAR* const  wszPath,
                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )                  WCHAR* const        wszAbsPath,
                            _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const        wszKeyPath );

        ERR ErrPrepareToDelete( _In_ CFilePathTableEntry* const pfpte );

        ERR ErrFileCreateCacheMiss( _In_z_  const WCHAR* const              wszAnyAbsPath,
                                    _In_    const IFileAPI::FileModeFlags   fmf,
                                    _In_    CFilePathTableEntry* const      pfpte,
                                    _Out_   CFileFilterReference** const    ppffr );
        ERR ErrTryMarkAsEverEligibleForCaching( _In_z_  const WCHAR* const  wszAnyAbsPath,
                                                _In_    const BOOL          fOverwriteExisting,
                                                _In_    const BOOL          fOpenExisting,
                                                _Out_   BOOL* const         pfMarked );

        ERR ErrFileConfigure(   _In_        CFileFilter* const                  pff,
                                _Inout_     ICachedFileConfiguration** const    ppcfconfig,
                                _In_        const BOOL                          fEverEligibleForCaching,
                                _In_z_      const WCHAR* const                  wszAnyAbsPath );

        ERR ErrFileOpenInternal(    _In_z_      const WCHAR* const                              wszPath,
                                    _In_        const IFileAPI::FileModeFlags                   fmf,
                                    _In_        const BOOL                                      fDetachFile,
                                    _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                    _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                    _Out_       IFileAPI** const                                ppfapi );
        ERR ErrFileOpenInternal(    _In_z_      const WCHAR* const                              wszAnyAbsPath,
                                    _In_z_      const WCHAR* const                              wszKeyPath,
                                    _In_        const IFileAPI::FileModeFlags                   fmf,
                                    _In_        const BOOL                                      fCacheOpen,
                                    _In_        const BOOL                                      fDetachFile,
                                    _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                    _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                    _Out_       CFileFilterReference** const                    ppffr );
        ERR ErrFileOpenCacheMiss(   _In_z_      const WCHAR* const                              wszAnyAbsPath,
                                    _In_        const IFileAPI::FileModeFlags                   fmf,
                                    _In_        const BOOL                                      fCacheOpen,
                                    _In_        CFilePathTableEntry* const                      pfpte,
                                    _In_        const BOOL                                      fDetachFile,
                                    _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                    _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                    _Out_       CFileFilterReference** const                    ppffr );
        ERR ErrFileOpenAndConfigure(    _In_z_      const WCHAR* const                              wszAnyAbsPath,
                                        _In_        const IFileAPI::FileModeFlags                   fmf,
                                        _In_        const BOOL                                      fCacheOpen,
                                        _In_        CFilePathTableEntry* const                      pfpte,
                                        _In_        const BOOL                                      fDetachFile,
                                        _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                        _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                        _Out_       CFileFilter** const                             ppff );
        ERR ErrEverEligibleForCaching(  _In_z_  const WCHAR* const              wszAnyAbsPath,
                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                        _Out_   BOOL* const                     pfEverEligible );
        ERR ErrFileOpenCacheHit(    _In_    const IFileAPI::FileModeFlags           fmf,
                                    _In_    const BOOL                              fCreate,
                                    _In_    const BOOL                              fCacheOpen,
                                    _In_    CFilePathTableEntry::COpenFile* const   pof,
                                    _Out_   CFileFilterReference** const            ppffr );
                
        void ReportCachedFileNotFoundById(  _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial );

        ERR ErrCacheOpenFailure(    _In_ CFileFilter* const pff,
                                    _In_ const char* const  szFunction, 
                                    _In_ const ERR          errFromCall,
                                    _In_ const ERR          errToReturn );
        ERR ErrGetCache(    _In_        CFileFilter* const                              pff,
                            _In_        const BOOL                                      fEverEligibleForCaching,
                            _In_        const BOOL                                      fCacheOpen,
                            _In_z_      const WCHAR* const                              wszKeyPath,
                            _In_        const BOOL                                      fDetachFile,
                            _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                            _In_opt_    const DWORD_PTR                                 keyDetachFileStatus );
        BOOL FDetachFile(   _In_ CFileFilter* const                 pff,
                            _In_ IBlockCacheConfiguration* const    pbcconfig,
                            _In_ const BOOL                         fCacheOpen );
        ERR ErrDetachFile(  _In_        CFileFilter* const                              pff,
                            _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                            _In_opt_    const DWORD_PTR                                 keyDetachFileStatus );

        ERR ErrGetConfiguration( _Out_ IBlockCacheConfiguration** const ppbcconfig );
        ERR ErrGetConfiguration();
        static ERR ErrGetConfiguration_( _In_ TFileSystemFilter<I>* const pfsf ) { return pfsf->ErrGetConfiguration(); }

        ERR ErrEnsureInitFilePathTable() { return m_initOnceFilePathTable.Init( ErrInitFilePathTable_, this ); };
        ERR ErrInitFilePathTable();
        static ERR ErrInitFilePathTable_( _In_ TFileSystemFilter<I>* const pfsf ) { return pfsf->ErrInitFilePathTable(); }
        void TermFilePathTable();

    private:

        class CDestagingFile : public CFileFilterWrapper
        {
            public:

                CDestagingFile( _In_ CFileFilter* const pff )
                    :   CFileFilterWrapper( pff, iomEngine ),
                        m_volumeid( pff->Pcfh()->Volumeid() ),
                        m_fileid( pff->Pcfh()->Fileid() ),
                        m_fileserial( pff->Pcfh()->Fileserial() )
                {
                }

                VolumeId Volumeid() const { return m_volumeid; }
                FileId Fileid() const { return m_fileid; }
                FileSerial Fileserial() const { return m_fileserial; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CDestagingFile, m_ile ); }

            private:

                const VolumeId                                                                  m_volumeid;
                const FileId                                                                    m_fileid;
                const FileSerial                                                                m_fileserial;
                typename CInvasiveList<CDestagingFile, CDestagingFile::OffsetOfILE>::CElement   m_ile;
        };

    private:

        const IFileAPI::FileModeFlags fmfReadOnlyMask = IFileAPI::fmfReadOnly | IFileAPI::fmfReadOnlyClient | IFileAPI::fmfReadOnlyPermissive;

    private:

        static const WCHAR                                                                  c_wszFileEverEligibleForCaching[];

        IFileSystemConfiguration* const                                                     m_pfsconfig;
        IFileIdentification* const                                                          m_pfident;
        ICacheTelemetry* const                                                              m_pctm;
        ICacheRepository* const                                                             m_pcrep;
        CInitOnce< ERR, decltype( &ErrInitFilePathTable_ ), TFileSystemFilter<I>* const >   m_initOnceFilePathTable;
        CFilePathHash                                                                       m_filePathHash;
        IBlockCacheConfiguration*                                                           m_pbcconfig;
        CInitOnce< ERR, decltype( &ErrGetConfiguration_ ), TFileSystemFilter<I>* const >    m_initOnceAttach;
        CReaderWriterLock                                                                   m_rwlDestagingFiles;
        CInvasiveList<CDestagingFile, CDestagingFile::OffsetOfILE>                          m_ilDestagingFiles;
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
        m_filePathHash( rankFilePathHash ),
        m_pbcconfig( NULL ),
        m_rwlDestagingFiles( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_rwlRegisterIFilePerfAPI" ), rankDestagingFiles, 0 ) )
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
    //  if this reference is not from the cache and the file is cached then ask the cache to close any references it
    //  may have to this file.  this will break any circular link that keeps the cache alive.  if the cache still needs
    //  to do write back to this file then it will open it again

    if ( !pffr->FCacheOpen() && pof->Pc() )
    {
        VolumeId    volumeid    = volumeidInvalid;
        FileId      fileid      = fileidInvalid;
        FileSerial  fileserial  = fileserialInvalid;

        CallS( pof->Pff()->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );
        CallS( pof->Pc()->ErrClose( volumeid, fileid, fileserial ) );
    }

    //  release the reference on this open file and file path table entry

    ReleaseFile( pof->Pfpte(), NULL, pof, pffr );
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileDetach(    _In_z_      const WCHAR* const                              wszFilePath,
                                            _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                            _In_opt_    const DWORD_PTR                                 keyDetachFileStatus )
{
    ERR         err     = JET_errSuccess;
    IFileAPI*   pfapi   = NULL;

    Call( ErrFileOpenInternal( wszFilePath, IFileAPI::fmfNone, fTrue, pfnDetachFileStatus, keyDetachFileStatus, &pfapi ) );

HandleError:
    delete pfapi;
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenById(  _In_    const VolumeId                  volumeid,
                                            _In_    const FileId                    fileid,
                                            _In_    const FileSerial                fileserial,
                                            _In_    const IFileAPI::FileModeFlags   fmf,
                                            _Out_   IFileAPI** const                ppfapi )
{
    ERR                     err                                 = JET_errSuccess;
    BOOL                    fLeave                              = fFalse;
    const DWORD             cwchAnyAbsPathMax                   = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAnyAbsPath[ cwchAnyAbsPathMax ]  = { 0 };
    const DWORD             cwchKeyPathMax                      = IFileIdentification::cwchKeyPathMax;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]        = { 0 };
    CFileFilterReference*   pffr                                = NULL;
    VolumeId                volumeidActual                      = volumeidInvalid;
    FileId                  fileidActual                        = fileidInvalid;
    FileSerial              fileserialActual                    = fileserialInvalid;
    const int               cAttemptMax                         = 10;
    int                     cAttempt                            = 0;

    *ppfapi = NULL;

    //  defend against an invalid physical id

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

    //  if we are destaging this file then do the special destaging open

    m_rwlDestagingFiles.EnterAsReader();
    fLeave = fTrue;

    for (   CDestagingFile* pdestagingfile = m_ilDestagingFiles.PrevMost();
            pdestagingfile;
            pdestagingfile = m_ilDestagingFiles.Next( pdestagingfile ) )
    {
        if (    pdestagingfile->Volumeid() == volumeid &&
                pdestagingfile->Fileid() == fileid &&
                pdestagingfile->Fileserial() == fileserial )
        {
            Alloc( *ppfapi = new CFileFilterWrapper( pdestagingfile, iomEngine ) );
            Error( JET_errSuccess );
        }
    }

    m_rwlDestagingFiles.LeaveAsReader();
    fLeave = fFalse;

    //  retry until we have opened a file with the correct physical id

    while ( volumeidActual != volumeid || fileidActual != fileid )
    {
        //  if we have a file open from a previous attempt then close it

        delete *ppfapi;
        *ppfapi = NULL;

        //  translate the file id to a file path and the unambiguous file key path
        //
        //  NOTE:  if we cannot compute this path then the file is truly gone

        Call( m_pfident->ErrGetFilePathById( volumeid, fileid, wszAnyAbsPath, wszKeyPath ) );

        //  if we have retried too many times then fail

        if ( ++cAttempt >= cAttemptMax )
        {
            Error( ErrBlockCacheInternalError( wszAnyAbsPath, "FileOpenByIdRetryLimit" ) );
        }

        //  open the file
        // 
        //  NOTE:  if we cannot find the file then it's name must have changed.  we will retry in that case
        //
        //  NOTE:  we presume any file opened by file id is opened by the cache

        err = ErrFileOpenInternal( wszAnyAbsPath, wszKeyPath, fmf, fTrue, fFalse, NULL, NULL, &pffr );

        if ( err == JET_errFileNotFound || err == JET_errInvalidPath )
        {
            continue;
        }

        Call( err );

        //  get the actual physical id for the file we opened

        Call( pffr->ErrGetPhysicalId( &volumeidActual, &fileidActual, &fileserialActual ) );
    }

    //  verify that the file we opened matches the requested file

    if ( fileserialActual != fileserial )
    {
        Error( ErrERRCheck( JET_errFileNotFound ) );
    }

    //  return the opened file

    *ppfapi = pffr;
    pffr = NULL;

HandleError:
    if ( fLeave )
    {
        m_rwlDestagingFiles.LeaveAsReader();
    }
    delete pffr;
    if ( err == JET_errFileNotFound || err == JET_errInvalidPath )
    {
        ReportCachedFileNotFoundById( volumeid, fileid, fileserial );
        err = ErrERRCheck( JET_errFileNotFound );
    }
    if ( err < JET_errSuccess )
    {
        delete *ppfapi;
        *ppfapi = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileRename(    _In_    IFileAPI* const    pfapi,
                                            _In_z_  const WCHAR* const wszPathDest,
                                            _In_    const BOOL         fOverwriteExisting )
{
    ERR                     err                                     = JET_errSuccess;
    const DWORD             cwchPathSrcMax                          = IFileSystemAPI::cchPathMax;
    WCHAR                   wszPathSrc[ cwchPathSrcMax ]            = { 0 };
    const DWORD             cwchAbsPathSrcMax                       = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPathSrc[ cwchAbsPathSrcMax ]      = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileRename" ) );
    CFilePathTableEntry*    pfpteSrc                                = NULL;
    const DWORD             cwchAbsPathDestMax                      = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPathDest[ cwchAbsPathDestMax ]    = { 0 };
    CFilePathTableEntry*    pfpteDest                               = NULL;

    //  do not accept relative destination paths

    if ( TFileSystemWrapper<I>::FPathIsRelative( wszPathDest ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  wait to lock the entry for the source and destination files

    Call( pfapi->ErrPath( wszPathSrc ) );
    Call( ErrLockFiles( wszPathSrc, wszPathDest, wszAbsPathSrc, wszAbsPathDest, &sem, &pfpteSrc, &pfpteDest ) );

    //  check if we can create the destination file

    Call( pfpteDest->ErrAccessCheck( fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone, fTrue, fFalse ) );

    //  rename the file

    Call( pfapi->ErrRename( wszAbsPathDest, fOverwriteExisting ) );

    //  move any existing handles from the source to the target

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
    const DWORD             cwchAbsPathMax                  = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileDelete" ) );
    CFilePathTableEntry*    pfpte                           = NULL;

    //  wait to lock the entry for this file

    Call( ErrLockFile( wszPath, wszAbsPath, &sem, &pfpte ) );

    //  prepare to delete the file

    Call( ErrPrepareToDelete( pfpte ) );

    //  delete the file

    Call( TFileSystemWrapper<I>::ErrFileDelete( wszAbsPath ) );

    //  remove the open file from our file path table

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
    const DWORD             cwchAbsPathSrcMax                       = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPathSrc[ cwchAbsPathSrcMax ]      = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileMove" ) );
    CFilePathTableEntry*    pfpteSrc                                = NULL;
    const DWORD             cwchAbsPathDestMax                      = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPathDest[ cwchAbsPathDestMax ]    = { 0 };
    CFilePathTableEntry*    pfpteDest                               = NULL;

    //  wait to lock the entry for the source and destination files

    Call( ErrLockFiles( wszPathSource, wszPathDest, wszAbsPathSrc, wszAbsPathDest, &sem, &pfpteSrc, &pfpteDest ) );

    //  prepare to delete the source file

    Call( ErrPrepareToDelete( pfpteSrc ) );

    //  check if we can create the destination file

    Call( pfpteDest->ErrAccessCheck( fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone, fTrue, fFalse ) );

    //  move the file

    Call( TFileSystemWrapper<I>::ErrFileMove( wszAbsPathSrc, wszAbsPathDest, fOverwriteExisting ) );

    //  move any existing handles from the source to the target

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
    const DWORD             cwchAbsPathSrcMax                       = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPathSrc[ cwchAbsPathSrcMax ]      = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileCopy" ) );
    CFilePathTableEntry*    pfpteSrc                                = NULL;
    const DWORD             cwchAbsPathDestMax                      = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPathDest[ cwchAbsPathDestMax ]    = { 0 };
    CFilePathTableEntry*    pfpteDest                               = NULL;

    //  wait to lock the entry for the source and destination files

    Call( ErrLockFiles( wszPathSource, wszPathDest, wszAbsPathSrc, wszAbsPathDest, &sem, &pfpteSrc, &pfpteDest ) );

    //  check if we can access the source file

    Call( pfpteSrc->ErrAccessCheck( IFileAPI::fmfReadOnly, fFalse, fFalse ) );

    //  check if we can create the destination file

    Call( pfpteDest->ErrAccessCheck( fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone, fTrue, fFalse ) );

    //  copy the file

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
    const DWORD             cwchAbsPathMax                  = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileCreate" ) );
    CFilePathTableEntry*    pfpte                           = NULL;
    CFileFilterReference*   pffr                            = NULL;

    *ppfapi = NULL;

    //  wait to lock the entry for this file

    Call( ErrLockFile( wszPath, wszAbsPath, &sem, &pfpte ) );
    
    //  if the file isn't open then open it

    if ( !pfpte->Pof() )
    {
        Call( ErrFileCreateCacheMiss( wszAbsPath, fmf, pfpte, &pffr ) );
    }

    //  the file is already open

    else
    {
        //  open the already open file
        //
        //  NOTE:  we presume any file opened by path is not opened by the cache

        Call( ErrFileOpenCacheHit( fmf, fTrue, fFalse, pfpte->Pof(), &pffr ) );

        //  we are overwriting an existing file then truncate it.  this also invalidates the cache for this file

        if ( DwCreationDispositionFromFileModeFlags( fTrue, fmf ) == CREATE_ALWAYS )
        {
            TraceContextScope tcScope;
            Call( pfpte->Pof()->Pff()->ErrSetSize( *tcScope, 0, fFalse, qosIONormal ) );
        }
    }

    //  return the opened file

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
    return ErrFileOpenInternal( wszPath, fmf, fFalse, NULL, NULL, ppfapi );
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
                                        m_pfident,
                                        m_pctm,
                                        m_pcrep,
                                        volumeidInvalid,
                                        fileidInvalid,
                                        NULL,
                                        fFalse,
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

    Assert( !pof && !pffr || pof && pffr );

    if ( !pfpte )
    {
        return;
    }

    CFilePathHashKey        key( pfpte );
    CFilePathHash::CLock    lock;
    m_filePathHash.WriteLockKey( key, &lock );

    if ( pof )
    {
        pof->RemoveReference( pffr );
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
    const DWORD             cwchKeyPathMax                  = IFileIdentification::cwchKeyPathMax;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]    = { 0 };

    *ppfpte = NULL;

    Call( ErrGetFilePath( wszPath, wszAbsPath, wszKeyPath ) );
    Call( ErrLockFile( wszKeyPath, psem, fFalse, ppfpte ) );

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
    const DWORD             cwchKeyPathSrcMax                       = IFileIdentification::cwchKeyPathMax;
    WCHAR                   wszKeyPathSrc[ cwchKeyPathSrcMax ]      = { 0 };
    const DWORD             cwchKeyPathDestMax                      = IFileIdentification::cwchKeyPathMax;
    WCHAR                   wszKeyPathDest[ cwchKeyPathDestMax ]    = { 0 };
    WCHAR*                  pwszKeyPath1                            = wszKeyPathSrc;
    CFilePathTableEntry**   ppfpte1                                 = ppfpteSrc;
    WCHAR*                  pwszKeyPath2                            = wszKeyPathDest;
    CFilePathTableEntry**   ppfpte2                                 = ppfpteDest;

    Call( ErrGetFilePath( wszPathSrc, wszAbsPathSrc, wszKeyPathSrc ) );
    Call( ErrGetFilePath( wszPathDest, wszAbsPathDest, wszKeyPathDest ) );

    // We use an unusual comparison where longer strings compare as greater than
    // shorter strings and only compare actual bits when lengths are the same.
    if ( ( LOSStrLengthW( wszKeyPathSrc ) > LOSStrLengthW( wszKeyPathDest ) ) ||
         ( LOSStrCompareW( wszKeyPathSrc, wszKeyPathDest ) > 0 )       )
    {
        pwszKeyPath1 = wszKeyPathDest;
        ppfpte1 = ppfpteDest;
        pwszKeyPath2 = wszKeyPathSrc;
        ppfpte2 = ppfpteSrc;
    }

    Call( ErrLockFile( pwszKeyPath1, psem, fFalse, ppfpte1 ) );
    Call( ErrLockFile( pwszKeyPath2, psem, fFalse, ppfpte2 ) );

HandleError:
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrLockFile(  _In_z_  const WCHAR* const          wszKeyPath,
                                        _In_    CSemaphore* const           psem,
                                        _In_    const BOOL                  fCacheOpen,
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
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( errFilePathHash == CFilePathHash::ERR::errSuccess );
        fRemove = fTrue;
    }

    //  if this is a cache open and the entry is already locked then reject the open for now.  the cache will retry.
    //  we do this to avoid a deadlock on destage due to a race where the cache has already tried to open the file when
    //  we decide we need to destage it

    if ( fCacheOpen && pfpteExisting->FOwned() )
    {
        Error( ErrERRCheck( JET_errFileAccessDenied ) );
    }

    Call( pfpteExisting->ErrAddAsOwnerOrWaiter( psem ) );

    pfpteExisting->AddRef();
    fRemove = fFalse;
    pfpteNew = NULL;

    m_filePathHash.WriteUnlockKey( &lock );
    fLocked = fFalse;

    *ppfpte = pfpteExisting;
    psem->Wait();

HandleError:
    if ( fRemove )
    {
        errFilePathHash = m_filePathHash.ErrDeleteEntry( &lock );
        Assert( errFilePathHash == CFilePathHash::ERR::errSuccess );
    }
    delete pfpteNew;
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
ERR TFileSystemFilter<I>::ErrGetFilePath(   _In_z_                                                  const WCHAR* const  wszPath,
                                            _Out_bytecap_c_( cbOSFSAPI_MAX_PATHW )                  WCHAR* const        wszAbsPath,
                                            _Out_bytecap_c_( IFileIdentification::cbKeyPathMax )    WCHAR* const        wszKeyPath )
{
    ERR err = JET_errSuccess;

    wszAbsPath[ 0 ] = 0;
    wszKeyPath[ 0 ] = 0;

    //  compute the absolute path of the given path

    Call( TFileSystemWrapper<I>::ErrPathComplete( wszPath, wszAbsPath ) );

    //  translate the absolute path into an unambiguous file key path

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

    //  if the file is open then ask any cache that may have this file open to close it so we can delete it

    if ( pfpte->Pof() && pfpte->Pof()->Pc() )
    {
        Call( pfpte->Pof()->Pff()->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );
        Call( pfpte->Pof()->Pc()->ErrClose( volumeid, fileid, fileserial ) );
    }

    //  check if we can delete the file

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
    ERR                             err                     = JET_errSuccess;
    IBlockCacheConfiguration*       pbcconfig               = NULL;
    ICachedFileConfiguration*       pcfconfig               = NULL;
    BOOL                            fCachingEnabled         = fFalse;
    BOOL                            fEverEligibleForCaching = fFalse;
    CFileFilter*                    pff                     = NULL;
    BOOL                            fCreated                = fFalse;
    CFilePathTableEntry::COpenFile* pof                     = NULL;

    *ppffr = NULL;

    //  get the caching configuration for this file

    Call( ErrGetConfiguration( &pbcconfig ) );
    Call( pbcconfig->ErrGetCachedFileConfiguration( pfpte->WszKeyPath(), &pcfconfig ) );

    //  determine if caching is enabled for this file

    fCachingEnabled = pcfconfig->FCachingEnabled();

    //  we do not support caching temp files

    if ( fmf & IFileAPI::fmfTemporary )
    {
        fCachingEnabled = fFalse;
    }

    //  if caching is enabled for this file then attempt to mark it as ever cached
    //
    //  NOTE:  we cannot attach a file to the cache that is not marked as ever cached

    if ( fCachingEnabled )
    {
        Call( ErrTryMarkAsEverEligibleForCaching(   wszAnyAbsPath,
                                                    ( fmf & IFileAPI::fmfOverwriteExisting ) != 0,
                                                    fFalse,
                                                    &fEverEligibleForCaching ) );
        fCreated = fEverEligibleForCaching;
    }

    //  create the file

    if ( fCreated && ( fmf & IFileAPI::fmfOverwriteExisting ) == 0 )
    {
        Call( TFileSystemWrapper<I>::ErrFileOpen(   wszAnyAbsPath,
                                                    fmf,
                                                    (IFileAPI**)&pff ) );
    }
    else
    {
        Call( TFileSystemWrapper<I>::ErrFileCreate( wszAnyAbsPath, fmf, (IFileAPI**)&pff ) );
        fCreated = fTrue;
    }

    //  configure the newly created file

    Call( ErrFileConfigure( pff, &pcfconfig, fEverEligibleForCaching, wszAnyAbsPath ) );

    //  make the file available for other opens

    Call( pfpte->ErrOpenFile( &pff, &pof ) );

    //  provide a wrapper for the cached file that will release it on the last close

    Alloc( *ppffr = new CFileFilterReference( this, pof, fmf, fFalse ) );
    pof = NULL;

HandleError:
    delete pof;
    delete pff;
    delete pcfconfig;
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
ERR TFileSystemFilter<I>::ErrTryMarkAsEverEligibleForCaching(   _In_z_  const WCHAR* const  wszAnyAbsPath,
                                                                _In_    const BOOL          fOverwriteExisting,
                                                                _In_    const BOOL          fOpenExisting,
                                                                _Out_   BOOL* const         pfMarked )
{
    ERR             err                                             = JET_errSuccess;
    IFileFindAPI*   pffapi                                          = NULL;
    BOOL            fFolder                                         = fFalse;
    BOOL            fReadOnly                                       = fFalse;
    QWORD           cb                                              = 0;
    WCHAR           wszAlternateDataStreamPath[ OSFSAPI_MAX_PATH ]  = { 0 };
    IFileAPI*       pfapiAlternateDataStream                        = NULL;

    *pfMarked = fFalse;

    if ( fOpenExisting )
    {
        err = TFileSystemWrapper<I>::ErrFileFind( wszAnyAbsPath, &pffapi );
        if ( err >= JET_errSuccess )
        {
            err = pffapi->ErrNext();
        }

        if ( err == JET_errFileNotFound || err == JET_errInvalidPath )
        {
            Error( ErrERRCheck( JET_errFileNotFound ) );
        }
        Call( err );

        Call( pffapi->ErrIsFolder( &fFolder ) );
        if ( fFolder )
        {
            Error( JET_errSuccess );
        }

        Call( pffapi->ErrIsReadOnly( &fReadOnly ) );
        if ( fReadOnly )
        {
            Error( JET_errSuccess );
        }

        Call( pffapi->ErrSize( &cb, IFileAPI::filesizeLogical ) );
        if ( !cb )
        {
            Error( JET_errSuccess );
        }
    }

    Call( ErrOSStrCbCopyW( wszAlternateDataStreamPath, _cbrg( wszAlternateDataStreamPath ), wszAnyAbsPath ) );
    Call( ErrOSStrCbAppendW( wszAlternateDataStreamPath, _cbrg( wszAlternateDataStreamPath ), c_wszFileEverEligibleForCaching ) );
    Call( TFileSystemWrapper<I>::ErrFileCreate( wszAlternateDataStreamPath,
                                                fOverwriteExisting ? IFileAPI::fmfOverwriteExisting : IFileAPI::fmfNone,
                                                &pfapiAlternateDataStream ) );

    *pfMarked = fTrue;

HandleError:
    delete pfapiAlternateDataStream;
    delete pffapi;
    if ( err < JET_errSuccess )
    {
        switch ( err )
        {
            case JET_errInvalidPath:
            case JET_errBufferTooSmall:
            case JET_errFileNotFound:
            case JET_errFileAlreadyExists:
                err = JET_errSuccess;
                break;
        }

        *pfMarked = fFalse;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrFileConfigure( _In_        CFileFilter* const                  pff,
                                            _Inout_     ICachedFileConfiguration** const    ppcfconfig,
                                            _In_        const BOOL                          fEverEligibleForCaching,
                                            _In_z_      const WCHAR* const                  wszAnyAbsPath )
{
    ERR                         err         = JET_errSuccess;
    ICachedFileConfiguration*   pcfconfig   = *ppcfconfig;
    VolumeId                    volumeid    = volumeidInvalid;
    FileId                      fileid      = fileidInvalid;

    *ppcfconfig = NULL;

    Call( m_pfident->ErrGetFileId( wszAnyAbsPath, &volumeid, &fileid ) );

    pff->Configure( volumeid, fileid, &pcfconfig, fEverEligibleForCaching );

HandleError:
    delete pcfconfig;
    return err;
}


template< class I >
ERR TFileSystemFilter<I>::ErrFileOpenInternal(  _In_z_      const WCHAR* const                              wszPath,
                                                _In_        const IFileAPI::FileModeFlags                   fmf,
                                                _In_        const BOOL                                      fDetachFile,
                                                _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                                _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                                _Out_       IFileAPI** const                                ppfapi )
{
    ERR                     err                             = JET_errSuccess;
    const DWORD             cwchAbsPathMax                  = IFileSystemAPI::cchPathMax;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    const DWORD             cwchKeyPathMax                  = IFileIdentification::cwchKeyPathMax;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]    = { 0 };
    CFileFilterReference*   pffr                            = NULL;

    *ppfapi = NULL;

    //  translate the relative file path into an absolute path and an unambiguous file key path

    Call( ErrGetFilePath( wszPath, wszAbsPath, wszKeyPath ) );

    //  open the file
    //
    //  NOTE:  we presume any file opened by path is not opened by the cache

    Call( ErrFileOpenInternal(  wszAbsPath,
                                wszKeyPath, 
                                fmf, 
                                fFalse, 
                                fDetachFile, 
                                pfnDetachFileStatus,
                                keyDetachFileStatus,
                                &pffr ) );

    //  return the opened file

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
ERR TFileSystemFilter<I>::ErrFileOpenInternal(  _In_z_      const WCHAR* const                              wszAnyAbsPath,
                                                _In_z_      const WCHAR* const                              wszKeyPath,
                                                _In_        const IFileAPI::FileModeFlags                   fmf,
                                                _In_        const BOOL                                      fCacheOpen,
                                                _In_        const BOOL                                      fDetachFile,
                                                _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                                _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                                _Out_       CFileFilterReference** const                    ppffr )
{
    ERR                     err         = JET_errSuccess;
    CSemaphore              sem( CSyncBasicInfo( "TFileSystemFilter<I>::ErrFileOpenInternal" ) );
    CFilePathTableEntry*    pfpte       = NULL;

    *ppffr = NULL;

    //  wait to lock the entry for this file

    Call( ErrLockFile( wszKeyPath, &sem, fCacheOpen, &pfpte ) );

    //  if the file isn't open then open it

    if ( !pfpte->Pof() )
    {
        Call( ErrFileOpenCacheMiss( wszAnyAbsPath,
                                    fmf,
                                    fCacheOpen, 
                                    pfpte, 
                                    fDetachFile, 
                                    pfnDetachFileStatus,
                                    keyDetachFileStatus,
                                    ppffr ) );
    }

    //  the file is already open

    else
    {
        //  open the already open file

        Call( ErrFileOpenCacheHit( fmf, fFalse, fCacheOpen, pfpte->Pof(), ppffr ) );

        //  we are overwriting an existing file then truncate it.  this also invalidates the cache for this file

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
ERR TFileSystemFilter<I>::ErrFileOpenCacheMiss( _In_z_      const WCHAR* const                              wszAnyAbsPath,
                                                _In_        const IFileAPI::FileModeFlags                   fmf,
                                                _In_        const BOOL                                      fCacheOpen,
                                                _In_        CFilePathTableEntry* const                      pfpte,
                                                _In_        const BOOL                                      fDetachFile,
                                                _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                                _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                                _Out_       CFileFilterReference** const                    ppffr )
{
    ERR                             err             = JET_errSuccess;
    CFileFilter*                    pff             = NULL;
    CFilePathTableEntry::COpenFile* pof             = NULL;

    *ppffr = NULL;

    //  open the file with the original flags

    Call( ErrFileOpenAndConfigure(  wszAnyAbsPath,
                                    fmf,
                                    fCacheOpen,
                                    pfpte,
                                    fDetachFile,
                                    pfnDetachFileStatus,
                                    keyDetachFileStatus,
                                    &pff ) );

    //  if we opened this file read only and it is attached to the cache then open it read write so that the cache can
    //  perform write back to the file due to cache pressure

    if ( pff->FAttached() && ( fmf & fmfReadOnlyMask ) != 0 )
    {
        delete pff;
        pff = NULL;

        Call( ErrFileOpenAndConfigure(  wszAnyAbsPath,
                                        fmf & ~fmfReadOnlyMask, 
                                        fCacheOpen, 
                                        pfpte, 
                                        fDetachFile,
                                        pfnDetachFileStatus,
                                        keyDetachFileStatus,
                                        &pff ) )
    }

    //  make the file available for other opens

    Call( pfpte->ErrOpenFile( &pff, &pof ) );

    //  provide a wrapper for the cached file that will release it on the last close

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
ERR TFileSystemFilter<I>::ErrFileOpenAndConfigure(  _In_z_      const WCHAR* const                              wszAnyAbsPath,
                                                    _In_        const IFileAPI::FileModeFlags                   fmf,
                                                    _In_        const BOOL                                      fCacheOpen,
                                                    _In_        CFilePathTableEntry* const                      pfpte,
                                                    _In_        const BOOL                                      fDetachFile,
                                                    _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                                    _In_opt_    const DWORD_PTR                                 keyDetachFileStatus,
                                                    _Out_       CFileFilter** const                             ppff )
{
    ERR                             err                     = JET_errSuccess;
    IBlockCacheConfiguration*       pbcconfig               = NULL;
    ICachedFileConfiguration*       pcfconfig               = NULL;
    BOOL                            fEverEligibleForCaching = fFalse;
    CFileFilter*                    pff                     = NULL;

    *ppff = NULL;

    //  get the caching configuration for this file

    Call( ErrGetConfiguration( &pbcconfig ) );
    Call( pbcconfig->ErrGetCachedFileConfiguration( pfpte->WszKeyPath(), &pcfconfig ) );

    //  determine if this file has ever been eligible for caching

    Call( ErrEverEligibleForCaching( wszAnyAbsPath, fmf, &fEverEligibleForCaching ) );

    //  if the file has never been eligible for caching then see if it should be marked as such

    if ( !fEverEligibleForCaching )
    {
        //  if caching is enabled for this file and we are opening it for RW access then attempt to mark it as ever
        //  cached
        //
        //  NOTE:  we cannot attach a file to the cache that is not marked as ever cached

        if ( pcfconfig->FCachingEnabled() && ( fmf & fmfReadOnlyMask ) == 0 )
        {
            Call( ErrTryMarkAsEverEligibleForCaching( wszAnyAbsPath, fFalse, fTrue, &fEverEligibleForCaching ) );
        }
    }

    //  open the file with the specified flags

    Call( TFileSystemWrapper<I>::ErrFileOpen( wszAnyAbsPath, fmf, (IFileAPI**)&pff ) );

    //  configure the newly opened file

    Call( ErrFileConfigure( pff, &pcfconfig, fEverEligibleForCaching, wszAnyAbsPath ) );

    //  get the file's cache

    Call( ErrGetCache(  pff, 
                        fEverEligibleForCaching, 
                        fCacheOpen, 
                        pfpte->WszKeyPath(), 
                        fDetachFile,
                        pfnDetachFileStatus,
                        keyDetachFileStatus ) );

    //  return the opened file

    *ppff = pff;
    pff = NULL;

HandleError:
    delete pff;
    delete pcfconfig;
    if ( err < JET_errSuccess )
    {
        delete *ppff;
        *ppff = NULL;
    }
    return err;
}

template< class I >
ERR TFileSystemFilter<I>::ErrEverEligibleForCaching(    _In_z_  const WCHAR* const              wszAnyAbsPath,
                                                        _In_    const IFileAPI::FileModeFlags   fmf,
                                                        _Out_   BOOL* const                     pfEverEligible )
{
    ERR         err                                             = JET_errSuccess;
    WCHAR       wszAlternateDataStreamPath[ OSFSAPI_MAX_PATH ]  = { 0 };
    IFileAPI*   pfapiAlternateDataStream                        = NULL;

    *pfEverEligible = fFalse;

    Call( ErrOSStrCbCopyW( wszAlternateDataStreamPath, _cbrg( wszAlternateDataStreamPath ), wszAnyAbsPath ) );
    Call( ErrOSStrCbAppendW( wszAlternateDataStreamPath, _cbrg( wszAlternateDataStreamPath ), c_wszFileEverEligibleForCaching ) );
    Call( TFileSystemWrapper<I>::ErrFileOpen( wszAlternateDataStreamPath, fmf, &pfapiAlternateDataStream ) );

    *pfEverEligible = fTrue;

HandleError:
    delete pfapiAlternateDataStream;
    if ( err < JET_errSuccess )
    {
        switch ( err )
        {
            case JET_errInvalidPath:
            case JET_errBufferTooSmall:
            case JET_errFileNotFound:
                err = JET_errSuccess;
                break;
        }

        *pfEverEligible = fFalse;
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

    //  perform the access check against all currently open file handles for this file

    Call( pof->Pfpte()->ErrAccessCheck( fmf, fCreate, fCacheOpen ) );

    //  provide a wrapper for the cached file that will release it on the last close

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

template<class I>
ERR TFileSystemFilter<I>::ErrCacheOpenFailure(  _In_ CFileFilter* const pff,
                                                _In_ const char* const  szFunction,
                                                _In_ const ERR          errFromCall,
                                                _In_ const ERR          errToReturn )
{
    WCHAR           wszCachingFile[ IFileSystemAPI::cchPathMax ]    = { 0 };
    WCHAR           wszFunction[ 256 ]                              = { 0 };
    WCHAR           wszErrorFromCall[ 64 ]                          = { 0 };
    WCHAR           wszErrorToReturn[ 64 ]                          = { 0 };
    const WCHAR*    rgpwsz[]                                        = { wszCachingFile, wszFunction, wszErrorFromCall, wszErrorToReturn };

    pff->Pcfconfig()->CachingFilePath(wszCachingFile);
    OSStrCbFormatW( wszFunction, sizeof( wszFunction ), L"%hs", szFunction );
    OSStrCbFormatW( wszErrorFromCall, sizeof( wszErrorFromCall ), L"%i (0x%08x)", errFromCall, errFromCall );
    OSStrCbFormatW( wszErrorToReturn, sizeof( wszErrorToReturn ), L"%i (0x%08x)", errToReturn, errToReturn );

    m_pfsconfig->EmitEvent( eventWarning,
                            BLOCK_CACHE_CATEGORY,
                            BLOCK_CACHE_CACHING_FILE_OPEN_FAILURE_ID,
                            _countof( rgpwsz ),
                            rgpwsz,
                            JET_EventLoggingLevelMin );

    OSTraceSuspendGC();
    BlockCacheNotableEvent( wszCachingFile, 
                            OSFormat(   "CacheOpenFailure:%hs:%i:%i",
                                        szFunction, 
                                        errFromCall,
                                        errToReturn ) );
    OSTraceResumeGC();

    return errToReturn;
}

template< class I >
ERR TFileSystemFilter<I>::ErrGetCache(  _In_        CFileFilter* const                              pff,
                                        _In_        const BOOL                                      fEverEligibleForCaching,
                                        _In_        const BOOL                                      fCacheOpen,
                                        _In_z_      const WCHAR* const                              wszKeyPath,
                                        _In_        const BOOL                                      fDetachFile,
                                        _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                        _In_opt_    const DWORD_PTR                                 keyDetachFileStatus )
{
    ERR                         err         = JET_errSuccess;
    CCachedFileHeader*          pcfh        = NULL;
    IBlockCacheConfiguration*   pbcconfig   = NULL;
    ICache*                     pc          = NULL;

    //  determine if this file has ever been eligible for caching.  if it is not marked then open it as if it were not
    //  already cached

    if ( !fEverEligibleForCaching )
    {
        Error( JET_errSuccess );
    }

    //  determine if this file has a cached file header.  if the file doesn't have a recognizable cached file header
    //  then open it as if it were not already cached

    err = CCachedFileHeader::ErrLoad( m_pfsconfig, pff, &pcfh );

    if ( err == JET_errReadVerifyFailure )
    {
        Error( JET_errSuccess );
    }

    Call( err );

    //  the file is cached so open its associated caching file

    Call( ErrGetConfiguration( &pbcconfig ) );
    err = m_pcrep->ErrOpenById( this,
                                m_pfsconfig,
                                pbcconfig,
                                pcfh->VolumeidCache(),
                                pcfh->FileidCache(),
                                pcfh->RgbUniqueIdCache(),
                                &pc );
    if ( err < JET_errSuccess )
    {
        Call( ErrCacheOpenFailure( pff, "OpenById", err, ErrERRCheck(JET_errDiskIO)));
    }

    //  save the caching state

    pff->SetCacheState( &pc, &pcfh );

    //  if caching is now disabled or the cache configuration has changed significantly and then detach the file from
    //  the cache
    //
    //  NOTE:  if the detach fails then we will only return an error for an explicit detach file operation

    if ( fDetachFile || FDetachFile( pff, pbcconfig, fCacheOpen ) )
    {
        const ERR errT = ErrDetachFile( pff, pfnDetachFileStatus, keyDetachFileStatus );
        if ( fDetachFile )
        {
            Call( errT );
        }
    }

HandleError:
    delete pc;
    delete pcfh;
    return err;
}

template< class I >
BOOL TFileSystemFilter<I>::FDetachFile( _In_ CFileFilter* const                 pff,
                                        _In_ IBlockCacheConfiguration* const    pbcconfig,
                                        _In_ const BOOL                         fCacheOpen )
{
    //  we cannot detach a file that is being opened by the cache for write back

    if ( fCacheOpen )
    {
        return fFalse;
    }

    //  we cannot detach a RO file (we will reopen as RW shortly and get another chance)

    if ( pff->Fmf() & fmfReadOnlyMask )
    {
        return fFalse;
    }

    //  we cannot detach if it isn't enabled
    //
    //  NOTE:  this is what prevents code that isn't EBC aware of detaching cached files on open

    if ( !pbcconfig->FDetachEnabled() )
    {
        return fFalse;
    }

    //  if caching is disabled for the file then we should detach

    if ( !pff->Pcfconfig()->FCachingEnabled() )
    {
        return fTrue;
    }

    //  if the cache is disabled then we should detach

    if ( pff->Pc() && !pff->Pc()->FEnabled() )
    {
        return fTrue;
    }

    return fFalse;
}

template< class I >
ERR TFileSystemFilter<I>::ErrDetachFile(    _In_        CFileFilter* const                              pff,
                                            _In_opt_    const TFileSystemFilter<I>::PfnDetachFileStatus pfnDetachFileStatus,
                                            _In_opt_    const DWORD_PTR                                 keyDetachFileStatus )
{
    ERR                 err                                         = JET_errSuccess;
    const DWORD         cbBlockSize                                 = max( cbCachedBlock, pff->Pcfconfig()->CbBlockSize());
    CDestagingFile*     pdestagingfile                              = NULL;
    void*               pvData                                      = NULL;
    TraceContextScope   tcScope( iorpBlockCache );
    WCHAR               wszCachedFile[ IFileSystemAPI::cchPathMax ] = { 0 };
    BOOL                fPresumeDetached                            = fFalse;

    //  allow only the cache to open this partially constructed file

    Alloc( pdestagingfile = new CDestagingFile( pff ) );
    m_rwlDestagingFiles.EnterAsWriter();
    m_ilDestagingFiles.InsertAsNextMost( pdestagingfile );
    m_rwlDestagingFiles.LeaveAsWriter();

    //  ask the cache to destage the file (except for the pinned header data)

    Call( pff->Pc()->ErrDestage(    pff->Pcfh()->Volumeid(),
                                    pff->Pcfh()->Fileid(),
                                    pff->Pcfh()->Fileserial(),
                                    (ICache::PfnDestageStatus)pfnDetachFileStatus,
                                    keyDetachFileStatus ) );

    //  ensure the destage is committed

    Call( pff->Pc()->ErrFlush( pff->Pcfh()->Volumeid(), pff->Pcfh()->Fileid(), pff->Pcfh()->Fileserial() ) );

    //  read the pinned header data out of the cache
    //
    //  NOTE:  if someone shrank this number since the file was attached then we could lose data!

    Alloc( pvData = PvOSMemoryPageAlloc( pff->Pcfh()->CbPinnedHeader(), NULL));
    Call( pff->Pc()->ErrRead(   *tcScope,
                                pff->Pcfh()->Volumeid(),
                                pff->Pcfh()->Fileid(),
                                pff->Pcfh()->Fileserial(),
                                0,
                                pff->Pcfh()->CbPinnedHeader(),
                                (BYTE*)pvData,
                                qosIONormal,
                                cpDontCache,
                                NULL,
                                NULL ) );

    //  write the pinned header data out to the file
    //
    //  NOTE:  if this write fails then the file may be destroyed!

    Call( pff->ErrPath( wszCachedFile ) );
    BlockCacheNotableEvent( wszCachedFile, "Detach" );

    Call( pff->ErrWrite(    *tcScope, 
                            0, 
                            pff->Pcfh()->CbPinnedHeader(),
                            (const BYTE*)pvData, 
                            qosIONormal, 
                            iomRaw,
                            NULL, 
                            NULL, 
                            NULL ) );
    fPresumeDetached = fTrue;
    Call( pff->ErrFlush( iofrBlockCache, iomRaw ) );

    //  ask the cache to close the file
    //
    //  NOTE:  if the detach was successful then the cache can no longer open this file and it can evict any remaining
    //  cached data.  if it wasn't successful then we will continue on with the file still attached

    Call( pff->Pc()->ErrClose( pff->Pcfh()->Volumeid(), pff->Pcfh()->Fileid(), pff->Pcfh()->Fileserial() ) );

HandleError:
    OSMemoryPageFree( pvData );
    if ( pdestagingfile )
    {
        m_rwlDestagingFiles.EnterAsWriter();
        m_ilDestagingFiles.Remove( pdestagingfile );
        m_rwlDestagingFiles.LeaveAsWriter();
    }
    err = ErrAccumulateError( err, pff->Pc()->ErrClose( pff->Pcfh()->Volumeid(), pff->Pcfh()->Fileid(), pff->Pcfh()->Fileserial() ) );
    delete pdestagingfile;
    if ( fPresumeDetached )
    {
        pff->SetCacheState( NULL, NULL );
    }
    if ( err < JET_errSuccess )
    {
        OSTraceSuspendGC();
        BlockCacheNotableEvent( wszCachedFile, OSFormat( "DetachFailed:%i", err ) );
        OSTraceResumeGC();
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
        Error( ErrERRCheck( JET_errOutOfMemory ) );
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

template< class I >
const WCHAR TFileSystemFilter<I>::c_wszFileEverEligibleForCaching[] = L":788638d4-9b8c-4518-99a6-2512769b1676";

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

//  CFileSystemFilter:  concrete TFileSystemFilter<IFileSystemFilter>

class CFileSystemFilter : public TFileSystemFilter<IFileSystemFilter>
{
    public:  //  specialized API

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

//  CFileFilterReference:  a reference to an IFileFilter implementation.

class CFileFilterReference : public CFileFilterWrapper
{
    public:  //  specialized API

        CFileFilterReference(   _In_ TFileSystemFilter<IFileSystemFilter>* const    pfsf,
                                _In_ CFilePathTableEntry::COpenFile* const          pof,
                                _In_ const IFileAPI::FileModeFlags                  fmf,
                                _In_ const BOOL                                     fCacheOpen )
            :   CFileFilterWrapper( pof->Pff(), fCacheOpen ? iomRaw : iomEngine ),
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

    public:  //  IFileAPI

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
    ERR     err     = JET_errSuccess;
    BOOL    fLeave  = fFalse;

    m_critReferences.Enter();
    fLeave = fTrue;

    //  perform the access check against all currently open file handles for this file

    for (   CFileFilterReference* ffr = m_ilReferences.PrevMost();
            ffr != NULL && err >= JET_errSuccess;
            ffr = m_ilReferences.Next( ffr ) )
    {
        //  if the current open file is for the cache then do not access check against its share mode
        
        if ( ffr->FCacheOpen() )
        {
            continue;
        }

        //  if the proposed file is for the cache then it automatically gets access

        if ( fCacheOpen )
        {
            continue;
        }

        //  get the desired access for the proposed file

        DWORD dwDesiredAccess = DwDesiredAccessFromFileModeFlags( fmf );

        //  get the share mode for the current open file

        DWORD dwShareMode = DwShareModeFromFileModeFlags( ffr->Fmf() );

        //  do not grant access if the desired access doesn't match the share mode of the current open file

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

        //  get the creation disposition for the proposed file

        DWORD dwCreationDisposition = DwCreationDispositionFromFileModeFlags( fCreate, fmf );

        //  existing file check

        if ( dwCreationDisposition == CREATE_NEW )
        {
            err = ErrERRCheck( JET_errFileAlreadyExists );
        }
    }

    if ( fLeave )
    {
        m_critReferences.Leave();
    }
    return err;
}

INLINE ERR CFilePathTableEntry::COpenFile::ErrDeleteCheck()
{
    ERR     err     = JET_errSuccess;
    BOOL    fLeave  = fFalse;

    m_critReferences.Enter();
    fLeave = fTrue;

    //  perform the delete check against all currently open file handles for this file

    for (   CFileFilterReference* ffr = m_ilReferences.PrevMost();
            ffr != NULL && err >= JET_errSuccess;
            ffr = m_ilReferences.Next( ffr ) )
    {
        //  get the share mode for the current open file

        DWORD dwShareMode = DwShareModeFromFileModeFlags( ffr->Fmf() );

        //  delete file check

        if ( !( dwShareMode & FILE_SHARE_DELETE ) )
        {
            err = ErrERRCheck( JET_errFileAccessDenied );
        }
    }

    if ( fLeave )
    {
        m_critReferences.Leave();
    }
    return err;
}
