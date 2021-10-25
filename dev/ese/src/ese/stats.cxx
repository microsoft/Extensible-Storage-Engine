// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

ERR ErrSTATSComputeIndexStats( PIB *ppib, FCB *pfcbIdx, FUCB *pfucbTable )
{
    ERR             err = JET_errSuccess;
    FUCB            *pfucbIdx;
    SR              sr;
    JET_DATESERIAL  dt;
    OBJID           objidTable;
    CHAR            szIndexName[JET_cbNameMost+1];

    CallR( ErrDIROpen( ppib, pfcbIdx, &pfucbIdx ) );
    Assert( pfucbIdx != pfucbNil );
    FUCBSetIndex( pfucbIdx );

    /*  initialize stats record
    /**/
    sr.cPages = sr.cItems = sr.cKeys = 0L;
    UtilGetDateTime( &dt );
    UtilMemCpy( &sr.dtWhenRun, &dt, sizeof sr.dtWhenRun );

    if ( pfcbIdx->FPrimaryIndex() )
    {
        objidTable = pfcbIdx->ObjidFDP();
    }
    else
    {
        FCB *pfcbTable = pfucbTable->u.pfcb;
        objidTable = pfcbTable->ObjidFDP();
        Assert( pfcbTable->Ptdb() != ptdbNil );

        Assert( pfcbIdx->FTypeSecondaryIndex() );
        if ( pfcbIdx->FDerivedIndex() )
        {
            Assert( pfcbIdx->Pidb()->FTemplateIndex() );
            pfcbTable = pfcbTable->Ptdb()->PfcbTemplateTable();
            Assert( pfcbNil != pfcbTable );
            Assert( pfcbTable->FTemplateTable() );
            Assert( pfcbTable->Ptdb() != ptdbNil );
        }
        
        pfcbTable->EnterDML();
        Assert( pfcbIdx->Pidb()->ItagIndexName() != 0 );
        OSStrCbCopyA( szIndexName, sizeof(szIndexName),
            pfcbTable->Ptdb()->SzIndexName( pfcbIdx->Pidb()->ItagIndexName() ) );
        pfcbTable->LeaveDML();
        FUCBSetSecondary( pfucbIdx );
    }
    
    Call( ErrDIRComputeStats( pfucbIdx, reinterpret_cast<INT *>( &sr.cItems ), reinterpret_cast<INT *>( &sr.cKeys ),
                                reinterpret_cast<INT *>( &sr.cPages ) ) );
    FUCBResetSecondary( pfucbIdx );

    /*  write stats
    /**/
    Call( ErrCATStats(
            ppib,
            pfucbIdx->ifmp,
            objidTable,
            pfcbIdx->FPrimaryIndex() ? NULL : szIndexName,
            &sr,
            fTrue ) );

HandleError:
    //  set secondary for cursor reuse support
    if ( !pfcbIdx->FPrimaryIndex() )
        FUCBSetSecondary( pfucbIdx );
    DIRClose( pfucbIdx );
    return err;
}


ERR VTAPI ErrIsamComputeStats( JET_SESID sesid, JET_VTID vtid )
{
    PIB     *ppib   = reinterpret_cast<PIB *>( sesid );
    FUCB    *pfucb = reinterpret_cast<FUCB *>( vtid );

    ERR     err = JET_errSuccess;
    FCB     *pfcbTable;
    FCB     *pfcbIdx;

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );

    // filters do not apply to stats
    if ( pfucb->pmoveFilterContext )
    {
        return ErrERRCheck( JET_errFilteredMoveNotSupported );
    }

    /*  start a transaction, in case anything fails
    /**/
    CallR( ErrDIRBeginTransaction( ppib, 63525, NO_GRBIT ) );

    /*  compute stats for each index
    /**/
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    Assert( !pfcbTable->FDeletePending() );
    Assert( !pfcbTable->FDeleteCommitted() );
    Call( ErrSTATSComputeIndexStats( ppib, pfcbTable, pfucb ) );
    
    pfcbTable->EnterDML();
    for ( pfcbIdx = pfcbTable->PfcbNextIndex(); pfcbIdx != pfcbNil; pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        Assert( pfcbIdx->FTypeSecondaryIndex() );
        Assert( pfcbIdx->Pidb() != pidbNil );

        err = ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIdx );

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
            pfcbTable->LeaveDML();
            // Because we're in a transaction, this guarantees that the FCB won't
            // be cleaned up while we're trying to retrieve stats.
            Call( ErrSTATSComputeIndexStats( ppib, pfcbIdx, pfucb ) );
            pfcbTable->EnterDML();
        }
    }
    pfcbTable->LeaveDML();

    /*  commit transaction if everything went OK
    /**/
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    return err;

HandleError:
    Assert( err < 0 );
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    return err;
}


/*=================================================================
ErrSTATSRetrieveStats

Description: Returns the number of records and pages used for a table

Parameters:     ppib                pointer to PIB for current session or ppibNil
                ifmp                database id or 0
                pfucb               cursor or pfucbNil
                szTableName         the name of the table or NULL
                pcRecord            pointer to count of records
                pcPage              pointer to count of pages

Errors/Warnings:
                JET_errSuccess or error from called routine.

=================================================================*/
ERR ErrSTATSRetrieveTableStats(
    PIB         * ppib,
    const IFMP  ifmp,
    _In_ PCSTR   szTable,
    LONG        * pcRecord,
    LONG        * pcKey,
    LONG        * pcPage )
{
    ERR         err;
    FUCB        * pfucb;
    SR          sr;

    CallR( ErrFILEOpenTable( ppib, ifmp, &pfucb, szTable, JET_bitTableReadOnly ) );

    Call( ErrCATStats(
                pfucb->ppib,
                pfucb->ifmp,
                pfucb->u.pfcb->ObjidFDP(),
                NULL,
                &sr,
                fFalse));

    /*  set output variables
    /**/
    if ( pcRecord )
        *pcRecord = sr.cItems;
    if ( pcPage )
        *pcPage = sr.cPages;
    if ( pcKey )
        *pcKey = sr.cKeys;

    CallS( err );

HandleError:
    CallS( ErrFILECloseTable( ppib, pfucb ) );
    return err;
}


ERR ErrSTATSRetrieveIndexStats(
    FUCB    *pfucbTable,
    _In_ PCSTR szIndex,
    BOOL    fPrimary,
    LONG    *pcItem,
    LONG    *pcKey,
    LONG    *pcPage )
{
    ERR     err;
    SR      sr;

    // The name is assumed to be valid.

    CallR( ErrCATStats(
                pfucbTable->ppib,
                pfucbTable->ifmp,
                pfucbTable->u.pfcb->ObjidFDP(),
                ( fPrimary ? NULL : szIndex),
                &sr,
                fFalse ) );

    /*  set output variables
    /**/
    if ( pcItem )
        *pcItem = sr.cItems;
    if ( pcPage )
        *pcPage = sr.cPages;
    if ( pcKey )
        *pcKey = sr.cKeys;

    CallS( err );

    return JET_errSuccess;
}


ERR VTAPI
ErrIsamGetRecordPosition( JET_SESID vsesid, JET_VTID vtid, JET_RECPOS *precpos, ULONG cbRecpos )
{
    ERR         err;
    ULONGLONG   ullLT;
    ULONGLONG   ullTotal;
    PIB *ppib = (PIB *)vsesid;
    FUCB *pfucb = (FUCB *)vtid;

    // filters do not apply to getting current record's approximate position
    if ( pfucb->pmoveFilterContext )
    {
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    Assert( FFUCBIndex( pfucb ) );

    C_ASSERT( sizeof( JET_RECPOS ) < sizeof( JET_RECPOS2 ) );

    if ( cbRecpos < sizeof( JET_RECPOS ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  get position of secondary or primary cursor
    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        Call( ErrDIRGetPosition( pfucb->pfucbCurIndex, &ullLT, &ullTotal ) );
    }
    else
    {
        Call( ErrDIRGetPosition( pfucb, &ullLT, &ullTotal ) );
    }

    // Give back the biggest struct we can.
    if ( cbRecpos < sizeof( JET_RECPOS2 ) )
    {
        Expected( cbRecpos == sizeof( JET_RECPOS ) );

        // Note we ignore overflow because that's the historic behavior.
        // Perhaps we should return an error?
        precpos->cbStruct = sizeof( JET_RECPOS );
        precpos->centriesLT = ( ULONG )ullLT;
        precpos->centriesTotal = ( ULONG )ullTotal;
        precpos->centriesInRange = 1;
    }
    else
    {
        // For the benefit of people who inadvertently requested a JET_RECPOS2
        // when they really wanted a JET_RECPOS, fill out the fields they expect.
        JET_RECPOS2 *precpos2 = reinterpret_cast< JET_RECPOS2 * >( precpos );
        precpos2->cbStruct = sizeof( JET_RECPOS2 );
        precpos2->centriesLTDeprecated = ( ULONG )ullLT;
        precpos2->centriesTotalDeprecated = ( ULONG )ullTotal;
        precpos2->centriesInRangeDeprecated = 1;
        precpos2->centriesLT = ullLT;
        precpos2->centriesTotal = ullTotal;
    }

HandleError:
    return err;
}


ERR ISAMAPI ErrIsamIndexRecordCount( JET_SESID sesid, JET_TABLEID tableid, ULONG64 *pullCount, ULONG64 ullCountMost )
{
    ERR     err;
    PIB     *ppib = (PIB *)sesid;
    FUCB    *pfucb;
    FUCB    *pfucbIdx;

    CallR( ErrPIBCheck( ppib ) );

    /*  get pfucb from tableid
    /**/
    CallR( ErrFUCBFromTableid( ppib, tableid, &pfucb ) );

    // filters do not apply to getting total record count
    if ( pfucb->pmoveFilterContext )
    {
        return ErrERRCheck( JET_errFilteredMoveNotSupported );
    }

    CheckTable( ppib, pfucb );

    /*  get cursor for current index
    /**/
    if ( pfucb->pfucbCurIndex != pfucbNil )
        pfucbIdx = pfucb->pfucbCurIndex;
    else
        pfucbIdx = pfucb;

    err = ErrDIRIndexRecordCount( pfucbIdx, pullCount, ullCountMost, fTrue );
    AssertDIRNoLatch( ppib );
    return err;
};

