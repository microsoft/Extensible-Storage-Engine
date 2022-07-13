// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


//  THashedLRUKCache:  a persistent write back cache implemented as a fixed size hash of chunks each running the LRUK
//  cache replacement algorithm.  crash resilience is provided by a journal

template< class I > class CHashedLRUKCacheThreadLocalStorage;

template< class I >
using THashedLRUKCacheBase = TCacheBase<I, CHashedLRUKCachedFileTableEntry<I>, CHashedLRUKCacheThreadLocalStorage<I>>;

template< class I >
class THashedLRUKCache
    :   public THashedLRUKCacheBase<I>
{
    public:

        ~THashedLRUKCache();

    public:  //  ICache

        ERR ErrCreate() override;

        ERR ErrMount() override;

        ERR ErrPrepareToDismount() override;

        ERR ErrDump( _In_ CPRINTF* const pcprintf ) override;

        BOOL FEnabled() override;

        ERR ErrFlush(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

        ERR ErrDestage( _In_        const VolumeId                  volumeid,
                        _In_        const FileId                    fileid,
                        _In_        const FileSerial                fileserial,
                        _In_opt_    const ICache::PfnDestageStatus  pfnDestageStatus,
                        _In_opt_    const DWORD_PTR                 keyDestageStatus ) override;

        ERR ErrInvalidate(  _In_ const VolumeId     volumeid,
                            _In_ const FileId       fileid,
                            _In_ const FileSerial   fileserial,
                            _In_ const QWORD        ibOffset,
                            _In_ const QWORD        cbData ) override;

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
                        _In_opt_                const DWORD_PTR             keyComplete ) override;

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
                        _In_opt_                const DWORD_PTR             keyComplete ) override;

        ERR ErrIssue(   _In_ const VolumeId     volumeid,
                        _In_ const FileId       fileid,
                        _In_ const FileSerial   fileserial ) override;

    protected:
        
        THashedLRUKCache(   _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig, 
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch );

        IFileFilter* PffCaching() const { return (IFileFilter*)&m_ffCaching; }

        ERR ErrGetThreadLocalStorage( _Out_ CHashedLRUKCacheThreadLocalStorage<I>** const ppctls ) override;

    private:

        friend CHashedLRUKCacheThreadLocalStorage<I>;
        friend CHashedLRUKCachedFileTableEntry<I>;

        //  Request context.

        class CRequest
            :   protected THashedLRUKCacheBase<I>::CRequest
        {
            public:

                CRequest(   _In_                    const BOOL                                  fRead,
                            _In_                    THashedLRUKCache<I>* const                  pc,
                            _In_                    const TraceContext&                         tc,
                            _Inout_                 CHashedLRUKCachedFileTableEntry<I>** const  ppcfte,
                            _In_                    const QWORD                                 ibOffset,
                            _In_                    const DWORD                                 cbData,
                            _In_reads_( cbData )    const BYTE* const                           pbData,
                            _In_                    const OSFILEQOS                             grbitQOS,
                            _In_                    const ICache::CachingPolicy                 cp,
                            _In_opt_                const ICache::PfnComplete                   pfnComplete,
                            _In_opt_                const DWORD_PTR                             keyComplete )
                    :   THashedLRUKCacheBase<I>::CRequest(  fRead,
                                                            pc,
                                                            tc,
                                                            ppcfte,
                                                            ibOffset,
                                                            cbData, 
                                                            pbData, 
                                                            grbitQOS, 
                                                            cp,
                                                            pfnComplete, 
                                                            keyComplete ),
                        m_rgcwcc
                        {
                            CClusterWriteCompletionContext( this, (CMeteredSection::Group)0 ),
                            CClusterWriteCompletionContext( this, (CMeteredSection::Group)1 )
                        },
                        m_cCachedFileIO( 0 ),
                        m_cCachingFileIO( 0 ),
                        m_iorl( this ),
                        m_pfnIORangeLockAcquired( NULL ),
                        m_keyIORangeLockAcquired( NULL )
                {
                    m_ilRequestsByIO.InsertAsPrevMost( this );
                }

                BOOL FRead() const { return THashedLRUKCacheBase<I>::CRequest::FRead(); }
                THashedLRUKCache<I>* Pc() const { return (THashedLRUKCache<I>*)THashedLRUKCacheBase<I>::CRequest::Pc(); }
                CHashedLRUKCachedFileTableEntry<I>* Pcfte() const { return THashedLRUKCacheBase<I>::CRequest::Pcfte(); }
                const COffsets& Offsets() const { return THashedLRUKCacheBase<I>::CRequest::Offsets(); }
                const BYTE* const PbData() const { return THashedLRUKCacheBase<I>::CRequest::PbData(); }
                ICache::CachingPolicy Cp() const { return THashedLRUKCacheBase<I>::CRequest::Cp(); }

                ERR ErrStatus() const { return THashedLRUKCacheBase<I>::CRequest::ErrStatus(); }
                typename CHashedLRUKCachedFileTableEntry<I>::CIORangeLockBase* Piorl() { return &m_iorl; }
                BOOL FIOCompleted() const { return m_msIO.FEmpty(); }

                COffsets OffsetsForIO() const
                {
                    CRequest* const prequestIOFirst = m_ilRequestsByIO.PrevMost();
                    const QWORD     ibStartIOFirst  = prequestIOFirst->Offsets().IbStart();
                    CRequest* const prequestIOLast  = m_ilRequestsByIO.NextMost();
                    const QWORD     ibEndIOLast     = prequestIOLast->Offsets().IbEnd();

                    return COffsets( ibStartIOFirst, ibEndIOLast );
                }

                typedef void (*PfnIORangeLockAcquired)( const DWORD_PTR dwCompletionKey );

                void WaitForIORangeLock(    _In_opt_ CRequest::PfnIORangeLockAcquired   pfnIORangeLockAcquired  = NULL,
                                            _In_opt_ const DWORD_PTR                    keyIORangeLockAcquired  = NULL )
                {
                    if ( pfnIORangeLockAcquired )
                    {
                        //  save the completion context

                        m_pfnIORangeLockAcquired = pfnIORangeLockAcquired;
                        m_keyIORangeLockAcquired = keyIORangeLockAcquired;

                        //  request the IO range lock, calling the completion when acquired

                        const BOOL fIORangeLockPending = Pcfte()->FTryRequestIORangeLock( Piorl() );
                        EnforceSz( fIORangeLockPending, "WaitForIORangeLock" );
                    }
                    else
                    {
                        //  acquire the IO range lock, waiting if necessary

                        Pcfte()->AcquireIORangeLock( Piorl() );
                    }
                }

                typedef CMeteredSection::PFNPARTITIONCOMPLETE PfnIOComplete;

                void WaitForIO( _In_opt_ CRequest::PfnIOComplete    pfnIOComplete   = NULL,
                                _In_opt_ const DWORD_PTR            keyIOComplete   = NULL )
                {
                    IssueIO();

                    m_msIO.Partition( pfnIOComplete, keyIOComplete );
                }

                ERR ErrWriteCluster(    _In_                ICachedBlockSlab* const pcbs,
                                        _In_                const CCachedBlockSlot& slot,
                                        _In_                const size_t            cb,
                                        _In_reads_( cb )    const BYTE* const       rgb )
                {
                    ERR                                     err     = JET_errSuccess;
                    const CClusterWriteCompletionContext*   pcwcc   = PcwccGetClusterCompletionContext();

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x F=%s Write Cluster %s",
                                            OSFormatFileId( Pc() ),
                                            QWORD( this ),
                                            OSFormatFileId( Pcfte()->Pff() ),
                                            OSFormat( slot ) ) );

                    Call( pcwcc->ErrWriteCluster( pcbs, slot, cb, rgb ) );
                    pcwcc = NULL;

                    m_cCachingFileIO++;

                    //  we must immediately issue after writing a cluster to avoid a deadlock in ErrFlushClusters when
                    //  this thread must block to access another slab.  the cluster write is in TLS and can't be
                    //  flushed by ErrFlushClusters.  this should not hurt IO coalescing in any core scenario

                    IssueIO();

                HandleError:
                    if ( pcwcc )
                    {
                        pcwcc->Cancel();
                    }
                    return err;
                }

                ERR ErrReadCluster( _In_                ICachedBlockSlab* const pcbs,
                                    _In_                const CCachedBlockSlot& slot,
                                    _In_                const size_t            cb,
                                    _Out_writes_( cb )  BYTE* const             rgb )
                {
                    ERR err = JET_errSuccess;

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x F=%s Read Cluster %s",
                                            OSFormatFileId( Pc() ),
                                            QWORD( this ),
                                            OSFormatFileId( Pcfte()->Pff() ),
                                            OSFormat( slot ) ) );

                    Call( pcbs->ErrReadCluster( slot,
                                                cb, 
                                                rgb, 
                                                ClusterReadComplete_, 
                                                DWORD_PTR( this ),
                                                ClusterReadHandoff_ ) );

                    m_cCachingFileIO++;

                HandleError:
                    return err;
                }

                ERR ErrReadCachedFile( _In_ const QWORD ibOffset, _In_ const DWORD cbData )
                {
                    ERR         err     = JET_errSuccess;
                    BYTE* const pbData  = (BYTE*)PbData() + ibOffset - Offsets().IbStart();

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x F=%s Read Block ib=%llu cb=%u",
                                            OSFormatFileId( Pc() ),
                                            QWORD( this ),
                                            OSFormatFileId( Pcfte()->Pff() ),
                                            ibOffset,
                                            cbData ) );

                    Call( ErrRead( Pcfte()->Pff(), ibOffset, cbData, pbData, iomCacheMiss ) );

                    m_cCachedFileIO++;

                HandleError:
                    return err;
                }

                ERR ErrWriteCachedFile( _In_ const QWORD ibOffset, _In_ const DWORD cbData )
                {
                    ERR                 err     = JET_errSuccess;
                    const BYTE* const   pbData  = PbData() + ibOffset - Offsets().IbStart();

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x F=%s Write Block ib=%llu cb=%u",
                                            OSFormatFileId( Pc() ),
                                            QWORD( this ),
                                            OSFormatFileId( Pcfte()->Pff() ),
                                            ibOffset,
                                            cbData ) );

                    Call( ErrWrite( Pcfte()->Pff(), ibOffset, cbData, pbData, iomCacheWriteThrough ) );

                    m_cCachedFileIO++;

                HandleError:
                    return err;
                }

                void Fail( _In_ const ERR err )
                {
                    THashedLRUKCacheBase<I>::CRequest::Fail( err );
                }

                static ERR ErrRelease( _Inout_ CRequest** const pprequest, _In_ const ERR errIn )
                {
                    CRequest* const prequest    = *pprequest;
                    ERR             err         = errIn;

                    *pprequest = NULL;

                    if ( prequest )
                    {
                        err = prequest->ErrRelease( err );
                    }

                    return err;
                }

                static SIZE_T OffsetOfRequestsByThread() { return OffsetOf( CRequest, m_ileRequestsByThread ); }
                static SIZE_T OffsetOfIOs() { return OffsetOf( CRequest, m_ileIOs ); }
                static SIZE_T OffsetOfRequestsByIO() { return OffsetOf( CRequest, m_ileRequestsByIO ); }

                CCountedInvasiveList<CRequest, CRequest::OffsetOfRequestsByIO>& IlRequestsByIO() { return m_ilRequestsByIO; }

            public:

                //  Trace context scope

                class CTraceContextScope : public THashedLRUKCacheBase<I>::CRequest::CTraceContextScope
                {
                    public:

                        CTraceContextScope( _In_ const CRequest* const prequest )
                            :   THashedLRUKCacheBase<I>::CRequest::CTraceContextScope( prequest )
                        {
                        }

                        CTraceContextScope( CTraceContextScope&& rvalue )
                            :   THashedLRUKCacheBase<I>::CRequest::CTraceContextScope( std::move( rvalue ) )
                        {
                        }

                    private:

                        CTraceContextScope( const CTraceContextScope& tc ) = delete;
                        const CTraceContextScope& operator=( const CTraceContextScope& tc ) = delete;
                };


            protected:

                ~CRequest()
                {
                    Assert( m_msIO.FEmpty() );
                }

                void Start() override
                {
                    THashedLRUKCacheBase<I>::CRequest::Start();

                    if ( m_msIO.GroupActive() != 0 )
                    {
                        m_msIO.Partition();
                    }

                    const CMeteredSection::Group group = m_msIO.Enter();
                    Assert( group == 0 );
                }

                void Finish( _In_ const ERR err ) override
                {
                    m_msIO.Leave( 0 );

                    THashedLRUKCacheBase<I>::CRequest::Finish( err );
                }

            private:

                class CClusterWriteCompletionContext  //  cwcc
                {
                    public:

                        CClusterWriteCompletionContext( _In_ CRequest* const prequest, _In_ const CMeteredSection::Group group )
                            :   m_prequest( prequest ),
                                m_group( group )
                        {
                        }

                        ERR ErrWriteCluster(    _In_                ICachedBlockSlab* const pcbs,
                                                _In_                const CCachedBlockSlot& slot,
                                                _In_                const size_t            cb,
                                                _In_reads_( cb )    const BYTE* const       rgb ) const
                        {
                            ERR err = JET_errSuccess;

                            Call( pcbs->ErrWriteCluster(    slot,
                                                            cb,
                                                            rgb,
                                                            ClusterWriteComplete_,
                                                            DWORD_PTR( this ),
                                                            ClusterWriteHandoff_ ) );

                        HandleError:
                            return err;
                        }

                        void Cancel() const
                        {
                            m_prequest->ClusterWriteCanceled( m_group );
                        }

                    private:
        
                        static void ClusterWriteComplete_(  _In_ const ERR          err,
                                                            _In_ const DWORD_PTR    keyClusterWritten )
                        {
                            ICachedBlockSlab::PfnClusterWritten pfnClusterWrite = ClusterWriteComplete_;
                            Unused( pfnClusterWrite );

                            CClusterWriteCompletionContext* const pcwcc = (CClusterWriteCompletionContext*)keyClusterWritten;
                            pcwcc->m_prequest->ClusterWriteComplete( err, pcwcc->m_group );
                        }

                        static void ClusterWriteHandoff_( _In_ const DWORD_PTR keyClusterHandoff )
                        {
                            ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff = ClusterWriteHandoff_;
                            Unused( pfnClusterHandoff );

                            CClusterWriteCompletionContext* const pcwcc = (CClusterWriteCompletionContext*)keyClusterHandoff;
                            pcwcc->m_prequest->ClusterWriteHandoff();
                        }

                    private:

                        CRequest* const                 m_prequest;
                        const CMeteredSection::Group    m_group;
                };

                class CIORangeLock : public CHashedLRUKCachedFileTableEntry<I>::CIORangeLockBase
                {
                    public:

                        CIORangeLock( _In_ CRequest* const prequest )
                            :   CHashedLRUKCachedFileTableEntry<I>::CIORangeLockBase( prequest->Pcfte() ),
                                m_prequest( prequest )
                        {
                        }

                        QWORD QwContext() const override { return (QWORD)m_prequest; }

                        COffsets Offsets() const override
                        {
                            return m_prequest->OffsetsForIO();
                        }

                        void Grant() override
                        {
                            CHashedLRUKCachedFileTableEntry<I>::CIORangeLockBase::Grant();

                            if ( m_prequest->m_pfnIORangeLockAcquired )
                            {
                                m_prequest->m_pfnIORangeLockAcquired( m_prequest->m_keyIORangeLockAcquired );
                            }
                        }

                    private:

                        CRequest* const m_prequest;
                };

            private:

                const CClusterWriteCompletionContext* PcwccGetClusterCompletionContext()
                {
                    return &m_rgcwcc[(int)Pc()->GroupForClusterWrite()];
                }

                void ClusterWriteCanceled( const CMeteredSection::Group group )
                {
                    Pc()->ClusterWritten( JET_errSuccess, group );
                }

                void ClusterWriteComplete(  _In_ const ERR                      err,
                                            _In_ const CMeteredSection::Group   group )
                {
                    Pc()->ClusterWritten( err, group );

                    Finish( err );
                }

                void ClusterWriteHandoff()
                {
                    Start();
                }

                void ClusterReadComplete( _In_ const ERR err )
                {
                    Finish( err );
                }

                void ClusterReadHandoff()
                {
                    Start();
                }

                static void ClusterReadComplete_(   _In_ const ERR          err,
                                                    _In_ const DWORD_PTR    keyClusterRead )
                {
                    ICachedBlockSlab::PfnClusterRead pfnClusterRead = ClusterReadComplete_;
                    Unused( pfnClusterRead );

                    CRequest* const prequest = (CRequest*)keyClusterRead;
                    prequest->ClusterReadComplete( err );
                }

                static void ClusterReadHandoff_( _In_ const DWORD_PTR keyClusterHandoff )
                {
                    ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff = ClusterReadHandoff_;
                    Unused( pfnClusterHandoff );

                    CRequest* const prequest = (CRequest*)keyClusterHandoff;
                    prequest->ClusterReadHandoff();
                }

                ERR ErrRelease( _In_ const ERR err )
                {
                    IssueIO();

                    return THashedLRUKCacheBase<I>::CRequest::ErrRelease( err );
                }

                void IssueIO()
                {
                    if ( m_cCachedFileIO )
                    {
                        m_cCachedFileIO = 0;
                        CallS( Pcfte()->Pff()->ErrIssue( FRead() ? iomCacheMiss : iomCacheWriteThrough ) );
                    }

                    if ( m_cCachingFileIO )
                    {
                        m_cCachingFileIO = 0;
                        CallS( Pc()->PffCaching()->ErrIOIssue() );
                    }
                }

            private:

                const CClusterWriteCompletionContext                                        m_rgcwcc[ 2 ];
                CMeteredSection                                                             m_msIO;
                int                                                                         m_cCachedFileIO;
                int                                                                         m_cCachingFileIO;
                typename CCountedInvasiveList<CRequest, OffsetOfRequestsByThread>::CElement m_ileRequestsByThread;
                typename CCountedInvasiveList<CRequest, OffsetOfIOs>::CElement              m_ileIOs;
                CCountedInvasiveList<CRequest, OffsetOfRequestsByIO>                        m_ilRequestsByIO;
                typename CCountedInvasiveList<CRequest, OffsetOfRequestsByIO>::CElement     m_ileRequestsByIO;
                CIORangeLock                                                                m_iorl;
                CRequest::PfnIORangeLockAcquired                                            m_pfnIORangeLockAcquired;
                DWORD_PTR                                                                   m_keyIORangeLockAcquired;
        };

        //  Update Slab Visitor

        class CUpdateSlabVisitor
        {
            public:

                CUpdateSlabVisitor( _In_        THashedLRUKCache<I>* const  pc,
                                    _In_opt_    ICachedBlockSlab* const     pcbsA,
                                    _In_opt_    ICachedBlockSlab* const     pcbsB   = NULL )
                    :   m_pc( pc ),
                        m_rgvc
                        {
                            CVisitContext( this, pcbsA ),
                            CVisitContext( this, pcbsB )
                        },
                        m_errVisitSlots( JET_errSuccess )
                {
                    m_arrayCachedBlockUpdate.SetCapacityGrowth( CCachedBlockChunk::Ccbl() );
                    m_arrayClusterSwap.SetCapacityGrowth( CCachedBlockChunk::Ccbl() );
                }

                size_t Ccbu() const { return m_arrayCachedBlockUpdate.Size(); }
                const CCachedBlockUpdate* Rgcbu() const { return m_arrayCachedBlockUpdate.PEntry( 0 ); }

                ERR ErrVisitSlots()
                {
                    ERR err = JET_errSuccess;

                    //  visit all the slots in each slab

                    for ( size_t ivc = 0; ivc < s_cvc; ivc++ )
                    {
                        Call( m_rgvc[ ivc ].ErrVisitSlots() );
                    }

                    //  check that we visited all slots successfully

                    Call( m_errVisitSlots );

                    //  check that we have no unmatched cluster swaps

                    for ( int i = 0; i < m_arrayClusterSwap.Size(); i++ )
                    {
                        if (    m_arrayClusterSwap[ i ].ClnoA() != clnoInvalid &&
                                m_arrayClusterSwap[ i ].ClnoB() != clnoInvalid )
                        {
                            Error( m_pc->ErrBlockCacheInternalError( "MismatchedClusterSwaps" ) );
                        }
                    }

                HandleError:
                    return err;
                }

            private:

                ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const ERR                      errChunk,
                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent )
                {
                    ERR     err     = JET_errSuccess;
                    QWORD   ibSlab  = 0;

                    //  ignore chunks that are in an error state

                    if ( errChunk < JET_errSuccess )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore slabs that haven't been updated

                    if ( !slotstCurrent.FSlabUpdated() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore chunks that haven't been updated

                    if ( !slotstCurrent.FChunkUpdated() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  if this slot is from a hash slab and it was updated and the contents were evicted or
                    //  invalidated then we must swap the cluster with a new journal cluster to maintain our recovery
                    //  scheme

                    Call( pcbs->ErrGetPhysicalId( &ibSlab ) );

                    if (    m_pc->FHashSlab( ibSlab ) &&
                            slotstCurrent.FSlotUpdated() &&
                            slotstAccepted.FValid() && !slotstCurrent.FValid() )
                    {
                        if ( slotstAccepted.Clno() == slotstCurrent.Clno() )
                        {
                            Error( m_pc->ErrBlockCacheInternalError( "ClusterMustBeSwappedOnEvictOrInvalidate" ) );
                        }
                    }

                    //  accumulate any cluster swaps made for this slot

                    if ( slotstAccepted.Clno() != slotstCurrent.Clno() )
                    {
                        BOOL fMatched = fFalse;

                        for ( int i = 0; !fMatched && i < m_arrayClusterSwap.Size(); i++ )
                        {
                            if (    slotstCurrent.Clno() == m_arrayClusterSwap[ i ].ClnoA() &&
                                    slotstAccepted.Clno() == m_arrayClusterSwap[ i ].ClnoB() )
                            {
                                Call( ErrToErr<CArray<CClusterSwap>>( m_arrayClusterSwap.ErrSetEntry( i, CClusterSwap() ) ) );
                                fMatched = fTrue;
                            }
                        }

                        if ( !fMatched )
                        {
                            Call( ErrToErr<CArray<CClusterSwap>>( m_arrayClusterSwap.ErrSetEntry(   m_arrayClusterSwap.Size(),
                                                                                                    CClusterSwap(   slotstAccepted.Clno(),
                                                                                                                    slotstCurrent.Clno() ) ) ) );
                        }
                    }

                    //  accumulate this slot for the journal entry

                    Call( ErrToErr<CArray<CCachedBlockUpdate>>( m_arrayCachedBlockUpdate.ErrSetEntry(   m_arrayCachedBlockUpdate.Size(),
                                                                                                        CCachedBlockUpdate( slotstCurrent ) ) ) );

                HandleError:
                    return err;
                }

                BOOL FVisitSlot(    _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const ERR                      errChunk,
                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent )
                {
                    ERR err = JET_errSuccess;

                    //  visit the slot

                    Call( ErrVisitSlot_( pcbs, errChunk, slotstAccepted, slotstCurrent ) );

                    //  continue visiting slots if we didn't fail
                    //
                    //  NOTE:  we use JET_errNoCurrentRecord as a signal to stop processing slots

                HandleError:
                    if ( err != JET_errNoCurrentRecord )
                    {
                        m_errVisitSlots = m_errVisitSlots < JET_errSuccess ? m_errVisitSlots : err;
                    }
                    return err != JET_errNoCurrentRecord;
                }

                class CVisitContext  //  vc
                {
                    public:

                        CVisitContext(  _In_        CUpdateSlabVisitor* const   pusv,
                                        _In_opt_    ICachedBlockSlab* const     pcbs )
                            :   m_pusv( pusv ),
                                m_pcbs( pcbs )
                        {
                        }

                        ERR ErrVisitSlots() const
                        {
                            ERR err = JET_errSuccess;

                            //  if there is no slab then we're done

                            if ( !m_pcbs )
                            {
                                Error( JET_errSuccess );
                            }

                            //  visit the slots in this slab

                            Call( m_pcbs->ErrVisitSlots( FVisitSlot_, (DWORD_PTR)this ) );

                        HandleError:
                            return err;
                        }

                    private:
        
                        static BOOL FVisitSlot_(    _In_ const ERR                      errChunk,
                                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                                    _In_ const DWORD_PTR                keyVisitSlot )
                        {
                            ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FVisitSlot_;
                            Unused( pfnVisitSlot );

                            CVisitContext* const pvc = (CVisitContext*)keyVisitSlot;
                            return pvc->m_pusv->FVisitSlot( pvc->m_pcbs, errChunk, slotstAccepted, slotstCurrent );
                        }

                    private:

                        CUpdateSlabVisitor* const   m_pusv;
                        ICachedBlockSlab* const     m_pcbs;
                };

            private:

                class CClusterSwap
                {
                    public:

                        CClusterSwap()
                            :   m_clnoA( clnoInvalid ),
                                m_clnoB( clnoInvalid )
                        {
                        }

                        CClusterSwap( _In_ const ClusterNumber clnoA, _In_ const ClusterNumber clnoB )
                            :   m_clnoA( clnoA ),
                                m_clnoB( clnoB )
                        {
                        }

                        ClusterNumber ClnoA() const { return m_clnoA; }
                        ClusterNumber ClnoB() const { return m_clnoB; }

                        CClusterSwap& operator=( _In_ const CClusterSwap& other )
                        {
                            this->m_clnoA = other.m_clnoA;
                            this->m_clnoB = other.m_clnoB;

                            return *this;
                        }

                    private:

                        ClusterNumber               m_clnoA;
                        ClusterNumber               m_clnoB;
                };

            private:

                static const size_t         s_cvc                       = 2;

                THashedLRUKCache<I>* const  m_pc;
                CVisitContext               m_rgvc[ s_cvc ];
                ERR                         m_errVisitSlots;
                CArray<CCachedBlockUpdate>  m_arrayCachedBlockUpdate;
                CArray<CClusterSwap>        m_arrayClusterSwap;
        };

        //  Cached Block Slab Visitor

        class CCachedBlockSlabVisitor
        {
            public:

                CCachedBlockSlabVisitor( _In_opt_ ICachedBlockSlab* const pcbs )
                    :   m_pcbs( pcbs ),
                        m_errVisitSlots( JET_errSuccess )
                {
                }

                ERR ErrVisitSlots()
                {
                    ERR err = JET_errSuccess;

                    //  visit the slots in the slab

                    Call( ErrVisitSlots_( m_pcbs, FConsiderUpdate_, DWORD_PTR( this ) ) );

                    //  check that we visited all slots successfully

                    Call( m_errVisitSlots );

                HandleError:
                    return err;
                }

            protected:

                virtual ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                            _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                            _In_ const DWORD_PTR                            keyConsiderUpdate ) = 0;

                virtual ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const CCachedBlockSlot&        slotNew ) = 0;

            private:

                BOOL FVisitSlot(    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew )
                {
                    ERR err = JET_errSuccess;

                    //  visit the slot

                    Call( ErrVisitSlot_( m_pcbs, slotstCurrent, slotNew ) );

                    //  continue visiting slots if we didn't fail
                    //
                    //  NOTE:  we use JET_errNoCurrentRecord as a signal to stop processing slots

                HandleError:
                    if ( err != JET_errNoCurrentRecord )
                    {
                        m_errVisitSlots = m_errVisitSlots < JET_errSuccess ? m_errVisitSlots : err;
                    }
                    return err != JET_errNoCurrentRecord;
                }

                static BOOL FConsiderUpdate_(   _In_ const CCachedBlockSlotState&   slotstCurrent,
                                                _In_ const CCachedBlockSlot&        slotNew,
                                                _In_ const DWORD_PTR                keyConsiderUpdate )
                {
                    ICachedBlockSlab::PfnConsiderUpdate pfnConsiderUpdate = FConsiderUpdate_;
                    Unused( pfnConsiderUpdate );

                    CCachedBlockSlabVisitor* const pcbsv = (CCachedBlockSlabVisitor*)keyConsiderUpdate;
                    return pcbsv->FVisitSlot( slotstCurrent, slotNew );
                }

            private:

                ICachedBlockSlab* const m_pcbs;
                ERR                     m_errVisitSlots;
        };

        //  Invalidate Slab Visitor

        class CInvalidateSlabVisitor : public CCachedBlockSlabVisitor
        {
            public:

                static ERR ErrExecute(  _In_    THashedLRUKCache<I>* const                  pc,
                                        _Inout_ ICachedBlockSlab** const                    ppcbs,
                                        _In_    CHashedLRUKCachedFileTableEntry<I>* const   pcfte,
                                        _In_    const COffsets                              offsets )
                {
                    return CInvalidateSlabVisitor( pc, ppcbs, pcfte, offsets ).ErrInvalidateSlots();
                }

                ~CInvalidateSlabVisitor()
                {
                    m_pc->ReleaseSlab( JET_errSuccess, &m_pcbsJournal );
                }

            protected:

                CInvalidateSlabVisitor( _In_    THashedLRUKCache<I>* const                  pc,
                                        _Inout_ ICachedBlockSlab** const                    ppcbs,
                                        _In_    CHashedLRUKCachedFileTableEntry<I>* const   pcfte,
                                        _In_    const COffsets                              offsets )
                    :   CCachedBlockSlabVisitor( *ppcbs ),
                        m_pc( pc ),
                        m_pcbs( *ppcbs ),
                        m_pcfte( pcfte ),
                        m_offsets( offsets ),
                        m_pcbsJournal( NULL )
                {
                }

                ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                    _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                {
                    return pcbs->ErrInvalidateSlots( pfnConsiderUpdate, keyConsiderUpdate );
                }

                ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    return ErrInvalidateSlot( pcbs, slotstCurrent, slotNew );
                }

            private:

                ERR ErrInvalidateSlots()
                {
                    ERR err = JET_errSuccess;

                    //  invalidate the slots

                    Call( ErrVisitSlots() );

                    //  update the affected slabs

                HandleError:
                    err = ErrAccumulateError( err, ErrUpdateSlabs() );
                    return err;
                }

                ERR ErrInvalidateSlot(  _In_ ICachedBlockSlab* const        pcbs,
                                        _In_ const CCachedBlockSlotState&   slotstCurrent,
                                        _In_ const CCachedBlockSlot&        slotNew )
                {
                    ERR                     err                 = JET_errSuccess;
                    const CCachedBlockId&   cbid                = slotstCurrent.Cbid();
                    const QWORD             ibCachedBlock       = (QWORD)cbid.Cbno() * cbCachedBlock;
                    const COffsets          offsetsCachedBlock  = COffsets( ibCachedBlock, ibCachedBlock + cbCachedBlock - 1 );

                    //  ignore any slot that is not for the target file

                    if ( cbid.Volumeid() != m_pcfte->Volumeid() )
                    {
                        Error( JET_errSuccess );
                    }
                    if ( cbid.Fileid() != m_pcfte->Fileid() )
                    {
                        Error( JET_errSuccess );
                    }
                    if ( cbid.Fileserial() != m_pcfte->Fileserial() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore any slot that is not for the target offset range

                    if ( !m_offsets.FContains( offsetsCachedBlock ) )
                    {
                        Error( JET_errSuccess );
                    }

                    //  invalidate the slot

                    Call( m_pc->ErrInvalidateSlot( pcbs, slotNew, &m_pcbsJournal ) );

                HandleError:
                    return err;
                }

                ERR ErrUpdateSlabs()
                {
                    ERR err = JET_errSuccess;

                    //  if we moved clusters due to invalidating cached data then we must perform our changes in one
                    //  atomic update

                    if ( m_pcbsJournal && m_pcbsJournal->FUpdated() )
                    {
                        Call( m_pc->ErrUpdateSlabs( &m_pcbs, &m_pcbsJournal ) );
                    }

                    //  release our journal slab

                HandleError:
                    m_pc->ReleaseSlab( err, &m_pcbsJournal );
                    return err;
                }

            private:

                THashedLRUKCache<I>* const                  m_pc;
                ICachedBlockSlab*&                          m_pcbs;
                CHashedLRUKCachedFileTableEntry<I>* const   m_pcfte;
                const COffsets                              m_offsets;
                ICachedBlockSlab*                           m_pcbsJournal;
        };

        //  Write Back

        class CWriteBack  //  wb
        {
            public:

                static SIZE_T OffsetOfILE() { return OffsetOf( CWriteBack, m_ile ); }

                static ERR ErrRegister( _In_        THashedLRUKCache<I>* const                          pc,
                                        _In_opt_    CRequest* const                                     prequest,
                                        _In_        ICachedBlockSlab* const                             pcbs,
                                        _Inout_     CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>& ilWriteBack,
                                        _In_        const CCachedBlockSlotState&                        slotstCurrent,
                                        _In_        CHashedLRUKCachedFileTableEntry<I>* const           pcfte,
                                        _In_        const COffsets                                      offsets )
                {
                    ERR         err     = JET_errSuccess;
                    CWriteBack* pwbNew  = NULL;
                    CWriteBack* pwb     = NULL;

                    //  create the write back

                    Call( ErrCreate( pc, prequest, pcbs, slotstCurrent, pcfte, offsets, &pwbNew ) );

                    //  insert the write back into our write back list in order by cached file and cached file offset
                    //  because we are optimizing the cached file write back IO

                    ilWriteBack.InsertAsNextMost( pwbNew );
                    pwb = pwbNew;
                    pwbNew = NULL;

                    for (   CWriteBack* pwbPrev = ilWriteBack.Prev( pwb );
                            pwbPrev && CWriteBack::Cmp( pwbPrev, pwb ) > 0;
                            pwbPrev = ilWriteBack.Prev( pwbPrev ) )
                    {
                        ilWriteBack.Remove( pwb );
                        ilWriteBack.Insert( pwb, pwbPrev );
                        pwbPrev = pwb;
                    }

                HandleError:
                    delete pwbNew;
                    return err;
                }

                static void Release( _Inout_ CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>& ilWriteBack )
                {
                    while ( CWriteBack* pwb = ilWriteBack.PrevMost() )
                    {
                        ilWriteBack.Remove( pwb );
                        Release( &pwb );
                    }
                }

                static void Release( _Inout_ CWriteBack** const ppwb )
                {
                    delete* ppwb;
                    *ppwb = NULL;
                }

                CHashedLRUKCachedFileTableEntry<I>* Pcfte() const { return m_pcfte; }

                const COffsets& Offsets() const { return m_offsets; }

                void StartRead()
                {
                    ERR                 err = JET_errSuccess;
                    TraceContextScope   tc( iorpBlockCache );

                    //  read the data from the caching file

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x F=%s Read Cluster %s",
                                            OSFormatFileId( m_pc ),
                                            QWORD( m_prequest ),
                                            OSFormatFileId( m_pcfte->Pff() ),
                                            OSFormat( m_slot ) ) );

                    Call( m_pcbs->ErrReadCluster(   m_slot,
                                                    (size_t)( m_offsets.Cb() ), 
                                                    m_rgbData, 
                                                    ClusterReadComplete_, 
                                                    DWORD_PTR( this ),
                                                    ClusterReadHandoff_ ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        ClusterReadComplete( err );
                    }
                }

                void StartWrite()
                {
                    ERR                 err = JET_errSuccess;
                    TraceContextScope   tc( iorpBlockCache );

                    //  wait for the read to complete

                    m_msigRead.Wait();
                    Call( m_errRead );

                    //  verify the read

                    m_errVerify = m_pcbs->ErrVerifyCluster( m_slot, (size_t)m_offsets.Cb(), m_rgbData );
                    if ( m_errVerify < JET_errSuccess )
                    {
                        m_errVerify = m_pc->ErrUnexpectedDataReadFailure(   m_pcfte,
                                                                            m_offsets,
                                                                            "CCleanSlabVisitor::CWriteBack::ErrRead",
                                                                            m_errVerify, 
                                                                            ErrERRCheck( JET_errDiskIO ) );
                        Error( m_errVerify );
                    }

                    //  forget our slab because we don't need it anymore and to allow the caller to release it after
                    //  calling StartWrite without worrying about incorrect behavior

                    m_pcbs = NULL;

                    //  write the data to the cached file
                    //
                    //  NOTE:  we use iomCacheWriteThrough rather than iomCacheWriteBack because we already
                    //  protect against conflicting IO with our IO range locks and because the write back
                    //  mechanism doesn't work for sync write back during a sync IO

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x F=%s Write Block ib=%llu cb=%llu",
                                            OSFormatFileId( m_pc ),
                                            QWORD( m_prequest ),
                                            OSFormatFileId( m_pcfte->Pff() ),
                                            m_offsets.IbStart(),
                                            m_offsets.Cb() ) );

                    Call( m_pcfte->Pff()->ErrWrite( *tc,
                                                    m_offsets.IbStart(),
                                                    (DWORD)m_offsets.Cb(),
                                                    m_rgbData,
                                                    qosIONormal,
                                                    iomCacheWriteThrough,
                                                    WriteComplete_, 
                                                    DWORD_PTR( this ),
                                                    WriteHandoff_ ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        WriteComplete( err );
                    }
                }

                void WaitForWriteCompletion()
                {
                    //  wait for the write to complete if started

                    m_msigWrite.Wait();
                }

                void CompleteFlush( _In_ const ERR errFlush )
                {
                    ERR err = JET_errSuccess;

                    m_errFlush = errFlush;

                    Call( m_errRead );
                    Call( m_errVerify );
                    Call( m_errWrite );
                    Call( m_errFlush );

                HandleError:
                    m_errComplete = err;
                }

                ERR ErrComplete() const { return m_errComplete; }

                BOOL FAffectedSlot( _In_ const CCachedBlockSlot& slot ) const
                {
                    //  if the specified slot isn't the same as our slot then bail

                    if ( slot.Chno() != m_slot.Chno() )
                    {
                        return fFalse;
                    }
                    if ( slot.Slno() != m_slot.Slno() )
                    {
                        return fFalse;
                    }

                    //  if we didn't complete the write then it didn't affect the slot

                    if ( ErrComplete() < JET_errSuccess )
                    {
                        return fFalse;
                    }

                    return fTrue;
                }

            private:

                static ERR ErrCreate(   _In_        THashedLRUKCache<I>* const                  pc,
                                        _In_opt_    CRequest* const                             prequest,
                                        _In_        ICachedBlockSlab* const                     pcbs,
                                        _In_        const CCachedBlockSlot&                     slot,
                                        _In_        CHashedLRUKCachedFileTableEntry<I>* const   pcfte,
                                        _In_        const COffsets                              offsets,
                                        _Out_       CWriteBack** const                          ppwb )
                {
                    ERR                                 err     = JET_errSuccess;
                    BYTE*                               rgbData = NULL;
                    CWriteBack*                         pwb     = NULL;

                    *ppwb = NULL;

                    Alloc( rgbData = (BYTE*)PvOSMemoryPageAlloc( cbCachedBlock, NULL ) );
                    Alloc( pwb = new CWriteBack( pc, prequest, pcbs, slot, pcfte, offsets, &rgbData ) );

                    *ppwb = pwb;
                    pwb = NULL;

                HandleError:
                    OSMemoryPageFree( rgbData );
                    return err;
                }

                CWriteBack( _In_        THashedLRUKCache<I>* const                  pc,
                            _In_opt_    CRequest* const                             prequest,
                            _In_        ICachedBlockSlab* const                     pcbs,
                            _In_        const CCachedBlockSlot&                     slot,
                            _In_        CHashedLRUKCachedFileTableEntry<I>* const   pcfte,
                            _In_        const COffsets                              offsets,
                            _Inout_     BYTE** const                                prgbData )
                    :   m_pc( pc ),
                        m_prequest( prequest ),
                        m_pcbs( pcbs ),
                        m_slot( slot ),
                        m_pcfte( pcfte ),
                        m_offsets( offsets ),
                        m_rgbData( *prgbData ),
                        m_errRead( JET_errInternalError ),
                        m_msigRead( CSyncBasicInfo( "THashedLRUKCache<I>::CCleanSlabVisitor::CWriteBack::m_msigRead" ) ),
                        m_errVerify( JET_errInternalError ),
                        m_errWrite( JET_errInternalError ),
                        m_msigWrite( CSyncBasicInfo( "THashedLRUKCache<I>::CCleanSlabVisitor::CWriteBack::m_msigWrite" ) ),
                        m_errFlush( JET_errInternalError ),
                        m_errComplete( JET_errInternalError )
                {
                    m_pcfte->AddRef();
                    *prgbData = NULL;
                    m_msigRead.Set();
                    m_msigWrite.Set();
                }

                ~CWriteBack()
                {
                    m_msigRead.Wait();
                    m_msigWrite.Wait();
                    OSMemoryPageFree( m_rgbData );
                    m_rgbData = NULL;
                    m_pc->ReleaseCachedFile( &m_pcfte );
                }

                void ClusterReadHandoff()
                {
                    m_msigRead.Reset();
                }

                void ClusterReadComplete( _In_ const ERR err )
                {
                    m_errRead = err;
                    m_msigRead.Set();
                }

                void WriteHandoff()
                {
                    m_msigWrite.Reset();
                }

                void WriteComplete( _In_ const ERR err )
                {
                    m_errWrite = err;
                    m_msigWrite.Set();
                }

                static void ClusterReadComplete_(   _In_ const ERR          err,
                                                    _In_ const DWORD_PTR    keyClusterRead )
                {
                    ICachedBlockSlab::PfnClusterRead pfnClusterRead = ClusterReadComplete_;
                    Unused( pfnClusterRead );

                    CWriteBack* const pwb = (CWriteBack*)keyClusterRead;
                    pwb->ClusterReadComplete( err );
                }

                static void ClusterReadHandoff_( _In_ const DWORD_PTR keyClusterRead )
                {
                    ICachedBlockSlab::PfnClusterHandoff pfnClusterHandoff = ClusterReadHandoff_;
                    Unused( pfnClusterHandoff );

                    CWriteBack* const pwb = (CWriteBack*)keyClusterRead;
                    pwb->ClusterReadHandoff();
                }

                static void WriteComplete_( _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete )
                {
                    IFileAPI::PfnIOComplete pfnIOComplete = WriteComplete_;
                    Unused( pfnIOComplete );

                    CWriteBack* const pwb = (CWriteBack*)keyIOComplete;
                    pwb->WriteComplete( err );
                }

                static void WriteHandoff_(  _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete,
                                            _In_                    void* const             pvIOContext )
                {
                    IFileAPI::PfnIOHandoff pfnIOHandoff = WriteHandoff_;
                    Unused( pfnIOHandoff );

                    CWriteBack* const pwb = (CWriteBack*)keyIOComplete;
                    pwb->WriteHandoff();
                }

                static int Cmp( _In_ CWriteBack* const pwbA, _In_ CWriteBack* const pwbB )
                {
                    //  write backs from different cached files are ordered by their volume id and file id for
                    //  deterministic behavior

                    if ( pwbA->m_pcfte->Volumeid() < pwbB->m_pcfte->Volumeid() )
                    {
                        return -1;
                    }
                    else if ( pwbA->m_pcfte->Volumeid() > pwbB->m_pcfte->Volumeid() )
                    {
                        return 1;
                    }

                    if ( pwbA->m_pcfte->Fileid() < pwbB->m_pcfte->Fileid() )
                    {
                        return -1;
                    }
                    else if ( pwbA->m_pcfte->Fileid() > pwbB->m_pcfte->Fileid() )
                    {
                        return 1;
                    }

                    if ( pwbA->m_pcfte->Fileserial() < pwbB->m_pcfte->Fileserial() )
                    {
                        return -1;
                    }
                    else if ( pwbA->m_pcfte->Fileserial() > pwbB->m_pcfte->Fileserial() )
                    {
                        return 1;
                    }

                    //  write backs whose offsets are in ascending order are not out of order

                    if ( pwbA->m_offsets.IbStart() < pwbB->m_offsets.IbStart() )
                    {
                        return -1;
                    }
                    else if ( pwbA->m_offsets.IbStart() > pwbB->m_offsets.IbStart() )
                    {
                        return 1;
                    }

                    return 0;
                }

            private:

                THashedLRUKCache<I>* const                                  m_pc;
                CRequest* const                                             m_prequest;
                ICachedBlockSlab*                                           m_pcbs;
                const CCachedBlockSlot                                      m_slot;
                CHashedLRUKCachedFileTableEntry<I>*                         m_pcfte;
                const COffsets                                              m_offsets;
                BYTE*                                                       m_rgbData;
                typename CInvasiveList<CWriteBack, OffsetOfILE>::CElement   m_ile;
                ERR                                                         m_errRead;
                CManualResetSignal                                          m_msigRead;
                ERR                                                         m_errVerify;
                ERR                                                         m_errWrite;
                CManualResetSignal                                          m_msigWrite;
                ERR                                                         m_errFlush;
                ERR                                                         m_errComplete;
        };

        //  Filtered Slot Slab Visitor

        class CFilteredSlotSlabVisitor : public CCachedBlockSlabVisitor
        {
            protected:

                CFilteredSlotSlabVisitor(   _In_        ICachedBlockSlab* const                     pcbs,
                                            _In_opt_    CHashedLRUKCachedFileTableEntry<I>* const   pcfte )
                        :   CCachedBlockSlabVisitor( pcbs ),
                            m_pilWriteBack( NULL ),
                            m_pcfte( pcfte )
                {
                }

                CFilteredSlotSlabVisitor(   _In_        ICachedBlockSlab* const                                     pcbs,
                                            _In_opt_    CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>* const   pilWriteBack )
                        :   CCachedBlockSlabVisitor( pcbs ),
                            m_pilWriteBack( pilWriteBack ),
                            m_pcfte( NULL )
                {
                }

                virtual ERR ErrVisitFilteredSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                                    _In_ const CCachedBlockSlot&        slotNew ) = 0;

                ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    ERR     err             = JET_errSuccess;
                    BOOL    fInteresting    = fFalse;

                    //  if we are filtering by write backs then determine if this slot was written back

                    if ( m_pilWriteBack )
                    {
                        for (   CWriteBack* pwb = m_pilWriteBack->PrevMost();
                                pwb && !fInteresting;
                                pwb = m_pilWriteBack->Next( pwb ) )
                        {
                            if ( pwb->FAffectedSlot( slotstCurrent ) )
                            {
                                fInteresting = fTrue;
                            }
                        }
                    }

                    //  if we are filtering by file then determine if this slot is part of that file

                    if ( m_pcfte )
                    {
                        if (    slotstCurrent.Cbid().Volumeid() == m_pcfte->Volumeid() &&
                                slotstCurrent.Cbid().Fileid() == m_pcfte->Fileid() &&
                                slotstCurrent.Cbid().Fileserial() == m_pcfte->Fileserial() )
                        {
                            fInteresting = fTrue;
                        }
                    }

                    //  ignore any slot that isn't interesting

                    if ( !fInteresting )
                    {
                        Error( JET_errSuccess );
                    }

                    //  visit the filtered slot

                    Call( ErrVisitFilteredSlot_( pcbs, slotstCurrent, slotNew ) );

                HandleError:
                    return err;
                }

            private:

                CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>* const   m_pilWriteBack;
                CHashedLRUKCachedFileTableEntry<I>* const                   m_pcfte;
        };

        //  Write Back Slots Slab Visitor

        class CWriteBackSlotsSlabVisitor : public CFilteredSlotSlabVisitor
        {
            public:

                static ERR ErrExecute(  _In_        THashedLRUKCache<I>* const                          pc,
                                        _In_        ICachedBlockSlab* const                             pcbs,
                                        _In_opt_    CHashedLRUKCachedFileTableEntry<I>* const           pcfte,
                                        _Inout_     CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>& ilWriteBack )
                {
                    return CWriteBackSlotsSlabVisitor( pc, pcbs, pcfte, ilWriteBack ).ErrVisitSlots();
                }

            protected:

                CWriteBackSlotsSlabVisitor( _In_        THashedLRUKCache<I>* const                          pc,
                                            _In_        ICachedBlockSlab* const                             pcbs,
                                            _In_opt_    CHashedLRUKCachedFileTableEntry<I>* const           pcfte,
                                            _Inout_     CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>& ilWriteBack )
                        :   CFilteredSlotSlabVisitor( pcbs, pcfte ),
                            m_pc( pc ),
                            m_pcbs( pcbs ),
                            m_pcfte( pcfte ),
                            m_ilWriteBack( ilWriteBack )
                {
                }

                ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                    _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                {
                    return pcbs->ErrCleanSlots( pfnConsiderUpdate, keyConsiderUpdate );
                }

                ERR ErrVisitFilteredSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    ERR                     err             = JET_errSuccess;
                    const CCachedBlockId&   cbid            = slotstCurrent.Cbid();
                    const QWORD             ibCachedBlock   = (QWORD)cbid.Cbno() * cbCachedBlock;
                    const COffsets          offsets         = COffsets( ibCachedBlock, ibCachedBlock - 1 + cbCachedBlock );

                    //  ignore any slot that is superceded

                    if ( slotstCurrent.FSuperceded() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore any slot that isn't dirty

                    if ( !slotstCurrent.FDirty() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore any slot that is pinned

                    if ( slotstCurrent.FPinned() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  register this write back

                    Call( CWriteBack::ErrRegister(  m_pc,
                                                    NULL,
                                                    m_pcbs,
                                                    m_ilWriteBack,
                                                    slotstCurrent,
                                                    m_pcfte,
                                                    offsets ) );

                HandleError:
                    return err;
                }

            private:

                THashedLRUKCache<I>* const                          m_pc;
                ICachedBlockSlab* const                             m_pcbs;
                CHashedLRUKCachedFileTableEntry<I>* const           m_pcfte;
                CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>& m_ilWriteBack;
        };

        //  Clean Slots Slab Visitor

        class CCleanSlotsSlabVisitor : public CFilteredSlotSlabVisitor
        {
            public:

                static ERR ErrExecute(  _In_ ICachedBlockSlab* const                                    pcbs,
                                        _In_ CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>* const  pilWriteBack )
                {
                    return CCleanSlotsSlabVisitor( pcbs, pilWriteBack ).ErrVisitSlots();
                }

            protected:

                CCleanSlotsSlabVisitor( _In_ ICachedBlockSlab* const                                    pcbs,
                                        _In_ CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>* const  pilWriteBack )
                        :   CFilteredSlotSlabVisitor( pcbs, pilWriteBack )
                {
                }

                ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                    _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                {
                    return pcbs->ErrCleanSlots( pfnConsiderUpdate, keyConsiderUpdate );
                }

                ERR ErrVisitFilteredSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    ERR err = JET_errSuccess;

                    //  mark the slot as clean

                    Call( pcbs->ErrUpdateSlot( slotNew ) );

                HandleError:
                    return err;
                }
        };

        //  Verify Destage Slab Visitor

        class CVerifyDestageSlabVisitor
        {
            public:

                static ERR ErrExecute(  _In_ THashedLRUKCache<I>* const                 pc,
                                        _In_ ICachedBlockSlab* const                    pcbs,
                                        _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte )
                {
                    return CVerifyDestageSlabVisitor( pc, pcbs, pcfte ).ErrVisitSlots();
                }

            private:

                CVerifyDestageSlabVisitor(  _In_ THashedLRUKCache<I>* const                 pc,
                                            _In_ ICachedBlockSlab* const                    pcbs,
                                            _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte )
                    :   m_pc( pc ),
                        m_pcbs( pcbs ),
                        m_pcfte( pcfte ),
                        m_errVisitSlots( JET_errSuccess )
                {
                }

                ERR ErrVisitSlots()
                {
                    ERR err = JET_errSuccess;

                    Call( m_pcbs->ErrVisitSlots( FVisitSlot_, DWORD_PTR( this ) ) );

                    Call( m_errVisitSlots );

                HandleError:
                    return err;
                }

                BOOL FVisitSlot(    _In_ const ERR                      errChunk,
                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent )
                {
                    ERR                     err             = JET_errSuccess;
                    const CCachedBlockId&   cbid            = slotstCurrent.Cbid();
                    const QWORD             ibCachedBlock   = (QWORD)cbid.Cbno() * cbCachedBlock;
                    const COffsets          offsets         = COffsets( ibCachedBlock, ibCachedBlock - 1 + cbCachedBlock );

                    //  ignore invalid slots

                    if ( !slotstCurrent.FValid() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore any slot that is superceded

                    if ( slotstCurrent.FSuperceded() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore clean slots

                    if ( !slotstCurrent.FDirty() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore slots belonging to other files

                    if ( cbid.Volumeid() != m_pcfte->Volumeid() )
                    {
                        Error( JET_errSuccess );
                    }
                    if ( cbid.Fileid() != m_pcfte->Fileid() )
                    {
                        Error( JET_errSuccess );
                    }
                    if ( cbid.Fileserial() != m_pcfte->Fileserial() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore pinned slots
                    //
                    //  NOTE:  we presume these will only be used by the pinned header

                    if ( slotstCurrent.FPinned() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  if we see a valid, dirty, unpinned slot for this file then we have failed

                    Error( m_pc->ErrBlockCacheInternalError( "VerifyDestageFailure" ) );

                    //  continue visiting slots if we didn't fail
                    //
                    //  NOTE:  we use JET_errNoCurrentRecord as a signal to stop processing slots

                HandleError:
                    if ( err != JET_errNoCurrentRecord )
                    {
                        m_errVisitSlots = m_errVisitSlots < JET_errSuccess ? m_errVisitSlots : err;
                    }
                    return err != JET_errNoCurrentRecord;
                }

                static BOOL FVisitSlot_(    _In_ const ERR                      errChunk,
                                            _In_ const CCachedBlockSlotState&   slotstAccepted,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const DWORD_PTR                keyVisitSlot )
                {
                    ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FVisitSlot_;
                    Unused( pfnVisitSlot );

                    CVerifyDestageSlabVisitor* const pvdsv = (CVerifyDestageSlabVisitor*)keyVisitSlot;
                    return pvdsv->FVisitSlot( errChunk, slotstAccepted, slotstCurrent );
                }

            private:

                THashedLRUKCache<I>* const                  m_pc;
                ICachedBlockSlab* const                     m_pcbs;
                CHashedLRUKCachedFileTableEntry<I>* const   m_pcfte;
                ERR                                         m_errVisitSlots;
        };

        //  Clean Slab Visitor

        class CCleanSlabVisitor : public CCachedBlockSlabVisitor
        {
            public:

                static ERR ErrExecute(  _In_    THashedLRUKCache<I>* const  pc, 
                                        _In_    CRequest* const             prequest,
                                        _Inout_ ICachedBlockSlab** const    ppcbs,
                                        _In_    const BOOL                  fRead,
                                        _In_    const QWORD                 cbRequested,
                                        _Out_   QWORD* const                pcbProduced,
                                        _Out_   QWORD* const                pcbWriteBackFailed )
                {
                    ERR                 err = JET_errSuccess;
                    CInspectSlabVisitor isv( *ppcbs );

                    *pcbProduced = 0;
                    *pcbWriteBackFailed = 0;

                    //  determine the amount of the slab that is part of the read cache and write cache

                    Call( isv.ErrVisitSlots() );

                    //  visit all the slots in the slab looking for at least as much available space as requested.  if
                    //  we need to evict data then do so.  if we need to perform write backs before we evict then start
                    //  those as well.

                    {
                        CCleanSlabVisitor csv( pc, prequest, ppcbs, fRead, cbRequested, isv.CbTotal(), isv.CbWriteCache(), isv.CbReadCache() );
                        Call( csv.ErrTryCleanSlots() );

                        *pcbProduced = csv.CbProduced();
                        *pcbWriteBackFailed = csv.CbWriteBackFailed();
                    }

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pcbProduced = 0;
                        *pcbWriteBackFailed = 0;
                    }
                    return err;
                }

                ~CCleanSlabVisitor()
                {
                    m_pc->ReleaseSlab( JET_errSuccess, &m_pcbsJournal );
                    ReleaseIORangeLocks();
                    CWriteBack::Release( m_ilWriteBack );
                }

            protected:

                CCleanSlabVisitor(  _In_    THashedLRUKCache<I>* const  pc,
                                    _In_    CRequest* const             prequest,
                                    _Inout_ ICachedBlockSlab** const    ppcbs,
                                    _In_    const BOOL                  fRead,
                                    _In_    const QWORD                 cbRequested,
                                    _In_    const QWORD                 cbTotal,
                                    _In_    const QWORD                 cbWriteCache,
                                    _In_    const QWORD                 cbReadCache )
                    :   CCachedBlockSlabVisitor( *ppcbs ),
                        m_pc( pc ),
                        m_prequest( prequest ),
                        m_pcbs( *ppcbs ),
                        m_fRead( fRead ),
                        m_cbRequested( cbRequested ),
                        m_cbTotal( cbTotal ),
                        m_pctWrite( max( 0, min( 100, m_pc->Pcconfig()->PctWrite() ) ) ),
                        m_cbWriteCacheMax( (QWORD)( m_cbTotal * m_pctWrite / 100 ) ),
                        m_cbReadCacheMax( m_cbTotal - m_cbWriteCacheMax ),
                        m_cbWriteCache( cbWriteCache ),
                        m_cbReadCache( cbReadCache ),
                        m_cbWriteCacheEligible( cbWriteCache > m_cbWriteCacheMax ? cbWriteCache - m_cbWriteCacheMax : 0 ),
                        m_cbReadCacheEligible( cbReadCache > m_cbReadCacheMax ? cbReadCache - m_cbReadCacheMax : 0 ),
                        m_cbSeen( 0 ),
                        m_cbWriteCacheSeen( 0 ),
                        m_cbReadCacheSeen( 0 ),
                        m_cbInvalid( 0 ),
                        m_cbIORangeLocked( 0 ),
                        m_cbWriteBackPending( 0 ),
                        m_cbWriteBackFailed( 0 ),
                        m_cbEvicted( 0 ),
                        m_cbInvalidatePending( 0 ),
                        m_pcbsJournal( NULL )
                {
                }

                ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                    _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                {
                    return pcbs->ErrEvictSlots( pfnConsiderUpdate, keyConsiderUpdate );
                }

                ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    return ErrTryCleanSlot( pcbs, slotstCurrent, slotNew );
                }

            private:

                ERR ErrTryCleanSlots()
                {
                    ERR err = JET_errSuccess;

                    //  perform any evictions required to ensure we have enough available space

                    Call( ErrVisitSlots() );

                    //  perform any write backs required to produce available space

                    Call( ErrPerformWriteBack() );

                    //  perform any invalidations required to produce available space

                    Call( ErrPerformInvalidation() );

                    //  update the affected slabs

                HandleError:
                    err = ErrAccumulateError( err, ErrUpdateSlabs() );
                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s R=0x%016I64x Clean cbRequested=%llu cbProduced=%llu (cbInvalid=%llu cbWriteBack=%llu cbEvicted=%llu cbInvalidated=%llu) err=%d",
                                            OSFormatFileId( m_pc ),
                                            QWORD( m_prequest ),
                                            m_cbRequested,
                                            CbProduced(),
                                            m_cbInvalid,
                                            m_cbWriteBackPending - m_cbWriteBackFailed,
                                            m_cbEvicted,
                                            m_cbInvalidatePending,
                                            err ) );
                    return err;
                }

                QWORD CbProduced() const
                {
                    return  m_cbInvalid +
                            ( m_cbWriteBackPending - m_cbWriteBackFailed ) + 
                            m_cbEvicted + 
                            m_cbInvalidatePending;
                }

                QWORD CbWriteBackFailed() const { return m_cbWriteBackFailed; }

                ERR ErrTryCleanSlot(    _In_ ICachedBlockSlab* const        pcbs,
                                        _In_ const CCachedBlockSlotState&   slotstCurrent,
                                        _In_ const CCachedBlockSlot&        slotNew )
                {
                    ERR                                 err                 = JET_errSuccess;
                    const CCachedBlockId&               cbid                = slotstCurrent.Cbid();
                    const QWORD                         ibCachedBlock       = (QWORD)cbid.Cbno() * cbCachedBlock;
                    const COffsets                      offsets             = COffsets( ibCachedBlock, ibCachedBlock - 1 + cbCachedBlock );
                    BOOL                                fFileNoLongerExists = fFalse;
                    CHashedLRUKCachedFileTableEntry<I>* pcfte               = NULL;
                    BOOL                                fIORangeLocked      = fFalse;
                    BOOL                                fWriteBackRequired  = fFalse;

                    //  we are visiting the slots in the slab in their least useful to most useful order

                    //  if we think we can produce enough available space without this slot then skip it

                    if ( CbProduced() >= m_cbRequested )
                    {
                        //  if we have no pending IO range locks to process then we have accomplished our goal

                        if ( !m_cbIORangeLocked )
                        {
                            Error( ErrERRCheck( JET_errNoCurrentRecord ) );
                        }

                        //  however, if we already have an IO range lock for this slot then we must still clean it to
                        //  ensure we don't have cached file blocks in mixed states

                        if ( !FExistingIORangeLock( cbid, offsets ) )
                        {
                            Error( JET_errSuccess );
                        }
                    }

                    //  if this slot is already not valid then it is already available

                    if ( !slotstCurrent.FValid() )
                    {
                        //  note the available space observed

                        m_cbInvalid += cbCachedBlock;

                        Error( JET_errSuccess );
                    }

                    //  if this slot is protected from clean due to the write caching percentage then skip it

                    if ( FProtectedFromClean( slotstCurrent ) )
                    {
                        //  however, if we already have an IO range lock for this slot then we must still clean it to
                        //  ensure we don't have cached file blocks in mixed states

                        if ( !FExistingIORangeLock( cbid, offsets ) )
                        {
                            Error( JET_errSuccess );
                        }
                    }

                    //  try to obtain the cached file handle associated with the slot for internal use

                    Call( ErrTryGetCachedFile( cbid, &fFileNoLongerExists, &pcfte ) );

                    //  try to get the IO range lock

                    Call( ErrTryGetIORangeLock( pcfte, offsets, &fIORangeLocked ) );

                    //  if the cached file still exists and this slot is dirty and not pinned and doesn't contain an
                    //  obsolete image of the data then try to write back its data to the cached file.  we will mark it
                    //  as written back after completion

                    fWriteBackRequired =    !fFileNoLongerExists &&
                                            slotstCurrent.FValid() &&
                                            slotstCurrent.FDirty() &&
                                            !slotstCurrent.FPinned() &&
                                            !slotstCurrent.FSuperceded();

                    if ( fWriteBackRequired && fIORangeLocked )
                    {
                        //  start a write back

                        Call( ErrWriteBackSlot( pcbs, slotstCurrent, pcfte, offsets ) );

                        m_cbIORangeLocked -= cbCachedBlock;
                        m_cbWriteBackPending += cbCachedBlock;

                        Error( JET_errSuccess );
                    }

                    //  if write back is not required for this slot then try to evict it

                    if ( !fWriteBackRequired )
                    {
                        BOOL fEvicted;
                        Call( ErrTryEvictSlot( pcbs, slotstCurrent, slotNew, pcfte, offsets, fIORangeLocked, &fEvicted ) );

                        //  note the available space evicted

                        m_cbIORangeLocked -= fEvicted && fIORangeLocked ? cbCachedBlock : 0;
                        m_cbEvicted += fEvicted ? cbCachedBlock : 0;

                        //  if we evicted this slot then we're done with it

                        if ( fEvicted )
                        {
                            Error( JET_errSuccess );
                        }
                    }

                    //  if the slot couldn't be written back or evicted because it belongs to a cached file that no
                    //  longer exists then we will invalidate it

                    if ( fFileNoLongerExists )
                    {
                        m_cbInvalidatePending += cbCachedBlock;

                        Error( JET_errSuccess );
                    }

                    //  if we couldn't do anything with this range locked block then it isn't potentially available

                    m_cbIORangeLocked -= fIORangeLocked ? cbCachedBlock : 0;

                HandleError:
                    m_pc->ReleaseCachedFile( &pcfte );
                    m_cbSeen += cbCachedBlock;
                    m_cbReadCacheSeen += slotstCurrent.FEverDirty() ? 0 : cbCachedBlock;
                    m_cbWriteCacheSeen += slotstCurrent.FEverDirty() ? cbCachedBlock : 0;
                    return err;
                }

                BOOL FProtectedFromClean( _In_ const CCachedBlockSlotState& slotstCurrent )
                {
                    //  we are cleaning to cache a read

                    if ( m_fRead )
                    {
                        //  if this slot contains excess write cache then we can clean it

                        if ( m_cbWriteCacheSeen < m_cbWriteCacheEligible )
                        {
                            return fFalse;
                        }

                        //  if protecting this slot from clean would cause us to fail to clean enough space then we
                        //  must clean it

                        if ( m_cbReadCacheSeen < m_cbReadCache )
                        {
                            return fFalse;
                        }
                    }

                    //  we are cleaning to cache a write

                    if ( !m_fRead )
                    {
                        //  if this slot contains excess read cache then we can clean it

                        if ( m_cbReadCacheSeen < m_cbReadCacheEligible )
                        {
                            return fFalse;
                        }

                        //  if protecting this slot from clean would cause us to fail to clean enough space then we
                        //  must clean it

                        if ( m_cbWriteCacheSeen < m_cbWriteCache )
                        {
                            return fFalse;
                        }
                    }

                    //  this slot is protected from clean to preserve our write caching percentage

                    return fTrue;
                }

                ERR ErrWriteBackSlot(   _In_ ICachedBlockSlab* const                    pcbs,
                                        _In_ const CCachedBlockSlotState&               slotstCurrent,
                                        _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_ const COffsets                             offsets )
                {
                    ERR err = JET_errSuccess;

                    //  emit telemetry

                    m_pc->ReportWrite( pcfte, offsets.IbStart(), fTrue );

                    //  register the write back to be performed later

                    Call( CWriteBack::ErrRegister( m_pc, m_prequest, m_pcbs, m_ilWriteBack, slotstCurrent, pcfte, offsets ) );

                HandleError:
                    return err;
                }

                ERR ErrTryEvictSlot(    _In_        ICachedBlockSlab* const                     pcbs,
                                        _In_        const CCachedBlockSlotState&                slotstCurrent,
                                        _In_        const CCachedBlockSlot&                     slotNew,
                                        _In_opt_    CHashedLRUKCachedFileTableEntry<I>* const   pcfte,
                                        _In_        const COffsets                              offsets,
                                        _In_        const BOOL                                  fIORangeLocked,
                                        _Out_       BOOL* const                                 pfEvicted )
                {
                    ERR     err         = JET_errSuccess;
                    BOOL    fEvicted    = fFalse;

                    *pfEvicted = fFalse;

                    //  if the slot is already evicted then bail

                    if ( !slotstCurrent.FValid() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  if the slot is not eligible for eviction then bail

                    if ( slotNew.FValid() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  if the cached file exists then we must get an IO range lock for the data to evict unless it is
                    //  superceded.  if the cached file doesn't exist then we are free to evict the data anytime

                    if ( pcfte && !fIORangeLocked && !slotstCurrent.FSuperceded() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  evict the slot

                    Call( m_pc->ErrEvictSlot( pcbs, slotNew, &m_pcbsJournal ) );

                    //  emit telemetry

                    m_pc->ReportEvict( pcfte, offsets.IbStart(), fTrue );

                    //  report that we evicted the data

                    fEvicted = fTrue;

                    //  return our results

                    *pfEvicted = fEvicted;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pfEvicted = fFalse;
                    }
                    return err;
                }

                ERR ErrPerformWriteBack()
                {
                    ERR err = JET_errSuccess;

                    //  if there is nothing to write back then we're done

                    if ( m_ilWriteBack.FEmpty() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  request all the reads we need to complete the write backs

                    for ( CWriteBack* pwb = m_ilWriteBack.PrevMost(); pwb; pwb = m_ilWriteBack.Next( pwb ) )
                    {
                        pwb->StartRead();
                    }

                    Call( m_pc->PffCaching()->ErrIOIssue() );

                    //  write them back as they complete

                    for ( CWriteBack* pwb = m_ilWriteBack.PrevMost(); pwb; pwb = m_ilWriteBack.Next( pwb ) )
                    {
                        pwb->StartWrite();

                        if ( pwb == m_ilWriteBack.NextMost() || pwb->Pcfte() != m_ilWriteBack.Next( pwb )->Pcfte() )
                        {
                            //  NOTE:  we use iomCacheWriteThrough rather than iomCacheWriteBack because we already
                            //  protect against conflicting IO with our IO range locks and because the write back
                            //  mechanism doesn't work for sync write back during a sync IO

                            Call( pwb->Pcfte()->Pff()->ErrIssue( iomCacheWriteThrough ) );
                        }
                    }

                    //  wait for the writes to succeed and then flush the cached file and record the result

                    for ( CWriteBack* pwb = m_ilWriteBack.PrevMost(); pwb; pwb = m_ilWriteBack.Next( pwb ) )
                    {
                        pwb->WaitForWriteCompletion();

                        if ( pwb == m_ilWriteBack.NextMost() || pwb->Pcfte() != m_ilWriteBack.Next( pwb )->Pcfte() )
                        {
                            const ERR errFlush = pwb->Pcfte()->Pff()->ErrFlush( iofrBlockCache, iomCacheWriteThrough );
                            for (   CWriteBack* pwbT = pwb;
                                    pwbT && pwbT->Pcfte() == pwb->Pcfte();
                                    pwbT = m_ilWriteBack.Prev( pwbT ) )
                            {
                                pwbT->CompleteFlush( errFlush );
                            }
                        }
                    }

                    //  mark all the slots we just successfully wrote back as clean

                    Call( CCleanSlotsSlabVisitor::ErrExecute( m_pcbs, &m_ilWriteBack ) );

                    //  evict all the slots that we just marked as clean

                    Call( CEvictCleanedSlotsSlabVisitor::ErrExecute( m_pc, m_pcbs, &m_ilWriteBack, &m_pcbsJournal ) );

                    //  count how many write backs failed

                    for ( CWriteBack* pwb = m_ilWriteBack.PrevMost(); pwb; pwb = m_ilWriteBack.Next( pwb ) )
                    {
                        if ( pwb->ErrComplete() < JET_errSuccess )
                        {
                            m_cbWriteBackFailed += cbCachedBlock;
                        }
                    }

                HandleError:
                    return err;
                }

                ERR ErrPerformInvalidation()
                {
                    ERR err = JET_errSuccess;

                    //  if there is nothing to invalidate then we're done

                    if ( !m_cbInvalidatePending )
                    {
                        Error( JET_errSuccess );
                    }

                    //  invalidate all the slots that contain data for deleted cached files

                    Call( CInvalidateAbandonedSlotsSlabVisitor::ErrExecute( m_pc, m_pcbs, &m_pcbsJournal ) );

                HandleError:
                    return err;
                }

                ERR ErrUpdateSlabs()
                {
                    ERR err = JET_errSuccess;

                    //  if we moved clusters due to evicting cached data then we must perform our changes in one atomic
                    //  update

                    if ( m_pcbsJournal && m_pcbsJournal->FUpdated() )
                    {
                        //  update both slabs atomically

                        Call( m_pc->ErrUpdateSlabs( &m_pcbs, &m_pcbsJournal ) );
                    }

                    //  release our journal slab

                HandleError:
                    m_pc->ReleaseSlab( err, &m_pcbsJournal );
                    return err;
                }

                ERR ErrTryGetCachedFile(    _In_    const CCachedBlockId&                       cbid,
                                            _Out_   BOOL* const                                 pfFileNoLongerExists,
                                            _Out_   CHashedLRUKCachedFileTableEntry<I>** const  ppcfte )
                {
                    return ErrTryGetCachedFile( m_pc, cbid, pfFileNoLongerExists, ppcfte );
                }

                static ERR ErrTryGetCachedFile( _In_    THashedLRUKCache<I>* const                  pc,
                                                _In_    const CCachedBlockId&                       cbid,
                                                _Out_   BOOL* const                                 pfFileNoLongerExists, 
                                                _Out_   CHashedLRUKCachedFileTableEntry<I>** const  ppcfte )
                {
                    ERR                                 err             = JET_errSuccess;
                    CHashedLRUKCachedFileTableEntry<I>* pcfte           = NULL;

                    *ppcfte = NULL;
                    *pfFileNoLongerExists = fFalse;

                    //  try to open the cached file for this cached block id

                    err = pc->ErrGetCachedFile( cbid.Volumeid(),
                                                cbid.Fileid(),
                                                cbid.Fileserial(),
                                                fTrue,
                                                &pcfte );

                    switch ( err )
                    {
                        //  if the file id was invalid then that something bad happened

                        case JET_errInvalidParameter:
                            Error( pc->ErrBlockCacheInternalError( "CleanSlabVisitorInvalidFileId" ) );

                        //  if the file positively no longer exists then indicate that
                        //
                        //  NOTE:  this criteria is critical because this enables cached data to be invalidated
                        //  NOTE:  currently a failure to find a volume even temporarily results in this error

                        case JET_errFileNotFound:
                            *pfFileNoLongerExists = fTrue;
                            Error( JET_errSuccess );

                        //  if we cannot access the file then it exists but we cannot access it right now

                        case JET_errFileAccessDenied:
                        case JET_errFileSystemCorruption:
                        case JET_errDiskIO:
                        case JET_errDiskReadVerificationFailure:
                            Error( JET_errSuccess );

                        //  fail on any other error

                        default:
                            Call( err );
                    }

                    //  return the cached file

                    *ppcfte = pcfte;
                    pcfte = NULL;

                HandleError:
                    pc->ReleaseCachedFile( &pcfte );
                    if ( err < JET_errSuccess )
                    {
                        if ( *ppcfte )
                        {
                            pc->ReleaseCachedFile( ppcfte );
                        }
                        *pfFileNoLongerExists = fFalse;
                    }
                    return err;
                }

            private:

                class CIORangeLock : public CHashedLRUKCachedFileTableEntry<I>::CIORangeLock
                {
                    public:

                        CIORangeLock(   _In_ THashedLRUKCache<I>* const                 pc,
                                        _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_ const COffsets                             offsets )
                            :   CHashedLRUKCachedFileTableEntry<I>::CIORangeLock( pcfte, offsets ),
                                m_pc( pc )
                        {
                            pcfte->AddRef();
                        }

                        ~CIORangeLock()
                        {
                            CHashedLRUKCachedFileTableEntry<I>* pcfteT = Pcfte();
                            m_pc->ReleaseCachedFile( &pcfteT );
                        }

                        static SIZE_T OffsetOfILE() { return OffsetOf( CIORangeLock, m_ile ); }

                    private:

                        typename CInvasiveList<CIORangeLock, OffsetOfILE>::CElement m_ile;
                        THashedLRUKCache<I>* const                                  m_pc;
                };

                ERR ErrTryGetIORangeLock(   _In_    CHashedLRUKCachedFileTableEntry<I>* const   pcfte,
                                            _In_    const COffsets                              offsetsSlot,
                                            _Out_   BOOL* const                                 pfIORangeLocked )
                {
                    ERR             err             = JET_errSuccess;
                    CIORangeLock*   piorlExisting   = NULL;
                    CIORangeLock*   piorlNew        = NULL;

                    *pfIORangeLocked = fFalse;

                    //  if we don't have the file open then we can't get the io range lock

                    if ( !pcfte )
                    {
                        Error( JET_errSuccess );
                    }

                    //  determine if we already have an IO range lock that covers the cached file block containing this
                    //  cache block

                    piorlExisting = PiorlTryGetExistingIORangeLock( pcfte, offsetsSlot );

                    //  try to get an IO range lock for the offsets if we don't already have it
                    //
                    //  NOTE:  we cannot wait here or risk a deadlock with another thread that owns this IO range
                    //  lock but is waiting to acquire this slab.  because we cannot wait we do not have to worry about
                    //  the order in which the IO range locks are acquired
                    //
                    //  NOTE:  we can also see the case where this thread already owns this IO range lock.  in that
                    //  case it is good to avoid write back / evict of the offsets we are accessing.  the cache
                    //  hash function should prevent us from blocking sufficient available space production for
                    //  this case

                    if ( !piorlExisting )
                    {
                        //  expand the IO range lock to cover the cached file block containing the offsets of the
                        //  cached file represented by this slot

                        const QWORD cbBlockSize     = pcfte->CbBlockSize();
                        const QWORD ibStart         = rounddn( offsetsSlot.IbStart(), cbBlockSize );
                        COffsets    offsetsBlock( ibStart, ibStart + ( cbBlockSize - 1 ) );

                        Alloc( piorlNew = new CIORangeLock( m_pc, pcfte, offsetsBlock ) );

                        if ( pcfte->FTryRequestIORangeLock( piorlNew, fFalse ) )
                        {
                            OSTrace(    JET_tracetagBlockCacheOperations,
                                        OSFormat(   "C=%s R=0x%016I64x Clean IORangeLock F=%s ib=%llu cb=%llu Grant",
                                                    OSFormatFileId( m_pc ),
                                                    QWORD( m_prequest ),
                                                    OSFormatFileId( piorlNew->Pcfte()->Pff()),
                                                    piorlNew->Offsets().IbStart(),
                                                    piorlNew->Offsets().Cb() ) );

                            m_cbIORangeLocked += piorlNew->Offsets().Cb();

                            m_ilIORangeLock.InsertAsNextMost( piorlNew );
                            piorlExisting = piorlNew;
                            piorlNew = NULL;
                        }
                        else
                        {
                            OSTrace(    JET_tracetagBlockCacheOperations,
                                        OSFormat(   "C=%s R=0x%016I64x Clean IORangeLock F=%s ib=%llu cb=%llu not available",
                                                    OSFormatFileId( m_pc ),
                                                    QWORD( m_prequest ),
                                                    OSFormatFileId( piorlNew->Pcfte()->Pff()),
                                                    piorlNew->Offsets().IbStart(),
                                                    piorlNew->Offsets().Cb() ) );
                        }
                    }

                    //  return if we acquired the IO range lock

                    *pfIORangeLocked = piorlExisting != NULL;

                HandleError:
                    delete piorlNew;
                    if ( err < JET_errSuccess )
                    {
                        *pfIORangeLocked = fFalse;
                    }
                    return err;
                }

                BOOL FIORangeLocked(    _In_ CIORangeLock* const    piorlExisting,
                                        _In_ const VolumeId         volumeid,
                                        _In_ const FileId           fileid,
                                        _In_ const FileSerial       fileserial,
                                        _In_ const COffsets         offsets )
                {
                    //  we must be referring to the same cached file

                    if ( piorlExisting->Pcfte()->Volumeid() != volumeid )
                    {
                        return fFalse;
                    }
                    if ( piorlExisting->Pcfte()->Fileid() != fileid )
                    {
                        return fFalse;
                    }
                    if ( piorlExisting->Pcfte()->Fileserial() != fileserial )
                    {
                        return fFalse;
                    }

                    //  the IO range lock offsets must contain the cached file offsets

                    if ( !piorlExisting->Offsets().FContains( offsets ) )
                    {
                        return fFalse;
                    }

                    return fTrue;
                }

                CIORangeLock* PiorlTryGetExistingIORangeLock(   _In_ const VolumeId     volumeid,
                                                                _In_ const FileId       fileid,
                                                                _In_ const FileSerial   fileserial,
                                                                _In_ const COffsets     offsets )
                {
                    CIORangeLock* piorlExisting = NULL;

                    for (   piorlExisting = m_ilIORangeLock.PrevMost();
                            piorlExisting && !FIORangeLocked( piorlExisting, volumeid, fileid, fileserial, offsets );
                            piorlExisting = m_ilIORangeLock.Next( piorlExisting ) )
                    {
                    }

                    return piorlExisting;
                }

                CIORangeLock* PiorlTryGetExistingIORangeLock(   _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                                                _In_ const COffsets                             offsets )
                {
                    return PiorlTryGetExistingIORangeLock(  pcfte->Volumeid(),
                                                            pcfte->Fileid(),
                                                            pcfte->Fileserial(),
                                                            offsets );
                }

                BOOL FExistingIORangeLock( _In_ const CCachedBlockId& cbid, _In_ const COffsets offsets )
                {
                    return PiorlTryGetExistingIORangeLock(  cbid.Volumeid(),
                                                            cbid.Fileid(),
                                                            cbid.Fileserial(),
                                                            offsets ) != NULL;
                }

                void ReleaseIORangeLocks()
                {
                    while ( CIORangeLock* const piorl = m_ilIORangeLock.PrevMost() )
                    {
                        OSTrace(    JET_tracetagBlockCacheOperations,
                                    OSFormat(   "C=%s R=0x%016I64x Clean IORangeLock F=%s ib=%llu cb=%llu Release",
                                                OSFormatFileId( m_pc ),
                                                QWORD( m_prequest ),
                                                OSFormatFileId( piorl->Pcfte()->Pff()),
                                                piorl->Offsets().IbStart(),
                                                piorl->Offsets().Cb() ) );

                        m_ilIORangeLock.Remove( piorl );
                        piorl->Release();
                        delete piorl;
                    }
                }

            private:

                class CInspectSlabVisitor
                {
                    public:

                        CInspectSlabVisitor( _In_ ICachedBlockSlab* const pcbs )
                            :   m_pcbs( pcbs ),
                                m_cbTotal( 0 ),
                                m_cbReadCache( 0 ),
                                m_cbWriteCache( 0 )
                        {
                        }

                        QWORD CbTotal() const { return m_cbTotal; }
                        QWORD CbReadCache() const { return m_cbReadCache; }
                        QWORD CbWriteCache() const { return m_cbWriteCache; }

                        ERR ErrVisitSlots()
                        {
                            return m_pcbs->ErrVisitSlots( FVisitSlot_, DWORD_PTR( this ) );
                        }

                    private:

                        BOOL FVisitSlot(    _In_ const ERR                      errChunk,
                                            _In_ const CCachedBlockSlotState&   slotstAccepted,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent )
                        {
                            m_cbTotal += cbCachedBlock;
                            m_cbReadCache += slotstCurrent.FValid() && !slotstCurrent.FEverDirty() ? cbCachedBlock : 0;
                            m_cbWriteCache += slotstCurrent.FValid() && slotstCurrent.FEverDirty() ? cbCachedBlock : 0;

                            return fTrue;
                        }

                        static BOOL FVisitSlot_(    _In_ const ERR                      errChunk,
                                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                                    _In_ const DWORD_PTR                keyVisitSlot )
                        {
                            ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FVisitSlot_;
                            Unused( pfnVisitSlot );

                            CInspectSlabVisitor* const pisv = (CInspectSlabVisitor*)keyVisitSlot;
                            return pisv->FVisitSlot( errChunk, slotstAccepted, slotstCurrent );
                        }

                    private:

                        ICachedBlockSlab* const m_pcbs;
                        QWORD                   m_cbTotal;
                        QWORD                   m_cbReadCache;
                        QWORD                   m_cbWriteCache;
                };

                class CEvictCleanedSlotsSlabVisitor : public CFilteredSlotSlabVisitor
                {
                    public:

                        static ERR ErrExecute(  _In_    THashedLRUKCache<I>* const                                  pc,
                                                _In_    ICachedBlockSlab* const                                     pcbs,
                                                _In_    CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>* const   pilWriteBack,
                                                _Inout_ ICachedBlockSlab** const                                    ppcbsJournal )
                        {
                            return CEvictCleanedSlotsSlabVisitor( pc, pcbs, pilWriteBack, ppcbsJournal ).ErrVisitSlots();
                        }

                    protected:

                        CEvictCleanedSlotsSlabVisitor(  _In_    THashedLRUKCache<I>* const                                  pc,
                                                        _In_    ICachedBlockSlab* const                                     pcbs,
                                                        _In_    CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>* const   pilWriteBack,
                                                        _Inout_ ICachedBlockSlab** const                                    ppcbsJournal )
                                :   CFilteredSlotSlabVisitor( pcbs, pilWriteBack ),
                                    m_pc( pc ),
                                    m_ppcbsJournal( ppcbsJournal )
                        {
                        }

                        ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                            _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                            _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                        {
                            return pcbs->ErrEvictSlots( pfnConsiderUpdate, keyConsiderUpdate );
                        }

                        ERR ErrVisitFilteredSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                                    _In_ const CCachedBlockSlot&        slotNew ) override
                        {
                            ERR err = JET_errSuccess;

                            //  ignore any slot that is invalid

                            if ( !slotstCurrent.FValid() )
                            {
                                Error( JET_errSuccess );
                            }

                            //  ignore any slot that is dirty

                            if ( slotstCurrent.FDirty() )
                            {
                                Error( JET_errSuccess );
                            }

                            //  evict the slot

                            Call( m_pc->ErrEvictSlot( pcbs, slotNew, m_ppcbsJournal ) );

                        HandleError:
                            return err;
                        }

                    private:

                        THashedLRUKCache<I>* const  m_pc;
                        ICachedBlockSlab** const    m_ppcbsJournal;
                };

                class CInvalidateAbandonedSlotsSlabVisitor : public CCachedBlockSlabVisitor
                {
                    public:

                        static ERR ErrExecute(  _In_    THashedLRUKCache<I>* const  pc,
                                                _In_    ICachedBlockSlab* const     pcbs,
                                                _Inout_ ICachedBlockSlab** const    ppcbsJournal )
                        {
                            return CInvalidateAbandonedSlotsSlabVisitor( pc, pcbs, ppcbsJournal ).ErrVisitSlots();
                        }

                    protected:

                        CInvalidateAbandonedSlotsSlabVisitor(   _In_    THashedLRUKCache<I>* const  pc,
                                                                _In_    ICachedBlockSlab* const     pcbs,
                                                                _Inout_ ICachedBlockSlab** const    ppcbsJournal )
                                :   CCachedBlockSlabVisitor( pcbs ),
                                    m_pc( pc ),
                                    m_ppcbsJournal( ppcbsJournal )
                        {
                        }

                        ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                            _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                            _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                        {
                            return pcbs->ErrInvalidateSlots( pfnConsiderUpdate, keyConsiderUpdate );
                        }

                        ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const CCachedBlockSlot&        slotNew ) override
                        {
                            ERR                                 err                 = JET_errSuccess;
                            BOOL                                fFileNoLongerExists = fFalse;
                            CHashedLRUKCachedFileTableEntry<I>* pcfte               = NULL;

                            //  ignore any slot that is invalid

                            if ( !slotstCurrent.FValid() )
                            {
                                Error( JET_errSuccess );
                            }

                            //  ignore any slot containing data for a valid cached file

                            Call( ErrTryGetCachedFile( m_pc, slotstCurrent.Cbid(), &fFileNoLongerExists, &pcfte ) );

                            if ( !fFileNoLongerExists )
                            {
                                Error( JET_errSuccess );
                            }

                            //  invalidate the slot

                            Call( m_pc->ErrInvalidateSlot( pcbs, slotNew, m_ppcbsJournal ) );

                        HandleError:
                            m_pc->ReleaseCachedFile( &pcfte );
                            return err;
                        }

                    private:

                        THashedLRUKCache<I>* const  m_pc;
                        ICachedBlockSlab** const    m_ppcbsJournal;
                };

            private:

                THashedLRUKCache<I>* const                              m_pc;
                CRequest* const                                         m_prequest;
                ICachedBlockSlab*&                                      m_pcbs;
                const BOOL                                              m_fRead;
                const QWORD                                             m_cbRequested;
                const QWORD                                             m_cbTotal;
                const double                                            m_pctWrite;
                const QWORD                                             m_cbWriteCacheMax;
                const QWORD                                             m_cbReadCacheMax;
                const QWORD                                             m_cbWriteCache;
                const QWORD                                             m_cbReadCache;
                const QWORD                                             m_cbWriteCacheEligible;
                const QWORD                                             m_cbReadCacheEligible;
                QWORD                                                   m_cbSeen;
                QWORD                                                   m_cbWriteCacheSeen;
                QWORD                                                   m_cbReadCacheSeen;
                QWORD                                                   m_cbInvalid;
                QWORD                                                   m_cbIORangeLocked;
                QWORD                                                   m_cbWriteBackPending;
                QWORD                                                   m_cbWriteBackFailed;
                QWORD                                                   m_cbEvicted;
                QWORD                                                   m_cbInvalidatePending;
                ICachedBlockSlab*                                       m_pcbsJournal;
                CInvasiveList<CIORangeLock, CIORangeLock::OffsetOfILE>  m_ilIORangeLock;
                CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>      m_ilWriteBack;
        };

        class CMarkUsedJournalClustersAsFreeSlabVisitor : public CCachedBlockSlabVisitor
        {
            public:

                static ERR ErrExecute( _In_ ICachedBlockSlab* const pcbs )
                {
                    return CMarkUsedJournalClustersAsFreeSlabVisitor( pcbs ).ErrVisitSlots();
                }

            protected:

                CMarkUsedJournalClustersAsFreeSlabVisitor( _In_ ICachedBlockSlab* const pcbs )
                        :   CCachedBlockSlabVisitor( pcbs )
                {
                }

                ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                    _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                {
                    return pcbs->ErrEvictSlots( pfnConsiderUpdate, keyConsiderUpdate );
                }

                ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    ERR err = JET_errSuccess;

                    //  ignore any slot that is invalid

                    if ( !slotstCurrent.FValid() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ignore any slot that is dirty

                    if ( slotstCurrent.FDirty() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  mark this journal cluster as free by evicting its slot

                    Call( pcbs->ErrUpdateSlot( slotNew ) );

                HandleError:
                    return err;
                }
        };

        class CMarkAllocatedJournalClustersAsUsedSlabVisitor : public CCachedBlockSlabVisitor
        {
            public:

                static ERR ErrExecute( _In_ ICachedBlockSlab* const pcbs )
                {
                    return CMarkAllocatedJournalClustersAsUsedSlabVisitor( pcbs ).ErrVisitSlots();
                }

            protected:

                CMarkAllocatedJournalClustersAsUsedSlabVisitor( _In_ ICachedBlockSlab* const pcbs )
                        :   CCachedBlockSlabVisitor( pcbs )
                {
                }

                ERR ErrVisitSlots_( _In_ ICachedBlockSlab* const                    pcbs,
                                    _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                    _In_ const DWORD_PTR                            keyConsiderUpdate ) override
                {
                    return pcbs->ErrCleanSlots( pfnConsiderUpdate, keyConsiderUpdate );
                }

                ERR ErrVisitSlot_(  _In_ ICachedBlockSlab* const        pcbs,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew ) override
                {
                    ERR err = JET_errSuccess;

                    //  ignore any slot that isn't dirty

                    if ( !slotstCurrent.FDirty() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  mark this allocated journal cluster as used by marking the slot as clean

                    Call( pcbs->ErrUpdateSlot( slotNew ) );

                HandleError:
                    return err;
                }
        };

        class CQueuedJournalEntry
        {
            public:

                CQueuedJournalEntry(    _In_    const JournalPosition       jpos,
                                        _In_    const JournalPosition       jposEnd, 
                                        _Inout_ const CJournalEntry** const ppje )
                    :   m_jpos( jpos ),
                        m_jposEnd( jposEnd ),
                        m_pje( *ppje )
                {
                    *ppje = NULL;
                }

                ~CQueuedJournalEntry()
                {
                    delete m_pje;
                }

                JournalPosition Jpos() const { return m_jpos; }
                JournalPosition JposEnd() const { return m_jposEnd; }
                const CJournalEntry* Pje() const { return m_pje; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CQueuedJournalEntry, m_ile ); }

            private:

                typename CInvasiveList<CQueuedJournalEntry, OffsetOfILE>::CElement  m_ile;
                const JournalPosition                                               m_jpos;
                const JournalPosition                                               m_jposEnd;
                const CJournalEntry* const                                          m_pje;
        };

        //  Journal wrapper
        //
        //  Guarantees that whenever anyone calls ErrAppendEntry:
        //  -  if the journal is full then we recover from that condition and retry
        //  -  the jposEnd of the last append is tracked
        // 
        //  Guarantees that whenever anyone calls ErrFlush:
        //  -  any previously written clusters are written first (write barrier)
        //  -  flush will actually advance the durable and durable for writeback pointers
        //  -  the caching file is flushed
        //
        //  Guarantees that whenever anyone calls ErrTruncate:
        //  -  any previously written state in the caching file is durable
        //  -  we will silently ignore attempts to rewind the replay pointer

        class CJournalWrapper : public IJournal
        {
            public:  //  specialized API

                CJournalWrapper( _In_ THashedLRUKCache<I>* const pc, _Inout_ IJournal** const ppj )
                    :   m_pc( pc ),
                        m_pjInner( *ppj ),
                        m_errAppend( JET_errSuccess ),
                        m_jposLastEnd( jposInvalid ),
                        m_semTruncate( CSyncBasicInfo( "THashedLRUKCache<I>::CJournalWrapper::m_semTruncate" ) )
                {
                    *ppj = NULL;
                    m_semTruncate.Release();
                }

                ~CJournalWrapper()
                {
                    m_semTruncate.Acquire();
                    delete m_pjInner;
                }

            public:  //  IJournal

                ERR ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                        _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                        _Out_opt_ JournalPosition* const    pjposDurable,
                                        _Out_opt_ JournalPosition* const    pjposAppend,
                                        _Out_opt_ JournalPosition* const    pjposFull ) override
                {
                    return m_pjInner->ErrGetProperties( pjposReplay, 
                                                        pjposDurableForWriteBack, 
                                                        pjposDurable, 
                                                        pjposAppend, 
                                                        pjposFull );
                }

                ERR ErrVisitEntries(    _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                        _In_ const DWORD_PTR                keyVisitEntry ) override
                {
                    return m_pjInner->ErrVisitEntries( pfnVisitEntry, keyVisitEntry );
                }

                ERR ErrRepair(  _In_    const JournalPosition   jposInvalidate,
                                _Out_   JournalPosition* const  pjposInvalidated ) override
                {
                    return m_pjInner->ErrRepair( jposInvalidate, pjposInvalidated );
                }

                ERR ErrAppendEntry( _In_                const size_t            cjb,
                                    _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                    _Out_               JournalPosition* const  pjpos,
                                    _Out_               JournalPosition* const  pjposEnd ) override
                {
                    return ErrAppendEntryInternal( cjb, rgjb, pjpos, pjposEnd );
                }

                ERR ErrFlush() override
                {
                    ERR             err                     = JET_errSuccess;
                    JournalPosition jposDurableForWriteBack = jposInvalid;
                    JournalPosition jposDurable             = jposInvalid;
                    JournalPosition jposLastEnd             = jposInvalid;

                    //  if this flush would not advance the durable for write back or durable pointers then ignore it

                    Call( m_pjInner->ErrGetProperties( NULL, &jposDurableForWriteBack, &jposDurable, NULL, NULL ) );

                    jposLastEnd = m_jposLastEnd;

                    if ( jposDurableForWriteBack >= jposDurable && jposDurable >= jposLastEnd )
                    {
                        OSTrace(    JET_tracetagBlockCacheOperations,
                                    OSFormat(   "C=%s Flush Ignored (jposDurableForWriteback=0x%016I64x, jposDurable=0x%016I64x, jposLastEnd=0x%016I64x)",
                                                OSFormatFileId( m_pc ),
                                                QWORD( jposDurableForWriteBack ),
                                                QWORD( jposDurable ),
                                                QWORD( jposLastEnd ) ) );

                        Error( JET_errSuccess );
                    }

                    //  ensure that any previously requested cluster writes are completed before flushing the journal.
                    //  this write barrier guarantees that journal entries that refer to clusters will only point to
                    //  clusters that have been acked as flushed.  this is required to successfully find the end of
                    //  the journal on start.  most specifically, any journal entry in a journal segment that is
                    //  indicated as durable via a persistent durable pointer (i.e. is <= jposDurableForWriteBack) must
                    //  only reference clusters that are also durable

                    Call( m_pc->ErrFlushClusters() );

                    //  force the journal to flush

                    Call( ErrForceFlush() );

                HandleError:
                    return err;
                }

                ERR ErrTruncate( _In_ const JournalPosition jposReplay ) override
                {
                    ERR             err                 = JET_errSuccess;
                    BOOL            fRelease            = fFalse;
                    JournalPosition jposReplayCurrent   = jposInvalid;

                    //  serialize advancements of the replay pointer

                    m_semTruncate.Acquire();
                    fRelease = fTrue;

                    //  get the current replay pointer

                    Call( m_pjInner->ErrGetProperties( &jposReplayCurrent, NULL, NULL, NULL, NULL ) );

                    //  the replay pointer can only go forwards

                    if ( jposReplayCurrent > jposReplay )
                    {
                        Error( JET_errSuccess );
                    }

                    //  ensure that the cache may advance to this replay pointer

                    Call( m_pc->ErrVerifyTruncate( jposReplay ) );

                    //  ensure that all state referred to by the journal entries we are about to truncate is durable

                    Call( m_pc->PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );

                    //  truncate the journal

                    Call( m_pjInner->ErrTruncate( jposReplay ) );

                HandleError:
                    if ( fRelease )
                    {
                        m_semTruncate.Release();
                    }
                    return err;
                }

            private:

                ERR ErrAppendEntryInternal( _In_                const size_t            cjb,
                                            _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                            _Out_               JournalPosition* const  pjpos,
                                            _Out_               JournalPosition* const  pjposEnd )
                {
                    ERR                     err     = JET_errSuccess;
                    CMeteredSection::Group  group   = CMeteredSection::groupInvalidNil;

                    *pjpos = jposInvalid;
                    *pjposEnd = jposInvalid;

                    //  retry the append until we succeed

                    while ( *pjpos == jposInvalid )
                    {
                        //  start our append attempt

                        Call( ErrStartAppendEntryAttempt( &group ) );

                        //  attempt to append to the journal

                        err = m_pjInner->ErrAppendEntry( cjb, rgjb, pjpos, pjposEnd );
                        Call( err == JET_errDiskFull ? JET_errSuccess : err );

                        //  if we failed because the journal is full then handle that

                        if ( *pjpos == jposInvalid )
                        {
                            HandleJournalFull( &group );
                        }
                    }

                    AtomicCompareExchange( (QWORD*)&m_jposLastEnd, (QWORD)jposInvalid, (QWORD)*pjposEnd );
                    AtomicExchangeMax( (QWORD*)&m_jposLastEnd, (QWORD)*pjposEnd );

                HandleError:
                    EndAppendEntryAttempt( &group );
                    if ( err < JET_errSuccess )
                    {
                        *pjpos = jposInvalid;
                        *pjposEnd = jposInvalid;
                    }
                    return err;
                }

                ERR ErrStartAppendEntryAttempt( _Out_ CMeteredSection::Group* const pgroup )
                {
                    ERR                     err     = JET_errSuccess;
                    CMeteredSection::Group  group   = CMeteredSection::groupInvalidNil;

                    *pgroup = CMeteredSection::groupInvalidNil;

                    //  start our state access attempt

                    Call( m_pc->ErrStartStateAccessAttempt( &group ) );

                    //  make sure we don't have a fatal error

                    Call( m_errAppend );

                    //  return our result

                    *pgroup = group;
                    group = CMeteredSection::groupInvalidNil;

                HandleError:
                    EndAppendEntryAttempt( &group );
                    if ( err < JET_errSuccess )
                    {
                        EndAppendEntryAttempt( pgroup );
                    }
                    return err;
                }

                void EndAppendEntryAttempt( _Inout_ CMeteredSection::Group* const pgroup )
                {
                    m_pc->EndStateAccessAttempt( pgroup );
                }

                void HandleJournalFull( _Inout_ CMeteredSection::Group* const pgroup )
                {
                    //  quiesce state access to setup for handling journal full

                    m_pc->QuiesceStateAccess( HandleJournalFull_, DWORD_PTR( this ) );

                    //  end our current append entry attempt

                    EndAppendEntryAttempt( pgroup );
                }

                static void HandleJournalFull_( _In_ const DWORD_PTR dwCompletionKey )
                {
                    PfnStateAccessQuiesced pfnStateAccessQuiesced = HandleJournalFull_;
                    Unused( pfnStateAccessQuiesced );

                    CJournalWrapper* const pj = (CJournalWrapper*)dwCompletionKey;
                    pj->HandleJournalFull();
                }

                void HandleJournalFull()
                {
                    //  if we are not already in an error state then handle journal full

                    if ( m_errAppend >= JET_errSuccess )
                    {
                        m_errAppend = ErrHandleJournalFull();
                    }

                    //  resume state access

                    m_pc->ResumeStateAccess();
                }
                
                ERR ErrHandleJournalFull()
                {
                    ERR                 err             = JET_errSuccess;
                    JournalPosition     jposAppend      = jposInvalid;
                    JournalPosition     jposFull        = jposInvalid;
                    QWORD               cbAvail         = 0;
                    CJournalBuffer      rgjbEmpty[]     = { { 0, NULL }, };
                    JournalPosition     jposEmpty       = jposInvalid;
                    JournalPosition     jposEmptyEnd    = jposInvalid;
                    JournalPosition     jposReplayNew   = jposInvalid;

                    m_pc->BlockCacheNotableEvent( "HandleJournalFull" );

                    //  get the amount of space left in the journal

                    Call( m_pjInner->ErrGetProperties( NULL, NULL, NULL, &jposAppend, &jposFull ) );
                    cbAvail = ( rounddn( (QWORD)jposFull, cbJournalSegment ) -
                                rounddn( (QWORD)jposAppend, cbJournalSegment ) -
                                cbJournalSegment );

                    //  perform the needed steps to recover from the full journal

                    if ( m_jposLastEnd != jposInvalid || cbAvail >= cbJournalFull )
                    {
                        //  ensure that any previously requested cluster writes are completed

                        Call( m_pc->ErrFlushClusters() );

                        //  advance the durable pointer (if not already done)

                        Call( m_pjInner->ErrFlush() );
                        Call( m_pc->PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );
                    }

                    if ( m_jposLastEnd != jposInvalid || cbAvail >= cbJournalFullAndDurable )
                    {
                        //  advance the durable for write back pointer

                        err = m_pjInner->ErrAppendEntry( _countof( rgjbEmpty ), rgjbEmpty, &jposEmpty, &jposEmptyEnd );
                        Call( err == JET_errDiskFull ? JET_errSuccess : err );

                        Call( m_pjInner->ErrFlush() );
                        Call( m_pc->PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );
                    }

                    //  ask the cache to flush all its state up to the write back pointer

                    Call( m_pjInner->ErrGetProperties( NULL, &jposReplayNew, NULL, NULL, NULL ) );
                    Call( m_pc->ErrFlushAllState( jposReplayNew ) );

                    //  advance the replay pointer to the write back pointer

                    Call( ErrTruncate( jposReplayNew ) );

                    //  release space in the journal

                    Call( m_pjInner->ErrAppendEntry( _countof( rgjbEmpty ), rgjbEmpty, &jposEmpty, &jposEmptyEnd ) );
                    Call( m_pjInner->ErrFlush() );
                    Call( m_pc->PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        OSTraceSuspendGC();
                        m_pc->BlockCacheNotableEvent( OSFormat( "HandleJournalFullFailure:%i", err ) );
                        OSTraceResumeGC();
                    }
                    return err;
                }

                ERR ErrForceFlush()
                {
                    ERR                 err             = JET_errSuccess;
                    CFlushJournalEntry* pfje            = NULL;
                    JournalPosition     jposFlush       = jposInvalid;
                    JournalPosition     jposFlushEnd    = jposInvalid;

                    //  add an entry to the journal to guarantee that the flush will not be a no-op.  this will allow
                    //  callers to make progress towards advancing durable for writeback

                    Call( CFlushJournalEntry::ErrCreate( &pfje ) );

                    {
                        CJournalBuffer rgjb[] = { { pfje->Cb(), (const BYTE*)pfje }, };
                        Call( ErrAppendEntryInternal( _countof( rgjb ), rgjb, &jposFlush, &jposFlushEnd ) );
                    }

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s 0x%016I64x  Flush",
                                            OSFormatFileId( m_pc ),
                                            QWORD( jposFlush ) ) );

                    //  flush the journal.  this will cause any cached journal segments to be written and then flushed
                    //  and considered durable
                    //
                    //  NOTE:  it is possible that journal segments may be written prior to this call and may point to
                    //  clusters that are not durable.  these journal segments must not be marked as durable by the 
                    //  journal implementation to guarantee that the recovery of our composite journal is successful

                    Call( m_pjInner->ErrFlush() );

                    //  flush the caching file

                    Call( m_pc->PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );

                HandleError:
                    delete pfje;
                    return err;
                }

            private:

                THashedLRUKCache<I>* const  m_pc;
                IJournal* const             m_pjInner;
                CMeteredSection             m_msAppend;
                ERR                         m_errAppend;
                volatile JournalPosition    m_jposLastEnd;
                CSemaphore                  m_semTruncate;
        };

        class CSlabWriteCompletionContext  //  swcc
        {
            public:

                CSlabWriteCompletionContext(    _In_ THashedLRUKCache<I>* const     pc,
                                                _In_ const CMeteredSection::Group   group )
                    :   m_pc( pc ),
                        m_group( group )
                {
                }

                ERR ErrSave(    _In_    CSlabWriteBack* const       pswb,
                                _Inout_ ICachedBlockSlab** const    ppcbs,
                                _In_    const BOOL                  fReleaseSlabOnSave ) const
                {
                    ERR                 err     = JET_errSuccess;
                    ICachedBlockSlab*   pcbs    = *ppcbs;

                    *ppcbs = NULL;

                    Call( pswb->ErrSave( Saved_, DWORD_PTR( this ), &pcbs, fReleaseSlabOnSave ) );

                HandleError:
                    if ( fReleaseSlabOnSave )
                    {
                        m_pc->ReleaseSlab( err, &pcbs );
                    }
                    return err;
                }

                void Cancel() const
                {
                    m_pc->m_msSlabWrites.Leave( m_group );
                }

            private:
        
                static void Saved_( _In_ const ERR                  errSave,
                                    _In_ CSlabWriteBack* const      pswb,
                                    _In_ ICachedBlockSlab* const    pcbs,
                                    _In_ const BOOL                 fReleaseSlabOnSave,
                                    _In_ const DWORD_PTR            keySaved )
                {
                    CSlabWriteBack::PfnSaved pfnSaved  = Saved_;
                    Unused( pfnSaved );

                    CSlabWriteCompletionContext* const pswcc = (CSlabWriteCompletionContext*)keySaved;
                    pswcc->m_pc->Saved( errSave, pswb, pcbs, fReleaseSlabOnSave );
                    pswcc->m_pc->m_msSlabWrites.Leave( pswcc->m_group );
                }

            private:

                THashedLRUKCache<I>* const      m_pc;
                const CMeteredSection::Group    m_group;
        };

        //  Caching File Wrapper
        //
        //  Guarantees that, whenever we perform IO on a caching file:
        //  -  all IO is either iomRaw or iomEngine.
        //  -  all normal IO is unthrottled (doesn't block if we exceed max concurrent IO).

        class CCachingFileWrapper : public CFileFilterWrapper
        {
            public:

                CCachingFileWrapper( _In_ IFileFilter* const pff )
                    :   CFileFilterWrapper( pff, iomEngine )
                {
                }

            public:  //  IFileFilter

                ERR ErrReserveIOREQ(    _In_    const QWORD     ibOffset,
                                        _In_    const DWORD     cbData,
                                        _In_    const OSFILEQOS grbitQOS,
                                        _Out_   VOID **         ppioreq ) override
                {
                    return CFileFilterWrapper::ErrReserveIOREQ( ibOffset, 
                                                                cbData, 
                                                                grbitQOS | CFileFilter::qosBypassIOQuota,
                                                                ppioreq );
                }

                ERR ErrIORead(  _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _Out_writes_( cbData )  BYTE* const                     pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_opt_                const VOID *                    pioreq ) override
                {
                    return CFileFilterWrapper::ErrIORead(   tc,
                                                            ibOffset,
                                                            cbData,
                                                            pbData,
                                                            grbitQOS | CFileFilter::qosBypassIOQuota,
                                                            pfnIOComplete,
                                                            keyIOComplete,
                                                            pfnIOHandoff,
                                                            pioreq );
                }

                ERR ErrIOWrite( _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override
                {
                    return CFileFilterWrapper::ErrIOWrite(  tc,
                                                            ibOffset,
                                                            cbData,
                                                            pbData,
                                                            grbitQOS | CFileFilter::qosBypassIOQuota,
                                                            pfnIOComplete,
                                                            keyIOComplete,
                                                            pfnIOHandoff );
                }

                ERR ErrRead(    _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _Out_writes_( cbData )  BYTE* const                     pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff,
                                _In_opt_                const VOID *                    pioreq ) override
                {
                    if ( iom != iomRaw && iom != iomEngine )
                    {
                        return ErrERRCheck( JET_errInternalError );
                    }

                    return CFileFilterWrapper::ErrRead( tc,
                                                        ibOffset,
                                                        cbData,
                                                        pbData,
                                                        grbitQOS | CFileFilter::qosBypassIOQuota,
                                                        iom,
                                                        pfnIOComplete,
                                                        keyIOComplete,
                                                        pfnIOHandoff,
                                                        pioreq );
                }

                ERR ErrWrite(   _In_                    const TraceContext&             tc,
                                _In_                    const QWORD                     ibOffset,
                                _In_                    const DWORD                     cbData,
                                _In_reads_( cbData )    const BYTE* const               pbData,
                                _In_                    const OSFILEQOS                 grbitQOS,
                                _In_                    const IFileFilter::IOMode       iom,
                                _In_opt_                const IFileAPI::PfnIOComplete   pfnIOComplete,
                                _In_opt_                const DWORD_PTR                 keyIOComplete,
                                _In_opt_                const IFileAPI::PfnIOHandoff    pfnIOHandoff ) override
                {
                    if ( iom != iomRaw && iom != iomEngine )
                    {
                        return ErrERRCheck( JET_errInternalError );
                    }

                    return CFileFilterWrapper::ErrWrite(    tc,
                                                            ibOffset,
                                                            cbData,
                                                            pbData,
                                                            grbitQOS | CFileFilter::qosBypassIOQuota,
                                                            iom,
                                                            pfnIOComplete,
                                                            keyIOComplete,
                                                            pfnIOHandoff );
                }
        };

        //  Cached Block Presence Filter

        class CCachedBlockPresenceFilter
        {
            public:

                static ERR ErrInit( _In_    THashedLRUKCache<I>* const          pc,
                                    _Out_   CCachedBlockPresenceFilter** const  ppcbpf )
                {
                    ERR                         err                 = JET_errSuccess;
                    const QWORD                 cbSlab              = pc->CbChunkPerSlab();
                    const QWORD                 ibSlabMin           = pc->m_pch->IbChunkHash();
                    const QWORD                 cSlab               = pc->m_pch->CbChunkHash() / cbSlab;
                    const size_t                cBitmap             = roundup( cSlab, CHAR_BIT );
                    const size_t                cbBitmap            = (size_t)CbFromCbit( cBitmap );
                    BYTE*                       rgbBitmap           = NULL;
                    IBitmapAPI*                 pbmLoaded           = NULL;
                    LONG*                       rgCount             = NULL;
                    const QWORD                 cCluster            = pc->m_pch->CbClustersHash() / cbCachedBlock;
                    const double                dblLoadFactorMax    = 0.94;
                    const QWORD                 cBucketRaw          = roundup( cCluster, CBucket::C() ) / CBucket::C();
                    const QWORD                 cBucket             = cBucketRaw < NextPowerOf2( cBucketRaw ) * dblLoadFactorMax ?
                                                                        NextPowerOf2( (QWORD)( cBucketRaw / dblLoadFactorMax ) ) :
                                                                        NextPowerOf2( cBucketRaw );
                    CBucket*                    rgBucket            = NULL;
                    const QWORD                 cCuckooMax          = min( cBucket, 500 );
                    const size_t                cStandby            = 1;
                    DWORD*                      rgdwStandby         = NULL;
                    CCachedBlockPresenceFilter* pcbpf               = NULL;

                    *ppcbpf = NULL;

                    Alloc( rgbBitmap = new BYTE[ cbBitmap ] );
                    Alloc( pbmLoaded = new CFixedBitmap( rgbBitmap, cbBitmap ) );
                    OnDebug( Alloc( rgCount = new LONG[ cSlab ] ) );
                    if ( rgCount )
                    {
                        memset( rgCount, 0, cSlab * sizeof( rgCount[ 0 ] ) );
                    }
                    Alloc( rgBucket = new CBucket[ cBucket ] );
                    Alloc( rgdwStandby = new DWORD[ cStandby ] );
                    memset( rgdwStandby, 0, cStandby * sizeof( rgdwStandby[ 0 ] ) );
                    Alloc( pcbpf = new CCachedBlockPresenceFilter(  pc,
                                                                    ibSlabMin, 
                                                                    cbSlab, 
                                                                    &rgbBitmap,
                                                                    &pbmLoaded,
                                                                    &rgCount,
                                                                    cCluster,
                                                                    cBucket,
                                                                    &rgBucket,
                                                                    cCuckooMax,
                                                                    cStandby,
                                                                    &rgdwStandby ) );

                    *ppcbpf = pcbpf;
                    pcbpf = NULL;

                HandleError:
                    delete pcbpf;
                    delete[] rgdwStandby;
                    delete[] rgBucket;
                    delete[] rgCount;
                    delete pbmLoaded;
                    delete[] rgbBitmap;
                    if ( err < JET_errSuccess )
                    {
                        delete *ppcbpf;
                        *ppcbpf = NULL;
                    }
                    return err;
                }

                ~CCachedBlockPresenceFilter()
                {
                    delete[] m_rgdwStandby;
                    delete[] m_rgBucket;
                    delete[] m_rgCount;
                    delete m_pbmLoaded;
                    delete[] m_rgbBitmap;
                }

                BOOL FPossiblyContains( _In_ const QWORD ibSlab, _In_ const CCachedBlockId& cbid )
                {
                    //  if this slab is out of range then claim we have the cached block.  this will cause us to try to
                    //  access the slab which will fail

                    if ( !FValidSlab( ibSlab ) )
                    {
                        return fTrue;
                    }

                    //  if the contents of this slab haven't been loaded yet then claim we have the cached block.  this
                    //  will cause us to access the slab which will then cause us to load the slab's cached blocks
                    //  which will allow us to make a better decision next time

                    if ( !FSlabLoaded( ibSlab ) )
                    {
                        return fTrue;
                    }

                    //  determine if we probably have this cached block.  false positives are possible.  false
                    //  false negatives are disallowed

                    return FPossiblyContains( cbid );
                }

                ERR ErrLoad( _In_ ICachedBlockSlab* const pcbs )
                {
                    ERR     err         = JET_errSuccess;
                    BOOL    fSlabLoaded = fFalse;
                    BOOL    fLeave      = fFalse;
                    QWORD   ibSlab      = 0;

                    //  if this is not a valid slab then ignore it

                    if ( !FValidSlab( pcbs ) )
                    {
                        Error( JET_errSuccess );
                    }

                    //  determine if we already loaded the info for this slab.  if we have then don't load it again

                    if ( ErrIsSlabLoaded( pcbs, &fSlabLoaded ) < JET_errSuccess )
                    {
                        Error( JET_errSuccess );
                    }

                    if ( fSlabLoaded )
                    {
                        Error( JET_errSuccess );
                    }

                    //  if this slab contains changes that aren't accepted then ignore it

                    if ( pcbs->FUpdated() )
                    {
                        Error( JET_errSuccess );
                    }

                    //  visit all slots in the slab and load what is cached

                    Call( pcbs->ErrVisitSlots( FLoadSlot_, (DWORD_PTR)this ) );

                    //  mark the slab as loaded

                    m_critPresenceFilterSlabs.Enter();
                    fLeave = fTrue;

                    Call( pcbs->ErrGetPhysicalId( &ibSlab ) );

                    Call( ErrToErr<IBitmapAPI>( m_pbmLoaded->ErrSet( Islab( ibSlab ), fTrue ) ) );

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter 0x%016I64x SlabLoaded",
                                            OSFormatFileId( m_pc ),
                                            ibSlab ) );

                    m_critPresenceFilterSlabs.Leave();
                    fLeave = fFalse;

                HandleError:
                    if ( fLeave )
                    {
                        m_critPresenceFilterSlabs.Leave();
                    }
                    return err;
                }

                void Update( _In_ ICachedBlockSlab* const pcbs )
                {
                    BOOL    fSlabLoaded = fFalse;

                    //  if this is not a valid slab then ignore it

                    if ( !FValidSlab( pcbs ) )
                    {
                        return;
                    }

                    //  determine if we already loaded the info for this slab.  if we have not then ignore it

                    if ( ErrIsSlabLoaded( pcbs, &fSlabLoaded ) < JET_errSuccess )
                    {
                        return;
                    }

                    if ( !fSlabLoaded )
                    {
                        return;
                    }

                    //  visit all slots in the slab and update our caching state

                    CallS( pcbs->ErrVisitSlots( FUpdateSlot_, (DWORD_PTR)this ) );
                }

            protected:

                class CBucket;

                CCachedBlockPresenceFilter( _In_    THashedLRUKCache<I>* const  pc,
                                            _In_    const QWORD                 ibSlabMin,
                                            _In_    const QWORD                 cbSlab,
                                            _Inout_ BYTE** const                prgbBitmap,
                                            _Inout_ IBitmapAPI** const          ppbmLoaded,
                                            _Inout_ LONG** const                prgCount,
                                            _In_    const QWORD                 cCluster,
                                            _In_    const QWORD                 cBucket,
                                            _Inout_ CBucket** const             prgBucket,
                                            _In_    const QWORD                 cCuckooMax,
                                            _In_    const size_t                cStandby,
                                            _Inout_ DWORD** const               prgdwStandby )
                    :   m_pc( pc ),
                        m_ibSlabMin( ibSlabMin ),
                        m_cbSlab( cbSlab ),
                        m_rgbBitmap( *prgbBitmap ),
                        m_pbmLoaded( *ppbmLoaded ),
                        m_rgCount( *prgCount ),
                        m_cCluster( cCluster ),
                        m_cBucket( cBucket ),
                        m_maskBucket( ( 1LL << Log2( cBucket ) ) - 1 ),
                        m_rgBucket( *prgBucket ),
                        m_cCuckooMax( max( 2, cCuckooMax ) ),
                        m_cStandby( cStandby ),
                        m_rgdwStandby( *prgdwStandby ),
                        m_cChunkPerSlab( m_cbSlab / sizeof( CCachedBlockChunk ) ),
                        m_cSlotPerChunk( CCachedBlockChunk::Ccbl() ),
                        m_critPresenceFilterSlabs( CLockBasicInfo( CSyncBasicInfo( "CCachedBlockPresenceFilter::m_critPresenceFilterSlabs" ), rankPresenceFilterSlabs, 0 ) ),
                        m_cTotal( 0 ),
                        m_rwlPresenceFilter( CLockBasicInfo( CSyncBasicInfo( "CCachedBlockPresenceFilter::m_rwlPresenceFilter" ), rankPresenceFilter, 0 ) )
                {
                    *prgbBitmap = NULL;
                    *ppbmLoaded = NULL;
                    *prgCount = NULL;
                    *prgBucket = NULL;
                    *prgdwStandby = NULL;
                }

            private:

                BOOL FValidSlab( _In_ ICachedBlockSlab* const pcbs )
                {
                    ERR     err         = JET_errSuccess;
                    QWORD   ibSlab      = 0;

                    //  get the slab id

                    Call( pcbs->ErrGetPhysicalId( &ibSlab ) );

                    //  determine if this slab id is valid

                    return FValidSlab( ibSlab );

                HandleError:
                    return err >= JET_errSuccess;
                }

                BOOL FValidSlab( _In_ const QWORD ibSlab )
                {
                    //  determine if this slab is a hash slab

                    return m_pc->FHashSlab( ibSlab );
                }

                ERR ErrIsSlabLoaded( _In_ ICachedBlockSlab* const pcbs, _Out_ BOOL* const pfSlabLoaded )
                {
                    ERR     err         = JET_errSuccess;
                    QWORD   ibSlab      = 0;
                    BOOL    fSlabLoaded = fFalse;

                    *pfSlabLoaded = fFalse;

                    //  get the slab id and compute the slab index

                    Call( pcbs->ErrGetPhysicalId( &ibSlab ) );

                    //  determine if we already loaded the info for this slab

                    fSlabLoaded = FSlabLoaded( ibSlab );

                    *pfSlabLoaded = fSlabLoaded;

                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *pfSlabLoaded = fFalse;
                    }
                    return err;
                }

                BOOL FSlabLoaded( _In_ const QWORD ibSlab )
                {
                    BOOL    fSlabLoaded = fFalse;

                    CallS( ErrToErr<IBitmapAPI>( m_pbmLoaded->ErrGet( Islab( ibSlab ), &fSlabLoaded ) ) );

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter 0x%016I64x IsSlabLoaded %s",
                                            OSFormatFileId( m_pc ),
                                            ibSlab,
                                            fSlabLoaded ? "fTrue" : "fFalse" ) );

                    return fSlabLoaded;
                }

                BOOL FLoadSlot( _In_ const ERR                      errChunk,
                                _In_ const CCachedBlockSlotState&   slotstAccepted,
                                _In_ const CCachedBlockSlotState&   slotstCurrent )
                {
                    //  ignore invalid slots

                    if ( !slotstCurrent.FValid() )
                    {
                        return fTrue;
                    }

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter %s Load Current",
                                            OSFormatFileId( m_pc ),
                                            OSFormat( slotstCurrent ) ) );

                    //  ignore any slot that is superceded

                    if ( slotstCurrent.FSuperceded() )
                    {
                        //  except if the slot is clean, to match the tracking in FUpdateSlot

                        if ( !slotstCurrent.FDirty() )
                        {
                        }
                        else
                        {
                            return fTrue;
                        }
                    }

                    //  remember that the cached block in this slot is cached

                    Add( slotstCurrent.IbSlab(), slotstCurrent.Cbid() );

                    //  if we just finished the entire slab then verify that the counts are correct

                    Assert( slotstCurrent.Chno() < (ChunkNumber)( m_cChunkPerSlab - 1 ) ||
                            slotstCurrent.Slno() < (SlotNumber)( m_cSlotPerChunk - 1 ) ||
                            FValidCounts( slotstCurrent.IbSlab() ) );

                    return fTrue;
                }

                static BOOL FLoadSlot_( _In_ const ERR                      errChunk,
                                        _In_ const CCachedBlockSlotState&   slotstAccepted,
                                        _In_ const CCachedBlockSlotState&   slotstCurrent,
                                        _In_ const DWORD_PTR                keyVisitSlot )
                {
                    ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FLoadSlot_;
                    Unused( pfnVisitSlot );
 
                    CCachedBlockPresenceFilter* const pcbpf = (CCachedBlockPresenceFilter*)keyVisitSlot;
                    return pcbpf->FLoadSlot( errChunk, slotstAccepted, slotstCurrent );
                }

                BOOL FUpdateSlot(   _In_ const ERR                      errChunk,
                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent )
                {
                    //  ignore chunks that are in an error state

                    if ( errChunk < JET_errSuccess )
                    {
                        return fTrue;
                    }

                    //  ignore slots that haven't been updated

                    if ( !slotstCurrent.FSlotUpdated() )
                    {
                        return fTrue;
                    }

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter %s Update Accepted",
                                            OSFormatFileId( m_pc ),
                                            OSFormat( slotstAccepted ) ) );
                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter %s Update Current",
                                            OSFormatFileId( m_pc ),
                                            OSFormat( slotstCurrent ) ) );

                    //  track cached blocks removed from the cache
                    //
                    //  -  any clean block
                    //  -  the current version of a dirty block (!FSuperceded)
                    //
                    //  we track clean blocks due to a limitation with add below

                    if ( slotstAccepted.FValid() && !slotstCurrent.FValid() )
                    {
                        if (    slotstAccepted.Cbid().Volumeid() != volumeidInvalid &&
                                slotstAccepted.Cbid().Fileid() != fileidInvalid && 
                                slotstAccepted.Cbid().Fileserial() != fileserialInvalid )
                        {
                            if ( !slotstAccepted.FDirty() || !slotstAccepted.FSuperceded() )
                            {
                                Remove( slotstAccepted.IbSlab(), slotstAccepted.Cbid() );
                            }
                        }
                    }

                    //  track cached blocks added to the cache
                    //
                    //  -  any clean block
                    //  -  the first update of a block (FFirstUpdate)
                    //
                    //  we track both clean blocks and blocks that are first updated because we don't have enough state
                    //  to only track either the clean block or the first dirty if the clean block was not previously
                    //  cached just by looking at this one slot.  ideally we would detect this and track only one

                    if ( !slotstAccepted.FValid() && slotstCurrent.FValid() )
                    {
                        if (    slotstCurrent.Cbid().Volumeid() != volumeidInvalid &&
                                slotstCurrent.Cbid().Fileid() != fileidInvalid && 
                                slotstCurrent.Cbid().Fileserial() != fileserialInvalid )
                        {
                            if ( !slotstCurrent.FDirty() || slotstCurrent.FFirstUpdate() )
                            {
                                Add( slotstCurrent.IbSlab(), slotstCurrent.Cbid() );
                            }
                        }
                    }

                    //  if we just finished the entire slab then verify that the counts are correct

                    Assert( slotstCurrent.Chno() < (ChunkNumber)( m_cChunkPerSlab - 1 ) ||
                            slotstCurrent.Slno() < (SlotNumber)( m_cSlotPerChunk - 1 ) ||
                            FValidCounts( slotstCurrent.IbSlab() ) );

                    return fTrue;
                }

                static BOOL FUpdateSlot_(   _In_ const ERR                      errChunk,
                                            _In_ const CCachedBlockSlotState&   slotstAccepted,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const DWORD_PTR                keyVisitSlot )
                {
                    ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FUpdateSlot_;
                    Unused( pfnVisitSlot );
 
                    CCachedBlockPresenceFilter* const pcbpf = (CCachedBlockPresenceFilter*)keyVisitSlot;
                    return pcbpf->FUpdateSlot( errChunk, slotstAccepted, slotstCurrent );
                }

                void Add( _In_ const QWORD ibSlab, _In_ const CCachedBlockId& cbid )
                {
                    //  convert the item into a hash

                    const DWORD dwHash = DwHash( cbid );

                    //  track the count of entries by slab

                    IncrementCount( ibSlab );

                    //  add the cached block to the presence filter

                    m_rwlPresenceFilter.EnterAsWriter();

                    Add( dwHash );

                    //  we should think that the cache contains this cached block

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter %s,0x%08x Add 0x%02x",
                                            OSFormatFileId( m_pc ),
                                            OSFormat(   cbid.Volumeid(),
                                                        cbid.Fileid(),
                                                        cbid.Fileserial() ),
                                            cbid.Cbno(),
                                            WFingerprint( dwHash ) ) );

                    EnforceSz( FPossiblyContains( dwHash ), "HashedLRUKCachePresenceFilterAdd" );

                    m_rwlPresenceFilter.LeaveAsWriter();
                }

                void Remove( _In_ const QWORD ibSlab, _In_ const CCachedBlockId& cbid )
                {
                    //  convert the item into a hash

                    const DWORD dwHash = DwHash( cbid );

                    //  track the count of entries by slab

                    DecrementCount( ibSlab );

                    //  we should think that the cache contains this cached block

                    m_rwlPresenceFilter.EnterAsWriter();

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter %s,0x%08x Remove 0x%02x",
                                            OSFormatFileId( m_pc ),
                                            OSFormat(   cbid.Volumeid(),
                                                        cbid.Fileid(),
                                                        cbid.Fileserial() ),
                                            cbid.Cbno(),
                                            WFingerprint( dwHash ) ) );

                    EnforceSz( FPossiblyContains( dwHash ), "HashedLRUKCachePresenceFilterRemove" );

                    //  remove the cached block from the presence filter

                    Remove( dwHash );

                    m_rwlPresenceFilter.LeaveAsWriter();
                }

                BOOL FPossiblyContains( _In_ const CCachedBlockId& cbid )
                {
                    //  convert the item into a hash

                    const DWORD dwHash = DwHash( cbid );

                    //  determine if the cached block is present via the presence filter

                    m_rwlPresenceFilter.EnterAsReader();

                    BOOL fPossiblyContains = FPossiblyContains( dwHash );

                    OSTrace(    JET_tracetagBlockCacheOperations,
                                OSFormat(   "C=%s Presence Filter %s,0x%08x Contains 0x%02x %s",
                                            OSFormatFileId( m_pc ),
                                            OSFormat(   cbid.Volumeid(),
                                                        cbid.Fileid(),
                                                        cbid.Fileserial() ),
                                            cbid.Cbno(),
                                            WFingerprint( dwHash ),
                                            fPossiblyContains ? "fTrue" : "fFalse" ) );

                    m_rwlPresenceFilter.LeaveAsReader();

                    return fPossiblyContains;
                }

                size_t Islab( _In_ const QWORD ibSlab ) const
                {
                    return (size_t)( ( ibSlab - m_ibSlabMin ) / m_cbSlab );
                }

                DWORD DwHash( _In_ const CCachedBlockId& cbid ) const
                {
                    //  we will hash the CachedBlockNumber

                    const DWORD dwHashInput = (DWORD)cbid.Cbno();

                    //  use a mixed tabulation hash based on cryptographically random numbers to hash the CachedBlockNumber.  this
                    //  allows us to get a good hash function though the input is an integer with non-uniformly used values that are
                    //  close to zero

                    DWORD dwHash = CMixedTabulationHash::DwHash( dwHashInput );

                    //  combine the hash with the FileSerial which is a cryptographically random number

                    dwHash ^= (DWORD)cbid.Fileserial();

                    //  we cannot return zero for the hash

                    return dwHash ? dwHash : 0x12345678;
                }

                void IncrementCount( _In_ const QWORD ibSlab )
                {
                    OnDebug( AtomicAdd( (QWORD*)&m_cTotal, 1 ) );
                    OnDebug( AtomicIncrement( (LONG*)&m_rgCount[ Islab( ibSlab ) ] ) );
                }

                void DecrementCount( _In_ const QWORD ibSlab )
                {
                    OnDebug( AtomicAdd( (QWORD*)&m_cTotal, (QWORD)-1LL ) );
                    OnDebug( AtomicDecrement( (LONG*)&m_rgCount[ Islab( ibSlab ) ] ) );
                }

                BOOL FValidCounts( _In_ const QWORD ibSlab ) const
                {
                    const LONGLONG  cTotal      = OnDebugOrRetail( (LONGLONG)AtomicRead( (QWORD*)&m_cTotal ), 0 );
                    const BOOL      fValidTotal = cTotal >= 0 && cTotal <= (LONGLONG)m_cCluster;
                    const size_t    iSlab       = Islab( ibSlab );
                    const LONG      cSlab       = OnDebugOrRetail( AtomicRead( (LONG*)&m_rgCount[ iSlab ] ), 0 );
                    const QWORD     cSlabMost   = m_cChunkPerSlab * m_cSlotPerChunk;
                    const BOOL      fValidSlab  = cSlab >= 0 && cSlab <= cSlabMost;

                    return fValidTotal && fValidSlab;
                }

            private:

                //  the presence filter is implemented as a Cuckoo Filter

                void Add( _In_ const DWORD dwHash )
                {
                    //  compute the fingerprint and primary location for this item

                    WORD    wFingerprint    = 0;
                    QWORD   qwIndex         = 0;

                    ComputeProperties( dwHash, &wFingerprint, &qwIndex );

                    //  try to add this item

                    BOOL fSucceeded = FAdd( qwIndex, wFingerprint );

                    //  if we didn't succeed then try to put the item on the standby list

                    if ( !fSucceeded )
                    {
                        fSucceeded = FAddStandbyItem( dwHash );
                    }

                    //  we must succeed

                    EnforceSz( fSucceeded, "HashedLRUKCachePresenceFilterAdd2" );
                }

                BOOL FAdd( _In_ const QWORD qwIndexInitial, _In_ const WORD wFingerprintInitial )
                {
                    BOOL    fSucceeded              = fFalse;
                    QWORD   qwIndex                 = qwIndexInitial;
                    WORD    wFingerprint            = wFingerprintInitial;
                    WORD    wFingerprintReplaced    = wFingerprintInitial;

                    //  try to add this item to the primary or alternate location, recursively pushing out entries
                    //  until we can find room or we hit our retry limit

                    for ( int i = 0; !fSucceeded && i < m_cCuckooMax; i++ )
                    {
                        fSucceeded = Bucket( qwIndex ).FTryAdd( wFingerprint, i > 0 ? &wFingerprintReplaced : NULL );

                        if ( !fSucceeded )
                        {
                            qwIndex = QwIndexAlternate( qwIndex, wFingerprintReplaced );
                            wFingerprint = wFingerprintReplaced;
                            wFingerprintReplaced = 0;
                        }
                    }

                    return fSucceeded;
                }

                void Remove( _In_ const DWORD dwHash )
                {
                    //  compute the fingerprint and primary location for this item

                    WORD    wFingerprint    = 0;
                    QWORD   qwIndex         = 0;

                    ComputeProperties( dwHash, &wFingerprint, &qwIndex );

                    //  try to remove this item from the primary location or the alternate location

                    BOOL fSucceeded = fFalse;

                    for ( int i = 0; !fSucceeded && i < 2; i++ )
                    {
                        fSucceeded = Bucket( qwIndex ).FTryRemove( wFingerprint );

                        if ( !fSucceeded )
                        {
                            qwIndex = QwIndexAlternate( qwIndex, wFingerprint );
                        }
                    }

                    //  this must succeed

                    EnforceSz( fSucceeded, "HashedLRUKCachePresenceFilterRemove2" );

                    //  try to retire an item from the standby list

                    RetireStandbyItem();
                }

                BOOL FPossiblyContains( _In_ const DWORD dwHash )
                {
                    //  compute the fingerprint and primary location for this item

                    WORD    wFingerprint    = 0;
                    QWORD   qwIndex         = 0;

                    ComputeProperties( dwHash, &wFingerprint, &qwIndex );

                    //  try to find this item in the primary location or the alternate location

                    BOOL fSucceeded = fFalse;

                    for ( int i = 0; !fSucceeded && i < 2; i++ )
                    {
                        fSucceeded = Bucket( qwIndex ).FContains( wFingerprint );

                        if ( !fSucceeded )
                        {
                            qwIndex = QwIndexAlternate( qwIndex, wFingerprint );
                        }
                    }

                    return fSucceeded;
                }

            private:

                QWORD QwIndex( _In_ const QWORD qw ) const
                {
                    return qw & m_maskBucket;
                }

                WORD WFingerprint( _In_ const DWORD dwHash ) const
                {
                    const WORD wFingerprintMin  = 1;
                    const WORD wFingerprintMax  = 1 << CBucket::CbitFingerprint();
                    const WORD wFingerprint     = wFingerprintMin + dwHash % ( wFingerprintMax - wFingerprintMin );

                    Assert( wFingerprintMin <= wFingerprint && wFingerprint < wFingerprintMax );

                    return wFingerprint;
                }

                QWORD QwIndexAlternate_( _In_ const QWORD qwIndex, _In_ const WORD wFingerprint ) const
                {
                    return QwIndex( qwIndex ^ CMixedTabulationHash::DwHash( wFingerprint ) );
                }

                QWORD QwIndexAlternate( _In_ const QWORD qwIndex, _In_ const WORD wFingerprint ) const
                {
                    const QWORD qwIndexAlternate = QwIndexAlternate_( qwIndex, wFingerprint );

                    //  we must be able to find the primary bucket index from the alternate bucket index and the
                    //  fingerprint.  in other words, this computation must be reversible
                    //
                    //  NOTE:  this only works if the bucket count is a power of 2

                    Assert( FPowerOf2( m_cBucket ) );
                    Assert( qwIndex == QwIndexAlternate_( qwIndexAlternate, wFingerprint ) );

                    return qwIndexAlternate;
                }

                void ComputeProperties( _In_    const DWORD     dwHash,
                                        _Out_   WORD* const     pwFingerprint,
                                        _Out_   QWORD* const    pqwIndex )
                {
                    //  compute the fingerprint 

                    *pwFingerprint = WFingerprint( dwHash );

                    //  compute the primary location

                    *pqwIndex = QwIndex( dwHash >> CBucket::CbitFingerprint() );
                }

                BOOL FAddStandbyItem( _In_ const DWORD dwHash )
                {
                    BOOL fSucceeded = fFalse;

                    for ( int iStandby = 0; !fSucceeded && iStandby < m_cStandby; iStandby++ )
                    {
                        fSucceeded = AtomicCompareExchange( (LONG*)&m_rgdwStandby[ iStandby ], 0, dwHash ) == 0;
                    }

                    return fSucceeded;
                }

                void RetireStandbyItem()
                {
                    DWORD   dwHash      = 0;
                    BOOL    fSucceeded  = fFalse;

                    fSucceeded = FRemoveStandbyItem( &dwHash );

                    if ( fSucceeded )
                    {
                        QWORD   qwIndex         = QwIndex( dwHash );
                        WORD    wFingerprint    = WFingerprint( dwHash );

                        fSucceeded = FAdd( qwIndex, wFingerprint );
                    }

                    if ( !fSucceeded )
                    {
                        fSucceeded = FAddStandbyItem( dwHash );
                    }

                    EnforceSz( fSucceeded, "HashedLRUKCachePresenceFilterRetireStandbyItem" );
                }

                BOOL FRemoveStandbyItem( _Out_ DWORD* const pdwHash )
                {
                    BOOL fSucceeded = fFalse;

                    *pdwHash = 0;

                    for ( int iStandby = 0; !fSucceeded && iStandby < m_cStandby; iStandby++ )
                    {
                        *pdwHash = AtomicExchange( (LONG*)&m_rgdwStandby[ iStandby ], 0 );
                        fSucceeded = *pdwHash != 0;
                    }

                    return fSucceeded;
                }

                CBucket& Bucket( _In_ const QWORD i ) const { return m_rgBucket[ (size_t)i ]; }

#include <pshpack1.h>

                class CBucket
                {
                    public:

                        CBucket()
                        {
                            C_ASSERT( sizeof( *this ) == sizeof( m_rgbFingerprint ) );
                            memset( this, 0, sizeof( *this ) );
                        }

                        static size_t C() { return _countof( m_rgbFingerprint ); }
                        static size_t CbitFingerprint() { return sizeof( m_rgbFingerprint[0] ) * 8; }

                        BOOL FTryAdd(   _In_        const WORD  wFingerprint,
                                        _Out_opt_   WORD* const pwFingerprintReplaced = NULL )
                        {
                            if ( pwFingerprintReplaced )
                            {
                                *pwFingerprintReplaced = 0;
                            }

                            for ( int i = 0; i < _countof( m_rgbFingerprint ); i++ )
                            {
                                if ( m_rgbFingerprint[ i ] == 0 )
                                {
                                    m_rgbFingerprint[ i ] = (BYTE)wFingerprint;
                                    return fTrue;
                                }
                            }

                            if ( pwFingerprintReplaced )
                            {
                                const size_t    iRandom = rand();
                                const size_t    iVictim = iRandom % _countof( m_rgbFingerprint );

                                *pwFingerprintReplaced = m_rgbFingerprint[ iVictim ];
                                m_rgbFingerprint[ iVictim ] = (BYTE)wFingerprint;
                            }

                            return fFalse;
                        }

                        BOOL FTryRemove( _In_ const WORD wFingerprint )
                        {
                            for ( int i = 0; i < _countof( m_rgbFingerprint ); i++ )
                            {
                                if ( m_rgbFingerprint[ i ] == wFingerprint )
                                {
                                    m_rgbFingerprint[ i ] = 0;
                                    return fTrue;
                                }
                            }

                            return fFalse;
                        }

                        BOOL FContains( _In_ const WORD wFingerprint )
                        {
                            for ( int i = 0; i < _countof( m_rgbFingerprint ); i++ )
                            {
                                if ( m_rgbFingerprint[ i ] == wFingerprint )
                                {
                                    return fTrue;
                                }
                            }

                            return fFalse;
                        }

                    private:

                        BYTE    m_rgbFingerprint[ 4 ];
                };

#include <poppack.h>

            private:

                THashedLRUKCache<I>* const  m_pc;
                const QWORD                 m_ibSlabMin;
                const QWORD                 m_cbSlab;
                BYTE* const                 m_rgbBitmap;
                IBitmapAPI* const           m_pbmLoaded;
                volatile LONG* const        m_rgCount;
                const QWORD                 m_cCluster;
                const QWORD                 m_cBucket;
                const QWORD                 m_maskBucket;
                CBucket* const              m_rgBucket;
                const QWORD                 m_cCuckooMax;
                const size_t                m_cStandby;
                DWORD* const                m_rgdwStandby;
                const QWORD                 m_cChunkPerSlab;
                const QWORD                 m_cSlotPerChunk;
                CCriticalSection            m_critPresenceFilterSlabs;
                volatile LONGLONG           m_cTotal;
                CReaderWriterLock           m_rwlPresenceFilter;
        };

    private:

        ERR ErrDumpJournalMetadata( _In_ CPRINTF* const pcprintf );
        ERR ErrDumpJournalEntries( _In_ CPRINTF* const pcprintf );
        ERR ErrDumpJournalEntry( _In_ const CQueuedJournalEntry* const pqje, _In_ CPRINTF* const pcprintf );

        class CChunkStatus
        {
            public:
 
                static ERR ErrExecute(  _In_    ICachedBlockSlab* const pcbs,
                                        _In_    const ChunkNumber       chno,
                                        _Out_   ERR* const              perrChunk )
                {
                    ERR             err = JET_errSuccess;
                    CChunkStatus    cs( pcbs, chno );

                    *perrChunk = JET_errSuccess;
 
                    Call( cs.ErrVisitSlots() );

                    *perrChunk = cs.m_errChunk;
 
                HandleError:
                    if ( err < JET_errSuccess )
                    {
                        *perrChunk = JET_errSuccess;
                    }
                    return err;
                }
 
            private:
 
                CChunkStatus( _In_ ICachedBlockSlab* const pcbs, _In_ const ChunkNumber chno )
                        :   m_pcbs( pcbs ),
                            m_chno( chno ),
                            m_errChunk( JET_errSuccess )
                {
                }
 
                ERR ErrVisitSlots()
                {
                    return m_pcbs->ErrVisitSlots( FVisitSlot_, DWORD_PTR( this ) );
                }

                BOOL FVisitSlot(    _In_ const ERR                      errChunk,
                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent )
                {
                    if ( slotstCurrent.Chno() == m_chno )
                    {
                        m_errChunk = errChunk;
                        return fFalse;
                    }

                    return fTrue;
                }
 
                static BOOL FVisitSlot_(    _In_ const ERR                      errChunk,
                                            _In_ const CCachedBlockSlotState&   slotstAccepted,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const DWORD_PTR                keyVisitSlot )
                {
                    ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FVisitSlot_;
                    Unused( pfnVisitSlot );
 
                    CChunkStatus* const pcs = (CChunkStatus*)keyVisitSlot;
                    return pcs->FVisitSlot( errChunk, slotstAccepted, slotstCurrent );
                }
 
            private:
 
                ICachedBlockSlab* const m_pcbs;
                const ChunkNumber       m_chno;
                ERR                     m_errChunk;
        };

        ERR ErrDumpWriteCounts( _In_ CPRINTF* const pcprintf );
        ERR ErrDumpWriteCounts( _In_ const QWORD    ibChunkBase,
                                _In_ const QWORD    cbChunk,
                                _In_ const QWORD    icbwcBase,
                                _In_ CPRINTF* const pcprintf );

        ERR ErrDumpHashChunks( _In_ CPRINTF* const pcprintf );
        ERR ErrDumpJournalChunks( _In_ CPRINTF* const pcprintf );

        class CChunkDumper
        {
            public:
 
                static ERR ErrExecute(  _In_ THashedLRUKCache<I>* const     pc,
                                        _In_ ICachedBlockSlabManager* const pcbsm,
                                        _In_ CPRINTF* const                 pcprintf )
                {
                    ERR err = JET_errSuccess;
 
                    Call( CChunkDumper( pc, pcbsm, pcprintf ).ErrVisitSlabs() );
 
                HandleError:
                    return err;
                }
 
            private:
 
                CChunkDumper(   _In_ THashedLRUKCache<I>* const     pc,
                                _In_ ICachedBlockSlabManager* const pcbsm,
                                _In_ CPRINTF* const                 pcprintf )
                        :   m_pc( pc ),
                            m_pcbsm( pcbsm ),
                            m_pcprintf( pcprintf )
                {
                }
 
                ERR ErrVisitSlabs()
                {
                    return m_pcbsm->ErrVisitSlabs( FVisitSlab_, DWORD_PTR( this ) );
                }
 
                BOOL FVisitSlab(    _In_ const QWORD                ib,
                                    _In_ const ERR                  errSlab,
                                    _In_ ICachedBlockSlab* const    pcbs );
 
                static BOOL FVisitSlab_(    _In_ const QWORD                ib,
                                            _In_ const ERR                  errSlab,
                                            _In_ ICachedBlockSlab* const    pcbs,
                                            _In_ const DWORD_PTR            keyVisitSlab )
                {
                    ICachedBlockSlabManager::PfnVisitSlab pfnVisitSlab = FVisitSlab_;
                    Unused( pfnVisitSlab );
 
                    CChunkDumper* const pcd = (CChunkDumper*)keyVisitSlab;
                    return pcd->FVisitSlab( ib, errSlab, pcbs );
                }

                BOOL FVisitSlot(    _In_ const ERR                      errChunk,
                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                    _In_ const CCachedBlockSlotState&   slotstCurrent );
 
                static BOOL FVisitSlot_(    _In_ const ERR                      errChunk,
                                            _In_ const CCachedBlockSlotState&   slotstAccepted,
                                            _In_ const CCachedBlockSlotState&   slotstCurrent,
                                            _In_ const DWORD_PTR                keyVisitSlot )
                {
                    ICachedBlockSlab::PfnVisitSlot pfnVisitSlot = FVisitSlot_;
                    Unused( pfnVisitSlot );
 
                    CChunkDumper* const pcd = (CChunkDumper*)keyVisitSlot;
                    return pcd->FVisitSlot( errChunk, slotstAccepted, slotstCurrent );
                }
 
            private:
 
                THashedLRUKCache<I>* const      m_pc;
                ICachedBlockSlabManager* const  m_pcbsm;
                CPRINTF* const                  m_pcprintf;
        };

    private:

        ERR ErrInit();

        ERR ErrRecover();

        ERR ErrMountJournal( _In_ CHashedLRUKCacheHeader* const pch, _Out_ IJournal** const ppj );

        ERR ErrAnalyzeJournal( _In_ const BOOL fAll = fFalse );

        static BOOL FVisitJournalEntry_(    _In_ const JournalPosition  jpos,
                                            _In_ const JournalPosition  jposEnd,
                                            _In_ const CJournalBuffer   jb,
                                            _In_ const DWORD_PTR        keyVisitEntry )
        {
            IJournal::PfnVisitEntry pfnVisitEntry = FVisitJournalEntry_;
            Unused( pfnVisitEntry );

            THashedLRUKCache<I>* const pc = ( THashedLRUKCache<I>*)keyVisitEntry;
            return pc->FVisitJournalEntry( jpos, jposEnd, jb );
        }

        BOOL FVisitJournalEntry(    _In_ const JournalPosition  jpos,
                                    _In_ const JournalPosition  jposEnd,
                                    _In_ const CJournalBuffer   jb );

        ERR ErrProcessClusterReference( _In_ const ClusterNumber    clno,
                                        _In_ const ERR              errCluster,
                                        _In_ const JournalPosition  jpos );
        void ProcessClusterInvalidate( _In_ const ClusterNumber clno );
        ERR ErrCheckForUnresolvedFailedClusterReferences();

        ERR ErrRedoJournalEntries();
        ERR ErrRedoJournalEntry( _In_ const CQueuedJournalEntry* const pqje );
        ERR ErrRedoCacheUpdateJournalEntry( _In_ const CQueuedJournalEntry* const pqje );
        void LogCacheUpdateJournalEntry( _In_ const CQueuedJournalEntry* const  pqje,
                                            _In_ const QWORD                    ibSlab );
        void ReleaseJournalEntries();

        ERR ErrChangeSlabs( _In_        const QWORD                 ibSlab,
                            _Inout_     ICachedBlockSlab** const    ppcbs,
                            _Out_opt_   BOOL* const                 pfChangedSlab = NULL );

        ERR ErrUpdateSlab( _Inout_opt_ ICachedBlockSlab** const ppcbs )
        {
            ICachedBlockSlab* pcbsEmpty = NULL;
            return ErrUpdateSlabs( ppcbs, &pcbsEmpty );
        }

        ERR ErrUpdateSlabs( _Inout_opt_ ICachedBlockSlab** const    ppcbsA,
                            _Inout_opt_ ICachedBlockSlab** const    ppcbsB );

        ERR ErrScheduleSlabForWriteBack(    _In_opt_    ICachedBlockSlab* const pcbs,
                                            _In_        const JournalPosition   jpos,
                                            _In_        const JournalPosition   jposEnd );
        ERR ErrGetOrAddSlabWriteBackContext(    _In_    ICachedBlockSlab* const pcbs,
                                                _Out_   CSlabWriteBack** const  ppswb );
        void RegisterSlabForWriteBack( _In_ ICachedBlockSlab* const pcbs, _In_ CSlabWriteBack* const pswb );
        void CueAsyncSlabWriteBackWorker();
        void AsyncSlabWriteBackWorker();
        void PerformOpportunisticSlabWriteBacks();
        ERR ErrVerifyTruncate( _In_ const JournalPosition jposReplay );
        ERR ErrFlushAllState( _In_ const JournalPosition jposDurableForWriteBack );
        ERR ErrTryStartSlabWriteBacks(  _In_ CArray<QWORD>&         arrayIbSlab,
                                        _In_ const BOOL             fSaveOpenSlabs );
        ICachedBlockSlab* PcbsGetOpenSlabSafeForWriteBack( _In_ const QWORD ibSlab );
        ERR ErrFinishSlabWriteBacks();
        ERR ErrTryStartSlabWriteBack(   _Inout_ ICachedBlockSlab** const    ppcbs,
                                        _In_    const BOOL                  fReleaseSlabOnSave,
                                        _Out_   BOOL* const                 pfSaved );
        ERR ErrSaveWriteCounts();
        static void Saved_( _In_ const ERR                  errSave,
                            _In_ CSlabWriteBack* const      pswb,
                            _In_ ICachedBlockSlab* const    pcbs,
                            _In_ const BOOL                 fReleaseSlabOnSave,
                            _In_ const DWORD_PTR            keySaved )
        {
            CSlabWriteBack::PfnSaved pfnSaved = Saved_;
            Unused( pfnSaved );

            THashedLRUKCache<I>* const pc = ( THashedLRUKCache<I>*)keySaved;
            pc->Saved( errSave, pswb, pcbs, fReleaseSlabOnSave );
        }
        void Saved( _In_ const ERR                  errSave,
                    _In_ CSlabWriteBack* const      pswb,
                    _In_ ICachedBlockSlab* const    pcbs,
                    _In_ const BOOL                 fReleaseSlabOnSave );
        static INT __cdecl CompareIbSlab( _In_ const QWORD* const pibSlab1, _In_ const QWORD* const pibSlab2 )
        {
            if ( *pibSlab1 > * pibSlab2 )
            {
                return 1;
            }
            else if ( *pibSlab1 < *pibSlab2 )
            {
                return -1;
            }
            
            return 0;
        }
        CSlabWriteBack* PswbTryGetSlabWriteBackContext( _In_ ICachedBlockSlab* const pcbs );

        CMeteredSection::Group GroupForClusterWrite();
        void ClusterWritten( _In_ const ERR err, _In_ const CMeteredSection::Group group );
        ERR ErrFlushClusters();

        ERR ErrFlush();

        ERR ErrEnqueue( _Inout_ CRequest** const pprequest );
        BOOL FConflicting( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB );
        BOOL FCombinable( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB );
        int CmpRequestIO( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB );

        void Issue();

        void AsyncIOWorker( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls );

        ERR ErrSynchronousIO( _In_ CRequest* const prequest );

        void RequestCachedFileIO( _In_ CRequest* const prequestIO );
        void RequestCachingFileIO( _In_ CRequest* const prequestIO );
        void RequestIO( _In_ CRequest* const prequestIO );
        void WaitForPendingIOAsync( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const   pctls,
                                    _In_ CRequest* const                                prequestIO );
        void WaitForPendingIO( _In_ CRequest* const prequestIO );
        BOOL FCompletedIO( _In_ CRequest* const prequestIO );
        void RequestFinalizeIO( _In_ CRequest* const prequestIO );

        void RequestIO( _In_    CRequest* const prequestIO, 
                        _In_    const BOOL      fCachedFile,
                        _In_    const BOOL      fCachingFile );
        void RequestRead(   _In_    CRequest* const             prequest,
                            _In_    const BOOL                  fCachedFile, 
                            _In_    const BOOL                  fCachingFile,
                            _Inout_ ICachedBlockSlab** const    ppcbs );
        void RequestFinalizeRead(   _In_    CRequest* const             prequestIO,
                                    _Inout_ ICachedBlockSlab** const    ppcbs );
        ERR ErrUnexpectedDataReadFailure(   _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                            _In_ const COffsets                             offsets,
                                            _In_ const char* const                          szFunction,
                                            _In_ const ERR                                  errFromCall,
                                            _In_ const ERR                                  errToReturn );
        void RequestWrite(  _In_    CRequest* const             prequest,
                            _In_    const BOOL                  fCachedFile, 
                            _In_    const BOOL                  fCachingFile,
                            _Inout_ ICachedBlockSlab** const    ppcbs );
        void RequestFinalizeWrite(  _In_    CRequest* const             prequestIO,
                                    _Inout_ ICachedBlockSlab** const    ppcbs );

        ERR ErrCleanSlab(   _In_    CRequest* const             prequest,
                            _Inout_ ICachedBlockSlab** const    ppcbs,
                            _In_    const BOOL                  fRead,
                            _In_    const QWORD                 ib,
                            _Inout_ QWORD* const                pcbClean );

        ERR ErrIsPossiblyCached(    _In_    CHashedLRUKCachedFileTableEntry<I>* pcfte,
                                    _In_    const QWORD                         ibCachedBlock,
                                    _Out_   QWORD* const                        pibSlab,
                                    _Out_   CCachedBlockId* const               pcbid,
                                    _Out_   BOOL* const                         pfPossiblyCached );
        void FailIO( _In_ CRequest* const prequestIO, _In_ const ERR err );

        ERR ErrEnsureInitSlabWriteBackHash() { return m_initOnceSlabWriteBackHash.Init( ErrInitSlabWriteBackHash_, this ); };
        ERR ErrInitSlabWriteBackHash();
        static ERR ErrInitSlabWriteBackHash_( _In_ THashedLRUKCache<I>* const pc ) { return pc->ErrInitSlabWriteBackHash(); }
        void TermSlabWriteBackHash();

        void ReleaseCompletedSlabWriteBacks();

        BOOL FHashSlab( _In_ const QWORD ibSlab )
        {
            return m_pch->IbChunkHash() <= ibSlab && ibSlab < m_pch->IbChunkHash() + m_pch->CbChunkHash();
        }

        BOOL FJournalSlab( _In_ const QWORD ibSlab )
        {
            return m_pch->IbChunkJournal() <= ibSlab && ibSlab < m_pch->IbChunkJournal() + m_pch->CbChunkJournal();
        }

        ERR ErrGetSlab( _In_ const QWORD ibSlab, _Out_ ICachedBlockSlab** const ppcbs )
        {
            return ErrGetSlabInternal( ibSlab, fFalse, fFalse, ppcbs );
        }
        ERR ErrGetSlabForRecovery( _In_ const QWORD ibSlab, _Out_ ICachedBlockSlab** const ppcbs )
        {
            return ErrGetSlabInternal( ibSlab, fTrue, fFalse, ppcbs );
        }
        ERR ErrTryGetSlabForWriteBack( _In_ const QWORD ibSlab, _Out_ ICachedBlockSlab** const ppcbs )
        {
            return ErrGetSlabInternal( ibSlab, fFalse, fTrue, ppcbs );
        }
        ERR ErrGetSlabInternal( _In_    const QWORD                 ibSlab,
                                _In_    const BOOL                  fIgnoreVerificationErrors,
                                _In_    const BOOL                  fForSlabWriteBack,
                                _Out_   ICachedBlockSlab** const    ppcbs );
        ERR ErrUnexpectedMetadataReadFailure(   _In_ const char* const  szFunction,
                                                _In_ const ERR          errFromCall,
                                                _In_ const ERR          errToReturn );
        void ReleaseSlab( _In_ const ERR err, _Inout_ ICachedBlockSlab** const ppcbs );
        BOOL FAnyOpenSlab();

        ERR ErrStartStateAccessAttempt( _Out_ CMeteredSection::Group* const pgroup );
        ERR ErrSuspendThreadFromStateAccess(    _In_ const CMeteredSection::Group                   group,
                                                _In_ CHashedLRUKCacheThreadLocalStorage<I>* const   pctls );
        void EndStateAccessAttempt( _Inout_ CMeteredSection::Group* const pgroup );
        typedef CMeteredSection::PFNPARTITIONCOMPLETE PfnStateAccessQuiesced;
        void QuiesceStateAccess(    _In_ const PfnStateAccessQuiesced   pfnStateAccessQuiesced,
                                    _In_ const DWORD_PTR                dwCompletionKey );
        void ResumeStateAccess();

        ERR ErrSuspendBlockedThreadsFromStateAccess();
        static BOOL FSuspendBlockedThreadFromStateAccess_(  _In_ CHashedLRUKCacheThreadLocalStorage<I>* const   pctls,
                                                            _In_ const DWORD_PTR                                keyVisitTls )
        {
            THashedLRUKCacheBase<I>::PfnVisitTls pfnVisitTls = (THashedLRUKCacheBase<I>::PfnVisitTls)FSuspendBlockedThreadFromStateAccess_;
            Unused( pfnVisitTls );

            THashedLRUKCache<I>* const pc = (THashedLRUKCache<I>*)keyVisitTls;
            return pc->FSuspendBlockedThreadFromStateAccess( pctls );
        }
        BOOL FSuspendBlockedThreadFromStateAccess( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls );

        ERR ErrRegisterOpenSlab( _In_ ICachedBlockSlab* const pcbs );
        void RegisterOpenSlab( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls, _In_ ICachedBlockSlab* const pcbs );
        void UnregisterOpenSlab( _In_ ICachedBlockSlab* const pcbs );

        void RegisterOpenSlabWait( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls, _In_ const QWORD ibSlab );
        void UnregisterOpenSlabWait( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls, _In_ const QWORD ibSlab );

        ERR ErrGetJournalSlab( _Out_ ICachedBlockSlab** const ppcbs );

        size_t IcrefJournalSlab( _In_ const QWORD ibSlab );
        void ReferenceJournalSlab( _In_ const QWORD ibSlab );
        void ReleaseJournalSlab( _In_ const QWORD ibSlab );

        ERR ErrEvictSlot(   _In_    ICachedBlockSlab*           pcbs,
                            _In_    const CCachedBlockSlot&     slot,
                            _Inout_ ICachedBlockSlab** const    ppcbsJournal )
        {
            return ErrEvictOrInvalidateSlot( pcbs, slot, ppcbsJournal );
        }
        ERR ErrInvalidateSlot(  _In_    ICachedBlockSlab*           pcbs,
                                _In_    const CCachedBlockSlot&     slot,
                                _Inout_ ICachedBlockSlab** const    ppcbsJournal )
        {
            return ErrEvictOrInvalidateSlot( pcbs, slot, ppcbsJournal );
        }
        ERR ErrEvictOrInvalidateSlot(   _In_    ICachedBlockSlab*           pcbs,
                                        _In_    const CCachedBlockSlot&     slot,
                                        _Inout_ ICachedBlockSlab** const    ppcbsJournal );

        ERR ErrStartThreadpool();
        void StopThreadpool();

        void ReleaseThreadpoolState( _Inout_ TP_WORK** const pptpw );

        static void NTAPI ThreadpoolAsyncIOWorker_( _Inout_ TP_CALLBACK_INSTANCE* const ptpcbi,
                                                    _Inout_ void* const                 pvContext,
                                                    _Inout_ TP_WORK* const              ptpw  );
        void ThreadpoolAsyncIOWorker(   _Inout_ TP_CALLBACK_INSTANCE* const                     ptpcbi,
                                        _In_    CHashedLRUKCacheThreadLocalStorage<I>* const    pctls );

        static void NTAPI ThreadpoolSlabWriteBackWorker_(   _Inout_ TP_CALLBACK_INSTANCE* const ptpcbi,
                                                            _Inout_ void* const                 pvContext,
                                                            _Inout_ TP_WORK* const              ptpw  );
        void ThreadpoolSlabWriteBackWorker( _Inout_ TP_CALLBACK_INSTANCE* const ptpcbi );

        void ReportMiss(    _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                            _In_ const QWORD                                ibCachedBlock,
                            _In_ const BOOL                                 fRead,
                            _In_ const BOOL                                 fCacheIfPossible );

        void ReportHit( _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                        _In_ const QWORD                                ibCachedBlock,
                        _In_ const BOOL                                 fRead,
                        _In_ const BOOL                                 fCacheIfPossible );

        void ReportUpdate(  _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                            _In_ const QWORD                                ibCachedBlock );

        void ReportWrite(   _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                            _In_ const QWORD                                ibCachedBlock,
                            _In_ const BOOL                                 fReplacementPolicy );

        void ReportEvict(   _In_opt_    CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                            _In_        const QWORD                                ibCachedBlock,
                            _In_        const BOOL                                 fReplacementPolicy );

        QWORD CbChunkPerSlab() const
        { 
            return ( m_pch->CbCachingFilePerSlab() / ( CCachedBlockChunk::Ccbl() * cbCachedBlock ) ) * sizeof( CCachedBlockChunk );
        }

    private:

        static const CCachedBlockId                                                         s_cbidInvalid;

        CHashedLRUKCacheHeader*                                                             m_pch;
        CCachingFileWrapper                                                                 m_ffCaching;
        CCachedBlockPresenceFilter*                                                         m_pcbpf;
        IJournal*                                                                           m_pj;
        ICachedBlockWriteCountsManager*                                                     m_pcbwcm;
        ICachedBlockSlabManager*                                                            m_pcbsmHash;
        ICachedBlockSlabManager*                                                            m_pcbsmJournal;

        CSemaphore                                                                          m_semQuiesceStateAccess;
        CMeteredSection                                                                     m_msStateAccess;
        CManualResetSignal                                                                  m_msigStateAccess0;
        CManualResetSignal                                                                  m_msigStateAccess1;
        CManualResetSignal*                                                                 m_rgpmsigStateAccess[ 2 ];
        CCriticalSection                                                                    m_critSuspendedThreads;
        CArray<CHashedLRUKCacheThreadLocalStorage<I>*>                                      m_rgarraySuspendedThreads[ 2 ];
        ERR                                                                                 m_errSuspendBlockedThreads;

        size_t                                                                              m_ccrefJournalSlab;
        volatile int*                                                                       m_rgcrefJournalSlab;

        CCriticalSection                                                                    m_critClusterWrites;
        CMeteredSection                                                                     m_msClusterWrites;
        volatile ERR                                                                        m_errClusterWrites;

        BYTE*                                                                               m_rgbCluster;
        CClusterReferenceHash                                                               m_clusterReferenceHash;
        JournalPosition                                                                     m_jposReplayBeforeMount;
        JournalPosition                                                                     m_jposDurableBeforeMount;
        JournalPosition                                                                     m_jposEnqueueMin;
        ERR                                                                                 m_errVisitingJournalEntries;
        ERR                                                                                 m_errJournalCorruption;
        JournalPosition                                                                     m_jposCorruption;
        JournalPosition                                                                     m_jposRepair;
        JournalPosition                                                                     m_jposRepairActual;
        CInvasiveList<CQueuedJournalEntry, CQueuedJournalEntry::OffsetOfILE>                m_ilQueuedJournalEntry;
        JournalPosition                                                                     m_jposRedo;
        BOOL                                                                                m_fRecovered;

        CInitOnce<ERR, decltype( &ErrInitSlabWriteBackHash_ ), THashedLRUKCache<I>* const>  m_initOnceSlabWriteBackHash;
        CSlabWriteBackHash                                                                  m_slabWriteBackHash;
        CCriticalSection                                                                    m_critSlabsToWriteBack;
        CCountedInvasiveList<CSlabWriteBack, CSlabWriteBack::OffsetOfILEJposMin>            m_ilSlabsToWriteBackByJposMin;
        TP_WORK*                                                                            m_ptpwSlabWriteBack;
        CSemaphore                                                                          m_semSlabWriteBackWorkerRequest;
        CSemaphore                                                                          m_semSlabWriteBackWorkerExecute;
        CCriticalSection                                                                    m_critSlabWrites;
        CMeteredSection                                                                     m_msSlabWrites;
        volatile ERR                                                                        m_errSlabWrites;
        const CSlabWriteCompletionContext                                                   m_rgswcc[ 2 ];
        CInvasiveList<CSlabWriteBack, CSlabWriteBack::OffsetOfILESave>                      m_ilSlabWriteBacksPending;
        CInvasiveList<CSlabWriteBack, CSlabWriteBack::OffsetOfILESave>                      m_ilSlabWriteBacksCompleted;
        volatile JournalPosition                                                            m_jposReplayWriteCounts;

        TP_CALLBACK_ENVIRON                                                                 m_tpcbe;
        TP_POOL*                                                                            m_ptp;
        TP_CLEANUP_GROUP*                                                                   m_ptpcg;
};

template< class I >
THashedLRUKCache<I>::~THashedLRUKCache()
{
    StopThreadpool();

    m_semSlabWriteBackWorkerRequest.Acquire();
    m_semSlabWriteBackWorkerRequest.Acquire();
    m_semSlabWriteBackWorkerExecute.Acquire();

    for ( size_t icrefJournalSlab = 0; icrefJournalSlab < m_ccrefJournalSlab; icrefJournalSlab++ )
    {
        Assert( !m_rgcrefJournalSlab[ icrefJournalSlab ] );
    }

    ReleaseCompletedSlabWriteBacks();
    TermSlabWriteBackHash();
    delete[] m_rgcrefJournalSlab;
    delete m_pcbsmJournal;
    delete m_pcbsmHash;
    delete m_pcbwcm;
    delete m_pj;
    delete m_pcbpf;
    delete m_pch;

    PffCaching()->SetNoFlushNeeded();
}

//  The Hashed LRUK Cache file is laid out like this:
//
//    CCacheHeader
//    CHashedLRUKCacheHeader
//    CJournalSegmentHeader array
//    CCachedBlockChunk array
//      Composed of X slabs of cached blocks which are divided into two portions:  hash and journal pool
//      Each slab is comprised of Y CCachedBlockChunk
//        Each CCachedBlockChunk contains Z CCachedBlock
//          Each CCachedBlock contains a pointer to cluster that can hold the cached file block
//    CCachedBlockWriteCounts array
//      Each write state contains a CachedBlockWriteType for each CCachedBlockChunk
//    Clusters holding the cached file blocks
//
//  The parameters of this layout are preserved in CHashedLRUKCacheHeader.

template< class I >
ERR THashedLRUKCache<I>::ErrCreate()
{
    ERR                     err                         = JET_errSuccess;
    const QWORD             cbCachingFileMaxConfigured  = Pcconfig()->CbMaximumSize();
    const QWORD             cbCachingFileMax            = rounddn( cbCachingFileMaxConfigured, cbCachedBlock );
    const QWORD             cbCachingFilePerChunk       = CCachedBlockChunk::Ccbl() * cbCachedBlock;
    const QWORD             cbCachingFilePerSlab        = max( cbCachingFilePerChunk, roundup( Pcconfig()->CbCachingFilePerSlab(), cbCachingFilePerChunk ) );
    const QWORD             cbChunkPerSlab              = cbCachingFilePerSlab / cbCachingFilePerChunk * sizeof( CCachedBlockChunk );
    const QWORD             cbCachedFilePerSlab         = max( cbCachedBlock, roundup( Pcconfig()->CbCachedFilePerSlab(), cbCachedBlock ) );
    const QWORD             cbJournalSegmentMax         = max( 16 * cbCachedBlock, rounddn( Pcconfig()->CbJournalSegmentsMaximumSize(), cbCachedBlock ) );
    const QWORD             cbJournalClusterMax         = max( cbCachingFilePerSlab, roundup( Pcconfig()->CbJournalClustersMaximumSize(), cbCachingFilePerSlab ) );
    QWORD                   cbCachingFile               = 0;
    QWORD                   ibCachingFileHeader         = 0;
    DWORD                   cbCachingFileHeader         = 0;
    QWORD                   ibHeader                    = 0;
    DWORD                   cbHeader                    = 0;
    QWORD                   ibJournal                   = 0;
    QWORD                   cbJournal                   = 0;
    QWORD                   ibChunk                     = 0;
    QWORD                   cbChunk                     = 0;
    QWORD                   ccbwcs                      = 0;
    QWORD                   ibWriteCounts               = 0;
    QWORD                   cbWriteCounts               = 0;
    QWORD                   ibClusters                  = 0;
    QWORD                   cbClusters                  = 0;
    QWORD                   cSlab                       = 0;
    QWORD                   cSlabHash                   = 0;
    QWORD                   cSlabJournal                = 0;
    QWORD                   ibChunkHash                 = 0;
    QWORD                   cbChunkHash                 = 0;
    QWORD                   ibClustersHash              = 0;
    QWORD                   cbClustersHash              = 0;
    QWORD                   icbwcHash                   = 0;
    QWORD                   ibChunkJournal              = 0;
    QWORD                   cbChunkJournal              = 0;
    QWORD                   ibClustersJournal           = 0;
    QWORD                   cbClustersJournal           = 0;
    QWORD                   icbwcJournal                = 0;
    TraceContextScope       tcScope( iorpBlockCache );
    CHashedLRUKCacheHeader* pch                         = NULL;

    //  compute the offset range of the CCacheHeader

    ibCachingFileHeader = cbCachingFile;
    cbCachingFileHeader = sizeof( CCacheHeader );

    cbCachingFile = ibCachingFileHeader + cbCachingFileHeader;
    if ( cbCachingFile > cbCachingFileMax )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall1" ) );
    }

    //  compute the offset range of the CHashedLRUKCacheHeader

    ibHeader = cbCachingFile;
    cbHeader = sizeof( CHashedLRUKCacheHeader );

    cbCachingFile = ibHeader + cbHeader;
    if ( cbCachingFile > cbCachingFileMax )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall2" ) );
    }

    //  compute the offset range of the CJournalSegmentHeader Array backing the journal

    ibJournal = cbCachingFile;
    cbJournal = rounddn( cbJournalSegmentMax, cbJournalSegment );

    cbCachingFile = ibJournal + cbJournal;
    if ( cbCachingFile > cbCachingFileMax )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall3" ) );
    }

    //  compute the offset range of the CCachedBlockChunk array backing the slabs
    //
    //  to avoid complexity we will reserve enough to represent the rest of the file but we will use less than that

    ibChunk = cbCachingFile;
    cbChunk = ( roundup( cbCachingFileMax - ibChunk, cbCachingFilePerSlab ) / cbCachingFilePerSlab ) * cbChunkPerSlab;

    cbCachingFile = ibChunk + cbChunk;
    if ( cbCachingFile > cbCachingFileMax )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall4" ) );
    }

    //  compute the offset range of the write counts protecting the CCachedBlockChunk array from lost writes

    ccbwcs = roundup( cbChunk / sizeof( CCachedBlockChunk ), CCachedBlockWriteCounts::Ccbwc() ) / CCachedBlockWriteCounts::Ccbwc();

    ibWriteCounts = cbCachingFile;
    cbWriteCounts = 2 * ccbwcs * sizeof( CCachedBlockWriteCounts );

    cbCachingFile = ibWriteCounts + cbWriteCounts;
    if ( cbCachingFile > cbCachingFileMax )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall5" ) );
    }

    //  compute the offset range of the clusters

    ibClusters = cbCachingFile;
    cbClusters = rounddn( cbCachingFileMax - ibClusters, cbCachingFilePerSlab );

    cbCachingFile = ibClusters + cbClusters;
    if ( cbCachingFile > cbCachingFileMax )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall6" ) );
    }

    //  compute the number of slabs used for the hash vs journal

    cSlab = cbClusters / cbCachingFilePerSlab;

    cSlabJournal = max( 1, cbJournalClusterMax / cbCachingFilePerSlab );
    cSlabHash = cSlab - cSlabJournal;

    if ( cSlabHash == 0 || cSlabHash > cSlab )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall7" ) );
    }

    if ( cSlabJournal == 0 || cSlabJournal > cSlab )
    {
        //  not enough storage was granted to create the cache
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTooSmall8" ) );
    }

    //  compute the offset ranges for the hash and journal chunks and clusters

    ibChunkHash = ibChunk;
    cbChunkHash = cSlabHash * cbChunkPerSlab;
    ibClustersHash = ibClusters;
    cbClustersHash = cSlabHash * cbCachingFilePerSlab;
    icbwcHash = 0;

    ibChunkJournal = ibChunkHash + cbChunkHash;
    cbChunkJournal = cSlabJournal * cbChunkPerSlab;
    ibClustersJournal = ibClustersHash + cbClustersHash;
    cbClustersJournal = cSlabJournal * cbCachingFilePerSlab;
    icbwcJournal = cbChunkHash / sizeof( CCachedBlockChunk );

    if ( ibChunkJournal + cbChunkJournal > ibChunk + cbChunk )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheChunkAllocation" ) );
    }

    //  set the caching file size

    Call( PffCaching()->ErrSetSize( *tcScope, cbCachingFile, fFalse, qosIONormal ) );

    //  initialize the caching file header

    Call( CHashedLRUKCacheHeader::ErrCreate(    ibJournal,
                                                cbJournal, 
                                                ibChunkHash,
                                                cbChunkHash,
                                                ibChunkJournal,
                                                cbChunkJournal,
                                                ibWriteCounts,
                                                cbWriteCounts,
                                                ccbwcs,
                                                ibClustersHash,
                                                cbClustersHash, 
                                                ibClustersJournal,
                                                cbClustersJournal,
                                                cbCachingFilePerSlab,
                                                cbCachedFilePerSlab,
                                                icbwcHash,
                                                icbwcJournal,
                                                cbCachingFileMaxConfigured,
                                                &pch ) );

    //  write the caching file header

    Call( PffCaching()->ErrIOWrite( *tcScope,
                                    ibHeader,
                                    cbHeader,
                                    (const BYTE*)pch,
                                    qosIONormal, 
                                    NULL, 
                                    NULL,
                                    NULL ) );

    //  flush the caching file

    Call( PffCaching()->ErrFlushFileBuffers( iofrBlockCache ) );

HandleError:
    delete pch;
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrMount()
{
    ERR err = JET_errSuccess;

    //  init the cache

    Call( ErrInit() );

    //  recover the cache by replaying operations from the journal

    Call( ErrRecover() );

HandleError:
    return err;
}


template< class I >
ERR THashedLRUKCache<I>::ErrPrepareToDismount()
{
    ERR err = JET_errSuccess;

    //  flush our state for all files

    Call( ErrFlush() );

HandleError:
    return err;
}

template< class I >
BOOL THashedLRUKCache<I>::FEnabled()
{
    //  if the cache base claims the cache isn't enabled then it is not enabled

    if ( !THashedLRUKCacheBase<I>::FEnabled() )
    {
        return fFalse;
    }

    //  if the cache maximum size has changed significantly then it doesn't match

    const QWORD cbMaxExpected   = Pcconfig()->CbMaximumSize();
    const QWORD cbMaxActual     = m_pch->CbMaxConfigured();
    const QWORD dcbMin          = 1 * cbMaxActual / 1000;

    if ( cbMaxExpected > cbMaxActual + dcbMin )
    {
        return fFalse;
    }

    if ( cbMaxExpected + dcbMin < cbMaxActual )
    {
        return fFalse;
    }

    return fTrue;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDump( _In_ CPRINTF* const pcprintf )
{
    ERR err                 = JET_errSuccess;
    ERR errInit             = JET_errSuccess;
    ERR errAnalyzeJournal   = JET_errSuccess;

    //  init the cache

    errInit = ErrInit();

    //  dump the header

    if ( !m_pch )
    {
        (*pcprintf)( OSFormat( "Failed to load the header (%d).\n", errInit ) );
    }
    else
    {
        Call( m_pch->ErrDump( pcprintf ) );
    }

    //  dump the journal

    if ( !m_pj )
    {
        (*pcprintf)( OSFormat( "Failed to mount the journal (%d).\n", errInit ) );
    }
    else
    {
        //  analyze the journal

        errAnalyzeJournal = ErrAnalyzeJournal();

        //  dump the journal metadata

        Call( ErrDumpJournalMetadata( pcprintf ) );

        //  dump the valid journal entries

        Call( ErrDumpJournalEntries( pcprintf ) );

        if ( m_errJournalCorruption < JET_errSuccess )
        {
            (*pcprintf)( OSFormat(  "The journal is corrupt at 0x%016I64x (%d).\n",
                                    QWORD( m_jposCorruption ),
                                    m_errJournalCorruption ) );
        }
    }

    //  dump the write counts

    if ( !m_pcbwcm )
    {
        (*pcprintf)( OSFormat( "Failed to load the write counts (%d).\n", errInit ) );
    }
    else
    {
        Call( ErrDumpWriteCounts( pcprintf ) );
    }

    //  dump the hash slabs

    if ( !m_pcbsmHash )
    {
        (*pcprintf)( OSFormat( "Failed to load the hash chunks (%d).\n", errInit ) );
    }
    else
    {
        Call( ErrDumpHashChunks( pcprintf ) );
    }

    //  dump the journal slabs

    if ( !m_pcbsmJournal )
    {
        (*pcprintf)( OSFormat( "Failed to load the journal chunks (%d).\n", errInit ) );
    }
    else
    {
        Call( ErrDumpJournalChunks( pcprintf ) );
    }

HandleError:
    ReleaseJournalEntries();
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrFlush(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial )
{
    ERR                                 err         = JET_errSuccess;
    CHashedLRUKCachedFileTableEntry<I>* pcfte       = NULL;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    OSTrace(    JET_tracetagBlockCacheOperations,
                OSFormat(   "C=%s F=%s Flush Start",
                            OSFormatFileId( this ),
                            OSFormatFileId( pcfte->Pff() ) ) );

    //  flush our state for all files

    Call( ErrFlush() );

HandleError:
    if ( pcfte )
    {
        OSTrace(    JET_tracetagBlockCacheOperations,
                    OSFormat(   "C=%s F=%s Flush End",
                                OSFormatFileId( this ),
                                OSFormatFileId( pcfte->Pff() ) ) );
    }
    ReleaseCachedFile( &pcfte );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDestage(    _In_        const VolumeId                  volumeid,
                                        _In_        const FileId                    fileid,
                                        _In_        const FileSerial                fileserial,
                                        _In_opt_    const ICache::PfnDestageStatus  pfnDestageStatus,
                                        _In_opt_    const DWORD_PTR                 keyDestageStatus )
{
    using CIORangeLock = CHashedLRUKCachedFileTableEntry<I>::CIORangeLock;

    ERR                                                             err                 = JET_errSuccess;
    CHashedLRUKCachedFileTableEntry<I>*                             pcfte               = NULL;
    BOOL                                                            fRetryDestage       = fTrue;
    QWORD                                                           ibSlab              = 0;
    const QWORD                                                     ibSlabMin           = m_pch->IbChunkHash();
    const QWORD                                                     ibSlabMax           = ibSlabMin + m_pch->CbChunkHash();
    const QWORD                                                     cbSlab              = CbChunkPerSlab();
    BOOL                                                            fRetrySlab          = fFalse;
    const QWORD                                                     cSlab               = ( ibSlabMax - ibSlabMin ) / cbSlab;
    QWORD                                                           iSlabMost           = 0;
    ICachedBlockSlab*                                               pcbs                = NULL;
    CInvasiveList<CWriteBack, CWriteBack::OffsetOfILE>              ilWriteBack;
    CIORangeLock*                                                   piorl               = NULL;
    const QWORD                                                     cIOMax              = Pcconfig()->CIOMaxOutstandingDestage();
    const QWORD                                                     cbIOMax             = min(  Pcconfig()->CbIOMaxOutstandingDestage(),
                                                                                                m_pch->CbCachingFilePerSlab() / 2 );
    CCountedInvasiveList<CIORangeLock, CIORangeLock::OffsetOfILE>   ilIORangeLock;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    OSTrace(    JET_tracetagBlockCacheOperations,
                OSFormat(   "C=%s F=%s Destage Start",
                            OSFormatFileId( this ),
                            OSFormatFileId( pcfte->Pff() ) ) );

    //  walk every slab in the cache and write back all data for this file

    while ( fRetryDestage )
    {
        fRetryDestage = fFalse;

        for ( ibSlab = ibSlabMin; ibSlab < ibSlabMax; ibSlab += fRetrySlab ? 0 : cbSlab )
        {
            fRetrySlab = fFalse;

            //  update status

            if ( pfnDestageStatus )
            {
                iSlabMost = max( iSlabMost, ( ibSlab - ibSlabMin ) / cbSlab );
                pfnDestageStatus( (int)iSlabMost, (int)cSlab, keyDestageStatus );
            }

            //  get the slots we need to write back for this slab sorted by destination offset

            Call( ErrGetSlab( ibSlab, &pcbs ) );

            Call( CWriteBackSlotsSlabVisitor::ErrExecute( this, pcbs, pcfte, ilWriteBack ) );

            //  determine which write backs we can perform based on count, size, and available IO range locks

            CWriteBack* pwbNext = NULL;
            QWORD       ibStart = 0;
            QWORD       cbIO    = 0;

            for ( CWriteBack* pwb = ilWriteBack.PrevMost(); pwb; pwb = pwbNext )
            {
                pwbNext = ilWriteBack.Next( pwb );

                if ( !ilWriteBack.Prev( pwb ) || !ilWriteBack.Prev( pwb )->Offsets().FAdjacent( pwb->Offsets() ) )
                {
                    ibStart = pwb->Offsets().IbStart();
                }

                if ( !ilWriteBack.Next( pwb ) || !pwb->Offsets().FAdjacent( ilWriteBack.Next( pwb )->Offsets() ) )
                {
                    BOOL            fSkipWriteBack  = fFalse;
                    const COffsets  offsets         = COffsets( ibStart, pwb->Offsets().IbEnd() );

                    if ( ilIORangeLock.Count() > 0 && ilIORangeLock.Count() > cIOMax )
                    {
                        fSkipWriteBack = fTrue;
                    }

                    if ( ilIORangeLock.Count() > 0 && cbIO + offsets.Cb() > cbIOMax )
                    {
                        fSkipWriteBack = fTrue;
                    }

                    if ( !fSkipWriteBack )
                    {
                        Alloc( piorl = new CIORangeLock( pcfte, offsets ) );

                        fSkipWriteBack = !pcfte->FTryRequestIORangeLock( piorl, fFalse );
                    }

                    if ( fSkipWriteBack )
                    {
                        if ( piorl && piorl->FLocked() )
                        {
                            piorl->Release();
                        }
                        delete piorl;

                        ilWriteBack.Remove( pwb );
                        CWriteBack::Release( &pwb );

                        fRetrySlab = fTrue;
                    }
                    else
                    {
                        cbIO += piorl->Offsets().Cb();

                        ilIORangeLock.InsertAsNextMost( piorl );
                        piorl = NULL;
                    }
                }
            }

            //  if we couldn't find anything we could write back then move on to the next slab.  if there is still
            //  dirty data for this slab then still move on to avoid deadlooping but we will need to retry destage

            if ( ilWriteBack.FEmpty() )
            {
                ReleaseSlab( JET_errSuccess, &pcbs );
                fRetryDestage = fRetrySlab;
                fRetrySlab = fFalse;
                continue;
            }

            //  request all the reads we need to complete the write backs

            for ( CWriteBack* pwb = ilWriteBack.PrevMost(); pwb; pwb = ilWriteBack.Next( pwb ) )
            {
                pwb->StartRead();
            }

            Call( PffCaching()->ErrIOIssue() );

            //  write them back as they complete

            for ( CWriteBack* pwb = ilWriteBack.PrevMost(); pwb; pwb = ilWriteBack.Next( pwb ) )
            {
                pwb->StartWrite();
            }

            ReleaseSlab( JET_errSuccess, &pcbs );

            Call( pcfte->Pff()->ErrIssue( iomCacheWriteThrough ) );

            //  wait for the writes to succeed

            for ( CWriteBack* pwb = ilWriteBack.PrevMost(); pwb; pwb = ilWriteBack.Next( pwb ) )
            {
                pwb->WaitForWriteCompletion();
            }

            //  flush the cached file and mark the write backs as durable

            Call( pcfte->Pff()->ErrFlush( iofrBlockCache, iomCacheWriteThrough ) );

            for ( CWriteBack* pwb = ilWriteBack.PrevMost(); pwb; pwb = ilWriteBack.Next( pwb ) )
            {
                pwb->CompleteFlush( JET_errSuccess );
            }

            //  check for write back errors

            for ( CWriteBack* pwb = ilWriteBack.PrevMost(); pwb; pwb = ilWriteBack.Next( pwb ) )
            {
                Call( pwb->ErrComplete() )
            }

            //  mark all the slots we successfully wrote back as clean

            Call( ErrGetSlab( ibSlab, &pcbs ) );

            Call( CCleanSlotsSlabVisitor::ErrExecute( pcbs, &ilWriteBack ) );

            Call( ErrUpdateSlab( &pcbs ) );
            ReleaseSlab( JET_errSuccess, &pcbs );

            //  release our IO range locks

            while ( piorl = ilIORangeLock.PrevMost() )
            {
                ilIORangeLock.Remove( piorl );
                piorl->Release();
                delete piorl;
            }

            //  release our write backs

            CWriteBack::Release( ilWriteBack );
        }
    }

    //  verify that the destage wrote out all unpinned data

    for ( ibSlab = m_pch->IbChunkHash(); ibSlab < m_pch->IbChunkHash() + m_pch->CbChunkHash(); ibSlab += cbSlab )
    {
        Call( ErrGetSlab( ibSlab, &pcbs ) );
        Call( CVerifyDestageSlabVisitor::ErrExecute( this, pcbs, pcfte ) );
        ReleaseSlab( JET_errSuccess, &pcbs );
    }

HandleError:
    if ( piorl )
    {
        piorl->Release();
        delete piorl;
    }
    while ( piorl = ilIORangeLock.PrevMost() )
    {
        ilIORangeLock.Remove( piorl );
        piorl->Release();
        delete piorl;
    }
    CWriteBack::Release( ilWriteBack );
    ReleaseSlab( err, &pcbs );
    if ( pcfte )
    {
        OSTrace(    JET_tracetagBlockCacheOperations,
                    OSFormat(   "C=%s F=%s Destage End err=%d",
                                OSFormatFileId( this ),
                                OSFormatFileId( pcfte->Pff() ),
                                err ) );
    }
    ReleaseCachedFile( &pcfte );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrInvalidate( _In_ const VolumeId     volumeid,
                                        _In_ const FileId       fileid,
                                        _In_ const FileSerial   fileserial,
                                        _In_ const QWORD        ibOffset,
                                        _In_ const QWORD        cbData )
{
    ERR                                                 err                 = JET_errSuccess;
    const QWORD                                         ibStart             = ibOffset;
    const QWORD                                         ibEnd               = ibOffset >= qwMax - cbData ? qwMax : ibOffset - 1 + cbData;
    COffsets                                            offsets             = COffsets( ibStart, ibEnd );
    CHashedLRUKCachedFileTableEntry<I>*                 pcfte               = NULL;
    CHashedLRUKCachedFileTableEntry<I>::CIORangeLock*   piorl               = NULL;
    QWORD                                               cbCachedFile        = 0;
    const QWORD                                         cbSlab              = CbChunkPerSlab();
    QWORD                                               cSlab               = 0;
    size_t                                              cSlabBitmap         = 0;
    size_t                                              cbSlabBitmap        = 0;
    BYTE*                                               rgbSlabBitmap       = NULL;
    IBitmapAPI*                                         pbmSlab             = NULL;
    QWORD                                               cSlabInvalidated    = 0;
    ICachedBlockSlab*                                   pcbs                = NULL;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    OSTrace(    JET_tracetagBlockCacheOperations,
                OSFormat(   "C=%s F=%s Invalidate ib=%llu cb=%llu Start",
                            OSFormatFileId( this ),
                            OSFormatFileId( pcfte->Pff() ),
                            ibOffset,
                            cbData ) );

    //  ignore empty requests

    if ( !cbData )
    {
        Error( JET_errSuccess );
    }

    //  wait for an IO range lock for the offsets to invalidate

    piorl = new( _malloca( sizeof( *piorl ) ) ) CHashedLRUKCachedFileTableEntry<I>::CIORangeLock( pcfte, offsets );
    pcfte->AcquireIORangeLock( piorl );

    //  if this request is not aligned to cached file blocks then fail

    if ( offsets.IbStart() % pcfte->CbBlockSize() != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( offsets.IbEnd() != qwMax && offsets.Cb() % pcfte->CbBlockSize() != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  update the sparse map of the file to indicate that the caller is invalidating these offsets.  this will force
    //  the next access to these offsets to be write through.  this preserves the file's space allocation meta-data so
    //  that it is updated identically to that of an uncached file

    Call( pcfte->ErrUpdateSparseMapForInvalidate( offsets ) );

    //  determine the current cached file size.  we only need to invalidate up to that file size

    Call( pcfte->Pff()->ErrSize( &cbCachedFile, IFileAPI::filesizeLogical ) );

    if ( !cbCachedFile )
    {
        Error( JET_errSuccess );
    }

    offsets = COffsets( ibOffset, min( ibEnd, roundup( cbCachedFile, cbCachedBlock ) - 1 ) );

    //  allocate a bitmap that will help us to determine when we have invaliated a given slab and invalidated all slabs
    //  so that we can cap the cost of invalidating large offset ranges.  this isn't trivial because we don't know the
    //  relationship between the cached blocks and the slabs

    cSlab = m_pch->CbChunkHash() / cbSlab;
    cSlabBitmap = roundup( (size_t)cSlab, CHAR_BIT );
    cbSlabBitmap = (size_t)CbFromCbit( cSlabBitmap );
    Alloc( rgbSlabBitmap = new BYTE[ cbSlabBitmap ] );
    Alloc( pbmSlab = new CFixedBitmap( rgbSlabBitmap, cbSlabBitmap ) );

    //  loop through the invalidate by cached block potentially crossing many cached file blocks

    for (   QWORD ibCachedBlock = offsets.IbStart();
            ibCachedBlock <= offsets.IbEnd() && cSlabInvalidated < cSlab;
            ibCachedBlock += cbCachedBlock )
    {
        QWORD               ibSlab          = 0;
        CCachedBlockId      cbid;
        BOOL                fPossiblyCached = fFalse;

        //  determine if we are likely to have this cached block

        Call( ErrIsPossiblyCached( pcfte, ibCachedBlock, &ibSlab, &cbid, &fPossiblyCached ) );

        //  if the cached block is not possibly cached then skip this offset

        if ( !fPossiblyCached )
        {
            //  except if our offset range is so large that it is cheaper to visit every slab

            if ( offsets.Cb() / cbCachedBlock < cSlab )
            {
                continue;
            }
        }

        //  change to the slab containing this block

        Call( ErrChangeSlabs( ibSlab, &pcbs ) );

        //  determine if we have invalidated this slab previously

        size_t  iSlab;
        BOOL    fSlabInvalidated;

        iSlab = (size_t)( ( ibSlab - m_pch->IbChunkHash() ) / cbSlab );
        Call( ErrToErr<IBitmapAPI>( pbmSlab->ErrGet( iSlab, &fSlabInvalidated ) ) );
        if ( !fSlabInvalidated )
        {
            Call( ErrToErr<IBitmapAPI>( pbmSlab->ErrSet( iSlab, fTrue ) ) );
        }

        //  if we have already invalidated this slab then skip this offset

        if ( fSlabInvalidated )
        {
            continue;
        }

        //  invalidate all matching cached blocks from this slab

        Call( CInvalidateSlabVisitor::ErrExecute( this, &pcbs, pcfte, offsets ) );

        //  note that we have invalidated this slab

        cSlabInvalidated++;
    }

HandleError:
    ReleaseSlab( err, &pcbs );
    if ( piorl )
    {
        piorl->Release();
        _freea( piorl );
    }
    if ( pcfte )
    {
        OSTrace(    JET_tracetagBlockCacheOperations,
                    OSFormat(   "C=%s F=%s Invalidate ib=%llu cb=%llu End",
                                OSFormatFileId( this ),
                                OSFormatFileId( pcfte->Pff() ),
                                ibOffset,
                                cbData ) );
    }
    ReleaseCachedFile( &pcfte );
    delete pbmSlab;
    delete[] rgbSlabBitmap;
    Assert( !FAnyOpenSlab() );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrRead(   _In_                    const TraceContext&         tc,
                                    _In_                    const VolumeId              volumeid,
                                    _In_                    const FileId                fileid,
                                    _In_                    const FileSerial            fileserial,
                                    _In_                    const QWORD                 ibOffset,
                                    _In_                    const DWORD                 cbData,
                                    _Out_writes_( cbData )  BYTE* const                 pbData,
                                    _In_                    const OSFILEQOS             grbitQOS,
                                    _In_                    const ICache::CachingPolicy cp,
                                    _In_opt_                const ICache::PfnComplete   pfnComplete,
                                    _In_opt_                const DWORD_PTR             keyComplete )
{
    ERR                                 err         = JET_errSuccess;
    CHashedLRUKCachedFileTableEntry<I>* pcfte       = NULL;
    const BOOL                          fAsync      = pfnComplete != NULL;
    CRequest*                           prequest    = NULL;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  if this request is not aligned to cached file blocks then fail

    Call( ErrValidateOffsets( pcfte, ibOffset, cbData ) );

    //  setup our request context

    Alloc( prequest = new CRequest( fTrue,
                                    this,
                                    tc,
                                    &pcfte,
                                    ibOffset,
                                    cbData, 
                                    pbData,
                                    grbitQOS,
                                    cp,
                                    pfnComplete, 
                                    keyComplete ) );

    //  if this request is async then we must perform the request async to avoid blocking the request for sync reads to
    //  read our cache state.  otherwise, perform it inline to allow a sync request to be performed directly

    if ( fAsync )
    {
        Call( ErrEnqueue( &prequest ) );
    }
    else
    {
        Call( ErrSynchronousIO( prequest ) );
    }

HandleError:
    err = CRequest::ErrRelease( &prequest, err );
    ReleaseCachedFile( &pcfte );
    Assert( !FAnyOpenSlab() );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrWrite(  _In_                    const TraceContext&         tc,
                                    _In_                    const VolumeId              volumeid,
                                    _In_                    const FileId                fileid,
                                    _In_                    const FileSerial            fileserial,
                                    _In_                    const QWORD                 ibOffset,
                                    _In_                    const DWORD                 cbData,
                                    _In_reads_( cbData )    const BYTE* const           pbData,
                                    _In_                    const OSFILEQOS             grbitQOS,
                                    _In_                    const ICache::CachingPolicy cp,
                                    _In_opt_                const ICache::PfnComplete   pfnComplete,
                                    _In_opt_                const DWORD_PTR             keyComplete )
{
    ERR                                 err         = JET_errSuccess;
    CHashedLRUKCachedFileTableEntry<I>* pcfte       = NULL;
    const BOOL                          fAsync      = pfnComplete != NULL;
    CRequest*                           prequest    = NULL;

    //  get the cached file

    Call( ErrGetCachedFile( volumeid, fileid, fileserial, fFalse, &pcfte ) );

    //  if this request is not aligned to cached file blocks then fail

    Call( ErrValidateOffsets( pcfte, ibOffset, cbData ) );

    //  setup our request context

    Alloc( prequest = new CRequest( fFalse,
                                    this,
                                    tc,
                                    &pcfte,
                                    ibOffset,
                                    cbData, 
                                    pbData,
                                    grbitQOS,
                                    cp,
                                    pfnComplete, 
                                    keyComplete ) );

    //  if this request is async then we must perform the request async to avoid blocking the request for sync reads to
    //  read our cache state.  otherwise, perform it inline to allow a sync request to be performed directly

    if ( fAsync )
    {
        Call( ErrEnqueue( &prequest ) );
    }
    else
    {
        Call( ErrSynchronousIO( prequest ) );
    }

HandleError:
    err = CRequest::ErrRelease( &prequest, err );
    ReleaseCachedFile( &pcfte );
    Assert( !FAnyOpenSlab() );
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrIssue(  _In_ const VolumeId     volumeid,
                                    _In_ const FileId       fileid,
                                    _In_ const FileSerial   fileserial )
{
    //  ErrIssue is not expected to fail and we currently track requested IO per thread not per file so we don't
    //  validate or use the file id

    Unused( volumeid );
    Unused( fileid );
    Unused( fileserial );

    //  issue requested IO for all files

    Issue();

    //  ErrIssue is not expected to fail

    return JET_errSuccess;
}

template< class I >
THashedLRUKCache<I>::THashedLRUKCache(  _In_    IFileSystemFilter* const            pfsf,
                                        _In_    IFileIdentification* const          pfident,
                                        _In_    IFileSystemConfiguration* const     pfsconfig, 
                                        _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                                        _Inout_ ICacheConfiguration** const         ppcconfig,
                                        _In_    ICacheTelemetry* const              pctm,
                                        _Inout_ IFileFilter** const                 ppffCaching,
                                        _Inout_ CCacheHeader** const                ppch )
            : THashedLRUKCacheBase<I>(  pfsf,
                                        pfident, 
                                        pfsconfig, 
                                        ppbcconfig, 
                                        ppcconfig, 
                                        pctm, 
                                        ppffCaching, 
                                        ppch ),
                m_pch( NULL ),
                m_ffCaching( THashedLRUKCacheBase<I>::PffCaching() ),
                m_pcbpf( NULL ),
                m_pj( NULL ),
                m_pcbwcm( NULL ),
                m_pcbsmHash( NULL ),
                m_pcbsmJournal( NULL ),
                m_semQuiesceStateAccess( CSyncBasicInfo( "THashedLRUKCache<I>::m_semQuiesceStateAccess" ) ),
                m_msigStateAccess0( CSyncBasicInfo( "THashedLRUKCache<I>::m_msigStateAccess0" ) ),
                m_msigStateAccess1( CSyncBasicInfo( "THashedLRUKCache<I>::m_msigStateAccess1" ) ),
                m_rgpmsigStateAccess
                {
                    &m_msigStateAccess0,
                    &m_msigStateAccess1,
                },
                m_critSuspendedThreads( CLockBasicInfo( CSyncBasicInfo( "THashedLRUKCache<I>::m_critSuspendedThreads" ), rankSuspendedThreads, 0 ) ),
                m_rgarraySuspendedThreads
                {
                    CArray<CHashedLRUKCacheThreadLocalStorage<I>*>(),
                    CArray<CHashedLRUKCacheThreadLocalStorage<I>*>(),
                },
                m_errSuspendBlockedThreads( JET_errSuccess ),
                m_ccrefJournalSlab( 0 ),
                m_rgcrefJournalSlab( NULL ),
                m_critClusterWrites( CLockBasicInfo( CSyncBasicInfo( "THashedLRUKCache<I>::m_critClusterWrites" ), rankClusterWrites, 0 ) ),
                m_errClusterWrites( JET_errSuccess ),
                m_rgbCluster( NULL ),
                m_clusterReferenceHash( rankClusterReferenceHash ),
                m_jposReplayBeforeMount( jposInvalid ),
                m_jposDurableBeforeMount( jposInvalid ),
                m_jposEnqueueMin( jposInvalid),
                m_errVisitingJournalEntries( JET_errSuccess ),
                m_errJournalCorruption( JET_errSuccess ),
                m_jposCorruption( jposInvalid ),
                m_jposRepair( jposInvalid ),
                m_jposRepairActual( jposInvalid ),
                m_jposRedo( jposInvalid ),
                m_fRecovered( fFalse ),
                m_slabWriteBackHash( rankSlabWriteBackHash ),
                m_critSlabsToWriteBack( CLockBasicInfo( CSyncBasicInfo( "THashedLRUKCache<I>::m_critSlabsToWriteBack" ), rankSlabsToWriteBack, 0 ) ),
                m_ptpwSlabWriteBack( NULL ),
                m_semSlabWriteBackWorkerRequest( CSyncBasicInfo( "THashedLRUKCache<I>::m_semSlabWriteBackWorkerRequest" ) ),
                m_semSlabWriteBackWorkerExecute( CSyncBasicInfo( "THashedLRUKCache<I>::m_semSlabWriteBackWorkerExecute" ) ),
                m_critSlabWrites( CLockBasicInfo( CSyncBasicInfo( "THashedLRUKCache<I>::m_critSlabWrites" ), rankSlabWrites, 0 ) ),
                m_errSlabWrites( JET_errSuccess ),
                m_rgswcc
                {
                    CSlabWriteCompletionContext( this, (CMeteredSection::Group)0 ),
                    CSlabWriteCompletionContext( this, (CMeteredSection::Group)1 )
                },
                m_jposReplayWriteCounts( jposInvalid ),
                m_tpcbe { },
                m_ptp( NULL ),
                m_ptpcg( NULL )
{
    m_semSlabWriteBackWorkerRequest.Release();
    m_semSlabWriteBackWorkerRequest.Release();
    m_semSlabWriteBackWorkerExecute.Release();
    m_semQuiesceStateAccess.Release();
    m_rgpmsigStateAccess[ m_msStateAccess.GroupActive() ]->Set();
}

template< class I >
ERR THashedLRUKCache<I>::ErrGetThreadLocalStorage( _Out_ CHashedLRUKCacheThreadLocalStorage<I>** const ppctls )
{
    ERR                                     err         = JET_errSuccess;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls       = NULL;
    TP_WORK*                                ptpwIssue   = NULL;

    *ppctls = NULL;

    Call( THashedLRUKCacheBase<I>::ErrGetThreadLocalStorage( &pctls ) );

    if ( !pctls->PtpwIssue() )
    {
        Alloc( ptpwIssue = CreateThreadpoolWork( ThreadpoolAsyncIOWorker_, pctls, &m_tpcbe ) );
        pctls->Initialize( this, &ptpwIssue );
    }

    *ppctls = pctls;
    pctls = NULL;

HandleError:
    ReleaseThreadpoolState( &ptpwIssue );
    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
    if ( err < JET_errSuccess )
    {
        CHashedLRUKCacheThreadLocalStorage<I>::Release( ppctls );
    }
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpJournalMetadata( _In_ CPRINTF* const pcprintf )
{
    ERR             err                     = JET_errSuccess;
    JournalPosition jposReplay              = jposInvalid;
    JournalPosition jposDurableForWriteBack = jposInvalid;
    JournalPosition jposDurable             = jposInvalid;
    JournalPosition jposAppend              = jposInvalid;
    JournalPosition jposFull                = jposInvalid;

    Call( m_pj->ErrGetProperties( &jposReplay, &jposDurableForWriteBack, &jposDurable, &jposAppend, &jposFull ) );

    (*pcprintf)( "Journal Metadata:\n" );
    (*pcprintf)( "\n" );
    (*pcprintf)( "                 jposReplay:  0x%016I64x\n", QWORD( jposReplay ) );
    (*pcprintf)( "    jposDurableForWriteBack:  0x%016I64x\n", QWORD( jposDurableForWriteBack ) );
    (*pcprintf)( "                jposDurable:  0x%016I64x\n", QWORD( jposDurable ) );
    (*pcprintf)( "                 jposAppend:  0x%016I64x\n", QWORD( jposAppend ) );
    (*pcprintf)( "                   jposFull:  0x%016I64x\n", QWORD( jposFull ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        (*pcprintf)( OSFormat( "Failed to dump the journal metadata (%d).\n", err ) );
    }
    (*pcprintf)( "\n" );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpJournalEntries( _In_ CPRINTF* const pcprintf )
{
    ERR err = JET_errSuccess;

    (*pcprintf)( "Journal Entries:\n" );
    (*pcprintf)( "\n" );

    //  walk the list of journal entries queued for redo

    for (   CQueuedJournalEntry* pqje = m_ilQueuedJournalEntry.PrevMost();
            pqje;
            pqje = m_ilQueuedJournalEntry.Next( pqje ) )
    {
        //  dump the journal entry

        Call( ErrDumpJournalEntry( pqje, pcprintf ) );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        (*pcprintf)( OSFormat( "Failed to dump the journal entries (%d).\n", err ) );
    }
    (*pcprintf)( "\n" );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpJournalEntry( _In_ const CQueuedJournalEntry* const pqje, _In_ CPRINTF* const pcprintf )
{
    ERR                 err     = JET_errSuccess;
    QWORD               ibSlab  = qwMax;
    ERR                 errSlab = JET_errSuccess;
    ICachedBlockSlab*   pcbs    = NULL;

    //  dump the position of the journal entry

    (*pcprintf)( "    0x%016I64x ", QWORD( pqje->Jpos() ) );

    //  dump the type and contents of the journal entry

    switch ( pqje->Pje()->Jetyp() )
    {
        case jetypCacheUpdate:
            (*pcprintf)( "CacheUpdate" );
            {
                const CCacheUpdateJournalEntry* pcuje = (const CCacheUpdateJournalEntry*)pqje->Pje();

                for ( ULONG icbu = 0; icbu < pcuje->Ccbu(); icbu++ )
                {
                    const CCachedBlockUpdate*   pcbu        = pcuje->Pcbu( icbu );
                    ERR                         errChunk    = JET_errSuccess;

                    if ( ibSlab != pcbu->IbSlab() )
                    {
                        ReleaseSlab( JET_errSuccess, &pcbs );
                        errSlab = ErrGetSlabForRecovery( pcbu->IbSlab(), &pcbs );
                        Call( ErrIgnoreVerificationErrors( errSlab ) );
                        ibSlab = pcbu->IbSlab();
                    }

                    if ( errSlab < JET_errSuccess )
                    {
                        Call( CChunkStatus::ErrExecute( pcbs, pcbu->Chno(), &errChunk ) );
                    }

                    if ( !pcbu->FSlotUpdated() )
                    {
                        if ( errChunk >= JET_errSuccess )
                        {
                            continue;
                        }
                    }

                    (*pcprintf)( "\n" );
                    (*pcprintf)(    "        %c%c ",
                                    pcbu->FSlotUpdated() ? 'U' : '_',
                                    pcbu->FClusterReference() ? 'R' : '_' );
                    CCachedBlockSlot::Dump( *pcbu, pcprintf, Pfident() );
                }
            }
            break;
        case jetypFlush:
            (*pcprintf)( "Flush" );
            break;
        default:
            (*pcprintf)( "Unknown (%d)", pqje->Pje()->Jetyp() );
            break;
    }

HandleError:
    (*pcprintf)( "\n" );
    ReleaseSlab( JET_errSuccess, &pcbs );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpWriteCounts( _In_ CPRINTF* const pcprintf )
{
    ERR err = JET_errSuccess;

    (*pcprintf)( "Write counts:\n" );

    Call( ErrDumpWriteCounts( m_pch->IbChunkHash(), m_pch->CbChunkHash(), m_pch->IcbwcHash(), pcprintf ) );
    Call( ErrDumpWriteCounts( m_pch->IbChunkJournal(), m_pch->CbChunkJournal(), m_pch->IcbwcJournal(), pcprintf ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        (*pcprintf)( OSFormat( "Failed to dump the write counts (%d).\n", err ) );
    }
    (*pcprintf)( "\n" );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpWriteCounts(    _In_ const QWORD    ibChunkBase,
                                                _In_ const QWORD    cbChunk,
                                                _In_ const QWORD    icbwcBase,
                                                _In_ CPRINTF* const pcprintf )
{
    ERR             err                 = JET_errSuccess;
    const size_t    cbCachedBlockChunk  = sizeof( CCachedBlockChunk );
    const size_t    cbWriteCounts       = sizeof( CCachedBlockWriteCounts );
    const QWORD     ccbwc               = cbChunk / cbCachedBlockChunk;
    const size_t    ccbwcPerPage        = CCachedBlockWriteCounts::Ccbwc();
    const size_t    ccbwcPerRow         = 8;

    for ( QWORD icbwc = icbwcBase; icbwc < icbwcBase + ccbwc; icbwc++ )
    {
        const QWORD             ibWriteCounts   = m_pch->IbWriteCounts() + ( icbwc / ccbwcPerPage ) * cbWriteCounts;
        const QWORD             ibChunk         = ibChunkBase + ( icbwc - icbwcBase ) * cbCachedBlockChunk;
        CachedBlockWriteCount   cbwc            = cbwcUnknown;

        Call( m_pcbwcm->ErrGetWriteCount( icbwc, &cbwc ) );

        if ( icbwc == icbwcBase || icbwc % ccbwcPerPage == 0 )
        {
            (*pcprintf)( "\n" );
            (*pcprintf)( "    0x%016I64x\n", QWORD( ibWriteCounts ) );
            (*pcprintf)( "\n" );
        }

        if ( icbwc == icbwcBase || icbwc % ccbwcPerRow == 0 )
        {
            if ( icbwc > icbwcBase )
            {
                (*pcprintf)( "\n" );
            }
            (*pcprintf)( "        0x%016I64x ", QWORD( ibChunk ) );
            for ( QWORD i = 0; i < icbwc % ccbwcPerRow; i++ )
            {
                (*pcprintf)( "           ", cbwc );
            }
        }

        (*pcprintf)( " 0x%08x", cbwc );
    }

HandleError:
    if ( ccbwc > 0 )
    {
        (*pcprintf)( "\n" );
    }
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpHashChunks( _In_ CPRINTF* const pcprintf )
{
    ERR err = JET_errSuccess;

    (*pcprintf)( "Hash chunks:\n" );

    Call( CChunkDumper::ErrExecute( this, m_pcbsmHash, pcprintf ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        (*pcprintf)( OSFormat( "Failed to dump the hash chunks (%d).\n", err ) );
    }
    (*pcprintf)( "\n" );
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrDumpJournalChunks( _In_ CPRINTF* const pcprintf )
{
    ERR err = JET_errSuccess;

    (*pcprintf)( "Journal chunks:\n" );

    Call( CChunkDumper::ErrExecute( this, m_pcbsmJournal, pcprintf ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        (*pcprintf)( OSFormat( "Failed to dump the journal chunks (%d).\n", err ) );
    }
    (*pcprintf)( "\n" );
    return err;
}

template< class I >
BOOL THashedLRUKCache<I>::CChunkDumper::FVisitSlab( _In_ const QWORD                ib,
                                                    _In_ const ERR                  errSlab,
                                                    _In_ ICachedBlockSlab* const    pcbs )
{
    ERR             err         = JET_errSuccess;
    CPRINTF* const  pcprintf    = m_pcprintf;

    (*pcprintf)( "\n" );
    (*pcprintf)( OSFormat( "    0x%016I64x  %d\n", QWORD( ib ), errSlab ) );

    Call( pcbs->ErrVisitSlots( FVisitSlot_, DWORD_PTR( this ) ) );

HandleError:
    return fTrue;
}

template< class I >
BOOL THashedLRUKCache<I>::CChunkDumper::FVisitSlot( _In_ const ERR                      errChunk,
                                                    _In_ const CCachedBlockSlotState&   slotstAccepted,
                                                    _In_ const CCachedBlockSlotState&   slotstCurrent )
{
    ERR             err         = JET_errSuccess;
    CPRINTF* const  pcprintf    = m_pcprintf;

    (*pcprintf)( "        " );

    if ( errChunk < JET_errSuccess && errChunk != JET_errReadLostFlushVerifyFailure )
    {
        if ( slotstCurrent.Slno() == (SlotNumber)0 )
        {
            (*pcprintf)( OSFormat(  "0x%016I64x,0x%01x,0x%02x  %d",
                                    QWORD( slotstCurrent.IbSlab() ),
                                    slotstCurrent.Chno(),
                                    slotstCurrent.Slno(),
                                    errChunk ) );
        }

        Call( errChunk );
    }


    CCachedBlockSlot::Dump( slotstCurrent, pcprintf, m_pc->Pfident() );
    if ( slotstCurrent.FSuperceded() )
    {
        (*pcprintf)( " (Superceded)" );
    }

HandleError:
    (*pcprintf)( "\n" );
    return fTrue;
}

template< class I >
ERR THashedLRUKCache<I>::ErrInit()
{
    ERR             err     = JET_errSuccess;
    ClusterNumber   clnoMin = clnoInvalid;
    ClusterNumber   clnoMax = clnoInvalid;

    //  read the header

    Call( CHashedLRUKCacheHeader::ErrLoad( Pfsconfig(), PffCaching(), sizeof( CCacheHeader ), &m_pch ) );

    //  init the cached block presence filter

    Call( CCachedBlockPresenceFilter::ErrInit( this, &m_pcbpf ) );

    //  mount the journal

    Call( ErrMountJournal( m_pch, &m_pj ) );
    Alloc( m_pj = new CJournalWrapper( this, &m_pj ) );

    //  load the write counts

    Call( CCachedBlockWriteCountsManager::ErrInit(  PffCaching(),
                                                    m_pch->IbWriteCounts(),
                                                    m_pch->CbWriteCounts(), 
                                                    m_pch->Ccbwcs(),
                                                    &m_pcbwcm ) );

    //  initialize the slab managers

    Assert( m_pch->IbClustersHash() + m_pch->CbClustersHash() == m_pch->IbClustersJournal() );
    clnoMin = (ClusterNumber)( m_pch->IbClustersHash() / cbCachedBlock );
    clnoMax = (ClusterNumber)( ( m_pch->IbClustersJournal() + m_pch->CbClustersJournal() ) / cbCachedBlock );

    Call( CCachedBlockSlabManager::ErrInit( PffCaching(),
                                            m_pch->CbCachingFilePerSlab(),
                                            m_pch->CbCachedFilePerSlab(),
                                            m_pch->IbChunkHash(),
                                            m_pch->CbChunkHash(),
                                            m_pcbwcm,
                                            m_pch->IcbwcHash(),
                                            clnoMin,
                                            clnoMax,
                                            &m_pcbsmHash ) );

    Call( CCachedBlockSlabManager::ErrInit( PffCaching(),
                                            m_pch->CbCachingFilePerSlab(),
                                            m_pch->CbCachedFilePerSlab(),
                                            m_pch->IbChunkJournal(),
                                            m_pch->CbChunkJournal(),
                                            m_pcbwcm,
                                            m_pch->IcbwcJournal(),
                                            clnoMin,
                                            clnoMax,
                                            &m_pcbsmJournal ) );

    //  start our thread pool

    Call( ErrStartThreadpool() );

    //  save our initialized resources

    m_ccrefJournalSlab = (size_t)( m_pch->CbChunkJournal() / CbChunkPerSlab() );
    Alloc( (void*)( m_rgcrefJournalSlab = new volatile int[ m_ccrefJournalSlab ] ) );
    for ( size_t icrefJournalSlab = 0; icrefJournalSlab < m_ccrefJournalSlab; icrefJournalSlab++ )
    {
        m_rgcrefJournalSlab[ icrefJournalSlab ] = 0;
    }

HandleError:
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrRecover()
{
    ERR err = JET_errSuccess;

    //  analyze the journal and journal clusters to find the valid extent of the journal

    Call( ErrAnalyzeJournal() );

    //  redo any remaining queued journal entries that we know will not be destroyed by the repair

    Call( ErrRedoJournalEntries() );

    //  indicate that recovery is successful

    m_fRecovered = fTrue;

HandleError:
    ReleaseJournalEntries();
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrMountJournal( _In_ CHashedLRUKCacheHeader* const pch, _Out_ IJournal** const ppj )
{
    ERR                             err             = JET_errSuccess;
    IJournalSegmentManager*         pjsm            = NULL;
    const size_t                    cbJournalCache  = (size_t)Pcconfig()->CbJournalSegmentsMaximumCacheSize();
    IJournal*                       pj              = NULL;

    *ppj = NULL;

    Call( CJournalSegmentManager::ErrMount( PffCaching(), pch->IbJournal(), pch->CbJournal(), &pjsm ) );
    Call( CJournal::ErrMount( PffCaching(), &pjsm, cbJournalCache, &pj ) );

    *ppj = pj;
    pj = NULL;

HandleError:
    delete pj;
    delete pjsm;
    if ( err < JET_errSuccess )
    {
        delete *ppj;
        *ppj = NULL;
    }
    return err;
}

template< class I >
ERR THashedLRUKCache<I>::ErrAnalyzeJournal( _In_ const BOOL fAll )
{
    ERR     err         = JET_errSuccess;
    BOOL    fTermHash   = fFalse;

    //  allocate temp storage to validate cluster references

    Alloc( m_rgbCluster = (BYTE*)PvOSMemoryPageAlloc( cbCachedBlock, NULL ) );

    //  init our cluster reference hash table

    if ( m_clusterReferenceHash.ErrInit( 5.0, 1.0 ) == CClusterReferenceHash::ERR::errOutOfMemory )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    fTermHash = fTrue;

    //  note the replay and durable pointers

    Call( m_pj->ErrGetProperties( &m_jposReplayBeforeMount, NULL, &m_jposDurableBeforeMount, NULL, NULL ) );

    //  determine the min journal entry to enqueue

    m_jposEnqueueMin = fAll ? (JournalPosition)0 : m_jposReplayBeforeMount;

    //  visit all entries in the journal.  this will fail if the journal is corrupt

    err = m_pj->ErrVisitEntries( FVisitJournalEntry_, (DWORD_PTR)this );
    if ( err < JET_errSuccess )
    {
        m_errJournalCorruption = m_errJournalCorruption < JET_errSuccess ? m_errJournalCorruption : err;
        m_jposCorruption = min( m_jposCorruption, 
                                m_ilQueuedJournalEntry.NextMost() ?
                                    m_ilQueuedJournalEntry.NextMost()->JposEnd() + 1 :
                                    m_jposEnqueueMin );
        Call( err );
    }

    //  if the journal encountered an error while visiting journal entries then fail

    Call( m_errVisitingJournalEntries );

    //  if there are any outstanding cluster references that failed validation after visiting the journal then the
    //  journal is corrupt

    Call( ErrCheckForUnresolvedFailedClusterReferences() );

    //  if we discovered a lost cluster write in the journal then repair it at that location.  this should be in the
    //  repairable portion of the journal.  other cluster write failures are journal corruptions and should have been
    //  caught already

    if ( m_jposRepair != jposInvalid )
    {
        Call( m_pj->ErrRepair( m_jposRepair, &m_jposRepairActual ) );
    }

    //  forget any journal entries destroyed by repairing the journal

    for (   CQueuedJournalEntry* pqje = m_ilQueuedJournalEntry.NextMost();
            pqje && m_jposRepairActual != jposInvalid && pqje->JposEnd() >= m_jposRepairActual;
            pqje = m_ilQueuedJournalEntry.NextMost() )
    {
        m_ilQueuedJournalEntry.Remove( pqje );
        delete pqje;
    }

HandleError:
    if ( fTermHash )
    {
        m_clusterReferenceHash.Term();
    }
    OSMemoryPageFree( m_rgbCluster );
    Assert( !FAnyOpenSlab() );
    return err;
}

template< class I >
BOOL THashedLRUKCache<I>::FVisitJournalEntry(   _In_ const JournalPosition  jpos,
                                                _In_ const JournalPosition  jposEnd,
                                                _In_ const CJournalBuffer   jb )
{
    ERR                             err         = JET_errSuccess;
    const CJournalEntry*            pje         = NULL;
    const CCacheUpdateJournalEntry* pcuje       = NULL;
    ULONG                           icbu        = 0;
    const CCachedBlockUpdate*       pcbu        = NULL;
    QWORD                           ibSlab      = qwMax;
    ICachedBlockSlab*               pcbs        = NULL;
    ERR                             errCluster  = JET_errSuccess;
    CQueuedJournalEntry*            pqje        = NULL;

    //  if this journal entry is not to be enqueued then ignore it

    if ( jpos < m_jposEnqueueMin )
    {
        Error( JET_errSuccess );
    }

    //  if this is an empty journal entry then ignore it

    if ( !jb.Cb() )
    {
        Error( JET_errSuccess );
    }

    //  get an uncompressed copy of the journal entry

    Call( CCompressedJournalEntry::ErrExtract( jb, &pje ) );

    //  inspect the journal entry by type

    switch ( pje->Jetyp() )
    {
        case jetypCacheUpdate:
            pcuje = (CCacheUpdateJournalEntry*)pje;
            for ( icbu = 0; icbu < pcuje->Ccbu(); icbu++ )
            {
                pcbu = pcuje->Pcbu( icbu );

                //  if we don't have the slab for this update yet then get it

                if ( ibSlab != pcbu->IbSlab() )
                {
                    //  release any currently open slab

                    ReleaseSlab( JET_errSuccess, &pcbs );

                    //  get the new slab by offset
                    //
                    //  NOTE:  this will not fail even if a chunk in the slab is corrupt

                    Call( ErrIgnoreVerificationErrors( ErrGetSlabForRecovery( pcbu->IbSlab(), &pcbs ) ) );
                    ibSlab = pcbu->IbSlab();
                }

                //  if this update contains a cluster reference then check the integrity of that cluster

                if ( pcbu->FClusterReference() )
                {
                    TraceContextScope tc( iorpBlockCache );

                    //  read the cluster to determine if the data is valid

                    errCluster = pcbs->ErrReadCluster( *pcbu, cbCachedBlock, m_rgbCluster );
                    if ( errCluster >= JET_errSuccess )
                    {
                        errCluster = pcbs->ErrVerifyCluster( *pcbu, cbCachedBlock, m_rgbCluster );
                    }
                    Call( ErrIgnoreVerificationErrors( errCluster ) );

                    //  if the cluster failed validation and we are past the durable portion of the journal then we have found
                    //  the tail of the journal based on our additional criteria beyond that of the journal itself.  this is
                    //  because we consider the cluster to be part of the journal itself.  a failure to validate the cluster is
                    //  interpreted as a torn write of the journal
                    //
                    //  NOTE:  this means that we must wait to redo a journal entry until it can no longer be truncated (and
                    //  thus destroyed) by repairing the journal at the torn write detected by this process

                    if ( errCluster < JET_errSuccess && jposEnd > m_jposDurableBeforeMount && m_jposRepair == jposInvalid )
                    {
                        m_jposRepair = jposEnd;
                    }

                    //  remember any cluster references that fail validation that are not subsequently followed by another
                    //  cluster reference that is valid.  if any of these are left over at the end then the journal is corrupt

                    if ( errCluster >= JET_errSuccess || jposEnd <= m_jposDurableBeforeMount )
                    {
                        Call( ErrProcessClusterReference( pcbu->Clno(), errCluster, jpos ) );
                    }
                }

                //  if this update contains an evict/invalidate of a cluster then forget any previous failed cluster
                //  reference.  this covers the case where a cluster is used, evicted, and then the cluster is reused
                //  but we lose the journal entry that indicates that it was reused because it wasn't durable

                else if ( pcbu->FSlotUpdated() && pcbu->Cbid().Cbno() != cbnoInvalid && !pcbu->FValid() )
                {
                    ProcessClusterInvalidate( pcbu->Clno() );
                }
            }
            break;

        default:
            break;
    }

    //  enqueue this journal entry

    Alloc( pqje = new CQueuedJournalEntry( jpos, jposEnd, &pje ) );
    m_ilQueuedJournalEntry.InsertAsNextMost( pqje );

HandleError:
    ReleaseSlab( JET_errSuccess, &pcbs );
    delete pje;
    m_errVisitingJournalEntries = m_errVisitingJournalEntries < JET_errSuccess ? m_errVisitingJournalEntries : err;
    return m_errVisitingJournalEntries >= JET_errSuccess && m_jposRepair == jposInvalid;
}

template<class I>
ERR THashedLRUKCache<I>::ErrProcessClusterReference(    _In_ const ClusterNumber    clno,
                                                        _In_ const ERR              errCluster,
                                                        _In_ const JournalPosition  jpos )
{
    ERR                             err                     = JET_errSuccess;
    CClusterReferenceKey            key;
    CClusterReferenceHash::CLock    lock;
    BOOL                            fWriteLocked            = fFalse;
    CClusterReferenceHash::ERR      errClusterReferenceHash = CClusterReferenceHash::ERR::errSuccess;
    CClusterReferenceEntry          entry;

    if ( errCluster < JET_errSuccess )
    {
        Error( JET_errSuccess );
    }

    key = CClusterReferenceKey( clno );
    m_clusterReferenceHash.WriteLockKey( key, &lock );
    fWriteLocked = fTrue;

    errClusterReferenceHash = m_clusterReferenceHash.ErrRetrieveEntry( &lock, &entry );
    if ( errClusterReferenceHash == CClusterReferenceHash::ERR::errSuccess )
    {
    }
    else
    {
        Assert( errClusterReferenceHash == CClusterReferenceHash::ERR::errEntryNotFound );

        entry = CClusterReferenceEntry( key, errCluster, jpos );
        errClusterReferenceHash = m_clusterReferenceHash.ErrInsertEntry( &lock, entry );
        if ( errClusterReferenceHash == CClusterReferenceHash::ERR::errOutOfMemory )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
        Assert( errClusterReferenceHash == CClusterReferenceHash::ERR::errSuccess );
    }

HandleError:
    if ( fWriteLocked )
    {
        m_clusterReferenceHash.WriteUnlockKey( &lock );
    }
    return err;
}

template<class I>
void THashedLRUKCache<I>::ProcessClusterInvalidate( _In_ const ClusterNumber clno )
{
    CClusterReferenceKey            key;
    CClusterReferenceHash::CLock    lock;
    CClusterReferenceHash::ERR      errClusterReferenceHash = CClusterReferenceHash::ERR::errSuccess;
    CClusterReferenceEntry          entry;

    key = CClusterReferenceKey( clno );
    m_clusterReferenceHash.WriteLockKey( key, &lock );

    errClusterReferenceHash = m_clusterReferenceHash.ErrRetrieveEntry( &lock, &entry );
    if ( errClusterReferenceHash == CClusterReferenceHash::ERR::errSuccess )
    {
        errClusterReferenceHash = m_clusterReferenceHash.ErrDeleteEntry( &lock );
        Assert( errClusterReferenceHash == CClusterReferenceHash::ERR::errSuccess );
    }
    else
    {
        Assert( errClusterReferenceHash == CClusterReferenceHash::ERR::errEntryNotFound );
    }

    m_clusterReferenceHash.WriteUnlockKey( &lock );
}

template<class I>
ERR THashedLRUKCache<I>::ErrCheckForUnresolvedFailedClusterReferences()
{
    ERR                             err                     = JET_errSuccess;
    BOOL                            fEndHashScan            = fFalse;
    CClusterReferenceHash::CLock    lock;
    CClusterReferenceHash::ERR      errClusterReferenceHash = CClusterReferenceHash::ERR::errSuccess;
    CClusterReferenceEntry          entry;
    CClusterReferenceEntry          entryFirstCorruption;

    m_clusterReferenceHash.BeginHashScan( &lock );
    fEndHashScan = fTrue;

    while ( ( errClusterReferenceHash = m_clusterReferenceHash.ErrMoveNext( &lock ) ) != CClusterReferenceHash::ERR::errNoCurrentEntry )
    {
        errClusterReferenceHash = m_clusterReferenceHash.ErrRetrieveEntry( &lock, &entry );
        Assert( errClusterReferenceHash == CClusterReferenceHash::ERR::errSuccess );

        if ( entry.Err() < JET_errSuccess )
        {
            if ( entryFirstCorruption.Jpos() > entry.Jpos() )
            {
                entryFirstCorruption = entry;
            }
        }
    }

    m_errJournalCorruption = m_errJournalCorruption < JET_errSuccess ?
        m_errJournalCorruption :
        entryFirstCorruption.Err();
    m_jposCorruption = min( m_jposCorruption, entryFirstCorruption.Jpos() );

    Call( entryFirstCorruption.Err() );

HandleError:
    if ( fEndHashScan )
    {
        m_clusterReferenceHash.EndHashScan( &lock );
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrRedoJournalEntries()
{
    ERR err = JET_errSuccess;

    //  walk the list of queued journal entries

    for (   CQueuedJournalEntry* pqje = m_ilQueuedJournalEntry.PrevMost();
            pqje;
            pqje = m_ilQueuedJournalEntry.Next( pqje ) )
    {
        //  redo the journal entry

        m_jposRedo = pqje->Jpos();

        Call( ErrRedoJournalEntry( pqje ) );
    }

    m_jposRedo = jposInvalid;

HandleError:
    Assert( !FAnyOpenSlab() );
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrRedoJournalEntry( _In_ const CQueuedJournalEntry* const pqje )
{
    ERR err = JET_errSuccess;

    switch ( pqje->Pje()->Jetyp() )
    {
        case jetypCacheUpdate:
            Call( ErrRedoCacheUpdateJournalEntry( pqje ) );
            break;

        case jetypFlush:
            break;

        default:
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheUnknownJournalEntry" ) );
    }

HandleError:
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrRedoCacheUpdateJournalEntry( _In_ const CQueuedJournalEntry* const pqje )
{
    ERR                                     err     = JET_errSuccess;
    const CCacheUpdateJournalEntry* const   pcuje   = (const CCacheUpdateJournalEntry*)pqje->Pje();
    const JournalPosition                   jpos    = pqje->Jpos();
    const JournalPosition                   jposEnd = pqje->JposEnd();
    QWORD                                   ibSlab  = qwMax;
    ICachedBlockSlab*                       pcbs    = NULL;

    //  process each cache update

    for ( ULONG icbu = 0; icbu < pcuje->Ccbu(); icbu++ )
    {
        const CCachedBlockUpdate* const pcbu = pcuje->Pcbu( icbu );

        //  if we don't have the slab for this update yet then get it

        if ( pcbu->IbSlab() != ibSlab )
        {
            //  schedule any existing slab for write back

            Call( ErrScheduleSlabForWriteBack( pcbs, jpos, jposEnd ) );

            LogCacheUpdateJournalEntry( pqje, ibSlab );

            //  release the slab

            ReleaseSlab( JET_errSuccess, &pcbs );

            //  get the new slab by offset
            //
            //  NOTE:  this will not fail even if a chunk in the slab is corrupt

            Call( ErrIgnoreVerificationErrors( ErrGetSlabForRecovery( pcbu->IbSlab(), &pcbs ) ) );
            ibSlab = pcbu->IbSlab();
        }

        //  copy the slot state into the slab

        Call( pcbs->ErrUpdateSlot( *pcbu ) );
    }

    //  schedule the slab for write back

    Call( ErrScheduleSlabForWriteBack( pcbs, jpos, jposEnd ) );

    LogCacheUpdateJournalEntry( pqje, ibSlab );

    //  release the slab

HandleError:
    ReleaseSlab( err, &pcbs );
    return err;
}

template<class I>
void THashedLRUKCache<I>::LogCacheUpdateJournalEntry(   _In_ const CQueuedJournalEntry* const   pqje,
                                                        _In_ const QWORD                        ibSlab )
{
    const CCacheUpdateJournalEntry* const   pcuje   = (const CCacheUpdateJournalEntry*)pqje->Pje();
    const JournalPosition                   jpos    = pqje->Jpos();

    if ( FOSTraceTagEnabled( JET_tracetagBlockCacheOperations ) )
    {
        for ( ULONG icbu = 0; icbu < pcuje->Ccbu(); icbu++ )
        {
            const CCachedBlockUpdate* const pcbu = pcuje->Pcbu( icbu );

            if ( pcbu->IbSlab() == ibSlab && pcbu->FSlotUpdated() )
            {
                OSTrace(    JET_tracetagBlockCacheOperations,
                            OSFormat(   "C=%s 0x%016I64x  CacheUpdate  %c%c %s (REDO)",
                                        OSFormatFileId( this ),
                                        QWORD( jpos ),
                                        pcbu->FSlotUpdated() ? 'U' : '_',
                                        pcbu->FClusterReference() ? 'R' : '_',
                                        OSFormat( *pcbu ) ) );
            }
        }
    }
}

template<class I>
void THashedLRUKCache<I>::ReleaseJournalEntries()
{
    while ( CQueuedJournalEntry* pqje = m_ilQueuedJournalEntry.PrevMost() )
    {
        m_ilQueuedJournalEntry.Remove( pqje );
        delete pqje;
    }
}

template<class I>
ERR THashedLRUKCache<I>::ErrChangeSlabs(    _In_        const QWORD                 ibSlab,
                                            _Inout_     ICachedBlockSlab** const    ppcbs,
                                            _Out_opt_   BOOL* const                 pfChangedSlab )
{
    ERR                 err             = JET_errSuccess;
    ICachedBlockSlab*   pcbs            = *ppcbs;
    QWORD               ibSlabCurrent   = 0;
    BOOL                fChangedSlab    = fFalse;

    *ppcbs = NULL;
    if ( pfChangedSlab )
    {
        *pfChangedSlab = fFalse;
    }

    //  we already have a slab open

    if ( pcbs )
    {
        //  if this slab cannot contain this cached block then we need to move to the correct one

        Call( pcbs->ErrGetPhysicalId( &ibSlabCurrent ) );

        if ( ibSlab != ibSlabCurrent )
        {
            //  update this slab

            Call( ErrUpdateSlab( &pcbs ) );

            //  release this slab

            ReleaseSlab( JET_errSuccess, &pcbs );
        }
    }

    //  if we don't have a slab open then open the one that can hold this cached block

    if ( !pcbs )
    {
        Call( ErrGetSlab( ibSlab, &pcbs ) );
        fChangedSlab = fTrue;
    }

    //  return the slab that can hold this cached block

    *ppcbs = pcbs;
    pcbs = NULL;
    if ( pfChangedSlab )
    {
        *pfChangedSlab = fChangedSlab;
    }

HandleError:
    ReleaseSlab( err, &pcbs );
    if ( err < JET_errSuccess )
    {
        ReleaseSlab( err, ppcbs );
        if ( pfChangedSlab )
        {
            *pfChangedSlab = fFalse;
        }
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrUpdateSlabs(    _Inout_opt_ ICachedBlockSlab** const    ppcbsA,
                                            _Inout_opt_ ICachedBlockSlab** const    ppcbsB )
{
    ERR                         err             = JET_errSuccess;
    const size_t                cpcbs           = 2;
    ICachedBlockSlab*           rgpcbs[ cpcbs ] = { *ppcbsA, *ppcbsB };
    CUpdateSlabVisitor          usv( this, rgpcbs[ 0 ], rgpcbs[ 1 ] );
    BOOL                        fUpdated        = fFalse;
    CCacheUpdateJournalEntry*   pcuje           = NULL;
    const CJournalEntry*        pjeCompressed   = NULL;
    JournalPosition             jpos            = jposInvalid;
    JournalPosition             jposEnd         = jposInvalid;

    //  if we don't have an updated slab then we're done

    for ( size_t ipcbs = 0; ipcbs < cpcbs; ipcbs++ )
    {
        if ( rgpcbs[ ipcbs ] && rgpcbs[ ipcbs ]->FUpdated() )
        {
            fUpdated = fTrue;
        }
    }

    if ( !fUpdated )
    {
        Error( JET_errSuccess );
    }

    //  if we updated a journal slab then mark any allocated slots as used

    for ( size_t ipcbs = 0; ipcbs < cpcbs; ipcbs++ )
    {
        if ( !rgpcbs[ ipcbs ] || !rgpcbs[ ipcbs ]->FUpdated() )
        {
            continue;
        }

        QWORD ibSlab;
        Call( rgpcbs[ ipcbs ]->ErrGetPhysicalId( &ibSlab ) );
        if ( !FJournalSlab( ibSlab ) )
        {
            continue;
        }

        Call( CMarkAllocatedJournalClustersAsUsedSlabVisitor::ErrExecute( rgpcbs[ ipcbs ] ) );
    }

    //  visit all the slots in the slabs to process any updates

    Call( usv.ErrVisitSlots() );

    //  emit a journal entry describing the final state of the affected chunks in the updated slabs
    //
    //  NOTE:  a single journal entry describing multiple updated slabs provides atomicity of that update
    //
    //  NOTE:  once this journal entry is appended then the rest of this method must succeed to ensure that the state
    //  of the journal and slabs cannot diverge (e.g. we cannot allow a subsequent call to
    //  ICachedBlockSlab::RevertChanges to undo a change we put in the journal)

    Call( CCacheUpdateJournalEntry::ErrCreate( usv.Ccbu(), usv.Rgcbu(), &pcuje ) );
    Call( CCompressedJournalEntry::ErrCreate( pcuje, &pjeCompressed ) );

    {
        CJournalBuffer rgjb[] = { { pjeCompressed->Cb(), (const BYTE*)pjeCompressed }, };
        Call( m_pj->ErrAppendEntry( _countof( rgjb ), rgjb, &jpos, &jposEnd ) );
    }

    for ( ULONG icbu = 0; icbu < pcuje->Ccbu(); icbu++ )
    {
        const CCachedBlockUpdate*   pcbu        = pcuje->Pcbu( icbu );

        if ( !pcbu->FSlotUpdated() )
        {
            continue;
        }

        OSTrace(    JET_tracetagBlockCacheOperations,
                    OSFormat(   "C=%s 0x%016I64x  CacheUpdate  %c%c %s",
                                OSFormatFileId( this ),
                                QWORD( jpos ),
                                pcbu->FSlotUpdated() ? 'U' : '_',
                                pcbu->FClusterReference() ? 'R' : '_',
                                OSFormat( *pcbu ) ) );
    }

    //  update our cached block presence filter if necessary

    for ( size_t ipcbs = 0; ipcbs < cpcbs; ipcbs++ )
    {
        if ( rgpcbs[ ipcbs ] && rgpcbs[ ipcbs ]->FUpdated() )
        {
            m_pcbpf->Update( rgpcbs[ ipcbs ] );
        }
    }

    //  schedule slabs for write back

    for ( size_t ipcbs = 0; ipcbs < cpcbs; ipcbs++ )
    {
        if ( rgpcbs[ ipcbs ] && rgpcbs[ ipcbs ]->FUpdated() )
        {
            err = ErrScheduleSlabForWriteBack( rgpcbs[ ipcbs ], jpos, jposEnd );
            Assert( err >= JET_errSuccess || err == JET_errOutOfMemory );
            EnforceSz( err >= JET_errSuccess, "UpdateSlabs" );
        }
    }

    //  on failure, release all slabs, reverting all changes, to ensure we never save a partial update

HandleError:
    delete pjeCompressed;
    delete pcuje;
    if ( err < JET_errSuccess )
    {
        for ( size_t ipcbs = 0; ipcbs < cpcbs; ipcbs++ )
        {
            ReleaseSlab( err, &rgpcbs[ ipcbs ] );
        }

        *ppcbsA = NULL;
        *ppcbsB = NULL;
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrScheduleSlabForWriteBack(   _In_opt_    ICachedBlockSlab* const pcbs,  
                                                        _In_        const JournalPosition   jpos,  
                                                        _In_        const JournalPosition   jposEnd )
{
    ERR                 err     = JET_errSuccess;
    CSlabWriteBack*     pswb    = NULL;

    //  if there is no slab to schedule then we're done

    if ( !pcbs )
    {
        Error( JET_errSuccess );
    }

    //  if the slab isn't updated or dirty then we're done

    if ( !pcbs->FUpdated() && !pcbs->FDirty() )
    {
        Error( JET_errSuccess );
    }

    //  get or add the slab write back context whose existence is protected by ownership of the slab itself

    Call( ErrGetOrAddSlabWriteBackContext( pcbs, &pswb ) );

    //  accept any updates that were made to the slab.  if this isn't called then the state is rolled back.  this step
    //  is what prevents us from accidentally writing back changes to the slab that were not journaled.  also note that
    //  even if this slab were saved before the updates were accepted, the unaccepted updates would not be written out

    Call( pcbs->ErrAcceptUpdates() );

    //  if the slab is dirty then schedule it for write back

    if ( pcbs->FDirty() )
    {
        //  register this slab for write back

        m_critSlabsToWriteBack.Enter();

        pswb->RegisterWrite( pcbs, jpos, jposEnd );

        RegisterSlabForWriteBack( pcbs, pswb );

        m_critSlabsToWriteBack.Leave();

        //  start async slab write back

        CueAsyncSlabWriteBackWorker();
    }

HandleError:
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrGetOrAddSlabWriteBackContext(   _In_    ICachedBlockSlab* const pcbs,
                                                            _Out_   CSlabWriteBack** const  ppswb )
{
    ERR                         err                     = JET_errSuccess;
    QWORD                       ibSlab                  = 0;
    CSlabWriteBackKey           key;
    CSlabWriteBackHash::CLock   lock;
    BOOL                        fReadLocked             = fFalse;
    BOOL                        fWriteLocked            = fFalse;
    CSlabWriteBackHash::ERR     errSlabWriteBackHash    = CSlabWriteBackHash::ERR::errSuccess;
    CSlabWriteBackEntry         entry;
    CSlabWriteBack*             pswb                    = NULL;
    CSlabWriteBack*             pswbNew                 = NULL;

    *ppswb = NULL;

    Call( ErrEnsureInitSlabWriteBackHash() );

    Call( pcbs->ErrGetPhysicalId( &ibSlab ) );

    key = CSlabWriteBackKey( ibSlab );
    m_slabWriteBackHash.ReadLockKey( key, &lock );
    fReadLocked = fTrue;

    errSlabWriteBackHash = m_slabWriteBackHash.ErrRetrieveEntry( &lock, &entry );
    if ( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errSuccess )
    {
        pswb = entry.Pswb();
    }
    else
    {
        Assert( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errEntryNotFound );

        m_slabWriteBackHash.ReadUnlockKey( &lock );
        fReadLocked = fFalse;
        m_slabWriteBackHash.WriteLockKey( key, &lock );
        fWriteLocked = fTrue;

        errSlabWriteBackHash = m_slabWriteBackHash.ErrRetrieveEntry( &lock, &entry );
        if ( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errSuccess )
        {
            pswb = entry.Pswb();
        }
        else
        {
            Assert( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errEntryNotFound );

            Alloc( pswb = pswbNew = new CSlabWriteBack( ibSlab ) );

            entry = CSlabWriteBackEntry( ibSlab, pswbNew );
            errSlabWriteBackHash = m_slabWriteBackHash.ErrInsertEntry( &lock, entry );
            if ( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errOutOfMemory )
            {
                Error( ErrERRCheck( JET_errOutOfMemory ) );
            }
            Assert( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errSuccess );

            pswbNew = NULL;
        }

        m_slabWriteBackHash.WriteUnlockKey( &lock );
        fWriteLocked = fFalse;
    }

    *ppswb = pswb;
    pswb = NULL;

HandleError:
    delete pswbNew;
    if ( fReadLocked )
    {
        m_slabWriteBackHash.ReadUnlockKey( &lock );
    }
    if ( fWriteLocked )
    {
        m_slabWriteBackHash.WriteUnlockKey( &lock );
    }
    if ( err < JET_errSuccess )
    {
        delete *ppswb;
        *ppswb = NULL;
    }
    return err;
}

template<class I>
void THashedLRUKCache<I>::RegisterSlabForWriteBack( _In_ ICachedBlockSlab* const pcbs, _In_ CSlabWriteBack* const pswb )
{
    Assert( pcbs );  //  caller should own the exclusive lock on the slab
    Assert( m_critSlabsToWriteBack.FOwner() );

    //  place this write back into the list in order by jposMin

    if ( !m_ilSlabsToWriteBackByJposMin.FMember( pswb ) )
    {
        m_ilSlabsToWriteBackByJposMin.InsertAsNextMost( pswb );
    }

    for (   CSlabWriteBack* pswbPrev = m_ilSlabsToWriteBackByJposMin.Prev( pswb );
            pswbPrev && pswbPrev->JposMin() > pswb->JposMin();
            pswbPrev = m_ilSlabsToWriteBackByJposMin.Prev( pswbPrev ) )
    {
        m_ilSlabsToWriteBackByJposMin.Remove( pswb );
        m_ilSlabsToWriteBackByJposMin.Insert( pswb, pswbPrev );
        pswbPrev = pswb;
    }
}

template<class I>
void THashedLRUKCache<I>::CueAsyncSlabWriteBackWorker()
{
    //  if the journal is not available then ignore the request

    if ( !m_pj )
    {
        return;
    }

    //  if we already have our max request count then ignore the request

    if ( m_semSlabWriteBackWorkerRequest.FTryAcquire() )
    {
        SubmitThreadpoolWork( m_ptpwSlabWriteBack );
    }
}

template<class I>
void THashedLRUKCache<I>::AsyncSlabWriteBackWorker()
{
    //  serialize slab write back worker requests

    m_semSlabWriteBackWorkerExecute.Acquire();

    //  perform opportunistic slab write backs

    PerformOpportunisticSlabWriteBacks();

    //  enable another slab write back worker task to execute

    m_semSlabWriteBackWorkerExecute.Release();

    //  allow another slab write back worker request to be made

    m_semSlabWriteBackWorkerRequest.Release();
}

template<class I>
void THashedLRUKCache<I>::PerformOpportunisticSlabWriteBacks()
{
    ERR                     err                     = JET_errSuccess;
    const size_t            cbSlabCacheMax          = (size_t)( Pcconfig()->CbSlabMaximumCacheSize() );
    const size_t            cbSlab                  = (size_t)CbChunkPerSlab();
    const size_t            cSlabCacheMax           = cbSlabCacheMax / cbSlab;
    JournalPosition         jposAppend              = jposInvalid;
    const QWORD             cbJournalTotal          = m_pch->CbJournal();
    const double            pctJournalUsedMax       = max( 0, min( 100, Pcconfig()->PctJournalSegmentsInUse() ) );
    JournalPosition         jposReplayTarget        = jposInvalid;
    BOOL                    fListLocked             = fFalse;
    size_t                  cSlabWriteBack          = 0;
    CArray<QWORD>           arrayIbSlab;
    JournalPosition         jposEndLastMin          = jposInvalid;
    JournalPosition         jposReplay              = jposInvalid;
    JournalPosition         jposDurableForWriteBack = jposInvalid;
    JournalPosition         jposRedo                = jposInvalid;
    JournalPosition         jposReplaySlab          = jposInvalid;
    JournalPosition         jposReplayWriteCounts   = jposInvalid;
    JournalPosition         jposReplayCandidate     = jposInvalid;

    //  do not perform opportunistic slab write back until we have recovered.  we don't want to modify the journal
    //  until recovery has finished.  we also cannot handle journal full during recovery

    if ( !m_fRecovered )
    {
        Error( JET_errSuccess );
    }

    //  compute our replay target

    Call( m_pj->ErrGetProperties( NULL, NULL, NULL, &jposAppend, NULL ) );

    jposReplayTarget = jposAppend + cbJournalSegment - (QWORD)( cbJournalTotal * pctJournalUsedMax / 100 );

    //  get the slabs we want to write back

    Call( ErrToErr<CArray<QWORD>>( arrayIbSlab.ErrSetCapacity( cSlabCacheMax ) ) );

    m_critSlabsToWriteBack.Enter();
    fListLocked = fTrue;

    if ( m_ilSlabsToWriteBackByJposMin.Count() > cSlabCacheMax )
    {
        cSlabWriteBack = m_ilSlabsToWriteBackByJposMin.Count() - cSlabCacheMax;
    }

    for (   CSlabWriteBack* pswbT = m_ilSlabsToWriteBackByJposMin.PrevMost();
            pswbT;
            pswbT = m_ilSlabsToWriteBackByJposMin.Next( pswbT ) )
    {
        //  if this slab fits in the cache and it isn't impeding the replay pointer then we're done

        if ( arrayIbSlab.Size() >= cSlabWriteBack && pswbT->JposMin() >= jposReplayTarget )
        {
            break;
        }

        //  collect this slab for write back

        Call( ErrToErr<CArray<QWORD>>( arrayIbSlab.ErrSetEntry( arrayIbSlab.Size(), pswbT->IbSlab() ) ) );

        //  remember the min of the youngest journal positions affecting any of the slabs to write back

        if ( jposEndLastMin == jposInvalid || pswbT->JposEndLast() < jposEndLastMin )
        {
            jposEndLastMin = pswbT->JposEndLast();
        }
    }

    m_critSlabsToWriteBack.Leave();
    fListLocked = fFalse;

    //  if we found slabs we need to write back then do so

    if ( jposEndLastMin != jposInvalid )
    {
        //  if we need to flush the log to make at least one of these slabs durable for write back then do so

        Call( m_pj->ErrGetProperties( NULL, &jposDurableForWriteBack, NULL, NULL, NULL ) );

        while ( jposEndLastMin > jposDurableForWriteBack )
        {
            Call( m_pj->ErrFlush() );
            Call( m_pj->ErrGetProperties( NULL, &jposDurableForWriteBack, NULL, NULL, NULL ) );
        }

        //  try to write back these dirty slabs

        Call( ErrTryStartSlabWriteBacks( arrayIbSlab, fFalse ) );
    }

    //  save our write counts if they are impeding the replay pointer

    if ( m_jposReplayWriteCounts != jposInvalid && m_jposReplayWriteCounts < jposReplayTarget )
    {
        Call( ErrSaveWriteCounts() );
    }

    //  truncate the journal if possible

    Call( m_pj->ErrGetProperties( &jposReplay, &jposDurableForWriteBack, NULL, NULL, NULL ) );

    jposRedo = m_jposRedo;

    m_critSlabsToWriteBack.Enter();
    for (   CSlabWriteBack* pswbT = m_ilSlabsToWriteBackByJposMin.PrevMost();
            pswbT;
            pswbT = m_ilSlabsToWriteBackByJposMin.Next( pswbT ) )
    {
        jposReplaySlab = min( jposReplaySlab, pswbT->JposMin() );
    }
    m_critSlabsToWriteBack.Leave();

    jposReplayWriteCounts = (JournalPosition)AtomicRead( (QWORD*)&m_jposReplayWriteCounts );

    jposReplayCandidate = min( jposReplayCandidate, jposDurableForWriteBack );
    jposReplayCandidate = min( jposReplayCandidate, jposRedo );
    jposReplayCandidate = min( jposReplayCandidate, jposReplaySlab );
    jposReplayCandidate = min( jposReplayCandidate, jposReplayWriteCounts );

    if ( jposReplayCandidate != jposInvalid && jposReplay < jposReplayTarget )
    {
        Call( m_pj->ErrTruncate( jposReplayCandidate ) );
        Call( m_pj->ErrFlush() );
    }

HandleError:
    if ( fListLocked )
    {
        m_critSlabsToWriteBack.Leave();
    }
}

template<class I>
ERR THashedLRUKCache<I>::ErrVerifyTruncate( _In_ const JournalPosition jposReplay )
{
    ERR             err                     = JET_errSuccess;
    BOOL            fLeave                  = fFalse;
    JournalPosition jposDurableForWriteBack = jposInvalid;

    //  it is illegal to delete journal entries that have not been replayed

    if ( jposReplay > m_jposRedo )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTruncateRedo" ) );
    }

    //  it is illegal to delete journal entries corresponding to dirty slabs because that could prevent us from writing
    //  them back on a future recovery

    m_critSlabsToWriteBack.Enter();
    fLeave = fTrue;

    for (   CSlabWriteBack* pswb = m_ilSlabsToWriteBackByJposMin.PrevMost();
            pswb;
            pswb = m_ilSlabsToWriteBackByJposMin.Next( pswb ) )
    {
        if ( jposReplay > pswb->JposMin() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheTruncateWAL" ) );
        }
    }

    m_critSlabsToWriteBack.Leave();
    fLeave = fFalse;

    //  it is illegal to delete journal entries corresponding to dirty write counts because that could prevent us from
    //  properly computing the write counts on a future recovery

    if ( jposReplay > (JournalPosition)AtomicRead( (QWORD*)&m_jposReplayWriteCounts ) )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTruncateWriteCounts" ) );
    }

    //  it is illegal to advance the replay pointer past the durable for write back pointer because that could prevent
    //  us from writing back updates that were durable but not durable for write back

    Call( m_pj->ErrGetProperties( NULL, &jposDurableForWriteBack, NULL, NULL, NULL ) );

    if ( jposReplay > jposDurableForWriteBack )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheTruncateDurable" ) );
    }

HandleError:
    if ( fLeave )
    {
        m_critSlabsToWriteBack.Leave();
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrFlushAllState( _In_ const JournalPosition jposDurableForWriteBack )
{
    ERR                                     err         = JET_errSuccess;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls       = NULL;
    BOOL                                    fListLocked = fFalse;
    CArray<QWORD>                           arrayIbSlab;

    //  get our thread local storage

    Call( ErrGetThreadLocalStorage( &pctls ) );

    //  register the thread that is performing the flush as suspended as well so that we can write its slabs

    Call( ErrSuspendThreadFromStateAccess( m_msStateAccess.GroupActive(), pctls ) );

    //  retry until all dirty slabs are written out

    while ( !m_ilSlabsToWriteBackByJposMin.FEmpty() )
    {
        //  register any effectively suspended threads

        Call( ErrSuspendBlockedThreadsFromStateAccess() );

        //  get a list of all dirty slabs

        Call( ErrToErr<CArray<QWORD>>( arrayIbSlab.ErrSetCapacity( m_ilSlabsToWriteBackByJposMin.Count() ) ) );
        CallS( ErrToErr<CArray<QWORD>>( arrayIbSlab.ErrSetSize( 0 ) ) );

        m_critSlabsToWriteBack.Enter();
        fListLocked = fTrue;

        for (   CSlabWriteBack* pswb = m_ilSlabsToWriteBackByJposMin.PrevMost();
                pswb;
                pswb = m_ilSlabsToWriteBackByJposMin.Next( pswb ) )
        {
            //  we should not see a slab that isn't durable for write back

            if ( pswb->JposEndLast() > jposDurableForWriteBack )
            {
                Error( ErrBlockCacheInternalError( "HashedLRUKCacheFlushAllStateNotDurable" ) );
            }

            //  collect the slab for write back

            Call( ErrToErr<CArray<QWORD>>( arrayIbSlab.ErrSetEntry( arrayIbSlab.Size(), pswb->IbSlab() ) ) );
        }

        m_critSlabsToWriteBack.Leave();
        fListLocked = fFalse;

        //  try to write back all the dirty slabs

        if ( arrayIbSlab.Size() > 0 )
        {
            Call( ErrTryStartSlabWriteBacks( arrayIbSlab, fTrue ) );
        }
    }

    //  if there are any dirty slabs then something went wrong

    if ( !m_ilSlabsToWriteBackByJposMin.FEmpty() )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheFlushAllStateIncomplete" ) );
    }

    //  save our write counts

    Call( ErrSaveWriteCounts() );

    //  resume slab opens

HandleError:
    if ( fListLocked )
    {
        m_critSlabsToWriteBack.Leave();
    }
    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrTryStartSlabWriteBacks( _In_ CArray<QWORD>&         arrayIbSlab,
                                                    _In_ const BOOL             fSaveOpenSlabs )
{
    ERR                 err                 = JET_errSuccess;
    ICachedBlockSlab*   pcbs                = NULL;
    BOOL                fReleaseSlabOnSave  = fTrue;
    BOOL                fIssue              = fFalse;

    //  sort the slabs by offset.  we must lock them in ascending offset order to avoid deadlocks

    arrayIbSlab.Sort( CompareIbSlab );

    //  begin write back for all slabs

    for ( size_t iibSlab = 0; iibSlab < arrayIbSlab.Size(); iibSlab++ )
    {
        Assert( iibSlab == 0 || arrayIbSlab[ iibSlab - 1 ] < arrayIbSlab[ iibSlab ] );

        const QWORD ibSlab = arrayIbSlab[ iibSlab ];

        //  if we are saving open slabs then try to get the open slab first but only if it is safe for write back

        if ( fSaveOpenSlabs )
        {
            pcbs = PcbsGetOpenSlabSafeForWriteBack( ibSlab );
            fReleaseSlabOnSave = fFalse;
        }

        //  if we still don't have the slab then try to get it by offset, but don't wait for the lock

        if ( !pcbs )
        {
            Call( ErrTryGetSlabForWriteBack( ibSlab, &pcbs ) );
            fReleaseSlabOnSave = fTrue;
        }

        //  try to start write back of the slab if possible

        if ( pcbs )
        {
            BOOL fSaved = fFalse;
            Call( ErrTryStartSlabWriteBack( &pcbs, fReleaseSlabOnSave, &fSaved ) );
            fIssue = fIssue || fSaved;
        }
    }

    //  wait for all slab write backs to complete

    if ( fIssue )
    {
        Call( ErrFinishSlabWriteBacks() );
        fIssue = fFalse;
    }

HandleError:
    if ( fReleaseSlabOnSave )
    {
        ReleaseSlab( err, &pcbs );
    }
    if ( fIssue )
    {
        ERR errT = ErrFinishSlabWriteBacks();
        err = err < JET_errSuccess ? err : errT;
    }
    return err;
}

template<class I>
ICachedBlockSlab* THashedLRUKCache<I>::PcbsGetOpenSlabSafeForWriteBack( _In_ const QWORD ibSlab )
{
    ICachedBlockSlab*                               pcbs                    = NULL;
    CArray<CHashedLRUKCacheThreadLocalStorage<I>*>* parraySuspendedThreads  = NULL;
    
    parraySuspendedThreads = &m_rgarraySuspendedThreads[ m_msStateAccess.GroupActive() ];

    m_critSuspendedThreads.Enter();

    for ( size_t i = 0; !pcbs && i < parraySuspendedThreads->Size(); i++ )
    {
        pcbs = parraySuspendedThreads->Entry( i )->PcbsOpenSlab( ibSlab );
    }

    m_critSuspendedThreads.Leave();

    return pcbs;
}

template<class I>
ERR THashedLRUKCache<I>::ErrFinishSlabWriteBacks()
{
    ERR err = JET_errSuccess;

    //  issue all write back IO

    Call( PffCaching()->ErrIOIssue() );

    //  wait for all slab write backs to complete

    m_critSlabWrites.Enter();
    m_msSlabWrites.Partition();
    m_critSlabWrites.Leave();

    Call( m_errSlabWrites );

HandleError:
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrTryStartSlabWriteBack(  _Inout_ ICachedBlockSlab** const    ppcbs,
                                                    _In_    const BOOL                  fReleaseSlabOnSave,
                                                    _Out_   BOOL* const                 pfSaved )
{
    ERR                                 err                     = JET_errSuccess;
    ICachedBlockSlab*                   pcbs                    = *ppcbs;
    JournalPosition                     jposDurableForWriteBack = jposInvalid;
    CSlabWriteBack*                     pswb                    = NULL;
    BOOL                                fLeave                  = fFalse;
    const CSlabWriteCompletionContext*  pswcc                   = NULL;

    *ppcbs = NULL;
    *pfSaved = fFalse;

    //  get the durable for write back pointer

    Call( m_pj->ErrGetProperties( NULL, &jposDurableForWriteBack, NULL, NULL, NULL ) );

    //  get the slab's write back context if any

    pswb = PswbTryGetSlabWriteBackContext( pcbs );

    //  if the slab is dirty and is durable for write back then we can write it back

    m_critSlabsToWriteBack.Enter();
    fLeave = fTrue;

    if ( pswb && !pswb->FSavePending() && pswb->FDirty() && pswb->JposEndLast() <= jposDurableForWriteBack )
    {
        //  add this slab to the pending write back list

        m_ilSlabWriteBacksPending.InsertAsNextMost( pswb );

        m_critSlabsToWriteBack.Leave();
        fLeave = fFalse;

        //  update the replay pointer associated with the write counts

        AtomicCompareExchange( (QWORD*)&m_jposReplayWriteCounts, (QWORD)jposInvalid, (QWORD)pswb->JposLast() );
        AtomicExchangeMin( (QWORD*)&m_jposReplayWriteCounts, (QWORD)pswb->JposLast() );

        //  begin the save operation on the slab

        pswcc = &m_rgswcc[ m_msSlabWrites.Enter() ];
        Call( pswcc->ErrSave( pswb, &pcbs, fReleaseSlabOnSave ) );
        pswcc = NULL;

        //  indicate that we saved the slab

        *pfSaved = fTrue;
    }

HandleError:
    if ( fLeave )
    {
        m_critSlabsToWriteBack.Leave();
    }
    if ( pswcc )
    {
        pswcc->Cancel();
    }
    if ( fReleaseSlabOnSave )
    {
        ReleaseSlab( err, &pcbs );
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrSaveWriteCounts()
{
    ERR err = JET_errSuccess;

    //  reset our tracking of the journal entries that cause slab write backs that impact the write counts

    AtomicExchange( (__int64*)&m_jposReplayWriteCounts, (__int64)jposInvalid );

    //  save the write counts

    Call( m_pcbwcm->ErrSave() );

HandleError:
    return err;
}

template<class I>
void THashedLRUKCache<I>::Saved(    _In_ const ERR                  errSave,
                                    _In_ CSlabWriteBack* const      pswb,
                                    _In_ ICachedBlockSlab* const    pcbs,
                                    _In_ const BOOL                 fReleaseSlabOnSave )
{
    //  remove the slab from the pending write back list.  if the write succeeded then remove it from the dirty list
    //  and put it on the completed write back list so that it may be released once this callback is complete.  also,
    //  release any *OTHER* slab write back contexts that have been completed

    m_critSlabsToWriteBack.Enter();

    ReleaseCompletedSlabWriteBacks();

    m_ilSlabWriteBacksPending.Remove( pswb );

    if ( errSave >= JET_errSuccess )
    {
        m_ilSlabsToWriteBackByJposMin.Remove( pswb );
        m_ilSlabWriteBacksCompleted.InsertAsNextMost( pswb );
    }

    m_critSlabsToWriteBack.Leave();

    //  if the slab write back was successful then delete its context

    if ( errSave >= JET_errSuccess )
    {
        CSlabWriteBackKey           key( pswb->IbSlab() );
        CSlabWriteBackHash::CLock   lock;
        CSlabWriteBackHash::ERR     errSlabWriteBackHash = CSlabWriteBackHash::ERR::errSuccess;

        m_slabWriteBackHash.WriteLockKey( key, &lock );
        errSlabWriteBackHash = m_slabWriteBackHash.ErrDeleteEntry( &lock );
        Assert( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errSuccess );
        m_slabWriteBackHash.WriteUnlockKey( &lock );
    }

    //  release the slab reference if appropriate

    if ( fReleaseSlabOnSave )
    {
        ICachedBlockSlab* pcbsT = pcbs;
        ReleaseSlab( JET_errSuccess, &pcbsT );
    }

    //  save our error state

    if ( errSave < JET_errSuccess )
    {
        AtomicCompareExchange(  (LONG*)&m_errSlabWrites,
                                JET_errSuccess,
                                errSave < JET_errSuccess ? errSave : JET_errSuccess );
    }
}

template<class I>
CSlabWriteBack* THashedLRUKCache<I>::PswbTryGetSlabWriteBackContext( _In_ ICachedBlockSlab* const pcbs )
{
    QWORD                       ibSlab                  = 0;
    CSlabWriteBackKey           key;
    CSlabWriteBackHash::CLock   lock;
    CSlabWriteBackHash::ERR     errSlabWriteBackHash    = CSlabWriteBackHash::ERR::errSuccess;
    CSlabWriteBackEntry         entry;
    CSlabWriteBack*             pswb                    = NULL;

    Assert( pcbs );  //  caller should own the exclusive lock on the slab

    if ( ErrEnsureInitSlabWriteBackHash() == JET_errSuccess )
    {
        CallS( pcbs->ErrGetPhysicalId( &ibSlab ) );

        key = CSlabWriteBackKey( ibSlab );
        m_slabWriteBackHash.ReadLockKey( key, &lock );

        errSlabWriteBackHash = m_slabWriteBackHash.ErrRetrieveEntry( &lock, &entry );
        if ( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errSuccess )
        {
            pswb = entry.Pswb();
        }
        else
        {
            Assert( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errEntryNotFound );
        }

        m_slabWriteBackHash.ReadUnlockKey( &lock );
    }

    return pswb;
}

template<class I>
CMeteredSection::Group THashedLRUKCache<I>::GroupForClusterWrite()
{
    return m_msClusterWrites.Enter();
}

template<class I>
void THashedLRUKCache<I>::ClusterWritten( _In_ const ERR err, _In_ const CMeteredSection::Group group )
{
    //  record any error that occurred

    AtomicCompareExchange( (LONG*)&m_errClusterWrites, JET_errSuccess, err < JET_errSuccess ? err : JET_errSuccess );

    //  complete the cluster write

    m_msClusterWrites.Leave( group );
}

template<class I>
ERR THashedLRUKCache<I>::ErrFlushClusters()
{
    ERR err = JET_errSuccess;

    //  ensure that we issue any cluster write requests made by this thread to prevent a deadlock waiting for cluster
    //  writes to complete

    Call( PffCaching()->ErrIOIssue() );

    //  wait for any pending cluster writes to complete

    m_critClusterWrites.Enter();
    m_msClusterWrites.Partition();
    m_critClusterWrites.Leave();

    //  check the error state for cluster writes
    //
    //  NOTE:  cluster writes are considered to be part of the journal.  if one fails then it is the equivalent of a
    //  journal segment write failure.  in that case the journal is considered to be down.  we indicate the journal
    //  is down by acking flush requests with failure

    Call( m_errClusterWrites );

HandleError:
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrFlush()
{
    ERR err = JET_errSuccess;

    //  flush the journal

    Call( m_pj->ErrFlush() );

HandleError:
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrEnqueue( _Inout_ CRequest** const pprequest )
{
    ERR                                     err             = JET_errSuccess;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls           = NULL;
    CRequest*                               prequest        = *pprequest;
    CRequest*                               prequestIO      = NULL;
    CRequest*                               prequestIOPrev  = NULL;

    *pprequest = NULL;

    //  get our thread local storage where we are enqueuing requests

    Call( ErrGetThreadLocalStorage( &pctls ) );

    //  enqueue the request, combining and ordering with previous requests
    //
    //  NOTE:  we do not handle IO gap coalescing here.  we leave that to the OS layer underneath us

    pctls->IlIORequested().InsertAsNextMost( prequest );

    prequestIO = prequest;
    for (   prequestIOPrev = pctls->IlIORequested().Prev( prequestIO );
            prequestIOPrev && !FConflicting( prequestIOPrev, prequestIO );
            prequestIOPrev = pctls->IlIORequested().Prev( prequestIOPrev ) )
    {
        if ( FCombinable( prequestIOPrev, prequestIO ) )
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

            pctls->IlIORequested().Remove( prequestIO );
            prequestIO = prequestIOPrev;
        }
        else if ( CmpRequestIO( prequestIOPrev, prequestIO ) > 0 )
        {
            pctls->IlIORequested().Remove( prequestIO );
            pctls->IlIORequested().Insert( prequestIO, prequestIOPrev );
            prequestIOPrev = prequestIO;
        }
    }

    //  give ownership of the request to the TLS

    pctls->AddRequest( &prequest );

HandleError:
    err = CRequest::ErrRelease( &prequest, err );
    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
    return err;
}

template<class I>
BOOL THashedLRUKCache<I>::FConflicting( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB )
{
    //  IOs for different cached file cannot conflict

    if ( prequestIOA->Pcfte() != prequestIOB->Pcfte() )
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

template<class I>
BOOL THashedLRUKCache<I>::FCombinable( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB )
{
    //  IOs from different cached files cannot be combined

    if ( prequestIOA->Pcfte() != prequestIOB->Pcfte() )
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

    return fTrue;
 }

template<class I>
int THashedLRUKCache<I>::CmpRequestIO( _In_ CRequest* const prequestIOA, _In_ CRequest* const prequestIOB )
{
    //  IOs from different cached files are ordered by their volume id and file id for deterministic behavior

    if ( prequestIOA->Pcfte()->Volumeid() < prequestIOB->Pcfte()->Volumeid() )
    {
        return -1;
    }
    else if ( prequestIOA->Pcfte()->Volumeid() > prequestIOB->Pcfte()->Volumeid() )
    {
        return 1;
    }

    if ( prequestIOA->Pcfte()->Fileid() < prequestIOB->Pcfte()->Fileid() )
    {
        return -1;
    }
    else if ( prequestIOA->Pcfte()->Fileid() > prequestIOB->Pcfte()->Fileid() )
    {
        return 1;
    }

    if ( prequestIOA->Pcfte()->Fileserial() < prequestIOB->Pcfte()->Fileserial() )
    {
        return -1;
    }
    else if ( prequestIOA->Pcfte()->Fileserial() > prequestIOB->Pcfte()->Fileserial() )
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

template<class I>
void THashedLRUKCache<I>::Issue()
{
    ERR                                     err         = JET_errSuccess;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls       = NULL;

    //  get our thread local storage where we are enqueuing requests
    //
    //  NOTE:  if this fails then we know we don't have any enqueued requests

    err = ErrGetThreadLocalStorage( &pctls );
    Assert( err >= JET_errSuccess || err == JET_errOutOfMemory );

    //  if there are enqueued requests then move them to the issue list and request the async IO worker to run ASAP

    if ( pctls && !pctls->IlIORequested().FEmpty() )
    {
        pctls->CritAsyncIOWorkerState().Enter();
        for (   CRequest* prequestIO = pctls->IlIORequested().PrevMost();
                prequestIO;
                prequestIO = pctls->IlIORequested().PrevMost() )
        {
            pctls->IlIORequested().Remove( prequestIO );
            pctls->IlIOIssued().InsertAsNextMost( prequestIO );
        }
        pctls->CritAsyncIOWorkerState().Leave();

        pctls->CueAsyncIOWorker();
    }

    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
}

template<class I>
void THashedLRUKCache<I>::AsyncIOWorker( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls )
{
    pctls->BeginAsyncIOWorker();

    //  for each issued IO, request an IO range lock in terms of the cached file.  these IO range locks not only
    //  protect against chaotic concurrent IO to overlapping offsets but they also serialize all activity for that
    //  offset range including things like write back or moving cached blocks in the caching file

    pctls->CritAsyncIOWorkerState().Enter();
    while ( CRequest* prequestIO = pctls->IlIOIssued().PrevMost() )
    {
        prequestIO->WaitForIORangeLock( CHashedLRUKCacheThreadLocalStorage<I>::CueAsyncIOWorker, (DWORD_PTR)pctls );

        pctls->IlIOIssued().Remove( prequestIO );
        pctls->IlIORangeLockPending().InsertAsNextMost( prequestIO );
    }
    pctls->CritAsyncIOWorkerState().Leave();

    //  determine which requested IO range locks have been acquired

    CRequest* prequestIONext = NULL;
    for (   CRequest* prequestIO = pctls->IlIORangeLockPending().PrevMost();
            prequestIO;
            prequestIO = prequestIONext )
    {
        prequestIONext = pctls->IlIORangeLockPending().Next( prequestIO );

        if ( prequestIO->Piorl()->FLocked() )
        {
            pctls->IlIORangeLockPending().Remove( prequestIO );
            pctls->IlIORangeLocked().InsertAsNextMost( prequestIO );
        }
    }

    //  for each locked IO, request IO against the cached file and then the caching file.  we do this to maximize
    //  our chances of IO optimization by the underlying file system implementation
    //
    //  NOTE:  RequestCachedFileIO / RequestCachingFileIO is touching slabs twice

    while ( CRequest* prequestIO = pctls->IlIORangeLocked().PrevMost() )
    {
        RequestCachedFileIO( prequestIO );

        pctls->IlIORangeLocked().Remove( prequestIO );
        pctls->IlCachedFileIORequested().InsertAsNextMost( prequestIO );
    }

    while ( CRequest* prequestIO = pctls->IlCachedFileIORequested().PrevMost() )
    {
        RequestCachingFileIO( prequestIO );

        pctls->IlCachedFileIORequested().Remove( prequestIO );
        pctls->IlCachingFileIORequested().InsertAsNextMost( prequestIO );
    }

    //  wait for all IO issued so far but asynchronously

    while ( CRequest* prequestIO = pctls->IlCachingFileIORequested().PrevMost() )
    {
        WaitForPendingIOAsync( pctls, prequestIO );

        pctls->IlCachingFileIORequested().Remove( prequestIO );
        pctls->IlIOPending().InsertAsNextMost( prequestIO );
    }

    //  for each pending IO, check for any IOs that are complete

    prequestIONext = NULL;
    for (   CRequest* prequestIO = pctls->IlIOPending().PrevMost();
            prequestIO;
            prequestIO = prequestIONext )
    {
        prequestIONext = pctls->IlIOPending().Next( prequestIO );

        if ( FCompletedIO( prequestIO ) )
        {
            pctls->IlIOPending().Remove( prequestIO );
            pctls->IlIOCompleted().InsertAsNextMost( prequestIO );
        }
    }

    //  for each completed IO, finalize the IO in the cache

    while ( CRequest* prequestIO = pctls->IlIOCompleted().PrevMost() )
    {
        RequestFinalizeIO( prequestIO );

        pctls->IlIOCompleted().Remove( prequestIO );
        pctls->IlFinalizeIORequested().InsertAsNextMost( prequestIO );
    }

    //  wait for all IO issued so far but asynchronously

    while ( CRequest* prequestIO = pctls->IlFinalizeIORequested().PrevMost() )
    {
        WaitForPendingIOAsync( pctls, prequestIO );

        pctls->IlFinalizeIORequested().Remove( prequestIO );
        pctls->IlFinalizeIOPending().InsertAsNextMost( prequestIO );
    }

    //  for each pending finalize IO, check for any IOs that are complete

    prequestIONext = NULL;
    for (   CRequest* prequestIO = pctls->IlFinalizeIOPending().PrevMost();
            prequestIO;
            prequestIO = prequestIONext )
    {
        prequestIONext = pctls->IlFinalizeIOPending().Next( prequestIO );

        if ( FCompletedIO( prequestIO ) )
        {
            pctls->IlFinalizeIOPending().Remove( prequestIO );
            pctls->IlFinalizeIOCompleted().InsertAsNextMost( prequestIO );
        }
    }

    //  for each finalized IO, release the IO range locks and trigger the IO completion

    while ( CRequest* prequestIO = pctls->IlFinalizeIOCompleted().PrevMost() )
    {
        pctls->IlFinalizeIOCompleted().Remove( prequestIO );

        prequestIO->Piorl()->Release();

        CRequest* prequestNext = NULL;
        for ( CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestNext )
        {
            prequestNext = prequestIO->IlRequestsByIO().Next( prequest );

            prequestIO->IlRequestsByIO().Remove( prequest );

            if ( prequest != prequestIO )
            {
                pctls->RemoveRequest( prequest );
            }
        }

        pctls->RemoveRequest( prequestIO );
    }

    pctls->EndAsyncIOWorker();
}

template<class I>
ERR THashedLRUKCache<I>::ErrSynchronousIO( _In_ CRequest* const prequest )
{
    ERR err = JET_errSuccess;

    //  acquire the IO range lock in terms of the cached file for this IO, waiting for IO range locks owned by other
    //  threads.  these IO range locks not only protect against chaotic concurrent IO to overlapping offsets but they
    //  also serialize all activity for that offset range including things like write back or moving cached blocks in
    //  the caching file

    prequest->WaitForIORangeLock();

    //  request our IO

    RequestIO( prequest );

    //  wait for all the IO to complete even if it is not needed for finalization

    WaitForPendingIO( prequest );

    //  finalize the IO in the cache

    RequestFinalizeIO( prequest );

    //  wait for any additional IO caused by finalization

    WaitForPendingIO( prequest );

    //  release the IO range lock

    prequest->Piorl()->Release();
    return err;
}

template<class I>
void THashedLRUKCache<I>::RequestCachedFileIO( _In_ CRequest* const prequestIO )
{
    RequestIO( prequestIO, fTrue, fFalse );
}

template<class I>
void THashedLRUKCache<I>::RequestCachingFileIO( _In_ CRequest* const prequestIO )
{
    RequestIO( prequestIO, fFalse, fTrue );
}

template<class I>
void THashedLRUKCache<I>::RequestIO( _In_ CRequest* const prequestIO )
{
    RequestIO( prequestIO, fTrue, fTrue );
}

template<class I>
void THashedLRUKCache<I>::WaitForPendingIOAsync(    _In_ CHashedLRUKCacheThreadLocalStorage<I>* const   pctls,
                                                    _In_ CRequest* const                                prequestIO )
{
    for (   CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestIO->IlRequestsByIO().Next( prequest ) )
    {
        prequest->WaitForIO( CHashedLRUKCacheThreadLocalStorage<I>::CueAsyncIOWorker, (DWORD_PTR)pctls );
    }
}

template<class I>
void THashedLRUKCache<I>::WaitForPendingIO( _In_ CRequest* const prequestIO )
{
    for (   CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestIO->IlRequestsByIO().Next( prequest ) )
    {
        prequest->WaitForIO();
    }
}

template<class I>
BOOL THashedLRUKCache<I>::FCompletedIO( _In_ CRequest* const prequestIO )
{
    for (   CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestIO->IlRequestsByIO().Next( prequest ) )
    {
        if ( !prequest->FIOCompleted() )
        {
            return fFalse;
        }
    }

    return fTrue;
}

template<class I>
void THashedLRUKCache<I>::RequestFinalizeIO( _In_ CRequest* const prequestIO )
{
    ICachedBlockSlab* pcbs = NULL;

    //  loop through every request in this IO

    for (   CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestIO->IlRequestsByIO().Next( prequest ) )
    {
        CRequest::CTraceContextScope tcScope( prequest );

        //  only finalize requests that haven't already failed.  for example, you wouldn't want to cache the result of
        //  a failed read

        if ( prequest->ErrStatus() >= JET_errSuccess )
        {
            //  finalize based on IO type

            if ( prequestIO->FRead() )
            {
                RequestFinalizeRead( prequest, &pcbs );
            }
            else
            {
                RequestFinalizeWrite( prequest, &pcbs );
            }
        }
    }

    //  update the current slab.  any failure will cause all requests in this write to fail.  failures for reads are
    //  ignored because finalizing a read is best effort
    //
    //  NOTE:  we may fail more requests than necessary because we don't know what requests were impacted by the
    //  failure to update this particular slab.  this is OK because this means the journal and thus the entire cache
    //  are about to go down

    const ERR err = ErrUpdateSlab( &pcbs );
    if ( err < JET_errSuccess )
    {
        if ( !prequestIO->FRead() )
        {
            FailIO( prequestIO, err );
        }
    }

    //  release the current slab

    ReleaseSlab( err, &pcbs );
}

template<class I>
void THashedLRUKCache<I>::RequestIO(    _In_    CRequest* const prequestIO,
                                        _In_    const BOOL      fCachedFile, 
                                        _In_    const BOOL      fCachingFile )
{
    ICachedBlockSlab* pcbs = NULL;

    //  loop through every request in this IO

    for (   CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestIO->IlRequestsByIO().Next( prequest ) )
    {
        CRequest::CTraceContextScope tcScope( prequest );

        //  request based on IO type

        if ( prequest->FRead() )
        {
            RequestRead( prequest, fCachedFile, fCachingFile, &pcbs );
        }
        else
        {
            RequestWrite( prequest, fCachedFile, fCachingFile, &pcbs );
        }
    }

    //  update the current slab.  any failure will cause all requests in this IO to fail
    //
    //  NOTE:  we may fail more requests than necessary because we don't know what requests were impacted by the
    //  failure to update this particular slab.  this is OK because this means the journal and thus the entire cache
    //  are about to go down

    const ERR err = ErrUpdateSlab( &pcbs );
    if ( err < JET_errSuccess )
    {
        FailIO( prequestIO, err );
    }

    //  release the current slab

    ReleaseSlab( err, &pcbs );
}

template<class I>
void THashedLRUKCache<I>::RequestRead(  _In_    CRequest* const             prequest,
                                        _In_    const BOOL                  fCachedFile, 
                                        _In_    const BOOL                  fCachingFile,
                                        _Inout_ ICachedBlockSlab** const    ppcbs )
{
    ERR                 err                     = JET_errSuccess;
    ICachedBlockSlab*&  pcbs                    = *ppcbs;
    QWORD               ibCachedFileDeferred    = 0;
    size_t              cbCachedFileDeferred    = 0;
    
    //  determine if we should cache this request

    const BOOL fCacheIfPossible = prequest->Cp() != cpDontCache && Pcconfig()->PctWrite() < 100;

    //  loop through the read by cached block potentially crossing many cached file blocks

    for (   QWORD ibCachedBlock = prequest->Offsets().IbStart();
            ibCachedBlock <= prequest->Offsets().IbEnd();
            ibCachedBlock += cbCachedBlock )
    {
        QWORD               ibSlab          = 0;
        CCachedBlockId      cbid;
        BOOL                fPossiblyCached = fFalse;
        CCachedBlockSlot    slot;
        BOOL                fCached         = fFalse;

        BYTE* const         pbCachedBlock   = (BYTE*)prequest->PbData() + ibCachedBlock - prequest->Offsets().IbStart();

        //  determine if we are likely to have this cached block

        Call( ErrIsPossiblyCached( prequest->Pcfte(), ibCachedBlock, &ibSlab, &cbid, &fPossiblyCached ) );

        //  if the cached block is possibly cached then determine if it is cached

        if ( fPossiblyCached )
        {
            //  change to the slab containing this block
            //
            //  NOTE:  this may experience sync reads from the caching file
            //  NOTE:  this may wait for another request to finish accessing the slab

            Call( ErrChangeSlabs( ibSlab, &pcbs ) );

            //  determine if the block is already cached

            Call( pcbs->ErrGetSlotForRead( cbid, &slot ) );
            fCached = slot.FValid();
        }

        //  emit telemetry

        if ( fCachingFile )
        {
            if ( fCached )
            {
                ReportHit( prequest->Pcfte(), ibCachedBlock, fFalse, fCacheIfPossible );
            }
            else
            {
                ReportMiss( prequest->Pcfte(), ibCachedBlock, fFalse, fCacheIfPossible );
            }
        }

        //  if the block is already cached then read the cluster

        if ( fCached )
        {
            //  reads against the caching file are requested

            if ( fCachingFile )
            {
                //  read the cluster into the output buffer

                Call( prequest->ErrReadCluster( pcbs, slot, cbCachedBlock, pbCachedBlock ) );

                //  we verify the cluster contents in RequestFinalizeRead

                //  we update our stats in RequestFinalizeRead to avoid updating the slab twice
            }
        }

        //  if the block is not already cached

        else
        {
            //  reads against the cached file are requested

            if ( fCachedFile )
            {
                //  accumulate contiguous IO against the cached file as much as possible

                if ( !cbCachedFileDeferred )
                {
                    ibCachedFileDeferred = ibCachedBlock;
                    cbCachedFileDeferred = cbCachedBlock;
                }
                else if ( ibCachedBlock == ibCachedFileDeferred + cbCachedFileDeferred )
                {
                    cbCachedFileDeferred += cbCachedBlock;
                }
                else
                {
                    Call( prequest->ErrReadCachedFile( ibCachedFileDeferred, cbCachedFileDeferred ) );

                    ibCachedFileDeferred = ibCachedBlock;
                    cbCachedFileDeferred = cbCachedBlock;
                }
            }
        }
    }
    
    //  perform any remaining deferred IO

    if ( cbCachedFileDeferred )
    {
        Call( prequest->ErrReadCachedFile( ibCachedFileDeferred, cbCachedFileDeferred ) );
    }

    //  fail the request on an error

HandleError:
    if ( err < JET_errSuccess )
    {
        prequest->Fail( err );
    }
}

template<class I>
void THashedLRUKCache<I>::RequestFinalizeRead(  _In_    CRequest* const             prequest,
                                                _Inout_ ICachedBlockSlab** const    ppcbs )
{
    ERR                 err             = JET_errSuccess;
    ICachedBlockSlab*&  pcbs            = *ppcbs;
    BOOL                fChangedSlab    = fFalse;
    QWORD               cbClean         = 0;

    //  determine if we should cache this request

    const BOOL fCacheIfPossible = prequest->Cp() != cpDontCache && Pcconfig()->PctWrite() < 100;

    //  loop through the read by cached block potentially crossing many cached file blocks

    for (   QWORD ibCachedBlock = prequest->Offsets().IbStart();
            ibCachedBlock <= prequest->Offsets().IbEnd();
            ibCachedBlock += cbCachedBlock )
    {
        QWORD               ibSlab          = 0;
        CCachedBlockId      cbid;
        BOOL                fPossiblyCached = fFalse;
        CCachedBlockSlot    slot;
        BOOL                fCached         = fFalse;

        BYTE* const         pbCachedBlock   = (BYTE*)prequest->PbData() + ibCachedBlock - prequest->Offsets().IbStart();

        //  determine if we are likely to have this cached block

        Call( ErrIsPossiblyCached( prequest->Pcfte(), ibCachedBlock, &ibSlab, &cbid, &fPossiblyCached ) );

        //  if the cached block is possibly cached then determine if it is cached.  otherwise, if we want to cache it
        //  then ensure that we check to see if it is already cached

        if ( fPossiblyCached || fCacheIfPossible )
        {
            //  get the slab for this cluster

            Call( ErrChangeSlabs( ibSlab, &pcbs, &fChangedSlab ) );

            cbClean = fChangedSlab ? 0 : cbClean;

            //  determine if the block is already cached

            Call( pcbs->ErrGetSlotForRead( cbid, &slot ) );
            fCached = slot.FValid();
        }

        //  if the block is already cached then verify the read

        if ( fCached )
        {
            //  verify the data we read

            err = pcbs->ErrVerifyCluster( slot, cbCachedBlock, pbCachedBlock );
            if ( err < JET_errSuccess )
            {
                Error( ErrUnexpectedDataReadFailure(    prequest->Pcfte(),
                                                        COffsets( ibCachedBlock, ibCachedBlock + ( cbCachedBlock - 1 ) ),
                                                        "RequestFinalizeRead",
                                                        err,
                                                        FVerificationError( err ) ?
                                                            err :
                                                            ErrERRCheck( JET_errDiskIO ) ) );
            }

            //  if we want to touch this cluster due to the read then update the slot

            if ( fCacheIfPossible )
            {
                Call( pcbs->ErrUpdateSlot( slot ) );
            }
        }

        //  if the block is not already cached and we want to cache it then cache it

        else if ( fCacheIfPossible )
        {
            //  ensure there is enough room in the slab to hold this cached file block
            //
            //  NOTE:  this can wait on cached file IO if the async clean process has fallen behind

            Call( ErrCleanSlab( prequest, &pcbs, fTrue, ibCachedBlock, &cbClean ) );

            //  try to get a slot to cache this cluster

            Call( pcbs->ErrGetSlotForCache( cbid, cbCachedBlock, pbCachedBlock, &slot ) );
            if ( slot.FValid() )
            {
                //  update the slot corresponding to this cluster

                Call( pcbs->ErrUpdateSlot( slot ) );
                cbClean -= cbCachedBlock;

                //  write the data we are caching to the cluster

                Call( prequest->ErrWriteCluster( pcbs, slot, cbCachedBlock, pbCachedBlock ) );
            }
        }
    }

HandleError:

    //  NOTE:  caching reads is best effort only.  we will not fail the original IO on an error.  note that this may
    //  result in partial reads being cached.  note that an error on write to the journal will cause the cache to go
    //  down soon and the failed portion of the journal will be excluded (past the tail)

    return;  //  required by the HandleError label
}

template<class I>
ERR THashedLRUKCache<I>::ErrUnexpectedDataReadFailure(  _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                                        _In_ const COffsets                             offsets,
                                                        _In_ const char* const                          szFunction,
                                                        _In_ const ERR                                  errFromCall,
                                                        _In_ const ERR                                  errToReturn )
{
    const QWORD ibOffset                                        = rounddn( offsets.IbStart(), pcfte->CbBlockSize() );
    const DWORD cbLength                                        = pcfte->CbBlockSize();
    WCHAR       wszCachedFilePath[ IFileSystemAPI::cchPathMax ] = { 0 };

    CallS( pcfte->Pff()->ErrPath( wszCachedFilePath ) );

    OSTraceSuspendGC();
    BlockCacheNotableEvent( OSFormat(   "UnexpectedDataReadFailure:%hs:%i:%i", 
                                        szFunction, 
                                        errFromCall, 
                                        errToReturn ) );
    OSTraceResumeGC();

    OSFileIIOReportError(   Pfsconfig(),
                            wszCachedFilePath,
                            fFalse,
                            ibOffset,
                            cbLength,
                            errFromCall,
                            0,
                            0 );

    return errToReturn;
}

template<class I>
void THashedLRUKCache<I>::RequestWrite( _In_    CRequest* const             prequest,
                                        _In_    const BOOL                  fCachedFile, 
                                        _In_    const BOOL                  fCachingFile,
                                        _Inout_ ICachedBlockSlab** const    ppcbs )
{
    ERR                 err                     = JET_errSuccess;
    ICachedBlockSlab*&  pcbs                    = *ppcbs;
    BOOL                fChangedSlab            = fFalse;
    QWORD               cbClean                 = 0;
    QWORD               ibCachedFileDeferred    = 0;
    size_t              cbCachedFileDeferred    = 0;

    //  loop through the write by cached block potentially crossing many cached file blocks

    for (   QWORD ibCachedBlock = prequest->Offsets().IbStart();
            ibCachedBlock <= prequest->Offsets().IbEnd();
            ibCachedBlock += cbCachedBlock )
    {
        QWORD               ibSlab = 0;
        CCachedBlockId      cbid;
        BOOL                fPossiblyCached = fFalse;
        CCachedBlockSlot    slot;
        BOOL                fCached         = fFalse;

        const BYTE* const   pbCachedBlock   = prequest->PbData() + ibCachedBlock - prequest->Offsets().IbStart();

        //  determine if we should cache this block
        //
        //  NOTE:  we do not cache writes to sparse regions of a file to force them to be reallocated.  this is
        //  required to maintain file meta-data parity with uncached files

        const BOOL          fCacheIfPossible =   (  prequest->Cp() != cpDontCache &&
                                                    Pcconfig()->PctWrite() > 0 &&
                                                    !prequest->Pcfte()->FSparse( ibCachedBlock, cbCachedBlock ) ) ||
                                                prequest->Cp() == cpPinned;

        //  determine if we are likely to have this cached block

        Call( ErrIsPossiblyCached( prequest->Pcfte(), ibCachedBlock, &ibSlab, &cbid, &fPossiblyCached ) );

        //  if the cached block is possibly cached then determine if it is cached.  otherwise, if we want to cache it
        //  then ensure that we check to see if it is already cached

        if ( fPossiblyCached || fCacheIfPossible )
        {
            //  change to the slab containing this block
            //
            //  NOTE:  this may experience sync reads from the caching file
            //  NOTE:  this may wait for another request to finish accessing the slab

            Call( ErrChangeSlabs( ibSlab, &pcbs, &fChangedSlab ) );

            if ( fChangedSlab && ibCachedBlock % min( m_pch->CbCachedFilePerSlab(), prequest->Pcfte()->CbBlockSize() ) )
            {
                BlockCacheNotableEvent( "TornWriteOpportunity" );
            }

            cbClean = fChangedSlab ? 0 : cbClean;

            //  determine if the block is already cached

            Call( pcbs->ErrGetSlotForRead( cbid, &slot ) );
            fCached = slot.FValid();
        }

        //  emit telemetry

        if ( fCachingFile )
        {
            if ( fCached )
            {
                ReportHit( prequest->Pcfte(), ibCachedBlock, fFalse, fCacheIfPossible );
            }
            else
            {
                ReportMiss( prequest->Pcfte(), ibCachedBlock, fFalse, fCacheIfPossible );
            }

            ReportUpdate( prequest->Pcfte(), ibCachedBlock );

            if ( !fCached && !fCacheIfPossible )
            {
                ReportWrite( prequest->Pcfte(), ibCachedBlock, fTrue );
            }
        }

        //  if the block is already cached or we want to cache it then do so

        if ( fCached || fCacheIfPossible )
        {
            //  writes against the caching file are requested

            if ( fCachingFile )
            {
                //  ensure there is enough room in the slab to hold this cached file block
                //
                //  NOTE:  this can wait on cached file IO if the async clean process has fallen behind

                const BOOL fUpdatedBeforeClean = pcbs->FUpdated();

                Call( ErrCleanSlab( prequest, &pcbs, fFalse, ibCachedBlock, &cbClean ) );

                if (    fUpdatedBeforeClean &&
                        !pcbs->FUpdated() &&
                        ibCachedBlock % min( m_pch->CbCachedFilePerSlab(), prequest->Pcfte()->CbBlockSize() ) != 0 )
                {
                    BlockCacheNotableEvent( "TornWriteOpportunity2" );
                }

                //  try to get a slot to write this cluster

                Call( pcbs->ErrGetSlotForWrite( cbid, cbCachedBlock, pbCachedBlock, &slot ) );
                if ( !slot.FValid() )
                {
                    Error( ErrBlockCacheInternalError( "HashedLRUKCacheRequestWriteNoSlotAvailable" ) );
                }

                //  if the caching policy requires that this cached block never be written back then modify the slot to
                //  indicate that

                if ( prequest->Cp() == cpPinned )
                {
                    CCachedBlockSlot::Pin( slot, &slot );
                }

                //  update the slot corresponding to this cluster

                Call( pcbs->ErrUpdateSlot( slot ) );
                cbClean -= cbCachedBlock;

                //  write the data we are caching to the cluster

                Call( prequest->ErrWriteCluster( pcbs, slot, cbCachedBlock, pbCachedBlock ) );
            }
        }

        //  if we are not caching the block then pass the write through to the cached file

        else
        {
            //  writes against the cached file are requested

            if ( fCachedFile )
            {
                //  accumulate contiguous IO against the cached file as much as possible

                if ( !cbCachedFileDeferred )
                {
                    ibCachedFileDeferred = ibCachedBlock;
                    cbCachedFileDeferred = cbCachedBlock;
                }
                else if ( ibCachedBlock == ibCachedFileDeferred + cbCachedFileDeferred )
                {
                    cbCachedFileDeferred += cbCachedBlock;
                }
                else
                {
                    Call( prequest->ErrWriteCachedFile( ibCachedFileDeferred, cbCachedFileDeferred ) );

                    ibCachedFileDeferred = ibCachedBlock;
                    cbCachedFileDeferred = cbCachedBlock;
                }
            }
        }
    }

    //  perform any remaining deferred IO

    if ( cbCachedFileDeferred )
    {
        Call( prequest->ErrWriteCachedFile( ibCachedFileDeferred, cbCachedFileDeferred ) );
    }

    //  fail the request on an error

HandleError:
    if ( err < JET_errSuccess )
    {
        prequest->Fail( err );
    }
}

template<class I>
void THashedLRUKCache<I>::RequestFinalizeWrite( _In_    CRequest* const             prequest,
                                                _Inout_ ICachedBlockSlab** const    ppcbs )
{
    //  if we just wrote to a sparse file then try to update its sparse map to reflect the write so that we can cache
    //  it next time.  it is ok if this fails (due to OOM) because that will only result in a performance drop
    //  due to an unnecessary write through until we can fix the sparse map

    if ( prequest->Pcfte()->FSparse() )
    {
        BOOL fUpdate = fFalse;
        for (   QWORD ibCachedBlock = prequest->Offsets().IbStart();
                !fUpdate && ibCachedBlock <= prequest->Offsets().IbEnd();
                ibCachedBlock += cbCachedBlock )
        {
            fUpdate = fUpdate || prequest->Pcfte()->FSparse( ibCachedBlock, cbCachedBlock );
        }

        if ( fUpdate )
        {
            (void)prequest->Pcfte()->ErrUpdateSparseMapForWrite( prequest->Offsets() );
        }
    }

    //  NOTE:  this is where you would cause a journal flush for files that support write through
}

template<class I>
ERR THashedLRUKCache<I>::ErrCleanSlab(  _In_    CRequest* const             prequest,
                                        _Inout_ ICachedBlockSlab** const    ppcbs,
                                        _In_    const BOOL                  fRead,
                                        _In_    const QWORD                 ib,
                                        _Inout_ QWORD* const                pcbClean )
{
    ERR         err                 = JET_errSuccess;

    //  determine how much we should clean to cache the rest of this slice of this block

    const QWORD cbBlockSize         = prequest->Pcfte()->CbBlockSize();
    const QWORD ibBlock             = ib % cbBlockSize;

    const QWORD cbCachedFilePerSlab = m_pch->CbCachedFilePerSlab();
    const QWORD ibSlice             = ib % cbCachedFilePerSlab;

    const QWORD cbCleanBlock        = cbBlockSize - ibBlock;
    const QWORD cbCleanSlice        = min( cbBlockSize, cbCachedFilePerSlab - ibSlice );
    const QWORD cbClean             = min( cbCleanBlock, cbCleanSlice );

    const QWORD cCleanMax           = m_pch->CbCachingFilePerSlab() / cbBlockSize;

    //  if we don't already have enough then clean until we do or we cannot clean any more

    QWORD   cClean                  = 0;
    QWORD   cbWriteBackFailedPrev   = 0;
    QWORD   cbWriteBackFailed       = 0;

    while ( cbClean > *pcbClean &&
            ( cClean == 0 || cbWriteBackFailed > cbWriteBackFailedPrev ) &&
            cClean < cCleanMax )
    {
        cbWriteBackFailedPrev = cbWriteBackFailed;

        Call( CCleanSlabVisitor::ErrExecute(    this, 
                                                prequest, 
                                                ppcbs, 
                                                fRead, 
                                                cbClean + cbWriteBackFailedPrev,
                                                pcbClean, 
                                                &cbWriteBackFailed ) );

        cClean++;
    }

    //  if we failed to clean enough then fail

    if ( cbClean > *pcbClean )
    {
        Error( ErrBlockCacheInternalError( "ErrCleanSlab" ) );
    }

HandleError:
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrIsPossiblyCached(   _In_    CHashedLRUKCachedFileTableEntry<I>* pcfte,
                                                _In_    const QWORD                         ibCachedBlock,
                                                _Out_   QWORD* const                        pibSlab,
                                                _Out_   CCachedBlockId* const               pcbid,
                                                _Out_   BOOL* const                         pfPossiblyCached )
{
    ERR     err             = JET_errSuccess;
    QWORD   ibSlab          = 0;
    BOOL    fPossiblyCached = fFalse;

    *pibSlab = 0;
    new( pcbid ) CCachedBlockId();
    *pfPossiblyCached = fFalse;

    //  compute the cached block id for this offset

    const CCachedBlockId cbid(  pcfte->Volumeid(),
                                pcfte->Fileid(),
                                pcfte->Fileserial(),
                                (CachedBlockNumber)( ibCachedBlock / cbCachedBlock ) );

    //  determine the slab that should hold this cached block

    Call( m_pcbsmHash->ErrGetSlabForCachedBlock( cbid, &ibSlab ) );

    //  determine if it is possible that we have this cached block in the cache

    fPossiblyCached = m_pcbpf->FPossiblyContains( ibSlab, cbid );

    //  return the results

    *pibSlab = ibSlab;
    new( pcbid ) CCachedBlockId( cbid.Volumeid(), cbid.Fileid(), cbid.Fileserial(), cbid.Cbno() );
    *pfPossiblyCached = fPossiblyCached;

HandleError:
    if ( err < JET_errSuccess )
    {
        *pibSlab = 0;
        new( pcbid ) CCachedBlockId();
        *pfPossiblyCached = fFalse;
    }
    return err;
}

template<class I>
void THashedLRUKCache<I>::FailIO( _In_ CRequest* const prequestIO, _In_ const ERR err )
{
    //  loop through every request in this IO

    for (   CRequest* prequest = prequestIO->IlRequestsByIO().PrevMost();
            prequest;
            prequest = prequestIO->IlRequestsByIO().Next( prequest ) )
    {
        //  fail the request

        prequest->Fail( err );
    }
}

template<class I>
ERR THashedLRUKCache<I>::ErrInitSlabWriteBackHash()
{
    ERR err = JET_errSuccess;

    if ( m_slabWriteBackHash.ErrInit( 5.0, 1.0 ) == CSlabWriteBackHash::ERR::errOutOfMemory )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

HandleError:
    return err;
}

template<class I>
void THashedLRUKCache<I>::TermSlabWriteBackHash()
{
    if ( m_initOnceSlabWriteBackHash.FIsInit() )
    {
        CSlabWriteBackHash::CLock   lock;
        CSlabWriteBackEntry         entry;
        CSlabWriteBackHash::ERR     errSlabWriteBackHash;

        m_slabWriteBackHash.BeginHashScan( &lock );

        while ( ( errSlabWriteBackHash = m_slabWriteBackHash.ErrMoveNext( &lock ) ) != CSlabWriteBackHash::ERR::errNoCurrentEntry )
        {
            errSlabWriteBackHash = m_slabWriteBackHash.ErrRetrieveEntry( &lock, &entry );
            Assert( errSlabWriteBackHash == CSlabWriteBackHash::ERR::errSuccess );

            delete entry.Pswb();
        }

        m_slabWriteBackHash.EndHashScan( &lock );

        m_slabWriteBackHash.Term();
    }
}

template<class I>
void THashedLRUKCache<I>::ReleaseCompletedSlabWriteBacks()
{
    while ( m_ilSlabWriteBacksCompleted.PrevMost() )
    {
        CSlabWriteBack* const pswb = m_ilSlabWriteBacksCompleted.PrevMost();
        m_ilSlabWriteBacksCompleted.Remove( pswb );
        delete pswb;
    }
}

template<class I>
ERR THashedLRUKCache<I>::ErrGetSlabInternal(    _In_    const QWORD                 ibSlab,
                                                _In_    const BOOL                  fIgnoreVerificationErrors,
                                                _In_    const BOOL                  fForSlabWriteBack,
                                                _Out_   ICachedBlockSlab** const    ppcbs )
{
    ERR                                     err             = JET_errSuccess;
    const BOOL                              fWait           = !fForSlabWriteBack;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls           = NULL;
    ICachedBlockSlabManager*                pcbsm           = NULL;
    BOOL                                    fRelease        = fFalse;
    BOOL                                    fUnregisterWait = fFalse;
    ERR                                     errSlab         = JET_errSuccess;
    ICachedBlockSlab*                       pcbs            = NULL;

    *ppcbs = NULL;

    Call( THashedLRUKCacheBase<I>::ErrGetThreadLocalStorage( &pctls ) );

    //  determine which slab manager owns this slab

    if ( FHashSlab( ibSlab ) )
    {
        pcbsm = m_pcbsmHash;
    }
    else if ( FJournalSlab( ibSlab ) )
    {
        pcbsm = m_pcbsmJournal;
    }
    else
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheUnknownSlabType" ) );
    }

    //  if this is a journal slab then note we are acquiring it

    if ( pcbsm == m_pcbsmJournal )
    {
        ReferenceJournalSlab( ibSlab );
        fRelease = fTrue;
    }

    //  register as a waiter for the slab

    if ( fWait && !fForSlabWriteBack )
    {
        RegisterOpenSlabWait( pctls, ibSlab );
        fUnregisterWait = fTrue;
    }

    //  get the slab

    errSlab = pcbsm->ErrGetSlab( ibSlab, fWait, fIgnoreVerificationErrors, &pcbs );
    err = fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( errSlab ) : errSlab;
    if ( err < JET_errSuccess )
    {
        Call( ErrUnexpectedMetadataReadFailure( "GetSlabById", err, ErrERRCheck( JET_errDiskIO ) ) );
    }

    fRelease = fRelease && !pcbs;

    //  update our cached block presence filter if necessary

    if ( pcbsm == m_pcbsmHash && pcbs )
    {
        //  only update if we are not in recovery where we unilaterally overwrite slab state

        if ( m_fRecovered )
        {
            Call( m_pcbpf->ErrLoad( pcbs ) );
        }
    }

    //  unregister as a waiter for the slab

    if ( fUnregisterWait )
    {
        UnregisterOpenSlabWait( pctls, ibSlab );
        fUnregisterWait = fFalse;
    }

    //  register our open slab

    if ( pcbs && !fForSlabWriteBack )
    {
        RegisterOpenSlab( pctls, pcbs );
    }

    //  return the slab

    *ppcbs = pcbs;
    pcbs = NULL;

    err = errSlab;

HandleError:
    if ( fUnregisterWait )
    {
        UnregisterOpenSlabWait( pctls, ibSlab );
    }
    if ( fRelease )
    {
        ReleaseJournalSlab( ibSlab );
    }
    ReleaseSlab( err, &pcbs );
    if ( ( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( err ) : err ) < JET_errSuccess )
    {
        ReleaseSlab( err, ppcbs );
    }
    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrUnexpectedMetadataReadFailure(  _In_ const char* const  szFunction,
                                                            _In_ const ERR          errFromCall,
                                                            _In_ const ERR          errToReturn )
{
    OSTraceSuspendGC();
    BlockCacheNotableEvent( OSFormat(   "UnexpectedMetadataReadFailure:%hs:%i:%i", 
                                        szFunction, 
                                        errFromCall, 
                                        errToReturn ) );
    OSTraceResumeGC();

    return errToReturn;
}

template<class I>
void THashedLRUKCache<I>::ReleaseSlab( _In_ const ERR err, _Inout_ ICachedBlockSlab** const ppcbs )
{
    ICachedBlockSlab* const pcbs            = *ppcbs;
    QWORD                   ibSlab          = 0;
    BOOL                    fJournalSlab    = fFalse;

    *ppcbs = NULL;

    //  we have a slab

    if ( pcbs )
    {
        //  determine if this is a journal slab

        CallS( pcbs->ErrGetPhysicalId( &ibSlab ) );
        fJournalSlab = FJournalSlab( ibSlab );

        //  unregister our open slab

        UnregisterOpenSlab( pcbs );

        //  if we are not in an error state then the slab had better not have any updates that haven't been accepted

        EnforceSz( !pcbs->FUpdated() || err < JET_errSuccess, "ReleaseSlab" );

        //  release the slab

        delete pcbs;

        //  if this was a journal slab then note that it has been released

        if ( fJournalSlab )
        {
            ReleaseJournalSlab( ibSlab );
        }
    }
}

template<class I>
BOOL THashedLRUKCache<I>::FAnyOpenSlab()
{
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls           = NULL;
    BOOL                                    fAnyOpenSlab    = fFalse;

    (void)ErrGetThreadLocalStorage( &pctls );

    fAnyOpenSlab = pctls && pctls->FAnyOpenSlab();

    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );

    return fAnyOpenSlab;
}

template<class I>
ERR THashedLRUKCache<I>::ErrStartStateAccessAttempt( _Out_ CMeteredSection::Group* const pgroup )
{
    ERR                                     err         = JET_errSuccess;
    CMeteredSection::Group                  group       = CMeteredSection::groupInvalidNil;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls       = NULL;

    *pgroup = CMeteredSection::groupInvalidNil;

    //  start our state access attempt

    group = m_msStateAccess.Enter();

    //  check if state access is suspended

    if ( !m_rgpmsigStateAccess[ group ]->FIsSet() )
    {
        //  get our thread local storage

        Call( ErrGetThreadLocalStorage( &pctls ) );

        //  register this thread as suspended

        Call( ErrSuspendThreadFromStateAccess( group, pctls ) );

        //  ensure any pending IO on the caching file is issued before we suspend

        Call( PffCaching()->ErrIOIssue() );

        //  wait for state access to be resumed

        m_rgpmsigStateAccess[ group ]->Wait();
    }

    //  return our group

    *pgroup = group;
    group = CMeteredSection::groupInvalidNil;

HandleError:
    EndStateAccessAttempt( &group );
    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
    if ( err < JET_errSuccess )
    {
        EndStateAccessAttempt( pgroup );
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrSuspendThreadFromStateAccess(   _In_ const CMeteredSection::Group                   group,
                                                            _In_ CHashedLRUKCacheThreadLocalStorage<I>* const   pctls )
{
    ERR     err     = JET_errSuccess;
    BOOL    fLeave  = fFalse;

    m_critSuspendedThreads.Enter();
    fLeave = fTrue;

    CArray<CHashedLRUKCacheThreadLocalStorage<I>*>* const parray = &m_rgarraySuspendedThreads[ group ];
    Call( ErrToErr<CArray<CHashedLRUKCacheThreadLocalStorage<I>*>>( parray->ErrSetEntry( parray->Size(), pctls ) ) );

    m_critSuspendedThreads.Leave();
    fLeave = fFalse;

HandleError:
    if ( fLeave )
    {
        m_critSuspendedThreads.Leave();
    }
    return err;
}

template<class I>
void THashedLRUKCache<I>::EndStateAccessAttempt( _Inout_ CMeteredSection::Group* const pgroup )
{
    if ( *pgroup != CMeteredSection::groupInvalidNil )
    {
        m_msStateAccess.Leave( *pgroup );
        *pgroup = CMeteredSection::groupInvalidNil;
    }
}

template<class I>
void THashedLRUKCache<I>::QuiesceStateAccess(   _In_ const PfnStateAccessQuiesced   pfnStateAccessQuiesced,
                                                _In_ const DWORD_PTR                dwCompletionKey)
{
    //  async quiesce state access

    if ( m_semQuiesceStateAccess.FTryAcquire() )
    {
        m_msStateAccess.Partition( pfnStateAccessQuiesced, dwCompletionKey );
    }
}

template<class I>
void THashedLRUKCache<I>::ResumeStateAccess()
{
    //  reset the suspended threads for the next cycle

    CArray<CHashedLRUKCacheThreadLocalStorage<I>*>* const parray = &m_rgarraySuspendedThreads[ m_msStateAccess.GroupInactive() ];
    CallS( ErrToErr<CArray<CHashedLRUKCacheThreadLocalStorage<I>*>>( parray->ErrSetSize( 0 ) ) );

    //  allow state access to continue

    m_rgpmsigStateAccess[ m_msStateAccess.GroupInactive() ]->Reset();
    m_rgpmsigStateAccess[ m_msStateAccess.GroupActive() ]->Set();

    m_semQuiesceStateAccess.Release();
}

template<class I>
ERR THashedLRUKCache<I>::ErrSuspendBlockedThreadsFromStateAccess()
{
    ERR err = JET_errSuccess;

    //  reset error state

    m_errSuspendBlockedThreads = JET_errSuccess;

    //  walk all thread local storage

    VisitThreadLocalStorage( (THashedLRUKCacheBase<I>::PfnVisitTls)FSuspendBlockedThreadFromStateAccess_, DWORD_PTR( this ) );

    //  sync with error state from enumeration

    Call( m_errSuspendBlockedThreads );

HandleError:
    return err;
}

template<class I>
BOOL THashedLRUKCache<I>::FSuspendBlockedThreadFromStateAccess( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls )
{
    ERR                                                     err                         = JET_errSuccess;
    const QWORD                                             ibSlabWait                  = pctls->IbSlabWait();
    const CMeteredSection::Group                            group                       = m_msStateAccess.GroupActive();
    CArray<CHashedLRUKCacheThreadLocalStorage<I>*>* const   parraySuspendedThreads      = &m_rgarraySuspendedThreads[ group ];
    BOOL                                                    fSuspendedThread            = fFalse;
    BOOL                                                    fBlockedBySuspendedThread   = fFalse;

    if ( ibSlabWait )
    {
        m_critSuspendedThreads.Enter();

        for ( size_t i = 0; i < parraySuspendedThreads->Size(); i++ )
        {
            fSuspendedThread = fSuspendedThread || parraySuspendedThreads->Entry( i ) == pctls;
            fBlockedBySuspendedThread = (   fBlockedBySuspendedThread ||
                                            parraySuspendedThreads->Entry( i )->PcbsOpenSlab( ibSlabWait ) != NULL );
        }

        m_critSuspendedThreads.Leave();

        if ( !fSuspendedThread && fBlockedBySuspendedThread )
        {
            Call( ErrSuspendThreadFromStateAccess( group, pctls ) );
        }
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        m_errSuspendBlockedThreads = m_errSuspendBlockedThreads < JET_errSuccess ? m_errSuspendBlockedThreads : err;
    }
    return m_errSuspendBlockedThreads >= JET_errSuccess;
}

template<class I>
ERR THashedLRUKCache<I>::ErrRegisterOpenSlab( _In_ ICachedBlockSlab* const pcbs )
{
    ERR                                     err     = JET_errSuccess;
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls   = NULL;

    Call( ErrGetThreadLocalStorage( &pctls ) );

    RegisterOpenSlab( pctls, pcbs );

HandleError:
    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
    return err;
}

template<class I>
void THashedLRUKCache<I>::RegisterOpenSlab( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls, _In_ ICachedBlockSlab* const pcbs )
{
    pctls->RegisterOpenSlab( pcbs );
}

template<class I>
void THashedLRUKCache<I>::UnregisterOpenSlab( _In_ ICachedBlockSlab* const pcbs )
{
    CHashedLRUKCacheThreadLocalStorage<I>*  pctls   = NULL;

    //  unregister our open slab

    CallS( ErrGetThreadLocalStorage( &pctls ) );

    if ( pctls && pctls->FOpenSlab( pcbs ) )
    {
        pctls->UnregisterOpenSlab( pcbs );
    }

    CHashedLRUKCacheThreadLocalStorage<I>::Release( &pctls );
}

template<class I>
void THashedLRUKCache<I>::RegisterOpenSlabWait( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls, _In_ const QWORD ibSlab )
{
    pctls->RegisterOpenSlabWait( ibSlab );
}

template<class I>
void THashedLRUKCache<I>::UnregisterOpenSlabWait( _In_ CHashedLRUKCacheThreadLocalStorage<I>* const pctls, _In_ const QWORD ibSlab )
{
    pctls->UnregisterOpenSlabWait( ibSlab );
}

template<class I>
ERR THashedLRUKCache<I>::ErrGetJournalSlab( _Out_ ICachedBlockSlab** const ppcbs )
{
    ERR                 err     = JET_errSuccess;
    ICachedBlockSlab*   pcbs    = NULL;

    *ppcbs = NULL;

    //  choose a journal slab randomly to provide scalability and to ensure all clusters can be used
    //
    //  NOTE:  this will acquire an extra ref count on the journal slab that we will release at the end

    UINT uiRandom;
    const BOOL fSuccess = rand_s( &uiRandom ) == 0;
    Assert( fSuccess );

    const size_t        icrefJournalSlabStart   = uiRandom % m_ccrefJournalSlab;
    const QWORD         cbSlab                  = CbChunkPerSlab();
    QWORD               ibSlab                  = 0;

    for ( size_t dicrefJournalSlab = 0; dicrefJournalSlab < m_ccrefJournalSlab; dicrefJournalSlab++ )
    {
        const size_t icrefJournalSlab = ( icrefJournalSlabStart + dicrefJournalSlab ) % m_ccrefJournalSlab;
        if ( AtomicCompareExchange( (LONG*)&m_rgcrefJournalSlab[ icrefJournalSlab ], 0, 1 ) == 0 )
        {
            ibSlab = m_pch->IbChunkJournal() + icrefJournalSlab * cbSlab;
            break;
        }
    }

    if ( ibSlab == 0 )
    {
        ibSlab = m_pch->IbChunkJournal() + icrefJournalSlabStart * cbSlab;
        ReferenceJournalSlab( ibSlab );
    }

    //  get the chosen journal slab

    Call( ErrGetSlab( ibSlab, &pcbs ) );

    //  return the journal slab

    *ppcbs = pcbs;
    pcbs = NULL;

    //  release the extra ref count on the journal slab

HandleError:
    ReleaseJournalSlab( ibSlab );
    ReleaseSlab( err, &pcbs );
    if ( err < JET_errSuccess )
    {
        ReleaseSlab( err, ppcbs );
    }
    return err;
}

template<class I>
size_t THashedLRUKCache<I>::IcrefJournalSlab( _In_ const QWORD ibSlab )
{
    return (size_t)( ( ibSlab - m_pch->IbChunkJournal() ) / CbChunkPerSlab() );
}

template<class I>
void THashedLRUKCache<I>::ReferenceJournalSlab( _In_ const QWORD ibSlab )
{
    AtomicIncrement( (LONG*)&m_rgcrefJournalSlab[ IcrefJournalSlab( ibSlab ) ] );
}

template<class I>
void THashedLRUKCache<I>::ReleaseJournalSlab( _In_ const QWORD ibSlab )
{
    AtomicDecrement( (LONG*)&m_rgcrefJournalSlab[ IcrefJournalSlab( ibSlab ) ] );
}

template<class I>
ERR THashedLRUKCache<I>::ErrEvictOrInvalidateSlot(  _In_    ICachedBlockSlab*           pcbs,
                                                    _In_    const CCachedBlockSlot&     slot,
                                                    _Inout_ ICachedBlockSlab** const    ppcbsJournal )
{
    ERR                 err                 = JET_errSuccess;
    ICachedBlockSlab*&  pcbsJournal         = *ppcbsJournal;
    CCachedBlockSlot    slotJournal;
    CCachedBlockSlot    slotJournalSwapped;
    CCachedBlockSlot    slotSwapped;
    BOOL                fUpdated            = fFalse;

    //  get the journal slab we will use
    //
    //  NOTE:  this may wait for other threads to complete access

    if ( !pcbsJournal )
    {
        Call( ErrGetJournalSlab( &pcbsJournal ) );
    }

    //  get an available cluster
    //
    //  NOTE:  we use ErrGetSlotForWrite here because it will find any slot that is !FValid and it will
    //  set the slot to be FValid and FDirty.  we will use the FDirty flag to indicate clusters that
    //  this clean operation is using and we will clear those at the end of the operation by marking
    //  them as written back.  we will use the FValid flag to indicate clusters that cannot be used
    //  until the journal is flushed.  if there are no slots available then we will flush the journal
    //  and evict all FValid and !FDirty slots and try again

    Call( pcbsJournal->ErrGetSlotForWrite( s_cbidInvalid, 0, NULL, &slotJournal ) );

    if ( !slotJournal.FValid() )
    {
        //  flush the journal twice to advance the durable for writeback pointer so that we know it is
        //  safe to reuse the clusters

        Call( m_pj->ErrFlush() );
        Call( m_pj->ErrFlush() );

        //  mark all used clusters as free

        Call( CMarkUsedJournalClustersAsFreeSlabVisitor::ErrExecute( pcbsJournal ) );
    }

    Call( pcbsJournal->ErrGetSlotForWrite( s_cbidInvalid, 0, NULL, &slotJournal ) );
    Assert( slotJournal.FValid() );
    Assert( slotJournal.FDirty() );

    //  swap the clusters backing these slots

    CCachedBlockSlot::SwapClusters( slot, slotJournal, &slotSwapped, &slotJournalSwapped );

    //  update the evict slot to swap its cluster with the journal slot

    Call( pcbs->ErrUpdateSlot( slotSwapped ) );
    fUpdated = fTrue;

    //  update the journal slot to swap its cluster with the evict slot

    Call( pcbsJournal->ErrUpdateSlot( slotJournalSwapped ) );

    //  if we failed partway through the swap then we must back out the swap
    //
    //  NOTE:  we cannot ever leave our state such that a partial update has occurred or we might double allocate a
    //  cluster which would result in continuing corruption

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( fUpdated )
        {
            CallS( pcbs->ErrUpdateSlot( slot ) );
            CallS( pcbsJournal->ErrUpdateSlot( slotJournal ) );
        }
    }
    return err;
}

template<class I>
ERR THashedLRUKCache<I>::ErrStartThreadpool()
{
    ERR err = JET_errSuccess;

    //  initialize our thread pool.  we must have our own thread pool to prevent deadlocks from occurring when other
    //  work posted to a thread pool in this process must wait for IO that is processed by this cache.  this in turn
    //  must be separate from the OS layer's IO thread

    InitializeThreadpoolEnvironment( &m_tpcbe );
    Alloc( m_ptp = CreateThreadpool( NULL ) );
    Alloc( m_ptpcg = CreateThreadpoolCleanupGroup() );
    SetThreadpoolCallbackPool( &m_tpcbe, m_ptp );
    SetThreadpoolCallbackCleanupGroup( &m_tpcbe, m_ptpcg, NULL );

    Alloc( m_ptpwSlabWriteBack = CreateThreadpoolWork( ThreadpoolSlabWriteBackWorker_, this, &m_tpcbe ) );

HandleError:
    return err;
}

template<class I>
void THashedLRUKCache<I>::ReleaseThreadpoolState( _Inout_ TP_WORK** const pptpw )
{
    TP_WORK* const ptpw = *pptpw;

    *pptpw = NULL;

    if ( ptpw && m_ptpcg )
    {
        CloseThreadpoolWork( ptpw );
    }
}

template<class I>
void THashedLRUKCache<I>::StopThreadpool()
{
    ReleaseThreadpoolState( &m_ptpwSlabWriteBack );

    if ( m_ptpcg )
    {
        TP_CLEANUP_GROUP* const ptpcg = m_ptpcg;
        m_ptpcg = NULL;

        CloseThreadpoolCleanupGroupMembers( ptpcg, FALSE, NULL );
        CloseThreadpoolCleanupGroup( ptpcg );
    }

    if ( m_ptp )
    {
        TP_POOL* const ptp = m_ptp;
        m_ptp = NULL;

        CloseThreadpool( ptp );
    }

    m_tpcbe = { };
}

template<class I>
void THashedLRUKCache<I>::ThreadpoolAsyncIOWorker_( _Inout_ TP_CALLBACK_INSTANCE* const ptpcbi,
                                                    _Inout_ void* const                 pvContext,
                                                    _Inout_ TP_WORK* const              ptpw )
{
    PTP_WORK_CALLBACK ptpwc = ThreadpoolAsyncIOWorker_;
    Unused( ptpwc );

    CHashedLRUKCacheThreadLocalStorage<I>* const pctls = (CHashedLRUKCacheThreadLocalStorage<I>*)pvContext;
    pctls->Pc()->ThreadpoolAsyncIOWorker( ptpcbi, pctls );
}

template<class I>
void THashedLRUKCache<I>::ThreadpoolAsyncIOWorker(  _Inout_ TP_CALLBACK_INSTANCE* const                     ptpcbi,
                                                    _In_    CHashedLRUKCacheThreadLocalStorage<I>* const    pctls )
{
    AsyncIOWorker( pctls );
}

template<class I>
void THashedLRUKCache<I>::ThreadpoolSlabWriteBackWorker_(   _Inout_ TP_CALLBACK_INSTANCE* const ptpcbi,
                                                            _Inout_ void* const                 pvContext,
                                                            _Inout_ TP_WORK* const              ptpw )
{
    PTP_WORK_CALLBACK ptpwc = ThreadpoolSlabWriteBackWorker_;
    Unused( ptpwc );

    THashedLRUKCache<I>* const pc = (THashedLRUKCache<I>*)pvContext;
    pc->ThreadpoolSlabWriteBackWorker( ptpcbi );
}

template<class I>
void THashedLRUKCache<I>::ThreadpoolSlabWriteBackWorker( _Inout_ TP_CALLBACK_INSTANCE* const ptpcbi )
{
    AsyncSlabWriteBackWorker();

    Assert( !FAnyOpenSlab() );
}

template<class I>
void THashedLRUKCache<I>::ReportMiss(   _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_ const QWORD                                ibCachedBlock,
                                        _In_ const BOOL                                 fRead,
                                        _In_ const BOOL                                 fCacheIfPossible )
{
    const DWORD cbBlockSize             = pcfte->CbBlockSize();
    const BOOL  fCachedFileBlockAligned = ibCachedBlock % cbBlockSize == 0;

    if ( fCachedFileBlockAligned )
    {
        THashedLRUKCacheBase<I>::ReportMiss( pcfte, ibCachedBlock, cbBlockSize, fRead, fCacheIfPossible );
    }
}

template<class I>
void THashedLRUKCache<I>::ReportHit(    _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_ const QWORD                                ibCachedBlock,
                                        _In_ const BOOL                                 fRead,
                                        _In_ const BOOL                                 fCacheIfPossible )
{
    const DWORD cbBlockSize             = pcfte->CbBlockSize();
    const BOOL  fCachedFileBlockAligned = ibCachedBlock % cbBlockSize == 0;

    if ( fCachedFileBlockAligned )
    {
        THashedLRUKCacheBase<I>::ReportHit( pcfte, ibCachedBlock, cbBlockSize, fRead, fCacheIfPossible );
    }
}

template<class I>
void THashedLRUKCache<I>::ReportUpdate( _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_ const QWORD                                ibCachedBlock )
{
    const DWORD cbBlockSize             = pcfte->CbBlockSize();
    const BOOL  fCachedFileBlockAligned = ibCachedBlock % cbBlockSize == 0;

    if ( fCachedFileBlockAligned )
    {
        THashedLRUKCacheBase<I>::ReportUpdate( pcfte, ibCachedBlock, cbBlockSize );
    }
}

template<class I>
void THashedLRUKCache<I>::ReportWrite(  _In_ CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_ const QWORD                                ibCachedBlock,
                                        _In_ const BOOL                                 fReplacementPolicy )
{
    const DWORD cbBlockSize             = pcfte->CbBlockSize();
    const BOOL  fCachedFileBlockAligned = ibCachedBlock % cbBlockSize == 0;

    if ( fCachedFileBlockAligned )
    {
        THashedLRUKCacheBase<I>::ReportWrite( pcfte, ibCachedBlock, cbBlockSize, fReplacementPolicy );
    }
}

template<class I>
void THashedLRUKCache<I>::ReportEvict(  _In_opt_    CHashedLRUKCachedFileTableEntry<I>* const  pcfte,
                                        _In_        const QWORD                                ibCachedBlock,
                                        _In_        const BOOL                                 fReplacementPolicy )
{
    //  UNDONE:  if we don't have the cached file then we cannot emit our telemetry

    if ( !pcfte )
    {
        return;
    }

    const DWORD cbBlockSize             = pcfte->CbBlockSize();
    const BOOL  fCachedFileBlockAligned = ibCachedBlock % cbBlockSize == 0;

    if ( fCachedFileBlockAligned )
    {
        THashedLRUKCacheBase<I>::ReportEvict( pcfte, ibCachedBlock, cbBlockSize, fReplacementPolicy );
    }
}

template< class I >
const CCachedBlockId THashedLRUKCache<I>::s_cbidInvalid = CCachedBlockId( volumeidInvalid, fileidInvalid, fileserialInvalid, cbnoInvalid );

//  CHashedLRUKCache:  concrete THashedLRUKCache<ICache>

//  1c73cc4d-ac35-41d9-a9e0-e46112c03a87
const BYTE c_rgbHashedLRUKCacheType[ sizeof( GUID ) ] = { 0x4D, 0xCC, 0x73, 0x1C, 0x35, 0xAC, 0xD9, 0x41, 0xA9, 0xE0, 0xE4, 0x61, 0x12, 0xC0, 0x3A, 0x87 };

class CHashedLRUKCache : public THashedLRUKCache<ICache>
{
    public:  //  specialized API

        static const BYTE* RgbCacheType()
        {
            return c_rgbHashedLRUKCacheType;
        }

        static ICache* PcCreate(    _In_    IFileSystemFilter* const            pfsf,
                                    _In_    IFileIdentification* const          pfident,
                                    _In_    IFileSystemConfiguration* const     pfsconfig, 
                                    _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                                    _Inout_ ICacheConfiguration** const         ppcconfig,
                                    _In_    ICacheTelemetry* const              pctm,
                                    _Inout_ IFileFilter** const                 ppffCaching,
                                    _Inout_ CCacheHeader** const                ppch )
        {
            return new CHashedLRUKCache( pfsf, pfident, pfsconfig, ppbcconfig, ppcconfig, pctm, ppffCaching, ppch );
        }

    private:

        CHashedLRUKCache(   _In_    IFileSystemFilter* const            pfsf,
                            _In_    IFileIdentification* const          pfident,
                            _In_    IFileSystemConfiguration* const     pfsconfig,
                            _Inout_ IBlockCacheConfiguration** const    ppbcconfig,
                            _Inout_ ICacheConfiguration** const         ppcconfig,
                            _In_    ICacheTelemetry* const              pctm,
                            _Inout_ IFileFilter** const                 ppffCaching,
                            _Inout_ CCacheHeader** const                ppch )
            :   THashedLRUKCache<ICache>( pfsf, pfident, pfsconfig, ppbcconfig, ppcconfig, pctm, ppffCaching, ppch )
        {
        }
};
