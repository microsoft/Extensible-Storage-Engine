// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


ERR ErrCMPRECGetAutoInc( _Inout_ FUCB * const pfucbAutoIncTable, _Out_ QWORD * pqwAutoIncMax )
{
    ERR err = JET_errSuccess;

    Expected( !Pcsr( pfucbAutoIncTable )->FLatched() );

    Call( ErrDIRGetRootField( pfucbAutoIncTable, noderfIsamAutoInc, latchRIW ) );
    if ( err >= JET_errSuccess )
    {
        Assert( err == JET_errSuccess || err == JET_wrnColumnNull );

        if ( err == JET_errSuccess )
        {
            Assert( pfucbAutoIncTable->kdfCurr.data.Cb() == sizeof(QWORD) );
            *pqwAutoIncMax = *(UnalignedLittleEndian<QWORD>*)pfucbAutoIncTable->kdfCurr.data.Pv();
        }

        DIRReleaseLatch( pfucbAutoIncTable );
    }

HandleError:

    Assert( err == JET_errSuccess || err == JET_wrnColumnNull );

    return err;
}

ERR ErrCMPRECSetAutoInc( _Inout_ FUCB * const pfucbAutoIncTable, _In_ QWORD qwAutoIncSet )
{
    ERR err = JET_errSuccess;
    DATA dataAutoIncSet;

    Call( ErrDIRGetRootField( pfucbAutoIncTable, noderfIsamAutoInc, latchRIW ) );
    if ( err == JET_wrnColumnNull )
    {
        AssertTrack( fFalse, "AutoIncRootHighWaterNull" );
        DIRReleaseLatch( pfucbAutoIncTable );
        Call( ErrERRCheck( errCodeInconsistency ) );
    }

    dataAutoIncSet.SetPv( &qwAutoIncSet );
    dataAutoIncSet.SetCb( sizeof(qwAutoIncSet) );
    err = ErrDIRSetRootField( pfucbAutoIncTable, noderfIsamAutoInc, dataAutoIncSet );

    DIRReleaseLatch( pfucbAutoIncTable );

HandleError:
    return err;
}

ERR ErrRECInitAutoIncSpace( _In_ FUCB* const pfucb, QWORD qwAutoInc )
{
    ERR     err         = JET_errSuccess;
    DATA    dataAutoInc;
    BOOL fTransactionStarted = fFalse;

    Expected( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( efvExtHdrRootFieldAutoInc ) >= JET_errSuccess );

    CallR( ErrDIRGetRootField( pfucb, noderfIsamAutoInc, latchRIW ) );
    dataAutoInc.SetPv( &qwAutoInc );
    dataAutoInc.SetCb( sizeof(qwAutoInc) );
    if ( pfucb->ppib->Level() == 0 )
    {
        Call( ErrDIRBeginTransaction( pfucb->ppib, 47054, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }
    Call( ErrDIRSetRootField( pfucb, noderfIsamAutoInc, dataAutoInc ) );
    if ( fTransactionStarted )
    {
        Call( ErrDIRCommitTransaction( pfucb->ppib, NO_GRBIT ) );
        fTransactionStarted = fFalse;
    }

HandleError:
    if ( fTransactionStarted )
    {
        CallS( ErrDIRRollback( pfucb->ppib ) );
    }
    DIRReleaseLatch( pfucb );
    return err;
}

LOCAL ERR ErrRECILoadAutoIncBatch( _In_ FUCB* const pfucb, _In_opt_ const QWORD qwAutoInc = 0 )
{
    ERR         err     = JET_errSuccess;
    FCB* const  pfcb    = pfucb->u.pfcb;
    TDB* const  ptdb    = pfcb->Ptdb();
    PIB* const  ppib    = pfucb->ppib;
    DATA        dataAutoInc;
    BOOL        fTransactionStarted = fFalse;

    if ( ptdb->FAutoIncOldFormat() )
    {
        err = ErrERRCheck( JET_wrnColumnNull );
        return err;
    }

    QWORD qwAutoIncStored = 0;
    CallR( ErrDIRGetRootField( pfucb, noderfIsamAutoInc, latchRIW ) );
    if ( err == JET_wrnColumnNull )
    {
        ptdb->SetAutoIncOldFormat();
        goto HandleError;
    }

    if ( qwAutoInc == 0 )
    {
        if ( ptdb->QwAutoincrement() < ptdb->QwGetAllocatedAutoIncMax() )
        {
            goto HandleError;
        }

        if ( pfucb->kdfCurr.data.Cb() != sizeof(QWORD) )
        {
            AssertSz( pfucb->kdfCurr.data.Cb() == sizeof(QWORD), "The AutoInc external header should be exactly QWORD wide!" );
            Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
        }
        qwAutoIncStored = *(UnalignedLittleEndian<QWORD>*)pfucb->kdfCurr.data.Pv();
        if ( qwAutoIncStored == 0 )
        {
            Assert( ptdb->QwGetAllocatedAutoIncMax() == 0 );
            Assert( ptdb->QwAutoincrement() == 0 );
        }
    }
    else
    {
        Assert( ptdb->QwGetAllocatedAutoIncMax() == 0 );
        Assert( ptdb->QwAutoincrement() == 0 );
        qwAutoIncStored = qwAutoInc - 1;
    }

    ULONG ulBatchSize = ptdb->DwGetAutoIncBatchSize();
    Assert( ulBatchSize >= 1 );
    if ( ulBatchSize < g_ulAutoIncBatchSize )
    {
        Assert( ( ulBatchSize << 1 ) > ulBatchSize );
        ulBatchSize <<= 1;
        if ( ulBatchSize > g_ulAutoIncBatchSize )
        {
            ulBatchSize = g_ulAutoIncBatchSize;
        }
        ptdb->SetAutoIncBatchSize( ulBatchSize );
    }

    while ( qwAutoIncStored >= ( qwCounterMax - ulBatchSize ) && ulBatchSize > 1 )
    {
        ulBatchSize >>= 1;
        ptdb->SetAutoIncBatchSize( ulBatchSize );
    }
    if ( qwAutoIncStored >= ( qwCounterMax - ulBatchSize ) )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagAlertOnly, L"d0a09b0c-e038-4f8e-8437-ba448e8080bd" );
        Call( ErrERRCheck( JET_errOutOfAutoincrementValues ) );
    }

    qwAutoIncStored += ulBatchSize;
    dataAutoInc.SetPv( &qwAutoIncStored );
    dataAutoInc.SetCb( sizeof(qwAutoIncStored) );

    if ( ppib->Level() == 0 )
    {
        Call( ErrDIRBeginTransaction( ppib, 36256, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }
    Call( ErrDIRSetRootField( pfucb, noderfIsamAutoInc, dataAutoInc ) );
    if ( fTransactionStarted )
    {
        Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
        fTransactionStarted = fFalse;
    }

    if ( ptdb->QwAutoincrement() == 0 )
    {
        Assert( ptdb->QwGetAllocatedAutoIncMax() == 0 );
        ptdb->InitAutoincrement( qwAutoIncStored - ulBatchSize + 1 );
    }
    ptdb->SetAllocatedAutoIncMax( qwAutoIncStored );

HandleError:
    if ( fTransactionStarted )
    {
        CallS( ErrDIRRollback( ppib ) );
    }
    DIRReleaseLatch( pfucb );
    return err;
}

LOCAL ERR ErrRECIGetAndIncrAutoincrement( _In_ FUCB* const pfucb, _In_ QWORD* const pqwAutoInc )
{
    ERR         err     = JET_errSuccess;
    FCB* const  pfcb    = pfucb->u.pfcb;
    TDB* const  ptdb    = pfcb->Ptdb();

    QWORD newAutoInc = 0;
    Call( ptdb->ErrGetAndIncrAutoincrement( &newAutoInc ) );

    if ( !ptdb->FAutoIncOldFormat()
      && newAutoInc > ptdb->QwGetAllocatedAutoIncMax() )
    {
        Assert( ptdb->QwGetAllocatedAutoIncMax() > 0 );
        Call( ErrRECILoadAutoIncBatch( pfucb ) );
    }
    *pqwAutoInc = newAutoInc;

HandleError:
    return err;
}

LOCAL ERR ErrRECIInitAutoIncOldFormat( PIB * const ppib, FCB * const pfcb, QWORD *pqwAutoInc )
{
    ERR             err                 = JET_errSuccess;
    FUCB *          pfucb               = pfucbNil;
    TDB * const     ptdb                = pfcb->Ptdb();
    const BOOL      fTemplateColumn     = ptdb->FFixedTemplateColumn( ptdb->FidAutoincrement() );
    const COLUMNID  columnidAutoInc     = ColumnidOfFid( ptdb->FidAutoincrement(), fTemplateColumn );
    const BOOL      f8BytesAutoInc      = ptdb->F8BytesAutoInc();
    FCB *           pfcbIdx             = pfcbNil;
    BOOL            fDescending         = fFalse;
    DIB             dib;
    DATA            dataField;
    QWORD           qwAutoInc           = 0;

    Assert( ptdb->FAutoIncOldFormat() );
    Assert( 0 != ptdb->FidAutoincrement() );

    pfcb->EnterDML();
    for ( pfcbIdx = ( pidbNil == pfcb->Pidb() ? pfcb->PfcbNextIndex() : pfcb );
        pfcbNil != pfcbIdx;
        pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        const IDB * const   pidb    = pfcbIdx->Pidb();
        Assert( pidbNil != pidb );

        if ( !pidb->FDeleted()
            && !pidb->FVersioned()
            && 0 == pidb->CidxsegConditional()
            && ( pidb->FNoNullSeg() || pidb->FAllowAllNulls() ) )
        {
            const IDXSEG * const    rgidxseg    = PidxsegIDBGetIdxSeg( pfcbIdx->Pidb(), ptdb );
            if ( rgidxseg[0].Columnid() == columnidAutoInc )
            {
                if ( rgidxseg[0].FDescending() )
                    fDescending = fTrue;
                break;
            }
        }
    }
    pfcb->LeaveDML();

    dib.dirflag = fDIRAllNode | fDIRAllNodesNoCommittedDeleted;
    dib.pbm = NULL;

    if ( pfcbIdx != pfcbNil )
    {
        Call( ErrBTOpen( ppib, pfcbIdx, &pfucb ) );
        Assert( pfucbNil != pfucb );

        dib.pos = ( fDescending ? posFirst : posLast );
        err = ErrBTDown( pfucb, &dib, latchReadTouch );
        if ( JET_errRecordNotFound == err )
        {
            Assert( 0 == qwAutoInc );
        }
        else
        {
            Assert( wrnNDFoundLess != err );
            Assert( wrnNDFoundGreater != err );
            Assert( JET_errNoCurrentRecord != err );
            Call( err );

            Assert( Pcsr( pfucb )->FLatched() );

            dataField.SetPv( &qwAutoInc );

            Assert( 0 == qwAutoInc );

            pfcb->EnterDML();
            err = ErrRECIRetrieveColumnFromKey(
                            ptdb,
                            pfcbIdx->Pidb(),
                            &pfucb->kdfCurr.key,
                            columnidAutoInc,
                            &dataField );
            pfcb->LeaveDML();
            CallS( err );
            Call( err );

            Assert( (SIZE_T)dataField.Cb() == ( f8BytesAutoInc ? sizeof(QWORD) : sizeof(ULONG) ) );
            Assert( qwAutoInc > 0 );
        }
    }
    else
    {
        Call( ErrBTOpen( ppib, pfcb, &pfucb ) );
        Assert( pfucbNil != pfucb );

        FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
        dib.pos     = posFirst;

        err = ErrBTDown( pfucb, &dib, latchReadTouch );
        Assert( JET_errNoCurrentRecord != err );
        if ( JET_errRecordNotFound == err )
        {
            Assert( 0 == qwAutoInc );
        }
        else
        {
            FCB *   pfcbT   = pfcb;

            if ( fTemplateColumn )
            {
                if ( !pfcb->FTemplateTable() )
                {
                    pfcb->Ptdb()->AssertValidDerivedTable();
                    pfcbT = pfcb->Ptdb()->PfcbTemplateTable();
                }
                else
                {
                    pfcb->Ptdb()->AssertValidTemplateTable();
                }
            }
            else
            {
                Assert( !pfcb->Ptdb()->FTemplateTable() );
            }
            
            do
            {
                Assert( wrnNDFoundLess != err );
                Assert( wrnNDFoundGreater != err );
                Call( err );

                Assert( locOnCurBM == pfucb->locLogical );
                Assert( Pcsr( pfucb )->FLatched() );
                
                Assert( FCOLUMNIDFixed( columnidAutoInc ) );
                Call( ErrRECIRetrieveFixedColumn(
                            pfcbT,
                            pfcbT->Ptdb(),
                            columnidAutoInc,
                            pfucb->kdfCurr.data,
                            &dataField ) );

                CallS( err );
                if ( err == JET_wrnColumnNull )
                {
                    FireWall( "AutoIncColumnNullOldFormat" );
                    Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
                }

                QWORD   qwT;
                if ( f8BytesAutoInc )
                {
                    Assert( sizeof(QWORD) == dataField.Cb() );
                    qwT = *( (UnalignedLittleEndian< QWORD > *)dataField.Pv() );
                }
                else
                {
                    Assert( sizeof(ULONG) == dataField.Cb() );
                    qwT = *( (UnalignedLittleEndian< ULONG > *)dataField.Pv() );
                }
                
                if ( qwT > qwAutoInc )
                {
                    qwAutoInc = qwT;
                }

                err = ErrBTNext( pfucb, fDIRAllNode | fDIRAllNodesNoCommittedDeleted );
            }
            while ( JET_errNoCurrentRecord != err );

            Assert( JET_errNoCurrentRecord == err );
        }
    }
    
    *pqwAutoInc = ++qwAutoInc;
    err = JET_errSuccess;

HandleError:
    if ( pfucbNil != pfucb )
    {
        BTClose( pfucb );
    }

    return err;
}

ERR ErrRECIInitAutoIncrement( _In_ FUCB* const pfucb, QWORD qwAutoInc )
{
    FCB* const  pfcb    = pfucb->u.pfcb;
    TDB* const  ptdb    = pfcb->Ptdb();
    ERR         err     = JET_errSuccess;

    Call( ErrRECILoadAutoIncBatch( pfucb, qwAutoInc ) );
    if ( JET_wrnColumnNull == err )
    {
        Assert( ptdb->FAutoIncOldFormat() );
        err = JET_errSuccess;

        if ( qwAutoInc != 0 )
        {
            ptdb->InitAutoincrement( qwAutoInc );
        }
        else
        {
            Call( ErrRECIInitAutoIncOldFormat( pfucb->ppib, pfcb, &qwAutoInc ) );

            const BOOL fExternalHeaderNewFormatEnabled =
                ( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( efvExtHdrRootFieldAutoInc ) >= JET_errSuccess );
            if ( fExternalHeaderNewFormatEnabled &&
                 ErrRECInitAutoIncSpace( pfucb, qwAutoInc - 1 ) >= JET_errSuccess )
            {
                ptdb->ResetAutoIncOldFormat();
                err = ErrRECILoadAutoIncBatch( pfucb );
                CallS( err );
            }
            else
            {
                ptdb->InitAutoincrement( qwAutoInc );
            }
        }
    }
HandleError:
    return err;
}

ERR TDB::ErrInitAutoInc( FUCB * const pfucb, QWORD qwAutoInc )
{
    ERR err = m_AutoIncInitOnce.Init( ErrRECIInitAutoIncrement, pfucb, qwAutoInc );
    Assert( err < JET_errSuccess || QwAutoincrement() != 0 );
    return err;
}

LOCAL ERR ErrRECISetAutoincrement( FUCB *pfucb )
{
    ERR             err             = JET_errSuccess;
    FCB             * pfcbT         = pfucb->u.pfcb;
    TDB             * const ptdbT   = pfcbT->Ptdb();
    const BOOL      fTemplateColumn = ptdbT->FFixedTemplateColumn( ptdbT->FidAutoincrement() );
    const COLUMNID  columnidT       = ColumnidOfFid( ptdbT->FidAutoincrement(), fTemplateColumn );
    QWORD           qwT             = 0;
    ULONG           ulT;
    const BOOL      f8BytesAutoInc  = ptdbT->F8BytesAutoInc();
    DATA            dataT;

    Assert( pfucb != pfucbNil );
    Assert( pfcbT != pfcbNil );

    Assert( !( pfcbT->FTypeSort()
            || pfcbT->FTypeTemporaryTable() ) );

    Call( ptdbT->ErrInitAutoInc( pfucb, 0 ) );

    Call( ErrRECIGetAndIncrAutoincrement( pfucb, &qwT ) );
    Assert( qwT > 0 );

    if ( !f8BytesAutoInc )
    {
        Assert( qwT < (QWORD)dwCounterMax );
        ulT = (ULONG)qwT;
        dataT.SetPv( &ulT );
        dataT.SetCb( sizeof(ulT) );
    }
    else
    {
        dataT.SetPv( &qwT );
        dataT.SetCb( sizeof(qwT) );
    }

    if ( fTemplateColumn )
    {
        Assert( FCOLUMNIDTemplateColumn( columnidT ) );
        if ( !pfcbT->FTemplateTable() )
        {
            pfcbT->Ptdb()->AssertValidDerivedTable();
            pfcbT = pfcbT->Ptdb()->PfcbTemplateTable();
        }
        else
        {
            pfcbT->Ptdb()->AssertValidTemplateTable();
        }
    }
    else
    {
        Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
        Assert( !pfcbT->FTemplateTable() );
    }

    pfcbT->EnterDML();
    err = ErrRECISetFixedColumn(
                pfucb,
                pfcbT->Ptdb(),
                columnidT,
                &dataT );
    pfcbT->LeaveDML();

HandleError:
    return err;
}


#if defined( DEBUG ) || !defined( RTM )
ERR ErrRECSessionWriteConflict( FUCB *pfucb )
{
    FUCB    *pfucbT;

    AssertDIRNoLatch( pfucb->ppib );
    for ( pfucbT = pfucb->ppib->pfucbOfSession; pfucbT != pfucbNil; pfucbT = pfucbT->pfucbNextOfSession )
    {
        Assert( pfucbT->ppib == pfucb->ppib );

        if ( pfucbT->ifmp == pfucb->ifmp
            && FFUCBReplacePrepared( pfucbT )
            && PgnoFDP( pfucbT ) == PgnoFDP( pfucb )
            && pfucbT != pfucb
            && 0 == CmpBM( pfucbT->bmCurr, pfucb->bmCurr ) )
        {
            WCHAR       szSession[32];
            WCHAR       szThreadID[16];
            WCHAR       szTableName[JET_cbNameMost+1];
            WCHAR       szConflictTableID1[32];
            WCHAR       szConflictTableID2[32];
            WCHAR       szBookmarkLength[32];
            WCHAR       szTransactionIds[512];
            BYTE        pbRawData[cbKeyAlloc];
            DWORD       cbRawData           = 0;
            const WCHAR * rgszT[]           = { szSession,
                                                szThreadID,
                                                g_rgfmp[pfucb->ifmp].WszDatabaseName(),
                                                szTableName,
                                                szConflictTableID1,
                                                szConflictTableID2,
                                                szBookmarkLength,
                                                szTransactionIds };

            OSStrCbFormatW( szSession, sizeof(szSession), L"0x%p", pfucb->ppib );
            OSStrCbFormatW( szThreadID, sizeof(szThreadID), L"0x%08lX", DwUtilThreadId() );
            OSStrCbFormatW( szTableName, sizeof(szTableName), L"%hs", pfucb->u.pfcb->Ptdb()->SzTableName() );
            OSStrCbFormatW( szConflictTableID1, sizeof(szConflictTableID1), L"0x%p", pfucb );
            OSStrCbFormatW( szConflictTableID2, sizeof(szConflictTableID2), L"0x%p", pfucbT );
            OSStrCbFormatW( szBookmarkLength, sizeof(szBookmarkLength), L"0x%lX/0x%lX",
                pfucb->bmCurr.key.prefix.Cb(), pfucb->bmCurr.key.suffix.Cb() );

            Assert( pfucb->bmCurr.key.Cb() < sizeof( pbRawData ) );
            Assert( pfucb->bmCurr.data.FNull() );
            pfucb->bmCurr.key.CopyIntoBuffer(pbRawData, sizeof(pbRawData) );
            Assert( cbRawData <= sizeof(pbRawData) );

            (void)pfucb->ppib->TrxidStack().ErrDump( szTransactionIds, _countof( szTransactionIds ) );

            UtilReportEvent(
                    eventError,
                    TRANSACTION_MANAGER_CATEGORY,
                    SESSION_WRITE_CONFLICT_ID,
                    _countof(rgszT),
                    rgszT,
                    cbRawData,
                    pbRawData,
                    PinstFromPfucb( pfucb ) );
            FireWall( "SessionWriteConflict" );
            return ErrERRCheck( JET_errSessionWriteConflict );
        }
    }

    return JET_errSuccess;
}
#endif


ERR VTAPI ErrIsamPrepareUpdate( JET_SESID sesid, JET_VTID vtid, ULONG grbit )
{
    ERR     err;
    PIB *   ppib                = reinterpret_cast<PIB *>( sesid );
    FUCB *  pfucb               = reinterpret_cast<FUCB *>( vtid );
    BOOL    fFreeCopyBufOnErr   = fFalse;
    
    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    Assert( ppib->Level() < levelMax );
    AssertDIRNoLatch( ppib );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] preparing update on objid=[0x%x:0x%x] [grbit=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            grbit ) );

    switch ( grbit )
    {
        case JET_prepCancel:
            if ( !FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errUpdateNotPrepared );

            if( FFUCBUpdateSeparateLV( pfucb ) )
            {
                Assert( updateidNil != pfucb->updateid );
                Assert( pfucb->ppib->Level() > 0 );
                err = ErrVERRollback( pfucb->ppib, pfucb->updateid );
                CallSx( err, JET_errRollbackError );
                Call ( err );
            }

            RECIFreeCopyBuffer( pfucb );
            FUCBResetUpdateFlags( pfucb );
            break;

        case JET_prepInsert:
            CallR( ErrFUCBCheckUpdatable( pfucb ) );
            if ( !FFMPIsTempDB(pfucb->ifmp) )
            {
                CallR( ErrPIBCheckUpdatable( ppib ) );
            }
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            RECIAllocCopyBuffer( pfucb );
            fFreeCopyBufOnErr = fTrue;
            Assert( pfucb->pvWorkBuf != NULL );

            Assert( pfucb != pfucbNil );
            Assert( pfucb->dataWorkBuf.Pv() != NULL );
            Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

#ifdef PREREAD_INDEXES_ON_PREPINSERT
            if( pfucb->u.pfcb->FPreread() )
            {
                BTPrereadIndexesOfFCB( pfucb );
            }
#endif

            Assert( pfucb->u.pfcb != pfcbNil );
            pfucb->u.pfcb->EnterDML();

            if ( NULL == pfucb->u.pfcb->Ptdb()->PdataDefaultRecord() )
            {
                Assert( ( FFUCBSort( pfucb ) && pfucb->u.pfcb->FTypeSort() )
                    || pfucb->u.pfcb->FTypeTemporaryTable()
                    || FCATSystemTable( pfucb->u.pfcb->Ptdb()->SzTableName() ) );
                pfucb->u.pfcb->LeaveDML();

                REC::SetMinimumRecord( pfucb->dataWorkBuf );
            }
            else
            {
                TDB     *ptdbT = pfucb->u.pfcb->Ptdb();
                BOOL    fBurstDefaultRecord;

                Assert( !FFUCBSort( pfucb ) );
                Assert( !pfucb->u.pfcb->FTypeSort() );
                Assert( !pfucb->u.pfcb->FTypeTemporaryTable() );
                Assert( !FCATSystemTable( ptdbT->SzTableName() ) );

                Assert( ptdbT->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );
                Assert( ptdbT->PdataDefaultRecord()->Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
                if ( ptdbT->PdataDefaultRecord()->Cb() > REC::CbRecordMost( pfucb ) )
                {
                    FireWall( "PrepInsertDefaultRecTooBig0.2" );
                }
                if ( ptdbT->PdataDefaultRecord()->Cb() < REC::cbRecordMin
                    || ptdbT->PdataDefaultRecord()->Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) )
                {
                    FireWall( "PrepInsertDefaultRecTooBig0.1" );
                    Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                }

                REC *precDefault = (REC *)ptdbT->PdataDefaultRecord()->Pv();

                Assert( precDefault->FidFixedLastInRec() <= ptdbT->FidFixedLast() );
                Assert( precDefault->FidVarLastInRec() <= ptdbT->FidVarLast() );

                fBurstDefaultRecord = fTrue;
                if ( precDefault->FidFixedLastInRec() >= fidFixedLeast )
                {
                    const FID   fid             = precDefault->FidFixedLastInRec();
                    const BOOL  fTemplateColumn = ptdbT->FFixedTemplateColumn( fid );
                    FIELD       *pfield         = ptdbT->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );

                    if ( FFIELDVersioned( pfield->ffield )
                        || ( FFIELDDeleted( pfield->ffield ) && fid > ptdbT->FidFixedLastInitial() ) )
                        fBurstDefaultRecord = fFalse;
                }
                if ( precDefault->FidVarLastInRec() >= fidVarLeast )
                {
                    const FID   fid             = precDefault->FidVarLastInRec();
                    const BOOL  fTemplateColumn = ptdbT->FVarTemplateColumn( fid );
                    FIELD       *pfield         = ptdbT->PfieldVar( ColumnidOfFid( fid, fTemplateColumn ) );

                    if ( FFIELDVersioned( pfield->ffield )
                        || ( FFIELDDeleted( pfield->ffield ) && fid > ptdbT->FidVarLastInitial() ) )
                        fBurstDefaultRecord = fFalse;
                }

                if ( fBurstDefaultRecord )
                {
                    pfucb->dataWorkBuf.SetCb( precDefault->PbTaggedData() - (BYTE *)precDefault );
                    Assert( pfucb->dataWorkBuf.Cb() >= REC::cbRecordMin );
                    Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
                    Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMost( pfucb ) );
                    Assert( pfucb->dataWorkBuf.Cb() <= ptdbT->PdataDefaultRecord()->Cb() );
                    if ( pfucb->dataWorkBuf.Cb() > REC::CbRecordMost( pfucb ) )
                    {
                        FireWall( "PrepInsertWorkingBufferAllowedRecTooBig1.2" );
                    }
                    if ( pfucb->dataWorkBuf.Cb() < REC::cbRecordMin
                        || pfucb->dataWorkBuf.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() )
                        || pfucb->dataWorkBuf.Cb() > ptdbT->PdataDefaultRecord()->Cb() )
                    {
                        FireWall( "PrepInsertWorkingBufferAllowedRecTooBig1.1" );
                        Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                    }

                    UtilMemCpy( pfucb->dataWorkBuf.Pv(),
                            (BYTE *)precDefault,
                            pfucb->dataWorkBuf.Cb() );
                    
                    pfucb->u.pfcb->LeaveDML();
                }
                else
                {

                    FID fidBurst;
                    for ( fidBurst = precDefault->FidFixedLastInRec();
                        fidBurst >= fidFixedLeast;
                        fidBurst-- )
                    {
                        const BOOL  fTemplateColumn = ptdbT->FFixedTemplateColumn( fidBurst );
                        const FIELD *pfield         = ptdbT->PfieldFixed( ColumnidOfFid( fidBurst, fTemplateColumn ) );
                        if ( !FFIELDVersioned( pfield->ffield )
                            && !FFIELDDeleted( pfield->ffield ) )
                        {
                            break;
                        }
                    }
                    if ( fidBurst >= fidFixedLeast )
                    {
                        BYTE    *pbRec  = (BYTE *)pfucb->dataWorkBuf.Pv();
                        REC     *prec   = (REC *)pbRec;

                        Assert( fidBurst <= fidFixedMost );
                        const INT   cFixedColumnsToBurst = fidBurst - fidFixedLeast + 1;
                        Assert( cFixedColumnsToBurst > 0 );

                        Assert( !ptdbT->FInitialisingDefaultRecord() );
                        const INT   ibFixEnd = ptdbT->IbOffsetOfNextColumn( fidBurst );
                        const INT   cbBitMap = ( cFixedColumnsToBurst + 7 ) / 8;
                        const INT   cbFixedBurst = ibFixEnd + cbBitMap;
                        pfucb->dataWorkBuf.SetCb( cbFixedBurst );

                        Assert( pfucb->dataWorkBuf.Cb() <= ptdbT->PdataDefaultRecord()->Cb() );
                        Assert( pfucb->dataWorkBuf.Cb() >= REC::cbRecordMin );
                        Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
                        Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMost( pfucb ) );
                        if ( pfucb->dataWorkBuf.Cb() > REC::CbRecordMost( pfucb ) )
                        {
                            FireWall( "WorkingBufferAllowedRecTooBig2.2" );
                        }
                        if ( pfucb->dataWorkBuf.Cb() < REC::cbRecordMin ||
                            pfucb->dataWorkBuf.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) ||
                            pfucb->dataWorkBuf.Cb() > ptdbT->PdataDefaultRecord()->Cb() )
                        {
                            FireWall( "WorkingBufferAllowedRecTooBig2.1" );
                            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                        }

                        UtilMemCpy( pbRec, (BYTE *)precDefault, cbFixedBurst );

                        prec->SetFidFixedLastInRec( fidBurst );
                        prec->SetFidVarLastInRec( fidVarLeast-1 );
                        prec->SetIbEndOfFixedData( (USHORT)cbFixedBurst );

                        BYTE    *pbDefaultBitMap    = precDefault->PbFixedNullBitMap();
                        Assert( pbDefaultBitMap - (BYTE *)precDefault ==
                            ptdbT->IbOffsetOfNextColumn( precDefault->FidFixedLastInRec() ) );

                        UtilMemCpy( pbRec + ibFixEnd, pbDefaultBitMap, cbBitMap );

                        BYTE    *pbitNullity = pbRec + cbFixedBurst - 1;
                        Assert( pbitNullity == pbRec + ibFixEnd + ( ( fidBurst - fidFixedLeast ) / 8 ) );

                        for ( FID fidT = FID( fidBurst + 1 ); ; fidT++ )
                        {
                            const UINT  ifid = fidT - fidFixedLeast;

                            if ( ( pbRec + ibFixEnd + ifid/8 ) != pbitNullity )
                            {
                                Assert( ( pbRec + ibFixEnd + ifid/8 ) == ( pbitNullity + 1 ) );
                                break;
                            }

                            Assert( pbitNullity - pbRec < cbFixedBurst );
                            Assert( fidT < fidBurst + 8 );
                            SetFixedNullBit( pbitNullity, ifid );
                        }

                        pfucb->u.pfcb->LeaveDML();
                    }
                    else
                    {
                        pfucb->u.pfcb->LeaveDML();

                        REC::SetMinimumRecord( pfucb->dataWorkBuf );
                    }
                }
            }

            FUCBResetColumnSet( pfucb );

            if ( pfucb->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
            {
                Call( ErrRECISetAutoincrement( pfucb ) );
            }
            err = JET_errSuccess;
            PrepareInsert( pfucb );
            break;

        case JET_prepReplace:
            CallR( ErrFUCBCheckUpdatable( pfucb ) );
            if ( !FFMPIsTempDB(pfucb->ifmp) )
            {
                CallR( ErrPIBCheckUpdatable( ppib ) );
            }
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            if ( pfucb->ppib->Level() == 0 )
            {
                goto ReplaceNoLock;
            }

            Call( ErrRECSessionWriteConflict( pfucb ) );

            Assert( !FFUCBUpdateSeparateLV( pfucb ) );

            Call( ErrDIRGetLock( pfucb, writeLock ) );
            Call( ErrDIRGet( pfucb ) );

            if( prceNil != pfucb->ppib->prceNewest )
            {
                pfucb->rceidBeginUpdate = pfucb->ppib->prceNewest->Rceid();
            }
            else
            {
                Assert( g_rgfmp[pfucb->ifmp].FVersioningOff() );
                pfucb->rceidBeginUpdate = rceidNull;
            }

            RECIAllocCopyBuffer( pfucb );
            fFreeCopyBufOnErr = fTrue;
            Assert( pfucb->pvWorkBuf != NULL );
            pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );

            Call( ErrDIRRelease( pfucb ) );
            FUCBResetColumnSet( pfucb );
            PrepareReplace( pfucb );
            break;

        case JET_prepReplaceNoLock:
            CallR( ErrFUCBCheckUpdatable( pfucb ) );
            if ( !FFMPIsTempDB(pfucb->ifmp) )
            {
                CallR( ErrPIBCheckUpdatable( ppib ) );
            }
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );

ReplaceNoLock:
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            Call( ErrRECSessionWriteConflict( pfucb ) );

            Call( ErrDIRGet( pfucb ) );

            RECIAllocCopyBuffer( pfucb );
            fFreeCopyBufOnErr = fTrue;
            Assert( pfucb->pvWorkBuf != NULL );
            pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );

            pfucb->rceidBeginUpdate = PinstFromPpib( ppib )->m_pver->RceidLast();

            FUCBResetColumnSet( pfucb );

            if ( pfucb->ppib->Level() == 0 )
            {
                StoreChecksum( pfucb );
            }
            else
            {
                FUCBSetDeferredChecksum( pfucb );
            }

            Call( ErrDIRRelease( pfucb ) );
            PrepareReplaceNoLock( pfucb );
            break;

        case JET_prepInsertCopy:
        case JET_prepInsertCopyDeleteOriginal:
        case JET_prepInsertCopyReplaceOriginal:
            CallR( ErrFUCBCheckUpdatable( pfucb ) );
            if ( !FFMPIsTempDB(pfucb->ifmp) )
            {
                CallR( ErrPIBCheckUpdatable( ppib ) );
            }
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            if ( JET_prepInsertCopyReplaceOriginal == grbit  || JET_prepInsertCopyDeleteOriginal == grbit )
            {
                Call( ErrDIRGetLock( pfucb, writeLock ) );
            }

            Call( ErrDIRGet( pfucb ) );

            RECIAllocCopyBuffer( pfucb );
            fFreeCopyBufOnErr = fTrue;
            Assert( pfucb->pvWorkBuf != NULL );
            pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );
            Assert( !pfucb->dataWorkBuf.FNull() );

            Call( ErrDIRRelease( pfucb ) );
            FUCBResetColumnSet( pfucb );

            if ( pfucb->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
            {
                if ( JET_prepInsertCopyReplaceOriginal == grbit )
                {
                    Call( pfucb->u.pfcb->Ptdb()->ErrInitAutoInc( pfucb, 0 ) );
                }
                else
                {
                    Call( ErrRECISetAutoincrement( pfucb ) );
                }
            }
            
            if ( JET_prepInsertCopyDeleteOriginal == grbit
                || JET_prepInsertCopyReplaceOriginal == grbit )
            {
                PrepareInsertCopyDeleteOriginal( pfucb );
            }
            else
            {
                PrepareInsertCopy( pfucb );

                Assert( updateidNil == ppib->updateid );
                PIBSetUpdateid( ppib, pfucb->updateid );
                err = ErrRECAffectLongFieldsInWorkBuf( pfucb, lvaffectReferenceAll );
                PIBSetUpdateid( ppib, updateidNil );
                if ( err < 0 )
                {
                    FUCBResetUpdateFlags( pfucb );
                    goto HandleError;
                }
            }
            break;

        case JET_prepReadOnlyCopy:
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            if( 0 == pfucb->ppib->Level() )
            {
                return ErrERRCheck( JET_errNotInTransaction );
            }


            Call( ErrDIRGet( pfucb ) );

            RECIAllocCopyBuffer( pfucb );
            fFreeCopyBufOnErr = fTrue;
            Assert( pfucb->pvWorkBuf != NULL );
            pfucb->kdfCurr.data.CopyInto( pfucb->dataWorkBuf );
            Assert( !pfucb->dataWorkBuf.FNull() );

            Call( ErrDIRRelease( pfucb ) );
            FUCBResetColumnSet( pfucb );

            PrepareInsertReadOnlyCopy( pfucb );
            break;

        default:
            err = ErrERRCheck( JET_errInvalidParameter );
            goto HandleError;
    }

    CallS( err );
    AssertDIRNoLatch( ppib );
    return err;

HandleError:
    Assert( err < 0 );
    if ( fFreeCopyBufOnErr )
    {
        RECIFreeCopyBuffer( pfucb );
    }
    AssertDIRNoLatch( ppib );
    return err;
}


ERR ErrRECICheckUniqueNormalizedTaggedRecordData(
    _In_ const FIELD *          pfield,
    _In_ const DATA&            dataFieldNorm,
    _In_ const DATA&            dataRecRaw,
    _In_ const NORM_LOCALE_VER* pnlv,
    _In_ const BOOL             fNormDataFieldTruncated )
{
    ERR                         err;
    DATA                        dataRecNorm;
    BYTE                        rgbRecNorm[JET_cbKeyMost_OLD];
    BOOL                        fNormDataRecTruncated;

    Assert( NULL != pfield );

    dataRecNorm.SetPv( rgbRecNorm );

    CallR( ErrFLDNormalizeTaggedData(
                pfield,
                dataRecRaw,
                dataRecNorm,
                pnlv,
                fFalse ,
                &fNormDataRecTruncated ) );

    CallS( err );

    if ( FDataEqual( dataFieldNorm, dataRecNorm ) )
    {
        if ( fNormDataFieldTruncated || fNormDataRecTruncated )
        {
            Assert( fNormDataFieldTruncated );
            Assert( fNormDataRecTruncated );
            err = ErrERRCheck( JET_errMultiValuedDuplicateAfterTruncation );
        }
        else
        {
            Assert( !fNormDataFieldTruncated );
            Assert( !fNormDataRecTruncated );
            err = ErrERRCheck( JET_errMultiValuedDuplicate );
        }
    }

    return err;
}


LOCAL ERR ErrRECICheckUniqueLVMultiValues(
    _In_ FUCB           * const pfucb,
    _In_ const COLUMNID columnid,
    _In_ const ULONG        itagSequence,
    _In_ const DATA&        dataToSet,
    _In_ const NORM_LOCALE_VER* pnlv,
    _Out_writes_opt_(JET_cbKeyMost_OLD) BYTE            * rgbLVData = NULL,
    _In_ const BOOL     fNormalizedDataToSetIsTruncated = fFalse )
{
    ERR             err;
    FCB             * const pfcb    = pfucb->u.pfcb;
    const BOOL      fNormalize      = ( NULL != rgbLVData );
    DATA            dataRetrieved;
    ULONG           itagSequenceT   = 0;

    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( pfcbNil != pfcb );
    Assert( ptdbNil != pfcb->Ptdb() );

#ifdef DEBUG
    pfcb->EnterDML();
    Assert( FRECLongValue( pfcb->Ptdb()->PfieldTagged( columnid )->coltyp ) );
    Assert( !FFIELDEncrypted( pfcb->Ptdb()->PfieldTagged( columnid )->ffield ) );
    pfcb->LeaveDML();
#endif

    forever
    {
        itagSequenceT++;
        if ( itagSequenceT != itagSequence )
        {
            CallR( ErrRECIRetrieveTaggedColumn(
                    pfcb,
                    columnid,
                    itagSequenceT,
                    pfucb->dataWorkBuf,
                    &dataRetrieved,
                    JET_bitRetrieveIgnoreDefault | grbitRetrieveColumnDDLNotLocked ) );

            if ( wrnRECSeparatedLV == err )
            {
                if ( fNormalize )
                {
                    ULONG   cbActual;
                    CallR( ErrRECRetrieveSLongField(
                                pfucb,
                                LidOfSeparatedLV( dataRetrieved ),
                                fTrue,
                                0,
                                fFalse,
                                rgbLVData,
                                JET_cbKeyMost_OLD,
                                &cbActual ) );
                    dataRetrieved.SetPv( rgbLVData );
                    dataRetrieved.SetCb( min( cbActual, JET_cbKeyMost_OLD ) );

                    pfcb->EnterDML();
                    err = ErrRECICheckUniqueNormalizedTaggedRecordData(
                                    pfcb->Ptdb()->PfieldTagged( columnid ),
                                    dataToSet,
                                    dataRetrieved,
                                    pnlv,
                                    fNormalizedDataToSetIsTruncated );
                    pfcb->LeaveDML();
                    CallR( err );
                }
                else
                {
                    CallR( ErrRECRetrieveSLongField(
                                pfucb,
                                LidOfSeparatedLV( dataRetrieved ),
                                fTrue,
                                0,
                                fFalse,
                                (BYTE *)dataToSet.Pv(),
                                dataToSet.Cb(),
                                NULL ) );
                }
            }
            else if ( wrnRECCompressed == err )
            {
                BYTE * pbDecompressed = NULL;
                INT cbActual = 0;

                CallR( ErrPKAllocAndDecompressData(
                        dataRetrieved,
                        pfucb,
                        &pbDecompressed,
                        &cbActual ) );

                dataRetrieved.SetPv( pbDecompressed );
                dataRetrieved.SetCb( cbActual );

                if ( fNormalize )
                {
                    dataRetrieved.SetCb( min( dataRetrieved.Cb(), JET_cbKeyMost_OLD ) );
                    pfcb->EnterDML();
                    err = ErrRECICheckUniqueNormalizedTaggedRecordData(
                                    pfcb->Ptdb()->PfieldTagged( columnid ),
                                    dataToSet,
                                    dataRetrieved,
                                    pnlv,
                                    fNormalizedDataToSetIsTruncated );
                    pfcb->LeaveDML();
                }
                else if ( FDataEqual( dataToSet, dataRetrieved ) )
                {
                    err = ErrERRCheck( JET_errMultiValuedDuplicate );
                }

                delete[] pbDecompressed;
                pbDecompressed = NULL;
                CallR( err );
            }
            else if ( wrnRECIntrinsicLV == err )
            {
                if ( fNormalize )
                {
                    dataRetrieved.SetCb( min( dataRetrieved.Cb(), JET_cbKeyMost_OLD ) );
                    pfcb->EnterDML();
                    err = ErrRECICheckUniqueNormalizedTaggedRecordData(
                                    pfcb->Ptdb()->PfieldTagged( columnid ),
                                    dataToSet,
                                    dataRetrieved,
                                    pnlv,
                                    fNormalizedDataToSetIsTruncated );
                    pfcb->LeaveDML();
                    CallR( err );
                }
                else if ( FDataEqual( dataToSet, dataRetrieved ) )
                {
                    return ErrERRCheck( JET_errMultiValuedDuplicate );
                }
            }
            else
            {
                Assert( JET_wrnColumnNull == err
                    || ( wrnRECUserDefinedDefault == err && 1 == itagSequenceT ) );
                break;
            }
        }
    }

    return JET_errSuccess;
}

LOCAL ERR ErrRECICheckUniqueNormalizedLVMultiValues(
    FUCB            * const pfucb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const DATA&     dataToSet,
    _In_ const NORM_LOCALE_VER* pnlv )
{
    ERR             err;
    FCB             * const pfcb                    = pfucb->u.pfcb;
    DATA            dataT;
    DATA            dataToSetNorm;
    BYTE            rgbDataToSetNorm[JET_cbKeyMost_OLD];
    BYTE            rgbLVData[JET_cbKeyMost_OLD];
    BOOL            fNormalizedDataToSetIsTruncated;

    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( pfcbNil != pfcb );
    Assert( ptdbNil != pfcb->Ptdb() );

    dataT.SetPv( dataToSet.Pv() );
    dataT.SetCb( min( dataToSet.Cb(), JET_cbKeyMost_OLD ) );

    dataToSetNorm.SetPv( rgbDataToSetNorm );

    pfcb->EnterDML();
    Assert( FRECLongValue( pfcb->Ptdb()->PfieldTagged( columnid )->coltyp ) );
    err = ErrFLDNormalizeTaggedData(
                pfcb->Ptdb()->PfieldTagged( columnid ),
                dataT,
                dataToSetNorm,
                pnlv,
                fFalse ,
                &fNormalizedDataToSetIsTruncated );
    pfcb->LeaveDML();

    CallR( err );

    return ErrRECICheckUniqueLVMultiValues(
                pfucb,
                columnid,
                itagSequence,
                dataToSetNorm,
                pnlv,
                rgbLVData,
                fNormalizedDataToSetIsTruncated );
}

LOCAL CompressFlags CalculateCompressFlags( FMP* const pfmp, const FIELD * const pfield, const ULONG grbit )
{
    const BOOL fLongValue = FRECLongValue( pfield->coltyp );

    CompressFlags compressFlags = compressNone;
    if( fLongValue )
    {
        if( grbit & JET_bitSetCompressed )
        {
            compressFlags = CompressFlags( compress7Bit | compressXpress );
        }
        else if( grbit & JET_bitSetUncompressed )
        {
            compressFlags = compressNone;
        }
        else if( FFIELDCompressed(pfield->ffield) )
        {
            compressFlags = CompressFlags( compress7Bit | compressXpress );
        }
        else
        {
#ifndef FORCE_COLUMN_COMPRESSION_ON
            compressFlags = compressNone;
#else
            compressFlags = CompressFlags( compress7Bit | compressXpress );
#endif
        }
    }
    return compressFlags;
}
    
LOCAL ERR ErrFLDSetOneColumn(
    FUCB        *pfucb,
    COLUMNID    columnid,
    DATA        *pdataField,
    ULONG       itagSequence,
    ULONG       ibLongValue,
    ULONG       grbit )
{
    ERR         err;
    FCB         *pfcb                       = pfucb->u.pfcb;
    BOOL        fEnforceUniqueMultiValues   = fFalse;
    ULONG       cbThreshold                 = (ULONG)g_cbPage;
    
    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] setting column on objid=[0x%x:0x%x] [columnid=0x%x,itag=0x%x,ib=0x%x,grbit=0x%x]",
            pfucb->ppib,
            ( ppibNil != pfucb->ppib ? pfucb->ppib->trxBegin0 : trxMax ),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            columnid,
            itagSequence,
            ibLongValue,
            grbit ) );

    if ( grbit & ( JET_bitSetUniqueMultiValues|JET_bitSetUniqueNormalizedMultiValues) )
    {
        if ( grbit & ( JET_bitSetAppendLV|JET_bitSetOverwriteLV|JET_bitSetSizeLV ) )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
            return err;
        }
        else if ( NULL == pdataField->Pv() || 0 == pdataField->Cb() )
        {
            if ( grbit & JET_bitSetZeroLength )
                fEnforceUniqueMultiValues = fTrue;
        }
        else
        {
            fEnforceUniqueMultiValues = fTrue;
        }
    }

    if ( ( grbit & JET_bitSetContiguousLV ) &&
            !( grbit & JET_bitSetSeparateLV ) )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "Using JET_bitSetContiguousLV without JET_bitSetSeparateLV" );
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    if ( ( grbit & JET_bitSetContiguousLV ) &&
            ( pdataField->Cb() < g_cbPage ) )
    {
        AssertSz( FNegTest( fInvalidAPIUsage ), "Using JET_bitSetContiguousLV on LV not big enough to warrant contiguous specifier." );
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    if ( ( grbit & grbitSetColumnInternalFlagsMask ) )

    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    CallR( ErrRECIAccessColumn( pfucb, columnid ) );

    if ( pdataField->Pv() == NULL && !( grbit & JET_bitSetSizeLV ) )
        pdataField->SetCb( 0 );

    Assert ( pdataField->Cb() >= 0 );

    if ( FCOLUMNIDFixed( columnid ) )
    {
#ifdef DEBUG
        FIELDFLAG   ffield;

        pfcb->EnterDML();
        ffield = pfcb->Ptdb()->PfieldFixed( columnid )->ffield;
        pfcb->LeaveDML();

        Assert( !( FFIELDVersion( ffield ) && FFIELDAutoincrement( ffield ) ) );
#endif

        if ( pfcb->Ptdb()->FidVersion() == FidOfColumnid( columnid ) )
        {
            Assert( pfcb->Ptdb()->FidAutoincrement() != FidOfColumnid( columnid ) );
            Assert( FFIELDVersion( ffield ) );

            if ( FFUCBReplacePrepared( pfucb ) )
                return ErrERRCheck( JET_errInvalidColumnType );
        }
        else if ( pfcb->Ptdb()->FidAutoincrement() == FidOfColumnid( columnid ) )
        {
            Assert( FFIELDAutoincrement( ffield ) );

            return ErrERRCheck( JET_errInvalidColumnType );
        }
    }

    else if ( FCOLUMNIDTagged( columnid ) )
    {
        pfcb->EnterDML();

        const FIELD *pfield             = pfcb->Ptdb()->PfieldTagged( columnid );
        const BOOL  fLongValue          = FRECLongValue( pfield->coltyp );
        const ULONG cbMaxLen            = pfield->cbMaxLen;

        const CompressFlags compressFlags = CalculateCompressFlags( &g_rgfmp[ pfucb->ifmp ], pfield, grbit );
        const BOOL fEncrypted           = FFIELDEncrypted( pfield->ffield );

        pfcb->LeaveDML();

        if ( fEncrypted && pfucb->pbEncryptionKey == NULL )
        {
            Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
        }

        if ( fLongValue )
        {
            if ( fEnforceUniqueMultiValues )
            {
                NORM_LOCALE_VER nlv =
                {
                    SORTIDNil,
                    PinstFromPfucb( pfucb )->m_dwLCMapFlagsDefault,
                    0,
                    0,
                    L'\0',
                };
                OSStrCbCopyW( &nlv.m_wszLocaleName[0], sizeof(nlv.m_wszLocaleName), PinstFromPfucb( pfucb )->m_wszLocaleNameDefault );

                if ( grbit & JET_bitSetUniqueNormalizedMultiValues )
                {
                    Call( ErrRECICheckUniqueNormalizedLVMultiValues(
                                pfucb,
                                columnid,
                                itagSequence,
                                *pdataField,
                                &nlv ) );
                }
                else
                {
                    Call( ErrRECICheckUniqueLVMultiValues(
                                pfucb,
                                columnid,
                                itagSequence,
                                *pdataField,
                                &nlv ) );
                }
            }

            Assert( ( (ULONG)g_cbPage ) == cbThreshold );
            err = ErrRECSetLongField(
                pfucb,
                columnid,
                itagSequence,
                pdataField,
                compressFlags,
                fEncrypted,
                grbit,
                ibLongValue,
                cbMaxLen,
                cbThreshold / 2 );

            if ( JET_errRecordTooBig == err )
            {
                Assert( (ULONG) g_cbPage == cbThreshold );
                const ULONG cbLid = LvId::CbLidFromCurrFormat( pfucb );
                while ( JET_errRecordTooBig == err && cbThreshold > cbLid )
                {
                    cbThreshold /= 2;
                    if ( cbThreshold < (ULONG) ( g_cbPage / 64 ) )
                    {
                        cbThreshold = cbLid;
                    }

                    Call( ErrRECAffectLongFieldsInWorkBuf( pfucb, lvaffectSeparateAll, cbThreshold ) );

                    err = ErrRECSetLongField(
                        pfucb,
                        columnid,
                        itagSequence,
                        pdataField,
                        compressFlags,
                        fEncrypted,
                        grbit,
                        ibLongValue,
                        cbMaxLen,
                        cbThreshold );
                }
            }

            goto HandleError;
        }
    }



    if ( pdataField->Cb() == 0 && !( grbit & JET_bitSetZeroLength ) )
        pdataField = NULL;

    Assert( !( grbit & grbitSetColumnInternalFlagsMask ) );

    err = ErrRECSetColumn( pfucb, columnid, itagSequence, pdataField, grbit );
    if ( JET_errRecordTooBig == err )
    {
        Assert( (ULONG) g_cbPage == cbThreshold );
        const ULONG cbLid = LvId::CbLidFromCurrFormat( pfucb );
        while ( err == JET_errRecordTooBig && cbThreshold > cbLid )
        {
            cbThreshold /= 2;
            if ( cbThreshold < (ULONG) ( g_cbPage / 64 ) )
            {
                cbThreshold = cbLid;
            }

            Call( ErrRECAffectLongFieldsInWorkBuf( pfucb, lvaffectSeparateAll, cbThreshold ) );

            err = ErrRECSetColumn( pfucb, columnid, itagSequence, pdataField, grbit );
        }
    }

HandleError:
    AssertDIRNoLatch( pfucb->ppib );
    return err;
}



ERR VTAPI ErrIsamSetColumn(
    JET_SESID       sesid,
    JET_VTID        vtid,
    JET_COLUMNID    columnid,
    const VOID*     pvData,
    const ULONG     cbData,
    ULONG           grbit,
    JET_SETINFO*    psetinfo )
{
    ERR             err;
    PIB*            ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB*           pfucb       = reinterpret_cast<FUCB *>( vtid );
    DATA            dataField;
    ULONG           itagSequence;
    ULONG           ibLongValue;

    CallR( ErrFUCBCheckUpdatable( pfucb ) );
    if ( !FFMPIsTempDB(pfucb->ifmp) )
    {
        CallR( ErrPIBCheckUpdatable( ppib ) );
    }

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );

    if ( cbData > JET_cbLVColumnMost )
        return ErrERRCheck( JET_errInvalidParameter );

    if ( !FFUCBSetPrepared( pfucb ) )
        return ErrERRCheck( JET_errUpdateNotPrepared );

    const UPDATEID  updateidSave    = ppib->updateid;
    PIBSetUpdateid( ppib, pfucb->updateid );

    if ( psetinfo != NULL )
    {
        if ( psetinfo->cbStruct < sizeof(JET_SETINFO) )
        {
            err = ErrERRCheck( JET_errInvalidParameter );
            goto HandleError;
        }
        itagSequence = psetinfo->itagSequence;
        ibLongValue = psetinfo->ibLongValue;
    }
    else
    {
        itagSequence = 1;
        ibLongValue = 0;
    }

    dataField.SetPv( (VOID *)pvData );
    dataField.SetCb( cbData );

    err = ErrFLDSetOneColumn(
                pfucb,
                columnid,
                &dataField,
                itagSequence,
                ibLongValue,
                grbit );

HandleError:
    PIBSetUpdateid( ppib, updateidSave );
    AssertDIRNoLatch( ppib );
    return err;
}


ERR VTAPI ErrIsamSetColumns(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    JET_SETCOLUMN   *psetcols,
    ULONG   csetcols )
{
    ERR             err;
    PIB             *ppib = (PIB *)vsesid;
    FUCB            *pfucb = (FUCB *)vtid;
    ULONG           ccols;
    DATA            dataField;
    JET_SETCOLUMN   *psetcolcurr;

    CallR( ErrFUCBCheckUpdatable( pfucb ) );
    if ( !FFMPIsTempDB( pfucb->ifmp ) )
    {
        CallR( ErrPIBCheckUpdatable( ppib ) );
    }

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckFUCB( ppib, pfucb );

    if ( !FFUCBSetPrepared( pfucb ) )
        return ErrERRCheck( JET_errUpdateNotPrepared );

    for ( psetcolcurr = psetcols, ccols = 0; ccols < csetcols ; ++ccols, ++psetcolcurr )
        if ( psetcolcurr->cbData > JET_cbLVColumnMost )
            return ErrERRCheck( JET_errInvalidParameter );
        
    const UPDATEID  updateidSave    = ppib->updateid;
    PIBSetUpdateid( ppib, pfucb->updateid );

    for ( ccols = 0; ccols < csetcols ; ccols++ )
    {
        psetcolcurr = psetcols + ccols;

        dataField.SetPv( (VOID *)psetcolcurr->pvData );
        dataField.SetCb( psetcolcurr->cbData );

        Call( ErrFLDSetOneColumn(
            pfucb,
            psetcolcurr->columnid,
            &dataField,
            psetcolcurr->itagSequence,
            psetcolcurr->ibLongValue,
            psetcolcurr->grbit ) );
        psetcolcurr->err = err;
    }

HandleError:
    PIBSetUpdateid( ppib, updateidSave );

    AssertDIRNoLatch( ppib );
    return err;
}


ERR ErrRECSetDefaultValue( FUCB *pfucbFake, const COLUMNID columnid, VOID *pvDefault, ULONG cbDefault )
{
    ERR         err;
    DATA        dataField;
    TDB         *ptdb       = pfucbFake->u.pfcb->Ptdb();
    const FIELD *pfield     = ptdb->Pfield( columnid );

    Assert( pfucbFake->u.pfcb->FTypeTable() );
    Assert( pfucbFake->u.pfcb->FFixedDDL() );

    dataField.SetPv( pvDefault );
    dataField.SetCb( cbDefault );

    if ( FRECLongValue( pfield->coltyp ) )
    {
        Assert( FCOLUMNIDTagged( columnid ) );
        Assert( FidOfColumnid( columnid ) <= ptdb->FidTaggedLast() );

        Assert( JET_cbLVDefaultValueMost < cbLVIntrinsicMost );

        if ( cbDefault > JET_cbLVDefaultValueMost )
        {
            err = ErrERRCheck( JET_errDefaultValueTooBig );
        }
        else
        {
            DATA dataNullT;
            dataNullT.Nullify();

            err = ErrRECAOIntrinsicLV(
                        pfucbFake,
                        columnid,
                        NO_ITAG,
                        &dataNullT,
                        &dataField,
                        0,
                        pfield->cbMaxLen,
                        lvopInsert,
                        compressNone,
                        fFalse );
        }
    }
    else
    {
        err = ErrRECSetColumn( pfucbFake, columnid, NO_ITAG, &dataField );
    }

    if ( JET_errColumnTooBig == err )
    {
        err = ErrERRCheck( JET_errDefaultValueTooBig );
    }

    return err;
}



LOCAL ERR ErrRECISetIFixedColumn(
    FUCB            * const pfucb,
    DATA            * const pdataWorkBuf,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    const FID       fid         = FidOfColumnid( columnid );

    Assert( FFixedFid( fid ) );

    Assert( ( ( pfucb != pfucbNil ) && ( &( pfucb->dataWorkBuf ) == pdataWorkBuf ) ) ||
            ( ( pfucb == pfucbNil ) && ( pdataWorkBuf != NULL ) ) );
    Assert( ( pfucb == pfucbNil ) || FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

    INT         cbRec = pdataWorkBuf->Cb();
    Assert( cbRec >= REC::cbRecordMin );
    Assert( cbRec <= REC::CbRecordMostWithGlobalPageSize() );
    if ( pfucb && cbRec > REC::CbRecordMost( pfucb ) )
    {
        FireWall( "SetFixedColumRecTooBig3.2" );
    }
    if ( cbRec < REC::cbRecordMin || cbRec > REC::CbRecordMostWithGlobalPageSize() )
    {
        FireWall( "SetFixedColumRecTooBig3.1" );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    BYTE        *pbRec = (BYTE *)pdataWorkBuf->Pv();
    REC         *prec = (REC *)pbRec;
    Assert( precNil != prec );

    if ( fid > ptdb->FidFixedLast() || fid < ptdb->FidFixedFirst() )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    FIELD       *pfield = ptdb->PfieldFixed( columnid );
    Assert( pfieldNil != pfield );

    if ( JET_coltypNil == pfield->coltyp )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    else if ( ( NULL == pdataField || pdataField->FNull() )
        && FFIELDNotNull( pfield->ffield ) )
    {
        return ErrERRCheck( JET_errNullInvalid );
    }

    if ( pfucb != pfucbNil )
    {
        FUCBSetColumnSet( pfucb, fid );
    }

    if ( fid > prec->FidFixedLastInRec() )
    {
        Assert( pfucb != pfucbNil );

        const FID   fidFixedLastInRec       = prec->FidFixedLastInRec();
        FID         fidLastDefaultToBurst;
        FID         fidT;

        Assert( fidFixedLastInRec < ptdb->FidFixedLast() );

        const WORD  ibOldFixEnd     = WORD( prec->PbFixedNullBitMap() - pbRec );
        const WORD  ibOldBitMapEnd  = prec->IbEndOfFixedData();
        const INT   cbOldBitMap     = ibOldBitMapEnd - ibOldFixEnd;
        Assert( ibOldBitMapEnd >= ibOldFixEnd );
        Assert( ibOldFixEnd == ptdb->IbOffsetOfNextColumn( fidFixedLastInRec ) );
        Assert( ibOldBitMapEnd == ibOldFixEnd + cbOldBitMap );
        Assert( (ULONG)cbOldBitMap == prec->CbFixedNullBitMap() );

        const WORD  ibNewFixEnd     = WORD( pfield->ibRecordOffset + pfield->cbMaxLen );
        const INT   cbNewBitMap     = ( ( fid - fidFixedLeast + 1 ) + 7 ) / 8;
        const WORD  ibNewBitMapEnd  = WORD( ibNewFixEnd + cbNewBitMap );
        const INT   cbShift         = ibNewBitMapEnd - ibOldBitMapEnd;
        Assert( ibNewFixEnd == ptdb->IbOffsetOfNextColumn( fid ) );
        Assert( ibNewFixEnd > ibOldFixEnd );
        Assert( cbNewBitMap >= cbOldBitMap );
        Assert( ibNewBitMapEnd > ibOldBitMapEnd );
        Assert( cbShift > 0 );

        if ( cbRec + cbShift > REC::CbRecordMost( pfucb ) )
            return ErrERRCheck( JET_errRecordTooBig );

        Assert( cbRec >= ibOldBitMapEnd );
        memmove(
            pbRec + ibNewBitMapEnd,
            pbRec + ibOldBitMapEnd,
            cbRec - ibOldBitMapEnd );

        memset( pbRec + ibOldBitMapEnd, chRECFill, cbShift );

        pdataWorkBuf->DeltaCb( cbShift );
        cbRec = pdataWorkBuf->Cb();

        prec->SetIbEndOfFixedData( ibNewBitMapEnd );

        memmove(
            pbRec + ibNewFixEnd,
            pbRec + ibOldFixEnd,
            cbOldBitMap );

        memset( pbRec + ibOldFixEnd, chRECFill, ibNewFixEnd - ibOldFixEnd );


        BYTE    *prgbitNullity;
        if ( fidFixedLastInRec >= fidFixedLeast )
        {
            UINT    ifid    = fidFixedLastInRec - fidFixedLeast;

            prgbitNullity = pbRec + ibNewFixEnd + ifid/8;

            for ( fidT = FID( fidFixedLastInRec + 1 ); fidT <= fid; fidT++ )
            {
                ifid = fidT - fidFixedLeast;
                if ( ( pbRec + ibNewFixEnd + ifid/8 ) != prgbitNullity )
                {
                    Assert( ( pbRec + ibNewFixEnd + ifid/8 ) == ( prgbitNullity + 1 ) );
                    break;
                }
                SetFixedNullBit( prgbitNullity, ifid );
            }

            prgbitNullity++;
            Assert( prgbitNullity <= pbRec + ibNewBitMapEnd );
        }
        else
        {
            prgbitNullity = pbRec + ibNewFixEnd;
            Assert( prgbitNullity < pbRec + ibNewBitMapEnd );
        }

        memset( prgbitNullity, 0xff, pbRec + ibNewBitMapEnd - prgbitNullity );


        const REC * const   precDefault = ( NULL != ptdb->PdataDefaultRecord() ?
                                                    (REC *)ptdb->PdataDefaultRecord()->Pv() :
                                                    NULL );
        Assert( NULL == precDefault ||
            ( ptdb->PdataDefaultRecord()->Cb() >= REC::cbRecordMin
            && ptdb->PdataDefaultRecord()->Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) ) );
        if ( NULL != ptdb->PdataDefaultRecord() &&
            ptdb->PdataDefaultRecord()->Cb() > REC::CbRecordMost( pfucb ) )
        {
            FireWall( "TemplateDefaultRecTooBig4.2" );
        }
        if ( NULL != ptdb->PdataDefaultRecord() &&
            ( ptdb->PdataDefaultRecord()->Cb() < REC::cbRecordMin
            || ptdb->PdataDefaultRecord()->Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) ) )
        {
            FireWall( "TemplateDefaultRecTooBig4.1" );
            return ErrERRCheck( JET_errDatabaseCorrupted );
        }

        fidLastDefaultToBurst = ( NULL != precDefault ?
                                        precDefault->FidFixedLastInRec() :
                                        FID( fidFixedLeast - 1 ) );
        Assert( fidLastDefaultToBurst >= fidFixedLeast-1 );
        Assert( fidLastDefaultToBurst <= ptdb->FidFixedLast() );

        if ( fidLastDefaultToBurst > fidFixedLastInRec )
        {
            const FID   fidLastDefaultToCheck   = min( fid, fidLastDefaultToBurst );

            Assert( !ptdb->FInitialisingDefaultRecord() );
            Assert( fidFixedLastInRec < fid );
            for ( fidT = FID( fidFixedLastInRec + 1 ); fidT <= fidLastDefaultToCheck; fidT++ )
            {
                const BOOL  fTemplateColumn = ptdb->FFixedTemplateColumn( fidT );
                const FIELD *pfieldFixed    = ptdb->PfieldFixed( ColumnidOfFid( fidT, fTemplateColumn ) );

                Assert( fidT <= ptdb->FidFixedLast() );

                if ( FFIELDDefault( pfieldFixed->ffield )
                    && !FFIELDCommittedDelete( pfieldFixed->ffield ) )
                {
                    UINT    ifid    = fidT - fidFixedLeast;

                    Assert( pfieldFixed->coltyp != JET_coltypNil );

                    prgbitNullity = pbRec + ibNewFixEnd + (ifid/8);
                    Assert( FFixedNullBit( prgbitNullity, ifid ) );
                    ResetFixedNullBit( prgbitNullity, ifid );
                    fidLastDefaultToBurst = fidT;
                }
            }

            Assert( fidLastDefaultToBurst > fidFixedLastInRec );
            if ( fidLastDefaultToBurst <= fid )
            {
                const WORD  ibLastDefaultToBurst    = ptdb->IbOffsetOfNextColumn( fidLastDefaultToBurst );
                Assert( ibLastDefaultToBurst > ibOldFixEnd );
                UtilMemCpy(
                    pbRec + ibOldFixEnd,
                    (BYTE *)precDefault + ibOldFixEnd,
                    ibLastDefaultToBurst - ibOldFixEnd );
            }
        }

        prec->SetFidFixedLastInRec( fid );
    }


    const UINT  ifid            = fid - fidFixedLeast;

    BYTE        *prgbitNullity  = prec->PbFixedNullBitMap() + ifid/8;

    if ( NULL == pdataField || pdataField->FNull() )
    {
        Assert( !FFIELDNotNull( pfield->ffield ) );
        SetFixedNullBit( prgbitNullity, ifid );
    }
    else
    {
        const JET_COLTYP    coltyp = pfield->coltyp;
        ULONG               cbCopy = pfield->cbMaxLen;

        Assert( pfield->cbMaxLen == UlCATColumnSize( coltyp, cbCopy, NULL ) );

        if ( (ULONG)pdataField->Cb() != cbCopy )
        {
            switch ( coltyp )
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
                    return ErrERRCheck( JET_errInvalidBufferSize );

                case JET_coltypBinary:
                    if ( (ULONG)pdataField->Cb() > cbCopy )
                        return ErrERRCheck( JET_errInvalidBufferSize );
                    else
                    {
                        memset(
                            pbRec + pfield->ibRecordOffset + pdataField->Cb(),
                            0,
                            cbCopy - pdataField->Cb() );
                    }
                    cbCopy = pdataField->Cb();
                    break;

                default:
                    Assert( JET_coltypText == coltyp );
                    if ( (ULONG)pdataField->Cb() > cbCopy )
                        return ErrERRCheck( JET_errInvalidBufferSize );
                    else
                    {
                        RECTextColumnPadding( pbRec + pfield->ibRecordOffset, pfield->cbMaxLen,  pdataField->Cb(), pfield->cp );
                    }
                    cbCopy = pdataField->Cb();
                    break;
            }
        }

        ResetFixedNullBit( prgbitNullity, ifid );

        if ( JET_coltypBit == coltyp )
        {
            if ( *( (BYTE *)pdataField->Pv() ) == 0 )
                *( pbRec + pfield->ibRecordOffset ) = 0x00;
            else
                *( pbRec + pfield->ibRecordOffset ) = 0xFF;
        }
        else
        {
            BYTE    rgb[8];
            BYTE    *pbDataField;

            SetPbDataField( &pbDataField, pdataField, rgb, coltyp );

            Assert( pbDataField != pdataField->Pv() || cbCopy == (ULONG)pdataField->Cb() );

            UtilMemCpy( pbRec + pfield->ibRecordOffset, pbDataField, cbCopy );
        }
    }

    Assert( ( pfucb == pfucbNil ) || ( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMost( pfucb ) ) );
    return JET_errSuccess;
}


ERR ErrRECISetFixedColumn(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    return ErrRECISetIFixedColumn(
            pfucb,
            &( pfucb->dataWorkBuf ),
            ptdb,
            columnid,
            pdataField );
}


ERR ErrRECISetFixedColumnInLoadedDataBuffer(
    DATA            * const pdataWorkBuf,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    return ErrRECISetIFixedColumn(
            pfucbNil,
            pdataWorkBuf,
            ptdb,
            columnid,
            pdataField );
}


INLINE ULONG CbBurstVarDefaults( TDB *ptdb, FUCB *pfucb, FID fidVarLastInRec, FID fidSet, FID *pfidLastDefault )
{
    ULONG               cbBurstDefaults     = 0;
    const REC * const   precDefault         = ( NULL != ptdb->PdataDefaultRecord() ?
                                                        (REC *)ptdb->PdataDefaultRecord()->Pv() :
                                                        NULL );

    Assert( NULL == precDefault ||
        ( ptdb->PdataDefaultRecord()->Cb() >= REC::cbRecordMin
        && ptdb->PdataDefaultRecord()->Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) ) );
    if ( NULL != ptdb->PdataDefaultRecord() &&
         ptdb->PdataDefaultRecord()->Cb() > REC::CbRecordMost( pfucb ) )
    {
        FireWall( "BurstVarDefaultsRecTooBig6.2" );
    }
    if ( NULL != ptdb->PdataDefaultRecord() &&
        ( ptdb->PdataDefaultRecord()->Cb() < REC::cbRecordMin
        || ptdb->PdataDefaultRecord()->Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) ) )
    {
        FireWall( "BurstVarDefaultsRecTooBig6.1" );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    *pfidLastDefault = ( NULL != precDefault ?
                            precDefault->FidVarLastInRec() :
                            FID( fidVarLeast - 1 ) );
    Assert( *pfidLastDefault >= fidVarLeast-1 );
    Assert( *pfidLastDefault <= ptdb->FidVarLast() );

    if ( *pfidLastDefault > fidVarLastInRec )
    {
        Assert( ptdb->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );
        Assert( fidVarLastInRec < fidSet );
        Assert( fidSet <= ptdb->FidVarLast() );

        const UnalignedLittleEndian<REC::VAROFFSET> *pibDefaultVarOffs = precDefault->PibVarOffsets();

        for ( FID fidT = FID( fidVarLastInRec + 1 ); fidT < fidSet; fidT++ )
        {
            const BOOL  fTemplateColumn = ptdb->FVarTemplateColumn( fidT );
            const FIELD *pfieldVar      = ptdb->PfieldVar( ColumnidOfFid( fidT, fTemplateColumn ) );
            if ( FFIELDDefault( pfieldVar->ffield )
                && !FFIELDCommittedDelete( pfieldVar->ffield ) )
            {
                const UINT  ifid = fidT - fidVarLeast;
                Assert( pfieldVar->coltyp != JET_coltypNil );

                Assert( !FVarNullBit( pibDefaultVarOffs[ifid] ) );

                const WORD  ibVarOffset = ( fidVarLeast == fidT ?
                                                (WORD)0 :
                                                IbVarOffset( pibDefaultVarOffs[ifid-1] ) );

                Assert( IbVarOffset( pibDefaultVarOffs[ifid] ) > ibVarOffset );
                Assert( precDefault->PbVarData() + IbVarOffset( pibDefaultVarOffs[ifid] )
                            <= (BYTE *)ptdb->PdataDefaultRecord()->Pv() + ptdb->PdataDefaultRecord()->Cb() );

                cbBurstDefaults += ( IbVarOffset( pibDefaultVarOffs[ifid] ) - ibVarOffset );
                Assert( cbBurstDefaults > 0 );

                *pfidLastDefault = fidT;
            }
        }
    }

    Assert( cbBurstDefaults == 0  ||
        ( *pfidLastDefault > fidVarLastInRec && *pfidLastDefault < fidSet ) );

    return cbBurstDefaults;
}


ERR ErrRECISetVarColumn(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    ERR             err             = JET_errSuccess;
    const FID       fid             = FidOfColumnid( columnid );

    Assert( FVarFid( fid ) );

    Assert( pfucbNil != pfucb );
    Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

    INT         cbRec = pfucb->dataWorkBuf.Cb();
    Assert( cbRec >= REC::cbRecordMin );
    Assert( cbRec <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( cbRec <= REC::CbRecordMost( pfucb ) );
    if ( cbRec > REC::CbRecordMost( pfucb ) )
    {
        FireWall( "SetVarColumnRecTooBig7.2" );
    }
    if ( cbRec < REC::cbRecordMin || cbRec > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) )
    {
        FireWall( "SetVarColumnRecTooBig7.1" );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    BYTE        *pbRec = (BYTE *)pfucb->dataWorkBuf.Pv();
    REC         *prec = (REC *)pbRec;
    Assert( precNil != prec );

    if ( fid > ptdb->FidVarLast() || fid < ptdb->FidVarFirst() )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    FIELD       *pfield = ptdb->PfieldVar( columnid );
    Assert( pfieldNil != pfield );
    if ( JET_coltypNil == pfield->coltyp )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    FUCBSetColumnSet( pfucb, fid );

    INT     cbCopy;
    if ( NULL == pdataField )
    {
        if ( FFIELDNotNull( pfield->ffield ) )
            return ErrERRCheck( JET_errNullInvalid );
        else
            cbCopy = 0;
    }
    else if ( NULL == pdataField->Pv() )
    {
        cbCopy = 0;
    }
    else if ( (ULONG)pdataField->Cb() > pfield->cbMaxLen )
    {
        cbCopy = pfield->cbMaxLen;
        err = ErrERRCheck( JET_wrnColumnMaxTruncated );
    }
    else
    {
        cbCopy = pdataField->Cb();
    }
    Assert( cbCopy >= 0 );

    UnalignedLittleEndian<REC::VAROFFSET>   *pib;
    UnalignedLittleEndian<REC::VAROFFSET>   *pibLast;
    UnalignedLittleEndian<REC::VAROFFSET>   *pibVarOffs;

    const BOOL      fBurstRecord = ( fid > prec->FidVarLastInRec() );
    if ( fBurstRecord )
    {
        const FID   fidVarLastInRec = prec->FidVarLastInRec();
        FID         fidLastDefault;

        Assert( !( pdataField == NULL && FFIELDNotNull( pfield->ffield ) ) );

        const INT   cbNeed = ( fid - fidVarLastInRec ) * sizeof(REC::VAROFFSET);
        const INT   cbBurstDefaults = CbBurstVarDefaults(
                                            ptdb,
                                            pfucb,
                                            fidVarLastInRec,
                                            fid,
                                            &fidLastDefault );

        if ( cbRec + cbNeed + cbBurstDefaults + cbCopy > REC::CbRecordMost( pfucb ) )
            return ErrERRCheck( JET_errRecordTooBig );

        pibVarOffs = prec->PibVarOffsets();

        BYTE    *pbVarOffsEnd = prec->PbVarData();
        Assert( pbVarOffsEnd >= (BYTE *)pibVarOffs );
        memmove(
                pbVarOffsEnd + cbNeed,
                pbVarOffsEnd,
                pbRec + cbRec - pbVarOffsEnd );

        memset( pbVarOffsEnd, chRECFill, cbNeed );


        pib = (UnalignedLittleEndian<REC::VAROFFSET> *)pbVarOffsEnd;
        pibLast = pibVarOffs + ( fid - fidVarLeast );

        WORD            ibTagFields = prec->IbEndOfVarData();
        SetVarNullBit( ibTagFields );

        Assert( pib == pibVarOffs + ( fidVarLastInRec+1-fidVarLeast ) );
        Assert( prec->PbVarData() + IbVarOffset( ibTagFields ) - pbRec <= cbRec );
        Assert( pib <= pibLast );
        Assert( SIZE_T( pibLast - pib + 1 ) == cbNeed / sizeof(REC::VAROFFSET) );
        while( pib <= pibLast )
            *pib++ = ibTagFields;
        Assert( pib == pibVarOffs + ( fid+1-fidVarLeast ) );

        Assert( prec->FidVarLastInRec() == fidVarLastInRec );
        prec->SetFidVarLastInRec( fid );
        Assert( pfucb->dataWorkBuf.Cb() == cbRec );
        cbRec += cbNeed;

        Assert( prec->PibVarOffsets() == pibVarOffs );
        Assert( pibVarOffs[fid-fidVarLeast] == ibTagFields );

        Assert( cbBurstDefaults == 0
            || ( fidLastDefault > fidVarLastInRec && fidLastDefault < fid ) );
        if ( cbBurstDefaults > 0 )
        {
            Assert( NULL != ptdb->PdataDefaultRecord() );

            BYTE            *pbVarData          = prec->PbVarData();
            const REC       *precDefault        = (REC *)ptdb->PdataDefaultRecord()->Pv();
            const BYTE      *pbVarDataDefault   = precDefault->PbVarData();
            UnalignedLittleEndian<REC::VAROFFSET>   *pibDefaultVarOffs;
            UnalignedLittleEndian<REC::VAROFFSET>   *pibDefault;

            Assert( pbVarData > pbVarOffsEnd );
            Assert( pbVarData > pbRec );
            Assert( pbVarData <= pbRec + cbRec );

            pibDefaultVarOffs = precDefault->PibVarOffsets();

            Assert( FVarNullBit( ibTagFields ) );
            Assert( cbRec >= pbVarData + IbVarOffset( ibTagFields ) - pbRec );
            const ULONG     cbTaggedData = cbRec - ULONG( pbVarData + IbVarOffset( ibTagFields ) - pbRec );
            memmove(
                pbVarData + IbVarOffset( ibTagFields ) + cbBurstDefaults,
                pbVarData + IbVarOffset( ibTagFields ),
                cbTaggedData );

            Assert( fidVarLastInRec < fidLastDefault );

            Assert( fidVarLastInRec == fidVarLeast-1
                || IbVarOffset( pibDefaultVarOffs[fidVarLastInRec-fidVarLeast] )
                        < IbVarOffset( pibDefaultVarOffs[fidLastDefault-fidVarLeast] ) );
            Assert( pbVarDataDefault + IbVarOffset( pibDefaultVarOffs[fidLastDefault-fidVarLeast] ) - (BYTE *)precDefault
                    <= ptdb->PdataDefaultRecord()->Cb() );

            pib = pibVarOffs + ( fidVarLastInRec + 1 - fidVarLeast );
            Assert( *pib == ibTagFields );
            pibDefault = pibDefaultVarOffs + ( fidVarLastInRec + 1 - fidVarLeast );

#ifdef DEBUG
            ULONG   cbBurstSoFar        = 0;
#endif

            for ( FID fidT = FID( fidVarLastInRec + 1 );
                fidT <= fidLastDefault;
                fidT++, pib++, pibDefault++ )
            {
                const BOOL  fTemplateColumn = ptdb->FVarTemplateColumn( fidT );
                const FIELD *pfieldVar      = ptdb->PfieldVar( ColumnidOfFid( fidT, fTemplateColumn ) );

                Assert( FVarNullBit( *pib ) );

                if ( FFIELDDefault( pfieldVar->ffield )
                    && !FFIELDCommittedDelete( pfieldVar->ffield ) )
                {
                    Assert( JET_coltypNil != pfieldVar->coltyp );

                    Assert( !FVarNullBit( *pibDefault ) );

                    if ( fidVarLeast == fidT )
                    {
                        Assert( IbVarOffset( *pibDefault ) > 0 );
                        const USHORT    cb  = IbVarOffset( *pibDefault );
                        *pib = cb;
                        UtilMemCpy( pbVarData, pbVarDataDefault, cb );
#ifdef DEBUG
                        cbBurstSoFar += cb;
#endif
                    }
                    else
                    {
                        Assert( IbVarOffset( *pibDefault ) > IbVarOffset( *(pibDefault-1) ) );
                        const USHORT    cb  = USHORT( IbVarOffset( *pibDefault )
                                                - IbVarOffset( *(pibDefault-1) ) );
                        const USHORT    ib  = USHORT( IbVarOffset( *(pib-1) ) + cb );
                        *pib = ib;
                        UtilMemCpy( pbVarData + IbVarOffset( *(pib-1) ),
                                pbVarDataDefault + IbVarOffset( *(pibDefault-1) ),
                                cb );
#ifdef DEBUG
                        cbBurstSoFar += cb;
#endif
                    }

                    Assert( !FVarNullBit( *pib ) );
                }
                else if ( fidT > fidVarLeast )
                {
                    *pib = IbVarOffset( *(pib-1) );

                    Assert( !FVarNullBit( *pib ) );
                    SetVarNullBit( *(UnalignedLittleEndian< WORD >*)(pib) );
                }
                else
                {
                    Assert( fidT == fidVarLeast );
                    Assert( FVarNullBit( *pib ) );
                    Assert( 0 == IbVarOffset( *pib ) );
                }
            }

            Assert( (ULONG)cbBurstDefaults == cbBurstSoFar );
            Assert( FVarNullBit( *pib ) );
            Assert( *pib == ibTagFields );
            if ( fidVarLastInRec >= fidVarLeast )
            {
                Assert( IbVarOffset( pibVarOffs[fidVarLastInRec-fidVarLeast] )
                    == IbVarOffset( ibTagFields ) );
                Assert( IbVarOffset( *(pib-1) ) -
                        IbVarOffset( pibVarOffs[fidVarLastInRec-fidVarLeast] )
                    == (WORD)cbBurstDefaults );
            }
            else
            {
                Assert( IbVarOffset( *(pib-1) ) == (WORD)cbBurstDefaults );
            }

            pibLast = pibVarOffs + ( fid - fidVarLeast );
            for ( ; pib <= pibLast; pib++ )
            {
                Assert( FVarNullBit( *pib ) );
                Assert( *pib == ibTagFields );

                *pib = REC::VAROFFSET( *pib + cbBurstDefaults );
            }

#ifdef DEBUG
            pibLast = pibVarOffs + ( fid - fidVarLeast );
            for ( pib = pibVarOffs+1; pib <= pibLast; pib++ )
            {
                Assert( IbVarOffset( *pib ) >= IbVarOffset( *(pib-1) ) );
                if ( FVarNullBit( *pib ) )
                {
                    Assert( pib != pibVarOffs + ( fidLastDefault - fidVarLeast ) );
                    Assert( IbVarOffset( *pib ) == IbVarOffset( *(pib-1) ) );
                }
                else
                {
                    Assert( pib <= pibVarOffs + ( fidLastDefault - fidVarLeast ) );
                    Assert( IbVarOffset( *pib ) > IbVarOffset( *(pib-1) ) );
                }
            }
#endif
        }

        Assert( pfucb->dataWorkBuf.Cb() == cbRec - cbNeed );
        cbRec += cbBurstDefaults;
        pfucb->dataWorkBuf.SetCb( cbRec );

        Assert( prec->FidVarLastInRec() == fid );
    }


    Assert( JET_errSuccess == err || JET_wrnColumnMaxTruncated == err );


    pibVarOffs = prec->PibVarOffsets();
    pib = pibVarOffs + ( fid - fidVarLeast );

    REC::VAROFFSET  ibStartOfColumn;
    if( fidVarLeast == fid )
    {
        ibStartOfColumn = 0;
    }
    else
    {
        ibStartOfColumn = IbVarOffset( *(pib-1) );
    }

    const REC::VAROFFSET    ibEndOfColumn   = IbVarOffset( *pib );
    const INT               dbFieldData     = cbCopy - ( ibEndOfColumn - ibStartOfColumn );

    if ( 0 != dbFieldData )
    {
        BYTE    *pbVarData = prec->PbVarData();

        if ( cbRec + dbFieldData > REC::CbRecordMost( pfucb ) )
        {
            Assert( !fBurstRecord );
            return ErrERRCheck( JET_errRecordTooBig );
        }

        Assert( cbRec >= pbVarData + ibEndOfColumn - pbRec );
        memmove(
            pbVarData + ibEndOfColumn + dbFieldData,
            pbVarData + ibEndOfColumn,
            cbRec - ( pbVarData + ibEndOfColumn - pbRec ) );

#ifdef DEBUG
        if ( dbFieldData > 0 )
            memset( pbVarData + ibEndOfColumn, chRECFill, dbFieldData );
#endif

        pfucb->dataWorkBuf.DeltaCb( dbFieldData );
        cbRec = pfucb->dataWorkBuf.Cb();

        Assert( fid <= prec->FidVarLastInRec() );
        Assert( prec->FidVarLastInRec() >= fidVarLeast );
        pibLast = pibVarOffs + ( prec->FidVarLastInRec() - fidVarLeast );
        for ( pib = pibVarOffs + ( fid - fidVarLeast ); pib <= pibLast; pib++ )
        {
            *pib = WORD( *pib + dbFieldData );
        }

        pib = pibVarOffs + ( fid - fidVarLeast );
    }

    Assert( cbCopy >= 0 );
    if ( cbCopy > 0 )
    {
        UtilMemCpy(
            prec->PbVarData() + ibStartOfColumn,
            pdataField->Pv(),
            cbCopy );
    }

    if ( NULL == pdataField )
    {
        SetVarNullBit( *( UnalignedLittleEndian< WORD >*)pib );
    }
    else
    {
        ResetVarNullBit( *( UnalignedLittleEndian< WORD >*)pib );
    }

    Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMost( pfucb ) );
    Assert( JET_errSuccess == err || JET_wrnColumnMaxTruncated == err );
    return err;
}


ERR ErrRECISetTaggedColumn(
    FUCB            * const pfucb,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const DATA      * const pdataToSet,
    const BOOL      fUseDerivedBit,
    const JET_GRBIT grbit )
{
    const FID       fid                 = FidOfColumnid( columnid );
    Assert( FCOLUMNIDTagged( columnid ) );

    Assert( pfucbNil != pfucb );
    Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

    if ( fid > ptdb->FidTaggedLast() || fid < ptdb->FidTaggedFirst() )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    const FIELD     *const pfield       = ptdb->PfieldTagged( columnid );
    Assert( pfieldNil != pfield );
    if ( JET_coltypNil == pfield->coltyp )
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }
    else if ( NULL == pdataToSet && FFIELDNotNull( pfield->ffield ) )
    {
        return ErrERRCheck( JET_errNullInvalid );
    }

    FUCBSetColumnSet( pfucb, fid );

    const ULONG cbMaxLenPhysical = ( grbit & grbitSetColumnEncrypted ) ? CbOSEncryptAes256SizeNeeded( pfield->cbMaxLen ) : pfield->cbMaxLen;
    if ( pfield->cbMaxLen > 0
        && NULL != pdataToSet
        && (ULONG)pdataToSet->Cb() > cbMaxLenPhysical )
    {
        Assert( !( grbit & grbitSetColumnEncrypted ) );
        return ErrERRCheck( JET_errColumnTooBig );
    }

    if ( NULL != pdataToSet
        && pdataToSet->Cb() > 0 )
    {
        switch ( pfield->coltyp )
        {
            case JET_coltypBit:
            case JET_coltypUnsignedByte:
                if ( pdataToSet->Cb() != 1 )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
                break;
            case JET_coltypShort:
            case JET_coltypUnsignedShort:
                if ( pdataToSet->Cb() != 2 )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
                break;
            case JET_coltypLong:
            case JET_coltypUnsignedLong:
            case JET_coltypIEEESingle:
                if ( pdataToSet->Cb() != 4 )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
                break;
            case JET_coltypLongLong:
            case JET_coltypUnsignedLongLong:
            case JET_coltypCurrency:
            case JET_coltypIEEEDouble:
            case JET_coltypDateTime:
                if ( pdataToSet->Cb() != 8 )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
                break;
            case JET_coltypGUID:
                if ( pdataToSet->Cb() != 16 )
                {
                    return ErrERRCheck( JET_errInvalidBufferSize );
                }
                break;
            default:
                Assert( JET_coltypText == pfield->coltyp
                    || JET_coltypBinary == pfield->coltyp
                    || FRECLongValue( pfield->coltyp ) );
                break;
        }
    }

    if ( NULL != pdataToSet
        && (ULONG) pdataToSet->Cb() > CbLVIntrinsicTableMost( pfucb ) )
    {
        return ErrERRCheck( JET_errColumnDoesNotFit );
    }

#ifdef DEBUG
    VOID            * pvDBGCopyOfRecord;
    const ULONG     cbDBGCopyOfRecord       = pfucb->dataWorkBuf.Cb();
    BFAlloc( bfasTemporary, &pvDBGCopyOfRecord );
    UtilMemCpy( pvDBGCopyOfRecord, pfucb->dataWorkBuf.Pv(), cbDBGCopyOfRecord );
#endif

    TAGFIELDS   tagfields( pfucb->dataWorkBuf );
    const ERR   errT        = tagfields.ErrSetColumn(
                                    pfucb,
                                    pfield,
                                    columnid,
                                    itagSequence,
                                    pdataToSet,
                                    grbit | ( fUseDerivedBit ? grbitSetColumnUseDerivedBit : 0 ) );

#ifdef DEBUG
    BFFree( pvDBGCopyOfRecord );
#endif

    return errT;
}


ERR VTAPI ErrIsamSetColumnDefaultValue(
    JET_SESID   vsesid,
    JET_DBID    vdbid,
    const CHAR  *szTableName,
    const CHAR  *szColumnName,
    const VOID  *pvData,
    const ULONG cbData,
    const ULONG grbit )
{
    ERR         err;
    PIB         *ppib               = (PIB *)vsesid;
    IFMP        ifmp                = (IFMP)vdbid;
    FUCB        *pfucb              = pfucbNil;
    FCB         *pfcb               = pfcbNil;
    TDB         *ptdb               = ptdbNil;
    BOOL        fInTrx              = fFalse;
    BOOL        fNeedToSetFlag      = fFalse;
    BOOL        fResetFlagOnErr     = fFalse;
    BOOL        fRestorePrevOnErr   = fFalse;
    DATA        dataDefault;
    DATA *      pdataDefaultPrev;
    OBJID       objidTable;
    FIELD       *pfield;
    COLUMNID    columnid            = JET_columnidNil;
    CHAR        szColumn[JET_cbNameMost+1];

    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

    if ( FFMPIsTempDB( ifmp ) )
    {
        Expected( fFalse );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( ErrUTILCheckName( szColumn, szColumnName, JET_cbNameMost+1 ) );

    if ( NULL == pvData || 0 == cbData )
    {
        return ErrERRCheck( JET_errNullInvalid );
    }

    Assert( cbData <= (ULONG)REC::CbRecordMostCHECK( g_rgfmp[ ifmp ].CbPage() ) );
    if ( cbData > (ULONG)REC::CbRecordMostCHECK( g_rgfmp[ ifmp ].CbPage() ) )
    {
        FireWall( "IsamSetColumnDefValRecTooBig5.1" );
        return ErrERRCheck( JET_errDefaultValueTooBig );
    }

    CallR( ErrFILEOpenTable(
                ppib,
                ifmp,
                &pfucb,
                szTableName,
                JET_bitTableDenyRead ) );
    CallSx( err, JET_wrnTableInUseBySystem );
    Assert( pfucbNil != pfucb );

    if ( cbData > (ULONG)REC::CbRecordMost( pfucb ) )
    {
        FireWall( "IsamSetColumnDefValRecTooBig5.2" );
    }

    pfcb = pfucb->u.pfcb;
    objidTable = pfcb->ObjidFDP();
    Assert( pfcbNil != pfcb );

    ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );

    Assert( NULL != ptdb->PdataDefaultRecord() );
    pdataDefaultPrev = ptdb->PdataDefaultRecord();
    Assert( NULL != pdataDefaultPrev );
    Assert( !pdataDefaultPrev->FNull() );

    Assert( cbData > 0 );
    Assert( pvData != NULL );
    dataDefault.SetCb( cbData );
    dataDefault.SetPv( (VOID *)pvData );

    FUCB    fucbFake( ppib, pfcb->Ifmp() );
    FCB     fcbFake( pfcb->Ifmp(), pfcb->PgnoFDP() );
    FILEPrepareDefaultRecord( &fucbFake, &fcbFake, ptdb );
    Assert( fucbFake.pvWorkBuf != NULL );

    Call( ErrDIRBeginTransaction( ppib, 41253, NO_GRBIT ) );
    fInTrx = fTrue;



    pfcb->EnterDML();
    err = ErrFILEPfieldFromColumnName(
            ppib,
            pfcb,
            szColumn,
            &pfield,
            &columnid );
    if ( err >= 0 )
    {
        if ( pfieldNil == pfield )
        {
            err = ErrERRCheck( JET_errColumnNotFound );
        }
        else if ( FFILEIsIndexColumn( ppib, pfcb, columnid ) )
        {
            err = ErrERRCheck( JET_errColumnInUse );
        }
        else
        {
            fNeedToSetFlag = !FFIELDDefault( pfield->ffield );
        }
        pfcb->LeaveDML();
    }

    Call( err );

    Call( ErrCATChangeColumnDefaultValue(
                ppib,
                ifmp,
                objidTable,
                szColumn,
                dataDefault ) )

    Assert( fucbFake.pvWorkBuf != NULL );
    Assert( fucbFake.u.pfcb == &fcbFake );
    Assert( fcbFake.Ptdb() == ptdb );

    if ( fNeedToSetFlag )
    {
        pfcb->EnterDDL();
        FIELDSetDefault( ptdb->Pfield( columnid )->ffield );
        if ( !FFIELDEscrowUpdate( ptdb->Pfield( columnid )->ffield ) )
        {
            ptdb->SetFTableHasNonEscrowDefault();
        }
        ptdb->SetFTableHasDefault();
        pfcb->LeaveDDL();

        fResetFlagOnErr = fTrue;
    }

    Call( ErrFILERebuildDefaultRec( &fucbFake, columnid, &dataDefault ) );
    Assert( NULL != ptdb->PdataDefaultRecord() );
    Assert( !ptdb->PdataDefaultRecord()->FNull() );
    Assert( ptdb->PdataDefaultRecord() != pdataDefaultPrev );
    fRestorePrevOnErr = fTrue;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    fInTrx = fFalse;

    Assert( NULL != pdataDefaultPrev );
    OSMemoryHeapFree( pdataDefaultPrev );

HandleError:
    if ( err < 0 )
    {
        Assert( ptdbNil != ptdb );

        if ( fResetFlagOnErr )
        {
            Assert( fNeedToSetFlag );
            pfcb->EnterDDL();
            FIELDResetDefault( ptdb->Pfield( columnid )->ffield );
            pfcb->LeaveDDL();
        }

        if ( fRestorePrevOnErr )
        {
            Assert( NULL != ptdb->PdataDefaultRecord() );
            OSMemoryHeapFree( ptdb->PdataDefaultRecord() );
            ptdb->SetPdataDefaultRecord( pdataDefaultPrev );
        }

        if ( fInTrx )
        {
            CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
        }
    }
    else
    {
        Assert( !fInTrx );
    }

    FILEFreeDefaultRecord( &fucbFake );

    Assert( pfucbNil != pfucb );
    DIRClose( pfucb );

    AssertDIRNoLatch( ppib );

    return err;
}
