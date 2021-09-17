// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"


#ifdef PERFMON_SUPPORT

//  performance counters

//  "asynchronous" purging in FCB::ErrAlloc_ and
//      FCB::FCheckFreeAndPurge_

PERFInstanceDelayedTotalWithClass<> cFCBAsyncScan;
PERFInstanceDelayedTotalWithClass<> cFCBAsyncPurgeSuccess;
PERFInstanceDelayedTotalWithClass<> cFCBAsyncPurgeFail;

//  synchronous purging in FCB::LockForPurge_

PERFInstanceDelayedTotal<> cFCBSyncPurge;
PERFInstanceDelayedTotal<> cFCBSyncPurgeStalls;
PERFInstanceDelayedTotal<> cFCBPurgeOnClose;
PERFInstanceDelayedTotal<> cFCBAllocWaitForRCEClean;

//  FCB cache activity

PERFInstanceDelayedTotal<> cFCBCacheHits;
PERFInstanceDelayedTotal<> cFCBCacheRequests;
PERFInstanceDelayedTotal<> cFCBCacheStalls;

//  FCB cache sizes and usage

PERFInstanceDelayedTotal<LONG, INST, fFalse> cFCBCacheMax;
PERFInstanceDelayedTotal<LONG, INST, fFalse> cFCBCachePreferred;

PERFInstanceDelayedTotal<> cFCBCacheAlloc;
PERFInstanceDelayedTotal<> cFCBCacheAllocAvail;
PERFInstanceDelayedTotal<> cFCBCacheAllocFailed;
PERFInstance<DWORD_PTR, fTrue> cFCBCacheAllocLatency;

//  FCB version store usage

PERFInstanceDelayedTotal<> cFCBAttachedRCE;

//  perf counter function declarations


//  perf counter function bodies

LONG LFCBAsyncScanCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBAsyncScan.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBAsyncPurgeCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBAsyncPurgeSuccess.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBAsyncThresholdPurgeFailCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBAsyncPurgeFail.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBSyncPurgeCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBSyncPurge.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBSyncPurgeStallsCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBSyncPurgeStalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBPurgeOnCloseCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBPurgeOnClose.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBAllocWaitForRCECleanCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBAllocWaitForRCEClean.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheHitsCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheHits.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheRequestsCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheRequests.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheStallsCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheStalls.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheMaxCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheMax.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCachePreferredCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCachePreferred.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheAllocCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheAlloc.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheAllocAvailCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheAllocAvail.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheAllocFailedCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheAllocFailed.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBCacheAllocLatencyCEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBCacheAllocLatency.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LFCBAttachedRCECEFLPv( LONG iInstance, void* pvBuf )
{
    cFCBAttachedRCE.PassTo( iInstance, pvBuf );
    return 0;
}

#endif  //  PERFMON_SUPPORT


//
// The FCB lock is taken shared before modifying the m_wRefCount, which is then modified
// using atomic APIs.  If the result of an atomic API is a transition from 0 to 1 or
// from 1 to 0, the write lock is taken before taking any further action.  This allows
// multi-threaded simultaneous access to the refcount, and single threaded access to
// decisions regarding the AvailList.
//
// The FCB lock is taken exclusive modifying an FCBs state with respect to the AvailList.
// In a very narrow sense, the write lock keeps m_wRefCount and m_fFCBInLRU in sync
// when they can be out of sync with just the shared lock.  Holding the write lock
// guarantees the FCB will not be purged by another thread while you hold the lock.
//
// Of course, it can also be taken exclusive for any other operations needing critical section
// semantics.
//
// A note about the effect of FNeedLock_() on these locking routines:
// Some FCBs don't need to be locked because they are guaranteed to be accessed either
// single-threaded or very carefully.  For those FCBs, any lock is at best redundant
// and at worst counter-productive.  So, in these locking routines, we check for that
// condition.  If it's true, we will respond as if the request to operate on the lock
// always succeeds, even though we don't actually do anything with the lock.
// This has one gotcha, though.  For the debug routines IsLocked_ and IsUnlocked_, we'll
// always respond "fTrue".  That means that for FCBs like that:
//
//   pfcb->IsLocked_( X ) != !pfcb->IsUnlocked_( X )
//
// IsLocked_() and IsUnlocked_() are used exclusively in asserts.  Make sure
// you don't use the negation operator on those routines; always use the positive
// routine.
// 
INLINE VOID FCB::Lock_( FCB::LOCK_TYPE lt)
{
    CSXWLatch::ERR errSXWLatch;

    if ( !FNeedLock_() )
    {
        return;
    }

    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    switch ( lt )
    {
        case FCB::LOCK_TYPE::ltShared:
            errSXWLatch = m_sxwl.ErrAcquireSharedLatch();

            switch ( errSXWLatch )
            {
                case CSXWLatch::ERR::errSuccess:
                    break;

                case CSXWLatch::ERR::errWaitForSharedLatch:
                    m_sxwl.WaitForSharedLatch();
                    break;

                default:
                    Enforce( fFalse );
                    break;
            }
            Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
            break;

        case FCB::LOCK_TYPE::ltWrite:
            m_sxwl.AcquireWriteLatch();
            Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
            break;

        default:
            EnforceSz( fFalse, "Unknown lock type.");
            break;
    }

    Assert( IsLocked_( lt ) );
}

INLINE BOOL FCB::FLockTry_( FCB::LOCK_TYPE lt)
{
    CSXWLatch::ERR errSXWLatch;
    BOOL fRetval;

    if ( !FNeedLock_() )
    {
        return fTrue;
    }

    switch ( lt )
    {
        case LOCK_TYPE::ltShared:
            ExpectedSz( fFalse, "No known callers of this yet." );
            errSXWLatch = m_sxwl.ErrTryAcquireSharedLatch();
            break;

        case LOCK_TYPE::ltWrite:
            errSXWLatch = m_sxwl.ErrTryAcquireWriteLatch();
            break;

        default:
            EnforceSz( fFalse, "Unknown lock type.");
            return fFalse;
    }

    switch ( errSXWLatch )
    {
        case CSXWLatch::ERR::errSuccess:
            fRetval = fTrue;
            break;

        case CSXWLatch::ERR::errWaitForWriteLatch:
            fRetval = fFalse;
            break;

        case CSXWLatch::ERR::errLatchConflict:
            fRetval = fFalse;
            break;

        default:
            EnforceSz( fFalse, "Unhandled result from ErrTryAcquire_*_Latch()" );
            fRetval = fFalse;
    }

    Assert( fRetval ? IsLocked_( lt ) : IsUnlocked_( lt ) );

    return fRetval;
}

INLINE VOID FCB::Unlock_( FCB::LOCK_TYPE lt)
{
    if ( !FNeedLock_() )
    {
        return;
    }

    Assert( IsLocked_( lt ) );
    switch ( lt )
    {
        case FCB::LOCK_TYPE::ltShared:
            m_sxwl.ReleaseSharedLatch();
            break;

        case FCB::LOCK_TYPE::ltWrite:
            m_sxwl.ReleaseWriteLatch();            
            break;

        default:
            EnforceSz( fFalse, "Unknown lock type.");
    }

    // Voice of experience.  One would like to do:
    //    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    //    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    // But, one can't.  We can be here, unlocking during DecrementRefCountAndUnlink where
    // we DIDN'T decrement to 0, but after we released another thread snuck in and
    // DID decrement to 0 and furthermore, purged.  Thus, it's not safe to do anything with "this"
    // after we unlocked. 
}    


#ifdef DEBUG
INLINE BOOL FCB::IsLocked_(  FCB::LOCK_TYPE lt )
{
    BOOL fLockedWrite;
    BOOL fLockedShared;;
    BOOL fLocked;

    if ( !FNeedLock_() )
    {
        return fTrue;
    }

    fLockedShared = m_sxwl.FOwnSharedLatch();
    fLockedWrite = m_sxwl.FOwnWriteLatch();

    Assert( !(fLockedShared && fLockedWrite ) );

    switch ( lt )
    {
        case FCB::LOCK_TYPE::ltShared:
            fLocked = fLockedShared;
            break;

        case FCB::LOCK_TYPE::ltWrite:
            fLocked = fLockedWrite;
            break;

        default:
            AssertSz( fFalse, "Unknown lock type.");
            fLocked = fFalse;
            break;
    }

    return fLocked;
}

INLINE BOOL FCB::IsUnlocked_( FCB::LOCK_TYPE lt )
{
    BOOL fLockedWrite;
    BOOL fLockedShared;;
    BOOL fLocked;

    if ( !FNeedLock_() )
    {
        return fTrue;
    }

    fLockedShared = m_sxwl.FOwnSharedLatch();
    fLockedWrite = m_sxwl.FOwnWriteLatch();

    Assert( !(fLockedShared && fLockedWrite ) );

    switch ( lt )
    {
        case FCB::LOCK_TYPE::ltShared:
            fLocked = fLockedShared;
            break;

        case FCB::LOCK_TYPE::ltWrite:
            fLocked = fLockedWrite;
            break;

        default:
            AssertSz( fFalse, "Unknown lock type.");
            fLocked = fFalse;
            break;
    }

    return !fLocked;
}
#endif

INLINE VOID FCB::LockDowngradeWriteToShared_()
{
    if ( !FNeedLock_() )
    {
        return;
    }

    Assert( IsLocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    
    m_sxwl.DowngradeWriteLatchToSharedLatch();

    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsLocked_( LOCK_TYPE::ltShared ) );
}


FUCB* FCB::Pfucb()
{
    FUCB *pfucb;

    Assert( IsLocked_( LOCK_TYPE::ltWrite ) );

    FucbList().LockForEnumeration();
    pfucb = FucbList()[0];
    FucbList().UnlockForEnumeration();

    return pfucb;
}

#ifdef DEBUG
//  Verifies that the FCB is in the correctly in or out of the avail list.
//  We kind of cheat.  We always return fTrue.  We expect to be called inside
//  an Assert(), but we assert on everything here, so we don't actually need
//  to return fFalse.  You get a more specific assert that way.
INLINE BOOL FCB::FCBCheckAvailList_( const BOOL fShouldBeInList, const BOOL fPurging )
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( pinst->m_critFCBList.FOwner() );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( fPurging ? IsUnlocked_( LOCK_TYPE::ltWrite ) : IsLocked_( LOCK_TYPE::ltWrite ) );

    FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU();
    FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU();

    if ( fShouldBeInList )
    {
        //  Since something SHOULD be in the list, the list should not be empty.
        Assert( pinst->m_cFCBAvail > 0 );
        Assert( pinst->m_cFCBAlloc > 0 );
        Assert( pinst->m_cFCBAvail <= pinst->m_cFCBAlloc );
        Assert( *ppfcbAvailMRU != pfcbNil );
        Assert( *ppfcbAvailLRU != pfcbNil );
        Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
        Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );

    }
    else
    {
        //  verify the consistency of the list (it may be empty)
        Assert( pinst->m_cFCBAvail >= 0 ); // 0 included, list may be empty. 
        Assert( pinst->m_cFCBAlloc > 0 );  // 0 not included, the list ought to exist.
        Assert( pinst->m_cFCBAvail <= pinst->m_cFCBAlloc );
        if ( *ppfcbAvailMRU != pfcbNil )
        {
            Assert( (*ppfcbAvailMRU)->PfcbMRU() == pfcbNil );
        }
        if ( *ppfcbAvailLRU != pfcbNil )
        {
            Assert( (*ppfcbAvailLRU)->PfcbLRU() == pfcbNil );
        }
    }

    //  scan for the FCB
    FCB *   pfcbThis = pfcbNil;
    for ( pfcbThis = *ppfcbAvailMRU;
          pfcbThis != pfcbNil;
          pfcbThis = pfcbThis->PfcbLRU() )
    {
        if ( pfcbThis == this )
            break;
    }

    // Can't assert on WRefCount; we might be about to add/remove from AvailList
    // after decrementing/incrementing m_wRefCount, so list membership isn't
    // currently in sync with refcount.

    if ( fShouldBeInList )
    {
        //  the FCB should be in both the avail-LRU list and the global list

        Assert( FInLRU() );


        // Can we assert something based on the the list pointers on the FCB?
        // Assert( PfcbMRU() ?? pfcbNil );
        // Assert( PfcbLRU() ?? pfcbNil );

        //  we only allow table FCBs in the avail-LRU list

        Assert( FTypeTable() );

        Assert( pfcbThis == this );        
    }
    else
    {
        Assert( !FInLRU() );

        Assert( PfcbMRU() == pfcbNil );
        Assert( PfcbLRU() == pfcbNil );

        Assert( pfcbThis == pfcbNil );        
    }

    return fTrue;
}
#endif  //  DEBUG

VOID FCB::RefreshPreferredPerfCounter( _In_ INST * const pinst )
{
    PERFOpt( cFCBCachePreferred.Set( pinst, (LONG)UlParam( pinst, JET_paramCachedClosedTables ) ) );
}

VOID FCB::ResetPerfCounters( _In_ INST * const pinst, BOOL fTerminating )
{
    PERFOpt( cFCBSyncPurge.Clear( pinst ) );
    PERFOpt( cFCBSyncPurgeStalls.Clear( pinst ) );
    PERFOpt( cFCBPurgeOnClose.Clear( pinst ) );
    PERFOpt( cFCBAllocWaitForRCEClean.Clear( pinst ) );
    PERFOpt( cFCBCacheHits.Clear( pinst ) );
    PERFOpt( cFCBCacheRequests.Clear( pinst ) );
    PERFOpt( cFCBCacheStalls.Clear( pinst ) );

    if ( fTerminating )
    {
        PERFOpt( cFCBCacheMax.Clear( pinst ) );
        PERFOpt( cFCBCachePreferred.Clear( pinst ) );
    }
    else
    {
        DWORD_PTR cFCBQuota;
        CallS( ErrRESGetResourceParam( pinst, JET_residFCB, JET_resoperMaxUse, &cFCBQuota ) );

        PERFOpt( cFCBCacheMax.Set( pinst, (LONG)cFCBQuota ) );
        FCB::RefreshPreferredPerfCounter( pinst );
    }

    PERFOpt( cFCBCacheAlloc.Clear( pinst ) );
    PERFOpt( cFCBCacheAllocAvail.Clear( pinst ) );
    PERFOpt( cFCBCacheAllocFailed.Clear( pinst ) );
    PERFOpt( cFCBCacheAllocLatency.Clear( pinst->m_iInstance ) );
    PERFOpt( cFCBAttachedRCE.Clear( pinst ) );
}

//  initialize the FCB manager (per-instance initialization)

ERR FCB::ErrFCBInit( INST *pinst )
{
    ERR             err;
    BYTE            *rgbFCBHash;
    FCBHash::ERR    errFCBHash;

    //  init the FCB, TDB, and IDB resource managers
    //
    //  NOTE:  if we are recovering, then disable quotas
    //
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residFCB, JET_resoperEnableMaxUse, fFalse ) );
        CallS( ErrRESSetResourceParam( pinst, JET_residTDB, JET_resoperEnableMaxUse, fFalse ) );
        CallS( ErrRESSetResourceParam( pinst, JET_residIDB, JET_resoperEnableMaxUse, fFalse ) );
    }
    Call( pinst->m_cresFCB.ErrInit( JET_residFCB ) );
    Call( pinst->m_cresTDB.ErrInit( JET_residTDB ) );
    Call( pinst->m_cresIDB.ErrInit( JET_residIDB ) );

    Assert( !pinst->m_pfcbhash );
    rgbFCBHash = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( FCBHash ), cbCacheLine );
    if ( NULL == rgbFCBHash  )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
    }
    Call( err );
    pinst->m_pfcbhash = new( rgbFCBHash ) FCBHash( rankFCBHash );

    Assert( pfcbNil == (FCB *)0 );
    Assert( pinst->m_pfcbList == pfcbNil );
    Assert( pinst->m_pfcbAvailMRU == pfcbNil );
    Assert( pinst->m_pfcbAvailLRU == pfcbNil );
    pinst->m_cFCBAvail = 0;
    pinst->m_cFCBAlloc = 0;

    //  initialize the FCB hash-table
    //      5.0 entries/bucket (average)
    //      uniformity ==> 1.0 (perfectly uniform)

    errFCBHash = pinst->m_pfcbhash->ErrInit( 5.0, 1.0 );
    if ( errFCBHash != FCBHash::ERR::errSuccess )
    {
        Assert( errFCBHash == FCBHash::ERR::errOutOfMemory );
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    //  init perf counters
    ResetPerfCounters( pinst, fFalse /* fTerminating */ );

    OnDebug( (VOID)ErrOSTraceCreateRefLog( 10000, 0, &pinst->m_pFCBRefTrace ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        FCB::Term( pinst );
    }
    return err;
}

//  term the FCB manager (per-instance termination)

VOID FCB::Term( INST *pinst )
{
    ResetPerfCounters( pinst, fTrue /* fTerminating */ );

    if ( pinst->m_pFCBRefTrace != NULL )
    {
        OSTraceDestroyRefLog( pinst->m_pFCBRefTrace );
        pinst->m_pFCBRefTrace = NULL;
    }

    pinst->m_pfcbList = pfcbNil;
    pinst->m_pfcbAvailMRU = pfcbNil;
    pinst->m_pfcbAvailLRU = pfcbNil;

    if ( pinst->m_pfcbhash )
    {
        pinst->m_pfcbhash->Term();
        pinst->m_pfcbhash->FCBHash::~FCBHash();
        OSMemoryHeapFreeAlign( pinst->m_pfcbhash );
        pinst->m_pfcbhash = NULL;
    }

    pinst->m_cresFCB.Term();
    pinst->m_cresTDB.Term();
    pinst->m_cresIDB.Term();
    if ( pinst->FRecovering() )
    {
        CallS( ErrRESSetResourceParam( pinst, JET_residFCB, JET_resoperEnableMaxUse, fTrue ) );
        CallS( ErrRESSetResourceParam( pinst, JET_residTDB, JET_resoperEnableMaxUse, fTrue ) );
        CallS( ErrRESSetResourceParam( pinst, JET_residIDB, JET_resoperEnableMaxUse, fTrue ) );
    }
}


BOOL FCB::FValid() const
{
    BOOL fValid = fFalse;

    TRY
    {
        fValid = CResource::FCallingProgramPassedValidJetHandle( JET_residFCB, this );
    }
    EXCEPT( efaExecuteHandler )
    {
        //  nop
        //
        AssertPREFIX( !fValid );
    }
    ENDEXCEPT;

    return fValid;
}

VOID FCB::UnlinkIDB( FCB *pfcbTable )
{
    Assert( pfcbNil != pfcbTable );
    Assert( ptdbNil != pfcbTable->Ptdb() );
    Assert( pfcbTable->FPrimaryIndex() );

    pfcbTable->AssertDDL();

    // If index and table FCB's are the same, then it's the primary index FCB.
    if ( pfcbTable == this )
    {
        Assert( !pfcbTable->FSequentialIndex() );
    }
    else
    {
        // Temp/sort tables don't have secondary indexes
        Assert( pfcbTable->FTypeTable() );

        // Only way IDB could have been linked in is if already set FCB type.
        Assert( FTypeSecondaryIndex() );
    }

    IDB *pidb = Pidb();
    Assert( pidbNil != pidb );

    Assert( !pidb->FTemplateIndex() );
    Assert( !pidb->FDerivedIndex() );

    // Wait for lazy version checkers.  Verify flags set to Deleted and non-Versioned
    // to prevent future version checkers while we're waiting for the lazy ones to finish.
    Assert( pidb->FDeleted() );
    Assert( !pidb->FVersioned() );
    while ( pidb->CrefVersionCheck() > 0 || pidb->CrefCurrentIndex() > 0 )
    {
        // Since we reset the Versioned flag, there should be no further
        // version checks on this IDB.
        pfcbTable->LeaveDDL();
        UtilSleep( cmsecWaitGeneric );
        pfcbTable->EnterDDL();

        Assert( pidb == Pidb() );       // Verify pointer.
    }

    // Free index name and idxseg array, if any.
    if ( pidb->ItagIndexName() != 0 )
    {
        pfcbTable->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
    }
    if ( pidb->FIsRgidxsegInMempool() )
    {
        Assert( pidb->ItagRgidxseg() != 0 );
        pfcbTable->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxseg() );
    }
    if ( pidb->FIsRgidxsegConditionalInMempool() )
    {
        Assert( pidb->ItagRgidxsegConditional() != 0 );
        pfcbTable->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxsegConditional() );
    }

    ReleasePidb();
}


//  lookup an FCB by ifmp/pgnoFDP using the FCB hash-table and pin it by
//      adding 1 to the refcnt
//
//  returns pfcbNil if the FCB does not exist (or exists but is not visible)
//
//  for FCBs that are being initialized but are not finished, this code will
//      retry repeatedly until the initialization finished with success or
//      an error-code
//
//  for FCBs that failed to initialize, they are left in place until they
//      can be purged and recycled
//
//  NOTE: this is the proper channel for accessing an FCB; it uses the locking
//      protocol setup by the FCB hash-table and FCB latch

FCB *FCB::PfcbFCBGet( IFMP ifmp, PGNO pgnoFDP, INT *pfState, const BOOL fIncrementRefCount, const BOOL fInitForRecovery )
{
    INT             fState = fFCBStateNull;
    INST            *pinst = PinstFromIfmp( ifmp );
    FCB             *pfcbT;
    BOOL            fDoIncRefCount = fFalse;
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( ifmp, pgnoFDP );
    FCBHashEntry    entryFCBHash;
    ULONG           cRetries = 0;

RetrieveFCB:
    AssertTrack( cRetries != 100000, "TooManyFcbGetRetries" );

    //  lock the hash table

    pinst->m_pfcbhash->ReadLockKey( keyFCBHash, &lockFCBHash );

    //  update performance counter

    PERFOpt( cFCBCacheRequests.Inc( pinst ) );

    //  try to retrieve the entry

    errFCBHash = pinst->m_pfcbhash->ErrRetrieveEntry( &lockFCBHash, &entryFCBHash );

    if ( errFCBHash != FCBHash::ERR::errSuccess )
    {
        //  unlock the hash table

        pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );

        pfcbT = pfcbNil;
        goto SetStateAndReturn;
    }

    //  update performance counter
    PERFOpt( cFCBCacheHits.Inc( pinst ) );

    //  the entry exists

    Assert( entryFCBHash.m_pgnoFDP == pgnoFDP );
    pfcbT = entryFCBHash.m_pfcb;
    Assert( pfcbNil != pfcbT );

    //  we must declare ourselves as an owner/waiter on the write latch
    //      to prevent the FCB from randomly disappearing (via the purge code)
    //      however, this is complicated due to us holding pinst->m_pfcbhash,
    //      which causes a rank violation if we have to actually wait for the
    //      lock.  So, we have to start by requesting a non-blocking
    //      exclusive latch (which either takes the latch or marks us as waiter),
    //      releasing the hash lock, then completing the exclusive latch and
    //      upgrading to writer.
    //
    //  Note: This is the only place we use an exclusive latch, and we access
    //      m_sxwl directly rather than using the lock abstraction methods of FCB.
    //      Abstracting this multistep process wasn't worth the effort.
    //
    //  SOMEONE: CONSIDER: An alternative would be to try to get the write lock, and if
    //      we fail, deal with it the same way we deal with an unit'ed FCB
    //      (i.e. drop the hash lock and goto RetrieveFCB:)  That we could do
    //      using the existing abstraction.  But I don't think it's as safe, since
    //      I think that when we try to acquire the exclusive latch, we register
    //      ourselves as next for the latch.
    if ( pfcbT->FNeedLock_() )
    {
        CSXWLatch::ERR errSXWLatch = pfcbT->m_sxwl.ErrAcquireExclusiveLatch();

        //  unlock the hash table
        pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );

        switch ( errSXWLatch )
        {
        case CSXWLatch::ERR::errSuccess:
            break;

        case CSXWLatch::ERR::errWaitForExclusiveLatch:
            pfcbT->m_sxwl.WaitForExclusiveLatch();
            break;

        default:
            Enforce( fFalse );
            break;
        }

        pfcbT->m_sxwl.UpgradeExclusiveLatchToWriteLatch();
    }
    else
    {
        //  unlock the hash table
        pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );
    }


    //  we now have the FCB pinned via the latch

    if ( !pfcbT->FInitialized() )
    {

        //  the FCB is not initialized meaning it is either being
        //      initialized now or failed somewhere in the middle
        //      of initialization

        const ERR   errInit     = pfcbT->ErrErrInit();

        //  release write latch
        pfcbT->Unlock_( LOCK_TYPE::ltWrite );

        if ( JET_errSuccess != errInit )
        {

            //  FCB init failed with an error code

            Assert( errInit < JET_errSuccess );
            pfcbT = pfcbNil;
            goto SetStateAndReturn;
        }
        else
        {

            //  FCB is not finished initializing

            //  update performance counter

            PERFOpt( cFCBCacheStalls.Inc( pinst ) );

            //  wait

            UtilSleep( 10 );

            //  try to get the FCB again

            cRetries++;
            goto RetrieveFCB;
        }
    }


    //  FCB is initialized

    CallS( pfcbT->ErrErrInit() );
    Assert( pfcbT->Ifmp() == ifmp );
    Assert( pfcbT->PgnoFDP() == pgnoFDP );

    if ( !fIncrementRefCount )
    {

        //  there is no state when "checking" the presence of the FCB

        Assert( pfState == NULL );
    }
    else
    {
        fState = fFCBStateInitialized;

        //  increment the reference count

        fDoIncRefCount = fTrue;
    }


    // If this is the dummy FCB created by recovery, we need to fully populate
    // it, make sure that the others wait while the first person finishes doing it

    if ( pfcbT != pfcbNil && !fInitForRecovery && pfcbT->FInitedForRecovery() )
    {
        if ( !pfcbT->FDoingAdditionalInitializationDuringRecovery() )
        {
            Assert( pfcbT->IsLocked_( LOCK_TYPE::ltWrite ) );
            pfcbT->SetDoingAdditionalInitializationDuringRecovery();
        }
        else
        {
            //  release write latch
            pfcbT->Unlock_( LOCK_TYPE::ltWrite );

            //  FCB is not finished initializing
            //  update performance counter

            PERFOpt( cFCBCacheStalls.Inc( pinst ) );

            //  wait

            UtilSleep( 10 );

            //  try to get the FCB again

            fState = fFCBStateNull;

            cRetries++;
            goto RetrieveFCB;
        }
    }

    if ( pfcbT != pfcbNil )
    {
        if ( fDoIncRefCount )
        {
            pfcbT->IncrementRefCount_( fTrue );
        }

        pfcbT->Unlock_( LOCK_TYPE::ltWrite );
    }

SetStateAndReturn:
    //  set the state
    if ( pfState )
    {
        *pfState = fState;
    }

    //  return the FCB
    Assert( ( pfcbNil == pfcbT ) || ( pfcbT->IsUnlocked_( LOCK_TYPE::ltShared ) && pfcbT->IsUnlocked_( LOCK_TYPE::ltWrite ) ) );
    return pfcbT;
}


//  create a new FCB
//
//  this function allocates an FCB and possibly recycles unused FCBs for later
//      use
//  once an FCB is allocated, this code uses the proper locking protocls to
//      insert it into the hash-table; no one will be able to look it up
//      until the initialization is completed successfully or with an error
//
//  WARNING: we leave this function holding the FCB lock (IsLocked_() == fTrue);
//           this is so the caller can perform further initialization before
//               anyone else can touch the FCB including PfcbFCBGet (this is
//               not currently used for anything except some Assert()s even
//               though it could be)

ERR FCB::ErrCreate( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb )
{
    ERR     err;
    INST    *pinst = PinstFromPpib( ppib );

    //  prepare output

    *ppfcb = pfcbNil;

    //  acquire the creation mutex (used critical section for deadlock-detect info)

    pinst->m_critFCBCreate.Enter();

    if ( !FInHashTable( ifmp, pgnoFDP ) )
    {

        //  the entry does not yet exist, so we are guaranteed
        //      to be the first user to create this FCB

        //  try to allocate a new FCB

        err = ErrAlloc_( ppib, ifmp, pgnoFDP, ppfcb );
        Assert( err == JET_errSuccess || err == JET_errTooManyOpenTables || err == JET_errTooManyOpenTablesAndCleanupTimedOut || err == errFCBExists );

        if ( JET_errTooManyOpenTables == err ||
            JET_errTooManyOpenTablesAndCleanupTimedOut == err )
        {
            if ( FInHashTable( ifmp, pgnoFDP ) )
            {
                //  someone beat us to it

                err = ErrERRCheck( errFCBExists );
            }
        }

        if ( err >= JET_errSuccess )
        {
            Assert( *ppfcb != pfcbNil );

            FCBHash::ERR    errFCBHash;
            FCBHash::CLock  lockFCBHash;
            FCBHashKey      keyFCBHash( ifmp, pgnoFDP );
            FCBHashEntry    entryFCBHash( pgnoFDP, *ppfcb );

            //  lock the hash-table

            pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

            //  try to insert the entry

            errFCBHash = pinst->m_pfcbhash->ErrInsertEntry( &lockFCBHash, entryFCBHash );
            if ( errFCBHash == FCBHash::ERR::errSuccess )
            {

                //  the FCB is now in the hash table

                //  verify the FCB

                Assert( pgnoNull == (*ppfcb)->PgnoNextAvailSE() );
                Assert( NULL == (*ppfcb)->Psplitbufdangling_() );
                Assert( (*ppfcb)->PrceOldest() == prceNil );

                //  declare ourselves as an owner/waiter on the write latch
                //  we should immediately become the owner since we just
                //      constructed this latch AND since we are the only ones
                //      with access to it
                (*ppfcb)->Lock();
                err = JET_errSuccess;
            }
            else
            {

                //  we were unable to insert the FCB into the hash table

                Assert( errFCBHash == FCBHash::ERR::errOutOfMemory );

                //  release the FCB

                delete *ppfcb;
                *ppfcb = pfcbNil;

                AtomicDecrement( (LONG *)&pinst->m_cFCBAlloc );

                //  update performance counter

                PERFOpt( cFCBCacheAlloc.Dec( pinst ) );

                //  NOTE: we should never get errKeyDuplicate because only 1 person
                //        can create FCBs at a time (that's us right now), and we
                //        made sure that the entry did not exist when we started!

                err = ErrERRCheck( JET_errOutOfMemory );
            }

            //  unlock the hash table

            pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );
        }
    }
    else
    {

        //  the FCB was already in the hash-table

        err = ErrERRCheck( errFCBExists );
    }

    //  unlock the creation mutex

    pinst->m_critFCBCreate.Leave();

    Assert( ( err >= JET_errSuccess && *ppfcb != pfcbNil ) ||
            ( err < JET_errSuccess && *ppfcb == pfcbNil ) );

    return err;
}


//  finish the FCB creation process by assigning an error code to the FCB
//      and setting/resetting the FInitialized flag
//
//  if the FCB is completing with error, it will be !FInitialized() and
//      the error-code will be stored in the FCB; this signals PfcbFCBGet
//      that the FCB is not "visible" even though it exists
//

VOID FCB::CreateComplete_( ERR err, PCSTR szFile, const LONG lLine )
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( IsLocked_( LOCK_TYPE::ltWrite ) );

    CallSx( err, errFCBUnusable );

    m_szInitFile = szFile;
    m_lInitLine = lLine;

    OSTrace( JET_tracetagFCBs, OSFormat( "FCB::CreateComplete: Err %d, file %s, line %d\n", err, szFile, lLine ) );

    if ( pinst->m_pFCBRefTrace != NULL )
    {
        OSTraceWriteRefLog( pinst->m_pFCBRefTrace, err, this );
    }

    //  set the initialization result

    SetErrInit( err );

    if ( JET_errSuccess == err )
    {
        Assert( !FInitialized() || FInitedForRecovery() );

        //  initialization succeeded

        SetInitialized_();
    }
    else
    {
        //  initialization failed

        ResetInitialized_();
    }
}


//  Scans and purges the LRU available list for FCBs.

BOOL FCB::FScanAndPurge_(
    _In_ INST * pinst,
    _In_ PIB * ppib,
    const BOOL fThreshold )
{
    ULONG   cFCBInspected                   = 0;

    const ULONG cMaxFCBInspected        = (ULONG) max( UlParam( pinst, JET_paramCachedClosedTables ) * 0.02, 256 ); // 2% of cached closed tables or 256, whatever is biggest.
    const ULONG cMinFCBToPurge          = (ULONG) max( UlParam( pinst, JET_paramCachedClosedTables ) * 0.01, 128 ); // 1% of cached closed tables or 128, whatever is biggest.
    ULONG cFCBToPurge;

    if ( fThreshold )
    {
        cFCBToPurge = (ULONG) min( pinst->m_cFCBAvail - UlParam( pinst, JET_paramCachedClosedTables ) + cMinFCBToPurge, cMinFCBToPurge ); // At most cMinFCBToPurge.
    }
    else
    {
        cFCBToPurge = 1;
    }

    ULONG       cFCBPurged              = 0;
    FCB *       pfcbToPurge             = NULL;
    FCB *       pfcbToPurgeNext         = NULL;

    Assert( pinst->m_critFCBList.FOwner() );

    for ( pfcbToPurge = *( pinst->PpfcbAvailLRU() );
          pfcbToPurge != pfcbNil
            && cFCBPurged < cFCBToPurge
            && (!fThreshold || cFCBInspected < cMaxFCBInspected);
          pfcbToPurge = pfcbToPurgeNext )
    {
        Assert( pfcbToPurge->FInLRU() );

        Enforce( pfcbToPurge->FTypeTable() );

        pfcbToPurgeNext = pfcbToPurge->PfcbMRU();

        PERFOptDeclare( const ::TCE tce = pfcbToPurge->TCE() );

        //  update performance counter

        PERFOpt( cFCBAsyncScan.Inc( pinst, tce ) );

        if ( pfcbToPurge->FCheckFreeAndPurge_( ppib, fThreshold ) )
        {
            // pfcbPurge is now gone.

            pfcbToPurge = NULL;

            //  update performance counter

            PERFOpt( cFCBAsyncPurgeSuccess.Inc( pinst, tce ) );

            cFCBPurged++;
        }

        cFCBInspected++;
    }

    return ( 0 != cFCBPurged );
}

//  allocate an FCB for FCB::ErrCreate
//

ERR FCB::ErrAlloc_( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb )
{
    FCB     *pfcbCandidate = pfcbNil;
    INST    *pinst = PinstFromPpib( ppib );

    // ESE's FCB Scan and Purge Strategy (or "The FCB Who Loved Me") by SOMEONE
    //
    // FCBs Available 
    // FCBs Available Max (Preferred perf counter)
    // FCBs Quota (Maximum perf counter)
    // 
    // When either (A) the number of available FCBs surpasses the preferred threshold or (B) the 
    // allocated minus avail goes above the preferred threshold or (C) we come within 80% / 
    // FCloseToQuota_() we will start scanning the LRU FCB available list to purge FCBs. This 
    // operation is called a Threshold Purge/Scan and will do a bounded scan through the available 
    // FCBs, LRU to MRU, trying to free up to a certain number of FCBs (% of cached closed tables 
    // bounded by a min and a max) while also being bounded as to how many FCBs are inspected (also 
    // a % of the cached closed tables with a max and min). Before the lock is released, we will 
    // try to alloc right after we purged so we will re-use one of the released FCBs.
    //
    // If by some chance we will attempt to allocate an FCB we cannot (reached quota), and we
    // have not done a threshold scan/purge (which means available FCBs never surpassed preferred),
    // we will then do a scan/purge exactly as done for threshold scan/purge, but we call it an async
    // scan/purge.
    //
    // Finally, when we reach 80% of the FCB quota with allocated FCBs (available and/or used), we
    // will signal Version Store to do a cleanup. This should help cleanup pinned down available FCBs
    // waiting to be cleaned up. It is also worth mentioning that when 80% of the FCB quota for an INST
    // is taken, Version Store will start skipping cleaning up deletes as a desperate measure (see 
    // VER::FVERICleanDiscardDeletes). This should only happen in desperate cases - healthy servers
    // should not see FCB usage go this high.
    //
    // This set of values triggers scanning / purging EARLIER than triggering version store cleanup ... 
    // this is important to keep the case, or we could settle the average FCB usage close to quota and
    // then version store will continuously pitch deletes.
    //
    Assert( pinst->m_critFCBCreate.FOwner() );

    PERFOptDeclare( const TICK tickStartAlloc = TickOSTimeCurrent() );

    BOOL fCleanupTimedOut = fFalse;

    const BOOL fCloseToQuota = FCloseToQuota_( pinst );

    if ( fCloseToQuota )
    {
        //  signal version cleanup to try to free some FCB's,
        //  but do not wait for it.

        VERSignalCleanup( ppib );
    }

    if ( ( pinst->m_cFCBAvail > UlParam( pinst, JET_paramCachedClosedTables ) ) ||
            ( ( pinst->m_cFCBAlloc - pinst->m_cFCBAvail ) > UlParam( pinst, JET_paramCachedClosedTables ) ) ||
            ( fCloseToQuota ) )
    {
        //  if the number of cached table or allocated table meta-data entries 
        //  is greater than the preferred threshold or we are up against the 
        //  wall / close to quota, try to free enough cached entries to get
        //  the length below the threshold. don't do too many and don't walk
        //  the entire list if they are not cleanable -- if version store cleanup
        //  is pinning the entries this would cause performance problems

        pinst->m_critFCBList.Enter();

        if ( FScanAndPurge_( pinst, ppib, fTrue /* fThreshold */ ) )
        {
            // allocate right away while we're within the lock so we 
            // will pick up one of the purged instances.

            pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
            AssertSz( pfcbNil != pfcbCandidate || FRFSAnyFailureDetected(), "We just purged at least one FCB, so the allocation should always succeed." );
        }

        pinst->m_critFCBList.Leave();
    }

    //  try to allocate an FCB

    if ( pfcbNil == pfcbCandidate )
    {
        pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
    }

    if ( pfcbNil == pfcbCandidate )
    {
        //  we have allocated all of allowed FCBs
        //      and need to recycle one of them

        //  signal version cleanup to try to free some FCB's,
        //  and give it some time to take effect.

        VERSignalCleanup( ppib );

        pinst->m_critFCBCreate.Leave();

        //  ensure RCE clean was performed recently (if our wait times out, it
        //  means something is horribly wrong and blocking version cleanup)
        //
        fCleanupTimedOut = !PverFromPpib( ppib )->m_msigRCECleanPerformedRecently.FWait( cmsecAsyncBackgroundCleanup );

        PERFOpt( cFCBAllocWaitForRCEClean.Inc( pinst ) );

        pinst->m_critFCBCreate.Enter();

        if ( FInHashTable( ifmp, pgnoFDP ) )
        {
            //  someone beat us to it, as we 
            //  released the creation lock above.

            return ErrERRCheck( errFCBExists );
        }

        //  lock the FCB list

        pinst->m_critFCBList.Enter();

        // Now let's try to scan and purge, but only for a single
        // item as a last ditch attempt.

        if ( FScanAndPurge_( pinst, ppib, fFalse /* fThreshold */ ) )
        {
            // allocate right away while we're within the lock so we 
            // will pick up one of the purged instances.

            pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
            AssertSz( pfcbNil != pfcbCandidate || FRFSAnyFailureDetected(), "We just purged at least one FCB, so the allocation should always succeed." );
        }

        //  unlock the list

        pinst->m_critFCBList.Leave();

        //  retry to alloc an object for a last time

        if ( pfcbNil == pfcbCandidate )
        {
            pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
            if ( pfcbNil == pfcbCandidate )
            {
                PERFOpt( cFCBCacheAllocFailed.Inc( pinst ) );

                if ( fCleanupTimedOut )
                {
                    return ErrERRCheck( JET_errTooManyOpenTablesAndCleanupTimedOut );
                }

                return ErrERRCheck( JET_errTooManyOpenTables );
            }
        }
    }

    AtomicIncrement( (LONG *)&pinst->m_cFCBAlloc );

    //  update performance counter

    PERFOpt( cFCBCacheAlloc.Inc( pinst ) );
    PERFOpt( cFCBCacheAllocLatency.Add( pinst->m_iInstance, (DWORD_PTR)( TickOSTimeCurrent() - tickStartAlloc ) ) );

    //  return a ptr to the FCB

    *ppfcb = pfcbCandidate;

    return JET_errSuccess;
}

enum FCBPurgeFailReason : BYTE // fcbpfr
{
    fcbpfrInvalid                   = 0,
    fcbpfrConflict                  = 1,
    fcbpfrInUse                     = 2,
    fcbpfrSystemRoot                = 3,
    fcbpfrDeletePending             = 4,
    fcbpfrOutstandingVersions       = 5,
    fcbpfrIndexOutstandingConflict  = 6,
    fcbpfrIndexOutstandingActive    = 6,
    fcbpfrLvOutstandingConflict     = 7,
    fcbpfrLvOutstandingActive       = 7,
    fcbpfrTasksActive               = 8,
    fcbpfrCallbacks                 = 9,
    fcbpfrDomainDenyRead            = 10,
};

//  used by ErrAlloc_ to passively check an FCB to see if it can be recycled
//
//  this will only block on the hash-table -- and since hash-table locks
//      are brief, it shouldn't block too long; locking the FCB itself
//      will not block because we only "Try" the lock
//
//  if the FCB is available and we were able to lock it, we remove it from
//      the hash-table (making it completely invisible); then, ErrAlloc_
//      will purge the FCB
//
//  HOW IT BECOMES COMPLETELY INVISIBLE:
//      we locked the hash-table meaning no one could hash to the FCB
//      it also means that anyone else hashing to the FCB has already
//          declared themselves as an owner/waiter on the FCB lock
//      we then TRIED locked the FCB itself
//      since we got that lock without blocking, it means no one else was
//          an owner/waiter on the FCB lock meaning no one else was looking
//          at the FCB
//      thus, if the FCB has refcnt == 0, and no outstanding versions, and
//          etc... (everything that makes it free), we can purge the FCB

BOOL FCB::FCheckFreeAndPurge_(
    _In_ PIB *ppib,
    _In_ const BOOL fThreshold )
{
    INST            *pinst = PinstFromPpib( ppib );

    Assert( pinst->m_critFCBList.FOwner() );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( FInLRU() );

    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );
    BOOL            fAvail = fFalse;
    FCBPurgeFailReason fcbpfr = fcbpfrInvalid;

    //  lock the hash table to protect the SXW latch

    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

    //  try to acquire a write latch on the FCB

    if ( FLockTry_( LOCK_TYPE::ltWrite ) )
    {
        //  we have the write latch meaning there were no owners/waiters
        //      on the shared or the exclusive latch
        //  if the FCB is free, we can delete it from the hash-table and
        //      know that no one will be touching it.

        //  check the condition of this FCB

        bool fFCBPossiblyFree;

        if ( WRefCount() != 0 )
        {
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrInUse;
        }
        else if ( PgnoFDP() == pgnoSystemRoot )
        {
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrSystemRoot;
        }
        else if ( FDeletePending() )
        {
            //  FCB with pending "delete-table" is freed by RCEClean
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrDeletePending;
        }
        else if ( FDomainDenyRead( ppib ) )
        {
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrDomainDenyRead;
        }
        else if ( FOutstandingVersions_() )
        {
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrOutstandingVersions;
        }
        else if ( 0 != CTasksActive() )
        {
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrTasksActive;
        }
        else if ( FHasCallbacks_( pinst ) )
        {
            fFCBPossiblyFree = fFalse;
            fcbpfr = fcbpfrCallbacks;
        }
        else
        {
            fFCBPossiblyFree = fTrue;
        }

        FCB *pfcbLastLockedIndex = NULL;

        if ( fFCBPossiblyFree )
        {
            FCB *pfcbIndex;

            //  check each secondary-index FCB

            for ( pfcbIndex = this; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
            {
                if ( pfcbIndex->Pidb() != pidbNil )
                {
                    // Since there is no cursor on the table,
                    // no one should be doing a version check on the index
                    // or setting a current secondary index.
                    Assert( pfcbIndex->Pidb()->CrefVersionCheck() == 0 );
                    Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
                }

                // Lock index FCB to not allow recovery to get it
                // Lock is released either after marking FCB as unusable or if we decide to not purge
                if ( pfcbIndex != this )
                {
                    if ( !pfcbIndex->FLockTry_( LOCK_TYPE::ltWrite ) )
                    {
                        fcbpfr = fcbpfrIndexOutstandingConflict;
                        break;
                    }
                    pfcbLastLockedIndex = pfcbIndex;
                }
                else
                {
                    Assert( pfcbIndex->IsLocked_( LOCK_TYPE::ltWrite ) );
                }

                //  should never get the case where there are no table cursors and
                //  no outstanding versions on the secondary index, but there is
                //  an outstanding cursor on the secondary index
                //  UNDONE: How to check for this?
                if ( pfcbIndex->FOutstandingVersions_()
                    || pfcbIndex->CTasksActive() > 0
                    || pfcbIndex->WRefCount() > 0 )
                {
                    fcbpfr = fcbpfrIndexOutstandingActive;
                    break;
                }
            }
            //  note if pfcbNil == pfcbIndex, then we've finished all indices with nothing pinning FCB.

            //  check the LV-tree FCB

            if ( pinst->FRecovering() && Ptdb() == ptdbNil )
            {
                //  A FCB not used during read from passive (which would have a Ptdb()) should not have
                //  any indices, so we can assert we found none active.
                Assert( pfcbNil == pfcbIndex );
                fAvail = fTrue;
            }
            else if ( pfcbNil == pfcbIndex )
            {
                //  Success, we iterated all indices above, and all are not "active" in any way, not move
                //  on to check the LV FCB...

                Assert( Ptdb() != ptdbNil );

                FCB *pfcbLV = Ptdb()->PfcbLV();
                if ( pfcbNil == pfcbLV )
                {
                    fAvail = fTrue;
                }
                // Lock LV FCB to not allow recovery to get it
                // Lock is released either after marking FCB as unusable or if we decide to not purge
                else if ( pfcbLV->FLockTry_( LOCK_TYPE::ltWrite ) )
                {
                    if ( pfcbLV->FOutstandingVersions_()
                        || pfcbLV->CTasksActive() > 0
                        || pfcbLV->WRefCount() > 0 )
                    {
                        //  should never get the case where there are no table cursors and
                        //  no outstanding versions on the LV tree, but there is an
                        //  outstanding cursor on the LV tree
                        //  UNDONE: How to check for this?
                        fcbpfr = fcbpfrLvOutstandingActive;

                        pfcbLV->Unlock_( LOCK_TYPE::ltWrite );
                    }
                    else
                    {
                        fAvail = fTrue;
                    }
                }
                else
                {
                    fcbpfr = fcbpfrLvOutstandingConflict;
                }
            }
        }

        FCB *pfcbT;

        if ( !fAvail )
        {
            // unlock already locked indices since we decided to not purge
            if ( pfcbLastLockedIndex != NULL )
            {
                for ( pfcbT = PfcbNextIndex(); pfcbT != pfcbLastLockedIndex; pfcbT = pfcbT->PfcbNextIndex() )
                {
                    pfcbT->Unlock_( LOCK_TYPE::ltWrite );
                }
                Assert( pfcbT == pfcbLastLockedIndex );
                pfcbT->Unlock_( LOCK_TYPE::ltWrite );
            }
        }
        else // fAvail
        {
            //  the FCB is ready to be purged

            //  delete the FCB from the hash table

            errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );

            //  FCB must be in the hash table unless FDeleteCommitted is set

            Assert( errFCBHash == FCBHash::ERR::errSuccess ||
                    ( errFCBHash == FCBHash::ERR::errNoCurrentEntry && FDeleteCommitted() ) );

            //  mark all children as uninitialized so no one can hash to them
            //
            //  this will prevent the following concurrency hole:
            //      thread 1: table A is being pitched
            //      thread 1: table A's table-FCB is removed from the hash-table
            //      thread 2: table A gets reloaded from disk
            //      thread 2: when loading table A's secondary index FCBs, we see
            //                that they alrady exist (not yet purged by thread 1)
            //                and try to link to them even though thread 1 will
            //                soon be purging them
            //  NOTE: we have already locked all the FCBs above

            pfcbT = PfcbNextIndex();
            while ( pfcbT != pfcbNil )
            {
                Assert( 0 == pfcbT->WRefCount() );
                pfcbT->CreateCompleteErr( errFCBUnusable );
                // release the lock taken above
                pfcbT->Unlock_( LOCK_TYPE::ltWrite );
                pfcbT = pfcbT->PfcbNextIndex();
            }
            if ( Ptdb() &&
                 Ptdb()->PfcbLV() )
            {
                Assert( 0 == Ptdb()->PfcbLV()->WRefCount() );
                Ptdb()->PfcbLV()->CreateCompleteErr( errFCBUnusable );
                // release the lock taken above
                Ptdb()->PfcbLV()->Unlock_( LOCK_TYPE::ltWrite );
            }
        }

#ifdef DEBUG
        // Make sure all index latches have been released
        for ( pfcbT = PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltWrite ) );
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltShared ) );
        }
#endif

        //  release the write latch
        Unlock_( LOCK_TYPE::ltWrite );
    }
    else // !if ( errSXWLatch == CSXWLatch::errSuccess )
    {
        //  update performance counter

        fcbpfr = fcbpfrConflict;
    }

    Assert( ( fAvail && fcbpfr == fcbpfrInvalid ) ||
            ( !fAvail && fcbpfr != fcbpfrInvalid ) );
    if ( !fAvail )
    {
        Assert( fcbpfr != fcbpfrInvalid ); // already checked above ...
        //  Is the TCE() member safe here?  I think as long as the lockFCBHash is still locked, this
        //  FCB can't be re-used as it hasn't been put in the avail / purged list below.
        PERFOpt( cFCBAsyncPurgeFail.Inc( pinst, TCE() ) );
        ETFCBPurgeFailure( pinst->m_iInstance, (BYTE)( 0x1 /* for async */ | ( fThreshold ? 0x2 : 0x0 ) ), fcbpfr, TCE() );
    }

    //  unlock hash table

    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

    if ( fAvail )
    {
        FCB *pfcbT;

        //  prepare the children of this FCB for purge
        //  NOTE: no one will be touching them because we have marked them
        //        all as being uninitialized due to an error

        pfcbT = PfcbNextIndex();
        while ( pfcbT != pfcbNil )
        {
            Assert( !pfcbT->FInitialized() );
            Assert( errFCBUnusable == pfcbT->ErrErrInit() );
            pfcbT->PrepareForPurge( fFalse );
            pfcbT = pfcbT->PfcbNextIndex();
        }
        if ( Ptdb() )
        {
            if ( Ptdb()->PfcbLV() )
            {
                Assert( !Ptdb()->PfcbLV()->FInitialized() );
                Assert( errFCBUnusable == Ptdb()->PfcbLV()->ErrErrInit() );
                Ptdb()->PfcbLV()->PrepareForPurge( fFalse );
            }

            //  remove the entry for this table from the catalog hash
            //  NOTE: there is the possibility that after this FCB is removed from the FCB hash
            //          someone could get to its pgno/objid from the catalog hash; this is ok
            //          because the pgno/objid are still valid (they will be until the space
            //          they occupy is released to the space-tree)

            if ( FCATHashActive( pinst ) )
            {
                CHAR szTable[JET_cbNameMost+1];

                //  catalog hash is active so we can delete the entry

                //  read the table name

                EnterDML();
                OSStrCbCopyA( szTable, sizeof(szTable), Ptdb()->SzTableName() );
                LeaveDML();

                //  delete the hash-table entry

                CATHashIDelete( this, szTable );
            }
        }

        //  purge it!
        //
        Assert( FTypeTable() );
        Purge( fFalse );
    }

    return fAvail;
}


//  remove all RCEs and close all cursors on this FCB

INLINE VOID FCB::CloseAllCursorsOnFCB_( const BOOL fTerminating )
{
    if ( fTerminating )
    {
        //  version-clean was already attempted as part of shutdown
        //  ignore any outstanding versions and continue

        m_prceNewest = prceNil;
        m_prceOldest = prceNil;
    }

    Assert( PrceNewest() == prceNil );
    Assert( PrceOldest() == prceNil );

    //  close all cursors on this FCB

    FUCBCloseAllCursorsOnFCB( ppibNil, this );
}


//  close all cursors on this FCB and its children FCBs

VOID FCB::CloseAllCursors( const BOOL fTerminating )
{
    // we are not set to "terminating" - which will ignore the existing links to the version store -
    // only is the instance unavailable or is going away (fSTInitNotDone, m_fTermInProgress is not set
    // if we recover and replay a JetTerm so it can't be used here)
    //
    Assert( !fTerminating ||
            ( ( PinstFromIfmp( Ifmp() )->m_fSTInit == fSTInitNotDone )||
                PinstFromIfmp( Ifmp() )->FInstanceUnavailable( ) ) );

#ifdef DEBUG
    if ( FTypeDatabase() )
    {
        Assert( PgnoFDP() == pgnoSystemRoot );
        Assert( Ptdb() == ptdbNil );
        Assert( PfcbNextIndex() == pfcbNil );
    }
    else if ( FTypeTable() )
    {
        Assert( PgnoFDP() > pgnoSystemRoot );
        if ( Ptdb() == ptdbNil )
        {
            INST *pinst = PinstFromIfmp( Ifmp() );
            Assert( pinst->FRecovering()
                || ( fTerminating && g_rgfmp[Ifmp()].FHardRecovery() ) );
        }
    }
    else
    {
        Assert( FTypeTemporaryTable() );
        Assert( PgnoFDP() > pgnoSystemRoot );
        Assert( Ptdb() != ptdbNil );
        Assert( PfcbNextIndex() == pfcbNil );
    }
#endif  //  DEBUG

    if ( Ptdb() != ptdbNil )
    {
        FCB * const pfcbLV = Ptdb()->PfcbLV();
        if ( pfcbNil != pfcbLV )
        {
            pfcbLV->CloseAllCursorsOnFCB_( fTerminating );
        }
    }

    FCB *pfcbNext;
    for ( FCB *pfcbT = this; pfcbNil != pfcbT; pfcbT = pfcbNext )
    {
        pfcbNext = pfcbT->PfcbNextIndex();
        Assert( pfcbNil == pfcbNext
            || ( FTypeTable()
                && pfcbNext->FTypeSecondaryIndex()
                && pfcbNext->PfcbTable() == this ) );

        pfcbT->CloseAllCursorsOnFCB_( fTerminating );
    }
}


//  finish cleaning up an FCB and release it
//
//  NOTE: this assumes the FCB has been locked for purging via
//      FCheckFreeAndPurge_ or PrepareForPurge

VOID FCB::Delete_( INST *pinst )
{
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    //  this FCB should no longer be in either of the lists

#if defined( DEBUG ) && defined( SYNC_DEADLOCK_DETECTION )
    const BOOL fDEBUGLockList = pinst->m_critFCBList.FNotOwner();
    if ( fDEBUGLockList )
    {
        pinst->m_critFCBList.Enter();
    }
    Assert( pinst->m_critFCBList.FOwner() );
    Assert( !FInList() );
    Assert( pfcbNil == PfcbNextList() );
    Assert( pfcbNil == PfcbPrevList() );
    Assert( !FInLRU() );
    Assert( pfcbNil == PfcbMRU() );
    Assert( pfcbNil == PfcbLRU() );
    if ( fDEBUGLockList )
    {
        pinst->m_critFCBList.Leave();
    }
#endif  //  DEBUG && SYNC_DEADLOCK_DETECTION

    //  this FCB should not be in the hash table

#ifdef DEBUG
    FCB *pfcbT;
    Assert( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) || pfcbT != this );
#endif

    //  verify the contents of this FCB

    Assert( Ptdb() == ptdbNil );
    Assert( Pidb() == pidbNil );
    Assert( WRefCount() == 0 );
    Assert( FWRefCountOK_() );
    Assert( PrceNewest() == prceNil );
    Assert( PrceOldest() == prceNil );

    //  release the FCB

    delete this;

    AtomicDecrement( (LONG *)&pinst->m_cFCBAlloc );

    //  update performance counter

    PERFOpt( cFCBCacheAlloc.Dec( pinst ) );
}


//  force the FCB to disappear such that we can purge it without fear that
//      we will be pulling the FCB out from underneath anyone else
//
//  the FCB disappears by being removed from the hash-table using the same
//      locking protocol as FCheckFreeAndPurge_; however, this routine will
//      block when locking the FCB to make sure that all other owner/waiters
//      will be done (we will be the last owner/waiter)

VOID FCB::PrepareForPurge( const BOOL fPrepareChildren )
{
    INST            *pinst = PinstFromIfmp( Ifmp() );
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );

    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );

#if defined( DEBUG ) && defined( SYNC_DEADLOCK_DETECTION )

    //  lock the FCB list

    const BOOL fDBGONLYLockList = pinst->m_critFCBList.FNotOwner();

    if ( fDBGONLYLockList )
    {
        pinst->m_critFCBList.Enter();
    }

    if ( FInList() )
    {

        //  verify that this FCB is in the global list

        Assert( pinst->m_pfcbList != pfcbNil );

        FCB *pfcbT = pinst->m_pfcbList;
        while ( pfcbT->PgnoFDP() != PgnoFDP() || pfcbT->Ifmp() != Ifmp() )
        {
            Assert( pfcbNil != pfcbT );
            pfcbT = pfcbT->PfcbNextList();
        }

        if ( pfcbT != this )
        {
            Assert( FFMPIsTempDB( pfcbT->Ifmp() ) );

            //  Because we delete the table at close time when it is from Temp DB,
            //  the pgnoFDP may be reused by other threads several time already
            //  and the Purge table for each thread may not be called in LRU order.
            //  So continue search for it.

            while ( pfcbT->PgnoFDP() != PgnoFDP() || pfcbT->Ifmp() != Ifmp() || pfcbT != this )
            {
                Assert( pfcbNil != pfcbT );
                pfcbT = pfcbT->PfcbNextList();
            }

            //  Must be found

            Assert( pfcbT == this );
        }
    }

    //  unlock the FCB list

    if ( fDBGONLYLockList )
    {
        pinst->m_critFCBList.Leave();
    }

#endif  //  DEBUG && SYNC_DEADLOCK_DETECTION
    //  update performance counter

    PERFOpt( cFCBSyncPurge.Inc( pinst ) );

#ifdef DEBUG
    DWORD cStalls = 0;
#endif  //  DEBUG

RetryLock:

    //  lock the hash table to protect the SXW latch

    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

    //  try to acquire a write latch on the FCB
    if ( !FLockTry_( LOCK_TYPE::ltWrite ) )
    {
        //  unlock hash table
        pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

        //  we were unable to get the write-latch
        //  someone else is touching with this FCB

#ifdef DEBUG
        cStalls++;
        Assert( cStalls < 100 );
#endif  //  DEBUG

        //  update performance counter

        PERFOpt( cFCBSyncPurgeStalls.Inc( pinst ) );

        //  wait

        UtilSleep( 10 );

        //  try to lock the FCB again

        goto RetryLock;
    }        

    //  we got the lock.
    //  we can now be sure that we are the only ones who can see this FCB

    //  remove the entry from the hash table regardless of whether or not
    //      we got the write latch

    errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );

    //  FCB must be in the hash table unless FDeleteCommitted is set

    Assert( errFCBHash == FCBHash::ERR::errSuccess || FDeleteCommitted() );

    //  mark all children as uninitialized so no one can hash to them
    //
    //  this will prevent the following concurrency hole:
    //      thread 1: table A is being pitched
    //      thread 1: table A's table-FCB is removed from the hash-table
    //      thread 2: table A gets reloaded from disk
    //      thread 2: when loading table A's secondary index FCBs, we see
    //                that they alrady exist (not yet purged by thread 1)
    //                and try to link to them even though thread 1 will
    //                soon be purging them
    //  NOTE: since we have the table-FCB all to ourselves, no one else
    //        should be trying to load the table or touch any of its
    //        children FCBs -- thus, we do not need to lock them!

    if ( fPrepareChildren )
    {
        FCB *pfcbT;

        pfcbT = PfcbNextIndex();
        while ( pfcbT != pfcbNil )
        {
            pfcbT->Lock();
            pfcbT->CreateCompleteErr( errFCBUnusable );
            pfcbT->Unlock_( LOCK_TYPE::ltWrite );
            pfcbT = pfcbT->PfcbNextIndex();
        }
        if ( Ptdb() )
        {
            if ( Ptdb()->PfcbLV() )
            {
                Ptdb()->PfcbLV()->Lock();
                Ptdb()->PfcbLV()->CreateCompleteErr( errFCBUnusable );
                Ptdb()->PfcbLV()->Unlock_( LOCK_TYPE::ltWrite );
            }
        }
    }

    //  unlock hash table

    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

    //  we have the write latch meaning there were no owners/waiters
    //      on the shared or the exclusive latch

    //  release the write latch

    Unlock_( LOCK_TYPE::ltWrite );

    //  the FCB is now invisible and so are its children FCBs

    if ( fPrepareChildren )
    {
        if ( FTypeDatabase() || FTypeTable() || FTypeTemporaryTable() )
        {
            FCB *pfcbT;

            pfcbT = PfcbNextIndex();
            while ( pfcbT != pfcbNil )
            {
                Assert( !pfcbT->FInitialized() );
                Assert( errFCBUnusable == pfcbT->ErrErrInit() );
                pfcbT->PrepareForPurge( fFalse );
                pfcbT = pfcbT->PfcbNextIndex();
            }
            if ( Ptdb() )
            {
                if ( Ptdb()->PfcbLV() )
                {
                    Assert( !Ptdb()->PfcbLV()->FInitialized() );
                    Assert( errFCBUnusable == Ptdb()->PfcbLV()->ErrErrInit() );
                    Ptdb()->PfcbLV()->PrepareForPurge( fFalse );
                }
            }
        }
    }

    if ( FTypeTable() )
    {
        if ( Ptdb() )
        {

            //  remove the entry for this table from the catalog hash
            //  NOTE: there is the possibility that after this FCB is removed from the FCB hash
            //          someone could get to its pgno/objid from the catalog hash; this is ok,
            //          because the pgno/objid are still valid -- they become invalid AFTER the
            //          space they occupy has been released (ErrSPFreeFDP) and that NEVER happens
            //          until after the FCB is prepared for purge [ie: this function is called]

            if ( FCATHashActive( pinst ) )
            {
                CHAR szTable[JET_cbNameMost+1];

                //  catalog hash is active so we can delete the entry

                //  read the table name

                EnterDML();
                OSStrCbCopyA( szTable, sizeof(szTable), Ptdb()->SzTableName() );
                LeaveDML();

                //  delete the hash-table entry

                CATHashIDelete( this, szTable );
            }
        }
    }
}


//  purge any FCB that has previously locked down via PrepareForPurge or
//      FCheckFreeAndPurge_

VOID FCB::Purge( const BOOL fLockList, const BOOL fTerminating )
{
    INST    *pinst = PinstFromIfmp( Ifmp() );
#ifdef DEBUG
    FCB     *pfcbInHash;
#endif  //  DEBUG
    FCB     *pfcbT;
    FCB     *pfcbNextT;

    if ( fLockList )
    {
        Assert( pinst->m_critFCBList.FNotOwner() );
    }
    else
    {
        //  either we already have the list locked,
        //  or this is an error during FCB creation,
        //  in which case it's guaranteed not to be
        //  in the avail or global lists
        Assert( pinst->m_critFCBList.FOwner()
                || ( !FInLRU() && !FInList() ) );
    }
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    //  the refcount should be zero by now
    //      (enforce this in retail builds because those are the only
    //       places this condition will likely arise)

    Enforce( WRefCount() == 0 );

    //  this FCB should not be in the catalog hash table

#ifdef DEBUG
    if ( FTypeTable() && Ptdb() != ptdbNil )
    {
        CHAR    szName[JET_cbNameMost+1];
        PGNO    pgnoT;
        OBJID   objidT;

        EnterDML();
        OSStrCbCopyA( szName, sizeof(szName), Ptdb()->SzTableName() );
        LeaveDML();
        Assert( !FCATHashLookup( Ifmp(), szName, &pgnoT, &objidT ) );
    }
#endif

    //  this FCB should not be in the FCB hash table

    Assert( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbInHash ) || pfcbInHash != this );

    //  verify the members of this FCB

    Assert( WRefCount() == 0 );
    Assert( FWRefCountOK_() );
    Assert( PrceOldest() == prceNil );
    Assert( PrceNewest() == prceNil );

    if ( FTypeTable() )
    {

        //  unlink the secondary-index chain

        pfcbT = PfcbNextIndex();
        SetPfcbNextIndex( pfcbNil );
        while ( pfcbT != pfcbNil )
        {

            //  ASSUME: no one will be touching these FCBs so we do not
            //          need to lock pfcbT for each one

            // No one should have this locked right now.
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltWrite ) );
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltShared ) );

            //  we cannot make assumption about the initialized state of the FCB
            //  if we are purging an FCB that was lingering due to an initialization error,
            //      the error-code could be anything! also, the member data could be garbled!
            //  the assumption below only accounts for initialized FCBs and FCBs which were
            //      prepared-for-purge
            //
            // Assert( pfcbT->FInitialized() || errFCBUnusable == pfcbT->ErrErrInit() );

            Assert( !pfcbT->FInitialized() || pfcbT->FTypeSecondaryIndex() );
            Assert( ptdbNil == pfcbT->Ptdb() );
            Assert( !pfcbT->FInitialized() || pfcbT->PfcbTable() == this );
            Assert( !FInHashTable( pfcbT->Ifmp(), pfcbT->PgnoFDP(), &pfcbInHash ) ||
                    pfcbInHash != pfcbT );

            // Return the memory used. No need to explicitly free index
            // name or idxseg array, since memory pool will be freed when
            // TDB is deleted below.
            if ( pidbNil != pfcbT->Pidb() )
            {
                pfcbT->ReleasePidb( fTerminating );
            }
            pfcbNextT = pfcbT->PfcbNextIndex();
            pfcbT->Delete_( pinst );
            pfcbT = pfcbNextT;
        }
    }
    else
    {
        Assert( !FTypeDatabase()        || pfcbNil == PfcbNextIndex() );
        Assert( !FTypeTemporaryTable()  || pfcbNil == PfcbNextIndex() );
        Assert( !FTypeSort()            || pfcbNil == PfcbNextIndex() );
        Assert( !FTypeLV()              || pfcbNil == PfcbNextIndex() );
    }

    //  delete the TDB

    if ( Ptdb() != ptdbNil )
    {
        Assert( FTypeTable() || FTypeTemporaryTable() );

        //  delete the LV-tree FCB

        pfcbT = Ptdb()->PfcbLV();
        Ptdb()->SetPfcbLV( pfcbNil );
        if ( pfcbT != pfcbNil )
        {

            //  verify the LV-tree FCB

//  we cannot make assumptions about the initialized state of the FCB
//  the FCB could have failed to be initialized because of a disk error
//      and it was left lingering meaning its member data would be garbled!
//
//          Assert( pfcbT->FInitialized() );

            Assert( !pfcbT->FInitialized() || pfcbT->FTypeLV() );
            Assert( !pfcbT->FInitialized() || pfcbT->PfcbTable() == this );
            Assert( !FInHashTable( pfcbT->Ifmp(), pfcbT->PgnoFDP(), &pfcbInHash ) ||
                    pfcbInHash != pfcbT );

            pfcbT->Delete_( pinst );
        }

        if ( Ptdb()->PfcbTemplateTable() != NULL )
        {
            Ptdb()->PfcbTemplateTable()->DecrementRefCountAndUnlink_( pfucbNil, fLockList );
            Ptdb()->SetPfcbTemplateTable( NULL );
        }
        delete Ptdb();
        SetPtdb( ptdbNil );
    }

    //  unlink the IDB

    if ( Pidb() != pidbNil )
    {
        Assert( FTypeTable() || FTypeTemporaryTable() );

        // No need to explicitly free index name or idxseg array, since
        // memory pool was freed when TDB was deleted above.
        ReleasePidb();
    }

    if ( fLockList )
    {

        //  lock the list

        pinst->m_critFCBList.Enter();
    }


    if ( FInLRU() )
    {
        //  Remove this FCB from the avail list.
#ifdef DEBUG
        RemoveAvailList_( fTrue );
#else   //  !DEBUG
        RemoveAvailList_();
#endif  //  DEBUG
    }

    if ( FInList() )
    {

        //  remove this FCB from the global list

        RemoveList_();
    }

    if ( fLockList )
    {

        //  unlock the list

        pinst->m_critFCBList.Leave();
    }

    //  delete this FCB

    Delete_( pinst );
}


//  returns fTrue when this FCB has temporary callbacks

INLINE BOOL FCB::FHasCallbacks_( INST *pinst )
{
    if ( pinst->FRecovering() || BoolParam( pinst, JET_paramDisableCallbacks ) )
    {
        return fFalse;
    }

    const CBDESC *pcbdesc = m_ptdb->Pcbdesc();
    while ( pcbdesc != NULL )
    {
        if ( !pcbdesc->fPermanent )
        {
            return fTrue;
        }
        pcbdesc = pcbdesc->pcbdescNext;
    }

    //  no callbacks, or all callbacks are in the catalog

    return fFalse;
}



//  returns fTrue when this FCB has at least one outstanding version

INLINE BOOL FCB::FOutstandingVersions_()
{
    //  if we're checking the RCE list with the intent to free the FCB, we must grab
    //  the critical section first, otherwise we can get into the state where version
    //  cleanup has freed the last RCE for the FCB, but has yet to leave the critical
    //  section when we suddenly free the FCB (and hence the critical section) out
    //  from underneath him.
    ENTERCRITICALSECTION    enterCritRCEList( &CritRCEList() );
    return ( prceNil != PrceOldest() );
}


//  Frees all FCBs belonging to either:
//    - A particular ESE instance (ifmp == ifmpNil && pgnoFDP == pgnoNull).
//    - A particular database (ifmp != ifmpNil && pgnoFDP == pgnoNull).
//    - A particular object/tree (ifmp != ifmpNil && pgnoFDP != pgnoNull). It must be a root object
//      (i.e., a table).
//      WARNING: If choosen object/tree is a template, all of the derived objects are also purged.

VOID FCB::PurgeObjects_( INST* const pinst, const IFMP ifmp, const PGNO pgnoFDP, const BOOL fTerminating )
{
    Assert( pinst != pinstNil );
    Assert( ( ifmp == ifmpNil ) || ( pinst == PinstFromIfmp( ifmp ) ) );
    Assert( ( ifmp != ifmpNil ) || ( pgnoFDP == pgnoNull ) );
    Assert( ( ifmp != ifmpNil ) || fTerminating );

    OnDebug( const BOOL fShrinking = ( ifmp != ifmpNil ) && g_rgfmp[ifmp].FShrinkIsRunning() );
    OnDebug( const BOOL fReclaimingLeaks = ( ifmp != ifmpNil ) && g_rgfmp[ifmp].FLeakReclaimerIsRunning() );

    //  WARNING: this function is very expensive if the engine has lots of open tables, so be careful when
    //  calling it to purge one specific table/object. Currently, DB shrink is the only consumer of this mode.
    Expected( ( pgnoFDP == pgnoNull ) || fShrinking || fReclaimingLeaks );

    OnDebug( const BOOL fDetaching = ( ifmp != ifmpNil ) && g_rgfmp[ifmp].FDetachingDB() );

    // Because this may tear down user handles indelicately, this is only safe during term or "single threaded" periods.
    // Be careful when adding a new consumer of this function outside of these.
    Assert( fTerminating || fDetaching || fShrinking || g_fRepair || fReclaimingLeaks );

    Expected( !fShrinking || ( !fTerminating && !fDetaching && !g_fRepair ) );
    Expected( !fTerminating || ( pgnoFDP == pgnoNull ) );
    Expected( !fDetaching || ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) ) );
    Expected( !g_fRepair || ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) ) );

    FCB* pfcbNext = pfcbNil;
    FCB* pfcbThis = pfcbNil;

    Assert( pinst->m_critFCBList.FNotOwner() );

    //  lock the FCB list

    pinst->m_critFCBList.Enter();

    //  Scan the list for any FCB which matches the restrictions.
    //  Purge in 2 steps: first, all non-template FCBs, releasing any reference on template FCBs

    pfcbThis = pinst->m_pfcbList;
    while ( pfcbThis != pfcbNil )
    {
        //  get the next FCB

        pfcbNext = pfcbThis->PfcbNextList();

        if ( !pfcbThis->FTemplateTable() &&
             (
               //  purge all FCBs of the instance.
               ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull ) ) ||

               //  purge all FCBs of a given databse.
               ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) ) ||

                //  purge FCB which has a specific pgnoFDP or is a derived object of a template.
               ( ( ifmp != ifmpNil ) && ( pgnoFDP != pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) &&
                 (
                   ( pfcbThis->PgnoFDP() == pgnoFDP ) ||  // object itself matches.
                   ( pfcbThis->FDerivedTable() &&
                     ( pfcbThis->Ptdb() != ptdbNil ) &&
                     ( pfcbThis->Ptdb()->PfcbTemplateTable() != pfcbNil ) &&
                     ( pfcbThis->Ptdb()->PfcbTemplateTable()->PgnoFDP() == pgnoFDP )  // template matches.
                   )
                 )
               )
             )
           )
        {
            //  this FCB matches the filter

            Assert( pfcbThis->FTypeDatabase()       ||
                    pfcbThis->FTypeTable()          ||
                    pfcbThis->FTypeTemporaryTable() );

            //  Mark this FCB for purging
            pfcbThis->PrepareForPurge();

            //  purge this FCB

            pfcbThis->CloseAllCursors( fTerminating );
            pfcbThis->Purge( fFalse, fTerminating );
        }

        //  move next

        pfcbThis = pfcbNext;
    }

    //  Now, purge template FCBs.

    pfcbThis = pinst->m_pfcbList;
    while ( pfcbThis != pfcbNil )
    {
        //  get the next FCB

        pfcbNext = pfcbThis->PfcbNextList();

        if ( //  purge all FCBs of the instance.
             ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull ) ) ||

             //  purge all FCBs of a given databse.
             ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) ) ||

             //  purge FCB which has a specific pgnoFDP.
             ( ( ifmp != ifmpNil ) && ( pgnoFDP != pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) && ( pfcbThis->PgnoFDP() == pgnoFDP ) )
           )
        {
            //  this FCB belongs to the IFMP that is being detached

            Assert( pfcbThis->FTypeDatabase()       ||
                    pfcbThis->FTypeTable()          ||
                    pfcbThis->FTypeTemporaryTable() );
            Assert( pfcbThis->FTemplateTable() );

            //  Mark this FCB for purging
            pfcbThis->PrepareForPurge();

            //  purge this FCB

            pfcbThis->CloseAllCursors( fTerminating );
            pfcbThis->Purge( fFalse, fTerminating );
        }

        //  move next

        pfcbThis = pfcbNext;
    }

    //  reset the list, just in case

    Expected( ( ifmp != ifmpNil ) || ( pgnoFDP != pgnoNull ) || ( pinst->m_pfcbList == pfcbNil ) );
    if ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull )  )
    {
        pinst->m_pfcbList = pfcbNil;
    }

    //  unlock the FCB list

    pinst->m_critFCBList.Leave();

#ifdef DEBUG
    //  make sure all entries which are expected to be gone are indeed gone

    if ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull ) )
    {
        for ( DBID dbidT = dbidMin; dbidT < dbidMax; dbidT++ )
        {
            const IFMP  ifmpT   = pinst->m_mpdbidifmp[dbidT];
            if ( ifmpT < g_ifmpMax )
            {
                //  make sure all catalog-hash entries are gone for all IFMPs
                //  used by the current instance
                CATHashAssertCleanIfmp( ifmpT );
            }
        }
    }
    else
    {
        CATHashAssertCleanIfmpPgnoFDP( ifmp, pgnoFDP );
    }
#endif  //  DEBUG
}


//  Free all FCBs belonging to a particular database and a particular tree (matching IFMP + pgnoFDP).
//  WARNING: this function is very expensive if the engine has lots of open tables, so be careful when
//  calling it to purge one specific table/object. Currently, DB shrink is the only consumer of this mode.

VOID FCB::PurgeObject( const IFMP ifmp, const PGNO pgnoFDP )
{
    Assert( ifmp != ifmpNil );
    Assert( pgnoFDP != pgnoNull );
    FCB::PurgeObjects_( PinstFromIfmp( ifmp ), ifmp, pgnoFDP, fFalse /* fTerminating */ );
}


//  Free all FCBs belonging to a particular database (matching IFMP).

VOID FCB::PurgeDatabase( const IFMP ifmp, const BOOL fTerminating )
{
    Assert( ifmp != ifmpNil );
    FCB::PurgeObjects_( PinstFromIfmp( ifmp ), ifmp, pgnoNull, fTerminating );
}


//  Free all FCBs (within the current instance).

VOID FCB::PurgeAllDatabases( INST* const pinst )
{
    Assert( pinst != pinstNil );
    FCB::PurgeObjects_( pinst, ifmpNil, pgnoNull, fTrue /* fTerminating */ );
}


//  insert this FCB into the hash-table
//      (USED ONLY BY SCBInsertHashTable!!!)

VOID FCB::InsertHashTable()
{
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) || FTypeSort() );;

    //  make sure this FCB is not in the hash-table

    Assert( !FInHashTable( Ifmp(), PgnoFDP() ) );

    INST            *pinst = PinstFromIfmp( Ifmp() );
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );
    FCBHashEntry    entryFCBHash( PgnoFDP(), this );

    //  lock the key

    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

    //  insert the entry

    errFCBHash = pinst->m_pfcbhash->ErrInsertEntry( &lockFCBHash, entryFCBHash );
    Assert( errFCBHash == FCBHash::ERR::errSuccess );

    //  unlock the key

    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

#ifdef DEBUG

    //  this FCB should now be in the hash-table

    FCB *pfcbT;
    Assert( FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) );
    Assert( pfcbT == this );
#endif  //  DEBUG
}


//  delete this FCB from the hash-table

VOID FCB::DeleteHashTable()
{
#ifdef DEBUG

    //  make sure this FCB is in the hash-table

    FCB *pfcbT;
    Assert( FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) );
    Assert( pfcbT == this );
#endif  //  DEBUG

    INST            *pinst = PinstFromIfmp( Ifmp() );
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );

    //  lock the key

    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

    //  delete the entry

    errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );
    Assert( errFCBHash == FCBHash::ERR::errSuccess );

    //  unlock the key

    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

    //  make sure this FCB is not in the hash-table

    Assert( !FInHashTable( Ifmp(), PgnoFDP() ) );
}

//  link an FUCB to this FCB

VOID FCB::CheckFCBLockingForLink_()
{
#ifdef DEBUG
    INST *pinst = PinstFromIfmp( Ifmp() );

    // Call this before you take any locks.
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( pinst->m_critFCBList.FNotOwner() );

    if ( FFMPIsTempDB( Ifmp() ) && !FTypeNull() )
    {
        if ( PgnoFDP() == pgnoSystemRoot )
        {
            Assert( FTypeDatabase() );
        }
        else
        {
            Assert( FTypeTemporaryTable() || FTypeSort() || FTypeLV() );
        }
    }

    //  if this FCB has a refcount of 0, it should not be purgeable
    //  otherwise, it could be purged while we are trying to link an FUCB to it!

    pinst->m_critFCBList.Enter();

    if ( FInLRU() )
    {

        Lock();
        //  FCB is in the avail-LRU list meaning it has the potential to be purged
        //  However, it can't be being purged right now since that requires holding
        //  pinst->m_critFCBList, and we're holding that right now.

        if ( WRefCount() == 0 &&
             PgnoFDP() != pgnoSystemRoot &&
             !FDeletePending() &&           //  FCB with pending "delete-table" is freed by RCEClean
             !FDeleteCommitted() &&         //  FCB with pending rollback of "create-table" is freed by VERIUndoCreateTable
             //!FDomainDenyRead( ppib ) &&
             !FOutstandingVersions_() &&
             0 == CTasksActive() &&
             !FHasCallbacks_( pinst ) )
        {

            //  FCB is ready to be purged -- this is very bad
            //
            //  whoever the caller is, they are calling Link() on an FCB which
            //      could disappear at any moment because its in the avail-LRU
            //      list and its purge-able
            //
            //  this is a bug in the caller...

            //  NOTE: this may have been reached in error!
            //        if the FCB's FDomainDenyRead counter is set, this is a bad
            //           assert (I couldn't call FDomainDenyRead without a good ppib)

            //  FCB should not be pinned down in LRU when it is being linked.
            AssertTrack( fFalse, "LinkingPurgeableFcb" );
        }
        Unlock_( LOCK_TYPE::ltWrite );
    }

    pinst->m_critFCBList.Leave();
#endif  //  DEBUG
}

ERR FCB::ErrLink( FUCB *pfucb )
{
    Assert( pfucb != pfucbNil );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    // Examine this to make sure we're correctly locking.
    CheckFCBLockingForLink_();

    // The FDP is about to be deleted. No operations should be allowed on such a table unless requested to skip the error.
    if ( FRevertedFDPToDelete() && !FPpibAllowRBSFDPDeleteReadByMe( pfucb->ppib ) )
    {
        return ErrERRCheck( JET_errRBSFDPToBeDeleted );
    }

    return ErrIncrementRefCountAndLink_( pfucb );
}

ERR FCB::ErrLinkReserveSpace()
{
    Assert( IsLocked_( LOCK_TYPE::ltWrite ) );

    FucbList().LockForModification();
    COLL::CInvasiveConcurrentModSet<FUCB, FUCBOffsetOfIAE>::ERR err = FucbList().ErrInsertReserveSpace();
    Assert( FWRefCountOK_() );
    FucbList().UnlockForModification();
    if ( COLL::CInvasiveConcurrentModSet<FUCB, FUCBOffsetOfIAE>::ERR::errSuccess != err )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    return JET_errSuccess;
}



//  ================================================================
VOID FCB::AttachRCE( RCE * const prce )
//  ================================================================
//
//  Add a newly created RCE to the head of the RCE queue. Increments
//  the version refcount on the FCB
//
//-
{
    Assert( CritRCEList().FOwner() );

    Assert( prce->Ifmp() == Ifmp() );
    Assert( prce->PgnoFDP() == PgnoFDP() );

    if ( prce->Oper() != operAddColumn )
    {
        // UNDONE: Don't hold pfcb->CritFCB() over versioning of AddColumn
        Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
        Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    }

    Assert( (prceNil == m_prceNewest) == (prceNil == m_prceOldest) );

    prce->SetPrceNextOfFCB( prceNil );
    prce->SetPrcePrevOfFCB( m_prceNewest );
    if( prceNil != m_prceNewest )
    {
        Assert( m_prceNewest->PrceNextOfFCB() == prceNil );
        m_prceNewest->SetPrceNextOfFCB( prce );
    }
    m_prceNewest = prce;
    if( prceNil == m_prceOldest )
    {
        m_prceOldest = prce;
    }

    Assert( PrceOldest() != prceNil );
    Assert( prceNil != m_prceNewest );
    Assert( prceNil != m_prceOldest );
    Assert( m_prceNewest == prce );
    Assert( this == prce->Pfcb() );

    PERFOptDeclare( const INST *pinst = PinstFromIfmp( Ifmp() ) );

    PERFOpt( cFCBAttachedRCE.Inc( pinst ) );
}


//  ================================================================
VOID FCB::DetachRCE( RCE * const prce )
//  ================================================================
//
//  Removes the RCE from the queue of RCEs held in the FCB. Decrements
//  the version count of the FCB
//
//-
{
    Assert( CritRCEList().FOwner() );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    Assert( this == prce->Pfcb() );
    Assert( prce->Ifmp() == Ifmp() );
    Assert( prce->PgnoFDP() == PgnoFDP() );

    Assert( PrceOldest() != prceNil );
    Assert( prceNil != m_prceNewest );
    Assert( prceNil != m_prceOldest );

    if( prce == m_prceNewest || prce == m_prceOldest )
    {
        //  at the head/tail of the list
        Assert( prceNil == prce->PrceNextOfFCB() || prceNil == prce->PrcePrevOfFCB() );
        if( prce == m_prceNewest )
        {
            Assert( prce->PrceNextOfFCB() == prceNil );
            m_prceNewest = prce->PrcePrevOfFCB();
            if( prceNil != m_prceNewest )
            {
                m_prceNewest->SetPrceNextOfFCB( prceNil );
            }
        }
        if( prce == m_prceOldest )
        {
            Assert( prce->PrcePrevOfFCB() == prceNil );
            m_prceOldest = prce->PrceNextOfFCB();
            if ( prceNil != m_prceOldest )
            {
                m_prceOldest->SetPrcePrevOfFCB( prceNil );
            }
        }
    }
    else
    {
        //  in the middle of the list
        Assert( prceNil != prce->PrceNextOfFCB() );
        Assert( prceNil != prce->PrcePrevOfFCB() );

        RCE * const prceNext = prce->PrceNextOfFCB();
        RCE * const prcePrev = prce->PrcePrevOfFCB();

        prceNext->SetPrcePrevOfFCB( prcePrev );
        prcePrev->SetPrceNextOfFCB( prceNext );
    }

    prce->SetPrceNextOfFCB( prceNil );
    prce->SetPrcePrevOfFCB( prceNil );

    Assert( ( prceNil == m_prceNewest ) == ( prceNil == m_prceOldest ) );
    Assert( prce->PrceNextOfFCB() == prceNil );
    Assert( prce->PrcePrevOfFCB() == prceNil );

    PERFOptDeclare( const INST *pinst = PinstFromIfmp( Ifmp() ) );

    PERFOpt( cFCBAttachedRCE.Dec( pinst ) );
}

//  unlink an FUCB from this FCB
VOID FCB::Unlink( FUCB *pfucb, const BOOL fPreventMoveToAvail )
{
    Assert( pfucb != pfucbNil );
    Assert( pfucb->u.pfcb != pfcbNil );
    Assert( this == pfucb->u.pfcb );

#ifdef DEBUG

    //  we do not need CritFCB() to check these flags because they are immutable

    if ( FFMPIsTempDB( Ifmp() ) && !FTypeNull() )
    {
        if ( PgnoFDP() == pgnoSystemRoot )
        {
            Assert( FTypeDatabase() );
        }
        else
        {
            Assert( FTypeTemporaryTable() || FTypeSort() || FTypeLV() );
        }
    }
#endif  //  DEBUG

    //  decrement the refcount of this FCB and unlink the FUCB from the FCB.
    DecrementRefCountAndUnlink_( pfucb, fTrue, fPreventMoveToAvail );

    //  WARNING: from this point on, we can't touch the FCB anymore, because the
    //  FCB may have been purged.
}

ERR FCB::ErrSetUpdatingAndEnterDML( PIB *ppib, BOOL fWaitOnConflict )
{
    ERR err = JET_errSuccess;

    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    // If DDL is fixed, then there's no contention with CreateIndex
    if ( !FFixedDDL() )
    {
        Assert( FTypeTable() );             // Sorts and temp tables have fixed DDL.
        Assert( !FTemplateTable() );
        Assert( Ptdb() != ptdbNil );

CheckIndexing:
        Ptdb()->EnterUpdating();
        EnterDML_();

        // SPECIAL CASE: Cannot update an uncommitted primary index if
        // it doesn't belong to us.
        if ( Pidb() != pidbNil && Pidb()->FVersionedCreate() )
        {
            err = ErrFILEIAccessIndex( ppib, this, this );
            if ( JET_errIndexNotFound == err )
            {
                LeaveDML_();
                ResetUpdating_();

                if ( fWaitOnConflict )
                {
                    // Abort update and wait for primary index to commit or
                    // rollback.  We're guaranteed the FCB will still exist
                    // because we have a cursor open on it.
                    UtilSleep( cmsecWaitGeneric );
                    err = JET_errSuccess;
                    goto CheckIndexing;
                }
                else
                {
                    err = ErrERRCheck( JET_errWriteConflictPrimaryIndex );
                }
            }
        }
    }

    return err;
}



ERR FCB::ErrSetDeleteIndex( PIB *ppib )
{
    Assert( FTypeSecondaryIndex() );
    Assert( !FDeletePending() );
    Assert( !FDeleteCommitted() );
    Assert( PfcbTable() != pfcbNil );
    Assert( PfcbTable()->FTypeTable() );
    Assert( Pidb() != pidbNil );
    Assert( !Pidb()->FDeleted() );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    PfcbTable()->AssertDDL();

    Assert( Pidb()->CrefCurrentIndex() <= WRefCount() );

    if ( Pidb()->CrefCurrentIndex() > 0 )
    {
        return ErrERRCheck( JET_errIndexInUse );
    }

    Assert( !PfcbTable()->FDomainDenyRead( ppib ) );
    if ( !PfcbTable()->FDomainDenyReadByUs( ppib ) )
    {
        Pidb()->SetFVersioned();
    }
    Pidb()->SetFDeleted();

    SetDomainDenyRead( ppib );

    Lock();
    SetDeletePending();
    Unlock_( LOCK_TYPE::ltWrite );

    return JET_errSuccess;
}



ERR VTAPI ErrIsamRegisterCallback(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_CBTYP       cbtyp,
    JET_CALLBACK    pCallback,
    VOID            *pvContext,
    JET_HANDLE      *phCallbackId )
{
    PIB     * const ppib    = reinterpret_cast<PIB *>( vsesid );
    FUCB    * const pfucb   = reinterpret_cast<FUCB *>( vtid );
    static BOOL     s_fLimitFinalizeCallbackNyi = fFalse;

    Assert( pfucb != pfucbNil );
    CheckTable( ppib, pfucb );

    ERR err = JET_errSuccess;

    if( JET_cbtypNull == cbtyp
        || NULL == pCallback
        || NULL == phCallbackId )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( !FNegTest( fInvalidAPIUsage ) && JET_cbtypFinalize == cbtyp && !s_fLimitFinalizeCallbackNyi )
    {
        AssertTrack( fFalse, "NyiFinalizeBehaviorClientRequestedCallback" );
        s_fLimitFinalizeCallbackNyi = fTrue;
    }

    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrDIRBeginTransaction( ppib, 38181, NO_GRBIT ) );

    CBDESC * const pcbdescInsert = new CBDESC;
    if( NULL != pcbdescInsert )
    {
        FCB * const pfcb = pfucb->u.pfcb;
        Assert( NULL != pfcb );
        TDB * const ptdb = pfcb->Ptdb();
        Assert( NULL != ptdb );

        *phCallbackId = (JET_HANDLE)pcbdescInsert;

        pcbdescInsert->pcallback    = pCallback;
        pcbdescInsert->cbtyp        = cbtyp;
        pcbdescInsert->pvContext    = pvContext;
        pcbdescInsert->cbContext    = 0;
        pcbdescInsert->ulId         = 0;
        pcbdescInsert->fPermanent   = fFalse;

#ifdef VERSIONED_CALLBACKS
        pcbdescInsert->fVersioned   = !g_rgfmp[pfucb->ifmp].FVersioningOff();

        pcbdescInsert->trxRegisterBegin0    =   ppib->trxBegin0;
        pcbdescInsert->trxRegisterCommit0   =   trxMax;
        pcbdescInsert->trxUnregisterBegin0  =   trxMax;
        pcbdescInsert->trxUnregisterCommit0 =   trxMax;

        VERCALLBACK vercallback;
        vercallback.pcallback   = pcbdescInsert->pcallback;
        vercallback.cbtyp       = pcbdescInsert->cbtyp;
        vercallback.pvContext   = pcbdescInsert->pvContext;
        vercallback.pcbdesc     = pcbdescInsert;

        if( pcbdescInsert->fVersioned )
        {
            VER *pver = PverFromIfmp( pfucb->ifmp );
            err = pver->ErrVERFlag( pfucb, operRegisterCallback, &vercallback, sizeof( VERCALLBACK ) );
        }
#else
        pcbdescInsert->fVersioned   = fFalse;
#endif  //  DYNAMIC_CALLBACKS

        if( err >= 0 )
        {
            pfcb->EnterDDL();
            ptdb->RegisterPcbdesc( pcbdescInsert );
            pfcb->LeaveDDL();
        }
    }
    else
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        *phCallbackId = 0xFFFFFFFA;
    }

    if( err >= 0 )
    {
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }
    if( err < 0 )
    {
        CallS( ErrDIRRollback( ppib ) );
    }
    return err;
}


ERR VTAPI ErrIsamUnregisterCallback(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_CBTYP       cbtyp,
    JET_HANDLE      hCallbackId )
{
    PIB     * const ppib    = reinterpret_cast<PIB *>( vsesid );
    FUCB    * const pfucb   = reinterpret_cast<FUCB *>( vtid );

    ERR     err = JET_errSuccess;

    //  check input
    CallR( ErrPIBCheck( ppib ) );
    Assert( pfucb != pfucbNil );
    CheckTable( ppib, pfucb );

    //  the callback id is a pointer to the CBDESC to remove

    FCB * const pfcb = pfucb->u.pfcb;
    Assert( NULL != pfcb );
    TDB * const ptdb = pfcb->Ptdb();
    Assert( NULL != ptdb );

    CBDESC * const pcbdescRemove = (CBDESC *)hCallbackId;
    Assert( !pcbdescRemove->fPermanent );
    Assert( JET_cbtypNull != pcbdescRemove->cbtyp );
    Assert( NULL != pcbdescRemove->pcallback );

    CallR( ErrDIRBeginTransaction( ppib, 54565, NO_GRBIT ) );

#ifndef VERSIONED_CALLBACKS
    Assert( !pcbdescRemove->fVersioned );
#else   //  !VERSIONED_CALLBACKS

    VERCALLBACK vercallback;
    vercallback.pcallback   = pcbdescRemove->pcallback;
    vercallback.cbtyp       = pcbdescRemove->cbtyp;
    vercallback.pvContext   = pcbdescRemove->pvContext;
    vercallback.pcbdesc     = pcbdescRemove;

    if( pcbdescRemove->fVersioned )
    {
        VER *pver = PverFromIfmp( pfucb->ifmp );
        err = pver->ErrVERFlag( pfucb, operUnregisterCallback, &vercallback, sizeof( VERCALLBACK ) );
        if( err >= 0 )
        {
            pfcb->EnterDDL();
            pcbdescRemove->trxUnregisterBegin0 = ppib->trxBegin0;
            pfcb->LeaveDDL();
        }
    }
    else
    {
#endif  //  VERSIONED_CALLBACKS

        //  unversioned.
        //
        //  access to the CBDESC list isn't synchronized so we can't remove
        //  the item, just nullify it
        pfcb->EnterDDL();
        pcbdescRemove->cbtyp = JET_cbtypNull;
        pfcb->LeaveDDL();

#ifdef VERSIONED_CALLBACKS
    }
#endif  //  VERSIONED_CALLBACKS

    if( err >= 0 )
    {
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }
    if( err < 0 )
    {
        CallS( ErrDIRRollback( ppib ) );
    }
    return err;
}


#ifdef DEBUG
VOID FCBAssertAllClean( INST *pinst )
{
    FCB *pfcbT;

    //  lock the FCB list

    pinst->m_critFCBList.Enter();

    //  verify that all FCB's have been cleaned and there are no outstanding versions.

    for ( pfcbT = pinst->m_pfcbList; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextList() )
    {
        Assert( pfcbT->PrceOldest() == prceNil );
    }

    //  unlock the FCB list

    pinst->m_critFCBList.Leave();
}
#endif


//  public API to increment the refcount of an FCB directly
//
//  this allows you to bypass PfcbFCBGet (the proper lookup function)
//      under the assumption that the FCB you are refcounting will
//      not suddenly disappear (e.g. you own a cursor on it or know
//      for a fact that someone else does and they will not close it)
VOID FCB::IncrementRefCount()
{
    IncrementRefCount_( fFalse );
}

VOID FCB::IncrementRefCount_( BOOL fOwnWriteLock )
{
    ERR err = ErrIncrementRefCountAndLink_( pfucbNil, fOwnWriteLock );
    // The only error path for this is in the "AndLink" portion so the return has to always be success.
    Enforce( JET_errSuccess == err );
}

//  decrement the refcount of an FCB directly
//
//  if refcnt goes from 1 to 0, the FCB is moved into the avail-LRU list
//
//  if refcnt goes from 1 to 0, the FCB is marked to try to purge on close, and we
//     unlinked a pfucb, try to purge the FCB.
//

VOID FCB::DecrementRefCountAndUnlink_( FUCB *pfucb, const BOOL fLockList, const BOOL fPreventMoveToAvail )
{
    INST *pinst = PinstFromIfmp( Ifmp() );
    BOOL fTryPurge = fFalse;

    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );

    // This shared lock makes sure that we don't get purged after we decrement the
    // refcount.  If two threads are in here:
    // * Both decrement shared.
    // * One and only one gets a return from the decrement saying it decremented to
    //     zero.
    // * The one that decremented to zero is going to drop the shared lock and take
    //     a write lock.  It can't do that until the one that decremented to 1
    //     releases it's shared lock.
    // * The thread that decrements to 1 does no more processing after releasing the
    //     shared lock.
    Lock_( LOCK_TYPE::ltShared );

    Assert( FWRefCountOK_() );

    // If a pfucb was provided, unlink it from the FCB.
    if (pfucbNil != pfucb)
    {
        pfucb->u.pfcb = pfcbNil;

        // This is a shared lock.
        FucbList().LockForModification();
        FucbList().Remove( pfucb );
        FucbList().UnlockForModification();
    }

    //  Decrement the refcount in a threadsafe way as there is only a shared lock held at this point.

    LONG lResultRefCount = AtomicDecrement( &m_wRefCount );
    Enforce( lResultRefCount >= 0 );

    if ( pinst->m_pFCBRefTrace != NULL )
    {
        OSTraceWriteRefLog( pinst->m_pFCBRefTrace, lResultRefCount, this );
    }

    if ( 0 != lResultRefCount )
    {
        Unlock_( LOCK_TYPE::ltShared );
        return;
    }

    if ( fPreventMoveToAvail )
    {
        Unlock_( LOCK_TYPE::ltShared );
        return;
    }

    // We only do further processing with FCBs that are tables.  However, during recovery, FCB
    // type can change from table to something else, so we have to check again under the write lock.
    if ( !FTypeTable() && !pinst->FRecovering() )
    {
        Unlock_( LOCK_TYPE::ltShared );
        return;
    }

    // To proceed, we need the write lock.  However, there may already be threads out there
    // waiting on the lock for exclusive, so we can't upgrade.  We're going to completely
    // drop the lock, but we'll increment the refcount to make sure the FCB isn't
    // purged out from under us.

    lResultRefCount = AtomicIncrement( &m_wRefCount );
    AssertTrack( lResultRefCount > 0, "RefCountTooLowAfterInc" );
    Unlock_( LOCK_TYPE::ltShared );
    if ( fLockList )
    {
        // The caller tells us that we also need to lock the pinst->m_critFCBList before we
        //   add the FCB to the AvailList.  We couldn't just take that lock when we already
        //   had a lock on the FCB because taking the lock on the list is a rank violaiton.
        //   We need to wait until now when we've dropped the shared lock on the FCB before
        //   we can take the list lock.
        Assert( pinst->m_critFCBList.FNotOwner() );
        pinst->m_critFCBList.Enter();
    }
    Lock();
    // Take off the refcount we just put on to keep this from being purged.
    lResultRefCount = AtomicDecrement( &m_wRefCount );
    AssertTrack( lResultRefCount >= 0, "RefCountTooLowAfterDec" );
    Assert( pinst->m_critFCBList.FOwner() );

    if ( FTypeTable() && ( 0 == WRefCount() ) && !FInLRU() )
    {
        // We're single threaded here, and the FCB still needs to be in the AvailList,
        // no other threads have intervened.
        InsertAvailListMRU_();
        fTryPurge = fTrue;
    }

    Unlock_( LOCK_TYPE::ltWrite );

    if ( fTryPurge && ( pfucbNil != pfucb ) && FTryPurgeOnClose() )
    {
        // We unlinked an FUCB from a table, and it was the last thing with
        // a refcount on the table.  Try to purge the FCB.  If we succeed,
        // it has to be the last reference to "this", as it may have
        // been purged.
        BOOL fPurgeable = FCheckFreeAndPurge_( pfucb->ppib, fFalse );

        if ( fPurgeable )
        {
            //  WARNING: from this point on, we shouldn't touch the FCB anymore; it may have been purged.
            PERFOpt( cFCBPurgeOnClose.Inc( pinst ) );
        }
    }

    if ( fLockList )
    {
        pinst->m_critFCBList.Leave();
    }

}


//  increment the refcount of the FCB
//
//  if a pfucb is supplied, link it to the FCB.
//
//  if the FCB's refcnt went from 0 to 1 and the FCB is of type
//      table/database, remove the FCB from the AvailList.
//

INLINE ERR FCB::ErrIncrementRefCountAndLink_( FUCB *pfucb, const BOOL fOwnWriteLock )
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    // You should be calling this either with no locks or with an already held write
    // lock.
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    if ( fOwnWriteLock )
    {
        // You should be holding the write lock if the FCB is on the AvailList and
        // has a refcount of 0.  In that case, the write lock keeps the FCB from being
        // purged.
        //
        // NOTE: The write lock is held when you get here via FCB::PfcbFCBGet().  There
        //  is no constraint on FInLRU() or m_wRefCount in that case.  However, that
        //  call stack doesn't ever include an FUCB to link, so make that an Expected().
        //
        Assert( IsLocked_( LOCK_TYPE::ltWrite ) );
        Expected( pfucbNil == pfucb );
    }
    else
    {
        //
        // Take the shared lock before we increment the refcount.  We do this because the
        // code that reads the refcount in purge is done inside the write lock.  That
        // gives the purge code a stable view of the refcount.
        //
        // NOTE: It's OK here for m_wRefCount == 0.  That happens everytime
        //   we get here when creating an FCB.
        //
        // This shared lock is the heart of the perf improvement for many threads
        // switching back and forth between 2 indices, thus calling this method and
        // FCB::DecrementRefCountAndUnlink_() many times, simultaneously on many threads.
        //
        Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
        Lock_( LOCK_TYPE::ltShared );
    }

    Assert( FWRefCountOK_() );

    // Briefly, the FUCB is in the FUCB list attached to the FCB, but is not reflected
    // in the refcount. However, because the FCB is at least shared-locked, that's OK, noone else
    // can make any bad decisions based on this without the write-lock.

    if ( pfucbNil != pfucb )
    {
        FucbList().LockForModification();
        COLL::CInvasiveConcurrentModSet<FUCB, FUCBOffsetOfIAE>::ERR err = FucbList().ErrInsert( pfucb );
        Assert( FWRefCountOK_() );
        FucbList().UnlockForModification();
        if ( COLL::CInvasiveConcurrentModSet<FUCB, FUCBOffsetOfIAE>::ERR::errSuccess != err )
        {
            if ( !fOwnWriteLock ) {
                Unlock_( LOCK_TYPE::ltShared );
            }
            return ErrERRCheck( JET_errOutOfMemory );
        }

        pfucb->u.pfcb = this;
    }

    // Increment the refcount in a threadsafe way.
    LONG lResultRefCount = AtomicIncrement( &m_wRefCount );
    EnforceSz( lResultRefCount > 0, OSFormat( "InvalidRefCount:%d", lResultRefCount ) );

    if ( pinst->m_pFCBRefTrace != NULL )
    {
        OSTraceWriteRefLog( pinst->m_pFCBRefTrace, lResultRefCount, this );
    }

    if ( 1 == lResultRefCount && FTypeTable() )
    {
        // This thread just incremented the refcount of a table to 1.  That means we
        // must also now make sure it's removed from the AvailListMRU_().  We need the
        // pinst->m_critFCBList, but we have to drop the write lock first due to
        // lock ranking.
        //
        // SOMEONE: CONSIDER: It would be nice if there was a way to downgrade from the
        // write lock to the waiting state on an exclusive lock, take the
        // pinst->m_critFCBList, then upgrade back to a write lock.
        //

        Assert( pinst->m_critFCBList.FNotOwner() );
        if ( !fOwnWriteLock )
        {
            // There are a variety of reasons why we might be incrementing the refcount
            // on an FCB where it's in the AvailList but we don't already have a
            // write lock on it.  We assert on those in FCBCheckFCBLockingForLink_().
            // Regardless, we need to drop the shared lock before we enter the critsec.
            Unlock_( LOCK_TYPE::ltShared );
        }
        else
        {
            Unlock_( LOCK_TYPE::ltWrite );
        }

        pinst->m_critFCBList.Enter();
        Lock_( LOCK_TYPE::ltWrite );

        // It's possible that after we dropped our lock, someone else decremented the ref
        // count back to 0, and they may or may not have moved the FCB into or out of
        // the AvailList.  We have to check again now that we have the write lock.
        if ( 0 != WRefCount() && FInLRU() )
        {
            RemoveAvailList_();
        }

        if ( !fOwnWriteLock )
        {
            LockDowngradeWriteToShared_();
        }

        pinst->m_critFCBList.Leave();
    }

    if ( !fOwnWriteLock )
    {
        Unlock_( LOCK_TYPE::ltShared );
    }

    return JET_errSuccess;
}


//  remove this FCB from the avail-LRU list

VOID FCB::RemoveAvailList_(
#ifdef DEBUG
    const BOOL fPurging
#endif  //  DEBUG
    )
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    EnforceSz( FTypeTable(), "AvailFcbMustBeTable" );

    Assert( FCBCheckAvailList_( fTrue, fPurging ) );

    //  get the list pointers

    FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU();
    FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU();

    //  remove the FCB

    if ( PfcbMRU() != pfcbNil )
    {
        Assert( *ppfcbAvailMRU != this );
        Assert( PfcbMRU()->PfcbLRU() == this );     //  verify that this FCB is in the LRU list
        PfcbMRU()->SetPfcbLRU( PfcbLRU() );
    }
    else
    {
        Assert( *ppfcbAvailMRU == this );           //  verify that this FCB is in the LRU list
        *ppfcbAvailMRU = PfcbLRU();
    }
    if ( PfcbLRU() != pfcbNil )
    {
        Assert( *ppfcbAvailLRU != this );
        Assert( PfcbLRU()->PfcbMRU() == this );     //  verify that this FCB is in the LRU list
        PfcbLRU()->SetPfcbMRU( PfcbMRU() );
    }
    else
    {
        Assert( *ppfcbAvailLRU == this );           //  verify that this FCB is in the LRU list
        *ppfcbAvailLRU = PfcbMRU();
    }
    ResetInLRU();
    SetPfcbMRU( pfcbNil );
    SetPfcbLRU( pfcbNil );
    pinst->m_cFCBAvail--;

    //  update performance counter

    PERFOpt( cFCBCacheAllocAvail.Dec( pinst ) );

    Assert( FCBCheckAvailList_( fFalse, fPurging ) );
}


//  insert this FCB into the avail list at the MRU position

VOID FCB::InsertAvailListMRU_()
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( FCBCheckAvailList_( fFalse, fFalse ) );

    Enforce( FTypeTable() );

    //  get the list pointers

    FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU();
    FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU();

    //  insert the FCB at the MRU end of the avail list

    if ( *ppfcbAvailMRU != pfcbNil )
    {
        (*ppfcbAvailMRU)->SetPfcbMRU( this );
    }
    //SetPfcbMRU( pfcbNil );
    SetPfcbLRU( *ppfcbAvailMRU );
    *ppfcbAvailMRU = this;
    if ( *ppfcbAvailLRU == pfcbNil )
    {
        *ppfcbAvailLRU = this;
    }
    SetInLRU();
    pinst->m_cFCBAvail++;

    //  update performance counters

    PERFOpt( cFCBCacheAllocAvail.Inc( pinst ) );

    Assert( FCBCheckAvailList_( fTrue, fFalse ) );
}


//  insert this FCB into the global list

VOID FCB::InsertList()
{
    INST *pinst = PinstFromIfmp( Ifmp() );
    Assert( pinst->m_critFCBList.FNotOwner() );

    //  lock the global list

    pinst->m_critFCBList.Enter();

    //  verify that this FCB is not yet in the list

    Assert( !FInList() );
    Assert( PfcbNextList() == pfcbNil );
    Assert( PfcbPrevList() == pfcbNil );

    //  verify the consistency of the list (it may be empty)

    Assert( pinst->m_pfcbList == pfcbNil ||
            pinst->m_pfcbList->PfcbPrevList() == pfcbNil );

    //  insert the FCB at the MRU end of the avail list

    if ( pinst->m_pfcbList != pfcbNil )
    {
        pinst->m_pfcbList->SetPfcbPrevList( this );
    }
    //SetPfcbPrevList( pfcbNil );
    SetPfcbNextList( pinst->m_pfcbList );
    pinst->m_pfcbList = this;
    SetInList();

    //  verify the consistency of the list (it should not be empty)

    Assert( pinst->m_pfcbList != pfcbNil );
    Assert( pinst->m_pfcbList->PfcbPrevList() == pfcbNil );

    //  unlock the global list

    pinst->m_critFCBList.Leave();
}

//  remove this FCB from the global list

VOID FCB::RemoveList()
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( pinst->FRecovering() );
    Assert( FInitedForRecovery() );

    pinst->m_critFCBList.Enter();
    RemoveList_();
    pinst->m_critFCBList.Leave();
}

VOID FCB::RemoveList_()
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( pinst->m_critFCBList.FOwner() );

    //  verify that this FCB is in the list

    Assert( FInList() );

    //  verify the consistency of the list (it will not be empty)

    Assert( pinst->m_pfcbList != pfcbNil );
    Assert( pinst->m_pfcbList->PfcbPrevList() == pfcbNil );

    //  remove the FCB

    if ( PfcbPrevList() != pfcbNil )
    {
        Assert( pinst->m_pfcbList != this );
        Assert( PfcbPrevList()->PfcbNextList() == this );   //  verify that this FCB is in the list
        PfcbPrevList()->SetPfcbNextList( PfcbNextList() );
    }
    else
    {
        Assert( pinst->m_pfcbList == this );                //  verify that this FCB is in the list
        pinst->m_pfcbList = PfcbNextList();
    }
    if ( PfcbNextList() != pfcbNil )
    {
        Assert( PfcbNextList()->PfcbPrevList() == this );   //  verify that this FCB is in the list
        PfcbNextList()->SetPfcbPrevList( PfcbPrevList() );
    }
    ResetInList();
    SetPfcbNextList( pfcbNil );
    SetPfcbPrevList( pfcbNil );

    //  verify the consistency of the list (it may be empty)

    Assert( pinst->m_pfcbList == pfcbNil ||
            pinst->m_pfcbList->PfcbPrevList() == pfcbNil );
}

//  returns true if OLD2 may be run against this btree

BOOL FCB::FUseOLD2()
{
    //  we currently only allow defrag of the clustered index

    if ( !FTypeTable() )
    {
        return fFalse;
    }

    //  efficient sequential scan must be configured

    if ( !FRetrieveHintTableScanForward() && !FRetrieveHintTableScanBackward() )
    {
        return fFalse;
    }

    //  contiguous append must be configured

    if ( !FContiguousAppend() && !FContiguousHotpoint() )
    {
        return fFalse;
    }

    return fTrue;
}

