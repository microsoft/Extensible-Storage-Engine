// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  TCacheBase:  base implementation of ICache and its derivatives.
//
//  I:  ICache or derivative
//  CFTE:  CCachedFileTableEntryBase or derivative
//  CTLS:  CCacheThreadLocalStorageBase or derivative

template< class I, class CFTE, class CTLS >
class TCacheBase  //  c
    :   public I
{
    public:

        virtual ~TCacheBase();

    public:  //  ICache

        ERR ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override;

        ERR ErrGetPhysicalId(   _Out_                   VolumeId* const pvolumeid,
                                _Out_                   FileId* const   pfileid,
                                _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId ) override;
        
        ERR ErrClose(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

    protected:
        
        TCacheBase( _In_    IFileSystemFilter* const            pfsf,
                    _In_    IFileIdentification* const          pfident,
                    _In_    IFileSystemConfiguration* const     pfsconfig, 
                    _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                    _Inout_ ICacheConfiguration** const         ppcconfig,
                    _In_    ICacheTelemetry* const              pctm,
                    _Inout_ IFileFilter** const                 ppffCaching,
                    _Inout_ CCacheHeader** const                ppch );

        IFileSystemFilter* Pfsf() const { return m_pfsf; }
        IFileIdentification* Pfident() const { return m_pfident; }
        IFileSystemConfiguration* Pfsconfig() const { return m_pfsconfig; }
        IBlockCacheConfiguration* Pbcconfig() const { return m_pbcconfig; }
        ICacheConfiguration* Pcconfig() const { return m_pcconfig; }
        IFileFilter* PffCaching() const { return m_pffCaching; }
        
        ERR ErrGetCachedFile(   _In_    const VolumeId      volumeid,
                                _In_    const FileId        fileid,
                                _In_    const FileSerial    fileserial,
                                _In_    const BOOL          fInternalOpen,
                                _Out_   CFTE** const        ppcfte );
        void ReleaseCachedFile( _Inout_ CFTE** const ppcfte );

        virtual ERR ErrGetThreadLocalStorage( _Out_ CTLS** const ppctls );

        //  Reports a cache miss for a file/block and if that block should be cached.

        void ReportMiss(    _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData,
                            _In_ const BOOL     fRead,
                            _In_ const BOOL     fCacheIfPossible );

        //  Reports a cache hit for a file/block and if that block should be cached.

        void ReportHit( _In_ CFTE* const    pcfte,
                        _In_ const QWORD    ibOffset,
                        _In_ const DWORD    cbData,
                        _In_ const BOOL     fRead,
                        _In_ const BOOL     fCacheIfPossible );

        //  Reports an update of a file/block.

        void ReportUpdate(  _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData );

        //  Reports a write of a file/block and if that write was due to limited resources.

        void ReportWrite(   _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData,
                            _In_ const BOOL     fReplacementPolicy );

        //  Reports the removal of a file/block from the cache and if that removal was due to limited resources.

        void ReportEvict(   _In_ CFTE* const    pcfte,
                            _In_ const QWORD    ibOffset,
                            _In_ const DWORD    cbData,
                            _In_ const BOOL     fReplacementPolicy );

    private:

        BOOL FCachedFileTableIsInit() const { return m_initOnceCachedFileTable.FIsInit(); }
        ERR ErrEnsureInitCachedFileTable() { return m_initOnceCachedFileTable.Init( ErrInitCachedFileTable_, this ); };
        ERR ErrInitCachedFileTable();
        static ERR ErrInitCachedFileTable_( _In_ TCacheBase<I, CFTE, CTLS>* const pc ) { return pc->ErrInitCachedFileTable(); }
        void TermCachedFileTable();

        ERR ErrGetCachedFileTableEntry( _In_    const VolumeId      volumeid,
                                        _In_    const FileId        fileid,
                                        _In_    const FileSerial    fileserial,
                                        _In_    const BOOL          fInternalOpen,
                                        _Out_   CFTE** const        ppcfte );
        ERR ErrGetCachedFileTableEntrySlowly(   _In_    const VolumeId      volumeid,
                                                _In_    const FileId        fileid,
                                                _In_    const FileSerial    fileserial,
                                                _In_    const BOOL          fInternalOpen,
                                                _Out_   CFTE** const        ppcfte );
        VOID ReleaseCachedFileSlowly( _In_ CFTE* const pcfte );
        VOID ExternalCloseCachedFile(   _In_    const VolumeId      volumeid,
                                        _In_    const FileId        fileid,
                                        _In_    const FileSerial    fileserial );

    protected:

        //  Request context.

        class CRequest
        {
            public:

                CRequest(   _In_                    const BOOL                          fHeap,
                            _In_                    const BOOL                          fRead,
                            _In_                    TCacheBase<I, CFTE, CTLS>* const    pc,
                            _In_                    const TraceContext&                 tc,
                            _Inout_                 CFTE** const                        ppcfte,
                            _In_                    const QWORD                         ibOffset,
                            _In_                    const DWORD                         cbData,
                            _In_reads_( cbData )    const BYTE* const                   pbData,
                            _In_                    const OSFILEQOS                     grbitQOS,
                            _In_                    const ICache::CachingPolicy         cp,
                            _In_                    const ICache::PfnComplete           pfnComplete,
                            _In_                    const DWORD_PTR                     keyComplete )
                    :   m_fHeap( fHeap ),
                        m_fRead( fRead ),
                        m_pc( pc ),
                        m_pcfte( *ppcfte ),
                        m_ibOffset( ibOffset ),
                        m_cbData( cbData ),
                        m_pbData( pbData ),
                        m_grbitQOS( grbitQOS ),
                        m_cp( cp ),
                        m_pfnComplete( pfnComplete ),
                        m_keyComplete( keyComplete ),
                        m_cref( 1 ),
                        m_err( JET_errSuccess ),
                        m_grbitQOSComplete( 0 )
                {
                    GetCurrUserTraceContext getutc;
                    m_ftc.DeepCopy( getutc.Utc(), tc );

                    *ppcfte = NULL;
                }

                BOOL FRead() const { return m_fRead; }
                TCacheBase<I, CFTE, CTLS>* Pc() const { return m_pc; }
                CFTE* Pcfte() const { return m_pcfte; }
                QWORD IbOffset() const { return m_ibOffset; }
                DWORD CbData() const { return m_cbData; }
                const BYTE* const PbData() const { return m_pbData; }
                ICache::CachingPolicy Cp() const { return m_cp; }
                BOOL FSync() const { return m_pfnComplete == NULL; }

                void Release( _In_ const ERR err )
                {
                    Finish( err );
                }

                ERR ErrRead(    _In_                    IFileFilter* const          pff,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _Out_writes_( cbData )  BYTE* const                 pbData,
                                _In_                    const IFileFilter::IOMode   iom )
                {
                    ERR err = JET_errSuccess;

                    Call( pff->ErrRead( m_ftc.etc,
                                        ibOffset,
                                        cbData,
                                        pbData,
                                        m_grbitQOS,
                                        iom,
                                        FSync() ? NULL : IOComplete_, 
                                        DWORD_PTR( this ),
                                        FSync() ? NULL : IOHandoff_,
                                        NULL ) );

                HandleError:
                    return err;
                }

                ERR ErrWrite(   _In_                    IFileFilter* const          pff,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _In_reads_( cbData )    const BYTE* const           pbData,
                                _In_                    const IFileFilter::IOMode   iom )
                {
                    ERR err = JET_errSuccess;

                    Call( pff->ErrWrite(    m_ftc.etc,
                                            ibOffset,
                                            cbData,
                                            pbData,
                                            m_grbitQOS,
                                            iom,
                                            FSync() ? NULL : IOComplete_, 
                                            DWORD_PTR( this ),
                                            FSync() ? NULL : IOHandoff_ ) );

                HandleError:
                    return err;
                }

            protected:

                virtual ~CRequest()
                {
                    m_pc->ReleaseCachedFile( &m_pcfte );
                }

                virtual void Start()
                {
                    AddRef();
                }
                
                virtual void Finish( _In_ const ERR err )
                {
                    RecordResult( err );

                    if ( FRelease() )
                    {
                        Complete();
                    }
                }

            private:

                CRequest( const CRequest& other ) = delete;

                void AddRef() { AtomicIncrement( (LONG*)&m_cref ); }
                BOOL FRelease() { return AtomicDecrement( (LONG*)&m_cref ) == 0; }

                void RecordResult( _In_ const ERR err )
                {
                    AtomicCompareExchange( (LONG*)&m_err, JET_errSuccess, err );
                }

                void Complete()
                {
                    if ( m_pfnComplete )
                    {
                        m_pfnComplete(  m_err,
                                        m_pcfte->Volumeid(),
                                        m_pcfte->Fileid(),
                                        m_pcfte->Fileserial(),
                                        m_ftc,
                                        ( m_grbitQOS & ~qosIOCompleteMask ) | m_grbitQOSComplete,
                                        m_ibOffset,
                                        m_cbData,
                                        m_pbData,
                                        m_keyComplete );
                    }

                    Release();
                }

                void Release()
                {
                    if ( m_fHeap )
                    {
                        delete this;
                    }
                    else
                    {
                        this->~CRequest();
                    }
                }

                static void IOComplete_(    _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete )
                {
                    IFileAPI::PfnIOComplete pfnIOComplete = IOComplete_;
                    Unused( pfnIOComplete );

                    CRequest* const prequest = (CRequest*)keyIOComplete;
                    prequest->m_grbitQOSComplete |= grbitQOS & qosIOCompleteMask;
                    prequest->Finish( err );
                }

                static void IOHandoff_( _In_                    const ERR               err,
                                        _In_                    IFileAPI* const         pfapi,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyIOComplete,
                                        _In_                    void* const             pvIOContext )
                {
                    IFileAPI::PfnIOHandoff pfnIOHandoff = IOHandoff_;
                    Unused( pfnIOHandoff );

                    CRequest* const prequest = (CRequest*)keyIOComplete;
                    prequest->Start();
                }

            private:

                const BOOL                          m_fHeap;
                const BOOL                          m_fRead;
                TCacheBase<I, CFTE, CTLS>* const    m_pc;
                CFTE*                               m_pcfte;
                const QWORD                         m_ibOffset;
                const DWORD                         m_cbData;
                const BYTE* const                   m_pbData;
                const OSFILEQOS                     m_grbitQOS;
                const ICache::CachingPolicy         m_cp;
                const ICache::PfnComplete           m_pfnComplete;
                const DWORD_PTR                     m_keyComplete;
                FullTraceContext                    m_ftc;
                int                                 m_cref;
                ERR                                 m_err;
                OSFILEQOS                           m_grbitQOSComplete;
        };

    private:

        typedef CInitOnce<ERR, decltype( &ErrInitCachedFileTable_ ), TCacheBase<I, CFTE, CTLS>* const> CInitOnceCachedFileTable;

        IFileSystemFilter* const                                                m_pfsf;
        IFileIdentification* const                                              m_pfident;
        IFileSystemConfiguration* const                                         m_pfsconfig;
        IBlockCacheConfiguration* const                                         m_pbcconfig;
        ICacheConfiguration* const                                              m_pcconfig;
        ICacheTelemetry* const                                                  m_pctm;
        IFileFilter* const                                                      m_pffCaching;
        CCacheHeader* const                                                     m_pch;
        CInitOnceCachedFileTable                                                m_initOnceCachedFileTable;
        CCachedFileHash                                                         m_cachedFileHash;
        TCacheThreadLocalStorage<CTLS>                                          m_cacheThreadLocalStorage;
};

template< class I, class CFTE, class CTLS >
TCacheBase<I, CFTE, CTLS>::~TCacheBase()
{
    delete m_pch;
    delete m_pffCaching;
    delete m_pcconfig;
    delete m_pbcconfig;
    TermCachedFileTable();
}

template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType )
{
    memcpy( rgbCacheType, m_pch->RgbCacheType(), cbGuid );

    return JET_errSuccess;
}

template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrGetPhysicalId(    _Out_                   VolumeId* const pvolumeid,
                                                    _Out_                   FileId* const   pfileid,
                                                    _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId )
{
    *pvolumeid = m_pch->Volumeid();
    *pfileid = m_pch->Fileid();
    memcpy( rgbUniqueId, m_pch->RgbUniqueId(), cbGuid );

    return JET_errSuccess;
}
                
template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrClose(    _In_ const VolumeId     volumeid,
                                            _In_ const FileId       fileid,
                                            _In_ const FileSerial   fileserial ) 
{
    //  close the external open on the file, quiesce future internal opens, and wait until all pending opens are closed
    //  or until a new external open occurs

    ExternalCloseCachedFile( volumeid, fileid, fileserial );

    return JET_errSuccess;
}

template< class I, class CFTE, class CTLS >
TCacheBase<I, CFTE, CTLS>::TCacheBase(  _In_    IFileSystemFilter* const            pfsf,
                                        _In_    IFileIdentification* const          pfident,
                                        _In_    IFileSystemConfiguration* const     pfsconfig, 
                                        _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                                        _Inout_ ICacheConfiguration** const         ppcconfig,
                                        _In_    ICacheTelemetry* const              pctm,
                                        _Inout_ IFileFilter** const                 ppffCaching,
                                        _Inout_ CCacheHeader** const                ppch )
    :   m_pfsf( pfsf ),
        m_pfident( pfident ),
        m_pfsconfig( pfsconfig ),
        m_pbcconfig( *ppbcconfig ),
        m_pcconfig( *ppcconfig ),
        m_pctm( pctm ),
        m_pffCaching( *ppffCaching ),
        m_pch( *ppch ),
        m_cachedFileHash( rankCachedFileHash )
{
    *ppbcconfig = NULL;
    *ppcconfig = NULL;
    *ppffCaching = NULL;
    *ppch = NULL;
}
        
template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrGetCachedFile(    _In_    const VolumeId      volumeid,
                                                    _In_    const FileId        fileid,
                                                    _In_    const FileSerial    fileserial,
                                                    _In_    const BOOL          fInternalOpen,
                                                    _Out_   CFTE** const        ppcfte )
{
    ERR     err     = JET_errSuccess;
    CFTE*   pcfte   = NULL;

    *ppcfte = NULL;

    //  create/open the cached file table entry for this file

    Call( ErrGetCachedFileTableEntry( volumeid, fileid, fileserial, fInternalOpen, &pcfte ) );

    //  ensure that the cached file is open

    Call( pcfte->ErrEnsureOpenCacheFile( m_pfsf, m_pfident, m_pbcconfig, m_pffCaching ) );

    //  return the cached file

    *ppcfte = pcfte;
    pcfte = NULL;

HandleError:
    ReleaseCachedFile( &pcfte );
    if ( err < JET_errSuccess )
    {
        ReleaseCachedFile( ppcfte );
    }
    return err;
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::ReleaseCachedFile( _Inout_ CFTE** const ppcfte )
{
    CFTE* const pcfte = *ppcfte;

    *ppcfte = NULL;

    if ( !pcfte )
    {
        return;
    }

    if ( pcfte->FReleaseIfNotLastReference() )
    {
        return;
    }

    ReleaseCachedFileSlowly( pcfte );
}

template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrGetThreadLocalStorage( _Out_ CTLS** const ppctls )
{
    return m_cacheThreadLocalStorage.ErrGetThreadLocalStorage( ppctls );
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::ReportMiss( _In_ CFTE* const    pcfte,
                                            _In_ const QWORD    ibOffset,
                                            _In_ const DWORD    cbData,
                                            _In_ const BOOL     fRead,
                                            _In_ const BOOL     fCacheIfPossible )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber++ )
    {
        m_pctm->Miss( filenumber, blocknumber, fRead, fCacheIfPossible );
    }
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::ReportHit(  _In_ CFTE* const    pcfte,
                                            _In_ const QWORD    ibOffset,
                                            _In_ const DWORD    cbData,
                                            _In_ const BOOL     fRead,
                                            _In_ const BOOL     fCacheIfPossible )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber++ )
    {
        m_pctm->Hit( filenumber, blocknumber, fRead, fCacheIfPossible );
    }
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::ReportUpdate(   _In_ CFTE* const    pcfte,
                                                _In_ const QWORD    ibOffset,
                                                _In_ const DWORD    cbData )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber++ )
    {
        m_pctm->Update( filenumber, blocknumber );
    }
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::ReportWrite(    _In_ CFTE* const    pcfte,
                                                _In_ const QWORD    ibOffset,
                                                _In_ const DWORD    cbData,
                                                _In_ const BOOL     fReplacementPolicy )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber++ )
    {
        m_pctm->Write( filenumber, blocknumber, fReplacementPolicy );
    }
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::ReportEvict(    _In_ CFTE* const    pcfte,
                                                _In_ const QWORD    ibOffset,
                                                _In_ const DWORD    cbData,
                                                _In_ const BOOL     fReplacementPolicy )
{
    const ICacheTelemetry::FileNumber filenumber = pcfte->Filenumber();
    const ICacheTelemetry::BlockNumber blocknumberMax = pcfte->Blocknumber( ibOffset + cbData + pcfte->CbBlockSize() - 1 );
    for (   ICacheTelemetry::BlockNumber blocknumber = pcfte->Blocknumber( ibOffset );
            blocknumber < blocknumberMax;
            blocknumber++ )
    {
        m_pctm->Evict( filenumber, blocknumber, fReplacementPolicy );
    }
}

template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrInitCachedFileTable()
{
    ERR err = JET_errSuccess;

    if ( m_cachedFileHash.ErrInit( 5.0, 1.0 ) == CCachedFileHash::ERR::errOutOfMemory )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

template< class I, class CFTE, class CTLS >
void TCacheBase<I, CFTE, CTLS>::TermCachedFileTable()
{
    if ( FCachedFileTableIsInit() )
    {
        CCachedFileHash::CLock  lock;
        CCachedFileEntry        entry;
        CCachedFileHash::ERR    errCachedFileHash;

        m_cachedFileHash.BeginHashScan( &lock );

        while ( ( errCachedFileHash = m_cachedFileHash.ErrMoveNext( &lock ) ) != CCachedFileHash::ERR::errNoCurrentEntry )
        {
            errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
            Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );

            delete entry.Pcfte();
        }

        m_cachedFileHash.EndHashScan( &lock );

        m_cachedFileHash.Term();
    }
}

template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrGetCachedFileTableEntry(  _In_    const VolumeId      volumeid,
                                                            _In_    const FileId        fileid,
                                                            _In_    const FileSerial    fileserial,
                                                            _In_    const BOOL          fInternalOpen,
                                                            _Out_   CFTE** const        ppcfte )
{
    ERR                     err                 = JET_errSuccess;
    CCachedFileKey          key( volumeid, fileid, fileserial );
    CCachedFileEntry        entry;
    CCachedFileHash::CLock  lock;
    CCachedFileHash::ERR    errCachedFileHash   = CCachedFileHash::ERR::errSuccess;
    BOOL                    fReadLocked         = fFalse;
    CFTE*                   pcfteExisting       = NULL;

    *ppcfte = NULL;

    Call( ErrEnsureInitCachedFileTable() );

    m_cachedFileHash.ReadLockKey( key, &lock );
    fReadLocked = fTrue;

    errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
    if ( errCachedFileHash == CCachedFileHash::ERR::errSuccess && !entry.Pcfte()->FWaitersForExternalClose() )
    {
        pcfteExisting = static_cast<CFTE*>( entry.Pcfte() );

        pcfteExisting->AddRef();

        if ( !fInternalOpen )
        {
            pcfteExisting->SetExternalOpen();
        }
    }
    else
    {
        Assert( errCachedFileHash == CCachedFileHash::ERR::errEntryNotFound );
    }

    if ( fReadLocked )
    {
        m_cachedFileHash.ReadUnlockKey( &lock );
        fReadLocked = fFalse;
    }

    *ppcfte = pcfteExisting;
    pcfteExisting = NULL;

    if ( !(*ppcfte) )
    {
        Call( ErrGetCachedFileTableEntrySlowly( volumeid, fileid, fileserial, fInternalOpen, ppcfte ) );
    }

HandleError:
    if ( fReadLocked )
    {
        m_cachedFileHash.ReadUnlockKey( &lock );
    }
    ReleaseCachedFile( &pcfteExisting );
    if ( err < JET_errSuccess )
    {
        ReleaseCachedFile( ppcfte );
    }
    return err;
}

template< class I, class CFTE, class CTLS >
ERR TCacheBase<I, CFTE, CTLS>::ErrGetCachedFileTableEntrySlowly(    _In_    const VolumeId      volumeid,
                                                                    _In_    const FileId        fileid,
                                                                    _In_    const FileSerial    fileserial,
                                                                    _In_    const BOOL          fInternalOpen,
                                                                    _Out_   CFTE** const        ppcfte )
{
    ERR                     err                 = JET_errSuccess;
    CCachedFileKey          key( volumeid, fileid, fileserial );
    CCachedFileEntry        entry;
    CCachedFileHash::CLock  lock;
    CCachedFileHash::ERR    errCachedFileHash   = CCachedFileHash::ERR::errSuccess;
    CFTE*                   pcfteExisting       = NULL;
    BOOL                    fWriteLocked        = fFalse;
    CFTE*                   pcfteNew            = NULL;
    BOOL                    fRemove             = fFalse;

    *ppcfte = NULL;

    m_cachedFileHash.WriteLockKey( key, &lock );
    fWriteLocked = fTrue;

    errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
    if ( errCachedFileHash == CCachedFileHash::ERR::errSuccess )
    {
        pcfteExisting = static_cast<CFTE*>( entry.Pcfte() );
    }
    else
    {
        Assert( errCachedFileHash == CCachedFileHash::ERR::errEntryNotFound );

        Alloc( pcfteExisting = pcfteNew = new CFTE( volumeid, fileid, fileserial ) );

        entry = CCachedFileEntry( pcfteNew );
        errCachedFileHash = m_cachedFileHash.ErrInsertEntry( &lock, entry );
        if ( errCachedFileHash == CCachedFileHash::ERR::errOutOfMemory )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
        fRemove = fTrue;
    }

    pcfteExisting->AddRef();
    fRemove = fFalse;
    pcfteNew = NULL;

    //  if this is an external open then make sure an additional reference is added for that external open to keep the
    //  cached file open until it is explicitly closed.  if someone is already waiting for an external close then
    //  release them to avoid starving their wait but they will fail if they expected the file to be closed.  this is
    //  better than a hang and the presumption is that no one should be trying to close a file with new IO coming in

    if ( !fInternalOpen )
    {
        pcfteExisting->SetExternalOpen();

        if ( pcfteExisting->FWaitersForExternalClose() )
        {
            pcfteExisting->ReleaseWaitersForExternalClose();
        }
    }

    //  if this is an internal open and someone is waiting for an external close then reject the internal open to
    //  facilitate completion of the close.  the internal opens are only used for cache write back which can be
    //  stalled for a bit

    else
    {
        if ( pcfteExisting->FWaitersForExternalClose() )
        {
            Error( ErrERRCheck( errDiskTilt ) );
        }
    }

    if ( fWriteLocked )
    {
        m_cachedFileHash.WriteUnlockKey( &lock );
        fWriteLocked = fFalse;
    }

    *ppcfte = pcfteExisting;

HandleError:
    if ( fRemove )
    {
        errCachedFileHash = m_cachedFileHash.ErrDeleteEntry( &lock );
        Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
    }
    delete pcfteNew;
    if ( fWriteLocked )
    {
        m_cachedFileHash.WriteUnlockKey( &lock );
    }
    if ( err < JET_errSuccess )
    {
        ReleaseCachedFile( ppcfte );
    }
    return err;
}

template< class I, class CFTE, class CTLS >
VOID TCacheBase<I, CFTE, CTLS>::ReleaseCachedFileSlowly( _In_ CFTE* const pcfte )
{
    BOOL fDeleteCachedFileTableEntry = fFalse;

    CCachedFileKey          key( pcfte );
    CCachedFileHash::CLock  lock;
    m_cachedFileHash.WriteLockKey( key, &lock );

    if ( fDeleteCachedFileTableEntry = pcfte->FRelease() )
    {
        CCachedFileHash::ERR errCachedFileHash = m_cachedFileHash.ErrDeleteEntry( &lock );
        Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
    }

    m_cachedFileHash.WriteUnlockKey( &lock );

    if ( fDeleteCachedFileTableEntry )
    {
        delete pcfte;
    }
}

template< class I, class CFTE, class CTLS >
VOID TCacheBase<I, CFTE, CTLS>::ExternalCloseCachedFile(    _In_    const VolumeId      volumeid,
                                                            _In_    const FileId        fileid,
                                                            _In_    const FileSerial    fileserial )
{
    CCachedFileKey          key( volumeid, fileid, fileserial );
    CCachedFileEntry        entry;
    CCachedFileHash::CLock  lock;
    BOOL                    fWriteLocked                = fFalse;
    CCachedFileHash::ERR    errCachedFileHash           = CCachedFileHash::ERR::errSuccess;
    CFTE*                   pcfteExisting               = NULL;
    BOOL                    fDeleteCachedFileTableEntry = fFalse;
    CFTE::CWaiter           waiter;

    if ( FCachedFileTableIsInit() )
    {
        m_cachedFileHash.WriteLockKey( key, &lock );
        fWriteLocked = fTrue;

        errCachedFileHash = m_cachedFileHash.ErrRetrieveEntry( &lock, &entry );
        if ( errCachedFileHash == CCachedFileHash::ERR::errSuccess )
        {
            pcfteExisting = static_cast<CFTE*>( entry.Pcfte() );

            pcfteExisting->AddAsWaiterForExternalClose( &waiter );

            if ( fDeleteCachedFileTableEntry = pcfteExisting->FResetExternalOpen() )
            {
                errCachedFileHash = m_cachedFileHash.ErrDeleteEntry( &lock );
                Assert( errCachedFileHash == CCachedFileHash::ERR::errSuccess );
            }
        }
        else
        {
            Assert( errCachedFileHash == CCachedFileHash::ERR::errEntryNotFound );
        }
    }

    if ( fWriteLocked )
    {
        m_cachedFileHash.WriteUnlockKey( &lock );
    }
    if ( fDeleteCachedFileTableEntry )
    {
        delete pcfteExisting;
    }
    if ( pcfteExisting )
    {
        waiter.Wait();
    }
}
