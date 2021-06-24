// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TJournal:  implementation of IJournal and its derivatives.
//
//  I:  IJournal or derivative
//
//  This class encapsulates the journal recovery scheme as well as the assembly of journal entries from journal regions
//  stored in the underlying journal segments.

template< class I >
class TJournal  //  j
    :   public I
{
    public:  //  specialized API

        ~TJournal();

        ERR ErrInit();

    public:  //  IJournal

        ERR ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                _Out_opt_ JournalPosition* const    pjposDurable ) override;

        ERR ErrVisitEntries(    _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                _In_ const DWORD_PTR                keyVisitEntry ) override;

        ERR ErrRepair(  _In_    const JournalPosition   jposInvalidate,
                        _Out_   JournalPosition* const  pjposInvalidated ) override;

        ERR ErrAppendEntry( _In_                const size_t            cjb,
                            _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                            _Out_               JournalPosition* const  pjpos,
                            _Out_               JournalPosition* const  pjposEnd ) override;

        ERR ErrFlush() override;

        ERR ErrTruncate( _In_ const JournalPosition jposReplay ) override;

    protected:

        TJournal( _Inout_ IJournalSegmentManager** const  ppjsm, _In_ const size_t cbCache );

    private:

        //  Cached, modified segment that is not yet sealed (written back to storage).

        class CSegment
        {
            public:

                CSegment(   _In_ TJournal<I>* const         pj, 
                            _In_ IJournalSegment** const    ppjs, 
                            _In_ const SegmentPosition      spos )
                    :   m_pj( pj ),
                        m_pjs( *ppjs ),
                        m_spos( spos ),
                        m_cbEntryAppended( 0 ),
                        m_fSealed( fFalse ),
                        m_sigSeal( CSyncBasicInfo( "TJournal<I>::CSegment::m_sigSeal" ) ),
                        m_errSeal( JET_errSuccess )
                {
                    m_sigSeal.Set();
                    *ppjs = NULL;
                }

                ~CSegment()
                {
                    m_sigSeal.Wait();
                    delete m_pjs;
                }

                SegmentPosition Spos() const { return m_spos; }
                DWORD CbEntryAppended() const { return m_cbEntryAppended; }

                ERR ErrAppendRegion(    _In_                const size_t            cjb,
                                        _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                        _In_                const DWORD             cbMin,
                                        _Out_               RegionPosition* const   prpos,
                                        _Out_               DWORD* const            pcbActual );

                ERR ErrBeginSeal();

                static SIZE_T OffsetOfILE() { return OffsetOf( CSegment, m_ile ); }

            private:

                static void Sealed_(    _In_ const ERR              err,
                                        _In_ const SegmentPosition  spos,
                                        _In_ const DWORD_PTR        keySealed )
                {
                    IJournalSegment::PfnSealed pfnSealed = Sealed_;
                    Unused( pfnSealed );

                    CSegment* const psegment = (CSegment*)keySealed;
                    psegment->Sealed( err );
                }

                void Sealed( _In_ const ERR err );

            private:

                typename CCountedInvasiveList<CSegment, OffsetOfILE>::CElement  m_ile;
                TJournal<I>* const                                              m_pj;
                IJournalSegment* const                                          m_pjs;
                const SegmentPosition                                           m_spos;
                DWORD                                                           m_cbEntryAppended;
                BOOL                                                            m_fSealed;
                CManualResetSignal                                              m_sigSeal;
                ERR                                                             m_errSeal;
        };

        //  Context used to visit journal entries.

        class CEntryVisitor
        {
            public:

                CEntryVisitor(  _In_ IJournalSegmentManager* const  pjsm,
                                _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                _In_ const DWORD_PTR                keyVisitEntry )
                    :   m_pjsm( pjsm ),
                        m_pfnVisitEntry( pfnVisitEntry ),
                        m_keyVisitEntry( keyVisitEntry ),
                        m_err( JET_errSuccess ),
                        m_jpos( jposInvalid ),
                        m_cbEntry( 0 ),
                        m_ibEntry( 0 ),
                        m_cbEntryRem( 0 ),
                        m_jb( 0, NULL )
                {
                }

                ~CEntryVisitor()
                {
                    delete[] m_jb.Rgb();
                }

                ERR ErrVisitEntries();

            private:

                static BOOL FVisitSegment_( _In_ const SegmentPosition  spos,
                                            _In_ const ERR              err,
                                            _In_ IJournalSegment* const pjs,
                                            _In_ const DWORD_PTR        keyVisitSegment )
                {
                    IJournalSegmentManager::PfnVisitSegment pfnVisitSegment = FVisitSegment_;
                    Unused( pfnVisitSegment );

                    CEntryVisitor* const pev = (CEntryVisitor*)keyVisitSegment;
                    return pev->FVisitSegment( spos, err, pjs );
                }

                BOOL FVisitSegment( _In_ const SegmentPosition  spos,
                                    _In_ const ERR              errValidation,
                                    _In_ IJournalSegment* const pjs );

                static BOOL FVisitRegion_(  _In_ const RegionPosition   rpos,
                                            _In_ const CJournalBuffer   jb,
                                            _In_ const DWORD_PTR        keyVisitRegion )
                {
                    IJournalSegment::PfnVisitRegion pfnVisitRegion = FVisitRegion_;
                    Unused( pfnVisitRegion );

                    CEntryVisitor* const pev = (CEntryVisitor*)keyVisitRegion;
                    return pev->FVisitRegion( rpos, jb );
                }

                BOOL FVisitRegion(  _In_ const RegionPosition   rpos,
                                    _In_ const CJournalBuffer   jb );


            private:

                IJournalSegmentManager* const   m_pjsm;
                const IJournal::PfnVisitEntry   m_pfnVisitEntry;
                const DWORD_PTR                 m_keyVisitEntry;
                ERR                             m_err;
                JournalPosition                 m_jpos;
                DWORD                           m_cbEntry;
                DWORD                           m_ibEntry;
                DWORD                           m_cbEntryRem;
                CJournalBuffer                  m_jb;
        };

        //  Wait context.

        class CWaiter
        {
            public:

                CWaiter( _In_ const SegmentPosition spos )
                    :   m_spos( spos ),
                        m_sig( CSyncBasicInfo( "TJournal<I>::CWaiter::m_sig" ) )
                {
                }

                SegmentPosition Spos() const { return m_spos; }

                BOOL FComplete() const { return m_sig.FIsSet(); }

                void Complete()
                {
                    m_sig.Set();
                }

                void Wait()
                {
                    m_sig.Wait();
                }

                static SIZE_T OffsetOfILE() { return OffsetOf( CWaiter, m_ile ); }

            private:

                typename CInvasiveList<CWaiter, OffsetOfILE>::CElement  m_ile;
                const SegmentPosition                                   m_spos;
                CManualResetSignal                                      m_sig;
        };

    private:

        void WaitForAppend( _In_ const QWORD cbEntry, _In_ CWaiter* const pwaiterForAppend );
        void PrepareToWaitForAppend( _In_ const QWORD cbEntry, _In_ CWaiter* const pwaiterForAppend );
        void ReleaseAppender(   _In_    const QWORD    cbEntry,
                                _In_    CWaiter* const  pwaiterForAppend,
                                _Out_   CWaiter** const ppwaiterNextAppender,
                                _Out_   CWaiter** const ppwaiterNextSealer );

        void PrepareToWaitForSeal( _In_ CWaiter* const pwaiterToSeal, _In_ CWaiter* const pwaiterForSeal );
        void WaitForSeal( _In_ CWaiter* const pwaiterToSeal, _In_ CWaiter* const pwaiterForSeal );
        void SealSegments( _In_ const SegmentPosition spos );
        void ReleaseSealer( _In_ CWaiter* const pwaiterToSeal );
        void PrepareToReleaseNextSealer( _Out_ CWaiter** const ppwaiterNextSealer );

        void PrepareToWaitForFlush( _In_ CWaiter* const pwaiterToFlush, _In_ CWaiter* const pwaiterForFlush );
        void WaitForFlush( _In_ CWaiter* const pwaiterToFlush, _In_ CWaiter* const pwaiterForFlush );
        void FlushSealedSegments( _In_ const SegmentPosition spos );
        void ReleaseFlusher( _In_ CWaiter* const pwaiterToFlush );
        void PrepareToReleaseNextFlusher( _Out_ CWaiter** const ppwaiterNextFlusher );

        ERR ErrGetAppendSegment( _In_ BOOL fForceNewSegment, _Out_ CSegment** const ppsegment );

        void Sealed( _In_ const ERR err, _In_ CSegment* const psegment );

        void ReleaseSegments( _In_ CInvasiveList<CSegment, CSegment::OffsetOfILE>* const pil );

    private:

        enum { cbSegment = sizeof( CJournalSegmentHeader ) };

        IJournalSegmentManager* const                   m_pjsm;
        const size_t                                    m_cbCache;
        const int                                       m_pctCacheAvailWriteBack;
        CCriticalSection                                m_crit;
        ERR                                             m_err;
        SegmentPosition                                 m_sposReplay;
        SegmentPosition                                 m_sposDurableForWriteback;
        SegmentPosition                                 m_sposDurable;
        SegmentPosition                                 m_sposSealed;
        SegmentPosition                                 m_sposAppend;
        BOOL                                            m_fAppending;

        CInvasiveList<CSegment, CSegment::OffsetOfILE>  m_ilSegmentsToSeal;
        CInvasiveList<CSegment, CSegment::OffsetOfILE>  m_ilSegmentsSealPending;
        CInvasiveList<CSegment, CSegment::OffsetOfILE>  m_ilSegmentsSealFailed;
        CInvasiveList<CSegment, CSegment::OffsetOfILE>  m_ilSegmentsSealed;

        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>    m_ilWaitersForSeal;
        CWaiter*                                        m_pwaiterSealer;
        SegmentPosition                                 m_sposSealPending;
        CWaiter*                                        m_pwaiterNextSealer;

        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>    m_ilWaitersForAppend;
        CWaiter*                                        m_pwaiterAppender;

        CInvasiveList<CWaiter, CWaiter::OffsetOfILE>    m_ilWaitersForFlush;
        CWaiter*                                        m_pwaiterFlusher;
        SegmentPosition                                 m_sposDurablePending;
        CWaiter*                                        m_pwaiterNextFlusher;
};

template< class I  >
INLINE TJournal<I>::~TJournal()
{
    ReleaseSegments( &m_ilSegmentsToSeal );
    ReleaseSegments( &m_ilSegmentsSealPending );
    ReleaseSegments( &m_ilSegmentsSealFailed );
    ReleaseSegments( &m_ilSegmentsSealed );

    delete m_pjsm;
}

template< class I  >
INLINE ERR TJournal<I>::ErrInit()
{
    ERR             err         = JET_errSuccess;
    SegmentPosition sposReplay  = sposInvalid;
    SegmentPosition sposDurable = sposInvalid;
    SegmentPosition sposLast    = sposInvalid;

    //  get the current replay and durable pointer from the segment manager

    Call( m_pjsm->ErrGetProperties( &sposReplay, &sposDurable, &sposLast ) );

    //  setup the journal to append after the last segment

    m_sposReplay = sposReplay;
    m_sposDurableForWriteback = sposDurable;
    m_sposDurable = sposDurable;
    m_sposSealed = sposLast;
    m_sposAppend = sposLast;

HandleError:
    return err;
}

template< class I  >
INLINE ERR TJournal<I>::ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                            _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                            _Out_opt_ JournalPosition* const    pjposDurable )
{
    //  protect our state

    m_crit.Enter();

    if ( pjposReplay )
    {
        *pjposReplay = (JournalPosition)m_sposReplay;
    }

    if ( pjposDurableForWriteBack )
    {
        //  we hold back write backs by an additional flush of the journal to guarantee that we have a persisted
        //  indication that they are durable.  this ensures that a corrupted segment found during journal recovery will
        //  not be misinterpreted as a torn write rather than a corruption.  if we don't guarantee this then in this
        //  double failure case (torn write + corruption in previous flush range) then the recovered journal may be
        //  behind the state it is protecting.  see TJournalSegmentManager<I>::ErrFindLastSegmentWithBinarySearch

        *pjposDurableForWriteBack = (JournalPosition)( m_sposDurableForWriteback + cbSegment - 1 );
    }

    if ( pjposDurable )
    {
        *pjposDurable = (JournalPosition)( m_sposDurable + cbSegment - 1 );
    }

    m_crit.Leave();

    return JET_errSuccess;
}

template< class I  >
INLINE ERR TJournal<I>::ErrVisitEntries(    _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                            _In_ const DWORD_PTR                keyVisitEntry )
{
    ERR             err = JET_errSuccess;
    CEntryVisitor   ev( m_pjsm, pfnVisitEntry, keyVisitEntry );

    //  visit every segment in the journal

    Call( ev.ErrVisitEntries() );

HandleError:
    return err;
}

template< class I  >
INLINE ERR TJournal<I>::ErrRepair(  _In_    const JournalPosition   jposInvalidate,
                                    _Out_   JournalPosition* const  pjposInvalidated )
{
    ERR             err             = JET_errSuccess;
    BOOL            fLocked         = fFalse;
    SegmentPosition sposInvalidate  = (SegmentPosition)( rounddn( (QWORD)jposInvalidate, cbSegment ) );

    *pjposInvalidated = jposInvalid;

    //  protect our state

    m_crit.Enter();
    fLocked = fTrue;

    //  validate the invalidate position

    if ( jposInvalidate == jposInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposInvalidate <= m_sposDurable )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposInvalidate > m_sposAppend + cbSegment )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  repair is not allowed if we have already started appending

    if ( m_fAppending )
    {
        Error( ErrERRCheck( JET_errIllegalOperation ) );
    }

    //  configure the journal to start appending at the invalidated segment.  the repair will not actually happen until
    //  the first newly appended segment is sealed

    m_sposSealed = sposInvalidate - cbSegment;
    m_sposAppend = m_sposSealed;

    //  return the actual invalidation position

    *pjposInvalidated = (JournalPosition)sposInvalidate;

HandleError:
    if ( fLocked )
    {
        m_crit.Leave();
    }
    if ( err < JET_errSuccess )
    {
        *pjposInvalidated = jposInvalid;
    }
    return err;
}

#pragma warning (push)
#pragma warning (disable: 4815)  //  zero-sized array in stack object will have no elements (unless the object is an aggregate that has been aggregate initialized)

template< class I  >
INLINE ERR TJournal<I>::ErrAppendEntry( _In_                const size_t            cjb,
                                        _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                        _Out_               JournalPosition* const  pjpos,
                                        _Out_               JournalPosition* const  pjposEnd )
{
    ERR                     err                 = JET_errSuccess;
    const size_t            cjbT                = cjb + 1;
    CJournalBuffer* const   rgjbT               = (CJournalBuffer*)_alloca( cjbT * sizeof( CJournalBuffer ) );
    CJournalEntryFragment   jef                 = CJournalEntryFragment( fTrue, 0 );
    CJournalBuffer          jb                  = CJournalBuffer( sizeof( jef ), (const BYTE*)&jef );
    QWORD                   cbEntry             = 0;
    BOOL                    fLocked             = fFalse;
    CWaiter                 waiterForAppend( sposInvalid );
    CSegment*               psegment            = NULL;
    DWORD                   ibEntry             = 0;
    DWORD                   cbEntryAppended     = dwMax;
    BOOL                    fForceNewSegment    = fFalse;
    RegionPosition          rpos                = rposInvalid;
    DWORD                   cbFragment          = 0;
    JournalPosition         jpos                = jposInvalid;
    DWORD                   cbEntryAppendedRem  = 0;
    CWaiter*                pwaiterNextAppender = NULL;
    CWaiter*                pwaiterNextSealer   = NULL;

    *pjpos = jposInvalid;
    *pjposEnd = jposInvalid;

    //  setup the buffer we will use to append regions

    rgjbT[ 0 ] = jb;

    for ( size_t ijbT = 1; ijbT < cjbT; ijbT++ )
    {
        rgjbT[ ijbT ] = rgjb[ ijbT - 1 ];
        cbEntry += rgjbT[ ijbT ].Cb();
    }

    //  reject entries that are too large

    if ( cbEntry >= CJournalEntryFragment::cbEntryMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  protect our state

    m_crit.Enter();
    fLocked = fTrue;

    //  wait for our turn and enough cache to append

    WaitForAppend( cbEntry, &waiterForAppend );

    //  if the journal is down then reject updates

    Call( m_err );

    //  append the entry as fragments

    for ( ibEntry = 0; ibEntry < cbEntry || jpos == jposInvalid; ibEntry += cbEntryAppended )
    {
        //  ensure we have an appendable segment

        Call( ErrGetAppendSegment( fForceNewSegment, &psegment ) );

        //  setup the journal entry fragment header for this fragment

        jef = CJournalEntryFragment( ibEntry == 0, cbEntry - ibEntry );

        //  append this fragment but only if the entire journal entry fragment header fits

        Call( psegment->ErrAppendRegion( cjbT, rgjbT, sizeof( jef ), &rpos, &cbFragment ) );

        cbEntryAppended = cbFragment == 0 ? 0 : cbFragment - sizeof( jef );

        //  remove what we appended from our buffers

        cbEntryAppendedRem = cbEntryAppended;

        for ( size_t ijbT = 1; ijbT < cjbT && cbEntryAppendedRem; ijbT++ )
        {
            const size_t cb = rgjbT[ ijbT ].Cb() < cbEntryAppendedRem ? rgjbT[ ijbT ].Cb() : cbEntryAppendedRem;
            cbEntryAppendedRem -= cb;

            rgjbT[ ijbT ] = CJournalBuffer( rgjbT[ ijbT ].Cb() - cb, rgjbT[ ijbT ].Rgb() + cb );
        }

        //  remember the journal position of the first fragment

        jpos = ibEntry == 0 ? (JournalPosition)rpos : jpos;

        //  we will need a new append segment next time if we couldn't append the entire payload

        fForceNewSegment = cbEntryAppended < cbEntry - ibEntry;
    }

    //  return the journal position of the entry that was appended

    *pjpos = jpos;
    *pjposEnd = (JournalPosition)( rpos + cbEntryAppended - 1 );

HandleError:
    if ( fLocked )
    {
        ReleaseAppender( cbEntry, &waiterForAppend, &pwaiterNextAppender, &pwaiterNextSealer );
        m_crit.Leave();
    }
    if ( pwaiterNextAppender )
    {
        pwaiterNextAppender->Complete();
    }
    if ( pwaiterNextSealer )
    {
        pwaiterNextSealer->Complete();
    }
    if ( err < JET_errSuccess )
    {
        *pjpos = jposInvalid;
        *pjposEnd = jposInvalid;
    }
    return err;
}

#pragma warning (pop)

template< class I  >
INLINE ERR TJournal<I>::ErrFlush()
{
    ERR     err     = JET_errSuccess;
    BOOL    fLocked = fFalse;

    //  protect our state

    m_crit.Enter();
    fLocked = fTrue;

    //  compute the segment that must be sealed and flushed

    const SegmentPosition spos = m_sposAppend;

    //  prepare to wait for that segment to be sealed

    CWaiter waiterToSeal( spos );
    CWaiter waiterForSeal( spos );
    PrepareToWaitForSeal( &waiterToSeal, &waiterForSeal );

    //  prepare to wait for flush

    CWaiter waiterToFlush( spos );
    CWaiter waiterForFlush( spos );
    PrepareToWaitForFlush( &waiterToFlush , &waiterForFlush );

    //  wait for the segment to be sealed

    WaitForSeal( &waiterToSeal, &waiterForSeal );

    //  wait for the segment to be flushed

    WaitForFlush( &waiterToFlush, &waiterForFlush );

    //  if our segment is not durable then indicate failure

    if ( m_sposDurable < spos )
    {
        Call( m_err >= JET_errSuccess ? ErrERRCheck( JET_errInternalError ) : m_err );
    }

HandleError:
    if ( fLocked )
    {
        m_crit.Leave();
    }
    return err;
}

template< class I  >
INLINE ERR TJournal<I>::ErrTruncate( _In_ const JournalPosition jposReplay )
{
    ERR             err         = JET_errSuccess;
    BOOL            fLocked     = fFalse;
    SegmentPosition sposReplay = (SegmentPosition)( rounddn( (QWORD)jposReplay, cbSegment ) );

    //  protect our state

    m_crit.Enter();
    fLocked = fTrue;

    //  validate the replay position

    if ( jposReplay == jposInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposReplay < m_sposReplay )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposReplay > m_sposDurable )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  update the replay position.  we will use this in future appends to actually truncate the journal

    m_sposReplay = sposReplay;

HandleError:
    if ( fLocked )
    {
        m_crit.Leave();
    }
    return err;
}

template< class I  >
INLINE TJournal<I>::TJournal( _Inout_ IJournalSegmentManager** const  ppjsm, _In_ const size_t cbCache )
    :   m_pjsm( *ppjsm ),
        m_cbCache( max( cbSegment, cbCache ) ),
        m_pctCacheAvailWriteBack( 50 ),
        m_crit( CLockBasicInfo( CSyncBasicInfo( "TJournal<I>::m_crit" ), rankJournalAppend, 0 ) ),
        m_err( JET_errSuccess ),
        m_sposReplay( sposInvalid ),
        m_sposDurableForWriteback( sposInvalid ),
        m_sposDurable( sposInvalid ),
        m_sposSealed( sposInvalid ),
        m_sposAppend( sposInvalid ),
        m_fAppending( fFalse ),
        m_pwaiterSealer( NULL ),
        m_sposSealPending( sposInvalid ),
        m_pwaiterNextSealer( NULL ),
        m_pwaiterAppender( NULL ),
        m_pwaiterFlusher( NULL ),
        m_sposDurablePending( sposInvalid ),
        m_pwaiterNextFlusher( NULL )
{
    *ppjsm = NULL;
}

template< class I  >
INLINE ERR TJournal<I>::CSegment::ErrAppendRegion(  _In_                const size_t            cjb,
                                                    _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                                    _In_                const DWORD             cbMin,
                                                    _Out_               RegionPosition* const   prpos,
                                                    _Out_               DWORD* const            pcbActual )
{
    //  perform the append

    ERR err = m_pjs->ErrAppendRegion( cjb, rgjb, cbMin, prpos, pcbActual );

    //  track how much entry payload was appended

    m_cbEntryAppended += *pcbActual - cbMin;

    return err;
}

template< class I  >
INLINE ERR TJournal<I>::CSegment::ErrBeginSeal()
{
    ERR err = JET_errSuccess;

    //  if we already failed to seal this segment then don't try again

    if ( AtomicCompareExchange( (long*)&m_fSealed, fFalse, fTrue ) )
    {
        Error( m_errSeal );
    }

    //  seal the segment

    m_sigSeal.Reset();
    err = m_pjs->ErrSeal( Sealed_, (DWORD_PTR)this );

    //  if the request to seal failed then fire the callback with that error

    if ( err < JET_errSuccess )
    {
        Sealed( err );
    }
    Call( err );

HandleError:
    return err;
}

template< class I  >
INLINE void TJournal<I>::CSegment::Sealed( _In_ const ERR err )
{
    m_errSeal = m_errSeal >= JET_errSuccess ? err : m_errSeal;

    m_pj->Sealed( m_errSeal, this );

    m_sigSeal.Set();
}

template< class I  >
INLINE ERR TJournal<I>::CEntryVisitor::ErrVisitEntries()
{
    ERR err = JET_errSuccess;

    //  visit all visible segments in the journal

    Call( m_pjsm->ErrVisitSegments( FVisitSegment_, (DWORD_PTR)this ) );

HandleError:
    m_err = m_err >= JET_errSuccess ? err : m_err;
    return m_err;
}

template< class I  >
INLINE BOOL TJournal<I>::CEntryVisitor::FVisitSegment(  _In_ const SegmentPosition  spos,
                                                        _In_ const ERR              errValidation,
                                                        _In_ IJournalSegment* const pjs )
{
    ERR     err         = JET_errSuccess;
    BOOL    fContinue   = fFalse;

    //  if we see a validation error on a segment that doesn't come after the durable segment then fail

    if ( errValidation < JET_errSuccess )
    {
        SegmentPosition sposDurable = sposInvalid;
        Call( m_pjsm->ErrGetProperties( NULL, &sposDurable, NULL ) );

        if ( spos <= sposDurable )
        {
            Call( errValidation );
        }
    }

    //  if we see a validation error then we are at the end of the journal

    if ( errValidation < JET_errSuccess )
    {
        Error( JET_errSuccess );
    }

    //  visit all the regions in this segment

    Call( pjs->ErrVisitRegions( FVisitRegion_, (DWORD_PTR)this ) );

    //  only continue if we visited this segment successfully

    fContinue = fTrue;

HandleError:
    m_err = m_err >= JET_errSuccess ? err : m_err;
    return fContinue;
}

template< class I  >
INLINE BOOL TJournal<I>::CEntryVisitor::FVisitRegion(   _In_ const RegionPosition   rpos,
                                                        _In_ const CJournalBuffer   jb )
{
    ERR                             err         = JET_errSuccess;
    CJournalEntryFragment* const    pef         = (CJournalEntryFragment*)jb.Rgb();
    DWORD                           cbFragment  = 0;
    BYTE*                           rgb         = NULL;

    //  if this doesn't look like a valid fragment then fail

    if ( jb.Cb() < sizeof( *pef ) )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    //  if this is the first fragment of a journal entry then remember its region position and size

    if ( pef->FFirstFragment() )
    {
        m_jpos = (JournalPosition)rpos;
        m_cbEntry = pef->CbEntryRemaining();
        m_ibEntry = 0;
        m_cbEntryRem = pef->CbEntryRemaining();
    }

    //  if this is a continuation fragment and we haven't seen the start of a journal entry yet then ignore this region

    if ( !pef->FFirstFragment() && m_jpos == jposInvalid )
    {
        Error( JET_errSuccess );
    }

    //  if the remaining size of the entry doesn't match then fail

    if ( pef->CbEntryRemaining() != m_cbEntryRem )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    //  compute the size of the fragment and ensure we have the right amount of payload

    cbFragment = jb.Cb() - sizeof( *pef );
    if ( cbFragment > m_cbEntryRem )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    //  ensure we have a buffer large enough to accumulate this entry

    const int cb = max( 1, m_cbEntry );
    if ( m_jb.Cb() < cb )
    {
        Alloc( rgb = new BYTE[ cb ] );

        delete[] m_jb.Rgb();
        m_jb = CJournalBuffer( cb, rgb );
        rgb = NULL;
    }

    //  accumulate this fragment of the entry

    UtilMemCpy( (VOID*)( m_jb.Rgb() + m_ibEntry ), pef->RgbFragment(), cbFragment );

    m_ibEntry += cbFragment;
    m_cbEntryRem -= cbFragment;

    //  if we have the entire fragment then visit it

    if ( m_cbEntryRem == 0 )
    {
        m_pfnVisitEntry(    m_jpos, 
                            (JournalPosition)( rpos + cbFragment - 1 ),
                            CJournalBuffer( m_cbEntry, m_jb.Rgb() ),
                            m_keyVisitEntry );
    }

HandleError:
    delete[] rgb;
    m_err = m_err >= JET_errSuccess ? err : m_err;
    return m_err >= JET_errSuccess;
}

template< class I >
void TJournal<I>::WaitForAppend( _In_ const QWORD cbEntry, _In_ CWaiter* const pwaiterForAppend )
{
    Assert( m_crit.FOwner() );

    //  prepare to wait for append

    PrepareToWaitForAppend( cbEntry, pwaiterForAppend );

    //  only leave the critical section if we have to wait to append

    if ( !pwaiterForAppend->FComplete() )
    {
        //  wait our turn

        m_crit.Leave();

        pwaiterForAppend->Wait();

        m_crit.Enter();
    }

    //  we had better be the appender

    Assert( m_pwaiterAppender == pwaiterForAppend );
    Assert( m_crit.FOwner() );

    //  compute the size of the append queue

    const size_t cbEntryAppended = m_ilSegmentsToSeal.FEmpty() ? 0 : m_ilSegmentsToSeal.NextMost()->CbEntryAppended();
    const QWORD cbEntryAppendQ = roundup( cbEntryAppended + cbEntry, cbSegment );

    //  compute the segment that should be sealed to make progress on writing our cache but is still below our maximum
    //  cache size

    const size_t cbCacheAvailWriteBack = rounddn( m_cbCache * m_pctCacheAvailWriteBack / 100, cbSegment );
    const SegmentPosition sposWriteBack = m_sposAppend + cbEntryAppendQ - cbCacheAvailWriteBack;

    //  compute the segment that must be sealed before we have enough room to append this entry in the cache

    const size_t cbCache = rounddn( m_cbCache, cbSegment );
    const SegmentPosition sposFull = min( m_sposAppend, m_sposAppend + cbEntryAppendQ - cbCache );

    //  prepare to wait for that segment to be sealed

    CWaiter waiterToSeal( sposWriteBack );
    CWaiter waiterForSeal( sposFull );
    PrepareToWaitForSeal( &waiterToSeal, &waiterForSeal );

    //  wait for the segment to be sealed

    WaitForSeal( &waiterToSeal, &waiterForSeal );
}

template< class I >
void TJournal<I>::PrepareToWaitForAppend( _In_ const QWORD cbEntry, _In_ CWaiter* const pwaiterForAppend )
{
    Assert( m_crit.FOwner() );

    //  if there is no appender then we are the next appender

    if ( !m_pwaiterAppender )
    {
        m_pwaiterAppender = pwaiterForAppend;
        pwaiterForAppend->Complete();
    }

    //  if someone else is waiting for append then put ourself in the queue

    else
    {
        m_ilWaitersForAppend.InsertAsNextMost( pwaiterForAppend );
    }
}

template< class I >
void TJournal<I>::ReleaseAppender(  _In_    const QWORD    cbEntry,
                                    _In_    CWaiter* const  pwaiterForAppend,
                                    _Out_   CWaiter** const ppwaiterNextAppender,
                                    _Out_   CWaiter** const ppwaiterNextSealer )
{
    Assert( m_crit.FOwner() );

    *ppwaiterNextAppender = NULL;
    *ppwaiterNextSealer = NULL;

    if ( m_pwaiterAppender == pwaiterForAppend )
    {
        //  remove ourselves as the appender

        m_pwaiterAppender = NULL;

        //  if there is another waiter for append then grant them access

        if ( !m_ilWaitersForAppend.FEmpty() )
        {
            m_pwaiterAppender = m_ilWaitersForAppend.PrevMost();
            m_ilWaitersForAppend.Remove( m_pwaiterAppender );
            *ppwaiterNextAppender = m_pwaiterAppender;
        }

        //  if there is another waiter to seal then prepare to release them

        PrepareToReleaseNextSealer( ppwaiterNextSealer );
    }
}

template< class I >
void TJournal<I>::PrepareToWaitForSeal( _In_ CWaiter* const pwaiterToSeal, _In_ CWaiter* const pwaiterForSeal )
{
    Assert( m_crit.FOwner() );
    Assert( pwaiterForSeal->Spos() <= pwaiterToSeal->Spos() );

    //  add ourselves as a waiter for seal in segment order if the desired segment isn't already sealed

    if ( m_sposSealed < pwaiterForSeal->Spos() )
    {
        CWaiter* pwaiter = NULL;
        for (   pwaiter = m_ilWaitersForSeal.NextMost();
                pwaiter != NULL && pwaiter->Spos() > pwaiterForSeal->Spos();
                pwaiter = m_ilWaitersForSeal.Prev( pwaiter ) )
        {
        }

        m_ilWaitersForSeal.Insert( pwaiterForSeal, pwaiter ? m_ilWaitersForSeal.Next( pwaiter ) : NULL );
    }
    else
    {
        pwaiterForSeal->Complete();
    }

    //  if the required segments are already sealed then we don't need to do anything

    if ( m_sposSealed >= pwaiterToSeal->Spos() )
    {
        pwaiterToSeal->Complete();
    }

    //  if there is no one sealing and the segments to seal exist then we are elected as the sealer

    else if ( !m_pwaiterSealer && m_sposSealed < pwaiterToSeal->Spos() )
    {
        m_pwaiterSealer = pwaiterToSeal;
        m_sposSealPending = pwaiterToSeal->Spos();
        pwaiterToSeal->Complete();
    }

    //  if there is no sealer or they are not handling our required range then try to sign up for the next slot

    else if ( ( !m_pwaiterSealer || m_sposSealPending < pwaiterToSeal->Spos() ) && !m_pwaiterNextSealer )
    {
        m_pwaiterNextSealer = pwaiterToSeal;
    }

    //  if we got neither slot then someone has been elected to take care of our requirement

    else
    {
        pwaiterToSeal->Complete();
    }

    Assert( m_crit.FOwner() );
}

template< class I >
void TJournal<I>::WaitForSeal( _In_ CWaiter* const pwaiterToSeal, _In_ CWaiter* const pwaiterForSeal )
{
    Assert( m_crit.FOwner() );

    //  only leave the critical section if we have to wait or seal

    if ( !pwaiterToSeal->FComplete() || m_pwaiterSealer == pwaiterToSeal || !pwaiterForSeal->FComplete() )
    {
        m_crit.Leave();

        //  wait to be elected as the sealer

        pwaiterToSeal->Wait();

        //  if we are the sealer then seal

        if ( m_pwaiterSealer == pwaiterToSeal )
        {
            //  seal the segments selected for us, not just the ones we need

            SealSegments( m_sposSealPending );

            //  release our sealer

            ReleaseSealer( pwaiterToSeal );
        }

        //  wait for the seal

        pwaiterForSeal->Wait();

        m_crit.Enter();

        //  release any sealed segments

        ReleaseSegments( &m_ilSegmentsSealed );
    }

    Assert( m_crit.FOwner() );
}

template< class I >
INLINE void TJournal<I>::SealSegments( _In_ const SegmentPosition spos )
{
    SegmentPosition sposSealed      = sposInvalid;
    CSegment*       psegment        = NULL;
    CSegment*       psegmentNext    = NULL;
    ERR             errSeal         = JET_errSuccess;

    //  protect our state

    m_crit.Enter();

    //  remember the position of the youngest segment to seal

    sposSealed = min( spos, m_sposAppend );

    //  capture all the unsealed segments we are trying to seal which may include the append segment

    for ( psegment = m_ilSegmentsToSeal.PrevMost(); psegment != NULL; psegment = psegmentNext )
    {
        psegmentNext = m_ilSegmentsToSeal.Next( psegment );

        if ( psegment->Spos() <= spos )
        {
            m_ilSegmentsToSeal.Remove( psegment );
            m_ilSegmentsSealPending.InsertAsNextMost( psegment );
        }
    }

    //  don't hold the lock while we perform IO

    m_crit.Leave();

    //  seal all unsealed segments

    for ( psegment = m_ilSegmentsSealPending.PrevMost(); psegment != NULL; psegment = psegmentNext )
    {
        psegmentNext = m_ilSegmentsSealPending.Next( psegment );

        ERR errT = psegment->ErrBeginSeal();

        errSeal = errSeal < JET_errSuccess ? errSeal : errT;
    }

    // if there was a problem sealing then flag the journal as failed

    if ( errSeal < JET_errSuccess )
    {
        m_crit.Enter();
        m_err = m_err < JET_errSuccess ? m_err : errSeal;
        m_crit.Leave();
    }
}

template< class I >
void TJournal<I>::ReleaseSealer( _In_ CWaiter* const pwaiterToSeal )
{
    //  protect our state

    m_crit.Enter();

    //  we had better be the sealer

    Assert( m_pwaiterSealer == pwaiterToSeal );

    //  remove ourselves as the sealer

    m_pwaiterSealer = NULL;

    //  prepare to release the next sealer

    CWaiter* pwaiterNextSealer = NULL;
    PrepareToReleaseNextSealer( &pwaiterNextSealer );

    //  don't hold the lock while we release waiters

    m_crit.Leave();

    //  release the next sealer if any

    if ( pwaiterNextSealer )
    {
        pwaiterNextSealer->Complete();
    }
}

template< class I >
void TJournal<I>::PrepareToReleaseNextSealer( _Out_ CWaiter** const ppwaiterNextSealer )
{
    Assert( m_crit.FOwner() );

    *ppwaiterNextSealer = NULL;

    //  compute the highest segment for which anyone is waiting

    const SegmentPosition sposWaitersForSealMax =
        max(    max(    m_pwaiterSealer ? m_pwaiterSealer->Spos() : (SegmentPosition)0,
                        m_pwaiterNextSealer ? m_pwaiterNextSealer->Spos() : (SegmentPosition)0),
                m_ilWaitersForSeal.NextMost() ? m_ilWaitersForSeal.NextMost()->Spos() : (SegmentPosition)0 );

    //  we must have covered all waiters or there had better be another waiter ready

    Assert( sposWaitersForSealMax <= m_sposSealPending || m_pwaiterNextSealer );

    //  if there is another waiter for seal and all those segments exist then grant them access and tell them how much
    //  to seal

    if ( !m_pwaiterSealer && m_pwaiterNextSealer && ( m_sposAppend >= sposWaitersForSealMax || m_err < JET_errSuccess ) )
    {
        m_pwaiterSealer = m_pwaiterNextSealer;
        m_sposSealPending = sposWaitersForSealMax;
        m_pwaiterNextSealer = NULL;

        *ppwaiterNextSealer = m_pwaiterSealer;
    }
}

template< class I >
void TJournal<I>::PrepareToWaitForFlush( _In_ CWaiter* const pwaiterToFlush, _In_ CWaiter* const pwaiterForFlush )
{
    Assert( m_crit.FOwner() );
    Assert( pwaiterForFlush->Spos() <= pwaiterToFlush->Spos() );

    //  add ourselves as a waiter for flush in segment order if the desired segment isn't already flushed

    if ( m_sposDurable < pwaiterForFlush->Spos() )
    {
        CWaiter* pwaiter = NULL;
        for (   pwaiter = m_ilWaitersForFlush.NextMost();
                pwaiter != NULL && pwaiter->Spos() > pwaiterForFlush->Spos();
                pwaiter = m_ilWaitersForFlush.Prev( pwaiter ) )
        {
        }

        m_ilWaitersForFlush.Insert( pwaiterForFlush, pwaiter ? m_ilWaitersForFlush.Next( pwaiter ) : NULL );
    }
    else
    {
        pwaiterForFlush->Complete();
    }

    //  if the required segments are already durable then we don't need to do anything

    if ( m_sposDurable >= pwaiterToFlush->Spos() )
    {
        pwaiterToFlush->Complete();
    }

    //  if there is no one flushing and the segments to flush exist then we are elected as the sealer

    else if ( !m_pwaiterFlusher && m_sposDurable < pwaiterToFlush->Spos() )
    {
        m_pwaiterFlusher = pwaiterToFlush;
        m_sposDurablePending = pwaiterToFlush->Spos();
        pwaiterToFlush->Complete();
    }

    //  if there is no flusher or they are not handling our required range then try to sign up for the next slot

    else if ( ( !m_pwaiterFlusher || m_sposDurablePending < pwaiterToFlush->Spos() ) && !m_pwaiterNextFlusher )
    {
        m_pwaiterNextFlusher = pwaiterToFlush;
    }

    //  if we got neither slot then someone has been elected to take care of our requirement

    else
    {
        pwaiterToFlush->Complete();
    }

    Assert( m_crit.FOwner() );
}

template< class I >
void TJournal<I>::WaitForFlush( _In_ CWaiter* const pwaiterToFlush, _In_ CWaiter* const pwaiterForFlush )
{
    Assert( m_crit.FOwner() );

    //  only leave the critical section if we have to wait or seal

    if ( !pwaiterToFlush->FComplete() || m_pwaiterFlusher == pwaiterToFlush || !pwaiterForFlush->FComplete() )
    {
        m_crit.Leave();

        //  wait to be elected as the flusher

        pwaiterToFlush->Wait();

        //  if we are the flusher then flush

        if ( m_pwaiterFlusher == pwaiterToFlush )
        {
            //  flush all sealed segments

            FlushSealedSegments( m_sposDurablePending );

            //  release the next flusher if any

            ReleaseFlusher( pwaiterToFlush );
        }

        //  wait for the flush

        pwaiterForFlush->Wait();

        m_crit.Enter();
    }

    Assert( m_crit.FOwner() );
}

template< class I >
INLINE void TJournal<I>::FlushSealedSegments( _In_ const SegmentPosition spos )
{
    m_crit.Enter();

    //  we had better need less than what is already sealed

    Assert( spos <= m_sposSealed || m_err < JET_errSuccess );

    //  flush the sealed segments

    m_crit.Leave();

    const ERR errFlush = m_pjsm->ErrFlush();

    m_crit.Enter();

    //  note any flush error that occurred

    m_err = m_err >= JET_errSuccess ? errFlush : m_err;

    //  if the flush succeeded then mark all sealed segments as durable

    if ( errFlush >= JET_errSuccess )
    {
        m_sposDurableForWriteback = m_sposDurable;
        m_sposDurable = m_sposSealed;
    }

    //  release all eligible flush waiters even if their segment is not durable

    CWaiter* pwaiterForFlush = NULL;
    while ( ( pwaiterForFlush = m_ilWaitersForFlush.PrevMost() ) && pwaiterForFlush->Spos() <= spos )
    {
        m_ilWaitersForFlush.Remove( pwaiterForFlush );
        pwaiterForFlush->Complete();
    }

    m_crit.Leave();
}

template< class I >
void TJournal<I>::ReleaseFlusher( _In_ CWaiter* const pwaiterToFlush )
{
    m_crit.Enter();

    //  we had better be the flusher

    Assert( m_pwaiterFlusher == pwaiterToFlush );

    //  remove ourselves as the flusher

    m_pwaiterFlusher = NULL;

    //  prepare to release the next flusher

    CWaiter* pwaiterNextFlusher = NULL;
    PrepareToReleaseNextFlusher( &pwaiterNextFlusher );

    //  don't hold the lock while we release waiters

    m_crit.Leave();

    //  release the next flusher if any

    if ( pwaiterNextFlusher )
    {
        pwaiterNextFlusher->Complete();
    }
}

template< class I >
void TJournal<I>::PrepareToReleaseNextFlusher( _Out_ CWaiter** ppwaiterNextFlusher )
{
    Assert( m_crit.FOwner() );

    *ppwaiterNextFlusher = NULL;

    //  compute the highest segment for which anyone is waiting

    const SegmentPosition sposWaitersForFlushMax =
        max(    max(    m_pwaiterFlusher ? m_pwaiterFlusher->Spos() : (SegmentPosition)0,
                        m_pwaiterNextFlusher ? m_pwaiterNextFlusher->Spos() : (SegmentPosition)0 ),
                m_ilWaitersForFlush.NextMost() ? m_ilWaitersForFlush.NextMost()->Spos() : (SegmentPosition)0 );

    //  we must have covered all waiters or there had better be another waiter ready

    Assert( sposWaitersForFlushMax <= m_sposDurablePending || m_pwaiterNextFlusher );

    //  if there is another waiter for flush and all those segments exist then grant them access and tell them how much
    //  to flush

    if ( !m_pwaiterFlusher && m_pwaiterNextFlusher && ( m_sposSealed >= sposWaitersForFlushMax || m_err < JET_errSuccess ) )
    {
        m_pwaiterFlusher = m_pwaiterNextFlusher;
        m_sposDurablePending = sposWaitersForFlushMax;
        m_pwaiterNextFlusher = NULL;

        *ppwaiterNextFlusher = m_pwaiterFlusher;
    }
}

template< class I >
INLINE ERR TJournal<I>::ErrGetAppendSegment( _In_ BOOL fForceNewSegment, _Out_ CSegment** const ppsegment )
{
    ERR                 err = JET_errSuccess;
    IJournalSegment* pjs = NULL;
    CSegment* psegment = NULL;

    *ppsegment = NULL;

    Assert( m_crit.FOwner() );

    //  the append segment is the youngest segment to seal

    psegment = m_ilSegmentsToSeal.NextMost();

    //  if we don't have an unsealed segment or we are being asked to make a new append segment then do so

    if ( !psegment || fForceNewSegment )
    {
        //  create a new append segment

        const SegmentPosition sposAppend = m_sposAppend + cbSegment;
        Call( m_pjsm->ErrAppendSegment( sposAppend, m_sposReplay, m_sposDurable, &pjs ) );
        Alloc( psegment = new CSegment( this, &pjs, sposAppend ) );

        //  remember our new append position

        m_sposAppend = sposAppend;

        //  add this new segment to the unsealed segments

        m_ilSegmentsToSeal.InsertAsNextMost( psegment );

        //  we are appending

        m_fAppending = fTrue;
    }

    //  return the append segment

    *ppsegment = psegment;

HandleError:
    delete pjs;
    if ( err < JET_errSuccess )
    {
        *ppsegment = NULL;
    }
    return err;
}

template< class I  >
INLINE void TJournal<I>::Sealed( _In_ const ERR err, _In_ CSegment* const psegment )
{
    m_crit.Enter();

    //  remove the segment from the list of segments to seal

    m_ilSegmentsSealPending.Remove( psegment );

    //  update our sealed pointer even if the seal failed so that we release waiters

    m_sposSealed = max( m_sposSealed,
                        m_ilSegmentsToSeal.FEmpty() ?
                            psegment->Spos() :
                            m_ilSegmentsToSeal.PrevMost()->Spos() - cbSegment );

    //  the segment was sealed successfully

    if ( err >= JET_errSuccess )
    {
        //  put the segment into the sealed list

        m_ilSegmentsSealed.InsertAsNextMost( psegment );
    }

    //  the segment was not sealed successfully

    else
    {
        //  put the segment in the failed segment list

        m_ilSegmentsSealFailed.InsertAsNextMost( psegment );

        //  flag the journal as failed

        m_err = m_err >= JET_errSuccess ? err : m_err;
    }

    //  release all eligible seal waiters

    CWaiter* pwaiterForSeal = NULL;
    while ( ( pwaiterForSeal = m_ilWaitersForSeal.PrevMost() ) && pwaiterForSeal->Spos() <= m_sposSealed )
    {
        m_ilWaitersForSeal.Remove( pwaiterForSeal );
        pwaiterForSeal->Complete();
    }

    //  prepare to release the next flusher

    CWaiter* pwaiterNextFlusher = NULL;
    PrepareToReleaseNextFlusher( &pwaiterNextFlusher );

    //  don't hold the lock while we release waiters

    m_crit.Leave();

    //  release the next flusher if any

    if ( pwaiterNextFlusher )
    {
        pwaiterNextFlusher->Complete();
    }
}

template<class I>
inline void TJournal<I>::ReleaseSegments( CInvasiveList<CSegment, CSegment::OffsetOfILE>* const pil )
{
    while ( pil->PrevMost() )
    {
        CSegment* const psegment = pil->PrevMost();
        pil->Remove( psegment );
        delete psegment;
    }
}

//  CJournal:  concrete TJournal<IJournal>

class CJournal  //  j
    :   public TJournal<IJournal>
{
    public:  //  specialized API

        static ERR ErrMount(    _Inout_ IJournalSegmentManager** const  ppjsm,
                                _In_    const size_t                    cbCache,
                                _Out_   IJournal** const                ppj );

    private:

        CJournal( _Inout_ IJournalSegmentManager** const  ppjsm, _In_ const size_t cbCache )
            :   TJournal<IJournal>( ppjsm, cbCache )
        {
        }
};

INLINE ERR CJournal::ErrMount(  _Inout_ IJournalSegmentManager** const  ppjsm,
                                _In_    const size_t                    cbCache,
                                _Out_   IJournal** const                ppj )
{
    ERR         err = JET_errSuccess;
    CJournal*   pj  = NULL;

    *ppj = NULL;

    Alloc( pj = new CJournal( ppjsm, cbCache ) );

    Call( pj->ErrInit() );

    *ppj = pj;
    pj = NULL;

HandleError:
    delete pj;
    if ( err < JET_errSuccess )
    {
        delete *ppj;
        *ppj = NULL;
    }
    return err;
}

