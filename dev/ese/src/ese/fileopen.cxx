// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

extern CAutoResetSignal sigDoneFCB;

ERR VTAPI ErrIsamDupCursor( JET_SESID sesid, JET_VTID vtid, JET_TABLEID  *ptableid, ULONG grbit )
{
    PIB *ppib       = reinterpret_cast<PIB *>( sesid );
    FUCB *pfucbOpen = reinterpret_cast<FUCB *>( vtid );
    FUCB **ppfucb   = reinterpret_cast<FUCB **>( ptableid );

    ERR     err;
    FUCB    *pfucb;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucbOpen );

    Call( ErrDIROpen( ppib, pfucbOpen->u.pfcb, &pfucb ) );

    pfucb->pvWorkBuf = NULL;
    pfucb->dataWorkBuf.SetPv( NULL );
    Assert( !FFUCBUpdatePrepared( pfucb ) );

    pfucb->dataSearchKey.Nullify();
    pfucb->cColumnsInSearchKey = 0;
    KSReset( pfucb );

    FUCBSetIndex( pfucb );
    if ( FFUCBUpdatable( pfucbOpen ) )
    {
        FUCBSetUpdatable( pfucb );
        Assert( !g_rgfmp[pfucb->ifmp].FReadOnlyAttach() );
    }
    else
    {
        FUCBResetUpdatable( pfucb );
    }

    Assert( !FFUCBMayCacheLVCursor( pfucb ) );
    if ( FFUCBMayCacheLVCursor( pfucbOpen ) )
    {
        FUCBSetMayCacheLVCursor( pfucb );
    }

    RECDeferMoveFirst( ppib, pfucb );
    err = JET_errSuccess;

    pfucb->pvtfndef = &vtfndefIsam;
    *ppfucb = pfucb;

    return JET_errSuccess;

HandleError:
    return err;
}



const ULONG cbAvgName   = 16;

ERR ErrTDBCreate(
    INST        *pinst,
    IFMP        ifmp,
    TDB         **pptdb,
    TCIB        *ptcib,
    const BOOL  fSystemTDB,
    FCB         *pfcbTemplateTable,
    const BOOL  fAllocateNameSpace )
{
    ERR     err;
    WORD    cfieldFixed;
    WORD    cfieldVar;
    WORD    cfieldTagged;
    WORD    cfieldTotal;
    TDB     *ptdb = ptdbNil;
    FID     fidFixedFirst;
    FID     fidVarFirst;
    FID     fidTaggedFirst;
    WORD    ibEndFixedColumns;

    Assert( pptdb != NULL );
    Assert( ptcib->fidFixedLast <= fidFixedMost );
    Assert( ptcib->fidVarLast >= fidVarLeast-1 && ptcib->fidVarLast <= fidVarMost );
    Assert( ptcib->fidTaggedLast >= fidTaggedLeast-1 && ptcib->fidTaggedLast <= fidTaggedMost );

    if ( pfcbNil == pfcbTemplateTable )
    {
        fidFixedFirst = fidFixedLeast;
        fidVarFirst = fidVarLeast;
        fidTaggedFirst = fidTaggedLeast;
        ibEndFixedColumns = ibRECStartFixedColumns;
    }
    else
    {
        fidFixedFirst = FID( pfcbTemplateTable->Ptdb()->FidFixedLast() + 1 );
        if ( fidFixedLeast-1 == ptcib->fidFixedLast )
        {
            ptcib->fidFixedLast = FID( fidFixedFirst - 1 );
        }
        Assert( ptcib->fidFixedLast >= fidFixedFirst-1 );

        fidVarFirst = FID( pfcbTemplateTable->Ptdb()->FidVarLast() + 1 );
        if ( fidVarLeast-1 == ptcib->fidVarLast )
        {
            ptcib->fidVarLast = FID( fidVarFirst - 1 );
        }
        Assert( ptcib->fidVarLast >= fidVarFirst-1 );

        fidTaggedFirst = ( 0 != pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template() ?
                            FID( pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template() + 1 ) :
                            fidTaggedLeast );
        if ( fidTaggedLeast-1 == ptcib->fidTaggedLast )
        {
            ptcib->fidTaggedLast = FID( fidTaggedFirst - 1 );
        }
        Assert( ptcib->fidTaggedLast >= fidTaggedFirst-1 );

        ibEndFixedColumns = pfcbTemplateTable->Ptdb()->IbEndFixedColumns();
    }

    cfieldFixed = WORD( ptcib->fidFixedLast + 1 - fidFixedFirst );
    cfieldVar = WORD( ptcib->fidVarLast + 1 - fidVarFirst );
    cfieldTagged = WORD( ptcib->fidTaggedLast + 1 - fidTaggedFirst );
    cfieldTotal = WORD( cfieldFixed + cfieldVar + cfieldTagged );

    C_ASSERT( sizeof(TDB) % 16 == 0 );

    ptdb = new( pinst ) TDB(    pinst,
                                fidFixedFirst,
                                ptcib->fidFixedLast,
                                fidVarFirst,
                                ptcib->fidVarLast,
                                fidTaggedFirst,
                                ptcib->fidTaggedLast,
                                ibEndFixedColumns,
                                pfcbTemplateTable,
                                fSystemTDB ? 0 : 1 );
    if ( !ptdb )
    {
        return ErrERRCheck( JET_errTooManyOpenTables );
    }

    if ( JET_errSuccess == PfmpFromIfmp( ifmp )->ErrDBFormatFeatureEnabled( JET_efvLid64 ) )
    {
        ptdb->m_fLid64 = fTrue;
    }
    else
    {
        ptdb->m_fLid64 = fFalse;
    }


    if ( pfcbNil != pfcbTemplateTable )
    {
        const TDB   * const ptdbTemplate = pfcbTemplateTable->Ptdb();

        if ( ptdbTemplate->FTableHasFinalizeColumn() )
        {
            ptdb->SetFTableHasFinalizeColumn();
        }
        if ( ptdbTemplate->FTableHasDeleteOnZeroColumn() )
        {
            ptdb->SetFTableHasDeleteOnZeroColumn();
        }
        if ( ptdbTemplate->FTableHasDefault() )
        {
            ptdb->SetFTableHasDefault();

            if ( ptdbTemplate->FTableHasUserDefinedDefault() )
            {
                ptdb->SetFTableHasUserDefinedDefault();
            }
            if ( ptdbTemplate->FTableHasNonEscrowDefault() )
            {
                ptdb->SetFTableHasNonEscrowDefault();
            }
        }
        else
        {
            Assert( !ptdbTemplate->FTableHasUserDefinedDefault() );
            Assert( !ptdbTemplate->FTableHasNonEscrowDefault() );
        }
    }

    if ( fAllocateNameSpace )
    {
        Call( ptdb->MemPool().ErrMEMPOOLInit(
            cbAvgName * ( cfieldTotal + 1 ),
            USHORT( cfieldTotal + 2 ),
            fTrue ) );
    }
    else
    {
        Call( ptdb->MemPool().ErrMEMPOOLInit( cbFIELDPlaceholder, 3, fFalse ) );
    }

    MEMPOOL::ITAG   itagNew;
    Call( ptdb->MemPool().ErrAddEntry( NULL, cbFIELDPlaceholder, &itagNew ) );
    Assert( itagNew == itagTDBFields );

    const ULONG     cbFieldInfo     = cfieldTotal * sizeof(FIELD);
    if ( cbFieldInfo > 0 )
    {
        FIELD * const   pfieldInitial   = (FIELD *)PvOSMemoryHeapAlloc( cbFieldInfo );
        if ( NULL == pfieldInitial )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        memset( pfieldInitial, 0, cbFieldInfo );
        ptdb->SetPfieldInitial( pfieldInitial );
    }
    else
    {
        ptdb->SetPfieldInitial( NULL );
    }

    *pptdb = ptdb;
    return JET_errSuccess;

HandleError:
    delete ptdb;
    return err;
}



ERR ErrRECAddFieldDef( TDB *ptdb, const COLUMNID columnid, FIELD *pfield )
{
    FIELD *     pfieldNew;
    const FID   fid         = FidOfColumnid( columnid );

    Assert( ptdb != ptdbNil );
    Assert( pfield != pfieldNil );

    Assert( FCOLUMNIDValid( columnid ) );

    if ( FCOLUMNIDTagged( columnid ) )
    {
        if ( fid > ptdb->FidTaggedLast() || fid < ptdb->FidTaggedFirst() )
            return ErrERRCheck( JET_errColumnNotFound );

        Assert( 0 == pfield->ibRecordOffset );
        pfieldNew = ptdb->PfieldTagged( columnid );
    }
    else if ( FCOLUMNIDFixed( columnid ) )
    {
        if ( fid > ptdb->FidFixedLast() || fid < ptdb->FidFixedFirst() )
            return ErrERRCheck( JET_errColumnNotFound );

        Assert( pfield->ibRecordOffset >= ibRECStartFixedColumns );
        pfieldNew = ptdb->PfieldFixed( columnid );
    }

    else
    {
        Assert( FCOLUMNIDVar( columnid ) );

        if ( fid > ptdb->FidVarLast() || fid < ptdb->FidVarFirst() )
            return ErrERRCheck( JET_errColumnNotFound );
        else if ( pfield->coltyp != JET_coltypBinary && pfield->coltyp != JET_coltypText )
            return ErrERRCheck( JET_errInvalidColumnType );

        Assert( 0 == pfield->ibRecordOffset );
        pfieldNew = ptdb->PfieldVar( columnid );
    }

    *pfieldNew = *pfield;

    return JET_errSuccess;
}


void FILEIMinimizePrimaryCbKeyMost( FCB * pfcb, IDB * const pidb )
{
    const IDXSEG    *pidxseg;
    const IDXSEG    *pidxsegMac;

    Assert( pfcb->Ptdb() != ptdbNil );
    Assert( pidb != pidbNil );
    Assert( pidb->Cidxseg() > 0 );
    Assert( pidb->FPrimary() );
    Assert( !pidb->FTuples() );

    const ULONG     cbKeyMost       = pidb->CbKeyMost();
    const ULONG     cbKeySegmentHeader = 1;
    
    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= cbKeyMost );
    const BOOL      fVarSegMac      = ( cbKeyMost != pidb->CbVarSegMac() );
    const ULONG     cbVarSegMac     = pidb->CbVarSegMac();
    Assert( cbVarSegMac > 0 );
    Assert( cbVarSegMac <= cbKeyMost );

    ULONG           cbActualKeyMost = 0;

    if ( pidb->FTemplateIndex() )
    {
        Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
        if ( pfcb->FDerivedTable() )
        {
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
            Assert( pfcbNil != pfcb );
            Assert( pfcb->FTemplateTable() );
        }
        else
        {
            Assert( pfcb->Ptdb()->PfcbTemplateTable() == pfcbNil );
        }
    }

    pidxseg = PidxsegIDBGetIdxSeg( pidb, pfcb->Ptdb() );
    pidxsegMac = pidxseg + pidb->Cidxseg();
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        FIELD           field;
        const COLUMNID  columnid    = pidxseg->Columnid();
        const BOOL      fFixedField = FCOLUMNIDFixed( columnid );

        field = *( pfcb->Ptdb()->Pfield( columnid ) );

        Assert( !FFIELDDeleted( field.ffield ) );
        Assert( JET_coltypNil != field.coltyp );
        Assert( !FCOLUMNIDVar( columnid ) || field.coltyp == JET_coltypBinary || field.coltyp == JET_coltypText );
        Assert( !FFIELDMultivalued( field.ffield ) || FCOLUMNIDTagged( columnid ) );

        switch ( field.coltyp )
        {
            case JET_coltypBit:
            case JET_coltypUnsignedByte:
            case JET_coltypShort:
            case JET_coltypUnsignedShort:
            case JET_coltypLong:
            case JET_coltypUnsignedLong:
            case JET_coltypLongLong:
            case JET_coltypUnsignedLongLong:
            case JET_coltypCurrency:
            case JET_coltypIEEESingle:
            case JET_coltypIEEEDouble:
            case JET_coltypDateTime:
            case JET_coltypGUID:
                Assert( field.cbMaxLen == UlCATColumnSize( field.coltyp, 0, NULL ) );
                cbActualKeyMost += ( cbKeySegmentHeader + field.cbMaxLen );
                break;

            case JET_coltypText:
            case JET_coltypLongText:
                if ( fVarSegMac )
                {
                    cbActualKeyMost += cbVarSegMac;
                }
                else
                {
                    return;
                }
                break;

            case JET_coltypBinary:
            case JET_coltypLongBinary:
                if ( fVarSegMac )
                {
                    if ( fFixedField )
                    {
                        cbActualKeyMost += min( ( cbKeySegmentHeader + field.cbMaxLen ), cbVarSegMac );
                    }
                    else
                    {
                        cbActualKeyMost += cbVarSegMac;
                    }
                }
                else
                {
                    if ( fFixedField )
                    {
                        cbActualKeyMost += ( cbKeySegmentHeader + field.cbMaxLen );
                    }
                    else
                    {

                        return;
                    }
                }
                break;

            default:
                Assert( fFalse );
                break;
        }
    }

    if ( cbActualKeyMost < cbKeyMost )
    {
        if ( cbActualKeyMost < cbVarSegMac )
        {
            Assert( cbVarSegMac == cbKeyMost );
            pidb->SetCbVarSegMac( (USHORT)cbActualKeyMost );
        }

        pidb->SetCbKeyMost( (USHORT)cbActualKeyMost );
    }
    
    return;
}


ERR ErrFILEIGenerateIDB( FCB *pfcb, TDB *ptdb, IDB *pidb )
{
    const IDXSEG*   pidxseg;
    const IDXSEG*   pidxsegMac;
    BOOL            fFoundUserDefinedDefault    = fFalse;

    Assert( pfcb != pfcbNil );
    Assert( ptdb != ptdbNil );
    Assert( pidb != pidbNil );

    Assert( (cbitFixed % 8) == 0 );
    Assert( (cbitVariable % 8) == 0 );
    Assert( (cbitTagged % 8) == 0 );

    Assert( pidb->Cidxseg() <= JET_ccolKeyMost );

    memset( pidb->RgbitIdx(), 0x00, 32 );

    pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    pidxsegMac = pidxseg + pidb->Cidxseg();

    if ( !pidb->FAllowFirstNull() )
    {
        pidb->ResetFAllowAllNulls();
    }



    const BOOL  fIgnoreAllNull      = !pidb->FAllowAllNulls();
    const BOOL  fIgnoreFirstNull    = !pidb->FAllowFirstNull();
    const BOOL  fIgnoreAnyNull      = !pidb->FAllowSomeNulls();
    BOOL        fSparseIndex        = ( !pidb->FPrimary()
                                        && !pidb->FNoNullSeg()
                                        && ( fIgnoreAllNull || fIgnoreFirstNull || fIgnoreAnyNull ) );

    Assert( !pidb->FPrimary()
        || pidb->FNoNullSeg()
        || ( pidb->FAllowAllNulls() && pidb->FAllowFirstNull() && pidb->FAllowSomeNulls() ) );

    const IDXSEG * const    pidxsegFirst    = pidxseg;
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        const COLUMNID          columnid    = pidxseg->Columnid();
        const FIELD * const     pfield      = ptdb->Pfield( columnid );
        Assert( pfield->coltyp != JET_coltypNil );
        Assert( !FFIELDDeleted( pfield->ffield ) );

        if ( FFIELDUserDefinedDefault( pfield->ffield ) )
        {
            if ( !fFoundUserDefinedDefault )
            {
                memset( pidb->RgbitIdx(), 0xff, 32 );
                fFoundUserDefinedDefault = fTrue;
            }

            if ( !fIgnoreFirstNull
                || fIgnoreAnyNull
                || pidxsegFirst == pidxseg )
            {
                fSparseIndex = fFalse;
            }
        }

        if ( FFIELDNotNull( pfield->ffield ) || FFIELDDefault( pfield->ffield ) )
        {
            if ( !fIgnoreFirstNull
                || fIgnoreAnyNull
                || pidxsegFirst == pidxseg )
            {
                fSparseIndex = fFalse;
            }
        }

        if ( FCOLUMNIDTagged( columnid ) )
        {
            Assert( !FFIELDPrimaryIndexPlaceholder( pfield->ffield ) );

            if ( FFIELDMultivalued( pfield->ffield ) )
            {
                Assert( !pidb->FPrimary() );

                if ( !pidb->FMultivalued() )
                {
                    Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulVersion == 0x00000620 );
                    Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulDaeUpdateMajor < 0x00000002 );
                }

                pidb->SetFMultivalued();
            }
        }
        else if ( FFIELDPrimaryIndexPlaceholder( pfield->ffield )
            && pidb->FPrimary() )
        {
            Assert( PidxsegIDBGetIdxSeg( pidb, ptdb ) == pidxseg );

            Assert( FCOLUMNIDFixed( columnid ) );
            Assert( JET_coltypBit == pfield->coltyp );
            pidb->SetFHasPlaceholderColumn();
        }

        if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
        {
            if ( !pidb->FLocalizedText() )
            {
                Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulVersion == 0x00000620 );
                Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulDaeUpdateMajor < 0x00000002 );
            }

            pidb->SetFLocalizedText();
        }

        pidb->SetColumnIndex( FidOfColumnid( columnid ) );
        Assert ( pidb->FColumnIndex( FidOfColumnid( columnid ) ) );
    }



    BOOL    fSparseConditionalIndex     = fFalse;

    Assert( !pidb->FPrimary()
        || 0 == pidb->CidxsegConditional() );

    pidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
    pidxsegMac = pidxseg + pidb->CidxsegConditional();
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        const COLUMNID          columnid    = pidxseg->Columnid();
        const FIELD * const     pfield      = ptdb->Pfield( columnid );
        Assert( pfield->coltyp != JET_coltypNil );
        Assert( !FFIELDDeleted( pfield->ffield ) );

        if ( FFIELDUserDefinedDefault( pfield->ffield ) )
        {
            if ( !fFoundUserDefinedDefault )
            {
                memset( pidb->RgbitIdx(), 0xff, 32 );
                fFoundUserDefinedDefault = fTrue;
            }

            if ( fSparseConditionalIndex )
                break;
        }
        else if ( !fSparseConditionalIndex )
        {
            const BOOL  fHasDefault = FFIELDDefault( pfield->ffield );

            fSparseConditionalIndex = ( pidxseg->FMustBeNull() ?
                                            fHasDefault :
                                            !fHasDefault );
        }

        pidb->SetColumnIndex( FidOfColumnid( columnid ) );
        Assert( pidb->FColumnIndex( FidOfColumnid( columnid ) ) );
    }

    if ( fSparseIndex )
    {
        Assert( !pidb->FPrimary() );
        pidb->SetFSparseIndex();
    }
    else
    {
        pidb->ResetFSparseIndex();
    }

    if ( fSparseConditionalIndex )
    {
        Assert( !pidb->FPrimary() );
        pidb->SetFSparseConditionalIndex();
    }
    else
    {
        pidb->ResetFSparseConditionalIndex();
    }

    if ( pidb->FPrimary() )
    {
        FILEIMinimizePrimaryCbKeyMost( pfcb, pidb );
    }

    IDB *pidbNew = new( PinstFromIfmp( pfcb->Ifmp() ) ) IDB( PinstFromIfmp( pfcb->Ifmp() ) );
    if ( pidbNil == pidbNew )
    {
        return ErrERRCheck( JET_errTooManyOpenIndexes );
    }

    *pidbNew = *pidb;

    pidbNew->InitRefcounts();

    pfcb->SetPidb( pidbNew );

    return JET_errSuccess;
}


ERR VTAPI ErrIsamSetSequential(
    const JET_SESID sesid,
    const JET_VTID  vtid,
    const JET_GRBIT grbit )
{
    ERR             err;
    PIB * const     ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB * const    pfucb       = reinterpret_cast<FUCB *>( vtid );

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    AssertDIRNoLatch( ppib );

    FUCB * const    pfucbIdx    = ( pfucbNil != pfucb->pfucbCurIndex ?
                                        pfucb->pfucbCurIndex :
                                        pfucb );
    FUCBSetSequential( pfucbIdx );

    if ( grbit & JET_bitPrereadForward )
    {
        FUCBSetPrereadForward( pfucbIdx, cpgPrereadSequential );
    }
    else if ( grbit & JET_bitPrereadBackward )
    {
        FUCBSetPrereadBackward( pfucbIdx, cpgPrereadSequential );
    }

    return JET_errSuccess;
}


ERR VTAPI ErrIsamResetSequential(
    const JET_SESID sesid,
    const JET_VTID vtid,
    const JET_GRBIT )
{
    ERR             err;
    PIB * const     ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB * const    pfucb       = reinterpret_cast<FUCB *>( vtid );

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    AssertDIRNoLatch( ppib );

    FUCB * const    pfucbIdx    = ( pfucbNil != pfucb->pfucbCurIndex ?
                                        pfucb->pfucbCurIndex :
                                        pfucb );
    FUCBResetSequential( pfucbIdx );
    FUCBResetPreread( pfucbIdx );

    return JET_errSuccess;
}


ERR ErrFILEIOpenTable(
    _In_ PIB            *ppib,
    _In_ IFMP       ifmp,
    _Out_ FUCB      **ppfucb,
    _In_ const CHAR *szName,
    _In_ ULONG      grbit,
    _In_opt_ FDPINFO        *pfdpinfo = NULL );

ERR VTAPI ErrIsamOpenTable(
    JET_SESID           vsesid,
    JET_DBID            vdbid,
    JET_TABLEID         *ptableid,
    __in const PCSTR    szPath,
    JET_GRBIT           grbit )
{
    ERR         err;
    PIB         *ppib   = (PIB *)vsesid;
    const IFMP  ifmp    = (IFMP)vdbid;
    FUCB        *pfucb  = pfucbNil;

    Assert( ptableid );
    *ptableid = JET_tableidNil;

    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, ifmp ) );
    AssertDIRNoLatch( ppib );

    if ( grbit & ( JET_bitTableDelete | JET_bitTableCreate ) )
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        goto HandleError;
    }
    else if ( grbit & JET_bitTablePermitDDL )
    {
        if ( !( grbit & JET_bitTableDenyRead ) )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
            goto HandleError;
        }
    }
    else if ( ( grbit & JET_bitTableDenyRead ) && ( grbit & JET_bitTableReadOnly ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    else if ( grbit & bitTableUpdatableDuringRecovery )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    Call( ErrFILEIOpenTable( ppib, ifmp, &pfucb, szPath, grbit ) );

#ifdef DEBUG
    if ( g_rgfmp[ifmp].FReadOnlyAttach() || ( grbit & JET_bitTableReadOnly ) || PinstFromPpib( ppib )->FRecovering() )
        Assert( !FFUCBUpdatable( pfucb ) );
    else
        Assert( FFUCBUpdatable( pfucb ) );
#endif

    pfucb->pvtfndef = &vtfndefIsam;
    *(FUCB **)ptableid = pfucb;

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( pfucb != pfucbNil )
        {
            CallS( ErrFILECloseTable( ppib, pfucb ) );
            pfucb = pfucbNil;
        }
    }

    AssertDIRNoLatch( ppib );
    return err;
}


#ifdef PERFMON_SUPPORT


PERFInstanceDelayedTotal<> cTableOpenCacheHits;
LONG LTableOpenCacheHitsCEFLPv( LONG iInstance, void *pvBuf )
{
    cTableOpenCacheHits.PassTo( iInstance, pvBuf );
    return 0;
}


PERFInstanceDelayedTotal<> cTableOpenCacheMisses;
LONG LTableOpenCacheMissesCEFLPv( LONG iInstance, void *pvBuf )
{
    cTableOpenCacheMisses.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LTableOpensCEFLPv( LONG iInstance, void *pvBuf )
{
    if ( NULL != pvBuf )
    {
        *(LONG*)pvBuf = cTableOpenCacheHits.Get( iInstance ) + cTableOpenCacheMisses.Get( iInstance );
    }
    return 0;
}

PERFInstanceDelayedTotal<> cTablesOpen;
LONG LTablesOpenCEFLPv( LONG iInstance, void *pvBuf )
{
    cTablesOpen.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cTablesCloses;
LONG LTableClosesCEFLPv( LONG iInstance, void *pvBuf )
{
    cTablesCloses.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cTableOpenPagesRead;
LONG LTableOpenPagesReadCEFLPv( LONG iInstance, void *pvBuf )
{
    cTableOpenPagesRead.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cTableOpenPagesPreread;
LONG LTableOpenPagesPrereadCEFLPv( LONG iInstance, void *pvBuf )
{
    cTableOpenPagesPreread.PassTo( iInstance, pvBuf );
    return 0;
}

#endif


LOCAL ERR ErrFILEISetMode( FUCB *pfucb, const JET_GRBIT grbit )
{
    ERR     wrn     = JET_errSuccess;
    PIB     *ppib   = pfucb->ppib;
    FCB     *pfcb   = pfucb->u.pfcb;
    FUCB    *pfucbT;

    Assert( pfcb->IsLocked() );

    Assert( !pfcb->FDeleteCommitted() );

    if ( pfcb->FDomainDenyRead( ppib ) )
    {
        Assert( pfcb->PpibDomainDenyRead() != ppibNil );
        if ( pfcb->PpibDomainDenyRead() != ppib )
        {
            return ErrERRCheck( JET_errTableLocked );
        }

#ifdef DEBUG
        pfcb->FucbList().LockForEnumeration();
        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++ )
        {
            pfucbT = pfcb->FucbList()[ifucbList];

            Assert( pfucbT->ppib == pfcb->PpibDomainDenyRead()
                || FPIBSessionSystemCleanup( pfucbT->ppib ) );
        }
        pfcb->FucbList().UnlockForEnumeration();
#endif
    }

    if ( grbit & JET_bitTableUpdatable )
    {
        if ( pfcb->FDomainDenyWrite() )
        {
            pfcb->FucbList().LockForEnumeration();
            for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++ )
            {
                pfucbT = pfcb->FucbList()[ ifucbList ];

                if ( pfucbT->ppib != ppib && FFUCBDenyWrite( pfucbT ) )
                {
                    pfcb->FucbList().UnlockForEnumeration();
                    return ErrERRCheck( JET_errTableLocked );
                }
            }
            pfcb->FucbList().UnlockForEnumeration();
        }
    }

    if ( grbit & JET_bitTableDenyWrite )
    {
        if ( pfcb->FDomainDenyWrite() )
        {
            return ErrERRCheck( JET_errTableInUse );
        }

        pfcb->FucbList().LockForEnumeration();
        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++ )
        {
            pfucbT = pfcb->FucbList()[ ifucbList ];

            if ( pfucbT->ppib != ppib
                && FFUCBUpdatable( pfucbT )
                && !FPIBSessionSystemCleanup( pfucbT->ppib ) )
            {
                pfcb->FucbList().UnlockForEnumeration();
                return ErrERRCheck( JET_errTableInUse );
            }
        }
        pfcb->FucbList().UnlockForEnumeration();
        pfcb->SetDomainDenyWrite();
        FUCBSetDenyWrite( pfucb );
    }

    if ( grbit & JET_bitTableDenyRead )
    {
        if ( pfcb->FDomainDenyRead( ppib ) )
        {
            return ErrERRCheck( JET_errTableInUse );
        }

        BOOL    fOpenSystemCursor = fFalse;
        pfcb->FucbList().LockForEnumeration();
        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++ )
        {
            pfucbT = pfcb->FucbList()[ ifucbList ];

            if ( pfucbT != pfucb )
            {
                if ( FPIBSessionSystemCleanup( pfucbT->ppib ) )
                {
                    fOpenSystemCursor = fTrue;
                }
                else if ( pfucbT->ppib != ppib
                    || ( ( grbit & JET_bitTableDelete ) && !FFUCBDeferClosed( pfucbT ) ) )
                {
                    pfcb->FucbList().UnlockForEnumeration();
                    return ErrERRCheck( JET_errTableInUse );
                }
            }
        }
        pfcb->FucbList().UnlockForEnumeration();

        if ( fOpenSystemCursor )
        {
            wrn = ErrERRCheck( JET_wrnTableInUseBySystem );
        }

        pfcb->SetDomainDenyRead( ppib );
        FUCBSetDenyRead( pfucb );

        if ( grbit & JET_bitTablePermitDDL )
        {
            if ( pfcb->FTemplateTable() )
            {
                if ( pfcb->FTemplateStatic() )
                {
                    return ErrERRCheck( JET_errTableInUse );
                }
            }
            else
            {
                ENTERCRITICALSECTION    enterCritFCBRCE( &pfcb->CritRCEList() );
                if ( pfcb->PrceNewest() != prceNil )
                {
                    return ErrERRCheck( JET_errTableInUse );
                }

                Assert( pfcb->PrceOldest() == prceNil );
            }

            FUCBSetPermitDDL( pfucb );
        }
    }

    return wrn;
}


LOCAL ERR ErrFILEICheckAndSetMode( FUCB *pfucb, const ULONG grbit )
{
    ERR err = JET_errSuccess;
    PIB *ppib = pfucb->ppib;
    FCB *pfcb = pfucb->u.pfcb;

    Assert( pfcb->IsLocked() );

    if ( ( grbit & JET_bitTableReadOnly ) ||
         ( PinstFromPpib( ppib )->FRecovering() && !( grbit & bitTableUpdatableDuringRecovery ) ) )
    {
        if ( grbit & JET_bitTableUpdatable )
        {
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        FUCBResetUpdatable( pfucb );
    }

    Assert( !( grbit & JET_bitTableDelete )
        || ( grbit & JET_bitTableDenyRead ) );

    Assert( !( grbit & JET_bitTablePermitDDL )
        || ( grbit & JET_bitTableDenyRead ) );

    if ( pfcb->FDeletePending() )
    {
        Error( ErrERRCheck( JET_errTableLocked ) );
    }

    if ( ( grbit & ( JET_bitTableDenyRead | JET_bitTableDenyWrite ) )
        || pfcb->FDomainDenyRead( ppib )
        || pfcb->FDomainDenyWrite() )
    {
        Call( ErrFILEISetMode( pfucb, grbit ) );
    }

    if( fFalse )
    {
        pfcb->SetNoCache();
    }
    else
    {
        pfcb->ResetNoCache();
    }

    if( grbit & JET_bitTablePreread )
    {
        pfcb->SetPreread();
    }
    else
    {
        pfcb->ResetPreread();
    }

HandleError:
    return err;
}

LOCAL ERR ErrFILEICheckLocalizedIndexesInTable(
    _In_ PIB        * const ppib,
    _In_ const IFMP ifmp,
    _In_ FCB        * const pfcbTable,
    _In_z_ const CHAR   *szTableName,
    _In_ JET_GRBIT  grbit )
{
    ERR err = JET_errSuccess;
    const BOOL fReadOnly = ( grbit & JET_bitTableReadOnly ) || PinstFromPpib( ppib )->FRecovering();
    const BOOL fDeletingTable = !!( grbit & JET_bitTableDelete );

    if ( UlParam( PinstFromIfmp( ifmp ), JET_paramEnableIndexChecking ) == JET_IndexCheckingDeferToOpenTable )
    {
        BOOL fIndicesUpdated;
        BOOL fIndicesDeleted;
        CATCheckIndicesFlags catcifFlags = catcifReadOnly;

        if ( fDeletingTable )
        {
            err = JET_errSuccess;
            goto HandleError;
        }

        if ( fReadOnly )
        {
            catcifFlags = catcifFlags | catcifReportOnlyOutOfDateSecondaryIndices;
        }

        if ( grbit & JET_bitTableAllowOutOfDate )
        {
            catcifFlags = catcifFlags | catcifAllowValidOutOfDateVersions;
        }


        err = ErrCATCheckLocalizedIndexesInTable( ppib, ifmp, pfcbTable, pfucbNil, szTableName, catcifFlags, &fIndicesUpdated, &fIndicesDeleted );

        AssertSz( !fIndicesDeleted, "No index should be deleted" );
        AssertSz( fReadOnly || !fIndicesUpdated, "No index should be updated in read only mode" );

        if ( fReadOnly &&
             UlParam( PinstFromIfmp( ifmp ), JET_paramEnableIndexChecking ) == JET_IndexCheckingDeferToOpenTable )
        {
            if ( err == JET_errPrimaryIndexCorrupted )
            {
                Assert( pfcbTable != pfcbNil );
                err = ErrERRCheck( JET_wrnPrimaryIndexOutOfDate );
            }
            else if ( err == JET_errSecondaryIndexCorrupted )
            {
                Assert( pfcbTable != pfcbNil );
                err = ErrERRCheck( JET_wrnSecondaryIndexOutOfDate );
            }
        }

        Call( err );
    }

HandleError:
    return err;
}

ERR ErrFILEOpenTable(
    _In_ PIB            *ppib,
    _In_ IFMP       ifmp,
    _Out_ FUCB      **ppfucb,
    _In_ const CHAR *szName,
    _In_ ULONG      grbit,
    _In_opt_ FDPINFO        *pfdpinfo )
{
    return ErrFILEIOpenTable( ppib, ifmp, ppfucb, szName, grbit | JET_bitTableAllowOutOfDate, pfdpinfo );
}

ERR ErrFILEIOpenTable(
    _In_ PIB            *ppib,
    _In_ IFMP       ifmp,
    _Out_ FUCB      **ppfucb,
    _In_ const CHAR *szName,
    _In_ ULONG      grbit,
    _In_opt_ FDPINFO        *pfdpinfo )
{
    ERR         err;
    ERR         wrnSurvives             = JET_errSuccess;
    FUCB        *pfucb                  = pfucbNil;
    FCB         *pfcb;
    CHAR        szTable[JET_cbNameMost+1];
    BOOL        fOpeningSys;
    PGNO        pgnoFDP                 = pgnoNull;
    OBJID       objidTable              = objidNil;
    BOOL        fInTransaction          = fFalse;
    BOOL        fInitialisedCursor      = fFalse;
    TABLECLASS  tableclass              = tableclassNone;

    Assert( ppib != ppibNil );
    Assert( ppfucb != NULL );
    FMP::AssertVALIDIFMP( ifmp );

    *ppfucb = pfucbNil;

#ifdef DEBUG
    CheckPIB( ppib );
    if( !Ptls()->FIsTaskThread()
        && !Ptls()->fIsRCECleanup )
    {
        CheckDBID( ppib, ifmp );
    }
#endif
    CallR( ErrUTILCheckName( szTable, szName, JET_cbNameMost+1 ) );

    OSTraceFMP(
        ifmp,
        JET_tracetagCursors,
        OSFormat(
            "Session=[0x%p:0x%x] opening table '%s' of ifmp=0x%x [grbit=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szTable,
            (ULONG)ifmp,
            grbit ) );

    fOpeningSys = FCATSystemTable( szTable );

    if ( NULL == pfdpinfo )
    {
        if ( FFMPIsTempDB( ifmp ) )
        {
            AssertSz( fFalse, "Illegal dbid" );
            return ErrERRCheck( JET_errInvalidDatabaseId );
        }

        if ( fOpeningSys )
        {
            if ( grbit & JET_bitTableDelete )
            {
                return ErrERRCheck( JET_errCannotDeleteSystemTable );
            }
            pgnoFDP     = PgnoCATTableFDP( szTable );
            objidTable  = ObjidCATTable( szTable );
        }
        else
        {
            if ( 0 == ppib->Level() )
            {
                CallR( ErrDIRBeginTransaction( ppib, 35109, JET_bitTransactionReadOnly ) );
                fInTransaction = fTrue;
            }

            if ( !FCATHashLookup( ifmp, szTable, &pgnoFDP, &objidTable ) )
            {
                Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDP, &objidTable ) );
            }
        }
    }
    else
    {
        Assert( pgnoNull != pfdpinfo->pgnoFDP );
        Assert( objidNil != pfdpinfo->objidFDP );
        pgnoFDP = pfdpinfo->pgnoFDP;
        objidTable = pfdpinfo->objidFDP;

#ifdef DEBUG
        if( fOpeningSys )
        {
            Assert( PgnoCATTableFDP( szTable ) == pgnoFDP );
            Assert( ObjidCATTable( szTable ) == objidTable );
        }
        else if ( !FFMPIsTempDB( ifmp ) )
        {
            PGNO    pgnoT;
            OBJID   objidT;

            if ( !FCATHashLookup( ifmp, szTable, &pgnoT, &objidT ) )
            {
                CallR( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoT, &objidT ) );
            }

            Assert( pgnoT == pgnoFDP );
            Assert( objidT == objidTable );
        }
#endif
    }

    Assert( pgnoFDP > pgnoSystemRoot );
    Assert( pgnoFDP <= pgnoSysMax );
    Assert( objidNil != objidTable );
    Assert( objidTable > objidSystemRoot );

    Call( ErrDIROpenNoTouch( ppib, ifmp, pgnoFDP, objidTable, fTrue, &pfucb, fTrue ) );
    Assert( pfucbNil != pfucb );

    pfcb = pfucb->u.pfcb;
    Assert( objidTable == pfcb->ObjidFDP() );
    Assert( pgnoFDP == pfcb->PgnoFDP() );

    FUCBSetIndex( pfucb );

    if ( fOpeningSys )
    {
        tableclass = TableClassFromSysTable( FCATShadowSystemTable( szTable ) );
    }
    else
    {
        const ULONG tableClassOffset = ( grbit & JET_bitTableClassMask ) / JET_bitTableClass1;
        if ( tableClassOffset != tableclassNone )
        {
            const ULONG paramid = ( tableClassOffset >= 16 ) ?
                ( ( JET_paramTableClass16Name - 1 ) + ( tableClassOffset - 15 ) ) :
                ( ( JET_paramTableClass1Name - 1 ) + tableClassOffset );
            if ( !FDefaultParam( paramid ) )
            {
                tableclass = TableClassFromTableClassOffset( tableClassOffset );
            }
        }
    }
    
    if( grbit & JET_bitTablePreread
        || grbit & JET_bitTableSequential )
    {
        PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
        tcScope->nParentObjectClass = TCEFromTableClass(
                                        tableclass,
                                        FFUCBLongValue( pfucb ),
                                        FFUCBSecondary( pfucb ),
                                        FFUCBSpace( pfucb ) );
        tcScope->SetDwEngineObjid( objidTable );

        (void)ErrBFPrereadPage( ifmp, pgnoFDP, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
    }

    Assert( !( grbit & JET_bitTableCreate ) || !pfcb->FInitialized() );

    if ( !pfcb->FInitialized() || pfcb->FInitedForRecovery() )
    {
        if ( fInTransaction )
        {
            Call( ErrDIRCommitTransaction( ppib,NO_GRBIT ) );
            fInTransaction = fFalse;
        }

        if ( fOpeningSys )
        {
            Call( ErrCATInitCatalogFCB( pfucb ) );
        }
        else if ( FFMPIsTempDB( ifmp ) )
        {
            Assert( !( grbit & JET_bitTableDelete ) );
            Call( ErrCATInitTempFCB( pfucb ) );
        }
        else
        {
            const ULONG cPageReadBefore = Ptls()->threadstats.cPageRead;
            const ULONG cPagePrereadBefore = Ptls()->threadstats.cPagePreread;

            Call( ErrCATInitFCB( pfucb, objidTable ) );

            const ULONG cPageReadAfter = Ptls()->threadstats.cPageRead;
            const ULONG cPagePrereadAfter = Ptls()->threadstats.cPagePreread;

            Assert( cPageReadAfter >= cPageReadBefore );
            PERFOpt( cTableOpenPagesRead.Add( PinstFromPpib( ppib )->m_iInstance, pfcb->TCE(), cPageReadAfter - cPageReadBefore ) );

            Assert( cPagePrereadAfter >= cPagePrereadBefore );
            PERFOpt( cTableOpenPagesPreread.Add( PinstFromPpib( ppib )->m_iInstance, pfcb->TCE(), cPagePrereadAfter - cPagePrereadBefore ) );

            Assert( pfcb->FTypeTable() );
            wrnSurvives = ErrFILEICheckLocalizedIndexesInTable( ppib, ifmp, pfcb, szTable, grbit );
            Call( wrnSurvives );

            if ( !( grbit & ( JET_bitTableCreate|JET_bitTableDelete) ) )
            {
                CATHashInsert( pfcb, pfcb->Ptdb()->SzTableName() );
            }

            PERFOpt( cTableOpenCacheMisses.Inc( PinstFromPpib( ppib ) ) );
            PERFOpt( cTablesOpen.Inc( PinstFromPpib( ppib ) ) );
        }

        Assert( pfcb->Ptdb() != ptdbNil );

        Call( ErrFaultInjection( 62880 ) );

        if ( !pfcb->FInitedForRecovery() )
        {
            pfcb->InsertList();
        }

        fInitialisedCursor = fTrue;

        pfcb->Lock();
        Assert( !pfcb->FTypeNull() );
        Assert( !pfcb->FInitialized() || pfcb->FInitedForRecovery() );
        Assert( pfcb == pfucb->u.pfcb );

        pfcb->CreateComplete();
        pfcb->ResetInitedForRecovery();

        err = ErrFILEICheckAndSetMode( pfucb, grbit ) + ErrFaultInjection( 38304 );

        if ( err >= JET_errSuccess )
        {
            if ( grbit & JET_bitTableTryPurgeOnClose )
            {
                pfcb->SetTryPurgeOnClose();
            }
        }
        else
        {
            pfcb->SetTryPurgeOnClose();
        }

        pfcb->Unlock();

        Call( err );
    }
    else
    {
        if ( !fOpeningSys
            && !FFMPIsTempDB( ifmp )
            && !( grbit & JET_bitTableDelete ) )
        {
            PERFOpt( cTableOpenCacheHits.Inc( PinstFromPpib( ppib ) ) );
            PERFOpt( cTablesOpen.Inc( PinstFromPpib( ppib ) ) );
        }

        fInitialisedCursor = fTrue;

        Assert( JET_errSuccess == wrnSurvives );

        if ( !fOpeningSys
             && !FFMPIsTempDB( ifmp ) )
        {
            Assert( pfcb->FTypeTable() );
            wrnSurvives = ErrFILEICheckLocalizedIndexesInTable( ppib, ifmp, pfcb, szTable, grbit );
        }

        Call( wrnSurvives );

        Assert( pfcb == pfucb->u.pfcb );
        pfcb->Lock();

        err = ErrFILEICheckAndSetMode( pfucb, grbit );

        if ( ( err >= JET_errSuccess ) && !( grbit & JET_bitTableTryPurgeOnClose ) )
        {
            pfcb->ResetTryPurgeOnClose();
        }

        pfcb->Unlock();

        Call( err );

        if ( fInTransaction )
        {
            Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
            fInTransaction = fFalse;
        }
    }


    Assert( !pfcb->FDeletePending() || FPIBSessionSystemCleanup( ppib ) );
    Assert( !pfcb->FDeleteCommitted() || FPIBSessionSystemCleanup( ppib ) );

    if ( grbit & JET_bitTableSequential )
        FUCBSetSequential( pfucb );
    else
        FUCBResetSequential( pfucb );


    if ( grbit & JET_bitTableOpportuneRead )
        FUCBSetOpportuneRead( pfucb );
    else
        FUCBResetOpportuneRead( pfucb );
        
    if ( pfcb->Ptdb() != ptdbNil )
    {
        pfcb->SetTableclass( tableclass );
    }
    else
    {
        FireWall( "DeprecatedSentinelFcbOpenTableSetTc" );
        Assert( pfcbNil == pfcb->PfcbNextIndex() );
    }

    pfucb->pvWorkBuf = NULL;
    pfucb->dataWorkBuf.SetPv( NULL );
    Assert( !FFUCBUpdatePrepared( pfucb ) );

    pfucb->dataSearchKey.Nullify();
    pfucb->cColumnsInSearchKey = 0;
    KSReset( pfucb );

    FUCBSetMayCacheLVCursor( pfucb );

    RECDeferMoveFirst( ppib, pfucb );

#ifdef DEBUG
    if ( pfcb->Ptdb() != ptdbNil )
    {
        pfcb->EnterDDL();
        for ( FCB *pfcbT = pfcb->PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( pfcbT->PfcbTable() == pfcb );
        }
        pfcb->LeaveDDL();
    }
    else
    {
        FireWall( "DeprecatedSentinelFcbOpenTableMoveCur" );
        Assert( pfcbNil == pfcb->PfcbNextIndex() );
    }
#endif

    Assert( !fInTransaction );
    AssertDIRNoLatch( ppib );
    *ppfucb = pfucb;
    
    CallSx( err, JET_wrnTableInUseBySystem );

    if ( wrnSurvives > JET_errSuccess )
    {
        Assert( err == JET_errSuccess );
    }

    if ( JET_errSuccess == err && wrnSurvives > JET_errSuccess )
    {
        err = wrnSurvives;
    }

    Assert( !pfcb->FDerivedTable() || pfcb->Ptdb()->PfcbTemplateTable()->FTemplateStatic() );

    return err;

HandleError:
    Assert( pfucbNil != pfucb || !fInitialisedCursor );
    if ( pfucbNil != pfucb )
    {
        if ( fInitialisedCursor )
        {
            CallS( ErrFILECloseTable( ppib, pfucb ) );
        }
        else
        {
            DIRClose( pfucb );
        }
    }

    if ( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }
    AssertDIRNoLatch( ppib );

    return err;
}


ERR ErrIsamPrereadTables( __in JET_SESID sesid, __in JET_DBID vdbid, __in_ecount( cwszTables ) PCWSTR * rgwszTables, __in INT cwszTables, JET_GRBIT grbit )
{
    ERR err = JET_errSuccess;
    PIB * const ppib = (PIB*)sesid;
    const IFMP ifmp = (IFMP)vdbid;

    PGNO    rgpgno[11];

    if ( grbit )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    if ( cwszTables > ( _countof(rgpgno) - 1  ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    for( INT iTable = 0; iTable < cwszTables; iTable++ )
    {
        CAutoSZDDL  lszTableName;
        OBJID objidTableT;

        rgpgno[iTable] = pgnoNull;

        Call( lszTableName.ErrSet( rgwszTables[iTable] ) );
        
        if ( !FCATHashLookup( ifmp, lszTableName, &(rgpgno[iTable]), &objidTableT ) )
        {
            (void)ErrCATSeekTable( ppib, ifmp, lszTableName, &(rgpgno[iTable]), &objidTableT );
        }
    }


    rgpgno[cwszTables] = pgnoNull;

    sort( rgpgno, rgpgno + cwszTables );

    INT iFirstTable = cwszTables;
    for( INT iTable = 0; iTable < cwszTables + 1; iTable++ )
    {
        if ( iFirstTable == cwszTables && rgpgno[iTable] != pgnoNull )
        {
            iFirstTable = iTable;
            break;
        }
    }


    if ( iFirstTable < cwszTables )
    {
        PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
        tcScope->nParentObjectClass = tceNone;

        BFPrereadPageList( ifmp, &( rgpgno[iFirstTable] ), bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
    }

HandleError:

    return err; 
}


ERR VTAPI ErrIsamCloseTable( JET_SESID sesid, JET_VTID vtid )
{
    PIB *ppib   = reinterpret_cast<PIB *>( sesid );
    FUCB *pfucb = reinterpret_cast<FUCB *>( vtid );

    ERR     err;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );

    Assert( pfucb->pvtfndef == &vtfndefIsam || pfucb->pvtfndef == &vtfndefIsamMustRollback );

    pfucb->pvtfndef = &vtfndefInvalidTableid;
    err = ErrFILECloseTable( ppib, pfucb );
    return err;
}


ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
{
    CheckPIB( ppib );
    CheckTable( ppib, pfucb );
    Assert( pfucb->pvtfndef == &vtfndefInvalidTableid );

    FCB     *pfcb = pfucb->u.pfcb;

    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        CallS( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }
    Assert( !FFUCBUpdatePrepared( pfucb ) );

    if ( ! FCATSystemTable( pfcb->PgnoFDP() )
        && !FFMPIsTempDB( pfcb->Ifmp() ) )
    {
        PERFOpt( cTablesCloses.Inc( PinstFromPpib( ppib ) ) );
        PERFOpt( cTablesOpen.Dec( PinstFromPpib( ppib ) ) );
    }

    DIRResetIndexRange( pfucb );

    RECIFreeCopyBuffer( pfucb );

    RECReleaseKeySearchBuffer( pfucb );

    DIRCloseIfExists( &pfucb->pfucbCurIndex );
    DIRCloseIfExists( &pfucb->pfucbLV );

    if ( pfucb->u.pfcb->FTypeTemporaryTable() )
    {
        INT     wRefCnt;
        FUCB    *pfucbT;

        Assert( FFMPIsTempDB( pfcb->Ifmp() ) );
        Assert( pfcb->Ptdb() != ptdbNil );
        Assert( pfcb->FFixedDDL() );
        DIRClose( pfucb );

        pfucbT = ppib->pfucbOfSession;
        wRefCnt = pfcb->WRefCount();
        while ( wRefCnt > 0 && pfucbT != pfucbNil )
        {
            Assert( pfucbT->ppib == ppib );
            if ( pfucbT->u.pfcb == pfcb )
            {
                if ( !FFUCBDeferClosed( pfucbT ) )
                {
                    break;
                }
                Assert( wRefCnt > 0 );
                wRefCnt--;
            }

            pfucbT = pfucbT->pfucbNextOfSession;
        }
        Assert( wRefCnt >= 0 );
        if ( wRefCnt == 0 )
        {
            Assert( ppibNil != ppib );

            Assert( pfcb->PfcbNextIndex() == pfcbNil );


            Assert( pfcb->Ptdb() != ptdbNil );
            FCB * const pfcbLV = pfcb->Ptdb()->PfcbLV();
            if ( pfcbNil != pfcbLV )
            {
                Assert( pfcbLV->PrceNewest() == prceNil || pfcbLV->PrceNewest()->TrxCommitted() == trxMax );
                Assert( pfcbLV->PrceOldest() == prceNil || pfcbLV->PrceOldest()->TrxCommitted() == trxMax );
                VERNullifyAllVersionsOnFCB( pfcbLV );
                FUCBCloseAllCursorsOnFCB( ppib, pfcbLV );

                Assert( !pfcbLV->FDeleteCommitted() );
                pfcbLV->Lock();
                pfcbLV->SetDeleteCommitted();
                pfcbLV->Unlock();

                pfcbLV->PrepareForPurge( fFalse );
            }

            Assert( pfcb->PrceNewest() == prceNil || pfcb->PrceNewest()->TrxCommitted() == trxMax  );
            Assert( pfcb->PrceOldest() == prceNil || pfcb->PrceOldest()->TrxCommitted() == trxMax  );
            VERNullifyAllVersionsOnFCB( pfcb );
            FUCBCloseAllCursorsOnFCB( ppib, pfcb );

            Assert( !pfcb->FDeleteCommitted() );
            pfcb->Lock();
            pfcb->SetDeleteCommitted();
            pfcb->Unlock();


            pfcb->PrepareForPurge( fFalse );


            (VOID)ErrSPFreeFDP( ppib, pfcb, pgnoSystemRoot );

            pfcb->Purge();
        }
        return JET_errSuccess;
    }

    DIRClose( pfucb );
    return JET_errSuccess;
}


VOID FILETableMustRollback( PIB *ppib, FCB *pfcbTable )
{
    CheckPIB( ppib );

    FUCB *pfucbT = ppib->pfucbOfSession;
    Assert( pfucbT != pfucbNil );
    while ( pfucbT != pfucbNil )
    {
        Assert( pfucbT->ppib == ppib );
        if ( pfucbT->u.pfcb == pfcbTable )
        {
            Assert( pfucbT->pvtfndef == &vtfndefIsam
                || pfucbT->pvtfndef == &vtfndefTTBase
                || pfucbT->pvtfndef == &vtfndefInvalidTableid );
            if( pfucbT->pvtfndef == &vtfndefIsam )
            {
                pfucbT->pvtfndef = &vtfndefIsamMustRollback;
            }
            else if ( pfucbT->pvtfndef == &vtfndefTTBase )
            {
                pfucbT->pvtfndef = &vtfndefTTBaseMustRollback;
            }
        }

        pfucbT = pfucbT->pfucbNextOfSession;
    }
}


ERR ErrFILEIInitializeFCB(
    PIB         *ppib,
    IFMP        ifmp,
    TDB         *ptdb,
    FCB         *pfcbNew,
    IDB         *pidb,
    BOOL        fPrimary,
    PGNO        pgnoFDP,
    __in const JET_SPACEHINTS * const pjsph,
    __out FCB * pfcbTemplateIndex )
{
    ERR     err = JET_errSuccess;

    Assert( pgnoFDP > pgnoSystemRoot );
    Assert( pgnoFDP <= pgnoSysMax );
    Assert( pfcbNew != pfcbNil );
    Assert( pfcbNew->Ifmp() == ifmp );
    Assert( pfcbNew->PgnoFDP() == pgnoFDP );
    Assert( pfcbNew->Ptdb() == ptdbNil );
    Assert( pjsph );

    if ( fPrimary )
    {
        pfcbNew->SetPtdb( ptdb );
        pfcbNew->Lock();
        pfcbNew->SetPrimaryIndex();
        Assert( !pfcbNew->FSequentialIndex() );
        if ( pidbNil == pidb )
        {
            pfcbNew->SetSequentialIndex();
        }
        pfcbNew->Unlock();
    }
    else
    {
        if ( pfcbNew->FInitedForRecovery() )
        {
            pfcbNew->RemoveList();
        }

        pfcbNew->Lock();
        pfcbNew->SetTypeSecondaryIndex();
        pfcbNew->Unlock();
    }

    
    if  ( pidb != pidbNil && pidb->FDerivedIndex() )
    {

        Assert( pfcbTemplateIndex );
        Assert( pfcbTemplateIndex->FTemplateIndex() );
        Assert( pfcbTemplateIndex->Pidb() != pidbNil );
        Assert( pfcbTemplateIndex->Pidb()->FTemplateIndex() );

        pfcbNew->SetSpaceHints( pjsph );

        if ( !pfcbTemplateIndex->Pidb()->FLocalizedText() )
        {
            pfcbNew->SetPidb( pfcbTemplateIndex->Pidb() );
        }
        else
        {
            Call( ErrFILEIGenerateIDB( pfcbNew, ptdb, pidb ) );

            pfcbNew->Pidb()->SetFIDBOwnedByFCB();
        }
        Assert( !pfcbNew->FTemplateIndex() );
        pfcbNew->Lock();
        pfcbNew->SetDerivedIndex();
        pfcbNew->Unlock();
    }
    else
    {
        Assert( pfcbTemplateIndex == NULL );

        pfcbNew->SetSpaceHints( pjsph );

        Assert( pidb != pidbNil || fPrimary );
        if ( pidb != pidbNil )
        {
            if ( pidb->FTemplateIndex() )
            {
                pfcbNew->Lock();
                pfcbNew->SetTemplateIndex();
                pfcbNew->Unlock();
            }
            Call( ErrFILEIGenerateIDB( pfcbNew, ptdb, pidb ) );

            Assert( pfcbNew->Ifmp() != 0 );
            Assert( !fPrimary || ( ptdb->IbEndFixedColumns() <= REC::CbRecordMost( g_rgfmp[ pfcbNew->Ifmp() ].CbPage(), pidb ) ) );
        }
    }

    Assert( err >= 0 );
    return err;

HandleError:

    Assert( err < 0 );
    Assert( pfcbNew->Pidb() == pidbNil );
    return err;
}


INLINE VOID RECIForceTaggedColumnsAsDerived(
    const TDB           * const ptdb,
    DATA&               dataDefault )
{
    TAGFIELDS           tagfields( dataDefault );
    tagfields.ForceAllColumnsAsDerived();
    tagfields.AssertValid( ptdb );
}

VOID FILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, TDB *ptdb )
{
    Assert( ptdbNil != ptdb );
    pfcbFake->SetPtdb( ptdb );
    pfucbFake->u.pfcb = pfcbFake;
    FUCBSetIndex( pfucbFake );

    pfcbFake->ResetFlags();
    pfcbFake->SetTypeTable();

    pfcbFake->Lock();
    pfcbFake->SetFixedDDL();
    pfcbFake->Unlock();

    pfucbFake->pvWorkBuf = NULL;
    RECIAllocCopyBuffer( pfucbFake );

    if ( pfcbNil != ptdb->PfcbTemplateTable() )
    {
        ptdb->AssertValidDerivedTable();
        pfcbFake->Lock();
        pfcbFake->SetDerivedTable();
        pfcbFake->Unlock();

        const TDB   * const ptdbTemplate = ptdb->PfcbTemplateTable()->Ptdb();

        Assert( ptdbTemplate != ptdbNil );
        Assert( NULL != ptdbTemplate->PdataDefaultRecord() );
        ptdbTemplate->PdataDefaultRecord()->CopyInto( pfucbFake->dataWorkBuf );

        RECIForceTaggedColumnsAsDerived( ptdb, pfucbFake->dataWorkBuf );
    }
    else
    {
        if ( ptdb->FTemplateTable() )
        {
            ptdb->AssertValidTemplateTable();
            pfcbFake->Lock();
            pfcbFake->SetTemplateTable();
            pfcbFake->Unlock();
        }

        REC::SetMinimumRecord( pfucbFake->dataWorkBuf );
    }
}


LOCAL ERR ErrFILERebuildOneDefaultValue(
    FUCB            * pfucbFake,
    const COLUMNID  columnid,
    const COLUMNID  columnidToAdd,
    const DATA      * const pdataDefaultToAdd )
{
    ERR             err;
    DATA            dataDefaultValue;

    if ( columnid == columnidToAdd )
    {
        Assert( pdataDefaultToAdd );
        dataDefaultValue = *pdataDefaultToAdd;
    }
    else
    {
        CallR( ErrRECIRetrieveDefaultValue(
                        pfucbFake->u.pfcb,
                        columnid,
                        &dataDefaultValue ) );

        Assert( JET_wrnColumnNull != err );
        Assert( wrnRECUserDefinedDefault != err );
        Assert( wrnRECSeparatedLV != err );
        Assert( wrnRECLongField != err );
        Assert( wrnRECCompressed != err );
    }

    Assert( dataDefaultValue.Pv() != NULL );
    Assert( dataDefaultValue.Cb() > 0 );
    CallR( ErrRECSetDefaultValue(
                pfucbFake,
                columnid,
                dataDefaultValue.Pv(),
                dataDefaultValue.Cb() ) );

    return err;
}

ERR ErrFILERebuildDefaultRec(
    FUCB            * pfucbFake,
    const COLUMNID  columnidToAdd,
    const DATA      * const pdataDefaultToAdd )
{
    ERR             err     = JET_errSuccess;
    TDB             * ptdb  = pfucbFake->u.pfcb->Ptdb();
    FID             fid;

    Assert( ptdbNil != ptdb );
    Assert( 0 != pfucbFake->ifmp );

    for ( fid = ptdb->FidFixedFirst(); ;fid++ )
    {
        if ( ptdb->FidFixedLast() + 1 == fid )
            fid = ptdb->FidVarFirst();
        if ( ptdb->FidVarLast() + 1 == fid )
            fid = ptdb->FidTaggedFirst();
        if ( fid > ptdb->FidTaggedLast() )
            break;

        Assert( ( fid >= ptdb->FidFixedFirst() && fid <= ptdb->FidFixedLast() )
            || ( fid >= ptdb->FidVarFirst() && fid <= ptdb->FidVarLast() )
            || ( fid >= ptdb->FidTaggedFirst() && fid <= ptdb->FidTaggedLast() ) );

        const COLUMNID  columnid        = ColumnidOfFid( fid, ptdb->FTemplateTable() );
        const FIELD     * const pfield  = ptdb->Pfield( columnid );
        if ( FFIELDDefault( pfield->ffield )
            && !FFIELDUserDefinedDefault( pfield->ffield )
            && !FFIELDCommittedDelete( pfield->ffield ) )
        {
            Assert( JET_coltypNil != pfield->coltyp );
            Call( ErrFILERebuildOneDefaultValue(
                        pfucbFake,
                        columnid,
                        columnidToAdd,
                        pdataDefaultToAdd ) );
        }
    }

    RECDANGLING *   precdangling;

    Assert( pfucbFake->dataWorkBuf.Cb() >= REC::cbRecordMin );
    Assert( pfucbFake->dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucbFake->ifmp ].CbPage() ) );
    if ( pfucbFake->dataWorkBuf.Cb() > REC::CbRecordMost( pfucbFake ) )
    {
        FireWall( "FILERebuildDefaultRecTooBig16.2" );
    }
    if ( pfucbFake->dataWorkBuf.Cb() < REC::cbRecordMin || pfucbFake->dataWorkBuf.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucbFake->ifmp ].CbPage() ) )
    {
        FireWall( "FILERebuildDefaultRecTooBig16.1" );
        err = ErrERRCheck( JET_errDatabaseCorrupted );
        goto HandleError;
    }

    precdangling = (RECDANGLING *)PvOSMemoryHeapAlloc( sizeof(RECDANGLING) + pfucbFake->dataWorkBuf.Cb() );
    if ( NULL == precdangling )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        goto HandleError;
    }

    precdangling->precdanglingNext = NULL;
    precdangling->data.SetPv( (BYTE *)precdangling + sizeof(RECDANGLING) );
    pfucbFake->dataWorkBuf.CopyInto( precdangling->data );
    ptdb->SetPdataDefaultRecord( &( precdangling->data ) );

HandleError:
    return err;
}


VOID FILESetAllIndexMask( FCB *pfcbTable )
{
    Assert( ptdbNil != pfcbTable->Ptdb() );

    FCB *                   pfcbT;
    TDB *                   ptdb        = pfcbTable->Ptdb();
    const LONG_PTR * const  plMax       = (LONG_PTR *)ptdb->RgbitAllIndex()
                                            + ( cbRgbitIndex / sizeof(LONG_PTR) );

    if ( pfcbTable->Pidb() != pidbNil )
    {
        ptdb->SetRgbitAllIndex( pfcbTable->Pidb()->RgbitIdx() );
    }
    else
    {
        ptdb->ResetRgbitAllIndex();
    }

    for ( pfcbT = pfcbTable->PfcbNextIndex();
        pfcbT != pfcbNil;
        pfcbT = pfcbT->PfcbNextIndex() )
    {
        Assert( pfcbT->Pidb() != pidbNil );

        if ( !pfcbT->Pidb()->FDeleted() || pfcbT->Pidb()->FVersioned() )
        {
            LONG_PTR *          plAll   = (LONG_PTR *)ptdb->RgbitAllIndex();
            const LONG_PTR *    plIndex = (LONG_PTR *)pfcbT->Pidb()->RgbitIdx();
            for ( ; plAll < plMax; plAll++, plIndex++ )
            {
                *plAll |= *plIndex;
            }
        }
    }

    return;
}

ERR ErrIDBSetIdxSeg(
    IDB             * const pidb,
    TDB             * const ptdb,
    const IDXSEG    * const rgidxseg )
{
    ERR             err;

    if ( pidb->FIsRgidxsegInMempool() )
    {
        USHORT  itag;

        CallR( ptdb->MemPool().ErrAddEntry(
                (BYTE *)rgidxseg,
                pidb->Cidxseg() * sizeof(IDXSEG),
                &itag ) );

        pidb->SetItagRgidxseg( itag );
    }
    else
    {
        const ULONG cIdxseg = pidb->Cidxseg();
        
        AssertPREFIX( cIdxseg <= cIDBIdxSegMax );
        UtilMemCpy( pidb->rgidxseg, rgidxseg, cIdxseg * sizeof(IDXSEG) );
        err = JET_errSuccess;
    }

    return err;
}
ERR ErrIDBSetIdxSegConditional(
    IDB             * const pidb,
    TDB             * const ptdb,
    const IDXSEG    * const rgidxseg )
{
    ERR     err;

    if ( pidb->FIsRgidxsegConditionalInMempool() )
    {
        USHORT  itag;

        CallR( ptdb->MemPool().ErrAddEntry(
                (BYTE *)rgidxseg,
                pidb->CidxsegConditional() * sizeof(IDXSEG),
                &itag ) );
        pidb->SetItagRgidxsegConditional( itag );
    }
    else
    {
        const ULONG cIdxsegConditional = pidb->CidxsegConditional();
        
        AssertPREFIX( cIdxsegConditional <= cIDBIdxSegConditionalMax );
        UtilMemCpy( pidb->rgidxsegConditional, rgidxseg, cIdxsegConditional * sizeof(IDXSEG) );
        err = JET_errSuccess;
    }

    return err;
}

ERR ErrIDBSetIdxSeg(
    IDB         * const pidb,
    TDB         * const ptdb,
    const BOOL  fConditional,
    const       LE_IDXSEG* const le_rgidxseg )
{
    IDXSEG      rgidxseg[JET_ccolKeyMost];
    const ULONG cidxseg     = ( fConditional ? pidb->CidxsegConditional() : pidb->Cidxseg() );

    if ( 0 == cidxseg )
    {
        Assert( fConditional );
        return JET_errSuccess;
    }


    for ( UINT iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
    {
        rgidxseg[ iidxseg ] = (LE_IDXSEG &) le_rgidxseg[iidxseg];
        Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
    }

    return ( fConditional ?
                ErrIDBSetIdxSegConditional( pidb, ptdb, rgidxseg ) :
                ErrIDBSetIdxSeg( pidb, ptdb, rgidxseg ) );
}

VOID SetIdxSegFromOldFormat(
    const UnalignedLittleEndian< IDXSEG_OLD >   * const le_rgidxseg,
    IDXSEG          * const rgidxseg,
    const ULONG     cidxseg,
    const BOOL      fConditional,
    const BOOL      fTemplateTable,
    const TCIB      * const ptcibTemplateTable )
{
    FID             fid;

    for ( UINT iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
    {
        rgidxseg[iidxseg].ResetFlags();

        if ( le_rgidxseg[iidxseg] < 0 )
        {
            if ( fConditional )
            {
                rgidxseg[iidxseg].SetFMustBeNull();
            }
            else
            {
                rgidxseg[iidxseg].SetFDescending();
            }
            fid = FID( -le_rgidxseg[iidxseg] );
        }
        else
        {
            fid = FID( le_rgidxseg[iidxseg] );
        }

        if ( NULL != ptcibTemplateTable )
        {
            Assert( !fTemplateTable );

            if ( FTaggedFid( fid ) )
            {
                if ( fidTaggedLeast-1 == ptcibTemplateTable->fidTaggedLast
                    || fid <= ptcibTemplateTable->fidTaggedLast )
                    rgidxseg[iidxseg].SetFTemplateColumn();
            }
            else if ( FFixedFid( fid ) )
            {
                if ( fidFixedLeast-1 == ptcibTemplateTable->fidFixedLast
                    || fid <= ptcibTemplateTable->fidFixedLast )
                    rgidxseg[iidxseg].SetFTemplateColumn();
            }
            else
            {
                Assert( FVarFid( fid ) );
                if ( fidVarLeast-1 == ptcibTemplateTable->fidVarLast
                    || fid <= ptcibTemplateTable->fidVarLast )
                    rgidxseg[iidxseg].SetFTemplateColumn();
            }
        }

        else if ( fTemplateTable )
        {
            Assert( NULL == ptcibTemplateTable );
            Assert( !rgidxseg[iidxseg].FTemplateColumn() );
            rgidxseg[iidxseg].SetFTemplateColumn();
        }

        rgidxseg[iidxseg].SetFid( fid );
        Assert( FCOLUMNIDValid( rgidxseg[iidxseg].Columnid() ) );
    }
}

ERR ErrIDBSetIdxSegFromOldFormat(
    IDB         * const pidb,
    TDB         * const ptdb,
    const BOOL  fConditional,
    const UnalignedLittleEndian< IDXSEG_OLD >   * const le_rgidxseg )
{
    IDXSEG      rgidxseg[JET_ccolKeyMost];
    const ULONG cidxseg                     = ( fConditional ? pidb->CidxsegConditional() : pidb->Cidxseg() );
    TCIB        tcibTemplateTable           = { FID( ptdb->FidFixedFirst()-1 ),
                                                FID( ptdb->FidVarFirst()-1 ),
                                                FID( ptdb->FidTaggedFirst()-1 ) };
#ifdef DEBUG
    if ( ptdb->FDerivedTable() )
    {
        Assert( ptdb->FESE97DerivedTable() );
        ptdb->AssertValidDerivedTable();
    }
    else if ( ptdb->FTemplateTable() )
    {
        Assert( ptdb->FESE97TemplateTable() );
        ptdb->AssertValidTemplateTable();
    }
#endif

    if ( 0 == cidxseg )
    {
        Assert( fConditional );
        return JET_errSuccess;
    }

    SetIdxSegFromOldFormat(
            le_rgidxseg,
            rgidxseg,
            cidxseg,
            fConditional,
            ptdb->FTemplateTable(),
            ( ptdb->FDerivedTable() ? &tcibTemplateTable : NULL ) );

    return ( fConditional ?
                ErrIDBSetIdxSegConditional( pidb, ptdb, rgidxseg ) :
                ErrIDBSetIdxSeg( pidb, ptdb, rgidxseg ) );
}


const IDXSEG* PidxsegIDBGetIdxSeg( const IDB * const pidb, const TDB * const ptdb )
{
    const IDXSEG* pidxseg;

    if ( pidb->FTemplateIndex() || (pidb->FDerivedIndex() && pidb->FLocalizedText() ) )
    {
        if ( pfcbNil != ptdb->PfcbTemplateTable() )
        {
            ptdb->AssertValidDerivedTable();
            pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb->PfcbTemplateTable()->Ptdb() );
            return pidxseg;
        }

    }

    if ( pidb->FIsRgidxsegInMempool() )
    {
        Assert( pidb->ItagRgidxseg() != 0 );
        Assert( ptdb->MemPool().CbGetEntry( pidb->ItagRgidxseg() ) == pidb->Cidxseg() * sizeof(IDXSEG) );
        pidxseg = (IDXSEG*)ptdb->MemPool().PbGetEntry( pidb->ItagRgidxseg() );
    }
    else
    {
        Assert( pidb->Cidxseg() > 0 );
        pidxseg = pidb->rgidxseg;
    }

    return pidxseg;
}

const IDXSEG* PidxsegIDBGetIdxSegConditional( const IDB * const pidb, const TDB * const ptdb )
{
    const IDXSEG* pidxseg;

    if ( pidb->FTemplateIndex() || (pidb->FDerivedIndex() && pidb->FLocalizedText() ) )
    {
        if ( pfcbNil != ptdb->PfcbTemplateTable() )
        {
            ptdb->AssertValidDerivedTable();
            pidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb->PfcbTemplateTable()->Ptdb() );
            return pidxseg;
        }

    }

    if ( pidb->FIsRgidxsegConditionalInMempool() )
    {
        Assert( pidb->ItagRgidxsegConditional() != 0 );
        Assert( ptdb->MemPool().CbGetEntry( pidb->ItagRgidxsegConditional() ) == pidb->CidxsegConditional() * sizeof(IDXSEG) );
        pidxseg = (IDXSEG*)ptdb->MemPool().PbGetEntry( pidb->ItagRgidxsegConditional() );
    }
    else
    {
        pidxseg = pidb->rgidxsegConditional;
    }

    return pidxseg;
}


