// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  Journal Full Constants

const size_t cbJournalFull = 3 * cbJournalSegment;
const size_t cbJournalFullAndDurable = 2 * cbJournalSegment;


//  TJournalSegmentManager:  implementation of IJournalSegmentManager and its derivatives.
//
//  I:  IJournalSegmentManager or derivative
//
//  This class manages the set of segments that belong to a journal.  This implementation is a simple circular queue of
//  a fixed number of segments.
//
//  This class encapsulates the management of the metadata used to determine the set of segments that are currently
//  part of the visible journal.  Note that this set includes any segments near the tail of the journal that can be
//  beyond a torn write or any segments that fail validation.
//
//  When the journal is mounted, the segments are bsearched by segment position to find the start/end of the circular
//  queue.  If a corrupt segment is encountered then all segments are enumerated to determine this.
//
//  Once the end of the queue has been determined then we will use the steps outlined in TJournalSegment to discover
//  the set of segments that belong to the journal.  These can then be visited to perform recovery using the journal.

template< class I >
class TJournalSegmentManager  //  jsm
    :   public I
{
    public:  //  specialized API

        ~TJournalSegmentManager();

        ERR ErrInit();

    public:  //  IJournalSegmentManager

        ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    psposFirst,
                                _Out_opt_ SegmentPosition* const    psposReplay,
                                _Out_opt_ SegmentPosition* const    psposDurable,
                                _Out_opt_ SegmentPosition* const    psposLast,
                                _Out_opt_ SegmentPosition* const    psposFull ) override;

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

        ERR ErrBlockCacheInternalError( _In_ const char* const szTag )
        {
            return ::ErrBlockCacheInternalError( m_pff, szTag );
        }

    private:

        class CSegmentInfo
        {
            public:

                CSegmentInfo(   _In_ const SegmentPosition  sposReplay,
                                _In_ const SegmentPosition  sposDurable,
                                _In_ const SegmentPosition  spos )
                    :   m_sposReplay( sposReplay ),
                        m_sposDurable( sposDurable ),
                        m_spos( spos )
                {
                }

                SegmentPosition SposReplay() const { return m_sposReplay; }
                SegmentPosition SposDurable() const { return m_sposDurable; }
                SegmentPosition Spos() const { return m_spos; }

                static SIZE_T OffsetOfILE() { return OffsetOf( CSegmentInfo, m_ile ); }

            private:

                typename CInvasiveList< CSegmentInfo, OffsetOfILE >::CElement   m_ile;
                const SegmentPosition                                           m_sposReplay;
                const SegmentPosition                                           m_sposDurable;
                const SegmentPosition                                           m_spos;
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

    //  try to find the last segment via binary search

    Call( ErrFindLastSegmentWithBinarySearch( &ibLast, &pjsLast, &fEmpty ) );

    //  if we could not find it (due to a corrupt block) then fall back to linear search

    if ( !pjsLast && !fEmpty )
    {
        Call( ErrFindLastSegmentWithLinearSearch( &ibLast, &pjsLast ) );
    }

    //  if we still could not find it then there are no valid segments so setup an empty journal

    if ( !pjsLast )
    {
        Call( ErrInitEmptyJournal() );
    }

    //  if we found a last segment then setup the existing journal

    if ( pjsLast )
    {
        Call( ErrInitExistingJournal( ibLast, pjsLast ) );
    }

HandleError:
    delete pjsLast;
    return err;
}

template< class I  >
INLINE ERR TJournalSegmentManager<I>::ErrGetProperties( _Out_opt_ SegmentPosition* const    psposFirst,
                                                        _Out_opt_ SegmentPosition* const    psposReplay,
                                                        _Out_opt_ SegmentPosition* const    psposDurable,
                                                        _Out_opt_ SegmentPosition* const    psposLast,
                                                        _Out_opt_ SegmentPosition* const    psposFull )
{
    if ( psposFirst )
    {
        *psposFirst = m_sposFirst;
    }

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

    if ( psposFull )
    {
        *psposFull = m_sposFirst + m_cb;
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
    DWORD               dwUniqueId              = 0;
    DWORD               dwUniqueIdPrevActual    = 0;
    DWORD               dwUniqueIdPrevExpected  = 0;

    for ( spos = m_sposFirst; spos <= m_sposLast; spos += cbJournalSegment )
    {
        //  release any previously loaded segment

        delete pjs;
        pjs = NULL;

        //  compute the segment offset

        Call( ErrGetSegmentOffset( spos, &ib ) );

        //  try to load the new segment at this offset

        Call( ErrLoadSegmentForRecovery( ib, &pjs, &errSegment, &sposActual ) );

        //  validate the segment at this offset

        if ( pjs )
        {
            //  validate the segment id.  if this is wrong then there was a lost write

            if ( sposActual != spos )
            {
                errSegment = ErrERRCheck( JET_errReadVerifyFailure );
            }

            //  validate the unique id.  if this is wrong then this segment is beyond the end of the journal

            Call( pjs->ErrGetProperties( NULL, &dwUniqueId, &dwUniqueIdPrevActual, NULL, NULL, NULL ) );

            if ( spos > m_sposFirst && dwUniqueIdPrevActual != dwUniqueIdPrevExpected )
            {
                errSegment = ErrERRCheck( JET_errReadVerifyFailure );
            }

            dwUniqueIdPrevExpected = dwUniqueId;
        }

        //  ensure that if there are any validation errors that we do not provide the segment

        if ( errSegment < JET_errSuccess )
        {
            delete pjs;
            pjs = NULL;
        }

        //  visit the segment

        if ( !pfnVisitSegment( spos, errSegment, pjs, keyVisitSegment ) )
        {
            break;
        }
    }

    //  do not fail if we enumerated all the segments

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
    QWORD                   ibDurable       = 0;
    IJournalSegment*        pjsDurable      = NULL;
    CSegmentInfo*           pseginfoLE      = NULL;
    QWORD                   cbTotal         = 0;
    QWORD                   cbUsed          = 0;
    QWORD                   cbAvailable     = 0;
    const SegmentPosition   sposPrev        = spos - cbJournalSegment;
    QWORD                   ibPrev          = 0;
    IJournalSegment*        pjsPrev         = NULL;
    DWORD                   dwUniqueIdPrev  = 0;
    IJournalSegment*        pjs             = NULL;
    CSegmentInfo*           pseginfoAppend  = NULL;
    DWORD                   dwUniqueId      = 0;

    *ppjs = NULL;

    //  reject invalid parameters or a proposed segment that would violate our recovery scheme

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
    if ( spos > m_sposLast + cbJournalSegment )
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

    //  compute the offset for the segment

    Call( ErrGetSegmentOffset( spos, &ib ) );

    //  find the metadata for the proposed segment's durable segment

    for (   pseginfoDurable = m_ilSegmentInfo.PrevMost();
            pseginfoDurable != NULL && pseginfoDurable->Spos() != sposDurable;
            pseginfoDurable = m_ilSegmentInfo.Next( pseginfoDurable ) )
    {
    }

    if ( !pseginfoDurable )
    {
        SegmentPosition sposReplayT;
        SegmentPosition sposDurableT;

        Call( ErrGetSegmentOffset( sposDurable, &ibDurable ) );
        Call( ErrLoadSegment( ibDurable, &pjsDurable ) );
        Call( pjsDurable->ErrGetProperties( NULL, NULL, NULL, &sposReplayT, &sposDurableT, NULL ) );

        Alloc( pseginfoDurable = new CSegmentInfo( sposReplayT, sposDurableT, sposDurable ) );

        for (   pseginfoLE = m_ilSegmentInfo.NextMost();
                pseginfoLE != NULL && pseginfoLE->Spos() > pseginfoDurable->Spos();
                pseginfoLE = m_ilSegmentInfo.Prev( pseginfoLE ) )
        {
        }

        m_ilSegmentInfo.Insert( pseginfoDurable, pseginfoLE ? m_ilSegmentInfo.Next( pseginfoLE ) : m_ilSegmentInfo.PrevMost() );
    }

    //  determine if it is safe to add the segment w/o running out of space

    cbTotal = m_cb;
    cbUsed = m_sposLast - m_sposFirst + cbJournalSegment;
    cbAvailable = cbTotal - cbUsed;

    if ( cbAvailable <= cbJournalFull )
    {
        //  we need to advance the durable pointer to recover from journal full

        if ( sposDurable <= m_sposDurableMax )
        {
            Error( ErrERRCheck( JET_errDiskFull ) );
        }
    }

    if ( cbAvailable == cbJournalFullAndDurable )
    {
        //  we need to advance the replay pointer to recover from journal full

        if ( sposReplay <= m_sposReplayMax )
        {
            Error( ErrERRCheck( JET_errDiskFull ) );
        }
    }

    if ( cbAvailable <= 0 || cbAvailable > m_cb )
    {
        //  we failed to recover from journal full

        Error( ErrBlockCacheInternalError( "JournalFullUnrecoverable" ) );
    }

    //  find the unique id of the segment prior to the append segment

    if ( m_sposUniqueIdPrev != sposPrev )
    {
        Call( ErrGetSegmentOffset( sposPrev, &ibPrev ) );
        Call( ErrLoadSegment( ibPrev, &pjsPrev ) );
        Call( pjsPrev->ErrGetProperties( NULL, &dwUniqueIdPrev, NULL, NULL, NULL, NULL ) );

        m_sposUniqueIdPrev = sposPrev;
        m_dwUniqueIdPrev = dwUniqueIdPrev;
    }

    //  create the segment

    Call( ErrCreateSegment( ib, spos, m_dwUniqueIdPrev, sposReplay, sposDurable, &pjs ) );

    //  update our state

    Alloc( pseginfoAppend = new CSegmentInfo( sposReplay, sposDurable, spos ) );

    Call( pjs->ErrGetProperties( NULL, &dwUniqueId, NULL, NULL, NULL, NULL ) );

    if ( m_sposFirst > pseginfoDurable->SposReplay() )
    {
        Error( ErrBlockCacheInternalError( "JournalReplayPointerLTFirst" ) );
    }

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

    m_ilSegmentInfo.Insert( pseginfoAppend, pseginfoLE ? m_ilSegmentInfo.Next( pseginfoLE ) : m_ilSegmentInfo.PrevMost() );

    if ( pseginfoLE && pseginfoLE->Spos() == pseginfoAppend->Spos() )
    {
        m_ilSegmentInfo.Remove( pseginfoLE );
        delete pseginfoLE;
    }

    pseginfoAppend = NULL;

    //  return the new segment

    *ppjs = pjs;
    pjs = NULL;

HandleError:
    delete pseginfoAppend;
    delete pjs;
    delete pjsPrev;
    delete pjsDurable;
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

    //  Search for the segment with the highest segment position in our circular queue of segments.
    //
    //  Each segment can be in one of these states:  corrupt / torn write, uninitialized, lost write, or valid.
    //
    //  If we see a corrupt / torn write segment then we cannot do an optimized search.  We fallback to linear search
    //  in that case.
    //
    //  We will only see uninitialized segments on the first cycle through the segments.  If we see an uninitialized
    //  segment during the search then we will interpret this as if it is a valid old sector with a position between 0
    //  and the size of the queue.  We will then assign all new segments positions >= the size of the cache.  This will
    //  collapse the uninitialized segment and lost write segment states into one.
    //
    //  This leaves us with valid segments in a circular queue in ascending order by segment position possibly with
    //  holes showing valid segments from a previous loop through the circular queue due to uninitialized / lost write.
    //  If we have reused the segments several times then we may see a sequence of segment positions like this:
    //
    //      state  L V V V V V V U  (U = uninitialized, L = lost write, V = valid)
    //      count  1 2 2 1 1 1 1 0  (count = (segment position / segment size) / segment count)
    //      index  0 1 2 3 4 5 6 7  (index = (segment position / segment size) % segment count)
    //
    //  We want to find the valid segment with the highest segment position to recover the journal.  That is because
    //  this segment contains the pointer to the last known durable segment which in turn contains the pointer to the
    //  lowest segment needed for replay which is the first segment in the journal.  It is tempting to say that it is
    //  ok to find the last segment just before any lost write, i.e. one where spos[index] > spos[index + 1].  However
    //  if there are multiple options and we choose the lowest one then we may land on a segment that has an
    //  sposDurable that is not the highest for any valid segment.  In that case if we encounter a corruption between
    //  that sposDurable and the found segment then we may incorrectly conclude it is a torn write when in fact it is a
    //  journal corruption.  This mistake would allow us to recover the journal but we would lose previously committed
    //  changes. This can result in the journal being behind the state it is protecting which could lead to corruption.
    //
    //  However, it is not possible to distinguish this case from one where there was no subsequent write or it was
    //  entirely corrupted/lost without remembering in some other way that the previous write was durable.  The only
    //  tool we have to solve this problem is to keep the journal ahead of its state by more than one durable write.
    //  So, to avoid this hazard, state write back should only occur when the sposDurable of the last durable segment
    //  is >= the jpos of the journal entry in question.  An artificial journal entry may need to be added and flushed
    //  to allow the journal and state to become completely flushed.
    //
    //  So, now that we don't care which of the multiple valid segments we find, we can reliably find one via a binary
    //  search implemented as lower_bound where the less than check is replaced with a check that eliminates the bottom
    //  half of the queue if we think it is sorted because spos[min] < spos[mid].
    //
    //  This is essentially a binary search of an array containing a circular queue of numbers looking for the dividing
    //  line between the two sets of sorted numbers in the array.  Surprisingly this still works even though lost
    //  writes can cause the array to be unsorted at multiple points.  The only problem is you won't always land on a
    //  particular dividing line.  It can be any one of them.
    //
    //  The search is also modified slightly to reuse the segment we used as a midpoint for the new min rather than
    //  loading yet another segment.

    Call( ErrLoadSegmentForRecovery( ibMin, &pjsMin, &errSegment, &sposMin ) );
    Call( errSegment == JET_errPageNotInitialized ? JET_errSuccess : errSegment );

    while ( ibMin < ibMax )
    {
        ibMid = rounddn( ibMin + ( ibMax - ibMin ) / 2, cbJournalSegment );

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

    //  ibMax now indicates the desired segment.  compute the offset of the last segment

    ibLast = ibMax;

    //  fetch the last segment if we don't already have it

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

    //  return the last segment we found if any

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

    //  perform a linear scan of every possible segment

    for ( QWORD ib = m_ib; ib < m_ib + m_cb; ib += cbJournalSegment )
    {
        //  try to load the current segment

        Call( ErrLoadSegmentForRecovery( ib, &pjs, &errSegment, &spos ) );

        //  if we couldn't load the segment because it was corrupt then skip it

        if ( !pjs && errSegment != JET_errPageNotInitialized )
        {
            continue;
        }

        //  aggregate the segments looking for the last segment by segment position

        if ( !pjsLast || sposLast < spos )
        {
            delete pjsLast;
            pjsLast = NULL;

            ibLast = ib;
            sposLast = spos;

            pjsLast = pjs;
            pjs = NULL;
        }

        //  release this segment if it wasn't interesting

        delete pjs;
        pjs = NULL;
    }

    //  return the last segment we found if any

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
    ERR                 err = JET_errSuccess;
    IJournalSegment*    pjs = NULL;

    //  if there are no valid segments in the journal then we will setup the journal to start at the first segment.
    //  this is required to allow our binary search algorithm to work with a partially used journal because we use
    //  uninitialized segments to hint us towards the start of the array.  we must also only create segments with a
    //  segment position >= m_cb to allow us to assign segment positions below that to uninitialized segments

    m_sposFirst = (SegmentPosition)m_cb;
    m_ibLast = m_ib;
    m_sposLast = m_sposFirst;
    m_sposUniqueIdPrev = m_sposFirst;
    m_dwUniqueIdPrev = 0;
    m_sposReplayMax = m_sposFirst;
    m_sposDurableMax = m_sposFirst;

    Call( CJournalSegment::ErrCreate( m_pff, m_ibLast, m_sposFirst, 0, m_sposFirst, m_sposFirst, &pjs ) );
    Call( pjs->ErrGetProperties( NULL, &m_dwUniqueIdPrev, NULL, NULL, NULL, NULL ) );
    Call( pjs->ErrSeal( NULL, NULL ) );
    Call( ErrFlush() );

HandleError:
    delete pjs;
    return err;
}

template<class I>
INLINE ERR TJournalSegmentManager<I>::ErrInitExistingJournal(   _In_ const QWORD            ibLast,
                                                                _In_ IJournalSegment* const pjsLast )
{
    ERR                 err                     = JET_errSuccess;
    SegmentPosition     sposLast                = sposInvalid;
    SegmentPosition     sposReplay              = sposInvalid;
    SegmentPosition     sposDurableForWriteBack = sposInvalid;
    QWORD               ibDurableForWriteBack   = 0;
    IJournalSegment*    pjsDurableForWriteBack  = NULL;
    SegmentPosition     sposFirst               = sposInvalid;

    //  we were given the segment with the highest segment position.  this segment contains the segment position of the
    //  last known durable segment.  the segment containing the replay pointer in the last durable segment is the
    //  oldest segment we could possibly have an interest in
    //
    //  NOTE:  if the last durable segment cannot be accessed then the journal is corrupt and cannot mount

    Call( pjsLast->ErrGetProperties( &sposLast, NULL, NULL, &sposReplay, &sposDurableForWriteBack, NULL ) );

    Call( ErrGetSegmentOffset( ibLast, sposLast, sposDurableForWriteBack, &ibDurableForWriteBack ) );

    Call( ErrLoadSegment( ibDurableForWriteBack, &pjsDurableForWriteBack ) );

    Call( pjsDurableForWriteBack->ErrGetProperties( NULL, NULL, NULL, &sposFirst, NULL, NULL ) );

    m_sposFirst = sposFirst;
    m_ibLast = ibLast;
    m_sposLast = sposLast;
    m_sposReplayMax = sposReplay;
    m_sposDurableMax = sposDurableForWriteBack;

HandleError:
    delete pjsDurableForWriteBack;
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
        Error( ErrBlockCacheInternalError( "JournalSegmentIllegalOffset" ) );
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

    //  try to load the segment and ignore all verification errors

    errSegment = ErrLoadSegment( ib, &pjs );
    Call( ErrIgnoreVerificationErrors( errSegment ) );

    //  determine the segment position.  compute the value in case it is uninitialized

    spos = (SegmentPosition)( ib - m_ib );
    if ( pjs )
    {
        Call( pjs->ErrGetProperties( &spos, NULL, NULL, NULL, NULL, NULL ) );
    }

    //  verify that the segment we loaded should be at this offset

    if ( m_ib + ( ( (QWORD)spos / cbJournalSegment ) % ( m_cb / cbJournalSegment ) ) * cbJournalSegment != ib )
    {
        Error( ErrERRCheck( JET_errReadVerifyFailure ) );
    }

    //  return the information

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

//  CJournalSegmentManager:  concrete TJournalSegmentManager<IJournalSegmentManager>

class CJournalSegmentManager  //  jsm
    :   public TJournalSegmentManager<IJournalSegmentManager>
{
    public:  //  specialized API

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
