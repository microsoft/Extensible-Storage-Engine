// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

enum RECOPER
{
    recoperInsert,
    recoperDelete,
    recoperReplace
};

INLINE PCSTR SzRecoper( RECOPER recoper )
{
    LPSTR szReturn;

    switch ( recoper )
    {
        case recoperInsert:
            szReturn = "Insert";
            break;
        case recoperDelete:
            szReturn = "Delete";
            break;
        case recoperReplace:
            szReturn = "Replace";
            break;
        default:
            AssertSz( fFalse, "Unexpected case in switch." );
            szReturn = "Unknown";
            break;
    }

    return szReturn;
};

enum KEY_LOCATION
{
    keyLocationCopyBuffer,
    keyLocationRecord,
    keyLocationRecord_RetrieveLVBeforeImg,
    keyLocationRCE
};

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotalWithClass<> cRECInserts;
LONG LRECInsertsCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECInserts.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECDeletes;
LONG LRECDeletesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECDeletes.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECReplaces;
LONG LRECReplacesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECReplaces.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECNoOpReplaces;
LONG LRECNoOpReplacesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECNoOpReplaces.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECEscrowUpdates;
LONG LRECEscrowUpdatesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECEscrowUpdates.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECIndexInserts;
LONG LRECIndexInsertsCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECIndexInserts.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECIndexDeletes;
LONG LRECIndexDeletesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECIndexDeletes.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECFalseIndexColumnUpdates;
LONG LRECFalseIndexColumnUpdatesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECFalseIndexColumnUpdates.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotalWithClass<> cRECFalseTupleIndexColumnUpdates;
LONG LRECFalseTupleIndexColumnUpdatesCEFLPv( LONG iInstance, VOID* pvBuf )
{
    cRECFalseTupleIndexColumnUpdates.PassTo( iInstance, pvBuf );
    return 0;
}

#endif


INLINE ERR ErrRECIRetrieveKeyForEnumeration(
    FUCB * const        pfucb,
    const FCB * const   pfcbIdx,
    KEY * const         pkey,
    const ULONG         *rgitag,
    const KEY_LOCATION  eKeyLocation,
    const ULONG         ichOffset,
    RCE * const         prcePrimary,
    ULONG *             piidxseg )
{
    ERR               err;
    const IDB * const pidb  = pfcbIdx->Pidb();
    BOOL fRetrieveLVBeforeImg = fFalse;

    switch ( eKeyLocation )
    {
        case keyLocationCopyBuffer:
            Assert( prceNil != prcePrimary ?
                    pfcbIdx->PfcbTable() == pfcbNil :
                    pfcbIdx->PfcbTable() == pfucb->u.pfcb );

            CallR( ErrRECRetrieveKeyFromCopyBuffer(
                       pfucb,
                       pidb,
                       pkey,
                       rgitag,
                       ichOffset,
                       prcePrimary,
                       piidxseg ) );
            break;

        case keyLocationRecord_RetrieveLVBeforeImg:
            fRetrieveLVBeforeImg = fTrue;
            __fallthrough;

        case keyLocationRecord:
            Assert( pfcbIdx->PfcbTable() == pfucb->u.pfcb );

            CallR( ErrRECRetrieveKeyFromRecord(
                       pfucb,
                       pidb,
                       pkey,
                       rgitag,
                       ichOffset,
                       fRetrieveLVBeforeImg,
                       piidxseg ) );
            break;

        case keyLocationRCE:
        {
            Assert( pfcbIdx->PfcbTable() == pfcbNil );
            DATA    dataRec;

            dataRec.SetPv( const_cast<BYTE *>( prcePrimary->PbData() ) + cbReplaceRCEOverhead );
            dataRec.SetCb( prcePrimary->CbData() - cbReplaceRCEOverhead );
            CallR( ErrRECIRetrieveKey(
                       pfucb,
                       pidb,
                       dataRec,
                       pkey,
                       rgitag,
                       ichOffset,
                       fTrue,
                       prcePrimary,
                       piidxseg ) );
        }
        break;

        default:
            CallR( ErrERRCheck( JET_errInvalidParameter ) );
    }

    CallS( ErrRECValidIndexKeyWarning( err ) );
    return err;
}

struct INDEX_ENTRY_CALLBACK_CONTEXT
{
    RECOPER m_recoper;

    INDEX_ENTRY_CALLBACK_CONTEXT( RECOPER Recoper )
    {
        m_recoper = Recoper;
    };
};

typedef ERR ( __cdecl INDEX_ENTRY_CALLBACK )(
    FUCB *,
    KEY &,
    const KEY &,
    RCE *,
    const BOOL,
    BOOL *,
    INDEX_ENTRY_CALLBACK_CONTEXT *);

INDEX_ENTRY_CALLBACK ErrRECIInsertIndexEntry;
INDEX_ENTRY_CALLBACK ErrRECIDeleteIndexEntry;
INDEX_ENTRY_CALLBACK ErrRECITrackIndexEntry;

ERR ErrRECIEnumerateKeys(
    FUCB                         *pfucb,
    FUCB                         *pfucbIdx,
    const BOOKMARK               *pbmPrimary,
    RCE                          *prcePrimary,
    KEY_LOCATION                 eKeyLocation,
    BOOL                         fErrorOnNullSegViolation,    
    BOOL                         *pfIndexUpdated,
    INDEX_ENTRY_CALLBACK_CONTEXT *pIndexEntryCallbackContext,
    INDEX_ENTRY_CALLBACK         *pfnIndexEntryCallback
    )
{
    ERR             err                     = JET_errSuccess;
    const FCB       * const pfcbIdx         = pfucbIdx->u.pfcb;
    const IDB       * const pidb            = pfcbIdx->Pidb();
    FCB * const     pfcb                    = pfucb->u.pfcb;
    Assert( NULL != pfcb );
    TDB * const     ptdb                    = pfcb->Ptdb();
    Assert( NULL != ptdb );

    const BOOL      fAllowAllNulls          = !!pidb->FAllowAllNulls();
    const BOOL      fAllowFirstNull         = !!pidb->FAllowFirstNull();
    const BOOL      fAllowSomeNulls         = !!pidb->FAllowSomeNulls();
    const BOOL      fTuples                 = !!pidb->FTuples();
    const BOOL      fNoNullSeg              = !!pidb->FNoNullSeg();
    const BOOL      fUnique                 = !!pidb->FUnique();
    const BOOL      fHasMultivalue          = !!pidb->FMultivalued();
    BOOL            fFirstColumnMultiValued = FRECIFirstIndexColumnMultiValued( pfucb, pidb );

    KEY             keyRead;
    BYTE            *pbReadKey              = NULL;

    Assert( ( pfnIndexEntryCallback == ErrRECIInsertIndexEntry ) ||
            ( pfnIndexEntryCallback == ErrRECIDeleteIndexEntry ) ||
            ( pfnIndexEntryCallback == ErrRECITrackIndexEntry  )   );

    Assert( NULL != pIndexEntryCallbackContext );
    Assert( NULL != pfnIndexEntryCallback );
    Assert( NULL != pfIndexUpdated );

    Alloc( pbReadKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyRead.prefix.Nullify();
    keyRead.suffix.SetPv( pbReadKey );

    err = JET_errSuccess;

    ULONG ichOffsetStart;
    ULONG ichIncrement;
    ULONG ulTupleMax;

    if ( fTuples )
    {
        ichOffsetStart = pidb->IchTuplesStart();
        ulTupleMax = pidb->IchTuplesToIndexMax();
        ichIncrement = pidb->CchTuplesIncrement();
        Assert( 0 < ulTupleMax );
        Assert( 0 < ichIncrement );
    }
    else
    {
        ichOffsetStart = 0;
        ichIncrement = 1;
        ulTupleMax = 1;
    }

    ULONG iidxseg = ULONG( -1 );
    for ( KeySequence ksRead( pfucb, pidb ); !ksRead.FSequenceComplete(); ksRead.Increment( iidxseg ) )
    {
        ULONG ichOffset;
        ULONG ulTuple;

        for ( ichOffset = ichOffsetStart, ulTuple = 0, err = JET_errSuccess;
              ( JET_errSuccess == err ) && ( ulTuple < ulTupleMax );
              ichOffset += ichIncrement, ulTuple++ )
        {
            Call( ErrRECIRetrieveKeyForEnumeration(
                      pfucb,
                      pfcbIdx,
                      &keyRead,
                      ksRead.Rgitag(),
                      eKeyLocation,
                      ichOffset,
                      prcePrimary,
                      &iidxseg ) );

            if ( fNoNullSeg )
            {
                switch (err)
                {
                    case wrnFLDNullKey:
                    case wrnFLDNullFirstSeg:
                    case wrnFLDNullSeg:
                        if ( fErrorOnNullSegViolation )
                        {
                            Call( ErrERRCheck( JET_errNullKeyDisallowed ) );
                        }

                        Assert( !fNoNullSeg );
                        break;

                    default:
                        break;
                }
            }

            switch ( err ) {
                case JET_errSuccess:
                    Assert( iidxsegComplete != iidxseg );
                    break;

                case wrnFLDOutOfKeys:
                    Assert( !ksRead.FBaseKey() );
                    continue;

                case wrnFLDOutOfTuples:
                    Assert ( fTuples );

                    if ( !fFirstColumnMultiValued )
                    {
                        iidxseg = iidxsegComplete;
                    }
                    continue;

                case wrnFLDNotPresentInIndex:
                    Assert( 0 == ulTuple );
                    Assert( ksRead.FBaseKey() );
                    iidxseg = iidxsegComplete;
                    continue;

                case wrnFLDNullKey:
                    Assert( !fTuples );
                    Assert( ksRead.FBaseKey() );

                    iidxseg = iidxsegComplete;

                    if ( !fAllowAllNulls ) {
                        continue;
                    }

                    if ( 0 != ulTuple ) {
                        continue;
                    }
                    break;

                case wrnFLDNullFirstSeg:
                    Assert( 0 == ulTuple );
                    if (!fAllowFirstNull )
                    {
                        iidxseg = iidxsegComplete;
                        continue;
                    }
                    break;

                case wrnFLDNullSeg:
                    if (!fAllowSomeNulls )
                    {
                        iidxseg = iidxsegComplete;
                        continue;
                    }
                    break;

                default:
                    AssertSz( fFalse, "Ignored unexpected non-Success condition.");
                    break;
            }

            const BOOL  fMayHaveAlreadyBeenModified  = ( !( ksRead.FBaseKey() && ( 0 == ulTuple ) )
                                                          && !fUnique
                                                          && *pfIndexUpdated );
            Call( pfnIndexEntryCallback(
                      pfucbIdx,
                      keyRead,
                      pbmPrimary->key,
                      prcePrimary,
                      fMayHaveAlreadyBeenModified,
                      pfIndexUpdated,
                      pIndexEntryCallbackContext ) );

            err = JET_errSuccess;
        }

        if ( iidxsegComplete == iidxseg) {
            continue;
        }

        if ( !fHasMultivalue )
        {
            Assert( ksRead.FBaseKey() );
            iidxseg = iidxsegComplete;
            continue;
        }
    }

HandleError:
    RESKEY.Free( pbReadKey );
    return err;
}

LOCAL ERR ErrRECIInsert(
    FUCB *pfucb,
    _Out_writes_bytes_to_opt_(cbMax, *pcbActual) VOID * pv,
    ULONG cbMax,
    ULONG *pcbActual,
    const JET_GRBIT grbit );
LOCAL ERR ErrRECIReplace( FUCB *pfucb, const JET_GRBIT grbit );


LOCAL ERR ErrRECICallback(
        PIB * const ppib,
        FUCB * const pfucb,
        TDB * const ptdb,
        const JET_CBTYP cbtyp,
        const ULONG ulId,
        void * const pvArg1,
        void * const pvArg2,
        const ULONG ulUnused )
{
    const CBDESC *  pcbdesc;
    for ( pcbdesc = ptdb->Pcbdesc();
        NULL != pcbdesc;
        pcbdesc = pcbdesc->pcbdescNext )
    {
        if( ( pcbdesc->cbtyp & cbtyp )
            && ( ulId == pcbdesc->ulId ) )
        {
            break;
        }
    }

    if( NULL == pcbdesc )
    {
        return JET_errSuccess;
    }


    if(JET_errSuccess != ErrFaultInjection(36013))
    {
        UtilSleep( cmsecWaitGeneric );
    }


    ERR                     err             = JET_errSuccess;
    const JET_SESID         sesid           = (JET_SESID)ppib;
    const JET_TABLEID       tableid         = (JET_TABLEID)pfucb;
    const JET_DBID          ifmp            = (JET_DBID)pfucb->ifmp;
    const VTFNDEF * const   pvtfndefSaved   = pfucb->pvtfndef;

    pfucb->pvtfndef = &vtfndefIsamCallback;

    while( NULL != pcbdesc && err >= JET_errSuccess )
    {
        BOOL fVisible = fTrue;
#ifdef VERSIONED_CALLBACKS
        if( !( pcbdesc->fVersioned ) )
        {
            fVisible = fTrue;
        }
        else
        {
            if( trxMax == pcbdesc->trxRegisterCommit0 )
            {
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax == pcbdesc->trxRegisterCommit0 );
                Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
                fVisible = trxSession == pcbdesc->trxRegisterBegin0;
            }
            else if( trxMax == pcbdesc->trxUnregisterBegin0 )
            {
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax != pcbdesc->trxRegisterCommit0 );
                Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
                fVisible = TrxCmp( trxSession, pcbdesc->trxRegisterCommit0 ) > 0;
            }
            else if( trxMax == pcbdesc->trxUnregisterCommit0 )
            {
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax != pcbdesc->trxRegisterCommit0 );
                Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
                fVisible = trxSession != pcbdesc->trxUnregisterBegin0;
            }
            else
            {
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax != pcbdesc->trxRegisterCommit0 );
                Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax != pcbdesc->trxUnregisterCommit0 );
                fVisible = TrxCmp( trxSession, pcbdesc->trxUnregisterCommit0 ) <= 0;
            }
        }
#endif

        if( fVisible
            && ( pcbdesc->cbtyp & cbtyp )
            && ulId == pcbdesc->ulId )
        {
            ++(Ptls()->ccallbacksNested);
            Ptls()->fInCallback = fTrue;

            OSTraceFMP(
                ifmp,
                JET_tracetagDMLRead,
                OSFormat(
                    "Session=[0x%p:0x%x] starting callback on objid=[0x%x:0x%x] [pcallback=0x%p,cbtyp=0x%x]",
                    ppib,
                    ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
                    ifmp,
                    pfucb->u.pfcb->ObjidFDP(),
                    pcbdesc->pcallback,
                    pcbdesc->cbtyp ) );

            TRY
            {
                if (    pcbdesc->fPermanent &&
                        !BoolParam( PinstFromPpib( ppib ), JET_paramEnablePersistedCallbacks ) )
                {
                    err = ErrERRCheck( JET_errCallbackFailed );
                }
                else
                {
                    err = (*pcbdesc->pcallback)(
                            sesid,
                            ifmp,
                            tableid,
                            cbtyp & pcbdesc->cbtyp,
                            pvArg1,
                            pvArg2,
                            pcbdesc->pvContext,
                            ulUnused );
                }
            }
            EXCEPT( efaExecuteHandler )
            {
                err = ErrERRCheck( JET_errCallbackFailed );
            }
            ENDEXCEPT;

            OSTraceFMP(
                ifmp,
                JET_tracetagDMLRead,
                OSFormat(
                    "Ended callback with error %d (0x%x)",
                    err,
                    err ) );

            if ( JET_errSuccess != err )
            {
                ErrERRCheck( err );
            }
            if( 0 == ( --(Ptls()->ccallbacksNested) ) )
            {
                Ptls()->fInCallback = fFalse;
            }
        }
        pcbdesc = pcbdesc->pcbdescNext;
    }

    pfucb->pvtfndef = pvtfndefSaved;

    return err;
}


ERR ErrRECCallback(
        PIB * const ppib,
        FUCB * const pfucb,
        const JET_CBTYP cbtyp,
        const ULONG ulId,
        void * const pvArg1,
        void * const pvArg2,
        const ULONG ulUnused )
{
    Assert( JET_cbtypNull != cbtyp );

    ERR err = JET_errSuccess;

    FCB * const pfcb = pfucb->u.pfcb;
    Assert( NULL != pfcb );
    TDB * const ptdb = pfcb->Ptdb();
    Assert( NULL != ptdb );

    Assert( err >= JET_errSuccess );

    if( NULL != ptdb->Pcbdesc() )
    {
        err = ErrRECICallback( ppib, pfucb, ptdb, cbtyp, ulId, pvArg1, pvArg2, ulUnused );
    }

    if( err >= JET_errSuccess )
    {
        FCB * const pfcbTemplate = ptdb->PfcbTemplateTable();
        if( pfcbNil != pfcbTemplate )
        {
            TDB * const ptdbTemplate = pfcbTemplate->Ptdb();
            err = ErrRECICallback( ppib, pfucb, ptdbTemplate, cbtyp, ulId, pvArg1, pvArg2, ulUnused );
        }
    }

    return err;
}


ERR VTAPI ErrIsamUpdate(
    JET_SESID   sesid,
    JET_VTID    vtid,
    _Out_writes_bytes_to_opt_(cbMax, *pcbActual) VOID       *pv,
    ULONG       cbMax,
    ULONG       *pcbActual,
    JET_GRBIT   grbit )
{
    PIB * const ppib    = reinterpret_cast<PIB *>( sesid );
    FUCB * const pfucb  = reinterpret_cast<FUCB *>( vtid );

    ERR         err;

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    if ( FFUCBReplacePrepared( pfucb ) )
    {
        BOOKMARK *pbm;

        if ( cbMax > 0 )
        {
            CallR( ErrDIRGetBookmark( pfucb, &pbm ) );
            AssertDIRNoLatch( ppib );
            Assert( pbm->data.Cb() == 0 );
            Assert( pbm->key.Cb() > 0 );
            Assert( pbm->key.prefix.FNull() );

            if ( pcbActual != NULL )
            {
                *pcbActual = pbm->key.Cb();
            }

            pbm->key.CopyIntoBuffer( pv, min( cbMax, (ULONG)pbm->key.Cb() ) );
        }

        Assert( pfucb->ppib == ppib );
        err = ErrRECIReplace( pfucb, grbit );
    }
    else if ( FFUCBInsertPrepared( pfucb ) )
    {
        err = ErrRECIInsert( pfucb, pv, cbMax, pcbActual, grbit );
    }
    else
        err = ErrERRCheck( JET_errUpdateNotPrepared );

    if ( err >= 0 && !g_fRepair )
    {
        RECIFreeCopyBuffer( pfucb );
    }

    AssertDIRNoLatch( ppib );
    Assert( err != JET_errNoCurrentRecord );
    return err;
}

LOCAL ERR ErrRECIUpdateIndex(
    FUCB            *pfucb,
    FCB             *pfcbIdx,
    const RECOPER   recoper,
    const DIB       *pdib = NULL )
{
    ERR             err = JET_errSuccess;
    FUCB            *pfucbIdx;

    Assert( pfcbIdx != pfcbNil );
    Assert( pfcbIdx->PfcbTable()->Ptdb() != ptdbNil );
    Assert( pfcbIdx->Pidb() != pidbNil );
    Assert( pfucb != pfucbNil );
    Assert( pfucb->ppib != ppibNil );
    Assert( pfucb->ppib->Level() < levelMax );
    Assert( !Pcsr( pfucb )->FLatched() );
    AssertDIRNoLatch( pfucb->ppib );

    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( pfcbIdx->PfcbTable() == pfucb->u.pfcb );

    CallR( ErrDIROpen( pfucb->ppib, pfcbIdx, &pfucbIdx ) );
    Assert( pfucbIdx != pfucbNil );
    FUCBSetIndex( pfucbIdx );
    FUCBSetSecondary( pfucbIdx );

    Assert( FFUCBPrimary( pfucb ) );
    Assert( FFUCBUnique( pfucb ) );

#ifdef DEBUG
    const BOOKMARK      *pbmPrimary     = ( recoperInsert == recoper ? pdib->pbm : &pfucb->bmCurr );
    Assert( pbmPrimary->key.prefix.FNull() );
    Assert( pbmPrimary->key.Cb() > 0 );
    Assert( pbmPrimary->data.FNull() );
#endif

    switch (recoper) {
        case recoperInsert:
            Assert( NULL != pdib );
            err = ErrRECIAddToIndex( pfucb, pfucbIdx, pdib->pbm, pdib->dirflag );
            break;

        case recoperDelete:
            Assert( NULL == pdib );
            err = ErrRECIDeleteFromIndex( pfucb, pfucbIdx, &pfucb->bmCurr );
            break;

        default:
            Assert( recoperReplace == recoper);
            Assert( NULL == pdib );
            err = ErrRECIReplaceInIndex( pfucb, pfucbIdx, &pfucb->bmCurr );
    }

    DIRClose( pfucbIdx );

    AssertDIRNoLatch( pfucb->ppib );
    return err;
}

LOCAL BOOL FRECIAllSparseIndexColumnsSet(
    const IDB * const       pidb,
    const FUCB * const      pfucbTable )
{
    Assert( pidb->FSparseIndex() );

    if ( pidb->CidxsegConditional() > 0 )
    {

        const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
        const TDB * const       ptdb        = pfcbTable->Ptdb();
        const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSeg( pidb, ptdb );
        const IDXSEG * const    pidxsegMac  = pidxseg + pidb->Cidxseg();

        Assert( pidxseg < pidxsegMac );
        for ( ; pidxseg < pidxsegMac; pidxseg++ )
        {
            if ( !FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
            {
                return fFalse;
            }
        }
    }
    else
    {

        const LONG_PTR *        plIdx       = (LONG_PTR *)pidb->RgbitIdx();
        const LONG_PTR * const  plIdxMax    = plIdx + ( cbRgbitIndex / sizeof(LONG_PTR) );
        const LONG_PTR *        plSet       = (LONG_PTR *)pfucbTable->rgbitSet;

        for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
        {
            if ( (*plIdx & *plSet) != *plIdx )
            {
                return fFalse;
            }
        }
    }

    return fTrue;
}

LOCAL BOOL FRECIAnySparseIndexColumnSet(
    const IDB * const       pidb,
    const FUCB * const      pfucbTable )
{
    Assert( pidb->FSparseIndex() );

    if ( pidb->CidxsegConditional() > 0 )
    {

        const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
        const TDB * const       ptdb        = pfcbTable->Ptdb();
        const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSeg( pidb, ptdb );
        const IDXSEG * const    pidxsegMac  = pidxseg + pidb->Cidxseg();

        Assert( pidxseg < pidxsegMac );
        for ( ; pidxseg < pidxsegMac; pidxseg++ )
        {
            if ( FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
            {
                return fTrue;
            }
        }
    }

    else
    {

        const LONG_PTR *        plIdx       = (LONG_PTR *)pidb->RgbitIdx();
        const LONG_PTR * const  plIdxMax    = plIdx + ( cbRgbitIndex / sizeof(LONG_PTR) );
        const LONG_PTR *        plSet       = (LONG_PTR *)pfucbTable->rgbitSet;

        for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
        {
            if ( *plIdx & *plSet )
            {
                return fTrue;
            }
        }
    }

    return fFalse;
}


INLINE BOOL FRECIPossiblyUpdateSparseIndex(
    const IDB * const       pidb,
    const FUCB * const      pfucbTable )
{
    Assert( pidb->FSparseIndex() );

    if ( !pidb->FAllowSomeNulls() )
    {
        return FRECIAllSparseIndexColumnsSet( pidb, pfucbTable );
    }
    else if ( !pidb->FAllowFirstNull() )
    {
        const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
        const TDB * const       ptdb        = pfcbTable->Ptdb();
        const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSeg( pidb, ptdb );
        return FFUCBColumnSet( pfucbTable, pidxseg->Fid() );
    }
    else
    {
        return FRECIAnySparseIndexColumnSet( pidb, pfucbTable );
    }
}

LOCAL BOOL FRECIPossiblyUpdateSparseConditionalIndex(
    const IDB * const   pidb,
    const FUCB * const  pfucbTable )
{
    Assert( pidb->FSparseConditionalIndex() );
    Assert( pidb->CidxsegConditional() > 0 );

    const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
    const TDB * const       ptdb        = pfcbTable->Ptdb();
    const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
    const IDXSEG * const    pidxsegMac  = pidxseg + pidb->CidxsegConditional();

    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        if ( !FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
        {
            const FIELD * const pfield  = ptdb->Pfield( pidxseg->Columnid() );

            if ( !FFIELDUserDefinedDefault( pfield->ffield ) )
            {
                const BOOL  fHasDefault = FFIELDDefault( pfield->ffield );
                const BOOL  fSparse     = ( pidxseg->FMustBeNull() ?
                                                fHasDefault :
                                                !fHasDefault );
                if ( fSparse )
                {
                    return fFalse;
                }
            }
        }
    }

    return fTrue;
}


LOCAL BOOL FRECIHasUserDefinedColumns( const IDXSEG * const pidxseg, const INT cidxseg, const TDB * const ptdb )
{
    INT iidxseg;
    for( iidxseg = 0; iidxseg < cidxseg; ++iidxseg )
    {
        const FIELD * const pfield = ptdb->Pfield( pidxseg[iidxseg].Columnid() );
        if( FFIELDUserDefinedDefault( pfield->ffield ) )
        {
            return fTrue;
        }
    }
    return fFalse;
}

LOCAL BOOL FRECIHasUserDefinedColumns( const IDB * const pidb, const TDB * const ptdb )
{
    const INT cidxseg = pidb->Cidxseg();
    const IDXSEG * const pidxseg = PidxsegIDBGetIdxSeg( pidb, ptdb );
    if( FRECIHasUserDefinedColumns( pidxseg, cidxseg, ptdb ) )
    {
        return fTrue;
    }
    else if( pidb->CidxsegConditional() > 0 )
    {
        const INT cidxsegConditional = pidb->CidxsegConditional();
        const IDXSEG * const pidxsegConditional = PidxsegIDBGetIdxSegConditional( pidb, ptdb );
        return FRECIHasUserDefinedColumns( pidxsegConditional, cidxsegConditional, ptdb );
    }
    return fFalse;
}


LOCAL VOID RECIReportIndexCorruption( const FCB * const pfcbIdx )
{
    const IDB * const   pidb                    = pfcbIdx->Pidb();
    INT                 iszT                    = 0;
    const WCHAR *       rgcwszT[4];

    Assert( pidbNil != pidb );
    Assert( !pidb->FPrimary() );

    if ( pfcbNil == pfcbIdx->PfcbTable() )
        return;


    const BOOL          fHasUserDefinedColumns  = FRECIHasUserDefinedColumns(
                                                            pidb,
                                                            pfcbIdx->PfcbTable()->Ptdb() );

    CAutoWSZDDL szIndexName;
    CAutoWSZDDL szTableName;

    CallS( szIndexName.ErrSet( pfcbIdx->PfcbTable()->Ptdb()->SzIndexName(
                                                        pidb->ItagIndexName(),
                                                        pfcbIdx->FDerivedIndex() ) ) );

    CallS( szTableName.ErrSet( pfcbIdx->PfcbTable()->Ptdb()->SzTableName() ) );

    rgcwszT[iszT++] = (const WCHAR*)szIndexName;
    rgcwszT[iszT++] = (const WCHAR*)szTableName;
    rgcwszT[iszT++] = g_rgfmp[pfcbIdx->Ifmp()].WszDatabaseName();
    rgcwszT[iszT++] = fHasUserDefinedColumns ? L"1" : L"0";

    Assert( iszT == _countof( rgcwszT ) );

    UtilReportEvent(
            eventError,
            DATABASE_CORRUPTION_CATEGORY,
            INDEX_CORRUPTED_ID,
            iszT,
            rgcwszT,
            0,
            NULL,
            PinstFromIfmp( pfcbIdx->Ifmp() ) );

    OSUHAPublishEvent(  HaDbFailureTagCorruption,
                        PinstFromIfmp( pfcbIdx->Ifmp() ),
                        HA_DATABASE_CORRUPTION_CATEGORY,
                        HaDbIoErrorNone, NULL, 0, 0,
                        HA_INDEX_CORRUPTED_ID,
                        iszT,
                        rgcwszT );
}


LOCAL ERR ErrRECICheckESE97Compatibility( FUCB * const pfucb, const DATA& dataRec )
{
    ERR                 err;
    FCB * const         pfcbTable       = pfucb->u.pfcb;
    const REC * const   prec            = (REC *)dataRec.Pv();
    SIZE_T              cbRecESE97;

    Assert( pfcbTable->FTypeTable() );

    cbRecESE97 = prec->PbTaggedData() - (BYTE *)prec;
    Assert( cbRecESE97 <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( cbRecESE97 <= (SIZE_T)REC::CbRecordMost( pfucb ) );
    Assert( cbRecESE97 <= (SIZE_T)dataRec.Cb() );

    const SIZE_T        cbESE97Remaining            = ( REC::CbRecordMost( g_rgfmp[ pfucb->ifmp ].CbPage(), JET_cbKeyMost_OLD ) - cbRecESE97 );
    const SIZE_T        cESE97MultiValues           = cbESE97Remaining / sizeof(TAGFLD);

    const SIZE_T        cbESE98ColumnOverhead       = 5;
    const SIZE_T        cbESE98MultiValueOverhead   = 2;

    const SIZE_T        cbESE98Threshold            = cbESE98ColumnOverhead
                                                        + ( cESE97MultiValues * cbESE98MultiValueOverhead );
    const BOOL          fRecordWillFitInESE97       = ( ( dataRec.Cb() - cbRecESE97 ) <= cbESE98Threshold );

    if ( fRecordWillFitInESE97 )
    {
#ifdef DEBUG
#else
        return JET_errSuccess;
#endif
    }
    else
    {
    }


    CTaggedColumnIter   citerTagged;
    Call( citerTagged.ErrInit( pfcbTable ) );
    Call( citerTagged.ErrSetRecord( dataRec ) );

    while ( errRECNoCurrentColumnValue != ( err = citerTagged.ErrMoveNext() ) )
    {
        COLUMNID    columnid;
        size_t      cbESE97Format;

        Call( err );

        Call( citerTagged.ErrGetColumnId( &columnid ) );
        err = ErrRECIAccessColumn( pfucb, columnid );
        if ( JET_errColumnNotFound != err )
        {
            Call( err );

            Call( citerTagged.ErrCalcCbESE97Format( &cbESE97Format ) );
            cbRecESE97 += cbESE97Format;

            if ( cbRecESE97 > (SIZE_T)REC::CbRecordMost( g_rgfmp[ pfucb->ifmp ].CbPage(), JET_cbKeyMost_OLD ) )
            {
                Assert( !fRecordWillFitInESE97 );
                Error( ErrERRCheck( JET_errRecordTooBigForBackwardCompatibility ) );
            }
        }
    }

    Assert( cbRecESE97 <= (SIZE_T)REC::CbRecordMost( g_rgfmp[ pfucb->ifmp ].CbPage(), JET_cbKeyMost_OLD ) );
    err = JET_errSuccess;

HandleError:
    return err;
}


LOCAL ERR ErrRECIInitDbkMost( FUCB * const pfucb )
{
    ERR     err;
    DIB     dib;
    DBK     dbk     = 0;

    DIRGotoRoot( pfucb );

    dib.dirflag = fDIRAllNode;
    dib.pbm = NULL;
    dib.pos = posLast;
    err = ErrDIRDown( pfucb, &dib );
    Assert( JET_errNoCurrentRecord != err );
    if ( JET_errRecordNotFound == err )
    {
        Assert( 0 == dbk );
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
        CallS( err );

        Assert( Pcsr( pfucb )->FLatched() );
        LongFromKey( &dbk, pfucb->kdfCurr.key );
        Assert( dbk > 0 );

        DIRUp( pfucb );
    }

    dbk++;

    pfucb->u.pfcb->Ptdb()->InitDbkMost( dbk );

HandleError:
    Assert( !Pcsr( pfucb )->FLatched( ) );
    return err;
}


LOCAL ERR ErrRECIInsert(
    FUCB *          pfucb,
    _Out_writes_bytes_to_opt_(cbMax, *pcbActual) VOID *         pv,
    ULONG           cbMax,
    ULONG *         pcbActual,
    const JET_GRBIT grbit )
{
    ERR             err;
    PIB *           ppib                    = pfucb->ppib;
    KEY             keyToAdd;
    BYTE            *pbKey                  = NULL;
    FCB *           pfcbTable;
    TDB *           ptdb;
    FCB *           pfcbIdx;
    FUCB *          pfucbT                  = pfucbNil;
    BOOL            fUpdatingLatchSet       = fFalse;
    ULONG           iidxsegT;

    DIRFLAG fDIRFlags = fDIRNull;
    BOOL    fNoVersionUpdate = fFalse;

    FID     fidVersion;

    CheckPIB( ppib );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    Assert( FFUCBUpdatable( pfucb ) );
    Assert( FFUCBInsertPrepared( pfucb ) );

    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    ptdb = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdb );

    CallR( ErrDIRBeginTransaction( ppib, 52005, NO_GRBIT ) );

    if ( ( 0 != (grbit & JET_bitUpdateNoVersion) ) )
    {
        if ( pfcbTable->FUncommitted() )
        {
            Assert( ppib->Level() > 0 );
            fDIRFlags = fDIRNoVersion;
        }
        else
        {
            Error( ErrERRCheck( JET_errUpdateMustVersion ) );
        }
    }

    if ( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
    {
        FUCBSetUpdateForInsertCopyDeleteOriginal( pfucb );
        Call( ErrIsamDelete( ppib, pfucb ) );
    }

    Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeInsert, 0, NULL, NULL, 0 ) );

    Call( ErrDIROpen( ppib, pfcbTable, &pfucbT ) );
    Assert( pfucbT != pfucbNil );
    FUCBSetIndex( pfucbT );

    Assert( !pfucb->dataWorkBuf.FNull() );
    Call( ErrRECIIllegalNulls( pfucb ) );

    if ( grbit & JET_bitUpdateCheckESE97Compatibility )
    {
        Call( ErrRECICheckESE97Compatibility( pfucb, pfucb->dataWorkBuf ) );
    }

    Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
    fUpdatingLatchSet = fTrue;

    pfcbTable->AssertDML();

    fidVersion = ptdb->FidVersion();
    if ( fidVersion != 0 && !( FFUCBColumnSet( pfucb, fidVersion ) ) )
    {
        TDB *           ptdbT           = ptdb;
        const BOOL      fTemplateColumn = ptdbT->FFixedTemplateColumn( fidVersion );
        const COLUMNID  columnidT       = ColumnidOfFid( fidVersion, fTemplateColumn );
        ULONG           ul              = 0;
        DATA            dataField;

        if ( fTemplateColumn )
        {
            Assert( FCOLUMNIDTemplateColumn( columnidT ) );
            if ( !pfcbTable->FTemplateTable() )
            {
                ptdbT->AssertValidDerivedTable();
                ptdbT = ptdbT->PfcbTemplateTable()->Ptdb();
            }
            else
            {
                ptdbT->AssertValidTemplateTable();
            }
        }
        else
        {
            Assert( !FCOLUMNIDTemplateColumn( columnidT ) );
            Assert( !pfcbTable->FTemplateTable() );
        }

        dataField.SetPv( (BYTE *)&ul );
        dataField.SetCb( sizeof(ul) );
        err = ErrRECISetFixedColumn(
                    pfucb,
                    ptdbT,
                    columnidT,
                    &dataField );
        if ( err < 0 )
        {
            pfcbTable->LeaveDML();
            goto HandleError;
        }
    }

    pfcbTable->AssertDML();

#ifdef DEBUG
    if ( ptdb->FidAutoincrement() != 0 )
    {
        const BOOL      fTemplateColumn = ptdb->FFixedTemplateColumn( ptdb->FidAutoincrement() );
        const COLUMNID  columnidT       = ColumnidOfFid( ptdb->FidAutoincrement(), fTemplateColumn );
        DATA            dataT;

        Assert( FFUCBColumnSet( pfucb, FidOfColumnid( columnidT ) ) || FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) );

        CallS( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdb,
                columnidT,
                pfucb->dataWorkBuf,
                &dataT ) );

        Assert( !( pfcbTable->FTypeSort()
                || pfcbTable->FTypeTemporaryTable() ) );

        Assert( ptdb->QwAutoincrement() > 0 );
        if ( ptdb->F8BytesAutoInc() )
        {
            Assert( dataT.Cb() == sizeof(QWORD) );
            Assert( *(UnalignedLittleEndian< QWORD > *)dataT.Pv() <= ptdb->QwAutoincrement() );
        }
        else
        {
            Assert( dataT.Cb() == sizeof(ULONG) );
            Assert( *(UnalignedLittleEndian< ULONG > *)dataT.Pv() <= (ULONG)ptdb->QwAutoincrement() );
        }
    }
#endif

    pfcbTable->LeaveDML();

    Alloc( pbKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyToAdd.prefix.Nullify();
    keyToAdd.suffix.SetPv( pbKey );

    if ( pidbNil == pfcbTable->Pidb() )
    {
        DBK dbk;


        if ( ptdb->DbkMost() == 0 )
        {
            Call( ErrRECIInitDbkMost( pfucbT ) );
        }

        Call( ptdb->ErrGetAndIncrDbkMost( &dbk ) );
        Assert( dbk > 0 );

        keyToAdd.suffix.SetCb( sizeof(DBK) );
        Assert( pbKey == keyToAdd.suffix.Pv() );
        KeyFromLong( pbKey, dbk );
    }

    else
    {
        Assert( !pfcbTable->Pidb()->FMultivalued() );
        Assert( !pfcbTable->Pidb()->FTuples() );
        Call( ErrRECRetrieveKeyFromCopyBuffer(
            pfucb,
            pfcbTable->Pidb(),
            &keyToAdd,
            rgitagBaseKey,
            0,
            prceNil,
            &iidxsegT ) );

        CallS( ErrRECValidIndexKeyWarning( err ) );
        Assert( wrnFLDNotPresentInIndex != err );
        Assert( wrnFLDOutOfKeys != err );
        Assert( wrnFLDOutOfTuples != err );

        if ( pfcbTable->Pidb()->FNoNullSeg()
            && ( wrnFLDNullKey == err || wrnFLDNullFirstSeg == err || wrnFLDNullSeg == err ) )
        {
            Error( ErrERRCheck( JET_errNullKeyDisallowed ) );
        }
    }

    if ( pv != NULL && (ULONG)keyToAdd.Cb() > cbMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    DIRGotoRoot( pfucbT );

    err = ErrDIRInsert( pfucbT, keyToAdd, pfucb->dataWorkBuf, fDIRFlags );

    if ( err != JET_errKeyDuplicate && ( 0 != ( fDIRFlags & fDIRNoVersion ) ) )
    {
        fNoVersionUpdate = fTrue;
    }

    PERFOpt( PERFIncCounterTable( cRECInserts, PinstFromPfucb( pfucbT ), pfcbTable->TCE() ) );
    OSTraceFMP(
        pfcbTable->Ifmp(),
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] inserted record into objid=[0x%x:0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfcbTable->Ifmp(),
            pfcbTable->ObjidFDP(),
            err,
            err ) );

    Call( err );

    Call( ErrFaultInjection( 55119 ) );

    AssertDIRNoLatch( ppib );
    Assert( !pfucbT->bmCurr.key.FNull() && pfucbT->bmCurr.key.Cb() == keyToAdd.Cb() );
    Assert( pfucbT->bmCurr.data.Cb() == 0 );

    if ( pcbActual != NULL || pv != NULL )
    {
        BOOKMARK    *pbmPrimary;

        CallS( ErrDIRGetBookmark( pfucbT, &pbmPrimary ) );

        if ( pcbActual != NULL )
        {
            Assert( pbmPrimary->key.Cb() == keyToAdd.Cb() );
            *pcbActual = pbmPrimary->key.Cb();
        }

        if ( pv != NULL )
        {
            Assert( cbMax >= (ULONG)pbmPrimary->key.Cb() );
            pbmPrimary->key.CopyIntoBuffer( pv, min( cbMax, (ULONG)pbmPrimary->key.Cb() ) );
        }
    }

    DIB     dib;
    BOOL fInsertCopy;
    fInsertCopy = FFUCBInsertCopyPrepared( pfucb );
    dib.pbm     = &pfucbT->bmCurr;
    dib.dirflag = fDIRFlags;
    for ( pfcbIdx = pfcbTable->PfcbNextIndex();
        pfcbIdx != pfcbNil;
        pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        if ( FFILEIPotentialIndex( pfcbTable, pfcbIdx ) )
        {
            const IDB * const   pidb        = pfcbIdx->Pidb();
            BOOL                fUpdate     = fTrue;

            if ( !fInsertCopy )
            {
                if ( pidb->FSparseConditionalIndex() )
                    fUpdate = FRECIPossiblyUpdateSparseConditionalIndex( pidb, pfucb );

                if ( fUpdate && pidb->FSparseIndex() )
                    fUpdate = FRECIPossiblyUpdateSparseIndex( pidb, pfucb );
            }

            if ( fUpdate )
            {
                Call( ErrRECIUpdateIndex( pfucb, pfcbIdx, recoperInsert, &dib ) );
            }
        }
    }

    Assert( pfcbTable != pfcbNil );
    pfcbTable->ResetUpdating();
    fUpdatingLatchSet = fFalse;

    DIRClose( pfucbT );
    pfucbT = pfucbNil;

    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    FUCBResetUpdateFlags( pfucb );

    CallS( ErrRECCallback( ppib, pfucb, JET_cbtypAfterInsert, 0, NULL, NULL, 0 ) );

    AssertDIRNoLatch( ppib );

    RESKEY.Free( pbKey );

    return err;

HandleError:
    Assert( err < 0 );

    RESKEY.Free( pbKey );

    if ( fUpdatingLatchSet )
    {
        Assert( pfcbTable != pfcbNil );
        pfcbTable->ResetUpdating();
    }

    if ( pfucbNil != pfucbT )
    {
        DIRClose( pfucbT );
    }

    
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    if ( fNoVersionUpdate )
    {
        Assert( ppib->Level() > 0 );
        Assert( pfcbTable->FUncommitted() );

        FILETableMustRollback( ppib, pfcbTable );

        ppib->SetMustRollbackToLevel0();
    }

    AssertDIRNoLatch( ppib );
    return err;
}


ERR ErrRECInsert( FUCB *pfucb, BOOKMARK * const pbmPrimary )
{
    ERR     err;
    PIB     *ppib = pfucb->ppib;

    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    Assert( pfucb->pfucbCurIndex == pfucbNil );
    Assert( !FFUCBSecondary( pfucb ) );

    if ( NULL != pbmPrimary )
    {
        ULONG cb = pbmPrimary->key.suffix.Cb();
        err = ErrRECIInsert(
                    pfucb,
                    pbmPrimary->key.suffix.Pv(),
                    cbKeyAlloc,
                    &cb,
                    NO_GRBIT );
        pbmPrimary->key.suffix.SetCb( cb );
        Assert( pbmPrimary->key.suffix.Cb() <= cbKeyMostMost );
    }
    else
    {
        err = ErrRECIInsert( pfucb, NULL, 0, NULL, NO_GRBIT );
    }

    Assert( JET_errNoCurrentRecord != err );

    AssertDIRNoLatch( ppib );
    return err;
}

struct INSERT_INDEX_ENTRY_CONTEXT : INDEX_ENTRY_CALLBACK_CONTEXT
{
    DIRFLAG m_dirflag;

    INSERT_INDEX_ENTRY_CONTEXT( RECOPER Recoper, DIRFLAG Dirflag )
        : INDEX_ENTRY_CALLBACK_CONTEXT( Recoper )
    {
        m_dirflag = Dirflag;
    }
};

ERR ErrRECIInsertIndexEntry(
    FUCB *                       pfucbIdx,
    KEY&                         keyToInsert,
    const KEY&                   keyPrimary,
    RCE *                        prcePrimary,
    const BOOL                   fMayHaveAlreadyBeenInserted,
    BOOL *                       pfIndexEntryInserted,
    INDEX_ENTRY_CALLBACK_CONTEXT *pContext )
{
    ERR         err;
    const FCB * const pfcbIdx = pfucbIdx->u.pfcb;
    const IDB * const pidb = pfcbIdx->Pidb();
    INSERT_INDEX_ENTRY_CONTEXT *pInsertContext = reinterpret_cast<INSERT_INDEX_ENTRY_CONTEXT *>(pContext);

    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( keyPrimary.prefix.FNull() );
    Assert( keyPrimary.Cb() > 0 );

    Assert( NULL != pContext );
    Assert( NULL != pInsertContext );

    PERFOpt( PERFIncCounterTable( cRECIndexInserts, PinstFromPfucb( pfucbIdx ), TceFromFUCB( pfucbIdx ) ) );

    err = ErrDIRInsert(
        pfucbIdx,
        keyToInsert,
        keyPrimary.suffix,
        pInsertContext->m_dirflag,
        prcePrimary );
    switch ( err )
    {
        case JET_errSuccess:
            break;

        case JET_errMultiValuedIndexViolation:
            if ( fMayHaveAlreadyBeenInserted )
            {
                err = JET_errSuccess;
                break;
            }

            RECIReportIndexCorruption( pfcbIdx );
            AssertSz( fFalse, "JET_errSecondaryIndexCorrupted during an insert" );
            err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
            break;

        default:
            break;
    }

    OSTraceFMP(
        pfcbIdx->Ifmp(),
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] %s %s entry for %s into objid=[0x%x:0x%x] with error %d (0x%x)",
            pfucbIdx->ppib,
            ( ( ppibNil != pfucbIdx->ppib ) ? pfucbIdx->ppib->trxBegin0 : trxMax ),
            ( ( err >= JET_errSuccess ) ? "inserted" : "skipped insertion of" ),
            ( pidb->FTuples() ? "tuple index" : "index" ),
            ( SzRecoper( pInsertContext->m_recoper ) ),
            (ULONG)pfcbIdx->Ifmp(),
            pfcbIdx->ObjidFDP(),
            err,
            err ) );

    Call( err );
    *pfIndexEntryInserted = fTrue;

HandleError:
    return err;
}

ERR ErrRECIAddToIndex(
    FUCB        *pfucb,
    FUCB        *pfucbIdx,
    const BOOKMARK  *pbmPrimary,
    DIRFLAG     dirflag,
    RCE         *prcePrimary )
{
    ERR         err;
    const FCB   * const pfcbIdx         = pfucbIdx->u.pfcb;
    const IDB   * const pidb            = pfcbIdx->Pidb();
    BOOL        fIndexUpdated           = fFalse;

    AssertDIRNoLatch( pfucb->ppib );

    Assert( pfcbIdx != pfcbNil );
    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( pidbNil != pidb );

    Assert( !( dirflag & fDIRNoVersion ) || pfucb->u.pfcb->FUncommitted() );

    INSERT_INDEX_ENTRY_CONTEXT Context( recoperInsert, dirflag );

    Call ( ErrRECIEnumerateKeys(
               pfucb,
               pfucbIdx,
               pbmPrimary,
               prcePrimary,
               keyLocationCopyBuffer,
               fTrue,
               &fIndexUpdated,
               &Context,
               ErrRECIInsertIndexEntry ) );

    if ( fIndexUpdated )
    {
        err = ErrERRCheck( wrnFLDIndexUpdated );
    }
    else
    {
        err = JET_errSuccess;
    }

HandleError:
    return err;
}


ERR VTAPI ErrIsamDelete(
    JET_SESID   sesid,
    JET_VTID    vtid )
{
    ERR         err;
    PIB         * ppib              = reinterpret_cast<PIB *>( sesid );
    FUCB        * pfucb             = reinterpret_cast<FUCB *>( vtid );
    FCB         * pfcbTable;
    FCB         * pfcbIdx;
    BOOL        fUpdatingLatchSet   = fFalse;

    CallR( ErrPIBCheck( ppib ) );

    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    AssertDIRNoLatch( ppib );

    CallR( ErrFUCBCheckUpdatable( pfucb )  );
    if ( !FFMPIsTempDB( pfucb->ifmp ) )
    {
        CallR( ErrPIBCheckUpdatable( ppib ) );
    }

    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

#ifdef PREREAD_INDEXES_ON_DELETE
    if( pfcbTable->FPreread() )
    {
        BTPrereadIndexesOfFCB( pfucb );
    }
#endif

    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        if ( FFUCBUpdatePrepared( pfucb ) )
        {
            if ( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
            {
                return ErrERRCheck( JET_errAlreadyPrepared );
            }
            else
            {
                CallR( ErrIsamPrepareUpdate( ppib, pfucb, JET_prepCancel ) );
            }
        }
        CallR( ErrDIRBeginTransaction( ppib, 45861, NO_GRBIT ) );
    }

    Assert( !FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
    Assert( ppib->Level() > 0 );

#ifdef DEBUG
    const BOOL  fLogIsDone = !PinstFromPpib( ppib )->m_plog->FLogDisabled()
                            && !PinstFromPpib( ppib )->m_plog->FRecovering()
                            && g_rgfmp[pfucb->ifmp].FLogOn();
#endif

    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeDelete, 0, NULL, NULL, 0 ) );

    Call( ErrDIRGetLock( pfucb, writeLock ) );

    Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
    fUpdatingLatchSet = fTrue;

    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );
    Assert( ppib->Level() < levelMax );

    pfcbTable->LeaveDML();
    for( pfcbIdx = pfcbTable->PfcbNextIndex();
        pfcbIdx != pfcbNil;
        pfcbIdx = pfcbIdx->PfcbNextIndex() )
    {
        if ( FFILEIPotentialIndex( pfcbTable, pfcbIdx ) )
        {
            Call( ErrRECIUpdateIndex( pfucb, pfcbIdx, recoperDelete ) );
        }
    }

    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        Call( ErrRECDereferenceLongFieldsInRecord( pfucb ) );
    }

    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );

    err = ErrDIRDelete( pfucb, fDIRNull );
    AssertDIRNoLatch( ppib );

    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );

    pfcbTable->ResetUpdating();
    fUpdatingLatchSet = fFalse;

    PERFOpt( PERFIncCounterTable( cRECDeletes, PinstFromPfucb( pfucb ), pfcbTable->TCE() ) );
    OSTraceFMP(
        pfcbTable->Ifmp(),
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] deleted record from objid=[0x%x:0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfcbTable->Ifmp(),
            pfcbTable->ObjidFDP(),
            err,
            err ) );

    Call( err );

    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( ppib );

    CallS( ErrRECCallback( ppib, pfucb, JET_cbtypAfterDelete, 0, NULL, NULL, 0 ) );

    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );

    return err;


HandleError:
    Assert( err < 0 );

    AssertDIRNoLatch( ppib );
    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );

    if ( fUpdatingLatchSet )
    {
        Assert( pfcbTable != pfcbNil );
        pfcbTable->ResetUpdating();
    }

    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}

struct DELETE_INDEX_ENTRY_CONTEXT : INDEX_ENTRY_CALLBACK_CONTEXT
{
    DELETE_INDEX_ENTRY_CONTEXT( RECOPER Recoper )
        : INDEX_ENTRY_CALLBACK_CONTEXT( Recoper )
    {
    }
};

ERR ErrRECIDeleteIndexEntry(
    FUCB *                       pfucbIdx,
    KEY&                         keyToDelete,
    const KEY&                   keyPrimary,
    RCE *                        prcePrimary,
    const BOOL                   fMayHaveAlreadyBeenDeleted,
    BOOL *                       pfIndexEntryDeleted,
    INDEX_ENTRY_CALLBACK_CONTEXT *pContext )
{
    ERR         err;
    const FCB * const pfcbIdx = pfucbIdx->u.pfcb;
    const IDB * const pidb = pfcbIdx->Pidb();
    DIB         dib;
    BOOKMARK    bmSeek;
    BOOL        fThisIndexEntryDeleted = fFalse;
    DELETE_INDEX_ENTRY_CONTEXT *pDeleteContext = reinterpret_cast<DELETE_INDEX_ENTRY_CONTEXT *>(pContext);

    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( keyPrimary.prefix.FNull() );
    Assert( keyPrimary.Cb() > 0 );

    Assert( NULL != pContext );
    Assert( NULL != pDeleteContext );

    PERFOpt( PERFIncCounterTable( cRECIndexDeletes, PinstFromPfucb( pfucbIdx ), TceFromFUCB( pfucbIdx ) ) );

    bmSeek.Nullify();
    bmSeek.key = keyToDelete;

    if( !FFUCBUnique( pfucbIdx ) )
    {
        bmSeek.data = keyPrimary.suffix;
    }

    dib.pos     = posDown;
    dib.dirflag = fDIRExact;
    dib.pbm     = &bmSeek;

    err = ErrDIRDown( pfucbIdx, &dib );
    switch ( err )
    {
        case JET_errRecordNotFound:
            if ( fMayHaveAlreadyBeenDeleted )
            {
                err = JET_errSuccess;
                break;
            }
            __fallthrough;

        case wrnNDFoundLess:
        case wrnNDFoundGreater:
            RECIReportIndexCorruption( pfcbIdx );
            AssertSz( fFalse, "JET_errSecondaryIndexCorrupted during a delete" );
            err = ErrERRCheck( JET_errSecondaryIndexCorrupted );
            break;

        default:
            CallR( err );
            CallS( err );

            CallR( ErrDIRRelease( pfucbIdx ) );
            CallR( ErrDIRDelete( pfucbIdx, fDIRNull, prcePrimary ) );
            fThisIndexEntryDeleted = fTrue;
            *pfIndexEntryDeleted = fTrue;
            break;
    }

    OSTraceFMP(
        pfcbIdx->Ifmp(),
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] %s %s entry for %s from objid=[0x%x:0x%x] with error %d (0x%x)",
            pfucbIdx->ppib,
            ( ppibNil != pfucbIdx->ppib ? pfucbIdx->ppib->trxBegin0 : trxMax ),
            ( fThisIndexEntryDeleted ? "deleted" : "skipped deletion of" ),
            ( pidb->FTuples() ? "tuple index" : "index" ),
            ( SzRecoper( pDeleteContext->m_recoper ) ),
            (ULONG)pfcbIdx->Ifmp(),
            pfcbIdx->ObjidFDP(),
            err,
            err ) );

    return err;
}


ERR ErrRECIDeleteFromIndex(
    FUCB            *pfucb,
    FUCB            *pfucbIdx,
    BOOKMARK        *pbmPrimary,
    RCE             *prcePrimary )
{
    ERR             err;
    const FCB       * const pfcbIdx    = pfucbIdx->u.pfcb;
    const IDB       * const pidb       = pfcbIdx->Pidb();
    const BOOL      fDeleteByProxy     = ( prcePrimary != prceNil );
    BOOL            fIndexUpdated      = fFalse;

    AssertDIRNoLatch( pfucb->ppib );

    Assert( pfcbIdx != pfcbNil );
    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( pidbNil != pidb );

    DELETE_INDEX_ENTRY_CONTEXT Context( recoperDelete );
    Call( ErrRECIEnumerateKeys(
               pfucb,
               pfucbIdx,
               pbmPrimary,
               prcePrimary,
               ( fDeleteByProxy ? keyLocationCopyBuffer : keyLocationRecord ),
               fFalse,
               &fIndexUpdated,
               &Context,
               ErrRECIDeleteIndexEntry ) );

    if ( fIndexUpdated )
    {
        err = ErrERRCheck( wrnFLDIndexUpdated );
    }
    else
    {
        err = JET_errSuccess;
    }

HandleError:
    return err;
}


LOCAL BOOL FRECIIndexPossiblyChanged(
    const BYTE * const      rgbitIdx,
    const BYTE * const      rgbitSet )
{
    const LONG_PTR *        plIdx       = (LONG_PTR *)rgbitIdx;
    const LONG_PTR * const  plIdxMax    = plIdx + ( cbRgbitIndex / sizeof(LONG_PTR) );
    const LONG_PTR *        plSet       = (LONG_PTR *)rgbitSet;

    for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
    {
        if ( *plIdx & *plSet )
        {
            return fTrue;
        }
    }

    return fFalse;
}


LOCAL ERR ErrRECFIndexChanged( FUCB *pfucb, FCB *pfcbIdx, BOOL *pfChanged )
{
    KEY     keyOld;
    KEY     keyNew;
    BYTE    *pbOldKey                       = NULL;
    BYTE    *pbNewKey                       = NULL;
    DATA    *plineNewData = &pfucb->dataWorkBuf;
    ERR     err;
    ULONG   iidxsegT;
    BOOL    fCopyBufferKeyIsPresentInIndex  = fFalse;
    BOOL    fRecordKeyIsPresentInIndex      = fFalse;

    Assert( pfucb );
    Assert( !Pcsr( pfucb )->FLatched( ) );
    Assert( pfcbNil != pfucb->u.pfcb );
    Assert( pfcbNil != pfcbIdx );
    Assert( pfucb->dataWorkBuf.Cb() == plineNewData->Cb() );
    Assert( pfucb->dataWorkBuf.Pv() == plineNewData->Pv() );

    Assert( !pfcbIdx->Pidb()->FTuples() );

    Alloc( pbNewKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyNew.prefix.Nullify();
    keyNew.suffix.SetCb( cbKeyAlloc );
    keyNew.suffix.SetPv( pbNewKey );
    CallR( ErrRECRetrieveKeyFromCopyBuffer(
        pfucb,
        pfcbIdx->Pidb(),
        &keyNew,
        rgitagBaseKey,
        0,
        prceNil,
        &iidxsegT ) );
    CallS( ErrRECValidIndexKeyWarning( err ) );
    Assert( wrnFLDOutOfKeys != err );
    Assert( wrnFLDOutOfTuples != err );

    fCopyBufferKeyIsPresentInIndex = ( wrnFLDNotPresentInIndex != err );


    Alloc( pbOldKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyOld.prefix.Nullify();
    keyOld.suffix.SetCb( cbKeyAlloc );
    keyOld.suffix.SetPv( pbOldKey );

    Call( ErrRECRetrieveKeyFromRecord( pfucb, pfcbIdx->Pidb(), &keyOld, rgitagBaseKey, 0, fTrue, &iidxsegT ) );

    CallS( ErrRECValidIndexKeyWarning( err ) );
    Assert( wrnFLDOutOfKeys != err );
    Assert( wrnFLDOutOfTuples != err );

    fRecordKeyIsPresentInIndex = ( wrnFLDNotPresentInIndex != err );

    if( fCopyBufferKeyIsPresentInIndex && !fRecordKeyIsPresentInIndex
        || !fCopyBufferKeyIsPresentInIndex && fRecordKeyIsPresentInIndex )
    {
        *pfChanged = fTrue;
        Assert( !Pcsr( pfucb )->FLatched() );
        err = JET_errSuccess;
        goto HandleError;
    }
    else if( !fCopyBufferKeyIsPresentInIndex && !fRecordKeyIsPresentInIndex )
    {
        *pfChanged = fFalse;
        Assert( !Pcsr( pfucb )->FLatched() );
        err = JET_errSuccess;
        goto HandleError;
    }

#ifdef DEBUG
    if ( pfcbIdx->Pidb()->FNoNullSeg() )
    {
        Assert( wrnFLDNullSeg != err );
        Assert( wrnFLDNullFirstSeg != err );
        Assert( wrnFLDNullKey != err );
    }
#endif

    *pfChanged = !FKeysEqual( keyOld, keyNew );
    err = JET_errSuccess;

HandleError:
    RESKEY.Free( pbNewKey );
    RESKEY.Free( pbOldKey );

    Assert( !Pcsr( pfucb )->FLatched() );
    return err;
}



ERR ErrRECUpgradeReplaceNoLock( FUCB *pfucb )
{
    ERR     err;

    Assert( FFUCBReplaceNoLockPrepared( pfucb ) );

    Assert( !FFUCBDeferredChecksum( pfucb )
        || pfucb->ppib->Level() > 0 );

    CallR( ErrDIRGetLock( pfucb, writeLock ) );

    CallR( ErrDIRGet( pfucb ));
    const BOOL  fWriteConflict = ( !FFUCBDeferredChecksum( pfucb ) && !FFUCBCheckChecksum( pfucb ) );
    CallS( ErrDIRRelease( pfucb ));

    if ( fWriteConflict )
    {
        OSTraceFMP(
            pfucb->ifmp,
            JET_tracetagDMLConflicts,
            OSFormat(
                "Write-conflict detected: Session=[0x%p:0x%x] upgrading replace-no-lock on objid=[0x%x:0x%x]",
                pfucb->ppib,
                ( ppibNil != pfucb->ppib ? pfucb->ppib->trxBegin0 : trxMax ),
                (ULONG)pfucb->ifmp,
                pfucb->u.pfcb->ObjidFDP() ) );
        err = ErrERRCheck( JET_errWriteConflict );
    }
    else
    {
        UpgradeReplaceNoLock( pfucb );
    }

    return err;
}


LOCAL ERR ErrRECIReplace( FUCB *pfucb, const JET_GRBIT grbit )
{
    ERR     err;
    PIB *   ppib                = pfucb->ppib;
    FCB *   pfcbTable;
    TDB *   ptdb;
    FCB *   pfcbIdx;
    FID     fidFixedLast;
    FID     fidVarLast;
    FID     fidVersion;
    BOOL    fUpdateIndex;
    BOOL    fUpdatingLatchSet   = fFalse;

    CheckPIB( ppib );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    Assert( FFUCBUpdatable( pfucb ) );
    Assert( FFUCBReplacePrepared( pfucb ) );

    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    ptdb = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdb );

    Assert( !pfucb->dataWorkBuf.FNull() );

    CallR( ErrDIRBeginTransaction( ppib, 62245, NO_GRBIT ) );

    if ( ( 0 != (grbit & JET_bitUpdateNoVersion) ) )
    {
        Error( ErrERRCheck( JET_errUpdateMustVersion ) );
    }

    if ( FFUCBReplaceNoLockPrepared( pfucb ) )
    {
        Call( ErrRECUpgradeReplaceNoLock( pfucb ) );
    }

    Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeReplace, 0, NULL, NULL, 0 ) );

    if ( !FFUCBAnyColumnSet( pfucb ) )
    {
        PERFOpt( PERFIncCounterTable( cRECNoOpReplaces, PinstFromPfucb( pfucb ), pfcbTable->TCE() ) );
        goto Done;
    }

    if ( grbit & JET_bitUpdateCheckESE97Compatibility )
    {
        Call( ErrRECICheckESE97Compatibility( pfucb, pfucb->dataWorkBuf ) );
    }

    Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
    fUpdatingLatchSet = fTrue;

    pfcbTable->AssertDML();
    fidFixedLast = ptdb->FidFixedLast();
    fidVarLast = ptdb->FidVarLast();

    fUpdateIndex = FRECIIndexPossiblyChanged( ptdb->RgbitAllIndex(), pfucb->rgbitSet );

    pfcbTable->LeaveDML();

    Assert( !Pcsr( pfucb )->FLatched() );
    if ( fUpdateIndex )
    {
        if ( pfcbTable->Pidb() != pidbNil )
        {
            BOOL    fIndexChanged;

            Call( ErrRECFIndexChanged( pfucb, pfcbTable, &fIndexChanged ) );
            Assert( !Pcsr( pfucb )->FLatched() );

            if ( fIndexChanged )
            {
                Error( ErrERRCheck( JET_errRecordPrimaryChanged ) );
            }
        }
    }

#ifdef DEBUG
    else
    {
        if ( pfcbTable->Pidb() != pidbNil )
        {
            BOOL    fIndexChanged;

            Call( ErrRECFIndexChanged( pfucb, pfcbTable, &fIndexChanged ) );
            Assert( !fIndexChanged );
        }
    }
#endif

    Assert( FFUCBIndex( pfucb ) );
    pfcbTable->EnterDML();
    fidVersion = ptdb->FidVersion();
    pfcbTable->LeaveDML();

    if ( fidVersion != 0 )
    {
        FCB             * pfcbT         = pfcbTable;
        const BOOL      fTemplateColumn = ptdb->FFixedTemplateColumn( fidVersion );
        const COLUMNID  columnidT       = ColumnidOfFid( fidVersion, fTemplateColumn );
        ULONG           ulT;
        DATA            dataField;

        Assert( !Pcsr( pfucb )->FLatched() );

        err =  ErrRECIAccessColumn( pfucb, columnidT );
        if ( err < 0 )
        {
            if ( JET_errColumnNotFound != err )
                goto HandleError;
        }

        Call( ErrDIRGet( pfucb ) );

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

        err = ErrRECIRetrieveFixedColumn(
                pfcbNil,
                pfcbT->Ptdb(),
                columnidT,
                pfucb->kdfCurr.data,
                &dataField );
        if ( err < 0 )
        {
            pfcbT->LeaveDML();
            CallS( ErrDIRRelease( pfucb ) );
            goto HandleError;
        }

        if ( dataField.Cb() == 0 )
        {
            ulT = 1;
        }
        else
        {
            Assert( dataField.Cb() == sizeof(ULONG) );
            ulT = *(UnalignedLittleEndian< ULONG > *)dataField.Pv();
            ulT++;
        }

        dataField.SetPv( (BYTE *)&ulT );
        dataField.SetCb( sizeof(ulT) );
        err = ErrRECISetFixedColumn( pfucb, pfcbT->Ptdb(), columnidT, &dataField );

        pfcbT->LeaveDML();

        CallS( ErrDIRRelease( pfucb ) );
        Call( err );
    }

    Assert( !Pcsr( pfucb )->FLatched( ) );

    if ( fUpdateIndex )
    {
#ifdef PREREAD_INDEXES_ON_REPLACE
        if( pfucb->u.pfcb->FPreread() )
        {
            const INT cSecondaryIndexesToPreread = 16;

            PGNO rgpgno[cSecondaryIndexesToPreread + 1];
            INT ipgno = 0;

            for ( pfcbIdx = pfcbTable->PfcbNextIndex();
                  pfcbIdx != pfcbNil && ipgno < cSecondaryIndexesToPreread;
                  pfcbIdx = pfcbIdx->PfcbNextIndex() )
            {
                if ( pfcbIdx->Pidb() != pidbNil
                     && FFILEIPotentialIndex( pfcbTable, pfcbIdx )
                     && FRECIIndexPossiblyChanged( pfcbIdx->Pidb()->RgbitIdx(), pfucb->rgbitSet ) )
                {
                    rgpgno[ipgno++] = pfcbIdx->PgnoFDP();
                }
            }
            rgpgno[ipgno] = pgnoNull;

            PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
            tcScope->nParentObjectClass = TceFromFUCB( pfucb );
            tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );

            BFPrereadPageList( pfucb->ifmp, rgpgno, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );
        }
#endif

        for ( pfcbIdx = pfcbTable->PfcbNextIndex();
            pfcbIdx != pfcbNil;
            pfcbIdx = pfcbIdx->PfcbNextIndex() )
        {
            if ( pfcbIdx->Pidb() != pidbNil )
            {
                if ( FFILEIPotentialIndex( pfcbTable, pfcbIdx ) )
                {
                    if ( FRECIIndexPossiblyChanged( pfcbIdx->Pidb()->RgbitIdx(), pfucb->rgbitSet ) )
                    {
                        Call( ErrRECIUpdateIndex( pfucb, pfcbIdx, recoperReplace ) );
                    }
#ifdef DEBUG
                    else if ( pfcbIdx->Pidb()->FTuples() )
                    {
                    }
                    else
                    {
                        BOOL    fIndexChanged;

                        Call( ErrRECFIndexChanged( pfucb, pfcbIdx, &fIndexChanged ) );
                        Assert( !fIndexChanged );
                    }
#endif
                }
            }
        }
    }

    err = ErrDIRReplace( pfucb, pfucb->dataWorkBuf, fDIRLogColumnDiffs );

    pfcbTable->ResetUpdating();
    fUpdatingLatchSet = fFalse;

    PERFOpt( PERFIncCounterTable( cRECReplaces, PinstFromPfucb( pfucb ), pfcbTable->TCE() ) );
    OSTraceFMP(
        pfcbTable->Ifmp(),
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] replaced record in objid=[0x%x:0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfcbTable->Ifmp(),
            pfcbTable->ObjidFDP(),
            err,
            err ) );

    Call( err );

Done:
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    FUCBResetUpdateFlags( pfucb );

    CallS( ErrRECCallback( ppib, pfucb, JET_cbtypAfterReplace, 0, NULL, NULL, 0 ) );

    AssertDIRNoLatch( ppib );

    return err;


HandleError:
    Assert( err < 0 );
    AssertDIRNoLatch( ppib );

    if ( fUpdatingLatchSet )
    {
        Assert( pfcbTable != pfcbNil );
        pfcbTable->ResetUpdating();
    }

    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    return err;
}

class TRACK_INDEX_ENTRY_DATA {
public:
    enum ACTION {
        eSkip = 0,
        eDelete = 1,
        eInsert = 2,
    };

private:
    KEY m_Key;
    ACTION m_Action;

public:

    TRACK_INDEX_ENTRY_DATA()
    {
        m_Key.Nullify();
        m_Action = eSkip;
    }

    ~TRACK_INDEX_ENTRY_DATA()
    {
        PVOID pbTemp = m_Key.suffix.Pv();

        Assert( eDelete == m_Action || eInsert == m_Action || eSkip == m_Action );

        if ( NULL != pbTemp )
        {
            delete [] pbTemp;
        }

        m_Key.Nullify();
        m_Action = eSkip;
    }

    ERR ErrSet(const KEY& Key, const ACTION Action)
    {
        Assert( eDelete == Action || eInsert == Action || eSkip == Action );

        m_Action = Action;
        m_Key.Nullify();

        BYTE *pbTemp = new BYTE[ Key.Cb() ];
        if (NULL == pbTemp)
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }

        Key.CopyIntoBuffer( pbTemp, Key.Cb() );

        m_Key.suffix.SetPv( pbTemp );
        m_Key.suffix.SetCb( Key.Cb() );

        return JET_errSuccess;
    }

    KEY Key() const
    {
        return m_Key;
    }

    ACTION Action()
    {
        return m_Action;
    }

    void AdjustForDuplicateKeys( TRACK_INDEX_ENTRY_DATA& next)
    {
        if (!FKeysEqual( this->m_Key, next.m_Key ))
        {
            return;
        }

#define STATE(X, Y)  ( (X << 4) | Y )
        Assert( ( 1 << 4 ) > next.m_Action );

        switch ( STATE(this->m_Action, next.m_Action ) )
        {
            case STATE( eDelete, eDelete ):
                this->m_Action = eSkip;
                break;

            case STATE( eInsert, eInsert):
            case STATE( eSkip, eInsert):
                next.m_Action = eSkip;
                break;

            case STATE( eDelete, eInsert ):
                this->m_Action = eSkip;
                next.m_Action = eSkip;
                break;

            case STATE( eSkip, eDelete ):
                break;

            case STATE( eSkip, eSkip ):
            case STATE( eDelete, eSkip ):
            case STATE( eInsert, eSkip ):
            case STATE( eInsert, eDelete ):
                AssertSz( fFalse, "Can't happen" );
                break;

            default:
                AssertSz( fFalse, "Bad value" );
                break;
        }
#undef STATE
    }

    static int Comp(
        const void *pvVal1,
        const void *pvVal2
        )
    {
        TRACK_INDEX_ENTRY_DATA *pVal1 = (TRACK_INDEX_ENTRY_DATA *)pvVal1;
        TRACK_INDEX_ENTRY_DATA *pVal2 = (TRACK_INDEX_ENTRY_DATA *)pvVal2;

        Assert( eDelete == pVal1->m_Action || eInsert == pVal1->m_Action || eSkip == pVal1->m_Action );
        Assert( eDelete == pVal2->m_Action || eInsert == pVal2->m_Action || eSkip == pVal2->m_Action );

        int comp;

        comp = CmpKey( pVal1->Key(), pVal2->Key());

        if (0 == comp )
        {
            static_assert( ( eSkip < eDelete ), "Required");
            static_assert( ( eDelete < eInsert ), "Required");
            comp = (pVal1->Action() - pVal2->Action());
        }

        return comp;
    }
};

struct TRACK_INDEX_ENTRY_CONTEXT : INDEX_ENTRY_CALLBACK_CONTEXT
{
    TRACK_INDEX_ENTRY_DATA::ACTION m_eCurrentAction;
    ULONG m_cDataUsed;
    ULONG m_cDataAlloced;
    TRACK_INDEX_ENTRY_DATA *m_pData;

    TRACK_INDEX_ENTRY_CONTEXT( RECOPER Recoper )
        : INDEX_ENTRY_CALLBACK_CONTEXT( Recoper )
    {
        m_eCurrentAction = TRACK_INDEX_ENTRY_DATA::eSkip;
        m_cDataUsed = 0;
        m_cDataAlloced = 0;
        m_pData = NULL;
    }

    ~TRACK_INDEX_ENTRY_CONTEXT()
    {
        if ( NULL != m_pData )
        {
            delete [] m_pData;
        }
    }

    ERR ErrReserveAdditionalData( ULONG cDataRequested = 1 )
    {
        ERR err = JET_errSuccess;
        ULONG cNeeded;
        ULONG cTotalRequested = m_cDataUsed + cDataRequested;

        if ( 256 >= cTotalRequested )
        {
            cNeeded = 256;
        }
        else
        {
            cNeeded = LNextPowerOf2( cTotalRequested );
            Assert( cNeeded >= cTotalRequested );
            Assert( ( 2 * cTotalRequested ) >= cNeeded );
        }
        
        if ( cNeeded > m_cDataAlloced )
        {
            TRACK_INDEX_ENTRY_DATA *pDataTemp;

            Alloc( pDataTemp = new TRACK_INDEX_ENTRY_DATA[ cNeeded ] );

            if ( NULL != m_pData )
            {
                memcpy(pDataTemp, m_pData, m_cDataAlloced * sizeof( TRACK_INDEX_ENTRY_DATA ) );
                delete [] m_pData;
            }
            m_cDataAlloced = cNeeded;
            m_pData = pDataTemp;
        }

    HandleError:
        return err;
    }

    VOID CleanActionList()
    {
        if (1 >= m_cDataUsed ) {
            return;
        }

        qsort( m_pData, m_cDataUsed, sizeof( m_pData[ 0 ] ), &(TRACK_INDEX_ENTRY_DATA::Comp) );

        for ( ULONG i = 1; i < m_cDataUsed; i++ )
        {
            m_pData[ i - 1 ].AdjustForDuplicateKeys( m_pData[ i ] );
        }
    }
};

ERR ErrRECITrackIndexEntry(
    FUCB *      pfucbIdx,
    KEY&        keyRead,
    const KEY&  keyPrimary,
    RCE *       prcePrimary,
    const BOOL  fMayHaveAlreadyBeenModified,
    BOOL *      pfIndexUpdated,
    INDEX_ENTRY_CALLBACK_CONTEXT *pContext )
{
    ERR                       err;
    TRACK_INDEX_ENTRY_CONTEXT *pTrackContext = reinterpret_cast<TRACK_INDEX_ENTRY_CONTEXT *>(pContext);

    Assert( NULL != pContext );
    Assert( NULL != pTrackContext );

    Assert( TRACK_INDEX_ENTRY_DATA::eInsert == pTrackContext->m_eCurrentAction ||
            TRACK_INDEX_ENTRY_DATA::eDelete == pTrackContext->m_eCurrentAction );

    Call( pTrackContext->ErrReserveAdditionalData() );
    Call( pTrackContext->m_pData[pTrackContext->m_cDataUsed].ErrSet( keyRead, pTrackContext->m_eCurrentAction ) );
    pTrackContext->m_cDataUsed += 1;

HandleError:
    return err;
}


ERR ErrRECIReplaceInIndex(
    FUCB            *pfucb,
    FUCB            *pfucbIdx,
    BOOKMARK        *pbmPrimary,
    RCE             *prcePrimary )
{
    ERR             err;
    const FCB       * const pfcbIdx         = pfucbIdx->u.pfcb;
    const IDB       * const pidb            = pfcbIdx->Pidb();
    const BOOL      fReplaceByProxy         = ( prcePrimary != prceNil );
    BOOL            fIndexUpdated           = fFalse;

    DELETE_INDEX_ENTRY_CONTEXT DeleteContext( recoperReplace );
    INSERT_INDEX_ENTRY_CONTEXT InsertContext( recoperReplace, fDIRBackToFather );
    TRACK_INDEX_ENTRY_CONTEXT TrackContext( recoperReplace );

    AssertDIRNoLatch( pfucb->ppib );

    Assert( pfcbIdx != pfcbNil );
    Assert( pfcbIdx->FTypeSecondaryIndex() );
    Assert( pidbNil != pidb );

    if ( pidb->FTuples() )
    {
        Call ( ErrRECIEnumerateKeys(
                   pfucb,
                   pfucbIdx,
                   pbmPrimary,
                   prcePrimary,
                   ( fReplaceByProxy ? keyLocationRCE : keyLocationRecord_RetrieveLVBeforeImg ),
                   fFalse,
                   &fIndexUpdated,
                   &DeleteContext,
                   ErrRECIDeleteIndexEntry ) );

        DIRGotoRoot( pfucbIdx );
        Call ( ErrRECIEnumerateKeys(
                   pfucb,
                   pfucbIdx,
                   pbmPrimary,
                   prcePrimary,
                   keyLocationCopyBuffer,
                   fTrue,
                   &fIndexUpdated,
                   &InsertContext,
                   ErrRECIInsertIndexEntry ) );
    }
    else {
        TrackContext.m_eCurrentAction = TRACK_INDEX_ENTRY_DATA::eDelete;
        Call( ErrRECIEnumerateKeys(
                  pfucb,
                  pfucbIdx,
                  pbmPrimary,
                  prcePrimary,
                  ( fReplaceByProxy ? keyLocationRCE : keyLocationRecord_RetrieveLVBeforeImg ),
                  fFalse,
                  &fIndexUpdated,
                  &TrackContext,
                  ErrRECITrackIndexEntry ) );
        Assert( fFalse == fIndexUpdated );

        TrackContext.m_eCurrentAction = TRACK_INDEX_ENTRY_DATA::eInsert;
        Call( ErrRECIEnumerateKeys(
                  pfucb,
                  pfucbIdx,
                  pbmPrimary,
                  prcePrimary,
                  keyLocationCopyBuffer,
                  fTrue,
                  &fIndexUpdated,
                  &TrackContext,
                  ErrRECITrackIndexEntry ) );
        Assert( fFalse == fIndexUpdated );

        if ( 0 != TrackContext.m_cDataUsed )
        {
            TrackContext.CleanActionList();

            for ( ULONG i = 0; i < TrackContext.m_cDataUsed; i++ )
            {
                switch (TrackContext.m_pData[i].Action())
                {
                    case TRACK_INDEX_ENTRY_DATA::eSkip:
                        continue;

                    case TRACK_INDEX_ENTRY_DATA::eDelete:
                        Call( ErrRECIDeleteIndexEntry(
                                  pfucbIdx,
                                  TrackContext.m_pData[i].Key(),
                                  pbmPrimary->key,
                                  prcePrimary,
                                  fFalse,
                                  &fIndexUpdated,
                                  &DeleteContext ) );
                        break;

                    case TRACK_INDEX_ENTRY_DATA::eInsert:
                        DIRGotoRoot( pfucbIdx );

                        Call( ErrRECIInsertIndexEntry(
                                  pfucbIdx,
                                  TrackContext.m_pData[i].Key(),
                                  pbmPrimary->key,
                                  prcePrimary,
                                  fFalse,
                                  &fIndexUpdated,
                                  &InsertContext ) );
                        break;

                    default:
                        AssertSz( fFalse, "Can't happen.");
                        break;
                }
            }
        }
    }

    if ( fIndexUpdated )
    {
        err = ErrERRCheck( wrnFLDIndexUpdated );
    }
    else
    {
        PERFOpt( PERFIncCounterTable( cRECFalseIndexColumnUpdates, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
        OSTraceFMP(
            pfcbIdx->Ifmp(),
            JET_tracetagDMLWrite,
            OSFormat(
                "Session=[0x%p:0x%x] triggered false index update on objid=[0x%x:0x%x]",
                pfucbIdx->ppib,
                ( ppibNil != pfucbIdx->ppib ? pfucbIdx->ppib->trxBegin0 : trxMax ),
                (ULONG)pfcbIdx->Ifmp(),
                pfcbIdx->ObjidFDP() ) );
        err = JET_errSuccess;
    }

HandleError:
    return err;
}

LOCAL ERR ErrRECIEscrowUpdate(
    PIB             *ppib,
    FUCB            *pfucb,
    const COLUMNID  columnid,
    const VOID      *pv,
    ULONG           cbMax,
    VOID            *pvOld,
    ULONG           cbOldMax,
    ULONG           *pcbOldActual,
    JET_GRBIT       grbit )
{
    ERR         err         = JET_errSuccess;
    FIELD       fieldFixed;

    CallR( ErrRECIAccessColumn( pfucb, columnid, &fieldFixed ) );

    if ( FFUCBUpdatePrepared( pfucb ) )
    {
        return ErrERRCheck( JET_errAlreadyPrepared );
    }

    if ( fieldFixed.cbMaxLen != cbMax )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    if ( pvOld && 0 != cbOldMax && fieldFixed.cbMaxLen > cbOldMax )
    {
        return ErrERRCheck( JET_errInvalidBufferSize );
    }

    if ( !FCOLUMNIDFixed( columnid ) )
    {
        return ErrERRCheck( JET_errInvalidOperation );
    }

    CallR( ErrRECSessionWriteConflict( pfucb ) );

    FCB *pfcb = pfucb->u.pfcb;

    if ( FCOLUMNIDTemplateColumn( columnid ) )
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

    if ( !FFIELDEscrowUpdate( fieldFixed.ffield ) )
    {
        return ErrERRCheck( JET_errInvalidOperation );
    }
    Assert( FCOLUMNIDFixed( columnid ) );

    CallR( ErrDIRBeginTransaction( ppib, 37669, NO_GRBIT ) );

#ifdef DEBUG
    err = ErrDIRGet( pfucb );
    if ( err >= 0 )
    {
        const REC * const prec = (REC *)pfucb->kdfCurr.data.Pv();
        Assert( prec->FidFixedLastInRec() >= FidOfColumnid( columnid ) );
        CallS( ErrDIRRelease( pfucb ) );
    }
    Assert( !Pcsr( pfucb )->FLatched() );
#endif


    DIRFLAG dirflag = fDIRNull;
    if ( JET_bitEscrowNoRollback & grbit )
    {
        dirflag |= fDIRNoVersion;
    }
    if( FFIELDFinalize( fieldFixed.ffield ) )
    {
        dirflag |= fDIREscrowCallbackOnZero;
    }
    if( FFIELDDeleteOnZero( fieldFixed.ffield ) )
    {
        dirflag |= fDIREscrowDeleteOnZero;
    }

    if ( fieldFixed.cbMaxLen == 4 )
    {
        LONG* plDelta = (LONG*)pv;
        LONG* plPrev = (LONG*)pvOld;
        err = ErrDIRDelta< LONG >(
            pfucb,
            fieldFixed.ibRecordOffset,
            *plDelta,
            plPrev,
            dirflag );
    }
    else if ( fieldFixed.cbMaxLen == 8 )
    {
        LONGLONG* pllDelta = (LONGLONG*) pv;
        LONGLONG* pllPrev = (LONGLONG*) pvOld;
        err = ErrDIRDelta< LONGLONG >(
            pfucb,
            fieldFixed.ibRecordOffset,
            *pllDelta,
            pllPrev,
            dirflag );
    }
    else
    {
        AssertSz( fFalse, "Tried escrow update operation on unsupported numerical type." );
        err = ErrERRCheck( JET_errInvalidOperation );
    }

    PERFOpt( PERFIncCounterTable( cRECEscrowUpdates, PinstFromPfucb( pfucb ), TceFromFUCB( pfucb ) ) );
    OSTraceFMP(
        pfcb->Ifmp(),
        JET_tracetagDMLWrite,
        OSFormat(
            "Session=[0x%p:0x%x] escrow-updated record in objid=[0x%x:0x%x] with error %d (0x%x)",
            ppib,
            ( ppibNil != ppib ? ppib->trxBegin0 : trxMax ),
            (ULONG)pfcb->Ifmp(),
            pfcb->ObjidFDP(),
            err,
            err ) );

    if ( err >= 0 )
    {
        if ( pcbOldActual )
        {
            *pcbOldActual = fieldFixed.cbMaxLen;
        }
        err = ErrDIRCommitTransaction( ppib, NO_GRBIT );
    }
    if ( err < 0 )
    {
        CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );
    }

    return err;
}

ERR VTAPI ErrIsamEscrowUpdate(
    JET_SESID       sesid,
    JET_VTID        vtid,
    JET_COLUMNID    columnid,
    VOID            *pv,
    ULONG           cbMax,
    VOID            *pvOld,
    ULONG           cbOldMax,
    ULONG           *pcbOldActual,
    JET_GRBIT       grbit )
{
    PIB * const ppib    = reinterpret_cast<PIB *>( sesid );
    FUCB * const pfucb  = reinterpret_cast<FUCB *>( vtid );

    ERR         err = JET_errSuccess;

    if( ppib->Level() <= 0 )
    {
        return ErrERRCheck( JET_errNotInTransaction );
    }

    CallR( ErrPIBCheck( ppib ) );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    CallR( ErrFUCBCheckUpdatable( pfucb )  );
    if ( !FFMPIsTempDB( pfucb->ifmp ) )
    {
        CallR( ErrPIBCheckUpdatable( ppib ) );
    }

    err = ErrRECIEscrowUpdate(
            ppib,
            pfucb,
            columnid,
            pv,
            cbMax,
            pvOld,
            cbOldMax,
            pcbOldActual,
            grbit );

    AssertDIRNoLatch( ppib );
    return err;
}
