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

    
    Call( ErrCATStats(
            ppib,
            pfucbIdx->ifmp,
            objidTable,
            pfcbIdx->FPrimaryIndex() ? NULL : szIndexName,
            &sr,
            fTrue ) );

HandleError:
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

    if ( pfucb->pmoveFilterContext )
    {
        return ErrERRCheck( JET_errFilteredMoveNotSupported );
    }

    
    CallR( ErrDIRBeginTransaction( ppib, 63525, NO_GRBIT ) );

    
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
            Call( ErrSTATSComputeIndexStats( ppib, pfcbIdx, pfucb ) );
            pfcbTable->EnterDML();
        }
    }
    pfcbTable->LeaveDML();

    
    Call( ErrDIRCommitTransaction( ppib, JET_bitCommitLazyFlush ) );

    return err;

HandleError:
    Assert( err < 0 );
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    return err;
}



ERR ErrSTATSRetrieveTableStats(
    PIB         * ppib,
    const IFMP  ifmp,
    __in PCSTR   szTable,
    LONG        * pcRecord,
    LONG        * pcKey,
    LONG        * pcPage )
{
    ERR         err;
    FUCB        * pfucb;
    SR          sr;

    CallR( ErrFILEOpenTable( ppib, ifmp, &pfucb, szTable ) );

    Call( ErrCATStats(
                pfucb->ppib,
                pfucb->ifmp,
                pfucb->u.pfcb->ObjidFDP(),
                NULL,
                &sr,
                fFalse));

    
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
    __in PCSTR szIndex,
    BOOL    fPrimary,
    LONG    *pcItem,
    LONG    *pcKey,
    LONG    *pcPage )
{
    ERR     err;
    SR      sr;


    CallR( ErrCATStats(
                pfucbTable->ppib,
                pfucbTable->ifmp,
                pfucbTable->u.pfcb->ObjidFDP(),
                ( fPrimary ? NULL : szIndex),
                &sr,
                fFalse ) );

    
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
    ERR     err;
    ULONG   ulLT;
    ULONG   ulTotal;
    PIB *ppib = (PIB *)vsesid;
    FUCB *pfucb = (FUCB *)vtid;

    if ( pfucb->pmoveFilterContext )
    {
        Error( ErrERRCheck( JET_errFilteredMoveNotSupported ) );
    }

    CallR( ErrPIBCheck( ppib ) );
    CheckTable( ppib, pfucb );
    Assert( FFUCBIndex( pfucb ) );

    if ( cbRecpos < sizeof(JET_RECPOS) )
        return ErrERRCheck( JET_errInvalidParameter );
    precpos->cbStruct = sizeof(JET_RECPOS);

    if ( pfucb->pfucbCurIndex != pfucbNil )
    {
        Call( ErrDIRGetPosition( pfucb->pfucbCurIndex, &ulLT, &ulTotal ) );
    }
    else
    {
        Call( ErrDIRGetPosition( pfucb, &ulLT, &ulTotal ) );
    }

    precpos->centriesLT = ulLT;
    precpos->centriesInRange = 1;
    precpos->centriesTotal = ulTotal;

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

    
    CallR( ErrFUCBFromTableid( ppib, tableid, &pfucb ) );

    if ( pfucb->pmoveFilterContext )
    {
        return ErrERRCheck( JET_errFilteredMoveNotSupported );
    }

    CheckTable( ppib, pfucb );

    
    if ( pfucb->pfucbCurIndex != pfucbNil )
        pfucbIdx = pfucb->pfucbCurIndex;
    else
        pfucbIdx = pfucb;

    err = ErrDIRIndexRecordCount( pfucbIdx, pullCount, ullCountMost, fTrue );
    AssertDIRNoLatch( ppib );
    return err;
};

