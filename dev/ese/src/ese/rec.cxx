// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

const WORD ibRECStartFixedColumns   = REC::cbRecordMin;

static const JET_COLUMNDEF rgcolumndefJoinlist[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
};


/*=================================================================
ErrIsamGetLock

Description:
    get lock on the record from the specified file.

Parameters:

    PIB         *ppib           PIB of user
    FUCB        *pfucb          FUCB for file
    JET_GRBIT   grbit           options

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamGetLock( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
{
    PIB     *ppib = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );
    ERR     err;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    if ( ppib->Level() <= 0 )
        return ErrERRCheck( JET_errNotInTransaction );

    if ( JET_bitReadLock == grbit )
    {
        Call( ErrDIRGetLock( pfucb, readLock ) );
    }
    else if ( JET_bitWriteLock == grbit )
    {
        //  ensure that table is updatable
        //
        CallR( ErrFUCBCheckUpdatable( pfucb )  );
        if ( !FFMPIsTempDB( pfucb->ifmp ) )
        {
            CallR( ErrPIBCheckUpdatable( ppib ) );
        }

        Call( ErrDIRGetLock( pfucb, writeLock ) );
    }
    else
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
    }

HandleError:
    return err;
}

//  adds one move filter to the end of the cursor's move filter stack
VOID RECAddMoveFilter(
    FUCB * const                pfucb,
    PFN_MOVE_FILTER const       pfnMoveFilter,
    MOVE_FILTER_CONTEXT * const pmoveFilterContext )
{
    Assert( pfucbNil != pfucb );

    pmoveFilterContext->pfnMoveFilter = pfnMoveFilter;
    pmoveFilterContext->pmoveFilterContextPrev = pfucb->pmoveFilterContext;
    pfucb->pmoveFilterContext = pmoveFilterContext;
}

//  removes one move filter from the cursor's move filter stack
VOID RECRemoveMoveFilter(
    FUCB * const                    pfucb,
    PFN_MOVE_FILTER const           pfnMoveFilter,
    MOVE_FILTER_CONTEXT ** const    ppmoveFilterContextRemoved )
{
    MOVE_FILTER_CONTEXT *   pmoveFilterContextRemovedT;
    MOVE_FILTER_CONTEXT *&  pmoveFilterContextRemoved   = ppmoveFilterContextRemoved ? *ppmoveFilterContextRemoved : pmoveFilterContextRemovedT;
                            pmoveFilterContextRemoved   = NULL;

    Assert( pfucbNil != pfucb );

    MOVE_FILTER_CONTEXT** ppmoveFilterContext = NULL;
    for (   ppmoveFilterContext = &pfucb->pmoveFilterContext;
            *ppmoveFilterContext && ( *ppmoveFilterContext )->pfnMoveFilter != pfnMoveFilter;
            ppmoveFilterContext = &( *ppmoveFilterContext )->pmoveFilterContextPrev )
    {
    }
    if ( *ppmoveFilterContext )
    {
        pmoveFilterContextRemoved = *ppmoveFilterContext;
        *ppmoveFilterContext = (*ppmoveFilterContext )->pmoveFilterContextPrev;
    }
}

//  checks the current move filter and invokes the previous filter if any
ERR ErrRECCheckMoveFilter( FUCB * const pfucb, MOVE_FILTER_CONTEXT * const pmoveFilterContext )
{
    ERR err = JET_errSuccess;

    Assert( pfucbNil != pfucb );
    Assert( NULL != pmoveFilterContext );

    ASSERT_VALID( pfucb );
    Assert( pfucb->u.pfcb->FTypeTable() || pfucb->u.pfcb->FTypeSecondaryIndex() );
    AssertNDGet( pfucb );

    if ( pmoveFilterContext->pmoveFilterContextPrev )
    {
        Call( pmoveFilterContext->pmoveFilterContextPrev->pfnMoveFilter( pfucb, pmoveFilterContext->pmoveFilterContextPrev ) );
    }

HandleError:
    return err;
}

struct NORMALIZED_FILTER_COLUMN : public JET_INDEX_COLUMN
{
    JET_COLTYP  coltyp;         //  the column type of the filter value
    BOOL        fNormalized;    //  was the filter value normalized?
};

struct CURSOR_FILTER_CONTEXT : public MOVE_FILTER_CONTEXT
{
    NORMALIZED_FILTER_COLUMN *  rgFilters;  //  array of column value filters to apply during lateral btree navigation
    DWORD                       cFilters;   //  number of column value filters to apply during lateral btree navigation
};

//  =================================================================
//  PURPOSE:
//      Determines whether the current record satisfies a simple
//      filter condition.
//
//  PARAMETERS:
//      pfucb           - cursor positioned on a record
//
//  RETURNS:
//      JET_errSuccess:           record matches the filter
//      wrnBTNotVisibleRejected:  record doesn't match the filter
//  =================================================================
LOCAL ERR ErrRECICheckCursorFilter( FUCB * const pfucb, CURSOR_FILTER_CONTEXT * const pcursorFilterContext )
{
    ERR     err         = JET_errSuccess;
    BOOL    fMatch      = fTrue;
    DATA    dataField;

    //  check any higher priority filter
    Call( ErrRECCheckMoveFilter( pfucb, (MOVE_FILTER_CONTEXT * const)pcursorFilterContext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }

    //  take .Net GUID normalization flag from current index
    //
    BOOL    fDotNetGuid = fFalse;
    if ( pfucb->pfucbCurIndex != NULL )
    {
        fDotNetGuid = pfucb->pfucbCurIndex->u.pfcb->Pidb()->FDotNetGuid();
    }
    else if ( pfucb->u.pfcb->Pidb() != NULL )
    {
        fDotNetGuid = pfucb->u.pfcb->Pidb()->FDotNetGuid();
    }

    for ( DWORD i = 0; i < pcursorFilterContext->cFilters; i++ )
    {
        const NORMALIZED_FILTER_COLUMN * const  pFilter     = &pcursorFilterContext->rgFilters[i];
        const BOOL                              fFilterNull = ( pFilter->cb == 0 && !( pFilter->grbit & JET_bitZeroLength ) );

        if ( FCOLUMNIDTagged( pFilter->columnid ) )
        {
            Call( ErrRECRetrieveTaggedColumn(
                pfucb->u.pfcb,
                pFilter->columnid,
                1,  // for multi-values, we only support validating the first multi-value
                pfucb->kdfCurr.data,
                &dataField,
                NO_GRBIT ) );
        }
        else
        {
            Assert( FCOLUMNIDFixed( pFilter->columnid ) || FCOLUMNIDVar( pFilter->columnid ) );
            Call( ErrRECRetrieveNonTaggedColumn(
                pfucb->u.pfcb,
                pFilter->columnid,
                pfucb->kdfCurr.data,
                &dataField,
                NULL /* pfieldFixed */ ) ); //  this is only required as a perf optimisation to avoid grabbing the DML latch
        }

        CallSx( err, JET_wrnColumnNull );
        if ( JET_errSuccess == err )
        {
            const BYTE * const  pvFilter    = (BYTE *)pFilter->pv;
            const INT           cbFilter    = pFilter->cb;
            BYTE                *pvData;
            INT                 cbData;
            BYTE                rgbNormData[ sizeof(GUID) + 1 ];    //  largest column type we should be normalising is a guid, and +1 for the prefix/header byte

            //  check if the filter value was normalised
            //
            if ( pFilter->fNormalized )
            {
                Assert( !FRECLongValue( pFilter->coltyp ) );
                Assert( !FRECTextColumn( pFilter->coltyp ) );
                Assert( !FRECBinaryColumn( pFilter->coltyp ) );
                Assert( pFilter->cb > 0 );

                //  the filter value was normalised, so we'll
                //  need to normalise the data value as well
                //
                FLDNormalizeFixedSegment(
                    (BYTE *)dataField.Pv(),
                    dataField.Cb(),
                    rgbNormData,
                    &cbData,
                    pFilter->coltyp,
                    fDotNetGuid );
                Assert( cbData <= sizeof(rgbNormData) );
                pvData = rgbNormData;
            }
            else
            {
                //  no normalisation needed
                //
                pvData = (BYTE *)dataField.Pv();
                cbData = dataField.Cb();
            }

            switch ( pFilter->relop )
            {
                case JET_relopEquals:
                    fMatch = ( !fFilterNull &&
                               FDataEqual( pvData, cbData, pvFilter, cbFilter ) );
                    break;
                case JET_relopPrefixEquals:
                    fMatch = ( cbData >= cbFilter &&
                               FDataEqual( pvData, cbFilter, pvFilter, cbFilter ) );
                    break;
                case JET_relopNotEquals:
                    fMatch = ( fFilterNull ||
                               !FDataEqual( pvData, cbData, pvFilter, cbFilter ) );
                    break;
                case JET_relopLessThanOrEqual:
                    fMatch = ( !fFilterNull &&
                               CmpData( pvData, cbData, pvFilter, cbFilter ) <= 0 );
                    break;
                case JET_relopLessThan:
                    fMatch = ( CmpData( pvData, cbData, pvFilter, cbFilter ) < 0 );
                    break;
                case JET_relopGreaterThanOrEqual:
                    fMatch = ( CmpData( pvData, cbData, pvFilter, cbFilter ) >= 0 );
                    break;
                case JET_relopGreaterThan:
                    fMatch = ( fFilterNull ||
                               CmpData( pvData, cbData, pvFilter, cbFilter ) > 0 );
                    break;
                case JET_relopBitmaskEqualsZero:
                    Assert( cbData == sizeof(BYTE) || cbData == sizeof(USHORT) || cbData == sizeof(ULONG) || cbData == sizeof(QWORD) );
                    Assert( cbFilter == cbData );
                    switch ( cbData )
                    {
                        case sizeof(BYTE):
                            fMatch = ( 0 == ( *(BYTE *)pvData & *(BYTE *)pvFilter ) );
                            break;
                        case sizeof(USHORT):
                            fMatch = ( 0 == ( *(USHORT *)pvData & *(USHORT *)pvFilter ) );
                            break;
                        case sizeof(ULONG):
                            fMatch = ( 0 == ( *(ULONG *)pvData & *(ULONG *)pvFilter ) );
                            break;
                        case sizeof(QWORD):
                            fMatch = ( 0 == ( *(QWORD *)pvData & *(QWORD *)pvFilter ) );
                            break;
                    }
                    break;
                case JET_relopBitmaskNotEqualsZero:
                    Assert( cbData == sizeof(BYTE) || cbData == sizeof(USHORT) || cbData == sizeof(ULONG) || cbData == sizeof(QWORD) );
                    Assert( cbFilter == cbData );
                    switch ( cbData )
                    {
                        case sizeof(BYTE):
                            fMatch = ( 0 != ( *(BYTE *)pvData & *(BYTE *)pvFilter ) );
                            break;
                        case sizeof(USHORT):
                            fMatch = ( 0 != ( *(USHORT *)pvData & *(USHORT *)pvFilter ) );
                            break;
                        case sizeof(ULONG):
                            fMatch = ( 0 != ( *(ULONG *)pvData & *(ULONG *)pvFilter ) );
                            break;
                        case sizeof(QWORD):
                            fMatch = ( 0 != ( *(QWORD *)pvData & *(QWORD *)pvFilter ) );
                            break;
                    }
                    break;
                default:
                    //  in unexpected cases, assume the record matches (so we don't filter it out)
                    //
                    AssertSz( fFalse, "Unrecognized relop for non-null column." );
                    fMatch = fTrue;
            }
        }
        else if ( JET_wrnColumnNull == err )
        {
            //  column was null in the record, so
            //  compare that with the specified
            //  filter
            //
            switch ( pFilter->relop )
            {
                case JET_relopEquals:
                    fMatch = fFilterNull;
                    break;
                case JET_relopPrefixEquals:
                    // null, so cannot be a prefix match
                    //
                    fMatch = fFalse;
                    break;
                case JET_relopNotEquals:
                    fMatch = !fFilterNull;
                    break;
                case JET_relopLessThanOrEqual:
                    //  null is considered less than everything,
                    //  so consider this a match
                    //
                    fMatch = fTrue;
                    break;
                case JET_relopLessThan:
                    fMatch = !fFilterNull;
                    break;
                case JET_relopGreaterThanOrEqual:
                    fMatch = fFilterNull;
                    break;
                case JET_relopGreaterThan:
                    //  null is considered less than everything,
                    //  so this can't be a match
                    //
                    fMatch = fFalse;
                    break;
                case JET_relopBitmaskEqualsZero:
                    //  since column was null, it didn't
                    //  contain any of the bits in the
                    //  bitmask, so consider it a match
                    //
                    fMatch = fTrue;
                    break;
                case JET_relopBitmaskNotEqualsZero:
                    //  since column was null, it can't
                    //  contain any of the bits in the
                    //  bitmask, so consider it not a
                    //  match
                    fMatch = fFalse;
                    break;
                default:
                    //  in unexpected cases, assume the record matches (so we don't filter it out)
                    //
                    AssertSz( fFalse, "Unrecognized relop for null column." );
                    fMatch = fTrue;
            }
        }
        else
        {
            //  unexpected error from column retrieval - looks like
            //  we hit some unforeseen/unsupported scenario
            //
            Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
        }

        if ( !fMatch )
        {
            break;
        }
    }

    //  ignore the record if it is not a match
    //
    err = fMatch ? JET_errSuccess : wrnBTNotVisibleRejected;
    
HandleError:
    return err;
}

VOID RECRemoveCursorFilter( FUCB * const pfucb )
{
    CURSOR_FILTER_CONTEXT* pcursorFilterContext = NULL;
    RECRemoveMoveFilter( pfucb, (PFN_MOVE_FILTER)ErrRECICheckCursorFilter, (MOVE_FILTER_CONTEXT**)&pcursorFilterContext );
    if ( pcursorFilterContext )
    {
        delete[] pcursorFilterContext->rgFilters;
        delete pcursorFilterContext;
    }
}

//  =================================================================
//  PURPOSE:
//      Determines whether the specified simple filter is valid for
//      a filtered move operation. Also determins the column type
//      of the filter value and returns it as an OUT parameter.
//
//  PARAMETERS:
//      pfucb       - table cursor on which the filtered move operation will be applied
//      pFilter     - the simple filters to validate
//      pcoltyp     - OUT param to be filled in with the column type of the filter value
//  =================================================================
ERR ErrRECIValidateOneMoveFilter(
    FUCB *                          pfucb,
    const JET_INDEX_COLUMN * const  pFilter,
    JET_COLTYP *                    pcoltyp )
{
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfucb->u.pfcb->FTypeTable() );
    Assert( ptdbNil != pfucb->u.pfcb->Ptdb() );

    ERR                 err             = JET_errSuccess;
    FCB * const         pfcb            = pfucb->u.pfcb;
    const COLUMNID      columnid        = pFilter->columnid;
    const FID           fid             = FidOfColumnid( columnid );
    const TDB * const   ptdb            = pfcb->Ptdb();
    BOOL                fUseDMLLatch    = fFalse;
    const FIELD         *pfield         = pfieldNil;
    JET_COLTYP          coltyp          = JET_coltypNil;
    FIELDFLAG           ffield          = 0;

    if ( ( ( pFilter->grbit & ~JET_bitZeroLength ) != 0 ) ||
         ( pFilter->cb == 0 && pFilter->pv != NULL ) ||
         ( pFilter->cb != 0 && ( pFilter->pv == NULL || pFilter->grbit == JET_bitZeroLength ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  verify column visibility
    //
    Call( ErrRECIAccessColumn( pfucb, columnid ) );

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        //  DDL is fixed on template tables, so DML latch
        //  is unnecessary
        //
        fUseDMLLatch = fFalse;
    }
    else if ( fid.FFixed() )
    {
        fUseDMLLatch = ( fid > ptdb->FidFixedLastInitial() );
    }
    else if ( fid.FVar() )
    {
        fUseDMLLatch = ( fid > ptdb->FidVarLastInitial() );
    }
    else
    {
        Assert( fid.FTagged() );
        fUseDMLLatch = ( fid > ptdb->FidTaggedLastInitial() );
    }

    if ( fUseDMLLatch )
    {
        pfcb->EnterDML();
    }

    pfield = ptdb->Pfield( columnid );
    coltyp = pfield->coltyp;
    ffield = pfield->ffield;

    if ( fUseDMLLatch )
    {
        pfcb->LeaveDML();
    }

    if ( FRECLongValue( coltyp ) || FRECTextColumn( coltyp ) || FFIELDUserDefinedDefault( ffield ) )
    {
        //  don't currently support long values, text values or columns
        //  with user-defined default values.
        //
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }
    else if ( ( pFilter->relop == JET_relopPrefixEquals ||
                pFilter->grbit & JET_bitZeroLength ) &&
              coltyp != JET_coltypBinary )
    {
        //  prefix-match and ZeroLength only make sense for
        //  binary columns
        //
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    else if ( pFilter->cb == 0 )
    {
        if ( pFilter->relop == JET_relopBitmaskEqualsZero ||
            pFilter->relop == JET_relopBitmaskNotEqualsZero ||
            pFilter->relop == JET_relopPrefixEquals )
        {
            //  bitmask must be non null, and it doesn't
            //  make sense to prefix-match to a null/zero-length
            //  value
            //
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else if ( coltyp != JET_coltypBinary && pFilter->cb != UlCATColumnSize( coltyp, 0, NULL ) )
    {
        //  for non-binary columns, if the specified size is non-zero,
        //  it must match the size of the column type
        //
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    else if ( ( coltyp == JET_coltypBit ||
                coltyp == JET_coltypGUID ||
                coltyp == JET_coltypBinary ) &&
              ( pFilter->relop == JET_relopBitmaskEqualsZero ||
                pFilter->relop == JET_relopBitmaskNotEqualsZero ) )
    {
        //  bitmask must be integer type
        //
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

HandleError:
    //  set OUT param
    //
    *pcoltyp = coltyp;

    return err;
}

ERR ErrRECSetCursorFilter(
    FUCB                *pfucb,
    JET_INDEX_COLUMN    *rgFilters,
    const DWORD         cFilters )
{
    ERR                     err                     = JET_errSuccess;
    JET_COLTYP              coltyp                  = JET_coltypNil;
    CURSOR_FILTER_CONTEXT*  pcursorFilterContext    = NULL;

    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );
    AssertSz( pfucb->u.pfcb->FTypeTable(), "Currently only support move filters on table cursors." );
    AssertSz( pfucbNil == pfucb->pfucbCurIndex, "Table cursor must be in primary index in order to use a move filter." );

    //  take .Net GUID normalization flag from current index
    //
    BOOL                fDotNetGuid = fFalse;
    if ( pfucb->pfucbCurIndex != NULL )
    {
        fDotNetGuid = pfucb->pfucbCurIndex->u.pfcb->Pidb()->FDotNetGuid();
    }
    else if ( pfucb->u.pfcb->Pidb() != NULL )
    {
        fDotNetGuid = pfucb->u.pfcb->Pidb()->FDotNetGuid();
    }

    //  before doing anything else, remove any existing filter on the cursor
    //
    RECRemoveCursorFilter( pfucb );

    if ( cFilters == 0 )
    {
        return JET_errSuccess;
    }

    DWORD   cbNeeded    = sizeof(NORMALIZED_FILTER_COLUMN) * cFilters;
    for ( DWORD i = 0; i < cFilters; i++ )
    {
        //  add an extra byte in case the filter value needs to be normalised
        //  (because a prefix/header byte will be pre-pended to the normalised
        //  value)
        //
        cbNeeded += rgFilters[i].cb + 1;
    }

    Alloc( pcursorFilterContext = new CURSOR_FILTER_CONTEXT() );
    Alloc( pcursorFilterContext->rgFilters = (NORMALIZED_FILTER_COLUMN *)PvOSMemoryHeapAlloc( cbNeeded ) );

    BYTE    *pBuffer    = (BYTE *)( pcursorFilterContext->rgFilters + cFilters);
    for ( DWORD i = 0; i < cFilters; i++ )
    {
        const JET_INDEX_COLUMN * const  pFilter = rgFilters + i;

        //  first, validate the filter value
        //
        Call( ErrRECIValidateOneMoveFilter( pfucb, pFilter, &coltyp ) );

        //  next, copy the filter meta-data
        //
        memcpy( &pcursorFilterContext->rgFilters[i], pFilter, sizeof(JET_INDEX_COLUMN) );

        //  next, determine if normalisation is necessary (initially
        //  assuming it's not)
        //
        pcursorFilterContext->rgFilters[i].coltyp = coltyp;
        pcursorFilterContext->rgFilters[i].fNormalized = fFalse;

        //  we don't need to normalise binary values or
        //  null/zero-length values
        //
        if ( coltyp != JET_coltypBinary && pFilter->cb != 0 )
        {
            //  we need to normalize coltypBit because we need
            //  to convert all non-zero values to 0xFF, and we
            //  need to normalize for relops involving relative
            //  comparisons in order to properly reconcile the
            //  comparison
            //
            if ( coltyp == JET_coltypBit ||
                pFilter->relop == JET_relopLessThanOrEqual ||
                pFilter->relop == JET_relopLessThan ||
                pFilter->relop == JET_relopGreaterThanOrEqual ||
                pFilter->relop == JET_relopGreaterThan )
            {
                pcursorFilterContext->rgFilters[i].fNormalized = fTrue;
            }
        }

        //  copy the filter value into the cursor (normalising first if needed)
        //
        if ( pcursorFilterContext->rgFilters[i].fNormalized )
        {
            INT cbNormalizedValue   = 0;

            Assert( !FRECLongValue( coltyp ) );
            Assert( !FRECTextColumn( coltyp ) );
            Assert( !FRECBinaryColumn( coltyp ) );
            Assert( pFilter->cb > 0 );

            //  normalise the filter value, storing the
            //  normalized value in the cursor
            //
            FLDNormalizeFixedSegment(
                (BYTE *)pFilter->pv,
                pFilter->cb,
                pBuffer,
                &cbNormalizedValue,
                coltyp,
                fDotNetGuid,
                fTrue );
            Assert( cbNormalizedValue <= sizeof(GUID) + 1 );    //  largest column type we should be normalising is a guid, and +1 for the prefix/header byte
            pcursorFilterContext->rgFilters[i].pv = pBuffer;
            pcursorFilterContext->rgFilters[i].cb = cbNormalizedValue;
            pBuffer += cbNormalizedValue;
        }

        else if ( rgFilters[i].cb != 0 )
        {
            //  normalisation isn't needed, so just copy
            //  the raw filter value
            //
            memcpy( pBuffer, rgFilters[i].pv, rgFilters[i].cb );
            pcursorFilterContext->rgFilters[i].pv = pBuffer;
            Assert( pcursorFilterContext->rgFilters[i].cb == rgFilters[i].cb );
            pBuffer += rgFilters[i].cb;
        }
        else
        {
            //  filter value is null or zero-length, so there's
            //  nothing to copy, but set the pointer to NULL
            //  for safety
            //
            pcursorFilterContext->rgFilters[i].pv = NULL;
            Assert( pcursorFilterContext->rgFilters[i].cb == 0 );
        }

        Assert( pBuffer <= (BYTE *)pcursorFilterContext->rgFilters + cbNeeded );
    }

    pcursorFilterContext->cFilters = cFilters;

    RECAddMoveFilter( pfucb, (PFN_MOVE_FILTER)ErrRECICheckCursorFilter, pcursorFilterContext );
    pcursorFilterContext = NULL;

HandleError:
    if ( err < JET_errSuccess )
    {
        RECRemoveCursorFilter( pfucb );
    }
    if ( pcursorFilterContext )
    {
        delete[] pcursorFilterContext->rgFilters;
        delete pcursorFilterContext;
    }
    return err;
}

/*=================================================================
ErrIsamSetCursorFilter

Description:
    Sets the filter to be used by JetMove

Parameters:

    PIB                 *ppib           PIB of user
    FUCB                *pfucb          FUCB for file
    JET_INDEX_COLUMN    *rgFilters      filters to apply
    DWORD               cFilters        number of filters
    JET_GRBIT           grbit           options

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamSetCursorFilter( JET_SESID sesid, JET_VTID vtid, JET_INDEX_COLUMN * rgFilters, DWORD cFilters, JET_GRBIT grbit )
{
    FUCB    *pfucb  = reinterpret_cast<FUCB *>( vtid );
    ERR     err     = JET_errSuccess;

    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfucb->u.pfcb );

    if ( grbit != 0 )
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    else if ( !pfucb->u.pfcb->FTypeTable()
        || pfucbNil != pfucb->pfucbCurIndex )
    {
        //  currently only support filtered moves on primary indices
        //
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }

    Call( ErrRECSetCursorFilter( pfucb, rgFilters, cFilters ) );

HandleError:
    if ( err < JET_errSuccess )
    {
        RECRemoveCursorFilter( pfucb );
    }
    return err;
}

/*=================================================================
 *
ErrIsamMove

Description:
    Retrieves the first, last, (nth) next, or (nth) previous
    record from the specified file.

Parameters:

    PIB                 *ppib           PIB of user
    FUCB                *pfucb          FUCB for file
    LONG                crow            number of rows to move
    JET_GRBIT           grbit           options

Return Value: standard error return

Errors/Warnings:
<List of any errors or warnings, with any specific circumstantial
 comments supplied on an as-needed-only basis>

Side Effects:
=================================================================*/

ERR VTAPI ErrIsamMove( JET_SESID sesid, JET_VTID vtid, LONG crow, JET_GRBIT grbit )
{
    PIB     *ppib = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );
    ERR     err = JET_errSuccess;
    FUCB    *pfucbSecondary;            // FUCB for secondary index (if any)
    FUCB    *pfucbIdx;              // FUCB of selected index (pri or sec)
    DIB     dib;                    // Information block for DirMan

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    if ( pfucb->pfucbCurIndex == NULL && pfucb->pmoveFilterContext && crow == 0 )
    {
        return ErrERRCheck( JET_errFilteredMoveNotSupported );
    }

    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

    AssertDIRNoLatch( ppib );
    dib.dirflag = fDIRNull;

    // Get secondary index FUCB if any
    pfucbSecondary = pfucb->pfucbCurIndex;
    if ( pfucbSecondary == pfucbNil )
        pfucbIdx = pfucb;
    else
        pfucbIdx = pfucbSecondary;

    if ( crow == JET_MoveLast )
    {
        DIRResetIndexRange( pfucb );

        dib.pos = posLast;

        //  move to DATA root
        //
        DIRGotoRoot( pfucbIdx );

        // Only enable preread when the filter context specified.
        if( pfucbIdx->pmoveFilterContext )
        {
            FUCBSetSequential( pfucbIdx );
            CPG cpgPreread = max( 1, (ULONG)UlParam( JET_paramMaxCoalesceReadSize ) / g_rgfmp[ pfucbIdx->ifmp ].CbPage() );
            FUCBSetPrereadBackward( pfucbIdx, cpgPreread );
        }

        err = ErrDIRDown( pfucbIdx, &dib );
    }
    else if ( crow > 0 )
    {
        LONG crowT = crow;

        if ( grbit & JET_bitMoveKeyNE )
            dib.dirflag |= fDIRNeighborKey;

        //  Move forward number of rows given
        //
        while ( crowT-- > 0 )
        {
            err = ErrDIRNext( pfucbIdx, dib.dirflag );
            if ( err < 0 )
            {
                Assert( !Pcsr( pfucbIdx )->FLatched() );
                break;
            }

            Assert( Pcsr( pfucbIdx )->FLatched() );

            if ( ( grbit & JET_bitMoveKeyNE ) && crowT > 0 )
            {
                // Need to do neighbour-key checking, so bookmark
                // must always be up-to-date.
                Call( ErrDIRRelease( pfucbIdx ) );
                Call( ErrDIRGet( pfucbIdx ) );
                Assert( Pcsr( pfucbIdx )->FLatched() );
            }
        }
    }
    else if ( crow == JET_MoveFirst )
    {
        DIRResetIndexRange( pfucb );

        dib.pos         = posFirst;

        //  move to DATA root
        //
        DIRGotoRoot( pfucbIdx );

        // Only enable preread when the filter context specified.
        if( pfucbIdx->pmoveFilterContext )
        {
            FUCBSetSequential( pfucbIdx );
            CPG cpgPreread = max( 1, (ULONG)UlParam( JET_paramMaxCoalesceReadSize ) / g_rgfmp[ pfucbIdx->ifmp ].CbPage() );
            FUCBSetPrereadForward( pfucbIdx, cpgPreread );
        }

        err = ErrDIRDown( pfucbIdx, &dib );
    }
    else if ( crow == 0 )
    {
        err = ErrDIRGet( pfucbIdx );
    }
    else
    {
        LONG crowT = crow;

        if ( grbit & JET_bitMoveKeyNE )
            dib.dirflag |= fDIRNeighborKey;

        while ( crowT++ < 0 )
        {
            err = ErrDIRPrev( pfucbIdx, dib.dirflag );
            if ( err < 0 )
            {
                AssertDIRNoLatch( ppib );
                break;
            }

            Assert( Pcsr( pfucbIdx )->FLatched() );
            if ( ( grbit & JET_bitMoveKeyNE ) && crowT < 0 )
            {
                // Need to do neighbour-key checking, so bookmark
                // must always be up-to-date.
                Call( ErrDIRRelease( pfucbIdx ) );
                Call( ErrDIRGet( pfucbIdx ) );
                Assert( Pcsr( pfucbIdx )->FLatched() );
            }
        }
    }

    //  if the movement was successful and a secondary index is
    //  in use, then position primary index to record.
    //
    if ( err == JET_errSuccess && pfucbSecondary != pfucbNil )
    {
        BOOKMARK    bmRecord;

        Assert( pfucbSecondary->kdfCurr.data.Pv() != NULL );
        Assert( pfucbSecondary->kdfCurr.data.Cb() > 0 );
        Assert( Pcsr( pfucbIdx )->FLatched() );

        bmRecord.key.prefix.Nullify();
        bmRecord.key.suffix = pfucbSecondary->kdfCurr.data;
        bmRecord.data.Nullify();

        //  We will need to touch the data page buffer.

        CallJ( ErrDIRGotoBookmark( pfucb, bmRecord ), ReleaseLatch );

        Assert( !Pcsr( pfucb )->FLatched() );
        Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );
        Assert( pfucb->u.pfcb->FPrimaryIndex() );
    }

    if ( JET_errSuccess == err )
    {
ReleaseLatch:
        ERR     errT;

        Assert( Pcsr( pfucbIdx )->FLatched() );

        errT = ErrDIRRelease( pfucbIdx );
        AssertDIRNoLatch( ppib );

        if ( JET_errSuccess == err && JET_errSuccess == errT )
        {
            return err;
        }

        if ( err >= 0 && errT < 0 )
        {
            //  return the more severe error
            //
            err = errT;
        }
    }

HandleError:
    AssertDIRNoLatch( ppib );

    if ( crow > 0 )
    {
        DIRAfterLast( pfucbIdx );
        DIRAfterLast(pfucb);
    }
    else if ( crow < 0 )
    {
        DIRBeforeFirst(pfucbIdx);
        DIRBeforeFirst(pfucb);
    }

    switch ( err )
    {
        case JET_errRecordNotFound:
            err = ErrERRCheck( JET_errNoCurrentRecord );
        case JET_errNoCurrentRecord:
        case JET_errRecordDeleted:
            break;
        default:
            Assert( JET_errSuccess != err );
            DIRBeforeFirst( pfucbIdx );
            if ( pfucbSecondary != pfucbNil )
                DIRBeforeFirst( pfucbSecondary );
    }

    OSTraceFMP(
        pfucbIdx->ifmp,
        JET_tracetagCursorNavigation,
        OSFormat(
            "Session=[0x%p:0x%x] has currency %d on node=[0x%x:0x%x:0x%x] of objid=[0x%x:0x%x] after Move [crow=0x%x,grbit=0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            pfucbIdx->locLogical,
            (ULONG)pfucbIdx->ifmp,
            Pcsr( pfucbIdx )->Pgno(),
            Pcsr( pfucbIdx )->ILine(),
            (ULONG)pfucbIdx->ifmp,
            pfucbIdx->u.pfcb->ObjidFDP(),
            crow,
            grbit,
            err,
            err ) );

    AssertDIRNoLatch( ppib );
    return err;
}


ERR ErrRECIFilteredMove( FUCB *pfucbTable, LONG crow, JET_GRBIT grbit )
{
    PIB     *ppib = pfucbTable->ppib;
    ERR     err = JET_errSuccess;
    FUCB    *pfucbIdx = pfucbTable->pfucbCurIndex ?
                            pfucbTable->pfucbCurIndex :
                            pfucbTable;

    DIRFLAG dirflag = fDIRNull;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucbTable );
    CheckSecondary( pfucbTable );

    Assert( Pcsr( pfucbIdx )->FLatched() );
    Assert( !FFUCBUpdatePrepared( pfucbTable ) );

    if ( crow > 0 )
    {
        LONG crowT = crow;

        if ( grbit & JET_bitMoveKeyNE )
            dirflag |= fDIRNeighborKey;

        //  Move forward number of rows given
        //
        while ( crowT-- > 0 )
        {
            if ( grbit & JET_bitMoveKeyNE )
            {
                // Need to do neighbour-key checking, so bookmark
                // must always be up-to-date.
                //
                // WARNING: if at level 0, the ErrDIRGet() might
                // end up returning JET_errRecordDeleted if the
                // node gets expunged when the page latch is
                // released
                //
                Call( ErrDIRRelease( pfucbIdx ) );
                Call( ErrDIRGet( pfucbIdx ) );
            }

            err = ErrDIRNext( pfucbIdx, dirflag );
            if ( err < 0 )
            {
                AssertDIRNoLatch( ppib );
                break;
            }

            Assert( Pcsr( pfucbIdx )->FLatched() );
        }
    }
    else
    {
        Assert( crow < 0 );
        LONG crowT = crow;

        if ( grbit & JET_bitMoveKeyNE )
            dirflag |= fDIRNeighborKey;

        while ( crowT++ < 0 )
        {
            if ( grbit & JET_bitMoveKeyNE )
            {
                // Need to do neighbour-key checking, so bookmark
                // must always be up-to-date.
                Call( ErrDIRRelease( pfucbIdx ) );
                Call( ErrDIRGet( pfucbIdx ) );
            }

            err = ErrDIRPrev( pfucbIdx, dirflag );
            if ( err < 0 )
            {
                AssertDIRNoLatch( ppib );
                break;
            }

            Assert( Pcsr( pfucbIdx )->FLatched() );
        }
    }

    //  if the movement was successful and a secondary index is
    //  in use, then position primary index to record.
    //
    if ( JET_errSuccess == err )
    {
        if ( pfucbIdx != pfucbTable )
        {
            BOOKMARK    bmRecord;

            Assert( pfucbIdx->kdfCurr.data.Pv() != NULL );
            Assert( pfucbIdx->kdfCurr.data.Cb() > 0 );
            Assert( pfucbIdx->locLogical == locOnCurBM );
            Assert( Pcsr( pfucbIdx )->FLatched() );

            bmRecord.key.prefix.Nullify();
            bmRecord.key.suffix = pfucbIdx->kdfCurr.data;
            bmRecord.data.Nullify();

            //  We will need to touch the data page buffer.

            Call( ErrDIRGotoBookmark( pfucbTable, bmRecord ) );

            Assert( !Pcsr( pfucbTable )->FLatched() );
            Assert( PgnoFDP( pfucbTable ) != pgnoSystemRoot );
            Assert( pfucbTable->u.pfcb->FPrimaryIndex() );
        }

        Assert( Pcsr( pfucbIdx )->FLatched() );
    }

HandleError:
    if ( err < 0 )
    {
        AssertDIRNoLatch( ppib );

        if ( crow > 0 )
        {
            DIRAfterLast( pfucbIdx );
            DIRAfterLast( pfucbTable );
        }
        else if ( crow < 0 )
        {
            DIRBeforeFirst( pfucbIdx );
            DIRBeforeFirst( pfucbTable );
        }

        Assert( err != JET_errRecordNotFound );
        switch ( err )
        {
            case JET_errNoCurrentRecord:
            case JET_errRecordDeleted:
                break;
            default:
                Assert( JET_errSuccess != err );
                DIRBeforeFirst( pfucbIdx );
                DIRBeforeFirst( pfucbTable );
        }
    }

    return err;
}

//  =================================================================
//  ErrIsamSeek

//  Description:
//  Retrieve the record specified by the given key or the
//  one just after it (SeekGT or SeekGE) or the one just
//  before it (SeekLT or SeekLE).

//  Parameters:

//  PIB         *ppib           PIB of user
//  FUCB        *pfucb          FUCB for file
//  JET_GRBIT   grbit           grbit

//  Return Value: standard error return

//  Errors/Warnings:
//  <List of any errors or warnings, with any specific circumstantial
//  comments supplied on an as-needed-only basis>

//  Side Effects:
//  =================================================================

ERR VTAPI ErrIsamSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
{
    PIB         *ppib           = reinterpret_cast<PIB *>( sesid );
    FUCB        *pfucbTable     = reinterpret_cast<FUCB *>( vtid );

    ERR         err;
    BOOKMARK    bm;                     //  for search key
    DIB         dib;
    FUCB        *pfucbSeek;             //  pointer to current FUCB
    BOOL        fFoundLess;
    BOOL        fFoundGreater;
    BOOL        fFoundEqual;
    BOOL        fRelease;
    FCB         *pfcb = pfcbNil;
    IDB         *pidb = pidbNil;

#define bitSeekAll (JET_bitSeekEQ | JET_bitSeekGE | JET_bitSeekGT | JET_bitSeekLE | JET_bitSeekLT)

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucbTable );
    CheckSecondary( pfucbTable );
    AssertDIRNoLatch( ppib );

    if( 0 == ( grbit & bitSeekAll ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    //  find cursor to seek on
    //
    pfucbSeek = pfucbTable->pfucbCurIndex == pfucbNil ?
                    pfucbTable :
                    pfucbTable->pfucbCurIndex;
    pfcb = pfucbSeek->u.pfcb;
    pidb = pfcb->Pidb();

    if ( !FKSPrepared( pfucbSeek ) )
    {
        return ErrERRCheck( JET_errKeyNotMade );
    }
    FUCBAssertValidSearchKey( pfucbSeek );

    //  Reset copy buffer status
    //
    if ( FFUCBUpdatePrepared( pfucbTable ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucbTable, JET_prepCancel ) );
    }

    //  reset index range limit
    //
    DIRResetIndexRange( pfucbTable );

    //  ignore segment counter
    //
    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( pfucbSeek->dataSearchKey.Pv() );
    bm.key.suffix.SetCb( pfucbSeek->dataSearchKey.Cb() );
    bm.data.Nullify();

    dib.pos = posDown;
    dib.pbm = &bm;

    if ( grbit & (JET_bitSeekLT|JET_bitSeekLE) )
    {
        dib.dirflag = fDIRFavourPrev;
    }
    else if ( grbit & JET_bitSeekGE )
    {
        dib.dirflag = fDIRFavourNext;
    }
    else if ( grbit & JET_bitSeekGT )
    {
        if ( !FFUCBUnique( pfucbSeek )
            && bm.key.suffix.Cb() < KEY::CbLimitKeyMost( pidb->CbKeyMost() ) )      //  may be equal if Limit already set or client used JET_bitNormalizedKey
        {
            Assert( pfcb->FTypeSecondaryIndex() );
            Assert( pidb->Cidxseg() > 0 );
            Assert( pfucbSeek->cColumnsInSearchKey <= pidb->Cidxseg() );
            if ( pfucbSeek->cColumnsInSearchKey == pidb->Cidxseg() )
            {
                //  PERF: seek on Limit of key, otherwise we would
                //  end up on the first index entry for this key
                //  (because of the trailing bookmark) and we
                //  would have to laterally navigate past it
                //  (and possibly others)
                //
                ( (BYTE *)bm.key.suffix.Pv() )[bm.key.suffix.Cb()] = 0xff;
                bm.key.suffix.DeltaCb( 1 );
            }
        }
        dib.dirflag = fDIRFavourNext;
    }
    else if ( grbit & JET_bitSeekEQ )
    {
        dib.dirflag = fDIRExact;
#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
        if ( grbit & JET_bitCheckUniqueness )
        {
            dib.dirflag |= fDIRCheckUniqueness;
        }
#endif
    }
    else
    {
        dib.dirflag = fDIRNull;
    }

#ifdef TRANSACTED_SEEK
    //
    //  UNDONE: support not yet fully added (still need
    //  transaction commit/rollback on all exit paths), so
    //  I've purposely not declared the fTransactionStarted
    //  variable so that this will fail to compile if
    //  someone tries to enable it
    //
    if ( 0 == ppib->Level() )
    {
        //  begin transaction for read consistency
        //
        Call( ErrDIRBeginTransaction( ppib, 60197, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }
#endif

    // Only enable preread when the filter context specified.
    if( pfucbSeek->pmoveFilterContext )
    {
        FUCBSetSequential( pfucbSeek );

        CPG cpgPreread = max( 1, (ULONG)UlParam( JET_paramMaxCoalesceReadSize ) / g_rgfmp[ pfucbSeek->ifmp ].CbPage() );

        if ( dib.dirflag & fDIRFavourNext )
        {
            FUCBSetPrereadForward( pfucbSeek, cpgPreread );
        }
        else if ( dib.dirflag & fDIRFavourPrev )
        {
            FUCBSetPrereadBackward( pfucbSeek, cpgPreread );
        }
    }

    err = ErrDIRDown( pfucbSeek, &dib );

    //  remember return from seek
    //
    fFoundLess = ( wrnNDFoundLess == err );
    fFoundGreater = ( wrnNDFoundGreater == err );
    fFoundEqual = ( !fFoundGreater
                    && !fFoundLess
                    && err >= 0 );
    fRelease = ( err >= 0 );

    Assert( !fRelease || Pcsr( pfucbSeek )->FLatched() );
    Assert( err < 0 || fFoundEqual || fFoundGreater || fFoundLess );

    if ( fFoundEqual )
    {
#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
        const BOOL  fFoundUniqueKey     = ( ( grbit & JET_bitCheckUniqueness )
                                            && ( JET_wrnUniqueKey == err || FFUCBUnique( pfucbSeek ) ) );
#else
        const BOOL  fFoundUniqueKey     = fFalse;
#endif

        Assert( Pcsr( pfucbSeek )->FLatched() );
        Assert( locOnCurBM == pfucbSeek->locLogical );
        if ( pfucbTable->pfucbCurIndex != pfucbNil )
        {
            //  if a secondary index is in use,
            //  then position primary index on record
            //
            Assert( FFUCBSecondary( pfucbSeek ) );
            Assert( pfucbSeek == pfucbTable->pfucbCurIndex );

            //  goto bookmark pointed to by secondary index node
            //
            BOOKMARK    bmRecord;

            Assert(pfucbSeek->kdfCurr.data.Pv() != NULL);
            Assert(pfucbSeek->kdfCurr.data.Cb() > 0 );

            bmRecord.key.prefix.Nullify();
            bmRecord.key.suffix = pfucbSeek->kdfCurr.data;
            bmRecord.data.Nullify();

            //  We will need to touch the data page buffer.

            Call( ErrDIRGotoBookmark( pfucbTable, bmRecord ) );

            Assert( PgnoFDP( pfucbTable ) != pgnoSystemRoot );
            Assert( pfucbTable->u.pfcb->FPrimaryIndex() );
        }

        switch ( grbit & bitSeekAll )
        {
            case JET_bitSeekEQ:
                Assert( fRelease );
                Assert( Pcsr( pfucbSeek )->FLatched() );
                Call( ErrDIRRelease( pfucbSeek ) );
                fRelease = fFalse;

                //  found equal on seek equal.  If index range grbit is
                //  set and we know the current key is not unique
                //  then set index range upper inclusive.
                //
                if ( fFoundUniqueKey )
                {
                    err = ErrERRCheck( JET_wrnUniqueKey );
                }
                else if ( grbit & JET_bitSetIndexRange )
                {
#ifdef TRANSACTED_SEEK
#else
                    //
                    //  WARNING: I've always told people that JetSeek()
                    //  will NEVER return JET_errNoCurrentRecord. But if
                    //  you get to this code path at level 0, and the
                    //  node is deleted out from underneath you, then
                    //  ErrIsamSetIndexRange() will end up failing with
                    //  precisely that error. Doh!
                    //
#endif
                    CallR( ErrIsamSetIndexRange( ppib, pfucbTable, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
                }

                goto Release;
                break;

            case JET_bitSeekGE:
            case JET_bitSeekLE:
                //  release and return
                //
                CallS( err );
                goto Release;
                break;

            case JET_bitSeekGT:
                //  move to next node with different key
                //  (WARNING: if at level 0, there is potential
                //  for this to fail with JET_errRecordDeleted
                //  because the page gets unlatched then
                //  re-latched)
                //
                err = ErrRECIFilteredMove( pfucbTable, JET_MoveNext, JET_bitMoveKeyNE );

                if ( err < 0 )
                {
                    AssertDIRNoLatch( ppib );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        KSReset( pfucbSeek );
                        return ErrERRCheck( JET_errRecordNotFound );
                    }

                    goto HandleError;
                }

                CallS( err );
                goto Release;
                break;

            case JET_bitSeekLT:
                //  move to previous node with different key
                //  (WARNING: if at level 0, there is potential
                //  for this to fail with JET_errRecordDeleted
                //  because the page gets unlatched then
                //  re-latched)
                //
                err = ErrRECIFilteredMove( pfucbTable, JET_MovePrevious, JET_bitMoveKeyNE );

                if ( err < 0 )
                {
                    AssertDIRNoLatch( ppib );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        KSReset( pfucbSeek );
                        return ErrERRCheck( JET_errRecordNotFound );
                    }

                    goto HandleError;
                }

                CallS( err );
                goto Release;
                break;

            default:
                Assert( fFalse );
                return err;
        }
    }
    else if ( fFoundLess )
    {
        Assert( Pcsr( pfucbSeek )->FLatched() );
        Assert( locBeforeSeekBM == pfucbSeek->locLogical );

        switch ( grbit & bitSeekAll )
        {
            case JET_bitSeekEQ:
                if ( FFUCBUnique( pfucbSeek ) )
                {
                    //  see RecordNotFound case below for an
                    //  explanation of why we need to set
                    //  the locLogical of the primary cursor
                    //  (note: the secondary cursor will
                    //  narmally get set to locLogical when
                    //  we call ErrDIRRelease() below, but
                    //  may get set to locOnFDPRoot if we
                    //  couldn't save the bookmark)
                    if ( pfucbTable != pfucbSeek )
                    {
                        Assert( !Pcsr( pfucbTable )->FLatched() );
                        pfucbTable->locLogical = locOnSeekBM;
                    }
                    err = ErrERRCheck( JET_errRecordNotFound );
                    goto Release;
                }

                //  For non-unique indexes, because we use
                //  key+data for keys of internal nodes,
                //  and because child nodes have a key
                //  strictly less than its parent, we
                //  might end up on the wrong leaf node.
                //  We want to go to the right sibling and
                //  check the first node there to see if
                //  the key-only matches.
                Assert( pfucbSeek->u.pfcb->FTypeSecondaryIndex() ); //  only secondary index can be non-unique

                //  FALL THROUGH

            case JET_bitSeekGE:
            case JET_bitSeekGT:
                //  move to next node
                //  release and return
AdjustPositionAfterFoundLess:
                err = ErrRECIFilteredMove( pfucbTable, JET_MoveNext, NO_GRBIT );

                if ( err < 0 )
                {
                    AssertDIRNoLatch( ppib );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        KSReset( pfucbSeek );
                        return ErrERRCheck( JET_errRecordNotFound );
                    }

                    goto HandleError;
                }

                CallS( err );

                if ( !FFUCBUnique( pfucbSeek ) )
                {
                    //  For a non-unique index, there are some complexities
                    //  because the keys are stored as key+data but we're
                    //  doing a key-only search (see comment in the
                    //  JET_bitSeekEQ just above for the full explanation).
                    //  This might cause us to fall short of the node we
                    //  truly want, so we have to laterally navigate.
                    Assert( pfucbSeek == pfucbTable->pfucbCurIndex );
                    Assert( Pcsr( pfucbSeek )->FLatched() );
                    const INT   cmp = CmpKey( pfucbSeek->kdfCurr.key, bm.key );

#ifdef TRANSACTED_SEEK
                    Assert( cmp >= 0 );
#else
                    //  if we're at level 0, the MoveNext above
                    //  may have landed on another node less than
                    //  our search criteria (because it just got
                    //  inserted and/or became visible), so need
                    //  to skip over it
                    //
                    Assert( cmp >= 0 || 0 == ppib->Level() );
                    if ( cmp < 0 && 0 == ppib->Level() )
                    {
                        goto AdjustPositionAfterFoundLess;
                    }
#endif

                    if ( grbit & JET_bitSeekGE )
                    {
                        if ( 0 != cmp )
                            err = ErrERRCheck( JET_wrnSeekNotEqual );
                    }
                    else if ( grbit & JET_bitSeekGT )
                    {
                        //  the keys match exactly, but we're doing
                        //  a strictly greater than search, so must
                        //  keep navigating
                        if ( 0 == cmp )
                            goto AdjustPositionAfterFoundLess;
                    }
                    else
                    {
                        Assert( grbit & JET_bitSeekEQ );

                        if ( 0 != cmp )
                        {
                            err = ErrERRCheck( JET_errRecordNotFound );
                        }
                        else if ( grbit & ( JET_bitSetIndexRange|JET_bitCheckUniqueness ) )
                        {
                            Assert( fRelease );
                            Assert( Pcsr( pfucbSeek )->FLatched() );

#ifdef CHECK_UNIQUE_KEY_ON_NONUNIQUE_INDEX
                            BOOL        fFoundUniqueKey     = fFalse;

                            if ( grbit & JET_bitCheckUniqueness )
                            {
                                const CSR * const   pcsr    = Pcsr( pfucbSeek );
                                if ( pcsr->ILine() < pcsr->Cpage().Clines() - 1 )
                                {
                                    const INT       cmpT    = CmpNDKeyOfNextNode( pcsr, bm.key );
                                    Assert( cmpT >= 0 );
                                    fFoundUniqueKey = ( cmpT > 0 );
                                }
                            }
#else
                            const BOOL  fFoundUniqueKey     = fFalse;
#endif

                            Call( ErrDIRRelease( pfucbSeek ) );
                            fRelease = fFalse;

                            if ( fFoundUniqueKey )
                            {
                                err = ErrERRCheck( JET_wrnUniqueKey );
                            }
                            else if ( grbit & JET_bitSetIndexRange )
                            {
#ifdef TRANSACTED_SEEK
#else
                                //
                                //  WARNING: I've always told people that JetSeek()
                                //  will NEVER return JET_errNoCurrentRecord. But if
                                //  you get to this code path at level 0, and the
                                //  node is deleted out from underneath you, then
                                //  ErrIsamSetIndexRange() will end up failing with
                                //  precisely that error. Doh!
                                //
#endif
                                CallR( ErrIsamSetIndexRange( ppib, pfucbTable, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
                            }
                        }
                    }
                }

#ifdef TRANSACTED_SEEK
#else
                else if ( 0 == ppib->Level() )
                {
                    //  since we're at level 0, the MoveNext above
                    //  may have landed on another node less than
                    //  our search criteria (because it just got
                    //  inserted and/or became visible), so need
                    //  to skip over it
                    //
                    CallS( err );
                    Assert( grbit & ( JET_bitSeekGE | JET_bitSeekGT ) );
                    Assert( Pcsr( pfucbSeek )->FLatched() );
                    const INT   cmp = CmpKey( pfucbSeek->kdfCurr.key, bm.key );

                    if ( cmp < 0 )
                    {
                        goto AdjustPositionAfterFoundLess;
                    }
                    else if ( cmp > 0 )
                    {
                        //  node has a key creater than what we're looking
                        //  for, so return appropriate error if necessary
                        //
                        if ( grbit & JET_bitSeekGE )
                            err = ErrERRCheck( JET_wrnSeekNotEqual );
                    }
                    else
                    {
                        //  node has key equal to the key we used for
                        //  seeking, but if we were performing a SeekGT,
                        //  then we should skip the node
                        //
                        if ( grbit & JET_bitSeekGT )
                            goto AdjustPositionAfterFoundLess;
                    }
                }
#endif

                else if ( grbit & JET_bitSeekGE )
                {
                    Assert( Pcsr( pfucbSeek )->FLatched() );
                    Assert( CmpKey( pfucbSeek->kdfCurr.key, bm.key ) > 0 );
                    err = ErrERRCheck( JET_wrnSeekNotEqual );
                }

                else
                {
                    Assert( grbit & JET_bitSeekGT );
                    Assert( Pcsr( pfucbSeek )->FLatched() );
                    Assert( CmpKey( pfucbSeek->kdfCurr.key, bm.key ) > 0 );
                    CallS( err );
                }

                goto Release;
                break;

            case JET_bitSeekLE:
            case JET_bitSeekLT:
                //  move to previous node -- to adjust DIR level locLogical
                //  (because coming out of ErrDIRDown(), the locLogical
                //  will be locBeforeSeekBM)
                //
                err = ErrRECIFilteredMove( pfucbTable, JET_MovePrevious, NO_GRBIT );

                if ( err < 0 )
                {
                    AssertDIRNoLatch( ppib );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        KSReset( pfucbSeek );
                        return ErrERRCheck( JET_errRecordNotFound );
                    }

                    goto HandleError;
                }

                if ( grbit & JET_bitSeekLE )
                {
                    err = ErrERRCheck( JET_wrnSeekNotEqual );
                }
                goto Release;
                break;

            default:
                Assert( fFalse );
                return err;
        }
    }
    else if ( fFoundGreater )
    {
        Assert( Pcsr( pfucbSeek )->FLatched() );
        Assert( locAfterSeekBM == pfucbSeek->locLogical );

        switch ( grbit & bitSeekAll )
        {
            case JET_bitSeekEQ:
                //  see RecordNotFound case below for an
                //  explanation of why we need to set
                //  the locLogical of the primary cursor
                //  (note: the secondary cursor will
                //  narmally get set to locLogical when
                //  we call ErrDIRRelease() below, but
                //  may get set to locOnFDPRoot if we
                //  couldn't save the bookmark)
                if ( pfucbTable != pfucbSeek )
                {
                    Assert( !Pcsr( pfucbTable )->FLatched() );
                    pfucbTable->locLogical = locOnSeekBM;
                }
                err = ErrERRCheck( JET_errRecordNotFound );
                goto Release;
                break;

            case JET_bitSeekGE:
            case JET_bitSeekGT:
                //  move next to fix DIR level locLogical
                //  (because coming out of ErrDIRDown(), the locLogical
                //  will be locAfterSeekBM)
                //
                err = ErrRECIFilteredMove( pfucbTable, JET_MoveNext, NO_GRBIT );

                Assert( err >= 0 );
                if ( err < 0 )
                {
                    AssertDIRNoLatch( ppib );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        KSReset( pfucbSeek );
                        return ErrERRCheck( JET_errRecordNotFound );
                    }

                    goto HandleError;
                }

                if ( grbit & JET_bitSeekGE )
                {
                    err = ErrERRCheck( JET_wrnSeekNotEqual );
                }
                goto Release;
                break;

            case JET_bitSeekLE:
            case JET_bitSeekLT:
                //  move previous
                //  release and return
                //
#ifdef TRANSACTED_SEEK
                err = ErrRECIFilteredMove( pfucbTable, JET_MovePrevious, NO_GRBIT );

                if ( err < 0 )
                {
                    AssertDIRNoLatch( ppib );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        KSReset( pfucbSeek );
                        return ErrERRCheck( JET_errRecordNotFound );
                    }

                    goto HandleError;
                }

                CallS( err );
                Assert( Pcsr( pfucbSeek )->FLatched() );
                Assert( CmpKey( pfucbSeek->kdfCurr.key, bm.key ) < 0 );
                if ( grbit & JET_bitSeekLE )
                {
                    err = ErrERRCheck( JET_wrnSeekNotEqual );
                }
#else
                forever
                {
                    err = ErrRECIFilteredMove( pfucbTable, JET_MovePrevious, NO_GRBIT );
                    
                    if ( err < 0 )
                    {
                        AssertDIRNoLatch( ppib );
                        if ( JET_errNoCurrentRecord == err )
                        {
                            KSReset( pfucbSeek );
                            return ErrERRCheck( JET_errRecordNotFound );
                        }
                    
                        goto HandleError;
                    }

                    CallS( err );
                    Assert( Pcsr( pfucbSeek )->FLatched() );

                    if ( 0 == ppib->Level() )
                    {
                        //  since we're at level 0, the MovePrev above
                        //  may have landed on another node greater than
                        //  our search criteria (because it just got
                        //  inserted and/or became visible), so need to
                        //  skip over it
                        //
                        const INT   cmp = CmpKey( pfucbSeek->kdfCurr.key, bm.key );

                        if ( cmp < 0 )
                        {
                            if ( grbit & JET_bitSeekLE )
                                err = ErrERRCheck( JET_wrnSeekNotEqual );
                            break;
                        }
                        else if ( 0 == cmp && ( grbit & JET_bitSeekLE ) )
                        {
                            break;
                        }
                        else
                        {
                            //  skip over this node
                        }
                    }
                    else
                    {
                        Assert( CmpKey( pfucbSeek->kdfCurr.key, bm.key ) < 0 );
                        if ( grbit & JET_bitSeekLE )
                            err = ErrERRCheck( JET_wrnSeekNotEqual );
                        break;
                    }
                }
#endif  //  TRANSACTED_SEEK

                goto Release;
                break;

            default:
                Assert( fFalse );
                return err;
        }
    }
    else
    {
        Assert( err < 0 );
        Assert( JET_errNoCurrentRecord != err );
        Assert( !Pcsr( pfucbSeek )->FLatched() );

        if ( JET_errRecordNotFound == err )
        {
            //  The secondary index cursor has been placed on a
            //  virtual record, so we must update the primary
            //  index cursor as well (if not, then it's possible
            //  to do, for instance, a RetrieveColumn on the
            //  primary cursor and you'll get back data from the
            //  record you were on before the seek but a
            //  RetrieveFromIndex on the secondary cursor will
            //  return JET_errNoCurrentRecord).
            //  Note that although the locLogical of the primary
            //  cursor is being updated, it's not necessary to
            //  update the primary cursor's bmCurr, because it
            //  will never be accessed (the secondary cursor takes
            //  precedence).  The only reason we reset the
            //  locLogical is for error-handling so that we
            //  properly err out with JET_errNoCurrentRecord if
            //  someone tries to use the cursor to access a record
            //  before repositioning the secondary cursor to a true
            //  record.
            Assert( locOnSeekBM == pfucbSeek->locLogical );
            if ( pfucbTable != pfucbSeek )
            {
                Assert( !Pcsr( pfucbTable )->FLatched() );
                pfucbTable->locLogical = locOnSeekBM;
            }
        }

        KSReset( pfucbSeek );
        AssertDIRNoLatch( ppib );
        return err;
    }

Release:
    //  release latched page and return error
    //
    if ( fRelease )
    {
        Assert( Pcsr( pfucbSeek ) ->FLatched() );
        const ERR   errT    = ErrDIRRelease( pfucbSeek );
        if ( errT < 0 )
        {
            err = errT;
            goto HandleError;
        }
    }

    OSTraceFMP(
        pfucbSeek->ifmp,
        JET_tracetagCursorNavigation,
        OSFormat(
            "Session=[0x%p:0x%x] has currency %d on node=[0x%x:0x%x:0x%x] of objid=[0x%x:0x%x] after Seek [grbit=0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            pfucbSeek->locLogical,
            (ULONG)pfucbSeek->ifmp,
            Pcsr( pfucbSeek )->Pgno(),
            Pcsr( pfucbSeek )->ILine(),
            (ULONG)pfucbSeek->ifmp,
            pfucbSeek->u.pfcb->ObjidFDP(),
            grbit,
            err,
            err ) );

    KSReset( pfucbSeek );
    AssertDIRNoLatch( ppib );
    return err;

HandleError:
    //  reset cursor to before first
    //
    Assert( err < 0 );
    KSReset( pfucbSeek );
    DIRUp( pfucbSeek );
    DIRBeforeFirst( pfucbSeek );
    if ( pfucbTable != pfucbSeek )
    {
        Assert( !Pcsr( pfucbTable )->FLatched() );
        DIRBeforeFirst( pfucbTable );
    }
    AssertDIRNoLatch( ppib );
    return err;
}


//  =================================================================
LOCAL ERR ErrRECICheckIndexrangesForUniqueness(
            const JET_SESID sesid,
            const JET_INDEXRANGE * const rgindexrange,
            const ULONG cindexrange )
//  =================================================================
{
    PIB * const ppib        = reinterpret_cast<PIB *>( sesid );

    //  check that all the tableid's are on the same table and different indexes
    for( SIZE_T itableid = 0; itableid < cindexrange; ++itableid )
    {
        if( sizeof( JET_INDEXRANGE ) != rgindexrange[itableid].cbStruct )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        const FUCB * const pfucb    = reinterpret_cast<FUCB *>( rgindexrange[itableid].tableid );
        CheckTable( ppib, pfucb );
        CheckSecondary( pfucb );

        //  don't do a join on the primary index!

        if( !pfucb->pfucbCurIndex )
        {
            AssertSz( fFalse, "Don't do a join on the primary index!" );
            return ErrERRCheck( JET_errInvalidParameter );
        }

        //  check the GRBITs

        if( JET_bitRecordInIndex != rgindexrange[itableid].grbit
//  SOMEONE: 01/10/02: removed until ErrRECIJoinFindDuplicates is fixed
//  for JET_bitRecordNotInIndex
///         && JET_bitRecordNotInIndex != rgindexrange[itableid].grbit
            )
        {
            AssertSz( fFalse, "Invalid grbit in JET_INDEXRANGE" );
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        //  check against all other indexes for duplications

        for( SIZE_T itableidT = 0; itableidT < cindexrange; ++itableidT )
        {
            if( itableidT == itableid )
            {
                continue;
            }

            const FUCB * const pfucbT   = reinterpret_cast<FUCB *>( rgindexrange[itableidT].tableid );

            //  don't do a join on the primary index!

            if( !pfucbT->pfucbCurIndex )
            {
                AssertSz( fFalse, "Don't do a join on the primary index!" );
                return ErrERRCheck( JET_errInvalidParameter );
            }

            //  compare FCB's to make sure we are on the same table

            if( pfucbT->u.pfcb != pfucb->u.pfcb )
            {
                AssertSz( fFalse, "Indexes are not on the same table" );
                return ErrERRCheck( JET_errInvalidParameter );
            }

            //  compare secondary indexes to make sure the indexes are different

            if( pfucb->pfucbCurIndex->u.pfcb == pfucbT->pfucbCurIndex->u.pfcb )
            {
                AssertSz( fFalse, "Indexes are the same" );
                return ErrERRCheck( JET_errInvalidParameter );
            }
        }
    }
    return JET_errSuccess;
}


//  =================================================================
LOCAL ERR ErrRECIInsertBookmarksIntoSort(
    FUCB * const        pfucb,
    FUCB * const        pfucbSort,
    const JET_GRBIT     grbit )
//  =================================================================
{
    ERR                 err;
    const INST * const  pinst       = PinstFromPfucb( pfucb );
    INT                 cBookmarks  = 0;
    KEY                 key;
    DATA                data;

    key.prefix.Nullify();
    data.Nullify();

    Call( ErrDIRGet( pfucb ) );
    do
    {
        //  this loop can take some time, so see if we need
        //  to terminate because of shutdown
        //
        Call( pinst->ErrCheckForTermination() );

        key.suffix = pfucb->kdfCurr.data;
        Call( ErrSORTInsert( pfucbSort, key, data ) );
        ++cBookmarks;
    }
    while( ( err = ErrDIRNext( pfucb, fDIRNull ) ) == JET_errSuccess );

    if( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    DIRUp( pfucb );
    FUCBResetPreread( pfucb );

    return err;
}


//  =================================================================
LOCAL ERR ErrRECIJoinFindDuplicates(
    JET_SESID                       sesid,
    _In_count_( cindexes ) const JET_INDEXRANGE * const rgindexrange,
    _In_count_( cindexes ) FUCB *   rgpfucbSort[],
    _In_range_( 1, 64 ) const ULONG cindexes,
    _Out_ JET_RECORDLIST * const            precordlist,
    JET_GRBIT                       grbit )
//  =================================================================
{
    ERR                             err;
    const INST * const              pinst       = PinstFromPpib( (PIB *)sesid );
    SIZE_T                          isort;
    BYTE                            rgfSortIsMin[64];

    // prefast wants to know the array count or rgfSortIsMin is big enough for cindexes
    if ( _countof(rgfSortIsMin) < cindexes )
    {
        AssertSz( false, "We have a problem." );
        // There really should be a code inconsistent error ...
        return( ErrERRCheck(JET_wrnNyi) );
    }

    //  move all the sorts to the first record

    Assert( cindexes > 0 );
    //  pull page size off first sort.
    const LONG cbPage = CbAssertGlobalPageSizeMatchRTL( g_rgfmp[ rgpfucbSort[ 0 ]->ifmp ].CbPage() );
    Assert( cbPage != 0 );
    for( isort = 0; isort < cindexes; ++isort )
    {
        err = ErrSORTNext( rgpfucbSort[isort] );
        if( JET_errNoCurrentRecord == err )
        {
            //  no bookmarks to return
            err = JET_errSuccess;
            return err;
        }
        Call( err );
        Expected( cbPage == g_rgfmp[ rgpfucbSort[ isort ]->ifmp ].CbPage() ); // page size shouldn't change between indices.
    }

    //  FUTURE: need JET_coltypKey so that made key is not needlessly renormalized as binary
    //          which can result in the key being truncated thereby perturbing primary key order.
    //          rgcolumndefJoinList would then use JET_coltypKey in place of JET_coltypBinary.
    //          Indexes over columns of this type may require that this is either the only column
    //          or that the column occurs at the end of the index.
    //
    //  create the temp table for the bookmarks
    //
    Assert( 1 == sizeof( rgcolumndefJoinlist ) / sizeof( rgcolumndefJoinlist[0] ) );
    Call( ErrIsamOpenTempTable(
                sesid,
                rgcolumndefJoinlist,
                sizeof( rgcolumndefJoinlist ) / sizeof( rgcolumndefJoinlist[0] ),
                NULL,
                NO_GRBIT,
                &(precordlist->tableid),
                &(precordlist->columnidBookmark ),
                CbKeyMostForPage( cbPage ),
                CbKeyMostForPage( cbPage ) ) );

    //  take a unique list of bookmarks from the sorts

    while( 1 )
    {
        SIZE_T  isortMin    = 0;

        //  this loop can take some time, so see if we need
        //  to terminate because of shutdown
        //
        Call( pinst->ErrCheckForTermination() );

        //  find the index of the sort with the smallest bookmark

        for( isort = 1; isort < cindexes; ++isort )
        {
            const INT cmp = CmpKey( rgpfucbSort[isortMin]->kdfCurr.key, rgpfucbSort[isort]->kdfCurr.key );
            if( 0 == cmp )
            {
            }
            else if( cmp < 0 )
            {
                //  the current min is smaller
            }
            else
            {
                //  we have a new minimum
                Assert( cmp > 0 );
                isortMin = isort;
            }
        }

        //  see if all keys are the same as the minimum

        BOOL    fDuplicate  = fTrue;
        memset( rgfSortIsMin, 0, sizeof( rgfSortIsMin ) );

        // We errored out at the top of the function if cindexes was greater than
        // the length of rgfSortIsMin, and isortMin is bounded by cindexes in the loop above.
        AssertPREFIX( isortMin < _countof( rgfSortIsMin ) );

        rgfSortIsMin[isortMin] = fTrue;
        for( isort = 0; isort < cindexes; ++isort )
        {
            if( isort == isortMin )
            {
                continue;
            }
            const INT cmp = CmpKey( rgpfucbSort[isortMin]->kdfCurr.key, rgpfucbSort[isort]->kdfCurr.key );
            if( 0 == cmp )
            {
                if( JET_bitRecordInIndex == rgindexrange[isort].grbit )
                {
                }
                else if( JET_bitRecordNotInIndex == rgindexrange[isort].grbit )
                {
                    fDuplicate = fFalse;
                }
                else
                {
                    Assert( fFalse );
                }

                rgfSortIsMin[isort] = fTrue;
            }
            else
            {
                if( JET_bitRecordInIndex == rgindexrange[isort].grbit )
                {
                    fDuplicate = fFalse;
                }
                else if( JET_bitRecordNotInIndex == rgindexrange[isort].grbit )
                {
                }
                else
                {
                    Assert( fFalse );
                }

                Assert( !rgfSortIsMin[isort] );
            }
        }

        //  if there are duplicates, insert into the temp table

        if( fDuplicate )
        {
            Call( ErrDispPrepareUpdate( sesid, precordlist->tableid, JET_prepInsert ) );

            Assert( rgpfucbSort[isortMin]->kdfCurr.key.prefix.FNull() );
            Call( ErrDispSetColumn(
                        sesid,
                        precordlist->tableid,
                        precordlist->columnidBookmark,
                        rgpfucbSort[isortMin]->kdfCurr.key.suffix.Pv(),
                        rgpfucbSort[isortMin]->kdfCurr.key.suffix.Cb(),
                        NO_GRBIT,
                        NULL ) );

            Call( ErrDispUpdate( sesid, precordlist->tableid, NULL, 0, NULL, NO_GRBIT ) );

            ++(precordlist->cRecord);
        }

        //  remove all minimums

        for( isort = 0; isort < cindexes; ++isort )
        {
            if( rgfSortIsMin[isort] )
            {
                err = ErrSORTNext( rgpfucbSort[isort] );
                if( JET_errNoCurrentRecord == err )
                {
                    err = JET_errSuccess;
                    return err;
                }
                Call( err );
            }
        }
    }

HandleError:

    if( err < 0 && JET_tableidNil != precordlist->tableid )
    {
        CallS( ErrDispCloseTable( sesid, precordlist->tableid ) );
        precordlist->tableid = JET_tableidNil;
    }

    return err;
}


//  =================================================================
ERR ErrIsamIntersectIndexes(
    const JET_SESID sesid,
    _In_count_( cindexrange ) const JET_INDEXRANGE * const rgindexrange,
    _In_range_( 1, 64 ) const ULONG cindexrange,
    _Out_ JET_RECORDLIST * const precordlist,
    const JET_GRBIT grbit )
//  =================================================================
{
    PIB * const ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB *      rgpfucbSort[64];
    SIZE_T      ipfucb;
    ERR         err;

    //  check input parameters
    //
    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );

    if( NULL == precordlist
        || sizeof( JET_RECORDLIST ) != precordlist->cbStruct )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if( 0 == cindexrange )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if( cindexrange > sizeof(rgpfucbSort)/sizeof(rgpfucbSort[0]) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  check that all the tableid's are on the same table and different indexes
    //
    CallR( ErrRECICheckIndexrangesForUniqueness( sesid, rgindexrange, cindexrange ) );

    //  set all the sort's to NULL
    //
    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        Assert( ipfucb < sizeof(rgpfucbSort)/sizeof(rgpfucbSort[0]) );
        rgpfucbSort[ipfucb] = pfucbNil;
    }

    //  initialize the pjoinlist
    //
    precordlist->tableid    = JET_tableidNil;
    precordlist->cRecord    = 0;
    precordlist->columnidBookmark = 0;

    Call( ErrPIBOpenTempDatabase( ppib ) );
    
    //  create the sorts
    //
    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        Call( ErrSORTOpen( ppib, rgpfucbSort + ipfucb, fTrue, fTrue ) );
    }

    //  for each index, put all the primary keys in its index range into its sort
    //
    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        FUCB * const pfucb  = reinterpret_cast<FUCB *>( rgindexrange[ipfucb].tableid );
        Call( ErrRECIInsertBookmarksIntoSort( pfucb->pfucbCurIndex, rgpfucbSort[ipfucb], grbit ) );
        Call( ErrSORTEndInsert( rgpfucbSort[ipfucb] ) );
    }

    //  insert duplicate bookmarks into a new temp table
    //
    Call( ErrRECIJoinFindDuplicates(
            sesid,
            rgindexrange,
            rgpfucbSort,
            cindexrange,
            precordlist,
            grbit ) );

HandleError:
    //  close all the sorts
    //
    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        if( pfucbNil != rgpfucbSort[ipfucb] )
        {
            SORTClose( rgpfucbSort[ipfucb] );
            rgpfucbSort[ipfucb] = pfucbNil;
        }
    }

    return err;
}


//  =================================================================
LOCAL ERR ErrIsamValidatePrereadKeysArguments(
    const PIB * const                               ppib,
    const FUCB * const                              pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __in_opt const LONG * const                     pckeysPreread,
    const JET_GRBIT                                 grbit )
//  =================================================================
{
    ERR err = JET_errSuccess;

    if( NULL == ppib || NULL == pfucb || NULL == rgpvKeys || NULL == rgcbKeys )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    if( ckeys <= 0 )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if( JET_bitPrereadBackward != grbit && JET_bitPrereadForward != grbit )
    {
        Call( ErrERRCheck( JET_errInvalidGrbit ) );
    }

    // determine the largest possible key for this index
    const FCB * const pfcbTable = pfucb->u.pfcb;
    const IDB * const pidb = pfcbTable->Pidb();
    const ULONG cbKeyMost = pidb ? pidb->CbKeyMost() : sizeof(DBK);

    // validate the arguments
    for( INT ikey = 0; ikey < ckeys; ++ikey )
    {
        if( NULL == rgpvKeys[ikey] )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        if( 0 == rgcbKeys[ikey] || rgcbKeys[ikey] > cbKeyMost )
        {
            Call( ErrERRCheck( JET_errInvalidBufferSize ) );
        }

        // make sure the keys are in order (duplicates are OK)
        if( ikey > 0 )
        {
            KEY keyPrev;
            KEY keyCurr;

            keyPrev.prefix.Nullify();
            keyPrev.suffix.SetPv( const_cast<void *>( rgpvKeys[ikey-1] ) );
            keyPrev.suffix.SetCb( rgcbKeys[ikey-1] );

            keyCurr.prefix.Nullify();
            keyCurr.suffix.SetPv( const_cast<void *>( rgpvKeys[ikey] ) );
            keyCurr.suffix.SetCb( rgcbKeys[ikey] );

            const INT cmp = CmpKey( keyPrev, keyCurr );
            if( (JET_bitPrereadBackward & grbit) && cmp < 0 )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
            else if( (JET_bitPrereadForward & grbit) && cmp > 0 )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
    }
    
HandleError:
    return err;
}



//  =================================================================
ERR VTAPI ErrIsamPrereadKeys(
    const JET_SESID                                 sesid,
    const JET_VTID                                  vtid,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __out_opt LONG * const                          pckeysPreread,
    const JET_GRBIT                                 grbit )
//  =================================================================
{
    ERR err = JET_errSuccess;

    PIB * const ppib = reinterpret_cast<PIB *>( sesid );
    FUCB * pfucb = reinterpret_cast<FUCB *>( vtid );

    // switch to any secondary index that is in use
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        pfucb = pfucb->pfucbCurIndex;
    }

    if( pckeysPreread )
    {
        *pckeysPreread = 0;
    }

    Call( ErrIsamValidatePrereadKeysArguments(
            ppib,
            pfucb,
            rgpvKeys,
            rgcbKeys,
            ckeys,
            pckeysPreread,
            grbit ) );

    LONG ckeysPrereadT;
    Call( ErrBTPrereadKeys(
            ppib,
            pfucb,
            rgpvKeys,
            rgcbKeys,
            ckeys,
            &ckeysPrereadT,
            grbit ) );

    if( pckeysPreread )
    {
        *pckeysPreread = ckeysPrereadT;
    }

HandleError:
    return err;
}


LOCAL ERR ErrRECIMakeKey(
          PIB * const                   ppib,
          FUCB * const                  pfucb,
    const JET_INDEX_COLUMN * const      rgIndexColumns,
    const DWORD                         cIndexColumns,
    const BOOL                          fStartRange,
          BYTE ** const                 ppKey,
          DWORD * const                 pcKey )
{
    ERR err = JET_errSuccess;

    *ppKey = NULL;
    *pcKey = 0;

    for ( DWORD i = 0; i < cIndexColumns; i++ )
    {
        // Check for valid relational operators for creating a key
        // Can be Equals/ZeroLength or PrefixEquals (that one only on the
        // last column spcified) - all unspecified columns become wildcards
        // Begin/End range is auto-inferred from whether or not it is the
        // start or end of the range.
        if ( rgIndexColumns[i].relop != JET_relopEquals &&
             ( i != cIndexColumns - 1 || rgIndexColumns[i].relop != JET_relopPrefixEquals ) )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
        if ( ( ( rgIndexColumns[i].grbit & ~JET_bitZeroLength ) != 0 ) ||
             ( rgIndexColumns[i].grbit == JET_bitZeroLength && rgIndexColumns[i].cb != 0 ) )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        DWORD grbitMakeKey = 0;
        if ( i == 0 )
        {
            grbitMakeKey |= JET_bitNewKey;
        }
        if ( rgIndexColumns[i].grbit & JET_bitZeroLength )
        {
            grbitMakeKey |= JET_bitKeyDataZeroLength;
        }
        if ( i == cIndexColumns - 1 )
        {
            if ( rgIndexColumns[i].relop == JET_relopPrefixEquals )
            {
                grbitMakeKey |= ( fStartRange ? JET_bitPartialColumnStartLimit : JET_bitPartialColumnEndLimit );
            }
            else
            {
                grbitMakeKey |= ( fStartRange ? JET_bitFullColumnStartLimit : JET_bitFullColumnEndLimit );
            }
        }

        Call( ErrIsamMakeKey( ppib, pfucb, rgIndexColumns[i].pv, rgIndexColumns[i].cb, grbitMakeKey ) );
    }

    Call( ErrIsamRetrieveKey( ppib, pfucb, NULL, 0, pcKey, JET_bitRetrieveCopy ) );
    Alloc( *ppKey = new BYTE[*pcKey] );
    CallS( ErrIsamRetrieveKey( ppib, pfucb, *ppKey, *pcKey, pcKey, JET_bitRetrieveCopy ) );

HandleError:
    Expected( err >= JET_errSuccess || *ppKey == NULL );
    if ( err < JET_errSuccess )
    {
        delete *ppKey;
        *ppKey = NULL;
        *pcKey = 0;
    }
    return err;
}

LOCAL ERR ErrRECIInsertionSort(
    __inout_ecount(cIndexRanges) BYTE **pStartKeys,
    __inout_ecount(cIndexRanges) DWORD *startKeyLengths,
    __inout_ecount(cIndexRanges) BYTE **pEndKeys,
    __inout_ecount(cIndexRanges) DWORD *endKeyLengths,
    DWORD cIndexRanges,
    BOOL fForward )
{
    ERR err = JET_errSuccess;

    for ( DWORD i = 1; i < cIndexRanges; i++ )
    {
        // If client does not want to know number of ranges read,
        // sort the ranges for them (still disallow overlapping ranges
        // use insertion sort, should be good enough here
        for ( DWORD j = 0; j < i; j++ )
        {
            INT cmp1 = CmpKey( pStartKeys[i], startKeyLengths[i], pEndKeys[j], endKeyLengths[j] );
            INT cmp2 = CmpKey( pEndKeys[i], endKeyLengths[i], pStartKeys[j], startKeyLengths[j] );
            BYTE *pvTmp;
            DWORD cbTmp;
            if ( fForward )
            {
                if ( cmp1 < 0 && cmp2 < 0 )
                {
                    // insert
                    pvTmp = pStartKeys[i];
                    memmove( pStartKeys+j+1, pStartKeys+j, (i-j)*sizeof(BYTE *) );
                    pStartKeys[j] = pvTmp;

                    cbTmp = startKeyLengths[i];
                    memmove( &startKeyLengths[j+1], &startKeyLengths[j], (i-j)*sizeof(DWORD) );
                    startKeyLengths[j] = cbTmp;

                    pvTmp = pEndKeys[i];
                    memmove( pEndKeys+j+1, pEndKeys+j, (i-j)*sizeof(BYTE *) );
                    pEndKeys[j] = pvTmp;

                    cbTmp = endKeyLengths[i];
                    memmove( &endKeyLengths[j+1], &endKeyLengths[j], (i-j)*sizeof(DWORD) );
                    endKeyLengths[j] = cbTmp;

                    break;
                }
                else if ( cmp1 < 0 || cmp2 < 0 )
                {
                    // overlapping ranges
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else if ( cmp1 > 0 && cmp2 > 0 )
            {
                // insert
                pvTmp = pStartKeys[i];
                memmove( pStartKeys+j+1, pStartKeys+j, (i-j)*sizeof(BYTE *) );
                pStartKeys[j] = pvTmp;

                cbTmp = startKeyLengths[i];
                memmove( &startKeyLengths[j+1], &startKeyLengths[j], (i-j)*sizeof(DWORD) );
                startKeyLengths[j] = cbTmp;

                pvTmp = pEndKeys[i];
                memmove( pEndKeys+j+1, pEndKeys+j, (i-j)*sizeof(BYTE *) );
                pEndKeys[j] = pvTmp;

                cbTmp = endKeyLengths[i];
                memmove( &endKeyLengths[j+1], &endKeyLengths[j], (i-j)*sizeof(DWORD) );
                endKeyLengths[j] = cbTmp;

                break;
            }
            else if ( cmp1 > 0 || cmp2 > 0 )
            {
                // overlapping ranges
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
    }

HandleError:
    return err;
}

JETUNITTEST( ErrRECIInsertionSort, SortKeys )
{
    PSTR start[] = { "ee", "a", "ccc" };
    DWORD startLength[] = { 2, 1, 3 };
    PSTR end[] = { "f", "bbb", "dd" };
    DWORD endLength[] = { 1, 3, 2 };

    ErrRECIInsertionSort( (BYTE **)start, startLength, (BYTE **)end, endLength, 3, fTrue );
#pragma prefast( push )
#pragma prefast( disable:26007, "test code" )
    CHECK( strcmp( start[0], "a" ) == 0 );
    CHECK( startLength[0] == 1 );
    CHECK( strcmp( start[1], "ccc" ) == 0 );
    CHECK( startLength[1] == 3 );
    CHECK( strcmp( start[2], "ee" ) == 0 );
    CHECK( startLength[2] == 2 );
    CHECK( strcmp( end[0], "bbb" ) == 0 );
    CHECK( endLength[0] == 3 );
    CHECK( strcmp( end[1], "dd" ) == 0 );
    CHECK( endLength[1] == 2 );
    CHECK( strcmp( end[2], "f" ) == 0 );
    CHECK( endLength[2] == 1 );

    ErrRECIInsertionSort( (BYTE **)end, endLength, (BYTE **)start, startLength, 3, fFalse );

    CHECK( strcmp( start[2], "a" ) == 0 );
    CHECK( startLength[2] == 1 );
    CHECK( strcmp( start[1], "ccc" ) == 0 );
    CHECK( startLength[1] == 3 );
    CHECK( strcmp( start[0], "ee" ) == 0 );
    CHECK( startLength[0] == 2 );
    CHECK( strcmp( end[2], "bbb" ) == 0 );
    CHECK( endLength[2] == 3 );
    CHECK( strcmp( end[1], "dd" ) == 0 );
    CHECK( endLength[1] == 2 );
    CHECK( strcmp( end[0], "f" ) == 0 );
    CHECK( endLength[0] == 1 );
#pragma prefast( pop )

    start[1] = "abc";

    CHECK( ErrRECIInsertionSort( (BYTE **)start, startLength, (BYTE **)end, endLength, 3, fTrue ) == JET_errInvalidParameter );
    CHECK( ErrRECIInsertionSort( (BYTE **)end, endLength, (BYTE **)start, startLength, 3, fFalse ) == JET_errInvalidParameter );
}


ERR ErrIsamIPrereadKeyRanges(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            vtid,
    _In_reads_(cIndexRanges) void **                            pStartKeys,
    _In_reads_(cIndexRanges) ULONG *                    startKeyLengths,
    _In_reads_opt_(cIndexRanges) void **                        pEndKeys,
    _In_reads_opt_(cIndexRanges) ULONG *                endKeyLengths,
    _In_ LONG                                                   cIndexRanges,
    _Out_opt_ ULONG * const                             pcRangesPreread,
    _In_reads_(ccolumnidPreread) const JET_COLUMNID * const     rgcolumnidPreread,
    _In_ const ULONG                                    ccolumnidPreread,
    _In_ const ULONG                                    cPageCacheMin,
    _In_ const ULONG                                    cPageCacheMax,
    _In_ JET_GRBIT                                              grbit,
    _Out_opt_ ULONG * const                             pcPageCacheActual )
{
    ERR     err = JET_errSuccess;
    LONG    iindexrangeT = 0;
    BOOL    fForward = fFalse;
    INT     cmp = 0;
    PIB     *const ppib = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );
    FUCB    *pfucbT = pfucbNil;
    LONG    cindexrangesPreread = 0;

    if( ( !( grbit & JET_bitPrereadBackward ) && !( grbit & JET_bitPrereadForward ) ) ||
        ( grbit & ~(JET_bitPrereadBackward|JET_bitPrereadForward|JET_bitPrereadFirstPage|bitPrereadSingletonRanges|JET_bitPrereadNormalizedKey) ) )
    {
        Call( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    fForward = !!( grbit & JET_bitPrereadForward );

    //  if prereading LVs then current index must be clustered index
    //
    if ( ccolumnidPreread > 0 && pfucb->pfucbCurIndex != pfucbNil )
    {
        Call( ErrERRCheck( JET_errInvalidPreread ) );
    }

    if ( pEndKeys == NULL && endKeyLengths != NULL ||
         pEndKeys != NULL && endKeyLengths == NULL )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pEndKeys == NULL )
    {
        pEndKeys = pStartKeys;
        endKeyLengths = startKeyLengths;
        grbit |= bitPrereadSingletonRanges;
    }

    //  check that all columnids provided are for long types which can be separated.  
    //  Long types that are constrained in the DDL not to ever be off page should 
    //  not be preread since they can never exist in there own pages.
    //
    ULONG               icolumnidT = 0;
    FCB                 * const pfcbTable = pfucb->u.pfcb;
    pfcbTable->EnterDML();
    TDB                 *ptdbT = pfcbTable->Ptdb();
    FIELD               *pfieldT = NULL;
    err = JET_errSuccess;
    for ( icolumnidT = 0; icolumnidT < ccolumnidPreread; icolumnidT++ )
    {
        if ( !FCOLUMNIDTagged( rgcolumnidPreread[icolumnidT] ) )
        {
            err = ErrERRCheck( JET_errMustBeSeparateLongValue );
            break;
        }
        pfieldT = ptdbT->PfieldTagged( rgcolumnidPreread[icolumnidT] );
        if ( !FRECLongValue( pfieldT->coltyp ) )
        {
            err = ErrERRCheck( JET_errMustBeSeparateLongValue );
            break;
        }
    }
    pfcbTable->LeaveDML();
    Call( err );

    for ( iindexrangeT = 0; iindexrangeT < (LONG)cIndexRanges; iindexrangeT++ )
    {
        // validate the arguments - make sure the keys are in order
        cmp = CmpKey( pStartKeys[iindexrangeT], startKeyLengths[iindexrangeT], pEndKeys[iindexrangeT], endKeyLengths[iindexrangeT] );
        if( fForward && cmp > 0 )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        else if( !fForward && cmp < 0 )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( pcRangesPreread != NULL && iindexrangeT > 0 )
        {
            // If client wants to know number of ranges read, enforce sorting
            cmp = CmpKey( pEndKeys[iindexrangeT-1], endKeyLengths[iindexrangeT-1], pStartKeys[iindexrangeT], startKeyLengths[iindexrangeT] );
            if( fForward && cmp > 0 )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
            else if( !fForward && cmp < 0 )
            {
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
    }

    if ( pcRangesPreread == NULL )
    {
        Call( ErrRECIInsertionSort( (BYTE **)pStartKeys, startKeyLengths, (BYTE **)pEndKeys, endKeyLengths, cIndexRanges, fForward ) );
    }

    //  preread primary or secondary index
    //
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        //  preread secondary index
        //
        Call( ErrBTPrereadKeyRanges(
            ppib,
            pfucb->pfucbCurIndex,
            (const VOID **)pStartKeys,
            startKeyLengths,
            (const VOID **)pEndKeys,
            endKeyLengths,
            cIndexRanges,
            &cindexrangesPreread,
            cPageCacheMin,
            cPageCacheMax,
            grbit,
            pcPageCacheActual ) );
    }
    else
    {
        //  preread primary index
        //
        Call( ErrBTPrereadKeyRanges(
            ppib,
            pfucb,
            (const VOID **)pStartKeys,
            startKeyLengths,
            (const VOID **)pEndKeys,
            endKeyLengths,
            cIndexRanges,
            &cindexrangesPreread,
            cPageCacheMin,
            cPageCacheMax,
            grbit,
            pcPageCacheActual ) );

        //  preread long values
        //
        if ( ccolumnidPreread > 0 )
        {
            LvId                rglid[clidMostPreread];
            memset( rglid, 0, sizeof(rglid) );
            INT                 ilidMac = 0;
            ULONG               cbActual = 0;
            JET_RETINFO         retinfoT;
            retinfoT.cbStruct = sizeof(JET_RETINFO);
            retinfoT.ibLongValue = 0;
            retinfoT.itagSequence = 1;
            retinfoT.columnidNextTagged = 0;
            LvId                lidT;

            Call( ErrFILEOpenTable( ppib, pfucb->ifmp, &pfucbT, pfucb->u.pfcb->Ptdb()->SzTableName() ) );
            
            Assert( pfucbNil != pfucbT );
        
            //  seek to each record via given key, and retreive all LIDs in all instances in all column ids provided
            //
            for( iindexrangeT = 0; ( iindexrangeT < cindexrangesPreread ) && ( ilidMac < _countof( rglid ) ); iindexrangeT++ )
            {
                //  seek to start of index range
                //
                Call( ErrIsamMakeKey( ppib, pfucbT, fForward ? pStartKeys[iindexrangeT] : pEndKeys[iindexrangeT], fForward ? startKeyLengths[iindexrangeT] : endKeyLengths[iindexrangeT], JET_bitNewKey|JET_bitNormalizedKey ) );
                err = ErrIsamSeek( ppib, pfucbT, JET_bitSeekGE );
                Assert( JET_errNoCurrentRecord != err );
                if ( JET_errRecordNotFound == err )
                {
                    //  if range empty then continue in a best effort fashion
                    //          
                    continue;
                }
                Call( err );

                //  set index range upper/lower+inclusive
                //
                Call( ErrIsamMakeKey( ppib, pfucbT, fForward ? pEndKeys[iindexrangeT] : pStartKeys[iindexrangeT], fForward ? endKeyLengths[iindexrangeT] : startKeyLengths[iindexrangeT], JET_bitNewKey|JET_bitNormalizedKey ) );
                err = ErrIsamSetIndexRange ( ppib, pfucbT, JET_bitRangeUpperLimit |JET_bitRangeInclusive );
                if ( JET_errNoCurrentRecord == err )
                {
                    //  if range empty then continue in best effort fashion
                    //
                    continue;
                }
                Call( err );
                
                //  retrieve LIDs for given columns for each record in range
                //
                while ( ilidMac < _countof( rglid ) )
                {
                    //  retrieve LIDs for given columns
                    //
                    for ( icolumnidT = 0; ( icolumnidT < ccolumnidPreread ) && ( ilidMac < _countof( rglid ) ); icolumnidT++ )
                    {
                        //  for each column, retrieve LIDs from all instances
                        //
                        for( retinfoT.itagSequence = 1; ilidMac < _countof( rglid ); retinfoT.itagSequence++ )
                        {
                            Call( ErrIsamRetrieveColumn( ppib, pfucbT, rgcolumnidPreread[icolumnidT], &lidT, sizeof(lidT),&cbActual, JET_bitRetrieveLongId, &retinfoT ) );
                            if ( err == JET_wrnColumnNull )
                            {
                                break;
                            }
                            if ( err == JET_wrnSeparateLongValue )
                            {
                                Assert( cbActual == sizeof( lidT ) );
                                Assert( ilidMac < _countof( rglid ) );
                                rglid[ilidMac++] = lidT;
                            }
                        }
                    }
                    err = ErrIsamMove( ppib, pfucbT, JET_MoveNext, NO_GRBIT );
                    if ( JET_errNoCurrentRecord == err )
                    {
                        break;
                    }
                    Call( err );
                }
            }
            

            //  preread LIDs if any found
            //
            if ( ilidMac > 0 )
            {
                LONG clidT;
                ULONG cPageT;
                Call( ErrLVPrereadLongValues( pfucb, rglid, ilidMac, 0, ulMax, &clidT, &cPageT, grbit ) );
            }

            //  Users dont expect seek errors from an async preread api
            //
            if ( JET_errNoCurrentRecord == err || JET_errRecordNotFound == err )
            {
                err = JET_errSuccess;
            }
        }
    }

    //  return progress if requested.
    //  Note that we cannot use LV preread as a contraint on ranges processed because the relationship between column LVs and ranges is not kept.
    //
    if( pcRangesPreread )
    {
        //  must return progress of at least one range so application can progress through even if resources unavailable to fully process even one range
        //
        Assert( cindexrangesPreread >= 1 );
        *pcRangesPreread = cindexrangesPreread;
    }
    
HandleError:
    if ( pfucbNil != pfucbT )
    {
        CallS( ErrFILECloseTable( ppib, pfucbT ) );
    }

    return err;
}

//  =================================================================
ERR ErrIsamIPrereadIndexRanges(
    const JET_SESID                                 sesid,
    const JET_VTID                                  vtid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const rgIndexRanges,
    __in const ULONG                        cIndexRanges,
    __out_opt ULONG * const                 pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const rgcolumnidPreread,
    const ULONG                                     ccolumnidPreread,
    __in const ULONG                        cPageCacheMin,
    __in const ULONG                        cPageCacheMax,
    const JET_GRBIT                                 grbit,
    __out_opt ULONG * const                 pcPageCacheActual )
//  =================================================================
{
    ERR     err = JET_errSuccess;
    LONG    iindexrangeT = 0;
    BYTE    **pStartKeys = NULL;
    BYTE    **pEndKeys = NULL;
    DWORD   *startKeyLengths = NULL;
    DWORD   *endKeyLengths = NULL;
    PIB     *const ppib = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );
    BOOL    fAllSingletonRanges = fTrue;

    Alloc( pStartKeys = new (BYTE *[cIndexRanges]) );
    memset( pStartKeys, 0, sizeof(BYTE *) * cIndexRanges );
    Alloc( startKeyLengths = new DWORD[cIndexRanges] );
    memset( startKeyLengths, 0, sizeof(DWORD) * cIndexRanges );
    Alloc( pEndKeys = new (BYTE *[cIndexRanges]) );
    memset( pEndKeys, 0, sizeof(BYTE *) * cIndexRanges );
    Alloc( endKeyLengths = new DWORD[cIndexRanges] );
    memset( endKeyLengths, 0, sizeof(DWORD) * cIndexRanges );

    for ( iindexrangeT = 0; iindexrangeT < (LONG)cIndexRanges; iindexrangeT++ )
    {
        if ( grbit & JET_bitPrereadNormalizedKey )
        {
            if ( rgIndexRanges[iindexrangeT].cStartColumns != 1 &&
                 rgIndexRanges[iindexrangeT].cEndColumns > 1 )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            pStartKeys[iindexrangeT] = (BYTE *)rgIndexRanges[iindexrangeT].rgStartColumns[0].pv;
            startKeyLengths[iindexrangeT] = rgIndexRanges[iindexrangeT].rgStartColumns[0].cb;
            if ( rgIndexRanges[iindexrangeT].cEndColumns == 1 )
            {
                pEndKeys[iindexrangeT] = (BYTE *)rgIndexRanges[iindexrangeT].rgEndColumns[0].pv;
                endKeyLengths[iindexrangeT] = rgIndexRanges[iindexrangeT].rgEndColumns[0].cb;

                fAllSingletonRanges = fFalse;
            }
        }
        else
        {
            Call( ErrRECIMakeKey(
                    ppib,
                    pfucb,
                    rgIndexRanges[iindexrangeT].rgStartColumns,
                    rgIndexRanges[iindexrangeT].cStartColumns,
                    !!( grbit & JET_bitPrereadForward ),
                    &pStartKeys[iindexrangeT],
                    &startKeyLengths[iindexrangeT] ) );
            fAllSingletonRanges = fAllSingletonRanges && ( rgIndexRanges[iindexrangeT].cEndColumns == 0 );
            Call( ErrRECIMakeKey(
                    ppib,
                    pfucb,
                    rgIndexRanges[iindexrangeT].rgEndColumns,
                    rgIndexRanges[iindexrangeT].cEndColumns,
                    !( grbit & JET_bitPrereadForward ),
                    &pEndKeys[iindexrangeT],
                    &endKeyLengths[iindexrangeT] ) );
        }
    }

    Call( ErrIsamIPrereadKeyRanges( sesid,
                                   vtid,
                                   (VOID **)pStartKeys,
                                   startKeyLengths,
                                   fAllSingletonRanges ? NULL : (VOID **)pEndKeys,
                                   fAllSingletonRanges ? NULL : endKeyLengths,
                                   cIndexRanges,
                                   pcRangesPreread,
                                   rgcolumnidPreread,
                                   ccolumnidPreread,
                                   cPageCacheMin,
                                   cPageCacheMax,
                                   grbit | ( fAllSingletonRanges ? bitPrereadSingletonRanges : 0 ),
                                   pcPageCacheActual ) );

HandleError:
    if ( !(grbit & JET_bitPrereadNormalizedKey) )
    {
        for ( iindexrangeT = 0; iindexrangeT < (LONG)cIndexRanges; iindexrangeT++ )
        {
            if ( pStartKeys )
            {
                delete[] pStartKeys[iindexrangeT];
            }
            if ( pEndKeys )
            {
                delete[] pEndKeys[iindexrangeT];
            }
        }
    }
    delete[] pStartKeys;
    delete[] startKeyLengths;
    delete[] pEndKeys;
    delete[] endKeyLengths;

    return err;
}

//  =================================================================
ERR VTAPI ErrIsamPrereadIndexRanges(
    const JET_SESID                                             sesid,
    const JET_VTID                                              vtid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const     rgIndexRanges,
    __in const ULONG                                    cIndexRanges,
    __out_opt ULONG * const                             pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const    rgcolumnidPreread,
    const ULONG                                                 ccolumnidPreread,
    const JET_GRBIT                                             grbit )
//  =================================================================
{
    return ErrIsamIPrereadIndexRanges(
        sesid,
        vtid,
        rgIndexRanges,
        cIndexRanges,
        pcRangesPreread,
        rgcolumnidPreread,
        ccolumnidPreread,
        0,
        ulMax,
        grbit,
        NULL
        );
}

ERR VTAPI ErrIsamPrereadKeyRanges(
    _In_ JET_SESID                                              sesid,
    _In_ JET_TABLEID                                            vtid,
    _In_reads_(cIndexRanges) void **                            pStartKeys,
    _In_reads_(cIndexRanges) ULONG *                    startKeyLengths,
    _In_reads_opt_(cIndexRanges) void **                        pEndKeys,
    _In_reads_opt_(cIndexRanges) ULONG *                endKeyLengths,
    _In_ LONG                                                   cIndexRanges,
    _Out_opt_ ULONG * const                             pcRangesPreread,
    _In_reads_(ccolumnidPreread) const JET_COLUMNID * const     rgcolumnidPreread,
    _In_ const ULONG                                    ccolumnidPreread,
    _In_ JET_GRBIT                                              grbit )
{
    return ErrIsamIPrereadKeyRanges(
        sesid,
        vtid,
        pStartKeys,
        startKeyLengths,
        pEndKeys,
        endKeyLengths,
        cIndexRanges,
        pcRangesPreread,
        rgcolumnidPreread,
        ccolumnidPreread,
        0,
        ulMax,
        grbit,
        NULL
        );
}

//  =================================================================
ERR VTAPI ErrIsamPrereadIndexRange(
    _In_  JET_SESID                     sesid,
    _In_  JET_TABLEID                   vtid,
    _In_  const JET_INDEX_RANGE * const pIndexRange,
    _In_  const ULONG           cPageCacheMin,
    _In_  const ULONG           cPageCacheMax,
    _In_  const JET_GRBIT               grbit,
    _Out_opt_ ULONG * const     pcPageCacheActual )
//  =================================================================
{
    if ( pIndexRange == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( cPageCacheMin > cPageCacheMax )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( cPageCacheMax > lMax )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( grbit & ~( JET_bitPrereadForward | JET_bitPrereadBackward | JET_bitPrereadNormalizedKey ) )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }
    if ( !!( grbit & JET_bitPrereadForward ) == !!( grbit & JET_bitPrereadBackward ) )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    return ErrIsamIPrereadIndexRanges(
        sesid,
        vtid,
        pIndexRange,
        1,
        NULL,
        NULL,
        0,
        cPageCacheMin,
        cPageCacheMax,
        grbit,
        pcPageCacheActual
        );
}

LOCAL ERR ErrRECIGotoBookmark(
    PIB*                ppib,
    FUCB *              pfucb,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark,
    const ULONG         itag )
{
    ERR                 err;
    BOOKMARK            bm;
    ULONG               iidxsegT;

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    Assert( FFUCBIndex( pfucb ) );
    Assert( FFUCBPrimary( pfucb ) );
    Assert( pfucb->u.pfcb->FPrimaryIndex() );
    CheckSecondary( pfucb );

    if( 0 == cbBookmark || NULL == pvBookmark )
    {
        //  don't pass a NULL bookmark into the DIR level
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    //  reset copy buffer status
    //
    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

    //  reset index range limit
    //
    DIRResetIndexRange( pfucb );

    KSReset( pfucb );

    //  get node, and return error if this node is not there for caller.
    //
    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( const_cast<VOID *>( pvBookmark ) );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrDIRGotoJetBookmark( pfucb, bm, fFalse ) );
    AssertDIRNoLatch( ppib );

    Assert( pfucb->u.pfcb->FPrimaryIndex() );
    Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );

    //  goto bookmark record build key for secondary index
    //  to bookmark record
    //
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        //  get secondary index cursor
        //
        FUCB        *pfucbIdx = pfucb->pfucbCurIndex;
        IDB         *pidb;
        KEY         key;

        Assert( pfucbIdx->u.pfcb != pfcbNil );
        Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );

        pidb = pfucbIdx->u.pfcb->Pidb();
        Assert( pidb != pidbNil );
        Assert( !pidb->FPrimary() );

        KSReset( pfucbIdx );

        KeySequence ksT( pfucb, pidb, itag );

        //  allocate goto bookmark resources
        //
        if ( NULL == pfucbIdx->dataSearchKey.Pv() )
        {
            pfucbIdx->dataSearchKey.SetPv( RESKEY.PvRESAlloc() );
            if ( NULL == pfucbIdx->dataSearchKey.Pv() )
                return ErrERRCheck( JET_errOutOfMemory );
            pfucbIdx->dataSearchKey.SetCb( cbKeyAlloc );
        }

        //  make key for record for secondary index
        //
        key.prefix.Nullify();
        key.suffix.SetPv( pfucbIdx->dataSearchKey.Pv() );
        key.suffix.SetCb( pfucbIdx->dataSearchKey.Cb() );
        Call( ErrRECRetrieveKeyFromRecord( pfucb, pidb, &key, ksT.Rgitag(), 0, fFalse, &iidxsegT ) );
        AssertDIRNoLatch( ppib );
        if ( wrnFLDOutOfKeys == err )
        {
            Assert( !ksT.FBaseKey() );
            err = ErrERRCheck( JET_errBadItagSequence );
            goto HandleError;
        }

        //  record must honor index no NULL segment requirements
        //
        if ( pidb->FNoNullSeg() )
        {
            Assert( wrnFLDNullSeg != err );
            Assert( wrnFLDNullFirstSeg != err );
            Assert( wrnFLDNullKey != err );
        }

        //  if item is not index, then move before first instead of seeking
        //
        Assert( err > 0 || JET_errSuccess == err );
        if ( err > 0
            && ( ( wrnFLDNullKey == err && !pidb->FAllowAllNulls() )
                || ( wrnFLDNullFirstSeg == err && !pidb->FAllowFirstNull() )
                || ( wrnFLDNullSeg == err && !pidb->FAllowSomeNulls() ) )
                || wrnFLDOutOfTuples == err
                || wrnFLDNotPresentInIndex == err )
        {
            //  assumes that NULLs sort low
            //
            DIRBeforeFirst( pfucbIdx );
            err = ErrERRCheck( JET_errNoCurrentRecord );
        }
        else
        {
            //  move to DATA root
            //
            DIRGotoRoot( pfucbIdx );

            //  seek on secondary key and primary key as data
            //
            Assert( bm.key.prefix.FNull() );
            Call( ErrDIRDownKeyData( pfucbIdx, key, bm.key.suffix ) );
            CallS( err );
        }
    }

    CallSx( err, JET_errNoCurrentRecord );

HandleError:
    OSTraceFMP(
        pfucb->ifmp,
        JET_tracetagCursorNavigation,
        OSFormat(
            "Session=[0x%p:0x%x] has currency %d on node=[0x%x:0x%x:0x%x] of objid=[0x%x:0x%x] after GotoBM [itag=0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            pfucb->locLogical,
            (ULONG)pfucb->ifmp,
            Pcsr( pfucb )->Pgno(),
            Pcsr( pfucb )->ILine(),
            (ULONG)pfucb->ifmp,
            pfucb->u.pfcb->ObjidFDP(),
            itag,
            err,
            err ) );

    AssertDIRNoLatch( ppib );
    return err;
}

ERR VTAPI ErrIsamGotoBookmark(
    JET_SESID           sesid,
    JET_VTID            vtid,
    const VOID * const  pvBookmark,
    const ULONG         cbBookmark )
{
    return ErrRECIGotoBookmark(
                        reinterpret_cast<PIB *>( sesid ),
                        reinterpret_cast<FUCB *>( vtid ),
                        pvBookmark,
                        cbBookmark,
                        1 );
}

ERR VTAPI ErrIsamGotoIndexBookmark(
    JET_SESID           sesid,
    JET_VTID            vtid,
    const VOID * const  pvSecondaryKey,
    const ULONG         cbSecondaryKey,
    const VOID * const  pvPrimaryBookmark,
    const ULONG         cbPrimaryBookmark,
    const JET_GRBIT     grbit )
{
    ERR                 err;
    PIB *               ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB *              pfucb       = reinterpret_cast<FUCB *>( vtid );
    FUCB * const        pfucbIdx    = pfucb->pfucbCurIndex;
    BOOL                fInTrx      = fFalse;
    BOOKMARK            bm;

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    Assert( FFUCBIndex( pfucb ) );
    Assert( FFUCBPrimary( pfucb ) );
    Assert( pfucb->u.pfcb->FPrimaryIndex() );

    if ( pfucbNil == pfucbIdx )
    {
        return ErrERRCheck( JET_errNoCurrentIndex );
    }

    Assert( !pfucb->pmoveFilterContext );

    Assert( FFUCBSecondary( pfucbIdx ) );
    Assert( pfucbIdx->u.pfcb->FTypeSecondaryIndex() );

    if( 0 == cbSecondaryKey || NULL == pvSecondaryKey )
    {
        //  don't pass a NULL bookmark into the DIR level
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( const_cast<VOID *>( pvSecondaryKey ) );
    bm.key.suffix.SetCb( cbSecondaryKey );

    if ( FFUCBUnique( pfucbIdx ) )
    {
        //  don't need primary bookmark, even if one was specified
        bm.data.Nullify();
    }
    else
    {
        if ( 0 == cbPrimaryBookmark || NULL == pvPrimaryBookmark )
            return ErrERRCheck( JET_errInvalidBookmark );

        bm.data.SetPv( const_cast<VOID *>( pvPrimaryBookmark ) );
        bm.data.SetCb( cbPrimaryBookmark );
    }


    //  reset copy buffer status
    //
    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

    //  reset index range limit
    //
    DIRResetIndexRange( pfucb );

    KSReset( pfucb );
    KSReset( pfucbIdx );

    if ( 0 == ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( ppib, 35621, JET_bitTransactionReadOnly ) );
        fInTrx = fTrue;
    }

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !Pcsr( pfucbIdx )->FLatched() );

    //  move secondary cursor to desired location
    err = ErrDIRGotoJetBookmark( pfucbIdx, bm, fTrue );

    //  Within ErrDIRGotoJetBookmark(), JET_errRecordDeleted
    //  from ErrBTGotoBookmark() means that the node with the
    //  specified bookmark was physically expunged, and
    //  JET_errRecordDeleted from ErrBTGet() means that the
    //  node is still physically present, but not visible
    //  to this cursor. In either case, establish virtual
    //  currency (so DIRNext/Prev() will work) if permitted.
    if ( JET_errRecordDeleted == err
        && ( grbit & JET_bitBookmarkPermitVirtualCurrency ) )
    {
        Assert( !Pcsr( pfucbIdx )->FLatched() );
        Call( ErrBTDeferGotoBookmark( pfucbIdx, bm, fFalse/*no touch*/ ) );
        pfucbIdx->locLogical = locOnSeekBM;
    }
    else
    {
        Call( err );
        CallS( err );

        Assert( Pcsr( pfucbIdx )->FLatched() );
        Assert( locOnCurBM == pfucbIdx->locLogical );

        Assert( pfucbIdx->kdfCurr.data.Pv() != NULL );
        Assert( pfucbIdx->kdfCurr.data.Cb() > 0 );

        //  primary key can be found in the data portion of the
        //  secondary bookmark on the index page (but not
        //  necessarily in the bmCurr, because we optimise
        //  that out on unique secondary indices)
        //
        Assert( bm.key.prefix.FNull() );
        bm.key.suffix = pfucbIdx->kdfCurr.data;
        bm.data.Nullify();

        //  move primary cursor to match secondary cursor
        //
        const ERR   errGotoBM   = ErrBTDeferGotoBookmark( pfucb, bm, fTrue/*touch*/ );
        CallSx( errGotoBM, JET_errOutOfMemory );

        //  now that we're done with the index page, release the latch
        //  (regardless of whether we err'd out setting the primary cursor)
        //
        err = ErrBTRelease( pfucbIdx );
        CallSx( err, JET_errOutOfMemory );

        if ( err < 0 || errGotoBM < 0 )
        {
            //  force both cursors to a virtual currency
            //  UNDONE: I'm not sure what exactly the
            //  appropriate currency should be if we
            //  err out here
            pfucbIdx->locLogical = locOnSeekBM;
            pfucb->locLogical = locOnSeekBM;

            err = ( err < 0 ? err : errGotoBM );
            goto HandleError;
        }

        Assert( ( pfucbIdx->u.pfcb->FUnique() && pfucbIdx->bmCurr.data.FNull() )
            || ( !pfucbIdx->u.pfcb->FUnique() && !pfucbIdx->bmCurr.data.FNull() ) );
    }

    Assert( !Pcsr( pfucb )->FLatched() );
    Assert( !Pcsr( pfucbIdx )->FLatched() );

    //  The secondary index cursor may have been placed on a
    //  virtual record, so we must update the primary
    //  index cursor as well (if not, then it's possible
    //  to do, for instance, a RetrieveColumn on the
    //  primary cursor and you'll get back data from the
    //  record you were on before the seek but a
    //  RetrieveFromIndex on the secondary cursor will
    //  return JET_errNoCurrentRecord).
    //  Note that although the locLogical of the primary
    //  cursor is being updated, it's not necessary to
    //  update the primary cursor's bmCurr, because it
    //  will never be accessed (the secondary cursor takes
    //  precedence).  The only reason we reset the
    //  locLogical is for error-handling so that we
    //  properly err out with JET_errNoCurrentRecord if
    //  someone tries to use the cursor to access a record
    //  before repositioning the secondary cursor to a true
    //  record.
    Assert( locOnCurBM == pfucbIdx->locLogical
        || locOnSeekBM == pfucbIdx->locLogical );
    pfucb->locLogical = pfucbIdx->locLogical;

    if ( locOnSeekBM == pfucbIdx->locLogical )
    {
        //  if currency was placed on virtual bookmark, record must have gotten deleted
        Call( ErrERRCheck( JET_errRecordDeleted ) );
    }

    else if ( FFUCBUnique( pfucbIdx ) && 0 != cbPrimaryBookmark )
    {
        //  on a unique index, we'll ignore any primary bookmark the
        //  user may have specified, so need to verify this is truly
        //  the record the user requested
        if ( (ULONG)pfucb->bmCurr.key.suffix.Cb() != cbPrimaryBookmark
            || 0 != memcmp( pfucb->bmCurr.key.suffix.Pv(), pvPrimaryBookmark, cbPrimaryBookmark ) )
        {
            //  index entry was found, but has a different primary
            //  bookmark, so must assume that original record
            //  was deleted (and new record subsequently got
            //  inserted with same secondary index key, but
            //  different primary key)
            //
            //  NOTE: the user's currency will be left on the
            //  index entry matching the desired key, even
            //  though we're erring out because the primary
            //  bookmark doesn't match - this makes things
            //  really interesting if PermitVirtualCurrency
            //  is specified
            //
            Call( ErrERRCheck( JET_errRecordDeleted ) );
        }
    }

    CallS( err );

HandleError:
    if ( fInTrx )
    {
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( ppib );
    return err;
}


ERR VTAPI ErrIsamGotoPosition( JET_SESID sesid, JET_VTID vtid, JET_RECPOS *precpos )
{
    PIB *ppib   = reinterpret_cast<PIB *>( sesid );
    FUCB *pfucb = reinterpret_cast<FUCB *>( vtid );

    ERR     err;
    FUCB    *pfucbSecondary;

    // filters do not apply to fractional moves
    if ( pfucb->pmoveFilterContext )
    {
        return ErrERRCheck( JET_errFilteredMoveNotSupported );
    }

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    if ( precpos->centriesLT > precpos->centriesTotal )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        return err;
    }

    //  Reset copy buffer status
    //
    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

    //  reset index range limit
    //
    DIRResetIndexRange( pfucb );

    //  reset key stat
    //
    KSReset( pfucb );

    //  set non primary index pointer, may be null
    //
    pfucbSecondary = pfucb->pfucbCurIndex;

    if ( pfucbSecondary == pfucbNil )
    {
        //  move to DATA root
        //
        DIRGotoRoot( pfucb );

        err = ErrDIRGotoPosition( pfucb, precpos->centriesLT, precpos->centriesTotal );

        if ( err >= 0 )
        {
            Assert( Pcsr( pfucb )->FLatched( ) );

            err = ErrDIRRelease( pfucb );
        }
    }
    else
    {
        ERR         errT = JET_errSuccess;

        //  move to DATA root
        //
        DIRGotoRoot( pfucbSecondary );

        err = ErrDIRGotoPosition( pfucbSecondary, precpos->centriesLT, precpos->centriesTotal );

        //  if the movement was successful and a secondary index is
        //  in use, then position primary index to record.
        //
        if ( JET_errSuccess == err )
        {
            //  goto bookmark pointed to by secondary index node
            //
            BOOKMARK    bmRecord;

            Assert(pfucbSecondary->kdfCurr.data.Pv() != NULL);
            Assert(pfucbSecondary->kdfCurr.data.Cb() > 0 );

            bmRecord.key.prefix.Nullify();
            bmRecord.key.suffix = pfucbSecondary->kdfCurr.data;
            bmRecord.data.Nullify();

            //  We will need to touch the data page buffer.

            errT = ErrDIRGotoBookmark( pfucb, bmRecord );

            Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );
            Assert( pfucb->u.pfcb->FPrimaryIndex() );
        }

        if ( err >= 0 )
        {
            //  release latch
            //
            err = ErrDIRRelease( pfucbSecondary );

            if ( errT < 0 && err >= 0 )
            {
                //  propagate the more severe error
                //
                err = errT;
            }
        }
        else
        {
            AssertDIRNoLatch( ppib );
        }
    }

    AssertDIRNoLatch( ppib );

    //  if no records then return JET_errRecordNotFound
    //  otherwise return error from called routine
    //
    if ( err < 0 )
    {
        DIRBeforeFirst( pfucb );

        if ( pfucbSecondary != pfucbNil )
        {
            DIRBeforeFirst( pfucbSecondary );
        }
    }
    else
    {
        Assert ( JET_errSuccess == err || wrnNDFoundLess == err || wrnNDFoundGreater == err );
        err = JET_errSuccess;
    }

    return err;
}


ERR VTAPI ErrIsamSetIndexRange( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
{
    ERR     err         = JET_errSuccess;
    PIB     *ppib       = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucbTable = reinterpret_cast<FUCB *>( vtid );
    FUCB    *pfucb      = pfucbTable->pfucbCurIndex ?
                            pfucbTable->pfucbCurIndex : pfucbTable;

    /*  ppib is not used in this function
    /**/
    AssertDIRNoLatch( ppib );

    /*  if instant duration index range, then reset index range.
    /**/
    if ( grbit & JET_bitRangeRemove )
    {
        if ( FFUCBLimstat( pfucb ) )
        {
            DIRResetIndexRange( pfucbTable );
            CallS( err );
            goto HandleError;
        }
        else
        {
            Call( ErrERRCheck( JET_errInvalidOperation ) );
        }
    }

    /*  must be on index
    /**/
    if ( pfucb == pfucbTable )
    {
        FCB     *pfcbTable = pfucbTable->u.pfcb;
        BOOL    fPrimaryIndexTemplate = fFalse;

        Assert( pfcbNil != pfcbTable );
        Assert( pfcbTable->FPrimaryIndex() );

        if ( pfcbTable->FDerivedTable() )
        {
            Assert( pfcbTable->Ptdb() != ptdbNil );
            Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
            FCB *pfcbTemplateTable = pfcbTable->Ptdb()->PfcbTemplateTable();
            if ( !pfcbTemplateTable->FSequentialIndex() )
            {
                //  If template table has a primary index,
                //  we must have inherited it,
                fPrimaryIndexTemplate = fTrue;
                Assert( pfcbTemplateTable->Pidb() != pidbNil );
                Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
                Assert( pfcbTable->Pidb() == pfcbTemplateTable->Pidb() || pfcbTable->Pidb()->FIDBOwnedByFCB() );
            }
            else
            {
                Assert( pfcbTemplateTable->Pidb() == pidbNil );
            }
        }

        //  if primary index was present when schema was faulted in,
        //  no need for further check because we don't allow
        //  the primary index to be deleted
        if ( !fPrimaryIndexTemplate && !pfcbTable->FInitialIndex() )
        {
            pfcbTable->EnterDML();
            if ( pidbNil == pfcbTable->Pidb() )
            {
                err = ErrERRCheck( JET_errNoCurrentIndex );
            }
            else
            {
                err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbTable );
                if ( JET_errIndexNotFound == err )
                    err = ErrERRCheck( JET_errNoCurrentIndex );
            }
            pfcbTable->LeaveDML();
            Call( err );
        }
    }

    /*  key must be prepared
    /**/
    if ( !( FKSPrepared( pfucb ) ) )
    {
        Call( ErrERRCheck( JET_errKeyNotMade ) );
    }

    FUCBAssertValidSearchKey( pfucb );

    /*  set index range and check current position
    /**/
    DIRSetIndexRange( pfucb, grbit );
    err = ErrDIRCheckIndexRange( pfucb );

    /*  reset key status
    /**/
    KSReset( pfucb );

    /*  if instant duration index range, then reset index range.
    /**/
    if ( grbit & JET_bitRangeInstantDuration )
    {
        DIRResetIndexRange( pfucbTable );
    }

HandleError:
    AssertDIRNoLatch( ppib );
    return err;
}


ERR ErrIsamSetCurrentIndex(
    JET_SESID           vsesid,
    JET_VTID            vtid,
    const CHAR          *szName,
    const JET_INDEXID   *pindexid,          //  default = NULL
    const JET_GRBIT     grbit,              //  default = JET_bitMoveFirst
    const ULONG         itagSequence )      //  default = 1
{
    ERR                 err;
    PIB                 *ppib           = (PIB *)vsesid;
    FUCB                *pfucb          = (FUCB *)vtid;
    CHAR                *szIndex;
    CHAR                szIndexNameBuf[ (JET_cbNameMost + 1) ];

    // filters do not migrate to new index
    if ( pfucb->pmoveFilterContext )
    {
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    Assert( JET_bitMoveFirst == grbit || JET_bitNoMove == grbit );

    //  index name is ignored if an indexid is passed in
    //
    //  a null or empty index name indicates switching to primary index
    //
    if ( NULL != pindexid
        || NULL == szName
        || '\0' == *szName )
    {
        szIndex = NULL;
    }
    else
    {
        szIndex = szIndexNameBuf;
        Call( ErrUTILCheckName( szIndex, szName, (JET_cbNameMost + 1) ) );
    }

    if ( JET_bitMoveFirst == grbit )
    {
        //  Reset copy buffer status
        //
        if ( FFUCBUpdatePrepared( pfucb ) )
        {
            CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
        }

        //  change index and defer move first
        //
        Call( ErrRECSetCurrentIndex( pfucb, szIndex, (INDEXID *)pindexid ) );
        AssertDIRNoLatch( ppib );

        if( pfucb->u.pfcb->FPreread() && pfucbNil != pfucb->pfucbCurIndex )
        {
            //  Preread the root of the index
            PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
            tcScope->nParentObjectClass = TceFromFUCB( pfucb->pfucbCurIndex );
            tcScope->SetDwEngineObjid( ObjidFDP( pfucb->pfucbCurIndex ) );

            (void)ErrBFPrereadPage( pfucb->ifmp, pfucb->pfucbCurIndex->u.pfcb->PgnoFDP(), bfprfDefault, ppib->BfpriPriority( pfucb->ifmp ), *tcScope );
        }

        RECDeferMoveFirst( ppib, pfucb );
    }
    else
    {
        Assert( JET_bitNoMove == grbit );

        //  get bookmark of current record, change index,
        //  and goto bookmark.
        BOOKMARK    *pbm;

        Call( ErrDIRGetBookmark( pfucb, &pbm ) );
        AssertDIRNoLatch( ppib );

        Call( ErrRECSetCurrentIndex( pfucb, szIndex, (INDEXID *)pindexid ) );

        //  UNDONE: error handling.  We should not have changed
        //  currency or current index, if set current index
        //  fails for any reason.  Note that this functionality
        //  could be provided by duplicating the cursor, on
        //  the primary index, setting the current index to the
        //  new index, getting the bookmark from the original
        //  cursor, goto bookmark on the duplicate cursor,
        //  instating the duplicate cursor for the table id of
        //  the original cursor, and closing the original cursor.
        //
        Assert( pbm->key.Cb() > 0 );
        Assert( pbm->data.Cb() == 0 );

        Assert( pbm->key.prefix.FNull() );
        Call( ErrRECIGotoBookmark(
                    pfucb->ppib,
                    pfucb,
                    pbm->key.suffix.Pv(),
                    pbm->key.suffix.Cb(),
                    max( 1, itagSequence ) ) );
    }

HandleError:
    AssertDIRNoLatch( ppib );
    return err;
}


ERR ErrRECSetCurrentIndex(
    FUCB            *pfucb,
    const CHAR      *szIndex,
    const INDEXID   *pindexid )
{
    ERR             err;
    FCB             *pfcbTable;
    FCB             *pfcbSecondary;
    FUCB            **ppfucbCurIdx;
    FUCB            * pfucbOldIndex         = pfucbNil;
    BOOL            fSettingToPrimaryIndex  = fFalse;
    BOOL            fInDMLLatch             = fFalse;
    BOOL            fIncrementedRefCount    = fFalse;
    BOOL            fClosedPreviousIndex    = fFalse;

    Assert( pfucb != pfucbNil );
    Assert( FFUCBIndex( pfucb ) );
    AssertDIRNoLatch( pfucb->ppib );

    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );
    Assert( pfcbTable->FPrimaryIndex() );

    Assert( pfcbTable->Ptdb() != ptdbNil );

    ppfucbCurIdx = &pfucb->pfucbCurIndex;

    //  caller should have verified that an
    //  empty index name is not being passed in
    //
    Assert( NULL == szIndex || '\0' != *szIndex );

    OSTraceFMP(
        pfcbTable->Ifmp(),
        JET_tracetagCursorNavigation,
        OSFormat(
            "Session=[0x%p:0x%x] setting current index to '%s' of objid=[0x%x:0x%x]",
            pfucb->ppib,
            ( ppibNil != pfucb->ppib ? pfucb->ppib->trxBegin0 : trxMax ),
            ( NULL != pindexid ? "<indexid>" : ( NULL != szIndex ? szIndex : "<primary>" ) ),
            (ULONG)pfcbTable->Ifmp(),
            pfcbTable->ObjidFDP() ) );

    //  NOTE: index name is ignored if an indexid
    //  is specified
    //
    if ( NULL != pindexid )
    {
        if ( sizeof(INDEXID) != pindexid->cbStruct )
        {
            return ErrERRCheck( JET_errInvalidIndexId );
        }

        const FCB   * pfcbIndex = pindexid->pfcbIndex;

        if ( pfcbIndex == pfcbTable )
        {
            fSettingToPrimaryIndex = fTrue;
            if ( pfcbIndex->ObjidFDP() != pindexid->objidFDP
                || pfcbIndex->PgnoFDP() != pindexid->pgnoFDP )
            {
                return ErrERRCheck( JET_errInvalidIndexId );
            }
        }

        else if ( NULL != *ppfucbCurIdx && pfcbIndex == (*ppfucbCurIdx)->u.pfcb )
        {
            //  switching to the current index
            //
            return ( pfcbIndex->ObjidFDP() != pindexid->objidFDP
                    || pfcbIndex->PgnoFDP() != pindexid->pgnoFDP ?
                            ErrERRCheck( JET_errInvalidIndexId ) :
                            JET_errSuccess );

        }

        else if ( !pfcbIndex->FValid() )
        {
//          AssertSz( fFalse, "Bogus FCB pointer." );
            return ErrERRCheck( JET_errInvalidIndexId );
        }

        else
        {
            //  verify index visibility
            pfcbTable->EnterDML();

            if ( pfcbIndex->ObjidFDP() != pindexid->objidFDP
                || pfcbIndex->PgnoFDP() != pindexid->pgnoFDP
                || !pfcbIndex->FTypeSecondaryIndex()
                || pfcbIndex->PfcbTable() != pfcbTable )
            {
                err = ErrERRCheck( JET_errInvalidIndexId );
            }
            else
            {
                err = ErrFILEIAccessIndex( pfucb->ppib, pfcbTable, pfcbIndex );
            }

            if ( err < 0 )
            {
                pfcbTable->LeaveDML();

                //  return JET_errInvalidIndexId if index not visible to us
                return ( JET_errIndexNotFound == err ?
                                ErrERRCheck( JET_errInvalidIndexId ) :
                                err );
            }

            else if ( pfcbIndex->Pidb()->FTemplateIndex() )
            {
                // Don't need refcount on template indexes, since we
                // know they'll never go away.
                Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
            }

            else
            {
                //  pin the index until we're ready to open a cursor on it
                pfcbIndex->Pidb()->IncrementCurrentIndex();
                fIncrementedRefCount = fTrue;
            }

            pfcbTable->LeaveDML();
        }

    }

    //  a null index name indicates switching to primary index
    //
    else if ( NULL == szIndex )
    {
        fSettingToPrimaryIndex = fTrue;
    }

    else
    {
        BOOL    fPrimaryIndexTemplate   = fFalse;

        //  see if we're switching to the derived
        //  primary index (if any)
        if ( pfcbTable->FDerivedTable() )
        {
            Assert( pfcbTable->Ptdb() != ptdbNil );
            Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
            const FCB * const   pfcbTemplateTable   = pfcbTable->Ptdb()->PfcbTemplateTable();

            if ( !pfcbTemplateTable->FSequentialIndex() )
            {
                // If template table has a primary index, we must have inherited it.

                fPrimaryIndexTemplate = fTrue;
                Assert( pfcbTemplateTable->Pidb() != pidbNil );
                Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
                Assert( pfcbTable->Pidb() == pfcbTemplateTable->Pidb() || pfcbTable->Pidb()->FIDBOwnedByFCB() );

                const TDB * const   ptdb            = pfcbTemplateTable->Ptdb();
                const IDB * const   pidb            = pfcbTemplateTable->Pidb();
                const CHAR * const  szPrimaryIdx    = ptdb->SzIndexName( pidb->ItagIndexName() );

                if ( 0 == UtilCmpName( szIndex, szPrimaryIdx ) )
                {
                    fSettingToPrimaryIndex = fTrue;
                }
            }
            else
            {
                Assert( pfcbTemplateTable->Pidb() == pidbNil );
            }
        }

        //  see if we're switching to the primary index
        if ( !fPrimaryIndexTemplate )
        {
            pfcbTable->EnterDML();

            if  ( pfcbTable->Pidb() != pidbNil )
            {
                Assert( pfcbTable->Pidb()->ItagIndexName() != 0 );
                err = ErrFILEIAccessIndexByName( pfucb->ppib, pfcbTable, pfcbTable, szIndex );
                if ( err < 0 )
                {
                    if ( JET_errIndexNotFound != err )
                    {
                        pfcbTable->LeaveDML();
                        return err;
                    }
                }
                else
                {
                    fSettingToPrimaryIndex = fTrue;
                }
            }

            if ( !fSettingToPrimaryIndex
                && pfucbNil != *ppfucbCurIdx
                && !( (*ppfucbCurIdx)->u.pfcb->FDerivedIndex() ) )
            {
                //  don't let go of the DML latch because
                //  we're going to need it below to check
                //  if we're switching to the current
                fInDMLLatch = fTrue;
            }
            else
            {
                pfcbTable->LeaveDML();
            }
        }
    }

    if ( fSettingToPrimaryIndex )
    {
        Assert( !fInDMLLatch );

        if ( *ppfucbCurIdx != pfucbNil )
        {
            Assert( FFUCBIndex( *ppfucbCurIdx ) );
            Assert( FFUCBSecondary( *ppfucbCurIdx ) );
            Assert( (*ppfucbCurIdx)->u.pfcb != pfcbNil );
            Assert( (*ppfucbCurIdx)->u.pfcb->FTypeSecondaryIndex() );
            Assert( (*ppfucbCurIdx)->u.pfcb->Pidb() != pidbNil );
            Assert( (*ppfucbCurIdx)->u.pfcb->Pidb()->ItagIndexName() != 0 );
            Assert( (*ppfucbCurIdx)->u.pfcb->Pidb()->CrefCurrentIndex() > 0
                || (*ppfucbCurIdx)->u.pfcb->Pidb()->FTemplateIndex() );

            //  move the sequential flag back to this index
            //  we are about to close this FUCB so we don't need to reset its flags
            if( FFUCBSequential( *ppfucbCurIdx ) )
            {
                FUCBSetSequential( pfucb );
            }

            //  really changing index, so close old one

            DIRClose( *ppfucbCurIdx );
            *ppfucbCurIdx = pfucbNil;

            //  changing to primary index.  Reset currency to beginning.
            ppfucbCurIdx = &pfucb;
            goto ResetCurrency;
        }

        //  UNDONE: this case should honor grbit move expectations
        return JET_errSuccess;
    }


    Assert( NULL != szIndex || NULL != pindexid );

    //  have a current secondary index
    //
    if ( *ppfucbCurIdx != pfucbNil )
    {
        const FCB * const   pfcbSecondaryIdx    = (*ppfucbCurIdx)->u.pfcb;

        Assert( FFUCBIndex( *ppfucbCurIdx ) );
        Assert( FFUCBSecondary( *ppfucbCurIdx ) );
        Assert( pfcbSecondaryIdx != pfcbNil );
        Assert( pfcbSecondaryIdx->FTypeSecondaryIndex() );
        Assert( pfcbSecondaryIdx->Pidb() != pidbNil );
        Assert( pfcbSecondaryIdx->Pidb()->ItagIndexName() != 0 );
        Assert( pfcbSecondaryIdx->Pidb()->CrefCurrentIndex() > 0
            || pfcbSecondaryIdx->Pidb()->FTemplateIndex() );

        if ( NULL != pindexid )
        {
            //  already verified above that we're not
            //  switching to the current index
            Assert( !fInDMLLatch );
            Assert( pfcbSecondaryIdx != pindexid->pfcbIndex );
        }

        else if ( pfcbSecondaryIdx->FDerivedIndex() )
        {
            Assert( !fInDMLLatch );
            Assert( pfcbSecondaryIdx->Pidb()->FTemplateIndex() || pfcbSecondaryIdx->Pidb()->FIDBOwnedByFCB() );
            Assert( pfcbTable->Ptdb() != ptdbNil );
            Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
            Assert( pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb() != ptdbNil );
            const TDB * const   ptdb        = pfcbTable->Ptdb()->PfcbTemplateTable()->Ptdb();
            const IDB * const   pidb        = pfcbSecondaryIdx->Pidb();
            const CHAR * const  szCurrIdx   = ptdb->SzIndexName( pidb->ItagIndexName() );
            if ( 0 == UtilCmpName( szIndex, szCurrIdx ) )
            {
                //  changing to the current secondary index, so do nothing.
                return JET_errSuccess;
            }
        }
        else
        {
            // See if the desired index is the current one
            if ( !fInDMLLatch )
                pfcbTable->EnterDML();

            const TDB * const   ptdb        = pfcbTable->Ptdb();
            const FCB * const   pfcbIndex   = (*ppfucbCurIdx)->u.pfcb;
            const IDB * const   pidb        = pfcbIndex->Pidb();
            const CHAR * const  szCurrIdx   = ptdb->SzIndexName(
                                                        pidb->ItagIndexName(),
                                                        pfcbIndex->FDerivedIndex() );
            const INT           cmp         = UtilCmpName( szIndex, szCurrIdx );

            pfcbTable->LeaveDML();
            fInDMLLatch = fFalse;

            if ( 0 == cmp )
            {
                //  changing to the current secondary index, so do nothing.
                return JET_errSuccess;
            }
        }


        Assert( !fInDMLLatch );

        //  really changing index, so close old one
        //
        if ( FFUCBVersioned( *ppfucbCurIdx ) )
        {
            //  if versioned, must defer-close cursor
            DIRClose( *ppfucbCurIdx );
        }
        else
        {
            pfucbOldIndex = *ppfucbCurIdx;
            Assert( !PinstFromPfucb( pfucbOldIndex )->FRecovering() );
            Assert( !Pcsr( pfucbOldIndex )->FLatched() );
            Assert( !FFUCBDeferClosed( pfucbOldIndex ) );
            Assert( !FFUCBVersioned( pfucbOldIndex ) );
            Assert( !FFUCBDenyRead( pfucbOldIndex ) );
            Assert( !FFUCBDenyWrite( pfucbOldIndex ) );
            Assert( JET_LSNil == pfucbOldIndex->ls );
            Assert( pfcbNil != pfucbOldIndex->u.pfcb );
            Assert( pfucbOldIndex->u.pfcb->FTypeSecondaryIndex() );
            FILEReleaseCurrentSecondary( pfucbOldIndex );
            pfucbOldIndex->u.pfcb->Unlink( pfucbOldIndex, fTrue );
            KSReset( pfucbOldIndex );
            pfucbOldIndex->bmCurr.Nullify();
        }

        *ppfucbCurIdx = pfucbNil;
        fClosedPreviousIndex = fTrue;
    }

    //  set new current secondary index
    //
    Assert( pfucbNil == *ppfucbCurIdx );
    Assert( !fInDMLLatch );

    if ( NULL != pindexid )
    {
        //  IDB was already pinned above
        pfcbSecondary = pindexid->pfcbIndex;
        Assert( pfcbNil != pfcbSecondary );
        Assert( pfcbSecondary->Pidb()->CrefCurrentIndex() > 0
            || pfcbSecondary->Pidb()->FTemplateIndex() );
    }
    else
    {
        //  verify visibility on desired index and pin it
        pfcbTable->EnterDML();

        for ( pfcbSecondary = pfcbTable->PfcbNextIndex();
            pfcbSecondary != pfcbNil;
            pfcbSecondary = pfcbSecondary->PfcbNextIndex() )
        {
            Assert( pfcbSecondary->Pidb() != pidbNil );
            Assert( pfcbSecondary->Pidb()->ItagIndexName() != 0 );

            err = ErrFILEIAccessIndexByName( pfucb->ppib, pfcbTable, pfcbSecondary, szIndex );
            if ( err < 0 )
            {
                if ( JET_errIndexNotFound != err )
                {
                    pfcbTable->LeaveDML();
                    goto HandleError;
                }
            }
            else
            {
                // Found the index we want.
                break;
            }
        }

        if ( pfcbNil == pfcbSecondary )
        {
            pfcbTable->LeaveDML();
            Call( ErrERRCheck( JET_errIndexNotFound ) );
        }

        else if ( pfcbSecondary->Pidb()->FTemplateIndex() )
        {
            // Don't need refcount on template indexes, since we
            // know they'll never go away.
            Assert( pfcbSecondary->Pidb()->CrefCurrentIndex() == 0 );
        }
        else
        {
            pfcbSecondary->Pidb()->IncrementCurrentIndex();
            fIncrementedRefCount = fTrue;
        }

        pfcbTable->LeaveDML();
    }

    Assert( !pfcbSecondary->FDomainDenyRead( pfucb->ppib ) );
    Assert( pfcbSecondary->FTypeSecondaryIndex() );

    Assert( ( fIncrementedRefCount
            && pfcbSecondary->Pidb()->CrefCurrentIndex() > 0 )
        || ( !fIncrementedRefCount
            && pfcbSecondary->Pidb()->FTemplateIndex()
            && pfcbSecondary->Pidb()->CrefCurrentIndex() == 0 ) );

    //  open an FUCB for the new index
    //
    Assert( pfucbNil == *ppfucbCurIdx );
    Assert( pfucb->ppib != ppibNil );
    Assert( pfucb->ifmp == pfcbSecondary->Ifmp() );
    Assert( !fInDMLLatch );

    if ( pfucbNil != pfucbOldIndex )
    {
        const BOOL  fUpdatable  = FFUCBUpdatable( pfucbOldIndex );

        FUCBResetFlags( pfucbOldIndex );
        FUCBResetPreread( pfucbOldIndex );

        if ( fUpdatable )
        {
            FUCBSetUpdatable( pfucbOldIndex );
        }

        //  initialize CSR in fucb
        //  this allocates page structure
        //
        new( Pcsr( pfucbOldIndex ) ) CSR;

        Call( pfcbSecondary->ErrLink( pfucbOldIndex ) );

        pfucbOldIndex->levelOpen = pfucbOldIndex->ppib->Level();
        pfucbOldIndex->levelReuse = 0;
        DIRInitOpenedCursor( pfucbOldIndex, pfucbOldIndex->ppib->Level() );

        *ppfucbCurIdx = pfucbOldIndex;
        pfucbOldIndex = pfucbNil;

        err = JET_errSuccess;
    }
    else
    {
        err = ErrDIROpen( pfucb->ppib, pfcbSecondary, ppfucbCurIdx );
        if ( err < 0 )
        {
            if ( fIncrementedRefCount )
                pfcbSecondary->Pidb()->DecrementCurrentIndex();
            goto HandleError;
        }
    }

    Assert( !fInDMLLatch );
    Assert( pfucbNil != *ppfucbCurIdx );
    FUCBSetIndex( *ppfucbCurIdx );
    FUCBSetSecondary( *ppfucbCurIdx );
    FUCBSetCurrentSecondary( *ppfucbCurIdx );
    (*ppfucbCurIdx)->pfucbTable = pfucb;

    //  move the sequential flag to this index
    //  we don't want the sequential flag set on the primary index any more
    //  because we don't want to preread while seeking on the bookmarks from
    //  the secondary index
    if( FFUCBSequential( pfucb ) )
    {
        FUCBResetSequential( pfucb );
        FUCBResetPreread( pfucb );
        FUCBSetSequential( *ppfucbCurIdx );
    }

    //  copy the opportune read flag to this index
    if ( FFUCBOpportuneRead( pfucb ) )
    {
        FUCBSetOpportuneRead( *ppfucbCurIdx );
    }

    //  reset the index and file currency
    //
ResetCurrency:
    Assert( !fInDMLLatch );
    DIRBeforeFirst( *ppfucbCurIdx );
    if ( pfucb != *ppfucbCurIdx )
    {
        DIRBeforeFirst( pfucb );
    }

    AssertDIRNoLatch( pfucb->ppib );
    return JET_errSuccess;

HandleError:
    Assert( err < JET_errSuccess );
    Assert( !fInDMLLatch );

    if ( pfucbNil != pfucbOldIndex )
    {
        if ( pfcbNil != pfucbOldIndex->u.pfcb )
        {
            // Managed to link to the FCB before we failed.
            pfucbOldIndex->u.pfcb->Unlink( pfucbOldIndex );
        }

        RECReleaseKeySearchBuffer( pfucbOldIndex );
        BTReleaseBM( pfucbOldIndex );
        FUCBClose( pfucbOldIndex );
    }

    Assert( pfucbNil == pfucb->pfucbCurIndex );
    if ( fClosedPreviousIndex )
    {
        DIRBeforeFirst( pfucb );
    }

    return err;
}


LOCAL ERR ErrRECIIllegalNulls( FUCB * const pfucb, FCB * const pfcb )
{
    Assert( pfucbNil != pfucb );
    Assert( pfcbNil != pfcb );

    ERR                 err             = JET_errSuccess;
    const DATA&         dataRec         = pfucb->dataWorkBuf;
    const TDB * const   ptdb            = pfcb->Ptdb();
    const BOOL          fTemplateTable  = ptdb->FTemplateTable();
    COLUMNID            columnid;

    Assert( !dataRec.FNull() );

    //  check fixed fields
    if ( ptdb->FTableHasNonNullFixedColumn()
        && ptdb->FidFixedLast() >= ptdb->FidFixedFirst() )
    {
        const COLUMNID  columnidFixedLast   = ColumnidOfFid( ptdb->FidFixedLast(), fTemplateTable );
        for ( columnid = ColumnidOfFid( ptdb->FidFixedFirst(), fTemplateTable );
            columnid <= columnidFixedLast;
            columnid++ )
        {
            FIELD   field;

            //  template columns are guaranteed to exist
            if ( fTemplateTable )
            {
                Assert( JET_coltypNil != ptdb->PfieldFixed( columnid )->coltyp );
                field.ffield = ptdb->PfieldFixed( columnid )->ffield;
            }
            else
            {
                Assert( !FCOLUMNIDTemplateColumn( columnid ) );
                err = ErrRECIAccessColumn( pfucb, columnid, &field );
                if ( err < 0 )
                {
                    if ( JET_errColumnNotFound == err  )
                        continue;
                    else
                        return err;
                }

                Assert( JET_coltypNil != field.coltyp );
            }

            if ( FFIELDNotNull( field.ffield ) && !FFIELDDefault( field.ffield ) )
            {
                const ERR   errCheckNull    = ErrRECIFixedColumnInRecord( columnid, pfcb, dataRec );

                if ( JET_wrnColumnNull == errCheckNull
                    || JET_errColumnNotFound == errCheckNull )  // if column not in record, it's null (since there's no default value)
                {
                    return ErrERRCheck( JET_errNullInvalid );
                }

                CallS( errCheckNull );
            }
        }
    }

    CallSx( err, JET_errColumnNotFound );

    //  check var fields
    if ( ptdb->FTableHasNonNullVarColumn()
        && ptdb->FidVarLast() >= ptdb->FidVarFirst() )
    {
        const COLUMNID  columnidVarLast     = ColumnidOfFid( ptdb->FidVarLast(), fTemplateTable );
        for ( columnid = ColumnidOfFid( ptdb->FidVarFirst(), fTemplateTable );
            columnid <= columnidVarLast;
            columnid++ )
        {
            //  template columns are guaranteed to exist
            if ( !fTemplateTable )
            {
                Assert( !FCOLUMNIDTemplateColumn( columnid ) );
                err = ErrRECIAccessColumn( pfucb, columnid );
                if ( err < 0 )
                {
                    if ( JET_errColumnNotFound == err  )
                        continue;
                    else
                        return err;
                }
            }

            if ( columnid > ptdb->FidVarLastInitial() )
                pfcb->EnterDML();

            Assert( JET_coltypNil != ptdb->PfieldVar( columnid )->coltyp );
            const FIELDFLAG     ffield  = ptdb->PfieldVar( columnid )->ffield;

            if ( columnid > ptdb->FidVarLastInitial() )
                pfcb->LeaveDML();

            if ( FFIELDNotNull( ffield ) && !FFIELDDefault( ffield ) )
            {
                const ERR   errCheckNull    = ErrRECIVarColumnInRecord( columnid, pfcb, dataRec );

                if ( JET_wrnColumnNull == errCheckNull
                    || JET_errColumnNotFound == errCheckNull )  // if column not in record, it's null (since there's no default value)
                {
                    return ErrERRCheck( JET_errNullInvalid );
                }

                CallS( errCheckNull );
            }
        }
    }

    CallSx( err, JET_errColumnNotFound );

    //  check tagged columns
    //
    if ( ptdb->FTableHasNonNullTaggedColumn()
        && ptdb->FidTaggedLast() >= ptdb->FidTaggedFirst() )
    {
        const COLUMNID  columnidTaggedLast      = ColumnidOfFid( ptdb->FidTaggedLast(), fTemplateTable );
        for ( columnid = ColumnidOfFid( ptdb->FidTaggedFirst(), fTemplateTable );
            columnid <= columnidTaggedLast;
            columnid++ )
        {
            //  template columns are guaranteed to exist
            if ( !fTemplateTable )
            {
                Assert( !FCOLUMNIDTemplateColumn( columnid ) );
                err = ErrRECIAccessColumn( pfucb, columnid );
                if ( err < 0 )
                {
                    if ( JET_errColumnNotFound == err  )
                        continue;
                    else
                        return err;
                }
            }

            if ( columnid > ptdb->FidTaggedLastInitial() )
                pfcb->EnterDML();

            Assert( JET_coltypNil != ptdb->PfieldTagged( columnid )->coltyp );
            const FIELDFLAG     ffield  = ptdb->PfieldTagged( columnid )->ffield;

            if ( columnid > ptdb->FidTaggedLastInitial() )
                pfcb->LeaveDML();

            if ( FFIELDNotNull( ffield ) && !FFIELDDefault( ffield ) )
            {
                DATA    dataT;
                CallR( ErrRECIRetrieveTaggedColumn(
                            pfucb->u.pfcb,      //  must use FCB of btree owning the record
                            columnid,
                            1,
                            dataRec,
                            &dataT,
                            JET_bitRetrieveIgnoreDefault | grbitRetrieveColumnDDLNotLocked ) );
                if ( JET_wrnColumnNull == err )
                {
                    return ErrERRCheck( JET_errNullInvalid );
                }

                Assert( JET_wrnColumnSetNull != err );  //  only returned during scan mode
            }
        }
    }

    Assert( JET_wrnColumnNull != err );
    Assert( JET_wrnColumnSetNull != err );
    Assert( err >= JET_errSuccess || JET_errColumnNotFound == err );
    return JET_errSuccess;
}


ERR ErrRECIIllegalNulls( FUCB * const pfucb )
{
    ERR         err;
    FCB * const pfcb = pfucb->u.pfcb;

    if ( pfcbNil != pfcb->Ptdb()->PfcbTemplateTable() )
    {
        pfcb->Ptdb()->AssertValidDerivedTable();
        CallR( ErrRECIIllegalNulls( pfucb, pfcb->Ptdb()->PfcbTemplateTable() ) );
    }

    return ErrRECIIllegalNulls( pfucb, pfcb );
}

ERR VTAPI ErrIsamGetCurrentIndex( JET_SESID sesid, JET_VTID vtid, _Out_z_bytecap_(cbMax) PSTR szCurIdx, ULONG cbMax )
{
    PIB     *ppib   = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );
    FCB     *pfcbTable;

    ERR     err = JET_errSuccess;
    CHAR    szIndex[JET_cbNameMost+1];

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    if ( cbMax < 1 )
    {
        return ErrERRCheck( JET_wrnBufferTruncated );
    }

    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbNil != pfcbTable );
    Assert( ptdbNil != pfcbTable->Ptdb() );

    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        Assert( pfucb->pfucbCurIndex->u.pfcb != pfcbNil );
        Assert( pfucb->pfucbCurIndex->u.pfcb->FTypeSecondaryIndex() );
        Assert( !pfucb->pfucbCurIndex->u.pfcb->FDeletePending() );
        Assert( !pfucb->pfucbCurIndex->u.pfcb->FDeleteCommitted() );
        Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb() != pidbNil );
        Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb()->ItagIndexName() != 0 );

        const FCB   * const pfcbIndex = pfucb->pfucbCurIndex->u.pfcb;
        if ( pfcbIndex->FDerivedIndex() )
        {
            Assert( pfcbIndex->Pidb()->FTemplateIndex() || pfcbIndex->Pidb()->FIDBOwnedByFCB() );
            const FCB   * const pfcbTemplateTable = pfcbTable->Ptdb()->PfcbTemplateTable();
            Assert( pfcbNil != pfcbTemplateTable );
            OSStrCbCopyA(
                szIndex,
                sizeof(szIndex),
                pfcbTemplateTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName() ) );
        }
        else
        {
            pfcbTable->EnterDML();
            OSStrCbCopyA(
                szIndex,
                sizeof(szIndex),
                pfcbTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName() ) );
            pfcbTable->LeaveDML();
        }
    }
    else
    {
        BOOL    fPrimaryIndexTemplate = fFalse;

        if ( pfcbTable->FDerivedTable() )
        {
            Assert( pfcbTable->Ptdb() != ptdbNil );
            Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
            const FCB   * const pfcbTemplateTable = pfcbTable->Ptdb()->PfcbTemplateTable();
            if ( !pfcbTemplateTable->FSequentialIndex() )
            {
                // If template table has a primary index, we must have inherited it.
                fPrimaryIndexTemplate = fTrue;
                Assert( pfcbTemplateTable->Pidb() != pidbNil );
                Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
                Assert( pfcbTable->Pidb() == pfcbTemplateTable->Pidb() || pfcbTable->Pidb()->FIDBOwnedByFCB() );
                Assert( pfcbTemplateTable->Pidb()->ItagIndexName() != 0 );
                OSStrCbCopyA(
                    szIndex,
                    sizeof(szIndex),
                    pfcbTemplateTable->Ptdb()->SzIndexName( pfcbTable->Pidb()->ItagIndexName() ) );
            }
            else
            {
                Assert( pfcbTemplateTable->Pidb() == pidbNil );
            }
        }

        if ( !fPrimaryIndexTemplate )
        {
            pfcbTable->EnterDML();
            if ( pfcbTable->Pidb() != pidbNil )
            {
                ERR errT = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbTable );
                if ( errT < 0 )
                {
                    if ( JET_errIndexNotFound != errT )
                    {
                        pfcbTable->LeaveDML();
                        return errT;
                    }
                    szIndex[0] = '\0';  // Primary index not visible - return sequential index
                }
                else
                {
                    Assert( pfcbTable->Pidb()->ItagIndexName() != 0 );
                    OSStrCbCopyA(
                        szIndex, sizeof(szIndex),
                        pfcbTable->Ptdb()->SzIndexName( pfcbTable->Pidb()->ItagIndexName() ) );
                }
            }
            else
            {
                szIndex[0] = '\0';
            }
            pfcbTable->LeaveDML();
        }
    }


    if ( cbMax > JET_cbNameMost + 1 )
        cbMax = JET_cbNameMost + 1;
    OSStrCbCopyA( szCurIdx, cbMax, szIndex);
    CallS( err );
    AssertDIRNoLatch( ppib );
    return err;
}


ERR VTAPI ErrIsamGetChecksum( JET_SESID sesid, JET_VTID vtid, ULONG *pulChecksum )
{
    PIB *ppib   = reinterpret_cast<PIB *>( sesid );
    FUCB *pfucb = reinterpret_cast<FUCB *>( vtid );

    ERR     err = JET_errSuccess;

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckFUCB( ppib, pfucb );
    Call( ErrDIRGet( pfucb ) );
    *pulChecksum = UlChecksum( pfucb->kdfCurr.data.Pv(), pfucb->kdfCurr.data.Cb() );

    CallS( ErrDIRRelease( pfucb ) );

HandleError:
    AssertDIRNoLatch( ppib );
    return err;
}


ULONG UlChecksum( VOID *pv, ULONG cb )
{
    //  UNDONE: find a way to compute check sum in longs independant
    //              of pb, byte offset in page

    /*  compute checksum by adding bytes in data record and shifting
    /*  result 1 bit to left after each operation.
    /**/
    BYTE    *pbT = (BYTE *) pv;
    BYTE    *pbMax = pbT + cb;
    ULONG   ulChecksum = 0;

    /*  compute checksum
    /**/
    for ( ; pbT < pbMax; pbT++ )
    {
        ulChecksum += *pbT;
        ulChecksum <<= 1;
    }

    return ulChecksum;
}

