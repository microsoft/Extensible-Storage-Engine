// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_bt.hxx"
#include "_ver.hxx"

#include "PageSizeClean.hxx"



#ifdef DEBUG



#endif


BOOL FResCloseToQuota( INST * const pinst, JET_RESID resid );

#ifdef PERFMON_SUPPORT


PERFInstanceDelayedTotal<> cVERcbucketAllocated;
PERFInstanceDelayedTotal<> cVERcbucketDeleteAllocated;
PERFInstanceDelayedTotal<> cVERBucketAllocWaitForRCEClean;
PERFInstanceDelayedTotal<> cVERcbBookmarkTotal;
PERFInstanceDelayedTotal<> cVERcrceHashEntries;
PERFInstanceDelayedTotal<> cVERUnnecessaryCalls;
PERFInstanceDelayedTotal<> cVERAsyncCleanupDispatched;
PERFInstanceDelayedTotal<> cVERSyncCleanupDispatched;
PERFInstanceDelayedTotal<> cVERCleanupDiscarded;
PERFInstanceDelayedTotal<> cVERCleanupFailed;


LONG LVERcbucketAllocatedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERcbucketAllocated.PassTo( iInstance, pvBuf );
    return 0;
}


LONG LVERcbucketDeleteAllocatedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( pvBuf )
    {
        LONG counter = cVERcbucketDeleteAllocated.Get( iInstance );
        if ( counter < 0 )
        {
            cVERcbucketDeleteAllocated.Clear( iInstance );
            *((LONG *)pvBuf) = 0;
        }
        else
        {
            *((LONG *)pvBuf) = counter;
        }
    }
    return 0;
}


LONG LVERBucketAllocWaitForRCECleanCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERBucketAllocWaitForRCEClean.PassTo( iInstance, pvBuf );
    return 0;
}


LONG LVERcbAverageBookmarkCEFLPv( LONG iInstance, VOID * pvBuf )
{
    if ( NULL != pvBuf )
    {
        LONG cHash = cVERcrceHashEntries.Get( iInstance );
        LONG cBookmark = cVERcbBookmarkTotal.Get( iInstance );
        if ( 0 < cHash && 0 <= cBookmark )
        {
            *( LONG * )pvBuf = cBookmark/cHash;
        }
        else if ( 0 > cHash || 0 > cBookmark )
        {
            cVERcrceHashEntries.Clear( iInstance );
            cVERcbBookmarkTotal.Clear( iInstance );
            *( LONG * )pvBuf = 0;
        }
        else
        {
            *( LONG * )pvBuf = 0;
        }
    }
    return 0;
}


LONG LVERUnnecessaryCallsCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERUnnecessaryCalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LVERAsyncCleanupDispatchedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERAsyncCleanupDispatched.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LVERSyncCleanupDispatchedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERSyncCleanupDispatched.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LVERCleanupDiscardedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERCleanupDiscarded.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LVERCleanupFailedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    cVERCleanupFailed.PassTo( iInstance, pvBuf );
    return 0;
}

#endif




const DIRFLAG   fDIRUndo = fDIRNoLog | fDIRNoVersion | fDIRNoDirty;

volatile LONG   g_crefVERCreateIndexLock  = 0;



static const INT cbatchesMaxDefault = 128;
static const INT ctasksPerBatchMaxDefault = 1024;
static const INT ctasksBatchedMaxDefault = 4096;

VER::VER( INST *pinst )
    :   CZeroInit( sizeof( VER ) ),
        m_pinst( pinst ),
        m_fVERCleanUpWait( 2 ),
        m_msigRCECleanPerformedRecently( CSyncBasicInfo( _T( "m_msigRCECleanPerformedRecently" ) ) ),
        m_asigRCECleanDone( CSyncBasicInfo( _T( "m_asigRCECleanDone" ) ) ),
        m_critRCEClean( CLockBasicInfo( CSyncBasicInfo( szRCEClean ), rankRCEClean, 0 ) ),
        m_critBucketGlobal( CLockBasicInfo( CSyncBasicInfo( szBucketGlobal ), rankBucketGlobal, 0 ) ),
#ifdef VERPERF
        m_critVERPerf( CLockBasicInfo( CSyncBasicInfo( szVERPerf ), rankVERPerf, 0 ) ),
#endif
        m_rceidLast( 0xFFFFFFFF - ( rand() * 2 ) ),
        m_rectaskbatcher(
            pinst,
            (INT)UlParam(pinst, JET_paramVersionStoreTaskQueueMax),
            ctasksPerBatchMaxDefault,
            ctasksBatchedMaxDefault ),
        m_cresBucket( pinst )
{

    m_msigRCECleanPerformedRecently.Set();

#ifdef VERPERF
    HRT hrtStartHrts = HrtHRTCount();
#endif

    Assert(0 != (m_rceidLast % 2));
    Assert(0 == (rceidNull % 2));
}

VER::~VER()
{
}




INLINE size_t VER::CbBUFree( const BUCKET * pbucket )
{
    const size_t cbBUFree = m_cbBucket - ( (BYTE*)pbucket->hdr.prceNextNew - (BYTE*)pbucket );
    Assert( cbBUFree < m_cbBucket );
    return cbBUFree;
}


INLINE BOOL VER::FVERICleanDiscardDeletes()
{
    DWORD_PTR       cbucketMost     = 0;
    DWORD_PTR       cbucket         = 0;
    BOOL            fDiscardDeletes = fFalse;

    CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
    CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucket ) );

    if ( cbucket > min( cbucketMost, UlParam( m_pinst, JET_paramPreferredVerPages ) ) )
    {
        fDiscardDeletes = fTrue;
    }
    else if ( m_pbucketGlobalHead != m_pbucketGlobalTail
            && ( cbucketMost - cbucket ) < 2 )
    {
        fDiscardDeletes = fTrue;
    }
    else if ( FResCloseToQuota( m_pinst, JET_residFCB ) )
    {
        fDiscardDeletes = fTrue;
    }

    return fDiscardDeletes;
}

VOID VER::VERIReportDiscardedDeletes( const RCE * const prce )
{
    Assert( m_critRCEClean.FOwner() );

    FMP * const pfmp        = g_rgfmp + prce->Ifmp();

    const TRX   trxBegin0   = ( prce->FOperNull() ? trxMax : prce->TrxBegin0() );

    if ( !pfmp->FRunningOLD()
        && !prce->FOperNull()
        && ( pfmp->TrxNewestWhenDiscardsLastReported() == trxMin ||
             TrxCmp( trxBegin0, pfmp->TrxNewestWhenDiscardsLastReported() ) >= 0 ) )
    {
        WCHAR wszRceid[16], wszTrxBegin0[16], wszTrxCommit0[16], wszTrxNewestLast[16], wszTrxNewest[16];

        OSStrCbFormatW( wszRceid, sizeof( wszRceid ), L"0x%x", prce->Rceid() );
        OSStrCbFormatW( wszTrxBegin0, sizeof( wszTrxBegin0 ), L"0x%x", trxBegin0 );
        OSStrCbFormatW( wszTrxCommit0, sizeof( wszTrxCommit0 ), L"0x%x", prce->TrxCommitted() );
        OSStrCbFormatW( wszTrxNewestLast, sizeof( wszTrxNewestLast ), L"0x%x", pfmp->TrxNewestWhenDiscardsLastReported() );
        OSStrCbFormatW( wszTrxNewest, sizeof( wszTrxNewest ), L"0x%x", m_pinst->m_trxNewest );

        const WCHAR * rgcwszT[] = { pfmp->WszDatabaseName(), wszRceid, wszTrxBegin0, wszTrxCommit0, wszTrxNewestLast, wszTrxNewest };

        UtilReportEvent(
                eventWarning,
                PERFORMANCE_CATEGORY,
                MANY_LOST_COMPACTION_ID,
                _countof( rgcwszT ),
                rgcwszT,
                0,
                NULL,
                m_pinst );

        pfmp->SetTrxNewestWhenDiscardsLastReported( m_pinst->m_trxNewest );
    }
}

VOID VER::VERIReportVersionStoreOOM( PIB * ppibTrxOldest, BOOL fMaxTrxSize, const BOOL fCleanupWasRun )
{
    Assert( m_critBucketGlobal.FOwner() );
    Expected( ppibTrxOldest || !fMaxTrxSize );

    BOOL            fLockedTrxOldest = fFalse;
    const size_t    cProcs          = (size_t)OSSyncGetProcessorCountMax();
    size_t          iProc;

    if ( ppibTrxOldest == ppibNil )
    {
        for ( iProc = 0; iProc < cProcs; iProc++ )
        {
            INST::PLS* const ppls = m_pinst->Ppls( iProc );
            ppls->m_rwlPIBTrxOldest.EnterAsReader();
        }

        fLockedTrxOldest = fTrue;
        
        for ( iProc = 0; iProc < cProcs; iProc++ )
        {
            INST::PLS* const ppls = m_pinst->Ppls( iProc );
            PIB* const ppibTrxOldestCandidate = ppls->m_ilTrxOldest.PrevMost();
            if ( ppibTrxOldest == ppibNil
                || ( ppibTrxOldestCandidate
                    && TrxCmp( ppibTrxOldestCandidate->trxBegin0, ppibTrxOldest->trxBegin0 ) < 0 ) )
            {
                ppibTrxOldest = ppibTrxOldestCandidate;
            }
        }
    }

    if ( ppibNil != ppibTrxOldest )
    {
        const TRX   trxBegin0           = ppibTrxOldest->trxBegin0;

        if ( trxBegin0 != m_trxBegin0LastLongRunningTransaction )
        {
            DWORD_PTR       cbucketMost;
            DWORD_PTR       cbucket;
            WCHAR           wszSession[64];
            WCHAR           wszSesContext[cchContextStringSize];
            WCHAR           wszSesContextThreadID[cchContextStringSize];
            WCHAR           wszInst[16];
            WCHAR           wszMaxVerPages[16];
            WCHAR           wszCleanupWasRun[16];
            WCHAR           wszTransactionIds[512];

            CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
            CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucket ) );

            OSStrCbFormatW( wszSession, sizeof( wszSession ), L"0x%p:0x%x", ppibTrxOldest, trxBegin0 );
            ppibTrxOldest->PIBGetSessContextInfo( wszSesContextThreadID, wszSesContext );

            OSStrCbFormatW( wszInst, sizeof( wszInst ), L"%d", IpinstFromPinst( m_pinst ) );
            OSStrCbFormatW( wszMaxVerPages, sizeof( wszMaxVerPages ), L"%d", (ULONG)( ( cbucketMost * m_cbBucket ) / ( 1024 * 1024 ) ) );
            OSStrCbFormatW( wszCleanupWasRun, sizeof( wszCleanupWasRun ), L"%d", fCleanupWasRun );

            (void)ppibTrxOldest->TrxidStack().ErrDump( wszTransactionIds, _countof( wszTransactionIds ) );

            Assert( cbucket <= cbucketMost );
            if ( fMaxTrxSize )
            {
                const UINT  csz     = 9;
                const WCHAR * rgcwsz[csz];
                WCHAR       wszCurrVerPages[16];
                WCHAR       wszMaxTrxSize[16];

                OSStrCbFormatW( wszCurrVerPages, sizeof( wszCurrVerPages ), L"%d", (ULONG)( ( cbucket * m_cbBucket ) / ( 1024 * 1024 ) ) );
                OSStrCbFormatW( wszMaxTrxSize, sizeof( wszMaxTrxSize ), L"%d", (ULONG)( ( m_cbBucket * cbucketMost * UlParam( m_pinst, JET_paramMaxTransactionSize ) / 100 ) / ( 1024 * 1024 ) ) );

                rgcwsz[0] = wszInst;
                rgcwsz[1] = wszCurrVerPages;
                rgcwsz[2] = wszMaxTrxSize;
                rgcwsz[3] = wszMaxVerPages;
                rgcwsz[4] = wszSession;
                rgcwsz[5] = wszSesContext;
                rgcwsz[6] = wszSesContextThreadID;
                rgcwsz[7] = wszCleanupWasRun;
                rgcwsz[8] = wszTransactionIds;

                UtilReportEvent(
                        eventError,
                        TRANSACTION_MANAGER_CATEGORY,
                        VERSION_STORE_REACHED_MAXIMUM_TRX_SIZE,
                        csz,
                        rgcwsz,
                        0,
                        NULL,
                        m_pinst );
            }
            else if ( cbucket < cbucketMost )
            {
                const UINT  csz     = 9;
                const WCHAR * rgcwsz[csz];
                WCHAR       wszCurrVerPages[16];
                WCHAR       wszGlobalMinVerPages[16];

                OSStrCbFormatW( wszCurrVerPages, sizeof( wszCurrVerPages ), L"%d", (ULONG)( ( cbucket * m_cbBucket ) / ( 1024 * 1024 ) ) );
                OSStrCbFormatW( wszGlobalMinVerPages, sizeof( wszCurrVerPages ), L"%d", (ULONG)( ( UlParam( JET_paramGlobalMinVerPages ) * m_cbBucket ) / ( 1024 * 1024 ) ) );

                rgcwsz[0] = wszInst;
                rgcwsz[1] = wszCurrVerPages;
                rgcwsz[2] = wszMaxVerPages;
                rgcwsz[3] = wszGlobalMinVerPages;
                rgcwsz[4] = wszSession;
                rgcwsz[5] = wszSesContext;
                rgcwsz[6] = wszSesContextThreadID;
                rgcwsz[7] = wszCleanupWasRun;
                rgcwsz[8] = wszTransactionIds;

                UtilReportEvent(
                        eventError,
                        TRANSACTION_MANAGER_CATEGORY,
                        VERSION_STORE_OUT_OF_MEMORY_ID,
                        csz,
                        rgcwsz,
                        0,
                        NULL,
                        m_pinst );
            }
            else
            {
                const UINT  csz     = 7;
                const WCHAR * rgcwsz[csz];

                rgcwsz[0] = wszInst;
                rgcwsz[1] = wszMaxVerPages;
                rgcwsz[2] = wszSession;
                rgcwsz[3] = wszSesContext;
                rgcwsz[4] = wszSesContextThreadID;
                rgcwsz[5] = wszCleanupWasRun;
                rgcwsz[6] = wszTransactionIds;

                UtilReportEvent(
                        eventError,
                        TRANSACTION_MANAGER_CATEGORY,
                        VERSION_STORE_REACHED_MAXIMUM_ID,
                        csz,
                        rgcwsz,
                        0,
                        NULL,
                        m_pinst );
            }

            OSTrace(
                JET_tracetagVersionStoreOOM,
                OSFormat(
                    "Version Store %s (%dMb): oldest session=[0x%p:0x%x] [threadid=0x%x,inst='%ws']",
                    fMaxTrxSize ? "Maximum trx size" : ( cbucket < cbucketMost ? "Out-Of-Memory" : "reached max size" ),
                    (ULONG)( ( cbucket * m_cbBucket ) / ( 1024 * 1024 ) ),
                    ppibTrxOldest,
                    trxBegin0,
                    (ULONG)ppibTrxOldest->TidActive(),
                    ( NULL != m_pinst->m_wszDisplayName ?
                                m_pinst->m_wszDisplayName :
                                ( NULL != m_pinst->m_wszInstanceName ? m_pinst->m_wszInstanceName : L"<null>" ) ) ) );

            m_trxBegin0LastLongRunningTransaction = trxBegin0;
            m_ppibTrxOldestLastLongRunningTransaction = ppibTrxOldest;
            m_dwTrxContextLastLongRunningTransaction = ppibTrxOldest->TidActive();
        }
    }

    if ( fLockedTrxOldest )
    {
        for ( iProc = 0; iProc < cProcs; iProc++ )
        {
            INST::PLS* const ppls = m_pinst->Ppls( iProc );
            ppls->m_rwlPIBTrxOldest.LeaveAsReader();
        }
    }
}


INLINE ERR VER::ErrVERIBUAllocBucket( const INT cbRCE, const UINT uiHash )
{
    Assert( m_critBucketGlobal.FOwner() );

    Assert( m_pbucketGlobalHead == pbucketNil
        || (size_t)cbRCE > CbBUFree( m_pbucketGlobalHead ) );

    VERSignalCleanup();

    BUCKET *  pbucket = new( this ) BUCKET( this );

    if ( pbucketNil == pbucket )
    {
        m_critBucketGlobal.Leave();

        if ( uiHashInvalid != uiHash )
        {
            RwlRCEChain( uiHash ).LeaveAsWriter();
        }

        const BOOL  fCleanupWasRun  = m_msigRCECleanPerformedRecently.FWait( cmsecAsyncBackgroundCleanup );

        PERFOpt( cVERBucketAllocWaitForRCEClean.Inc( m_pinst ) );

        if ( uiHashInvalid != uiHash )
        {
            RwlRCEChain( uiHash ).EnterAsWriter();
        }

        m_critBucketGlobal.Enter();

        if ( m_pbucketGlobalHead == pbucketNil || (size_t)cbRCE > CbBUFree( m_pbucketGlobalHead ) )
        {
            pbucket = new( this ) BUCKET( this );

            if ( pbucketNil == pbucket )
            {
                VERIReportVersionStoreOOM( NULL, fFalse , fCleanupWasRun );
                return ErrERRCheck( fCleanupWasRun ?
                                        JET_errVersionStoreOutOfMemory :
                                        JET_errVersionStoreOutOfMemoryAndCleanupTimedOut );
            }
        }
        else
        {
            return JET_errSuccess;
        }
    }

    Assert( pbucketNil != pbucket );
    Assert( FAlignedForThisPlatform( pbucket ) );
    Assert( FAlignedForThisPlatform( pbucket->rgb ) );
    Assert( (BYTE *)PvAlignForThisPlatform( pbucket->rgb ) == pbucket->rgb );

    pbucket->hdr.pbucketPrev = m_pbucketGlobalHead;
    if ( pbucket->hdr.pbucketPrev )
    {
        pbucket->hdr.pbucketPrev->hdr.pbucketNext = pbucket;
    }
    else
    {
        m_pbucketGlobalTail = pbucket;
    }
    m_pbucketGlobalHead = pbucket;

    PERFOpt( cVERcbucketAllocated.Inc( m_pinst ) );
#ifdef BREAK_ON_PREFERRED_BUCKET_LIMIT
    {
    DWORD_PTR       cbucketMost     = 0;
    DWORD_PTR       cbucket         = 0;
    CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
    CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucket ) );
    AssertRTL( cbucket <= min( cbucketMost, UlParam( m_pinst, JET_paramPreferredVerPages ) ) );
    }
#endif

    if ( !m_fAboveMaxTransactionSize )
    {
        DWORD_PTR cbucketMost;
        DWORD_PTR cbucketAllocated;
        CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
        CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucketAllocated ) );
        const DWORD_PTR cbucketAllowed = cbucketMost * UlParam( m_pinst, JET_paramMaxTransactionSize ) / 100;
        if ( cbucketAllocated > cbucketAllowed )
        {
            m_fAboveMaxTransactionSize = fTrue;
        }
    }

    Assert( (size_t)cbRCE <= CbBUFree( pbucket ) );
    return ( (size_t)cbRCE > CbBUFree( pbucket ) ? ErrERRCheck( JET_errVersionStoreEntryTooBig ) : JET_errSuccess );
}


INLINE BUCKET *VER::PbucketVERIGetOldest( )
{
    Assert( m_critBucketGlobal.FOwner() );

    BUCKET  * const pbucket = m_pbucketGlobalTail;

    Assert( pbucketNil == pbucket || pbucketNil == pbucket->hdr.pbucketPrev );
    return pbucket;
}


BUCKET *VER::PbucketVERIFreeAndGetNextOldestBucket( BUCKET * pbucket )
{
    Assert( m_critBucketGlobal.FOwner() );

    BUCKET * const pbucketNext = (BUCKET *)pbucket->hdr.pbucketNext;
    BUCKET * const pbucketPrev = (BUCKET *)pbucket->hdr.pbucketPrev;

    if ( pbucketNil != pbucketNext )
    {
        Assert( m_pbucketGlobalHead != pbucket );
        pbucketNext->hdr.pbucketPrev = pbucketPrev;
    }
    else
    {
        Assert( m_pbucketGlobalHead == pbucket );
        m_pbucketGlobalHead = pbucketPrev;
    }

    if ( pbucketNil != pbucketPrev )
    {
        Assert( m_pbucketGlobalTail != pbucket );
        pbucketPrev->hdr.pbucketNext = pbucketNext;
    }
    else
    {
        m_pbucketGlobalTail = pbucketNext;
    }

    Assert( ( m_pbucketGlobalHead && m_pbucketGlobalTail )
            || ( !m_pbucketGlobalHead && !m_pbucketGlobalTail ) );

    delete pbucket;
    PERFOpt( cVERcbucketAllocated.Dec( m_pinst ) );

    if ( m_fAboveMaxTransactionSize )
    {
        DWORD_PTR cbucketMost;
        DWORD_PTR cbucketAllocated;
        CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
        CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucketAllocated ) );
        const DWORD_PTR cbucketAllowed = cbucketMost * UlParam( m_pinst, JET_paramMaxTransactionSize ) / 100;
        if ( cbucketAllocated <= cbucketAllowed )
        {
            m_fAboveMaxTransactionSize = fFalse;
        }
    }

    return pbucketNext;
}


INLINE CReaderWriterLock& VER::RwlRCEChain( UINT ui )
{
    Assert( m_frceHashTableInited );
    Assert( ui < m_crceheadHashTable );
    return m_rgrceheadHashTable[ ui ].rwl;
}

INLINE RCE *VER::GetChain( UINT ui ) const
{
    Assert( m_frceHashTableInited );
    Assert( ui < m_crceheadHashTable );
    return m_rgrceheadHashTable[ ui ].prceChain;
}

INLINE RCE **VER::PGetChain( UINT ui )
{
    Assert( m_frceHashTableInited );
    Assert( ui < m_crceheadHashTable );
    return &m_rgrceheadHashTable[ ui ].prceChain;
}

INLINE VOID VER::SetChain( UINT ui, RCE *prce )
{
    Assert( m_frceHashTableInited );
    Assert( ui < m_crceheadHashTable );
    m_rgrceheadHashTable[ ui ].prceChain = prce;
}

CReaderWriterLock& RCE::RwlChain()
{
    Assert( ::FOperInHashTable( m_oper ) );
    Assert( uiHashInvalid != m_uiHash );
    return PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash );
}


LOCAL UINT UiRCHashFunc( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const UINT crcehead = 0 )
{
    ASSERT_VALID( &bookmark );
    Assert( pgnoNull != pgnoFDP );

    const UINT crceheadHashTable = crcehead ? crcehead : PverFromIfmp( ifmp )->m_crceheadHashTable;


    UINT uiHash =   (UINT)ifmp
                    + pgnoFDP
                    + bookmark.key.prefix.Cb()
                    + bookmark.key.suffix.Cb()
                    + bookmark.data.Cb();

    INT ib;
    INT cb;
    const BYTE * pb;

    ib = 0;
    cb = (INT)bookmark.key.prefix.Cb();
    pb = (BYTE *)bookmark.key.prefix.Pv();
    switch( cb % 8 )
    {
        case 0:
        while ( ib < cb )
        {
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 7:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 6:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 5:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 4:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 3:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 2:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 1:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        }
    }

    ib = 0;
    cb = (INT)bookmark.key.suffix.Cb();
    pb = (BYTE *)bookmark.key.suffix.Pv();
    switch( cb % 8 )
    {
        case 0:
        while ( ib < cb )
        {
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 7:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 6:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 5:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 4:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 3:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 2:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 1:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        }
    }

    ib = 0;
    cb = (INT)bookmark.data.Cb();
    pb = (BYTE *)bookmark.data.Pv();
    switch( cb % 8 )
    {
        case 0:
        while ( ib < cb )
        {
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 7:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 6:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 5:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 4:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 3:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 2:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        case 1:
            uiHash = ( _rotl( uiHash, 6 ) + pb[ib++] );
        }
    }

    uiHash %= crceheadHashTable;

    return uiHash;
}


#ifdef DEBUGGER_EXTENSION

UINT UiVERHash( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const UINT crcehead )
{
    return UiRCHashFunc( ifmp, pgnoFDP, bookmark, crcehead );
}

#endif

#ifdef DEBUG


BOOL RCE::FAssertRwlHash_() const
{
    Assert( ::FOperInHashTable( m_oper ) );
    return  PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash ).FReader() ||
            PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash ).FWriter();
}

BOOL RCE::FAssertRwlHashAsWriter_() const
{
    Assert( ::FOperInHashTable( m_oper ) );
    return  PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash ).FWriter();
}


BOOL RCE::FAssertCritFCBRCEList_() const
{
    Assert( !::FOperNull( m_oper ) );
    return Pfcb()->CritRCEList().FOwner();
}


BOOL RCE::FAssertRwlPIB_() const
{
    Assert( !FFullyCommitted() );
    Assert( !::FOperNull( m_oper ) );
    return ( PinstFromPpib( m_pfucb->ppib )->RwlTrx( m_pfucb->ppib ).FReader() || Ptls()->fAddColumn );
}


BOOL RCE::FAssertReadable_() const
{
    AssertRTL( !::FOperNull( m_oper ) );
    return fTrue;
}


#endif

VOID RCE::AssertValid() const
{
    Assert( FAlignedForThisPlatform( this  ) );
    Assert( FAssertReadable_() );
    AssertRTL( rceidNull != m_rceid );
    AssertRTL( pfcbNil != Pfcb() );
    AssertRTL( trxMax != m_trxBegin0 );
    AssertRTL( TrxCmp( m_trxBegin0, TrxCommitted() ) < 0 );
    if ( trxMax == TrxCommitted() )
    {
        ASSERT_VALID( m_pfucb );
    }
    if ( ::FOperInHashTable( m_oper ) )
    {
#ifdef DEBUG
        if( FExpensiveDebugCodeEnabled( Debug_VER_AssertValid ) )
        {
            BOOKMARK    bookmark;
            GetBookmark( &bookmark );
            AssertRTL( UiRCHashFunc( Ifmp(), PgnoFDP(), bookmark ) == m_uiHash );
        }
#endif
    }
    else
    {
        AssertRTL( prceNil == m_prceNextOfNode );
        AssertRTL( prceNil == m_prcePrevOfNode );
    }
    AssertRTL( !::FOperNull( m_oper ) );

    switch( m_oper )
    {
        default:
            AssertSzRTL( fFalse, "Invalid RCE operand" );
        case operReplace:
        case operInsert:
        case operReadLock:
        case operWriteLock:
        case operPreInsert:
        case operFlagDelete:
        case operDelta:
        case operDelta64:
        case operAllocExt:
        case operCreateTable:
        case operDeleteTable:
        case operAddColumn:
        case operDeleteColumn:
        case operCreateLV:
        case operCreateIndex:
        case operDeleteIndex:
            break;
    };

    if ( FFullyCommitted() )
    {
        AssertRTL( 0 == m_level );
        AssertRTL( prceNil == m_prceNextOfSession );
        AssertRTL( prceNil == m_prcePrevOfSession );
    }

#ifdef SYNC_DEADLOCK_DETECTION

    const IFMP ifmp = Ifmp();

    if ( ::FOperInHashTable( m_oper )
        && (    PverFromIfmp( ifmp )->RwlRCEChain( m_uiHash ).FReader() ||
                PverFromIfmp( ifmp )->RwlRCEChain( m_uiHash ).FWriter() ) )
    {
        if ( prceNil != m_prceNextOfNode )
        {
            AssertRTL( RceidCmp( m_rceid, m_prceNextOfNode->m_rceid ) < 0
                        || operWriteLock == m_prceNextOfNode->Oper() );
            AssertRTL( this == m_prceNextOfNode->m_prcePrevOfNode );
        }

        if ( prceNil != m_prcePrevOfNode )
        {
            AssertRTL( RceidCmp( m_rceid, m_prcePrevOfNode->m_rceid ) > 0
                        || operWriteLock == Oper() );
            AssertRTL( this == m_prcePrevOfNode->m_prceNextOfNode );
            if ( trxMax == m_prcePrevOfNode->TrxCommitted() )
            {
                AssertRTL( TrxCmp( TrxCommitted(), TrxVERISession( m_prcePrevOfNode->m_pfucb ) ) > 0 );
            }
            const BOOL  fRedoOrUndo = ( fRecoveringRedo == PinstFromIfmp( ifmp )->m_plog->FRecoveringMode()
                                        || fRecoveringUndo == PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() );
            if( !fRedoOrUndo )
            {
                const BOOL  fPrevRCEIsDelete    = ( operFlagDelete == m_prcePrevOfNode->m_oper
                                                    && !m_prcePrevOfNode->FMoved() );
                if( fPrevRCEIsDelete )
                {
                    switch ( m_oper )
                    {
                        case operInsert:
                        case operPreInsert:
                        case operWriteLock:
                            break;

                        default:
                        {
                            Assert( fFalse );
                        }
                    }
                }
            }
        }
    }

#endif

}


#ifndef RTM


ERR VER::ErrCheckRCEChain( const RCE * const prce, const UINT uiHash ) const
{
    const RCE   * prceChainOld  = prceNil;
    const RCE   * prceChain     = prce;
    while ( prceNil != prceChain )
    {
        AssertRTL( prceChain->PrceNextOfNode() == prceChainOld );
        prceChain->AssertValid();
        AssertRTL( prceChain->PgnoFDP() == prce->PgnoFDP() );
        AssertRTL( prceChain->Ifmp() == prce->Ifmp() );
        AssertRTL( prceChain->UiHash() == uiHash );
        AssertRTL( !prceChain->FOperDDL() );

        prceChainOld    = prceChain;
        prceChain       = prceChain->PrcePrevOfNode();
    }
    return JET_errSuccess;
}


ERR VER::ErrCheckRCEHashList( const RCE * const prce, const UINT uiHash ) const
{
    ERR err = JET_errSuccess;
    const RCE * prceT = prce;
    for ( ; prceNil != prceT; prceT = prceT->PrceHashOverflow() )
    {
        CallR( ErrCheckRCEChain( prceT, uiHash ) );
    }
    return err;
}


ERR VER::ErrInternalCheck()
{
    ERR err = JET_errSuccess;
    UINT uiHash = 0;
    for ( ; uiHash < m_crceheadHashTable; ++uiHash )
    {
        if( NULL != GetChain( uiHash ) )
        {
            ENTERREADERWRITERLOCK rwlHashAsReader( &(RwlRCEChain( uiHash )), fTrue );
            const RCE * const prce = GetChain( uiHash );
            CallR( ErrCheckRCEHashList( prce, uiHash ) );
        }
    }
    return err;
}


#endif


INLINE RCE::RCE(
            FCB *       pfcb,
            FUCB *      pfucb,
            UPDATEID    updateid,
            TRX         trxBegin0,
            TRX *       ptrxCommit0,
            LEVEL       level,
            USHORT      cbBookmarkKey,
            USHORT      cbBookmarkData,
            USHORT      cbData,
            OPER        oper,
            UINT        uiHash,
            BOOL        fProxy,
            RCEID       rceid
            ) :
    m_pfcb( pfcb ),
    m_pfucb( pfucb ),
    m_ifmp( pfcb->Ifmp() ),
    m_pgnoFDP( pfcb->PgnoFDP() ),
    m_updateid( updateid ),
    m_trxBegin0( trxBegin0 ),
    m_trxCommittedInactive( trxMax ),
    m_ptrxCommitted( NULL != ptrxCommit0 ? ptrxCommit0 : &m_trxCommittedInactive ),
    m_cbBookmarkKey( cbBookmarkKey ),
    m_cbBookmarkData( cbBookmarkData ),
    m_cbData( cbData ),
    m_level( level ),
    m_prceNextOfNode( prceNil ),
    m_prcePrevOfNode( prceNil ),
    m_oper( (USHORT)oper ),
    m_uiHash( uiHash ),
    m_pgnoUndoInfo( pgnoNull ),
    m_fRolledBack( fFalse ),
    m_fMoved( fFalse ),
    m_fProxy( fProxy ? fTrue : fFalse ),
    m_fEmptyDiff( fFalse ),
    m_rceid( rceid )
    {
#ifdef DEBUG
        m_prceUndoInfoNext              = prceInvalid;
        m_prceUndoInfoPrev              = prceInvalid;
        m_prceHashOverflow              = prceInvalid;
        m_prceNextOfSession             = prceInvalid;
        m_prcePrevOfSession             = prceInvalid;
        m_prceNextOfFCB                 = prceInvalid;
        m_prcePrevOfFCB                 = prceInvalid;
#endif

    }


INLINE BYTE * RCE::PbBookmark()
{
    Assert( FAssertRwlHash_() );
    return m_rgbData + m_cbData;
}


INLINE RCE *&RCE::PrceHashOverflow()
{
    Assert( FAssertRwlHash_() );
    return m_prceHashOverflow;
}


INLINE VOID RCE::SetPrceHashOverflow( RCE * prce )
{
    Assert( FAssertRwlHashAsWriter_() );
    m_prceHashOverflow = prce;
}


INLINE VOID RCE::SetPrceNextOfNode( RCE * prce )
{
    Assert( FAssertRwlHashAsWriter_() );
    Assert( prceNil == prce
        || RceidCmp( m_rceid, prce->Rceid() ) < 0 );
    m_prceNextOfNode = prce;
}


INLINE VOID RCE::SetPrcePrevOfNode( RCE * prce )
{
    Assert( FAssertRwlHashAsWriter_() );
    Assert( prceNil == prce
        || RceidCmp( m_rceid, prce->Rceid() ) > 0 );
    m_prcePrevOfNode = prce;
}


INLINE VOID RCE::FlagRolledBack()
{
    Assert( FAssertRwlPIB_() );
    m_fRolledBack = fTrue;
}


INLINE VOID RCE::FlagMoved()
{
    m_fMoved = fTrue;
}


INLINE VOID RCE::SetPrcePrevOfSession( RCE * prce )
{
    m_prcePrevOfSession = prce;
}


INLINE VOID RCE::SetPrceNextOfSession( RCE * prce )
{
    m_prceNextOfSession = prce;
}


INLINE VOID RCE::SetLevel( LEVEL level )
{
    Assert( FAssertRwlPIB_() || PinstFromIfmp( m_ifmp )->m_plog->FRecovering() );
    Assert( m_level > level );
    m_level = level;
}


INLINE VOID RCE::SetTrxCommitted( const TRX trx )
{
    Assert( !FOperInHashTable() || FAssertRwlHashAsWriter_() );
    Assert( FAssertRwlPIB_() || PinstFromIfmp( m_ifmp )->m_plog->FRecovering() );
    Assert( FAssertCritFCBRCEList_()  );
    Assert( prceNil == m_prcePrevOfSession );
    Assert( prceNil == m_prceNextOfSession );
    Assert( prceInvalid == m_prceUndoInfoNext );
    Assert( prceInvalid == m_prceUndoInfoPrev );
    Assert( pgnoNull == m_pgnoUndoInfo );
    Assert( TrxCmp( trx, m_trxBegin0 ) > 0 );

    Assert( trx == *m_ptrxCommitted );
    Assert( trxMax == m_trxCommittedInactive );
    m_trxCommittedInactive = trx;
    m_ptrxCommitted = &m_trxCommittedInactive;
}


INLINE VOID RCE::NullifyOper()
{
    Assert( prceNil == m_prceNextOfNode );
    Assert( prceNil == m_prcePrevOfNode );
    Assert( prceNil == m_prceNextOfSession || prceInvalid == m_prceNextOfSession );
    Assert( prceNil == m_prcePrevOfSession || prceInvalid == m_prcePrevOfSession );
    Assert( prceNil == m_prceNextOfFCB || prceInvalid == m_prceNextOfFCB );
    Assert( prceNil == m_prcePrevOfFCB || prceInvalid == m_prcePrevOfFCB );
    Assert( prceInvalid == m_prceUndoInfoNext );
    Assert( prceInvalid == m_prceUndoInfoPrev );
    Assert( pgnoNull == m_pgnoUndoInfo );

    m_oper |= operMaskNull;

    m_ptrxCommitted = &m_trxCommittedInactive;
}


INLINE VOID RCE::NullifyOperForMove()
{
    m_oper |= ( operMaskNull | operMaskNullForMove );
}


LOCAL BOOL FRCECorrect( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const RCE * prce )
{
    ASSERT_VALID( prce );

    BOOL fRCECorrect = fFalse;

    if ( prce->Ifmp() == ifmp && prce->PgnoFDP() == pgnoFDP )
    {
        if ( bookmark.key.Cb() == prce->CbBookmarkKey() && (INT)bookmark.data.Cb() == prce->CbBookmarkData() )
        {
            BOOKMARK bookmarkRCE;
            prce->GetBookmark( &bookmarkRCE );

            fRCECorrect = ( CmpKeyData( bookmark, bookmarkRCE ) == 0 );
        }
    }
    return fRCECorrect;
}


BOOL RCE::FRCECorrectEDBG( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
{
    BOOL fRCECorrect = fFalse;

    if ( m_ifmp == ifmp && m_pgnoFDP == pgnoFDP )
    {
        if ( bookmark.key.Cb() == m_cbBookmarkKey && bookmark.data.Cb() == m_cbBookmarkData )
        {
            BOOKMARK bookmarkRCE;
            bookmarkRCE.key.prefix.Nullify();
            bookmarkRCE.key.suffix.SetPv( const_cast<BYTE *>( m_rgbData + m_cbData ) );
            bookmarkRCE.key.suffix.SetCb( m_cbBookmarkKey );
            bookmarkRCE.data.SetPv( const_cast<BYTE *>( m_rgbData + m_cbData + m_cbBookmarkKey ) );
            bookmarkRCE.data.SetCb( m_cbBookmarkData );

            fRCECorrect = ( CmpKeyData( bookmark, bookmarkRCE ) == 0 );
        }
    }
    return fRCECorrect;
}


LOCAL RCE **PprceRCEChainGet( UINT uiHash, IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
{
    Assert( PverFromIfmp( ifmp )->RwlRCEChain( uiHash ).FReader() ||
            PverFromIfmp( ifmp )->RwlRCEChain( uiHash ).FWriter() );

    AssertRTL( UiRCHashFunc( ifmp, pgnoFDP, bookmark ) == uiHash );

    RCE **pprceChain = PverFromIfmp( ifmp )->PGetChain( uiHash );
    while ( prceNil != *pprceChain )
    {
        RCE * const prceT = *pprceChain;

        AssertRTL( prceT->UiHash() == uiHash );

        if ( FRCECorrect( ifmp, pgnoFDP, bookmark, prceT ) )
        {
#ifdef DEBUG
            Assert( prceNil == prceT->PrcePrevOfNode()
                || prceT->PrcePrevOfNode()->UiHash() == prceT->UiHash() );
            if ( prceNil == prceT->PrceNextOfNode() )
            {
                Assert( prceNil == prceT->PrceHashOverflow()
                    || prceT->PrceHashOverflow()->UiHash() == prceT->UiHash() );
            }
            else
            {
                Assert( prceT->PrceNextOfNode()->UiHash() == prceT->UiHash() );
            }

            if ( prceNil != prceT->PrcePrevOfNode()
                && prceT->PrcePrevOfNode()->UiHash() != prceT->UiHash() )
            {
                Assert( fFalse );
            }
            if ( prceNil != prceT->PrceNextOfNode() )
            {
                if ( prceT->PrceNextOfNode()->UiHash() != prceT->UiHash() )
                {
                    Assert( fFalse );
                }
            }
            else if ( prceNil != prceT->PrceHashOverflow()
                && prceT->PrceHashOverflow()->UiHash() != prceT->UiHash() )
            {
                Assert( fFalse );
            }
#endif

            return pprceChain;
        }

        pprceChain = &prceT->PrceHashOverflow();
    }

    Assert( prceNil == *pprceChain );
    return NULL;
}


LOCAL RCE *PrceRCEGet( UINT uiHash, IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
{
    ASSERT_VALID( &bookmark );

    AssertRTL( UiRCHashFunc( ifmp, pgnoFDP, bookmark ) == uiHash );

    RCE *           prceChain   = prceNil;
    RCE **  const   pprceChain  = PprceRCEChainGet( uiHash, ifmp, pgnoFDP, bookmark );
    if ( NULL != pprceChain )
    {
        prceChain = *pprceChain;
    }

    return prceChain;
}


LOCAL RCE * PrceFirstVersion ( const UINT uiHash, const FUCB * pfucb, const BOOKMARK& bookmark )
{
    RCE * const prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

    Assert( prceNil == prce
            || prce->Oper() == operReplace
            || prce->Oper() == operInsert
            || prce->Oper() == operFlagDelete
            || prce->Oper() == operDelta
            || prce->Oper() == operDelta64
            || prce->Oper() == operReadLock
            || prce->Oper() == operWriteLock
            || prce->Oper() == operPreInsert
            );

    return prce;
}


LOCAL const RCE *PrceVERIGetPrevReplace( const RCE * const prce )
{
    const RCE * prcePrevReplace = prce->PrcePrevOfNode();
    for ( ; prceNil != prcePrevReplace
            && !prcePrevReplace->FOperReplace()
            && prcePrevReplace->TrxCommitted() == trxMax;
         prcePrevReplace = prcePrevReplace->PrcePrevOfNode() )
        ;

    if ( prceNil != prcePrevReplace )
    {
        if ( !prcePrevReplace->FOperReplace()
            || prcePrevReplace->TrxCommitted() != trxMax )
        {
            prcePrevReplace = prceNil;
        }
        else
        {
            Assert( prcePrevReplace->FOperReplace() );
            Assert( trxMax == prcePrevReplace->TrxCommitted() );
            Assert( prce->Pfucb()->ppib == prcePrevReplace->Pfucb()->ppib );
        }
    }

    return prcePrevReplace;
}


BOOL FVERActive( const IFMP ifmp, const PGNO pgnoFDP, const BOOKMARK& bm, const TRX trxSession )
{
    ASSERT_VALID( &bm );

    const UINT              uiHash      = UiRCHashFunc( ifmp, pgnoFDP, bm );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    BOOL                    fVERActive  = fFalse;

    const RCE * prce = PrceRCEGet( uiHash, ifmp, pgnoFDP, bm );
    for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
    {
        if ( TrxCmp( prce->TrxCommitted(), trxSession ) > 0 )
        {
            fVERActive = fTrue;
            break;
        }
    }

    return fVERActive;
}


BOOL FVERActive( const FUCB * pfucb, const BOOKMARK& bm, const TRX trxSession )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bm );

    Assert( trxMax != trxSession || PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    const UINT              uiHash      = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    BOOL                    fVERActive  = fFalse;

    const RCE * prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
    for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
    {
        if ( TrxCmp( prce->TrxCommitted(), trxSession ) > 0 )
        {
            fVERActive = fTrue;
            break;
        }
    }

    return fVERActive;
}


INLINE BOOL FVERIAddUndoInfo( const RCE * const prce )
{
    BOOL    fAddUndoInfo = fFalse;

    Assert( prce->TrxCommitted() == trxMax );

    Assert( !g_rgfmp[prce->Pfucb()->ifmp].FLogOn() || !PinstFromIfmp( prce->Pfucb()->ifmp )->m_plog->FLogDisabled() );
    if ( g_rgfmp[prce->Pfucb()->ifmp].FLogOn() )
    {
        if ( prce->FOperReplace() )
        {

                fAddUndoInfo = !prce->FEmptyDiff();
        }
        else if ( operFlagDelete == prce->Oper() )
        {
            fAddUndoInfo = fTrue;
        }
        else
        {
            Assert( operInsert == prce->Oper()
                    || operDelta == prce->Oper()
                    || operDelta64 == prce->Oper()
                    || operReadLock == prce->Oper()
                    || operWriteLock == prce->Oper()
                    || operPreInsert == prce->Oper()
                    || operAllocExt == prce->Oper()
                    || operCreateLV == prce->Oper()
                    || prce->FOperDDL() );
        }
    }

    return fAddUndoInfo;
}

ERR VER::ErrVERIAllocateRCE( INT cbRCE, RCE ** pprce, const UINT uiHash )
{
    Assert( m_critBucketGlobal.FOwner() );

    ERR err = JET_errSuccess;

    if ( cbRCE < (INT)0 )
    {
        return(JET_errOutOfMemory);
    }


    if ( m_pbucketGlobalHead == pbucketNil || (size_t)cbRCE > CbBUFree( m_pbucketGlobalHead ) )
    {
        Call( ErrVERIBUAllocBucket( cbRCE, uiHash ) );
    }
    Assert( (size_t)cbRCE <= CbBUFree( m_pbucketGlobalHead ) );

    Assert( FAlignedForThisPlatform( m_pbucketGlobalHead ) );


    *pprce = m_pbucketGlobalHead->hdr.prceNextNew;
    m_pbucketGlobalHead->hdr.prceNextNew =
        reinterpret_cast<RCE *>( PvAlignForThisPlatform( reinterpret_cast<BYTE *>( *pprce ) + cbRCE ) );

    Assert( FAlignedForThisPlatform( *pprce ) );
    Assert( FAlignedForThisPlatform( m_pbucketGlobalHead->hdr.prceNextNew ) );

HandleError:
    return err;
}


ERR VER::ErrVERICreateRCE(
    INT         cbNewRCE,
    FCB         *pfcb,
    FUCB        *pfucb,
    UPDATEID    updateid,
    const TRX   trxBegin0,
    TRX *       ptrxCommit0,
    const LEVEL level,
    INT         cbBookmarkKey,
    INT         cbBookmarkData,
    OPER        oper,
    UINT        uiHash,
    RCE         **pprce,
    const BOOL  fProxy,
    RCEID       rceid
    )
{
    ERR         err                 = JET_errSuccess;
    RCE *       prce                = prceNil;
    UINT        uiHashConcurrentOp;

    if ( FOperConcurrent( oper ) )
    {
        Assert( uiHashInvalid != uiHash );
        RwlRCEChain( uiHash ).EnterAsWriter();
        uiHashConcurrentOp = uiHash;
    }
    else
    {
        uiHashConcurrentOp = uiHashInvalid;
    }

    m_critBucketGlobal.Enter();

    Assert( pfucbNil == pfucb ? 0 == level : level > 0 );

    Assert( cbNewRCE >= (INT)( sizeof(RCE) + cbBookmarkKey + cbBookmarkData ) );
    Assert( cbNewRCE <= (INT)( sizeof(RCE) + cbBookmarkKey + cbBookmarkData + USHRT_MAX ) );
    if (    cbNewRCE < (INT)( sizeof(RCE) + cbBookmarkKey + cbBookmarkData ) ||
            cbNewRCE > (INT)( sizeof(RCE) + cbBookmarkKey + cbBookmarkData + USHRT_MAX ))
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }

    Call( ErrVERIAllocateRCE( cbNewRCE, &prce, uiHashConcurrentOp ) );

#ifdef DEBUG
    if ( !PinstFromIfmp( pfcb->Ifmp() )->m_plog->FRecovering() )
    {
        Assert( rceidNull == rceid );
    }
    else
    {
        Assert( rceidNull != rceid && uiHashInvalid != uiHash ||
                rceidNull == rceid && uiHashInvalid == uiHash );
    }
#endif

    Assert( trxMax != trxBegin0 );
    Assert( NULL == ptrxCommit0 || trxMax == *ptrxCommit0 );


    new( prce ) RCE(
            pfcb,
            pfucb,
            updateid,
            trxBegin0,
            ptrxCommit0,
            level,
            USHORT( cbBookmarkKey ),
            USHORT( cbBookmarkData ),
            USHORT( cbNewRCE - ( sizeof(RCE) + cbBookmarkKey + cbBookmarkData ) ),
            oper,
            uiHash,
            fProxy,
            rceidNull == rceid ? RceidLastIncrement() : rceid
            );

HandleError:
    m_critBucketGlobal.Leave();

    if ( err >= 0 )
    {
        Assert( prce != prceNil );

        Assert( FAlignedForThisPlatform( prce ) );
        Assert( (RCE *)PvAlignForThisPlatform( prce ) == prce );

        *pprce = prce;
    }
    else if ( err < JET_errSuccess && FOperConcurrent( oper ) )
    {
        RwlRCEChain( uiHash ).LeaveAsWriter();
    }

    return err;
}


LOCAL ERR ErrVERIInsertRCEIntoHash( RCE * const prce )
{
    BOOKMARK    bookmark;
    prce->GetBookmark( &bookmark );
#ifdef DEBUG_VER
    Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif

    RCE ** const    pprceChain  = PprceRCEChainGet( prce->UiHash(),
                                                    prce->Ifmp(),
                                                    prce->PgnoFDP(),
                                                    bookmark );

    if ( pprceChain )
    {
        Assert( *pprceChain != prceNil );

        Assert( prce->Rceid() != rceidNull );

        RCE * prceNext      = prceNil;
        RCE * prcePrev      = *pprceChain;
        const RCEID rceid   = prce->Rceid();


        if( PinstFromIfmp( prce->Ifmp() )->m_plog->FRecovering() )
        {
            for ( ;
                  prcePrev != prceNil && RceidCmp( prcePrev->Rceid(), rceid ) >= 0;
                  prceNext = prcePrev,
                  prcePrev = prcePrev->PrcePrevOfNode() )
            {
                if ( prcePrev->Rceid() == rceid )
                {

                    if( pgnoNull != prcePrev->PgnoUndoInfo() )
                    {
                        LGPOS lgposTip;
                        PinstFromIfmp( prce->Ifmp() )->m_plog->LGLogTipWithLock( &lgposTip );
                        BFRemoveUndoInfo( prcePrev, lgposTip );
                    }

                    Assert( PinstFromIfmp( prce->Ifmp() )->m_plog->FRecovering() );
                    Assert( FAlignedForThisPlatform( prce ) );

                    VER *pver = PverFromIfmp( prce->Ifmp() );
                    Assert( (RCE *)PvAlignForThisPlatform( (BYTE *)prce + prce->CbRce() )
                                == pver->m_pbucketGlobalHead->hdr.prceNextNew );
                    pver->m_pbucketGlobalHead->hdr.prceNextNew = prce;

                    ERR err = ErrERRCheck( JET_errPreviousVersion );
                    return err;
                }
            }
        }
        else
        {


        }

        if ( prceNil == prceNext )
        {
            Assert( prcePrev == *pprceChain );
            prce->SetPrceHashOverflow( (*pprceChain)->PrceHashOverflow() );
            *pprceChain = prce;

            #ifdef DEBUG
            prcePrev->SetPrceHashOverflow( prceInvalid );
            #endif
            Assert( prceNil == prce->PrceNextOfNode() );
        }
        else
        {
            Assert( PinstFromIfmp( prce->Ifmp() )->m_plog->FRecovering() );
            Assert( prcePrev != prceNext );
            Assert( prcePrev != *pprceChain );

            prceNext->SetPrcePrevOfNode( prce );
            prce->SetPrceNextOfNode( prceNext );

            Assert( prce->UiHash() == prceNext->UiHash() );
        }

        if ( prcePrev != prceNil )
        {
            prcePrev->SetPrceNextOfNode( prce );

            Assert( prce->UiHash() == prcePrev->UiHash() );
        }

        prce->SetPrcePrevOfNode( prcePrev );
    }
    else
    {
        Assert( prceNil == prce->PrceNextOfNode() );
        Assert( prceNil == prce->PrcePrevOfNode() );

        prce->SetPrceHashOverflow( PverFromIfmp( prce->Ifmp() )->GetChain( prce->UiHash() ) );
        PverFromIfmp( prce->Ifmp() )->SetChain( prce->UiHash(), prce );
    }

    Assert( prceNil != prce->PrceNextOfNode()
        || prceNil == prce->PrceHashOverflow()
        || prce->PrceHashOverflow()->UiHash() == prce->UiHash() );

#ifdef DEBUG
    if ( prceNil == prce->PrceNextOfNode()
        && prceNil != prce->PrceHashOverflow()
        && prce->PrceHashOverflow()->UiHash() != prce->UiHash() )
    {
        Assert( fFalse );
    }
#endif

#ifdef DEBUG_VER
    Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif

    Assert( prceNil == prce->PrceNextOfNode() ||
            PinstFromIfmp( prce->Ifmp() )->m_plog->FRecovering()
                && RceidCmp( prce->PrceNextOfNode()->Rceid(), prce->Rceid() ) > 0 );

    Assert( prce->Pfcb() != pfcbNil );

{

    PERFOptDeclare( const INT cbBookmark = prce->CbBookmarkKey() + prce->CbBookmarkData() );
    PERFOptDeclare( VER *pver = PverFromIfmp( prce->Ifmp() ) );
    PERFOptDeclare( INST *pinst = pver->m_pinst );
    PERFOpt( cVERcrceHashEntries.Inc( pinst ) );
    PERFOpt( cVERcbBookmarkTotal.Add( pinst, cbBookmark ) );
}

    return JET_errSuccess;
}


LOCAL VOID VERIDeleteRCEFromHash( RCE * const prce )
{
    BOOKMARK    bookmark;
    prce->GetBookmark( &bookmark );
#ifdef DEBUG_VER
    Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif

    RCE ** const pprce = PprceRCEChainGet( prce->UiHash(), prce->Ifmp(), prce->PgnoFDP(), bookmark );
    AssertPREFIX( pprce != NULL );

    if ( prce == *pprce )
    {
        Assert( prceNil == prce->PrceNextOfNode() );

        if ( prceNil != prce->PrcePrevOfNode() )
        {
            Assert( prce->PrcePrevOfNode()->UiHash() == prce->UiHash() );
            prce->PrcePrevOfNode()->SetPrceHashOverflow( prce->PrceHashOverflow() );
            *pprce = prce->PrcePrevOfNode();
            (*pprce)->SetPrceNextOfNode( prceNil );
            Assert( prceInvalid != (*pprce)->PrceHashOverflow() );
            ASSERT_VALID( *pprce );
        }
        else
        {
            *pprce = prce->PrceHashOverflow();
            Assert( prceInvalid != *pprce );
        }
    }
    else
    {
        Assert( prceNil != prce->PrceNextOfNode() );
        RCE * const prceNext = prce->PrceNextOfNode();

        Assert( prceNext->UiHash() == prce->UiHash() );

        prceNext->SetPrcePrevOfNode( prce->PrcePrevOfNode() );
        if ( prceNil != prceNext->PrcePrevOfNode() )
        {
            Assert( prceNext->PrcePrevOfNode()->UiHash() == prce->UiHash() );
            prceNext->PrcePrevOfNode()->SetPrceNextOfNode( prceNext );
        }
    }

    prce->SetPrceNextOfNode( prceNil );
    prce->SetPrcePrevOfNode( prceNil );


    PERFOptDeclare( VER *pver = PverFromIfmp( prce->Ifmp() ) );
    PERFOptDeclare( INST *pinst = pver->m_pinst );
    PERFOpt( cVERcrceHashEntries.Dec( pinst ) );
    PERFOpt( cVERcbBookmarkTotal.Add( pinst, -(prce->CbBookmarkKey() + prce->CbBookmarkData()) ) );
}


INLINE VOID VERIInsertRCEIntoSessionList( PIB * const ppib, RCE * const prce )
{
    RCE         *prceNext   = prceNil;
    RCE         *prcePrev   = ppib->prceNewest;
    const RCEID rceid       = prce->Rceid();
    const LEVEL level       = prce->Level();
    INST        *pinst      = PinstFromPpib( ppib );
    LOG         *plog       = pinst->m_plog;

    Assert( level > 0 );
    Assert( level == ppib->Level()
        || ( level < ppib->Level() && plog->FRecovering() ) );

    if ( !plog->FRecovering()
        || prceNil == prcePrev
        || level > prcePrev->Level()
        || ( level == prcePrev->Level() && RceidCmp( rceid, prcePrev->Rceid() ) > 0) )
    {
        prce->SetPrceNextOfSession( prceNil );
        prce->SetPrcePrevOfSession( ppib->prceNewest );
        if ( prceNil != ppib->prceNewest )
        {
            Assert( prceNil == ppib->prceNewest->PrceNextOfSession() );
            Assert( ppib->prceNewest->Level() <= prce->Level() );
            ppib->prceNewest->SetPrceNextOfSession( prce );
        }
        PIBSetPrceNewest( ppib, prce );
        Assert( prce == ppib->prceNewest );
    }

    else
    {
        Assert( plog->FRecovering() );

        prcePrev = ppib->PrceNearestRegistered(prce->Rceid());

        while( prcePrev && (level > prcePrev->Level() || RceidCmp( prcePrev->Rceid(), rceid ) < 0 ) )
        {
            prcePrev = prcePrev->PrceNextOfSession();
        }

        if (NULL == prcePrev)
        {
            prcePrev = ppib->prceNewest;
        }

        Assert( level < prcePrev->Level() || RceidCmp( rceid, prcePrev->Rceid() ) < 0 );
        for ( ;
            prcePrev != prceNil
                && ( level < prcePrev->Level()
                    || ( level == prcePrev->Level() &&
RceidCmp( rceid, prcePrev->Rceid() ) < 0 ) );
            prceNext = prcePrev,
            prcePrev = prcePrev->PrcePrevOfSession() )
        {
            NULL;
        }

        Assert( prceNil != prceNext );
        Assert( prceNext->Level() > level
            || prceNext->Level() == level && RceidCmp( prceNext->Rceid(), rceid ) > 0 );
        prceNext->SetPrcePrevOfSession( prce );

        if ( prceNil != prcePrev )
        {
            Assert( prcePrev->Level() < level
                || ( prcePrev->Level() == level && RceidCmp( prcePrev->Rceid(), rceid ) < 0 ) );
            prcePrev->SetPrceNextOfSession( prce );
        }

        prce->SetPrcePrevOfSession( prcePrev );
        prce->SetPrceNextOfSession( prceNext );
    }

    if ( plog->FRecovering() )
    {
        (void)ppib->ErrRegisterRceid(prce->Rceid(), prce);
    }
    
    Assert( prceNil == ppib->prceNewest->PrceNextOfSession() );
}


INLINE VOID VERIInsertRCEIntoSessionList( PIB * const ppib, RCE * const prce, RCE * const prceParent )
{
    Assert( PinstFromPpib( ppib )->RwlTrx( ppib ).FWriter() );

    Assert( !PinstFromPpib( ppib )->m_plog->FRecovering() );

    Assert( prceParent->TrxCommitted() == trxMax );
    Assert( prceParent->Level() == prce->Level() );
    Assert( prceNil != ppib->prceNewest );
    Assert( ppib == prce->Pfucb()->ppib );

    RCE *prcePrev = prceParent->PrcePrevOfSession();

    prce->SetPrceNextOfSession( prceParent );
    prce->SetPrcePrevOfSession( prcePrev );
    prceParent->SetPrcePrevOfSession( prce );

    if ( prceNil != prcePrev )
    {
        prcePrev->SetPrceNextOfSession( prce );
    }

    Assert( prce != ppib->prceNewest );
}


LOCAL VOID VERIDeleteRCEFromSessionList( PIB * const ppib, RCE * const prce )
{
    Assert( !prce->FFullyCommitted() );
    Assert( ppib == prce->Pfucb()->ppib );

    if ( prce == ppib->prceNewest )
    {
        PIBSetPrceNewest( ppib, prce->PrcePrevOfSession() );
        if ( prceNil != ppib->prceNewest )
        {
            Assert( prce == ppib->prceNewest->PrceNextOfSession() );
            ppib->prceNewest->SetPrceNextOfSession( prceNil );
        }
    }
    else
    {
        Assert( prceNil != prce->PrceNextOfSession() );
        RCE * const prceNext = prce->PrceNextOfSession();
        RCE * const prcePrev = prce->PrcePrevOfSession();
        prceNext->SetPrcePrevOfSession( prcePrev );
        if ( prceNil != prcePrev )
        {
            prcePrev->SetPrceNextOfSession( prceNext );
        }
    }
    prce->SetPrceNextOfSession( prceNil );
    prce->SetPrcePrevOfSession( prceNil );

    if ( PinstFromPpib( ppib )->m_plog->FRecovering() )
    {
        (void)ppib->ErrDeregisterRceid(prce->Rceid());
    }
}


LOCAL VOID VERIInsertRCEIntoFCBList( FCB * const pfcb, RCE * const prce )
{
    Assert( pfcb->CritRCEList().FOwner() );
    pfcb->AttachRCE( prce );
}


LOCAL VOID VERIDeleteRCEFromFCBList( FCB * const pfcb, RCE * const prce )
{
    Assert( pfcb->CritRCEList().FOwner() );
    pfcb->DetachRCE( prce );
}


VOID VERInsertRCEIntoLists(
    FUCB        *pfucbNode,
    CSR         *pcsr,
    RCE         *prce,
    const VERPROXY  *pverproxy )
{
    Assert( prceNil != prce );

    FCB * const pfcb = prce->Pfcb();
    Assert( pfcbNil != pfcb );

    LOG *plog = PinstFromIfmp( pfcb->Ifmp() )->m_plog;

    if ( prce->TrxCommitted() == trxMax )
    {
        FUCB    *pfucbVer = prce->Pfucb();
        Assert( pfucbNil != pfucbVer );
        Assert( pfcb == pfucbVer->u.pfcb );

        Assert( pfucbNode == pfucbVer || pverproxy != NULL );

        if ( FVERIAddUndoInfo( prce ) )
        {
            Assert( pcsrNil != pcsr || plog->FRecovering() );
            if ( pcsrNil != pcsr )
            {
                Assert( !g_rgfmp[ pfcb->Ifmp() ].FLogOn() || !plog->FLogDisabled() );
                if ( g_rgfmp[ pfcb->Ifmp() ].FLogOn() )
                {
                    if( pcsr->Cpage().FLoadedPage() )
                    {
                        Assert( plog->FRecovering() );
                    }
                    else
                    {
                        BFAddUndoInfo( pcsr->Cpage().PBFLatch(), prce );
                    }
                }
            }
        }

        if ( NULL != pverproxy && proxyCreateIndex == pverproxy->proxy )
        {
            Assert( !plog->FRecovering() );
            Assert( pfucbNode != pfucbVer );
            Assert( prce->Oper() == operInsert
                || prce->Oper() == operReplace
                || prce->Oper() == operFlagDelete );
            VERIInsertRCEIntoSessionList( pfucbVer->ppib, prce, pverproxy->prcePrimary );
        }
        else
        {
            Assert( pfucbNode == pfucbVer );
            if ( NULL != pverproxy )
            {
                Assert( plog->FRecovering() );
                Assert( proxyRedo == pverproxy->proxy );
                Assert( prce->Level() == pverproxy->level );
                Assert( prce->Level() > 0 );
                Assert( prce->Level() <= pfucbVer->ppib->Level() );
            }
            VERIInsertRCEIntoSessionList( pfucbVer->ppib, prce );
        }
    }
    else
    {
        Assert( !plog->FRecovering() );
        Assert( NULL != pverproxy );
        Assert( proxyCreateIndex == pverproxy->proxy );
        Assert( prceNil != pverproxy->prcePrimary );
        Assert( pverproxy->prcePrimary->TrxCommitted() == prce->TrxCommitted() );
        Assert( pfcb->FTypeSecondaryIndex() );
        Assert( pfcb->PfcbTable() == pfcbNil );
        Assert( prce->Oper() == operInsert
                || prce->Oper() == operReplace
                || prce->Oper() == operFlagDelete );
    }


    ENTERCRITICALSECTION enterCritFCBRCEList( &(pfcb->CritRCEList()) );
    VERIInsertRCEIntoFCBList( pfcb, prce );
}


LOCAL VOID VERINullifyUncommittedRCE( RCE * const prce )
{
    Assert( prceInvalid != prce->PrceNextOfSession() );
    Assert( prceInvalid != prce->PrcePrevOfSession() );
    Assert( prceInvalid != prce->PrceNextOfFCB() );
    Assert( prceInvalid != prce->PrcePrevOfFCB() );
    Assert( !prce->FFullyCommitted() );

    BFRemoveUndoInfo( prce );

    VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );
    VERIDeleteRCEFromSessionList( prce->Pfucb()->ppib, prce );

    const BOOL fInHash = prce->FOperInHashTable();
    if ( fInHash )
    {
        VERIDeleteRCEFromHash( prce );
    }

    prce->NullifyOper();
}


LOCAL VOID VERINullifyRolledBackRCE(
    PIB             *ppib,
    RCE             * const prceToNullify,
    RCE             **pprceNextToNullify = NULL )
{
    const BOOL          fOperInHashTable        = prceToNullify->FOperInHashTable();
    CReaderWriterLock   *prwlHash               = fOperInHashTable ?
                                                    &( PverFromIfmp( prceToNullify->Ifmp() )->RwlRCEChain( prceToNullify->UiHash() ) ) :
                                                    NULL;
    CCriticalSection&   critRCEList             = prceToNullify->Pfcb()->CritRCEList();
    const BOOL          fPossibleSecondaryIndex = prceToNullify->Pfcb()->FTypeTable()
                                                    && !prceToNullify->Pfcb()->FFixedDDL()
                                                    && prceToNullify->FOperAffectsSecondaryIndex();


    Assert( PinstFromPpib( ppib )->RwlTrx( ppib ).FReader() );

    if ( fOperInHashTable )
    {
        prwlHash->EnterAsWriter();
    }
    critRCEList.Enter();

    prceToNullify->FlagRolledBack();

    const LONG  crefCreateIndexLock     = ( fPossibleSecondaryIndex ? g_crefVERCreateIndexLock : 0 );
    Assert( g_crefVERCreateIndexLock >= 0 );
    if ( 0 == crefCreateIndexLock )
    {
        if ( NULL != pprceNextToNullify )
            *pprceNextToNullify = prceToNullify->PrcePrevOfSession();

        Assert( !prceToNullify->FOperNull() );
        Assert( prceToNullify->Level() <= ppib->Level() );
        Assert( prceToNullify->TrxCommitted() == trxMax );

        VERINullifyUncommittedRCE( prceToNullify );

        critRCEList.Leave();
        if ( fOperInHashTable )
        {
            prwlHash->LeaveAsWriter();
        }
    }
    else
    {
        Assert( crefCreateIndexLock > 0 );

        critRCEList.Leave();
        if ( fOperInHashTable )
        {
            prwlHash->LeaveAsWriter();
        }


        if ( NULL != pprceNextToNullify )
        {
            PinstFromPpib( ppib )->RwlTrx( ppib ).LeaveAsReader();
            UtilSleep( cmsecWaitGeneric );
            PinstFromPpib( ppib )->RwlTrx( ppib ).EnterAsReader();

            *pprceNextToNullify = ppib->prceNewest;
        }
    }
}


LOCAL VOID VERINullifyCommittedRCE( RCE * const prce )
{
    Assert( prce->PgnoUndoInfo() == pgnoNull );
    Assert( prce->TrxCommitted() != trxMax );
    Assert( prce->FFullyCommitted() );
    VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );

    const BOOL fInHash = prce->FOperInHashTable();
    if ( fInHash )
    {
        VERIDeleteRCEFromHash( prce );
    }

    prce->NullifyOper();
}


INLINE VOID VERINullifyRCE( RCE *prce )
{
    if ( prce->FFullyCommitted() )
    {
        VERINullifyCommittedRCE( prce );
    }
    else
    {
        VERINullifyUncommittedRCE( prce );
    }
}


VOID VERNullifyFailedDMLRCE( RCE *prce )
{
    ASSERT_VALID( prce );
    Assert( prceInvalid == prce->PrceNextOfSession() || prceNil == prce->PrceNextOfSession() );
    Assert( prceInvalid == prce->PrcePrevOfSession() || prceNil == prce->PrcePrevOfSession() );

    BFRemoveUndoInfo( prce );

    Assert( prce->FOperInHashTable() );

    {
        ENTERREADERWRITERLOCK enterRwlHashAsWriter( &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ), fFalse );
        VERIDeleteRCEFromHash( prce );
    }

    prce->NullifyOper();
}


VOID VERNullifyAllVersionsOnFCB( FCB * const pfcb )
{
    VER *pver = PverFromIfmp( pfcb->Ifmp() );
    LOG *plog = pver->m_pinst->m_plog;

    pfcb->CritRCEList().Enter();

    Assert( pfcb->FTypeTable()
        || pfcb->FTypeSecondaryIndex()
        || pfcb->FTypeTemporaryTable()
        || pfcb->FTypeLV() );

    while ( prceNil != pfcb->PrceOldest() )
    {
        RCE * const prce = pfcb->PrceOldest();

        Assert( prce->Pfcb() == pfcb );
        Assert( !prce->FOperNull() );

        Assert( prce->Ifmp() == pfcb->Ifmp() );
        Assert( prce->PgnoFDP() == pfcb->PgnoFDP() );

        Assert( prce->TrxCommitted() != trxMax || !plog->FRecovering() );

        if ( prce->FOperInHashTable() )
        {
            const UINT  uiHash          = prce->UiHash();
            const BOOL  fNeedRwlTrx     = ( prce->TrxCommitted() == trxMax
                                            && !FFMPIsTempDB( prce->Ifmp() ) );
            PIB         *ppib;

            Assert( !pfcb->FTypeTable()
                || plog->FRecovering()
                || ( prce->FMoved() && operFlagDelete == prce->Oper() ) );

            if ( fNeedRwlTrx )
            {
                Assert( prce->Pfucb() != pfucbNil );
                Assert( prce->Pfucb()->ppib != ppibNil );
                ppib = prce->Pfucb()->ppib;
            }
            else
            {
                ppib = ppibNil;
            }

            pfcb->CritRCEList().Leave();

            if ( plog->FRecovering() )
            {
                pver->m_critRCEClean.Enter();
            }
            ENTERREADERWRITERLOCK   enterRwlPIBTrx(
                                        fNeedRwlTrx ? &PinstFromPpib( ppib )->RwlTrx( ppib ) : NULL,
                                        fFalse,
                                        fNeedRwlTrx );
            ENTERREADERWRITERLOCK   enterRwlHashAsWriter( &( pver->RwlRCEChain( uiHash ) ), fFalse );

            pfcb->CritRCEList().Enter();

            if ( pfcb->PrceOldest() == prce )
            {
                Assert( !prce->FOperNull() );
                VERINullifyRCE( prce );
            }
            if ( plog->FRecovering() )
            {
                pver->m_critRCEClean.Leave();
            }
        }
        else
        {
            if ( !plog->FRecovering() )
            {
                Assert( FFMPIsTempDB( prce->Ifmp() ) );
                Assert( prce->Pfcb()->FTypeTemporaryTable() );
            }

            pfcb->CritRCEList().Leave();
            pver->m_critRCEClean.Enter();
            pfcb->CritRCEList().Enter();


            if ( pfcb->PrceOldest() == prce )
            {
                Assert( !prce->FOperNull() );
                VERINullifyRCE( prce );
            }
            pver->m_critRCEClean.Leave();
        }
    }

    pfcb->CritRCEList().Leave();
}


VOID VERNullifyInactiveVersionsOnBM( const FUCB * pfucb, const BOOKMARK& bm )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bm );

    INST * const            pinst               = PinstFromPfucb( pfucb );
    bool                    fUpdatedTrxOldest   = false;
    TRX                     trxOldest           = TrxOldestCached( pinst );
    const UINT              uiHash              = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
    ENTERREADERWRITERLOCK   enterRwlHashAsWriter( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fFalse );

    bool fNullified = false;
    
    RCE * prcePrev;
    RCE * prce      = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
    for ( ; prceNil != prce; prce = prcePrev )
    {
        prcePrev = prce->PrcePrevOfNode();

        if ( TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 && !fUpdatedTrxOldest )
        {
            UpdateCachedTrxOldest( pinst );
            trxOldest = TrxOldestCached( pinst );
            fUpdatedTrxOldest = true;
        }

        if ( TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 )
        {
            FCB * const pfcb = prce->Pfcb();
            ENTERCRITICALSECTION enterCritFCBRCEList( &( pfcb->CritRCEList() ) );

            VERINullifyRCE( prce );
            fNullified = true;
        }
    }
}





VOID VER::VERSignalCleanup()
{
    if ( NULL != m_pbucketGlobalTail
        && 0 == AtomicCompareExchange( (LONG *)&m_fVERCleanUpWait, 0, 1 ) )
    {
        m_msigRCECleanPerformedRecently.Reset();

        if ( m_fSyncronousTasks
            || m_pinst->Taskmgr().ErrTMPost( VERIRCECleanProc, this ) < JET_errSuccess )
        {
            m_msigRCECleanPerformedRecently.Set();

            LONG fStatus = AtomicCompareExchange( (LONG *)&m_fVERCleanUpWait, 1, 0 );
            if ( 2 == fStatus )
            {
                m_asigRCECleanDone.Set();
            }
        }
    }
}


DWORD VER::VERIRCECleanProc( VOID *pvThis )
{
    VER *pver = (VER *)pvThis;

    pver->m_critRCEClean.Enter();
    pver->ErrVERIRCEClean();
    LONG fStatus = AtomicCompareExchange( (LONG *)&pver->m_fVERCleanUpWait, 1, 0 );
    pver->m_critRCEClean.Leave();

    pver->m_msigRCECleanPerformedRecently.Set();

    if ( 2 == fStatus )
    {
        pver->m_asigRCECleanDone.Set();
    }

    return 0;
}


DWORD_PTR CbVERISize( BOOL fLowMemory, BOOL fMediumMemory, DWORD_PTR cbVerStoreMax )
{
    DWORD_PTR cbVER;

    if ( fLowMemory )
    {
        cbVER = 16*1024;
    }
    else if ( fMediumMemory )
    {
        cbVER = 64*1024;
    }
    else
    {
        cbVER = cbVerStoreMax / 1024;
        cbVER = cbVER * VER::cbrcehead / VER::cbrceheadLegacy;
        cbVER = max( cbVER, OSMemoryPageReserveGranularity() );
    }

#if DEBUG
    cbVER = cbVER / 30;
#endif
    cbVER = roundup( cbVER, OSMemoryPageCommitGranularity() );

    return cbVER;
}


JETUNITTEST( VER, CheckVerSizing )
{
#if DEBUG
    CHECK( OSMemoryPageCommitGranularity() == CbVERISize( fTrue, fFalse, 0 ) );
    CHECK( OSMemoryPageCommitGranularity() == CbVERISize( fTrue, fFalse, INT_MAX ) );

    CHECK( OSMemoryPageCommitGranularity() == CbVERISize( fFalse, fTrue, 0 ) );
    CHECK( OSMemoryPageCommitGranularity() == CbVERISize( fFalse, fTrue, INT_MAX ) );

    CHECK( OSMemoryPageCommitGranularity() == CbVERISize( fFalse, fFalse, 0 ) );
    CHECK( 72*1024 == CbVERISize( fFalse, fFalse, 1024*1024*1024 ) );
#else
    CHECK( 16*1024 == CbVERISize( fTrue, fFalse, 0 ) );
    CHECK( 16*1024 == CbVERISize( fTrue, fFalse, INT_MAX ) );

    CHECK( 64*1024 == CbVERISize( fFalse, fTrue, 0 ) );
    CHECK( 64*1024 == CbVERISize( fFalse, fTrue, INT_MAX ) );

    CHECK( OSMemoryPageReserveGranularity() == CbVERISize( fFalse, fFalse, 0 ) );
    CHECK( 2048*1024 == CbVERISize( fFalse, fFalse, 1024*1024*1024 ) );
#endif
}



VER * VER::VERAlloc( INST* pinst )
{
    C_ASSERT( OffsetOf( VER, m_rgrceheadHashTable ) == sizeof( VER ) );

    const BOOL fLowMemory = pinst->m_config & JET_configLowMemory;
    const BOOL fMediumMemory = ( pinst->m_config & JET_configDynamicMediumMemory ) || FDefaultParam( pinst, JET_paramMaxVerPages );
    DWORD_PTR cbBucket;
    CallS( ErrRESGetResourceParam( pinstNil, JET_residVERBUCKET, JET_resoperSize, &cbBucket ) );
    const DWORD_PTR cbVersionStoreMax = UlParam( pinst, JET_paramMaxVerPages ) * cbBucket;
    const DWORD_PTR cbMemoryMax = (DWORD_PTR)min( OSMemoryPageReserveTotal(), OSMemoryTotal() );
    const DWORD_PTR cbVER = CbVERISize( fLowMemory, fMediumMemory, min( cbVersionStoreMax, cbMemoryMax / 2 ) );

    VOID *pv = PvOSMemoryPageAlloc( cbVER, NULL );
    if ( pv == NULL )
    {
        return NULL;
    }

    VER *pVer = new (pv) VER( pinst );
    pVer->m_crceheadHashTable = UINT( ( cbVER - sizeof( VER ) ) / sizeof( VER::RCEHEAD ) );
#ifndef DEBUG
    if ( FDefaultParam( pinst, JET_paramMaxVerPages ) )
    {
        pVer->m_crceheadHashTable = min( pVer->m_crceheadHashTable, 4001 );
    }
#endif

    AssertRTL( sizeof(VER::RCEHEAD) *  pVer->m_crceheadHashTable < cbVER );

    return pVer;
}

VOID VER::VERFree( VER * pVer )
{
    if ( pVer == NULL )
    {
        return;
    }

    pVer->~VER();
    OSMemoryPageFree( pVer );
}

ERR VER::ErrVERInit( INST* pinst )
{
    ERR     err = JET_errSuccess;
    OPERATION_CONTEXT opContext = { OCUSER_VERSTORE, 0, 0, 0, 0 };

    PERFOpt( cVERcbucketAllocated.Clear( m_pinst ) );
    PERFOpt( cVERcbucketDeleteAllocated.Clear( m_pinst ) );
    PERFOpt( cVERBucketAllocWaitForRCEClean.Clear( m_pinst ) );
    PERFOpt( cVERcrceHashEntries.Clear( m_pinst ) );
    PERFOpt( cVERcbBookmarkTotal.Clear( m_pinst ) );
    PERFOpt( cVERUnnecessaryCalls.Clear( m_pinst ) );
    PERFOpt( cVERSyncCleanupDispatched.Clear( m_pinst ) );
    PERFOpt( cVERAsyncCleanupDispatched.Clear( m_pinst ) );
    PERFOpt( cVERCleanupDiscarded.Clear( m_pinst ) );
    PERFOpt( cVERCleanupFailed.Clear( m_pinst ) );

    AssertRTL( TrxCmp( trxMax, trxMax ) == 0 );
    AssertRTL( TrxCmp( trxMax, 0 ) > 0 );
    AssertRTL( TrxCmp( trxMax, 2 ) > 0 );
    AssertRTL( TrxCmp( trxMax, 2000000 ) > 0 );
    AssertRTL( TrxCmp( trxMax, trxMax - 1 ) > 0 );
    AssertRTL( TrxCmp( 0, trxMax ) < 0 );
    AssertRTL( TrxCmp( 2, trxMax ) < 0 );
    AssertRTL( TrxCmp( 2000000, trxMax ) < 0 );
    AssertRTL( TrxCmp( trxMax - 1, trxMax ) < 0 );
    AssertRTL( TrxCmp( 0xF0000000, 0xEFFFFFFE ) > 0 );
    AssertRTL( TrxCmp( 0xEFFFFFFF, 0xF0000000 ) < 0 );
    AssertRTL( TrxCmp( 0xfffffdbc, 0x000052e8 ) < 0 );
    AssertRTL( TrxCmp( 10, trxMax - 259 ) > 0 );
    AssertRTL( TrxCmp( trxMax - 251, 16 ) < 0 );
    AssertRTL( TrxCmp( trxMax - 257, trxMax - 513 ) > 0 );
    AssertRTL( TrxCmp( trxMax - 511, trxMax - 255 ) < 0 );

    if ( m_pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperEnableMaxUse, fFalse ) );
    }
    CallR( m_cresBucket.ErrInit( JET_residVERBUCKET ) );
    CallS( ErrRESGetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperSize, &m_cbBucket ) );

    Assert( pbucketNil == m_pbucketGlobalHead );
    Assert( pbucketNil == m_pbucketGlobalTail );
    m_pbucketGlobalHead = pbucketNil;
    m_pbucketGlobalTail = pbucketNil;

    Assert( ppibNil == m_ppibRCEClean );
    Assert( ppibNil == m_ppibRCECleanCallback );

    CallJ( ErrPIBBeginSession( m_pinst, &m_ppibRCEClean, m_pinst->FRecovering() ? procidRCEClean : procidNil, fFalse ), DeleteHash );
    CallJ( m_ppibRCEClean->ErrSetOperationContext( &opContext, sizeof( opContext ) ), EndCleanSession );
    
    CallJ( ErrPIBBeginSession( m_pinst, &m_ppibRCECleanCallback, m_pinst->FRecovering() ? procidRCECleanCallback : procidNil, fFalse ), EndCleanSession );
    CallJ( m_ppibRCECleanCallback->ErrSetOperationContext( &opContext, sizeof( opContext ) ), EndCleanCallbackSession );

    m_ppibRCEClean->grbitCommitDefault = JET_bitCommitLazyFlush;
    m_ppibRCECleanCallback->SetFSystemCallback();

    m_fSyncronousTasks = fFalse;

    m_fVERCleanUpWait = 0;

    for ( size_t ircehead = 0; ircehead < m_crceheadHashTable; ircehead++ )
    {
        new( &m_rgrceheadHashTable[ ircehead ] ) VER::RCEHEAD;
    }
    m_frceHashTableInited = fTrue;

    return err;

EndCleanCallbackSession:
    PIBEndSession( m_ppibRCECleanCallback );

EndCleanSession:
    PIBEndSession( m_ppibRCEClean );

DeleteHash:

    m_cresBucket.Term();
    if ( m_pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperEnableMaxUse, fTrue ) );
    }
    return err;
}


VOID VER::VERTerm( BOOL fNormal )
{
    Assert ( m_fSyncronousTasks );

    m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
    LONG fStatus = AtomicExchange( (LONG *)&m_fVERCleanUpWait, 2 );
    m_critRCEClean.Leave();
    if ( 1 == fStatus )
    {
        m_asigRCECleanDone.Wait();
    }

    if ( fNormal )
    {
        Assert( trxMax == TrxOldest( m_pinst ) );
        ERR err = ErrVERRCEClean( );
        Assert( err != JET_wrnRemainingVersions );
        if ( err < JET_errSuccess )
        {
            fNormal = fFalse;
            FireWall( OSFormat( "RceCleanErrOnVerTerm:%d", err ) );
        }
    }

    if ( ppibNil != m_ppibRCEClean )
    {
#ifdef DEBUG
        Assert( pfucbNil == m_ppibRCEClean->pfucbOfSession || m_pinst->FInstanceUnavailable() );

        const PIB* ppibT = m_pinst->m_ppibGlobal;
        for ( ; ppibT != m_ppibRCEClean && ppibT != ppibNil; ppibT = ppibT->ppibNext );
        Assert( ppibT == m_ppibRCEClean );
#endif

        m_ppibRCEClean = ppibNil;
    }

    if ( ppibNil != m_ppibRCECleanCallback )
    {
#ifdef DEBUG
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            Assert( !FPIBUserOpenedDatabase( m_ppibRCECleanCallback, dbid ) );
        }

        Assert( pfucbNil == m_ppibRCECleanCallback->pfucbOfSession || m_pinst->FInstanceUnavailable() );
        
        const PIB* ppibT = m_pinst->m_ppibGlobal;
        for ( ; ppibT != m_ppibRCECleanCallback && ppibT != ppibNil; ppibT = ppibT->ppibNext );
        Assert( ppibT == m_ppibRCECleanCallback );
#endif


        m_ppibRCECleanCallback = ppibNil;
    }

    Assert( pbucketNil == m_pbucketGlobalHead || !fNormal);
    Assert( pbucketNil == m_pbucketGlobalTail || !fNormal );

    BUCKET* pbucket = m_pbucketGlobalHead;
    while ( pbucketNil != pbucket )
    {
        BUCKET* const pbucketPrev = pbucket->hdr.pbucketPrev;
        delete pbucket;
        pbucket = pbucketPrev;
    }

    m_pbucketGlobalHead = pbucketNil;
    m_pbucketGlobalTail = pbucketNil;

    m_cresBucket.Term();
    if ( m_pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperEnableMaxUse, fTrue ) );
    }

    if ( m_frceHashTableInited )
    {
        for ( size_t ircehead = 0; ircehead < m_crceheadHashTable; ircehead++ )
        {
            m_rgrceheadHashTable[ ircehead ].~RCEHEAD();
        }
    }
    m_frceHashTableInited = fFalse;

    PERFOpt( cVERcbucketAllocated.Clear( m_pinst ) );
    PERFOpt( cVERcbucketDeleteAllocated.Clear( m_pinst ) );
    PERFOpt( cVERBucketAllocWaitForRCEClean.Clear( m_pinst ) );
    PERFOpt( cVERcrceHashEntries.Clear( m_pinst ) );
    PERFOpt( cVERcbBookmarkTotal.Clear( m_pinst ) );
    PERFOpt( cVERUnnecessaryCalls.Clear( m_pinst ) );
    PERFOpt( cVERSyncCleanupDispatched.Clear( m_pinst ) );
    PERFOpt( cVERAsyncCleanupDispatched.Clear( m_pinst ) );
    PERFOpt( cVERCleanupDiscarded.Clear( m_pinst ) );
    PERFOpt( cVERCleanupFailed.Clear( m_pinst ) );
}


ERR VER::ErrVERStatus( )
{
    ERR             err             = JET_errSuccess;
    DWORD_PTR       cbucketMost     = 0;
    DWORD_PTR       cbucket         = 0;

    ENTERCRITICALSECTION    enterCritRCEBucketGlobal( &m_critBucketGlobal );

    CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
    CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucket ) );

    err = ( cbucket > ( min( cbucketMost, UlParam( m_pinst, JET_paramPreferredVerPages ) ) * 0.6 ) ? ErrERRCheck( JET_wrnIdleFull ) : JET_errSuccess );

    return err;
}


VS VsVERCheck( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    const UINT          uiHash  = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    ENTERREADERWRITERLOCK enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );
    const RCE * const   prce    = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

    VS vs = vsNone;

    if ( prceNil == prce )
    {
        PERFOpt( cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) ) );
        if ( FFUCBUpdatable( pfucb ) )
        {
            NDDeferResetNodeVersion( pcsr );
        }
        vs = vsCommitted;
    }
    else if ( prce->TrxCommitted() != trxMax )
    {
        vs = vsCommitted;
    }
    else if ( prce->Pfucb()->ppib != pfucb->ppib )
    {
        vs = vsUncommittedByOther;
    }
    else
    {
        vs = vsUncommittedByCaller;
    }

    Assert( vsNone != vs );
    return vs;
}


ERR ErrVERAccessNode( FUCB * pfucb, const BOOKMARK& bookmark, NS * pns )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    Assert( !FPIBDirty( pfucb->ppib ) );
    Assert( FPIBVersion( pfucb->ppib ) );
    Assert( !FFUCBUnique( pfucb ) || 0 == bookmark.data.Cb() );
    Assert( Pcsr( pfucb )->FLatched() );



    const UINT uiHash = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

    if ( prceNil == PverFromIfmp( pfucb->ifmp )->GetChain( uiHash ) )
    {
        PERFOpt( cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) ) );

        if ( FFUCBUpdatable( pfucb ) )
        {
            NDDeferResetNodeVersion( Pcsr( const_cast<FUCB *>( pfucb ) ) );
        }

        *pns = nsDatabase;
        return JET_errSuccess;
    }


    ERR err = JET_errSuccess;

    const TRX           trxSession  = TrxVERISession( pfucb );
    ENTERREADERWRITERLOCK enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    const RCE           *prce       = PrceFirstVersion( uiHash, pfucb, bookmark );

    NS nsStatus = nsNone;
    if ( prceNil == prce )
    {
        PERFOpt( cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) ) );
        if ( FFUCBUpdatable( pfucb ) )
        {
            NDDeferResetNodeVersion( Pcsr( const_cast<FUCB *>( pfucb ) ) );
        }
        nsStatus = nsDatabase;
    }
    else if ( prce->TrxCommitted() == trxMax &&
              prce->Pfucb()->ppib == pfucb->ppib )
    {
        Assert( !prce->FRolledBack() );

        nsStatus = nsUncommittedVerInDB;
    }
    else if ( TrxCmp( prce->TrxCommitted(), trxSession ) < 0 )
    {
        nsStatus = nsCommittedVerInDB;
    }
    else
    {
        Assert( prceNil != prce );

        Assert( TrxCmp( prce->TrxCommitted(), trxSession ) >= 0
            || trxMax == trxSession );

        const RCE * prceLastNonDelta = prce;
        const RCE * prceLastReplace  = prceNil;
        for ( ; prceNil != prce && TrxCmp( prce->TrxCommitted(), trxSession ) >= 0; prce = prce->PrcePrevOfNode() )
        {
            if ( !prce->FRolledBack() )
            {
                switch( prce->Oper() )
                {
                    case operDelta:
                    case operDelta64:
                    case operReadLock:
                    case operWriteLock:
                        break;
                    case operReplace:
                        prceLastReplace = prce;
                    default:
                        prceLastNonDelta = prce;
                        break;
                }
            }
        }

        prce = prceLastNonDelta;

        switch( prce->Oper() )
        {
            case operReplace:
            case operFlagDelete:
                nsStatus = ( prce->FRolledBack() ? nsDatabase : nsVersionedUpdate );
                break;
            case operInsert:
                nsStatus = nsVersionedInsert;
                break;
            case operDelta:
            case operDelta64:
            case operReadLock:
            case operWriteLock:
            case operPreInsert:
                nsStatus = nsDatabase;
                break;
            default:
                AssertSz( fFalse, "Illegal operation in RCE chain" );
                break;
        }

        if ( prceNil != prceLastReplace && nsVersionedInsert != nsStatus )
        {
            Assert( prceLastReplace->CbData() >= cbReplaceRCEOverhead );
            Assert( !prceLastReplace->FRolledBack() );

            pfucb->kdfCurr.key.prefix.Nullify();
            pfucb->kdfCurr.key.suffix.SetPv( const_cast<BYTE *>( prceLastReplace->PbBookmark() ) );
            pfucb->kdfCurr.key.suffix.SetCb( prceLastReplace->CbBookmarkKey() );
            pfucb->kdfCurr.data.SetPv( const_cast<BYTE *>( prceLastReplace->PbData() ) + cbReplaceRCEOverhead );
            pfucb->kdfCurr.data.SetCb( prceLastReplace->CbData() - cbReplaceRCEOverhead );

            if ( 0 == pfucb->ppib->Level() )
            {

                Assert( Pcsr( pfucb )->Latch() == latchReadTouch
                        || Pcsr( pfucb )->Latch() == latchReadNoTouch );

                const INT cbRecord = pfucb->kdfCurr.key.Cb() + pfucb->kdfCurr.data.Cb();
                if ( NULL != pfucb->pvRCEBuffer )
                {
                    OSMemoryHeapFree( pfucb->pvRCEBuffer );
                }
                pfucb->pvRCEBuffer = PvOSMemoryHeapAlloc( cbRecord );
                if ( NULL == pfucb->pvRCEBuffer )
                {
                    Call( ErrERRCheck( JET_errOutOfMemory ) );
                }
                Assert( 0 == pfucb->kdfCurr.key.prefix.Cb() );
                memcpy( pfucb->pvRCEBuffer,
                        pfucb->kdfCurr.key.suffix.Pv(),
                        pfucb->kdfCurr.key.suffix.Cb() );
                memcpy( (BYTE *)(pfucb->pvRCEBuffer) + pfucb->kdfCurr.key.suffix.Cb(),
                        pfucb->kdfCurr.data.Pv(),
                        pfucb->kdfCurr.data.Cb() );
                pfucb->kdfCurr.key.suffix.SetPv( pfucb->pvRCEBuffer );
                pfucb->kdfCurr.data.SetPv( (BYTE *)pfucb->pvRCEBuffer + pfucb->kdfCurr.key.suffix.Cb() );
            }
            ASSERT_VALID( &(pfucb->kdfCurr) );
        }
    }
    Assert( nsNone != nsStatus );

HandleError:

    *pns = nsStatus;

    Assert( JET_errSuccess == err || 0 == pfucb->ppib->Level() );
    return err;
}


LOCAL BOOL FUpdateIsActive( const PIB * const ppib, const UPDATEID& updateid )
{
    BOOL fUpdateIsActive = fFalse;

    const FUCB * pfucb;
    for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
    {
        if( FFUCBReplacePrepared( pfucb ) && pfucb->updateid == updateid )
        {
            fUpdateIsActive = fTrue;
            break;
        }
    }

    return fUpdateIsActive;
}


template< typename TDelta >
TDelta DeltaVERGetDelta( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    TDelta tDelta       = 0;

    if( FPIBDirty( pfucb->ppib ) )
    {
        Assert( 0 == tDelta );
    }
    else
    {
        const TRX               trxSession  = TrxVERISession( pfucb );
        const UINT              uiHash      = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
        ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );

        const RCE               *prce       = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
        for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
        {
            if ( operReplace == prce->Oper() )
            {
                if ( prce->FActiveNotByMe( pfucb->ppib, trxSession ) )
                {
                    tDelta = 0;

                }
            }
            else if ( _VERDELTA< TDelta >::TRAITS::oper == prce->Oper() )
            {
                Assert( !prce->FRolledBack() );

                const _VERDELTA< TDelta >* const pverdelta = ( _VERDELTA< TDelta >* )prce->PbData();
                if ( pverdelta->cbOffset == cbOffset
                    &&  ( prce->FActiveNotByMe( pfucb->ppib, trxSession )
                        || (    trxMax == prce->TrxCommitted()
                                && prce->Pfucb()->ppib == pfucb->ppib
                                && FUpdateIsActive( prce->Pfucb()->ppib, prce->Updateid() ) ) ) )
                {
                    tDelta -= pverdelta->tDelta;
                }
            }
        }
    }

    return tDelta;
}


BOOL FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );
    Assert( 0 == cbOffset );

    const TRX               trxSession      = pfucb->ppib->trxBegin0;
    Assert( trxMax != trxSession );

    const UINT              uiHash          = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    BOOL                    fVersionExists  = fFalse;

    const RCE               *prce   = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    for ( ; prceNil != prce; prce = prce->PrcePrevOfNode() )
    {
        const VERDELTA32* const pverdelta = ( VERDELTA32* )prce->PbData();
        const VERDELTA64* const pverdelta64 = ( VERDELTA64* )prce->PbData();
        if ( ( operDelta == prce->Oper() && pverdelta->cbOffset == cbOffset && pverdelta->tDelta != 0 )
            || ( operDelta64 == prce->Oper() && pverdelta64->cbOffset == cbOffset && pverdelta64->tDelta != 0 ) )
        {
            const TRX   trxCommitted    = prce->TrxCommitted();
            if ( ( trxMax == trxCommitted
                    && prce->Pfucb()->ppib != pfucb->ppib )
                || ( trxMax != trxCommitted
                    && TrxCmp( trxCommitted, trxSession ) > 0 ) )
            {
                fVersionExists = fTrue;
                break;
            }
        }
    }

    return fVersionExists;
}


INLINE BOOL FVERIGetReplaceInRangeByUs(
    const PIB       *ppib,
    const RCE       *prceLastBeforeEndOfRange,
    const RCEID     rceidFirst,
    const RCEID     rceidLast,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    const BOOL      fFindLastReplace,
    const RCE       **pprceReplaceInRange )
{
    const RCE       *prce;
    BOOL            fSawCommittedByUs = fFalse;
    BOOL            fSawUncommittedByUs = fFalse;

    Assert( prceNil != prceLastBeforeEndOfRange );
    Assert( trxCommitted == trxMax ? ppibNil != ppib : ppibNil == ppib );

    Assert( PverFromIfmp( prceLastBeforeEndOfRange->Ifmp() )->RwlRCEChain( prceLastBeforeEndOfRange->UiHash() ).FReader() );

    *pprceReplaceInRange = prceNil;

    Assert( rceidNull != rceidLast );
    Assert( RceidCmp( rceidFirst, rceidLast ) < 0 );
    if ( rceidNull == rceidFirst )
    {
        return fFalse;
    }

    const RCE   *prceFirstReplaceInRange = prceNil;
    for ( prce = prceLastBeforeEndOfRange;
        prceNil != prce && RceidCmp( rceidFirst, prce->Rceid() ) < 0;
        prce = prce->PrcePrevOfNode() )
    {
        Assert( RceidCmp( prce->Rceid(), rceidLast ) < 0 );
        if ( prce->FOperReplace() )
        {
            if ( fSawCommittedByUs )
            {
                Assert( !fFindLastReplace );

                Assert( trxMax != trxCommitted );
                Assert( ppibNil == ppib );

                if ( fSawUncommittedByUs )
                {
                    Assert( prce->TrxCommitted() == trxMax );
                    Assert( prce->TrxBegin0() == trxBegin0 );
                }
                else if ( prce->TrxCommitted() == trxMax )
                {
                    Assert( prce->TrxBegin0() == trxBegin0 );
                    fSawUncommittedByUs = fTrue;
                }
                else
                {
                    Assert( prce->TrxCommitted() == trxCommitted );
                }
            }

            else if ( fSawUncommittedByUs )
            {
                Assert( !fFindLastReplace );

                Assert( prce->TrxCommitted() == trxMax );
                Assert( prce->TrxBegin0() == trxBegin0 );
                if ( trxMax == trxCommitted )
                {
                    Assert( ppibNil != ppib );
                    Assert( prce->Pfucb()->ppib == ppib );
                }
            }
            else
            {
                if ( prce->TrxCommitted() == trxMax )
                {
                    Assert( ppibNil != prce->Pfucb()->ppib );
                    if ( prce->TrxBegin0() != trxBegin0 )
                    {
                        Assert( trxCommitted != trxMax
                            || prce->Pfucb()->ppib != ppib );
                        return fFalse;
                    }

                    Assert( trxCommitted != trxMax
                        || prce->Pfucb()->ppib == ppib );

                    fSawUncommittedByUs = fTrue;
                }
                else if ( prce->TrxCommitted() == trxCommitted )
                {
                    Assert( trxMax != trxCommitted );
                    Assert( ppibNil == ppib );
                    fSawCommittedByUs = fTrue;
                }
                else
                {
                    Assert( prce->TrxBegin0() != trxBegin0 );
                    return fFalse;
                }

                if ( fFindLastReplace )
                {
                    return fTrue;
                }
            }

            prceFirstReplaceInRange = prce;
        }
    }
    Assert( prceNil == prce || RceidCmp( rceidFirst, prce->Rceid() ) >= 0 );

    if ( prceNil != prceFirstReplaceInRange )
    {
        *pprceReplaceInRange = prceFirstReplaceInRange;
        return fTrue;
    }

    return fFalse;
}


#ifdef DEBUG
LOCAL VOID VERDBGCheckReplaceByOthers(
    const RCE   * const prce,
    const PIB   * const ppib,
    const TRX   trxCommitted,
    const RCE   * const prceFirstActiveReplaceByOther,
    BOOL        *pfFoundUncommitted,
    BOOL        *pfFoundCommitted )
{
    if ( prce->TrxCommitted() == trxMax )
    {
        Assert( ppibNil != prce->Pfucb()->ppib );
        Assert( ppib != prce->Pfucb()->ppib );

        if ( *pfFoundUncommitted )
        {
            Assert( prceNil != prceFirstActiveReplaceByOther );
            Assert( prceFirstActiveReplaceByOther->TrxCommitted() == trxMax || prceFirstActiveReplaceByOther->TrxCommitted() == prce->TrxCommitted() );
            Assert( prceFirstActiveReplaceByOther->TrxBegin0() == prce->TrxBegin0() );
            Assert( prceFirstActiveReplaceByOther->Pfucb()->ppib == prce->Pfucb()->ppib );
        }
        else
        {
            if ( *pfFoundCommitted )
            {
                Assert( prceNil != prceFirstActiveReplaceByOther );
                Assert( prceFirstActiveReplaceByOther->TrxBegin0() == prce->TrxBegin0() );
            }
            else
            {
                Assert( prceNil == prceFirstActiveReplaceByOther );
            }

            *pfFoundUncommitted = fTrue;
        }
    }
    else
    {
        Assert( prce->TrxCommitted() != trxCommitted );

        if ( !*pfFoundCommitted )
        {
            if ( *pfFoundUncommitted )
            {
                Assert( prceNil != prceFirstActiveReplaceByOther );
                Assert( prceFirstActiveReplaceByOther->TrxCommitted() == trxMax || prceFirstActiveReplaceByOther->TrxCommitted() == prce->TrxCommitted() );
                Assert( RceidCmp( prce->Rceid(), prceFirstActiveReplaceByOther->Rceid() ) < 0 );
                Assert( prceFirstActiveReplaceByOther->TrxCommitted() != trxMax || TrxCmp( prce->TrxCommitted(), prceFirstActiveReplaceByOther->TrxBegin0() ) < 0 );
            }

            const RCE   * prceT;
            for ( prceT = prce; prceNil != prceT; prceT = prceT->PrcePrevOfNode() )
            {
                if ( prceT->TrxCommitted() == trxMax )
                {
                    Assert( !*pfFoundUncommitted );
                    Assert( prceT->TrxBegin0() == prce->TrxBegin0() );
                }
            }

            *pfFoundCommitted = fTrue;
        }
    }
}
#endif


BOOL FVERGetReplaceImage(
    const PIB       *ppib,
    const IFMP      ifmp,
    const PGNO      pgnoLVFDP,
    const BOOKMARK& bookmark,
    const RCEID     rceidFirst,
    const RCEID     rceidLast,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    const BOOL      fAfterImage,
    const BYTE      **ppb,
    ULONG           * const pcbActual
    )
{
    const UINT  uiHash              = UiRCHashFunc( ifmp, pgnoLVFDP, bookmark );
    ENTERREADERWRITERLOCK enterRwlHashAsReader( &( PverFromIfmp( ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    const RCE   *prce               = PrceRCEGet( uiHash, ifmp, pgnoLVFDP, bookmark );
    const RCE   *prceDesiredImage   = prceNil;
    const RCE   *prceFirstAfterRange= prceNil;

    Assert( trxMax != trxBegin0 );

    while ( prceNil != prce && RceidCmp( prce->Rceid(), rceidLast ) >= 0 )
    {
        prceFirstAfterRange = prce;
        prce = prce->PrcePrevOfNode();
    }

    const RCE   * const prceLastBeforeEndOfRange    = prce;

    if ( prceNil == prceLastBeforeEndOfRange )
    {
        Assert( prceNil == prceDesiredImage );
    }
    else
    {
        Assert( prceNil == prceFirstAfterRange
            || RceidCmp( prceFirstAfterRange->Rceid(), rceidLast ) >= 0 );
        Assert( RceidCmp( prceLastBeforeEndOfRange->Rceid(), rceidLast ) < 0 );
        Assert( prceFirstAfterRange == prceLastBeforeEndOfRange->PrceNextOfNode() );

        const BOOL fReplaceInRangeByUs = FVERIGetReplaceInRangeByUs(
                                                    ppib,
                                                    prceLastBeforeEndOfRange,
                                                    rceidFirst,
                                                    rceidLast,
                                                    trxBegin0,
                                                    trxCommitted,
                                                    fAfterImage,
                                                    &prceDesiredImage );
        if ( fReplaceInRangeByUs )
        {
            if ( fAfterImage )
            {
                Assert( prceNil == prceDesiredImage );
                Assert( prceNil != prceLastBeforeEndOfRange );
                Assert( prceFirstAfterRange == prceLastBeforeEndOfRange->PrceNextOfNode() );
            }
            else
            {
                Assert( prceNil != prceDesiredImage );
            }
        }

        else if ( prceLastBeforeEndOfRange->TrxBegin0() == trxBegin0 )
        {

            if ( prceLastBeforeEndOfRange->TrxCommitted() != trxMax )
            {
                Assert( prceLastBeforeEndOfRange->TrxCommitted() == trxCommitted );
                Assert( ppibNil == ppib );
            }
            else if ( trxCommitted == trxMax )
            {
                Assert( ppibNil != ppib );
                Assert( ppib == prceLastBeforeEndOfRange->Pfucb()->ppib );
                Assert( trxBegin0 == ppib->trxBegin0 );
            }
            else
            {
            }

            Assert( prceNil == prceDesiredImage );
        }

        else
        {
            const RCE   *prceFirstActiveReplaceByOther = prceNil;
#ifdef DEBUG
            BOOL        fFoundUncommitted = fFalse;
            BOOL        fFoundCommitted = fFalse;
#endif

            Assert( prceNil == prceDesiredImage );
            Assert( prceFirstAfterRange == prceLastBeforeEndOfRange->PrceNextOfNode() );

            for ( prce = prceLastBeforeEndOfRange;
                prceNil != prce;
                prce = prce->PrcePrevOfNode() )
            {
                Assert( RceidCmp( prce->Rceid(), rceidLast ) < 0 );
                Assert( prce->TrxBegin0() != trxBegin0 || operDelta == prce->Oper() || operDelta64 == prce->Oper() );

                if ( TrxCmp( prce->TrxCommitted(), trxBegin0 ) < 0 )
                    break;

                if ( prce->FOperReplace() )
                {
#ifdef DEBUG
                    VERDBGCheckReplaceByOthers(
                                prce,
                                ppib,
                                trxCommitted,
                                prceFirstActiveReplaceByOther,
                                &fFoundUncommitted,
                                &fFoundCommitted );
#endif

                    prceFirstActiveReplaceByOther = prce;
                }

            }


            Assert( prceNil == prceDesiredImage );
            if ( prceNil != prceFirstActiveReplaceByOther )
                prceDesiredImage = prceFirstActiveReplaceByOther;
        }
    }

    if ( prceNil == prceDesiredImage )
    {
        for ( prce = prceFirstAfterRange;
            prceNil != prce;
            prce = prce->PrceNextOfNode() )
        {
            Assert( RceidCmp( prce->Rceid(), rceidLast ) >= 0 );
            if ( prce->FOperReplace() )
            {
                prceDesiredImage = prce;
                break;
            }
        }
    }

    if ( prceNil != prceDesiredImage )
    {
        const VERREPLACE* pverreplace = (VERREPLACE*)prceDesiredImage->PbData();
        *pcbActual  = prceDesiredImage->CbData() - cbReplaceRCEOverhead;
        *ppb        = (BYTE *)pverreplace->rgbBeforeImage;
        return fTrue;
    }

    return fFalse;
}


ERR VER::ErrVERICreateDMLRCE(
    FUCB            * pfucb,
    UPDATEID        updateid,
    const BOOKMARK& bookmark,
    const UINT      uiHash,
    const OPER      oper,
    const LEVEL     level,
    const BOOL      fProxy,
    RCE             **pprce,
    RCEID           rceid
    )
{
    Assert( pfucb->ppib->Level() > 0 );
    Assert( level > 0 );
    Assert( pfucb->u.pfcb != pfcbNil );
    Assert( FOperInHashTable( oper ) );

    ERR     err     = JET_errSuccess;
    RCE     *prce   = prceNil;

    const INT cbBookmark = bookmark.key.Cb() + bookmark.data.Cb();

    INT cbNewRCE = sizeof( RCE ) + cbBookmark;
    switch( oper )
    {
        case operReplace:
            Assert( !pfucb->kdfCurr.data.FNull() );
            cbNewRCE += cbReplaceRCEOverhead + pfucb->kdfCurr.data.Cb();
            break;
        case operDelta:
            cbNewRCE += sizeof( VERDELTA32 );
            break;
        case operDelta64:
            cbNewRCE += sizeof( VERDELTA64 );
            break;
        case operInsert:
        case operFlagDelete:
        case operReadLock:
        case operWriteLock:
        case operPreInsert:
            break;
        default:
            Assert( fFalse );
            break;
    }

    Assert( trxMax == pfucb->ppib->trxCommit0 );
    Call( ErrVERICreateRCE(
            cbNewRCE,
            pfucb->u.pfcb,
            pfucb,
            updateid,
            pfucb->ppib->trxBegin0,
            &pfucb->ppib->trxCommit0,
            level,
            bookmark.key.Cb(),
            bookmark.data.Cb(),
            oper,
            uiHash,
            &prce,
            fProxy,
            rceid
            ) );

    if ( FOperConcurrent( oper ) )
    {
        Assert( RwlRCEChain( uiHash ).FWriter() );
    }
    else
    {
        Assert( RwlRCEChain( uiHash ).FNotWriter() );
    }

    prce->CopyBookmark( bookmark );

    Assert( pgnoNull == prce->PgnoUndoInfo( ) );
    Assert( prce->Oper() != operAllocExt );
    Assert( !prce->FOperDDL() );

    FUCBSetVersioned( pfucb );

    CallS( err );

HandleError:
    if ( pprce )
    {
        *pprce = prce;
    }

    return err;
}


LOCAL VOID VERISetupInsertedDMLRCE( const FUCB * pfucb, RCE * prce )
{
    Assert( prce->TrxCommitted() == trxMax );
    switch( prce->Oper() )
    {
        case operReplace:
        {
            Assert( prce->CbData() >= cbReplaceRCEOverhead );

            VERREPLACE* const pverreplace   = (VERREPLACE*)prce->PbData();

            if ( pfucb->fUpdateSeparateLV )
            {
                pverreplace->rceidBeginReplace = pfucb->rceidBeginUpdate;
            }
            else
            {
                pverreplace->rceidBeginReplace = rceidNull;
            }

            const RCE * const prcePrevReplace = PrceVERIGetPrevReplace( prce );
            if ( prceNil != prcePrevReplace )
            {
                Assert( !prcePrevReplace->FRolledBack() );
                const VERREPLACE* const pverreplacePrev = (VERREPLACE*)prcePrevReplace->PbData();
                Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() && fRecoveringUndo != PinstFromIfmp( pfucb->ifmp )->m_plog->FRecoveringMode() ||
                        pverreplacePrev->cbMaxSize >= (SHORT)pfucb->kdfCurr.data.Cb() );
                pverreplace->cbMaxSize = pverreplacePrev->cbMaxSize;
            }
            else
            {
                pverreplace->cbMaxSize = (SHORT)pfucb->kdfCurr.data.Cb();
            }

            pverreplace->cbDelta = 0;

            Assert( prce->Oper() == operReplace );

            UtilMemCpy( pverreplace->rgbBeforeImage, pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );
        }
            break;

        case operDelta:
        {
            Assert( sizeof( VERDELTA32 ) == pfucb->kdfCurr.data.Cb() );
            *( VERDELTA32* )prce->PbData() = *( VERDELTA32* )pfucb->kdfCurr.data.Pv();
        }
            break;

        case operDelta64:
        {
            Assert( sizeof( VERDELTA64 ) == pfucb->kdfCurr.data.Cb() );
            *(VERDELTA64*) prce->PbData() = *(VERDELTA64*) pfucb->kdfCurr.data.Pv();
        }
            break;

        default:
            break;
    }
}


LOCAL BOOL FVERIWriteConflict(
    FUCB*           pfucb,
    const BOOKMARK& bookmark,
    UINT            uiHash,
    const OPER      oper
    )
{
    BOOL            fWriteConflict  = fFalse;
    const TRX       trxSession      = TrxVERISession( pfucb );
    RCE*            prce            = PrceRCEGet(
                                            uiHash,
                                            pfucb->ifmp,
                                            pfucb->u.pfcb->PgnoFDP(),
                                            bookmark );

    Assert( trxSession != trxMax );

    if ( prce != prceNil )
    {
        if ( prce->FActiveNotByMe( pfucb->ppib, trxSession ) )
        {
            if ( operReadLock == oper || operDelta == oper || operDelta64 == oper )
            {
                const RCE   * prceT         = prce;
                for ( ;
                    prceNil != prceT && TrxCmp( prceT->TrxCommitted(), trxSession ) > 0;
                    prceT = prceT->PrcePrevOfNode() )
                {
                    if ( prceT->Oper() != oper )
                    {
                        fWriteConflict = fTrue;
                        break;
                    }
                }
            }
            else
            {
                fWriteConflict = fTrue;
            }
        }

        else
        {
#ifdef DEBUG
            if ( prce->TrxCommitted() == trxMax )
            {
                Assert( prce->Pfucb()->ppib == pfucb->ppib );
                Assert( prce->Level() <= pfucb->ppib->Level()
                        || PinstFromIfmp( pfucb->ifmp )->FRecovering() );
            }
            else
            {
                Assert( TrxCmp( prce->TrxCommitted(), trxSession ) < 0
                    || ( prce->TrxCommitted() == trxSession && PinstFromIfmp( pfucb->ifmp )->FRecovering() ) );
            }
#endif

            if ( prce->Oper() != oper && (  operReadLock == prce->Oper()
                                            || operDelta == prce->Oper()
                                            || operDelta64 == prce->Oper() ) )
            {
                const RCE   * prceT         = prce;
                for ( ;
                     prceNil != prceT;
                     prceT = prceT->PrcePrevOfNode() )
                {
                    if ( prceT->FActiveNotByMe( pfucb->ppib, trxSession ) )
                    {
                        fWriteConflict = fTrue;
                        break;
                    }
                }

                Assert( fWriteConflict || prceNil == prceT );
            }
        }


#ifdef DEBUG
        if ( !fWriteConflict )
        {
            if ( prce->TrxCommitted() == trxMax )
            {
                Assert( prce->Pfucb()->ppib != pfucb->ppib
                    || prce->Level() <= pfucb->ppib->Level()
                    || PinstFromIfmp( prce->Pfucb()->ifmp )->FRecovering() );
            }

            if ( prce->Oper() == operFlagDelete )
            {
                Assert( operInsert == oper
                    || operPreInsert == oper
                    || operWriteLock == oper
                    || prce->FMoved()
                    || PinstFromIfmp( prce->Ifmp() )->FRecovering() );
            }
        }
#endif
    }

    return fWriteConflict;
}

BOOL FVERWriteConflict(
    FUCB            * pfucb,
    const BOOKMARK& bookmark,
    const OPER      oper )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    const UINT      uiHash  = UiRCHashFunc(
                                    pfucb->ifmp,
                                    pfucb->u.pfcb->PgnoFDP(),
                                    bookmark );
    ENTERREADERWRITERLOCK enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    return FVERIWriteConflict( pfucb, bookmark, uiHash, oper );
}


INLINE ERR VER::ErrVERModifyCommitted(
    FCB             *pfcb,
    const BOOKMARK& bookmark,
    const OPER      oper,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    RCE             **pprce
    )
{
    ASSERT_VALID( &bookmark );
    Assert( pfcb->FTypeSecondaryIndex() );
    Assert( pfcb->PfcbTable() == pfcbNil );
    Assert( trxCommitted != trxMax );

    const UINT          uiHash  = UiRCHashFunc( pfcb->Ifmp(), pfcb->PgnoFDP(), bookmark );

    ERR                 err     = JET_errSuccess;
    RCE                 *prce   = prceNil;

    Assert( NULL != pprce );
    Assert( prceNil == *pprce );

    {
        const INT cbBookmark = bookmark.key.Cb() + bookmark.data.Cb();
        const INT cbNewRCE = sizeof( RCE ) + cbBookmark;

        Call( ErrVERICreateRCE(
                cbNewRCE,
                pfcb,
                pfucbNil,
                updateidNil,
                trxBegin0,
                NULL,
                0,
                bookmark.key.Cb(),
                bookmark.data.Cb(),
                oper,
                uiHash,
                &prce,
                fTrue
                ) );
        AssertPREFIX( prce );

        prce->CopyBookmark( bookmark );

        if( !prce->FOperConcurrent() )
        {
            ENTERREADERWRITERLOCK enterRwlHashAsWriter( &( RwlRCEChain( uiHash ) ), fFalse );
            Call( ErrVERIInsertRCEIntoHash( prce ) );
            prce->SetCommittedByProxy( trxCommitted );
        }
        else
        {
            Call( ErrVERIInsertRCEIntoHash( prce ) );
            prce->SetCommittedByProxy( trxCommitted );
            RwlRCEChain( uiHash ).LeaveAsWriter();
        }

        Assert( pgnoNull == prce->PgnoUndoInfo( ) );
        Assert( prce->Oper() == operWriteLock || prce->Oper() == operFlagDelete || prce->Oper() == operPreInsert );

        *pprce = prce;

        ASSERT_VALID( *pprce );
    }


HandleError:
    Assert( err < JET_errSuccess || prceNil != *pprce );
    return err;
}

ERR VER::ErrVERCheckTransactionSize( PIB * const ppib )
{
    ERR err = JET_errSuccess;
    if ( m_fAboveMaxTransactionSize )
    {
        UpdateCachedTrxOldest( m_pinst );

        if ( ppib->trxBegin0 == TrxOldestCached( m_pinst ) )
        {
            const BOOL fCleanupWasRun   = m_msigRCECleanPerformedRecently.FWait( cmsecAsyncBackgroundCleanup );

            m_critBucketGlobal.Enter();

            VERIReportVersionStoreOOM ( ppib, fTrue , fCleanupWasRun );

            m_critBucketGlobal.Leave();

            Error( ErrERRCheck( JET_errVersionStoreOutOfMemory ) );
        }
    }

HandleError:
    return err;
}


ERR VER::ErrVERModify(
    FUCB            * pfucb,
    const BOOKMARK& bookmark,
    const OPER      oper,
    RCE             **pprce,
    const VERPROXY  * const pverproxy
    )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );
    Assert( FOperInHashTable( oper ) );
    Assert( !bookmark.key.FNull() );
    Assert( !g_rgfmp[pfucb->ifmp].FVersioningOff() );
    Assert( !pfucb->ppib->FReadOnlyTrx() );
    AssertTrack( !g_rgfmp[pfucb->ifmp].FShrinkIsRunning(), "VerCreateDuringShrink" );

    Assert( NULL != pprce );
    *pprce = prceNil;

    ERR         err             = JET_errSuccess;
    BOOL        fRCECreated     = fFalse;

    UPDATEID    updateid        = updateidNil;
    RCEID       rceid           = rceidNull;
    LEVEL       level;
    RCE         *prcePrimary    = prceNil;
    FUCB        *pfucbProxy     = pfucbNil;
    const BOOL  fProxy          = ( NULL != pverproxy );

    Assert( m_pinst->m_plog->FRecovering() || operInsert != oper );

    Assert( !m_pinst->m_plog->FRecovering() || ( fProxy && proxyRedo == pverproxy->proxy ) );
    if ( fProxy )
    {
        if ( proxyCreateIndex == pverproxy->proxy )
        {
            Assert( !m_pinst->m_plog->FRecovering() );
            Assert( oper == operWriteLock
                || oper == operPreInsert
                || oper == operReplace
                || oper == operFlagDelete );
            Assert( prceNil != pverproxy->prcePrimary );
            prcePrimary = pverproxy->prcePrimary;

            if ( pverproxy->prcePrimary->TrxCommitted() != trxMax )
            {
                err = ErrVERModifyCommitted(
                            pfucb->u.pfcb,
                            bookmark,
                            oper,
                            prcePrimary->TrxBegin0(),
                            prcePrimary->TrxCommitted(),
                            pprce );
                return err;
            }
            else
            {
                Assert( PinstFromPpib( prcePrimary->Pfucb()->ppib )->RwlTrx( prcePrimary->Pfucb()->ppib ).FWriter() );

                level = prcePrimary->Level();

                CallR( ErrDIROpenByProxy(
                            prcePrimary->Pfucb()->ppib,
                            pfucb->u.pfcb,
                            &pfucbProxy,
                            level ) );
                Assert( pfucbNil != pfucbProxy );

                Assert( pfucbProxy->ppib->Level() > 0 );
                FUCBSetVersioned( pfucbProxy );

                pfucb = pfucbProxy;
            }
        }
        else
        {
            Assert( proxyRedo == pverproxy->proxy );
            Assert( m_pinst->m_plog->FRecovering() );
            Assert( rceidNull != pverproxy->rceid );
            rceid = pverproxy->rceid;
            level = LEVEL( pverproxy->level );
        }
    }
    else
    {
        if ( FUndoableLoggedOper( oper )
            || operPreInsert == oper )
        {
            updateid = UpdateidOfPpib( pfucb->ppib );
        }
        else
        {
            Assert( updateidNil == UpdateidOfPpib( pfucb->ppib )
                || operWriteLock == oper
                || operReadLock == oper );
        }
        level = pfucb->ppib->Level();
    }

    const UINT          uiHash      = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

    Call( ErrVERICreateDMLRCE(
            pfucb,
            updateid,
            bookmark,
            uiHash,
            oper,
            level,
            fProxy,
            pprce,
            rceid ) );
    Assert( prceNil != *pprce );
    fRCECreated = fTrue;

    if ( FOperConcurrent( oper ) )
    {
        Assert( RwlRCEChain( uiHash ).FWriter() );
    }
    else
    {
        RwlRCEChain( uiHash ).EnterAsWriter();
    }

    Assert( FPIBVersion( pfucb->ppib )
        || ( prceNil != prcePrimary && prcePrimary->Pfucb()->ppib == pfucb->ppib ) );

    if ( !m_pinst->m_plog->FRecovering() && FVERIWriteConflict( pfucb, bookmark, uiHash, oper ) )
    {
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagDMLConflicts,
            OSFormat(
                "Write-conflict detected: Session=[0x%p:0x%x] performing operation [oper=0x%x] on objid=[0x%x:0x%x]",
                pfucb->ppib,
                ( ppibNil != pfucb->ppib ? pfucb->ppib->trxBegin0 : trxMax ),
                oper,
                (ULONG)pfucb->ifmp,
                pfucb->u.pfcb->ObjidFDP() ) );
        Call( ErrERRCheck( JET_errWriteConflict ) );
    }

    Call( ErrVERIInsertRCEIntoHash( *pprce ) );

    VERISetupInsertedDMLRCE( pfucb, *pprce );

#ifdef DEBUG_VER
{
    BOOKMARK bookmarkT;
    (*pprce)->GetBookmark( &bookmarkT );
    CallR( ErrCheckRCEChain(
        *PprceRCEChainGet( (*pprce)->UiHash(), (*pprce)->Ifmp(), (*pprce)->PgnoFDP(), bookmarkT ),
        (*pprce)->UiHash() ) );
}
#endif

    RwlRCEChain( uiHash ).LeaveAsWriter();

    ASSERT_VALID( *pprce );

    CallS( err );

HandleError:
    if ( err < 0 && fRCECreated )
    {
        (*pprce)->NullifyOper();
        Assert( RwlRCEChain( uiHash ).FWriter() );
        RwlRCEChain( uiHash ).LeaveAsWriter();
        *pprce = prceNil;
    }

    if ( pfucbNil != pfucbProxy )
    {
        Assert( pfucbProxy->ppib->Level() > 0 );
        Assert( FFUCBVersioned( pfucbProxy ) );
        DIRClose( pfucbProxy );
    }

    return err;
}


ERR VER::ErrVERFlag( FUCB * pfucb, OPER oper, const VOID * pv, INT cb )
{
#ifdef DEBUG
    ASSERT_VALID( pfucb );
    Assert( pfucb->ppib->Level() > 0 );
    Assert( cb >= 0 );
    Assert( !FOperInHashTable( oper ) );
    Ptls()->fAddColumn = operAddColumn == oper;
#endif

    ERR     err     = JET_errSuccess;
    RCE     *prce   = prceNil;
    FCB     *pfcb   = pfcbNil;

    if ( g_rgfmp[pfucb->ifmp].FVersioningOff() )
    {
        Assert( !g_rgfmp[pfucb->ifmp].FLogOn() );
        return JET_errSuccess;
    }

    pfcb = pfucb->u.pfcb;
    Assert( pfcb != NULL );

    Assert( trxMax == pfucb->ppib->trxCommit0 );
    Call( ErrVERICreateRCE(
            sizeof(RCE) + cb,
            pfucb->u.pfcb,
            pfucb,
            updateidNil,
            pfucb->ppib->trxBegin0,
            &pfucb->ppib->trxCommit0,
            pfucb->ppib->Level(),
            0,
            0,
            oper,
            uiHashInvalid,
            &prce
            ) );
    AssertPREFIX( prce );

    UtilMemCpy( prce->PbData(), pv, cb );

    Assert( prce->TrxCommitted() == trxMax );
    VERInsertRCEIntoLists( pfucb, pcsrNil, prce, NULL );

    ASSERT_VALID( prce );

    FUCBSetVersioned( pfucb );

HandleError:
#ifdef DEBUG
    Ptls()->fAddColumn = fFalse;
#endif

    return err;
}


VOID VERSetCbAdjust(
            CSR         *pcsr,
    const   RCE         *prce,
            INT         cbDataNew,
            INT         cbDataOld,
            UPDATEPAGE  updatepage )
{
    ASSERT_VALID( pcsr );
    Assert( pcsr->Latch() == latchWrite || fDoNotUpdatePage == updatepage );
    Assert( fDoNotUpdatePage == updatepage || pcsr->Cpage().FLeafPage() );
    Assert( prce->FOperReplace() );

    INT cbDelta = cbDataNew - cbDataOld;

    Assert( cbDelta != 0 );

    VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
    INT cbMax = pverreplace->cbMaxSize;
    Assert( pverreplace->cbMaxSize >= cbDataOld );

    if ( cbDataNew > cbMax )
    {
        Assert( cbDelta > 0 );
        pverreplace->cbMaxSize  = SHORT( cbDataNew );
        cbDelta                 = cbMax - cbDataOld;
    }

    pverreplace->cbDelta = SHORT( pverreplace->cbDelta - cbDelta );

    if ( fDoUpdatePage == updatepage )
    {
        if ( cbDelta > 0 )
        {
            pcsr->Cpage().ReclaimUncommittedFreed( cbDelta );
        }
        else if ( cbDelta < 0 )
        {
            pcsr->Cpage().AddUncommittedFreed( -cbDelta );
        }
    }
#ifdef DEBUG
    else
    {
        Assert( fDoNotUpdatePage == updatepage );
    }
#endif
}


LOCAL INT CbVERIGetNodeMax( const FUCB * pfucb, const BOOKMARK& bookmark, UINT uiHash )
{
    INT         nodeMax = 0;

    const RCE *prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    for ( ; prceNil != prce && trxMax == prce->TrxCommitted(); prce = prce->PrcePrevOfNode() )
    {
        if ( prce->FOperReplace() && !prce->FRolledBack() )
        {
            nodeMax = ((const VERREPLACE*)prce->PbData())->cbMaxSize;
            break;
        }
    }

    Assert( nodeMax >= 0 );
    return nodeMax;
}


INT CbVERGetNodeMax( const FUCB * pfucb, const BOOKMARK& bookmark )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    const UINT              uiHash  = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );
    const INT               nodeMax = CbVERIGetNodeMax( pfucb, bookmark, uiHash );

    Assert( nodeMax >= 0 );
    return nodeMax;
}


INT CbVERGetNodeReserve( const PIB * ppib, const FUCB * pfucb, const BOOKMARK& bookmark, INT cbCurrentData )
{
    Assert( ppibNil == ppib || (ASSERT_VALID( ppib ), fTrue) );
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );
    Assert( cbCurrentData >= 0 );

    const UINT              uiHash                      = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );
    const BOOL              fIgnorePIB                  = ( ppibNil == ppib );
    const RCE *             prceFirstUncommittedReplace = prceNil;
    INT                     cbNodeReserve               = 0;

    for ( const RCE * prce = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
        prceNil != prce && trxMax == prce->TrxCommitted();
        prce = prce->PrcePrevOfNode() )
    {
        ASSERT_VALID( prce );
        if ( ( fIgnorePIB || prce->Pfucb()->ppib == ppib )
            && prce->FOperReplace()
            && !prce->FRolledBack() )
        {
            const VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
            cbNodeReserve += pverreplace->cbDelta;
            if ( prceNil == prceFirstUncommittedReplace )
                prceFirstUncommittedReplace = prce;
        }
    }

    if ( prceNil != prceFirstUncommittedReplace
        && trxMax != prceFirstUncommittedReplace->TrxCommitted() )
    {
        cbNodeReserve = 0;
    }
    else if ( 0 != cbNodeReserve )
    {
        Assert( cbNodeReserve > 0 );

        Assert( cbNodeReserve == CbVERIGetNodeMax( pfucb, bookmark, uiHash ) - (INT)cbCurrentData
            || ( prceNil != prceFirstUncommittedReplace && trxMax != prceFirstUncommittedReplace->TrxCommitted() ) );
    }

    return cbNodeReserve;
}


BOOL FVERCheckUncommittedFreedSpace(
    const FUCB  * pfucb,
    CSR         * const pcsr,
    const INT   cbReq,
    const BOOL  fPermitUpdateUncFree )
{
    BOOL    fEnoughPageSpace    = fTrue;

    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );
    Assert( cbReq <= pcsr->Cpage().CbPageFree() );


    if ( PinstFromPfucb( pfucb )->FRecovering() )
    {
        if ( fPermitUpdateUncFree && pcsr->Cpage().CbUncommittedFree() != 0 )
        {

            LATCH   latchOld;
            ERR err = pcsr->ErrUpgradeToWARLatch( &latchOld );

            if ( err == JET_errSuccess )
            {
                pcsr->Cpage().SetCbUncommittedFree( 0 );
                BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy, *TraceContextScope() );
                pcsr->DowngradeFromWARLatch( latchOld );
            }
        }

        
        return fTrue;
    }


    Assert( cbReq <= pcsr->Cpage().CbPageFree() );

    Assert( pcsr->Cpage().CbUncommittedFree() >= 0 );
    Assert( pcsr->Cpage().CbPageFree() >= pcsr->Cpage().CbUncommittedFree() );

    if ( cbReq > pcsr->Cpage().CbPageFree() - pcsr->Cpage().CbUncommittedFree() )
    {
        Assert( !FFUCBSpace( pfucb ) );
        Assert( !pcsr->Cpage().FSpaceTree() );

        const INT   cbUncommittedFree = CbNDUncommittedFree( pfucb, pcsr );
        Assert( cbUncommittedFree >= 0 );

        if ( cbUncommittedFree == pcsr->Cpage().CbUncommittedFree() )
        {
            fEnoughPageSpace = fFalse;
        }
        else
        {
            if ( fPermitUpdateUncFree )
            {
                LATCH latchOld;
                if ( pcsr->ErrUpgradeToWARLatch( &latchOld ) == JET_errSuccess )
                {
                    pcsr->Cpage().SetCbUncommittedFree( cbUncommittedFree );
                    BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy, *TraceContextScope() );
                    pcsr->DowngradeFromWARLatch( latchOld );
                }
            }

            Assert( pcsr->Cpage().CbUncommittedFree() >= 0 );
            Assert( pcsr->Cpage().CbPageFree() >= pcsr->Cpage().CbUncommittedFree() );

            fEnoughPageSpace = ( cbReq <= ( pcsr->Cpage().CbPageFree() - cbUncommittedFree ) );
        }
    }

    return fEnoughPageSpace;
}




ERR RCE::ErrGetTaskForDelete( VOID ** ppvtask ) const
{
    ERR                 err     = JET_errSuccess;
    DELETERECTASK *     ptask   = NULL;


    const UINT              uiHash      = m_uiHash;
    ENTERREADERWRITERLOCK   enterRwlRCEChainAsReader( &( PverFromIfmp( Ifmp() )->RwlRCEChain( uiHash ) ), fTrue );

    if ( !FOperNull() )
    {
        Assert( operFlagDelete == Oper() || operInsert == Oper() );

        Assert( !FFMPIsTempDB( Ifmp() ) );
        if ( !Pfcb()->FDeletePending() )
        {
            BOOKMARK    bm;
            GetBookmark( &bm );

            ptask = new DELETERECTASK( PgnoFDP(), Pfcb(), Ifmp(), bm );
            if( NULL == ptask )
            {
                err = ErrERRCheck( JET_errOutOfMemory );
            }
        }
    }
    else
    {
        Assert( Oper() == ( operFlagDelete | operMaskNull ) );
    }

    *ppvtask = ptask;

    return err;
}


ERR VER::ErrVERIDelete( PIB * ppib, const RCE * const prce )
{
    ERR             err;
    DELETERECTASK * ptask;

    Assert( ppibNil != ppib );
    Assert( m_critRCEClean.FOwner() );

    if ( ppib->ErrRollbackFailure() < JET_errSuccess )
    {
        err = m_pinst->ErrInstanceUnavailableErrorCode() ?  m_pinst->ErrInstanceUnavailableErrorCode() : JET_errInstanceUnavailable;
        return( ErrERRCheck( err ) );
    }

    Assert( 0 == ppib->Level() );

    Assert( !FFMPIsTempDB( prce->Ifmp() ) );
    Assert( !m_pinst->FRecovering() || fRecoveringUndo == m_pinst->m_plog->FRecoveringMode() );

    CallR( prce->ErrGetTaskForDelete( (VOID **)&ptask ) );

    if( NULL == ptask )
    {
        CallS( err );
    }

    else if ( m_fSyncronousTasks
        || g_rgfmp[prce->Ifmp()].FDetachingDB()
        || m_pinst->Taskmgr().CPostedTasks() > UlParam( m_pinst, JET_paramVersionStoreTaskQueueMax ) )
    {
        IncrementCSyncCleanupDispatched();
        TASK::Dispatch( m_ppibRCECleanCallback, (ULONG_PTR)ptask );
        CallS( err );
    }

    else
    {
        IncrementCAsyncCleanupDispatched();
        err = m_rectaskbatcher.ErrPost( ptask );
    }

    return err;
}


LOCAL VOID VERIFreeExt( PIB * const ppib, FCB *pfcb, PGNO pgnoFirst, CPG cpg )
{
    Assert( pfcb );

    ERR     err;
    FUCB    *pfucb = pfucbNil;

    Assert( !PinstFromPpib( ppib )->m_plog->FRecovering() );
    Assert( ppib->Level() > 0 );

    err = ErrDBOpenDatabaseByIfmp( ppib, pfcb->Ifmp() );
    if ( err < 0 )
        return;

    Call( ErrBTOpen( ppib, pfcb, &pfucb ) );

    Assert( !FFUCBSpace( pfucb ) );

    const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

    (VOID)ErrSPFreeExt( pfucb, pgnoFirst, cpg, "VerFreeExt" );
    
    FOSSetCleanupState( fCleanUpStateSavedSavedSaved );

HandleError:
    if ( pfucbNil != pfucb )
    {
        Assert( !FFUCBDeferClosed( pfucb ) );
        FUCBAssertNoSearchKey( pfucb );
        Assert( !FFUCBCurrentSecondary( pfucb ) );
        BTClose( pfucb );
    }

    (VOID)ErrDBCloseDatabase( ppib, pfcb->Ifmp(), NO_GRBIT );
}


#ifdef DEBUG
BOOL FIsRCECleanup()
{
    return Ptls()->fIsRCECleanup;
}

BOOL FInCritBucket( VER *pver )
{
    return pver->m_critBucketGlobal.FOwner();
}
#endif


BOOL FPIBSessionRCEClean( PIB *ppib )
{
    Assert( ppibNil != ppib );
    return ( (PverFromPpib( ppib ))->m_ppibRCEClean == ppib );
}


INLINE VOID VERIUnlinkDefunctSecondaryIndex(
    PIB * const ppib,
    FCB * const pfcb )
{
    Assert( pfcb->FTypeSecondaryIndex() );

    pfcb->Lock();

    while ( pfcb->Pfucb() != pfucbNil )
    {
        FUCB * const    pfucbT = pfcb->Pfucb();
        PIB * const     ppibT = pfucbT->ppib;

        pfcb->Unlock();

        Assert( ppibNil != ppibT );
        if ( ppib == ppibT )
        {
            pfucbT->u.pfcb->Unlink( pfucbT );

            BTReleaseBM( pfucbT );
            FUCBClose( pfucbT );
        }
        else
        {
            BOOL fDoUnlink = fFalse;
            PinstFromPpib( ppibT )->RwlTrx( ppibT ).EnterAsWriter();



            pfcb->Lock();
            if ( pfcb->Pfucb() == pfucbT )
            {
                fDoUnlink = fTrue;
            }
            pfcb->Unlock();

            if ( fDoUnlink )
            {
                pfucbT->u.pfcb->Unlink( pfucbT );
            }

            PinstFromPpib( ppibT )->RwlTrx( ppibT ).LeaveAsWriter();
        }

        pfcb->Lock();
    }

    pfcb->Unlock();
}


INLINE VOID VERIUnlinkDefunctLV(
    PIB * const ppib,
    FCB * const pfcb )
{
    Assert( pfcb->FTypeLV() );

    pfcb->Lock();

    while ( pfcb->Pfucb() != pfucbNil )
    {
        FUCB * const    pfucbT = pfcb->Pfucb();
        PIB * const     ppibT = pfucbT->ppib;

        pfcb->Unlock();

        Assert( ppib == ppibT );
        pfucbT->u.pfcb->Unlink( pfucbT );
        FUCBClose( pfucbT );

        pfcb->Lock();
    }

    pfcb->Unlock();
}


template< typename TDelta >
ERR VER::ErrVERICleanDeltaRCE( const RCE * const prce )
{
    ERR       err = JET_errSuccess;
    IFMP      ifmp;
    RECTASK  *ptask;
    PIB      *ppib;
    
    {
    ENTERREADERWRITERLOCK enterRwlRCEChainAsWriter( &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ), fFalse );

    if ( prce->Oper() != _VERDELTA<TDelta>::TRAITS::oper )
    {
        Assert( prce->Oper() == ( _VERDELTA<TDelta>::TRAITS::oper | operMaskNull ) );
        return err;
    }

    const _VERDELTA< TDelta >* const pverdelta = reinterpret_cast<const _VERDELTA<TDelta>*>( prce->PbData() );

    Assert( !FFMPIsTempDB( prce->Ifmp() ) );
    Assert( operDelta == prce->Oper() || operDelta64 == prce->Oper() );
    
    Assert( !m_pinst->m_plog->FRecovering() );
    
    BOOKMARK    bookmark;
    prce->GetBookmark( &bookmark );

    if ( pverdelta->fDeferredDelete &&
        !prce->Pfcb()->FDeletePending() )
    {
        Assert( !prce->Pfcb()->FDeleteCommitted() );
        Assert( !prce->Pfcb()->Ptdb() );
        ptask = new DELETELVTASK( prce->PgnoFDP(),
                                  prce->Pfcb(),
                                  prce->Ifmp(),
                                  bookmark );
        if ( NULL == ptask )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
        ppib = m_ppibRCEClean;
    }
    else if ( ( pverdelta->fCallbackOnZero || pverdelta->fDeleteOnZero ) &&
              !prce->Pfcb()->FDeletePending() )
    {
        Assert( trxMax != prce->TrxCommitted() );
        Assert( TrxCmp( prce->TrxCommitted(), TrxOldest( m_pinst ) ) < 0 );
        Assert( !prce->Pfcb()->FDeleteCommitted() );
        Assert( prce->Pfcb()->Ptdb() );

        ptask = new FINALIZETASK<TDelta>( prce->PgnoFDP(),
                                  prce->Pfcb(),
                                  prce->Ifmp(),
                                  bookmark,
                                  pverdelta->cbOffset,
                                  pverdelta->fCallbackOnZero,
                                  pverdelta->fDeleteOnZero );
        if ( NULL == ptask )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
        ppib = m_ppibRCECleanCallback;
    }
    else
    {
        return err;
    }

    if ( ppib->ErrRollbackFailure() < JET_errSuccess )
    {
        err = m_pinst->ErrInstanceUnavailableErrorCode() ? m_pinst->ErrInstanceUnavailableErrorCode() : JET_errInstanceUnavailable;
        delete ptask;
        return( ErrERRCheck( err ) );
    }

    ifmp = prce->Ifmp();

    Assert( prce->Oper() == operDelta || prce->Oper() == operDelta64 );
    }

    Assert( ptask );
    Assert( ppib );
    Assert( FIsRCECleanup() );
    
    if ( m_fSyncronousTasks
        || g_rgfmp[ ifmp ].FDetachingDB()
        || m_pinst->Taskmgr().CPostedTasks() > UlParam( m_pinst, JET_paramVersionStoreTaskQueueMax ) )
    {
        IncrementCSyncCleanupDispatched();
        TASK::Dispatch( ppib, (ULONG_PTR)ptask );
    }
    else
    {
        IncrementCAsyncCleanupDispatched();

        err = m_rectaskbatcher.ErrPost( ptask );
    }

    return err;
}


LOCAL VOID VERIRemoveCallback( const RCE * const prce )
{
    Assert( prce->CbData() == sizeof(VERCALLBACK) );
    const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
    CBDESC * const pcbdescRemove = pvercallback->pcbdesc;
    prce->Pfcb()->EnterDDL();
    prce->Pfcb()->Ptdb()->UnregisterPcbdesc( pcbdescRemove );
    prce->Pfcb()->LeaveDDL();
    delete pcbdescRemove;
}


INLINE VOID VER::IncrementCAsyncCleanupDispatched()
{
    PERFOpt( cVERAsyncCleanupDispatched.Inc( m_pinst ) );
}

INLINE VOID VER::IncrementCSyncCleanupDispatched()
{
    PERFOpt( cVERSyncCleanupDispatched.Inc( m_pinst ) );
}

VOID VER::IncrementCCleanupFailed()
{
    PERFOpt( cVERCleanupFailed.Inc( m_pinst ) );
}

VOID VER::IncrementCCleanupDiscarded( const RCE * const prce )
{
    if ( !prce->FOperNull() )
    {
        PERFOpt( cVERCleanupDiscarded.Inc( m_pinst ) );

        VERIReportDiscardedDeletes( prce );
    }
}


VOID VERIWaitForTasks( VER *pver, FCB *pfcb, BOOL fInRollback, BOOL fHaveRceCleanLock )
{
    const BOOL fCleanUpStateSavedSavedSaved = fInRollback ? FOSSetCleanupState( fFalse ) : fTrue;
    if ( fHaveRceCleanLock )
    {
        Assert( pver->m_critRCEClean.FOwner() );
    }
    else
    {
        pver->m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
    }
    (void)pver->m_rectaskbatcher.ErrPostAllPending();
    fHaveRceCleanLock ? 0 : pver->m_critRCEClean.Leave();
    fInRollback ? FOSSetCleanupState( fCleanUpStateSavedSavedSaved ) : 0;

    pfcb->WaitForTasksToComplete();
    Assert( pfcb->CTasksActive() == 0 );
}

ERR VER::ErrVERICleanOneRCE( RCE * const prce )
{
    ERR err = JET_errSuccess;

    Assert( m_critRCEClean.FOwner() );
    Assert( !FFMPIsTempDB( prce->Ifmp() ) );
    Assert( prce->TrxCommitted() != trxMax );
    Assert( !prce->FRolledBack() );

    switch( prce->Oper() )
    {
        case operCreateTable:
            Assert( pfcbNil != prce->Pfcb() );
            Assert( prce->Pfcb()->PrceOldest() != prceNil );
            if ( prce->Pfcb()->FTypeTable() )
            {
                if ( FCATHashActive( m_pinst ) )
                {


                    CHAR szTable[JET_cbNameMost+1];


                    prce->Pfcb()->EnterDML();
                    OSStrCbFormatA( szTable, sizeof(szTable), prce->Pfcb()->Ptdb()->SzTableName() );
                    prce->Pfcb()->LeaveDML();


                    CATHashIInsert( prce->Pfcb(), szTable );
                }
            }
            else
            {
                Assert( prce->Pfcb()->FTypeTemporaryTable() );
            }
            break;

        case operAddColumn:
        {
            Assert( prce->Pfcb()->PrceOldest() != prceNil );

            Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
            const JET_COLUMNID      columnid            = ( (VERADDCOLUMN*)prce->PbData() )->columnid;
            BYTE                    * pbOldDefaultRec   = ( (VERADDCOLUMN*)prce->PbData() )->pbOldDefaultRec;
            FCB                     * pfcbTable         = prce->Pfcb();

            pfcbTable->EnterDDL();

            TDB                     * const ptdb        = pfcbTable->Ptdb();
            FIELD                   * const pfield      = ptdb->Pfield( columnid );

            FIELDResetVersionedAdd( pfield->ffield );

            if ( FFIELDVersioned( pfield->ffield ) && !FFIELDDeleted( pfield->ffield ) )
            {
                FIELDResetVersioned( pfield->ffield );
            }

            Assert( NULL == pbOldDefaultRec
                || (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec );
            if ( NULL != pbOldDefaultRec
                && (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec )
            {
                pfcbTable->RemovePrecdangling( (RECDANGLING *)pbOldDefaultRec );
            }

            pfcbTable->LeaveDDL();

            break;
        }

        case operDeleteColumn:
        {
            Assert( prce->Pfcb()->PrceOldest() != prceNil );

            prce->Pfcb()->EnterDDL();

            Assert( prce->CbData() == sizeof(COLUMNID) );
            const COLUMNID  columnid        = *( (COLUMNID*)prce->PbData() );
            TDB             * const ptdb    = prce->Pfcb()->Ptdb();
            FIELD           * const pfield  = ptdb->Pfield( columnid );

            Assert( pfield->coltyp != JET_coltypNil );


            ptdb->MemPool().DeleteEntry( pfield->itagFieldName );

            Assert( !( FFIELDVersion( pfield->ffield )
                     && FFIELDAutoincrement( pfield->ffield ) ) );
            if ( FFIELDVersion( pfield->ffield ) )
            {
                Assert( ptdb->FidVersion() == FidOfColumnid( columnid ) );
                ptdb->ResetFidVersion();
            }
            else if ( FFIELDAutoincrement( pfield->ffield ) )
            {
                Assert( ptdb->FidAutoincrement() == FidOfColumnid( columnid ) );
                ptdb->ResetFidAutoincrement();
                ptdb->ResetAutoIncInitOnce();
            }

            Assert( !FFIELDVersionedAdd( pfield->ffield ) );
            Assert( FFIELDDeleted( pfield->ffield ) );
            FIELDResetVersioned( pfield->ffield );

            prce->Pfcb()->LeaveDDL();

            break;
        }

        case operCreateIndex:
        {
            FCB                     * const pfcbT = *(FCB **)prce->PbData();
            FCB                     * const pfcbTable = prce->Pfcb();
            FCB                     * const pfcbIndex = ( pfcbT == pfcbNil ? pfcbTable : pfcbT );
            IDB                     * const pidb = pfcbIndex->Pidb();

            pfcbTable->EnterDDL();

            Assert( pidbNil != pidb );

            Assert( pfcbTable->FTypeTable() );

            if ( pfcbTable == pfcbIndex )
            {
                Assert( !pidb->FVersionedCreate() );
                Assert( !pidb->FDeleted() );
                pidb->ResetFVersioned();
            }
            else if ( pidb->FVersionedCreate() )
            {
                pidb->ResetFVersionedCreate();

                if ( !pidb->FDeleted() )
                {
                    pidb->ResetFVersioned();
                }
            }

            pfcbTable->LeaveDDL();

            break;
        }

        case operDeleteIndex:
        {
            FCB * const pfcbIndex   = (*(FCB **)prce->PbData());
            FCB * const pfcbTable   = prce->Pfcb();

            Assert( pfcbIndex->FDeletePending() );
            Assert( pfcbIndex->FDeleteCommitted() );

            VERIWaitForTasks( this, pfcbIndex, fFalse, fTrue );

            pfcbTable->SetIndexing();
            pfcbTable->EnterDDL();

            Assert( pfcbTable->FTypeTable() );
            Assert( pfcbIndex->FTypeSecondaryIndex() );
            Assert( pfcbIndex != pfcbTable );
            Assert( pfcbIndex->PfcbTable() == pfcbTable );

            Assert( pfcbIndex->FDomainDenyRead( m_ppibRCEClean ) );

            Assert( pfcbIndex->Pidb() != pidbNil );
            Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
            Assert( pfcbIndex->Pidb()->FDeleted() );
            Assert( !pfcbIndex->Pidb()->FVersioned() );

            pfcbTable->UnlinkSecondaryIndex( pfcbIndex );

            pfcbTable->LeaveDDL();
            pfcbTable->ResetIndexing();

            Assert( !m_pinst->FRecovering() );
            VERNullifyAllVersionsOnFCB( pfcbIndex );
            VERIUnlinkDefunctSecondaryIndex( ppibNil, pfcbIndex );


            pfcbIndex->PrepareForPurge( fFalse );


            if ( !pfcbTable->FDeleteCommitted() )
            {
                if ( ErrLGFreeFDP( pfcbIndex, prce->TrxCommitted() ) >= JET_errSuccess )
                {
                    (VOID)ErrSPFreeFDP(
                                m_ppibRCEClean,
                                pfcbIndex,
                                pfcbTable->PgnoFDP() );
                }
            }


            pfcbIndex->Purge();

            break;
        }

        case operDeleteTable:
        {
            INT         fState;
            const IFMP  ifmp                = prce->Ifmp();
            const PGNO  pgnoFDPTable        = *(PGNO*)prce->PbData();
            FCB         * const pfcbTable   = FCB::PfcbFCBGet(
                                                    ifmp,
                                                    pgnoFDPTable,
                                                    &fState );
            Assert( pfcbNil != pfcbTable );
            Assert( pfcbTable->FTypeTable() );
            Assert( fFCBStateInitialized == fState );

            Assert( !m_pinst->FRecovering() );

            for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
            {
                Assert( pfcbT->FDeletePending() );
                Assert( pfcbT->FDeleteCommitted() );

                VERIWaitForTasks( this, pfcbT, fFalse, fTrue );

                Assert( pfcbT->PrceOldest() == prceNil
                    || ( pfcbT->PrceOldest()->Oper() == operFlagDelete
                        && pfcbT->PrceOldest()->FMoved() ) );
                VERNullifyAllVersionsOnFCB( pfcbT );

                pfcbT->PrepareForPurge( fFalse );
            }

            if ( pfcbTable->Ptdb() != ptdbNil )
            {
                Assert( fFCBStateInitialized == fState );
                FCB * const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
                if ( pfcbNil != pfcbLV )
                {
                    Assert( pfcbLV->FDeletePending() );
                    Assert( pfcbLV->FDeleteCommitted() );

                    VERIWaitForTasks( this, pfcbLV, fFalse, fTrue );

                    Assert( pfcbLV->PrceOldest() == prceNil
                         || pfcbLV->PrceOldest()->Oper() == operFlagDelete
                         || pfcbLV->PrceOldest()->Oper() == operWriteLock );
                    VERNullifyAllVersionsOnFCB( pfcbLV );

                    pfcbLV->PrepareForPurge( fFalse );
                }
            }
            else
            {
                FireWall( "DeprecatedSentinelFcbVerCleanFcbNil" );
            }

            if ( ErrLGFreeFDP( pfcbTable, prce->TrxCommitted() ) >= JET_errSuccess )
            {
                (VOID)ErrSPFreeFDP(
                            m_ppibRCEClean,
                            pfcbTable,
                            pgnoSystemRoot );
            }

            if ( fFCBStateInitialized == fState )
            {
                pfcbTable->Release();

                Assert( pfcbTable->PgnoFDP() == pgnoFDPTable );
                Assert( pfcbTable->FDeletePending() );
                Assert( pfcbTable->FDeleteCommitted() );

                Assert( pfcbTable->PrceOldest() == prceNil );
                Assert( pfcbTable->PrceNewest() == prceNil );
            }
            else
            {
                FireWall( "DeprecatedSentinelFcbVerCleanFcbInvState" );
            }
            pfcbTable->Purge();

            break;
        }

        case operRegisterCallback:
        {
        }
            break;

        case operUnregisterCallback:
        {
            VERIRemoveCallback( prce );
        }
            break;

        case operFlagDelete:
        {
            if ( FVERICleanDiscardDeletes() )
            {
                IncrementCCleanupDiscarded( prce );
                err = JET_errSuccess;
            }

            else if ( !m_pinst->FRecovering() )
            {
                if ( !prce->FFutureVersionsOfNode() )
                {
                    err = ErrVERIDelete( m_ppibRCEClean, prce );
                }
            }

            break;
        }

        case operDelta:
            if ( FVERICleanDiscardDeletes() )
            {
                IncrementCCleanupDiscarded( prce );
                err = JET_errSuccess;
            }
            else if ( !m_pinst->FRecovering() )
            {
                err = ErrVERICleanDeltaRCE<LONG>( prce );
            }
            break;

        case operDelta64:
            if ( FVERICleanDiscardDeletes() )
            {
                IncrementCCleanupDiscarded( prce );
                err = JET_errSuccess;
            }
            else if ( !m_pinst->FRecovering() )
            {
                err = ErrVERICleanDeltaRCE<LONGLONG>( prce );
            }
            break;

        default:
            break;
    }

    return err;
}


ERR RCE::ErrPrepareToDeallocate( TRX trxOldest )
{
    ERR         err     = JET_errSuccess;
    const OPER  oper    = m_oper;
    const UINT  uiHash  = m_uiHash;

    Assert( PinstFromIfmp( m_ifmp )->m_pver->m_critRCEClean.FOwner() );

    Assert( TrxCommitted() != trxMax );
    Assert( FFullyCommitted() );
    Assert( !FFMPIsTempDB( m_ifmp ) );

#ifdef DEBUG
    const TRX   trxDBGOldest = TrxOldest( PinstFromIfmp( m_ifmp ) );
    Assert( TrxCmp( trxDBGOldest, trxOldest ) >= 0 || trxMax == trxOldest );
#endif

    VER *pver = PverFromIfmp( m_ifmp );
    CallR( pver->ErrVERICleanOneRCE( this ) );

    const BOOL  fInHash = ::FOperInHashTable( oper );
    ENTERREADERWRITERLOCK maybeEnterRwlHashAsWriter(
                            fInHash ? &( PverFromIfmp( Ifmp() )->RwlRCEChain( uiHash ) ) : NULL,
                            fFalse,
                            fInHash );

    if ( FOperNull() || wrnVERRCEMoved == err )
    {
    }
    else
    {
        Assert( !FRolledBack() );
        Assert( !FOperNull() );


        RCE *prce = this;

        FCB * const pfcb = prce->Pfcb();
        ENTERCRITICALSECTION enterCritFCBRCEList( &( pfcb->CritRCEList() ) );

        do
        {
            RCE *prceNext;
            if ( prce->FOperInHashTable() )
                prceNext = prce->PrceNextOfNode();
            else
                prceNext = prceNil;

            ASSERT_VALID( prce );
            Assert( !prce->FOperNull() );
            VERINullifyCommittedRCE( prce );

            prce = prceNext;
        } while (
                prce != prceNil &&
                prce->FFullyCommitted() &&
                TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 &&
                operFlagDelete != prce->Oper() &&
                operDelta != prce->Oper() &&
                operDelta64 != prce->Oper() );
    }

    return JET_errSuccess;
}


ERR VER::ErrVERRCEClean( const IFMP ifmp )
{

    m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
    const ERR err = ErrVERIRCEClean( ifmp );
    m_critRCEClean.Leave();
    return err;
}

ERR VER::ErrVERIRCEClean( const IFMP ifmp )
{
    Assert( m_critRCEClean.FOwner() );

    const BOOL fCleanOneDb = ( ifmp != g_ifmpMax );

    Assert( !fCleanOneDb || g_rgfmp[ifmp].FDetachingDB() || g_rgfmp[ifmp].FShrinkIsRunning() || g_rgfmp[ifmp].FLeakReclaimerIsRunning() );

    const BOOL fSyncronousTasks = m_fSyncronousTasks;

    m_fSyncronousTasks = fCleanOneDb ? fTrue : m_fSyncronousTasks;

#ifdef DEBUG
    Ptls()->fIsRCECleanup = fTrue;
#endif

#ifdef VERPERF
    m_cbucketCleaned    = 0;
    m_cbucketSeen       = 0;
    m_crceSeen          = 0;
    m_crceCleaned       = 0;
    HRT hrtStartHrts    = HrtHRTCount();
    m_crceFlagDelete    = 0;
    m_crceDeleteLV      = 0;
#endif

    ERR         err     = JET_errSuccess;
    BUCKET *    pbucket;


    m_critBucketGlobal.Enter();

    pbucket = PbucketVERIGetOldest();

    m_critBucketGlobal.Leave();

    TRX trxOldest = TrxOldest( m_pinst );

    while ( pbucketNil != pbucket )
    {
#ifdef VERPERF
        INT crceInBucketSeen    = 0;
        INT crceInBucketCleaned = 0;
#endif


        BOOL    fNeedBeInCritBucketGlobal   = ( pbucket->hdr.pbucketNext == pbucketNil );


        Assert( m_critRCEClean.FOwner() );
        RCE *   prce                        = pbucket->hdr.prceOldest;
        BOOL    fSkippedRCEInBucket         = fFalse;

        forever
        {
#ifdef VERPERF
            ++m_crceSeen;
            ++crceInBucketSeen;
#endif

            Assert( pbucket->rgb <= (BYTE*)prce );
            Assert( (BYTE*)prce < (BYTE*)pbucket + m_cbBucket ||
                    (   (BYTE*)prce == (BYTE*)pbucket + m_cbBucket &&
                        prce == pbucket->hdr.prceNextNew ) );

            if ( fNeedBeInCritBucketGlobal )
            {
                m_critBucketGlobal.Enter();
            }

            const BOOL  fRecalcOldestRCE    = ( !fSkippedRCEInBucket );

            if ( fRecalcOldestRCE )
            {
                pbucket->hdr.prceOldest = prce;
            }

            Assert( pbucket->rgb <= pbucket->hdr.pbLastDelete );
            Assert( pbucket->hdr.pbLastDelete <= reinterpret_cast<BYTE *>( pbucket->hdr.prceOldest ) );
            Assert( pbucket->hdr.prceOldest <= pbucket->hdr.prceNextNew );

            if ( pbucket->hdr.prceNextNew == prce )
                break;

            if ( fNeedBeInCritBucketGlobal )
                m_critBucketGlobal.Leave();

            const INT   cbRce = prce->CbRce();

            Assert( pbucket->rgb <= (BYTE*)prce );
            Assert( prce->CbRce() > 0 );
            Assert( (BYTE*)prce + prce->CbRce() <= (BYTE*)pbucket + m_cbBucket );

            if ( !prce->FOperNull() )
            {
                Assert( g_rgfmp[ prce->Ifmp() ].Pinst() == m_pinst );
#ifdef DEBUG
                const TRX   trxDBGOldest    = TrxOldest( m_pinst );
#endif
                const BOOL  fFullyCommitted = prce->FFullyCommitted();
                const TRX   trxRCECommitted = prce->TrxCommitted();
                BOOL        fCleanable      = fFalse;

                if ( trxMax == trxOldest )
                {
                    trxOldest = TrxOldest( m_pinst );
                }

                if ( fFullyCommitted && !FFMPIsTempDB( prce->Ifmp() ) )
                {
                    Assert( trxMax != trxRCECommitted );
                    if ( TrxCmp( trxRCECommitted, trxOldest ) < 0 )
                    {
                        fCleanable = fTrue;
                    }
                    else if ( trxMax != trxOldest )
                    {
                        trxOldest = TrxOldest( m_pinst );
                        if ( TrxCmp( trxRCECommitted, trxOldest ) < 0 )
                        {
                            fCleanable = fTrue;
                        }
                    }
                    else
                    {
                        Assert( fFalse );
                    }
                }

                if ( !fCleanable )
                {
                    if ( fCleanOneDb )
                    {
                        Assert( !FFMPIsTempDB( ifmp ) );
                        if ( prce->Ifmp() == ifmp )
                        {
                            if ( !prce->FFullyCommitted() )
                            {
                                err = ErrERRCheck( JET_wrnRemainingVersions );
                                goto HandleError;
                            }
                            else
                            {
                            }
                        }
                        else
                        {
                            fSkippedRCEInBucket = fTrue;
                            goto NextRCE;
                        }
                    }
                    else
                    {
                        Assert( pbucketNil != pbucket );
                        Assert( !prce->FMoved() );
                        err = ErrERRCheck( JET_wrnRemainingVersions );
                        goto HandleError;
                    }
                }

                Assert( prce->FFullyCommitted() );
                Assert( prce->TrxCommitted() != trxMax );
                Assert( TrxCmp( prce->TrxCommitted(), trxDBGOldest ) < 0
                        || fCleanOneDb
                        || TrxCmp( prce->TrxCommitted(), trxOldest ) < 0 );

#ifdef VERPERF
                if ( operFlagDelete == prce->Oper() )
                {
                    ++m_crceFlagDelete;
                }
                else if ( operDelta == prce->Oper() )
                {
                    const VERDELTA32* const pverdelta = reinterpret_cast<VERDELTA32*>( prce->PbData() );
                    if ( pverdelta->fDeferredDelete )
                    {
                        ++m_crceDeleteLV;
                    }
                }
                else if ( operDelta64 == prce->Oper() )
                {
                    const VERDELTA64* const pverdelta = reinterpret_cast<VERDELTA64*>( prce->PbData() );
                    if ( pverdelta->fDeferredDelete )
                    {
                        ++m_crceDeleteLV;
                    }
                }
#endif

                Call( prce->ErrPrepareToDeallocate( trxOldest ) );

#ifdef VERPERF
                ++crceInBucketCleaned;
                ++m_crceCleaned;
#endif
            }

NextRCE:
            Assert( m_critRCEClean.FOwner() );


            const BYTE  *pbRce      = reinterpret_cast<BYTE *>( prce );
            const BYTE  *pbNextRce  = reinterpret_cast<BYTE *>( PvAlignForThisPlatform( pbRce + cbRce ) );

            prce = (RCE *)pbNextRce;

            Assert( pbucket->rgb <= (BYTE*)prce );
            Assert( (BYTE*)prce < (BYTE*)pbucket + m_cbBucket ||
                    (   (BYTE*)prce == (BYTE*)pbucket + m_cbBucket &&
                        prce == pbucket->hdr.prceNextNew ) );

            Assert( pbucket->hdr.prceOldest <= pbucket->hdr.prceNextNew );
        }


        if ( fNeedBeInCritBucketGlobal )
            Assert( m_critBucketGlobal.FOwner() );
        else
            m_critBucketGlobal.Enter();

#ifdef VERPERF
        ++m_cbucketSeen;
#endif

        const BOOL  fRemainingRCEs  = ( fSkippedRCEInBucket );

        if ( fRemainingRCEs )
        {
            pbucket = pbucket->hdr.pbucketNext;
        }
        else
        {
            Assert( pbucket->rgb == pbucket->hdr.pbLastDelete );
            pbucket = PbucketVERIFreeAndGetNextOldestBucket( pbucket );

#ifdef VERPERF
            ++m_cbucketCleaned;
#endif
        }

        m_critBucketGlobal.Leave();
    }


    Assert( pbucketNil == pbucket );
    err = JET_errSuccess;

    if ( !fCleanOneDb )
    {
        m_critBucketGlobal.Enter();
        if ( pbucketNil != m_pbucketGlobalHead )
        {
            Assert( pbucketNil != m_pbucketGlobalTail );
            err = ErrERRCheck( JET_wrnRemainingVersions );
        }
        else
        {
            Assert( pbucketNil == m_pbucketGlobalTail );
        }
        m_critBucketGlobal.Leave();
    }

HandleError:
    const ERR errT = m_rectaskbatcher.ErrPostAllPending();
    if( errT < JET_errSuccess && err >= JET_errSuccess )
    {
        err = errT;
    }

#ifdef DEBUG_VER_EXPENSIVE
    if ( !m_pinst->m_plog->FRecovering() )
    {
        double  dblElapsedTime  = DblHRTElapsedTimeFromHrtStart( hrtStartHrts );
        const size_t cchBuf = 512;
        CHAR    szBuf[cchBuf];

        StringCbPrintfA( szBuf,
                        cchBuf,
                        "RCEClean: "
                        "elapsed time %10.10f seconds, "
                        "saw %6.6d RCEs, "
                        "( %6.6d flagDelete, "
                        "%6.6d deleteLV ), "
                        "cleaned %6.6d RCEs, "
                        "cleaned %4.4d buckets",
                        dblElapsedTime,
                        m_crceSeen,
                        m_crceFlagDelete,
                        m_crceDeleteLV,
                        m_crceCleaned,
                        m_cbucketCleaned
                    );
        (VOID)m_pinst->m_plog->ErrLGTrace( ppibNil, szBuf );
    }
#endif

#ifdef DEBUG
    Ptls()->fIsRCECleanup = fFalse;
#endif

    m_fSyncronousTasks = fSyncronousTasks;

    if ( !fCleanOneDb )
    {
        m_tickLastRCEClean = TickOSTimeCurrent();
    }

    return err;
}


VOID VERICommitRegisterCallback( const RCE * const prce, const TRX trxCommit0 )
{
#ifdef VERSIONED_CALLBACKS
    Assert( prce->CbData() == sizeof(VERCALLBACK) );
    const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
    CBDESC * const pcbdesc = pvercallback->pcbdesc;
    prce->Pfcb()->EnterDDL();
    Assert( trxMax != pcbdesc->trxRegisterBegin0 );
    Assert( trxMax == pcbdesc->trxRegisterCommit0 );
    Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
    Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
    pvercallback->pcbdesc->trxRegisterCommit0 = trxCommit0;
    prce->Pfcb()->LeaveDDL();
#endif
}


VOID VERICommitUnregisterCallback( const RCE * const prce, const TRX trxCommit0 )
{
#ifdef VERSIONED_CALLBACKS
    Assert( prce->CbData() == sizeof(VERCALLBACK) );
    const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
    CBDESC * const pcbdesc = pvercallback->pcbdesc;
    prce->Pfcb()->EnterDDL();
    Assert( trxMax != pcbdesc->trxRegisterBegin0 );
    Assert( trxMax != pcbdesc->trxRegisterCommit0 );
    Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
    Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
    pvercallback->pcbdesc->trxUnregisterCommit0 = trxCommit0;
    prce->Pfcb()->LeaveDDL();
#endif
}


LOCAL VOID VERICommitTransactionToLevelGreaterThan0( const PIB * const ppib )
{
    const LEVEL level   = ppib->Level();
    Assert( level > 1 );


    RCE         *prce   = ppib->prceNewest;
    for ( ; prceNil != prce && prce->Level() == level; prce = prce->PrcePrevOfSession() )
    {
        Assert( prce->TrxCommitted() == trxMax );

        prce->SetLevel( LEVEL( level - 1 ) );
    }
}


RCE * PIB::PrceOldest()
{
    RCE     * prcePrev  = prceNil;

    for ( RCE * prceCurr = prceNewest;
        prceNil != prceCurr;
        prceCurr = prceCurr->PrcePrevOfSession() )
    {
        prcePrev = prceCurr;
    }

    return prcePrev;
}

INLINE VOID VERICommitOneRCEToLevel0( PIB * const ppib, RCE * const prce )
{
    Assert( !prce->FFullyCommitted() );

    prce->SetLevel( 0 );

    VERIDeleteRCEFromSessionList( ppib, prce );

    prce->Pfcb()->CritRCEList().Enter();
    prce->SetTrxCommitted( ppib->trxCommit0 );
    prce->Pfcb()->CritRCEList().Leave();
}

LOCAL VOID VERICommitTransactionToLevel0( PIB * const ppib )
{
    RCE         * prce              = ppib->prceNewest;

    Assert( 1 == ppib->Level() );


    RCE * prceNextToCommit;
#ifdef DEBUG
    prceNextToCommit = prceInvalid;
#endif
    for ( ; prceNil != prce; prce = prceNextToCommit )
    {
        prceNextToCommit    = prce->PrcePrevOfSession();

        ASSERT_VALID( prce );
        Assert( !prce->FOperNull() );
        Assert( !prce->FFullyCommitted() );
        Assert( 1 == prce->Level() || PinstFromPpib( ppib )->m_plog->FRecovering() );
        Assert( prce->Pfucb()->ppib == ppib );

        Assert( prceInvalid != prce );

        if ( FFMPIsTempDB( prce->Ifmp() ) )
        {

            Assert( !prce->FOperNull() );
            Assert( !prce->FOperDDL() || operCreateTable == prce->Oper() || operCreateLV == prce->Oper() );
            Assert( !PinstFromPpib( ppib )->m_plog->FRecovering() );
            Assert( !prce->FFullyCommitted() );
            Assert( prce->PgnoUndoInfo() == pgnoNull );

            const BOOL fInHash = prce->FOperInHashTable();

            ENTERREADERWRITERLOCK maybeEnterRwlHashAsWriter( fInHash ? &( PverFromPpib( ppib )->RwlRCEChain( prce->UiHash() ) ) : 0, fFalse, fInHash );
            ENTERCRITICALSECTION enterCritFCBRCEList( &prce->Pfcb()->CritRCEList() );

            VERIDeleteRCEFromSessionList( ppib, prce );

            prce->SetLevel( 0 );
            prce->SetTrxCommitted( ppib->trxCommit0 );


            VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );
            if ( fInHash )
            {
                VERIDeleteRCEFromHash( prce );
            }
            prce->NullifyOper();

            continue;
        }


        if ( prce->PgnoUndoInfo() != pgnoNull )
            BFRemoveUndoInfo( prce );

        ENTERREADERWRITERLOCK maybeEnterRwlHashAsWriter(
            prce->FOperInHashTable() ? &( PverFromPpib( ppib )->RwlRCEChain( prce->UiHash() ) ) : 0,
            fFalse,
            prce->FOperInHashTable()
            );

        if ( prce->FOperDDL() )
        {
            switch( prce->Oper() )
            {
                case operAddColumn:
                    Assert( prce->Pfcb()->PrceOldest() != prceNil );
                    Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
                    break;

                case operDeleteColumn:
                    Assert( prce->Pfcb()->PrceOldest() != prceNil );
                    Assert( prce->CbData() == sizeof(COLUMNID) );
                    break;

                case operCreateIndex:
                {
                    const FCB   * const pfcbT       = *(FCB **)prce->PbData();
                    if ( pfcbNil == pfcbT )
                    {
                        FCB     * const pfcbTable   = prce->Pfcb();

                        pfcbTable->EnterDDL();
                        Assert( pfcbTable->FPrimaryIndex() );
                        Assert( pfcbTable->FTypeTable() );
                        Assert( pfcbTable->Pidb() != pidbNil );
                        Assert( pfcbTable->Pidb()->FPrimary() );

                        pfcbTable->Pidb()->ResetFVersionedCreate();

                        pfcbTable->LeaveDDL();
                    }
                    else
                    {
                        Assert( pfcbT->FTypeSecondaryIndex() );
                        Assert( pfcbT->Pidb() != pidbNil );
                        Assert( !pfcbT->Pidb()->FPrimary() );
                        Assert( pfcbT->PfcbTable() != pfcbNil );
                    }
                    break;
                }

                case operCreateLV:
                {
                    break;
                }

                case operDeleteIndex:
                {
                    FCB * const pfcbIndex = (*(FCB **)prce->PbData());
                    FCB * const pfcbTable = prce->Pfcb();

                    Assert( pfcbTable->FTypeTable() );
                    Assert( pfcbIndex->FDeletePending() );
                    Assert( !pfcbIndex->FDeleteCommitted() || ( pfcbIndex->FDeleteCommitted() && pfcbIndex->PfcbTable()->FDeleteCommitted() ) );
                    Assert( pfcbIndex->FTypeSecondaryIndex() );
                    Assert( pfcbIndex != pfcbTable );
                    Assert( pfcbIndex->PfcbTable() == pfcbTable );
                    Assert( pfcbIndex->FDomainDenyReadByUs( prce->Pfucb()->ppib ) );

                    pfcbTable->EnterDDL();


                    Assert( pfcbIndex->Pidb() != pidbNil );
                    Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
                    Assert( pfcbIndex->Pidb()->FDeleted() );
                    pfcbIndex->Pidb()->ResetFVersioned();

                    pfcbIndex->Lock();
                    pfcbIndex->SetDeleteCommitted();
                    pfcbIndex->Unlock();

                    FILESetAllIndexMask( pfcbTable );

                    pfcbTable->LeaveDDL();
                    break;
                }

                case operDeleteTable:
                {
                    FCB     *pfcbTable;
                    INT     fState;

                    pfcbTable = FCB::PfcbFCBGet( prce->Ifmp(), *(PGNO*)prce->PbData(), &fState );
                    Assert( pfcbTable != pfcbNil );
                    Assert( pfcbTable->FTypeTable() );
                    Assert( fFCBStateInitialized == fState );

                    if ( pfcbTable->Ptdb() != ptdbNil )
                    {
                        Assert( fFCBStateInitialized == fState );

                        pfcbTable->EnterDDL();

                        Assert( !pfcbTable->FDeleteCommitted() );
                        for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
                        {
                            Assert( pfcbT->FDeletePending() );
                            pfcbT->Lock();
                            pfcbT->SetDeleteCommitted();
                            pfcbT->Unlock();
                        }

                        FCB * const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
                        if ( pfcbNil != pfcbLV )
                        {
                            Assert( pfcbLV->FDeletePending() );
                            pfcbLV->Lock();
                            pfcbLV->SetDeleteCommitted();
                            pfcbLV->Unlock();
                        }

                        pfcbTable->LeaveDDL();

                        pfcbTable->Release();
                    }
                    else
                    {
                        FireWall( "DeprecatedSentinelFcbVerCommit0" );
                        Assert( pfcbTable->PfcbNextIndex() == pfcbNil );
                        pfcbTable->Lock();
                        pfcbTable->SetDeleteCommitted();
                        pfcbTable->Unlock();
                    }

                    break;
                }

                case operCreateTable:
                {
                    FCB     *pfcbTable;

                    pfcbTable = prce->Pfcb();
                    Assert( pfcbTable != pfcbNil );
                    Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeTemporaryTable() );

                    pfcbTable->EnterDDL();

                    pfcbTable->Lock();
                    Assert( pfcbTable->FUncommitted() );
                    pfcbTable->ResetUncommitted();
                    pfcbTable->Unlock();
                    pfcbTable->LeaveDDL();
                    
                    break;
                }

                case operRegisterCallback:
                    VERICommitRegisterCallback( prce, ppib->trxCommit0 );
                    break;

                case operUnregisterCallback:
                    VERICommitUnregisterCallback( prce, ppib->trxCommit0 );
                    break;

                default:
                    Assert( fFalse );
                    break;
            }
        }
#ifdef DEBUG
        else
        {
            const PGNO  pgnoUndoInfo = prce->PgnoUndoInfo();
            Assert( pgnoNull == pgnoUndoInfo );
        }
#endif

        VERICommitOneRCEToLevel0( ppib, prce );
    }

    Assert( prceNil == ppib->prceNewest );

    Assert( prceNil == ppib->prceNewest );
    PIBSetPrceNewest( ppib, prceNil );
}


VOID VERCommitTransaction( PIB * const ppib )
{
    ASSERT_VALID( ppib );

    const LEVEL             level = ppib->Level();

    Assert( level > 0 );
    Assert( PinstFromPpib( ppib )->m_plog->FRecovering() || trxMax != TrxOldest( PinstFromPpib( ppib ) ) );
    Assert( PinstFromPpib( ppib )->m_plog->FRecovering() || TrxCmp( ppib->trxBegin0, TrxOldest( PinstFromPpib( ppib ) ) ) >= 0 );

    if ( level > 1 )
    {
        VERICommitTransactionToLevelGreaterThan0( ppib );
    }
    else
    {
        VERICommitTransactionToLevel0( ppib );
    }

    Assert( ppib->Level() > 0 );
    ppib->DecrementLevel();
}


LOCAL ERR ErrVERILogUndoInfo( RCE *prce, CSR* pcsr )
{
    ERR     err             = JET_errSuccess;
    LGPOS   lgpos           = lgposMin;
    LOG     * const plog    = PinstFromIfmp( prce->Ifmp() )->m_plog;

    Assert( pcsr->FLatched() );

    if ( plog->FRecoveringMode() != fRecoveringRedo )
    {
        CallR( ErrLGUndoInfo( prce, &lgpos ) );
    }

    BFRemoveUndoInfo( prce, lgpos );

    return err;
}



ERR ErrVERIUndoReplacePhysical( RCE * const prce, CSR *pcsr, const BOOKMARK& bm )
{
    ERR     err;
    DATA    data;
    FUCB    * const pfucb   = prce->Pfucb();

    Assert( prce->FOperReplace() );

    if ( prce->FEmptyDiff() )
    {
        Assert( prce->PgnoUndoInfo() == pgnoNull );
        Assert( ((VERREPLACE *)prce->PbData())->cbDelta == 0 );
        return JET_errSuccess;
    }

    if ( prce->PgnoUndoInfo() != pgnoNull )
    {
        CallR( ErrVERILogUndoInfo( prce, pcsr ) );
    }

    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );


    data.SetPv( prce->PbData() + cbReplaceRCEOverhead );
    data.SetCb( prce->CbData() - cbReplaceRCEOverhead );

    const VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
    const INT cbDelta = pverreplace->cbDelta;

    if ( cbDelta > 0 )
    {
        pcsr->Cpage().ReclaimUncommittedFreed( cbDelta );
    }

    if ( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() )
    {
        pfucb->bmCurr = bm;
    }
    else
    {
        Assert( CmpKeyData( pfucb->bmCurr, bm ) == 0 );
    }

    CallS( ErrNDReplace ( pfucb, pcsr, &data, fDIRUndo, rceidNull, prceNil ) );

    if ( cbDelta < 0 )
    {
        pcsr->Cpage().AddUncommittedFreed( -cbDelta );
    }

    return err;
}


ERR ErrVERIUndoInsertPhysical( RCE * const prce, CSR *pcsr, PIB * ppib = NULL, BOOL *pfRolledBack = NULL )
{
    ERR     err;
    FUCB    * const pfucb = prce->Pfucb();

    Assert( pgnoNull == prce->PgnoUndoInfo() );

    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

    CallS( ErrNDFlagDelete( pfucb, pcsr, fDIRUndo, rceidNull, NULL ) );

    if ( ( !PinstFromIfmp( prce->Ifmp() )->FRecovering()
     ) &&
         !FFMPIsTempDB( prce->Ifmp() ) &&
         !prce->FFutureVersionsOfNode() )
    {
        Assert( ppib != NULL );
        Assert( pfRolledBack != NULL );

        DELETERECTASK * ptask;

        const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

#ifdef DEBUG
        Ptls()->fIsRCECleanup = fTrue;
#endif

        err = prce->ErrGetTaskForDelete( (VOID **)&ptask );
        if ( err >= JET_errSuccess && ptask != NULL )
        {
            VERINullifyRolledBackRCE( ppib, prce );
            *pfRolledBack = fTrue;

            BTUp( pfucb );

            PverFromPpib( ppib )->IncrementCSyncCleanupDispatched();
            TASK::DispatchGP( ptask );
        }

#ifdef DEBUG
        Ptls()->fIsRCECleanup = fFalse;
#endif

        FOSSetCleanupState( fCleanUpStateSavedSavedSaved );
    }

    return err;
}


LOCAL ERR ErrVERIUndoFlagDeletePhysical( RCE * prce, CSR *pcsr )
{
    ERR     err;
#ifdef DEBUG
    FUCB    * const pfucb   = prce->Pfucb();

    Unused( pfucb );
#endif

    if ( prce->PgnoUndoInfo() != pgnoNull )
    {
        CallR( ErrVERILogUndoInfo( prce, pcsr ) );
    }

    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

    NDResetFlagDelete( pcsr );
    return err;
}


template< typename TDelta >
ERR ErrVERIUndoDeltaPhysical( RCE * const prce, CSR *pcsr )
{
    ERR     err;
    FUCB    * const pfucb = prce->Pfucb();
    LOG     *plog = PinstFromIfmp( prce->Ifmp() )->m_plog;

    _VERDELTA<TDelta>* const    pverdelta   = (_VERDELTA<TDelta>*)prce->PbData();
    const TDelta                tDelta      = -pverdelta->tDelta;

    Assert( pgnoNull == prce->PgnoUndoInfo() );


    if ( pverdelta->tDelta < 0 && !plog->FRecovering() )
    {
        ENTERREADERWRITERLOCK enterRwlHashAsWriter( &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ), fFalse );

        RCE * prceT = prce;
        for ( ; prceNil != prceT->PrceNextOfNode(); prceT = prceT->PrceNextOfNode() )
            ;
        for ( ; prceNil != prceT; prceT = prceT->PrcePrevOfNode() )
        {
            _VERDELTA<TDelta>* const pverdeltaT = ( _VERDELTA<TDelta>* )prceT->PbData();
            if ( _VERDELTA<TDelta>::TRAITS::oper == prceT->Oper() && pverdelta->cbOffset == pverdeltaT->cbOffset )
            {
                pverdeltaT->fDeferredDelete = fFalse;
                pverdeltaT->fCallbackOnZero = fFalse;
                pverdeltaT->fDeleteOnZero = fFalse;
            }
        }
    }

    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );


    TDelta tOldValue;
    CallS( ErrNDDelta(
            pfucb,
            pcsr,
            pverdelta->cbOffset,
            tDelta,
            &tOldValue,
            fDIRUndo,
            rceidNull ) );
    if ( 0 == ( tOldValue + tDelta ) )
    {
    }

    pverdelta->tDelta = 0;

    return err;
}


VOID VERRedoPhysicalUndo( INST *pinst, const LRUNDO *plrundo, FUCB *pfucb, CSR *pcsr, BOOL fRedoNeeded )
{
    BOOKMARK    bm;
    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( (VOID *) plrundo->rgbBookmark );
    bm.key.suffix.SetCb( plrundo->CbBookmarkKey() );
    bm.data.SetPv( (BYTE *) plrundo->rgbBookmark + plrundo->CbBookmarkKey() );
    bm.data.SetCb( plrundo->CbBookmarkData() );

    const DBID              dbid    = plrundo->dbid;
    IFMP                    ifmp = pinst->m_mpdbidifmp[ dbid ];

    const UINT              uiHash  = UiRCHashFunc( ifmp, plrundo->le_pgnoFDP, bm );
    ENTERREADERWRITERLOCK   enterRwlHashAsWriter( &( PverFromIfmp( ifmp )->RwlRCEChain( uiHash ) ), fFalse );

    RCE * prce = PrceRCEGet( uiHash, ifmp, plrundo->le_pgnoFDP, bm );
    Assert( !fRedoNeeded || prce != prceNil );

    for ( ; prce != prceNil ; prce = prce->PrcePrevOfNode() )
    {
        if ( prce->Rceid() == plrundo->le_rceid )
        {
            Assert( prce->Pfucb() == pfucb );
            Assert( prce->Pfucb()->ppib == pfucb->ppib );
            Assert( prce->Oper() == plrundo->le_oper );
            Assert( prce->TrxCommitted() == trxMax );

            Assert( prce->Level() == plrundo->level );

            if ( fRedoNeeded )
            {
                Assert( prce->FUndoableLoggedOper( ) );
                OPER oper = plrundo->le_oper;

                switch ( oper )
                {
                    case operReplace:
                        CallS( ErrVERIUndoReplacePhysical( prce,
                                                           pcsr,
                                                           bm ) );
                        break;

                    case operInsert:
                        CallS( ErrVERIUndoInsertPhysical( prce, pcsr ) );
                        break;

                    case operFlagDelete:
                        CallS( ErrVERIUndoFlagDeletePhysical( prce, pcsr ) );
                        break;

                    case operDelta:
                        CallS( ErrVERIUndoDeltaPhysical<LONG>( prce, pcsr ) );
                        break;

                    case operDelta64:
                        CallS( ErrVERIUndoDeltaPhysical<LONGLONG>( prce, pcsr ) );
                        break;

                    default:
                        Assert( fFalse );
                }
            }

            ENTERCRITICALSECTION critRCEList( &(prce->Pfcb()->CritRCEList()) );
            VERINullifyUncommittedRCE( prce );
            break;
        }
    }

    Assert( !fRedoNeeded || prce != prceNil );

    return;
}


LOCAL ERR ErrVERITryUndoLoggedOper( PIB *ppib, RCE * const prce )
{
    ERR             err;
    FUCB * const    pfucb       = prce->Pfucb();
    LATCH           latch       = latchReadNoTouch;
    BOOL            fRolledBack = fFalse;
    BOOKMARK        bm;
    BOOKMARK        bmSave;

    Assert( pfucbNil != pfucb );
    ASSERT_VALID( pfucb );
    Assert( ppib == pfucb->ppib );
    Assert( prce->FUndoableLoggedOper() );
    Assert( prce->TrxCommitted() == trxMax );

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucb );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );
    tcScope->iorReason.SetIort( iortRollbackUndo );

    if ( fRecoveringRedo == PinstFromPpib( ppib )->m_plog->FRecoveringMode() )
    {
        VERINullifyRolledBackRCE( ppib, prce );
        return JET_errSuccess;
    }
    
    prce->GetBookmark( &bm );

    DIRResetIndexRange( pfucb );

    bmSave = pfucb->bmCurr;
    pfucb->bmCurr = bm;

    BTUp( pfucb );

Refresh:
    Call( ErrFaultInjection( 58139 ) );
    err = ErrBTIRefresh( pfucb, latch );
    Assert( JET_errRecordDeleted != err );
    Call( err );

    err = Pcsr( pfucb )->ErrUpgrade();
    if ( errBFLatchConflict == err )
    {
        Assert( !Pcsr( pfucb )->FLatched() );
        latch = latchRIW;
        goto Refresh;
    }
    Call( err );

    switch( prce->Oper() )
    {
        case operReplace:
            Call( ErrVERIUndoReplacePhysical( prce, Pcsr( pfucb ), bm ) );
            break;

        case operInsert:
            Call( ErrVERIUndoInsertPhysical( prce, Pcsr( pfucb ), ppib, &fRolledBack ) );
            break;

        case operFlagDelete:
            Call( ErrVERIUndoFlagDeletePhysical( prce, Pcsr( pfucb ) ) );
            break;

        case operDelta:
            Call( ErrVERIUndoDeltaPhysical<LONG>( prce, Pcsr( pfucb ) ) );
            break;

        case operDelta64:
            Call( ErrVERIUndoDeltaPhysical<LONGLONG>( prce, Pcsr( pfucb ) ) );
            break;

        default:
            Assert( fFalse );
    }

    if ( !fRolledBack )
    {
        Assert( !prce->FOperNull() );
        Assert( !prce->FRolledBack() );

        VERINullifyRolledBackRCE( ppib, prce );
    }

    pfucb->bmCurr = bmSave;
    BTUp( pfucb );

    CallS( err );
    return JET_errSuccess;

HandleError:
    pfucb->bmCurr = bmSave;
    BTUp( pfucb );

    Assert( err < JET_errSuccess );
    Assert( JET_errDiskFull != err );

    return err;
}


LOCAL ERR ErrVERIUndoLoggedOper( PIB *ppib, RCE * const prce )
{
    ERR             err     = JET_errSuccess;
    FUCB    * const pfucb   = prce->Pfucb();

    Assert( pfucbNil != pfucb );
    Assert( ppib == pfucb->ppib );
    Call( ErrVERITryUndoLoggedOper( ppib, prce ) );

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );



    Assert( JET_errDiskFull != err );

    switch ( err )
    {
        

        case JET_errBadPageLink:
        case JET_errDatabaseCorrupted:
        case JET_errDbTimeCorrupted:
        case JET_errRecordDeleted:
        case JET_errLVCorrupted:
        case JET_errBadParentPageLink:
        case JET_errPageNotInitialized:
        
    
        
        AssertSzRTL( false, "Unexpected error condition blocking Rollback. Error = %d, oper = %d", err, prce->Oper() );



        case JET_errFileAccessDenied:
        case JET_errDiskIO:
        case_AllDatabaseStorageCorruptionErrs:
        case JET_errOutOfBuffers:
        case JET_errOutOfMemory :
        case JET_errCheckpointDepthTooDeep:


        case JET_errLogWriteFail:
        case JET_errLogDiskFull:
            {


                INST * const    pinst       = PinstFromPpib( ppib );
                LOG * const     plog        = pinst->m_plog;
                WCHAR           wszRCEID[16];
                WCHAR           wszERROR[16];

                ERR errLog;
                if ( JET_errLogWriteFail == err
                    && plog->FNoMoreLogWrite( &errLog )
                    && JET_errLogDiskFull == errLog )
                {
                    err = ErrERRCheck( JET_errLogDiskFull );
                }

                OSStrCbFormatW( wszRCEID, sizeof(wszRCEID), L"%d", prce->Rceid() );
                OSStrCbFormatW( wszERROR, sizeof(wszERROR),  L"%d", err );

                const WCHAR *rgpsz[] = { wszRCEID, g_rgfmp[pfucb->ifmp ].WszDatabaseName(), wszERROR };
                UtilReportEvent(
                        eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        UNDO_FAILED_ID,
                        _countof( rgpsz ),
                        rgpsz,
                        0,
                        NULL,
                        pinst );

                if ( g_rgfmp[pfucb->ifmp].FLogOn() )
                {
                    Assert( plog );
                    Assert( !plog->FLogDisabled() );


                    (void)ErrLGWrite( ppib );
                    UtilSleep( cmsecWaitLogWrite );
                    plog->SetNoMoreLogWrite( err );
                }



                Assert( !prce->FOperNull() );
                Assert( !prce->FRolledBack() );

                ERR errLogNoMore;
                if ( !plog->FNoMoreLogWrite( &errLogNoMore ) || errLogNoMore != errLogServiceStopped )
                {
                    pinst->SetInstanceUnavailable( err );
                }
                ppib->SetErrRollbackFailure( err );
                err = ErrERRCheck( JET_errRollbackError );
                break;
            }

        default:
            break;
    }

    return err;
}


INLINE VOID VERINullifyForUndoCreateTable( PIB * const ppib, FCB * const pfcb )
{
    Assert( pfcb->FTypeTable()
        || pfcb->FTypeTemporaryTable() );

    while ( prceNil != pfcb->PrceNewest() )
    {
        RCE * const prce = pfcb->PrceNewest();

        Assert( prce->Pfcb() == pfcb );
        Assert( !prce->FOperNull() );

        Assert( prce->TrxCommitted() == trxMax );

        if ( !prce->FRolledBack() )
        {
            Assert( prce->Oper() == operCreateTable );
            Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
            Assert( pfcb->PrceNewest() == prce );
            Assert( pfcb->PrceOldest() == prce );
        }

        Assert( prce->Pfucb() != pfucbNil );
        Assert( ppib == prce->Pfucb()->ppib );

        ENTERREADERWRITERLOCK   maybeEnterRwlHashAsWriter(
                                    prce->FOperInHashTable() ? &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ) : NULL,
                                    fFalse,
                                    prce->FOperInHashTable() );
        ENTERCRITICALSECTION    enterCritFCBRCEList( &( pfcb->CritRCEList() ) );
        VERINullifyUncommittedRCE( prce );
    }

    Assert( pfcb->PrceOldest() == prceNil );
    Assert( pfcb->PrceNewest() == prceNil );
}

INLINE VOID VERICleanupForUndoCreateTable( RCE * const prceCreateTable )
{
    FCB * const pfcbTable = prceCreateTable->Pfcb();

    Assert( pfcbTable->FInitialized() );
    Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeTemporaryTable() );
    Assert( pfcbTable->FPrimaryIndex() );

    Assert( operCreateTable == prceCreateTable->Oper() );
    Assert( pfcbTable->PrceOldest() == prceCreateTable );

    Assert( prceCreateTable->Pfucb() != pfucbNil );
    PIB * const ppib    = prceCreateTable->Pfucb()->ppib;
    Assert( ppibNil != ppib );

    Assert( pfcbTable->Ptdb() != ptdbNil );
    FCB * pfcbT         = pfcbTable->Ptdb()->PfcbLV();

    if ( pfcbNil != pfcbT )
    {
        Assert( pfcbT->FTypeLV() );

        Assert( prceNil == pfcbT->PrceNewest() );

        FUCBCloseAllCursorsOnFCB( ppib, pfcbT );
    }

    for ( pfcbT = pfcbTable->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
    {
        Assert( pfcbT->FTypeSecondaryIndex() );

        Assert( prceNil == pfcbT->PrceNewest() );

        FUCBCloseAllCursorsOnFCB( ppib, pfcbT );
    }

    VERINullifyForUndoCreateTable( ppib, pfcbTable );
    FUCBCloseAllCursorsOnFCB( ppib, pfcbTable );
}

LOCAL VOID VERIUndoCreateTable( PIB * const ppib, RCE * const prce )
{
    FCB     * const pfcb    = prce->Pfcb();

    ASSERT_VALID( ppib );
    Assert( prce->Oper() == operCreateTable );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( pfcb->FInitialized() );
    Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
    Assert( pfcb->FPrimaryIndex() );

    PinstFromPpib( ppib )->RwlTrx( ppib ).LeaveAsReader();

    for ( FCB *pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
    {
        VERIWaitForTasks( PverFromPpib( ppib ), pfcbT, fTrue, fFalse );
    }
    if ( pfcb->Ptdb() != ptdbNil )
    {
        FCB * const pfcbLV = pfcb->Ptdb()->PfcbLV();
        if ( pfcbNil != pfcbLV )
        {
            VERIWaitForTasks( PverFromPpib( ppib ), pfcbLV, fTrue, fFalse );
        }
    }

    PinstFromPpib( ppib )->RwlTrx( ppib ).EnterAsReader();

    pfcb->Lock();
    pfcb->SetDeleteCommitted();
    pfcb->Unlock();

    forever
    {
        FUCB *pfucb = pfucbNil;

        pfcb->Lock();
        pfcb->FucbList().LockForEnumeration();
        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++)
        {
            pfucb = pfcb->FucbList()[ ifucbList ];

            ASSERT_VALID( pfucb );

            Assert( pfucb->ppib == ppib );

            if ( !FFUCBDeferClosed( pfucb ) )
            {
                break;
            }
            pfucb = pfucbNil;
        }
        pfcb->FucbList().UnlockForEnumeration();
        pfcb->Unlock();

        if ( pfucb == pfucbNil )
        {
            break;
        }

        if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
        {
            CallS( ErrDispCloseTable( ( JET_SESID )ppib, ( JET_TABLEID )pfucb ) );
        }
        else
        {
            CallS( ErrFILECloseTable( ppib, pfucb ) );
        }
    }

    VERICleanupForUndoCreateTable( prce );

    pfcb->PrepareForPurge();
    const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

    (VOID)ErrSPFreeFDP( ppib, pfcb, pgnoSystemRoot, fTrue );

    FOSSetCleanupState( fCleanUpStateSavedSavedSaved );

    pfcb->Purge();
}


LOCAL VOID VERIUndoAddColumn( const RCE * const prce )
{
    Assert( prce->Oper() == operAddColumn );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
    Assert( prce->Pfcb() == prce->Pfucb()->u.pfcb );

    FCB         * const pfcbTable   = prce->Pfcb();

    pfcbTable->EnterDDL();

    Assert( pfcbTable->PrceOldest() != prceNil );

    TDB             * const ptdb        = pfcbTable->Ptdb();
    const COLUMNID  columnid            = ( (VERADDCOLUMN*)prce->PbData() )->columnid;
    BYTE            * pbOldDefaultRec   = ( (VERADDCOLUMN*)prce->PbData() )->pbOldDefaultRec;
    FIELD           * const pfield      = ptdb->Pfield( columnid );

    Assert( ( FCOLUMNIDTagged( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() )
            || ( FCOLUMNIDVar( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidVarLast() )
            || ( FCOLUMNIDFixed( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidFixedLast() ) );

    Assert( !FFIELDDeleted( pfield->ffield ) );
    FIELDSetDeleted( pfield->ffield );

    FIELDResetVersioned( pfield->ffield );
    FIELDResetVersionedAdd( pfield->ffield );

    Assert( !( FFIELDVersion( pfield->ffield ) && FFIELDAutoincrement( pfield->ffield ) ) );
    if ( FFIELDVersion( pfield->ffield ) )
    {
        Assert( ptdb->FidVersion() == FidOfColumnid( columnid ) );
        ptdb->ResetFidVersion();
    }
    else if ( FFIELDAutoincrement( pfield->ffield ) )
    {
        Assert( ptdb->FidAutoincrement() == FidOfColumnid( columnid ) );
        ptdb->ResetFidAutoincrement();
        ptdb->ResetAutoIncInitOnce();
    }

    if ( 0 != pfield->itagFieldName )
    {
        ptdb->MemPool().DeleteEntry( pfield->itagFieldName );
    }



    if ( NULL != pbOldDefaultRec
        && (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec )
    {
        Assert( !FFIELDUserDefinedDefault( pfield->ffield ) );

        for ( RECDANGLING * precdangling = pfcbTable->Precdangling();
            ;
            precdangling = precdangling->precdanglingNext )
        {
            if ( NULL == precdangling )
            {
                Assert( NULL == ( (RECDANGLING *)pbOldDefaultRec )->precdanglingNext );
                ( (RECDANGLING *)pbOldDefaultRec )->precdanglingNext = pfcbTable->Precdangling();
                pfcbTable->SetPrecdangling( (RECDANGLING *)pbOldDefaultRec );
                break;
            }
            else if ( (BYTE *)precdangling == pbOldDefaultRec )
            {
                break;
            }
        }
    }

    pfcbTable->LeaveDDL();
}


LOCAL VOID VERIUndoDeleteColumn( const RCE * const prce )
{
    Assert( prce->Oper() == operDeleteColumn );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( prce->CbData() == sizeof(COLUMNID) );
    Assert( prce->Pfcb() == prce->Pfucb()->u.pfcb );

    FCB         * const pfcbTable   = prce->Pfcb();

    pfcbTable->EnterDDL();

    Assert( pfcbTable->PrceOldest() != prceNil );

    TDB             * const ptdb        = pfcbTable->Ptdb();
    const COLUMNID  columnid            = *( (COLUMNID*)prce->PbData() );
    FIELD           * const pfield      = ptdb->Pfield( columnid );

    Assert( ( FCOLUMNIDTagged( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() )
            || ( FCOLUMNIDVar( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidVarLast() )
            || ( FCOLUMNIDFixed( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidFixedLast() ) );

    Assert( pfield->coltyp != JET_coltypNil );
    Assert( FFIELDDeleted( pfield->ffield ) );
    FIELDResetDeleted( pfield->ffield );

    if ( FFIELDVersioned( pfield->ffield ) )
    {
        if ( !FFIELDVersionedAdd( pfield->ffield ) )
        {
            FIELDResetVersioned( pfield->ffield );
        }
    }
    else
    {
        Assert( !FFIELDVersionedAdd( pfield->ffield ) );
    }

    pfcbTable->LeaveDDL();
}


LOCAL VOID VERIUndoDeleteTable( const RCE * const prce )
{
    if ( FFMPIsTempDB( prce->Ifmp() ) )
    {
        return;
    }

    Assert( prce->Oper() == operDeleteTable );
    Assert( prce->TrxCommitted() == trxMax );

    INT fState;
    FCB * const pfcbTable = FCB::PfcbFCBGet( prce->Ifmp(), *(PGNO*)prce->PbData(), &fState );
    Assert( pfcbTable != pfcbNil );
    Assert( pfcbTable->FTypeTable() );
    Assert( fFCBStateInitialized == fState );

    if ( pfcbTable->Ptdb() != ptdbNil )
    {
        Assert( fFCBStateInitialized == fState );

        pfcbTable->EnterDDL();

        for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( pfcbT->FDeletePending() );

            if (!pfcbT->FDeleteCommitted())
            {
                pfcbT->Lock();
                pfcbT->ResetDeletePending();
                pfcbT->Unlock();
            }
        }

        FCB * const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
        if ( pfcbNil != pfcbLV )
        {
            Assert( pfcbLV->FDeletePending() );
            Assert( !pfcbLV->FDeleteCommitted() );
            pfcbLV->Lock();
            pfcbLV->ResetDeletePending();
            pfcbLV->Unlock();
        }

        pfcbTable->LeaveDDL();

        pfcbTable->Release();
    }
    else
    {
        FireWall( "DeprecatedSentinelFcbUndoDelTable" );
        Assert( pfcbTable->FDeletePending() );
        Assert( !pfcbTable->FDeleteCommitted() );
        pfcbTable->Lock();
        pfcbTable->ResetDeletePending();
        pfcbTable->Unlock();

        pfcbTable->PrepareForPurge();
        pfcbTable->Purge();
    }
}


LOCAL VOID VERIUndoCreateLV( PIB *ppib, const RCE * const prce )
{
    if ( pfcbNil != prce->Pfcb()->Ptdb()->PfcbLV() )
    {
        FCB * const pfcbLV = prce->Pfcb()->Ptdb()->PfcbLV();

        PinstFromPpib( ppib )->RwlTrx( ppib ).LeaveAsReader();

        VERIWaitForTasks( PverFromPpib( ppib ), pfcbLV, fTrue, fFalse );

        PinstFromPpib( ppib )->RwlTrx( ppib ).EnterAsReader();


        VERIUnlinkDefunctLV( prce->Pfucb()->ppib, pfcbLV );
        pfcbLV->PrepareForPurge( fFalse );
        pfcbLV->Purge();
        prce->Pfcb()->Ptdb()->SetPfcbLV( pfcbNil );
    }
}


LOCAL VOID VERIUndoCreateIndex( PIB *ppib, const RCE * const prce )
{
    Assert( prce->Oper() == operCreateIndex );
    Assert( prce->TrxCommitted() == trxMax );
    Assert( prce->CbData() == sizeof(TDB *) );

    FCB * const pfcb = *(FCB **)prce->PbData();
    FCB * const pfcbTable = prce->Pfucb()->u.pfcb;

    Assert( pfcbNil != pfcbTable );
    Assert( pfcbTable->FTypeTable() );
    Assert( pfcbTable->PrceOldest() != prceNil );


    if ( pfcb != pfcbNil )
    {
        Assert( prce->Pfucb()->u.pfcb != pfcb );

        Assert( pfcb->FTypeSecondaryIndex() );
        Assert( pfcb->Pidb() != pidbNil );

        CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock();

        pfcbTable->SetIndexing();
        pfcbTable->EnterDDL();

        Assert( !pfcb->Pidb()->FDeleted() );
        Assert( !pfcb->FDeletePending() );
        Assert( !pfcb->FDeleteCommitted() );

        pfcb->Pidb()->SetFDeleted();
        pfcb->Pidb()->ResetFVersioned();

        if ( pfcb->PfcbTable() == pfcbNil )
        {
            pfcb->UnlinkIDB( pfcbTable );
        }
        else
        {
            Assert( pfcb->PfcbTable() == pfcbTable );

            pfcbTable->UnlinkSecondaryIndex( pfcb );

            FILESetAllIndexMask( pfcbTable );
        }

        pfcbTable->LeaveDDL();
        pfcbTable->ResetIndexing();

        PinstFromPpib( ppib )->RwlTrx( ppib ).LeaveAsReader();

        VERNullifyAllVersionsOnFCB( pfcb );

        VERIWaitForTasks( PverFromPpib( ppib ), pfcb, fTrue, fFalse );

        pfcb->Lock();
        pfcb->SetDeleteCommitted();
        pfcb->Unlock();

        
        VERIUnlinkDefunctSecondaryIndex( prce->Pfucb()->ppib, pfcb );

        PinstFromPpib( ppib )->RwlTrx( ppib ).EnterAsReader();

        pfcb->PrepareForPurge( fFalse );

        const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

        (VOID)ErrSPFreeFDP( ppib, pfcb, pfcbTable->PgnoFDP(), fTrue );

        FOSSetCleanupState( fCleanUpStateSavedSavedSaved );

        pfcb->Purge();
    }

    else if ( pfcbTable->Pidb() != pidbNil )
    {
        pfcbTable->EnterDDL();

        IDB     *pidb   = pfcbTable->Pidb();

        Assert( !pidb->FDeleted() );
        Assert( !pfcbTable->FDeletePending() );
        Assert( !pfcbTable->FDeleteCommitted() );
        Assert( !pfcbTable->FSequentialIndex() );

        pidb->SetFDeleted();
        pidb->ResetFVersioned();
        pidb->ResetFVersionedCreate();

        pfcbTable->UnlinkIDB( pfcbTable );
        pfcbTable->SetPidb( pidbNil );
        pfcbTable->Lock();
        pfcbTable->SetSequentialIndex();
        pfcbTable->Unlock();

        pfcbTable->SetSpaceHints( PSystemSpaceHints(eJSPHDefaultUserIndex) );

        FILESetAllIndexMask( pfcbTable );

        pfcbTable->LeaveDDL();
    }
}


LOCAL VOID VERIUndoDeleteIndex( const RCE * const prce )
{
    FCB * const pfcbIndex = *(FCB **)prce->PbData();
    FCB * const pfcbTable = prce->Pfcb();

    Assert( prce->Oper() == operDeleteIndex );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( pfcbTable->FTypeTable() );
    Assert( pfcbIndex->PfcbTable() == pfcbTable );
    Assert( prce->CbData() == sizeof(FCB *) );
    Assert( pfcbIndex->FTypeSecondaryIndex() );

    pfcbIndex->ResetDeleteIndex();
}


INLINE VOID VERIUndoAllocExt( const RCE * const prce )
{
    Assert( prce->CbData() == sizeof(VEREXT) );
    Assert( prce->PgnoFDP() == ((VEREXT*)prce->PbData())->pgnoFDP );

    const VEREXT* const pverext = (const VEREXT*)(prce->PbData());

    VERIFreeExt( prce->Pfucb()->ppib, prce->Pfcb(),
        pverext->pgnoFirst,
        pverext->cpgSize );
}


INLINE VOID VERIUndoRegisterCallback( const RCE * const prce )
{
    VERIRemoveCallback( prce );
}


VOID VERIUndoUnregisterCallback( const RCE * const prce )
{
#ifdef VERSIONED_CALLBACKS
    Assert( prce->CbData() == sizeof(VERCALLBACK) );
    const VERCALLBACK* const pvercallback = (VERCALLBACK*)prce->PbData();
    CBDESC * const pcbdesc = pvercallback->pcbdesc;
    prce->Pfcb()->EnterDDL();
    Assert( trxMax != pcbdesc->trxRegisterBegin0 );
    Assert( trxMax != pcbdesc->trxRegisterCommit0 );
    Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
    Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
    pvercallback->pcbdesc->trxUnregisterBegin0 = trxMax;
#endif
    prce->Pfcb()->LeaveDDL();
}


INLINE VOID VERIUndoNonLoggedOper( PIB *ppib, RCE * const prce, RCE **pprceNextToUndo )
{
    Assert( *pprceNextToUndo == prce->PrcePrevOfSession() );

    switch( prce->Oper() )
    {
        case operAllocExt:
            VERIUndoAllocExt( prce );
            break;
        case operDeleteTable:
            VERIUndoDeleteTable( prce );
            break;
        case operAddColumn:
            VERIUndoAddColumn( prce );
            break;
        case operDeleteColumn:
            VERIUndoDeleteColumn( prce );
            break;
        case operCreateLV:
            VERIUndoCreateLV( ppib, prce );
            break;
        case operCreateIndex:
            VERIUndoCreateIndex( ppib, prce );
            *pprceNextToUndo = prce->PrcePrevOfSession();
            break;
        case operDeleteIndex:
            VERIUndoDeleteIndex( prce );
            break;
        case operRegisterCallback:
            VERIUndoRegisterCallback( prce );
            break;
        case operUnregisterCallback:
            VERIUndoUnregisterCallback( prce );
            break;
        case operReadLock:
        case operWriteLock:
            break;
        case operPreInsert:
        default:
            Assert( fFalse );
            break;

        case operCreateTable:
            VERIUndoCreateTable( ppib, prce );
            Assert( prce->FOperNull() );
            return;
    }

    Assert( !prce->FOperNull() );
    Assert( !prce->FRolledBack() );

    VERINullifyRolledBackRCE( ppib, prce );
}


#define MAX_ROLLBACK_RETRIES 100

ERR ErrVERRollback( PIB *ppib )
{
    ASSERT_VALID( ppib );
    Assert( ppib->Level() > 0 );

    const   LEVEL   level               = ppib->Level();
    INT     cRepeat = 0;

    ERR err = JET_errSuccess;
    
    if ( prceNil != ppib->prceNewest )
    {
        ENTERREADERWRITERLOCK rwlTrx( &PinstFromPpib( ppib )->RwlTrx( ppib ), fTrue );
    
        RCE *prceToUndo;
        RCE *prceNextToUndo;
        RCE *prceToNullify;
        for( prceToUndo = ppib->prceNewest;
            prceNil != prceToUndo && prceToUndo->Level() == level;
            prceToUndo = prceNextToUndo )
        {
            Assert( !prceToUndo->FOperNull() );
            Assert( prceToUndo->Level() <= level );
            Assert( prceToUndo->TrxCommitted() == trxMax );
            Assert( prceToUndo->Pfcb() != pfcbNil );
            Assert( !prceToUndo->FRolledBack() );
    
            prceNextToUndo = prceToUndo->PrcePrevOfSession();
    
            if ( prceToUndo->FUndoableLoggedOper() )
            {
                Assert( pfucbNil != prceToUndo->Pfucb() );
                Assert( ppib == prceToUndo->Pfucb()->ppib );
                Assert( JET_errSuccess == ppib->ErrRollbackFailure() );
    
    
                err = ErrVERIUndoLoggedOper( ppib, prceToUndo );
                if ( err < JET_errSuccess )
                {
                    if ( JET_errRollbackError == err )
                    {
                        Assert( ppib->ErrRollbackFailure() < JET_errSuccess );
                        Call ( err );
                    }
                    else if ( cRepeat > MAX_ROLLBACK_RETRIES )
                    {
                        EnforceSz( fFalse, OSFormat( "TooManyRollbackRetries1:Err%d:Op%u", err, prceToUndo->Oper() ) );
                    }
                    else
                    {
                        cRepeat++;
                        prceNextToUndo = prceToUndo;
                        continue;
                    }
                    Assert ( fFalse );
                }
            }
    
            else
            {
                VERIUndoNonLoggedOper( ppib, prceToUndo, &prceNextToUndo );
            }
    
            cRepeat = 0;
        }
    
        prceToNullify = ppib->prceNewest;
        while ( prceNil != prceToNullify && prceToNullify->Level() == level )
        {
            Assert( pfucbNil != prceToNullify->Pfucb() );
            Assert( ppib == prceToNullify->Pfucb()->ppib );
            Assert( !prceToNullify->FOperNull() );
            Assert( prceToNullify->FOperAffectsSecondaryIndex() );
            Assert( prceToNullify->FRolledBack() );
            Assert( prceToNullify->Pfcb()->FTypeTable() );
            Assert( !prceToNullify->Pfcb()->FFixedDDL() );
    
            RCE *prceNextToNullify;
            VERINullifyRolledBackRCE( ppib, prceToNullify, &prceNextToNullify );
    
            prceToNullify = prceNextToNullify;
        }
    }

    Assert( prceNil == ppib->prceNewest
        || ppib->prceNewest->Level() <= level );

    Assert( level == ppib->Level() );
    if ( 1 == ppib->Level() )
    {
        Assert( prceNil == ppib->prceNewest );
        PIBSetPrceNewest( ppib, prceNil );
    }
    Assert( ppib->Level() > 0 );
    ppib->DecrementLevel();

HandleError:
    return err;
}


ERR ErrVERRollback( PIB *ppib, UPDATEID updateid )
{
    ASSERT_VALID( ppib );
    Assert( ppib->Level() > 0 );

    Assert( updateidNil != updateid );

#ifdef DEBUG
    const   LEVEL   level   = ppib->Level();

    Unused( level );
#endif

    INT     cRepeat = 0;
    ERR err = JET_errSuccess;

    PinstFromPpib( ppib )->RwlTrx( ppib ).EnterAsReader();

    RCE *prceToUndo;
    RCE *prceNextToUndo;

    for( prceToUndo = ppib->prceNewest;
        prceNil != prceToUndo && prceToUndo->TrxCommitted() == trxMax;
        prceToUndo = prceNextToUndo )
    {
        prceNextToUndo = prceToUndo->PrcePrevOfSession();

        if ( prceToUndo->Updateid() == updateid )
        {
            Assert( pfucbNil != prceToUndo->Pfucb() );
            Assert( ppib == prceToUndo->Pfucb()->ppib );
            Assert( !prceToUndo->FOperNull() );
            Assert( prceToUndo->Pfcb() != pfcbNil );
            Assert( !prceToUndo->FRolledBack() );

            Assert( prceToUndo->FUndoableLoggedOper() );
            Assert( JET_errSuccess == ppib->ErrRollbackFailure() );

            err = ErrVERIUndoLoggedOper( ppib, prceToUndo );
            if ( err < JET_errSuccess )
            {
                if ( JET_errRollbackError == err )
                {
                    Assert( ppib->ErrRollbackFailure() < JET_errSuccess );
                    Call( err );
                }
                else
                {
                    cRepeat++;
                    EnforceSz( cRepeat < MAX_ROLLBACK_RETRIES, OSFormat( "TooManyRollbackRetries2:Err%d:Op%u", err, prceToUndo->Oper() ) );
                    prceNextToUndo = prceToUndo;
                    continue;
                }
                Assert ( fFalse );
            }
        }

        cRepeat = 0;
    }


    RCE *prceToNullify;
    prceToNullify = ppib->prceNewest;
    while ( prceNil != prceToNullify && prceToNullify->TrxCommitted() == trxMax )
    {
        if ( prceToNullify->Updateid() == updateid )
        {
            Assert( pfucbNil != prceToNullify->Pfucb() );
            Assert( ppib == prceToNullify->Pfucb()->ppib );
            Assert( !prceToNullify->FOperNull() );
            Assert( prceToNullify->FOperAffectsSecondaryIndex() );
            Assert( prceToNullify->FRolledBack() );
            Assert( prceToNullify->Pfcb()->FTypeTable() );
            Assert( !prceToNullify->Pfcb()->FFixedDDL() );

            RCE *prceNextToNullify;
            VERINullifyRolledBackRCE( ppib, prceToNullify, &prceNextToNullify );

            prceToNullify = prceNextToNullify;
        }
        else
        {
            prceToNullify = prceToNullify->PrcePrevOfSession();
        }
    }

HandleError:
    PinstFromPpib( ppib )->RwlTrx( ppib ).LeaveAsReader();

    return err;
}

JETUNITTEST( VER, RceidCmpReturnsZeroWhenRceidsAreEqual )
{
    CHECK(0 == RceidCmp(5,5));
}

JETUNITTEST( VER, RceidCmpGreaterThan )
{
    CHECK(RceidCmp(11,5) > 0);
}

JETUNITTEST( VER, RceidCmpGreaterThanWraparound )
{
    CHECK(RceidCmp(3,0xFFFFFFFD) > 0);
}

JETUNITTEST( VER, RceidCmpLessThan )
{
    CHECK(RceidCmp(21,35) < 0);
}

JETUNITTEST( VER, RceidCmpLessThanWraparound )
{
    CHECK(RceidCmp(0xFFFFFF0F,1) < 0);
}

JETUNITTEST( VER, RceidNullIsLessThanAnything )
{
    CHECK(RceidCmp(rceidNull,0xFFFFFFFF) < 0);
}

JETUNITTEST( VER, AnythingIsGreaterThanRceidNull )
{
    CHECK(RceidCmp(1,rceidNull) > 0);
}

