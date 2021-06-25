// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"
#include "_bt.hxx"
#include "_ver.hxx"

#include "PageSizeClean.hxx"


///#define BREAK_ON_PREFERRED_BUCKET_LIMIT

#ifdef DEBUG

//  DEBUG_VER:  check the consistency of the version store hash table
///#define DEBUG_VER

//  DEBUG_VER_EXPENSIVE:  time-consuming version store consistency check
///#define DEBUG_VER_EXPENSIVE

#endif  //  DEBUG

//  forward declarations

BOOL FResCloseToQuota( INST * const pinst, JET_RESID resid );

#ifdef PERFMON_SUPPORT

//  ****************************************************************
//  PERFMON STATISTICS
//  ****************************************************************

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


//  ================================================================
LONG LVERcbucketAllocatedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERcbucketAllocated.PassTo( iInstance, pvBuf );
    return 0;
}


//  ================================================================
LONG LVERcbucketDeleteAllocatedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
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


//  ================================================================
LONG LVERBucketAllocWaitForRCECleanCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERBucketAllocWaitForRCEClean.PassTo( iInstance, pvBuf );
    return 0;
}


//  ================================================================
LONG LVERcbAverageBookmarkCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
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


//  ================================================================
LONG LVERUnnecessaryCallsCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERUnnecessaryCalls.PassTo( iInstance, pvBuf );
    return 0;
}

//  ================================================================
LONG LVERAsyncCleanupDispatchedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERAsyncCleanupDispatched.PassTo( iInstance, pvBuf );
    return 0;
}

//  ================================================================
LONG LVERSyncCleanupDispatchedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERSyncCleanupDispatched.PassTo( iInstance, pvBuf );
    return 0;
}

//  ================================================================
LONG LVERCleanupDiscardedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERCleanupDiscarded.PassTo( iInstance, pvBuf );
    return 0;
}

//  ================================================================
LONG LVERCleanupFailedCEFLPv( LONG iInstance, VOID * pvBuf )
//  ================================================================
{
    cVERCleanupFailed.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT



//  ****************************************************************
//  GLOBALS
//  ****************************************************************

const DIRFLAG   fDIRUndo = fDIRNoLog | fDIRNoVersion | fDIRNoDirty;

//  exported
volatile LONG   g_crefVERCreateIndexLock  = 0;


//  ****************************************************************
//  VER class
//  ****************************************************************

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
#endif  //  VERPERF
        // RAND_MAX is only 32K so there is no underflow risk
        m_rceidLast( 0xFFFFFFFF - ( rand() * 2 ) ),
        m_rectaskbatcher(
            pinst,
            (INT)UlParam(pinst, JET_paramVersionStoreTaskQueueMax),
            ctasksPerBatchMaxDefault,
            ctasksBatchedMaxDefault ),
        m_cresBucket( pinst )
{

    //  initialised to the Set state (it should
    //  be impossible to wait on the signal if
    //  it didn't first get reset, but this
    //  will guarantee it)
    //
    m_msigRCECleanPerformedRecently.Set();

#ifdef VERPERF
    HRT hrtStartHrts = HrtHRTCount();
#endif

    // m_rceidLast must be odd so that it can never be equal to rceidNull
    Assert(0 != (m_rceidLast % 2));
    Assert(0 == (rceidNull % 2));
}

VER::~VER()
{
}



//  ****************************************************************
//  BUCKET LAYER
//  ****************************************************************
//
//  A bucket is a contiguous block of memory used to hold versions.
//
//-

//  ================================================================
INLINE size_t VER::CbBUFree( const BUCKET * pbucket )
//  ================================================================
//
//-
{
    const size_t cbBUFree = m_cbBucket - ( (BYTE*)pbucket->hdr.prceNextNew - (BYTE*)pbucket );
    Assert( cbBUFree < m_cbBucket );
    return cbBUFree;
}


//  ================================================================
INLINE BOOL VER::FVERICleanDiscardDeletes()
//  ================================================================
//
//  If the version store is really full we will simply discard the
//  RCEs (only if not cleaning the last bucket)
//
{
    DWORD_PTR       cbucketMost     = 0;
    DWORD_PTR       cbucket         = 0;
    BOOL            fDiscardDeletes = fFalse;

    CallS( m_cresBucket.ErrGetParam( JET_resoperMaxUse, &cbucketMost ) );
    CallS( m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cbucket ) );

    if ( cbucket > min( cbucketMost, UlParam( m_pinst, JET_paramPreferredVerPages ) ) )
    {
        //  discard deletes if we've exceeded the preferred threshold
        //  or if the task manager is being overrun
        //
        fDiscardDeletes = fTrue;
    }
    else if ( m_pbucketGlobalHead != m_pbucketGlobalTail
            && ( cbucketMost - cbucket ) < 2 )
    {
        //  discard deletes if this is not the only bucket and there are
        //  less than two buckets left
        //
        fDiscardDeletes = fTrue;
    }
    else if ( FResCloseToQuota( m_pinst, JET_residFCB ) )
    {
        //  discard deletes if we are running out of FCBs for
        //  active instances.
        fDiscardDeletes = fTrue;
    }

    return fDiscardDeletes;
}

VOID VER::VERIReportDiscardedDeletes( const RCE * const prce )
{
    Assert( m_critRCEClean.FOwner() );

    FMP * const pfmp        = g_rgfmp + prce->Ifmp();

    //  UNDONE: there's a small window here where the RCE could get nullified
    //  after we do the FOperNull() check but before we can extract the trxBegin0,
    //  in which case we'll assert in DBG that the RCE is invalid to read. This is
    //  actually harmless, but it's too much of a pain to try to completely close
    //  the concurrency hole just for an assert, so just ignore the assert if
    //  it ever goes off. We know the RCE could not be going away (because we
    //  have critRCEClean), so it's fine to read trxBegin0 from it even if it
    //  gets nullified underneath us. Nevertheless, a second FOperNull() check is
    //  done anyway just to be safe and ensure we don't read some bogus trxBegin0
    //  value.
    //
    const TRX   trxBegin0   = ( prce->FOperNull() ? trxMax : prce->TrxBegin0() );

    //  if OLD is already running on this database,
    //  don't bother reporting discarded deletes
    //
    //  we keep track of the NEWEST trx at the time discards were last
    //  reported, and will only generate future discard reports for
    //  deletes that occurred after the current trxNewest (this is a
    //  means of ensuring we don't report too often)
    //
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

        //  this ensures no further reports will be generated for
        //  deletes which were present in the version store at
        //  the time this report was generated
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

    //  only generate the eventlog entry once per long-running transaction
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
                // Report cases where we have hit max trx size but not yet VSOOM.
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
                //  OOM because NT returned OOM
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
                //  OOM because the user-specified max. has been reached
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


//  ================================================================
INLINE ERR VER::ErrVERIBUAllocBucket( const INT cbRCE, const UINT uiHash )
//  ================================================================
//
//  Inserts a bucket to the top of the bucket chain, so that new RCEs
//  can be inserted.  Note that the caller must set ibNewestRCE himself.
//
//-
{
    //  use m_critBucketGlobal to make sure only one allocation
    //  occurs at one time.
    Assert( m_critBucketGlobal.FOwner() );

    //  caller would only call this routine if they determined
    //  that current bucket was not enough to satisfy allocation
    //
    Assert( m_pbucketGlobalHead == pbucketNil
        || (size_t)cbRCE > CbBUFree( m_pbucketGlobalHead ) );

    //  signal RCE clean in case it can now do work
    VERSignalCleanup();

    //  Must allocate within m_critBucketGlobal to make sure that
    //  the allocated bucket will be in circularly increasing order.
    BUCKET *  pbucket = new( this ) BUCKET( this );

    //  We are really out of bucket, return to high level function
    //  call to retry.
    if ( pbucketNil == pbucket )
    {
        m_critBucketGlobal.Leave();

        if ( uiHashInvalid != uiHash )
        {
            RwlRCEChain( uiHash ).LeaveAsWriter();
        }

        //  ensure RCE clean was performed recently (if our wait times out, it
        //  means something is horribly wrong and blocking version cleanup)
        //
        const BOOL  fCleanupWasRun  = m_msigRCECleanPerformedRecently.FWait( cmsecAsyncBackgroundCleanup );

        PERFOpt( cVERBucketAllocWaitForRCEClean.Inc( m_pinst ) );

        if ( uiHashInvalid != uiHash )
        {
            RwlRCEChain( uiHash ).EnterAsWriter();
        }

        m_critBucketGlobal.Enter();

        //  see if someone else beat us to it
        //
        if ( m_pbucketGlobalHead == pbucketNil || (size_t)cbRCE > CbBUFree( m_pbucketGlobalHead ) )
        {
            //  retry allocation
            //
            pbucket = new( this ) BUCKET( this );

            //  check again
            //
            if ( pbucketNil == pbucket )
            {
                //  still couldn't allocate bucket, so bail with
                //  appropriate error
                //
                VERIReportVersionStoreOOM( NULL, fFalse /* fMaxTrxSize */, fCleanupWasRun );
                return ErrERRCheck( fCleanupWasRun ?
                                        JET_errVersionStoreOutOfMemory :
                                        JET_errVersionStoreOutOfMemoryAndCleanupTimedOut );
            }
        }
        else
        {
            //  someone allocated a bucket for us, so just bail
            //
            return JET_errSuccess;
        }
    }

    //  ensure RCE's will be properly aligned
    Assert( pbucketNil != pbucket );
    Assert( FAlignedForThisPlatform( pbucket ) );
    Assert( FAlignedForThisPlatform( pbucket->rgb ) );
    Assert( (BYTE *)PvAlignForThisPlatform( pbucket->rgb ) == pbucket->rgb );

    //  Link up the bucket to global list.
    pbucket->hdr.pbucketPrev = m_pbucketGlobalHead;
    if ( pbucket->hdr.pbucketPrev )
    {
        //  there is a bucket after us
        pbucket->hdr.pbucketPrev->hdr.pbucketNext = pbucket;
    }
    else
    {
        //  we are last in the chain
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

    //  if RCE doesn't fit into an empty bucket, something is horribly wrong
    //
    Assert( (size_t)cbRCE <= CbBUFree( pbucket ) );
    return ( (size_t)cbRCE > CbBUFree( pbucket ) ? ErrERRCheck( JET_errVersionStoreEntryTooBig ) : JET_errSuccess );
}


//  ================================================================
INLINE BUCKET *VER::PbucketVERIGetOldest( )
//  ================================================================
//
//  find the oldest bucket in the bucket chain
//
//-
{
    Assert( m_critBucketGlobal.FOwner() );

    BUCKET  * const pbucket = m_pbucketGlobalTail;

    Assert( pbucketNil == pbucket || pbucketNil == pbucket->hdr.pbucketPrev );
    return pbucket;
}


//  ================================================================
BUCKET *VER::PbucketVERIFreeAndGetNextOldestBucket( BUCKET * pbucket )
//  ================================================================
{
    Assert( m_critBucketGlobal.FOwner() );

    BUCKET * const pbucketNext = (BUCKET *)pbucket->hdr.pbucketNext;
    BUCKET * const pbucketPrev = (BUCKET *)pbucket->hdr.pbucketPrev;

    //  unlink bucket from bucket chain and free.
    if ( pbucketNil != pbucketNext )
    {
        Assert( m_pbucketGlobalHead != pbucket );
        pbucketNext->hdr.pbucketPrev = pbucketPrev;
    }
    else    //  ( pbucketNil == pbucketNext )
    {
        Assert( m_pbucketGlobalHead == pbucket );
        m_pbucketGlobalHead = pbucketPrev;
    }

    if ( pbucketNil != pbucketPrev )
    {
        Assert( m_pbucketGlobalTail != pbucket );
        pbucketPrev->hdr.pbucketNext = pbucketNext;
    }
    else    //  ( pbucketNil == pbucketPrev )
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


//  ================================================================
INLINE CReaderWriterLock& VER::RwlRCEChain( UINT ui )
//  ================================================================
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

//  ================================================================
CReaderWriterLock& RCE::RwlChain()
//  ================================================================
{
    Assert( ::FOperInHashTable( m_oper ) );
    Assert( uiHashInvalid != m_uiHash );
    return PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash );
}


//  ================================================================
LOCAL UINT UiRCHashFunc( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const UINT crcehead = 0 )
//  ================================================================
{
    ASSERT_VALID( &bookmark );
    Assert( pgnoNull != pgnoFDP );

    const UINT crceheadHashTable = crcehead ? crcehead : PverFromIfmp( ifmp )->m_crceheadHashTable;

    //  An optimized version of the hash function from "Algorithms in C++" by Sedgewick

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

//  ================================================================
UINT UiVERHash( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const UINT crcehead )
//  ================================================================
//
//  Used by the debugger extension
//
{
    return UiRCHashFunc( ifmp, pgnoFDP, bookmark, crcehead );
}

#endif  //  DEBUGGER_EXTENSION

#ifdef DEBUG


//  ================================================================
BOOL RCE::FAssertRwlHash_() const
//  ================================================================
{
    Assert( ::FOperInHashTable( m_oper ) );
    return  PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash ).FReader() ||
            PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash ).FWriter();
}

//  ================================================================
BOOL RCE::FAssertRwlHashAsWriter_() const
//  ================================================================
{
    Assert( ::FOperInHashTable( m_oper ) );
    return  PverFromIfmp( Ifmp() )->RwlRCEChain( m_uiHash ).FWriter();
}


//  ================================================================
BOOL RCE::FAssertCritFCBRCEList_() const
//  ================================================================
{
    Assert( !::FOperNull( m_oper ) );
    return Pfcb()->CritRCEList().FOwner();
}


//  ================================================================
BOOL RCE::FAssertCritPIB_() const
//  ================================================================
{
    Assert( !FFullyCommitted() );
    Assert( !::FOperNull( m_oper ) );
    return ( m_pfucb->ppib->CritTrx().FOwner() || Ptls()->fAddColumn );
}


//  ================================================================
BOOL RCE::FAssertReadable_() const
//  ================================================================
//
//  We can read the const members of a RCE if we are holding an accessing
//  critical section or the RCE is committed but younger than the oldest active
//  transaction or the RCE is older than the oldest transaction and we are
//  RCECleanup
//
//-
{
    AssertRTL( !::FOperNull( m_oper ) );
    return fTrue;
}


#endif  //  DEBUG

//  ================================================================
VOID RCE::AssertValid() const
//  ================================================================
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
        //  No-one can see these members if we are not in the hash table
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
        //  we are committed to level 0
        AssertRTL( 0 == m_level );
        AssertRTL( prceNil == m_prceNextOfSession );
        AssertRTL( prceNil == m_prcePrevOfSession );
    }

#ifdef SYNC_DEADLOCK_DETECTION
    //  UNDONE: must be in critRCEChain to make these checks
    //  (otherwise the RCE might get nullified from underneath us),
    //  but the FOwner() check currently doesn't work properly if
    //  SYNC_DEADLOCK_DETECTION is disabled

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
                            //  these are the only valid operations after a delete
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

#endif  //  SYNC_DEADLOCK_DETECTION

}


#ifndef RTM


//  ================================================================
ERR VER::ErrCheckRCEChain( const RCE * const prce, const UINT uiHash ) const
//  ================================================================
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


//  ================================================================
ERR VER::ErrCheckRCEHashList( const RCE * const prce, const UINT uiHash ) const
//  ================================================================
{
    ERR err = JET_errSuccess;
    const RCE * prceT = prce;
    for ( ; prceNil != prceT; prceT = prceT->PrceHashOverflow() )
    {
        CallR( ErrCheckRCEChain( prceT, uiHash ) );
    }
    return err;
}


//  ================================================================
ERR VER::ErrInternalCheck()
//  ================================================================
//
//  check the consistency of the entire hash table, entering the
//  critical sections if needed
//
//-
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


#endif  //  !RTM


//  ================================================================
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
//  ================================================================
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
    m_fRolledBack( fFalse ),            //  protected by m_critBucketGlobal
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
#endif  //  DEBUG

    }


//  ================================================================
INLINE BYTE * RCE::PbBookmark()
//  ================================================================
{
    Assert( FAssertRwlHash_() );
    return m_rgbData + m_cbData;
}


//  ================================================================
INLINE RCE *&RCE::PrceHashOverflow()
//  ================================================================
{
    Assert( FAssertRwlHash_() );
    return m_prceHashOverflow;
}


//  ================================================================
INLINE VOID RCE::SetPrceHashOverflow( RCE * prce )
//  ================================================================
{
    Assert( FAssertRwlHashAsWriter_() );
    m_prceHashOverflow = prce;
}


//  ================================================================
INLINE VOID RCE::SetPrceNextOfNode( RCE * prce )
//  ================================================================
{
    Assert( FAssertRwlHashAsWriter_() );
    Assert( prceNil == prce
        || RceidCmp( m_rceid, prce->Rceid() ) < 0 );
    m_prceNextOfNode = prce;
}


//  ================================================================
INLINE VOID RCE::SetPrcePrevOfNode( RCE * prce )
//  ================================================================
{
    Assert( FAssertRwlHashAsWriter_() );
    Assert( prceNil == prce
        || RceidCmp( m_rceid, prce->Rceid() ) > 0 );
    m_prcePrevOfNode = prce;
}


//  ================================================================
INLINE VOID RCE::FlagRolledBack()
//  ================================================================
{
    Assert( FAssertCritPIB_() );
    m_fRolledBack = fTrue;
}


//  ================================================================
INLINE VOID RCE::FlagMoved()
//  ================================================================
{
    m_fMoved = fTrue;
}


//  ================================================================
INLINE VOID RCE::SetPrcePrevOfSession( RCE * prce )
//  ================================================================
{
    m_prcePrevOfSession = prce;
}


//  ================================================================
INLINE VOID RCE::SetPrceNextOfSession( RCE * prce )
//  ================================================================
{
    m_prceNextOfSession = prce;
}


//  ================================================================
INLINE VOID RCE::SetLevel( LEVEL level )
//  ================================================================
{
    Assert( FAssertCritPIB_() || PinstFromIfmp( m_ifmp )->m_plog->FRecovering() );
    Assert( m_level > level );  // levels are always decreasing
    m_level = level;
}


//  ================================================================
INLINE VOID RCE::SetTrxCommitted( const TRX trx )
//  ================================================================
{
    Assert( !FOperInHashTable() || FAssertRwlHashAsWriter_() );
    Assert( FAssertCritPIB_() || PinstFromIfmp( m_ifmp )->m_plog->FRecovering() );
    Assert( FAssertCritFCBRCEList_()  );
    Assert( prceNil == m_prcePrevOfSession );   //  only uncommitted RCEs in session list
    Assert( prceNil == m_prceNextOfSession );
    Assert( prceInvalid == m_prceUndoInfoNext );    //  no before image when committing to 0
    Assert( prceInvalid == m_prceUndoInfoPrev );
    Assert( pgnoNull == m_pgnoUndoInfo );
    Assert( TrxCmp( trx, m_trxBegin0 ) > 0 );

    //  ptrxCommitted should currently be pointing into the
    //  owning sessions's PIB, since the session has now
    //  committed, we want to copy the trxCommit0 into
    //  the RCE and switch ptrxCommitted to now point there
    //
    Assert( trx == *m_ptrxCommitted );
    Assert( trxMax == m_trxCommittedInactive );
    m_trxCommittedInactive = trx;
    m_ptrxCommitted = &m_trxCommittedInactive;
}


//  ================================================================
INLINE VOID RCE::NullifyOper()
//  ================================================================
//
//  This is the end of the RCEs lifetime. It must have been removed
//  from all lists
//
//-
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

    //  make sure ptrxCommitted points into the RCE
    //
    m_ptrxCommitted = &m_trxCommittedInactive;
}


//  ================================================================
INLINE VOID RCE::NullifyOperForMove()
//  ================================================================
//
//  RCE has been copied elsewhere -- nullify this copy
//
//-
{
    m_oper |= ( operMaskNull | operMaskNullForMove );
}


//  ================================================================
LOCAL BOOL FRCECorrect( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark, const RCE * prce )
//  ================================================================
//
//  Checks whether a RCE describes the given BOOKMARK
//
//-
{
    ASSERT_VALID( prce );

    BOOL fRCECorrect = fFalse;

    //  same database and table
    if ( prce->Ifmp() == ifmp && prce->PgnoFDP() == pgnoFDP )
    {
        if ( bookmark.key.Cb() == prce->CbBookmarkKey() && (INT)bookmark.data.Cb() == prce->CbBookmarkData() )
        {
            //  bookmarks are the same length
            BOOKMARK bookmarkRCE;
            prce->GetBookmark( &bookmarkRCE );

            fRCECorrect = ( CmpKeyData( bookmark, bookmarkRCE ) == 0 );
        }
    }
    return fRCECorrect;
}


//  ================================================================
BOOL RCE::FRCECorrectEDBG( IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
//
//  Checks whether a RCE describes the given BOOKMARK
//
//  HACK: This is just a stripped-down version of the function above, with asserts removed
//  so debugger extensions don't trip over them.
//
//-
{
    BOOL fRCECorrect = fFalse;

    if ( m_ifmp == ifmp && m_pgnoFDP == pgnoFDP )
    {
        if ( bookmark.key.Cb() == m_cbBookmarkKey && bookmark.data.Cb() == m_cbBookmarkData )
        {
            //  bookmarks are the same length, so now compare the bytes
            //
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


//  ================================================================
LOCAL RCE **PprceRCEChainGet( UINT uiHash, IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
//
//  Given a BOOKMARK, get the correct RCEHEAD.
//
//-
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
                //  if there is a NextOfNode, then we are not in the hash overflow
                //  chain, so can't check PrceHashOverflow().
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
            //  can only check prceHashOverflow if there is no prceNextOfNode
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


//  ================================================================
LOCAL RCE *PrceRCEGet( UINT uiHash, IFMP ifmp, PGNO pgnoFDP, const BOOKMARK& bookmark )
//  ================================================================
//
//  Given a BOOKMARK, get the correct hash chain of RCEs.
//
//-
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


//  ================================================================
LOCAL RCE * PrceFirstVersion ( const UINT uiHash, const FUCB * pfucb, const BOOKMARK& bookmark )
//  ================================================================
//
//  gets the first version of a node
//
//-
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


//  ================================================================
LOCAL const RCE *PrceVERIGetPrevReplace( const RCE * const prce )
//  ================================================================
//
//  Find previous replace of changed by the session within
//  current transaction. The found rce may be in different transaction level.
//
//-
{
    //  Look for previous replace on this node of this transaction ( level > 0 ).
    const RCE * prcePrevReplace = prce->PrcePrevOfNode();
    for ( ; prceNil != prcePrevReplace
            && !prcePrevReplace->FOperReplace()
            && prcePrevReplace->TrxCommitted() == trxMax;
         prcePrevReplace = prcePrevReplace->PrcePrevOfNode() )
        ;

    //  did we find a previous replace operation at level greater than 0
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


//  ================================================================
BOOL FVERActive( const IFMP ifmp, const PGNO pgnoFDP, const BOOKMARK& bm, const TRX trxSession )
//  ================================================================
//
//  is there a version on the node that is visible to an uncommitted trx?
//
//-
{
    ASSERT_VALID( &bm );

    const UINT              uiHash      = UiRCHashFunc( ifmp, pgnoFDP, bm );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    BOOL                    fVERActive  = fFalse;

    //  get RCE
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


//  ================================================================
BOOL FVERActive( const FUCB * pfucb, const BOOKMARK& bm, const TRX trxSession )
//  ================================================================
//
//  is there a version on the node that is visible to an uncommitted trx?
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bm );

    Assert( trxMax != trxSession || PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() );

    const UINT              uiHash      = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bm );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    BOOL                    fVERActive  = fFalse;

    //  get RCE
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


//  ================================================================
INLINE BOOL FVERIAddUndoInfo( const RCE * const prce )
//  ================================================================
//
//  Do we need to create a deferred before image for this RCE?
//
//  UNDONE: do not create dependency for
//              replace at same level as before
{
    BOOL    fAddUndoInfo = fFalse;

    Assert( prce->TrxCommitted() == trxMax );

    Assert( !g_rgfmp[prce->Pfucb()->ifmp].FLogOn() || !PinstFromIfmp( prce->Pfucb()->ifmp )->m_plog->FLogDisabled() );
    if ( g_rgfmp[prce->Pfucb()->ifmp].FLogOn() )
    {
        if ( prce->FOperReplace() )
        {
//          ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ), fTrue );
//          const RCE               *prcePrevReplace = PrceVERIGetPrevReplace( prce );

            //  if previous replace is at the same level
            //  before image for rollback is in the other RCE
//          Assert( prce->Level() > 0 );
//          if ( prceNil == prcePrevReplace
//              || prcePrevReplace->Level() != prce->Level() )
//              {
                fAddUndoInfo = !prce->FEmptyDiff();
//              }
        }
        else if ( operFlagDelete == prce->Oper() )
        {
            fAddUndoInfo = fTrue;
        }
        else
        {
            //  these operations log the logical bookmark or do not log
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

//  ================================================================
ERR VER::ErrVERIAllocateRCE( INT cbRCE, RCE ** pprce, const UINT uiHash )
//  ================================================================
//
//  Allocates enough space for a new RCE or return out of memory
//  error. New buckets may be allocated
//
//-
{
    Assert( m_critBucketGlobal.FOwner() );

    ERR err = JET_errSuccess;

    //  Verify for prefast.
    if ( cbRCE < (INT)0 )
    {
        return(JET_errOutOfMemory);
    }

    //  if insufficient bucket space, then allocate new bucket.

    if ( m_pbucketGlobalHead == pbucketNil || (size_t)cbRCE > CbBUFree( m_pbucketGlobalHead ) )
    {
        Call( ErrVERIBUAllocBucket( cbRCE, uiHash ) );
    }
    Assert( (size_t)cbRCE <= CbBUFree( m_pbucketGlobalHead ) );

    //  pbucket always on double-word boundary
    Assert( FAlignedForThisPlatform( m_pbucketGlobalHead ) );

    //  set prce to next avail RCE location, and assert aligned

    *pprce = m_pbucketGlobalHead->hdr.prceNextNew;
    m_pbucketGlobalHead->hdr.prceNextNew =
        reinterpret_cast<RCE *>( PvAlignForThisPlatform( reinterpret_cast<BYTE *>( *pprce ) + cbRCE ) );

    Assert( FAlignedForThisPlatform( *pprce ) );
    Assert( FAlignedForThisPlatform( m_pbucketGlobalHead->hdr.prceNextNew ) );

HandleError:
    return err;
}


//  ================================================================
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
//  ================================================================
//
//  Allocate a new RCE in a bucket. m_critBucketGlobal is used to
//  protect the RCE until its oper and trxCommitted are set
//
//-
{
    ERR         err                 = JET_errSuccess;
    RCE *       prce                = prceNil;
    UINT        uiHashConcurrentOp;

    // For concurrent operations, we must be in critHash
    // from the moment the rceid is allocated until the RCE
    // is placed in the hash table, otherwise we may get
    // rceid's out of order.
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

    //  UNDONE: break this new function into two parts. One that only need to
    //  UNDONE: be recognizable by Clean up (trxTime?) and release
    //  UNDONE: the m_critBucketGlobal as soon as possible.

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
            );  //lint !e522

HandleError:
    m_critBucketGlobal.Leave();

    if ( err >= 0 )
    {
        Assert( prce != prceNil );

        //  check RCE
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


//  ================================================================
LOCAL ERR ErrVERIInsertRCEIntoHash( RCE * const prce )
//  ================================================================
//
//  Inserts an RCE to hash table. We must already be in the critical
//  section of the hash chain.
//
//-
{
    BOOKMARK    bookmark;
    prce->GetBookmark( &bookmark );
#ifdef DEBUG_VER
    Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif  //  DEBUG_VER

    RCE ** const    pprceChain  = PprceRCEChainGet( prce->UiHash(),
                                                    prce->Ifmp(),
                                                    prce->PgnoFDP(),
                                                    bookmark );

    if ( pprceChain )
    {
        //  hash chain for node already exists
        Assert( *pprceChain != prceNil );

        //  insert in order of rceid
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
                    //  we are recovering and have already created this RCE (redo)
                    //
                    //  that means we've encountered a deferred undo-info
                    //  log record for this operation
                    //
                    //  this RCE may have a pgno undo info. In which case the
                    //  page cannot be flushed (we have no facilities to log
                    //  deferred undo-info log records during recovery)
                    //
                    //  *but*
                    //
                    //  we've now encountered a deferred-undo log record which
                    //  means it is in fact safe to flush this page
                    //
                    //  remove the undo info

                    if( pgnoNull != prcePrev->PgnoUndoInfo() )
                    {
                        LGPOS lgposTip;
                        PinstFromIfmp( prce->Ifmp() )->m_plog->LGLogTipWithLock( &lgposTip );
                        BFRemoveUndoInfo( prcePrev, lgposTip );
                    }

                    //  release last version created
                    //  remove RCE from bucket
                    //
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

            //  if we are not recovering don't insert in RCEID order -- if we have waited
            //  for a wait-latch we actually want to insert after it

        }

        //  adjust head links
        if ( prceNil == prceNext )
        {
            //  insert before first rce in chain
            //
            Assert( prcePrev == *pprceChain );
            prce->SetPrceHashOverflow( (*pprceChain)->PrceHashOverflow() );
            *pprceChain = prce;

            #ifdef DEBUG
            prcePrev->SetPrceHashOverflow( prceInvalid );
            #endif  //  DEBUG
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

        //  adjust RCE links
        prce->SetPrcePrevOfNode( prcePrev );
    }
    else
    {
        Assert( prceNil == prce->PrceNextOfNode() );
        Assert( prceNil == prce->PrcePrevOfNode() );

        //  create new rce chain
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
#endif  //  DEBUG_VER

    Assert( prceNil == prce->PrceNextOfNode() ||
            PinstFromIfmp( prce->Ifmp() )->m_plog->FRecovering()
                && RceidCmp( prce->PrceNextOfNode()->Rceid(), prce->Rceid() ) > 0 );

    Assert( prce->Pfcb() != pfcbNil );

    //  monitor statistics
{
    //  Do not need critical section here. It is OK to miss a little.

    PERFOptDeclare( const INT cbBookmark = prce->CbBookmarkKey() + prce->CbBookmarkData() );
    PERFOptDeclare( VER *pver = PverFromIfmp( prce->Ifmp() ) );
    PERFOptDeclare( INST *pinst = pver->m_pinst );
    PERFOpt( cVERcrceHashEntries.Inc( pinst ) );
    PERFOpt( cVERcbBookmarkTotal.Add( pinst, cbBookmark ) );
}

    return JET_errSuccess;
}


//  ================================================================
LOCAL VOID VERIDeleteRCEFromHash( RCE * const prce )
//  ================================================================
//
//  Deletes RCE from hashtable/RCE chain, and may set hash table entry to
//  prceNil. Must already be in the critical section for the hash chain.
//
//-
{
    BOOKMARK    bookmark;
    prce->GetBookmark( &bookmark );
#ifdef DEBUG_VER
    Assert( UiRCHashFunc( prce->Ifmp(), prce->PgnoFDP(), bookmark ) == prce->UiHash() );
#endif  //  DEBUG_VER

    RCE ** const pprce = PprceRCEChainGet( prce->UiHash(), prce->Ifmp(), prce->PgnoFDP(), bookmark );
    // We won't call the function to delete an RCE from a hash chain, unless there actually is an RCE
    // to delete, but prefix can't comprehend this, so we have to convince it.
    AssertPREFIX( pprce != NULL );

    if ( prce == *pprce )
    {
        Assert( prceNil == prce->PrceNextOfNode() );

        //  the RCE is at the head of the chain
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

    //  monitor statistics
    //  Do not need critical section here. It is OK to miss a little.

    PERFOptDeclare( VER *pver = PverFromIfmp( prce->Ifmp() ) );
    PERFOptDeclare( INST *pinst = pver->m_pinst );
    PERFOpt( cVERcrceHashEntries.Dec( pinst ) );
    PERFOpt( cVERcbBookmarkTotal.Add( pinst, -(prce->CbBookmarkKey() + prce->CbBookmarkData()) ) );
}


//  ================================================================
INLINE VOID VERIInsertRCEIntoSessionList( PIB * const ppib, RCE * const prce )
//  ================================================================
//
//  Inserts the RCE into the session list of the pib provided. During ordinary
//  operation this means putting the RCE at the head of the list. During recovery
//  the RCE is inserted in its RCEID order.
//  
//  No critical section needed to do this, because the only session that
//  inserts at the head of the list is the pib itself.
//
//-
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

        // move previous until we are pointing at the RCE that comes before this insertion
        while( prcePrev && (level > prcePrev->Level() || RceidCmp( prcePrev->Rceid(), rceid ) < 0 ) )
        {
            prcePrev = prcePrev->PrceNextOfSession();
        }

        if (NULL == prcePrev)
        {
            prcePrev = ppib->prceNewest;
        }

        //  insert RCE in rceid order at the same level
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
        // track this RCE
        (void)ppib->ErrRegisterRceid(prce->Rceid(), prce);
    }
    
    Assert( prceNil == ppib->prceNewest->PrceNextOfSession() );
}


//  ================================================================
INLINE VOID VERIInsertRCEIntoSessionList( PIB * const ppib, RCE * const prce, RCE * const prceParent )
//  ================================================================
//
//  Inserts the RCE after the given parent RCE in the session list.
//  NEVER inserts at the head (since the insert occurs after an existing
//  RCE), so no possibility of conflict with regular session inserts, and
//  therefore no critical section needed.  However, may conflict with
//  other concurrent create indexers, which is why we must obtain critTrx
//  (to ensure only one concurrent create indexer is processing the parent
//  at a time.
//
//-
{
    Assert( ppib->CritTrx().FOwner() );

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


//  ================================================================
LOCAL VOID VERIDeleteRCEFromSessionList( PIB * const ppib, RCE * const prce )
//  ================================================================
//
//  Deletes the RCE from the session list of the given PIB. Must be
//  in the critical section of the PIB
//
//-
{
    Assert( !prce->FFullyCommitted() ); //  only uncommitted RCEs in the session list
    Assert( ppib == prce->Pfucb()->ppib );

    if ( prce == ppib->prceNewest )
    {
        //  we are at the head of the list
        PIBSetPrceNewest( ppib, prce->PrcePrevOfSession() );
        if ( prceNil != ppib->prceNewest )
        {
            Assert( prce == ppib->prceNewest->PrceNextOfSession() );
            ppib->prceNewest->SetPrceNextOfSession( prceNil );
        }
    }
    else
    {
        // we are in the middle/end
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


//  ================================================================
LOCAL VOID VERIInsertRCEIntoFCBList( FCB * const pfcb, RCE * const prce )
//  ================================================================
//
//  Must be in critFCB
//
//-
{
    Assert( pfcb->CritRCEList().FOwner() );
    pfcb->AttachRCE( prce );
}


//  ================================================================
LOCAL VOID VERIDeleteRCEFromFCBList( FCB * const pfcb, RCE * const prce )
//  ================================================================
//
//  Must be in critFCB
//
//-
{
    Assert( pfcb->CritRCEList().FOwner() );
    pfcb->DetachRCE( prce );
}


//  ================================================================
VOID VERInsertRCEIntoLists(
    FUCB        *pfucbNode,     // cursor of session performing node operation
    CSR         *pcsr,
    RCE         *prce,
    const VERPROXY  *pverproxy )
//  ================================================================
{
    Assert( prceNil != prce );

    FCB * const pfcb = prce->Pfcb();
    Assert( pfcbNil != pfcb );

    LOG *plog = PinstFromIfmp( pfcb->Ifmp() )->m_plog;

    if ( prce->TrxCommitted() == trxMax )
    {
        FUCB    *pfucbVer = prce->Pfucb();  // cursor of session for whom the
                                            // RCE was created,
        Assert( pfucbNil != pfucbVer );
        Assert( pfcb == pfucbVer->u.pfcb );

        // cursor performing the node operation may be different than cursor
        // for which the version was created if versioning by proxy
        Assert( pfucbNode == pfucbVer || pverproxy != NULL );

        if ( FVERIAddUndoInfo( prce ) )
        {
            Assert( pcsrNil != pcsr || plog->FRecovering() );   // Allow Redo to override UndoInfo.
            if ( pcsrNil != pcsr )
            {
                Assert( !g_rgfmp[ pfcb->Ifmp() ].FLogOn() || !plog->FLogDisabled() );
                if ( g_rgfmp[ pfcb->Ifmp() ].FLogOn() )
                {
                    if( pcsr->Cpage().FLoadedPage() )
                    {
                        // during recovery we may use the before-image of a page to redo a split
                        // or merge. the before-image isn't a real page and we shouldn't set
                        // the undo info on it
                        Assert( plog->FRecovering() );
                    }
                    else
                    {
                        //  set up UndoInfo dependency
                        //  to make sure UndoInfo will be logged if the buffer
                        //  is flushed before commit/rollback.
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
                || prce->Oper() == operReplace      // via FlagInsertAndReplaceData
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
        // Don't put committed RCE's into session list.
        // Only way to get a committed RCE is via concurrent create index.
        Assert( !plog->FRecovering() );
        Assert( NULL != pverproxy );
        Assert( proxyCreateIndex == pverproxy->proxy );
        Assert( prceNil != pverproxy->prcePrimary );
        Assert( pverproxy->prcePrimary->TrxCommitted() == prce->TrxCommitted() );
        Assert( pfcb->FTypeSecondaryIndex() );
        Assert( pfcb->PfcbTable() == pfcbNil );
        Assert( prce->Oper() == operInsert
                || prce->Oper() == operReplace      // via FlagInsertAndReplaceData
                || prce->Oper() == operFlagDelete );
    }


    ENTERCRITICALSECTION enterCritFCBRCEList( &(pfcb->CritRCEList()) );
    VERIInsertRCEIntoFCBList( pfcb, prce );
}


//  ================================================================
LOCAL VOID VERINullifyUncommittedRCE( RCE * const prce )
//  ================================================================
//
//  Remove undo info from a RCE, remove it from the session list
//  FCB list and the hash table. The oper is then nullified
//
//-
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
        //  VERIDeleteRCEFromHash may call ASSERT_VALID so we do this deletion first,
        //  before we mess up the other linked lists
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


    Assert( ppib->CritTrx().FOwner() );

    if ( fOperInHashTable )
    {
        prwlHash->EnterAsWriter();
    }
    critRCEList.Enter();

    prceToNullify->FlagRolledBack();

    // Take snapshot of count of people concurrently creating a secondary
    // index entry.  Since this count is only ever incremented within this
    // table's critRCEList (which we currently have) it doesn't matter
    // if value changes after the snapshot is taken, because it will
    // have been incremented for some other table.
    // Also, if this FCB is not a table or if it is but has fixed DDL, it
    // doesn't matter if others are doing concurrent create index -- we know
    // they're not doing it on this FCB.
    // Finally, the only RCE types we have to lock are Insert, FlagDelete, and Replace,
    // because they are the only RCE's that concurrent CreateIndex acts upon.
    const LONG  crefCreateIndexLock     = ( fPossibleSecondaryIndex ? g_crefVERCreateIndexLock : 0 );
    Assert( g_crefVERCreateIndexLock >= 0 );
    if ( 0 == crefCreateIndexLock )
    {
        //  set return value before the RCE is nullified
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
            ppib->CritTrx().Leave();
            UtilSleep( cmsecWaitGeneric );
            ppib->CritTrx().Enter();

            //  restart RCE scan
            *pprceNextToNullify = ppib->prceNewest;
        }
    }
}


//  ================================================================
LOCAL VOID VERINullifyCommittedRCE( RCE * const prce )
//  ================================================================
//
//  Remove a RCE from the Hash table (if necessary) and the FCB list.
//  The oper is then nullified
//
//-
{
    Assert( prce->PgnoUndoInfo() == pgnoNull );
    Assert( prce->TrxCommitted() != trxMax );
    Assert( prce->FFullyCommitted() );
    VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );

    const BOOL fInHash = prce->FOperInHashTable();
    if ( fInHash )
    {
        //  VERIDeleteRCEFromHash may call ASSERT_VALID so we do this deletion first,
        //  before we mess up the other linked lists
        VERIDeleteRCEFromHash( prce );
    }

    prce->NullifyOper();
}


//  ================================================================
INLINE VOID VERINullifyRCE( RCE *prce )
//  ================================================================
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


//  ================================================================
VOID VERNullifyFailedDMLRCE( RCE *prce )
//  ================================================================
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


//  ================================================================
VOID VERNullifyAllVersionsOnFCB( FCB * const pfcb )
//  ================================================================
//
//  This is used to nullify all RCEs on an FCB
//
//-
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

        //  during recovery, should not see any uncommitted RCE's
        Assert( prce->TrxCommitted() != trxMax || !plog->FRecovering() );

        if ( prce->FOperInHashTable() )
        {
            const UINT  uiHash          = prce->UiHash();
            const BOOL  fNeedCritTrx    = ( prce->TrxCommitted() == trxMax
                                            && !FFMPIsTempDB( prce->Ifmp() ) );
            PIB         *ppib;

            Assert( !pfcb->FTypeTable()
                || plog->FRecovering()
                || ( prce->FMoved() && operFlagDelete == prce->Oper() ) );

            if ( fNeedCritTrx )
            {
                // If uncommitted, must grab critTrx to ensure that
                // RCE does not commit or rollback on us while
                // nullifying.  Note that we don't need critTrx
                // if this is a temp table because we're the
                // only ones who have access to it.
                Assert( prce->Pfucb() != pfucbNil );
                Assert( prce->Pfucb()->ppib != ppibNil );
                ppib = prce->Pfucb()->ppib;
            }
            else
            {
                //  initialise to silence compiler warning
                //
                ppib = ppibNil;
            }

            pfcb->CritRCEList().Leave();

            if ( plog->FRecovering() )
            {
                pver->m_critRCEClean.Enter();
            }
            ENTERCRITICALSECTION    enterCritPIBTrx(
                                        fNeedCritTrx ? &ppib->CritTrx() : NULL,
                                        fNeedCritTrx );
            ENTERREADERWRITERLOCK   enterRwlHashAsWriter( &( pver->RwlRCEChain( uiHash ) ), fFalse );

            pfcb->CritRCEList().Enter();

            // Verify no one nullified the RCE while we were
            // switching critical sections.
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
            // If not hashable, nullification will be expensive,
            // because we have to grab m_critRCEClean.  Fortunately
            // this should be rare:
            //  - the only non-hashable versioned operation
            //    possible for a temporary table is CreateTable, unless
            //    the table is close while still in the transaction that created
            //    it.
            //  - no non-hashable versioned operations possible
            //    on secondary index.
            if ( !plog->FRecovering() )
            {
                Assert( FFMPIsTempDB( prce->Ifmp() ) );
                Assert( prce->Pfcb()->FTypeTemporaryTable() );
            }

            pfcb->CritRCEList().Leave();
            pver->m_critRCEClean.Enter();
            pfcb->CritRCEList().Enter();

            // critTrx not needed, since we're the only ones
            // who should have access to this FCB.

            // Verify no one nullified the RCE while we were
            // switching critical sections.
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


//  ================================================================
VOID VERNullifyInactiveVersionsOnBM( const FUCB * pfucb, const BOOKMARK& bm )
//  ================================================================
//
//  Nullifies all RCE's for given bm. Used by online cleanup when
//  a page is being cleaned.
//
//  All the RCE's should be inactive and thus don't need to be
//  removed from a session list
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bm );

    // calculating the oldest TRX in the system takes a lot of locks
    // to avoid concurrency problems we will use a cached version of
    // the oldest TRX and only update the trx if we find a version
    // that can't be cleaned.
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
            // update the oldest TRX and retry
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



//  ****************************************************************
//  VERSION LAYER
//  ****************************************************************


VOID VER::VERSignalCleanup()
{
    //  check whether we already requested clean up of that version store
    if ( NULL != m_pbucketGlobalTail
        && 0 == AtomicCompareExchange( (LONG *)&m_fVERCleanUpWait, 0, 1 ) )
    {
        //  reset signal in case we need to ensure RCE clean gets
        //  performed soon
        //
        m_msigRCECleanPerformedRecently.Reset();

        //  be aware if we are in syncronous mode already, do not try to start a new task
        if ( m_fSyncronousTasks
            || m_pinst->Taskmgr().ErrTMPost( VERIRCECleanProc, this ) < JET_errSuccess )
        {
            //  couldn't post task, so set signal to ensure no one waits on it
            //
            m_msigRCECleanPerformedRecently.Set();

            LONG fStatus = AtomicCompareExchange( (LONG *)&m_fVERCleanUpWait, 1, 0 );
            if ( 2 == fStatus )
            {
                m_asigRCECleanDone.Set();
            }
        }
    }
}


//  ================================================================
DWORD VER::VERIRCECleanProc( VOID *pvThis )
//  ================================================================
//
//  Go through all sessions, cleaning buckets as versions are
//  no longer needed.  Only those versions older than oldest
//  transaction are cleaned up.
//
//  Side Effects:
//      frees buckets.
//
//-
{
    VER *pver = (VER *)pvThis;

    pver->m_critRCEClean.Enter();
    pver->ErrVERIRCEClean();
    LONG fStatus = AtomicCompareExchange( (LONG *)&pver->m_fVERCleanUpWait, 1, 0 );
    pver->m_critRCEClean.Leave();

    //  indicate that RCE clean was performed recently
    //
    pver->m_msigRCECleanPerformedRecently.Set();

    if ( 2 == fStatus )
    {
        pver->m_asigRCECleanDone.Set();
    }

    return 0;
}


//  ================================================================
DWORD_PTR CbVERISize( BOOL fLowMemory, BOOL fMediumMemory, DWORD_PTR cbVerStoreMax )
//  ================================================================
//
//  Creates version store hash table
//
//-
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
        cbVER = cbVER * VER::cbrcehead / VER::cbrceheadLegacy;  //  compensate for hash chain size increase
        cbVER = max( cbVER, OSMemoryPageReserveGranularity() );
    }

#ifdef DEBUG
    // reduce store size on debug to increase chance of hash collision
    cbVER = cbVER / 30;
#endif
    cbVER = roundup( cbVER, OSMemoryPageCommitGranularity() );

    return cbVER;
}


//  ================================================================
JETUNITTEST( VER, CheckVerSizing )
//  ================================================================
{
#ifdef DEBUG
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



//  ================================================================
VER * VER::VERAlloc( INST* pinst )
//  ================================================================
//
//  Creates version store hash table
//
//-
{
    C_ASSERT( OffsetOf( VER, m_rgrceheadHashTable ) == sizeof( VER ) );

    const BOOL fLowMemory = pinst->m_config & JET_configLowMemory;
    const BOOL fMediumMemory = ( pinst->m_config & JET_configDynamicMediumMemory ) || FDefaultParam( pinst, JET_paramMaxVerPages );
    // Get version store page size from resource manager, JET_paramVerPageSize is not the correct value.
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
        // would like to assert that this is in fact 4001, but this is only
        // non-debug
    }
#endif

    AssertRTL( sizeof(VER::RCEHEAD) *  pVer->m_crceheadHashTable < cbVER );

    return pVer;
}

//  ================================================================
VOID VER::VERFree( VER * pVer )
//  ================================================================
//
//  Deletes version store hash table
//
//-
{
    if ( pVer == NULL )
    {
        return;
    }

    pVer->~VER();
    OSMemoryPageFree( pVer );
}

//  ================================================================
ERR VER::ErrVERInit( INST* pinst )
//  ================================================================
//
//  Creates background version bucket clean up thread.
//
//-
{
    ERR     err = JET_errSuccess;
    OPERATION_CONTEXT opContext = { OCUSER_VERSTORE, 0, 0, 0, 0 };

    PERFOpt( cVERcbucketAllocated.Clear( m_pinst ) );
    PERFOpt( cVERcbucketDeleteAllocated.Clear( m_pinst ) );
    PERFOpt( cVERBucketAllocWaitForRCEClean.Clear( m_pinst ) );
    PERFOpt( cVERcrceHashEntries.Clear( m_pinst ) );            //  number of RCEs that currently exist in hash table
    PERFOpt( cVERcbBookmarkTotal.Clear( m_pinst ) );            //  amount of space used by bookmarks of existing RCEs
    PERFOpt( cVERUnnecessaryCalls.Clear( m_pinst ) );           //  calls for a bookmark that doesn't exist
    PERFOpt( cVERSyncCleanupDispatched.Clear( m_pinst ) );      //  cleanup operations dispatched synchronously
    PERFOpt( cVERAsyncCleanupDispatched.Clear( m_pinst ) );     //  cleanup operations dispatched asynchronously
    PERFOpt( cVERCleanupDiscarded.Clear( m_pinst ) );           //  cleanup operations dispatched but failed
    PERFOpt( cVERCleanupFailed.Clear( m_pinst ) );              //  cleanup operations discarded

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

    //  init the Bucket resource manager
    //
    //  NOTE:  if we are recovering, then disable quotas
    //
    if ( m_pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperEnableMaxUse, fFalse ) );
    }
    CallR( m_cresBucket.ErrInit( JET_residVERBUCKET ) );
    CallS( ErrRESGetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperSize, &m_cbBucket ) );

    // pbucketGlobal{Head,Tail} should be NULL. If they aren't we probably
    // didn't terminate properly at some point
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

    // sync tasks only during termination
    m_fSyncronousTasks = fFalse;

    m_fVERCleanUpWait = 0;

    //  init the hash table
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

    //  term the Bucket resource manager
    //
    m_cresBucket.Term();
    if ( m_pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperEnableMaxUse, fTrue ) );
    }
    return err;
}


//  ================================================================
VOID VER::VERTerm( BOOL fNormal )
//  ================================================================
//
//  Terminates background thread and releases version store
//  resources.
//
//-
{
    //  The TASKMGR has gone away by the time we are shutting down
    Assert ( m_fSyncronousTasks );

    //  be sure that the state will not change
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
        //  We don't end RCE session here, because PurgeAllDatabases/PIBTerm is
        //  doing it in the correct order (closing FUCBs before ending sessions),
        //  and ending session here doesn't work with failure case such as 
        //  when rollback failed, in which case there could still be FUCBs not closed.
        //
        //  PIBEndSession( m_ppibRCEClean );
#ifdef DEBUG
        //  There should not be any fucbOfSession attached, unless there's rollback failure and instance unavailable
        Assert( pfucbNil == m_ppibRCEClean->pfucbOfSession || m_pinst->FInstanceUnavailable() );

        //  Ensure the RCEClean PIB is in the global PIB list,
        //  so that we know PurgeAllDatabases/PIBTerm will take care of closing it
        const PIB* ppibT = m_pinst->m_ppibGlobal;
        for ( ; ppibT != m_ppibRCEClean && ppibT != ppibNil; ppibT = ppibT->ppibNext );
        Assert( ppibT == m_ppibRCEClean );
#endif  //  DEBUG

        m_ppibRCEClean = ppibNil;
    }

    if ( ppibNil != m_ppibRCECleanCallback )
    {
#ifdef DEBUG
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            Assert( !FPIBUserOpenedDatabase( m_ppibRCECleanCallback, dbid ) );
        }

        //  There should not be any fucbOfSession attached, unless there's rollback failure and instance unavailable
        Assert( pfucbNil == m_ppibRCECleanCallback->pfucbOfSession || m_pinst->FInstanceUnavailable() );
        
        //  Ensure the RCECleanCallback PIB is in the global PIB list,
        //  so that we know PurgeAllDatabases/PIBTerm will take care of closing it
        const PIB* ppibT = m_pinst->m_ppibGlobal;
        for ( ; ppibT != m_ppibRCECleanCallback && ppibT != ppibNil; ppibT = ppibT->ppibNext );
        Assert( ppibT == m_ppibRCECleanCallback );
#endif  //  DEBUG

        //  We don't end RCECleanCallback session here, because PurgeAllDatabases/PIBTerm is
        //  doing it in the correct order (closing FUCBs before ending sessions),
        //  and ending session here doesn't work with failure case such as 
        //  when rollback failed, in which case there could still be FUCBs not closed.
        //
        //  PIBEndSession( m_ppibRCECleanCallback );

        m_ppibRCECleanCallback = ppibNil;
    }

    //  free all buckets
    //  Note that linked list goes from head -> prev -> prev -> prev -> nil
    //
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

    //  term the Bucket resource manager
    //
    m_cresBucket.Term();
    if ( m_pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( m_pinst, JET_residVERBUCKET, JET_resoperEnableMaxUse, fTrue ) );
    }

    //  term the hash table
    //
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
    PERFOpt( cVERcrceHashEntries.Clear( m_pinst ) );            //  number of RCEs that currently exist in hash table
    PERFOpt( cVERcbBookmarkTotal.Clear( m_pinst ) );            //  amount of space used by bookmarks of existing RCEs
    PERFOpt( cVERUnnecessaryCalls.Clear( m_pinst ) );           //  calls for a bookmark that doesn't exist
    PERFOpt( cVERSyncCleanupDispatched.Clear( m_pinst ) );      //  cleanup operations dispatched synchronously
    PERFOpt( cVERAsyncCleanupDispatched.Clear( m_pinst ) );     //  cleanup operations dispatched asynchronously
    PERFOpt( cVERCleanupDiscarded.Clear( m_pinst ) );           //  cleanup operations dispatched but failed
    PERFOpt( cVERCleanupFailed.Clear( m_pinst ) );              //  cleanup operations discarded
}


//  ================================================================
ERR VER::ErrVERStatus( )
//  ================================================================
//
//  Returns JET_wrnIdleFull if version store more than half full.  Half is defined as 60% of the
//  optimal max version store (before we stop cleaning up deletes).
//
//-
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


//  ================================================================
VS VsVERCheck( const FUCB * pfucb, CSR * pcsr, const BOOKMARK& bookmark )
//  ================================================================
//
//  Given a BOOKMARK, returns the version status / visibility calculation.
//
//  RETURN VALUE
//      vsCommitted
//      vsUncommittedByCaller
//      vsUncommittedByOther
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    const UINT          uiHash  = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    ENTERREADERWRITERLOCK enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );
    const RCE * const   prce    = PrceRCEGet( uiHash, pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

    VS vs = vsNone;

    //  if no RCE for node then version bit in node header must
    //  have been orphaned due to crash.  Remove node bit.
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
        //  committed
        vs = vsCommitted;
    }
    else if ( prce->Pfucb()->ppib != pfucb->ppib )
    {
        //  not modified (uncommitted)
        vs = vsUncommittedByOther;
    }
    else
    {
        //  modifier (uncommitted)
        vs = vsUncommittedByCaller;
    }

    Assert( vsNone != vs );
    return vs;
}


//  ================================================================
ERR ErrVERAccessNode( FUCB * pfucb, const BOOKMARK& bookmark, NS * pns )
//  ================================================================
//
//  Finds the correct version of a node.
//
//  PARAMETERS
//      pfucb           various fields used/returned.
//      pfucb->kdfCurr  the returned prce or NULL to tell caller to
//                      use the node in the DB page.
//
//  RETURN VALUE
//      nsDatabase
//      nsVersionedUpdate
//      nsVersionedInsert
//      nsUncommittedVerInDB
//      nsCommittedVerInDB
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    //  session with dirty cursor isolation model should never
    //  call NsVERAccessNode.
    Assert( !FPIBDirty( pfucb->ppib ) );
    Assert( FPIBVersion( pfucb->ppib ) );
    Assert( !FFUCBUnique( pfucb ) || 0 == bookmark.data.Cb() );
    Assert( Pcsr( pfucb )->FLatched() );


    //  FAST PATH:  if there are no RCEs in this bucket, immediately bail with
    //  nsDatabase.  the assumption is that the version store is almost always
    //  nearly empty

    const UINT uiHash = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );

    if ( prceNil == PverFromIfmp( pfucb->ifmp )->GetChain( uiHash ) )
    {
        PERFOpt( cVERUnnecessaryCalls.Inc( PinstFromPfucb( pfucb ) ) );

        if ( FFUCBUpdatable( pfucb ) )
        {
            //  the version bit is set but there is no version. reset the bit
            //  we cast away const because we are secretly modifying the page
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
            //  the version bit is set but there is no version. reset the bit
            //  we cast away const because we are secretly modifying the page
            NDDeferResetNodeVersion( Pcsr( const_cast<FUCB *>( pfucb ) ) );
        }
        nsStatus = nsDatabase;
    }
    else if ( prce->TrxCommitted() == trxMax &&
              prce->Pfucb()->ppib == pfucb->ppib )
    {
        //  cannot be trying to access an RCE that we ourselves rolled back
        Assert( !prce->FRolledBack() );

        //  if caller is modifier of uncommitted version then database
        nsStatus = nsUncommittedVerInDB;
    }
    else if ( TrxCmp( prce->TrxCommitted(), trxSession ) < 0 )
    {
        //  if committed version younger than our transaction then database
        nsStatus = nsCommittedVerInDB;
    }
    else
    {
        //  active version created by another session. look for before image
        Assert( prceNil != prce );

        //  level 0 sessions may play havoc with visibility here, as
        //  RCE's in the middle of the node chain may suddenly become
        //  visible while we're traversing the chain
        //
        Assert( TrxCmp( prce->TrxCommitted(), trxSession ) >= 0
            || trxMax == trxSession );

        //  loop will set prce to the non-delta RCE whose before image was committed
        //  before this transaction began. if all active RCE's are delta RCE's we set prce to
        //  the oldest uncommitted delta RCE
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
                        //  FALLTHRU to case below
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
                //  if the RCE was rolled back, then it's just as if 
                //  the node was unversioned, so set to nsDatabase;
                //  otherwise, the operation is not visible to us,
                //  so set to nsVersionedUpdate (note that the node
                //  is visible, though the operation on the node
                //  is not)
                //
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
                //  all the active versions are delta or lock,
                //  so no before images, so treat this as unversioned for visibility
                //  purposes (ie. classify it as nsDatabase and not ns*VerInDB)
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
                //  Because we are at level 0 this RCE may disappear at any time after
                //  we leave the version store. We copy the before image into the FUCB
                //  to make sure we can always access it

                //  we should not be modifying the page at level 0
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


//  ================================================================
LOCAL BOOL FUpdateIsActive( const PIB * const ppib, const UPDATEID& updateid )
//  ================================================================
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


//  ================================================================
template< typename TDelta >
TDelta DeltaVERGetDelta( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset )
//  ================================================================
//
//  Returns the correct compensating delta for this transaction on the given offset
//  of the bookmark. Collect the negation of all active delta versions not created
//  by our session.
//
//-
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
                    //  there is an outstanding replace (not by us) on this
                    //  node -- must reset compensating delta count to begin
                    //  from this point
                    tDelta = 0;

                    //  UNDONE: return a flag indicating potential write
                    //  conflict so ErrRECAOSeparateLV() will not bother even
                    //  even trying to do a replace on the LVROOT and instead
                    //  burst immediately
                    //  *pfPotentialWriteConflict = fTrue;
                }
            }
            else if ( _VERDELTA< TDelta >::TRAITS::oper == prce->Oper() )
            {
                Assert( !prce->FRolledBack() );     // Delta RCE's can never be flag-RolledBack

                const _VERDELTA< TDelta >* const pverdelta = ( _VERDELTA< TDelta >* )prce->PbData();
                if ( pverdelta->cbOffset == cbOffset
                    &&  ( prce->FActiveNotByMe( pfucb->ppib, trxSession )
                        || (    trxMax == prce->TrxCommitted()
                                && prce->Pfucb()->ppib == pfucb->ppib
                                && FUpdateIsActive( prce->Pfucb()->ppib, prce->Updateid() ) ) ) )
                {
                    //  delta version created by a different session.
                    //  the version is either uncommitted or created by
                    //  a session that started after us
                    //  or a delta done by a currently uncommitted replace
                    tDelta -= pverdelta->tDelta;
                }
            }
        }
    }

    return tDelta;
}


//  ================================================================
BOOL FVERDeltaActiveNotByMe( const FUCB * pfucb, const BOOKMARK& bookmark, INT cbOffset )
//  ================================================================
//
//  get prce for node and look for uncommitted increment/decrement
//  versions created by another session.Used to determine if the delta
//  value we see may change because of a rollback.
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );
    Assert( 0 == cbOffset );    //  should only be used from LV

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
                //  uncommitted delta version created by another session
                fVersionExists = fTrue;
                break;
            }
        }
    }

    return fVersionExists;
}


//  ================================================================
INLINE BOOL FVERIGetReplaceInRangeByUs(
    const PIB       *ppib,
    const RCE       *prceLastBeforeEndOfRange,
    const RCEID     rceidFirst,
    const RCEID     rceidLast,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    const BOOL      fFindLastReplace,
    const RCE       **pprceReplaceInRange )
//  ================================================================
{
    const RCE       *prce;
    BOOL            fSawCommittedByUs = fFalse;
    BOOL            fSawUncommittedByUs = fFalse;

    Assert( prceNil != prceLastBeforeEndOfRange );
    Assert( trxCommitted == trxMax ? ppibNil != ppib : ppibNil == ppib );

    Assert( PverFromIfmp( prceLastBeforeEndOfRange->Ifmp() )->RwlRCEChain( prceLastBeforeEndOfRange->UiHash() ).FReader() );

    // Initialize return value.
    *pprceReplaceInRange = prceNil;

    Assert( rceidNull != rceidLast );
    Assert( RceidCmp( rceidFirst, rceidLast ) < 0 );
    if ( rceidNull == rceidFirst )
    {
        // If rceidFirst == NULL, then no updates were done in the range (this
        // will force retrieval of the after-image (since no updates were done
        // the before-image will be equivalent to the after-image).
        return fFalse;
    }

    //  go backwards through all RCEs in the range
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
                // If only looking for the last RCE, we would have exited
                // before finding a second one.
                Assert( !fFindLastReplace );

                Assert( trxMax != trxCommitted );
                Assert( ppibNil == ppib );

                // If one node in the range belogs to us, they must all belong
                // to us (otherwise a WriteConflict would have been generated).
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
                // If only looking for the last RCE, we would have exited
                // before finding a second one.
                Assert( !fFindLastReplace );

                // If one node in the range belogs to us, they must all belong
                // to us (otherwise a WriteConflict would have been generated).
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
                        // The node is uncommitted, but it doesn't belong to us.
                        Assert( trxCommitted != trxMax
                            || prce->Pfucb()->ppib != ppib );
                        return fFalse;
                    }

                    //  if original RCE also uncommitted, assert same session.
                    //  if original RCE already committed, can't assert anything.
                    Assert( trxCommitted != trxMax
                        || prce->Pfucb()->ppib == ppib );

                    fSawUncommittedByUs = fTrue;
                }
                else if ( prce->TrxCommitted() == trxCommitted )
                {
                    // The node was committed by us.
                    Assert( trxMax != trxCommitted );
                    Assert( ppibNil == ppib );
                    fSawCommittedByUs = fTrue;
                }
                else
                {
                    // The node is committed, but not by us.
                    Assert( prce->TrxBegin0() != trxBegin0 );
                    return fFalse;
                }

                if ( fFindLastReplace )
                {
                    // Only interested in the existence of a replace in the range,
                    // don't really want the image.
                    return fTrue;
                }
            }

            prceFirstReplaceInRange = prce;
        }
    }   // for
    Assert( prceNil == prce || RceidCmp( rceidFirst, prce->Rceid() ) >= 0 );

    if ( prceNil != prceFirstReplaceInRange )
    {
        *pprceReplaceInRange = prceFirstReplaceInRange;
        return fTrue;
    }

    return fFalse;
}


//  ================================================================
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
            // All uncommitted RCE's should be owned by the same session.
            Assert( prceNil != prceFirstActiveReplaceByOther );
            Assert( prceFirstActiveReplaceByOther->TrxCommitted() == trxMax || prceFirstActiveReplaceByOther->TrxCommitted() == prce->TrxCommitted() );
            Assert( prceFirstActiveReplaceByOther->TrxBegin0() == prce->TrxBegin0() );
            Assert( prceFirstActiveReplaceByOther->Pfucb()->ppib == prce->Pfucb()->ppib );
        }
        else
        {
            if ( *pfFoundCommitted )
            {
                //  must belong to same session in the middle of committing to level 0
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
                // If there's also an uncommitted RCEs on this node,
                // it must have started its transaction after this one committed.
                Assert( prceNil != prceFirstActiveReplaceByOther );
                Assert( prceFirstActiveReplaceByOther->TrxCommitted() == trxMax || prceFirstActiveReplaceByOther->TrxCommitted() == prce->TrxCommitted() );
                Assert( RceidCmp( prce->Rceid(), prceFirstActiveReplaceByOther->Rceid() ) < 0 );
                Assert( prceFirstActiveReplaceByOther->TrxCommitted() != trxMax || TrxCmp( prce->TrxCommitted(), prceFirstActiveReplaceByOther->TrxBegin0() ) < 0 );
            }

            //  Cannot be any uncommitted RCE's of any type
            //  before a committed replace RCE, except if the
            //  same session is in the middle of committing to level 0.
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
//  ================================================================


//  ================================================================
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
//  ================================================================
//
//  Extract the before image from the oldest replace RCE for the bookmark
//  that falls exclusively within the range (rceidFirst,rceidLast)
//
//-
{
    const UINT  uiHash              = UiRCHashFunc( ifmp, pgnoLVFDP, bookmark );
    ENTERREADERWRITERLOCK enterRwlHashAsReader( &( PverFromIfmp( ifmp )->RwlRCEChain( uiHash ) ), fTrue );

    const RCE   *prce               = PrceRCEGet( uiHash, ifmp, pgnoLVFDP, bookmark );
    const RCE   *prceDesiredImage   = prceNil;
    const RCE   *prceFirstAfterRange= prceNil;

    Assert( trxMax != trxBegin0 );

    // find the last RCE before the end of the range
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
                // If looking for the after-image, it will be found in the
                // node's next replace RCE after the one found in the range.
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
            //  If last operation before the end of the range belongs
            //  to us, then there will be no other active images on the
            //  node by other sessions (they would have write-conflicted).
            //  We can just fall through below to grab the proper image.

            if ( prceLastBeforeEndOfRange->TrxCommitted() != trxMax )
            {
                //  If the last RCE in the range has already committed,
                //  then the RCE on which the search was based must
                //  also have been committed at the same time.
                Assert( prceLastBeforeEndOfRange->TrxCommitted() == trxCommitted );
                Assert( ppibNil == ppib );
            }
            else if ( trxCommitted == trxMax )
            {
                //  Verify last RCE in the range belongs to same
                //  transaction.
                Assert( ppibNil != ppib );
                Assert( ppib == prceLastBeforeEndOfRange->Pfucb()->ppib );
                Assert( trxBegin0 == ppib->trxBegin0 );
            }
            else
            {
                //  This is the case where the RCE in the range is uncommitted,
                //  but the RCE on which the search was based has already
                //  committed.  This is a valid case (we could be looking at
                //  the RCE while the transaction is in the middle of being
                //  committed).  Can't really do anything to assert this except
                //  to check that the trxBegin0 is the same, which we've already
                //  done above.
            }

            // Force to look after the specified range for our image.
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

            // No replace RCE's by us between the specified range, or
            // any RCE's by us of any type before the end of the range.
            // Check active RCE's by others.
            for ( prce = prceLastBeforeEndOfRange;
                prceNil != prce;
                prce = prce->PrcePrevOfNode() )
            {
                Assert( RceidCmp( prce->Rceid(), rceidLast ) < 0 );
                //  For retrieving a LVROOT while getting a before image we may see deltas
                Assert( prce->TrxBegin0() != trxBegin0 || operDelta == prce->Oper() || operDelta64 == prce->Oper() );

                if ( TrxCmp( prce->TrxCommitted(), trxBegin0 ) < 0 )
                    break;  // No more active RCE's.

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

                    // There may be multiple active RCE's on
                    // the same node. We want the very first one.
                    prceFirstActiveReplaceByOther = prce;
                }

            }   // for


            Assert( prceNil == prceDesiredImage );
            if ( prceNil != prceFirstActiveReplaceByOther )
                prceDesiredImage = prceFirstActiveReplaceByOther;
        }
    }

    // If no RCE's within range or before range, look after the range.
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


//  ================================================================
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
//  ================================================================
//
//  Creates a DML RCE in a bucket
//
//-
{
    Assert( pfucb->ppib->Level() > 0 );
    Assert( level > 0 );
    Assert( pfucb->u.pfcb != pfcbNil );
    Assert( FOperInHashTable( oper ) );

    ERR     err     = JET_errSuccess;
    RCE     *prce   = prceNil;

    //  calculate the length of the RCE in the bucket.
    //  if updating node, set cbData in RCE to length of data. (w/o the key).
    //  set cbNewRCE as well.
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

    //  Set up a skeleton RCE. This holds m_critBucketGlobal, so do it
    //  first before filling the rest.
    //
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

    //  copy the bookmark
    prce->CopyBookmark( bookmark );

    Assert( pgnoNull == prce->PgnoUndoInfo( ) );
    Assert( prce->Oper() != operAllocExt );
    Assert( !prce->FOperDDL() );

    //  flag FUCB version
    FUCBSetVersioned( pfucb );

    CallS( err );

HandleError:
    if ( pprce )
    {
        *pprce = prce;
    }

    return err;
}


//  ================================================================
LOCAL VOID VERISetupInsertedDMLRCE( const FUCB * pfucb, RCE * prce )
//  ================================================================
//
//  This copies the appropriate data from the FUCB into the RCE and
//  propagates the maximum node size. This must be called after
//  insertion so the maximum node size can be found (for operReplace)
//
//  This currently does not need to be called from VERModifyByProxy
//
//-
{
    Assert( prce->TrxCommitted() == trxMax );
    //  If replacing node, rather than inserting or deleting node,
    //  copy the data to RCE for before image for version readers.
    //  Data size may be 0.
    switch( prce->Oper() )
    {
        case operReplace:
        {
            Assert( prce->CbData() >= cbReplaceRCEOverhead );

            VERREPLACE* const pverreplace   = (VERREPLACE*)prce->PbData();

            if ( pfucb->fUpdateSeparateLV )
            {
                //  we updated a separateLV store the begin time of the PrepareUpdate
                pverreplace->rceidBeginReplace = pfucb->rceidBeginUpdate;
            }
            else
            {
                pverreplace->rceidBeginReplace = rceidNull;
            }

            const RCE * const prcePrevReplace = PrceVERIGetPrevReplace( prce );
            if ( prceNil != prcePrevReplace )
            {
                //  a previous version exists. its max size is the max of the before- and
                //  after-images (the after-image is our before-image)
                Assert( !prcePrevReplace->FRolledBack() );
                const VERREPLACE* const pverreplacePrev = (VERREPLACE*)prcePrevReplace->PbData();
                Assert( PinstFromIfmp( pfucb->ifmp )->m_plog->FRecovering() && fRecoveringUndo != PinstFromIfmp( pfucb->ifmp )->m_plog->FRecoveringMode() ||
                        pverreplacePrev->cbMaxSize >= (SHORT)pfucb->kdfCurr.data.Cb() );
                pverreplace->cbMaxSize = pverreplacePrev->cbMaxSize;
            }
            else
            {
                //  no previous replace. max size is the size of our before image
                pverreplace->cbMaxSize = (SHORT)pfucb->kdfCurr.data.Cb();
            }

            pverreplace->cbDelta = 0;

            Assert( prce->Oper() == operReplace );

            // move to data byte and copy old data (before image)
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


//  ================================================================
LOCAL BOOL FVERIWriteConflict(
    FUCB*           pfucb,
    const BOOKMARK& bookmark,
    UINT            uiHash,
    const OPER      oper
    )
//  ================================================================
{
    BOOL            fWriteConflict  = fFalse;
    const TRX       trxSession      = TrxVERISession( pfucb );
    RCE*            prce            = PrceRCEGet(
                                            uiHash,
                                            pfucb->ifmp,
                                            pfucb->u.pfcb->PgnoFDP(),
                                            bookmark );

    Assert( trxSession != trxMax );

    //  check for write conficts
    //  we can't use the pfucb of a committed transaction as the FUCB has been closed
    //  if a version is committed after we started however, it must have been
    //  created by another session
    if ( prce != prceNil )
    {
        if ( prce->FActiveNotByMe( pfucb->ppib, trxSession ) )
        {
            if ( operReadLock == oper || operDelta == oper || operDelta64 == oper )
            {
                //  these operations commute. i.e. two sessions can perform
                //  these operations without conflicting
                //  we can only do this modification if all the active RCE's
                //  in the chain are of this type
                //  look at all active versions for an operation not of this type
                //  OPTIMIZATION:   if the session changes again (i.e. we get a
                //                  _third_ session) we can stop looking as we
                //                  know that the second session commuted with
                //                  the first, therefore the third will commute
                //                  with the second and first (transitivity)
                const RCE   * prceT         = prce;
                for ( ;
                    prceNil != prceT && TrxCmp( prceT->TrxCommitted(), trxSession ) > 0;
                    prceT = prceT->PrcePrevOfNode() )
                {
                    //  if all active RCEs have the same oper we are OK,
                    //  else WriteConflict.
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
                // Must be my uncommitted version, otherwise it would have been
                // caught by FActiveNotByMe().
                Assert( prce->Pfucb()->ppib == pfucb->ppib );
                Assert( prce->Level() <= pfucb->ppib->Level()
                        || PinstFromIfmp( pfucb->ifmp )->FRecovering() );       //  could be an RCE created by redo of UndoInfo
            }
            else
            {
                //  RCE exists, but it committed before our session began, so no
                //  danger of write conflict.
                //  Normally, this session's Begin0 cannot be equal to anyone else's Commit0,
                //  but because we only log an approximate trxCommit0, during recovery, we
                //  may find that this session's Begin0 is equal to someone else's Commit0
                Assert( TrxCmp( prce->TrxCommitted(), trxSession ) < 0
                    || ( prce->TrxCommitted() == trxSession && PinstFromIfmp( pfucb->ifmp )->FRecovering() ) );
            }
#endif

            if ( prce->Oper() != oper && (  operReadLock == prce->Oper()
                                            || operDelta == prce->Oper()
                                            || operDelta64 == prce->Oper() ) )
            {
                //  these previous operation commuted. i.e. two sessions can perform
                //  these operations without conflicting
                //
                //  we are creating a different type of operation that does
                //  not commute
                //
                //  therefore we must check all active versions to make sure
                //  that we are the only session that has created them
                //
                //  we can only do this modification if all the active RCE's
                //  in the chain created by us
                //
                //  look at all versions for a active versions for a different session
                //
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
                    || PinstFromIfmp( prce->Pfucb()->ifmp )->FRecovering() );       //  could be an RCE created by redo of UndoInfo
            }

            if ( prce->Oper() == operFlagDelete )
            {
                //  normally, the only RCE that can follow a FlagDelete is an Insert.
                //  unless the RCE was moved, or if we're recovering, in which case we might
                //  create RCE's for UndoInfo
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


//  ================================================================
INLINE ERR VER::ErrVERModifyCommitted(
    FCB             *pfcb,
    const BOOKMARK& bookmark,
    const OPER      oper,
    const TRX       trxBegin0,
    const TRX       trxCommitted,
    RCE             **pprce
    )
//  ================================================================
//
//  Used by concurrent create index to create a RCE as though it was done
//  by another session. The trxCommitted of the RCE is set and no checks for
//  write conflicts are done
//
//-
{
    ASSERT_VALID( &bookmark );
    Assert( pfcb->FTypeSecondaryIndex() );      // only called from concurrent create index.
    Assert( pfcb->PfcbTable() == pfcbNil );
    Assert( trxCommitted != trxMax );

    const UINT          uiHash  = UiRCHashFunc( pfcb->Ifmp(), pfcb->PgnoFDP(), bookmark );

    ERR                 err     = JET_errSuccess;
    RCE                 *prce   = prceNil;

    //  assert default return value
    Assert( NULL != pprce );
    Assert( prceNil == *pprce );

    {
        //  calculate the length of the RCE in the bucket.
        const INT cbBookmark = bookmark.key.Cb() + bookmark.data.Cb();
        const INT cbNewRCE = sizeof( RCE ) + cbBookmark;

        //  Set up a skeleton RCE. This holds m_critBucketGlobal, so do it
        //  first before filling the rest.
        //
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

        //  copy the bookmark
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

//  ================================================================
ERR VER::ErrVERCheckTransactionSize( PIB * const ppib )
//  ================================================================
{
    ERR err = JET_errSuccess;
    if ( m_fAboveMaxTransactionSize )
    {
        UpdateCachedTrxOldest( m_pinst );

        // If this is the oldest transaction and the version store is too
        // full, return an error
        if ( ppib->trxBegin0 == TrxOldestCached( m_pinst ) )
        {
            const BOOL fCleanupWasRun   = m_msigRCECleanPerformedRecently.FWait( cmsecAsyncBackgroundCleanup );

            m_critBucketGlobal.Enter();

            VERIReportVersionStoreOOM ( ppib, fTrue /* fMaxTrxSize */, fCleanupWasRun );

            m_critBucketGlobal.Leave();

            Error( ErrERRCheck( JET_errVersionStoreOutOfMemory ) );
        }
    }

HandleError:
    return err;
}


//  ================================================================
ERR VER::ErrVERModify(
    FUCB            * pfucb,
    const BOOKMARK& bookmark,
    const OPER      oper,
    RCE             **pprce,
    const VERPROXY  * const pverproxy
    )
//  ================================================================
//
//  Create an RCE for a DML operation.
//
//  OPTIMIZATION:   combine delta/readLock/replace versions
//                  remove redundant replace versions
//
//  RETURN VALUE
//      Jet_errWriteConflict for two cases:
//          -for any committed node, caller's transaction begin time
//          is less than node's level 0 commit time.
//          -for any uncommitted node except operDelta/operReadLock at all by another session
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );
    Assert( FOperInHashTable( oper ) ); //  not supposed to be in hash table? use VERFlag
    Assert( !bookmark.key.FNull() );
    Assert( !g_rgfmp[pfucb->ifmp].FVersioningOff() );
    Assert( !pfucb->ppib->FReadOnlyTrx() );
    AssertTrack( !g_rgfmp[pfucb->ifmp].FShrinkIsRunning(), "VerCreateDuringShrink" );

    //  set default return value
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

    //  we never create an insert version at runtime. instead we create a writeLock version
    //  and use ChangeOper to change it into an insert
    //
    Assert( m_pinst->m_plog->FRecovering() || operInsert != oper );

    Assert( !m_pinst->m_plog->FRecovering() || ( fProxy && proxyRedo == pverproxy->proxy ) );
    if ( fProxy )
    {
        if ( proxyCreateIndex == pverproxy->proxy )
        {
            Assert( !m_pinst->m_plog->FRecovering() );
            Assert( oper == operWriteLock
                || oper == operPreInsert
                || oper == operReplace      // via FlagInsertAndReplaceData
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
                Assert( prcePrimary->Pfucb()->ppib->CritTrx().FOwner() );

                level = prcePrimary->Level();

                // Need to allocate an FUCB for the proxy, in case it rolls back.
                CallR( ErrDIROpenByProxy(
                            prcePrimary->Pfucb()->ppib,
                            pfucb->u.pfcb,
                            &pfucbProxy,
                            level ) );
                Assert( pfucbNil != pfucbProxy );

                //  force pfucbProxy to be defer-closed, so that it will
                //  not be released until the owning session commits
                //  or rolls back
                Assert( pfucbProxy->ppib->Level() > 0 );
                FUCBSetVersioned( pfucbProxy );

                // Use proxy FUCB for versioning.
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
            || operPreInsert == oper )      //  HACK: this oper will get promoted to operInsert on success (or manually nullified on failure)
        {
            updateid = UpdateidOfPpib( pfucb->ppib );
        }
        else
        {
            //  If in the middle of an update, only a few
            //  non-DML RCE's are possible.  These RCE's
            //  will remain outstanding (by design) even
            //  if the update rolls back.
            //
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
        // For concurrent operations, we had to obtain rwlHash before
        // allocating an rceid, to ensure that rceid's are chained in order.
        Assert( RwlRCEChain( uiHash ).FWriter() );
    }
    else
    {
        // For non-concurrent operations, rceid's won't be chained out
        // of order because all but one RCE will fail with write-conflict
        // below.
        RwlRCEChain( uiHash ).EnterAsWriter();
    }

    // UNDONE: CIM support for updates is currently broken -- only
    // used for dirty reads by concurrent create index.
    Assert( FPIBVersion( pfucb->ppib )
        || ( prceNil != prcePrimary && prcePrimary->Pfucb()->ppib == pfucb->ppib ) );

    if ( !m_pinst->m_plog->FRecovering() && FVERIWriteConflict( pfucb, bookmark, uiHash, oper ) )
    {
        //  UNDONE: is there a way to easily report the bm?
        //
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
#endif  //  DEBUG_VER

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
        Assert( pfucbProxy->ppib->Level() > 0 );    // Ensure defer-closed, even on error
        Assert( FFUCBVersioned( pfucbProxy ) );
        DIRClose( pfucbProxy );
    }

    return err;
}


//  ================================================================
ERR VER::ErrVERFlag( FUCB * pfucb, OPER oper, const VOID * pv, INT cb )
//  ================================================================
//
//  Creates a RCE for a DDL or space operation. The RCE is not put
//  in the hash table
//
//-
{
#ifdef DEBUG
    ASSERT_VALID( pfucb );
    Assert( pfucb->ppib->Level() > 0 );
    Assert( cb >= 0 );
    Assert( !FOperInHashTable( oper ) );    //  supposed to be in hash table? use VERModify
    Ptls()->fAddColumn = operAddColumn == oper;
#endif  //  DEBUG

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

    //  Set up a skeleton RCE. This holds m_critBucketGlobal, so do it
    //  first before filling the rest.
    //
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
#endif  //  DEBUG

    return err;
}


//  ================================================================
VOID VERSetCbAdjust(
            CSR         *pcsr,
    const   RCE         *prce,
            INT         cbDataNew,
            INT         cbDataOld,
            UPDATEPAGE  updatepage )
//  ================================================================
//
//  Sets the max size and delta fields in a replace RCE
//
//  WARNING:  The following comments explain how a Replace RCE's delta field
//  (ie. the second SHORT stored in rgbData) is used.  The semantics can get
//  pretty confusing, so PLEASE DO NOT REMOVE THESE COMMENTS.  -- JL
//
//  *psDelta records how much the operation contributes to deferred node
//  space reservation. A positive cbDelta here means the node is growing,
//  so we will use up space which may have been reserved (ie. *psDelta will
//  decrease).  A negative cbDelta here means the node is shrinking,
//  so we must add abs(cbDelta) to the *psDelta to reflect how much more node
//  space must be reserved.
//
//  This is how to interpret the value of *psDelta:
//      - if *psDelta is positive, then *psDelta == reserved node space.  *psDelta can only
//        be positive after a node shrinkage.
//      - if *psDelta is negative, then abs(*psDelta) is the reserved node space that
//        was consumed during a node growth.  *psDelta can only become negative
//        after a node shrinkage (which sets aside some reserved node space)
//        followed by a node growth (which consumes some/all of that
//        reserved node space).
//
//-
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

    //  set new node maximum size.
    if ( cbDataNew > cbMax )
    {
        //  this is the largest the node has ever been. set the max size and
        //  free all previously reserved space
        Assert( cbDelta > 0 );
        pverreplace->cbMaxSize  = SHORT( cbDataNew );
        cbDelta                 = cbMax - cbDataOld;
    }

    pverreplace->cbDelta = SHORT( pverreplace->cbDelta - cbDelta );

    if ( fDoUpdatePage == updatepage )
    {
        if ( cbDelta > 0 )
        {
            // If, during this transaction, we've shrunk the node.  There will be
            // some uncommitted freed space.  Reclaim as much of this as needed to
            // satisfy the new node growth.  Note that we can update cbUncommittedFreed
            // in this fashion because the subsequent call to ErrPMReplace() is
            // guaranteed to succeed (ie. the node is guaranteed to grow).
            pcsr->Cpage().ReclaimUncommittedFreed( cbDelta );
        }
        else if ( cbDelta < 0 )
        {
            // Node has decreased in size.  The page header's cbFree has already
            // been increased to reflect this.  But we must also increase
            // cbUncommittedFreed to indicate that the increase in cbFree is
            // contingent on commit of this operation.
            pcsr->Cpage().AddUncommittedFreed( -cbDelta );
        }
    }
#ifdef DEBUG
    else
    {
        Assert( fDoNotUpdatePage == updatepage );
    }
#endif  //  DEBUG
}


//  ================================================================
LOCAL INT CbVERIGetNodeMax( const FUCB * pfucb, const BOOKMARK& bookmark, UINT uiHash )
//  ================================================================
//
//  This assumes nodeMax is propagated through the replace RCEs. Assumes
//  it is in the critical section to CbVERGetNodeReserverved can use it for
//  debugging
//
//-
{
    INT         nodeMax = 0;

    // Look for any replace RCE's.
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


//  ================================================================
INT CbVERGetNodeMax( const FUCB * pfucb, const BOOKMARK& bookmark )
//  ================================================================
//
//  This enters the critical section and calls CbVERIGetNodeMax
//
//-
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( &bookmark );

    const UINT              uiHash  = UiRCHashFunc( pfucb->ifmp, pfucb->u.pfcb->PgnoFDP(), bookmark );
    ENTERREADERWRITERLOCK   enterRwlHashAsReader( &( PverFromIfmp( pfucb->ifmp )->RwlRCEChain( uiHash ) ), fTrue );
    const INT               nodeMax = CbVERIGetNodeMax( pfucb, bookmark, uiHash );

    Assert( nodeMax >= 0 );
    return nodeMax;
}


//  ================================================================
INT CbVERGetNodeReserve( const PIB * ppib, const FUCB * pfucb, const BOOKMARK& bookmark, INT cbCurrentData )
//  ================================================================
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

    //  find all uncommitted replace RCE's for this node
    //
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

    //  double-check one last time that the transaction with the outstanding
    //  replace RCE's is still outstanding (or more importantly, that it
    //  didn't commit while we were traversing the RCE list, because that
    //  may result in a bogus, or even negative, reserved node space
    //  calculation)
    //
    if ( prceNil != prceFirstUncommittedReplace
        && trxMax != prceFirstUncommittedReplace->TrxCommitted() )
    {
        cbNodeReserve = 0;
    }
    else if ( 0 != cbNodeReserve )
    {
        //  the deltas should always net out to a non-negative value
        //
        Assert( cbNodeReserve > 0 );

        //  the deltas should net out to the same value as the max node size minus
        //  the current node size, except if we compute a bogus max size because
        //  the transaction with the outstanding replace RCE's commits while we're
        //  trying to compute the max node size
        //
        Assert( cbNodeReserve == CbVERIGetNodeMax( pfucb, bookmark, uiHash ) - (INT)cbCurrentData
            || ( prceNil != prceFirstUncommittedReplace && trxMax != prceFirstUncommittedReplace->TrxCommitted() ) );
    }

    return cbNodeReserve;
}


//  ================================================================
BOOL FVERCheckUncommittedFreedSpace(
    const FUCB  * pfucb,
    CSR         * const pcsr,
    const INT   cbReq,
    const BOOL  fPermitUpdateUncFree )
//  ================================================================
//
// This function is called after it has been determined that cbFree will satisfy
// cbReq. We now check that cbReq doesn't use up any uncommitted freed space.
//
//-
{
    BOOL    fEnoughPageSpace    = fTrue;

    ASSERT_VALID( pfucb );
    ASSERT_VALID( pcsr );
    Assert( cbReq <= pcsr->Cpage().CbPageFree() );

    //  during recovery we would normally set cbUncommitted free to 0
    //  but if we didn't redo anything on the page and are now rolling
    //  back or if this is viewcache, this may not be the case

    if ( PinstFromPfucb( pfucb )->FRecovering() )
    {
        if ( fPermitUpdateUncFree && pcsr->Cpage().CbUncommittedFree() != 0 )
        {
            //  recovery is single threaded so we should only conflict with the buffer manager;
            //  however, this is known to always fail for ViewCache.

            LATCH   latchOld;
            ERR err = pcsr->ErrUpgradeToWARLatch( &latchOld );

            if ( err == JET_errSuccess )
            {
                pcsr->Cpage().SetCbUncommittedFree( 0 );
                BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy, *TraceContextScope() );  // Not tracing untidied pages
                pcsr->DowngradeFromWARLatch( latchOld );
            }
        }

        //  even if we were unable to update cbUncommittedFree, we know that the
        //  it is essentially zero (because of recovery); so just return fTrue.
        
        return fTrue;
    }


    // We should already have performed the check against cbFree only (in other
    // words, this function is only called from within FNDFreePageSpace(),
    // or something that simulates its function).  This tells us that if all
    // currently-uncommitted transactions eventually commit, we should have
    // enough space to satisfy this request.
    Assert( cbReq <= pcsr->Cpage().CbPageFree() );

    // The amount of space freed but possibly uncommitted should be a subset of
    // the total amount of free space for this page.
    Assert( pcsr->Cpage().CbUncommittedFree() >= 0 );
    Assert( pcsr->Cpage().CbPageFree() >= pcsr->Cpage().CbUncommittedFree() );

    // In the worst case, all transactions that freed space on this page will
    // rollback, causing the space freed to be reclaimed.  If the space
    // required can be satisfied even in the worst case, then we're okay;
    // otherwise, we have to do more checking.
    if ( cbReq > pcsr->Cpage().CbPageFree() - pcsr->Cpage().CbUncommittedFree() )
    {
        Assert( !FFUCBSpace( pfucb ) );
        Assert( !pcsr->Cpage().FSpaceTree() );

        //  UNDONE: use the CbNDUncommittedFree call
        //          to get rglineinfo for later split
        //          this will reduce CPU usage for RCE hashing
        //
        const INT   cbUncommittedFree = CbNDUncommittedFree( pfucb, pcsr );
        Assert( cbUncommittedFree >= 0 );

        if ( cbUncommittedFree == pcsr->Cpage().CbUncommittedFree() )
        {
            //  cbUncommittedFreed in page is correct
            //  return
            //
            fEnoughPageSpace = fFalse;
        }
        else
        {
            if ( fPermitUpdateUncFree )
            {
                // Try updating cbUncommittedFreed, in case some freed space was committed.
                LATCH latchOld;
                if ( pcsr->ErrUpgradeToWARLatch( &latchOld ) == JET_errSuccess )
                {
                    pcsr->Cpage().SetCbUncommittedFree( cbUncommittedFree );
                    BFDirty( pcsr->Cpage().PBFLatch(), bfdfUntidy, *TraceContextScope() );  // Not tracing untidied pages
                    pcsr->DowngradeFromWARLatch( latchOld );
                }
            }

            // The amount of space freed but possibly uncommitted should be a subset of
            // the total amount of free space for this page.
            Assert( pcsr->Cpage().CbUncommittedFree() >= 0 );
            Assert( pcsr->Cpage().CbPageFree() >= pcsr->Cpage().CbUncommittedFree() );

            fEnoughPageSpace = ( cbReq <= ( pcsr->Cpage().CbPageFree() - cbUncommittedFree ) );
        }
    }

    return fEnoughPageSpace;
}



//  ****************************************************************
//  RCE CLEANUP
//  ****************************************************************

//  ================================================================
ERR RCE::ErrGetTaskForDelete( VOID ** ppvtask ) const
//  ================================================================
{
    ERR                 err     = JET_errSuccess;
    DELETERECTASK *     ptask   = NULL;

    //  since we have the RwlRCEChain, we're guaranteed that the RCE will not go away,
    //  nor will it be nullified

    const UINT              uiHash      = m_uiHash;
    ENTERREADERWRITERLOCK   enterRwlRCEChainAsReader( &( PverFromIfmp( Ifmp() )->RwlRCEChain( uiHash ) ), fTrue );

    if ( !FOperNull() )
    {
        Assert( operFlagDelete == Oper() || operInsert == Oper() );

        //  no cleanup for temporary tables or tables/indexes scheduled for deletion
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


//  ================================================================
ERR VER::ErrVERIDelete( PIB * ppib, const RCE * const prce )
//  ================================================================
{
    ERR             err;
    DELETERECTASK * ptask;

    Assert( ppibNil != ppib );
    Assert( m_critRCEClean.FOwner() );

    //  We've had a rollback failure on this session, we're a stone throw away from terminating uncleanly, bail ...
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
        //  we determined that a task wasn't needed for whatever reason
        //
        CallS( err );
    }

    else if ( m_fSyncronousTasks
        || g_rgfmp[prce->Ifmp()].FDetachingDB()
        || m_pinst->Taskmgr().CPostedTasks() > UlParam( m_pinst, JET_paramVersionStoreTaskQueueMax ) )  //  if task manager is overloaded, perform task synchronously
    {
        IncrementCSyncCleanupDispatched();
        TASK::Dispatch( m_ppibRCECleanCallback, (ULONG_PTR)ptask );
        CallS( err );
    }

    else
    {
        IncrementCAsyncCleanupDispatched();
        // DELETERECTASK is a RECTASK so use the RECTASKBATCHER object to batch the tasks together      
        err = m_rectaskbatcher.ErrPost( ptask );
    }

    return err;
}


//  ================================================================
LOCAL VOID VERIFreeExt( PIB * const ppib, FCB *pfcb, PGNO pgnoFirst, CPG cpg )
//  ================================================================
//  Throw away any errors encountered -- at worst, we just lose space.
{
    Assert( pfcb );

    ERR     err;
    FUCB    *pfucb = pfucbNil;

    Assert( !PinstFromPpib( ppib )->m_plog->FRecovering() );
    Assert( ppib->Level() > 0 );

    err = ErrDBOpenDatabaseByIfmp( ppib, pfcb->Ifmp() );
    if ( err < 0 )
        return;

    // Can't call DIROpen() because this function gets called
    // only during rollback AFTER the logic in DIRRollback()
    // which properly resets the navigation level of this
    // session's cursors.  Thus, when we close the cursor
    // below, the navigation level will not get properly reset.
    // Call( ErrDIROpen( ppib, pfcb, &pfucb ) );
    Call( ErrBTOpen( ppib, pfcb, &pfucb ) );

    Assert( !FFUCBSpace( pfucb ) );

    const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

    (VOID)ErrSPFreeExt( pfucb, pgnoFirst, cpg, "VerFreeExt" );
    
    // Restore cleanup checking
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
//  ================================================================
BOOL FIsRCECleanup()
//  ================================================================
//
//  DEBUG:  is the current thread a RCECleanup thread?
{
    return Ptls()->fIsRCECleanup;
}

//  ================================================================
BOOL FInCritBucket( VER *pver )
//  ================================================================
//
//  DEBUG:  is the current thread in m_critBucketGlobal
{
    return pver->m_critBucketGlobal.FOwner();
}
#endif  //  DEBUG


//  ================================================================
BOOL FPIBSessionRCEClean( PIB *ppib )
//  ================================================================
//
// Is the given PIB the one used by RCE clean?
//
//-
{
    Assert( ppibNil != ppib );
    return ( (PverFromPpib( ppib ))->m_ppibRCEClean == ppib );
}


//  ================================================================
INLINE VOID VERIUnlinkDefunctSecondaryIndex(
    PIB * const ppib,
    FCB * const pfcb )
//  ================================================================
{
    Assert( pfcb->FTypeSecondaryIndex() );

    // Must unlink defunct FCB from all deferred-closed cursors.
    // The cursors themselves will be closed when the
    // owning session commits or rolls back.
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
            // If cursor belongs to us, we can close
            // it right now.
            FUCBClose( pfucbT );
        }
        else
        {
            BOOL fDoUnlink = fFalse;
            ppibT->CritTrx().Enter();

            //  if undoing CreateIndex, we know other session must be
            //  in a transaction if it has a link to this FCB because
            //  the index is not visible yet.

            // FCB may have gotten unlinked if other session
            // committed or rolled back while we were switching
            // critical sections.

            // We rely on pfcb->Pfucb() continuing to return the same value until the FUCB* is
            // unlinked.  That is, we rely on any addition or removal to the FUCB list on the
            // FCB to leave the 0th element in place as the 0th element, although any other
            // reordering is possible.
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

            ppibT->CritTrx().Leave();
        }

        pfcb->Lock();
    }

    pfcb->Unlock();
}


//  ================================================================
INLINE VOID VERIUnlinkDefunctLV(
    PIB * const ppib,
    FCB * const pfcb )
//  ================================================================
//
//  If we are rolling back, only the owning session could have seen
//  the LV tree, because
//      -- the table was opened exclusively and the session that opened
//      the table created the LV tree
//      -- ppibLV created the LV tree and is rolling back before returning
//
//-
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


//  ================================================================
template< typename TDelta >
ERR VER::ErrVERICleanDeltaRCE( const RCE * const prce )
//  ================================================================
{
    ERR       err = JET_errSuccess;
    IFMP      ifmp;
    RECTASK  *ptask;
    PIB      *ppib;
    
    // To access the RCE, we need to take the RwlRCE chain to prevent others
    // from nullifying it.
    {
    ENTERREADERWRITERLOCK enterRwlRCEChainAsWriter( &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ), fFalse );

    if ( prce->Oper() != _VERDELTA<TDelta>::TRAITS::oper )
    {
        //  Nullified by someone else (e.g. VERNullifyInactiveVersionsOnBM), nothing for us to do
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
        Assert( !prce->Pfcb()->Ptdb() );    //  LV trees don't have TDB's
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
        // Nothing to do
        return err;
    }

    //  We've had a rollback failure on this session, we're a stone throw away from terminating uncleanly, bail ...
    if ( ppib->ErrRollbackFailure() < JET_errSuccess )
    {
        err = m_pinst->ErrInstanceUnavailableErrorCode() ? m_pinst->ErrInstanceUnavailableErrorCode() : JET_errInstanceUnavailable;
        delete ptask;
        return( ErrERRCheck( err ) );
    }

    ifmp = prce->Ifmp();

    //  In order to do the next steps, we need to leave the critical section
    Assert( prce->Oper() == operDelta || prce->Oper() == operDelta64 );
    } // End of local scope to release the critical section

    //  The RCE can not go away because we are on a cleanup thread processing this RCE. 
    //  Careful to not access it, however, to avoid FReadable asserts if it gets nullified.
    Assert( ptask );
    Assert( ppib );
    Assert( FIsRCECleanup() );
    
    if ( m_fSyncronousTasks
        || g_rgfmp[ ifmp ].FDetachingDB()
        || m_pinst->Taskmgr().CPostedTasks() > UlParam( m_pinst, JET_paramVersionStoreTaskQueueMax ) )  //  if task manager is overloaded, perform task synchronously
    {
        IncrementCSyncCleanupDispatched();
        TASK::Dispatch( ppib, (ULONG_PTR)ptask );
    }
    else
    {
        IncrementCAsyncCleanupDispatched();

        // DELETELVTASK and FINALIZETASK are RECTASKs so use the RECTASKBATCHER object to batch the tasks together
        err = m_rectaskbatcher.ErrPost( ptask );
    }

    return err;
}


//  ================================================================
LOCAL VOID VERIRemoveCallback( const RCE * const prce )
//  ================================================================
//
//  Remove the callback from the list
//
//-
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
    //  if another thread nullifies this RCE (via VERNullifyAllVersionsOnFCB
    //  or VERNullifyInactiveVersionsOnBM) while we were trying to discard it,
    //  then don't count it as a discard
    //
    if ( !prce->FOperNull() )
    {
        PERFOpt( cVERCleanupDiscarded.Inc( m_pinst ) );

        VERIReportDiscardedDeletes( prce );
    }
}


VOID VERIWaitForTasks( VER *pver, FCB *pfcb, BOOL fInRollback, BOOL fHaveRceCleanLock )
{
    //  wait for all tasks on the FCB to complete
    //  no new tasks should be created because there is a delete on the FCB
    //
    //  UNDONE: we're assuming that the tasks we're waiting
    //  for will be serviced by other threads while this task
    //  thread waits, but is it possible that NT only assigns
    //  one thread to us, meaning the other tasks will never
    //  get serviced and we will therefore end up waiting
    //  forever?
    //
    //  Post all pending tasks to avoid deadlock where we wait for tasks
    //  which haven't been issued yet
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

//  ================================================================
ERR VER::ErrVERICleanOneRCE( RCE * const prce )
//  ================================================================
{
    ERR err = JET_errSuccess;

    Assert( m_critRCEClean.FOwner() );
    Assert( !FFMPIsTempDB( prce->Ifmp() ) );
    Assert( prce->TrxCommitted() != trxMax );
    Assert( !prce->FRolledBack() );

    switch( prce->Oper() )
    {
        case operCreateTable:
            // RCE list ensures FCB is still pinned
            Assert( pfcbNil != prce->Pfcb() );
            Assert( prce->Pfcb()->PrceOldest() != prceNil );
            if ( prce->Pfcb()->FTypeTable() )
            {
                if ( FCATHashActive( m_pinst ) )
                {

                    //  catalog hash is active so we need to insert this table

                    CHAR szTable[JET_cbNameMost+1];

                    //  read the table-name from the TDB

                    prce->Pfcb()->EnterDML();
                    OSStrCbFormatA( szTable, sizeof(szTable), prce->Pfcb()->Ptdb()->SzTableName() );
                    prce->Pfcb()->LeaveDML();

                    //  insert the table into the catalog hash

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
            // RCE list ensures FCB is still pinned
            Assert( prce->Pfcb()->PrceOldest() != prceNil );

            Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
            const JET_COLUMNID      columnid            = ( (VERADDCOLUMN*)prce->PbData() )->columnid;
            BYTE                    * pbOldDefaultRec   = ( (VERADDCOLUMN*)prce->PbData() )->pbOldDefaultRec;
            FCB                     * pfcbTable         = prce->Pfcb();

            pfcbTable->EnterDDL();

            TDB                     * const ptdb        = pfcbTable->Ptdb();
            FIELD                   * const pfield      = ptdb->Pfield( columnid );

            FIELDResetVersionedAdd( pfield->ffield );

            // Only reset the Versioned bit if a Delete
            // is not pending.
            if ( FFIELDVersioned( pfield->ffield ) && !FFIELDDeleted( pfield->ffield ) )
            {
                FIELDResetVersioned( pfield->ffield );
            }

            //  should be impossible for current default record to be same as old default record,
            //  but check anyways to be safe
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
            // RCE list ensures FCB is still pinned
            Assert( prce->Pfcb()->PrceOldest() != prceNil );

            prce->Pfcb()->EnterDDL();

            Assert( prce->CbData() == sizeof(COLUMNID) );
            const COLUMNID  columnid        = *( (COLUMNID*)prce->PbData() );
            TDB             * const ptdb    = prce->Pfcb()->Ptdb();
            FIELD           * const pfield  = ptdb->Pfield( columnid );

            // If field was version-added, it would have been cleaned
            // up by now.
            Assert( pfield->coltyp != JET_coltypNil );

            // UNDONE: Don't reset coltyp to Nil, so that we can support
            // column access at level 0.
///                 pfield->coltyp = JET_coltypNil;

            //  remove the column name from the TDB name space
            ptdb->MemPool().DeleteEntry( pfield->itagFieldName );

            // Reset version and autoinc fields.
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
            //  pfcb of secondary index FCB or pfcbNil for primary
            //  index creation
            FCB                     * const pfcbT = *(FCB **)prce->PbData();
            FCB                     * const pfcbTable = prce->Pfcb();
            FCB                     * const pfcbIndex = ( pfcbT == pfcbNil ? pfcbTable : pfcbT );
            IDB                     * const pidb = pfcbIndex->Pidb();

            pfcbTable->EnterDDL();

            Assert( pidbNil != pidb );

            Assert( pfcbTable->FTypeTable() );

            if ( pfcbTable == pfcbIndex )
            {
                // VersionedCreate flag is reset at commit time for primary index.
                Assert( !pidb->FVersionedCreate() );
                Assert( !pidb->FDeleted() );
                pidb->ResetFVersioned();
            }
            else if ( pidb->FVersionedCreate() )
            {
                pidb->ResetFVersionedCreate();

                // If deleted, Versioned bit will be properly reset when
                // Delete commits or rolls back.
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

            // Use dummy ppib because we lost the original one when the
            // transaction committed.
            Assert( pfcbIndex->FDomainDenyRead( m_ppibRCEClean ) );

            Assert( pfcbIndex->Pidb() != pidbNil );
            Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
            Assert( pfcbIndex->Pidb()->FDeleted() );
            Assert( !pfcbIndex->Pidb()->FVersioned() );

            pfcbTable->UnlinkSecondaryIndex( pfcbIndex );

            pfcbTable->LeaveDDL();
            pfcbTable->ResetIndexing();

            //  verify not called during recovery, which would be
            //  bad because VERNullifyAllVersionsOnFCB() enters
            //  m_critRCEClean, which we already have
            Assert( !m_pinst->FRecovering() );
            VERNullifyAllVersionsOnFCB( pfcbIndex );
            VERIUnlinkDefunctSecondaryIndex( ppibNil, pfcbIndex );

            //  prepare the FCB to be purged
            //  this removes the FCB from the hash-table among other things
            //      so that the following case cannot happen:
            //          we free the space for this FCB
            //          someone else allocates it
            //          someone else BTOpen's the space
            //          we try to purge the table and find that the refcnt
            //              is not zero and the state of the FCB says it is
            //              currently in use!
            //          result --> CONCURRENCY HOLE

            pfcbIndex->PrepareForPurge( fFalse );

            //  if the parent (ie. the table) is pending deletion, we
            //  don't need to bother freeing the index space because
            //  it will be freed when the parent is freed
            //  Note that the DeleteCommitted flag is only ever set
            //  when the delete is guaranteed to be committed.  The
            //  flag NEVER gets reset, so there's no need to grab
            //  the FCB critical section to check it.

            if ( !pfcbTable->FDeleteCommitted() )
            {
                if ( ErrLGFreeFDP( pfcbIndex, prce->TrxCommitted() ) >= JET_errSuccess )
                {
                    //  ignore errors if we can't free the space
                    //  (it will be leaked)
                    //
                    (VOID)ErrSPFreeFDP(
                                m_ppibRCEClean,
                                pfcbIndex,
                                pfcbTable->PgnoFDP() );
                }
            }

            //  purge the FCB

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

            //  verify VERNullifyAllVersionsOnFCB() not called during recovery,
            //  which would be bad because VERNullifyAllVersionsOnFCB() enters
            //  m_critRCEClean, which we already have
            Assert( !m_pinst->FRecovering() );

            // Remove all associated FCB's from hash table, so they will
            // be available for another file using the FDP that is about
            // about to be freed.
            for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
            {
                Assert( pfcbT->FDeletePending() );
                Assert( pfcbT->FDeleteCommitted() );

                VERIWaitForTasks( this, pfcbT, fFalse, fTrue );

                // bugfix (#45382): May have outstanding moved RCE's
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

                    // bugfix (#36315): processing of delta RCEs may have created flagDelete/writeLock
                    // RCEs after this RCE.
                    Assert( pfcbLV->PrceOldest() == prceNil
                         || pfcbLV->PrceOldest()->Oper() == operFlagDelete
                         || pfcbLV->PrceOldest()->Oper() == operWriteLock );
                    VERNullifyAllVersionsOnFCB( pfcbLV );

                    pfcbLV->PrepareForPurge( fFalse );
                }
            }
            else
            {
                FireWall( "DeprecatedSentinelFcbVerCleanFcbNil" ); // Sentinel FCBs are believed deprecated
            }

            if ( ErrLGFreeFDP( pfcbTable, prce->TrxCommitted() ) >= JET_errSuccess )
            {
                //  free table FDP (which implicitly frees child FDP's)
                //
                //  ignore errors if we can't free the space
                //  (it will be leaked)
                //
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

                // All transactions which were able to access this table
                // must have committed and been cleaned up by now.
                Assert( pfcbTable->PrceOldest() == prceNil );
                Assert( pfcbTable->PrceNewest() == prceNil );
            }
            else
            {
                FireWall( "DeprecatedSentinelFcbVerCleanFcbInvState" ); // Sentinel FCBs are believed deprecated
            }
            pfcbTable->Purge();

            break;
        }

        case operRegisterCallback:
        {
            //  the callback is now visible to all transactions
            //  CONSIDER: unset the fVersioned flag if the callback has not been unregistered
        }
            break;

        case operUnregisterCallback:
        {
            //  the callback cannot be seen by any transaction. remove the callback from the list
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
                //  don't bother cleaning if there are future versions
                if ( !prce->FFutureVersionsOfNode() )
                {
                    err = ErrVERIDelete( m_ppibRCEClean, prce );
                }
            }

            break;
        }

        case operDelta:
            //  we may have to defer delete a LV
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
            //  we may have to defer delete a LV
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


//  ================================================================
ERR RCE::ErrPrepareToDeallocate( TRX trxOldest )
//  ================================================================
//
// Called by RCEClean to clean/nullify RCE before deallocation.
//
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
        //  RCE may have been nullified by VERNullifyAllVersionsOnFCB()
        //  (which is called while closing the temp table).
        //  or RCE may have been moved instead of being cleaned up
    }
    else
    {
        Assert( !FRolledBack() );
        Assert( !FOperNull() );

        //  Clean up the RCE. Also clean up any RCE of the same list to reduce
        //  Enter/leave critical section calls.

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
                operFlagDelete != prce->Oper() &&       // Let RCE clean do the nullify for delete.
                operDelta != prce->Oper() &&            // Delta is used to indicate if
                operDelta64 != prce->Oper() );          // it needs to do LV delete.
    }

    return JET_errSuccess;
}


//  ================================================================
ERR VER::ErrVERRCEClean( const IFMP ifmp )
//  ================================================================
//  critical section wrapper
{
    //  clean PIB in critical section held across IO operations

    m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
    const ERR err = ErrVERIRCEClean( ifmp );
    m_critRCEClean.Leave();
    return err;
}

//  ================================================================
ERR VER::ErrVERIRCEClean( const IFMP ifmp )
//  ================================================================
//
//  Cleans RCEs in bucket chain.
//  We only clean up the RCEs that has a commit timestamp older
//  that the oldest XactBegin of any user.
//
//-
{
    Assert( m_critRCEClean.FOwner() );

    const BOOL fCleanOneDb = ( ifmp != g_ifmpMax );

    // Only two places we do per-DB RCE Clean is just before we want to
    // detach the database, running shrink or reclaiming space leaks.
    Assert( !fCleanOneDb || g_rgfmp[ifmp].FDetachingDB() || g_rgfmp[ifmp].FShrinkIsRunning() || g_rgfmp[ifmp].FLeakReclaimerIsRunning() );

    // keep the original value
    const BOOL fSyncronousTasks = m_fSyncronousTasks;

    //  override the default if we are cleaning one database
    m_fSyncronousTasks = fCleanOneDb ? fTrue : m_fSyncronousTasks;

#ifdef DEBUG
    Ptls()->fIsRCECleanup = fTrue;
#endif  //  DEBUG

#ifdef VERPERF
    //  UNDONE: why do these get reset every RCEClean??
    m_cbucketCleaned    = 0;
    m_cbucketSeen       = 0;
    m_crceSeen          = 0;
    m_crceCleaned       = 0;
    HRT hrtStartHrts    = HrtHRTCount();
    m_crceFlagDelete    = 0;
    m_crceDeleteLV      = 0;
#endif  //  VERPERF

    ERR         err     = JET_errSuccess;
    BUCKET *    pbucket;

    //  get oldest bucket and clean RCEs from oldest to youngest

    m_critBucketGlobal.Enter();

    pbucket = PbucketVERIGetOldest();

    m_critBucketGlobal.Leave();

    TRX trxOldest = TrxOldest( m_pinst );
    //  loop through buckets, oldest to newest. stop when we run out of buckets
    //  or find an uncleanable RCE

    while ( pbucketNil != pbucket )
    {
#ifdef VERPERF
        INT crceInBucketSeen    = 0;
        INT crceInBucketCleaned = 0;
#endif  //  VERPERF

        //  Check if need to get RCE within m_critBucketGlobal

        BOOL    fNeedBeInCritBucketGlobal   = ( pbucket->hdr.pbucketNext == pbucketNil );

        //  Only clean can change prceOldest and only one clean thread is active.

        Assert( m_critRCEClean.FOwner() );
        RCE *   prce                        = pbucket->hdr.prceOldest;
        BOOL    fSkippedRCEInBucket         = fFalse;

        forever
        {
#ifdef VERPERF
            ++m_crceSeen;
            ++crceInBucketSeen;
#endif

            //  verify RCE is within the bucket or we are at the end of the bucket
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

            //  break to release the bucket
            if ( pbucket->hdr.prceNextNew == prce )
                break;

            if ( fNeedBeInCritBucketGlobal )
                m_critBucketGlobal.Leave();

            //  Save the size for use later
            const INT   cbRce = prce->CbRce();

            //  verify RCE is within the bucket
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
                    //  trxOldest may no longer be trxMax. if so we may not be able to
                    //  clean up this RCE after all. we retrieve the trxCommitted of the rce
                    //  first to avoid a race condition
                    trxOldest = TrxOldest( m_pinst );
                }

                //  RCE's for the temp db are normally cleaned up at commit
                //  time (see VERICommitTransactionToLevel0()), so if we hit
                //  a fully-committed non-nullified RCE for the temp db at
                //  this point, it means we hit the small timing window where
                //  VERICommitTransactionToLevel0() has set the trxCommitted
                //  in the RCE (so the RCE is fully committed) but hasn't
                //  yet nullified the RCE, so we need to wait until the RCE
                //  is nullified by the committing session before proceeding
                //  any further
                //
                if ( fFullyCommitted && !FFMPIsTempDB( prce->Ifmp() ) )
                {
                    Assert( trxMax != trxRCECommitted );
                    if ( TrxCmp( trxRCECommitted, trxOldest ) < 0 )
                    {
                        fCleanable = fTrue;
                    }
                    else if ( trxMax != trxOldest )
                    {
                        //  refresh trxOldest to see if it's been
                        //  updated, which might now make this RCE
                        //  cleanable
                        //
                        trxOldest = TrxOldest( m_pinst );
                        if ( TrxCmp( trxRCECommitted, trxOldest ) < 0 )
                        {
                            fCleanable = fTrue;
                        }
                    }
                    else
                    {
                        //  should be impossible, because if trxOldest was
                        //  trxMax, then the RCE should have been cleanable
                        //  (since trxRCECommitted can't be trxMax if we
                        //  reached this code path)
                        //
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
                                //  this can be caused by a task that is active
                                //  stop here as we have cleaned up what we can
                                err = ErrERRCheck( JET_wrnRemainingVersions );
                                goto HandleError;
                            }
                            else
                            {
                                // Fall through and clean the RCE.
                            }
                        }
                        else
                        {
                            // Skip uncleanable RCE's that don't belong to
                            // the db we're trying to clean.
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
#endif  //  VERPERF

                Call( prce->ErrPrepareToDeallocate( trxOldest ) );

#ifdef VERPERF
                ++crceInBucketCleaned;
                ++m_crceCleaned;
#endif  //  VERPERF
            }

            //  Set the oldest to next prce entry in the bucket
            //  Rce clean thread ( run within m_critRCEClean ) is
            //  the only one touch prceOldest
NextRCE:
            Assert( m_critRCEClean.FOwner() );


            const BYTE  *pbRce      = reinterpret_cast<BYTE *>( prce );
            const BYTE  *pbNextRce  = reinterpret_cast<BYTE *>( PvAlignForThisPlatform( pbRce + cbRce ) );

            prce = (RCE *)pbNextRce;

            //  verify RCE is within the bucket or we are at the end of the bucket
            Assert( pbucket->rgb <= (BYTE*)prce );
            Assert( (BYTE*)prce < (BYTE*)pbucket + m_cbBucket ||
                    (   (BYTE*)prce == (BYTE*)pbucket + m_cbBucket &&
                        prce == pbucket->hdr.prceNextNew ) );

            Assert( pbucket->hdr.prceOldest <= pbucket->hdr.prceNextNew );
        }

        //  all RCEs in bucket cleaned.  Now get next bucket and free
        //  cleaned bucket.

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

    //  stop as soon as find RCE commit time younger than oldest
    //  transaction.  If bucket left then set ibOldestRCE and
    //  unlink back offset of last remaining RCE.
    //  If no error then set warning code if some buckets could
    //  not be cleaned.

    Assert( pbucketNil == pbucket );
    err = JET_errSuccess;

    // If only cleaning one db, we don't clean buckets because
    // there may be outstanding versions on other databases.
    if ( !fCleanOneDb )
    {
        m_critBucketGlobal.Enter();
        if ( pbucketNil != m_pbucketGlobalHead )
        {
            //  return warning if remaining versions
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
    // dispatch any batched tasks. this should be done even if there is an error
    // as we don't want the tasks created before the error to hang around indefinitely
    const ERR errT = m_rectaskbatcher.ErrPostAllPending();
    // but, we don't want to overwrite warnings with success
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
#endif  //  DEBUG_VER_EXPENSIVE

#ifdef DEBUG
    Ptls()->fIsRCECleanup = fFalse;
#endif  //  DEBUG

    // restore the original value
    m_fSyncronousTasks = fSyncronousTasks;

    if ( !fCleanOneDb )
    {
        //  record when we performed this pass of version cleanup
        //
        m_tickLastRCEClean = TickOSTimeCurrent();
    }

    return err;
}


//  ================================================================
VOID VERICommitRegisterCallback( const RCE * const prce, const TRX trxCommit0 )
//  ================================================================
//
//  Set the trxRegisterCommit0 in the CBDESC
//
//-
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
#endif  //  VERSIONED_CALLBACKS
}


//  ================================================================
VOID VERICommitUnregisterCallback( const RCE * const prce, const TRX trxCommit0 )
//  ================================================================
//
//  Set the trxUnregisterCommit0 in the CBDESC
//
//-
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
#endif  //  VERSIONED_CALLBACKS
}


//  ================================================================
LOCAL VOID VERICommitTransactionToLevelGreaterThan0( const PIB * const ppib )
//  ================================================================
{
    const LEVEL level   = ppib->Level();
    Assert( level > 1 );

    //  we do not need to lock the RCEs as other transactions do not care about the level,
    //  only that they are uncommitted

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

//  ================================================================
LOCAL VOID VERICommitTransactionToLevel0( PIB * const ppib )
//  ================================================================
{
    RCE         * prce              = ppib->prceNewest;

    Assert( 1 == ppib->Level() );

    //  because of some optimizations at the DIR/LOG level we cannot always assert this
/// Assert( TrxCmp( ppib->trxCommit0, ppib->trxBegin0 ) > 0 );

    RCE * prceNextToCommit;
#ifdef DEBUG
    prceNextToCommit = prceInvalid;
#endif  //  DEBUG
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
            //  RCEs for temp tables are used only for rollback.
            //   - The table is not shared so there is no risk of write conflicts
            //   - The table is not recovered so no undo-info is needed
            //   - No committed RCEs exist on temp tables so RCEClean will never access the FCB
            //   - No concurrent-create-index on temp tables
            //  Thus, we simply nullify the RCE so that the FCB can be freed
            //
            //  UNDONE: RCEs for temp tables should not be inserted into the FCB list or hash table

            // DDL currently not supported on temp tables.
            Assert( !prce->FOperNull() );
            Assert( !prce->FOperDDL() || operCreateTable == prce->Oper() || operCreateLV == prce->Oper() );
            Assert( !PinstFromPpib( ppib )->m_plog->FRecovering() );
            Assert( !prce->FFullyCommitted() );
            Assert( prce->PgnoUndoInfo() == pgnoNull ); //  no logging on temp tables

            const BOOL fInHash = prce->FOperInHashTable();

            ENTERREADERWRITERLOCK maybeEnterRwlHashAsWriter( fInHash ? &( PverFromPpib( ppib )->RwlRCEChain( prce->UiHash() ) ) : 0, fFalse, fInHash );
            ENTERCRITICALSECTION enterCritFCBRCEList( &prce->Pfcb()->CritRCEList() );

            VERIDeleteRCEFromSessionList( ppib, prce );

            prce->SetLevel( 0 );
            prce->SetTrxCommitted( ppib->trxCommit0 );

            //
            //  WARNING: now that the RCE is fully committed,
            //  there's a small chance version cleanup will
            //  make it to this RCE before we've had a chance
            //  to nullify it (via the call to NullifyOper() a
            //  half-dozen lines below), so I added a check in
            //  VER::ErrVERIRCEClean() to ensure that it doesn't
            //  attempt to call RCE::ErrPrepareToDeallocate()
            //  for temp. db RCE's, since they should always
            //  be nullified here
            //

            VERIDeleteRCEFromFCBList( prce->Pfcb(), prce );
            if ( fInHash )
            {
                VERIDeleteRCEFromHash( prce );
            }
            prce->NullifyOper();

            continue;
        }

        //  Remove UndoInfo dependency if committing to level 0

        if ( prce->PgnoUndoInfo() != pgnoNull )
            BFRemoveUndoInfo( prce );

        ENTERREADERWRITERLOCK maybeEnterRwlHashAsWriter(
            prce->FOperInHashTable() ? &( PverFromPpib( ppib )->RwlRCEChain( prce->UiHash() ) ) : 0,
            fFalse,
            prce->FOperInHashTable()
            );

        //  if version for DDL operation then reset deny DDL
        //  and perform special handling
        if ( prce->FOperDDL() )
        {
            switch( prce->Oper() )
            {
                case operAddColumn:
                    // RCE list ensures FCB is still pinned
                    Assert( prce->Pfcb()->PrceOldest() != prceNil );
                    Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
                    break;

                case operDeleteColumn:
                    // RCE list ensures FCB is still pinned
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

                        // For primary index, must reset VersionedCreate()
                        // flag at commit time so updates can occur
                        // immediately once the primary index has been
                        // committed (see ErrSetUpdatingAndEnterDML()).
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
                    //  no further action is required
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

                    //  free in-memory structure

                    Assert( pfcbIndex->Pidb() != pidbNil );
                    Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
                    Assert( pfcbIndex->Pidb()->FDeleted() );
                    pfcbIndex->Pidb()->ResetFVersioned();

                    pfcbIndex->Lock();
                    pfcbIndex->SetDeleteCommitted();
                    pfcbIndex->Unlock();

                    //  update all index mask
                    FILESetAllIndexMask( pfcbTable );

                    pfcbTable->LeaveDDL();
                    break;
                }

                case operDeleteTable:
                {
                    FCB     *pfcbTable;
                    INT     fState;

                    //  pfcb should be found, even if it's a sentinel
                    pfcbTable = FCB::PfcbFCBGet( prce->Ifmp(), *(PGNO*)prce->PbData(), &fState );
                    Assert( pfcbTable != pfcbNil );
                    Assert( pfcbTable->FTypeTable() );
                    Assert( fFCBStateInitialized == fState );

                    if ( pfcbTable->Ptdb() != ptdbNil )
                    {
                        Assert( fFCBStateInitialized == fState );

                        pfcbTable->EnterDDL();

                        // Nothing left to prevent access to this FCB except
                        // for DeletePending.
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

                        // If regular FCB, decrement refcnt
                        pfcbTable->Release();
                    }
                    else
                    {
                        FireWall( "DeprecatedSentinelFcbVerCommit0" ); // Sentinel FCBs are believed deprecated
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
            //  the deferred before image chain of the prce should have been
            //  cleaned up in the beginning of this while block
            const PGNO  pgnoUndoInfo = prce->PgnoUndoInfo();
            Assert( pgnoNull == pgnoUndoInfo );
        }
#endif  //  DEBUG

        //  set level and trxCommitted
        VERICommitOneRCEToLevel0( ppib, prce );
    }   //  WHILE

    Assert( prceNil == ppib->prceNewest );

    //  UNDONE: prceNewest should already be prceNil
    Assert( prceNil == ppib->prceNewest );
    PIBSetPrceNewest( ppib, prceNil );
}


//  ================================================================
VOID VERCommitTransaction( PIB * const ppib )
//  ================================================================
//
//  OPTIMIZATION:   combine delta/readLock/replace versions
//                  remove redundant replace versions
//
//-
{
    ASSERT_VALID( ppib );

    const LEVEL             level = ppib->Level();

    //  must be in a transaction in order to commit
    Assert( level > 0 );
    Assert( PinstFromPpib( ppib )->m_plog->FRecovering() || trxMax != TrxOldest( PinstFromPpib( ppib ) ) );
    Assert( PinstFromPpib( ppib )->m_plog->FRecovering() || TrxCmp( ppib->trxBegin0, TrxOldest( PinstFromPpib( ppib ) ) ) >= 0 );

    //  handle commit to intermediate transaction level and
    //  commit to transaction level 0 differently.
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


//  ================================================================
LOCAL ERR ErrVERILogUndoInfo( RCE *prce, CSR* pcsr )
//  ================================================================
//
//  log undo information [if not in redo phase]
//  remove rce from before-image chain
//
{
    ERR     err             = JET_errSuccess;
    LGPOS   lgpos           = lgposMin;
    LOG     * const plog    = PinstFromIfmp( prce->Ifmp() )->m_plog;

    Assert( pcsr->FLatched() );

    if ( plog->FRecoveringMode() != fRecoveringRedo )
    {
        CallR( ErrLGUndoInfo( prce, &lgpos ) );
    }

    //  remove RCE from deferred BI chain
    //
    BFRemoveUndoInfo( prce, lgpos );

    return err;
}



//  ================================================================
ERR ErrVERIUndoReplacePhysical( RCE * const prce, CSR *pcsr, const BOOKMARK& bm )
//  ================================================================
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

    //  dirty page and log operation
    //
    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

    //  replace should not fail since splits are avoided at undo
    //  time via deferred page space release.  This refers to space
    //  within a page and not pages freed when indexes and tables
    //  are deleted
    //

    data.SetPv( prce->PbData() + cbReplaceRCEOverhead );
    data.SetCb( prce->CbData() - cbReplaceRCEOverhead );

    const VERREPLACE* const pverreplace = (VERREPLACE*)prce->PbData();
    const INT cbDelta = pverreplace->cbDelta;

    //  if we are recovering we don't need to track the cbUncommitted free
    //  (the version store will be empty at the end of recovery)
    if ( cbDelta > 0 )
    {
        //  Rolling back replace that shrunk the node.  To satisfy the rollback,
        //  we will consume the reserved node space, but first we must remove this
        //  reserved node space from the uncommitted freed count so that BTReplace()
        //  can see it.
        //  (This complements the call to AddUncommittedFreed() in SetCbAdjust()).
        pcsr->Cpage().ReclaimUncommittedFreed( cbDelta );
    }

    //  ND expects that fucb will contain bookmark of replaced node
    //
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
        // Rolling back a replace that grew the node.  Add to uncommitted freed
        // count the amount of reserved node space, if any, that we must restore.
        // (This complements the call to ReclaimUncommittedFreed in SetCbAdjust()).
        pcsr->Cpage().AddUncommittedFreed( -cbDelta );
    }

    return err;
}


//  ================================================================
ERR ErrVERIUndoInsertPhysical( RCE * const prce, CSR *pcsr, PIB * ppib = NULL, BOOL *pfRolledBack = NULL )
//  ================================================================
//
//  set delete bit in node header and let RCE clean up
//  remove the node later
//
//-
{
    ERR     err;
    FUCB    * const pfucb = prce->Pfucb();

    Assert( pgnoNull == prce->PgnoUndoInfo() );

    //  dirty page and log operation
    //
    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

    CallS( ErrNDFlagDelete( pfucb, pcsr, fDIRUndo, rceidNull, NULL ) );

    if ( ( !PinstFromIfmp( prce->Ifmp() )->FRecovering()
    /* || fRecoveringUndo == PinstFromIfmp( ifmp )->m_plog->FRecoveringMode() */ ) &&
         !FFMPIsTempDB( prce->Ifmp() ) &&
         !prce->FFutureVersionsOfNode() )
    {
        Assert( ppib != NULL );
        Assert( pfRolledBack != NULL );

        DELETERECTASK * ptask;

        const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

#ifdef DEBUG
        Ptls()->fIsRCECleanup = fTrue;
#endif  //  DEBUG

        err = prce->ErrGetTaskForDelete( (VOID **)&ptask );
        if ( err >= JET_errSuccess && ptask != NULL )
        {
            //  must nullify RCE so that ErrBTDelete does not see an active version on this node
            //  and can actually reclaim space.
            VERINullifyRolledBackRCE( ppib, prce );
            *pfRolledBack = fTrue;

            // Release all latches before calling cleanup
            BTUp( pfucb );

            // This BT may get marked for deletion between creating the task above and executing it, but that is
            // all right because the cleanup of deletion still has to wait for this transaction to complete.
            PverFromPpib( ppib )->IncrementCSyncCleanupDispatched();
            TASK::DispatchGP( ptask );
        }

#ifdef DEBUG
        Ptls()->fIsRCECleanup = fFalse;
#endif  //  DEBUG

        // Restore cleanup checking
        FOSSetCleanupState( fCleanUpStateSavedSavedSaved );
    }

    return err;
}


//  ================================================================
LOCAL ERR ErrVERIUndoFlagDeletePhysical( RCE * prce, CSR *pcsr )
//  ================================================================
//
//  reset delete bit
//
//-
{
    ERR     err;
#ifdef DEBUG
    FUCB    * const pfucb   = prce->Pfucb();

    Unused( pfucb );
#endif  //  DEBUG

    if ( prce->PgnoUndoInfo() != pgnoNull )
    {
        CallR( ErrVERILogUndoInfo( prce, pcsr ) );
    }

    //  dirty page and log operation
    //
    CallR( ErrLGUndo( prce, pcsr, fMustDirtyCSR ) );

    NDResetFlagDelete( pcsr );
    return err;
}


//  ================================================================
template< typename TDelta >
ERR ErrVERIUndoDeltaPhysical( RCE * const prce, CSR *pcsr )
//  ================================================================
//
//  undo delta change. modifies the RCE by setting the lDelta to 0
//
//-
{
    ERR     err;
    FUCB    * const pfucb = prce->Pfucb();
    LOG     *plog = PinstFromIfmp( prce->Ifmp() )->m_plog;

    _VERDELTA<TDelta>* const    pverdelta   = (_VERDELTA<TDelta>*)prce->PbData();
    const TDelta                tDelta      = -pverdelta->tDelta;

    Assert( pgnoNull == prce->PgnoUndoInfo() );

    //  NDDelta is dependant on the data that it is operating on. for this reason we use
    //  NDGet to get the real data from the database (DIRGet will get the versioned copy)
/// AssertNDGet( pfucb, pcsr );
/// NDGet( pfucb, pcsr );

    if ( pverdelta->tDelta < 0 && !plog->FRecovering() )
    {
        ENTERREADERWRITERLOCK enterRwlHashAsWriter( &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ), fFalse );

        //  we are rolling back a decrement. we need to remove all the deferredDelete flags
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

    //  dirty page and log operation
    //
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
        //  by undoing an increment delta we have reduced the refcount of a LV to zero
        //  UNDONE:  morph the RCE into a committed level 0 decrement with deferred delete
        //           the RCE must be removed from the list of RCE's on the pib
    }

    //  in order that the compensating delta is calculated properly, set the delta value to 0
    pverdelta->tDelta = 0;

    return err;
}


//  ================================================================
VOID VERRedoPhysicalUndo( INST *pinst, const LRUNDO *plrundo, FUCB *pfucb, CSR *pcsr, BOOL fRedoNeeded )
//  ================================================================
//  retrieve RCE to be undone
//  call corresponding physical undo
//
{
    //  get RCE for operation
    //
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
            //  UNDONE: use rceid instead of level and procid
            //          to identify RCE
            //
            Assert( prce->Pfucb() == pfucb );
            Assert( prce->Pfucb()->ppib == pfucb->ppib );
            Assert( prce->Oper() == plrundo->le_oper );
            Assert( prce->TrxCommitted() == trxMax );

            //  UNDONE: the following assert will fire if
            //  the original node operation was created by proxy
            //  (ie. concurrent create index).
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


//  ================================================================
LOCAL ERR ErrVERITryUndoLoggedOper( PIB *ppib, RCE * const prce )
//  ================================================================
//  seek to bookmark, upgrade latch
//  call corresponding physical undo
//
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

    //  logged opers are undone by their own individual
    //  Undo log records, so by the time we get around to redoing
    //  the Rollback log record, there shouldn't be any more
    //  logged opers to undo
    //
    // - except -
    //
    //  If concurrent create-index rolls back, then any RCEs that other sessions 
    //  created for the index will be nullified out from underneath the session. This 
    //  works fine so long as the session doesn't ultimately roll back. If the session 
    //  rolls back, then because the RCE was nullified, no Undo record is generated.  
    //  This is fine, until you try to replay the whole thing on recovery. Recovery 
    //  will recreate the RCE, but because CreateIndex is not logged, recovery does not 
    //  replay the nullification of the RCE when the index rolls back. So when the 
    //  session rolls back, the RCE is outstanding. To deal with this we simply ignore
    //  the RCE during recovery redo.
    //
    if ( fRecoveringRedo == PinstFromPpib( ppib )->m_plog->FRecoveringMode() )
    {
        VERINullifyRolledBackRCE( ppib, prce );
        return JET_errSuccess;
    }
    
    prce->GetBookmark( &bm );

    //  reset index range on this cursor
    //  we may be using a deferred-closed cursor or a cursor that
    //  had an index-range on it before the rollback
    DIRResetIndexRange( pfucb );

    //  save off cursor's current bookmark, set
    //  to bookmark of operation to be rolled back,
    //  then release any latches to force re-seek
    //  to bookmark
    bmSave = pfucb->bmCurr;
    pfucb->bmCurr = bm;

    BTUp( pfucb );

Refresh:
    Call( ErrFaultInjection( 58139 ) );
    err = ErrBTIRefresh( pfucb, latch );
    Assert( JET_errRecordDeleted != err );
    Call( err );

    //  upgrade latch on page
    //
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
        //  logged operations
        //
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
        //  we have successfully undone the operation
        //  must now set RolledBack flag before releasing page latch
        Assert( !prce->FOperNull() );
        Assert( !prce->FRolledBack() );

        //  must nullify RCE while page is still latched, to avoid inconsistency
        //  between what's on the page and what's in the version store
        VERINullifyRolledBackRCE( ppib, prce );
    }

    //  re-instate original bookmark
    pfucb->bmCurr = bmSave;
    BTUp( pfucb );

    CallS( err );
    return JET_errSuccess;

HandleError:
    //  re-instate original bookmark
    pfucb->bmCurr = bmSave;
    BTUp( pfucb );

    Assert( err < JET_errSuccess );
    Assert( JET_errDiskFull != err );

    return err;
}


//  ================================================================
LOCAL ERR ErrVERIUndoLoggedOper( PIB *ppib, RCE * const prce )
//  ================================================================
//  seek to bookmark, upgrade latch
//  call corresponding physical undo
//
{
    ERR             err     = JET_errSuccess;
    FUCB    * const pfucb   = prce->Pfucb();

    Assert( pfucbNil != pfucb );
    Assert( ppib == pfucb->ppib );
    Call( ErrVERITryUndoLoggedOper( ppib, prce ) );

    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );

    //  if rollback fails due to an error, then we will disable
    //  logging in order to force the system down (we are in
    //  an uncontinuable state).  Recover to restart the database.

    //  We should never fail to rollback due to disk full

    Assert( JET_errDiskFull != err );

    switch ( err )
    {
        
        // failures possibly due to lost flushes on a specific database

        case JET_errBadPageLink:
        case JET_errDatabaseCorrupted:
        case JET_errDbTimeCorrupted:
        case JET_errRecordDeleted:
        case JET_errLVCorrupted:
        case JET_errBadParentPageLink:
        case JET_errPageNotInitialized:
        
    
        //  by adding errors to this list, we're effectively downgrading from Enforce to AssertRTL for these cases. 
        //  with RTM, the application will see JET_errRollbackError and the instance will go unavailable. 
        //  this will be a clear signal to the application that it needs to rebuild or restore from
        //  a backup
        
        AssertSzRTL( false, "Unexpected error condition blocking Rollback. Error = %d, oper = %d", err, prce->Oper() );

        //  intentional fall-through to next block

        //  failure with impact on the database only

        case JET_errFileAccessDenied:
        case JET_errDiskIO:
        case_AllDatabaseStorageCorruptionErrs:
        case JET_errOutOfBuffers:
        case JET_errOutOfMemory :       // BF may hit OOM when trying to latch the page
        case JET_errCheckpointDepthTooDeep:

        //  failure with impact on the instance

        case JET_errLogWriteFail:
        case JET_errLogDiskFull:
            {

                //  rollback failed -- log an event message

                INST * const    pinst       = PinstFromPpib( ppib );
                LOG * const     plog        = pinst->m_plog;
                WCHAR           wszRCEID[16];
                WCHAR           wszERROR[16];

                ERR errLog;
                if ( JET_errLogWriteFail == err
                    && plog->FNoMoreLogWrite( &errLog )
                    && JET_errLogDiskFull == errLog )
                {
                    //  special-case: trap JET_errLogDiskFull
                    //
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

                //  REVIEW: (SOMEONE) how can we get a logging failure if FLogOn() is
                //  not set?
                if ( g_rgfmp[pfucb->ifmp].FLogOn() )
                {
                    Assert( plog );
                    Assert( !plog->FLogDisabled() );

                    //  flush and halt the log

                    (void)ErrLGWrite( ppib );
                    UtilSleep( cmsecWaitLogWrite );
                    plog->SetNoMoreLogWrite( err );
                }


                //  There may be an older version of this page which needs
                //  the undo-info from this RCE. Take down the instance and
                //  error out
                //
                //  (Yes, this could be optimized to only do this for RCEs
                //  with before-images, but why complicate the code to optimize
                //  a failure case)

                Assert( !prce->FOperNull() );
                Assert( !prce->FRolledBack() );

                // Do not bring down the whole instance because we have deliberately stopped logging
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
            //  error is non-fatal, so caller will attempt to redo rollback
            break;
    }

    return err;
}


//  ================================================================
INLINE VOID VERINullifyForUndoCreateTable( PIB * const ppib, FCB * const pfcb )
//  ================================================================
//
//  This is used to nullify all RCEs on table FCB because CreateTable
//  was rolled back.
//
//-
{
    Assert( pfcb->FTypeTable()
        || pfcb->FTypeTemporaryTable() );

    // Because rollback is done in two phases, the RCE's on
    // this FCB are still outstanding -- they've already
    // been processed for rollback, they just need to be
    // nullified.
    while ( prceNil != pfcb->PrceNewest() )
    {
        RCE * const prce = pfcb->PrceNewest();

        Assert( prce->Pfcb() == pfcb );
        Assert( !prce->FOperNull() );

        // Since we're rolling back CreateTable, all operations
        // on the table must also be uncommitted and rolled back
        Assert( prce->TrxCommitted() == trxMax );

        if ( !prce->FRolledBack() )
        {
            // The CreateTable RCE itself should be the only
            // RCE on this FCB that is not yet marked as
            // rolled back, because that's what we're in the
            // midst of doing
            Assert( prce->Oper() == operCreateTable );
            Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
            Assert( pfcb->PrceNewest() == prce );
            Assert( pfcb->PrceOldest() == prce );
        }

        Assert( prce->Pfucb() != pfucbNil );
        Assert( ppib == prce->Pfucb()->ppib );  // only one session should have access to the table

        ENTERREADERWRITERLOCK   maybeEnterRwlHashAsWriter(
                                    prce->FOperInHashTable() ? &( PverFromIfmp( prce->Ifmp() )->RwlRCEChain( prce->UiHash() ) ) : NULL,
                                    fFalse,
                                    prce->FOperInHashTable() );
        ENTERCRITICALSECTION    enterCritFCBRCEList( &( pfcb->CritRCEList() ) );
        VERINullifyUncommittedRCE( prce );
    }

    // should be no more versions on the FCB
    Assert( pfcb->PrceOldest() == prceNil );
    Assert( pfcb->PrceNewest() == prceNil );
}

//  ================================================================
INLINE VOID VERICleanupForUndoCreateTable( RCE * const prceCreateTable )
//  ================================================================
//
//  This is used to cleanup RCEs and deferred-closed cursors
//  on table FCB because CreateTable was rolled back.
//
//-
{
    FCB * const pfcbTable = prceCreateTable->Pfcb();

    Assert( pfcbTable->FInitialized() );
    Assert( pfcbTable->FTypeTable() || pfcbTable->FTypeTemporaryTable() );
    Assert( pfcbTable->FPrimaryIndex() );

    // Last RCE left should be the CreateTable RCE (ie. this RCE).
    Assert( operCreateTable == prceCreateTable->Oper() );
    Assert( pfcbTable->PrceOldest() == prceCreateTable );

    Assert( prceCreateTable->Pfucb() != pfucbNil );
    PIB * const ppib    = prceCreateTable->Pfucb()->ppib;
    Assert( ppibNil != ppib );

    Assert( pfcbTable->Ptdb() != ptdbNil );
    FCB * pfcbT         = pfcbTable->Ptdb()->PfcbLV();

    // force-close any deferred closed cursors
    if ( pfcbNil != pfcbT )
    {
        Assert( pfcbT->FTypeLV() );

        //  all RCE's on the table's LV should have already been rolled back AND nullified
        //  (since we don't suppress nullification of LV RCE's during concurrent create-index
        //  like we do for Table RCE's)
        Assert( prceNil == pfcbT->PrceNewest() );
//      VERINullifyForUndoCreateTable( ppib, pfcbT );

        FUCBCloseAllCursorsOnFCB( ppib, pfcbT );
    }

    for ( pfcbT = pfcbTable->PfcbNextIndex(); pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
    {
        Assert( pfcbT->FTypeSecondaryIndex() );

        //  all RCE's on the table's indexes should have already been rolled back AND nullified
        //  (since we don't suppress nullification of Index RCE's during concurrent create-index
        //  like we do for Table RCE's)
        Assert( prceNil == pfcbT->PrceNewest() );
//      VERINullifyForUndoCreateTable( ppib, pfcbT );

        FUCBCloseAllCursorsOnFCB( ppib, pfcbT );
    }

    //  there may be rolled-back versions on the table that couldn't be nullified
    //  because concurrent create-index was in progress on another table (we
    //  suppress nullification of certain table RCE's if a CCI is in progress
    //  anywhere)
    VERINullifyForUndoCreateTable( ppib, pfcbTable );
    FUCBCloseAllCursorsOnFCB( ppib, pfcbTable );
}

//  ================================================================
LOCAL VOID VERIUndoCreateTable( PIB * const ppib, RCE * const prce )
//  ================================================================
//
//  Takes a non-const RCE as VersionDecrement sets the pfcb to NULL
//
//-
{
    FCB     * const pfcb    = prce->Pfcb();

    ASSERT_VALID( ppib );
    Assert( prce->Oper() == operCreateTable );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( pfcb->FInitialized() );
    Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
    Assert( pfcb->FPrimaryIndex() );

    // Need to leave critTrx to wait for tasks to complete
    ppib->CritTrx().Leave();

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

    ppib->CritTrx().Enter();

    pfcb->Lock();
    //  set FDeleteCommitted flag to silence asserts in ErrSPFreeFDP()
    //
    pfcb->SetDeleteCommitted();
    pfcb->Unlock();

    // close all cursors on this table
    forever
    {
        FUCB *pfucb = pfucbNil;

        pfcb->Lock();
        pfcb->FucbList().LockForEnumeration();
        // find the first element in FucbList() that is NOT defer-closed.
        // This is an N^2 algorithm because we release the lock on
        // pfcb to deal with the pfucb we find.  I'm not sure that's
        // necessary.
        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++)
        {
            pfucb = pfcb->FucbList()[ ifucbList ];

            ASSERT_VALID( pfucb );

            // Since CreateTable is uncommitted, we should be the
            // only one who could have opened cursors on it.
            Assert( pfucb->ppib == ppib );

            //  if defer closed then continue
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
            // There are no non-defer-closed FUCB* left.  This is the way out of forever.  
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

    //  prepare the FCB to be purged
    //  this removes the FCB from the hash-table among other things
    //      so that the following case cannot happen:
    //          we free the space for this FCB
    //          someone else allocates it
    //          someone else BTOpen's the space
    //          we try to purge the table and find that the refcnt
    //              is not zero and the state of the FCB says it is
    //              currently in use!
    //          result --> CONCURRENCY HOLE
    pfcb->PrepareForPurge();
    const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

    //  ignore errors if we can't free the space (it will be leaked)
    //
    (VOID)ErrSPFreeFDP( ppib, pfcb, pgnoSystemRoot, fTrue );

    //  Restore cleanup checking
    //
    FOSSetCleanupState( fCleanUpStateSavedSavedSaved );

    pfcb->Purge();
}


//  ================================================================
LOCAL VOID VERIUndoAddColumn( const RCE * const prce )
//  ================================================================
{
    Assert( prce->Oper() == operAddColumn );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( prce->CbData() == sizeof(VERADDCOLUMN) );
    Assert( prce->Pfcb() == prce->Pfucb()->u.pfcb );

    FCB         * const pfcbTable   = prce->Pfcb();

    pfcbTable->EnterDDL();

    // RCE list ensures FCB is still pinned
    Assert( pfcbTable->PrceOldest() != prceNil );

    TDB             * const ptdb        = pfcbTable->Ptdb();
    const COLUMNID  columnid            = ( (VERADDCOLUMN*)prce->PbData() )->columnid;
    BYTE            * pbOldDefaultRec   = ( (VERADDCOLUMN*)prce->PbData() )->pbOldDefaultRec;
    FIELD           * const pfield      = ptdb->Pfield( columnid );

    Assert( ( FCOLUMNIDTagged( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() )
            || ( FCOLUMNIDVar( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidVarLast() )
            || ( FCOLUMNIDFixed( columnid ) && FidOfColumnid( columnid ) <= ptdb->FidFixedLast() ) );

    //  rollback the added column by marking it as deleted
    Assert( !FFIELDDeleted( pfield->ffield ) );
    FIELDSetDeleted( pfield->ffield );

    FIELDResetVersioned( pfield->ffield );
    FIELDResetVersionedAdd( pfield->ffield );

    //  rollback version and autoinc fields, if set.
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

    //  itag 0 in the TDB is reserved for the FIELD structures.  We
    //  cannibalise it for itags of field names to indicate that a name
    //  has not been added to the buffer.
    if ( 0 != pfield->itagFieldName )
    {
        //  remove the column name from the TDB name space
        ptdb->MemPool().DeleteEntry( pfield->itagFieldName );
    }

    //  UNDONE: remove the CBDESC for this from the TDB
    //  the columnid will not be re-used so the callback
    //  will not be called. The CBDESC will be freed when
    //  the table is closed so no memory will be lost
    //  Its just not pretty though...

    //  if we modified the default record, the changes will be invisible,
    //  since the field is now flagged as deleted (unversioned).  The
    //  space will be reclaimed by a subsequent call to AddColumn.

    //  if there was an old default record and we were successful in
    //  creating a new default record for the TDB, must get rid of
    //  the old default record.  However, we can't just free the
    //  memory because other threads could have stale pointers.
    //  So build a list hanging off the table FCB and free the
    //  memory when the table FCB is freed.
    if ( NULL != pbOldDefaultRec
        && (BYTE *)ptdb->PdataDefaultRecord() != pbOldDefaultRec )
    {
        //  user-defined defaults are not stored in the default
        //  record (ie. this AddColumn would not have caused
        //  us to rebuild the default record)
        Assert( !FFIELDUserDefinedDefault( pfield->ffield ) );

        for ( RECDANGLING * precdangling = pfcbTable->Precdangling();
            ;
            precdangling = precdangling->precdanglingNext )
        {
            if ( NULL == precdangling )
            {
                //  not in list, so add it;
                //  assumes that the memory pointed to by pmemdangling is always at
                //  least sizeof(ULONG_PTR) bytes
                //
                Assert( NULL == ( (RECDANGLING *)pbOldDefaultRec )->precdanglingNext );
                ( (RECDANGLING *)pbOldDefaultRec )->precdanglingNext = pfcbTable->Precdangling();
                pfcbTable->SetPrecdangling( (RECDANGLING *)pbOldDefaultRec );
                break;
            }
            else if ( (BYTE *)precdangling == pbOldDefaultRec )
            {
                //  pointer is already in the list, just get out
                //
                break;
            }
        }
    }

    pfcbTable->LeaveDDL();
}


//  ================================================================
LOCAL VOID VERIUndoDeleteColumn( const RCE * const prce )
//  ================================================================
{
    Assert( prce->Oper() == operDeleteColumn );
    Assert( prce->TrxCommitted() == trxMax );

    Assert( prce->CbData() == sizeof(COLUMNID) );
    Assert( prce->Pfcb() == prce->Pfucb()->u.pfcb );

    FCB         * const pfcbTable   = prce->Pfcb();

    pfcbTable->EnterDDL();

    // RCE list ensures FCB is still pinned
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
        // UNDONE: Instead of the VersionedAdd flag, scan the version store
        // for other outstanding versions on this column (should either be
        // none or a single AddColumn version).
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


//  ================================================================
LOCAL VOID VERIUndoDeleteTable( const RCE * const prce )
//  ================================================================
{
    if ( FFMPIsTempDB( prce->Ifmp() ) )
    {
        // DeleteTable (ie. CloseTable) doesn't get rolled back for temp. tables.
        return;
    }

    Assert( prce->Oper() == operDeleteTable );
    Assert( prce->TrxCommitted() == trxMax );

    //  may be pfcbNil if sentinel
    INT fState;
    FCB * const pfcbTable = FCB::PfcbFCBGet( prce->Ifmp(), *(PGNO*)prce->PbData(), &fState );
    Assert( pfcbTable != pfcbNil );
    Assert( pfcbTable->FTypeTable() );
    Assert( fFCBStateInitialized == fState );

    // If regular FCB, decrement refcnt, else free sentinel.
    if ( pfcbTable->Ptdb() != ptdbNil )
    {
        Assert( fFCBStateInitialized == fState );

        pfcbTable->EnterDDL();

        for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( pfcbT->FDeletePending() );

            //Reset the DeletePending flag, unless the index was deleted in a previous transaction
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
        FireWall( "DeprecatedSentinelFcbUndoDelTable" ); // Sentinel FCBs are believed deprecated
        Assert( pfcbTable->FDeletePending() );
        Assert( !pfcbTable->FDeleteCommitted() );
        pfcbTable->Lock();
        pfcbTable->ResetDeletePending();
        pfcbTable->Unlock();

        pfcbTable->PrepareForPurge();
        pfcbTable->Purge();
    }
}


//  ================================================================
LOCAL VOID VERIUndoCreateLV( PIB *ppib, const RCE * const prce )
//  ================================================================
{
    if ( pfcbNil != prce->Pfcb()->Ptdb()->PfcbLV() )
    {
        FCB * const pfcbLV = prce->Pfcb()->Ptdb()->PfcbLV();

        // Need to leave critTrx to wait for tasks to complete
        ppib->CritTrx().Leave();

        VERIWaitForTasks( PverFromPpib( ppib ), pfcbLV, fTrue, fFalse );

        ppib->CritTrx().Enter();

        //  if we rollback the creation of the LV tree, unlink the LV FCB
        //  the FCB will be lost (memory leak)

        VERIUnlinkDefunctLV( prce->Pfucb()->ppib, pfcbLV );
        pfcbLV->PrepareForPurge( fFalse );
        pfcbLV->Purge();
        prce->Pfcb()->Ptdb()->SetPfcbLV( pfcbNil );
    }
}


//  ================================================================
LOCAL VOID VERIUndoCreateIndex( PIB *ppib, const RCE * const prce )
//  ================================================================
{
    Assert( prce->Oper() == operCreateIndex );
    Assert( prce->TrxCommitted() == trxMax );
    Assert( prce->CbData() == sizeof(TDB *) );

    //  pfcb of secondary index FCB or pfcbNil for primary
    //  index creation
    FCB * const pfcb = *(FCB **)prce->PbData();
    FCB * const pfcbTable = prce->Pfucb()->u.pfcb;

    Assert( pfcbNil != pfcbTable );
    Assert( pfcbTable->FTypeTable() );
    Assert( pfcbTable->PrceOldest() != prceNil );   // This prevents the index FCB from being deallocated.

    //  if secondary index then close all cursors on index
    //  and purge index FCB, else free IDB for primary index.

    if ( pfcb != pfcbNil )
    {
        // This can't be the primary index, because we would not have allocated
        // an FCB for it.
        Assert( prce->Pfucb()->u.pfcb != pfcb );

        Assert( pfcb->FTypeSecondaryIndex() );
        Assert( pfcb->Pidb() != pidbNil );

        //  Normally, we grab the updating/indexing latch before we grab
        //  the ppib's critTrx, but in this case, we are already in the
        //  ppib's critTrx (because we are rolling back) and we need the
        //  updating/indexing latch.  We can guarantee that this will not
        //  cause a deadlock because the only person that grabs critTrx
        //  after grabbing the updating/indexing latch is concurrent create
        //  index, which quiesces all rollbacks before it begins.
        CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock();

        pfcbTable->SetIndexing();
        pfcbTable->EnterDDL();

        Assert( !pfcb->Pidb()->FDeleted() );
        Assert( !pfcb->FDeletePending() );
        Assert( !pfcb->FDeleteCommitted() );

        // Mark as committed delete so no one else will attempt version check.
        pfcb->Pidb()->SetFDeleted();
        pfcb->Pidb()->ResetFVersioned();

        if ( pfcb->PfcbTable() == pfcbNil )
        {
            // Index FCB not yet linked into table's FCB list, but we did use
            // table's TDB memory pool to store some information for the IDB.
            pfcb->UnlinkIDB( pfcbTable );
        }
        else
        {
            Assert( pfcb->PfcbTable() == pfcbTable );

            // Unlink the FCB from the table's index list.
            // Note that the only way the FCB could have been
            // linked in is if the IDB was successfully created.
            pfcbTable->UnlinkSecondaryIndex( pfcb );

            //  update all index mask
            FILESetAllIndexMask( pfcbTable );
        }

        pfcbTable->LeaveDDL();
        pfcbTable->ResetIndexing();

        //  must leave critTrx because we may enter the critTrx
        //  of other sessions when we try to remove their RCEs
        //  or cursors on this index
        ppib->CritTrx().Leave();

        // Index FCB has been unlinked from table, so we're
        // guaranteed no further versions will occur on this
        // FCB.  Clean up remaining versions.
        VERNullifyAllVersionsOnFCB( pfcb );

        VERIWaitForTasks( PverFromPpib( ppib ), pfcb, fTrue, fFalse );

        //  set FDeleteCommitted flag to silence asserts in ErrSPFreeFDP()
        //
        pfcb->Lock();
        pfcb->SetDeleteCommitted();
        pfcb->Unlock();

        //  ignore errors if we can't free the space (it will be leaked)
        //
        //  WARNING: if this is recovery and the index was not logged,
        //  it's very possible that not all pages in the OwnExt tree
        //  were flushed and the btree is therefore inconsistent, so
        //  this may fail with weird errors (though I don't believe
        //  it will crash or corrupt space in the parent) and
        //  therefore leak the space
        //
        
        VERIUnlinkDefunctSecondaryIndex( prce->Pfucb()->ppib, pfcb );

        ppib->CritTrx().Enter();

        // The table's version count will prevent the
        // table FCB (and thus this secondary index FCB)
        // from being deallocated before we can delete
        // this index FCB.
        //
        //  prepare the FCB to be purged
        //  this removes the FCB from the hash-table among other things
        //      so that the following case cannot happen:
        //          we free the space for this FCB
        //          someone else allocates it
        //          someone else BTOpen's the space
        //          we try to purge the table and find that the refcnt
        //              is not zero and the state of the FCB says it is
        //              currently in use!
        //          result --> CONCURRENCY HOLE
        pfcb->PrepareForPurge( fFalse );

        const BOOL fCleanUpStateSavedSavedSaved = FOSSetCleanupState( fFalse );

        //  ignore errors if we can't free the space (it will be leaked)
        //
        (VOID)ErrSPFreeFDP( ppib, pfcb, pfcbTable->PgnoFDP(), fTrue );

        //  Restore cleanup checking
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

        // Mark as committed delete so no one else will attempt version check.
        pidb->SetFDeleted();
        pidb->ResetFVersioned();
        pidb->ResetFVersionedCreate();

        pfcbTable->UnlinkIDB( pfcbTable );
        pfcbTable->SetPidb( pidbNil );
        pfcbTable->Lock();
        pfcbTable->SetSequentialIndex();
        pfcbTable->Unlock();

        // UNDONE: Reset density to original value.
        pfcbTable->SetSpaceHints( PSystemSpaceHints(eJSPHDefaultUserIndex) );

        //  update all index mask
        FILESetAllIndexMask( pfcbTable );

        pfcbTable->LeaveDDL();
    }
}


//  ================================================================
LOCAL VOID VERIUndoDeleteIndex( const RCE * const prce )
//  ================================================================
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


//  ================================================================
INLINE VOID VERIUndoAllocExt( const RCE * const prce )
//  ================================================================
{
    Assert( prce->CbData() == sizeof(VEREXT) );
    Assert( prce->PgnoFDP() == ((VEREXT*)prce->PbData())->pgnoFDP );

    const VEREXT* const pverext = (const VEREXT*)(prce->PbData());

    VERIFreeExt( prce->Pfucb()->ppib, prce->Pfcb(),
        pverext->pgnoFirst,
        pverext->cpgSize );
}


//  ================================================================
INLINE VOID VERIUndoRegisterCallback( const RCE * const prce )
//  ================================================================
//
//  Remove the callback from the list
//
//-
{
    VERIRemoveCallback( prce );
}


//  ================================================================
VOID VERIUndoUnregisterCallback( const RCE * const prce )
//  ================================================================
//
//  Set the trxUnregisterBegin0 in the CBDESC to trxMax
//
//-
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
#endif  //  VERSIONED_CALLBACKS
    prce->Pfcb()->LeaveDDL();
}


//  ================================================================
INLINE VOID VERIUndoNonLoggedOper( PIB *ppib, RCE * const prce, RCE **pprceNextToUndo )
//  ================================================================
{
    Assert( *pprceNextToUndo == prce->PrcePrevOfSession() );

    switch( prce->Oper() )
    {
        //  non-logged operations
        //
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
            //  refresh prceNextToUndo in case RCE list was
            //  updated when we lost critTrx
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
            //  should never need to rollback an operPreInsert, because
            //  they are either promoted to an operInsert or manually nullified
            //  in the same transaction
            //
        default:
            Assert( fFalse );
            break;

        case operCreateTable:
            VERIUndoCreateTable( ppib, prce );
            // For CreateTable only, the RCE is nullified, so no need
            // to set RolledBack flag -- get out immediately.
            Assert( prce->FOperNull() );
            return;
    }

    Assert( !prce->FOperNull() );
    Assert( !prce->FRolledBack() );

    //  we have successfully undone the operation
    VERINullifyRolledBackRCE( ppib, prce );
}


//
//Log file recovery gets into infinite loop if there is a logical corruption 
//if it exceeds the limit(MAX_ROLLBACK_RETRIES) then we terminate the process
//
#define MAX_ROLLBACK_RETRIES 100

//  ================================================================
ERR ErrVERRollback( PIB *ppib )
//  ================================================================
//
//  Rollback is done in 2 phase. 1st phase is to undo the versioned
//  operation and may involve IO. 2nd phase is nullify the undone
//  RCE. 2 phases are needed so that the version will be held till all
//  IO is done, then wipe them all. If it is mixed, then once we undo
//  a RCE, the record become writable to other session. This may mess
//  up recovery where we may get write conflict since we have not guarrantee
//  that the log for operations on undone record won't be logged before
//  Rollback record is logged.
//  UNDONE: rollback should behave the same as commit, and need two phase log.
//
//-
{
    ASSERT_VALID( ppib );
    Assert( ppib->Level() > 0 );

    const   LEVEL   level               = ppib->Level();
    INT     cRepeat = 0;

    ERR err = JET_errSuccess;
    
    if ( prceNil != ppib->prceNewest )
    {
        ENTERCRITICALSECTION critTrx( &ppib->CritTrx() );
    
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
    
            //  Save next RCE to process, because RCE will attempt to be
            //  nullified if undo is successful.
            prceNextToUndo = prceToUndo->PrcePrevOfSession();
    
            if ( prceToUndo->FUndoableLoggedOper() )
            {
                Assert( pfucbNil != prceToUndo->Pfucb() );
                Assert( ppib == prceToUndo->Pfucb()->ppib );
                Assert( JET_errSuccess == ppib->ErrRollbackFailure() );
    
                //  logged operations
                //
    
                err = ErrVERIUndoLoggedOper( ppib, prceToUndo );
                if ( err < JET_errSuccess )
                {
                    // if due to an error we stopped a database usage
                    // error out from the rollback
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
                        continue;       //  sidestep resetting of cRepeat
                    }
                    Assert ( fFalse );
                }
            }
    
            else
            {
                // non-logged operations can never fail to rollback
                VERIUndoNonLoggedOper( ppib, prceToUndo, &prceNextToUndo );
            }
    
            cRepeat = 0;
        }
    
        //  must loop through again and catch any RCE's that couldn't be nullified
        //  during the first pass because of concurrent create index
        prceToNullify = ppib->prceNewest;
        while ( prceNil != prceToNullify && prceToNullify->Level() == level )
        {
            // Only time nullification should have failed during the first pass is
            // if the RCE was locked for concurrent create index.  This can only
            // occur on a non-catalog, non-fixed DDL table for an Insert,
            // FlagDelete, or Replace operation.
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

    // If this PIB has any RCE's remaining, they must be at a lower level.
    Assert( prceNil == ppib->prceNewest
        || ppib->prceNewest->Level() <= level );

    //  decrement session transaction level
    Assert( level == ppib->Level() );
    if ( 1 == ppib->Level() )
    {
        //  we should have processed all RCEs
        Assert( prceNil == ppib->prceNewest );
        PIBSetPrceNewest( ppib, prceNil );          //  safety measure, in case it's not NULL for whatever reason
    }
    Assert( ppib->Level() > 0 );
    ppib->DecrementLevel();

HandleError:
    return err;
}


//  ================================================================
ERR ErrVERRollback( PIB *ppib, UPDATEID updateid )
//  ================================================================
//
//  This is used to rollback the operations from one particular update
//  all levels are rolled back.
//
//  Rollback is done in 2 phase. 1st phase is to undo the versioned
//  operation and may involve IO. 2nd phase is nullify the undone
//  RCE. 2 phases are needed so that the version will be held till all
//  IO is done, then wipe them all. If it is mixed, then once we undo
//  a RCE, the record become writable to other session. This may mess
//  up recovery where we may get write conflict since we have not guarrantee
//  that the log for operations on undone record won't be logged before
//  Rollback record is logged.
//  UNDONE: rollback should behave the same as commit, and need two phase log.
//
//-
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

    ppib->CritTrx().Enter();

    RCE *prceToUndo;
    RCE *prceNextToUndo;

    for( prceToUndo = ppib->prceNewest;
        prceNil != prceToUndo && prceToUndo->TrxCommitted() == trxMax;
        prceToUndo = prceNextToUndo )
    {
        //  Save next RCE to process, because RCE will attemp to be nullified
        //  if undo is successful.
        prceNextToUndo = prceToUndo->PrcePrevOfSession();

        if ( prceToUndo->Updateid() == updateid )
        {
            Assert( pfucbNil != prceToUndo->Pfucb() );
            Assert( ppib == prceToUndo->Pfucb()->ppib );
            Assert( !prceToUndo->FOperNull() );
            Assert( prceToUndo->Pfcb() != pfcbNil );
            Assert( !prceToUndo->FRolledBack() );

            //  the only RCEs with an updateid should be DML RCE's.
            Assert( prceToUndo->FUndoableLoggedOper() );
            Assert( JET_errSuccess == ppib->ErrRollbackFailure() );

            err = ErrVERIUndoLoggedOper( ppib, prceToUndo );
            if ( err < JET_errSuccess )
            {
                // if due to an error we stopped a database usage
                // error out from the rollback
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
                    continue;       //  sidestep resetting of cRepeat
                }
                Assert ( fFalse );
            }
        }

        cRepeat = 0;
    }


    //  must loop through again and catch any RCE's that couldn't be nullified
    //  during the first pass because of concurrent create index
    RCE *prceToNullify;
    prceToNullify = ppib->prceNewest;
    while ( prceNil != prceToNullify && prceToNullify->TrxCommitted() == trxMax )
    {
        if ( prceToNullify->Updateid() == updateid )
        {
            // Only time nullification should have failed during the first pass is
            // if the RCE was locked for concurrent create index.  This can only
            // occur on a non-catalog, non-fixed DDL table for an Insert,
            // FlagDelete, or Replace operation.
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
    ppib->CritTrx().Leave();

    return err;
}

//  ================================================================
JETUNITTEST( VER, RceidCmpReturnsZeroWhenRceidsAreEqual )
//  ================================================================
{
    CHECK(0 == RceidCmp(5,5));
}

//  ================================================================
JETUNITTEST( VER, RceidCmpGreaterThan )
//  ================================================================
{
    CHECK(RceidCmp(11,5) > 0);
}

//  ================================================================
JETUNITTEST( VER, RceidCmpGreaterThanWraparound )
//  ================================================================
{
    CHECK(RceidCmp(3,0xFFFFFFFD) > 0);
}

//  ================================================================
JETUNITTEST( VER, RceidCmpLessThan )
//  ================================================================
{
    CHECK(RceidCmp(21,35) < 0);
}

//  ================================================================
JETUNITTEST( VER, RceidCmpLessThanWraparound )
//  ================================================================
{
    CHECK(RceidCmp(0xFFFFFF0F,1) < 0);
}

//  ================================================================
JETUNITTEST( VER, RceidNullIsLessThanAnything )
//  ================================================================
{
    CHECK(RceidCmp(rceidNull,0xFFFFFFFF) < 0);
}

//  ================================================================
JETUNITTEST( VER, AnythingIsGreaterThanRceidNull )
//  ================================================================
{
    CHECK(RceidCmp(1,rceidNull) > 0);
}

