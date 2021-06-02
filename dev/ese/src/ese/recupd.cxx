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

#endif // PERFMON_SUPPORT


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
                    pfcbIdx->PfcbTable() == pfcbNil :       // Index not linked in yet.
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
            Assert( pfcbIdx->PfcbTable() == pfcbNil );      // Index not linked in yet.
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

    // Set inner-most loop limit params.
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
        // Non-tuple degenerate case, 1 "tuple" starting at offset 0.
        ichOffsetStart = 0;
        ichIncrement = 1;
        ulTupleMax = 1;
    }

    // Read all the KeySequence values from one source.
    ULONG iidxseg = ULONG( -1 );
    for ( KeySequence ksRead( pfucb, pidb ); !ksRead.FSequenceComplete(); ksRead.Increment( iidxseg ) )
    {
        ULONG ichOffset;
        ULONG ulTuple;

        // Read all the required tuples from one KeySequence value.
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
                        // A NoNullSeg requirement violation.
                        if ( fErrorOnNullSegViolation )
                        {
                            Call( ErrERRCheck( JET_errNullKeyDisallowed ) );
                        }

                        Assert( !fNoNullSeg ); // No matter where the key is from, it should honor index no NULL segment requirements.
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
                    // Record was not in this index because of a conditional column.
                    // No need to check any more keys, since all are affected by the same conditional column.
                    Assert( 0 == ulTuple );
                    Assert( ksRead.FBaseKey() );
                    iidxseg = iidxsegComplete;
                    continue;

                case wrnFLDNullKey:
                    Assert( !fTuples ); // Tuples would have generated wrnFLDOutOfTuples.
                    Assert( ksRead.FBaseKey() ); //  NULLs beyond base key would have generated wrnFLDOutOfKeys

                    // Regardless, we're done reading with this KeySequence.
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

            //  Is it possible that the index entry may already have been deleted/inserted
            //  as a result of a previous tuple-value (from this or another value
            //  of a multi-valued column) generating the same index entry as the
            //  current tuple-value?
            //  We use this as an overly broad correction for DeleteKey failing with
            //  KeyNotPresent or InsertKey failing with KeyAlreadyPresent.
            //  We assume that the only reason to get these kinds of errors is that
            //  we've already inserted this/deleted key, but we can't be sure.
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

            // We've dealt with any error cases; reset to success as it's a loop control criterion.
            err = JET_errSuccess;
        }

        if ( iidxsegComplete == iidxseg) {
            continue;
        }

        if ( !fHasMultivalue )
        {
            //  If there are no multivalues in this index, there's no point going beyond base key.
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


//  ================================================================
LOCAL ERR ErrRECICallback(
        PIB * const ppib,
        FUCB * const pfucb,
        TDB * const ptdb,
        const JET_CBTYP cbtyp,
        const ULONG ulId,
        void * const pvArg1,
        void * const pvArg2,
        const ULONG ulUnused )
//  ================================================================
{
    //  optimization: do a quick sanity check to see if we have any callbacks
    //  of the appropriate type.
    //
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
        //  no matching callbacks were found
        //  if the pointer is non-null then it is pointing to
        //  the first matching CBDESC
        //
        return JET_errSuccess;
    }

    // concurrency testing. sleep here for a while to allow
    // time for the callback to be unregistered

    if(JET_errSuccess != ErrFaultInjection(36013))
    {
        UtilSleep( cmsecWaitGeneric );
    }

    //  if we reach this point, we have a matching callback
    //  if callback versioning is on, we'll check the visibility later

    ERR                     err             = JET_errSuccess;
    const JET_SESID         sesid           = (JET_SESID)ppib;
    const JET_TABLEID       tableid         = (JET_TABLEID)pfucb;
    const JET_DBID          ifmp            = (JET_DBID)pfucb->ifmp;
    const VTFNDEF * const   pvtfndefSaved   = pfucb->pvtfndef;

    pfucb->pvtfndef = &vtfndefIsamCallback;

    //  the first loop above, stopped us at the first possibly matching CBDESC
    //  we don't need to re-initialize the variable as we've done the work already
    //
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
                //  uncommitted register. only visible if we are the session that added it
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax == pcbdesc->trxRegisterCommit0 );
                Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
                fVisible = trxSession == pcbdesc->trxRegisterBegin0;
            }
            else if( trxMax == pcbdesc->trxUnregisterBegin0 )
            {
                //  committed register. visible if we began after the register committed
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax != pcbdesc->trxRegisterCommit0 );
                Assert( trxMax == pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
                fVisible = TrxCmp( trxSession, pcbdesc->trxRegisterCommit0 ) > 0;
            }
            else if( trxMax == pcbdesc->trxUnregisterCommit0 )
            {
                //  uncommitted unregister. visible unless we are the session that unregistered it
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax != pcbdesc->trxRegisterCommit0 );
                Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax == pcbdesc->trxUnregisterCommit0 );
                fVisible = trxSession != pcbdesc->trxUnregisterBegin0;
            }
            else
            {
                //  commited unregister. only visible if we began before the unregister committed
                Assert( trxMax != pcbdesc->trxRegisterBegin0 );
                Assert( trxMax != pcbdesc->trxRegisterCommit0 );
                Assert( trxMax != pcbdesc->trxUnregisterBegin0 );
                Assert( trxMax != pcbdesc->trxUnregisterCommit0 );
                fVisible = TrxCmp( trxSession, pcbdesc->trxUnregisterCommit0 ) <= 0;
            }
        }
#endif  //  VERSIONED_CALLBACKS

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


//  ================================================================
ERR ErrRECCallback(
        PIB * const ppib,
        FUCB * const pfucb,
        const JET_CBTYP cbtyp,
        const ULONG ulId,
        void * const pvArg1,
        void * const pvArg2,
        const ULONG ulUnused )
//  ================================================================
//
//  Call the specified callback type for the specified id
//
//-
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
///     pfcb->EnterDDL();
        err = ErrRECICallback( ppib, pfucb, ptdb, cbtyp, ulId, pvArg1, pvArg2, ulUnused );
///     pfcb->LeaveDDL();
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
        //  get bookmark of inserted node in pv
        //
        err = ErrRECIInsert( pfucb, pv, cbMax, pcbActual, grbit );
    }
    else
        err = ErrERRCheck( JET_errUpdateNotPrepared );

    //  free temp working buffer
    //
    if ( err >= 0 && !g_fRepair )       //  for g_fRepair we will cache these until we close the cursor
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
    ERR             err = JET_errSuccess;                   // error code of various utility
    FUCB            *pfucbIdx;                              //  cursor on secondary index

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

    //  open FUCB on this index
    //
    CallR( ErrDIROpen( pfucb->ppib, pfcbIdx, &pfucbIdx ) );
    Assert( pfucbIdx != pfucbNil );
    FUCBSetIndex( pfucbIdx );
    FUCBSetSecondary( pfucbIdx );

    //  get bookmark of primary index record replaced
    //
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

    //  close the FUCB
    //
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
        //  can't use IDB's rgbitIdx because
        //  we must filter out conditional index
        //  columns

        const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
        const TDB * const       ptdb        = pfcbTable->Ptdb();
        const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSeg( pidb, ptdb );
        const IDXSEG * const    pidxsegMac  = pidxseg + pidb->Cidxseg();

        Assert( pidxseg < pidxsegMac );
        for ( ; pidxseg < pidxsegMac; pidxseg++ )
        {
            if ( !FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
            {
                //  found a sparse index column that didn't get set
                return fFalse;
            }
        }
    }
    else
    {
        //  no conditional columns, so we can use
        //  the IDB bit array

        const LONG_PTR *        plIdx       = (LONG_PTR *)pidb->RgbitIdx();
        const LONG_PTR * const  plIdxMax    = plIdx + ( cbRgbitIndex / sizeof(LONG_PTR) );
        const LONG_PTR *        plSet       = (LONG_PTR *)pfucbTable->rgbitSet;

        for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
        {
            if ( (*plIdx & *plSet) != *plIdx )
            {
                //  found a sparse index column that didn't get set
                return fFalse;
            }
        }
    }

    //  all sparse index columns were set
    return fTrue;
}

LOCAL BOOL FRECIAnySparseIndexColumnSet(
    const IDB * const       pidb,
    const FUCB * const      pfucbTable )
{
    Assert( pidb->FSparseIndex() );

    if ( pidb->CidxsegConditional() > 0 )
    {
        //  can't use IDB's rgbitIdx because
        //  we must filter out conditional index
        //  columns

        const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
        const TDB * const       ptdb        = pfcbTable->Ptdb();
        const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSeg( pidb, ptdb );
        const IDXSEG * const    pidxsegMac  = pidxseg + pidb->Cidxseg();

        Assert( pidxseg < pidxsegMac );
        for ( ; pidxseg < pidxsegMac; pidxseg++ )
        {
            if ( FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
            {
                //  found a sparse index column that got set
                return fTrue;
            }
        }
    }

    else
    {
        //  no conditional columns, so we can use
        //  the IDB bit array

        const LONG_PTR *        plIdx       = (LONG_PTR *)pidb->RgbitIdx();
        const LONG_PTR * const  plIdxMax    = plIdx + ( cbRgbitIndex / sizeof(LONG_PTR) );
        const LONG_PTR *        plSet       = (LONG_PTR *)pfucbTable->rgbitSet;

        for ( ; plIdx < plIdxMax; plIdx++, plSet++ )
        {
            if ( *plIdx & *plSet )
            {
                //  found a sparse index column that got set
                return fTrue;
            }
        }
    }

    //  no sparse index columns were set
    return fFalse;
}


INLINE BOOL FRECIPossiblyUpdateSparseIndex(
    const IDB * const       pidb,
    const FUCB * const      pfucbTable )
{
    Assert( pidb->FSparseIndex() );

    if ( !pidb->FAllowSomeNulls() )
    {
        //  IgnoreAnyNull specified, so only need to
        //  update the index if all index columns
        //  were set
        return FRECIAllSparseIndexColumnsSet( pidb, pfucbTable );
    }
    else if ( !pidb->FAllowFirstNull() )
    {
        //  IgnoreFirstNull specified, so only need to
        //  update the index if the first column was set
        const FCB * const       pfcbTable   = pfucbTable->u.pfcb;
        const TDB * const       ptdb        = pfcbTable->Ptdb();
        const IDXSEG *          pidxseg     = PidxsegIDBGetIdxSeg( pidb, ptdb );
        return FFUCBColumnSet( pfucbTable, pidxseg->Fid() );
    }
    else
    {
        //  IgnoreNull specified, so need to update the
        //  index if any index column was set
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

    //  check conditional columns and see if we can
    //  automatically deduce whether the record should
    //  be added to the index, regardless of whether any
    //  of the actual index columns were set or not
    for ( ; pidxseg < pidxsegMac; pidxseg++ )
    {
        //  check that the update didn't modify the
        //  conditional column
        if ( !FFUCBColumnSet( pfucbTable, pidxseg->Fid() ) )
        {
            const FIELD * const pfield  = ptdb->Pfield( pidxseg->Columnid() );

            if ( !FFIELDUserDefinedDefault( pfield->ffield ) )
            {
                //  given that the update didn't modify the
                //  conditional column, see if the default
                //  value of the column would cause the
                //  record to be excluded from the index
                //  (NOTE: default values cannot be NULL)
                const BOOL  fHasDefault = FFIELDDefault( pfield->ffield );
                const BOOL  fSparse     = ( pidxseg->FMustBeNull() ?
                                                fHasDefault :
                                                !fHasDefault );
                if ( fSparse )
                {
                    //  this record will be excluded from
                    //  the index
                    return fFalse;
                }
            }
        }
    }

    //  could not exclude the record based on
    //  unset conditional columns
    return fTrue;
}


//  ================================================================
LOCAL BOOL FRECIHasUserDefinedColumns( const IDXSEG * const pidxseg, const INT cidxseg, const TDB * const ptdb )
//  ================================================================
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

//  ================================================================
LOCAL BOOL FRECIHasUserDefinedColumns( const IDB * const pidb, const TDB * const ptdb )
//  ================================================================
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

    //  only for secondary indexes
    Assert( pidbNil != pidb );
    Assert( !pidb->FPrimary() );

    //  pfcbTable may not be linked up if we're in CreateIndex(),
    //  in which case we don't have access to the information
    //  we want to report
    if ( pfcbNil == pfcbIdx->PfcbTable() )
        return;

    //  WARNING! WARNING!  This code currently does not grab the DML latch,
    //  so there doesn't appear to be any guarantee that the table and index
    //  name won't be relocated from underneath us

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

    //  fixed/variable column overhead hasn't changed
    //
    //  UNDONE: we don't currently account for fixed/variable columns
    //  that may have been deleted
    //
    cbRecESE97 = prec->PbTaggedData() - (BYTE *)prec;
    Assert( cbRecESE97 <= (SIZE_T)REC::CbRecordMostCHECK( g_rgfmp[ pfucb->ifmp ].CbPage() ) );
    Assert( cbRecESE97 <= (SIZE_T)REC::CbRecordMost( pfucb ) );
    Assert( cbRecESE97 <= (SIZE_T)dataRec.Cb() );

    //  OPTIMISATION: see if the record size is small enough such that
    //  we know we're guaranteed to fit in an ESE97 record
    //
    //  The only size difference between ESE97 and ESE98 is the amount
    //  of overhead that multi-values consume.  Fixed, variable, and
    //  non-multi-valued tagged columns still take the same amount of
    //  overhead.  Thus, for the greatest size difference, we need to
    //  ask how we could cram the most multi-values into a record.
    //  The answer is to have just a single multi-valued non-long-value
    //  tagged column where all multi-values contain zero-length data.
    //
    //  So what we're going to do first is take the amount of possible
    //  record space remaining and compute how many such ESE97 multi-
    //  values we could fill that space with.
    //
    const SIZE_T        cbESE97Remaining            = ( REC::CbRecordMost( g_rgfmp[ pfucb->ifmp ].CbPage(), JET_cbKeyMost_OLD ) - cbRecESE97 );
    const SIZE_T        cESE97MultiValues           = cbESE97Remaining / sizeof(TAGFLD);

    //  In ESE98, the marginal cost for a multi-value is 2 bytes, and the
    //  overhead for the initial column is 5 bytes.
    //
    const SIZE_T        cbESE98ColumnOverhead       = 5;
    const SIZE_T        cbESE98MultiValueOverhead   = 2;

    //  Next, see how big the tagged data would be if the same
    //  multi-values were represented in ESE98.  This will be
    //  our threshold record size.  If the current tagged data
    //  is less than this threshold, then we're guaranteed
    //  that this record will fit in ESE97 no matter what.
    //
    const SIZE_T        cbESE98Threshold            = cbESE98ColumnOverhead
                                                        + ( cESE97MultiValues * cbESE98MultiValueOverhead );
    const BOOL          fRecordWillFitInESE97       = ( ( dataRec.Cb() - cbRecESE97 ) <= cbESE98Threshold );

    if ( fRecordWillFitInESE97 )
    {
#ifdef DEBUG
        //  in DEBUG, take non-optimised path anyway to verify
        //  that record does indeed fit in ESE97
#else
        return JET_errSuccess;
#endif
    }
    else
    {
        //  this DOESN'T mean that the record won't fit, it
        //  just means it may or may not fit, we can't make
        //  an absolute determination just by the record
        //  size, so we have to iterate through all the
        //  tagged data
    }


    //  If the optimisation above couldn't definitively
    //  determine if the record will fit in ESE97, then
    //  we have to take the slow path of computing
    //  ESE97 record size column-by-column.
    //
    CTaggedColumnIter   citerTagged;
    Call( citerTagged.ErrInit( pfcbTable ) );   //  initialises currency to BeforeFirst
    Call( citerTagged.ErrSetRecord( dataRec ) );

    while ( errRECNoCurrentColumnValue != ( err = citerTagged.ErrMoveNext() ) )
    {
        COLUMNID    columnid;
        size_t      cbESE97Format;

        //  validate error returned from column navigation
        //
        Call( err );

        //  ignore columns that are not visible to us (we're assuming the
        //  column has been deleted and the column space would be able to
        //  be re-used
        //
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

    //  down to the last data record
    //
    dib.dirflag = fDIRAllNode;
    dib.pbm = NULL;
    dib.pos = posLast;
    err = ErrDIRDown( pfucb, &dib );
    Assert( JET_errNoCurrentRecord != err );
    if ( JET_errRecordNotFound == err )
    {
        //  table is empty
        //
        Assert( 0 == dbk );
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
        CallS( err );           //  warnings not expected

        Assert( Pcsr( pfucb )->FLatched() );
        LongFromKey( &dbk, pfucb->kdfCurr.key );
        Assert( dbk > 0 );      // dbk's start numbering at 1

        DIRUp( pfucb );
    }

    //  if there are no records in the table, then the first
    //  dbk value is 1, otherwise, set dbk to next value after
    //  maximum found
    //
    dbk++;

    //  while retrieving the dbkMost, someone else may have been
    //  doing the same thing and beaten us to it, so when this happens,
    //  cede to the other guy.
    //
    pfucb->u.pfcb->Ptdb()->InitDbkMost( dbk );

HandleError:
    Assert( !Pcsr( pfucb )->FLatched( ) );
    return err;
}


//+local
// ErrRECIInsert
// ========================================================================
// ErrRECIInsert( FUCB *pfucb, VOID *pv, ULONG cbMax, ULONG *pcbActual, DIRFLAG dirflag )
//
// Adds a record to a data file.  All indexes on the data file are
// updated to reflect the addition.
//
// PARAMETERS   pfucb                       FUCB for file
//              pv                          pointer to bookmark buffer pv != NULL, bookmark is returned
//              cbMax                       size of bookmark buffer
//              pcbActual                   returned size of bookmark
//
// RETURNS      Error code, one of the following:
//                   JET_errSuccess     Everything went OK.
//                  -KeyDuplicate       The record being added causes
//                                      an illegal duplicate entry in an index.
//                  -NullKeyDisallowed  A key of the new record is NULL.
//                  -RecordNoCopy       There is no working buffer to add from.
//                  -NullInvalid        The record being added contains
//                                      at least one null-valued field
//                                      which is defined as NotNull.
// SIDE EFFECTS
//      After addition, file currency is left on the new record.
//      Index currency (if any) is left on the new index entry.
//      On failure, the currencies are returned to their initial states.
//
//  COMMENTS
//      No currency is needed to add a record.
//      A transaction is wrapped around this function.  Thus, any
//      work done will be undone if a failure occurs.
//-
LOCAL ERR ErrRECIInsert(
    FUCB *          pfucb,
    _Out_writes_bytes_to_opt_(cbMax, *pcbActual) VOID *         pv,
    ULONG           cbMax,
    ULONG *         pcbActual,
    const JET_GRBIT grbit )
{
    ERR             err;                            // error code of various utility
    PIB *           ppib                    = pfucb->ppib;
    KEY             keyToAdd;                       // key of new data record
    BYTE            *pbKey                  = NULL;
    FCB *           pfcbTable;                      // file's FCB
    TDB *           ptdb;
    FCB *           pfcbIdx;                        // loop variable for each index on file
    FUCB *          pfucbT                  = pfucbNil;
    BOOL            fUpdatingLatchSet       = fFalse;
    ULONG           iidxsegT;

    //  if table itself created in same transaction then allow application
    //  to update table without creating separate versions for each update.
    //  It is expected that any error returned from any update operation
    //  to lead to the table creation being rolled back.
    //
    DIRFLAG fDIRFlags = fDIRNull;
    BOOL    fNoVersionUpdate = fFalse;

    FID     fidVersion;

    CheckPIB( ppib );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    //  should have been checked in PrepareUpdate
    //
    Assert( FFUCBUpdatable( pfucb ) );
    Assert( FFUCBInsertPrepared( pfucb ) );

    //  efficiency variables
    //
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    ptdb = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdb );

    //  if necessary, begin transaction
    //
    CallR( ErrDIRBeginTransaction( ppib, 52005, NO_GRBIT ) );

    //  insert unversioned if requested, and if table uncommitted.
    //  Any error should result in client application rolling backt transaction
    //  which created table, since table may be left inconsistent.
    //
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

    //  delete the original copy if necessary
    //
    if ( FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) )
    {
        FUCBSetUpdateForInsertCopyDeleteOriginal( pfucb );
        Call( ErrIsamDelete( ppib, pfucb ) );
    }

    // Do the BeforeInsert callback
    Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeInsert, 0, NULL, NULL, 0 ) );

    //  open temp FUCB on data file
    //
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

    //  set version and autoinc columns
    //
    pfcbTable->AssertDML();

    fidVersion = ptdb->FidVersion();        // UNDONE: Need to properly version these.
    if ( fidVersion != 0 && !( FFUCBColumnSet( pfucb, fidVersion ) ) )
    {
        //  set version column to zero
        //
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
                // Switch to template table.
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

        //  AutoInc column id not set in JET_prepInsertCopyReplaceOriginal.  
        //  FFUCBUpdateForInsertCopyDeleteOriginalPrepared is set both for this grbit and also for JET_prepInsertCopyReplaceOriginal.
        //
        Assert( FFUCBColumnSet( pfucb, FidOfColumnid( columnidT ) ) || FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) );

        //  just retrieve column, even if we don't have versioned access to it
        //
        CallS( ErrRECIRetrieveFixedColumn(
                pfcbNil,
                ptdb,
                columnidT,
                pfucb->dataWorkBuf,
                &dataT ) );

        Assert( !( pfcbTable->FTypeSort()
                || pfcbTable->FTypeTemporaryTable() ) );    // Don't currently support autoinc with sorts/temp. tables

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

    //  get key to add with new record
    //
    Alloc( pbKey = (BYTE *)RESKEY.PvRESAlloc() );
    keyToAdd.prefix.Nullify();
    keyToAdd.suffix.SetPv( pbKey );

    if ( pidbNil == pfcbTable->Pidb() )
    {
        DBK dbk;

        //  file is sequential
        //

        //  dbk's are numbered starting at 1.  A dbk of 0 indicates that we must
        //  first retrieve the dbkMost.  In the pathological case where there are
        //  currently no dbk's, we'll go through here anyway, but only the first
        //  time (since there will be dbk's after that).
        //
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
        //  file is primary
        //
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

    //  check if return buffer for bookmark is sufficient size
    //
    if ( pv != NULL && (ULONG)keyToAdd.Cb() > cbMax )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    //  insert record.  Move to DATA root.
    //
    DIRGotoRoot( pfucbT );

    err = ErrDIRInsert( pfucbT, keyToAdd, pfucb->dataWorkBuf, fDIRFlags );

    //  if have gotten this far then set fNoVersionUpdate flag so subsequent error
    //  will require table to be abandoned.  Key duplicate is a permitted error
    //  since it means that no changes have been made to the index.
    //
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

    //  process result of insertion
    //
    Call( err );

    //  test error handling
    //
    Call( ErrFaultInjection( 55119 ) );

    //  return bookmark of inserted record
    //
    AssertDIRNoLatch( ppib );
    Assert( !pfucbT->bmCurr.key.FNull() && pfucbT->bmCurr.key.Cb() == keyToAdd.Cb() );
    Assert( pfucbT->bmCurr.data.Cb() == 0 );

    if ( pcbActual != NULL || pv != NULL )
    {
        BOOKMARK    *pbmPrimary;    //  bookmark of primary index node inserted

        CallS( ErrDIRGetBookmark( pfucbT, &pbmPrimary ) );

        //  set return values
        //
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

    //  insert item in secondary indexes
    //
    // No critical section needed to guard index list because Updating latch
    // protects it.
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
                //  see if the sparse conditional index can tell
                //  us to skip the update
                if ( pidb->FSparseConditionalIndex() )
                    fUpdate = FRECIPossiblyUpdateSparseConditionalIndex( pidb, pfucb );

                //  if the sparse conditional index could not cause us to
                //  skip the index update, see if the sparse index can
                //  tell us to skip the update
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

    //  if no error, commit transaction
    //
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    FUCBResetUpdateFlags( pfucb );

    // Do the AfterInsert callback
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

    /*  rollback all changes on error
    /**/
    CallSx( ErrDIRRollback( ppib ), JET_errRollbackError );

    //  if no version update failed then table may be corrupt.
    //  Session must roll back to level 0 to delete table.
    //
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


// Called by defrag to insert record but preserve copy buffer.
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
                //  must have been record with multi-value column
                //  or tuples with sufficiently similar values
                //  (ie. the indexed portion of the multi-values
                //  or tuples were identical) to produce redundant
                //  index entries.
                //
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

//+local
// ErrRECIAddToIndex
// ========================================================================
// ERR ErrRECIAddToIndex( FCB *pfcbIdx, FUCB *pfucb )
//
// Extracts key from data record and adds that key with the given primary key to the index
//
// PARAMETERS   pfcbIdx                     FCB of index to insert into
//              pfucb                       cursor pointing to primary index record
//
// RETURNS      JET_errSuccess, or error code from failing routine
//
// SIDE EFFECTS
// SEE ALSO     Insert
//-
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

    //  unversioned inserts into a secondary index are dangerous as if this fails the
    //  record will not be removed from the primary index.  
    //  Uncommitted tables that suffer an update failure must be rolled back.
    //
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


//+API
// ErrIsamDelete
// ========================================================================
// ErrIsamDelete( PIB *ppib, FCBU *pfucb )
//
// Deletes the current record from data file.  All indexes on the data
// file are updated to reflect the deletion.
//
// PARAMETERS
//          ppib        PIB of this user
//          pfucb       FUCB for file to delete from
// RETURNS
//      Error code, one of the following:
//          JET_errSuccess              Everything went OK.
//          JET_errNoCurrentRecord      There is no current record.
// SIDE EFFECTS
//          After the deletion, file currency is left just before
//          the next record.  Index currency (if any) is left just
//          before the next index entry.  If the deleted record was
//          the last in the file, the currencies are left after the
//          new last record.  If the deleted record was the only record
//          in the entire file, the currencies are left in the
//          "beginning of file" state.  On failure, the currencies are
//          returned to their initial states.
//          If there is a working buffer for SetField commands,
//          it is discarded.
// COMMENTS
//          If the currencies are not ON a record, the delete will fail.
//          A transaction is wrapped around this function.  Thus, any
//          work done will be undone if a failure occurs.
//          Index entries are not made for entirely-null keys.
//          For temporary files, transaction logging is deactivated
//          for the duration of the routine.
//-
ERR VTAPI ErrIsamDelete(
    JET_SESID   sesid,
    JET_VTID    vtid )
{
    ERR         err;
    PIB         * ppib              = reinterpret_cast<PIB *>( sesid );
    FUCB        * pfucb             = reinterpret_cast<FUCB *>( vtid );
    FCB         * pfcbTable;                    // table FCB
    FCB         * pfcbIdx;                      // loop variable for each index on file
    BOOL        fUpdatingLatchSet   = fFalse;

    CallR( ErrPIBCheck( ppib ) );

    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );
    AssertDIRNoLatch( ppib );

    //  ensure that table is updatable
    //
    CallR( ErrFUCBCheckUpdatable( pfucb )  );
    if ( !FFMPIsTempDB( pfucb->ifmp ) )
    {
        CallR( ErrPIBCheckUpdatable( ppib ) );
    }

    //  efficiency variables
    //
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

#ifdef PREREAD_INDEXES_ON_DELETE
    if( pfcbTable->FPreread() )
    {
        BTPrereadIndexesOfFCB( pfucb );
    }
#endif  //  PREREAD_INDEXES_ON_DELETE

    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        //  reset copy buffer status on record delete unless we are in
        //  insert-copy-delete-original mode (ie: copy buffer is in use)
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

    //  if InsertCopyDeleteOriginal, transaction is started in ErrRECIInsert()
    Assert( !FFUCBInsertCopyDeleteOriginalPrepared( pfucb ) );
    Assert( ppib->Level() > 0 );

#ifdef DEBUG
    const BOOL  fLogIsDone = !PinstFromPpib( ppib )->m_plog->FLogDisabled()
                            && !PinstFromPpib( ppib )->m_plog->FRecovering()
                            && g_rgfmp[pfucb->ifmp].FLogOn();
#endif

    //  efficiency variables
    //
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    // Do the BeforeDelete callback
    Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeDelete, 0, NULL, NULL, 0 ) );

    // After ensuring that we're in a transaction, refresh
    // our cursor to ensure we still have access to the record.
    Call( ErrDIRGetLock( pfucb, writeLock ) );

    Call( pfcbTable->ErrSetUpdatingAndEnterDML( ppib ) );
    fUpdatingLatchSet = fTrue;

    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );
    Assert( ppib->Level() < levelMax );

    //  delete from secondary indexes
    //
    // No critical section needed to guard index list because Updating latch
    // protects it.
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

    //  do not touch LV data if we are doing an insert-copy-delete-original
    //  
    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        //  delete record long values
        //
        Call( ErrRECDereferenceLongFieldsInRecord( pfucb ) );
    }

    Assert( fLogIsDone == ( !PinstFromPpib( ppib )->m_plog->FLogDisabled() && !PinstFromPpib( ppib )->m_plog->FRecovering() && g_rgfmp[pfucb->ifmp].FLogOn() ) );

    //  delete record
    //
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

    //  process result of deletion
    //
    Call( err );

    //  if no error, commit transaction
    //
    //  if InsertCopyDeleteOriginal, commit will be performed in ErrRECIInsert()
    if ( !FFUCBUpdateForInsertCopyDeleteOriginal( pfucb ) )
    {
        Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );
    }

    AssertDIRNoLatch( ppib );

    // Do the AfterDelete callback
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
        pfcbTable->ResetUpdating();     //lint !e644
    }

    //  rollback all changes on error
    //  if InsertCopyDeleteOriginal, rollback will be performed in ErrRECIInsert()
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
                //  must have been record with multi-value column
                //  or tuples with sufficiently similar values
                //  (ie. the indexed portion of the multi-values
                //  or tuples were identical) to produce redundant
                //  index entries.
                //
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
            CallS( err );   //  don't expect any warnings except the ones filtered out above

            //  PERF: we should be able to avoid the release and call
            //  ErrDIRDelete with the page latched
            //
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

//+INTERNAL
//  ErrRECIDeleteFromIndex
//  ========================================================================
//  ErrRECIDeleteFromIndex( FCB *pfcbIdx, FUCB *pfucb )
//
//  Extracts key from data record and deletes the key with the given SRID
//
//  PARAMETERS
//              pfucb                           pointer to primary index record to delete
//              pfcbIdx                         FCB of index to delete from
//  RETURNS
//              JET_errSuccess, or error code from failing routine
//  SIDE EFFECTS
//  SEE ALSO    ErrRECDelete
//-

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

    //  delete all keys from this index for dying data record
    //
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


//  determines whether an index may have changed using the hashed tags
//
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


//  determines whether an index has changed by comparing the keys
//  UNDONE: only checks first multi-value in a multi-valued index, but
//  for now that's fine because this code is only used for DEBUG purposes
//  except on the primary index, which will never have multi-values
//
LOCAL ERR ErrRECFIndexChanged( FUCB *pfucb, FCB *pfcbIdx, BOOL *pfChanged )
{
    KEY     keyOld;
    KEY     keyNew;
    BYTE    *pbOldKey                       = NULL;     //  this function is called on primary index to ensure it hasn't changed
    BYTE    *pbNewKey                       = NULL;     //  and on secondary index to cascade record updates
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

    //  UNDONE: do something else for tuple indexes
    Assert( !pfcbIdx->Pidb()->FTuples() );

    //  get new key from copy buffer
    //
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
    Assert( wrnFLDOutOfKeys != err );       //  should never get OutOfKeys since we're only retrieving itagSequence 1
    Assert( wrnFLDOutOfTuples != err );     //  this routine not currently called for tuple indexes

    fCopyBufferKeyIsPresentInIndex = ( wrnFLDNotPresentInIndex != err );


    //  get the old key from the node
    //
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
        //  one is in the index and the other isn't (even though an indexed column may not have changed!)
        *pfChanged = fTrue;
        Assert( !Pcsr( pfucb )->FLatched() );
        err = JET_errSuccess;
        goto HandleError;
    }
    else if( !fCopyBufferKeyIsPresentInIndex && !fRecordKeyIsPresentInIndex )
    {
        //  neither are in the index (nothing has changed)
        *pfChanged = fFalse;
        Assert( !Pcsr( pfucb )->FLatched() );
        err = JET_errSuccess;
        goto HandleError;
    }

#ifdef DEBUG
    //  record must honor index no NULL segment requirements
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



//  upgrades a ReplaceNoLock to a regular Replace by write-locking the record
ERR ErrRECUpgradeReplaceNoLock( FUCB *pfucb )
{
    ERR     err;

    Assert( FFUCBReplaceNoLockPrepared( pfucb ) );

    //  UNDONE: compute checksum on commit to level 0
    //          in support of following sequence:
    //              BeginTransaction
    //              PrepareUpdate, defer checksum since in xact
    //              SetColumns
    //              Commit to level 0, other user may update it
    //              Update
    Assert( !FFUCBDeferredChecksum( pfucb )
        || pfucb->ppib->Level() > 0 );

    CallR( ErrDIRGetLock( pfucb, writeLock ) );

    CallR( ErrDIRGet( pfucb ));
    const BOOL  fWriteConflict = ( !FFUCBDeferredChecksum( pfucb ) && !FFUCBCheckChecksum( pfucb ) );
    CallS( ErrDIRRelease( pfucb ));

    if ( fWriteConflict )
    {
        //  UNDONE: is there a way to easily report the bm?
        //
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


//+local
//  ErrRECIReplace
//  ========================================================================
//  ErrRECIReplace( FUCB *pfucb, DIRFLAG dirflag )
//
//  Updates a record in a data file.     All indexes on the data file are
//  pdated to reflect the updated data record.
//
//  PARAMETERS  pfucb        FUCB for file
//  RETURNS     Error code, one of the following:
//                   JET_errSuccess              Everything went OK.
//                  -NoCurrentRecord             There is no current record
//                                               to update.
//                  -RecordNoCopy                There is no working buffer
//                                               to update from.
//                  -KeyDuplicate                The new record data causes an
//                                               illegal duplicate index entry
//                                               to be generated.
//                  -RecordPrimaryChanged        The new data causes the primary
//                                               key to change.
//  SIDE EFFECTS
//      After update, file currency is left on the updated record.
//      Similar for index currency.
//      The effect of a GetNext or GetPrevious operation will be
//      the same in either case.  On failure, the currencies are
//      returned to their initial states.
//      If there is a working buffer for SetField commands,
//      it is discarded.
//
//  COMMENTS
//      If currency is not ON a record, the update will fail.
//      A transaction is wrapped around this function.  Thus, any
//      work done will be undone if a failure occurs.
//      For temporary files, transaction logging is deactivated
//      for the duration of the routine.
//      Index entries are not made for entirely-null keys.
//-
LOCAL ERR ErrRECIReplace( FUCB *pfucb, const JET_GRBIT grbit )
{
    ERR     err;                    // error code of various utility
    PIB *   ppib                = pfucb->ppib;
    FCB *   pfcbTable;              // file's FCB
    TDB *   ptdb;
    FCB *   pfcbIdx;                // loop variable for each index on file
    FID     fidFixedLast;
    FID     fidVarLast;
    FID     fidVersion;
    BOOL    fUpdateIndex;
    BOOL    fUpdatingLatchSet   = fFalse;

    CheckPIB( ppib );
    AssertDIRNoLatch( ppib );
    CheckTable( ppib, pfucb );
    CheckSecondary( pfucb );

    //  should have been checked in PrepareUpdate
    //
    Assert( FFUCBUpdatable( pfucb ) );
    Assert( FFUCBReplacePrepared( pfucb ) );

    //  efficiency variables
    //
    pfcbTable = pfucb->u.pfcb;
    Assert( pfcbTable != pfcbNil );

    ptdb = pfcbTable->Ptdb();
    Assert( ptdbNil != ptdb );

    //  data to use for update is in workBuf
    //
    Assert( !pfucb->dataWorkBuf.FNull() );

    CallR( ErrDIRBeginTransaction( ppib, 62245, NO_GRBIT ) );

    //  NoVersion replace operations are not yet supported.
    //
    if ( ( 0 != (grbit & JET_bitUpdateNoVersion) ) )
    {
        Error( ErrERRCheck( JET_errUpdateMustVersion ) );
    }

    //  optimistic locking, ensure that record has
    //  not changed since PrepareUpdate
    //
    if ( FFUCBReplaceNoLockPrepared( pfucb ) )
    {
        Call( ErrRECUpgradeReplaceNoLock( pfucb ) );
    }

    // Do the BeforeReplace callback
    Call( ErrRECCallback( ppib, pfucb, JET_cbtypBeforeReplace, 0, NULL, NULL, 0 ) );

    //  check for clients that commit an update even though they didn't actually change anything
    //
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

    //  Set these efficiency variables after FUCB read latch
    //
    pfcbTable->AssertDML();
    fidFixedLast = ptdb->FidFixedLast();
    fidVarLast = ptdb->FidVarLast();

    //  if need to update indexes, then cache old record
    //
    fUpdateIndex = FRECIIndexPossiblyChanged( ptdb->RgbitAllIndex(), pfucb->rgbitSet );

    pfcbTable->LeaveDML();

    Assert( !Pcsr( pfucb )->FLatched() );
    if ( fUpdateIndex )
    {
        //  ensure primary key did not change
        //
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

    //  set version column if present
    //
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

        //  get current record
        //
        Call( ErrDIRGet( pfucb ) );

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

        //  increment field from value in current record
        //
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

        //  handle case where field is NULL when column added
        //  to table with records present
        //
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

    //  update indexes
    //
    if ( fUpdateIndex )
    {
#ifdef PREREAD_INDEXES_ON_REPLACE
        if( pfucb->u.pfcb->FPreread() )
        {
            const INT cSecondaryIndexesToPreread = 16;

            PGNO rgpgno[cSecondaryIndexesToPreread + 1];    //  NULL-terminated
            INT ipgno = 0;

            // No critical section needed to guard index list because Updating latch
            // protects it.
            for ( pfcbIdx = pfcbTable->PfcbNextIndex();
                  pfcbIdx != pfcbNil && ipgno < cSecondaryIndexesToPreread;
                  pfcbIdx = pfcbIdx->PfcbNextIndex() )
            {
                if ( pfcbIdx->Pidb() != pidbNil
                     && FFILEIPotentialIndex( pfcbTable, pfcbIdx )
                     && FRECIIndexPossiblyChanged( pfcbIdx->Pidb()->RgbitIdx(), pfucb->rgbitSet ) )
                {
                    //  preread this index as we will probably update it
                    rgpgno[ipgno++] = pfcbIdx->PgnoFDP();
                }
            }
            rgpgno[ipgno] = pgnoNull;

            PIBTraceContextScope tcScope = pfucb->ppib->InitTraceContextScope();
            tcScope->nParentObjectClass = TceFromFUCB( pfucb );
            tcScope->SetDwEngineObjid( ObjidFDP( pfucb ) );

            BFPrereadPageList( pfucb->ifmp, rgpgno, bfprfDefault, pfucb->ppib->BfpriPriority( pfucb->ifmp ), *tcScope );
        }
#endif  //  PREREAD_INDEXES_ON_REPLACE

        // No critical section needed to guard index list because Updating latch
        // protects it.
        for ( pfcbIdx = pfcbTable->PfcbNextIndex();
            pfcbIdx != pfcbNil;
            pfcbIdx = pfcbIdx->PfcbNextIndex() )
        {
            if ( pfcbIdx->Pidb() != pidbNil )       // sequential indexes don't need updating
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
                        //  UNDONE: Need to come up with some other validation
                        //  routine because ErrRECFIndexChanged() doesn't
                        //  properly handle tuple index entries
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

    //  do the replace
    //
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

    //  process result of update
    //
    Call( err );

Done:
    //  if no error, commit transaction
    //
    Call( ErrDIRCommitTransaction( ppib, NO_GRBIT ) );

    FUCBResetUpdateFlags( pfucb );

    // Do the AfterReplace callback
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

    //  rollback all changes on error
    //
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

        // For a sequence of eDeletes, turn all but the last one into an eSkip.
        // For a sequence of iInserts, turn all but the first one into an eSkip.
        // For an eDelete followed by an eInsert, turn both into eSkips.
#define STATE(X, Y)  ( (X << 4) | Y )
        Assert( ( 1 << 4 ) > next.m_Action );

        switch ( STATE(this->m_Action, next.m_Action ) )
        {
            case STATE( eDelete, eDelete ): //  => STATE(eSkip, eDelete)
                this->m_Action = eSkip;
                break;

            case STATE( eInsert, eInsert): // => STATE( eInsert, eSkip )
            case STATE( eSkip, eInsert): // => STATE( eSkip, eSkip )
                next.m_Action = eSkip;
                break;

            case STATE( eDelete, eInsert ): // => STATE( eSkip, eSkip )
                this->m_Action = eSkip;
                next.m_Action = eSkip;
                break;

            case STATE( eSkip, eDelete ): // => STATE( eSkip, eDelete )
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

        // At the minimum, allocate 256 of them.
        if ( 256 >= cTotalRequested )
        {
            cNeeded = 256;
        }
        else
        {
            // Round up to the next power of 2.  If already a power of 2, doesn't do anything.
            // That is (LNextPowerOf2( 256 ) == 256 ), not (LNextPowerOf2(256) == 512).
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

        //
        // We tracked keys to post process.  Sort by key, with eDeletes from the old record
        // coming before eInserts from the new record.  There should be no eSkips at this time,
        // they all come during AdjustForDuplicateKeys().
        //
        qsort( m_pData, m_cDataUsed, sizeof( m_pData[ 0 ] ), &(TRACK_INDEX_ENTRY_DATA::Comp) );

        //
        // Look for successive duplicate keys, re-marking actions appropriately.
        //
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


//+local
// ErrRECIReplaceInIndex
// ========================================================================
// ERR ErrRECIReplaceInIndex( FCB *pfcbIdx, FUCB *pfucb )
//
// Removes all unneded index entries from an index and adds all needed entries.
//
// PARAMETERS
//              pfcbIdx                     FCB of index to insert into
//              pfucb                       record FUCB pointing to primary record changed
//
// RETURNS      JET_errSuccess, or error code from failing routine
//
// SIDE EFFECTS
// SEE ALSO     Replace
//-
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
        // Delete all the old keys.
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

        // Insert all the new keys.
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
        // Track all the old keys for potential deletion.
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

        // Track all the new keys for potential insertion.
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
            //
            // We tracked keys to post process.
            TrackContext.CleanActionList();

            //
            // Do the resulting actions.
            //
            for ( ULONG i = 0; i < TrackContext.m_cDataUsed; i++ )
            {
                switch (TrackContext.m_pData[i].Action())
                {
                    case TRACK_INDEX_ENTRY_DATA::eSkip:
                        // Key either was in both old and new record AND/OR was present in old or new
                        // more than once.  We act only once on a key.
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
                        // Necessary on the first eInsert, cheap on all the others.
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

//  ================================================================
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
//  ================================================================
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

    //  assert against client misbehaviour - EscrowUpdating a record while
    //  another cursor of the same session has an update pending on the record
    CallR( ErrRECSessionWriteConflict( pfucb ) );

    FCB *pfcb = pfucb->u.pfcb;

    if ( FCOLUMNIDTemplateColumn( columnid ) )
    {
        if ( !pfcb->FTemplateTable() )
        {
            // Switch to template table.
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

    //  ASSERT: the offset is represented in the record

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

    // Escrow updates on 32-bit and 64-bit signed/unsigned types are supported
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

//  ================================================================
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
//  ================================================================
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

    //  ensure that table is updatable
    //
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
