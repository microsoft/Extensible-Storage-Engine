// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/*******************************************************************

Long Values are stored in a per-table B-Tree. Each long value has
a first node that contains the refcount and size of the long value
-- an LVROOT struct. The key of this node is the LID. Further nodes
have a key of LID:OFFSET (a LVKEY struct). The offset starts at 1
These values are big endian on disk.

OPTIMIZATION:  consider a critical section in each FCB for LV access
OPTIMIZATION:  don't store the longIdMax in the TDB. Seek to the end of
               the table, write latch the page and get the last id.

*******************************************************************/


#include "std.hxx"

//  ****************************************************************
//  GLOBALS
//  ****************************************************************

//  OPTIMIZATION:  replace this with a pool of critical sections. hash on pgnoFDP
// LOCAL CCriticalSection critLV( CLockBasicInfo( CSyncBasicInfo( szLVCreate ), rankLVCreate, 0 ) );
// LOCAL PIB * ppibLV;


#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotalWithClass<> cLVSeeks;
LONG LLVSeeksCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVSeeks.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVRetrieves;
LONG LLVRetrievesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVRetrieves.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVCreates;
LONG LLVCreatesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVCreates.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotalWithClass< ULONG64, INST, 1, fFalse > cLVMaximumLID;
LONG LLVMaximumLIDCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVMaximumLID.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVUpdates;
LONG LLVUpdatesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVUpdates.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVDeletes;
LONG LLVDeletesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVDeletes.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVCopies;
LONG LLVCopiesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVCopies.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVChunkSeeks;
LONG LLVChunkSeeksCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVChunkSeeks.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVChunkRetrieves;
LONG LLVChunkRetrievesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVChunkRetrieves.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVChunkAppends;
LONG LLVChunkAppendsCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVChunkAppends.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVChunkReplaces;
LONG LLVChunkReplacesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVChunkReplaces.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVChunkDeletes;
LONG LLVChunkDeletesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVChunkDeletes.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cLVChunkCopies;
LONG LLVChunkCopiesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cLVChunkCopies.PassTo( iInstance, pvBuf );
    return 0;
}

//  UNDONE: these REC counters are probably more appropriate in recupd.cxx
//
PERFInstanceDelayedTotalWithClass<> cRECUpdateIntrinsicLV;
LONG LRECUpdateIntrinsicLVCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECUpdateIntrinsicLV.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECAddSeparateLV;
LONG LRECAddSeparateLVCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECAddSeparateLV.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECAddForcedSeparateLV;
LONG LRECAddForcedSeparateLVCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECAddForcedSeparateLV.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECForceSeparateAllLV;
LONG LRECForceSeparateAllLVCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECForceSeparateAllLV.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECRefAllSeparateLV;
LONG LRECRefAllSeparateLVCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECRefAllSeparateLV.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECDerefAllSeparateLV;
LONG LRECDerefAllSeparateLVCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECDerefAllSeparateLV.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT


//  ****************************************************************
//  FUNCTION PROTOTYPES
//  ****************************************************************

ERR ErrRECAOSeparateLV(
    FUCB                *pfucb,
    LvId                *plid,
    const DATA          *plineField,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    const BOOL          fContiguousLv,
    const ULONG         ibLongValue,
    const ULONG         ulColMax,
    const LVOP          lvop );

ERR ErrRECICreateLvRootAndChunks(
    _In_ FUCB                   * const pfucb,
    _In_ const DATA             * const pdataField,
    _In_ const CompressFlags    compressFlags,
    _In_ const BOOL             fEncrypted,
    _In_ CPG *                  pcpgLvSpaceRequired,
    _Out_ LvId                  * const plid,
    __in_opt FUCB               **ppfucb,
    __in_opt LVROOT2            *plvrootInit = NULL );

//  ****************************************************************
//  INTERNAL FUNCTIONS
//  ****************************************************************


//  retrieve versioned LV refcount
INLINE ULONG UlLVIVersionedRefcount( FUCB *pfucbLV )
{
    LVROOT              *plvrootVersioned;
    LONG                lCompensating;
    BOOKMARK            bookmark;

    Assert( Pcsr( pfucbLV )->FLatched() );
    Assert( FIsLVRootKey( pfucbLV->kdfCurr.key ) );
    Assert( sizeof( LVROOT ) == pfucbLV->kdfCurr.data.Cb() || sizeof( LVROOT2 ) == pfucbLV->kdfCurr.data.Cb() );

    bookmark.key = pfucbLV->kdfCurr.key;
    bookmark.data.Nullify();

    plvrootVersioned = reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() );

    lCompensating = DeltaVERGetDelta<LONG>( pfucbLV, bookmark, OffsetOf( LVROOT, ulReference )  );
    Assert( lCompensating == 0 || !FPartiallyDeletedLV( plvrootVersioned->ulReference ) );

    return plvrootVersioned->ulReference + lCompensating;
}


//  ****************************************************************
//  DEBUG ROUTINES
//  ****************************************************************


//  ================================================================
INLINE VOID AssertLVRootNode( FUCB *pfucbLV, const LvId lid )
//  ================================================================
//
//  This checks that the FUCB is currently referencing a valid LVROOT node
//
//-
{
#ifdef DEBUG
    LvId    lidT;

    Assert( Pcsr( pfucbLV )->FLatched() );
    Assert( FIsLVRootKey( pfucbLV->kdfCurr.key ) );
    Assert( sizeof( LVROOT ) == pfucbLV->kdfCurr.data.Cb() || sizeof( LVROOT2 ) == pfucbLV->kdfCurr.data.Cb() );
    LidFromKey( &lidT, pfucbLV->kdfCurr.key );
    Assert( lid == lidT );

    //  versioned refcount must be non-zero, otherwise we couldn't see it
    //  if FPIBDirty() is set then concurrent create index is looking at this LV
    //  it is possible that the refcount could be zero
    //  in repair we may be setting the refcount of an LV that (incorrectly) has a 0 refcount.
    //  It is also possible for refcount to be 0 in the case of JET_prepInsertCopyReplace/DeleteOriginal.
    //
    //  Assert( UlLVIVersionedRefcount( pfucbLV ) > 0 || FPIBDirty( pfucbLV->ppib )  || g_fRepair );
#endif
}


//  verify lid/offset of an LVKEY node
INLINE VOID AssertLVDataNode(
    FUCB        *pfucbLV,
    const LvId  lid,
    const ULONG ulOffset )
{
#ifdef DEBUG
    LvId        lidT;
    ULONG       ib;

    Assert( Pcsr( pfucbLV )->FLatched() );
    Assert( FIsLVChunkKey( pfucbLV->kdfCurr.key ) );
    LidOffsetFromKey( &lidT, &ib, pfucbLV->kdfCurr.key );
    Assert( lid == lidT );
    Assert( ib % pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() == 0 );
    Assert( ulOffset == ib );
#endif
}


#define LVReportAndTrapCorruptedLV( pfucbLV, lid, wszGuid ) LVReportAndTrapCorruptedLV_( pfucbLV, lid, __FILE__, __LINE__, wszGuid )

//  ================================================================
VOID LVReportAndTrapCorruptedLV_( const FUCB * const pfucbLV, const LvId lid, const CHAR * const szFile, const LONG lLine, const WCHAR* const wszGuid )
//  ================================================================
{
    LVReportAndTrapCorruptedLV_( PinstFromPfucb( pfucbLV ), g_rgfmp[pfucbLV->u.pfcb->Ifmp()].WszDatabaseName(), pfucbLV->u.pfcb->PfcbTable()->Ptdb()->SzTableName(), Pcsr( pfucbLV )->Pgno(), lid, szFile, lLine, wszGuid );
}

#define LVReportAndTrapCorruptedLV2( pinst, WszDatabaseName, szTable, pgno, lid, wszGuid ) LVReportAndTrapCorruptedLV_( pinst, WszDatabaseName, szTable, pgno, lid, __FILE__, __LINE__, wszGuid )

//  ================================================================
VOID LVReportAndTrapCorruptedLV_( const INST * const pinst, PCWSTR wszDatabaseName, PCSTR szTable, const PGNO pgno, const LvId lid, const CHAR * const szFile, const LONG lLine, const WCHAR* const wszGuid )
//  ================================================================
{
    const WCHAR *rgsz[4];
    INT         irgsz = 0;

    WCHAR wszTable[JET_cbNameMost+1];
    WCHAR wszpgno[16];
    WCHAR wszlid[24];

    OSStrCbFormatW( wszTable, sizeof(wszTable), L"%hs", szTable );
    OSStrCbFormatW( wszpgno, sizeof(wszpgno), L"%d", pgno );
    OSStrCbFormatW( wszlid, sizeof(wszlid), L"0x%I64x", (_LID64)lid );

    rgsz[irgsz++] = wszTable;
    rgsz[irgsz++] = wszDatabaseName;
    rgsz[irgsz++] = wszpgno;
    rgsz[irgsz++] = wszlid;

    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            CORRUPT_LONG_VALUE_ID,
            irgsz,
            rgsz,
            0,
            NULL,
            pinst );

    OSUHAEmitFailureTag( pinst, HaDbFailureTagCorruption, wszGuid );

    FireWallAt( "LogicalLvCorruption", szFile, lLine );
}


ERR ErrLVCheckDataNodeOfLid( FUCB *pfucbLV, const LvId lid )
{
    ERR     err;
    LvId    lidT;

    Assert( Pcsr( pfucbLV )->FLatched() );

    if( g_fRepair )
    {
        //  don't bother returning an error during repair
        //  if we are running repair the database is probably corrupted
        return JET_errSuccess;
    }

    if ( FIsLVChunkKey( pfucbLV->kdfCurr.key ) )
    {
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if ( lid == lidT )
        {
#ifdef DEBUG
            ULONG   ib;
            OffsetFromKey( &ib, pfucbLV->kdfCurr.key );
            Assert( ib % pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() == 0 );
#endif

            //  this is a valid data node belonging to this lid
            err = JET_errSuccess;
        }
        else
        {
            //  should be impossible (if lids don't match, we
            //  should have hit an LV root node, not another
            //  LV data node)
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"552cdeaa-c62e-47f6-81b4-fe87f2fb9274" );
            err = ErrERRCheck( JET_errLVCorrupted );
        }
    }

    // if it is an LVROOT key
    else if ( FIsLVRootKey( pfucbLV->kdfCurr.key ) &&
                ( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucbLV->kdfCurr.data.Cb() ) )
    {
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if ( lidT <= lid  )
        {
            //  current LV has should have higher lid than previous LV
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"952e2a73-05bf-4299-8e9b-cc5674ad72eb" );
            err = ErrERRCheck( JET_errLVCorrupted );
        }
        else
        {
            //  moved on to a new lid, issue warning
            err = ErrERRCheck( wrnLVNoMoreData );
        }
    }
    else
    {
        //  bogus LV node
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"b1312def-92df-4461-a853-b5c344b642f7" );
        err = ErrERRCheck( JET_errLVCorrupted );
    }

    return err;
}


#ifdef DEBUG
//  ================================================================
BOOL FAssertLVFUCB( const FUCB * const pfucb )
//  ================================================================
//
//  Asserts that the FUCB references a Long Value directory
//  long value directories do not have TDBs.
//
//-
{
    FCB *pfcb = pfucb->u.pfcb;
    if ( pfcb->FTypeLV() )
    {
        Assert( pfcb->Ptdb() == ptdbNil );
    }
    return pfcb->FTypeLV();
}
#endif


//  ****************************************************************
//  ROUTINES
//  ****************************************************************


//  ================================================================
BOOL FPIBSessionLV( PIB *ppib )
//  ================================================================
{
    Assert( ppibNil != ppib );
    return ( PinstFromPpib(ppib)->m_ppibLV == ppib );
}


//  ================================================================
LOCAL ERR ErrFILECreateLVRoot( PIB *ppib, FUCB *pfucb, PGNO *ppgnoLV )
//  ================================================================
//
//  Creates the LV tree for the given table.
//
//-
{
    ERR     err = JET_errSuccess;
    FUCB    *pfucbTable = pfucbNil;
    PGNO    pgnoLVFDP = pgnoNull;
    OBJID   objidLV;
    BOOL    fInTransaction = fFalse;

    Assert( !FAssertLVFUCB( pfucb ) );

    const BOOL  fTemp = FFMPIsTempDB( pfucb->ifmp );
    Assert( ( fTemp && pfucb->u.pfcb->FTypeTemporaryTable() )
        || ( !fTemp && pfucb->u.pfcb->FTypeTable() ) );

#ifdef DEBUG
    const BOOL  fExclusive = !FPIBSessionLV( ppib );
    if ( fExclusive )
    {
        Assert( pfucb->ppib == ppib );
        Assert( pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) );
    }
    else
    {
        Assert( !FPIBSessionLV( pfucb->ppib ) );
        Assert( !pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) );
        Assert( PinstFromPpib(ppib)->m_critLV.FOwner() );
    }
#endif

    //  open the parent directory (so we are on the pgnoFDP ) and create the LV tree
    CallR( ErrDIROpen( ppib, pfucb->u.pfcb, &pfucbTable ) );
    Call( ErrDIRBeginTransaction( ppib, 49445, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( PverFromIfmp( pfucb->ifmp )->ErrVERFlag( pfucbTable, operCreateLV, NULL, 0 ) );

    //  CONSIDER:  get at least enough pages to hold the LV we are about to insert
    //  we must open the directory with a different session.
    //  if this fails, rollback will free the extent, or at least, it will attempt
    //  to free the extent.
    Call( ErrDIRCreateDirectory(    pfucbTable,
                                    cpgLVTree,
                                    &pgnoLVFDP,
                                    &objidLV,
                                    CPAGE::fPageLongValue,
                                    fTemp ? fSPUnversionedExtent : 0 ) );   // For temp. tables, create unversioned extents

    Assert( pgnoLVFDP > pgnoSystemRoot );
    Assert( pgnoLVFDP <= pgnoSysMax );
    Assert( pgnoLVFDP != pfucbTable->u.pfcb->PgnoFDP() );

    if ( !fTemp )
    {
        Assert( objidLV > objidSystemRoot );
        Call( ErrCATAddTableLV(
                    ppib,
                    pfucb->ifmp,
                    pfucb->u.pfcb->ObjidFDP(),
                    pgnoLVFDP,
                    objidLV ) );
    }

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

    *ppgnoLV = pgnoLVFDP;

HandleError:
    if ( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    Assert( pfucbTable != pfucbNil );
    DIRClose( pfucbTable );

#ifdef DEBUG
    if ( !fExclusive )
        Assert( PinstFromPpib(ppib)->m_critLV.FOwner() );
#endif

    //  should have no other directory with the same parentId and name, or the same pgnoFDP
    Assert( JET_errKeyDuplicate != err );
    return err;
}


//  ================================================================
INLINE ERR ErrFILEIInitLVRoot( FUCB *pfucb, const PGNO pgnoLV, FUCB **ppfucbLV )
//  ================================================================
{
    ERR             err;
    FCB * const     pfcbTable   = pfucb->u.pfcb;
    FCB *           pfcbLV;

    // Link LV FCB into table.
    CallR( ErrDIROpen( pfucb->ppib, pgnoLV, pfucb->ifmp, ppfucbLV, fTrue ) );
    Assert( *ppfucbLV != pfucbNil );
    Assert( !FFUCBVersioned( *ppfucbLV ) ); // Verify won't be deferred closed.
    pfcbLV = (*ppfucbLV)->u.pfcb;
    Assert( !pfcbLV->FInitialized() || pfcbLV->FInitedForRecovery() );

    Assert( pfcbLV->Ifmp() == pfucb->ifmp );
    Assert( pfcbLV->PgnoFDP() == pgnoLV );
    Assert( pfcbLV->Ptdb() == ptdbNil );
    Assert( pfcbLV->CbDensityFree() == 0 );

    // Recovery creates all FCBs as table FCB, now that we know better, we need to remove from list of table FCBs
    // before we mark FCB as being a LV FCB
    if ( pfcbLV->FInitedForRecovery() )
    {
        pfcbLV->RemoveList();
    }

    Assert( pfcbLV->FTypeNull() || pfcbLV->FInitedForRecovery() );
    pfcbLV->SetTypeLV();

    Assert( pfcbLV->PfcbTable() == pfcbNil );
    pfcbLV->SetPfcbTable( pfcbTable );

    //  transition the deferred space hints from the table def to the LV

    if ( pfcbTable->Ptdb()->PjsphDeferredLV() )
    {
        pfcbLV->SetSpaceHints( pfcbTable->Ptdb()->PjsphDeferredLV() );
    }
    else if ( pfcbTable->Ptdb()->PfcbTemplateTable() &&
            pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb()->PjsphDeferredLV() )
    {
        pfcbLV->SetSpaceHints( pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb()->PjsphDeferredLV() );
    }

    //  finish the initialization of this LV FCB

    pfcbLV->Lock();
    pfcbLV->CreateComplete();
    pfcbLV->ResetInitedForRecovery();
    pfcbLV->Unlock();

    //  WARNING: publishing the FCB in the TDB *must*
    //  be the last thing or else other sessions might
    //  see an FCB that's not fully initialised
    //
    Assert( pfcbNil == pfcbTable->Ptdb()->PfcbLV() );
    pfcbTable->Ptdb()->SetPfcbLV( pfcbLV );

    return err;
}


//  ================================================================
ERR ErrFILEOpenLVRoot( FUCB *pfucb, FUCB **ppfucbLV, BOOL fCreate )
//  ================================================================
{
    ERR         err;
    PIB         *ppib;
    PGNO        pgnoLV = pgnoNull;

    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->ppib != PinstFromPfucb(pfucb)->m_ppibLV );

    //  HACK: repair checks only one table per thread, but we can't open derived tables
    //  if the template table is opened exculsively

    const BOOL  fExclusive = pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) || g_fRepair;
    const BOOL  fTemp = FFMPIsTempDB( pfucb->ifmp );

    Assert( PinstFromPfucb(pfucb)->m_critLV.FNotOwner() );

    if ( fExclusive )
    {
        Assert( ( fTemp && pfucb->u.pfcb->FTypeTemporaryTable() )
            || ( !fTemp && pfucb->u.pfcb->FTypeTable() ) );

        ppib = pfucb->ppib;

        // ErrInfoGetTableAvailSpace() is calling this function with a table which may be
        // exclusively opened and which may have already opened the LV tree (loading its FCB).
        // We will give this function the same semantics for exclusively opened tables as
        // for ordinary tables -- open a cursor on the existing LV FCB if it exists.
        FCB * const pfcbLV = pfucb->u.pfcb->Ptdb()->PfcbLV();
        if ( pfcbNil != pfcbLV )
        {
            // best we can assert that space hints are set somehow
            Expected( pfcbLV->UlDensity() > 20 );

            Call( ErrDIROpen( pfucb->ppib, pfcbLV, ppfucbLV ) );
            goto HandleError;
        }
    }
    else
    {
        Assert( !fTemp );
        Assert( pfucb->u.pfcb->FTypeTable() );

        PinstFromPfucb(pfucb)->m_critLV.Enter();
        ppib = PinstFromPfucb(pfucb)->m_ppibLV;

        FCB *pfcbLV = pfucb->u.pfcb->Ptdb()->PfcbLV();

        //  someone may have come in and created the LV tree already
        if ( pfcbNil != pfcbLV )
        {
            // best we can assert that space hints are set somehow
            Expected( pfcbLV->UlDensity() > 20 );

            PinstFromPfucb(pfucb)->m_critLV.Leave();

            // PfcbLV won't go away, since only way it would be freed is if table
            // FCB is freed, which can't happen because we've got a cursor on the table.
            err = ErrDIROpen( pfucb->ppib, pfcbLV, ppfucbLV );
            return err;
        }

        err = ErrDBOpenDatabaseByIfmp( PinstFromPfucb(pfucb)->m_ppibLV, pfucb->ifmp );
        if ( err < 0 )
        {
            PinstFromPfucb(pfucb)->m_critLV.Leave();
            return err;
        }
    }

    Assert( pgnoNull == pgnoLV );   // initial value
    if ( !fTemp )
    {
        Call( ErrCATAccessTableLV( ppib, pfucb->ifmp, pfucb->u.pfcb->ObjidFDP(), &pgnoLV ) );
    }
    else
    {
        //  if opening LV tree for a temp. table, it MUST be created
        Assert( fCreate );
    }

    if ( pgnoNull == pgnoLV && fCreate )
    {
        // LV root not yet created.
        Call( ErrFILECreateLVRoot( ppib, pfucb, &pgnoLV ) );
    }

    // For temp. tables, if initialisation fails here, the space
    // for the LV will be lost, because it's not persisted in
    // the catalog.
    if( pgnoNull != pgnoLV )
    {
        err = ErrFILEIInitLVRoot( pfucb, pgnoLV, ppfucbLV );
    }
    else
    {
        //  if only opening LV tree (ie. no creation), and no LV tree
        //  exists, then return warning and no cursor.
        Assert( !fTemp );
        Assert( !fCreate );
        *ppfucbLV = pfucbNil;
        err = ErrERRCheck( wrnLVNoLongValues );
    }

HandleError:
    if ( !fExclusive )
    {
        Assert( ppib == PinstFromPfucb(pfucb)->m_ppibLV );
        CallS( ErrDBCloseAllDBs( PinstFromPfucb(pfucb)->m_ppibLV ) );

        PinstFromPfucb(pfucb)->m_critLV.Leave();
    }

#ifdef DEBUG
    if ( *ppfucbLV == pfucbNil )
    {
        Assert( ( err < JET_errSuccess ) ||
                ( !fCreate && ( wrnLVNoLongValues == err ) && ( pfucb->u.pfcb->Ptdb()->PfcbLV() == pfcbNil ) ) );
    }
    else
    {
        Assert( err >= JET_errSuccess );
        Assert( pfucb->u.pfcb->Ptdb()->PfcbLV() != pfcbNil );
    }
#endif

    return err;
}


//  ================================================================
LOCAL ERR ErrDIROpenLongRoot(
    FUCB *      pfucb,
    FUCB **     ppfucbLV,
    const BOOL  fAllowCreate )
//  ================================================================
//
//  Extract the pgnoFDP of the long value tree table from the catalog and go to it
//  if the Long Value tree does not exist we will create it
//
//-
{
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( ppfucbLV );


    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->u.pfcb->FTypeTable()
        || pfucb->u.pfcb->FTypeTemporaryTable() );

    ERR         err     = JET_errSuccess;
    FCB * const pfcbLV  = pfucb->u.pfcb->Ptdb()->PfcbLV();

    if ( pfcbNil == pfcbLV )
    {
        CallR( ErrFILEOpenLVRoot( pfucb, ppfucbLV, fAllowCreate ) );
        if ( wrnLVNoLongValues == err )
        {
            Assert( !fAllowCreate );
            Assert( pfucbNil == *ppfucbLV );
            return err;
        }
        else
        {
            CallS( err );
            Assert( pfucbNil != *ppfucbLV );
        }
    }
    else
    {
        CallR( ErrDIROpen( pfucb->ppib, pfcbLV, ppfucbLV ) );
        Assert( (*ppfucbLV)->u.pfcb == pfcbLV );
    }

    ASSERT_VALID( *ppfucbLV );

#ifdef DEBUG
    FCB * const pfcbLVDBG   = (*ppfucbLV)->u.pfcb;
    Assert( pfcbLVDBG != pfcbNil );
    Assert( pfcbLVDBG->Ifmp() == pfucb->ifmp );
    Assert( pfcbLVDBG->PgnoFDP() != pfucb->u.pfcb->PgnoFDP() );
    Assert( pfcbLVDBG->Ptdb() == ptdbNil );
    Assert( pfcbLVDBG->FInitialized() );
    Assert( pfcbLVDBG->FTypeLV() );
    Assert( pfcbLVDBG->PfcbTable() == pfucb->u.pfcb );
    Assert( pfucb->u.pfcb->Ptdb()->PfcbLV() == pfcbLVDBG );

    // UNDONE: Since FCB critical sections are shared, is it not possible for this
    // assert to fire on a hash collision??
    Assert( pfcbLVDBG->IsUnlocked() );  //lint !e539
#endif

    FUCBSetLongValue( *ppfucbLV );

    //  if our table is open in sequential mode open the long value
    //  table in sequential mode as well.
    //  note that compact opens all its tables in sequential mode
    if ( FFUCBSequential( pfucb ) )
    {
        FUCBSetSequential( *ppfucbLV );
    }
    else
    {
        FUCBResetSequential( *ppfucbLV );
    }

    return err;
}

ERR ErrDIROpenLongRoot( FUCB * pfucb )
{
    ERR err = JET_errSuccess;

    if ( pfucbNil == pfucb->pfucbLV )
    {
        //  use cached LV cursor
        CallR( ErrDIROpenLongRoot( pfucb, &pfucb->pfucbLV, fFalse ) );
        if ( pfucb->pfucbLV != NULL )
        {
            pfucb->pfucbLV->pfucbTable = pfucb;
        }
        else
        {
            // During recovery, table can get deleted while a read-only transaction is running. Since we
            // use a separate session to open the long-value tree, it may not be visible to that session.
            // Space is not freed until the user session is closed, so there is no risk of the same pgno
            // getting assigned to a different LV tree.
            Assert( err == wrnLVNoLongValues );
            Assert( PinstFromPfucb( pfucb )->FRecovering() );
            return ErrERRCheck( JET_errObjectNotFound );
        }
    }
    else
    {
        Assert( FFUCBMayCacheLVCursor( pfucb ) );
        Assert( FFUCBLongValue( pfucb->pfucbLV ) );
        Assert( pfucb->pfucbLV->pfucbTable == pfucb );
        Assert( pfcbNil != pfucb->pfucbLV->u.pfcb );
        Assert( pfucb->pfucbLV->u.pfcb->FTypeLV() );
        Assert( pfucb->pfucbLV->u.pfcb->FInitialized() );
        Assert( pfucb->pfucbLV->u.pfcb->PfcbTable() == pfucb->u.pfcb );

        //  if our table is open in sequential mode open the long value
        //  table in sequential mode as well.
        //  note that compact opens all its tables in sequential mode
        if ( FFUCBSequential( pfucb ) )
        {
            FUCBSetSequential( pfucb->pfucbLV );
        }
        else
        {
            FUCBResetSequential( pfucb->pfucbLV );
        }

        //  OPTIMISATION: Should really call DIRUp() here to reset
        //  currency, but all callers of this function immediately
        //  do a ErrDIRDownLV(), which performs the DIRUp() for us
///     DIRUp();
    }

    return err;
}

INLINE VOID DIRCloseLongRoot( FUCB * pfucb )
{
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->u.pfcb->FTypeTable()
        || pfucb->u.pfcb->FTypeTemporaryTable() );
    Assert( pfucbNil != pfucb->pfucbLV );
    Assert( pfucb->pfucbLV->pfucbTable == pfucb );
    Assert( pfcbNil != pfucb->pfucbLV->u.pfcb );
    Assert( pfucb->pfucbLV->u.pfcb->FTypeLV() );

    if ( FFUCBMayCacheLVCursor( pfucb ) )
    {
        DIRUp( pfucb->pfucbLV );
        FUCBResetPreread( pfucb->pfucbLV );
        FUCBResetSequential( pfucb->pfucbLV );
        FUCBResetLimstat( pfucb->pfucbLV );
    }
    else
    {
        DIRClose( pfucb->pfucbLV );
        pfucb->pfucbLV = pfucbNil;
    }
}


//  ================================================================
ERR ErrDIRDownLVDataPreread(
    FUCB            *pfucbLV,
    const LvId      lid,
    const ULONG     ulOffset,
    const DIRFLAG   dirflag,
    const ULONG     cbPreread )
//  ================================================================
//
//  This takes an FUCB that is opened on the Long Value tree table and seeks
//  to the appropriate offset in the long value.
//
//-
{
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( lid > lidMin );

    //  this key is used if we do an index-range preread
    LVKEY_BUFFER lvkeyPrereadLast;

    //  normalize the offset to a multiple of cbLVChunkMost. we should seek to the exact node
    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG ulOffsetChunk = ( ulOffset / cbLVChunkMost ) * cbLVChunkMost;

    DIRUp( pfucbLV );

    //  make a key that consists of the long id and the offset
    LVKEY_BUFFER    lvkey;
    BOOKMARK        bm;
    LVKeyFromLidOffset( &lvkey, &bm.key, lid, ulOffsetChunk );
    bm.data.Nullify();

    DIB         dib;
    dib.dirflag = dirflag | fDIRExact;
    dib.pos     = posDown;
    dib.pbm     = &bm;

    // only preread if we want more than one chunk
    if ( cbPreread > (ULONG)cbLVChunkMost )
    {
        // we do the math this way to avoid integer overflow if cbPreread is 0xFFFFFFFF
        const ULONG cChunksPartial  = ( cbPreread % cbLVChunkMost ) ? 1 : 0;
        const ULONG cChunksPreread  = ( cbPreread  / cbLVChunkMost ) + cChunksPartial;
        const ULONG ulOffsetPrereadLast = ulOffsetChunk + ( ( cChunksPreread - 1 ) * cbLVChunkMost );
        Assert( ulOffsetPrereadLast > ulOffsetChunk );

        //  make a key that consists of the long id and the offset
        KEY keyPrereadLast;
        LVKeyFromLidOffset( &lvkeyPrereadLast, &keyPrereadLast, lid, ulOffsetPrereadLast );

        //  set the FUCB to have an index range
        Assert( !FFUCBLimstat( pfucbLV ) );
        Assert( pfucbLV->dataSearchKey.FNull() );
        
        FUCBSetPrereadForward( pfucbLV, cpgPrereadSequential );
        FUCBSetLimstat( pfucbLV );
        FUCBSetUpper( pfucbLV );
        FUCBSetInclusive( pfucbLV );

        Assert( keyPrereadLast.prefix.Cb() == 0 );  // prefix is always null for keys constructed by LVKeyFromLidOffset()
        pfucbLV->dataSearchKey.SetPv( keyPrereadLast.suffix.Pv() );
        pfucbLV->dataSearchKey.SetCb( keyPrereadLast.suffix.Cb() );
    }

    PERFOpt( PERFIncCounterTable( cLVChunkSeeks, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    ERR err = ErrDIRDown( pfucbLV, &dib );

    if ( JET_errSuccess == err )
    {
        AssertLVDataNode( pfucbLV, lid, ulOffsetChunk );
    }
    else if ( wrnNDFoundGreater == err
            || wrnNDFoundLess == err )
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"c89ec3eb-cbdd-4fd9-896f-c3a9991f7aea" );
        err = ErrERRCheck( JET_errLVCorrupted );
    }

    FUCBResetLimstat( pfucbLV );
    FUCBResetUpper( pfucbLV );
    FUCBResetInclusive( pfucbLV );
    pfucbLV->dataSearchKey.SetPv( NULL );
    pfucbLV->dataSearchKey.SetCb( 0 );

    return err;
}

//  comparison function for LIDs for preread ordering
//
BOOL CmpLid( _In_ const LvId& lid1, _In_ const LvId& lid2 )
{
    return lid1 < lid2;
}

//  preread array of LIDs
//
ERR ErrLVPrereadLongValues(
    FUCB * const                        pfucb,
    __inout_ecount(clid) LvId * const   plid,
    const ULONG                         clid,
    const ULONG                         cPageCacheMin,
    const ULONG                         cPageCacheMax,
    LONG * const                        pclidPrereadActual,
    ULONG * const                       pcPageCacheActual,
    const JET_GRBIT                     grbit )
{
    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( plid != NULL );
    Assert( clid <= clidMostPreread );
    AssumePREFAST( clid <= clidMostPreread );
    JET_ERR     err = JET_errSuccess;
    FUCB        *pfucbLV = pfucbNil;
    LVKEY_BUFFER
                rglvkeyStart[clidMostPreread];
    ULONG       rgcbStart[clidMostPreread];
    VOID        *rgpvStart[clidMostPreread];
    LVKEY_BUFFER rglvkeyEnd[clidMostPreread];
    ULONG       rgcbEnd[clidMostPreread];
    VOID        *rgpvEnd[clidMostPreread];
    const LONG  cbLVChunkMost = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    ULONG       cbLVOnePage = ( (g_cbPage / cbLVChunkMost) * cbLVChunkMost );
    ULONG       cbLVDefaultPages = ( (cpageLVPrereadDefault * g_cbPage / cbLVChunkMost) * cbLVChunkMost );
    ULONG       cbLVPreread = ( (grbit & JET_bitPrereadFirstPage) ? cbLVOnePage : cbLVDefaultPages );

    //  open LV tree and preread values if this table has an LV tree
    //
    Call( ErrDIROpenLongRoot( pfucb, &pfucbLV, fFalse ) );
    Assert( pfucbLV != pfucbNil || wrnLVNoLongValues == err );
    if ( wrnLVNoLongValues != err )
    {
        //  sort LIDs in ascending order to promote IO coallescence
        //
        std::sort( plid, plid + clid, CmpLid );

        //  preread LIDs in ascending order
        //
        for ( ULONG ilidT = 0; ilidT < clid; ilidT++ )
        {
            //  convert LIDs to LV key pairs for desired range
            //
            KEY keyT;
            LVRootKeyFromLid( &rglvkeyStart[ ilidT ], &keyT, plid[ ilidT ] );   // key is needed temporarily
            rgpvStart[ ilidT ] = (VOID *) &rglvkeyStart[ ilidT ];
            rgcbStart[ ilidT ] = keyT.Cb();
            
            //  make key for LID and offset, for first chunk at given offset
            LVKeyFromLidOffset( &rglvkeyEnd[ ilidT ], &keyT, plid[ ilidT ], cbLVPreread - cbLVChunkMost );  // index ranges are inclusive, so move back the end by 1 chunk
            rgpvEnd[ ilidT ] = (VOID *) &rglvkeyEnd[ ilidT ];
            rgcbEnd[ ilidT ] = keyT.Cb();
        }

        Call( ErrBTPrereadKeyRanges(
            pfucbLV->ppib,
            pfucbLV,
            (const VOID **)rgpvStart,
            rgcbStart,
            (const VOID **)rgpvEnd,
            rgcbEnd,
            clid,
            pclidPrereadActual,
            cPageCacheMin,
            cPageCacheMax,
            JET_bitPrereadForward /*LVs sorted ascending*/,
            pcPageCacheActual ) );
    }

HandleError:
    if ( pfucbNil != pfucbLV )
    {
        DIRClose( pfucbLV );
    }

    return err;
}

ERR ErrLVPrereadLongValue(
    _In_ FUCB * const   pfucb,
    _In_ const LvId lid,
    _In_ const ULONG    ulOffset,
    _In_ const ULONG    cbData )
{
    JET_ERR     err             = JET_errSuccess;
    FUCB*       pfucbLV         = pfucbNil;
    LVKEY_BUFFER rglvkeyStart[2];
    ULONG       rgcbStart[2];
    VOID*       rgpvStart[2];
    LVKEY_BUFFER rglvkeyEnd[2];
    ULONG       rgcbEnd[2];
    VOID*       rgpvEnd[2];
    LONG        cRange          = 0;
    KEY         keyT;

    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );

    //  don't preread if we only want to access the root
    if ( cbData == 0 )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    //  open LV tree and preread values if this table has an LV tree
    Call( ErrDIROpenLongRoot( pfucb, &pfucbLV, fFalse ) );
    Assert( pfucbLV != pfucbNil || wrnLVNoLongValues == err );
    if ( wrnLVNoLongValues == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    //  compute the offset range to preread aligned to chunk boundaries
    const LONG  cbLVChunkMost       = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG ulOffsetPreread     = ( ulOffset / cbLVChunkMost ) * cbLVChunkMost;
    const ULONG cbAddPreread        = ulOffset - ulOffsetPreread;
    const ULONG cbPreread           = cbData + cbAddPreread < cbData ? ulMax : cbData + cbAddPreread;
    const ULONG cChunksPartial      = ( cbPreread % cbLVChunkMost ) ? 1 : 0;
    const ULONG cChunksPreread      = ( cbPreread / cbLVChunkMost ) + cChunksPartial;
    const ULONG ulOffsetPrereadLast = ulOffsetPreread + ( ( cChunksPreread - 1 ) * cbLVChunkMost );

    //  the first chunk will be on the same page as the LVROOT. only preread if we want more than that chunk
    if ( ulOffsetPreread == 0 && ulOffsetPrereadLast == 0 )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    //  make one key range for the LV Root
    LVRootKeyFromLid( &rglvkeyStart[ cRange ], &keyT, lid );
    rgpvStart[ cRange ] = (VOID *) &rglvkeyStart[ cRange ];
    rgcbStart[ cRange ] = keyT.Cb();
    rgpvEnd[ cRange ] = rgpvStart[ cRange ];
    rgcbEnd[ cRange ] = rgcbStart[ cRange ];
    cRange++;

    //  make another key range for the requested LV offset range
    LVKeyFromLidOffset( &rglvkeyStart[ cRange ], &keyT, lid, ulOffsetPreread );
    rgpvStart[ cRange ] = (VOID *) &rglvkeyStart[ cRange ];
    rgcbStart[cRange] = keyT.Cb();
    LVKeyFromLidOffset( &rglvkeyEnd[ cRange ], &keyT, lid, ulOffsetPrereadLast );
    rgpvEnd[ cRange ] = (VOID *) &rglvkeyEnd[ cRange ];
    rgcbEnd[cRange] = keyT.Cb();
    cRange++;

    //  preread these ranges of the LV tree but only up to our sequential preread limit

    LONG cRangePreread;
    Call( ErrBTPrereadKeyRanges(
        pfucbLV->ppib,
        pfucbLV,
        (const VOID **)rgpvStart,
        rgcbStart,
        (const VOID **)rgpvEnd,
        rgcbEnd,
        cRange,
        &cRangePreread,
        cRange,
        cpgPrereadSequential,
        JET_bitPrereadForward,
        NULL ) );

HandleError:
    if ( pfucbNil != pfucbLV )
    {
        DIRClose( pfucbLV );
    }

    return err;
}


//  ================================================================
ERR ErrDIRDownLVData(
    FUCB            *pfucb,
    const LvId      lid,
    const ULONG     ulOffset,
    const DIRFLAG   dirflag )
//  ================================================================
{
    return ErrDIRDownLVDataPreread( pfucb, lid, ulOffset, dirflag, 0 );
}

//  ================================================================
ERR ErrDIRDownLVRootPreread(
    FUCB * const    pfucbLV,
    const LvId      lid,
    const DIRFLAG   dirflag,
    const ULONG     cbPreread,
    const BOOL      fCorruptIfMissing )
//  ================================================================
//
//  This takes an FUCB that is opened on the Long Value tree table and seeks
//  to the LVROOT node
//
//  cbPreread is used to preread the LV chunks on the way down.
//
//-
{
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( lid > lidMin );

    DIRUp( pfucbLV );

    //  this key is used if we do an index-range preread
    LVKEY_BUFFER lvkeyPrereadLast;

    //  make a key that consists of the LID only

    LVKEY_BUFFER lvkey;
    BOOKMARK    bm;
    LVRootKeyFromLid( &lvkey, &bm.key, lid );
    bm.data.Nullify();

    DIB dib;
    dib.dirflag = dirflag | fDIRExact;
    dib.pos     = posDown;
    dib.pbm     = &bm;

    // the first chunk will be on the same page as the LVROOT. only preread if we want more than that chunk
    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    if ( cbPreread > (ULONG)cbLVChunkMost )
    {
        // we do the math this way to avoid integer overflow if cbPreread is 0xFFFFFFFF
        const ULONG cChunksPartial  = ( cbPreread % cbLVChunkMost ) ? 1 : 0;
        const ULONG cChunksPreread  = ( cbPreread  / cbLVChunkMost ) + cChunksPartial;
        const ULONG ulOffsetPrereadLast = ( cChunksPreread - 1 ) * cbLVChunkMost;
        Assert( ulOffsetPrereadLast > 0 );

        //  make a key that consists of the long id and the offset
        KEY keyPrereadLast;
        LVKeyFromLidOffset( &lvkeyPrereadLast, &keyPrereadLast, lid, ulOffsetPrereadLast );

        //  set the FUCB to have an index range
        Assert( !FFUCBLimstat( pfucbLV ) );
        Assert( pfucbLV->dataSearchKey.FNull() );
        
        FUCBSetPrereadForward( pfucbLV, cpgPrereadSequential );
        FUCBSetLimstat( pfucbLV );
        FUCBSetUpper( pfucbLV );
        FUCBSetInclusive( pfucbLV );
        pfucbLV->dataSearchKey.SetPv( keyPrereadLast.suffix.Pv() ); // prefix is always null for keys constructed by LVKeyFromLidOffset()
        pfucbLV->dataSearchKey.SetCb( keyPrereadLast.suffix.Cb() );
    }

    PERFOpt( PERFIncCounterTable( cLVSeeks, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    ERR err     = ErrDIRDown( pfucbLV, &dib );

    if ( JET_errSuccess == err )
    {
        AssertLVRootNode( pfucbLV, lid );
    }
    else if ( wrnNDFoundGreater == err
              || wrnNDFoundLess == err
              || JET_errRecordNotFound == err )
    {
        if ( fCorruptIfMissing )
        {
            Assert( pfucbLV->ppib->Level() > 0 );
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"14d52e2e-b5f6-43f4-aace-645012367fa5" );
            err = ErrERRCheck( JET_errLVCorrupted );
        }

        err = ErrERRCheck( JET_errRecordNotFound );
    }

    FUCBResetLimstat( pfucbLV );
    FUCBResetUpper( pfucbLV );
    FUCBResetInclusive( pfucbLV );
    pfucbLV->dataSearchKey.SetPv( NULL );
    pfucbLV->dataSearchKey.SetCb( 0 );

    return err;
}

//  ================================================================
ERR ErrDIRDownLVRootPreread(
    FUCB * const    pfucbLV,
    const LvId      lid,
    const DIRFLAG   dirflag,
    const ULONG     cbPreread )
    //  ================================================================
{
    return ErrDIRDownLVRootPreread( pfucbLV, lid, dirflag, cbPreread, !g_fRepair );
}

//  ================================================================
ERR ErrDIRDownLVRoot(
    FUCB * const    pfucb,
    const LvId      lid,
    const DIRFLAG   dirflag )
//  ================================================================
{
    return ErrDIRDownLVRootPreread( pfucb, lid, dirflag, 0 );
}

//  ================================================================
INLINE ERR ErrRECISetLid(
    FUCB            *pfucb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const LvId      lid )
//  ================================================================
//
//  Sets the separated LV struct in the record to point to the given LV.
//  Used after an LV is copied.
//
//-
{
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( lid > lidMin );
    Assert( lid.FLidObeysCurrFormat( pfucb ) );

    UnalignedLittleEndian< _LID64 >     lidT    = lid;
    DATA                                dataLV;

    dataLV.SetPv( &lidT );
    dataLV.SetCb( lid.FIsLid64() ? sizeof( _LID64 ) : sizeof( _LID32 ) );

    const ERR   err     = ErrRECSetColumn(
                                pfucb,
                                columnid,
                                itagSequence,
                                &dataLV,
                                grbitSetColumnSeparated );
    return err;
}

//  ================================================================
LOCAL bool FLVCompressedChunk( const LONG cbLVChunkMost, const KEY& key, const DATA& data, const ULONG ulLVSize )
//  ================================================================
//
//  Use the size of the LV to determine if it is compressed.
//
//-
{
    LvId lid;
    ULONG ulOffset;
    LidOffsetFromKey( &lid, &ulOffset, key );
    Assert( ulOffset % cbLVChunkMost == 0 );

    const bool fNotLastChunk = ( ulOffset + cbLVChunkMost ) < ulLVSize;
    const INT cbExpected = fNotLastChunk ? cbLVChunkMost : ( ulLVSize - ulOffset );
    Assert( data.Cb() <= cbExpected );
    if( data.Cb() < cbExpected )
    {
        return true;
    }
    return false;
}

LOCAL CompressFlags LVIAddCompressionFlagsIfEnabled(
        CompressFlags compressFlags,
        const INST* const pinst,
        const IFMP ifmp
        )
{
    // If user asks for xpress and xpress10 is enabled, use that.
    if ( (compressFlags & compressXpress) &&
         BoolParam( pinst, JET_paramFlight_EnableXpress10Compression ) &&
         g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvXpress10Compression ) >= JET_errSuccess )
    {
        return CompressFlags( compressFlags | compressXpress10 );
    }

    // If user asks for xpress and lz4 is enabled, use that.
    if ( (compressFlags & compressXpress) &&
         g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvLz4Compression ) >= JET_errSuccess )
    {
        return CompressFlags( compressFlags | compressLz4 );
    }

    return compressFlags;
}

//  ================================================================
LOCAL ERR ErrLVITryCompress(
    FUCB *pfucbLV,
    const DATA& data,
    const INST* const pinst,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    FUCB *pfucbTable,
    _Out_ DATA *pdataToSet,
    _Out_ BYTE ** pbAlloc )
//  ================================================================
//
//  Returns the data to be set in the LV tree. Compression is tried,
//  and if compression succeeds the compressed data is returned, 
//  otherwise the original data is returned.
//
//  If memory has to be allocated, the pointer is returned in *pbAlloc.
//  That point should be freed with PKFreeCompressionBuffer()
//
//-
{
    ERR err = JET_errSuccess;
    *pbAlloc = NULL;
    
    BYTE * pbDataCompressed = NULL;

    pdataToSet->SetPv( const_cast<VOID *>( data.Pv() ) );
    pdataToSet->SetCb( data.Cb() );

    // Compress if the user asked for it and the chi-squared test estimates that we will get good results
    bool fTryCompress = false;
    if ( compressNone != compressFlags )
    {
        fTryCompress = true;
        if ( data.Cb() >= cbMinChiSquared )
        {
            __declspec( align( 64 ) ) WORD rgwFreqTable[ 256 ] = { 0 };
            const INT cbSample = min( data.Cb(), cbChiSquaredSample );
            double dChiSquared = ChiSquaredSignificanceTest( (const BYTE*) data.Pv(), cbSample, rgwFreqTable );
            if ( dChiSquared < dChiSquaredThreshold )
            {
                fTryCompress = false;
            }
        }
    }

    if ( fTryCompress && NULL != ( pbDataCompressed = PbPKAllocCompressionBuffer() ) )
    {
        CompressFlags compressFlagsEffective = LVIAddCompressionFlagsIfEnabled( compressFlags, pinst, pfucbTable->ifmp );

        INT cbDataCompressedActual;
        const ERR errT = ErrPKCompressData(
            data,
            compressFlagsEffective,
            pinst,
            pbDataCompressed,
            CbPKCompressionBuffer(),
            &cbDataCompressedActual );

        // There isn't a separate compressed bit, the size of the node is used to determine
        // if the data is compressed. That means the compressed data must be smaller.       
        if ( errT >= JET_errSuccess && cbDataCompressedActual < data.Cb() )
        {
            *pbAlloc = pbDataCompressed;
            pdataToSet->SetPv( pbDataCompressed );
            pdataToSet->SetCb( cbDataCompressedActual );
        }
        else
        {
            // failed to compress. use the original data
            PKFreeCompressionBuffer( pbDataCompressed );
            Assert( pdataToSet->Pv() == data.Pv() );
            Assert( pdataToSet->Cb() == data.Cb() );
        }
    }
    else
    {
        Assert( pdataToSet->Pv() == data.Pv() );
        Assert( pdataToSet->Cb() == data.Cb() );
    }

    if ( fEncrypted )
    {
        BYTE *pbDataEncrypted = NULL;
        const ULONG cbDataEncryptedNeeded = CbOSEncryptAes256SizeNeeded( pdataToSet->Cb() );
        if ( cbDataEncryptedNeeded > (ULONG)CbPKCompressionBuffer() )
        {
            Assert( fFalse );
            return ErrERRCheck( JET_errInternalError );
        }

        if ( *pbAlloc != NULL )
        {
            Assert( pdataToSet->Pv() == *pbAlloc );
            pbDataEncrypted = *pbAlloc;
        }
        else
        {
            pbDataEncrypted = PbPKAllocCompressionBuffer();
            if ( pbDataEncrypted == NULL )
            {
                return ErrERRCheck( JET_errOutOfMemory );
            }
            UtilMemCpy( pbDataEncrypted, pdataToSet->Pv(), pdataToSet->Cb() );
        }

        ULONG cbDataEncryptedActual = pdataToSet->Cb();
        err = ErrOSUEncrypt(
                pbDataEncrypted,
                &cbDataEncryptedActual,
                CbPKCompressionBuffer(),
                pfucbLV,
                pfucbTable );
        if ( err < JET_errSuccess )
        {
            PKFreeCompressionBuffer( pbDataEncrypted );
            return err;
        }
        *pbAlloc = pbDataEncrypted;
        pdataToSet->SetPv( pbDataEncrypted );
        pdataToSet->SetCb( cbDataEncryptedActual );
    }

    return JET_errSuccess;
}

//  ================================================================
LOCAL ERR ErrLVInsert(
    FUCB * const pfucbLV,
    const KEY& key,
    const DATA& data,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    FUCB *pfucbTable,
    const DIRFLAG dirflag )
//  ================================================================
//
//  Compress the data (if possible) then insert it into the LV tree
//
//-
{
    ERR err = JET_errSuccess;

    BYTE * pbToFree = NULL;
    DATA dataToSet;
    Call( ErrLVITryCompress( pfucbLV, data, PinstFromPfucb( pfucbLV ), compressFlags, fEncrypted, pfucbTable, &dataToSet, &pbToFree ) );

    Call( ErrDIRInsert( pfucbLV, key, dataToSet, dirflag ) );
    
HandleError:
    PKFreeCompressionBuffer( pbToFree );
    return err;
}


//  ================================================================
LOCAL ERR ErrLVReplace(
    FUCB * const pfucbLV,
    const DATA& data,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    const DIRFLAG dirflag )
//  ================================================================
//
//  Compress the data (if possible) then replace the current node inthe LV tree
//
//-
{
    ERR err = JET_errSuccess;

    BYTE * pbToFree = NULL;
    DATA dataToSet;
    Call( ErrLVITryCompress( pfucbLV, data, PinstFromPfucb( pfucbLV ), compressFlags, fEncrypted, pfucbLV->pfucbTable, &dataToSet, &pbToFree ) );

    Call( ErrDIRReplace( pfucbLV, dataToSet, dirflag ) );
    
HandleError:
    PKFreeCompressionBuffer( pbToFree );
    return err;
}

//  ================================================================
LOCAL void LVICaptureCorruptedLVChunkInfo(
    const FUCB* const pfucbLV,
    const KEY& key )
//  ================================================================
{
    // Convert key to string. Key is already big endian.
    const WCHAR wchHexLUT[ 16 ] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
    BYTE* keyBuff = (BYTE*) _alloca( key.Cb() );
    key.CopyIntoBuffer( keyBuff, key.Cb() );

    INT cBuff = 2 * key.Cb() + 1; // 2 chars to display each byte
    WCHAR* wszKey = (WCHAR*) _alloca( cBuff * sizeof( WCHAR ) );
    WCHAR* pwszT = wszKey;

    for ( INT i = 0; i < key.Cb(); i++ )
    {
        BYTE b = keyBuff[ i ];
        *pwszT++ = wchHexLUT[ b >> 4 ];
        *pwszT++ = wchHexLUT[ b & 0x0f ];
    }

    Assert( pwszT < wszKey + cBuff );
    *pwszT = 0;

    const WCHAR *rgpsz[ 3 ];
    rgpsz[ 0 ] = g_rgfmp[ pfucbLV->ifmp ].WszDatabaseName();
    WCHAR szPageNumber[ 32 ];
    OSStrCbFormatW( szPageNumber, sizeof( szPageNumber ), L"%d (0x%X)", pfucbLV->u.pfcb->PgnoFDP(), pfucbLV->u.pfcb->PgnoFDP() );
    rgpsz[ 1 ] = szPageNumber;
    rgpsz[ 2 ] = wszKey;

    UtilReportEvent( eventError,
        DATABASE_CORRUPTION_CATEGORY,
        COMPRESSED_LVCHUNK_CHECKSUM_MISMATCH_ID,
        _countof( rgpsz ),
        rgpsz,
        0,
        NULL,
        PinstFromPfucb( pfucbLV ) );
}

//  ================================================================
LOCAL ERR ErrLVIDecompressAndCompare(
    const FUCB* const pfucbLV,
    const LvId lid,
    const DATA& dataCompressed,
    const INT ibOffset,
    __in_bcount( cbData ) const BYTE * const pbData,
    const INT cbData,
    _Out_ bool * pfIdentical )
//  ================================================================
//
//  Decompress data, and compare with the passed in data
//
//-
{
    ERR err;

    BYTE * pbDecompressed = NULL;
    INT cbDecompressed = 0;

    Call( ErrPKAllocAndDecompressData( dataCompressed, pfucbLV, &pbDecompressed, &cbDecompressed ) );
    Assert( ( cbDecompressed - ibOffset ) >= cbData );
    if ( ( cbDecompressed - ibOffset ) < cbData )
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"989222f8-ff0d-46a6-a58a-86f5c9cb472a" );
        return ErrERRCheck( JET_errLVCorrupted );
    }
    
    if ( 0 != memcmp( pbData, pbDecompressed + ibOffset, cbData ) )
    {
        *pfIdentical = false;
    }
    else
    {
        *pfIdentical = true;
    }
    
HandleError:
    delete [] pbDecompressed;
    return err;
}

//  ================================================================
LOCAL ERR ErrLVIDecompress(
    const FUCB* const pfucbLV,
    const LvId lid,
    const DATA& dataCompressed,
    const INT ibOffset,
    __out_bcount( cbData ) BYTE * const pbData,
    const INT cbData )
//  ================================================================
//
//  Decompress data, dealing with the ibOffset
//
//-
{
    ERR err;

    if( 0 == ibOffset )
    {
        INT cbDataActual;
        Call( ErrPKDecompressData(
                dataCompressed,
                pfucbLV,
                pbData,
                cbData,
                &cbDataActual ) );
    }
    else
    {
        // we are retrieving compressed data, starting at a non-zero offset
        // we need to allocate a temporary buffer, decompress into
        // the buffer and then copy the desired data out. we can't
        // start decompressing in the middle of the blob, we have
        // to start at the beginning.
        //
        BYTE * pbDecompressed = NULL;
        INT cbDecompressed = 0;
        
        Call( ErrPKAllocAndDecompressData( dataCompressed, pfucbLV, &pbDecompressed, &cbDecompressed ) );
        if( (ULONG)( ibOffset + cbData ) > (ULONG)cbDecompressed )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"6c499d75-2c5f-432a-bcf9-6de555ac0d7f" );
            return ErrERRCheck( JET_errLVCorrupted );
        }
        UtilMemCpy( pbData, pbDecompressed+ibOffset, cbData );
        delete[] pbDecompressed;
    }
    
HandleError:
    return err;
}

//  ================================================================
LOCAL ERR ErrLVRetrieve(
    const FUCB* const pfucbLV,
    const LvId lid,
    const ULONG ulLVSize,
    const KEY& key,
    const DATA& data,
    const BOOL fEncrypted,
    const INT ibOffset,
    _Out_writes_bytes_to_( *pcb, *pcb ) BYTE * const pb,
    ULONG *pcb )
//  ================================================================
//
//  Copies from data to the provided buffer, decompressing
//  if necessary
//
//-
{
    ERR err = JET_errSuccess;
    DATA dataIn = data;
    BYTE *pbDataDecrypted = NULL;
    INT cbActual, cbRetrieve;
    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();

#ifdef DEBUG
    LvId lidT;
    ULONG ulOffsetT;
    LidOffsetFromKey( &lidT, &ulOffsetT, key );
    Assert( lidT == lid );
    Assert( ulOffsetT % cbLVChunkMost == 0 );
    Assert( ulOffsetT < ulLVSize );
#endif

    if ( dataIn.Cb() > CbPKCompressionBuffer() )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errInternalError );
    }

    if ( fEncrypted )
    {
        Alloc( pbDataDecrypted = PbPKAllocCompressionBuffer() );
        ULONG cbDataDecryptedActual = dataIn.Cb();
        Call( ErrOSUDecrypt(
                    (BYTE*)(BYTE*)dataIn.Pv(),
                    pbDataDecrypted,
                    &cbDataDecryptedActual,
                    pfucbLV,
                    pfucbLV->pfucbTable ) );
        dataIn.SetPv( pbDataDecrypted );
        dataIn.SetCb( cbDataDecryptedActual );
    }

    if ( dataIn.Cb() > cbLVChunkMost )
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"cd9c27df-d859-47cd-a480-ac865087668a" );
        Error( ErrERRCheck( JET_errLVCorrupted ) );
    }

    if( FLVCompressedChunk( cbLVChunkMost, key, dataIn, ulLVSize ) )
    {
        // Getting uncompressed size is cheap for current compression algorithms, so ok to call it before doing actual decompression
        Call( ErrPKDecompressData( dataIn, pfucbLV, NULL, 0, &cbActual ) );
        if( JET_wrnBufferTruncated == err )
        {
            // this is the expected error
            err = JET_errSuccess;
        }
        if ( cbActual > cbLVChunkMost )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"7c027fb9-768c-4c91-ab5c-543ea6ac0db7" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }
        cbRetrieve = min( cbActual - ibOffset, (INT)*pcb );
        Call( ErrLVIDecompress( pfucbLV, lid, dataIn, ibOffset, pb, cbRetrieve ) );
        *pcb = cbRetrieve;
    }
    else
    {
        INT cbT = dataIn.Cb();
        cbRetrieve = min( cbT - ibOffset, (INT)*pcb );
        UtilMemCpy( pb, (BYTE *)dataIn.Pv() + ibOffset, cbRetrieve );
        *pcb = cbRetrieve;
    }

HandleError:

    if ( err == JET_errCompressionIntegrityCheckFailed )
    {
        LVICaptureCorruptedLVChunkInfo( pfucbLV, key );
    }

    if ( pbDataDecrypted )
    {
        PKFreeCompressionBuffer( pbDataDecrypted );
        pbDataDecrypted = NULL;
    }

    return err;
}

//  ================================================================
LOCAL ERR ErrLVCompare(
    const FUCB* const pfucbLV,
    const LvId lid,
    const ULONG ulLVSize,
    const KEY& key,
    const DATA& data,
    const BOOL fEncrypted,
    const INT ibOffset,
    _In_reads_( *pcb ) const BYTE * const pb,
    ULONG *pcb,
    _Out_ bool * const pfIdentical )
//  ================================================================
//
//  Compares data to the provided buffer, decompressing if necessary
//
//-
{
    ERR err = JET_errSuccess;
    DATA dataIn = data;
    BYTE *pbDataDecrypted = NULL;
    INT cbActual, cbCompare;
    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();

#ifdef DEBUG
    LvId lidT;
    ULONG ulOffsetT;
    LidOffsetFromKey( &lidT, &ulOffsetT, key );
    Assert( lidT == lid );
    Assert( ulOffsetT % cbLVChunkMost == 0 );
    Assert( ulOffsetT < ulLVSize );
#endif

    if ( dataIn.Cb() > CbPKCompressionBuffer() )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errInternalError );
    }

    if ( fEncrypted )
    {
        Alloc( pbDataDecrypted = PbPKAllocCompressionBuffer() );
        ULONG cbDataDecryptedActual = dataIn.Cb();
        Call( ErrOSUDecrypt(
                    (BYTE*)dataIn.Pv(),
                    pbDataDecrypted,
                    &cbDataDecryptedActual,
                    pfucbLV,
                    pfucbLV->pfucbTable ) );
        dataIn.SetPv( pbDataDecrypted );
        dataIn.SetCb( cbDataDecryptedActual );
    }

    if ( dataIn.Cb() > cbLVChunkMost )
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"88fafa3c-0baf-4e7c-8498-6020ed6c91f7" );
        Error( ErrERRCheck( JET_errLVCorrupted ) );
    }

    if( FLVCompressedChunk( cbLVChunkMost, key, dataIn, ulLVSize ) )
    {
        // Getting uncompressed size is cheap for current compression algorithms, so ok to call it before doing actual decompression
        Call( ErrPKDecompressData( dataIn, pfucbLV, NULL, 0, &cbActual ) );
        if( JET_wrnBufferTruncated == err )
        {
            // this is the expected error
            err = JET_errSuccess;
        }
        if ( cbActual > cbLVChunkMost )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"e05aaeb7-a975-49ca-884a-a2bd73f8ad53" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }
        cbCompare = min( cbActual - ibOffset, (INT)*pcb );
        Call( ErrLVIDecompressAndCompare( pfucbLV, lid, dataIn, ibOffset, pb, cbCompare, pfIdentical ) );
        *pcb = cbCompare;
    }
    else
    {
        cbCompare = min( dataIn.Cb() - ibOffset, (INT)*pcb );
        *pfIdentical = ( 0 == memcmp( pb, (BYTE *)dataIn.Pv() + ibOffset, cbCompare ) );
        *pcb = cbCompare;
    }

HandleError:
    if ( err == JET_errCompressionIntegrityCheckFailed )
    {
        LVICaptureCorruptedLVChunkInfo( pfucbLV, key );
    }

    if ( pbDataDecrypted )
    {
        PKFreeCompressionBuffer( pbDataDecrypted );
        pbDataDecrypted = NULL;
    }

    return err;
}

//  ================================================================
LOCAL ERR ErrLVIGetDataSize(
    const FUCB* const pfucbLV,
    const KEY& key,
    const DATA& data,
    const BOOL fEncrypted,
    const ULONG ulLVSize,
    _Out_ ULONG * const pulActual,
    const LONG cbLVChunkMostOverride = 0 )
//  ================================================================
//
//  Gets the uncompressed size of the data
//
//-
{
    ERR err = JET_errSuccess;
    DATA dataIn = data;
    BYTE *pbDataDecrypted = NULL;

    if ( dataIn.Cb() > CbPKCompressionBuffer() )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errInternalError );
    }

    if ( fEncrypted )
    {
        Alloc( pbDataDecrypted = PbPKAllocCompressionBuffer() );
        ULONG cbDataDecryptedActual = dataIn.Cb();
        Call( ErrOSUDecrypt(
                    (BYTE*)dataIn.Pv(),
                    pbDataDecrypted,
                    &cbDataDecryptedActual,
                    pfucbLV,
                    pfucbLV->pfucbTable ) );
        dataIn.SetPv( pbDataDecrypted );
        dataIn.SetCb( cbDataDecryptedActual );
    }

    if( FLVCompressedChunk(
                pfucbLV ? pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() : cbLVChunkMostOverride,
                key,
                dataIn,
                ulLVSize ) )
    {
        Call( ErrPKDecompressData(
                    dataIn,
                    pfucbLV,
                    NULL,
                    0,
                    (INT *)pulActual ) );
        if( JET_wrnBufferTruncated == err )
        {
            // this is the expected error
            err = JET_errSuccess;
        }
    }
    else
    {
        *pulActual = dataIn.Cb();
    }

HandleError:

    if ( pbDataDecrypted )
    {
        PKFreeCompressionBuffer( pbDataDecrypted );
        pbDataDecrypted = NULL;
    }

    return err;
}


//  ========================================================================
ERR ErrLVInit( INST *pinst )
//  ========================================================================
//
//  This may leak critical sections if ErrPIBBeginSession fails.
//  Is this important?
//
//-
{
    ERR err = JET_errSuccess;
    Call( ErrPIBBeginSession( pinst, &pinst->m_ppibLV, procidNil, fFalse ) );

HandleError:
    return err;
}


//  ========================================================================
VOID LVTerm( INST * pinst )
//  ========================================================================
{
    Assert( pinst->m_critLV.FNotOwner() );

    if ( pinst->m_ppibLV )
    {
        // We don't end LV session here, because PurgeAllDatabases/PIBTerm is
        // doing it in the correct order (closing FUCBs before ending sessions),
        // and ending session here doesn't work with failure case such as 
        // when rollback failed, in which case there could still be FUCBs not closed.
        //
        // PIBEndSession( pinst->m_ppibLV );

#ifdef DEBUG
        const PIB* const ppibLV = pinst->m_ppibLV;

        //  There should not be any fucbOfSession attached, unless there's rollback failure and instance unavailable
        Assert( pfucbNil == ppibLV->pfucbOfSession || pinst->FInstanceUnavailable() );

        // Ensure the LV PIB is in the global PIB list,
        // so that we know PurgeAllDatabases/PIBTerm will take care of closing it
        const PIB* ppibT = pinst->m_ppibGlobal;
        for ( ; ppibT != ppibLV && ppibT != ppibNil; ppibT = ppibT->ppibNext );
        Assert( ppibT == ppibLV );
#endif // DEBUG

        pinst->m_ppibLV = ppibNil;
    }
}


// WARNING: See "LV grbit matrix" before making any modifications here.
INLINE ERR ErrLVOpFromGrbit(
    const JET_GRBIT grbit,
    const ULONG     cbData,
    const BOOL      fNewInstance,
    const ULONG     ibLongValue,
    LVOP            *plvop )
{
    const BOOL      fNoCbData = ( 0 == cbData );

    switch ( grbit )
    {
        case JET_bitSetAppendLV:
            if ( fNoCbData )
            {
                *plvop = ( fNewInstance ? lvopInsertZeroLength : lvopNull );
            }
            else
            {
                *plvop = ( fNewInstance ? lvopInsert : lvopAppend );
            }
            break;
        case JET_bitSetOverwriteLV:
            if ( fNoCbData )
            {
                if ( fNewInstance )
                {
                    if ( 0 == ibLongValue )
                        *plvop = lvopInsertZeroLength;
                    else
                    {
                        *plvop = lvopNull;
                        return ErrERRCheck( JET_errColumnNoChunk );
                    }
                }
                else
                    *plvop = lvopNull;
            }
            else
            {
                if ( fNewInstance )
                {
                    if ( 0 == ibLongValue )
                        *plvop = lvopInsert;
                    else
                    {
                        *plvop = lvopNull;
                        return ErrERRCheck( JET_errColumnNoChunk );
                    }
                }
                else
                    *plvop = lvopOverwriteRange;
            }
            break;
        case JET_bitSetSizeLV:
            if ( fNoCbData )
            {
                *plvop = ( fNewInstance ? lvopInsertNull : lvopReplaceWithNull );
            }
            else
            {
                *plvop = ( fNewInstance ? lvopInsertZeroedOut : lvopResize );
            }
            break;
        case JET_bitSetZeroLength:
            if ( fNoCbData )
            {
                *plvop = ( fNewInstance ? lvopInsertZeroLength : lvopReplaceWithZeroLength );
            }
            else
            {
                *plvop = ( fNewInstance ? lvopInsert : lvopReplace );
            }
            break;
        case JET_bitSetSizeLV|JET_bitSetZeroLength:
            if ( fNoCbData )
            {
                *plvop = ( fNewInstance ? lvopInsertZeroLength : lvopReplaceWithZeroLength );
            }
            else
            {
                *plvop = ( fNewInstance ? lvopInsertZeroedOut : lvopResize );
            }
            break;
        case JET_bitSetSizeLV|JET_bitSetOverwriteLV:
            if ( fNoCbData )
            {
                *plvop = ( fNewInstance ? lvopInsertNull : lvopReplaceWithNull );
            }
            else if ( fNewInstance )
            {
                if ( 0 == ibLongValue )
                    *plvop = lvopInsert;
                else
                {
                    *plvop = lvopNull;
                    return ErrERRCheck( JET_errColumnNoChunk );
                }
            }
            else
            {
                *plvop = lvopOverwriteRangeAndResize;
            }
            break;
        default:
            Assert( 0 == grbit );
            if ( fNoCbData )
            {
                *plvop = ( fNewInstance ? lvopInsertNull : lvopReplaceWithNull );
            }
            else
            {
                *plvop = ( fNewInstance ? lvopInsert : lvopReplace );
            }
    }

    return JET_errSuccess;
}

//  ================================================================
LOCAL ERR ErrRECIOverwriteSeparateLV(
    _In_ FUCB * const pfucb,
    _In_ const DATA * const pdataInRecord,
    __inout DATA * const pdataNew,
    _In_ const CompressFlags compressFlags,
    _In_ const BOOL fEncrypted,
    __inout ULONG * const pibLongValue,
    _Out_ LvId * const plid )
//  ================================================================
{
    Assert( pfucb );
    ASSERT_VALID( pfucb );
    Assert( pdataInRecord );
    ASSERT_VALID( pdataInRecord );
    Assert( pdataNew );
    ASSERT_VALID( pdataNew );
    Assert( plid );

    Assert( pdataInRecord->Cb() < g_cbPage );
    Assert( pdataInRecord->Cb() > 0 || pdataNew->Cb() > 0 );
    
#ifdef DEBUG
    //  store these values to assert that the output parameters are set correctly
    const BYTE * const pbDataNewOrig    = reinterpret_cast<BYTE *>( pdataNew->Pv() );
    const size_t cbDataNewOrig          = pdataNew->Cb();
    const size_t ibLongValueOrig        = *pibLongValue;
#endif

    ERR err = JET_errSuccess;

    // we will combine the old/new data to create the first chunk
    // this requires allocating a temporary buffer (using BFAlloc)
    VOID *pvbf = NULL;
    BFAlloc( bfasTemporary, &pvbf );

    DATA data;
    data.SetPv( pvbf );
    data.SetCb( 0 );

    BYTE * pbBuffer         = reinterpret_cast<BYTE *>( pvbf );
    const LONG cbBuffer     = g_cbPage;
    LONG cbBufferRemaining  = cbBuffer;
    LONG cbToCopy;

    // copy the data from the record. we only want to copy
    // data that won't be overwritten by the new data

    if( *pibLongValue > static_cast<ULONG>( pdataInRecord->Cb() ) )
    {
        FireWall( "OffsetTooHighOnOverwriteSepLv" );    // expect the caller to prevent this
        Call( ErrERRCheck( JET_errColumnNoChunk ) );
    }
    
    cbToCopy = min( static_cast<ULONG>( pdataInRecord->Cb() ), *pibLongValue );
    Assert( cbToCopy >= 0 );
    Assert( cbToCopy <= cbBufferRemaining );
    UtilMemCpy( pbBuffer, pdataInRecord->Pv(), cbToCopy );
    pbBuffer += cbToCopy;
    cbBufferRemaining -= cbToCopy;
    data.DeltaCb( cbToCopy );
    Assert( pbBuffer <= pbBuffer+cbBuffer );
    Assert( cbBufferRemaining >= 0 );
    Assert( data.Cb() == cbBuffer - cbBufferRemaining );
    
    // now copy the new data into the buffer
    
    cbToCopy = min( pdataNew->Cb(), cbBufferRemaining );
    Assert( cbToCopy >= 0 );
    Assert( cbToCopy <= cbBufferRemaining );
    UtilMemCpy( pbBuffer, pdataNew->Pv(), cbToCopy );
    pbBuffer += cbToCopy;
    cbBufferRemaining -= cbToCopy;
    data.DeltaCb( cbToCopy );
    Assert( pbBuffer <= pbBuffer+cbBuffer );
    Assert( cbBufferRemaining >= 0 );
    Assert( data.Cb() == cbBuffer - cbBufferRemaining );

    // adjust the output parameters by the amount
    // of data copied from the new data buffer
    
    pdataNew->DeltaCb( -cbToCopy ); // less data to copy
    pdataNew->DeltaPv( cbToCopy );  // from further in the buffer
    *pibLongValue += cbToCopy;      // inserted later in the LV

    Assert( data.Cb() >= 0 );
    Assert( data.Pv() || data.Cb() == 0 );
    Assert( data.Cb() <= cbBuffer );
    Assert( cbBufferRemaining == 0 || pdataNew->Cb() == 0 ); // if we didn't fill the buffer then we ran out of data
    Assert( data.Cb() == g_cbPage || pdataNew->Cb() == 0 ); // we are either inserting a full LV chunk or all the data was used

    // insert the data chunk and create the LVROOT
    Call( ErrRECSeparateLV( pfucb, &data, compressFlags, fEncrypted, plid, NULL ) );

#ifdef DEBUG
    // check the output parameters
    Assert( 0 != *plid );
    ASSERT_VALID( pdataNew );
    if( pbDataNewOrig == pdataNew->Pv() )
    {
        // nothing changed
        Assert( static_cast<size_t>( pdataNew->Cb() ) == cbDataNewOrig );
        Assert( ibLongValueOrig == *pibLongValue );
    }
    else
    {
        // the pointer changed, the cb of the data and the ibOffset should
        // be updated as well
        const size_t cbDiff = reinterpret_cast<BYTE *>( pdataNew->Pv() ) - pbDataNewOrig;
        Assert( cbDiff > 0 );
        Assert( ( cbDataNewOrig - pdataNew->Cb() ) == cbDiff );
        Assert( ( *pibLongValue - ibLongValueOrig ) == static_cast<ULONG>( cbDiff ) );
        Assert( pdataNew->Cb() == 0 || *pibLongValue == static_cast<ULONG>( g_cbPage ) );
    }
#endif

HandleError:
    BFFree( pvbf ); // this function handles the NULL pointer correctly
    return err;
}
    
//  ================================================================
LOCAL ERR ErrRECIAppendSeparateLV(
    _In_ FUCB * const pfucb,
    _In_ const DATA * const pdataInRecord,
    __inout DATA * const pdataNew,
    _In_ const CompressFlags compressFlags,
    _In_ const BOOL fEncrypted,
    _Out_ LvId * const plid )
//  ================================================================
{
    ULONG ibLongValue = pdataInRecord->Cb();
    return ErrRECIOverwriteSeparateLV( pfucb, pdataInRecord, pdataNew, compressFlags, fEncrypted, &ibLongValue, plid );
}

//  ================================================================
LOCAL CPG CpgLVIRequired_( _In_ const ULONG cbPage, _In_ const ULONG cbLvChunk, _In_ const ULONG cbLv )
//  ================================================================
//  Computes the number of pages required for this long-value (LV) given the page and max chunk size.
//-
{
    Expected( cbLv > cbPage );

    const ULONG cChunksPerPage = cbPage / cbLvChunk;
    const CPG cpgRet = roundup( roundup( cbLv, cbLvChunk ) / cbLvChunk, cChunksPerPage ) / cChunksPerPage;

    return cpgRet;
}

//  ================================================================
LOCAL CPG CpgLVIRequired( _In_ const FCB * const pfcbTable, _In_ const ULONG cbLv )
//  ================================================================
//  Computes the number of pages required for this long-value (LV) given a table.
//-
{
    Assert( pfcbTable->FTypeTable() );
    Expected( cbLv > (ULONG)g_cbPage );

    return CpgLVIRequired_( g_cbPage, pfcbTable->Ptdb()->CbLVChunkMost(), cbLv );
}

JETUNITTEST( LV, TestLvToCpgRequirements )
{
    // 32 KB pages, 8192 is 32KB / 4 - so we use 8191 for chunk - 1 less ...
    CHECK( 8 == CpgLVIRequired_( 32 * 1024, 8191, ( 32 * 1024 - 4 ) * 8 ) );    //  exact match of chunk size - fills chunks perfectly.
    CHECK( 8 == CpgLVIRequired_( 32 * 1024, 8191, ( 32 * 1024 - 8 ) * 8 ) );    //  one byte less per chunk - 32 less bytes in last chunk
    CHECK( 9 == CpgLVIRequired_( 32 * 1024, 8191, ( 32 * 1024 - 0 ) * 8 ) );    //  one byte too much per chunk - 32 too many bytes for 8 pages
}

//  ================================================================
LOCAL ERR ErrRECISeparateLV(
    _In_ FUCB * const pfucb,
    _In_ const DATA dataInRecord,
    _In_ DATA dataNew,
    _In_ const CompressFlags compressFlags,
    _In_ const BOOL fEncrypted,
    _In_ BOOL fContiguousLv,
    _In_ ULONG ibLongValue,
    _Out_ LvId * const plid,
    _In_ const ULONG ulColMax,
    _In_ const LVOP lvop )
//  ================================================================
//
//  When bursting an intrinsic LV into a separated LV we want to avoid
//  creating the separated LV with just the data in the record and then
//  appending/overwriting the new data. This function combines the data 
//  into one buffer if necessary. This makes the insertion of the first 
//  data chunk the correct size, which helps split performance
//  (see ErrRECSeparateLV).
//
//  ErrRECSeparateLV only inserts the first data chunk so we initially need
//  to deal with that. The pdataNew and pibLongValue parameters are
//  updated to reflect the data that is copied into the LV. The rest of
//  the work is done by a call to ErrRECAOSeparateLV. 
//
//  As the first chunk is inserted by the call to ErrRECSeparateLV we end
//  up changing the lvop -- inserts become appends.
//
//  This function only works for these cases:
//      - Completely overwriting the intrinsic data with new data. ErrRECSeparateLV
//        is called with the first chunk of the new data.
//      - Appending data to an existing intrinsic LV. ErrRECSeparateLV is called with
//        a chunk made up of the intrinsic data and the new data.
//      - Overwriting some of the intrinsic data and setting the size. ErrRECSeparateLV
//        is called with a chunk made up of the intrinsic data and the new data.
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( plid );
    Assert( dataInRecord.Cb() > 0 || dataNew.Cb() > 0 );    // we shouldn't separate an LV that has no data

    CPG cpgLvSpaceRequired = 0; // 0 really means unknown or dynamic

    *plid = 0;

    ERR err = JET_errSuccess;

    // note that having separate cases avoids allocating a buffer
    // and copying data unless we have both old and new data to combine
    
    switch( lvop )
    {
        default:
            AssertSz( fFalse, "Unexpected lvop" );
            Call( ErrERRCheck( JET_errInternalError ) );
            break;
            
        case lvopInsert:
            Assert( !fContiguousLv || ( lvopInsert == lvop  ) );
            if ( fContiguousLv && lvopInsert == lvop /* just in case */ )
            {
                //  User wants a contiguous laid out LV for better IO performance (over space concerns), so
                //  compute the number of pages required for the LV.
                Assert( dataNew.Cb() > pfucb->u.pfcb->Ptdb()->CbLVChunkMost() );
                Assert( dataNew.Cb() > g_cbPage );  // even stronger, but should be checked externally
                if ( compressFlags != compressNone )
                {
                    //  NYI contiguity for compression - requires trickiness of computing final space ahead of time
                    //  so that we can get final size.
                    AssertSz( FNegTest( fInvalidAPIUsage ), "Use of JET_bitSetContiguousLV not implemented / allowed with compressed column data.  File DCR." );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
                Expected( ibLongValue == 0 );

                cpgLvSpaceRequired = CpgLVIRequired( pfucb->u.pfcb, dataNew.Cb() );
            }

            __fallthrough;
        
        case lvopReplace:
        {
            // pass only enough data for the first LV data chunk

            DATA data;
            data.SetPv( dataNew.Pv() );
            data.SetCb( min( dataNew.Cb(), pfucb->u.pfcb->Ptdb()->CbLVChunkMost() ) );

            // adjust the new data parameters by the amount
            // of data copied from the new data buffer
            
            dataNew.DeltaCb( -data.Cb() );  // less data to copy
            dataNew.DeltaPv( data.Cb() );   // from further in the buffer
            ibLongValue += data.Cb();       // inserted later in the LV

            // we are either inserting a full LV chunk or all the data was used
            Assert( data.Cb() == pfucb->u.pfcb->Ptdb()->CbLVChunkMost() || dataNew.Cb() == 0 );
            Assert( lvop == lvopInsert || cpgLvSpaceRequired == 0 );

            Call( ErrRECICreateLvRootAndChunks( pfucb, &data, compressFlags, fEncrypted, &cpgLvSpaceRequired, plid, NULL ) );
            if ( cpgLvSpaceRequired == 0 )
            {
                //  We have consumed the needed pages for laying this cpg out contiguously, disable ErrRECAOSeparateLV()
                fContiguousLv = fFalse;
            }
            // else ... it is worth noting that if we have fContiguousLv set and we didn't use the pages, there is a significant
            // chance that the LV will be only mostly contiguous ... spread across two IOs, one for the first chunks and one for
            // all the remaining chunks.
            Assert( JET_wrnCopyLongValue == err );
        }
            break;

        case lvopAppend:
            Call( ErrRECIAppendSeparateLV( pfucb, &dataInRecord, &dataNew, compressFlags, fEncrypted, plid ) );
            Assert( JET_wrnCopyLongValue == err );
            break;
            
        case lvopOverwriteRangeAndResize:
            Call( ErrRECIOverwriteSeparateLV( pfucb, &dataInRecord, &dataNew, compressFlags, fEncrypted, &ibLongValue, plid ) );
            Assert( JET_wrnCopyLongValue == err );
            break;
    }
    
    // see if there is any data left to process. all processing of the intrinsic data happened in the
    // calls above (intrinsic data will always fit in the first chunk), so all that has to happen is
    // that the rest of the data is appended
    if( dataNew.Cb() > 0 )
    {
        Call( ErrRECAOSeparateLV( pfucb, plid, &dataNew, compressFlags, fEncrypted, fContiguousLv, ibLongValue, ulColMax, lvopAppend ) );
        Assert( JET_wrnCopyLongValue != err || FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
    }
    else
    {
        Expected( !fContiguousLv ); // it would be confusing as this flag is only used for multi-page / multi-chunk LVs (today).
    }

HandleError:

    return err;
}

//  ================================================================
ERR ErrRECSetLongField(
    FUCB                *pfucb,
    const COLUMNID      columnid,
    const ULONG         itagSequence,
    DATA                *pdataField,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    JET_GRBIT           grbit,
    const ULONG         ibLongValue,
    const ULONG         ulColMax,
    ULONG               cbThreshold )
    //  ================================================================
{
    ERR         err;
    DATA        dataRetrieved;
    BYTE *      pbDecompressed = NULL;
    BYTE *      pbDataDecrypted = NULL;
    dataRetrieved.Nullify();

    //  Consume user preferences for LV intrinsicness, then strip off the flags ...
    const ULONG cbPreferredIntrinsicLV  =
                        //  first process extemporaneous user hints
                        ( grbit & JET_bitSetSeparateLV ) ?
                            LvId::CbLidFromCurrFormat( pfucb ) :    // user wants separated LV, but we do not separate LVs less than equal LID size
                        ( grbit & JET_bitSetIntrinsicLV ) ?
                            ( CbLVIntrinsicTableMost( pfucb ) ) :       // user wants intrinsic LV, but we will not try if over the largest LV we could fit
                        //  else default from spacehints
                        ( UlBound( CbPreferredIntrinsicLVMost( pfucb->u.pfcb ), LvId::CbLidFromCurrFormat( pfucb ), CbLVIntrinsicTableMost( pfucb ) ) );
    
    //  save SetRevertToDefaultValue, then strip off flag
    const BOOL  fRevertToDefault        = ( grbit & JET_bitSetRevertToDefaultValue );
    const BOOL  fUseContiguousSpace     = ( grbit & JET_bitSetContiguousLV );

    //  stip off grbits no longer recognised by this function
    //  note: unique multivalues are checked in ErrFLDSetOneColumn()
    grbit &= ~( JET_bitSetSeparateLV
                | JET_bitSetUniqueMultiValues
                | JET_bitSetUniqueNormalizedMultiValues
                | JET_bitSetRevertToDefaultValue
                | JET_bitSetIntrinsicLV
                | JET_bitSetCompressed
                | JET_bitSetUncompressed
                | JET_bitSetContiguousLV );

    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );

    //  if we are setting size only, pv may be NULL with non-zero cb
#ifdef DEBUG
    const BOOL  fNullPvDataAllowed =
                    ( grbit == JET_bitSetSizeLV
                    || grbit == ( JET_bitSetSizeLV|JET_bitSetZeroLength ) );
    pdataField->AssertValid( fNullPvDataAllowed );
#endif

    Assert( 0 == grbit
            || JET_bitSetAppendLV == grbit
            || JET_bitSetOverwriteLV == grbit
            || JET_bitSetSizeLV == grbit
            || JET_bitSetZeroLength == grbit
            || ( JET_bitSetSizeLV | JET_bitSetZeroLength ) == grbit
            || ( JET_bitSetOverwriteLV | JET_bitSetSizeLV ) == grbit );

    AssertSz( pfucb->ppib->Level() > 0, "LV update attempted at trx level 0" );
    if ( 0 == pfucb->ppib->Level() )
    {
        err = ErrERRCheck( JET_errNotInTransaction );
        return err;
    }

    Assert( FCOLUMNIDTagged( columnid ) );
    FUCBSetColumnSet( pfucb, FidOfColumnid( columnid ) );

    CallR( ErrDIRBeginTransaction( pfucb->ppib, 48933, NO_GRBIT ) );

    //  sequence == 0 means that new field instance is to be set, otherwise retrieve the existing data
    BOOL    fModifyExistingSLong    = fFalse;
    BOOL    fNewInstance            = ( 0 == itagSequence );

    // Encrypted columns can only set one value, so itag should either be 1 or there shouldn't be an existing value
    if ( fEncrypted &&
         ( itagSequence > 1 ||
           fNewInstance && UlRECICountTaggedColumnInstances( pfucb->u.pfcb, columnid, pfucb->dataWorkBuf ) >= 1 ) )
    {
        Error( ErrERRCheck( JET_errEncryptionBadItag ) );
    }

    if ( !fNewInstance )
    {
        Call( ErrRECIRetrieveTaggedColumn(
                    pfucb->u.pfcb,
                    columnid,
                    itagSequence,
                    pfucb->dataWorkBuf,
                    &dataRetrieved,
                    grbitRetrieveColumnDDLNotLocked ) );
        Assert( wrnRECLongField != err );

        if ( fEncrypted &&
             ( err == wrnRECCompressed || err == wrnRECIntrinsicLV ) &&
             dataRetrieved.Cb() > 0 )
        {
            // Be careful not to overwrite err in block below

            pbDataDecrypted = new BYTE[ dataRetrieved.Cb() ];
            if ( pbDataDecrypted == NULL )
            {
                Error( ErrERRCheck( JET_errOutOfMemory ) );
            }
            ULONG cbDataDecryptedActual = dataRetrieved.Cb();
            ERR errT = ErrOSUDecrypt(
                        (BYTE*)dataRetrieved.Pv(),
                        pbDataDecrypted,
                        &cbDataDecryptedActual,
                        pfucb );
            if ( errT < JET_errSuccess )
            {
                Call( errT );
            }
            dataRetrieved.SetPv( pbDataDecrypted );
            dataRetrieved.SetCb( cbDataDecryptedActual );
        }

        switch ( err )
        {
            case wrnRECSeparatedLV:
                Assert( !fNewInstance );
                FUCBSetUpdateSeparateLV( pfucb );
                fModifyExistingSLong = fTrue;
                break;
            case wrnRECCompressed:
            {
                Assert( !fNewInstance );
                Assert( NULL == pbDecompressed );
                INT cbActual;
                Call( ErrPKAllocAndDecompressData(
                        dataRetrieved,
                        pfucb,
                        &pbDecompressed,
                        &cbActual ) );

                dataRetrieved.SetPv( pbDecompressed );
                dataRetrieved.SetCb( cbActual );
            }
                break;
            case wrnRECIntrinsicLV:
                Assert( !fNewInstance );
                break;
            default:
                Assert( JET_wrnColumnNull == err
                    || wrnRECUserDefinedDefault == err );
                Assert( 0 == dataRetrieved.Cb() );
                fNewInstance = fTrue;
        }
    }

    LVOP    lvop;
    Call( ErrLVOpFromGrbit(
                grbit,
                pdataField->Cb(),
                fNewInstance,
                ibLongValue,
                &lvop ) );

    // Using the JET_bitSetContiguousLV is only legal on first set, not replace or null out type operations.
    if ( fUseContiguousSpace && lvop != lvopInsert )
    {
        if ( !FNegTest( fInvalidAPIUsage ) )
        {
            FireWall( "ContiguousLvOnBadLvOp" );
        }
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( fModifyExistingSLong )
    {
        if ( lvopReplace == lvop
            || lvopReplaceWithNull == lvop
            || lvopReplaceWithZeroLength == lvop )
        {
            //  we will be replacing an existing SLong in its entirety,
            //  so must first deref existing SLong.  If WriteConflict,
            //  it means that someone is already replacing over this
            //  column, so just return the error.
            Assert( FFUCBUpdateSeparateLV( pfucb ) );
            LvId lidT = LidOfSeparatedLV( dataRetrieved );
            Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVDereference ) );
            Assert( JET_wrnCopyLongValue != err );

            //  the call to ErrRECAffectSeparateLV() above will initialise
            //  the LV FCB if it hasn't already been inited
            //
            Assert( pfcbNil != pfucb->u.pfcb->Ptdb()->PfcbLV() );
            PERFOpt( PERFIncCounterTable( cLVUpdates, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Ptdb()->PfcbLV()->TCE() ) );
            
            //  separated LV is being replaced entirely, so reset modify flag
            fModifyExistingSLong = fFalse;
        }
    }

    switch ( lvop )
    {
        case lvopNull:
            err = JET_errSuccess;
            goto Commit;

        case lvopInsertNull:
        case lvopReplaceWithNull:
            Assert( ( lvopInsertNull == lvop && fNewInstance )
                || ( lvopReplaceWithNull == lvop && !fNewInstance ) );
            Assert( !fModifyExistingSLong );
            Call( ErrRECSetColumn(
                        pfucb,
                        columnid,
                        itagSequence,
                        NULL,
                        ( fRevertToDefault ? JET_bitSetRevertToDefaultValue : NO_GRBIT ) ) );
            goto Commit;

        case lvopInsertZeroLength:
        case lvopReplaceWithZeroLength:
        {
            Assert( ( lvopInsertZeroLength == lvop && fNewInstance )
                || ( lvopReplaceWithZeroLength == lvop && !fNewInstance ) );
            Assert( !fModifyExistingSLong );

            DATA    dataT;
            dataT.SetPv( NULL );
            dataT.SetCb( 0 );
            Call( ErrRECSetColumn( pfucb, columnid, itagSequence, &dataT ) );
            goto Commit;
        }

        case lvopInsert:
        case lvopInsertZeroedOut:
        case lvopReplace:
            if ( lvopReplace == lvop )
            {
                Assert( !fNewInstance );
                ASSERT_VALID( &dataRetrieved );
            }
            else
            {
                Assert( fNewInstance );
            }
            Assert( !fModifyExistingSLong );
            dataRetrieved.Nullify();
            break;

        case lvopAppend:
        case lvopResize:
        case lvopOverwriteRange:
        case lvopOverwriteRangeAndResize:
            Assert( !fNewInstance );
            ASSERT_VALID( &dataRetrieved );
            break;

        default:
            Assert( fFalse );
            break;
    }

    // All null/zero-length cases should have been handled above.
    Assert( pdataField->Cb() > 0 );


    if ( fModifyExistingSLong )
    {
        Assert( lvopAppend == lvop
            || lvopResize == lvop
            || lvopOverwriteRange == lvop
            || lvopOverwriteRangeAndResize == lvop );

        // Flag should have gotten set when column was retrieved above.
        Assert( FFUCBUpdateSeparateLV( pfucb ) );

        LvId lidT = LidOfSeparatedLV( dataRetrieved );
        Call( ErrRECAOSeparateLV( pfucb, &lidT, pdataField, compressFlags, fEncrypted, fFalse, ibLongValue, ulColMax, lvop ) );
        if ( JET_wrnCopyLongValue == err )
        {
            Call( ErrRECISetLid( pfucb, columnid, itagSequence, lidT ) );
        }

        //  the call to ErrRECAOSeparateLV() above will initialise
        //  the LV FCB if it hasn't already been inited
        //
        Assert( pfcbNil != pfucb->u.pfcb->Ptdb()->PfcbLV() );
        PERFOpt( PERFIncCounterTable( cLVUpdates, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Ptdb()->PfcbLV()->TCE() ) );
    }
    else
    {
        //  determine space requirements of operation to see if we can
        //  make the LV intrinsic
        //  note that long field flag is included in length thereby limiting
        //  intrinsic long field to cbLVIntrinsicMost - sizeof(BYTE)
        ULONG       cbIntrinsic         = pdataField->Cb();

        Assert( fNewInstance || (ULONG)dataRetrieved.Cb() <= CbLVIntrinsicTableMost( pfucb ) );

        switch ( lvop )
        {
            case lvopInsert:
            case lvopInsertZeroedOut:
                Assert( dataRetrieved.FNull() );
            case lvopReplace:   //lint !e616
            case lvopResize:
                break;

            case lvopOverwriteRange:
                cbIntrinsic += ibLongValue;
                if ( (ULONG)dataRetrieved.Cb() > cbIntrinsic )
                    cbIntrinsic = dataRetrieved.Cb();
                break;

            case lvopOverwriteRangeAndResize:
                cbIntrinsic += ibLongValue;
                break;

            case lvopAppend:
                cbIntrinsic += dataRetrieved.Cb();
                break;

            case lvopNull:
            case lvopInsertNull:
            case lvopInsertZeroLength:
            case lvopReplaceWithNull:
            case lvopReplaceWithZeroLength:
            default:
                Assert( fFalse );
                break;
        }

        const ULONG cbLidFromEfv = LvId::CbLidFromCurrFormat( pfucb );
        const ULONG cbIntrinsicPhysical = fEncrypted ? CbOSEncryptAes256SizeNeeded( cbIntrinsic ) : cbIntrinsic;

        Assert( cbLidFromEfv <= cbPreferredIntrinsicLV );
        Assert( cbPreferredIntrinsicLV <= CbLVIntrinsicTableMost( pfucb ) );
        Assert( UlBound( cbPreferredIntrinsicLV, cbLidFromEfv, CbLVIntrinsicTableMost( pfucb ) ) == cbPreferredIntrinsicLV ); // another way.
        BOOL    fForceSeparateLV = ( cbIntrinsicPhysical > cbPreferredIntrinsicLV );

        if ( fForceSeparateLV )
        {
            if ( ulColMax > 0 && cbIntrinsic > ulColMax )
            {
                Error( ErrERRCheck( JET_errColumnTooBig ) );
            }
            PERFOpt( PERFIncCounterTable( cRECAddSeparateLV, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
        }
        else
        {
            err = ErrRECAOIntrinsicLV(
                        pfucb,
                        columnid,
                        itagSequence,
                        &dataRetrieved,
                        pdataField,
                        ibLongValue,
                        ulColMax,
                        lvop,
                        compressFlags,
                        fEncrypted );

            if ( JET_errRecordTooBig == err )
            {
                if ( cbThreshold == 0 )
                {
                    cbThreshold = cbLidFromEfv;
                }

                //  on RecordTooBig, try bursting to LV tree if value is sufficiently large
                Assert( cbThreshold >= cbLidFromEfv );
                if ( cbIntrinsicPhysical > cbThreshold )
                {
                    fForceSeparateLV = fTrue;
                    err = JET_errSuccess;

                    PERFOpt( PERFIncCounterTable( cRECAddForcedSeparateLV, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
                }
            }
            
            Call( err );
            PERFOpt( PERFIncCounterTable( cRECUpdateIntrinsicLV, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
        }

        if ( fForceSeparateLV )
        {
            LvId    lidT;

            // Flag may not have gotten set if this is a new instance.
            FUCBSetUpdateSeparateLV( pfucb );

            if( lvopInsert == lvop
                || lvopReplace == lvop
                || lvopAppend == lvop
                || lvopOverwriteRangeAndResize == lvop )
            {
                // these operations can be done more efficently by ErrRECISeparateLV
                Call( ErrRECISeparateLV( pfucb, dataRetrieved, *pdataField, compressFlags, fEncrypted, fUseContiguousSpace, ibLongValue, &lidT, ulColMax, lvop ) );
            }
            else
            {
                Call( ErrRECSeparateLV( pfucb, &dataRetrieved, compressFlags, fEncrypted, &lidT, NULL ) );
                Assert( JET_wrnCopyLongValue == err );
                Call( ErrRECAOSeparateLV( pfucb, &lidT, pdataField, compressFlags, fEncrypted, fFalse, ibLongValue, ulColMax, lvop ) );
                Assert( JET_wrnCopyLongValue != err );
            }

            Call( ErrRECISetLid( pfucb, columnid, itagSequence, lidT ) );
        }
    }

Commit:
    if( pbDecompressed )
    {
        delete[] pbDecompressed;
        pbDecompressed = NULL;
    }

    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }

    Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    return err;

HandleError:
    Assert( err < 0 );
    if( pbDecompressed )
    {
        delete[] pbDecompressed;
        pbDecompressed = NULL;
    }

    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }

    CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
    return err;
}


//  ================================================================
LOCAL ERR ErrRECIBurstSeparateLV( FUCB * pfucbTable, FUCB * pfucbSrc, LvId * plid )
//  ================================================================
//
//  Makes a new copy of an existing long value. If we are successful pfucbSrc
//  will point to the root of the new long value. We cannot have two cursors
//  open on the table at the same time (deadlock if they try to get the same
//  page), so we use a temp buffer.
//
//  On return pfucbSrc points to the root of the new long value
//
//-
{
    ASSERT_VALID( pfucbTable );
    Assert( !FAssertLVFUCB( pfucbTable ) );
    ASSERT_VALID( pfucbSrc );
    Assert( FAssertLVFUCB( pfucbSrc ) );
    Assert( NULL != plid );
    Assert( *plid > lidMin );

    ERR         err         = JET_errSuccess;
    FUCB        *pfucbDest  = pfucbNil;
    LvId        lid         = lidMin;
    ULONG       ulOffset    = 0;
    LVROOT2     lvroot = { 0 };
    DATA        data;
    BYTE        *pvAlloc    = NULL;
    BOOL        fLatchedSrc = fFalse;
    LVKEY_BUFFER
                lvkey;
    KEY         key;

    //  get long value length
    AssertLVRootNode( pfucbSrc, *plid );
#pragma warning(suppress: 26015)
    UtilMemCpy( &lvroot, pfucbSrc->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbSrc->kdfCurr.data.Cb() ) );

    AllocR( pvAlloc = PbPKAllocCompressionBuffer() );
    data.SetPv( pvAlloc );

    PERFOpt( PERFIncCounterTable( cLVCopies, PinstFromPfucb( pfucbSrc ), TceFromFUCB( pfucbSrc ) ) );

    //  if we have data in the LV copy it all

    if ( lvroot.ulSize > 0 )
    {
        //  move source cursor to first chunk. remember its length
        Call( ErrDIRDownLVData( pfucbSrc, *plid, ulLVOffsetFirst, fDIRNull ) );

        //  assert that no chunks are too large
        const LONG cbLVChunkMost = pfucbTable->u.pfcb->Ptdb()->CbLVChunkMost();
        Assert( pfucbSrc->kdfCurr.data.Cb() <= (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) );
        if ( pfucbSrc->kdfCurr.data.Cb() > (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) )
        {

            LVReportAndTrapCorruptedLV( pfucbSrc, *plid, L"2dff4d03-0a99-4141-aeef-fe408236bce1" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        //  make separate long value root, and insert first chunk
        UtilMemCpy( pvAlloc, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
        Assert( data.Pv() == pvAlloc );
        data.SetCb( pfucbSrc->kdfCurr.data.Cb() );
        CallS( ErrDIRRelease( pfucbSrc ) );

        // we just want to preserve the compression status of the LV so we just copy the compressed
        // data. the size in the LVROOT will be corrected at the end of the copy
        // Make copy of flags on LVROOT to the new one
        LVROOT2 lvrootT;
        lvrootT.ulReference = 1;
        lvrootT.ulSize = 0;
        lvrootT.fFlags = lvroot.fFlags;
        DATA dataNull;
        dataNull.Nullify();
        Call( ErrRECSeparateLV(
                    pfucbTable,
                    &dataNull,
                    compressNone,
                    fFalse, // if encrypted column, data is already encrypted.
                    &lid,
                    &pfucbDest,
                    &lvrootT ) );
        Assert( lid.FLidObeysCurrFormat( pfucbTable ) );

        LVKeyFromLidOffset( &lvkey, &key, lid, ulOffset );

        ulOffset += cbLVChunkMost;

        PERFOpt( PERFIncCounterTable( cLVChunkCopies, PinstFromPfucb( pfucbSrc ), TceFromFUCB( pfucbSrc ) ) );

        Call( ErrDIRInsert( pfucbDest, key, data, fDIRBackToFather ) );

        //  release one cursor, restore another
        DIRUp( pfucbDest );

        //  copy remaining chunks of long value.
        //  if the data is compressed don't bother decompressing and recompressing,
        //  just copy the compressed data
        while ( ( err = ErrDIRNext( pfucbSrc, fDIRSameLIDOnly ) ) >= JET_errSuccess )
        {
            fLatchedSrc = fTrue;

            //  make sure we are still on the same long value
            Call( ErrLVCheckDataNodeOfLid( pfucbSrc, *plid ) );
            if( wrnLVNoMoreData == err )
            {
                //  ErrDIRNext should have returned JET_errNoCurrentRecord
                Assert( fFalse );
                Error( ErrERRCheck( JET_errInternalError ) );
            }

            Assert( pfucbSrc->kdfCurr.data.Cb() <= (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) );
            if ( pfucbSrc->kdfCurr.data.Cb() > (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) )
            {

                LVReportAndTrapCorruptedLV( pfucbSrc, *plid, L"0d41f61b-1957-4f39-9c99-42b4c64634dc" );
                Error( ErrERRCheck( JET_errLVCorrupted ) );
            }

            //  cache the data and insert it into pfucbDest
            //  OPTIMIZATION:  if ulOffset > 1 page we can keep both cursors open
            //  and insert directly from one to the other
            Assert( ulOffset % cbLVChunkMost == 0 );
            LVKeyFromLidOffset( &lvkey, &key, lid, ulOffset );
            UtilMemCpy( pvAlloc, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
            Assert( data.Pv() == pvAlloc );
            data.SetCb( pfucbSrc->kdfCurr.data.Cb() ) ;

            // Determine offset of next chunk.
            ulOffset += cbLVChunkMost;

            Call( ErrDIRRelease( pfucbSrc ) );
            fLatchedSrc = fFalse;

            PERFOpt( PERFIncCounterTable( cLVChunkCopies, PinstFromPfucb( pfucbSrc ), TceFromFUCB( pfucbSrc ) ) );

            Call( ErrDIRInsert( pfucbDest, key, data, fDIRBackToFather ) );

            //  release one cursor, restore the other
            DIRUp( pfucbDest );
        }

        if ( JET_errNoCurrentRecord != err )
        {
            Call( err );
        }
    }

    //  move cursor to new long value
    err = ErrDIRDownLVRoot( pfucbSrc, lid, fDIRNull );
    Assert( JET_errWriteConflict != err );
    Call( err );
    CallS( err );

    //  update lvroot.ulSize to correct long value size.
    data.SetPv( &lvroot );
    data.SetCb( lvroot.fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );
    lvroot.ulReference = 1;
    Call( ErrDIRReplace( pfucbSrc, data, fDIRNull ) );
    Call( ErrDIRGet( pfucbSrc ) );      // Recache

    //  set warning and new long value id for return.
    err     = ErrERRCheck( JET_wrnCopyLongValue );
    *plid   = lid;

HandleError:
    if ( fLatchedSrc )
    {
        Assert( err < 0 );
        CallS( ErrDIRRelease( pfucbSrc ) );
    }
    if ( pfucbNil != pfucbDest )
    {
        DIRClose( pfucbDest );
    }

    // The first thing we do is allocate a temporary buffer, so
    // we should never get here without having a buffer to free.
    Assert( NULL != pvAlloc );
    PKFreeCompressionBuffer( pvAlloc );

    return err;
}

INLINE ERR ErrLVAppendChunks(
    FUCB                *pfucbLV,
    LvId                lid,
    ULONG               ulSize,
    BYTE                *pbAppend,
    const ULONG         cbAppend,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    const BOOL          fContiguousLv )
{
    ERR         err             = JET_errSuccess;
    LVKEY_BUFFER
                lvkey;
    KEY         key;
    DATA        data;
    CPG         cpgRequiredReserve = fContiguousLv ? CpgLVIRequired( pfucbLV->u.pfcb->PfcbTable(), cbAppend ) : 0;
    const BYTE  * const pbMax   = pbAppend + cbAppend;

    //  append remaining long value data
    while( pbAppend < pbMax )
    {
        Assert( ulSize % pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() == 0 );
        LVKeyFromLidOffset( &lvkey, &key, lid, ulSize );

        Assert( key.prefix.FNull() );
        Assert( key.suffix.Pv() == &lvkey );

        data.SetPv( pbAppend );
        data.SetCb( min( pbMax - pbAppend, pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() ) );

        PERFOpt( PERFIncCounterTable( cLVChunkAppends, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

        if ( cpgRequiredReserve )
        {
            //  Note: It's the additional reserve, so it's the page we'll allocate for this
            //  LV minus one.
            DIRSetActiveSpaceRequestReserve( pfucbLV, cpgRequiredReserve - 1 );
        }

        err = ErrLVInsert( pfucbLV, key, data, compressFlags, fEncrypted, pfucbLV->pfucbTable, fDIRBackToFather );

        if ( CpgDIRActiveSpaceRequestReserve( pfucbLV ) == cpgDIRReserveConsumed )
        {
            //  yay, we allocated contiguous pages for the LV.  Turn off computations of LV reserve required.
            DIRSetActiveSpaceRequestReserve( pfucbLV, 0 );
            cpgRequiredReserve = 0;
        }

        Call( err );

        ulSize += data.Cb();

        Assert( pbAppend + data.Cb() <= pbMax );
        pbAppend += data.Cb();

        //  If we didn't manage to consume our cpgRequiredReserve, then we need to reduce it to what remains
        //  in the LV to avoid a potential page of over allocation.
        if ( cpgRequiredReserve )
        {
            cpgRequiredReserve = CpgLVIRequired( pfucbLV->u.pfcb->PfcbTable(), ULONG( pbMax - pbAppend ) );
        }
    }

HandleError:

    Assert( CpgDIRActiveSpaceRequestReserve( pfucbLV ) != cpgDIRReserveConsumed );

    DIRSetActiveSpaceRequestReserve( pfucbLV, 0 );

    return err;
}

// Note that defrag uses this to copy compressed chunks from one database to another
ERR ErrCMPAppendLVChunk(
    FUCB                *pfucbLV,
    LvId                lid,
    ULONG               ulSize,
    BYTE                *pbAppend,
    const ULONG         cbAppend )
{
    LVKEY_BUFFER    lvkey;
    KEY             key;
    DATA            data;

    // Current size a chunk multiple.
    Assert( ulSize % pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() == 0 );

    LVKeyFromLidOffset( &lvkey, &key, lid, ulSize );

    data.SetPv( pbAppend );
    data.SetCb( cbAppend );

    PERFOpt( PERFIncCounterTable( cLVChunkAppends, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    return ErrDIRInsert( pfucbLV, key, data, fDIRBackToFather );
}


LOCAL ERR ErrLVAppend(
    FUCB                *pfucbLV,
    LvId                lid,
    ULONG               ulSize,         // offset to start appending at
    const DATA          *pdataAppend,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    const BOOL          fContiguousLv,
    VOID                *pvbf )
{
    ERR         err             = JET_errSuccess;
    BYTE        *pbAppend       = reinterpret_cast<BYTE *>( pdataAppend->Pv() );
    ULONG       cbAppend        = pdataAppend->Cb();
    
    if ( 0 == cbAppend )
        return JET_errSuccess;

    //  APPEND long value
    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    if ( ulSize > 0 )
    {
        const ULONG ulOffsetLast = ( ( ulSize - 1 ) / cbLVChunkMost ) * cbLVChunkMost;
        CallR( ErrDIRDownLVData( pfucbLV, lid, ulOffsetLast, fDIRFavourPrev ) );

        ULONG cbUsedInChunk = g_cbPage;
        CallR( ErrLVRetrieve(
                pfucbLV,
                lid,
                ulSize,
                pfucbLV->kdfCurr.key,
                pfucbLV->kdfCurr.data,
                fEncrypted,
                0,
                (BYTE *)pvbf,
                &cbUsedInChunk ) );
        Assert( cbUsedInChunk <= (ULONG)cbLVChunkMost );
        if ( cbUsedInChunk < (ULONG)cbLVChunkMost )
        {
            DATA        data;
            const INT   cbAppendToChunk = min( cbAppend, cbLVChunkMost - cbUsedInChunk );

            UtilMemCpy( (BYTE *)pvbf + cbUsedInChunk, pbAppend, cbAppendToChunk );

            pbAppend += cbAppendToChunk;
            cbAppend -= cbAppendToChunk;
            ulSize += cbAppendToChunk;

            Assert( cbUsedInChunk + cbAppendToChunk <= (ULONG)cbLVChunkMost );

            PERFOpt( PERFIncCounterTable( cLVChunkReplaces, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

            data.SetPv( pvbf );
            data.SetCb( cbUsedInChunk + cbAppendToChunk );
            CallR( ErrLVReplace( pfucbLV, data, compressFlags, fEncrypted, fDIRNull ) );
        }
    }
    else
    {
        Assert( 0 == ulSize );

        //  the LV is size 0 and has a root only
        CallR( ErrDIRDownLVRoot( pfucbLV, lid, fDIRNull ) );
    }

    if ( cbAppend > 0 )
    {
        Assert( ulSize % cbLVChunkMost == 0 );
        DIRUp( pfucbLV );
        CallR( ErrLVAppendChunks( pfucbLV, lid, ulSize, pbAppend, cbAppend, compressFlags, fEncrypted, fContiguousLv ) );
    }

    return err;
}


LOCAL ERR ErrLVAppendZeroedOutChunks(
    FUCB                *pfucbLV,
    LvId                lid,
    ULONG               ulSize,
    ULONG               cbAppend,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    VOID                *pvbf )
{
    ERR             err     = JET_errSuccess;
    LVKEY_BUFFER    lvkey;
    KEY             key;
    DATA            data;

    DIRUp( pfucbLV );

    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    memset( pvbf, 0, cbLVChunkMost );

    Assert( NULL != pvbf );
    data.SetPv( pvbf );

    //  append remaining long value data
    while( cbAppend > 0 )
    {
        Assert( ulSize % cbLVChunkMost == 0 );
        LVKeyFromLidOffset( &lvkey, &key, lid, ulSize );

        Assert( key.prefix.FNull() );
        Assert( key.suffix.Pv() == &lvkey );

        Assert( data.Pv() == pvbf );
        data.SetCb( min( cbAppend, (SIZE_T)cbLVChunkMost ) );

        PERFOpt( PERFIncCounterTable( cLVChunkAppends, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

        // CONSIDER: compressing the zeroed out chunks may cause splits when the data is later overwritten
        CallR( ErrLVInsert( pfucbLV, key, data, compressFlags, fEncrypted, pfucbLV->pfucbTable, fDIRBackToFather ) );

        ulSize += data.Cb();

        Assert( cbAppend >= (ULONG)data.Cb() );
        cbAppend -= data.Cb();
    }

    return err;
}


LOCAL ERR ErrLVTruncate(
    FUCB                *pfucbLV,
    LvId                lid,
    const ULONG         ulLVSizeCurr,
    const ULONG         ulTruncatedSize,
    VOID                *pvbf,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted )
{
    ERR         err;
    const ULONG ulOffset = ulTruncatedSize;
    ULONG       ulOffsetChunk;
    DATA        data;

    //  seek to offset to begin deleting
    CallR( ErrDIRDownLVData( pfucbLV, lid, ulOffset, fDIRNull ) );

    //  get offset of last byte in current chunk
    //  replace current chunk with remaining data, or delete if
    //  no remaining data.
    OffsetFromKey( &ulOffsetChunk, pfucbLV->kdfCurr.key );
    Assert( ulOffset >= ulOffsetChunk );
    data.SetCb( ulOffset - ulOffsetChunk );
    if ( data.Cb() > 0 )
    {
        PERFOpt( PERFIncCounterTable( cLVChunkReplaces, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
        
        data.SetPv( pvbf );
        INT cbRetrieve = data.Cb();
        CallR( ErrLVRetrieve(
            pfucbLV,
            lid,
            ulLVSizeCurr,
            pfucbLV->kdfCurr.key,
            pfucbLV->kdfCurr.data,
            fEncrypted,
            0,
            (BYTE *)data.Pv(),
            (ULONG *)&cbRetrieve ) );
        Assert( cbRetrieve == data.Cb() );
        CallR( ErrLVReplace(
                pfucbLV,
                data,
                compressFlags,
                fEncrypted,
                fDIRLogChunkDiffs ) );
    }
    else
    {
        PERFOpt( PERFIncCounterTable( cLVChunkDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
        CallR( ErrDIRDelete( pfucbLV, fDIRNull ) );
    }

    //  delete forward chunks
    while ( ( err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly ) ) >= JET_errSuccess )
    {
        //  make sure we are still on the same long value
        err = ErrLVCheckDataNodeOfLid( pfucbLV, lid );
        if ( JET_errSuccess != err )
        {
            CallS( ErrDIRRelease( pfucbLV ) );
            //  ErrDIRNext should have returned JET_errNoCurrentRecord
            Assert( wrnLVNoMoreData != err );
            return ( wrnLVNoMoreData == err ? ErrERRCheck( JET_errInternalError ) : err );
        }

        PERFOpt( PERFIncCounterTable( cLVChunkDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
        CallR( ErrDIRDelete( pfucbLV, fDIRNull ) );
    }

    if ( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

    return err;
}


INLINE ERR ErrLVOverwriteRange(
    FUCB                *pfucbLV,
    LvId                lid,
    const ULONG         ulLVSize,
    ULONG               ibLongValue,
    const DATA          *pdataField,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    VOID                *pvbf )
{
    ERR         err;
    DATA        data;
    BYTE        *pb             = reinterpret_cast< BYTE *>( pdataField->Pv() );
    const BYTE  * const pbMax   = pb + pdataField->Cb();

    //  OVERWRITE long value. seek to offset to begin overwritting
    CallR( ErrDIRDownLVData( pfucbLV, lid, ibLongValue, fDIRNull ) );

#ifdef DEBUG
    ULONG   cPartialChunkOverwrite  = 0;
#endif

    ULONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();

    //  overwrite portions of and complete chunks to effect overwrite
    for ( ; ; )
    {
        //  get size and offset of current chunk.
        ULONG   ibChunk;        //  the index of the current chunk
        ULONG   cb = 0;

        OffsetFromKey( &ibChunk, pfucbLV->kdfCurr.key );
        Assert( ibLongValue >= ibChunk );
        Assert( ibLongValue < ibChunk + cbLVChunkMost );

        //  special case overwrite of whole chunk
        if ( ibChunk == ibLongValue &&
             ULONG( pbMax - pb ) >= cbLVChunkMost )
        {
            // Start overwriting at the beginning of a chunk.
            cb = cbLVChunkMost;
            data.SetCb( cb );
            data.SetPv( pb );
        }
        else
        {
#ifdef DEBUG
            // Should only do partial chunks for the first and last chunks.
            cPartialChunkOverwrite++;
            Assert( cPartialChunkOverwrite <= 2 );
#endif
            const ULONG ib = ibLongValue - ibChunk;
            ULONG   cbChunk = g_cbPage;     //  the size of the current chunk
            CallR( ErrLVRetrieve(
                pfucbLV,
                lid,
                ulLVSize,
                pfucbLV->kdfCurr.key,
                pfucbLV->kdfCurr.data,
                fEncrypted,
                0,
                (BYTE *)pvbf,
                &cbChunk ) );
            Assert( ibLongValue < ibChunk + cbChunk );
            Assert( ib < cbChunk );
            cb = min( cbChunk - ib, ULONG( pbMax - pb ) );
            Assert( cb <= cbChunk );
            
            UtilMemCpy( reinterpret_cast<BYTE *>( pvbf ) + ib, pb, cb );
            data.SetCb( cbChunk );
            data.SetPv( pvbf );
        }
        CallR( ErrLVReplace( pfucbLV, data, compressFlags, fEncrypted, fDIRLogChunkDiffs ) );

        pb += cb;
        ibLongValue += cb;

        //  we mey have written the entire long value
        //  note: pb should never exceed pbMax, but handle it just in case
        //
        Assert( pb <= pbMax );
        if ( pb >= pbMax )
        {
            return JET_errSuccess;
        }

        //  goto the next chunk
        err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly );
        if ( err < 0 )
        {
            if ( JET_errNoCurrentRecord == err )
                break;
            else
                return err;
        }

        //  make sure we are still on the same long value
        err = ErrLVCheckDataNodeOfLid( pfucbLV, lid );
        if ( err < 0 )
        {
            CallS( ErrDIRRelease( pfucbLV ) );
            return err;
        }
        else if ( wrnLVNoMoreData == err )
        {
            //  ErrDIRNext should have returned JET_errNoCurrentRecord
            return ErrERRCheck( JET_errInternalError );
        }

        //  All overwrites beyond the first should happen at the beginning
        //  of a chunk.
        Assert( ibLongValue % cbLVChunkMost == 0 );
    }

    // If we got here, we ran out of stuff to overwrite before we ran out
    // of new data.  Append the new data.
    Assert( pb < pbMax );
    data.SetPv( pb );
    data.SetCb( pbMax - pb );
    err = ErrLVAppend(
                pfucbLV,
                lid,
                ibLongValue,
                &data,
                compressFlags,
                fEncrypted,
                fFalse,
                pvbf );

    return err;
}

//  ================================================================
ERR ErrRECAOSeparateLV(
    FUCB                *pfucb,
    LvId                *plid,
    const DATA          *pdataField,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted,
    const BOOL          fContiguousLv,
    const ULONG         ibLongValue,
    const ULONG         ulColMax,
    const LVOP          lvop )
//  ================================================================
//
//  Appends, overwrites and sets length of separate long value data.
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( plid );
    Assert( pfucb->ppib->Level() > 0 );

    // Null and zero-length are handled by RECSetLongField
    Assert( lvopInsert == lvop
            || lvopInsertZeroedOut == lvop
            || lvopReplace == lvop
            || lvopAppend == lvop
            || lvopResize == lvop
            || lvopOverwriteRange == lvop
            || lvopOverwriteRangeAndResize == lvop );

#ifdef DEBUG
    //  if we are setting size, pv may be NULL with non-zero cb
    pdataField->AssertValid( lvopInsertZeroedOut == lvop || lvopResize == lvop );
#endif  //  DEBUG

    // NULL and zero-length cases are handled by ErrRECSetLongField().
    Assert( pdataField->Cb() > 0 );

    ERR         err             = JET_errSuccess;
    ERR         wrn             = JET_errSuccess;
    FUCB        *pfucbLV        = pfucbNil;
    VOID        *pvbf           = NULL;
    LVROOT2     lvroot = { 0 };
    DATA        data;
    data.SetPv( &lvroot );
    ULONG       ulVerRefcount;
    LvId        lidCurr         = *plid;

    //  open cursor on LONG directory
    //  seek to this field instance
    //  find current field size
    //  add new field segment in chunks no larger than max chunk size
    CallR( ErrDIROpenLongRoot( pfucb ) );
    Assert( wrnLVNoLongValues != err );

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    //  move to start of long field instance
    Call( ErrDIRDownLVRoot( pfucbLV, *plid, fDIRNull ) );
#pragma warning(suppress: 26015)
    UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );

    // Only encrypted LVs should use LVROOT2
    Assert( pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT) || FLVEncrypted( lvroot.fFlags ) );
    // Encryption state of LV better match what we think it should be.
    Assert( !fEncrypted == !FLVEncrypted( lvroot.fFlags ) );

    ulVerRefcount = UlLVIVersionedRefcount( pfucbLV );

    // versioned refcount must be non-zero and not partially deleted, otherwise we couldn't see it
    Assert( ulVerRefcount > 0 );
    Assert( !FPartiallyDeletedLV( ulVerRefcount ) );

    //  get offset of last byte from long value size
    ULONG       ulSize;
    ULONG       ulNewSize;
#ifdef DEBUG
    ULONG       ulRefcountDBG;
    ulRefcountDBG   = lvroot.ulReference;
#endif
    ulSize          = lvroot.ulSize;

    if ( ibLongValue > ulSize
        && ( lvopOverwriteRange == lvop || lvopOverwriteRangeAndResize == lvop ) )
    {
        Error( ErrERRCheck( JET_errColumnNoChunk ) );
    }

    //  if we have more than one reference we have to burst the long value.
    //  We also have to burst the LV if the copy buffer was prepared for
    //  InsertCopyDelete/ReplaceOriginal, since only through bursting the LV
    //  can Update get access to the before image, for both index maintenance
    //  including concurrent create index.  
    //
    BOOL fLVBursted = fFalse;

    if ( ulVerRefcount > 1 || FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
    {
        Assert( *plid == lidCurr );
        Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) ); // lid will change
        Assert( JET_wrnCopyLongValue == err );
        Assert( *plid > lidCurr );
        fLVBursted = fTrue;
        wrn = err;
        AssertLVRootNode( pfucbLV, *plid );
#pragma warning(suppress: 26015)
        UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );
        Assert( 1 == lvroot.ulReference );
    }

    Assert( ulSize == lvroot.ulSize );

    switch ( lvop )
    {
        case lvopInsert:
        case lvopInsertZeroedOut:
        case lvopReplace:
            Assert( 0 == ulSize );      // replace = delete old + insert new (thus, new LV currently has size 0)
        case lvopResize:
            ulNewSize = pdataField->Cb();
            break;

        case lvopOverwriteRange:
            ulNewSize = max( ibLongValue + pdataField->Cb(), ulSize );
            break;

        case lvopOverwriteRangeAndResize:
            ulNewSize = ibLongValue + pdataField->Cb();
            break;

        case lvopAppend:
            ulNewSize = ulSize + pdataField->Cb();
            break;

        case lvopNull:
        case lvopInsertNull:
        case lvopInsertZeroLength:
        case lvopReplaceWithNull:
        case lvopReplaceWithZeroLength:
        default:
            Assert( fFalse );
            ulNewSize = ulSize;
            break;
    }

    //  check for field too long
    if ( ( ulColMax > 0 && ulNewSize > ulColMax ) || ulNewSize > lMax )
    {
        Error( ErrERRCheck( JET_errColumnTooBig ) );
    }

    //  if ulVerRefcount was greater than 1, we would have burst above, and the new LV refcount should be 1
    AssertTrack( 1 == ulVerRefcount || ( fLVBursted && 1 == lvroot.ulReference ), "LVRefcountGreaterThan1" );

    if ( lvroot.ulSize  == ulNewSize &&
         lvroot.ulReference == 1 ) // Not strictly needed, just to make the change safer.
    {
        CallS( ErrDIRRelease( pfucbLV ) );
        // If we bursted this LV, the Insert is already protecting this LV root
        if ( !fLVBursted )
        {
            err = ErrDIRGetLock( pfucbLV, writeLock );
        }
    }
    else
    {
        //  replace long value size with new size. This also 'locks' the long value for us.
        lvroot.ulReference = 1;
        lvroot.ulSize = ulNewSize;
        data.SetCb( lvroot.fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );
        Assert( &lvroot == data.Pv() );
        err = ErrDIRReplace( pfucbLV, data, fDIRNull );
    }
    if ( err < 0 )
    {
        if ( JET_errWriteConflict != err )
            goto HandleError;

        //  write conflict means someone else was modifying/deltaing
        //  the long value

        //  if ulVerRefcount was greater than 1, we would have burst
        //  and thus should not have write-conflicted
        Assert( 1 == ulVerRefcount );

        //  we lost the page during the write conflict
        Call( ErrDIRGet( pfucbLV ) );

        if ( FDIRDeltaActiveNotByMe( pfucbLV, OffsetOf( LVROOT2, ulReference ) ) )
        {
            //  we lost our latch and someone else entered the page and did a delta
            //  this should only happen if we didn't burst above
            Assert( JET_wrnCopyLongValue != wrn );
            Assert( *plid == lidCurr );
            Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) ); // lid will change
            Assert( JET_wrnCopyLongValue == err );
            Assert( *plid > lidCurr );
            fLVBursted = fTrue;
            wrn = err;
            AssertLVRootNode( pfucbLV, *plid );
            UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );
            Assert( 1 == lvroot.ulReference );
            if ( lvroot.ulSize  == ulNewSize )
            {
                CallS( ErrDIRRelease( pfucbLV ) );
                // No need for a lock since the insert from above will protect this node
            }
            else
            {
                lvroot.ulSize = ulNewSize;
                data.SetCb( lvroot.fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );
                Assert( &lvroot == data.Pv() );
                Call( ErrDIRReplace( pfucbLV, data, fDIRNull ) );
            }
        }
        else
        {
            // Another thread doing replace or delete on same LV.
            CallS( ErrDIRRelease( pfucbLV ) );

            //  UNDONE: is there a way to easily report the bm?
            //
            OSTraceFMP(
                pfucb->ifmp,
                JET_tracetagDMLConflicts,
                OSFormat(
                    "Write-conflict detected: Session=[0x%p:0x%x] updating LV [lid=0x%I64x] on objid=[0x%x:0x%x]",
                    pfucb->ppib,
                    ( ppibNil != pfucb->ppib ? pfucb->ppib->trxBegin0 : trxMax ),
                    (_LID64)*plid,
                    (ULONG)pfucb->ifmp,
                    pfucb->u.pfcb->ObjidFDP() ) );

            Error( ErrERRCheck( JET_errWriteConflict ) );
        }
    }

    if ( fLVBursted )
    {
        //  bursting was successful - update refcount on original LV.
        //  if this deref subsequently write-conflicts, it means
        //  someone else is already updating this record.
        Assert( !Pcsr( pfucb )->FLatched() );
        Call( ErrRECAffectSeparateLV( pfucb, &lidCurr, fLVDereference ) );
        Assert( JET_wrnCopyLongValue != err );
    }

    Assert( 1 == lvroot.ulReference );

    //  allocate buffer for partial overwrite caching.
    Assert( NULL == pvbf );
    BFAlloc( bfasForDisk, &pvbf );

    switch( lvop )
    {
        case lvopInsert:
        case lvopReplace:
            Assert( 0 == ulSize );
            DIRUp( pfucbLV );
            Call( ErrLVAppendChunks(
                        pfucbLV,
                        *plid,
                        0,
                        reinterpret_cast<BYTE *>( pdataField->Pv() ),
                        pdataField->Cb(),
                        compressFlags,
                        fEncrypted,
                        fContiguousLv ) );
            break;

        case lvopResize:
            //  TRUNCATE long value
            if ( ulNewSize < ulSize )
            {
                Call( ErrLVTruncate( pfucbLV, *plid, ulSize, ulNewSize, pvbf, compressFlags, fEncrypted ) );
            }
            else if ( ulNewSize > ulSize )
            {
                const LONG cbLVChunkMost = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
                if ( ulSize > 0 )
                {
                    //  EXTEND long value with chunks of 0s

                    //  seek to the maximum offset to get the last chunk
                    //  long value chunk tree may be empty
                    const ULONG ulOffsetLast = ( ( ulSize - 1 ) / cbLVChunkMost ) * cbLVChunkMost;
                    Call( ErrDIRDownLVData( pfucbLV, *plid, ulOffsetLast, fDIRFavourPrev ) );
                    AssertLVDataNode( pfucbLV, *plid, ulOffsetLast );

                    ULONG cbUsedInChunk = g_cbPage;
                    Call( ErrLVRetrieve(
                            pfucbLV,
                            *plid,
                            ulSize,
                            pfucbLV->kdfCurr.key,
                            pfucbLV->kdfCurr.data,
                            fEncrypted,
                            0,
                            (BYTE *)pvbf,
                            &cbUsedInChunk ) );
                    Assert( cbUsedInChunk <= (SIZE_T)cbLVChunkMost );
                    if ( cbUsedInChunk < (SIZE_T)cbLVChunkMost )
                    {
                        const ULONG cbAppendToChunk = min(
                                                        ulNewSize - ulSize,
                                                        cbLVChunkMost - cbUsedInChunk );

                        memset( (BYTE *)pvbf + cbUsedInChunk, 0, cbAppendToChunk );

                        ulSize += cbAppendToChunk;

                        Assert( cbUsedInChunk + cbAppendToChunk <= (SIZE_T)cbLVChunkMost );

                        data.SetPv( pvbf );
                        data.SetCb( cbUsedInChunk + cbAppendToChunk );
                        Call( ErrLVReplace( pfucbLV, data, compressFlags, fEncrypted, fDIRLogChunkDiffs ) );
                    }
                    else
                    {
                        Assert( ulSize % cbLVChunkMost == 0 );
                    }

                    Assert( ulSize <= ulNewSize );
                }

                else
                {
                    Call( ErrDIRDownLVRoot( pfucbLV, *plid, fDIRNull ) );

                    //  the LV is size 0 and has a root only. we have landed on the root
                    AssertLVRootNode( pfucbLV, *plid );

                    //  we will be appending zerout-out chunks to this LV
                    Assert( ulSize < ulNewSize );
                }

                if ( ulSize < ulNewSize )
                {
                    Assert( ulSize % cbLVChunkMost == 0 );
                    Call( ErrLVAppendZeroedOutChunks(
                            pfucbLV,
                            *plid,
                            ulSize,
                            ulNewSize - ulSize,
                            compressFlags,
                            fEncrypted,
                            pvbf ) );
                }
            }
            else
            {
                // no size change required
                err = JET_errSuccess;
            }
            break;

        case lvopInsertZeroedOut:
            Assert( 0 == ulSize );
            Call( ErrLVAppendZeroedOutChunks(
                        pfucbLV,
                        *plid,
                        ulSize,
                        pdataField->Cb(),
                        compressFlags,
                        fEncrypted,
                        pvbf ) );
            break;

        case lvopAppend:
            Call( ErrLVAppend(
                        pfucbLV,
                        *plid,
                        ulSize,
                        pdataField,
                        compressFlags,
                        fEncrypted,
                        fContiguousLv,
                        pvbf ) );
            break;

        case lvopOverwriteRangeAndResize:
            if ( ulNewSize < ulSize )
            {
                Call( ErrLVTruncate( pfucbLV, *plid, ulSize, ulNewSize, pvbf, compressFlags, fEncrypted ) );
                ulSize = ulNewSize;
            }
            //  Fall through to do the actual overwrite:

        case lvopOverwriteRange:
            if ( 0 == ulSize )
            {
                //  may hit this case if we're overwriting a zero-length column
                //  that was force-separated
                Assert( 0 == ibLongValue );
                DIRUp( pfucbLV );
                Call( ErrLVAppendChunks(
                            pfucbLV,
                            *plid,
                            0,
                            reinterpret_cast<BYTE *>( pdataField->Pv() ),
                            pdataField->Cb(),
                            compressFlags,
                            fEncrypted,
                            fFalse ) );
            }
            else if ( ibLongValue == ulSize )
            {
                //  pathological case of overwrite starting exactly at the point where
                //  the LV ends - this degenerates to an append
                Call( ErrLVAppend(
                            pfucbLV,
                            *plid,
                            ulSize,
                            pdataField,
                            compressFlags,
                            fEncrypted,
                            fFalse,
                            pvbf ) );
            }
            else
            {
                Call( ErrLVOverwriteRange(
                            pfucbLV,
                            *plid,
                            ulSize,
                            ibLongValue,
                            pdataField,
                            compressFlags,
                            fEncrypted,
                            pvbf ) );
            }
            break;

        case lvopNull:
        case lvopInsertNull:
        case lvopInsertZeroLength:
        case lvopReplaceWithNull:
        case lvopReplaceWithZeroLength:
        default:
            Assert( fFalse );
            break;
    }


#ifdef DEBUG
{
        //  move to start of long field instance
        Call( ErrDIRDownLVRoot( pfucbLV, *plid, fDIRNull ) );
        UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );
        Assert( lvroot.ulSize == ulNewSize );
        Assert( ulNewSize > 0 );

        ULONG ulOffsetLastT;
        ulOffsetLastT = ( ( ulNewSize - 1 ) / pfucb->u.pfcb->Ptdb()->CbLVChunkMost() ) * pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
        Call( ErrDIRDownLVData( pfucbLV, *plid, ulOffsetLastT, fDIRFavourPrev ) );

        ULONG ulOffsetT;
        OffsetFromKey( &ulOffsetT, pfucbLV->kdfCurr.key );
        Assert( ulOffsetLastT == ulOffsetT );

        LvId    lidT;
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        Assert( *plid == lidT );

        ULONG   ibChunk;
        OffsetFromKey( &ibChunk, pfucbLV->kdfCurr.key );

        ULONG cbT;
        CallS( ErrLVIGetDataSize( pfucbLV, pfucbLV->kdfCurr.key, pfucbLV->kdfCurr.data, fEncrypted, ulNewSize, &cbT ) );
        Assert( ulNewSize == ibChunk + cbT );

        err = ErrDIRNext( pfucbLV, fDIRNull );
        if( JET_errNoCurrentRecord == err )
        {
            err = JET_errSuccess;
        }
        else if ( err < 0 )
        {
            goto HandleError;
        }
        else
        {
            LidFromKey( &lidT, pfucbLV->kdfCurr.key );
            Assert( *plid != lidT );
        }
}
#endif


HandleError:
    if ( NULL != pvbf )
    {
        BFFree( pvbf );
    }

    Assert( pfucbNil != pfucbLV );
    DIRCloseLongRoot( pfucb );

    //  return warning if no failure
    err = ( err < JET_errSuccess ) ? err : wrn;
    return err;
}


//  ================================================================
ERR ErrRECAOIntrinsicLV(
    FUCB                *pfucb,
    const COLUMNID      columnid,
    const ULONG         itagSequence,
    const DATA          *pdataColumn,
    const DATA          *pdataNew,
    const ULONG         ibLongValue,
    const ULONG         ulColMax,
    const LVOP          lvop,
    const CompressFlags compressFlags,
    const BOOL          fEncrypted )
//  ================================================================
{
    ASSERT_VALID( pdataColumn );

//  Can't perform this check on the FUCB, because it may be a fake FUCB used
//  solely as a placeholder while building the default record.
/// ASSERT_VALID( pfucb );

    Assert( pdataNew->Cb() > 0 );
    Assert( (ULONG)pdataNew->Cb() <= CbLVIntrinsicTableMost( pfucb ) );
    JET_ERR     err = JET_errSuccess;
    DATA        dataSet, dataOrigSet;
    VOID        *pvbf = NULL;
    VOID        *pvbfCompressed = NULL;
    VOID        *pvbfEncrypted = NULL;
    ULONG       cbT = 0;
    BYTE        *pbT = NULL;
    BYTE        rgb[ cbLVIntrinsicMost ];
    BYTE        rgbCompressed[ sizeof(rgb) ];
    BYTE        rgbEncrypted[ sizeof(rgb) ];
    JET_GRBIT   grbit = 0;
    
    switch ( lvop )
    {
        case lvopInsert:
        case lvopReplace:
        case lvopInsertZeroedOut:
        case lvopResize:
            cbT = pdataNew->Cb();
            break;
        case lvopOverwriteRange:
            Assert( (ULONG)pdataColumn->Cb() <= CbLVIntrinsicTableMost( pfucb ) );
            if ( (LONG)ibLongValue > pdataColumn->Cb() )
            {
                return ErrERRCheck( JET_errColumnNoChunk );
            }
            if ( ibLongValue + pdataNew->Cb() < (ULONG)pdataColumn->Cb() )
            {
                cbT = pdataColumn->Cb();
            }
            else
            {
                cbT = ibLongValue + pdataNew->Cb();
            }
            break;

        case lvopOverwriteRangeAndResize:
            Assert( (ULONG)pdataColumn->Cb() <= CbLVIntrinsicTableMost( pfucb ) );
            if ( (LONG)ibLongValue > pdataColumn->Cb() )
            {
                return ErrERRCheck( JET_errColumnNoChunk );
            }
            cbT = ibLongValue + pdataNew->Cb();
            break;

        case lvopAppend:
            cbT = pdataColumn->Cb() + pdataNew->Cb();
            break;
        case lvopNull:
        case lvopInsertNull:
        case lvopInsertZeroLength:
        case lvopReplaceWithNull:
        case lvopReplaceWithZeroLength:
        default:
            Assert( fFalse );
            break;
    }

    //  ErrRECISetTaggedColumn() will eventually
    //  check for ColumnTooBig, but by then it
    //  may be too late because we might have
    //  compressed the data, so must check now
    //
    if ( ulColMax > 0 && cbT > ulColMax )
    {
        return ErrERRCheck( JET_errColumnTooBig );
    }

    if ( cbT <= sizeof(rgb) )
    {
        pbT = rgb;
    }
    else
    {
        if ( cbT > CbLVIntrinsicTableMost( pfucb ) )
        {
            return ErrERRCheck( JET_errDatabaseCorrupted );
        }
        BFAlloc( bfasTemporary, &pvbf );
        pbT = (BYTE *)pvbf;
    }
    dataSet.SetPv( pbT );

    switch ( lvop )
    {
        case lvopInsert:
        case lvopReplace:
        {
            Assert( pdataColumn->FNull() );

            const INT cbDataNew = pdataNew->Cb();

            AssertPREFIX( cbT == (ULONG)cbDataNew );
            UtilMemCpy( pbT, pdataNew->Pv(), cbDataNew );
        }
            break;

        case lvopInsertZeroedOut:
        {
            Assert( pdataColumn->FNull() );

            const INT cbDataNew = pdataNew->Cb();

            AssertPREFIX( cbT == (ULONG)cbDataNew );
            memset( pbT, 0, cbDataNew );
        }
            break;

        case lvopResize:
        {
            const INT cbDataColumn = pdataColumn->Cb();
            const INT cbDataNew = pdataNew->Cb();
            
            Assert( (ULONG)cbDataColumn <= CbLVIntrinsicTableMost( pfucb ) );
            AssertPREFIX( cbT == (ULONG)cbDataNew );
            UtilMemCpy(
                pbT,
                pdataColumn->Pv(),
                min( cbDataNew, cbDataColumn ) );
            if ( cbDataNew > cbDataColumn )
            {
                memset(
                    pbT + cbDataColumn,
                    0,
                    cbDataNew - cbDataColumn );
            }
            Assert( cbT == (ULONG)cbDataNew );
        }
            break;

        case lvopOverwriteRange:
        {
            const INT cbDataColumn = pdataColumn->Cb();
            const INT cbDataNew = pdataNew->Cb();
            
            Assert( (ULONG)cbDataColumn <= CbLVIntrinsicTableMost( pfucb ) );
            Assert( ibLongValue <= (ULONG)cbDataColumn );
            if ( ibLongValue + cbDataNew < (ULONG)cbDataColumn )
            {
                AssertPREFIX( cbT == (ULONG)cbDataColumn );
                UtilMemCpy( pbT, pdataColumn->Pv(), cbDataColumn );
            }
            else
            {
                AssertPREFIX( cbT == ibLongValue + cbDataNew );
                UtilMemCpy( pbT, pdataColumn->Pv(), ibLongValue );
            }
            UtilMemCpy(
                pbT + ibLongValue,
                pdataNew->Pv(),
                cbDataNew );
        }
            break;

        case lvopOverwriteRangeAndResize:
        {
            const INT cbDataColumn = pdataColumn->Cb();
            const INT cbDataNew = pdataNew->Cb();
            
            Assert( (ULONG)cbDataColumn <= CbLVIntrinsicTableMost( pfucb ) );
            AssertPREFIX( ibLongValue <= (ULONG)cbDataColumn );
            AssertPREFIX( cbT == ibLongValue + cbDataNew );
            UtilMemCpy( pbT, pdataColumn->Pv(), ibLongValue );
            UtilMemCpy(
                pbT + ibLongValue,
                pdataNew->Pv(),
                cbDataNew );
        }
            break;

        case lvopAppend:
        {
            const INT cbDataColumn = pdataColumn->Cb();
            const INT cbDataNew = pdataNew->Cb();

            Assert( (ULONG)cbDataColumn <= CbLVIntrinsicTableMost( pfucb ) );
            AssertPREFIX( cbT == ULONG( cbDataColumn + cbDataNew ) );
            UtilMemCpy( pbT, pdataColumn->Pv(), cbDataColumn );
            UtilMemCpy( pbT + cbDataColumn, pdataNew->Pv(), cbDataNew );
        }
            break;

        case lvopNull:
        case lvopInsertNull:
        case lvopInsertZeroLength:
        case lvopReplaceWithNull:
        case lvopReplaceWithZeroLength:
        default:
            Assert( fFalse );
            break;
    }

    //  set size from precomputed value
    //
    Assert( cbT <= CbLVIntrinsicTableMost( pfucb ) );
    Assert( cbT <= cbLVIntrinsicMost || pvbf != NULL );
    dataSet.SetCb( cbT );

    dataOrigSet = dataSet;

    // the record format only supports compressing the first multi-value, so if we know that we are updating
    // itag 2 or greater don't try compression. it is possible that we are inserting a new multi-value (the
    // itag 0 case) but ErrRECSetColumn will return an error in that case and we will store the non-compressed
    // data
    if ( itagSequence < 2 )
    {
        // Compress if the user asked for it and the chi-squared test estimates that we will get good results
        bool fTryCompress = false;
        if ( ( compressNone != compressFlags ) )
        {
            fTryCompress = true;
            if ( dataSet.Cb() >= cbMinChiSquared )
            {
                __declspec( align( 64 ) ) WORD rgwFreqTable[ 256 ] = { 0 };
                INT cbSample = min( dataSet.Cb(), cbChiSquaredSample );
                double dChiSquared = ChiSquaredSignificanceTest( (const BYTE*) dataSet.Pv(), cbSample, rgwFreqTable );
                if ( dChiSquared < dChiSquaredThreshold )
                {
                    fTryCompress = false;
                }
            }
        }

        if ( fTryCompress )
        {
            // If user asks for xpress and xpress10/lz4 is enabled, use that.
            CompressFlags compressFlagsEffective = LVIAddCompressionFlagsIfEnabled( compressFlags, PinstFromPfucb( pfucb ), pfucb->ifmp );

            BYTE * pbDataCompressed = rgbCompressed;
            INT cbDataCompressedMax = sizeof( rgbCompressed );

            // the compressed data won't be larger than the input data so get a buffer of the same size
            // (having the output buffer be at least as big as the input buffer means this code can be
            // used for changes that don't shrink the data -- e.g. obfuscation or encryption)
            if ( cbDataCompressedMax < dataSet.Cb() )
            {
                // if we have already allocated a buffer, there is probably enough space in it for
                // the compressed image as well
                if ( NULL != pvbf )
                {
                    Assert( pvbf == dataSet.Pv() );

                    pbDataCompressed = (BYTE *) dataSet.Pv() + dataSet.Cb();
                    cbDataCompressedMax = max( g_cbPageMin, g_cbPage ) - dataSet.Cb();

                    if ( cbDataCompressedMax < dataSet.Cb() )
                    {
                        // allocate our own buffer
                        BFAlloc( bfasTemporary, &pvbfCompressed );
                        pbDataCompressed = ( (BYTE *) pvbfCompressed );
                        cbDataCompressedMax = g_cbPage;
                    }
                }
            }

            INT cbDataCompressedActual;
            err = ErrPKCompressData(
                dataSet,
                compressFlagsEffective,
                PinstFromPfucb( pfucb ),
                pbDataCompressed,
                cbDataCompressedMax,
                &cbDataCompressedActual );

            // we only store compressed data that is at least as large as a LID. If compress data to smaller than a LID
            // then it becomes possible to get more columns into a record with compression. this would cause
            // compatibility problems when data is replicated between ESE versions (i.e. the Active Directory)
            //
            if ( JET_errSuccess == err && cbDataCompressedActual < LvId::CbLidFromCurrFormat( pfucb ) )
            {
                err = ErrERRCheck( errRECCompressionNotPossible );
            }

            if ( JET_errSuccess == err )
            {
                dataSet.SetPv( pbDataCompressed );
                dataSet.SetCb( cbDataCompressedActual );
                grbit |= grbitSetColumnCompressed;
            }
        }
    }
    //  ================================================================

    if ( fEncrypted )
    {
        Assert( itagSequence < 2 );

        BYTE * pbDataEncrypted = NULL;
        ULONG cbDataEncryptedMax = 0;
        const ULONG cbDataEncryptedNeeded = CbOSEncryptAes256SizeNeeded( dataSet.Cb() );
        if ( cbDataEncryptedNeeded > (ULONG)g_cbPage )
        {
            Assert( fFalse );
            Call( ErrERRCheck( JET_errInternalError ) );
        }

        // if we have already allocated a buffer, there is enough space in it for
        // the encrypted image as well
        if ( dataSet.Pv() == pvbfCompressed || dataSet.Pv() == pvbf )
        {
            pbDataEncrypted = (BYTE *)dataSet.Pv();
            cbDataEncryptedMax = g_cbPage;
        }
        else
        {
            if ( sizeof( rgbEncrypted ) >= cbDataEncryptedNeeded )
            {
                pbDataEncrypted = rgbEncrypted;
                cbDataEncryptedMax = sizeof( rgbEncrypted );
            }
            else
            {
                BFAlloc( bfasTemporary, &pvbfEncrypted );
                pbDataEncrypted = (BYTE *)pvbfEncrypted;
                cbDataEncryptedMax = g_cbPage;
            }

            Assert( cbDataEncryptedNeeded > (ULONG)dataSet.Cb() );
#pragma warning(suppress: 26015)
            UtilMemCpy( pbDataEncrypted, dataSet.Pv(), dataSet.Cb() );
        }

        ULONG cbDataEncryptedActual = dataSet.Cb();
        Call( ErrOSUEncrypt(
                    pbDataEncrypted,
                    &cbDataEncryptedActual,
                    cbDataEncryptedMax,
                    pfucb ) );

        dataSet.SetPv( pbDataEncrypted );
        dataSet.SetCb( cbDataEncryptedActual );
        grbit |= grbitSetColumnEncrypted;
    }

    err = ErrRECSetColumn( pfucb, columnid, itagSequence, &dataSet, grbit );
    // If failed with compression (and no encryption), we can try again with no compression
    if ( err < JET_errSuccess && (grbit == grbitSetColumnCompressed) )
    {
        err = ErrRECSetColumn( pfucb, columnid, itagSequence, &dataOrigSet );
    }

HandleError:
    
    //  free buffer if allocated
    //
    if ( pvbf != NULL )
    {
        BFFree( pvbf );
        pvbf = NULL;
    }

    if ( pvbfCompressed != NULL )
    {
        BFFree( pvbfCompressed );
        pvbfCompressed = NULL;
    }

    if ( pvbfEncrypted != NULL )
    {
        BFFree( pvbfEncrypted );
        pvbfEncrypted = NULL;
    }

    return err;
}


//  ================================================================
LOCAL VOID LVIGetProperLVImageFromRCE(
    PIB         * const ppib,
    const FUCB  * const pfucb,
    FUCB        * const pfucbLV,
    const BOOL  fAfterImage,
    const RCE   * const prceBase
    )
//  ================================================================
{
    PIB         *ppibT;
    RCEID       rceidBegin;

    if ( prceBase->TrxCommitted() == trxMax )
    {
        Assert( ppibNil != prceBase->Pfucb()->ppib );
        ppibT = prceBase->Pfucb()->ppib;
        Assert( prceBase->TrxBegin0() == ppibT->trxBegin0 );
    }
    else
    {
        ppibT = ppibNil;
    }
    Assert( TrxCmp( prceBase->TrxBegin0(), prceBase->TrxCommitted() ) < 0 );

    if ( prceBase->FOperReplace() )
    {
        const VERREPLACE* pverreplace = reinterpret_cast<const VERREPLACE*>( prceBase->PbData() );
        rceidBegin = pverreplace->rceidBeginReplace;
        Assert( rceidNull == rceidBegin || RceidCmp( rceidBegin, prceBase->Rceid() ) < 0 );
    }
    else
    {
        // If not a replace, force retrieval of after-image.
        rceidBegin = rceidNull;
    }

    const BYTE  *pbImage    = NULL;
    ULONG       cbImage     = 0;

    LVKEY_BUFFER    lvkeyBuff;
    BOOKMARK        bookmark;

    pfucbLV->kdfCurr.key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
    bookmark.key.prefix.Nullify();
    bookmark.key.suffix.SetPv( &lvkeyBuff );
    bookmark.key.suffix.SetCb( pfucbLV->kdfCurr.key.Cb() );
    bookmark.data.Nullify();

    const BOOL fImage = FVERGetReplaceImage(
                    ppibT,
                    pfucbLV->u.pfcb->Ifmp(),
                    pfucbLV->u.pfcb->PgnoFDP(),
                    bookmark,
                    rceidBegin,
                    prceBase->Rceid(),
                    prceBase->TrxBegin0(),
                    prceBase->TrxCommitted(),
                    fAfterImage,
                    &pbImage,
                    &cbImage
                    );
    if( fImage )
    {
        pfucbLV->kdfCurr.data.SetPv( (BYTE *)pbImage );
        pfucbLV->kdfCurr.data.SetCb( cbImage );
    }
}

//  ================================================================
LOCAL VOID LVIGetProperLVImageNoRCE(
    PIB         * const ppib,
    const FUCB  * const pfucb,
    FUCB        * const pfucbLV
    )
//  ================================================================
{
    Assert( FNDVersion( pfucbLV->kdfCurr ) );   //  no need to call if its not versioned

    const BYTE  *pbImage    = NULL;
    ULONG       cbImage     = 0;

    LVKEY_BUFFER    lvkeyBuff;
    BOOKMARK        bookmark;

    pfucbLV->kdfCurr.key.CopyIntoBuffer( &lvkeyBuff, sizeof( lvkeyBuff ) );
    bookmark.key.prefix.Nullify();
    bookmark.key.suffix.SetPv( &lvkeyBuff );
    bookmark.key.suffix.SetCb( pfucbLV->kdfCurr.key.Cb() );
    bookmark.data.Nullify();

    Assert( ppib->Level() > 0 );
    Assert( trxMax != ppib->trxBegin0 );

    const RCEID rceidBegin  = ( FFUCBReplacePrepared( pfucb ) ?
                                    pfucb->rceidBeginUpdate :
                                    rceidNull );

    //  on a Replace, only way rceidBeginUpdate is not set is if versioning is off
    //  (otherwise, at the very least, we would have a Write-Lock RCE)
    Assert( rceidNull != rceidBegin
        || !FFUCBReplacePrepared( pfucb )
        || g_rgfmp[pfucb->ifmp].FVersioningOff() );

    //  with RCEID wraparound we are incrementing RCEIDs by two so that
    //  we never end up with rceidNull
    const RCEID rceidLast = PinstFromPpib( ppib )->m_pver->RceidLast() + 2;
    const BOOL fImage = FVERGetReplaceImage(
                    ppib,
                    pfucbLV->u.pfcb->Ifmp(),
                    pfucbLV->u.pfcb->PgnoFDP(),
                    bookmark,
                    rceidBegin,
                    rceidLast,
                    ppib->trxBegin0,
                    trxMax,
                    fFalse, //  always want the before-image
                    &pbImage,
                    &cbImage
                    );
    if( fImage )
    {
        pfucbLV->kdfCurr.data.SetPv( (BYTE *)pbImage );
        pfucbLV->kdfCurr.data.SetCb( cbImage );
    }
}


//  ================================================================
LOCAL VOID LVIGetProperLVImage(
    PIB         * const ppib,
    const FUCB  * const pfucb,
    FUCB        * const pfucbLV,
    const BOOL  fAfterImage,
    const RCE   * const prceBase
    )
//  ================================================================
{
    if( prceBase )
    {
        LVIGetProperLVImageFromRCE(
            ppib,
            pfucb,
            pfucbLV,
            fAfterImage,
            prceBase );
    }
    else
    {
        Assert( !fAfterImage );
        LVIGetProperLVImageNoRCE( ppib, pfucb, pfucbLV );
    }
}


//  ================================================================
ERR ErrRECRetrieveSLongField(
    FUCB            *pfucb,
    LvId            lid,
    BOOL            fAfterImage,
    ULONG           ibGraphic,
    const BOOL      fEncrypted,
    BYTE            *pb,
    ULONG           cbMax,
    ULONG           *pcbActual, //  pass NULL to force LV comparison instead of retrieval
    JET_PFNREALLOC  pfnRealloc,
    void*           pvReallocContext,
    const RCE       * const prceBase    //  used to retrieve older versions of the long-value
    )
//  ================================================================
//
//  opens cursor on LONG tree of table
//  seeks to given lid
//  copies LV from given ibGraphic into given buffer
//  must not use given FUCB to retrieve
//  also must release latches held on LONG tree and close cursor on LONG
//  pb call be null -- in that case we just retrieve the full size of the
//  long value
//
// If fAfterImage is FALSE:
//  We want to see what the long-value looked like before the insert/replace/delete
//  performed by this session. That means we have to adjust the nodes for the before
//  images of any replaces the session has done. This is used (for example) to determine
//  which index entries to delete when replacing a record with an indexed long-value.
//
// If fAfterImage is TRUE:
//  We want to see the LV as it is right now.
//
// If prceBase is not null:
//  We want to see the long-value as it was at the time of the given RCE. This is used
//  by concurrent create index. In order to process replaces properly both the before
//  and after images must be available, so the fAfterImage flag is used here as well.
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );

    ERR         err             = JET_errSuccess;
    BOOL        fFreeBuffer     = fFalse;
    FUCB        *pfucbLV        = pfucbNil;
    ULONG       cb;
    ULONG       ulOffset;
    ULONG       ib;
    BOOL        fInTransaction  = fFalse;

    const BOOL      fComparing      = ( NULL == pcbActual );

    // if we are retrieving an older image, the actual chunks we want to see may have been flag deleted
    // (they won't have been removed by version store cleanup though). In order to see the flag-deleted
    // chunks we have to use fDIRAllNode

    //  BUG: this code doesn't deal with retrieving the before-image of an LV that has been shrunk. If
    //  the end chunks of the LV have been deleted we will not be able to see them. Passing in fDIRAllNode
    //  would allow this but breaks other things (we see the after-image of other transactions replaces).
    //  This should only really affect tuple indexing
    const DIRFLAG   dirflag         = ( prceBase ? fDIRAllNode : fDIRNull );

    //  begin transaction for read consistency
    
    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 65317, JET_bitTransactionReadOnly ) );
        fInTransaction = fTrue;
    }

    //  open cursor on LONG, seek to long field instance
    //  seek to ibGraphic
    //  copy data from long field instance segments as
    //  necessary
    
    Call( ErrDIROpenLongRoot( pfucb ) );

    if( wrnLVNoLongValues == err )
    {
        if( g_fRepair )
        {
            //  if we are running repair and reach to this point,
            //  the LV tree is corrupted and deleted from catalog
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        else
        {
            LVReportAndTrapCorruptedLV2( PinstFromPfucb( pfucb ), g_rgfmp[pfucb->u.pfcb->Ifmp()].WszDatabaseName(), pfucb->u.pfcb->Ptdb()->SzTableName(), Pcsr( pfucb )->Pgno(), lid, L"2a34e07b-fe00-40ae-8cd4-5ebd87a6faa1" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }
    }

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    PERFOpt( PERFIncCounterTable( cLVRetrieves, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
    Ptls()->threadstats.cSeparatedLongValueRead++;

    //  move to long field instance

    const ULONG cbToPreread = (0 == ibGraphic && NULL != pb ) ? cbMax : 0;
    Call( ErrDIRDownLVRootPreread( pfucbLV, lid, dirflag, cbToPreread ) );
    if ( ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) ) || prceBase )
    {
        LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fAfterImage, prceBase );
    }

    Assert( pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT) || pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT2) );
    if ( pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT2) )
    {
        // Only encrypted LVs should use LVROOT2
        Assert( FLVEncrypted( reinterpret_cast<LVROOT2*>( pfucbLV->kdfCurr.data.Pv() )->fFlags ) );
        Assert( fEncrypted );
    }
    else
    {
        Assert( !fEncrypted );
    }

    const LVROOT* plvroot = reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() );
    if( plvroot->ulReference == ulMax )
    {
        FireWall( "InvalidLVRefcount" );
        Error( ErrERRCheck( JET_errInternalError ) );
    }

    const ULONG ulLVSize        = plvroot->ulSize;
    const ULONG ulActual        = ulLVSize - ibGraphic;
    const ULONG cbToRead        = min( cbMax, ulActual );

    //  set return value cbActual
    
    if ( ibGraphic >= ulLVSize )
    {
        if ( NULL != pcbActual )
            *pcbActual = 0;

        if ( fComparing && 0 == cbMax )
            err = ErrERRCheck( JET_errMultiValuedDuplicate );   //  both are zero-length
        else
            err = JET_errSuccess;

        goto HandleError;
    }
    else
    {
        if ( NULL != pcbActual )
            *pcbActual = ulActual;
    }

    if ( fComparing )
    {
        if ( ulActual != cbMax )
        {
            err = JET_errSuccess;       //  size is different, so LV must be different
            goto HandleError;
        }
    }
    else if ( NULL == pb )      //  special code to handle NULL buffer. just return the size
    {
        goto HandleError;
    }

    //  if we are using the pfnRealloc hack to read up to cbMax bytes of the LV
    //  then grab a buffer large enough to store the data to return and place
    //  a pointer to that buffer in the output buffer.  we will then rewrite
    //  the args to look like a normal LV retrieval
    //
    //  NOTE:  on an error, the allocated memory will NOT be freed

    if ( pfnRealloc )
    {
        Alloc( *((BYTE**)pb) = (BYTE*)pfnRealloc( pvReallocContext, NULL, min( cbMax, ulActual ) ) );
        fFreeBuffer = fTrue;                    //  free pb on an error
        pb          = *((BYTE**)pb);            //  redirect pv to the new buffer
        cbMax       = min( cbMax, ulActual );   //  fixup cbMax to be the size of the new buffer
    }

    const BYTE * const pbMax    = pb + cbToRead;
    
    //  move to ibGraphic in long field. if we are prereading, we seek for the chunk, even if it is the first one
    //  that will intitiate preread (which is done in BTDown). if only the next page is to be preread we will have
    //  done that above (using the PgnoNext()) so we don't have to reseek

    const LONG cbLVChunkMost = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    if( ibGraphic < (SIZE_T)cbLVChunkMost )
    {
        //  the chunk we want is offset 0, which is the next chunk. do a DIRNext to avoid the seek
        
        Call( ErrDIRNext( pfucbLV, dirflag ) );
        if ( ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) ) || prceBase )
        {
            LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fAfterImage, prceBase );
        }

        Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
        OffsetFromKey( &ulOffset, pfucbLV->kdfCurr.key );
        if( 0 != ulOffset )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"691dccf0-13bc-48f6-a156-732dfba599af" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }
    }
    else
    {
        Call( ErrDIRDownLVDataPreread( pfucbLV, lid, ibGraphic, dirflag, cbToRead ) );
        if ( ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) ) || prceBase )
        {
            LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fAfterImage, prceBase );
        }
    }

    //  increment counter to reflect read of the initial chunk
    //
    PERFOpt( PERFIncCounterTable( cLVChunkRetrieves, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    //  determine offset and length of data to copy 
    
    Assert( FIsLVChunkKey( pfucbLV->kdfCurr.key ) );
    OffsetFromKey( &ulOffset, pfucbLV->kdfCurr.key );
    cb = cbMax;
    ib = ibGraphic - ulOffset;

    if ( fComparing )
    {
        bool fIdentical;
        Call( ErrLVCompare(
            pfucbLV,
            lid,
            ulLVSize,
            pfucbLV->kdfCurr.key,
            pfucbLV->kdfCurr.data,
            fEncrypted,
            ib,
            pb,
            &cb,
            &fIdentical ) );
        if ( !fIdentical )
        {
            err = JET_errSuccess;           //  diff found
            goto HandleError;
        }
    }
    else
    {
        Call( ErrLVRetrieve(
            pfucbLV,
            lid,
            ulLVSize,
            pfucbLV->kdfCurr.key,
            pfucbLV->kdfCurr.data,
            fEncrypted,
            ib,
            pb,
            &cb ) );
    }
    if ( (ib+cb) > (SIZE_T)cbLVChunkMost )
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"d930c3f9-c311-4aa1-8347-530368fc7746" );
        Error( ErrERRCheck( JET_errLVCorrupted ) );
    }
    pb += cb;

    //  copy further chunks
    
    while ( pb < pbMax )
    {
        ulOffset += cbLVChunkMost;

        // if we have run out of preread pages and have more than one chunk to retrieve
        // then issue another preread
        const ULONG cbRemaining = (ULONG)(pbMax-pb);
        if( 0 == pfucbLV->cpgPrereadNotConsumed && cbRemaining > (ULONG)cbLVChunkMost )
        {
            err = ErrDIRDownLVDataPreread( pfucbLV, lid, ulOffset, dirflag, cbRemaining );
        }
        else
        {
            err = ErrDIRNext( pfucbLV, dirflag | fDIRSameLIDOnly );
        }
        if ( ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) ) || prceBase )
        {
            LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fAfterImage, prceBase );
        }

#ifdef DEBUG
        //  This should only fail for resource failures.
        // something like CallSx( err , "JET_errDiskIO && JET_errOutOfMemory" );
        if ( JET_errDiskIO != err &&
                JET_errOutOfMemory != err &&
                !FErrIsDbCorruption( err ) )
        {
            CallS( err );
        }
#endif // DEBUG

        Call( err );

        //  increment counter to reflect read of each subsequent chunk
        //
        PERFOpt( PERFIncCounterTable( cLVChunkRetrieves, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

        //  make sure we are still on the same long value
        Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
        if ( wrnLVNoMoreData == err )
        {
            //  ErrDIRNext should have returned JET_errNoCurrentRecord
            Assert( fFalse );
            break;
        }

        cb = (ULONG)(pbMax - pb);
        if ( fComparing )
        {
            bool fIdentical;
            Call( ErrLVCompare(
                pfucbLV,
                lid,
                ulLVSize,
                pfucbLV->kdfCurr.key,
                pfucbLV->kdfCurr.data,
                fEncrypted,
                0,
                pb,
                &cb,
                &fIdentical ) );
            
            if ( !fIdentical )
            {
                err = JET_errSuccess;       //  diff found
                goto HandleError;
            }
        }
        else
        {
            Call( ErrLVRetrieve(
                pfucbLV,
                lid,
                ulLVSize,
                pfucbLV->kdfCurr.key,
                pfucbLV->kdfCurr.data,
                fEncrypted,
                0,
                pb,
                &cb ) );
        }
        Assert( cb <= (SIZE_T)cbLVChunkMost );
        if ( cb > (SIZE_T)cbLVChunkMost )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"a0f292cb-f892-48ef-a7d9-ea886fb030fd" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        pb += cb;
    }

    err = ( fComparing ? ErrERRCheck( JET_errMultiValuedDuplicate ) : JET_errSuccess );

HandleError:
    if ( pfucbNil != pfucbLV )
    {
        DIRCloseLongRoot( pfucb );
    }

    //  commit -- we have done no updates must succeed
    if ( fInTransaction )
    {
        CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( pfucb->ppib );
    return err;
}


//  ================================================================
ERR ErrRECRetrieveSLongFieldPrereadOnly(
    FUCB        *pfucb,
    LvId        lid,
    const ULONG ulOffset,
    const ULONG cbMax,
    const BOOL fLazy,
    const JET_GRBIT grbit
    )
//  ================================================================
//
//  opens cursor on LONG tree of table
//  seeks to given lid
//  prereads the LV root and the first chunk at offset given
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );
    ERR         err             = JET_errSuccess;
    FUCB        *pfucbLV        = pfucbNil;
    const LONG  cbLVChunkMost   = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    ULONG       cbLVPerPage     = (( g_cbPage / cbLVChunkMost ) * cbLVChunkMost);
    //  if preread many then artificially set cbMax to that value to preread the most data
    //
    ULONG       cbMaxT          = cbMax;
    if ( ( grbit & JET_bitRetrievePrereadMany ) != 0 )
    {
        cbMaxT = cpageLVPrereadMany * cbLVPerPage;
    }

    //  Truncate to preivous chunk boundary to create an lvkey that can actually exist in the LV tree
    //  Truncate because index ranges are inclusive
    if ( cbMaxT % cbLVChunkMost > 0 )
    {
        cbMaxT = (cbMaxT / cbLVChunkMost) * cbLVChunkMost;
    }
    else if ( cbMaxT >= (ULONG) cbLVChunkMost )
    {
        cbMaxT -= cbLVChunkMost;
    }

    //  Make keys for the range [LID, LID:cbMaxT]
    //  A cbMax smaller than lv chunk size will result in an index range that reads the LV root + the first chunk
    //
    LVKEY_BUFFER    lvkeyLidStart;
    KEY             key;
    LVRootKeyFromLid( &lvkeyLidStart, &key, lid );

    Assert( key.prefix.FNull() );
    VOID        *rgpvStart = key.suffix.Pv();
    ULONG       rgcbStart = key.suffix.Cb();

    LVKEY_BUFFER    lvkeyLidOffsetEnd;
    LVKeyFromLidOffset( &lvkeyLidOffsetEnd, &key, lid, cbMaxT );

    Assert( key.prefix.FNull() );
    VOID        *rgpvEnd = key.suffix.Pv();
    ULONG       rgcbEnd = key.suffix.Cb();
    LONG        lT = 0;

    Call( ErrDIROpenLongRoot( pfucb ) );
    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    Call( ErrBTPrereadKeyRanges(
        pfucbLV->ppib,
        pfucbLV,
        &rgpvStart,
        &rgcbStart,
        &rgpvEnd,
        &rgcbEnd,
        1,
        &lT,
        0,
        ulMax,
        JET_bitPrereadForward,
        NULL ) );

HandleError:
    if ( pfucbLV != pfucbNil )
    {
        DIRCloseLongRoot( pfucb );
    }

    AssertDIRNoLatch( pfucb->ppib );
    return err;
}


//  ================================================================
ERR ErrRECRetrieveSLongFieldRefCount(
    FUCB    *pfucb,
    LvId        lid,
    BYTE    *pb,
    ULONG   cbMax,
    ULONG   *pcbActual
    )
//  ================================================================
//
//  opens cursor on LONG tree of table
//  seeks to given lid
//  returns the reference count on the LV
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( pcbActual );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );

    ERR         err             = JET_errSuccess;
    FUCB        *pfucbLV        = pfucbNil;
    BOOL        fInTransaction  = fFalse;

    //  begin transaction for read consistency
    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 40741, JET_bitTransactionReadOnly ) );
        fInTransaction = fTrue;
    }

    Call( ErrDIROpenLongRoot( pfucb ) );
    Assert( wrnLVNoLongValues != err );

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    // move to long field instance
    Call( ErrDIRDownLVRoot( pfucbLV, lid, fDIRNull ) );

    const LVROOT* plvroot;
    plvroot = reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() );

    if ( FPartiallyDeletedLV( plvroot->ulReference ) )
    {
        FireWall( "GetRefcountOfPartiallyDeletedLV" );
        Error( ErrERRCheck( JET_errRecordDeleted ) );
    }

    if ( cbMax < sizeof( plvroot->ulReference ) )
    {
        Error( ErrERRCheck( JET_errInvalidBufferSize ) );
    }

    *pcbActual = sizeof( plvroot->ulReference );
    *( reinterpret_cast<Unaligned< ULONG > *>( pb ) ) = plvroot->ulReference;

HandleError:
    if ( pfucbLV != pfucbNil )
    {
        DIRCloseLongRoot( pfucb );
    }

    //  commit -- we have done no updates must succeed
    if ( fInTransaction )
    {
        CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( pfucb->ppib );
    return err;
}

ERR ErrRECGetLVSize(
    FUCB * const    pfucb,
    const LvId      lid,
    const BOOL      fLogicalOnly,
    QWORD * const   pcbLVDataLogical,   // logical (i.e. uncompressed) size of the data
    QWORD * const   pcbLVDataPhysical,  // physical (i.e. compressed) size of the data
    QWORD * const   pcbLVOverhead )
{
    ERR             err                 = JET_errSuccess;
    FUCB *          pfucbLV             = pfucbNil;
    ULONG           cbLVSize;

    //  we only want the before image if we're in the middle of a replace
    //  that updated a separated long-value and we're not looking at the
    //  copy buffer
    //
    const BOOL      fAfterImage = ( !Pcsr( pfucb )->FLatched()
                                    || !FFUCBUpdateSeparateLV( pfucb )
                                    || !FFUCBReplacePrepared( pfucb ) );

    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

    //  we don't call ErrRECRetrieveSLongField() because we have the
    //  data page latched and that function can't be called while
    //  holding a latch
    //

    //  open cursor on LONG, seek to long field instance
    //  seek to ibGraphic
    //  copy data from long field instance segments as
    //  necessary
    
    Assert( pfucb->ppib->Level() > 0 );
    Call( ErrDIROpenLongRoot( pfucb ) );

    if( wrnLVNoLongValues == err )
    {
        LVReportAndTrapCorruptedLV2( PinstFromPfucb( pfucb ), g_rgfmp[pfucb->u.pfcb->Ifmp()].WszDatabaseName(), pfucb->u.pfcb->Ptdb()->SzTableName(), Pcsr( pfucb )->Pgno(), lid, L"977037ff-3128-4ea5-a1c3-fa6800dd1991" );
        Error( ErrERRCheck( JET_errLVCorrupted ) );
    }

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    //  move to long field instance
    
    Call( ErrDIRDownLVRootPreread( pfucbLV, lid, fDIRNull, fLogicalOnly ? 0 : ulMax ) );
    if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
    {
        LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fFalse, prceNil );
    }

    //  get the logical size of the LV and account for the LVROOT
    cbLVSize = reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() )->ulSize;

    *pcbLVDataLogical   += cbLVSize;
    Assert( lid.Cb() == pfucbLV->kdfCurr.key.Cb() );
    *pcbLVOverhead      += CPAGE::cbInsertionOverhead + cbKeyCount + pfucbLV->kdfCurr.key.Cb() + pfucbLV->kdfCurr.data.Cb();

    if ( fLogicalOnly )
    {
        *pcbLVDataPhysical += cbLVSize;
        goto HandleError;
    }

    ULONG ulOffset = ulLVOffsetFirst;
    // UNDONE: if pfucbLV->cpgPrereadNotConsumed is 0 then use ErrDIRDownLVDataPreread
    while ( ( err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly ) ) >= JET_errSuccess )
    {
        if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
        {
            LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fFalse, prceNil );
        }
        
        //  make sure we are still on the same long value
        Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
        if( wrnLVNoMoreData == err )
        {
            AssertSz( fFalse, "ErrDIRNext should have returned JET_errNoCurrentRecord" );
            Error( ErrERRCheck( JET_errInternalError ) );
        }
        if( ulOffset > cbLVSize )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"844a5ab7-5d88-4629-a1d0-a9811c1bf46d" );
            Error( ErrERRCheck( JET_errLVCorrupted ) ); // LV is too large
        }

        Assert( FIsLVChunkKey( pfucbLV->kdfCurr.key ) );
        *pcbLVDataPhysical  += pfucbLV->kdfCurr.data.Cb();
        *pcbLVOverhead      += ( CPAGE::cbInsertionOverhead + cbKeyCount + pfucb->kdfCurr.key.Cb() );

        ulOffset += pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    }

    if ( JET_errNoCurrentRecord != err )
    {
        Call( err );
    }
    err = JET_errSuccess;

HandleError:
    if ( pfucbNil != pfucbLV )
    {
        DIRCloseLongRoot( pfucb );
    }
    return err;
}

//  ================================================================
ERR ErrRECDoesSLongFieldExist( FUCB * const pfucb, const LvId lid, BOOL* const pfExists )
//  ================================================================
//
//  Tests to see if a given LV exists for the current transaction.
//  An LV with a zero ref count is considered to not exist.
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );

    ERR     err = JET_errSuccess;
    FUCB *  pfucbLV = pfucbNil;
    BOOL    fInTransaction = fFalse;

    //  init out params for failure

    *pfExists = fFalse;

    //  begin transaction for read consistency

    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 38916, JET_bitTransactionReadOnly ) );
        fInTransaction = fTrue;
    }

    //  open the LV tree.  if there are no LVs at all then obviously this LV doesn't exist

    Call( ErrDIROpenLongRoot( pfucb ) );

    if ( wrnLVNoLongValues == err )
    {
        *pfExists = false;
        err = JET_errSuccess;
        goto HandleError;
    }

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    //  try to find this LV by its LID.  if we can't find it then it doesn't exist
    //
    //  NOTE:  do not declare logical corruption if it doesn't exist!

    err = ErrDIRDownLVRootPreread( pfucbLV, lid, fDIRNull, 0, fFalse );
    if ( err < JET_errSuccess && err != JET_errRecordNotFound )
    {
        Error( err );
    }
    if ( err != JET_errSuccess )
    {
        *pfExists = false;
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( pfucbLV->kdfCurr.data.Cb() == sizeof( LVROOT ) || pfucbLV->kdfCurr.data.Cb() == sizeof( LVROOT2 ) );

    //  the LV must have a non zero ref count to exist

    ULONG ulRef = UlLVIVersionedRefcount( pfucbLV );
    *pfExists = ( ulRef > 0 && !FPartiallyDeletedLV( ulRef ) );

HandleError:
    if ( pfucbNil != pfucbLV )
    {
        DIRCloseLongRoot( pfucb );
    }

    if ( fInTransaction )
    {
        CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( pfucb->ppib );
    return err;
}

//  ================================================================
LOCAL ERR ErrLVIGetMaxLidInTree( FUCB * pfucb, FUCB * pfucbLV, _Out_ LvId* const plid )
//  ================================================================
//
//  Seek to the end of the LV tree passed in and return the highest LID found.
//  We use critLV to syncronize this.
//
//  We expect to be in the critical section of the fcb of the table
//
//-
{
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( FAssertLVFUCB( pfucbLV ) );

    ERR     err     = JET_errSuccess;
    DIB     dib;

    dib.dirflag = fDIRAllNode;
    dib.pos     = posLast;

    err = ErrDIRDown( pfucbLV, &dib );
    switch( err )
    {
        case JET_errSuccess:
            LidFromKey( plid, pfucbLV->kdfCurr.key );
            Assert( *plid > lidMin );       //  lid's start numbering at 1.
            break;

        case JET_errRecordNotFound:
            *plid = lidMin;             //  Empty tree, so first lid is 1
            err = JET_errSuccess;           //  the tree can be empty
            break;

        default:                            //  error condition -- don't set lid
            Assert( err != JET_errNoCurrentRecord );
            Assert( err < 0 );              //  if we get a warning back, we're in a lot of trouble because the caller doesn't handle that case
            break;
    }

    DIRUp( pfucbLV );                       //  back to LONG.
    return err;
}

//  ================================================================
LOCAL ERR ErrLVIGetNextLID( _In_ FUCB * const pfucb, _In_ FUCB * const pfucbLV, _Out_ LvId * const plid )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pfucbLV );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( plid );
    *plid = 0;
    
    ERR err = JET_errSuccess;
    
    // Lid's are numbered starting at 1.  A lidLast of 0 indicates that we must
    // first retrieve the lidLast. In the pathological case where there are
    // currently no lid's, we'll go through here anyway, but only the first
    // time (since there will be lid's after that).
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    
    TDB* const ptdb = pfucb->u.pfcb->Ptdb();
    _LID64 lidInitial = *ptdb->PUi64LidLast();
    _LID64 lidFinal = lidInitial;

    if ( lidInitial == lidMin )
    {
        // TDB's lidLast hasn't been set yet.  Must seek to it.
        LvId lvid;
        Call( ErrLVIGetMaxLidInTree( pfucb, pfucbLV, &lvid ) );
        Assert( pfucb->u.pfcb->IsUnlocked() );
        lidFinal = lvid;
    }

    if ( ptdb->FLid64() )
    {
        // Snap to: 0x80000000`80000000 (the first LID64 we will generate will be 0x80000000`80000001)
        // This makes it so that even if we lose the top or lower half of the lid (e.g. because of a code bug),
        // we would still be able to identify a partial LID64, for debugging purposes (atleast for the first 2B lids).
        lidFinal = max( lid64First - 1, lidFinal );
    }

    OSSYNC_FOREVER
    {
        // Keep updating because another thread can switch from LID32 -> LID64 at any time.
        // We have to use the correct lidMaxAllowed in that case, or risk returning JET_errOutOfLongValueIDs erroneously.
        const LvId lidMaxAllowed = LvId::FIsLid64( lidFinal ) ? lid64MaxAllowed : lid32MaxAllowed;
        lidFinal++;

        if ( lidFinal < lidMaxAllowed )
        {
            const _LID64 lidNew = AtomicCompareExchange( (__int64*) ptdb->PUi64LidLast(), lidInitial, lidFinal );
            if ( lidInitial == lidNew )
            {
                *plid = lidFinal;
                break;
            }

            // retry
            lidFinal = lidInitial = lidNew;
        }
        else
        {
            OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagAlertOnly, L"38b03ad5-363f-42a5-bf9f-294c1a687e45" );
            Error( ErrERRCheck( JET_errOutOfLongValueIDs ) );
        }
    }

    Assert( *plid > lidInitial );
    Assert( *plid < ( plid->FIsLid64() ? lid64MaxAllowed : lid32MaxAllowed ) );

    // Validate if we obey the current format, switch if we don't
    // We might have to switch the format because of misconfiguration of the LID64 feature.
    if ( !plid->FLidObeysCurrFormat( pfucb ) )
    {
        // The tree has a LID64 in it, while the format flags say we are on LID32.
        // We've seen this happen when we enable LID64 via vcfg for the first time.
        // The configuration isn't applied to all the copies of a DB instantly.
        // So a DB with LID64 can failover to another copy that is on LID32 config.
        if ( plid->FIsLid64() )
        {
            pfucb->u.pfcb->Ptdb()->SetFLid64( fTrue );
        }
    }

    PERFOpt( PERFIncCounterTable( cLVCreates, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
    PERFOpt( cLVMaximumLID.Max( PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ), *plid ) );
    Ptls()->threadstats.cSeparatedLongValueCreated++;

    Assert( *plid > 0 );
HandleError:
    return err;
}

//  ================================================================
LOCAL ERR ErrLVIInsertLVROOT( _In_ FUCB * const pfucbLV, _In_ const LvId lid, _In_ const LVROOT2 * const plvroot )
//  ================================================================
{
    ASSERT_VALID( pfucbLV );
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( lid > 0 );
    Assert( plvroot );
    Assert( plvroot->ulReference > 0 );
    
    ERR err = JET_errSuccess;
    
    KEY key;
    DATA data;
    LVKEY_BUFFER lvkey;

    data.SetPv( const_cast<LVROOT2 *>( plvroot ) );
    // Only use new size LVROOT if any flags are set
    data.SetCb( plvroot->fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );

    LVRootKeyFromLid( &lvkey, &key, lid );
    Call( ErrDIRInsert( pfucbLV, key, data, fDIRNull ) );
    
HandleError:
    return err;
}

//  ================================================================
LOCAL ERR ErrLVIInsertLVData(
    _In_ FUCB * const pfucbLV,
    const LvId lid,
    const ULONG ulOffset,
    _In_ const DATA * const pdata,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    FUCB *pfucbTable )
//  ================================================================
{
    ERR err = JET_errSuccess;
    KEY key;
    LVKEY_BUFFER lvkey;

    ASSERT_VALID( pfucbLV );
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( lid > 0 );
    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    Assert( 0 == ( ulOffset % cbLVChunkMost ) );
    Assert( pdata );
    Assert( NULL != pdata->Pv() );
    Assert( pdata->Cb() <= cbLVChunkMost );
    if ( pdata->Cb() > cbLVChunkMost )
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"500022a2-0b2b-44da-8298-7703d67c6c85" );
        Error( ErrERRCheck( JET_errLVCorrupted ) );
    }
    
    LVKeyFromLidOffset( &lvkey, &key, lid, ulOffset );

    PERFOpt( PERFIncCounterTable( cLVChunkAppends, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    err = ErrLVInsert( pfucbLV, key, *pdata, compressFlags, fEncrypted, pfucbTable, fDIRBackToFather );
    Assert( JET_errKeyDuplicate != err );
    Call( err );

HandleError:
    return err;
}


//  ================================================================
ERR ErrRECICreateLvRootAndChunks(
    _In_ FUCB                   * const pfucb,
    _In_ const DATA             * const pdataField,
    _In_ const CompressFlags    compressFlags,
    _In_ const BOOL             fEncrypted,
    _In_ CPG *                  pcpgLvSpaceRequired,
    _Out_ LvId                  * const plid,
    __in_opt FUCB               **ppfucb,
    __in_opt LVROOT2            *plvrootInit )
//  ================================================================
//
//  Creates (or converts intrinsic long field) into separated long field.
//  Intrinsic long field constraint of length less than CbLVIntrinsicTableMost() bytes
//  means that breakup is unnecessary.  Long field may also be
//  null. At the end a LV with LID==pfucb->u.pfcb->ptdb->ulLongIdLast will have
//  been inserted.
//
//  ppfucb can be null
//
//  *pcpgLvSpaceRequired should really be a BOOL fContiguousLv requested by user, but since
//  the way lvopInsert works we only pass in the first chunk, we can't compute the actual
//  cpg required for the LV in here.  And we can't set the transient space request out a
//  layer because we don't have the pfucbLV open until we're in here.  Restructuring the
//  code to figure out why main lvopInsert path doesn't pass whole LV in here would be a
//  good thing to do, because it is confusing as clearly this code can build out a larger
//  LV than just one chunk.
//
//-
{
    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    ASSERT_VALID( pdataField );
    Assert( plid );
    //  modification allows this case to be hit.
    //Assert( sizeof(LID) < (ULONG)pdataField->Cb() );
    Expected( pcpgLvSpaceRequired == NULL || *pcpgLvSpaceRequired == 0 || pdataField->Cb() < g_cbPage );

    ERR         err             = JET_errSuccess;
    FUCB        *pfucbLV        = NULL;
    DATA        dataRemaining;
    LVROOT2     lvroot;
    INT         cbInserted      = 0;

    if( NULL == plvrootInit )
    {
        lvroot.ulReference  = 1;
        lvroot.ulSize       = pdataField->Cb();
        lvroot.fFlags       = fEncrypted ? fLVEncrypted : 0;
        plvrootInit = &lvroot;
    }
    Assert( plvrootInit );

    if ( pfucb->u.pfcb->FIntrinsicLVsOnly() )
    {
        //  UNDONE: this flag is currently only supported on
        //  sorts and temp. tables, but it would be nice
        //  for regular tables to support it as well
        //
        Assert( pfucb->u.pfcb->FTypeSort() || pfucb->u.pfcb->FTypeTemporaryTable() );
        return ErrERRCheck( JET_errCannotSeparateIntrinsicLV );
    }
    
    //  sorts don't have LV's (either the sort would have been materialised
    //  or the LV's would have been forced intrinsic)
    //
    Assert( !pfucb->u.pfcb->FTypeSort() );

    CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV, fTrue ) );
    Assert( wrnLVNoLongValues != err );
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    BOOL fBeginTrx = fFalse;

    if ( pcpgLvSpaceRequired && *pcpgLvSpaceRequired != 0 )
    {
        //  Note: It's the additional reserve, so it's the page we'll allocate for this
        //  LV minus one, irrelevant of spacehints min/max extent, so defend the limits
        //  here.  Debatable if user may want this to work, to override min/max extent
        //  but for now, pretend not supported.  We can always remove the check.

        if ( pfucbLV->u.pfcb->Pfcbspacehints() &&
                !pfucbLV->u.pfcb->Pfcbspacehints()->FRequestFitsExtentRange( ( *pcpgLvSpaceRequired ) ) )
        {
            //  The specified JET_bitSetContiguousLV bit is invalid if the space hints are too small.
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }
    }

    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 57125, NO_GRBIT ) );
        fBeginTrx = fTrue;
    }

    Call( ErrLVIGetNextLID( pfucb, pfucbLV, plid ) );
    
    //  Insert the data first
    //
    //  There are often cases where the LVROOT of an LV would often fit on a page,
    //  but the first data chunk would have to go on a new page. That made retrieval
    //  of the LV slow, because 2 pages had to be read to retrieve the root and
    //  the data.
    //
    //  The solution to that problem was to change the split code to move the LVROOT
    //  as well as the data when doing a split. This created another performance problem:
    //  when appending LVs to a tree append splits were turned into right splits -- the
    //  LVROOT would be moved as well as the inserted data causing the split. Right splits
    //  are less performant than append splits (requiring page dependencies or extra data
    //  logging).
    //
    //  The solution is to insert the data first and then the LVROOT. If a split is required
    //  to hold the data, that can be done as an append split and then the LVROOT will be
    //  inserted on the same page as the data (the separator key for the previous page 
    //  won't allow the LVROOT to be inserted there).
    //

    dataRemaining.SetPv( pdataField->Pv() );
    dataRemaining.SetCb( pdataField->Cb() );

    // first chunk

    ULONG ulOffset = ulLVOffsetFirst;

    if ( pcpgLvSpaceRequired && *pcpgLvSpaceRequired != 0 )
    {
        //  Note: It's the additional reserve, so it's the page we'll allocate for this
        //  LV minus one.
        Assert( dataRemaining.Cb() > 0 ); // should be incompatible with the zero length type grbits that lead to this.
        DIRSetActiveSpaceRequestReserve( pfucbLV, (*pcpgLvSpaceRequired) - 1 );
    }
    const LONG cbLVChunkMost = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    if ( dataRemaining.Cb() > 0 )
    {
        DATA dataInsert;
        dataInsert.SetPv( dataRemaining.Pv() );
        dataInsert.SetCb( min( cbLVChunkMost, dataRemaining.Cb() ) );
        
        Call( ErrLVIInsertLVData( pfucbLV, *plid, ulOffset, &dataInsert, compressFlags, fEncrypted, pfucb ) );
        cbInserted += dataInsert.Cb();
        
        dataRemaining.DeltaCb( -dataInsert.Cb() );  // less data to copy
        dataRemaining.DeltaPv( dataInsert.Cb() );   // from further in the buffer

        ulOffset += cbLVChunkMost;
    }

    // LVROOT
    
    Call( ErrLVIInsertLVROOT( pfucbLV, *plid, plvrootInit ) );

    // append any remaining data
    
    while( dataRemaining.Cb() > 0 )
    {
        //  Note: Strangely this is not the typical path for standard large LV inserts, we 
        //  instead only insert for those the LVROOT and first LVCHUNK above.  The rest of 
        //  the LV past root / first chunk are inserted with ErrRECAOSeparateLV() after this 
        //  function is called.  So you might wonder what does this case handle?  Well this
        //  handles if there was a very large / forcibly kept intrinsic LV that then has to
        //  burst out of the record into more than one LVCHUNK when burst.  Apparently the
        //  ErrRECAOSeparateLV() is more efficient for large LVs.

        Assert( pcpgLvSpaceRequired == NULL || *pcpgLvSpaceRequired == 0 );

        DATA dataInsert;
        dataInsert.SetPv( dataRemaining.Pv() );
        dataInsert.SetCb( min( cbLVChunkMost, dataRemaining.Cb() ) );
        
        Call( ErrLVIInsertLVData( pfucbLV, *plid, ulOffset, &dataInsert, compressFlags, fEncrypted, pfucb ) );
        cbInserted += dataInsert.Cb();
        
        dataRemaining.DeltaCb( -dataInsert.Cb() );  // less data to copy
        dataRemaining.DeltaPv( dataInsert.Cb() );   // from further in the buffer

        ulOffset += cbLVChunkMost;
    }

    err = ErrERRCheck( JET_wrnCopyLongValue );

    Assert( pdataField->Cb() == cbInserted );
HandleError:

    if ( pcpgLvSpaceRequired && CpgDIRActiveSpaceRequestReserve( pfucbLV ) )
    {
        if ( CpgDIRActiveSpaceRequestReserve( pfucbLV ) == cpgDIRReserveConsumed )
        {
            //  Yay, we consumed the space, communicate to the caller they don't need to reserve any more 
            //  space for this LV.  This is not guaranteed though, as it could be this first LV chunk actually
            //  fits on a page.
            *pcpgLvSpaceRequired = 0;
        }
        DIRSetActiveSpaceRequestReserve( pfucbLV, 0 );
    }
    Assert( CpgDIRActiveSpaceRequestReserve( pfucbLV ) == 0 );

    // discard temporary FUCB, or return to caller if ppfucb is not NULL.
    if ( err < JET_errSuccess || NULL == ppfucb )
    {
        DIRClose( pfucbLV );
    }
    else
    {
        *ppfucb = pfucbLV;
    }

    if ( fBeginTrx )
    {
        if ( err >= JET_errSuccess )
        {
            err = ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT );
            //  if we fail, fallthrough to Rollback below
        }
        if ( err < JET_errSuccess )
        {
            CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
        }
    }

    Assert( JET_errKeyDuplicate != err );

    return err;
}

//  ================================================================
ERR ErrRECSeparateLV(
    _In_ FUCB                   * const pfucb,
    _In_ const DATA             * const pdataField,
    _In_ const CompressFlags    compressFlags,
    _In_ const BOOL             fEncrypted,
    _Out_ LvId                  * const plid,
    __in_opt FUCB               **ppfucb,
    __in_opt LVROOT2            *plvrootInit )
//  ================================================================
{
    return ErrRECICreateLvRootAndChunks( pfucb, pdataField, compressFlags, fEncrypted, NULL, plid, ppfucb, plvrootInit );
}

//  ================================================================
ERR ErrRECAffectSeparateLV( FUCB *pfucb, LvId *plid, ULONG fLV )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( plid && *plid > lidMin );
    Assert( pfucb->ppib->Level() > 0 );

    ERR         err         = JET_errSuccess;
    FUCB        *pfucbLV    = pfucbNil;

    CallR( ErrDIROpenLongRoot( pfucb ) );
    Assert( wrnLVNoLongValues != err );

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    //  move to long field instance
    Call( ErrDIRDownLVRoot( pfucbLV, *plid, fDIRNull ) );

    Assert( locOnCurBM == pfucbLV->locLogical );
    Assert( Pcsr( pfucbLV )->FLatched() );

    if ( fLVDereference == fLV )
    {
        const LONG      lDelta      = -1;
        KEYDATAFLAGS    kdf;

        // Take a lock on the row (not the LVROOT)
        err = ErrDIRGetLock( pfucb, writeLock );
        if( err == JET_errWriteConflict )
        {
            Call( err );
        }

        //  cursor's kdfCurr may point to version store,
        //  so must go directly to node to find true refcount
        NDIGetKeydataflags( Pcsr( pfucbLV )->Cpage(), Pcsr( pfucbLV )->ILine(), &kdf );
        Assert( sizeof( LVROOT ) == kdf.data.Cb() || sizeof( LVROOT2 ) == kdf.data.Cb() );

        const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
        const LONG cbLV = plvroot->ulSize;

        if ( 0 == plvroot->ulReference )
        {
            // we should never get here
            AssertTrack( fFalse, "LVRefCountWriteConflictDetected")
            Error( ErrERRCheck( JET_errWriteConflict ) );
        }

        LONG lOldValue;
        Call( ErrDIRDelta< LONG >(
                pfucbLV,
                OffsetOf( LVROOT, ulReference ),
                lDelta,
                &lOldValue,
                fDIRNull | fDIRDeltaDeleteDereferencedLV ) );

        AssertTrack( lOldValue > 0, "LVRefCountBelowZero" );

        //  track the count and size of any LV nodes we are releasing as a way of measuring the version store
        //  cleanup debt we are incurring
        if ( lOldValue <= 1 )
        {
            const LONG  cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
            const LONG  cChunks = ( cbLV + cbLVChunkMost - 1 ) / cbLVChunkMost;

            TLS* const ptls = Ptls();
            ptls->threadstats.cNodesFlagDeleted += 1 + cChunks;
            ptls->threadstats.cbNodesFlagDeleted += kdf.key.Cb() + kdf.data.Cb() + cChunks * sizeof( LVKEY_BUFFER ) + cbLV;
        }
    }
    else
    {
        Assert( fLVReference == fLV );

        //  long value may already be in the process of being
        //  modified for a specific record.  This can only
        //  occur if the long value reference is 1.  If the reference
        //  is 1, then check the root for any version, committed
        //  or uncommitted.  If version found, then burst copy of
        //  old version for caller record.
        const LONG lDelta   = 1;
        err = ErrDIRDelta< LONG >(
                pfucbLV,
                OffsetOf( LVROOT, ulReference ),
                lDelta,
                NULL,
                fDIRNull | fDIRDeltaDeleteDereferencedLV );
        if ( JET_errWriteConflict == err )
        {
            //  we lost the page during the write conflict
            Call( ErrDIRGet( pfucbLV ) );
            Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );
        }
    }

HandleError:
    // discard temporary FUCB
    Assert( pfucbNil != pfucbLV );
    DIRCloseLongRoot( pfucb );

    return err;
}

ERR ErrRECDeleteLV_LegacyVersioned( FUCB *pfucbLV, const ULONG ulLVDeleteOffset, const DIRFLAG dirflag )
//
//  Given a FUCB set to the root of a LV, delete from ulLVDeleteOffset to end of LV.
//  NOTE: This version of LV delete is being obsoleted in favor of the synchronouscleanup version
//  (ErrRECDeleteLV_SynchronousCleanup).
//
{
    ASSERT_VALID( pfucbLV );
    Assert( FAssertLVFUCB( pfucbLV ) || g_fRepair );
    Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucbLV->kdfCurr.data.Cb() || g_fRepair );
    Assert( FIsLVRootKey( pfucbLV->kdfCurr.key ) || g_fRepair );

    ERR         err             = JET_errSuccess;
    ULONG       cChunksDeleted  = 0;
    LvId        lidDelete;

    LidFromKey( &lidDelete, pfucbLV->kdfCurr.key );

    if ( ulLVDeleteOffset > 0 )
    {
        Assert( !g_fRepair );
        Assert( fDIRNull == dirflag );

        //  we're going to truncate the LV at the specified offset, which should
        //  be on a chunk boundary
        //
        Assert( 0 == ulLVDeleteOffset % pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() );

        //  the caller should already have updated the size of the LV accordingly,
        //  which should be transacted with this operation
        //
        Assert( ( (LVROOT *)( pfucbLV->kdfCurr.data.Pv() ) )->ulSize == ulLVDeleteOffset );
        Assert( pfucbLV->ppib->Level() > 0 );

        //  now go to the offset and start deleting from that point
        //
        Call( ErrDIRDownLVData( pfucbLV, lidDelete, ulLVDeleteOffset, dirflag ) );
    }
    else
    {
        PERFOpt( PERFIncCounterTable( cLVDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
    }

    //  delete each chunk
    for( ; ; )
    {
        Call( ErrDIRDelete( pfucbLV, dirflag ) );

        PERFOpt( PERFIncCounterTable( cLVChunkDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
        cChunksDeleted++;

        //  if we're being called from DELETELVTASK::ErrExecuteDbTask(),
        //  we need to be careful not to attempt to move to the
        //  next chunk if it's already been deleted, because
        //  ErrBTNext() will be forced to navigate through
        //  all the deleted nodes (and even worse, pre-read
        //  will eventually be turned off because we don't
        //  refresh the preread buffer if we're on a deleted
        //  node)
        //
        Assert( 0 == ulLVDeleteOffset || cChunksDeleted <= DELETELVTASK::g_cpgLVDataToDeletePerTrx );
        if ( ulLVDeleteOffset > 0
            && cChunksDeleted >= DELETELVTASK::g_cpgLVDataToDeletePerTrx )
        {
#ifdef DEBUG
            //  next chunk should either be a different lid or
            //  the same lid but deleted (or otherwise not visible)
            //
            const ERR   errT    = ErrDIRNext( pfucbLV, fDIRSameLIDOnly );
            Assert( JET_errNoCurrentRecord == errT );
#endif
            break;
        }

        //  passing fDIRSameLIDOnly also ensures that ErrBTNext()
        //  won't scan too many pages, since it will err out on
        //  the first node where the lid doesn't match, or where
        //  the lid matches but the node is not visible
        //
        err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly );
        if ( err < JET_errSuccess )
        {
            if ( JET_errNoCurrentRecord == err )
            {
                err = JET_errSuccess;   // No more LV chunks. We're done.
            }
            goto HandleError;
        }
        CallS( err );       // Warnings not expected.

        //  make sure we are still on the same long value

        LvId lidT;
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if ( lidDelete != lidT )
        {
            //  ErrDIRNext should have returned JET_errNoCurrentRecord
            LVReportAndTrapCorruptedLV( pfucbLV, lidDelete, L"0c37a830-b7b6-4e60-ab81-0e45c8184ae1" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }
    }

    //  verify return value
    CallS( err );

HandleError:
    Assert( JET_errNoCurrentRecord != err );
    Assert( JET_errRecordNotFound != err );
    Assert( pfucbNil != pfucbLV );
    return err;
}

ERR ErrRECDeleteLV_SynchronousCleanup( FUCB *pfucbLV, const ULONG cChunksToDelete )
//
//  Given a FUCB set to the root of a LV, flag-delete and bt-delete cChunksToDelete from the LV.
//
{
    ASSERT_VALID( pfucbLV );
    Assert( FAssertLVFUCB( pfucbLV ) || g_fRepair );
    Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucbLV->kdfCurr.data.Cb() || g_fRepair );
    Assert( FIsLVRootKey( pfucbLV->kdfCurr.key ) || g_fRepair );

    ERR         err             = JET_errSuccess;
    ULONG       cChunksDeleted  = 0;
    LvId        lidDelete;
    BOOL        fDeletingRoot   = fFalse;
    DIRFLAG     dirflag         = fDIRNoVersion;
    DIRFLAG     dirnextflag     = fDIRSameLIDOnly;

    LidFromKey( &lidDelete, pfucbLV->kdfCurr.key );
    
    PERFOpt( PERFIncCounterTable( cLVDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    // On each iteration, navigate to the root and delete the next chunk
    // On the last iteration, don't go to the next chunk and just delete the root
    for( cChunksDeleted = 0; cChunksDeleted < cChunksToDelete || cChunksToDelete == 0; cChunksDeleted++ )
    {

        // ErrBTDelete leaves currency on PgnoFDP so re-establish on LVROOT
        Call( ErrDIRDownLVRootPreread( pfucbLV, lidDelete, dirflag, 0, fFalse ) );

        //  passing fDIRSameLIDOnly also ensures that ErrBTNext()
        //  won't scan too many pages, since it will err out on
        //  the first node where the lid doesn't match, or where
        //  the lid matches but the node is not visible
        err = ErrDIRNext( pfucbLV, dirnextflag );
        if( JET_errNoCurrentRecord == err )
        {
            // If this is come sooner than we expected, check again with fDIRAll to make 
            // sure there aren't remnants of the LV leftover. This can happen if a prior 
            // attempt to delete this LV got interrupted
            if( !( dirnextflag & fDIRAllNode ) && cChunksDeleted < cChunksToDelete - 1 )
            {
                // Add the fDIRAllNode and retry this chunk
                dirnextflag |= fDIRAllNode;
                cChunksDeleted -= 1;
                continue;
            }
            else
            {
                // Reposition on the root and delete it with versioning (it's locked, so can't do BTDelete)
                fDeletingRoot = fTrue;
                dirflag = fDIRNull;
                Call( ErrDIRDownLVRootPreread( pfucbLV, lidDelete, dirflag, 0, fFalse ) );
            }
        }
        else if( err < JET_errSuccess )
        {
            Call( err );
        }
        // Warnings not expected
        CallS( err );

        //  make sure we are still on the same long value
        LvId lidT;
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if( lidDelete != lidT )
        {
            //  ErrDIRNext should have returned JET_errNoCurrentRecord
            LVReportAndTrapCorruptedLV( pfucbLV, lidDelete, L"d6d29ea0-caad-41c5-8310-92956573d89c" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        // Flag-delete the node
        Call( ErrDIRDelete( pfucbLV, dirflag ) );

        // For the root we deleted with versioning, so go ahead
        // and bail now
        if( fDeletingRoot )
        {
            Assert( fDIRNull == dirflag );
            break;
        }

        Call( ErrFaultInjection( 39454 ) );

        // Remove the node now
        Call( ErrBTDelete( pfucbLV, pfucbLV->bmCurr ) );

        PERFOpt( PERFIncCounterTable( cLVChunkDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
    }

HandleError:
    Assert( pfucbNil != pfucbLV );
    return err;
}

//  ================================================================
ERR ErrRECAffectLongFieldsInWorkBuf( FUCB *pfucb, LVAFFECT lvaffect, ULONG cbThreshold )
//  ================================================================
{
    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( !pfucb->dataWorkBuf.FNull() );
    Assert( lvaffectSeparateAll == lvaffect || lvaffectReferenceAll == lvaffect );

    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( ptdbNil != pfucb->u.pfcb->Ptdb() );

    ERR             err;
    VOID *          pvWorkBufSav    = NULL;
    const ULONG     cbWorkBufSav    = pfucb->dataWorkBuf.Cb();

    if ( cbThreshold == 0 )
    {
        cbThreshold = LvId::CbLidFromCurrFormat( pfucb );
    }

    AssertDIRNoLatch( pfucb->ppib );

    AssertSz( pfucb->ppib->Level() > 0, "LV update attempted at level 0" );
    if ( 0 == pfucb->ppib->Level() )
    {
        err = ErrERRCheck( JET_errNotInTransaction );
        return err;
    }

    CallR( ErrDIRBeginTransaction( pfucb->ppib, 44837, NO_GRBIT ) );

    Assert( NULL != pfucb->dataWorkBuf.Pv() );
    Assert( cbWorkBufSav >= REC::cbRecordMin );
    Assert( cbWorkBufSav <= (SIZE_T)REC::CbRecordMost( pfucb ) );
    if ( cbWorkBufSav < REC::cbRecordMin || cbWorkBufSav > (SIZE_T)REC::CbRecordMost( pfucb ) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    if ( lvaffectSeparateAll == lvaffect )
    {
        BFAlloc( bfasForDisk, &pvWorkBufSav );
        UtilMemCpy( pvWorkBufSav, pfucb->dataWorkBuf.Pv(), cbWorkBufSav );
        PERFOpt( PERFIncCounterTable( cRECForceSeparateAllLV, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
    }
    else
    {
        //  don't need to save off copy buffer because this is
        //  an InsertCopy and on failure, we will throw away the
        //  copy buffer
        Assert( lvaffectReferenceAll == lvaffect );
        Assert( FFUCBInsertCopyPrepared( pfucb ) );
        PERFOpt( PERFIncCounterTable( cRECRefAllSeparateLV, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
    }

{
    TAGFIELDS       tagfields( pfucb->dataWorkBuf );
    Call( tagfields.ErrAffectLongValuesInWorkBuf( pfucb, lvaffect, cbThreshold ) );
}
    
    Assert( !Pcsr( pfucb )->FLatched() );

    Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    FUCBSetTagImplicitOp( pfucb );

    if ( NULL != pvWorkBufSav )
        BFFree( pvWorkBufSav );

    return err;

HandleError:
    Assert( err < 0 );
    CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
    Assert( !Pcsr( pfucb )->FLatched() );

    if ( NULL != pvWorkBufSav )
    {
        //  restore original copy buffer, because any LV updates
        //  that happened got rolled back
        UtilMemCpy( pfucb->dataWorkBuf.Pv(), pvWorkBufSav, cbWorkBufSav );
        pfucb->dataWorkBuf.SetCb( cbWorkBufSav );
        BFFree( pvWorkBufSav );
    }

    return err;
}


//  ================================================================
ERR ErrRECDereferenceLongFieldsInRecord( FUCB *pfucb )
//  ================================================================
{
    ERR                 err;

    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( pfcbNil != pfucb->u.pfcb );

    //  only called by ErrIsamDelete(), which always begins a transaction
    Assert( pfucb->ppib->Level() > 0 );

    AssertDIRMaybeNoLatch( pfucb->ppib, pfucb );

    CallR( ErrDIRGet( pfucb ) );
    Expected( !pfucb->kdfCurr.data.FNull() );
    Assert( NULL != pfucb->kdfCurr.data.Pv() );
    Assert( pfucb->kdfCurr.data.Cb() >= REC::cbRecordMin );
    Assert( pfucb->kdfCurr.data.Cb() <= REC::CbRecordMost( pfucb ) );
    if ( pfucb->kdfCurr.data.Cb() < REC::cbRecordMin || pfucb->kdfCurr.data.Cb() > REC::CbRecordMost( pfucb ) )
    {
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

    PERFOpt( PERFIncCounterTable( cRECDerefAllSeparateLV, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );

    {
    TAGFIELDS   tagfields( pfucb->kdfCurr.data );
    Call( tagfields.ErrDereferenceLongValuesInRecord( pfucb ) );
    }

HandleError:
    if ( Pcsr( pfucb )->FLatched() )
    {
        CallS( ErrDIRRelease( pfucb ) );
    }
    AssertDIRMaybeNoLatch( pfucb->ppib, pfucb );
    return err;
}



//  ****************************************************************
//  Functions used by compact to scan long value tree
//      ErrCMPGetFirstSLongField
//      ErrCMPGetNextSLongField
//  are used to scan the long value root and get lid. CMPRetrieveSLongFieldValue
//  is used to scan the long value. Due to the way the long value tree is
//  organized, we use the calling sequence (see CMPCopyLVTree in sortapi.c)
//      ErrCMPGetSLongFieldFirst
//      loop
//          loop to call CMPRetrieveSLongFieldValue to get the whole long value
//      till ErrCMPGetSLongFieldNext return no current record
//      ErrCMPGetSLongFieldClose
//  ****************************************************************

LOCAL ERR ErrCMPGetReferencedSLongField(
    FUCB        *pfucbGetLV,
    LvId        *plid,
    LVROOT2     *plvroot )
{
    ERR         err;
    const LvId  lidPrevLV   = *plid;
    BOOL fEncrypted = fFalse;

#ifdef DEBUG
    ULONG       ulSizeCurr  = 0;
    ULONG       ulSizeSeen  = 0;
    LvId        lidPrevNode = *plid;

    LVCheckOneNodeWhileWalking( pfucbGetLV, plid, &lidPrevNode, &ulSizeCurr, &ulSizeSeen );
#endif

    forever
    {
        if ( !FIsLVRootKey( pfucbGetLV->kdfCurr.key ) ||
             ( sizeof(LVROOT) != pfucbGetLV->kdfCurr.data.Cb() && sizeof(LVROOT2) != pfucbGetLV->kdfCurr.data.Cb() ) )
        {
            LVReportAndTrapCorruptedLV( pfucbGetLV, *plid, L"2d6f879b-087b-4f3b-8721-876709d8bae3" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        LidFromKey( plid, pfucbGetLV->kdfCurr.key );
        if ( *plid <= lidPrevLV )
        {
            //  lids are monotonically increasing
            LVReportAndTrapCorruptedLV( pfucbGetLV, *plid, L"641b46b0-c426-4b55-b2b1-82cc7aea1743" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        const ULONG ulRefcount  = (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulReference;
        if ( ulRefcount > 0 && !FPartiallyDeletedLV( ulRefcount ) )
        {
            plvroot->ulSize = (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulSize;
            plvroot->ulReference = ulRefcount;
            plvroot->fFlags = 0;
            fEncrypted = fFalse;
            if ( pfucbGetLV->kdfCurr.data.Cb() == sizeof(LVROOT2) )
            {
                plvroot->fFlags = (reinterpret_cast<LVROOT2*>( pfucbGetLV->kdfCurr.data.Pv() ))->fFlags;
                fEncrypted = FLVEncrypted( plvroot->fFlags );
            }
            err = ErrDIRRelease( pfucbGetLV );
            Assert( JET_errNoCurrentRecord != err );
            Assert( JET_errRecordNotFound != err );
            break;
        }

        // Skip LV's with refcount == 0 or refcount == ulMax
        do
        {
            Call( ErrDIRNext( pfucbGetLV, fDIRNull ) );

            Call( ErrLVCheckDataNodeOfLid( pfucbGetLV, *plid ) );

#ifdef DEBUG
            if ( ulRefcount > 0 && !FPartiallyDeletedLV( ulRefcount ) && !fEncrypted )
            {
                LVCheckOneNodeWhileWalking( pfucbGetLV, plid, &lidPrevNode, &ulSizeCurr, &ulSizeSeen );
            }
#endif
        }
        while ( wrnLVNoMoreData != err );

    }   // forever


HandleError:
    return err;
}


//  ================================================================
ERR ErrCMPGetSLongFieldFirst(
    FUCB    *pfucb,
    FUCB    **ppfucbGetLV,
    LvId    *plid,
    LVROOT2 *plvroot )
//  ================================================================
{
    ERR     err         = JET_errSuccess;
    FUCB    *pfucbGetLV = pfucbNil;

    Assert( pfucb != pfucbNil );

    //  open cursor on LONG

    CallR( ErrDIROpenLongRoot( pfucb, &pfucbGetLV, fFalse ) );
    if ( wrnLVNoLongValues == err )
    {
        Assert( pfucbNil == pfucbGetLV );
        *ppfucbGetLV = pfucbNil;
        return ErrERRCheck( JET_errRecordNotFound );
    }

    Assert( pfucbNil != pfucbGetLV );
    Assert( FFUCBLongValue( pfucbGetLV ) );

    // seek to first long field instance

    DIB     dib;
    dib.dirflag         = fDIRNull;
    dib.pos             = posFirst;
    Call( ErrDIRDown( pfucbGetLV, &dib ) );

    err = ErrCMPGetReferencedSLongField( pfucbGetLV, plid, plvroot );
    Assert( JET_errRecordNotFound != err );
    if ( err < 0 )
    {
        if ( JET_errNoCurrentRecord == err )
            err = ErrERRCheck( JET_errRecordNotFound );
        goto HandleError;
    }

    *ppfucbGetLV = pfucbGetLV;

    return err;


HandleError:
    //  discard temporary FUCB

    if ( pfucbGetLV != pfucbNil )
    {
        DIRClose( pfucbGetLV );
    }

    return err;
}


//  ================================================================
ERR ErrCMPGetSLongFieldNext(
    FUCB    *pfucbGetLV,
    LvId    *plid,
    LVROOT2 *plvroot )
//  ================================================================
{
    ERR     err = JET_errSuccess;

    Assert( pfucbNil != pfucbGetLV );

    //  move to next long field instance

    CallR( ErrDIRNext( pfucbGetLV, fDIRNull ) );

    err = ErrCMPGetReferencedSLongField( pfucbGetLV, plid, plvroot );
    Assert( JET_errRecordNotFound != err );

    return err;
}

//  ================================================================
VOID CMPGetSLongFieldClose( FUCB *pfucbGetLV )
//  ================================================================
{
    Assert( pfucbGetLV != pfucbNil );
    DIRClose( pfucbGetLV );
}

//  ================================================================
ERR ErrCMPRetrieveSLongFieldValueByChunk(
    FUCB        *pfucbGetLV,        //  pfucb must be on the LV root node.
    const LvId  lid,
    const ULONG cbTotal,            //  Total LV data length.
    ULONG       ibLongValue,        //  starting offset.
    BYTE        *pbBuf,
    const ULONG cbMax,
    ULONG       *pcbReturnedPhysical )  //  Total returned byte count
//  ================================================================
//
//  Returns one chunk of the LV. The data is _not_ decompressed.
//
//-
{
    ERR         err;

    *pcbReturnedPhysical = 0;

    //  We must be on LVROOT if ibGraphic == 0
#ifdef DEBUG
    if ( 0 == ibLongValue )
    {
        CallS( ErrDIRGet( pfucbGetLV ) );
        AssertLVRootNode( pfucbGetLV, lid );
        Assert( cbTotal == (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulSize );
    }
#endif

    // For this to work properly, ibLongValue must always point to the
    // beginning of a chunk, and the buffer passed in must be big enough
    // to hold one chunk.
    const LONG cbLVChunkMost = pfucbGetLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    Assert( ibLongValue % cbLVChunkMost == 0 );

    CallR( ErrDIRNext( pfucbGetLV, fDIRNull ) );

    //  make sure we are still on the same long value
    Call( ErrLVCheckDataNodeOfLid( pfucbGetLV, lid ) );
    if ( wrnLVNoMoreData == err )
    {
        //  ran out of data before we were supposed to
        LVReportAndTrapCorruptedLV( pfucbGetLV, lid, L"c25a9a09-0ce6-45a7-b4d1-e03b4eab941d" );
        Error( ErrERRCheck( JET_errLVCorrupted ) );
    }

    if( cbMax < (ULONG)pfucbGetLV->kdfCurr.data.Cb() )
    {
        Call( ErrERRCheck( JET_errBufferTooSmall ) );
    }
    UtilMemCpy( pbBuf, pfucbGetLV->kdfCurr.data.Pv(), pfucbGetLV->kdfCurr.data.Cb() );

    *pcbReturnedPhysical = ULONG( pfucbGetLV->kdfCurr.data.Cb() );

    Assert( *pcbReturnedPhysical <= cbMax );

    err = JET_errSuccess;

HandleError:
    CallS( ErrDIRRelease( pfucbGetLV ) );
    return err;
}


ERR ErrCMPUpdateLVRefcount(
    FUCB        *pfucb,
    const LvId  lid,
    const ULONG ulRefcountOld,
    const ULONG ulRefcountNew )
{
    ERR         err;
    FUCB        *pfucbLV    = pfucbNil;

    CallR( ErrDIROpenLongRoot( pfucb ) );
    Assert( wrnLVNoLongValues != err );     // should only call this func if we know LV's exist

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    Call( ErrDIRDownLVRoot( pfucbLV, lid, fDIRNull ) );
    Assert( (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulReference
        == ulRefcountOld );

    if ( 0 == ulRefcountNew )
    {
        Assert( ulRefcountOld != ulRefcountNew );

        Call(ErrRECDeleteLV_LegacyVersioned(pfucbLV, 0, fDIRNull));
    }
#ifdef DEBUG
    //  in non-DEBUG, this check is performed before calling this function
    else if ( ulRefcountOld == ulRefcountNew )
    {
        //  refcount is already correct. Do nothing.
    }
#endif
    else
    {
        //  update refcount with correct count
        LVROOT2 lvroot = { 0 };
        DATA    data;

        data.SetPv( &lvroot );
#pragma warning(suppress: 26015)
        UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );
        lvroot.ulReference = ulRefcountNew;
        data.SetCb( lvroot.fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );
        Call( ErrDIRReplace( pfucbLV, data, fDIRNull ) );
    }

HandleError:
    Assert( pfucbNil != pfucbLV );
    DIRCloseLongRoot( pfucb );

    return err;
}



#ifdef DEBUG

VOID LVCheckOneNodeWhileWalking(
    FUCB    *pfucbLV,
    LvId    *plidCurr,
    LvId        *plidPrev,
    ULONG   *pulSizeCurr,
    ULONG   *pulSizeSeen )
{
    //  make sure we are still on the same long value
    if ( FIsLVRootKey( pfucbLV->kdfCurr.key ) )
    {
        //  we must be on the first node of a new long value
        Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucbLV->kdfCurr.data.Cb() );

        //  get the new LID
        LidFromKey( plidCurr, pfucbLV->kdfCurr.key );
        Assert( *plidCurr > *plidPrev );
        *plidPrev = *plidCurr;

        //  get the new size. make sure we saw all of the previous LV
        Assert( *pulSizeCurr == *pulSizeSeen );
        *pulSizeCurr = (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulSize;
        *pulSizeSeen = 0;

        //  its O.K. to have an unreferenced LV (it should get cleaned up), but we may want
        //  to set a breakpoint
        if ( 0 == ( reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ) )->ulReference )
        {
            /// AssertSz( fFalse, "Unreferenced long value" );
            Assert( 0 == *pulSizeSeen );    //  a dummy statement to set a breakpoint on
        }

    }
    else
    {
        Assert( *plidCurr > lidMin );

        //  check that we are still on our own lv.
        LvId    lid         = lidMin;
        ULONG   ulOffset    = 0;
        LidOffsetFromKey( &lid, &ulOffset, pfucbLV->kdfCurr.key );
        Assert( lid == *plidCurr );
        const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
        Assert( ulOffset % cbLVChunkMost == 0 );
        Assert( ulOffset == *pulSizeSeen );

        //  keep track of how much of the LV we have seen
        //  the nodes should be of maximum size, except at the end
        ULONG cbChunk;
        CallS( ErrLVIGetDataSize( pfucbLV, pfucbLV->kdfCurr.key, pfucbLV->kdfCurr.data, fFalse, *pulSizeCurr, &cbChunk ) );
        *pulSizeSeen += cbChunk;
        Assert( *pulSizeSeen <= *pulSizeCurr );
        Assert( (ULONG)cbLVChunkMost == cbChunk
            || *pulSizeSeen == *pulSizeCurr );

    }
}

#endif  //  DEBUG


//  ================================================================
RECCHECKLV::RECCHECKLV( TTMAP& ttmap, const REPAIROPTS * m_popts, LONG cbLVChunkMost ) :
//  ================================================================
    m_ttmap( ttmap ),
    m_popts( m_popts ),
    m_cbLVChunkMost( cbLVChunkMost ),
    m_ulSizeCurr( 0 ),
    m_ulReferenceCurr( 0 ),
    m_ulSizeSeen( 0 ),
    m_fEncrypted( fFalse ),
    m_lidCurr( lidMin )
{
}


//  ================================================================
RECCHECKLV::~RECCHECKLV()
//  ================================================================
{
}


//  ================================================================
ERR RECCHECKLV::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
//  ================================================================
{
    ERR err = JET_errSuccess;
    ERR wrn = JET_errSuccess;

    if ( FIsLVRootKey( kdf.key ) )
    {
        const LvId lidOld = m_lidCurr;

        LidFromKey( &m_lidCurr, kdf.key );

        if( sizeof(LVROOT) != kdf.data.Cb() && sizeof(LVROOT2) != kdf.data.Cb() )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (LVROOT data mismatch: expected %d bytes, got %d bytes\r\n",
                                        (_LID64)m_lidCurr, sizeof( LVROOT2 ), kdf.data.Cb() );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        if( m_lidCurr <= lidOld )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (LIDs are not increasing: 0x%I64x )\r\n", (_LID64)m_lidCurr, (_LID64)lidOld );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        if( m_ulSizeCurr != m_ulSizeSeen && m_ulReferenceCurr != 0 && !FPartiallyDeletedLV( m_ulReferenceCurr ) )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (LV too short: expected %d bytes, saw %d bytes)\r\n", (_LID64)lidOld, m_ulSizeCurr, m_ulSizeSeen );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        m_ulSizeCurr = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulSize;
        m_ulReferenceCurr = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulReference;
        m_fEncrypted = fFalse;
        if ( kdf.data.Cb() == sizeof(LVROOT2) &&
             FLVEncrypted( (reinterpret_cast<LVROOT2*>( kdf.data.Pv() ))->fFlags ) )
        {
            m_fEncrypted = fTrue;
        }
        m_ulSizeSeen = 0;

#ifdef SYNC_DEADLOCK_DETECTION
        COwner* const pownerSaved = Pcls()->pownerLockHead;
        Pcls()->pownerLockHead = NULL;
#endif  //  SYNC_DEADLOCK_DETECTION
        err = m_ttmap.ErrSetValue( m_lidCurr, m_ulReferenceCurr );
#ifdef SYNC_DEADLOCK_DETECTION
        Pcls()->pownerLockHead = pownerSaved;
#endif  //  SYNC_DEADLOCK_DETECTION
        Call( err );
    }
    else
    {
        if( lidMin == m_lidCurr )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (lid == lidmin)\r\n", (_LID64)m_lidCurr );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if( sizeof( LVKEY64 ) != kdf.key.Cb() && sizeof( LVKEY32 ) != kdf.key.Cb() )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (LVKEY size mismatch: expected %d or %d bytes, got %d bytes)\r\n",
                                        (_LID64)m_lidCurr, sizeof( LVKEY64 ), sizeof( LVKEY32 ), kdf.key.Cb() );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        //  check that we are still on our own lv.
        LvId    lid         = lidMin;
        ULONG   ulOffset    = 0;
        LidOffsetFromKey( &lid, &ulOffset, kdf.key );
        Assert( ulOffset % m_cbLVChunkMost == 0 );
        if( lid != m_lidCurr )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (lid changed to 0x%I64x without root, size is %d bytes)\r\n",
                                        (_LID64)m_lidCurr, (_LID64)lid, kdf.data.Cb() );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        if( ulOffset != m_ulSizeSeen && m_ulReferenceCurr != 0 && !FPartiallyDeletedLV( m_ulReferenceCurr ) )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (missing chunk: offset is %d, saw %d bytes, size is %d bytes)\r\n",
                                            (_LID64)m_lidCurr, ulOffset, m_ulSizeSeen, kdf.data.Cb() );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        if( 0 != ( ulOffset % m_cbLVChunkMost ) )
        {
            (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (incorrect offset: offset is %d, size is %d bytes)\r\n",
                                        (_LID64)m_lidCurr, ulOffset, kdf.data.Cb() );
            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }

        //  keep track of how much of the LV we have seen (cannot determine real size for encrypted LVs)
        ULONG cbDecompressed;
        if ( m_fEncrypted )
        {
            cbDecompressed = min( m_ulSizeCurr - m_ulSizeSeen, (ULONG)m_cbLVChunkMost );
        }
        else
        {
            Call( ErrLVIGetDataSize(
                        NULL,
                        kdf.key,
                        kdf.data,
                        fFalse,
                        m_ulSizeCurr,
                        &cbDecompressed,
                        m_cbLVChunkMost ) );
            if ( err == wrnRECCompressionScrubDetected )
            {
                if ( m_ulReferenceCurr != 0 && !FPartiallyDeletedLV( m_ulReferenceCurr ) )
                {
                    (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (scrubbed data on referenced LV: references %u,total size %d bytes, saw %d bytes)\r\n",
                                               (_LID64)m_lidCurr, m_ulReferenceCurr, m_ulSizeCurr, m_ulSizeSeen );
                    Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                }
                else
                {
                    //we can't assert anything about the consistency of this LV.
                    (*m_popts->pcprintfWarning)( "orphaned scrubbed LV(0x%I64x) detected (total size %d bytes, saw %d bytes). Offline defragmentation or database maintenance should fix this.\r\n",
                                                 (_LID64)m_lidCurr, m_ulSizeCurr, m_ulSizeSeen );
                    cbDecompressed = 0;
                    err = JET_errSuccess;
                }
            }
        }
        m_ulSizeSeen += cbDecompressed;

        if ( m_ulReferenceCurr != 0 && !FPartiallyDeletedLV( m_ulReferenceCurr ) )
        {
            if( m_ulSizeSeen > m_ulSizeCurr )
            {
                (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (LV too long: expected %d bytes, saw %d bytes)\r\n",
                                            (_LID64)m_lidCurr, m_ulSizeCurr, m_ulSizeSeen );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
            if( (ULONG)m_cbLVChunkMost != cbDecompressed && m_ulSizeSeen != m_ulSizeCurr )
            {
                (*m_popts->pcprintfError)( "corrupted LV(0x%I64x) (node is wrong size: node is %d bytes)\r\n",
                                            (_LID64)m_lidCurr, kdf.data.Cb() );
                Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
            }
        }
    }

HandleError:
    if( JET_errSuccess == err )
    {
        err = wrn;
    }
    return err;
}


//  ================================================================
ERR RECCHECKLV::ErrTerm()
//  ================================================================
//
//  was the last LV we saw complete?
//
//-
{
    ERR err = JET_errSuccess;

    if( m_ulSizeCurr != m_ulSizeSeen && m_ulReferenceCurr != 0 && !FPartiallyDeletedLV( m_ulReferenceCurr ) )
    {
        (*m_popts->pcprintfError)( "corrupted LV(%d) (LV too short: expected %d bytes, saw %d bytes)\r\n", (_LID64)m_lidCurr, m_ulSizeCurr, m_ulSizeSeen );
        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
    }

HandleError:
    return err;
}


//  ================================================================
RECCHECKLVSTATS::RECCHECKLVSTATS( LVSTATS * plvstats ) :
//  ================================================================
    m_plvstats( plvstats ),
    m_pgnoLastRoot( pgnoNull )
{
}


//  ================================================================
RECCHECKLVSTATS::~RECCHECKLVSTATS()
//  ================================================================
{
}


//  ================================================================
ERR RECCHECKLVSTATS::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
//  ================================================================
{
    if ( FIsLVRootKey( kdf.key ) )
    {
        LvId lid;
        LidFromKey( &lid, kdf.key );

        const ULONG ulSize      = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulSize;
        const ULONG ulReference = (reinterpret_cast<LVROOT*>( kdf.data.Pv() ))->ulReference;

        m_plvstats->cbLVTotal       += ulSize;
        m_plvstats->referenceTotal  += ulReference;

        if( lidMin == m_plvstats->lidMin )
        {
            m_plvstats->lidMin = lid;
        }
        m_plvstats->lidMin = min( lid, m_plvstats->lidMin );
        m_plvstats->lidMax = max( lid, m_plvstats->lidMax );

        ++(m_plvstats->clv );

        if( ulReference > 1 )
        {
            ++(m_plvstats->clvMultiRef );
            m_plvstats->cbLVMultiRefTotal += ulSize;
        }
        else if( 0 == ulReference )
        {
            ++(m_plvstats->clvNoRef );
        }

        m_plvstats->referenceMax = max( ulReference, m_plvstats->referenceMax );

        m_plvstats->cbLVMin = min( ulSize, m_plvstats->cbLVMin );
        m_plvstats->cbLVMax = max( ulSize, m_plvstats->cbLVMax );

        m_pgnoLastRoot  = pgno;
        m_lidLastRoot   = lid;
    }
    else
    {
        LvId    lid;
        ULONG   ulOffset;
        LidOffsetFromKey( &lid, &ulOffset, kdf.key );
        if( lid == m_lidLastRoot && 0 == ulOffset )
        {
            if( pgno == m_pgnoLastRoot )
            {
                ++(m_plvstats->clvUnfragmentedRoot);
            }
            else
            {
                ++(m_plvstats->clvFragmentedRoot);
            }
        }
        
    }

    return JET_errSuccess;
}


//  ================================================================
ERR ErrREPAIRCheckLV(
    FUCB * const pfucb,
    LvId * const plid,
    ULONG * const pulRefcount,
    ULONG * const pulSize,
    BOOL * const pfLVHasRoot,
    BOOL * const pfLVComplete,
    BOOL * const pfLVPartiallyScrubbed,
    BOOL * const pfDone )
//  ================================================================
{
    ERR err = JET_errSuccess;
    ULONG ulSizeSeen = 0;
    BOOL fEncrypted = fFalse;

    Call( ErrDIRGet( pfucb ) );

    *pfLVHasRoot            = fFalse;
    *pfLVComplete           = fFalse;
    *pfLVPartiallyScrubbed  = fFalse;
    *pfDone                 = fFalse;
    *pulRefcount            = 0;
    LidFromKey( plid, pfucb->kdfCurr.key );

    if ( !FIsLVRootKey( pfucb->kdfCurr.key )
        || ( sizeof( LVROOT ) != pfucb->kdfCurr.data.Cb()
           && sizeof( LVROOT2 ) != pfucb->kdfCurr.data.Cb() ) )
    {
        // no root, so no way to recover this LV
        goto HandleError;
    }
    else
    {
        *pfLVHasRoot = fTrue;
        *pulRefcount = (reinterpret_cast<LVROOT*>( pfucb->kdfCurr.data.Pv() ) )->ulReference;
        *pulSize = (reinterpret_cast<LVROOT*>( pfucb->kdfCurr.data.Pv() ) )->ulSize;
        if ( pfucb->kdfCurr.data.Cb() == sizeof( LVROOT2 ) )
        {
            fEncrypted = FLVEncrypted( (reinterpret_cast<LVROOT2*>( pfucb->kdfCurr.data.Pv() ) )->fFlags );
        }
    }

    for( ; ; )
    {
        err = ErrDIRNext( pfucb, fDIRNull );
        if( JET_errNoCurrentRecord == err )
        {
            break;
        }
        Call( err );

        LvId lidT;
        LidFromKey( &lidT, pfucb->kdfCurr.key );

        if( *plid != lidT )
        {
            //  we are on a new LV
            break;
        }

        if( !FIsLVChunkKey( pfucb->kdfCurr.key ) )
        {
            //  this would be a really strange corruption
            goto HandleError;
        }

        ULONG   ulOffset;
        LidOffsetFromKey( &lidT, &ulOffset, pfucb->kdfCurr.key );
        Assert( ulOffset % pfucb->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() == 0 );
        if( ( ulOffset != ulSizeSeen ) && !(*pfLVPartiallyScrubbed) )
        {
            //  we are missing a chunk
            goto HandleError;
        }

        ULONG cbDecompressed;
        // For encrypted columns, cannot verify that decompression/chunk-size is all right
        if ( fEncrypted )
        {
            cbDecompressed = min( (ULONG)pfucb->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost(), *pulSize - ulSizeSeen );
        }
        else
        {
            Call( ErrLVIGetDataSize(
                        pfucb,
                        pfucb->kdfCurr.key,
                        pfucb->kdfCurr.data,
                        fFalse,
                        *pulSize,
                        &cbDecompressed ) );

            if ( err == wrnRECCompressionScrubDetected )
            {
                *pfLVPartiallyScrubbed = fTrue;
                err = JET_errSuccess;
            }
        }

        //  we can't assert anything about the size-related consistency of partially scrubbed LVs.
        if ( !(*pfLVPartiallyScrubbed) )
        {
            ulSizeSeen += cbDecompressed;

            if( (ULONG)pfucb->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() != cbDecompressed
                && ulSizeSeen < *pulSize
                && *pfLVHasRoot )
            {
                //  all chunks in the middle should be the full LV chunk size
                goto HandleError;
            }

            if( ulSizeSeen > *pulSize
                && *pfLVHasRoot )
            {
                //  this LV is too long
                goto HandleError;
            }
        }
    }

    Assert( err >= JET_errSuccess || err == JET_errNoCurrentRecord );

    *pfDone = ( JET_errNoCurrentRecord == err );

    if( *pfLVHasRoot )
    {
        *pfLVComplete = ( ulSizeSeen == *pulSize );
    }
    else
    {
        *pfLVComplete = fFalse;
        *pulSize = ulSizeSeen;
        *pulRefcount = 1;
    }

    if( err >= JET_errSuccess )
    {
        Call( ErrDIRRelease( pfucb ) );
    }

    err = JET_errSuccess;

HandleError:
    return err;
}


//  ================================================================
LOCAL ERR ErrREPAIRDownLV( FUCB * pfucb, LvId lid )
//  ================================================================
//
//  This will search for a partial chunk of a LV
//
//-
{
    ERR err = JET_errSuccess;

    LVKEY_BUFFER    lvkey;
    BOOKMARK        bm;

    bm.data.Nullify();
    LVRootKeyFromLid( &lvkey, &bm.key, lid );

    DIB         dib;
    dib.dirflag = fDIRNull;
    dib.pos     = posDown;
    dib.pbm     = &bm;

    DIRUp( pfucb );
    err = ErrDIRDown( pfucb, &dib );
    Assert( JET_errRecordDeleted != err );

    const BOOL fFoundLess       = ( err == wrnNDFoundLess );
    const BOOL fFoundGreater    = ( err == wrnNDFoundGreater );
    const BOOL fFoundEqual      = ( !fFoundGreater && !fFoundLess && err >= 0 );
    Call( err );

    Assert( g_fRepair || !fFoundGreater );
    if( fFoundEqual )
    {
        //  we are on the node we want
    }
    else if( fFoundLess )
    {
        //  we are on a node that is less than our current node
        Assert( locBeforeSeekBM == pfucb->locLogical );
        Call( ErrDIRNext( pfucb, fDIRNull ) );
    }
    else if( fFoundGreater )
    {
        //  we are on a node with a greater key than the one we were seeking for
        //  this is actually the node we want to be on
        Assert( locAfterSeekBM == pfucb->locLogical );
        pfucb->locLogical = locOnCurBM;
    }

HandleError:
    return err;
}


//  ================================================================
ERR ErrREPAIRDeleteLV( FUCB * pfucb, LvId lid )
//  ================================================================
{
    Assert( g_fRepair );

    ERR err = JET_errSuccess;

    Call( ErrREPAIRDownLV( pfucb, lid ) );

#ifdef DEBUG
{
    LvId lidT;
    LidFromKey( &lidT, pfucb->kdfCurr.key );
    Assert( lidT == lid );
}
#endif  //  DEBUG

    Call(ErrRECDeleteLV_LegacyVersioned(pfucb, 0, fDIRNoVersion));

HandleError:
    return err;
}


//  ================================================================
ERR ErrREPAIRCreateLVRoot( FUCB * const pfucb, const LvId lid, const ULONG ulRefcount, const ULONG ulSize )
//  ================================================================
{

    Expected( fFalse );

    ERR err = JET_errSuccess;

    LVKEY_BUFFER lvkey;
    KEY key;
    LVRootKeyFromLid( &lvkey, &key, lid );

    LVROOT lvroot;
    lvroot.ulReference = ulRefcount;
    lvroot.ulSize = ulSize;

    DATA data;
    data.SetPv( &lvroot );
    data.SetCb( sizeof( LVROOT ) );

    err = ErrDIRInsert( pfucb, key, data, fDIRNull | fDIRNoVersion | fDIRNoLog, NULL );

    return err;
}


//  ================================================================
ERR ErrREPAIRNextLV( FUCB * pfucb, LvId lidCurr, BOOL * pfDone )
//  ================================================================
{
    ERR err = JET_errSuccess;
    const LvId lidNext = lidCurr + 1;

    err = ErrREPAIRDownLV( pfucb, lidNext );
    if( JET_errNoCurrentRecord == err
        || JET_errRecordNotFound == err )
    {
        *pfDone = fTrue;
        return JET_errSuccess;
    }
    Call( err );

#ifdef DEBUG
{
    LvId lidT;
    LidFromKey( &lidT, pfucb->kdfCurr.key );
    Assert( lidT >= lidNext );
}
#endif  //  DEBUG

HandleError:
    return err;
}


//  ================================================================
ERR ErrREPAIRUpdateLVRefcount(
    FUCB        * const pfucbLV,
    const LvId  lid,
    const ULONG ulRefcountOld,
    const ULONG ulRefcountNew )
//  ================================================================
{
    Assert( ulRefcountOld != ulRefcountNew );

    ERR         err;

    LVROOT2 lvroot = { 0 };
    DATA    data;

    Call( ErrDIRDownLVRoot( pfucbLV, lid, fDIRNull ) );
    Assert( (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulReference
        == ulRefcountOld );

    data.SetPv( &lvroot );
#pragma warning(suppress: 26015)
    UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );
    lvroot.ulReference = ulRefcountNew;
    data.SetCb( lvroot.fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );
    Call( ErrDIRReplace( pfucbLV, data, fDIRNull ) );

HandleError:
    return err;
}

#ifdef MINIMAL_FUNCTIONALITY
#else

//  ================================================================
LOCAL ERR ErrSCRUBIZeroLV(  PIB * const     ppib,
                            const IFMP      ifmp,
                            CSR * const     pcsr,
                            const INT       iline,
                            const LvId      lid,
                            const ULONG     ulSize,
                            const BOOL      fCanChangeCSR )
//  ================================================================
{
    ERR             err;
    KEYDATAFLAGS    kdf;

    for ( INT ilineT = iline; ; ilineT++ )
    {
        if( ilineT == pcsr->Cpage().Clines() )
        {
            const PGNO pgnoNext = pcsr->Cpage().PgnoNext();
            if( NULL == pgnoNext )
            {
                //  end of the tree
                break;
            }

            auto tc = TcCurr( );
            Expected( tc.iorReason.Iort() == iortScrubbing );

            if ( !fCanChangeCSR )
            {
                //  we need to keep the root of the LV latched for further processing
                //  use a new CSR to venture onto new pages
                CSR csrNext;
                CallR( csrNext.ErrGetRIWPage( ppib, ifmp, pgnoNext ) );
                csrNext.UpgradeFromRIWLatch();

                BFDirty( csrNext.Cpage().PBFLatch(), bfdfDirty, tc );

                err = ErrSCRUBIZeroLV( ppib, ifmp, &csrNext, 0, lid, ulSize, fTrue );

                csrNext.ReleasePage();
                csrNext.Reset();

                return err;
            }
            else
            {
                //  the page should be write latched downgrade to a RIW latch
                //  ErrSwitchPage will RIW latch the next page
                pcsr->Downgrade( latchRIW );

                CallR( pcsr->ErrSwitchPage( ppib, ifmp, pgnoNext ) );
                pcsr->UpgradeFromRIWLatch();

                BFDirty( pcsr->Cpage().PBFLatch(), bfdfDirty, tc );

                ilineT = 0;
            }
        }

        NDIGetKeydataflags( pcsr->Cpage(), ilineT, &kdf );

        //  check to see if we have reached the next LVROOT
        if( FIsLVRootKey( kdf.key ) )
        {
            if( sizeof( LVROOT ) != kdf.data.Cb() && sizeof( LVROOT2 ) != kdf.data.Cb() )
            {
                //  We need to skip this check for flag-deleted nodes because they might have been shrunk
                //  by runtime cleanup. See comment in ErrBTISPCDeleteNodes(). Databases created with this
                //  version of ESE should not have this problem, because that code is now fixed, but
                //  scrubbing older databases may hit this.
                //  It is OK to remove this ExpectedSz() if we hit problems in Windows upgrade testing.
                ExpectedSz( fFalse, "Invalid LVROOT size (%d bytes).", kdf.data.Cb() );

                if ( !FNDDeleted( kdf ) )
                {
                    LvId lidT;
                    LidFromKey( &lidT, kdf.key );
                    LVReportAndTrapCorruptedLV2( PinstFromIfmp( ifmp ), g_rgfmp[ifmp].WszDatabaseName(), "", pcsr->Pgno(), lidT, L"5482999a-362e-48e5-bd04-a09c934aec73" );
                    return ErrERRCheck( JET_errLVCorrupted );
                }
            }
            //  reached the start of the next long-value
            break;
        }

        //  make sure we are on a LVDATA node
        else if( !FIsLVChunkKey( kdf.key ) )
        {
            LVReportAndTrapCorruptedLV2( PinstFromIfmp( ifmp ), g_rgfmp[ifmp].WszDatabaseName(), "", pcsr->Pgno(), 0, L"7b30e61f-46c6-48d0-9e3d-3d5c6c6454f0" );
            return ErrERRCheck( JET_errLVCorrupted );
        }

        //  make sure LIDs haven't changed
        LvId lidT;
        LidFromKey( &lidT, kdf.key );
        if( lid != lidT )
        {
            //  this can happen if the next LV is being deleted
            break;
        }

        //  make sure the LV isn't too long
        ULONG ulOffset;
        OffsetFromKey( &ulOffset, kdf.key );
        if( ulOffset > ulSize && !FNDDeleted( kdf ) )
        {
            LVReportAndTrapCorruptedLV2( PinstFromIfmp( ifmp ), g_rgfmp[ifmp].WszDatabaseName(), "", pcsr->Pgno(), lidT, L"8ab78a02-ca42-4de4-9d89-df3111a367fa" );
            return ErrERRCheck( JET_errLVCorrupted );
        }

        //  we are on a data node of our LV
        //  There is similar code in NDScrubOneUsedPage() and a comment on why we do this.
        Assert( pcsr->FDirty() );
        CallS( ErrRECScrubLVChunk( kdf.data, chSCRUBLegacyLVChunkFill ) );
    }

    return JET_errSuccess;
}


//  ================================================================
ERR ErrSCRUBZeroLV( PIB * const ppib,
                    const IFMP ifmp,
                    CSR * const pcsr,
                    const INT iline )
//  ================================================================
{
    CSR csrT;

    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
    Assert( sizeof( LVROOT ) == kdf.data.Cb() || sizeof( LVROOT2 ) == kdf.data.Cb() );

    const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
    Assert( 0 == plvroot->ulReference );

    const ULONG ulSize = plvroot->ulSize;
    LvId lid;
    LidFromKey( &lid, kdf.key );

    return ErrSCRUBIZeroLV( ppib, ifmp, pcsr, iline+1, lid, ulSize, fFalse );
}

//  These next two functions work together on the EVAL_LV_PAGE_CTX from
//  the callbacks in ErrBTUTLAcross().

ERR ErrAccumulateLvPageData(
    const PGNO pgno, const ULONG iLevel, const CPAGE * pcpage, void * pvCtx
    )
{
    EVAL_LV_PAGE_CTX * pbtsLvCtx = (EVAL_LV_PAGE_CTX *) pvCtx;

    Assert( pbtsLvCtx );
    Assert( pbtsLvCtx->pLvData );

    Assert( pbtsLvCtx->pgnoCurrent != pgnoNull || !pbtsLvCtx->fStarted );
    if ( !pbtsLvCtx->fStarted )
    {
        Assert( pbtsLvCtx->pgnoLast == pgnoNull );
        Assert( pbtsLvCtx->pgnoCurrent == pgnoNull );
    }
    pbtsLvCtx->fStarted = fTrue;

    //  rotate pgnos ...
    pbtsLvCtx->pgnoLast = pbtsLvCtx->pgnoCurrent;
    pbtsLvCtx->pgnoCurrent = pgno;

    return JET_errSuccess;
}

ERR ErrAccumulateLvNodeData(
    const CPAGE::PGHDR * const  ppghdr,
    INT                         itag,
    DWORD                       fNodeFlags,
    const KEYDATAFLAGS * const  pkdf,
    void *                      pvCtx
    )
{
    ERR err = JET_errSuccess;
    EVAL_LV_PAGE_CTX * pbtsLvCtx = (EVAL_LV_PAGE_CTX *)pvCtx;
    LvId lidCurr;

    Assert( pbtsLvCtx );
    Assert( pbtsLvCtx->pLvData );
    Assert( pbtsLvCtx->fStarted );
    Assert( pbtsLvCtx->pgnoCurrent );

    if ( NULL == pkdf )
    {
        // we don't care about the external header ...
        Assert( itag == 0 );
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( pkdf->fFlags == fNodeFlags );

    LidFromKey( &lidCurr, pkdf->key );

    if ( pbtsLvCtx->pLvData->lidMax < lidCurr )
    {
        pbtsLvCtx->pLvData->lidMax = lidCurr;
    }

    if ( !( fNodeFlags & fNDDeleted ) )
    {

        if ( FIsLVRootKey( pkdf->key ) )
        {
            //  We have an LV Root.
            if ( pkdf->data.Cb() != sizeof( LVROOT ) && pkdf->data.Cb() != sizeof( LVROOT2 ) )
            {
                AssertSz( fFalse, "Corrupt LVROOT data?" );
                pbtsLvCtx->pLvData->cCorruptLVs++;
                goto HandleError;
            }

            pbtsLvCtx->lidCurrent = lidCurr;
            pbtsLvCtx->cbCurrent = ( (LVROOT*)pkdf->data.Pv() )->ulSize;
            pbtsLvCtx->cRefsCurrent = ( (LVROOT*)pkdf->data.Pv() )->ulReference;
            pbtsLvCtx->fEncrypted = fFalse;
            if ( pkdf->data.Cb() == sizeof( LVROOT2 ) &&
                 FLVEncrypted( ( (LVROOT2*)pkdf->data.Pv() )->fFlags ) )
            {
                pbtsLvCtx->fEncrypted = fTrue;
            }

            if( FPartiallyDeletedLV( pbtsLvCtx->cRefsCurrent ) )
            {
                pbtsLvCtx->pLvData->cPartiallyDeletedLVs++;
            }

            pbtsLvCtx->pgnoLvStart = pbtsLvCtx->pgnoCurrent;
            pbtsLvCtx->cbAccumLogical = 0;
            pbtsLvCtx->cbAccumActual = 0;

            pbtsLvCtx->cpgAccum = 0;

            pbtsLvCtx->cRunsAccessed = 0;
            pbtsLvCtx->cpgAccessed = 0;

            // For the purposes of counting refs, treat a partially deleted LV as having no refs
            pbtsLvCtx->pLvData->cLVRefs += FPartiallyDeletedLV( pbtsLvCtx->cRefsCurrent ) ? 0 : pbtsLvCtx->cRefsCurrent;
        }
        else if ( pkdf->key.Cb() == sizeof( LVKEY64 ) || pkdf->key.Cb() == sizeof( LVKEY32 ) )
        {
            //  We have an LV Chunk.
            LvId lidT;
            ULONG ibOffset;
            LidOffsetFromKey( &lidT, &ibOffset, pkdf->key );
            Assert( ibOffset % pbtsLvCtx->pLvData->cbLVChunkMax == 0 );
            Assert( lidT == lidCurr );

            if ( pbtsLvCtx->lidCurrent != lidCurr )
            {
                AssertSz( fFalse, "Corrupt LV, non-matching LVROOT / LVCHUNK?" );
                pbtsLvCtx->pLvData->cCorruptLVs++;
                goto HandleError;
            }
            if ( pbtsLvCtx->cbAccumLogical != ibOffset && !FPartiallyDeletedLV( pbtsLvCtx->cRefsCurrent ) )
            {
                AssertSz( fFalse, "Corrupt LV, LVCHUNK with unexpected offset" );
                pbtsLvCtx->pLvData->cCorruptLVs++;
                goto HandleError;
            }
            if ( pbtsLvCtx->cbAccumLogical >= pbtsLvCtx->cbCurrent )
            {
                AssertSz( fFalse, "Corrupt LV, extra chunks" );
                pbtsLvCtx->pLvData->cCorruptLVs++;
                goto HandleError;
            }

            ULONG cbUncompressedExpected = min( pbtsLvCtx->cbCurrent - pbtsLvCtx->cbAccumLogical,
                (ULONG)pbtsLvCtx->pLvData->cbLVChunkMax );
            if ( !pbtsLvCtx->fEncrypted )
            {
                ULONG cbUncompressed;
                CallS( ErrLVIGetDataSize( NULL, pkdf->key, pkdf->data, fFalse, pbtsLvCtx->cbCurrent, &cbUncompressed, pbtsLvCtx->pLvData->cbLVChunkMax ) );
                if ( cbUncompressed != cbUncompressedExpected && !FPartiallyDeletedLV( pbtsLvCtx->cRefsCurrent ) )
                {
                    AssertSz( fFalse, "Corrupt LV, invalid chunk size" );
                    pbtsLvCtx->pLvData->cCorruptLVs++;
                    goto HandleError;
                }
            }

            if ( pbtsLvCtx->pgnoCurrent != pbtsLvCtx->pgnoLvStart )
            {
                if ( pbtsLvCtx->cbAccumActual == 0 )
                {
                    //  track LVROOTs that are on a separate page from their first LVCHUNK
                    pbtsLvCtx->pLvData->cSeparatedRootChunks++;
                }
            }

            //  accumulate the uncompressed and compressed size for this chunk
            pbtsLvCtx->cbAccumLogical += cbUncompressedExpected;
            ULONG cbCompressed = pkdf->data.Cb();
            pbtsLvCtx->cbAccumActual += cbCompressed;

            //  accumulate stats on the estimated IO cost of retrieving this LV vs. the ideal IO cost.
            //  we measure these as:
            //  -  extra seeks per 1MB chunk (1MB chosen as the current seek/throughput tradeoff for HDDs)
            //  -  extra bytes read
            //
            //  there is some flexibility in the extra seeks number.  for example, if you have an LV that is
            //  large but is not contiguous because it happens to cross a larger space allocation boundary
            //  then this is acceptable.  we cannot expect to layout all LVs to have the absolute minimum
            //  run count because that would waste space and be impractically expensive.  we will allow an
            //  extra seek for any LV exceeding the optimal read IO size because we cannot analyze the space
            //  trees here
            //
            //  there is also flexibility in the extra bytes read.  you obviously cannot do better than the
            //  best case packing of the LV into pages accounting for format overhead.
            //
            //  we model IO coalescing via skipping gaps on read.  this will not count as an extra seek but it
            //  will show up as extra bytes read.  this can be used to model the disk access time
            //
            //  we also model the IO sorting provided by LV prefetch (ErrDIRDownLVPreread -> ErrBTIPreread)
            const CPG cpgAccumMax = cpgPrereadSequential;  //  from ErrDIRDownLVPreread
            const INT cbRunMax = 1 << 20;  // IO contiguity goal, not in code anywhere?
            Assert( cpgAccumMax > 0 );
            if ( pbtsLvCtx->rgpgnoAccum == NULL || pbtsLvCtx->cpgAccumMax < cpgAccumMax )
            {
                Assert( pbtsLvCtx->cpgAccum == 0 );
                delete[] pbtsLvCtx->rgpgnoAccum;
                pbtsLvCtx->cpgAccumMax = 0;
                Alloc( pbtsLvCtx->rgpgnoAccum = new PGNO[cpgAccumMax] );
                pbtsLvCtx->cpgAccumMax = cpgAccumMax;
            }
            if ( pbtsLvCtx->cpgAccum == 0 || pbtsLvCtx->rgpgnoAccum[pbtsLvCtx->cpgAccum - 1] != pbtsLvCtx->pgnoCurrent )
            {
                pbtsLvCtx->rgpgnoAccum[pbtsLvCtx->cpgAccum++] = pbtsLvCtx->pgnoCurrent;
            }
            if ( pbtsLvCtx->cpgAccum == pbtsLvCtx->cpgAccumMax || pbtsLvCtx->cbAccumLogical == pbtsLvCtx->cbCurrent )
            {
                sort( pbtsLvCtx->rgpgnoAccum, pbtsLvCtx->rgpgnoAccum + pbtsLvCtx->cpgAccum );
                PGNO pgnoLast = pgnoMax;
                PGNO pgnoRunStart = pgnoMax;
                for ( CPG ipg = 0; ipg < pbtsLvCtx->cpgAccum; ipg++ )
                {
                    const PGNO pgnoCurr = pbtsLvCtx->rgpgnoAccum[ipg];
                    if ( pgnoCurr - pgnoLast != 1 )
                    {
                        if ( (ULONG64)( pgnoCurr - pgnoLast - 1 ) * g_cbPage > UlParam( JET_paramMaxCoalesceReadGapSize )
                            || (ULONG64)( pgnoCurr - pgnoRunStart + 1 ) * g_cbPage > cbRunMax )
                        {
                            // this page is not contiguous and is either too far away to combine into one IO via over-read
                            // or exceeds our max run length so force a new run
                            pgnoRunStart = pgnoCurr;
                            pbtsLvCtx->cRunsAccessed++;
                            pbtsLvCtx->cpgAccessed++;
                        }
                        else
                        {
                            // this page is not contiguous and is near enough to combine into one IO via over-read
                            pbtsLvCtx->cpgAccessed += pgnoCurr - pgnoLast;
                        }
                    }
                    else
                    {
                        if ( (ULONG64)( pgnoCurr - pgnoRunStart + 1 ) * g_cbPage > cbRunMax )
                        {
                            // this page is contiguous but exceeds our max run length so force a new run
                            pgnoRunStart = pgnoCurr;
                            pbtsLvCtx->cRunsAccessed++;
                        }

                        // this page is contiguous
                        pbtsLvCtx->cpgAccessed++;
                    }
                    pgnoLast = pgnoCurr;
                }
                pbtsLvCtx->cpgAccum = 0;
            }
            if ( pbtsLvCtx->cbAccumLogical == pbtsLvCtx->cbCurrent )
            {
                // compute how much LV data we could reasonably expect to access per page given our format overhead
                ULONG64 cbAccessedPerPage = ( g_cbPage / pbtsLvCtx->pLvData->cbLVChunkMax ) * pbtsLvCtx->pLvData->cbLVChunkMax;

                // compute how much data we should reasonably access to read the entire LV
                ULONG64 cpgAccessedExpected = ( pbtsLvCtx->cbAccumActual + cbAccessedPerPage - 1 ) / cbAccessedPerPage;

                // compute how many seeks we should reasonably perform to read the entire LV, capped at a max run length
                ULONG64 cRunsAccessedExpected = ( cpgAccessedExpected * g_cbPage + cbRunMax - 1 ) / cbRunMax;

                // allow an extra seek for any LV exceeding the optimal read IO size.  anything smaller should probably be
                // contiguous
                if ( (ULONG64)cpgAccessedExpected * g_cbPage > UlParam( JET_paramMaxCoalesceReadSize ) && pbtsLvCtx->cRunsAccessed > cRunsAccessedExpected )
                {
                    cRunsAccessedExpected++;
                }

                // allow an extra page for any LV exceeding a single chunk.  this allows multi-chunk LVs whose space is not
                // perfectly aligned to page boundaries to not be penalized for that misalignment in terms of bytes accessed
                if ( ibOffset > 0 && pbtsLvCtx->cpgAccessed > cpgAccessedExpected )
                {
                    cpgAccessedExpected++;
                }

                // this is how much it would cost to read this LV as is (cRunsAccessed / avgRandomReadSeeksPerSec + ( cpgAccessedExpected * g_cbPage ) / avgMediaReadBytesPerSec)
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVSeeks )->ErrAddSample( pbtsLvCtx->cRunsAccessed ) ) );
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVBytes )->ErrAddSample( pbtsLvCtx->cpgAccessed * g_cbPage ) ) );

                // this is how much more it costs to read this LV when defragmented to a reasonable (not ideal) level
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVExtraSeeks )->ErrAddSample( pbtsLvCtx->cRunsAccessed - cRunsAccessedExpected ) ) );
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVExtraBytes )->ErrAddSample( ( pbtsLvCtx->cpgAccessed - cpgAccessedExpected ) * g_cbPage ) ) );
            }

            //  accumulate stats on this LV now that we have reached the end
            if ( pbtsLvCtx->cbAccumLogical == pbtsLvCtx->cbCurrent )
            {
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVSize )->ErrAddSample( pbtsLvCtx->cbAccumLogical ) ) );
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVComp )->ErrAddSample( pbtsLvCtx->cbAccumActual ) ) );
                SAMPLE ratio = (SAMPLE)100 * pbtsLvCtx->cbAccumActual / pbtsLvCtx->cbAccumLogical;
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVRatio )->ErrAddSample( ratio ) ) );
            }
        }
        else
        {
            AssertSz( fFalse, "Corrupt LV node, bad key size?" );
            pbtsLvCtx->pLvData->cCorruptLVs++;
            goto HandleError;
        }
    }   // if !deleted ...

HandleError:
    return err;
}

#endif  //  MINIMAL_FUNCTIONALITY



