// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

///#define SHOW_INDEX_PERF
#ifdef SHOW_INDEX_PERF
const JET_TRACETAG  tracetagIndexPerf   = JET_tracetagBtree;
#endif

// UNDONE:  TEMPORARY HACK to get around unversioned AddColumn/CreateIndex.

CCriticalSection    g_critUnverCol( CLockBasicInfo( CSyncBasicInfo( szFILEUnverCol ), rankFILEUnverCol, 0 ) );
CCriticalSection    g_critUnverIndex( CLockBasicInfo( CSyncBasicInfo( szFILEUnverIndex ), rankFILEUnverIndex, 0 ) );

struct UNVER_DDL
{
    AGENT       agentUnverDDL;
    OBJID       objidTable;
    CHAR        szName[JET_cbNameMost+4];   // +1 for null-terminator, +3 for 4-byte alignment
    UNVER_DDL   *punverNext;
};

UNVER_DDL   *g_punvercolGlobal = NULL;
UNVER_DDL   *g_punveridxGlobal = NULL;


//  ================================================================
ERR ErrFILEGetPfieldAndEnterDML(
    PIB         * ppib,
    FCB         * pfcb,
    const CHAR  * szColumnName,
    FIELD       ** ppfield,
    COLUMNID    * pcolumnid,
    BOOL        * pfColumnWasDerived,
    const BOOL  fLockColumn )
//  ================================================================
{
    ERR         err;

    *pfColumnWasDerived = fFalse;

    //  CONSIDER: Look in template table first because we don't
    //  have to grab DML/DDL latch (since DDL is fixed).  However,
    //  we used to do this, but we found that for Exchange,
    //  GetTableColumnInfo() calls would be very expensive because
    //  it is typically only called for derived columns and we
    //  first end up searching the 400+ columns of the template
    //  table

    //  WARNING: This function does a EnterDML() for the derived table only
    //  and stays in the latch if the requested field is found.  If the
    //  field is in the template table, the latch is not held.

    if ( fLockColumn )
    {
        Assert( ppib->Level() > 0 );
        Assert( !pfcb->Ptdb()->FTemplateTable() );
        Assert( !pfcb->FDomainDenyReadByUs( ppib ) );

        err = ErrCATAccessTableColumn(
                    ppib,
                    pfcb->Ifmp(),
                    pfcb->ObjidFDP(),
                    szColumnName,
                    pcolumnid,
                    fTrue );            //  read-lock the column in the catalog to ensure it doesn't disappear
        if ( err < 0 )
        {
            if ( JET_errColumnNotFound != err )
                return err;

            //  force retrieval from template table
            *ppfield = pfieldNil;
            err = JET_errSuccess;
        }
        else
        {
            CallS( err );   //  warnings not expected

            //  shouldn't be a template table if it needs,
            //  to be locked, and besides, we don't persist
            //  the template bit anyways
            Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );
            Assert( !*pfColumnWasDerived );

            pfcb->EnterDML();
            *ppfield = pfcb->Ptdb()->Pfield( *pcolumnid );
        }
    }
    else
    {
        pfcb->EnterDML();

        //  WARNING: The following function does a LeaveDML() on error
        CallR( ErrFILEPfieldFromColumnName(
                    ppib,
                    pfcb,
                    szColumnName,
                    ppfield,
                    pcolumnid ) );

            //  not expecting warnings
        CallS( err );

        if ( pfieldNil == *ppfield )
            pfcb->LeaveDML();
    }

    CallS( err );

    if ( pfieldNil == *ppfield )
    {
        FCB     * const pfcbTemplate    = pfcb->Ptdb()->PfcbTemplateTable();
        if ( pfcbNil != pfcbTemplate )
        {
            pfcb->Ptdb()->AssertValidDerivedTable();
            CallS( ErrFILEPfieldFromColumnName(
                        ppib,
                        pfcbTemplate,
                        szColumnName,
                        ppfield,
                        pcolumnid ) );
        }

        if ( pfieldNil != *ppfield )
        {
            //  must have found it in the template table
            Assert( pfcbNil != pfcbTemplate );
            Assert( FCOLUMNIDTemplateColumn( *pcolumnid ) );
            *pfColumnWasDerived = fTrue;
        }
        else
        {
            err = ErrERRCheck( JET_errColumnNotFound );
        }
    }
    else if ( FCOLUMNIDTemplateColumn( *pcolumnid ) )
    {
        Assert( pfcb->FTemplateTable() );
        pfcb->Ptdb()->AssertValidTemplateTable();
    }

    return err;
}


LOCAL BOOL FFILEIUnverColumnExists(
    TDB * const     ptdb,
    COLUMNID        columnid,
    const COLUMNID  columnidLastInitial,
    const COLUMNID  columnidLast,
    const CHAR *    szColumnName )
{
    Assert( FidOfColumnid( columnid ) == ptdb->FidFixedFirst()
        || FidOfColumnid( columnid ) == ptdb->FidVarFirst()
        || FidOfColumnid( columnid ) == ptdb->FidTaggedFirst() );

    if ( FidOfColumnid( columnidLast ) >= FidOfColumnid( columnid ) )
    {
        const STRHASH   strhash     = StrHashValue( szColumnName );

        for ( const FIELD * pfield = ptdb->Pfield( columnid );
            columnid <= columnidLast;
            pfield++, columnid++ )
        {
            if ( columnid == columnidLastInitial + 1 )
            {
                //  refresh in case the FIELD structures were
                //  partitioned into initial/dynamic fields
                //
                pfield = ptdb->Pfield( columnid );
            }

            Assert( pfield == ptdb->Pfield( columnid ) );
            if ( !FFIELDVersioned( pfield->ffield )
                && !FFIELDDeleted( pfield->ffield )
                && strhash == pfield->strhashFieldName
                && !UtilCmpName( szColumnName, ptdb->SzFieldName( pfield->itagFieldName, fFalse ) ) )
            {
                return fTrue;
            }
        }
    }

    return fFalse;
}

INLINE BOOL FFILEUnverColumnExists( FCB *pfcb, const CHAR *szColumnName )
{
    pfcb->EnterDML();

    TDB * const ptdb            = pfcb->Ptdb();
    const BOOL  fTemplateTable  = ptdb->FTemplateTable();
    const BOOL  fExists         = FFILEIUnverColumnExists(
                                                ptdb,
                                                ColumnidOfFid( ptdb->FidTaggedFirst(), fTemplateTable ),
                                                ColumnidOfFid( ptdb->FidTaggedLastInitial(), fTemplateTable ),
                                                ColumnidOfFid( ptdb->FidTaggedLast(), fTemplateTable ),
                                                szColumnName )
                                || FFILEIUnverColumnExists(
                                                ptdb,
                                                ColumnidOfFid( ptdb->FidFixedFirst(), fTemplateTable ),
                                                ColumnidOfFid( ptdb->FidFixedLastInitial(), fTemplateTable ),
                                                ColumnidOfFid( ptdb->FidFixedLast(), fTemplateTable ),
                                                szColumnName )
                                || FFILEIUnverColumnExists(
                                                ptdb,
                                                ColumnidOfFid( ptdb->FidVarFirst(), fTemplateTable ),
                                                ColumnidOfFid( ptdb->FidVarLastInitial(), fTemplateTable ),
                                                ColumnidOfFid( ptdb->FidVarLast(), fTemplateTable ),
                                                szColumnName );

    pfcb->LeaveDML();

    return fExists;
}


INLINE ERR ErrFILEInsertIntoUnverColumnList( FCB *pfcbTable, const CHAR *szColumnName )
{
    ERR         err         = JET_errSuccess;
    const OBJID objidTable  = pfcbTable->ObjidFDP();
    UNVER_DDL   *punvercol;

    g_critUnverCol.Enter();

FindColumn:
    for ( punvercol = g_punvercolGlobal; NULL != punvercol; punvercol = punvercol->punverNext )
    {
        if ( objidTable == punvercol->objidTable
            && 0 == UtilCmpName( punvercol->szName, szColumnName ) )
        {
            punvercol->agentUnverDDL.Wait( g_critUnverCol );

            if ( FFILEUnverColumnExists( pfcbTable, szColumnName ) )
            {
                g_critUnverCol.Leave();
                err = ErrERRCheck( JET_errColumnDuplicate );
                return err;
            }
            goto FindColumn;
        }
    }

    punvercol = (UNVER_DDL *)PvOSMemoryHeapAlloc( sizeof( UNVER_DDL ) );
    if ( NULL == punvercol )
        err = ErrERRCheck( JET_errOutOfMemory );
    else
    {
        memset( (BYTE *)punvercol, 0, sizeof( UNVER_DDL ) );
        new( &punvercol->agentUnverDDL ) AGENT;
        punvercol->objidTable = objidTable;
        Assert( strlen( szColumnName ) <= JET_cbNameMost );
        OSStrCbCopyA( punvercol->szName, sizeof(punvercol->szName), szColumnName );
        punvercol->punverNext = g_punvercolGlobal;
        g_punvercolGlobal = punvercol;
    }

    g_critUnverCol.Leave();

    return err;
}

INLINE BOOL FFILEUnverIndexExists( FCB *pfcbTable, const CHAR *szIndexName )
{
    TDB     *ptdb = pfcbTable->Ptdb();
    FCB     *pfcb;
    BOOL    fExists = fFalse;

    pfcbTable->EnterDML();

    for ( pfcb = pfcbTable; pfcbNil != pfcb; pfcb = pfcb->PfcbNextIndex() )
    {
        const IDB   * const pidb    = pfcb->Pidb();
        if ( pidbNil != pidb && !pidb->FDeleted() )
        {
            if ( UtilCmpName(
                    szIndexName,
                    ptdb->SzIndexName( pidb->ItagIndexName(), pfcb->FDerivedIndex() ) ) == 0 )
            {
                fExists = fTrue;
                break;
            }
        }
    }

    pfcbTable->LeaveDML();

    return fExists;
}

INLINE ERR ErrFILEInsertIntoUnverIndexList( FCB *pfcbTable, const CHAR *szIndexName )
{
    ERR         err         = JET_errSuccess;
    const OBJID objidTable  = pfcbTable->ObjidFDP();
    UNVER_DDL   *punveridx;

    g_critUnverIndex.Enter();

FindIndex:
    for ( punveridx = g_punveridxGlobal; NULL != punveridx; punveridx = punveridx->punverNext )
    {
        if ( objidTable == punveridx->objidTable
            && 0 == UtilCmpName( punveridx->szName, szIndexName ) )
        {
            punveridx->agentUnverDDL.Wait( g_critUnverIndex );

            if ( FFILEUnverIndexExists( pfcbTable, szIndexName ) )
            {
                g_critUnverIndex.Leave();
                err = ErrERRCheck( JET_errIndexDuplicate );
                return err;
            }
            goto FindIndex;
        }
    }

    punveridx = (UNVER_DDL *)PvOSMemoryHeapAlloc( sizeof( UNVER_DDL ) );
    if ( NULL == punveridx )
    {
        err = ErrERRCheck( JET_errOutOfMemory );
    }
    else
    {
        memset( (BYTE *)punveridx, 0, sizeof( UNVER_DDL ) );
        new( &punveridx->agentUnverDDL ) AGENT;
        punveridx->objidTable = objidTable;
        Assert( strlen( szIndexName ) <= JET_cbNameMost );
        OSStrCbCopyA( punveridx->szName, sizeof(punveridx->szName), szIndexName );
        punveridx->punverNext = g_punveridxGlobal;
        g_punveridxGlobal = punveridx;
    }

    g_critUnverIndex.Leave();

    return err;
}


LOCAL VOID FILERemoveFromUnverList(
    UNVER_DDL   **ppunverGlobal,
    CCriticalSection&       critUnver,
    const OBJID objidTable,
    const CHAR  *szName )
{
    UNVER_DDL   **ppunver;

    critUnver.Enter();

    Assert( NULL != *ppunverGlobal );
    for ( ppunver = ppunverGlobal; *ppunver != NULL; ppunver = &( (*ppunver)->punverNext ) )
    {
        if ( objidTable == (*ppunver)->objidTable
            && 0 == UtilCmpName( (*ppunver)->szName, szName ) )
        {
            UNVER_DDL   *punverToRemove;
            (*ppunver)->agentUnverDDL.Release( critUnver );
            punverToRemove = *ppunver;
            *ppunver = (*ppunver)->punverNext;
            punverToRemove->agentUnverDDL.~AGENT();
            OSMemoryHeapFree( punverToRemove );
            critUnver.Leave();
            return;
        }
    }

    Assert( fFalse );
    critUnver.Leave();
}

BOOL FFILEIIndicesHasSystemSpaceHints( __in_ecount(cIndexes) const JET_INDEXCREATE3_A * const rgIndexes, ULONG cIndexes )
{
    const JET_INDEXCREATE3_A * pidxcreate;
    const JET_INDEXCREATE3_A * pidxcreateNext;

    pidxcreate      = rgIndexes;
    for( SIZE_T iIndex = 0; iIndex < cIndexes; iIndex++, pidxcreate = pidxcreateNext )
    {
        pidxcreateNext  = (JET_INDEXCREATE3_A *)((BYTE *)( pidxcreate ) + pidxcreate->cbStruct);

        Assert( sizeof(JET_INDEXCREATE3_A) == pidxcreate->cbStruct );

        if ( FIsSystemSpaceHint( pidxcreate->pSpacehints ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}

BOOL FFILEITableHasSystemSpaceHints( _In_ const JET_TABLECREATE5_A * const ptablecreate )
{
    if ( FIsSystemSpaceHint( ptablecreate->pSeqSpacehints ) )
    {
        return fTrue;
    }
    if ( FIsSystemSpaceHint( ptablecreate->pLVSpacehints ) )
    {
        return fTrue;
    }
    return FFILEIIndicesHasSystemSpaceHints( ptablecreate->rgindexcreate, ptablecreate->cIndexes );
}

ERR ErrIDXCheckUnicodeFlagAndDefn(
    __in_ecount( cIndexCreate ) JET_INDEXCREATE3_A * pindexcreate,
    _In_ ULONG                               cIndexCreate )
{
    JET_INDEXCREATE3_A * pidxCurr = pindexcreate;
    JET_INDEXCREATE3_A * pidxNext = NULL;

    // We must check if the usage of the JET_bitIndexUnicode flag is consistent for the JetCreateIndex4* API.
    for ( ULONG iIndexCreate = 0 ;
          ( iIndexCreate < cIndexCreate ) && ( nullptr != pidxCurr );
          iIndexCreate++, pidxCurr = pidxNext )
    {
        pidxNext = (JET_INDEXCREATE3_A *)(((BYTE*)pidxCurr) + pidxCurr->cbStruct );

        // Only the current version of the structure should have made it here.
        // The older versions should have been up-converted in jetapi.cxx.
        Assert( pidxCurr->cbStruct == sizeof( JET_INDEXCREATE3_A ) );

        // When checking for null-ness, only look at the bottom 32 bits.
        // When a structure is created on the stack, then the upper 32-bits may be
        // garbage, and we did a raw memcpy() up above.
        const JET_UNICODEINDEX2* pidxunicodeNullCheck = ( JET_UNICODEINDEX2* ) ( DWORD_PTR( pidxCurr->pidxunicode ) & 0xffffffff );
        if ( ( ( nullptr != pidxunicodeNullCheck ) && ( 0 == ( pidxCurr->grbit & JET_bitIndexUnicode ) ) ) ||
            ( ( nullptr == pidxunicodeNullCheck ) && ( 0 != ( pidxCurr->grbit & JET_bitIndexUnicode ) ) ) )
        {
            return ErrERRCheck( JET_errInvalidParameter );
        }
    }

    return JET_errSuccess;
}

ERR VTAPI ErrIsamCreateTable( JET_SESID vsesid, JET_DBID vdbid, JET_TABLECREATE5_A *ptablecreate )
{
    ERR             err;
    PIB             *ppib = (PIB *)vsesid;
    FUCB            *pfucb;
    IFMP            ifmp = (IFMP) vdbid;

    //  check parameters
    //
    CallR( ErrPIBCheck( ppib ) );

    Expected( FInRangeIFMP( ifmp ) ); // but just in case, we handle the error ...
    if ( !FInRangeIFMP( ifmp ) )
    {
        err = ErrERRCheck( JET_errInvalidDatabaseId );
        return err;
    }

    if ( FFMPIsTempDB( ifmp ) )
    {
        err = ErrERRCheck( JET_errInvalidDatabaseId );
        return err;
    }    

    // Block this if the DB was open read-only.
    if ( g_rgfmp[ ifmp ].FReadOnlyAttach() )
    {
        return ErrERRCheck( JET_errPermissionDenied );
    }

    // UNDONE: Supported nesting of hierarchical DDL.
    if ( ptablecreate->grbit & JET_bitTableCreateTemplateTable )
    {
        if ( ptablecreate->szTemplateTableName != NULL )
        {
            err = ErrERRCheck( JET_errCannotNestDDL );
            return err;
        }

        //  This option was originally designed to allow for fixed and variable columns
        //  to be added to template tables post template creation. Neither the ability
        //  to add fixed and variable columns to template tables nor the blocking of
        //  adding them to derived tables are currently implemented in the engine, so
        //  we'll just reject this bit so users are aware of it. MSDN lists the bit as
        //  deprecated.

        if ( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
            return err;
        }
    }
    else if ( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables )
    {
        //  this grbit must be used in conjunction with JET_bitTableCreateTemplateTable
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    if ( ptablecreate->grbit & JET_bitTableCreateSystemTable )      //  internal use only
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    CallR( ErrIDXCheckUnicodeFlagAndDefn( ptablecreate->rgindexcreate, ptablecreate->cIndexes ) );

    //  create the table, and open it
    //
    CallR( ErrFILECreateTable( ppib, ifmp, ptablecreate ) );

    if ( ( ptablecreate->grbit & JET_bitTableCreateImmutableStructure ) != 0 )
    {
        // If the client has immutable structure, we must close the table as we can not pass the JET_TABLEID back to the client.
        ErrFILECloseTable( ppib, (FUCB *) ptablecreate->tableid );

        // Defense-in-depth: This value *should* get discarded at the jetapi layer.
        ptablecreate->tableid = JET_tableidNil;
    }
    else
    {
        pfucb = (FUCB *)(ptablecreate->tableid);
        pfucb->pvtfndef = &vtfndefIsam;
    }

    //  Unlikely occurance, but it is easy to check, so lets verify we're not passing an
    //  internal pointer up out the API.
    Assert( !FFILEITableHasSystemSpaceHints( ptablecreate ) );

    Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes + ( ptablecreate->szCallback ? 1 : 0 ) );

    return err;
}


//  return fTrue if the column type specified has a fixed length
//
INLINE BOOL FCOLTYPFixedLength( JET_COLTYP coltyp )
{
    switch( coltyp )
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
            return fTrue;

        default:
            return fFalse;
    }
}


LOCAL ERR ErrFILEIScanForColumnName(
    PIB             *ppib,
    FCB             *pfcb,
    const CHAR      *szColumnName,
    FIELD           **ppfield,
    COLUMNID        *pcolumnid,
    const COLUMNID  columnidLastInitial,
    const COLUMNID  columnidLast )
{
    ERR             err;
    TDB             *ptdb       = pfcb->Ptdb();
    FIELD           *pfield;
    const STRHASH   strhash     = StrHashValue( szColumnName );
    const BOOL      fDerived    = fFalse;       //  only looking at columns in this TDB (ie. no derived columns)

    Assert( FidOfColumnid( *pcolumnid ) == ptdb->FidFixedFirst()
        || FidOfColumnid( *pcolumnid ) == ptdb->FidVarFirst()
        || FidOfColumnid( *pcolumnid ) == ptdb->FidTaggedFirst() );

    Assert( pfieldNil == *ppfield );

    pfcb->AssertDML();

    Assert( FidOfColumnid( columnidLast ) >= FidOfColumnid( *pcolumnid ) );

    for ( pfield = ptdb->Pfield( *pcolumnid );
        *pcolumnid <= columnidLast;
        pfield++, (*pcolumnid)++ )
    {
        if ( *pcolumnid == columnidLastInitial + 1 )
        {
            //  refresh in case the FIELD structures were
            //  partitioned into initial/dynamic fields
            pfield = ptdb->Pfield( *pcolumnid );
        }

        Assert( pfield == ptdb->Pfield( *pcolumnid ) );
        Assert( !pfcb->FFixedDDL() || !FFIELDVersioned( pfield->ffield ) );
        if ( !FFIELDCommittedDelete( pfield->ffield ) )
        {
            Assert( JET_coltypNil != pfield->coltyp );
            Assert( 0 != pfield->itagFieldName );
            if ( strhash == pfield->strhashFieldName
                && 0 == UtilCmpName( szColumnName, ptdb->SzFieldName( pfield->itagFieldName, fDerived ) ) )
            {
                if ( FFIELDVersioned( pfield->ffield ) )
                {
                    COLUMNID    columnidT;

                    pfcb->LeaveDML();

                    //  no versioned operations on template table
                    Assert( !ptdb->FTemplateTable() );
                    Assert( !FCOLUMNIDTemplateColumn( *pcolumnid ) );

                    //  must consult catalog.
                    CallR( ErrCATAccessTableColumn(
                                ppib,
                                pfcb->Ifmp(),
                                pfcb->ObjidFDP(),
                                szColumnName,
                                &columnidT ) );
                    CallS( err );       //  shouldn't return warnings.

                    pfcb->EnterDML();

                    //  WARNING: the columnid returned
                    //  from the catalog lookup (columnidT)
                    //  may be different from the original
                    //  columnid (*pcolumnid) if the column
                    //  got deleted then re-added with the
                    //  same name
                    *pcolumnid = columnidT;

                    *ppfield = ptdb->Pfield( columnidT );
                }
                else
                {
                    Assert( !FFIELDDeleted( pfield->ffield ) );
                    Assert( ptdb->Pfield( *pcolumnid ) == pfield );
                    *ppfield = pfield;
                }

                Assert( pfieldNil != *ppfield );
                return JET_errSuccess;
            }
        }
    }

    Assert( pfieldNil == *ppfield );    // Scan was successful, but seek failed.
    return JET_errSuccess;
}

ERR ErrFILEPfieldFromColumnName(
    PIB         *ppib,
    FCB         *pfcb,
    const CHAR  *szColumnName,
    FIELD       **ppfield,
    COLUMNID    *pcolumnid )
{
    ERR         err;
    TDB         *ptdb           = pfcb->Ptdb();
    const BOOL  fTemplateTable  = ptdb->FTemplateTable();

    pfcb->AssertDML();

    *ppfield = pfieldNil;

    if ( ptdb->FidTaggedLast() >= ptdb->FidTaggedFirst() )
    {
        *pcolumnid = ColumnidOfFid( ptdb->FidTaggedFirst(), fTemplateTable );
        CallR( ErrFILEIScanForColumnName(
                    ppib,
                    pfcb,
                    szColumnName,
                    ppfield,
                    pcolumnid,
                    ColumnidOfFid( ptdb->FidTaggedLastInitial(), fTemplateTable ),
                    ColumnidOfFid( ptdb->FidTaggedLast(), fTemplateTable ) ) );

        if ( pfieldNil != *ppfield )
            return JET_errSuccess;
    }


    if ( ptdb->FidFixedLast() >= ptdb->FidFixedFirst() )
    {
        *pcolumnid = ColumnidOfFid( ptdb->FidFixedFirst(), fTemplateTable );
        CallR( ErrFILEIScanForColumnName(
                    ppib,
                    pfcb,
                    szColumnName,
                    ppfield,
                    pcolumnid,
                    ColumnidOfFid( ptdb->FidFixedLastInitial(), fTemplateTable ),
                    ColumnidOfFid( ptdb->FidFixedLast(), fTemplateTable ) ) );

        if ( pfieldNil != *ppfield )
            return JET_errSuccess;
    }

    if ( ptdb->FidVarLast() >= ptdb->FidVarFirst() )
    {
        *pcolumnid = ColumnidOfFid( ptdb->FidVarFirst(), fTemplateTable );
        CallR( ErrFILEIScanForColumnName(
                    ppib,
                    pfcb,
                    szColumnName,
                    ppfield,
                    pcolumnid,
                    ColumnidOfFid( ptdb->FidVarLastInitial(), fTemplateTable ),
                    ColumnidOfFid( ptdb->FidVarLast(), fTemplateTable ) ) );
    }

    return JET_errSuccess;
}

ERR ErrFILEGetPfieldFromColumnid(
    FCB             *pfcb,
    const COLUMNID  columnid,
    FIELD           **ppfield )
{
    TDB             *ptdb   = pfcb->Ptdb();
    const FID       fid     = FidOfColumnid( columnid );

    *ppfield = pfieldNil;

    Assert( FCOLUMNIDTemplateColumn( columnid ) == pfcb->FTemplateTable() );
    pfcb->AssertDML();

    //  special case of TDB::Pfield( fidT )
    if ( FCOLUMNIDTagged( columnid ) )
    {
        if ( fid >= ptdb->FidTaggedFirst() && fid <= ptdb->FidTaggedLast() )
            *ppfield = ptdb->PfieldTagged( columnid );
    }
    else if ( FCOLUMNIDFixed( columnid ) )
    {
        if ( fid >= ptdb->FidFixedFirst() && fid <= ptdb->FidFixedLast() )
            *ppfield = ptdb->PfieldFixed( columnid );
    }
    else if ( FCOLUMNIDVar( columnid ) )
    {
        if ( fid >= ptdb->FidVarFirst() && fid <= ptdb->FidVarLast() )
            *ppfield = ptdb->PfieldVar( columnid );
    }

    if ( pfieldNil == *ppfield )
    {
        return ErrERRCheck( JET_errColumnNotFound );
    }

    Assert( !FFIELDCommittedDelete( (*ppfield)->ffield ) );
    return JET_errSuccess;
}


LOCAL BOOL FFILEIColumnExists(
    const TDB * const   ptdb,
    const CHAR * const  szColumn,
    const FIELD * const pfieldStart,
    const ULONG         cfields )
{
    const STRHASH       strhash     = StrHashValue( szColumn );
    const FIELD * const pfieldMax   = pfieldStart + cfields;

    //  caller ensures there must be at least one column
    Assert( pfieldNil != pfieldStart );
    Assert( pfieldStart < pfieldMax );

    for ( const FIELD * pfield = pfieldStart; pfield < pfieldMax; pfield++ )
    {
        //  field may have been marked as deleted if an AddColumn rolled back
        if ( !FFIELDDeleted( pfield->ffield )
            && ( strhash == pfield->strhashFieldName )
            && ( 0 == UtilCmpName( szColumn, ptdb->SzFieldName( pfield->itagFieldName, fFalse ) ) ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}

LOCAL BOOL FFILEITemplateTableColumn(
    const FCB * const   pfcbTemplateTable,
    const CHAR * const  szColumn )
{
    Assert( pfcbNil != pfcbTemplateTable );
    Assert( pfcbTemplateTable->FTemplateTable() );
    Assert( pfcbTemplateTable->FFixedDDL() );

    const TDB * const   ptdbTemplateTable   = pfcbTemplateTable->Ptdb();
    Assert( ptdbTemplateTable != ptdbNil );
    ptdbTemplateTable->AssertValidTemplateTable();

    const ULONG         cInitialCols        = ptdbTemplateTable->CInitialColumns();
    const ULONG         cDynamicCols        = ptdbTemplateTable->CDynamicColumns();

    if ( cInitialCols > 0 )
    {
        if ( FFILEIColumnExists(
                    ptdbTemplateTable,
                    szColumn,
                    ptdbTemplateTable->PfieldsInitial(),
                    cInitialCols ) )
        {
            return fTrue;
        }
    }

    if ( cDynamicCols > 0 )
    {
        //  shouldn't be any fixed or variable dynamic columns in the template table
        Assert( ptdbTemplateTable->CDynamicFixedColumns() == 0 );
        Assert( ptdbTemplateTable->CDynamicVarColumns() == 0 );

        if ( FFILEIColumnExists(
                    ptdbTemplateTable,
                    szColumn,
                    (FIELD *)ptdbTemplateTable->MemPool().PbGetEntry( itagTDBFields ),
                    cDynamicCols ) )
        {
            return fTrue;
        }
    }

    return fFalse;
}

LOCAL BOOL FFILEITemplateTableIndex( const FCB * const pfcbTemplateTable, const CHAR *szIndex )
{
    Assert( pfcbNil != pfcbTemplateTable );
    Assert( pfcbTemplateTable->FTemplateTable() );
    Assert( pfcbTemplateTable->FFixedDDL() );

    const TDB   * const ptdbTemplateTable = pfcbTemplateTable->Ptdb();
    Assert( ptdbTemplateTable != ptdbNil );

    const FCB   *pfcbIndex;
    for ( pfcbIndex = pfcbTemplateTable;
        pfcbIndex != pfcbNil;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        const IDB   * const pidb = pfcbIndex->Pidb();
        if ( pfcbIndex == pfcbTemplateTable )
        {
            Assert( pidbNil == pidb || pidb->FPrimary() );
        }
        else
        {
            Assert( pfcbIndex->FTypeSecondaryIndex() );
            Assert( pidbNil != pidb );
            Assert( !pidb->FPrimary() );
        }
        if ( pidbNil != pidb
            && UtilCmpName( szIndex, ptdbTemplateTable->SzIndexName( pidb->ItagIndexName() ) ) == 0 )
        {
            return fTrue;
        }
    }

    return fFalse;
}


LOCAL ERR ErrFILEIValidateAddColumn(
    const IFMP      ifmp,
    PCSTR szName,
    __out_bcount(JET_cbNameMost+1)  PSTR szColumnName,
    FIELD           *pfield,
    const JET_GRBIT grbit,
    const INT       cbDefaultValue,
    const BOOL      fExclusiveLock,
    FCB             *pfcbTemplateTable )
{
    ERR             err;
    const   BOOL    fDefaultValue = ( cbDefaultValue > 0 );

    pfield->ffield = 0;
    
    if ( pfield->cbMaxLen > JET_cbLVColumnMost )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // Duplicate column names will get detected when catalog
    // insertion is attempted.
    CallR( ErrUTILCheckName( szColumnName, szName, ( JET_cbNameMost + 1 ) ) );
    if ( pfcbNil != pfcbTemplateTable
        && FFILEITemplateTableColumn( pfcbTemplateTable, szColumnName ) )
    {
        err = ErrERRCheck( JET_errColumnDuplicate );
        return err;
    }


//  OLD
//  if ( pfield->coltyp == 0 || pfield->coltyp > JET_coltypLongText )
//  NEW - because of the new column type
    if ( pfield->coltyp == 0 || pfield->coltyp >= JET_coltypMax || pfield->coltyp == JET_coltypSLV )
    {
        return ErrERRCheck( JET_errInvalidColumnType );
    }

    //  if column type is text then check code page
    //
    if ( FRECTextColumn( pfield->coltyp ) )
    {
        //  check code page
        //
        if ( 0 == pfield->cp )
        {
            //  force text column to always have a code page
            pfield->cp = usEnglishCodePage;
        }
        else if ( pfield->cp != usEnglishCodePage && pfield->cp != usUniCodePage )
        {
            return ErrERRCheck( JET_errInvalidCodePage );
        }
    }
    else
    {
        pfield->cp = 0;
    }

    if( ( grbit & JET_bitColumnCompressed ) && !FRECLongValue( pfield->coltyp ) )
    {
        return ErrERRCheck( JET_errColumnCannotBeCompressed );
    }

    if( ( grbit & JET_bitColumnEncrypted ) && ( !FRECLongValue( pfield->coltyp ) || ( grbit & JET_bitColumnMultiValued ) || fDefaultValue ) )
    {
        return ErrERRCheck( JET_errColumnCannotBeEncrypted );
    }

    // check conflicting Tagged column options
    if ( ( grbit & JET_bitColumnTagged ) || FRECLongValue( pfield->coltyp )  )
    {
        if ( grbit & JET_bitColumnFixed )
            return ErrERRCheck( JET_errInvalidGrbit );
    }
    else if ( grbit & JET_bitColumnMultiValued )
    {
        return ErrERRCheck( JET_errMultiValuedColumnMustBeTagged );
    }

    if ( grbit & JET_bitColumnEscrowUpdate )
    {
        //  Escrow columns can be updated by different sessions in a concurrent fashion
        //  in order for that to be possible the column must always be present in the record
        //  otherwise we will end up in a situation where a column must be inserted into the
        //  record before we escrow it. It would be extremely difficult to insert a column
        //  into a record concurrently (a replace would be required).
        //
        //  Thus, Escrow columns _must_ have a default value and must be fixed so that they
        //  will be burst into any new records. One alternative is to disallow default values
        //  for escrow columns so that they will have to be set before they can be escrowed --
        //  that would probably reduce their usefullness though..
        //
        //  This also means that an escrow column cannot be added to a table with any records
        //  aleady in it because the column will not be present in those records (the alternative
        //  is to syncronously insert the default-value of the column into all of the records).

        // Supports 32-bit and 64-bit column types
        if ( pfield->coltyp != JET_coltypLong && pfield->coltyp != JET_coltypLongLong )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }
        if ( grbit & JET_bitColumnTagged )
            return ErrERRCheck( JET_errCannotBeTagged );
        if ( grbit & JET_bitColumnVersion )
            return ErrERRCheck( JET_errInvalidGrbit );
        if ( grbit & JET_bitColumnAutoincrement )
            return ErrERRCheck( JET_errInvalidGrbit );

        if ( !fDefaultValue )
            return ErrERRCheck( JET_errInvalidGrbit );

        if ( !fExclusiveLock )
            return ErrERRCheck( JET_errExclusiveTableLockRequired );

        // Check if efv param allows us to create a 64-bit escrow column
        if ( pfield->coltyp == JET_coltypLongLong )
        {
            CallR( g_rgfmp[ ifmp ].ErrDBFormatFeatureEnabled( JET_efvEscrow64 ) );    // returns JET_errEngineFormatVersionParamTooLowForRequestedFeature if the feature isn't available.
        }

        if ( ( grbit & JET_bitColumnFinalize ) && ( grbit & JET_bitColumnDeleteOnZero ) )
        {
            // These two grbits are not allowed together.
            AssertSz( FNegTest( fInvalidAPIUsage ), "UseOfColumnFinalizeWithDeleteOnZeroInvalidBehavior" );
            if ( !FNegTest( fInvalidAPIUsage ) )
            {
                FireWall( "UseOfColumnFinalizeWithDeleteOnZeroInvalidBehavior" );
            }
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        FIELDSetEscrowUpdate( pfield->ffield );
        if ( grbit & JET_bitColumnFinalize )
        {
            // JET_bitColumnFinalize actually performs all the actions of JET_bitColumnDeleteOnZero but
            // we discourage it's usage in MSDN.  Checking if people are listening!?!?  :-)
            // See "Historical Notes" above dbtask.cxx FINALIZETASK<TDelta>::ErrExecute() for more deets.
            AssertTrack( FNegTest( fInvalidAPIUsage ), "NyiFinalizeBehaviorClientRequestedColumnGrbit" );
            FIELDSetFinalize( pfield->ffield );
        }
        if ( grbit & JET_bitColumnDeleteOnZero )
        {
            FIELDSetDeleteOnZero( pfield->ffield );
        }
    }
    else if ( grbit & (JET_bitColumnFinalize|JET_bitColumnDeleteOnZero) )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }
    else if ( grbit & (JET_bitColumnVersion|JET_bitColumnAutoincrement) )
    {
        //  if any special column properties have been set,
        //  then check parameters and set column attributes.
        //
        if ( grbit & JET_bitColumnAutoincrement )
        {
            if ( pfield->coltyp != JET_coltypLong
                && pfield->coltyp != JET_coltypLongLong
                && pfield->coltyp != JET_coltypUnsignedLongLong
                && pfield->coltyp != JET_coltypCurrency )
                return ErrERRCheck( JET_errInvalidGrbit );
        }
        else if ( pfield->coltyp != JET_coltypLong )
            return ErrERRCheck( JET_errInvalidGrbit );

        if ( grbit & JET_bitColumnTagged )
            return ErrERRCheck( JET_errCannotBeTagged );

        if ( grbit & JET_bitColumnVersion )
        {
            if ( grbit & (JET_bitColumnAutoincrement|JET_bitColumnEscrowUpdate) )
                return ErrERRCheck( JET_errInvalidGrbit );
            FIELDSetVersion( pfield->ffield );
        }

        else if ( grbit & JET_bitColumnAutoincrement )
        {
            Assert( !( grbit & JET_bitColumnVersion ) );
            if ( grbit & JET_bitColumnEscrowUpdate )
                return ErrERRCheck( JET_errInvalidGrbit );

            // For AutoInc or EscrowUpdate columns, we first need
            // exclusive use of the table.
            if ( !fExclusiveLock )
                return ErrERRCheck( JET_errExclusiveTableLockRequired );

            FIELDSetAutoincrement( pfield->ffield );
        }
    }

    //  Check FOR user-defined default value
    if( grbit & JET_bitColumnUserDefinedDefault )
    {
        if( grbit & JET_bitColumnFixed )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnNotNULL )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnVersion )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnAutoincrement )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnUpdatable )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnEscrowUpdate )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnFinalize )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnDeleteOnZero )
            return ErrERRCheck( JET_errInvalidGrbit );
        if( grbit & JET_bitColumnMaybeNull )
            return ErrERRCheck( JET_errInvalidGrbit );

        //  The column must be tagged. If it is fixed/variable
        //  we may have to copy the default value into the record
        //  this won't work for calculated defaults
        if( !( grbit & JET_bitColumnTagged )
            && !FRECLongValue( pfield->coltyp ) )
            return ErrERRCheck( JET_errInvalidGrbit );

        if( sizeof( JET_USERDEFINEDDEFAULT_A ) != cbDefaultValue )
            return ErrERRCheck( JET_errInvalidParameter );

        FIELDSetUserDefinedDefault( pfield->ffield );
    }

    if ( grbit & JET_bitColumnNotNULL )
    {
        FIELDSetNotNull( pfield->ffield );
    }

    if ( grbit & JET_bitColumnMultiValued )
    {
        FIELDSetMultivalued( pfield->ffield );
    }

    if ( grbit & JET_bitColumnCompressed )
    {
        FIELDSetCompressed( pfield->ffield );
    }

    if ( grbit & JET_bitColumnEncrypted )
    {
        FIELDSetEncrypted( pfield->ffield );
    }

    BOOL    fMaxTruncated = fFalse;
    pfield->cbMaxLen = UlCATColumnSize( pfield->coltyp, pfield->cbMaxLen, &fMaxTruncated );

    if ( fDefaultValue )
    {
        FIELDSetDefault( pfield->ffield );
        if( !FFIELDUserDefinedDefault( pfield->ffield ) && pfield->cbMaxLen && pfield->cbMaxLen < (ULONG)cbDefaultValue )
        {
            return ErrERRCheck( JET_errDefaultValueTooBig );
        }
    }

    return ( fMaxTruncated ? ErrERRCheck( JET_wrnColumnMaxTruncated ) : JET_errSuccess );
}


//  ================================================================
LOCAL ERR ErrFILEIAddColumns(
    PIB                 * const ppib,
    const IFMP          ifmp,
    JET_TABLECREATE5_A  * const ptablecreate,
    const OBJID         objidTable,
    FCB                 * const pfcbTemplateTable )
//  ================================================================
//
//  User-defined-default columns can be dependant on other columns
//  we need to know the columnids of all the normal columns in order
//  to be able to create user-defined-default columns. We make two
//  passes through the columns to do this -- once for the normal ones
//  and once for the user-defined defaults
//
//  OPTIMIZATION: on the first pass note if there are any user-defined defaults
//  and only make the second pass if there are
//
//-
{
    ERR                 err;
    FUCB                *pfucbCatalog       = pfucbNil;
    CHAR                szColumnName[ JET_cbNameMost+1 ];
    JET_COLUMNCREATE_A  *pcolcreate;
    JET_COLUMNCREATE_A  *plastcolcreate;
    BOOL                fSetColumnError     = fFalse;
    TCIB                tcib;
    USHORT              ibNextFixedOffset   = ibRECStartFixedColumns;
    FID                 fidVersion          = 0;
    FID                 fidAutoInc          = 0;
    const BOOL          fTemplateTable      = ( ptablecreate->grbit & JET_bitTableCreateTemplateTable );
    LONG                cbKeyMost           = 0;
    LONG                cbRecordMost        = 0;

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  table has been created
    //
    Assert( 1 == ptablecreate->cCreated );

    if ( ptablecreate->rgcolumncreate == NULL )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        return err;
    }

    //  compute the max record size allowed for this table based on the max key
    //  size for the primary index (or sequential index if a primary index was
    //  not defined)
    //
    cbKeyMost = JET_cbKeyMost_OLD;
    if ( ptablecreate->cIndexes > 0 )
    {
        SIZE_T                  iindexcreateT = 0;

        JET_INDEXCREATE3_A      *pindexcreateT = ptablecreate->rgindexcreate;
        JET_INDEXCREATE3_A      *pindexcreateNext;

        for ( ; iindexcreateT < ptablecreate->cIndexes; iindexcreateT++, pindexcreateT = pindexcreateNext )
        {

            // All paths should've upgraded to the new index structure.
            Assert( sizeof(JET_INDEXCREATE3_A) == pindexcreateT->cbStruct );
            // but in case we missed one, we'll use old style incrementing ...
            pindexcreateNext = (JET_INDEXCREATE3_A *)((BYTE *)pindexcreateT + pindexcreateT->cbStruct);

            Assert( sizeof( JET_INDEXCREATE3_A ) == pindexcreateT->cbStruct ); // but just in case leaving the code ...
            if (    ( pindexcreateT->grbit & JET_bitIndexPrimary ) &&
                    ( sizeof( JET_INDEXCREATE_A ) == pindexcreateT->cbStruct ||
                        sizeof( JET_INDEXCREATE2_A ) == pindexcreateT->cbStruct ||
                        sizeof( JET_INDEXCREATE3_A ) == pindexcreateT->cbStruct ) &&
                    ( pindexcreateT->grbit & JET_bitIndexKeyMost ) )
            {
                cbKeyMost = pindexcreateT->cbKeyMost;
            }
        }
    }
    cbRecordMost = REC::CbRecordMost( g_rgfmp[ ifmp ].CbPage(), cbKeyMost );

    if ( pfcbNil != pfcbTemplateTable )
    {
        TDB *ptdbTemplateTable = pfcbTemplateTable->Ptdb();

        Assert( pfcbTemplateTable->FTemplateTable() );
        Assert( pfcbTemplateTable->FFixedDDL() );
        ptdbTemplateTable->AssertValidTemplateTable();

        // Can't have same fidAutoInc and fidVersion.
        fidAutoInc = ptdbTemplateTable->FidAutoincrement();
        fidVersion = ptdbTemplateTable->FidVersion();
        Assert( 0 == fidAutoInc || fidVersion != fidAutoInc );
        Assert( 0 == fidVersion || fidAutoInc != fidVersion );

        //  fixed and variable columns continue column space started by template table
        //  tagged columns start their own column space
        tcib.fidFixedLast = ptdbTemplateTable->FidFixedLast();
        tcib.fidVarLast = ptdbTemplateTable->FidVarLast();
        ibNextFixedOffset = ptdbTemplateTable->IbEndFixedColumns();
    }

    Assert( fidTaggedLeast-1 == tcib.fidTaggedLast );

    CallR( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    Assert( ptablecreate->rgcolumncreate != NULL );

    plastcolcreate = ptablecreate->rgcolumncreate + ptablecreate->cColumns;

    for ( pcolcreate = ptablecreate->rgcolumncreate;
        pcolcreate < plastcolcreate;
        pcolcreate++ )
    {
        Assert( pcolcreate != NULL );
        Assert( pcolcreate < ptablecreate->rgcolumncreate + ptablecreate->cColumns );

        //  this is the data that will actually be inserted into the catalog
        const VOID      *pvDefaultAdd   = NULL;
        ULONG           cbDefaultAdd    = 0;
        CHAR            *szCallbackAdd  = NULL;
        const VOID      *pvUserDataAdd  = NULL;
        ULONG           cbUserDataAdd   = 0;

        if ( pcolcreate->cbStruct != sizeof(JET_COLUMNCREATE_A) )
        {
            err = ErrERRCheck( JET_errInvalidParameter );
            goto HandleError;
        }

        pvDefaultAdd = pcolcreate->pvDefault;
        cbDefaultAdd = pcolcreate->cbDefault;

        FIELD           field;
        memset( &field, 0, sizeof(FIELD) );
        field.coltyp = FIELD_COLTYP( pcolcreate->coltyp );
        field.cbMaxLen = pcolcreate->cbMax;
        field.cp = (USHORT)pcolcreate->cp;

        //  UNDONE: interpret pbDefault of NULL for NULL value, and
        //          cbDefault == 0 and pbDefault != NULL as set to
        //          zero length.
        Assert( pcolcreate->cbDefault == 0 || pcolcreate->pvDefault != NULL );

        fSetColumnError = fTrue;

        // May return a warning.  Hold warning in pcolcreate->err unless
        // error encountered.
        Call( ErrFILEIValidateAddColumn(
                    ifmp,
                    pcolcreate->szColumnName,
                    szColumnName,
                    &field,
                    pcolcreate->grbit,
                    ( pcolcreate->pvDefault ? pcolcreate->cbDefault : 0 ),
                    fTrue,
                    pfcbTemplateTable ) );

        //  for fixed-length columns, make sure record not too big
        //
        Assert( tcib.fidFixedLast >= fidFixedLeast ?
            ibNextFixedOffset > ibRECStartFixedColumns :
            ibNextFixedOffset == ibRECStartFixedColumns );
        if ( ( ( pcolcreate->grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( field.coltyp ) )
            && ibNextFixedOffset + pcolcreate->cbMax > (SIZE_T)cbRecordMost )
        {
            err = ErrERRCheck( JET_errRecordTooBig );
            goto HandleError;
        }

        Call( ErrFILEGetNextColumnid(
                    field.coltyp,
                    pcolcreate->grbit,
                    fTemplateTable,
                    &tcib,
                    &pcolcreate->columnid ) );

        //  update TDB
        //
        Assert( 0 == field.ibRecordOffset );
        if ( FCOLUMNIDFixed( pcolcreate->columnid ) )
        {
            Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidFixedLast );
            field.ibRecordOffset = ibNextFixedOffset;
            ibNextFixedOffset = USHORT( ibNextFixedOffset + field.cbMaxLen );
        }
        else if ( FCOLUMNIDTagged( pcolcreate->columnid ) )
        {
            Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidTaggedLast );
        }
        else
        {
            Assert( FCOLUMNIDVar( pcolcreate->columnid ) );
            Assert( FidOfColumnid( pcolcreate->columnid ) == tcib.fidVarLast );
        }

        if ( FFIELDVersion( field.ffield ) )
        {
            Assert( !FFIELDAutoincrement( field.ffield ) );
            Assert( !FFIELDEscrowUpdate( field.ffield ) );
            Assert( field.coltyp == JET_coltypLong );
            Assert( FCOLUMNIDFixed( pcolcreate->columnid ) );
            if ( 0 != fidVersion )
            {
                err = ErrERRCheck( JET_errColumnRedundant );
                goto HandleError;
            }
            fidVersion = FidOfColumnid( pcolcreate->columnid );
        }
        else if ( FFIELDAutoincrement( field.ffield ) )
        {
            Assert( !FFIELDVersion( field.ffield ) );
            Assert( !FFIELDEscrowUpdate( field.ffield ) );
            Assert( field.coltyp == JET_coltypLong
                || field.coltyp == JET_coltypLongLong
                || field.coltyp == JET_coltypUnsignedLongLong
                || field.coltyp == JET_coltypCurrency );
            Assert( FCOLUMNIDFixed( pcolcreate->columnid ) );
            if ( 0 != fidAutoInc )
            {
                err = ErrERRCheck( JET_errColumnRedundant );
                goto HandleError;
            }
            fidAutoInc = FidOfColumnid( pcolcreate->columnid );
        }
        else if ( FFIELDEscrowUpdate( field.ffield ) )
        {
            Assert( !FFIELDVersion( field.ffield ) );
            Assert( !FFIELDAutoincrement( field.ffield ) );
            Assert( field.coltyp == JET_coltypLong || field.coltyp == JET_coltypLongLong );
            Assert( FidOfColumnid( pcolcreate->columnid ).FFixed() );
        }
        else if ( FFIELDUserDefinedDefault( field.ffield ) )
        {
            JET_USERDEFINEDDEFAULT_A * const pudd = (JET_USERDEFINEDDEFAULT_A *)(pcolcreate->pvDefault);

            //  the table will be opened from the catalog later so we don't need to deal with registering
            //  a CBDESC right now. Make sure that it can be resolved though
            JET_CALLBACK callback = NULL;
            if ( BoolParam( PinstFromPpib( ppib ), JET_paramEnablePersistedCallbacks ) )
            {
                Call( ErrCALLBACKResolve( pudd->szCallback, &callback ) );
            }

            //  for user-defined-defaults we don't store a default value at all
            pvDefaultAdd    = NULL;
            cbDefaultAdd    = 0;
            szCallbackAdd   = pudd->szCallback;
            pvUserDataAdd   = pudd->pbUserData;
            cbUserDataAdd   = pudd->cbUserData;
        }

        fSetColumnError = fFalse;
        pcolcreate->err = JET_errSuccess;

        ptablecreate->cCreated++;
        Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns );

        if ( fTemplateTable )
        {
            FIELDSetTemplateColumnESE98( field.ffield );
        }

        Call( ErrCATAddTableColumn(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    szColumnName,
                    pcolcreate->columnid,
                    &field,
                    pvDefaultAdd,
                    cbDefaultAdd,
                    szCallbackAdd,
                    pvUserDataAdd,
                    cbUserDataAdd ) );
    }   // for

    err = JET_errSuccess;

HandleError:
    CallS( ErrCATClose( ppib, pfucbCatalog ) );

    if ( fSetColumnError )
    {
        Assert( err < 0 );
        Assert( pcolcreate != NULL );
        Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE_A) );
        pcolcreate->err = err;
    }

    return err;
}

LOCAL ERR ErrFILEIValidateCreateIndex(
    INST * const                    pinst,
    const IFMP                      ifmp,
    IDB *                           pidb,
    __out_ecount(JET_ccolKeyMost) const CHAR *                  rgszColumns[],
    __out_ecount(JET_ccolKeyMost) BYTE * const  rgfbDescending,
    const JET_INDEXCREATE3_A * const    pidxcreate,
    __inout JET_SPACEHINTS *            pspacehints )
{
    ERR                             err;
    const JET_GRBIT                 grbit               = pidxcreate->grbit;
    const CHAR *                    szKey               = pidxcreate->szKey;
    const ULONG                     cchKey              = pidxcreate->cbKey;
    const JET_UNICODEINDEX2 * const pidxunicode         = pidxcreate->pidxunicode;

    const BOOL                      fConditional        = ( pidxcreate->cConditionalColumn > 0 );
    const BOOL                      fPrimary            = ( grbit & JET_bitIndexPrimary );
    const BOOL                      fUnique             = ( grbit & ( JET_bitIndexUnique|JET_bitIndexPrimary ) );
    const BOOL                      fDisallowNull       = ( grbit & JET_bitIndexDisallowNull );
    const BOOL                      fIgnoreNull         = ( grbit & JET_bitIndexIgnoreNull );
    const BOOL                      fIgnoreAnyNull      = ( grbit & JET_bitIndexIgnoreAnyNull );
    const BOOL                      fIgnoreFirstNull    = ( grbit & JET_bitIndexIgnoreFirstNull );
    const BOOL                      fEmptyIndex         = ( grbit & JET_bitIndexEmpty );
    const BOOL                      fSortNullsHigh      = ( grbit & JET_bitIndexSortNullsHigh );
    const BOOL                      fUserDefinedUnicode = ( grbit & JET_bitIndexUnicode );
    const BOOL                      fCustomTupleLimits  = ( grbit & JET_bitIndexTupleLimits );
    const BOOL                      fTuples             = ( ( grbit & JET_bitIndexTuples )
                                                            || fCustomTupleLimits );
    const BOOL                      fCrossProduct   = ( grbit & JET_bitIndexCrossProduct );
    const BOOL                      fNestedTable    = ( grbit & JET_bitIndexNestedTable );
    const BOOL                      fDotNetGuid     = ( grbit & JET_bitIndexDotNetGuid );
    const BOOL                      fKeyMost            = ( grbit & JET_bitIndexKeyMost );
    const BOOL                      fDisallowTruncation = ( grbit & JET_bitIndexDisallowTruncation );

    const CHAR *                    pch                 = szKey;
    ULONG                           cFields             = 0;
    USHORT                          cbKeyMost           = 0;
    USHORT                          cbVarSegMac         = 0;

    //  if index has custom key size set then set in IDB
    //  otherwise default to historic maximum key size of 255 bytes
    //
    if ( fKeyMost )
    {
        if ( pidxcreate->cbKeyMost < JET_cbKeyMostMin || pidxcreate->cbKeyMost > (ULONG) CbKeyMostForPage() )
        {
            return ErrERRCheck( JET_errIndexInvalidDef );
        }
        cbKeyMost = (USHORT)pidxcreate->cbKeyMost;
    }
    else
    {
        cbKeyMost = JET_cbKeyMost_OLD;
    }

    //  set cbVarSegMac as requested or default to set key size for index
    //
    if ( fCustomTupleLimits || 0 == pidxcreate->cbVarSegMac )
    {
        cbVarSegMac = cbKeyMost;
    }
    else
    {
        cbVarSegMac =(USHORT)pidxcreate->cbVarSegMac;
    }

    pidb->ResetFlags();

    //  do not allow primary indexes with any ignore bits on
    //
    if ( fPrimary && ( fIgnoreNull || fIgnoreAnyNull || fIgnoreFirstNull ) )
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    //  do not allow both linear and cross product
    //
    if ( fCrossProduct && fNestedTable )
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    if ( fPrimary && fConditional )
    {
        err = ErrERRCheck( JET_errIndexInvalidDef );
        return err;
    }

    if ( fEmptyIndex && !fIgnoreAnyNull )
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
        return err;
    }

    if ( fCustomTupleLimits && !pidxcreate->ptuplelimits )
    {
        err = ErrERRCheck( JET_errIndexInvalidDef );
        return err;
    }

    if ( fUserDefinedUnicode )
    {
        if ( NULL == pidxunicode )
        {
            err = ErrERRCheck( JET_errIndexInvalidDef );
            return err;
        }

        if ( NULL != pidxunicode->szLocaleName )
        {
            pidb->SetWszLocaleName( pidxunicode->szLocaleName );
        }

        if ( pidxunicode->dwMapFlags )
        {
            pidb->SetDwLCMapFlags( pidxunicode->dwMapFlags );
        }
        else
        {
            pidb->SetDwLCMapFlags( pinst->m_dwLCMapFlagsDefault );
        }
    }
    else
    {
        // Catch that it's a 'reasonable' pointer value and not an LCID. (The
        // older version of the structure was a union with an LCID.)
        const JET_UNICODEINDEX2* pidxunicodeNullCheck = ( JET_UNICODEINDEX2* ) ( DWORD_PTR( pidxunicode ) & 0xffffffff );
        Assert( pidxunicodeNullCheck == NULL || (__int64)pidxunicodeNullCheck > (__int64)64*1024 );

        // With the JET_INDEXCREATE3_A structure, we no longer need to worry about
        // the LCID being specified without JET_bitIndexUnicode.

        // The user didn't pass in JET_bitIndexUnicode (or the LCID in older versions of JET_INDEXCREATE was 0 [lcidNone]).
        // Even if pidxunicode is valid it will be ignored. Fall back to the instance's defaults.
        pidb->SetWszLocaleName( pinst->m_wszLocaleNameDefault );
        pidb->SetDwLCMapFlags( pinst->m_dwLCMapFlagsDefault );
    }

    //  check index description for required format
    //
    if ( cchKey == 0 )
    {
        err = ErrERRCheck( JET_errInvalidParameter );
        return err;
    }

    if ( ( szKey[0] != '+' && szKey[0] != '-' ) ||
        szKey[cchKey - 1] != '\0' ||
        szKey[cchKey - 2] != '\0' )
    {
        err = ErrERRCheck( JET_errIndexInvalidDef );
        return err;
    }
    Assert( szKey[cchKey - 1] == '\0' );
    Assert( szKey[cchKey - 2] == '\0' );

    CallR( ErrCATCheckJetSpaceHints( g_rgfmp[ ifmp ].CbPage(), pspacehints, fTrue ) );

    pch = szKey;
    while ( *pch != '\0' )
    {
        if ( cFields >= JET_ccolKeyMost )
        {
            err = ErrERRCheck( JET_errIndexInvalidDef );
            return err;
        }
        if ( *pch == '-' )
        {
            rgfbDescending[cFields] = fTrue;
            pch++;
        }
        else
        {
            rgfbDescending[cFields] = fFalse;
            if ( *pch == '+' )
                pch++;
        }
        rgszColumns[cFields++] = pch;
        pch += strlen( pch ) + 1;
    }
    if ( cFields == 0 )
    {
        err = ErrERRCheck( JET_errIndexInvalidDef );
        return err;
    }

    //  number of columns should not exceed maximum
    //
    Assert( cFields <= JET_ccolKeyMost );

    //  get locale from end of szKey if present
    //
    pch++;
    Assert( pch > szKey );
    if ( (unsigned)( pch - szKey ) < cchKey )
    {
        EnforceSz( fFalse, "UnsupportedIndexKeyString" );
        return ErrERRCheck( JET_errIndexInvalidDef );
    }

    if ( cbVarSegMac <= 0 || cbVarSegMac > cbKeyMost )
    {
        err = ErrERRCheck( JET_errIndexInvalidDef );
        return err;
    }

    BOOL fUppercaseTextNormalization = fFalse;

    // At this point the locale and map flags may have been set by pidxunicode, and
    // may have been overwritten by the index-string syntax.
    if ( fUserDefinedUnicode )
    {
        CallR( ErrNORMCheckLocaleName( pinst, pidb->WszLocaleName() ) );

        // Check the incoming LCMap Flags, possibly adding necessary flags.
        DWORD dwLCMapFlags = pidb->DwLCMapFlags();
        if ( dwLCMapFlags & LCMAP_UPPERCASE )
    {
            CallR( g_rgfmp[ifmp].ErrDBFormatFeatureEnabled( JET_efvUppercaseTextNormalization ) );
            fUppercaseTextNormalization = fTrue;
    }
    
        CallR( ErrNORMCheckLCMapFlags( pinst, &dwLCMapFlags, fUppercaseTextNormalization ) );

        pidb->SetDwLCMapFlags( dwLCMapFlags );
        pidb->SetFLocaleSet();
    }
    else
    {
        // Even if there's no user defined unicode (JET_bitIndexUnicode), the locale name gets set to the
        // default up above. pinst->m_wszLocaleNameDefault

        CallS( ErrNORMCheckLCMapFlags( pinst, pidb->DwLCMapFlags(), fUppercaseTextNormalization ) );
        if ( !FNORMEqualsLocaleName( pidb->WszLocaleName(), pinst->m_wszLocaleNameDefault ) )
        {
            pidb->SetFLocaleSet();
            CallR( ErrNORMCheckLocaleName( pinst, pidb->WszLocaleName() ) );
        }
        else
        {
            Assert( !pidb->FLocaleSet() );
        }
    }

    Assert( JET_errSuccess == ErrNORMCheckLocaleName( pinst, pidb->WszLocaleName() ) );
    Assert( 0 != pidb->DwLCMapFlags() );

    if ( fTuples )
    {
        if ( fPrimary )
            return ErrERRCheck( JET_errIndexTuplesSecondaryIndexOnly );

        if ( fUnique )
            return ErrERRCheck( JET_errIndexTuplesNonUniqueOnly );


        if ( cbVarSegMac < cbKeyMost )
            return ErrERRCheck( JET_errIndexTuplesVarSegMacNotAllowed );

        pidb->SetFTuples();
        pidb->SetCbVarSegMac( cbKeyMost );
        pidb->SetCidxseg( (BYTE)cFields );

        const ULONG chLengthMin     = ( fCustomTupleLimits ?
                                                pidxcreate->ptuplelimits->chLengthMin :
                                                (ULONG)UlParam( pinst, JET_paramIndexTuplesLengthMin ) );
        const ULONG chLengthMax     = ( fCustomTupleLimits ?
                                                pidxcreate->ptuplelimits->chLengthMax :
                                                (ULONG)UlParam( pinst, JET_paramIndexTuplesLengthMax ) );
        const ULONG chToIndexMax    = ( fCustomTupleLimits ?
                                                        pidxcreate->ptuplelimits->chToIndexMax :
                                                        (ULONG)UlParam( pinst, JET_paramIndexTuplesToIndexMax ) );

        const ULONG cchIncrement    = ( fCustomTupleLimits ?
                                                        pidxcreate->ptuplelimits->cchIncrement :
                                                        (ULONG)UlParam( pinst, JET_paramIndexTupleIncrement ) );
        const ULONG ichStart        = ( fCustomTupleLimits ?
                                                        pidxcreate->ptuplelimits->ichStart :
                                                        (ULONG)UlParam( pinst, JET_paramIndexTupleStart ) );

        if ( ( chLengthMin > usMax ) ||
             ( chLengthMax > usMax ) ||
             ( chToIndexMax > usMax ) ||
             ( cchIncrement > usMax ) ||
             ( ( ichStart > usMax ) && (ichStart != -1 ) ) )
        {
            // Truncation leads to behavior that doesn't match what was requested.
            return ErrERRCheck( JET_errIndexTuplesInvalidLimits );
        }
        
        pidb->SetCchTuplesLengthMin( USHORT( 0 != chLengthMin ? chLengthMin : cchIDXTuplesLengthMinDefault ) );
        pidb->SetCchTuplesLengthMax( USHORT( 0 != chLengthMax ? chLengthMax : cchIDXTuplesLengthMaxDefault ) );
        pidb->SetIchTuplesToIndexMax( USHORT( 0 != chToIndexMax ? chToIndexMax : cchIDXTuplesToIndexMaxDefault ) );
        pidb->SetCchTuplesIncrement( USHORT( 0 != cchIncrement ? cchIncrement : cchIDXTupleIncrementDefault ) );
        pidb->SetIchTuplesStart( USHORT( ichStart != -1 ? ichStart : ichIDXTupleStartDefault ) );

        Assert( 0 != pidb->CchTuplesIncrement() );
        
        if ( pidb->CchTuplesLengthMin() < cchIDXTuplesLengthMinAbsolute
            || pidb->CchTuplesLengthMin() > cchIDXTuplesLengthMaxAbsolute
            || pidb->CchTuplesLengthMax() < cchIDXTuplesLengthMinAbsolute
            || pidb->CchTuplesLengthMax() > cchIDXTuplesLengthMaxAbsolute
            || pidb->CchTuplesLengthMin() > pidb->CchTuplesLengthMax()
            || pidb->IchTuplesToIndexMax() > ichIDXTuplesToIndexMaxAbsolute )
        {
            return ErrERRCheck( JET_errIndexTuplesInvalidLimits );
        }

    }
    else
    {
        pidb->SetCbVarSegMac( (USHORT)cbVarSegMac );
        pidb->SetCidxseg( (BYTE)cFields );
    }

    //  set key size for index
    //
    pidb->SetCbKeyMost( cbKeyMost );

    if ( !fDisallowNull && !fIgnoreAnyNull )
    {
        pidb->SetFAllowSomeNulls();
        if ( !fIgnoreFirstNull )
        {
            pidb->SetFAllowFirstNull();
            if ( !fIgnoreNull )
                pidb->SetFAllowAllNulls();
        }
    }

    if ( fUnique )
    {
        pidb->SetFUnique();
    }
    if ( fPrimary )
    {
        pidb->SetFPrimary();
    }
    if ( fDisallowNull )
    {
        pidb->SetFNoNullSeg();
    }
    else if ( fSortNullsHigh )
    {
        pidb->SetFSortNullsHigh();
    }
    
    //  not both linear and cross product
    //
    Assert( !( fCrossProduct && fNestedTable ) );
    if ( fCrossProduct )
    {
        pidb->SetFCrossProduct();
    }
    else if ( fNestedTable )
    {
        pidb->SetFNestedTable();
    }

    if ( fDisallowTruncation )
    {
        pidb->SetFDisallowTruncation();
    }

    if ( fDotNetGuid )
    {
        pidb->SetFDotNetGuid();
    }

#ifdef DEBUG
    IDB     idbT( pinst );
    idbT.SetFlagsFromGrbit( grbit );
    if ( pidb->FLocaleSet() )
        idbT.SetFLocaleSet();
    Assert( idbT.FPersistedFlags() == pidb->FPersistedFlags() );
    Assert( idbT.FPersistedFlagsX() == pidb->FPersistedFlagsX() );
#endif

    return JET_errSuccess;
}

JET_SPACEHINTS * PjsphFromDensity(
    _Out_ JET_SPACEHINTS * pjsphPreAlloc,
    _In_ const JET_SPACEHINTS * const pjsphTemplateOrTable )
{
    Assert( pjsphPreAlloc );

    memset( pjsphPreAlloc, 0, sizeof(*pjsphPreAlloc) );
    pjsphPreAlloc->cbStruct = sizeof(*pjsphPreAlloc);
    pjsphPreAlloc->ulInitialDensity = ( pjsphTemplateOrTable && pjsphTemplateOrTable->ulInitialDensity ) ?
                    pjsphTemplateOrTable->ulInitialDensity :
                    ulFILEDefaultDensity;
    pjsphPreAlloc->cbInitial = g_cbPage;

    return pjsphPreAlloc;
}

LOCAL ERR ErrFILEICreateIndexes(
    PIB *               ppib,
    IFMP                ifmp,
    JET_TABLECREATE5_A* ptablecreate,
    PGNO                pgnoTableFDP,
    OBJID               objidTable,
    const JET_SPACEHINTS * pTableSpacehints,
    FCB *               pfcbTemplateTable )
{
    ERR             err = JET_errSuccess;
    FUCB            *pfucbTableExtent       = pfucbNil;
    FUCB            *pfucbCatalog           = pfucbNil;
    CHAR            szIndexName[ JET_cbNameMost+1 ];
    const CHAR      *rgsz[JET_ccolKeyMost];
    BYTE            rgfbDescending[JET_ccolKeyMost];
    IDXSEG          rgidxseg[JET_ccolKeyMost];
    IDXSEG          rgidxsegConditional[JET_ccolKeyMost];
    JET_INDEXCREATE3_A  *pidxcreate             = NULL;
    PGNO            pgnoIndexFDP;
    OBJID           objidIndex;
    IDB             idb( PinstFromIfmp( ifmp ) );
    BOOL            fProcessedPrimary       = fFalse;
    BOOL            fSetIndexError          = fFalse;
    const BOOL      fTemplateTable          = ( ptablecreate->grbit & JET_bitTableCreateTemplateTable );

    JET_INDEXCREATE3_A  * pidxcreateNext = NULL;

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    Assert( !FCATSystemTable( pgnoTableFDP ) );

    Assert( ptablecreate->cIndexes > 0 );
    Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns );

    if ( ptablecreate->rgindexcreate == NULL
        || ptablecreate->rgcolumncreate == NULL )   // must have columns in order to create indexes
    {
        //  if an invalid structure is encountered, get out right away
        //
        err = ErrERRCheck( JET_errInvalidCreateIndex );
        return err;
    }

    // Open cursor for space navigation
    CallR( ErrDIROpen( ppib, pgnoTableFDP, ifmp, &pfucbTableExtent ) );
    Assert( pfucbNil != pfucbTableExtent );
    Assert( !FFUCBVersioned( pfucbTableExtent ) );  // Verify won't be deferred closed.
    Assert( pfcbNil != pfucbTableExtent->u.pfcb );
    Assert( !pfucbTableExtent->u.pfcb->FInitialized() );
    Assert( pfucbTableExtent->u.pfcb->Pidb() == pidbNil );

    //  force the FCB to be initialized successfully

    pfucbTableExtent->u.pfcb->Lock();
    pfucbTableExtent->u.pfcb->CreateComplete();
    pfucbTableExtent->u.pfcb->Unlock();

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    if ( pfcbNil != pfcbTemplateTable )
    {
        Assert( !fTemplateTable );
        Assert( pfcbTemplateTable->FTemplateTable() );
        Assert( pfcbTemplateTable->FTypeTable() );
        Assert( pfcbTemplateTable->FFixedDDL() );
        if ( pfcbTemplateTable->FSequentialIndex() )
        {
            Assert( pfcbTemplateTable->Pidb() == pidbNil );
        }
        else
        {
            Assert( pfcbTemplateTable->Pidb() != pidbNil );
            Assert( pfcbTemplateTable->Pidb()->FPrimary() );
            Assert( pfcbTemplateTable->Pidb()->FTemplateIndex() );
            fProcessedPrimary = fTrue;
        }
    }

    pidxcreate      = ptablecreate->rgindexcreate;
    for( SIZE_T iIndex = 0; iIndex < ptablecreate->cIndexes; iIndex++, pidxcreate = pidxcreateNext )
    {
        pidxcreateNext  = (JET_INDEXCREATE3_A *)((BYTE *)( pidxcreate ) + pidxcreate->cbStruct);

        Assert( sizeof(JET_INDEXCREATE3_A) == pidxcreate->cbStruct );
        if ( ((JET_INDEXCREATE3_A *)pidxcreate)->pSpacehints )
        {
            Assert( ((JET_INDEXCREATE3_A *)pidxcreate)->pSpacehints->cbStruct == sizeof(JET_SPACEHINTS) );
        }

        Assert( (JET_INDEXCREATE3_A *)pidxcreate < ((JET_INDEXCREATE3_A*)(ptablecreate->rgindexcreate)) + ptablecreate->cIndexes );

        if ( pidxcreate->cbStruct != sizeof(JET_INDEXCREATE3_A) )
        {
            //  if an invalid structure is encountered, get out right away
            //
            err = ErrERRCheck( JET_errInvalidCreateIndex );
            goto HandleError;
        }

        pidxcreate->err = JET_errSuccess;

        fSetIndexError = fTrue;

        Call( ErrUTILCheckName( szIndexName, pidxcreate->szIndexName, JET_cbNameMost+1 ) );

        JET_SPACEHINTS          jsphIndex;
        PjsphFromDensity( &jsphIndex, pTableSpacehints );
        if ( pidxcreate->pSpacehints )
        {
            jsphIndex = *(pidxcreate->pSpacehints);
        }

        Call( ErrFILEIValidateCreateIndex(
                    PinstFromPpib( ppib ),
                    ifmp,
                    &idb,
                    rgsz,
                    rgfbDescending,
                    pidxcreate,
                    &jsphIndex ) );

        ULONG iidxseg;
        for ( iidxseg = 0 ; iidxseg < idb.Cidxseg(); iidxseg++ )
        {
            COLUMNID                columnidT;
            BOOL                    fEscrow;
            BOOL                    fMultivalued;
            BOOL                    fText;
            BOOL                    fBinary;
            BOOL                    fLocalizedText;
            BOOL                    fEncrypted;
            const JET_COLUMNCREATE_A    * pcolcreate            = ptablecreate->rgcolumncreate;
            const JET_COLUMNCREATE_A    * const plastcolcreate  = pcolcreate + ptablecreate->cColumns;

            Assert( NULL != pcolcreate );
            for ( ; pcolcreate < plastcolcreate; pcolcreate++ )
            {
                Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE_A) );
                Assert( pcolcreate->err >= 0 );     // must have been created successfully
                if ( UtilCmpName( rgsz[iidxseg], pcolcreate->szColumnName ) == 0 )
                    break;
            }

            if ( plastcolcreate != pcolcreate )
            {
                columnidT = pcolcreate->columnid;
                Assert( ( fTemplateTable && FCOLUMNIDTemplateColumn( columnidT ) )
                    || ( !fTemplateTable && !FCOLUMNIDTemplateColumn( columnidT ) ) );
                fEscrow = !!( pcolcreate->grbit & JET_bitColumnEscrowUpdate );
                fMultivalued = !!( pcolcreate->grbit & JET_bitColumnMultiValued );
                fText = FRECTextColumn( pcolcreate->coltyp );
                fBinary = FRECBinaryColumn( pcolcreate->coltyp );
                fLocalizedText = ( fText && usUniCodePage == (USHORT)pcolcreate->cp );
                fEncrypted = !!( pcolcreate->grbit & JET_bitColumnEncrypted );
            }
            else
            {
                FIELD   *pfield = pfieldNil;

                //  initialise to silence compiler warning
                //
                columnidT = JET_columnidNil;

                //  column is not in table, so check template table (if any)
                if ( pfcbNil != pfcbTemplateTable )
                {
                    Assert( !fTemplateTable );
                    CallS( ErrFILEPfieldFromColumnName(
                                ppib,
                                pfcbTemplateTable,
                                rgsz[iidxseg],
                                &pfield,
                                &columnidT ) );
                }

                if ( pfieldNil != pfield )
                {
                    //  must be a template column
                    Assert( FCOLUMNIDTemplateColumn( columnidT ) );
                    Assert( pfcbNil != pfcbTemplateTable );
                    fEscrow = FFIELDEscrowUpdate( pfield->ffield );
                    fMultivalued = FFIELDMultivalued( pfield->ffield );
                    fText = FRECTextColumn( pfield->coltyp );
                    fBinary = FRECBinaryColumn( pfield->coltyp );
                    fLocalizedText = ( fText && usUniCodePage == pfield->cp );
                    fEncrypted = FFIELDEncrypted( pfield->ffield );
                }
                else
                {
                    err = ErrERRCheck( JET_errColumnNotFound );
                    goto HandleError;
                }
            }

            if ( fEncrypted )
            {
                err = ErrERRCheck( JET_errCannotIndexOnEncryptedColumn );
                goto HandleError;
            }

            if ( fEscrow )          //lint !e644
            {
                err = ErrERRCheck( JET_errCannotIndex );
                goto HandleError;
            }

            if ( fMultivalued )     //lint !e644
            {
                if ( idb.FPrimary() )
                {
                    //  primary index cannot be multivalued
                    //
                    err = ErrERRCheck( JET_errIndexInvalidDef );
                    goto HandleError;
                }

                idb.SetFMultivalued();
            }

            if ( ( 0 == iidxseg ) && idb.FTuples() )
            {
                if ( !fText && !fBinary )
                {
                    err = ErrERRCheck( JET_errIndexTuplesTextBinaryColumnsOnly );
                    goto HandleError;
                }
            }

            if ( fLocalizedText )   //lint !e644
            {
                idb.SetFLocalizedText();
            }

            rgidxseg[iidxseg].ResetFlags();

            if ( rgfbDescending[iidxseg] )
                rgidxseg[iidxseg].SetFDescending();

            rgidxseg[iidxseg].SetColumnid( columnidT );
        }

        if( JET_ccolKeyMost < pidxcreate->cConditionalColumn )
        {
            err = ErrERRCheck( JET_errIndexInvalidDef );
            Call( err );
        }

        idb.SetCidxsegConditional( BYTE( pidxcreate->cConditionalColumn ) );
        for ( iidxseg = 0 ; iidxseg < idb.CidxsegConditional(); iidxseg++ )
        {
            COLUMNID                columnidT;
            const CHAR              * const szColumnName    = pidxcreate->rgconditionalcolumn[iidxseg].szColumnName;
            const JET_GRBIT         grbit                   = pidxcreate->rgconditionalcolumn[iidxseg].grbit;
            const JET_COLUMNCREATE_A    * pcolcreate            = ptablecreate->rgcolumncreate;
            const JET_COLUMNCREATE_A    * const plastcolcreate  = pcolcreate + ptablecreate->cColumns;

            C_ASSERT( sizeof( rgidxsegConditional ) / sizeof( rgidxsegConditional[0] ) == JET_ccolKeyMost );

            if( sizeof( JET_CONDITIONALCOLUMN_A ) != pidxcreate->rgconditionalcolumn[iidxseg].cbStruct )
            {
                err = ErrERRCheck( JET_errIndexInvalidDef );
                Call( err );
            }

            if( JET_bitIndexColumnMustBeNonNull != grbit
                && JET_bitIndexColumnMustBeNull != grbit )
            {
                err = ErrERRCheck( JET_errInvalidGrbit );
                Call( err );
            }

            Assert( NULL != pcolcreate );
            for ( ; pcolcreate < plastcolcreate; pcolcreate++ )
            {
                Assert( pcolcreate->cbStruct == sizeof(JET_COLUMNCREATE_A) );
                Assert( pcolcreate->err >= 0 );     // must have been created successfully
                if ( UtilCmpName( szColumnName, pcolcreate->szColumnName ) == 0 )
                    break;
            }

            if ( plastcolcreate != pcolcreate )
            {
                columnidT = pcolcreate->columnid;
                Assert( ( fTemplateTable && FCOLUMNIDTemplateColumn( columnidT ) )
                    || ( !fTemplateTable && !FCOLUMNIDTemplateColumn( columnidT ) ) );
            }
            else
            {
                FIELD   *pfield = pfieldNil;

                //  initialise to silence compiler warning
                //
                columnidT = JET_columnidNil;

                //  column is not in table, so check template table (if any)
                if ( pfcbNil != pfcbTemplateTable )
                {
                    Assert( !fTemplateTable );
                    CallS( ErrFILEPfieldFromColumnName(
                                ppib,
                                pfcbTemplateTable,
                                szColumnName,
                                &pfield,
                                &columnidT ) );
                }

                if ( pfieldNil == pfield )
                {
                    err = ErrERRCheck( JET_errColumnNotFound );
                    goto HandleError;
                }
                else
                {
                    Assert( FCOLUMNIDTemplateColumn( columnidT ) );
                }
            }

            Assert( FCOLUMNIDValid( columnidT ) );

            rgidxsegConditional[iidxseg].ResetFlags();

            if ( JET_bitIndexColumnMustBeNull == grbit )
            {
                rgidxsegConditional[iidxseg].SetFMustBeNull();
            }

            rgidxsegConditional[iidxseg].SetColumnid( columnidT );
        }

        if ( idb.FPrimary() )
        {
            if ( fProcessedPrimary )
            {
                err = ErrERRCheck( JET_errIndexHasPrimary );
                goto HandleError;
            }

            fProcessedPrimary = fTrue;
            pgnoIndexFDP = pgnoTableFDP;
            objidIndex = objidTable;
        }
        else
        {
            Call( ErrDIRCreateDirectory(
                        pfucbTableExtent,
                        CpgInitial( &jsphIndex, g_rgfmp[ ifmp ].CbPage() ),
                        &pgnoIndexFDP,
                        &objidIndex,
                        CPAGE::fPageIndex | ( idb.FUnique() ? 0 : CPAGE::fPageNonUniqueKeys ),
                        fSPUnversionedExtent | ( idb.FUnique() ? 0 : fSPNonUnique ) ) );
            Assert( pgnoIndexFDP != pgnoTableFDP );
            Assert( objidIndex > objidSystemRoot );

            //  objids are monotonically increasing, so an index should
            //  always have higher objid than its table
            Assert( objidIndex > objidTable );
        }

        ptablecreate->cCreated++;
        Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes );

        fSetIndexError = fFalse;

        if ( fTemplateTable )
        {
            Assert( NULL == ptablecreate->szTemplateTableName );

            // If we're creating a template table, this must be a template index.
            idb.SetFTemplateIndex();
        }

        Assert( !idb.FVersioned() );
        Assert( !idb.FVersionedCreate() );

        Call( ErrCATAddTableIndex(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    szIndexName,
                    pgnoIndexFDP,
                    objidIndex,
                    &idb,
                    rgidxseg,
                    rgidxsegConditional,
                    &jsphIndex ) );
    }

HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    Assert( pfucbTableExtent != pfucbNil );
    Assert( pfucbTableExtent->u.pfcb->WRefCount() == 1 );

    //  force the FCB to be uninitialized so it will be purged by DIRClose

    pfucbTableExtent->u.pfcb->Lock();
    pfucbTableExtent->u.pfcb->CreateCompleteErr( errFCBUnusable );
    pfucbTableExtent->u.pfcb->Unlock();

    //  verify that this FUCB will not be defer-closed

    Assert( !FFUCBVersioned( pfucbTableExtent ) );

    //  close the FUCB

    DIRClose( pfucbTableExtent );

    if ( fSetIndexError )
    {
        Assert( err < 0 );
        Assert( pidxcreate != NULL );
        Assert( sizeof( JET_INDEXCREATE3_A ) == pidxcreate->cbStruct );
        Assert( JET_errSuccess == pidxcreate->err );
        pidxcreate->err = err;  //lint !e644
    }

    return err;
}


LOCAL ERR ErrFILEIInheritIndexes(
    PIB *               ppib,
    const IFMP          ifmp,
    const PGNO          pgnoTableFDP,
    const OBJID         objidTable,
    FCB *               pfcbTemplateTable )
{
    ERR             err = JET_errSuccess;
    FUCB            *pfucbTableExtent   = pfucbNil;
    FUCB            *pfucbCatalog       = pfucbNil;
    TDB             *ptdbTemplateTable;
    FCB             *pfcbIndex;
    PGNO            pgnoIndexFDP;
    OBJID           objidIndex;

    // Temp tables and catalogs don't use hierarchical DDL.
    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    Assert( !FCATSystemTable( pgnoTableFDP ) );

    Assert( pfcbTemplateTable->FPrimaryIndex() );
    Assert( pfcbTemplateTable->FTypeTable() );
    Assert( pfcbTemplateTable->FFixedDDL() );
    Assert( pfcbTemplateTable->FTemplateTable() );

    // Open cursor for space navigation
    CallR( ErrDIROpen( ppib, pgnoTableFDP, ifmp, &pfucbTableExtent ) );
    Assert( pfucbNil != pfucbTableExtent );
    Assert( !FFUCBVersioned( pfucbTableExtent ) );  // Verify won't be deferred closed.
    Assert( pfcbNil != pfucbTableExtent->u.pfcb );
    Assert( !pfucbTableExtent->u.pfcb->FInitialized() );
    Assert( pfucbTableExtent->u.pfcb->Pidb() == pidbNil );

    //  force the FCB to be initialized successfully

    pfucbTableExtent->u.pfcb->Lock();
    pfucbTableExtent->u.pfcb->CreateComplete();
    pfucbTableExtent->u.pfcb->Unlock();

    Call( ErrCATOpen( ppib, ifmp, &pfucbCatalog ) );
    Assert( pfucbNil != pfucbCatalog );

    ptdbTemplateTable = pfcbTemplateTable->Ptdb();
    Assert( ptdbNil != ptdbTemplateTable );


    if ( pfcbTemplateTable->Pidb() == pidbNil )
    {
        Assert( pfcbTemplateTable->FSequentialIndex() );
        pfcbIndex = pfcbTemplateTable->PfcbNextIndex();
    }
    else
    {
        Assert( !pfcbTemplateTable->FSequentialIndex() );
        pfcbIndex = pfcbTemplateTable;
    }

    for ( ; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        Assert( pfcbIndex == pfcbTemplateTable || pfcbIndex->FTypeSecondaryIndex() );
        Assert( pfcbIndex->FTemplateIndex() );
        Assert( pfcbIndex->Pidb() != pidbNil );

        IDB idb = *pfcbIndex->Pidb();

        Assert( idb.FTemplateIndex() );
        Assert( !idb.FDerivedIndex() );
        idb.SetFDerivedIndex();
        idb.ResetFTemplateIndex();

        JET_SPACEHINTS jsphTemplate = { 0 };
        pfcbIndex->GetAPISpaceHints( &jsphTemplate );
        Assert( jsphTemplate.cbStruct == sizeof(JET_SPACEHINTS) );
        Assert( jsphTemplate.cbInitial % g_cbPage == 0 );
        JET_SPACEHINTS jsphIndexDensity = { sizeof(JET_SPACEHINTS), 0 };
        jsphIndexDensity.ulInitialDensity = jsphTemplate.ulInitialDensity;
        jsphIndexDensity.cbInitial = ( ( (ULONG)g_cbPage * cpgInitialTreeDefault ) == jsphTemplate.cbInitial ) ? 0 : jsphTemplate.cbInitial;

        if ( idb.FPrimary() )
        {
            Assert( pfcbIndex == pfcbTemplateTable );
            pgnoIndexFDP = pgnoTableFDP;
            objidIndex = objidTable;
        }
        else
        {
            Call( ErrDIRCreateDirectory(
                        pfucbTableExtent,
                        CpgInitial( &jsphTemplate, g_rgfmp[ ifmp ].CbPage() ),
                        &pgnoIndexFDP,
                        &objidIndex,
                        CPAGE::fPageIndex | ( idb.FUnique() ? 0 : CPAGE::fPageNonUniqueKeys ),
                        fSPUnversionedExtent | ( idb.FUnique() ? 0 : fSPNonUnique ) ) );
            Assert( pgnoIndexFDP != pgnoTableFDP );
            Assert( objidIndex > objidSystemRoot );

            //  objids are monotonically increasing, so an index should
            //  always have higher objid than its table
            Assert( objidIndex > objidTable );
        }


        // Can hold pointers into the TDB's memory pool because the
        // template table has fixed DDL (and therefore, a fixed memory pool).
        Assert( pfcbTemplateTable->FFixedDDL() );

        const IDXSEG*   rgidxseg;
        const IDXSEG*   rgidxsegConditional;
        CHAR*           szIndexName;

        Assert( idb.Cidxseg() > 0 );
        rgidxseg = PidxsegIDBGetIdxSeg( pfcbIndex->Pidb(), pfcbTemplateTable->Ptdb() );
        rgidxsegConditional = PidxsegIDBGetIdxSegConditional( pfcbIndex->Pidb(), pfcbTemplateTable->Ptdb() );
        szIndexName = ptdbTemplateTable->SzIndexName( idb.ItagIndexName() );

        Assert( !idb.FVersioned() );
        Assert( !idb.FVersionedCreate() );

        Call( ErrCATAddTableIndex(
                    ppib,
                    pfucbCatalog,
                    objidTable,
                    szIndexName,
                    pgnoIndexFDP,
                    objidIndex,
                    &idb,
                    rgidxseg,
                    rgidxsegConditional,
                    &jsphIndexDensity ) );
    }

HandleError:
    if ( pfucbNil != pfucbCatalog )
    {
        CallS( ErrCATClose( ppib, pfucbCatalog ) );
    }

    Assert( pfucbTableExtent != pfucbNil );
    Assert( pfucbTableExtent->u.pfcb->WRefCount() == 1 );

    //  force the FCB to be uninitialized so it will be purged by DIRClose

    pfucbTableExtent->u.pfcb->Lock();
    pfucbTableExtent->u.pfcb->CreateCompleteErr( errFCBUnusable );
    pfucbTableExtent->u.pfcb->Unlock();

    //  verify that this FUCB will not be defer-closed

    Assert( !FFUCBVersioned( pfucbTableExtent ) );

    //  close the FUCB

    DIRClose( pfucbTableExtent );

    return err;
}


//  ================================================================
LOCAL ERR ErrFILEIValidateCallback(
    const PIB * const ppib,
    const IFMP ifmp,
    const OBJID objidTable,
    const JET_CBTYP cbtyp,
    const CHAR * const szCallback )
//  ================================================================
{
    if( 0 == cbtyp )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if( NULL == szCallback )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if( strlen( szCallback ) >= JET_cbColumnMost )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    return JET_errSuccess;
}


//  ================================================================
LOCAL ERR ErrFILEICreateCallbacks(
    PIB * const ppib,
    const IFMP ifmp,
    JET_TABLECREATE5_A * const ptablecreate,
    const OBJID objidTable )
//  ================================================================
{
    Assert( sizeof( JET_TABLECREATE5_A ) == ptablecreate->cbStruct );

    ERR err = JET_errSuccess;

    Call( ErrFILEIValidateCallback( ppib, ifmp, objidTable, ptablecreate->cbtyp, ptablecreate->szCallback ) );
    if (    !BoolParam( PinstFromPpib( ppib ), JET_paramDisableCallbacks ) &&
            BoolParam( PinstFromPpib( ppib ), JET_paramEnablePersistedCallbacks ) )
    {
        JET_CALLBACK callback = NULL;
        Call( ErrCALLBACKResolve( ptablecreate->szCallback, &callback ) );
        Assert( NULL != callback );
    }
    Call( ErrCATAddTableCallback( ppib, ifmp, objidTable, ptablecreate->cbtyp, ptablecreate->szCallback ) );
    ++(ptablecreate->cCreated);
    Assert( ptablecreate->cCreated <= 1 + ptablecreate->cColumns + ptablecreate->cIndexes + 1 );

HandleError:
    return err;
}


LOCAL ERR ErrFILECreateTableProcessSpaceHints(
    _In_    const LONG                          cbPageSize,
    _In_    const JET_TABLECREATE5_A * const    ptablecreate,
    _Out_   JET_SPACEHINTS * const              pjsphPrimaryAlloc
    )
{
    ERR err = JET_errSuccess;
    JET_INDEXCREATE3_A * pidxcreate;
    JET_INDEXCREATE3_A * pidxcreateNext;

    JET_INDEXCREATE3_A * pidxprimary = NULL;

    Assert( ptablecreate && pjsphPrimaryAlloc );

    ULONG ulTotalPagesRequired = 0;

    SIZE_T iIndex;
    pidxcreate      = ptablecreate->rgindexcreate;
    for( iIndex = 0; iIndex < ptablecreate->cIndexes; iIndex++, pidxcreate = pidxcreateNext )
    {
        pidxcreateNext  = (JET_INDEXCREATE3_A *)((BYTE *)( pidxcreate ) + pidxcreate->cbStruct);

        Assert( sizeof(JET_INDEXCREATE3_A) == pidxcreate->cbStruct );

        if ( pidxcreate->pSpacehints )
        {
            JET_SPACEHINTS spacehintsT = *( pidxcreate->pSpacehints );
            Call( ErrCATCheckJetSpaceHints( cbPageSize, &spacehintsT, fTrue ) );
            Assert( spacehintsT.cbStruct == sizeof(JET_SPACEHINTS) );

            
            ulTotalPagesRequired += CpgInitial( &spacehintsT, cbPageSize );
        }
        else
        {
            ulTotalPagesRequired++;
        }

        if ( pidxcreate->grbit & JET_bitIndexPrimary )
        {
            //  We can't assert this, because the check for this isn't until later.
            // Assert( NULL != pidxprimary );
            pidxprimary = pidxcreate;
        }
    }

    // It is improper API usage to specify sequential space hints when there is a
    // primary index specified. We could trigger this error in ESENT and
    // break downlevel code. Otherwise, below the sequential hints will be ignored if there 
    // is a primary index specified.
    if ( pidxprimary && ptablecreate->pSeqSpacehints )
    {
        Call( ErrERRCheck( JET_errSpaceHintsInvalid ) );
    }

    if ( pidxprimary )
    {
        //  We do not need to consume CpgInitial / cbInitial here because it was
        //  factored in in the iIndex loop above ...
        if ( pidxprimary->pSpacehints )
        {
            Assert( pidxprimary->pSpacehints->cbStruct == sizeof(JET_SPACEHINTS) );
            *pjsphPrimaryAlloc = *(pidxprimary->pSpacehints);
            CallS( ErrCATCheckJetSpaceHints( cbPageSize, pjsphPrimaryAlloc, fTrue ) );
        }
        else
        {
            *pjsphPrimaryAlloc = *PSystemSpaceHints(eJSPHDefaultUserTable);

            //  Consume default table density if set.

            pjsphPrimaryAlloc->ulInitialDensity = pidxprimary->ulDensity ?
                                                    pidxprimary->ulDensity :
                                                    ( ptablecreate->ulDensity ? ptablecreate->ulDensity : ulFILEDefaultDensity );

            //  This can error out if the density is bad
            Call( ErrCATCheckJetSpaceHints( cbPageSize, pjsphPrimaryAlloc, fFalse ) );
        }

    }
    else if ( ptablecreate->pSeqSpacehints )
    {
        Assert( NULL == pidxprimary );

        *pjsphPrimaryAlloc = *(ptablecreate->pSeqSpacehints);
        Call( ErrCATCheckJetSpaceHints( cbPageSize, pjsphPrimaryAlloc, fTrue ) );
        Assert( ptablecreate->pSeqSpacehints->cbStruct == sizeof(JET_SPACEHINTS) );
        
        ulTotalPagesRequired += CpgInitial( pjsphPrimaryAlloc, cbPageSize );
    }
    else
    {
        *pjsphPrimaryAlloc = *PSystemSpaceHints(eJSPHDefaultUserTable);

        //  Consume default table density if set.
        pjsphPrimaryAlloc->ulInitialDensity = ptablecreate->ulDensity ? ptablecreate->ulDensity : ulFILEDefaultDensity;

        //  This can error out if the density is bad
        Call( ErrCATCheckJetSpaceHints( cbPageSize, pjsphPrimaryAlloc, fFalse ) );

        ulTotalPagesRequired += CpgInitial( pjsphPrimaryAlloc, cbPageSize );
    }

    if ( ptablecreate->pLVSpacehints )
    {
        JET_SPACEHINTS spacehintsT = *( ptablecreate->pLVSpacehints );
        Call( ErrCATCheckJetSpaceHints( cbPageSize, &spacehintsT, fTrue ) );
    }

    Assert( 1 <= ulTotalPagesRequired );

    if( ptablecreate->ulPages > ulTotalPagesRequired )
    {
        //  If table asking for more pages, upgrade required pages to total table ask.
        ulTotalPagesRequired = ptablecreate->ulPages;
    }

    //  If the primary index doesn't represent the total pages, fix that.
    if( ( ulTotalPagesRequired * g_cbPage ) > pjsphPrimaryAlloc->cbInitial )
    {
        pjsphPrimaryAlloc->cbInitial = ulTotalPagesRequired * g_cbPage;
    }

    //  normalize this to 0x0 if default (cpgInitialTreeDefault=1 page).
    if ( ( (ULONG)g_cbPage * cpgInitialTreeDefault ) == pjsphPrimaryAlloc->cbInitial )
    {
        pjsphPrimaryAlloc->cbInitial = 0x0;
    }

HandleError:

    return err;
}

//+API
// ErrFILECreateTable
// =========================================================================
// ERR ErrFILECreateTable( PIB *ppib, IFMP ifmp, CHAR *szName,
//      ULONG ulPages, ULONG ulDensity, FUCB **ppfucb )
//
// Create file with pathname szName.  Created file will have no fields or
// indexes defined (and so will be a "sequential" file ).
//
// PARAMETERS
//                  ppib            PIB of user
//                  ifmp            database id
//                  szName          path name of new file
//                  ulPages         initial page allocation for file
//                  ulDensity       initial loading density
//                  ppfucb          Exclusively locked FUCB on new file
// RETURNS      Error codes from DIRMAN or
//                   JET_errSuccess     Everything worked OK
//                  -DensityInvalid     Density parameter not valid
//                  -TableDuplicate     A file already exists with the path given
// COMMENTS     A transaction is wrapped around this function.  Thus, any
//              work done will be undone if a failure occurs.
// SEE ALSO     ErrIsamAddColumn, ErrIsamCreateIndex, ErrIsamDeleteTable
//-
ERR ErrFILECreateTable( PIB *ppib, IFMP ifmp, JET_TABLECREATE5_A *ptablecreate, UINT fSPFlags )
{
    ERR         err;
    CHAR        szTable[JET_cbNameMost+1];
    FCB         *pfcb = pfcbNil;
    FUCB        *pfucb = pfucbNil;
    FDPINFO     fdpinfo;
    VER         *pver;
    BOOL        fOpenedTable    = fFalse;
    BOOL        fCreatedRCE     = fFalse;

    Expected( 0 == (fSPFlags & ~fSPMultipleExtent)); // Only expected flag (if there is a flag) is MultipleExtent.
    FMP::AssertVALIDIFMP( ifmp );

    Assert( sizeof(JET_TABLECREATE5_A) == ptablecreate->cbStruct );

    ptablecreate->cCreated = 0;

    //  check parms
    //
    CheckPIB( ppib );
    CheckDBID( ppib, ifmp );
    CallR( ErrUTILCheckName( szTable, ptablecreate->szTableName, JET_cbNameMost+1 ) );

    Expected( !( ptablecreate->grbit & JET_bitTableCreateSystemTable )
        || FOLDSystemTable( szTable )
        || FSCANSystemTable( szTable )
        || FCATObjidsTable( szTable )
        || MSysDBM::FIsSystemTable( szTable )
        || FKVPTestTable( szTable )
        || FCATExtentPageCountCacheTable( szTable )
        || FCATLocalesTable( szTable ) );

    const BOOL  fTemplateTable      = !!( ptablecreate->grbit & JET_bitTableCreateTemplateTable );
    const BOOL  fDerived            = ( ptablecreate->szTemplateTableName != NULL );
    FCB         *pfcbTemplateTable  = pfcbNil;

    OSTraceFMP(
        ifmp,
        JET_tracetagDDLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] creating table '%s' on database=['%ws':0x%x] [szTemplate=%s,cCols=0x%x,cIdxs=0x%x,ulPages=0x%x,ulDensity=0x%x,szCallback=%s,cbtyp=0x%x,grbit=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szTable,
            g_rgfmp[ifmp].WszDatabaseName(),
            (ULONG)ifmp,
            OSFormatString( ptablecreate->szTemplateTableName ),
            ptablecreate->cColumns,
            ptablecreate->cIndexes,
            ptablecreate->ulPages,
            ptablecreate->ulDensity,
            OSFormatString( ptablecreate->szCallback ),
            ptablecreate->cbtyp,
            ptablecreate->grbit ) );

    if ( fDerived )
    {
        FUCB    *pfucbTemplateTable;

        Assert( !FFMPIsTempDB( ifmp ) );

        //  UNDONE: nested hierarchical DDL not yet supported
        //
        Assert( !fTemplateTable );

        //  bring base table FCB into memory
        //
        CallR( ErrFILEOpenTable(
            ppib,
            ifmp,
            &pfucbTemplateTable,
            ptablecreate->szTemplateTableName,
            JET_bitTableReadOnly ) );

        Assert( pfcbNil == pfcbTemplateTable );
        if ( pfucbTemplateTable->u.pfcb->FTemplateTable() )
        {
            Assert( pfucbTemplateTable->u.pfcb->FFixedDDL() );
            pfcbTemplateTable = pfucbTemplateTable->u.pfcb;
        }

        if ( pfcbNil != pfcbTemplateTable )
        {
            //  flag the template as static, i.e., it can't be opened for modification anymore
            //  need the lock because it's a bit-field, so read/write must be atomic
            //  NOTE: Once upon a time, the bit fields needed this lock.  Not anymore; the bits
            //  are set atomically.  Leaving the lock in place now just out of worry they have
            //  some other unintended sync impact.
            pfcbTemplateTable->Lock();
            pfcbTemplateTable->SetTemplateStatic();
            pfcbTemplateTable->Unlock();

            //  Increment ref count so that template table does not get purged
            pfcbTemplateTable->IncrementRefCount();
        }

        //  close cursor.
        CallS( ErrFILECloseTable( ppib, pfucbTemplateTable ) );

        if ( pfcbNil == pfcbTemplateTable )
        {
            return ErrERRCheck( JET_errDDLNotInheritable );
        }
    }

    //
    //  Calculate required space, validate any space hints .... 
    //

    JET_SPACEHINTS jsphPrimaryAlloc;
    CallR( ErrFILECreateTableProcessSpaceHints( g_rgfmp[ ifmp ].CbPage(), ptablecreate, &jsphPrimaryAlloc ) );

    //
    //  Making initial space allocation and updating catalog.
    //

    CallR( ErrDIRBeginTransaction( ppib, 42277, NO_GRBIT ) );

    //  check to make sure temp db created if creating a temporary table
    //
    Assert( !FFMPIsTempDB( ifmp ) || PinstFromPpib( ppib )->m_fTempDBCreated );

    //  allocate cursor
    //
    Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucb ) );
    Call( ErrDIRCreateDirectory(
                pfucb,
                CpgInitial( &jsphPrimaryAlloc, g_rgfmp[ ifmp ].CbPage() ),
                &fdpinfo.pgnoFDP,
                &fdpinfo.objidFDP,
                CPAGE::fPagePrimary,
                fSPFlags | ( FFMPIsTempDB( ifmp ) ? fSPUnversionedExtent : 0 ) ) );    // For temp. tables, create unversioned extents
    DIRClose( pfucb );
    pfucb = pfucbNil;

    Assert( ptablecreate->cCreated == 0 );
    ptablecreate->cCreated = 1;

    if ( FFMPIsTempDB( ifmp ) )
    {
        //  don't currently support creation of columns/indexes for temp.
        //  tables via this API.
        Assert( 0 == ptablecreate->cColumns );
        Assert( 0 == ptablecreate->cIndexes );
        Assert( NULL == ptablecreate->szCallback );
    }

    else
    {
        Assert( !FCATSystemTable( fdpinfo.pgnoFDP ) );
        Assert( fdpinfo.objidFDP > objidSystemRoot );

        //  insert record in MSysObjects
        //
        JET_GRBIT   grbit;
        if ( fTemplateTable )
        {
            Assert( !fDerived );    // UNDONE: Nested hierarchical DDL not yet supported
            Assert( !( ptablecreate->grbit & JET_bitTableCreateSystemTable ) );
            grbit = ( JET_bitObjectTableTemplate | JET_bitObjectTableFixedDDL );
        }
        else
        {
            Assert( !( ptablecreate->grbit & JET_bitTableCreateNoFixedVarColumnsInDerivedTables ) );
            grbit = 0;
            if ( fDerived )
                grbit |= JET_bitObjectTableDerived;
            if ( ptablecreate->grbit & JET_bitTableCreateFixedDDL )
                grbit |= JET_bitObjectTableFixedDDL;
            if ( ptablecreate->grbit & JET_bitTableCreateSystemTable )
            {
                //  if calling CreateTable(), this must be a dynamic system table
                grbit |= JET_bitObjectSystemDynamic;
                Assert( FOLDSystemTable( szTable )
                    || FSCANSystemTable( szTable )
                    || FCATObjidsTable( szTable )
                    || FCATExtentPageCountCacheTable( szTable )
                    || MSysDBM::FIsSystemTable( szTable )
                    || FKVPTestTable( szTable )
                    || FCATLocalesTable( szTable ) );
            }
        }

#ifdef DEBUG
        if ( fDerived )
        {
            Assert( pfcbNil != pfcbTemplateTable );
            Assert( NULL != ptablecreate->szTemplateTableName );
        }
        else
        {
            Assert( pfcbNil == pfcbTemplateTable );
            Assert( NULL == ptablecreate->szTemplateTableName );
        }
#endif

        //  we don't want to modify the LV space hints so copy them here
        JET_SPACEHINTS jsphLV;
        if( ptablecreate->pLVSpacehints )
        {
            jsphLV = *(ptablecreate->pLVSpacehints);
            CallS( ErrCATCheckJetSpaceHints( g_rgfmp[ ifmp ].CbPage(), &jsphLV, fTrue ) );    // validated in ErrFILECreateTableProcessSpaceHints
        }

        Call( ErrCATAddTable(
                    ppib,
                    ifmp,
                    fdpinfo.pgnoFDP,
                    fdpinfo.objidFDP,
                    szTable,
                    ptablecreate->szTemplateTableName,
                    grbit,
                    &jsphPrimaryAlloc,  // contains ulTableDensity,
                    ptablecreate->cbSeparateLV,
                    ptablecreate->pLVSpacehints ? &jsphLV : NULL,
                    ptablecreate->cbLVChunkMax ) );

        //  create columns and indexes as necessary.

        if ( fDerived )
        {
            Call( ErrFILEIInheritIndexes(
                    ppib,
                    ifmp,
                    fdpinfo.pgnoFDP,
                    fdpinfo.objidFDP,
                    pfcbTemplateTable ) );
        }
        if ( ptablecreate->cColumns > 0 )
        {
            Call( ErrFILEIAddColumns(
                    ppib,
                    ifmp,
                    ptablecreate,
                    fdpinfo.objidFDP,
                    pfcbTemplateTable ) );
        }
        Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns );

        if ( ptablecreate->cIndexes > 0 )
        {
            Call( ErrFILEICreateIndexes(
                    ppib,
                    ifmp,
                    ptablecreate,
                    fdpinfo.pgnoFDP,
                    fdpinfo.objidFDP,
                    &jsphPrimaryAlloc,
                    pfcbTemplateTable ) );
        }

        if ( NULL != ptablecreate->szCallback )
        {
            Call( ErrFILEICreateCallbacks(
                    ppib,
                    ifmp,
                    ptablecreate,
                    fdpinfo.objidFDP ) );
        }

    }


    Assert( ptablecreate->cCreated == 1 + ptablecreate->cColumns + ptablecreate->cIndexes + ( ptablecreate->szCallback ? 1 : 0 ) );


    //  open table in exclusive mode, for output parameter
    //
    Call( ErrFILEOpenTable(
            ppib,
            ifmp,
            &pfucb,
            szTable,
            JET_bitTableCreate|JET_bitTableDenyRead,
            &fdpinfo ) );
    fOpenedTable = fTrue;

    pfcb = pfucb->u.pfcb;
    Assert( pfcb != pfcbNil );

    Assert( pfcbTemplateTable == pfcbNil || pfcb->Ptdb()->FidAutoincrement() == pfcbTemplateTable->Ptdb()->FidAutoincrement() );
    if ( pfcb->Ptdb()->FidAutoincrement() != 0 )
    {
        const BOOL fExternalHeaderNewFormatEnabled =
            ( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( efvExtHdrRootFieldAutoInc ) >= JET_errSuccess );
        if ( fExternalHeaderNewFormatEnabled )
        {
            Call( ErrRECInitAutoIncSpace( pfucb, 0 ) );
        }
    }

#ifdef DEBUG
    if ( 0 == ptablecreate->cColumns )
    {
        TDB *ptdb = pfcb->Ptdb();
        Assert( ptdb->FidFixedLast() == ptdb->FidFixedFirst()-1 );
        Assert( ptdb->FidVarLast() == ptdb->FidVarFirst()-1 );
        Assert( ptdb->FidTaggedLast() == ptdb->FidTaggedFirst()-1 );
    }
#endif

    // Allow DDL until table (cursor) is closed.
    if ( pfcb->FFixedDDL() && !fTemplateTable )
    {
        FUCBSetPermitDDL( pfucb );
    }

    Assert( pfcb->FInitialized() );
    Assert( pfcb->FTypeTable() || pfcb->FTypeTemporaryTable() );
    Assert( pfcb->FPrimaryIndex() );

    if ( FFMPIsTempDB( ifmp ) )
    {
        pfcb->SetSpaceHints( &jsphPrimaryAlloc );
        Assert( ( ptablecreate->cbSeparateLV == 0 ) || ( pfcb->Ptdb()->CbPreferredIntrinsicLV() == (ULONG)ptablecreate->cbSeparateLV ) );
    }
    
    // If this is not a derived table, we expect the space hints to be
    // unchanged. A derived table's space hints may be changed (from the default provided hints) by 
    // inheriting the space hints from the template table, unless there are space hints explicitly set
    // on the derived table, in which case the derived set wins.
#ifdef DEBUG
    JET_SPACEHINTS jsphCheck;
    pfcb->GetAPISpaceHints( &jsphCheck );

    // If it's not a derived table or new hints were specified, they should
    // have been set.
    if ( !fDerived || ptablecreate->pSeqSpacehints )
    {
        Assert( 0 == memcmp( &jsphCheck, &jsphPrimaryAlloc, sizeof(jsphCheck) ) );
    }
#endif

    //  flag FCB as uncommitted for unversioned DML operations
    //
    if ( !g_rgfmp[ ifmp ].FVersioningOff() )
    {
        pfcb->Lock();
        pfcb->SetUncommitted();
        pfcb->Unlock();
    }

    pver = PverFromIfmp( pfucb->ifmp );
    Call( ErrFaultInjection( 52640 ) );
    Call( pver->ErrVERFlag( pfucb, operCreateTable, NULL, 0 ) );
    fCreatedRCE = fTrue;

    Call( ErrFaultInjection( 46496 ) );
    Call( ErrDIRCommitTransaction( ppib, 0 ) );

    if ( pfcbTemplateTable != pfcbNil )
    {
        pfcbTemplateTable->Release();
        pfcbTemplateTable = pfcbNil;
    }

    //  internally, we use tableid and pfucb interchangeably
    ptablecreate->tableid = (JET_TABLEID)pfucb;

    return JET_errSuccess;

HandleError:
    OSTraceFMP(
        ifmp,
        JET_tracetagDDLWrite,
        OSFormat(
            "Finished creation of table '%s' with error %d (0x%x)",
            szTable,
            err,
            err ) );

    if ( fOpenedTable )
    {
        Assert( pfcb != pfcbNil );

        //  if we didn't insert operCreateTable, make sure that
        //  we'll purge the FCB  on close (because rollback won't do it)
        //
        if ( !fCreatedRCE )
        {
            pfcb->Lock();
            pfcb->SetTryPurgeOnClose();
            pfcb->Unlock();
        }
        CallS( ErrFILECloseTable( ppib, pfucb ) );
    }

    //  pfucb is closed by rollback if necessary
    //
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    if ( pfcbTemplateTable != pfcbNil )
    {
        pfcbTemplateTable->Release();
        pfcbTemplateTable = pfcbNil;
    }

    //  reset return variable if close table via rollback
    //
    ptablecreate->tableid = JET_tableidNil;

    return err;
}


//====================================================
// Determine field "mode" as follows:
// if ("long" textual || JET_bitColumnTagged given ) ==> TAGGED
// else if (numeric type || JET_bitColumnFixed given ) ==> FIXED
// else ==> VARIABLE
//====================================================
ERR ErrFILEGetNextColumnid(
    const JET_COLTYP    coltyp,
    const JET_GRBIT     grbit,
    const BOOL          fTemplateTable,
    TCIB                *ptcib,
    COLUMNID            *pcolumnid )
{
    FID                 fid;
    FID                 fidMost;

    if ( ( grbit & JET_bitColumnTagged ) || FRECLongValue( coltyp ) )
    {
        Assert( ptcib->fidTaggedLast.FTaggedNone() || ptcib->fidTaggedLast.FTagged() );
        fid = ++(ptcib->fidTaggedLast);
        fidMost = fidTaggedMost;
    }
    else if ( ( grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( coltyp ) )
    {
        Assert( ptcib->fidFixedLast.FFixedNone() || ptcib->fidFixedLast.FFixed() );
        fid = ++(ptcib->fidFixedLast);
        fidMost = fidFixedMost;
    }
    else
    {
        Assert( !( grbit & JET_bitColumnTagged ) );
        Assert( !( grbit & JET_bitColumnFixed ) );
        Assert( JET_coltypText == coltyp || JET_coltypBinary == coltyp );
        Assert( ptcib->fidVarLast.FVarNone() || ptcib->fidVarLast.FVar() );
        fid = ++(ptcib->fidVarLast);
        fidMost = fidVarMost;
    }
    if ( fid > fidMost )
    {
        return ErrERRCheck( JET_errTooManyColumns );
    }
    *pcolumnid = ColumnidOfFid( fid, fTemplateTable );
    return JET_errSuccess;
}

INLINE ERR ErrFILEIUpdateAutoInc( PIB *ppib, FUCB *pfucb )
{
    ERR             err;
    QWORD           qwT             = 1;
    TDB             * const ptdb    = pfucb->u.pfcb->Ptdb();
    const BOOL      f8BytesAutoInc  = ptdb->F8BytesAutoInc();
    const BOOL      fTemplateColumn = ptdb->FFixedTemplateColumn( ptdb->FidAutoincrement() );
    const COLUMNID  columnidT       = ColumnidOfFid( ptdb->FidAutoincrement(), fTemplateColumn );

    // set column does not allow sets over an
    // autoincrement column.  To work around this,
    // temporarily disable AutoInc column.
    Assert( pfucb->u.pfcb->FDomainDenyReadByUs( ppib ) );
    ptdb->ResetFidAutoincrement();
    BOOL fFidReset = fTrue;

    err = ErrIsamMove( ppib, pfucb, JET_MoveFirst, NO_GRBIT );
    Assert( err <= 0 );
    while ( JET_errSuccess == err )
    {
        Call( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepReplaceNoLock ) );

        if ( !f8BytesAutoInc )
        {
            ULONG ulT;
            ulT = (ULONG)qwT;
            Call( ErrIsamSetColumn( ppib,
                    pfucb,
                    columnidT,
                    (BYTE *)&ulT,
                    sizeof(ulT),
                    NO_GRBIT,
                    NULL ) );
        }
        else
        {
            Call( ErrIsamSetColumn( ppib,
                    pfucb,
                    columnidT,
                    (BYTE *)&qwT,
                    sizeof(qwT),
                    NO_GRBIT,
                    NULL ) );
        }
        Call( ErrIsamUpdate( ppib, pfucb, NULL, 0, NULL, NO_GRBIT ) );

        qwT++;
        err = ErrIsamMove( ppib, pfucb, JET_MoveNext, NO_GRBIT );
        Assert( err <= 0 );
    }

    ptdb->SetFidAutoincrement( FidOfColumnid( columnidT ), f8BytesAutoInc );
    fFidReset = fFalse;

    Assert( err < 0 );
    if ( JET_errNoCurrentRecord == err )
    {
        // Since we just added all the auto-inc values, we can try to initialize the auto-inc ext-hdr on the root, if configured
        const BOOL fExternalHeaderNewFormatEnabled =
            ( g_rgfmp[pfucb->ifmp].ErrDBFormatFeatureEnabled( efvExtHdrRootFieldAutoInc ) >= JET_errSuccess );
        if ( fExternalHeaderNewFormatEnabled &&
             // May fail if root page does not have enough space for the auto-inc ext-hdr
             ErrRECInitAutoIncSpace( pfucb, qwT - 1 ) >= JET_errSuccess )
        {
        }
        else
        {
            Assert( ptdb->QwAutoincrement() == 0 );
            Call( ptdb->ErrInitAutoInc( pfucb, qwT ) );
            Assert( ptdb->QwAutoincrement() == qwT );
        }
        err = JET_errSuccess;
    }

HandleError:
    if ( fFidReset )
    {
        ptdb->SetFidAutoincrement( FidOfColumnid( columnidT ), f8BytesAutoInc );
    }

    return err;
}

//  ================================================================
ERR ErrFILEICheckForDuplicateColumn(
        PIB * const ppib,
        FCB * const pfcb,
        const CHAR * const szName )
//  ================================================================
//
//  Returns JET_errColumnDuplicate if the given column exists, 
//  JET_errSuccess if it doesn't or an error code.
//
//-
{

    FIELD *         pfield = pfieldNil;
    JET_COLUMNID    columnid;

    pfcb->EnterDML();
    
    //  WARNING: The following function does a LeaveDML() on error
    ERR err = ErrFILEPfieldFromColumnName(
        ppib,
        pfcb,
        szName,
        &pfield,
        &columnid );

    if( err >= JET_errSuccess )
    {
        pfcb->LeaveDML();
    }

    // it is possible for ErrFILEPfieldFromColumnName to return JET_errColumnNotFound
    // in this case it isn't actually an error as we are checking for the non-existence 
    // of the column.   
    if ( JET_errColumnNotFound == err )
    {
        err = JET_errSuccess;
    }
    else if ( err >= JET_errSuccess && pfieldNil != pfield )
    {
        err = ErrERRCheck( JET_errColumnDuplicate );
    }
    
    return err;
}


//+API
// ErrIsamAddColumn
// ========================================================================
// ERR ErrIsamAddColumn(
//      PIB             *ppib;          // IN PIB of user
//      FUCB            *pfucb;         // IN Exclusively opened FUCB on file
//      CHAR            *szName;        // IN name of new field
//      JET_COLUMNDEF   *pcolumndef     // IN definition of column added
//      BYTE            *pvDefault      // IN Default value of column
//      ULONG           cbDefault       // IN length of Default value
//      JET_COLUMNID    *pcolumnid )    // OUT columnid of added column
//
// Creates a new field definition for a file.
//
// PARAMETERS
//              pcolumndef->coltyp          data type of new field, see jet.h
//              pcolumndef->grbit           field describing flags:
//                  VALUE               MEANING
//                  ========================================
//                  JET_bitColumnNotNULL            Indicates that the field may
//                                                  not take on NULL values.
//                  JET_bitColumnTagged             The field is a "tagged" field.
//                  JET_bitColumnVersion            The field is a version field
//
// RETURNS      JET_errSuccess          Everything worked OK.
//                  -TaggedDefault          A default value was specified
//                                              for a tagged field.
//                  -ColumnDuplicate        There is already a field
//                                              defined for the name given.
// COMMENTS
//      There must not be anyone currently using the file, unless
//      the ErrIsamAddColumn is at level 0 [when non-exclusive ErrIsamAddColumn works].
//      A transaction is wrapped around this function.  Thus, any
//      work done will be undone if a failure occurs.
//      Transaction logging is turned off for temporary files.
//
// SEE ALSO     ErrIsamCreateTable, ErrIsamCreateIndex
//-
ERR VTAPI ErrIsamAddColumn(
    JET_SESID           sesid,
    JET_VTID            vtid,
    const CHAR          * const szName,
    const JET_COLUMNDEF * const pcolumndef,
    const VOID          * const pvDefault,
    ULONG               cbDefault,
    JET_COLUMNID        * const pcolumnid )
{
    ERR                 err;
    PIB                 * const ppib                = reinterpret_cast<PIB *>( sesid );
    FUCB                * const pfucb               = reinterpret_cast<FUCB *>( vtid );
    TCIB                tcib;
    CHAR                szFieldName[JET_cbNameMost+1];
    FCB                 *pfcb;
    TDB                 *ptdb;
    JET_COLUMNID        columnid;
    VERADDCOLUMN        veraddcolumn;
    FIELD               field;
    BOOL                fMaxTruncated;
    ULONG               cFixed, cVar, cTagged;
    ULONG               cbFieldsTotal, cbFieldsUsed;
    BOOL                fInCritDDL                  = fFalse;
    FIELD               *pfieldNew                  = pfieldNil;
    BOOL                fUnversioned                = fFalse;
    BOOL                fSetVersioned               = fFalse;
    VER                 *pver;

    //  this is the data that will actually be inserted into the catalog
    const VOID          *pvDefaultAdd               = pvDefault;
    ULONG               cbDefaultAdd                = cbDefault;
    CHAR                *szCallbackAdd              = NULL;
    const VOID          *pvUserDataAdd              = NULL;
    ULONG               cbUserDataAdd               = 0;
    LONG                cbRecordMost                = REC::CbRecordMost( pfucb );

    //  check paramaters
    //
    CallR( ErrPIBCheck( ppib ) );
    Assert( pfucb != pfucbNil );
    CheckTable( ppib, pfucb );
    FMP::AssertVALIDIFMP( pfucb->ifmp );

    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    //  ensure that table is updatable
    //
    CallR( ErrFUCBCheckUpdatable( pfucb ) );
    CallR( ErrPIBCheckUpdatable( ppib ) );

    pfcb = pfucb->u.pfcb;
    Assert( pfcb->WRefCount() > 0 );
    Assert( pfcb->FTypeTable() );   // Temp. tables have fixed DDL.

    ptdb = pfcb->Ptdb();
    Assert( ptdbNil != ptdb );

    OSTraceFMP(
        pfcb->Ifmp(),
        JET_tracetagDDLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] adding column %s to objid=[0x%x:0x%x] [coltyp=0x%x,cp=0x%x,cbMax=0x%x,pvDefault=0x%p,cbDefault=0x%x,grbit=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            OSFormatString( szName ),
            (ULONG)pfcb->Ifmp(),
            pfcb->ObjidFDP(),
            pcolumndef->coltyp,
            pcolumndef->cp,
            pcolumndef->cbMax,
            pvDefault,
            cbDefault,
            pcolumndef->grbit ) );

    if ( pfcb->FFixedDDL() )
    {
        // Check FixedDDL override.
        if ( !FFUCBPermitDDL( pfucb ) )
        {
            return ErrERRCheck( JET_errFixedDDL );
        }

        // If DDL temporarily permitted, we must have exclusive use of the table.
        Assert( pfcb->FDomainDenyReadByUs( ppib ) );
    }

    if ( pcolumndef->cbStruct < sizeof(JET_COLUMNDEF) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( pcolumndef->grbit & JET_bitColumnUnversioned )
    {
        if ( !FFUCBDenyRead( pfucb ) )
        {
            if ( ppib->Level() != 0 )
            {
                AssertSz( fFalse, "Must not be in transaction for unversioned AddColumn." );
                err = ErrERRCheck( JET_errInTransaction );
                return err;
            }
            fUnversioned = fTrue;
        }
    }

    //  UNDONE: interpret pvDefault of NULL for NULL value, and
    //          cbDefault == 0 and pvDefault != NULL as set to
    //          zero length.
    Assert( cbDefault == 0 || pvDefault != NULL );

    memset( &field, 0, sizeof(FIELD) );
    field.coltyp = FIELD_COLTYP( pcolumndef->coltyp );
    field.cbMaxLen = pcolumndef->cbMax;
    field.cp = pcolumndef->cp;

    CallR( ErrFILEIValidateAddColumn(
                pfucb->ifmp,
                szName,
                szFieldName,
                &field,
                pcolumndef->grbit,
                ( pvDefault ? cbDefault : 0 ),
                pfcb->FDomainDenyReadByUs( ppib ),
                ptdb->PfcbTemplateTable() ) );
    fMaxTruncated = ( JET_wrnColumnMaxTruncated == err );

    //  at this time, we only support adding tagged columns to template tables
    //  if we ever add support for adding fixed and variable columns to template tables,
    //  JET_bitTableCreateNoFixedVarColumnsInDerivedTables would also need to be supported
    //  to prevent derived tables from adding columns whose FIDs conflict with those from
    //  the template
    if ( pfcb->FTemplateTable() && !( ( pcolumndef->grbit & JET_bitColumnTagged ) || FRECLongValue( field.coltyp ) ) )
    {
        return ErrERRCheck( JET_errFixedDDL );
    }

    if ( fUnversioned )
    {
        CallR( ErrFILEInsertIntoUnverColumnList( pfcb, szFieldName ) );
    }

    if ( FFIELDEscrowUpdate( field.ffield ) )
    {
        //  see the comment int ErrFILEIValidateAddColumn to find out why we
        //  cannot add an escrow column to a table with any records in it
        Assert( pfcb->FDomainDenyReadByUs( ppib ) );
        Assert( FFIELDDefault( field.ffield ) );

        //  check for any node there, even if it is deleted.  This
        //  is too strong since it may be deleted and committed,
        //  but this strong check ensures that no potentially there
        //  records exist.
        DIB dib;
        dib.dirflag = fDIRAllNode;
        dib.pos     = posFirst;
        DIRGotoRoot( pfucb );
        if ( ( err = ErrDIRDown( pfucb, &dib ) ) != JET_errRecordNotFound )
        {
            if ( JET_errSuccess == err )
            {
                err = ErrERRCheck( JET_errTableNotEmpty );
                DIRUp( pfucb );
                DIRDeferMoveFirst( pfucb );
            }
            return err;
        }
        DIRGotoRoot( pfucb );
    }

    CallR( ErrDIRBeginTransaction( ppib, 58661, NO_GRBIT ) );

    const PGNO  pgnoFDP         = pfcb->PgnoFDP();
    const OBJID objidTable      = pfcb->ObjidFDP();
    FUCB        fucbFake( ppib, pfucb->ifmp );      // for rebuilding default record
    FCB         fcbFake( pfucb->ifmp, pgnoFDP );

    // ExchangeLegacy 261788: Bail out in case the column exists already 
    // to avoid increasing the columnid
    Call( ErrFILEICheckForDuplicateColumn( ppib, pfcb, szFieldName ) );
    
    if ( FFIELDDefault( field.ffield ) && !FFIELDUserDefinedDefault( field.ffield ) )
    {
        // pre-allocate buffer for rebuilding default record
        //
        FILEPrepareDefaultRecord( &fucbFake, &fcbFake, ptdb );
        Assert( fucbFake.pvWorkBuf != NULL );
    }
    else
    {
        fucbFake.pvWorkBuf = NULL;
    }

    //  move to FDP root and update FDP timestamp
    //
    Assert( ppib->Level() < levelMax );
    DIRGotoRoot( pfucb );

    pfcb->EnterDDL();
    fInCritDDL = fTrue;

    //  set tcib
    //
    tcib.fidFixedLast = ptdb->FidFixedLast();
    tcib.fidVarLast = ptdb->FidVarLast();
    tcib.fidTaggedLast = ptdb->FidTaggedLast();

    //******************************************************
    //  Determine maximum field length as follows:
    //  switch field type
    //      case numeric:
    //          max = <exact length of specified type>;
    //      case "short" textual ( Text || Binary ):
    //          if (specified max == 0 ) max = JET_cbColumnMost
    //          else max = MIN( JET_cbColumnMost, specified max )
    //      case "long" textual (Memo || Graphic ):
    //          max = specified max (if 0, unlimited )
    //*****************************************************
    //
    Assert( field.coltyp != JET_coltypNil );

    //  for fixed-length columns, make sure record not too big
    //
    Assert( ptdb->FidFixedLast() >= fidFixedLeast ?
        ptdb->IbEndFixedColumns() > ibRECStartFixedColumns :
        ptdb->IbEndFixedColumns() == ibRECStartFixedColumns );
    if ( ( ( pcolumndef->grbit & JET_bitColumnFixed ) || FCOLTYPFixedLength( field.coltyp ) )
        && ptdb->IbEndFixedColumns() + field.cbMaxLen > (SIZE_T)cbRecordMost )
    {
        err = ErrERRCheck( JET_errRecordTooBig );
        goto HandleError;
    }

    Call( ErrFILEGetNextColumnid(
            field.coltyp,
            pcolumndef->grbit,
            ptdb->FTemplateTable(),
            &tcib,
            &columnid ) );

    if ( pcolumnid != NULL )
    {
        *pcolumnid = columnid;
    }

    //  version and autoincrement are mutually exclusive
    //
    if ( FFIELDVersion( field.ffield ) )
    {
        if ( ptdb->FidVersion() != 0 )
        {
            err = ErrERRCheck( JET_errColumnRedundant );
            goto HandleError;
        }
        ptdb->SetFidVersion( FidOfColumnid( columnid ) );
    }
    else if ( FFIELDAutoincrement( field.ffield ) )
    {
        if ( ptdb->FidAutoincrement() != 0 )
        {
            err = ErrERRCheck( JET_errColumnRedundant );
            goto HandleError;
        }
        ptdb->SetFidAutoincrement( FidOfColumnid( columnid ),
            ( ( field.coltyp == JET_coltypLongLong ) || ( field.coltyp == JET_coltypUnsignedLongLong ) || ( field.coltyp == JET_coltypCurrency ) ) );
    }

    //  update TDB and default record value
    //
    cFixed = ptdb->CDynamicFixedColumns();
    cVar = ptdb->CDynamicVarColumns();
    cTagged = ptdb->CDynamicTaggedColumns();
    cbFieldsUsed = ( cFixed + cVar + cTagged ) * sizeof(FIELD);
    cbFieldsTotal = ptdb->MemPool().CbGetEntry( itagTDBFields );

    //  WARNING: if cbFieldsTotal is less than even one FIELD,
    //  this is the special-case where we just stuck in a
    //  placeholder in the MEMPOOL for the FIELD structures
    if ( cbFieldsTotal < sizeof(FIELD) )
    {
        Assert( cbFIELDPlaceholder == cbFieldsTotal );
        Assert( 0 == cbFieldsUsed );
        cbFieldsTotal = 0;
    }

    Assert( cbFieldsTotal >= cbFieldsUsed );

    // Is there enough space to add another FIELD?
    if ( sizeof(FIELD) > ( cbFieldsTotal - cbFieldsUsed ) )
    {
        //  add space for another 10 columns
        //
        Call( ptdb->MemPool().ErrReplaceEntry(
            itagTDBFields,
            NULL,
            cbFieldsTotal + ( sizeof(FIELD) * 10 )
            ) );
    }

    //  add the column name to the buffer
    //
    Call( ptdb->MemPool().ErrAddEntry(
                            reinterpret_cast<BYTE *>( szFieldName ),
                            (ULONG)strlen( szFieldName ) + 1,
                            &field.itagFieldName ) );
    field.strhashFieldName = StrHashValue( szFieldName );
    Assert( field.itagFieldName != 0 );

    veraddcolumn.columnid = columnid;

    Assert( NULL != ptdb->PdataDefaultRecord() );
    if ( FFIELDDefault( field.ffield )
        && !FFIELDUserDefinedDefault( field.ffield ) )
    {
        //  note that if AddColumn rolls back, the memory may end up
        //  on the FCB RECDANGLING list,
        veraddcolumn.pbOldDefaultRec = (BYTE *)ptdb->PdataDefaultRecord();
    }
    else
    {
        veraddcolumn.pbOldDefaultRec = NULL;
    }

    pver = PverFromIfmp( pfucb->ifmp );
    err = pver->ErrVERFlag( pfucb, operAddColumn, (VOID *)&veraddcolumn, sizeof(VERADDCOLUMN) );
    if ( err < 0 )
    {
        ptdb->MemPool().DeleteEntry( field.itagFieldName );
        field.itagFieldName = 0;
        goto HandleError;
    }

    // If we have the table exclusively locked, then there's no need to
    // set the Versioned bit.
    Assert( !pfcb->FDomainDenyRead( ppib ) );
    if ( !pfcb->FDomainDenyReadByUs( ppib ) )
    {
        FIELDSetVersioned( field.ffield );
        FIELDSetVersionedAdd( field.ffield );
        fSetVersioned = fTrue;
    }

    if ( pfcb->FTemplateTable() )
    {
        Assert( FCOLUMNIDTemplateColumn( columnid ) );
        FIELDSetTemplateColumnESE98( field.ffield );
    }
    else
    {
        Assert( !FCOLUMNIDTemplateColumn( columnid ) );
    }

    //  incrementing fidFixed/Var/TaggedLast guarantees that a new FIELD structure
    //  was added -- rollback checks for this.
    //
    Assert( pfieldNil == pfieldNew );
    if ( FCOLUMNIDFixed( columnid ) )
    {
        Assert( FidOfColumnid( columnid ) == ptdb->FidFixedLast() + 1 );
        ptdb->IncrementFidFixedLast();
        Assert( ptdb->FidFixedLast().FFixed() );
        pfieldNew = ptdb->PfieldFixed( columnid );

        //  Adjust the location of the FIELD structures for tagged and variable
        //  columns to accommodate the insertion of a fixed column FIELD structure
        //  and its associated entry in the fixed offsets table.
        //  Thus, the new fixed column FIELD structure is now located where
        //  the variable column FIELD structures used to start.
        memmove(
            (BYTE *)pfieldNew + sizeof(FIELD),
            pfieldNew,
            sizeof(FIELD) * ( cVar + cTagged )
            );

        field.ibRecordOffset = ptdb->IbEndFixedColumns();

        *pfieldNew = field;
        Assert( ptdb->Pfield( columnid ) == pfieldNew );

        Assert( field.ibRecordOffset + field.cbMaxLen <= (SIZE_T)cbRecordMost );
        ptdb->SetIbEndFixedColumns( (WORD)( field.ibRecordOffset + field.cbMaxLen ), ptdb->FidFixedLast() );
    }
    else
    {
        if ( FCOLUMNIDTagged( columnid ) )
        {
            Assert( FidOfColumnid( columnid ) == ptdb->FidTaggedLast() + 1 );

            // Append the new FIELD structure to the end of the tagged column
            // FIELD structures.
            ptdb->IncrementFidTaggedLast();
            Assert( ptdb->FidTaggedLast().FTagged() );
            pfieldNew = ptdb->PfieldTagged( columnid );
        }
        else
        {
            // The new variable column FIELD structure, now located where
            // the tagged column FIELD structures used to start.
            Assert( FCOLUMNIDVar( columnid ) );
            Assert( FidOfColumnid( columnid ) == ptdb->FidVarLast() + 1 );
            ptdb->IncrementFidVarLast();
            Assert( ptdb->FidVarLast().FVar() );
            pfieldNew = ptdb->PfieldVar( columnid );

            //  adjust the location of the FIELD structures for tagged columns to
            //  accommodate the insertion of the variable column FIELD structure.
            memmove( pfieldNew + 1, pfieldNew, sizeof(FIELD) * cTagged );
        }

        *pfieldNew = field;
        Assert( ptdb->Pfield( columnid ) == pfieldNew );
    }


    Assert( field.itagFieldName != 0 );
    field.itagFieldName = 0;    // From this point on, version cleanup
                                // will reclaim name space on rollback.

    //  WARNING: rollback will not reset these flags in the TDB
    //
    if ( FFIELDFinalize( field.ffield ) )
    {
        Assert( FFIELDEscrowUpdate( field.ffield ) );
        ptdb->SetFTableHasFinalizeColumn();
    }
    if ( FFIELDDeleteOnZero( field.ffield ) )
    {
        Assert( FFIELDEscrowUpdate( field.ffield ) );
        ptdb->SetFTableHasDeleteOnZeroColumn();
    }

    if ( FFIELDUserDefinedDefault( field.ffield ) )
    {
        //  the size of the structure should have been checked by ErrFILEIValidateAddColumn
        Assert( NULL != pvDefault );
        Assert( sizeof( JET_USERDEFINEDDEFAULT_A ) == cbDefault );

        JET_USERDEFINEDDEFAULT_A * const pudd = (JET_USERDEFINEDDEFAULT_A *)pvDefault;

        JET_CALLBACK callback = NULL;
        if ( BoolParam( PinstFromPpib( ppib ), JET_paramEnablePersistedCallbacks ) )
        {
            Call( ErrCALLBACKResolve( pudd->szCallback, &callback ) );
        }

        CBDESC * const pcbdesc = new CBDESC;
        BYTE * const pbUserData = ( ( pudd->cbUserData > 0 ) ?
                        (BYTE *)PvOSMemoryHeapAlloc( pudd->cbUserData ) : NULL );

        if( NULL == pcbdesc
            || ( NULL == pbUserData && pudd->cbUserData > 0 ) )
        {
            delete pcbdesc;
            OSMemoryHeapFree( pbUserData );
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        //  we should not fail between this point and registering the CBDESC in the TDB
        //  once the CBDESC is in the TDB it will be freed by TDB::Delete

        memcpy( pbUserData, pudd->pbUserData, pudd->cbUserData );

        pcbdesc->pcallback  = callback;
        pcbdesc->cbtyp      = JET_cbtypUserDefinedDefaultValue;
        pcbdesc->pvContext  = pbUserData;
        pcbdesc->cbContext  = pudd->cbUserData;
        pcbdesc->ulId       = columnid;
        pcbdesc->fPermanent = fTrue;
        pcbdesc->fVersioned = fFalse;

        //  for user-defined-defaults we don't store a default value at all
        pvDefaultAdd    = NULL;
        cbDefaultAdd    = 0;
        szCallbackAdd   = pudd->szCallback;
        pvUserDataAdd   = pudd->pbUserData;
        cbUserDataAdd   = pudd->cbUserData;

        pfcb->AssertDDL();
        ptdb->RegisterPcbdesc( pcbdesc );

        ptdb->SetFTableHasUserDefinedDefault();
        ptdb->SetFTableHasNonEscrowDefault();
        ptdb->SetFTableHasDefault();
    }
    else if ( FFIELDDefault( field.ffield ) )
    {
        DATA    dataDefault;

        Assert( cbDefault > 0 );
        Assert( pvDefault != NULL );
        dataDefault.SetCb( cbDefault );
        dataDefault.SetPv( (VOID *)pvDefault );

        Assert( fucbFake.pvWorkBuf != NULL );
        Assert( fucbFake.u.pfcb == &fcbFake );
        Assert( fcbFake.Ptdb() == ptdb );
        Call( ErrFILERebuildDefaultRec( &fucbFake, columnid, &dataDefault ) );

        Assert( NULL != veraddcolumn.pbOldDefaultRec );
        Assert( veraddcolumn.pbOldDefaultRec != (BYTE *)ptdb->PdataDefaultRecord() );

        //  we normally rely on version cleanup to clean up pbOldDefaultRec,
        //  but in case versioning is disabled or version cleanup can't get
        //  to the RCE for some reason, put it on the RECDANGLING list
        //

#ifdef DEBUG
        //  verify not already in list
        //
        for ( RECDANGLING * precdangling = pfcb->Precdangling();
            NULL != precdangling;
            precdangling = precdangling->precdanglingNext )
        {
            Assert( (BYTE *)precdangling != veraddcolumn.pbOldDefaultRec );
        }
#endif

        //  assumes that the memory pointed to by pmemdangling is always at
        //  least sizeof(ULONG_PTR) bytes
        //
        Assert( NULL == ( (RECDANGLING *)veraddcolumn.pbOldDefaultRec )->precdanglingNext );
        ( (RECDANGLING *)veraddcolumn.pbOldDefaultRec )->precdanglingNext = pfcb->Precdangling();
        pfcb->SetPrecdangling( (RECDANGLING *)veraddcolumn.pbOldDefaultRec );

        if ( !FFIELDEscrowUpdate( field.ffield ) )
        {
            ptdb->SetFTableHasNonEscrowDefault();
        }
        ptdb->SetFTableHasDefault();
    }
    else if ( FFIELDNotNull( field.ffield ) )
    {
        if ( FCOLUMNIDTagged( columnid ) )
        {
            ptdb->SetFTableHasNonNullTaggedColumn();
        }
        else if ( FCOLUMNIDFixed( columnid ) )
        {
            ptdb->SetFTableHasNonNullFixedColumn();
        }
        else
        {
            Assert( FCOLUMNIDVar( columnid ) );
            ptdb->SetFTableHasNonNullVarColumn();
        }
    }

    pfcb->LeaveDDL();
    fInCritDDL = fFalse;

    //  set currency before first
    //
    DIRBeforeFirst( pfucb );
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        DIRBeforeFirst( pfucb->pfucbCurIndex );
    }

    // Insert record into MSysColumns.
    Call( ErrCATAddTableColumn(
                ppib,
                pfucb->ifmp,
                objidTable,
                szFieldName,
                columnid,
                &field,
                pvDefaultAdd,
                cbDefaultAdd,
                szCallbackAdd,
                pvUserDataAdd,
                cbUserDataAdd ) );

    if ( ( pcolumndef->grbit & JET_bitColumnAutoincrement ) != 0 )
    {
        Call( ErrFILEIUpdateAutoInc( ppib, pfucb ) );
    }

    Call( ErrDIRCommitTransaction( ppib, 0 ) );

    FILEFreeDefaultRecord( &fucbFake );

    if ( fMaxTruncated )
        return ErrERRCheck( JET_wrnColumnMaxTruncated );

    AssertDIRNoLatch( ppib );

    if ( fUnversioned )
    {
        if ( fSetVersioned )
        {
            // On success, reset versioned bit (on error, bit reset by rollback)
            FIELD   *pfieldT;
            pfcb->EnterDDL();
            Assert( pfcb->Ptdb() == ptdb );
            pfieldT = ptdb->Pfield( columnid );
            FIELDResetVersioned( pfieldT->ffield );
            FIELDResetVersionedAdd( pfieldT->ffield );
            pfcb->LeaveDDL();
        }

        FILERemoveFromUnverList( &g_punvercolGlobal, g_critUnverCol, objidTable, szFieldName );
    }

    return JET_errSuccess;

HandleError:

    OSTraceFMP(
        pfcb->Ifmp(),
        JET_tracetagDDLWrite,
        OSFormat(
            "Finished adding column '%s' with error %d (0x%x)",
            szName,
            err,
            err ) );

    //  For FixedDDL tables, we assume such tables NEVER have deleted columns,
    //  so we can't tolerate the hack where we set the FIELD of the rolled-back
    //  column to deleted

    // Verify name space has been reclaimed.
    Assert( field.itagFieldName == 0 );

    if ( fInCritDDL )
    {
        pfcb->LeaveDDL();
    }

    FILEFreeDefaultRecord( &fucbFake );

    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    if ( fUnversioned )
    {
        FILERemoveFromUnverList( &g_punvercolGlobal, g_critUnverCol, objidTable, szFieldName );
    }

    AssertDIRNoLatch( ppib );

    return err;
}


LOCAL ERR ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
    FUCB            * const pfucbTable,
    FCB             * const pfcbIndex,
    DATA&           dataRec,
    const DATA&     dataPrimaryBM,
    FUCB            * const pfucbSort,
    KEY&            keyBuffer,
    LONG            *pcEntriesAdded
    )
{
    ERR             err;
    IDB             * const pidb            = pfcbIndex->Pidb();
    BOOL            fNullKey                = fFalse;
    KeySequence     ksT( pfucbTable, pidb );
    ULONG           iidxsegT    ;
    BOOL            fFirstColumnMultiValued = FRECIFirstIndexColumnMultiValued( pfucbTable, pidb );

#ifdef DEBUG
    //  latch status shouldn't change during this function
    const BOOL      fLatched    = Pcsr( pfucbTable )->FLatched();
#endif

    *pcEntriesAdded = 0;

    Assert( ksT.FBaseKey() );
    for ( ; !ksT.FSequenceComplete(); ksT.Increment( iidxsegT ) )
    {
        //  latch status shouldn't change during this function
        Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
            || ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

        CallR( ErrRECIRetrieveKey(
                pfucbTable,
                pidb,
                dataRec,
                &keyBuffer,
                ksT.Rgitag(),
                pidb->FTuples() ? pidb->IchTuplesStart() : 0,
                fFalse,
                prceNil,
                &iidxsegT ) );

        //  latch status shouldn't change during this function
        Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
            || ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

        CallS( ErrRECValidIndexKeyWarning( err ) );

        const ERR   errRetrieveKey  = err;      //  for debugging

        if ( JET_errSuccess == err )
        {
            //  insert key (below)
        }

        else if ( wrnFLDOutOfKeys == err )
        {
            Assert( !ksT.FBaseKey() );
            continue;
        }

        else if ( wrnFLDOutOfTuples == err )
        {
            //  try next itagSequence
            Assert( pidb->FTuples() );
            //  tuple index supported for first column in index only
            //
            if ( fFirstColumnMultiValued )
                continue;
            else
                break;
        }

        else if ( wrnFLDNotPresentInIndex == err )
        {
            Assert( ksT.FBaseKey() );
            break;
        }

        else
        {
            Assert( wrnFLDNullSeg == err
                || wrnFLDNullFirstSeg == err
                || wrnFLDNullKey == err );

            if ( pidb->FNoNullSeg() )
            {
                err = ErrERRCheck( JET_errNullKeyDisallowed );
                return err;
            }

            if ( wrnFLDNullKey == err )
            {
                Assert( ksT.FBaseKey() );   //  nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
                if ( !pidb->FAllowAllNulls() )
                {
                    //  do not insert NULL key as indicated
                    break;
                }
                fNullKey = fTrue;
            }
            else if ( wrnFLDNullFirstSeg == err )
            {
                if ( !pidb->FAllowFirstNull() )
                {
                    //  do not insert keys with NULL first
                    //  segment as indicated
                    break;
                }
            }
            else
            {
                Assert( wrnFLDNullSeg == err );
                if ( !pidb->FAllowSomeNulls() )
                {
                    //  do not insert keys with null
                    //  segment as indicated
                    break;
                }
            }
        }

        Assert( keyBuffer.Cb() <= cbKeyMostMost );
        CallR( ErrSORTInsert( pfucbSort, keyBuffer, dataPrimaryBM ) );
        (*pcEntriesAdded)++;

        if ( pidb->FTuples() )
        {
            //  if we received a warning on the first tuple,
            //  the logic above should ensure we don't
            //  try to create further tuples
            //  (except the wrnFLDNullSeg which is expected for an index allowing it)
            //
            CallSx( errRetrieveKey, ( pidb->FAllowSomeNulls() ? wrnFLDNullSeg : JET_errSuccess ) );

            ULONG   ichOffset;
            ULONG   ulTuple;

            for ( ichOffset = pidb->IchTuplesStart() + pidb->CchTuplesIncrement(), ulTuple = 1; ulTuple < pidb->IchTuplesToIndexMax(); ichOffset += pidb->CchTuplesIncrement(), ulTuple++ )
            {
                //  latch status shouldn't change during this function
                Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
                    || ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

                CallR( ErrRECIRetrieveKey(
                        pfucbTable,
                        pidb,
                        dataRec,
                        &keyBuffer,
                        ksT.Rgitag(),
                        ichOffset,
                        fFalse,
                        prceNil,
                        &iidxsegT ) );

                //  latch status shouldn't change during this function
                Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
                    || ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

                //  all other warnings should have been detected
                //  when we retrieved with ichOffset==0
                //
                if ( err == wrnFLDNullSeg && pidb->FAllowSomeNulls() )
                {
                    err = JET_errSuccess;
                }

                CallSx( err, wrnFLDOutOfTuples );
                if ( JET_errSuccess != err )
                    break;

                Assert( keyBuffer.Cb() <= cbKeyMostMost );
                CallR( ErrSORTInsert( pfucbSort, keyBuffer, dataPrimaryBM ) );
                (*pcEntriesAdded)++;
            }
        }

        if ( !pidb->FMultivalued() || fNullKey )
        {
            //  if no multivalues in this index, there's no point going beyond first itagSequence
            //  if key is null, this implies there are no further multivalues
            Assert( ksT.FBaseKey() );   //  nulls beyond itagSequence 1 would have generated wrnFLDOutOfKeys
            break;
        }
    }

    //  latch status shouldn't change during this function
    Assert( ( fLatched && Pcsr( pfucbTable )->FLatched() )
        || ( !fLatched && !Pcsr( pfucbTable )->FLatched() ) );

    return JET_errSuccess;
}

INLINE ERR ErrFILEIndexProgress( STATUSINFO * const pstatus )
{
    JET_SNPROG  snprog;

    Assert( pstatus->pfnStatus );
    Assert( pstatus->snt == JET_sntProgress );

    pstatus->cunitDone += pstatus->cunitPerProgression;

    Assert( pstatus->cunitProjected <= pstatus->cunitTotal );
    if ( pstatus->cunitDone > pstatus->cunitProjected )
    {
        Assert( g_fRepair );
        pstatus->cunitPerProgression = 0;
        pstatus->cunitDone = pstatus->cunitProjected;
    }

    Assert( pstatus->cunitDone <= pstatus->cunitTotal );

    snprog.cbStruct = sizeof( JET_SNPROG );
    snprog.cunitDone = pstatus->cunitDone;
    snprog.cunitTotal = pstatus->cunitTotal;

    return ( ERR )( *pstatus->pfnStatus )(
        pstatus->sesid,
        pstatus->snp,
        pstatus->snt,
        &snprog );
}


ERR ErrFILEIndexBatchInit(
    PIB         * const ppib,
    FUCB        ** const rgpfucbSort,
    FCB         * const pfcbIndexesToBuild,
    ULONG       * const pcIndexesToBuild,
    ULONG       * const rgcRecInput,
    FCB         **ppfcbNextBuildIndex,
    const ULONG cIndexBatchMax )
{
    ERR         err;
    FCB         *pfcbIndex;
    ULONG       iindex;

    Assert( pfcbNil != pfcbIndexesToBuild );
    Assert( cIndexBatchMax > 0 );

    iindex = 0;
    for ( pfcbIndex = pfcbIndexesToBuild;
        pfcbIndex != pfcbNil;
        pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        Assert( pfcbIndex->Pidb() != pidbNil );
        Assert( pfcbIndex->FTypeSecondaryIndex() );
        Assert( pfcbIndex->FInitialized() );

        //  open sort
        //
        CallR( ErrSORTOpen( ppib, &rgpfucbSort[iindex], pfcbIndex->Pidb()->FUnique(), fTrue ) );

        rgcRecInput[iindex] = 0;

        iindex++;

        if ( cIndexBatchMax == iindex )
        {
            break;
        }
    }

    *pcIndexesToBuild = (INT)iindex;
    Assert( *pcIndexesToBuild > 0 );

    if ( NULL != ppfcbNextBuildIndex )
    {
        *ppfcbNextBuildIndex = ( cIndexBatchMax == iindex ? pfcbIndex->PfcbNextIndex() : pfcbNil );
    }

    return JET_errSuccess;
}

// ORIGINAL BATCH INDEX CREATION (SERIAL)
// OR DEFRAG IF LESS THEN 16 INDICES
ERR ErrFILEIndexBatchAddEntry(
    FUCB            ** const rgpfucbSort,
    FUCB            * const pfucbTable,
    const BOOKMARK  * const pbmPrimary,
    DATA&           dataRec,
    FCB             * const pfcbIndexesToBuild,
    const ULONG     cIndexesToBuild,
    ULONG           * const rgcRecInput,
    KEY&            keyBuffer )
{
    ERR             err;
    FCB             *pfcbIndex;
    LONG            cSecondaryIndexEntriesAdded;

    Assert( pfcbNil != pfcbIndexesToBuild );
    Assert( cIndexesToBuild > 0 );

    Assert( pbmPrimary->key.prefix.FNull() );
    Assert( pbmPrimary->data.FNull() );

    pfcbIndex = pfcbIndexesToBuild;
    for ( ULONG iindex = 0; iindex < cIndexesToBuild; iindex++, pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        Assert( pfcbNil != pfcbIndex );
        Assert( pidbNil != pfcbIndex->Pidb() );
        CallR( ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
                    pfucbTable,
                    pfcbIndex,
                    dataRec,
                    pbmPrimary->key.suffix,
                    rgpfucbSort[iindex],
                    keyBuffer,
                    &cSecondaryIndexEntriesAdded
                    ) );

        Assert( cSecondaryIndexEntriesAdded >= 0 );
        rgcRecInput[iindex] += cSecondaryIndexEntriesAdded;
    }

    return JET_errSuccess;
}


INLINE ERR ErrFILEIAppendToIndex(
    FUCB        * const pfucbSort,
    FUCB        * const pfucbIndex,
    ULONG       *pcRecOutput,
    const BOOL  fLogged )
{
    ERR             err;
    const INT       fDIRFlags   = ( fDIRNoVersion | fDIRAppend | ( fLogged ? 0 : fDIRNoLog ) );
    INST * const    pinst       = PinstFromPfucb( pfucbIndex );

    CallR( ErrDIRInitAppend( pfucbIndex ) );
    do
    {
        //  this loop can take some time, so see if we need
        //  to terminate because of shutdown
        //
        CallR( pinst->ErrCheckForTermination() );

        //  don't want to fill up reserve logs with index entries,
        //  so bail out if we're in the reserve logs (or out
        //  of log disk entirely)
        //
        //  NOTE: must check even if fLogged is FALSE
        //  because we may be in the pseudo-logging-disabled
        //  state where logging is actually enabled, but
        //  we're not logging batch index creation, but
        //  we are still logging space operations, so we may
        //  end up filling the logs with that
        //
        CallR( pinst->m_plog->ErrLGCheckState() );

        CallR( ErrDIRAppend(
                    pfucbIndex,
                    pfucbSort->kdfCurr.key,
                    pfucbSort->kdfCurr.data,
                    fDIRFlags ) );
        Assert( Pcsr( pfucbIndex )->FLatched() );
        (*pcRecOutput)++;

        err = ErrSORTNext( pfucbSort );
    }
    while ( err >= 0 );

    if ( JET_errNoCurrentRecord == err )
        err = ErrDIRTermAppend( pfucbIndex );

    return err;
}


// Don't make this INLINE, since it's just error-handling code.
LOCAL VOID FILEIReportIndexCorrupt( FUCB * const pfucbIndex, CPRINTF * const pcprintf )
{

    FCB         *pfcbIndex = pfucbIndex->u.pfcb;
    Assert( pfcbNil != pfcbIndex );
    Assert( pfcbIndex->FTypeSecondaryIndex() );
    Assert( pfcbIndex->Pidb() != pidbNil );

    FCB         *pfcbTable = pfcbIndex->PfcbTable();
    Assert( pfcbNil != pfcbTable );
    Assert( pfcbTable->FTypeTable() );
    Assert( pfcbTable->Ptdb() != ptdbNil );

    // Strictly speaking, EnterDML() is not needed because this
    // function only gets executed by the consistency-checker,
    // which is single-threaded.  However, make the call anyway
    // to avoid potential bugs in the future if this function
    // ever gets used in multi-threaded situations.
    pfcbTable->EnterDML();
    (*pcprintf)(
        "Table \"%s\", Index \"%s\"%s:  ",
        pfcbTable->Ptdb()->SzTableName(),
        pfcbTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() ),
        pfcbIndex->Pidb()->FLocalizedText() ? " (localized text)" : "" );
    pfcbTable->LeaveDML();
}


INLINE ERR ErrFILEICheckIndex(
    FUCB    * const pfucbSort,
    FUCB    * const pfucbIndex,
    ULONG   *pcRecSeen,
    BOOL    *pfCorruptedIndex,
    CPRINTF * const pcprintf )
{
    ERR     err;
    INT     cMisMatchCount = 0;
    DIB     dib;
    dib.dirflag = fDIRNull;
    dib.pos     = posFirst;

    Assert( NULL != pfCorruptedIndex );

    FUCBSetPrereadForward( pfucbIndex, cpgPrereadSequential );

    Call( ErrBTDown( pfucbIndex, &dib, latchReadTouch ) );

    do
    {
        (*pcRecSeen)++;

        Assert( Pcsr( pfucbIndex )->FLatched() );

        INT cmp = CmpKeyData( pfucbIndex->kdfCurr, pfucbSort->kdfCurr );
        if( 0 != cmp )
        {
            FILEIReportIndexCorrupt( pfucbIndex, pcprintf );
            (*pcprintf)(    "index entry %d [%d:%d] (objid %d) inconsistent\r\n",
                            *pcRecSeen,
                            Pcsr( pfucbIndex )->Pgno(),
                            Pcsr( pfucbIndex )->ILine(),
                            Pcsr( pfucbIndex )->Cpage().ObjidFDP() );

            BYTE rgbKey[cbKeyAlloc];
            CHAR rgchBuf[1024];

            INT ib;
            INT ibMax;
            const BYTE * pb;
            CHAR * pchBuf;
            ULONG cbBufLeft;

            pfucbIndex->kdfCurr.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );
            pb = rgbKey;
            ibMax = pfucbIndex->kdfCurr.key.Cb();
            pchBuf = rgchBuf;
            cbBufLeft = sizeof(rgchBuf);
            for( ib = 0; ib < ibMax && cbBufLeft > 0; ++ib )
            {
                OSStrCbFormatA( pchBuf, cbBufLeft, " %2.2x", pb[ib] );
                cbBufLeft -= strlen(pchBuf);
                pchBuf += strlen(pchBuf);
            }
            (*pcprintf)( "\tkey in database (%d bytes):%s\r\n", ibMax, rgchBuf );

            pb = (BYTE*)pfucbIndex->kdfCurr.data.Pv();
            ibMax = pfucbIndex->kdfCurr.data.Cb();
            pchBuf = rgchBuf;
            cbBufLeft = sizeof(rgchBuf);
            for( ib = 0; ib < ibMax && cbBufLeft > 0; ++ib )
            {
                OSStrCbFormatA( pchBuf, cbBufLeft, " %2.2x", pb[ib] );
                cbBufLeft -= strlen(pchBuf);
                pchBuf += strlen(pchBuf);
            }
            (*pcprintf)( "\tdata in database (%d bytes):%s\r\n", ibMax, rgchBuf );

            pfucbSort->kdfCurr.key.CopyIntoBuffer( rgbKey, sizeof( rgbKey ) );
            pb = rgbKey;
            ibMax = pfucbSort->kdfCurr.key.Cb();
            pchBuf = rgchBuf;
            cbBufLeft = sizeof(rgchBuf);
            for( ib = 0; ib < ibMax && cbBufLeft > 0; ++ib )
            {
                OSStrCbFormatA( pchBuf, cbBufLeft, " %2.2x", pb[ib] );
                cbBufLeft -= strlen(pchBuf);
                pchBuf += strlen(pchBuf);
            }
            (*pcprintf)( "\tcalculated key (%d bytes):%s\r\n", ibMax, rgchBuf );

            pb = (BYTE*)pfucbSort->kdfCurr.data.Pv();
            ibMax = pfucbSort->kdfCurr.data.Cb();
            pchBuf = rgchBuf;
            cbBufLeft = sizeof(rgchBuf);
            for( ib = 0; ib < ibMax && cbBufLeft > 0; ++ib )
            {
                OSStrCbFormatA( pchBuf, cbBufLeft, " %2.2x", pb[ib] );
                cbBufLeft -= strlen(pchBuf);
                pchBuf += strlen(pchBuf);
            }
            (*pcprintf)( "\tcalculated data (%d bytes):%s\r\n", ibMax, rgchBuf );

            *pfCorruptedIndex = fTrue;
            cMisMatchCount++;

            if ( cMisMatchCount > 10 )
            {
                (*pcprintf)( "Found too many inconsistent keys, halting further checks." );
                break;
            }
        }

        err = ErrBTNext( pfucbIndex, fDIRNull );
        if( err < 0 )
        {
            if ( JET_errNoCurrentRecord == err
                && JET_errNoCurrentRecord != ErrSORTNext( pfucbSort ) )
            {
                FILEIReportIndexCorrupt( pfucbIndex, pcprintf );
                (*pcprintf)( "real index has fewer (%d) srecords\r\n", *pcRecSeen );
                *pfCorruptedIndex = fTrue;
                break;
            }
        }
        else
        {
            err = ErrSORTNext( pfucbSort );
            if( JET_errNoCurrentRecord == err )
            {
                FILEIReportIndexCorrupt( pfucbIndex, pcprintf );
                (*pcprintf)( "generated index has fewer (%d) srecords\r\n", *pcRecSeen );
                *pfCorruptedIndex = fTrue;
                break;
            }
        }
    }
    while ( err >= 0 );

    err = ( JET_errNoCurrentRecord == err ? JET_errSuccess : err );

HandleError:
    FUCBResetPreread( pfucbIndex );
    BTUp( pfucbIndex );
    return err;
}


#ifdef PARALLEL_BATCH_INDEX_BUILD

LOCAL ERR ErrFILESingleIndexTerm(
    PIB         * const ppib,
    FUCB        ** const rgpfucbSort,
    FCB         * const pfcbIndex,
    ULONG       * const rgcRecInput,
    const ULONG iindex,
    BOOL        *pfCorruptionEncountered,
    CPRINTF     * const pcprintf,
    const BOOL  fLogged )
{
    Assert( pfcbNil != pfcbIndex );

    ERR         err             = JET_errSuccess;
    FUCB        *pfucbIndex     = pfucbNil;
    const BOOL  fUnique         = pfcbIndex->Pidb()->FUnique();
    ULONG       cRecOutput      = 0;
    BOOL        fEntriesExist   = fTrue;
    BOOL        fCorruptedIndex = fFalse;

    Assert( pfcbIndex->FTypeSecondaryIndex() );
    Assert( pidbNil != pfcbIndex->Pidb() );

#ifdef SHOW_INDEX_PERF
    const TICK  tickStart       = TickOSTimeCurrent();

    OSTrace( tracetagIndexPerf, OSFormat( "About to sort keys for index %d.\n", iindex+1 ) );
#endif

    Call( ErrSORTEndInsert( rgpfucbSort[iindex] ) );

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Sorted keys for index %d in %d msecs.\n",
                                        iindex+1,
                                        TickOSTimeCurrent() - tickStart ) );
#endif

    //  transfer index entries to actual index
    //  insert first one in normal method!
    //
    err = ErrSORTNext( rgpfucbSort[iindex] );
    if ( err < 0 )
    {
        if ( JET_errNoCurrentRecord != err )
            goto HandleError;

        err = JET_errSuccess;

        Assert( rgcRecInput[iindex] == 0 );
        fEntriesExist = fFalse;
    }

    if ( fEntriesExist )
    {

        //  open cursor on index
        //
        Assert( pfucbNil == pfucbIndex );
        Call( ErrDIROpen( ppib, pfcbIndex, &pfucbIndex ) );
        FUCBSetIndex( pfucbIndex );
        FUCBSetSecondary( pfucbIndex );
        DIRGotoRoot( pfucbIndex );

        if ( NULL != pfCorruptionEncountered )
        {
            Assert( NULL != pcprintf );
            Call( ErrFILEICheckIndex(
                        rgpfucbSort[iindex],
                        pfucbIndex,
                        &cRecOutput,
                        &fCorruptedIndex,
                        pcprintf ) );
            if ( fCorruptedIndex )
                *pfCorruptionEncountered = fTrue;
        }
        else
        {
#ifdef SHOW_INDEX_PERF
            const TICK  tickStart   = TickOSTimeCurrent();
#endif

            Call( ErrFILEIAppendToIndex(
                        rgpfucbSort[iindex],
                        pfucbIndex,
                        &cRecOutput,
                        fLogged ) );

#ifdef SHOW_INDEX_PERF
            OSTraceIndent( tracetagIndexPerf, +1 );
            OSTrace( tracetagIndexPerf, OSFormat(   "Appended %d keys for index %d in %d msecs.\n",
                                                cRecOutput,
                                                iindex+1,
                                                TickOSTimeCurrent() - tickStart ) );
            OSTraceIndent( tracetagIndexPerf, -1 );
#endif
        }

        DIRClose( pfucbIndex );
        pfucbIndex = pfucbNil;
    }
    else
    {
#ifdef SHOW_INDEX_PERF
        OSTraceIndent( tracetagIndexPerf, +1 );
        OSTrace( tracetagIndexPerf, OSFormat( "No keys generated for index %d.\n", iindex+1 ) );
        OSTraceIndent( tracetagIndexPerf, -1 );
#endif
    }

    SORTClose( rgpfucbSort[iindex] );
    rgpfucbSort[iindex] = pfucbNil;

    Assert( cRecOutput <= rgcRecInput[iindex] );
    if ( cRecOutput != rgcRecInput[iindex] )
    {
        if ( cRecOutput < rgcRecInput[iindex] && !fUnique )
        {
            //  duplicates over multi-valued columns must have been removed
        }

        else if ( NULL != pfCorruptionEncountered )
        {
            *pfCorruptionEncountered = fTrue;

            // Only report this error if we haven't
            // encountered corruption on this index already.
            if ( !fCorruptedIndex )
            {
                if ( cRecOutput > rgcRecInput[iindex] )
                {
                    (*pcprintf)( "Too many index keys generated\n" );
                }
                else
                {
                    Assert( fUnique );
                    (*pcprintf)( "%d duplicate key(s) on unique index\n", rgcRecInput[iindex] - cRecOutput );
                }
            }
        }

        else if ( cRecOutput > rgcRecInput[iindex] )
        {
            err = ErrERRCheck( JET_errIndexBuildCorrupted );
            goto HandleError;
        }

        else
        {
            Assert( cRecOutput < rgcRecInput[iindex] );
            Assert( fUnique );

            //  If we got dupes and the index was NOT unique, it must have been
            //  dupes over multi-value columns, in which case the dupes
            //  get properly eliminated.
            err = ErrERRCheck( JET_errKeyDuplicate );
            goto HandleError;
        }
    }

HandleError:
    if ( pfucbNil != pfucbIndex )
    {
        Assert( err < 0 );
        DIRClose( pfucbIndex );
    }

    return err;
}


ERR ErrFILEIndexBatchTerm(
    PIB         * const ppib,
    FUCB        ** const rgpfucbSort,
    FCB         * const pfcbIndexesToBuild,
    const ULONG cIndexesToBuild,
    ULONG       * const rgcRecInput,
    STATUSINFO  * const pstatus,
    BOOL        *pfCorruptionEncountered,
    CPRINTF     * const pcprintf )
{
    ERR         err = JET_errSuccess;
    FCB         *pfcbIndex = pfcbIndexesToBuild;

    Assert( pfcbNil != pfcbIndexesToBuild );
    Assert( cIndexesToBuild > 0 );

    for ( ULONG iindex = 0; iindex < cIndexesToBuild; iindex++, pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        CallR( ErrFILESingleIndexTerm(
                        ppib,
                        rgpfucbSort,
                        pfcbIndex,
                        rgcRecInput,
                        iindex,
                        pfCorruptionEncountered,
                        pcprintf,
                        fFalse ) );     //  only called by defrag, which has logging disabled

        if ( pstatus )
        {
            CallR( ErrFILEIndexProgress( pstatus ) );
        }
    }

    return err;
}


struct CREATEINDEXCONTEXT
{
    CTaskManager    taskmgrCreateIndex;
    ULONG           iindexStart;
    ULONG           cIndexesToBuild;
    FCB *           pfcbIndexesToBuild;
    FUCB *          pfucbTable;
    FUCB **         rgpfucbSort;
    ULONG *         rgcRecInput;
    DATA            dataBMBuffer;
    KEY             keyBuffer;
    STATUSINFO  *   pstatus;
    CPRINTF *       pcprintf;
    ERR             err;

    union
    {
        ULONG       ulFlags;
        struct
        {
            FLAG32  fAllocatedTaskManager:1;
            FLAG32  fLogged:1;
            FLAG32  fCheckOnly:1;
            FLAG32  fCorruptionEncountered:1;
        };
    };
};

struct FCBINDEXCONTEXT
{
    FCB *       pfcbIndex;
    ULONG       iindex;
};

LOCAL VOID ErrFILEIndexDispatchAddEntries(
    const DWORD                 dwError,
    const DWORD_PTR             dwParam1,
    const DWORD                 dwParam2,
    const DWORD_PTR             dwParam3 )
{
    CREATEINDEXCONTEXT * const  pidxcontext     = (CREATEINDEXCONTEXT *)dwParam1;
    CSR * const                 pcsr            = (CSR *)dwParam3;
    KEYDATAFLAGS                kdf;
    LONG                        cEntriesAdded;

    // Establish user trace context because this code runs on a threadpool thread
    Assert( PutcTLSGetUserContext() == NULL );
    pidxcontext->pfucbTable->ppib->SetUserTraceContextInTls();
    PIBTraceContextScope tcScope = pidxcontext->pfucbTable->ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSort );

    //  only proceed if previous tasks did not encounter an error
    //
    if ( JET_errSuccess == pidxcontext->err )
    {
        const INT   ilineMax    = pcsr->Cpage().Clines();

        for ( pcsr->SetILine( 0 ); pcsr->ILine() < ilineMax; pcsr->IncrementILine() )
        {
            Assert( pcsr->FLatched() );
            NDIGetKeydataflags( pcsr->Cpage(), pcsr->ILine(), &kdf );

            //  WARNING! WARNING! WARNING!
            //  Batch index creation is only performed when we have
            //  exclusive use of the table, so we don't need to worry
            //  about versioning and can therefore get away with just
            //  checking the fDeleted flag
            //
            if ( FNDDeleted( kdf ) )
            {
                continue;
            }

            //  must copy key to a buffer because the routines that use it
            //  are not prepared to deal with prefix/suffix
            //
            kdf.key.CopyIntoBuffer( pidxcontext->dataBMBuffer.Pv(), cbKeyAlloc );
            pidxcontext->dataBMBuffer.SetCb( kdf.key.Cb() );

            FCB *       pfcbIndex   = pidxcontext->pfcbIndexesToBuild;
            const ULONG iindexMax   = pidxcontext->iindexStart + pidxcontext->cIndexesToBuild;

            for ( ULONG iindex = pidxcontext->iindexStart;
                iindex < iindexMax;
                iindex++, pfcbIndex = pfcbIndex->PfcbNextIndex() )
            {
                Assert( pfcbNil != pfcbIndex );
                Assert( pidbNil != pfcbIndex->Pidb() );
                const ERR   errT    = ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
                                            pidxcontext->pfucbTable,
                                            pfcbIndex,
                                            kdf.data,
                                            pidxcontext->dataBMBuffer,
                                            pidxcontext->rgpfucbSort[iindex],
                                            pidxcontext->keyBuffer,
                                            &cEntriesAdded );
                if ( errT < JET_errSuccess )
                {
                    pidxcontext->err = errT;
                    break;
                }

                Assert( cEntriesAdded >= 0 );
                pidxcontext->rgcRecInput[iindex] += cEntriesAdded;
            }
        }
    }

    //  NOTE:  we disable ownership tracking here because we are
    //  releasing read latches acquired for us by another thread
    //
    CLockDeadlockDetectionInfo::DisableOwnershipTracking();
    pcsr->ReleasePage();
    CLockDeadlockDetectionInfo::EnableOwnershipTracking();
    delete pcsr;
    TLSSetUserTraceContext( NULL );
}

LOCAL VOID ErrFILEIndexDispatchSortTerm(
    const DWORD                 dwError,
    const DWORD_PTR             dwParam1,
    const DWORD                 dwParam2,
    const DWORD_PTR             dwParam3
    )
{
    CREATEINDEXCONTEXT * const  pidxcontext     = (CREATEINDEXCONTEXT *)dwParam1;
    FCBINDEXCONTEXT * const     pfcbcontext     = (FCBINDEXCONTEXT *)dwParam3;

    // Establish user trace context because this code runs on a threadpool thread
    Assert( PutcTLSGetUserContext() == NULL );
    pidxcontext->pfucbTable->ppib->SetUserTraceContextInTls();
    PIBTraceContextScope tcScope = pidxcontext->pfucbTable->ppib->InitTraceContextScope();
    tcScope->iorReason.SetIort( iortSort );

    //  only proceed if previous tasks did not encounter an error
    //
    if ( JET_errSuccess == pidxcontext->err )
    {
        BOOL    fCorrupt    = fFalse;
        ERR     errT        = ErrFILESingleIndexTerm(
                                        pidxcontext->pfucbTable->ppib,
                                        pidxcontext->rgpfucbSort,
                                        pfcbcontext->pfcbIndex,
                                        pidxcontext->rgcRecInput,
                                        pfcbcontext->iindex,
                                        pidxcontext->fCheckOnly ? &fCorrupt : NULL,
                                        pidxcontext->pcprintf,
                                        pidxcontext->fLogged );
        if ( errT >= JET_errSuccess )
        {
            if ( fCorrupt )
            {
                pidxcontext->fCorruptionEncountered = fTrue;
            }

            if ( pidxcontext->pstatus )
            {
                //  HACK: use DDL/DML latch to serialize progress meter update
                //
                pidxcontext->pfucbTable->u.pfcb->EnterDDL();
                errT = ErrFILEIndexProgress( pidxcontext->pstatus );
                pidxcontext->pfucbTable->u.pfcb->LeaveDDL();
            }
        }
        if ( errT < JET_errSuccess )
        {
            pidxcontext->err = errT;
        }
    }

    TLSSetUserTraceContext( NULL );
}

LOCAL ERR ErrFILEITerminateSorts(
    CREATEINDEXCONTEXT **   rgpidxcreate,
    FCB *                   pfcbIndexes,
    const ULONG             cIndexes,
    const ULONG             cProcs )
{
    ERR                     err;
    CTaskManager            taskmgrTerminateSorts;
    FCBINDEXCONTEXT *       rgfcbcontext        = NULL;

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat( "About to terminate sorts using %d threads.\n", cProcs ) );
#endif

    CallR( taskmgrTerminateSorts.ErrTMInit( cProcs, (DWORD_PTR *)rgpidxcreate, fTrue ) );

    Alloc( rgfcbcontext = (FCBINDEXCONTEXT *)PvOSMemoryHeapAlloc( sizeof(FCBINDEXCONTEXT) * cIndexes ) );

    //  sort and generate index keys
    //
    for ( ULONG iindex = 0; iindex < cIndexes; iindex++, pfcbIndexes = pfcbIndexes->PfcbNextIndex() )
    {
        rgfcbcontext[iindex].pfcbIndex = pfcbIndexes;
        rgfcbcontext[iindex].iindex = iindex;
        Call( taskmgrTerminateSorts.ErrTMPost( ErrFILEIndexDispatchSortTerm, 0, (DWORD_PTR)( rgfcbcontext + iindex ) ) );
    }

    err = JET_errSuccess;

HandleError:
    taskmgrTerminateSorts.TMTerm();

    if ( NULL != rgfcbcontext )
    {
        OSMemoryHeapFree( rgfcbcontext );
    }

    return err;
}

INLINE VOID FILEIPossiblyWaitForDispatchedTasks( const CREATEINDEXCONTEXT * const pidxcontext, const ULONG iProc )
{
    //  if we're latching too many pages, wait for some tasks to complete
    //
    ULONG_PTR   cbfCacheT       = 0;
    ULONG       cTasksPending   = pidxcontext->taskmgrCreateIndex.CPostedTasks();

    //  snapshot current buffer manager size
    CallS( ErrBFGetCacheSize( &cbfCacheT ) );

    //  tasks implicitly match to page latches, so we'll
    //  compare the number of pending tasks to the current
    //  buffer manager size to determine if we need to wait
    //
    //  if this thread is hogging over half the cached pages,
    //  wait for it to finish some work
    //
    if ( cTasksPending > SIZE_T( cbfCacheT * 2 / 3  ) )
    {
        ULONG   csecWait    = 1;
        for ( ULONG citer = 1; cTasksPending > SIZE_T( cbfCacheT / 3 ); citer++ )
        {
#ifdef SHOW_INDEX_PERF
            OSTrace( tracetagIndexPerf, OSFormat(   "Pausing scanning thread for %d seconds to wait for task manager %d (retry #%d, %d tasks pending, cbfCache==%d).\n",
                                                csecWait,
                                                iProc+1,
                                                citer,
                                                cTasksPending,
                                                cbfCacheT ) );
#endif

            UtilSleep( csecWait * 1000 );

            //  increment wait if we have to retry again
            //  (capping the wait time at 30 seconds)
            //
            if ( csecWait < 30 )
                csecWait *= 2;

            cTasksPending = pidxcontext->taskmgrCreateIndex.CPostedTasks();
        }
    }
}

// USED BY PARALLEL INDEX BUILD AND
// BY OFFLINE DEFRAG WITH MORE THEN 16 INDICES
//
ERR ErrFILEBuildAllIndexes(
    PIB * const             ppib,
    FUCB * const            pfucbTable,
    FCB * const             pfcbIndexes,
    STATUSINFO * const      pstatus,
    const ULONG             cIndexBatchMaxRequested,
    const BOOL              fCheckOnly,
    CPRINTF * const         pcprintf )
{
    ERR                     err                         = JET_errSuccess;
    DIB                     dib;
    INST *                  pinst                       = PinstFromPpib( ppib );
    ULONG                   cProcs                      = 0;
    ULONG                   cIndexBatchMax              = 0;
    DWORD_PTR               cSCBMax                     = 0;
    DWORD_PTR               cSCBInUse                   = 0;
    CSR *                   pcsrTable                   = Pcsr( pfucbTable );
    CREATEINDEXCONTEXT *    rgidxcontext                = NULL;
    FUCB **                 rgpfucbSort                 = NULL;
    FCB *                   pfcbIndexesToBuild          = NULL;
    FCB *                   pfcbNextBuildIndex          = NULL;
    ULONG                   cIndexesToBuild             = 0;
    BOOL                    fTransactionStarted         = fFalse;
    BOOL                    fCorruptionEncountered      = fFalse;
    CSR *                   pcsrT                       = pcsrNil;
    ULONG                   iProc                       = 0;
    ULONG_PTR               ulCacheSizeMaxSave          = 0;
    ULONG_PTR               ulStartFlushThresholdSave   = 0;
    ULONG_PTR               ulStopFlushThresholdSave    = 0;
    CPG                     cpgTable                    = 0;
    const CPG               cpgDbExtensionSizeSave      = (CPG)UlParam( pinst, JET_paramDbExtensionSize );
    const BOOL              fLogged                     = ( !BoolParam( pinst, JET_paramCircularLog )
                                                            && g_rgfmp[pfucbTable->ifmp].FLogOn() );

    Assert( !fLogged || !pinst->m_plog->FLogDisabled() );

    //  just to be safe, don't use up all available sorts
    //
    CallS( ErrRESGetResourceParam( pinst, JET_residSCB, JET_resoperMaxUse, &cSCBMax ) );
    CallS( ErrRESGetResourceParam( pinst, JET_residSCB, JET_resoperCurrentUse, &cSCBInUse ) );
    cIndexBatchMax = min( cIndexBatchMaxRequested, ULONG( ( cSCBMax - cSCBInUse ) * 0.9 ) );
    if ( 0 == cIndexBatchMax )
    {
        cIndexBatchMax = 1;
    }

    // Set the trace context
    //
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    tcScope->nParentObjectClass = TceFromFUCB( pfucbTable );
    tcScope->SetDwEngineObjid( ObjidFDP( pfucbTable ) );

    //  save off current buffer settings as we will be resizing
    //  the buffer manager during the index rebuild
    //
    ulCacheSizeMaxSave = UlParam( JET_paramCacheSizeMax );
    ulStartFlushThresholdSave = UlParam( JET_paramStartFlushThreshold );
    ulStopFlushThresholdSave = UlParam( JET_paramStopFlushThreshold );

    if ( 0 == ppib->Level() )
    {
        //  only reason we need a transaction is to ensure read-consistency of primary data
        CallR( ErrDIRBeginTransaction( ppib, 34085, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }

    const ULONG_PTR     ulStartFlushThresholdForIndexCreate     = 5 * ulCacheSizeMaxSave / 100;
    const ULONG_PTR     ulStopFlushThresholdForIndexCreate      = 6 * ulCacheSizeMaxSave / 100;

    //  have to do this comparison to ensure
    //  settings are always valid
    //
    if ( ulStopFlushThresholdForIndexCreate > ulStopFlushThresholdSave )
    {
        CallS( Param( pinstNil, JET_paramStopFlushThreshold )->Set( pinstNil, ppibNil, ulStopFlushThresholdForIndexCreate, NULL ) );
        CallS( Param( pinstNil, JET_paramStartFlushThreshold )->Set( pinstNil, ppibNil, ulStartFlushThresholdForIndexCreate, NULL ) );
    }
    else
    {
        CallS( Param( pinstNil, JET_paramStartFlushThreshold )->Set( pinstNil, ppibNil, ulStartFlushThresholdForIndexCreate, NULL ) );
        CallS( Param( pinstNil, JET_paramStopFlushThreshold )->Set( pinstNil, ppibNil, ulStopFlushThresholdForIndexCreate, NULL ) );
    }
    
    //  set DB extension size to 1% of the current size of the table, up to
    //  a limit (I somewhat arbitrarily chose the limit to be a number of pages
    //  equal to the page size)
    //
    Call( ErrSPGetInfo(
              ppib,
              pfucbTable->ifmp,
              pfucbTable,
              (BYTE *)&cpgTable,
              sizeof(cpgTable),
              fSPOwnedExtent,
              gci::Allow ) );

    Call( Param( pinst, JET_paramDbExtensionSize )->Set( pinst, ppibNil, max( cpgDbExtensionSizeSave, (CPG)min( g_cbPage, cpgTable / 100 ) ), NULL ) );

    //  if we have a lot of indices to rebuild, make sure we
    //  have enough temp disk space to accommodate the request
    //
    if ( cIndexBatchMax > cFILEIndexBatchSizeDefault )
    {
        const QWORD     cbTable         = QWORD( cpgTable ) * QWORD( g_cbPage );
        CPG             cpgFreeTempDb;
        QWORD           cbFreeTempDisk;

        Call( ErrEnsureTempDatabaseOpened( pinst, ppib ) );

        //  first check if we have enough free space for potential temp. database growth
        //
        Call( pinst->m_pfsapi->ErrDiskSpace( SzParam( pinst, JET_paramTempPath ), &cbFreeTempDisk ) );

        //  add on any free space in the temp. db, since that's potentially re-usable
        //
        Call( ErrSPGetInfo(
                  ppib,
                  pinst->m_mpdbidifmp[ dbidTemp ],
                  pfucbNil,
                  (BYTE *)&cpgFreeTempDb,
                  sizeof(cpgFreeTempDb),
                  fSPAvailExtent,
                  gci::Forbid ) ); // gci::Allow doesn't necessarily include pages in split buffers.
        
        cbFreeTempDisk += ( cpgFreeTempDb * g_cbPage );

        if ( cbFreeTempDisk > QWORD( cbTable * 0.75 ) )
        {
            //  plenty of disk space for the temp database to grow (relative
            //  to the size of the table), so permit building as many indices
            //  as requested
            //
            NULL;
        }
        else
        {
            //  assume that on average, each index is going to be about 1%
            //  of the size of the table, so use that assumption to come
            //  up with an index batch size to use
            //
            cIndexBatchMax = min( cIndexBatchMax, ULONG( cbFreeTempDisk * 100 / cbTable ) );

            //  if more than 75%, should have been permitted to build all indices
            //
            Assert( cIndexBatchMax <= 75 );

            //  enforce minimum batch size
            //
            cIndexBatchMax = max( cIndexBatchMax, cFILEIndexBatchSizeDefault );
        }
    }

    //  now check if we have enough free space for potential log disk growth
    //
    if ( cIndexBatchMax > cFILEIndexBatchSizeDefault
        && g_rgfmp[pfucbTable->ifmp].FLogOn() )
    {
        if ( BoolParam( pinst, JET_paramCircularLog ) )
        {
            const QWORD     cbTable         = QWORD( cpgTable ) * QWORD( g_cbPage );
            QWORD           cbFreeLogDisk;

            //  if circular logging enabled, then we're not going to log
            //  the actual index entries, but we are actually going to log
            //  space operations, so need a small amount of log disk space
            //
            Call( pinst->m_pfsapi->ErrDiskSpace( SzParam( pinst, JET_paramLogFilePath ), &cbFreeLogDisk ) );
            if ( cbFreeLogDisk > QWORD( cbTable * 0.001 ) )
            {
                //  plenty of disk space for logging space operations (relative
                //  to the size of the table), so permit building as many indices
                //  as requested
                //
                NULL;
            }
            else
            {
                //  there's a really small amount of free log disk space relative
                //  to the size of the table, so rather than try to come up with
                //  a formula for this, I'm just going to set the batch size to
                //  the default batch size
                //
                cIndexBatchMax = cFILEIndexBatchSizeDefault;
            }
        }
        else
        {
            //  if circular logging is disabled, we'll use the same amount of
            //  log disk space regardless of batch size, so don't bother trimming
            //  the index batch size
            //
            NULL;
        }
    }

    //  use all procs unless there are more procs than indices to build
    //
    cProcs = min( CUtilProcessProcessor(), cIndexBatchMax );

    //  open LV root so the async threads don't do it
    //
    if( pfcbNil == pfucbTable->u.pfcb->Ptdb()->PfcbLV() )
    {
        FUCB *  pfucbT  = pfucbNil;
        Call( ErrFILEOpenLVRoot( pfucbTable, &pfucbT, fFalse ) );   //  UNDONE: use ErrDIROpenLongRoot instead
        if ( pfucbNil != pfucbT )   //  may be NULL if no LV tree
            DIRClose( pfucbT );
    }

    Assert( cIndexBatchMax > 0 );

    ppib->SetFBatchIndexCreation();

    if ( !fLogged )
    {
        for ( FCB * pfcbT = pfcbIndexes; pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( !pfcbT->FDontLogSpaceOps() );
            pfcbT->Lock();
            pfcbT->SetDontLogSpaceOps();
            pfcbT->Unlock();
        }
    }

    //  allocate space for index contexts, index context pointers, primary key buffers, and secondary key buffers
    Alloc( rgidxcontext = (CREATEINDEXCONTEXT *)PvOSMemoryHeapAlloc( ( sizeof(CREATEINDEXCONTEXT) + sizeof(CREATEINDEXCONTEXT *) + cbKeyAlloc + cbKeyAlloc ) * cProcs ) );
    memset( rgidxcontext, 0, sizeof(CREATEINDEXCONTEXT) * cProcs );

    //  allocate space for sort cursors and entry counts
    Alloc( rgpfucbSort = (FUCB **)PvOSMemoryHeapAlloc( ( sizeof(FUCB *) + sizeof(ULONG) ) * cIndexBatchMax ) );
    memset( rgpfucbSort, 0, sizeof(FUCB *) * cIndexBatchMax );

    //  arrays of index context pointers, primary key buffers, and secondary key buffers
    //  hangs off the end of the array of index contexts
    CREATEINDEXCONTEXT **   rgpidxcontext   = (CREATEINDEXCONTEXT **)( (BYTE *)rgidxcontext + ( sizeof(CREATEINDEXCONTEXT) * cProcs ) );
    BYTE * const            rgbBMBuffer     = (BYTE *)rgpidxcontext + ( sizeof(CREATEINDEXCONTEXT *) * cProcs );
    BYTE * const            rgbKeyBuffer    = rgbBMBuffer + ( cbKeyAlloc * cProcs );

    //  array of entry counts hangs off the end of the array of sort cursors
    ULONG *                 rgcRecInput     = (ULONG *)( (BYTE *)rgpfucbSort + ( sizeof(FUCB *) * cIndexBatchMax ) );


    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;

    FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
    err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
    Assert( JET_errNoCurrentRecord != err );
    if ( err < JET_errSuccess )
    {
        if ( JET_errRecordNotFound == err )
        {
            //  the tree is empty
            err = JET_errSuccess;
        }
        goto HandleError;
    }

    // Cursor building index should only be navigating primary index.
    Assert( pfucbNil == pfucbTable->pfucbCurIndex );

    // Don't waste time by calling this function when unneeded.
    Assert( pfcbNil != pfcbIndexes );


    for ( iProc = 0; iProc < cProcs; iProc++ )
    {
        CREATEINDEXCONTEXT *    pidxcontext     = rgidxcontext + iProc;
        FUCB *                  pfucbT;

        new( &pidxcontext->taskmgrCreateIndex ) CTaskManager;
        pidxcontext->fAllocatedTaskManager = fTrue;

        if ( fLogged )
            pidxcontext->fLogged = fTrue;

        if ( fCheckOnly )
            pidxcontext->fCheckOnly = fTrue;

        //  open session/cursor for this thread and place it at
        //  the beginning of the table
        //
        Call( ErrDIROpen( ppib, pfucbTable->u.pfcb, &pfucbT ) );
        FUCBSetIndex( pfucbT );
        FUCBSetMayCacheLVCursor( pfucbT );

        pidxcontext->pfucbTable = pfucbT;
        pidxcontext->rgpfucbSort = rgpfucbSort;
        pidxcontext->rgcRecInput = rgcRecInput;
        pidxcontext->dataBMBuffer.SetCb( cbKeyAlloc );
        pidxcontext->dataBMBuffer.SetPv( rgbBMBuffer + ( cbKeyAlloc * iProc ) );
        pidxcontext->keyBuffer.suffix.SetCb( cbKeyAlloc );
        pidxcontext->keyBuffer.suffix.SetPv( rgbKeyBuffer + ( cbKeyAlloc * iProc ) );

        pidxcontext->pstatus = pstatus;
        pidxcontext->pcprintf = pcprintf;

        rgpidxcontext[iProc] = pidxcontext;
    }

    pfcbNextBuildIndex = pfcbIndexes;

NextBuild:
#ifdef SHOW_INDEX_PERF
    ULONG   cRecs, cPages;
    TICK    tickStart;
    cRecs = 0;
    cPages  = 0;
    tickStart   = TickOSTimeCurrent();
#endif

    //  HACK: force locLogical to OnCurBM to silence
    //  RECIRetrieveKey(), which will make DIR-level
    //  calls using this cursor
    pfucbTable->locLogical = locOnCurBM;

    pfcbIndexesToBuild = pfcbNextBuildIndex;
    pfcbNextBuildIndex = pfcbNil;

    Call( ErrFILEIndexBatchInit(
                ppib,
                rgpfucbSort,
                pfcbIndexesToBuild,
                &cIndexesToBuild,
                rgcRecInput,
                &pfcbNextBuildIndex,
                cIndexBatchMax ) );

    // This is either a full batch or the last batch of indexes to build.
    Assert( cIndexBatchMax == cIndexesToBuild
        || pfcbNil == pfcbNextBuildIndex );

    //  re-initialise task manager with new index batch
    //
    ULONG   cProcsCurrBatch;
    ULONG   iindexNext;
    FCB *   pfcbIndexesRemaining;

    cProcsCurrBatch = min( cProcs, cIndexesToBuild );
    iindexNext = 0;
    pfcbIndexesRemaining = pfcbIndexesToBuild;

    for ( iProc = 0; iProc < cProcsCurrBatch; iProc++ )
    {
        Assert( iindexNext < cIndexesToBuild );

        const ULONG             cIndexesRemaining   = cIndexesToBuild - iindexNext;
        const ULONG             cProcsRemaining     = cProcsCurrBatch - iProc;
        ULONG                   cIndexesThisProc    = ( cIndexesRemaining + cProcsRemaining - 1 ) / cProcsRemaining;
        CREATEINDEXCONTEXT *    pidxcontext         = rgidxcontext + iProc;

        Assert( cIndexesThisProc > 0 );

        Call( pidxcontext->taskmgrCreateIndex.ErrTMInit( 1, (DWORD_PTR *)( rgpidxcontext + iProc ) ) );

        pidxcontext->iindexStart = iindexNext;
        pidxcontext->cIndexesToBuild = cIndexesThisProc;
        pidxcontext->pfcbIndexesToBuild = pfcbIndexesRemaining;

        iindexNext += cIndexesThisProc;

        for ( ULONG iindex = 0; iindex < cIndexesThisProc; iindex++ )
        {
            pfcbIndexesRemaining = pfcbIndexesRemaining->PfcbNextIndex();
        }

        DIRUp( pidxcontext->pfucbTable );
        pidxcontext->err = JET_errSuccess;
    }

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Beginning scan of records to rebuild %d indexes with %d task managers.\n", cIndexesToBuild, cProcsCurrBatch ) );
    OSTraceIndent( tracetagIndexPerf, +1 );

    FCB *   pfcbIndexT;
    ULONG   iindexT;
    for ( pfcbIndexT = pfcbIndexesToBuild, iindexT = 0; iindexT < cIndexesToBuild; pfcbIndexT = pfcbIndexT->PfcbNextIndex(), iindexT++ )
    {
        TDB *   ptdbT   = pfucbTable->u.pfcb->Ptdb();
        IDB *   pidbT   = pfcbIndexT->Pidb();

        OSTrace( tracetagIndexPerf, OSFormat(   "Index #%d: %s\n",
                                            iindexT+1,
                                            ptdbT->SzIndexName( pidbT->ItagIndexName() ) ) );
    }
    OSTraceIndent( tracetagIndexPerf, -1 );
#endif

    //  build up new index in a sort file
    //

    // The PIB here is going to be used on multiple threads at the same time
    // when we post index creation tasks.
    // Set its cached TLS pointer to NULL to force an actual TLS lookup
    ppib->ptlsApi = NULL;

    CallS( err );
    while ( JET_errNoCurrentRecord != err )
    {
        Call( err );

#ifdef SHOW_INDEX_PERF
        cPages++;
        cRecs += pcsrTable->Cpage().Clines();   //  this is an estimate because we don't know how many are deleted      cPages++;
#endif

        Assert( pcsrTable->FLatched() );
        Assert( locOnCurBM == pfucbTable->locLogical );

        for ( iProc = 0; iProc < cProcsCurrBatch; iProc++ )
        {
            CREATEINDEXCONTEXT * const  pidxcontext     = rgidxcontext + iProc;

            //  verify no errors encountered on previous tasks
            //
            Call( pidxcontext->err );

            //  this loop can take some time, so see if we need
            //  to terminate because of shutdown
            //
            Call( pinst->ErrCheckForTermination() );

            //  check if the log disk is full, because even
            //  though we're still in the scanning phase,
            //  we won't be able to insert keys into the
            //  index if we can't log the insertion
            //
            //  NOTE: must check even if fLogged is FALSE
            //  because we may be in the pseudo-logging-disabled
            //  state where logging is actually enabled, but
            //  we're not logging batch index creation, but
            //  we are still logging space operations, so we may
            //  end up filling the logs with that
            //
            Call( pinst->m_plog->ErrLGCheckState() );

            //  create a new CSR for this task
            //
            Alloc( pcsrT = new CSR );

            //  latch the page for the task
            //
            //  NOTE:  we disable ownership tracking here because we are
            //  acquiring read latches for other threads
            //
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            Call( pcsrT->ErrGetReadPage(
                            ppib,
                            pfucbTable->ifmp,
                            pcsrTable->Pgno(),
                            bflfNoTouch,
                            pcsrTable->Cpage().PBFLatch() ) );
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();

            //  post the task with the page latched to
            //  ensure it doesn't go away
            //
            Call( pidxcontext->taskmgrCreateIndex.ErrTMPost(
                                    ErrFILEIndexDispatchAddEntries,
                                    0,
                                    (DWORD_PTR)pcsrT ) );

            //  memory will be freed after task has been dispatched
            //
            pcsrT = pcsrNil;

            FILEIPossiblyWaitForDispatchedTasks( pidxcontext,iProc );
        }

        //  HACK! HACK! HACK!
        //  Force currency to the last node on the page so that the subsequent call
        //  to ErrBTNext() will skip to the next page.  The individual nodes on the
        //  current page will be handled by the dispatched tasks.
        //
        pcsrTable->SetILine( pcsrTable->Cpage().Clines() - 1 );
#ifdef DEBUG
        NDGet( pfucbTable );
#endif

        Assert( pcsrTable->FLatched() );
        err = ErrBTNext( pfucbTable, fDIRNull );
    }
    Assert( JET_errNoCurrentRecord == err );

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Scanned %d records (%d pages) and dispatched all tasks in %d msecs.\n",
                                        cRecs,
                                        cPages,
                                        TickOSTimeCurrent() - tickStart ) );
#endif

    FUCBResetPreread( pfucbTable );
    BTUp( pfucbTable );

    //  wait for sorts to finish
    //
    for ( iProc = 0; iProc < cProcsCurrBatch; iProc++ )
    {
        CREATEINDEXCONTEXT *    pidxcontext     = rgidxcontext + iProc;

        pidxcontext->taskmgrCreateIndex.TMTerm();

        DIRUp( pidxcontext->pfucbTable );
        Call( pidxcontext->err );

#ifdef SHOW_INDEX_PERF
        ULONG   cTotalKeys  = 0;
        for ( ULONG iindex = pidxcontext->iindexStart;
            iindex < pidxcontext->iindexStart + pidxcontext->cIndexesToBuild;
            iindex++ )
        {
            if ( rgpfucbSort[iindex]->u.pscb->crun > 0 )
            {
                OSTrace( tracetagIndexPerf, OSFormat(   "Index #%d used sort runs [objid:0x%x,pgnoFDP:0x%x].\n",
                                                    iindex+1,
                                                    rgpfucbSort[iindex]->u.pscb->fcb.ObjidFDP(),
                                                    rgpfucbSort[iindex]->u.pscb->fcb.PgnoFDP() ) );
            }
            else
            {
                OSTrace( tracetagIndexPerf, OSFormat(   "Index #%d did not use sort runs.\n",
                                                    iindex+1 ) );
            }

            cTotalKeys += rgcRecInput[iindex];
        }
        OSTrace( tracetagIndexPerf, OSFormat( "Task manager %d generated %d total keys.\n", iProc+1, cTotalKeys ) );
#endif
    }

    Call( ErrFILEITerminateSorts( rgpidxcontext, pfcbIndexesToBuild, cIndexesToBuild, cProcsCurrBatch ) );

    //  check for errors and set corruption flag if necessary
    //
    for ( iProc = 0; iProc < cProcsCurrBatch; iProc++ )
    {
        CREATEINDEXCONTEXT *    pidxcontext     = rgidxcontext + iProc;

        Call( pidxcontext->err );

        if ( fCheckOnly && pidxcontext->fCorruptionEncountered )
        {
            fCorruptionEncountered = fTrue;
        }
    }

    if ( pfcbNil != pfcbNextBuildIndex )
    {
        //  reseek to beginning of data (must succeed since it succeeded the first time)
        FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
        err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
        CallS( err );
        Call( err );

        goto NextBuild;
    }

    if ( fCorruptionEncountered )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbTable ), HaDbFailureTagCorruption, L"476cb08b-2aa4-49d6-b9ed-0ec536585fd7" );
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    else if ( !fLogged
        && g_rgfmp[pfucbTable->ifmp].FLogOn() )
    {
        //  index creation wasn't logged, but
        //  logging in general is enabled, so must
        //  force-flush database and guarantee it
        //  succeeded
        //
#ifdef SHOW_INDEX_PERF
        OSTrace( tracetagIndexPerf, "About to force-flush entire database after index creation.\n" );
#endif

        Call( ErrBFFlush( pfucbTable->ifmp ) );
        Enforce( JET_errSuccess == err );

        //  HACK: must force a dummy logged update anywhere in this
        //  database to ensure that after recovery, the dbtime in the
        //  db header is greater than any of the dbtimes in the non-
        //  logged indexes
        //
        Call( ErrSPDummyUpdate( pfucbTable ) );

#ifdef SHOW_INDEX_PERF
        OSTrace( tracetagIndexPerf, "Finished force-flush of entire database after index creation.\n" );
#endif
    }
    else
    {
        err = JET_errSuccess;
    }

HandleError:
#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat( "Completed batch index build with error %d.\n", err ) );
#endif

    Assert ( err != errDIRNoShortCircuit );

    if ( NULL != rgidxcontext )
    {
        for ( iProc = 0; iProc < cProcs; iProc++ )
        {
            CREATEINDEXCONTEXT * const  pidxcontext     = rgidxcontext + iProc;

            if ( pidxcontext->fAllocatedTaskManager )
            {
                pidxcontext->taskmgrCreateIndex.TMTerm();
                pidxcontext->taskmgrCreateIndex.~CTaskManager();
            }
            if ( pfucbNil != pidxcontext->pfucbTable )
            {
                DIRCloseIfExists( &pidxcontext->pfucbTable->pfucbLV );
                DIRClose( pidxcontext->pfucbTable );
            }
        }
        OSMemoryHeapFree( rgidxcontext );
    }

    if ( err < 0 )
    {
        if ( pcsrNil != pcsrT )
        {
            //  NOTE:  we disable ownership tracking here because we are
            //  releasing read latches we acquired for other threads
            //
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            pcsrT->ReleasePage();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
            delete pcsrT;
        }

        if ( NULL != rgpfucbSort )
        {
            for ( ULONG iindex = 0; iindex < cIndexBatchMax; iindex++ )
            {
                if ( pfucbNil != rgpfucbSort[iindex] )
                {
                    SORTClose( rgpfucbSort[iindex] );
                    rgpfucbSort[iindex] = pfucbNil;
                }
            }
        }

        FUCBResetPreread( pfucbTable );
        BTUp( pfucbTable );
    }
    else
    {
#ifdef DEBUG
        Assert( pcsrNil == pcsrT );
        for ( ULONG iindex = 0; iindex < cIndexBatchMax; iindex++ )
        {
            Assert( pfucbNil == rgpfucbSort[iindex] );
        }
#endif
    }

    if ( NULL != rgpfucbSort )
    {
        OSMemoryHeapFree( rgpfucbSort );
    }

    if ( fTransactionStarted )
    {
        //  read-only transaction, so should never fail
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }


    //  restore original buffer manager thresholds
    //
    if ( ulStopFlushThresholdSave > ulStopFlushThresholdForIndexCreate )
    {
        CallS( Param( pinstNil, JET_paramStopFlushThreshold )->Set( pinstNil, ppibNil, ulStopFlushThresholdSave, NULL ) );
        CallS( Param( pinstNil, JET_paramStartFlushThreshold )->Set( pinstNil, ppibNil, ulStartFlushThresholdSave, NULL ) );
    }
    else
    {
        CallS( Param( pinstNil, JET_paramStartFlushThreshold )->Set( pinstNil, ppibNil, ulStartFlushThresholdSave, NULL ) );
        CallS( Param( pinstNil, JET_paramStopFlushThreshold )->Set( pinstNil, ppibNil, ulStopFlushThresholdSave, NULL ) );
    }

    if ( !fLogged )
    {
        for ( FCB * pfcbT = pfcbIndexes; pfcbNil != pfcbT; pfcbT = pfcbT->PfcbNextIndex() )
        {
            pfcbT->Lock();
            pfcbT->ResetDontLogSpaceOps();
            pfcbT->Unlock();
        }
    }

    //  restore original DB extension size
    //
    CallS( Param( pinst, JET_paramDbExtensionSize )->Set( pinst, ppibNil, cpgDbExtensionSizeSave, NULL ) );

    ppib->ResetFBatchIndexCreation();

    return err;
}

#else

ERR ErrFILEIndexBatchTerm(
    PIB         * const ppib,
    FUCB        ** const rgpfucbSort,
    FCB         * const pfcbIndexesToBuild,
    const ULONG cIndexesToBuild,
    ULONG       * const rgcRecInput,
    STATUSINFO  * const pstatus,
    BOOL        *pfCorruptionEncountered,
    CPRINTF     * const pcprintf )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucbIndex = pfucbNil;
    FCB         *pfcbIndex = pfcbIndexesToBuild;

    Assert( pfcbNil != pfcbIndexesToBuild );
    Assert( cIndexesToBuild > 0 );

    for ( ULONG iindex = 0; iindex < cIndexesToBuild; iindex++, pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        const BOOL  fUnique         = pfcbIndex->Pidb()->FUnique();
        ULONG       cRecOutput      = 0;
        BOOL        fEntriesExist   = fTrue;
        BOOL        fCorruptedIndex = fFalse;

        Assert( pfcbNil != pfcbIndex );
        Assert( pfcbIndex->FTypeSecondaryIndex() );
        Assert( pidbNil != pfcbIndex->Pidb() );

#ifdef SHOW_INDEX_PERF
        const TICK  tickStart       = TickOSTimeCurrent();

        OSTrace( tracetagIndexPerf, OSFormat( "About to sort keys for index %d.\n", iindex+1 ) );
#endif

        Call( ErrSORTEndInsert( rgpfucbSort[iindex] ) );

#ifdef SHOW_INDEX_PERF
        OSTrace( tracetagIndexPerf, OSFormat(   "Sorted keys for index %d in %d msecs.\n",
                                            iindex+1,
                                            TickOSTimeCurrent() - tickStart ) );
#endif

        //  transfer index entries to actual index
        //  insert first one in normal method!
        //
        err = ErrSORTNext( rgpfucbSort[iindex] );
        if ( err < 0 )
        {
            if ( JET_errNoCurrentRecord != err )
                goto HandleError;

            err = JET_errSuccess;

            Assert( rgcRecInput[iindex] == 0 );
            fEntriesExist = fFalse;
        }

        if ( fEntriesExist )
        {

            //  open cursor on index
            //
            Assert( pfucbNil == pfucbIndex );
            Call( ErrDIROpen( ppib, pfcbIndex, &pfucbIndex ) );
            FUCBSetIndex( pfucbIndex );
            FUCBSetSecondary( pfucbIndex );
            DIRGotoRoot( pfucbIndex );

            if ( NULL != pfCorruptionEncountered )
            {
                Assert( NULL != pcprintf );
                Call( ErrFILEICheckIndex(
                            rgpfucbSort[iindex],
                            pfucbIndex,
                            &cRecOutput,
                            &fCorruptedIndex,
                            pcprintf ) );
                if ( fCorruptedIndex )
                    *pfCorruptionEncountered = fTrue;
            }
            else
            {
#ifdef SHOW_INDEX_PERF
                const TICK  tickStart   = TickOSTimeCurrent();
#endif

                Call( ErrFILEIAppendToIndex(
                            rgpfucbSort[iindex],
                            pfucbIndex,
                            &cRecOutput,
                            g_rgfmp[pfucbIndex->ifmp].FLogOn() ) );

#ifdef SHOW_INDEX_PERF
                OSTraceIndent( tracetagIndexPerf, +1 );
                OSTrace( tracetagIndexPerf, OSFormat(   "Appended %d keys for index %d in %d msecs.\n",
                                                    cRecOutput,
                                                    iindex+1,
                                                    TickOSTimeCurrent() - tickStart ) );
                OSTraceIndent( tracetagIndexPerf, -1 );
#endif
            }

            DIRClose( pfucbIndex );
            pfucbIndex = pfucbNil;
        }
        else
        {
#ifdef SHOW_INDEX_PERF
            OSTraceIndent( tracetagIndexPerf, +1 );
            OSTrace( tracetagIndexPerf, OSFormat( "No keys generated for index %d.\n", iindex+1 ) );
            OSTraceIndent( tracetagIndexPerf, -1 );
#endif
        }

        SORTClose( rgpfucbSort[iindex] );
        rgpfucbSort[iindex] = pfucbNil;

        Assert( cRecOutput <= rgcRecInput[iindex] );
        if ( cRecOutput != rgcRecInput[iindex] )
        {
            if ( cRecOutput < rgcRecInput[iindex] && !fUnique )
            {
                //  duplicates over multi-valued columns must have been removed
            }

            else if ( NULL != pfCorruptionEncountered )
            {
                *pfCorruptionEncountered = fTrue;

                // Only report this error if we haven't
                // encountered corruption on this index already.
                if ( !fCorruptedIndex )
                {
                    if ( cRecOutput > rgcRecInput[iindex] )
                    {
                        (*pcprintf)( "Too many index keys generated\n" );
                    }
                    else
                    {
                        Assert( fUnique );
                        (*pcprintf)( "%d duplicate key(s) on unique index\n", rgcRecInput[iindex] - cRecOutput );
                    }
                }
            }

            else if ( cRecOutput > rgcRecInput[iindex] )
            {
                err = ErrERRCheck( JET_errIndexBuildCorrupted );
                goto HandleError;
            }

            else
            {
                Assert( cRecOutput < rgcRecInput[iindex] );
                Assert( fUnique );

                //  If we got dupes and the index was NOT unique, it must have been
                //  dupes over multi-value columns, in which case the dupes
                //  get properly eliminated.
                err = ErrERRCheck( JET_errKeyDuplicate );
                goto HandleError;
            }
        }

        if ( pstatus )
        {
            Call( ErrFILEIndexProgress( pstatus ) );
        }
    }

HandleError:
    if ( pfucbNil != pfucbIndex )
    {
        Assert( err < 0 );
        DIRClose( pfucbIndex );
    }

    return err;
}


ERR ErrFILEBuildAllIndexes(
    PIB         * const ppib,
    FUCB        * const pfucbTable,
    FCB         * const pfcbIndexes,
    STATUSINFO  * const pstatus,
    const ULONG cIndexBatchMax,
    const BOOL  fCheckOnly,
    CPRINTF     * const pcprintf )
{
    ERR         err;
    DIB         dib;
    FUCB        *rgpfucbSort[cFILEIndexBatchSizeDefault];
    FCB         *pfcbIndexesToBuild;
    FCB         *pfcbNextBuildIndex;
    KEY         keyBuffer;
    BYTE        *pbKey                  = NULL;
    ULONG       rgcRecInput[cFILEIndexBatchSizeDefault];
    ULONG       cIndexesToBuild;
    ULONG       iindex;
    BOOL        fTransactionStarted     = fFalse;
    BOOL        fCorruptionEncountered  = fFalse;
    const BOOL  fLogged                 = g_rgfmp[pfucbTable->ifmp].FLogOn();
    INST        * const pinst           = PinstFromPpib( ppib );

    if ( 0 == ppib->Level() )
    {
        //  only reason we need a transaction is to ensure read-consistency of primary data
        CallR( ErrDIRBeginTransaction( ppib, 50469, NO_GRBIT ) );
        fTransactionStarted = fTrue;
    }

    Assert( cIndexBatchMax > 0 );
    Assert( cIndexBatchMax <= cFILEIndexBatchSizeDefault );
    for ( iindex = 0; iindex < cIndexBatchMax; iindex++ )
        rgpfucbSort[iindex] = pfucbNil;

    Alloc( pbKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyBuffer.prefix.Nullify();
    keyBuffer.suffix.SetCb( cbKeyAlloc );
    keyBuffer.suffix.SetPv( pbKey );

    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;

    FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
    err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
    Assert( JET_errNoCurrentRecord != err );
    if( JET_errRecordNotFound == err )
    {
        //  the tree is empty
        err = JET_errSuccess;
        goto HandleError;
    }

    // Cursor building index should only be navigating primary index.
    Assert( pfucbNil == pfucbTable->pfucbCurIndex );

    // Don't waste time by calling this function when unneeded.
    Assert( pfcbNil != pfcbIndexes );

    pfcbNextBuildIndex = pfcbIndexes;

NextBuild:
#ifdef SHOW_INDEX_PERF
    ULONG   cRecs, cPages;
    TICK    tickStart;
    PGNO    pgnoLast;
    cRecs = 0;
    cPages  = 1;
    pgnoLast = Pcsr( pfucbTable )->Pgno();
    tickStart   = TickOSTimeCurrent();
#endif

    //  HACK: force locLogical to OnCurBM to silence
    //  RECIRetrieveKey(), which will make DIR-level
    //  calls using this cursor
    pfucbTable->locLogical = locOnCurBM;

    pfcbIndexesToBuild = pfcbNextBuildIndex;
    pfcbNextBuildIndex = pfcbNil;

    Assert( cIndexBatchMax <= cFILEIndexBatchSizeDefault );
    err = ErrFILEIndexBatchInit(
                ppib,
                rgpfucbSort,
                pfcbIndexesToBuild,
                &cIndexesToBuild,
                rgcRecInput,
                &pfcbNextBuildIndex,
                cIndexBatchMax );

    // This is either a full batch or the last batch of indexes to build.
    Assert( err < 0
        || cIndexBatchMax == cIndexesToBuild
        || pfcbNil == pfcbNextBuildIndex );

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Beginning scan of records to rebuild %d indexes.\n", cIndexesToBuild ) );
    OSTraceIndent( tracetagIndexPerf, +1 );

    FCB *   pfcbIndexT;
    for ( pfcbIndexT = pfcbIndexesToBuild, iindex = 0; iindex < cIndexesToBuild; pfcbIndexT = pfcbIndexT->PfcbNextIndex(), iindex++ )
    {
        TDB *   ptdbT   = pfcb->Ptdb();
        IDB *   pidbT   = pfcbIndexT->Pidb();

        OSTrace( tracetagIndexPerf, OSFormat(   "Index #%d: %s\n",
                                            iindex+1,
                                            ptdbT->SzIndexName( pidbT->ItagIndexName() ) ) );
    }
    OSTraceIndent( tracetagIndexPerf, -1 );
#endif

    //  build up new index in a sort file
    //
    do
    {
        Call( err );

        //  this loop can take some time, so see if we need
        //  to terminate because of shutdown
        //
        Call( pinst->ErrCheckForTermination() );

        if ( fLogged )
        {
            //  check if the log disk is full, because even
            //  though we're still in the scanning phase,
            //  we won't be able to insert keys into the
            //  index if we can't log the insertion
            //
            Call( pinst->m_plog->ErrLGCheckState() );
        }

#ifdef SHOW_INDEX_PERF
        cRecs++;

        if ( Pcsr( pfucbTable )->Pgno() != pgnoLast )
        {
            cPages++;
            pgnoLast = Pcsr( pfucbTable )->Pgno();
        }
#endif

        //  get bookmark of primary index node
        //
        Assert( Pcsr( pfucbTable )->FLatched() );
        Assert( locOnCurBM == pfucbTable->locLogical );
        Call( ErrBTISaveBookmark( pfucbTable ) );

        Call( ErrFILEIndexBatchAddEntry(
                    rgpfucbSort,
                    pfucbTable,
                    &pfucbTable->bmCurr,
                    pfucbTable->kdfCurr.data,
                    pfcbIndexesToBuild,
                    cIndexesToBuild,
                    rgcRecInput,
                    keyBuffer ) );

        Assert( Pcsr( pfucbTable )->FLatched() );
        err = ErrBTNext( pfucbTable, fDIRNull );
    }
    while ( JET_errNoCurrentRecord != err );
    Assert( JET_errNoCurrentRecord == err );

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Scanned %d records (%d pages) and formulated all keys in %d msecs.\n",
                                        cRecs,
                                        cPages,
                                        TickOSTimeCurrent() - tickStart ) );
#endif

    FUCBResetPreread( pfucbTable );
    BTUp( pfucbTable );

    Call( ErrFILEIndexBatchTerm(
                ppib,
                rgpfucbSort,
                pfcbIndexesToBuild,
                cIndexesToBuild,
                rgcRecInput,
                pstatus,
                fCheckOnly ? &fCorruptionEncountered : NULL,
                pcprintf ) );

    if ( pfcbNil != pfcbNextBuildIndex )
    {
        //  reseek to beginning of data (must succeed since it succeeded the first time)
        FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
        err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
        CallS( err );
        Call( err );

        goto NextBuild;
    }

    if ( fCorruptionEncountered )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucbTable ), HaDbFailureTagCorruption, L"b277592d-6179-477f-8f56-390b6827a9f8" );
        err = ErrERRCheck( JET_errDatabaseCorrupted );
    }
    else
    {
        err = JET_errSuccess;
    }

HandleError:
#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat( "Completed batch index build with error %d.\n", err ) );
#endif

    Assert ( err != errDIRNoShortCircuit );
    if ( err < 0 )
    {
        for ( iindex = 0; iindex < cIndexBatchMax; iindex++ )
        {
            if ( pfucbNil != rgpfucbSort[iindex] )
            {
                SORTClose( rgpfucbSort[iindex] );
                rgpfucbSort[iindex] = pfucbNil;
            }
        }

        FUCBResetPreread( pfucbTable );
        BTUp( pfucbTable );
    }

    if ( fTransactionStarted )
    {
        //  read-only transaction, so should never fail
        CallS( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

HandleError:
    RESKEY.Free( pbKey );
    return err;
}

#endif  //  PARALLEL_BATCH_INDEX_BUILD


//  resume logging on an index build for which logging was initially suspended
//
LOCAL VOID FILEIEnableCreateIndexLogging( PIB * ppib, FCB * const pfcbIdx )
{
    INST * const    pinst           = PinstFromPpib( ppib );
    const ULONG     cbTraceBuffer   = 127;
    CHAR            szTraceBuffer[cbTraceBuffer + 1];

    Assert( g_rgfmp[pfcbIdx->Ifmp()].FLogOn() );
    Assert( !pinst->m_plog->FLogDisabled() );

    //  reset flag
    //
    pfcbIdx->Lock();
    pfcbIdx->ResetDontLogSpaceOps();
    pfcbIdx->Unlock();

    //  decrement count of non-logged index creators to
    //  release backup
    //
    pinst->DecrementCNonLoggedIndexCreators();

    szTraceBuffer[cbTraceBuffer] = 0;
    OSStrCbFormatA(
        szTraceBuffer,
        cbTraceBuffer,
        "CreateIndex logging prematurely enabled [objid:0x%x,pgnoFDP:0x%x]",
        pfcbIdx->ObjidFDP(),
        pfcbIdx->PgnoFDP() );
    CallS( pinst->m_plog->ErrLGTrace( ppib, szTraceBuffer ) );
}


//+API
// BuildIndex
// ========================================================================
// ERR BuildIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
//
// Builds a new index for a file from scratch;  szIndex gives the
// name of an index definition.
//
// PARAMETERS   ppib                        PIB of user
//              pfucb                       Exclusively opened FUCB on file
//              szIndex                     name of index to build
//
// RETURNS      Error code from DIRMAN or SORT or
//                  JET_errSuccess          Everything worked OK.
// COMMENTS
//          A transaction is wrapped around this function at the callee.
//
// SEE ALSO     ErrIsamCreateIndex
//-
LOCAL ERR ErrFILEIBuildIndex(
    PIB *           ppib,
    FUCB *          pfucbTable,
    FUCB *          pfucbIndex,
    BOOL * const    pfIndexLogged )
{
    ERR             err;
    INST * const    pinst           = PinstFromPpib( ppib );
    LOG * const     plog            = pinst->m_plog;
    BACKUP_CONTEXT  *pbackup        = pinst->m_pbackup;
    FCB * const     pfcbIdx         = pfucbIndex->u.pfcb;
    INT             fDIRFlags       = ( fDIRNoVersion | fDIRAppend | ( *pfIndexLogged ? 0 : fDIRNoLog ) );
    FUCB *          pfucbSort       = pfucbNil;
    IDB *           pidb;
    DIB             dib;
    KEY             keyBuffer;
    BYTE            *pbKey          = NULL;
    INT             fUnique;
    LONG            cRecInput       = 0;
    LONG            cRecOutput;

    // Cursor building index should only be navigating primary index.
    Assert( pfucbNil == pfucbTable->pfucbCurIndex );

    // Work buffer is not used when building index.
    Assert( NULL == pfucbIndex->pvWorkBuf );

    Assert( FFUCBSecondary( pfucbIndex ) );
    Assert( pfcbNil != pfcbIdx );
    Assert( pfcbIdx->Pidb() != pidbNil );
    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( pfcbIdx->FInitialized() );

    // Index has not yet been linked into table's index list.
    Assert( pfcbIdx->PfcbTable() == pfcbNil );
    Assert( pfcbIdx->PfcbNextIndex() == pfcbNil );

    pidb = pfcbIdx->Pidb();
    fUnique = pidb->FUnique();

    dib.pos     = posFirst;
    dib.pbm     = NULL;
    dib.dirflag = fDIRNull;

    FUCBSetPrereadForward( pfucbTable, cpgPrereadSequential );
    err = ErrBTDown( pfucbTable, &dib, latchReadTouch );
    Assert( JET_errNoCurrentRecord != err );
    if( JET_errRecordNotFound == err )
    {
        //  the tree is empty
        err = JET_errSuccess;
        goto HandleError;
    }

    Call( err );

    //  HACK: force locLogical to OnCurBM to silence
    //  RECIRetrieveKey(), which will make DIR-level
    //  calls using this cursor
    pfucbTable->locLogical = locOnCurBM;

    Alloc( pbKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyBuffer.prefix.Nullify();
    keyBuffer.suffix.SetCb( cbKeyAlloc );
    keyBuffer.suffix.SetPv( pbKey );

    //  open sort
    //
    err = ErrSORTOpen( ppib, &pfucbSort, fUnique, fTrue );

#ifdef SHOW_INDEX_PERF
    ULONG   cRecs, cPages;
    TICK    tickStart;
    PGNO    pgnoLast;
    cRecs = 0;
    cPages  = 1;
    pgnoLast = Pcsr( pfucbTable )->Pgno();
    tickStart   = TickOSTimeCurrent();
#endif

    //  build up new index in a sort file
    //
    do
    {
        Call( err );

        //  this loop can take some time, so see if we need
        //  to terminate because of shutdown
        //
        Call( pinst->ErrCheckForTermination() );

        //  check if the log disk is full, because even
        //  though we're still in the scanning phase,
        //  we won't be able to insert keys into the
        //  index if we can't log the insertion
        //
        //  NOTE: must check even if fIndexLogged is FALSE
        //  because we may be in the pseudo-logging-disabled
        //  state where logging is actually enabled, but
        //  we're not logging index creation, but we are
        //  still logging space operations, so we may end
        //  up filling the logs with that, and even if
        //  logging is truly disabled, we're just doing
        //  an extra check of a couple of members of LOG
        //
        Call( plog->ErrLGCheckState() );

        //  if we've suspended logging on this btree,
        //  but a backup is in progress, we're going
        //  to have to enable logging to ensure
        //  the backup set is created properly
        //
        if ( pbackup->FBKBackupInProgress()
            && !( *pfIndexLogged )
            && g_rgfmp[pfucbTable->ifmp].FLogOn() )
        {
            // surrogate backup shouldn't happen on circular 
            // logging, which is the only time we suspend logging.
            Assert( pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) );
            Assert( !plog->FLogDisabled() );
            Assert( fDIRFlags & fDIRNoLog );
            fDIRFlags &= ~fDIRNoLog;

            //  NOTE: we don't need to force-flush here
            //  (as we do below) because at this point,
            //  we're just scanning and building the
            //  sort, so we haven't written anything
            //  non-logged to the database yet

            //  re-enable logging
            //
            *pfIndexLogged = fTrue;
            FILEIEnableCreateIndexLogging( ppib, pfcbIdx );
        }

#ifdef SHOW_INDEX_PERF
        cRecs++;

        if ( Pcsr( pfucbTable )->Pgno() != pgnoLast )
        {
            cPages++;
            pgnoLast = Pcsr( pfucbTable )->Pgno();
        }
#endif

        //  get bookmark of primary index node
        //
        Assert( Pcsr( pfucbTable )->FLatched() );
        Assert( locOnCurBM == pfucbTable->locLogical );
        Call( ErrBTISaveBookmark( pfucbTable ) );

        Assert( pfucbTable->bmCurr.key.prefix.FNull() );
        Assert( pfucbTable->bmCurr.data.FNull() );

        LONG    cSecondaryIndexEntriesAdded;
        Call( ErrFILEIAddSecondaryIndexEntriesForPrimaryKey(
                    pfucbTable,
                    pfcbIdx,
                    pfucbTable->kdfCurr.data,
                    pfucbTable->bmCurr.key.suffix,
                    pfucbSort,
                    keyBuffer,
                    &cSecondaryIndexEntriesAdded
                    ) );

        cRecInput += cSecondaryIndexEntriesAdded;

        Assert( Pcsr( pfucbTable )->FLatched() );
        err = ErrBTNext( pfucbTable, fDIRNull );
    }
    while ( JET_errNoCurrentRecord != err );
    Assert( JET_errNoCurrentRecord == err );

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Scanned %d records (%d pages) and formed %d keys in %d msecs.\n",
                                        cRecs,
                                        cPages,
                                        cRecInput,
                                        TickOSTimeCurrent() - tickStart ) );
#endif

    FUCBResetPreread( pfucbTable );
    BTUp( pfucbTable );

#ifdef SHOW_INDEX_PERF
    tickStart   = TickOSTimeCurrent();
#endif

    Call( ErrSORTEndInsert( pfucbSort ) );

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Sorted keys in %d msecs.\n",
                                        TickOSTimeCurrent() - tickStart ) );
#endif

    //  transfer index entries to actual index
    //  insert first one in normal method!
    //
    err = ErrSORTNext( pfucbSort );
    if ( err < 0 )
    {
        if ( JET_errNoCurrentRecord == err )
        {
            SORTClose( pfucbSort );
            err = JET_errSuccess;
        }
        goto HandleError;
    }

    //  move to FDP root
    //
    DIRGotoRoot( pfucbIndex );
    Call( ErrDIRInitAppend( pfucbIndex ) );

#ifdef SHOW_INDEX_PERF
    tickStart = TickOSTimeCurrent();
#endif

    Call( ErrDIRAppend( pfucbIndex,
                        pfucbSort->kdfCurr.key,
                        pfucbSort->kdfCurr.data,
                        fDIRFlags ) );
    Assert( Pcsr( pfucbIndex )->FLatched() );
    cRecOutput = 1;

    //  from now on, try short circuit first
    //
    forever
    {
        err = ErrSORTNext( pfucbSort );
        if ( JET_errNoCurrentRecord == err )
            break;
        Call( err );

        //  this loop can take some time, so see if we need
        //  to terminate because of shutdown
        //
        Call( pinst->ErrCheckForTermination() );

        //  don't want to fill up reserve logs with index entries,
        //  so bail out if we're in the reserve logs (or out
        //  of log disk entirely)
        //
        //  NOTE: must check even if fIndexLogged is FALSE
        //  because we may be in the pseudo-logging-disabled
        //  state where logging is actually enabled, but
        //  we're not logging index creation, but we are
        //  still logging space operations, so we may end
        //  up filling the logs with that, and even if
        //  logging is truly disabled, we're just doing
        //  an extra check of a couple of members of LOG
        //
        Call( plog->ErrLGCheckState() );

        //  if we've suspended logging on this btree,
        //  but a backup is in progress, we're going
        //  to have to enable logging to ensure
        //  the backup set is created properly
        //
        if ( pbackup->FBKBackupInProgress()
            && !( *pfIndexLogged )
            && g_rgfmp[pfucbTable->ifmp].FLogOn() )
        {
            // surrogate backup shouldn't happen on circular 
            // logging, which is the only time we suspend logging.
            Assert( pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) );
            Assert( !plog->FLogDisabled() );
            Assert( fDIRFlags & fDIRNoLog );
            fDIRFlags &= ~fDIRNoLog;

            //  schedule log flush to minimise the chance a log
            //  flush dependendency will be hit trying to flush
            //  this index (because some things, like space ops
            //  in the table space tree, are still logged and
            //  also because we'll try to flush pages we couldn't
            //  latch)
            //
            Call( ErrLGWrite( ppib ) );

            //  flush all the work we've done so for to
            //  ensure it makes it into the backup set
            //
            Call( ErrBFFlush( pfucbTable->ifmp, pfcbIdx->ObjidFDP() ) );

            //  re-enable logging
            //
            *pfIndexLogged = fTrue;
            FILEIEnableCreateIndexLogging( ppib, pfcbIdx );
        }

        Call( ErrDIRAppend( pfucbIndex,
                            pfucbSort->kdfCurr.key,
                            pfucbSort->kdfCurr.data,
                            fDIRFlags ) );
        Assert( Pcsr( pfucbIndex )->FLatched() );
        cRecOutput++;
    }

#ifdef SHOW_INDEX_PERF
    OSTraceIndent( tracetagIndexPerf, +1 );
    OSTrace( tracetagIndexPerf, OSFormat(   "Appended %d records to secondary index in %d msecs.\n",
                                        cRecOutput,
                                        TickOSTimeCurrent() - tickStart ) );
    OSTraceIndent( tracetagIndexPerf, -1 );
#endif

    Call( ErrDIRTermAppend( pfucbIndex ) );

    //  If we got dupes and the index was NOT unique, it must have been
    //  dupes over multi-value columns, in which case the dupes
    //  get properly eliminated.
    Assert( cRecOutput <= cRecInput );
    if ( cRecOutput != cRecInput )
    {
        if ( cRecOutput < cRecInput )
        {
            if ( fUnique )
            {
                err = ErrERRCheck( JET_errKeyDuplicate );
                goto HandleError;
            }
            else
            {
                //  duplicates over multi-valued columns must have been removed
            }
        }
        else
        {
            err = ErrERRCheck( JET_errIndexBuildCorrupted );
            goto HandleError;
        }
    }

    SORTClose( pfucbSort );
    err = JET_errSuccess;

HandleError:
    Assert ( err != errDIRNoShortCircuit );
    if ( err < 0 )
    {
        if ( pfucbSort != pfucbNil )
        {
            SORTClose( pfucbSort );
        }

        FUCBResetPreread( pfucbTable );
        BTUp( pfucbTable );
    }

    // Work buffer is not used when building index.
    Assert( NULL == pfucbIndex->pvWorkBuf );

    RESKEY.Free( pbKey );

    return err;
}

LOCAL ERR ErrFILEIProcessRCE( PIB *ppib, FUCB *pfucbTable, FUCB *pfucbIndex, RCE *prce )
{
    ERR         err;
    FCB         *pfcbTable = pfucbTable->u.pfcb;
    FCB         *pfcbIndex = pfucbIndex->u.pfcb;
    const OPER  oper = prce->Oper();
    BOOKMARK    bookmark;
    BOOL        fUncommittedRCE = fFalse;
    PIB         *ppibProxy = ppibNil;

    Assert( pfcbTable != pfcbNil );
    Assert( pfcbIndex != pfcbNil );

    Assert( pfcbTable->CritRCEList().FOwner() );

    Assert( prceNil != prce );
    if ( prce->TrxCommitted() == trxMax )
    {
        Assert( prce->Pfucb() != pfucbNil );
        Assert( !prce->FRolledBack() );

        ppibProxy = prce->Pfucb()->ppib;
        Assert( ppibNil != ppibProxy );
        Assert( ppibProxy != ppib );

        Assert( g_crefVERCreateIndexLock >= 0 );
        AtomicIncrement( const_cast<LONG *>( &g_crefVERCreateIndexLock ) );
        Assert( g_crefVERCreateIndexLock >= 0 );

        fUncommittedRCE = fTrue;
    }

    prce->GetBookmark( &bookmark );
    Assert( bookmark.key.prefix.FNull() );
    Assert( bookmark.key.Cb() > 0 );

    pfcbTable->CritRCEList().Leave();

    // Find node and obtain read latch.
    err = ErrBTGotoBookmark( pfucbTable, bookmark, latchReadNoTouch );
    if ( err < 0 )
    {
        Assert( JET_errRecordDeleted != err );
        AssertDIRNoLatch( pfucbTable->ppib );

        if ( fUncommittedRCE )
        {
            Assert( g_crefVERCreateIndexLock >= 0 );
            AtomicDecrement( const_cast<LONG *>( &g_crefVERCreateIndexLock ) );
            Assert( g_crefVERCreateIndexLock >= 0 );
        }

        // Must re-enter CritRCEList() before leaving, to get to the
        // next RCE on this FCB.
        pfcbTable->CritRCEList().Enter();
        return err;
    }

    Assert( Pcsr( pfucbTable )->FLatched() );   // Read-latch obtained.

    // Must extract proper data-record image with which to update secondary index.
    // In order to obtain proper image, must scan future RCE's on this node.
    prce->RwlChain().EnterAsReader();

    RCE *prceNextReplace;
    for ( prceNextReplace = prce->PrceNextOfNode();
        prceNextReplace != prceNil && prceNextReplace->Oper() != operReplace;
        prceNextReplace = prceNextReplace->PrceNextOfNode() )
    {
        // Shouldn't be NULL, since our transaction will block cleanup.
        Assert( !prceNextReplace->FOperNull() );
    }

    if ( prceNextReplace != prceNil )
    {
        Assert( prceNextReplace->Oper() == operReplace );
        pfucbTable->dataWorkBuf.SetPv(
            const_cast<BYTE *>( prceNextReplace->PbData() ) + cbReplaceRCEOverhead );
        pfucbTable->dataWorkBuf.SetCb(
            prceNextReplace->CbData() - cbReplaceRCEOverhead );
        prce->RwlChain().LeaveAsReader();
    }
    else
    {
        prce->RwlChain().LeaveAsReader();

        // Set dataWorkBuf.pv to point to pvWorkBuf, in case it was reset.
        Assert ( NULL != pfucbTable->pvWorkBuf );
        pfucbTable->dataWorkBuf.SetPv( (BYTE*)pfucbTable->pvWorkBuf );

        // Go to database for record.
        pfucbTable->kdfCurr.data.CopyInto( pfucbTable->dataWorkBuf );
    }

    // Table's cursor now contains (in dataWorkBuf) the data record used to generate
    // the secondary index key to be inserted (for operInsert or operReplace)
    // or deleted (for operDeleted).
    // Read-latch may now be freed.
    BTUp( pfucbTable );

    // RCE's can only be removed by two routines: RCEClean and rollback.  Since
    // we're in a transaction and this RCE is active, the RCE is blocked from
    // being cleaned up.  This leaves just rollback, which we handle using
    // the g_crefVERCreateIndexLock global variable.
    // If this count is non-zero, anyone trying to rollback an update (ie. insert,
    // delete, or replace) on any primary index will fail and retry.
    if ( fUncommittedRCE )
    {

        // This ensures that the RCE is not committed from underneath us.
        // Note that even if the RCE has since been committed and the
        // PIB freed, we are still able to use the rwlTrx member of
        // the PIB because we never actually release the PIB memory
        // (we just put it on the free PIB list).
        // UNDONE: Optimise by blocking only Commit, not all other CreateIndex
        // threads working on an RCE belonging to this session as well.
        PinstFromPpib( ppib )->RwlTrx( ppibProxy ).EnterAsWriter();

        // Decrement PreventRollback count.  The RCEChain critical section now
        // protects the RCE from being deleted.
        Assert( g_crefVERCreateIndexLock >= 0 );
        AtomicDecrement( const_cast<LONG *>( &g_crefVERCreateIndexLock ) );
        Assert( g_crefVERCreateIndexLock >= 0 );

        // See if the RCE committed or rolled back in the time when we lost
        // the FCB critical section and when we gained the PIB critical section.
        if ( prce->TrxCommitted() == trxMax )
        {
            if ( prce->FRolledBack() )
            {
                err = JET_errSuccess;   // RCE rolled back, so nothing to do.
                goto HandleError;
            }
        }
        else
        {
            Assert( TrxCmp( prce->TrxCommitted(), ppib->trxBegin0 ) > 0 );

            PinstFromPpib( ppib )->RwlTrx( ppibProxy ).LeaveAsWriter();
            fUncommittedRCE = fFalse;
        }
    }

    Assert( ::FOperAffectsSecondaryIndex( oper ) );
    Assert( prce->TrxCommitted() != trxMax || !prce->FRolledBack() );

    if ( oper == operInsert )
    {
        Call( ErrRECIAddToIndex( pfucbTable, pfucbIndex, &bookmark, fDIRNull, prce ) );
    }
    else if ( oper == operFlagDelete )
    {
        Call( ErrRECIDeleteFromIndex( pfucbTable, pfucbIndex, &bookmark, prce ) );
    }
    else
    {
        Assert( oper == operReplace );
        Call( ErrRECIReplaceInIndex( pfucbTable, pfucbIndex, &bookmark, prce ) );
    }

    // Must version operations committed after the indexer began his
    // transaction is to ensure that the indexer himself will have a
    // consistent view of the secondary index.  Additionally, we must
    // provide an RCE for write-conflict detection.

HandleError:
    AssertDIRNoLatch( pfucbTable->ppib );

    pfcbTable->CritRCEList().Enter();

    if ( fUncommittedRCE )
    {
        Assert( ppibNil != ppibProxy );
        PinstFromPpib( ppib )->RwlTrx( ppibProxy ).LeaveAsWriter();
    }

    return err;
}


// This routine updates the index being created with the versioned up
LOCAL ERR ErrFILEIUpdateIndex( PIB *ppib, FUCB *pfucbTable, FUCB *pfucbIndex )
{
    ERR         err         = JET_errSuccess;
    FCB         *pfcbTable  = pfucbTable->u.pfcb;
    FCB         *pfcbIndex  = pfucbIndex->u.pfcb;
    const TRX   trxSession  = ppib->trxBegin0;

    Assert( trxSession != trxMax );

    Assert( pfcbTable->FInitialized() );
    Assert( pfcbTable->FPrimaryIndex() );
    Assert( pfcbTable->FTypeTable() );
    Assert( pfcbTable->FInitialized() );

    Assert( FFUCBSecondary( pfucbIndex ) );
    Assert( pfcbIndex->FInitialized() );
    Assert( pfcbIndex->FTypeSecondaryIndex() );
    Assert( pfcbIndex->PfcbTable() == pfcbNil );    // Index FCB should not be linked in yet.
    Assert( pfcbIndex->Pidb() != pidbNil );

    //  allocate working buffer
    //
    Assert( NULL == pfucbTable->pvWorkBuf );
    RECIAllocCopyBuffer( pfucbTable );

    // Set dirty cursor isolation model to allow unversioned
    // access to secondary index by proxy.
    Assert( !FPIBDirty( pfucbIndex->ppib ) );
    PIBSetCIMDirty( pfucbIndex->ppib );


    BOOL    fUpdatesQuiesced    = fFalse;
    RCE     *prceLastProcessed  = prceNil;
    RCE     *prce;

    // Add sleeps to allow testing of concurrent create index finalization with concurrent updates
    ULONG msecSleepTime = (ULONG)UlConfigOverrideInjection( 58210, 0 );
    if ( msecSleepTime != 0 )
    {
        UtilSleep( msecSleepTime );
    }

    pfcbTable->CritRCEList().Enter();
    prce = pfcbTable->PrceOldest();

    forever
    {
        //  scan RCE's for next one to process
        while ( prceNil != prce
                && ( !prce->FOperAffectsSecondaryIndex()
                    || prce->FRolledBack()
                    || !prce->FActiveNotByMe( ppib, trxSession ) ) )
        {
            Assert( prce->FActiveNotByMe( ppib, trxSession ) || prce->TrxCommitted() != trxSession );
            prce = prce->PrceNextOfFCB();
        }

        if ( prceNil == prce )
        {
            if ( fUpdatesQuiesced )
                break;

            // Reached end of RCE list for this FCB.  Quiesce further updates
            // (letting current updates complete), then re-read RCE list in
            // case more RCE's entered while we were quiescing.  The RCE we
            // last processed is guaranteed to remain because we block rollbacks.
            Assert( prceNil == prceLastProcessed || !prceLastProcessed->FOperNull() );
            Assert( g_crefVERCreateIndexLock >= 0 );
            AtomicIncrement( const_cast<LONG *>( &g_crefVERCreateIndexLock ) );
            Assert( g_crefVERCreateIndexLock >= 0 );

            pfcbTable->CritRCEList().Leave();
            pfcbTable->SetIndexing();
            pfcbTable->CritRCEList().Enter();

            Assert( g_crefVERCreateIndexLock >= 0 );

            if ( msecSleepTime != 0 )
            {
                UtilSleep( msecSleepTime );
            }
            AtomicDecrement( const_cast<LONG *>( &g_crefVERCreateIndexLock ) );
            if ( msecSleepTime != 0 )
            {
                UtilSleep( msecSleepTime );
            }

            Assert( g_crefVERCreateIndexLock >= 0 );

            fUpdatesQuiesced = fTrue;

            Assert( prceNil == prceLastProcessed || !prceLastProcessed->FOperNull() );
            prce = ( prceNil == prceLastProcessed ? pfcbTable->PrceOldest() : prceLastProcessed->PrceNextOfFCB() );
        }

        else
        {
            Assert( prce->FOperAffectsSecondaryIndex() );
            Assert( !prce->FRolledBack() );
            Assert( prce->FActiveNotByMe( ppib, trxSession ) );
            Call( ErrFILEIProcessRCE( ppib, pfucbTable, pfucbIndex, prce ) );

            prceLastProcessed = prce;
            prce = prce->PrceNextOfFCB();
        }
    }

    pfcbTable->CritRCEList().Leave();

    if ( msecSleepTime != 0 )
    {
        UtilSleep( msecSleepTime );
    }

    //  link new FCB and update index mask
    Assert( fUpdatesQuiesced );

    pfcbTable->EnterDDL();
    pfcbTable->LinkSecondaryIndex( pfcbIndex );
    FILESetAllIndexMask( pfcbTable );
    pfcbTable->LeaveDDL();

    pfcbTable->ResetIndexing();
    goto ResetCursor;

HandleError:
    pfcbTable->CritRCEList().Leave();
    if ( fUpdatesQuiesced )
    {
        pfcbTable->ResetIndexing();
    }

ResetCursor:
    Assert( FPIBDirty( pfucbIndex->ppib ) );
    PIBResetCIMDirty( pfucbIndex->ppib );

    Assert ( NULL != pfucbTable->pvWorkBuf );
    RECIFreeCopyBuffer( pfucbTable );

    return err;
}


LOCAL ERR ErrFILEIPrepareOneIndex(
    PIB             * const ppib,
    FUCB            * const pfucbTable,
    FUCB            ** ppfucbIdx,
    JET_INDEXCREATE3_A  * const pidxcreate,
    const CHAR      * const szIndexName,
    const CHAR      * rgszColumns[],
    const BYTE      * rgfbDescending,
    IDB             * const pidb,
    const JET_SPACEHINTS * const pspacehints )
{
    ERR             err;
    const IFMP      ifmp                    = pfucbTable->ifmp;
    FCB             * const pfcb            = pfucbTable->u.pfcb;
    FCB             * pfcbIdx               = pfcbNil;
    PGNO            pgnoIndexFDP;
    OBJID           objidIndex;
    FIELD           * pfield;
    COLUMNID        columnid;
    IDXSEG          rgidxseg[JET_ccolKeyMost];
    IDXSEG          rgidxsegConditional[JET_ccolKeyMost];
    const BOOL      fLockColumn             = !pfcb->FDomainDenyReadByUs( ppib );
    const BOOL      fPrimary                = pidb->FPrimary();
    BOOL            fColumnWasDerived;
    BOOL            fMultivalued;
    BOOL            fText;
    BOOL            fBinary;
    BOOL            fLocalizedText;
    BOOL            fEncrypted;
    BOOL            fCleanupIDB             = fFalse;
    USHORT          iidxseg;

    //  if we don't have exclusive use of the table, we have
    //  to lock all columns in the index to ensure they don't
    //  get deleted out from underneath us
    Assert( !fLockColumn || !pfcb->FTemplateTable() );

    for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
    {
        Call( ErrFILEGetPfieldAndEnterDML(
                    ppib,
                    pfcb,
                    rgszColumns[iidxseg],
                    &pfield,
                    &columnid,
                    &fColumnWasDerived,
                    fLockColumn ) );

        fMultivalued = FFIELDMultivalued( pfield->ffield );
        fText = FRECTextColumn( pfield->coltyp );
        fBinary = FRECBinaryColumn( pfield->coltyp );
        fLocalizedText = ( fText && usUniCodePage == pfield->cp );
        fEncrypted = FFIELDEncrypted( pfield->ffield );

        if ( fEncrypted )
        {
            err = ErrERRCheck( JET_errCannotIndexOnEncryptedColumn );
        }
        else if ( FFIELDEscrowUpdate( pfield->ffield ) )
        {
            err = ErrERRCheck( JET_errCannotIndex );
        }
        else if ( fPrimary && fMultivalued )
        {
            //  primary index cannot be multivalued
            //
            err = ErrERRCheck( JET_errIndexInvalidDef );
        }
        else if ( ( pidxcreate->grbit & JET_bitIndexEmpty ) && FFIELDDefault( pfield->ffield ) )
        {
            //  can't build empty index over column with default value
            err = ErrERRCheck( JET_errIndexInvalidDef );
        }

        if ( !fColumnWasDerived )
        {
            pfcb->LeaveDML();
        }

        Call( err );

        if ( ( 0 == iidxseg ) && pidb->FTuples() )
        {
            if ( !fText && !fBinary )
            {
                err = ErrERRCheck( JET_errIndexTuplesTextBinaryColumnsOnly );
                goto HandleError;
            }
        }

        if ( fMultivalued )
            pidb->SetFMultivalued();
        if ( fLocalizedText )
        {
            pidb->SetFLocalizedText();
        }

        rgidxseg[iidxseg].ResetFlags();

        if ( rgfbDescending[iidxseg] )
            rgidxseg[iidxseg].SetFDescending();

        rgidxseg[iidxseg].SetColumnid( columnid );
    }

    if( pidxcreate->cConditionalColumn > JET_ccolKeyMost )
    {
        err = ErrERRCheck( JET_errIndexInvalidDef );
        Call( err );
    }

    pidb->SetCidxsegConditional( BYTE( pidxcreate->cConditionalColumn ) );
    for ( iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
    {
        Assert( !pidb->FPrimary() );

        const CHAR          * const szColumnName    = pidxcreate->rgconditionalcolumn[iidxseg].szColumnName;
        const JET_GRBIT     grbit                   = pidxcreate->rgconditionalcolumn[iidxseg].grbit;

        if( sizeof( JET_CONDITIONALCOLUMN_A ) != pidxcreate->rgconditionalcolumn[iidxseg].cbStruct )
        {
            err = ErrERRCheck( JET_errIndexInvalidDef );
            Call( err );
        }

        if( JET_bitIndexColumnMustBeNonNull != grbit
            && JET_bitIndexColumnMustBeNull != grbit )
        {
            err = ErrERRCheck( JET_errInvalidGrbit );
            Call( err );
        }

        Call( ErrFILEGetPfieldAndEnterDML(
                ppib,
                pfcb,
                szColumnName,
                &pfield,
                &columnid,
                &fColumnWasDerived,
                fLockColumn ) );
        if ( !fColumnWasDerived )
        {
            pfcb->LeaveDML();
        }

        rgidxsegConditional[iidxseg].ResetFlags();

        if ( JET_bitIndexColumnMustBeNull == grbit )
        {
            rgidxsegConditional[iidxseg].SetFMustBeNull();
        }

        rgidxsegConditional[iidxseg].SetColumnid( columnid );
    }

    pfcb->EnterDDL();

    USHORT  itag;
    err = pfcb->Ptdb()->MemPool().ErrAddEntry(
                (BYTE *)szIndexName,
                (ULONG)strlen( szIndexName ) + 1,
                &itag );
    if ( err < JET_errSuccess )
    {
        pfcb->LeaveDDL();
        goto HandleError;
    }
    Assert( 0 != itag );
    pidb->SetItagIndexName( itag );

    err = ErrIDBSetIdxSeg( pidb, pfcb->Ptdb(), rgidxseg );
    if ( err < JET_errSuccess )
    {
        pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
        pfcb->LeaveDDL();
        goto HandleError;
    }
    err = ErrIDBSetIdxSegConditional( pidb, pfcb->Ptdb(), rgidxsegConditional );
    if ( err < JET_errSuccess )
    {
        if ( pidb->Cidxseg() > cIDBIdxSegMax )
            pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxseg() );
        pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
        pfcb->LeaveDDL();
        goto HandleError;
    }
    fCleanupIDB = fTrue;

    Assert( !pfcb->FDomainDenyRead( ppib ) );
    if ( !pfcb->FDomainDenyReadByUs( ppib ) )
    {
        pidb->SetFVersioned();
        pidb->SetFVersionedCreate();
    }

    //  currently on Table FDP
    //
    DIRGotoRoot( pfucbTable );
    if ( fPrimary )
    {
        pfcb->AssertDDL();

        if ( pfcb->FSequentialIndex() )
        {
            Assert( pfcb->Pidb() == pidbNil );
        }
        else
        {
            // Primary index exists, or is in the process of being created (thus,
            // IDB may or may not be linked in).
            Assert( pfcb->Pidb() != pidbNil );
            pfcb->LeaveDDL();
            err = ErrERRCheck( JET_errIndexHasPrimary );
            goto HandleError;
        }

        // Block other threads from also creating primary index.
        pfcb->Lock();
        pfcb->ResetSequentialIndex();
        pfcb->Unlock();

        pfcb->LeaveDDL();

        //  primary indexes are in same FDP as table
        //
        pgnoIndexFDP = pfcb->PgnoFDP();
        objidIndex = pfcb->ObjidFDP();

        // Must quiesce all updates in order to check that table is empty and
        // then link IDB in.
        pfcb->SetIndexing();

        //  check for records
        //
        DIRGotoRoot( pfucbTable );

        //  check for any node there, even if it is deleted.
        //  UNDONE: This is too strong since it may be deleted and
        //  committed, but this strong check ensures that no
        //  potentially there records exist.
        //
        DIB dib;
        dib.dirflag = fDIRAllNode;
        dib.pos = posFirst;

        err = ErrDIRDown( pfucbTable, &dib );
        Assert( err <= 0 );     // Shouldn't get warnings.
        if ( JET_errRecordNotFound != err )
        {
            if ( JET_errSuccess == err )
            {
                err = ErrERRCheck( JET_errTableNotEmpty );
                DIRUp( pfucbTable );
                DIRDeferMoveFirst( pfucbTable );
            }

            pfcb->ResetIndexing();
            pfcb->EnterDDL();
            pfcb->Lock();
            pfcb->SetSequentialIndex();
            pfcb->Unlock();
            pfcb->LeaveDDL();
            goto HandleError;
        }

        DIRGotoRoot( pfucbTable );

        pfcb->EnterDDL();

        Assert( !pfcb->FSequentialIndex() );
        err = ErrFILEIGenerateIDB( pfcb, pfcb->Ptdb(), pidb );
        if ( err < 0 )
        {
            pfcb->LeaveDDL();
            pfcb->ResetIndexing();
            pfcb->EnterDDL();
            pfcb->Lock();
            pfcb->SetSequentialIndex();
            pfcb->Unlock();
            pfcb->LeaveDDL();
            goto HandleError;
        }

        //  primary index should allow all fixed length columns to be set
        //
        if ( fPrimary && ( pfcb->Ptdb()->IbEndFixedColumns() > REC::CbRecordMost( g_rgfmp[ ifmp ].CbPage(), pidb ) ) )
        {
            err = ErrERRCheck( JET_errRecordTooBig );
            
            pfcb->LeaveDDL();
            pfcb->ResetIndexing();
            pfcb->EnterDDL();
            pfcb->Lock();
            pfcb->SetSequentialIndex();
            pfcb->Unlock();
            pfcb->LeaveDDL();
            goto HandleError;
        }

        Assert( pidbNil != pfcb->Pidb() );
        Assert( !pfcb->FSequentialIndex() );

        Assert( pfcb->PgnoFDP() == pgnoIndexFDP );

        pfcb->SetSpaceHints( pspacehints );

        // update all index mask
        FILESetAllIndexMask( pfcb );

        pfcb->LeaveDDL();

        pfcb->ResetIndexing();

        //  set currency to before first
        //
        DIRBeforeFirst( pfucbTable );

        Assert( pfucbNil == *ppfucbIdx );
        Assert( pfcbIdx == pfcbNil );
    }
    else
    {
        pfcb->LeaveDDL();

        Call( ErrDIRCreateDirectory(
                pfucbTable,
                CpgInitial( pspacehints, g_rgfmp[ ifmp ].CbPage() ),
                &pgnoIndexFDP,
                &objidIndex,
                CPAGE::fPageIndex | ( pidb->FUnique() ? 0 : CPAGE::fPageNonUniqueKeys ),
                ( pidb->FUnique() ? 0 : fSPNonUnique ) ) );
        Assert( pgnoIndexFDP != pfcb->PgnoFDP() );
        Assert( objidIndex > objidSystemRoot );

        //  objids are monotonically increasing, so an index should
        //  always have higher objid than its table
        Assert( objidIndex > pfcb->ObjidFDP() );

        //  get pfcb of index directory
        //
        Call( ErrDIROpen( ppib, pgnoIndexFDP, ifmp, ppfucbIdx ) );
        Assert( *ppfucbIdx != pfucbNil );
        Assert( !FFUCBVersioned( *ppfucbIdx ) );    // Verify won't be deferred closed.
        pfcbIdx = (*ppfucbIdx)->u.pfcb;
        Assert( !pfcbIdx->FInitialized() );
        Assert( pfcbIdx->Pidb() == pidbNil );

        //  make an FCB for this index
        //
        pfcb->EnterDDL();
        err = ErrFILEIInitializeFCB(
            ppib,
            ifmp,
            pfcb->Ptdb(),
            pfcbIdx,
            pidb,
            fFalse,
            pgnoIndexFDP,
            pspacehints,
            NULL );
        pfcb->LeaveDDL();
        Call( err );

        Assert( pidbNil != pfcbIdx->Pidb() );

        //  finish the initialization of the secondary index

        pfcbIdx->Lock();
        pfcbIdx->CreateComplete();
        pfcbIdx->Unlock();
    }

    Assert( pgnoIndexFDP > pgnoSystemRoot );
    Assert( pgnoIndexFDP <= pgnoSysMax );

    //  create index is flagged in version store so that
    //  DDL will be undone.  If flag fails then pfcbIdx
    //  must be released.
    //
    err = PverFromIfmp( ifmp )->ErrVERFlag( pfucbTable, operCreateIndex, &pfcbIdx, sizeof( pfcbIdx ) );
    if ( err < 0 )
    {
        if ( fPrimary )
        {
            pfcb->EnterDDL();
            pfcb->SetSequentialIndex();
            pfcb->LeaveDDL();
        }
        else
        {

            //  force the FCB to be uninitialized so that it will
            //      eventually be purged by DIRClose

            pfcbIdx->Lock();
            pfcbIdx->CreateCompleteErr( errFCBUnusable );
            pfcbIdx->Unlock();

            //  verify that the FUCB will not be defer-closed

            Assert( !FFUCBVersioned( *ppfucbIdx ) );
        }
        goto HandleError;
    }

    fCleanupIDB = fFalse;   // FCB and IDB now in version store, so rollback will clean up.

    // Insert record into MSysIndexes.
    Call( ErrCATAddTableIndex(
                ppib,
                ifmp,
                pfcb->ObjidFDP(),
                szIndexName,
                pgnoIndexFDP,
                objidIndex,
                pidb,
                rgidxseg,
                rgidxsegConditional,
                pspacehints ) );

HandleError:
    if ( fCleanupIDB )
    {
        // We failed before we could properly version the index's FCB, so we'll
        // have to clean up the IDB ourself.
        Assert( err < 0 );

        pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagIndexName() );
        if ( pidb->FIsRgidxsegInMempool() )
            pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxseg() );
        if ( pidb->FIsRgidxsegConditionalInMempool() )
            pfcb->Ptdb()->MemPool().DeleteEntry( pidb->ItagRgidxsegConditional() );
        if ( pfcbNil != pfcbIdx && pidbNil != pfcbIdx->Pidb() )
        {
            pfcbIdx->ReleasePidb();
        }
    }

    return err;
}


// ErrFILEICreateIndex
// ========================================================================
// ERR ErrFILEICreateIndex(
//      PIB     *ppib;          // IN   PIB of user
//      FUCB    *pfucb;         // IN   Exclusively opened FUCB of file
//      JET_INDEXCREATE3_A * pindexcreate )
//
//  Defines an index on a file.
//
// PARAMETERS
//      ppib            PIB of user
//      pfucb           Exclusively opened FUCB of file
//      pindexcreate    Pointer to index create structure
//
//
// RETURNS  Error code from DIRMAN or
//          JET_errSuccess          Everything worked OK.
//          -JET_errColumnNotFound  The index key specified
//                                  contains an undefined field.
//          -IndexHasPrimary        The primary index for this
//                                  Insertfile is already defined.
//          -IndexDuplicate         An index on this file is
//                                  already defined with the
//                                  given name.
//          -IndexInvalidDef        There are too many segments
//                                  in the key.
//          -TableNotEmpty          A primary index may not be
//                                  defined because there is at
//                                  least one record already in
//                                  the file.
// COMMENTS
//      If transaction level > 0, there must not be anyone currently
//      using the file.
//      A transaction is wrapped around this function.  Thus, any
//      work done will be undone if a failure occurs.
//
// SEE ALSO     ErrIsamAddColumn, ErrIsamCreateTable, ErrIsamCreateIndex
//-
LOCAL ERR VTAPI ErrFILEICreateIndex(
    PIB                 * ppib,
    FUCB                * pfucbTable,
    JET_INDEXCREATE3_A  * pidxcreate )
{
    ERR                 err;
    INST * const        pinst                       = PinstFromPpib( ppib );
    FUCB                * pfucb                     = pfucbNil;
    FUCB                * pfucbIdx                  = pfucbNil;
    FCB                 * const pfcb                = pfucbTable->u.pfcb;
    FCB                 * pfcbIdx                   = pfcbNil;
    IDB                 idb( pinst );
    CHAR                szIndexName[ JET_cbNameMost+1 ];
    const CHAR          * rgszColumns[JET_ccolKeyMost];
    BYTE                rgfbDescending[JET_ccolKeyMost];

    BOOL                fInTransaction              = fFalse;
    BOOL                fUnversioned                = fFalse;
    BOOL                fResetVersionedOnSuccess    = fFalse;
    JET_SPACEHINTS      jsphIndex;

    //  check parms
    //
    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucbTable );

    if ( sizeof( JET_INDEXCREATE3_A ) != pidxcreate->cbStruct )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    // Don't currently support DDL on temp. tables.
    if ( FFMPIsTempDB( pfucbTable->ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    
    // Force a write to the output structure (but don't actually modify the value).
    OnDebug( AtomicCompareExchange( &pidxcreate->err, pidxcreate->err, pidxcreate->err ) );

    if ( pidxcreate->pSpacehints )
    {
        jsphIndex = *(pidxcreate->pSpacehints);
        Assert( pidxcreate->ulDensity == pidxcreate->pSpacehints->ulInitialDensity || pidxcreate->ulDensity == 0 );
    }
    else
    {
        JET_SPACEHINTS jsphTable;
        pfcb->GetAPISpaceHints( &jsphTable );
        Assert( jsphTable.cbStruct == sizeof(jsphTable) );
        memset( &jsphIndex, 0, sizeof(jsphIndex) );
        jsphIndex.cbStruct = sizeof(jsphIndex);
        jsphIndex.ulInitialDensity = jsphTable.ulInitialDensity;
    }

    if ( pidxcreate->grbit & JET_bitIndexUnversioned )
    {
        if ( !FFUCBDenyRead( pfucbTable ) )
        {
            if ( ppib->Level() != 0 )
            {
                AssertSz( fFalse, "Must not be in transaction for unversioned CreateIndex." );
                err = ErrERRCheck( JET_errInTransaction );
                return err;
            }
            fUnversioned = fTrue;
        }
    }


    //  ensure that table is updatable
    //
    CallR( ErrFUCBCheckUpdatable( pfucbTable ) );
    CallR( ErrPIBCheckUpdatable( ppib ) );

    if ( pfcb->FFixedDDL() )
    {
        // Check FixedDDL override.
        if ( !FFUCBPermitDDL( pfucbTable ) || pfcb->FTemplateTable() )
        {
            err = ErrERRCheck( JET_errFixedDDL );
            return err;
        }

        // If DDL temporarily permitted, we must have exclusive use of the table.
        Assert( pfcb->FDomainDenyReadByUs( ppib ) );
    }

    CallR( ErrUTILCheckName( szIndexName, pidxcreate->szIndexName, JET_cbNameMost+1 ) );

    OSTraceFMP(
        pfucbTable->ifmp,
        JET_tracetagDDLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] creating index '%s' on objid=[0x%x:0x%x] [szKey=%s,cbKey=0x%x,grbit=0x%x,ulDensity=0x%x,punicode=0x%p,LocaleName=%ws,dwMapFlags=%s,cbVarSegMac=0x%x,ptuples=0x%p,rgCondCols=0x%p,cCondCols=0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szIndexName,
            (ULONG)pfucbTable->ifmp,
            pfucbTable->u.pfcb->ObjidFDP(),
            OSFormatString( pidxcreate->szKey ),
            pidxcreate->cbKey,
            pidxcreate->grbit,
            pidxcreate->ulDensity,
            pidxcreate->pidxunicode,
            ( NULL != pidxcreate->pidxunicode && ( pidxcreate->grbit & JET_bitIndexUnicode ) && NULL != pidxcreate->pidxunicode->szLocaleName ?
                            pidxcreate->pidxunicode->szLocaleName :
                            wszLocaleNameNone ),
            ( NULL != pidxcreate->pidxunicode && ( pidxcreate->grbit & JET_bitIndexUnicode ) ?
                            OSFormat( "0x%x", pidxcreate->pidxunicode->dwMapFlags ) :
                            "<default>" ),
            pidxcreate->cbVarSegMac,
            pidxcreate->ptuplelimits,
            pidxcreate->rgconditionalcolumn,
            pidxcreate->cConditionalColumn ) );

    Assert( pfcb->Ptdb() != ptdbNil );
    CallR( ErrFILEIValidateCreateIndex(
                PinstFromPpib( ppib ),
                pfucbTable->ifmp,
                &idb,
                rgszColumns,
                rgfbDescending,
                pidxcreate,
                &jsphIndex ) );

    if ( fUnversioned )
    {
        CallR( ErrFILEInsertIntoUnverIndexList( pfcb, szIndexName ) );
        fResetVersionedOnSuccess = !pfcb->FDomainDenyReadByUs( ppib );
    }

    const BOOL  fPrimary    = idb.FPrimary();

    // Temporarily open new table cursor.
    Call( ErrDIROpen( ppib, pfcb, &pfucb ) );
    FUCBSetIndex( pfucb );
    FUCBSetMayCacheLVCursor( pfucb );

    Assert( pfucb->u.pfcb != pfcbNil );
    Assert( pfcb == pfucbTable->u.pfcb );
    Assert( pfcb->WRefCount() > 0 );
    Assert( pfcb->FTypeTable() );   // Temp. tables have fixed DDL.
    Assert( pfcb->Ptdb() != ptdbNil );

    Assert( !FFUCBSecondary( pfucb ) );
    Assert( !FCATSystemTable( pfcb->PgnoFDP() ) );

    Call( ErrDIRBeginTransaction( ppib, 47397, NO_GRBIT ) );
    fInTransaction = fTrue;

#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Starting build of index '%s' with log at generation %d.\n",
                                        szIndexName,
                                        PinstFromPpib( ppib )->m_plog->m_plgfilehdr->lgfilehdr.le_lGeneration ) );
#endif

    Call( ErrFILEIPrepareOneIndex(
            ppib,
            pfucb,
            &pfucbIdx,
            pidxcreate,
            szIndexName,
            rgszColumns,
            rgfbDescending,
            &idb,
            &jsphIndex ) );

    if ( fPrimary )
    {
        Assert( pfucbNil == pfucbIdx );
        Assert( pfcbNil == pfcbIdx );
    }
    else
    {
        Assert( pfucbNil != pfucbIdx );
        pfcbIdx = pfucbIdx->u.pfcb;

        Assert( pfcbNil != pfcbIdx );
        Assert( pfcbIdx->FTypeSecondaryIndex() );

        if ( pidxcreate->grbit & JET_bitIndexEmpty )
        {
            // UNDONE: This is a VERY dangerous flag.  The client had better
            // really know what he's doing, or the index could be easily
            // corrupted.  Ideally, the CreateIndex() should be wrapped in
            // a transaction along with the AddColumn() calls for the columns
            // in the index, and no records should be added between the
            // AddColumn() and the CreateIndex().  By putting the AddColumn()
            // in the same transaction as the CreateIndex(), you ensure that
            // other sessions won't be adding records with values over the
            // indexed columns.
            Assert( !idb.FNoNullSeg() );        // JET_bitIndexIgnoreAnyNull must also be specified.
            Assert( !idb.FAllowSomeNulls() );
            Assert( !idb.FAllowFirstNull() );
            Assert( !idb.FAllowAllNulls() );

            // link new FCB and update all index mask
            pfcb->EnterDDL();
            pfcb->LinkSecondaryIndex( pfcbIdx );
            FILESetAllIndexMask( pfcb );
            pfcb->LeaveDDL();
        }
        else
        {
            const ULONG cbTraceBuffer   = 127;
            CHAR        szTraceBuffer[cbTraceBuffer + 1];
            BOOL        fIndexLogged    = g_rgfmp[pfucbTable->ifmp].FLogOn();

            //  we're not going to log this index creation if logging
            //  itself is disabled or if logging is enabled, but we're
            //  using circular logging and no backup is in progress
            //
            //  Any new backup is blocked from starting below
            fIndexLogged = ( fIndexLogged
                            && ( !BoolParam( pinst, JET_paramCircularLog )
                                || pinst->m_pbackup->FBKBackupInProgress() ) );

            Assert( !fIndexLogged || !pinst->m_plog->FLogDisabled() );

            //  at this point the index has been added to the catalog
            //
            const IDBFLAG   idbflagPersisted    = idb.FPersistedFlags();
            const IDXFLAG   idbflagPersistedX   = idb.FPersistedFlagsX();

            FUCBSetIndex( pfucbIdx );
            FUCBSetSecondary( pfucbIdx );

            Assert( !Pcsr( pfucbTable )->FLatched() );
            if ( !fIndexLogged && g_rgfmp[pfucbTable->ifmp].FLogOn() )
            {
                const CPG       cpgSmallTableToIndex    = 64;
                CSR * const     pcsrTable               = Pcsr( pfucbTable );

                Call( ErrBTIGotoRoot( pfucbTable, latchReadNoTouch ) );

                if ( pcsrTable->Cpage().FLeafPage()
                    || ( pcsrTable->Cpage().FParentOfLeaf() && pcsrTable->Cpage().Clines() < cpgSmallTableToIndex ) )
                {
                    //  if root page is also a leaf page or a
                    //  parent-of-leaf page with just a few nodes,
                    //  then this table doesn't have a lot of records,
                    //  so don't bother disabling logging, because
                    //  the cost of force-flushing this index is going
                    //  to be more expensive than just logging everything
                    //
                    fIndexLogged = fTrue;
                    Assert( !pinst->m_plog->FLogDisabled() );
                }
                else
                {
                    //  logging of this index is disabled, but logging
                    //  in general is enabled, so we must block backup
                    //
                    pinst->IncrementCNonLoggedIndexCreators();

                    //  disable logging of space operations on this index
                    //
                    Assert( !pfcbIdx->FDontLogSpaceOps() );
                    pfcbIdx->Lock();
                    pfcbIdx->SetDontLogSpaceOps();
                    pfcbIdx->Unlock();
                }

                BTUp( pfucbTable );
            }

            //  ignore errors if trace fails
            //
            szTraceBuffer[cbTraceBuffer] = 0;
            OSStrCbFormatA(
                szTraceBuffer,
                cbTraceBuffer,
                "Begin %s CreateIndex: %s [objid:0x%x,pgnoFDP:0x%x]",
                ( fIndexLogged ? "" : "NON-LOGGED" ),
                szIndexName,
                pfcbIdx->ObjidFDP(),
                pfcbIdx->PgnoFDP() );
            CallS( pinst->m_plog->ErrLGTrace( ppib, szTraceBuffer ) );

            //  build index using our versioned view of the table
            //
            err = ErrFILEIBuildIndex( ppib, pfucb, pfucbIdx, &fIndexLogged );

            OSStrCbFormatA(
                szTraceBuffer,
                cbTraceBuffer,
                "End CreateIndex scan phase [objid:0x%x,pgnoFDP:0x%x,error:0x%x]",
                pfcbIdx->ObjidFDP(),
                pfcbIdx->PgnoFDP(),
                err );
            CallS( pinst->m_plog->ErrLGTrace( ppib, szTraceBuffer ) );

            if ( !fIndexLogged && g_rgfmp[pfucbTable->ifmp].FLogOn() )
            {
                //  if logging of this index build was disabled, but
                //  logging in general is enabled, we need to force-flush
                //  because we're going to begin logging again
                //
                if ( err >= JET_errSuccess )
                {
                    //  schedule log flush to minimise the chance a log
                    //  flush dependendency will be hit trying to flush
                    //  this index (because some things, like space ops
                    //  in the table space tree, are still logged and
                    //  also because we'll try to flush pages we couldn't
                    //  latch)
                    //
                    Call( ErrLGWrite( ppib ) );

                    err = ErrBFFlush( pfucbTable->ifmp, pfcbIdx->ObjidFDP() );
                }

                //  must reset flag and count regardless of error
                //
                pfcbIdx->Lock();
                pfcbIdx->ResetDontLogSpaceOps();
                pfcbIdx->Unlock();
                pinst->DecrementCNonLoggedIndexCreators();

                //  HACK: must force a dummy logged update anywhere in this
                //  database to ensure that after recovery, the dbtime in the
                //  db header is greater than any of the dbtimes in the non-
                //  logged index
                //
                if ( err >= JET_errSuccess )
                {
                    err = ErrSPDummyUpdate( pfucbIdx );
                }
            }

            //  process error
            //
            Call( err );

            Assert( !FFUCBVersioned( pfucbIdx ) );  // no versioned operations should have occurred on this cursor
            DIRBeforeFirst( pfucb );

            //  update the index with operations happening concurrently
            //
            err = ErrFILEIUpdateIndex( ppib, pfucb, pfucbIdx );

            OSStrCbFormatA(
                szTraceBuffer,
                cbTraceBuffer,
                "End CreateIndex update phase [objid:0x%x,pgnoFDP:0x%x,error:0x%x]",
                pfcbIdx->ObjidFDP(),
                pfcbIdx->PgnoFDP(),
                err );
            CallS( pinst->m_plog->ErrLGTrace( ppib, szTraceBuffer ) );

            Call( err );

            //  The FUnicodeFixupFlag() may have been reset during index build,
            //  so propagate any possible changes to the catalog.
            //
            //  Jan 2012: Since the MSU code has been deleted, the flags should
            //  not be changing during index build.
            //

            Assert( idbflagPersisted == pfcbIdx->Pidb()->FPersistedFlags() );
            if ( idbflagPersisted != pfcbIdx->Pidb()->FPersistedFlags() )
            {
                AssertRTL( idbflagPersisted == ( pfcbIdx->Pidb()->FPersistedFlags() ) );
                AssertRTL( idbflagPersistedX == ( pfcbIdx->Pidb()->FPersistedFlagsX() ) );

                Call( ErrCATChangeIndexFlags(
                                ppib,
                                pfucbTable->ifmp,
                                ObjidFDP( pfucbTable ),
                                szIndexName,
                                pfcbIdx->Pidb()->FPersistedFlags(),
                                pfcbIdx->Pidb()->FPersistedFlagsX() ) );
            }
        }

        // FCB now linked into table's index list, which guarantees that
        // it will be available at Commit/Rollback time, so we can dispose
        // of the index cursor.
        Assert( !FFUCBVersioned( pfucbIdx ) );  // no versioned operations should have occurred on this cursor
        Assert( pfucbNil != pfucbIdx );
        DIRClose( pfucbIdx );
        pfucbIdx = pfucbNil;
    }

    Call( ErrDIRCommitTransaction( ppib, ( pidxcreate->grbit & JET_bitIndexLazyFlush ) ? JET_bitCommitLazyFlush : 0 ) );
    fInTransaction = fFalse;

HandleError:
#ifdef SHOW_INDEX_PERF
    OSTrace( tracetagIndexPerf, OSFormat(   "Ending build of index '%s' (error %d) with log at generation %d.\n",
                                        szIndexName,
                                        err,
                                        PinstFromPpib( ppib )->m_plog->m_plgfilehdr->lgfilehdr.le_lGeneration ) );
#endif

    OSTraceFMP(
        pfucbTable->ifmp,
        JET_tracetagDDLWrite,
        OSFormat(
            "Finished creation of index '%s' with error %d (0x%x)",
            szIndexName,
            err,
            err ) );

    if ( fInTransaction )
    {
        Assert( err < 0 );      // Must have hit an error.

        Assert( pfucbNil != pfucb );
        DIRCloseIfExists( &pfucb->pfucbLV );

        if ( pfucbNil != pfucbIdx )
        {
            //  no versioned operations should have been performed,
            //  so this FUCB will not be defer-closed
            Assert( !FFUCBVersioned( pfucbIdx ) );
            DIRClose( pfucbIdx );
        }

        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

        DIRClose( pfucb );
    }
    else if ( pfucbNil != pfucb )
    {
        Assert( pfucbNil == pfucbIdx );
        DIRCloseIfExists( &pfucb->pfucbLV );
        DIRClose( pfucb );
    }
    else
    {
        Assert( pfucbNil == pfucbIdx );
    }

    AssertDIRNoLatch( ppib );

    if ( fUnversioned )
    {
        if ( fResetVersionedOnSuccess && err >= 0 )
        {
            IDB     * pidbT;

            //  for primary index, FUCB is not opened
            //  for secondary index, FUCB is closed on success
            Assert( pfucbNil == pfucbIdx );

            // On success, reset versioned bit (on error, bit reset by rollback)
            pfcb->EnterDDL();
            if ( fPrimary )
            {
                Assert( pfcbNil == pfcbIdx );
                pidbT = pfcb->Pidb();
            }
            else
            {
                Assert( pfcbNil != pfcbIdx );
                pidbT = pfcbIdx->Pidb();
            }
            Assert( pidbNil != pidbT );
            pidbT->ResetFVersioned();
            pidbT->ResetFVersionedCreate();
            pfcb->LeaveDDL();
        }

        FILERemoveFromUnverList( &g_punveridxGlobal, g_critUnverIndex, pfcb->ObjidFDP(), szIndexName );
    }

    return err;
}


LOCAL ERR VTAPI ErrFILEIBatchCreateIndex(
    PIB                 *ppib,
    FUCB                *pfucbTable,
    JET_INDEXCREATE3_A  *pidxcreate,
    const ULONG         cIndexes )
{
    ERR                 err;
    FUCB                * pfucb                     = pfucbNil;
    FCB                 * const pfcb                = pfucbTable->u.pfcb;
    FCB                 * pfcbIndexes               = pfcbNil;
    IDB                 idb( PinstFromPpib( ppib ) );
    CHAR                szIndexName[ JET_cbNameMost+1 ];
    const CHAR          *rgszColumns[JET_ccolKeyMost];
    BYTE                rgfbDescending[JET_ccolKeyMost];
    BOOL                fInTransaction              = fFalse;
    BOOL                fLazyCommit                 = fTrue;
    ULONG               iindex;

#ifdef PARALLEL_BATCH_INDEX_BUILD
    FUCB                ** rgpfucbIdx               = NULL;
#else
    FUCB                * rgpfucbIdx[cFILEIndexBatchSizeDefault];
#endif

    JET_INDEXCREATE3_A      *pidxcreateT                = NULL;
    JET_INDEXCREATE3_A      *pidxcreateNext             = NULL;

    //  check parms
    //
    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucbTable );

    // Don't currently support DDL on temp. tables.    
    if ( FFMPIsTempDB( pfucbTable->ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  ensure that table is updatable
    //
    CallR( ErrFUCBCheckUpdatable( pfucbTable ) );
    CallR( ErrPIBCheckUpdatable( ppib ) );

    if ( 0 != ppib->Level() )
    {
        //  batch mode requires level 0
        err = ErrERRCheck( JET_errInTransaction );
        return err;
    }

    if ( !pfcb->FDomainDenyReadByUs( ppib ) )
    {
        //  batch mode requires
        err = ErrERRCheck( JET_errExclusiveTableLockRequired );
        return err;
    }

    if ( pfcb->FFixedDDL() )
    {
        // Check FixedDDL override.
        if ( !FFUCBPermitDDL( pfucbTable ) || pfcb->FTemplateTable() )
        {
            err = ErrERRCheck( JET_errFixedDDL );
            return err;
        }

        // If DDL temporarily permitted, we must have exclusive use of the table.
        Assert( pfcb->FDomainDenyReadByUs( ppib ) );
    }

    //  if no work to do then return
    //
    if ( 0 == cIndexes )
    {
        return JET_errSuccess;
    }

#ifdef PARALLEL_BATCH_INDEX_BUILD
    AllocR( rgpfucbIdx = (FUCB **)PvOSMemoryHeapAlloc( sizeof(FUCB *) * cIndexes ) );
    memset( rgpfucbIdx, 0, sizeof(FUCB *) * cIndexes );
#else
    if ( cIndexes > cFILEIndexBatchSizeDefault )
    {
        err = ErrERRCheck( JET_errTooManyIndexes );
        return err;
    }

    for ( iindex = 0; iindex < cIndexes; iindex++ )
        rgpfucbIdx[iindex] = pfucbNil;
#endif

    // Temporarily open new table cursor.
    CallJ( ErrDIROpen( ppib, pfcb, &pfucb ), Cleanup );
    FUCBSetIndex( pfucb );
    FUCBSetMayCacheLVCursor( pfucb );

    Assert( pfucb->u.pfcb != pfcbNil );
    Assert( pfcb == pfucb->u.pfcb );
    Assert( pfcb->WRefCount() > 0 );
    Assert( pfcb->FTypeTable() );   // Temp. tables have fixed DDL.
    Assert( pfcb->Ptdb() != ptdbNil );

    Assert( !FFUCBSecondary( pfucb ) );
    Assert( !FCATSystemTable( pfcb->PgnoFDP() ) );

    Call( ErrDIRBeginTransaction( ppib, 63781, NO_GRBIT ) );
    fInTransaction = fTrue;

    pidxcreateT = pidxcreate;
    for( iindex = 0; iindex < cIndexes; iindex++, pidxcreateT = pidxcreateNext )
    {
        FCB *       pfcbIndexT      = pfcbNil;

        Assert( sizeof(JET_INDEXCREATE3_A) == pidxcreateT->cbStruct );

        //  support all JET_INDEXCREATE*_A definitions (anyway)
        //
        pidxcreateNext = (JET_INDEXCREATE3_A *)((BYTE *)( pidxcreateT ) + pidxcreateT->cbStruct);

        if (    sizeof( JET_INDEXCREATE3_A ) != pidxcreateT->cbStruct )
        {
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }

        if ( pidxcreateT->grbit & ( JET_bitIndexPrimary | JET_bitIndexUnversioned | JET_bitIndexEmpty ) )
        {
            //  currently unsupported in batch mode
            //
            Call( ErrERRCheck( JET_errInvalidGrbit ) );
        }

        // Force a write to the output structure (but don't actually modify the value).
        OnDebug( AtomicCompareExchange( &pidxcreate->err, pidxcreate->err, pidxcreate->err ) );

        if ( fLazyCommit && !( pidxcreateT->grbit & JET_bitIndexLazyFlush ) )
        {
            //  if even one of the indexes cannot be lazily flushed,
            //  then none of them can
            fLazyCommit = fFalse;
        }

        JET_SPACEHINTS          jsphIndex;
        pfcb->GetAPISpaceHints( &jsphIndex );
        if ( pidxcreateT->pSpacehints )
        {
            jsphIndex = *(pidxcreate->pSpacehints);
        }
        if ( ( (ULONG)g_cbPage * cpgInitialTreeDefault ) == jsphIndex.cbInitial )
        {
            jsphIndex.cbInitial = 0;    // if default cpgInitialTreeDefault=1 page, set to 0 (meaning default)
        }

        Call( ErrUTILCheckName( szIndexName, pidxcreateT->szIndexName, JET_cbNameMost+1 ) );

        Call( ErrFILEIValidateCreateIndex(
                    PinstFromPpib( ppib ),
                    pfucbTable->ifmp,
                    &idb,
                    rgszColumns,
                    rgfbDescending,
                    pidxcreateT,
                    &jsphIndex ) );

        Call( ErrFILEIPrepareOneIndex(
                ppib,
                pfucb,
                &rgpfucbIdx[iindex],
                pidxcreateT,
                szIndexName,
                rgszColumns,
                rgfbDescending,
                &idb,
                &jsphIndex ) );

        pfcbIndexT = rgpfucbIdx[iindex]->u.pfcb;
        Assert( pfcbIndexT->FTypeSecondaryIndex() );
        pfcbIndexT->SetPfcbNextIndex( pfcbIndexes );
        pfcbIndexT->SetPfcbTable( pfcb );

        pfcbIndexes = pfcbIndexT;
    }

    Call( ErrFILEBuildAllIndexes(
                ppib,
                pfucb,
                pfcbIndexes,
                NULL,
                cIndexes ) );

    pfcb->EnterDDL();

    Assert( pfcbNil == rgpfucbIdx[0]->u.pfcb->PfcbNextIndex() );
    Assert( cIndexes > 0 );
    Assert( pfcbIndexes == rgpfucbIdx[cIndexes-1]->u.pfcb );
    rgpfucbIdx[0]->u.pfcb->SetPfcbNextIndex( pfcb->PfcbNextIndex() );
    pfcb->SetPfcbNextIndex( pfcbIndexes );

    FILESetAllIndexMask( pfcb );

    pfcb->LeaveDDL();

    for ( iindex = 0; iindex < cIndexes; iindex++ )
    {
        FUCB * const    pfucbIndexT     = rgpfucbIdx[iindex];
        Assert( pfucbNil != pfucbIndexT );

        FCB * const     pfcbIndexT      = pfucbIndexT->u.pfcb;

        Assert( pfcbNil != pfcbIndexT );
        Assert( pfcbIndexT->FTypeSecondaryIndex() );

        pfcbIndexT->Lock();
        pfcbIndexT->ResetDontLogSpaceOps();
        pfcbIndexT->Unlock();

        Assert( !FFUCBVersioned( pfucbIndexT ) );   // No versioned operations should have been performed, so won't be defer-closed.
        DIRClose( pfucbIndexT );
    }

    Call( ErrDIRCommitTransaction( ppib, fLazyCommit ? JET_bitCommitLazyFlush : 0 ) );
    fInTransaction = fFalse;

HandleError:
    Assert( pfucbNil != pfucb );
    DIRCloseIfExists( &pfucb->pfucbLV );

    if ( fInTransaction )
    {
        Assert( err < 0 );      // Must have hit an error.

        if ( NULL != rgpfucbIdx )
        {
            for ( iindex = 0; iindex < cIndexes; iindex++ )
            {
                if ( pfucbNil != rgpfucbIdx[iindex] )
                {
                    Assert( !FFUCBVersioned( rgpfucbIdx[iindex] ) );    // No versioned operations should have been performed, so won't be defer-closed.
                    DIRClose( rgpfucbIdx[iindex] );
                }
            }
        }

        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    DIRClose( pfucb );
    AssertDIRNoLatch( ppib );

Cleanup:
#ifdef PARALLEL_BATCH_INDEX_BUILD
    if ( NULL != rgpfucbIdx )
    {
        OSMemoryHeapFree( rgpfucbIdx );
    }
#endif

    return err;
}

//+API
// ErrIsamCreateIndex
// ========================================================================
// ERR ErrIsamCreateIndex(
//      SESID   sesid;          // IN   PIB of user
//      TABLEID tableid;        // IN   Exclusively opened FUCB of file
//      JET_INDEXCREATE3_A * pindexcreate;  // IN   Array of indexes to create
//      unsigned long cIndexCreate );   // IN   Number of indexes to create
//
//  Creates indexes on a table
//
//
// RETURNS  Error code from ErrFILEICreateIndex or
//          JET_errSuccess          Everything worked OK.
//          -JET_errColumnNotFound  The index key specified
//                                  contains an undefined field.
//          -IndexHasPrimary        The primary index for this
//                                  Insertfile is already defined.
//          -IndexDuplicate         An index on this file is
//                                  already defined with the
//                                  given name.
//          -IndexInvalidDef        There are too many segments
//                                  in the key.
//          -TableNotEmpty          A primary index may not be
//                                  defined because there is at
//                                  least one record already in
//                                  the file.
// COMMENTS
//      If transaction level > 0, there must not be anyone currently
//      using the file.
//      A transaction is wrapped around this function.  Thus, any
//      work done will be undone if a failure occurs.
//
// SEE ALSO     ErrIsamAddColumn, ErrIsamCreateTable, ErrFILEICreateTable
//-
ERR VTAPI ErrIsamCreateIndex(
    JET_SESID           sesid,
    JET_VTID            vtid,
    JET_INDEXCREATE3_A  *pindexcreate,
    ULONG       cIndexCreate )
{
    ERR                 err                 = JET_errSuccess;
    PIB                 * const ppib        = reinterpret_cast<PIB *>( sesid );
    FUCB                * const pfucbTable  = reinterpret_cast<FUCB *>( vtid );
    BOOL                fInTransaction      = fFalse;

#ifdef SHOW_INDEX_PERF
    const TICK          tickStart           = TickOSTimeCurrent();
    OSTrace( tracetagIndexPerf, OSFormat( "About to [re]build %d indexes.\n", cIndexCreate ) );
    OSTraceIndent( tracetagIndexPerf, +1 );
#endif

    Call( ErrIDXCheckUnicodeFlagAndDefn( pindexcreate, cIndexCreate ) );
    Call( ErrPIBOpenTempDatabase ( ppib ) );

#ifdef MINIMAL_FUNCTIONALITY

    ULONG       iIndexCreate    = 0;
    JET_INDEXCREATE3_A* pindexcreateT   = NULL;
    BOOL                fLazyCommit     = fTrue;

    Call( ErrDIRBeginTransaction( ppib, 39205, NO_GRBIT ) );
    fInTransaction = fTrue;

    for (   iIndexCreate = 0, pindexcreateT = pindexcreate;
            iIndexCreate < cIndexCreate;
            iIndexCreate++, pindexcreateT = (JET_INDEXCREATE3_A *)( (BYTE *)pindexcreateT + pindexcreateT->cbStruct ) )
    {
        Assert( sizeof(JET_INDEXCREATE3_A) == pindexcreateT->cbStruct );

        Call( ErrFILEICreateIndex( ppib, pfucbTable, pindexcreateT ) );
        fLazyCommit = fLazyCommit && ( pindexcreateT->grbit & JET_bitIndexLazyFlush );
    }

    Call( ErrDIRCommitTransaction( ppib, fLazyCommit ? JET_bitCommitLazyFlush : 0 ) );
    fInTransaction = fFalse;

#else  //  !MINIMAL_FUNCTIONALITY

    Assert( cIndexCreate >= 0 );
    if ( 1 == cIndexCreate )
    {
        Call( ErrFILEICreateIndex( ppib, pfucbTable, pindexcreate ) );
    }
    else
    {
        Call( ErrFILEIBatchCreateIndex( ppib, pfucbTable, pindexcreate, cIndexCreate ) );
    }

#endif  //  MINIMAL_FUNCTIONALITY

HandleError:
    if ( fInTransaction )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

#ifdef SHOW_INDEX_PERF
    OSTraceIndent( tracetagIndexPerf, -1 );
    OSTrace( tracetagIndexPerf, OSFormat(   "Finished rebuild of %d indexes in %d msecs with error %d.\n",
                                        cIndexCreate,
                                        TickOSTimeCurrent() - tickStart,
                                        err ) );
#endif

    return err;
}


//+API
// ErrIsamDeleteTable
// ========================================================================
// ERR ErrIsamDeleteTable( JET_SESID vsesid, JET_DBID vdbid, CHAR *szName )
//
// Calls ErrFILEIDeleteTable to
// delete a file and all indexes associated with it.
//
// RETURNS      JET_errSuccess or err from called routine.
//
// SEE ALSO     ErrIsamCreateTable
//-
ERR VTAPI ErrIsamDeleteTable( JET_SESID vsesid, JET_DBID vdbid, const CHAR *szName, BOOL fAllowTableDeleteSensitive )
{
    ERR     err;
    PIB     *ppib = (PIB *)vsesid;
    IFMP    ifmp = (IFMP)vdbid;

    //  check parameters
    //
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

    if ( FFMPIsTempDB( ifmp ) )
    {
        // Cannot use DeleteTable on temporary tables.
        // Must use CloseTable instead.
        err = ErrERRCheck( JET_errCannotDeleteTempTable );
    }
    else if ( !fAllowTableDeleteSensitive &&
              ( FCATObjidsTable( szName ) ||
                FCATExtentPageCountCacheTable( szName ) ) )
    {
        err = ErrERRCheck( JET_errCannotDeleteSystemTable );
    }
    else
    {
        CallR( ErrPIBCheckUpdatable( ppib ) );
        
        err = ErrFILEDeleteTable( ppib, ifmp, szName, fAllowTableDeleteSensitive );
    }

    return err;
}


// ErrFILEDeleteTable
// ========================================================================
// ERR ErrFILEDeleteTable( PIB *ppib, IFMP ifmp, CHAR *szName )
//
// Deletes a file and all indexes associated with it.
//
// RETURNS      JET_errSuccess or err from called routine.
//

// COMMENTS
//  Acquires an exclusive lock on the file [FCBSetDelete].
//  A transaction is wrapped around this function.  Thus,
//  any work done will be undone if a failure occurs.
//  Transaction logging is turned off for temporary files.
//
// SEE ALSO     ErrIsamCreateTable
//-
ERR ErrFILEDeleteTable( PIB *ppib, IFMP ifmp, const CHAR *szName, BOOL fAllowTableDeleteSensitive )
{
    ERR     err;
    FUCB    *pfucb              = pfucbNil;
    FUCB    *pfucbParent        = pfucbNil;
    FCB     *pfcb               = pfcbNil;
    PGNO    pgnoFDP;
    OBJID   objidTable;
    CHAR    szTable[JET_cbNameMost+1];
    BOOL    fInUseBySystem;
    VER     *pver;

    CheckPIB( ppib );
    CheckDBID( ppib, ifmp );

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Don't call DeleteTable on temp tables; use CloseTable instead." );
        return ErrERRCheck( JET_errCannotDeleteTempTable );
    }
    
    //  must normalise for CAT hash deletion
    //  PERF UNDONE: the name will be normalised again in FILEOpenTable()
    CallR( ErrUTILCheckName( szTable, szName, JET_cbNameMost+1 ) );

    CallR( ErrDIRBeginTransaction( ppib, 55589, NO_GRBIT ) );

    OSTraceFMP(
        ifmp,
        JET_tracetagDDLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] deleting table '%s' of database=['%ws':0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szTable,
            g_rgfmp[ifmp].WszDatabaseName(),
            (ULONG)ifmp ) );

    //  open cursor on database and seek to table without locking
    //
    Call( ErrDIROpen( ppib, pgnoSystemRoot, ifmp, &pfucbParent ) );

    {
    JET_GRBIT grbit = JET_bitTableDelete|JET_bitTableDenyRead ;
    if ( fAllowTableDeleteSensitive )
    {
        grbit |= JET_bitTableAllowSensitiveOperation;
    }
    Call( ErrFILEOpenTable(
                ppib,
                ifmp,
                &pfucb,
                szName,
                grbit ) );
    fInUseBySystem = ( JET_wrnTableInUseBySystem == err );
    }

    // We should now have exclusive use of the table.
    pfcb = pfucb->u.pfcb;
    pgnoFDP = pfcb->PgnoFDP();
    objidTable = pfcb->ObjidFDP();

    Assert( pfcb->FTypeTable() );
    if ( pfcb->FTemplateTable() )
    {
        // UNDONE: Deletion of template table not currently supported.
        // However, if the template table is opened as a sentinel,
        // then the TemplateTable flag will not be set, in which
        // case the table will be allowed to be deleted.  Any existing
        // inherited tables will fail if an attempt is made to open
        // them.
        err = ErrERRCheck( JET_errCannotDeleteTemplateTable );
        goto HandleError;
    }

    //  the following assert(s) goes off when we reuse a cursor that
    //  was defer closed by us -- it has been disabled for now
    //
//  Assert( !FFUCBVersioned( pfucb ) );
    Assert( pfcb->FDomainDenyReadByUs( ppib ) );

    pver = PverFromIfmp( pfucb->ifmp );
    err = pver->ErrVERFlag( pfucbParent, operDeleteTable, &pgnoFDP, sizeof(pgnoFDP) );
    if ( err < 0 )
    {
        // Must close FUCB first in case sentinel exists.  Set to pfucbNil
        // afterwards so HandleError doesn't try to close FUCB again.
//      Assert( !FFUCBVersioned( pfucb ) );     // Verifies FUCB is not deferred closed.
        DIRClose( pfucb );
        pfucb = pfucbNil;

        goto HandleError;
    }

    Assert( pfcb->PgnoFDP() == pgnoFDP );
    Assert( pfcb->Ifmp() == ifmp );

    // UNDONE: Is it necessary to grab critical section, since we have
    // exclusive use of the table?
    if ( pfcb->Ptdb() != ptdbNil )
    {
        pfcb->EnterDDL();

        for ( FCB *pfcbT = pfcb; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            Assert( pfcbT->Ifmp() == ifmp );
            Assert( pfcbT == pfcb
                || ( pfcbT->FTypeSecondaryIndex()
                    && pfcbT->PfcbTable() == pfcb ) );

            pfcbT->Lock();
            pfcbT->SetDeletePending();
            pfcbT->Unlock();
        }

        if ( pfcb->Ptdb()->PfcbLV() != pfcbNil )
        {
            pfcb->Ptdb()->PfcbLV()->Lock();
            pfcb->Ptdb()->PfcbLV()->SetDeletePending();
            pfcb->Ptdb()->PfcbLV()->Unlock();
        }

        pfcb->LeaveDDL();
    }
    else
    {
        FireWall( "DeprecatedSentinelFcbDeleteTable" ); // Sentinel FCBs are believed deprecated
        Assert( !fInUseBySystem );
        Assert( pfcbNil == pfcb->PfcbNextIndex() );

        pfcb->Lock();
        pfcb->SetDeletePending();
        pfcb->Unlock();
    }


    while ( fInUseBySystem )
    {
        pfcb->Lock();
        pfcb->FucbList().LockForEnumeration();

        fInUseBySystem = fFalse;

        for ( INT ifucbList = 0; ifucbList < pfcb->FucbList().Count(); ifucbList++ )
        {
            FUCB * pfucbT = pfcb->FucbList()[ifucbList];

            if ( FPIBSessionSystemCleanup( pfucbT->ppib ) )
            {
                //  don't care about RCE clean, because any outstanding versions will be cleaned
                //  before the DeleteTable version is cleaned.
                if( !FPIBSessionRCEClean(pfucbT->ppib ) )
                {
                    //  the DeletePending flag for this table has now been set, forcing
                    //  OLD2/DBSCAN to exit at its earliest convenience. Wait for it.
                    fInUseBySystem = fTrue;
                    break;
                }
            }
#ifdef DEBUG
            //  User owned FUCBs can exist at this point but they will be short-lived. They will be closed when
            //  ErrFILEICheckAndSetMode() fails because we have JET_bitTableDenyRead.
            //  Once this tx commits and is the oldest tx, no one should be able to see the FCB and open FUCB's on it
            else
            {
                // If this cursor was opened by this session but is not the DeleteTable cursor, it should be defer-closed.
                Assert(pfucbT->ppib != ppib || pfucbT == pfucb || FFUCBDeferClosed( pfucbT ));
            }
#endif
        }

        pfcb->FucbList().UnlockForEnumeration();
        pfcb->Unlock();

        if ( fInUseBySystem )
        {
            UtilSleep( 500 );
        }
    }

#ifdef DEBUG
    if ( pfcb->Ptdb() == ptdbNil )
    {
        Assert( !FFUCBVersioned( pfucb ) );
    }
#endif

    DIRClose( pfucb );
    pfucb = pfucbNil;

    DIRClose( pfucbParent );
    pfucbParent = pfucbNil;

    //  remove table record from MSysObjects before committing.
    Call( ErrCATDeleteTable( ppib, ifmp, objidTable ) );
    CATHashDelete( pfcb, const_cast< CHAR * >( szTable ) );

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    AssertDIRNoLatch( ppib );
    return err;

HandleError:
    OSTraceFMP(
        ifmp,
        JET_tracetagDDLWrite,
        OSFormat( "Finished deletion of table '%s' with error %d (0x%x)", szTable, err, err ) );

    if ( pfucb != pfucbNil )
    {
        DIRClose( pfucb );
    }
    if ( pfucbParent != pfucbNil )
        DIRClose( pfucbParent );

    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    AssertDIRNoLatch( ppib );

    return err;
}


//+API
// DeleteIndex
// ========================================================================
// ERR DeleteIndex( PIB *ppib, FUCB *pfucb, CHAR *szIndex )
//
// Deletes an index definition and all index entries it contains.
//
// PARAMETERS   ppib                        PIB of user
//              pfucb                       Exclusively opened FUCB on file
//              szName                      name of index to delete
// RETURNS      Error code from DIRMAN or
//                  JET_errSuccess           Everything worked OK.
//                  -TableInvalid            There is no file corresponding
//                                           to the file name given.
//                  -TableNoSuchIndex        There is no index corresponding
//                                           to the index name given.
//                  -IndexMustStay           The primary index of a file may
//                                           not be deleted.
// COMMENTS
//      There must not be anyone currently using the file.
//      A transaction is wrapped around this function.  Thus,
//      any work done will be undone if a failure occurs.
//      Transaction logging is turned off for temporary files.
// SEE ALSO     DeleteTable, CreateTable, CreateIndex
//-
ERR VTAPI ErrIsamDeleteIndex(
    JET_SESID       sesid,
    JET_VTID        vtid,
    const CHAR      *szName
    )
{
    PIB *ppib           = reinterpret_cast<PIB *>( sesid );
    FUCB *pfucbTable    = reinterpret_cast<FUCB *>( vtid );

    ERR     err;
    CHAR    szIndex[ (JET_cbNameMost + 1) ];
    FCB     *pfcbTable;
    FCB     *pfcbIdx;
    FUCB    *pfucb;
    PGNO    pgnoIndexFDP;
    BOOL    fInTransaction = fFalse;
    VER     *pver;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucbTable );

    // Don't currently support DDL operations on temp tables.
    if ( FFMPIsTempDB( pfucbTable->ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    CallR( ErrUTILCheckName( szIndex, szName, ( JET_cbNameMost + 1 ) ) );

    //  ensure that table is updatable
    //
    CallR( ErrFUCBCheckUpdatable( pfucbTable )  );
    CallR( ErrPIBCheckUpdatable( ppib ) );

    Assert( ppib != ppibNil );
    Assert( pfucbTable != pfucbNil );
    Assert( pfucbTable->u.pfcb != pfcbNil );
    pfcbTable = pfucbTable->u.pfcb;

    OSTraceFMP(
        pfcbTable->Ifmp(),
        JET_tracetagDDLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] deleting index '%s' of objid=[0x%x:0x%x]",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            szIndex,
            (ULONG)pfcbTable->Ifmp(),
            pfcbTable->ObjidFDP() ) );

    Assert( pfcbTable->FTypeTable() );      // Temp. tables have fixed DDL.

    if ( pfcbTable->FFixedDDL() )
    {

//  UNDONE: Cannot currently permit DDL deletes even if PermitDDL flag is
//  specified because DDL deletes will leave RCE's that have to be cleaned
//  up and modify the FCB while doing so.  This will mess up cursors
//  opened normally after the PermitDDL cursor closes.
#ifdef PERMIT_DDL_DELETE
        // Check FixedDDL override.
        if ( !FFUCBPermitDDL( pfucbTable ) )
        {
            err = ErrERRCheck( JET_errFixedDDL );
            return err;
        }

        // If DDL temporarily permitted, we must have exclusive use of the table.
        Assert( pfcbTable->FDomainDenyReadByUs( ppib ) );
#else
        err = ErrERRCheck( JET_errFixedDDL );
        return err;
#endif
    }

    Assert( pfcbTable->Ptdb() != ptdbNil );
    if ( pfcbTable->Ptdb()->PfcbTemplateTable() != pfcbNil
        && FFILEITemplateTableIndex( pfcbTable->Ptdb()->PfcbTemplateTable(), szIndex ) )
    {
        err = ErrERRCheck( JET_errFixedInheritedDDL );
        return err;
    }

    //  create new cursor -- to leave user's cursor unmoved
    //
    CallR( ErrDIROpen( ppib, pfcbTable, &pfucb ) );

    Call( ErrDIRBeginTransaction( ppib, 43301, NO_GRBIT ) );
    fInTransaction = fTrue;

    //  remove index record from MSysIndexes, preventing other threads from
    //  trying to delete the same index.
    //
    Call( ErrCATDeleteTableIndex(
                ppib,
                pfcbTable->Ifmp(),
                pfcbTable->ObjidFDP(),
                szIndex,
                &pgnoIndexFDP ) );

    // Can't delete primary index.
    Assert( pfcbTable->PgnoFDP() != pgnoIndexFDP );

    // Find index's FCB in the table's index list.
    pfcbTable->EnterDDL();
    for ( pfcbIdx = pfcbTable->PfcbNextIndex(); pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        Assert( pfcbIdx->Pidb() != pidbNil );
        if ( pfcbIdx->PgnoFDP() == pgnoIndexFDP )
        {
#ifdef DEBUG
            // Verify no one else is deleting this index (if so, conflict would
            // have been detected when updating catalog above).
            Assert( !pfcbIdx->FDeletePending() );
            Assert( !pfcbIdx->FDeleteCommitted() );
            Assert( !pfcbIdx->Pidb()->FDeleted() );

            // verify that no other FCB has the same FDP -- would never happen
            // because the FCB is deallocated before the space is freed.
            FCB *pfcbT;
            for ( pfcbT = pfcbIdx->PfcbNextIndex(); pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
            {
                Assert( pfcbT->PgnoFDP() != pgnoIndexFDP );
            }
#endif
            break;
        }
    }
    pfcbTable->AssertDDL();

    if ( pfcbIdx == pfcbNil )
    {
        Assert( fFalse );       // If index in catalog, FCB must exist.
        pfcbTable->LeaveDDL();
        err = ErrERRCheck( JET_errIndexNotFound );
        goto HandleError;
    }

    Assert( !pfcbIdx->FTemplateIndex() );
    Assert( !pfcbIdx->FDerivedIndex() );
    Assert( pfcbIdx->PfcbTable() == pfcbTable );

    err = pfcbIdx->ErrSetDeleteIndex( ppib );
    pfcbTable->LeaveDDL();
    Call( err );

    // Assert not deleting current secondary index.
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        Assert( pfucb->pfucbCurIndex->u.pfcb != pfcbNil );
        Assert( pfucb->pfucbCurIndex->u.pfcb->PfcbTable() == pfcbTable );
        Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb() != pidbNil );
        Assert( pfucb->pfucbCurIndex->u.pfcb->Pidb()->CrefCurrentIndex() > 0
            || pfucb->pfucbCurIndex->u.pfcb->Pidb()->FTemplateIndex() );
        Assert( pgnoIndexFDP != pfucb->pfucbCurIndex->u.pfcb->PgnoFDP() );
    }

    pver = PverFromIfmp( pfucb->ifmp );
    err = pver->ErrVERFlag( pfucb, operDeleteIndex, &pfcbIdx, sizeof(pfcbIdx) );
    if ( err < 0 )
    {
        pfcbIdx->ResetDeleteIndex();
        goto HandleError;
    }

    // Ensure consistent view of table's space tree (to prevent doubly-freed space).
    Assert( pfcbIdx->PfcbTable() == pfcbTable );
    Assert( pfcbIdx->FDeletePending() );
    Assert( !pfcbIdx->FDeleteCommitted() );
//  Call( ErrDIRDeleteDirectory( pfucb, pfcbIdx->PgnoFDP() ) );

    Call( ErrDIRCommitTransaction( ppib, 0 ) );
    fInTransaction = fFalse;

    //  set currency to before first
    //
    DIRBeforeFirst( pfucb );
    CallS( err );

HandleError:
    OSTraceFMP(
        pfcbTable->Ifmp(),
        JET_tracetagDDLWrite,
        OSFormat(
            "Finished deletion of index '%s' with error %d (0x%x)",
            szIndex,
            err,
            err ) );

    if ( fInTransaction )
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    DIRClose( pfucb );
    AssertDIRNoLatch( ppib );
    return err;
}


// Ensures the column doesn't belong to any non-deleted indexes, or indexes
// version-deleted by someone else (because the delete may roll back).
BOOL FFILEIsIndexColumn( PIB *ppib, FCB *pfcbTable, const COLUMNID columnid )
{
    FCB*            pfcbIndex;
    ULONG           iidxseg;
    const IDXSEG*   rgidxseg;

    Assert( pfcbNil != pfcbTable );
    for ( pfcbIndex = pfcbTable; pfcbIndex != pfcbNil; pfcbIndex = pfcbIndex->PfcbNextIndex() )
    {
        const IDB * const pidb = pfcbIndex->Pidb();
        if ( pidbNil == pidb )
        {
            Assert( pfcbIndex == pfcbTable );   // Only sequential index has no IDB
            continue;
        }

        if ( pidb->FDeleted() )
        {
            if ( pidb->FVersioned() )
            {
                // The cursor that deletes the index will set DomainDenyRead
                // on the FCB.  If it's us, we can bypass the index check (because
                // this operation will only commit if the DeleteIndex commits),
                // otherwise we can't (because the DeleteIndex may roll back).
                Assert( pfcbIndex->FDomainDenyRead( ppib )
                    || pfcbIndex->FDomainDenyReadByUs( ppib ) );
                if ( pfcbIndex->FDomainDenyReadByUs( ppib ) )
                {
                    continue;
                }
            }
            else
            {
                // Index is unversioned deleted, meaning the delete has committed
                // or we have exclusive use of the table and we deleted the index.
                // In either case, we can bypass the index check.
                continue;
            }
        }

        rgidxseg = PidxsegIDBGetIdxSeg( pidb, pfcbTable->Ptdb() );
        for ( iidxseg = 0; iidxseg < pidb->Cidxseg(); iidxseg++ )
        {
            if ( rgidxseg[iidxseg].FIsEqual( columnid ) )
            {
                //  found the column in an index
                //
                return fTrue;
            }
        }

        rgidxseg = PidxsegIDBGetIdxSegConditional( pidb, pfcbTable->Ptdb() );
        for ( iidxseg = 0; iidxseg < pidb->CidxsegConditional(); iidxseg++ )
        {
            if ( rgidxseg[iidxseg].FIsEqual( columnid ) )
            {
                //  column is used for a condition
                //
                return fTrue;
            }
        }
    }
    return fFalse;
}


//  determines whether a version-deleted index has committed
//
BOOL FFILEIVersionDeletedIndexPotentiallyThere(
    FCB * const         pfcbTable,
    const FCB * const   pfcbIndex )
{
    const IDB * const   pidb                = pfcbIndex->Pidb();
    BOOL                fPotentiallyThere   = fTrue;

    //  once we've got the DML latch, verify the index is still version-deleted
    //
    //  UNDONE: we should always be in the Updating/Indexing latch when
    //  calling this function, so do we really need to enter the DML
    //  latch as well?  Probably not, but let's just be safe, since this
    //  is a rare code path anyways.
    //
    pfcbTable->EnterDML();

    if ( pidb->FDeleted() )
    {
        if ( pidb->FVersioned() )
        {
            ENTERCRITICALSECTION    critRCEList( &pfcbTable->CritRCEList() );
            const RCE *             prce;

            //  find corresponding DeleteIndex RCE in table list
            //
            for ( prce = pfcbTable->PrceNewest(); prceNil != prce; prce = prce->PrcePrevOfFCB() )
            {
                if ( operDeleteIndex == prce->Oper()
                    && pfcbIndex == *(FCB **)prce->PbData() )
                {
                    //  RCE should not be fully committed (otherwise
                    //  the Versioned bit would have been cleared),
                    //  so just need to check if a trxCommit0 has
                    //  already been assigned
                    //
                    Assert( !prce->FFullyCommitted() );
                    fPotentiallyThere = ( trxMax == prce->TrxCommitted() );
                    break;
                }
            }

#ifdef DEBUG
            if ( prceNil != prce )
            {
                //  verify that the table has no other DeleteIndex RCE's for this index
                //
                for ( prce = prce->PrcePrevOfFCB(); prceNil != prce; prce = prce->PrcePrevOfFCB() )
                {
                    Assert( operDeleteIndex != prce->Oper()
                        || pfcbIndex != *( FCB**)prce->PbData() );
                }
            }
            else
            {
                //  if we couldn't find the DeleteIndex RCE, it must mean that
                //  it has yet to be added, which means that the session deleting
                //  the index has already flagged the IDB, but has yet to insert
                //  the RCE (probably because we beat it into critRCEList), so
                //  the index is still potentially there
                //
                Assert( fPotentiallyThere );
            }
#endif
        }
        else
        {
            //  unversioned and deleted, so index is no longer potentially there
            //
            fPotentiallyThere = fFalse;
        }
    }

    pfcbTable->LeaveDML();

    return fPotentiallyThere;
}


ERR VTAPI ErrIsamDeleteColumn(
    JET_SESID       sesid,
    JET_VTID        vtid,
    const CHAR      *szName,
    const JET_GRBIT grbit )
{
    PIB             *ppib = reinterpret_cast<PIB *>( sesid );
    FUCB            *pfucb = reinterpret_cast<FUCB *>( vtid );
    ERR             err;
    CHAR            szColumn[ (JET_cbNameMost + 1) ];
    FCB             *pfcb;
    TDB             *ptdb;
    COLUMNID        columnidColToDelete;
    FIELD           *pfield;
    BOOL            fIndexColumn;
    VER             *pver;

    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables" );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    CallR( ErrUTILCheckName( szColumn, szName, (JET_cbNameMost + 1) ) );

    //  ensure that table is updatable
    //
    CallR( ErrFUCBCheckUpdatable( pfucb ) );
    CallR( ErrPIBCheckUpdatable( ppib ) );

    Assert( ppib != ppibNil );
    Assert( pfucb != pfucbNil );
    Assert( pfucb->u.pfcb != pfcbNil );
    pfcb = pfucb->u.pfcb;
    Assert( pfcb->WRefCount() > 0 );
    Assert( pfcb->FTypeTable() );       // Temp. tables have fixed DDL.
    Assert( pfcb->FPrimaryIndex() );

    if ( pfcb->FFixedDDL() )
    {
//  UNDONE: Cannot currently permit DDL deletes even if PermitDDL flag is
//  specified because DDL deletes will leave RCE's that have to be cleaned
//  up and modify the FCB while doing so.  This will mess up cursors
//  opened normally after the PermitDDL cursor closes.
#ifdef PERMIT_DDL_DELETE
        // Check FixedDDL override.
        if ( !FFUCBPermitDDL( pfucb ) )
        {
            err = ErrERRCheck( JET_errFixedDDL );
            return err;
        }

        // If DDL temporarily permitted, we must have exclusive use of the table.
        Assert( pfcb->FDomainDenyReadByUs( ppib ) );
#else
        err = ErrERRCheck( JET_errFixedDDL );
        return err;
#endif
    }

    if ( pfcb->Ptdb()->PfcbTemplateTable() != pfcbNil
        && !( grbit & JET_bitDeleteColumnIgnoreTemplateColumns )
        && FFILEITemplateTableColumn( pfcb->Ptdb()->PfcbTemplateTable(), szColumn ) )
    {
        return ErrERRCheck( JET_errFixedInheritedDDL );
    }


    CallR( ErrDIRBeginTransaction( ppib, 59685, NO_GRBIT ) );

    // Flag-delete in the catalog.
    // Also has the effect of properly reconciling concurrent
    // CreateIndexes over this column.
    Call( ErrCATDeleteTableColumn(
                ppib,
                pfcb->Ifmp(),
                pfcb->ObjidFDP(),
                szColumn,
                &columnidColToDelete ) );

#ifdef PERMIT_DDL_DELETE
    if ( pfcb->FTemplateTable() )
    {
        Assert( !COLUMNIDTemplateColumn( columnidColToDelete ) );   //  Template flag is not persisted
        COLUMNIDSetFTemplateColumn( columnidColToDelete );
    }
#endif

    // Search for column in use.  For indexes being concurrently created,
    // conflict would have been detected by catalog update above.
    pfcb->EnterDML();
    fIndexColumn = FFILEIsIndexColumn( ppib, pfcb, columnidColToDelete );
    pfcb->LeaveDML();

    if ( fIndexColumn )
    {
        err = ErrERRCheck( JET_errColumnInUse );
        goto HandleError;
    }

    pver = PverFromIfmp( pfucb->ifmp );
    Call( pver->ErrVERFlag( pfucb, operDeleteColumn, (VOID *)&columnidColToDelete, sizeof(COLUMNID) ) );

    pfcb->EnterDDL();

    ptdb = pfcb->Ptdb();
    pfield = ptdb->Pfield( columnidColToDelete );

    // If we have the table exclusively locked, then there's no need to
    // set the Versioned bit.
    Assert( !pfcb->FDomainDenyRead( ppib ) );
    if ( !pfcb->FDomainDenyReadByUs( ppib ) )
    {
        FIELDSetVersioned( pfield->ffield );
    }
    FIELDSetDeleted( pfield->ffield );

    pfcb->LeaveDDL();

    //  move to FDP root, then set currencies to BeforeFirst and remove unused CSR
    DIRGotoRoot( pfucb );
    Assert( Pcsr( pfucb ) != pcsrNil );
    DIRBeforeFirst( pfucb );
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        DIRBeforeFirst( pfucb->pfucbCurIndex );
    }

    Call( ErrDIRCommitTransaction( ppib, 0 ) );

    return JET_errSuccess;

HandleError:
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    return err;
}


//  ================================================================
ERR VTAPI ErrIsamRenameTable( JET_SESID sesid, JET_DBID dbid, const CHAR *szName, const CHAR *szNameNew )
//  ================================================================
//
//  WARNINGS:
//    This doesn't version the name properly. Its instantly visible in the TDB, but versioned in the catalog
//    You can't rename a template table (derived tables are not updated).
//
//
{
    ERR     err;
    PIB     * const ppib = reinterpret_cast<PIB *>( sesid );
    IFMP    ifmp = (IFMP) dbid;

    if ( FFMPIsTempDB( ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables" );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckUpdatable( ppib ) );
    CallR( ErrDBCheckUserDbid( ifmp ) );
    CallR( ErrPIBCheckIfmp( ppib, ifmp ) );

    if( NULL == szName )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if( NULL == szNameNew )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    CHAR    szTableOld[JET_cbNameMost+1];
    CHAR    szTableNew[JET_cbNameMost+1];

    CallR( ErrUTILCheckName( szTableOld, szName, JET_cbNameMost+1 ) );
    CallR( ErrUTILCheckName( szTableNew, szNameNew, JET_cbNameMost+1 ) );

    CallR( ErrCATRenameTable( ppib, ifmp, szTableOld, szTableNew ) );

    return err;
}


ERR VTAPI ErrIsamRenameObject( JET_SESID vsesid, JET_DBID   vdbid, const CHAR *szName, const CHAR  *szNameNew )
{
    Assert( fFalse );
    return JET_errSuccess;
}


//  ================================================================
ERR VTAPI ErrIsamRenameColumn(
    JET_SESID       vsesid,
    JET_VTID        vtid,
    const CHAR      *szName,
    const CHAR      *szNameNew,
    const JET_GRBIT grbit )
//  ================================================================
//
//  WARNINGS:
//    This doesn't version the name properly. Its instantly visible in the FIELD, but versioned in the catalog
//    You can't rename an inherited column.
//
//
{
    ERR     err;
    PIB     * const ppib    = reinterpret_cast<PIB *>( vsesid );
    FUCB    * const pfucb   = reinterpret_cast<FUCB *>( vtid );

    if ( FFMPIsTempDB( pfucb->ifmp ) )
    {
        AssertSz( fFalse, "Don't support DDL on temp tables" );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    
    CallR( ErrPIBCheck( ppib ) );
    CallR( ErrPIBCheckUpdatable( ppib ) );
    CheckTable( ppib, pfucb );

    if( NULL == szName )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if( NULL == szNameNew )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if( 0 != ppib->Level() )
    {
        return ErrERRCheck( JET_errInTransaction );
    }

    CHAR    szColumnOld[JET_cbNameMost+1];
    CHAR    szColumnNew[JET_cbNameMost+1];

    CallR( ErrUTILCheckName( szColumnOld, szName, JET_cbNameMost+1 ) );
    CallR( ErrUTILCheckName( szColumnNew, szNameNew, JET_cbNameMost+1 ) );

    CallR( ErrCATRenameColumn( ppib, pfucb, szColumnOld, szColumnNew, grbit ) );

    return err;
}


ERR VTAPI ErrIsamRenameIndex( JET_SESID vsesid, JET_VTID vtid, const CHAR *szName, const CHAR *szNameNew )
{
    Assert( fFalse );
    return JET_errSuccess;
}


VOID IDB::SetFlagsFromGrbit( const JET_GRBIT grbit )
{
    const BOOL  fPrimary            = ( grbit & JET_bitIndexPrimary );
    const BOOL  fUnique             = ( grbit & JET_bitIndexUnique );
    const BOOL  fDisallowNull       = ( grbit & JET_bitIndexDisallowNull );
    const BOOL  fIgnoreNull         = ( grbit & JET_bitIndexIgnoreNull );
    const BOOL  fIgnoreAnyNull      = ( grbit & JET_bitIndexIgnoreAnyNull );
    const BOOL  fIgnoreFirstNull    = ( grbit & JET_bitIndexIgnoreFirstNull );
    const BOOL  fSortNullsHigh      = ( grbit & JET_bitIndexSortNullsHigh );
    const BOOL  fTuples             = ( grbit & JET_bitIndexTuples );
    const BOOL  fCrossProduct       = ( grbit & JET_bitIndexCrossProduct );
    const BOOL  fNestedTable        = ( grbit & JET_bitIndexNestedTable );
    const BOOL  fDotNetGuid         = ( grbit & JET_bitIndexDotNetGuid );
    const BOOL  fDisallowTruncation = ( grbit & JET_bitIndexDisallowTruncation );

    ResetFlags();

    if ( !fDisallowNull && !fIgnoreAnyNull )
    {
        SetFAllowSomeNulls();
        if ( !fIgnoreFirstNull )
        {
            SetFAllowFirstNull();
            if ( !fIgnoreNull )
                SetFAllowAllNulls();
        }
    }

    if ( fPrimary )
    {
        SetFUnique();
        SetFPrimary();
    }
    else if ( fUnique )
    {
        SetFUnique();
    }

    if ( fTuples )
    {
        SetFTuples();
    }

    if ( fDisallowNull )
    {
        SetFNoNullSeg();
    }
    else if ( fSortNullsHigh )
    {
        SetFSortNullsHigh();
    }
    
    //  not both linear and cross product
    //
    Assert( !( fCrossProduct && fNestedTable ) );
    if ( fCrossProduct )
    {
        SetFCrossProduct();
    }
    else if ( fNestedTable )
    {
        SetFNestedTable();
    }

    if ( fDisallowTruncation )
    {
        SetFDisallowTruncation();
    }

    if ( fDotNetGuid )
    {
        SetFDotNetGuid();
    }

}

JET_GRBIT IDB::GrbitFromFlags() const
{
    JET_GRBIT   grbit = 0;

    if ( FPrimary() )
        grbit |= JET_bitIndexPrimary;
    if ( FUnique() )
        grbit |= JET_bitIndexUnique;
    if( FTuples() )
        grbit |= JET_bitIndexTuples;

    //  not both linear and cross product
    //
    Assert( !( FCrossProduct() && FNestedTable() ) );
    if ( FCrossProduct() )
        grbit |= JET_bitIndexCrossProduct;
    if ( FNestedTable() )
        grbit |= JET_bitIndexNestedTable;
    if ( FDotNetGuid() )
    {
        grbit |= JET_bitIndexDotNetGuid;
    }
    
    if ( FDisallowTruncation() )
        grbit |= JET_bitIndexDisallowTruncation;
    if ( FNoNullSeg() )
        grbit |= JET_bitIndexDisallowNull;
    else
    {
        if ( !FAllowAllNulls() )
            grbit |= JET_bitIndexIgnoreNull;
        if ( !FAllowFirstNull() )
            grbit |= JET_bitIndexIgnoreFirstNull;
        if ( !FAllowSomeNulls() )
            grbit |= JET_bitIndexIgnoreAnyNull;
        if ( FSortNullsHigh() )
            grbit |= JET_bitIndexSortNullsHigh;
    }

    if ( m_cbKeyMost > JET_cbKeyMost_OLD )
    {
        grbit |= JET_bitIndexKeyMost;
    }

    return grbit;
}

