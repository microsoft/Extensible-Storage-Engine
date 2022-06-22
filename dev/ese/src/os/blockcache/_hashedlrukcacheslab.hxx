// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TCachedBlockSlab:  implementation of ICachedBlockSlab and its derivatives.
//
//  I:  ICachedBlockSlab or derivative
//
//  This class encapsulates the state and caching policy of the Hashed LRUK Cache (CHashedLRUKCache).  Each slab may be
//  comprised of multiple atomically modifiable chunks of state (CCachedBlockChunk).  Each chunk is the size of one
//  block and contains N slots, each of which contain the complete persistent description of a cached block
//  (CCachedBlock).  Each slot also contains computed and volatile state.  A CCachedBlockSlot is a CCachedBlock which
//  includes its addressing information (chunk number and slot number).  A CCachedBlockSlotState is a CCachedBlockSlot
//  that also includes the volatile and/or computed state of the slab, chunk, and slot.
//
//  A slab is the fundamental unit of cache management.  All caching policy decisions are made local to a slab.  A
//  slab is accessed via the slab manager (CCachedBlockSlabManager) via a hashing scheme.  The overall performance of
//  the caching policy relies on the hash to divide the load so that the caching policy decisions made only with local
//  information are statistically equivalent to managing one monolithic cache.  
// 
//  This reliance on local decisions is a key tradeoff to achieve a simpler cache implementation and to reduce overhead
//  of the implementation.  We want to enable a complex caching policy without requiring us to build complicated
//  persistent data structures to manage global state of the cache such as an LRU index.
//
//  When any caching operation is performed, the target slab is acquired for exclusive access.  The appropriate slots
//  in the slab are then updated in memory.  This may happen in one or more calls and may impact multiple chunks.  If
//  the changes are accepted then the entire contents of each impacted chunk are journaled for recovery.  This prevents
//  recovery failures due to corruption by always overwriting entire chunks (i.e. no read/modify/write).  The slab
//  state write back to storage is coordinated with the journal to implement write ahead logging.  A slab will not be
//  written back until the journal entries that modified it are considered durable for write back by the journal.
// 
//  The chunks in a slab are protected from corruption by an ECC.  This ECC must be verified to access a chunk.  Each
//  chunk is also protected from lost writes by a persisted write counter which must be verified to match the last
//  known write counter for the chunk.  These write counters are managed by the CCachedBlockWriteCountsManager and are
//  maintained at run time.  Note that, because we always overwrite entire chunks during journal recovery, lost writes
//  for chunks are not relevant during recovery.  However, we must ensure that we replay any update that isn't
//  fully reflected in the state of the persistent write counts.  So this write back must be coordinated with journal
//  recovery and helps determine the replay pointer.
// 
//  Multiple slabs may be updated atomically via one journal entry.  This is used to reassign clusters to newly evicted
//  slots to allow new data to be written into the cache without damaging existing data.  These reassigned clusters are
//  conceptually part of the journal but are also the final location for new data.  This scheme has the benefit of
//  fully journaled writes but avoids the need to write the cached data twice.  This scheme also prevents the
//  appearance of corruption if the journal entry associated with that data doesn't survive recovery (is past the tail
//  of the journal).
//
//  Each slab must support holding two copies of the same block of a cached file concurrently.  This is required to
//  support safe overwrite of a dirty block in the cache without first requiring an expensive write back operation.
//  These different versions are indicated by an update number.  The integrity of the block is protected by the
//  eviction scheme.  Only an older version of a block that has not been written back to storage may be evicted.  This
//  is indicated by the persisted property FDirty and the computed property FSuperceded.  The eviction process requires
//  the slot containing the older version of the block to be assigned to a new cluster.  This reassignment preserves
//  the state of the block in case its newer version and the overwrite of its older image are lost in a crash.
//
//  The slab implements a variant of the LRU-K policy (K=2) based on two touch counters.  We do not attempt to handle
//  correlated touches because we presume anything accessed via this cache is already too cold to hold in memory and
//  thus correlated touches cannot be meaningful. The replacement order is:  !FValid, FSuperceded, touched once by
//  touch number ascending, and then touched twice by previous touch number ascending.  
// 
//  In addition, a consideration is made by the cache implementation (CHashedLRUKCache) for the percentage of the cache
//  reserved for read only access vs blocks that have been written since cached.  If the request is to cache data for a
//  read then we will only consider slots that have never been written (!FEverDirty) plus the coldest slots that have
//  ever been written up to the percentage of the cache reserved for writes.  An analogous consideration is made for
//  writes.
//
//  The slab is actually used in two ways:  the primary cache state and then also the avail pool.  The avail pool is
//  used to manage clusters for loading new data into the cache as a virtual part of the journal.  For the avail pool,
//  we really only use the FValid and FDirty flags of each slot.  They are used to track the reusability state of the
//  clusters in the slabs.

template< class I >
class TCachedBlockSlab  //  cbs
    :   public I
{
    public:  //  specialized API

        ~TCachedBlockSlab();

        void SetIgnoreVerificationErrors( _In_ const BOOL fIgnoreVerificationErrors )
        {
            m_fIgnoreVerificationErrors = fIgnoreVerificationErrors;
        }

        ERR ErrStatus() const { return m_errSlab; }

    public:  //  ICachedBlockSlab

        ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) override;

        ERR ErrGetSlotForCache( _In_                const CCachedBlockId&   cbid,
                                _In_                const size_t            cb,
                                _In_reads_( cb )    const BYTE* const       rgb,
                                _Out_               CCachedBlockSlot* const pslot ) override;

        ERR ErrGetSlotForWrite( _In_                    const CCachedBlockId&   cbid,
                                _In_                    const size_t            cb,
                                _In_reads_opt_( cb )    const BYTE* const       rgb,
                                _Out_                   CCachedBlockSlot* const pslot ) override;

        ERR ErrGetSlotForRead(  _In_    const CCachedBlockId&   cbid,
                                _Out_   CCachedBlockSlot* const pslot ) override;

        ERR ErrWriteCluster(    _In_                const CCachedBlockSlot&                     slot,
                                _In_                const size_t                                cb,
                                _In_reads_( cb )    const BYTE* const                           rgb,
                                _In_opt_            const ICachedBlockSlab::PfnClusterWritten   pfnClusterWritten,
                                _In_opt_            const DWORD_PTR                             keyClusterWritten,
                                _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff ) override;

        ERR ErrUpdateSlot( _In_ const CCachedBlockSlot& slotNew ) override;
        
        ERR ErrReadCluster( _In_                const CCachedBlockSlot&                     slot,
                            _In_                const size_t                                cb,
                            _Out_writes_( cb )  BYTE* const                                 rgb,
                            _In_opt_            const ICachedBlockSlab::PfnClusterRead      pfnClusterRead,
                            _In_opt_            const DWORD_PTR                             keyClusterRead,
                            _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff ) override;
        
        ERR ErrVerifyCluster(   _In_                const CCachedBlockSlot& slot,
                                _In_                const size_t            cb,
                                _In_reads_( cb )    const BYTE* const       rgb ) override;

        ERR ErrInvalidateSlots( _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                _In_ const DWORD_PTR                            keyConsiderUpdate ) override;

        ERR ErrCleanSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                            _In_ const DWORD_PTR                            keyConsiderUpdate ) override;

        ERR ErrEvictSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                            _In_ const DWORD_PTR                            keyConsiderUpdate ) override;

        ERR ErrVisitSlots(  _In_ const ICachedBlockSlab::PfnVisitSlot   pfnVisitSlot,
                            _In_ const DWORD_PTR                        keyVisitSlot ) override;

        BOOL FUpdated() override;

        ERR ErrAcceptUpdates() override;

        void RevertUpdates() override;

        BOOL FDirty() override;

        ERR ErrSave(    _In_opt_    const ICachedBlockSlab::PfnSlabSaved    pfnSlabSaved,
                        _In_opt_    const DWORD_PTR                         keySlabSaved ) override;

    protected:

        TCachedBlockSlab(   _In_    IFileFilter* const                      pff,
                            _In_    const QWORD                             ibSlab,
                            _In_    const QWORD                             cbSlab,
                            _In_    const size_t                            ccbc,
                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                            _In_    const QWORD                             icbwcBase,
                            _In_    const ClusterNumber                     clnoMin,
                            _In_    const ClusterNumber                     clnoMax );

        ERR ErrInit();

        ERR ErrInitSupercededState();
        ERR ErrComputeSupercededState();

    private:

        class CUpdate
        {
            public:

                CUpdate(    _In_ const CCachedBlockSlot&    slotBefore,
                            _In_ const BOOL                 fSupercededBefore,
                            _In_ const CCachedBlockSlot&    slotAfter,
                            _In_ const BOOL                 fSupercededAfter,
                            _In_ const ChunkNumber          chnoSuperceded,
                            _In_ const SlotNumber           slnoSuperceded )
                    :   m_slotBefore( slotBefore ),
                        m_fSupercededBefore( fSupercededBefore ),
                        m_slotAfter( slotAfter ),
                        m_fSupercededAfter( fSupercededAfter ),
                        m_chnoSuperceded( chnoSuperceded ),
                        m_slnoSuperceded( slnoSuperceded ),
                        m_fClusterUpdateExpected( FClusterUpdateExpected( slotBefore, slotAfter ) ),
                        m_fClusterUpdated( fFalse )
                {
                }

                void SetClusterUpdated()
                {
                    m_fClusterUpdated = fTrue;
                }

                const CCachedBlockSlot& SlotBefore() const { return m_slotBefore; }
                const BOOL FSupercededBefore() const { return m_fSupercededBefore; }
                const CCachedBlockSlot& SlotAfter() const { return m_slotAfter; }
                const BOOL FSupercededAfter() const { return m_fSupercededAfter; }
                const ChunkNumber ChnoSuperceded() const { return m_chnoSuperceded; }
                const SlotNumber SlnoSuperceded() const { return m_slnoSuperceded; }
                BOOL FClusterUpdateExpected() const { return m_fClusterUpdateExpected && !m_fClusterUpdated; }
                BOOL FClusterUpdated() const { return m_fClusterUpdated; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CUpdate, m_ile ); }

            private:

            static BOOL FClusterUpdateExpected( _In_ const CCachedBlockSlot&    slotBefore,
                                                _In_ const CCachedBlockSlot&    slotNew )
            {
                return !slotBefore.FValid() &&
                        slotNew.FValid() &&
                        !(  slotNew.Cbid().Volumeid() == volumeidInvalid &&
                            slotNew.Cbid().Fileid() == fileidInvalid &&
                            slotNew.Cbid().Fileserial() == fileserialInvalid &&
                            slotNew.Cbid().Cbno() == cbnoInvalid );
            }

            private:

                typename CInvasiveList<CUpdate, OffsetOfILE>::CElement  m_ile;
                const CCachedBlockSlot                                  m_slotBefore;
                const BOOL                                              m_fSupercededBefore;
                const CCachedBlockSlot                                  m_slotAfter;
                const BOOL                                              m_fSupercededAfter;
                const ChunkNumber                                       m_chnoSuperceded;
                const SlotNumber                                        m_slnoSuperceded;
                const BOOL                                              m_fClusterUpdateExpected;
                BOOL                                                    m_fClusterUpdated;
        };

        void GetSlotState(  _In_    const size_t                    icbc,
                            _In_    const size_t                    icbl,
                            _Out_   CCachedBlockSlotState* const    pslotst )
        {
            CUpdate* pupdateFirst;
            CUpdate* pupdateLast;
            return GetSlotState( icbc, icbl, pslotst, &pupdateFirst, &pupdateLast );
        }

        void GetSlotState(  _In_    const size_t                    icbc,
                            _In_    const size_t                    icbl,
                            _Out_   CCachedBlockSlotState* const    pslotst,
                            _Out_   CUpdate** const                 ppupdateFirst,
                            _Out_   CUpdate** const                 ppupdateLast );

        ERR ErrGetSlotForNewImage(  _In_                    const CCachedBlockId&   cbid,
                                    _In_                    const size_t            cb,
                                    _In_reads_opt_( cb )    const BYTE* const       rgb,
                                    _In_                    const BOOL              fWrite,
                                    _Out_                   CCachedBlockSlot* const pslot );

        ERR ErrGetSlotState(    _In_    const CCachedBlockId&           cbid,
                                _Out_   CCachedBlockSlotState* const    pslotst );
        TouchNumber TonoGetNextTouch();

        ERR ErrVerifySlotAddress( _In_ const CCachedBlockSlot& slot );
        ERR ErrVerifySlot( _In_ const CCachedBlockSlot& slot );
        ERR ErrVerifySlotUpdate(    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                    _In_ const CCachedBlockSlot&        slotNew );
        ERR ErrVerifyCluster(   _In_                const CCachedBlockSlot&         slot,
                                _In_                const size_t                    cb,
                                _In_reads_( cb )    const BYTE* const               rgb,
                                _In_                const BOOL                      fRepair,
                                _Out_               CCachedBlockSlotState* const    pslotst,
                                _Out_               CUpdate** const                 ppupdateLast );
        ERR ErrRepairCluster(   _In_                const CCachedBlockSlot&         slot,
                                _In_                const size_t                    cb,
                                _In_reads_( cb )    const BYTE* const               rgb,
                                _In_                const DWORD                     dwECCExpected,
                                _In_                const DWORD                     dwECCActual );

        void RevertUnacceptedUpdates( _In_ const BOOL fPermanently );
        void RevertUnacceptedUpdate( _In_ CUpdate* const pupdate, _In_ const BOOL fPermanently );
        void RestoreUnacceptedUpdates();
        void InvalidateCachedRead();

        ERR ErrResetChunk( _In_ const size_t icbc );

    private:

        size_t Icbc( _In_ const ChunkNumber chno ) const { return (size_t)chno; }
        size_t Icbl( _In_ const SlotNumber slno ) const { return (size_t)slno; }

        ChunkNumber Chno( _In_ const size_t icbc ) const { return (ChunkNumber)icbc; }
        SlotNumber Slno( _In_ const size_t icbl ) const { return (SlotNumber)icbl; }

        CCachedBlockChunk* Pcbc( _In_ const size_t icbc ) const
        {
            return icbc >= m_ccbc ? NULL : &m_rgcbc[ icbc ];
        }

        CCachedBlockChunk* Pcbc( _In_ const ChunkNumber chno ) const
        {
            return Pcbc( Icbc( chno ) );
        }

    private:

        void SaveStart( _In_ const ICachedBlockSlab::PfnSlabSaved   pfnSlabSaved,
                        _In_ const DWORD_PTR                        keySlabSaved )
        {
            m_pfnSlabSaved = pfnSlabSaved;
            m_keySlabSaved = keySlabSaved;
            m_errSaved = JET_errSuccess;
            m_cSaveStepPending = 1;
        }

        void SaveComplete()
        {
            //  restore any unaccepted changes made to our chunk image

            RestoreUnacceptedUpdates();

            //  reset the dirty chunk / slab status if the save was successful

            if ( m_errSaved >= JET_errSuccess )
            {
                for ( size_t icbc = 0; icbc < m_ccbc && m_rgfDirty; icbc++ )
                {
                    m_rgfDirty[ icbc ] = fFalse;
                }

                m_fDirty = fFalse;
            }

            //  trigger the save completion

            m_pfnSlabSaved( m_errSaved, m_keySlabSaved );
        }

        void SaveStepComplete( _In_ const ERR err )
        {
            AtomicCompareExchange( (LONG*)&m_errSaved, JET_errSuccess, err < JET_errSuccess ? err : JET_errSuccess );
            if ( AtomicDecrement( (LONG*)&m_cSaveStepPending ) == 0 )
            {
                SaveComplete();
            }
        }

        void SaveRequestComplete( _In_ const ERR err )
        {
            SaveStepComplete( err );
        }

        void ChunkWriteComplete( _In_ const ERR errWrite, _In_ const QWORD ibOffset )
        {
            ERR err = errWrite;

            //  update the write count if the write request succeeded

            if ( err >= JET_errSuccess )
            {
                const size_t                icbc    = (size_t)( ( ibOffset - m_ibSlab ) / sizeof( CCachedBlockChunk ) );
                CCachedBlockChunk* const    pcbc    = Pcbc( icbc );

                Call( m_pcbwcm->ErrSetWriteCount( m_icbwcBase + icbc, pcbc->Cbwc() ) );
            }

        HandleError:
            SaveStepComplete( err );
        }

        void ChunkWriteHandoff()
        {
            AtomicIncrement( (LONG*)&m_cSaveStepPending );
        }

        static void ChunkWriteComplete_(    _In_                    const ERR               err,
                                            _In_                    IFileAPI* const         pfapi,
                                            _In_                    const FullTraceContext& tc,
                                            _In_                    const OSFILEQOS         grbitQOS,
                                            _In_                    const QWORD             ibOffset,
                                            _In_                    const DWORD             cbData,
                                            _In_reads_( cbData )    const BYTE* const       pbData,
                                            _In_                    const DWORD_PTR         keyIOComplete )
        {
            IFileAPI::PfnIOComplete pfnIOComplete = ChunkWriteComplete_;
            Unused( pfnIOComplete );

            TCachedBlockSlab<I>* const pcbs = (TCachedBlockSlab<I>*)keyIOComplete;
            pcbs->ChunkWriteComplete( err, ibOffset );
        }

        static void ChunkWriteHandoff_( _In_                    const ERR               err,
                                        _In_                    IFileAPI* const         pfapi,
                                        _In_                    const FullTraceContext& tc,
                                        _In_                    const OSFILEQOS         grbitQOS,
                                        _In_                    const QWORD             ibOffset,
                                        _In_                    const DWORD             cbData,
                                        _In_reads_( cbData )    const BYTE* const       pbData,
                                        _In_                    const DWORD_PTR         keyIOComplete,
                                        _In_                    void* const             pvIOContext )
        {
            IFileAPI::PfnIOHandoff pfnIOHandoff = ChunkWriteHandoff_;
            Unused( pfnIOHandoff );

            TCachedBlockSlab<I>* const pcbs = (TCachedBlockSlab<I>*)keyIOComplete;
            pcbs->ChunkWriteHandoff();
        }

        class CSaveComplete
        {
            public:

                CSaveComplete()
                    :   m_sigComplete( CSyncBasicInfo( "TCachedBlockSlab<I>::CSaveComplete::m_sigComplete" ) ),
                        m_errComplete( JET_errSuccess )
                {
                }

                ERR ErrComplete()
                {
                    m_sigComplete.Wait();
                    return m_errComplete;
                }

                static void Saved_( _In_ const ERR          err,
                                    _In_ const DWORD_PTR    keySaved )
                {
                    ICachedBlockSlab::PfnSlabSaved pfnSlabSaved = Saved_;
                    Unused( pfnSlabSaved );

                    CSaveComplete* const psavecomplete = (CSaveComplete*)keySaved;
                    psavecomplete->m_errComplete = err;
                    psavecomplete->m_sigComplete.Set();
                }

            private:

                CSaveComplete( _In_ const CSaveComplete& other ) = delete;

            private:

                CManualResetSignal  m_sigComplete;
                ERR                 m_errComplete;
        };

    private:

        //  Request context.

        class CRequest
        {
            public:

                void Release( _In_ const ERR err )
                {
                    Finish( err );
                }

                virtual ERR ErrExecute() = 0;

            protected:

                CRequest()
                    :   m_cref( 1 ),
                        m_err( JET_errSuccess )
                {
                }

                virtual ~CRequest() {}

                virtual void Handoff() {}

                virtual void Complete( _In_ const ERR err ) {}

            private:

                CRequest( const CRequest& other ) = delete;

                void Start()
                {
                    AtomicIncrement( (LONG*)&m_cref );
                    Handoff();
                }
                
                void Finish( _In_ const ERR err )
                {
                    AtomicCompareExchange( (LONG*)&m_err, JET_errSuccess, err < JET_errSuccess ? err : JET_errSuccess );

                    if ( AtomicDecrement( (LONG*)&m_cref ) == 0 )
                    {
                        Complete( m_err );
                        Release();
                    }
                }

                void Release()
                {
                    delete this;
                }

            protected:

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

                volatile int    m_cref;
                volatile ERR    m_err;
        };

        class CReadRequest : public CRequest
        {
            public:

                CReadRequest(   _In_                    IFileFilter* const                          pff,
                                _In_                    const QWORD                                 ibOffset,
                                _In_                    const size_t                                cbData,
                                _Out_writes_( cbData )  BYTE* const                                 pbData,
                                _In_opt_                const ICachedBlockSlab::PfnClusterRead      pfnClusterRead,
                                _In_opt_                const DWORD_PTR                             keyClusterRead,
                                _In_opt_                const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff )
                    :   m_pff( pff ),
                        m_ibOffset( ibOffset ),
                        m_cbData( cbData ),
                        m_pbData( pbData ),
                        m_pfnClusterRead( pfnClusterRead ),
                        m_keyClusterRead( keyClusterRead ),
                        m_pfnClusterHandoff( pfnClusterHandoff )
                {
                }

                ERR ErrExecute() override
                {
                    return m_pff->ErrIORead(    TcCurr(),
                                                m_ibOffset,
                                                m_cbData,
                                                m_pbData,
                                                qosIONormal,
                                                m_pfnClusterRead ? IOComplete_ : NULL,
                                                DWORD_PTR( this ),
                                                m_pfnClusterRead || m_pfnClusterHandoff ? IOHandoff_ : NULL,
                                                NULL );
                }

            protected:

                void Handoff() override
                {
                    if ( m_pfnClusterHandoff )
                    {
                        m_pfnClusterHandoff( m_keyClusterRead );
                    }

                    if ( !m_pfnClusterRead )
                    {
                        Release( JET_errSuccess );
                    }
                }

                void Complete( _In_ const ERR err ) override
                {
                    if ( m_pfnClusterRead )
                    {
                        m_pfnClusterRead( err, m_keyClusterRead );
                    }
                }

            private:

                IFileFilter* const                          m_pff;
                const QWORD                                 m_ibOffset;
                const size_t                                m_cbData;
                BYTE* const                                 m_pbData;
                const ICachedBlockSlab::PfnClusterRead      m_pfnClusterRead;
                const DWORD_PTR                             m_keyClusterRead;
                const ICachedBlockSlab::PfnClusterHandoff   m_pfnClusterHandoff;
        };

        class CWriteRequest : public CRequest
        {
            public:

                CWriteRequest(  _In_                    IFileFilter* const                          pff,
                                _In_                    const QWORD                                 ibOffset,
                                _In_                    const size_t                                cbData,
                                _In_reads_( cbData )    const BYTE* const                           pbData,
                                _In_opt_                const ICachedBlockSlab::PfnClusterWritten   pfnClusterWritten,
                                _In_opt_                const DWORD_PTR                             keyClusterWritten,
                                _In_opt_                const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff )
                    :   m_pff( pff ),
                        m_ibOffset( ibOffset ),
                        m_cbData( cbData ),
                        m_pbData( pbData ),
                        m_pfnClusterWritten( pfnClusterWritten ),
                        m_keyClusterWritten( keyClusterWritten ),
                        m_pfnClusterHandoff( pfnClusterHandoff )
                {
                }

                ERR ErrExecute() override
                {
                    return m_pff->ErrIOWrite(   TcCurr(),
                                                m_ibOffset,
                                                m_cbData,
                                                m_pbData,
                                                qosIONormal,
                                                m_pfnClusterWritten ? IOComplete_ : NULL,
                                                DWORD_PTR( this ),
                                                m_pfnClusterWritten || m_pfnClusterHandoff ? IOHandoff_ : NULL );
                }

            protected:

                void Handoff() override
                {
                    if ( m_pfnClusterHandoff )
                    {
                        m_pfnClusterHandoff( m_keyClusterWritten );
                    }

                    if ( !m_pfnClusterWritten )
                    {
                        Release( JET_errSuccess );
                    }
                }

                void Complete( _In_ const ERR err ) override
                {
                    //  fire the completion if provided

                    if ( m_pfnClusterWritten )
                    {
                        m_pfnClusterWritten( err, m_keyClusterWritten );
                    }
                }

            private:

                IFileFilter* const                          m_pff;
                const QWORD                                 m_ibOffset;
                const size_t                                m_cbData;
                const BYTE* const                           m_pbData;
                const ICachedBlockSlab::PfnClusterWritten   m_pfnClusterWritten;
                const DWORD_PTR                             m_keyClusterWritten;
                const ICachedBlockSlab::PfnClusterHandoff   m_pfnClusterHandoff;
        };

    private:

        enum class ISlot : size_t
        {
        };

        const ISlot Islot( _In_ const size_t icbc, _In_ const size_t icbl ) const { return (ISlot)( icbc * CCachedBlockChunk::Ccbl() + icbl ); }

        const size_t Icbc( _In_ const ISlot islot ) const { return (size_t)islot / CCachedBlockChunk::Ccbl(); }
        const size_t Icbl( _In_ const ISlot islot ) const { return (size_t)islot % CCachedBlockChunk::Ccbl(); }

        typedef int( __cdecl* PfnCompare )( _In_ void* pvContext, _In_ const void* pvA, _In_ const void* pvB );


        ERR ErrCreateSortedIslotArray(  _In_                                const PfnCompare    pfnCompare,
                                        _Out_                               size_t* const       pcislot,
                                        _Outptr_result_buffer_( *pcislot )  ISlot** const       prgislot );

        int CompareSlotsForInit( _In_ const ISlot islotA, _In_ const ISlot islotB );
        int CompareSlotsForEvict( _In_ const ISlot islotA, _In_ const ISlot islotB );

        static int __cdecl CompareSlotsForInit_( _In_ void* pvContext, _In_ const void* pvA, _In_ const void* pvB )
        {
            TCachedBlockSlab<I>* const pcbs = (TCachedBlockSlab<I>*)pvContext;

            return pcbs->CompareSlotsForInit( *((ISlot*)pvA), *((ISlot*)pvB) );
        }

        static int __cdecl CompareSlotsForEvict_( _In_ void* pvContext, _In_ const void* pvA, _In_ const void* pvB )
        {
            TCachedBlockSlab<I>* const pcbs = (TCachedBlockSlab<I>*)pvContext;

            return pcbs->CompareSlotsForEvict( *((ISlot*)pvA), *((ISlot*)pvB) );
        }

    private:

        ERR ErrBlockCacheInternalError( _In_ const char* const szTag )
        {
            return ::ErrBlockCacheInternalError( m_pff, szTag );
        }

    private:

        IFileFilter* const                              m_pff;
        const QWORD                                     m_ibSlab;
        const QWORD                                     m_cbSlab;
        const size_t                                    m_ccbc;
        ICachedBlockWriteCountsManager* const           m_pcbwcm;
        const QWORD                                     m_icbwcBase;
        const ClusterNumber                             m_clnoMin;
        const ClusterNumber                             m_clnoMax;

        CCachedBlockChunk*                              m_rgcbc;
        ERR*                                            m_rgerrChunk;
        ERR                                             m_errSlab;
        BOOL                                            m_fIgnoreVerificationErrors;
        TouchNumber                                     m_tonoLast;
        int*                                            m_rgcUpdate;
        int                                             m_cUpdate;
        BOOL*                                           m_rgfDirty;
        BOOL                                            m_fDirty;
        CInvasiveList<CUpdate, CUpdate::OffsetOfILE>    m_ilUpdate;
        BOOL*                                           m_rgfSlotSuperceded;

        ICachedBlockSlab::PfnSlabSaved                  m_pfnSlabSaved;
        DWORD_PTR                                       m_keySlabSaved;
        volatile ERR                                    m_errSaved;
        volatile int                                    m_cSaveStepPending;

        CCachedBlockId                                  m_cbidLastRead;
        ChunkNumber                                     m_chnoLastRead;
        SlotNumber                                      m_slnoLastRead;
};

template< class I  >
INLINE TCachedBlockSlab<I>::~TCachedBlockSlab()
{
    RevertUpdates();
    delete[] m_rgfSlotSuperceded;
    delete[] m_rgcUpdate;
    delete[] m_rgfDirty;
    delete[] m_rgerrChunk;
    OSMemoryPageFree( m_rgcbc );
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrGetPhysicalId( _Out_ QWORD* const pib )
{
    *pib = m_ibSlab;
    return JET_errSuccess;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrGetSlotForCache( _In_                const CCachedBlockId&   cbid,
                                                    _In_                const size_t            cb,
                                                    _In_reads_( cb )    const BYTE* const       rgb,
                                                    _Out_               CCachedBlockSlot* const pslot )
{
    return ErrGetSlotForNewImage( cbid, cb, rgb, fFalse, pslot );
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrGetSlotForWrite( _In_                    const CCachedBlockId&   cbid,
                                                    _In_                    const size_t            cb,
                                                    _In_reads_opt_( cb )    const BYTE* const       rgb,
                                                    _Out_                   CCachedBlockSlot* const pslot )
{
    return ErrGetSlotForNewImage( cbid, cb, rgb, fTrue, pslot );
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrGetSlotForRead(  _In_    const CCachedBlockId&   cbid,
                                                    _Out_   CCachedBlockSlot* const pslot )
{
    ERR                     err             = JET_errSuccess;
    CCachedBlockSlotState   slotstCurrent;
    CCachedBlockSlot        slotNew;
    TouchNumber             tonoNew         = tonoInvalid;

    new( pslot ) CCachedBlockSlot();

    //  do not allow operation on a corrupt slab

    Call( m_errSlab );

    //  get the current image of the slot

    Call( ErrGetSlotState( cbid, &slotstCurrent ) );

    //  we found the slot

    if ( slotstCurrent.FValid() )
    {
        //  generate a new image of the slot that has one more touch which the caller may choose to use

        tonoNew = TonoGetNextTouch();
        new( pslot ) CCachedBlockSlot(  slotstCurrent.IbSlab(),
                                        slotstCurrent.Chno(),
                                        slotstCurrent.Slno(),
                                        CCachedBlock(   slotstCurrent.Cbid(),
                                                        slotstCurrent.Clno(),
                                                        slotstCurrent.DwECC(),
                                                        tonoNew,
                                                        slotstCurrent.Tono0() == tonoInvalid ?
                                                            tonoNew :
                                                            slotstCurrent.Tono0(),
                                                        slotstCurrent.FValid(),
                                                        slotstCurrent.FPinned(),
                                                        slotstCurrent.FDirty(),
                                                        slotstCurrent.FEverDirty(),
                                                        slotstCurrent.FPurged(),
                                                        slotstCurrent.Updno() ) );

        //  verify that this change is allowed

        err = ErrVerifySlotUpdate( slotstCurrent, *pslot );
        CallS( err );
        Call( err );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        new( pslot ) CCachedBlockSlot();
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrWriteCluster(    _In_                const CCachedBlockSlot&                     slot,
                                                    _In_                const size_t                                cb,
                                                    _In_reads_( cb )    const BYTE* const                           rgb,
                                                    _In_opt_            const ICachedBlockSlab::PfnClusterWritten   pfnClusterWritten,
                                                    _In_opt_            const DWORD_PTR                             keyClusterWritten,
                                                    _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff )
{
    ERR                     err             = JET_errSuccess;
    CCachedBlockSlotState   slotstCurrent;
    CUpdate*                pupdateLast     = NULL;
    const BOOL              fAsync          = pfnClusterWritten != NULL;
    CWriteRequest*          prequest        = NULL;

    //  do not allow operation on a corrupt slab

    Call( m_errSlab );

    //  validate that the slot and data match the current state

    Call( ErrVerifyCluster( slot, cb, rgb, fFalse, &slotstCurrent, &pupdateLast ) );

    //  we only support writing a cluster associated with an update that has not yet been accepted

    if ( !pupdateLast )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  we only support writing a cluster once per update that is caching a new image

    if ( !pupdateLast->FClusterUpdateExpected() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  we only support writing a cluster whose slot state hasn't changed

    if ( memcmp( &slotstCurrent, &slot, sizeof( slot ) ) != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  create our request context

    Alloc( prequest = new CWriteRequest(    m_pff,
                                            (QWORD)slotstCurrent.Clno() * cbCachedBlock,
                                            cb,
                                            rgb,
                                            pfnClusterWritten, 
                                            keyClusterWritten,
                                            pfnClusterHandoff ) );

    //  write the cluster

    Call( prequest->ErrExecute() );

    //  note that we updated the cluster

    pupdateLast->SetClusterUpdated();

HandleError:
    if ( prequest )
    {
        prequest->Release( err );
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrUpdateSlot( _In_ const CCachedBlockSlot& slotNew )
{
    ERR                     err                 = JET_errSuccess;
    const size_t            icbc                = Icbc( slotNew.Chno() );
    const size_t            icbl                = Icbl( slotNew.Slno() );
    CCachedBlockSlotState   slotstCurrent;
    CUpdate*                pupdateFirst        = NULL;
    CUpdate*                pupdateLast         = NULL;
    BOOL                    fSupercededCurrent  = fFalse;
    BOOL                    fSupercededNew      = fFalse;
    CCachedBlockSlotState   slotstSuperceded;
    BOOL                    fSupercedesOther    = fFalse;
    CUpdate*                pupdate             = NULL;

    //  the slot address must be valid

    Call( ErrVerifySlotAddress( slotNew ) );

    //  if this chunk failed validation on load then reset it before the update
    //
    //  NOTE:  this will leave the slots in a state where if they are not all updated then ErrSave will fail

    if ( m_rgerrChunk[ icbc ] < JET_errSuccess )
    {
        Call( ErrResetChunk( icbc ) );
    }

    //  lookup the current slot state

    GetSlotState( icbc, icbl, &slotstCurrent, &pupdateFirst, &pupdateLast );

    //  if there is no actual change to the state then just succeed immediately

    if ( memcmp( &slotstCurrent, &slotNew, sizeof( slotNew ) ) == 0 )
    {
        Error( JET_errSuccess );
    }

    //  if this change undoes the last update then revert it and succeed

    if ( pupdateLast && memcmp( &pupdateLast->SlotBefore(), &slotNew, sizeof( slotNew ) ) == 0 )
    {
        RevertUnacceptedUpdate( pupdateLast, fTrue );
        Error( JET_errSuccess );
    }

    //  verify that this change is allowed

    if ( m_fIgnoreVerificationErrors )
    {
        Call( ErrVerifySlot( slotNew ) );
    }
    else
    {
        Call( ErrVerifySlotUpdate( slotstCurrent, slotNew ) );
    }

    //  determine this slot's superceded state

    fSupercededCurrent = m_rgfSlotSuperceded[ (size_t)Islot( icbc, icbl ) ];
    fSupercededNew = fFalse;

    //  if we are updating a block then check to see if we are superceding an older version of that block

    if (    slotNew.FValid() &&
            slotNew.Cbid().Cbno() != cbnoInvalid &&
            slotNew.Cbid().Fileserial() != fileserialInvalid &&
            slotNew.Cbid().Fileid() != fileidInvalid &&
            slotNew.Cbid().Volumeid() != volumeidInvalid )
    {
        //  find the youngest version of this cached block, if any

        Call( ErrGetSlotState( slotNew.Cbid(), &slotstSuperceded ) );

        //  if we did find the youngest existing version of this cached block then it had better be younger than the
        //  new image

        if (    slotstSuperceded.Chno() != chnoInvalid && slotstSuperceded.Slno() != slnoInvalid &&
                !( slotstSuperceded.Chno() == slotNew.Chno() && slotstSuperceded.Slno() == slotNew.Slno() ) )
        {
            //  the youngest existing version of this cached block had better be younger than the new image

            if (    slotstSuperceded.FValid() && 
                    ( slotNew.Updno() == updnoInvalid || slotstSuperceded.Updno() >= slotNew.Updno() ) )
            {
                //  however, this is allowed during recovery

                if ( !m_fIgnoreVerificationErrors )
                {
                    Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlabUpdnoSequence" ) );
                }
            }

            //  note that this slot supercedes the other

            fSupercedesOther = fTrue;
        }
    }

    //  ensure we have our updated and dirty state storage allocated

    if ( !m_rgcUpdate || !m_rgfDirty )
    {
        Alloc( m_rgcUpdate = m_rgcUpdate ? m_rgcUpdate : new int[ m_ccbc ] );
        Alloc( m_rgfDirty = m_rgfDirty ? m_rgfDirty : new BOOL[ m_ccbc ] );
        for ( size_t icbcT = 0; icbcT < m_ccbc; icbcT++ )
        {
            m_rgcUpdate[ icbcT ] = 0;
            m_rgfDirty[ icbcT ] = fFalse;
        }
    }

    //  remember this update

    Alloc( pupdate = new CUpdate(   slotstCurrent,
                                    fSupercededCurrent,
                                    slotNew,
                                    fSupercededNew,
                                    fSupercedesOther ? slotstSuperceded.Chno() : chnoInvalid,
                                    fSupercedesOther ? slotstSuperceded.Slno() : slnoInvalid ) );

    m_ilUpdate.InsertAsNextMost( pupdate );
    pupdate = NULL;

    //  update the cached block state

    UtilMemCpy( Pcbc( icbc )->Pcbl( icbl ), (CCachedBlock*)&slotNew, sizeof( CCachedBlock ) );

    //  remember that we have updated this chunk and the slab

    m_rgcUpdate[ icbc ]++;
    m_cUpdate++;

    //  update the superceded state

    m_rgfSlotSuperceded[ (size_t)Islot( icbc, icbl ) ] = fSupercededNew;

    if ( fSupercedesOther )
    {
        m_rgfSlotSuperceded[ (size_t)Islot( Icbc( slotstSuperceded.Chno() ), Icbl( slotstSuperceded.Slno() ) ) ] = fTrue;
    }

    //  invalidate any cached read

    InvalidateCachedRead();

    //  ensure that the last touch time is larger than the value we just used.  this handles the case where the slab
    //  fails validation on load, is fixed up by recovery, and then is updated before being written out

    m_tonoLast = ( m_tonoLast == tonoInvalid || m_tonoLast < slotNew.Tono0() ) ? slotNew.Tono0() : m_tonoLast;

HandleError:
    delete pupdate;
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrReadCluster( _In_                const CCachedBlockSlot&                     slot,
                                                _In_                const size_t                                cb,
                                                _Out_writes_( cb )  BYTE* const                                 rgb,
                                                _In_opt_            const ICachedBlockSlab::PfnClusterRead      pfnClusterRead,
                                                _In_opt_            const DWORD_PTR                             keyClusterRead,
                                                _In_opt_            const ICachedBlockSlab::PfnClusterHandoff   pfnClusterHandoff )
{
    ERR                     err             = JET_errSuccess;
    CCachedBlockSlotState   slotstCurrent;
    CUpdate*                pupdateFirst    = NULL;
    CUpdate*                pupdateLast     = NULL;
    const BOOL              fAsync          = pfnClusterRead != NULL;
    CReadRequest*           prequest        = NULL;

    //  the slot address must be valid

    Call( ErrVerifySlotAddress( slot ) );

    //  ensure we are given a valid data block

    if ( !rgb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cb != cbCachedBlock )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  lookup the current slot state

    GetSlotState( Icbc( slot.Chno() ), Icbl( slot.Slno() ), &slotstCurrent, &pupdateFirst , &pupdateLast );

    //  the slot must hold a valid current copy of a valid cached block

    if ( !slot.FValid() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if (    slot.Cbid().Volumeid() == volumeidInvalid ||
            slot.Cbid().Fileid() == fileidInvalid || 
            slot.Cbid().Fileserial() == fileserialInvalid || 
            slot.Cbid().Cbno() == cbnoInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( !slotstCurrent.FValid() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( slotstCurrent.FSuperceded() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if (    slot.Cbid().Volumeid() != slotstCurrent.Cbid().Volumeid() ||
            slot.Cbid().Fileid() != slotstCurrent.Cbid().Fileid() ||
            slot.Cbid().Fileserial() != slotstCurrent.Cbid().Fileserial() ||
            slot.Cbid().Cbno() != slotstCurrent.Cbid().Cbno() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    
    //  we will not allow reading a cluster that has not been written

    if ( pupdateLast && pupdateLast->FClusterUpdateExpected() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    //  create our request context

    Alloc( prequest = new CReadRequest( m_pff,
                                        (QWORD)slot.Clno() * cbCachedBlock,
                                        cb,
                                        rgb,
                                        pfnClusterRead, 
                                        keyClusterRead,
                                        pfnClusterHandoff ) );

    //  read the cluster

    Call( prequest->ErrExecute() );

HandleError:
    if ( prequest )
    {
        prequest->Release( err );
    }
    return err;
}
        
template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrVerifyCluster(   _In_                const CCachedBlockSlot& slot,
                                                    _In_                const size_t            cb,
                                                    _In_reads_( cb )    const BYTE* const       rgb )
{
    CCachedBlockSlotState   slotstCurrent;
    CUpdate*                pupdateLast;

    return ErrVerifyCluster( slot, cb, rgb, fTrue, &slotstCurrent, &pupdateLast );
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrInvalidateSlots( _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                                    _In_ const DWORD_PTR                            keyConsiderUpdate )
{
    ERR err = JET_errSuccess;

    //  do not allow operation on a corrupt slab

    Call( m_errSlab );

    //  visit all slots

    for ( size_t icbc = 0; icbc < m_ccbc; icbc++ )
    {
        for ( size_t icbl = 0; icbl < CCachedBlockChunk::Ccbl(); icbl++ )
        {
            //  get the current slot state

            CCachedBlockSlotState slotstCurrent;
            GetSlotState( icbc, icbl, &slotstCurrent );

            //  compute the legal state change for this slot

            const CCachedBlockSlot slotNew( slotstCurrent.IbSlab(),
                                            slotstCurrent.Chno(),
                                            slotstCurrent.Slno(),
                                            CCachedBlock(   slotstCurrent.Cbid(),
                                                            slotstCurrent.Clno(),
                                                            slotstCurrent.DwECC(),
                                                            tonoInvalid,
                                                            tonoInvalid,
                                                            fFalse,
                                                            fFalse,
                                                            fFalse,
                                                            slotstCurrent.FEverDirty(),
                                                            slotstCurrent.FDirty() && !slotstCurrent.FSuperceded(),
                                                            slotstCurrent.Updno() ) );

            //  verify that this change is allowed

            err = ErrVerifySlotUpdate( slotstCurrent, slotNew );
            CallS( err );
            Call( err );

            //  visit the slot and provide the allowed state change

            if ( !pfnConsiderUpdate( slotstCurrent, slotNew, keyConsiderUpdate ) )
            {
                Error( JET_errSuccess );
            }
        }
    }

HandleError:
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrCleanSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                                _In_ const DWORD_PTR                            keyConsiderUpdate )
{
    ERR err = JET_errSuccess;

    //  do not allow operation on a corrupt slab

    Call( m_errSlab );

    //  visit all slots

    for ( size_t icbc = 0; icbc < m_ccbc; icbc++ )
    {
        for ( size_t icbl = 0; icbl < CCachedBlockChunk::Ccbl(); icbl++ )
        {
            //  get the current slot state

            CCachedBlockSlotState slotstCurrent;
            GetSlotState( icbc, icbl, &slotstCurrent );

            //  determine if this slot may be cleaned

            const BOOL fMayClean =  slotstCurrent.FValid() &&
                                    !slotstCurrent.FPinned() &&
                                    slotstCurrent.FDirty() &&
                                    !slotstCurrent.FSuperceded();

            //  compute the legal state change for this slot

            CCachedBlockSlot        slot;
            const CCachedBlockSlot& slotNew = fMayClean ? slot : slotstCurrent;

            if ( fMayClean )
            {
                new ( &slot ) CCachedBlockSlot( slotstCurrent.IbSlab(), 
                                                slotstCurrent.Chno(),
                                                slotstCurrent.Slno(),
                                                CCachedBlock(   slotstCurrent.Cbid(),
                                                                slotstCurrent.Clno(),
                                                                slotstCurrent.DwECC(),
                                                                slotstCurrent.Tono0(),
                                                                slotstCurrent.Tono1(),
                                                                slotstCurrent.FValid(),
                                                                slotstCurrent.FPinned(),
                                                                fFalse,
                                                                slotstCurrent.FEverDirty(),
                                                                slotstCurrent.FPurged(),
                                                                slotstCurrent.Updno() ) );
            }

            //  verify that this change is allowed

            err = ErrVerifySlotUpdate( slotstCurrent, slotNew );
            CallS( err );
            Call( err );

            //  visit the slot and provide the allowed state change

            if ( !pfnConsiderUpdate( slotstCurrent, slotNew, keyConsiderUpdate ) )
            {
                Error( JET_errSuccess );
            }
        }
    }

HandleError:
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrEvictSlots(  _In_ const ICachedBlockSlab::PfnConsiderUpdate  pfnConsiderUpdate,
                                                _In_ const DWORD_PTR                            keyConsiderUpdate )
{
    ERR     err     = JET_errSuccess;
    size_t  cislot  = 0;
    ISlot*  rgislot = NULL;

    //  do not allow operation on a corrupt slab

    Call( m_errSlab );

    //  sort all slots by increasing utility

    Call( ErrCreateSortedIslotArray( &CompareSlotsForEvict_, &cislot, &rgislot ) );

    //  visit all slots by increasing utility

    for ( size_t iislot = 0; iislot < cislot; iislot++ )
    {
        const ISlot islot = rgislot[ iislot ];

        //  get the current slot state

        CCachedBlockSlotState slotstCurrent;
        GetSlotState( Icbc( islot ), Icbl( islot ), &slotstCurrent );

        //  determine if this slot may be evicted

        const BOOL fMayEvict =  slotstCurrent.FValid() &&
                                ( !slotstCurrent.FPinned() || slotstCurrent.FSuperceded() ) &&
                                ( !slotstCurrent.FDirty() || slotstCurrent.FSuperceded() );

        //  compute the legal state change for this slot

        CCachedBlockSlot        slot;
        const CCachedBlockSlot& slotNew = fMayEvict ? slot : slotstCurrent;

        if ( fMayEvict )
        {
            new ( &slot ) CCachedBlockSlot( slotstCurrent.IbSlab(), 
                                            slotstCurrent.Chno(),
                                            slotstCurrent.Slno(),
                                            CCachedBlock(   slotstCurrent.Cbid(),
                                                            slotstCurrent.Clno(),
                                                            slotstCurrent.DwECC(),
                                                            tonoInvalid,
                                                            tonoInvalid,
                                                            fFalse,
                                                            fFalse,
                                                            fFalse,
                                                            slotstCurrent.FEverDirty(),
                                                            fFalse,
                                                            slotstCurrent.Updno() ) );
        }

        //  verify that this change is allowed

        err = ErrVerifySlotUpdate( slotstCurrent, slotNew );
        CallS( err );
        Call( err );

        //  visit the slot and provide the allowed state change

        if ( !pfnConsiderUpdate( slotstCurrent, slotNew, keyConsiderUpdate ) )
        {
            Error( JET_errSuccess );
        }
    }

HandleError:
    delete[] rgislot;
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrVisitSlots(  _In_ const ICachedBlockSlab::PfnVisitSlot   pfnVisitSlot,
                                                _In_ const DWORD_PTR                        keyVisitSlot )
{
    ERR err = JET_errSuccess;

    //  visit all slots

    for ( size_t icbc = 0; icbc < m_ccbc; icbc++ )
    {
        for ( size_t icbl = 0; icbl < CCachedBlockChunk::Ccbl(); icbl++ )
        {
            //  get the current slot state

            CCachedBlockSlotState slotst;
            CUpdate* pupdateFirst;
            CUpdate* pupdateLast;
            GetSlotState( icbc, icbl, &slotst, &pupdateFirst, &pupdateLast );

            //  visit the slot

            if ( !pfnVisitSlot( m_rgerrChunk[ icbc ],
                                pupdateFirst ?
                                    CCachedBlockSlotState(  pupdateFirst->SlotBefore(),
                                                            fFalse,
                                                            fFalse,
                                                            fFalse,
                                                            fFalse,
                                                            pupdateFirst->FSupercededBefore() ) :
                                    slotst,
                                slotst,
                                keyVisitSlot ) )
            {
                Error( JET_errSuccess );
            }
        }
    }

HandleError:
    return err;
}

template< class I  >
INLINE BOOL TCachedBlockSlab<I>::FUpdated()
{
    return m_cUpdate > 0;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrAcceptUpdates()
{
    ERR err = JET_errSuccess;

    //  if the data associated with new cached block images was not written then fail

    for ( CUpdate* pupdate = m_ilUpdate.PrevMost(); pupdate; pupdate = m_ilUpdate.Next( pupdate ) )
    {
        if ( pupdate->FClusterUpdateExpected() )
        {
            //  however, this is allowed during recovery

            if ( !m_fIgnoreVerificationErrors )
            {
                Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlabClusterUpdateExpected" ) );
            }
        }
    }

    //  if we are updating the slab as a part of recovery then recompute our superceded state
    //
    //  NOTE:  we can't know the superceded state if any chunk in the slab fails validation, which is something that
    //  recovery fixes.  also note that during recovery we always redo any set of chunk updates we see and that this
    //  can transiently cause the entire slab's image to be inconsistent, so we must recompute after these as well

    if ( m_fIgnoreVerificationErrors )
    {
        Call( ErrComputeSupercededState() );
    }

    //  we no longer need the previously accepted state for the updated slots

    while ( CUpdate* const pupdate = m_ilUpdate.PrevMost() )
    {
        m_ilUpdate.Remove( pupdate );
        delete pupdate;
    }

    //  migrate our updated chunk / slab status to our dirty chunk / slab status

    for ( size_t icbc = 0; icbc < m_ccbc && m_rgfDirty; icbc++ )
    {
        m_rgfDirty[ icbc ] = m_rgfDirty[ icbc ] || m_rgcUpdate[ icbc ] > 0;
        m_rgcUpdate[ icbc ] = 0;
    }

    m_fDirty = m_fDirty || m_cUpdate > 0;
    m_cUpdate = 0;

HandleError:
    return err;
}

template< class I  >
INLINE void TCachedBlockSlab<I>::RevertUpdates()
{
    //  permanently revert any unaccepted updates

    RevertUnacceptedUpdates( fTrue );

    //  reset our updated chunk / slab status

    for ( size_t icbc = 0; icbc < m_ccbc && m_rgcUpdate; icbc++ )
    {
        m_rgcUpdate[ icbc ] = 0;
    }

    m_cUpdate = 0;
}

template< class I  >
INLINE BOOL TCachedBlockSlab<I>::FDirty()
{
    return m_fDirty;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrSave(    _In_opt_    const ICachedBlockSlab::PfnSlabSaved    pfnSlabSaved,
                                            _In_opt_    const DWORD_PTR                         keySlabSaved )
{
    ERR             err             = JET_errSuccess;
    CSaveComplete   savecomplete;
    int             cWrite          = 0;

    //  temporarily revert any unaccepted updates

    RevertUnacceptedUpdates( fFalse );

    //  setup our completion context

    SaveStart(  pfnSlabSaved ? pfnSlabSaved : CSaveComplete::Saved_,
                pfnSlabSaved ? keySlabSaved : (DWORD_PTR)&savecomplete );

    //  start writing all the updated chunks in the slab

    for ( size_t icbc = 0; icbc < m_ccbc; icbc++ )
    {
        if ( m_rgfDirty && m_rgfDirty[ icbc ] )
        {
            CCachedBlockChunk* const    pcbc    = Pcbc( icbc );
            TraceContextScope           tcScope( iorpBlockCache );

            //  if this chunk was corrupt on load and hasn't been patched then do not save

            if ( m_rgerrChunk[ icbc ] < JET_errSuccess )
            {
                Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlabUnpatchedAtSave" ) );
            }

            //  finalize the chunk

            Call( pcbc->ErrFinalize() );

            //  verify all slots in the chunk

            for ( size_t icbl = 0; icbl < CCachedBlockChunk::Ccbl(); icbl++ )
            {
                Call( ErrVerifySlot( CCachedBlockSlot( m_ibSlab, Chno( icbc ), Slno( icbl ), *pcbc->Pcbl( icbl ) ) ) );
            }

            //  write the chunk

            Call( m_pff->ErrIOWrite(    *tcScope,
                                        m_ibSlab + icbc * sizeof( CCachedBlockChunk ),
                                        sizeof( CCachedBlockChunk ),
                                        (const BYTE*)pcbc,
                                        qosIONormal,
                                        ChunkWriteComplete_,
                                        (DWORD_PTR)this,
                                        ChunkWriteHandoff_ ) );
            cWrite++;
        }
    }

    //  if this was a sync request then issue the writes

    if ( !pfnSlabSaved && cWrite )
    {
        Call( m_pff->ErrIOIssue() );
    }

    //  complete the request

HandleError:
    SaveRequestComplete( err );
    if ( !pfnSlabSaved )
    {
        const ERR errSave = savecomplete.ErrComplete();
        err = err < JET_errSuccess ? err : errSave;
    }
    return err;
}

template< class I  >
INLINE TCachedBlockSlab<I>::TCachedBlockSlab(   _In_    IFileFilter* const                      pff,
                                                _In_    const QWORD                             ibSlab,
                                                _In_    const QWORD                             cbSlab,
                                                _In_    const size_t                            ccbc,
                                                _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                                                _In_    const QWORD                             icbwcBase,
                                                _In_    const ClusterNumber                     clnoMin,
                                                _In_    const ClusterNumber                     clnoMax )
    :   m_pff( pff ),
        m_ibSlab( ibSlab ),
        m_cbSlab( cbSlab ),
        m_ccbc( ccbc ),
        m_pcbwcm( pcbwcm ),
        m_icbwcBase( icbwcBase ),
        m_clnoMin( clnoMin ),
        m_clnoMax( clnoMax ),
        m_rgcbc( NULL ),
        m_rgerrChunk( NULL ),
        m_errSlab( JET_errSuccess ),
        m_fIgnoreVerificationErrors( fFalse  ),
        m_tonoLast( tonoInvalid ),
        m_rgcUpdate( NULL ),
        m_cUpdate( 0 ),
        m_rgfDirty( NULL ),
        m_fDirty( fFalse ),
        m_rgfSlotSuperceded( NULL ),
        m_pfnSlabSaved( NULL ),
        m_keySlabSaved( NULL ),
        m_errSaved( JET_errSuccess ),
        m_cSaveStepPending( 0 ),
        m_cbidLastRead( CCachedBlockId() ),
        m_chnoLastRead( chnoInvalid ),
        m_slnoLastRead( slnoInvalid )
{
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrInit()
{
    ERR                 err         = JET_errSuccess;
    ERR                 errRead     = JET_errSuccess;
    TraceContextScope   tcScope( iorpBlockCache );

    //  read in all the cached block chunks in the slab

    Alloc( m_rgcbc = (CCachedBlockChunk*)PvOSMemoryPageAlloc( (size_t)m_cbSlab, NULL ) );
    Alloc( m_rgerrChunk = new ERR[ m_ccbc ] );
    errRead = m_pff->ErrIORead( *tcScope, m_ibSlab, (DWORD)m_cbSlab, (BYTE*)m_rgcbc, qosIONormal );
    Call( ErrIgnoreVerificationErrors( errRead ) );

    //  verify each cached block chunk

    for ( size_t icbc = 0; icbc < m_ccbc; icbc++ )
    {
        CCachedBlockChunk* const    pcbc        = Pcbc( icbc );
        const size_t                ccbl        = CCachedBlockChunk::Ccbl();
        CachedBlockWriteCount       cbwc        = cbwcNeverWritten;
        BOOL                        fUninit     = fFalse;

        //  fetch the cached block chunk write count

        Call( m_pcbwcm->ErrGetWriteCount( m_icbwcBase + icbc, &cbwc ) );

        //  verify this cached block chunk

        m_rgerrChunk[ icbc ] = errRead < JET_errSuccess ?
            errRead :
            pcbc->ErrVerify( m_ibSlab + icbc * sizeof( CCachedBlockChunk ), cbwc );
        Call( ErrIgnoreVerificationErrors( m_rgerrChunk[ icbc ] ) );

        //  if this cached block chunk was uninitialized then we will defer initialize the slots

        if ( m_rgerrChunk[ icbc ] == JET_errPageNotInitialized )
        {
            fUninit = fTrue;
            m_rgerrChunk[ icbc ] = JET_errSuccess;
        }

        //  if this chunk passed verification then verify each slot in this cached block chunk

        if ( m_rgerrChunk[ icbc ] >= JET_errSuccess )
        {
            for ( size_t icbl = 0; icbl < ccbl; icbl++ )
            {
                CCachedBlock* const pcbl = pcbc->Pcbl( icbl );

                //  if this cached block chunk was uninit then defer init this slot

                if ( fUninit )
                {
                    new( pcbl ) CCachedBlock(   CCachedBlockId( volumeidInvalid,
                                                                fileidInvalid,
                                                                fileserialInvalid,
                                                                cbnoInvalid ),
                                                m_clnoMin + (DWORD)( ( m_icbwcBase + icbc ) * ccbl + icbl ),
                                                0,
                                                tonoInvalid,
                                                tonoInvalid,
                                                fFalse,
                                                fFalse,
                                                fFalse,
                                                fFalse,
                                                fFalse,
                                                updnoInvalid );
                }

                //  verify this slot

                Call( ErrVerifySlot( CCachedBlockSlot( m_ibSlab, Chno( icbc ), Slno( icbl ), *pcbl ) ) );

                //  determine the max touch number

                if ( pcbl->Tono0() != tonoInvalid && pcbl->Tono0() > m_tonoLast )
                {
                    m_tonoLast = pcbl->Tono0();
                }
            }
        }

        //  accumulate any cached block chunk validation errors

        m_errSlab = m_errSlab < JET_errSuccess ? m_errSlab : m_rgerrChunk[ icbc ];
    }

    //  init and compute our superceded state

    Call( ErrInitSupercededState() );

    Call( ErrComputeSupercededState() );

    //  return our slab validation error

    err = m_errSlab;

HandleError:
    return err;
}

template< class I >
ERR TCachedBlockSlab<I>::ErrInitSupercededState()
{
    ERR             err     = JET_errSuccess;
    const size_t    cislot  = m_ccbc * CCachedBlockChunk::Ccbl();

    Alloc( m_rgfSlotSuperceded = new BOOL[ cislot ] );

    for ( size_t iislot = 0; iislot < cislot; iislot++ )
    {
        m_rgfSlotSuperceded[ iislot ] = fFalse;
    }

HandleError:
    return err;
}

template< class I >
ERR TCachedBlockSlab<I>::ErrComputeSupercededState()
{
    ERR     err     = JET_errSuccess;
    size_t  cislot  = 0;
    ISlot*  rgislot = NULL;

    //  if the slab has validation errors then we cannot yet compute the superceded state.  we will recompute it later
    //  after the invalid chunks have been fixed in ErrAcceptUpdates
    //
    //  NOTE:  even with a valid slab, we may still generate inconsistent results until all chunks in the slab are
    //  updated to their current version as a part of the crash recovery process

    if ( m_errSlab < JET_errSuccess )
    {
        Error( JET_errSuccess );
    }

    //  sort all slots by validity and then by cached block id and then by update number

    Call( ErrCreateSortedIslotArray( &CompareSlotsForInit_, &cislot, &rgislot ) );

    //  use the array to discover all the cached blocks with older versions in the cache

    for ( size_t iislot = 0; iislot < cislot - 1; iislot++ )
    {
        const ISlot                 islotThis   = rgislot[ iislot ];
        const CCachedBlock* const   pcblThis    = Pcbc( Icbc( islotThis ) )->Pcbl( Icbl( islotThis ) );

        const ISlot                 islotNext   = rgislot[ iislot + 1 ];
        const CCachedBlock* const   pcblNext    = Pcbc( Icbc( islotNext ) )->Pcbl( Icbl( islotNext ) );

        m_rgfSlotSuperceded[ (size_t)islotThis ] = fFalse;

        if (    pcblThis->FValid() && pcblNext->FValid() &&
                pcblThis->Cbid().Cbno() == pcblNext->Cbid().Cbno() &&
                pcblThis->Cbid().Fileserial() == pcblNext->Cbid().Fileserial() &&
                pcblThis->Cbid().Fileid() == pcblNext->Cbid().Fileid() &&
                pcblThis->Cbid().Volumeid() == pcblNext->Cbid().Volumeid() &&
                pcblThis->Cbid().Cbno() != cbnoInvalid &&
                pcblThis->Cbid().Fileserial() != fileserialInvalid &&
                pcblThis->Cbid().Fileid() != fileidInvalid &&
                pcblThis->Cbid().Volumeid() != volumeidInvalid )
        {
            //  it is illegal to have more than one slot claim the same version of a cached block

            if ( pcblThis->Updno() == pcblNext->Updno() )
            {
                //  however, this is allowed during recovery

                if ( !m_fIgnoreVerificationErrors )
                {
                    Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlabDuplicateUpdno" ) );
                }
            }

            m_rgfSlotSuperceded[ (size_t)islotThis ] = fTrue;
        }
    }

    m_rgfSlotSuperceded[ (size_t)rgislot[ cislot - 1 ] ] = fFalse;

HandleError:
    delete[] rgislot;
    return err;
}

template<class I>
INLINE void TCachedBlockSlab<I>::GetSlotState(  _In_    const size_t                    icbc,
                                                _In_    const size_t                    icbl,
                                                _Out_   CCachedBlockSlotState* const    pslotst,
                                                _Out_   CUpdate** const                 ppupdateFirst,
                                                _Out_   CUpdate** const                 ppupdateLast )
{
    new( pslotst ) CCachedBlockSlotState();
    *ppupdateFirst = NULL;
    *ppupdateLast = NULL;

    //  compute the absolute address of this slot

    const ChunkNumber           chno                = Chno( icbc );
    const SlotNumber            slno                = Slno( icbl );

    //  find the current image of the slot

    CCachedBlockChunk* const    pcbc                = Pcbc( chno );
    const CCachedBlock*         pcbl                = pcbc->Pcbl( icbl );

    //  determine if the slab, chunk, slot, and cluster have been updated

    CUpdate*                    pupdateFirst        = NULL;
    CUpdate*                    pupdateLast         = NULL;
    CUpdate*                    pupdate             = NULL;
    BOOL                        fSlabUpdated        = fFalse;
    BOOL                        fChunkUpdated       = fFalse;
    BOOL                        fSlotUpdated        = fFalse;
    BOOL                        fClusterUpdated     = fFalse;

    for ( pupdate = m_ilUpdate.PrevMost(); pupdate; pupdate = m_ilUpdate.Next( pupdate ) )
    {
        fSlabUpdated = fTrue;
        fChunkUpdated = fChunkUpdated || pupdate->SlotBefore().Chno() == chno;

        if ( pupdate->SlotBefore().Chno() == chno && pupdate->SlotBefore().Slno() == slno )
        {
            pupdateFirst = pupdateFirst ? pupdateFirst : pupdate;
            pupdateLast = pupdate;

            fSlotUpdated = fTrue;
            fClusterUpdated = pupdate->FClusterUpdated();
        }
    }

    //  determine if the slot is superceded

    BOOL fSuperceded = m_rgfSlotSuperceded[ (size_t)Islot( Icbc( chno ), Icbl( slno ) ) ];

    //  generate the slot state

    new( pslotst ) CCachedBlockSlotState(   CCachedBlockSlot( m_ibSlab, chno, slno, *pcbl ),
                                            fSlabUpdated,
                                            fChunkUpdated,
                                            fSlotUpdated,
                                            fClusterUpdated,
                                            fSuperceded );

    //  return a pointer to the first and last update of this slot, if any

    *ppupdateFirst = pupdateFirst;
    *ppupdateLast = pupdateLast;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrGetSlotForNewImage(  _In_                    const CCachedBlockId&   cbid,
                                                        _In_                    const size_t            cb,
                                                        _In_reads_opt_( cb )    const BYTE* const       rgb,
                                                        _In_                    const BOOL              fWrite,
                                                        _Out_                   CCachedBlockSlot* const pslot )
{
    ERR                     err             = JET_errSuccess;
    const BOOL              fJournalSlab    = ( fWrite &&
                                                cbid.Cbno() == cbnoInvalid &&
                                                cbid.Fileserial() == fileserialInvalid &&
                                                cbid.Fileid() == fileidInvalid &&
                                                cbid.Volumeid() == volumeidInvalid );
    BOOL                    fFoundEmpty     = fFalse;
    size_t                  icbcEmpty       = m_ccbc;
    size_t                  icblEmpty       = CCachedBlockChunk::Ccbl();
    CCachedBlockSlotState   slotstCurrent;
    DWORD                   dwECC           = 0;
    CCachedBlockSlot        slotNew;
    TouchNumber             tonoNew         = tonoInvalid;

    new( pslot ) CCachedBlockSlot();

    //  do not allow operation on a corrupt slab

    Call( m_errSlab );

    //  ensure we are given a valid cached block id

    if (    cbid.Cbno() == cbnoInvalid ||
            cbid.Fileserial() == fileserialInvalid ||
            cbid.Fileid() == fileidInvalid ||
            cbid.Volumeid() == volumeidInvalid )
    {
        //  however we allow a fully invalid cached block id for write to support Journal Slabs

        if ( !fJournalSlab )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    //  ensure we are given a valid data block but only when not for Journal Slabs

    if ( !rgb )
    {
        //  however we allow ErrGetSlotForWrite w/o a buffer to support Journal Slabs

        if ( !fJournalSlab )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else if ( fJournalSlab )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( rgb && cb != cbCachedBlock )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  try to find an empty slot to hold the new image

    for ( size_t icbc = 0; icbc < m_ccbc && !fFoundEmpty; icbc++ )
    {
        CCachedBlockChunk* const    pcbc    = Pcbc( icbc );
        const size_t                ccbl    = CCachedBlockChunk::Ccbl();

        for ( size_t icbl = 0; icbl < ccbl && !fFoundEmpty; icbl++ )
        {
            CCachedBlock* const pcbl = pcbc->Pcbl( icbl );

            //  the slot is empty if it is invalid

            if ( !pcbl->FValid() )
            {
                fFoundEmpty = fTrue;
                icbcEmpty = icbc;
                icblEmpty = icbl;
            }
        }
    }

    //  if we didn't find an empty slot then return an invalid slot

    if ( !fFoundEmpty )
    {
        Error( JET_errSuccess );
    }

    //  try to find this cached block

    Call( ErrGetSlotState( cbid, &slotstCurrent ) );

    //  if we found a cached block and shouldn't have then fail

    if ( !fWrite && slotstCurrent.FValid() )
    {
        Error( ErrERRCheck( JET_errIllegalOperation ) );
    }

    //  update the checksum for the provided data, if any

    if ( rgb )
    {
        dwECC = DwECCChecksumFromXEChecksum( ChecksumNewFormat( (const BYTE*)rgb, cb, 0, fFalse ) );
    }
    else
    {
        dwECC = slotstCurrent.DwECC();
    }

    //  generate an image of the slot holding the new image of this cached block

    tonoNew = TonoGetNextTouch();
    new( &slotNew ) CCachedBlockSlot(   m_ibSlab,
                                        Chno( icbcEmpty ),
                                        Slno( icblEmpty ),
                                        CCachedBlock(   cbid,
                                                        Pcbc( icbcEmpty )->Pcbl( icblEmpty )->Clno(),
                                                        dwECC,
                                                        tonoNew,
                                                        slotstCurrent.Tono0() == tonoInvalid ?
                                                            tonoNew :
                                                            slotstCurrent.Tono0(),
                                                        fTrue,
                                                        slotstCurrent.FPinned(),
                                                        fWrite,
                                                        fWrite || slotstCurrent.FEverDirty(),
                                                        fFalse,
                                                        slotstCurrent.Updno() + ( fWrite ? 1 : 0 ) ) );

    //  if we are replacing a different slot then fetch its before image

    if ( slotstCurrent.Chno() != slotNew.Chno() || slotstCurrent.Slno() != slotNew.Slno() )
    {
        GetSlotState( Icbc( slotNew.Chno() ), Icbl( slotNew.Slno() ), &slotstCurrent );
    }

    //  verify that this change is allowed

    err = ErrVerifySlotUpdate( slotstCurrent, slotNew );
    CallS( err );
    Call( err );

    //  return the slot we found

    UtilMemCpy( pslot, &slotNew, sizeof( *pslot ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        new( pslot ) CCachedBlockSlot();
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrGetSlotState(    _In_    const CCachedBlockId&           cbid,
                                                    _Out_   CCachedBlockSlotState* const    pslotst )
{
    ERR                     err             = JET_errSuccess;
    CCachedBlockSlotState   slotstCurrent;

    new( pslotst ) CCachedBlockSlotState();

    //  if the request is for an invalid cached block id then don't return a slot

    if (    cbid.Cbno() == cbnoInvalid ||
            cbid.Fileserial() == fileserialInvalid ||
            cbid.Fileid() == fileidInvalid ||
            cbid.Volumeid() == volumeidInvalid )
    {
    }

    //  if this is the same cached block we just sought then we already know the result of the search

    else if (   m_cbidLastRead.Cbno() == cbid.Cbno() &&
                m_cbidLastRead.Fileserial() == cbid.Fileserial() &&
                m_cbidLastRead.Fileid() == cbid.Fileid() &&
                m_cbidLastRead.Volumeid() == cbid.Volumeid() )
    {
        //  if the cached block is resident then get the current slot state

        if ( m_chnoLastRead != chnoInvalid )
        {
            GetSlotState( Icbc( m_chnoLastRead ), Icbl( m_slnoLastRead ), &slotstCurrent );
        }
    }

    //  scan the slab looking for the slot containing the current version of this cached block

    else
    {
        for ( size_t icbc = 0; icbc < m_ccbc && !slotstCurrent.FValid(); icbc++ )
        {
            CCachedBlockChunk* const    pcbc    = Pcbc( icbc );
            const size_t                ccbl    = CCachedBlockChunk::Ccbl();

            for ( size_t icbl = 0; icbl < ccbl && !slotstCurrent.FValid(); icbl++ )
            {
                const CCachedBlock* const pcbl = pcbc->Pcbl( icbl );

                //  if this slot isn't ours then skip it

                if (    pcbl->Cbid().Cbno() != cbid.Cbno() ||
                        pcbl->Cbid().Fileserial() != cbid.Fileserial() ||
                        pcbl->Cbid().Fileid() != cbid.Fileid() ||
                        pcbl->Cbid().Volumeid() != cbid.Volumeid() )
                {
                    continue;
                }

                //  get the current slot state

                GetSlotState( icbc, icbl, &slotstCurrent );

                //  if this isn't the current copy of the slot then skip it

                if ( slotstCurrent.FSuperceded() )
                {
                    new( &slotstCurrent ) CCachedBlockSlotState();
                    continue;
                }
            }
        }
    }

    //  cache the location of this cached block id, if any.  it is common to access the same block twice

    new( &m_cbidLastRead ) CCachedBlockId(  slotstCurrent.Cbid().Volumeid(),
                                            slotstCurrent.Cbid().Fileid(),
                                            slotstCurrent.Cbid().Fileserial(),
                                            slotstCurrent.Cbid().Cbno() );
    m_chnoLastRead = slotstCurrent.FValid() ? slotstCurrent.Chno() : chnoInvalid;
    m_slnoLastRead = slotstCurrent.FValid() ? slotstCurrent.Slno() : slnoInvalid;

    //  we found the slot

    if ( slotstCurrent.FValid() )
    {
        //  the slot must contain the current version of the sought cached block id

        if (    slotstCurrent.Cbid().Cbno() != cbid.Cbno() ||
                slotstCurrent.Cbid().Fileserial() != cbid.Fileserial() ||
                slotstCurrent.Cbid().Fileid() != cbid.Fileid() ||
                slotstCurrent.Cbid().Volumeid() != cbid.Volumeid() ||
                slotstCurrent.FSuperceded() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlabGetSlotState" ) );
        }

        //  return the slot we found

        UtilMemCpy( pslotst, &slotstCurrent, sizeof( *pslotst ) );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        new( pslotst ) CCachedBlockSlotState();
    }
    return err;
}

template<class I>
INLINE TouchNumber TCachedBlockSlab<I>::TonoGetNextTouch()
{
    return ++m_tonoLast;
}

template<class I>
INLINE ERR TCachedBlockSlab<I>::ErrVerifySlotAddress( _In_ const CCachedBlockSlot& slot )
{
    ERR err = JET_errSuccess;

    //  the slot must belong to this slab

    if ( slot.IbSlab() != m_ibSlab )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotIncorrectSlab" ) );
    }

    //  the chunk number and slot number must always be valid

    if ( Icbc( slot.Chno() ) >= m_ccbc )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotInvalidChno" ) );
    }
    if ( Icbl( slot.Slno() ) >= CCachedBlockChunk::Ccbl() )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotInvalidSlno" ) );
    }

HandleError:
    return err;
}

template<class I>
INLINE ERR TCachedBlockSlab<I>::ErrVerifySlot( _In_ const CCachedBlockSlot& slot )
{
    ERR err = JET_errSuccess;

    //  the slot address must be valid

    Call( ErrVerifySlotAddress( slot ) );

    //  the cached block id must either be fully invalid or have no invalid components

    if (    slot.Cbid().Volumeid() == volumeidInvalid &&
            slot.Cbid().Fileid() == fileidInvalid &&
            slot.Cbid().Fileserial() == fileserialInvalid &&
            slot.Cbid().Cbno() == cbnoInvalid )
    {
    }
    else if (   slot.Cbid().Volumeid() == volumeidInvalid ||
                slot.Cbid().Fileid() == fileidInvalid ||
                slot.Cbid().Fileserial() == fileserialInvalid ||
                slot.Cbid().Cbno() == cbnoInvalid )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotInvalidCbid" ) );
    }

    //  the slot must always have a valid cluster

    if ( slot.Clno() < m_clnoMin || slot.Clno() >= m_clnoMax || slot.Clno() == clnoInvalid )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotInvalidClno" ) );
    }

    //  the slot must either have all invalid touches, tono0 valid and tono1 invalid, or both valid and tono0 >= tono1

    if ( slot.Tono0() == tonoInvalid && slot.Tono1() == tonoInvalid )
    {
    }
    else if ( slot.Tono0() != tonoInvalid && slot.Tono1() == tonoInvalid )
    {
    }
    else if ( slot.Tono0() != tonoInvalid && slot.Tono1() != tonoInvalid && slot.Tono0() >= slot.Tono1() )
    {
    }
    else
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotInvalidTono" ) );
    }

    //  the slot must have a valid touch number iff valid

    if ( !slot.FValid() && ( slot.Tono0() != tonoInvalid || slot.Tono1() != tonoInvalid ) )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotIllegalTono" ) );
    }
    if ( slot.FValid() && ( slot.Tono0() == tonoInvalid && slot.Tono1() == tonoInvalid ) )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotMissingTono" ) );
    }

    //  an invalid slot may not be pinned or dirty

    if ( !slot.FValid() && ( slot.FPinned() || slot.FDirty() ) )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotInvalidAndPinnedOrDirty" ) );
    }

    //  a dirty slot must also be marked as ever dirty

    if ( slot.FDirty() && !slot.FEverDirty() )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotDirtyAndNotEverDirty" ) );
    }

    //  a slot that may only have an update number iff it has ever been dirty

    if ( !slot.FEverDirty() && slot.Updno() != updnoInvalid )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotIllegalUpdno" ) );
    }
    if ( slot.FEverDirty() && slot.Updno() == updnoInvalid )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotMissingUpdno" ) );
    }

    //  a purged slot may not be valid, pinned, or dirty

    if ( slot.FPurged() && ( slot.FValid() || slot.FPinned() || slot.FDirty() ) )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotPurgedAndValidOrPinnedOrDirty" ) );
    }

    //  the reserved space must always be zero

    if ( slot.RgbitReserved0() != 0 )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotReserved" ) );
    }

HandleError:
    return err;
}

template<class I>
INLINE ERR TCachedBlockSlab<I>::ErrVerifySlotUpdate(    _In_ const CCachedBlockSlotState&   slotstCurrent,
                                                        _In_ const CCachedBlockSlot&        slotNew )
{
    ERR err = JET_errSuccess;

    //  verify the new slot state

    Call( ErrVerifySlot( slotNew ) );

    //  state transition validation

    //  no change is always ok

    if ( &slotstCurrent == &slotNew )
    {
        Error( JET_errSuccess );
    }

    if ( memcmp( &slotstCurrent, &slotNew, sizeof( slotNew ) ) == 0 )
    {
        Error( JET_errSuccess );
    }

    //  the chunk number and slot number must not change from one valid value to another

    if (    slotstCurrent.Chno() == chnoInvalid &&
            slotstCurrent.Slno() == slnoInvalid )
    {
    }
    else if (   slotNew.Chno() != slotstCurrent.Chno() ||
                slotNew.Slno() != slotstCurrent.Slno() )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalAddressChange" ) );
    }

    //  the cached block id may not change unless we are caching new data

    if ( !slotstCurrent.FValid() && slotNew.FValid() )
    {
    }
    else if (   slotNew.Cbid().Volumeid() != slotstCurrent.Cbid().Volumeid() ||
                slotNew.Cbid().Fileid() != slotstCurrent.Cbid().Fileid() ||
                slotNew.Cbid().Fileserial() != slotstCurrent.Cbid().Fileserial() ||
                slotNew.Cbid().Cbno() != slotstCurrent.Cbid().Cbno() )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalCbidChange" ) );
    }

    //  a slot can only be unpinned by invalidate or by evict when the slot is superceded

    if ( slotstCurrent.FPinned() && !slotNew.FPinned() )
    {
        if ( slotNew.FPurged() )
        {
        }
        else if ( !slotNew.FValid() && slotstCurrent.FSuperceded() )
        {
        }
        else
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalUnpin" ) );
        }
    }

    //  a slot can only go from dirty to invalid on invalidate or evict when the slot is superceded

    if ( slotstCurrent.FDirty() && !slotNew.FValid() )
    {
        if ( slotNew.FPurged() )
        {
        }
        else if ( !slotNew.FValid() && slotstCurrent.FSuperceded() )
        {
        }
        else
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalPurge" ) );
        }
    }

    //  caching new data is the only time when a slot can have its ever dirty flag reset

    if ( !( !slotstCurrent.FValid() && slotNew.FValid() ) && slotstCurrent.FEverDirty() && !slotNew.FEverDirty() )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalDirtyReset" ) );
    }

    //  caching new data is the only time when a slot can have its ever update number reset

    if ( !( !slotstCurrent.FValid() && slotNew.FValid() ) && slotstCurrent.Updno() != updnoInvalid && slotNew.Updno() == updnoInvalid )
    {
        Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalUpdnoReset" ) );
    }

    //  scenario validation

    //  if we are caching new clean data (!FValid to FValid and !FDirty due to ErrGetSlotForCache) then the slot should
    //  be touched once

    if ( !slotstCurrent.FValid() && slotNew.FValid() && !slotNew.FDirty() )
    {
        if ( !( slotNew.Tono0() != tonoInvalid && slotNew.Tono1() == slotNew.Tono0() ) )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateCleanDataMustBeTouchedOnce" ) );
        }
    }

    //  if we are caching new data (!FValid to FValid and FDirty() due to ErrGetSlotForWrite) then the slot should be
    //  touched at least once

    if ( !slotstCurrent.FValid() && slotNew.FValid() )
    {
        if ( slotNew.Tono0() == tonoInvalid )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateNewDataMustBeTouched" ) );
        }
    }

    //  if we are reading existing data (FValid to FValid and FDirty unchanged due to ErrGetSlotForRead) then the slot
    //  should be touched at least one more time

    if ( slotstCurrent.FValid() && slotNew.FValid() && slotNew.FDirty() == slotstCurrent.FDirty() )
    {
        if ( slotNew.Tono1() != slotstCurrent.Tono0() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateReadMustTouch" ) );
        }
        if ( slotNew.Tono0() < slotNew.Tono1() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalTonos" ) );
        }
    }

    //  if we are cleaning a slot (FValid to FValid and FDirty to !FDirty() then nothing else must change and the slot
    //  must not be pinned

    if ( slotstCurrent.FValid() && slotNew.FValid() && slotstCurrent.FDirty() && !slotNew.FDirty() )
    {
        if (    slotNew.Cbid().Volumeid() != slotstCurrent.Cbid().Volumeid() ||
                slotNew.Cbid().Fileid() != slotstCurrent.Cbid().Fileid() ||
                slotNew.Cbid().Fileserial() != slotstCurrent.Cbid().Fileserial() ||
                slotNew.Cbid().Cbno() != slotstCurrent.Cbid().Cbno() ||
                slotNew.Clno() != slotstCurrent.Clno() ||
                slotNew.DwECC() != slotstCurrent.DwECC() ||
                slotNew.Tono0() != slotstCurrent.Tono0() ||
                slotNew.Tono1() != slotstCurrent.Tono1() ||
                slotNew.FPinned() != slotstCurrent.FPinned() ||
                slotNew.FEverDirty() != slotstCurrent.FEverDirty() ||
                slotNew.FPurged() != slotstCurrent.FPurged() ||
                slotNew.Updno() != slotstCurrent.Updno() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalCleanStateTransition" ) );
        }

        if ( slotstCurrent.FPinned() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalCleanOfPinned" ) );
        }
    }

    //  if we are evicting a slot (FValid and !FDirty to !FValid) then the slot must not be pinned but not superceded
    //  and it must not be dirty but not superceded

    if ( slotstCurrent.FValid() && !slotstCurrent.FDirty() && !slotNew.FValid() && !slotNew.FPurged() )
    {
        if (    slotNew.Cbid().Volumeid() != slotstCurrent.Cbid().Volumeid() ||
                slotNew.Cbid().Fileid() != slotstCurrent.Cbid().Fileid() ||
                slotNew.Cbid().Fileserial() != slotstCurrent.Cbid().Fileserial() ||
                slotNew.Cbid().Cbno() != slotstCurrent.Cbid().Cbno() ||
                slotNew.Clno() == clnoInvalid ||
                slotNew.DwECC() != slotstCurrent.DwECC() ||
                slotNew.FValid() != fFalse ||
                slotNew.FPinned() != fFalse ||
                slotNew.FDirty() != fFalse ||
                slotNew.FEverDirty() != slotstCurrent.FEverDirty() ||
                slotNew.FPurged() != fFalse ||
                slotNew.Updno() != slotstCurrent.Updno() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalEvictStateTransition" ) );
        }

        if ( slotstCurrent.FPinned() && !slotstCurrent.FSuperceded() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalEvictOfPinned" ) );
        }

        if ( slotstCurrent.FDirty() && !slotstCurrent.FSuperceded() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalEvictOfDirty" ) );
        }
    }

    //  if we are invalidating a slot (FValid to !FValid and FPurged) then any transition is supported and the purge
    //  indicator must be set if we dropped a dirty cached block that should have been written back

    if ( slotstCurrent.FValid() && !slotNew.FValid() && slotNew.FPurged() )
    {
        if (    slotNew.Cbid().Volumeid() != slotstCurrent.Cbid().Volumeid() ||
                slotNew.Cbid().Fileid() != slotstCurrent.Cbid().Fileid() ||
                slotNew.Cbid().Fileserial() != slotstCurrent.Cbid().Fileserial() ||
                slotNew.Cbid().Cbno() != slotstCurrent.Cbid().Cbno() ||
                slotNew.Clno() == clnoInvalid ||
                slotNew.DwECC() != slotstCurrent.DwECC() ||
                slotNew.FValid() != fFalse ||
                slotNew.FPinned() != fFalse ||
                slotNew.FDirty() != fFalse ||
                slotNew.FEverDirty() != slotstCurrent.FEverDirty() ||
                slotNew.FPurged() != slotstCurrent.FDirty() && !slotstCurrent.FSuperceded() ||
                slotNew.Updno() != slotstCurrent.Updno() )
        {
            Error( ErrBlockCacheInternalError( "HashedLRUKCacheSlotUpdateIllegalInvalidateStateTransition" ) );
        }
    }

HandleError:
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrVerifyCluster(   _In_                const CCachedBlockSlot&         slot,
                                                    _In_                const size_t                    cb,
                                                    _In_reads_( cb )    const BYTE* const               rgb,
                                                    _In_                const BOOL                      fRepair,
                                                    _Out_               CCachedBlockSlotState* const    pslotst,
                                                    _Out_               CUpdate** const                 ppupdateLast )
{
    ERR                     err             = JET_errSuccess;
    CCachedBlockSlotState&  slotstCurrent   = *pslotst;
    CUpdate*                pupdateFirst    = NULL;
    CUpdate*                pupdateLast     = NULL;
    DWORD                   dwECCActual     = 0;

    new ( pslotst ) CCachedBlockSlotState();
    *ppupdateLast = NULL;

    //  the slot address must be valid

    Call( ErrVerifySlotAddress( slot ) );

    //  ensure we are given a valid data block

    if ( !rgb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cb != cbCachedBlock )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  lookup the current slot state

    GetSlotState( Icbc( slot.Chno() ), Icbl( slot.Slno() ), &slotstCurrent, &pupdateFirst, &pupdateLast );

    //  the slot must hold a valid current copy of the same cached block

    if ( !slot.FValid() )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if (    slot.Cbid().Volumeid() == volumeidInvalid ||
            slot.Cbid().Fileid() == fileidInvalid || 
            slot.Cbid().Fileserial() == fileserialInvalid || 
            slot.Cbid().Cbno() == cbnoInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( !slotstCurrent.FValid() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( slotstCurrent.FSuperceded() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if (    slot.Cbid().Volumeid() != slotstCurrent.Cbid().Volumeid() ||
            slot.Cbid().Fileid() != slotstCurrent.Cbid().Fileid() ||
            slot.Cbid().Fileserial() != slotstCurrent.Cbid().Fileserial() ||
            slot.Cbid().Cbno() != slotstCurrent.Cbid().Cbno() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( slot.Clno() != slotstCurrent.Clno() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    if ( slot.DwECC() != slotstCurrent.DwECC() )
    {
        //  however, this is allowed during recovery

        if ( !m_fIgnoreVerificationErrors )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    //  validate the checksum of the cluster

    dwECCActual = DwECCChecksumFromXEChecksum( ChecksumNewFormat( rgb, cb, 0, fFalse ) );

    if ( dwECCActual != slot.DwECC() )
    {
        //  if we aren't attempting to repair the corruption then fail

        if ( !fRepair )
        {
            Error( ErrERRCheck( JET_errReadVerifyFailure ) );
        }

        //  try to repair the corruption

        Call( ErrRepairCluster( slot, cb, rgb, slot.DwECC(), dwECCActual ) );
    }

    //  return the slot state and update

    UtilMemCpy( pslotst, &slotstCurrent, sizeof( *pslotst ) );
    *ppupdateLast = pupdateLast;

HandleError:
    if ( err < JET_errSuccess )
    {
        new ( pslotst ) CCachedBlockSlotState();
        *ppupdateLast = NULL;
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockSlab<I>::ErrRepairCluster(   _In_                const CCachedBlockSlot&         slot,
                                                    _In_                const size_t                    cb,
                                                    _In_reads_( cb )    const BYTE* const               rgb,
                                                    _In_                const DWORD                     dwECCExpected,
                                                    _In_                const DWORD                     dwECCActual )
{
    ERR     err             = JET_errSuccess;
    DWORD   dwECCCurrent    = 0;

    //  determine if we have a correctable error

    if ( FECCErrorIsCorrectable( cb, dwECCExpected, dwECCActual ) )
    {
        //  try to use the ECC to determine the location of a single bit error

        const UINT ibitCorrupted = IbitCorrupted( cb, dwECCExpected, dwECCActual );

        //  if we think we found a single bit error then flip it back

        const UINT ib = ibitCorrupted / 8;

        if ( ib < cb )
        {
            ( (BYTE*)rgb )[ ib ] ^= (BYTE)( 1 << ( ibitCorrupted % 8 ) );
        }
    }

    //  recompute the ECC and fail if we haven't succeeded in fixing the error

    dwECCCurrent = DwECCChecksumFromXEChecksum( ChecksumNewFormat( rgb, cb, 0, fFalse ) );
    if ( dwECCCurrent != dwECCExpected )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

HandleError:
    return err;
}

template<class I>
INLINE void TCachedBlockSlab<I>::RevertUnacceptedUpdates( _In_ const BOOL fPermanently )
{
    CUpdate* pupdatePrev = NULL;
    for ( CUpdate* pupdate = m_ilUpdate.NextMost(); pupdate; pupdate = pupdatePrev )
    {
        pupdatePrev = m_ilUpdate.Prev( pupdate );

        RevertUnacceptedUpdate( pupdate, fPermanently );
    }
}

template<class I>
INLINE void TCachedBlockSlab<I>::RevertUnacceptedUpdate( _In_ CUpdate* const pupdate, _In_ const BOOL fPermanently )
{
    const size_t                icbc = Icbc( pupdate->SlotBefore().Chno() );
    CCachedBlockChunk* const    pcbc = Pcbc( icbc );
    const size_t                icbl = Icbl( pupdate->SlotBefore().Slno() );
    CCachedBlock* const         pcbl = pcbc->Pcbl( icbl );

    UtilMemCpy( pcbl, &pupdate->SlotBefore(), sizeof( *pcbl ) );

    m_rgfSlotSuperceded[ (size_t)Islot( icbc, icbl ) ] = pupdate->FSupercededBefore();

    if ( pupdate->ChnoSuperceded() != chnoInvalid )
    {
        m_rgfSlotSuperceded[ (size_t)Islot( Icbc( pupdate->ChnoSuperceded() ), Icbl( pupdate->SlnoSuperceded() ) ) ] = fFalse;
    }

    m_rgcUpdate[ icbc ]--;
    m_cUpdate--;

    if ( fPermanently )
    {
        m_ilUpdate.Remove( pupdate );
        delete pupdate;
    }

    InvalidateCachedRead();
}

template<class I>
INLINE void TCachedBlockSlab<I>::RestoreUnacceptedUpdates()
{
    for ( CUpdate* pupdate = m_ilUpdate.PrevMost(); pupdate; pupdate = m_ilUpdate.Next( pupdate ) )
    {
        const size_t                icbc = Icbc( pupdate->SlotAfter().Chno() );
        CCachedBlockChunk* const    pcbc = Pcbc( icbc );
        const size_t                icbl = Icbl( pupdate->SlotAfter().Slno() );
        CCachedBlock* const         pcbl = pcbc->Pcbl( icbl );

        UtilMemCpy( pcbl, &pupdate->SlotAfter(), sizeof( *pcbl ) );

        m_rgfSlotSuperceded[ (size_t)Islot( icbc, icbl ) ] = pupdate->FSupercededAfter();

        if ( pupdate->ChnoSuperceded() != chnoInvalid )
        {
            m_rgfSlotSuperceded[ (size_t)Islot( Icbc( pupdate->ChnoSuperceded() ), Icbl( pupdate->SlnoSuperceded() ) ) ] = fTrue;
        }

        m_rgcUpdate[ icbc ]++;
        m_cUpdate++;
    }

    InvalidateCachedRead();
}

template<class I>
INLINE void TCachedBlockSlab<I>::InvalidateCachedRead()
{
    new( &m_cbidLastRead ) CCachedBlockId();
}

template<class I>
INLINE ERR TCachedBlockSlab<I>::ErrResetChunk( _In_ const size_t icbc )
{
    ERR                         err     = JET_errSuccess;
    const size_t                ccbl    = CCachedBlockChunk::Ccbl();
    CCachedBlockChunk* const    pcbc    = Pcbc( icbc );
    const QWORD                 ibSlab  = m_ibSlab + icbc * sizeof( CCachedBlockChunk );
    CachedBlockWriteCount       cbwc    = cbwcUnknown;

    //  reset the chunk to a usable state but with empty slots with invalid cluster numbers.  this will prevent the
    //  chunk from being saved unless all the slots have been fixed.  note that we cannot know the cluster numbers we
    //  could use anyway.  the cluster numbers we assign for the real defer init state could have been reassigned to
    //  some other slots

    Call( m_pcbwcm->ErrGetWriteCount( m_icbwcBase + icbc, &cbwc ) );
    pcbc->Reset( ibSlab, cbwc );

    for ( size_t icbl = 0; icbl < ccbl; icbl++ )
    {
        new( pcbc->Pcbl( icbl ) ) CCachedBlock( CCachedBlockId( volumeidInvalid,
                                                                fileidInvalid,
                                                                fileserialInvalid,
                                                                cbnoInvalid ),
                                                clnoInvalid,
                                                0,
                                                tonoInvalid,
                                                tonoInvalid,
                                                fFalse,
                                                fFalse,
                                                fFalse,
                                                fFalse,
                                                fFalse,
                                                updnoInvalid );
    }

    //  clear the error state for this chunk and possibly for the entire slab

    m_rgerrChunk[ icbc ] = JET_errSuccess;

    m_errSlab = JET_errSuccess;
    for ( int icbcT = 0; icbcT < m_ccbc; icbcT++ )
    {
        m_errSlab = m_errSlab < JET_errSuccess ? m_errSlab : m_rgerrChunk[ icbcT ];
    }

HandleError:
    return err;
}

template<class I>
INLINE ERR TCachedBlockSlab<I>::ErrCreateSortedIslotArray(  _In_                                const PfnCompare    pfnCompare,
                                                            _Out_                               size_t* const       pcislot,
                                                            _Outptr_result_buffer_( *pcislot )  ISlot** const       prgislot )
{
    ERR             err     = JET_errSuccess;
    const size_t    cislot  = m_ccbc * CCachedBlockChunk::Ccbl();
    ISlot*          rgislot = NULL;

    *pcislot = 0;
    *prgislot = NULL;

    //  sort all slots by the given criteria

    Alloc( rgislot = new ISlot[ cislot ] );
    for ( size_t iislot = 0; iislot < cislot; iislot++ )
    {
        rgislot[ iislot ] = (ISlot)iislot;
    }

    qsort_s( rgislot, cislot, sizeof( rgislot[ 0 ] ), pfnCompare, this );

    //  return the array

    *prgislot = rgislot;
    rgislot = NULL;
    *pcislot = cislot;

HandleError:
    delete[] rgislot;
    if ( err < JET_errSuccess )
    {
        delete[] *prgislot;
        *prgislot = NULL;
        *pcislot = 0;
    }
    return err;
}

template< class I  >
int TCachedBlockSlab<I>::CompareSlotsForInit( _In_ const ISlot islotA, _In_ const ISlot islotB )
{
    //  recover pointers to the cached blocks for each slot

    const CCachedBlock* const   pcblA   = Pcbc( Icbc( islotA ) )->Pcbl( Icbl( islotA ) );
    const CCachedBlock* const   pcblB   = Pcbc( Icbc( islotB ) )->Pcbl( Icbl( islotB ) );

    //  compare the slots by their validity and then identity and then update number

    if ( !pcblA->FValid() && pcblB->FValid() )
    {
        return -1;
    }
    if ( pcblA->FValid() && !pcblB->FValid() )
    {
        return 1;
    }
    if ( !pcblA->FValid() && !pcblB->FValid() )
    {
        if ( islotA < islotB )
        {
            return -1;
        }
        if ( islotA > islotB )
        {
            return 1;
        }
        return 0;
    }

    if ( pcblA->Cbid().Volumeid() < pcblB->Cbid().Volumeid() )
    {
        return -1;
    }
    if ( pcblA->Cbid().Volumeid() > pcblB->Cbid().Volumeid() )
    {
        return 1;
    }
    if ( pcblA->Cbid().Fileid() < pcblB->Cbid().Fileid() )
    {
        return -1;
    }
    if ( pcblA->Cbid().Fileid() > pcblB->Cbid().Fileid() )
    {
        return 1;
    }
    if ( pcblA->Cbid().Fileserial() < pcblB->Cbid().Fileserial() )
    {
        return -1;
    }
    if ( pcblA->Cbid().Fileserial() > pcblB->Cbid().Fileserial() )
    {
        return 1;
    }
    if ( pcblA->Cbid().Cbno() < pcblB->Cbid().Cbno() )
    {
        return -1;
    }
    if ( pcblA->Cbid().Cbno() > pcblB->Cbid().Cbno() )
    {
        return 1;
    }

    if ( pcblA->Updno() < pcblB->Updno() )
    {
        return -1;
    }
    if ( pcblA->Updno() > pcblB->Updno() )
    {
        return 1;
    }
    if ( islotA < islotB )
    {
        return -1;
    }
    if ( islotA > islotB )
    {
        return 1;
    }
    return 0;
}

template< class I  >
int TCachedBlockSlab<I>::CompareSlotsForEvict( _In_ const ISlot islotA, _In_ const ISlot islotB )
{
    //  retrieve the current state of each slot

    CCachedBlockSlotState slotstA;
    GetSlotState( Icbc( islotA ), Icbl( islotA ), &slotstA );

    CCachedBlockSlotState slotstB;
    GetSlotState( Icbc( islotB ), Icbl( islotB ), &slotstB );

    //  compare the slots for their relative utility vs our cache replacement policy
    //
    //  The replacement order is:  !FValid, FSuperceded, touched once by touch number ascending, and then touched twice
    //  by previous touch number ascending.

    if ( !slotstA.FValid() && slotstB.FValid() )
    {
        return -1;
    }
    if ( slotstA.FValid() && !slotstB.FValid() )
    {
        return 1;
    }
    if ( !slotstA.FValid() && !slotstB.FValid() )
    {
        if ( islotA < islotB )
        {
            return -1;
        }
        if ( islotA > islotB )
        {
            return 1;
        }
        return 0;
    }

    if ( slotstA.FSuperceded() && !slotstB.FSuperceded() )
    {
        return -1;
    }
    if ( !slotstA.FSuperceded() && slotstB.FSuperceded() )
    {
        return 1;
    }
    if ( slotstA.FSuperceded() && slotstB.FSuperceded() )
    {
        if ( islotA < islotB )
        {
            return -1;
        }
        if ( islotA > islotB )
        {
            return 1;
        }
        return 0;
    }

    if ( slotstA.Tono0() == slotstA.Tono1() && slotstB.Tono0() != slotstB.Tono1() )
    {
        return -1;
    }
    if ( slotstA.Tono0() != slotstA.Tono1() && slotstB.Tono0() == slotstB.Tono1() )
    {
        return 1;
    }

    if ( slotstA.Tono1() < slotstB.Tono1() )
    {
        return -1;
    }
    if ( slotstA.Tono1() > slotstB.Tono1() )
    {
        return 1;
    }

    if ( islotA < islotB )
    {
        return -1;
    }
    if ( islotA > islotB )
    {
        return 1;
    }
    return 0;
}

//  CCachedBlockSlab:  concrete TCachedBlockSlab<ICachedBlockSlab>

class CCachedBlockSlab  //  cbsm
    :   public TCachedBlockSlab<ICachedBlockSlab>
{
    public:  //  specialized API

        static ERR ErrLoad( _In_    IFileFilter* const                      pff,
                            _In_    const QWORD                             ibSlab,
                            _In_    const QWORD                             cbSlab,
                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                            _In_    const QWORD                             icbwcBase,
                            _In_    const ClusterNumber                     clnoMin,
                            _In_    const ClusterNumber                     clnoMax,
                            _In_    const BOOL                              fIgnoreVerificationErrors,
                            _Inout_ CCachedBlockSlab** const                ppcbs )
        {
            ERR                 err     = JET_errSuccess;
            const size_t        ccbc    = (size_t)( cbSlab / sizeof( CCachedBlockChunk ) );
            CCachedBlockSlab*   pcbs    = NULL;
            ERR                 errSlab = JET_errSuccess;

            *ppcbs = NULL;

            if ( ibSlab % sizeof( CCachedBlockChunk ) )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( !cbSlab )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( cbSlab % sizeof( CCachedBlockChunk ) )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( cbSlab > dwMax )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            //  create the slab

            Alloc( pcbs = new CCachedBlockSlab( pff,
                                                ibSlab, 
                                                cbSlab, 
                                                ccbc, 
                                                pcbwcm, 
                                                icbwcBase, 
                                                clnoMin, 
                                                clnoMax ) );

            //  init the slab

            errSlab = pcbs->ErrInit();
            Call( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( errSlab ) : errSlab );

            //  return the slab and its validation status

            *ppcbs = pcbs;
            pcbs = NULL;

            err = errSlab;

        HandleError:
            delete pcbs;
            if ( ( fIgnoreVerificationErrors ? ErrIgnoreVerificationErrors( err ) : err ) < JET_errSuccess )
            {
                delete *ppcbs;
                *ppcbs = NULL;
            }
            return err;
        }

    private:

        CCachedBlockSlab(   _In_    IFileFilter* const                      pff,
                            _In_    const QWORD                             ibSlab,
                            _In_    const QWORD                             cbSlab,
                            _In_    const size_t                            ccbc,
                            _In_    ICachedBlockWriteCountsManager* const   pcbwcm,
                            _In_    const QWORD                             icbwcBase,
                            _In_    const ClusterNumber                     clnoMin,
                            _In_    const ClusterNumber                     clnoMax )
            : TCachedBlockSlab<ICachedBlockSlab>( pff, ibSlab, cbSlab, ccbc, pcbwcm, icbwcBase, clnoMin, clnoMax )
        {
        }
};
