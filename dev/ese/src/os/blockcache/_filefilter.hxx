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
    public:  //  specialized API

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
                            _Inout_     ICache** const                      ppc,
                            _Inout_opt_ CCachedFileHeader** const           ppcfh )
        {
            m_pcfconfig = *ppcfconfig;
            *ppcfconfig = NULL;

            if ( ppc && *ppc )
            {
                m_pc = &m_cWrapper;
                m_cWrapper.SetCache( ppc );
            }

            m_pcfh = ppcfh ? *ppcfh : NULL;
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

    public:  //  IFileFilter

        ERR ErrGetPhysicalId(   _Out_ VolumeId* const   pvolumeid,
                                _Out_ FileId* const     pfileid,
                                _Out_ FileSerial* const pfileserial ) override;

    public:  //  IFileAPI

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

                typename CInvasiveList<CWaiter, OffsetOfILE>::CElement  m_ile;
                COffsets                                                m_offsets;
                CManualResetSignal                                      m_sig;
                CMeteredSection::Group                                  m_group;
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

        BOOL FAttached() const { return m_pc && m_pcfh; }

        ICache::CachingPolicy CpGetCachingPolicy( _In_ const TraceContext& tc, _In_ const BOOL fWrite );

        ERR ErrCacheRead(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _Out_writes_( cbData )  BYTE* const                     pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_opt_                const VOID *                    pioreq,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );
        ERR ErrCacheMiss(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _Out_writes_( cbData )  BYTE* const                     pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_opt_                const VOID *                    pioreq,
                            _Inout_                 CMeteredSection::Group* const   pgroup,
                            _Inout_                 CSemaphore** const              ppsem );

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
        ERR ErrWriteThrough(    _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _Inout_                 CMeteredSection::Group* const   pgroup,
                                _Inout_                 CSemaphore** const              ppsem );
        ERR ErrWriteBack(   _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _In_                    CWriteBack* const               pwriteback );
        ERR ErrWriteCommon( _In_                    const TraceContext&             tc,
                            _In_                    const QWORD                     ibOffset,
                            _In_                    const DWORD                     cbData,
                            _In_reads_( cbData )    const BYTE* const               pbData,
                            _In_                    const OSFILEQOS                 grbitQOS,
                            _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                            _In_opt_                const DWORD_PTR                 keyIOComplete,
                            _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                            _Inout_opt_             CMeteredSection::Group* const   pgroup,
                            _Inout_opt_             CSemaphore** const              ppsem,
                            _In_opt_                CWriteBack* const               pwriteback );

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

                void SetCache( _Inout_ ICache** const ppc )
                {
                    m_pcInner = *ppc;
                    *ppc = NULL;
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

                ERR ErrDump( _In_ CPRINTF* const pcprintf ) override
                {
                    return ErrToErr( "Dump", m_pcInner->ErrDump( pcprintf ) );
                }

                ERR ErrGetCacheType( _Out_writes_( cbGuid ) BYTE* const rgbCacheType ) override
                {
                    return ErrToErr( "GetCacheType", m_pcInner->ErrGetCacheType( rgbCacheType ) );
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

        typedef CInitOnce< ERR, decltype( &ErrAttach_ ), TFileFilter<I>* const, const COffsets& > CInitOnceAttach;

        IFileSystemFilter* const                                    m_pfsf;
        IFileSystemConfiguration* const                             m_pfsconfig;
        ICacheTelemetry* const                                      m_pctm;
        VolumeId                                                    m_volumeid;
        FileId                                                      m_fileid;
        FileSerial                                                  m_fileserial;
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

    protected:

        //  IO completion context for an IFileFilter implementation.

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
                        m_pff->EndAccess( &m_groupPendingWriteBacks, &m_psemCachedFileHeader );
                        m_pff->CompleteWriteBack( m_fReleaseWriteback ? m_pwriteback : NULL );
                    }
                }

            private:

                TFileFilter<I>* const   m_pff;
                const BOOL              m_fWrite;
                CMeteredSection::Group  m_groupPendingWriteBacks;
                CSemaphore*             m_psemCachedFileHeader;
                CWriteBack* const       m_pwriteback;
                BOOL                    m_fReleaseWriteback;
                volatile BOOL           m_fReleaseResources;
        };
};

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
        m_pc( ppc && *ppc ? &m_cWrapper : NULL ),
        m_cWrapper( this, ppc ),
        m_pcfh( ppcfh ? *ppcfh : NULL ),
        m_filenumber( filenumberInvalid ),
        m_cbBlockSize( 0 ),
        m_offsetsCachedFileHeader( 0, sizeof( CCachedFileHeader ) ),
        m_critWaiters( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_critWaiters" ), rankFileFilter, 0 ) ),
        m_critPendingWriteBacks( CLockBasicInfo( CSyncBasicInfo( "TFileFilter<I>::m_critPendingWriteBacks" ), rankFileFilter, 0 ) ),
        m_cPendingWriteBacksMax( 0 ),
        m_semRequestWriteBacks( CSyncBasicInfo( "TFileFilter<I>::m_semRequestWriteBacks" ) ),
        m_cCacheWriteForFlush( 0 ),
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
    return ErrFlush( iofr, iomEngine );
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
            min(offsets.Cb(), m_offsetsCachedFileHeader.IbEnd() - offsets.IbStart() + 1 ) :
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
    ERR                     err         = JET_errSuccess;
    BOOL                    fIOREQUsed  = fFalse;
    COffsets                offsets     = COffsets( ibOffset, ibOffset + cbData - 1 );
    CMeteredSection::Group  group       = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem        = NULL;

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
        fIOREQUsed = fTrue;
        Call( ErrCacheRead( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq, &group, &psem ) );
    }
    else if ( iom == iomCacheMiss )
    {
        Call( ErrVerifyAccess( offsets ) );
        fIOREQUsed = fTrue;
        Call( ErrCacheMiss( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq, &group, &psem ) );
    }
    else
    {
        fIOREQUsed = fTrue;
        Call( TFileWrapper<I>::ErrIORead( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, pioreq ) );
    }

HandleError:
    if ( group != CMeteredSection::groupInvalidNil )
    {
        EndAccess( &group, &psem );
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
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff )
{
    ERR                     err     = JET_errSuccess;
    COffsets                offsets = COffsets( ibOffset, ibOffset + cbData - 1 );
    CMeteredSection::Group  group   = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem    = NULL;

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
        Call( ErrWriteThrough( tc, ibOffset, cbData, pbData, grbitQOS, pfnIOComplete, keyIOComplete, pfnIOHandoff, &group, &psem ) );
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
    if ( group != CMeteredSection::groupInvalidNil )
    {
        EndAccess( &group, &psem );
    }
    return err;
}

template< class I >
ERR TFileFilter<I>::ErrIssue( _In_ const IFileFilter::IOMode iom )
{
    ERR     err         = JET_errSuccess;
    BOOL    fIssue      = fFalse;
    BOOL    fIssueCache = fFalse;

    Assert( iom == iomRaw ||
            iom == iomEngine ||
            iom == iomCacheMiss ||
            iom == iomCacheWriteThrough ||
            iom == iomCacheWriteBack );

    OSTrace( JET_tracetagBlockCache, OSFormat( "%s ErrIOIssue iom=%u", OSFormat( this ), iom ) );

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
    COffsets                offsetsActual   = offsets;
    CMeteredSection::Group  group           = CMeteredSection::groupInvalidNil;
    CSemaphore*             psem            = NULL;

    *pgroup = CMeteredSection::groupInvalidNil;
    *ppsem = NULL;

    //  if this is a write and the file is cached and not yet attached then expand the offset range to include the
    //  header so that we can write our cached file header

    const BOOL fNotYetAttached = m_pc && !m_pcfh && !m_initOnceAttach.FIsInit();
    const BOOL fNeedsAttach = fWrite && fNotYetAttached;
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
                                    NULL ));
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
    ERR                 err                         = JET_errSuccess;
    QWORD               cbSize                      = 0;
    QWORD               cbSizeWithFirstWrite        = 0;
    void*               pvData                      = NULL;
    TraceContextScope   tcScope( iorpBlockCache );
    CCachedFileHeader*  pcfh                        = NULL;
    BOOL                fPresumeCached              = fFalse;
    BOOL                fPresumeAttached            = fFalse;

    //  we do not currently support caching files opened for write through

    if ( ( Fmf() & fmfStorageWriteBack ) == 0 )
    {
        goto HandleError;
    }

    //  get the size of the cached file and extend to include the proposed write.  if it is smaller than the header
    //  then don't attach it.  our contract is to not cause any noticeable changes to the file meta-data such as by
    //  changing the file size

    Call( TFileWrapper<I>::ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    cbSizeWithFirstWrite = max( cbSize, offsetsFirstWrite.IbEnd() + 1 );

    if ( cbSizeWithFirstWrite < m_offsetsCachedFileHeader.Cb() )
    {
        goto HandleError;
    }

    //  read the data to be displaced by the cached file header, if any

    Alloc( pvData = PvOSMemoryPageAlloc( m_offsetsCachedFileHeader.Cb(), NULL ) );
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

    Call( CCachedFileHeader::ErrCreate( this, m_pc, &pcfh ) );

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
                            NULL ));
    fPresumeCached = fTrue;
    Call( m_pc->ErrFlush( m_volumeid, m_fileid, m_fileserial ) );

    //  write the header to the file, making it cached.  once this happens we must presume it is attached
    //
    //  NOTE:  if the cached file header doesn't make it to the file then the cache will be loaded with an image of the
    //  initial bytes of this file.  however, it will be keyed to a FileSerial that didn't get stamped on the file.  so
    //  the cache will end up tossing that write later because the target cached file doesn't exist.  also once we fail
    //  we will remove the ability of the cache to open this already open file for write through / write back.  so we 
    //  are not at risk of writing this stale data back to the file

    memset( pvData, 0, m_offsetsCachedFileHeader.Cb() );
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
                                    _In_opt_                const VOID *                    pioreq,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err                     = JET_errSuccess;
    BOOL            fCleanUpStateSaved      = fFalse;
    BOOL            fRestoreCleanupState    = fFalse;
    BOOL            fIOREQUsed              = fFalse;
    CIOComplete*    piocomplete             = NULL;
    OSFILEQOS       grbitQOSActual          = grbitQOS;

    //  determine the caching policy for this read

    const ICache::CachingPolicy cp = CpGetCachingPolicy( tc, fFalse );

    //  if the file isn't attached to the cache then read as if it is a cache miss.  otherwise, use the cache

    if ( FAttached() )
    {
        //  cache accesses will allocate memory for sync reads

        fCleanUpStateSaved = FOSSetCleanupState( fFalse );
        fRestoreCleanupState = fTrue;

        //  try to service the read via the cache

        if ( pfnIOComplete || pfnIOHandoff )
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

        //  release any reserved IOREQ because we will not be using it

        if ( pioreq )
        {
            fIOREQUsed = fTrue;
            ReleaseUnusedIOREQ( (void*)pioreq );
        }

        //  AEG  handle the cases where a read request can be rejected which have not already been granted by the
        //       reserved pioreq:  qosIOOptimizeCombinable
        //       -  note that we must intercept ErrReserveIOREQ and handle it there as well!

        //  strip any grbitQOS bits that can result in the rejection of the read request that we have handled.  if we
        //  missed any then that will be caught by the below assert that we never see errDiskTilt
        //
        //  NOTE:  we allow qosIODispatchBackground because this indicates IO smoothing and won't result in rejection

        grbitQOSActual &= ~qosIOOptimizeCombinable;

        //  ask the cache for this data
        //
        //  NOTE:  the handoff reference to the io completion context occurs in the Release call on exit

        err = m_pc->ErrRead(    tc,
                                m_volumeid,
                                m_fileid, 
                                m_fileserial,
                                ibOffset, 
                                cbData, 
                                pbData, 
                                grbitQOSActual,
                                cp, 
                                pfnIOComplete ? CIOComplete::Complete_ : NULL,
                                DWORD_PTR( piocomplete ) );

        //  reject on request due to grbitQOS should not happen in the cache.  that should be handled here

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
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _In_opt_                const VOID *                    pioreq,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err         = JET_errSuccess;
    BOOL            fIOREQUsed  = fFalse;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
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

    fIOREQUsed = fTrue;
    Call( ErrRead(  tc,
                    ibOffset,
                    cbData,
                    pbData,
                    grbitQOS, 
                    iomRaw,
                    pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                    DWORD_PTR( piocomplete ),
                    piocomplete ? CIOComplete::IOHandoff_ : NULL,
                    pioreq ) );

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
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_                 CMeteredSection::Group* const   pgroup,
                                    _Inout_                 CSemaphore** const              ppsem )
{
    ERR             err             = JET_errSuccess;
    CIOComplete*    piocomplete     = NULL;
    OSFILEQOS       grbitQOSActual  = grbitQOS;

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

        //  AEG  handle the cases where a write request can be rejected:  qosIOOptimizeCombinable,
        //       qosIODispatchBackground and qosIODispatchUrgentBackgroundMask

        //  strip any grbitQOS bits that can result in the rejection of the write request that we have handled.  if we
        //  missed any then that will be caught by the below assert that we never see errDiskTilt

        grbitQOSActual &= ~qosIOOptimizeCombinable;
        grbitQOSActual &= ~qosIODispatchBackground;
        grbitQOSActual &= ~qosIODispatchUrgentBackgroundMask;

        //  if we don't have a dispatch mode then specify dispatch immediate

        if ( ( grbitQOSActual & qosIODispatchMask ) == 0 )
        {
            grbitQOSActual |= qosIODispatchImmediate;
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
                                grbitQOSActual,
                                cp,
                                pfnIOComplete ? CIOComplete::Complete_ : NULL,
                                DWORD_PTR( piocomplete ) );

        //  reject on request due to grbitQOS should not happen in the cache.  that should be handled here

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
                                        _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                        _In_opt_                const DWORD_PTR                 keyIOComplete,
                                        _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
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
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
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
                                    _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                    _In_opt_                const DWORD_PTR                 keyIOComplete,
                                    _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                    _Inout_opt_             CMeteredSection::Group* const   pgroup,
                                    _Inout_opt_             CSemaphore** const              ppsem,
                                    _In_opt_                CWriteBack* const               pwriteback )
{
    ERR             err         = JET_errSuccess;
    CIOComplete*    piocomplete = NULL;

    if ( pfnIOComplete || pfnIOHandoff )
    {
        const BOOL fHeap = pfnIOComplete != NULL;
        Alloc( piocomplete = new( fHeap ? new Buffer<CIOComplete>() : _malloca( sizeof( CIOComplete ) ) )
            CIOComplete(    fHeap,
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

    Call( ErrWrite( tc,
                    ibOffset,
                    cbData,
                    pbData,
                    grbitQOS, 
                    iomRaw,
                    pfnIOComplete ? CIOComplete::IOComplete_ : NULL,
                    DWORD_PTR( piocomplete ),
                    piocomplete ? CIOComplete::IOHandoff_ : NULL ) );

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

//  CFileFilter:  concrete TFileFilter<IFileFilter>

class CFileFilter : public TFileFilter<IFileFilter>
{
    public:  //  specialized API

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

        static void Cleanup() { CIOComplete::Cleanup(); }
};
