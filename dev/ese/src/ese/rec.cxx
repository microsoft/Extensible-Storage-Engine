// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "PageSizeClean.hxx"

const WORD ibRECStartFixedColumns   = REC::cbRecordMin;

static const JET_COLUMNDEF rgcolumndefJoinlist[] =
{
    { sizeof(JET_COLUMNDEF), 0, JET_coltypBinary, 0, 0, 0, 0, 0, JET_bitColumnTTKey },
};




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
    JET_COLTYP  coltyp;
    BOOL        fNormalized;
};

struct CURSOR_FILTER_CONTEXT : public MOVE_FILTER_CONTEXT
{
    NORMALIZED_FILTER_COLUMN *  rgFilters;
    DWORD                       cFilters;
};

LOCAL ERR ErrRECICheckCursorFilter( FUCB * const pfucb, CURSOR_FILTER_CONTEXT * const pcursorFilterContext )
{
    ERR     err         = JET_errSuccess;
    BOOL    fMatch      = fTrue;
    DATA    dataField;

    Call( ErrRECCheckMoveFilter( pfucb, (MOVE_FILTER_CONTEXT * const)pcursorFilterContext ) );
    if ( err > JET_errSuccess )
    {
        goto HandleError;
    }

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
                1,
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
                NULL  ) );
        }

        CallSx( err, JET_wrnColumnNull );
        if ( JET_errSuccess == err )
        {
            const BYTE * const  pvFilter    = (BYTE *)pFilter->pv;
            const INT           cbFilter    = pFilter->cb;
            BYTE                *pvData;
            INT                 cbData;
            BYTE                rgbNormData[ sizeof(GUID) + 1 ];

            if ( pFilter->fNormalized )
            {
                Assert( !FRECLongValue( pFilter->coltyp ) );
                Assert( !FRECTextColumn( pFilter->coltyp ) );
                Assert( !FRECBinaryColumn( pFilter->coltyp ) );
                Assert( pFilter->cb > 0 );

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
                    AssertSz( fFalse, "Unrecognized relop for non-null column." );
                    fMatch = fTrue;
            }
        }
        else if ( JET_wrnColumnNull == err )
        {
            switch ( pFilter->relop )
            {
                case JET_relopEquals:
                    fMatch = fFilterNull;
                    break;
                case JET_relopPrefixEquals:
                    fMatch = fFalse;
                    break;
                case JET_relopNotEquals:
                    fMatch = !fFilterNull;
                    break;
                case JET_relopLessThanOrEqual:
                    fMatch = fTrue;
                    break;
                case JET_relopLessThan:
                    fMatch = !fFilterNull;
                    break;
                case JET_relopGreaterThanOrEqual:
                    fMatch = fFilterNull;
                    break;
                case JET_relopGreaterThan:
                    fMatch = fFalse;
                    break;
                case JET_relopBitmaskEqualsZero:
                    fMatch = fTrue;
                    break;
                case JET_relopBitmaskNotEqualsZero:
                    fMatch = fFalse;
                    break;
                default:
                    AssertSz( fFalse, "Unrecognized relop for null column." );
                    fMatch = fTrue;
            }
        }
        else
        {
            Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
        }

        if ( !fMatch )
        {
            break;
        }
    }

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

    Call( ErrRECIAccessColumn( pfucb, columnid ) );

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        fUseDMLLatch = fFalse;
    }
    else if ( FFixedFid( fid ) )
    {
        fUseDMLLatch = ( fid > ptdb->FidFixedLastInitial() );
    }
    else if ( FVarFid( fid ) )
    {
        fUseDMLLatch = ( fid > ptdb->FidVarLastInitial() );
    }
    else
    {
        Assert( FTaggedFid( fid ) );
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
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }
    else if ( ( pFilter->relop == JET_relopPrefixEquals ||
                pFilter->grbit & JET_bitZeroLength ) &&
              coltyp != JET_coltypBinary )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    else if ( pFilter->cb == 0 )
    {
        if ( pFilter->relop == JET_relopBitmaskEqualsZero ||
            pFilter->relop == JET_relopBitmaskNotEqualsZero ||
            pFilter->relop == JET_relopPrefixEquals )
        {
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }
    else if ( coltyp != JET_coltypBinary && pFilter->cb != UlCATColumnSize( coltyp, 0, NULL ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    else if ( ( coltyp == JET_coltypBit ||
                coltyp == JET_coltypGUID ||
                coltyp == JET_coltypBinary ) &&
              ( pFilter->relop == JET_relopBitmaskEqualsZero ||
                pFilter->relop == JET_relopBitmaskNotEqualsZero ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

HandleError:
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

    BOOL                fDotNetGuid = fFalse;
    if ( pfucb->pfucbCurIndex != NULL )
    {
        fDotNetGuid = pfucb->pfucbCurIndex->u.pfcb->Pidb()->FDotNetGuid();
    }
    else if ( pfucb->u.pfcb->Pidb() != NULL )
    {
        fDotNetGuid = pfucb->u.pfcb->Pidb()->FDotNetGuid();
    }

    RECRemoveCursorFilter( pfucb );

    if ( cFilters == 0 )
    {
        return JET_errSuccess;
    }

    DWORD   cbNeeded    = sizeof(NORMALIZED_FILTER_COLUMN) * cFilters;
    for ( DWORD i = 0; i < cFilters; i++ )
    {
        cbNeeded += rgFilters[i].cb + 1;
    }

    Alloc( pcursorFilterContext = new CURSOR_FILTER_CONTEXT() );
    Alloc( pcursorFilterContext->rgFilters = (NORMALIZED_FILTER_COLUMN *)PvOSMemoryHeapAlloc( cbNeeded ) );

    BYTE    *pBuffer    = (BYTE *)( pcursorFilterContext->rgFilters + cFilters);
    for ( DWORD i = 0; i < cFilters; i++ )
    {
        const JET_INDEX_COLUMN * const  pFilter = rgFilters + i;

        Call( ErrRECIValidateOneMoveFilter( pfucb, pFilter, &coltyp ) );

        memcpy( &pcursorFilterContext->rgFilters[i], pFilter, sizeof(JET_INDEX_COLUMN) );

        pcursorFilterContext->rgFilters[i].coltyp = coltyp;
        pcursorFilterContext->rgFilters[i].fNormalized = fFalse;

        if ( coltyp != JET_coltypBinary && pFilter->cb != 0 )
        {
            if ( coltyp == JET_coltypBit ||
                pFilter->relop == JET_relopLessThanOrEqual ||
                pFilter->relop == JET_relopLessThan ||
                pFilter->relop == JET_relopGreaterThanOrEqual ||
                pFilter->relop == JET_relopGreaterThan )
            {
                pcursorFilterContext->rgFilters[i].fNormalized = fTrue;
            }
        }

        if ( pcursorFilterContext->rgFilters[i].fNormalized )
        {
            INT cbNormalizedValue   = 0;

            Assert( !FRECLongValue( coltyp ) );
            Assert( !FRECTextColumn( coltyp ) );
            Assert( !FRECBinaryColumn( coltyp ) );
            Assert( pFilter->cb > 0 );

            FLDNormalizeFixedSegment(
                (BYTE *)pFilter->pv,
                pFilter->cb,
                pBuffer,
                &cbNormalizedValue,
                coltyp,
                fDotNetGuid,
                fTrue );
            Assert( cbNormalizedValue <= sizeof(GUID) + 1 );
            pcursorFilterContext->rgFilters[i].pv = pBuffer;
            pcursorFilterContext->rgFilters[i].cb = cbNormalizedValue;
            pBuffer += cbNormalizedValue;
        }

        else if ( rgFilters[i].cb != 0 )
        {
            memcpy( pBuffer, rgFilters[i].pv, rgFilters[i].cb );
            pcursorFilterContext->rgFilters[i].pv = pBuffer;
            Assert( pcursorFilterContext->rgFilters[i].cb == rgFilters[i].cb );
            pBuffer += rgFilters[i].cb;
        }
        else
        {
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



ERR VTAPI ErrIsamMove( JET_SESID sesid, JET_VTID vtid, LONG crow, JET_GRBIT grbit )
{
    PIB     *ppib = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );
    ERR     err = JET_errSuccess;
    FUCB    *pfucbSecondary;
    FUCB    *pfucbIdx;
    DIB     dib;

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

    pfucbSecondary = pfucb->pfucbCurIndex;
    if ( pfucbSecondary == pfucbNil )
        pfucbIdx = pfucb;
    else
        pfucbIdx = pfucbSecondary;

    if ( crow == JET_MoveLast )
    {
        DIRResetIndexRange( pfucb );

        dib.pos = posLast;

        DIRGotoRoot( pfucbIdx );

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

        DIRGotoRoot( pfucbIdx );

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
                Call( ErrDIRRelease( pfucbIdx ) );
                Call( ErrDIRGet( pfucbIdx ) );
                Assert( Pcsr( pfucbIdx )->FLatched() );
            }
        }
    }

    if ( err == JET_errSuccess && pfucbSecondary != pfucbNil )
    {
        BOOKMARK    bmRecord;

        Assert( pfucbSecondary->kdfCurr.data.Pv() != NULL );
        Assert( pfucbSecondary->kdfCurr.data.Cb() > 0 );
        Assert( Pcsr( pfucbIdx )->FLatched() );

        bmRecord.key.prefix.Nullify();
        bmRecord.key.suffix = pfucbSecondary->kdfCurr.data;
        bmRecord.data.Nullify();


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

        while ( crowT-- > 0 )
        {
            if ( grbit & JET_bitMoveKeyNE )
            {
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








ERR VTAPI ErrIsamSeek( JET_SESID sesid, JET_VTID vtid, JET_GRBIT grbit )
{
    PIB         *ppib           = reinterpret_cast<PIB *>( sesid );
    FUCB        *pfucbTable     = reinterpret_cast<FUCB *>( vtid );

    ERR         err;
    BOOKMARK    bm;
    DIB         dib;
    FUCB        *pfucbSeek;
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

    if ( FFUCBUpdatePrepared( pfucbTable ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucbTable, JET_prepCancel ) );
    }

    DIRResetIndexRange( pfucbTable );

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
            && bm.key.suffix.Cb() < KEY::CbLimitKeyMost( pidb->CbKeyMost() ) )
        {
            Assert( pfcb->FTypeSecondaryIndex() );
            Assert( pidb->Cidxseg() > 0 );
            Assert( pfucbSeek->cColumnsInSearchKey <= pidb->Cidxseg() );
            if ( pfucbSeek->cColumnsInSearchKey == pidb->Cidxseg() )
            {
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
    if ( 0 == ppib->Level() )
    {
        Call( ErrDIRBeginTransaction( ppib, 60197, JET_bitTransactionReadOnly ) );
        fTransactionStarted = fTrue;
    }
#endif

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
            Assert( FFUCBSecondary( pfucbSeek ) );
            Assert( pfucbSeek == pfucbTable->pfucbCurIndex );

            BOOKMARK    bmRecord;

            Assert(pfucbSeek->kdfCurr.data.Pv() != NULL);
            Assert(pfucbSeek->kdfCurr.data.Cb() > 0 );

            bmRecord.key.prefix.Nullify();
            bmRecord.key.suffix = pfucbSeek->kdfCurr.data;
            bmRecord.data.Nullify();


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

                if ( fFoundUniqueKey )
                {
                    err = ErrERRCheck( JET_wrnUniqueKey );
                }
                else if ( grbit & JET_bitSetIndexRange )
                {
#ifdef TRANSACTED_SEEK
#else
#endif
                    CallR( ErrIsamSetIndexRange( ppib, pfucbTable, JET_bitRangeInclusive | JET_bitRangeUpperLimit ) );
                }

                goto Release;
                break;

            case JET_bitSeekGE:
            case JET_bitSeekLE:
                CallS( err );
                goto Release;
                break;

            case JET_bitSeekGT:
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
                    if ( pfucbTable != pfucbSeek )
                    {
                        Assert( !Pcsr( pfucbTable )->FLatched() );
                        pfucbTable->locLogical = locOnSeekBM;
                    }
                    err = ErrERRCheck( JET_errRecordNotFound );
                    goto Release;
                }

                Assert( pfucbSeek->u.pfcb->FTypeSecondaryIndex() );


            case JET_bitSeekGE:
            case JET_bitSeekGT:
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
                    Assert( pfucbSeek == pfucbTable->pfucbCurIndex );
                    Assert( Pcsr( pfucbSeek )->FLatched() );
                    const INT   cmp = CmpKey( pfucbSeek->kdfCurr.key, bm.key );

#ifdef TRANSACTED_SEEK
                    Assert( cmp >= 0 );
#else
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
                        if ( grbit & JET_bitSeekGE )
                            err = ErrERRCheck( JET_wrnSeekNotEqual );
                    }
                    else
                    {
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
#endif

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


LOCAL ERR ErrRECICheckIndexrangesForUniqueness(
            const JET_SESID sesid,
            const JET_INDEXRANGE * const rgindexrange,
            const ULONG cindexrange )
{
    PIB * const ppib        = reinterpret_cast<PIB *>( sesid );

    for( SIZE_T itableid = 0; itableid < cindexrange; ++itableid )
    {
        if( sizeof( JET_INDEXRANGE ) != rgindexrange[itableid].cbStruct )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }

        const FUCB * const pfucb    = reinterpret_cast<FUCB *>( rgindexrange[itableid].tableid );
        CheckTable( ppib, pfucb );
        CheckSecondary( pfucb );


        if( !pfucb->pfucbCurIndex )
        {
            AssertSz( fFalse, "Don't do a join on the primary index!" );
            return ErrERRCheck( JET_errInvalidParameter );
        }


        if( JET_bitRecordInIndex != rgindexrange[itableid].grbit
            )
        {
            AssertSz( fFalse, "Invalid grbit in JET_INDEXRANGE" );
            return ErrERRCheck( JET_errInvalidGrbit );
        }


        for( SIZE_T itableidT = 0; itableidT < cindexrange; ++itableidT )
        {
            if( itableidT == itableid )
            {
                continue;
            }

            const FUCB * const pfucbT   = reinterpret_cast<FUCB *>( rgindexrange[itableidT].tableid );


            if( !pfucbT->pfucbCurIndex )
            {
                AssertSz( fFalse, "Don't do a join on the primary index!" );
                return ErrERRCheck( JET_errInvalidParameter );
            }


            if( pfucbT->u.pfcb != pfucb->u.pfcb )
            {
                AssertSz( fFalse, "Indexes are not on the same table" );
                return ErrERRCheck( JET_errInvalidParameter );
            }


            if( pfucb->pfucbCurIndex->u.pfcb == pfucbT->pfucbCurIndex->u.pfcb )
            {
                AssertSz( fFalse, "Indexes are the same" );
                return ErrERRCheck( JET_errInvalidParameter );
            }
        }
    }
    return JET_errSuccess;
}


LOCAL ERR ErrRECIInsertBookmarksIntoSort(
    FUCB * const        pfucb,
    FUCB * const        pfucbSort,
    const JET_GRBIT     grbit )
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


LOCAL ERR ErrRECIJoinFindDuplicates(
    JET_SESID                       sesid,
    _In_count_( cindexes ) const JET_INDEXRANGE * const rgindexrange,
    _In_count_( cindexes ) FUCB *   rgpfucbSort[],
    _In_range_( 1, 64 ) const ULONG cindexes,
    _Out_ JET_RECORDLIST * const            precordlist,
    JET_GRBIT                       grbit )
{
    ERR                             err;
    const INST * const              pinst       = PinstFromPpib( (PIB *)sesid );
    SIZE_T                          isort;
    BYTE                            rgfSortIsMin[64];

    if ( _countof(rgfSortIsMin) < cindexes )
    {
        AssertSz( false, "We have a problem." );
        return( ErrERRCheck(JET_wrnNyi) );
    }


    Assert( cindexes > 0 );
    const LONG cbPage = CbAssertGlobalPageSizeMatchRTL( g_rgfmp[ rgpfucbSort[ 0 ]->ifmp ].CbPage() );
    Assert( cbPage != 0 );
    for( isort = 0; isort < cindexes; ++isort )
    {
        err = ErrSORTNext( rgpfucbSort[isort] );
        if( JET_errNoCurrentRecord == err )
        {
            err = JET_errSuccess;
            return err;
        }
        Call( err );
        Expected( cbPage == g_rgfmp[ rgpfucbSort[ isort ]->ifmp ].CbPage() );
    }

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


    while( 1 )
    {
        SIZE_T  isortMin    = 0;

        Call( pinst->ErrCheckForTermination() );


        for( isort = 1; isort < cindexes; ++isort )
        {
            const INT cmp = CmpKey( rgpfucbSort[isortMin]->kdfCurr.key, rgpfucbSort[isort]->kdfCurr.key );
            if( 0 == cmp )
            {
            }
            else if( cmp < 0 )
            {
            }
            else
            {
                Assert( cmp > 0 );
                isortMin = isort;
            }
        }


        BOOL    fDuplicate  = fTrue;
        memset( rgfSortIsMin, 0, sizeof( rgfSortIsMin ) );

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


ERR ErrIsamIntersectIndexes(
    const JET_SESID sesid,
    _In_count_( cindexrange ) const JET_INDEXRANGE * const rgindexrange,
    _In_range_( 1, 64 ) const ULONG cindexrange,
    _Out_ JET_RECORDLIST * const precordlist,
    const JET_GRBIT grbit )
{
    PIB * const ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB *      rgpfucbSort[64];
    SIZE_T      ipfucb;
    ERR         err;

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

    CallR( ErrRECICheckIndexrangesForUniqueness( sesid, rgindexrange, cindexrange ) );

    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        Assert( ipfucb < sizeof(rgpfucbSort)/sizeof(rgpfucbSort[0]) );
        rgpfucbSort[ipfucb] = pfucbNil;
    }

    precordlist->tableid    = JET_tableidNil;
    precordlist->cRecord    = 0;
    precordlist->columnidBookmark = 0;

    Call( ErrPIBOpenTempDatabase( ppib ) );
    
    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        Call( ErrSORTOpen( ppib, rgpfucbSort + ipfucb, fTrue, fTrue ) );
    }

    for( ipfucb = 0; ipfucb < cindexrange; ++ipfucb )
    {
        FUCB * const pfucb  = reinterpret_cast<FUCB *>( rgindexrange[ipfucb].tableid );
        Call( ErrRECIInsertBookmarksIntoSort( pfucb->pfucbCurIndex, rgpfucbSort[ipfucb], grbit ) );
        Call( ErrSORTEndInsert( rgpfucbSort[ipfucb] ) );
    }

    Call( ErrRECIJoinFindDuplicates(
            sesid,
            rgindexrange,
            rgpfucbSort,
            cindexrange,
            precordlist,
            grbit ) );

HandleError:
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


LOCAL ERR ErrIsamValidatePrereadKeysArguments(
    const PIB * const                               ppib,
    const FUCB * const                              pfucb,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __in_opt const LONG * const                     pckeysPreread,
    const JET_GRBIT                                 grbit )
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

    const FCB * const pfcbTable = pfucb->u.pfcb;
    const IDB * const pidb = pfcbTable->Pidb();
    const ULONG cbKeyMost = pidb ? pidb->CbKeyMost() : sizeof(DBK);

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



ERR VTAPI ErrIsamPrereadKeys(
    const JET_SESID                                 sesid,
    const JET_VTID                                  vtid,
    __in_ecount(ckeys) const void * const * const   rgpvKeys,
    __in_ecount(ckeys) const ULONG * const  rgcbKeys,
    const LONG                                      ckeys,
    __out_opt LONG * const                          pckeysPreread,
    const JET_GRBIT                                 grbit )
{
    ERR err = JET_errSuccess;

    PIB * const ppib = reinterpret_cast<PIB *>( sesid );
    FUCB * pfucb = reinterpret_cast<FUCB *>( vtid );

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
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else if ( cmp1 > 0 && cmp2 > 0 )
            {
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

    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
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
        
            for( iindexrangeT = 0; ( iindexrangeT < cindexrangesPreread ) && ( ilidMac < _countof( rglid ) ); iindexrangeT++ )
            {
                Call( ErrIsamMakeKey( ppib, pfucbT, fForward ? pStartKeys[iindexrangeT] : pEndKeys[iindexrangeT], fForward ? startKeyLengths[iindexrangeT] : endKeyLengths[iindexrangeT], JET_bitNewKey|JET_bitNormalizedKey ) );
                err = ErrIsamSeek( ppib, pfucbT, JET_bitSeekGE );
                Assert( JET_errNoCurrentRecord != err );
                if ( JET_errRecordNotFound == err )
                {
                    continue;
                }
                Call( err );

                Call( ErrIsamMakeKey( ppib, pfucbT, fForward ? pEndKeys[iindexrangeT] : pStartKeys[iindexrangeT], fForward ? endKeyLengths[iindexrangeT] : startKeyLengths[iindexrangeT], JET_bitNewKey|JET_bitNormalizedKey ) );
                err = ErrIsamSetIndexRange ( ppib, pfucbT, JET_bitRangeUpperLimit |JET_bitRangeInclusive );
                if ( JET_errNoCurrentRecord == err )
                {
                    continue;
                }
                Call( err );
                
                while ( ilidMac < _countof( rglid ) )
                {
                    for ( icolumnidT = 0; ( icolumnidT < ccolumnidPreread ) && ( ilidMac < _countof( rglid ) ); icolumnidT++ )
                    {
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
            

            if ( ilidMac > 0 )
            {
                LONG clidT;
                ULONG cPageT;
                Call( ErrLVPrereadLongValues( pfucb, rglid, ilidMac, 0, ulMax, &clidT, &cPageT, grbit ) );
            }

            if ( JET_errNoCurrentRecord == err || JET_errRecordNotFound == err )
            {
                err = JET_errSuccess;
            }
        }
    }

    if( pcRangesPreread )
    {
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

ERR VTAPI ErrIsamPrereadIndexRanges(
    const JET_SESID                                             sesid,
    const JET_VTID                                              vtid,
    __in_ecount(cIndexRanges) const JET_INDEX_RANGE * const     rgIndexRanges,
    __in const ULONG                                    cIndexRanges,
    __out_opt ULONG * const                             pcRangesPreread,
    __in_ecount(ccolumnidPreread) const JET_COLUMNID * const    rgcolumnidPreread,
    const ULONG                                                 ccolumnidPreread,
    const JET_GRBIT                                             grbit )
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

ERR VTAPI ErrIsamPrereadIndexRange(
    _In_  JET_SESID                     sesid,
    _In_  JET_TABLEID                   vtid,
    _In_  const JET_INDEX_RANGE * const pIndexRange,
    _In_  const ULONG           cPageCacheMin,
    _In_  const ULONG           cPageCacheMax,
    _In_  const JET_GRBIT               grbit,
    _Out_opt_ ULONG * const     pcPageCacheActual )
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
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

    DIRResetIndexRange( pfucb );

    KSReset( pfucb );

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( const_cast<VOID *>( pvBookmark ) );
    bm.key.suffix.SetCb( cbBookmark );
    bm.data.Nullify();

    Call( ErrDIRGotoJetBookmark( pfucb, bm, fFalse ) );
    AssertDIRNoLatch( ppib );

    Assert( pfucb->u.pfcb->FPrimaryIndex() );
    Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );

    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
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

        if ( NULL == pfucbIdx->dataSearchKey.Pv() )
        {
            pfucbIdx->dataSearchKey.SetPv( RESKEY.PvRESAlloc() );
            if ( NULL == pfucbIdx->dataSearchKey.Pv() )
                return ErrERRCheck( JET_errOutOfMemory );
            pfucbIdx->dataSearchKey.SetCb( cbKeyAlloc );
        }

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

        if ( pidb->FNoNullSeg() )
        {
            Assert( wrnFLDNullSeg != err );
            Assert( wrnFLDNullFirstSeg != err );
            Assert( wrnFLDNullKey != err );
        }

        Assert( err > 0 || JET_errSuccess == err );
        if ( err > 0
            && ( ( wrnFLDNullKey == err && !pidb->FAllowAllNulls() )
                || ( wrnFLDNullFirstSeg == err && !pidb->FAllowFirstNull() )
                || ( wrnFLDNullSeg == err && !pidb->FAllowSomeNulls() ) )
                || wrnFLDOutOfTuples == err
                || wrnFLDNotPresentInIndex == err )
        {
            DIRBeforeFirst( pfucbIdx );
            err = ErrERRCheck( JET_errNoCurrentRecord );
        }
        else
        {
            DIRGotoRoot( pfucbIdx );

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
        return ErrERRCheck( JET_errInvalidBookmark );
    }

    bm.key.prefix.Nullify();
    bm.key.suffix.SetPv( const_cast<VOID *>( pvSecondaryKey ) );
    bm.key.suffix.SetCb( cbSecondaryKey );

    if ( FFUCBUnique( pfucbIdx ) )
    {
        bm.data.Nullify();
    }
    else
    {
        if ( 0 == cbPrimaryBookmark || NULL == pvPrimaryBookmark )
            return ErrERRCheck( JET_errInvalidBookmark );

        bm.data.SetPv( const_cast<VOID *>( pvPrimaryBookmark ) );
        bm.data.SetCb( cbPrimaryBookmark );
    }


    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

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

    err = ErrDIRGotoJetBookmark( pfucbIdx, bm, fTrue );

    if ( JET_errRecordDeleted == err
        && ( grbit & JET_bitBookmarkPermitVirtualCurrency ) )
    {
        Assert( !Pcsr( pfucbIdx )->FLatched() );
        Call( ErrBTDeferGotoBookmark( pfucbIdx, bm, fFalse ) );
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

        Assert( bm.key.prefix.FNull() );
        bm.key.suffix = pfucbIdx->kdfCurr.data;
        bm.data.Nullify();

        const ERR   errGotoBM   = ErrBTDeferGotoBookmark( pfucb, bm, fTrue );
        CallSx( errGotoBM, JET_errOutOfMemory );

        err = ErrBTRelease( pfucbIdx );
        CallSx( err, JET_errOutOfMemory );

        if ( err < 0 || errGotoBM < 0 )
        {
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

    Assert( locOnCurBM == pfucbIdx->locLogical
        || locOnSeekBM == pfucbIdx->locLogical );
    pfucb->locLogical = pfucbIdx->locLogical;

    if ( locOnSeekBM == pfucbIdx->locLogical )
    {
        Call( ErrERRCheck( JET_errRecordDeleted ) );
    }

    else if ( FFUCBUnique( pfucbIdx ) && 0 != cbPrimaryBookmark )
    {
        if ( (ULONG)pfucb->bmCurr.key.suffix.Cb() != cbPrimaryBookmark
            || 0 != memcmp( pfucb->bmCurr.key.suffix.Pv(), pvPrimaryBookmark, cbPrimaryBookmark ) )
        {
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

    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
    }

    DIRResetIndexRange( pfucb );

    KSReset( pfucb );

    pfucbSecondary = pfucb->pfucbCurIndex;

    if ( pfucbSecondary == pfucbNil )
    {
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

        DIRGotoRoot( pfucbSecondary );

        err = ErrDIRGotoPosition( pfucbSecondary, precpos->centriesLT, precpos->centriesTotal );

        if ( JET_errSuccess == err )
        {
            BOOKMARK    bmRecord;

            Assert(pfucbSecondary->kdfCurr.data.Pv() != NULL);
            Assert(pfucbSecondary->kdfCurr.data.Cb() > 0 );

            bmRecord.key.prefix.Nullify();
            bmRecord.key.suffix = pfucbSecondary->kdfCurr.data;
            bmRecord.data.Nullify();


            errT = ErrDIRGotoBookmark( pfucb, bmRecord );

            Assert( PgnoFDP( pfucb ) != pgnoSystemRoot );
            Assert( pfucb->u.pfcb->FPrimaryIndex() );
        }

        if ( err >= 0 )
        {
            err = ErrDIRRelease( pfucbSecondary );

            if ( errT < 0 && err >= 0 )
            {
                err = errT;
            }
        }
        else
        {
            AssertDIRNoLatch( ppib );
        }
    }

    AssertDIRNoLatch( ppib );

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

    
    AssertDIRNoLatch( ppib );

    
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

    
    if ( !( FKSPrepared( pfucb ) ) )
    {
        Call( ErrERRCheck( JET_errKeyNotMade ) );
    }

    FUCBAssertValidSearchKey( pfucb );

    
    DIRSetIndexRange( pfucb, grbit );
    err = ErrDIRCheckIndexRange( pfucb );

    
    KSReset( pfucb );

    
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
    const JET_INDEXID   *pindexid,
    const JET_GRBIT     grbit,
    const ULONG         itagSequence )
{
    ERR                 err;
    PIB                 *ppib           = (PIB *)vsesid;
    FUCB                *pfucb          = (FUCB *)vtid;
    CHAR                *szIndex;
    CHAR                szIndexNameBuf[ (JET_cbNameMost + 1) ];

    if ( pfucb->pmoveFilterContext )
    {
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    Assert( JET_bitMoveFirst == grbit || JET_bitNoMove == grbit );

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
        if ( FFUCBUpdatePrepared( pfucb ) )
        {
            CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
        }

        Call( ErrRECSetCurrentIndex( pfucb, szIndex, (INDEXID *)pindexid ) );
        AssertDIRNoLatch( ppib );

        if( pfucb->u.pfcb->FPreread() && pfucbNil != pfucb->pfucbCurIndex )
        {
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

        BOOKMARK    *pbm;

        Call( ErrDIRGetBookmark( pfucb, &pbm ) );
        AssertDIRNoLatch( ppib );

        Call( ErrRECSetCurrentIndex( pfucb, szIndex, (INDEXID *)pindexid ) );

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
            return ( pfcbIndex->ObjidFDP() != pindexid->objidFDP
                    || pfcbIndex->PgnoFDP() != pindexid->pgnoFDP ?
                            ErrERRCheck( JET_errInvalidIndexId ) :
                            JET_errSuccess );

        }

        else if ( !pfcbIndex->FValid() )
        {
            return ErrERRCheck( JET_errInvalidIndexId );
        }

        else
        {
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

                return ( JET_errIndexNotFound == err ?
                                ErrERRCheck( JET_errInvalidIndexId ) :
                                err );
            }

            else if ( pfcbIndex->Pidb()->FTemplateIndex() )
            {
                Assert( pfcbIndex->Pidb()->CrefCurrentIndex() == 0 );
            }

            else
            {
                pfcbIndex->Pidb()->IncrementCurrentIndex();
                fIncrementedRefCount = fTrue;
            }

            pfcbTable->LeaveDML();
        }

    }

    else if ( NULL == szIndex )
    {
        fSettingToPrimaryIndex = fTrue;
    }

    else
    {
        BOOL    fPrimaryIndexTemplate   = fFalse;

        if ( pfcbTable->FDerivedTable() )
        {
            Assert( pfcbTable->Ptdb() != ptdbNil );
            Assert( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil );
            const FCB * const   pfcbTemplateTable   = pfcbTable->Ptdb()->PfcbTemplateTable();

            if ( !pfcbTemplateTable->FSequentialIndex() )
            {

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

            if( FFUCBSequential( *ppfucbCurIdx ) )
            {
                FUCBSetSequential( pfucb );
            }


            DIRClose( *ppfucbCurIdx );
            *ppfucbCurIdx = pfucbNil;

            ppfucbCurIdx = &pfucb;
            goto ResetCurrency;
        }

        return JET_errSuccess;
    }


    Assert( NULL != szIndex || NULL != pindexid );

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
                return JET_errSuccess;
            }
        }
        else
        {
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
                return JET_errSuccess;
            }
        }


        Assert( !fInDMLLatch );

        if ( FFUCBVersioned( *ppfucbCurIdx ) )
        {
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

    Assert( pfucbNil == *ppfucbCurIdx );
    Assert( !fInDMLLatch );

    if ( NULL != pindexid )
    {
        pfcbSecondary = pindexid->pfcbIndex;
        Assert( pfcbNil != pfcbSecondary );
        Assert( pfcbSecondary->Pidb()->CrefCurrentIndex() > 0
            || pfcbSecondary->Pidb()->FTemplateIndex() );
    }
    else
    {
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

    if( FFUCBSequential( pfucb ) )
    {
        FUCBResetSequential( pfucb );
        FUCBResetPreread( pfucb );
        FUCBSetSequential( *ppfucbCurIdx );
    }

    if ( FFUCBOpportuneRead( pfucb ) )
    {
        FUCBSetOpportuneRead( *ppfucbCurIdx );
    }

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

    if ( ptdb->FTableHasNonNullFixedColumn()
        && ptdb->FidFixedLast() >= ptdb->FidFixedFirst() )
    {
        const COLUMNID  columnidFixedLast   = ColumnidOfFid( ptdb->FidFixedLast(), fTemplateTable );
        for ( columnid = ColumnidOfFid( ptdb->FidFixedFirst(), fTemplateTable );
            columnid <= columnidFixedLast;
            columnid++ )
        {
            FIELD   field;

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
                    || JET_errColumnNotFound == errCheckNull )
                {
                    return ErrERRCheck( JET_errNullInvalid );
                }

                CallS( errCheckNull );
            }
        }
    }

    CallSx( err, JET_errColumnNotFound );

    if ( ptdb->FTableHasNonNullVarColumn()
        && ptdb->FidVarLast() >= ptdb->FidVarFirst() )
    {
        const COLUMNID  columnidVarLast     = ColumnidOfFid( ptdb->FidVarLast(), fTemplateTable );
        for ( columnid = ColumnidOfFid( ptdb->FidVarFirst(), fTemplateTable );
            columnid <= columnidVarLast;
            columnid++ )
        {
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
                    || JET_errColumnNotFound == errCheckNull )
                {
                    return ErrERRCheck( JET_errNullInvalid );
                }

                CallS( errCheckNull );
            }
        }
    }

    CallSx( err, JET_errColumnNotFound );

    if ( ptdb->FTableHasNonNullTaggedColumn()
        && ptdb->FidTaggedLast() >= ptdb->FidTaggedFirst() )
    {
        const COLUMNID  columnidTaggedLast      = ColumnidOfFid( ptdb->FidTaggedLast(), fTemplateTable );
        for ( columnid = ColumnidOfFid( ptdb->FidTaggedFirst(), fTemplateTable );
            columnid <= columnidTaggedLast;
            columnid++ )
        {
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
                            pfucb->u.pfcb,
                            columnid,
                            1,
                            dataRec,
                            &dataT,
                            JET_bitRetrieveIgnoreDefault | grbitRetrieveColumnDDLNotLocked ) );
                if ( JET_wrnColumnNull == err )
                {
                    return ErrERRCheck( JET_errNullInvalid );
                }

                Assert( JET_wrnColumnSetNull != err );
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
                    szIndex[0] = '\0';
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

    
    BYTE    *pbT = (BYTE *) pv;
    BYTE    *pbMax = pbT + cb;
    ULONG   ulChecksum = 0;

    
    for ( ; pbT < pbMax; pbT++ )
    {
        ulChecksum += *pbT;
        ulChecksum <<= 1;
    }

    return ulChecksum;
}

