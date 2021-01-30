// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


template < class I >
class TJournalSegment
    :   public I
{
    public:

        ~TJournalSegment();

    public:

        ERR ErrGetPhysicalId( _Out_ QWORD* const pib ) override;

        ERR ErrGetProperties(   _Out_opt_ SegmentPosition* const    pspos,
                                _Out_opt_ DWORD* const              pdwUniqueId,
                                _Out_opt_ DWORD* const              pdwUniqueIdPrev,
                                _Out_opt_ SegmentPosition* const    psposReplay,
                                _Out_opt_ SegmentPosition* const    psposDurable,
                                _Out_opt_ BOOL* const               pfSealed ) override;

        ERR ErrVisitRegions(    _In_ const IJournalSegment::PfnVisitRegion  pfnVisitRegion,
                                _In_ const DWORD_PTR                        keyVisitRegion ) override;

        ERR ErrAppendRegion(    _In_                const size_t            cjb,
                                _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                _In_                const DWORD             cbMin,
                                _Out_               RegionPosition* const   prpos,
                                _Out_               DWORD* const            pcbActual ) override;

        ERR ErrSeal( _In_opt_ IJournalSegment::PfnSealed pfnSealed, _In_ const DWORD_PTR keySealed ) override;

    protected:

        TJournalSegment(    _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _Inout_ CJournalSegmentHeader** const   ppjsh,
                            _In_    const BOOL                      fSealed );

    private:

        void IOComplete( _In_ const ERR err );

        static void IOComplete_(    _In_                    const ERR               err,
                                    _In_                    IFileAPI* const         pfapi,
                                    _In_                    const FullTraceContext& tc,
                                    _In_                    const OSFILEQOS         grbitQOS,
                                    _In_                    const QWORD             ibOffset,
                                    _In_                    const DWORD             cbData,
                                    _In_reads_( cbData )    const BYTE* const       pbData,
                                    _In_                    const DWORD_PTR         keyIOComplete );

        void IOHandoff();

        static void IOHandoff_( _In_                    const ERR               err,
                                _In_                    IFileAPI* const         pfapi,
                                _In_                    const FullTraceContext& tc,
                                _In_                    const OSFILEQOS         grbitQOS,
                                _In_                    const QWORD             ibOffset,
                                _In_                    const DWORD             cbData,
                                _In_reads_( cbData )    const BYTE* const       pbData,
                                _In_                    const DWORD_PTR         keyIOComplete,
                                _In_                    void* const             pvIOContext );

    private:

        enum { cbSegment = sizeof( CJournalSegmentHeader ) };

        IFileFilter* const              m_pff;
        const QWORD                     m_ibSegment;
        CJournalSegmentHeader* const    m_pjsh;
        CCriticalSection                m_crit;
        BOOL                            m_fSealed;
        ERR                             m_err;
        IJournalSegment::PfnSealed      m_pfnSealed;
        DWORD_PTR                       m_keySealed;
        size_t                          m_ibRegions;
        CManualResetSignal              m_sigSealed;
};

template< class I  >
INLINE TJournalSegment<I>::~TJournalSegment()
{
    m_sigSealed.Wait();
    delete m_pjsh;
}

template< class I  >
INLINE ERR TJournalSegment<I>::ErrGetPhysicalId( _Out_ QWORD* const pib )
{
    *pib = m_ibSegment;
    return JET_errSuccess;
}

template< class I  >
INLINE ERR TJournalSegment<I>::ErrGetProperties(    _Out_opt_ SegmentPosition* const    pspos,
                                                    _Out_opt_ DWORD* const              pdwUniqueId,
                                                    _Out_opt_ DWORD* const              pdwUniqueIdPrev,
                                                    _Out_opt_ SegmentPosition* const    psposReplay,
                                                    _Out_opt_ SegmentPosition* const    psposDurable,
                                                    _Out_opt_ BOOL* const               pfSealed )
{
    if ( pspos )
    {
        *pspos = m_pjsh->Spos();
    }

    if ( pdwUniqueId )
    {
        *pdwUniqueId = m_pjsh->DwUniqueId();
    }

    if ( pdwUniqueIdPrev )
    {
        *pdwUniqueIdPrev = m_pjsh->DwUniqueIdPrev();
    }

    if ( psposReplay )
    {
        *psposReplay = m_pjsh->SposReplay();
    }

    if ( psposDurable )
    {
        *psposDurable = m_pjsh->SposDurable();
    }

    if ( pfSealed )
    {
        *pfSealed = m_fSealed;
    }

    return JET_errSuccess;
}

template< class I  >
INLINE ERR TJournalSegment<I>::ErrVisitRegions( _In_ const IJournalSegment::PfnVisitRegion  pfnVisitRegion,
                                                _In_ const DWORD_PTR                        keyVisitRegion )
{

    RegionPosition rpos = m_pjsh->RposFirst();
    for (   const CJournalRegion* pjreg = m_pjsh->Pjreg( rpos );
            pjreg && pjreg->CbPayload();
            pjreg = m_pjsh->Pjreg( rpos += pjreg->CbRegion() ) )
    {
        if ( !pfnVisitRegion( rpos, pjreg->JbPayload(), keyVisitRegion ) )
        {
            break;
        }
    }

    return JET_errSuccess;
}

#pragma warning (push)
#pragma warning (disable: 4815)

template< class I  >
INLINE ERR TJournalSegment<I>::ErrAppendRegion( _In_                const size_t            cjb,
                                                _In_reads_( cjb )   CJournalBuffer* const   rgjb,
                                                _In_                const DWORD             cbMin,
                                                _Out_               RegionPosition* const   prpos,
                                                _Out_               DWORD* const            pcbActual )
{
    ERR     err                 = JET_errSuccess;
    DWORD   cbPayloadRequested  = 0;
    BOOL    fLocked             = fFalse;
    DWORD   cbRegionMin         = 0;
    DWORD   cbRegionMax         = 0;
    DWORD   ibRegion            = 0;
    DWORD   cbPayloadActual     = 0;

    *prpos = rposInvalid;
    *pcbActual = 0;


    for ( size_t ijb = 0; ijb < cjb; ijb++ )
    {
        cbPayloadRequested += rgjb[ ijb ].Cb();
    }


    if ( !cbPayloadRequested )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    m_crit.Enter();
    fLocked = fTrue;


    if ( m_fSealed )
    {
        Error( JET_errSuccess );
    }


    cbRegionMin = sizeof( CJournalRegion ) + cbMin;
    cbRegionMax = m_pjsh->CbRegions() - m_ibRegions;


    if ( cbRegionMin <= cbRegionMax )
    {

        ibRegion = m_ibRegions;
        cbPayloadActual = min( cbPayloadRequested, cbRegionMax - sizeof( CJournalRegion ) );


        const CJournalRegion jreg( cbPayloadActual );
        UtilMemCpy( m_pjsh->RgbRegions() + m_ibRegions, &jreg, sizeof( jreg ) );
        m_ibRegions += sizeof( jreg );


        DWORD cbPayloadRemaining = cbPayloadActual;
        for ( size_t ijb = 0; ijb < cjb && cbPayloadRemaining > 0; ijb++ )
        {
            const DWORD cb = min( rgjb[ ijb ].Cb(), cbPayloadRemaining );
            cbPayloadRemaining -= cb;

            UtilMemCpy( m_pjsh->RgbRegions() + m_ibRegions, rgjb[ ijb ].Rgb(), cb );
            m_ibRegions += cb;
        }
    }


    if ( cbPayloadActual > 0 )
    {
        *prpos = m_pjsh->RposFirst() + ibRegion;
        *pcbActual = cbPayloadActual;
    }

HandleError:
    if ( fLocked )
    {
        m_crit.Leave();
    }
    if ( err < JET_errSuccess )
    {
        *prpos = rposInvalid;
        *pcbActual = 0;
    }
    return err;
}

#pragma warning (pop)

template< class I  >
INLINE ERR TJournalSegment<I>::ErrSeal( _In_opt_ IJournalSegment::PfnSealed pfnSealed, _In_ const DWORD_PTR keySealed )
{
    ERR                 err     = JET_errSuccess;
    BOOL                fLocked = fFalse;
    TraceContextScope   tcScope( iorpBlockCache );
    IFileFilter* const  pff     = m_pff;


    m_crit.Enter();
    fLocked = fTrue;


    if ( m_fSealed )
    {
        if ( pfnSealed && m_err >= JET_errSuccess )
        {
            const SegmentPosition spos = m_pjsh->Spos();

            m_crit.Leave();
            fLocked = fFalse;

            pfnSealed( m_err, spos, keySealed );
        }

        Error( m_err );
    }


    memset( m_pjsh->RgbRegions() + m_ibRegions, 0, m_pjsh->CbRegions() - m_ibRegions );


    Call( m_pjsh->ErrFinalize() );


    m_fSealed = fTrue;
    m_pfnSealed = pfnSealed;
    m_keySealed = keySealed;


    m_crit.Leave();
    fLocked = fFalse;


    Call( pff->ErrIOWrite(  *tcScope,
                            m_ibSegment,
                            cbSegment,
                            (const BYTE*)m_pjsh,
                            qosIONormal, 
                            (IFileAPI::PfnIOComplete)IOComplete_, 
                            (DWORD_PTR)this,
                            (IFileAPI::PfnIOHandoff)IOHandoff_ ) );
    Call( pff->ErrIOIssue() );
    pff->SetNoFlushNeeded();


    if ( !pfnSealed )
    {
        m_sigSealed.Wait();
        Error( m_err );
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( !fLocked )
        {
            m_crit.Enter();
            fLocked = fTrue;
        }
        m_err = m_err >= JET_errSuccess ? err : m_err;
    }
    if ( fLocked )
    {
        m_crit.Leave();
    }
    return err;
}

template< class I  >
INLINE TJournalSegment<I>::TJournalSegment( _In_    IFileFilter* const              pff,
                                            _In_    const QWORD                     ib,
                                            _Inout_ CJournalSegmentHeader** const   ppjsh,
                                            _In_    const BOOL                      fSealed )
    :   m_pff( pff ),
        m_ibSegment( ib ),
        m_pjsh( *ppjsh ),
        m_crit( CLockBasicInfo( CSyncBasicInfo( "TJournalSegment<I>::m_crit" ), rankJournalSegment, 0 ) ),
        m_fSealed( fSealed ),
        m_err( JET_errSuccess ),
        m_pfnSealed( NULL ),
        m_keySealed( NULL ),
        m_ibRegions( 0 ),
        m_sigSealed( CSyncBasicInfo( "TJournalSegment<I>::m_sigSealed" ) )
{
    m_sigSealed.Set();
    *ppjsh = NULL;
}

template< class I  >
INLINE void TJournalSegment<I>::IOComplete( _In_ const ERR err )
{
    m_err = m_err >= JET_errSuccess ? err : m_err;

    if ( m_pfnSealed )
    {
        m_pfnSealed( m_err, m_pjsh->Spos(), m_keySealed );
    }

    m_sigSealed.Set();
}

template< class I  >
INLINE void TJournalSegment<I>::IOComplete_(    _In_                    const ERR               err,
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

    TJournalSegment<I>* const pjs = (TJournalSegment<I>*)keyIOComplete;
    pjs->IOComplete( err );
}

template< class I  >
INLINE void TJournalSegment<I>::IOHandoff()
{
    m_sigSealed.Reset();
}

template< class I  >
INLINE void TJournalSegment<I>::IOHandoff_( _In_                    const ERR               err,
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

    TJournalSegment<I>* const pjs = (TJournalSegment<I>*)keyIOComplete;
    pjs->IOHandoff();
}



class CJournalSegment
    :   public TJournalSegment<IJournalSegment>
{
    public:

        static ERR ErrCreate(   _In_    IFileFilter* const      pff,
                                _In_    const QWORD             ib,
                                _In_    const SegmentPosition   spos,
                                _In_    const DWORD             dwUniqueIdPrev,
                                _In_    const SegmentPosition   sposReplay,
                                _In_    const SegmentPosition   sposDurable,
                                _Out_   IJournalSegment** const ppjs );
        static ERR ErrLoad( _In_    IFileFilter* const      pff,
                            _In_    const QWORD             ib,
                            _Out_   IJournalSegment** const ppjs );

    private:

        CJournalSegment(    _In_    IFileFilter* const              pff,
                            _In_    const QWORD                     ib,
                            _Inout_ CJournalSegmentHeader** const   ppjsh,
                            _In_    const BOOL                      fSealed )
            : TJournalSegment<IJournalSegment>( pff, ib, ppjsh, fSealed )
        {
        }
};

INLINE ERR CJournalSegment::ErrCreate(  _In_    IFileFilter* const      pff,
                                        _In_    const QWORD             ib,
                                        _In_    const SegmentPosition   spos,
                                        _In_    const DWORD             dwUniqueIdPrev,
                                        _In_    const SegmentPosition   sposReplay,
                                        _In_    const SegmentPosition   sposDurable,
                                        _Out_   IJournalSegment** const ppjs )
{
    ERR                     err     = JET_errSuccess;
    CJournalSegmentHeader*  pjsh    = NULL;
    IJournalSegment*        pjs     = NULL;

    *ppjs = NULL;

    Call( CJournalSegmentHeader::ErrCreate( spos, dwUniqueIdPrev, sposReplay, sposDurable, &pjsh ) );

    Alloc( pjs = new CJournalSegment( pff, ib, &pjsh, fFalse ) );

    *ppjs = pjs;
    pjs = NULL;

HandleError:
    delete pjs;
    delete pjsh;
    if ( err < JET_errSuccess )
    {
        delete *ppjs;
        *ppjs = NULL;
    }
    return err;
}

INLINE ERR CJournalSegment::ErrLoad(    _In_    IFileFilter* const      pff,
                                        _In_    const QWORD             ib,
                                        _Out_   IJournalSegment** const ppjs )
{
    ERR                     err     = JET_errSuccess;
    CJournalSegmentHeader*  pjsh    = NULL;
    IJournalSegment*        pjs     = NULL;

    *ppjs = NULL;

    Call( CJournalSegmentHeader::ErrLoad( pff, ib, &pjsh ) );

    Alloc( pjs = new CJournalSegment( pff, ib, &pjsh, fTrue ) );

    *ppjs = pjs;
    pjs = NULL;

HandleError:
    delete pjs;
    delete pjsh;
    if ( err < JET_errSuccess )
    {
        delete *ppjs;
        *ppjs = NULL;
    }
    return err;
}
