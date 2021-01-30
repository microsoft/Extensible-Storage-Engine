// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"


#ifdef PERFMON_SUPPORT



PERFInstanceDelayedTotalWithClass<> cFCBAsyncScan;
PERFInstanceDelayedTotalWithClass<> cFCBAsyncPurgeSuccess;
PERFInstanceDelayedTotalWithClass<> cFCBAsyncPurgeFail;


PERFInstanceDelayedTotal<> cFCBSyncPurge;
PERFInstanceDelayedTotal<> cFCBSyncPurgeStalls;
PERFInstanceDelayedTotal<> cFCBPurgeOnClose;
PERFInstanceDelayedTotal<> cFCBAllocWaitForRCEClean;


PERFInstanceDelayedTotal<> cFCBCacheHits;
PERFInstanceDelayedTotal<> cFCBCacheRequests;
PERFInstanceDelayedTotal<> cFCBCacheStalls;


PERFInstanceDelayedTotal<LONG, INST, fFalse> cFCBCacheMax;
PERFInstanceDelayedTotal<LONG, INST, fFalse> cFCBCachePreferred;

PERFInstanceDelayedTotal<> cFCBCacheAlloc;
PERFInstanceDelayedTotal<> cFCBCacheAllocAvail;
PERFInstanceDelayedTotal<> cFCBCacheAllocFailed;
PERFInstance<DWORD_PTR, fTrue> cFCBCacheAllocLatency;


PERFInstanceDelayedTotal<> cFCBAttachedRCE;




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

#endif


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
        Assert( pinst->m_cFCBAvail >= 0 );
        Assert( pinst->m_cFCBAlloc > 0 );
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

    FCB *   pfcbThis = pfcbNil;
    for ( pfcbThis = *ppfcbAvailMRU;
          pfcbThis != pfcbNil;
          pfcbThis = pfcbThis->PfcbLRU() )
    {
        if ( pfcbThis == this )
            break;
    }


    if ( fShouldBeInList )
    {

        Assert( FInLRU() );




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
#endif

VOID FCB::RefreshPreferredPerfCounter( __in INST * const pinst )
{
    PERFOpt( cFCBCachePreferred.Set( pinst, (LONG)UlParam( pinst, JET_paramCachedClosedTables ) ) );
}

VOID FCB::ResetPerfCounters( __in INST * const pinst, BOOL fTerminating )
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


ERR FCB::ErrFCBInit( INST *pinst )
{
    ERR             err;
    BYTE            *rgbFCBHash;
    FCBHash::ERR    errFCBHash;

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


    errFCBHash = pinst->m_pfcbhash->ErrInit( 5.0, 1.0 );
    if ( errFCBHash != FCBHash::ERR::errSuccess )
    {
        Assert( errFCBHash == FCBHash::ERR::errOutOfMemory );
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    ResetPerfCounters( pinst, fFalse  );

    OnDebug( (VOID)ErrOSTraceCreateRefLog( 10000, 0, &pinst->m_pFCBRefTrace ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        FCB::Term( pinst );
    }
    return err;
}


VOID FCB::Term( INST *pinst )
{
    ResetPerfCounters( pinst, fTrue  );

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

    if ( pfcbTable == this )
    {
        Assert( !pfcbTable->FSequentialIndex() );
    }
    else
    {
        Assert( pfcbTable->FTypeTable() );

        Assert( FTypeSecondaryIndex() );
    }

    IDB *pidb = Pidb();
    Assert( pidbNil != pidb );

    Assert( !pidb->FTemplateIndex() );
    Assert( !pidb->FDerivedIndex() );

    Assert( pidb->FDeleted() );
    Assert( !pidb->FVersioned() );
    while ( pidb->CrefVersionCheck() > 0 || pidb->CrefCurrentIndex() > 0 )
    {
        pfcbTable->LeaveDDL();
        UtilSleep( cmsecWaitGeneric );
        pfcbTable->EnterDDL();

        Assert( pidb == Pidb() );
    }

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


    pinst->m_pfcbhash->ReadLockKey( keyFCBHash, &lockFCBHash );


    PERFOpt( cFCBCacheRequests.Inc( pinst ) );


    errFCBHash = pinst->m_pfcbhash->ErrRetrieveEntry( &lockFCBHash, &entryFCBHash );

    if ( errFCBHash != FCBHash::ERR::errSuccess )
    {

        pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );

        pfcbT = pfcbNil;
        goto SetStateAndReturn;
    }

    PERFOpt( cFCBCacheHits.Inc( pinst ) );


    Assert( entryFCBHash.m_pgnoFDP == pgnoFDP );
    pfcbT = entryFCBHash.m_pfcb;
    Assert( pfcbNil != pfcbT );

    if ( pfcbT->FNeedLock_() )
    {
        CSXWLatch::ERR errSXWLatch = pfcbT->m_sxwl.ErrAcquireExclusiveLatch();

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
        pinst->m_pfcbhash->ReadUnlockKey( &lockFCBHash );
    }



    if ( !pfcbT->FInitialized() )
    {


        const ERR   errInit     = pfcbT->ErrErrInit();

        pfcbT->Unlock_( LOCK_TYPE::ltWrite );

        if ( JET_errSuccess != errInit )
        {


            Assert( errInit < JET_errSuccess );
            pfcbT = pfcbNil;
            goto SetStateAndReturn;
        }
        else
        {



            PERFOpt( cFCBCacheStalls.Inc( pinst ) );


            UtilSleep( 10 );


            cRetries++;
            goto RetrieveFCB;
        }
    }



    CallS( pfcbT->ErrErrInit() );
    Assert( pfcbT->Ifmp() == ifmp );
    Assert( pfcbT->PgnoFDP() == pgnoFDP );

    if ( !fIncrementRefCount )
    {


        Assert( pfState == NULL );
    }
    else
    {
        fState = fFCBStateInitialized;


        fDoIncRefCount = fTrue;
    }



    if ( pfcbT != pfcbNil && !fInitForRecovery && pfcbT->FInitedForRecovery() )
    {
        if ( !pfcbT->FDoingAdditionalInitializationDuringRecovery() )
        {
            Assert( pfcbT->IsLocked_( LOCK_TYPE::ltWrite ) );
            pfcbT->SetDoingAdditionalInitializationDuringRecovery();
        }
        else
        {
            pfcbT->Unlock_( LOCK_TYPE::ltWrite );


            PERFOpt( cFCBCacheStalls.Inc( pinst ) );


            UtilSleep( 10 );


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
    if ( pfState )
    {
        *pfState = fState;
    }

    Assert( ( pfcbNil == pfcbT ) || ( pfcbT->IsUnlocked_( LOCK_TYPE::ltShared ) && pfcbT->IsUnlocked_( LOCK_TYPE::ltWrite ) ) );
    return pfcbT;
}



ERR FCB::ErrCreate( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb )
{
    ERR     err;
    INST    *pinst = PinstFromPpib( ppib );


    *ppfcb = pfcbNil;


    pinst->m_critFCBCreate.Enter();

    if ( !FInHashTable( ifmp, pgnoFDP ) )
    {



        err = ErrAlloc_( ppib, ifmp, pgnoFDP, ppfcb );
        Assert( err == JET_errSuccess || err == JET_errTooManyOpenTables || err == JET_errTooManyOpenTablesAndCleanupTimedOut || err == errFCBExists );

        if ( JET_errTooManyOpenTables == err ||
            JET_errTooManyOpenTablesAndCleanupTimedOut == err )
        {
            if ( FInHashTable( ifmp, pgnoFDP ) )
            {

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


            pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );


            errFCBHash = pinst->m_pfcbhash->ErrInsertEntry( &lockFCBHash, entryFCBHash );
            if ( errFCBHash == FCBHash::ERR::errSuccess )
            {



                Assert( pgnoNull == (*ppfcb)->PgnoNextAvailSE() );
                Assert( NULL == (*ppfcb)->Psplitbufdangling_() );
                Assert( (*ppfcb)->PrceOldest() == prceNil );

                (*ppfcb)->Lock();
                err = JET_errSuccess;
            }
            else
            {


                Assert( errFCBHash == FCBHash::ERR::errOutOfMemory );


                delete *ppfcb;
                *ppfcb = pfcbNil;

                AtomicDecrement( (LONG *)&pinst->m_cFCBAlloc );


                PERFOpt( cFCBCacheAlloc.Dec( pinst ) );


                err = ErrERRCheck( JET_errOutOfMemory );
            }


            pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );
        }
    }
    else
    {


        err = ErrERRCheck( errFCBExists );
    }


    pinst->m_critFCBCreate.Leave();

    Assert( ( err >= JET_errSuccess && *ppfcb != pfcbNil ) ||
            ( err < JET_errSuccess && *ppfcb == pfcbNil ) );

    return err;
}



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


    SetErrInit( err );

    if ( JET_errSuccess == err )
    {
        Assert( !FInitialized() || FInitedForRecovery() );


        SetInitialized_();
    }
    else
    {

        ResetInitialized_();
    }
}



BOOL FCB::FScanAndPurge_(
    __in INST * pinst,
    __in PIB * ppib,
    const BOOL fThreshold )
{
    ULONG   cFCBInspected                   = 0;

    const ULONG cMaxFCBInspected        = (ULONG) max( UlParam( pinst, JET_paramCachedClosedTables ) * 0.02, 256 );
    const ULONG cMinFCBToPurge          = (ULONG) max( UlParam( pinst, JET_paramCachedClosedTables ) * 0.01, 128 );
    ULONG cFCBToPurge;

    if ( fThreshold )
    {
        cFCBToPurge = (ULONG) min( pinst->m_cFCBAvail - UlParam( pinst, JET_paramCachedClosedTables ) + cMinFCBToPurge, cMinFCBToPurge );
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


        PERFOpt( cFCBAsyncScan.Inc( pinst, tce ) );

        if ( pfcbToPurge->FCheckFreeAndPurge_( ppib, fThreshold ) )
        {

            pfcbToPurge = NULL;


            PERFOpt( cFCBAsyncPurgeSuccess.Inc( pinst, tce ) );

            cFCBPurged++;
        }

        cFCBInspected++;
    }

    return ( 0 != cFCBPurged );
}


ERR FCB::ErrAlloc_( PIB *ppib, IFMP ifmp, PGNO pgnoFDP, FCB **ppfcb )
{
    FCB     *pfcbCandidate = pfcbNil;
    INST    *pinst = PinstFromPpib( ppib );

    Assert( pinst->m_critFCBCreate.FOwner() );

    PERFOptDeclare( const TICK tickStartAlloc = TickOSTimeCurrent() );

    BOOL fCleanupTimedOut = fFalse;

    const BOOL fCloseToQuota = FCloseToQuota_( pinst );

    if ( fCloseToQuota )
    {

        VERSignalCleanup( ppib );
    }

    if ( ( pinst->m_cFCBAvail > UlParam( pinst, JET_paramCachedClosedTables ) ) ||
            ( ( pinst->m_cFCBAlloc - pinst->m_cFCBAvail ) > UlParam( pinst, JET_paramCachedClosedTables ) ) ||
            ( fCloseToQuota ) )
    {

        pinst->m_critFCBList.Enter();

        if ( FScanAndPurge_( pinst, ppib, fTrue  ) )
        {

            pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
            AssertSz( pfcbNil != pfcbCandidate || FRFSAnyFailureDetected(), "We just purged at least one FCB, so the allocation should always succeed." );
        }

        pinst->m_critFCBList.Leave();
    }


    if ( pfcbNil == pfcbCandidate )
    {
        pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
    }

    if ( pfcbNil == pfcbCandidate )
    {


        VERSignalCleanup( ppib );

        pinst->m_critFCBCreate.Leave();

        fCleanupTimedOut = !PverFromPpib( ppib )->m_msigRCECleanPerformedRecently.FWait( cmsecAsyncBackgroundCleanup );

        PERFOpt( cFCBAllocWaitForRCEClean.Inc( pinst ) );

        pinst->m_critFCBCreate.Enter();

        if ( FInHashTable( ifmp, pgnoFDP ) )
        {

            return ErrERRCheck( errFCBExists );
        }


        pinst->m_critFCBList.Enter();


        if ( FScanAndPurge_( pinst, ppib, fFalse  ) )
        {

            pfcbCandidate = new( pinst ) FCB( ifmp, pgnoFDP );
            AssertSz( pfcbNil != pfcbCandidate || FRFSAnyFailureDetected(), "We just purged at least one FCB, so the allocation should always succeed." );
        }


        pinst->m_critFCBList.Leave();


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


    PERFOpt( cFCBCacheAlloc.Inc( pinst ) );
    PERFOpt( cFCBCacheAllocLatency.Add( pinst->m_iInstance, (DWORD_PTR)( TickOSTimeCurrent() - tickStartAlloc ) ) );


    *ppfcb = pfcbCandidate;

    return JET_errSuccess;
}

enum FCBPurgeFailReason : BYTE
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


BOOL FCB::FCheckFreeAndPurge_(
    __in PIB *ppib,
    __in const BOOL fThreshold )
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


    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );


    if ( FLockTry_( LOCK_TYPE::ltWrite ) )
    {


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


            for ( pfcbIndex = this; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
            {
                if ( pfcbIndex->Pidb() != pidbNil )
                {
                    Assert( pfcbIndex->Pidb()->CrefVersionCheck() == 0 );
                    Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
                }

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

                if ( pfcbIndex->FOutstandingVersions_()
                    || pfcbIndex->CTasksActive() > 0
                    || pfcbIndex->WRefCount() > 0 )
                {
                    fcbpfr = fcbpfrIndexOutstandingActive;
                    break;
                }
            }


            if ( pinst->FRecovering() && Ptdb() == ptdbNil )
            {
                Assert( pfcbNil == pfcbIndex );
                fAvail = fTrue;
            }
            else if ( pfcbNil == pfcbIndex )
            {

                Assert( Ptdb() != ptdbNil );

                FCB *pfcbLV = Ptdb()->PfcbLV();
                if ( pfcbNil == pfcbLV )
                {
                    fAvail = fTrue;
                }
                else if ( pfcbLV->FLockTry_( LOCK_TYPE::ltWrite ) )
                {
                    if ( pfcbLV->FOutstandingVersions_()
                        || pfcbLV->CTasksActive() > 0
                        || pfcbLV->WRefCount() > 0 )
                    {
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
        else
        {


            errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );


            Assert( errFCBHash == FCBHash::ERR::errSuccess ||
                    ( errFCBHash == FCBHash::ERR::errNoCurrentEntry && FDeleteCommitted() ) );


            pfcbT = PfcbNextIndex();
            while ( pfcbT != pfcbNil )
            {
                Assert( 0 == pfcbT->WRefCount() );
                pfcbT->CreateCompleteErr( errFCBUnusable );
                pfcbT->Unlock_( LOCK_TYPE::ltWrite );
                pfcbT = pfcbT->PfcbNextIndex();
            }
            if ( Ptdb() &&
                 Ptdb()->PfcbLV() )
            {
                Assert( 0 == Ptdb()->PfcbLV()->WRefCount() );
                Ptdb()->PfcbLV()->CreateCompleteErr( errFCBUnusable );
                Ptdb()->PfcbLV()->Unlock_( LOCK_TYPE::ltWrite );
            }
        }

#ifdef DEBUG
        for ( pfcbT = PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltWrite ) );
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltShared ) );
        }
#endif

        Unlock_( LOCK_TYPE::ltWrite );
    }
    else
    {

        fcbpfr = fcbpfrConflict;
    }

    Assert( ( fAvail && fcbpfr == fcbpfrInvalid ) ||
            ( !fAvail && fcbpfr != fcbpfrInvalid ) );
    if ( !fAvail )
    {
        Assert( fcbpfr != fcbpfrInvalid );
        PERFOpt( cFCBAsyncPurgeFail.Inc( pinst, TCE() ) );
        ETFCBPurgeFailure( pinst->m_iInstance, (BYTE)( 0x1  | ( fThreshold ? 0x2 : 0x0 ) ), fcbpfr, TCE() );
    }


    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

    if ( fAvail )
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


            if ( FCATHashActive( pinst ) )
            {
                CHAR szTable[JET_cbNameMost+1];



                EnterDML();
                OSStrCbCopyA( szTable, sizeof(szTable), Ptdb()->SzTableName() );
                LeaveDML();


                CATHashIDelete( this, szTable );
            }
        }

        Assert( FTypeTable() );
        Purge( fFalse );
    }

    return fAvail;
}



INLINE VOID FCB::CloseAllCursorsOnFCB_( const BOOL fTerminating )
{
    if ( fTerminating )
    {

        m_prceNewest = prceNil;
        m_prceOldest = prceNil;
    }

    Assert( PrceNewest() == prceNil );
    Assert( PrceOldest() == prceNil );


    FUCBCloseAllCursorsOnFCB( ppibNil, this );
}



VOID FCB::CloseAllCursors( const BOOL fTerminating )
{
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
#endif

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



VOID FCB::Delete_( INST *pinst )
{
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );


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
#endif


#ifdef DEBUG
    FCB *pfcbT;
    Assert( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) || pfcbT != this );
#endif


    Assert( Ptdb() == ptdbNil );
    Assert( Pidb() == pidbNil );
    Assert( WRefCount() == 0 );
    Assert( FWRefCountOK_() );
    Assert( PrceNewest() == prceNil );
    Assert( PrceOldest() == prceNil );


    delete this;

    AtomicDecrement( (LONG *)&pinst->m_cFCBAlloc );


    PERFOpt( cFCBCacheAlloc.Dec( pinst ) );
}



VOID FCB::PrepareForPurge( const BOOL fPrepareChildren )
{
    INST            *pinst = PinstFromIfmp( Ifmp() );
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );

    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );

#if defined( DEBUG ) && defined( SYNC_DEADLOCK_DETECTION )


    const BOOL fDBGONLYLockList = pinst->m_critFCBList.FNotOwner();

    if ( fDBGONLYLockList )
    {
        pinst->m_critFCBList.Enter();
    }

    if ( FInList() )
    {


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


            while ( pfcbT->PgnoFDP() != PgnoFDP() || pfcbT->Ifmp() != Ifmp() || pfcbT != this )
            {
                Assert( pfcbNil != pfcbT );
                pfcbT = pfcbT->PfcbNextList();
            }


            Assert( pfcbT == this );
        }
    }


    if ( fDBGONLYLockList )
    {
        pinst->m_critFCBList.Leave();
    }

#endif

    PERFOpt( cFCBSyncPurge.Inc( pinst ) );

#ifdef DEBUG
    DWORD cStalls = 0;
#endif

RetryLock:


    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );

    if ( !FLockTry_( LOCK_TYPE::ltWrite ) )
    {
        pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );


#ifdef DEBUG
        cStalls++;
        Assert( cStalls < 100 );
#endif


        PERFOpt( cFCBSyncPurgeStalls.Inc( pinst ) );


        UtilSleep( 10 );


        goto RetryLock;
    }        



    errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );


    Assert( errFCBHash == FCBHash::ERR::errSuccess || FDeleteCommitted() );


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


    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );



    Unlock_( LOCK_TYPE::ltWrite );


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


            if ( FCATHashActive( pinst ) )
            {
                CHAR szTable[JET_cbNameMost+1];



                EnterDML();
                OSStrCbCopyA( szTable, sizeof(szTable), Ptdb()->SzTableName() );
                LeaveDML();


                CATHashIDelete( this, szTable );
            }
        }
    }
}



VOID FCB::Purge( const BOOL fLockList, const BOOL fTerminating )
{
    INST    *pinst = PinstFromIfmp( Ifmp() );
#ifdef DEBUG
    FCB     *pfcbInHash;
#endif
    FCB     *pfcbT;
    FCB     *pfcbNextT;

    if ( fLockList )
    {
        Assert( pinst->m_critFCBList.FNotOwner() );
    }
    else
    {
        Assert( pinst->m_critFCBList.FOwner()
                || ( !FInLRU() && !FInList() ) );
    }
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );


    Enforce( WRefCount() == 0 );


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


    Assert( !FInHashTable( Ifmp(), PgnoFDP(), &pfcbInHash ) || pfcbInHash != this );


    Assert( WRefCount() == 0 );
    Assert( FWRefCountOK_() );
    Assert( PrceOldest() == prceNil );
    Assert( PrceNewest() == prceNil );

    if ( FTypeTable() )
    {


        pfcbT = PfcbNextIndex();
        SetPfcbNextIndex( pfcbNil );
        while ( pfcbT != pfcbNil )
        {


            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltWrite ) );
            Assert( pfcbT->IsUnlocked_( LOCK_TYPE::ltShared ) );


            Assert( !pfcbT->FInitialized() || pfcbT->FTypeSecondaryIndex() );
            Assert( ptdbNil == pfcbT->Ptdb() );
            Assert( !pfcbT->FInitialized() || pfcbT->PfcbTable() == this );
            Assert( !FInHashTable( pfcbT->Ifmp(), pfcbT->PgnoFDP(), &pfcbInHash ) ||
                    pfcbInHash != pfcbT );

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


    if ( Ptdb() != ptdbNil )
    {
        Assert( FTypeTable() || FTypeTemporaryTable() );


        pfcbT = Ptdb()->PfcbLV();
        Ptdb()->SetPfcbLV( pfcbNil );
        if ( pfcbT != pfcbNil )
        {



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


    if ( Pidb() != pidbNil )
    {
        Assert( FTypeTable() || FTypeTemporaryTable() );

        ReleasePidb();
    }

    if ( fLockList )
    {


        pinst->m_critFCBList.Enter();
    }


    if ( FInLRU() )
    {
#ifdef DEBUG
        RemoveAvailList_( fTrue );
#else
        RemoveAvailList_();
#endif
    }

    if ( FInList() )
    {


        RemoveList_();
    }

    if ( fLockList )
    {


        pinst->m_critFCBList.Leave();
    }


    Delete_( pinst );
}



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


    return fFalse;
}




INLINE BOOL FCB::FOutstandingVersions_()
{
    ENTERCRITICALSECTION    enterCritRCEList( &CritRCEList() );
    return ( prceNil != PrceOldest() );
}



VOID FCB::PurgeObjects_( INST* const pinst, const IFMP ifmp, const PGNO pgnoFDP, const BOOL fTerminating )
{
    Assert( pinst != pinstNil );
    Assert( ( ifmp == ifmpNil ) || ( pinst == PinstFromIfmp( ifmp ) ) );
    Assert( ( ifmp != ifmpNil ) || ( pgnoFDP == pgnoNull ) );
    Assert( ( ifmp != ifmpNil ) || fTerminating );

    OnDebug( const BOOL fShrinking = ( ifmp != ifmpNil ) && g_rgfmp[ifmp].FShrinkIsRunning() );
    OnDebug( const BOOL fReclaimingLeaks = ( ifmp != ifmpNil ) && g_rgfmp[ifmp].FLeakReclaimerIsRunning() );

    Expected( ( pgnoFDP == pgnoNull ) || fShrinking || fReclaimingLeaks );

    OnDebug( const BOOL fDetaching = ( ifmp != ifmpNil ) && g_rgfmp[ifmp].FDetachingDB() );

    Assert( fTerminating || fDetaching || fShrinking || g_fRepair || fReclaimingLeaks );

    Expected( !fShrinking || ( !fTerminating && !fDetaching && !g_fRepair ) );
    Expected( !fTerminating || ( pgnoFDP == pgnoNull ) );
    Expected( !fDetaching || ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) ) );
    Expected( !g_fRepair || ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) ) );

    FCB* pfcbNext = pfcbNil;
    FCB* pfcbThis = pfcbNil;

    Assert( pinst->m_critFCBList.FNotOwner() );


    pinst->m_critFCBList.Enter();


    pfcbThis = pinst->m_pfcbList;
    while ( pfcbThis != pfcbNil )
    {

        pfcbNext = pfcbThis->PfcbNextList();

        if ( !pfcbThis->FTemplateTable() &&
             (
               ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull ) ) ||

               ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) ) ||

               ( ( ifmp != ifmpNil ) && ( pgnoFDP != pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) &&
                 (
                   ( pfcbThis->PgnoFDP() == pgnoFDP ) ||
                   ( pfcbThis->FDerivedTable() &&
                     ( pfcbThis->Ptdb() != ptdbNil ) &&
                     ( pfcbThis->Ptdb()->PfcbTemplateTable() != pfcbNil ) &&
                     ( pfcbThis->Ptdb()->PfcbTemplateTable()->PgnoFDP() == pgnoFDP )
                   )
                 )
               )
             )
           )
        {

            Assert( pfcbThis->FTypeDatabase()       ||
                    pfcbThis->FTypeTable()          ||
                    pfcbThis->FTypeTemporaryTable() );

            pfcbThis->PrepareForPurge();


            pfcbThis->CloseAllCursors( fTerminating );
            pfcbThis->Purge( fFalse, fTerminating );
        }


        pfcbThis = pfcbNext;
    }


    pfcbThis = pinst->m_pfcbList;
    while ( pfcbThis != pfcbNil )
    {

        pfcbNext = pfcbThis->PfcbNextList();

        if (
             ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull ) ) ||

             ( ( ifmp != ifmpNil ) && ( pgnoFDP == pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) ) ||

             ( ( ifmp != ifmpNil ) && ( pgnoFDP != pgnoNull ) && ( ifmp == pfcbThis->Ifmp() ) && ( pfcbThis->PgnoFDP() == pgnoFDP ) )
           )
        {

            Assert( pfcbThis->FTypeDatabase()       ||
                    pfcbThis->FTypeTable()          ||
                    pfcbThis->FTypeTemporaryTable() );
            Assert( pfcbThis->FTemplateTable() );

            pfcbThis->PrepareForPurge();


            pfcbThis->CloseAllCursors( fTerminating );
            pfcbThis->Purge( fFalse, fTerminating );
        }


        pfcbThis = pfcbNext;
    }


    Expected( ( ifmp != ifmpNil ) || ( pgnoFDP != pgnoNull ) || ( pinst->m_pfcbList == pfcbNil ) );
    if ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull )  )
    {
        pinst->m_pfcbList = pfcbNil;
    }


    pinst->m_critFCBList.Leave();

#ifdef DEBUG

    if ( ( ifmp == ifmpNil ) && ( pgnoFDP == pgnoNull ) )
    {
        for ( DBID dbidT = dbidMin; dbidT < dbidMax; dbidT++ )
        {
            const IFMP  ifmpT   = pinst->m_mpdbidifmp[dbidT];
            if ( ifmpT < g_ifmpMax )
            {
                CATHashAssertCleanIfmp( ifmpT );
            }
        }
    }
    else
    {
        CATHashAssertCleanIfmpPgnoFDP( ifmp, pgnoFDP );
    }
#endif
}



VOID FCB::PurgeObject( const IFMP ifmp, const PGNO pgnoFDP )
{
    Assert( ifmp != ifmpNil );
    Assert( pgnoFDP != pgnoNull );
    FCB::PurgeObjects_( PinstFromIfmp( ifmp ), ifmp, pgnoFDP, fFalse  );
}



VOID FCB::PurgeDatabase( const IFMP ifmp, const BOOL fTerminating )
{
    Assert( ifmp != ifmpNil );
    FCB::PurgeObjects_( PinstFromIfmp( ifmp ), ifmp, pgnoNull, fTerminating );
}



VOID FCB::PurgeAllDatabases( INST* const pinst )
{
    Assert( pinst != pinstNil );
    FCB::PurgeObjects_( pinst, ifmpNil, pgnoNull, fTrue  );
}



VOID FCB::InsertHashTable()
{
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) || FTypeSort() );;


    Assert( !FInHashTable( Ifmp(), PgnoFDP() ) );

    INST            *pinst = PinstFromIfmp( Ifmp() );
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );
    FCBHashEntry    entryFCBHash( PgnoFDP(), this );


    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );


    errFCBHash = pinst->m_pfcbhash->ErrInsertEntry( &lockFCBHash, entryFCBHash );
    Assert( errFCBHash == FCBHash::ERR::errSuccess );


    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );

#ifdef DEBUG


    FCB *pfcbT;
    Assert( FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) );
    Assert( pfcbT == this );
#endif
}



VOID FCB::DeleteHashTable()
{
#ifdef DEBUG


    FCB *pfcbT;
    Assert( FInHashTable( Ifmp(), PgnoFDP(), &pfcbT ) );
    Assert( pfcbT == this );
#endif

    INST            *pinst = PinstFromIfmp( Ifmp() );
    FCBHash::ERR    errFCBHash;
    FCBHash::CLock  lockFCBHash;
    FCBHashKey      keyFCBHash( Ifmp(), PgnoFDP() );


    pinst->m_pfcbhash->WriteLockKey( keyFCBHash, &lockFCBHash );


    errFCBHash = pinst->m_pfcbhash->ErrDeleteEntry( &lockFCBHash );
    Assert( errFCBHash == FCBHash::ERR::errSuccess );


    pinst->m_pfcbhash->WriteUnlockKey( &lockFCBHash );


    Assert( !FInHashTable( Ifmp(), PgnoFDP() ) );
}


VOID FCB::CheckFCBLockingForLink_()
{
#ifdef DEBUG
    INST *pinst = PinstFromIfmp( Ifmp() );

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


    pinst->m_critFCBList.Enter();

    if ( FInLRU() )
    {

        Lock();

        if ( WRefCount() == 0 &&
             PgnoFDP() != pgnoSystemRoot &&
             !FDeletePending() &&
             !FDeleteCommitted() &&
             !FOutstandingVersions_() &&
             0 == CTasksActive() &&
             !FHasCallbacks_( pinst ) )
        {



            AssertTrack( fFalse, "LinkingPurgeableFcb" );
        }
        Unlock_( LOCK_TYPE::ltWrite );
    }

    pinst->m_critFCBList.Leave();
#endif
}

ERR FCB::ErrLink( FUCB *pfucb )
{
    Assert( pfucb != pfucbNil );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    CheckFCBLockingForLink_();

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



VOID FCB::AttachRCE( RCE * const prce )
{
    Assert( CritRCEList().FOwner() );

    Assert( prce->Ifmp() == Ifmp() );
    Assert( prce->PgnoFDP() == PgnoFDP() );

    if ( prce->Oper() != operAddColumn )
    {
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


VOID FCB::DetachRCE( RCE * const prce )
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

VOID FCB::Unlink( FUCB *pfucb, const BOOL fPreventMoveToAvail )
{
    Assert( pfucb != pfucbNil );
    Assert( pfucb->u.pfcb != pfcbNil );
    Assert( this == pfucb->u.pfcb );

#ifdef DEBUG


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
#endif

    DecrementRefCountAndUnlink_( pfucb, fTrue, fPreventMoveToAvail );

}

ERR FCB::ErrSetUpdatingAndEnterDML( PIB *ppib, BOOL fWaitOnConflict )
{
    ERR err = JET_errSuccess;

    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    if ( !FFixedDDL() )
    {
        Assert( FTypeTable() );
        Assert( !FTemplateTable() );
        Assert( Ptdb() != ptdbNil );

CheckIndexing:
        Ptdb()->EnterUpdating();
        EnterDML_();

        if ( Pidb() != pidbNil && Pidb()->FVersionedCreate() )
        {
            err = ErrFILEIAccessIndex( ppib, this, this );
            if ( JET_errIndexNotFound == err )
            {
                LeaveDML_();
                ResetUpdating_();

                if ( fWaitOnConflict )
                {
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
#endif

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

    CallR( ErrPIBCheck( ppib ) );
    Assert( pfucb != pfucbNil );
    CheckTable( ppib, pfucb );


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
#else

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
#endif

        pfcb->EnterDDL();
        pcbdescRemove->cbtyp = JET_cbtypNull;
        pfcb->LeaveDDL();

#ifdef VERSIONED_CALLBACKS
    }
#endif

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


    pinst->m_critFCBList.Enter();


    for ( pfcbT = pinst->m_pfcbList; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextList() )
    {
        Assert( pfcbT->PrceOldest() == prceNil );
    }


    pinst->m_critFCBList.Leave();
}
#endif


VOID FCB::IncrementRefCount()
{
    IncrementRefCount_( fFalse );
}

VOID FCB::IncrementRefCount_( BOOL fOwnWriteLock )
{
    ERR err = ErrIncrementRefCountAndLink_( pfucbNil, fOwnWriteLock );
    Enforce( JET_errSuccess == err );
}


VOID FCB::DecrementRefCountAndUnlink_( FUCB *pfucb, const BOOL fLockList, const BOOL fPreventMoveToAvail )
{
    INST *pinst = PinstFromIfmp( Ifmp() );
    BOOL fTryPurge = fFalse;

    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );
    Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );

    Lock_( LOCK_TYPE::ltShared );

    Assert( FWRefCountOK_() );

    if (pfucbNil != pfucb)
    {
        pfucb->u.pfcb = pfcbNil;

        FucbList().LockForModification();
        FucbList().Remove( pfucb );
        FucbList().UnlockForModification();
    }


    LONG lResultRefCount = AtomicDecrement( &m_wRefCount );
    if ( lResultRefCount < 0 )
    {
        Enforce( fFalse );
    }

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

    if ( !FTypeTable() && !pinst->FRecovering() )
    {
        Unlock_( LOCK_TYPE::ltShared );
        return;
    }


    AtomicIncrement( &m_wRefCount );
    Unlock_( LOCK_TYPE::ltShared );
    if ( fLockList )
    {
        Assert( pinst->m_critFCBList.FNotOwner() );
        pinst->m_critFCBList.Enter();
    }
    Lock();
    AtomicDecrement( &m_wRefCount );

    Assert( pinst->m_critFCBList.FOwner() );

    if ( FTypeTable() && ( 0 == WRefCount() ) && !FInLRU() )
    {
        InsertAvailListMRU_();
        fTryPurge = fTrue;
    }

    Unlock_( LOCK_TYPE::ltWrite );

    if ( fTryPurge && ( pfucbNil != pfucb ) && FTryPurgeOnClose() )
    {
        BOOL fPurgeable = FCheckFreeAndPurge_( pfucb->ppib, fFalse );

        if ( fPurgeable )
        {
            PERFOpt( cFCBPurgeOnClose.Inc( pinst ) );
        }
    }

    if ( fLockList )
    {
        pinst->m_critFCBList.Leave();
    }

}



INLINE ERR FCB::ErrIncrementRefCountAndLink_( FUCB *pfucb, const BOOL fOwnWriteLock )
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( IsUnlocked_( LOCK_TYPE::ltShared ) );

    if ( fOwnWriteLock )
    {
        Assert( IsLocked_( LOCK_TYPE::ltWrite ) );
        Expected( pfucbNil == pfucb );
    }
    else
    {
        Assert( IsUnlocked_( LOCK_TYPE::ltWrite ) );
        Lock_( LOCK_TYPE::ltShared );
    }

    Assert( FWRefCountOK_() );


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

    LONG lResultRefCount = AtomicIncrement( &m_wRefCount );

    if ( lResultRefCount <= 0 )
    {
        Enforce( fFalse );
    }

    if ( pinst->m_pFCBRefTrace != NULL )
    {
        OSTraceWriteRefLog( pinst->m_pFCBRefTrace, lResultRefCount, this );
    }

    if ( 1 == lResultRefCount && FTypeTable() )
    {

        Assert( pinst->m_critFCBList.FNotOwner() );
        if ( !fOwnWriteLock )
        {
            Unlock_( LOCK_TYPE::ltShared );
        }
        else
        {
            Unlock_( LOCK_TYPE::ltWrite );
        }

        pinst->m_critFCBList.Enter();
        Lock_( LOCK_TYPE::ltWrite );

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



VOID FCB::RemoveAvailList_(
#ifdef DEBUG
    const BOOL fPurging
#endif
    )
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    EnforceSz( FTypeTable(), "AvailFcbMustBeTable" );

    Assert( FCBCheckAvailList_( fTrue, fPurging ) );


    FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU();
    FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU();


    if ( PfcbMRU() != pfcbNil )
    {
        Assert( *ppfcbAvailMRU != this );
        Assert( PfcbMRU()->PfcbLRU() == this );
        PfcbMRU()->SetPfcbLRU( PfcbLRU() );
    }
    else
    {
        Assert( *ppfcbAvailMRU == this );
        *ppfcbAvailMRU = PfcbLRU();
    }
    if ( PfcbLRU() != pfcbNil )
    {
        Assert( *ppfcbAvailLRU != this );
        Assert( PfcbLRU()->PfcbMRU() == this );
        PfcbLRU()->SetPfcbMRU( PfcbMRU() );
    }
    else
    {
        Assert( *ppfcbAvailLRU == this );
        *ppfcbAvailLRU = PfcbMRU();
    }
    ResetInLRU();
    SetPfcbMRU( pfcbNil );
    SetPfcbLRU( pfcbNil );
    pinst->m_cFCBAvail--;


    PERFOpt( cFCBCacheAllocAvail.Dec( pinst ) );

    Assert( FCBCheckAvailList_( fFalse, fPurging ) );
}



VOID FCB::InsertAvailListMRU_()
{
    INST *pinst = PinstFromIfmp( Ifmp() );

    Assert( FCBCheckAvailList_( fFalse, fFalse ) );

    Enforce( FTypeTable() );


    FCB **ppfcbAvailMRU = pinst->PpfcbAvailMRU();
    FCB **ppfcbAvailLRU = pinst->PpfcbAvailLRU();


    if ( *ppfcbAvailMRU != pfcbNil )
    {
        (*ppfcbAvailMRU)->SetPfcbMRU( this );
    }
    SetPfcbLRU( *ppfcbAvailMRU );
    *ppfcbAvailMRU = this;
    if ( *ppfcbAvailLRU == pfcbNil )
    {
        *ppfcbAvailLRU = this;
    }
    SetInLRU();
    pinst->m_cFCBAvail++;


    PERFOpt( cFCBCacheAllocAvail.Inc( pinst ) );

    Assert( FCBCheckAvailList_( fTrue, fFalse ) );
}



VOID FCB::InsertList()
{
    INST *pinst = PinstFromIfmp( Ifmp() );
    Assert( pinst->m_critFCBList.FNotOwner() );


    pinst->m_critFCBList.Enter();


    Assert( !FInList() );
    Assert( PfcbNextList() == pfcbNil );
    Assert( PfcbPrevList() == pfcbNil );


    Assert( pinst->m_pfcbList == pfcbNil ||
            pinst->m_pfcbList->PfcbPrevList() == pfcbNil );


    if ( pinst->m_pfcbList != pfcbNil )
    {
        pinst->m_pfcbList->SetPfcbPrevList( this );
    }
    SetPfcbNextList( pinst->m_pfcbList );
    pinst->m_pfcbList = this;
    SetInList();


    Assert( pinst->m_pfcbList != pfcbNil );
    Assert( pinst->m_pfcbList->PfcbPrevList() == pfcbNil );


    pinst->m_critFCBList.Leave();
}


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


    Assert( FInList() );


    Assert( pinst->m_pfcbList != pfcbNil );
    Assert( pinst->m_pfcbList->PfcbPrevList() == pfcbNil );


    if ( PfcbPrevList() != pfcbNil )
    {
        Assert( pinst->m_pfcbList != this );
        Assert( PfcbPrevList()->PfcbNextList() == this );
        PfcbPrevList()->SetPfcbNextList( PfcbNextList() );
    }
    else
    {
        Assert( pinst->m_pfcbList == this );
        pinst->m_pfcbList = PfcbNextList();
    }
    if ( PfcbNextList() != pfcbNil )
    {
        Assert( PfcbNextList()->PfcbPrevList() == this );
        PfcbNextList()->SetPfcbPrevList( PfcbPrevList() );
    }
    ResetInList();
    SetPfcbNextList( pfcbNil );
    SetPfcbPrevList( pfcbNil );


    Assert( pinst->m_pfcbList == pfcbNil ||
            pinst->m_pfcbList->PfcbPrevList() == pfcbNil );
}


BOOL FCB::FUseOLD2()
{
    if( FTypeTable()
        && ( FRetrieveHintTableScanForward() || FRetrieveHintTableScanBackward() ) )
    {
        return fTrue;
    }
    return fFalse;
}

