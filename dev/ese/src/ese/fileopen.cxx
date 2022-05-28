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

    //  allocate FUCB
    //
    Call( ErrDIROpen( ppib, pfucbOpen->u.pfcb, &pfucb ) );

    //  reset copy buffer
    //
    pfucb->pvWorkBuf = NULL;
    pfucb->dataWorkBuf.SetPv( NULL );
    Assert( !FFUCBUpdatePrepared( pfucb ) );

    //  reset key buffer
    //
    pfucb->dataSearchKey.Nullify();
    pfucb->cColumnsInSearchKey = 0;
    KSReset( pfucb );

    //  copy cursor flags
    //
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

    Assert( !FFUCBMayCacheLVCursor( pfucb ) );  //  initialized to FALSE
    if ( FFUCBMayCacheLVCursor( pfucbOpen ) )
    {
        FUCBSetMayCacheLVCursor( pfucb );
    }

    //  move currency to the first record and ignore error if no records
    //
    RECDeferMoveFirst( ppib, pfucb );
    err = JET_errSuccess;

    pfucb->pvtfndef = &vtfndefIsam;
    *ppfucb = pfucb;

    return JET_errSuccess;

HandleError:
    return err;
}


//+local
// ErrTDBCreate
// ========================================================================
// ErrTDBCreate(
//      TDB **pptdb,            // OUT   receives new TDB
//      FID fidFixedLast,       // IN      last fixed field id to be used
//      FID fidVarLast,         // IN      last var field id to be used
//      FID fidTaggedLast )     // IN      last tagged field id to be used
//
// Allocates a new TDB, initializing internal elements appropriately.
//
// PARAMETERS
//              pptdb           receives new TDB
//              fidFixedLast    last fixed field id to be used
//                              (should be FFixedNone() if none)
//              fidVarLast      last var field id to be used
//                              (should be FVarNone() if none)
//              fidTaggedLast   last tagged field id to be used
//                              (should be FTaggedNone() if none)
// RETURNS      Error code, one of:
//                   JET_errSuccess             Everything worked.
//                  -JET_errOutOfMemory Failed to allocate memory.
// SEE ALSO     ErrRECAddFieldDef
//-

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
    ERR     err;                    // standard error value
    WORD    cfieldFixed;            // # of fixed fields
    WORD    cfieldVar;              // # of var fields
    WORD    cfieldTagged;           // # of tagged fields
    WORD    cfieldTotal;            // Fixed + Var + Tagged
    TDB     *ptdb = ptdbNil;        // temporary TDB pointer
    FID     fidFixedFirst;
    FID     fidVarFirst;
    FID     fidTaggedFirst;
    WORD    ibEndFixedColumns;

    Assert( pptdb != NULL );
    Assert( ptcib->fidFixedLast.FFixedNone() || ptcib->fidFixedLast.FFixed() );
    Assert( ptcib->fidVarLast.FVarNone() || ptcib->fidVarLast.FVar() );
    Assert( ptcib->fidTaggedLast.FTaggedNone() || ptcib->fidTaggedLast.FTagged() );

    if ( pfcbNil == pfcbTemplateTable )
    {
        fidFixedFirst = FID( fidtypFixed, fidlimLeast );
        fidVarFirst = FID( fidtypVar, fidlimLeast );
        fidTaggedFirst = FID( fidtypTagged, fidlimLeast );
        ibEndFixedColumns = ibRECStartFixedColumns;
    }
    else
    {
        fidFixedFirst = FID( pfcbTemplateTable->Ptdb()->FidFixedLast() + 1 );
        if ( ptcib->fidFixedLast.FFixedNone() )
        {
            ptcib->fidFixedLast = FID( fidFixedFirst - 1 );
        }
        Assert( ptcib->fidFixedLast >= fidFixedFirst-1 );

        fidVarFirst = FID( pfcbTemplateTable->Ptdb()->FidVarLast() + 1 );
        if ( ptcib->fidVarLast.FVarNone() )
        {
            ptcib->fidVarLast = FID( fidVarFirst - 1 );
        }
        Assert( ptcib->fidVarLast >= fidVarFirst-1 );

        if ( pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template().FFixed() )
        {
            fidTaggedFirst = FID( pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template() + 1 );
        }
        else
        {
            Assert( pfcbTemplateTable->Ptdb()->FidTaggedLastOfESE97Template().FFixedNone() );
            fidTaggedFirst = FID( fidtypTagged, fidlimLeast );
        }

        if ( ptcib->fidTaggedLast.FTaggedNone() )
        {
            ptcib->fidTaggedLast = FID( fidtypTagged, fidlimNone );
        }
        Assert( ptcib->fidTaggedLast.FTaggedNone() || ptcib->fidTaggedLast.FTagged() );

        ibEndFixedColumns = pfcbTemplateTable->Ptdb()->IbEndFixedColumns();
    }

    //  calculate how many of each field type to allocate
    //
    cfieldFixed = WORD( ptcib->fidFixedLast + 1 - fidFixedFirst );
    cfieldVar = WORD( ptcib->fidVarLast + 1 - fidVarFirst );
    cfieldTagged = WORD( ptcib->fidTaggedLast + 1 - fidTaggedFirst );
    cfieldTotal = WORD( cfieldFixed + cfieldVar + cfieldTagged );

    //  sizeof(TDB) == (x86: 160 bytes), (amd64: 240 bytes)
    C_ASSERT( sizeof(TDB) % 16 == 0 );

    //  allocate the TDB
    //
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

    //  propagate TDB flags from the template table

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

    // Allocate space for the FIELD structures.  In addition, allocate padding
    // in case there's index info
    if ( fAllocateNameSpace )
    {
        Call( ptdb->MemPool().ErrMEMPOOLInit(
            cbAvgName * ( cfieldTotal + 1 ),    // +1 for table name
            USHORT( cfieldTotal + 2 ),          // # tag entries = 1 per fieldname, plus 1 for all FIELD structures, plus 1 for table name
            fTrue ) );
    }
    else
    {
        //  this is a temp/sort table, so only allocate enough tags
        //  for the FIELD structures, table name, and IDXSEG
        Call( ptdb->MemPool().ErrMEMPOOLInit( cbFIELDPlaceholder, 3, fFalse ) );
    }

    //  add FIELD placeholder
    MEMPOOL::ITAG   itagNew;
    Call( ptdb->MemPool().ErrAddEntry( NULL, cbFIELDPlaceholder, &itagNew ) );
    Assert( itagNew == itagTDBFields ); // Should be the first entry in the buffer.

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

    //  set output parameter and return
    //
    *pptdb = ptdb;
    return JET_errSuccess;

HandleError:
    delete ptdb;
    return err;
}



//+API
// ErrRECAddFieldDef
// ========================================================================
// ErrRECAddFieldDef(
//      TDB *ptdb,      //  INOUT   TDB to add field definition to
//      FID fid );      //  IN      field id of new field
//
// Adds a field descriptor to an TDB.
//
// PARAMETERS   ptdb            TDB to add new field definition to
//              fid             field id of new field (should be within
//                              the ranges imposed by the parameters
//                              supplied to ErrTDBCreate)
//              ftFieldType     data type of field
//              cbField         field length (only important when
//                              defining fixed textual fields)
//              bFlags          field behaviour flags:
//              szFieldName     name of field
//
// RETURNS      Error code, one of:
//                   JET_errSuccess         Everything worked.
//                  -JET_errColumnNotFound          Field id given is greater than
//                                          the maximum which was given
//                                          to ErrTDBCreate.
//                  -JET_errColumnNotFound      A nonsensical field id was given.
//                  -JET_errInvalidColumnType The field type given is either
//                                          undefined, or is not acceptable
//                                          for this field id.
// COMMENTS     When adding a fixed field, the fixed field offset table
//              in the TDB is recomputed.
// SEE ALSO     ErrTDBCreate
//-
ERR ErrRECAddFieldDef( TDB *ptdb, const COLUMNID columnid, FIELD *pfield )
{
    FIELD *     pfieldNew;
    const FID   fid         = FidOfColumnid( columnid );

    Assert( ptdb != ptdbNil );
    Assert( pfield != pfieldNil );

    //  fixed field: determine length, either from field type
    //  or from parameter (for text/binary types)
    //
    Assert( FCOLUMNIDValid( columnid ) );

    if ( FCOLUMNIDTagged( columnid ) )
    {
        //  tagged field: any type is ok
        //
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

        //  variable column.  Check for bogus numeric and long types
        //
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


//  determine actual key most for primary index so that additional record space
//  can be used even when applications do not set cbKeyMost precisely.
//  This is of the greatest benefit for clustered indexes with a few fixed size columns,
//  e.g. integers.
//
//  This feature is only supported on 16kByte or larger page sizes for backward compatibility.
//
void FILEIMinimizePrimaryCbKeyMost( FCB * pfcb, IDB * const pidb )
{
    const IDXSEG    *pidxseg;
    const IDXSEG    *pidxsegMac;

    Assert( pfcb->Ptdb() != ptdbNil );
    Assert( pidb != pidbNil );
    Assert( pidb->Cidxseg() > 0 );
    //  primary index cannot have tuples
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

    //  retrieve each segment in key description
    //
    if ( pidb->FTemplateIndex() )
    {
        Assert( pfcb->FDerivedTable() || pfcb->FTemplateTable() );
        if ( pfcb->FDerivedTable() )
        {
            //  switch to template table
            //
            pfcb = pfcb->Ptdb()->PfcbTemplateTable();
            Assert( pfcbNil != pfcb );
            Assert( pfcb->FTemplateTable() );
        }
        else
        {
            Assert( pfcb->Ptdb()->PfcbTemplateTable() == pfcbNil );
        }
    }

    //  for each index segment, add its largest actual size to the actual key most counter
    //
    pidxseg = PidxsegIDBGetIdxSeg( pidb, pfcb->Ptdb() );
    pidxsegMac = pidxseg + pidb->Cidxseg();
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        FIELD           field;
        const COLUMNID  columnid    = pidxseg->Columnid();
        const BOOL      fFixedField = FCOLUMNIDFixed( columnid );

        //  no need to check column access since the column belongs to an index, 
        //  it must be available.  It can't be deleted, or even versioned deleted.
        field = *( pfcb->Ptdb()->Pfield( columnid ) );

        Assert( !FFIELDDeleted( field.ffield ) );
        Assert( JET_coltypNil != field.coltyp );
        Assert( !FCOLUMNIDVar( columnid ) || field.coltyp == JET_coltypBinary || field.coltyp == JET_coltypText );
        Assert( !FFIELDMultivalued( field.ffield ) || FCOLUMNIDTagged( columnid ) );

        //  for each field, add the largest actual key size possible
        //
        switch ( field.coltyp )
        {
            //  fixed length data types
            //
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
                //  rather than attempt to compute the actual key most for unicode,
                //  and other text, simply return cbKeyMost.  If applications
                //  want to utilize more record space with text based keys
                //  they can always set a more accurate cbKeyMost for the index.
                //
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
                        //  add smaller of fixed size limitation and cbVarSegMac.  
                        //  Note that this seems wrong because cbVarSegMac should not
                        //  apply to fixed length binary columns.  However, it does
                        //  and this code must match existing behavior.
                        //
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
                        //  if variable length binary with no maximum size then return
                        //
//                      if ( field.cbMaxLen == 0 )
//                          {
//                          return;
//                          }
//                      cbActualKeyMost += ( cbKeySegmentHeader + ( ( ( field.cbMaxLen + 7 ) / 8 ) * 9 ) );

                        //  rather than attempt to compute the actual key most 
                        //  for variable length binary, simply return cbKeyMost.  
                        //  If applications want to utilize more record space 
                        //  they can always set a more accurate cbKeyMost for the index.
                        //
                        return;
                    }
                }
                break;

            default:
                Assert( fFalse );
                break;
        }
    }

    //  if the active constraint on key size comes from the key itself,
    //  and is smaller than the externally enforced cbKeyMost then
    //  cbKeyMost can be reduced to permit more space to be used for record data.
    //
    if ( cbActualKeyMost < cbKeyMost )
    {
        //  cbVarSegMac must be reduced as well to avoid asserts
        //
        if ( cbActualKeyMost < cbVarSegMac )
        {
            //  if actual key most less than cbVarSegMac 
            //  then cbVarSegMac must not be set.
            //
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

    //  check validity of each segment id and
    //  also set index mask bits
    //
    pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    pidxsegMac = pidxseg + pidb->Cidxseg();

    if ( !pidb->FAllowFirstNull() )
    {
        pidb->ResetFAllowAllNulls();
    }


    //  SPARSE INDEXES
    //  --------------
    //  Sparse indexes are an attempt to
    //  optimise record insertion by not having to
    //  update indexes that are likely to be sparse
    //  (ie. there are enough unset columns in the
    //  record to cause the record to be excluded
    //  due to the Ignore*Null flags imposed on the
    //  index).  Sparse indexes exploit any Ignore*Null
    //  flags to check before index update that enough
    //  index columns were left unset to satisfy the
    //  Ignore*Null conditions and therefore not require
    //  that the index be updated. In order for an
    //  index to be labelled as sparse, the following
    //  conditions must be satisfied:
    //      1) this is not the primary index (can't
    //         skip records in the primary index)
    //      2) null segments are permitted
    //      3) no index entry generated if any/all/first
    //         index column(s) is/are null (ie. at
    //         least one of the Ignore*Null flags
    //         was used to create the index)
    //      4) all of the index columns may be set
    //         to NULL
    //      5) none of the index columns has a default
    //         value (because then the column would
    //         be non-null)

    const BOOL  fIgnoreAllNull      = !pidb->FAllowAllNulls();
    const BOOL  fIgnoreFirstNull    = !pidb->FAllowFirstNull();
    const BOOL  fIgnoreAnyNull      = !pidb->FAllowSomeNulls();
    BOOL        fSparseIndex        = ( !pidb->FPrimary()
                                        && !pidb->FNoNullSeg()
                                        && ( fIgnoreAllNull || fIgnoreFirstNull || fIgnoreAnyNull ) );

    //  the primary index should not have had any of the Ignore*Null flags set
    Assert( !pidb->FPrimary()
        || pidb->FNoNullSeg()
        || ( pidb->FAllowAllNulls() && pidb->FAllowFirstNull() && pidb->FAllowSomeNulls() ) );

    const IDXSEG * const    pidxsegFirst    = pidxseg;
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        //  field id is absolute value of segment id -- these should already
        //  have been validated, so just add asserts to verify integrity of
        //  fid's and their FIELD structures
        //
        const COLUMNID          columnid    = pidxseg->Columnid();
        const FIELD * const     pfield      = ptdb->Pfield( columnid );
        Assert( pfield->coltyp != JET_coltypNil );
        Assert( !FFIELDDeleted( pfield->ffield ) );

        if ( FFIELDUserDefinedDefault( pfield->ffield ) )
        {
            if ( !fFoundUserDefinedDefault )
            {
                //  UNDONE:  use the dependency list to optimize this
                memset( pidb->RgbitIdx(), 0xff, 32 );
                fFoundUserDefinedDefault = fTrue;
            }

            //  user-defined defaults may return a NULL or
            //  non-NULL default value for the column,
            //  so it is unclear whether the record would be
            //  present in the index
            //  SPECIAL CASE: if IgnoreFirstNull and this is
            //  not the first column, then it's still possible
            //  for this to be a sparse index
            if ( !fIgnoreFirstNull
                || fIgnoreAnyNull
                || pidxsegFirst == pidxseg )
            {
                fSparseIndex = fFalse;
            }
        }

        if ( FFIELDNotNull( pfield->ffield ) || FFIELDDefault( pfield->ffield ) )
        {
            //  if field can't be NULL or if there's a default value
            //  overriding NULL, this can't be a sparse index
            //  SPECIAL CASE: if IgnoreFirstNull and this is
            //  not the first column, then it's still possible
            //  for this to be a sparse index
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

                //  Multivalued flag was persisted in catalog as of 0x620,2
                if ( !pidb->FMultivalued() )
                {
                    Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulVersion == 0x00000620 );
                    Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulDaeUpdateMajor < 0x00000002 );
                }

                //  Must set the flag anyway to ensure backward
                //  compatibility.
                pidb->SetFMultivalued();
            }
        }
        else if ( FFIELDPrimaryIndexPlaceholder( pfield->ffield )
            && pidb->FPrimary() )
        {
            //  must be first column in index
            Assert( PidxsegIDBGetIdxSeg( pidb, ptdb ) == pidxseg );

            //  must be fixed bitfield
            Assert( FCOLUMNIDFixed( columnid ) );
            Assert( JET_coltypBit == pfield->coltyp );
            pidb->SetFHasPlaceholderColumn();
        }

        if ( FRECTextColumn( pfield->coltyp ) && usUniCodePage == pfield->cp )
        {
            //  LocalizedText flag was persisted in catalog as of 0x620,2 (circa 1997)
            if ( !pidb->FLocalizedText() )
            {
                Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulVersion == 0x00000620 );
                Assert( g_rgfmp[pfcb->Ifmp()].Pdbfilehdr()->le_ulDaeUpdateMajor < 0x00000002 );
            }

            //  Must set the flag anyway to ensure backward
            //  compatibility.
            pidb->SetFLocalizedText();
        }

        pidb->SetColumnIndex( FidOfColumnid( columnid ) );
        Assert ( pidb->FColumnIndex( FidOfColumnid( columnid ) ) );
    }


    //  SPARSE CONDITIONAL INDEXES
    //  ---------------------------
    //  Sparse conditional indexes are an attempt to
    //  optimise record insertion by not having to
    //  update conditional indexes that are likely to
    //  be sparse (ie. there are enough unset columns
    //  in the record to cause the record to be
    //  excluded due to the conditional columns imposed
    //  on the index).  We label a conditional index as
    //  sparse if there is AT LEAST ONE conditional
    //  column which, if unset, will cause the record
    //  to NOT be included in the index.  This means
    //  that if the condition is MustBeNull, the column
    //  must have a default value, and if the
    //  condition is MustBeNonNull, the column must
    //  NOT have a default value.

    BOOL    fSparseConditionalIndex     = fFalse;

    //  the primary index should not have any conditional columns
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
                //  UNDONE:  use the dependency list to optimize this
                memset( pidb->RgbitIdx(), 0xff, 32 );
                fFoundUserDefinedDefault = fTrue;
            }

            //  only reason to continue is to see if
            //  this might be a sparse conditional
            //  index, so if this has already been
            //  determined, then we can exit
            if ( fSparseConditionalIndex )
                break;
        }
        else if ( !fSparseConditionalIndex )
        {
            //  if there is a default value to force the
            //  column to be non-NULL by default
            //  (and thus NOT be present in the index),
            //  then we can treat this as a sparse column
            //  if there is no default value for this column,
            //  the column will be NULL by default (and
            //  thus be present in the index), so we can
            //  treat this as a sparse column
            const BOOL  fHasDefault = FFIELDDefault( pfield->ffield );

            fSparseConditionalIndex = ( pidxseg->FMustBeNull() ?
                                            fHasDefault :
                                            !fHasDefault );
        }

        pidb->SetColumnIndex( FidOfColumnid( columnid ) );
        Assert( pidb->FColumnIndex( FidOfColumnid( columnid ) ) );
    }

    //  all requirements for sparse indexes are met
    //
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

    //  minimize cbKeyMost for primary index based on key definition
    //
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


//  ================================================================
ERR VTAPI ErrIsamSetSequential(
    const JET_SESID sesid,
    const JET_VTID  vtid,
    const JET_GRBIT grbit )
//  ================================================================
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

    //  if direction hint was given, set up for preread now
    //  so it can be initiated on the very first seek
    //
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


//  ================================================================
ERR VTAPI ErrIsamResetSequential(
    const JET_SESID sesid,
    const JET_VTID vtid,
    const JET_GRBIT )
//  ================================================================
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
    _In_ const PCSTR    szPath,
    JET_GRBIT           grbit )
{
    ERR         err;
    PIB         *ppib   = (PIB *)vsesid;
    const IFMP  ifmp    = (IFMP)vdbid;
    FUCB        *pfucb  = pfucbNil;

    //  initialise return value
    Assert( ptableid );
    *ptableid = JET_tableidNil;

    //  check parameters
    //
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
        //  Both of these grbits can result in a warning from ErrFILEIOpenTable().
        //  JET_wrnTableInUseBySystem, and JET_wrnPrimaryIndexOutOfDate/JET_wrnSecondaryIndexOutOfDate
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    else if ( grbit & bitTableUpdatableDuringRecovery )
    {
        //  Internal grbit, not to be used publicly
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    // ErrFILEIOpenTable() can return warnings as well.
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

// monitoring statistics

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

#endif // PERFMON_SUPPORT


//  set domain usage mode or return error.  Allow only one deny read
//  or one deny write.  Sessions that own locks may open other read
//  or read write cursors.
//
LOCAL ERR ErrFILEISetMode( FUCB *pfucb, const JET_GRBIT grbit )
{
    ERR     wrn     = JET_errSuccess;
    PIB     *ppib   = pfucb->ppib;
    FCB     *pfcb   = pfucb->u.pfcb;
    FUCB    *pfucbT;

    Assert( pfcb->IsLocked() );

    Assert( !pfcb->FDeleteCommitted() );

    //  all cursors can read so check for deny read flag by other session.
    //
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

    //  check for deny write flag by other session.  If deny write flag
    //  set then only cursors of that session or cleanup cursors may have
    //  write privileges.
    //
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

    //  if deny write lock requested, check for updatable cursor of
    //  other session.  If lock is already held, even by given session,
    //  then return error.
    //
    if ( grbit & JET_bitTableDenyWrite )
    {
        //  if any session has this table open deny write, including given
        //  session, then return error.
        //
        if ( pfcb->FDomainDenyWrite() )
        {
            return ErrERRCheck( JET_errTableInUse );
        }

        //  check is cursors with write mode on domain.
        //
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

    //  if deny read lock requested, then check for cursor of other
    //  session.  If lock is already held, return error.
    //
    if ( grbit & JET_bitTableDenyRead )
    {
        //  if other session has this table open deny read, return error
        //
        if ( pfcb->FDomainDenyRead( ppib ) )
        {
            return ErrERRCheck( JET_errTableInUse );
        }

        //  check if cursors belonging to another session
        //  are open on this domain.
        //
        BOOL    fOpenSystemCursor = fFalse;
        pfcb->FucbList().LockForEnumeration();
        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++ )
        {
            pfucbT = pfcb->FucbList()[ ifucbList ];

            if ( pfucbT != pfucb )      // Ignore current cursor
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
                //  if this is a template table, only allow it to be open for modification if
                //  its FCB hasn't been flagged as static
                //
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


//  if opening domain for read, write or read write, and not with
//  deny read or deny write, and domain does not have deny read or
//  deny write set, then return JET_errSuccess, else call
//  ErrFILEISetMode to determine if lock is by other session or to
//  put lock on domain.
//
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

    // table delete cannot be specified without DenyRead
    //
    Assert( !( grbit & JET_bitTableDelete )
        || ( grbit & JET_bitTableDenyRead ) );

    // PermitDDL cannot be specified without DenyRead
    //
    Assert( !( grbit & JET_bitTablePermitDDL )
        || ( grbit & JET_bitTableDenyRead ) );

    //  if table is scheduled for deletion, then return error
    //
    if ( pfcb->FDeletePending() )
    {
        // Normally, the FCB of a table to be deleted is protected by the
        // DomainDenyRead flag.  However, this flag is released during VERCommit,
        // while the FCB is not actually purged until RCEClean.  So to prevent
        // anyone from accessing this FCB after the DomainDenyRead flag has been
        // released but before the FCB is actually purged, check the DeletePending
        // flag, which is NEVER cleared after a table is flagged for deletion.
        //
        Error( ErrERRCheck( JET_errTableLocked ) );
    }

    //  if read/write restrictions specified, or other
    //  session has any locks, then must check further
    //
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

// This will be done on every OpenTable call. The CAT function has a short-
// circuit check on the FCB to avoid the expense on every call.
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
            // When deleting a table, we don't care about whether the indices
            // are actually out of date or not.
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
            //  Massage the error code, allowing the caller to open a table with an
            //  out-of-date text index.
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

//+local
//  ErrFILEIOpenTable
//  ========================================================================
//  ErrFILEIOpenTable(
//      PIB *ppib,          // IN    PIB of who is opening file
//      IFMP ifmp,          // IN    database id
//      FUCB **ppfucb,      // OUT   receives a new FUCB open on the file
//      CHAR *szName,       // IN    path name of file to open
//      ULONG grbit );      // IN    open flags
//  Opens a data file, returning a new
//  FUCB on the file.
//
// PARAMETERS
//              ppib        PIB of who is opening file
//              ifmp        database id
//              ppfucb      receives a new FUCB open on the file
//                          ( should NOT already be pointing to an FUCB )
//              szName      path name of file to open ( the node
//                          corresponding to this path must be an FDP )
//              grbit       flags:
//                          JET_bitTableDenyRead    open table in exclusive mode;
//                          default is share mode
//              pfdpinfo    FDP information
//              catcifFlags Check index flags.
//
// RETURNS      Lower level errors, or one of:
//                   JET_errSuccess                 Everything worked.
//                  -TableInvalidName               The path given does not
//                                                  specify a file.
//                  -JET_errDatabaseCorrupted       The database directory tree
//                                                  is corrupted.
//                  -Various out-of-memory error codes.
//              In the event of a fatal ( negative ) error, a new FUCB
//              will not be returned.
// SIDE EFFECTS FCBs for the file and each of its secondary indexes are
//              created ( if not already in the global list ).  The file's
//              FCB is inserted into the global FCB list.  One or more
//              unused FCBs may have had to be reclaimed.
//              The currency of the new FUCB is set to "before the first item".
// SEE ALSO     ErrFILECloseTable
//-
ERR ErrFILEIOpenTable(
    _In_ PIB         *ppib,
    _In_ IFMP        ifmp,
    _Out_ FUCB       **ppfucb,
    _In_ const CHAR  *szName,
    _In_ ULONG       grbit,
    _In_opt_ FDPINFO *pfdpinfo )
{
    ERR         err;
    ERR         wrnSurvives             = JET_errSuccess;
    FUCB        *pfucb                  = pfucbNil;
    FCB         *pfcb;
    CHAR        szTable[JET_cbNameMost+1];
    PGNO        pgnoFDP                 = pgnoNull;
    OBJID       objidTable              = objidNil;
    BOOL        fInTransaction          = fFalse;
    BOOL        fInitialisedCursor      = fFalse;
    TABLECLASS  tableclass              = tableclassNone;

    Assert( ppib != ppibNil );
    Assert( ppfucb != NULL );
    FMP::AssertVALIDIFMP( ifmp );

    typedef enum class TABLE_TYPE {
        System,
        ExtentPageCountCache,
        Temp,
        Normal } tt;

    TABLE_TYPE ttSubject;

    //  initialize return value to Nil
    //
    *ppfucb = pfucbNil;

#ifdef DEBUG
    CheckPIB( ppib );
    if( !Ptls()->FIsTaskThread()
        && !Ptls()->fIsRCECleanup )
    {
        CheckDBID( ppib, ifmp );
    }
#endif  //  DEBUG
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

    if ( FCATSystemTable( szTable ) )
    {
        ttSubject = tt::System;
        Assert( !FFMPIsTempDB( ifmp ) );
    }
    else if ( FCATExtentPageCountCacheTable( szTable ) )
    {
        // The Extent Page Count Cache doesn't always exist, but when it exists we cache the
        // objidFDP and pgnoFDP on the FMP, so if the values are there, use them and do
        // more specific processing based on them.
        if ( pgnoNull != g_rgfmp[ ifmp ].PgnoExtentPageCountCacheFDP() &&
             objidNil != g_rgfmp[ ifmp ].ObjidExtentPageCountCacheFDP()   )
        {
            ttSubject = tt::ExtentPageCountCache;
        }
        else
        {
            ttSubject = tt::Normal;
        }
        Assert( !FFMPIsTempDB( ifmp ) );
    }
    else if ( !FFMPIsTempDB( ifmp ) )
    {
        ttSubject = tt::Normal;
    }
    else
    {
        ttSubject = tt::Temp;
    }

    if ( NULL == pfdpinfo )
    {
        switch ( ttSubject )
        {
            case tt::Temp:
                // Temp tables should pass in PgnoFDP.
                AssertSz( fFalse, "Illegal dbid" );
                return ErrERRCheck( JET_errInvalidDatabaseId );

            case tt::System:
                if ( grbit & JET_bitTableDelete )
                {
                    return ErrERRCheck( JET_errCannotDeleteSystemTable );
                }
                pgnoFDP     = PgnoCATTableFDP( szTable );
                objidTable  = ObjidCATTable( szTable );
                break;

            case tt::ExtentPageCountCache:
                if ( 0 == ( grbit & JET_bitTableAllowSensitiveOperation ) )
                {
                    // We require callers who didn't explicitly specify they want to work on
                    // a sensitive table to meet extra restrictions.  This is to block end users
                    // from deleting the table or opening the table updatably and then doing
                    // nasty things to it.
                    switch ( grbit & ( JET_bitTableReadOnly | JET_bitTableDelete ) )
                    {
                        case JET_bitTableReadOnly:
                            // Allowably opening read only.
                            break;

                        case JET_bitTableDelete:
                            // Trying to delete without stating you are OK with a sensitive table delete.
                            return ErrERRCheck( JET_errCannotDeleteSystemTable );

                        case JET_bitNil:
                            // Trying to just open the table, allowing updates.
                            return ErrERRCheck( JET_errTableLocked );

                        default:
                            // Unexpected combination of grbits. Never seen this during normal operation.
                            Expected( fFalse );
                            return ErrERRCheck( JET_errInvalidGrbit );
                    }
                }
                pgnoFDP = g_rgfmp[ ifmp ].PgnoExtentPageCountCacheFDP();
                objidTable = g_rgfmp[ ifmp ].ObjidExtentPageCountCacheFDP();

                // I don't think this needs any synchronization, but these asserts might
                // catch a case that shows we DO.
                Assert( pgnoNull != pgnoFDP );
                Assert( objidNil != objidTable );
                break;

            case tt::Normal:
                if ( 0 == ppib->Level() )
                {
                    CallR( ErrDIRBeginTransaction( ppib, 35109, JET_bitTransactionReadOnly ) );
                    fInTransaction = fTrue;
                }

                //  lookup the table in the catalog hash before seeking
                if ( !FCATHashLookup( ifmp, szTable, &pgnoFDP, &objidTable ) )
                {
                    Call( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoFDP, &objidTable ) );
                }
                break;

            default:
                Enforce( fFalse );
        }
    }
    else
    {
        Assert( pgnoNull != pfdpinfo->pgnoFDP );
        Assert( objidNil != pfdpinfo->objidFDP );
        pgnoFDP = pfdpinfo->pgnoFDP;
        objidTable = pfdpinfo->objidFDP;

#ifdef DEBUG
        switch ( ttSubject )
        {
            case tt::Temp:
                // No verification.
                break;

            case tt::System:
                Assert( PgnoCATTableFDP( szTable ) == pgnoFDP );
                Assert( ObjidCATTable( szTable ) == objidTable );
                break;

            case tt::ExtentPageCountCache:
                Assert( g_rgfmp[ ifmp ].PgnoExtentPageCountCacheFDP() == pgnoFDP );
                Assert( g_rgfmp[ ifmp ].ObjidExtentPageCountCacheFDP() == objidTable );
                break;

            case tt::Normal:
            {
                PGNO    pgnoT;
                OBJID   objidT;
                
                //  lookup the table in the catalog hash before seeking
                if ( !FCATHashLookup( ifmp, szTable, &pgnoT, &objidT ) )
                {
                    CallR( ErrCATSeekTable( ppib, ifmp, szTable, &pgnoT, &objidT ) );
                }

                Assert( pgnoT == pgnoFDP );
                Assert( objidT == objidTable );
                break;
            }
            default:
                Enforce( fFalse );
        }
#endif  //  DEBUG
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

    switch ( ttSubject )
    {
        case tt::System:
            tableclass = TableClassFromSysTable( FCATShadowSystemTable( szTable ) );
            break;

        case tt::ExtentPageCountCache:
            tableclass = TableClassFromExtentPageCountCache();
            break;

        case tt::Temp:
        case tt::Normal:
        {
            // Note the inclusion of temp and normal tables in this branch of the switch.
            
            const ULONG tableClassOffset = ( grbit & JET_bitTableClassMask ) / JET_bitTableClass1;
            if ( tableClassOffset != tableclassNone )
            {
                //  Pointless if that table class has not been set.
                const ULONG paramid = ( tableClassOffset >= 16 ) ?
                    ( ( JET_paramTableClass16Name - 1 ) + ( tableClassOffset - 15 ) ) :
                    ( ( JET_paramTableClass1Name - 1 ) + tableClassOffset );
                if ( !FDefaultParam( paramid ) )
                {
                    tableclass = TableClassFromTableClassOffset( tableClassOffset );
                }
            }
            break;
        }

        default:
            Enforce( fFalse );
    }
    
    if( grbit & JET_bitTablePreread
        || grbit & JET_bitTableSequential )
    {
        //  Preread the root page of the tree
        PIBTraceContextScope tcScope = ppib->InitTraceContextScope( );
        tcScope->nParentObjectClass = TCEFromTableClass(
                                        tableclass,
                                        FFUCBLongValue( pfucb ),
                                        FFUCBSecondary( pfucb ),
                                        FFUCBSpace( pfucb ) );
        tcScope->SetDwEngineObjid( objidTable );

        (void)ErrBFPrereadPage( ifmp, pgnoFDP, bfprfDefault, ppib->BfpriPriority( ifmp ), *tcScope );
    }

    //  if we're opening after table creation, the FCB shouldn't be initialised
    Assert( !( grbit & JET_bitTableCreate ) || !pfcb->FInitialized() );

    // Only one thread could possibly get to this point with an uninitialized
    // FCB, which is why we don't have to grab the FCB's critical section.
    if ( !pfcb->FInitialized() || pfcb->FInitedForRecovery() )
    {
        if ( fInTransaction )
        {
            //  if FCB is not initialised, access to is serialised
            //  at this point, so no more need for transaction
            Call( ErrDIRCommitTransaction( ppib,NO_GRBIT ) );
            fInTransaction = fFalse;
        }

        switch ( ttSubject )
        {
            case tt::System:
                Call( ErrCATInitCatalogFCB( pfucb ) );
                break;

            case tt::Temp:
                Assert( !( grbit & JET_bitTableDelete ) );
                Call( ErrCATInitTempFCB( pfucb ) );
                break;

            case tt::ExtentPageCountCache:
            case tt::Normal:
            {
                const ULONG cPageReadBefore = Ptls()->threadstats.cPageRead;
                const ULONG cPagePrereadBefore = Ptls()->threadstats.cPagePreread;
                
                //  initialize the table's FCB
                //
                Call( ErrCATInitFCB( pfucb, objidTable, !( grbit & JET_bitAllowPgnoFDPLastSetTime ) ) );
                
                const ULONG cPageReadAfter = Ptls()->threadstats.cPageRead;
                const ULONG cPagePrereadAfter = Ptls()->threadstats.cPagePreread;
                
                Assert( cPageReadAfter >= cPageReadBefore );
                PERFOpt( cTableOpenPagesRead.Add( PinstFromPpib( ppib )->m_iInstance, pfcb->TCE(), cPageReadAfter - cPageReadBefore ) );
                
                Assert( cPagePrereadAfter >= cPagePrereadBefore );
                PERFOpt( cTableOpenPagesPreread.Add( PinstFromPpib( ppib )->m_iInstance, pfcb->TCE(), cPagePrereadAfter - cPagePrereadBefore ) );
                
                Assert( pfcb->FTypeTable() );
                //  If out-of-date index checking has been deferred to OpenTable() time, this is the function
                //  that evaluates it.
                wrnSurvives = ErrFILEICheckLocalizedIndexesInTable( ppib, ifmp, pfcb, szTable, grbit );
                Call( wrnSurvives );
                
                //  cache the table name in the catalog hash after it gets initialized
                if ( !( grbit & ( JET_bitTableCreate|JET_bitTableDelete) ) )
                {
                    CATHashInsert( pfcb, pfcb->Ptdb()->SzTableName() );
                }
                
                //  only count "regular" table opens
                //
                if ( ttSubject == tt::Normal )
                {
                    PERFOpt( cTableOpenCacheMisses.Inc( PinstFromPpib( ppib ) ) );
                    PERFOpt( cTablesOpen.Inc( PinstFromPpib( ppib ) ) );
                }
                break;
            }
            default:
                Enforce( fFalse );
        }

        Assert( pfcb->Ptdb() != ptdbNil );

        //  the primary purpose of this failure spot is to validate the error handling path when
        //  fInitialisedCursor is false but the FCB is already in the hash table from CATHashInsert()
        //  above
        //
        Call( ErrFaultInjection( 62880 ) );

        if ( !pfcb->FInitedForRecovery() )
        {
            //  insert the FCB into the global list
            //
            pfcb->InsertList();
        }

        //  we have a full-fledged initialised cursor
        //  (FCB will be marked initialised in CreateComplete()
        //  below, which is guaranteed to succeed)
        //
        fInitialisedCursor = fTrue;

        //  allow other cursors to use this FCB
        //
        pfcb->Lock();
        Assert( !pfcb->FTypeNull() );
        Assert( !pfcb->FInitialized() || pfcb->FInitedForRecovery() );
        Assert( pfcb == pfucb->u.pfcb );

        pfcb->CreateComplete();
        pfcb->ResetInitedForRecovery();

        err = ErrFILEICheckAndSetMode( pfucb, grbit ) + ErrFaultInjection( 38304 );

        if ( err >= JET_errSuccess )
        {
            //  if we faulted in this table, then it indicates that we
            //  don't want it cached when we close it
            //
            if ( grbit & JET_bitTableTryPurgeOnClose )
            {
                pfcb->SetTryPurgeOnClose();
            }
        }
        else
        {
            //  At this point, we've already flagged the FCB as completed, which may be a problem
            //  if this is a new table being created because we'll leave the FCB in the hash table
            //  upon closing the table and it's going to be hashable by a pgnoFDP which will be
            //  reused later since this table creation failed. To avoid that, set the purge-on-close
            //  flag so it gets purged upon cursor-close.
            //
            pfcb->SetTryPurgeOnClose();
        }

        pfcb->Unlock();

        //  check result of ErrFILEICheckAndSetMode
        Call( err );
    }
    else
    {
        if ( ttSubject == tt::Normal 
             && !( grbit & JET_bitTableDelete ) )
        {
            //  only count "regular" table opens
            PERFOpt( cTableOpenCacheHits.Inc( PinstFromPpib( ppib ) ) );
            PERFOpt( cTablesOpen.Inc( PinstFromPpib( ppib ) ) );
        }

        //  we have a full-fledged initialised cursor
        fInitialisedCursor = fTrue;

        //  Check if the OS sort order has changed for localized indices..
        Assert( JET_errSuccess == wrnSurvives );

        if ( ttSubject == tt::Normal )
        {
            Assert( pfcb->FTypeTable() );
            //  If out-of-date index checking has been deferred to OpenTable() time, this is the function
            //  that evaluates it.
            wrnSurvives = ErrFILEICheckLocalizedIndexesInTable( ppib, ifmp, pfcb, szTable, grbit );
        }

        //  check result of ErrFILEICheckLocalizedIndexesInTable
        Call( wrnSurvives );

        //  set table usage mode
        //
        Assert( pfcb == pfucb->u.pfcb );
        pfcb->Lock();

        err = ErrFILEICheckAndSetMode( pfucb, grbit );

        //  this table open did not cause a schema fault,
        //  so don't try to purge it on close
        //
        if ( ( err >= JET_errSuccess ) && !( grbit & JET_bitTableTryPurgeOnClose ) )
        {
            pfcb->ResetTryPurgeOnClose();
        }

        pfcb->Unlock();

        //  check result of ErrFILEICheckAndSetMode
        Call( err );

        if ( fInTransaction )
        {
            Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
            fInTransaction = fFalse;
        }
    }

    //
    //  -------------------    NO FAILURES FROM HERE!!!!    -------------------
    //
    //  At this point, the FCB may have been already changed irreversibly in
    //  cases where state that is not ref-counted by FUCB gets set in the FCB
    //  (and so won't be cleaned up by ErrFILECloseTable()).
    //
    //  Refrain from introducing failure paths from here on.
    //
    //  NOTE: there's already a failure path introduced by
    //  ErrDIRCommitTransaction() above.
    //

    //  System cleanup threads (OLD and RCEClean) are permitted to open
    //  a cursor on a deleted table
    Assert( !pfcb->FDeletePending() || FPIBSessionSystemCleanup( ppib ) );
    Assert( !pfcb->FDeleteCommitted() || FPIBSessionSystemCleanup( ppib ) );

    //  set FUCB for sequential access if requested
    //
    if ( grbit & JET_bitTableSequential )
        FUCBSetSequential( pfucb );
    else
        FUCBResetSequential( pfucb );


    if ( grbit & JET_bitTableOpportuneRead )
        FUCBSetOpportuneRead( pfucb );
    else
        FUCBResetOpportuneRead( pfucb );
        
    // set the Table Class
    //
    if ( pfcb->Ptdb() != ptdbNil )
    {
        pfcb->SetTableclass( tableclass );
    }
    else
    {
        FireWall( "DeprecatedSentinelFcbOpenTableSetTc" ); // Sentinel FCBs are believed deprecated
        Assert( pfcbNil == pfcb->PfcbNextIndex() );
    }

    //  reset copy buffer
    //
    pfucb->pvWorkBuf = NULL;
    pfucb->dataWorkBuf.SetPv( NULL );
    Assert( !FFUCBUpdatePrepared( pfucb ) );

    //  reset key buffer
    //
    pfucb->dataSearchKey.Nullify();
    pfucb->cColumnsInSearchKey = 0;
    KSReset( pfucb );

    FUCBSetMayCacheLVCursor( pfucb );

    //  move currency to the first record and ignore error if no records
    //
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
        FireWall( "DeprecatedSentinelFcbOpenTableMoveCur" ); // Sentinel FCBs are believed deprecated
        Assert( pfcbNil == pfcb->PfcbNextIndex() );
    }
#endif

    Assert( !fInTransaction );
    AssertDIRMaybeNoLatch( ppib, pfucb );
    *ppfucb = pfucb;
    
    //  Be sure to return the error from ErrFILEICheckAndSetMode()
    CallSx( err, JET_wrnTableInUseBySystem );

    if ( wrnSurvives > JET_errSuccess )
    {
        //  We shouldn't be clobbering JET_wrnTableInUseBySystem, because
        //  ErrIsamOpenTable is supposed to block the grbit that allows
        //  JET_wrnTableInUseBySystem to be returned.
        Assert( err == JET_errSuccess );
    }

    //  Restore any warnings that may have been subsequently overwritten.
    if ( JET_errSuccess == err && wrnSurvives > JET_errSuccess )
    {
        err = wrnSurvives;
    }

    //  Not all paths opening derived tables will explicitly set the flag
    //  to signal static-template because we rely on the assumption that
    //  the first time the derived gets opened or at the time it gets created,
    //  the flag will be set. Therefore, the flag is guaranteed to be set
    //  at this point regardless of it being the first time or not.
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
    AssertDIRMaybeNoLatch( ppib, NULL );

    return err;
}

//  This routine takes multiple table names and issues prefetches for all the pgnoFDPs of all
//  the tables. It crushes part of my soul to write this API instead of DT (Deferred Tables).

ERR ErrIsamPrereadTables( _In_ JET_SESID sesid, _In_ JET_DBID vdbid, __in_ecount( cwszTables ) PCWSTR * rgwszTables, _In_ INT cwszTables, JET_GRBIT grbit )
{
    ERR err = JET_errSuccess;
    PIB * const ppib = (PIB*)sesid;
    const IFMP ifmp = (IFMP)vdbid;

    PGNO    rgpgno[11]; // limit preread 10 tables at once, plus pgnoNull terminator

    if ( grbit )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    if ( cwszTables > ( _countof(rgpgno) - 1 /* leave room for pgnoNull terminator */ ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  First walk through all the table names and retrieve all the pgnoFDPs for
    //  all the tables provided.
    //

    for( INT iTable = 0; iTable < cwszTables; iTable++ )
    {
        CAutoSZDDL  lszTableName;
        OBJID objidTableT;

        rgpgno[iTable] = pgnoNull;

        Call( lszTableName.ErrSet( rgwszTables[iTable] ) );
        
        //  lookup the table in the catalog hash before seeking
        if ( !FCATHashLookup( ifmp, lszTableName, &(rgpgno[iTable]), &objidTableT ) )
        {
            (void)ErrCATSeekTable( ppib, ifmp, lszTableName, &(rgpgno[iTable]), &objidTableT );
            // on error (esp. for such as JET_errObjectNotFound) no big deal, just don't preread
            // that table.
        }
    }

    //  Second transmorgify the the array to be appropriate for pre-reading, it 
    //  should be pgnoNull terminated, sorted and have no "trim out" any pgnoNulls 
    //  at the beginning of the array.
    //

    //  null terminate the array.
    rgpgno[cwszTables] = pgnoNull;

    //  sort the array.
    sort( rgpgno, rgpgno + cwszTables );
    // note: we have effectively sorted any pgnoNull's to the beginning.

    //  find the first valid pgno in the sorted rgpgno array 
    INT iFirstTable = cwszTables;
    for( INT iTable = 0; iTable < cwszTables + 1; iTable++ )
    {
        if ( iFirstTable == cwszTables && rgpgno[iTable] != pgnoNull )
        {
            iFirstTable = iTable;
            break;
        }
    }

    //  Third / lastly preread the list of pgnos we need (if there are any).
    //

    if ( iFirstTable < cwszTables )
    {
        // Note: No TCE provided. Difficult since we're dealing with multiple tables. :P
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

    //  reset pfucb which was exported as tableid
    //
    pfucb->pvtfndef = &vtfndefInvalidTableid;
    err = ErrFILECloseTable( ppib, pfucb );
    return err;
}


//+API
//  ErrFILECloseTable
//  ========================================================================
//  ErrFILECloseTable( PIB *ppib, FUCB *pfucb )
//
//  Closes the FUCB of a data file, previously opened using FILEOpen.
//  Also closes the current secondary index, if any.
//
//  PARAMETERS  ppib    PIB of this user
//              pfucb   FUCB of file to close
//
//  RETURNS     JET_errSuccess
//              or lower level errors
//
//  SEE ALSO    ErrFILEIOpenTable
//-
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
        //  only count "regular" table closes
        PERFOpt( cTablesCloses.Inc( PinstFromPpib( ppib ) ) );
        PERFOpt( cTablesOpen.Dec( PinstFromPpib( ppib ) ) );
    }

    //  reset the index-range in case this cursor
    //  is used for a rollback
    DIRResetIndexRange( pfucb );

    //  release working buffer and search key
    //
    RECIFreeCopyBuffer( pfucb );

    RECReleaseKeySearchBuffer( pfucb );

    //  detach, close and free index/lv FUCB, if any
    //
    DIRCloseIfExists( &pfucb->pfucbCurIndex );
    DIRCloseIfExists( &pfucb->pfucbLV );

    //  if closing a temporary table, free resources if
    //  last one to close.
    //
    if ( pfucb->u.pfcb->FTypeTemporaryTable() )
    {
        INT     wRefCnt;
        FUCB    *pfucbT;

        Assert( FFMPIsTempDB( pfcb->Ifmp() ) );
        Assert( pfcb->Ptdb() != ptdbNil );
        Assert( pfcb->FFixedDDL() );
        DIRClose( pfucb );

        //  We may have deferred close cursors on the temporary table.
        //  If one or more cursors are open, then temporary table
        //  should not be deleted.
        //
        pfucbT = ppib->pfucbOfSession;
        wRefCnt = pfcb->WRefCount();
        while ( wRefCnt > 0 && pfucbT != pfucbNil )
        {
            Assert( pfucbT->ppib == ppib ); // We should be the only one with access to the temp. table.
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

            // Shouldn't be any index FDP's to free, since we don't
            // currently support secondary indexes on temp. tables.
            Assert( pfcb->PfcbNextIndex() == pfcbNil );

            // We nullify temp table RCEs at commit time so there should be
            // no RCEs on the FCBs except uncommitted ones

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

            //  prepare the FCB to be purged
            //  this removes the FCB from the hash-table among other things
            //      so that the following case cannot happen:
            //          we free the space for this FCb
            //          someone else allocates it
            //          someone else BTOpen's the space
            //          we try to purge the table and find that the refcnt
            //              is not zero and the state of the FCB says it is
            //              currently in use!
            //          result --> CONCURRENCY HOLE

            pfcb->PrepareForPurge( fFalse );

            //  if fail to delete temporary table, then lose space until
            //  termination.  Temporary database is deleted on termination
            //  and space is reclaimed.  This error should be rare, and
            //  can be caused by resource failure.

            // Under most circumstances, this should not fail.  Since we check
            // the RefCnt beforehand, we should never fail with JET_errTableLocked
            // or JET_errTableInUse.  If we do, it's indicative of a
            // concurrency problem.
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

    //  all cursors on table must be set to vtfndefIsamMustRollback
    //  or vtfndefTTBaseMustRollback
    //
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
    _In_ const JET_SPACEHINTS * const pjsph,
    _Out_ FCB * pfcbTemplateIndex )
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
        // Recovery creates all FCBs as table FCB, now that we know better, we need to remove from list of table FCBs
        // before we mark FCB as being a secondary index
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

        //  Note: If this was a derived index like this, then ErrCATIInitIDB()
        //  already grabbed the space hints from the template FCB.
        pfcbNew->SetSpaceHints( pjsph );

        if ( !pfcbTemplateIndex->Pidb()->FLocalizedText() )
        {
            pfcbNew->SetPidb( pfcbTemplateIndex->Pidb() );
        }
        else
        {
            // For UNICODE indexes, uses its own IDB instance to support possible
            // multiple NLS versions among derived tables.
            Call( ErrFILEIGenerateIDB( pfcbNew, ptdb, pidb ) );

            // Mark the IDB object owned by the FCB.
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

            //  primary index should allow sufficient record space for all fixed columns
            //
            Assert( pfcbNew->Ifmp() != 0 );
            Assert( !fPrimary || ( ptdb->IbEndFixedColumns() <= REC::CbRecordMost( g_rgfmp[ pfcbNew->Ifmp() ].CbPage(), pidb ) ) );
        }
    }

    Assert( err >= 0 );
    return err;

HandleError:

    Assert( err < 0 );
    Assert( pfcbNew->Pidb() == pidbNil );       // Verify IDB not allocated.
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

// To build a default record, we need a fake FUCB and FCB for RECSetColumn().
// We also need to allocate a temporary buffer in which to store the default
// record.
VOID FILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, TDB *ptdb )
{
    Assert( ptdbNil != ptdb );
    pfcbFake->SetPtdb( ptdb );          // Attach a real TDB and a fake FCB.
    pfucbFake->u.pfcb = pfcbFake;
    FUCBSetIndex( pfucbFake );

    pfcbFake->ResetFlags();
    pfcbFake->SetTypeTable();           // Ensures SetColumn doesn't need crit. sect.

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

        // If template table exists, use its default record.
        Assert( ptdbTemplate != ptdbNil );
        Assert( NULL != ptdbTemplate->PdataDefaultRecord() );
        ptdbTemplate->PdataDefaultRecord()->CopyInto( pfucbFake->dataWorkBuf );

        //  update derived bit of all tagged columns
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

        // Start with an empty record.
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
            && !FFIELDCommittedDelete( pfield->ffield ) )   //  make sure column not deleted
        {
            Assert( JET_coltypNil != pfield->coltyp );
            Call( ErrFILERebuildOneDefaultValue(
                        pfucbFake,
                        columnid,
                        columnidToAdd,
                        pdataDefaultToAdd ) );
        }
    }

    //  in case we have to chain together the buffers (to keep
    //  around copies of previous of old default records
    //  because other threads may have stale pointers),
    //  allocate a RECDANGLING buffer to preface the actual
    //  default record
    //
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


//  combines all index column masks into a single per table
//  index mask, used for index update check skip.
//
VOID FILESetAllIndexMask( FCB *pfcbTable )
{
    Assert( ptdbNil != pfcbTable->Ptdb() );

    FCB *                   pfcbT;
    TDB *                   ptdb        = pfcbTable->Ptdb();
    const LONG_PTR * const  plMax       = (LONG_PTR *)ptdb->RgbitAllIndex()
                                            + ( cbRgbitIndex / sizeof(LONG_PTR) );

    //  initialize mask to primary index, or to 0s for sequential file.
    //
    if ( pfcbTable->Pidb() != pidbNil )
    {
        ptdb->SetRgbitAllIndex( pfcbTable->Pidb()->RgbitIdx() );
    }
    else
    {
        ptdb->ResetRgbitAllIndex();
    }

    //  for each secondary index, combine index mask with all index
    //  mask.  Also, combine has tagged flag.
    //
    for ( pfcbT = pfcbTable->PfcbNextIndex();
        pfcbT != pfcbNil;
        pfcbT = pfcbT->PfcbNextIndex() )
    {
        Assert( pfcbT->Pidb() != pidbNil );

        // Only process non-deleted indexes (or deleted but versioned).
        //
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

        // Array is too big to fit into IDB, so put into TDB's byte pool instead.
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

        // Array is too big to fit into IDB, so put into TDB's byte pool instead.
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

    //  If it is on little endian machine, we still copy it into
    //  the stack array which is aligned.
    //  If it is on big endian machine, we always need to convert first.

    for ( UINT iidxseg = 0; iidxseg < cidxseg; iidxseg++ )
    {
        //  Endian conversion
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
///     rgidxseg[iidxseg].SetFOldFormat();

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

            //  WARNING: the fidLast's were set based on what was found
            //  in the derived table, so if any are equal to fidLeast-1,
            //  it actually means there were no columns in the derived
            //  table and hence the column must belong to the template
            if ( fid.FTagged() )
            {
                if ( ptcibTemplateTable->fidTaggedLast.FTaggedNone()
                    || fid <= ptcibTemplateTable->fidTaggedLast )
                    rgidxseg[iidxseg].SetFTemplateColumn();
            }
            else if ( fid.FFixed() )
            {
                if ( ptcibTemplateTable->fidFixedLast.FFixedNone()
                    || fid <= ptcibTemplateTable->fidFixedLast )
                    rgidxseg[iidxseg].SetFTemplateColumn();
            }
            else
            {
                Assert( fid.FVar() );
                if ( ptcibTemplateTable->fidVarLast.FVarNone()
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
            // Must retrieve from the template table's TDB.
            ptdb->AssertValidDerivedTable();
            pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb->PfcbTemplateTable()->Ptdb() );
            return pidxseg;
        }

        // If marked as a template index, but pfcbTemplateTable is NULL,
        // then this must already be the TDB for the template table.
    }

    if ( pidb->FIsRgidxsegInMempool() )
    {
        Assert( pidb->ItagRgidxseg() != 0 );
        Assert( ptdb->MemPool().CbGetEntry( pidb->ItagRgidxseg() ) == pidb->Cidxseg() * sizeof(IDXSEG) );
        pidxseg = (IDXSEG*)ptdb->MemPool().PbGetEntry( pidb->ItagRgidxseg() );
    }
    else
    {
        Assert( pidb->Cidxseg() > 0 );      // Must be at least one segment.
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
            // Must retrieve from the template table's TDB.
            ptdb->AssertValidDerivedTable();
            pidxseg = PidxsegIDBGetIdxSegConditional( pidb, ptdb->PfcbTemplateTable()->Ptdb() );
            return pidxseg;
        }

        // If marked as a template index, but pfcbTemplateTable is NULL,
        // then this must already be the TDB for the template table.
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


