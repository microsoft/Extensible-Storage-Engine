// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

//  ****************************************************************
//  TASK
//  ****************************************************************

THREADSTATSCOUNTERS g_tscountersDbTasks;

#ifdef PERFMON_SUPPORT

LONG LDBTASKPageReferencedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageReferenced.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPageReadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageRead.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPagePrereadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPagePreread.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPageDirtiedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageDirtied.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKPageRedirtiedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cPageRedirtied.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKLogRecordCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cLogRecord.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBTASKLogBytesCEFLPv( LONG iInstance, VOID * pvBuf )
{
    g_tscountersDbTasks.cbLogRecord.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT


INT TASK::g_tasksDropped = 0;

TASK::TASK()
{
}

TASK::~TASK()
{
}

DWORD TASK::DispatchGP( void *pvThis )
{
    ERR             err;
    TASK * const    ptask   = (TASK *)pvThis;
    INST * const    pinst   = ptask->PInstance();
    PIB *           ppibT;

    UPDATETHREADSTATSCOUNTERS   updatetscounters( pinst, &g_tscountersDbTasks );

    Assert( pinstNil != pinst );

    err = pinst->ErrGetSystemPib( &ppibT );
    if ( err >= JET_errSuccess )
    {
        err = ptask->ErrExecute( ppibT );
        pinst->ReleaseSystemPib( ppibT );
    }
    else
    {
        ++g_tasksDropped;
        if( 0 == g_tasksDropped % 100 )
        {
            //  UNDONE: eventlog
            //
            //  The system was unable to perform background tasks due to low resources
            //  If this problem persists there may be a performance problem with the server
        }
    }

    if ( err < JET_errSuccess )
    {
        ptask->HandleError( err );
    }

    //  the TASK must have been allocated with "new"
    delete ptask;
    return 0;
}

VOID TASK::Dispatch( PIB * const ppib, const ULONG_PTR ulThis )
{
    UPDATETHREADSTATSCOUNTERS   updatetscounters( PinstFromPpib( ppib ), &g_tscountersDbTasks );

    TASK * const    ptask   = (TASK *)ulThis;
    const ERR       err     = ptask->ErrExecute( ppib );

    if ( err < 0 )
    {
        ptask->HandleError( err );
    }

    //  the TASK must have been allocated with "new"
    delete ptask;
}


//  ****************************************************************
//  DBTASK
//  ****************************************************************

DBTASK::DBTASK( const IFMP ifmp ) :
    m_ifmp( ifmp )
{
    FMP * const pfmp = &g_rgfmp[m_ifmp];
    CallS( pfmp->RegisterTask() );
}

ERR DBTASK::ErrExecute( PIB * const ppib )
{
    // Prevent any tasks from being issued while we're shrinking or reclaiming leaked space.
    // Known cases:
    //  1- MERGEAVAILEXTTASK.
    FMP * const pfmp = &g_rgfmp[m_ifmp];
    if ( pfmp->FShrinkIsRunning() || pfmp->FLeakReclaimerIsRunning() )
    {
        return ErrERRCheck( JET_errTaskDropped );
    }

    return ErrExecuteDbTask( ppib );
}

INST *DBTASK::PInstance()
{
    return PinstFromIfmp( m_ifmp );
}

DBTASK::~DBTASK()
{
    FMP * const pfmp = &g_rgfmp[m_ifmp];
    CallS( pfmp->UnregisterTask() );
}

//  ****************************************************************
//  DBREGISTEROLD2TASK
//  ****************************************************************

DBREGISTEROLD2TASK::DBREGISTEROLD2TASK(
    _In_ const IFMP ifmp,
    _In_z_ const char * const szTableName,
    _In_opt_z_ const char * const szIndexName,
    _In_ DEFRAGTYPE defragtype ) :
    DBTASK( ifmp ), m_defragtype( defragtype )
{
    OSStrCbCopyA( m_szTableName, sizeof(m_szTableName), szTableName );
    m_szIndexName[ 0 ] = '\0';

    if ( szIndexName != NULL )
    {
        OSStrCbCopyA( m_szIndexName, sizeof( m_szIndexName ), szIndexName );
    }
}

ERR DBREGISTEROLD2TASK::ErrExecuteDbTask( _In_opt_ PIB *const ppib )
{
    return ErrOLDRegisterObjectForOLD2( m_ifmp, m_szTableName, m_szIndexName, m_defragtype );
}

void DBREGISTEROLD2TASK::HandleError( const ERR err )
{
    // ignore errors. we'll re-register the table later
}

//  ****************************************************************
//  DBTASK
//  ****************************************************************

DBEXTENDTASK::DBEXTENDTASK( const IFMP ifmp ) :
    DBTASK( ifmp )
{
}

ERR DBEXTENDTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR         err         = JET_errSuccess;
    FMP *       pfmp        = g_rgfmp + m_ifmp;

    pfmp->AcquireIOSizeChangeLatch();

    QWORD cbSize = 0;
    err = pfmp->Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical );

    if ( err >= JET_errSuccess )
    {
        //  check to make sure no one beat us to the file extension
        //
        const QWORD cbSizeTarget = pfmp->CbFsFileSizeAsyncTarget();
        Assert( cbSizeTarget >= cbSize );

        if ( cbSizeTarget > cbSize )
        {
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->iorReason.SetIorp( iorpDatabaseExtension );
            tcScope->SetDwEngineObjid( objidSystemRoot );

            err = pfmp->Pfapi()->ErrSetSize( *tcScope,
                                                cbSizeTarget,
                                                fTrue,
                                                QosSyncDefault( pfmp->Pinst() ) );
            Assert( err <= JET_errSuccess );

            if ( err < JET_errSuccess )
            {
                // Rollback target, let extension code retry.
                pfmp->SetFsFileSizeAsyncTarget( cbSize );
            }
        }
    }

    pfmp->ReleaseIOSizeChangeLatch();

    return err;
}

VOID DBEXTENDTASK::HandleError( const ERR err )
{
    //  ignore errors - db extension will simply be
    //  retried synchronously
}

//  ****************************************************************
//  RECTASK
//  ****************************************************************

RECTASK::RECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    DBTASK( ifmp ),
    m_pgnoFDP( pgnoFDP ),
    m_pfcb( pfcb ),
    m_cbBookmarkKey( bm.key.Cb() ),
    m_cbBookmarkData( bm.data.Cb() )
{
    //  don't fire off async tasks on the temp. database because the
    //  temp. database is simply not equipped to deal with concurrent access
    AssertRTL( !FFMPIsTempDB( ifmp ) );

    Assert( !bm.key.FNull() );
    Assert( m_cbBookmarkKey <= sizeof( m_rgbBookmarkKey ) );
    Assert( m_cbBookmarkData <= sizeof( m_rgbBookmarkData ) );
    bm.key.CopyIntoBuffer( m_rgbBookmarkKey, sizeof( m_rgbBookmarkKey ) );
    memcpy( m_rgbBookmarkData, bm.data.Pv(), min( sizeof( m_rgbBookmarkData ), m_cbBookmarkData ) );

    //  if coming from version cleanup, refcount may be zero, so the only thing
    //  currently keeping this FCB pinned is the RCE that spawned this task
    //  if coming from OLD, there must already be a cursor open on this
    Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
    Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 0 );

    //  pin the FCB by incrementing its refcnt

//  Assert( m_pfcb->FNeedLock() );
//  m_pfcb->IncrementRefCount();
    m_pfcb->RegisterTask();

//  Assert( prceNil != m_pfcb->PrceOldest() || m_pfcb->WRefCount() > 1 );
}

RECTASK::~RECTASK()
{
    Assert( m_pfcb->PgnoFDP() == m_pgnoFDP );

    //  release the FCB by decrementing its refcnt

//  Assert( m_pfcb->WRefCount() > 0 );
//  Assert( m_pfcb->FNeedLock() );
//  m_pfcb->Release();
    m_pfcb->UnregisterTask();
}

ERR RECTASK::ErrOpenCursor( PIB * const ppib, FUCB ** ppfucb )
{
    ERR     err;

    Assert( m_pfcb->FInitialized() );
    Assert( m_pfcb->CTasksActive() > 0 );
    Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );

    Call( ErrDIROpen( ppib, m_pfcb, ppfucb ) );
    Assert( pfucbNil != *ppfucb );
    FUCBSetIndex( *ppfucb );

HandleError:
    return err;
}

ERR RECTASK::ErrOpenCursorAndGotoBookmark( PIB * const ppib, FUCB ** ppfucb )
{
    ERR         err;
    BOOKMARK    bookmark;

    CallR( ErrOpenCursor( ppib, ppfucb ) );
    GetBookmark( &bookmark );
    Call( ErrDIRGotoBookmark( *ppfucb, bookmark ) );

HandleError:
    if( err < 0 )
    {
        DIRClose( *ppfucb );
        *ppfucb = pfucbNil;     //  set to NULL in case caller tries to close it again
    }

    return err;
}

VOID RECTASK::GetBookmark( __out BOOKMARK * const pbookmark ) const
{
    pbookmark->key.Nullify();
    pbookmark->key.suffix.SetPv( const_cast<BYTE *>( m_rgbBookmarkKey ) );
    pbookmark->key.suffix.SetCb( m_cbBookmarkKey );
    pbookmark->data.SetPv( const_cast<BYTE *>( m_rgbBookmarkData ) );
    pbookmark->data.SetCb( m_cbBookmarkData );
}

bool RECTASK::FBookmarkIsLessThan( const RECTASK * ptask1, const RECTASK * ptask2 )
{
    BOOKMARK    bookmark1;
    BOOKMARK    bookmark2;

    ptask1->GetBookmark( &bookmark1 );
    ptask2->GetBookmark( &bookmark2 );

    return ( CmpBM( bookmark1, bookmark2 ) < 0 );
}

//  ****************************************************************
//  DELETERECTASK
//  ****************************************************************

DELETERECTASK::DELETERECTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm )
{
}

ERR DELETERECTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR         err;
    FUCB *      pfucb       = pfucbNil;
    BOOKMARK    bookmark;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortRecTask );

    Assert( 0 == ppib->Level() );
    CallR( ErrOpenCursor( ppib, &pfucb ) );
    Assert( pfucbNil != pfucb );

    GetBookmark( &bookmark );

    Call( ErrBTDelete( pfucb, bookmark ) );

HandleError:
    DIRClose( pfucb );

    return err;
}

VOID DELETERECTASK::HandleError( const ERR err )
{
    //  this should only be called if the task failed for some reason
    //
    Assert( err < JET_errSuccess );

    PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
}


//  ****************************************************************
//  FINALIZETASK
//  ****************************************************************

template< typename TDelta >
FINALIZETASK<TDelta>::FINALIZETASK(
    const PGNO      pgnoFDP,
    FCB * const     pfcb,
    const IFMP      ifmp,
    const BOOKMARK& bm,
    const USHORT    ibRecordOffset,
    const BOOL      fCallback,
    const BOOL      fDelete ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm ),
    m_ibRecordOffset( ibRecordOffset ),
    m_fCallback( !!fCallback ),
    m_fDelete( !!fDelete )
{
    //  We do not allow both.
    Assert( m_fCallback ^ m_fDelete );
    static BOOL s_fLimitFinalizeTaskNyi = fFalse;

    if ( m_fCallback && !FNegTest( fInvalidAPIUsage ) && !s_fLimitFinalizeTaskNyi )
    {
        //  See above "Historical Notes" below function.
        AssertTrack( fFalse, "NyiFinalizeBehaviorInFinalizeTask" );
        s_fLimitFinalizeTaskNyi = fTrue;
    }
}

//  JET_bitColumnDeleteOnZero / m_fDelete vs. JET_bigColumnFinalize / m_fCallback ...
//
//  Historical Notes: 
//   - The very first JET_bitColumnFinalize behaviour was just to delete the record when the escrow column 
//      became zero, and AD consumed this behavior for Windows Server 2003 (which remember is off the Exch  
//      2000 code base).  Exchange may have also consumed this behavior.
//   - For Exchange 2000 or maybe Exchange 2003, Store changed their mind and wanted the behavior to JUST 
//      trigger a callback with NO delete behavior, BUT when we implemented this, we poorly choose to:
//          1. JET_bitColumnFinalize for Windows - leave existing delete on zero behavior.
//          2. JET_bitColumnFinalize for Exchange - the new callback-only behavior under this EXISTING grbit. 
//          3. A new JET_bitColumnDeleteOnZero for the OLD behavior in both branches of our source code.
//      Adding a new grbit for the old behavior was a bad call, the grbit should've been for the new behavior
//      only.  So now there are two grbits that mean the same thing as far as windows is concerned:
//          * JET_bitColumnFinalize has been _behaving_ as delete on zero in windows for fifteen+ years.
//          * JET_bitColumnDeleteOnZero ALSO behaves this way in windows and is more obviously documented as
//              such (and in use as such).
//   - AD was migrated to use JetConvertDDL() to upgrade to the new JET_bitColumnDeleteOnZero in maybe Windows
//      Server 2008.
//   - When we first published our API, we didn't protect ourselves against poor use of JET_bitColumnFinalize,
//      but we did note in MSDN clients should not sue _bitColumnFinalize, but _bitColumnDeleteOnZero
//      Note: MOM Agent also uses the new JET_bitColumnDeleteOnZero grbit.
//   - Finally, Exchange abandoned usage of JET_bitColumnFinalize with ManagedStore rewrite.
//
//  This change (the one accompanying this big block comment) removed all the old / callback only Exchange 
//  behavior to consolidate the code base.  As well as removed all the testing of the callback behavior.
//  
//  Neither bit can be guaranteed to be removed from a Windows perspective.  And they both essentially behave 
//  the same way now (in both branchs / "SKUs" of ESE).  But since we declared this bit un-welcome in MSDN 
//  from the beginning we'll start optics to see if anyone uses it, to hopefully make it better if anyone
//  wants to resurrect the behavior.
//
//  If we ever re-implement the callback behavior, then the recommendation would be:
//      Just make the JET_bitColumnFinalize trigger the callback, but have the delete depend upon a special 
//      signal error code returned from the callback.  No client today should asks for the finalize callback 
//      because it is not reliably triggered in NT (only for DbScan/OLD deletes ironically).  And so then there 
//      is low compat problems, no need for a new FFIELD-based column grbit (only 6 left as of mid 2017), and
//      a way to define the behavior to optional delete, which the original store had wanted anyways. ;-)
//      One concern to work out: original callback model was isolated in a RO transaction ... why?  Was this 
//      important to avoid some sort of conflict.  Also probably resurrect some of the testing we deleted with
//      this change.
//

template< typename TDelta >
ERR FINALIZETASK<TDelta>::ErrExecuteDbTask( PIB * const ppib )
{
    ERR     err;
    FUCB *  pfucb       = pfucbNil;

    Assert( 0 == ppib->Level() );
    CallR( ErrOpenCursorAndGotoBookmark( ppib, &pfucb ) );
    Assert( pfucbNil != pfucb );

    //  See above "Historical Notes" above function.
    Assert( m_fCallback ^ m_fDelete );
    Assert( m_fDelete || FNegTest( fInvalidAPIUsage ) );

    BOOL    fInTrx      = fFalse;

    Call( ErrDIRBeginTransaction( ppib, 36133, NO_GRBIT ) );
    fInTrx = fTrue;

    Call( ErrDIRGet( pfucb ) );

    //  Finalizable columns are 32-bit and 64-bit signed types
    TDelta  tColumnValue;
    tColumnValue = *( (UnalignedLittleEndian< TDelta > *)( (BYTE *)pfucb->kdfCurr.data.Pv() + m_ibRecordOffset ) );

    //  verify refcount is still at zero and there are
    //  no outstanding record updates
    const BOOL fDeletable = 0 == tColumnValue
                    && !FVERActive( pfucb, pfucb->bmCurr )
                    && !pfucb->u.pfcb->FDeletePending();

    CallS( ErrDIRRelease( pfucb ) );

    if ( fDeletable )
    {
        Call( ErrIsamDelete( ppib, pfucb ) );
    }

    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    Assert( 0 == ppib->Level() );
    fInTrx = fFalse;

HandleError:
    if ( fInTrx )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    Assert( pfucbNil != pfucb );
    DIRClose( pfucb );
    return err;
}

template< typename TDelta >
VOID FINALIZETASK<TDelta>::HandleError( const ERR err )
{
    //  this should only be called if the task failed for some reason
    //
    Assert( err < JET_errSuccess );

    //  we don't expect to drop this during shrink or when reclaimed leaked space
    //
    AssertTrack( !g_rgfmp[m_ifmp].FShrinkIsRunning(), "FinalizeTaskDroppedOnShrink" );
    AssertTrack( !g_rgfmp[m_ifmp].FLeakReclaimerIsRunning(), "FinalizeTaskDroppedOnLeakReclaim" );

    PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
}

// Explicitly instantiate the only allowed legal instances of this template
template class FINALIZETASK<LONG>;
template class FINALIZETASK<LONGLONG>;


//  ****************************************************************
//  DELETELVTASK
//  ****************************************************************

DELETELVTASK::DELETELVTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm )
{
}

ERR DELETELVTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR err = JET_errSuccess;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortRecTask );

    // The below private param controls flighting of the synchronous cleanup and allow a revert to old behavior if necessary.
    // Additionally, need to make sure the format feature is enabled
    if ( UlParam( PinstFromPpib( ppib ), JET_paramFlight_SynchronousLVCleanup) &&
         g_rgfmp[m_ifmp].ErrDBFormatFeatureEnabled( JET_efvSynchronousLVCleanup ) >= JET_errSuccess )
    {
        Call( ErrExecute_SynchronousCleanup( ppib ) );
    }
    else
    {
        Call( ErrExecute_LegacyVersioned(ppib ) );
    }

HandleError:
    return err;
}

// Flag-delete each LV chunk, starting from the end and moving towards the root. Version 
// store entries will take care of actually removing the nodes at some later time.
ERR DELETELVTASK::ErrExecute_LegacyVersioned( PIB * const ppib )
{
    ERR             err                     = JET_errSuccess;
    FUCB *          pfucb                   = pfucbNil;
    BOOL            fInTrx                  = fFalse;
    ULONG           ulLVDeleteOffset        = 0;
    CPG             cpgLVToDelete           = 0;
    ULONG           citer                   = 0;
    BOOKMARK        bookmark;
    LVROOT2         lvroot = { 0 };
    DATA            data;
    const LONG      cbLVChunkMost = m_pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG     cbLVDataToDeletePerTrx  = ULONG( cbLVChunkMost * g_cpgLVDataToDeletePerTrx );


    bookmark.key.Nullify();
    bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
    bookmark.key.suffix.SetCb( m_cbBookmarkKey );
    bookmark.data.SetPv( m_rgbBookmarkData );
    bookmark.data.SetCb( m_cbBookmarkData );

    Assert( FIsLVRootKey( bookmark.key ) );
    Assert( 0 == m_cbBookmarkData );

    Assert( m_pfcb->FTypeLV() );
    CallR( ErrOpenCursor( ppib, &pfucb ) );

    do
    {

        // Before starting a new trx to delete a new chunk of LV, lets check that we're 
        // not out of log space ...
        Call( PinstFromIfmp( pfucb->ifmp )->m_plog->ErrLGCheckState() );

        Call( ErrDIRGotoBookmark( pfucb, bookmark ) );

        Call( ErrDIRBeginTransaction( ppib, 46373, NO_GRBIT ) );
        fInTrx = fTrue;

        Call( ErrDIRGet( pfucb ) );

        Assert( sizeof(LVROOT) == pfucb->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucb->kdfCurr.data.Cb() );
        Assert( FIsLVRootKey( pfucb->kdfCurr.key ) );

        //  extract the LV root
        //
#pragma warning(suppress: 26015)
        UtilMemCpy( &lvroot, pfucb->kdfCurr.data.Pv(), min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );
        data.SetPv( &lvroot );
        data.SetCb( min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );


        //  in order for us to delete the LV, it must have a refcount of 0 and there
        //  must be no other versions
        //
        //  on iterations of the loop beyond the first, ideally we'd like to verify
        //  that no versions have been generated other than for the size updates
        //  generated by this session, but there's currently no way to check that,
        //  so the next-best thing is to verify that the refcount is still 0, that
        //  the size haven't changed size the previous loop iteration, and that
        //  there are no other versions active relative to the current transaction
        //  (mainly, we miss the case where a session comes in after the transaction
        //  from the previous iteration started, modifies the LV somehow without
        //  modifying the refcount or the size, then commits before the transaction
        //  from this iteration started, but even if this theoretically-impossible
        //  scenario did happen, since the refcount is still 0, I think it's safe
        //  to continue deleting the LV anyway without causing problems/conflicts
        //  for the other session)
        //
        if ( 0 != lvroot.ulReference
            || ( 0 != citer && lvroot.ulSize != ulLVDeleteOffset )
            || FDIRActiveVersion( pfucb, ( 0 != citer ? ppib->trxBegin0 : TrxOldest( PinstFromPpib( ppib ) ) ) ) )
        {
            //  should never get into the case where we were able
            //  to delete some chunks (ie. citer > 0), but then some references
            //  to the LV suddenly popped up somehow
            //
            Assert( 0 == citer );
            Error( ErrERRCheck( JET_errRecordNotDeleted ) );
        }

        //  after the first iteration, the current size of the LV
        //  should be on a chunk boundary
        //
        Assert( 0 == citer
            || 0 == lvroot.ulSize % cbLVDataToDeletePerTrx );

        if ( lvroot.ulSize > cbLVDataToDeletePerTrx )
        {
            //  LV is still too big to delete in one transaction, so
            //  lop some more off the end (ending up on a chunk boundary)
            //
            ulLVDeleteOffset = ( ( lvroot.ulSize - 1 ) / cbLVDataToDeletePerTrx ) * cbLVDataToDeletePerTrx;

            //  verify we're on a chunk boundary
            //
            Assert( 0 == ulLVDeleteOffset % cbLVDataToDeletePerTrx );
            Assert( 0 == cbLVDataToDeletePerTrx % cbLVChunkMost );
            Assert( ulLVDeleteOffset < lvroot.ulSize );

            //  compute how many pages we'll be visiting so that we can preread them
            //
            cpgLVToDelete = ( lvroot.ulSize - ulLVDeleteOffset + cbLVChunkMost - 1 ) / cbLVChunkMost;

            //  update LV root with truncated size
            //
            lvroot.ulSize = ulLVDeleteOffset;
            Assert( data.Pv() == &lvroot );
            Assert( data.Cb() == sizeof(LVROOT) || data.Cb() == sizeof(LVROOT2) );
            Call( ErrDIRReplace( pfucb, data, fDIRNull ) );
            Call( ErrDIRGet( pfucb ) ); // replace releases the latch, re-establish so ErrRECDeleteLV() is on the LV root...
        }
        else
        {
            ulLVDeleteOffset = 0;
            cpgLVToDelete = ( lvroot.ulSize + cbLVChunkMost - 1 ) / cbLVChunkMost;
        }
        
        FUCBSetPrereadForward( pfucb, cpgLVToDelete );

        //  CONSIDER: should we be calling ErrLVTruncate() instead??
        //
        Call( ErrRECDeleteLV_LegacyVersioned( pfucb, ulLVDeleteOffset, fDIRNull ) );

        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
    
        Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
        fInTrx = fFalse;

        citer++;
    }
    while ( 0 != ulLVDeleteOffset );

HandleError:
    if ( fInTrx )
    {
        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }
    return err;
}

// Unversioned-flag-delete each chunk and immediately bt-delete the node
ERR DELETELVTASK::ErrExecute_SynchronousCleanup( PIB * const ppib )
{
    ERR             err                     = JET_errSuccess;
    FUCB *          pfucb                   = pfucbNil;
    BOOL            fInTrx                  = fFalse;
    ULONG           cChunksToDelete         = 0;
    ULONG           cRemainChunksToDelete   = 0;
    CPG             cpgLVToDelete           = 0;
    ULONG           citer                   = 0;
    BOOKMARK        bookmark;
    LVROOT2         lvroot                  = { 0 };
    DATA            data;
    const LONG      cbLVChunkMost           = m_pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG     cChunksPerPage          = ULONG( g_rgfmp[ m_pfcb->Ifmp() ].CbPage() / cbLVChunkMost );
    const ULONG     cChunksToDeletePerTrx   = ULONG( g_cpgLVDataToDeletePerTrx * cChunksPerPage );
    ULONG           ulLVSize                = 0;

    bookmark.key.Nullify();
    bookmark.key.suffix.SetPv( m_rgbBookmarkKey );
    bookmark.key.suffix.SetCb( m_cbBookmarkKey );
    bookmark.data.SetPv( m_rgbBookmarkData );
    bookmark.data.SetCb( m_cbBookmarkData );

    Assert( FIsLVRootKey( bookmark.key ) );
    Assert( 0 == m_cbBookmarkData );

    Assert( m_pfcb->FTypeLV() );
    CallR( ErrOpenCursor( ppib, &pfucb ) );

    do
    {

        // Before starting a new trx to delete a new chunk of LV, lets check that we're 
        // not out of log space ...
        Call( PinstFromIfmp( pfucb->ifmp )->m_plog->ErrLGCheckState() );

        err = ErrDIRGotoBookmark( pfucb, bookmark );
        if ( JET_errNoCurrentRecord == err || JET_errWriteConflict == err )
        {
            // The LV is no more
            err = JET_errSuccess;
            break;
        }
        Call( err );

        Assert( 0 == ppib->Level() );
        Call( ErrDIRBeginTransaction( ppib, 38778, NO_GRBIT ) );
        fInTrx = fTrue;

        // Lock the LVROOT so no other tasks can try to delete it
        err = ErrDIRGetLock( pfucb, writeLock );
        if( JET_errWriteConflict == err )
        {
            err = JET_errSuccess;
            break;
        }
        Call( err );

        Call( ErrDIRGet( pfucb ) );

        // LVROOT should be intact still, it is deleted last
        Assert( sizeof(LVROOT) == pfucb->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucb->kdfCurr.data.Cb() );
        Assert( FIsLVRootKey( pfucb->kdfCurr.key ) );

        //  extract the LV root
        //
#pragma warning(suppress: 26015)
        UtilMemCpy( &lvroot, pfucb->kdfCurr.data.Pv(), min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );
        data.SetPv( &lvroot );
        data.SetCb( min( pfucb->kdfCurr.data.Cb(), sizeof(lvroot) ) );

        //  On each iteration, verify that no one is messing with the LV (by checking refcount
        //  and size haven't changed).
        if ( ( 0 != lvroot.ulReference && !FPartiallyDeletedLV( lvroot.ulReference ) )
            || ( 0 != citer && lvroot.ulSize != ulLVSize ) )
        {
            //  should never get into the case where we were able
            //  to delete some chunks (ie. citer > 0), but then some
            //  modification to the LV suddenly popped up somehow
            //
            Assert( 0 == citer );
            Error( ErrERRCheck( JET_errRecordNotDeleted ) );
        }

        if ( citer == 0 )
        {
            Assert( lvroot.ulReference == 0 || lvroot.ulReference == ulMax );
            Assert( data.Pv() == &lvroot );
            Assert( data.Cb() == sizeof(LVROOT) || data.Cb() == sizeof(LVROOT2) );
            
            // Set refcount to ulMax to indicate this LV is in the process of being
            // deleted and is NOT expected to be logically consistent during the
            // operation
            lvroot.ulReference = ulMax;
            Call( ErrDIRReplace( pfucb, data, fDIRNoVersion ) );
            Call( ErrDIRGet( pfucb ) ); // replace releases the latch, re-establish on the LV root...

            // Compute how many total chunks we have to delete. If this is cleanup (e.g. from
            // dbscan) this might be a bit bigger than the LV but it will still work since we'll
            // stop if we hit the end of the LV
            cRemainChunksToDelete = ( lvroot.ulSize + cbLVChunkMost - 1 ) / cbLVChunkMost;
            cRemainChunksToDelete++; // for LVROOT
            
            // Save the LV size to make sure it's not changing
            ulLVSize = lvroot.ulSize;
        }

        cChunksToDelete = min( cChunksToDeletePerTrx, cRemainChunksToDelete );
        cpgLVToDelete = cChunksToDelete / cChunksPerPage;
        FUCBSetPrereadForward( pfucb, cpgLVToDelete );

        // Delete cChunksToDeletePerTrx (starting after LVROOT) and clean up nodes synchronously
        Call( ErrRECDeleteLV_SynchronousCleanup( pfucb, cChunksToDelete ) );

        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
    
        Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
        fInTrx = fFalse;

        citer++;
        cRemainChunksToDelete -= cChunksToDelete;
    }
    while ( cRemainChunksToDelete > 0 );

HandleError:
    if ( fInTrx )
    {
        if ( Pcsr( pfucb )->FLatched() )
        {
            DIRReleaseLatch( pfucb );
        }
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    if ( pfucbNil != pfucb )
    {
        DIRClose( pfucb );
    }

    return err;
}

VOID DELETELVTASK::HandleError( const ERR err )
{
    //  this should only be called if the task failed for some reason
    //
    Assert( err < JET_errSuccess );

    PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
}


//  ****************************************************************
//  MERGEAVAILEXTTASK
//  ****************************************************************

MERGEAVAILEXTTASK::MERGEAVAILEXTTASK( const PGNO pgnoFDP, FCB * const pfcb, const IFMP ifmp, const BOOKMARK& bm ) :
    RECTASK( pgnoFDP, pfcb, ifmp, bm )
{
}

ERR MERGEAVAILEXTTASK::ErrExecuteDbTask( PIB * const ppib )
{
    ERR         err         = JET_errSuccess;
    FUCB        * pfucbAE   = pfucbNil;
    BOOL        fInTrx      = fFalse;
    BOOKMARK    bookmark;

    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortRecTask );

    GetBookmark( &bookmark );

    Assert( m_pgnoFDP == m_pfcb->PgnoFDP() );
    Assert( pgnoNull != m_pfcb->PgnoAE() );
    CallR( ErrSPIOpenAvailExt( ppib, m_pfcb, &pfucbAE ) );

    //  see comment in ErrBTDelete() regarding why transaction
    //  here is necessary
    //
    Call( ErrDIRBeginTransaction( ppib, 62757, NO_GRBIT ) );
    fInTrx = fTrue;

    Call( ErrBTIMultipageCleanup( pfucbAE, bookmark, NULL, NULL, NULL, fTrue ) );

    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );
    fInTrx = fFalse;

HandleError:
    Assert( pfucbNil != pfucbAE );
    Assert( !Pcsr( pfucbAE )->FLatched() );

    if ( fInTrx )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    Assert( pfucbNil != pfucbAE );
    BTClose( pfucbAE );

    return err;
}

VOID MERGEAVAILEXTTASK::HandleError( const ERR err )
{
    //  this task will be re-issued when another record on the page is deleted
}

//  ****************************************************************
//  BATCHRECTASK
//  ****************************************************************

BATCHRECTASK::BATCHRECTASK( const PGNO pgnoFDP, const IFMP ifmp ) :
    DBTASK( ifmp ),
    m_rgprectask( NULL ),
    m_cptaskMax( 0 ),
    m_iptaskCurr( 0 ),
    m_pgnoFDP( pgnoFDP )
{
    Assert( pgnoNull != m_pgnoFDP );
    Assert( ifmpNil != m_ifmp );
}

BATCHRECTASK::~BATCHRECTASK()
{
    // delete any tasks that weren't executed. the tasks must have
    // been allocated with new (this matches the TASK::Dispatch code).
    if( m_rgprectask )
    {
        for( INT iptask = 0; iptask < m_iptaskCurr; ++iptask )
        {
            RECTASK * const prectaskT = m_rgprectask[iptask];
            m_rgprectask[iptask] = NULL;
            delete prectaskT;
        }
    }
    delete [] m_rgprectask;
    m_rgprectask = NULL;
}

ERR BATCHRECTASK::ErrAddTask( RECTASK * const prectask )
{
    Assert( prectask );
    Assert( prectask->PgnoFDP() == m_pgnoFDP );
    Assert( prectask->Ifmp() == m_ifmp );

    // concurrent access is controlled by the fact this should only happen from the RCE Clean thread.
    
    Assert( PinstFromIfmp( prectask->Ifmp() )->m_pver->m_critRCEClean.FOwner() );
    
    ERR err = JET_errSuccess;

    if( m_iptaskCurr >= m_cptaskMax )
    {
        // need to grow the array
        const INT cptaskMaxT = m_cptaskMax + 16;
        RECTASK ** rgprectaskT;
        
        Alloc( rgprectaskT = new RECTASK*[cptaskMaxT] );
        memset( rgprectaskT, 0, cptaskMaxT * sizeof(TASK*) );
        
        UtilMemCpy( rgprectaskT, m_rgprectask, m_cptaskMax * sizeof(TASK*) );
        delete [] m_rgprectask;

        m_rgprectask = rgprectaskT;
        m_cptaskMax = cptaskMaxT;
    }

    AssertPREFIX( m_iptaskCurr < m_cptaskMax );
    m_rgprectask[m_iptaskCurr++] = prectask;
    AssertPREFIX( m_iptaskCurr <= m_cptaskMax );

HandleError:
    return err;
}

ERR BATCHRECTASK::ErrExecuteDbTask( PIB * const ppib )
{
    // if there are no tasks, we have no work to do
    if( !m_rgprectask )
    {
        return JET_errSuccess;
    }
    
    SortTasksByBookmark();

    // it is possible that preread could fail (e.g. there are no available buffer)
    // if that happens we don't want to keep trying preread, so we won't issue
    // any more preread attempts for the rest of the function
    LONG ctasksPreread = 0;
    PrereadTaskBookmarks( ppib, 0, &ctasksPreread );
    
    for( INT iptask = 0; iptask < m_iptaskCurr; ++iptask )
    {
        // If the instance is unavailable or log is down, there is no point in continuing to execute these
        // tasks. An instance unavailable state at this point may make the session enter a
        // transaction multiple times and ultimately, exceed the maximum number of levels.
        // This happens because the BeginTransaction() will normally succeed (due to being deferred)
        // and then any further operations will fail, including attempts to rollback.
        if ( PInstance()->FInstanceUnavailable() )
        {
            return PInstance()->m_errInstanceUnavailable;
        }

        if ( PInstance()->m_plog->FNoMoreLogWrite() )
        {
            return ErrERRCheck( JET_errLogWriteFail );
        }

        // dispatching the task also deletes the object,
        // so don't keep a pointer to it
        RECTASK * const prectaskT = m_rgprectask[iptask];
        m_rgprectask[iptask] = NULL;
        TASK::Dispatch( ppib, (ULONG_PTR)prectaskT );

        // see if we have used up all the bookmarks previously preread
        if( ctasksPreread > 0 )
        {
            --ctasksPreread;
            if( 0 == ctasksPreread )
            {
                PrereadTaskBookmarks( ppib, iptask+1, &ctasksPreread );
            }
        }
    }
    return JET_errSuccess;
}


//  this should only be called if the BATCHRECTASK failed for some reason
//  if executing one of the batched tasks fails, the HandleError method
//  for that specific RECTASK will be executed
VOID BATCHRECTASK::HandleError( const ERR err )
{
    Assert( err < JET_errSuccess );

    // we won't be executing the remaining tasks
    for( INT iptask = 0; iptask < m_iptaskCurr; ++iptask )
    {
        if( m_rgprectask[iptask] )
        {
            PinstFromIfmp( m_ifmp )->m_pver->IncrementCCleanupFailed();
        }
    }
}

VOID BATCHRECTASK::SortTasksByBookmark()
{
    Assert( m_rgprectask );
    sort( m_rgprectask, m_rgprectask + m_iptaskCurr, RECTASK::FBookmarkIsLessThan );
}

//  preread bookmarks of the tasks, starting at itaskStart. the number of tasks that had their bookmarks
//  preread is stored in *pctasksPreread
VOID BATCHRECTASK::PrereadTaskBookmarks( PIB * const ppib, const INT itaskStart, __out LONG * pctasksPreread )
{
    *pctasksPreread = 0;

    const INT cbookmarksPreread = min( m_iptaskCurr - itaskStart, 1024 );
    if( 0 == cbookmarksPreread )
    {
        Assert( 0 == *pctasksPreread );
        return;
    }

    // ignore errors. if preread fails we don't want to stop the execution of the task

    BOOKMARK * rgbm;
    if( NULL != ( rgbm = new BOOKMARK[cbookmarksPreread] ) )
    {
        FUCB * pfucb = pfucbNil;
        if( JET_errSuccess == ErrDIROpen( ppib, m_pgnoFDP, m_ifmp, &pfucb ) )
        {
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->iorReason.SetIort( iortRecTask );

            for( INT ibm = 0; ibm < cbookmarksPreread; ++ibm )
            {
                m_rgprectask[ibm+itaskStart]->GetBookmark( rgbm+ibm );
            }

            const ERR errT = ErrBTPrereadBookmarks(
                ppib,
                pfucb,
                rgbm,
                cbookmarksPreread,
                pctasksPreread,
                // This is only used by verstore cleanup task, do not trigger OLD2 from this preread.
                JET_bitPrereadForward | bitPrereadDoNotDoOLD2 );
            Assert( JET_errInvalidGrbit != errT );
            DIRClose(pfucb);
///         printf( "Preread %d/%d\n", *pctasksPreread, cbookmarksPreread );
        }
        delete[] rgbm;
    }
}

    
//  ****************************************************************
//  RECTASKBATCHER
//  ****************************************************************

RECTASKBATCHER::RECTASKBATCHER(
    INST * const pinst,
    const INT cbatchesMax,
    const INT ctasksPerBatchMax,
    const INT ctasksBatchedMax ) :
        m_pinst( pinst ),
        m_cbatchesMax( cbatchesMax ),
        m_ctasksPerBatchMax( ctasksPerBatchMax ),
        m_ctasksBatchedMax( ctasksBatchedMax ),
        m_rgpbatchtask( NULL ),
        m_ctasksBatched( 0 )
{
    // there should be some batching
    Assert( cbatchesMax > 0 );
    // batching 1 task is useless
    Assert( ctasksPerBatchMax >= 2 );
    // we should be able to batch at least two tasks per b-tree
    Assert( ctasksBatchedMax >= cbatchesMax * 2 );
}
    
// delete any tasks that weren't executed. the tasks must have
// been allocated with 'new'. This matches the TASK::Dispatch
// code.
RECTASKBATCHER::~RECTASKBATCHER()
{
    if( m_rgpbatchtask )
    {
        for( INT ipbatchtask = 0; ipbatchtask < m_cbatchesMax; ++ipbatchtask )
        {
            if( m_rgpbatchtask[ipbatchtask] )
            {
                BATCHRECTASK * const pbatchrectaskT = m_rgpbatchtask[ipbatchtask];
                m_ctasksBatched -= pbatchrectaskT->CTasks();
                m_rgpbatchtask[ipbatchtask] = NULL;
                
                delete pbatchrectaskT;
            }
        }
        Assert( 0 == m_ctasksBatched );
        delete[] m_rgpbatchtask;
        m_rgpbatchtask = NULL;
    }
}

// adds a task to the batch list
//
// this method does not delete the TASK if the TASK is successfully added to a batch list. 
// it is designed to be a replacement for ErrTMPost
//
ERR RECTASKBATCHER::ErrPost( RECTASK * prectask )
{
    Assert( prectask );
    Assert( m_ctasksBatched >= 0 );

    // concurrent access is controlled by the fact this should only happen from the RCE Clean thread.
    
    Assert( PinstFromIfmp( prectask->Ifmp() )->m_pver->m_critRCEClean.FOwner() );

    ERR err = JET_errSuccess;
    bool fTaskAdded = false;
    if( !m_rgpbatchtask )
    {
        Alloc( m_rgpbatchtask = new BATCHRECTASK*[m_cbatchesMax] );
        memset( m_rgpbatchtask, 0, m_cbatchesMax*sizeof(BATCHRECTASK*) );
    }

    // look in the array of batch tasks to see if we are already batching tasks
    // for this ifmp/pgno. If we are already batching then add the task to the
    // batch object. If we aren't and there is a free slot then create a new
    // batch task, otherwise dispatch the task directly

    bool fFoundBatchTask = false;
    
    INT ipbatchtaskFree = m_cbatchesMax;
    for( INT ipbatchtask = 0; ipbatchtask < m_cbatchesMax; ++ipbatchtask )
    {
        if( m_rgpbatchtask[ipbatchtask] )
        {
            BATCHRECTASK * const pbatchrectask = m_rgpbatchtask[ipbatchtask];
            Assert( pbatchrectask->CTasks() < m_ctasksPerBatchMax );
            if( pbatchrectask->PgnoFDP() == prectask->PgnoFDP()
                && pbatchrectask->Ifmp() == prectask->Ifmp() )
            {
                Assert( !fFoundBatchTask );
                Call( pbatchrectask->ErrAddTask( prectask ) );
                fTaskAdded = true;
                fFoundBatchTask = true;
                m_ctasksBatched++;
                Assert( m_ctasksBatched > 0 );
                if( pbatchrectask->CTasks() >= m_ctasksPerBatchMax )
                {
                    Assert( pbatchrectask->CTasks() == m_ctasksPerBatchMax );
                    Assert( pbatchrectask == m_rgpbatchtask[ipbatchtask] );
                    Call( ErrPostOneBatch( ipbatchtask ) );
                }
                break;
            }
        }
        else
        {
            ipbatchtaskFree = min( ipbatchtaskFree, ipbatchtask );
        }
    }

    // if we didn't post the task then create a new BATCHRECTASK object
    // if we have hit the limit of BATCHRECTASKs then issue all the tasks
    
    if( !fFoundBatchTask )
    {
        Assert( !fTaskAdded );
        if( ipbatchtaskFree < m_cbatchesMax )
        {
            Alloc( m_rgpbatchtask[ipbatchtaskFree] = new BATCHRECTASK( prectask->PgnoFDP(), prectask->Ifmp() ) );
            Call( m_rgpbatchtask[ipbatchtaskFree]->ErrAddTask( prectask ) );
            fTaskAdded = true;
            Assert( 1 == m_rgpbatchtask[ipbatchtaskFree]->CTasks() );
            m_ctasksBatched++;
            Assert( m_ctasksBatched > 0 );
        }
        else
        {
            Call( ErrPostAllPending() );
            // dispatch the RECTASK as well, as it wasn't added to a BATCHRECTASK
            Call( m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, prectask ) );
            fTaskAdded = true;
            Assert( 0 == m_ctasksBatched );
        }
    }

    // if we have hit the task limit then dispatch them all
    
    if( m_ctasksBatched >= m_ctasksBatchedMax )
    {
        Assert( m_ctasksBatched == m_ctasksBatchedMax );
        Call( ErrPostAllPending() );
        Assert( 0 == m_ctasksBatched );
    }

HandleError:
    
    if( ! fTaskAdded )
    {
        //  The task was not added/posted sucessfully.
        //  WARNING: Please do not delete the prectask in the caller as the failure is already handled here.
        Assert( err < JET_errSuccess );
        PinstFromIfmp( prectask->Ifmp() )->m_pver->IncrementCCleanupFailed();
        delete prectask;
        prectask = NULL;
    }
    return err;
}

ERR RECTASKBATCHER::ErrPostAllPending()
{
    // concurrent access is controlled by the fact this should only happen from the RCE Clean thread.
    
    Assert( m_pinst->m_pver->m_critRCEClean.FOwner() );
    
    ERR err = JET_errSuccess;
    
    if( m_rgpbatchtask )
    {
        // loop through all the BATCHRECTASKs, even if we encounter an error
        for( INT ipbatchtask = 0; ipbatchtask < m_cbatchesMax; ++ipbatchtask )
        {
            if( m_rgpbatchtask[ipbatchtask] )
            {
                const ERR errT = ErrPostOneBatch( ipbatchtask );
                if( errT < JET_errSuccess ) // don't overwrite previous error
                {
                    err = errT;
                }
            }
        }
    }

    Assert( 0 == m_ctasksBatched );
    return err;
}

ERR RECTASKBATCHER::ErrPostOneBatch( const INT ipbatchtask )
{
    Assert( ipbatchtask >= 0 );
    Assert( ipbatchtask < m_cbatchesMax );
    Assert( m_rgpbatchtask );
    Assert( NULL != m_rgpbatchtask[ipbatchtask] );

    // concurrent access is controlled by the fact this should only happen from the RCE Clean thread.
    
    Assert( PinstFromIfmp( m_rgpbatchtask[ipbatchtask]->Ifmp() )->m_pver->m_critRCEClean.FOwner() );
    
    BATCHRECTASK * const pbatchrectaskT = m_rgpbatchtask[ipbatchtask];
    Assert( pbatchrectaskT );
    
    m_ctasksBatched -= pbatchrectaskT->CTasks();
    m_rgpbatchtask[ipbatchtask] = NULL;

    // if the task is posted successfully, then execution will delete the task,
    const ERR err = m_pinst->Taskmgr().ErrTMPost( TASK::DispatchGP, pbatchrectaskT );
    if( err < JET_errSuccess )
    {
        for( INT i = 0; i < pbatchrectaskT->CTasks(); ++i )
        {
            PinstFromIfmp( pbatchrectaskT->Ifmp() )->m_pver->IncrementCCleanupFailed();
        }
        delete pbatchrectaskT;
    }

    Assert( NULL == m_rgpbatchtask[ipbatchtask] );
    return err;
}

