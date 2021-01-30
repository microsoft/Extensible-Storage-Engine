// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.




#include "std.hxx"




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

#endif



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
    __in FUCB                   * const pfucb,
    __in const DATA             * const pdataField,
    __in const CompressFlags    compressFlags,
    __in const BOOL             fEncrypted,
    __in CPG *                  pcpgLvSpaceRequired,
    __out LvId                  * const plid,
    __in_opt FUCB               **ppfucb,
    __in_opt LVROOT2            *plvrootInit = NULL );



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




INLINE VOID AssertLVRootNode( FUCB *pfucbLV, const LvId lid )
{
#ifdef DEBUG
    LvId    lidT;

    Assert( Pcsr( pfucbLV )->FLatched() );
    Assert( FIsLVRootKey( pfucbLV->kdfCurr.key ) );
    Assert( sizeof( LVROOT ) == pfucbLV->kdfCurr.data.Cb() || sizeof( LVROOT2 ) == pfucbLV->kdfCurr.data.Cb() );
    LidFromKey( &lidT, pfucbLV->kdfCurr.key );
    Assert( lid == lidT );

#endif
}


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

VOID LVReportAndTrapCorruptedLV_( const FUCB * const pfucbLV, const LvId lid, const CHAR * const szFile, const LONG lLine, const WCHAR* const wszGuid )
{
    LVReportAndTrapCorruptedLV_( PinstFromPfucb( pfucbLV ), g_rgfmp[pfucbLV->u.pfcb->Ifmp()].WszDatabaseName(), pfucbLV->u.pfcb->PfcbTable()->Ptdb()->SzTableName(), Pcsr( pfucbLV )->Pgno(), lid, szFile, lLine, wszGuid );
}

#define LVReportAndTrapCorruptedLV2( pinst, WszDatabaseName, szTable, pgno, lid, wszGuid ) LVReportAndTrapCorruptedLV_( pinst, WszDatabaseName, szTable, pgno, lid, __FILE__, __LINE__, wszGuid )

VOID LVReportAndTrapCorruptedLV_( const INST * const pinst, PCWSTR wszDatabaseName, PCSTR szTable, const PGNO pgno, const LvId lid, const CHAR * const szFile, const LONG lLine, const WCHAR* const wszGuid )
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

            err = JET_errSuccess;
        }
        else
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"552cdeaa-c62e-47f6-81b4-fe87f2fb9274" );
            err = ErrERRCheck( JET_errLVCorrupted );
        }
    }

    else if ( FIsLVRootKey( pfucbLV->kdfCurr.key ) &&
                ( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucbLV->kdfCurr.data.Cb() ) )
    {
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if ( lidT <= lid  )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"952e2a73-05bf-4299-8e9b-cc5674ad72eb" );
            err = ErrERRCheck( JET_errLVCorrupted );
        }
        else
        {
            err = ErrERRCheck( wrnLVNoMoreData );
        }
    }
    else
    {
        LVReportAndTrapCorruptedLV( pfucbLV, lid, L"b1312def-92df-4461-a853-b5c344b642f7" );
        err = ErrERRCheck( JET_errLVCorrupted );
    }

    return err;
}


#ifdef DEBUG
BOOL FAssertLVFUCB( const FUCB * const pfucb )
{
    FCB *pfcb = pfucb->u.pfcb;
    if ( pfcb->FTypeLV() )
    {
        Assert( pfcb->Ptdb() == ptdbNil );
    }
    return pfcb->FTypeLV();
}
#endif




BOOL FPIBSessionLV( PIB *ppib )
{
    Assert( ppibNil != ppib );
    return ( PinstFromPpib(ppib)->m_ppibLV == ppib );
}


LOCAL ERR ErrFILECreateLVRoot( PIB *ppib, FUCB *pfucb, PGNO *ppgnoLV )
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

    CallR( ErrDIROpen( ppib, pfucb->u.pfcb, &pfucbTable ) );
    Call( ErrDIRBeginTransaction( ppib, 49445, NO_GRBIT ) );
    fInTransaction = fTrue;

    Call( PverFromIfmp( pfucb->ifmp )->ErrVERFlag( pfucbTable, operCreateLV, NULL, 0 ) );

    Call( ErrDIRCreateDirectory(    pfucbTable,
                                    cpgLVTree,
                                    &pgnoLVFDP,
                                    &objidLV,
                                    CPAGE::fPageLongValue,
                                    fTemp ? fSPUnversionedExtent : 0 ) );

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

    Assert( JET_errKeyDuplicate != err );
    return err;
}


INLINE ERR ErrFILEIInitLVRoot( FUCB *pfucb, const PGNO pgnoLV, FUCB **ppfucbLV )
{
    ERR             err;
    FCB * const     pfcbTable   = pfucb->u.pfcb;
    FCB *           pfcbLV;

    CallR( ErrDIROpen( pfucb->ppib, pgnoLV, pfucb->ifmp, ppfucbLV, fTrue ) );
    Assert( *ppfucbLV != pfucbNil );
    Assert( !FFUCBVersioned( *ppfucbLV ) );
    pfcbLV = (*ppfucbLV)->u.pfcb;
    Assert( !pfcbLV->FInitialized() || pfcbLV->FInitedForRecovery() );

    Assert( pfcbLV->Ifmp() == pfucb->ifmp );
    Assert( pfcbLV->PgnoFDP() == pgnoLV );
    Assert( pfcbLV->Ptdb() == ptdbNil );
    Assert( pfcbLV->CbDensityFree() == 0 );

    if ( pfcbLV->FInitedForRecovery() )
    {
        pfcbLV->RemoveList();
    }

    Assert( pfcbLV->FTypeNull() || pfcbLV->FInitedForRecovery() );
    pfcbLV->SetTypeLV();

    Assert( pfcbLV->PfcbTable() == pfcbNil );
    pfcbLV->SetPfcbTable( pfcbTable );


    if ( pfcbTable->Ptdb()->PjsphDeferredLV() )
    {
        pfcbLV->SetSpaceHints( pfcbTable->Ptdb()->PjsphDeferredLV() );
    }
    else if ( pfcbTable->Ptdb()->PfcbTemplateTable() &&
            pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb()->PjsphDeferredLV() )
    {
        pfcbLV->SetSpaceHints( pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb()->PjsphDeferredLV() );
    }


    pfcbLV->Lock();
    pfcbLV->CreateComplete();
    pfcbLV->ResetInitedForRecovery();
    pfcbLV->Unlock();

    Assert( pfcbNil == pfcbTable->Ptdb()->PfcbLV() );
    pfcbTable->Ptdb()->SetPfcbLV( pfcbLV );

    return err;
}


ERR ErrFILEOpenLVRoot( FUCB *pfucb, FUCB **ppfucbLV, BOOL fCreate )
{
    ERR         err;
    PIB         *ppib;
    PGNO        pgnoLV = pgnoNull;

    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->ppib != PinstFromPfucb(pfucb)->m_ppibLV );


    const BOOL  fExclusive = pfucb->u.pfcb->FDomainDenyReadByUs( pfucb->ppib ) || g_fRepair;
    const BOOL  fTemp = FFMPIsTempDB( pfucb->ifmp );

    Assert( PinstFromPfucb(pfucb)->m_critLV.FNotOwner() );

    if ( fExclusive )
    {
        Assert( ( fTemp && pfucb->u.pfcb->FTypeTemporaryTable() )
            || ( !fTemp && pfucb->u.pfcb->FTypeTable() ) );

        ppib = pfucb->ppib;

        FCB * const pfcbLV = pfucb->u.pfcb->Ptdb()->PfcbLV();
        if ( pfcbNil != pfcbLV )
        {
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

        if ( pfcbNil != pfcbLV )
        {
            Expected( pfcbLV->UlDensity() > 20 );

            PinstFromPfucb(pfucb)->m_critLV.Leave();

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

    Assert( pgnoNull == pgnoLV );
    if ( !fTemp )
    {
        Call( ErrCATAccessTableLV( ppib, pfucb->ifmp, pfucb->u.pfcb->ObjidFDP(), &pgnoLV ) );
    }
    else
    {
        Assert( fCreate );
    }

    if ( pgnoNull == pgnoLV && fCreate )
    {
        Call( ErrFILECreateLVRoot( ppib, pfucb, &pgnoLV ) );
    }

    if( pgnoNull != pgnoLV )
    {
        err = ErrFILEIInitLVRoot( pfucb, pgnoLV, ppfucbLV );
    }
    else
    {
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


LOCAL ERR ErrDIROpenLongRoot(
    FUCB *      pfucb,
    FUCB **     ppfucbLV,
    const BOOL  fAllowCreate )
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

    Assert( pfcbLVDBG->IsUnlocked() );
#endif

    FUCBSetLongValue( *ppfucbLV );

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
        CallR( ErrDIROpenLongRoot( pfucb, &pfucb->pfucbLV, fFalse ) );
        if ( pfucb->pfucbLV != NULL )
        {
            pfucb->pfucbLV->pfucbTable = pfucb;
        }
        else
        {
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

        if ( FFUCBSequential( pfucb ) )
        {
            FUCBSetSequential( pfucb->pfucbLV );
        }
        else
        {
            FUCBResetSequential( pfucb->pfucbLV );
        }

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


ERR ErrDIRDownLVDataPreread(
    FUCB            *pfucbLV,
    const LvId      lid,
    const ULONG     ulOffset,
    const DIRFLAG   dirflag,
    const ULONG     cbPreread )
{
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( lid > lidMin );

    LVKEY_BUFFER lvkeyPrereadLast;

    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG ulOffsetChunk = ( ulOffset / cbLVChunkMost ) * cbLVChunkMost;

    DIRUp( pfucbLV );

    LVKEY_BUFFER    lvkey;
    BOOKMARK        bm;
    LVKeyFromLidOffset( &lvkey, &bm.key, lid, ulOffsetChunk );
    bm.data.Nullify();

    DIB         dib;
    dib.dirflag = dirflag | fDIRExact;
    dib.pos     = posDown;
    dib.pbm     = &bm;

    if ( cbPreread > (ULONG)cbLVChunkMost )
    {
        const ULONG cChunksPartial  = ( cbPreread % cbLVChunkMost ) ? 1 : 0;
        const ULONG cChunksPreread  = ( cbPreread  / cbLVChunkMost ) + cChunksPartial;
        const ULONG ulOffsetPrereadLast = ulOffsetChunk + ( ( cChunksPreread - 1 ) * cbLVChunkMost );
        Assert( ulOffsetPrereadLast > ulOffsetChunk );

        KEY keyPrereadLast;
        LVKeyFromLidOffset( &lvkeyPrereadLast, &keyPrereadLast, lid, ulOffsetPrereadLast );

        Assert( !FFUCBLimstat( pfucbLV ) );
        Assert( pfucbLV->dataSearchKey.FNull() );
        
        FUCBSetPrereadForward( pfucbLV, cpgPrereadSequential );
        FUCBSetLimstat( pfucbLV );
        FUCBSetUpper( pfucbLV );
        FUCBSetInclusive( pfucbLV );

        Assert( keyPrereadLast.prefix.Cb() == 0 );
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

BOOL CmpLid( __in const LvId& lid1, __in const LvId& lid2 )
{
    return lid1 < lid2;
}

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

    Call( ErrDIROpenLongRoot( pfucb, &pfucbLV, fFalse ) );
    Assert( pfucbLV != pfucbNil || wrnLVNoLongValues == err );
    if ( wrnLVNoLongValues != err )
    {
        std::sort( plid, plid + clid, CmpLid );

        for ( ULONG ilidT = 0; ilidT < clid; ilidT++ )
        {
            KEY keyT;
            LVRootKeyFromLid( &rglvkeyStart[ ilidT ], &keyT, plid[ ilidT ] );
            rgpvStart[ ilidT ] = (VOID *) &rglvkeyStart[ ilidT ];
            rgcbStart[ ilidT ] = keyT.Cb();
            
            LVKeyFromLidOffset( &rglvkeyEnd[ ilidT ], &keyT, plid[ ilidT ], cbLVPreread - cbLVChunkMost );
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
            JET_bitPrereadForward ,
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

    if ( cbData == 0 )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    Call( ErrDIROpenLongRoot( pfucb, &pfucbLV, fFalse ) );
    Assert( pfucbLV != pfucbNil || wrnLVNoLongValues == err );
    if ( wrnLVNoLongValues == err )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    const LONG  cbLVChunkMost       = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    const ULONG ulOffsetPreread     = ( ulOffset / cbLVChunkMost ) * cbLVChunkMost;
    const ULONG cbAddPreread        = ulOffset - ulOffsetPreread;
    const ULONG cbPreread           = cbData + cbAddPreread < cbData ? ulMax : cbData + cbAddPreread;
    const ULONG cChunksPartial      = ( cbPreread % cbLVChunkMost ) ? 1 : 0;
    const ULONG cChunksPreread      = ( cbPreread / cbLVChunkMost ) + cChunksPartial;
    const ULONG ulOffsetPrereadLast = ulOffsetPreread + ( ( cChunksPreread - 1 ) * cbLVChunkMost );

    if ( ulOffsetPreread == 0 && ulOffsetPrereadLast == 0 )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    LVRootKeyFromLid( &rglvkeyStart[ cRange ], &keyT, lid );
    rgpvStart[ cRange ] = (VOID *) &rglvkeyStart[ cRange ];
    rgcbStart[ cRange ] = keyT.Cb();
    rgpvEnd[ cRange ] = rgpvStart[ cRange ];
    rgcbEnd[ cRange ] = rgcbStart[ cRange ];
    cRange++;

    LVKeyFromLidOffset( &rglvkeyStart[ cRange ], &keyT, lid, ulOffsetPreread );
    rgpvStart[ cRange ] = (VOID *) &rglvkeyStart[ cRange ];
    rgcbStart[cRange] = keyT.Cb();
    LVKeyFromLidOffset( &rglvkeyEnd[ cRange ], &keyT, lid, ulOffsetPrereadLast );
    rgpvEnd[ cRange ] = (VOID *) &rglvkeyEnd[ cRange ];
    rgcbEnd[cRange] = keyT.Cb();
    cRange++;


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


ERR ErrDIRDownLVData(
    FUCB            *pfucb,
    const LvId      lid,
    const ULONG     ulOffset,
    const DIRFLAG   dirflag )
{
    return ErrDIRDownLVDataPreread( pfucb, lid, ulOffset, dirflag, 0 );
}

ERR ErrDIRDownLVRootPreread(
    FUCB * const    pfucbLV,
    const LvId      lid,
    const DIRFLAG   dirflag,
    const ULONG     cbPreread,
    const BOOL      fCorruptIfMissing )
{
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( lid > lidMin );

    DIRUp( pfucbLV );

    LVKEY_BUFFER lvkeyPrereadLast;


    LVKEY_BUFFER lvkey;
    BOOKMARK    bm;
    LVRootKeyFromLid( &lvkey, &bm.key, lid );
    bm.data.Nullify();

    DIB dib;
    dib.dirflag = dirflag | fDIRExact;
    dib.pos     = posDown;
    dib.pbm     = &bm;

    const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    if ( cbPreread > (ULONG)cbLVChunkMost )
    {
        const ULONG cChunksPartial  = ( cbPreread % cbLVChunkMost ) ? 1 : 0;
        const ULONG cChunksPreread  = ( cbPreread  / cbLVChunkMost ) + cChunksPartial;
        const ULONG ulOffsetPrereadLast = ( cChunksPreread - 1 ) * cbLVChunkMost;
        Assert( ulOffsetPrereadLast > 0 );

        KEY keyPrereadLast;
        LVKeyFromLidOffset( &lvkeyPrereadLast, &keyPrereadLast, lid, ulOffsetPrereadLast );

        Assert( !FFUCBLimstat( pfucbLV ) );
        Assert( pfucbLV->dataSearchKey.FNull() );
        
        FUCBSetPrereadForward( pfucbLV, cpgPrereadSequential );
        FUCBSetLimstat( pfucbLV );
        FUCBSetUpper( pfucbLV );
        FUCBSetInclusive( pfucbLV );
        pfucbLV->dataSearchKey.SetPv( keyPrereadLast.suffix.Pv() );
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

ERR ErrDIRDownLVRootPreread(
    FUCB * const    pfucbLV,
    const LvId      lid,
    const DIRFLAG   dirflag,
    const ULONG     cbPreread )
{
    return ErrDIRDownLVRootPreread( pfucbLV, lid, dirflag, cbPreread, !g_fRepair );
}

ERR ErrDIRDownLVRoot(
    FUCB * const    pfucb,
    const LvId      lid,
    const DIRFLAG   dirflag )
{
    return ErrDIRDownLVRootPreread( pfucb, lid, dirflag, 0 );
}

INLINE ERR ErrRECISetLid(
    FUCB            *pfucb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const LvId      lid )
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

LOCAL bool FLVCompressedChunk( const LONG cbLVChunkMost, const KEY& key, const DATA& data, const ULONG ulLVSize )
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

LOCAL CompressFlags LVIAddXpress10FlagsIfEnabled(
        CompressFlags compressFlags,
        const INST* const pinst,
        const IFMP ifmp
        )
{
    if ( (compressFlags & compressXpress) &&
         BoolParam( pinst, JET_paramFlight_EnableXpress10Compression ) &&
         g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvXpress10Compression ) >= JET_errSuccess )
    {
        return CompressFlags( compressFlags | compressXpress10 );
    }

    return compressFlags;
}

LOCAL ERR ErrLVITryCompress(
    FUCB *pfucbLV,
    const DATA& data,
    const INST* const pinst,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    FUCB *pfucbTable,
    __out DATA *pdataToSet,
    __out BYTE ** pbAlloc )
{
    ERR err = JET_errSuccess;
    *pbAlloc = NULL;
    
    BYTE * pbDataCompressed = NULL;

    pdataToSet->SetPv( const_cast<VOID *>( data.Pv() ) );
    pdataToSet->SetCb( data.Cb() );

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
        CompressFlags compressFlagsEffective = LVIAddXpress10FlagsIfEnabled( compressFlags, pinst, pfucbTable->ifmp );

        INT cbDataCompressedActual;
        const ERR errT = ErrPKCompressData(
            data,
            compressFlagsEffective,
            pinst,
            pbDataCompressed,
            CbPKCompressionBuffer(),
            &cbDataCompressedActual );

        if ( errT >= JET_errSuccess && cbDataCompressedActual < data.Cb() )
        {
            *pbAlloc = pbDataCompressed;
            pdataToSet->SetPv( pbDataCompressed );
            pdataToSet->SetCb( cbDataCompressedActual );
        }
        else
        {
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
                pfucbTable->pbEncryptionKey,
                pfucbTable->cbEncryptionKey,
                PinstFromPfucb( pfucbTable )->m_iInstance,
                pfucbLV->u.pfcb->TCE() );
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

LOCAL ERR ErrLVInsert(
    FUCB * const pfucbLV,
    const KEY& key,
    const DATA& data,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    FUCB *pfucbTable,
    const DIRFLAG dirflag )
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


LOCAL ERR ErrLVReplace(
    FUCB * const pfucbLV,
    const DATA& data,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    const DIRFLAG dirflag )
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

LOCAL void LVICaptureCorruptedLVChunkInfo(
    const FUCB* const pfucbLV,
    const KEY& key )
{
    const WCHAR wchHexLUT[ 16 ] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
    BYTE* keyBuff = (BYTE*) _alloca( key.Cb() );
    key.CopyIntoBuffer( keyBuff, key.Cb() );

    INT cBuff = 2 * key.Cb() + 1;
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

LOCAL ERR ErrLVIDecompressAndCompare(
    const FUCB* const pfucbLV,
    const LvId lid,
    const DATA& dataCompressed,
    const INT ibOffset,
    __in_bcount( cbData ) const BYTE * const pbData,
    const INT cbData,
    __out bool * pfIdentical )
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

LOCAL ERR ErrLVIDecompress(
    const FUCB* const pfucbLV,
    const LvId lid,
    const DATA& dataCompressed,
    const INT ibOffset,
    __out_bcount( cbData ) BYTE * const pbData,
    const INT cbData )
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
                    pfucbLV->pfucbTable->pbEncryptionKey,
                    pfucbLV->pfucbTable->cbEncryptionKey,
                    PinstFromPfucb( pfucbLV )->m_iInstance,
                    pfucbLV->u.pfcb->TCE() ) );
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
        Call( ErrPKDecompressData( dataIn, pfucbLV, NULL, 0, &cbActual ) );
        if( JET_wrnBufferTruncated == err )
        {
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
    __out bool * const pfIdentical )
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
                    pfucbLV->pfucbTable->pbEncryptionKey,
                    pfucbLV->pfucbTable->cbEncryptionKey,
                    PinstFromPfucb( pfucbLV )->m_iInstance,
                    pfucbLV->u.pfcb->TCE() ) );
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
        Call( ErrPKDecompressData( dataIn, pfucbLV, NULL, 0, &cbActual ) );
        if( JET_wrnBufferTruncated == err )
        {
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

LOCAL ERR ErrLVIGetDataSize(
    const FUCB* const pfucbLV,
    const KEY& key,
    const DATA& data,
    const BOOL fEncrypted,
    const ULONG ulLVSize,
    __out ULONG * const pulActual,
    const LONG cbLVChunkMostOverride = 0 )
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
                    pfucbLV->pfucbTable->pbEncryptionKey,
                    pfucbLV->pfucbTable->cbEncryptionKey,
                    PinstFromPfucb( pfucbLV )->m_iInstance,
                    pfucbLV->u.pfcb->TCE() ) );
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


ERR ErrLVInit( INST *pinst )
{
    ERR err = JET_errSuccess;
    Call( ErrPIBBeginSession( pinst, &pinst->m_ppibLV, procidNil, fFalse ) );

HandleError:
    return err;
}


VOID LVTerm( INST * pinst )
{
    Assert( pinst->m_critLV.FNotOwner() );

    if ( pinst->m_ppibLV )
    {

#ifdef DEBUG
        const PIB* const ppibLV = pinst->m_ppibLV;

        Assert( pfucbNil == ppibLV->pfucbOfSession || pinst->FInstanceUnavailable() );

        const PIB* ppibT = pinst->m_ppibGlobal;
        for ( ; ppibT != ppibLV && ppibT != ppibNil; ppibT = ppibT->ppibNext );
        Assert( ppibT == ppibLV );
#endif

        pinst->m_ppibLV = ppibNil;
    }
}


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

LOCAL ERR ErrRECIOverwriteSeparateLV(
    __in FUCB * const pfucb,
    __in const DATA * const pdataInRecord,
    __inout DATA * const pdataNew,
    __in const CompressFlags compressFlags,
    __in const BOOL fEncrypted,
    __inout ULONG * const pibLongValue,
    __out LvId * const plid )
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
    const BYTE * const pbDataNewOrig    = reinterpret_cast<BYTE *>( pdataNew->Pv() );
    const size_t cbDataNewOrig          = pdataNew->Cb();
    const size_t ibLongValueOrig        = *pibLongValue;
#endif

    ERR err = JET_errSuccess;

    VOID *pvbf = NULL;
    BFAlloc( bfasTemporary, &pvbf );

    DATA data;
    data.SetPv( pvbf );
    data.SetCb( 0 );

    BYTE * pbBuffer         = reinterpret_cast<BYTE *>( pvbf );
    const LONG cbBuffer     = g_cbPage;
    LONG cbBufferRemaining  = cbBuffer;
    LONG cbToCopy;


    if( *pibLongValue > static_cast<ULONG>( pdataInRecord->Cb() ) )
    {
        FireWall( "OffsetTooHighOnOverwriteSepLv" );
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

    
    pdataNew->DeltaCb( -cbToCopy );
    pdataNew->DeltaPv( cbToCopy );
    *pibLongValue += cbToCopy;

    Assert( data.Cb() >= 0 );
    Assert( data.Pv() || data.Cb() == 0 );
    Assert( data.Cb() <= cbBuffer );
    Assert( cbBufferRemaining == 0 || pdataNew->Cb() == 0 );
    Assert( data.Cb() == g_cbPage || pdataNew->Cb() == 0 );

    Call( ErrRECSeparateLV( pfucb, &data, compressFlags, fEncrypted, plid, NULL ) );

#ifdef DEBUG
    Assert( 0 != *plid );
    ASSERT_VALID( pdataNew );
    if( pbDataNewOrig == pdataNew->Pv() )
    {
        Assert( static_cast<size_t>( pdataNew->Cb() ) == cbDataNewOrig );
        Assert( ibLongValueOrig == *pibLongValue );
    }
    else
    {
        const size_t cbDiff = reinterpret_cast<BYTE *>( pdataNew->Pv() ) - pbDataNewOrig;
        Assert( cbDiff > 0 );
        Assert( ( cbDataNewOrig - pdataNew->Cb() ) == cbDiff );
        Assert( ( *pibLongValue - ibLongValueOrig ) == static_cast<ULONG>( cbDiff ) );
        Assert( pdataNew->Cb() == 0 || *pibLongValue == static_cast<ULONG>( g_cbPage ) );
    }
#endif

HandleError:
    BFFree( pvbf );
    return err;
}
    
LOCAL ERR ErrRECIAppendSeparateLV(
    __in FUCB * const pfucb,
    __in const DATA * const pdataInRecord,
    __inout DATA * const pdataNew,
    __in const CompressFlags compressFlags,
    __in const BOOL fEncrypted,
    __out LvId * const plid )
{
    ULONG ibLongValue = pdataInRecord->Cb();
    return ErrRECIOverwriteSeparateLV( pfucb, pdataInRecord, pdataNew, compressFlags, fEncrypted, &ibLongValue, plid );
}

LOCAL CPG CpgLVIRequired_( _In_ const ULONG cbPage, _In_ const ULONG cbLvChunk, _In_ const ULONG cbLv )
{
    Expected( cbLv > cbPage );

    const ULONG cChunksPerPage = cbPage / cbLvChunk;
    const CPG cpgRet = roundup( roundup( cbLv, cbLvChunk ) / cbLvChunk, cChunksPerPage ) / cChunksPerPage;

    return cpgRet;
}

LOCAL CPG CpgLVIRequired( _In_ const FCB * const pfcbTable, _In_ const ULONG cbLv )
{
    Assert( pfcbTable->FTypeTable() );
    Expected( cbLv > (ULONG)g_cbPage );

    return CpgLVIRequired_( g_cbPage, pfcbTable->Ptdb()->CbLVChunkMost(), cbLv );
}

JETUNITTEST( LV, TestLvToCpgRequirements )
{
    CHECK( 8 == CpgLVIRequired_( 32 * 1024, 8191, ( 32 * 1024 - 4 ) * 8 ) );
    CHECK( 8 == CpgLVIRequired_( 32 * 1024, 8191, ( 32 * 1024 - 8 ) * 8 ) );
    CHECK( 9 == CpgLVIRequired_( 32 * 1024, 8191, ( 32 * 1024 - 0 ) * 8 ) );
}

LOCAL ERR ErrRECISeparateLV(
    __in FUCB * const pfucb,
    __in const DATA dataInRecord,
    __in DATA dataNew,
    __in const CompressFlags compressFlags,
    __in const BOOL fEncrypted,
    __in BOOL fContiguousLv,
    __in ULONG ibLongValue,
    __out LvId * const plid,
    __in const ULONG ulColMax,
    __in const LVOP lvop )
{
    ASSERT_VALID( pfucb );
    Assert( plid );
    Assert( dataInRecord.Cb() > 0 || dataNew.Cb() > 0 );

    CPG cpgLvSpaceRequired = 0;

    *plid = 0;

    ERR err = JET_errSuccess;

    
    switch( lvop )
    {
        default:
            AssertSz( fFalse, "Unexpected lvop" );
            Call( ErrERRCheck( JET_errInternalError ) );
            break;
            
        case lvopInsert:
            Assert( !fContiguousLv || ( lvopInsert == lvop  ) );
            if ( fContiguousLv && lvopInsert == lvop  )
            {
                Assert( dataNew.Cb() > pfucb->u.pfcb->Ptdb()->CbLVChunkMost() );
                Assert( dataNew.Cb() > g_cbPage );
                if ( compressFlags != compressNone )
                {
                    AssertSz( FNegTest( fInvalidAPIUsage ), "Use of JET_bitSetContiguousLV not implemented / allowed with compressed column data.  File DCR." );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
                Expected( ibLongValue == 0 );

                cpgLvSpaceRequired = CpgLVIRequired( pfucb->u.pfcb, dataNew.Cb() );
            }

            __fallthrough;
        
        case lvopReplace:
        {

            DATA data;
            data.SetPv( dataNew.Pv() );
            data.SetCb( min( dataNew.Cb(), pfucb->u.pfcb->Ptdb()->CbLVChunkMost() ) );

            
            dataNew.DeltaCb( -data.Cb() );
            dataNew.DeltaPv( data.Cb() );
            ibLongValue += data.Cb();

            Assert( data.Cb() == pfucb->u.pfcb->Ptdb()->CbLVChunkMost() || dataNew.Cb() == 0 );
            Assert( lvop == lvopInsert || cpgLvSpaceRequired == 0 );

            Call( ErrRECICreateLvRootAndChunks( pfucb, &data, compressFlags, fEncrypted, &cpgLvSpaceRequired, plid, NULL ) );
            if ( cpgLvSpaceRequired == 0 )
            {
                fContiguousLv = fFalse;
            }
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
    
    if( dataNew.Cb() > 0 )
    {
        Call( ErrRECAOSeparateLV( pfucb, plid, &dataNew, compressFlags, fEncrypted, fContiguousLv, ibLongValue, ulColMax, lvopAppend ) );
        Assert( JET_wrnCopyLongValue != err || FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
    }
    else
    {
        Expected( !fContiguousLv );
    }

HandleError:

    return err;
}

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
{
    ERR         err;
    DATA        dataRetrieved;
    BYTE *      pbDecompressed = NULL;
    BYTE *      pbDataDecrypted = NULL;
    dataRetrieved.Nullify();

    const ULONG cbPreferredIntrinsicLV  =
                        ( grbit & JET_bitSetSeparateLV ) ?
                            LvId::CbLidFromCurrFormat( pfucb ) :
                        ( grbit & JET_bitSetIntrinsicLV ) ?
                            ( CbLVIntrinsicTableMost( pfucb ) ) :
                        ( UlBound( CbPreferredIntrinsicLVMost( pfucb->u.pfcb ), LvId::CbLidFromCurrFormat( pfucb ), CbLVIntrinsicTableMost( pfucb ) ) );
    
    const BOOL  fRevertToDefault        = ( grbit & JET_bitSetRevertToDefaultValue );
    const BOOL  fUseContiguousSpace     = ( grbit & JET_bitSetContiguousLV );

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

    BOOL    fModifyExistingSLong    = fFalse;
    BOOL    fNewInstance            = ( 0 == itagSequence );

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
                        pfucb->pbEncryptionKey,
                        pfucb->cbEncryptionKey,
                        PinstFromPfucb( pfucb )->m_iInstance,
                        pfucb->u.pfcb->TCE() );
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
            Assert( FFUCBUpdateSeparateLV( pfucb ) );
            LvId lidT = LidOfSeparatedLV( dataRetrieved );
            Call( ErrRECAffectSeparateLV( pfucb, &lidT, fLVDereference ) );
            Assert( JET_wrnCopyLongValue != err );

            Assert( pfcbNil != pfucb->u.pfcb->Ptdb()->PfcbLV() );
            PERFOpt( PERFIncCounterTable( cLVUpdates, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Ptdb()->PfcbLV()->TCE() ) );
            
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

    Assert( pdataField->Cb() > 0 );


    if ( fModifyExistingSLong )
    {
        Assert( lvopAppend == lvop
            || lvopResize == lvop
            || lvopOverwriteRange == lvop
            || lvopOverwriteRangeAndResize == lvop );

        Assert( FFUCBUpdateSeparateLV( pfucb ) );

        LvId lidT = LidOfSeparatedLV( dataRetrieved );
        Call( ErrRECAOSeparateLV( pfucb, &lidT, pdataField, compressFlags, fEncrypted, fFalse, ibLongValue, ulColMax, lvop ) );
        if ( JET_wrnCopyLongValue == err )
        {
            Call( ErrRECISetLid( pfucb, columnid, itagSequence, lidT ) );
        }

        Assert( pfcbNil != pfucb->u.pfcb->Ptdb()->PfcbLV() );
        PERFOpt( PERFIncCounterTable( cLVUpdates, PinstFromPfucb( pfucb ), pfucb->u.pfcb->Ptdb()->PfcbLV()->TCE() ) );
    }
    else
    {
        ULONG       cbIntrinsic         = pdataField->Cb();

        Assert( fNewInstance || (ULONG)dataRetrieved.Cb() <= CbLVIntrinsicTableMost( pfucb ) );

        switch ( lvop )
        {
            case lvopInsert:
            case lvopInsertZeroedOut:
                Assert( dataRetrieved.FNull() );
            case lvopReplace:
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
        Assert( UlBound( cbPreferredIntrinsicLV, cbLidFromEfv, CbLVIntrinsicTableMost( pfucb ) ) == cbPreferredIntrinsicLV );
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

            FUCBSetUpdateSeparateLV( pfucb );

            if( lvopInsert == lvop
                || lvopReplace == lvop
                || lvopAppend == lvop
                || lvopOverwriteRangeAndResize == lvop )
            {
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


LOCAL ERR ErrRECIBurstSeparateLV( FUCB * pfucbTable, FUCB * pfucbSrc, LvId * plid )
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

    AssertLVRootNode( pfucbSrc, *plid );
#pragma warning(suppress: 26015)
    UtilMemCpy( &lvroot, pfucbSrc->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbSrc->kdfCurr.data.Cb() ) );

    AllocR( pvAlloc = PbPKAllocCompressionBuffer() );
    data.SetPv( pvAlloc );

    PERFOpt( PERFIncCounterTable( cLVCopies, PinstFromPfucb( pfucbSrc ), TceFromFUCB( pfucbSrc ) ) );


    if ( lvroot.ulSize > 0 )
    {
        Call( ErrDIRDownLVData( pfucbSrc, *plid, ulLVOffsetFirst, fDIRNull ) );

        const LONG cbLVChunkMost = pfucbTable->u.pfcb->Ptdb()->CbLVChunkMost();
        Assert( pfucbSrc->kdfCurr.data.Cb() <= (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) );
        if ( pfucbSrc->kdfCurr.data.Cb() > (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) )
        {

            LVReportAndTrapCorruptedLV( pfucbSrc, *plid, L"2dff4d03-0a99-4141-aeef-fe408236bce1" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        UtilMemCpy( pvAlloc, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
        Assert( data.Pv() == pvAlloc );
        data.SetCb( pfucbSrc->kdfCurr.data.Cb() );
        CallS( ErrDIRRelease( pfucbSrc ) );

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
                    fFalse,
                    &lid,
                    &pfucbDest,
                    &lvrootT ) );
        Assert( lid.FLidObeysCurrFormat( pfucbTable ) );

        LVKeyFromLidOffset( &lvkey, &key, lid, ulOffset );

        ulOffset += cbLVChunkMost;

        PERFOpt( PERFIncCounterTable( cLVChunkCopies, PinstFromPfucb( pfucbSrc ), TceFromFUCB( pfucbSrc ) ) );

        Call( ErrDIRInsert( pfucbDest, key, data, fDIRBackToFather ) );

        DIRUp( pfucbDest );

        while ( ( err = ErrDIRNext( pfucbSrc, fDIRSameLIDOnly ) ) >= JET_errSuccess )
        {
            fLatchedSrc = fTrue;

            Call( ErrLVCheckDataNodeOfLid( pfucbSrc, *plid ) );
            if( wrnLVNoMoreData == err )
            {
                Assert( fFalse );
                Error( ErrERRCheck( JET_errInternalError ) );
            }

            Assert( pfucbSrc->kdfCurr.data.Cb() <= (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) );
            if ( pfucbSrc->kdfCurr.data.Cb() > (LONG)CbOSEncryptAes256SizeNeeded( cbLVChunkMost ) )
            {

                LVReportAndTrapCorruptedLV( pfucbSrc, *plid, L"0d41f61b-1957-4f39-9c99-42b4c64634dc" );
                Error( ErrERRCheck( JET_errLVCorrupted ) );
            }

            Assert( ulOffset % cbLVChunkMost == 0 );
            LVKeyFromLidOffset( &lvkey, &key, lid, ulOffset );
            UtilMemCpy( pvAlloc, pfucbSrc->kdfCurr.data.Pv(), pfucbSrc->kdfCurr.data.Cb() );
            Assert( data.Pv() == pvAlloc );
            data.SetCb( pfucbSrc->kdfCurr.data.Cb() ) ;

            ulOffset += cbLVChunkMost;

            Call( ErrDIRRelease( pfucbSrc ) );
            fLatchedSrc = fFalse;

            PERFOpt( PERFIncCounterTable( cLVChunkCopies, PinstFromPfucb( pfucbSrc ), TceFromFUCB( pfucbSrc ) ) );

            Call( ErrDIRInsert( pfucbDest, key, data, fDIRBackToFather ) );

            DIRUp( pfucbDest );
        }

        if ( JET_errNoCurrentRecord != err )
        {
            Call( err );
        }
    }

    err = ErrDIRDownLVRoot( pfucbSrc, lid, fDIRNull );
    Assert( JET_errWriteConflict != err );
    Call( err );
    CallS( err );

    data.SetPv( &lvroot );
    data.SetCb( lvroot.fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );
    lvroot.ulReference = 1;
    Call( ErrDIRReplace( pfucbSrc, data, fDIRNull ) );
    Call( ErrDIRGet( pfucbSrc ) );

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
            DIRSetActiveSpaceRequestReserve( pfucbLV, cpgRequiredReserve - 1 );
        }

        err = ErrLVInsert( pfucbLV, key, data, compressFlags, fEncrypted, pfucbLV->pfucbTable, fDIRBackToFather );

        if ( CpgDIRActiveSpaceRequestReserve( pfucbLV ) == cpgDIRReserveConsumed )
        {
            DIRSetActiveSpaceRequestReserve( pfucbLV, 0 );
            cpgRequiredReserve = 0;
        }

        Call( err );

        ulSize += data.Cb();

        Assert( pbAppend + data.Cb() <= pbMax );
        pbAppend += data.Cb();

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
    ULONG               ulSize,
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

    while( cbAppend > 0 )
    {
        Assert( ulSize % cbLVChunkMost == 0 );
        LVKeyFromLidOffset( &lvkey, &key, lid, ulSize );

        Assert( key.prefix.FNull() );
        Assert( key.suffix.Pv() == &lvkey );

        Assert( data.Pv() == pvbf );
        data.SetCb( min( cbAppend, (SIZE_T)cbLVChunkMost ) );

        PERFOpt( PERFIncCounterTable( cLVChunkAppends, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

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

    CallR( ErrDIRDownLVData( pfucbLV, lid, ulOffset, fDIRNull ) );

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

    while ( ( err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly ) ) >= JET_errSuccess )
    {
        err = ErrLVCheckDataNodeOfLid( pfucbLV, lid );
        if ( JET_errSuccess != err )
        {
            CallS( ErrDIRRelease( pfucbLV ) );
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

    CallR( ErrDIRDownLVData( pfucbLV, lid, ibLongValue, fDIRNull ) );

#ifdef DEBUG
    ULONG   cPartialChunkOverwrite  = 0;
#endif

    ULONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();

    for ( ; ; )
    {
        ULONG   ibChunk;
        ULONG   cb = 0;

        OffsetFromKey( &ibChunk, pfucbLV->kdfCurr.key );
        Assert( ibLongValue >= ibChunk );
        Assert( ibLongValue < ibChunk + cbLVChunkMost );

        if ( ibChunk == ibLongValue &&
             ULONG( pbMax - pb ) >= cbLVChunkMost )
        {
            cb = cbLVChunkMost;
            data.SetCb( cb );
            data.SetPv( pb );
        }
        else
        {
#ifdef DEBUG
            cPartialChunkOverwrite++;
            Assert( cPartialChunkOverwrite <= 2 );
#endif
            const ULONG ib = ibLongValue - ibChunk;
            ULONG   cbChunk = g_cbPage;
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

        Assert( pb <= pbMax );
        if ( pb >= pbMax )
        {
            return JET_errSuccess;
        }

        err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly );
        if ( err < 0 )
        {
            if ( JET_errNoCurrentRecord == err )
                break;
            else
                return err;
        }

        err = ErrLVCheckDataNodeOfLid( pfucbLV, lid );
        if ( err < 0 )
        {
            CallS( ErrDIRRelease( pfucbLV ) );
            return err;
        }
        else if ( wrnLVNoMoreData == err )
        {
            return ErrERRCheck( JET_errInternalError );
        }

        Assert( ibLongValue % cbLVChunkMost == 0 );
    }

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
{
    ASSERT_VALID( pfucb );
    Assert( plid );
    Assert( pfucb->ppib->Level() > 0 );

    Assert( lvopInsert == lvop
            || lvopInsertZeroedOut == lvop
            || lvopReplace == lvop
            || lvopAppend == lvop
            || lvopResize == lvop
            || lvopOverwriteRange == lvop
            || lvopOverwriteRangeAndResize == lvop );

#ifdef DEBUG
    pdataField->AssertValid( lvopInsertZeroedOut == lvop || lvopResize == lvop );
#endif

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

    CallR( ErrDIROpenLongRoot( pfucb ) );
    Assert( wrnLVNoLongValues != err );

    pfucbLV = pfucb->pfucbLV;
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    Call( ErrDIRDownLVRoot( pfucbLV, *plid, fDIRNull ) );
#pragma warning(suppress: 26015)
    UtilMemCpy( &lvroot, pfucbLV->kdfCurr.data.Pv(), min( sizeof(lvroot), pfucbLV->kdfCurr.data.Cb() ) );

    Assert( pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT) || FLVEncrypted( lvroot.fFlags ) );
    Assert( !fEncrypted == !FLVEncrypted( lvroot.fFlags ) );

    ulVerRefcount = UlLVIVersionedRefcount( pfucbLV );

    Assert( ulVerRefcount > 0 );
    Assert( !FPartiallyDeletedLV( ulVerRefcount ) );

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

    BOOL fLVBursted = fFalse;

    if ( ulVerRefcount > 1 || FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
    {
        Assert( *plid == lidCurr );
        Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );
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
            Assert( 0 == ulSize );
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

    if ( ( ulColMax > 0 && ulNewSize > ulColMax ) || ulNewSize > lMax )
    {
        Error( ErrERRCheck( JET_errColumnTooBig ) );
    }

    AssertTrack( 1 == ulVerRefcount || ( fLVBursted && 1 == lvroot.ulReference ), "LVRefcountGreaterThan1" );

    if ( lvroot.ulSize  == ulNewSize &&
         lvroot.ulReference == 1 )
    {
        CallS( ErrDIRRelease( pfucbLV ) );
        if ( !fLVBursted )
        {
            err = ErrDIRGetLock( pfucbLV, writeLock );
        }
    }
    else
    {
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


        Assert( 1 == ulVerRefcount );

        Call( ErrDIRGet( pfucbLV ) );

        if ( FDIRDeltaActiveNotByMe( pfucbLV, OffsetOf( LVROOT2, ulReference ) ) )
        {
            Assert( JET_wrnCopyLongValue != wrn );
            Assert( *plid == lidCurr );
            Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );
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
            CallS( ErrDIRRelease( pfucbLV ) );

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
        Assert( !Pcsr( pfucb )->FLatched() );
        Call( ErrRECAffectSeparateLV( pfucb, &lidCurr, fLVDereference ) );
        Assert( JET_wrnCopyLongValue != err );
    }

    Assert( 1 == lvroot.ulReference );

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
            if ( ulNewSize < ulSize )
            {
                Call( ErrLVTruncate( pfucbLV, *plid, ulSize, ulNewSize, pvbf, compressFlags, fEncrypted ) );
            }
            else if ( ulNewSize > ulSize )
            {
                const LONG cbLVChunkMost = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
                if ( ulSize > 0 )
                {

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

                    AssertLVRootNode( pfucbLV, *plid );

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

        case lvopOverwriteRange:
            if ( 0 == ulSize )
            {
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

    err = ( err < JET_errSuccess ) ? err : wrn;
    return err;
}


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
{
    ASSERT_VALID( pdataColumn );


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

    Assert( cbT <= CbLVIntrinsicTableMost( pfucb ) );
    Assert( cbT <= cbLVIntrinsicMost || pvbf != NULL );
    dataSet.SetCb( cbT );

    dataOrigSet = dataSet;

    if ( itagSequence < 2 )
    {
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
            CompressFlags compressFlagsEffective = LVIAddXpress10FlagsIfEnabled( compressFlags, PinstFromPfucb( pfucb ), pfucb->ifmp );

            BYTE * pbDataCompressed = rgbCompressed;
            INT cbDataCompressedMax = sizeof( rgbCompressed );

            if ( cbDataCompressedMax < dataSet.Cb() )
            {
                if ( NULL != pvbf )
                {
                    Assert( pvbf == dataSet.Pv() );

                    pbDataCompressed = (BYTE *) dataSet.Pv() + dataSet.Cb();
                    cbDataCompressedMax = max( g_cbPageMin, g_cbPage ) - dataSet.Cb();

                    if ( cbDataCompressedMax < dataSet.Cb() )
                    {
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
                    pfucb->pbEncryptionKey,
                    pfucb->cbEncryptionKey,
                    PinstFromPfucb( pfucb )->m_iInstance,
                    pfucb->u.pfcb->TCE() ) );

        dataSet.SetPv( pbDataEncrypted );
        dataSet.SetCb( cbDataEncryptedActual );
        grbit |= grbitSetColumnEncrypted;
    }

    err = ErrRECSetColumn( pfucb, columnid, itagSequence, &dataSet, grbit );
    if ( err < JET_errSuccess && (grbit == grbitSetColumnCompressed) )
    {
        err = ErrRECSetColumn( pfucb, columnid, itagSequence, &dataOrigSet );
    }

HandleError:
    
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


LOCAL VOID LVIGetProperLVImageFromRCE(
    PIB         * const ppib,
    const FUCB  * const pfucb,
    FUCB        * const pfucbLV,
    const BOOL  fAfterImage,
    const RCE   * const prceBase
    )
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

LOCAL VOID LVIGetProperLVImageNoRCE(
    PIB         * const ppib,
    const FUCB  * const pfucb,
    FUCB        * const pfucbLV
    )
{
    Assert( FNDVersion( pfucbLV->kdfCurr ) );

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

    Assert( rceidNull != rceidBegin
        || !FFUCBReplacePrepared( pfucb )
        || g_rgfmp[pfucb->ifmp].FVersioningOff() );

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
                    fFalse,
                    &pbImage,
                    &cbImage
                    );
    if( fImage )
    {
        pfucbLV->kdfCurr.data.SetPv( (BYTE *)pbImage );
        pfucbLV->kdfCurr.data.SetCb( cbImage );
    }
}


LOCAL VOID LVIGetProperLVImage(
    PIB         * const ppib,
    const FUCB  * const pfucb,
    FUCB        * const pfucbLV,
    const BOOL  fAfterImage,
    const RCE   * const prceBase
    )
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


ERR ErrRECRetrieveSLongField(
    FUCB            *pfucb,
    LvId            lid,
    BOOL            fAfterImage,
    ULONG           ibGraphic,
    const BOOL      fEncrypted,
    BYTE            *pb,
    ULONG           cbMax,
    ULONG           *pcbActual,
    JET_PFNREALLOC  pfnRealloc,
    void*           pvReallocContext,
    const RCE       * const prceBase
    )
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


    const DIRFLAG   dirflag         = ( prceBase ? fDIRAllNode : fDIRNull );

    
    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 65317, JET_bitTransactionReadOnly ) );
        fInTransaction = fTrue;
    }

    
    Call( ErrDIROpenLongRoot( pfucb ) );

    if( wrnLVNoLongValues == err )
    {
        if( g_fRepair )
        {
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


    const ULONG cbToPreread = (0 == ibGraphic && NULL != pb ) ? cbMax : 0;
    Call( ErrDIRDownLVRootPreread( pfucbLV, lid, dirflag, cbToPreread ) );
    if ( ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) ) || prceBase )
    {
        LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fAfterImage, prceBase );
    }

    Assert( pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT) || pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT2) );
    if ( pfucbLV->kdfCurr.data.Cb() == sizeof(LVROOT2) )
    {
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

    
    if ( ibGraphic >= ulLVSize )
    {
        if ( NULL != pcbActual )
            *pcbActual = 0;

        if ( fComparing && 0 == cbMax )
            err = ErrERRCheck( JET_errMultiValuedDuplicate );
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
            err = JET_errSuccess;
            goto HandleError;
        }
    }
    else if ( NULL == pb )
    {
        goto HandleError;
    }


    if ( pfnRealloc )
    {
        Alloc( *((BYTE**)pb) = (BYTE*)pfnRealloc( pvReallocContext, NULL, min( cbMax, ulActual ) ) );
        fFreeBuffer = fTrue;
        pb          = *((BYTE**)pb);
        cbMax       = min( cbMax, ulActual );
    }

    const BYTE * const pbMax    = pb + cbToRead;
    

    const LONG cbLVChunkMost = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    if( ibGraphic < (SIZE_T)cbLVChunkMost )
    {
        
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

    PERFOpt( PERFIncCounterTable( cLVChunkRetrieves, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

    
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
            err = JET_errSuccess;
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

    
    while ( pb < pbMax )
    {
        ulOffset += cbLVChunkMost;

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
        if ( JET_errDiskIO != err &&
                JET_errOutOfMemory != err &&
                !FErrIsDbCorruption( err ) )
        {
            CallS( err );
        }
#endif

        Call( err );

        PERFOpt( PERFIncCounterTable( cLVChunkRetrieves, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );

        Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
        if ( wrnLVNoMoreData == err )
        {
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
                err = JET_errSuccess;
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

    if ( fInTransaction )
    {
        CallS( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( pfucb->ppib );
    return err;
}


ERR ErrRECRetrieveSLongFieldPrereadOnly(
    FUCB        *pfucb,
    LvId        lid,
    const ULONG ulOffset,
    const ULONG cbMax,
    const BOOL fLazy,
    const JET_GRBIT grbit
    )
{
    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );
    ERR         err             = JET_errSuccess;
    FUCB        *pfucbLV        = pfucbNil;
    const LONG  cbLVChunkMost   = pfucb->u.pfcb->Ptdb()->CbLVChunkMost();
    ULONG       cbLVPerPage     = (( g_cbPage / cbLVChunkMost ) * cbLVChunkMost);
    ULONG       cbMaxT          = cbMax;
    if ( ( grbit & JET_bitRetrievePrereadMany ) != 0 )
    {
        cbMaxT = cpageLVPrereadMany * cbLVPerPage;
    }

    if ( cbMaxT % cbLVChunkMost > 0 )
    {
        cbMaxT = (cbMaxT / cbLVChunkMost) * cbLVChunkMost;
    }
    else if ( cbMaxT >= (ULONG) cbLVChunkMost )
    {
        cbMaxT -= cbLVChunkMost;
    }

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


ERR ErrRECRetrieveSLongFieldRefCount(
    FUCB    *pfucb,
    LvId        lid,
    BYTE    *pb,
    ULONG   cbMax,
    ULONG   *pcbActual
    )
{
    ASSERT_VALID( pfucb );
    Assert( pcbActual );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );

    ERR         err             = JET_errSuccess;
    FUCB        *pfucbLV        = pfucbNil;
    BOOL        fInTransaction  = fFalse;

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
    QWORD * const   pcbLVDataLogical,
    QWORD * const   pcbLVDataPhysical,
    QWORD * const   pcbLVOverhead )
{
    ERR             err                 = JET_errSuccess;
    FUCB *          pfucbLV             = pfucbNil;
    ULONG           cbLVSize;

    const BOOL      fAfterImage = ( !Pcsr( pfucb )->FLatched()
                                    || !FFUCBUpdateSeparateLV( pfucb )
                                    || !FFUCBReplacePrepared( pfucb ) );

    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );


    
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

    
    Call( ErrDIRDownLVRootPreread( pfucbLV, lid, fDIRNull, fLogicalOnly ? 0 : ulMax ) );
    if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
    {
        LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fFalse, prceNil );
    }

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
    while ( ( err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly ) ) >= JET_errSuccess )
    {
        if ( !fAfterImage && FNDPossiblyVersioned( pfucbLV, Pcsr( pfucbLV ) ) )
        {
            LVIGetProperLVImage( pfucbLV->ppib, pfucb, pfucbLV, fFalse, prceNil );
        }
        
        Call( ErrLVCheckDataNodeOfLid( pfucbLV, lid ) );
        if( wrnLVNoMoreData == err )
        {
            AssertSz( fFalse, "ErrDIRNext should have returned JET_errNoCurrentRecord" );
            Error( ErrERRCheck( JET_errInternalError ) );
        }
        if( ulOffset > cbLVSize )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lid, L"844a5ab7-5d88-4629-a1d0-a9811c1bf46d" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
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

ERR ErrRECDoesSLongFieldExist( FUCB * const pfucb, const LvId lid, BOOL* const pfExists )
{
    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRNoLatch( pfucb->ppib );

    ERR     err = JET_errSuccess;
    FUCB *  pfucbLV = pfucbNil;
    BOOL    fInTransaction = fFalse;


    *pfExists = fFalse;


    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 38916, JET_bitTransactionReadOnly ) );
        fInTransaction = fTrue;
    }


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

LOCAL ERR ErrLVIGetMaxLidInTree( FUCB * pfucb, FUCB * pfucbLV, __out LvId* const plid )
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
            Assert( *plid > lidMin );
            break;

        case JET_errRecordNotFound:
            *plid = lidMin;
            err = JET_errSuccess;
            break;

        default:
            Assert( err != JET_errNoCurrentRecord );
            Assert( err < 0 );
            break;
    }

    DIRUp( pfucbLV );
    return err;
}

LOCAL ERR ErrLVIGetNextLID( __in FUCB * const pfucb, __in FUCB * const pfucbLV, __out LvId * const plid )
{
    ASSERT_VALID( pfucb );
    ASSERT_VALID( pfucbLV );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( FAssertLVFUCB( pfucbLV ) );
    Assert( plid );
    *plid = 0;
    
    ERR err = JET_errSuccess;
    
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    
    TDB* const ptdb = pfucb->u.pfcb->Ptdb();
    _LID64 lidInitial = *ptdb->PUi64LidLast();
    _LID64 lidFinal = lidInitial;

    if ( lidInitial == lidMin )
    {
        LvId lvid;
        Call( ErrLVIGetMaxLidInTree( pfucb, pfucbLV, &lvid ) );
        Assert( pfucb->u.pfcb->IsUnlocked() );
        lidFinal = lvid;
    }

    if ( ptdb->FLid64() )
    {
        lidFinal = max( lid64First - 1, lidFinal );
    }

    OSSYNC_FOREVER
    {
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

    if ( !plid->FLidObeysCurrFormat( pfucb ) )
    {
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

LOCAL ERR ErrLVIInsertLVROOT( __in FUCB * const pfucbLV, __in const LvId lid, __in const LVROOT2 * const plvroot )
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
    data.SetCb( plvroot->fFlags ? sizeof(LVROOT2) : sizeof(LVROOT) );

    LVRootKeyFromLid( &lvkey, &key, lid );
    Call( ErrDIRInsert( pfucbLV, key, data, fDIRNull ) );
    
HandleError:
    return err;
}

LOCAL ERR ErrLVIInsertLVData(
    __in FUCB * const pfucbLV,
    const LvId lid,
    const ULONG ulOffset,
    __in const DATA * const pdata,
    const CompressFlags compressFlags,
    const BOOL fEncrypted,
    FUCB *pfucbTable )
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


ERR ErrRECICreateLvRootAndChunks(
    __in FUCB                   * const pfucb,
    __in const DATA             * const pdataField,
    __in const CompressFlags    compressFlags,
    __in const BOOL             fEncrypted,
    __in CPG *                  pcpgLvSpaceRequired,
    __out LvId                  * const plid,
    __in_opt FUCB               **ppfucb,
    __in_opt LVROOT2            *plvrootInit )
{
    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    ASSERT_VALID( pdataField );
    Assert( plid );
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
        Assert( pfucb->u.pfcb->FTypeSort() || pfucb->u.pfcb->FTypeTemporaryTable() );
        return ErrERRCheck( JET_errCannotSeparateIntrinsicLV );
    }
    
    Assert( !pfucb->u.pfcb->FTypeSort() );

    CallR( ErrDIROpenLongRoot( pfucb, &pfucbLV, fTrue ) );
    Assert( wrnLVNoLongValues != err );
    Assert( pfucbNil != pfucbLV );
    Assert( FFUCBLongValue( pfucbLV ) );

    BOOL fBeginTrx = fFalse;

    if ( pcpgLvSpaceRequired && *pcpgLvSpaceRequired != 0 )
    {

        if ( pfucbLV->u.pfcb->Pfcbspacehints() &&
                !pfucbLV->u.pfcb->Pfcbspacehints()->FRequestFitsExtentRange( ( *pcpgLvSpaceRequired ) ) )
        {
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }
    }

    if ( 0 == pfucb->ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 57125, NO_GRBIT ) );
        fBeginTrx = fTrue;
    }

    Call( ErrLVIGetNextLID( pfucb, pfucbLV, plid ) );
    

    dataRemaining.SetPv( pdataField->Pv() );
    dataRemaining.SetCb( pdataField->Cb() );


    ULONG ulOffset = ulLVOffsetFirst;

    if ( pcpgLvSpaceRequired && *pcpgLvSpaceRequired != 0 )
    {
        Assert( dataRemaining.Cb() > 0 );
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
        
        dataRemaining.DeltaCb( -dataInsert.Cb() );
        dataRemaining.DeltaPv( dataInsert.Cb() );

        ulOffset += cbLVChunkMost;
    }

    
    Call( ErrLVIInsertLVROOT( pfucbLV, *plid, plvrootInit ) );

    
    while( dataRemaining.Cb() > 0 )
    {

        Assert( pcpgLvSpaceRequired == NULL || *pcpgLvSpaceRequired == 0 );

        DATA dataInsert;
        dataInsert.SetPv( dataRemaining.Pv() );
        dataInsert.SetCb( min( cbLVChunkMost, dataRemaining.Cb() ) );
        
        Call( ErrLVIInsertLVData( pfucbLV, *plid, ulOffset, &dataInsert, compressFlags, fEncrypted, pfucb ) );
        cbInserted += dataInsert.Cb();
        
        dataRemaining.DeltaCb( -dataInsert.Cb() );
        dataRemaining.DeltaPv( dataInsert.Cb() );

        ulOffset += cbLVChunkMost;
    }

    err = ErrERRCheck( JET_wrnCopyLongValue );

    Assert( pdataField->Cb() == cbInserted );
HandleError:

    if ( pcpgLvSpaceRequired && CpgDIRActiveSpaceRequestReserve( pfucbLV ) )
    {
        if ( CpgDIRActiveSpaceRequestReserve( pfucbLV ) == cpgDIRReserveConsumed )
        {
            *pcpgLvSpaceRequired = 0;
        }
        DIRSetActiveSpaceRequestReserve( pfucbLV, 0 );
    }
    Assert( CpgDIRActiveSpaceRequestReserve( pfucbLV ) == 0 );

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
        }
        if ( err < JET_errSuccess )
        {
            CallSx( ErrDIRRollback( pfucb->ppib ), JET_errRollbackError );
        }
    }

    Assert( JET_errKeyDuplicate != err );

    return err;
}

ERR ErrRECSeparateLV(
    __in FUCB                   * const pfucb,
    __in const DATA             * const pdataField,
    __in const CompressFlags    compressFlags,
    __in const BOOL             fEncrypted,
    __out LvId                  * const plid,
    __in_opt FUCB               **ppfucb,
    __in_opt LVROOT2            *plvrootInit )
{
    return ErrRECICreateLvRootAndChunks( pfucb, pdataField, compressFlags, fEncrypted, NULL, plid, ppfucb, plvrootInit );
}

ERR ErrRECAffectSeparateLV( FUCB *pfucb, LvId *plid, ULONG fLV )
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

    Call( ErrDIRDownLVRoot( pfucbLV, *plid, fDIRNull ) );

    Assert( locOnCurBM == pfucbLV->locLogical );
    Assert( Pcsr( pfucbLV )->FLatched() );

    if ( fLVDereference == fLV )
    {
        const LONG      lDelta      = -1;
        KEYDATAFLAGS    kdf;

        err = ErrDIRGetLock( pfucb, writeLock );
        if( err == JET_errWriteConflict )
        {
            Call( err );
        }

        NDIGetKeydataflags( Pcsr( pfucbLV )->Cpage(), Pcsr( pfucbLV )->ILine(), &kdf );
        Assert( sizeof( LVROOT ) == kdf.data.Cb() || sizeof( LVROOT2 ) == kdf.data.Cb() );

        const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
        const LONG cbLV = plvroot->ulSize;

        if ( 0 == plvroot->ulReference )
        {
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

        const LONG lDelta   = 1;
        err = ErrDIRDelta< LONG >(
                pfucbLV,
                OffsetOf( LVROOT, ulReference ),
                lDelta,
                NULL,
                fDIRNull | fDIRDeltaDeleteDereferencedLV );
        if ( JET_errWriteConflict == err )
        {
            Call( ErrDIRGet( pfucbLV ) );
            Call( ErrRECIBurstSeparateLV( pfucb, pfucbLV, plid ) );
        }
    }

HandleError:
    Assert( pfucbNil != pfucbLV );
    DIRCloseLongRoot( pfucb );

    return err;
}

ERR ErrRECDeleteLV_LegacyVersioned( FUCB *pfucbLV, const ULONG ulLVDeleteOffset, const DIRFLAG dirflag )
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

        Assert( 0 == ulLVDeleteOffset % pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() );

        Assert( ( (LVROOT *)( pfucbLV->kdfCurr.data.Pv() ) )->ulSize == ulLVDeleteOffset );
        Assert( pfucbLV->ppib->Level() > 0 );

        Call( ErrDIRDownLVData( pfucbLV, lidDelete, ulLVDeleteOffset, dirflag ) );
    }
    else
    {
        PERFOpt( PERFIncCounterTable( cLVDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
    }

    for( ; ; )
    {
        Call( ErrDIRDelete( pfucbLV, dirflag ) );

        PERFOpt( PERFIncCounterTable( cLVChunkDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
        cChunksDeleted++;

        Assert( 0 == ulLVDeleteOffset || cChunksDeleted <= DELETELVTASK::g_cpgLVDataToDeletePerTrx );
        if ( ulLVDeleteOffset > 0
            && cChunksDeleted >= DELETELVTASK::g_cpgLVDataToDeletePerTrx )
        {
#ifdef DEBUG
            const ERR   errT    = ErrDIRNext( pfucbLV, fDIRSameLIDOnly );
            Assert( JET_errNoCurrentRecord == errT );
#endif
            break;
        }

        err = ErrDIRNext( pfucbLV, fDIRSameLIDOnly );
        if ( err < JET_errSuccess )
        {
            if ( JET_errNoCurrentRecord == err )
            {
                err = JET_errSuccess;
            }
            goto HandleError;
        }
        CallS( err );


        LvId lidT;
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if ( lidDelete != lidT )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lidDelete, L"0c37a830-b7b6-4e60-ab81-0e45c8184ae1" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }
    }

    CallS( err );

HandleError:
    Assert( JET_errNoCurrentRecord != err );
    Assert( JET_errRecordNotFound != err );
    Assert( pfucbNil != pfucbLV );
    return err;
}

ERR ErrRECDeleteLV_SynchronousCleanup( FUCB *pfucbLV, const ULONG cChunksToDelete )
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

    for( cChunksDeleted = 0; cChunksDeleted < cChunksToDelete || cChunksToDelete == 0; cChunksDeleted++ )
    {

        Call( ErrDIRDownLVRootPreread( pfucbLV, lidDelete, dirflag, 0, fFalse ) );

        err = ErrDIRNext( pfucbLV, dirnextflag );
        if( JET_errNoCurrentRecord == err )
        {
            if( !( dirnextflag & fDIRAllNode ) && cChunksDeleted < cChunksToDelete - 1 )
            {
                dirnextflag |= fDIRAllNode;
                cChunksDeleted -= 1;
                continue;
            }
            else
            {
                fDeletingRoot = fTrue;
                dirflag = fDIRNull;
                Call( ErrDIRDownLVRootPreread( pfucbLV, lidDelete, dirflag, 0, fFalse ) );
            }
        }
        else if( err < JET_errSuccess )
        {
            Call( err );
        }
        CallS( err );

        LvId lidT;
        LidFromKey( &lidT, pfucbLV->kdfCurr.key );
        if( lidDelete != lidT )
        {
            LVReportAndTrapCorruptedLV( pfucbLV, lidDelete, L"d6d29ea0-caad-41c5-8310-92956573d89c" );
            Error( ErrERRCheck( JET_errLVCorrupted ) );
        }

        Call( ErrDIRDelete( pfucbLV, dirflag ) );

        if( fDeletingRoot )
        {
            Assert( fDIRNull == dirflag );
            break;
        }

        Call( ErrFaultInjection( 39454 ) );

        Call( ErrBTDelete( pfucbLV, pfucbLV->bmCurr ) );

        PERFOpt( PERFIncCounterTable( cLVChunkDeletes, PinstFromPfucb( pfucbLV ), TceFromFUCB( pfucbLV ) ) );
    }

HandleError:
    Assert( pfucbNil != pfucbLV );
    return err;
}

ERR ErrRECAffectLongFieldsInWorkBuf( FUCB *pfucb, LVAFFECT lvaffect, ULONG cbThreshold )
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
        UtilMemCpy( pfucb->dataWorkBuf.Pv(), pvWorkBufSav, cbWorkBufSav );
        pfucb->dataWorkBuf.SetCb( cbWorkBufSav );
        BFFree( pvWorkBufSav );
    }

    return err;
}


ERR ErrRECDereferenceLongFieldsInRecord( FUCB *pfucb )
{
    ERR                 err;

    ASSERT_VALID( pfucb );
    Assert( !FAssertLVFUCB( pfucb ) );
    Assert( pfcbNil != pfucb->u.pfcb );

    Assert( pfucb->ppib->Level() > 0 );

    AssertDIRNoLatch( pfucb->ppib );

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
    AssertDIRNoLatch( pfucb->ppib );
    return err;
}




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

    }


HandleError:
    return err;
}


ERR ErrCMPGetSLongFieldFirst(
    FUCB    *pfucb,
    FUCB    **ppfucbGetLV,
    LvId    *plid,
    LVROOT2 *plvroot )
{
    ERR     err         = JET_errSuccess;
    FUCB    *pfucbGetLV = pfucbNil;

    Assert( pfucb != pfucbNil );


    CallR( ErrDIROpenLongRoot( pfucb, &pfucbGetLV, fFalse ) );
    if ( wrnLVNoLongValues == err )
    {
        Assert( pfucbNil == pfucbGetLV );
        *ppfucbGetLV = pfucbNil;
        return ErrERRCheck( JET_errRecordNotFound );
    }

    Assert( pfucbNil != pfucbGetLV );
    Assert( FFUCBLongValue( pfucbGetLV ) );


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

    if ( pfucbGetLV != pfucbNil )
    {
        DIRClose( pfucbGetLV );
    }

    return err;
}


ERR ErrCMPGetSLongFieldNext(
    FUCB    *pfucbGetLV,
    LvId    *plid,
    LVROOT2 *plvroot )
{
    ERR     err = JET_errSuccess;

    Assert( pfucbNil != pfucbGetLV );


    CallR( ErrDIRNext( pfucbGetLV, fDIRNull ) );

    err = ErrCMPGetReferencedSLongField( pfucbGetLV, plid, plvroot );
    Assert( JET_errRecordNotFound != err );

    return err;
}

VOID CMPGetSLongFieldClose( FUCB *pfucbGetLV )
{
    Assert( pfucbGetLV != pfucbNil );
    DIRClose( pfucbGetLV );
}

ERR ErrCMPRetrieveSLongFieldValueByChunk(
    FUCB        *pfucbGetLV,
    const LvId  lid,
    const ULONG cbTotal,
    ULONG       ibLongValue,
    BYTE        *pbBuf,
    const ULONG cbMax,
    ULONG       *pcbReturnedPhysical )
{
    ERR         err;

    *pcbReturnedPhysical = 0;

#ifdef DEBUG
    if ( 0 == ibLongValue )
    {
        CallS( ErrDIRGet( pfucbGetLV ) );
        AssertLVRootNode( pfucbGetLV, lid );
        Assert( cbTotal == (reinterpret_cast<LVROOT*>( pfucbGetLV->kdfCurr.data.Pv() ))->ulSize );
    }
#endif

    const LONG cbLVChunkMost = pfucbGetLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
    Assert( ibLongValue % cbLVChunkMost == 0 );

    CallR( ErrDIRNext( pfucbGetLV, fDIRNull ) );

    Call( ErrLVCheckDataNodeOfLid( pfucbGetLV, lid ) );
    if ( wrnLVNoMoreData == err )
    {
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
    Assert( wrnLVNoLongValues != err );

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
    else if ( ulRefcountOld == ulRefcountNew )
    {
    }
#endif
    else
    {
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
    if ( FIsLVRootKey( pfucbLV->kdfCurr.key ) )
    {
        Assert( sizeof(LVROOT) == pfucbLV->kdfCurr.data.Cb() || sizeof(LVROOT2) == pfucbLV->kdfCurr.data.Cb() );

        LidFromKey( plidCurr, pfucbLV->kdfCurr.key );
        Assert( *plidCurr > *plidPrev );
        *plidPrev = *plidCurr;

        Assert( *pulSizeCurr == *pulSizeSeen );
        *pulSizeCurr = (reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ))->ulSize;
        *pulSizeSeen = 0;

        if ( 0 == ( reinterpret_cast<LVROOT*>( pfucbLV->kdfCurr.data.Pv() ) )->ulReference )
        {
            Assert( 0 == *pulSizeSeen );
        }

    }
    else
    {
        Assert( *plidCurr > lidMin );

        LvId    lid         = lidMin;
        ULONG   ulOffset    = 0;
        LidOffsetFromKey( &lid, &ulOffset, pfucbLV->kdfCurr.key );
        Assert( lid == *plidCurr );
        const LONG cbLVChunkMost = pfucbLV->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost();
        Assert( ulOffset % cbLVChunkMost == 0 );
        Assert( ulOffset == *pulSizeSeen );

        ULONG cbChunk;
        CallS( ErrLVIGetDataSize( pfucbLV, pfucbLV->kdfCurr.key, pfucbLV->kdfCurr.data, fFalse, *pulSizeCurr, &cbChunk ) );
        *pulSizeSeen += cbChunk;
        Assert( *pulSizeSeen <= *pulSizeCurr );
        Assert( (ULONG)cbLVChunkMost == cbChunk
            || *pulSizeSeen == *pulSizeCurr );

    }
}

#endif


RECCHECKLV::RECCHECKLV( TTMAP& ttmap, const REPAIROPTS * m_popts, LONG cbLVChunkMost ) :
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


RECCHECKLV::~RECCHECKLV()
{
}


ERR RECCHECKLV::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
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
#endif
        err = m_ttmap.ErrSetValue( m_lidCurr, m_ulReferenceCurr );
#ifdef SYNC_DEADLOCK_DETECTION
        Pcls()->pownerLockHead = pownerSaved;
#endif
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


ERR RECCHECKLV::ErrTerm()
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


RECCHECKLVSTATS::RECCHECKLVSTATS( LVSTATS * plvstats ) :
    m_plvstats( plvstats ),
    m_pgnoLastRoot( pgnoNull )
{
}


RECCHECKLVSTATS::~RECCHECKLVSTATS()
{
}


ERR RECCHECKLVSTATS::operator()( const KEYDATAFLAGS& kdf, const PGNO pgno )
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


ERR ErrREPAIRCheckLV(
    FUCB * const pfucb,
    LvId * const plid,
    ULONG * const pulRefcount,
    ULONG * const pulSize,
    BOOL * const pfLVHasRoot,
    BOOL * const pfLVComplete,
    BOOL * const pfLVPartiallyScrubbed,
    BOOL * const pfDone )
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
            break;
        }

        if( !FIsLVChunkKey( pfucb->kdfCurr.key ) )
        {
            goto HandleError;
        }

        ULONG   ulOffset;
        LidOffsetFromKey( &lidT, &ulOffset, pfucb->kdfCurr.key );
        Assert( ulOffset % pfucb->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() == 0 );
        if( ( ulOffset != ulSizeSeen ) && !(*pfLVPartiallyScrubbed) )
        {
            goto HandleError;
        }

        ULONG cbDecompressed;
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

        if ( !(*pfLVPartiallyScrubbed) )
        {
            ulSizeSeen += cbDecompressed;

            if( (ULONG)pfucb->u.pfcb->PfcbTable()->Ptdb()->CbLVChunkMost() != cbDecompressed
                && ulSizeSeen < *pulSize
                && *pfLVHasRoot )
            {
                goto HandleError;
            }

            if( ulSizeSeen > *pulSize
                && *pfLVHasRoot )
            {
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


LOCAL ERR ErrREPAIRDownLV( FUCB * pfucb, LvId lid )
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
    }
    else if( fFoundLess )
    {
        Assert( locBeforeSeekBM == pfucb->locLogical );
        Call( ErrDIRNext( pfucb, fDIRNull ) );
    }
    else if( fFoundGreater )
    {
        Assert( locAfterSeekBM == pfucb->locLogical );
        pfucb->locLogical = locOnCurBM;
    }

HandleError:
    return err;
}


ERR ErrREPAIRDeleteLV( FUCB * pfucb, LvId lid )
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
#endif

    Call(ErrRECDeleteLV_LegacyVersioned(pfucb, 0, fDIRNoVersion));

HandleError:
    return err;
}


ERR ErrREPAIRCreateLVRoot( FUCB * const pfucb, const LvId lid, const ULONG ulRefcount, const ULONG ulSize )
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


ERR ErrREPAIRNextLV( FUCB * pfucb, LvId lidCurr, BOOL * pfDone )
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
#endif

HandleError:
    return err;
}


ERR ErrREPAIRUpdateLVRefcount(
    FUCB        * const pfucbLV,
    const LvId  lid,
    const ULONG ulRefcountOld,
    const ULONG ulRefcountNew )
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

LOCAL ERR ErrSCRUBIZeroLV(  PIB * const     ppib,
                            const IFMP      ifmp,
                            CSR * const     pcsr,
                            const INT       iline,
                            const LvId      lid,
                            const ULONG     ulSize,
                            const BOOL      fCanChangeCSR )
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
                break;
            }

            auto tc = TcCurr( );
            Expected( tc.iorReason.Iort() == iortScrubbing );

            if ( !fCanChangeCSR )
            {
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
                pcsr->Downgrade( latchRIW );

                CallR( pcsr->ErrSwitchPage( ppib, ifmp, pgnoNext ) );
                pcsr->UpgradeFromRIWLatch();

                BFDirty( pcsr->Cpage().PBFLatch(), bfdfDirty, tc );

                ilineT = 0;
            }
        }

        NDIGetKeydataflags( pcsr->Cpage(), ilineT, &kdf );

        if( FIsLVRootKey( kdf.key ) )
        {
            if( sizeof( LVROOT ) != kdf.data.Cb() && sizeof( LVROOT2 ) != kdf.data.Cb() )
            {
                ExpectedSz( fFalse, "Invalid LVROOT size (%d bytes).", kdf.data.Cb() );

                if ( !FNDDeleted( kdf ) )
                {
                    LvId lidT;
                    LidFromKey( &lidT, kdf.key );
                    LVReportAndTrapCorruptedLV2( PinstFromIfmp( ifmp ), g_rgfmp[ifmp].WszDatabaseName(), "", pcsr->Pgno(), lidT, L"5482999a-362e-48e5-bd04-a09c934aec73" );
                    return ErrERRCheck( JET_errLVCorrupted );
                }
            }
            break;
        }

        else if( !FIsLVChunkKey( kdf.key ) )
        {
            LVReportAndTrapCorruptedLV2( PinstFromIfmp( ifmp ), g_rgfmp[ifmp].WszDatabaseName(), "", pcsr->Pgno(), 0, L"7b30e61f-46c6-48d0-9e3d-3d5c6c6454f0" );
            return ErrERRCheck( JET_errLVCorrupted );
        }

        LvId lidT;
        LidFromKey( &lidT, kdf.key );
        if( lid != lidT )
        {
            break;
        }

        ULONG ulOffset;
        OffsetFromKey( &ulOffset, kdf.key );
        if( ulOffset > ulSize && !FNDDeleted( kdf ) )
        {
            LVReportAndTrapCorruptedLV2( PinstFromIfmp( ifmp ), g_rgfmp[ifmp].WszDatabaseName(), "", pcsr->Pgno(), lidT, L"8ab78a02-ca42-4de4-9d89-df3111a367fa" );
            return ErrERRCheck( JET_errLVCorrupted );
        }

        Assert( pcsr->FDirty() );
        CallS( ErrRECScrubLVChunk( kdf.data, chSCRUBLegacyLVChunkFill ) );
    }

    return JET_errSuccess;
}


ERR ErrSCRUBZeroLV( PIB * const ppib,
                    const IFMP ifmp,
                    CSR * const pcsr,
                    const INT iline )
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

            pbtsLvCtx->pLvData->cLVRefs += FPartiallyDeletedLV( pbtsLvCtx->cRefsCurrent ) ? 0 : pbtsLvCtx->cRefsCurrent;
        }
        else if ( pkdf->key.Cb() == sizeof( LVKEY64 ) || pkdf->key.Cb() == sizeof( LVKEY32 ) )
        {
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
                    pbtsLvCtx->pLvData->cSeparatedRootChunks++;
                }
            }

            pbtsLvCtx->cbAccumLogical += cbUncompressedExpected;
            ULONG cbCompressed = pkdf->data.Cb();
            pbtsLvCtx->cbAccumActual += cbCompressed;

            const CPG cpgAccumMax = cpgPrereadSequential;
            const INT cbRunMax = 1 << 20;
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
                            pgnoRunStart = pgnoCurr;
                            pbtsLvCtx->cRunsAccessed++;
                            pbtsLvCtx->cpgAccessed++;
                        }
                        else
                        {
                            pbtsLvCtx->cpgAccessed += pgnoCurr - pgnoLast;
                        }
                    }
                    else
                    {
                        if ( (ULONG64)( pgnoCurr - pgnoRunStart + 1 ) * g_cbPage > cbRunMax )
                        {
                            pgnoRunStart = pgnoCurr;
                            pbtsLvCtx->cRunsAccessed++;
                        }

                        pbtsLvCtx->cpgAccessed++;
                    }
                    pgnoLast = pgnoCurr;
                }
                pbtsLvCtx->cpgAccum = 0;
            }
            if ( pbtsLvCtx->cbAccumLogical == pbtsLvCtx->cbCurrent )
            {
                ULONG64 cbAccessedPerPage = ( g_cbPage / pbtsLvCtx->pLvData->cbLVChunkMax ) * pbtsLvCtx->pLvData->cbLVChunkMax;

                ULONG64 cpgAccessedExpected = ( pbtsLvCtx->cbAccumActual + cbAccessedPerPage - 1 ) / cbAccessedPerPage;

                ULONG64 cRunsAccessedExpected = ( cpgAccessedExpected * g_cbPage + cbRunMax - 1 ) / cbRunMax;

                if ( (ULONG64)cpgAccessedExpected * g_cbPage > UlParam( JET_paramMaxCoalesceReadSize ) && pbtsLvCtx->cRunsAccessed > cRunsAccessedExpected )
                {
                    cRunsAccessedExpected++;
                }

                if ( ibOffset > 0 && pbtsLvCtx->cpgAccessed > cpgAccessedExpected )
                {
                    cpgAccessedExpected++;
                }

                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVSeeks )->ErrAddSample( pbtsLvCtx->cRunsAccessed ) ) );
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVBytes )->ErrAddSample( pbtsLvCtx->cpgAccessed * g_cbPage ) ) );

                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVExtraSeeks )->ErrAddSample( pbtsLvCtx->cRunsAccessed - cRunsAccessedExpected ) ) );
                Call( ErrFromCStatsErr( CStatsFromPv( pbtsLvCtx->pLvData->phistoLVExtraBytes )->ErrAddSample( ( pbtsLvCtx->cpgAccessed - cpgAccessedExpected ) * g_cbPage ) ) );
            }

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
    }

HandleError:
    return err;
}

#endif



