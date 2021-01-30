// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


template< class I >
class TJournal
    :   public I
{
    public:

        ~TJournal();

        ERR ErrInit();

    public:

        ERR ErrGetProperties(   _Out_opt_ JournalPosition* const    pjposReplay,
                                _Out_opt_ JournalPosition* const    pjposDurableForWriteBack,
                                _Out_opt_ JournalPosition* const    pjposDurable ) override;

        ERR ErrVisitEntries(    _In_ const IJournal::PfnVisitEntry  pfnVisitEntry,
                                _In_ const DWORD_PTR                keyVisitEntry ) override;

        ERR ErrRepair( _In_ const JournalPosition jposLast ) override;

        ERR ErrAppendEntry( _In_                const size_t            cjb,
                            _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                            _Out_               JournalPosition* const  pjpos ) override;

        ERR ErrFlush() override;

        ERR ErrTruncate( _In_ const JournalPosition jposReplay ) override;

    protected:

        TJournal( _Inout_ IJournalSegmentManager** const  ppjsm, _In_ const size_t cbCache );

    private:


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
                    delete m_jb.Rgb();
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

        void WaitForAppend( _In_ const size_t cbEntry, _In_ CWaiter* const pwaiterForAppend );
        void PrepareToWaitForAppend( _In_ const size_t cbEntry, _In_ CWaiter* const pwaiterForAppend );
        void ReleaseAppender(   _In_    const size_t    cbEntry,
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


    Call( m_pjsm->ErrGetProperties( &sposReplay, &sposDurable, &sposLast ) );


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

    m_crit.Enter();

    if ( pjposReplay )
    {
        *pjposReplay = (JournalPosition)m_sposReplay;
    }

    if ( pjposDurableForWriteBack )
    {

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


    Call( ev.ErrVisitEntries() );

HandleError:
    return err;
}

template< class I  >
INLINE ERR TJournal<I>::ErrRepair( _In_ const JournalPosition jposInvalidate )
{
    ERR             err             = JET_errSuccess;
    BOOL            fLocked         = fFalse;
    SegmentPosition sposInvalidate  = (SegmentPosition)( rounddn( (QWORD)jposInvalidate, cbSegment ) );


    m_crit.Enter();
    fLocked = fTrue;


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


    if ( m_fAppending )
    {
        Error( ErrERRCheck( JET_errIllegalOperation ) );
    }


    m_sposSealed = sposInvalidate - cbSegment;
    m_sposAppend = m_sposSealed;

HandleError:
    if ( fLocked )
    {
        m_crit.Leave();
    }
    return err;
}

#pragma warning (push)
#pragma warning (disable: 4815)

template< class I  >
INLINE ERR TJournal<I>::ErrAppendEntry( _In_                const size_t            cjb,
                                        _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                        _Out_               JournalPosition* const  pjpos )
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


    rgjbT[ 0 ] = jb;

    for ( size_t ijbT = 1; ijbT < cjbT; ijbT++ )
    {
        rgjbT[ ijbT ] = rgjb[ ijbT - 1 ];
        cbEntry += rgjbT[ ijbT ].Cb();
    }


    if ( cbEntry >= CJournalEntryFragment::cbEntryMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    m_crit.Enter();
    fLocked = fTrue;


    WaitForAppend( cbEntry, &waiterForAppend );


    Call( m_err );


    for ( ibEntry = 0; ibEntry < cbEntry || jpos == jposInvalid; ibEntry += cbEntryAppended )
    {

        Call( ErrGetAppendSegment( fForceNewSegment, &psegment ) );


        jef = CJournalEntryFragment( ibEntry == 0, cbEntry - ibEntry );


        Call( psegment->ErrAppendRegion( cjbT, rgjbT, sizeof( jef ), &rpos, &cbFragment ) );

        cbEntryAppended = cbFragment == 0 ? 0 : cbFragment - sizeof( jef );


        cbEntryAppendedRem = cbEntryAppended;

        for ( size_t ijbT = 1; ijbT < cjbT && cbEntryAppendedRem; ijbT++ )
        {
            const size_t cb = rgjbT[ ijbT ].Cb() < cbEntryAppendedRem ? rgjbT[ ijbT ].Cb() : cbEntryAppendedRem;
            cbEntryAppendedRem -= cb;

            rgjbT[ ijbT ] = CJournalBuffer( rgjbT[ ijbT ].Cb() - cb, rgjbT[ ijbT ].Rgb() + cb );
        }


        jpos = ibEntry == 0 ? (JournalPosition)rpos : jpos;


        fForceNewSegment = cbEntryAppended < cbEntry - ibEntry;
    }


    *pjpos = jpos;

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
    }
    return err;
}

#pragma warning (pop)

template< class I  >
INLINE ERR TJournal<I>::ErrFlush()
{
    ERR     err     = JET_errSuccess;
    BOOL    fLocked = fFalse;


    m_crit.Enter();
    fLocked = fTrue;


    const SegmentPosition spos = m_sposAppend;


    CWaiter waiterToSeal( spos );
    CWaiter waiterForSeal( spos );
    PrepareToWaitForSeal( &waiterToSeal, &waiterForSeal );


    CWaiter waiterToFlush( spos );
    CWaiter waiterForFlush( spos );
    PrepareToWaitForFlush( &waiterToFlush , &waiterForFlush );


    WaitForSeal( &waiterToSeal, &waiterForSeal );


    WaitForFlush( &waiterToFlush, &waiterForFlush );


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


    m_crit.Enter();
    fLocked = fTrue;


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

    ERR err = m_pjs->ErrAppendRegion( cjb, rgjb, cbMin, prpos, pcbActual );


    m_cbEntryAppended += *pcbActual - cbMin;

    return err;
}

template< class I  >
INLINE ERR TJournal<I>::CSegment::ErrBeginSeal()
{
    ERR err = JET_errSuccess;


    if ( AtomicCompareExchange( (long*)&m_fSealed, fFalse, fTrue ) )
    {
        Error( m_errSeal );
    }


    m_sigSeal.Reset();
    err = m_pjs->ErrSeal( Sealed_, (DWORD_PTR)this );


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


    if ( errValidation < JET_errSuccess )
    {
        SegmentPosition sposDurable = sposInvalid;
        Call( m_pjsm->ErrGetProperties( NULL, &sposDurable, NULL ) );

        if ( spos <= sposDurable )
        {
            Call( errValidation );
        }
    }


    if ( errValidation < JET_errSuccess )
    {
        Error( JET_errSuccess );
    }


    Call( pjs->ErrVisitRegions( FVisitRegion_, (DWORD_PTR)this ) );


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


    if ( jb.Cb() < sizeof( *pef ) )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }


    if ( pef->FFirstFragment() )
    {
        m_jpos = (JournalPosition)rpos;
        m_cbEntry = pef->CbEntryRemaining();
        m_ibEntry = 0;
        m_cbEntryRem = pef->CbEntryRemaining();
    }


    if ( !pef->FFirstFragment() && m_jpos == jposInvalid )
    {
        Error( JET_errSuccess );
    }


    if ( pef->CbEntryRemaining() != m_cbEntryRem )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }


    cbFragment = jb.Cb() - sizeof( *pef );
    if ( cbFragment > m_cbEntryRem )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }


    const int cb = max( 1, m_cbEntry );
    if ( m_jb.Cb() < cb )
    {
        Alloc( rgb = new BYTE[ cb ] );

        delete m_jb.Rgb();
        m_jb = CJournalBuffer( cb, rgb );
        rgb = NULL;
    }


    UtilMemCpy( (VOID*)( m_jb.Rgb() + m_ibEntry ), pef->RgbFragment(), cbFragment );

    m_ibEntry += cbFragment;
    m_cbEntryRem -= cbFragment;


    if ( m_cbEntryRem == 0 )
    {
        m_pfnVisitEntry( m_jpos, CJournalBuffer( m_cbEntry, m_jb.Rgb() ), m_keyVisitEntry );
    }

HandleError:
    delete[] rgb;
    m_err = m_err >= JET_errSuccess ? err : m_err;
    return m_err >= JET_errSuccess;
}

template< class I >
void TJournal<I>::WaitForAppend( _In_ const size_t cbEntry, _In_ CWaiter* const pwaiterForAppend )
{
    Assert( m_crit.FOwner() );


    PrepareToWaitForAppend( cbEntry, pwaiterForAppend );


    if ( !pwaiterForAppend->FComplete() )
    {

        m_crit.Leave();

        pwaiterForAppend->Wait();

        m_crit.Enter();
    }


    Assert( m_pwaiterAppender == pwaiterForAppend );
    Assert( m_crit.FOwner() );


    const size_t cbEntryAppended = m_ilSegmentsToSeal.FEmpty() ? 0 : m_ilSegmentsToSeal.NextMost()->CbEntryAppended();
    const size_t cbEntryAppendQ = roundup( cbEntryAppended + cbEntry, cbSegment );


    const size_t cbCacheAvailWriteBack = rounddn( m_cbCache * m_pctCacheAvailWriteBack / 100, cbSegment );
    const SegmentPosition sposWriteBack = m_sposAppend + cbEntryAppendQ - cbCacheAvailWriteBack;


    const size_t cbCache = rounddn( m_cbCache, cbSegment );
    const SegmentPosition sposFull = min( m_sposAppend, m_sposAppend + cbEntryAppendQ - cbCache );


    CWaiter waiterToSeal( sposWriteBack );
    CWaiter waiterForSeal( sposFull );
    PrepareToWaitForSeal( &waiterToSeal, &waiterForSeal );


    WaitForSeal( &waiterToSeal, &waiterForSeal );
}

template< class I >
void TJournal<I>::PrepareToWaitForAppend( _In_ const size_t cbEntry, _In_ CWaiter* const pwaiterForAppend )
{
    Assert( m_crit.FOwner() );


    if ( !m_pwaiterAppender )
    {
        m_pwaiterAppender = pwaiterForAppend;
        pwaiterForAppend->Complete();
    }


    else
    {
        m_ilWaitersForAppend.InsertAsNextMost( pwaiterForAppend );
    }
}

template< class I >
void TJournal<I>::ReleaseAppender(  _In_    const size_t    cbEntry,
                                    _In_    CWaiter* const  pwaiterForAppend,
                                    _Out_   CWaiter** const ppwaiterNextAppender,
                                    _Out_   CWaiter** const ppwaiterNextSealer )
{
    Assert( m_crit.FOwner() );

    *ppwaiterNextAppender = NULL;
    *ppwaiterNextSealer = NULL;

    if ( m_pwaiterAppender == pwaiterForAppend )
    {

        m_pwaiterAppender = NULL;


        if ( !m_ilWaitersForAppend.FEmpty() )
        {
            m_pwaiterAppender = m_ilWaitersForAppend.PrevMost();
            m_ilWaitersForAppend.Remove( m_pwaiterAppender );
            *ppwaiterNextAppender = m_pwaiterAppender;
        }


        PrepareToReleaseNextSealer( ppwaiterNextSealer );
    }
}

template< class I >
void TJournal<I>::PrepareToWaitForSeal( _In_ CWaiter* const pwaiterToSeal, _In_ CWaiter* const pwaiterForSeal )
{
    Assert( m_crit.FOwner() );
    Assert( pwaiterForSeal->Spos() <= pwaiterToSeal->Spos() );


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


    if ( m_sposSealed >= pwaiterToSeal->Spos() )
    {
        pwaiterToSeal->Complete();
    }


    else if ( !m_pwaiterSealer && m_sposSealed < pwaiterToSeal->Spos() )
    {
        m_pwaiterSealer = pwaiterToSeal;
        m_sposSealPending = pwaiterToSeal->Spos();
        pwaiterToSeal->Complete();
    }


    else if ( ( !m_pwaiterSealer || m_sposSealPending < pwaiterToSeal->Spos() ) && !m_pwaiterNextSealer )
    {
        m_pwaiterNextSealer = pwaiterToSeal;
    }


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


    if ( !pwaiterToSeal->FComplete() || m_pwaiterSealer == pwaiterToSeal || !pwaiterForSeal->FComplete() )
    {
        m_crit.Leave();


        pwaiterToSeal->Wait();


        if ( m_pwaiterSealer == pwaiterToSeal )
        {

            SealSegments( m_sposSealPending );


            ReleaseSealer( pwaiterToSeal );
        }


        pwaiterForSeal->Wait();

        m_crit.Enter();


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


    m_crit.Enter();


    sposSealed = min( spos, m_sposAppend );


    for ( psegment = m_ilSegmentsToSeal.PrevMost(); psegment != NULL; psegment = psegmentNext )
    {
        psegmentNext = m_ilSegmentsToSeal.Next( psegment );

        if ( psegment->Spos() <= spos )
        {
            m_ilSegmentsToSeal.Remove( psegment );
            m_ilSegmentsSealPending.InsertAsNextMost( psegment );
        }
    }


    m_crit.Leave();


    for ( psegment = m_ilSegmentsSealPending.PrevMost(); psegment != NULL; psegment = psegmentNext )
    {
        psegmentNext = m_ilSegmentsSealPending.Next( psegment );

        ERR errT = psegment->ErrBeginSeal();

        errSeal = errSeal < JET_errSuccess ? errSeal : errT;
    }


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

    m_crit.Enter();


    Assert( m_pwaiterSealer == pwaiterToSeal );


    m_pwaiterSealer = NULL;


    CWaiter* pwaiterNextSealer = NULL;
    PrepareToReleaseNextSealer( &pwaiterNextSealer );


    m_crit.Leave();


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


    const SegmentPosition sposWaitersForSealMax =
        max(    max(    m_pwaiterSealer ? m_pwaiterSealer->Spos() : (SegmentPosition)0,
                        m_pwaiterNextSealer ? m_pwaiterNextSealer->Spos() : (SegmentPosition)0),
                m_ilWaitersForSeal.NextMost() ? m_ilWaitersForSeal.NextMost()->Spos() : (SegmentPosition)0 );


    Assert( sposWaitersForSealMax <= m_sposSealPending || m_pwaiterNextSealer );


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


    if ( m_sposDurable >= pwaiterToFlush->Spos() )
    {
        pwaiterToFlush->Complete();
    }


    else if ( !m_pwaiterFlusher && m_sposDurable < pwaiterToFlush->Spos() )
    {
        m_pwaiterFlusher = pwaiterToFlush;
        m_sposDurablePending = pwaiterToFlush->Spos();
        pwaiterToFlush->Complete();
    }


    else if ( ( !m_pwaiterFlusher || m_sposDurablePending < pwaiterToFlush->Spos() ) && !m_pwaiterNextFlusher )
    {
        m_pwaiterNextFlusher = pwaiterToFlush;
    }


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


    if ( !pwaiterToFlush->FComplete() || m_pwaiterFlusher == pwaiterToFlush || !pwaiterForFlush->FComplete() )
    {
        m_crit.Leave();


        pwaiterToFlush->Wait();


        if ( m_pwaiterFlusher == pwaiterToFlush )
        {

            FlushSealedSegments( m_sposDurablePending );


            ReleaseFlusher( pwaiterToFlush );
        }


        pwaiterForFlush->Wait();

        m_crit.Enter();
    }

    Assert( m_crit.FOwner() );
}

template< class I >
INLINE void TJournal<I>::FlushSealedSegments( _In_ const SegmentPosition spos )
{
    m_crit.Enter();


    Assert( spos <= m_sposSealed || m_err < JET_errSuccess );


    m_crit.Leave();

    const ERR errFlush = m_pjsm->ErrFlush();

    m_crit.Enter();


    m_err = m_err >= JET_errSuccess ? errFlush : m_err;


    if ( errFlush >= JET_errSuccess )
    {
        m_sposDurableForWriteback = m_sposDurable;
        m_sposDurable = m_sposSealed;
    }


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


    Assert( m_pwaiterFlusher == pwaiterToFlush );


    m_pwaiterFlusher = NULL;


    CWaiter* pwaiterNextFlusher = NULL;
    PrepareToReleaseNextFlusher( &pwaiterNextFlusher );


    m_crit.Leave();


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


    const SegmentPosition sposWaitersForFlushMax =
        max(    max(    m_pwaiterFlusher ? m_pwaiterFlusher->Spos() : (SegmentPosition)0,
                        m_pwaiterNextFlusher ? m_pwaiterNextFlusher->Spos() : (SegmentPosition)0 ),
                m_ilWaitersForFlush.NextMost() ? m_ilWaitersForFlush.NextMost()->Spos() : (SegmentPosition)0 );


    Assert( sposWaitersForFlushMax <= m_sposDurablePending || m_pwaiterNextFlusher );


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


    psegment = m_ilSegmentsToSeal.NextMost();


    if ( !psegment || fForceNewSegment )
    {

        const SegmentPosition sposAppend = m_sposAppend + cbSegment;
        Call( m_pjsm->ErrAppendSegment( sposAppend, m_sposReplay, m_sposDurable, &pjs ) );
        Alloc( psegment = new CSegment( this, &pjs, sposAppend ) );


        m_sposAppend = sposAppend;


        m_ilSegmentsToSeal.InsertAsNextMost( psegment );


        m_fAppending = fTrue;
    }


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


    m_ilSegmentsSealPending.Remove( psegment );


    m_sposSealed = max( m_sposSealed,
                        m_ilSegmentsToSeal.FEmpty() ?
                            psegment->Spos() :
                            m_ilSegmentsToSeal.PrevMost()->Spos() - cbSegment );


    if ( err >= JET_errSuccess )
    {

        m_ilSegmentsSealed.InsertAsNextMost( psegment );
    }


    else
    {

        m_ilSegmentsSealFailed.InsertAsNextMost( psegment );


        m_err = m_err >= JET_errSuccess ? err : m_err;
    }


    CWaiter* pwaiterForSeal = NULL;
    while ( ( pwaiterForSeal = m_ilWaitersForSeal.PrevMost() ) && pwaiterForSeal->Spos() <= m_sposSealed )
    {
        m_ilWaitersForSeal.Remove( pwaiterForSeal );
        pwaiterForSeal->Complete();
    }


    CWaiter* pwaiterNextFlusher = NULL;
    PrepareToReleaseNextFlusher( &pwaiterNextFlusher );


    m_crit.Leave();


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


class CJournal
    :   public TJournal<IJournal>
{
    public:

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

