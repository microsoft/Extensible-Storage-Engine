// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


//  This retrieves the auto-inc value from the root page's external header.
//  NOTE: Returns JET_errColumnNull if the table is not upgraded with noderfIsamAutoInc storage.
ERR ErrCMPRECGetAutoInc( _Inout_ FUCB * const pfucbAutoIncTable, _Out_ QWORD * pqwAutoIncMax )
{
    ERR err = JET_errSuccess;

    Expected( !Pcsr( pfucbAutoIncTable )->FLatched() ); // don't want to ruin anyone else's currency

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

    Assert( err == JET_errSuccess || err == JET_wrnColumnNull );    // later error is for non-upgraded ...

    return err;
}

//  This sets the auto-inc value on the root page's external header.
ERR ErrCMPRECSetAutoInc( _Inout_ FUCB * const pfucbAutoIncTable, _In_ QWORD qwAutoIncSet )
{
    ERR err = JET_errSuccess;
    DATA dataAutoIncSet;

    Call( ErrDIRGetRootField( pfucbAutoIncTable, noderfIsamAutoInc, latchRIW ) );
    if ( err == JET_wrnColumnNull )
    {
        AssertTrack( fFalse, "AutoIncRootHighWaterNull" ); // possibly JET_wrnColumnNull if caller didn't avoid older format tables properly
        DIRReleaseLatch( pfucbAutoIncTable );
        Call( ErrERRCheck( errCodeInconsistency ) );
    }

    dataAutoIncSet.SetPv( &qwAutoIncSet );
    dataAutoIncSet.SetCb( sizeof(qwAutoIncSet) );
    err = ErrDIRSetRootField( pfucbAutoIncTable, noderfIsamAutoInc, dataAutoIncSet );
    //  need to release latch, just fall through to return the error

    DIRReleaseLatch( pfucbAutoIncTable );

HandleError:
    return err;
}

ERR ErrRECInitAutoIncSpace( _In_ FUCB* const pfucb, QWORD qwAutoInc )
{
    ERR     err         = JET_errSuccess;
    DATA    dataAutoInc;
    BOOL fTransactionStarted = fFalse;

    // To ensure the format changing is not accidentally performed
    Expected( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( efvExtHdrRootFieldAutoInc ) >= JET_errSuccess );

    CallR( ErrDIRGetRootField( pfucb, noderfIsamAutoInc, latchRIW ) );
    dataAutoInc.SetPv( &qwAutoInc );
    dataAutoInc.SetCb( sizeof(qwAutoInc) );
    if ( pfucb->ppib->Level() == 0 )
    {
        // ErrLGSetExternalHeader, called in ErrNDSetExternalHeader, asks to be in trx
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

// Initialize the new autoInc.
// If old format, return errNotFound. Caller would fall back to old autoInc.
// If autoInc is given with parameter, try to set it to the external header
// Otherwise load autoInc from external header
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
        // Some other thread may have already increased the max
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
            // This is the first time in history we visit the autoInc on this root page
            Assert( ptdb->QwGetAllocatedAutoIncMax() == 0 );
            Assert( ptdb->QwAutoincrement() == 0 );
        }
    }
    else
    {
        // This happens when we add autoInc column
        Assert( ptdb->QwGetAllocatedAutoIncMax() == 0 );
        Assert( ptdb->QwAutoincrement() == 0 );
        // We want currently used max autoInc. Caller (fcreate.cxx) passes in next usable autoInc
        qwAutoIncStored = qwAutoInc - 1;
    }

    ULONG ulBatchSize = ptdb->DwGetAutoIncBatchSize();
    Assert( ulBatchSize >= 1 );
    // Batch size starts from 1, but it will be multiplied by 2 with below block,
    // so batch size effectively starts from 2
    if ( ulBatchSize < g_ulAutoIncBatchSize )
    {
        // We are quite sure it doesn't overflow because of the value of g_ulAutoIncBatchSize
        // But let's assert just to make sure.
        Assert( ( ulBatchSize << 1 ) > ulBatchSize );
        ulBatchSize <<= 1;
        if ( ulBatchSize > g_ulAutoIncBatchSize )
        {
            ulBatchSize = g_ulAutoIncBatchSize;
        }
        ptdb->SetAutoIncBatchSize( ulBatchSize );
    }

    // In case the autoInc is nearly used up, decrease the batch
    while ( qwAutoIncStored >= ( qwCounterMax - ulBatchSize ) && ulBatchSize > 1 )
    {
        ulBatchSize >>= 1;
        // The is the rare case that autoInc is almost used up, and called at most 9 times in the very worest case
        // at the end of life of a table with AutoInc column. It should be OK to put the set in this loop
        // in order not to affect the normal scenario.
        ptdb->SetAutoIncBatchSize( ulBatchSize );
    }
    if ( qwAutoIncStored >= ( qwCounterMax - ulBatchSize ) )
    {
        // Autoinc has been really used up
        OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagAlertOnly, L"d0a09b0c-e038-4f8e-8437-ba448e8080bd" );
        Call( ErrERRCheck( JET_errOutOfAutoincrementValues ) );
    }

    qwAutoIncStored += ulBatchSize;
    dataAutoInc.SetPv( &qwAutoIncStored );
    dataAutoInc.SetCb( sizeof(qwAutoIncStored) );

    if ( ppib->Level() == 0 )
    {
        // ErrLGSetExternalHeader, called in ErrNDSetExternalHeader, asks to be in trx
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
        // This is the init for this autoInc since this DB is up
        Assert( ptdb->QwGetAllocatedAutoIncMax() == 0 );
        // Set next usable autoInc in TDB.
        // qwAutoIncStored was increased by ulBatchSize with some lines above, so
        // minus ulBatchSize; and we want the next availabe autoInc, so + 1 here.
        ptdb->InitAutoincrement( qwAutoIncStored - ulBatchSize + 1 );
    }
    // Set max allocated auto inc in TDB
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
        // At this point, it should has been initialized, batch loaded.
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

    //  look for index with first column as autoincrement column
    //
    pfcb->EnterDML();
    for ( pfcbIdx = ( pidbNil == pfcb->Pidb() ? pfcb->PfcbNextIndex() : pfcb );
        pfcbNil != pfcbIdx;
        pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        const IDB * const   pidb    = pfcbIdx->Pidb();
        Assert( pidbNil != pidb );

        //  don't use the index if it's potentially not visible to us
        //  or if not all records may be represented in the index
        //
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

    // delete nodes might be scrubbed so we wont' find any data
    // on there: skip them
    dib.dirflag = fDIRAllNode | fDIRAllNodesNoCommittedDeleted;
    dib.pbm = NULL;

    if ( pfcbIdx != pfcbNil )
    {
        //  seek on index to find maximum existing auto-inc value
        //
        Call( ErrBTOpen( ppib, pfcbIdx, &pfucb ) );
        Assert( pfucbNil != pfucb );

        dib.pos = ( fDescending ? posFirst : posLast );
        err = ErrBTDown( pfucb, &dib, latchReadTouch );
        if ( JET_errRecordNotFound == err )
        {
            //  the index (and therefore the table) is empty
            //
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

            //  ensure all bytes are zeroed out in case
            //  we don't use them all (ie. 4-byte auto-inc)
            //
            Assert( 0 == qwAutoInc );

            //  retrieve auto-inc value from key
            //
            pfcb->EnterDML();
            err = ErrRECIRetrieveColumnFromKey(
                            ptdb,
                            pfcbIdx->Pidb(),
                            &pfucb->kdfCurr.key,
                            columnidAutoInc,
                            &dataField );
            pfcb->LeaveDML();
            CallS( err );   //  should succeed with non-NULL value
            Call( err );

            Assert( (SIZE_T)dataField.Cb() == ( f8BytesAutoInc ? sizeof(QWORD) : sizeof(ULONG) ) );
            Assert( qwAutoInc > 0 );        // auto-inc's start numbering at 1
        }
    }
    else
    {
        //  no appropriate index, so must scan table instead in
        //  order to find maximum existing auto-inc value
        //
        Call( ErrBTOpen( ppib, pfcb, &pfucb ) );
        Assert( pfucbNil != pfucb );

        FUCBSetPrereadForward( pfucb, cpgPrereadSequential );
        dib.pos     = posFirst;

        err = ErrBTDown( pfucb, &dib, latchReadTouch );
        Assert( JET_errNoCurrentRecord != err );
        if ( JET_errRecordNotFound == err )
        {
            //  the table is empty
            //
            Assert( 0 == qwAutoInc );
        }
        else
        {
            FCB *   pfcbT   = pfcb;

            if ( fTemplateColumn )
            {
                if ( !pfcb->FTemplateTable() )
                {
                    // switch to template table
                    //
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
                //  validate result of record navigation
                //
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

                //  warnings not expected (including NULL, since
                //  all records must have an auto-inc value)
                //
                CallS( err );
                if ( err == JET_wrnColumnNull )
                {
                    //  We shouldn't be here, but we get here from watsons, so we
                    //  need to handle it gracefully.
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
                
                //  check whether current auto-inc value is greater
                //  than any we've encountered so far
                //
                if ( qwT > qwAutoInc )
                {
                    qwAutoInc = qwT;
                }

                err = ErrBTNext( pfucb, fDIRAllNode | fDIRAllNodesNoCommittedDeleted );
            }
            while ( JET_errNoCurrentRecord != err );

            //  traversed entire table
            //
            Assert( JET_errNoCurrentRecord == err );
        }
    }
    
    //  if there are no records in the table, then the first
    //  autoincrement value is 1.  Otherwise, set autoincrement
    //  to next value after maximum found.
    //
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

    // First try to load from root ext-hdr
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
            //  If autoincrement is not yet initialized, query table to
            //  initialize autoincrement value.
            //
            Call( ErrRECIInitAutoIncOldFormat( pfucb->ppib, pfcb, &qwAutoInc ) );

            // If root ext-hdr auto-inc is configured, upgrade the table now
            const BOOL fExternalHeaderNewFormatEnabled =
                ( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( efvExtHdrRootFieldAutoInc ) >= JET_errSuccess );
            if ( fExternalHeaderNewFormatEnabled &&
                 // This can fail if the root page does not have enough space, just keep going
                 ErrRECInitAutoIncSpace( pfucb, qwAutoInc - 1 ) >= JET_errSuccess )
            {
                // Now load a new batch
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
            || pfcbT->FTypeTemporaryTable() ) );    // Don't currently support autoinc with sorts/temp. tables

    //  If autoincrement is not yet initialized, query table to
    //  initialize autoincrement value.
    //  
    Call( ptdbT->ErrInitAutoInc( pfucb, 0 ) );

    //  set auto increment column in record
    //
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
            // Switch to template table.
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
        //  all cursors in the list should be owned by the same session
        //
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
#endif  //  defined( DEBUG ) || !defined( RTM )


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
                //  rollback the operations we did while the update was prepared.
                //  on insert copy, also rollback refcount increments.
                Assert( updateidNil != pfucb->updateid );
                Assert( pfucb->ppib->Level() > 0 );
                err = ErrVERRollback( pfucb->ppib, pfucb->updateid );
                CallSx( err, JET_errRollbackError );
                Call ( err );
            }

            // Ensure empty LV buffer.  Don't put this check inside the
            // FFUCBUpdateSeparateLV() check above because we may have created
            // a copy buffer, but cancelled the SetColumn() (eg. write conflict)
            // before the LV was actually updated (before FUCBSetUpdateSeparateLV()
            // could be called).
            RECIFreeCopyBuffer( pfucb );
            FUCBResetUpdateFlags( pfucb );
            break;

        case JET_prepInsert:
            //  ensure that table is updatable
            //
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

            //  initialize record
            //
            Assert( pfucb != pfucbNil );
            Assert( pfucb->dataWorkBuf.Pv() != NULL );
            Assert( FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

#ifdef PREREAD_INDEXES_ON_PREPINSERT
            if( pfucb->u.pfcb->FPreread() )
            {
                BTPrereadIndexesOfFCB( pfucb );
            }
#endif  //  PREREAD_INDEXES_ON_PREPINSERT

            Assert( pfucb->u.pfcb != pfcbNil );
            pfucb->u.pfcb->EnterDML();

            if ( NULL == pfucb->u.pfcb->Ptdb()->PdataDefaultRecord() )
            {
                // Only temporary tables and system tables don't have default records
                // (ie. all "regular" tables have at least a minimal default record).
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

                // Temp/sort tables and system tables don't have default records.
                Assert( !FFUCBSort( pfucb ) );
                Assert( !pfucb->u.pfcb->FTypeSort() );
                Assert( !pfucb->u.pfcb->FTypeTemporaryTable() );
                Assert( !FCATSystemTable( ptdbT->SzTableName() ) );

                Assert( ptdbT->PdataDefaultRecord()->Cb() >= REC::cbRecordMin );
                Assert( ptdbT->PdataDefaultRecord()->Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
                if ( ptdbT->PdataDefaultRecord()->Cb() > REC::CbRecordMost( pfucb ) )
                {
                    FireWall( "PrepInsertDefaultRecTooBig0.2" ); // Trying to see if we can upgrade to a more restrictive clause in below if
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

                // May only burst default record if last fixed and
                // var columns in the default record are committed.
                // If they are versioned, there's a risk they might
                // be rolled back from underneath us.
                fBurstDefaultRecord = fTrue;
                if ( precDefault->FidFixedLastInRec() >= fidFixedLeast )
                {
                    const FID   fid             = precDefault->FidFixedLastInRec();
                    const BOOL  fTemplateColumn = ptdbT->FFixedTemplateColumn( fid );
                    FIELD       *pfield         = ptdbT->PfieldFixed( ColumnidOfFid( fid, fTemplateColumn ) );

                    //  if field is versioned or deleted after schema was faulted in, there's
                    //  a chance the column is in the default record, but we don't have
                    //  visibility on it, so must must burst columns one-by-one
                    //
                    if ( FFIELDVersioned( pfield->ffield )
                        || ( FFIELDDeleted( pfield->ffield ) && fid > ptdbT->FidFixedLastInitial() ) )
                        fBurstDefaultRecord = fFalse;
                }
                if ( precDefault->FidVarLastInRec() >= fidVarLeast )
                {
                    const FID   fid             = precDefault->FidVarLastInRec();
                    const BOOL  fTemplateColumn = ptdbT->FVarTemplateColumn( fid );
                    FIELD       *pfield         = ptdbT->PfieldVar( ColumnidOfFid( fid, fTemplateColumn ) );

                    //  if field is versioned or deleted after schema was faulted in, there's
                    //  a chance the column is in the default record, but we don't have
                    //  visibility on it, so must must burst columns one-by-one
                    //
                    if ( FFIELDVersioned( pfield->ffield )
                        || ( FFIELDDeleted( pfield->ffield ) && fid > ptdbT->FidVarLastInitial() ) )
                        fBurstDefaultRecord = fFalse;
                }

                if ( fBurstDefaultRecord )
                {
                    // Only burst fixed and variable column defaults.
                    pfucb->dataWorkBuf.SetCb( precDefault->PbTaggedData() - (BYTE *)precDefault );
                    Assert( pfucb->dataWorkBuf.Cb() >= REC::cbRecordMin );
                    Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
                    Assert( pfucb->dataWorkBuf.Cb() <= REC::CbRecordMost( pfucb ) );
                    Assert( pfucb->dataWorkBuf.Cb() <= ptdbT->PdataDefaultRecord()->Cb() );
                    if ( pfucb->dataWorkBuf.Cb() > REC::CbRecordMost( pfucb ) )
                    {
                        FireWall( "PrepInsertWorkingBufferAllowedRecTooBig1.2" ); // Trying to see if we can upgrade to a more restrictive clause in below if
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
                    // Try bursting just the fixed columns.

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

                        //  there is at least one non-versioned column. burst it
                        Assert( fidBurst <= fidFixedMost );
                        const INT   cFixedColumnsToBurst = fidBurst - fidFixedLeast + 1;
                        Assert( cFixedColumnsToBurst > 0 );

                        //  get the starting offset of the column ahead of this one
                        //  add space for the column bitmap
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
                            FireWall( "WorkingBufferAllowedRecTooBig2.2" ); // Trying to see if we can upgrade to this in below if.
                        }
                        if ( pfucb->dataWorkBuf.Cb() < REC::cbRecordMin ||
                            pfucb->dataWorkBuf.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) ||
                            pfucb->dataWorkBuf.Cb() > ptdbT->PdataDefaultRecord()->Cb() )
                        {
                            FireWall( "WorkingBufferAllowedRecTooBig2.1" );
                            Error( ErrERRCheck( JET_errDatabaseCorrupted ) );
                        }

                        //  copy the default record values
                        UtilMemCpy( pbRec, (BYTE *)precDefault, cbFixedBurst );

                        prec->SetFidFixedLastInRec( fidBurst );
                        prec->SetFidVarLastInRec( fidVarLeast-1 );
                        prec->SetIbEndOfFixedData( (USHORT)cbFixedBurst );

                        //  set the fixed column bitmap
                        BYTE    *pbDefaultBitMap    = precDefault->PbFixedNullBitMap();
                        Assert( pbDefaultBitMap - (BYTE *)precDefault ==
                            ptdbT->IbOffsetOfNextColumn( precDefault->FidFixedLastInRec() ) );

                        UtilMemCpy( pbRec + ibFixEnd, pbDefaultBitMap, cbBitMap );

                        //  must nullify bits for columns not in this record
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
                        // all fixed columns are versioned, or no fixed columns
                        pfucb->u.pfcb->LeaveDML();

                        // Start with an empty record.  Columns will be
                        // burst on an as-needed basis.
                        REC::SetMinimumRecord( pfucb->dataWorkBuf );
                    }
                }
            }

            FUCBResetColumnSet( pfucb );

            //  if table has an autoincrement column, then set column
            //  value now so that it can be retrieved from copy buffer.
            //
            if ( pfucb->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
            {
                Call( ErrRECISetAutoincrement( pfucb ) );
            }
            err = JET_errSuccess;
            PrepareInsert( pfucb );
            break;

        case JET_prepReplace:
            //  ensure that table is updatable
            //
            CallR( ErrFUCBCheckUpdatable( pfucb ) );
            if ( !FFMPIsTempDB(pfucb->ifmp) )
            {
                CallR( ErrPIBCheckUpdatable( ppib ) );
            }
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            //  write lock node.  Note that ErrDIRGetLock also
            //  gets the current node, so no additional call to ErrDIRGet
            //  is required.
            //
            //  if locking at level 0 then goto JET_prepReplaceNoLock
            //  since lock cannot be acquired at level 0 and lock flag
            //  in fucb will prevent locking in update operation required
            //  for rollback.
            //
            if ( pfucb->ppib->Level() == 0 )
            {
                goto ReplaceNoLock;
            }

            //  put assert to catch client's misbehavior. Make sure that
            //  no such sequence:
            //      PrepUpdate(t1) PrepUpdate(t2) Update(t1) Update(t2)
            //  where t1 and t2 happen to be on the same record and on the
            //  the same table. Client will experience lost update of t1 if
            //  they have such calling sequence.
            //
            Call( ErrRECSessionWriteConflict( pfucb ) );

            // Ensure we don't mistakenly set rceidBeginReplace.
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
            //  ensure that table is updatable
            //
            CallR( ErrFUCBCheckUpdatable( pfucb ) );
            if ( !FFMPIsTempDB(pfucb->ifmp) )
            {
                CallR( ErrPIBCheckUpdatable( ppib ) );
            }
            if ( FFUCBUpdatePrepared( pfucb ) )
                return ErrERRCheck( JET_errAlreadyPrepared );

ReplaceNoLock:
            Assert( !FFUCBUpdatePrepared( pfucb ) );

            //  put assert to catch client's misbehavior. Make sure that
            //  no such sequence:
            //      PrepUpdate(t1) PrepUpdate(t2) Update(t1) Update(t2)
            //  where t1 and t2 happen to be on the same record and on the
            //  the same table. Client will experience lost update of t1 if
            //  they have such calling sequence.
            //
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
            //  ensure that table is updatable
            //
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

            //  if table has an autoincrement column, then set column value now
            //  so that it can be retrieved from copy buffer.
            //
            if ( pfucb->u.pfcb->Ptdb()->FidAutoincrement() != 0 )
            {
                //  if ICRO then keep same autoincrement value
                //
                if ( JET_prepInsertCopyReplaceOriginal == grbit )
                {
                    //  ICRO does not allocate new autoinc values, and thus does not need
                    //  autoinc support to be initialized.  However, initializing autoincrement here
                    //  allows checks during update to work correctly.
                    //
                    Call( pfucb->u.pfcb->Ptdb()->ErrInitAutoInc( pfucb, 0 ) );
                }
                else
                {
                    //  update autoinc column in copy buffer
                    //
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

                //  increment reference count on long values
                //
                Assert( updateidNil == ppib->updateid );        // should not be nested
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
                fFalse /* GUID collation does not affect uniqueness */,
                &fNormDataRecTruncated ) );

    CallS( err );       //  shouldn't get warnings

    if ( FDataEqual( dataFieldNorm, dataRecNorm ) )
    {
        //  since data is equal, they should either both be truncated or both not truncated
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
                                NULL ) );               //  pass NULL to force comparison instead of retrieval
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
    //  FUTURE:     unique column values is AD feature and JET_cbKeyMost_OLD is used to ensure backward compatibility
    //              w/o new forrest mode.  More elegant would be to tie length of uniqueness to index or table
    //              definition.
    //
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
                fFalse /* GUID collation does not affect uniqueness */,
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

// given the column we are updating and the options passed in, determine which compression flags to use
///#define FORCE_COLUMN_COMPRESSION_ON
LOCAL CompressFlags CalculateCompressFlags( FMP* const pfmp, const FIELD * const pfield, const ULONG grbit )
{
    const BOOL fLongValue = FRECLongValue( pfield->coltyp );

    CompressFlags compressFlags = compressNone;
    if( fLongValue )
    {
        // the grbits take priority over the column meta-data
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
            //  cannot currently combine JET_bitSetUniqueMultiValues with other grbits
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
            //  Technically the required should be slightly less than a full page size / i.e how much ever LV
            //  data overflows a single page.
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

    // Verify column is visible to us.
    CallR( ErrRECIAccessColumn( pfucb, columnid ) );

    // If pv is NULL, cb should be 0, except if SetSizeLV is specified.
    if ( pdataField->Pv() == NULL && !( grbit & JET_bitSetSizeLV ) )
        pdataField->SetCb( 0 );

    Assert ( pdataField->Cb() >= 0 );

    //  Return error if version or autoinc column is being set.  We don't
    //  need to grab the FCB critical section -- since we can see the
    //  column, we're guaranteed the FID won't be deleted or rolled back.
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
            Assert( pfcb->Ptdb()->FidAutoincrement() != FidOfColumnid( columnid ) );    // Assert mutual-exclusivity.
            Assert( FFIELDVersion( ffield ) );

            // Cannot set a Version field during a replace.
            if ( FFUCBReplacePrepared( pfucb ) )
                return ErrERRCheck( JET_errInvalidColumnType );
        }
        else if ( pfcb->Ptdb()->FidAutoincrement() == FidOfColumnid( columnid ) )
        {
            Assert( FFIELDAutoincrement( ffield ) );

            // Can never set an AutoInc field.
            return ErrERRCheck( JET_errInvalidColumnType );
        }
    }

    else if ( FCOLUMNIDTagged( columnid ) )     // check for long value
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
                    SORTIDNil, // Sort GUID
                    PinstFromPfucb( pfucb )->m_dwLCMapFlagsDefault,
                    0, // NLS Version
                    0, // NLS Defined Version
                    L'\0', // locale name
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

            //  if column does not fit then try to separate long values and try to set column again
            //
            if ( JET_errRecordTooBig == err )
            {
                Assert( (ULONG) g_cbPage == cbThreshold );
                const ULONG cbLid = LvId::CbLidFromCurrFormat( pfucb );
                while ( JET_errRecordTooBig == err && cbThreshold > cbLid )
                {
                    //  update threshold before next attempt to set long field
                    //
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


    //  do the actual column operation
    //

    //  setting value to NULL
    //
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
            //  update threshold before next attempt to set long field
            //
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



//+API
//  ErrIsamSetColumn
//  ========================================================================
//  ErrIsamSetColumn( PIB *ppib, FUCB *pfucb, FID fid, ULONG itagSequence, DATA *pdataField, JET_GRBIT grbit )
//
//  Adds or changes a column value in the record being worked on.
//  Fixed and variable columns are replaced if they already have values.
//  A sequence number must be given for tagged columns.  If this is zero,
//  a new tagged column occurance is added to the record.  If not zero, it
//  specifies the occurance to change.
//  A working buffer is allocated if there isn't one already.
//  If fNewBuf == fTrue, the buffer is initialized with the default values
//  for the columns in the record.  If fNewBuf == fFalse, and there was
//  already a working buffer, it is left unaffected;     if a working buffer
//  had to be allocated, then this new working buffer will be initialized
//  with either the column values of the current record, or the default column
//  values (if the user's currency is not on a record).
//
//  PARAMETERS  ppib            PIB of user
//              pfucb           FUCB of data file to which this record
//                              is being added/updated.
//              fid             column id: which column to set
//              itagSequence    Occurance number (for tagged columns):
//                              which occurance of the column to change
//                              If zero, it means "add a new occurance"
//              pdataField      column data to use
//              grbit           If JET_bitSetZeroLength, the column is set to size 0.
//
//  RETURNS     Error code, one of:
//                   JET_errSuccess             Everything worked.
//                  -JET_errOutOfBuffers        Failed to allocate a working
//                                              buffer
//                  -JET_errInvalidBufferSize
//
//                  -ColumnInvalid              The column id given does not
//                                              corresponding to a defined column
//                  -NullInvalid                An attempt was made to set a
//                                              column to NULL which is defined
//                                              as NotNull.
//                  -JET_errRecordTooBig        There is not enough room in
//                                              the record for new column.
//  COMMENTS    The GET and DELETE commands discard the working buffer
//              without writing it to the database.  The REPLACE and INSERT
//              commands may be used to write the working buffer to the
//              database, but they also discard it when finished (the INSERT
//              command can be told not to discard it, though;  this is
//              useful for adding several similar records).
//              For tagged columns, if the data given is NULL-valued, then the
//              tagged column occurance specified is deleted from the record.
//              If there is no tagged column occurance corresponding to the
//              specified occurance number, a new tagged column is added to
//              the record, and assumes the new highest occurance number
//              (unless the data given is NULL-valued, in which case the
//              record is unaffected).
//-
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

    // check for updatable table
    //
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

    //  remember which update we are part of (save off session's current
    //  updateid because we may be nested if the top-level SetColumn()
    //  causes catalog updates when creating LV tree).
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

    // check for updatable table
    //
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
        
    //  remember which update we are part of (save off session's current
    //  updateid because we may be nested if the top-level SetColumn()
    //  causes catalog updates when creating LV tree).
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

        //  max. default long value is cbLVIntrinsicMost-1 (one byte
        //  reserved for fSeparated flag).
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
                        NO_ITAG,        // itagSequence == 0 to force new column
                        &dataNullT,
                        &dataField,
                        0,
                        pfield->cbMaxLen,
                        lvopInsert,
                        compressNone,   // don't compress the in-memory default record
                        fFalse );       // don't encrypt the in-memory default record
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


// The engine normally passes an FUCB and the data buffer hanging off the FUCB, except shrink because it has the advantage
// of knowing it isn't updating any indices and needs to do it directly to a buffer during root moves.

LOCAL ERR ErrRECISetIFixedColumn(
    FUCB            * const pfucb,
    DATA            * const pdataWorkBuf,
    TDB             * const ptdb,
    const COLUMNID  columnid,
    DATA            *pdataField )
{
    const FID       fid         = FidOfColumnid( columnid );

    Assert( fid.FFixed() );

    Assert( ( ( pfucb != pfucbNil ) && ( &( pfucb->dataWorkBuf ) == pdataWorkBuf ) ) ||
            ( ( pfucb == pfucbNil ) && ( pdataWorkBuf != NULL ) ) );
    Assert( ( pfucb == pfucbNil ) || FFUCBIndex( pfucb ) || FFUCBSort( pfucb ) );

    INT         cbRec = pdataWorkBuf->Cb();
    Assert( cbRec >= REC::cbRecordMin );
    Assert( cbRec <= REC::CbRecordMostWithGlobalPageSize() );
    if ( pfucb && cbRec > REC::CbRecordMost( pfucb ) )
    {
        FireWall( "SetFixedColumRecTooBig3.2" ); // Trying to upgrade to this check in below if.
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
        //  this should be impossible, because the columnid
        //  should already have been validated by this point
        //
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    FIELD       *pfield = ptdb->PfieldFixed( columnid );
    Assert( pfieldNil != pfield );

    if ( JET_coltypNil == pfield->coltyp )
    {
        //  this should be impossible, because the columnid
        //  should already have been validated by this point
        //
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    //  for fixed columns, we interpret setting to zero-length as
    //  the same thing as setting to NULL
    //
    else if ( ( NULL == pdataField || pdataField->FNull() )
        && FFIELDNotNull( pfield->ffield ) )
    {
        return ErrERRCheck( JET_errNullInvalid );
    }

    if ( pfucb != pfucbNil )
    {
        //  record the fact that this column has been changed
        //
        FUCBSetColumnSet( pfucb, fid );
    }

    //  column not represented in record? Make room, make room
    //
    if ( fid > prec->FidFixedLastInRec() )
    {
        Assert( pfucb != pfucbNil );

        const FID   fidFixedLastInRec       = prec->FidFixedLastInRec();
        FID         fidLastDefaultToBurst;
        FID         fidT;

        // Verify there's at least one more fid beyond fidFixedLastInRec,
        // thus enabling us to reference the FIELD structure of the fid
        // beyond fidFixedLastInRec.
        Assert( fidFixedLastInRec < ptdb->FidFixedLast() );

        //  compute room needed for new column and bitmap
        //
        const WORD  ibOldFixEnd     = WORD( prec->PbFixedNullBitMap() - pbRec );
        const WORD  ibOldBitMapEnd  = prec->IbEndOfFixedData();
        const INT   cbOldBitMap     = ibOldBitMapEnd - ibOldFixEnd;
        Assert( ibOldBitMapEnd >= ibOldFixEnd );
        Assert( ibOldFixEnd == ptdb->IbOffsetOfNextColumn( fidFixedLastInRec ) );
        Assert( ibOldBitMapEnd == ibOldFixEnd + cbOldBitMap );
        Assert( (ULONG)cbOldBitMap == prec->CbFixedNullBitMap() );
        // FUTURE (as of 9/07/2006): Change Assert validations into retail checks:
        //if ( ( ibOldBitMapEnd < ibOldFixEnd )||
        //     (    ibOldFixEnd != ptdb->IbOffsetOfNextColumn( fidFixedLastInRec ) ) ||
        //     ( ibOldBitMapEnd != ibOldFixEnd + cbOldBitMap ) ||
        //     ( (ULONG)cbOldBitMap != prec->CbFixedNullBitMap() ) )
        //  {
        //  return ErrERRCheck( JET_errInvalidBufferSize );
        //  }

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

        //  shift rest of record over to make room
        //
        Assert( cbRec >= ibOldBitMapEnd );
        memmove(
            pbRec + ibNewBitMapEnd,
            pbRec + ibOldBitMapEnd,
            cbRec - ibOldBitMapEnd );

        // fill the new space with predictable data
        memset( pbRec + ibOldBitMapEnd, chRECFill, cbShift );

        pdataWorkBuf->DeltaCb( cbShift );
        cbRec = pdataWorkBuf->Cb();

        // set new location of variable/tagged data
        prec->SetIbEndOfFixedData( ibNewBitMapEnd );

        //  shift fixed column bitmap over
        //
        memmove(
            pbRec + ibNewFixEnd,
            pbRec + ibOldFixEnd,
            cbOldBitMap );

        // fill the new space with predictable data
        memset( pbRec + ibOldFixEnd, chRECFill, ibNewFixEnd - ibOldFixEnd );

        //  clear all new bitmap bits
        //

        // If there's at least one fixed column currently in the record,
        // find the nullity bit for the last fixed column and clear the
        // rest of the bits in that byte.
        BYTE    *prgbitNullity;
        if ( fidFixedLastInRec >= fidFixedLeast )
        {
            UINT    ifid    = fidFixedLastInRec - fidFixedLeast;    // Fid converted to an index.

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

            prgbitNullity++;        // Advance to next nullity byte.
            Assert( prgbitNullity <= pbRec + ibNewBitMapEnd );
        }
        else
        {
            prgbitNullity = pbRec + ibNewFixEnd;
            Assert( prgbitNullity < pbRec + ibNewBitMapEnd );
        }

        // set all NULL bits at once
        memset( prgbitNullity, 0xff, pbRec + ibNewBitMapEnd - prgbitNullity );


        // Default values may have to be burst if there are default value columns
        // between the last one currently in the record and the one we are setting.
        // (note that if the column being set also has a default value, we have
        // to set the default value first in case the actual set fails.
        const REC * const   precDefault = ( NULL != ptdb->PdataDefaultRecord() ?
                                                    (REC *)ptdb->PdataDefaultRecord()->Pv() :
                                                    NULL );
        Assert( NULL == precDefault ||  // temp/system tables have no default record.
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

                    // Update nullity bit.  Assert that it's currently
                    // set to null, then set it to non-null in preparation
                    // of the bursting of the default value.
                    prgbitNullity = pbRec + ibNewFixEnd + (ifid/8);
                    Assert( FFixedNullBit( prgbitNullity, ifid ) );
                    ResetFixedNullBit( prgbitNullity, ifid );
                    fidLastDefaultToBurst = fidT;
                }
            }

            // Only burst default values between the last fixed
            // column currently in the record and the column now
            // being set.
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

        //  increase fidFixedLastInRec
        //
        prec->SetFidFixedLastInRec( fid );
    }

    //  fid is now definitely represented in
    //  the record; its data can simply be replaced
    //

    //  adjust fid to an index
    //
    const UINT  ifid            = fid - fidFixedLeast;

    //  byte containing bit representing column's nullity
    //
    BYTE        *prgbitNullity  = prec->PbFixedNullBitMap() + ifid/8;

    //  adding NULL: clear bit
    //
    if ( NULL == pdataField || pdataField->FNull() )
    {
        Assert( !FFIELDNotNull( pfield->ffield ) );     //  already verified above
        SetFixedNullBit( prgbitNullity, ifid );
    }
    else
    {
        //  adding non-NULL value: set bit, copy value into slot
        //
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

            //  If the data is converted, then the cbCopy must be the same as pdataField->Cb()
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

    // Compute space needed to burst default values.
    // Default values may have to be burst if there are default value columns
    // between the last one currently in the record and the one we are setting.
    // (note that if the column being set also has a default value, we don't
    // bother setting it, since it will be overwritten).
    Assert( NULL == precDefault ||  // temp/system tables have no default record.
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

                // Don't currently support NULL default values.
                Assert( !FVarNullBit( pibDefaultVarOffs[ifid] ) );

                //  beginning of current column is end of previous column
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

    Assert( fid.FVar() );

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
        //  this should be impossible, because the columnid
        //  should already have been validated by this point
        //
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    FIELD       *pfield = ptdb->PfieldVar( columnid );
    Assert( pfieldNil != pfield );
    if ( JET_coltypNil == pfield->coltyp )
    {
        //  this should be impossible, because the columnid
        //  should already have been validated by this point
        //
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    //  record the fact that this column has been changed
    //
    FUCBSetColumnSet( pfucb, fid );

    //  NULL-value check
    //
    INT     cbCopy;             // Number of bytes to copy from user's buffer
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
        //  column too long
        //
        cbCopy = pfield->cbMaxLen;
        err = ErrERRCheck( JET_wrnColumnMaxTruncated );
    }
    else
    {
        cbCopy = pdataField->Cb();
    }
    Assert( cbCopy >= 0 );

    //  variable column offsets
    //
    UnalignedLittleEndian<REC::VAROFFSET>   *pib;
    UnalignedLittleEndian<REC::VAROFFSET>   *pibLast;
    UnalignedLittleEndian<REC::VAROFFSET>   *pibVarOffs;

    //  column not represented in record?  Make room, make room
    //
    const BOOL      fBurstRecord = ( fid > prec->FidVarLastInRec() );
    if ( fBurstRecord )
    {
        const FID   fidVarLastInRec = prec->FidVarLastInRec();
        FID         fidLastDefault;

        Assert( !( pdataField == NULL && FFIELDNotNull( pfield->ffield ) ) );

        //  compute space needed for new var column offsets
        //
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

        //  shift rest of record over to make room
        //
        BYTE    *pbVarOffsEnd = prec->PbVarData();
        Assert( pbVarOffsEnd >= (BYTE *)pibVarOffs );
        memmove(
                pbVarOffsEnd + cbNeed,
                pbVarOffsEnd,
                pbRec + cbRec - pbVarOffsEnd );

        // fill the new space with predictable data
        memset( pbVarOffsEnd, chRECFill, cbNeed );


        //  set new var offsets to tag offset, making them NULL
        //
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

        //  increase record size to reflect addition of entries in the
        //  variable offsets table.
        //
        Assert( prec->FidVarLastInRec() == fidVarLastInRec );
        prec->SetFidVarLastInRec( fid );
        Assert( pfucb->dataWorkBuf.Cb() == cbRec );
        cbRec += cbNeed;

        Assert( prec->PibVarOffsets() == pibVarOffs );
        Assert( pibVarOffs[fid-fidVarLeast] == ibTagFields );   // Includes null-bit comparison.

        // Burst default values if required.
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

            // Should have changed since last time, since we added
            // some columns.
            Assert( pbVarData > pbVarOffsEnd );
            Assert( pbVarData > pbRec );
            Assert( pbVarData <= pbRec + cbRec );

            pibDefaultVarOffs = precDefault->PibVarOffsets();

            // Make room for the default values to be burst.
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
            Assert( *pib == ibTagFields );  // Null bit also compared.
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

                // Null bit is initially set when the offsets
                // table is expanded above.
                Assert( FVarNullBit( *pib ) );

                if ( FFIELDDefault( pfieldVar->ffield )
                    && !FFIELDCommittedDelete( pfieldVar->ffield ) )
                {
                    Assert( JET_coltypNil != pfieldVar->coltyp );

                    // Update offset entry in preparation for the default value.
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

//                  Null bit gets reset when cb is set
//                  ResetVarNullBit( *pib );
                    Assert( !FVarNullBit( *pib ) );
                }
                else if ( fidT > fidVarLeast )
                {
                    *pib = IbVarOffset( *(pib-1) );

//                  Null bit gets reset when cb is set
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

            // Offset entries up to the last default have been set.
            // Update the entries between the last default and the
            // column being set.
            pibLast = pibVarOffs + ( fid - fidVarLeast );
            for ( ; pib <= pibLast; pib++ )
            {
                Assert( FVarNullBit( *pib ) );
                Assert( *pib == ibTagFields );

                *pib = REC::VAROFFSET( *pib + cbBurstDefaults );
            }

#ifdef DEBUG
            // Verify null bits vs. offsets.
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

    //  fid is now definitely represented in the record;
    //  its data can be replaced, shifting remainder of record,
    //  either to the right or left (if expanding/shrinking)
    //

    Assert( JET_errSuccess == err || JET_wrnColumnMaxTruncated == err );


    //  compute change in column size and value of null-bit in offset
    //
    pibVarOffs = prec->PibVarOffsets();
    pib = pibVarOffs + ( fid - fidVarLeast );

    // Calculate size change of column data
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

    //  size changed: must shift rest of record data
    //
    if ( 0 != dbFieldData )
    {
        BYTE    *pbVarData = prec->PbVarData();

        //  shift data
        //
        if ( cbRec + dbFieldData > REC::CbRecordMost( pfucb ) )
        {
            Assert( !fBurstRecord );        // If record had to be extended, space
                                            // consumption was already checked.
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

        //  bump affected var column offsets
        //
        Assert( fid <= prec->FidVarLastInRec() );
        Assert( prec->FidVarLastInRec() >= fidVarLeast );
        pibLast = pibVarOffs + ( prec->FidVarLastInRec() - fidVarLeast );
        for ( pib = pibVarOffs + ( fid - fidVarLeast ); pib <= pibLast; pib++ )
        {
            *pib = WORD( *pib + dbFieldData );
        }

        // Reset for setting of null bit below.
        pib = pibVarOffs + ( fid - fidVarLeast );
    }

    //  data shift complete, if any;  copy new column value in
    //
    Assert( cbCopy >= 0 );
    if ( cbCopy > 0 )
    {
        UtilMemCpy(
            prec->PbVarData() + ibStartOfColumn,
            pdataField->Pv(),
            cbCopy );
    }

    //  set value of null-bit in offset
    //
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
        //  this should be impossible, because the columnid
        //  should already have been validated by this point
        //
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }

    const FIELD     *const pfield       = ptdb->PfieldTagged( columnid );
    Assert( pfieldNil != pfield );
    if ( JET_coltypNil == pfield->coltyp )
    {
        //  this should be impossible, because the columnid
        //  should already have been validated by this point
        //
        Assert( fFalse );
        return ErrERRCheck( JET_errColumnNotFound );
    }
    else if ( NULL == pdataToSet && FFIELDNotNull( pfield->ffield ) )
    {
        return ErrERRCheck( JET_errNullInvalid );
    }

    //  record the fact that this column has been changed
    //
    FUCBSetColumnSet( pfucb, fid );

    const ULONG cbMaxLenPhysical = ( grbit & grbitSetColumnEncrypted ) ? CbOSEncryptAes256SizeNeeded( pfield->cbMaxLen ) : pfield->cbMaxLen;
    //  check for column too long
    //
    if ( pfield->cbMaxLen > 0
        && NULL != pdataToSet
        && (ULONG)pdataToSet->Cb() > cbMaxLenPhysical )
    {
        // Encrypted columns are only set via ErrRECAOSeparateLV which already checks the max logical size
        Assert( !( grbit & grbitSetColumnEncrypted ) );
        return ErrERRCheck( JET_errColumnTooBig );
    }

    //  check fixed size column size
    //
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

    //  cannot set column more than CbLVIntrinsicTableMost() bytes
    //
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


//  change default value of a non-derived column
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

    //  check parameters
    //
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

    if ( FFMPIsTempDB( ifmp ) )
    {
        Expected( fFalse ); // API never run on temp DB.
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CallR( ErrUTILCheckName( szColumn, szColumnName, JET_cbNameMost+1 ) );

    if ( NULL == pvData || 0 == cbData )
    {
        //  don't currently support null/zero-length default value
        //
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

    // We should now have exclusive use of the table.
    pfcb = pfucb->u.pfcb;
    objidTable = pfcb->ObjidFDP();
    Assert( pfcbNil != pfcb );

    ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );

    //  save off old default record in case we have to restore it on error
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


    //  NOTE: Can only change the default value of non-derived columns
    //  so don't even bother looking for the column in the template
    //  table's column space

    pfcb->EnterDML();
    //  WARNING: The following function does a LeaveDML() on error  
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

    //  if adding a default value, the default value
    //  flag will not be set, so must set it now
    //  before rebuilding the default record (because
    //  the rebuild code checks that flag)
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
