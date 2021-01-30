// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define fBumpIndexCount         (1<<0)
#define fDropIndexCount         (1<<1)
#define fDDLStamp               (1<<2)
ERR ErrFILEIUpdateFDPData( FUCB *pfucb, ULONG grbit );


ERR ErrFILEGetPfieldAndEnterDML(
    PIB             * ppib,
    FCB             * pfcb,
    const CHAR      * szColumnName,
    FIELD           ** ppfield,
    COLUMNID        * pcolumnid,
    BOOL            * pfColumnWasDerived,
    const BOOL      fLockColumn );
ERR ErrFILEPfieldFromColumnName(
    PIB             * ppib,
    FCB             * pfcb,
    const CHAR      * szColumnName,
    FIELD           ** ppfield,
    COLUMNID        * pcolumnid );
ERR ErrFILEGetPfieldFromColumnid(
    FCB             *pfcb,
    const COLUMNID  columnid,
    FIELD           **ppfield );
BOOL FFILEIsIndexColumn(
    PIB             * ppib,
    FCB             * pfcbTable,
    const COLUMNID  columnid );
ERR ErrFILEGetNextColumnid(
    const JET_COLTYP    coltyp,
    const JET_GRBIT grbit,
    const BOOL      fTemplateTable,
    TCIB            *ptcib,
    COLUMNID        *pcolumnid );
ERR ErrFILEIGenerateIDB(
    FCB             * pfcb,
    TDB             * ptdb,
    IDB             * pidb);
ERR ErrTDBCreate(
    INST            * pinst,
    IFMP            ifmp,
    TDB             ** pptdb,
    TCIB            * ptcib,
    const BOOL      fSystemTDB = fFalse,
    FCB             * pfcbTemplateTable = pfcbNil,
    const BOOL      fAllocateNameSpace = fFalse );
ERR ErrRECAddFieldDef( TDB *ptdb, const COLUMNID columnid, FIELD *pfield );
VOID FILEPrepareDefaultRecord( FUCB *pfucbFake, FCB *pfcbFake, TDB *ptdb );
ERR ErrFILERebuildDefaultRec(
    FUCB            * pfucbFake,
    const COLUMNID  columnidToAdd,
    const DATA      * const pdataDefaultToAdd );
ERR ErrRECSetDefaultValue( FUCB *pfucbFake, const COLUMNID columnid, VOID *pvDefault, ULONG cbDefault );

INLINE VOID FILEFreeDefaultRecord( FUCB *pfucbFake )
{
    Assert( pfucbNil != pfucbFake );
    RECIFreeCopyBuffer( pfucbFake );
}

VOID FILETableMustRollback( PIB *ppib, FCB *pfcbTable );

ERR ErrFILEIInitializeFCB(
    PIB         *ppib,
    IFMP        ifmp,
    TDB         *ptdb,
    FCB         *ppfcbNew,
    IDB         *pidb,
    BOOL        fPrimary,
    PGNO        pgnoFDP,
    __in const JET_SPACEHINTS * const pjsph,
    __out FCB * pfcbTemplate );

VOID FILESetAllIndexMask( FCB *pfcbTable );
ERR ErrFILEDeleteTable( PIB *ppib, IFMP ifmp, const CHAR *szTable );

FIELD *PfieldFCBFromColumnName( FCB *pfcb, __in PCSTR szColumnName );
    
FCB *PfcbFCBFromIndexName( FCB *pfcbTable, __in PCSTR szName );

struct FDPINFO
{
    PGNO    pgnoFDP;
    OBJID   objidFDP;
};

ERR ErrFILECreateTable( PIB *ppib, IFMP ifmp, JET_TABLECREATE5_A *ptablecreate );

enum CATCheckIndicesFlags;

ERR ErrFILEOpenTable(
    _In_ PIB            *ppib,
    _In_ IFMP       ifmp,
    _Out_ FUCB      **ppfucb,
    _In_ const CHAR *szName,
    _In_ ULONG      grbit = NO_GRBIT,
    _In_opt_ FDPINFO        *pfdpinfo = NULL );
ERR ErrFILECloseTable( PIB *ppib, FUCB *pfucb );


const ULONG cFILEIndexBatchSizeDefault  = 16;

ERR ErrFILEIndexBatchInit(
    PIB         * const ppib,
    FUCB        ** const rgpfucbSort,
    FCB         * const pfcbIndexesToBuild,
    ULONG       * const pcIndexesToBuild,
    ULONG       * const rgcRecInput,
    FCB         **ppfcbNextBuildIndex,
    const ULONG cIndexBatchMax );
ERR ErrFILEIndexBatchAddEntry(
    FUCB        ** const rgpfucbSort,
    FUCB        * const pfucbTable,
    const BOOKMARK  * const pbmPrimary,
    DATA&       dataRec,
    FCB         * const pfcbIndexesToBuild,
    const ULONG cIndexesToBuild,
    ULONG       * const rgcRecInput,
    KEY&        keyBuffer );
ERR ErrFILEIndexBatchTerm(
    PIB         * const ppib,
    FUCB        ** const rgpfucbSort,
    FCB         * const pfcbIndexesToBuild,
    const ULONG cIndexesToBuild,
    ULONG       * const rgcRecInput,
    STATUSINFO  * const pstatus,
    BOOL        *pfCorruptionEncountered = NULL,
    CPRINTF     * const pcprintf = NULL );
ERR ErrFILEBuildAllIndexes(
    PIB         * const ppib,
    FUCB        * const pfucbTable,
    FCB         * const pfcbIndexes,
    STATUSINFO  * const pstatus,
    const ULONG cIndexBatchMaxRequested,
    const BOOL  fCheckOnly = fFalse,
    CPRINTF     * const pcprintf = NULL );

INLINE ERR ErrFILEIAccessIndex(
    PIB * const         ppib,
    FCB * const         pfcbTable,
    const FCB * const   pfcbIndex )
{
    Assert( pfcbNil != pfcbTable );
    Assert( pfcbNil != pfcbIndex );
    Assert( pidbNil != pfcbIndex->Pidb() );

    pfcbTable->AssertDML();

    ERR                 err         = JET_errSuccess;
    IDB * const         pidb        = pfcbIndex->Pidb();

    if ( pidb->FDeleted() )
    {
        Assert( pidb->CrefCurrentIndex() == 0 );
        Assert( pfcbIndex->FDeletePending() );
        err = ErrERRCheck( JET_errIndexNotFound );
    }
    else if ( pidb->FVersioned() )
    {
        ERR ErrCATAccessTableIndex(
                    PIB         *ppib,
                    const IFMP  ifmp,
                    const OBJID objidTable,
                    const OBJID objidIndex );
        
        Assert( dbidTemp != g_rgfmp[ pfcbTable->Ifmp() ].Dbid() );
        Assert( !pfcbTable->FFixedDDL() );
        pidb->IncrementVersionCheck();
        pfcbTable->LeaveDML();
        err = ErrCATAccessTableIndex(
                    ppib,
                    pfcbTable->Ifmp(),
                    pfcbTable->ObjidFDP(),
                    pfcbIndex->ObjidFDP() );
        pfcbTable->EnterDML();
        pidb->DecrementVersionCheck();
    }
    
    return err;
}

INLINE ERR ErrFILEIAccessIndexByName(
    PIB * const         ppib,
    FCB * const         pfcbTable,
    const FCB * const   pfcbIndex,
    const CHAR * const  szIndexName )
{
    Assert( NULL != szIndexName );
    Assert( pfcbNil != pfcbTable );
    Assert( pfcbNil != pfcbIndex );
    Assert( pidbNil != pfcbIndex->Pidb() );

    pfcbTable->AssertDML();

    const INT   cmp = UtilCmpName(
                            szIndexName,
                            pfcbTable->Ptdb()->SzIndexName( pfcbIndex->Pidb()->ItagIndexName(), pfcbIndex->FDerivedIndex() ) );

    return ( 0 == cmp ?
                ErrFILEIAccessIndex( ppib, pfcbTable, pfcbIndex ) :
                ErrERRCheck( JET_errIndexNotFound ) );
}


BOOL FFILEIVersionDeletedIndexPotentiallyThere(
    FCB * const         pfcbTable,
    const FCB * const   pfcbIndex );

INLINE BOOL FFILEIPotentialIndex(
    FCB * const         pfcbTable,
    const FCB * const   pfcbIndex )
{
    const IDB * const   pidb                = pfcbIndex->Pidb();
    BOOL                fPotentiallyThere;

    Assert( pfcbNil != pfcbTable );
    Assert( pfcbNil != pfcbIndex );
    Assert( pidbNil != pidb );

    Assert( pfcbTable->IsUnlocked() );

    if ( pidb->FDeleted() )
    {
        fPotentiallyThere = ( pidb->FVersioned() ?
                                    FFILEIVersionDeletedIndexPotentiallyThere( pfcbTable, pfcbIndex ) :
                                    fFalse );
    }
    else
    {
        fPotentiallyThere = fTrue;
    }

    return fPotentiallyThere;
}


INLINE VOID FILEReleaseCurrentSecondary( FUCB *pfucb )
{
    if ( FFUCBCurrentSecondary( pfucb ) )
    {
        Assert( FFUCBSecondary( pfucb ) );
        Assert( pfcbNil != pfucb->u.pfcb );
        Assert( pfucb->u.pfcb->FTypeSecondaryIndex() );
        Assert( pfucb->u.pfcb->Pidb() != pidbNil );
        Assert( pfucb->u.pfcb->PfcbTable() != pfcbNil );
        Assert( pfucb->u.pfcb->PfcbTable()->FPrimaryIndex() );
        FUCBResetCurrentSecondary( pfucb );

        if ( pfucb->u.pfcb->Pidb()->FIDBOwnedByFCB() || ( !pfucb->u.pfcb->FTemplateIndex() && !pfucb->u.pfcb->FDerivedIndex() ) )
        {
            pfucb->u.pfcb->Pidb()->DecrementCurrentIndex();
        }
    }
}

