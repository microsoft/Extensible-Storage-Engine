// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  TCachedBlockWriteCountsManager:  implementation of ICachedBlockWriteCountsManager and its derivatives.
//
//  I:  ICachedBlockWriteCountsManager or derivative
//
//  This class implements a persistent set of write counts that can be used to catch lost writes to cached block
//  chunks.  The last known write count must be provided to read a cached block chunk from storage.  If there is a
//  mismatch (due to either the write counts or the chunk experiencing a lost write) then the read will fail.
//  Whenever a cached block chunk is written back to storage then the corresponding write count is updated.  The write
//  counts are maintained during recovery and thus hold back the journal redo position.

template< class I >
class TCachedBlockWriteCountsManager  //  cbwcm
    :   public I
{
    public:  //  specialized API

        ~TCachedBlockWriteCountsManager();

        ERR ErrLoad();

    public:  //  ICachedBlockWriteCountsManager

        ERR ErrGetWriteCount( _In_ const QWORD icbwc, _Out_ CachedBlockWriteCount* const pcbwc ) override;

        ERR ErrSetWriteCount(   _In_ const QWORD                    icbwc,
                                _In_ const CachedBlockWriteCount    cbwc ) override;

        ERR ErrSave() override;

    protected:

        TCachedBlockWriteCountsManager( _In_    IFileFilter* const          pff,
                                        _In_    const QWORD                 ib,
                                        _In_    const QWORD                 cb );

    private:

        IFileFilter* const          m_pff;
        const QWORD                 m_ib;
        const QWORD                 m_cb;
        const size_t                m_ccbwcs;
        CCachedBlockWriteCounts*    m_rgcbwcs;
        CReaderWriterLock           m_rwlSave;
};

template< class I  >
INLINE TCachedBlockWriteCountsManager<I>::~TCachedBlockWriteCountsManager()
{
    OSMemoryPageFree( m_rgcbwcs );
}

template< class I  >
INLINE ERR TCachedBlockWriteCountsManager<I>::ErrLoad()
{
    ERR                         err     = JET_errSuccess;
    CCachedBlockWriteCounts*    rgcbwcs = NULL;
    TraceContextScope           tcScope( iorpBlockCache );

    if ( m_ib % sizeof( CCachedBlockWriteCounts ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( !m_cb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( m_cb % sizeof( CCachedBlockWriteCounts ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( m_cb > dwMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  cache the entire set of write counts

    Alloc( rgcbwcs = (CCachedBlockWriteCounts*)PvOSMemoryPageAlloc( m_cb, NULL ) );
    Call( m_pff->ErrIORead( *tcScope, m_ib, (DWORD)m_cb, (BYTE*)rgcbwcs, qosIONormal ) );

    //  verify each page of write counts

    for ( size_t icbwcs = 0; icbwcs < m_ccbwcs; icbwcs++ )
    {
        Call( rgcbwcs[ icbwcs ].ErrVerify( m_ib + icbwcs * sizeof( CCachedBlockWriteCounts ) ) );
    }

    //  save our verified state

    m_rgcbwcs = rgcbwcs;
    rgcbwcs = NULL;

HandleError:
    OSMemoryPageFree( rgcbwcs );
    return err;
}

template< class I  >
INLINE ERR TCachedBlockWriteCountsManager<I>::ErrGetWriteCount( _In_    const QWORD icbwc,
                                                                _Out_   CachedBlockWriteCount* const pcbwc )
{
    ERR             err     = JET_errSuccess;
    const size_t    icbwcs  = icbwc / CCachedBlockWriteCounts::Ccbwc();
    const size_t    icbwcT  = icbwc % CCachedBlockWriteCounts::Ccbwc();

    *pcbwc = cbwcNeverWritten;

    if ( icbwcs >= m_ccbwcs )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_rwlSave.EnterAsReader();

    *pcbwc = (CachedBlockWriteCount)AtomicRead( (DWORD*)m_rgcbwcs[ icbwcs ].Pcbwc( icbwcT ) );

    m_rwlSave.LeaveAsReader();

HandleError:
    if ( err < JET_errSuccess )
    {
        *pcbwc = cbwcNeverWritten;
    }
    return err;
}

template< class I  >
INLINE ERR TCachedBlockWriteCountsManager<I>::ErrSetWriteCount( _In_ const QWORD                    icbwc,
                                                                _In_ const CachedBlockWriteCount    cbwc )
{
    ERR             err     = JET_errSuccess;
    const size_t    icbwcs  = icbwc / CCachedBlockWriteCounts::Ccbwc();
    const size_t    icbwcT  = icbwc % CCachedBlockWriteCounts::Ccbwc();

    if ( icbwcs >= m_ccbwcs )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( cbwc == cbwcNeverWritten || cbwc == cbwcUnknown )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    m_rwlSave.EnterAsReader();

    AtomicExchange( (LONG*)m_rgcbwcs[ icbwcs ].Pcbwc( icbwcT ), (LONG)cbwc );

    m_rwlSave.LeaveAsReader();

HandleError:
    return err;
}

template< class I  >
INLINE ERR TCachedBlockWriteCountsManager<I>::ErrSave()
{
    ERR                         err                         = JET_errSuccess;
    CCachedBlockWriteCounts*    rgcbwcs                     = NULL;
    BOOL                        fReleaseWriter              = fFalse;
    size_t                      icbwcs                      = 0;
    TraceContextScope           tcScope( iorpBlockCache );

    //  allocate temp storage for the write counts

    Alloc( rgcbwcs = (CCachedBlockWriteCounts*)PvOSMemoryPageAlloc( m_cb, NULL ) );

    //  freeze updates to the write counts

    m_rwlSave.EnterAsWriter();
    fReleaseWriter = fTrue;

    //  copy the write counts

    UtilMemCpy( rgcbwcs, m_rgcbwcs, m_cb );

    //  allow updates to the write counts

    m_rwlSave.LeaveAsWriter();
    fReleaseWriter = fFalse;

    //  finalize the write counts

    for ( icbwcs = 0; icbwcs < m_ccbwcs; icbwcs++ )
    {
        Call( rgcbwcs[ icbwcs ].ErrFinalize() );
    }

    //  write the entire set of write counts

    Call( m_pff->ErrIOWrite( *tcScope, m_ib, (DWORD)m_cb, (const BYTE*)rgcbwcs, qosIONormal ) );

HandleError:
    if ( fReleaseWriter )
    {
        m_rwlSave.LeaveAsWriter();
    }
    OSMemoryPageFree( rgcbwcs );
    return err;
}

template< class I  >
INLINE TCachedBlockWriteCountsManager<I>::TCachedBlockWriteCountsManager(   _In_    IFileFilter* const          pff,
                                                                            _In_    const QWORD                 ib,
                                                                            _In_    const QWORD                 cb )
    :   m_pff( pff ),
        m_ib( ib ),
        m_cb( cb ),
        m_ccbwcs( cb / sizeof( CCachedBlockWriteCounts ) ),
        m_rgcbwcs( NULL ),
        m_rwlSave( CLockBasicInfo( CSyncBasicInfo( "TCachedBlockWriteCountsManager<I>::m_rwlSave" ), rankCachedBlockWriteCounts, 0 ) )
{
}

//  CCachedBlockWriteCountsManager:  concrete TCachedBlockWriteCountsManager<ICachedBlockWriteCountsManager>

class CCachedBlockWriteCountsManager  //  cbwcm
    :   public TCachedBlockWriteCountsManager<ICachedBlockWriteCountsManager>
{
    public:  //  specialized API

        static ERR ErrInit( _In_    IFileFilter* const                      pff,
                            _In_    const QWORD                             ib,
                            _In_    const QWORD                             cb,
                            _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm );

    private:


        CCachedBlockWriteCountsManager( _In_    IFileFilter* const          pff,
                                        _In_    const QWORD                 ib,
                                        _In_    const QWORD                 cb )
            : TCachedBlockWriteCountsManager<ICachedBlockWriteCountsManager>( pff, ib, cb )
        {
        }
};

INLINE ERR CCachedBlockWriteCountsManager::ErrInit( _In_    IFileFilter* const                      pff,
                                                    _In_    const QWORD                             ib,
                                                    _In_    const QWORD                             cb,
                                                    _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm )
{
    ERR                             err     = JET_errSuccess;
    CCachedBlockWriteCountsManager* pcbwcm  = NULL;

    *ppcbwcm = NULL;

    Alloc( pcbwcm = new CCachedBlockWriteCountsManager( pff, ib, cb ) );

    Call( pcbwcm->ErrLoad() );

    *ppcbwcm = pcbwcm;
    pcbwcm = NULL;

HandleError:
    delete pcbwcm;
    if ( err < JET_errSuccess )
    {
        delete *ppcbwcm;
        *ppcbwcm = NULL;
    }
    return err;
}
