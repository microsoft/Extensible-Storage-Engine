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
//
//  Each set of write counts is stored N times sequentially using a shadow paging scheme to prevent destruction of a
//  valid set of write counts on overwrite.

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
                                        _In_    const QWORD                 cb,
                                        _In_    const QWORD                 ccbwcs );

    private:

        IFileFilter* const          m_pff;
        const QWORD                 m_ib;
        const QWORD                 m_cb;
        const size_t                m_ccbwcs;
        const size_t                m_cbWriteSet;
        const size_t                m_cWriteSet;
        size_t                      m_iWriteSet;
        CachedBlockWriteCount       m_cbwcWriteSet;
        DWORD                       m_dwUniqueIdWriteSet;
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
    ERR                         err             = JET_errSuccess;
    CCachedBlockWriteCounts*    rgcbwcs         = NULL;
    TraceContextScope           tcScope( iorpBlockCache );
    ERR                         errWriteSets    = JET_errSuccess;
    CCachedBlockWriteCounts*    rgcbwcsValid    = NULL;

    if ( m_ib % sizeof( CCachedBlockWriteCounts ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( !m_cb )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( m_cb % ( m_ccbwcs * sizeof( CCachedBlockWriteCounts ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( m_cb > dwMax )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  cache all sets of write counts

    Alloc( rgcbwcs = (CCachedBlockWriteCounts*)PvOSMemoryPageAlloc( m_cb, NULL ) );
    Call( m_pff->ErrIORead( *tcScope, m_ib, (DWORD)m_cb, (BYTE*)rgcbwcs, qosIONormal ) );

    //  verify each set of write counts and capture the most recent valid copy

    for ( size_t iWriteSet = 0; iWriteSet < m_cWriteSet; iWriteSet++ )
    {
        ERR                     errWriteSet         = JET_errSuccess;
        CachedBlockWriteCount   cbwcWriteSet        = cbwcUnknown;
        DWORD                   dwUniqueIdWriteSet  = 0;

        for ( size_t icbwcs = 0; icbwcs < m_ccbwcs; icbwcs++ )
        {
            const size_t icbwcsT = iWriteSet * m_ccbwcs + icbwcs;

            const ERR errVerify = rgcbwcs[ icbwcsT ].ErrVerify( m_ib + icbwcsT * sizeof( CCachedBlockWriteCounts ) );
            Call( ErrIgnoreVerificationErrors( errVerify ) );

            errWriteSet = ErrAccumulateError( errWriteSet, errVerify );

            if ( cbwcWriteSet == cbwcUnknown )
            {
                cbwcWriteSet = rgcbwcs[ icbwcsT ].Cbwc();
                dwUniqueIdWriteSet = rgcbwcs[ icbwcsT ].DwUniqueId();
            }

            if ( rgcbwcs[ icbwcsT ].Cbwc() != cbwcWriteSet || rgcbwcs[ icbwcsT ].DwUniqueId() != dwUniqueIdWriteSet )
            {
                if ( m_cWriteSet > 1 )
                {
                    errWriteSet = ErrAccumulateError( errWriteSet, ErrERRCheck( JET_errReadLostFlushVerifyFailure ) );
                }
            }
        }

        if ( errWriteSet >= JET_errSuccess )
        {
            if ( m_cbwcWriteSet == cbwcUnknown || m_cbwcWriteSet < cbwcWriteSet )
            {
                m_iWriteSet = iWriteSet;
                m_cbwcWriteSet = cbwcWriteSet;
                m_dwUniqueIdWriteSet = dwUniqueIdWriteSet;
            }
        }

        errWriteSets = ErrAccumulateError( errWriteSets, errWriteSet );
    }

    //  if we didn't find any valid write set then fail

    if ( m_cbwcWriteSet == cbwcUnknown )
    {
        err = errWriteSets;

        OSTraceSuspendGC();
        const ERR errT = ErrBlockCacheInternalError( m_pff, OSFormat( "CachedBlockWriteCountsInvalid:%i", err ) );
        OSTraceResumeGC();
        Error( ErrAccumulateError( err, errT ) );
    }

    //  save our verified state

    Alloc( rgcbwcsValid = (CCachedBlockWriteCounts*)PvOSMemoryPageAlloc( m_cbWriteSet, NULL ) );
    UtilMemCpy( rgcbwcsValid, (BYTE*)rgcbwcs + m_iWriteSet * m_cbWriteSet, m_cbWriteSet );

    m_rgcbwcs = rgcbwcsValid;
    rgcbwcsValid = NULL;

HandleError:
    OSMemoryPageFree( rgcbwcsValid );
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
    const size_t                iWriteSetNew                = ( m_iWriteSet + 1 ) % m_cWriteSet;
    const CachedBlockWriteCount cbwcWriteSetNew             = m_cbwcWriteSet + 1;
    DWORD                       dwUniqueIdWriteSetNew       = 0;
    const QWORD                 ibWriteSet                  = m_ib + iWriteSetNew * m_cbWriteSet;
    TraceContextScope           tcScope( iorpBlockCache );

    //  allocate temp storage for the write counts

    Alloc( rgcbwcs = (CCachedBlockWriteCounts*)PvOSMemoryPageAlloc( m_cbWriteSet, NULL ) );

    //  freeze updates to the write counts

    m_rwlSave.EnterAsWriter();
    fReleaseWriter = fTrue;

    //  copy the write counts

    UtilMemCpy( rgcbwcs, m_rgcbwcs, m_cbWriteSet );

    //  allow updates to the write counts

    m_rwlSave.LeaveAsWriter();
    fReleaseWriter = fFalse;

    //  finalize the write counts

    rand_s( (unsigned int*)&dwUniqueIdWriteSetNew );

    for ( size_t icbwcs = 0; icbwcs < m_ccbwcs; icbwcs++ )
    {
        Call( rgcbwcs[ icbwcs ].ErrFinalize(    ibWriteSet + icbwcs * sizeof( CCachedBlockWriteCounts ),
                                                cbwcWriteSetNew,
                                                dwUniqueIdWriteSetNew ) );
    }

    //  write the entire set of write counts

    Call( m_pff->ErrIOWrite(    *tcScope, 
                                ibWriteSet,
                                (DWORD)m_cbWriteSet,
                                (const BYTE*)rgcbwcs,
                                qosIONormal ) );

    //  flush the write

    Call( m_pff->ErrFlushFileBuffers( iofrBlockCache ) );

    //  move to the new write set

    m_iWriteSet = iWriteSetNew;
    m_cbwcWriteSet = cbwcWriteSetNew;
    m_dwUniqueIdWriteSet = dwUniqueIdWriteSetNew;

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
                                                                            _In_    const QWORD                 cb,
                                                                            _In_    const QWORD                 ccbwcs )
    :   m_pff( pff ),
        m_ib( ib ),
        m_cb( cb ),
        m_ccbwcs( ccbwcs ? ccbwcs : m_cb / sizeof( CCachedBlockWriteCounts ) ),
        m_cbWriteSet( m_ccbwcs * sizeof( CCachedBlockWriteCounts ) ),
        m_cWriteSet( cb / m_cbWriteSet ),
        m_iWriteSet( 0 ),
        m_cbwcWriteSet( cbwcUnknown ),
        m_dwUniqueIdWriteSet( 0 ),
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
                            _In_    const QWORD                             ccbwcs,
                            _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm );

    private:


        CCachedBlockWriteCountsManager( _In_    IFileFilter* const          pff,
                                        _In_    const QWORD                 ib,
                                        _In_    const QWORD                 cb,
                                        _In_    const QWORD                 ccbwcs )
            : TCachedBlockWriteCountsManager<ICachedBlockWriteCountsManager>( pff, ib, cb, ccbwcs )
        {
        }
};

INLINE ERR CCachedBlockWriteCountsManager::ErrInit( _In_    IFileFilter* const                      pff,
                                                    _In_    const QWORD                             ib,
                                                    _In_    const QWORD                             cb,
                                                    _In_    const QWORD                             ccwbcs,
                                                    _Out_   ICachedBlockWriteCountsManager** const  ppcbwcm )
{
    ERR                             err     = JET_errSuccess;
    CCachedBlockWriteCountsManager* pcbwcm  = NULL;

    *ppcbwcm = NULL;

    Alloc( pcbwcm = new CCachedBlockWriteCountsManager( pff, ib, cb, ccwbcs ) );

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
