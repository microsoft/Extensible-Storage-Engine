// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TFileFilter:  core implementation of IFileFilter and its derivatives.
//
//  This class provides the most important interaction between the file system filter and the block cache.  It is
//  responsible for hooking the methods that impact cached file state and indirecting through the cache.  Most of
//  the complexity involves synchronizing the engine IO coming through the filter and the write back from the cache.

template< class I >
class TFileFilter  //  ff
    :   public TFileWrapper<I>
{
    public:

        //  IO throttling override.

        static const OSFILEQOS qosBypassIOQuota = 0x8000000000000000L;

        C_ASSERT( ( qosBypassIOQuota & ( qosIOInMask | qosIOCompleteMask ) ) == 0 );

    public:  //  specialized API

        TFileFilter(    _Inout_     IFileAPI** const                    ppfapi,
                        _In_        IFileSystemFilter* const            pfsf,
                        _In_        IFileSystemConfiguration* const     pfsconfig,
                        _In_        IFileIdentification* const          pfident,
                        _In_        ICacheTelemetry* const              pctm,
                        _In_        ICacheRepository* const             pcrep,
                        _In_        const VolumeId                      volumeid,
                        _In_        const FileId                        fileid,
                        _Inout_opt_ ICachedFileConfiguration** const    ppcfconfig,
                        _In_        const BOOL                          fEverEligibleForCaching,
                        _Inout_opt_ ICache** const                      ppc,
                        _Inout_opt_ CCachedFileHeader** const           ppcfh );

        virtual ~TFileFilter();

        static void Cleanup();

        ICachedFileConfiguration* Pcfconfig() const { return m_pcfconfig; }

        ICache* Pc() const { return m_pc; }

        CCachedFileHeader* Pcfh() const { return m_pcfh; }

        BOOL FAttached() const { return m_pc && m_pcfh; }

        void Configure( _In_    const VolumeId                      volumeid,
                        _In_    const FileId                        fileid, 
                        _Inout_ ICachedFileConfiguration** const    ppcfconfig,
                        _In_    const BOOL                          fEverEligibleForCaching )
        {
            m_volumeid = volumeid;
            m_fileid = fileid;
            m_fileserial = fileserialInvalid;
            if ( m_volumeid == volumeidInvalid || m_fileid == fileidInvalid )
            {
                rand_s( (unsigned int*)&m_fileserial );
            }
            m_pcfconfig = *ppcfconfig;
            *ppcfconfig = NULL;
            m_fEverEligibleForCaching = fEverEligibleForCaching;

            SetCacheParameters();
        }

        void SetCacheState( _Inout_opt_ ICache** const              ppc,
                            _Inout_opt_ CCachedFileHeader** const   ppcfh )
        {
            m_pc = ppc && *ppc ? &m_cWrapper : NULL;
            m_cWrapper.SetCache( ppc );

            delete m_pcfh;
            m_pcfh = ppcfh ? *ppcfh : NULL;
            if ( ppcfh )
            {
                *ppcfh = NULL;
            }

            if ( m_pcfh )
            {
                m_fileserial = m_pcfh->Fileserial();
            }

            OSTrace( JET_tracetagBlockCache, OSFormat( "%s ctor", OSFormat( this ) ) );
        }

    public:  //  IFileFilter

        ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                _Out_ FileId* const     pfileid,
                                _Out_ FileSerial* const pfileserial ) override;

    public:  //  IFileAPI

        ERR ErrFlushFileBuffers( _In_ const IOFLUSHREASON iofr ) override;
        void SetNoFlushNeeded() override;

        ERR ErrSetSize( _In_ const TraceContext&    tc,
                        _In_ const QWORD            cbSize,
                        _In_ const BOOL             fZeroFill,
                        _In_ const OSFILEQOS        grbitQOS ) override;

        ERR ErrRename(  _In_z_  const WCHAR* const wszPathDest,
                        _In_    const BOOL         fOverwriteExisting = fFalse ) override;

        ERR ErrIOTrim(  _In_ const TraceContext&    tc,
                        _In_ const QWORD            ibOffset,
                        _In_ const QWORD            cbToFree ) override;

        ERR ErrReserveIOREQ(    _In_    const QWORD     ibOffset,
                                _In_    const DWORD     cbData,
                                _In_    const OSFILEQOS grbitQOS,
                                _Out_   VOID **         ppioreq ) override;
        VOID ReleaseUnusedIOREQ( _In_ VOID * pioreq ) override;

        ERR ErrIORead(  _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_opt_                const VOID *                    pioreq ) override;
        ERR ErrIOWrite( _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIOIssue() override;

        void RegisterIFilePerfAPI( _In_ IFilePerfAPI* const pfpapi ) override;

        LONG64 CioNonFlushed() const override;

    public:  //  IFileFilter

        ERR ErrRead(    _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_opt_                const VOID *                    pioreq ) override;
        ERR ErrWrite(   _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIssue( _In_ const IFileFilter::IOMode iom ) override;
        ERR ErrFlush( _In_ const IOFLUSHREASON iofr, _In_ const IFileFilter::IOMode iom ) override;

    private:

        //  Wait context for access to an IO offset range.

        class CWaiter
        {
            public:

                CWaiter( _In_ const COffsets& offsets )
                    :   m_offsets( offsets ),
                        m_sig( CSyncBasicInfo( "TFileFilter<I>::CWaiter::m_sig" ) ),
                        m_group( CMeteredSection::groupInvalidNil )
                {
                }

                const COffsets& Offsets() const { return m_offsets; }

                VOID Complete( _In_ const CMeteredSection::Group group )
                {
                    m_group = group;
                    m_sig.Set();
                }

                CMeteredSection::Group Wait()
                {
                    m_sig.Wait();
                    return m_group;
                }

                static SIZE_T OffsetOfILE() { return OffsetOf( CWaiter, m_ile ); }

            private:

                typename CInvasiveList<CWaiter, CWaiter::OffsetOfILE>::CElement m_ile;
                COffsets                                                        m_offsets;
                CManualResetSignal                                              m_sig;
                CMeteredSection::Group                                          m_group;
        };

        //  Context for a write back from the cache.

        class CWriteBack
        {
            public:

                CWriteBack( _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
                    :   m_ibOffset( ibOffset ),
                        m_cbData( cbData ),
                        m_pbData( pbData ),
                        m_grbitQOS( grbitQOS ),
                        m_pfnIOComplete( pfnIOComplete ),
                        m_keyIOComplete( keyIOComplete ),
                        m_pfnIOHandoff( pfnIOHandoff )
                {
                    GetCurrUserTraceContext getutc;
                    m_tc.DeepCopy( getutc.Utc(), tc );
                }

                CWriteBack( _In_ const CWriteBack& other )
                    :   m_tc( other.m_tc, fTrue ),
                        m_ibOffset( other.m_ibOffset ),
                        m_cbData( other.m_cbData ),
                        m_pbData( other.m_pbData ),
                        m_grbitQOS( other.m_grbitQOS ),
                        m_pfnIOComplete( other.m_pfnIOComplete ),
                        m_keyIOComplete( other.m_keyIOComplete ),
                        m_pfnIOHandoff( other.m_pfnIOHandoff )
                {
                }

                ERR ErrWriteBack( _In_ TFileFilter<I>* const pff, _In_ const BOOL fOverrideThrottling = fFalse )
                {
                    CTraceContextScope tcScope( m_tc );

                    return pff->ErrWriteBack(   m_tc.etc,
                                                m_ibOffset,
                                                m_cbData,
                                                m_pbData,
                                                fOverrideThrottling ? qosIONormal : m_grbitQOS,
                                                m_pfnIOComplete,
                                                m_keyIOComplete,
                                                m_pfnIOHandoff,
                                                this );
                }

                void Complete( _In_ const ERR err, _In_ TFileFilter<I>* const pff )
                {
                    if ( m_pfnIOHandoff )
                    {
                        //  NOTE:  if there is an error when requesting a write back then we will call the handoff
                        //  callback, when provided, with that error and with a NULL IOREQ*.  This deviates from the
                        //  behavior for all engine IOs and for successful write back requests.  However, we can write
                        //  the block cache to expect this behavior.

                        m_pfnIOHandoff( err,
                                        pff,
                                        m_tc,
                                        m_grbitQOS,
                                        m_ibOffset,
                                        m_cbData,
                                        m_pbData,
                                        m_keyIOComplete,
                                        NULL );
                    }

                    if ( m_pfnIOComplete )
                    {
                        m_pfnIOComplete(    err,
                                            pff,
                                            m_tc,
                                            m_grbitQOS,
                                            m_ibOffset,
                                            m_cbData,
                                            m_pbData,
                                            m_keyIOComplete );
                    }
                }

                COffsets Offsets() const { return COffsets( m_ibOffset, m_ibOffset + m_cbData - 1 ); }

                static SIZE_T OffsetOfILE() { return OffsetOf( CWriteBack, m_ile ); }

            private:

                typename CCountedInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>::CElement    m_ile;
                FullTraceContext                                                                m_tc;
                const QWORD                                                                     m_ibOffset;
                const DWORD                                                                     m_cbData;
                const BYTE* const                                                               m_pbData;
                const OSFILEQOS                                                                 m_grbitQOS;
                const IFileAPI::PfnIOComplete                                                   m_pfnIOComplete;
                const DWORD_PTR                                                                 m_keyIOComplete;
                const IFileAPI::PfnIOHandoff                                                    m_pfnIOHandoff;
        };

        //  Context for a sync write back completion.

        class CWriteBackComplete
        {
            public:

                CWriteBackComplete( _In_opt_ const DWORD_PTR                keyIOComplete,
                                    _In_opt_ const IFileAPI::PfnIOHandoff   pfnIOHandoff )
                    :   m_sigComplete( CSyncBasicInfo( "TFileFilter<I>::CWriteBackComplete::m_sigComplete" ) ),
                        m_errComplete( JET_errSuccess ),
                        m_keyIOComplete( keyIOComplete ),
                        m_pfnIOHandoff( pfnIOHandoff )
                {
                }

                ERR ErrComplete()
                {
                    m_sigComplete.Wait();
                    return m_errComplete;
                }

                static void IOComplete_(    _In_                    const ERR                   err,
                                            _In_                    IFileAPI* const             pfapi,
                                            _In_                    const FullTraceContext&     tc,
                                            _In_                    const OSFILEQOS             grbitQOS,
                                            _In_                    const QWORD                 ibOffset,
                                            _In_                    const DWORD                 cbData,
                                            _In_reads_( cbData )    const BYTE* const           pbData,
                                            _In_                    const DWORD_PTR             keyIOComplete )
                {
                    IFileAPI::PfnIOComplete pfnIOComplete = IOComplete_;
                    Unused( pfnIOComplete );

                    CWriteBackComplete* const pwriteBackComplete = (CWriteBackComplete*)keyIOComplete;
                    pwriteBackComplete->m_errComplete = err;
                    pwriteBackComplete->m_sigComplete.Set();
                }

                static void IOHandoff_( _In_                    const ERR                   err,
                                        _In_                    IFileAPI* const             pfapi,
                                        _In_                    const FullTraceContext&     tc,
                                        _In_                    const OSFILEQOS             grbitQOS,
                                        _In_                    const QWORD                 ibOffset,
                                        _In_                    const DWORD                 cbData,
                                        _In_reads_( cbData )    const BYTE* const           pbData,
                                        _In_                    const DWORD_PTR             keyIOComplete,
                                        _In_                    void* const                 pvIOContext )
                {
                    IFileAPI::PfnIOHandoff pfnIOHandoff = IOHandoff_;
                    Unused( pfnIOHandoff );

                    CWriteBackComplete* const pwriteBackComplete = (CWriteBackComplete*)keyIOComplete;
                    if ( pwriteBackComplete->m_pfnIOHandoff )
                    {
                        pwriteBackComplete->m_pfnIOHandoff( err,
                                                            pfapi,
                                                            tc,
                                                            grbitQOS,
                                                            ibOffset,
                                                            cbData,
                                                            pbData,
                                                            pwriteBackComplete->m_keyIOComplete,
                                                            pvIOContext );
                    }
                }

            private:

                CWriteBackComplete( _In_ const CWriteBackComplete& other );

            private:

                CManualResetSignal              m_sigComplete;
                ERR                             m_errComplete;
                const DWORD_PTR                 m_keyIOComplete;
                const IFileAPI::PfnIOHandoff    m_pfnIOHandoff;
        };

        ERR ErrBeginAccess( _In_    const COffsets&                 offsets,
                            _In_    const BOOL                      fWrite,
                            _Out_   CMeteredSection::Group* const   pgroup,
                            _Out_   CSemaphore** const              ppsem );
        VOID WaitForAccess( _In_ const COffsets& offsets, _Inout_ CMeteredSection::Group* const pgroup );
        BOOL FAccessPermitted( _In_ const COffsets& offsets, _In_ const CMeteredSection::Group group );
        ERR ErrVerifyAccessIgnoringWriteBack( _In_ const COffsets& offsets );
        ERR ErrVerifyAccess( _In_ const COffsets& offsets );
        VOID EndAccess( _Inout_opt_ CMeteredSection::Group* const   pgroup,
                        _Inout_opt_ CSemaphore** const              ppsem );

        ERR ErrInvalidate( _In_ const COffsets& offsets );

        static ERR ErrAttach_(  _In_ TFileFilter<I>* const  pff,
                                _In_ const COffsets&        offsetsFirstWrite )
        {
            return pff->ErrAttach( offsetsFirstWrite );
        }
        ERR ErrAttach( _In_ const COffsets& offsetsFirstWrite );
        ERR ErrGetConfiguredCache();
        ERR ErrCacheOpenFailure(    _In_ const char* const                  szFunction,
                                    _In_ const ERR                          errFromCall,
                                    _In_ const ERR                          errToReturn );

        ERR ErrTryEnqueueWriteBack( _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff );
        size_t CPendingWriteBacks();
        size_t CPendingWriteBackMax();

        void IssueWriteBacks();

        static void RequestWriteBacks_( _In_ TFileFilter<I>* const pff ) { pff->RequestWriteBacks(); }
        void RequestWriteBacks();

        void ReleaseWaitingIOs();

        void CompleteWriteBack( _In_opt_ CWriteBack* const pwriteback );

        void GetWriteBackOffsets(   _In_ CArray<COffsets>&                                      arrayPendingWriteBacks,
                                    _In_ CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>&    ilWriteBacks,
                                    _In_ size_t&                                                iwriteback );
        BOOL FTooManyWrites( _In_ const ERR err );

        ICache::CachingPolicy CpGetCachingPolicy( _In_ const TraceContext& tc, _In_ const BOOL fWrite );

        ERR ErrCacheRead(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _Out_writes_( cbData )  BYTE* const                     pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_                 const void** const              ppioreq,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );
        ERR ErrCacheMiss(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _Out_writes_( cbData )  BYTE* const                     pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const OSFILEQOS                 grbitQOSInput,
                            _In_                    const OSFILEQOS                 grbitQOSOutput,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_                 const void** const              ppioreq,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem,
                            _Inout_opt_             BOOL* const                     pfThrottleReleaser );

        ERR ErrCacheWrite(  _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );
        ERR ErrWriteThrough(    _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const OSFILEQOS                 grbitQOSInput,
                                _In_                    const OSFILEQOS                 grbitQOSOutput,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _Inout_                 CMeteredSection::Group* const   pgroup,
                                _Inout_                 CSemaphore** const              ppsem,
                                _Inout_opt_             BOOL* const                     pfThrottleReleaser );
        ERR ErrWriteBack(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_                    CWriteBack* const               pwriteback );
        ERR ErrWriteCommon( _In_                    const IFileFilter::IOMode       iom,
                            _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const OSFILEQOS                 grbitQOSInput,
                            _In_                    const OSFILEQOS                 grbitQOSOutput,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_opt_             CMeteredSection::Group* const   pgroup,
                            _Inout_opt_             CSemaphore** const              ppsem,
                            _In_opt_                CWriteBack* const               pwriteback,
                            _Inout_opt_             BOOL* const                     pfThrottleReleaser );

        void SetCacheParameters()
        {
            if ( m_pcfconfig )
            {
                m_filenumber = (ICacheTelemetry::FileNumber)m_pcfconfig->UlCacheTelemetryFileNumber();
                m_cbBlockSize = max( cbCachedBlock, m_pcfconfig->CbBlockSize() );
                m_offsetsCachedFileHeader = COffsets(   0,
                                                        max(    m_cbBlockSize,
                                                                roundup(    m_pcfconfig->UlPinnedHeaderSizeInBytes(),
                                                                            m_cbBlockSize ) ) - 1 );
            }
        }

        ICacheTelemetry::FileNumber Filenumber() const { return m_filenumber; }
        DWORD CbBlockSize() const { return m_cbBlockSize; }
        ICacheTelemetry::BlockNumber Blocknumber( const QWORD ibOffset ) const
        {
            return (ICacheTelemetry::BlockNumber)( ibOffset / CbBlockSize() );
        }

        ERR ErrBlockCacheInternalError( _In_ const char* const szTag )
        {
            return ::ErrBlockCacheInternalError( this, szTag );
        }

    private:

        class CCacheWrapper : public ICache
        {
            public:  //  specialized API

                CCacheWrapper( _In_ TFileFilter<I>* const pff, _Inout_ ICache** const ppc )
                    :   m_pff( pff ),
                        m_pcInner( ppc ? *ppc : NULL )
                {
                    if ( ppc )
                    {
                        *ppc = NULL;
                    }
                }

                virtual ~CCacheWrapper()
                {
                    delete m_pcInner;
                }

                void SetCache( _Inout_opt_ ICache** const ppc )
                {
                    delete m_pcInner;
                    m_pcInner = ppc ? *ppc : NULL;
                    if ( ppc )
                    {
                        *ppc = NULL;
                    }
                }

            public:  //  ICache

                ERR ErrCreate() override
                {
                    return ErrToErr( "Create", m_pcInner->ErrCreate() );
                }

                ERR ErrMount() override
                {
                    return ErrToErr( "Mount", m_pcInner->ErrMount() );
                }

                ERR ErrPrepareToDismount() override
                {
                    return ErrToErr( "PrepareToDismount", m_pcInner->ErrPrepareToDismount() );
                }

                ERR ErrDump( _In_ CPRINTF* const pcprintf ) override
                {
                    return ErrToErr( "Dump", m_pcInner->ErrDump( pcprintf ) );
                }

                BOOL FEnabled() override
                {
                    return m_pcInner->FEnabled();
                }

                ERR ErrGetPhysicalId(   _Out_                   VolumeId* const pvolumeid,
                                        _Out_                   FileId* const   pfileid,
                                        _Out_writes_( cbGuid )  BYTE* const     rgbUniqueId ) override
                {
                    return ErrToErr( "GetPhysicalId", m_pcInner->ErrGetPhysicalId( pvolumeid, pfileid, rgbUniqueId ) );
                }
                        
                ERR ErrClose(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) override
                {
                    return ErrToErr( "Close", m_pcInner->ErrClose( volumeid, fileid, fileserial ) );
                }

                ERR ErrFlush(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) override
                {
                    return ErrToErr( "Flush", m_pcInner->ErrFlush( volumeid, fileid, fileserial ) );
                }

                ERR ErrDestage( _In_        const VolumeId                  volumeid,
                                _In_        const FileId                    fileid,
                                _In_        const FileSerial                fileserial,
                                _In_opt_    const ICache::PfnDestageStatus  pfnDestageStatus,
                                _In_opt_    const DWORD_PTR                 keyDestageStatus ) override
                {
                    return ErrToErr(    "Destage",
                                        m_pcInner->ErrDestage(  volumeid, 
                                            fileid, 
                                            fileserial, 
                                            pfnDestageStatus, 
                                            keyDestageStatus ) );
                }

                ERR ErrInvalidate(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial,
                                    _In_ const QWORD        ibOffset,
                                    _In_ const QWORD        cbData ) override
                {
                    return ErrToErr( "Invalidate", m_pcInner->ErrInvalidate( volumeid, fileid, fileserial, ibOffset, cbData ) );
                }

                ERR ErrRead(    _In_                    const TraceContext&         tc,
                                _In_                    const VolumeId              volumeid,
                                _In_                    const FileId                fileid,
                                _In_                    const FileSerial            fileserial,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _Out_writes_( cbData )  BYTE* const                 pbData,
                                _In_                    const OSFILEQOS             grbitQOS,
                                _In_                    const ICache::CachingPolicy cp,
                                _In_opt_                const ICache::PfnComplete   pfnComplete,
                                _In_opt_                const DWORD_PTR             keyComplete ) override
                {
                    return ErrToErr( "Read", m_pcInner->ErrRead( tc, volumeid, fileid, fileserial, ibOffset, cbData, pbData, grbitQOS, cp, pfnComplete, keyComplete ) );
                }

                ERR ErrWrite(   _In_                    const TraceContext&         tc,
                                _In_                    const VolumeId              volumeid,
                                _In_                    const FileId                fileid,
                                _In_                    const FileSerial            fileserial,
                                _In_                    const QWORD                 ibOffset,
                                _In_                    const DWORD                 cbData,
                                _In_reads_( cbData )    const BYTE* const           pbData,
                                _In_                    const OSFILEQOS             grbitQOS,
                                _In_                    const ICache::CachingPolicy cp,
                                _In_opt_                const ICache::PfnComplete   pfnComplete,
                                _In_opt_                const DWORD_PTR             keyComplete ) override
                {
                    return ErrToErr( "Write", m_pcInner->ErrWrite( tc, volumeid, fileid, fileserial, ibOffset, cbData, pbData, grbitQOS, cp, pfnComplete, keyComplete ) );
                }
        
                ERR ErrIssue(   _In_ const VolumeId     volumeid,
                                _In_ const FileId       fileid,
                                _In_ const FileSerial   fileserial ) override
                {
                    return ErrToErr( "Issue", m_pcInner->ErrIssue( volumeid, fileid, fileserial ) );
                }

            private:

                ERR ErrToErr( _In_ const char* const szFunction, _In_ const ERR err )
                {
                    if ( err >= JET_errSuccess )
                    {
                        return err;
                    }

                    if ( FVerificationError( err ) )
                    {
                        return err;
                    }

                    switch ( err )
                    {
                        case JET_errOutOfMemory:
                        case JET_errFileIOBeyondEOF:
                            return err;

                        default:
                            return ErrUnexpectedCacheFailure( szFunction, err, ErrERRCheck( JET_errDiskIO ) );
                    }
                }

                ERR ErrUnexpectedCacheFailure(  _In_ const char* const  szFunction, 
                                                _In_ const ERR          errFromCall,
                                                _In_ const ERR          errToReturn )
                {
                    WCHAR           wszCachingFile[ IFileSystemAPI::cchPathMax ]    = { 0 };
                    WCHAR           wszFunction[ 256 ]                              = { 0 };
                    WCHAR           wszErrorFromCall[ 64 ]                          = { 0 };
                    WCHAR           wszErrorToReturn[ 64 ]                          = { 0 };
                    const WCHAR*    rgpwsz[]                                        = { wszCachingFile, wszFunction, wszErrorFromCall, wszErrorToReturn };

                    m_pff->m_pcfconfig->CachingFilePath( wszCachingFile );
                    OSStrCbFormatW( wszFunction, sizeof( wszFunction ), L"%hs", szFunction );
                    OSStrCbFormatW( wszErrorFromCall, sizeof( wszErrorFromCall ), L"%i (0x%08x)", errFromCall, errFromCall );
                    OSStrCbFormatW( wszErrorToReturn, sizeof( wszErrorToReturn ), L"%i (0x%08x)", errToReturn, errToReturn );

                    m_pff->m_pfsconfig->EmitEvent(  eventWarning,
                                                    BLOCK_CACHE_CATEGORY,
                                                    BLOCK_CACHE_CACHING_UNEXPECTED_FAILURE_ID,
                                                    _countof( rgpwsz ),
                                                    rgpwsz,
                                                    JET_EventLoggingLevelMin );

                    OSTraceSuspendGC();
                    BlockCacheNotableEvent( wszCachingFile,
                                            OSFormat(   "UnexpectedCacheFailure:%hs:%i:%i", 
                                                        szFunction, 
                                                        errFromCall, 
                                                        errToReturn ) );
                    OSTraceResumeGC();

#if defined( USE_HAPUBLISH_API )
                    m_pff->m_pfsconfig->EmitFailureTag( HaDbFailureTagIoHard, L"834f46a3-37e7-4bb8-b8a7-a5af4394a9c6" );
#endif

                    return errToReturn;
                }

            private:

                TFileFilter<I>* const   m_pff;
                ICache*                 m_pcInner;
        };

    private:

        //  global scoped thread local storage used for async IO run accounting

        class CThreadLocalStorage  //  ctls
            :   public CCacheThreadLocalStorageBase
        {
            public:

                CThreadLocalStorage( _In_ const CacheThreadId ctid )
                    :   CCacheThreadLocalStorageBase( ctid )
                {
                }

                static void Release( _Inout_ CThreadLocalStorage** const ppctls )
                {
                    CCacheThreadLocalStorageBase::Release( (CCacheThreadLocalStorageBase** const)ppctls );
                }

                ERR ErrRequest( _In_    IFileSystemConfiguration* const pfsconfig,
                                _In_    const VolumeId                  volumeid,
                                _In_    const FileId                    fileid,
                                _In_    const FileSerial                fileserial,
                                _In_    const BOOL                      fAsync,
                                _In_    const BOOL                      fRead,
                                _In_    const COffsets&                 offsets,
                                _In_    const OSFILEQOS                 grbitQOS,
                                _In_    const BOOL                      fMustCombineIO,
                                _Out_   BOOL* const                     pfCombined )
                {
                    ERR         err         = JET_errSuccess;
                    CRequest*   prequest    = NULL;
                    BOOL        fCombined   = fFalse;

                    *pfCombined = fFalse;

                    if ( !fAsync )
                    {
                        Error( JET_errSuccess );
                    }

                    Alloc( prequest = new CRequest( volumeid, fileid, fileserial, fRead, offsets ) );

                    Call( ErrRequest( pfsconfig, fRead, grbitQOS, fMustCombineIO, &prequest, &fCombined ) );

                    *pfCombined = fCombined;

                HandleError:
                    delete prequest;
                    if ( err < JET_errSuccess )
                    {
                        *pfCombined = fFalse;
                    }
                    return err;
                }

                void ClearRequests()
                {
                    while ( CRequest* prequestIO = IlIORequested().PrevMost() )
                    {
                        while ( CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost() )
                        {
                            prequestIO->IlRequestsByIO().Remove( prequest );

                            if ( prequest != prequestIO )
                            {
                                delete prequest;
                            }
                        }

                        IlIORequested().Remove( prequestIO );

                        delete prequestIO;
                    }
                }

            protected:

                ~CThreadLocalStorage()
                {
                    ClearRequests();
                }

            private:

                class CRequest
                {
                    public:

                        CRequest(   _In_ const VolumeId         volumeid,
                                    _In_ const FileId           fileid,
                                    _In_ const FileSerial       fileserial,
                                    _In_ const BOOL             fRead,
                                    _In_ const COffsets&        offsets )
                            :   m_volumeid( volumeid ),
                                m_fileid( fileid ),
                                m_fileserial( fileserial ),
                                m_fRead( fRead ),
                                m_offsets( offsets )
                        {
                            m_ilRequestsByIO.InsertAsPrevMost( this );
                        }

                        VolumeId Volumeid() const { return m_volumeid; }
                        FileId Fileid() const { return m_fileid; }
                        FileSerial Fileserial() const { return m_fileserial; }
                        BOOL FRead() const { return m_fRead; }
                        const COffsets& Offsets() const { return m_offsets; }

                        COffsets OffsetsForIO() const
                        {
                            CRequest* const prequestIOFirst = m_ilRequestsByIO.PrevMost();
                            const QWORD     ibStartIOFirst = prequestIOFirst->Offsets().IbStart();
                            CRequest* const prequestIOLast = m_ilRequestsByIO.NextMost();
                            const QWORD     ibEndIOLast = prequestIOLast->Offsets().IbEnd();

                            return COffsets( ibStartIOFirst, ibEndIOLast );
                        }

                        static SIZE_T OffsetOfIOs() { return OffsetOf( CRequest, m_ileIOs ); }
                        static SIZE_T OffsetOfRequestsByIO() { return OffsetOf( CRequest, m_ileRequestsByIO ); }

                        CCountedInvasiveList<CRequest, CRequest::OffsetOfRequestsByIO>& IlRequestsByIO() { return m_ilRequestsByIO; }

                    private:

                        const VolumeId                                                          m_volumeid;
                        const FileId                                                            m_fileid;
                        const FileSerial                                                        m_fileserial;
                        const BOOL                                                              m_fRead;
                        const COffsets                                                          m_offsets;
                        typename CCountedInvasiveList<CRequest, OffsetOfIOs>::CElement          m_ileIOs;
                        CCountedInvasiveList<CRequest, OffsetOfRequestsByIO>                    m_ilRequestsByIO;
                        typename CCountedInvasiveList<CRequest, OffsetOfRequestsByIO>::CElement m_ileRequestsByIO;
                };

            private:

                ERR ErrRequest( _In_    IFileSystemConfiguration* const pfsconfig,
                                _In_    const BOOL                      fRead,
                                _In_    const OSFILEQOS                 grbitQOS,
                                _In_    const BOOL                      fMustCombineIO,
                                _Inout_ CRequest** const                pprequest,
                                _Out_   BOOL* const                     pfCombined )
                {
                    ERR         err             = JET_errSuccess;
                    CRequest*   prequest        = *pprequest;
                    CRequest*   prequestIO      = NULL;
                    CRequest*   prequestIOPrev  = NULL;
                    BOOL        fCombined       = fFalse;

                    *pprequest = NULL;
                    *pfCombined = fFalse;

                    //  insert the request and combine with any other requests if possible

                    IlIORequested().InsertAsNextMost( prequest );

                    prequestIO = prequest;
                    for (   prequestIOPrev = IlIORequested().Prev( prequestIO );
                            prequestIOPrev && !FConflicting( prequestIOPrev, prequestIO );
                            prequestIOPrev = IlIORequested().Prev( prequestIOPrev ) )
                    {
                        if ( FCombinable( pfsconfig, grbitQOS, prequestIOPrev, prequestIO ) )
                        {
                            if ( prequestIOPrev->OffsetsForIO().IbStart() > prequestIO->OffsetsForIO().IbStart() )
                            {
                                while ( CRequest* prequestT = prequestIO->IlRequestsByIO().NextMost() )
                                {
                                    prequestIO->IlRequestsByIO().Remove( prequestT );
                                    prequestIOPrev->IlRequestsByIO().InsertAsPrevMost( prequestT );
                                }
                            }
                            else
                            {
                                while ( CRequest* prequestT = prequestIO->IlRequestsByIO().PrevMost() )
                                {
                                    prequestIO->IlRequestsByIO().Remove( prequestT );
                                    prequestIOPrev->IlRequestsByIO().InsertAsNextMost( prequestT );
                                }
                            }

                            IlIORequested().Remove( prequestIO );
                            prequestIO = prequestIOPrev;

                            fCombined = fTrue;
                        }
                        else if ( CmpRequestIO( prequestIOPrev, prequestIO ) > 0 )
                        {
                            IlIORequested().Remove( prequestIO );
                            IlIORequested().Insert( prequestIO, prequestIOPrev );
                            prequestIOPrev = prequestIO;
                        }
                    }

                    //  determine if this request could be combined via IO gap coalescing

                    if (    IlIORequested().Prev( prequestIO ) &&
                            FBridgeableGap( pfsconfig, grbitQOS, IlIORequested().Prev( prequestIO ), prequestIO ) )
                    {
                        fCombined = fTrue;
                    }

                    if (    IlIORequested().Next( prequestIO ) &&
                            FBridgeableGap( pfsconfig, grbitQOS, prequestIO, IlIORequested().Next( prequestIO ) ) )
                    {
                        fCombined = fTrue;
                    }

                    //  if this request could not be combined with another yet this is requested then don't accept it

                    if ( !fCombined && ( ( grbitQOS & qosIOOptimizeCombinable ) || fMustCombineIO ) )
                    {
                        IlIORequested().Remove( prequest );
                        *pprequest = prequest;
                        Error( ErrERRCheck( errDiskTilt ) );
                    }

                    *pfCombined = fCombined;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pfCombined = fFalse;
                    }
                    return err;
                }

                BOOL FConflicting( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB )
                {
                    //  IOs for different cached file cannot conflict

                    if ( prequestIOA->Volumeid() != prequestIOB->Volumeid() )
                    {
                        return fFalse;
                    }

                    if ( prequestIOA->Fileid() != prequestIOB->Fileid() )
                    {
                        return fFalse;
                    }

                    if ( prequestIOA->Fileserial() != prequestIOB->Fileserial() )
                    {
                        return fFalse;
                    }

                    //  IOs with disjoint offset ranges cannot conflict

                    if ( !prequestIOA->OffsetsForIO().FOverlaps( prequestIOB->OffsetsForIO() ) )
                    {
                        return fFalse;
                    }

                    return fTrue;
                }

                BOOL FCombinable(   _In_ IFileSystemConfiguration* const    pfsconfig, 
                                    _In_ const OSFILEQOS                    grbitQOS,
                                    _In_ CRequest* const                    prequestIOA, 
                                    _In_ CRequest* const                    prequestIOB )
                {
                    //  IOs from different cached files cannot be combined

                    if ( prequestIOA->Volumeid() != prequestIOB->Volumeid() )
                    {
                        return fFalse;
                    }

                    if ( prequestIOA->Fileid() != prequestIOB->Fileid() )
                    {
                        return fFalse;
                    }

                    if ( prequestIOA->Fileserial() != prequestIOB->Fileserial() )
                    {
                        return fFalse;
                    }

                    //  IOs with different types cannot be combined

                    if ( prequestIOA->FRead() != prequestIOB->FRead() )
                    {
                        return fFalse;
                    }

                    //  IOs with overlapping offsets cannot be combined

                    const COffsets offsetsIOA = prequestIOA->OffsetsForIO();
                    const COffsets offsetsIOB = prequestIOB->OffsetsForIO();

                    if ( offsetsIOA.FOverlaps( offsetsIOB ) )
                    {
                        return fFalse;
                    }

                    //  IOs with non-adjacent offsets cannot be combined

                    if ( !offsetsIOA.FAdjacent( offsetsIOB ) )
                    {
                        return fFalse;
                    }

                    //  IOs that are too large cannot be combined

                    const QWORD cbMaxSize = prequestIOA->FRead() ? pfsconfig->CbMaxReadSize() : pfsconfig->CbMaxWriteSize();

                    if ( offsetsIOA.Cb() + offsetsIOB.Cb() > cbMaxSize )
                    {
                        if ( !FOverrideMaxSize( grbitQOS, prequestIOA->FRead() ) )
                        {
                            return fFalse;
                        }
                    }

                    return fTrue;
                }

                BOOL FOverrideMaxSize(  _In_ const OSFILEQOS grbitQOS, _In_ const BOOL fRead )
                {
                    if ( fRead )
                    {
                        return fFalse;
                    }

                    if ( !( grbitQOS & qosIOOptimizeOverrideMaxIOLimits ) )
                    {
                        return fFalse;
                    }

                    return fTrue;
                }

                int CmpRequestIO( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB )
                {
                    //  IOs from different cached files are ordered by their volume id and file id for deterministic behavior

                    if ( prequestIOA->Volumeid() < prequestIOB->Volumeid() )
                    {
                        return -1;
                    }
                    else if ( prequestIOA->Volumeid() > prequestIOB->Volumeid() )
                    {
                        return 1;
                    }

                    if ( prequestIOA->Fileid() < prequestIOB->Fileid() )
                    {
                        return -1;
                    }
                    else if ( prequestIOA->Fileid() > prequestIOB->Fileid() )
                    {
                        return 1;
                    }

                    if ( prequestIOA->Fileserial() < prequestIOB->Fileserial() )
                    {
                        return -1;
                    }
                    else if ( prequestIOA->Fileserial() > prequestIOB->Fileserial() )
                    {
                        return 1;
                    }

                    //  IOs whose offsets are in ascending order are not out of order

                    if ( prequestIOA->OffsetsForIO().IbStart() < prequestIOB->OffsetsForIO().IbStart() )
                    {
                        return -1;
                    }
                    else if ( prequestIOA->OffsetsForIO().IbStart() > prequestIOB->OffsetsForIO().IbStart() )
                    {
                        return 1;
                    }

                    return 0;
                }

                BOOL FBridgeableGap(    _In_ IFileSystemConfiguration* const    pfsconfig,
                                        _In_ const OSFILEQOS                    grbitQOS,
                                        _In_ CRequest* const                    prequestIOA, 
                                        _In_ CRequest* const                    prequestIOB )
                {
                    //  IOs from different cached files cannot be combined via a gap

                    if ( prequestIOA->Volumeid() != prequestIOB->Volumeid() )
                    {
                        return fFalse;
                    }

                    if ( prequestIOA->Fileid() != prequestIOB->Fileid() )
                    {
                        return fFalse;
                    }

                    if ( prequestIOA->Fileserial() != prequestIOB->Fileserial() )
                    {
                        return fFalse;
                    }

                    //  IOs with different types cannot be combined via a gap

                    if ( prequestIOA->FRead() != prequestIOB->FRead() )
                    {
                        return fFalse;
                    }

                    //  IOs with overlapping offsets cannot be combined via a gap

                    const COffsets offsetsIOA = prequestIOA->OffsetsForIO();
                    const COffsets offsetsIOB = prequestIOB->OffsetsForIO();

                    if ( offsetsIOA.FOverlaps( offsetsIOB ) )
                    {
                        return fFalse;
                    }

                    //  writes cannot be combined via a gap

                    if ( !prequestIOA->FRead() )
                    {
                        return fFalse;
                    }

                    //  reads with non-adjacent offsets that are too far apart cannot be combined via a gap

                    const QWORD cbGap = offsetsIOA.IbStart() < offsetsIOB.IbStart() ?
                                            offsetsIOB.IbStart() - offsetsIOA.IbEnd() - 1 :
                                            offsetsIOA.IbStart() - offsetsIOB.IbEnd() - 1;

                    if ( cbGap > pfsconfig->CbMaxReadGapSize() )
                    {
                        return fFalse;
                    }

                    //  reads that are too large cannot be combined via a gap

                    if ( offsetsIOA.Cb() + offsetsIOB.Cb() + cbGap > pfsconfig->CbMaxReadSize() )
                    {
                        if ( !FOverrideMaxSize( grbitQOS, prequestIOA->FRead() ) )
                        {
                            return fFalse;
                        }
                    }

                    return fTrue;
                }

                CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>& IlIORequested() { return m_ilIORequested; }

            private:

                CCountedInvasiveList<CRequest, CRequest::OffsetOfIOs>   m_ilIORequested;
        };

        class CThreadLocalStorageRepository
        {
            public:

                static ERR ErrGetThreadLocalStorage( _Out_ CThreadLocalStorage** const ppctls )
                {
                    return s_tls.ErrGetThreadLocalStorage( ppctls );
                }

                static void Cleanup()
                {
                    s_tls.Cleanup();
                }

            private:

                using CStaticThreadLocalStorage = TCacheThreadLocalStorage<CThreadLocalStorage>;

                static CStaticThreadLocalStorage    s_tls;
        };

    private:

        class CReservedIOREQ
        {
            public:

                CReservedIOREQ( _In_    TFileFilter<I>* const   pff,
                                _In_    const OSFILEQOS         grbitQOSOutput,
                                _Inout_ BOOL* const             pfThrottleReleaser )
                    :   m_pff( pff ),
                        m_grbitQOSOutput( grbitQOSOutput ),
                        m_fThrottleReleaser( *pfThrottleReleaser )
                {
                    *pfThrottleReleaser = fFalse;
                }

                ~CReservedIOREQ()
                {
                    (void)m_pff->FThrottleRelease( fTrue, fTrue, m_grbitQOSOutput, &m_fThrottleReleaser );
                }

                void Capture( _Out_ BOOL* const pfThrottleReleaser, _Out_ OSFILEQOS* const pgrbitQOSOutput )
                {
                    *pfThrottleReleaser = m_fThrottleReleaser;
                    m_fThrottleReleaser = fFalse;

                    *pgrbitQOSOutput = m_grbitQOSOutput;
                }

            private:

                TFileFilter<I>* const   m_pff;
                const OSFILEQOS         m_grbitQOSOutput;
                BOOL                    m_fThrottleReleaser;
        };

        void ReleaseUnusedIOREQ( _Inout_ const void** const ppioreq )
        {
            ReleaseUnusedIOREQ( (void**)ppioreq );
        }

        void ReleaseUnusedIOREQ( _Inout_ void** const ppioreq )
        {
            CReservedIOREQ* preservedIOREQ = (CReservedIOREQ*)*ppioreq;

            *ppioreq = NULL;

            ReleaseUnusedIOREQ( &preservedIOREQ );
        }

        void ReleaseUnusedIOREQ( _Inout_ CReservedIOREQ** const ppreservedIOREQ )
        {
            CReservedIOREQ* const preservedIOREQ = *ppreservedIOREQ;

            *ppreservedIOREQ = NULL;

            delete preservedIOREQ;
        }

    private:

        class CThrottleContext
        {
            public:

                CThrottleContext(   _In_ IFileSystemConfiguration* const    pfsconfig,
                                    _In_ const ULONG_PTR                    ulDiskId )
                    :   m_ulDiskId( ulDiskId ),
                        m_cIOMax( pfsconfig->CIOMaxOutstanding() ),
                        m_cIOBackgroundMax( pfsconfig->CIOMaxOutstandingBackground() ),
                        m_cIOBackgroundLow( CioBackgroundIOLow( m_cIOBackgroundMax ) ),
                        m_cIOUrgentBackgroundMax( CioDefaultUrgentOutstandingIOMax( m_cIOMax ) ),
                        m_crit( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::CThrottleContext::m_crit" ), rankThrottleContext, 0 ) ),
                        m_cIO( 0 ),
                        m_cIOBackground( 0 ),
                        m_cIOUrgentBackground( 0 )
                {
                }

                ULONG_PTR UlDiskId() const { return m_ulDiskId; }

                class CWaiter;

                BOOL FRequest(  _In_ const BOOL         fAsync,
                                _In_ const BOOL         fRead,
                                _In_ const OSFILEQOS    grbitQOS,
                                _In_ CWaiter* const     pwaiter )
                {
                    BOOL                                                fAcquiredIO                 = fFalse;
                    BOOL                                                fAcquiredBackground         = fFalse;
                    BOOL                                                fAcquiredUrgentBackground   = fFalse;
                    BOOL                                                fSucceeded                  = fFalse;
                    CInvasiveList<CWaiter, CWaiter::OffsetOfToComplete> ilToComplete;

                    m_crit.Enter();

                    fAcquiredBackground = FRequestBackgroundIO( fRead, grbitQOS );
                    fAcquiredUrgentBackground = FRequestUrgentBackgroundIO( fRead, grbitQOS );

                    if ( fAcquiredBackground && fAcquiredUrgentBackground )
                    {
                        fAcquiredIO = FRequestIO( fAsync, fRead, grbitQOS, pwaiter );
                    }
                    else
                    {
                        pwaiter->Complete();
                    }

                    fSucceeded = fAcquiredIO && fAcquiredBackground && fAcquiredUrgentBackground;

                    if ( !fSucceeded )
                    {
                        if ( fAcquiredBackground )
                        {
                            (void)FReleaseBackgroundIO( fRead, grbitQOS );
                        }
                        if ( fAcquiredUrgentBackground )
                        {
                            (void)FReleaseUrgentBackgroundIO( fRead, grbitQOS );
                        }
                        if ( fAcquiredIO )
                        {
                            ReleaseIO( grbitQOS, pwaiter );
                            GetIOWaitersToComplete( pwaiter, ilToComplete );
                        }
                    }

                    m_crit.Leave();

                    CompleteIOWaiters( ilToComplete );

                    return fSucceeded;
                }

                BOOL FRelease(  _In_        const BOOL      fAsync,
                                _In_        const BOOL      fRead,
                                _In_        const OSFILEQOS grbitQOS,
                                _Inout_opt_ BOOL* const     pfThrottleReleaser,   
                                _In_opt_    CWaiter* const  pwaiter )
                {
                    const BOOL                                          fThrottleReleaser   = pfThrottleReleaser ? *pfThrottleReleaser : fFalse;
                    BOOL                                                fQuotaAvailable     = fFalse;
                    CInvasiveList<CWaiter, CWaiter::OffsetOfToComplete> ilToComplete;

                    if ( pfThrottleReleaser )
                    {
                        *pfThrottleReleaser = fFalse;
                    }

                    if ( fThrottleReleaser )
                    {
                        m_crit.Enter();

                        if ( fThrottleReleaser )
                        {
                            if ( FReleaseBackgroundIO( fRead, grbitQOS ) )
                            {
                                fQuotaAvailable = fTrue;
                            }

                            if ( FReleaseUrgentBackgroundIO( fRead, grbitQOS ) )
                            {
                                fQuotaAvailable = fTrue;
                            }

                            ReleaseIO( grbitQOS, pwaiter );
                        }

                        GetIOWaitersToComplete( pwaiter, ilToComplete );

                        m_crit.Leave();
                    }

                    CompleteIOWaiters( ilToComplete );

                    return fQuotaAvailable;
                }

                static SIZE_T OffsetOfILE() { return OffsetOf( CThrottleContext, m_ile ); }

            public:

                //  Wait context.

                class CWaiter
                {
                    public:

                        CWaiter()
                            :   m_sig( CSyncBasicInfo( "TFileFilter<I>::CThrottleContext::CWaiter::m_sig" ) )
                        {
                        }

                        ~CWaiter()
                        {
                            Wait();
                        }

                        BOOL FWaiting() const { return !m_ileWaiting.FUninitialized(); }

                        BOOL FComplete() const { return m_sig.FIsSet(); }

                        void Complete()
                        {
                            m_sig.Set();
                        }

                        void Wait()
                        {
                            m_sig.Wait();
                        }

                        static SIZE_T OffsetOfWaiting() { return OffsetOf( CWaiter, m_ileWaiting ); }
                        static SIZE_T OffsetOfToComplete() { return OffsetOf( CWaiter, m_ileToComplete ); }

                    private:

                        typename CInvasiveList<CWaiter, CWaiter::OffsetOfWaiting>::CElement     m_ileWaiting;
                        typename CInvasiveList<CWaiter, CWaiter::OffsetOfToComplete>::CElement  m_ileToComplete;
                        CManualResetSignal                                                      m_sig;
                };

            private:

                BOOL FRequestIO(    _In_ const BOOL         fAsync,
                                    _In_ const BOOL         fRead,
                                    _In_ const OSFILEQOS    grbitQOS, 
                                    _In_ CWaiter* const     pwaiter )
                { 
                    if ( grbitQOS & qosBypassIOQuota )
                    {
                        pwaiter->Complete();
                        return fTrue;
                    }

                    if ( FIncrementMax( m_cIO, m_cIOMax ) )
                    {
                        pwaiter->Complete();
                        return fTrue;
                    }

                    if ( fAsync && !fRead )
                    {
                        if (    ( grbitQOS & ( qosIODispatchBackground | qosIODispatchUrgentBackgroundMask ) ) != 0 ||
                                ( grbitQOS & qosIOOptimizeCombinable ) != 0 )
                        {
                            pwaiter->Complete();
                            return fFalse;
                        }
                    }

                    m_ilWaiting.InsertAsNextMost( pwaiter );

                    return fTrue;
                }

                void ReleaseIO( _In_ const OSFILEQOS grbitQOS, _In_opt_ CWaiter* const pwaiter )
                {
                    if ( grbitQOS & qosBypassIOQuota )
                    {
                        return;
                    }

                    if ( !pwaiter || !pwaiter->FWaiting() )
                    {
                        DecrementMin( m_cIO, 0 );
                    }
                }

                void GetIOWaitersToComplete(    _In_opt_    CWaiter* const                                          pwaiter,
                                                _Inout_     CInvasiveList<CWaiter, CWaiter::OffsetOfToComplete>&    ilToComplete )
                {
                    if ( pwaiter && pwaiter->FWaiting() )
                    {
                        m_ilWaiting.Remove( pwaiter );
                        pwaiter->Complete();
                    }

                    while ( m_ilWaiting.PrevMost() && FIncrementMax( m_cIO, m_cIOMax ) )
                    {
                        CWaiter* const pwaiterToComplete = m_ilWaiting.PrevMost();
                        m_ilWaiting.Remove( pwaiterToComplete );
                        ilToComplete.InsertAsPrevMost( pwaiterToComplete );
                    }
                }

                void CompleteIOWaiters( _Inout_ CInvasiveList<CWaiter, CWaiter::OffsetOfToComplete>& ilToComplete )
                {
                    while ( ilToComplete.PrevMost() )
                    {
                        CWaiter* const pwaiterToComplete = ilToComplete.PrevMost();
                        ilToComplete.Remove( pwaiterToComplete );
                        pwaiterToComplete->Complete();
                    }
                }

                BOOL FRequestBackgroundIO( _In_ const BOOL fRead, _In_ const OSFILEQOS grbitQOS )
                { 
                    if ( fRead )
                    {
                        return fTrue;
                    }

                    if ( !( grbitQOS & qosIODispatchBackground ) )
                    {
                        return fTrue;
                    }

                    if ( FIncrementMax( m_cIOBackground, m_cIOBackgroundMax ) )
                    {
                        return fTrue;
                    }

                    return fFalse;
                }

                BOOL FReleaseBackgroundIO( _In_ const BOOL fRead, _In_ const OSFILEQOS grbitQOS )
                {
                    if ( fRead )
                    {
                        return fFalse;
                    }

                    if ( !( grbitQOS & qosIODispatchBackground ) )
                    {
                        return fFalse;
                    }

                    DecrementMin( m_cIOBackground, 0 );

                    return m_cIOBackground <= m_cIOBackgroundLow;
                }

                BOOL FRequestUrgentBackgroundIO( _In_ const BOOL fRead, _In_ const OSFILEQOS grbitQOS )
                {
                    if ( fRead )
                    {
                        return fTrue;
                    }

                    if ( !( grbitQOS & qosIODispatchUrgentBackgroundMask ) )
                    {
                        return fTrue;
                    }

                    const ULONG iUrgent = IOSDiskIUrgentLevelFromQOS( grbitQOS );
                    const DWORD cIOUrgentBackgroundMax = CioOSDiskIFromUrgentLevel( iUrgent, m_cIOUrgentBackgroundMax );

                    if ( FIncrementMax( m_cIOUrgentBackground, cIOUrgentBackgroundMax, m_cIOUrgentBackgroundMax ) )
                    {
                        return fTrue;
                    }

                    return fFalse;
                }

                BOOL FReleaseUrgentBackgroundIO( _In_ const BOOL fRead, _In_ const OSFILEQOS grbitQOS )
                {
                    if ( fRead )
                    {
                        return fFalse;
                    }

                    if ( !( grbitQOS & qosIODispatchUrgentBackgroundMask ) )
                    {
                        return fFalse;
                    }

                    const ULONG iUrgentLower = max( 2, IOSDiskIUrgentLevelFromQOS( grbitQOS ) ) - 1;
                    const DWORD cIOUrgentBackgroundMax = CioOSDiskIFromUrgentLevel( iUrgentLower, m_cIOUrgentBackgroundMax );

                    DecrementMin( m_cIOUrgentBackground, 0 );
                    
                    return m_cIOUrgentBackground <= cIOUrgentBackgroundMax;
                }

                BOOL FIncrementMax( _Inout_ DWORD& dwTarget, _In_ const DWORD dwTargetMax )
                {
                    return FIncrementMax( dwTarget, dwTargetMax, dwTargetMax );
                }

                BOOL FIncrementMax( _Inout_ DWORD& dwTarget, _In_ const DWORD dwTargetMax, _In_ const DWORD dwTargetMaxMax )
                {
                    Assert( dwTarget <= dwTargetMaxMax );

                    if ( dwTarget >= dwTargetMax )
                    {
                        return fFalse;
                    }

                    dwTarget++;

                    return fTrue;
                }

                void DecrementMin( _Inout_ DWORD& dwTarget, _In_ const DWORD dwTargetMin )
                {
                    Assert( dwTarget > dwTargetMin );

                    if ( dwTarget > dwTargetMin )
                    {
                        dwTarget--;
                    }
                }

            private:

                const ULONG_PTR                                                                             m_ulDiskId;
                const DWORD                                                                                 m_cIOMax;
                const DWORD                                                                                 m_cIOBackgroundMax;
                const DWORD                                                                                 m_cIOBackgroundLow;
                const DWORD                                                                                 m_cIOUrgentBackgroundMax;
                typename CCountedInvasiveList<CThrottleContext, CThrottleContext::OffsetOfILE>::CElement    m_ile;
                CCriticalSection                                                                            m_crit;
                CCountedInvasiveList<CWaiter, CWaiter::OffsetOfWaiting>                                     m_ilWaiting;
                DWORD                                                                                       m_cIO;
                DWORD                                                                                       m_cIOBackground;
                DWORD                                                                                       m_cIOUrgentBackground;
        };

        class CThrottleContextRepository
        {
            public:

                static ERR ErrGetThrottleContext(   _In_    IFileSystemConfiguration* const pfsconfig,
                                                    _In_    const ULONG_PTR                 ulDiskId,
                                                    _Out_   CThrottleContext** const        pptc )
                {
                    ERR                 err             = JET_errSuccess;
                    BOOL                fLeaveReader    = fFalse;
                    CThrottleContext*   ptcCached       = NULL;

                    *pptc = NULL;

                    s_rwlThrottleContexts.EnterAsReader();
                    fLeaveReader = fTrue;

                    for (   ptcCached = s_ilThrottleContexts.PrevMost();
                            ptcCached != NULL && ptcCached->UlDiskId() != ulDiskId;
                            ptcCached = s_ilThrottleContexts.Next( ptcCached ) )
                    {
                    }

                    s_rwlThrottleContexts.LeaveAsReader();
                    fLeaveReader = fFalse;

                    if ( !ptcCached )
                    {
                        Call( ErrGetThrottleContextSlowly( pfsconfig, ulDiskId, &ptcCached ) );
                    }

                    *pptc = ptcCached;

                HandleError:
                    if ( fLeaveReader )
                    {
                        s_rwlThrottleContexts.LeaveAsReader();
                    }
                    if ( err < JET_errSuccess )
                    {
                        *pptc = NULL;
                    }
                    return err;
                }

                static void Cleanup()
                {
                    while ( s_ilThrottleContexts.PrevMost() )
                    {
                        CThrottleContext* const ptc = s_ilThrottleContexts.PrevMost();
                        s_ilThrottleContexts.Remove( ptc );
                        delete ptc;
                    }
                }

            private:

                static ERR ErrGetThrottleContextSlowly( _In_    IFileSystemConfiguration* const pfsconfig,
                                                        _In_    const ULONG_PTR                 ulDiskId,
                                                        _Out_   CThrottleContext** const        pptc )
                {
                    ERR                 err             = JET_errSuccess;
                    CThrottleContext*   ptcCached       = NULL;
                    CThrottleContext*   ptcNew          = NULL;
                    BOOL                fLeaveWriter    = fFalse;

                    *pptc = NULL;

                    s_rwlThrottleContexts.EnterAsWriter();
                    fLeaveWriter = fTrue;

                    for (   ptcCached = s_ilThrottleContexts.PrevMost();
                            ptcCached != NULL && ptcCached->UlDiskId() != ulDiskId;
                            ptcCached = s_ilThrottleContexts.Next( ptcCached ) )
                    {
                    }

                    if ( !ptcCached )
                    {
                        Alloc( ptcCached = ptcNew = new CThrottleContext( pfsconfig, ulDiskId ) );

                        s_ilThrottleContexts.InsertAsNextMost( ptcNew );
                        ptcNew = NULL;
                    }

                    *pptc = ptcCached;

                HandleError:
                    if ( fLeaveWriter )
                    {
                        s_rwlThrottleContexts.LeaveAsWriter();
                    }
                    delete ptcNew;
                    if ( err < JET_errSuccess )
                    {
                        *pptc = NULL;
                    }
                    return err;
                }

            private:

                using CThrottleContextList = CCountedInvasiveList<CThrottleContext, CThrottleContext::OffsetOfILE>;

                static CReaderWriterLock    s_rwlThrottleContexts;
                static CThrottleContextList s_ilThrottleContexts;
        };

    private:

        static ERR ErrCombineCheck( _In_    IFileSystemConfiguration* const             pfsconfig,
                                    _In_    const VolumeId                              volumeid,
                                    _In_    const FileId                                fileid,
                                    _In_    const FileSerial                            fileserial,
                                    _In_    const BOOL                                  fAsync,
                                    _In_    const BOOL                                  fRead,
                                    _In_    const COffsets&                             offsets,
                                    _In_    const OSFILEQOS                             grbitQOS,
                                    _In_    const BOOL                                  fMustCombine,
                                    _Out_   BOOL* const                                 pfCombined )
        {
            ERR                     err                 = JET_errSuccess;
            CThreadLocalStorage*    pctls               = NULL;
            BOOL                    fCombined           = fFalse;

            *pfCombined = fFalse;

            Call( CThreadLocalStorageRepository::ErrGetThreadLocalStorage( &pctls ) );

            //  determine if we can combine this request with an existing request

            Call( pctls->ErrRequest(    pfsconfig, 
                                        volumeid, 
                                        fileid, 
                                        fileserial, 
                                        fAsync, 
                                        fRead, 
                                        offsets, 
                                        grbitQOS, 
                                        fMustCombine, 
                                        &fCombined ) );

            //  indicate to the caller if we combined the IO

            *pfCombined = fCombined;

        HandleError:
            CThreadLocalStorage::Release( &pctls );
            if ( err < JET_errSuccess )
            {
                *pfCombined = fFalse;
            }
            return err;
        }

        static ERR ErrThrottleCheck(    _In_    IFileSystemConfiguration* const             pfsconfig,
                                        _In_    const ULONG_PTR                             ulDiskId,
                                        _In_    const VolumeId                              volumeid,
                                        _In_    const FileId                                fileid,
                                        _In_    const FileSerial                            fileserial,
                                        _In_    const BOOL                                  fAsync,
                                        _In_    const BOOL                                  fRead,
                                        _In_    const COffsets&                             offsets,
                                        _In_    const OSFILEQOS                             grbitQOS,
                                        _In_    typename CThrottleContext::CWaiter* const   pwaiter,
                                        _Out_   BOOL* const                                 pfThrottleReleaser,
                                        _Out_   BOOL* const                                 pfCombined )
        {
            ERR                 err                 = JET_errSuccess;
            CThrottleContext*   ptc                 = NULL;
            BOOL                fThrottleAcquired   = fFalse;
            BOOL                fCombined           = fFalse;

            *pfThrottleReleaser = fFalse;
            *pfCombined = fFalse;

            Call( CThrottleContextRepository::ErrGetThrottleContext( pfsconfig, ulDiskId, &ptc ) );

            //  try to acquire a throttle count

            fThrottleAcquired = ptc->FRequest( fAsync, fRead, grbitQOS, pwaiter );

            //  check if we can combine this request with an existing request.  if we failed to acquire a throttle
            //  count then this must succeed for the IO to be allowed

            Call( ErrCombineCheck(  pfsconfig, 
                                    volumeid, 
                                    fileid,
                                    fileserial, 
                                    fAsync, 
                                    fRead, 
                                    offsets, 
                                    grbitQOS, 
                                    !fThrottleAcquired,
                                    &fCombined ) );

            //  if the request was combined with an existing request and we acquired a throttle count then release it

            if ( fCombined )
            {
                (void)ptc->FRelease( fAsync, fRead, grbitQOS, &fThrottleAcquired, pwaiter );
            }

            //  if we still have a throttle count then indicate to the caller that they own releasing it

            *pfThrottleReleaser = fThrottleAcquired;
            fThrottleAcquired = fFalse;
            *pfCombined = fCombined;

        HandleError:
            if ( ptc )
            {
                (void)ptc->FRelease( fAsync, fRead, grbitQOS, &fThrottleAcquired, pwaiter );
            }
            if ( err < JET_errSuccess )
            {
                if ( ptc )
                {
                    (void)ptc->FRelease( fAsync, fRead, grbitQOS, pfThrottleReleaser, pwaiter );
                }
                *pfCombined = fFalse;
            }
            return err;
        }

        static BOOL FThrottleRelease(   _In_        IFileSystemConfiguration* const             pfsconfig,
                                        _In_        const ULONG_PTR                             ulDiskId,
                                        _In_        const BOOL                                  fAsync,
                                        _In_        const BOOL                                  fRead,
                                        _In_        const OSFILEQOS                             grbitQOS,
                                        _Inout_     BOOL* const                                 pfThrottleReleaser,
                                        _In_opt_    typename CThrottleContext::CWaiter* const   pwaiter             = NULL )
        {
            BOOL                fThrottleReleaser   = *pfThrottleReleaser;
            CThrottleContext*   ptc                 = NULL;

            *pfThrottleReleaser = fFalse;

            if ( !fThrottleReleaser )
            {
                return fFalse;
            }

            CallSx( CThrottleContextRepository::ErrGetThrottleContext( pfsconfig, ulDiskId, &ptc ), JET_errOutOfMemory );

            if ( !ptc )
            {
                return fFalse;
            }

            return ptc->FRelease( fAsync, fRead, grbitQOS, &fThrottleReleaser, pwaiter );
        }

        ERR ErrThrottleCheck(   _In_    const BOOL          fAsync,
                                _In_    const BOOL          fRead,
                                _In_    const COffsets&     offsets,
                                _In_    const OSFILEQOS     grbitQOS,
                                _Out_   BOOL* const         pfThrottleReleaser,
                                _Out_   BOOL* const         pfCombined )
        {
            ERR                         err                 = JET_errSuccess;
            ULONG_PTR                   ulDiskId            = 0;
            CThrottleContext::CWaiter   waiter;
            BOOL                        fThrottleAcquired   = fFalse;
            BOOL                        fCombined           = fFalse;

            *pfThrottleReleaser = fFalse;
            *pfCombined = fFalse;

            //  try to acquire a throttle count for the disk backing this file

            Call( ErrDiskId( &ulDiskId ) );

            Call( ErrThrottleCheck( m_pfsconfig, 
                                    ulDiskId, 
                                    m_volumeid, 
                                    m_fileid,
                                    m_fileserial, 
                                    fAsync, 
                                    fRead, 
                                    offsets, 
                                    grbitQOS, 
                                    &waiter,
                                    &fThrottleAcquired,
                                    &fCombined ) );

            //  if we need to wait to get a throttle count then we must issue to avoid deadlocks

            if ( fThrottleAcquired && !fCombined && !waiter.FComplete() )
            {
                //  issue any IO queued for this thread

                Call( ErrIOIssue() );

                //  either register this request for future checks or fail if the request must be combined because we
                //  just issued all our IO

                Call( ErrCombineCheck(  m_pfsconfig, 
                                        m_volumeid, 
                                        m_fileid,
                                        m_fileserial, 
                                        fAsync, 
                                        fRead, 
                                        offsets, 
                                        grbitQOS, 
                                        fFalse,
                                        &fCombined ) );

                Assert( fThrottleAcquired && !fCombined );
            }

            //  if we need to wait to get a throttle count then do so

            waiter.Wait();

            //  if we still have a throttle count then indicate to the caller that they own releasing it

            *pfThrottleReleaser = fThrottleAcquired;
            fThrottleAcquired = fFalse;
            *pfCombined = fCombined;

        HandleError:
            (void)FThrottleRelease( m_pfsconfig, ulDiskId, fAsync, fRead, grbitQOS, &fThrottleAcquired, &waiter );
            if ( err < JET_errSuccess )
            {
                err = ErrERRCheck( errDiskTilt );

                (void)FThrottleRelease( m_pfsconfig, ulDiskId, fAsync, fRead, grbitQOS, pfThrottleReleaser, &waiter );
                *pfCombined = fFalse;
            }
            return err;
        }

        BOOL FThrottleRelease(  _In_    const BOOL      fAsync,
                                _In_    const BOOL      fRead,
                                _In_    const OSFILEQOS grbitQOS,
                                _Inout_ BOOL* const     pfThrottleReleaser )
        {
            BOOL        fThrottleReleaser   = *pfThrottleReleaser;
            ULONG_PTR   ulDiskId            = 0;

            *pfThrottleReleaser = fFalse;

            if ( !fThrottleReleaser )
            {
                return fFalse;
            }

            CallS( ErrDiskId( &ulDiskId ) );

            return FThrottleRelease( m_pfsconfig, ulDiskId, fAsync, fRead, grbitQOS, &fThrottleReleaser );
        }

    private:

        typedef CInitOnce< ERR, decltype( &ErrAttach_ ), TFileFilter<I>* const, const COffsets& > CInitOnceAttach;

        IFileSystemFilter* const                                    m_pfsf;
        IFileSystemConfiguration* const                             m_pfsconfig;
        IFileIdentification* const                                  m_pfident;
        ICacheTelemetry* const                                      m_pctm;
        ICacheRepository* const                                     m_pcrep;
        CReaderWriterLock                                           m_rwlRegisterIFilePerfAPI;
        BOOL                                                        m_fRegisteredIFilePerfAPI;
        VolumeId                                                    m_volumeid;
        FileId                                                      m_fileid;
        FileSerial                                                  m_fileserial;
        BOOL                                                        m_fEverEligibleForCaching;
        ICachedFileConfiguration*                                   m_pcfconfig;
        ICache*                                                     m_pc;
        CCacheWrapper                                               m_cWrapper;
        CCachedFileHeader*                                          m_pcfh;
        ICacheTelemetry::FileNumber                                 m_filenumber;
        ULONG                                                       m_cbBlockSize;
        COffsets                                                    m_offsetsCachedFileHeader;
        CMeteredSection                                             m_msPendingWriteBacks;
        CArray<COffsets>*                                           m_rgparrayPendingWriteBacks[ 2 ];
        CCriticalSection                                            m_critWaiters;
        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>                m_ilWaiters;
        CCriticalSection                                            m_critPendingWriteBacks;
        size_t                                                      m_cPendingWriteBacksMax;
        CArray<COffsets>*                                           m_rgparrayUnused[ 2 ];
        CCountedInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>   m_ilQueuedWriteBacks;
        CCountedInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>   m_ilRequestedWriteBacks;
        CSemaphore                                                  m_semRequestWriteBacks;
        CArray<COffsets>                                            m_rgarrayOffsets[ 4 ];
        volatile int                                                m_cCacheWriteForFlush;
        volatile int                                                m_cCacheWriteBackForIssue;
        CInitOnceAttach                                             m_initOnceAttach;
        CSemaphore                                                  m_semCachedFileHeader;
        volatile LONG64                                             m_cioUnflushed;
        volatile LONG64                                             m_cioFlushing;

    private:

        void BeginIORequest()
        {
            //  disallow registration of IFilePerfAPI during any IO request

            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
            m_rwlRegisterIFilePerfAPI.EnterAsReader();
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }

        void EndIORequest()
        {
            //  disallow registration of IFilePerfAPI during any IO request

            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
            m_rwlRegisterIFilePerfAPI.LeaveAsReader();
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }

        //  context used to track pending IO requests

        class CIORequestPending
        {
            public:

                CIORequestPending( _In_ TFileFilter<I>* const pff )
                    :   m_pff( pff ),
                        m_fComplete( fFalse )
                {
                    m_pff->BeginIORequest();
                }

                ~CIORequestPending()
                {
                    Complete();
                }

                void Complete()
                {
                    if ( AtomicCompareExchange( (LONG*)&m_fComplete, fFalse, fTrue ) == fFalse )
                    {
                        m_pff->EndIORequest();
                    }
                }

            private:

                TFileFilter<I>* const   m_pff;
                volatile BOOL           m_fComplete;
        };

    protected:

        //  IO completion context for an IFileFilter implementation.

        class CIOComplete
            :   public TFileWrapper<I>::CIOComplete
        {
            public:

                CIOComplete(    _In_                    const BOOL                      fIsHeapAlloc,
                                _In_                    TFileFilter<I>* const           pff,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const BOOL                      fWrite,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _Inout_opt_             CMeteredSection::Group* const   pgroupPendingWriteBacks,
                                _Inout_opt_             CSemaphore** const              ppsemCachedFileHeader,
                                _In_opt_                CWriteBack* const               pwriteback,
                                _Inout_opt_             BOOL* const                     pfThrottleReleaser,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_opt_                const DWORD_PTR                 keyIOComplete )
                    :   TFileWrapper<I>::CIOComplete(   fIsHeapAlloc,
                                                        pff, 
                                                        ibOffset, 
                                                        cbData, 
                                                        pbData,
                                                        pfnIOComplete,
                                                        pfnIOHandoff,
                                                        keyIOComplete ),
                        m_pff( pff ),
                        m_iom( iom ),
                        m_fAsync( pfnIOComplete != NULL ),
                        m_fWrite( fWrite ),
                        m_grbitQOS( grbitQOS ),
                        m_groupPendingWriteBacks( pgroupPendingWriteBacks ? *pgroupPendingWriteBacks : CMeteredSection::groupInvalidNil ),
                        m_psemCachedFileHeader( ppsemCachedFileHeader ? *ppsemCachedFileHeader : NULL ),
                        m_pwriteback( pwriteback ),
                        m_fReleaseWriteback( m_pwriteback != NULL ),
                        m_fThrottleReleaser( pfThrottleReleaser ? *pfThrottleReleaser : fFalse ),
                        m_fReleaseResources( fTrue ),
                        m_iorequestpending( pff )
                {
                    if ( pgroupPendingWriteBacks )
                    {
                        *pgroupPendingWriteBacks = CMeteredSection::groupInvalidNil;
                    }
                    if ( ppsemCachedFileHeader )
                    {
                        *ppsemCachedFileHeader = NULL;
                    }
                    if ( pfThrottleReleaser )
                    {
                        *pfThrottleReleaser = fFalse;
                    }
                }

                BOOL FAccessingHeader() const { return m_psemCachedFileHeader != NULL; }

                void DoNotReleaseWriteBack()
                {
                    m_fReleaseWriteback = fFalse;
                }
 
                static void Complete_(  _In_                    const ERR               err,
                                        _In_                    const VolumeId          volumeid,
                                        _In_                    const FileId            fileid,
                                        _In_                    const FileSerial        fileserial,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyComplete )
                {
                    ICache::PfnComplete pfnComplete = Complete_;
                    Unused( pfnComplete );

                    CIOComplete* const piocomplete = (CIOComplete*)keyComplete;
                    piocomplete->IOComplete( err, tc, grbitQOS );
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

                    CIOComplete* const piocomplete = (CIOComplete*)keyIOComplete;
                    piocomplete->IOComplete( err, tc, grbitQOS );
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

                    CIOComplete* const piocomplete = (CIOComplete*)keyIOComplete;
                    piocomplete->IOHandoff( err, tc, grbitQOS );
                }

            protected:

                ~CIOComplete()
                {
                    ReleaseResources();
                }

                void CleanupBeforeAsyncIOCompletion()
                {
                    ReleaseResources();

                    TFileWrapper<I>::CIOComplete::CleanupBeforeAsyncIOCompletion();
                }

            private:

                CIOComplete( _In_ const CIOComplete& other );

                void ReleaseResources()
                {
                    if ( AtomicCompareExchange( (LONG*)&m_fReleaseResources, fTrue, fFalse ) == fTrue )
                    {
                        (void)FReleaseThrottle();
                        m_pff->EndAccess( &m_groupPendingWriteBacks, &m_psemCachedFileHeader );
                        m_pff->CompleteWriteBack( m_fReleaseWriteback ? m_pwriteback : NULL );
                        m_iorequestpending.Complete();
                    }
                }

                BOOL FReleaseThrottle()
                {
                    return m_pff->FThrottleRelease( m_fAsync, m_fWrite == fFalse, m_grbitQOS, &m_fThrottleReleaser );
                }

                void IOComplete(    _In_ const ERR                  err,
                                    _In_ const FullTraceContext&    tc,
                                    _In_ const OSFILEQOS            grbitQOS )
                {
                    //  note any unflushed async engine write
                    
                    if ( m_fAsync && m_iom == iomEngine && m_fWrite )
                    {
                        AtomicAdd( (QWORD*)&m_pff->m_cioUnflushed, 1 );
                    }
                    
                    //  pass on the return signals from the lower layer

                    OSFILEQOS grbitQOSOutput = m_grbitQOS & qosIOInMask;
                    grbitQOSOutput |= ( grbitQOS & qosIOCompleteMask & ~( qosIOCompleteIoCombined | qosIOCompleteWriteGameOn ) );

                    //  if throttling has dropped low enough then send the signal to resume background writes

                    if ( FReleaseThrottle() )
                    {
                        grbitQOSOutput |= qosIOCompleteWriteGameOn;
                    }

                    //  continue the completion

                    TFileWrapper<I>::CIOComplete::IOComplete( err, tc, grbitQOSOutput );
                }
                
                void IOHandoff( _In_ const ERR                  err,
                                _In_ const FullTraceContext&    tc,
                                _In_ const OSFILEQOS            grbitQOS )
                {
                    //  pass on the return signals from the lower layer

                    OSFILEQOS grbitQOSOutput = m_grbitQOS & qosIOInMask;
                    grbitQOSOutput |= ( grbitQOS & qosIOCompleteMask & ~( qosIOCompleteIoCombined | qosIOCompleteWriteGameOn ) );

                    //  continue the completion

                    TFileWrapper<I>::CIOComplete::IOHandoff( err, tc, grbitQOSOutput );
                }

            private:

                TFileFilter<I>* const       m_pff;
                const IFileFilter::IOMode   m_iom;
                const BOOL                  m_fAsync;
                const BOOL                  m_fWrite;
                const OSFILEQOS             m_grbitQOS;
                CMeteredSection::Group      m_groupPendingWriteBacks;
                CSemaphore*                 m_psemCachedFileHeader;
                CWriteBack* const           m_pwriteback;
                BOOL                        m_fReleaseWriteback;
                BOOL                        m_fThrottleReleaser;
                volatile BOOL               m_fReleaseResources;
                CIORequestPending           m_iorequestpending;
        };
};

template< class I >
TFileFilter<I>::TFileFilter(    _Inout_     IFileAPI** const                    ppfapi,
                                _In_        IFileSystemFilter* const            pfsf,
                                _In_        IFileSystemConfiguration* const     pfsconfig,
                                _In_        IFileIdentification* const          pfident,
                                _In_        ICacheTelemetry* const              pctm,
                                _In_        ICacheRepository* const             pcrep,
                                _In_        const VolumeId                      volumeid,
                                _In_        const FileId                        fileid,
                                _Inout_opt_ ICachedFileConfiguration** const    ppcfconfig,
                                _In_        const BOOL                          fEverEligibleForCaching,
                                _Inout_opt_ ICache** const                      ppc,
                                _Inout_opt_ CCachedFileHeader** const           ppcfh )
    :   TFileWrapper<I>( (I** const)ppfapi ),
        m_pfsf( pfsf ),
        m_pfsconfig( pfsconfig ),
        m_pfident( pfident ),
        m_pctm( pctm ),
        m_pcrep( pcrep ),
        m_rwlRegisterIFilePerfAPI( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_rwlRegisterIFilePerfAPI" ), rankRegisterIFilePerfAPI, 0 ) ),
        m_fRegisteredIFilePerfAPI( fFalse ),
        m_volumeid( volumeid ),
        m_fileid( fileid ),
        m_fileserial( fileserialInvalid ),
        m_fEverEligibleForCaching( fEverEligibleForCaching ),
        m_pcfconfig( ppcfconfig ? *ppcfconfig : NULL ),
        m_pc( ppc && *ppc ? &m_cWrapper : NULL ),
        m_cWrapper( this, ppc ),
        m_pcfh( ppcfh ? *ppcfh : NULL ),
        m_filenumber( filenumberInvalid ),
        m_cbBlockSize( cbCachedBlock ),
        m_offsetsCachedFileHeader( 0, sizeof( CCachedFileHeader ) - 1 ),
        m_critWaiters( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_critWaiters" ), rankFileFilter, 0 ) ),
        m_critPendingWriteBacks( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_critPendingWriteBacks" ), rankFileFilter, 0 ) ),
        m_cPendingWriteBacksMax( 0 ),
        m_semRequestWriteBacks( CSyncBasicInfo( "TFileFilter<I>::m_semRequestWriteBacks" ) ),
        m_cCacheWriteForFlush( 0 ),
        m_cCacheWriteBackForIssue( 0 ),
        m_semCachedFileHeader( CSyncBasicInfo( "TFileFilter<I>::m_semCachedFileHeader" ) ),
        m_cioUnflushed( 0 ),
        m_cioFlushing( 0 )
{
    SetCacheParameters();
    m_rgparrayPendingWriteBacks[ 0 ] = &m_rgarrayOffsets[ 0 ];
    m_rgparrayPendingWriteBacks[ 1 ] = &m_rgarrayOffsets[ 1 ];
    m_rgparrayUnused[ 0 ] = &m_rgarrayOffsets[ 2 ];
    m_rgparrayUnused[ 1 ] = &m_rgarrayOffsets[ 3 ];
    m_semRequestWriteBacks.Release( 1 );
    m_semCachedFileHeader.Release( 1 );

    if ( ppcfconfig )
    {
        *ppcfconfig = NULL;
    }
    if ( ppcfh )
    {
        *ppcfh = NULL;
    }
}

template< class I >
TFileFilter<I>::~TFileFilter()
{
    OSTrace( JET_tracetagBlockCache, OSFormat( "%s dtor", OSFormat( this ) ) );

    m_semRequestWriteBacks.Acquire();
    delete m_pcfh;
    delete m_pcfconfig;
}

template<class I>
void TFileFilter<I>::Cleanup()
{
    CIOComplete::Cleanup();
    CThreadLocalStorageRepository::Cleanup();
    CThrottleContextRepository::Cleanup();
}

template< class I >
ERR TFileFilter<I>::ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                        _Out_ FileId* const     pfileid,
                                        _Out_ FileSerial* const pfileserial )
{
    *pvolumeid = m_volumeid;
    *pfileid = m_fileid;
    *pfileserial = m_fileserial;

    return JET_errSuccess;
}

template< class I >
ERR TFileFilter<I>::ErrFlushFileBuffers( _In_ const IOFLUSHREASON iofr )
{
    ERR err = JET_errSuccess;

    const LONG64 ciosDelta = AtomicExchange( &m_cioUnflushed, 0 );
    AtomicAdd( (QWORD*)&m_cioFlushing, ciosDelta );

    Call( ErrFlush( iofr, iomEngine ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        AtomicAdd( (QWORD*)&m_cioUnflushed, ciosDelta );
    }
    AtomicAdd( (QWORD*)&m_cioFlushing, -ciosDelta );
    return err;
}

template< class I >
void TFileFilter<I>::SetNoFlushNeeded()
{
    AtomicExchange( &m_cioUnflushed, 0 );

    TFileWrapper<I>::SetNoFlushNeeded();
}

template< class I >
ERR TFileFilter<I>::ErrSetSize( _In_ const TraceContext&    tc,
                                _In_ const QWORD            cbSize,
                                _In_ const BOOL             fZeroFill,
                                _In_ const OSFILEQOS        grbitQOS )
{
    ERR                     err             = JET_errSuccess;
    QWORD                   cbSizeCurrent   = 0;
    QWORD                   cbSizeNew       = cbSize;
    CMeteredSection::Group  group           = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem            = NULL;

    //  get the current size of the file

    Call( TFileWrapper<I>::ErrSize( &cbSizeCurrent, IFileAPI::filesizeLogical ) );

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrSetSize %llu -> %llu", OSFormat( this ), cbSizeCurrent, cbSizeNew ) );

    //  protect all impacted offsets in the file from write back

    Call( ErrBeginAccess( COffsets( min( cbSizeCurrent, cbSizeNew ), qwMax ), fTrue, &group, &psem ) );

    //  invalidate all offsets beyond the new end of the file

    Call( ErrInvalidate( COffsets( cbSizeNew, qwMax ) ) );

    //  change the file size
    //
    //  NOTE:  we will not truncate the header if we are attached.  ideally we would detach the file from the cache
    //  and truly set the file to zero size.  we will defer this work until later

    if ( FAttached() )
    {
        cbSizeNew = max( cbSizeNew, m_offsetsCachedFileHeader.Cb() );
    }

    Call( TFileWrapper<I>::ErrSetSize( tc, cbSizeNew, fZeroFill, grbitQOS ) );

HandleError:
    EndAccess( &group, &psem );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrRename(  _In_z_  const WCHAR* const  wszPathDest,
                                _In_    const BOOL          fOverwriteExisting )
{
    OSTrace(    JET_tracetagBlockCache,
                OSFormat(   "%s ErrRename %ws fOverwriteExisting=%s", 
                            OSFormat( this ),
                            wszPathDest, 
                            OSFormatBoolean( fOverwriteExisting ) ) );

    return m_pfsf->ErrFileRename( m_piInner, wszPathDest, fOverwriteExisting );
}

template< class I >
ERR TFileFilter<I>::ErrIOTrim(  _In_ const TraceContext&    tc,
                                _In_ const QWORD            ibOffset,
                                _In_ const QWORD            cbToFree )
{
    ERR                     err     = JET_errSuccess;
    COffsets                offsets = COffsets( ibOffset, ibOffset + cbToFree - 1 );
    CMeteredSection::Group  group   = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem    = NULL;

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrIOTrim ib=%llu cb=%llu", OSFormat( this ), ibOffset, cbToFree ) );

    //  ignore zero length trims

    if ( cbToFree > 0 )
    {
        //  protect this offset range in the file from write back

        Call( ErrBeginAccess( offsets, fTrue, &group, &psem ) );

        //  invalidate this offset range in the file

        Call( ErrInvalidate( offsets ) );

        //  trim the file
        //
        //  NOTE:  we will not trim the header if we are attached

        QWORD cbHeaderTrimmed = FAttached() && offsets.FOverlaps( m_offsetsCachedFileHeader ) ?
            min( offsets.Cb(), m_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 ) :
            0;

        if ( cbToFree > cbHeaderTrimmed )
        {
            Call( TFileWrapper<I>::ErrIOTrim( tc, ibOffset + cbHeaderTrimmed, cbToFree - cbHeaderTrimmed ) );
        }
    }

HandleError:
    EndAccess( &group, &psem );
    return err;
}

 
template< class I >
ERR TFileFilter<I>::ErrReserveIOREQ(    _In_    const QWORD     ibOffset,
                                        _In_    const DWORD     cbData,
                                        _In_    const OSFILEQOS grbitQOS,
                                        _Out_   VOID **         ppioreq )
{
    ERR                     err                 = JET_errSuccess;
    OSFILEQOS               grbitQOSOutput      = grbitQOS & ( qosIOInMask | qosBypassIOQuota );
    COffsets                offsets             = COffsets( ibOffset, ibOffset + cbData - 1 );
    BOOL                    fThrottleReleaser   = fFalse;
    BOOL                    fCombined           = fFalse;
    CReservedIOREQ*         preservedIOREQ      = NULL;

    *ppioreq = NULL;

    Call( ErrThrottleCheck( fTrue, fTrue, offsets, grbitQOS, &fThrottleReleaser, &fCombined ) );

    //  set the IO combined state for output

    if ( fCombined )
    {
        grbitQOSOutput |= qosIOCompleteIoCombined;
    }

    Alloc( preservedIOREQ = new CReservedIOREQ( this, grbitQOSOutput, &fThrottleReleaser ) );

    *ppioreq = preservedIOREQ;
    preservedIOREQ = NULL;

HandleError:
    ReleaseUnusedIOREQ( &preservedIOREQ );
    (void)FThrottleRelease( fTrue, fTrue, grbitQOS, &fThrottleReleaser );
    if ( err < JET_errSuccess )
    {
        err = ErrERRCheck( errDiskTilt );

        ReleaseUnusedIOREQ( ppioreq );
    }
    return err;
}

template< class I >
VOID TFileFilter<I>::ReleaseUnusedIOREQ( _In_ VOID * pioreq )
{
    ReleaseUnusedIOREQ( &pioreq );
}

template< class I >
ERR TFileFilter<I>::ErrIORead(  _In_                    const TraceContext&                 tc,
                                _In_                    const QWORD                         ibOffset,
                                _In_                    const DWORD                         cbData,
                                _Out_writes_( cbData )  __out_bcount( cbData ) BYTE* const  pbData,
                                _In_                    const OSFILEQOS                     grbitQOS,
                                _In_opt_                const IFileAPI::PfnIOComplete       pfnIOComplete,
                                _In_opt_                const DWORD_PTR                     keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                                _In_opt_                const VOID *                        pioreq )
{
    return ErrRead( tc, ibOffset, cbData, pbData, grbitQOS, iomEngine, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq );
}

template< class I >
ERR TFileFilter<I>::ErrIOWrite( _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    return ErrWrite( tc, ibOffset, cbData, pbData, grbitQOS, iomEngine, pfnIOComplete, keyIOComplete, pfnIOHandoff );
}

template< class I >
ERR TFileFilter<I>::ErrIOIssue()
{
    return ErrIssue( iomEngine );
}

template< class I >
void TFileFilter<I>::RegisterIFilePerfAPI( _In_ IFilePerfAPI* const pfpapi )
{
    //  ensure any previously requested IO is issued to avoid deadlock during registration

    CallS( ErrIOIssue() );

    //  disallow registration of IFilePerfAPI during any IO request

    m_rwlRegisterIFilePerfAPI.EnterAsWriter();

    //  if we already registered an IFilePerfAPI then drop this one, otherwise register it

    if ( m_fRegisteredIFilePerfAPI )
    {
        delete pfpapi;
    }
    else
    {
        TFileWrapper<I>::RegisterIFilePerfAPI( pfpapi );

        m_fRegisteredIFilePerfAPI = fTrue;
    }

    m_rwlRegisterIFilePerfAPI.LeaveAsWriter();
}

template< class I >
LONG64 TFileFilter<I>::CioNonFlushed() const
{
    return AtomicRead( (QWORD*)&m_cioUnflushed ) + AtomicRead( (QWORD*)&m_cioFlushing );
}

template< class I >
ERR TFileFilter<I>::ErrRead(    _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _Out_writes_( cbData )  BYTE* const                     pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_opt_                const VOID *                    pioreq )
{
    ERR                     err                 = JET_errSuccess;
    COffsets                offsets             = COffsets( ibOffset, ibOffset + cbData - 1 );
    CIORequestPending       iorequestpending( this );
    CMeteredSection::Group  group               = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem                = NULL;

    Assert( cbData > 0 );  //  underlying impl also asserts no zero length IO
    Assert( iom == iomRaw || iom == iomEngine || iom == iomCacheMiss );

    //  if this read is from the engine and not the cache then we need to protect this offset range in the file from
    //  colliding with cache write back.  if the read is from the cache then it is the cache doing a read inside
    //  the scope of the original engine read and thus it is already protected.  the cache cannot do a caching read
    //  outside of the original scope (e.g. a prefetch read of additional offsets) without a potential deadlock in this
    //  locking model.  we check for such requests and fail them

    if ( iom == iomEngine )
    {
        Call( ErrBeginAccess( offsets, fFalse, &group, &psem ) );
        Call( ErrCacheRead( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &pioreq, &group, &psem ) );
    }
    else if ( iom == iomCacheMiss )
    {
        Call( ErrVerifyAccess( offsets ) );
        Call( ErrCacheMiss( tc, ibOffset, cbData, pbData, grbitQOS, grbitQOS, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &pioreq, &group, &psem, NULL ) );
    }
    else
    {
        ReleaseUnusedIOREQ( &pioreq);

        Call( TFileWrapper<I>::ErrIORead( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, NULL ) );
    }

HandleError:
    EndAccess( &group, &psem );
    OSTrace( JET_tracetagBlockCache,
        OSFormat( "%s ErrIORead ib=%llu cb=%u fZeroed=%s fCachedFileHeader=%s iom=%d",
            OSFormat( this ),
            ibOffset,
            cbData,
            err >= JET_errSuccess ? OSFormatBoolean( FUtilZeroed( pbData, cbData ) ) : "err",
            err >= JET_errSuccess ? OSFormatBoolean( CCachedFileHeader::FValid( m_pfsconfig, pbData, cbData ) ) : "err",
            iom ) );
    return HandleReservedIOREQ( tc, 
                                ibOffset, 
                                cbData, 
                                pbData,
                                grbitQOS,
                                pfnIOComplete,
                                keyIOComplete, 
                                pfnIOHandoff,
                                pioreq,
                                err );
}

template< class I >
ERR TFileFilter<I>::ErrWrite(   _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR                     err                 = JET_errSuccess;
    COffsets                offsets             = COffsets( ibOffset, ibOffset + cbData - 1 );
    CIORequestPending       iorequestpending( this );
    CMeteredSection::Group  group               = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem                = NULL;

    Assert( cbData > 0 );  //  underlying impl also asserts no zero length IO
    Assert( iom == iomRaw ||
            iom == iomEngine || 
            iom == iomCacheWriteThrough || 
            iom == iomCacheWriteBack );

    OSTrace(    JET_tracetagBlockCache,
                OSFormat(   "%s ErrIOWrite ib=%llu cb=%u fZeroed=%s fCachedFileHeader=%s iom=%d", 
                            OSFormat( this ), 
                            ibOffset, 
                            cbData, 
                            OSFormatBoolean( FUtilZeroed( pbData, cbData ) ),
                            OSFormatBoolean( CCachedFileHeader::FValid( m_pfsconfig, pbData, cbData ) ),
                            iom ) );

    //  if this write is from the engine and not the cache then we need to protect this offset range in the file from
    //  colliding with cache write back.  if this is a cache write through then the write should be inside the scope of
    //  the original engine write and thus it is already protected.  the cache cannot do a write through outside of the
    //  original scope without a potential deadlock in this locking model.  we check for such requests and fail them.
    //  if this is a cache write back then we need to wait until any engine IOs to this offset range are complete
    //  before proceeding

    if ( iom == iomEngine )
    {
        Call( ErrBeginAccess( offsets, fTrue, &group, &psem ) );
        Call( ErrCacheWrite( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &group, &psem ) );
    }
    else if ( iom == iomCacheWriteThrough )
    {
        Call( ErrVerifyAccess( offsets ) );
        Call( ErrWriteThrough( iomCacheWriteThrough, tc, ibOffset, cbData, pbData, grbitQOS, grbitQOS, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &group, &psem, NULL ) );
    }
    else if ( iom == iomCacheWriteBack )
    {
        Call( ErrTryEnqueueWriteBack( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff ) );
    }
    else
    {
        Call( TFileWrapper<I>::ErrIOWrite( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff ) );
    }

HandleError:
    EndAccess( &group, &psem );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrIssue( _In_ const IFileFilter::IOMode iom )
{
    ERR                     err         = JET_errSuccess;
    CThreadLocalStorage*    pctls       = NULL;
    BOOL                    fIssue      = fFalse;
    BOOL                    fIssueCache = fFalse;

    Assert( iom == iomRaw ||
            iom == iomEngine ||
            iom == iomCacheMiss ||
            iom == iomCacheWriteThrough ||
            iom == iomCacheWriteBack );

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrIOIssue iom=%u", OSFormat( this ), iom ) );

    CallSx( CThreadLocalStorageRepository::ErrGetThreadLocalStorage( &pctls ), JET_errOutOfMemory );
    if ( pctls )
    {
        pctls->ClearRequests();
    }

    switch ( iom )
    {
        case iomRaw:
            fIssue = fTrue;
            break;

        case iomEngine:
            fIssue = fTrue;
            fIssueCache = m_pc != NULL;
            break;

        case iomCacheMiss:
            fIssue = fTrue;
            break;

        case iomCacheWriteThrough:
            fIssue = fTrue;
            break;

        case iomCacheWriteBack:
            if ( AtomicExchange( (LONG*)&m_cCacheWriteBackForIssue, 0 ) > 0 )
            {
                IssueWriteBacks();
            }
            break;
    }

    if ( fIssueCache )
    {
        Call( m_pc->ErrIssue( m_volumeid, m_fileid, m_fileserial ) );
    }

    if ( fIssue )
    {
        Call( TFileWrapper<I>::ErrIOIssue() );
    }

HandleError:
    CThreadLocalStorage::Release( &pctls );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrFlush( _In_ const IOFLUSHREASON iofr, _In_ const IFileFilter::IOMode iom )
{
    ERR     err         = JET_errSuccess;
    BOOL    fFlush      = fFalse;
    BOOL    fFlushCache = fFalse;

    Assert( iom == iomRaw ||
            iom == iomEngine ||
            iom == iomCacheMiss ||
            iom == iomCacheWriteThrough ||
            iom == iomCacheWriteBack );

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrFlushFileBuffers iom=%u", OSFormat( this ), iom ) );

    switch ( iom )
    {
        case iomRaw:
            fFlush = fTrue;
            break;

        case iomEngine:
            fFlush = fTrue;
            fFlushCache = AtomicExchange( (LONG*)&m_cCacheWriteForFlush, 0 ) > 0;
            break;

        case iomCacheMiss:
            fFlush = fTrue;
            break;

        case iomCacheWriteThrough:
            fFlush = fTrue;
            break;

        case iomCacheWriteBack:
            fFlush = fTrue;
            break;
    }

    if ( fFlushCache )
    {
        Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );
    }

    if ( fFlush )
    {
        Call( TFileWrapper<I>::ErrFlushFileBuffers( iofr ) );
    }

HandleError:
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrBeginAccess( _In_    const COffsets&                 offsets, 
                                    _In_    const BOOL                      fWrite,
                                    _Out_   CMeteredSection::Group* const   pgroup,
                                    _Out_   CSemaphore** const              ppsem )
{
    ERR                     err             = JET_errSuccess;
    const BOOL              fNotYetAttached = !m_pcfh && !m_initOnceAttach.FIsInit();
    const BOOL              fNeedsAttach    = fWrite && fNotYetAttached && m_pcfconfig && m_pcfconfig->FCachingEnabled();
    COffsets                offsetsActual   = offsets;
    CMeteredSection::Group  group           = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem            = NULL;

    *pgroup = CMeteredSection::groupInvalidNil;
    *ppsem = NULL;

    //  if this is a write and the file is cached and not yet attached then expand the offset range to include the
    //  header so that we can write our cached file header

    if ( fNeedsAttach )
    {
        offsetsActual = COffsets( 0, offsetsActual.IbEnd() );
    }

    //  determine which list of pending write backs are effective for this IO

    group = m_msPendingWriteBacks.Enter();

    //  if we cannot access this offset range of the file due to pending write backs then wait until we can

    if ( !FAccessPermitted( offsetsActual, group ) )
    {
        WaitForAccess( offsetsActual, &group );

        Assert( FAccessPermitted( offsetsActual, group ) );
    }

    //  if this IO is accessing the offset range that may contain the cached file header and the file is not yet
    //  attached then serialize access to that offset range to ensure the IO and the attach do not conflict

    if ( fNotYetAttached && offsetsActual.FOverlaps( m_offsetsCachedFileHeader ) )
    {
        psem = &m_semCachedFileHeader;
        psem->Acquire();
    }

    //  if we need to attach the file to the cache then do so now

    if ( fNeedsAttach )
    {
        Call( m_initOnceAttach.Init( ErrAttach_, this, offsets ) );

        psem->Release();
        psem = NULL;
    }

    //  return the list of pending write backs that are effective for this IO

    *pgroup = group;
    group = CMeteredSection::groupInvalidNil;

    //  return the lock controlling access to the header for this IO, if applicable

    *ppsem = psem;
    psem = NULL;

HandleError:
    EndAccess( &group, &psem );
    if ( err < JET_errSuccess )
    {
        *pgroup = CMeteredSection::groupInvalidNil;
        *ppsem = NULL;
    }
    return err;
}

template< class I >
VOID TFileFilter<I>::WaitForAccess( _In_ const COffsets& offsets, _Inout_ CMeteredSection::Group* const pgroup )
{
    CWaiter waiter( offsets );

    m_critWaiters.Enter();
    m_ilWaiters.InsertAsNextMost( &waiter );
    m_critWaiters.Leave();

    m_msPendingWriteBacks.Leave( *pgroup );
    *pgroup = CMeteredSection::groupInvalidNil;

    *pgroup = waiter.Wait();
}

template< class I >
BOOL TFileFilter<I>::FAccessPermitted( _In_ const COffsets& offsets, _In_ const CMeteredSection::Group group )
{
    if ( group == CMeteredSection::groupInvalidNil )
    {
        return fFalse;
    }

    const CArray<COffsets>& arrayPendingWriteBacks = *m_rgparrayPendingWriteBacks[ group ];
    const size_t c = arrayPendingWriteBacks.Size();

    for ( size_t i = 0; i < c; i++ )
    {
        if ( offsets.FOverlaps( arrayPendingWriteBacks[ i ] ) )
        {
            return fFalse;
        }
    }

    return fTrue;
}

template< class I >
ERR TFileFilter<I>::ErrVerifyAccessIgnoringWriteBack( _In_ const COffsets& offsets )
{
    ERR err = JET_errSuccess;

    //  if the file is not attached to the cache then the cache had better not be accessing it

    if ( !FAttached() )
    {
        Error( ErrBlockCacheInternalError( "IllegalCachedFileAccess" ) );
    }

    //  the cache is not allowed to access the cached file header

    if ( FAttached() && offsets.FOverlaps( m_offsetsCachedFileHeader ) )
    {
        Error( ErrBlockCacheInternalError( "IllegalCachedFileHeaderAccess" ) );
    }

HandleError:
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrVerifyAccess( _In_ const COffsets& offsets )
{
    ERR err = JET_errSuccess;

    //  verify that this access is allowed ignoring pending write backs

    Call( ErrVerifyAccessIgnoringWriteBack( offsets ) );

    //  if the requested offset range conflicts with a write back then we cannot allow the cache to access these
    //  offsets or it could cause a deadlock

    const CMeteredSection::Group group = m_msPendingWriteBacks.GroupActive();
    if ( !FAccessPermitted( offsets, group ) )
    {
        Error( ErrBlockCacheInternalError( "CachedFileIORangeConflict" ) );
    }

HandleError:
    return err;
}

template< class I >
VOID TFileFilter<I>::EndAccess( _Inout_opt_ CMeteredSection::Group* const   pgroup,
                                _Inout_opt_ CSemaphore** const              ppsem )
{
    const CMeteredSection::Group    group   = *pgroup;
    CSemaphore* const               psem    = *ppsem;

    *pgroup = CMeteredSection::groupInvalidNil;
    *ppsem = NULL;

    if ( group != CMeteredSection::groupInvalidNil )
    {
        m_msPendingWriteBacks.Leave( group );
    }

    if ( psem )
    {
        psem->Release();
    }
}

template< class I >
ERR TFileFilter<I>::ErrInvalidate( _In_ const COffsets& offsets )
{
    ERR                 err                 = JET_errSuccess;
    DWORD               cbHeaderInvalidated = 0;
    BYTE*               rgbZero             = NULL;
    TraceContextScope   tcScope( iorpBlockCache );

    //  we only need to invalidate the cache if we are attached

    if ( FAttached() )
    {
        //  if we are invalidating the header area then explicitly clear the affected area.  we cannot invalidate it
        //  because that would remove the pinned state which would allow the application to see the header.  the best
        //  we can do is return zeroes here

        if ( offsets.FOverlaps( m_offsetsCachedFileHeader ) && offsets.Cb() > 0 )
        {
            cbHeaderInvalidated = (DWORD)min( offsets.Cb(), m_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 );

            Alloc( rgbZero = (BYTE*)PvOSMemoryPageAlloc( roundup( cbHeaderInvalidated, OSMemoryPageCommitGranularity() ), NULL ) );

            Call( m_pc->ErrWrite(   *tcScope,
                                    m_volumeid,
                                    m_fileid,
                                    m_fileserial,
                                    offsets.IbStart(),
                                    cbHeaderInvalidated,
                                    rgbZero,
                                    qosIONormal,
                                    cpPinned,
                                    NULL,
                                    NULL ) );
        }

        //  invalidate the cache for this offset range

        Call( m_pc->ErrInvalidate(  m_volumeid, 
                                    m_fileid, 
                                    m_fileserial, 
                                    offsets.IbStart() + cbHeaderInvalidated,
                                    offsets.Cb() - cbHeaderInvalidated ) );

        //  flush the cache to insure this data stays invalidated

        Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );
    }

HandleError:
    OSMemoryPageFree( rgbZero );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrAttach( _In_ const COffsets& offsetsFirstWrite )
{
    ERR                         err                         = JET_errSuccess;
    QWORD                       cbSize                      = 0;
    QWORD                       cbSizeWithFirstWrite        = 0;
    void*                       pvData                      = NULL;
    TraceContextScope           tcScope( iorpBlockCache );
    CCachedFileHeader*          pcfh                        = NULL;
    BOOL                        fPresumeCached              = fFalse;
    BOOL                        fPresumeAttached            = fFalse;

    //  if the file has an invalid file id then it is not eligible for caching

    if ( m_volumeid == volumeidInvalid || m_fileid == fileidInvalid )
    {
        Error( JET_errSuccess );
    }

    //  we do not currently support caching files opened for write through

    if ( ( Fmf() & fmfStorageWriteBack ) == 0 )
    {
        Error( JET_errSuccess );
    }

    //  get the size of the cached file and extend to include the proposed write.  if it is smaller than the header
    //  then don't attach it.  our contract is to not cause any noticeable changes to the file meta-data such as by
    //  changing the file size

    Call( TFileWrapper<I>::ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    cbSizeWithFirstWrite = max( cbSize, offsetsFirstWrite.IbEnd() + 1 );

    if ( cbSizeWithFirstWrite < m_offsetsCachedFileHeader.Cb() )
    {
        Error( JET_errSuccess );
    }

    //  if the file is not marked as having ever been eligible for caching then we cannot attach the cache

    if ( !m_fEverEligibleForCaching )
    {
        Error( JET_errSuccess );
    }

    //  try to open the cache for this file.  if we can't then we will access the file without caching.  this is
    //  intentionally best effort only to allow continued operation if the caching storage is failed

    err = ErrGetConfiguredCache();
    if ( err < JET_errSuccess )
    {
        Error( ErrCacheOpenFailure( "Open", err, JET_errSuccess ) );
    }

    //  if we didn't get a cache because caching is not enabled then we cannot attach the cache

    if ( !m_pc )
    {
        Error( JET_errSuccess );
    }

    //  read the data to be displaced by the cached file header, if any

    Alloc( pvData = PvOSMemoryPageAlloc( (size_t)( m_offsetsCachedFileHeader.Cb() ), NULL ) );
    if ( cbSize > 0 )
    {
        Call( ErrRead(  *tcScope, 
                        0, 
                        (DWORD)min( m_offsetsCachedFileHeader.Cb(), cbSize ),
                        (BYTE*)pvData, 
                        qosIONormal, 
                        iomRaw,
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL ) );
    }

    //  create the new cached file header

    Call( CCachedFileHeader::ErrCreate( this, m_pc, (DWORD)m_offsetsCachedFileHeader.Cb(), &pcfh ) );

    //  allow this file to be opened by the cache

    m_fileserial = pcfh->Fileserial();

    //  load the displaced data into the cache on behalf of the cached file once it is attached

    Call( m_pc->ErrWrite(   *tcScope,
                            m_volumeid,
                            m_fileid,
                            m_fileserial,
                            0,
                            (DWORD)m_offsetsCachedFileHeader.Cb(),
                            (const BYTE*)pvData,
                            qosIONormal,
                            cpPinned,
                            NULL,
                            NULL ) );
    fPresumeCached = fTrue;
    Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );

    //  write the header to the file, making it cached.  once this happens we must presume it is attached
    //
    //  NOTE:  if the cached file header doesn't make it to the file then the cache will be loaded with an image of the
    //  initial bytes of this file.  however, it will be keyed to a FileSerial that didn't get stamped on the file.  so
    //  the cache will end up tossing that write later because the target cached file doesn't exist.  also once we fail
    //  we will remove the ability of the cache to open this already open file for write through / write back.  so we 
    //  are not at risk of writing this stale data back to the file

    memset( pvData, 0, (size_t)( m_offsetsCachedFileHeader.Cb() ) );
    UtilMemCpy( pvData, pcfh, sizeof( *pcfh ) );

    Call( ErrWrite( *tcScope, 
                    0, 
                    (DWORD)m_offsetsCachedFileHeader.Cb(), 
                    (const BYTE*)pvData, 
                    qosIONormal, 
                    iomRaw, 
                    NULL, 
                    NULL, 
                    NULL ) );
    fPresumeAttached = fTrue;

    Call( ErrFlushFileBuffers( iofrBlockCache ) );

    //  mark the file as attached by retaining the cached file header.  this will allow cache write through / write back
    //  to the cached file to occur

    m_pcfh = pcfh;
    pcfh = NULL;

    //  if we could not attach the file then ignore the failure and continue running uncached

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( fPresumeCached )
        {
            //  don't allow the cache to open this file anymore 

            m_fileserial = fileserialInvalid;

            //  cause the cache to close its handle to this file

            (void)m_pc->ErrClose( m_volumeid, m_fileid, pcfh->Fileserial() );
        }
    }
    delete pcfh;
    OSMemoryPageFree( pvData );
    return fPresumeAttached ? err : JET_errSuccess;
}

template< class I >
ERR TFileFilter<I>::ErrGetConfiguredCache()
{
    ERR                             err                                                 = JET_errSuccess;
    const DWORD                     cwchAbsPathCachedFileMax                            = IFileSystemAPI::cchPathMax;
    WCHAR                           wszAbsPathCachedFile[ cwchAbsPathCachedFileMax ]    = { 0 };
    const DWORD                     cwchKeyPathCachedFileMax                            = IFileIdentification::cwchKeyPathMax;
    WCHAR                           wszKeyPathCachedFile[ cwchKeyPathCachedFileMax ]    = { 0 };
    IBlockCacheConfiguration*       pbcconfig                                           = NULL;
    const DWORD                     cwchAbsPathCachingFileMax                           = IFileSystemAPI::cchPathMax;
    WCHAR                           wszAbsPathCachingFile[ cwchAbsPathCachingFileMax ]  = { 0 };
    const DWORD                     cwchKeyPathCachingFileMax                           = IFileIdentification::cwchKeyPathMax;
    WCHAR                           wszKeyPathCachingFile[ cwchKeyPathCachingFileMax ]  = { 0 };
    ICacheConfiguration*            pcconfig                                            = NULL;
    ICache*                         pc                                                  = NULL;

    //  if we already have a cache then we're done

    if ( m_pc )
    {
        Error( JET_errSuccess );
    }

    //  get the caching configuration for this file

    Call( ErrPath( wszAbsPathCachedFile ) );
    Call( m_pfident->ErrGetFileKeyPath( wszAbsPathCachedFile, wszKeyPathCachedFile ) );
    Call( m_pfsconfig->ErrGetBlockCacheConfiguration( &pbcconfig ) );

    //  if caching is enabled for this file then open its backing store

    if ( m_pcfconfig->FCachingEnabled() )
    {
        //  get the cache configuration for this file

        m_pcfconfig->CachingFilePath( wszAbsPathCachingFile );
        Call( m_pfident->ErrGetFileKeyPath( wszAbsPathCachingFile, wszKeyPathCachingFile ) );
        Call( pbcconfig->ErrGetCacheConfiguration( wszKeyPathCachingFile, &pcconfig ) );

        //  open the cache for this file

        Call( m_pcrep->ErrOpen( m_pfsf, m_pfsconfig, &pcconfig, &pc ) );

        //  save the file's cache configuration and cache

        SetCacheState( &pc, NULL );
    }

HandleError:
    delete pc;
    delete pcconfig;
    delete pbcconfig;
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrCacheOpenFailure(    _In_ const char* const                  szFunction,
                                            _In_ const ERR                          errFromCall,
                                            _In_ const ERR                          errToReturn )
{
    WCHAR           wszCachingFile[ IFileSystemAPI::cchPathMax ]    = { 0 };
    WCHAR           wszFunction[ 256 ]                              = { 0 };
    WCHAR           wszErrorFromCall[ 64 ]                          = { 0 };
    WCHAR           wszErrorToReturn[ 64 ]                          = { 0 };
    const WCHAR*    rgpwsz[]                                        = { wszCachingFile, wszFunction, wszErrorFromCall, wszErrorToReturn };

    CallS( ErrPath( wszCachingFile ) );
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
ERR TFileFilter<I>::ErrTryEnqueueWriteBack( _In_                    const TraceContext&             tc,
                                            _In_                    const QWORD                     ibOffset,
                                            _In_                    const DWORD                     cbData,
                                            _In_reads_( cbData )    const BYTE* const               pbData,
                                            _In_                    const OSFILEQOS                 grbitQOS,
                                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR                     err         = JET_errSuccess;
    CWriteBack*             pwriteback  = NULL;
    CMeteredSection::Group  group       = CMeteredSection::groupInvalidNil;
    BOOL                    fLeave      = fFalse;
    CWriteBackComplete      writeBackComplete( keyIOComplete, pfnIOHandoff );

    //  verify that this access is allowed ignoring pending write backs

    Call( ErrVerifyAccessIgnoringWriteBack( COffsets( ibOffset, ibOffset + cbData - 1 ) ) );

    //  enter the pending write backs metered section only to stabilize the active group so we can ensure enough
    //  enough storage for the pending write back offset array

    group = m_msPendingWriteBacks.Enter();

    //  protect our access to the pending write back lists

    m_critPendingWriteBacks.Enter();
    fLeave = fTrue;

    //  if we are already at the max pending write backs then reject new requests

    const size_t cPendingWriteBacks = CPendingWriteBacks();
    const size_t cPendingWriteBackMax = CPendingWriteBackMax();

    if ( cPendingWriteBacks >= cPendingWriteBackMax )
    {
        Error( ErrERRCheck( errDiskTilt ) );
    }

    //  ensure that we have enough storage in the pending write back offset array to represent this write back.  we do
    //  this here to prevent OOM during IssueWriteBacks

    if ( m_cPendingWriteBacksMax < cPendingWriteBacks + 1 )
    {
        m_cPendingWriteBacksMax = max( 1, min( cPendingWriteBackMax, 2 * m_cPendingWriteBacksMax ) );
    }

    if ( m_rgparrayUnused[ 0 ]->ErrSetCapacity( m_cPendingWriteBacksMax ) != CArray<COffsets>::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    if ( m_rgparrayUnused[ 1 ]->ErrSetCapacity( m_cPendingWriteBacksMax ) != CArray<COffsets>::ERR::errSuccess )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  queue the new write request

    Alloc( pwriteback = new CWriteBack( tc,
                                        ibOffset,
                                        cbData,
                                        pbData,
                                        grbitQOS,
                                        pfnIOComplete ? pfnIOComplete : CWriteBackComplete::IOComplete_,
                                        pfnIOComplete ? keyIOComplete : (DWORD_PTR)&writeBackComplete,
                                        pfnIOComplete ? pfnIOHandoff : CWriteBackComplete::IOHandoff_ ) );
    m_ilQueuedWriteBacks.InsertAsNextMost( pwriteback );

    AtomicIncrement( (LONG*)&m_cCacheWriteBackForIssue );

HandleError:
    if ( fLeave )
    {
        m_critPendingWriteBacks.Leave();
    }
    if ( group != CMeteredSection::groupInvalidNil )
    {
        m_msPendingWriteBacks.Leave( group );
    }
    if ( err >= JET_errSuccess && pwriteback && !pfnIOComplete )
    {
        CallS( ErrIssue( iomCacheWriteBack ) );
        err = writeBackComplete.ErrComplete();
    }
    return err;
}

template<class I>
size_t TFileFilter<I>::CPendingWriteBacks()
{
    return m_ilRequestedWriteBacks.Count() + m_ilQueuedWriteBacks.Count();
}

template<class I>
size_t TFileFilter<I>::CPendingWriteBackMax()
{
    return m_pcfconfig ? m_pcfconfig->CConcurrentBlockWriteBackMax() : 1;
}

template< class I >
void TFileFilter<I>::IssueWriteBacks()
{
    //  if we can become the write back requestor then we will request write backs

    if ( m_semRequestWriteBacks.FTryAcquire() )
    {
        //  get the inactive group

        CMeteredSection::Group groupInactive = m_msPendingWriteBacks.GroupInactive();

        //  generate the set of offset ranges that correspond to existing and desired write backs

        m_critPendingWriteBacks.Enter();

        const size_t cPendingWriteBacks = CPendingWriteBacks();
        if ( m_rgparrayPendingWriteBacks[ groupInactive ]->Capacity() < cPendingWriteBacks )
        {
            CArray<COffsets>* parrayT = m_rgparrayUnused[ groupInactive ];
            m_rgparrayUnused[ groupInactive ] = m_rgparrayPendingWriteBacks[ groupInactive ];
            m_rgparrayPendingWriteBacks[ groupInactive ] = parrayT;
        }

        CArray<COffsets>& arrayPendingWriteBacks = *m_rgparrayPendingWriteBacks[ groupInactive ];
        Assert( arrayPendingWriteBacks.Capacity() >= CPendingWriteBacks() );
        CArray<COffsets>::ERR errT = arrayPendingWriteBacks.ErrSetSize( CPendingWriteBacks() );
        Assert( errT == CArray<COffsets>::ERR::errSuccess );

        size_t iwriteback = 0;
        GetWriteBackOffsets( arrayPendingWriteBacks, m_ilRequestedWriteBacks, iwriteback );
        GetWriteBackOffsets( arrayPendingWriteBacks, m_ilQueuedWriteBacks, iwriteback );

        m_critPendingWriteBacks.Leave();

        //  lock these offsets for write back by switching active groups.  this will take effect for all new engine IOs
        //  immediately.  however we must wait until any pending engine IOs have completed before we can request new
        //  write backs.  to be continued in RequestWriteBacks

        m_msPendingWriteBacks.Partition( (CMeteredSection::PFNPARTITIONCOMPLETE)TFileFilter<I>::RequestWriteBacks_, (DWORD_PTR)this );
    }
}

template<class I>
void TFileFilter<I>::RequestWriteBacks()
{
    //  release any IOs that are waiting for a pending write back to complete if it is now safe to do so

    ReleaseWaitingIOs();

    //  try to issue as many of these write backs as possible

    size_t cWriteBacksStarted = 0;
    size_t cWriteBacksFailed = 0;
    ERR err = JET_errSuccess;
    while ( err >= JET_errSuccess )
    {
        //  move the oldest queued write back, if any, to the requested write back list

        m_critPendingWriteBacks.Enter();
        CWriteBack* const pwriteback = m_ilQueuedWriteBacks.PrevMost();
        if ( pwriteback )
        {
            m_ilQueuedWriteBacks.Remove( pwriteback );
            m_ilRequestedWriteBacks.InsertAsNextMost( pwriteback );
        }
        m_critPendingWriteBacks.Leave();

        if ( !pwriteback )
        {
            break;
        }

        //  try to request the write

        CWriteBack writebackCopy( *pwriteback );
        err = pwriteback->ErrWriteBack( this );

        //  if we failed due to too many writes on the first write then try again while overriding the throttling.  it
        //  is very important that we maintain forward progress

        if ( FTooManyWrites( err ) && !cWriteBacksStarted )
        {
            err = pwriteback->ErrWriteBack( this, fTrue );
        }

        //  if the write request succeeded then note that

        if ( err >= JET_errSuccess )
        {
            cWriteBacksStarted++;
        }

        //  if the write request failed then we must handle that

        else
        {
            //  if the write request failed due to too many writes then put it back in the queue

            if ( FTooManyWrites( err ) )
            {
                m_critPendingWriteBacks.Enter();
                m_ilRequestedWriteBacks.Remove( pwriteback );
                m_ilQueuedWriteBacks.InsertAsPrevMost( pwriteback );
                m_critPendingWriteBacks.Leave();
            }

            //  if the write request failed for any other reason then remove it from the requested list and complete
            //  its callbacks

            else
            {
                m_critPendingWriteBacks.Enter();

                //  we cannot use "m_ilRequestedWriteBacks.FMember( pwriteback )" here because it could touch freed memory
                CWriteBack* pwritebackT = NULL;
                for (   pwritebackT = m_ilRequestedWriteBacks.PrevMost();
                        pwritebackT != NULL && pwritebackT != pwriteback;
                        pwritebackT = m_ilRequestedWriteBacks.Next( pwritebackT ) )
                {
                }

                if ( pwritebackT == pwriteback )
                {
                    Assert( err == JET_errOutOfMemory );  //  OOM on alloc of CIOComplete in ErrWriteCommon
                    m_ilRequestedWriteBacks.Remove( pwriteback );
                }

                m_critPendingWriteBacks.Leave();

                if ( pwritebackT == pwriteback )
                {
                    delete pwriteback;
                }

                writebackCopy.Complete( err, this );

                cWriteBacksFailed++;

                //  continue processing other write backs

                err = JET_errSuccess;
            }
        }
    }

    //  if any write backs were successfully requested then issue IO

    if ( cWriteBacksStarted )
    {
        CallS( TFileWrapper<I>::ErrIOIssue() );
    }

    //  allow another attempt to request write backs

    m_semRequestWriteBacks.Release();

    //  if any write backs failed and completed while we were processing write backs then immediately retry so we can
    //  release any engine IOs waiting on those failed write backs

    if ( cWriteBacksFailed )
    {
        IssueWriteBacks();
    }
}

template<class I>
void TFileFilter<I>::ReleaseWaitingIOs()
{
    const CMeteredSection::Group groupActive = m_msPendingWriteBacks.GroupActive();
    m_critWaiters.Enter();
    CWaiter* pwaiterNext = NULL;
    for ( CWaiter* pwaiter = m_ilWaiters.PrevMost(); pwaiter != NULL; pwaiter = pwaiterNext )
    {
        pwaiterNext = m_ilWaiters.Next( pwaiter );

        if ( FAccessPermitted( pwaiter->Offsets(), groupActive ) )
        {
            m_ilWaiters.Remove( pwaiter );

            const CMeteredSection::Group group = m_msPendingWriteBacks.Enter();
            Assert( group == groupActive );

            pwaiter->Complete( group );
        }
    }
    m_critWaiters.Leave();
}

template<class I>
void TFileFilter<I>::CompleteWriteBack( _In_opt_ CWriteBack* const pwriteback )
{
    //  remove the write back from the requested write back list if necessary

    if ( pwriteback )
    {
        m_critPendingWriteBacks.Enter();
        m_ilRequestedWriteBacks.Remove( pwriteback );
        m_critPendingWriteBacks.Leave();

        delete pwriteback;
    }

    //  try to issue more write backs and, crucially, release any waiters blocked by this write back

    IssueWriteBacks();
}

template<class I>
void TFileFilter<I>::GetWriteBackOffsets(   _In_ CArray<COffsets>&                                      arrayPendingWriteBacks,
                                            _In_ CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>&    ilWriteBacks,
                                            _In_ size_t&                                                iwriteback )
{
    for (   CWriteBack* pwriteback = ilWriteBacks.PrevMost();
            pwriteback != NULL;
            pwriteback = ilWriteBacks.Next( pwriteback ) )
    {
        arrayPendingWriteBacks[ iwriteback++ ] = pwriteback->Offsets();
    }
}

template< class I >
BOOL TFileFilter<I>::FTooManyWrites( _In_ const ERR err )
{
    return err == errDiskTilt;
}

template< class I >
ICache::CachingPolicy TFileFilter<I>::CpGetCachingPolicy( _In_ const TraceContext& tc, _In_ const BOOL fWrite )
{
    //  trivial implementation:  always request best effort caching

    return cpBestEffort;
}

template< class I >
ERR TFileFilter<I>::ErrCacheRead(   _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_                 const void** const              ppioreq,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err                     = JET_errSuccess;
    OSFILEQOS       grbitQOSInput           = grbitQOS & qosIOInMask;
    OSFILEQOS       grbitQOSOutput          = grbitQOS & ( qosIOInMask | qosBypassIOQuota );
    const void*     pioreq                  = *ppioreq;
    BOOL            fThrottleReleaser       = fFalse;
    BOOL            fCombined               = fFalse;
    BOOL            fCleanUpStateSaved      = fFalse;
    BOOL            fRestoreCleanupState    = fFalse;
    CIOComplete*    piocomplete             = NULL;

    *ppioreq = NULL;

    //  check to see if this IO may proceed or if it is throttled

    if ( !pioreq )
    {
        Call( ErrThrottleCheck( pfnIOComplete != NULL,
                                fTrue, 
                                COffsets( ibOffset, ibOffset + cbData - 1 ), 
                                grbitQOS, 
                                &fThrottleReleaser,
                                &fCombined ) );

        //  set the IO combined state for output

        if ( fCombined )
        {
            grbitQOSOutput |= qosIOCompleteIoCombined;
        }
    }

    //  strip any grbitQOS bits that can result in the rejection of the read request that we have handled.  if we
    //  missed any then that will be caught by the below assert that we never see errDiskTilt
    //
    //  NOTE:  we allow qosIODispatchBackground because this indicates IO smoothing and won't result in rejection

    grbitQOSInput &= ~qosIOOptimizeCombinable;

    //  determine the caching policy for this read

    const ICache::CachingPolicy cp = CpGetCachingPolicy( tc, fFalse );

    //  if the file isn't attached to the cache then read as if it is a cache miss.  otherwise, use the cache

    if ( FAttached() )
    {
        //  cache accesses will allocate memory for sync reads

        fCleanUpStateSaved = FOSSetCleanupState( fFalse );
        fRestoreCleanupState = fTrue;

        //  release any reserved IOREQ because we will not be using it

        if ( pioreq )
        {
            CReservedIOREQ* const preservedIOREQ = (CReservedIOREQ*)pioreq;
            preservedIOREQ->Capture( &fThrottleReleaser, &grbitQOSOutput );
            ReleaseUnusedIOREQ( &pioreq);
        }

        //  try to service the read via the cache

        if ( pfnIOComplete || pfnIOHandoff )
        {
            Alloc( piocomplete = new CIOComplete(   fTrue,
                                                    this,
                                                    iomEngine,
                                                    fFalse, 
                                                    grbitQOSOutput,
                                                    pgroup, 
                                                    ppsem,
                                                    NULL, 
                                                    &fThrottleReleaser,
                                                    ibOffset, 
                                                    cbData, 
                                                    pbData, 
                                                    pfnIOComplete, 
                                                    pfnIOHandoff, 
                                                    keyIOComplete ) );
        }

        //  ask the cache for this data
        //
        //  NOTE:  the handoff reference to the io completion context occurs in the Release call on exit

        err =  m_pc->ErrRead(   tc,
                                m_volumeid,
                                m_fileid, 
                                m_fileserial,
                                ibOffset, 
                                cbData, 
                                pbData, 
                                grbitQOSInput,
                                cp, 
                                pfnIOComplete ? CIOComplete::Complete_ : NULL,
                                DWORD_PTR( piocomplete ) );

        //  reject on request due to grbitQOS should not happen here

        Assert( err != errDiskTilt );
        Call( err );
    }
    else
    {
        //  there is no cache so handle as if it were a cache miss

        const ICacheTelemetry::FileNumber filenumber = Filenumber();
        const BOOL fCacheIfPossible = cp != cpDontCache;
        const ICacheTelemetry::BlockNumber blocknumberMax = Blocknumber( ibOffset + cbData + CbBlockSize() - 1 );
        for (   ICacheTelemetry::BlockNumber blocknumber = Blocknumber( ibOffset );
                blocknumber < blocknumberMax;
                blocknumber++ )
        {
            m_pctm->Miss( filenumber, blocknumber, fTrue, fCacheIfPossible );
        }

        err = ErrCacheMiss( tc,
                            ibOffset, 
                            cbData, 
                            pbData,
                            grbitQOS,
                            grbitQOSInput, 
                            grbitQOSOutput, 
                            pfnIOComplete, 
                            keyIOComplete, 
                            pfnIOHandoff, 
                            &pioreq, 
                            pgroup, 
                            ppsem, 
                            &fThrottleReleaser );

        //  reject on request due to grbitQOS should not happen here

        Assert( err != errDiskTilt );
        Call( err );
    }

HandleError:
    err = HandleReservedIOREQ(  tc,
                                ibOffset, 
                                cbData, 
                                pbData,
                                grbitQOS,
                                pfnIOComplete,
                                keyIOComplete, 
                                pfnIOHandoff,
                                pioreq,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    (void)FThrottleRelease( pfnIOComplete != NULL, fTrue, grbitQOS, &fThrottleReleaser );
    if ( fRestoreCleanupState )
    {
        FOSSetCleanupState( fCleanUpStateSaved );
    }
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrCacheMiss(   _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const OSFILEQOS                 grbitQOSInput,
                                    _In_                    const OSFILEQOS                 grbitQOSOutput,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_                 const void** const              ppioreq,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem,
                                    _Inout_opt_             BOOL* const                     pfThrottleReleaser )
{
    ERR             err                     = JET_errSuccess;
    OSFILEQOS       grbitQOSOutputActual    = grbitQOSOutput;
    const void*     pioreq                  = *ppioreq;
    BOOL            fThrottleReleaser       = pfThrottleReleaser ? *pfThrottleReleaser : NULL;
    CIOComplete*    piocomplete             = NULL;

    *ppioreq = NULL;

    if ( pfThrottleReleaser )
    {
        *pfThrottleReleaser = fFalse;
    }

    //  release any reserved IOREQ because we will not be using it

    if ( pioreq )
    {
        Assert( !fThrottleReleaser );

        CReservedIOREQ* const preservedIOREQ = (CReservedIOREQ*)pioreq;
        preservedIOREQ->Capture( &fThrottleReleaser, &grbitQOSOutputActual );
        ReleaseUnusedIOREQ( &pioreq);
    }

    //  try to service the read via the file system

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
                            this,
                            iomCacheMiss,
                            fFalse, 
                            grbitQOSOutputActual,
                            pgroup, 
                            ppsem,
                            NULL, 
                            &fThrottleReleaser,
                            ibOffset, 
                            cbData, 
                            pbData, 
                            pfnIOComplete, 
                            pfnIOHandoff, 
                            keyIOComplete ) );
    }

    Call( ErrRead(  tc,
                    ibOffset,
                    cbData,
                    pbData,
                    grbitQOSInput, 
                    iomRaw,
                    pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                    DWORD_PTR( piocomplete ),
                    piocomplete ? CIOComplete::IOHandoff_ : NULL,
                    NULL ) );

    if ( piocomplete && piocomplete->FAccessingHeader() )
    {
        Call( ErrIssue( iomRaw ) );
    }

HandleError:
    err = HandleReservedIOREQ(  tc, 
                                ibOffset, 
                                cbData, 
                                pbData,
                                grbitQOS,
                                pfnIOComplete,
                                keyIOComplete, 
                                pfnIOHandoff,
                                pioreq,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    (void)FThrottleRelease( pfnIOComplete != NULL, fTrue, grbitQOS, &fThrottleReleaser );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrCacheWrite(  _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err                 = JET_errSuccess;
    OSFILEQOS       grbitQOSInput       = grbitQOS & qosIOInMask;
    OSFILEQOS       grbitQOSOutput      = grbitQOS & ( qosIOInMask | qosBypassIOQuota );
    BOOL            fThrottleReleaser   = fFalse;
    BOOL            fCombined           = fFalse;
    CIOComplete*    piocomplete         = NULL;

    //  check to see if this IO may proceed or if it is throttled

    Call( ErrThrottleCheck( pfnIOComplete != NULL,
                            fFalse,
                            COffsets( ibOffset, ibOffset + cbData - 1 ),
                            grbitQOS,
                            &fThrottleReleaser,
                            &fCombined ) );

    //  set the IO combined state for output

    if ( fCombined )
    {
        grbitQOSOutput |= qosIOCompleteIoCombined;
    }

    //  strip any grbitQOS bits that can result in the rejection of the write request that we have handled.  if we
    //  missed any then that will be caught by the below assert that we never see errDiskTilt

    grbitQOSInput &= ~qosIOOptimizeCombinable;
    grbitQOSInput &= ~qosIODispatchBackground;
    grbitQOSInput &= ~qosIODispatchUrgentBackgroundMask;

    //  if we don't have a dispatch mode then specify dispatch immediate

    if ( !( grbitQOSInput & qosIODispatchMask ) )
    {
        grbitQOSInput |= qosIODispatchImmediate;
    }

    //  determine the caching policy for this write

    const ICache::CachingPolicy cp = CpGetCachingPolicy( tc, fTrue );

    //  if the file isn't attached to the cache then write as if it is a write through.  otherwise, use the cache

    if ( FAttached() )
    {
        //  try to service the write via the cache
        
        if ( pfnIOComplete || pfnIOHandoff )
        {
            Alloc( piocomplete = new CIOComplete(   fTrue,
                                                    this, 
                                                    iomEngine,
                                                    fTrue,
                                                    grbitQOSOutput,
                                                    pgroup, 
                                                    ppsem, 
                                                    NULL, 
                                                    &fThrottleReleaser,
                                                    ibOffset,
                                                    cbData, 
                                                    pbData,
                                                    pfnIOComplete,
                                                    pfnIOHandoff,
                                                    keyIOComplete ) );
        }

        //  send this data to the cache
        //
        //  NOTE:  the handoff reference to the io completion context occurs in the Release call on exit

        err = m_pc->ErrWrite(   tc,
                                m_volumeid, 
                                m_fileid, 
                                m_fileserial,
                                ibOffset, 
                                cbData, 
                                pbData, 
                                grbitQOSInput,
                                cp,
                                pfnIOComplete ? CIOComplete::Complete_ : NULL,
                                DWORD_PTR( piocomplete ) );

        //  reject on request due to grbitQOS should not happen here

        Assert( err != errDiskTilt );
        Call( err );

        //  if the file is configured for write back then remember to flush the cache later

        if ( ( Fmf() & fmfStorageWriteBack ) != 0 )
        {
            AtomicIncrement( (LONG*)&m_cCacheWriteForFlush );
        }
    }
    else
    {
        //  there is no cache so handle as if it were a write through

        const ICacheTelemetry::FileNumber filenumber = Filenumber();
        const BOOL fCacheIfPossible = cp != cpDontCache;
        const ICacheTelemetry::BlockNumber blocknumberMax = Blocknumber( ibOffset + cbData + CbBlockSize() - 1 );
        for (   ICacheTelemetry::BlockNumber blocknumber = Blocknumber( ibOffset );
                blocknumber < blocknumberMax;
                blocknumber++ )
        {
            m_pctm->Miss( filenumber, blocknumber, fFalse, fCacheIfPossible );
            m_pctm->Update( filenumber, blocknumber );
            m_pctm->Write( filenumber, blocknumber, true );
        }

        err = ErrWriteThrough(  iomEngine,
                                tc,
                                ibOffset,
                                cbData,
                                pbData,
                                grbitQOS,
                                grbitQOSInput,
                                grbitQOSOutput,
                                pfnIOComplete,
                                keyIOComplete, 
                                pfnIOHandoff, 
                                pgroup, 
                                ppsem, 
                                &fThrottleReleaser );

        //  reject on request due to grbitQOS should not happen here

        Assert( err != errDiskTilt );
        Call( err );
    }

HandleError:
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    (void)FThrottleRelease( pfnIOComplete != NULL, fFalse, grbitQOS, &fThrottleReleaser );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrWriteThrough(    _In_                    const IFileFilter::IOMode       iom,
                                        _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const OSFILEQOS                 grbitQOSInput,
                                        _In_                    const OSFILEQOS                 grbitQOSOutput,
                                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                        _Inout_                 CMeteredSection::Group* const   pgroup,
                                        _Inout_                 CSemaphore** const              ppsem,
                                        _Inout_opt_             BOOL* const                     pfThrottleReleaser )
{
    return ErrWriteCommon( iom, tc, ibOffset, cbData, pbData, grbitQOS, grbitQOSInput, grbitQOSOutput, pfnIOComplete, keyIOComplete, pfnIOHandoff, pgroup, ppsem, NULL, pfThrottleReleaser );
}

template< class I >
ERR TFileFilter<I>::ErrWriteBack(   _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_                    CWriteBack* const               pwriteback )
{
    return ErrWriteCommon( iomCacheWriteBack, tc, ibOffset, cbData, pbData, grbitQOS, grbitQOS, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, NULL, NULL, pwriteback, NULL );
}

template< class I >
ERR TFileFilter<I>::ErrWriteCommon( _In_                    const IFileFilter::IOMode       iom,
                                    _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const OSFILEQOS                 grbitQOSInput,
                                    _In_                    const OSFILEQOS                 grbitQOSOutput,
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_opt_             CMeteredSection::Group* const   pgroup,
                                    _Inout_opt_             CSemaphore** const              ppsem,
                                    _In_opt_                CWriteBack* const               pwriteback,
                                    _Inout_opt_             BOOL* const                     pfThrottleReleaser )
{
    ERR             err                 = JET_errSuccess;
    BOOL            fThrottleReleaser   = pfThrottleReleaser ? *pfThrottleReleaser : fFalse;
    CIOComplete*    piocomplete         = NULL;

    if ( pfThrottleReleaser )
    {
        *pfThrottleReleaser = fFalse;
    }

    //  try to service the write via the file system

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
                            this, 
                            iom,
                            fTrue,
                            grbitQOSOutput,
                            pgroup, 
                            ppsem, 
                            pwriteback, 
                            &fThrottleReleaser,
                            ibOffset,
                            cbData, 
                            pbData,
                            pfnIOComplete,
                            pfnIOHandoff,
                            keyIOComplete ) );
    }

    Call( ErrWrite( tc,
                    ibOffset,
                    cbData,
                    pbData,
                    grbitQOSInput, 
                    iomRaw,
                    pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                    DWORD_PTR( piocomplete ),
                    piocomplete ? CIOComplete::IOHandoff_ : NULL ) );

    //  note any unflushed sync engine write

    if ( !pfnIOComplete && iom == iomEngine )
    {
        AtomicAdd( (QWORD*)&m_cioUnflushed, 1 );
    }

HandleError:
    if ( piocomplete )
    {
        if ( FTooManyWrites( err ) )
        {
            piocomplete->DoNotReleaseWriteBack();
        }
        piocomplete->Release( err, tc, grbitQOS );
    }
    (void)FThrottleRelease( pfnIOComplete != NULL, fFalse, grbitQOS, &fThrottleReleaser );
    return err;
}

template< class I >
typename TFileFilter<I>::CThreadLocalStorageRepository::CStaticThreadLocalStorage TFileFilter<I>::CThreadLocalStorageRepository::s_tls;

template< class I >
typename CReaderWriterLock TFileFilter<I>::CThrottleContextRepository::s_rwlThrottleContexts( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::CThrottleContextRepository::s_rwlThrottleContexts" ), rankThrottleContexts, 0 ) );

template< class I >
typename TFileFilter<I>::CThrottleContextRepository::CThrottleContextList TFileFilter<I>::CThrottleContextRepository::s_ilThrottleContexts;


//  CFileFilter:  concrete TFileFilter<IFileFilter>

class CFileFilter : public TFileFilter<IFileFilter>
{
    public:  //  specialized API

        CFileFilter(    _Inout_     IFileAPI** const                    ppfapi,
                        _In_        IFileSystemFilter* const            pfsf,
                        _In_        IFileSystemConfiguration* const     pfsconfig,
                        _In_        IFileIdentification* const          pfident,
                        _In_        ICacheTelemetry* const              pctm,
                        _In_        ICacheRepository* const             pcrep,
                        _In_        const VolumeId                      volumeid,
                        _In_        const FileId                        fileid,
                        _Inout_opt_ ICachedFileConfiguration** const    ppcfconfig,
                        _In_        const BOOL                          fEverEligibleForCaching,
                        _Inout_opt_ ICache** const                      ppc,
                        _Inout_opt_ CCachedFileHeader** const           ppcfh )
            :   TFileFilter<IFileFilter>( ppfapi, pfsf, pfsconfig, pfident, pctm, pcrep, volumeid, fileid, ppcfconfig, fEverEligibleForCaching, ppc, ppcfh )
        {
        }

        virtual ~CFileFilter() {}

        static void Cleanup()
        {
            TFileFilter<IFileFilter>::Cleanup();
        }
};
