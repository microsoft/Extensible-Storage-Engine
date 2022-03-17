// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TCacheRepository:  core implementation of ICacheRepository and its derivatives.
//
//  This class provides a per-process repository of all block caches implementing ICache and backed by a caching file.
//  The lifecycle of an ICache implementation is maintained via CCacheReference wrappers over the core implementation.
//  Caches can be accesssed by path or by physical id.

class CCacheReference;
static SIZE_T CCacheReferenceOffsetOfILE();

template< class I >
class TCacheRepository  //  crep
    :   public I
{
    public:

        class CCachePathTableEntry;

    public:

        TCacheRepository( _In_ IFileIdentification* const pfident, _In_ ICacheTelemetry* const pctm );

        virtual ~TCacheRepository();

        void Cleanup() { CloseAllCaches(); }

        void AddCacheReference( _In_ CCacheReference* const         pcr,
                                _In_ CCachePathTableEntry* const    pcpte );

        void RemoveCacheReference(  _In_ CCacheReference* const         pcr,
                                    _In_ CCachePathTableEntry* const    pcpte );

    public:  //  ICacheRepository

        ERR ErrOpen(    _In_    IFileSystemFilter* const        pfsf,
                        _In_    IFileSystemConfiguration* const pfsconfig,
                        _Inout_ ICacheConfiguration** const     ppcconfig,
                        _Out_   ICache** const                  ppc ) override;

        ERR ErrOpenById(    _In_                IFileSystemFilter* const        pfsf,
                            _In_                IFileSystemConfiguration* const pfsconfig,
                            _In_                IBlockCacheConfiguration* const pbcconfig,
                            _In_                const VolumeId                  volumeid,
                            _In_                const FileId                    fileid,
                            _In_reads_(cbGuid)  const BYTE* const               rgbUniqueId,
                            _Out_               ICache** const                  ppc ) override;

    private:

        //  Wait context for cache miss collisions on the cache path table.

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

    public:

        //  A cache path table entry.

        class CCachePathTableEntry  //  cpte
        {
            public:

                CCachePathTableEntry( _Deref_inout_z_ const WCHAR** const pwszKeyPath );

                ~CCachePathTableEntry();

                const WCHAR* WszKeyPath() const { return m_wszKeyPath; }
                const UINT UiHash() const { return m_uiHash; }
                ICache* Pc() const { return m_pc; }

                void AddRef() { AtomicIncrement((LONG*)&m_cref); }
                BOOL FRelease() { return AtomicDecrement((LONG*)&m_cref) == 0; }

                ERR ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem );
                void AddNextOwner();
                void RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem );

                static UINT UiHash( _In_z_ const WCHAR* const wszKeyPath );

                VOID OpenCache( _Inout_ ICache** const ppc );

                void AddReference( _In_ CCacheReference* const pcr )
                {
                    m_ilReferences.InsertAsNextMost( pcr );
                }

                void RemoveReference( _In_ CCacheReference* const pcr )
                {
                    m_ilReferences.Remove( pcr );
                }

                static SIZE_T OffsetOfILE() { return OffsetOf( CCachePathTableEntry, m_ile ); }

            private:

                typename CInvasiveList< CCachePathTableEntry, OffsetOfILE >::CElement   m_ile;
                const WCHAR* const                                                      m_wszKeyPath;
                const UINT                                                              m_uiHash;
                volatile int                                                            m_cref;
                CSemaphore*                                                             m_psemOwner;
                CInvasiveList<CWaiter, CWaiter::OffsetOfILE>                            m_ilWaiters;
                ICache*                                                                 m_pc;
                CInvasiveList<CCacheReference, CCacheReferenceOffsetOfILE>              m_ilReferences;
        };

    public:

        void ReleaseCache(  _In_opt_    CCachePathTableEntry* const pcpte,
                            _In_opt_    CSemaphore* const           psem    = NULL,
                            _In_opt_    CCacheReference* const      pcr     = NULL );

    private:

        void CloseAllCaches();

        ERR ErrLockCache(   _In_z_  const WCHAR* const              wszKeyPath,
                            _In_    CSemaphore* const               psem,
                            _Out_   CCachePathTableEntry** const    ppcpte );

        ERR ErrOpenCacheMiss(   _In_    IFileSystemFilter* const        pfsf,
                                _In_    IFileSystemConfiguration* const pfsconfig,
                                _Inout_ ICacheConfiguration** const     ppcconfig,
                                _In_z_  const WCHAR* const              wszAbsPath,
                                _In_    CCachePathTableEntry* const     pcpte,
                                _Out_   CCacheReference** const         ppcr );
        ERR ErrOpenByIdCacheMiss(   _In_    IFileSystemFilter* const        pfsf,
                                    _In_    IFileSystemConfiguration* const pfsconfig,
                                    _In_    IBlockCacheConfiguration* const pbcconfig,
                                    _In_z_  const WCHAR* const              wszAnyAbsPath,
                                    _In_    CCachePathTableEntry* const     pcpte,
                                    _Out_   CCacheReference** const         ppcr );
        ERR ErrOpenCacheHit(    _In_    CCachePathTableEntry* const pcpte,
                                _Out_   CCacheReference** const     ppcr );

        ERR ErrMountCache(  _In_    CCachePathTableEntry* const     pcpte,
                            _In_    IFileSystemFilter* const        pfsf,
                            _In_    IFileSystemConfiguration* const pfsconfig,
                            _Inout_ ICacheConfiguration** const     ppcconfig,
                            _Inout_ IFileFilter** const             ppff,
                            _In_    const BOOL                      fCreate );
        
        ERR ErrCachingFileNotFoundById( _In_                IFileSystemConfiguration* const pfsconfig,
                                        _In_                const VolumeId                  volumeid,
                                        _In_                const FileId                    fileid, 
                                        _In_reads_(cbGuid)  const BYTE* const               rgbUniqueId );

    private:

        IFileIdentification* const                                              m_pfident;
        ICacheTelemetry* const                                                  m_pctm;
        CCriticalSection                                                        m_critCachePathTableEntries;
        CInvasiveList<CCachePathTableEntry, CCachePathTableEntry::OffsetOfILE>  m_ilCachePathTableEntries;
};

template< class I >
TCacheRepository<I>::TCacheRepository(  _In_ IFileIdentification* const pfident,
                                        _In_ ICacheTelemetry* const     pctm )
    :   m_pfident( pfident ),
        m_pctm( pctm ),
        m_critCachePathTableEntries( CLockBasicInfo( CSyncBasicInfo( "TCacheRepository<I>::m_critCachePathTableEntries" ), rankCacheRepository, 0 ) )
{
}

template< class I >
TCacheRepository<I>::~TCacheRepository()
{
    Cleanup();
}

template< class I >
void TCacheRepository<I>::AddCacheReference(    _In_ CCacheReference* const         pcr,
                                                _In_ CCachePathTableEntry* const    pcpte )
{
    pcpte->AddRef();
    pcpte->AddReference( pcr );
}

template< class I >
void TCacheRepository<I>::RemoveCacheReference( _In_ CCacheReference* const         pcr,
                                                _In_ CCachePathTableEntry* const    pcpte )
{
    ReleaseCache( pcpte, NULL, pcr );
}

template< class I >
ERR TCacheRepository<I>::ErrOpen(   _In_    IFileSystemFilter* const        pfsf,
                                    _In_    IFileSystemConfiguration* const pfsconfig,
                                    _Inout_ ICacheConfiguration** const     ppcconfig,
                                    _Out_   ICache** const                  ppc )
{
    ERR                     err                             = JET_errSuccess;
    const DWORD             cwchAbsPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszAbsPath[ cwchAbsPathMax ]    = { 0 };
    const DWORD             cwchKeyPathMax                  = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]    = { 0 };
    CSemaphore              sem( CSyncBasicInfo( "TCacheRepository<I>::ErrOpen" ) );
    CCachePathTableEntry*   pcpte                           = NULL;
    CCacheReference*        pcr                             = NULL;

    *ppc = NULL;

    //  only open the cache if it is enabled

    if ( (*ppcconfig)->FCacheEnabled() )
    {
        //  get the absolute path of the cache

        (*ppcconfig)->Path( wszAbsPath );

        //  translate the absolute path into an unambiguous file key path

        Call( m_pfident->ErrGetFileKeyPath( wszAbsPath, wszKeyPath ) );

        //  wait to lock the entry for this cache

        Call( ErrLockCache( wszKeyPath, &sem, &pcpte ) );

        //  if the cache isn't open then open it

        if ( !pcpte->Pc() )
        {
            Call( ErrOpenCacheMiss( pfsf, pfsconfig, ppcconfig, wszAbsPath, pcpte, &pcr ) );
        }

        //  the cache is already open

        else
        {
            //  open the already open cache

            Call( ErrOpenCacheHit( pcpte, &pcr ) );
        }

        //  return the opened cache

        *ppc = pcr;
        pcr = NULL;
    }

HandleError:
    ReleaseCache( pcpte, &sem );
    delete pcr;
    if ( err < JET_errSuccess )
    {
        delete *ppc;
        *ppc = NULL;
    }
    return err;
}

template< class I >
ERR TCacheRepository<I>::ErrOpenById(   _In_                    IFileSystemFilter* const        pfsf,
                                        _In_                    IFileSystemConfiguration* const pfsconfig,
                                        _In_                    IBlockCacheConfiguration* const pbcconfig,
                                        _In_                    const VolumeId                  volumeid,
                                        _In_                    const FileId                    fileid,
                                        _In_reads_( cbGuid )    const BYTE* const               rgbUniqueId,
                                        _Out_                   ICache** const                  ppc )
{
    ERR                     err                                 = JET_errSuccess;
    const DWORD             cwchAnyAbsPathMax                   = OSFSAPI_MAX_PATH;
    WCHAR                   wszAnyAbsPath[ cwchAnyAbsPathMax ]  = { 0 };
    const DWORD             cwchKeyPathMax                      = OSFSAPI_MAX_PATH;
    WCHAR                   wszKeyPath[ cwchKeyPathMax ]        = { 0 };
    VolumeId                volumeidActual                      = volumeidInvalid;
    FileId                  fileidActual                        = fileidInvalid;
    BYTE                    rgbUniqueIdActual[ cbGuid ]         = { 0 };
    const int               cAttemptMax                         = 10;
    int                     cAttempt                            = 0;
    CSemaphore              sem( CSyncBasicInfo( "TCacheRepository<I>::ErrOpenById" ) );
    CCachePathTableEntry*   pcpte                               = NULL;
    CCacheReference*        pcr                                 = NULL;

    *ppc = NULL;

    //  defend against an invalid physical id

    if ( volumeid == volumeidInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( fileid == fileidInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( FUtilZeroed( rgbUniqueId, cbGuid ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  retry until we have opened a cache with the correct file id

    while ( volumeidActual != volumeid || fileidActual != fileid )
    {
        //  if we have a cache open from a previous attempt then close it

        delete *ppc;
        *ppc = NULL;

        //  translate the file id to a file path and the unambiguous file key path

        Call( m_pfident->ErrGetFilePathById( volumeid, fileid, wszAnyAbsPath, wszKeyPath ) );

        //  if we have retried too many times then fail

        if ( ++cAttempt >= cAttemptMax )
        {
            Error( ErrBlockCacheInternalError( wszAnyAbsPath, "CacheOpenByIdRetryLimit" ) );
        }

        //  wait to lock the entry for this cache

        Call( ErrLockCache( wszKeyPath, &sem, &pcpte ) );

        //  if the cache isn't open then open it

        if ( !pcpte->Pc() )
        {
            Call( ErrOpenByIdCacheMiss( pfsf, pfsconfig, pbcconfig, wszAnyAbsPath, pcpte, &pcr ) );
        }

        //  the cache is already open

        else
        {
            //  open the already open cache

            Call( ErrOpenCacheHit( pcpte, &pcr ) );
        }

        //  get the physical id for the cache we opened

        Call( pcr->ErrGetPhysicalId( &volumeidActual, &fileidActual, rgbUniqueIdActual ) );
    }

    //  verify that the cache we opened matches the requested cache

    if ( memcmp( rgbUniqueIdActual, rgbUniqueId, cbGuid ) )
    {
        Error( ErrCachingFileNotFoundById( pfsconfig, volumeid, fileid, rgbUniqueId ) );
    }

    //  return the opened cache

    *ppc = pcr;
    pcr = NULL;

HandleError:
    ReleaseCache( pcpte, &sem );
    delete pcr;
    if ( err < JET_errSuccess )
    {
        delete *ppc;
        *ppc = NULL;

        if ( err == JET_errFileNotFound || err == JET_errInvalidPath )
        {
            err = ErrCachingFileNotFoundById( pfsconfig, volumeid, fileid, rgbUniqueId );
        }
    }
    return err;
}

template< class I >
INLINE TCacheRepository<I>::CCachePathTableEntry::CCachePathTableEntry( _Deref_inout_z_ const WCHAR** const pwszKeyPath )
    :   m_wszKeyPath( *pwszKeyPath ),
        m_uiHash( UiHash( *pwszKeyPath ) ),
        m_cref( 0 ),
        m_psemOwner( NULL ),
        m_pc( NULL )
{
    *pwszKeyPath = NULL;
}

template< class I >
INLINE TCacheRepository<I>::CCachePathTableEntry::~CCachePathTableEntry()
{
    delete[] m_wszKeyPath;
    delete m_pc;
}

template< class I >
INLINE ERR TCacheRepository<I>::CCachePathTableEntry::ErrAddAsOwnerOrWaiter( _In_ CSemaphore* const psem )
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

template< class I >
INLINE void TCacheRepository<I>::CCachePathTableEntry::AddNextOwner()
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

template< class I >
INLINE void TCacheRepository<I>::CCachePathTableEntry::RemoveAsOwnerOrWaiter( _In_opt_ CSemaphore* const psem )
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

template< class I >
INLINE UINT TCacheRepository<I>::CCachePathTableEntry::UiHash( _In_z_ const WCHAR* const wszKeyPath )
{
    const INT cbKeyPath = wszKeyPath ? LOSStrLengthW( wszKeyPath ) * sizeof( wszKeyPath[ 0 ] ) : 0;

    unsigned __int64 ui64Hash = Ui64FNVHash( wszKeyPath, cbKeyPath );

    return (UINT)( ( ui64Hash >> 32 ) | ( ui64Hash & dwMax ) );
}

template< class I >
VOID TCacheRepository<I>::CCachePathTableEntry::OpenCache( _Inout_ ICache** const ppc )
{
    m_pc = *ppc;
    *ppc = NULL;
}

template< class I >
void TCacheRepository<I>::ReleaseCache( _In_opt_    CCachePathTableEntry* const pcpte,
                                        _In_opt_    CSemaphore* const           psem,
                                        _In_opt_    CCacheReference* const      pcr )
{
    BOOL fDeleteCacheEntry = fFalse;

    if ( !pcpte )
    {
        return;
    }

    m_critCachePathTableEntries.Enter();

    if ( pcr )
    {
        pcpte->RemoveReference( pcr );
    }

    pcpte->RemoveAsOwnerOrWaiter( psem );

    if ( fDeleteCacheEntry = pcpte->FRelease() )
    {
        m_ilCachePathTableEntries.Remove( pcpte );
    }
    else
    {
        pcpte->AddNextOwner();
    }

    m_critCachePathTableEntries.Leave();

    if ( fDeleteCacheEntry )
    {
        delete pcpte;
    }
}

template< class I >
void TCacheRepository<I>::CloseAllCaches()
{
    while ( !m_ilCachePathTableEntries.FEmpty() )
    {
        CCachePathTableEntry* const pcpte = m_ilCachePathTableEntries.PrevMost();

        m_ilCachePathTableEntries.Remove( pcpte );
        delete pcpte;
    }
}

template< class I >
ERR TCacheRepository<I>::ErrLockCache(  _In_z_  const WCHAR* const              wszKeyPath,
                                        _In_    CSemaphore* const               psem,
                                        _Out_   CCachePathTableEntry** const    ppcpte )
{
    ERR                     err             = JET_errSuccess;
    BOOL                    fLeave          = fFalse;
    CCachePathTableEntry*   pcpteExisting   = NULL;
    UINT                    uiHash          = CCachePathTableEntry::UiHash( wszKeyPath );
    int                     cwchKeyPath     = 0;
    WCHAR*                  wszKeyPathCopy  = NULL;
    CCachePathTableEntry*   pcpteNew        = NULL;
    BOOL                    fRemove         = fFalse;

    *ppcpte = NULL;

    m_critCachePathTableEntries.Enter();
    fLeave = fTrue;

    for (   pcpteExisting = m_ilCachePathTableEntries.PrevMost();
            (   pcpteExisting != NULL
                && !(   pcpteExisting->UiHash() == uiHash
                        && LOSStrCompareW( pcpteExisting->WszKeyPath(), wszKeyPath ) == 0 ) );
            pcpteExisting = m_ilCachePathTableEntries.Next( pcpteExisting ) )
    {
    }

    if ( !pcpteExisting )
    {
        cwchKeyPath = LOSStrLengthW( wszKeyPath ) + 1;
        Alloc( wszKeyPathCopy = new WCHAR[ cwchKeyPath ] );
        Call( ErrOSStrCbCopyW( wszKeyPathCopy, cwchKeyPath * sizeof( WCHAR ), wszKeyPath ) );

        Alloc( pcpteExisting = pcpteNew = new CCachePathTableEntry( const_cast<const WCHAR**>( &wszKeyPathCopy ) ) );

        m_ilCachePathTableEntries.InsertAsPrevMost( pcpteNew );
        fRemove = fTrue;
    }

    Call( pcpteExisting->ErrAddAsOwnerOrWaiter( psem ) );

    pcpteExisting->AddRef();
    fRemove = fFalse;
    pcpteNew = NULL;

    m_critCachePathTableEntries.Leave();
    fLeave = fFalse;

    *ppcpte = pcpteExisting;
    psem->Wait();

HandleError:
    if ( fRemove )
    {
        m_ilCachePathTableEntries.Remove( pcpteNew );
    }
    delete pcpteNew;
    if ( fLeave )
    {
        m_critCachePathTableEntries.Leave();
    }
    delete[] wszKeyPathCopy;
    if ( err < JET_errSuccess )
    {
        *ppcpte = NULL;
    }
    return err;
}

template< class I >
ERR TCacheRepository<I>::ErrOpenCacheMiss(  _In_    IFileSystemFilter* const        pfsf,
                                            _In_    IFileSystemConfiguration* const pfsconfig,
                                            _Inout_ ICacheConfiguration** const     ppcconfig,
                                            _In_z_  const WCHAR* const              wszAbsPath,
                                            _In_    CCachePathTableEntry* const     pcpte,
                                            _Out_   CCacheReference** const         ppcr )
{
    ERR             err         = JET_errSuccess;
    const int       cAttemptMax = 10;
    int             cAttempt    = 0;
    IFileFilter*    pff         = NULL;
    BOOL            fCreated    = fFalse;

    *ppcr = NULL;

    //  retry until we have created/opened the file backing the cache

    err = JET_errFileAlreadyExists;  //  ErrERRCheck( JET_errFileAlreadyExists ) not appropriate here
    while ( err == JET_errFileAlreadyExists )
    {
        //  if we have retried too many times then fail

        if ( ++cAttempt >= cAttemptMax )
        {
            Error( ErrBlockCacheInternalError( wszAbsPath, "CacheOpenRetryLimit" ) );
        }

        //  try to open the file

        err = pfsf->ErrFileOpen( wszAbsPath, IFileAPI::fmfStorageWriteBack, (IFileAPI**)&pff );

        //  if we couldn't find the file then try to create it

        if ( err == JET_errFileNotFound )
        {
            err = pfsf->ErrFileCreate( wszAbsPath, IFileAPI::fmfStorageWriteBack, (IFileAPI**)&pff );
            fCreated = err >= JET_errSuccess;
        }
    }
    Call( err );

    //  mount the cache

    Call( ErrMountCache( pcpte, pfsf, pfsconfig, ppcconfig, &pff, fCreated ) );

    //  provide a wrapper for the cache that will release it on the last close

    Alloc( *ppcr = new CCacheReference( this, pcpte ) );

HandleError:
    delete pff;
    if ( err < JET_errSuccess )
    {
        delete *ppcr;
        *ppcr = NULL;
        if ( fCreated )
        {
            CallS( pfsf->ErrFileDelete( wszAbsPath ) );
        }
    }
    return err;
}

template< class I >
ERR TCacheRepository<I>::ErrOpenByIdCacheMiss(  _In_    IFileSystemFilter* const        pfsf,
                                                _In_    IFileSystemConfiguration* const pfsconfig,
                                                _In_    IBlockCacheConfiguration* const pbcconfig,
                                                _In_z_  const WCHAR* const              wszAnyAbsPath,
                                                _In_    CCachePathTableEntry* const     pcpte,
                                                _Out_   CCacheReference** const         ppcr )
{
    ERR                     err         = JET_errSuccess;
    IFileFilter*            pff         = NULL;
    ICacheConfiguration*    pcconfig    = NULL;

    *ppcr = NULL;

    //  open the file backing the cache

    Call( pfsf->ErrFileOpen( wszAnyAbsPath, IFileAPI::fmfStorageWriteBack, (IFileAPI**)&pff ) );

    //  get the cache configuration

    Call( pbcconfig->ErrGetCacheConfiguration( pcpte->WszKeyPath(), &pcconfig ) );

    //  mount the cache

    Call( ErrMountCache( pcpte, pfsf, pfsconfig, &pcconfig, &pff, fFalse) );

    //  provide a wrapper for the cache that will release it on the last close

    Alloc( *ppcr = new CCacheReference( this, pcpte ) );

HandleError:
    delete pcconfig;
    delete pff;
    if ( err < JET_errSuccess )
    {
        delete *ppcr;
        *ppcr = NULL;
    }
    return err;
}

template< class I >
ERR TCacheRepository<I>::ErrOpenCacheHit( _In_ CCachePathTableEntry* const pcpte, _Out_ CCacheReference** const ppcr )
{
    ERR err = JET_errSuccess;

    *ppcr = NULL;

    //  provide a wrapper for the cache that will release it on the last close

    Alloc( *ppcr = new CCacheReference( this, pcpte ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        delete *ppcr;
        *ppcr = NULL;
    }
    return err;
}

template< class I >
ERR TCacheRepository<I>::ErrMountCache( _In_    CCachePathTableEntry* const     pcpte,
                                        _In_    IFileSystemFilter* const        pfsf,
                                        _In_    IFileSystemConfiguration* const pfsconfig,
                                        _Inout_ ICacheConfiguration** const     ppcconfig,
                                        _Inout_ IFileFilter** const             ppff,
                                        _In_    const BOOL                      fCreate )
{
    ERR     err = JET_errSuccess;
    ICache* pc  = NULL;

    //  create / mount the cache

    if ( fCreate )
    {
        Call( CCacheFactory::ErrCreate( pfsf, m_pfident, pfsconfig, ppcconfig, m_pctm, ppff, &pc ) );
    }
    else
    {
        Call( CCacheFactory::ErrMount( pfsf, m_pfident, pfsconfig, ppcconfig, m_pctm, ppff, &pc ) );
    }

    //  make the cache available for other opens

    pcpte->OpenCache( &pc );

HandleError:
    delete pc;
    return err;
}

template<class I>
ERR TCacheRepository<I>::ErrCachingFileNotFoundById(    _In_                IFileSystemConfiguration* const pfsconfig,
                                                        _In_                const VolumeId                  volumeid,
                                                        _In_                const FileId                    fileid, 
                                                        _In_reads_(cbGuid)  const BYTE* const               rgbUniqueId )
{
    const ULONG     cwsz                = 3;
    const WCHAR*    rgpwsz[ cwsz ]      = { 0 };
    DWORD           irgpwsz             = 0;
    WCHAR           wszVolumeId[ 64 ]   = { 0 };
    WCHAR           wszFileId[ 64 ]     = { 0 };
    WCHAR           wszUniqueId[ 64 ]   = { 0 };

    OSStrCbFormatW( wszVolumeId, sizeof( wszVolumeId ), L"0x%08x", volumeid );
    OSStrCbFormatW( wszFileId, sizeof( wszFileId ), L"0x%016I64x", fileid );
    OSStrCbFormatW( wszUniqueId,
                    sizeof( wszUniqueId ),
                    L"%08lx-%04hx-%04hx-%02hx%02hx-%02hx%02hx%02hx%02hx%02hx%02hx",
                    *((DWORD*)&rgbUniqueId[ 0 ]),
                    *((WORD*)&rgbUniqueId[ 4 ]),
                    *((WORD*)&rgbUniqueId[ 6 ]),
                    *((BYTE*)&rgbUniqueId[ 8 ]),
                    *((BYTE*)&rgbUniqueId[ 9 ]),
                    *((BYTE*)&rgbUniqueId[ 10 ]),
                    *((BYTE*)&rgbUniqueId[ 11 ]),
                    *((BYTE*)&rgbUniqueId[ 12 ]),
                    *((BYTE*)&rgbUniqueId[ 13 ]),
                    *((BYTE*)&rgbUniqueId[ 14 ]),
                    *((BYTE*)&rgbUniqueId[ 15 ]) );

    rgpwsz[ irgpwsz++ ] = wszVolumeId;
    rgpwsz[ irgpwsz++ ] = wszFileId;
    rgpwsz[ irgpwsz++ ] = wszUniqueId;

    pfsconfig->EmitEvent(   eventError,
                            BLOCK_CACHE_CATEGORY,
                            BLOCK_CACHE_CACHING_FILE_ID_MISMATCH_ID,
                            irgpwsz,
                            rgpwsz,
                            JET_EventLoggingLevelMin );

    return ErrERRCheck( JET_errDiskIO );
}

//  CCacheRepository:  concrete TCacheRepository<ICacheRepository>

class CCacheRepository : public TCacheRepository<ICacheRepository>
{
    public:

        CCacheRepository( _In_ IFileIdentification* const pfident, _In_ ICacheTelemetry* const pctm )
            :   TCacheRepository<ICacheRepository>( pfident, pctm )
        {
        }

        virtual ~CCacheRepository() {}
};

//  CCacheReference:  a reference to an ICache implementation.

class CCacheReference : public TCacheWrapper<ICache>
{
    public:  //  specialized API

        CCacheReference(    _In_ TCacheRepository<ICacheRepository>* const                          pcrep,
                            _In_ TCacheRepository<ICacheRepository>::CCachePathTableEntry* const    pcpte )
            :   TCacheWrapper<ICache>( pcpte->Pc() ),
                m_pcrep( pcrep ),
                m_pcpte( pcpte )
        {
            m_pcrep->AddCacheReference( this, m_pcpte );
        }

        virtual ~CCacheReference()
        {
            m_pcrep->RemoveCacheReference( this, m_pcpte );
        }

    private:

        friend SIZE_T CCacheReferenceOffsetOfILE();

        TCacheRepository<ICacheRepository>* const                               m_pcrep;
        TCacheRepository<ICacheRepository>::CCachePathTableEntry* const         m_pcpte;
        CInvasiveList< CCacheReference, CCacheReferenceOffsetOfILE >::CElement  m_ile;
};

static SIZE_T CCacheReferenceOffsetOfILE() { return OffsetOf( CCacheReference, m_ile ); }
