// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"


INLINE ERR ErrRECIGetRecord(
    FUCB    *pfucb,
    DATA    **ppdataRec,
    BOOL    fUseCopyBuffer )
{
    ERR     err = JET_errSuccess;

    Assert( ppdataRec );
    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

    //  retrieving from copy buffer?
    //
    if ( fUseCopyBuffer )
    {
        //  only index cursors have copy buffers.
        //
        Assert( FFUCBIndex( pfucb ) );
        Assert( !Pcsr( pfucb )->FLatched() );
        *ppdataRec = &pfucb->dataWorkBuf;
    }
    else
    {
        if ( FFUCBIndex( pfucb ) )
        {
            err = ErrDIRGet( pfucb );
        }
        else
        {
            //  sorts always have current data cached.
            Assert( pfucb->locLogical == locOnCurBM
                || pfucb->locLogical == locBeforeFirst
                || pfucb->locLogical == locAfterLast );
            if ( pfucb->locLogical != locOnCurBM )
            {
                err = ErrERRCheck( JET_errNoCurrentRecord );
            }
            else
            {
                Assert( pfucb->kdfCurr.data.Cb() != 0 );
            }
        }

        *ppdataRec = &pfucb->kdfCurr.data;
    }

    // Negative lengths indicate fatal corruption issues
    if ( err >= JET_errSuccess && (*ppdataRec)->Cb() < 0 )
    {
        err = ErrERRCheck( JET_errDatabaseCorrupted );
        AssertSzRTL( fFalse, "A database corruption has resulted in a record with a negative length." );
    }

    return err;
}

ERR ErrRECIAccessColumn( FUCB *pfucb, COLUMNID columnid, FIELD * const pfieldFixed, BOOL * pfEncrypted )
{
    ERR         err     = JET_errSuccess;
    PIB *       ppib    = pfucb->ppib;
    FCB *       pfcb    = pfucb->u.pfcb;
    TDB *       ptdb    = pfcb->Ptdb();

    if ( !FCOLUMNIDValid( columnid ) )
    {
        return ErrERRCheck( JET_errBadColumnId );
    }

    const FID   fid     = FidOfColumnid( columnid );
    
    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( pfcb->FTemplateTable() )
        {
            ptdb->AssertValidTemplateTable();
        }
        else if ( pfcbNil != ptdb->PfcbTemplateTable() )
        {
            ptdb->AssertValidDerivedTable();
            ptdb = ptdb->PfcbTemplateTable()->Ptdb();
        }
        else
        {
            return ErrERRCheck( JET_errColumnNotFound );
        }

        BOOL    fCopyField  = fFalse;
        FID     fidFirst;
        FID     fidLast;

        if ( fid.FTagged() )
        {
            fidFirst = ptdb->FidTaggedFirst();
            fidLast = ptdb->FidTaggedLast();
        }
        else if ( fid.FFixed() )
        {
            fidFirst = ptdb->FidFixedFirst();
            fidLast = ptdb->FidFixedLast();
            if ( pfieldNil != pfieldFixed )
                fCopyField = fTrue;
        }
        else
        {
            Assert( fid.FVar() );
            fidFirst = ptdb->FidVarFirst();
            fidLast = ptdb->FidVarLast();
        }

        Assert( JET_errSuccess == err );
        if ( fid > fidLast || fid < fidFirst )
        {
            err = ErrERRCheck( JET_errColumnNotFound );
        }
        else
        {
            const FIELD * const pfield = ptdb->Pfield( columnid );
            Assert( JET_coltypNil != pfield->coltyp );
            Assert( !FFIELDVersioned( pfield->ffield ) );
            Assert( !FFIELDDeleted( pfield->ffield ) );
            CallS( err );

            if ( fCopyField )
            {
                *pfieldFixed = *( pfield );
            }

            if ( pfEncrypted )
            {
                *pfEncrypted = FFIELDEncrypted( pfield->ffield );
            }
        }

        return err;
    }

    else if ( pfcb->FTemplateTable() )
    {
        return ErrERRCheck( JET_errColumnNotFound );
    }


    BOOL        fUseDMLLatch    = fFalse;
    FIELDFLAG   ffield;

    if ( fid.FTagged() )
    {
        if ( fid > ptdb->FidTaggedLastInitial() )
        {
            pfcb->EnterDML();
            if ( fid > ptdb->FidTaggedLast() )
            {
                pfcb->LeaveDML();
                return ErrERRCheck( JET_errColumnNotFound );
            }
            fUseDMLLatch = fTrue;
        }
        else if ( fid < ptdb->FidTaggedFirst () )
        {
            return ErrERRCheck( JET_errColumnNotFound );
        }

        ffield = ptdb->PfieldTagged( columnid )->ffield;
    }
    else if ( fid.FFixed() )
    {
        if ( fid > ptdb->FidFixedLastInitial() )
        {
            pfcb->EnterDML();
            if ( fid > ptdb->FidFixedLast() )
            {
                pfcb->LeaveDML();
                return ErrERRCheck( JET_errColumnNotFound );
            }
            fUseDMLLatch = fTrue;
        }
        else if ( fid < ptdb->FidFixedFirst () )
        {
            return ErrERRCheck( JET_errColumnNotFound );
        }

        FIELD * const   pfieldT     = ptdb->PfieldFixed( columnid );
        if ( pfieldNil != pfieldFixed )
        {
            *pfieldFixed = *pfieldT;

            //  use the snapshotted FIELD, because the real one
            //  could be being modified
            ffield = pfieldFixed->ffield;
        }
        else
        {
            ffield = pfieldT->ffield;
        }
    }
    else if ( fid.FVar() )
    {
        if ( fid > ptdb->FidVarLastInitial() )
        {
            pfcb->EnterDML();
            if ( fid > ptdb->FidVarLast() )
            {
                pfcb->LeaveDML();
                return ErrERRCheck( JET_errColumnNotFound );
            }
            fUseDMLLatch = fTrue;
        }
        else if ( fid < ptdb->FidVarFirst () )
        {
            return ErrERRCheck( JET_errColumnNotFound );
        }

        ffield = ptdb->PfieldVar( columnid )->ffield;
    }
    else
    {
        return ErrERRCheck( JET_errColumnNotFound );
    }
        
    if ( fUseDMLLatch )
        pfcb->AssertDML();

    if ( pfEncrypted )
    {
        *pfEncrypted = FFIELDEncrypted( ffield );
    }

    //  check if the FIELD has been versioned or deleted
    //  (note that we took only a snapshot of the flags
    //  above, so by now the FIELD may be different),
    if ( FFIELDVersioned( ffield ) )
    {
        Assert( !pfcb->FFixedDDL() );
        Assert( !pfcb->FTemplateTable() );

        if ( fUseDMLLatch )
            pfcb->LeaveDML();

        const BOOL  fLatchHeld  = Pcsr( pfucb )->FLatched();
        if ( fLatchHeld )
        {
            // UNDONE:  Move latching logic to caller.
            CallS( ErrDIRRelease( pfucb ) );
        }

        // Inherited columns are never versioned.
        Assert( !FCOLUMNIDTemplateColumn( columnid ) );

        err = ErrCATAccessTableColumn(
                    ppib,
                    pfcb->Ifmp(),
                    pfcb->ObjidFDP(),
                    NULL,
                    &columnid );
        Assert( err <= 0 );     // No warning should be generated.

        if ( fLatchHeld )
        {
            ERR errT = ErrDIRGet( pfucb );
            if ( errT < 0 )
            {
                //  if error encountered while re-latching record, return the error
                //  only if no unexpected errors from catalog consultation.
                if ( err >= 0 || JET_errColumnNotFound == err )
                    err = errT;
            }
        }
    }

    else
    {
        if ( FFIELDDeleted( ffield ) )
        {
            //  may get deleted columns in FixedDDL tables if an AddColumn() in those
            //  tables rolled back
//          Assert( !pfcb->FFixedDDL() );
//          Assert( !FCOLUMNIDTemplateColumn( columnid ) );
            err = ErrERRCheck( JET_errColumnNotFound );
        }

        if ( fUseDMLLatch )
            pfcb->LeaveDML();
    }

    return err;
}

ERR ErrRECIRetrieveFixedColumn(
    FCB             * const pfcb,       // pass pfcbNil to bypass EnterDML()
    const TDB       *ptdb,
    const COLUMNID  columnid,
    const DATA&     dataRec,
    DATA            * pdataField,
    const FIELD     * const pfieldFixed )
{
    ERR             err;
    const FID       fid     = FidOfColumnid( columnid );

    Assert( ptdb != ptdbNil );
    Assert( pfcbNil == pfcb || pfcb->Ptdb() == ptdb );
    Assert( FCOLUMNIDFixed( columnid ) );
    Assert( !dataRec.FNull() );
    Assert( pdataField != NULL );

    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMostWithGlobalPageSize() );
    if ( dataRec.Cb() < REC::cbRecordMin || dataRec.Cb() > REC::CbRecordMostWithGlobalPageSize() )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }
    
    const REC   *prec = (REC *)dataRec.Pv();

    Assert( prec->FidFixedLastInRec().FFixedNone() || prec->FidFixedLastInRec().FFixed() );

#ifdef DEBUG
    const BOOL  fUseDMLLatchDBG     = ( pfcbNil != pfcb && fid > ptdb->FidFixedLastInitial() );
#else
    const BOOL  fUseDMLLatchDBG     = fFalse;
#endif

    if ( fUseDMLLatchDBG )
        pfcb->EnterDML();

    // RECIAccessColumn() should have already been called to verify FID.
    Assert( fid <= ptdb->FidFixedLast() );
    Assert( pfieldNil != ptdb->PfieldFixed( columnid ) );
    Assert( JET_coltypNil != ptdb->PfieldFixed( columnid )->coltyp );
    Assert( ptdb->PfieldFixed( columnid )->ibRecordOffset >= ibRECStartFixedColumns );
    Assert( ptdb->PfieldFixed( columnid )->ibRecordOffset < ptdb->IbEndFixedColumns() );
    Assert( !FFIELDUserDefinedDefault( ptdb->PfieldFixed( columnid )->ffield ) );

    // Don't forget to LeaveDML() before exitting this function.

    //  column not represented in record, retrieve from default
    //  or null column.
    //
    if ( fid > prec->FidFixedLastInRec() )
    {
        // if DEBUG, then EnterDML() was already done at the top of this function
        const BOOL  fUseDMLLatch    = ( !fUseDMLLatchDBG
                                        && pfcbNil != pfcb
                                        && fid > ptdb->FidFixedLastInitial() );

        if ( fUseDMLLatch )
            pfcb->EnterDML();

        //  assert no infinite recursion
        Assert( dataRec.Pv() != ptdb->PdataDefaultRecord() );

        //  if default value set, then retrieve default
        //
        if ( FFIELDDefault( ptdb->PfieldFixed( columnid )->ffield ) )
        {
            err = ErrRECIRetrieveFixedDefaultValue( ptdb, columnid, pdataField );
        }
        else
        {
            pdataField->Nullify();
            err = ErrERRCheck( JET_wrnColumnNull );
        }

        if ( fUseDMLLatch || fUseDMLLatchDBG )
            pfcb->LeaveDML();

        return err;
    }

    Assert( prec->FidFixedLastInRec().FFixed() );
    Assert( ptdb->PfieldFixed( columnid )->ibRecordOffset < prec->IbEndOfFixedData() );

    // check nullity

    const UINT  ifid            = fid.IndexOf( fidtypFixed );
    const BYTE  *prgbitNullity  = prec->PbFixedNullBitMap() + ifid/8;

    //  bit is not set: column is NULL
    //
    if ( FFixedNullBit( prgbitNullity, ifid ) )
    {
        pdataField->Nullify();
        err = ErrERRCheck( JET_wrnColumnNull );

    }
    else
    {
        // if DEBUG, then EnterDML() was already done at the top of this function
        const BOOL  fUseDMLLatch    = ( !fUseDMLLatchDBG
                                        && pfcbNil != pfcb
                                        && pfieldNil == pfieldFixed
                                        && fid > ptdb->FidFixedLastInitial() );

        if ( fUseDMLLatch )
            pfcb->EnterDML();

        //  set output parameter to length and address of column
        //
        const FIELD * const pfield = ( pfieldNil != pfieldFixed ? pfieldFixed : ptdb->PfieldFixed( columnid ) );
        Assert( pfield->cbMaxLen == UlCATColumnSize( pfield->coltyp, pfield->cbMaxLen, NULL ) );
        pdataField->SetCb( pfield->cbMaxLen );
        pdataField->SetPv( (BYTE *)prec + pfield->ibRecordOffset );

        if ( fUseDMLLatch )
            pfcb->LeaveDML();

        err = JET_errSuccess;
    }


    if ( fUseDMLLatchDBG )
        pfcb->LeaveDML();

    return err;
}


ERR ErrRECIRetrieveVarColumn(
    FCB             * const pfcb,       // pass pfcbNil to bypass EnterDML()
    const TDB       * ptdb,
    const COLUMNID  columnid,
    const DATA&     dataRec,
    DATA            * pdataField )
{
    const FID       fid         = FidOfColumnid( columnid );

    Assert( ptdbNil != ptdb );
    Assert( pfcbNil == pfcb || pfcb->Ptdb() == ptdb );
    Assert( FCOLUMNIDVar( columnid ) );
    Assert( !dataRec.FNull() );
    Assert( pdataField != NULL );

    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMostWithGlobalPageSize() );
    if ( dataRec.Cb() < REC::cbRecordMin || dataRec.Cb() > REC::CbRecordMostWithGlobalPageSize() )
    {
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    const REC   *prec = (REC *)dataRec.Pv();

    Assert( prec->FidVarLastInRec().FVarNone() || prec->FidVarLastInRec().FVar() );

#ifdef DEBUG
    const BOOL  fUseDMLLatchDBG     = ( pfcbNil != pfcb && fid > ptdb->FidVarLastInitial() );
#else
    const BOOL  fUseDMLLatchDBG     = fFalse;
#endif

    if ( fUseDMLLatchDBG )
        pfcb->EnterDML();

    // RECIAccessColumn() should have already been called to verify FID.
    Assert( fid <= ptdb->FidVarLast() );
    Assert( JET_coltypNil != ptdb->PfieldVar( columnid )->coltyp );
    Assert( !FFIELDUserDefinedDefault( ptdb->PfieldVar( columnid )->ffield ) );

    //  column not represented in record: column is NULL
    //
    if ( fid > prec->FidVarLastInRec() )
    {
        // if DEBUG, then EnterDML() was already done at the top of this function
        ERR         err;
        const BOOL  fUseDMLLatch    = ( !fUseDMLLatchDBG
                                        && pfcbNil != pfcb
                                        && fid > ptdb->FidVarLastInitial() );

        if ( fUseDMLLatch )
            pfcb->EnterDML();

        //  assert no infinite recursion
        Assert( dataRec.Pv() != ptdb->PdataDefaultRecord() );

        //  if default value set, then retrieve default
        //
        if ( FFIELDDefault( ptdb->PfieldVar( columnid )->ffield ) )
        {
            err = ErrRECIRetrieveVarDefaultValue( ptdb, columnid, pdataField );
        }
        else
        {
            pdataField->Nullify();
            err = ErrERRCheck( JET_wrnColumnNull );
        }

        if ( fUseDMLLatch || fUseDMLLatchDBG )
            pfcb->LeaveDML();

        return err;
    }

    if ( fUseDMLLatchDBG )
        pfcb->LeaveDML();

    Assert( prec->FidVarLastInRec().FVar() );

    UnalignedLittleEndian<REC::VAROFFSET>   *pibVarOffs     = prec->PibVarOffsets();

    //  adjust fid to an index
    //
    const UINT              ifid            = fid.IndexOf( fidtypVar );

    //  beginning of current column is end of previous column
    const REC::VAROFFSET    ibStartOfColumn = prec->IbVarOffsetStart( fid );

    Assert( IbVarOffset( pibVarOffs[ifid] ) == prec->IbVarOffsetEnd( fid ) );
    Assert( IbVarOffset( pibVarOffs[ifid] ) >= ibStartOfColumn );

    //  column is set to Null
    //
    if ( FVarNullBit( pibVarOffs[ifid] ) )
    {
        Assert( IbVarOffset( pibVarOffs[ifid] ) - ibStartOfColumn == 0 );
        pdataField->Nullify();
        return ErrERRCheck( JET_wrnColumnNull );
    }

    pdataField->SetCb( IbVarOffset( pibVarOffs[ifid] ) - ibStartOfColumn );
    Assert( pdataField->Cb() < dataRec.Cb() );

    if ( pdataField->Cb() == 0 )
    {
        // length is zero: return success [zero-length non-null
        // values are allowed]
        pdataField->SetPv( NULL );
    }
    else
    {
        //  set output parameter: column address
        //
        BYTE    *pbVarData = prec->PbVarData();
        Assert( pbVarData + IbVarOffset( pibVarOffs[ prec->FidVarLastInRec().IndexOf( fidtypVar ) ] )
                    <= (BYTE *)dataRec.Pv() + dataRec.Cb() );
        pdataField->SetPv( pbVarData + ibStartOfColumn );
        Assert( pdataField->Pv() >= (BYTE *)prec );
        Assert( pdataField->Pv() <= (BYTE *)prec + dataRec.Cb() );
    }

    return JET_errSuccess;
}


ERR ErrRECIRetrieveTaggedColumn(
    FCB             * pfcb,
    const COLUMNID  columnid,
    const ULONG     itagSequence,
    const DATA&     dataRec,
    DATA            * const pdataRetrieveBuffer,
    const ULONG     grbit )
{
    BOOL            fUseDerivedBit      = fFalse;
    Assert( !( grbit & grbitRetrieveColumnUseDerivedBit ) );    //  fDerived check is made below

    Assert( pfcb != pfcbNil );
    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( itagSequence > 0 );
    Assert( !dataRec.FNull() );
    Assert( NULL != pdataRetrieveBuffer );

    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfcb->Ifmp() ].CbPage() ) );
    if ( dataRec.Cb() > REC::CbRecordMost( pfcb ) )
    {
        FireWall( "RECIRetrieveTaggedColumnsRecTooBig14.2" );
    }
    if ( dataRec.Cb() < REC::cbRecordMin || dataRec.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfcb->Ifmp() ].CbPage() ) )
    {
        FireWall( "RECIRetrieveTaggedColumnsRecTooBig14.1" );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( !pfcb->FTemplateTable() )
        {
            pfcb->Ptdb()->AssertValidDerivedTable();

            //  HACK: treat derived columns in original-format derived table as
            //  non-derived, because they don't have the fDerived bit set in the TAGFLD
            fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

            // switch to template table
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
        }
        else
        {
            pfcb->Ptdb()->AssertValidTemplateTable();
            Assert( !fUseDerivedBit );
        }
    }
    else
    {
        Assert( !pfcb->FTemplateTable() );
    }

    TAGFIELDS   tagfields( dataRec );
    return tagfields.ErrRetrieveColumn(
                pfcb,
                columnid,
                itagSequence,
                dataRec,
                pdataRetrieveBuffer,
                grbit | ( fUseDerivedBit ? grbitRetrieveColumnUseDerivedBit : 0  ) );
}

INLINE ULONG UlRECICountTaggedColumnInstances(
    FCB             * pfcb,
    const COLUMNID  columnid,
    const DATA&     dataRec )
{
    BOOL            fUseDerivedBit      = fFalse;

    Assert( FCOLUMNIDTagged( columnid ) );
    Assert( !dataRec.FNull() );
    Assert( dataRec.Cb() >= REC::cbRecordMin );
    Assert( dataRec.Cb() <= REC::CbRecordMostCHECK( g_rgfmp[ pfcb->Ifmp() ].CbPage() ) );
    if ( dataRec.Cb() > REC::CbRecordMost( pfcb ) )
    {
        FireWall( "RECICountTaggedColInstsRecTooBig15.2" );
    }
    if ( dataRec.Cb() < REC::cbRecordMin || dataRec.Cb() > REC::CbRecordMostCHECK( g_rgfmp[ pfcb->Ifmp() ].CbPage() ) )
    {
        FireWall( "RECICountTaggedColInstsRecTooBig15.1" );
        return ErrERRCheck( JET_errDatabaseCorrupted );
    }

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( !pfcb->FTemplateTable() )
        {
            pfcb->Ptdb()->AssertValidDerivedTable();

            //  HACK: treat derived columns in original-format derived table as
            //  non-derived, because they don't have the fDerived bit set in the TAGFLD
            fUseDerivedBit = FRECUseDerivedBitForTemplateColumnInDerivedTable( columnid, pfcb->Ptdb() );

            // switch to template table
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
        }
        else
        {
            pfcb->Ptdb()->AssertValidTemplateTable();
            Assert( !fUseDerivedBit );
        }
    }
    else
    {
        Assert( !pfcb->FTemplateTable() );
    }

    TAGFIELDS   tagfields( dataRec );
    return tagfields.UlColumnInstances(
                pfcb,
                columnid,
                fUseDerivedBit );
}


ERR ErrRECIRetrieveSeparatedLongValue(
    FUCB        *pfucb,
    const DATA& dataField,
    BOOL        fAfterImage,
    ULONG       ibLVOffset,
    const BOOL  fEncrypted,
    VOID        *pv,
    ULONG       cbMax,
    ULONG       *pcbActual,
    JET_GRBIT   grbit )
{
    Assert( NULL != pv || 0 == cbMax );

    const LvId  lid         = LidOfSeparatedLV( dataField );
    ERR         err;

    ULONG       cbActual;
    QWORD       cbDataLogical = 0;
    QWORD       cbDataPhysical = 0;
    QWORD       cbOverhead = 0;

    Assert( FFUCBIndex( pfucb ) );      // Sorts don't have separated LV's.

    if ( grbit & JET_bitRetrieveLongId )
    {
        Assert( ibLVOffset == 0 );

        // We will start returning LID64 on new builds if the client asks for it by passing in a 64-bit or larger buffer.
        // The move to LID32 -> LID64 on this interface is analogus to changing any runtime structure
        // (e.g. JET_THREADSTATS3 -> JET_THREADSTATS4). It depends on the cb passed in (rather than the current efv).
        // Passing in cb == 4, will always give out LID32. We will error out if we have a LID64 to return.
        // Passing in cb >= 8, will always return 8-byte LIDs, whether its a LID64 or a LID32.
        // Mixed LIDs is an implementation detail that we won't expose.

        bool fRetLid64 = ( cbMax >= sizeof( _LID64 ) );
        if ( !fRetLid64 && cbMax < (ULONG) lid.Cb() )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }

        cbActual = fRetLid64 ? sizeof( _LID64 ) : sizeof( _LID32 );
        Assert( fRetLid64 || lid.FIsLid32() );
        _LID64 value = lid;
        UtilMemCpy( pv, (BYTE *)&value, cbActual );
        err = ErrERRCheck( JET_wrnSeparateLongValue );
    }
    else
    {
        //  must release any latch held
        //
        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }
        AssertDIRNoLatch( pfucb->ppib );

        if ( grbit & JET_bitRetrieveLongValueRefCount )
        {
            Assert( 0 == ibLVOffset );

            Call( ErrRECRetrieveSLongFieldRefCount(
                    pfucb,
                    lid,
                    reinterpret_cast<BYTE *>( pv ),
                    cbMax,
                    &cbActual ) );
            err = ErrERRCheck( JET_wrnSeparateLongValue );
        }
        else if ( grbit & JET_bitRetrievePrereadOnly )
        {
            Call( ErrRECRetrieveSLongFieldPrereadOnly( pfucb, lid, ibLVOffset, cbMax, fFalse, grbit ) );
            cbActual = 0;
        }
        else if ( grbit & JET_bitRetrievePhysicalSize )
        {
            Assert( NULL != pcbActual );
            Assert( 0 == ibLVOffset );
            Call( ErrRECGetLVSize( pfucb, lid, fFalse, &cbDataLogical, &cbDataPhysical, &cbOverhead ) );
            Assert( cbDataPhysical <= JET_cbLVColumnMost );
            cbActual = (ULONG)cbDataPhysical;
        }
        else
        {
            Call( ErrRECRetrieveSLongField( pfucb,
                    lid,
                    fAfterImage,
                    ibLVOffset,
                    fEncrypted,
                    reinterpret_cast<BYTE *>( pv ),
                    cbMax,
                    &cbActual ) );
            CallS( err );           // Don't expect any warnings.

            if ( cbActual > cbMax )
                err = ErrERRCheck( JET_wrnBufferTruncated );
        }
    }

    if ( pcbActual )
        *pcbActual = cbActual;

HandleError:
    return err;
}


COLUMNID ColumnidRECFirstTaggedForScanOfDerivedTable( const TDB * const ptdb )
{
    COLUMNID    columnidT;

    ptdb->AssertValidDerivedTable();
    const TDB   * ptdbTemplate  = ptdb->PfcbTemplateTable()->Ptdb();

    if ( ptdb->FESE97DerivedTable()
        && 0 != ptdbTemplate->FidTaggedLastOfESE97Template() )
    {
        if ( ptdbTemplate->FidTaggedLast() > ptdbTemplate->FidTaggedLastOfESE97Template() )
        {
            //  HACK: scan starts with first ESE98 template column
            columnidT = ColumnidOfFid(
                            FID( ptdbTemplate->FidTaggedLastOfESE97Template() + 1 ),
                            fTrue );
        }
        else
        {
            //  no ESE98 template columns, go to first ESE97 template column (at least one
            //  must exist)
            Assert( ptdbTemplate->FidTaggedLast() >= ptdbTemplate->FidTaggedFirst() );
            columnidT = ColumnidOfFid( ptdbTemplate->FidTaggedFirst(), fTrue );
        }
    }
    else
    {
        //  since no ESE97 tagged columns in template, template and derived tables
        //  must both start numbering at same place
        Assert( ptdbTemplate->FidTaggedFirst() == ptdb->FidTaggedFirst() );
        if ( ptdbTemplate->FidTaggedLast().FTagged() )
        {
            columnidT = ColumnidOfFid( ptdbTemplate->FidTaggedFirst(), fTrue );
        }
        else
        {
            Assert(  ptdbTemplate->FidTaggedLast().FTaggedNone() );
            //  no template columns, go to derived columns
            columnidT = ColumnidOfFid( ptdb->FidTaggedFirst(), fFalse );
        }
    }

    return columnidT;
}

COLUMNID ColumnidRECNextTaggedForScanOfDerivedTable( const TDB * const ptdb, const COLUMNID columnid )
{
    COLUMNID    columnidT   = columnid + 1;

    Assert( FCOLUMNIDTemplateColumn( columnidT ) );

    ptdb->AssertValidDerivedTable();
    const TDB   * const ptdbTemplate    = ptdb->PfcbTemplateTable()->Ptdb();

    Assert( FidOfColumnid( columnid ) <= ptdbTemplate->FidTaggedLast() );
    if ( ptdb->FESE97DerivedTable()
        && 0 != ptdbTemplate->FidTaggedLastOfESE97Template() )
    {
        Assert( ptdbTemplate->FidTaggedLastOfESE97Template() <= ptdbTemplate->FidTaggedLast() );
        if ( FidOfColumnid( columnidT ) == ptdbTemplate->FidTaggedLastOfESE97Template() + 1 )
        {
            Assert( ptdbTemplate->FidTaggedLastOfESE97Template() + 1 == ptdb->FidTaggedFirst() );
            columnidT = ColumnidOfFid( ptdb->FidTaggedFirst(), fFalse );
        }
        else if ( FidOfColumnid( columnidT ) > ptdbTemplate->FidTaggedLast() )
        {
            //  move to ESE97 template column space (at least one must exist)
            Assert( ptdbTemplate->FidTaggedLast() >= ptdbTemplate->FidTaggedFirst() );
            columnidT = ColumnidOfFid( ptdbTemplate->FidTaggedFirst(), fTrue );
        }
        else
        {
            //  still in template table column space, so do nothing special
        }
    }
    else if ( FidOfColumnid( columnidT ) > ptdbTemplate->FidTaggedLast() )
    {
        //  move to derived table column space
        columnidT = ColumnidOfFid( ptdb->FidTaggedFirst(), fFalse );
    }
    else
    {
        //  still in template table column space, so do nothing special
    }

    return columnidT;
}


LOCAL ERR ErrRECIScanTaggedColumns(
            FUCB            *pfucb,
            const ULONG     itagSequence,
            const DATA&     dataRec,
            DATA            *pdataField,
            COLUMNID        *pcolumnidRetrieved,
            ULONG           *pitagSequenceRetrieved,
            BOOL            *pfEncrypted,
            JET_GRBIT       grbit )
{
    TAGFIELDS tagfields( dataRec );
    const ERR err = tagfields.ErrScan(
                pfucb,
                itagSequence,
                dataRec,
                pdataField,
                pcolumnidRetrieved,
                pitagSequenceRetrieved,
                pfEncrypted,
                grbit );

    if ( ( err >= JET_errSuccess ) &&
        ( !( grbit & ( JET_bitRetrievePhysicalSize | JET_bitRetrieveLongId | JET_bitRetrieveLongValueRefCount ) ) &&
             *pfEncrypted && pfucb->pbEncryptionKey == NULL ) )
    {
        return ErrERRCheck( JET_errColumnNoEncryptionKey );
    }

    return err;
}


JET_COLTYP ColtypFromColumnid( FUCB *pfucb, const COLUMNID columnid )
{
    FCB         *pfcbTable;
    TDB         *ptdb;
    FIELD       *pfield;
    JET_COLTYP  coltyp;

    pfcbTable = pfucb->u.pfcb;
    ptdb = pfcbTable->Ptdb();

    pfcbTable->EnterDML();

    if ( FCOLUMNIDTagged( columnid ) )
    {
        pfield = ptdb->PfieldTagged( columnid );
    }
    else if ( FCOLUMNIDFixed( columnid ) )
    {
        pfield = ptdb->PfieldFixed( columnid );
        Assert( !FRECLongValue( pfield->coltyp ) );
    }
    else
    {
        Assert( FCOLUMNIDVar( columnid ) );
        pfield = ptdb->PfieldVar( columnid );
        Assert( !FRECLongValue( pfield->coltyp ) );
    }

    coltyp = pfield->coltyp;

    pfcbTable->LeaveDML();

    return coltyp;
}

LOCAL ERR ErrRECIRetrieveFromIndex(
    FUCB        *pfucb,
    COLUMNID    columnid,
    ULONG       *pitagSequence,
    BYTE        *pb,
    ULONG       cbMax,
    ULONG       *pcbActual,
    ULONG       ibGraphic,
    JET_GRBIT   grbit )
{
    ERR             err;
    FUCB            *pfucbIdx;
    FCB             *pfcbTable;
    TDB             *ptdb;
    IDB             *pidb;
    BOOL            fInitialIndex;
    BOOL            fDerivedIndex;
    BOOL            fLatched                = fFalse;
    BOOL            fText                   = fFalse;
    BOOL            fLongValue              = fFalse;
    BOOL            fBinaryChunks           = fFalse;
    BOOL            fMultiValued            = fFalse;
    BOOL            fVarSegMac              = fFalse;
    BOOL            fSawUnicodeTextColumn   = fFalse;
    INT             iidxseg;
    DATA            dataColumn;
    BOOL            fRetrieveFromPrimaryBM;
    ULONG           cbKeyMost;
    const size_t    cbKeyStack              = 256;
    BYTE            rgbKeyStack[ cbKeyStack ];
    BYTE            *pbKeyRes               = NULL;
    BYTE            *pbKeyAlloc             = NULL;
    KEY             keyT;
    KEY             *pkey;
    FIELD           *pfield;
    const IDXSEG*   rgidxseg;
    ULONG           iidxsegT;
    ULONG           ichOffset;
    ULONG           ulTuple;

    Assert( NULL != pitagSequence );
    AssertDIRNoLatch( pfucb->ppib );

    //  caller checks for these grbits
    Assert( grbit & ( JET_bitRetrieveFromIndex|JET_bitRetrieveFromPrimaryBookmark ) );

    //  RetrieveFromIndex and RetrieveFromPrimaryBookmark are mutually exclusive
    if ( ( grbit & JET_bitRetrieveFromIndex )
        && ( grbit & JET_bitRetrieveFromPrimaryBookmark ) )
        return ErrERRCheck( JET_errInvalidGrbit );

    if ( !FCOLUMNIDValid( columnid ) )
        return ErrERRCheck( JET_errBadColumnId );

    //  if on primary index, then return code indicating that
    //  retrieve should be from record.  Note, sequential files
    //  having no indexes, will be natually handled this way.
    //
    if ( pfucbNil == pfucb->pfucbCurIndex )
    {
        pfucbIdx = pfucb;

        Assert( pfcbNil != pfucb->u.pfcb );
        Assert( pfucb->u.pfcb->FTypeTable()
            || pfucb->u.pfcb->FTypeTemporaryTable()
            || pfucb->u.pfcb->FTypeSort() );

        Assert( ( pfucb->u.pfcb->FSequentialIndex() && pidbNil == pfucb->u.pfcb->Pidb() )
            || ( !pfucb->u.pfcb->FSequentialIndex() && pidbNil != pfucb->u.pfcb->Pidb() ) );

        //  if currency is not locOnCurBM, we have to go to the record anyway,
        //      so it's pointless to continue
        //  for a sequential index, must go to the record
        //  for a sort, data is always cached anyway so just retrieve normally
        if ( locOnCurBM != pfucb->locLogical
            || pfucb->u.pfcb->FSequentialIndex()
            || FFUCBSort( pfucb ) )
        {
            //  can't have multi-valued column in a primary index,
            //  so itagSequence must be 1
            *pitagSequence = 1;
            return ErrERRCheck( errDIRNoShortCircuit );
        }

        Assert( pidbNil != pfucbIdx->u.pfcb->Pidb() );
        Assert( pfucbIdx->u.pfcb->Pidb()->FPrimary() );

        //  force RetrieveFromIndex, strip off RetrieveTag
        grbit = JET_bitRetrieveFromIndex;
        fRetrieveFromPrimaryBM = fFalse;
    }
    else
    {
        pfucbIdx = pfucb->pfucbCurIndex;
        Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );
        Assert( pidbNil != pfucbIdx->u.pfcb->Pidb() );
        Assert( !pfucbIdx->u.pfcb->Pidb()->FPrimary() );
        fRetrieveFromPrimaryBM = ( grbit & JET_bitRetrieveFromPrimaryBookmark );
    }

    //  find index segment for given column id
    //
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbNil != pfcbTable );
    Assert( pfcbTable->Ptdb() != ptdbNil );

    if ( fRetrieveFromPrimaryBM )
    {
        Assert( !pfucbIdx->u.pfcb->Pidb()->FPrimary() );
        if ( pfcbTable->FSequentialIndex() )
        {
            //  no columns in a sequential index
            Assert( pidbNil == pfcbTable->Pidb() );
            return ErrERRCheck( JET_errColumnNotFound );
        }

        pidb = pfcbTable->Pidb();
        Assert( pidbNil != pidb );

        fInitialIndex = pfcbTable->FInitialIndex();
        fDerivedIndex = pfcbTable->FDerivedIndex();
    }
    else
    {
        const FCB * const   pfcbIdx     = pfucbIdx->u.pfcb;

        pidb = pfcbIdx->Pidb();
        fInitialIndex = pfcbIdx->FInitialIndex();
        fDerivedIndex = pfcbIdx->FDerivedIndex();
    }

    //  pidb should already be pointing to appropriate index
    //
    cbKeyMost = pidb->CbKeyMost();

    if ( fDerivedIndex )
    {
        Assert( pidb->FTemplateIndex() || pidb->FIDBOwnedByFCB() );
        Assert( pfcbTable->Ptdb() != ptdbNil );
        pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
        Assert( pfcbNil != pfcbTable );
        Assert( pfcbTable->FTemplateTable() );
        Assert( pfcbTable->Ptdb() != ptdbNil );
    }

    ptdb = pfcbTable->Ptdb();

    const BOOL      fUseDMLLatch    = ( !fInitialIndex
                                        || pidb->FIsRgidxsegInMempool() );

    if ( fUseDMLLatch )
        pfcbTable->EnterDML();

    rgidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
    {
        const COLUMNID  columnidT   = rgidxseg[iidxseg].Columnid();
        if ( columnidT == columnid )
        {
            break;
        }

        //  We need to know if there is a Unicode column before the column we want to 
        //  retrieve.
        //  If there is a Unicode column before the column we want to retrieve then we can't
        //  retrieve the column from the normalized key
        const FIELD * const pfieldT = ptdb->Pfield( columnidT );
        if( FRECTextColumn( pfieldT->coltyp ) && usUniCodePage == pfieldT->cp )
        {
            fSawUnicodeTextColumn = fTrue;
        }
    }
    Assert( iidxseg <= pidb->Cidxseg() );
    if ( iidxseg == pidb->Cidxseg() )
    {
        if ( fUseDMLLatch )
            pfcbTable->LeaveDML();
        return ErrERRCheck( JET_errColumnNotFound );
    }

    //  if retrieving column (and not tuple) from tuple index,
    //  then fail if column is tuplized.
    //
    if ( pidb->FTuples()
        && ( 0 == iidxseg ) /* only first column of tuple index is tuplized */
        && ( 0 == ( grbit & JET_bitRetrieveTuple ) ) )
    {
        if ( fUseDMLLatch )
            pfcbTable->LeaveDML();
        return ErrERRCheck( JET_errIndexTuplesCannotRetrieveFromIndex );
    }

    Assert( pidb->CbVarSegMac() > 0 );
    Assert( pidb->CbVarSegMac() <= cbKeyMost );
    fVarSegMac = ( pidb->CbVarSegMac() < cbKeyMost );

    // Since the column belongs to an active index, we are guaranteed
    // that the column is accessible.
    if ( FCOLUMNIDTagged( columnid ) )
    {
        pfield = ptdb->PfieldTagged( columnid );
        fLongValue = FRECLongValue( pfield->coltyp );
        fBinaryChunks = FRECBinaryColumn( pfield->coltyp );

        Assert( !FFIELDMultivalued( pfield->ffield )
            || ( pidb->FMultivalued() && !pidb->FPrimary() ) );     //  primary index can't be over multi-valued column
        fMultiValued = FFIELDMultivalued( pfield->ffield );
    }
    else if ( FCOLUMNIDFixed( columnid ) )
    {
        pfield = ptdb->PfieldFixed( columnid );
        Assert( !FRECLongValue( pfield->coltyp ) );
    }
    else
    {
        Assert( FCOLUMNIDVar( columnid ) );
        pfield = ptdb->PfieldVar( columnid );
        Assert( !FRECLongValue( pfield->coltyp ) );
        fBinaryChunks = FRECBinaryColumn( pfield->coltyp );
    }

    fText = FRECTextColumn( pfield->coltyp );
    Assert( pfield->cp != usUniCodePage || fText );     // Can't be Unicode unless it's text.

    if ( fUseDMLLatch )
        pfcbTable->LeaveDML();

    //  if not locOnCurBM, must go to record
    //
    //  if locOnCurBM and retrieving from the index, we can just retrieve from bmCurr
    //
    //  if locOnCurBM and retrieveing from the primary bookmark we can use the bmCurr
    //  of the FUCB of the table -- unless the table is not locOnCurBM. The table can
    //  not be on locCurBM if (iff?) we call JetSetCurrentIndex( JET_bitMoveFirst ) and
    //  then call JetRetrieveColumn() -- the FUCB of the index will be updated, but not the
    //  table. We could optimize this case by noting that on a non-unique secondary index
    //  we could get the primary key from the data of the bmCurr of the pfucbIdx
    if ( locOnCurBM != pfucbIdx->locLogical
        || ( fRetrieveFromPrimaryBM && ( locOnCurBM != pfucb->locLogical ) ) )
    {
        CallR( ErrDIRGet( pfucbIdx ) );
        fLatched = fTrue;

        if ( fRetrieveFromPrimaryBM )
        {
            keyT.prefix.Nullify();
            keyT.suffix.SetCb( pfucbIdx->kdfCurr.data.Cb() );
            keyT.suffix.SetPv( pfucbIdx->kdfCurr.data.Pv() );
            pkey = &keyT;
        }
        else
        {
            pkey = &pfucbIdx->kdfCurr.key;
        }
    }
    else if ( fRetrieveFromPrimaryBM )
    {
        //  the key in the primary index is in sync with the secondary
        Assert( pfucbIdx != pfucb );
        Assert( locOnCurBM == pfucb->locLogical );
        pkey = &pfucb->bmCurr.key;
    }
    else
    {
        pkey = &pfucbIdx->bmCurr.key;
    }

    //  allocate key buffer
    //
    if ( cbKeyMost > cbKeyStack )
    {
        Alloc( pbKeyRes = (BYTE *)RESKEY.PvRESAlloc() );
        pbKeyAlloc = pbKeyRes;
    }
    else
    {
        pbKeyAlloc = rgbKeyStack;
    }
    dataColumn.SetPv( pbKeyAlloc );

    //  If there is a Unicode column before the column we want to retrieve then we can't
    //  reliably retrieve the column from the normalized key
    if( fSawUnicodeTextColumn )
    {
        err = ErrERRCheck( errDIRNoShortCircuit );
        goto ComputeItag;
    }
    
    //  If key is text, then can't de-normalise (due to case).
    //
    //  If key may have been truncated, then return code indicating
    //  that retrieve should be from record. This calculation is a
    //  bit tricky. The cbKeyMost stored in the IDB may be one of
    //  three values:
    //      a) A max key size specified by the user. This must be
    //         >= JET_cbKeyMostMin
    //      b) The default value of JET_cbKeyMostMin
    //      c) A max key size calculated from the index definition.
    //         This can be done if the normalized size of all columns
    //         is capped (e.g. if they are all fixed columns). In
    //         this case cbKeyMost will be < JET_cbKeyMostMin.
    //  So, to check for truncation we look for a case where (a) or (b)
    //  is true and the key size is the same as cbKeyMost. In that case
    //  we can't be sure if truncation has occured so errDIRNoShortCircuit
    //  is returned.
    //
    //  Note that since we
    //  always consume as much keyspace as possible, the key would
    //  only have been truncated if the size of the key is cbKeyMost.
    //
    //  If key is variable/tagged binary and cbVarSegMac specified,
    //  then key may also have been truncated.
    Assert( (ULONG)pkey->Cb() <= cbKeyMost );
    if ( fText
        || ((ULONG)pkey->Cb() == cbKeyMost && cbKeyMost >= JET_cbKeyMostMin)
        || ( fBinaryChunks && fVarSegMac ) )
    {
        err = ErrERRCheck( errDIRNoShortCircuit );
        goto ComputeItag;
    }

    OSTraceFMP(
        pfucbIdx->ifmp,
        JET_tracetagDMLRead,
        OSFormat(
            "Session=[0x%p:0x%x] retrieving index column from objid=[0x%x:0x%x] [columnid=0x%x]",
            pfucb->ppib,
            ( ppibNil != pfucbIdx->ppib ? pfucbIdx->ppib->trxBegin0 : trxMax ),
            (ULONG)pfucbIdx->ifmp,
            pfucbIdx->u.pfcb->ObjidFDP(),
            columnid ) );

    pfcbTable->EnterDML();
    Assert( pfcbTable->Ptdb() == ptdb );
    err = ErrRECIRetrieveColumnFromKey( ptdb, pidb, pkey, columnid, &dataColumn );
    pfcbTable->LeaveDML();
    CallSx( err, JET_wrnColumnNull );
    Call( err );

    Assert( locOnCurBM == pfucbIdx->locLogical );
    if ( fLatched )
    {
        AssertDIRGet( pfucbIdx );
    }

    //  if long value then effect offset
    //
    if ( fLongValue )
    {
        if ( ibGraphic >= (ULONG)dataColumn.Cb() )
        {
            dataColumn.SetCb( 0 );
        }
        else
        {
            dataColumn.DeltaPv( ibGraphic );
            dataColumn.DeltaCb( 0 - ibGraphic );
        }
    }

    //  set return values
    //
    if ( pcbActual )
        *pcbActual = dataColumn.Cb();

    if ( 0 == dataColumn.Cb() )
    {
        //  either NULL, zero-length, or
        //  LV with ibOffset out-of-range
        CallSx( err, JET_wrnColumnNull );
    }
    else
    {
        ULONG   cbReturned;
        if ( (ULONG)dataColumn.Cb() <= cbMax )
        {
            cbReturned = dataColumn.Cb();
            CallS( err );
        }
        else
        {
            cbReturned = cbMax;
            err = ErrERRCheck( JET_wrnBufferTruncated );
        }

        UtilMemCpy( pb, dataColumn.Pv(), (size_t)cbReturned );
    }

ComputeItag:
    if ( errDIRNoShortCircuit == err || ( grbit & JET_bitRetrieveTag ) )
    {
        ERR             errT            = err;
        KeySequence     ksT( pfucb, pidb );

        Assert( JET_errSuccess == err
            || JET_wrnColumnNull == err
            || JET_wrnBufferTruncated == err
            || errDIRNoShortCircuit == err );

        //  Retrieve keys from record and compare against current key
        //  to compute itag for tagged column instance, responsible for
        //  this index key.
        //  If column is NULL, then there must only be one itagsequence.
        //
        if ( fMultiValued && JET_wrnColumnNull != err )
        {

            keyT.prefix.Nullify();
            keyT.suffix.SetCb( cbKeyMost );
            keyT.suffix.SetPv( pbKeyAlloc );

            if ( fLatched )
            {
                CallR( ErrDIRRelease( pfucbIdx ) );
                fLatched = fFalse;
            }

            Assert( ksT.FBaseKey() );
            for ( ; !ksT.FSequenceComplete(); ksT.Increment(iidxsegT) )
            {
                Call( ErrRECRetrieveKeyFromRecord(
                                pfucb,
                                pidb,
                                &keyT,
                                ksT.Rgitag(),
                                0,
                                fFalse,
                                &iidxsegT ) );
                CallS( ErrRECValidIndexKeyWarning( err ) );
                Assert( wrnFLDOutOfTuples != err );
                Assert( wrnFLDNotPresentInIndex != err );
                if ( wrnFLDOutOfKeys == err )
                {
                    continue;
                }

                Assert( !fLatched );
                Call( ErrDIRGet( pfucbIdx ) );
                fLatched = fTrue;

                if ( FKeysEqual( pfucbIdx->kdfCurr.key, keyT ) )
                    break;

                Assert( fLatched );
                Call( ErrDIRRelease( pfucbIdx ) );
                fLatched = fFalse;

                if ( pidb->FTuples() )
                {
                    for ( ichOffset = pidb->IchTuplesStart() + pidb->CchTuplesIncrement(), ulTuple = 1; ulTuple < pidb->IchTuplesToIndexMax(); ichOffset += pidb->CchTuplesIncrement(), ulTuple++ )
                    {
                        CallR( ErrRECRetrieveKeyFromRecord(
                                    pfucb,
                                    pidb,
                                    &keyT,
                                    ksT.Rgitag(),
                                    ichOffset,
                                    fFalse,
                                    &iidxsegT ) );

                        //  all other warnings should have been detected
                        //  when we retrieved with ichOffset==0
                        CallSx( err, wrnFLDOutOfTuples );
                        if ( JET_errSuccess != err )
                            break;

                        Assert( !fLatched );
                        Call( ErrDIRGet( pfucbIdx ) );
                        fLatched = fTrue;

                        if ( FKeysEqual( pfucbIdx->kdfCurr.key, keyT ) )
                            goto Found;

                        Assert( fLatched );
                        Call( ErrDIRRelease( pfucbIdx ) );
                        fLatched = fFalse;
                    }
                }

            }
        }

        //  loop should not end on out of keys or index corrupt
        //
        if ( ksT.FSequenceComplete() )
        {
            //  index entry cannot be formulated from primary record
            //
            Assert( wrnFLDOutOfKeys == err );
            OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagCorruption, L"823ac3a5-1237-4706-8d1f-85d945697154" );
            err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
            goto HandleError;
        }

Found:
        *pitagSequence = pidb->FMultivalued() ? ksT.ItagFromColumnid(columnid) : 1;

        err = errT;
    }

HandleError:
    RESKEY.Free( pbKeyRes );

    if ( fLatched )
    {
        Assert( Pcsr( pfucbIdx )->FLatched() );
        CallS( ErrDIRRelease( pfucbIdx ) );
    }
    AssertDIRNoLatch( pfucbIdx->ppib );
    return err;
}


//  ================================================================
INLINE ERR ErrRECAdjustEscrowedColumn(
    FUCB *          pfucb,
    const COLUMNID  columnid,
    const FIELD&    fieldFixed,
    VOID *          pv,
    const ULONG     cb )
//  ================================================================
//
//  if this is an escrowed column, get the compensating delta from the version
//  store and apply it to the buffer
//
//-
{
    //  UNDONE: remove columnid param because it's only
    //  used for DEBUG purposes
    Assert( 0 != columnid );
    Assert( FCOLUMNIDFixed( columnid ) );

    if ( fieldFixed.cbMaxLen != cb )
        return ErrERRCheck( JET_errInvalidBufferSize );

    if ( fieldFixed.cbMaxLen == sizeof( LONG ) )
    {
        const LONG  lDelta = DeltaVERGetDelta< LONG >( pfucb, pfucb->bmCurr, fieldFixed.ibRecordOffset );
        *(LONG *) pv += lDelta;
    }
    else if ( fieldFixed.cbMaxLen == sizeof( LONGLONG ) )
    {
        const LONGLONG  llDelta = DeltaVERGetDelta< LONGLONG >( pfucb, pfucb->bmCurr, fieldFixed.ibRecordOffset );
        *(LONGLONG *) pv += llDelta;
    }
    else
    {
        Assert( fFalse );
        return ErrERRCheck( JET_errInvalidOperation );
    }

    return JET_errSuccess;
}

//  ================================================================
LOCAL ERR ErrRECIDecompressDataForRetrieve(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    const INT ibOffset,
    __out_bcount_part_opt( cbDataMax, *pcbDataActual ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual )
//  ================================================================
//
//  Decompress data, dealing with the ibOffset
//
//-
{
    ERR err;
    
    if( 0 == ibOffset )
    {
        Call( ErrPKDecompressData(
                dataCompressed,
                pfucb,
                pbData,
                cbDataMax,
                pcbDataActual ) );
    }
    else
    {
        // we are retrieving compressed data, starting at a non-zero offset
        // we need to allocate a temporary buffer, decompress into
        // the buffer and then copy the desired data out. we can't
        // start decompressing in the middle of the blob, we have
        // to start at the beginning.
        //
        INT cbDataActualT;
        Call( ErrPKDecompressData(
                dataCompressed,
                pfucb,
                NULL,
                0,
                &cbDataActualT ) );
        *pcbDataActual = max( 0, cbDataActualT - ibOffset );
        if( pbData && cbDataMax )
        {
            const INT cbToCopy      = min(cbDataMax, *pcbDataActual);
            const INT cbToRetrieve  = cbToCopy + ibOffset;
            BYTE * pbT = NULL;
            Alloc( pbT = new BYTE[cbToRetrieve] );
            // don't use Call() here, pbT won't be freed
            err = ErrPKDecompressData(
                    dataCompressed,
                    pfucb,
                    pbT,
                    cbToRetrieve,
                    &cbDataActualT );
            UtilMemCpy( pbData, pbT+ibOffset, cbToCopy );
            delete[] pbT;
            Call( err );

            if( *pcbDataActual > cbDataMax )
            {
                err = ErrERRCheck( JET_wrnBufferTruncated );
            }
        }
    }
HandleError:
    
#pragma prefast( suppress : 26030, "In case of JET_wrnBufferTruncated, we return what the buffer size should be." )
    return err;
}

//  return the amount of data that can be set into a given column without requiring 
//  any of the existing intrinsic record data to be separated out to another database page, e.g. in the long tree.
//  This routine only operates on a copy buffer.  Errors are returned if the copy buffer is not set,
//  or if the given column instance is already separated from the intrinsic record.
//
INLINE ERR ErrRECIGetIntrinsicAvail(
    FUCB                *pfucb,
    JET_COLUMNID        columnid,
    JET_RETINFO         *pretinfo,
    ULONG               *pulIntrinsicAvail )
{
    if ( !FFUCBUpdatePrepared( pfucb ) )
    {
        return ErrERRCheck( JET_errUpdateNotPrepared );
    }

    FCB                 * const pfcbTable = pfucb->u.pfcb;

    pfcbTable->EnterDML();

    ERR                 err = JET_errSuccess;
    TDB                 *ptdbT = pfcbTable->Ptdb();
    FIELD               *pfieldT = NULL;
    const DATA&         dataRec = pfucb->dataWorkBuf;
    const REC           *precT = (REC *)dataRec.Pv();
    Assert( 0 != columnid );
    const FID           fidT = FidOfColumnid( columnid );

    //  compute copy buffer intrinsic available space
    //
    Assert( REC::CbRecordMost( pfucb ) >= dataRec.Cb() );
    LONG                cbIntrinsicAvail = (LONG)REC::CbRecordMost( pfucb ) - (LONG)dataRec.Cb();
    
    //  if fixed column then cbIntrinsicAvail is either 0 or column size depending on whether setting column will exceed copy buffer space or not
    //
    if ( FCOLUMNIDFixed( columnid ) )
    {
        //  columnid should already have been validated
        Assert( fidT >= ptdbT->FidFixedFirst() );
        Assert( fidT <= ptdbT->FidFixedLast() );
        Assert( precT->FidFixedLastInRec().FFixed() );
        pfieldT = ptdbT->PfieldFixed( columnid );
        //  if setting fixed field does not require any space, or if the space required is available then return the full size of the fixed column.
        //  Otherwise, return 0.  With fixed columns, its all or nothing.
        //
        if ( ( pfieldT->ibRecordOffset < precT->IbEndOfFixedData() )
            || ( ( (LONG)pfieldT->ibRecordOffset + (LONG)pfieldT->cbMaxLen - (LONG)precT->IbEndOfFixedData() ) < cbIntrinsicAvail ) )
        {
            //  since there is room for this fixed length column, return full size of fixed length column.  Note that not more than this can be set.
            //
            cbIntrinsicAvail = pfieldT->cbMaxLen;
        }
        else
        {
            cbIntrinsicAvail = 0;
        }
    }
    //  if variable or tagged column then cbIntrinsicAvail is remaining copy buffer space minus per column overhead
    //  
    else
    {
        Assert( !FCOLUMNIDFixed( columnid ) );
        if ( FCOLUMNIDVar( columnid ) )
        {
            Assert( fidT >= ptdbT->FidVarFirst() );
            Assert( fidT <= ptdbT->FidVarLast() );
            //  pfieldT not used for variable columns
            //  pfieldT = ptdbT->PfieldVar( columnid );

            //  adjust cbIntrinsicAvail down for variable column overhead, including preceding columns that have never been set
            //
            if ( fidT > precT->FidVarLastInRec() )
            {
                //  setting this variable column will implicitly consume one WORD 
                //  for each of the variable columns between the last set in this record and the column id of this column.
                //
                cbIntrinsicAvail -= sizeof(WORD)*(fidT - precT->FidVarLastInRec());
            }
        }
        //  else must be a tagged column.  cbIntrinsicAvail is remaining copy buffer space minus per column overhead.
        //
        else
        {
            Assert( FCOLUMNIDTagged( columnid ) );
            pfieldT = ptdbT->PfieldTagged( columnid );
            
            //  check for NULL value and separated long value
            //
            DATA        dataT;
            dataT.SetCb(0);
            dataT.SetPv(NULL);
            TAGFIELDS   tagfields( dataRec );
            Call( tagfields.ErrRetrieveColumn(
                        pfcbTable,
                        columnid,
                        ( pretinfo == NULL ) ? 1 : pretinfo->itagSequence,
                        dataRec,
                        &dataT,
                        JET_bitRetrieveIgnoreDefault ) );

            //  if column is NULL then adjust cbIntrinsicAvail for tagged column overhead
            //
            if ( JET_wrnColumnNull == err )
            {
                cbIntrinsicAvail -= sizeof(TAGFLD);
            }

            //  long values have one byte overhead for intrinsic/separate flag
            //
            if ( FRECLongValue( pfieldT->coltyp ) )
            {
                //  check for separate LV
                //
                if ( JET_wrnSeparateLongValue == err )
                {
                    err = ErrERRCheck( JET_errSeparatedLongValue );
                    goto HandleError;
                }

                //  adjust cbIntrinsicAvail for per column LV overhead
                //
                cbIntrinsicAvail -= sizeof(BYTE);
            }
        }
    }

    //  cbIntrinsicAvail cannot be negative
    //
    if ( cbIntrinsicAvail < 0 )
    {
        *pulIntrinsicAvail = 0;
    }
    else
    {
        *pulIntrinsicAvail = (ULONG)cbIntrinsicAvail;
    }
    err = JET_errSuccess;
    
HandleError:
    pfcbTable->LeaveDML();
    return err;
}

ERR VTAPI ErrIsamRetrieveColumn(
    _In_ JET_SESID                                                      sesid,
    _In_ JET_VTID                                                       vtid,
    _In_ JET_COLUMNID                                                   columnid,
    _Out_writes_bytes_to_opt_( cbMax, min( cbMax, *pcbActual ) ) VOID*  pv,
    _In_ const ULONG                                                    cbMax,
    _Out_opt_ ULONG*                                                    pcbActual,
    _In_ JET_GRBIT                                                      grbit,
    _Inout_opt_ JET_RETINFO*                                            pretinfo )
{
    ERR             err;
    PIB*            ppib                = reinterpret_cast<PIB *>( sesid );
    FUCB*           pfucb               = reinterpret_cast<FUCB *>( vtid );
    DATA*           pdataRec;
    DATA            dataRetrieved;
    ULONG           itagSequence;
    ULONG           ibLVOffset;
    FIELD           fieldFixed;
    BOOL            fScanTagged         = fFalse;
    BOOL            fTransactionStarted = fFalse;
    BOOL            fSetReturnValue     = fTrue;
    BOOL            fUseCopyBuffer      = fFalse;
    BOOL            fEncrypted          = fFalse;
    BYTE *          pbDataDecrypted     = NULL;
    BYTE *          pbRef               = NULL;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRMaybeNoLatch( ppib, pfucb );

    //  set ptdb.  ptdb is same for indexes and for sorts.
    //
    Assert( pfucb->u.pfcb->Ptdb() == pfucb->u.pscb->fcb.Ptdb() );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

    if ( pretinfo != NULL )
    {
        if ( pretinfo->cbStruct < sizeof(JET_RETINFO) )
            return ErrERRCheck( JET_errInvalidParameter );
        ibLVOffset = pretinfo->ibLongValue;
        itagSequence = pretinfo->itagSequence;
    }
    else
    {
        itagSequence = 1;
        ibLVOffset = 0;
    }

    if ( grbit & grbitRetrieveColumnInternalFlagsMask )
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    if ( pfucb->cbstat == fCBSTATInsertReadOnlyCopy )
    {
        grbit = grbit & ~( JET_bitRetrieveFromIndex | JET_bitRetrieveFromPrimaryBookmark );
    }

    if ( ppib->Level() == 0 )
    {
        // Begin transaction for read consistency (in case page
        // latch must be released in order to validate column).
        CallR( ErrDIRBeginTransaction( ppib, 61733, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    AssertDIRMaybeNoLatch( ppib, pfucb );

    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

    //  if the page number then just retrieve it
    if ( grbit & JET_bitRetrievePageNumber )
    {
        if ( JET_bitRetrievePageNumber != grbit )
        {
            // no other options are supported
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }
        if ( !FFUCBIndex( pfucb ) )
        {
            // this doesn't work on sorts
            Call( ErrERRCheck( JET_errInvalidTableId ) );
        }
        
        *pcbActual = sizeof(PGNO);
        if ( !pv || sizeof(PGNO) > cbMax )
        {
            Call( ErrERRCheck( JET_errBufferTooSmall ) );
        }
        Call( ErrDIRGet( pfucb ) );
        *((PGNO *)pv) = pfucb->csr.Pgno();
        Call( ErrDIRRelease( pfucb ) );
        err = JET_errSuccess;
        goto HandleError;
    }
    
    //  if retrieving copy buffer intrinsic space available for column then actual must be size of int
    if ( grbit & JET_bitRetrieveCopyIntrinsic )
    {
        if ( JET_bitRetrieveCopyIntrinsic != grbit )
        {
            // no other options are supported
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }
        if ( !FFUCBIndex( pfucb ) )
        {
            // this doesn't work on sorts
            Call( ErrERRCheck( JET_errInvalidTableId ) );
        }
        
        *pcbActual = sizeof(ULONG);
        if ( !pv || sizeof(ULONG) > cbMax )
        {
            Call( ErrERRCheck( JET_errBufferTooSmall ) );
        }
        err = ErrRECIGetIntrinsicAvail( pfucb, columnid, pretinfo, (ULONG *)pv );
        goto HandleError;
    }
    
    //  if retrieving compressed size for column then cannot expect data to be returned
    //
    if ( grbit & JET_bitRetrievePhysicalSize )
    {
        //  only other option supported with retrieve physical size is whether to operate on the copy buffer or row
        //
        if ( grbit & ~(JET_bitRetrievePhysicalSize|JET_bitRetrieveCopy) )
        {
            //  incompatible options
            //
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }
        
        //  only information returned is in pcbActual, and no ibLVOffset since latter is logical concept
        //
        if ( cbMax || !pcbActual || ibLVOffset )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    //  can only retrieve tuple if also retreving from index
    //
    if ( ( grbit & JET_bitRetrieveTuple ) && ( 0 == ( grbit & JET_bitRetrieveFromIndex ) ) )
    {
        Call( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    //  do not allow retrieve as ref if the storage isn't stable
    //
    if (    ( grbit & JET_bitRetrieveAsRefIfNotInRecord ) &&
            ( FFUCBUpdatePrepared( pfucb ) || FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    if ( grbit & ( JET_bitRetrieveFromIndex|JET_bitRetrieveFromPrimaryBookmark ) )
    {
        if ( FFUCBAlwaysRetrieveCopy( pfucb )
            || FFUCBNeverRetrieveCopy( pfucb ) )
        {
            //  insde a callback, so cannot use RetrieveFromIndex/PrimaryBookmark
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        err = ErrRECIRetrieveFromIndex(
                    pfucb,
                    columnid,
                    &itagSequence,
                    reinterpret_cast<BYTE *>( pv ),
                    cbMax,
                    pcbActual,
                    ibLVOffset,
                    grbit );

        //  return itagSequence if requested
        //
        if ( pretinfo != NULL
            && ( grbit & JET_bitRetrieveTag )
            && ( errDIRNoShortCircuit == err || err >= 0 ) )
        {
            pretinfo->itagSequence = itagSequence;
        }

        if ( err != errDIRNoShortCircuit )
        {
            goto HandleError;
        }
    }

    AssertDIRMaybeNoLatch( ppib, pfucb );

    fieldFixed.ffield = 0;
    fieldFixed.ibRecordOffset = 0;
    if ( 0 != columnid )
    {
        Assert( !fScanTagged );
        Call( ErrRECIAccessColumn( pfucb, columnid, &fieldFixed, &fEncrypted ) );
        if ( !( grbit & (JET_bitRetrievePhysicalSize|JET_bitRetrieveLongId|JET_bitRetrieveLongValueRefCount) ) &&
             fEncrypted && pfucb->pbEncryptionKey == NULL )
        {
            Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
        }
        AssertDIRMaybeNoLatch( ppib, pfucb );
    }
    else
    {
        fScanTagged = fTrue;
    }

    fUseCopyBuffer = ( ( ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
                        || FFUCBAlwaysRetrieveCopy( pfucb ) );

    Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagDMLRead,
        OSFormat(
            "Session=[0x%p:0x%x] retrieving column from objid=[0x%x:0x%x] [columnid=0x%x,itag=0x%x,ib=0x%x,copy=%c,grbit=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            columnid,
            itagSequence,
            ibLVOffset,
            ( fUseCopyBuffer ? 'Y' : 'N' ),
            grbit ) );

    if ( fScanTagged )
    {
        const ULONG icolumnToRetrieve = itagSequence;

        Assert( 0 == columnid );
        if ( 0 == itagSequence )
        {
            //  must use RetrieveColumns() in order to count columns
            err = ErrERRCheck( JET_errBadItagSequence );
            goto HandleError;
        }

        Call( ErrRECIScanTaggedColumns(
                pfucb,
                icolumnToRetrieve,
                *pdataRec,
                &dataRetrieved,
                &columnid,
                &itagSequence,
                &fEncrypted,
                grbit ) );
        Assert( 0 != columnid || JET_wrnColumnNull == err );
    }
    else if ( FCOLUMNIDTagged( columnid ) )
    {
        Call( ErrRECRetrieveTaggedColumn(
                pfucb->u.pfcb,
                columnid,
                itagSequence,
                *pdataRec,
                &dataRetrieved,
                grbit ) );
    }
    else
    {
        Call( ErrRECRetrieveNonTaggedColumn(
                pfucb->u.pfcb,
                columnid,
                *pdataRec,
                &dataRetrieved,
                &fieldFixed ) );
    }

    // Some grbits are unsupported for intrinsic LVs
    if ( ( grbit & (JET_bitRetrieveLongId|JET_bitRetrieveLongValueRefCount) ) &&
         err != wrnRECSeparatedLV )
    {
        if ( pcbActual )
            *pcbActual = 0;
        err = ErrERRCheck( JET_wrnColumnNull );
        goto HandleError;
    }

    if ( fEncrypted &&
         ( err == wrnRECCompressed || err == wrnRECIntrinsicLV ) &&
         !( grbit & JET_bitRetrievePhysicalSize ) &&
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

    if ( wrnRECUserDefinedDefault == err )
    {
        Assert( FCOLUMNIDTagged( columnid ) );
        Assert( dataRetrieved.Cb() == 0 );

        if ( Pcsr( pfucb )->FLatched() )
        {
            CallS( ErrDIRRelease( pfucb ) );
        }

        const BOOL fAlwaysRetrieveCopy  = FFUCBAlwaysRetrieveCopy( pfucb );
        const BOOL fNeverRetrieveCopy   = FFUCBNeverRetrieveCopy( pfucb );

        Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );

        if( fUseCopyBuffer )
        {
            FUCBSetAlwaysRetrieveCopy( pfucb );
        }
        else
        {
            FUCBSetNeverRetrieveCopy( pfucb );
        }

        *pcbActual = cbMax;
        err =  ErrRECCallback(
                    ppib,
                    pfucb,
                    JET_cbtypUserDefinedDefaultValue,
                    columnid,
                    pv,
                    (VOID *)pcbActual,
                    columnid );
        if( JET_errSuccess == err && *pcbActual > cbMax )
        {
            //  the callback function may not set this correctly.
            err = ErrERRCheck( JET_wrnBufferTruncated );
        }

        FUCBResetAlwaysRetrieveCopy( pfucb );
        FUCBResetNeverRetrieveCopy( pfucb );

        if( fAlwaysRetrieveCopy )
        {
            FUCBSetAlwaysRetrieveCopy( pfucb );
        }
        else if( fNeverRetrieveCopy )
        {
            FUCBSetNeverRetrieveCopy( pfucb );
        }

        Call( err );

        fSetReturnValue = fFalse;
    }
    else if ( wrnRECCompressed == err )
    {
        Assert( FCOLUMNIDTagged( columnid ) );
        Assert( dataRetrieved.Cb() > 0 );

        INT cbActual;

        //  if retrieving physical size then do not decompress data
        //
        if ( !( grbit & JET_bitRetrievePhysicalSize ) )
        {
            Call( ErrRECIDecompressDataForRetrieve(
                    dataRetrieved,
                    pfucb,
                    ibLVOffset,
                    (BYTE *)pv,
                    cbMax,
                    &cbActual ) );
        }
        else
        {
            Assert( 0 == ibLVOffset );
            cbActual = (ULONG)dataRetrieved.Cb();
            err = JET_errSuccess;
        }

        if ( pcbActual )
            *pcbActual = (ULONG)cbActual;

        fSetReturnValue = fFalse;
    }
    else
    {
        Assert( wrnRECLongField != err );       //  obsolete error code

        switch ( err )
        {
            case wrnRECSeparatedLV:
            {
                if ( grbit & JET_bitRetrieveAsRefIfNotInRecord )
                {
                    ULONG cbRef;
                    Call( ErrRECCreateColumnReference(  pfucb,
                                                        columnid,
                                                        itagSequence,
                                                        LidOfSeparatedLV( dataRetrieved ),
                                                        &cbRef,
                                                        &pbRef ));
                    dataRetrieved.SetPv( pbRef );
                    dataRetrieved.SetCb( cbRef );
                    err = ErrERRCheck( JET_wrnColumnReference );
                }
                else
                {
                    //  If we are retrieving an after-image or
                    //  haven't replaced a LV we can simply go
                    //  to the LV tree. Otherwise we have to
                    //  perform a more detailed consultation of
                    //  the version store with ErrRECGetLVImage
                    const BOOL fAfterImage = fUseCopyBuffer
                                            || !FFUCBUpdateSeparateLV( pfucb )
                                            || !FFUCBReplacePrepared( pfucb );

                    //  If JET_bitRetrievePrereadOnly, the only other valid grbit is JET_bitRetrievePrereadMany
                    //  Plus, JET_bitRetrievePrereadMany can't be specified without JET_bitRetrievePrereadOnly
                    //
                    if ( grbit & ( JET_bitRetrievePrereadOnly | JET_bitRetrievePrereadMany )
                        && grbit != JET_bitRetrievePrereadOnly
                        && grbit != ( JET_bitRetrievePrereadOnly | JET_bitRetrievePrereadMany ) )
                    {
                        Call( ErrERRCheck( JET_errInvalidGrbit ) );
                    }

                    Call( ErrRECIRetrieveSeparatedLongValue(
                            pfucb,
                            dataRetrieved,
                            fAfterImage,
                            ibLVOffset,
                            fEncrypted,
                            pv,
                            cbMax,
                            pcbActual,
                            grbit ) );
                    fSetReturnValue = fFalse;
                }
                break;
            }
            case wrnRECIntrinsicLV:
                Assert( !( grbit & JET_bitRetrievePhysicalSize ) || ibLVOffset == 0 );
                if ( ibLVOffset >= (ULONG)dataRetrieved.Cb() )
                    dataRetrieved.SetCb( 0 );
                else
                {
                    dataRetrieved.DeltaPv( ibLVOffset );
                    dataRetrieved.DeltaCb( 0 - ibLVOffset );
                }
                err = JET_errSuccess;
                break;
            case JET_wrnColumnSetNull:
                Assert( fScanTagged );
                break;
            default:
                CallSx( err, JET_wrnColumnNull );
        }
    }

    //  these should have been handled and mapped
    //  to an appropriate return value
    Assert( wrnRECUserDefinedDefault != err );
    Assert( wrnRECSeparatedLV != err );
    Assert( wrnRECIntrinsicLV != err );

    //** Set return values **
    if ( fSetReturnValue )
    {
        ULONG   cbCopy;
        BYTE    rgb[8];

        if ( 0 != columnid && dataRetrieved.Cb() <= 8 && dataRetrieved.Cb() && !FHostIsLittleEndian() )
        {
            Assert( dataRetrieved.Pv() );

            //  Depends on coltyp, we may need to reverse the bytes
            JET_COLTYP coltyp = ColtypFromColumnid( pfucb, columnid );
            if ( coltyp == JET_coltypShort
                || coltyp == JET_coltypUnsignedShort )
            {
                *(USHORT*)rgb = ReverseBytesOnBE( *(USHORT*)dataRetrieved.Pv() );
                dataRetrieved.SetPv( rgb );
            }
            else if ( coltyp == JET_coltypLong ||
                coltyp == JET_coltypUnsignedLong ||
                coltyp == JET_coltypIEEESingle )
            {
                *(ULONG*)rgb = ReverseBytesOnBE( *(ULONG*)dataRetrieved.Pv() );
                dataRetrieved.SetPv( rgb );
            }
            else if ( coltyp == JET_coltypLongLong ||
                coltyp == JET_coltypUnsignedLongLong ||
                coltyp == JET_coltypCurrency ||
                coltyp == JET_coltypIEEEDouble  ||
                coltyp == JET_coltypDateTime )
            {
                *(QWORD*)rgb = ReverseBytesOnBE( *(QWORD*)dataRetrieved.Pv() );
                dataRetrieved.SetPv( rgb );
            }
        }

        if ( pcbActual )
        {
            *pcbActual = dataRetrieved.Cb();
        }

        //  only retrieve data if not getting column physical size
        //
        if ( !( grbit & JET_bitRetrievePhysicalSize ) )
        {
            if ( (ULONG)dataRetrieved.Cb() <= cbMax )
            {
                cbCopy = dataRetrieved.Cb();
            }
            else
            {
                cbCopy = cbMax;
                err = ErrERRCheck( JET_wrnBufferTruncated );
            }

            if ( cbCopy )
            {
                UtilMemCpy( pv, dataRetrieved.Pv(), cbCopy );
            }
            
            //  if we are inserting a new record there can't be any versions on the
            //  escrow column. we also don't know the bookmark of the new record yet...
            //
            if ( FFIELDEscrowUpdate( fieldFixed.ffield ) && !FFUCBInsertPrepared( pfucb ) )
            {
                const ERR   errT    = ErrRECAdjustEscrowedColumn(
                                            pfucb,
                                            columnid,
                                            fieldFixed,
                                            pv,
                                            cbCopy );
                if ( errT < 0 )
                {
                    Call( errT );
                }
            }
        }
    }

    if ( pretinfo != NULL )
    {
        pretinfo->columnidNextTagged = columnid;
    }

HandleError:
    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }
    if ( pbRef )
    {
        delete[] pbRef;
    }

    if ( Pcsr( pfucb )->FLatched() )
    {
        ERR errT;
        Assert( !fUseCopyBuffer );
        errT = ErrDIRRelease( pfucb );
        CallSx( errT, JET_errOutOfMemory );
        if ( errT < JET_errSuccess && err >= JET_errSuccess )
        {
            err = errT;
        }
    }

    if ( fTransactionStarted )
    {
        //  No updates performed, so force success.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
    
    AssertDIRMaybeNoLatch( ppib, pfucb );
    return err;
}


INLINE VOID RECICountColumn(
    FCB             *pfcb,
    const COLUMNID  columnid,
    const DATA&     dataRec,
    ULONG           *pulNumOccurrences )
{
    Assert( 0 != columnid );

    if ( FCOLUMNIDTagged( columnid ) )
    {
        *pulNumOccurrences = UlRECICountTaggedColumnInstances(
                    pfcb,
                    columnid,
                    dataRec );
    }
    else
    {
        ERR     errT;
        BOOL    fNull;

        if ( FCOLUMNIDTemplateColumn( columnid ) && !pfcb->FTemplateTable() )
        {
            if ( !pfcb->FTemplateTable() )
            {
                pfcb->Ptdb()->AssertValidDerivedTable();
                pfcb = pfcb->Ptdb()->PfcbTemplateTable();
            }
            else
            {
                pfcb->Ptdb()->AssertValidTemplateTable();
            }
        }

        if ( FCOLUMNIDFixed( columnid ) )
        {
            //  columnid should already have been validated
            Assert( FidOfColumnid( columnid ) >= pfcb->Ptdb()->FidFixedFirst() );
            Assert( FidOfColumnid( columnid ) <= pfcb->Ptdb()->FidFixedLast() );

            errT = ErrRECIFixedColumnInRecord( columnid, pfcb, dataRec );

            if ( JET_errColumnNotFound == errT )
            {
                // Column not in record -- it's null if there's no default value.
                const TDB * const   ptdb    = pfcb->Ptdb();
                if ( FidOfColumnid( columnid ) > ptdb->FidFixedLastInitial() )
                {
                    pfcb->EnterDML();
                    fNull = !FFIELDDefault( ptdb->PfieldFixed( columnid )->ffield );
                    pfcb->LeaveDML();
                }
                else
                {
                    fNull = !FFIELDDefault( ptdb->PfieldFixed( columnid )->ffield );
                }
            }
            else
            {
                CallSx( errT, JET_wrnColumnNull );
                fNull = ( JET_wrnColumnNull == errT );
            }
        }
        else
        {
            Assert( FCOLUMNIDVar( columnid ) );

            //  columnid should already have been validated
            Assert( FidOfColumnid( columnid ) >= pfcb->Ptdb()->FidVarFirst() );
            Assert( FidOfColumnid( columnid ) <= pfcb->Ptdb()->FidVarLast() );

            errT = ErrRECIVarColumnInRecord( columnid, pfcb, dataRec );

            if ( JET_errColumnNotFound == errT )
            {
                // Column not in record -- it's null if there's no default value.
                const TDB * const   ptdb    = pfcb->Ptdb();
                if ( FidOfColumnid( columnid ) > ptdb->FidVarLastInitial() )
                {
                    pfcb->EnterDML();
                    fNull = !FFIELDDefault( ptdb->PfieldVar( columnid )->ffield );
                    pfcb->LeaveDML();
                }
                else
                {
                    fNull = !FFIELDDefault( ptdb->PfieldVar( columnid )->ffield );
                }
            }
            else
            {
                CallSx( errT, JET_wrnColumnNull );
                fNull = ( JET_wrnColumnNull == errT );
            }
        }

        *pulNumOccurrences = ( fNull ? 0 : 1 );
    }
}

LOCAL ERR ErrRECRetrieveColumns(
    FUCB                *pfucb,
    JET_RETRIEVECOLUMN  *pretcol,
    ULONG               cretcol,
    BOOL                *pfBufferTruncated )
{
    ERR                 err;
    const DATA *        pdataRec        = NULL;
    BYTE *              pbDataDecrypted = NULL;
    BYTE *              pbRef           = NULL;

    Assert( !( FFUCBAlwaysRetrieveCopy( pfucb ) && FFUCBNeverRetrieveCopy( pfucb ) ) );

    //  set ptdb, ptdb is same for indexes and for sorts
    //
    Assert( pfucb->u.pfcb->Ptdb() == pfucb->u.pscb->fcb.Ptdb() );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );
    AssertDIRMaybeNoLatch( pfucb->ppib, pfucb );

    *pfBufferTruncated = fFalse;

    //  get current data
    //
    if ( FFUCBSort( pfucb ) )
    {
        //  sorts always have current data cached.
        if ( pfucb->locLogical != locOnCurBM )
        {
            Assert( locBeforeFirst == pfucb->locLogical
                || locAfterLast == pfucb->locLogical );
            Call( ErrERRCheck( JET_errNoCurrentRecord ) );
        }
        else
        {
            Assert( pfucb->kdfCurr.data.Cb() != 0 );
        }

        pdataRec = &pfucb->kdfCurr.data;
    }

    for ( ULONG i = 0; i < cretcol; i++ )
    {
        // efficiency variables
        //
        JET_RETRIEVECOLUMN  * pretcolT      = pretcol + i;
        JET_GRBIT           grbit           = pretcolT->grbit;
        COLUMNID            columnid        = pretcolT->columnid;
        FIELD               fieldFixed;
        DATA                dataRetrieved;
        BOOL                fSetReturnValue = fTrue;
        BOOL                fEncrypted = fFalse;

        fieldFixed.ffield = 0;
        fieldFixed.ibRecordOffset = 0;

        if ( grbit & grbitRetrieveColumnInternalFlagsMask )
        {
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        if ( pfucb->cbstat == fCBSTATInsertReadOnlyCopy )
        {
            grbit = grbit & ~( JET_bitRetrieveFromIndex | JET_bitRetrieveFromPrimaryBookmark );
        }

        //  if retrieving compressed size for column then cannot expect data to be returned
        //
        if ( grbit & JET_bitRetrievePhysicalSize )
        {
            //  only other option supported with retrieve physical size is whether to operate on the copy buffer or row
            //
            if ( grbit & ~(JET_bitRetrievePhysicalSize|JET_bitRetrieveCopy) )
            {
                //  incompatible options
                //
                Call( ErrERRCheck( JET_errInvalidGrbit ) );
            }
            
            //  no ibLongValue since that logical concept cannot be compbined with JET_bitRetrievePhysicalSize, and no cbData since no data returned
            //
            if ( pretcol->cbData || pretcol->ibLongValue )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }

        //  do not allow retrieve as ref if the storage isn't stable
        //
        if (    ( grbit & JET_bitRetrieveAsRefIfNotInRecord ) &&
                ( FFUCBUpdatePrepared( pfucb ) || FFUCBSort( pfucb ) || FFMPIsTempDB( pfucb->ifmp) ) )
        {
            Error( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        if ( FFUCBIndex( pfucb ) )
        {
            // UNDONE: No copy buffer with sorts, and don't currently support
            // secondary indexes with sorts, so RetrieveFromIndex reduces to
            // a record retrieval.
            if ( grbit & ( JET_bitRetrieveFromIndex|JET_bitRetrieveFromPrimaryBookmark ) )
            {
                if ( Pcsr( pfucb )->FLatched() )
                {
                    Call( ErrDIRRelease( pfucb ) );
                }

                if ( FFUCBAlwaysRetrieveCopy( pfucb )
                    || FFUCBNeverRetrieveCopy( pfucb ) )
                {
                    //  insde a callback, so cannot use RetrieveFromIndex/PrimaryBookmark
                    Call( ErrERRCheck( JET_errInvalidGrbit ) );
                }

                err = ErrRECIRetrieveFromIndex(
                            pfucb,
                            columnid,
                            &pretcolT->itagSequence,
                            reinterpret_cast<BYTE *>( pretcolT->pvData ),
                            pretcolT->cbData,
                            &pretcolT->cbActual,
                            pretcolT->ibLongValue,
                            grbit );
                        AssertDIRNoLatch( pfucb->ppib );

                if ( errDIRNoShortCircuit == err )
                {
                    //  UNDONE: this is EXTREMELY expensive because we
                    //  will go to the record and formulate all keys
                    //  for the record to try to determine the itagSequence
                    //  of the current index entry
                    Assert( pretcolT->itagSequence > 0 );
                }
                else
                {
                    if ( err < 0 )
                        goto HandleError;

                    pretcolT->columnidNextTagged = columnid;
                    pretcolT->err = err;
                    if ( JET_wrnBufferTruncated == err )
                        *pfBufferTruncated = fTrue;

                    continue;
                }
            }

            if ( ( ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
                || FFUCBAlwaysRetrieveCopy( pfucb ) )
            {
                //  if we obtained the latch on a previous pass,
                //  we don't relinquish it because we want to
                //  avoid excessive latching/unlatching
                Assert( !Pcsr( pfucb )->FLatched()
                    || !FFUCBAlwaysRetrieveCopy( pfucb ) );
                pdataRec = &pfucb->dataWorkBuf;
            }
            else
            {
                if ( !Pcsr( pfucb )->FLatched() )
                {
                    Call( ErrDIRGet( pfucb ) );
                }
                pdataRec = &pfucb->kdfCurr.data;
            }
        }
        else
        {
            Assert( FFUCBSort( pfucb ) );
            Assert( !Pcsr( pfucb )->FLatched() );
            Assert( pdataRec == &pfucb->kdfCurr.data );
            pdataRec = &pfucb->kdfCurr.data;  //  prefix doesn't understand that pdataRec is already valid here
        }

        ULONG   itagSequence    = pretcolT->itagSequence;

        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagDMLRead,
            OSFormat(
                "Session=[0x%p:0x%x] retrieving multiple columns from objid=[0x%x:0x%x] [columnid=0x%x,itag=0x%x,ib=0x%x,copy=%c,grbit=0x%x]",
                pfucb->ppib,
                ( ppibNil != pfucb->ppib ? pfucb->ppib->trxBegin0 : trxMax ),
                (ULONG)pfucb->ifmp,
                pfucb->u.pfcb->ObjidFDP(),
                columnid,
                itagSequence,
                pretcolT->ibLongValue,
                ( pdataRec == &pfucb->dataWorkBuf ? 'Y' : 'N' ),
                pretcolT->grbit ) );

        if ( 0 == columnid )
        {
            if ( 0 == itagSequence )        // Are we counting all tagged columns?
            {
                Call( ErrRECIScanTaggedColumns(
                        pfucb,
                        0,
                        *pdataRec,
                        &dataRetrieved,
                        &columnid,
                        &pretcolT->itagSequence,    // Store count in itagSequence
                        &fEncrypted,
                        grbit ) );
                Assert( 0 == columnid );
                Assert( JET_wrnColumnNull == err );
                pretcolT->cbActual = 0;
                pretcolT->columnidNextTagged = 0;
                pretcolT->err = JET_errSuccess;
                continue;
            }
            else
            {
                Call( ErrRECIScanTaggedColumns(
                        pfucb,
                        pretcolT->itagSequence,
                        *pdataRec,
                        &dataRetrieved,
                        &columnid,
                        &itagSequence,
                        &fEncrypted,
                        grbit ) );
                Assert( 0 != columnid || JET_wrnColumnNull == err );
            }
        }
        else
        {
            // Verify column is accessible and that we're in a transaction
            // (for read consistency).
            Assert( pfucb->ppib->Level() > 0 );
            Call( ErrRECIAccessColumn( pfucb, columnid, &fieldFixed, &fEncrypted ) );
            if ( !( grbit & (JET_bitRetrievePhysicalSize|JET_bitRetrieveLongId|JET_bitRetrieveLongValueRefCount) ) &&
                 fEncrypted && pfucb->pbEncryptionKey == NULL )
            {
                Error( ErrERRCheck( JET_errColumnNoEncryptionKey ) );
            }

            if ( 0 == itagSequence )
            {
                RECICountColumn(
                        pfucb->u.pfcb,
                        columnid,
                        *pdataRec,
                        &pretcolT->itagSequence );  // Store count in itagSequence
                pretcolT->cbActual = 0;
                pretcolT->columnidNextTagged = columnid;
                pretcolT->err = JET_errSuccess;
                continue;
            }
            else if ( FCOLUMNIDTagged( columnid ) )
            {
                Call( ErrRECRetrieveTaggedColumn(
                        pfucb->u.pfcb,
                        columnid,
                        itagSequence,
                        *pdataRec,
                        &dataRetrieved,
                        grbit ) );
            }
            else
            {
                Call( ErrRECRetrieveNonTaggedColumn(
                        pfucb->u.pfcb,
                        columnid,
                        *pdataRec,
                        &dataRetrieved,
                        &fieldFixed ) );
            }
        }

        // Some grbits are unsupported for intrinsic LVs
        if ( ( grbit & (JET_bitRetrieveLongId|JET_bitRetrieveLongValueRefCount) ) &&
             err != wrnRECSeparatedLV )
        {
            pretcolT->cbActual = 0;
            pretcolT->err = ErrERRCheck( JET_wrnColumnNull );
            continue;
        }

        if ( fEncrypted &&
             ( err == wrnRECCompressed || err == wrnRECIntrinsicLV ) &&
             !( grbit & JET_bitRetrievePhysicalSize ) &&
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

        if ( wrnRECUserDefinedDefault == err )
        {
            Assert( FCOLUMNIDTagged( columnid ) );
            Assert( dataRetrieved.Cb() == 0 );

            if ( Pcsr( pfucb )->FLatched() )
            {
                Call( ErrDIRRelease( pfucb ) );
            }

            Assert( pretcolT->columnid == columnid );

            VOID * const pvArg1 = pretcolT->pvData;
            pretcolT->cbActual = pretcolT->cbData;
            VOID * const pvArg2 = &(pretcolT->cbActual);

            const BOOL fAlwaysRetrieveCopy  = FFUCBAlwaysRetrieveCopy( pfucb );
            const BOOL fNeverRetrieveCopy   = FFUCBNeverRetrieveCopy( pfucb );

            Assert( !( fAlwaysRetrieveCopy && fNeverRetrieveCopy ) );

            if( ( ( grbit & JET_bitRetrieveCopy) || FFUCBAlwaysRetrieveCopy( pfucb ) )
                && FFUCBUpdatePrepared( pfucb ) )
            {
                FUCBSetAlwaysRetrieveCopy( pfucb );
            }
            else
            {
                FUCBSetNeverRetrieveCopy( pfucb );
            }

            err = ErrRECCallback(
                        pfucb->ppib,
                        pfucb,
                        JET_cbtypUserDefinedDefaultValue,
                        columnid,
                        pvArg1,
                        pvArg2,
                        columnid );
            if( JET_errSuccess == err && pretcolT->cbActual > pretcolT->cbData )
            {
                //  the callback function may not set this correctly.
                err = ErrERRCheck( JET_wrnBufferTruncated );
            }

            FUCBResetAlwaysRetrieveCopy( pfucb );
            FUCBResetNeverRetrieveCopy( pfucb );

            if( fAlwaysRetrieveCopy )
            {
                FUCBSetAlwaysRetrieveCopy( pfucb );
            }
            else if ( fNeverRetrieveCopy )
            {
                FUCBSetNeverRetrieveCopy( pfucb );
            }

            Call( err );

            pretcolT->err = err;
            fSetReturnValue = fFalse;

            Assert( !Pcsr( pfucb )->FLatched() );
        }
        else if ( wrnRECCompressed == err )
        {
            Assert( FCOLUMNIDTagged( columnid ) );
            Assert( dataRetrieved.Cb() > 0 );

            INT cbActual = 0;

            //  if retrieving physical size then do not decompress data
            //
            if ( !( grbit & JET_bitRetrievePhysicalSize ) )
            {
                Call( ErrRECIDecompressDataForRetrieve(
                        dataRetrieved,
                        pfucb,
                        pretcolT->ibLongValue,
                        (BYTE *)(pretcolT->pvData),
                        pretcolT->cbData,
                        &cbActual ) );
            }
            else
            {
                Assert( 0 == pretcolT->ibLongValue );
                cbActual = (ULONG)dataRetrieved.Cb();
                err = JET_errSuccess;
            }

            pretcolT->err = err;
            pretcolT->cbActual = (ULONG)cbActual;

            fSetReturnValue = fFalse;
        }
        else
        {
            Assert( wrnRECLongField != err );       //  obsolete error code
            Assert( !(grbit & JET_bitRetrievePhysicalSize ) || pretcol->ibLongValue == 0 );

            switch ( err )
            {
                case wrnRECSeparatedLV:
                {
                    if ( grbit & JET_bitRetrieveAsRefIfNotInRecord )
                    {
                        ULONG cbRef;
                        Call( ErrRECCreateColumnReference(  pfucb,
                                                            columnid,
                                                            itagSequence,
                                                            LidOfSeparatedLV( dataRetrieved ),
                                                            &cbRef,
                                                            &pbRef ) );
                        dataRetrieved.SetPv( pbRef );
                        dataRetrieved.SetCb( cbRef );
                        err = ErrERRCheck( JET_wrnColumnReference );
                    }
                    else
                    {
                        //  If we are retrieving a copy, go ahead
                        //  and do a normal retrieval. Otherwise
                        //  we have to consult the version store
                        const BOOL fRetrieveBeforeImage =
                            ( FFUCBNeverRetrieveCopy( pfucb ) || !( grbit & JET_bitRetrieveCopy ) )
                            && FFUCBReplacePrepared( pfucb )
                            && FFUCBUpdateSeparateLV( pfucb )
                            && !FFUCBAlwaysRetrieveCopy( pfucb );

                        Call( ErrRECIRetrieveSeparatedLongValue(
                                    pfucb,
                                    dataRetrieved,
                                    !fRetrieveBeforeImage,
                                    pretcolT->ibLongValue,
                                    fEncrypted,
                                    pretcolT->pvData,
                                    pretcolT->cbData,
                                    &pretcolT->cbActual,
                                    grbit ) );
                        pretcolT->err = err;
                        fSetReturnValue = fFalse;
                }
                    break;
                }
                case wrnRECIntrinsicLV:
                    if ( pretcolT->ibLongValue >= (ULONG)dataRetrieved.Cb() )
                        dataRetrieved.SetCb( 0 );
                    else
                    {
                        dataRetrieved.DeltaPv( pretcolT->ibLongValue );
                        dataRetrieved.DeltaCb( 0 - pretcolT->ibLongValue );
                    }
                    err = JET_errSuccess;
                    break;
                case JET_wrnColumnSetNull:
                    Assert( 0 == pretcolT->columnid );
                    break;
                default:
                    CallSx( err, JET_wrnColumnNull );
            }
        }

        //  these should have been handled and mapped
        //  to an appropriate return value
        Assert( wrnRECUserDefinedDefault != err );
        Assert( wrnRECSeparatedLV != err );
        Assert( wrnRECIntrinsicLV != err );

        if ( fSetReturnValue )
        {
            ULONG   cbCopy;
            BYTE    rgb[8];

            if ( 0 != columnid && dataRetrieved.Cb() <= 8 && dataRetrieved.Cb() && !FHostIsLittleEndian() )
            {
                Assert( dataRetrieved.Pv() );

                //  Depends on coltyp, we may need to reverse the bytes
                JET_COLTYP coltyp = ColtypFromColumnid( pfucb, columnid );
                if ( coltyp == JET_coltypShort
                    || coltyp == JET_coltypUnsignedShort )
                {
                    *(USHORT*)rgb = ReverseBytesOnBE( *(USHORT*)dataRetrieved.Pv() );
                    dataRetrieved.SetPv( rgb );
                }
                else if ( coltyp == JET_coltypLong
                    ||coltyp == JET_coltypUnsignedLong
                    ||coltyp == JET_coltypIEEESingle )
                {
                    *(ULONG*)rgb = ReverseBytesOnBE( *(ULONG*)dataRetrieved.Pv() );
                    dataRetrieved.SetPv( rgb );
                }
                else if ( coltyp == JET_coltypLongLong
                    ||coltyp == JET_coltypUnsignedLongLong
                    ||coltyp == JET_coltypCurrency
                    ||coltyp == JET_coltypIEEEDouble
                    ||coltyp == JET_coltypDateTime )
                {
                    *(QWORD*)rgb = ReverseBytesOnBE( *(QWORD*)dataRetrieved.Pv() );
                    dataRetrieved.SetPv( rgb );
                }
            }

            pretcolT->cbActual = dataRetrieved.Cb();


            //  only retrieve data if not getting column physical size
            //
            if ( grbit & JET_bitRetrievePhysicalSize )
            {
                pretcolT->err = err;
            }
            else
            {
                if ( (ULONG)dataRetrieved.Cb() <= pretcolT->cbData )
                {
                    pretcolT->err = err;
                    cbCopy = dataRetrieved.Cb();
                }
                else
                {
                    pretcolT->err = ErrERRCheck( JET_wrnBufferTruncated );
                    cbCopy = pretcolT->cbData;
                }

                UtilMemCpy( pretcolT->pvData, dataRetrieved.Pv(), cbCopy );

                //  if we are inserting a new record there can't be any versions on the
                //  escrow column. we also don't know the bookmark of the new record yet...
                if ( FFIELDEscrowUpdate( fieldFixed.ffield ) && !FFUCBInsertPrepared( pfucb ) )
                {
                    const ERR   errT    = ErrRECAdjustEscrowedColumn(
                                                pfucb,
                                                columnid,
                                                fieldFixed,
                                                pretcolT->pvData,
                                                cbCopy );
                    //  only assign to err if there was an error
                    if ( errT < 0 )
                    {
                        Call( errT );
                    }
                }
            }
        }

        if ( pbDataDecrypted )
        {
            delete[] pbDataDecrypted;
            pbDataDecrypted = NULL;
        }
        if ( pbRef )
        {
            delete[] pbRef;
            pbRef = NULL;
        }

        pretcolT->columnidNextTagged = columnid;

        if ( JET_wrnBufferTruncated == pretcolT->err )
            *pfBufferTruncated = fTrue;

        Assert( pretcolT->err != JET_errNullInvalid );
    }   // for

    err = JET_errSuccess;

HandleError:
    if ( pbDataDecrypted )
    {
        delete[] pbDataDecrypted;
        pbDataDecrypted = NULL;
    }
    if ( pbRef )
    {
        delete[] pbRef;
    }

    if ( Pcsr( pfucb )->FLatched() )
    {
        CallS( ErrDIRRelease( pfucb ) );
    }

    AssertDIRMaybeNoLatch( pfucb->ppib, pfucb );

    return err;
}

ERR VTAPI ErrIsamRetrieveColumns(
    JET_SESID               vsesid,
    JET_VTID                vtid,
    JET_RETRIEVECOLUMN      *pretcol,
    ULONG                   cretcol )
{
    ERR                     err;
    PIB                     *ppib               = (PIB *)vsesid;
    FUCB                    *pfucb              = (FUCB *)vtid;
    BOOL                    fBufferTruncated;
    BOOL                    fTransactionStarted = fFalse;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRMaybeNoLatch( ppib, pfucb );

    if ( 0 == ppib->Level() )
    {
        // Begin transaction for read consistency (in case page
        // latch must be released in order to validate column).
        Call( ErrDIRBeginTransaction( ppib, 37157, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    AssertDIRMaybeNoLatch( ppib, pfucb );
    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

    Call( ErrRECRetrieveColumns(
                pfucb,
                pretcol,
                cretcol,
                &fBufferTruncated ) );
    if ( fBufferTruncated )
        err = ErrERRCheck( JET_wrnBufferTruncated );

HandleError:
    if ( fTransactionStarted )
    {
        //  No updates performed, so force success.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }
    AssertDIRMaybeNoLatch( ppib, pfucb );

    return err;
}


INLINE VOID RECIAddTaggedColumnListEntry(
    TAGCOLINFO                  * const ptagcolinfo,
    const TAGFIELDS_ITERATOR    * const piterator,
    const TDB                   * const ptdb,
    const BOOL                  fDefaultValue )
{
    const BOOL          fDerived    = ( piterator->FDerived()
                                        || ( ptdb->FESE97DerivedTable()
                                            && piterator->Fid() < ptdb->FidTaggedFirst() ) );

    ptagcolinfo->columnid           = piterator->Columnid( ptdb );
    ptagcolinfo->cMultiValues       = static_cast<USHORT>( piterator->TagfldIterator().Ctags() );
    ptagcolinfo->usFlags            = 0;
    ptagcolinfo->fLongValue         = USHORT( piterator->FLV() ? fTrue : fFalse );
    ptagcolinfo->fDefaultValue      = USHORT( fDefaultValue ? fTrue : fFalse );
    ptagcolinfo->fNullOverride      = USHORT( piterator->FNull() ? fTrue : fFalse );
    ptagcolinfo->fDerived           = USHORT( fDerived ? fTrue : fFalse );
}


LOCAL ERR ErrRECIBuildTaggedColumnList(
    FUCB                * const pfucb,
    const DATA&         dataRec,
    TAGCOLINFO          * const rgtagcolinfo,
    ULONG               * const pcentries,
    const ULONG         centriesMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit )
{
    ERR                 err = JET_errSuccess;

    FCB                 * const pfcb        = pfucb->u.pfcb;
    TDB                 * const ptdb        = pfcb->Ptdb();

    const BOOL          fCountOnly          = ( NULL == rgtagcolinfo );
    const BOOL          fRetrieveDefaults   = ( !( grbit & JET_bitRetrieveIgnoreDefault )
                                                && ptdb->FTableHasNonEscrowDefault() );

    ULONG               centriesCurr        = 0;

    TAGFIELDS_ITERATOR * precordIterator        = NULL;
    TAGFIELDS_ITERATOR * pdefaultValuesIterator = NULL;


    FID                 fidRecordFID        = 0;
    FID                 fidDefaultFID       = 0;
    
    //  initialize return values

    *pcentries = 0;

    //  create the iterators

    precordIterator = new TAGFIELDS_ITERATOR( dataRec );
    if( NULL == precordIterator )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
        goto HandleError;
    }
    precordIterator->MoveBeforeFirst();
    if( JET_errNoCurrentRecord == ( err = precordIterator->ErrMoveNext() ) )
    {
        //  no tagged columns;
        delete precordIterator;
        precordIterator = NULL;
        err = JET_errSuccess;
    }
    Call( err );

    if( fRetrieveDefaults )
    {
        pdefaultValuesIterator = new TAGFIELDS_ITERATOR( *ptdb->PdataDefaultRecord() );
        if( NULL == pdefaultValuesIterator )
        {
            Call( ErrERRCheck( JET_errOutOfMemory ) );
        }
        pdefaultValuesIterator->MoveBeforeFirst();
        if( JET_errNoCurrentRecord == ( err = pdefaultValuesIterator->ErrMoveNext() ) )
        {
            //  no tagged columns;
            delete pdefaultValuesIterator;
            pdefaultValuesIterator = NULL;
            err = JET_errSuccess;
        }
        Call( err );
    }

    //  if necessary, advance to starting column

    if ( FidOfColumnid( columnidStart ).FTagged() )
    {
        if ( precordIterator )
        {
            while( CmpFid(
                    precordIterator->Fid(),
                    precordIterator->FDerived(),
                    FidOfColumnid( columnidStart ),
                    FRECUseDerivedBit( columnidStart, ptdb ) ) < 0 )
            {
                if( JET_errNoCurrentRecord == ( err = precordIterator->ErrMoveNext() ) )
                {
                    delete precordIterator;
                    precordIterator = NULL;
                    err = JET_errSuccess;
                    break;
                }
                Call( err );
            }
        }

        if ( pdefaultValuesIterator )
        {
            while( CmpFid(
                    pdefaultValuesIterator->Fid(),
                    pdefaultValuesIterator->FDerived(),
                    FidOfColumnid( columnidStart ),
                    FRECUseDerivedBit( columnidStart, ptdb ) ) < 0 )
            {
                if( JET_errNoCurrentRecord == ( err = pdefaultValuesIterator->ErrMoveNext() ) )
                {
                    delete pdefaultValuesIterator;
                    pdefaultValuesIterator = NULL;
                    err = JET_errSuccess;
                    break;
                }
                Call( err );
            }
        }
    }

    //  iterate

    TAGFIELDS_ITERATOR *    pIteratorCur        = NULL;
    INT                     ExistingIterators   = 0;
    BOOL                    fRecordDerived      = fFalse;
    BOOL                    fDefaultDerived     = fFalse;
    INT                     cmp                 = 0;


    if ( NULL != pdefaultValuesIterator )
    {
        ExistingIterators++;
        pIteratorCur = pdefaultValuesIterator;
        cmp = 1;
    }
    if ( NULL != precordIterator )
    {
        ExistingIterators++;
        pIteratorCur = precordIterator;
        cmp = -1;
        // we have both iterators, assume that we were starting with recordIterator
        if ( 2 == ExistingIterators )
        {
            Assert( 0 == fidRecordFID );
            Assert( !fRecordDerived );
            fidDefaultFID = pdefaultValuesIterator->Fid();
            fDefaultDerived = pdefaultValuesIterator->FDerived();
        }
    }

    while ( ExistingIterators > 0 )
    {
        if ( 2 == ExistingIterators )
        {
            if ( pIteratorCur == precordIterator )
            {
                fidRecordFID = pIteratorCur->Fid();
                fRecordDerived = pIteratorCur->FDerived();
                if ( 0 == cmp )
                {
                    // skip this because we've alread picked up this column from record
                    pIteratorCur = pdefaultValuesIterator;
                    goto NextIteration;
                }
            }
            else
            {
                Assert( pIteratorCur == pdefaultValuesIterator );
                fidDefaultFID = pIteratorCur->Fid();
                fDefaultDerived = pIteratorCur->FDerived();
            }
            //  there are both record and default tagged column values
            //  find which one is first
            cmp = CmpFid( fidRecordFID, fRecordDerived, fidDefaultFID, fDefaultDerived );
            if ( cmp > 0 )
            {
                //  the column is less than the current column in the record
                pIteratorCur = pdefaultValuesIterator;
            }
            else
            {
                //  columns are equal or the default value is greater
                //  select the one in the record
                pIteratorCur = precordIterator;
            }
        }

        err = ErrRECIAccessColumn(
                    pfucb,
                    pIteratorCur->Columnid( ptdb ) );
        if ( err < 0 )
        {
            if ( JET_errColumnNotFound != err )
            {
                Call( err );
            }
        }
        else
        {
            //  column is visible to us
            if( !fCountOnly )
            {
                RECIAddTaggedColumnListEntry(
                    rgtagcolinfo + centriesCurr,
                    pIteratorCur,
                    ptdb,
                    fFalse );
            }
            ++centriesCurr;
            if( centriesMax == centriesCurr )
            {
                break;
            }
        }

NextIteration:
        err = pIteratorCur->ErrMoveNext();
        if ( JET_errNoCurrentRecord == err )
        {
            ExistingIterators--;
            if ( pIteratorCur == precordIterator )
            {
                pIteratorCur = pdefaultValuesIterator;
                if ( 0 == cmp )
                {
                    goto NextIteration;
                }
            }
            else
            {
                Assert( pIteratorCur == pdefaultValuesIterator );
                pIteratorCur = precordIterator;
            }
            err = JET_errSuccess;
        }
        Call( err );
        CallS( err );
    }
    CallS( err );
    *pcentries = centriesCurr;

HandleError:
    if ( NULL != precordIterator )
    {
        delete precordIterator;
    }
    if ( NULL != pdefaultValuesIterator )
    {
        delete pdefaultValuesIterator;
    }

    return err;
}

ERR VTAPI ErrIsamRetrieveTaggedColumnList(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    ULONG               *pcentries,
    VOID                *pv,
    const ULONG         cbMax,
    const JET_COLUMNID  columnidStart,
    const JET_GRBIT     grbit )
{
    ERR                 err;
    PIB                 *ppib               = reinterpret_cast<PIB *>( vsesid );
    FUCB                *pfucb              = reinterpret_cast<FUCB *>( vtid );
    DATA                *pdataRec;
    BOOL                fTransactionStarted = fFalse;

    CallR( ErrPIBCheck( ppib ) );
    CheckFUCB( ppib, pfucb );
    AssertDIRNoLatch( ppib );

    //  must always provide facility to return number of entries in the list
    if ( NULL == pcentries )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        return err;
    }

    //  set ptdb.  ptdb is same for indexes and for sorts.
    //
    Assert( pfucb->u.pfcb->Ptdb() == pfucb->u.pscb->fcb.Ptdb() );
    Assert( pfucb->u.pfcb->Ptdb() != ptdbNil );

    if ( 0 == ppib->Level() )
    {
        // Begin transaction for read consistency (in case page
        // latch must be released in order to validate column).
        CallR( ErrDIRBeginTransaction( ppib, 53541, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }

    AssertDIRNoLatch( ppib );

    Assert( FFUCBSort( pfucb ) || FFUCBIndex( pfucb ) );

    const BOOL  fUseCopyBuffer  = ( ( ( grbit & JET_bitRetrieveCopy ) && FFUCBUpdatePrepared( pfucb ) && !FFUCBNeverRetrieveCopy( pfucb ) )
                                    || FFUCBAlwaysRetrieveCopy( pfucb ) );

    Call( ErrRECIGetRecord( pfucb, &pdataRec, fUseCopyBuffer ) );

    Call( ErrRECIBuildTaggedColumnList(
                pfucb,
                *pdataRec,
                reinterpret_cast<TAGCOLINFO *>( pv ),
                pcentries,
                cbMax / sizeof(TAGCOLINFO),
                columnidStart,
                grbit ) );

HandleError:
    if ( Pcsr( pfucb )->FLatched() )
    {
        Assert( !fUseCopyBuffer );
        CallS( ErrDIRRelease( pfucb ) );
    }

    if ( fTransactionStarted )
    {
        //  No updates performed, so force success.
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( ppib );

    return err;
}

