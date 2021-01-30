// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


template< class I >
class TFileFilter
    :   public TFileWrapper<I>
{
    public:

        TFileFilter(    _Inout_     IFileAPI** const                    ppfapi,
                        _In_        IFileSystemFilter* const            pfsf,
                        _In_        IFileSystemConfiguration* const     pfsconfig,
                        _In_        ICacheTelemetry* const              pctm,
                        _In_        const VolumeId                      volumeid,
                        _In_        const FileId                        fileid,
                        _Inout_opt_ ICachedFileConfiguration** const    ppcfconfig,
                        _Inout_opt_ ICache** const                      ppc,
                        _Inout_opt_ CCachedFileHeader** const           ppcfh );

        virtual ~TFileFilter();

        void SetFileId( _In_ const VolumeId volumeid, _In_ const FileId fileid )
        {
            m_volumeid = volumeid;
            m_fileid = fileid;
        }

        void SetCacheState( _Inout_     ICachedFileConfiguration** const    ppcfconfig,
                            _Inout_opt_ ICache** const                      ppc,
                            _Inout_opt_ CCachedFileHeader** const           ppcfh )
        {
            m_pcfconfig = *ppcfconfig;
            m_pc = ppc ? *ppc : NULL;
            m_pcfh = ppcfh ? *ppcfh : NULL;

            *ppcfconfig = NULL;
            if ( ppc )
            {
                *ppc = NULL;
            }
            if ( ppcfh )
            {
                *ppcfh = NULL;
            }

            SetCacheParameters();

            if ( m_pcfh )
            {
                m_fileserial = m_pcfh->Fileserial();
            }

            OSTrace( JET_tracetagBlockCache, OSFormat( "%s ctor", OSFormat( this ) ) );
        }

    public:

        ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                _Out_ FileId* const     pfileid,
                                _Out_ FileSerial* const pfileserial ) override;

    public:

        ERR ErrFlushFileBuffers( _In_ const IOFLUSHREASON iofr ) override;

        ERR ErrSetSize( _In_ const TraceContext&    tc,
                        _In_ const QWORD            cbSize,
                        _In_ const BOOL             fZeroFill,
                        _In_ const OSFILEQOS        grbitQOS ) override;

        ERR ErrRename(  _In_z_  const WCHAR* const wszPathDest,
                        _In_    const BOOL         fOverwriteExisting = fFalse ) override;

        ERR ErrIOTrim(  _In_ const TraceContext&    tc,
                        _In_ const QWORD            ibOffset,
                        _In_ const QWORD            cbToFree ) override;

        ERR ErrIORead(  _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_                    const VOID *                    pioreq ) override;
        ERR ErrIOWrite( _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIOIssue() override;

    public:

        ERR ErrRead(    _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_                    const VOID *                    pioreq ) override;
        ERR ErrWrite(   _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIssue( _In_ const IFileFilter::IOMode iom ) override;

    private:


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

                typename CInvasiveList<CWaiter, OffsetOfILE>::CElement  m_ile;
                COffsets                                                m_offsets;
                CManualResetSignal                                      m_sig;
                CMeteredSection::Group                                  m_group;
        };


        class CWriteBack
        {
            public:

                CWriteBack( _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_                    const DWORD_PTR                 keyIOComplete,
                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
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
                    TLSSetUserTraceContext( &m_tc.utc );
                    ERR err = pff->ErrWriteBack(    m_tc.etc,
                                                    m_ibOffset,
                                                    m_cbData,
                                                    m_pbData,
                                                    fOverrideThrottling ? qosIONormal : m_grbitQOS,
                                                    m_pfnIOComplete,
                                                    m_keyIOComplete,
                                                    m_pfnIOHandoff,
                                                    this );
                    TLSSetUserTraceContext( NULL );
                    return err;
                }

                void Complete( _In_ const ERR err, _In_ TFileFilter<I>* const pff )
                {
                    if ( m_pfnIOHandoff )
                    {

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

                typename CCountedInvasiveList<CWriteBack, OffsetOfILE>::CElement    m_ile;
                FullTraceContext                                                    m_tc;
                const QWORD                                                         m_ibOffset;
                const DWORD                                                         m_cbData;
                const BYTE* const                                                   m_pbData;
                const OSFILEQOS                                                     m_grbitQOS;
                const IFileAPI::PfnIOComplete                                       m_pfnIOComplete;
                const DWORD_PTR                                                     m_keyIOComplete;
                const IFileAPI::PfnIOHandoff                                        m_pfnIOHandoff;
        };


        class CWriteBackComplete
        {
            public:

                CWriteBackComplete( _In_ const DWORD_PTR                keyIOComplete,
                                    _In_ const IFileAPI::PfnIOHandoff   pfnIOHandoff )
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
                                            _In_                    OSFILEQOS                   grbitQOS,
                                            _In_                    const QWORD                 ibOffset,
                                            _In_                    const DWORD                 cbData,
                                            _In_reads_( cbData )    BYTE* const                 pbData,
                                            _In_                    CWriteBackComplete* const   pwriteBackComplete )
                {
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
                                        _In_                    CWriteBackComplete* const   pwriteBackComplete,
                                        _In_                    void* const                 pvIOContext )
                {
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
        VOID EndAccess( _In_ const CMeteredSection::Group group, _In_ CSemaphore* const psem );

        ERR ErrInvalidate( _In_ const COffsets& offsets );

        static ERR ErrAttach_(  _In_ TFileFilter<I>* const  pff,
                                _In_ const COffsets&        offsetsFirstWrite )
        {
            return pff->ErrAttach( offsetsFirstWrite );
        }
        ERR ErrAttach( _In_ const COffsets& offsetsFirstWrite );

        ERR ErrTryEnqueueWriteBack( _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff );
        size_t CPendingWriteBacks();
        size_t CPendingWriteBackMax();

        void IssueWriteBacks();

        static void RequestWriteBacks_( _In_ TFileFilter<I>* const pff ) { pff->RequestWriteBacks(); }
        void RequestWriteBacks();

        void ReleaseWaitingIOs();

        void CompleteWriteBack( _In_ CWriteBack* const pwriteback );

        void GetWriteBackOffsets(   _In_ CArray<COffsets>&                                      arrayPendingWriteBacks,
                                    _In_ CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>&    ilWriteBacks,
                                    _In_ size_t&                                                iwriteback );
        BOOL FTooManyWrites( _In_ const ERR err );

        BOOL FAttached() const { return m_pc && m_pcfh; }

        ICache::CachingPolicy CpGetCachingPolicy( _In_ const TraceContext& tc, _In_ const BOOL fWrite );

        ERR ErrCacheRead(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _Out_writes_( cbData )  BYTE* const                     pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_                    const DWORD_PTR                 keyIOComplete,
                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_                    const VOID *                    pioreq,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );
        ERR ErrCacheMiss(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _Out_writes_( cbData )  BYTE* const                     pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_                    const DWORD_PTR                 keyIOComplete,
                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_                    const VOID *                    pioreq,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );

        ERR ErrCacheWrite(  _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_                    const DWORD_PTR                 keyIOComplete,
                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );
        ERR ErrWriteThrough(    _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _Inout_                 CMeteredSection::Group* const   pgroup,
                                _Inout_                 CSemaphore** const              ppsem );
        ERR ErrWriteBack(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_                    const DWORD_PTR                 keyIOComplete,
                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_                    CWriteBack* const               pwriteback );
        ERR ErrWriteCommon( _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_                    const DWORD_PTR                 keyIOComplete,
                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_opt_             CMeteredSection::Group* const   pgroup,
                            _Inout_opt_             CSemaphore** const              ppsem,
                            _In_opt_                CWriteBack* const               pwriteback );

        void SetCacheParameters()
        {
            if ( m_pcfconfig )
            {
                m_filenumber = (ICacheTelemetry::FileNumber)m_pcfconfig->LCacheTelemetryFileNumber();
                m_cbBlockSize = m_pcfconfig->CbBlockSize();
            }
        }

        ICacheTelemetry::FileNumber Filenumber() const { return m_filenumber; }
        DWORD CbBlockSize() const { return m_cbBlockSize; }
        ICacheTelemetry::BlockNumber Blocknumber( const QWORD ibOffset ) const
        {
            return (ICacheTelemetry::BlockNumber)( ibOffset / CbBlockSize() );
        }

    private:

        typedef CInitOnce< ERR, decltype( &ErrAttach_ ), TFileFilter<I>* const, const COffsets& > CInitOnceAttach;

        const COffsets                                              s_offsetsCachedFileHeader           = COffsets( 0, sizeof( CCachedFileHeader ) - 1 );

        IFileSystemFilter * const                                   m_pfsf;
        IFileSystemConfiguration* const                             m_pfsconfig;
        ICacheTelemetry* const                                      m_pctm;
        VolumeId                                                    m_volumeid;
        FileId                                                      m_fileid;
        FileSerial                                                  m_fileserial;
        ICachedFileConfiguration*                                   m_pcfconfig;
        ICache*                                                     m_pc;
        CCachedFileHeader*                                          m_pcfh;
        ICacheTelemetry::FileNumber                                 m_filenumber;
        ULONG                                                       m_cbBlockSize;
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
        LONG                                                        m_cCacheWriteForFlush;
        LONG                                                        m_cCacheMissForIssue;
        LONG                                                        m_cCacheWriteThroughForIssue;
        LONG                                                        m_cCacheWriteBackForIssue;
        CInitOnceAttach                                             m_initOnceAttach;
        CSemaphore                                                  m_semCachedFileHeader;

    protected:


        class CIOComplete
            :   public TFileWrapper<I>::CIOComplete
        {
            public:

                CIOComplete(    _In_                    const BOOL                      fIsHeapAlloc,
                                _In_                    TFileFilter<I>* const           pff,
                                _In_                    const BOOL                      fWrite,
                                _Inout_opt_             CMeteredSection::Group* const   pgroupPendingWriteBacks,
                                _Inout_opt_             CSemaphore** const              ppsemCachedFileHeader,
                                _In_opt_                CWriteBack* const               pwriteback,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_                    const DWORD_PTR                 keyIOComplete )
                    :   TFileWrapper<I>::CIOComplete(   fIsHeapAlloc,
                                                        pff, 
                                                        ibOffset, 
                                                        cbData, 
                                                        pbData,
                                                        pfnIOComplete,
                                                        pfnIOHandoff,
                                                        keyIOComplete ),
                        m_pff( pff ),
                        m_fWrite( fWrite ),
                        m_groupPendingWriteBacks( pgroupPendingWriteBacks ? *pgroupPendingWriteBacks : CMeteredSection::groupInvalidNil ),
                        m_psemCachedFileHeader( ppsemCachedFileHeader ? *ppsemCachedFileHeader : NULL ),
                        m_pwriteback( pwriteback ),
                        m_fReleaseWriteback( m_pwriteback != NULL ),
                        m_fReleaseResources( fTrue )
                {
                    if ( pgroupPendingWriteBacks )
                    {
                        *pgroupPendingWriteBacks = CMeteredSection::groupInvalidNil;
                    }
                    if ( ppsemCachedFileHeader )
                    {
                        *ppsemCachedFileHeader = NULL;
                    }
                }

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
                        m_pff->EndAccess( m_groupPendingWriteBacks, m_psemCachedFileHeader );
                        m_pff->CompleteWriteBack( m_fReleaseWriteback ? m_pwriteback : NULL );
                    }
                }

            private:

                TFileFilter<I>* const           m_pff;
                const BOOL                      m_fWrite;
                const CMeteredSection::Group    m_groupPendingWriteBacks;
                CSemaphore* const               m_psemCachedFileHeader;
                CWriteBack* const               m_pwriteback;
                BOOL                            m_fReleaseWriteback;
                volatile BOOL                   m_fReleaseResources;
        };
};

INLINE const char* OSFormat( _In_ IFileFilter* const pff )
{
    WCHAR       wszAbsPath[ OSFSAPI_MAX_PATH ]  = { 0 };
    VolumeId    volumeid                        = volumeidInvalid;
    FileId      fileid                          = fileidInvalid;
    FileSerial  fileserial                      = fileserialInvalid;

    CallS( pff->ErrPath( wszAbsPath ) );
    CallS( pff->ErrGetPhysicalId( &volumeid, &fileid, &fileserial ) );

    return OSFormat( "%S (0x%08x:0x%016I64x)", wszAbsPath, volumeid, fileid );
}

template< class I >
TFileFilter<I>::TFileFilter(    _Inout_     IFileAPI** const                    ppfapi,
                                _In_        IFileSystemFilter* const            pfsf,
                                _In_        IFileSystemConfiguration* const     pfsconfig,
                                _In_        ICacheTelemetry* const              pctm,
                                _In_        const VolumeId                      volumeid,
                                _In_        const FileId                        fileid,
                                _Inout_opt_ ICachedFileConfiguration** const    ppcfconfig,
                                _Inout_opt_ ICache** const                      ppc,
                                _Inout_opt_ CCachedFileHeader** const           ppcfh )
    :   TFileWrapper<I>( (I** const)ppfapi ),
        m_pfsf( pfsf ),
        m_pfsconfig( pfsconfig ),
        m_pctm( pctm ),
        m_volumeid( volumeid ),
        m_fileid( fileid ),
        m_fileserial( fileserialInvalid ),
        m_pcfconfig( ppcfconfig ? *ppcfconfig : NULL ),
        m_pc( ppc ? *ppc : NULL ),
        m_pcfh( ppcfh ? *ppcfh : NULL ),
        m_filenumber( filenumberInvalid ),
        m_cbBlockSize( 0 ),
        m_critWaiters( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_critWaiters" ), rankFileFilter, 0 ) ),
        m_critPendingWriteBacks( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_critPendingWriteBacks" ), rankFileFilter, 0 ) ),
        m_cPendingWriteBacksMax( 0 ),
        m_semRequestWriteBacks( CSyncBasicInfo( "TFileFilter<I>::m_semRequestWriteBacks" ) ),
        m_cCacheWriteForFlush( 0 ),
        m_cCacheMissForIssue( 0 ),
        m_cCacheWriteThroughForIssue( 0 ),
        m_cCacheWriteBackForIssue( 0 ),
        m_semCachedFileHeader( CSyncBasicInfo( "TFileFilter<I>::m_semCachedFileHeader" ) )
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
    if ( ppc )
    {
        *ppc = NULL;
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

    m_semCachedFileHeader.Acquire();
    m_semRequestWriteBacks.Acquire();
    delete m_pcfh;
    delete m_pc;
    delete m_pcfconfig;
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

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrFlushFileBuffers", OSFormat( this ) ) );


    if ( AtomicExchange( &m_cCacheWriteForFlush, 0 ) > 0 )
    {
        Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );
    }


    Call( TFileWrapper<I>::ErrFlushFileBuffers( iofr ) );

HandleError:
    return err;
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


    Call( TFileWrapper<I>::ErrSize( &cbSizeCurrent, IFileAPI::filesizeLogical ) );

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrSetSize %llu -> %llu", OSFormat( this ), cbSizeCurrent, cbSizeNew ) );


    Call( ErrBeginAccess( COffsets( min( cbSizeCurrent, cbSizeNew ), qwMax ), fTrue, &group, &psem ) );


    Call( ErrInvalidate( COffsets( cbSizeNew, qwMax ) ) );


    if ( FAttached() )
    {
        cbSizeNew = max( cbSizeNew, sizeof( CCachedFileHeader ) );
    }

    Call( TFileWrapper<I>::ErrSetSize( tc, cbSizeNew, fZeroFill, grbitQOS ) );

HandleError:
    EndAccess( group, psem );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrRename(  _In_z_  const WCHAR* const  wszPathDest,
                                _In_    const BOOL          fOverwriteExisting )
{
    OSTrace(    JET_tracetagBlockCache,
                OSFormat(   "%s ErrRename %S fOverwriteExisting=%s", 
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


    if ( cbToFree > 0 )
    {

        Call( ErrBeginAccess( offsets, fTrue, &group, &psem ) );


        Call( ErrInvalidate( offsets ) );


        QWORD cbHeaderOverlapped = FAttached() && offsets.FOverlaps( s_offsetsCachedFileHeader ) ?
            min(offsets.Cb(), s_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 ) :
            0;

        if ( cbToFree > cbHeaderOverlapped )
        {
            Call( TFileWrapper<I>::ErrIOTrim( tc, ibOffset + cbHeaderOverlapped, cbToFree - cbHeaderOverlapped ) );
        }
    }

HandleError:
    EndAccess( group, psem );
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrIORead(  _In_                    const TraceContext&                 tc,
                                _In_                    const QWORD                         ibOffset,
                                _In_                    const DWORD                         cbData,
                                _Out_writes_( cbData )  __out_bcount( cbData ) BYTE* const  pbData,
                                _In_                    const OSFILEQOS                     grbitQOS,
                                _In_                    const IFileAPI::PfnIOComplete       pfnIOComplete,
                                _In_                    const DWORD_PTR                     keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                                _In_                    const VOID *                        pioreq )
{
    return ErrRead( tc, ibOffset, cbData, pbData, grbitQOS, IOMode::iomEngine, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq );
}

template< class I >
ERR TFileFilter<I>::ErrIOWrite( _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    return ErrWrite( tc, ibOffset, cbData, pbData, grbitQOS, IOMode::iomEngine, pfnIOComplete, keyIOComplete, pfnIOHandoff );
}

template< class I >
ERR TFileFilter<I>::ErrIOIssue()
{
    return ErrIssue( IOMode::iomEngine );
}

template< class I >
ERR TFileFilter<I>::ErrRead(    _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _Out_writes_( cbData )  BYTE* const                     pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_                    const VOID *                    pioreq )
{
    ERR                     err         = JET_errSuccess;
    BOOL                    fIOREQUsed  = fFalse;
    COffsets                offsets     = COffsets( ibOffset, ibOffset + cbData - 1 );
    CMeteredSection::Group  group       = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem        = NULL;

    Assert( cbData > 0 );
    Assert( iom == IOMode::iomRaw || iom == IOMode::iomEngine || iom == IOMode::iomCacheMiss );


    if ( iom == IOMode::iomEngine )
    {
        Call( ErrBeginAccess( offsets, fFalse, &group, &psem ) );
        fIOREQUsed = fTrue;
        Call( ErrCacheRead( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq, &group, &psem ) );
    }
    else if ( iom == IOMode::iomCacheMiss )
    {
        Call( ErrVerifyAccess( offsets ) );
        fIOREQUsed = fTrue;
        Call( ErrCacheMiss( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq, &group, &psem ) );
        AtomicIncrement( &m_cCacheMissForIssue );
    }
    else
    {
        fIOREQUsed = fTrue;
        Call( TFileWrapper<I>::ErrIORead( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq ) );
    }

HandleError:
    if ( group >= 0 )
    {
        EndAccess( group, psem );
    }
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
                                fIOREQUsed,
                                err );
}

template< class I >
ERR TFileFilter<I>::ErrWrite(   _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_                    const DWORD_PTR                 keyIOComplete,
                                _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR                     err     = JET_errSuccess;
    COffsets                offsets = COffsets( ibOffset, ibOffset + cbData - 1 );
    CMeteredSection::Group  group   = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem    = NULL;

    Assert( cbData > 0 );
    Assert( iom == IOMode::iomRaw ||
            iom == IOMode::iomEngine || 
            iom == IOMode::iomCacheWriteThrough || 
            iom == IOMode::iomCacheWriteBack );

    OSTrace(    JET_tracetagBlockCache,
                OSFormat(   "%s ErrIOWrite ib=%llu cb=%u fZeroed=%s fCachedFileHeader=%s iom=%d", 
                            OSFormat( this ), 
                            ibOffset, 
                            cbData, 
                            OSFormatBoolean( FUtilZeroed( pbData, cbData ) ),
                            OSFormatBoolean( CCachedFileHeader::FValid( m_pfsconfig, pbData, cbData ) ),
                            iom ) );


    if ( iom == IOMode::iomEngine )
    {
        Call( ErrBeginAccess( offsets, fTrue, &group, &psem ) );
        Call( ErrCacheWrite( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &group, &psem ) );
    }
    else if ( iom == IOMode::iomCacheWriteThrough )
    {
        Call( ErrVerifyAccess( offsets ) );
        Call( ErrWriteThrough( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &group, &psem ) );
        AtomicIncrement( &m_cCacheWriteThroughForIssue );
    }
    else if ( iom == IOMode::iomCacheWriteBack )
    {
        Call( ErrTryEnqueueWriteBack( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff ) );
    }
    else
    {
        Call( TFileWrapper<I>::ErrIOWrite( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff ) );
    }

HandleError:
    if ( group >= 0 )
    {
        EndAccess( group, psem );
    }
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrIssue( _In_ const IFileFilter::IOMode iom )
{
    ERR     err     = JET_errSuccess;
    BOOL    fIssue  = fFalse;

    Assert( iom == IOMode::iomRaw ||
            iom == IOMode::iomEngine ||
            iom == IOMode::iomCacheMiss ||
            iom == IOMode::iomCacheWriteThrough ||
            iom == IOMode::iomCacheWriteBack );

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrIOIssue iom=%u", OSFormat( this ), iom ) );

    switch ( iom )
    {
        case IOMode::iomRaw:
            fIssue = fTrue;
            break;

        case IOMode::iomEngine:
            fIssue = fTrue;
            AtomicExchange( &m_cCacheMissForIssue, 0 );
            AtomicExchange( &m_cCacheWriteThroughForIssue, 0 );
            break;

        case IOMode::iomCacheMiss:
            fIssue = AtomicExchange( &m_cCacheMissForIssue, 0 ) > 0;
            break;

        case IOMode::iomCacheWriteThrough:
            fIssue = AtomicExchange( &m_cCacheWriteThroughForIssue, 0 ) > 0;
            break;

        case IOMode::iomCacheWriteBack:
            if ( AtomicExchange( &m_cCacheWriteBackForIssue, 0 ) > 0 )
            {
                IssueWriteBacks();
            }
            break;
    }

    if ( fIssue )
    {
        Call( TFileWrapper<I>::ErrIOIssue() );
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
    COffsets                offsetsActual   = offsets;
    CMeteredSection::Group  group           = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem            = NULL;

    *pgroup = CMeteredSection::groupInvalidNil;
    *ppsem = NULL;


    const BOOL fNotYetAttached = m_pc && !m_pcfh && !m_initOnceAttach.FIsInit();
    const BOOL fNeedsAttach = fWrite && fNotYetAttached;
    if ( fNeedsAttach )
    {
        offsetsActual = COffsets( 0, offsetsActual.IbEnd() );
    }


    group = m_msPendingWriteBacks.Enter();


    if ( !FAccessPermitted( offsetsActual, group ) )
    {
        WaitForAccess( offsetsActual, &group );

        Assert( FAccessPermitted( offsetsActual, group ) );
    }


    if ( fNotYetAttached && offsetsActual.FOverlaps( s_offsetsCachedFileHeader ) )
    {
        psem = &m_semCachedFileHeader;
        psem->Acquire();
    }


    if ( fNeedsAttach )
    {
        Call( m_initOnceAttach.Init( ErrAttach_, this, offsets ) );
    }


    *pgroup = group;
    group = CMeteredSection::groupInvalidNil;


    *ppsem = psem;
    psem = NULL;

HandleError:
    EndAccess( group, psem );
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

    if ( *pgroup >= 0 )
    {
        m_msPendingWriteBacks.Leave( *pgroup );
        *pgroup = CMeteredSection::groupInvalidNil;
    }

    *pgroup = waiter.Wait();
}

template< class I >
BOOL TFileFilter<I>::FAccessPermitted( _In_ const COffsets& offsets, _In_ const CMeteredSection::Group group )
{
    if ( group < 0 )
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


    if ( !FAttached() )
    {
        Call( ErrERRCheck( JET_errInternalError ) );
    }


    if ( FAttached() && offsets.FOverlaps( s_offsetsCachedFileHeader ) )
    {
        Call( ErrERRCheck( JET_errInternalError ) );
    }

HandleError:
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrVerifyAccess( _In_ const COffsets& offsets )
{
    ERR err = JET_errSuccess;


    Call( ErrVerifyAccessIgnoringWriteBack( offsets ) );


    const CMeteredSection::Group group = m_msPendingWriteBacks.GroupActive();
    if ( !FAccessPermitted( offsets, group ) )
    {
        Call( ErrERRCheck( JET_errInternalError ) );
    }

HandleError:
    return err;
}

template< class I >
VOID TFileFilter<I>::EndAccess( _In_ const CMeteredSection::Group group, _In_ CSemaphore* const psem )
{
    if ( group >= 0 )
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
    ERR err = JET_errSuccess;


    if ( FAttached() )
    {

        Call( m_pc->ErrInvalidate( m_volumeid, m_fileid, m_fileserial, offsets.IbStart(), offsets.Cb() ) );
    }

HandleError:
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrAttach( _In_ const COffsets& offsetsFirstWrite )
{
    ERR                     err                         = JET_errSuccess;
    const QWORD             cbHeader                    = sizeof( CCachedFileHeader );
    QWORD                   cbSize                      = 0;
    QWORD                   cbSizeWithFirstWrite        = 0;
    void*                   pvData                      = NULL;
    TraceContextScope       tcScope( iorpBlockCache );
    CCachedFileHeader*      pcfh                        = NULL;
    BOOL                    fPresumeCached              = fFalse;
    BOOL                    fPresumeAttached            = fFalse;


    Call( TFileWrapper<I>::ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    cbSizeWithFirstWrite = max( cbSize, offsetsFirstWrite.IbEnd() + 1 );

    if ( cbSizeWithFirstWrite < cbHeader )
    {
        goto HandleError;
    }


    Alloc( pvData = PvOSMemoryPageAlloc( cbHeader, NULL ) );
    if ( cbSize > 0 )
    {
        Call( ErrRead(  *tcScope, 
                        0, 
                        (DWORD)min(cbHeader, cbSize), 
                        (BYTE*)pvData, 
                        qosIONormal, 
                        iomRaw,
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL ) );
    }


    Call( CCachedFileHeader::ErrCreate( this, m_pc, &pcfh ) );


    m_fileserial = pcfh->Fileserial();


    Call( m_pc->ErrWrite(   *tcScope,
                            m_volumeid,
                            m_fileid,
                            m_fileserial,
                            0,
                            cbHeader,
                            (const BYTE*)pvData,
                            qosIONormal,
                            cpBestEffort,
                            NULL,
                            NULL ));
    fPresumeCached = fTrue;
    Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );


    Call( ErrWrite( *tcScope, 0, cbHeader, (const BYTE*)pcfh, qosIONormal, iomRaw, NULL, NULL, NULL ) );
    fPresumeAttached = fTrue;
    Call( ErrFlushFileBuffers( iofrBlockCache ) );


    m_pcfh = pcfh;
    pcfh = NULL;


HandleError:
    if ( err < JET_errSuccess )
    {
        if ( fPresumeCached )
        {

            m_fileserial = fileserialInvalid;


            (void)m_pc->ErrClose( m_volumeid, m_fileid, pcfh->Fileserial() );
        }
    }
    delete pcfh;
    OSMemoryPageFree( pvData );
    return fPresumeAttached ? err : JET_errSuccess;
}

template< class I >
ERR TFileFilter<I>::ErrTryEnqueueWriteBack( _In_                    const TraceContext&             tc,
                                            _In_                    const QWORD                     ibOffset,
                                            _In_                    const DWORD                     cbData,
                                            _In_reads_( cbData )    const BYTE* const               pbData,
                                            _In_                    const OSFILEQOS                 grbitQOS,
                                            _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                            _In_                    const DWORD_PTR                 keyIOComplete,
                                            _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR                     err         = JET_errSuccess;
    CWriteBack*             pwriteback  = NULL;
    CMeteredSection::Group  group       = CMeteredSection::groupInvalidNil;
    BOOL                    fLeave      = fFalse;
    CWriteBackComplete      writeBackComplete( keyIOComplete, pfnIOHandoff );


    Call( ErrVerifyAccessIgnoringWriteBack( COffsets( ibOffset, ibOffset + cbData - 1 ) ) );


    group = m_msPendingWriteBacks.Enter();


    m_critPendingWriteBacks.Enter();
    fLeave = fTrue;


    const size_t cPendingWriteBacks = CPendingWriteBacks();
    const size_t cPendingWriteBackMax = CPendingWriteBackMax();

    if ( cPendingWriteBacks >= cPendingWriteBackMax )
    {
        Call( ErrERRCheck( errDiskTilt ) );
    }


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


    Alloc( pwriteback = new CWriteBack( tc,
                                        ibOffset,
                                        cbData,
                                        pbData,
                                        grbitQOS,
                                        pfnIOComplete ? pfnIOComplete : (IFileAPI::PfnIOComplete)CWriteBackComplete::IOComplete_,
                                        pfnIOComplete ? keyIOComplete : (DWORD_PTR)&writeBackComplete,
                                        pfnIOComplete ? pfnIOHandoff : (IFileAPI::PfnIOHandoff)CWriteBackComplete::IOHandoff_ ) );
    m_ilQueuedWriteBacks.InsertAsNextMost( pwriteback );

    AtomicIncrement( &m_cCacheWriteBackForIssue );

HandleError:
    if ( fLeave )
    {
        m_critPendingWriteBacks.Leave();
    }
    if ( group >= 0 )
    {
        m_msPendingWriteBacks.Leave( group );
    }
    if ( err >= JET_errSuccess && pwriteback && !pfnIOComplete )
    {
        CallS( ErrIssue( IOMode::iomCacheWriteBack ) );
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

    if ( m_semRequestWriteBacks.FTryAcquire() )
    {

        CMeteredSection::Group groupInactive = m_msPendingWriteBacks.GroupInactive();


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


        m_msPendingWriteBacks.Partition( (CMeteredSection::PFNPARTITIONCOMPLETE)TFileFilter<I>::RequestWriteBacks_, (DWORD_PTR)this );
    }
}

template<class I>
void TFileFilter<I>::RequestWriteBacks()
{

    ReleaseWaitingIOs();


    size_t cWriteBacksStarted = 0;
    size_t cWriteBacksFailed = 0;
    ERR err = JET_errSuccess;
    while ( err >= JET_errSuccess )
    {

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


        CWriteBack writebackCopy( *pwriteback );
        err = pwriteback->ErrWriteBack( this );


        if ( FTooManyWrites( err ) && !cWriteBacksStarted )
        {
            err = pwriteback->ErrWriteBack( this, fTrue );
        }


        if ( err >= JET_errSuccess )
        {
            cWriteBacksStarted++;
        }


        else
        {

            if ( FTooManyWrites( err ) )
            {
                m_critPendingWriteBacks.Enter();
                m_ilRequestedWriteBacks.Remove( pwriteback );
                m_ilQueuedWriteBacks.InsertAsPrevMost( pwriteback );
                m_critPendingWriteBacks.Leave();
            }


            else
            {
                m_critPendingWriteBacks.Enter();

                CWriteBack* pwritebackT = NULL;
                for (   pwritebackT = m_ilRequestedWriteBacks.PrevMost();
                        pwritebackT != NULL && pwritebackT != pwriteback;
                        pwritebackT = m_ilRequestedWriteBacks.Next( pwritebackT ) )
                {
                }

                if ( pwritebackT == pwriteback )
                {
                    Assert( err == JET_errOutOfMemory );
                    m_ilRequestedWriteBacks.Remove( pwriteback );
                }

                m_critPendingWriteBacks.Leave();

                if ( pwritebackT == pwriteback )
                {
                    delete pwriteback;
                }

                writebackCopy.Complete( err, this );

                cWriteBacksFailed++;


                err = JET_errSuccess;
            }
        }
    }


    if ( cWriteBacksStarted )
    {
        CallS( TFileWrapper<I>::ErrIOIssue() );
    }


    m_semRequestWriteBacks.Release();


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
void TFileFilter<I>::CompleteWriteBack( _In_ CWriteBack* const pwriteback )
{
    if ( !pwriteback )
    {
        return;
    }


    m_critPendingWriteBacks.Enter();
    m_ilRequestedWriteBacks.Remove( pwriteback );
    m_critPendingWriteBacks.Leave();

    delete pwriteback;


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

    return cpBestEffort;
}

template< class I >
ERR TFileFilter<I>::ErrCacheRead(   _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_                    const VOID *                    pioreq,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err         = JET_errSuccess;
    BOOL            fIOREQUsed  = fFalse;
    CIOComplete*    piocomplete = NULL;


    const ICache::CachingPolicy cp = CpGetCachingPolicy( tc, fFalse );


    if ( FAttached() )
    {

        if ( pfnIOComplete || pfnIOHandoff )
        {
            if ( pfnIOComplete )
            {
                Alloc( piocomplete = new CIOComplete(   fTrue,
                                                        this,
                                                        fFalse,
                                                        pgroup,
                                                        ppsem,
                                                        NULL,
                                                        ibOffset, 
                                                        cbData,
                                                        pbData,
                                                        pfnIOComplete, 
                                                        pfnIOHandoff,
                                                        keyIOComplete ) );
            }
            else
            {
                piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse, 
                                                                                    this,
                                                                                    fFalse, 
                                                                                    pgroup, 
                                                                                    ppsem,
                                                                                    NULL, 
                                                                                    ibOffset, 
                                                                                    cbData, 
                                                                                    pbData, 
                                                                                    pfnIOComplete, 
                                                                                    pfnIOHandoff, 
                                                                                    keyIOComplete );
            }
        }


        if ( pioreq )
        {
            fIOREQUsed = fTrue;
            ReleaseUnusedIOREQ( (void*)pioreq );
        }


        Call( m_pc->ErrRead(    tc,
                                m_volumeid,
                                m_fileid, 
                                m_fileserial,
                                ibOffset, 
                                cbData, 
                                pbData, 
                                grbitQOS,
                                cp, 
                                pfnIOComplete ? (ICache::PfnComplete)CIOComplete::Complete_ : NULL,
                                DWORD_PTR( piocomplete ) ) );
    }
    else
    {

        const ICacheTelemetry::FileNumber filenumber = Filenumber();
        const BOOL fCacheIfPossible = cp != cpDontCache;
        const ICacheTelemetry::BlockNumber blocknumberMax = Blocknumber( ibOffset + cbData + CbBlockSize() - 1 );
        for (   ICacheTelemetry::BlockNumber blocknumber = Blocknumber( ibOffset );
                blocknumber < blocknumberMax;
                blocknumber = (ICacheTelemetry::BlockNumber)( (QWORD)blocknumber + 1 ) )
        {
            m_pctm->Miss( filenumber, blocknumber, fTrue, fCacheIfPossible );
        }

        fIOREQUsed = fTrue;
        Call( ErrCacheMiss( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq, pgroup, ppsem ) );
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
                                fIOREQUsed,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrCacheMiss(   _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_                    const VOID *                    pioreq,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err         = JET_errSuccess;
    BOOL            fIOREQUsed  = fFalse;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        if ( pfnIOComplete )
        {
            Alloc( piocomplete = new CIOComplete(   fTrue,
                                                    this,
                                                    fFalse,
                                                    pgroup,
                                                    ppsem,
                                                    NULL,
                                                    ibOffset, 
                                                    cbData,
                                                    pbData,
                                                    pfnIOComplete, 
                                                    pfnIOHandoff,
                                                    keyIOComplete ) );
        }
        else
        {
            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse, 
                                                                                this,
                                                                                fFalse, 
                                                                                pgroup, 
                                                                                ppsem,
                                                                                NULL, 
                                                                                ibOffset, 
                                                                                cbData, 
                                                                                pbData, 
                                                                                pfnIOComplete, 
                                                                                pfnIOHandoff, 
                                                                                keyIOComplete );
        }
    }

    fIOREQUsed = fTrue;
    Call( ErrRead(  tc,
                    ibOffset,
                    cbData,
                    pbData,
                    grbitQOS, 
                    iomRaw,
                    pfnIOComplete ? (IFileAPI::PfnIOComplete)CIOComplete::IOComplete_ : NULL,
                    DWORD_PTR( piocomplete ),
                    piocomplete ? (IFileAPI::PfnIOHandoff)CIOComplete::IOHandoff_ : NULL,
                    pioreq ) );

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
                                fIOREQUsed,
                                err,
                                piocomplete );
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrCacheWrite(  _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;


    const ICache::CachingPolicy cp = CpGetCachingPolicy( tc, fTrue );


    if ( FAttached() )
    {
        
        if ( pfnIOComplete || pfnIOHandoff )
        {
            if ( pfnIOComplete )
            {
                Alloc( piocomplete = new CIOComplete(   fTrue,
                                                        this,
                                                        fTrue,
                                                        pgroup,
                                                        ppsem,
                                                        NULL,
                                                        ibOffset,
                                                        cbData,
                                                        pbData,
                                                        pfnIOComplete, 
                                                        pfnIOHandoff,
                                                        keyIOComplete ) );
            }
            else
            {
                piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse,
                                                                                    this, 
                                                                                    fTrue,
                                                                                    pgroup, 
                                                                                    ppsem, 
                                                                                    NULL, 
                                                                                    ibOffset,
                                                                                    cbData, 
                                                                                    pbData,
                                                                                    pfnIOComplete,
                                                                                    pfnIOHandoff,
                                                                                    keyIOComplete );
            }
        }

        Call( m_pc->ErrWrite(   tc,
                                m_volumeid, 
                                m_fileid, 
                                m_fileserial,
                                ibOffset, 
                                cbData, 
                                pbData, 
                                grbitQOS,
                                cp,
                                pfnIOComplete ? (ICache::PfnComplete)CIOComplete::Complete_ : NULL,
                                DWORD_PTR( piocomplete ) ) );


        if ( ( Fmf() & fmfStorageWriteBack ) == 0 )
        {
            Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );
        }
        else
        {
            AtomicIncrement( &m_cCacheWriteForFlush );
        }
    }
    else
    {

        const ICacheTelemetry::FileNumber filenumber = Filenumber();
        const BOOL fCacheIfPossible = cp != cpDontCache;
        const ICacheTelemetry::BlockNumber blocknumberMax = Blocknumber( ibOffset + cbData + CbBlockSize() - 1 );
        for (   ICacheTelemetry::BlockNumber blocknumber = Blocknumber( ibOffset );
                blocknumber < blocknumberMax;
                blocknumber = (ICacheTelemetry::BlockNumber)( (QWORD)blocknumber + 1 ) )
        {
            m_pctm->Miss( filenumber, blocknumber, fFalse, fCacheIfPossible );
            m_pctm->Update( filenumber, blocknumber );
            m_pctm->Write( filenumber, blocknumber, true );
        }

        Call( ErrWriteThrough( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pgroup, ppsem ) );
    }

HandleError:
    if ( piocomplete )
    {
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrWriteThrough(    _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_                    const DWORD_PTR                 keyIOComplete,
                                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                        _Inout_                 CMeteredSection::Group* const   pgroup,
                                        _Inout_                 CSemaphore** const              ppsem )
{
    return ErrWriteCommon( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pgroup, ppsem, NULL );
}

template< class I >
ERR TFileFilter<I>::ErrWriteBack(   _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_                    CWriteBack* const               pwriteback )
{
    return ErrWriteCommon( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, NULL, NULL, pwriteback );
}

template< class I >
ERR TFileFilter<I>::ErrWriteCommon( _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _In_reads_( cbData )    const BYTE* const               pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_opt_             CMeteredSection::Group* const   pgroup,
                                    _Inout_opt_             CSemaphore** const              ppsem,
                                    _In_opt_                CWriteBack* const               pwriteback )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        if ( pfnIOComplete )
        {
            Alloc( piocomplete = new CIOComplete(   fTrue,
                                                    this,
                                                    fTrue,
                                                    pgroup,
                                                    ppsem,
                                                    pwriteback,
                                                    ibOffset,
                                                    cbData,
                                                    pbData,
                                                    pfnIOComplete, 
                                                    pfnIOHandoff,
                                                    keyIOComplete ) );
        }
        else
        {
            piocomplete = new( _alloca( sizeof( CIOComplete ) ) ) CIOComplete(  fFalse,
                                                                                this, 
                                                                                fTrue,
                                                                                pgroup, 
                                                                                ppsem, 
                                                                                pwriteback, 
                                                                                ibOffset,
                                                                                cbData, 
                                                                                pbData,
                                                                                pfnIOComplete,
                                                                                pfnIOHandoff,
                                                                                keyIOComplete );
        }
    }

    Call( ErrWrite( tc,
                    ibOffset,
                    cbData,
                    pbData,
                    grbitQOS, 
                    iomRaw,
                    pfnIOComplete ? (IFileAPI::PfnIOComplete)CIOComplete::IOComplete_ : NULL,
                    DWORD_PTR( piocomplete ),
                    piocomplete ? (IFileAPI::PfnIOHandoff)CIOComplete::IOHandoff_ : NULL ) );

HandleError:
    if ( piocomplete )
    {
        if ( FTooManyWrites( err ) )
        {
            piocomplete->DoNotReleaseWriteBack();
        }
        piocomplete->Release( err, tc, grbitQOS );
    }
    return err;
}


class CFileFilter : public TFileFilter<IFileFilter>
{
    public:

        CFileFilter(    _Inout_     IFileAPI** const                    ppfapi,
                        _In_        IFileSystemFilter* const            pfsf,
                        _In_        IFileSystemConfiguration* const     pfsconfig,
                        _In_        ICacheTelemetry* const              pctm,
                        _In_        const VolumeId                      volumeid,
                        _In_        const FileId                        fileid,
                        _Inout_opt_ ICachedFileConfiguration** const    ppcfconfig,
                        _Inout_opt_ ICache** const                      ppc,
                        _Inout_opt_ CCachedFileHeader** const           ppcfh )
            :   TFileFilter<IFileFilter>( ppfapi, pfsf, pfsconfig, pctm, volumeid, fileid, ppcfconfig, ppc, ppcfh )
        {
        }

        virtual ~CFileFilter() {}
};



template< class I >
class TFileFilterWrapper
    :   public TFileWrapper<I>
{
    public:

        TFileFilterWrapper( _Inout_ I** const ppi, _In_ const IFileFilter::IOMode iom )
            :   TFileWrapper<I>( ppi ),
                m_iom( iom )
        {
        }

        TFileFilterWrapper( _In_ I* const pi, _In_ const IFileFilter::IOMode iom )
            :   TFileWrapper<I>( pi ),
                m_iom( iom )
        {
        }

    public:

        ERR ErrIORead(  _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_                    const VOID *                    pioreq ) override;
        ERR ErrIOWrite( _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIOIssue() override;

    public:

        ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                _Out_ FileId* const     pfileid,
                                _Out_ FileSerial* const pfileserial ) override;

        ERR ErrRead(    _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _Out_writes_( cbData )  BYTE* const                     pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                        _In_                    const VOID *                    pioreq ) override;
        ERR ErrWrite(   _In_                    const TraceContext&             tc,
                        _In_                    const QWORD                     ibOffset,
                        _In_                    const DWORD                     cbData,
                        _In_reads_( cbData )    const BYTE* const               pbData,
                        _In_                    const OSFILEQOS                 grbitQOS,
                        _In_                    const IFileFilter::IOMode       iom,
                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                        _In_                    const DWORD_PTR                 keyIOComplete,
                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override;
        ERR ErrIssue( _In_ const IFileFilter::IOMode iom ) override;

    private:

        const IFileFilter::IOMode m_iom;
};

template< class I >
ERR TFileFilterWrapper<I>::ErrGetPhysicalId(    _Out_ VolumeId* const   pvolumeid,
                                                _Out_ FileId* const     pfileid,
                                                _Out_ FileSerial* const pfileserial )
{
    return m_piInner->ErrGetPhysicalId( pvolumeid, pfileid, pfileserial );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIORead(   _In_                    const TraceContext&                 tc,
                                        _In_                    const QWORD                         ibOffset,
                                        _In_                    const DWORD                         cbData,
                                        _Out_writes_( cbData )  __out_bcount( cbData ) BYTE* const  pbData,
                                        _In_                    const OSFILEQOS                     grbitQOS,
                                        _In_                    const IFileAPI::PfnIOComplete       pfnIOComplete,
                                        _In_                    const DWORD_PTR                     keyIOComplete,
                                        _In_                    const IFileAPI::PfnIOHandoff        pfnIOHandoff,
                                        _In_                    const VOID *                        pioreq )
{
    return ErrRead( tc, ibOffset, cbData, pbData, grbitQOS, m_iom, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIOWrite(  _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_                    const DWORD_PTR                 keyIOComplete,
                                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    return ErrWrite( tc, ibOffset, cbData, pbData, grbitQOS, m_iom, pfnIOComplete, keyIOComplete, pfnIOHandoff );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIOIssue()
{
    return ErrIssue( m_iom );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrRead( _In_                    const TraceContext&             tc,
                                    _In_                    const QWORD                     ibOffset,
                                    _In_                    const DWORD                     cbData,
                                    _Out_writes_( cbData )  BYTE* const                     pbData,
                                    _In_                    const OSFILEQOS                 grbitQOS,
                                    _In_                    const IFileFilter::IOMode       iom,
                                    _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_                    const DWORD_PTR                 keyIOComplete,
                                    _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_                    const VOID *                    pioreq )
{
    return m_piInner->ErrRead( tc, ibOffset, cbData, pbData, grbitQOS, iom, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrWrite(    _In_                    const TraceContext&             tc,
                                        _In_                    const QWORD                     ibOffset,
                                        _In_                    const DWORD                     cbData,
                                        _In_reads_( cbData )    const BYTE* const               pbData,
                                        _In_                    const OSFILEQOS                 grbitQOS,
                                        _In_                    const IFileFilter::IOMode       iom,
                                        _In_                    const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_                    const DWORD_PTR                 keyIOComplete,
                                        _In_                    const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    return m_piInner->ErrWrite( tc, ibOffset, cbData, pbData, grbitQOS, iom, pfnIOComplete, keyIOComplete, pfnIOHandoff );
}

template< class I >
ERR TFileFilterWrapper<I>::ErrIssue( _In_ const IFileFilter::IOMode iom )
{
    return m_piInner->ErrIssue( iom );
}


class CFileFilterWrapper : public TFileFilterWrapper<IFileFilter>
{
    public:

        CFileFilterWrapper( _Inout_ IFileFilter** const ppff, _In_ const IFileFilter::IOMode iom )
            :   TFileFilterWrapper<IFileFilter>( ppff, iom )
        {
        }

        CFileFilterWrapper( _In_ IFileFilter* const pff, _In_ const IFileFilter::IOMode iom )
            :   TFileFilterWrapper<IFileFilter>( pff, iom )
        {
        }
};


