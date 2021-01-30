// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


template< class I >
class TJournalSegmentManager
    :   public I
{
    public:

        ~TJournalSegmentManager();

        ERR ErrInit();

    public:

        ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    psposReplay,
                                _Out_opt_ SegmentPosition* const    psposDurable,
                                _Out_opt_ SegmentPosition* const    psposLast ) override;

        ERR ErrVisitSegments(   _In_ const IJournalSegmentManager::PfnVisitSegment  pfnVisitSegment,
                                _In_ const DWORD_PTR                                keyVisitSegment ) override;

        ERR ErrAppendSegment(   _In_    const SegmentPosition   spos,
                                _In_    const SegmentPosition   sposReplay,
                                _In_    const SegmentPosition   sposDurable,
                                _Out_   IJournalSegment** const ppjs ) override;

        ERR ErrFlush() override;

    protected:

        TJournalSegmentManager( _In_    IFileFilter* const  pff,
                                _In_    const QWORD         ib,
                                _In_    const QWORD         cb );

    private:

        enum { cbSegment = sizeof( CJournalSegmentHeader ) };

        ERR ErrFindLastSegmentWithBinarySearch( _Out_ QWORD* const              pibLast,
                                                _Out_ IJournalSegment** const   ppjsLast,
                                                _Out_ BOOL* const               pfEmpty );
        ERR ErrFindLastSegmentWithLinearSearch( _Out_ QWORD* const              pibLast,
                                                _Out_ IJournalSegment** const   ppjsLast );

        ERR ErrInitEmptyJournal();

        ERR ErrInitExistingJournal( _In_ const QWORD ibLast, _In_ IJournalSegment* const pjsLast );


        ERR ErrGetSegmentOffset(    _In_    const SegmentPosition   spos,
                                    _Out_   QWORD* const            pib );
        ERR ErrGetSegmentOffset(    _In_    const QWORD             ibLast,
                                    _In_    const SegmentPosition   sposLast,
                                    _In_    const SegmentPosition   spos,
                                    _Out_   QWORD* const            pib );

        ERR ErrLoadSegment( _In_ const QWORD ib, _Out_ IJournalSegment** const ppjs );
        ERR ErrLoadSegmentForRecovery(  _In_    const QWORD             ib,
                                        _Out_   IJournalSegment** const ppjs,
                                        _Out_   ERR* const              perrSegment,
                                        _Out_   SegmentPosition* const  pspos );

        ERR ErrCreateSegment(   _In_    const QWORD             ib,
                                _In_    const SegmentPosition   spos,
                                _In_    const DWORD             dwUniqueIdPrev,
                                _In_    const SegmentPosition   sposReplay,
                                _In_    const SegmentPosition   sposDurable,
                                _Out_   IJournalSegment** const ppjs );

        ERR ErrIgnoreVerificationErrors( _In_ const ERR err );

    private:

        class CSegmentInfo
        {
            public:

                CSegmentInfo( _In_ const SegmentPosition spos, _In_ const SegmentPosition sposReplay )
                    :   m_spos( spos ),
                        m_sposReplay( sposReplay )
                {
                }

                SegmentPosition Spos() const { return m_spos; }
                SegmentPosition SposReplay() const { return m_sposReplay; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CSegmentInfo, m_ile ); }

            private:

                typename CInvasiveList< CSegmentInfo, OffsetOfILE >::CElement   m_ile;
                const SegmentPosition                                           m_spos;
                const SegmentPosition                                           m_sposReplay;
        };

    private:

        IFileFilter* const                                      m_pff;
        const QWORD                                             m_ib;
        const QWORD                                             m_cb;
        SegmentPosition                                         m_sposFirst;
        QWORD                                                   m_ibLast;
        SegmentPosition                                         m_sposLast;
        SegmentPosition                                         m_sposUniqueIdPrev;
        DWORD                                                   m_dwUniqueIdPrev;
        SegmentPosition                                         m_sposReplayMax;
        SegmentPosition                                         m_sposDurableMax;
        CInvasiveList<CSegmentInfo, CSegmentInfo::OffsetOfILE>  m_ilSegmentInfo;
};

template< class I  >
INLINE TJournalSegmentManager<I>::~TJournalSegmentManager()
{
    while ( m_ilSegmentInfo.PrevMost() )
    {
        CSegmentInfo* const pseginfoT = m_ilSegmentInfo.PrevMost();
        m_ilSegmentInfo.Remove( pseginfoT );
        delete pseginfoT;
    }
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrInit()
{
    ERR                 err     = JET_errSuccess;
    QWORD               ibLast  = 0;
    IJournalSegment*    pjsLast = NULL;
    BOOL                fEmpty  = fFalse;


    Call( ErrFindLastSegmentWithBinarySearch( &ibLast, &pjsLast, &fEmpty ) );


    if ( !pjsLast && !fEmpty )
    {
        Call( ErrFindLastSegmentWithLinearSearch( &ibLast, &pjsLast ) );
    }


    if ( !pjsLast )
    {
        Call( ErrInitEmptyJournal() );
    }


    if ( pjsLast )
    {
        Call( ErrInitExistingJournal( ibLast, pjsLast ) );
    }

HandleError:
    delete pjsLast;
    return err;
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrGetProperties( _Out_opt_ SegmentPosition* const    psposReplay,
                                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                                        _Out_opt_ SegmentPosition* const    psposLast )
{
    if ( psposReplay )
    {
        *psposReplay = m_sposReplayMax;
    }

    if ( psposDurable )
    {
        *psposDurable = m_sposDurableMax;
    }

    if ( psposLast )
    {
        *psposLast = m_sposLast;
    }

    return JET_errSuccess;
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrVisitSegments( _In_ const IJournalSegmentManager::PfnVisitSegment  pfnVisitSegment,
                                                        _In_ const DWORD_PTR                                keyVisitSegment )
{
    ERR                 err                     = JET_errSuccess;
    SegmentPosition     spos                    = sposInvalid;
    ERR                 errSegment              = JET_errSuccess;
    QWORD               ib                      = 0;
    IJournalSegment*    pjs                     = NULL;
    SegmentPosition     sposActual              = sposInvalid;
    DWORD               dwUniqueIdPrev          = 0;
    DWORD               dwUniqueIdPrevActual    = 0;
    DWORD               dwUniqueIdPrevExpected  = 0;

    for ( spos = m_sposFirst; spos <= m_sposLast; spos += cbSegment )
    {

        delete pjs;
        pjs = NULL;


        Call( ErrGetSegmentOffset( spos, &ib ) );


        Call( ErrLoadSegmentForRecovery( ib, &pjs, &errSegment, &sposActual ) );


        if ( pjs )
        {

            if ( sposActual != spos )
            {
                errSegment = ErrERRCheck( JET_errReadVerifyFailure );
            }


            Call( pjs->ErrGetProperties( NULL, &dwUniqueIdPrev, &dwUniqueIdPrevActual, NULL, NULL, NULL ) );

            if ( spos > m_sposFirst && dwUniqueIdPrevActual != dwUniqueIdPrevExpected )
            {
                errSegment = ErrERRCheck( JET_errReadVerifyFailure );
            }

            dwUniqueIdPrevExpected = dwUniqueIdPrev;
        }


        if ( errSegment < JET_errSuccess )
        {
            delete pjs;
            pjs = NULL;
        }


        if ( !pfnVisitSegment( spos, errSegment, pjs, keyVisitSegment ) )
        {
            break;
        }
    }


    err = JET_errSuccess;

HandleError:
    delete pjs;
    err = ErrIgnoreVerificationErrors( err );
    return err;
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrAppendSegment( _In_    const SegmentPosition   spos,
                                                        _In_    const SegmentPosition   sposReplay,
                                                        _In_    const SegmentPosition   sposDurable,
                                                        _Out_   IJournalSegment** const ppjs )
{
    ERR                     err             = JET_errSuccess;
    QWORD                   ib              = 0;
    CSegmentInfo*           pseginfoDurable = NULL;
    QWORD                   cbUsed          = 0;
    const SegmentPosition   sposPrev        = spos - cbSegment;
    QWORD                   ibPrev          = 0;
    IJournalSegment*        pjsPrev         = NULL;
    DWORD                   dwUniqueIdPrev  = 0;
    IJournalSegment*        pjs             = NULL;
    CSegmentInfo*           pseginfoAppend  = NULL;
    DWORD                   dwUniqueId      = 0;
    CSegmentInfo*           pseginfoLE      = NULL;

    *ppjs = NULL;


    if ( spos == sposInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( spos < (SegmentPosition)m_cb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( spos <= m_sposDurableMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( spos > m_sposLast + cbSegment )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposDurable == sposInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposDurable < m_sposDurableMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposDurable >= spos )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposReplay == sposInvalid )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposReplay < m_sposReplayMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    if ( sposReplay > sposDurable )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    Call( ErrGetSegmentOffset( spos, &ib ) );


    for (   pseginfoDurable = m_ilSegmentInfo.PrevMost();
            pseginfoDurable != NULL && pseginfoDurable->Spos() != sposDurable;
            pseginfoDurable = m_ilSegmentInfo.Next( pseginfoDurable ) )
    {
    }

    if ( !pseginfoDurable )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    cbUsed = spos - pseginfoDurable->SposReplay();
    if ( cbUsed + 2 * cbSegment >= m_cb )
    {
        if ( sposReplay <= pseginfoDurable->SposReplay() )
        {
            Error( ErrERRCheck( JET_errDiskFull ) );
        }

        if ( cbUsed + 1 * cbSegment >= m_cb )
        {
            if ( sposDurable + cbSegment <= spos )
            {
                Error( ErrERRCheck( JET_errDiskFull ) );
            }

            if ( cbUsed >= m_cb )
            {
                Error( ErrERRCheck( JET_errInternalError ) );
            }
        }
    }


    if ( m_sposUniqueIdPrev != sposPrev )
    {
        Call( ErrGetSegmentOffset( sposPrev, &ibPrev ) );
        Call( ErrLoadSegment( ibPrev, &pjsPrev ) );
        Call( pjsPrev->ErrGetProperties( NULL, &dwUniqueIdPrev, NULL, NULL, NULL, NULL ) );

        m_sposUniqueIdPrev = sposPrev;
        m_dwUniqueIdPrev = dwUniqueIdPrev;
    }


    Call( ErrCreateSegment( ib, spos, m_dwUniqueIdPrev, sposReplay, sposDurable, &pjs ) );


    Alloc( pseginfoAppend = new CSegmentInfo( spos, sposReplay ) );

    Call( pjs->ErrGetProperties( NULL, &dwUniqueId, NULL, NULL, NULL, NULL ) );

    m_sposFirst = pseginfoDurable->SposReplay();
    m_ibLast = spos > m_sposLast ? ib : m_ibLast;
    m_sposLast = max( m_sposLast, spos );
    m_sposUniqueIdPrev = spos;
    m_dwUniqueIdPrev = dwUniqueId;
    m_sposReplayMax = max( m_sposReplayMax, sposReplay );
    m_sposDurableMax = max( m_sposDurableMax, sposDurable );

    while ( m_ilSegmentInfo.PrevMost()->Spos() < sposDurable )
    {
        CSegmentInfo* const pseginfoT = m_ilSegmentInfo.PrevMost();
        m_ilSegmentInfo.Remove( pseginfoT );
        delete pseginfoT;
    }

    for (   pseginfoLE = m_ilSegmentInfo.NextMost();
            pseginfoLE != NULL && pseginfoLE->Spos() > pseginfoAppend->Spos();
            pseginfoLE = m_ilSegmentInfo.Prev( pseginfoLE ) )
    {
    }

    m_ilSegmentInfo.Insert( pseginfoAppend, pseginfoLE ? m_ilSegmentInfo.Next( pseginfoLE ) : NULL );

    if ( pseginfoLE && pseginfoLE->Spos() == pseginfoAppend->Spos() )
    {
        m_ilSegmentInfo.Remove( pseginfoLE );
        delete pseginfoLE;
    }

    pseginfoAppend = NULL;


    *ppjs = pjs;
    pjs = NULL;

HandleError:
    delete pseginfoAppend;
    delete pjs;
    delete pjsPrev;
    if ( err < JET_errSuccess )
    {
        delete *ppjs;
        *ppjs = NULL;
    }
    return err;
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrFlush()
{
    return m_pff->ErrFlushFileBuffers( iofrBlockCache );
}

template< class I  >
INLINE TJournalSegmentManager<I>::TJournalSegmentManager(   _In_    IFileFilter* const  pff,
                                                            _In_    const QWORD         ib,
                                                            _In_    const QWORD         cb )
    :   m_pff( pff ),
        m_ib( ib ),
        m_cb( cb ),
        m_sposFirst( sposInvalid ),
        m_ibLast( qwMax ),
        m_sposLast( sposInvalid ),
        m_sposUniqueIdPrev( sposInvalid ),
        m_dwUniqueIdPrev( dwMax ),
        m_sposReplayMax( sposInvalid ),
        m_sposDurableMax( sposInvalid )
{
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrFindLastSegmentWithBinarySearch(   _Out_ QWORD* const              pibLast,
                                                                            _Out_ IJournalSegment** const   ppjsLast,
                                                                            _Out_ BOOL* const               pfEmpty )
{
    ERR                 err         = JET_errSuccess;
    QWORD               ibMin       = m_ib;
    QWORD               ibMax       = m_ib + m_cb;
    IJournalSegment*    pjsMin      = NULL;
    ERR                 errSegment  = JET_errSuccess;
    SegmentPosition     sposMin     = sposInvalid;
    QWORD               ibMid       = 0;
    IJournalSegment*    pjsMid      = NULL;
    SegmentPosition     sposMid     = sposInvalid;
    QWORD               ibLast      = 0;
    IJournalSegment*    pjsLast     = NULL;

    *pibLast = 0;
    *ppjsLast = NULL;
    *pfEmpty = fFalse;


    Call( ErrLoadSegmentForRecovery( ibMin, &pjsMin, &errSegment, &sposMin ) );
    Call( errSegment == JET_errPageNotInitialized ? JET_errSuccess : errSegment );

    while ( ibMin < ibMax )
    {
        ibMid = rounddn( ibMin + ( ibMax - ibMin ) / 2, cbSegment );

        delete pjsMid;
        pjsMid = NULL;

        Call( ErrLoadSegmentForRecovery( ibMid, &pjsMid, &errSegment, &sposMid ) );
        Call( errSegment == JET_errPageNotInitialized ? JET_errSuccess : errSegment );

        if ( sposMin < sposMid )
        {
            ibMin = ibMid;

            delete pjsMin;
            pjsMin = NULL;

            sposMin = sposMid;

            pjsMin = pjsMid;
            pjsMid = NULL;
        }
        else
        {
            ibMax = ibMid;
        }
    }


    ibLast = ibMax;


    if ( ibLast == ibMin && pjsMin )
    {
        pjsLast = pjsMin;
        pjsMin = NULL;
    }
    else if ( ibLast == ibMid && pjsMid )
    {
        pjsLast = pjsMid;
        pjsMid = NULL;
    }
    else
    {
        errSegment = ErrLoadSegment( ibLast, &pjsLast );
        Call( errSegment == JET_errPageNotInitialized ? JET_errSuccess : errSegment );
    }


    *pibLast = ibLast;
    *ppjsLast = pjsLast;
    pjsLast = NULL;
    *pfEmpty = *ppjsLast == NULL;

HandleError:
    delete pjsLast;
    delete pjsMid;
    delete pjsMin;
    err = ErrIgnoreVerificationErrors( err );
    if ( err < JET_errSuccess )
    {
        *pibLast = 0;
        delete* ppjsLast;
        *ppjsLast = NULL;
        *pfEmpty = fFalse;
    }
    return err;
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrFindLastSegmentWithLinearSearch(   _Out_ QWORD* const              pibLast,
                                                                            _Out_ IJournalSegment** const   ppjsLast )
{
    ERR                 err         = JET_errSuccess;
    ERR                 errSegment  = JET_errSuccess;
    IJournalSegment*    pjs         = NULL;
    SegmentPosition     spos        = sposInvalid;
    QWORD               ibLast      = m_cb;
    IJournalSegment*    pjsLast     = NULL;
    SegmentPosition     sposLast    = (SegmentPosition)0;

    *pibLast = 0;
    *ppjsLast = NULL;


    for ( QWORD ib = m_ib; ib < m_ib + m_cb; ib += cbSegment )
    {

        Call( ErrLoadSegmentForRecovery( ib, &pjs, &errSegment, &spos ) );


        if ( !pjs && errSegment != JET_errPageNotInitialized )
        {
            continue;
        }


        if ( !pjsLast || sposLast < spos )
        {
            delete pjsLast;
            pjsLast = NULL;

            ibLast = ib;
            sposLast = spos;

            pjsLast = pjs;
            pjs = NULL;
        }


        delete pjs;
        pjs = NULL;
    }


    *pibLast = ibLast;
    *ppjsLast = pjsLast;
    pjsLast = NULL;

HandleError:
    delete pjs;
    delete pjsLast;
    err = ErrIgnoreVerificationErrors( err );
    if ( err < JET_errSuccess )
    {
        *pibLast = 0;
        delete* ppjsLast;
        *ppjsLast = NULL;
    }
    return err;
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrInitEmptyJournal()
{
    ERR                 err         = JET_errSuccess;
    CSegmentInfo*       pseginfo    = NULL;
    IJournalSegment*    pjs         = NULL;


    m_sposFirst = (SegmentPosition)m_cb;
    m_ibLast = m_ib;
    m_sposLast = m_sposFirst;
    m_sposUniqueIdPrev = m_sposFirst;
    m_dwUniqueIdPrev = 0;
    m_sposReplayMax = m_sposFirst;
    m_sposDurableMax = m_sposFirst;

    Alloc( pseginfo = new CSegmentInfo( m_sposFirst, m_sposFirst ) );
    m_ilSegmentInfo.InsertAsPrevMost( pseginfo );
    pseginfo = NULL;

    Call( CJournalSegment::ErrCreate( m_pff, m_ibLast, m_sposLast, 0, m_sposLast, m_sposLast, &pjs ) );
    Call( pjs->ErrGetProperties( NULL, &m_dwUniqueIdPrev, NULL, NULL, NULL, NULL ) );
    Call( pjs->ErrSeal( NULL, NULL ) );
    Call( ErrFlush() );

HandleError:
    delete pjs;
    delete pseginfo;
    return err;
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrInitExistingJournal(   _In_ const QWORD            ibLast,
                                                                _In_ IJournalSegment* const pjsLast )
{
    ERR                 err                     = JET_errSuccess;
    SegmentPosition     sposLast                = sposInvalid;
    SegmentPosition     sposLastDurable         = sposInvalid;
    QWORD               ibLastDurable           = 0;
    IJournalSegment*    pjsLastDurable          = NULL;
    SegmentPosition     sposFirst               = sposInvalid;
    DWORD               dwUniqueIdLastDurable   = 0;
    QWORD               ibFirst                 = 0;
    CSegmentInfo*       pseginfo                = NULL;


    Call( pjsLast->ErrGetProperties( &sposLast, NULL, NULL, NULL, &sposLastDurable, NULL ) );

    Call( ErrGetSegmentOffset( ibLast, sposLast, sposLastDurable, &ibLastDurable ) );

    Call( ErrLoadSegment( ibLastDurable, &pjsLastDurable ) );

    Call( pjsLastDurable->ErrGetProperties( NULL, &dwUniqueIdLastDurable, NULL, &sposFirst, NULL, NULL ) );

    Call( ErrGetSegmentOffset( ibLast, sposLast, sposFirst, &ibFirst ) );

    m_sposFirst = sposFirst;
    m_ibLast = ibLast;
    m_sposLast = sposLast;
    m_sposUniqueIdPrev = sposLastDurable;
    m_dwUniqueIdPrev = dwUniqueIdLastDurable;
    m_sposReplayMax = sposFirst;
    m_sposDurableMax = sposLastDurable;

    Alloc( pseginfo = new CSegmentInfo( sposLastDurable, sposFirst ) );
    m_ilSegmentInfo.InsertAsPrevMost( pseginfo );
    pseginfo = NULL;

HandleError:
    delete pseginfo;
    delete pjsLastDurable;
    return err;
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrGetSegmentOffset(  _In_    const SegmentPosition   spos,
                                                            _Out_   QWORD* const            pib )
{
    return ErrGetSegmentOffset( m_ibLast, m_sposLast, spos, pib );
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrGetSegmentOffset(  _In_    const QWORD             ibLast,
                                                            _In_    const SegmentPosition   sposLast,
                                                            _In_    const SegmentPosition   spos,
                                                            _Out_   QWORD* const            pib )
{
    ERR     err = JET_errSuccess;
    LONG64  ib  = 0;

    *pib = qwMax;

    ib = ibLast - ( sposLast - spos );
    ib = ib < (LONG64)m_ib ? ib + m_cb : ib;
    ib = ib >= (LONG64)( m_ib + m_cb ) ? ib - m_cb : ib;

    if ( ib < (LONG64)m_ib || ib >= (LONG64)( m_ib + m_cb ) )
    {
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    *pib = ib;

HandleError:
    if ( err < JET_errSuccess )
    {
        *pib = qwMax;
    }
    return err;
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrLoadSegment( _In_ const QWORD ib, _Out_ IJournalSegment** const ppjs )
{
    return CJournalSegment::ErrLoad( m_pff, ib, ppjs );
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrLoadSegmentForRecovery(    _In_    const QWORD             ib,
                                                                    _Out_   IJournalSegment** const ppjs,
                                                                    _Out_   ERR* const              perrSegment,
                                                                    _Out_   SegmentPosition* const  pspos )
{
    ERR                 err         = JET_errSuccess;
    ERR                 errSegment  = JET_errSuccess;
    IJournalSegment*    pjs         = NULL;
    SegmentPosition     spos        = sposInvalid;

    *ppjs = NULL;
    *perrSegment = JET_errSuccess;
    *pspos = sposInvalid;


    errSegment = ErrLoadSegment( ib, &pjs );
    Call( ErrIgnoreVerificationErrors( errSegment ) );


    spos = (SegmentPosition)( ib - m_ib );
    if ( pjs )
    {
        Call( pjs->ErrGetProperties( &spos, NULL, NULL, NULL, NULL, NULL ) );
    }


    *ppjs = pjs;
    pjs = NULL;
    *perrSegment = errSegment;
    *pspos = spos;

HandleError:
    if ( err < JET_errSuccess )
    {
        delete *ppjs;
        *ppjs = NULL;
        *perrSegment = JET_errSuccess;
        *pspos = sposInvalid;
    }
    return err;
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrCreateSegment( _In_    const QWORD             ib,
                                                        _In_    const SegmentPosition   spos,
                                                        _In_    const DWORD             dwUniqueIdPrev,
                                                        _In_    const SegmentPosition   sposReplay,
                                                        _In_    const SegmentPosition   sposDurable,
                                                        _Out_   IJournalSegment** const ppjs )
{
    return CJournalSegment::ErrCreate( m_pff, ib, spos, dwUniqueIdPrev, sposReplay, sposDurable, ppjs );
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrIgnoreVerificationErrors( _In_ const ERR err )
{
    switch ( err )
    {
        case JET_errReadVerifyFailure:
        case JET_errPageNotInitialized:
        case JET_errDiskReadVerificationFailure:
            return JET_errSuccess;
    }

    return err;
}


class CJournalSegmentManager
    :   public TJournalSegmentManager<IJournalSegmentManager>
{
    public:

        static ERR ErrMount(    _In_    IFileFilter* const              pff,
                                _In_    const QWORD                     ib,
                                _In_    const QWORD                     cb,
                                _Out_   IJournalSegmentManager** const  ppjsm );

    private:

        CJournalSegmentManager( _In_    IFileFilter* const  pff,
                                _In_    const QWORD         ib,
                                _In_    const QWORD         cb )
            :   TJournalSegmentManager<IJournalSegmentManager>( pff, ib, cb )
        {
        }
};

INLINE ERR CJournalSegmentManager::ErrMount(    _In_    IFileFilter* const              pff,
                                                _In_    const QWORD                     ib,
                                                _In_    const QWORD                     cb,
                                                _Out_   IJournalSegmentManager** const  ppjsm )
{
    ERR                     err     = JET_errSuccess;
    CJournalSegmentManager* pjsm    = NULL;

    *ppjsm = NULL;

    Alloc( pjsm = new CJournalSegmentManager( pff, ib, cb ) );

    Call( pjsm->ErrInit() );

    *ppjsm = pjsm;
    pjsm = NULL;

HandleError:
    delete pjsm;
    if ( err < JET_errSuccess )
    {
        delete *ppjsm;
        *ppjsm = NULL;
    }
    return err;
}
