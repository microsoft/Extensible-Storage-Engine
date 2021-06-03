// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  LOGREDO - logical part of soft/hard recovery
//  ============================================
//
//  ENTRY POINT(S):
//      ErrLGSoftStart
//
//  PURPOSE:
//      Logical part of soft/hard recovery. Replays logfiles, starting from
//      the begining of logfile pointed by checkpoint. If there is no checkpoint
//      starts from the begining of the lowest available log generation file.
//
//      Main loop is through ErrLGRIRedoOperations
//
//  BASE PROTOTYPES:
//      class LOG in log.hxx
//
//  OFTEN USED PROTOTYPES:
//      classes LRxxxx in logapi.hxx
//
/////////////////////////////////////////////

#include "logstd.hxx"
#include "_ver.hxx"
#include "_space.hxx"
#include "_bt.hxx"
#include "_logredomap.hxx"

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotal<ULONG, INST, fFalse> cLGRecoveryStallReadOnly;
LONG RecoveryStallReadOnlyCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGRecoveryStallReadOnly.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<ULONG, INST, fFalse> cLGRecoveryLongStallReadOnly;
LONG RecoveryLongStallReadOnlyCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGRecoveryLongStallReadOnly.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse> cLGRecoveryStallReadOnlyTime;
LONG RecoveryStallReadOnlyTimeCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGRecoveryStallReadOnlyTime.PassTo( iInstance, pvBuf );
    return 0;
}

#endif

// LOG functions.

ERR LOG::ErrGetLgposRedoWithCheck()
{
    ERR     err = JET_errSuccess;

    LGPOS lgposT;
    m_pLogReadBuffer->GetLgposOfPbNext( &lgposT );
    C_ASSERT( sizeof(LGPOS) == sizeof(LONGLONG) );
    AtomicExchange( (LONGLONG*)&m_lgposRedo, *(LONGLONG*)&lgposT );

    PERFOpt( ibLGTip.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_lgposRedo, lgposMin ) ) );

#ifdef RTM
#else
    extern LGPOS g_lgposRedoTrap;
    AssertSzRTL( CmpLgpos( m_lgposRedo, g_lgposRedoTrap ), "Redo Trap" );
#endif

    // check for the potential stopping point
    if ( FLGRecoveryLgposStopLogRecord( m_lgposRedo ) )
    {
        // we hit already the desired log stop position


        Error( ErrLGISetQuitWithoutUndo( ErrERRCheck( JET_errRecoveredWithoutUndo ) ) );
    }

    Assert ( JET_errSuccess == err );

HandleError:
    return err;
}

const DIRFLAG   fDIRRedo = fDIRNoLog;

LOCAL VOID TraceReplacePageImage( const char * const szOperation, const PGNO pgno )
{
    OSTrace(
        JET_tracetagRecovery,
        OSFormat(
            "ErrReplacePageImage: %s: %d",
            szOperation,
            pgno));
}

//  replace a page with a before image. used in redo of split and merge
//  we are given an image of a page at time 'dbtime'. the image is copied
//  in and then reverted to time 'dbtimeBefore' (this is because the before
//  image is logged after the page is dirtied).
//
LOCAL ERR ErrReplacePageImage(
                        PIB * const ppib,
                        const IFMP ifmp,
                        CSR& csr,
    __in_bcount( cb )   const BYTE * const pbBeforeImage,
                        const INT cb,
                        const PGNO pgno,
                        const DBTIME dbtime,
                        const DBTIME dbtimeBefore )
{
    ERR err;

    Assert( cb == g_cbPage );
    Assert( csr.Pgno() == pgno );
    Assert( dbtime > dbtimeBefore );

    // It should either be a normal page, or Sparse.
    Assert( csr.PagetrimState() == pagetrimNormal || csr.PagetrimState() == pagetrimTrimmed );

    if ( pagetrimTrimmed == csr.PagetrimState() )
    {
#ifdef DEBUG
        Assert( g_rgfmp[ ifmp ].FPgnoInZeroedOrRevertedMaps( pgno ) );
#endif
    }
    else
    {
#ifdef DEBUG
        Assert( csr.Latch() == latchRIW );
        Expected( csr.Dbtime() != dbtimeBefore );

        // the current version of the page should always be newer than the before image
        // (otherwise we are moving the page forwards in time), except for when it's going to be 
        // trimmed in the future, in which case the page must be in the bad dbtime redo map.
        Assert( ( csr.Dbtime() > dbtimeBefore ) ||
                ( g_rgfmp[ ifmp ].PLogRedoMapBadDbTime() && g_rgfmp[ ifmp ].PLogRedoMapBadDbTime()->FPgnoSet( pgno ) ) );

        // replacing a page image with itself?
        Assert( 0 != memcmp( csr.Cpage().PvBuffer(), pbBeforeImage, cb ) );

        {
        CPAGE cpage;
        void * pvPage = NULL;
        BFAlloc( bfasTemporary, &pvPage );
        memcpy( pvPage, pbBeforeImage, cb );
        cpage.LoadPage( (BYTE *)pvPage, cb );
        ASSERT_VALID_OBJ( cpage );
        cpage.DebugCheckAll();
        Assert( cpage.Dbtime() == dbtime );
        BFFree( pvPage );
        }
#endif

        if( cb != g_cbPage
            || pgno != csr.Pgno()
            || latchRIW != csr.Latch() )
        {
            Call( ErrERRCheck( JET_errInternalError ) );
        }

        // release the current cached page and allocate a new buffer and copy the before image into 
        // it. the before image page won't be written to disk even if it is dirtied. this is good
        // as writing the updated before image to disk causes recovery problems -- we aren't 
        // guaranteed that all operations against the page since the split/merge will replay as
        // the modifying transaction's Begin0 record may be older than the checkpoint.

        csr.ReleasePage();
    }

    Assert( !csr.FLatched() );

    Call( csr.ErrLoadPage( ppib, ifmp, pgno, pbBeforeImage, cb, latchWrite ) );
    // the before image of the page is logged after the dbtime was updated so we have to restore it
    csr.RestoreDbtime( dbtimeBefore );
    csr.Downgrade( latchRIW );

    Assert( csr.Pgno() == pgno );
    Assert( csr.Latch() == latchRIW );
    Assert( csr.Dbtime() == dbtimeBefore );

HandleError:
    return err;
}

//  rebuild a page with its header/trailer.
//
LOCAL VOID RebuildPageImageHeaderTrailer(
    __in_bcount( cbHeader )     const VOID * const pvHeader,
                                const INT cbHeader,
    __in_bcount( cbTrailer )    const VOID * const pvTrailer,
                                const INT cbTrailer,
    _Out_                       VOID * const pvDest )
{
    Assert( ( cbHeader + cbTrailer ) <= g_cbPage );

    BYTE * pbCurr = reinterpret_cast<BYTE *>( pvDest );
    UtilMemCpy( pbCurr, pvHeader, cbHeader );
    pbCurr += cbHeader;

    const size_t cbBlank = g_cbPage - ( cbHeader + cbTrailer );
    memset( pbCurr, 0, cbBlank );
    pbCurr += cbBlank;

    UtilMemCpy( pbCurr, pvTrailer, cbTrailer );
}

//  replace a page with a before image which is broken into header+trailer
//  (used by page move)
//
//  we are given an image of a page at time 'dbtime'. the image is copied
//  in and then reverted to time 'dbtimeBefore' (this is because the before
//  image is logged after the page is dirtied).
//
LOCAL ERR ErrReplacePageImageHeaderTrailer(
                                PIB * const ppib,
                                const IFMP ifmp,
                                CSR& csr,
    __in_bcount( cbHeader )     const VOID * const pvHeader,
                                const INT cbHeader,
    __in_bcount( cbTrailer )    const VOID * const pvTrailer,
                                const INT cbTrailer,
                                const PGNO pgno,
                                const DBTIME dbtime,
                                const DBTIME dbtimeBefore )
{

    VOID * pvBuffer;
    BFAlloc( bfasTemporary, &pvBuffer );

    RebuildPageImageHeaderTrailer( pvHeader, cbHeader, pvTrailer, cbTrailer, pvBuffer );

    const ERR err = ErrReplacePageImage( ppib, ifmp, csr, (BYTE *)pvBuffer, g_cbPage, pgno, dbtime, dbtimeBefore );
    BFFree( pvBuffer );

    return err;
}

//  checks if page needs a redo of operation
//
INLINE BOOL FLGINeedRedo( const CSR& csr, const DBTIME dbtime )
{
    if ( pagetrimTrimmed == csr.PagetrimState() )
    {
        // The CSR's latch state doesn't need to be checked if we won't need to redo the operation.
        return fFalse;
    }

    Assert( csr.FLatched() );
    Assert( csr.Dbtime() == csr.Cpage().Dbtime() );

    return dbtime > csr.Dbtime();
}

//  report unexpected dbtime
//
LOCAL ERR ErrLGRIReportDbtimeMismatch(
    const INST*     pinst,
    const IFMP      ifmp,
    const PGNO      pgno,
    const DBTIME    dbtimeBeforeInLogRec,
    const DBTIME    dbtimeOnPage,
    const DBTIME    dbtimeAfterInLogRec,
    const LGPOS&    lgposMismatchLR,
    const CPG       cpgAffected )
{
    Assert( FMP::FAllocatedFmp( ifmp ) );
    Assert( dbtimeOnPage != dbtimeBeforeInLogRec );

    const ERR       err         = ErrERRCheck( dbtimeOnPage < dbtimeBeforeInLogRec ? JET_errDbTimeTooOld : ( dbtimeOnPage < dbtimeAfterInLogRec ? JET_errDbTimeTooNew : JET_errDbTimeBeyondMaxRequired ) );
    const FMP* const pfmp       = FMP::FAllocatedFmp( ifmp ) ? &g_rgfmp[ ifmp ] : NULL;
    const LGPOS     lgposStop   = pinst->m_plog->LgposLGLogTipNoLock();
    Assert( err != JET_errDbTimeBeyondMaxRequired || !pfmp->FContainsDataFromFutureLogs() );

    OSTraceSuspendGC();
    const WCHAR* wszDatabaseName = pfmp ?
                                    pfmp->WszDatabaseName() :
                                    pinst->m_wszInstanceName;
    wszDatabaseName = wszDatabaseName ? wszDatabaseName : L"";
    const WCHAR* rgwsz[] =
    {
        wszDatabaseName,
        OSFormatW( L"%I32u (0x%08I32x)", pgno, pgno ),
        OSFormatW( L"0x%I64x", dbtimeBeforeInLogRec ),
        OSFormatW( L"0x%I64x", dbtimeOnPage ),
        OSFormatW( L"%d", err ),
        OSFormatW( L"0x%I64x", dbtimeAfterInLogRec ),
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposMismatchLR.lGeneration, lgposMismatchLR.isec, lgposMismatchLR.ib ),
        pfmp ?
            OSFormatW( L"%d", (INT)pfmp->FContainsDataFromFutureLogs() ) :
            L"?",
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposStop.lGeneration, lgposStop.isec, lgposStop.ib ),
        OSFormatW( L"%d", cpgAffected )
    };
    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        DBTIME_MISMATCH_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pinst );
    OSUHAPublishEvent(  ( err == JET_errDbTimeTooOld ? HaDbFailureTagLostFlushDbTimeTooOld :
                          ( err == JET_errDbTimeTooNew ? HaDbFailureTagLostFlushDbTimeTooNew : HaDbFailureTagLostFlushDbTimeBeyondMaxRequired ) ),
                        pinst,
                        HA_LOGGING_RECOVERY_CATEGORY,
                        HaDbIoErrorNone,
                        wszDatabaseName,
                        OffsetOfPgno( pgno ),
                        g_cbPage,
                        HA_DBTIME_MISMATCH_ID,
                        _countof( rgwsz ),
                        rgwsz );
    OSTraceResumeGC();

    return err;
}

//  Checks if page needs a redo of operation.
//  perr contains an error if there is a dbtime-too-(new|old) error.
//  If the page is trimmed (according to csr), then returns fFalse (and perr = JET_errSuccess).
//
INLINE BOOL FLGNeedRedoCheckDbtimeBefore(
    const IFMP      ifmp,
    const CSR&      csr,
    const DBTIME    dbtimeAfter,
    const DBTIME    dbtimeBefore,
    ERR*            const perr )
{
    BOOL        fRedoNeeded     = FLGINeedRedo( csr, dbtimeAfter );

    *perr = JET_errSuccess;

    Assert( dbtimeNil != dbtimeBefore );

    switch ( csr.PagetrimState() )
    {
        case pagetrimNormal:
        {
            Assert( dbtimeInvalid != dbtimeBefore );
            if ( fRedoNeeded )
            {
                // dbtimeBefore on page should be the same one as in the record
                if ( csr.Dbtime() != dbtimeBefore )
                {
                    FMP* const pfmp = &g_rgfmp[ ifmp ];
                    LOG* const plog = pfmp->Pinst()->m_plog;


                    if ( csr.Dbtime() < dbtimeBefore
                         && plog->FRedoMapNeeded( ifmp ) )
                    {
                        // We're likely redoing a page operation that will be trimmed or shrunk
                        // in the future, but we do need to keep track of it just in case
                        // it isn't (i.e., it's a legitimate DBTIME mismatch from a lost flush or
                        // a bug in the engine).
                        *perr = pfmp->ErrEnsureLogRedoMapsAllocated();
                        if ( *perr >= JET_errSuccess )
                        {
                            *perr = pfmp->PLogRedoMapBadDbTime()->ErrSetPgno(
                                csr.Pgno(),
                                plog->LgposLGLogTipNoLock(),
                                JET_errDbTimeTooOld,
                                dbtimeBefore,
                                csr.Dbtime(),
                                dbtimeAfter );
     
                            if ( *perr >= JET_errSuccess )
                            {
                                plog->SetPendingRedoMapEntries();

                                // Since this page is missing other updates from the past, we cannot let it
                                // be updated since it would be missing an indeterminate amount of data.
                                fRedoNeeded = fFalse;
                            }
                        }
                    }
                    else
                    {
                        *perr = ErrLGRIReportDbtimeMismatch(
                            pfmp->Pinst(),
                            ifmp,
                            csr.Pgno(),
                            dbtimeBefore,
                            csr.Dbtime(),
                            dbtimeAfter,
                            plog->LgposLGLogTipNoLock(),
                            1 );

                        // assert after event log for easier diagnosis ...
                        Assert( csr.Dbtime() == dbtimeBefore || FNegTest( fCorruptingWithLostFlush ) );
                    }
                }
            }
            else
            {
                Assert( dbtimeAfter > dbtimeBefore );
                // If the page is trimmed, we have no consistency guarantees.
                Assert( csr.Dbtime() >= dbtimeAfter );
            }
            break;
        }
        case pagetrimTrimmed:
            Assert( !csr.FLatched() );
            break;
        default:
            AssertSz( fFalse, "Invalid csr.PagetrimState() state!" );
            *perr = ErrERRCheck( JET_errInternalError );
    }

    Assert( fRedoNeeded || JET_errSuccess == *perr );
    // If we are beyond required range, all updates should require redo (i.e. not be already written out)
    // Or in other words, if we do not require redo for an update, we should be in required range.
    //
    if ( !fRedoNeeded && !g_rgfmp[ ifmp ].FContainsDataFromFutureLogs() && *perr >= JET_errSuccess )
    {
        AssertTrack( fFalse, "RedoNotNeededBeyondRequiredRange" );
        if ( BoolParam( g_rgfmp[ ifmp ].Pinst(), JET_paramFlight_CheckRedoNeededBeyondRequiredRange ) )
        {
            *perr = ErrLGRIReportDbtimeMismatch(
                g_rgfmp[ ifmp ].Pinst(),
                ifmp,
                csr.Pgno(),
                dbtimeBefore,
                csr.Dbtime(),
                dbtimeAfter,
                g_rgfmp[ ifmp ].Pinst()->m_plog->LgposLGLogTipNoLock(),
                1 );
        }
    }

    return fRedoNeeded;
}

//  checks if page needs a redo of operation
//
INLINE BOOL FLGNeedRedoPage( const CSR& csr, const DBTIME dbtime )
{
    return FLGINeedRedo( csr, dbtime );
}

#ifdef DEBUG
//  checks if page needs a redo of operation
//
INLINE BOOL FAssertLGNeedRedo( const IFMP ifmp, const CSR& csr, const DBTIME dbtime, const DBTIME dbtimeBefore )
{
    ERR         err;
    const BOOL  fRedoNeeded = FLGNeedRedoCheckDbtimeBefore( ifmp, csr, dbtime, dbtimeBefore, &err );

    return ( fRedoNeeded && JET_errSuccess == err );
}
#endif // DEBUG


LOCAL
VOID LGIReportEventOfReadError( const INST* pinst, const IFMP ifmp, const PGNO pgno, const ERR err, const DBTIME dbtime )
{
    Assert( FMP::FAllocatedFmp( ifmp ) );

    const LGPOS lgposStop = pinst->m_plog->LgposLGLogTipNoLock();
    OSTraceSuspendGC();
    const WCHAR* wszDatabaseName = FMP::FAllocatedFmp( ifmp ) ?
                                    g_rgfmp[ifmp].WszDatabaseName() :
                                    pinst->m_wszInstanceName;
    wszDatabaseName = wszDatabaseName ? wszDatabaseName : L"";
    const WCHAR* rgwsz[] =
    {
        wszDatabaseName,
        OSFormatW( L"%I32u", pgno ),
        OSFormatW( L"%d", err ),
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposStop.lGeneration, lgposStop.isec, lgposStop.ib ),
        OSFormatW( L"0x%I64x", dbtime )
    };
    UtilReportEvent(
            eventError,
            LOGGING_RECOVERY_CATEGORY,
            RECOVERY_DATABASE_READ_PAGE_ERROR_ID,
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            pinst );
    OSTraceResumeGC();
}

LOCAL
VOID LGIReportPageDataMissing( const INST* pinst, const IFMP ifmp, const PGNO pgno, const ERR err, const LGPOS& lgposMismatchLR, const CPG cpgAffected )
{
    Assert( FMP::FAllocatedFmp( ifmp ) );

    const FMP* const pfmp = FMP::FAllocatedFmp( ifmp ) ? &g_rgfmp[ ifmp ] : NULL;
    const LGPOS lgposStop = pinst->m_plog->LgposLGLogTipNoLock();

    OSTraceSuspendGC();
    const WCHAR* wszDatabaseName = pfmp ?
                                    pfmp->WszDatabaseName() :
                                    pinst->m_wszInstanceName;
    wszDatabaseName = wszDatabaseName ? wszDatabaseName : L"";
    const WCHAR* rgwsz[] =
    {
        wszDatabaseName,
        OSFormatW( L"%I32u (0x%08x)", pgno, pgno ),
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposMismatchLR.lGeneration, lgposMismatchLR.isec, lgposMismatchLR.ib ),
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposStop.lGeneration, lgposStop.isec, lgposStop.ib ),
        OSFormatW( L"%d", err ),
        pfmp ?
            OSFormatW( L"%d", (INT)pfmp->FContainsDataFromFutureLogs() ) :
            L"?",
        OSFormatW( L"%d", cpgAffected )
    };
    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        RECOVERY_DATABASE_PAGE_DATA_MISSING_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pinst );
    OSUHAPublishEvent(
        HaDbFailureTagCorruption,
        pinst,
        Ese2HaId( LOGGING_RECOVERY_CATEGORY ),
        HaDbIoErrorNone,
        wszDatabaseName,
        OffsetOfPgno( pgno ),
        g_cbPage,
        Ese2HaId( RECOVERY_DATABASE_PAGE_DATA_MISSING_ID ),
        _countof( rgwsz ),
        rgwsz );
    OSTraceResumeGC();
}

LOCAL
VOID LGIReportBadRevertedPage( const INST* pinst, const IFMP ifmp, const PGNO pgno, const ERR err, const LGPOS& lgposMismatchLR, const CPG cpgAffected )
{
    Assert( FMP::FAllocatedFmp( ifmp ) );

    const FMP* const pfmp = FMP::FAllocatedFmp( ifmp ) ? &g_rgfmp[ ifmp ] : NULL;
    const LGPOS lgposStop = pinst->m_plog->LgposLGLogTipNoLock();

    OSTraceSuspendGC();
    const WCHAR* wszDatabaseName = pfmp ?
        pfmp->WszDatabaseName() :
        pinst->m_wszInstanceName;
    wszDatabaseName = wszDatabaseName ? wszDatabaseName : L"";
    const WCHAR* rgwsz[] =
    {
        wszDatabaseName,
        OSFormatW( L"%I32u (0x%08x)", pgno, pgno ),
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposMismatchLR.lGeneration, lgposMismatchLR.isec, lgposMismatchLR.ib ),
        OSFormatW( L"(%08I32X,%04hX,%04hX)", lgposStop.lGeneration, lgposStop.isec, lgposStop.ib ),
        OSFormatW( L"%d", err ),
        pfmp ?
            OSFormatW( L"%d", (INT)pfmp->FContainsDataFromFutureLogs() ) :
            L"?",
        OSFormatW( L"%d", cpgAffected )
    };
    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        RECOVERY_DATABASE_BAD_REVERTED_PAGE_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        pinst );
    OSUHAPublishEvent(
        HaDbFailureTagCorruption,
        pinst,
        Ese2HaId( LOGGING_RECOVERY_CATEGORY ),
        HaDbIoErrorNone,
        wszDatabaseName,
        OffsetOfPgno( pgno ),
        g_cbPage,
        Ese2HaId( RECOVERY_DATABASE_BAD_REVERTED_PAGE_ID ),
        _countof( rgwsz ),
        rgwsz );
    OSTraceResumeGC();
}

//  access page RIW latched
//  remove dependence
//
//  On success, the page is latched with latchRIW.
//  On failure, the page is not latched.
//  errSkipLogRedoOperation: the page is not latched, and the Pagetrim State is pagetrimTrimmed unless fSkipSetRedoMapDbtimeRevert is set and dbtime of page is dbtimeRevert.
//
INLINE ERR LOG::ErrLGIAccessPage(
    PIB             *ppib,
    CSR             *pcsr,
    const IFMP      ifmp,
    const PGNO      pgno,
    const OBJID     objid,
    const BOOL      fUninitPageOk,
    const BOOL      fSkipSetRedoMapDbtimeRevert )
{
    ERR err = JET_errSuccess, errPage = JET_errSuccess;
    DBTIME dbtime = 0;
    BOOL fBeyondEofPageOk = fFalse;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
    BOOL fRevertedNewPage = fFalse;

    Assert( pgnoNull != pgno );
    Assert( NULL != ppib );
    Assert( !pcsr->FLatched() );

    // Both fUninitPageOk and fSkipSetRedoMapDbtimeRevert shouldn't be true.
    Assert( !( fUninitPageOk && fSkipSetRedoMapDbtimeRevert ) );

    //  right off the bat, if the pgno for this redo operation is already
    //  tracked by the log redo map, we should just skip it instead of
    //  trying to operate on a page that we know is not up-to-date.
    if ( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgno ) )
    {
        pcsr->SetPagetrimState( pagetrimTrimmed, pgno );

        Assert( !pcsr->FLatched() );
        Error( ErrERRCheck( errSkipLogRedoOperation ) );
    }

    tcScope->SetDwEngineObjid( objid );

    errPage = pcsr->ErrGetRIWPage( ppib, ifmp, pgno );

    tcScope->SetDwEngineObjid( dwEngineObjidNone );

    if ( errPage >= JET_errSuccess )
    {
        dbtime = pcsr->Cpage().Dbtime();
        fRevertedNewPage = pcsr->Cpage().FRevertedNewPage();
    }

    //  If a shrink or trim have happened at any point in the log range we are recovering,
    //  trying to access a page would either yield JET_errFileIOBeyondEOF or
    //  JET_errPageNotInitialized if such a page was within the shrunk or trimmed range.
    //
    //  If a page was freed at any point in the log range we are recovering, and if that
    //  page was later reverted back from a future point to a new page, it will have a dbtime
    //  equal to dbtimeRevert.
    //
    //  JET_errFileIOBeyondEOF is simple to understand: the database has shrunk
    //  and a redo operation applies to a page in this area. For these operations,
    //  we must skip them but track them in the redo map.
    //
    //  JET_errPageNotInitialized is more interesting of a case. If there is a split
    //  redo operation for a shrunk range, the database will be extended to accomodate
    //  such an operation. However, all the pages in this extended range are blank,
    //  and any further redo operations on them will yield the JET_errPageNotInitialized
    //  error. We must skip these pages and also track them in the redo map.
    //
    //  Treat pages marked as shrunk similarly.
    //
    fBeyondEofPageOk = fUninitPageOk && g_rgfmp[ifmp].FOlderDemandExtendDb();
    if ( ( ( ( errPage == JET_errFileIOBeyondEOF ) && !fBeyondEofPageOk ) ||
           ( ( errPage == JET_errPageNotInitialized ) && !fUninitPageOk ) ||
           ( ( errPage >= JET_errSuccess ) && pcsr->Cpage().FShrunkPage() && !fUninitPageOk ) ||
           ( ( errPage >= JET_errSuccess ) && fRevertedNewPage && !fUninitPageOk ) ) &&
         FRedoMapNeeded( ifmp ) )
    {
        if ( errPage >= JET_errSuccess )
        {
            pcsr->ReleasePage();
        }

        // The only LR which passes fSkipSetRedoMapDbtimeRevert as true currently is the scrub LR for an unused page.
        // For that LR, we will skip redo operation if the dbtime of the page is set to dbtimeRevert.
        // We don't want to add it to redomap as nothing is going to remove it as it was freed earlier.
        //
        if ( fRevertedNewPage && fSkipSetRedoMapDbtimeRevert )
        {
            Error( ErrERRCheck( errSkipLogRedoOperation ) );
        }


        Call( g_rgfmp[ifmp].ErrEnsureLogRedoMapsAllocated() );

        CLogRedoMap* const pLogRedoMapToSet = fRevertedNewPage ? g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert() : g_rgfmp[ ifmp ].PLogRedoMapZeroed();

        Call( pLogRedoMapToSet->ErrSetPgno(
            pgno,
            LgposLGLogTipNoLock(),
            ( errPage < JET_errSuccess ) ? errPage : ErrERRCheck( JET_errRecoveryVerifyFailure ),
            dbtimeNil,
            dbtime,
            dbtimeNil ) );

        SetPendingRedoMapEntries();
        pcsr->SetPagetrimState( pagetrimTrimmed, pgno );

        Assert( !pcsr->FLatched() );

        Error( ErrERRCheck( errSkipLogRedoOperation ) );
    }
    else
    {
        if ( ( errPage >= JET_errSuccess ) && ( pcsr->Cpage().FShrunkPage() || fRevertedNewPage ) )
        {
            pcsr->ReleasePage();
            Error( ErrERRCheck( JET_errRecoveryVerifyFailure ) );
        }

        switch ( errPage )
        {
            case JET_errPageNotInitialized:
            case JET_errFileIOBeyondEOF:
            case JET_errOutOfMemory:
            case JET_errOutOfBuffers:
            case wrnBFPageFault:
            case wrnBFPageFlushPending:
            case wrnBFBadLatchHint:
            case_AllDatabaseStorageCorruptionErrs:
            case JET_errDiskIO:
            case JET_errDiskFull:
                break;
            default:
                CallS( errPage );
        }
    }

    err = errPage;

HandleError:

    if ( errSkipLogRedoOperation != err &&
        err < JET_errSuccess )
    {
        if ( ( ( err == JET_errFileIOBeyondEOF ) && !fBeyondEofPageOk ) ||
             ( ( err == JET_errPageNotInitialized ) && !fUninitPageOk ) ||
             ( ( err == JET_errRecoveryVerifyFailure ) && !fUninitPageOk ) )
        {
            if ( fRevertedNewPage )
            {
                LGIReportBadRevertedPage( m_pinst, ifmp, pgno, err, LgposLGLogTipNoLock(), 1 );
            }
            else
            {
                LGIReportPageDataMissing( m_pinst, ifmp, pgno, err, LgposLGLogTipNoLock(), 1 );
            }
        }
        else if ( ( err != JET_errFileIOBeyondEOF ) &&
                  ( err != JET_errPageNotInitialized ) &&
                  ( err != JET_errRecoveryVerifyFailure ) )
        {
            LGIReportEventOfReadError( m_pinst, ifmp, pgno, err, dbtime );
        }
    }

    return err;
}

//  ================================================================
ERR LOG::ErrLGIAccessPageCheckDbtimes(
    _In_    PIB             * const ppib,
    _In_    CSR             * const pcsr,
            const IFMP      ifmp,
            const PGNO      pgno,
            const OBJID     objid,
            const DBTIME    dbtimeBefore,
            const DBTIME    dbtimeAfter,
    _Out_   BOOL * const    pfRedoRequired )
//  ================================================================
//
// Given a page and its dbtimeBefore/After latch the page and determine
// if redo is required. If redo is required the page is write latched,
// otherwise it is RIW latched.
//
//-
{
    Assert( ppib );
    Assert( pcsr );
    Assert( ifmpNil != ifmp );
    Assert( pgnoNull != pgno );
    Assert( dbtimeNil != dbtimeBefore );
    Assert( dbtimeInvalid != dbtimeBefore );
    Assert( dbtimeNil != dbtimeAfter );
    Assert( dbtimeInvalid != dbtimeAfter );
    Assert( dbtimeBefore < dbtimeAfter );
    Assert( pfRedoRequired );
    
    ERR err;

    *pfRedoRequired = fFalse;
    
    Call( ErrLGIAccessPage( ppib, pcsr, ifmp, pgno, objid, fFalse ) );

    *pfRedoRequired = FLGNeedRedoCheckDbtimeBefore( ifmp, *pcsr, dbtimeAfter, dbtimeBefore, &err );
    Call( err );

    if( *pfRedoRequired )
    {
        pcsr->UpgradeFromRIWLatch();
    }
    
HandleError:
    if( err >= JET_errSuccess )
    {
        if( *pfRedoRequired )
        {
            Assert( latchWrite == pcsr->Latch() );
        }
        else
        {
            Assert( latchRIW == pcsr->Latch() );
        }
    }
    else
    {
        if ( errSkipLogRedoOperation == err )
        {
            // But it's still a success...
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgno ) );

            err = JET_errSuccess;
        }
        pcsr->ReleasePage();
        Assert( !pcsr->FLatched() );
    }

    return err;
}


//  report flush-order dependency violation
//
LOCAL ERR ErrLGRIReportFlushDependencyCorrupted(
    const IFMP      ifmp,
    const PGNO      pgnoFlushFirst,
    const PGNO      pgnoFlushSecond )
{
    WCHAR           szPgnoFlushFirst[32];
    WCHAR           szPgnoFlushSecond[32];
    WCHAR           szErr[16];
    const WCHAR *   rgszT[]     = { g_rgfmp[ifmp].WszDatabaseName(), szPgnoFlushFirst, szPgnoFlushSecond, szErr };
    
    OSStrCbFormatW( szPgnoFlushFirst, sizeof(szPgnoFlushFirst), L"%u (0x%08x)", pgnoFlushFirst, pgnoFlushFirst );
    OSStrCbFormatW( szPgnoFlushSecond, sizeof(szPgnoFlushSecond), L"%u (0x%08x)", pgnoFlushSecond, pgnoFlushSecond );
    OSStrCbFormatW( szErr, sizeof(szErr), L"%d", JET_errDatabaseBufferDependenciesCorrupted );

    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        FLUSH_DEPENDENCY_CORRUPTED_ID,
        _countof( rgszT ),
        rgszT,
        0,
        NULL,
        g_rgfmp[ifmp].Pinst() );

    return ErrERRCheck( JET_errDatabaseBufferDependenciesCorrupted );
}

//  retrieves new page from database
//
ERR LOG::ErrLGRIAccessNewPage(
    PIB *               ppib,
    CSR *               pcsrNew,
    const IFMP          ifmp,
    const PGNO          pgnoNew,
    const OBJID         objid,
    const DBTIME        dbtime,
    BOOL *              pfRedoNewPage )
{
    //  if database could contain updates from future logs
    //      access new page
    //      if page exists
    //          if page's dbtime < dbtime of oper
    //              release page
    //              get new page
    //      else
    //          get new page
    //  else
    //      get new page
    //

    //  assume new page needs to be redone
    *pfRedoNewPage = fTrue;

    if ( g_rgfmp[ ifmp ].FContainsDataFromFutureLogs() )
    {
        ERR err = ErrLGIAccessPage( ppib, pcsrNew, ifmp, pgnoNew, objid, fTrue );

        if ( errSkipLogRedoOperation == err )
        {
            err = JET_errSuccess;

            Assert( pagetrimTrimmed == pcsrNew->PagetrimState() );
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgnoNew ) );

            *pfRedoNewPage = fFalse;

            return err;
        }

        if ( err >= 0 )
        {
            //  get new page if page is older than operation
            //
            Assert( latchRIW == pcsrNew->Latch() );

            *pfRedoNewPage = FLGNeedRedoPage( *pcsrNew, dbtime );
            if ( *pfRedoNewPage )
            {
                pcsrNew->ReleasePage();
            }
        }
    }

    Assert( !( *pfRedoNewPage ) || !pcsrNew->FLatched() );

    return JET_errSuccess;
}


ERR LOG::ErrLGRIPpibFromProcid( PROCID procid, PIB **pppib )
{
    ERR     err     = JET_errSuccess;

    Assert( procidNil != procid );
    Assert( procidReadDuringRecovery != procid );
    AssertSzRTL( NULL != m_pcsessionhash, "We should not see any LRs referring to a transaction after Term and before Init" );

    if ( m_pcsessionhash == NULL )
    {
        Error( ErrERRCheck( JET_errLogCorrupted ) );
    }

    *pppib = m_pcsessionhash->PpibHashFind( procid );

    //  if we didn't find it, begin a new session for the requested procid

    if ( *pppib == ppibNil )
    {
        Call( ErrPIBBeginSession( m_pinst, pppib, procid, fFalse ) );
        Assert( procid == (*pppib)->procid );
        m_pcsessionhash->InsertPib( *pppib );
    }

HandleError:
    return err;
}


//  creates new fucb with given criteria
//  links fucb to hash table
//
LOCAL ERR ErrLGRICreateFucb(
    PIB         *ppib,
    const IFMP  ifmp,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const BOOL  fUnique,
    const BOOL  fSpace,
    FUCB        **ppfucb )
{
    ERR         err = JET_errSuccess;
    FUCB        *pfucb  = pfucbNil;
    FCB         *pfcb = pfcbNil;
    BOOL        fState;
    ULONG       cRetries = 0;
    BOOL        fCreatedNewFCB = fFalse;

    //  create fucb
    //
    Call( ErrFUCBOpen( ppib, ifmp, &pfucb ) );
    pfucb->pvtfndef = &vtfndefIsam;

    //  set ifmp
    //
    pfucb->ifmp = ifmp;

    Assert( pfcbNil == pfucb->u.pfcb );

    Assert( PinstFromIfmp( ifmp )->m_isdlInit.FActiveSequence() );

Restart:
    AssertTrack( cRetries != 100000, "StalledCreateFucbRetries" );

    //  get fcb for table, if one exists
    //
    pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState, fTrue /* FIncrementRefCount */, fTrue /* fInitForRecovery */);
    Assert( pfcbNil == pfcb || fFCBStateInitialized == fState );
    if ( pfcbNil == pfcb )
    {
        //  there exists no fcb for FDP
        //      allocate new fcb as a regular table FCB
        //      and set up for FUCB
        //
        err = FCB::ErrCreate( ppib, ifmp, pgnoFDP, &pfcb );
        if ( err == errFCBExists )
        {
            err = JET_errSuccess;
            UtilSleep( 10 );
            cRetries++;
            goto Restart;
        }
        Call( err );
        fCreatedNewFCB = fTrue;

        Assert( pfcb != pfcbNil );
        Assert( pfcb->IsLocked() );
        Assert( pfcb->WRefCount() == 0 );
        Assert( !pfcb->FInLRU() );
        Assert( !pfcb->UlFlags() );

        pfcb->SetInitedForRecovery();

        Assert( objidNil == pfcb->ObjidFDP() );
        pfcb->SetObjidFDP( objidFDP );

        if ( pgnoSystemRoot == pgnoFDP )
        {
            pfcb->SetTypeDatabase();
        }
        else
        {
            pfcb->SetTypeTable();
        }

        //  FCB always initialised as unique, though
        //  this flag is ignored during recovery
        //  (uniqueness is strictly determined by
        //  FUCB's flags)
        Assert( pfcb->FUnique() );      // FCB always initialised as unique
        if ( !fUnique )
            pfcb->SetNonUnique();

        pfcb->Unlock();

        //  link in the FUCB to new FCB
        //
        Call( pfcb->ErrLink( pfucb ) );

        //  insert the FCB into the global list

        pfcb->InsertList();

        Assert( pfcb->WRefCount() >= 1 );

        //  complete the creation of this FCB
        pfcb->Lock();
        pfcb->CreateComplete();
        pfcb->Unlock();
    }
    else
    {
        //  link in the FUCB to new FCB
        //
        err = pfcb->ErrLink( pfucb );
        //  release FCB for the PfcbFCBGet() call
        //
        Assert( pfcb->WRefCount() > 1 );
        pfcb->Release();
        Call( err );
    }

    //  set unique-ness in FUCB if requested
    //
    Assert( !FFUCBSpace( pfucb ) );
    Assert( ( fUnique && FFUCBUnique( pfucb ) )
        || ( !fUnique && !FFUCBUnique( pfucb ) ) );

    if ( fSpace )
    {
        FUCBSetOwnExt( pfucb );
    }

    pfucb->dataSearchKey.Nullify();
    pfucb->cColumnsInSearchKey = 0;
    KSReset( pfucb );

    *ppfucb = pfucb;

HandleError:

    if ( err < 0 )
    {
        if ( pfucbNil != pfucb )
        {
            if ( pfcb == pfucb->u.pfcb )
            {
                // We managed to link the FUCB to the FCB before we errored.
                pfcb->Unlink( pfucb );
            }

            //  close the FUCB
            FUCBClose( pfucb );
        }

        if ( fCreatedNewFCB )
        {
            //  synchronously purge the FCB we created but didn't use.
            pfcb->PrepareForPurge( fFalse );
            pfcb->Purge( fFalse );
        }
    }

    return err;
}



//  closes all cursors created on database during recovery by given session
//  releases corresponding table handles
//
VOID LGRIPurgeFucbs( PIB * ppib, const IFMP ifmp, CTableHash * pctablehash )
{
    Assert( NULL != pctablehash );

    FUCB    *pfucb = ppib->pfucbOfSession;
    FUCB    *pfucbNext;

    for ( ; pfucb != pfucbNil; pfucb = pfucbNext )
    {
        pfucbNext = pfucb->pfucbNextOfSession;

        if ( ifmp == pfucb->ifmp &&
             pfucb->ppib->procid == ppib->procid )
        {
            Assert( pfucb->ppib == ppib );
            Assert( pfucb == pctablehash->PfucbHashFind(
                                                ifmp,
                                                PgnoFDP( pfucb ),
                                                ppib->procid,
                                                FFUCBSpace( pfucb ) ) );

            Assert( pfucb->fInRecoveryTableHash );
            pctablehash->RemoveFucb( pfucb );
            pfucb->fInRecoveryTableHash = fFalse;

            //  unlink and close fucb
            //
            pfucb->u.pfcb->Unlink( pfucb );
            FUCBClose( pfucb );
        }
    }
}

//  releases all references to this table
//
LOCAL ERR ErrLGRIPurgeFcbs( const IFMP ifmp, const PGNO pgnoFDP, FDPTYPE fFDPType, CTableHash * pctablehash )
{
    FCB *pfcbTable = NULL;
    BOOL fDeleteTable = fFalse;
    ERR err = JET_errSuccess;

    Assert( NULL != pctablehash );

    BOOL    fState;
    FCB *   pfcb    = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState );

    if ( pfcbNil == pfcb )
    {
        return JET_errSuccess;
    }

    if ( pfcb->FTypeTable() )
    {
        pfcbTable = pfcb;
        fDeleteTable = fTrue;
    }
    else if ( pfcb->FTypeLV() )
    {
        Assert( fFDPType == fFDPTypeUnknown );
        pfcbTable = pfcb->PfcbTable();
        fDeleteTable = fTrue;
    }
    else if ( pfcb->FTypeSecondaryIndex() )
    {
        Assert( fFDPType != fFDPTypeTable );
        pfcbTable = pfcb->PfcbTable();
        if ( fFDPType == fFDPTypeUnknown )
        {
            // Found a secondary index FCB that got reused, figure out if the whole table has been deleted, ok to do
            // logical calls to catalog here because we obviously have a fully hydrated secondary index here.
            PIB *ppib = ppibNil;
            PGNO pgnoLookup;
            CallR( ErrPIBBeginSession( PinstFromIfmp( pfcbTable->Ifmp() ), &ppib,procidNil, fFalse ) );
            DBSetOpenDatabaseFlag( ppib, ifmp );
            if ( ErrCATSeekTableByObjid( ppib, ifmp, pfcbTable->ObjidFDP(), NULL, 0, &pgnoLookup ) >= JET_errSuccess &&
                 pgnoLookup == pfcbTable->PgnoFDP() )
            {
                fDeleteTable = fFalse;
            }
            else
            {
                fDeleteTable = fTrue;
            }
            PIBEndSession( ppib );
        }
    }
    else
    {
        Assert( fFalse );
    }

    if ( fDeleteTable )
    {
        if ( pfcb != pfcbTable )
        {
            pfcbTable->IncrementRefCount();
            pfcb->Release();
        }

        // Close all recovery cursors on this FCB
        //
        for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            pctablehash->PurgeTablesForFCB( pfcbT );
        }
        if ( pfcbTable->Ptdb() != ptdbNil )
        {
            FCB     * const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
            if ( pfcbNil != pfcbLV )
            {
                pctablehash->PurgeTablesForFCB( pfcbLV );
            }
        }

        if ( pfcbTable->Ptdb() != ptdbNil )
        {
            // Remove from catalog hash so no user session will open this table any more - not needed right now
            // because catalog hash is disabled during recovery
            Assert( !FCATHashActive( PinstFromIfmp( pfcbTable->Ifmp() ) ) );
            /*
            CHAR szTable[JET_cbNameMost+1];
            pfcbTable->EnterDML();
            OSStrCbCopyA( szTable, sizeof(szTable), pfcbTable->Ptdb()->SzTableName() );
            pfcbTable->LeaveDML();
            CATHashDelete( pfcbTable, szTable );
            */

            // Setting delete pending/committed.
            pfcbTable->EnterDDL();

            for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
            {
                BOOL fHasFUCB;
                Assert( pfcbT->Ifmp() == ifmp );
                Assert( pfcbT == pfcbTable
                        || ( pfcbT->FTypeSecondaryIndex()
                             && pfcbT->PfcbTable() == pfcbTable ) );

                pfcbT->Lock();
                pfcbT->SetDeletePending();
                pfcbT->SetDeleteCommitted();
                fHasFUCB = ( NULL != pfcbT->Pfucb() );
                pfcbT->Unlock();
                
                // If any user cursors are still open, we have to wait for them to close (cannot force close)
                while ( fHasFUCB )
                {
                    pfcbTable->LeaveDDL();
                    UtilSleep( cmsecWaitGeneric );
                    pfcbTable->EnterDDL();

                    pfcbT->Lock();
                    fHasFUCB = ( NULL != pfcbT->Pfucb() );
                    pfcbT->Unlock();
                }
            }
            if ( pfcbTable->Ptdb() != ptdbNil )
            {
                FCB     * const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
                if ( pfcbNil != pfcbLV )
                {
                    BOOL fHasFUCB;

                    pfcbLV->Lock();
                    pfcbLV->SetDeletePending();
                    pfcbLV->SetDeleteCommitted();
                    fHasFUCB = (NULL != pfcbLV->Pfucb() );
                    pfcbLV->Unlock();

                    // If any user cursors are still open, we have to wait for them to close (cannot force close)
                    while ( fHasFUCB )
                    {
                        pfcbTable->LeaveDDL();
                        UtilSleep( cmsecWaitGeneric );
                        pfcbTable->EnterDDL();

                        pfcbLV->Lock();
                        fHasFUCB = (NULL != pfcbLV->Pfucb() );
                        pfcbLV->Unlock();
                    }
                }
            }
            pfcbTable->LeaveDDL();
        }
        else
        {
            pfcbTable->Lock();
            pfcbTable->SetDeletePending();
            pfcbTable->SetDeleteCommitted();
            pfcbTable->Unlock();
        }

        //  Purge the FCB tree
        //
        for ( FCB *pfcbT = pfcbTable; pfcbT != pfcbNil; pfcbT = pfcbT->PfcbNextIndex() )
        {
            VERNullifyAllVersionsOnFCB( pfcbT );
            pfcbT->PrepareForPurge( fFalse );
        }
        if ( pfcbTable->Ptdb() != ptdbNil )
        {
            FCB     * const pfcbLV = pfcbTable->Ptdb()->PfcbLV();
            if ( pfcbNil != pfcbLV )
            {
                VERNullifyAllVersionsOnFCB( pfcbLV );
                pfcbLV->PrepareForPurge( fFalse );
            }
        }
        Assert( pfcbTable->PrceOldest() == prceNil );
        Assert( 1 == pfcbTable->WRefCount() );
        pfcbTable->Release();
        pfcbTable->Purge();
    }
    else
    {
        Assert( pfcbTable->FTypeTable() );
        Assert( pfcb->FTypeSecondaryIndex() );
        Assert( pfcb != pfcbTable );
        Assert( pfcb->PfcbTable() == pfcbTable );

        // Close all recovery cursors on this FCB
        //
        pctablehash->PurgeTablesForFCB( pfcb );

        pfcbTable->EnterDDL();

        pfcb->Pidb()->SetFDeleted();

        pfcb->Lock();
        pfcb->SetDeletePending();
        pfcb->SetDeleteCommitted();
        pfcb->Unlock();

        //      update all index mask
        FILESetAllIndexMask( pfcbTable );

        pfcbTable->LeaveDDL();

        pfcbTable->SetIndexing();
        pfcbTable->EnterDDL();

        pfcbTable->UnlinkSecondaryIndex( pfcb );

        pfcbTable->LeaveDDL();
        pfcbTable->ResetIndexing();

        VERNullifyAllVersionsOnFCB( pfcb );
        pfcb->PrepareForPurge( fFalse );

        Assert( pfcb->PrceOldest() == prceNil );
        Assert( 1 == pfcb->WRefCount() );
        pfcb->Release();
        pfcb->Purge();
    }

    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState ) == pfcbNil );

    return JET_errSuccess;
}

//  releases and closes all tables in hash for this FCB
//
VOID CTableHash::PurgeTablesForFCB( const FCB * pfcb )
{
    FUCB *  pfucbNext;
    for ( ULONG i = 0; i < CEntries(); i++ )
    {
        for ( FUCB * pfucb = PentryOfHash( i ); pfucbNil != pfucb; pfucb = pfucbNext )
        {
            Assert( ppibNil != pfucb->ppib );
            pfucbNext = pfucb->pfucbHashOverflow;

            if ( pfucb->u.pfcb == pfcb )
            {
                Assert( pfucb->fInRecoveryTableHash );
                RemoveFucb( pfucb );
                pfucb->fInRecoveryTableHash = fFalse;

                //  unlink and close fucb
                //
                pfucb->u.pfcb->Unlink( pfucb );
                FUCBClose( pfucb );
            }
        }
    }
}


//  releases and closes all unversioned tables in hash
//
VOID CTableHash::PurgeUnversionedTables()
{
    FUCB *  pfucbNext;
    for ( ULONG i = 0; i < CEntries(); i++ )
    {
        for ( FUCB * pfucb = PentryOfHash( i ); pfucbNil != pfucb; pfucb = pfucbNext )
        {
            Assert( ppibNil != pfucb->ppib );
            pfucbNext = pfucb->pfucbHashOverflow;

            if ( !FFUCBVersioned( pfucb ) )
            {
                Assert( pfucb->fInRecoveryTableHash );
                RemoveFucb( pfucb );
                pfucb->fInRecoveryTableHash = fFalse;

                //  unlink and close fucb
                //
                pfucb->u.pfcb->Unlink( pfucb );
                FUCBClose( pfucb );
            }
        }
    }
}


//  Returns pfucb for given pib and FDP.
//
//  PARAMETERS      ppib    pib of session being redone
//                  fdp     FDP page for logged page
//                  ppfucb  out FUCB for open table for logged page
//
//  RETURNS         JET_errSuccess or error from called routine
//

LOCAL ERR ErrLGRIGetFucb(
    CTableHash  *pctablehash,
    PIB         *ppib,
    const IFMP  ifmp,
    const PGNO  pgnoFDP,
    const OBJID objidFDP,
    const BOOL  fUnique,
    const BOOL  fSpace,
    FUCB        **ppfucb )
{
    FUCB        * pfucb     = pctablehash->PfucbHashFind( ifmp, pgnoFDP, ppib->procid, fSpace );

    Assert( pctablehash != NULL );

    //  allocate an all-purpose fucb for this table, if not already allocated
    //
    if ( NULL == pfucb )
    {
        //  fucb not created
        //
        ERR err = ErrLGRICreateFucb(
                            ppib,
                            ifmp,
                            pgnoFDP,
                            objidFDP,
                            fUnique,
                            fSpace,
                            &pfucb );
        Assert( errFCBAboveThreshold != err );
        Assert( errFCBTooManyOpen != err );

        if ( JET_errTooManyOpenTables == err
            || JET_errOutOfCursors == err )
        {
            //  release tables without uncommitted versions and retry
            //
            pctablehash->PurgeUnversionedTables();
            err = ErrLGRICreateFucb(
                            ppib,
                            ifmp,
                            pgnoFDP,
                            objidFDP,
                            fUnique,
                            fSpace,
                            &pfucb );
        }

        CallR( err );

        Assert( pfucbNil != pfucb );
        pfucb->fInRecoveryTableHash = fTrue;
        pctablehash->InsertFucb( pfucb );
    }

    pfucb->bmCurr.Nullify();

    //  reset copy buffer and flags
    //
    Assert( !FFUCBDeferredChecksum( pfucb ) );
    Assert( !FFUCBUpdateSeparateLV( pfucb ) );
    FUCBResetUpdateFlags( pfucb );

    Assert( pfucb->ppib == ppib );

#ifdef DEBUG
    if ( fSpace )
    {
        Assert( FFUCBUnique( pfucb ) );
        Assert( FFUCBSpace( pfucb ) );
    }
    else
    {
        Assert( !FFUCBSpace( pfucb ) );
        if ( fUnique )
        {
            Assert( FFUCBUnique( pfucb ) );
        }
        else
        {
            Assert( !FFUCBUnique( pfucb ) );
        }
    }
#endif

    *ppfucb = pfucb;
    return JET_errSuccess;
}

VOID LOG::LGRRemoveFucb( FUCB * pfucb )
{
    Assert( pfucb->fInRecoveryTableHash );
    if ( NULL != m_pctablehash )
    {
        m_pctablehash->RemoveFucb( pfucb );
        pfucb->fInRecoveryTableHash = fFalse;
    }
}


ERR LOG::ErrLGRIInitSession(
    DBMS_PARAM              *pdbms_param,
    BYTE                    *pbAttach,
    const REDOATTACHMODE    redoattachmode )
{
    ERR                     err             = JET_errSuccess;
    DBID                    dbid;

    /*  set log stored db environment
    /**/
    Assert( NULL != pdbms_param );
    m_pinst->RestoreDBMSParams( pdbms_param );

    CallR( ErrITSetConstants( m_pinst ) );

    CallR( m_pinst->ErrINSTInit() );

    //  allocate and initialize global hash for table and session handles
    //
    if ( NULL == m_pctablehash )
    {
        Alloc( m_pctablehash = new CTableHash( pdbms_param->le_lCursorsMax ) );
        Call( m_pctablehash->ErrInit() );
    }

    if ( NULL == m_pcsessionhash )
    {
        Alloc( m_pcsessionhash = new CSessionHash( pdbms_param->le_lSessionsMax ) );
        Call( m_pcsessionhash->ErrInit() );
    }

    //  restore the attached dbs
    Assert( pbAttach );
    if ( redoattachmodeInitBeforeRedo == redoattachmode )
    {
        // We will initialize the revert snapshot from Rstmap during LGRIInitSession.
        // Also, check if we need to roll the snapshot and roll it if required.
        Call( CRevertSnapshotForAttachedDbs::ErrRBSInitFromRstmap( m_pinst ) );

        // If required range was a problem or if db's are not on the required efv m_prbs would be null in ErrRBSInitFromRstmap
        if ( m_pinst->m_prbs && m_pinst->m_prbs->FRollSnapshot() )
        {
            Call( m_pinst->m_prbs->ErrRollSnapshot( fTrue, fTrue ) );
        }

        err = ErrLGLoadFMPFromAttachments( pbAttach );
        CallS( err );
        Call( err );

        /*  Make sure all the attached database are consistent!
         */
        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP        ifmp        = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            FMP         *pfmp       = &g_rgfmp[ifmp];
            WCHAR       *wszDbName;
            REDOATTACH  redoattach;

            if ( !pfmp->FInUse() || !pfmp->Patchchk() )
                continue;

            wszDbName = pfmp->WszDatabaseName();
            Assert ( wszDbName );

            Assert( redoattachmodeInitBeforeRedo == redoattachmode );

            // if we have a map (m_irstmapMac check) 
            // and we are not replaying what is NOT in the map (m_fReplayMissingMapEntryDB)
            // then we should marked the missing one as Skipped
            //
            // SOFT_HARD: not needed, irstmapMac should do it
            if ( ( m_fHardRestore || m_irstmapMac > 0 ) && !m_fReplayMissingMapEntryDB )
            {
                if ( 0 > IrstmapSearchNewName( wszDbName ) )
                {
                    /*  not in the restore map, set to skip it.
                     */
                    Assert( pfmp->Pdbfilehdr() == NULL );
                    pfmp->SetSkippedAttach();
                }
            }

            if ( pfmp->FSkippedAttach() )
            {
                err = JET_errSuccess;
                continue;
            }
            

            Assert( !pfmp->FReadOnlyAttach() );
            Call( ErrLGRICheckAttachedDb(
                        ifmp,
                        &m_signLog,
                        &redoattach,
                        redoattachmode ) );
            Assert( NULL != pfmp->Pdbfilehdr() );

            switch ( redoattach )
            {
                case redoattachNow:
                    Assert( !pfmp->FReadOnlyAttach() );

                    Call( ErrLGRIRedoAttachDb(
                                ifmp,
                                pfmp->Patchchk()->CpgDatabaseSizeMax(),
                                pfmp->Patchchk()->FSparseEnabledFile(),
                                redoattachmode ) );
                    break;

                case redoattachCreate:
                default:
                    Assert( fFalse );   //  should be impossible, but as a firewall, set to defer the attachment
                case redoattachDefer:
                case redoattachDeferConsistentFuture:
                case redoattachDeferAccessDenied:
                    Assert( !pfmp->FReadOnlyAttach() );
                    LGRISetDeferredAttachment( ifmp, (redoattach == redoattachDeferConsistentFuture), (redoattach == redoattachDeferAccessDenied) );
                    break;
            }

            /* keep attachment info and update it. */
            Assert( pfmp->Patchchk() != NULL );
        }
    }
    else if ( redoattachmodeInitLR == redoattachmode )
    {
        // Implicitly reattaching keep-alive databases, allow header updates again
        for ( dbid = dbidMin; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            if ( BoolParam( m_pinst, JET_paramFlight_EnableReattachRaceBugFix ) )
            {
                FMP * const pfmp = &g_rgfmp[ ifmp ];
                pfmp->RwlDetaching().EnterAsWriter();
                pfmp->SetAllowHeaderUpdate();
                pfmp->RwlDetaching().LeaveAsWriter();
            }
        }
    }

    Assert( err >= JET_errSuccess );
    return err;

HandleError:
    Assert( err < JET_errSuccess );

    //  disable checkpoint, because we may not have been able to
    //  completely/accurately build list of attachments
    //
    LGDisableCheckpoint();

    // clean up instance resources allocated in ErrINSTInit
    m_fLogDisabledDueToRecoveryFailure = fTrue;
    (void)m_pinst->ErrINSTTerm( termtypeError );
    m_fLogDisabledDueToRecoveryFailure = fFalse;

    delete m_pctablehash;
    m_pctablehash = NULL;

    delete m_pcsessionhash;
    m_pcsessionhash = NULL;

    return err;
}


ERR ErrLGICheckDatabaseFileSize( PIB *ppib, IFMP ifmp )
{
    ERR     err         = JET_errSuccess;
    BOOL    fAdjSize    = fFalse;
    FMP     *pfmp       = g_rgfmp + ifmp;

    if ( pfmp->Pdbfilehdr()->Dbstate() != JET_dbstateDirtyShutdown )
    {
        //  When we're in the patched or just-created state we can not adjust the size of the database
        //  because we may not have a consistent Root OE tree due to a inc-reseed V2
        //  or passive page patching updating only one page of the larger tree during
        //  the patch operation ... this means we must replay through the max gen
        //  required in order to trust the OE tree, and until we play through the max
        //  gen required the database will be in the patched state so we can 
        //  distinguish when it is safe to walk the OE tree and resize the database.
        fAdjSize = fFalse;
    }
    else
    {
        err = ErrDBSetLastPage( ppib, ifmp );
        fAdjSize = fTrue;
    }
    if ( JET_errFileNotFound == err )
    {
        //  UNDONE: The file should be there. Put this code to get around
        //  UNDONE: such that DS database file that was not detached can
        //  UNDONE: continue recovering.
        const WCHAR *rgszT[1];
        rgszT[0] = pfmp->WszDatabaseName();
        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                FILE_NOT_FOUND_ERROR_ID,
                1,
                rgszT,
                0,
                NULL,
                PinstFromPpib( ppib ) );
    }
    else if( err >= JET_errSuccess )
    {
        CallS( err );

        //  the current file size does not match what the FMP (and OwnExt) say
        //  it should be

        Assert( 0 != pfmp->CbOwnedFileSize() );

        QWORD cbFileSize;
        CallR( pfmp->Pfapi()->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
        if ( pfmp->CbOwnedFileSize() != cbFileSize )
        {
            //  flush/purge the database before adjusting the file size to match the
            //  space trees to ensure that we don't have any pages cached for the area
            //  to be affected
            //
            //  NOTE:  if we are using a memory mapped cache and we need to shrink
            //  the file then we will purge the entire database anyway.  we must do
            //  this or the file shrink operation will fail.
            // 
            //  We also have to stop database scanning so that it won't read any pages
            //  while the shrink is happening. We depend on another attach to restart
            //  the scan.
            //

            // allow waypoint advancement to the current gen
            pfmp->SetNoWaypointLatency();
            CallR( ErrBFFlush( ifmp ) );
            pfmp->ResetNoWaypointLatency();

            QWORD qwMinDb = min( pfmp->CbOwnedFileSize(), cbFileSize );
            QWORD qwMaxDb = max( pfmp->CbOwnedFileSize(), cbFileSize );
            PGNO pgnoMinDb = PgnoOfOffset( qwMinDb );
            PGNO pgnoMaxDb = PgnoOfOffset( qwMaxDb );

            if (    BoolParam( JET_paramEnableViewCache ) &&
                    pfmp->CbOwnedFileSize() < cbFileSize )
            {
                pgnoMinDb = pgnoNull;
            }
            
            if ( g_rgfmp[ifmp].PdbmFollower() )
            {
                g_rgfmp[ifmp].PdbmFollower()->DeRegisterFollower( &g_rgfmp[ifmp], CDBMScanFollower::dbmdrrStoppedScan );
                g_rgfmp[ifmp].DestroyDBMScanFollower();
            }
            g_rgfmp[ifmp].StopDBMScan();

            BFPurge( ifmp, pgnoMinDb, pgnoMaxDb - pgnoMinDb );

            if ( fAdjSize )
            {
                //  set file size to what the FMP (and OwnExt) says it should be.
                err = ErrIONewSize( ifmp, TcCurr(), pfmp->PgnoLast(), 0, JET_bitNil );
            }
        }
    }

    return err;
}

LOCAL VOID LGICleanupTransactionToLevel0( PIB * const ppib, CTableHash * pctablehash )
{
    const ULONG ulStartCleanCursorsThreshold    = 64;       //  number of cursors open at which point we should start freeing some
    const ULONG ulStopCleanCursorsThreshold     = 16;       //  minimum number of cursors to leave opened
    const BOOL  fCleanupCursors                 = ( ppib->cCursors > ulStartCleanCursorsThreshold );
    DWORD_PTR   cStaleCursors                   = 0;
    FUCB *      pfucbPrev                       = pfucbNil;

    //  there should be no more RCEs
    Assert( 0 == ppib->Level() );
    Assert( prceNil == ppib->prceNewest );

    //  UNDONE: Is it definitely safe to traverse the cursor
    //  list without grabbing ppib->critCursors?  Even though
    //  this code path is only taken on recovery and therefore
    //  it is single-threaded, I'm a little worried there might
    //  be some case where an FUCB gets freed as part of version
    //  cleanup of some operation.
    //
    for ( FUCB * pfucb = ppib->pfucbOfSession; pfucbNil != pfucb; )
    {
        FUCB * const    pfucbNext   = pfucb->pfucbNextOfSession;

        Assert( pfucb->ppib == ppib );

        if ( fCleanupCursors
            && !FFUCBVersioned( pfucb )
            && ++cStaleCursors > ulStopCleanCursorsThreshold )
        {
            //  accumulating too many cursors for this session,
            //  so free up ones at the tail end of the list
            //  that don't appear to have been used (ie. no
            //  versioned operations)
            //
            Assert( pfucb->fInRecoveryTableHash );
            pctablehash->RemoveFucb( pfucb );
            pfucb->fInRecoveryTableHash = fFalse;

            //  unlink and close fucb
            //
            pfucb->u.pfcb->Unlink( pfucb );
            FUCBClose( pfucb, pfucbPrev );
        }
        else
        {
            FUCBResetVersioned( pfucb );
            pfucbPrev = pfucb;
        }

        pfucb = pfucbNext;
    }

    ppib->trxBegin0     = trxMax;
    ppib->lgposStart    = lgposMax;
    ppib->ResetFBegin0Logged();
    VERSignalCleanup( ppib );

    //  empty the list of RCEs
    ppib->RemoveAllDeferredRceid();
    ppib->RemoveAllRceid();
    ppib->ErrSetClientCommitContextGeneric( NULL, 0 );
    ppib->SetFCommitContextNeedPreCommitCallback( fFalse );
}

ERR LOG::ErrLGEndAllSessionsMacro( BOOL fLogEndMacro )
{
    ERR     err         = JET_errSuccess;
    PIB     *ppib       = NULL;

    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
    {
        CallR( ppib->ErrAbortAllMacros( fLogEndMacro ) );
    }

    return JET_errSuccess;
}

ERR LOG::ErrLGRIEndAllSessionsWithError()
{
    ERR     err;
    LGPOS lgposWriteTip;
    PIB * ppib = ppibNil;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );

    for (DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( NULL != pfmp->Pdbfilehdr() &&
             !pfmp->FContainsDataFromFutureLogs() )
        {
            //  fire callback for the dbs to be detached
            (void)ErrLGDbDetachingCallback( m_pinst, pfmp );
        }
    }

    CallR( ErrLGEndAllSessionsMacro( fFalse /* fLogEndMacro */ ) );

    (VOID) m_pinst->m_pver->ErrVERRCEClean();

    delete m_pctablehash;
    m_pctablehash = NULL;

    delete m_pcsessionhash;
    m_pcsessionhash = NULL;

    //  checkpoint updating will use m_lgposToFlush as
    //  the starting point for the checkpoint, so
    //  set it to the oldest outstanding transaction
    //
    lgposWriteTip = m_lgposRedo;
    m_pinst->m_critPIB.Enter();
    for ( PIB * ppibT = m_pinst->m_ppibGlobal; ppibT != NULL; ppibT = ppibT->ppibNext )
    {
        if ( ppibT->FAfterFirstBT()             //  session active
            && ppibT->Level() > 0                   //  open transaction
            && CmpLgpos( &ppibT->lgposStart, &lgposWriteTip ) < 0 )
        {
            lgposWriteTip = ppibT->lgposStart;
        }
    }
    m_pinst->m_critPIB.Leave();
    m_pLogWriteBuffer->SetLgposWriteTip( lgposWriteTip );

        //  remove all deferred RCEs and inserted RCEs for any session
    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
    {
        ppib->RemoveAllDeferredRceid();
        ppib->RemoveAllRceid();
    }

    //  term with checkpoint updates
    //
    m_fLGFMPLoaded = fTrue;
    const ERR errT = m_pinst->ErrINSTTerm( termtypeError );
    //  ErrINSTTerm may have already been called before in ErrLGRIEndAllSessions
    //  then re-entering ErrINSTTerm would get JET_errNotInitialized
    Assert( errT == JET_errSuccess || m_pinst->m_fTermAbruptly || errT == JET_errNotInitialized );
    m_fLGFMPLoaded = fFalse;

    return err;
}

LOCAL VOID LGReportAttachedDbMismatch(
    const INST * const  pinst,
    const WCHAR * const szDbName,
    const BOOL          fEndOfRecovery )
{
    WCHAR               szErrT[16];
    const WCHAR *       rgszT[2]        = { szErrT, szDbName };

    Assert( NULL != szDbName );
    Assert( LOSStrLengthW( szDbName ) > 0 );

    OSStrCbFormatW( szErrT, sizeof(szErrT), L"%d", JET_errAttachedDatabaseMismatch );

    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        ( fEndOfRecovery ?
                ATTACHED_DB_MISMATCH_END_OF_RECOVERY_ID :
                ATTACHED_DB_MISMATCH_DURING_RECOVERY_ID ),
        _countof( rgszT ),
        rgszT,
        0,
        NULL,
        pinst );
    
    OSUHAPublishEvent(  HaDbFailureTagConfiguration,
                        pinst,
                        HA_LOGGING_RECOVERY_CATEGORY,
                        HaDbIoErrorNone, NULL, 0, 0,
                        ( fEndOfRecovery ?
                                HA_ATTACHED_DB_MISMATCH_END_OF_RECOVERY_ID :
                                HA_ATTACHED_DB_MISMATCH_DURING_RECOVERY_ID ),
                        _countof( rgszT ),
                        rgszT );
}

LOCAL VOID LGReportConsistentTimeMismatch(
    const INST * const  pinst,
    const WCHAR * const szDbName )
{
    WCHAR               szErrT[16];
    const WCHAR *       rgszT[2]        = { szErrT, szDbName };

    Assert( NULL != szDbName );
    Assert( LOSStrLengthW( szDbName ) > 0 );

    OSStrCbFormatW( szErrT, sizeof(szErrT), L"%d", JET_errConsistentTimeMismatch );

    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        CONSISTENT_TIME_MISMATCH_ID,
        sizeof( rgszT ) / sizeof( rgszT[0] ),
        rgszT,
        0,
        NULL,
        pinst );

    OSUHAPublishEvent(  HaDbFailureTagCorruption,
                        pinst,
                        HA_LOGGING_RECOVERY_CATEGORY,
                        HaDbIoErrorNone, NULL, 0, 0,
                        HA_CONSISTENT_TIME_MISMATCH_ID,
                        sizeof( rgszT ) / sizeof( rgszT[ 0 ] ),
                        rgszT );
}


//  Parameters:
//      errRedoOrDbHdrCheck - The error returned from running redo,
//          or if redo was successful the error returned in checking
//          the attached DBs had their required log ranges ...
VOID LOG::LGITryCleanupAfterRedoError( ERR errRedoOrDbHdrCheck )
{
    //  about to err out, but before doing so, attempt to flush
    //  as much as possible (except if LLR in effect)
    //
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );

    //  we are specifically NOT triggering this at all ... because
    //  since we are error'ing out, we never did undo, so skip
    //  that step (so it shows up as skipped in timing seq).
    //      m_pinst->m_isdlInit.Trigger( eInitLogRecoveryUndoDone );

    m_fLogDisabledDueToRecoveryFailure = fTrue;
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP  ifmp    = m_pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp < g_ifmpMax )
        {
            const BOOL fTestPreimageRedo = !!UlConfigOverrideInjection( 62228, 0 );

            // Avoid the final BFFlush if the test hook is set. This makes it easier for
            // calling code to call 'JetInit(); ExitProcess()' and get an incomplete Recovery.
            if ( !fTestPreimageRedo && (LONG)UlParam( m_pinst, JET_paramWaypointLatency ) == 0 )
            {
                //  Note: since we're failing out of recovery, it is quite possible some
                //  buffers are in an unflushable state, and this function will then return
                //  a failure (that we ignore), so the flush here is best effort.  We maybe
                //  should reconsider the real value of this flush?
                (VOID)ErrBFFlush( ifmp );
            }

            if ( g_rgfmp[ifmp].FDeferredAttach()
                && !g_rgfmp[ifmp].FIgnoreDeferredAttach()
                && !m_fReplayingIgnoreMissingDB
                && !m_fAfterEndAllSessions )
            {
                //  there are some deferred attachments which we can't
                //  ignore, so don't update the checkpoint, otherwise
                //  it will advance beyond those missing attachments
                //
                LGDisableCheckpoint();
            }
        }
    }
    m_fLogDisabledDueToRecoveryFailure = fFalse;

    m_pinst->m_isdlInit.Trigger( eInitLogRecoveryBfFlushDone );

    if ( errRedoOrDbHdrCheck == JET_errInvalidLogSequence )
    {
        //  In this case we can't trust the value of m_plgfilehdr->lgfilehdr.le_lGeneration,
        //  which is used in checkpoint calculation (LGIUpdateCheckpoint())
        LGDisableCheckpoint();
    }

    if ( !m_fAfterEndAllSessions )
    {
        CallS( ErrLGRIEndAllSessionsWithError() );
        m_fAfterEndAllSessions = fTrue;
    }
}


ERR LOG::ErrLGICheckGenMaxRequiredAfterRedo()
{

    const ERR   err     = ErrLGCheckGenMaxRequired();
    Assert( m_fIgnoreLostLogs || JET_wrnCommittedLogFilesLost != err );

    if ( err < JET_errSuccess )     //  if this clause evaluates to FALSE, cleanup will be performed by the caller
    {
        Assert( err == JET_errCommittedLogFilesMissing ||
                err == JET_errRequiredLogFilesMissing );
        Assert( !m_fIgnoreLostLogs || err != JET_errCommittedLogFilesMissing );
    
        //  Flush as much as possible before bail out
        //
        LGITryCleanupAfterRedoError( err );
    }

    return err;
}

LOCAL ERR ErrLGIAttachedDatabaseMismatch( const FMP * const pfmp, const bool fIgnoreAnyMissing, const bool fLastLRIsShutdown )
{
    if ( pfmp->FDeferredAttach() && !pfmp->FIgnoreDeferredAttach() && !fIgnoreAnyMissing && ( !fLastLRIsShutdown || !pfmp->FDeferredAttachConsistentFuture() ) )
    {
        if ( pfmp->FDeferredForAccessDenied() )
        {
            return ErrERRCheck( JET_errAccessDenied );
        }
        else
        {
            return ErrERRCheck( JET_errAttachedDatabaseMismatch );
        }
    }
    if( pfmp->FSkippedAttach() )
    {
        return ErrERRCheck( JET_errAttachedDatabaseMismatch );
    }
    return JET_errSuccess;
}

LOCAL ERR ErrLGRICheckForAttachedDatabaseMismatch( INST * const pinst, const bool fIgnoreAnyMissing, const bool fLastLRIsShutdown )
{
    ERR err = JET_errSuccess;

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
        {
            continue;
        }
        const FMP * const pfmp = &g_rgfmp[ ifmp ];

        err = ErrLGIAttachedDatabaseMismatch( pfmp, fIgnoreAnyMissing, fLastLRIsShutdown );
        if ( err < JET_errSuccess )
        {
            LGReportAttachedDbMismatch( pinst, pfmp->WszDatabaseName(), fTrue );
            Call( err );
        }
    }

HandleError:
    return err;
}

LOCAL ERR ErrLGRICheckForAttachedDatabaseMismatchFailFast( INST * const pinst )
{
    ERR err = JET_errSuccess;

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = pinst->m_mpdbidifmp[dbid];
        if ( ifmp >= g_ifmpMax )
        {
            continue;
        }
        const FMP * const pfmp = &g_rgfmp[ifmp];

        if ( pfmp->FDeferredAttach() && pfmp->FFailFastDeferredAttach() )
        {
            LGReportAttachedDbMismatch( pinst, pfmp->WszDatabaseName(), fFalse );
            Call( ErrERRCheck( JET_errAttachedDatabaseMismatch ) );
        }
    }

HandleError:
    Assert( JET_errSuccess == err || JET_errAttachedDatabaseMismatch == err );
    return err;
}

// Terminate each session in the global session list
ERR LOG::ErrLGRIEndEverySession()
{
    ERR err = JET_errSuccess;

    PIB * ppib = m_pinst->m_ppibGlobal;
    PIB * ppibNext;
    PROCID procidT = 0;
    while( ppib != ppibNil )
    {
        //  clean up the hash table because we're about to terminate
        //
        Assert( procidNil != ppib->procid );
        Assert( NULL != m_pcsessionhash );
        // If the pib is in the instance's session list but not in m_pcsessionhash,
        // it was created by someone doing read-from-passive, and will be cleaned up
        // by them, otherwise we need to clean it up
        //
        if ( m_pcsessionhash->PpibHashFind( ppib->procid ) == ppibNil )
        {
            ppib = ppib->ppibNext;
            continue;
        }
        m_pcsessionhash->RemovePib( ppib );

        //  WARNING! WARNING! WARNING!
        //  The force-detach code assumes that the pib list is in
        //  monotonically increasing procid order, so that in case
        //  some databases are force-detached, we will always replay
        //  the undo's for all sessions in the exact same order
        //
        Assert( procidT < ppib->procid );
        procidT = ppib->procid;

        ppibNext = ppib->ppibNext;
        Assert( sizeof(JET_SESID) == sizeof(ppib) );
        Call( ErrIsamEndSession( (JET_SESID)ppib, NO_GRBIT ) );
        ppib = ppibNext;
    }

HandleError:
    return err;
}

/*
 *      Ends redo session.
 *  If fEndOfLog, then write log records to indicate the operations
 *  for recovery. If fPass1 is true, then it is for phase1 operations,
 *  otherwise for phase 2.
 */

ERR LOG::ErrLGRIEndAllSessions(
    const BOOL              fEndOfLog,
    const BOOL              fKeepDbAttached,
    const LE_LGPOS *        ple_lgposRedoFrom,
    BYTE *                  pbAttach )
{
    ERR                     err                 = JET_errSuccess;
    PIB *                   ppib                = ppibNil;
    BOOL                    fNeedCallINSTTerm   = fTrue;
    DBID dbid;

    //  UNDONE: is this call needed?
    //
    //(VOID)ErrVERRCEClean( );

    //  Set current time to attached db's dbfilehdr

    for (dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];

        // If any db is in the middle of CreateDB, it needs to be cleaned up
        if ( fKeepDbAttached && pfmp->FCreatingDB() )
        {
            (VOID)ErrIOTermFMP( pfmp, m_lgposRedo, fFalse );

            continue;
        }

        //  If there is no redo operations on an attached db, then
        //  pfmp->dbtimeCurrent may == 0, i.e. no operation, then do
        //  not change pdbfilehdr->dbtime

        if ( pfmp->Pdbfilehdr() != NULL &&
             pfmp->DbtimeCurrentDuringRecovery() > pfmp->DbtimeLast() )
        {
            pfmp->SetDbtimeLast( pfmp->DbtimeCurrentDuringRecovery() );
        }

        //  if we are out of redo, then note that we aren't recovering anything anymore
        if ( !pfmp->FSkippedAttach() &&
                !pfmp->FDeferredAttach() &&
                m_fRecoveringMode == fRecoveringUndo )
        {
            Assert( pfmp->Pdbfilehdr() );

            if ( NULL != pfmp->Pdbfilehdr() )   // for insurance
            {
                pfmp->PdbfilehdrUpdateable()->le_lGenRecovering = 0;
                
                //  If we've made it all the way to undo, we certainly should not have 
                //  a DB in the patched state ... we should've bailed out before we started
                //  undo in ErrLGICheckGenMaxRequiredAfterRedo() / ErrLGCheckGenMaxRequired()
                //  due to the required log generations not being present.
                DBEnforce( ifmp, pfmp->Pdbfilehdr()->Dbstate() != JET_dbstateDirtyAndPatchedShutdown );
            }
        }

        if ( NULL != pfmp->Pdbfilehdr() &&
             !pfmp->FContainsDataFromFutureLogs() )
        {
            //  fire callback for the dbs to be detached

            Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );
        }
    }


        //  make sure there are no deferred RCEs and and remove all inserted RCEs for any session
    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
    {
#ifndef RTM
        ppib->AssertNoDeferredRceid();
#endif  //  !RTM
        ppib->RemoveAllRceid();
    }

    Assert( !fEndOfLog || m_fRecovering );
    Assert( !fEndOfLog || fRecoveringUndo == m_fRecoveringMode );

    Call( ErrLGEndAllSessionsMacro( fTrue /* fLogEndMacro */ ) );

    //  if last shutdown is dirty, need to force-detach
    //  any missing databases
    //
    if ( fEndOfLog )
    {
        Call( ErrLGRICheckForAttachedDatabaseMismatch( m_pinst, !!m_fReplayingIgnoreMissingDB, !!m_fLastLRIsShutdown ) );
    }

    Call( ErrLGRIEndEverySession() );

    (VOID)m_pinst->m_pver->ErrVERRCEClean( );

    if ( fEndOfLog )
    {
        m_pinst->m_isdlInit.Trigger( eInitLogRecoveryUndoDone );
    }

    if ( !fKeepDbAttached )
    {
        // Set term in progress to allow waypoint advancement to the current gen
        m_pinst->m_fNoWaypointLatency = fTrue;

        for ( dbid = dbidUserLeast; dbid < dbidMax; dbid ++ )
        {
            if ( m_pinst->m_mpdbidifmp[ dbid ] < g_ifmpMax )
            {
                Call( ErrBFFlush( m_pinst->m_mpdbidifmp[ dbid ] ) );
            }
        }

        m_pinst->m_fNoWaypointLatency = fFalse;

        if ( fEndOfLog )
        {
            m_pinst->m_isdlInit.Trigger( eInitLogRecoveryBfFlushDone );
        }
    }

    /*  Detach all the faked attachment. Detach all the databases that were restored
     *  to new location. Attach those database with new location.
     */
    Assert( ppibNil == ppib );
    Call( ErrPIBBeginSession( m_pinst, &ppib, procidNil, fFalse ) );

    Assert( !ppib->FRecoveringEndAllSessions() );
    ppib->SetFRecoveringEndAllSessions();

    for ( dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP  ifmp    = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( !fKeepDbAttached &&
             pfmp->Pdbfilehdr() )
        {
            //  We can't keep the hdr lock over ErrLGICheckDatabaseFileSize() because it also
            //  takes the lock.
            if ( CmpLgpos( &lgposMax, &pfmp->Pdbfilehdr()->le_lgposAttach ) != 0 )
            {
                //  make sure the attached database's size is consistent with the file size.

                Call( ErrLGICheckDatabaseFileSize( ppib, ifmp ) );
            }
        }

        if ( fEndOfLog )
        {
            /*  for each faked attached database, log database detachment
            /*  This happen only when someone restore a database that was compacted,
            /*  attached, used, then crashed. When restore, we ignore the compact and
            /*  attach after compact since the db does not match. At the end of restore
            /*  we fake a detach since the database is not recovered.
            /**/
            if ( !pfmp->Pdbfilehdr() && pfmp->Patchchk() )
            {
                FMP::EnterFMPPoolAsWriter();

                Assert( pfmp->FInUse() );
                Assert( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() );

                /*  clean up the fmp entry
                /**/
                pfmp->ResetFlags();
                pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = g_ifmpMax;

                OSMemoryHeapFree( pfmp->WszDatabaseName() );
                pfmp->SetWszDatabaseName( NULL );

                OSMemoryHeapFree( pfmp->Patchchk() );
                pfmp->SetPatchchk( NULL );

                FMP::LeaveFMPPoolAsWriter();
            }
            else
            {
                Assert( !pfmp->FSkippedAttach() );
                Assert( !pfmp->FDeferredAttach() );
            }
        }
    }

    PIBEndSession( ppib );
    ppib = ppibNil;

    delete m_pctablehash;
    m_pctablehash = NULL;

    delete m_pcsessionhash;
    m_pcsessionhash = NULL;

    if ( fEndOfLog )
    {
        /*  enable checkpoint updates
        /**/
        m_fLGFMPLoaded = fTrue;
    }

    if ( !fKeepDbAttached )
    {
        *pbAttach = 0;

#ifdef DEBUG
        m_pinst->m_trxNewest = trxMax - 1023;
#else
        m_pinst->m_trxNewest = trxMin;
#endif
        // We now know for sure what the next trxid we will see in recovery will be
        m_pinst->m_fTrxNewestSetByRecovery = fTrue;
    }

    /*  term with checkpoint updates
    /**/
    err = m_pinst->ErrINSTTerm( fKeepDbAttached ? termtypeRecoveryQuitKeepAttachments : termtypeNoCleanUp );
    if ( err < JET_errSuccess )
    {
        // Since ErrINSTTerm/ErrIOTerm have failed to produce a clean db, we should bail now before logging
        // RecoveryQuit. All other cleanup is already done.
        return err;
    }
    fNeedCallINSTTerm = fFalse;

    /*  stop checkpoint updates
    /**/
    m_fLGFMPLoaded = fFalse;

    if ( m_fRecovering && fRecoveringRedo == m_fRecoveringMode )
    {
        // If we're in redo mode, we didn't log anything, so there's no need
        // to try and flush. We don't want to call ErrLGWaitAllFlushed() when
        // we're in redo mode.
    }
    else if ( fEndOfLog )
    {
        LE_LGPOS    le_lgposRecoveryUndo    = m_lgposRecoveryUndo;
        LGPOS       lgposRecoveryQuit;

        Assert( m_fRecovering );
        Assert( fRecoveringUndo == m_fRecoveringMode );

        Call( ErrLGRecoveryQuit(
                    this,
                    &le_lgposRecoveryUndo,
                    ple_lgposRedoFrom,
                    fKeepDbAttached,
                    // SOFT_HARD: info flag only, leave it
                    m_fHardRestore,
                    BoolParam( m_pinst, JET_paramAggressiveLogRollover ),
                    &lgposRecoveryQuit ) );

        if ( !fKeepDbAttached )
        {
            /*  Note: flush is needed in case a new generation is generated and
            /*  the global variable szLogName is set while it is changed to new names.
            /*  critical section not requested, not needed
            /**/

            // The above comment is misleading, because we really want to flush the log
            // so that all our data will hit the disk if we've logged a lrtypRecoveryQuit.
            // If it doesn't hit the disk, we'll be missing a quit record on disk and we'll
            // be in trouble. Note that calling ErrLGFlushLog() will not necessarily flush
            // all of the log buffers.
            Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );

            // the last thing to do with the checkpoint is to update it to
            // the Term log record. This will make determine a clean shutdown
            Call( ErrLGIUpdateCheckpointLgposForTerm( lgposRecoveryQuit ) );
        }
    }
    else if ( !fKeepDbAttached )
    {
        Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );
    }

HandleError:

    (void)ErrLGEndAllSessionsMacro( fFalse /* fLogEndMacro */ );

    if ( ppib != ppibNil )
    {
        PIBEndSession( ppib );
    }

    delete m_pctablehash;
    m_pctablehash = NULL;

    delete m_pcsessionhash;
    m_pcsessionhash = NULL;

    if ( fNeedCallINSTTerm )
    {
        Assert( err < JET_errSuccess );
        m_fLogDisabledDueToRecoveryFailure = fTrue;
        (void)m_pinst->ErrINSTTerm( termtypeError );
        m_fLogDisabledDueToRecoveryFailure = fFalse;
    }

    return err;
}


#define cbSPExt 30

#ifdef DEBUG
void LOG::LGRITraceRedo(const LR *plr)
{
    /* easier to debug */
    if ( m_fDBGTraceRedo )
    {
        g_critDBGPrint.Enter();

        if ( GetNOP() > 0 )
        {
            CheckEndOfNOPList( plr, this );
        }

        if ( 0 == GetNOP() || plr->lrtyp != lrtypNOP )
        {
            PrintLgposReadLR();
            ShowLR(plr, this);
        }

        g_critDBGPrint.Leave();
    }
}

#ifndef RFS2

#undef CallJ
#undef CallR

#define CallJ(f, hndlerr)                                     \
    {                                                         \
        if ( ( err = ( f ) ) < 0 )                            \
        {                                                     \
            AssertSz( 0, "Debug Only: ignore this assert" );  \
            goto hndlerr;                                     \
        }                                                     \
    }

#define CallR(f)                                              \
    {                                                         \
        if ( ( err = ( f ) ) < 0 )                            \
        {                                                     \
            AssertSz( 0, "Debug Only: ignore this assert" );  \
            return err;                                       \
        }                                                     \
    }

#endif
#endif


ERR ErrLGIStoreLogRec( PIB *ppib, const DBTIME dbtime, const LR *plr )
{
    const ULONG cb  = CbLGSizeOfRec( plr );

    return ppib->ErrInsertLogrec( dbtime, plr, cb );
}

BOOL LOG::FLGRICheckRedoConditionForAttachedDb(
    const BOOL          fReplayIgnoreLogRecordsBeforeMinRequiredLog,
    const DBFILEHDR *   pdbfilehdr,
    const LGPOS&        lgpos )
{
    //  Check if the database is open.

    if ( NULL == pdbfilehdr )
    {
        return fFalse;
    }

    //  We haven't reached the point where the database is attached.

    if ( CmpLgpos( &lgpos, &pdbfilehdr->le_lgposAttach ) <= 0 )
    {
        // This might happen when prereading pages referenced in the first log to be replayed, before
        // lgpos has a valid value.
        Expected( CmpLgpos( lgpos, lgposMin ) == 0 );

        return fFalse;
    }

    //  optionally skip redo of this log record if it is below the min required log for this database
    //
    //  NOTE:  there may be a range of log records [le_lGenMinRequired, le_lGenMinConsistent) where the normal redo
    //  check will return false.  we must not skip these because the persisted lost flush map is fixed up as a side
    //  effect of reading the pages to perform these redo checks.

    if (    fReplayIgnoreLogRecordsBeforeMinRequiredLog &&
            lgpos.lGeneration < pdbfilehdr->le_lGenMinRequired )
    {
        return fFalse;
    }

    return fTrue;
}

BOOL LOG::FLGRICheckRedoConditionForDb(
    const DBID      dbid,
    const LGPOS&    lgpos )
{
    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];

    if ( ifmp >= g_ifmpMax )
    {
        return fFalse;
    }

    return LOG::FLGRICheckRedoConditionForAttachedDb(
            m_fReplayIgnoreLogRecordsBeforeMinRequiredLog,
            g_rgfmp[ifmp].Pdbfilehdr().get(),
            lgpos );
}

//  Check redo condition to decide if we need to skip redoing
//  this log record. It does not check the database pages referred
//  to in the log record; only the database header.

ERR LOG::ErrLGRICheckRedoCondition(
    const DBID      dbid,                   //  dbid from the log record.
    DBTIME          dbtime,                 //  dbtime from the log record.
    OBJID           objidFDP,               //  objid so far,
    PIB             *ppib,                  //  returned ppib
    BOOL            fUpdateCountersOnly,    //  if TRUE, operation will NOT be redone, but still need to update dbtimeLast and objidLast counters
    BOOL            *pfSkip )               //  returned skip flag
{
    ERR             err;
    INST * const    pinst   = PinstFromPpib( ppib );
    const IFMP      ifmp    = pinst->m_mpdbidifmp[ dbid ];

    //  By default we want to skip it.

    *pfSkip = fTrue;

    //  check if we have to redo the database.

    if ( ifmp >= g_ifmpMax )
    {
        return JET_errSuccess;
    }

    if ( !FLGRICheckRedoConditionForDb( dbid, m_lgposRedo ) )
    {
        // NOTE:  we must fall through to continue to keep dbtimeLast and objidLast up to date!
        fUpdateCountersOnly = fTrue;
    }

    FMP * const pfmp = g_rgfmp + ifmp;

    //  check if database needs opening.

    if ( !fUpdateCountersOnly
        && !FPIBUserOpenedDatabase( ppib, dbid ) )
    {
        IFMP    ifmpT   = ifmp;
        CallR( ErrDBOpenDatabase( ppib,
                                  pfmp->WszDatabaseName(),
                                  &ifmpT,
                                  bitDbOpenForRecovery ) );
        Assert( ifmpT == ifmp );
    }

    //  Keep track of largest dbtime so far, since the number could be
    //  logged out of order, we need to check if dbtime > than dbtimeCurrent.

    Assert( m_fRecoveringMode == fRecoveringRedo );
    if ( dbtime > pfmp->DbtimeCurrentDuringRecovery() )
        pfmp->SetDbtimeCurrentDuringRecovery( dbtime );
    if ( pfmp->FNeedUpdateDbtimeBeginRBS() && dbtime > pfmp->DbtimeBeginRBS() )
    {
        Assert( pfmp->FContainsDataFromFutureLogs() );
        pfmp->SetDbtimeBeginRBS( dbtime );
    }
    if ( objidFDP > pfmp->ObjidLast() )
        pfmp->SetObjidLast( objidFDP );

    if ( fUpdateCountersOnly )
    {
        Assert( *pfSkip );
    }
    else
    {
        //  We do need to redo this log record.
        *pfSkip = fFalse;
    }

    return JET_errSuccess;
}


ERR LOG::ErrLGRICheckRedoConditionInTrx(
    const PROCID    procid,
    const DBID      dbid,                   //  dbid from the log record.
    DBTIME          dbtime,                 //  dbtime from the log record.
    OBJID           objidFDP,
    const LR        *plr,
    PIB             **pppib,                //  returned ppib
    BOOL            *pfSkip )               //  returned skip flag
{
    ERR             err;
    PIB             *ppib;
    BOOL            fUpdateCountersOnly     = fFalse;   //  if TRUE, redo not needed, but must still update dbtimeLast and objidLast counters

    *pfSkip = fTrue;

    CallR( ErrLGRIPpibFromProcid( procid, &ppib ) );
    *pppib = ppib;

    if ( ppib->FAfterFirstBT() )
    {
        Assert( NULL != plr || !ppib->FMacroGoing( dbtime ) );
        if ( ppib->FMacroGoing( dbtime )
            && lrtypUndoInfo != plr->lrtyp )
        {
            return ErrLGIStoreLogRec( ppib, dbtime, plr );
        }
    }
    else
    {
        //  it's possible that there are no Begin0's between the first and
        //  last log records, in which case nothing needs to
        //  get redone.  HOWEVER, the dbtimeLast and objidLast
        //  counters in the db header would not have gotten
        //  flushed (they only get flushed on a DetachDb or a
        //  clean shutdown), so we must still track these
        //  counters during recovery so that we can properly
        //  update the database header on RecoveryQuit (since
        //  we pass TRUE for the fUpdateCountersOnly param to
        //  ErrLGRICheckRedoCondition() below, that function
        //  will do nothing but update the counters for us).
        fUpdateCountersOnly = fTrue;
    }

    return ErrLGRICheckRedoCondition(
            dbid,
            dbtime,
            objidFDP,
            ppib,
            fUpdateCountersOnly,
            pfSkip );
}

//  sets dbtime on write-latched pages
//
INLINE VOID LGRIRedoDirtyAndSetDbtime( CSR *pcsr, DBTIME dbtime )
{
    if ( latchWrite == pcsr->Latch() )
    {
        Assert( pcsr->Cpage().PgnoThis() != pgnoNull );
        Assert( pcsr->Dbtime() <= dbtime );
        pcsr->Dirty();
        pcsr->SetDbtime( dbtime );
    }
}

ERR LOG::ErrLGRIRedoFreeEmptyPages(
    FUCB                * const pfucb,
    LREMPTYTREE         * const plremptytree )
{
    ERR                 err                     = JET_errSuccess;
    const INST          * pinst                 = PinstFromPpib( pfucb->ppib );
    const IFMP          ifmp                    = pfucb->ifmp;
    CSR                 * const pcsrEmpty       = &pfucb->csr;
    const EMPTYPAGE     * const rgemptypage     = (EMPTYPAGE *)plremptytree->rgb;
    const CPG           cpgToFree               = plremptytree->CbEmptyPageList() / sizeof(EMPTYPAGE);

    Assert( ifmp == pinst->m_mpdbidifmp[ plremptytree->dbid ] );
    Assert( plremptytree->CbEmptyPageList() % sizeof(EMPTYPAGE) == 0 );
    Assert( cpgToFree > 0 );

    for ( INT i = 0; i < cpgToFree; i++ )
    {
        BOOL fRedoNeeded;

        Assert( !pcsrEmpty->FLatched() );

        err = ErrLGIAccessPage( pfucb->ppib, pcsrEmpty, ifmp, rgemptypage[i].pgno, plremptytree->le_objidFDP, fFalse );

        if ( errSkipLogRedoOperation == err )
        {
            // If we get errSkipLogRedoOperation, it's important to check the remaining pages in rgemptypage.
            err = JET_errSuccess;

            Assert( pagetrimTrimmed == pcsrEmpty->PagetrimState() );
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( rgemptypage[i].pgno ) );
            Assert( !pcsrEmpty->FLatched() );

            // Clear the page from dbtimerevert redo map since the page is being freed.
            // Any future operation on this page should be a new page operation and we shouldn't have to worry about dbtime.
            //
            if ( g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert() )
            {
                g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert()->ClearPgno( rgemptypage[i].pgno );
            }
        }
        else
        {
            Call( err );
            Assert( latchRIW == pcsrEmpty->Latch() );
        }

        Assert( rgemptypage[i].pgno == pcsrEmpty->Pgno() );

        fRedoNeeded = FLGNeedRedoCheckDbtimeBefore( ifmp,
                                                    *pcsrEmpty,
                                                    plremptytree->le_dbtime,
                                                    rgemptypage[i].dbtimeBefore,
                                                    &err );

        // for the FLGNeedRedoCheckDbtimeBefore error code
        if ( err < 0 )
        {
            pcsrEmpty->ReleasePage();
            goto HandleError;
        }

        //  upgrade latch if needed
        //
        if ( fRedoNeeded )
        {
            pcsrEmpty->SetILine( 0 );
            pcsrEmpty->UpgradeFromRIWLatch();
            pcsrEmpty->CoordinatedDirty( plremptytree->le_dbtime );
            NDEmptyPage( pcsrEmpty );
        }

        pcsrEmpty->ReleasePage();
    }

HandleError:
    AssertSz( errSkipLogRedoOperation != err, "%s() got errSkipLogRedoOperation from ErrLGIAccessPage(%d=%#x).\n",
              __FUNCTION__, (DBTIME) plremptytree->le_dbtime, (DBTIME) plremptytree->le_dbtime );

    Assert( !pcsrEmpty->FLatched() );

    return err;
}

ERR LOG::ErrLGRIRedoNodeOperation( const LRNODE_ *plrnode, ERR *perr )
{
    ERR             err;
    PIB             *ppib;
    const PGNO      pgno        = plrnode->le_pgno;
    const PGNO      pgnoFDP     = plrnode->le_pgnoFDP;
    const OBJID     objidFDP    = plrnode->le_objidFDP; // Debug only info.
    const PROCID    procid      = plrnode->le_procid;
    const DBID      dbid        = plrnode->dbid;
    const DBTIME    dbtime      = plrnode->le_dbtime;
    const BOOL      fUnique     = plrnode->FUnique();
    const BOOL      fSpace      = plrnode->FSpace();
    const DIRFLAG   dirflag     = plrnode->FVersioned() ? fDIRNull : fDIRNoVersion;
    VERPROXY        verproxy;

    verproxy.rceid = plrnode->le_rceid;
    verproxy.level = plrnode->level;
    verproxy.proxy = proxyRedo;

    Assert( !plrnode->FVersioned() || !plrnode->FSpace() );
    Assert( !plrnode->FVersioned() || rceidNull != verproxy.rceid );
    Assert( !plrnode->FVersioned() || verproxy.level > 0 );

    BOOL fSkip;
    CallR( ErrLGRICheckRedoConditionInTrx(
                procid,
                dbid,
                dbtime,
                objidFDP,
                (LR *) plrnode, //  can be in macro.
                &ppib,
                &fSkip ) );
    if ( fSkip )
        return JET_errSuccess;

    if ( ppib->Level() > plrnode->level )
    {
        //  if operation was performed by concurrent CreateIndex, the
        //  updater could be at a higher trx level than when the
        //  indexer logged the operation
        //  UNDONE: explain why Undo and UndoInfo can have ppib at higher trx
        Assert( plrnode->FConcCI()
            || lrtypUndoInfo == plrnode->lrtyp
            || lrtypUndo == plrnode->lrtyp );
    }
    else
    {
        //  UNDONE: for lrtypUndoInfo, is it really possible for ppib to
        //  be at lower level than logged operation?
        Assert( ppib->Level() == plrnode->level
            || lrtypUndoInfo == plrnode->lrtyp );
    }

    INST    *pinst = PinstFromPpib( ppib );
    IFMP    ifmp = pinst->m_mpdbidifmp[ dbid ];
    FUCB    *pfucb;

    CallR( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, &pfucb ) );

    Assert( pfucb->ppib == ppib );

    //  reset CSR
    //
    CSR     csr;
    BOOL    fRedoNeeded;

    err = ErrLGIAccessPage( ppib, &csr, ifmp, pgno, plrnode->le_objidFDP, fFalse );

    if ( errSkipLogRedoOperation == err )
    {
        // If we get errSkipLogRedoOperation, it's important to check the remaining pages in the log record.
        err = JET_errSuccess;

        Assert( pagetrimTrimmed == csr.PagetrimState() );
        Assert( FRedoMapNeeded( ifmp ) );
        Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgno ) );
        Assert( !csr.FLatched() );
    }
    else
    {
        Call( err );
        Assert( latchRIW == csr.Latch() );
    }


    fRedoNeeded = lrtypUndoInfo == plrnode->lrtyp ?
                                fFalse :
                                FLGNeedRedoCheckDbtimeBefore( ifmp, csr, dbtime, plrnode->le_dbtimeBefore, &err );

    // for the FLGNeedRedoCheckDbtimeBefore error code
    Call( err );

    err = JET_errSuccess;

    //  set CSR
    //  upgrade latch if needed
    //
    csr.SetILine( plrnode->ILine() );
    if ( fRedoNeeded )
    {
        csr.UpgradeFromRIWLatch();
    }

    LGRITraceRedo( plrnode );

    Assert( !fRedoNeeded || objidFDP == csr.Cpage().ObjidFDP() );

    switch ( plrnode->lrtyp )
    {
        default:
            Assert( fFalse );
            break;

        case lrtypSetExternalHeader:
        {
            if ( !fRedoNeeded )
            {
                CallS( err );
                goto HandleError;
            }

            DATA                data;
            LRSETEXTERNALHEADER *plrsetextheader = (LRSETEXTERNALHEADER *) plrnode;

            data.SetPv( plrsetextheader->rgbData );
            data.SetCb( plrsetextheader->CbData() );

            err = ErrNDSetExternalHeader( pfucb, &csr, &data, dirflag | fDIRRedo, noderfWhole );
            CallS( err );
            Call( err );
        }
            break;

        case lrtypUndoInfo:
        {
            //  restore undo information in version store
            //      for a page flushed with uncommitted change
            //      if RCE already exists for this operation at the same level,
            //          do nothing
            //
            LRUNDOINFO  *plrundoinfo = (LRUNDOINFO *) plrnode;
            RCE         *prce;
            const OPER  oper = plrundoinfo->le_oper;

            Assert( !fRedoNeeded );

            //  mask PIB fields to logged values for creating version
            //
            const TRX   trxOld      = ppib->trxBegin0;
            const LEVEL levelOld    = ppib->Level();

            Assert( pfucb->ppib == ppib );

            if ( 0 == ppib->Level() )
            {
                //  RCE is not useful, since there is no transaction to roll back
                //
                goto HandleError;
            }

            //  remove this RCE from the list of uncreated RCEs
            Call( ppib->ErrDeregisterDeferredRceid( plrundoinfo->le_rceid ) );

            Assert( trxOld == plrundoinfo->le_trxBegin0 );
            ppib->SetLevel( min( plrundoinfo->level, ppib->Level() ) );
            ppib->trxBegin0 = plrundoinfo->le_trxBegin0;

            Assert( trxMax == ppib->trxCommit0 );

            //  force RCE to be recreated at same current level as ppib
            Assert( plrundoinfo->level == verproxy.level );
            verproxy.level = ppib->Level();

            Assert( operReplace == oper ||
                    operFlagDelete == oper );
            Assert( FUndoableLoggedOper( oper ) );

            BOOKMARK    bm;
            bm.key.prefix.Nullify();
            bm.key.suffix.SetPv( plrundoinfo->rgbData );
            bm.key.suffix.SetCb( plrundoinfo->CbBookmarkKey() );

            bm.data.SetPv( plrundoinfo->rgbData + plrundoinfo->CbBookmarkKey() );
            bm.data.SetCb( plrundoinfo->CbBookmarkData() );

            //  set up fucb as in do-time
            //
            if ( operReplace == oper )
            {
                pfucb->kdfCurr.Nullify();
                pfucb->kdfCurr.key = bm.key;
                pfucb->kdfCurr.data.SetPv( plrundoinfo->rgbData +
                                             plrundoinfo->CbBookmarkKey() +
                                             plrundoinfo->CbBookmarkData() );
                pfucb->kdfCurr.data.SetCb( plrundoinfo->le_cbData );
            }

            //  create RCE
            //
            Assert( plrundoinfo->le_pgnoFDP == PgnoFDP( pfucb ) );

            CallJ( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, oper, &prce, &verproxy ), RestorePIB );
            Assert( prceNil != prce );

            //  if oper is replace, set verreplace in RCE
            if ( prce->FOperReplace() )
            {
                VERREPLACE* pverreplace = (VERREPLACE*)prce->PbData();

                pverreplace->cbMaxSize  = plrundoinfo->le_cbMaxSize;
                pverreplace->cbDelta    = 0;
            }

            // Pass pcsrNil to prevent creation of UndoInfo
            VERInsertRCEIntoLists( pfucb, pcsrNil, prce );

        RestorePIB:
            ppib->SetLevel( levelOld );
            ppib->trxBegin0 = trxOld;

            if ( JET_errPreviousVersion == err )
            {
                err = JET_errSuccess;
            }
            Call( err );

            //  skip to release page
            //
            goto HandleError;
        }
            break;

        case lrtypUndo:
        {
            LRUNDO  *plrundo = (LRUNDO *)plrnode;

            CallR( ErrLGRIPpibFromProcid( plrundo->le_procid, &ppib ) );

            Assert( !ppib->FMacroGoing( dbtime ) );

            //  check transaction level
            //
            if ( ppib->Level() <= 0 )
            {
                Assert( fFalse );
                break;
            }

            LGRITraceRedo( plrnode );

            Assert( plrundo->le_pgnoFDP == PgnoFDP( pfucb ) );
            VERRedoPhysicalUndo( m_pinst, plrundo, pfucb, &csr, fRedoNeeded );

            if ( !fRedoNeeded )
            {
                CallS( err );
                goto HandleError;
            }
        }
            break;

        case lrtypInsert:
        {
            LRINSERT        *plrinsert = (LRINSERT *)plrnode;

            KEYDATAFLAGS    kdf;

            kdf.key.prefix.SetPv( (BYTE *) plrinsert->szKey );
            kdf.key.prefix.SetCb( plrinsert->CbPrefix() );

            kdf.key.suffix.SetPv( (BYTE *)( plrinsert->szKey ) + plrinsert->CbPrefix() );
            kdf.key.suffix.SetCb( plrinsert->CbSuffix() );

            kdf.data.SetPv( (BYTE *)( plrinsert->szKey ) + kdf.key.Cb() );
            kdf.data.SetCb( plrinsert->CbData() );

            kdf.fFlags  = 0;

            if ( kdf.key.prefix.Cb() > 0 )
            {
                Assert( plrinsert->CbPrefix() > cbPrefixOverhead );
                kdf.fFlags  |= fNDCompressed;
            }

            //  even if operation is not redone, create version
            //  for rollback support.
            //
            if ( plrinsert->FVersioned() )
            {
                RCE         *prce = prceNil;
                BOOKMARK    bm;

                NDGetBookmarkFromKDF( pfucb, kdf, &bm );
                Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
                Assert( prceNil != prce );
                //  we have the page latched and we are not logging so the NDInsert won't fail
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

            if( fRedoNeeded )
            {
                Assert( plrinsert->ILine() == csr.ILine() );
                //  no logging of versioning so this can't fail
                Call( ErrNDInsert( pfucb, &csr, &kdf, dirflag | fDIRRedo, rceidNull, NULL ) );
            }
            else
            {
                err = JET_errSuccess;
                goto HandleError;
            }
        }
            break;

        case lrtypFlagInsert:
        {
            LRFLAGINSERT    *plrflaginsert = (LRFLAGINSERT *)plrnode;

            if ( plrflaginsert->FVersioned() )
            {
                KEYDATAFLAGS    kdf;

                kdf.data.SetPv( plrflaginsert->rgbData + plrflaginsert->CbKey() );
                kdf.data.SetCb( plrflaginsert->CbData() );

                kdf.key.prefix.Nullify();
                kdf.key.suffix.SetCb( plrflaginsert->CbKey() );
                kdf.key.suffix.SetPv( plrflaginsert->rgbData );

                BOOKMARK    bm;
                RCE         *prce = prceNil;

                NDGetBookmarkFromKDF( pfucb, kdf, &bm );
                Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
                Assert( prceNil != prce );
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

            if( fRedoNeeded )
            {
                Assert( plrflaginsert->ILine() == csr.ILine() );
                Call( ErrNDGet( pfucb, &csr ) );
                Assert( FNDDeleted( pfucb->kdfCurr ) );


                err = ErrNDFlagInsert( pfucb, &csr, dirflag | fDIRRedo, rceidNull, NULL );
                CallS( err );
            }
            else
            {
                err = JET_errSuccess;
                goto HandleError;
            }
            Call( err );
        }
            break;

        case lrtypFlagInsertAndReplaceData:
        {
            LRFLAGINSERTANDREPLACEDATA  *plrfiard =
                                        (LRFLAGINSERTANDREPLACEDATA *)plrnode;
            KEYDATAFLAGS    kdf;

            kdf.data.SetPv( plrfiard->rgbData + plrfiard->CbKey() );
            kdf.data.SetCb( plrfiard->CbData() );

            VERPROXY        verproxyReplace;
            verproxyReplace.rceid = plrfiard->le_rceidReplace;
            verproxyReplace.level = plrfiard->level;
            verproxyReplace.proxy = proxyRedo;

            if ( plrfiard->FVersioned() )
            {
                kdf.key.prefix.Nullify();
                kdf.key.suffix.SetCb( plrfiard->CbKey() );
                kdf.key.suffix.SetPv( plrfiard->rgbData );

                BOOKMARK    bm;
                RCE         *prce = prceNil;

                NDGetBookmarkFromKDF( pfucb, kdf, &bm );
                Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
                Assert( prceNil != prce );
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

            if( fRedoNeeded )
            {
                Assert( verproxyReplace.level == verproxy.level );
                Assert( plrfiard->ILine() == csr.ILine() );
                Call( ErrNDGet( pfucb, &csr ) );
                Assert( FNDDeleted( pfucb->kdfCurr ) );

                //  page may be reorganized
                //  copy key from node
                //

                BYTE *rgb;
                BFAlloc( bfasTemporary, (VOID **)&rgb );

//              BYTE    rgb[g_cbPageMax];

                pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );

                kdf.key.prefix.SetCb( pfucb->kdfCurr.key.prefix.Cb() );
                kdf.key.prefix.SetPv( rgb );
                kdf.key.suffix.SetCb( pfucb->kdfCurr.key.suffix.Cb() );
                kdf.key.suffix.SetPv( rgb + kdf.key.prefix.Cb() );
                Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

                if( plrfiard->FVersioned() )
                {
                    verproxy.rceid = plrfiard->le_rceidReplace;

                    //  we need to create the replace version for undo
                    //  if we don't need to redo the operation, the undo-info will create
                    //  the version if necessary
                    BOOKMARK    bm;
                    RCE         *prce   = prceNil;

                    NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
                    err = PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operReplace, &prce, &verproxy );
                    if ( err < 0 )
                    {
                        BFFree( rgb );
                        goto HandleError;
                    }
                    Assert( prceNil != prce );
                    VERInsertRCEIntoLists( pfucb, &csr, prce );
                }

                err = ErrNDFlagInsertAndReplaceData( pfucb,
                                                     &csr,
                                                     &kdf,
                                                     dirflag | fDIRRedo,
                                                     rceidNull,
                                                     rceidNull,
                                                     prceNil,
                                                     NULL );

                BFFree( rgb );

                CallS( err );
                Call( err );
            }
            else
            {
                //  add this RCE to the list of uncreated RCEs
                if ( plrfiard->FVersioned() )
                {
                    Call( ppib->ErrRegisterDeferredRceid( plrfiard->le_rceidReplace, pgno ) );
                }

                CallS( err );
                goto HandleError;
            }
        }
            break;

        case lrtypReplace:
        case lrtypReplaceD:
        {
            LRREPLACE   * const plrreplace  = (LRREPLACE *)plrnode;

            if ( !fRedoNeeded )
            {

                //  add this RCE to the list of uncreated RCEs
                if ( plrreplace->FVersioned() )
                {
                    Call( ppib->ErrRegisterDeferredRceid( plrnode->le_rceid, pgno ) );
                }

                CallS( err );
                goto HandleError;
            }

            RCE             *prce = prceNil;
            const UINT      cbNewData = plrreplace->CbNewData();
            DATA            data;
            BYTE            *rgbRecNew = NULL;
//          BYTE            rgbRecNew[g_cbPageMax];

            //  cache node
            //
            Assert( plrreplace->ILine() == csr.ILine() );
            Call( ErrNDGet( pfucb, &csr ) );

            if ( plrnode->lrtyp == lrtypReplaceD )
            {
                SIZE_T  cb;
                BYTE    *pbDiff = (BYTE *)( plrreplace->szData );
                ULONG   cbDiff  = plrreplace->Cb();

                BFAlloc( bfasTemporary, (VOID **)&rgbRecNew );

                err = ErrLGGetAfterImage( pfucb->ifmp,
                                          pbDiff,
                                          cbDiff,
                                          (BYTE *)pfucb->kdfCurr.data.Pv(),
                                          pfucb->kdfCurr.data.Cb(),
                                          plrreplace->FDiff2(),
                                          rgbRecNew,
                                          &cb );
                if ( err < 0 )
                {
                    BFFree( rgbRecNew );
                    goto HandleError;
                }
                
                Assert( cb < (SIZE_T)g_cbPage );
//              Assert( cb < sizeof( rgbRecNew ) );
                if ( cb >= (SIZE_T)g_cbPage )
                {
                    Error( ErrERRCheck( JET_errLogCorrupted ) );
                }

                data.SetPv( rgbRecNew );
                data.SetCb( cb );
            }
            else
            {
                data.SetPv( plrreplace->szData );
                data.SetCb( plrreplace->Cb() );
            }
            Assert( (ULONG)data.Cb() == cbNewData );
            if ( (ULONG)data.Cb() != cbNewData )
            {
                Error( ErrERRCheck( JET_errLogCorrupted ) );
            }

            //  copy bm to pfucb->bmCurr
            //
            BYTE *rgb;
            BFAlloc( bfasTemporary, (VOID **)&rgb );
//          BYTE    rgb[g_cbPageMax];
            pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );

            Assert( FFUCBUnique( pfucb ) );
            pfucb->bmCurr.data.Nullify();

            pfucb->bmCurr.key.prefix.SetCb( pfucb->kdfCurr.key.prefix.Cb() );
            pfucb->bmCurr.key.prefix.SetPv( rgb );
            pfucb->bmCurr.key.suffix.SetCb( pfucb->kdfCurr.key.suffix.Cb() );
            pfucb->bmCurr.key.suffix.SetPv( rgb + pfucb->kdfCurr.key.prefix.Cb() );
            Assert( CmpKey( pfucb->bmCurr.key, pfucb->kdfCurr.key ) == 0 );

            //  if we have to redo the operation we have to create the version
            //  the page has the proper before-image on it
            if( plrreplace->FVersioned() )
            {
                err = PverFromPpib( ppib )->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prce, &verproxy );
                if ( err < 0 )
                {
                    BFFree( rgb );
                    if ( NULL != rgbRecNew )
                    {
                        BFFree( rgbRecNew );
                    }
                    goto HandleError;
                }
                Assert( prceNil != prce );
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

            err = ErrNDReplace( pfucb, &csr, &data, dirflag | fDIRRedo, rceidNull, prceNil );

            BFFree( rgb );
            if ( NULL != rgbRecNew )
            {
                BFFree( rgbRecNew );
            }
            CallS( err );
            Call( err );
        }
            break;

        case lrtypFlagDelete:
        {
            const LRFLAGDELETE  * const plrflagdelete   = (LRFLAGDELETE *)plrnode;

            if ( !fRedoNeeded )
            {

                //  we'll create the version using the undo-info if necessary

                //  add this RCE to the list of uncreated RCEs
                if ( plrflagdelete->FVersioned() )
                {
                    Call( ppib->ErrRegisterDeferredRceid( plrnode->le_rceid, pgno ) );
                }

                CallS( err );
                goto HandleError;
            }

            //  cache node
            //
            Assert( plrflagdelete->ILine() == csr.ILine() );
            Call( ErrNDGet( pfucb, &csr ) );

            //  if we have to redo the operation we have to create the version
            if( plrflagdelete->FVersioned() )
            {
                BOOKMARK    bm;
                RCE         *prce = prceNil;

                NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
                Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operFlagDelete, &prce, &verproxy ) );
                Assert( prceNil != prce );
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

            //  redo operation
            //
            err = ErrNDFlagDelete( pfucb, &csr, dirflag | fDIRRedo, rceidNull, NULL );
            CallS( err );
            Call( err );
        }
            break;

        case lrtypDelete:
        {
            if ( !fRedoNeeded )
            {
                err = JET_errSuccess;
                goto HandleError;
            }

            LRDELETE        *plrdelete = (LRDELETE *) plrnode;

            BOOL fEventloggedLongWait = fFalse;
            TICK tickStart = TickOSTimeCurrent();
            TICK tickEnd;
            while ( fTrue )
            {
                //  cache node
                //
                BOOKMARK    bm;
                Assert( plrdelete->ILine() == csr.ILine() );
                Call( ErrNDGet( pfucb, &csr ) );
                NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

                if ( !FVERActive( pfucb, bm ) )
                {
                    break;
                }

                // release page before waiting to avoid deadlock
                csr.ReleasePage();

                // Assert that we are not holding any locks
                CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );
                m_pinst->m_sigTrxOldestDuringRecovery.FWait( cmsecMaxReplayDelayDueToReadTrx );

                tickEnd = TickOSTimeCurrent();
                if ( !fEventloggedLongWait &&
                     tickEnd - tickStart >= cmsecMaxReplayDelayDueToReadTrx )
                {
                    PERFOpt( cLGRecoveryLongStallReadOnly.Inc( m_pinst ) );

                    WCHAR szTime[16];
                    const WCHAR *rgsz[1] = { szTime };
                    OSStrCbFormatW( szTime, sizeof(szTime), L"%d", tickEnd - tickStart );
                    UtilReportEvent(
                            eventWarning,
                            LOGGING_RECOVERY_CATEGORY,
                            RECOVERY_LONG_WAIT_READ_ONLY,
                            1,
                            rgsz,
                            0,
                            NULL,
                            m_pinst );
                    fEventloggedLongWait = fTrue;
                }

                // re-acquire page latch
                Call( ErrLGIAccessPage( ppib, &csr, ifmp, pgno, plrnode->le_objidFDP, fFalse ) );
                csr.SetILine( plrnode->ILine() );
                csr.UpgradeFromRIWLatch();
            }

            tickEnd = TickOSTimeCurrent();
            if ( !fEventloggedLongWait &&
                 tickEnd - tickStart >= cmsecMaxReplayDelayDueToReadTrx / 10 )
            {
                PERFOpt( cLGRecoveryStallReadOnly.Inc( m_pinst ) );

                WCHAR szTime[16];
                const WCHAR *rgsz[1] = { szTime };
                OSStrCbFormatW( szTime, sizeof(szTime), L"%d", tickEnd - tickStart );
                UtilReportEvent(
                        eventInformation,
                        LOGGING_RECOVERY_CATEGORY,
                        RECOVERY_WAIT_READ_ONLY,
                        1,
                        rgsz,
                        0,
                        NULL,
                        m_pinst,
                        JET_EventLoggingLevelMedium );
            }
            if ( tickEnd != tickStart )
            {
                PERFOpt( cLGRecoveryStallReadOnlyTime.Add( m_pinst, tickEnd - tickStart ) );
            }

            //  redo node delete
            //
            Assert( plrdelete->ILine() == csr.ILine() );
            err = ErrNDDelete( pfucb, &csr, fDIRNull );
            CallS( err );
            Call( err );
        }
            break;


        case lrtypDelta:
        {
            const LRDELTA32 *const plrdelta = (LRDELTA32 *) plrnode;
            Call( ErrLGRIRedoDelta( ppib, pfucb, &csr, verproxy, plrdelta, dirflag, fRedoNeeded ) );

            if ( !fRedoNeeded )
            {
                err = JET_errSuccess;
                goto HandleError;
            }
        }
            break;

        case lrtypDelta64:
        {
            const LRDELTA64 *const plrdelta = (LRDELTA64 *) plrnode;
            Call( ErrLGRIRedoDelta( ppib, pfucb, &csr, verproxy, plrdelta, dirflag, fRedoNeeded ) );

            if ( !fRedoNeeded )
            {
                err = JET_errSuccess;
                goto HandleError;
            }
        }
            break;

        case lrtypSLVPageAppendOBSOLETE:
        case lrtypSLVSpaceOBSOLETE:
        {
            Call( ErrERRCheck( JET_wrnNyi ) );
        }

        case lrtypEmptyTree:
        {
            Call( ErrLGRIRedoFreeEmptyPages( pfucb, (LREMPTYTREE *)plrnode ) );
            if ( fRedoNeeded )
            {
                csr.Dirty();
                NDSetEmptyTree( &csr );
            }
            else
            {
                err = JET_errSuccess;
                goto HandleError;
            }

            break;
        }
    }

    Assert( fRedoNeeded );
    Assert( csr.FDirty() );

    //  the timestamp set in ND operation is not correct so reset it
    //
    csr.SetDbtime( dbtime );

    err = JET_errSuccess;

HandleError:

    AssertSz( errSkipLogRedoOperation != err, "%s() got errSkipLogRedoOperation from ErrLGIAccessPage(%d=%#x).\n",
              __FUNCTION__, (DBTIME) plrnode->le_dbtime, (DBTIME) plrnode->le_dbtime );

    if ( csr.FLatched() )
    {
        Assert( pgno == csr.Pgno() );
        Assert( pagetrimTrimmed != csr.PagetrimState() );

        csr.ReleasePage();
    }

    Assert( !csr.FLatched() );
    
    return err;
}

template< typename TDelta>
ERR LOG::ErrLGRIRedoDelta( PIB * ppib, FUCB * pfucb, CSR * pcsr, VERPROXY verproxy, const _LRDELTA< TDelta > * const plrdelta, const DIRFLAG dirflag, BOOL fRedoNeeded )
{
    ERR err = JET_errSuccess;
    TDelta tDelta = plrdelta->Delta();
    USHORT  cbOffset = plrdelta->CbOffset();

    Assert( plrdelta->ILine() == pcsr->ILine() );

    if ( !( dirflag & fDIRNoVersion ) )
    {
        _VERDELTA< TDelta > verdelta;
        RCE                 *prce = prceNil;

        verdelta.tDelta = tDelta;
        verdelta.cbOffset = cbOffset;
        verdelta.fDeferredDelete = fFalse;
        verdelta.fCallbackOnZero = fFalse;
        verdelta.fDeleteOnZero = fFalse;

        BOOKMARK    bm;
        bm.key.prefix.Nullify();
        bm.key.suffix.SetPv( (void*)plrdelta->rgbData );
        bm.key.suffix.SetCb( plrdelta->CbBookmarkKey() );
        bm.data.SetPv( (BYTE *) plrdelta->rgbData + plrdelta->CbBookmarkKey() );
        bm.data.SetCb( plrdelta->CbBookmarkData() );

        pfucb->kdfCurr.data.SetPv( &verdelta );
        pfucb->kdfCurr.data.SetCb( sizeof( _VERDELTA< TDelta > ) );

        CallR( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, _VERDELTA< TDelta >::TRAITS::oper, &prce, &verproxy ) );
        Assert( prce != prceNil );
        VERInsertRCEIntoLists( pfucb, pcsr, prce );
    }

    if ( fRedoNeeded )
    {
        CallR( ErrNDGet( pfucb, pcsr ) );
        err = ErrNDDelta< TDelta >( pfucb,
            pcsr,
            cbOffset,
            tDelta,
            NULL,
            dirflag | fDIRRedo,
            rceidNull );
        CallS( err );
    }

    return err;
}


ERR LOG::ErrLGRIRedoScrub( const LRSCRUB * const plrscrub )
{
    ERR err = JET_errSuccess;
    PIB *ppib;
    BOOL fSkip;

    CSR csr;
    
    Call( ErrLGRICheckRedoConditionInTrx(
            plrscrub->le_procid,
            plrscrub->dbid,
            plrscrub->le_dbtime,
            plrscrub->le_objidFDP,
            plrscrub,
            &ppib,
            &fSkip ) );
    if ( !fSkip )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[ plrscrub->dbid ];
        Call( ErrLGIAccessPage( ppib, &csr, ifmp, plrscrub->le_pgno, plrscrub->le_objidFDP, fFalse, plrscrub->FUnusedPage() ) );
        const BOOL fRedo = FLGNeedRedoCheckDbtimeBefore( ifmp, csr, plrscrub->le_dbtime, plrscrub->le_dbtimeBefore, &err );
        Call( err );
        if( fRedo )
        {
            csr.UpgradeFromRIWLatch();
            if( plrscrub->FUnusedPage() )
            {
                Call( ErrNDScrubOneUnusedPage( ppib, ifmp, &csr, fDIRRedo ) );
            }
            else
            {
                const SCRUBOPER * const pscrubOper = (SCRUBOPER *) plrscrub->PbData();
                const INT cscrubOper = plrscrub->CscrubOper();
                Call( ErrNDScrubOneUsedPage( ppib, ifmp, &csr, pscrubOper, cscrubOper, fDIRRedo ) );
            }
            CallS( err );
            csr.SetDbtime( plrscrub->le_dbtime );
        }
    }
HandleError:
    csr.ReleasePage();
    return err;
}

//
//  This function never returns succes.
//  This function if the user options do not allow data loss, we will
//  log events to note we failed and that there is a recovery option.
//  If the user does allow data loss, it will fix the log stream such
//  that recovery can continue to create a clean / consistent database.
ERR LOG::ErrLGEvaluateDestructiveCorrectiveLogOptions(
    _In_ const LONG lgenBad,
    _In_ const ERR errCondition     // Note: we expect ErrERRCheck() was already called on this param.
    )
{
    ERR     err = errCondition;
    LONG    lGenFirst = 0;
    LONG    lGenLast = 0;

    //  The errCondition signifies the "type" of badness of the log, either it's missing
    //  in the case of invalid sequence, or its a corruption error ...
    Assert( errCondition < JET_errSuccess );
    Assert( errCondition == JET_errInvalidLogSequence ||
            FErrIsLogCorruption( errCondition ) );

    Call( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, &lGenFirst, &lGenLast ) );

    //
    //  Evaluate if the last good generation was sufficient to satisfy all circumstances ...
    //
    ERR errGenRequiredCheck = ErrLGCheckDBGensRequired( lgenBad - 1 );
    Assert( errGenRequiredCheck != JET_wrnCommittedLogFilesLost );

    if ( errGenRequiredCheck != JET_errCommittedLogFilesMissing )
    {
        //  Nope, we are missing required files for consistency, bail ... we will 
        //  use the errCondition provided below as there is no way to correct for
        //  this error condition.
        Call( errCondition );
    }

    //
    //  ... at this point we know the bad log is only committed, not required.
    //

    if ( !m_fIgnoreLostLogs || !BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) )
    {
        //  User did not indicate we want to lose data.

        //  We've lost or corrupted a log we can recover from, log an event to indicate 
        //  admin may be able to manually recover.

        if ( FErrIsLogCorruption( errCondition ) )
        {
            WCHAR szT1[16];
            const WCHAR * rgszT[3];
            rgszT[0] = m_pLogStream->LogName();
            OSStrCbFormatW( szT1, sizeof( szT1 ), L"%d", errCondition );
            rgszT[1] = szT1;
            rgszT[2] = m_wszLogCurrent;
            UtilReportEvent(    eventError,
                                LOGGING_RECOVERY_CATEGORY,
                                LOG_FILE_CORRUPTED_BUT_HAS_LOSSY_RECOVERY_OPTION_ID,
                                _countof( rgszT ),
                                rgszT,
                                0,
                                NULL,
                                m_pinst );
            
            OSUHAPublishEvent(  HaDbFailureTagRecoveryRedoLogCorruption,
                                m_pinst,
                                HA_LOGGING_RECOVERY_CATEGORY,
                                HaDbIoErrorNone, NULL, 0, 0,
                                HA_LOG_FILE_CORRUPTED_BUT_HAS_LOSSY_RECOVERY_OPTION_ID,
                                _countof( rgszT ),
                                rgszT );

            Call( ErrERRCheck( JET_errCommittedLogFileCorrupt ) );
        }
        else
        {
            Assert( errCondition == JET_errInvalidLogSequence );
            WCHAR wszLastGoodT[ IFileSystemAPI::cchPathMax ];
            WCHAR wszT1[16];
            WCHAR wszT2[16];
            const WCHAR *rgwszT[5];
            m_pLogStream->LGMakeLogName( wszLastGoodT, sizeof(wszLastGoodT), eArchiveLog, lgenBad - 1 );
            rgwszT[0] = wszLastGoodT;
            OSStrCbFormatW( wszT1, sizeof(wszT1), L"%d", lgenBad - 1 );
            rgwszT[1] = wszT1;
            rgwszT[2] = m_pLogStream->LogName();
            OSStrCbFormatW( wszT2, sizeof(wszT2), L"%d", lgenBad );
            rgwszT[3] = wszT2;
            rgwszT[4] = m_wszLogCurrent;
            UtilReportEvent(
                        eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        REDO_COMMITTED_LOGS_OUTOFORDERMISSING_BUT_HAS_LOSSY_RECOVERY_OPTION_ID,
                        sizeof( rgwszT ) / sizeof( rgwszT[ 0 ] ),
                        rgwszT,
                        0,
                        NULL,
                        m_pinst );
            
            OSUHAPublishEvent(  HaDbFailureTagCorruption,
                                m_pinst,
                                HA_LOGGING_RECOVERY_CATEGORY,
                                HaDbIoErrorNone, NULL, 0, 0,
                                HA_REDO_COMMITTED_LOGS_OUTOFORDERMISSING_BUT_HAS_LOSSY_RECOVERY_OPTION_ID,
                                sizeof( rgwszT ) / sizeof( rgwszT[ 0 ] ),
                                rgwszT );

            Call( ErrERRCheck( JET_errCommittedLogFilesMissing ) );
        }
    }

    //
    //  user has instructed us to take corrective action, do so ...
    //

    //  Note we are removing committed data, make very very sure we are removing
    //  only logs we can do without.

    Assert( m_fIgnoreLostLogs );
    Assert( lGenFirst < lgenBad );
    Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration < lgenBad );
    //  If there are more than committed log files missing, quit.
    Assert( ErrLGCheckDBGensRequired( lgenBad - 1 ) == JET_errSuccess ||
            ErrLGCheckDBGensRequired( lgenBad - 1 ) == JET_errCommittedLogFilesMissing );
    if ( !m_fIgnoreLostLogs ||
        ( lGenFirst >= lgenBad ) ||
        ( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= lgenBad ) ||
        ( ErrLGCheckDBGensRequired( lgenBad - 1 ) != JET_errSuccess &&
          ErrLGCheckDBGensRequired( lgenBad - 1 ) != JET_errCommittedLogFilesMissing ) )
    {
        AssertSz( fFalse, " This would be an unexpected condition." );
        Call( errCondition );   // punt
    }


    //
    //  Ok we belive we are ok to start removing data ...
    //
    if ( FErrIsLogCorruption( errCondition ) )
    {
        //  Corruption event here, missing / out of sequence event in ErrLGRRestartLOGAtLogGen().
        WCHAR szT1[16];
        WCHAR szT2[16];
        WCHAR szT3[16];
        const WCHAR * rgszT[5];
        OSStrCbFormatW( szT1, sizeof( szT1 ), L"0x%x", lgenBad - 1 );
        rgszT[0] = szT1;
        rgszT[1] = m_pLogStream->LogName();
        OSStrCbFormatW( szT2, sizeof( szT2 ), L"0x%x", lgenBad );
        rgszT[2] = szT2;
        OSStrCbFormatW( szT3, sizeof( szT3 ), L"%d", errCondition );
        rgszT[3] = szT3;
        rgszT[4] = m_wszLogCurrent;
        //  Note we are cheating a little by reporting this event here ... we actually
        //  don't remove this log file until ErrLGIRemoveCommittedLogs() if its an archive
        //  log, but I think its ok to pre-log it.
        UtilReportEvent(    eventWarning,
                            LOGGING_RECOVERY_CATEGORY,
                            REDO_CORRUPT_COMMITTED_LOG_REMOVED_ID,
                            _countof( rgszT ),
                            rgszT,
                            0,
                            NULL,
                            m_pinst );
    }

    CallR( m_pLogStream->ErrLGRRestartLOGAtLogGen( lgenBad, ( errCondition == JET_errInvalidLogSequence ) ) );
    
    //  So while for backup (above) we remove the remaining archive log files at 
    //  the very beginning of recovery in ErrLGSoftStart() for LLR based recovery 
    //  it is easier to do so at the exact discovery of out of sequene log files.

    CallR( m_pLogStream->ErrLGRemoveCommittedLogs( lgenBad ) );

    //  We've recovered, cover up error ... 
    m_fLostLogs = fTrue;

    err = ErrERRCheck( JET_wrnCommittedLogFilesRemoved );

    // We are not fixing up committed generation in db header(s) here, but ErrLGCheckGenMaxRequired should run later
    // and fix it.

HandleError:

    Assert( err != JET_errSuccess );

    if ( err < JET_errSuccess &&
            err != JET_errCommittedLogFilesMissing &&
            err != JET_errCommittedLogFileCorrupt )
    {
        //  If we failed for any reason except missing or corrupt committed logs, we report 
        //  the original condition that caused us to try this path ...
        err = errCondition;
    }

    return err;
}

#define fNSGotoDone     1
#define fNSGotoCheck    2

#define FSameTime( ptm1, ptm2 )     ( memcmp( (ptm1), (ptm2), sizeof(LOGTIME) ) == 0 )

ERR LOG::ErrLGRedoFill( LR **pplr, BOOL fLastLRIsQuit, INT *pfNSNextStep )
{
    ERR     err, errT = JET_errSuccess;
    LONG    lgen;
    BOOL    fCloseNormally;
    LOGTIME tmOldLog;
    m_pLogStream->GetCurrentFileGen( &tmOldLog );
    LE_LGPOS   le_lgposFirstT;
    BOOL    fJetLog = fFalse;   // this means we've opened the current edb.jtx/log file...
    WCHAR   wszOldLogName[ IFileSystemAPI::cchPathMax ];
    LGPOS   lgposOldLastRec = m_pLogReadBuffer->LgposFileEnd();
    QWORD checksumAccPrevLog = m_pLogStream->GetAccumulatedSectorChecksum();

    /*  end of redoable logfile, read next generation
    /**/
    if ( m_pLogStream->FLGRedoOnCurrentLog() )
    {
        Assert( m_wszLogCurrent != m_wszRestorePath );

        /*  we have done all the log records
        /**/
        *pfNSNextStep = fNSGotoDone;
        OnNonRTM( m_fRedidAllLogs = fTrue );
        err = JET_errSuccess;
        goto CheckGenMaxReq;
    }

    /* close current logfile, open next generation */
    m_pLogStream->LGCloseFile();

    OSStrCbCopyW( wszOldLogName, sizeof( wszOldLogName ), m_pLogStream->LogName() );

    lgen = m_pLogStream->GetCurrentFileGen() + 1;

    // if we replayed all logs from target directory
    if ( m_wszLogCurrent == m_wszTargetInstanceLogPath && m_lGenHighTargetInstance == m_pLogStream->GetCurrentFileGen() )
    {
        Assert ( L'\0' != m_wszTargetInstanceLogPath[0] );
        // we don't have to try nothing more
        err = ErrERRCheck( JET_errFileNotFound );
        goto NoMoreLogs;
    }

    m_fLGFMPLoaded = fTrue;
    (VOID)ErrLGUpdateCheckpointFile( fFalse );
    m_fLGFMPLoaded = fFalse;

    err = m_pLogStream->ErrLGTryOpenLogFile( JET_OpenLogForRedo, eArchiveLog, lgen );

    if ( err == JET_errFileNotFound )
    {
        if ( m_wszLogCurrent == m_wszRestorePath || m_wszLogCurrent == m_wszTargetInstanceLogPath )
        {
NoMoreLogs:
            // if we have a target instance or we didn't replayed those
            // try that directory
            if ( m_wszTargetInstanceLogPath[0] && m_wszLogCurrent != m_wszTargetInstanceLogPath )
                m_wszLogCurrent = m_wszTargetInstanceLogPath;
            // try instance directory
            else
                m_wszLogCurrent = SzParam( m_pinst, JET_paramLogFilePath );

            err = m_pLogStream->ErrLGTryOpenLogFile( JET_OpenLogForRedo, eArchiveLog, lgen );
        }

    }

    if ( err == JET_errFileNotFound )
    {
        //  open failed, try the current log name (edb.jtx/log)

        err = m_pLogStream->ErrLGTryOpenLogFile( JET_OpenLogForRedo, eCurrentLog, lgen );

        if ( JET_errSuccess == err )
        {
            fJetLog = fTrue;
        }
    }

    if ( err < 0 )
    {
        //  If open next generation fail and we haven't finished
        //  all the backup logs, then return as abrupt end.

        if ( err == JET_errFileNotFound )
        {
            // if we need to stop at a certain log / position, 
            // we should not start generating a new log 
            // Note: becuase we check the existance of the
            // upper log specified on startup, we won't hit this
            // unless there is a gap in the log series
            // (like we have logs 3,4 and 6 (with checkpoint at 3)
            // and the stop position is set to 6.
            // 
            if ( FLGRecoveryLgposStop() )
            {
                Assert( m_lgposRecoveryStop.lGeneration >= ( lgen - 1 ) );

                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"07dd83a7-c6e8-413a-beb9-e934ea597918" );
                return ErrERRCheck( JET_errMissingLogFile );
            }

            // Also, we should not start a new log if we might hit a gap in the logs, so we need
            // to check if we actually reached the end (and there is no edb.log)
            // or we just hit a gap. We will return specific errors as such.
            // 
            LONG    lgenHigh = 0;
            
            CallR ( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenHigh ) );

            // We can't do this in hard restore/recovery because of the different
            // path (m_wszTargetInstanceLogPath / m_wszLogCurrent) garbage ...
            if ( !m_fHardRestore && ( lgenHigh == 0 || lgenHigh > lgen ) )
            {
                WCHAR szT1[16];
                WCHAR szT2[16];
                const WCHAR *rgszT[2];
                OSStrCbFormatW( szT1, sizeof( szT1 ), L"0x%x", lgen );
                rgszT[0] = szT1;
                OSStrCbFormatW( szT2, sizeof( szT2 ), L"0x%x", lgenHigh );
                rgszT[1] = szT2;
                UtilReportEvent( eventError,
                                 LOGGING_RECOVERY_CATEGORY,
                                 GAP_LOG_FILE_GEN_ID,
                                 _countof( rgszT ),
                                 rgszT,
                                 0,
                                 NULL,
                                 m_pinst );

                OSUHAPublishEvent( HaDbFailureTagCorruption,
                                   m_pinst,
                                   HA_LOGGING_RECOVERY_CATEGORY,
                                   HaDbIoErrorNone, NULL, 0, 0,
                                   HA_GAP_LOG_FILE_GEN_ID,
                                   _countof( rgszT ),
                                   rgszT );

                return ErrERRCheck( JET_errMissingLogFile );
            }

            // we can not create a new log file if a log file more then the one we
            // just closed (which is the last we have) is needed

            // SOFT_HARD: ??? do we need the check here? We might need the checks in the functions anyway
            if ( m_fHardRestore || m_fIgnoreLostLogs )
            {
                err = ErrLGCheckDBGensRequired( lgen - 1 );
                if ( m_fIgnoreLostLogs &&
                        ( err == JET_errSuccess || err == JET_errCommittedLogFilesMissing ) )
                {
                    //  Call it success ...
                    // we are just missing the last log, so no logs deleted, but lost data.
                    m_pLogStream->ResetRemovedLogs();
                    m_fLostLogs = ( err == JET_errCommittedLogFilesMissing );
                    err = JET_errSuccess;
                }
                CallR( err );
            }

            //  Traditionally if we are missing the edbtmp.log in addition to edb.log, then
            //  we report JET_errMisssingLogFile as NTFS is never supposed to allow us
            //  to get into that state due to the order of operations in how we roll a
            //  log file.
            const ERR errMissingCurrentLog = m_fHardRestore || m_pLogStream->FTempLogExists() ? JET_errSuccess : ErrERRCheckDefault( JET_errMissingLogFile );

            err = ErrLGOpenLogMissingCallback( m_pinst, m_pLogStream->LogName(), errMissingCurrentLog, lgen, fTrue, JET_MissingLogContinueToUndo );
            Assert( err != JET_errFileNotFound );
            CallR( err );

            OnNonRTM( m_fRedidAllLogs = fTrue );

            m_pLogStream->LGMakeLogName( eArchiveLog, lgen - 1 );

            *pfNSNextStep = fNSGotoDone;
            return JET_errSuccess;
        }

        /* Open Fails */
        Assert( !m_pLogStream->FLGFileOpened() );
        return err;
    }

    //  We got the next log to play, but if last one is abruptly ended
    //  we want to stop here.

    if ( m_fAbruptEnd )
    {
        WCHAR szT1[16];
        WCHAR szT2[16];
        const WCHAR *rgszT[3];
AbruptEnd:
        rgszT[0] = wszOldLogName;
        OSStrCbFormatW( szT1, sizeof( szT1 ), L"%d", lgposOldLastRec.isec );
        rgszT[1] = szT1;
        OSStrCbFormatW( szT2, sizeof( szT2 ), L"%d", lgposOldLastRec.ib );
        rgszT[2] = szT2;
        UtilReportEvent(    eventError,
                            LOGGING_RECOVERY_CATEGORY,
                            REDO_END_ABRUPTLY_ERROR_ID,
                            _countof( rgszT ),
                            rgszT,
                            0,
                            NULL,
                            m_pinst );

        OSUHAPublishEvent(  HaDbFailureTagCorruption,
                            m_pinst,
                            HA_LOGGING_RECOVERY_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_REDO_END_ABRUPTLY_ERROR_ID,
                            _countof( rgszT ),
                            rgszT );

        return ErrERRCheck( JET_errRedoAbruptEnded );
    }

    // We might have transitioned to verbose redo (transition to active) while waiting to open this log, verify.
    if ( !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoverySilentRedoDone ) )
    {
        BOOL fCallback = ( ErrLGNotificationEventConditionCallback( m_pinst, STATUS_REDO_ID ) >= JET_errSuccess );
        if ( fCallback )
        {
            m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoverySilentRedoEnd = m_lgposRedo;
            m_pinst->m_isdlInit.Trigger( eInitLogRecoverySilentRedoDone );
        }
    }

    // save the previous log header
    m_pLogStream->SaveCurrentFileHdr();

    /* reset the log buffers */
    CallR( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID ) );

    const ULONG ulMajorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor;
    const ULONG ulMinorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor;
    const ULONG ulUpdateMajorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor;
    const ULONG ulUpdateMinorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMinor;

    //  We can not make this invalid log sequence only based on the fact
    //  that time is the same because the granularity is only 1 second,
    //  so its possible to have an unexpected log generation that is at
    //  the same expected time.  We should base this on the fact that
    //  this generation is +1 from the last generation.
    if ( !FSameTime( &tmOldLog, &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen ) ||
         ( lgen != m_pLogStream->GetCurrentFileGen() ) )
    {

        //  we found a edb.jtx/log older, which must be deleted if requested by the user
        //

        //  It wouldn't make sense to use both of these bits at the same time ...
        Assert( !m_fIgnoreLostLogs || !m_fHardRestore );

        // SOFT_HARD: the per SG one should do it
        if ( m_fHardRestore && BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) && fJetLog && m_pLogStream->GetCurrentFileGen() < lgen )
        {

            #ifdef DEBUG
            WCHAR   wszNameDebug[IFileSystemAPI::cchPathMax];
            m_pLogStream->LGMakeLogName( wszNameDebug, sizeof(wszNameDebug), eArchiveLog );
            Assert( 0 == LOSStrCompareW( wszNameDebug, m_pLogStream->LogName() ) );
            #endif

            CallR( m_pLogStream->ErrLGRRestartLOGAtLogGen( lgen, fTrue ) );
            fJetLog = fFalse;
            *pfNSNextStep = fNSGotoDone;
            return err;
        }

        CallR( ErrLGEvaluateDestructiveCorrectiveLogOptions( lgen, ErrERRCheck( JET_errInvalidLogSequence ) ) );

        //  If we exited ErrLGEvaluateDestructiveCorrectiveLogOptions() with a non-failing
        //  err it means we've taken corrective action ... signal done and return.
        Assert( err == JET_wrnCommittedLogFilesRemoved );
        // Note this is like m_fRedidAllLogs, but we didn't.
        Assert( m_fLostLogs );
        err = JET_errSuccess;
        *pfNSNextStep = fNSGotoDone;
        return err;
    }

    // If the log format supports accumulated segment checksum, then we can check log sequence integrity using the checksum and return error on it.

    if ( FLogFormatUnifiedMinorUpdateVersion( ulMajorCurrLog, ulMinorCurrLog, ulUpdateMajorCurrLog, ulUpdateMinorCurrLog ) &&
        ulUpdateMinorCurrLog >= ulLGVersionUpdateMinorPrevGenAllSegmentChecksum &&
        checksumAccPrevLog != m_pLogStream->GetCurrentFileHdr()->lgfilehdr.checksumPrevLogAllSegments )
    {
        WCHAR szT1[20];
        WCHAR szT2[20];
        const WCHAR *rgszT[3];

        rgszT[0] = m_pLogStream->LogName();
        OSStrCbFormatW( szT1, sizeof( szT1 ), L"0x%016I64x", checksumAccPrevLog );
        rgszT[1] = szT1;
        OSStrCbFormatW( szT2, sizeof( szT2 ), L"0x%016I64x", (XECHECKSUM) m_pLogStream->GetCurrentFileHdr()->lgfilehdr.checksumPrevLogAllSegments );
        rgszT[2] = szT2;
        UtilReportEvent(    eventError,
                            LOGGING_RECOVERY_CATEGORY,
                            LOG_LAST_SEGMENT_CHECKSUM_MISMATCH_ID,
                            _countof( rgszT ),
                            rgszT,
                            0,
                            NULL,
                            m_pinst );

        OSUHAPublishEvent(  HaDbFailureTagCorruption,
                            m_pinst,
                            HA_LOGGING_RECOVERY_CATEGORY,
                            HaDbIoErrorNone, NULL, 0, 0,
                            HA_LOG_LAST_SEGMENT_CHECKSUM_MISMATCH_ID,
                            _countof( rgszT ),
                            rgszT );

        return ErrERRCheck( JET_errLogSequenceChecksumMismatch );
    }

    if( CmpLogFormatVersion( FmtlgverLGCurrent( this ), fmtlgverInitialDbSizeLoggedMain ) < 0 )
    {
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
            {
                continue;
            }
            //  should we insist anything else, like that it is attached?
            FMP * const pfmp = &g_rgfmp[ ifmp ];
            pfmp->SetOlderDemandExtendDb();
        }
    }

    le_lgposFirstT.le_lGeneration = m_pLogStream->GetCurrentFileGen();
    le_lgposFirstT.le_isec = (WORD)m_pLogStream->CSecHeader();
    le_lgposFirstT.le_ib = 0;

    m_pLogReadBuffer->ResetLgposFileEnd();

    //  scan the log to find traces of corruption before going record-to-record
    //  if any corruption is found, an error will be returned
    err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue );
    if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
    {

        if ( FErrIsLogCorruption( err ) )
        {
            err = ErrLGEvaluateDestructiveCorrectiveLogOptions( lgen, ErrERRCheck( err ) );
            if ( err >= JET_errSuccess )
            {
                //  If we exited ErrLGEvaluateDestructiveCorrectiveLogOptions() with a non-failing
                //  err it means we've taken corrective action ... signal done and return.
                Assert( err == JET_wrnCommittedLogFilesRemoved );
                Assert( m_fLostLogs );
                // Note this is like m_fRedidAllLogs, but we didn't.
                *pfNSNextStep = fNSGotoDone;
                err = JET_errSuccess;
                return err;
            }
        }

        errT = err;
    }
    else
    {
        Assert( err < 0 );
        CallR( err );
    }

    //  Check if abrupt end if the file size is not the same as recorded.

    QWORD cbSize;
    CallR( m_pLogStream->ErrFileSize( &cbSize ) );
    if ( cbSize != m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec * QWORD( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile ) )
    {
        m_fAbruptEnd = fTrue;
    }

    //  now scan the first record
    //  there should be no errors about corruption since they'll be handled by ErrLGCheckReadLastLogRecordFF
    CallR( m_pLogReadBuffer->ErrLGLocateFirstRedoLogRecFF( &le_lgposFirstT, (BYTE **)pplr ) );
    //  we expect no warnings -- only success at this point
    CallS( err );
    *pfNSNextStep = fNSGotoCheck;

    //  If log is not end properly and we haven't finished
    //  all the backup logs, then return as abrupt end.

    //  UNDONE: for soft recovery, we need some other way to know
    //  UNDONE: if the record is played up to the crashing point.

    // SOFT_HARD: ??? what to do here, not a big deal most likely, per SG should be fine
    if ( m_fHardRestore && lgen <= m_lGenHighRestore
         && m_fAbruptEnd )
    {
        goto AbruptEnd;
    }

CheckGenMaxReq:
    // check the genMaxReq+genMaxCreateTime of this log with all dbs
    if ( m_fRecovering )
    {
        LOGTIME tmEmpty;

        memset( &tmEmpty, '\0', sizeof( LOGTIME ) );

        for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
        {
            const IFMP  ifmp    = m_pinst->m_mpdbidifmp[ dbidT ];

            if ( ifmp >= g_ifmpMax )
                continue;

            FMP *       pfmpT   = &g_rgfmp[ ifmp ];

            Assert( !pfmpT->FReadOnlyAttach() );
            if ( pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach() )
            {
                //  skipped attachments is a restore-only concept
                // SOFT_HARD: whatever, the assert is kind of weak anyway
                Assert( !pfmpT->FSkippedAttach() || m_fHardRestore || m_irstmapMac || !m_fReplayMissingMapEntryDB );
                continue;
            }

            LONG lGenCommitted = pfmpT->Pdbfilehdr()->le_lGenMaxCommitted;
            LONG lGenRequired = pfmpT->Pdbfilehdr()->le_lGenMaxRequired;
            LOGTIME tmCreate;
            LONG lGenCurrent = m_pLogStream->GetCurrentFileGen( &tmCreate );
            BOOL fLogtimeGenMaxRequiredEfvEnabled = ( pfmpT->ErrDBFormatFeatureEnabled( JET_efvLogtimeGenMaxRequired ) == JET_errSuccess );

            PdbfilehdrReadOnly pdbfilehdr = pfmpT->Pdbfilehdr();

            // if the time of the log file does not match
            if ( !m_fIgnoreLostLogs &&
                 lGenRequired == lGenCurrent )
            {
                LOGTIME tmLog;
                if ( fLogtimeGenMaxRequiredEfvEnabled )
                {
                    memcpy( &tmLog, &pdbfilehdr->logtimeGenMaxRequired, sizeof( LOGTIME ) );
                }
                else
                {
                    memcpy( &tmLog, &tmEmpty, sizeof( LOGTIME ) );
                }
                if ( memcmp( &tmLog, &tmEmpty, sizeof( LOGTIME ) ) == 0 && lGenRequired == lGenCommitted )
                {
                    memcpy( &tmLog, &pdbfilehdr->logtimeGenMaxCreate, sizeof( LOGTIME ) );
                }
                if ( memcmp( &tmLog, &tmEmpty, sizeof( LOGTIME ) ) &&
                     memcmp( &tmLog, &tmCreate, sizeof( LOGTIME ) ) )
                {
                    WCHAR szT2[32];
                    WCHAR szT3[32];
                    const WCHAR *rgszT[4];

                    rgszT[0] = pfmpT->WszDatabaseName();
                    rgszT[1] = m_pLogStream->LogName();

                    OSStrCbFormatW( szT2, sizeof( szT2 ), L" %02d/%02d/%04d %02d:%02d:%02d.%3.3d ",
                            (SHORT) tmLog.bMonth, (SHORT) tmLog.bDay, (SHORT) tmLog.bYear + 1900,
                            (SHORT) tmLog.bHours, (SHORT) tmLog.bMinutes, (SHORT) tmLog.bSeconds, (SHORT) tmLog.Milliseconds());
                    rgszT[2] = szT2;

                    OSStrCbFormatW( szT3, sizeof( szT3 ), L" %02d/%02d/%04d %02d:%02d:%02d.%3.3d ",
                            (SHORT) tmCreate.bMonth, (SHORT) tmCreate.bDay, (SHORT) tmCreate.bYear + 1900,
                            (SHORT) tmCreate.bHours, (SHORT) tmCreate.bMinutes, (SHORT) tmCreate.bSeconds, (SHORT) tmCreate.Milliseconds());
                    rgszT[3] = szT3;

                    UtilReportEvent(
                        eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        REDO_HIGH_LOG_MISMATCH_ERROR_ID,
                        sizeof( rgszT ) / sizeof( rgszT[ 0 ] ),
                        rgszT,
                        0,
                        NULL,
                        m_pinst );
                
                    OSUHAPublishEvent(  HaDbFailureTagCorruption,
                                    m_pinst,
                                    HA_LOGGING_RECOVERY_CATEGORY,
                                    HaDbIoErrorNone, NULL, 0, 0,
                                    HA_REDO_HIGH_LOG_MISMATCH_ERROR_ID,
                                    sizeof( rgszT ) / sizeof( rgszT[ 0 ] ),
                                    rgszT );

                    return ErrERRCheck( JET_errRequiredLogFilesMissing );
                }
            }
        }
    }

    //  we should have success at this point
    CallS( err );

    //  return the error from ErrLGCheckReadLastLogRecordFF
    return errT;
}

ERR LOG::ErrLGRIPreSetupFMPFromAttach(
                PIB                     *ppib,
                const ATTACHINFO *      pAttachInfo )
{
    ERR         err             = JET_errSuccess;
    CAutoWSZ    wszDbName;
 
    Assert ( pAttachInfo );
    Assert ( pAttachInfo->CbNames() > 0 );

    if ( pAttachInfo->FUnicodeNames() )
    {
        Call( wszDbName.ErrSet( (UnalignedLittleEndian<WCHAR>*)pAttachInfo->szNames ) );
    }
    else
    {
        Call( wszDbName.ErrSet( (CHAR*)(pAttachInfo->szNames) ) );
    }

    // we are looking into the map for hard restore or if the map is present 
    // (Note: JetRestore = hard restore and no map)
    // We will replace the map entry with the names we setup 

    // SOFT_HARD: restore map should be enough
    if ( m_fHardRestore || m_irstmapMac > 0 )
    {
        err = ErrReplaceRstMapEntryBySignature( wszDbName, &pAttachInfo->signDb );
        if ( JET_errFileNotFound == err )
        {
            //  database not in restore map, so it won't be restored
            //
            err = JET_errSuccess;
        }
        Call( err );
    }

HandleError:
    return err;

}

ERR LOG::ErrLGRISetupFMPFromAttach(
                PIB                     *ppib,
                const ATTACHINFO *      pAttachInfo,
                LGSTATUSINFO *          plgstat,
                IFMP*                   pifmp,
                INT*                    pirstmap )
{
    ERR         err                             = JET_errSuccess;
    CAutoWSZ    wszDbName;
    BOOL        fSkippedAttach                  = fFalse;
    INT         irstmap                         = -1;
    IFMP        ifmp                            = ifmpNil;
    RSTMAP*     psrtmap                         = NULL;
    ULONG       pctCachePriority                = g_pctCachePriorityUnassigned;
    JET_GRBIT   grbitShrinkDatabaseOptions      = NO_GRBIT;
    LONG        dtickShrinkDatabaseTimeQuota    = -1;
    CPG         cpgShrinkDatabaseSizeLimit      = 0;
    BOOL        fLeakReclaimerEnabled           = fFalse;
    LONG        dtickLeakReclaimerTimeQuota     = -1;


    pifmp = pifmp ? pifmp : &ifmp;
    pirstmap = pirstmap ? pirstmap : &irstmap;

    Assert ( pAttachInfo );
    Assert ( pAttachInfo->CbNames() > 0 );

    if ( pAttachInfo->FUnicodeNames() )
    {
        Call( wszDbName.ErrSet( pAttachInfo->szNames ) );
    }
    else
    {
        Call( wszDbName.ErrSet( (CHAR*)(pAttachInfo->szNames) ) );
    }

    //  attach the database specified in restore map.
    //
    err = ErrGetDestDatabaseName( (WCHAR*)wszDbName, m_wszRestorePath, m_wszNewDestination, pirstmap, plgstat );
    if ( JET_errFileNotFound == err )
    {
        //  if a restore map was provided but the database could
        //  not be matched to an entry in the restore map, then
        //  set to skip it if we are not set to replay at default
        //  location all the entries missing from the map (which
        //  is true for JetInit and JetInit2)
        //
        if ( m_irstmapMac > 0 && !m_fReplayMissingMapEntryDB )
        {
            fSkippedAttach = fTrue;
        }
        err = JET_errSuccess;
        *pirstmap = -1;
    }
    else
    {
        Call( err );

        // we found a new name, we will use it later
        Assert( *pirstmap >= 0 );
        // szDbName = m_rgrstmap[irstmap].wszNewDatabaseName;
    }

    psrtmap = ( *pirstmap >= 0 ) ? &m_rgrstmap[ *pirstmap ] : NULL;

    //  Process database parameters.

    Call( ErrDBParseDbParams(
                psrtmap ? psrtmap->rgsetdbparam : NULL,
                psrtmap ? psrtmap->csetdbparam : 0,
                NULL,                           // JET_dbparamDbSizeMaxPages (not used here).
                &pctCachePriority,              // JET_dbparamCachePriority.
                &grbitShrinkDatabaseOptions,    // JET_dbparamShrinkDatabaseOptions.
                &dtickShrinkDatabaseTimeQuota,  // JET_dbparamShrinkDatabaseTimeQuota.
                &cpgShrinkDatabaseSizeLimit,    // JET_dbparamShrinkDatabaseSizeLimit.
                &fLeakReclaimerEnabled,         // JET_dbparamLeakReclaimerEnabled.
                &dtickLeakReclaimerTimeQuota,   // JET_dbparamLeakReclaimerTimeQuota.
                NULL                            // JET_dbparamMaintainExtentPageCountCache (not used here).
                ) );

    //  Get one free fmp entry

    Call( FMP::ErrNewAndWriteLatch(
                    pifmp,
                    psrtmap ? psrtmap->wszNewDatabaseName : (WCHAR*)wszDbName,
                    ppib,
                    m_pinst,
                    m_pinst->m_pfsapi,
                    pAttachInfo->Dbid(),
                    fTrue,
                    fTrue,
                    NULL ) );

    // we should not see double attach
    // during recovery
    //
    if ( err == JET_wrnDatabaseAttached )
    {
        // it is ok to return as there is no
        // action in ErrNewAndWriteLatch if 
        // JET_wrnDatabaseAttached was returned
        //
        Call( ErrERRCheck( JET_errDatabaseDuplicate ) );
    }

    FMP     *pfmpT;
    pfmpT = &g_rgfmp[ *pifmp ];

    Assert( !pfmpT->Pfapi() );
    Assert( NULL == pfmpT->Pdbfilehdr() );
    Assert( pfmpT->FInUse() );
    Assert( pfmpT->Dbid() == pAttachInfo->Dbid() );

    //  get logging/versioning flags (versioning can only be disabled if logging is disabled)
    pfmpT->ResetReadOnlyAttach();
    pfmpT->ResetVersioningOff();
    pfmpT->ResetDeferredAttach();

    pfmpT->SetPctCachePriorityFmp( pctCachePriority );
    pfmpT->SetShrinkDatabaseOptions( grbitShrinkDatabaseOptions );
    pfmpT->SetShrinkDatabaseTimeQuota( dtickShrinkDatabaseTimeQuota );
    pfmpT->SetShrinkDatabaseSizeLimit( cpgShrinkDatabaseSizeLimit );
    pfmpT->SetLeakReclaimerEnabled( fLeakReclaimerEnabled );
    pfmpT->SetLeakReclaimerTimeQuota( dtickLeakReclaimerTimeQuota );

    FMP::EnterFMPPoolAsWriter();
    pfmpT->SetLogOn();
    FMP::LeaveFMPPoolAsWriter();

    if ( fSkippedAttach )
    {
        pfmpT->SetSkippedAttach();
    }
    LGPOS   lgposConsistent;
    LGPOS   lgposAttach;

    lgposConsistent = pAttachInfo->le_lgposConsistent;
    lgposAttach = pAttachInfo->le_lgposAttach;

    //  get lgposAttch
    err = ErrLGRISetupAtchchk(
                *pifmp,
                &pAttachInfo->signDb,
                &m_signLog,
                &lgposAttach,
                &lgposConsistent,
                pAttachInfo->Dbtime(),
                pAttachInfo->ObjidLast(),
                pAttachInfo->CpgDatabaseSizeMax(),
                pAttachInfo->FSparseEnabledFile() );

    //  setup replay options
    if ( psrtmap && ( psrtmap->grbit & JET_bitRestoreMapIgnoreWhenMissing ) )
    {
        pfmpT->SetIgnoreDeferredAttach();
    }
    else
    {
        pfmpT->ResetIgnoreDeferredAttach();
    }
    if ( psrtmap && ( psrtmap->grbit & JET_bitRestoreMapFailWhenMissing ) )
    {
        pfmpT->SetFailFastDeferredAttach();
    }
    else
    {
        pfmpT->ResetFailFastDeferredAttach();
    }
    if ( psrtmap && ( psrtmap->grbit & JET_bitRestoreMapOverwriteOnCreate ) )
    {
        pfmpT->SetOverwriteOnCreate();
    }
    else
    {
        pfmpT->ResetOverwriteOnCreate();
    }

    pfmpT->ReleaseWriteLatch( ppib );
    
HandleError:
    return err;
}

ERR ErrLGRISetupAtchchk(
    const IFMP                  ifmp,
    const SIGNATURE             *psignDb,
    const SIGNATURE             *psignLog,
    const LGPOS                 *plgposAttach,
    const LGPOS                 *plgposConsistent,
    const DBTIME                dbtime,
    const OBJID                 objidLast,
    const CPG                   cpgDatabaseSizeMax,
    const BOOL                  fSparseEnabledFile )
{
    FMP             *pfmp       = &g_rgfmp[ifmp];
    ATCHCHK         *patchchk;

    Assert( NULL != pfmp );

    if ( pfmp->Patchchk() == NULL )
    {
        patchchk = static_cast<ATCHCHK *>( PvOSMemoryHeapAlloc( sizeof( ATCHCHK ) ) );
        if ( NULL == patchchk )
            return ErrERRCheck( JET_errOutOfMemory );
        pfmp->SetPatchchk( patchchk );
    }
    else
    {
        patchchk = pfmp->Patchchk();
    }

    patchchk->signDb = *psignDb;
    patchchk->signLog = *psignLog;
    patchchk->lgposAttach = *plgposAttach;
    patchchk->lgposConsistent = *plgposConsistent;
    patchchk->SetDbtime( dbtime );
    patchchk->SetObjidLast( objidLast );
    patchchk->SetCpgDatabaseSizeMax( cpgDatabaseSizeMax );
    patchchk->SetFSparseEnabledFile( fSparseEnabledFile );
    pfmp->SetLgposAttach( *plgposAttach );

    return JET_errSuccess;
}

LOCAL VOID LGRIReportUnableToReadDbHeader(
    const INST * const  pinst,
    const ERR           err,
    const WCHAR *       szDbName )
{
    //  even though this is a warning, suppress it
    //  if information events are suppressed
    //
    if ( !BoolParam( pinst, JET_paramNoInformationEvent ) )
    {
        WCHAR       szT1[16];
        const WCHAR * rgszT[2];

        rgszT[0] = szDbName;
        OSStrCbFormatW( szT1, sizeof( szT1 ), L"%d", err );
        rgszT[1] = szT1;

        UtilReportEvent(
                eventWarning,
                LOGGING_RECOVERY_CATEGORY,
                RESTORE_DATABASE_READ_HEADER_WARNING_ID,
                2,
                rgszT,
                0,
                NULL,
                pinst );
    }
}

//  Redo need access to some private functions in DB to determine / perform a correct
//  attachment process.
VOID DBISetHeaderAfterAttach( DBFILEHDR_FIX * const pdbfilehdr, const LGPOS lgposAttach, const LOGTIME * const plogtimeOfGenWithAttach, const IFMP ifmp, const BOOL fKeepBackupInfo );
ERR ErrDBIValidateUserVersions(
    _In_    const INST* const               pinst,
    _In_z_  const WCHAR* const              wszDbFullName,
    _In_    const IFMP                      ifmp,
    _In_    const DBFILEHDR_FIX * const     pdbfilehdr,
    _Out_   const FormatVersions ** const   ppfmtversDesired,
    _Out_   BOOL * const                    pfDbNeedsUpdate = NULL );


ERR LOG::ErrLGRICheckRedoCreateDb(
    const IFMP                  ifmp,
    DBFILEHDR                   *pdbfilehdr,
    REDOATTACH                  *predoattach )
{
    ERR             err = JET_errSuccess;
    FMP *           pfmp        = &g_rgfmp[ifmp];
    ATCHCHK *       patchchk    = pfmp->Patchchk();
    const WCHAR *   wszDbName   = pfmp->WszDatabaseName();
    eDeferredAttachReason reason;
    ERR             errIO       = JET_errSuccess;

    OSTrace( JET_tracetagDatabases, __FUNCTION__ );

    Assert( NULL != pfmp );
    Assert( NULL == pfmp->Pdbfilehdr() );
    Assert( NULL != patchchk );
    Assert( NULL != wszDbName );
    Assert( NULL != pdbfilehdr );
    Assert( NULL != predoattach );

    Assert( !pfmp->FReadOnlyAttach() );

    //  presume a deferred attachment
    *predoattach = redoattachDefer;
    reason = eDARUnknown;

    err = ErrUtilReadShadowedHeader(
            m_pinst,
            m_pinst->m_pfsapi,
            wszDbName,
            (BYTE*)pdbfilehdr,
            g_cbPage,
            OffsetOf( DBFILEHDR, le_cbPageSize ) );

    if ( err >= JET_errSuccess && pdbfilehdr->Dbstate() == JET_dbstateJustCreated )
    {
// ***  deletion now performed in ErrDBCreateDatabase() via JET_bitDbOverwriteExisting
// ***  CallR( ErrIODeleteDatabase( pfsapi, ifmp ) );

        *predoattach = redoattachCreate;
        err = JET_errSuccess;
        goto HandleError;
    }

    //  never replay against a database that is in the middle of an incremental reseed.
    //
    else if ( err >= JET_errSuccess && pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress )
    {
        reason = eDARIncReseedInProgress;
        if ( pfmp->FIgnoreDeferredAttach() )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"27914370-320b-4a4a-ac63-057afc9951a1" );
            err = ErrERRCheck( JET_errDatabaseIncompleteIncrementalReseed );
            goto HandleError;
        }
    }
    //  never replay against a database that is in the middle of a revert.
    //
    else if ( err >= JET_errSuccess && pdbfilehdr->Dbstate() == JET_dbstateRevertInProgress )
    {
        reason = eDARRevertInProgress;
        if ( pfmp->FIgnoreDeferredAttach() )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"3ceab1f6-eaaa-4335-a10b-e0c0338c43ae" );
            err = ErrERRCheck( JET_errDatabaseIncompleteRevert );
            goto HandleError;
        }
    }
    else if ( JET_errReadVerifyFailure == err
        || ( err >= 0
            && pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown
            && pdbfilehdr->Dbstate() != JET_dbstateDirtyShutdown
            && pdbfilehdr->Dbstate() != JET_dbstateDirtyAndPatchedShutdown ) )
    {
        // JET_dbstateJustCreated dealt with above
        Assert( JET_errReadVerifyFailure == err || pdbfilehdr->Dbstate() != JET_dbstateJustCreated );
        // JET_dbstateIncrementalReseedInProgress dealt with above
        Assert( JET_errReadVerifyFailure == err || pdbfilehdr->Dbstate() != JET_dbstateIncrementalReseedInProgress );
        // JET_dbstateRevertInProgress dealt with above
        Assert( JET_errReadVerifyFailure == err || pdbfilehdr->Dbstate() != JET_dbstateRevertInProgress );

        //  header checksums incorrectly or invalid state, so go ahead and recreate the db
        *predoattach = redoattachCreate;
        err = JET_errSuccess;
        goto HandleError;
    }

    else if ( err < 0 )
    {
        reason = eDARIOError;
        errIO = err;
        switch ( err )
        {
            case JET_errFileAccessDenied:
                // File is there but we do not have access, maybe we will not need it by end of recovery, defer the error
                *predoattach = redoattachDeferAccessDenied;
                __fallthrough;

            case JET_errFileNotFound:
            case JET_errInvalidPath:
                //  assume database got deleted in the future
                err = JET_errSuccess;
                break;

            default:
                LGRIReportUnableToReadDbHeader( m_pinst, err, wszDbName );
                err = pfmp->FIgnoreDeferredAttach() ? JET_errSuccess : err;
                break;
        }

        goto HandleError;
    }

    if ( memcmp( &pdbfilehdr->signLog, &m_signLog, sizeof( SIGNATURE ) ) != 0 )
    {
        if ( !pfmp->FOverwriteOnCreate() )
        {
            // just as we do for attach, we will simply ignore any db with a mismatched log signature
            reason = eDARLogSignatureMismatch;
            goto HandleError;
        }
        else
        {
            // delete the old database file
            Call( ErrIODeleteDatabase( m_pinst->m_pfsapi, ifmp ) );

            // setup to create the new database
            *predoattach = redoattachCreate;
            goto HandleError;
        }
    }

    const INT   i   = CmpLgpos( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach );
    if ( 0 == i )
    {
        if ( 0 == memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) )
        {
            if ( CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->le_lgposConsistent ) <= 0 )
            {
                //  db was brought to a consistent state in the future, so no
                //  need to redo operations until then, so attach null
                *predoattach = redoattachDeferConsistentFuture;
                reason = eDARConsistentFuture;
            }
            else
            {
                Call( ErrLGRICheckAttachNow( pdbfilehdr, wszDbName ) );

                //  db never brought to consistent state after it was created
                Assert( 0 == CmpLgpos( &lgposMin, &pdbfilehdr->le_lgposConsistent ) );
                Assert( JET_dbstateDirtyShutdown == pdbfilehdr->Dbstate() ||
                        JET_dbstateDirtyAndPatchedShutdown == pdbfilehdr->Dbstate() );
                *predoattach = redoattachNow;
            }
        }
        else
        {
            //  database has same log signature and lgposAttach as
            //  what was logged, but db signature is different - must
            //  have manipulated the db with logging disabled, which
            //  causes us to generate a new signDb.
            //  Defer this attachment.  We will attach later on when
            //  we hit the AttachDb log record matching this signDb.

            // Consider adding the FireWall back once we figure out that all our
            // customers are respecting it.
            // (It is a problem with the current IBS install status)
            // FireWall( "SignatureMismatchOnRedoCreateDb" );
            reason = eDARDbSignatureMismatch;
        }
    }
    else if ( i > 0 )
    {
        //  database was attached in the future (if db signatures match)
        //  or deleted then recreated in the future (if db signatures don't match),
        //  but in either case, we simply defer attachment to the future
        reason = eDARAttachFuture;
    }
    else
    {
        //  this must be a different database (but with the same name)
        //  that was deleted in the past
        reason = eDARDbSignatureMismatch;
        Assert( 0 != memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) );
        
        if ( !pfmp->FOverwriteOnCreate() )
        {
            // we won't overwrite it, rather we report it and keep going ... 
            const WCHAR * rgszT[3];
            WCHAR       szTTime[2][128];
            LOGTIME     tm;

            rgszT[0] = pfmp->WszDatabaseName();

            memset( szTTime, '\0', sizeof( szTTime ) );
            tm = patchchk->signDb.logtimeCreate;
            OSStrCbFormatW( szTTime[0], sizeof( szTTime[0] ), L"%d/%d/%d %d:%d:%d.%3.3d",
                (SHORT)tm.bMonth, (SHORT)tm.bDay, (SHORT)tm.bYear + 1900,
                (SHORT)tm.bHours, (SHORT)tm.bMinutes, (SHORT)tm.bSeconds, (SHORT)tm.Milliseconds() );
            rgszT[1] = szTTime[0];

            tm = pdbfilehdr->signDb.logtimeCreate;
            OSStrCbFormatW( szTTime[1], sizeof( szTTime[1] ), L"%d/%d/%d %d:%d:%d.%3.3d",
                (SHORT)tm.bMonth, (SHORT)tm.bDay, (SHORT)tm.bYear + 1900,
                (SHORT)tm.bHours, (SHORT)tm.bMinutes, (SHORT)tm.bSeconds, (SHORT)tm.Milliseconds() );
            rgszT[2] = szTTime[1];

            //%1 (%2) %3The database %4 created at %5 was not recovered. The recovered database was created at %5.
            UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_PARTIALLY_ERROR_ID, 3, rgszT, sizeof( szTTime ), szTTime, m_pinst );
        }
        else
        {
            // delete the old database file
            Call( ErrIODeleteDatabase( m_pinst->m_pfsapi, ifmp ) );

            // setup to create the new database
            *predoattach = redoattachCreate;
        }
    }

HandleError:
    if ( err == JET_errRequiredLogFilesMissing )
    {
        LGRIReportRequiredLogFilesMissing( pdbfilehdr, wszDbName );
    }
    if ( err >= JET_errSuccess && ( *predoattach == redoattachDefer || *predoattach == redoattachDeferConsistentFuture || *predoattach == redoattachDeferAccessDenied ) )
    {
        LGRIReportDeferredAttach( wszDbName, reason, fTrue, errIO );
    }

    return err;
}


ERR LOG::ErrLGRICheckRedoAttachDb(
    const IFMP                  ifmp,
    DBFILEHDR                   *pdbfilehdr,
    const SIGNATURE             *psignLogged,
    REDOATTACH                  *predoattach,
    const REDOATTACHMODE        redoattachmode )
{
    ERR             err         = JET_errSuccess;
    FMP *           pfmp        = &g_rgfmp[ifmp];
    ATCHCHK *       patchchk    = pfmp->Patchchk();
    const WCHAR *   wszDbName   = pfmp->WszDatabaseName();
    eDeferredAttachReason reason;
    ERR             errIO       = JET_errSuccess;

    OSTrace( JET_tracetagDatabases, __FUNCTION__ );

    Assert( NULL != pfmp );
    Assert( NULL == pfmp->Pdbfilehdr() );
    Assert( NULL != patchchk );
    Assert( NULL != wszDbName );
    Assert( NULL != pdbfilehdr );
    Assert( NULL != psignLogged );
    Assert( NULL != predoattach );

    Assert( !pfmp->FReadOnlyAttach() );

    //  presume a deferred attachment
    *predoattach = redoattachDefer;
    reason = eDARUnknown;

    err = ErrUtilReadShadowedHeader(
            m_pinst,
            m_pinst->m_pfsapi,
            wszDbName,
            (BYTE*)pdbfilehdr,
            g_cbPage,
            OffsetOf( DBFILEHDR, le_cbPageSize ) );

    //  never replay against a database that is in the middle of an incremental reseed
    //
    if ( err >= JET_errSuccess && pdbfilehdr->Dbstate() == JET_dbstateIncrementalReseedInProgress )
    {
        reason = eDARIncReseedInProgress;
        if ( pfmp->FIgnoreDeferredAttach() )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"b4ef3d5b-78fb-4573-b142-46c17f7bb56e" );
            err = ErrERRCheck( JET_errDatabaseIncompleteIncrementalReseed );
            goto HandleError;
        }
    }
    //  never replay against a database that is in the middle of a revert.
    //
    else if ( err >= JET_errSuccess && pdbfilehdr->Dbstate() == JET_dbstateRevertInProgress )
    {
        reason = eDARRevertInProgress;
        if ( pfmp->FIgnoreDeferredAttach() )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        else
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"954dd127-73d4-4fe0-bb22-6b8bad4b399d" );
            err = ErrERRCheck( JET_errDatabaseIncompleteRevert );
            goto HandleError;
        }
    }
    else if ( JET_errReadVerifyFailure == err )
    {
        reason = eDARHeaderCorrupt;
        if ( pfmp->FIgnoreDeferredAttach() )
        {
            err = JET_errSuccess;
            goto HandleError;
        }
        else
        {
            // the log file header is corrupt
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"9106f5c1-2f93-479b-a12a-c93c6ab3de68" );
            goto HandleError;
        }
    }
    else if ( err >= 0 )
    {
        if ( pdbfilehdr->Dbstate() == JET_dbstateJustCreated )
        {
            // Just-created is equivalent to missing db, may get re-created in
            // the future
            reason = eDARHeaderStateBad;
            err = JET_errSuccess;
            goto HandleError;
        }
        else if ( pdbfilehdr->Dbstate() != JET_dbstateCleanShutdown &&
                  pdbfilehdr->Dbstate() != JET_dbstateDirtyShutdown &&
                  pdbfilehdr->Dbstate() != JET_dbstateDirtyAndPatchedShutdown )
        {
            if ( pfmp->FIgnoreDeferredAttach() )
            {
                reason = eDARHeaderStateBad;
                err = JET_errSuccess;
                goto HandleError;
            }
            else
            {
                // the dbstate is unexpected            
                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"2bfd590a-d8bc-42a5-a45a-9ca332a91faa" );
                reason = eDARHeaderStateBad;
                err = ErrERRCheck( JET_errDatabaseCorrupted );
                goto HandleError;
            }
        }
    }
    else
    {
        reason = eDARIOError;
        errIO = err;
        switch ( err )
        {
            case JET_errFileAccessDenied:
                // File is there but we do not have access, maybe we will not need it by end of recovery, defer the error
                *predoattach = redoattachDeferAccessDenied;
                __fallthrough;

            case JET_errFileNotFound:
            case JET_errInvalidPath:
                //  assume database got deleted in the future
                err = JET_errSuccess;
                break;

            default:
                LGRIReportUnableToReadDbHeader( m_pinst, err, wszDbName );
                err = pfmp->FIgnoreDeferredAttach() ? JET_errSuccess : err;
                break;
        }

        goto HandleError;
    }

    const BOOL  fMatchingSignDb         = ( 0 == memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) );
    const BOOL  fMatchingSignLog        = ( 0 == memcmp( &pdbfilehdr->signLog, &m_signLog, sizeof(SIGNATURE) ) );
    const BOOL  fMatchingLoggedSignLog  = ( 0 == memcmp( &pdbfilehdr->signLog, psignLogged, sizeof(SIGNATURE) ) );

    //  When we are recovering a dirty-and-patched database, it's possible that lGenMinRequired gets
    //  stalled due to pending redo map entries. When that happens and there are mulitple attach/detach
    //  cycles before the redo map entries are resolved, we could have lgposAttach ahead of lGenMinRequired.
    //  In that case, we need to reset lGenMinRequired and lgposAttach so that we are forced to re-attach
    //  and rebuild the redo maps. Note that ErrIsamEndDatabaseIncrementalReseed() does something similar to
    //  force early attaches, so think of this as an inc-reseed fixup/reset.
    if ( ( pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown ) && !pfmp->FPendingRedoMapEntries() )
    {
        // Do not override anything if there's no chance of a match anyways.
        if ( !fMatchingSignDb || ( !fMatchingSignLog && !fMatchingLoggedSignLog ) )
        {
            goto PostDirtyAndPatchedFixup;
        }

        // Consistency.
        if ( ( pdbfilehdr->le_lGenMinRequired <= 0 ) ||
             ( 0 == CmpLgpos( &pdbfilehdr->le_lgposAttach, &lgposMin ) ) ||
             ( 0 == CmpLgpos( &patchchk->lgposAttach, &lgposMin ) ) )
        {
            reason = eDARHeaderStateBad;
            err = ErrERRCheck( JET_errDatabaseCorrupted );
            goto HandleError;
        }

        // No need to fix it if it's already ahead or below even what we about to stamp on it.
        // Also, do not patch if we're about to move the header's lgposAttach ahead.
        if ( ( pdbfilehdr->le_lGenMinRequired > pdbfilehdr->le_lgposAttach.le_lGeneration ) ||
             ( pdbfilehdr->le_lGenMinRequired < patchchk->lgposAttach.lGeneration ) ||
             ( CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->le_lgposAttach ) > 0 ) )
        {
            if ( ( pdbfilehdr->le_lGenMinRequired < pdbfilehdr->le_lgposAttach.le_lGeneration ) &&
                 ( CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->le_lgposAttach ) > 0 ) &&
                 ( CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->le_lgposConsistent ) != 0 ) )
            {
                // This is unexpected: it means we would need to patch (min. req. < lgposAttach in the header),
                // but we would move lgposAttached forward. The case in which lgposConsistent matches is legit: it's
                // the normal case for replaying an attach LR for the first time with pending redo map entries.
                FireWall( "CheckRedoAttachMinReqTooLow" );
                reason = eDARHeaderStateBad;
                err = ErrERRCheck( JET_errDatabaseCorrupted );
                goto HandleError;
            }

            goto PostDirtyAndPatchedFixup;
        }

        LGIGetDateTime( &pdbfilehdr->logtimeAttach );
        UtilMemCpy( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach, sizeof( pdbfilehdr->le_lgposAttach ) );
        LGIGetDateTime( &pdbfilehdr->logtimeConsistent );
        UtilMemCpy( &pdbfilehdr->le_lgposConsistent, &patchchk->lgposConsistent, sizeof( pdbfilehdr->le_lgposConsistent ) );        

        CFlushMapForUnattachedDb* pfm = NULL;
        Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbName, pdbfilehdr, m_pinst, &pfm ) );
        ERR errT = ErrUtilWriteUnattachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, wszDbName, pdbfilehdr, NULL, pfm );
        if ( pfm != NULL )
        {
            pfm->TermFlushMap();
            delete pfm;
            pfm = NULL;
        }

        Call( errT );
    }


PostDirtyAndPatchedFixup:

    if ( fMatchingSignLog )
    {
        //  db is in sync with current log set, so use normal attach logic below

        //  only way logged log signature doesn't match db log signature
        //  is if we're redoing an attachment
        Assert( fMatchingLoggedSignLog || redoattachmodeAttachDbLR == redoattachmode );
    }
    else if ( fMatchingLoggedSignLog )
    {
        //  if db matches prev log signature, then it should also match lgposConsistent
        //  (since dbfilehdr never got updated, it must have both prev log signature
        //  and prev lgposConsistent)
        if ( 0 == CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->le_lgposConsistent ) )
        {
            if ( fMatchingSignDb )
            {
                Assert( !pfmp->FReadOnlyAttach() );
                Call( ErrLGRICheckAttachNow( pdbfilehdr, wszDbName ) );

                //  the attach operation was logged, but header was not changed.
                //  set up the header such that it looks like it is set up after
                //  attach (if this is currently a ReadOnly attach, the header
                //  update will be deferred to the next non-ReadOnly attach)
                Assert( redoattachmodeAttachDbLR == redoattachmode );
                Assert( 0 == CmpLgpos( &patchchk->lgposAttach, &m_lgposRedo ) );

                //  UNDONE: in theory, lgposAttach should already have been set
                //  when the ATCHCHK was setup, but SOMEONE says he's not 100%
                //  sure, so to be safe, we definitely set the lgposAttach here
                Assert( 0 == CmpLgpos( patchchk->lgposAttach, pfmp->LgposAttach() ) );
                pfmp->SetLgposAttach( patchchk->lgposAttach );
                DBISetHeaderAfterAttach( pdbfilehdr, patchchk->lgposAttach, NULL, ifmp, fFalse /* no keep bkinfo */);
                Assert( pdbfilehdr->le_objidLast > 0 );

                CFlushMapForUnattachedDb* pfm = NULL;
                Call( CFlushMapForUnattachedDb::ErrGetPersistedFlushMapOrNullObjectIfRuntime( wszDbName, pdbfilehdr, m_pinst, &pfm ) );

                ERR errT = ErrUtilWriteUnattachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, wszDbName, pdbfilehdr, NULL, pfm );

                if ( pfm != NULL )
                {
                    if ( pdbfilehdr->Dbstate() == JET_dbstateCleanShutdown )
                    {
                        errT = pfm->ErrCleanFlushMap();
                    }

                    pfm->TermFlushMap();
                    delete pfm;
                    pfm = NULL;
                }

                Call( errT );

                *predoattach = redoattachNow;
            }
            else
            {
                //  database has same log signature and lgposConsistent as
                //  what was logged, but db signature is different - must
                //  have manipulated the db with logging disabled, which
                //  causes us to generate a new signDb.
                //  Defer this attachment.  We will attach later on when
                //  we hit the AttachDb log record matching this signDb.

                if ( !pfmp->FIgnoreDeferredAttach() )
                {
                    FireWall( "SignatureMismatchOnRedoAttachDb" );
                }

                reason = eDARDbSignatureMismatch;
            }
        }
        else
        {
            if ( fMatchingSignDb )
            {
                reason = eDARConsistentTimeMismatch;
                if ( !pfmp->FIgnoreDeferredAttach() )
                {
                    //  the database for this log sequence has been rolled back
                    //  in time (or at least its header has)
                    LGReportConsistentTimeMismatch( m_pinst, wszDbName );
                    err = ErrERRCheck( JET_errConsistentTimeMismatch );
                }
            }
            else
            {
                reason = eDARDbSignatureMismatch;
            }
        }

        goto HandleError;
    }
    else
    {
        //  the database's log signature is not the same as current log set or
        //  as the log set before it was attached, so just ignore it
        reason = eDARLogSignatureMismatch;
        err = JET_errSuccess;
        goto HandleError;
    }

    const INT   i   = CmpLgpos( &pdbfilehdr->le_lgposConsistent, &patchchk->lgposConsistent );

    //  if log signature in db header doesn't match logged log signature,
    //  then comparing lgposConsistent is irrelevant and must instead
    //  rely on lgposAttach comparison
    if ( !fMatchingLoggedSignLog
        || 0 == i
        || 0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
    {
        if ( fMatchingSignDb )
        {
            const INT   j   = CmpLgpos( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach );
            if ( 0 == j )
            {
                Call( ErrLGRICheckAttachNow( pdbfilehdr, wszDbName ) );

                //  either lgposAttach also matches, or we're redoing a new attach
                *predoattach = redoattachNow;
            }
            else if ( j < 0 )
            {
                if ( redoattachmodeAttachDbLR == redoattachmode && fMatchingLoggedSignLog )
                {
                    Call( ErrLGRICheckAttachNow( pdbfilehdr, wszDbName ) );
                    *predoattach = redoattachNow;
                }
                else
                {
                    if ( !pfmp->FIgnoreDeferredAttach() )
                    {
                        //  lgposConsistent match, but the lgposAttach in the database is before
                        //  the logged lgposAttach, which means the attachment was somehow skipped
                        //  in this database (typically happens when a file copy of the database
                        //  is copied back in)
                        LGReportAttachedDbMismatch( m_pinst, wszDbName, fFalse );

                        FireWall( "AttachedDbMismatchOnRedoAttachDb" );
                        err = ErrERRCheck( JET_errAttachedDatabaseMismatch );
                        goto HandleError;
                    }
                }
            }
            else
            {
                // Removed the following assert. The assert where probably here when
                // the 3rd OR condition (0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
                // was missing.
                //  only way we could have same lgposConsistent, but lgposAttach
                //  in db is in the future is if this is a ReadOnly attach
                //Assert( redoattachmodeAttachDbLR == redoattachmode );
                //Assert( fReadOnly || !fMatchingLoggedSignLog );
                reason = eDARAttachFuture;
            }
        }
        else
        {
            reason = eDARDbSignatureMismatch;
#ifdef DEBUG
            //  database has same log signature and lgposConsistent as
            //  what was logged, but db signature is different - must
            //  have recreated db or manipulated it with logging disabled,
            //  which causes us to generate a new signDb
            if ( 0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
            {
                //  there's a chance we can still hook up with the correct
                //  signDb in the future
            }
            else
            {
                //  can no longer replay operations against this database
                //  because we've hit a point where logging was disabled
                //  UNDONE: add eventlog entry

                // Consider adding the FireWall back once we figure out that all our
                // customers are respecting it.
                // (It is a problem with the current IBS install status)
                // FireWall( "SignatureMismatchOnRedoAttachDbLoggedSignMismatch" );
            }
#endif
        }
    }
    else if ( i > 0
        && ( redoattachmodeInitBeforeRedo == redoattachmode
            || CmpLgpos( &pdbfilehdr->le_lgposConsistent, &m_lgposRedo ) > 0 ) )
    {
        //  database was brought to a consistent state in the future
        //  (if db signatures match) or deleted then recreated and
        //  reconsisted in the future (if db signatures don't match),
        //  but in either case, we simply defer attachment to the future
        reason = eDARConsistentFuture;
        *predoattach = redoattachDeferConsistentFuture;
        Assert( redoattachmodeInitBeforeRedo == redoattachmode
            || redoattachmodeAttachDbLR == redoattachmode );
        Assert( 0 != CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) );
        Assert( CmpLgpos( &pdbfilehdr->le_lgposAttach, &patchchk->lgposAttach ) >= 0 );
    }
    else
    {
        if ( fMatchingSignDb )
        {
            //  One way to get here is to do the following:
            //      - shutdown cleanly
            //      - make a file copy of the db
            //      - start up and re-attach
            //      - make a backup
            //      - shutdown cleanly
            //      - copy back original database
            //      - start up and re-attach
            //      - shutdown cleanly
            //      - restore from backup
            //  When hard recovery hits the attachment to the
            //  original database, the dbfilehdr's lgposConsistent
            //  will be greater than the logged one, but less
            //  than the current lgposRedo.
            //
            //  Another way to hit this is if the lgposConsistent
            //  in the dbfilehdr is less than the logged lgposConsistent.
            //  This is usually caused when an old copy of the database
            //  is being played against a more current copy of the log files.
            //
            reason = eDARConsistentTimeMismatch;

            if ( !pfmp->FIgnoreDeferredAttach() )
            {
                // Consider adding the FireWall back once we figure out that all our
                // customers are respecting it.
                // (It is a problem with the current IBS install status)
                // FireWall( "ConsistentTimeMismatchOnRedoAttachDb" );
                LGReportConsistentTimeMismatch( m_pinst, wszDbName );
                err = ErrERRCheck( JET_errConsistentTimeMismatch );
                goto HandleError;
            }
        }
        else
        {
            Assert( 0 != CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) );

            //  database has been manipulated with logging disabled in
            //  the past.  Therefore, we cannot replay operations
            //  since we are missing the non-logged operations.
            //  UNDONE: add eventlog entry

            // Consider adding the FireWall back once we figure out that all our
            // customers are respecting it.
            // (It is a problem with the current IBS install status)
            // FireWall( "UnloggedDbUpdateOnRedoAttachDb" );
            reason = eDARUnloggedDbUpdate;
        }
    }

HandleError:
    if ( err == JET_errRequiredLogFilesMissing )
    {
        if ( pfmp->FIgnoreDeferredAttach() )
        {
            *predoattach = redoattachDefer;
            reason = eDARRequiredLogsMissing;
            err = JET_errSuccess;
        }
        else
        {
            LGRIReportRequiredLogFilesMissing( pdbfilehdr, wszDbName );
        }
    }
    if ( err >= JET_errSuccess && ( *predoattach == redoattachDefer || *predoattach == redoattachDeferConsistentFuture || *predoattach == redoattachDeferAccessDenied ) )
    {
        LGRIReportDeferredAttach( wszDbName, reason, fFalse, errIO );
    }

    return err;
}


ERR LOG::ErrLGRICheckAttachedDb(
    const IFMP                  ifmp,
    const SIGNATURE             *psignLogged,           //  pass NULL for CreateDb
    REDOATTACH                  *predoattach,
    const REDOATTACHMODE        redoattachmode )
{
    ERR             err;
    FMP             *pfmp       = &g_rgfmp[ifmp];
    DBFILEHDR       *pdbfilehdr;

    Assert( NULL != pfmp );
    Assert( NULL == pfmp->Pdbfilehdr() );
    Assert( NULL != pfmp->Patchchk() );
    Assert( NULL != predoattach );

    AllocR( pdbfilehdr = (DBFILEHDR *)PvOSMemoryPageAlloc( g_cbPage, NULL ) );

    //  WARNING: must zero out memory, because we may end
    //  up defer-attaching the database, but still compare
    //  against stuff in this memory
    //
    memset( pdbfilehdr, 0, g_cbPage );

    Assert( !pfmp->FReadOnlyAttach() );
    if ( redoattachmodeCreateDbLR == redoattachmode )
    {
        Assert( NULL == psignLogged );
        err = ErrLGRICheckRedoCreateDb( ifmp, pdbfilehdr, predoattach );
    }
    else
    {
        Assert( NULL != psignLogged );
        err = ErrLGRICheckRedoAttachDb(
            ifmp,
            pdbfilehdr,
            psignLogged,
            predoattach,
            redoattachmode );
        Assert( err < 0 || redoattachCreate != *predoattach );
    }

    //  if redoattachCreate, dbfilehdr will be allocated
    //  when we re-create the db
    if ( err >= 0 && redoattachCreate != *predoattach )
    {
        const ATCHCHK   *patchchk   = pfmp->Patchchk();
        Assert( NULL != patchchk );

        const DBTIME    dbtime  = max( (DBTIME) patchchk->Dbtime(), (DBTIME) pdbfilehdr->le_dbtimeDirtied );
        const OBJID     objid   = max( (OBJID) patchchk->ObjidLast(), (OBJID) pdbfilehdr->le_objidLast );

        pfmp->SetDbtimeLast( dbtime );

        pfmp->SetObjidLast( objid );
        pfmp->SetLGenMaxCommittedAttachedDuringRecovery( pdbfilehdr->le_lGenMaxCommitted );
        DBFILEHDR * pdbfilehdrPrevIgnored;
        err = pfmp->ErrSetPdbfilehdr( pdbfilehdr, &pdbfilehdrPrevIgnored );
        if ( err < JET_errSuccess )
        {
            OSMemoryPageFree( pdbfilehdr );
        }

        if ( pfmp->PRBS() )
        {
            err = pfmp->PRBS()->ErrRBSRecordDbAttach( pfmp );
        }
        if ( pfmp->FRBSOn() )
        {
            // This is just an estimate, will get a better value as we replay through the required range
            pfmp->SetDbtimeBeginRBS( pfmp->PRBS()->GetDbtimeForFmp( pfmp ) );
            if ( pfmp->DbtimeBeginRBS() == 0 )
            {
                pfmp->SetNeedUpdateDbtimeBeginRBS();
                pfmp->SetDbtimeBeginRBS( dbtime );
            }
        }

        if ( err < JET_errSuccess )
        {
            return err;
        }
    }
    else
    {
        OSMemoryPageFree( pdbfilehdr );
    }

    return err;
}

VOID LOG::LGRIReportRequiredLogFilesMissing(
    DBFILEHDR   * pdbfilehdr,
    const WCHAR * wszDbName )
{
    const LONG  lGenMinRequired = pdbfilehdr->le_lGenMinRequired;
    const LONG  lGenCurrent = m_pLogStream->GetCurrentFileGen();

    // we claim log committed if available, merely because that is what normal 
    // recovery (w/ no options would require)
    const LONG  lGenMaxRequired = ( pdbfilehdr->le_lGenMaxCommitted != 0 ) ?
        pdbfilehdr->le_lGenMaxCommitted :
        pdbfilehdr->le_lGenMaxRequired;
    WCHAR       szT1[16];
    WCHAR       szT2[16];
    WCHAR       szT3[16];
    const UINT  csz = 4;
    const WCHAR *rgszT[csz];

    rgszT[0] = wszDbName;
    OSStrCbFormatW( szT1, sizeof( szT1 ), L"%d", lGenMinRequired );
    rgszT[1] = szT1;
    OSStrCbFormatW( szT2, sizeof( szT2 ), L"%d", lGenMaxRequired );
    rgszT[2] = szT2;
    OSStrCbFormatW( szT3, sizeof( szT3 ), L"%d", lGenCurrent );
    rgszT[3] = szT3;

    UtilReportEvent(
        eventError,
        LOGGING_RECOVERY_CATEGORY,
        REDO_MISSING_LOW_LOG_ERROR_ID,
        sizeof( rgszT ) / sizeof( rgszT[0] ),
        rgszT,
        0,
        NULL,
        m_pinst );

    OSUHAPublishEvent( HaDbFailureTagCorruption,
        m_pinst,
        HA_LOGGING_RECOVERY_CATEGORY,
        HaDbIoErrorNone, NULL, 0, 0,
        HA_REDO_MISSING_LOW_LOG_ERROR_ID,
        sizeof( rgszT ) / sizeof( rgszT[0] ),
        rgszT );
}

VOID LOG::LGRIReportDeferredAttach(
    const WCHAR                 *wszDbName,
    const eDeferredAttachReason reason,
    const BOOL                  fCreateDb,
    const ERR                   errIO )
{
    WCHAR       wszT2[64];
    WCHAR       wszT3[16];
    WCHAR       wszT4[64];
    const WCHAR *rgwszT[5];

    //  these deferred attaches are not interesting
    if ( reason == eDARConsistentFuture )
    {
        return;
    }

    rgwszT[0] = fCreateDb ? L"Create" : L"Attach";
    rgwszT[1] = wszDbName;
    OSStrCbFormatW(
        wszT2,
        sizeof( wszT2 ),
        L"(%08X,%04X,%04X)",
        m_lgposRedo.lGeneration,
        m_lgposRedo.isec,
        m_lgposRedo.ib );
    rgwszT[2] = wszT2;
    switch ( reason )
    {
        case eDARUnknown:
            rgwszT[3] = L"Unknown";
            break;

        case eDARIOError:
            rgwszT[3] = L"IOError";
            break;

        case eDARLogSignatureMismatch:
            rgwszT[3] = L"LogSignatureMismatch";
            break;

        case eDARDbSignatureMismatch:
            rgwszT[3] = L"DbSignatureMismatch";
            break;

        case eDARConsistentFuture:
            rgwszT[3] = L"ConsistentFuture";
            break;

        case eDARAttachFuture:
            rgwszT[3] = L"AttachFuture";
            break;

        case eDARHeaderCorrupt:
            rgwszT[3] = L"HeaderCorrupt";
            break;

        case eDARHeaderStateBad:
            rgwszT[3] = L"HeaderStateBad";
            break;

        case eDARIncReseedInProgress:
            rgwszT[3] = L"IncReseedInProgress";
            break;

        case eDARUnloggedDbUpdate:
            rgwszT[3] = L"UnloggedDbUpdate";
            break;

        case eDARRequiredLogsMissing:
            rgwszT[3] = L"RequiredLogsMissing";
            break;

        case eDARConsistentTimeMismatch:
            rgwszT[3] = L"ConsistentTimeMismatch";
            break;

        default:
            Assert( fFalse );
            OSStrCbFormatW( wszT3, sizeof( wszT3 ), L"%d", reason );
            rgwszT[3] = wszT3;
            break;
    }
    switch ( reason )
    {
        case eDARIOError:
            OSStrCbFormatW( wszT4, sizeof( wszT4 ), L"%i (0x%08x)", errIO, errIO );
            rgwszT[4] = wszT4;
            break;

        default:
            rgwszT[4] = L"";
            break;
    }

    UtilReportEvent(
        eventInformation,
        LOGGING_RECOVERY_CATEGORY,
        REDO_DEFERRED_ATTACH_REASON_ID,
        sizeof( rgwszT ) / sizeof( rgwszT[0] ),
        rgwszT,
        0,
        NULL,
        m_pinst );

    OSDiagTrackDeferredAttach( reason );
}

//  we've determined this is the correct attachment point,
//  but first check database header for possible logfile mismatches
ERR LOG::ErrLGRICheckAttachNow(
    DBFILEHDR   * pdbfilehdr,
    const WCHAR * wszDbName )
{
    ERR         err                 = JET_errSuccess;
    const LONG  lGenMinRequired     = pdbfilehdr->le_lGenMinRequired;
    const LONG  lGenCurrent         = m_pLogStream->GetCurrentFileGen();

    if ( lGenMinRequired            //  0 means db is consistent or this is an ESE97 db
        && JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->Dbstate()  // IncReSeed DB recovery can leave lGenMinRequired behind
        && lGenMinRequired < lGenCurrent )
    {
        err = ErrERRCheck( JET_errRequiredLogFilesMissing );
    }

    else if ( pdbfilehdr->bkinfoFullCur.le_genLow && !m_fHardRestore )
    {

        // We are adding the option to allow soft recovery on a backup set
        // predicated on if the client asks us to. (Such as store's/HA's 
        // log shipping replay which starts from a "seed" / backed up database.
        // There is no technical reason for this (we could do it all the time)
        // but we are doing this to: 
        // - reduce the test impact
        // - maintain compatibility with the current behaviour, if this is important
        // ( ie if someone is forgething to check "last backup set" on restore,
        // they will get a different error other then JET_errSoftRecoveryOnBackupDatabase)
        //

        // the check for pdbfilehdr->bkinfoFullCur.le_genHigh checks
        // if we already moved the trailer in the header. 
        // If so (pdbfilehdr->bkinfoFullCur.le_genHigh !=0 ) then allow restore
        // because the only way to move it is using "recovery w/o undo".
        // The bkinfoFullCur will be reset only when replaying a Detach or Term
        // or when the database is made consistent.
        //
        CallS( err );
        if ( 0 == pdbfilehdr->bkinfoFullCur.le_genHigh )
        {
            err = ErrLGSignalErrorConditionCallback( m_pinst, JET_errSoftRecoveryOnBackupDatabase );
            if ( err < JET_errSuccess )
            {
                const WCHAR *rgszT[1];
                rgszT[0] = wszDbName;
                
                //  attempting to use a database which did not successfully
                //  complete conversion
                UtilReportEvent(
                        eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        ATTACH_TO_BACKUP_SET_DATABASE_ERROR_ID,
                        1,
                        rgszT,
                        0,
                        NULL,
                        m_pinst );

                Error( err );
            }
        }
        // we should patch the header with gen max required
        //
        err = ErrDBUpdateHeaderFromTrailer( m_pinst, m_pinst->m_pfsapi, pdbfilehdr, wszDbName, fFalse );
    }

HandleError:

    return err;
}

ERR LOG::ErrLGRIRedoCreateDb(
    _In_ PIB                                                    *ppib,
    _In_ const IFMP                                             ifmp,
    _In_ const DBID                                             dbid,
    _In_ const JET_GRBIT                                        grbit,
    _In_ const BOOL                                             fSparseEnabledFileRedo,
    _In_opt_ SIGNATURE                                          *psignDb,
    _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM* const   rgsetdbparam,
    _In_ const ULONG                                            csetdbparam )
{
    ERR             err;
    FMP             *pfmp   = &g_rgfmp[ifmp];
    IFMP            ifmpT   = ifmp;
    const WCHAR     *wszDbName = pfmp->WszDatabaseName() ;

    Assert( dbid < dbidMax );
    Assert( dbidTemp != dbid );

    OSTrace( JET_tracetagDatabases, OSFormat( "[Redo] Session=[0x%p:0x%x] creating database %ws [cpgMax=0x%x,grbit=0x%x,sparse=%d]",
            ppib, ppib->trxBegin0, wszDbName, 0x0, grbit, fSparseEnabledFileRedo ) );

    extern CPG cpgLegacyDatabaseDefaultSize;
    CPG CpgDBDatabaseMinMin();

    // With the advent of ESE supporting custom initial DB sizes for Phone 8.1 (so that size of 
    // of the DB is variable at JetCreateDatabase() time) arose a complication for redo in that
    // the lrtypCreateDB LR comes to this path with no cpgPrimary that we can pass to 
    // ErrDBCreateDatabase().  This means that unfortunately we'll call this:
    //      ErrIONewSize( ifmp, cpgPrimary );
    // with the cpgPrimary = MinMin size from the below call, which is wrong / not what the DB 
    // was actually created with.  But due to SOMEONE logging DB create, SUPER conveniently we bail 
    // before this:
    //      ErrDBInitDatabase( ppib, ifmp, cpgPrimary ) );
    // which would have setup a space tree with inaccurate first extent in the DB Root OE!  But 
    // this still means the DB size will be inaccurate right after we replay the create DB (MinMin
    // instead of whatever the user specified for the JET_paramDbExtensionSize at JetCreateDatabase()
    // call).  In Win Threshold we realized a way to make up for this conundrum and now we log a 
    // lrtypExtendDB for the cpgPrimary size from do-time right after lrtypCreateDB (well, if the 
    // user specified a larger size than MinMin).  
    // Note: the alternative of this would have been to rev the CreateDB LR and handle all the 
    // versioning issues with that.  The two LRs means the DB will be slightly smaller size for a 
    // very slight time between these two LRs, and that we'll grow in two extents back to back, 
    // instead of one (acceptable casualties, for not having to rev the minor log version).
    CallR( ErrDBCreateDatabase(
                ppib,
                NULL,
                wszDbName,
                &ifmpT,
                dbid,
                ( grbit & bitCreateDbImplicitly ) ? cpgLegacyDatabaseDefaultSize : CpgDBDatabaseMinMin(),
                fSparseEnabledFileRedo,
                psignDb,
                rgsetdbparam,
                csetdbparam,
                grbit ) );
    Assert( ifmp == ifmpT );

    /*  close it as it will get reopened on first use
    /**/
    CallR( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
    CallSx( err, JET_wrnDatabaseAttached );

    /*  restore information stored in database file
    /**/
    pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genLow = m_lGenLowRestore;
    pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genHigh = m_lGenHighRestore;

    pfmp->SetDbtimeCurrentDuringRecovery( pfmp->DbtimeLast() );
    pfmp->SetAttachedForRecovery();

    if( CmpLogFormatVersion( FmtlgverLGCurrent( this ), fmtlgverInitialDbSizeLoggedMain ) < 0 )
    {
        pfmp->SetOlderDemandExtendDb();
    }

    Assert( pfmp->Pdbfilehdr()->le_objidLast == pfmp->ObjidLast() );

    // now with active logging dbscan, it is unnecessary to initiate a separate dbscan task
    // unless specified they specially want to override the default follow-active behavior.
    // start checksum on this database
    if ( UlParam( pfmp->Pinst(), JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryPassiveScan )
    {
        CallR( pfmp->ErrStartDBMScan() );
    }

    return err;
}

ERR LOG::ErrLGRIRedoAttachDb(
    const IFMP                  ifmp,
    const CPG                   cpgDatabaseSizeMax,
    const BOOL                  fSparseEnabledFile,
    const REDOATTACHMODE        redoattachmode )
{
    ERR             err             = JET_errSuccess;
    FMP             *pfmp           = &g_rgfmp[ifmp];
    CFlushMapForUnattachedDb* pfm   = NULL;
    const WCHAR                 *wszDbName = pfmp->WszDatabaseName();

    Assert( NULL != pfmp );
    Assert( NULL != wszDbName );

    OSTrace( JET_tracetagDatabases, OSFormat( "[Redo] attaching database %ws [cpgMax=0x%x,grbit=0x%x,sparse=%d]",
                wszDbName, cpgDatabaseSizeMax, 0x0, fSparseEnabledFile ) );

    //  Do not re-create the database. Simply attach it. Assuming the
    //  given database is a good one since signature matches.
    //
    FMP::EnterFMPPoolAsWriter();
    pfmp->ResetFlags();
    pfmp->SetAttached();
    pfmp->SetAttachedForRecovery();
    FMP::LeaveFMPPoolAsWriter();

    pfmp->SetDatabaseSizeMax( cpgDatabaseSizeMax );
    Assert( pfmp->CpgDatabaseSizeMax() != 0xFFFFFFFF );

    // Versioning flag is not persisted (since versioning off
    // implies logging off).
    Assert( !pfmp->FVersioningOff() );
    pfmp->ResetVersioningOff();

    // If there's a log record for CreateDatabase(), then logging
    // must be on.
    FMP::EnterFMPPoolAsWriter();
    Assert( !pfmp->FLogOn() );
    pfmp->SetLogOn();
    FMP::LeaveFMPPoolAsWriter();

    //  Update database file header as necessary
    //
    Assert( !pfmp->FReadOnlyAttach() );
    BOOL fUpdateHeader = fFalse;
    OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusHdrLoaded ) );
    
    // If RBS header flush is set in the database header but RBS is not enabled anymore, we will reset the sign in the database header 
    // and update it so that the next time RBS is enabled the older snapshot would be considered invalid.
    {
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

    if ( FSIGSignSet( &pdbfilehdr->signRBSHdrFlush ) && !pfmp->FRBSOn() )
    {
        SIGResetSignature( &pdbfilehdr->signRBSHdrFlush );
        fUpdateHeader = fTrue;
    }
    }

    if ( redoattachmodeAttachDbLR == redoattachmode )
    {
        BOOL    fKeepBackupInfo     = fFalse;

        //  if we're replaying an attach during hard recovery, it
        //  might correspond to the attachment under which the
        //  backup was made (if the AttachDb log record was in one
        //  of the logs in the backup set), so in that case, retain
        //  the backup info
        //
        if ( g_rgfmp[ifmp].FHardRecovery() )
        {
            const INT   irstmap = IrstmapSearchNewName( wszDbName );

            if ( irstmap >= 0 )
            {
                fKeepBackupInfo = ( 0 == UtilCmpFileName( m_rgrstmap[irstmap].wszDatabaseName, m_rgrstmap[irstmap].wszNewDatabaseName ) );
            }
            else
            {
                //  unless we're performing recovery without undo
                //  (against a backup database), the database should
                //  always be in the restore map
                //
                Assert( ErrLGSignalErrorConditionCallback( m_pinst, JET_errSoftRecoveryOnBackupDatabase ) == JET_errSuccess );
                fKeepBackupInfo = fTrue;
            }
        }

        //  UNDONE: in theory, lgposAttach should already have been set
        //  when the ATCHCHK was setup, but SOMEONE says he's not 100%
        //  sure, so to be safe, we definitely set the lgposAttach here
        Assert( 0 == CmpLgpos( m_lgposRedo, pfmp->LgposAttach() ) );
        pfmp->SetLgposAttach( m_lgposRedo );

        {
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
        DBISetHeaderAfterAttach( pdbfilehdr.get(), m_lgposRedo, NULL, ifmp, fKeepBackupInfo );
        }
        Assert( pfmp->LgposWaypoint().lGeneration <= pfmp->LgposAttach().lGeneration );
        fUpdateHeader = fTrue;
    }
    else
    {
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
        if ( JET_dbstateDirtyShutdown != pdbfilehdr->Dbstate() &&
                JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->Dbstate() )
        {
            //  must force inconsistent during recovery (may currently be marked as
            //  consistent because we replayed a RcvQuit and are now replaying an Init)
            FireWall( OSFormat( "InvalidDbStateOnRedoAttachDb:%d", (INT)pdbfilehdr->Dbstate() ) );  //  should no longer be possible with forced detach on shutdown
            LOGTIME tmCreate;
            LONG lGenCurrent = m_pLogStream->GetCurrentFileGen( &tmCreate );
            pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, lGenCurrent, &tmCreate, fTrue );
            fUpdateHeader = fTrue;
        }
    }

    //  SoftRecoveryOnBackupDatabase check should already have been performed in
    //  ErrLGRICheckAttachedDb()
    { // .ctor acquires PdbfilehdrReadOnly
    OnDebug( PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr() );
    Assert( 0 == pdbfilehdr->bkinfoFullCur.le_genLow ||
            m_fHardRestore ||
            ( ErrLGSignalErrorConditionCallback( m_pinst, JET_errSoftRecoveryOnBackupDatabase ) == JET_errSuccess ) ||
            ( 0 != pdbfilehdr->bkinfoFullCur.le_genLow && 0 != pdbfilehdr->bkinfoFullCur.le_genHigh ) );
    } // .dtor releases PdbfilehdrReadOnly

    //  check DB versions' compatibility
    //
    Call( ErrDBIValidateUserVersions( m_pinst, wszDbName, ifmp, pfmp->Pdbfilehdr().get(), NULL ) );

    //  set up other miscellaneous header fields
    //
    const BOOL fLowerMinReqLogGenOnRedo = ( pfmp->ErrDBFormatFeatureEnabled( JET_efvLowerMinReqLogGenOnRedo ) == JET_errSuccess );
    { // .ctor acquires PdbfilehdrReadWrite

    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    Assert( ( pdbfilehdr->le_lGenMinRequired > 0 ) == ( pdbfilehdr->le_lGenMaxRequired > 0 ) );

    const LONG lGenCurrent = m_pLogStream->GetCurrentFileGen();
    Assert( lGenCurrent > 0 );
    Assert( lGenCurrent >= pdbfilehdr->le_lgposAttach.le_lGeneration );

    // Make sure lgposLastResize does not point to a log that is not required and therefore may have been deleted.
    if ( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) != 0 ) &&
         ( pdbfilehdr->le_lGenMaxRequired > 0 ) &&
         ( pdbfilehdr->le_lgposLastResize.le_lGeneration > pdbfilehdr->le_lGenMaxRequired ) )
    {
        // We can assert this because shrink always quiesces LLR, so if le_lgposLastResize points to a non-required log,
        // it means the last resize must have been an extension.
        Assert( pdbfilehdr->logtimeLastExtend.bYear != 0 );
        Assert( ( pdbfilehdr->logtimeLastShrink.bYear == 0 ) ||
                ( LOGTIME::CmpLogTime( pdbfilehdr->logtimeLastExtend, pdbfilehdr->logtimeLastShrink ) >= 0 ) );

        LGPOS lgposLastResize = lgposMax;
        lgposLastResize.lGeneration = pdbfilehdr->le_lGenMaxRequired;
        pdbfilehdr->le_lgposLastResize = lgposLastResize;
    }
    Assert( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) == 0 ) ||
            ( CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 ) );

    if ( ( pdbfilehdr->le_lGenMinRequired > 0 ) &&
         !m_fReplayIgnoreLogRecordsBeforeMinRequiredLog &&
         fLowerMinReqLogGenOnRedo )
    {
        // Bring the required range down so that any potential operations that get redone between the current
        // lgpos and the current minimum required are guaranteed to be redone in case of a crash. Normally,
        // nothing would get redone in this range, but there are a few exceptions. Namely: databases with
        // lost flushes, databases that underwent Shrink, databases that underwent Trim.
        //

        Expected( ( pdbfilehdr->le_lGenPreRedoMinRequired == 0 ) ||
                  ( pdbfilehdr->le_lGenPreRedoMinRequired > pdbfilehdr->le_lGenMinRequired ) );
        if ( lGenCurrent < pdbfilehdr->le_lGenMinRequired )
        {
            if ( ( pdbfilehdr->le_lGenPreRedoMinRequired == 0 ) ||
                 ( pdbfilehdr->le_lGenMinRequired > pdbfilehdr->le_lGenPreRedoMinRequired ) )
            {
                pdbfilehdr->le_lGenPreRedoMinRequired = pdbfilehdr->le_lGenMinRequired;
            }

            pdbfilehdr->le_lGenMinRequired = lGenCurrent;
            fUpdateHeader = fTrue;
        }

        Expected( ( pdbfilehdr->le_lGenPreRedoMinConsistent == 0 ) ||
                  ( pdbfilehdr->le_lGenPreRedoMinConsistent > pdbfilehdr->le_lGenMinConsistent ) );
        if ( lGenCurrent < pdbfilehdr->le_lGenMinConsistent )
        {
            if ( ( pdbfilehdr->le_lGenPreRedoMinConsistent == 0 ) ||
                 ( pdbfilehdr->le_lGenMinConsistent > pdbfilehdr->le_lGenPreRedoMinConsistent ) )
            {
                pdbfilehdr->le_lGenPreRedoMinConsistent = pdbfilehdr->le_lGenMinConsistent;
            }

            pdbfilehdr->le_lGenMinConsistent = lGenCurrent;
            fUpdateHeader = fTrue;
        }
    }
    // Set lGenMinConsistent back to lGenMinRequired because that's our effective initial checkpoint.
    // In the future, this can be delayed until a DB or flush map page gets dirtied.
    if ( pdbfilehdr->le_lGenMinConsistent != pdbfilehdr->le_lGenMinRequired )
    {
        pdbfilehdr->le_lGenMinConsistent = pdbfilehdr->le_lGenMinRequired;
        fUpdateHeader = fTrue;
    }
    } // .dtor releases PdbfilehdrReadWrite

    //  create a flush map for this DB
    //
    Call( pfmp->ErrCreateFlushMap( JET_bitNil ) );

    if ( fUpdateHeader )
    {
        Assert( pfmp->Pdbfilehdr()->le_objidLast > 0 );
        Call( ErrUtilWriteAttachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, wszDbName, pfmp ) );
    }

    OnDebug( const ULONG dbstate = pfmp->Pdbfilehdr()->Dbstate() );
    Assert( JET_dbstateDirtyShutdown == dbstate ||
            JET_dbstateDirtyAndPatchedShutdown == dbstate );

    /*  restore information stored in database file
    /**/
    if ( m_fHardRestore )
    {
        pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genLow = m_lGenLowRestore;
        pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genHigh = m_lGenHighRestore;
    }

    //  We're moving from ErrFileOpen() to ErrIOOpenDatabase() very late in the E14 game,
    //  and these Expected()s will prove we're effectively doing close to the same thing as we used to do.
    //  Some of these are obviously trivially true, and can be removed in E15.
    Expected( !FIODatabaseOpen( ifmp ) );
    Expected( !pfmp->FReadOnlyAttach() );
    Expected( pfmp->FLogOn() );
    Expected( pfmp->Dbid() != dbidTemp );

    Call( ErrIOOpenDatabase( m_pinst->m_pfsapi, ifmp, wszDbName, iofileDbRecovery, fSparseEnabledFile ) );
    pfmp->SetDbtimeCurrentDuringRecovery( 0 );
    
    OnDebug( const OBJID objidLastT = pfmp->Pdbfilehdr()->le_objidLast );
    Assert( pfmp->ObjidLast() == objidLastT
        || ( pfmp->ObjidLast() > objidLastT && redoattachmodeInitBeforeRedo == redoattachmode ) );

    Assert( !pfmp->FReadOnlyAttach() );

    //  since the attach is now reflected in the header (either
    //  because it existed that way or because we just updated
    //  it in the fUpdateHeader code path above), we can now
    //  safely set the waypoint
    //
    FMP::EnterFMPPoolAsWriter();
    pfmp->SetWaypoint( pfmp->LgposAttach().lGeneration );
    FMP::LeaveFMPPoolAsWriter();

    Assert( !pfmp->FContainsDataFromFutureLogs() );

    // Check if the database may contain any pages from the future.
    if ( m_lgposRedo.lGeneration == 0 || m_lgposRedo.lGeneration <= pfmp->Pdbfilehdr()->le_lGenMaxRequired )
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->SetContainsDataFromFutureLogs();
        FMP::LeaveFMPPoolAsWriter();
    }

    // we allow header update before starting to log any operation
    pfmp->SetAllowHeaderUpdate();

    if ( !pfmp->FContainsDataFromFutureLogs() )
    {
        CallR( ErrLGDbAttachedCallback( m_pinst, pfmp ) );
    }

    if( CmpLogFormatVersion( FmtlgverLGCurrent( this ), fmtlgverInitialDbSizeLoggedMain ) < 0 )
    {
        pfmp->SetOlderDemandExtendDb();
    }

    // now with active logging dbscan, it is unnecessary to initiate a separate dbscan task
    // unless specified they specially want to override the default follow-active behavior.
    // start checksum on this database
    // Putting this in, so if someone runs this, they'll see that testing of this code is 
    // on them. It should work, because it is the same as the original DbScan in Recovery 
    // code.
    if ( UlParam( pfmp->Pinst(), JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryPassiveScan )
    {
        CallR( pfmp->ErrStartDBMScan() );
    }

HandleError:
    delete pfm;
    pfm = NULL;

    if( err < JET_errSuccess )
    {
        pfmp->DestroyFlushMap();
    }
    else
    {
        OSTrace( JET_tracetagDatabases, OSFormat( "Successfully attached database[0x%x]='%ws'", (ULONG)ifmp, wszDbName ) );
    }
    return err;
}

VOID LOG::LGRISetDeferredAttachment( const IFMP ifmp, const bool fDeferredAttachConsistentFuture, const bool fDeferredForAccessDenied )
{
    FMP     *pfmp   = &g_rgfmp[ifmp];

    Assert( NULL != pfmp );

    pfmp->RwlDetaching().EnterAsWriter();

    Assert( !pfmp->Pfapi() );
    Assert( pfmp->FInUse() );
    Assert( NULL != pfmp->Pdbfilehdr() );
    pfmp->FreePdbfilehdr();

    pfmp->RwlDetaching().LeaveAsWriter();

    // Versioning flag is not persisted (since versioning off
    // implies logging off).
    Assert( !pfmp->FVersioningOff() );
    pfmp->ResetVersioningOff();

    Assert( !pfmp->FReadOnlyAttach() );

    //  still have to set fFlags for keep track of the db status.
    FMP::EnterFMPPoolAsWriter();
    pfmp->SetLogOn( );
    FMP::LeaveFMPPoolAsWriter();

    pfmp->SetDeferredAttach( fDeferredAttachConsistentFuture, fDeferredForAccessDenied );
}

ERR LOG::ErrLGRIRedoBackupUpdate(
    LRLOGBACKUP *   plrlb )
{
    ERR err = JET_errSuccess;
    
    Assert( plrlb->phaseDetails.le_genLow != 0 );
    Assert( plrlb->phaseDetails.le_genHigh!= 0 );
    Assert( plrlb->phaseDetails.le_lgposMark.le_lGeneration != 0 );

    if ( !FLGRICheckRedoConditionForDb( plrlb->dbid, m_lgposRedo ) )
    {
        return JET_errSuccess;
    }

    //
    //  Redo the header update ...
    //

    LGPOS lgposT = plrlb->phaseDetails.le_lgposMark;

    //  reset legacy way ...
    m_pinst->m_pbackup->BKSetLgposFullBackup( lgposMin );
    m_pinst->m_pbackup->BKSetLgposIncBackup( lgposMin );
    m_pinst->m_pbackup->BKSetLogsTruncated( fFalse );

    //  redo the header update ...
    BOOL fT = fTrue;
    CallR( m_pinst->m_pbackup->ErrBKUpdateHdrBackupInfo(
                m_pinst->m_mpdbidifmp[ plrlb->dbid ],
                plrlb->eBackupType,
                LRLOGBACKUP::fLGBackupScopeIncremental == plrlb->eBackupScope || LRLOGBACKUP::fLGBackupScopeDifferential == plrlb->eBackupScope,
                LRLOGBACKUP::fLGBackupScopeIncremental == plrlb->eBackupScope || LRLOGBACKUP::fLGBackupScopeFull == plrlb->eBackupScope,
                plrlb->phaseDetails.le_genLow,
                plrlb->phaseDetails.le_genHigh,
                &lgposT,
                &(plrlb->phaseDetails.logtimeMark),
                &fT ) );
    
    return( err );
}


ERR LOG::ErrLGRIRedoSpaceRootPage(  PIB                 *ppib,
                                    const LRCREATEMEFDP *plrcreatemefdp,
                                    BOOL                fAvail )
{
    ERR         err;
    const DBID  dbid        = plrcreatemefdp->dbid;
    const PGNO  pgnoFDP     = plrcreatemefdp->le_pgno;
    const OBJID objidFDP    = plrcreatemefdp->le_objidFDP;
    const CPG   cpgPrimary  = plrcreatemefdp->le_cpgPrimary;
    const PGNO  pgnoRoot    = fAvail ?
                                    plrcreatemefdp->le_pgnoAE :
                                    plrcreatemefdp->le_pgnoOE ;
    const CPG   cpgExtent   = fAvail ?
                                cpgPrimary - 1 - 1 - 1 :
                                cpgPrimary;

    const PGNO  pgnoLast    = pgnoFDP + cpgPrimary - 1;
    const ULONG fPageFlags  = plrcreatemefdp->le_fPageFlags;

    FUCB        *pfucb      = pfucbNil;

    CSR     csr;
    INST    *pinst = PinstFromPpib( ppib );
    IFMP    ifmp = pinst->m_mpdbidifmp[ dbid ];

    BOOL fRedoNewPage = fFalse;
    Call( ErrLGRIAccessNewPage(
            ppib,
            &csr,
            ifmp,
            pgnoRoot,
            objidFDP,
            plrcreatemefdp->le_dbtime,
            &fRedoNewPage ) );
    if ( !fRedoNewPage )
    {
        csr.ReleasePage();
        goto HandleError;
    }
    Assert( !csr.FLatched() );

    LGRITraceRedo( plrcreatemefdp );

    Call( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, plrcreatemefdp->FUnique(), fTrue, &pfucb ) );
    pfucb->u.pfcb->SetPgnoOE( plrcreatemefdp->le_pgnoOE );
    pfucb->u.pfcb->SetPgnoAE( plrcreatemefdp->le_pgnoAE );
    Assert( plrcreatemefdp->le_pgnoOE + 1 == plrcreatemefdp->le_pgnoAE );

    Assert( FFUCBOwnExt( pfucb ) );
    FUCBResetOwnExt( pfucb );

    Call( csr.ErrGetNewPreInitPageForRedo(
                    ppib,
                    ifmp,
                    fAvail ? PgnoAE( pfucb ) : PgnoOE( pfucb ),
                    objidFDP,
                    plrcreatemefdp->le_dbtime ) );
    csr.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );

    SPICreateExtentTree( pfucb, &csr, pgnoLast, cpgExtent, fAvail );

    //  set dbtime to logged dbtime
    //  release page
    //
    Assert( latchWrite == csr.Latch() );
    Assert( csr.FDirty() );
    csr.SetDbtime( plrcreatemefdp->le_dbtime );
    csr.FinalizePreInitPage();
    csr.ReleasePage();

HandleError:
    if ( pfucb != pfucbNil )
    {
        FUCBSetOwnExt( pfucb );
    }

    Assert( !csr.FLatched() );
    return err;
}


//  redo single extent creation
//
ERR LOG::ErrLGIRedoFDPPage( CTableHash *pctablehash, PIB *ppib, const LRCREATESEFDP *plrcreatesefdp )
{
    ERR             err;
    const DBID      dbid        = plrcreatesefdp->dbid;
    const PGNO      pgnoFDP     = plrcreatesefdp->le_pgno;
    const OBJID     objidFDP    = plrcreatesefdp->le_objidFDP;
    const DBTIME    dbtime      = plrcreatesefdp->le_dbtime;
    const PGNO      pgnoParent  = plrcreatesefdp->le_pgnoFDPParent;
    const CPG       cpgPrimary  = plrcreatesefdp->le_cpgPrimary;
    const BOOL      fUnique     = plrcreatesefdp->FUnique();
    const ULONG     fPageFlags  = plrcreatesefdp->le_fPageFlags;


    FUCB            *pfucb;
    CSR             csr;
    INST            *pinst = PinstFromPpib( ppib );
    IFMP            ifmp = pinst->m_mpdbidifmp[ dbid ];

    BOOL fRedoNewPage = fFalse;
    Call( ErrLGRIAccessNewPage(
            ppib,
            &csr,
            ifmp,
            pgnoFDP,
            objidFDP,
            dbtime,
            &fRedoNewPage ) );
    if ( !fRedoNewPage )
    {
        csr.ReleasePage();
        goto HandleError;
    }
    Assert( !csr.FLatched() );

    Call( ErrLGRIGetFucb( pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fFalse, &pfucb ) );
    pfucb->u.pfcb->SetPgnoOE( pgnoNull );
    pfucb->u.pfcb->SetPgnoAE( pgnoNull );

    pfucb->u.pfcb->Lock();

    //  UNDONE: I can't tell if it's possible to get a cached FCB that used to belong to
    //  a different btree, so to be safe, reset the values for objidFDP and fUnique
    pfucb->u.pfcb->SetObjidFDP( objidFDP );
    if ( fUnique )
        pfucb->u.pfcb->SetUnique();
    else
        pfucb->u.pfcb->SetNonUnique();

    pfucb->u.pfcb->Unlock();

    //  It is okay to call ErrGetNewPreInitPage() instead of ErrGetNewPreInitPageForRedo()
    //  here because if the new page succeeds, we are guaranteed to make it
    //  to the code below that updates the dbtime
    Call( ErrSPICreateSingle(
                pfucb,
                &csr,
                pgnoParent,
                pgnoFDP,
                objidFDP,
                cpgPrimary,
                fUnique,
                fPageFlags,
                dbtime ) );

    //  set dbtime to logged dbtime
    //  release page
    //
    Assert( latchWrite == csr.Latch() );
    csr.SetDbtime( dbtime );
    csr.ReleasePage();

HandleError:
    Assert( !csr.FLatched() );
    return err;
}

//  redo multiple extent creation
//
ERR LOG::ErrLGIRedoFDPPage( CTableHash *pctablehash, PIB *ppib, const LRCREATEMEFDP *plrcreatemefdp )
{
    ERR             err;
    const DBID      dbid        = plrcreatemefdp->dbid;
    const PGNO      pgnoFDP     = plrcreatemefdp->le_pgno;
    const OBJID     objidFDP    = plrcreatemefdp->le_objidFDP;
    const DBTIME    dbtime      = plrcreatemefdp->le_dbtime;
    const PGNO      pgnoOE      = plrcreatemefdp->le_pgnoOE;
    const PGNO      pgnoAE      = plrcreatemefdp->le_pgnoAE;
    const PGNO      pgnoParent  = plrcreatemefdp->le_pgnoFDPParent;
    const CPG       cpgPrimary  = plrcreatemefdp->le_cpgPrimary;
    const BOOL      fUnique     = plrcreatemefdp->FUnique();
    const ULONG     fPageFlags  = plrcreatemefdp->le_fPageFlags;

    FUCB            *pfucb;
    SPACE_HEADER    sph;

    CSR     csr;
    INST            *pinst = PinstFromPpib( ppib );
    IFMP            ifmp = pinst->m_mpdbidifmp[ dbid ];

    BOOL fRedoNewPage = fFalse;
    Call( ErrLGRIAccessNewPage(
            ppib,
            &csr,
            ifmp,
            pgnoFDP,
            objidFDP,
            dbtime,
            &fRedoNewPage ) );
    if ( !fRedoNewPage )
    {
        csr.ReleasePage();
        goto HandleError;
    }
    Assert( !csr.FLatched() );

    Call( ErrLGRIGetFucb( pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fFalse, &pfucb ) );
    pfucb->u.pfcb->SetPgnoOE( pgnoOE );
    pfucb->u.pfcb->SetPgnoAE( pgnoAE );
    Assert( pgnoOE + 1 == pgnoAE );

    pfucb->u.pfcb->Lock();

    //  UNDONE: I can't tell if it's possible to get a cached FCB that used to belong to
    //  a different btree, so to be safe, reset the values for objidFDP and fUnique
    pfucb->u.pfcb->SetObjidFDP( objidFDP );
    if ( fUnique )
        pfucb->u.pfcb->SetUnique();
    else
        pfucb->u.pfcb->SetNonUnique();

    pfucb->u.pfcb->Unlock();

    //  get pgnoFDP to initialize in current CSR pgno
    //
    Call( csr.ErrGetNewPreInitPageForRedo(
                    pfucb->ppib,
                    pfucb->ifmp,
                    pgnoFDP,
                    objidFDP,
                    dbtime ) );
    csr.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf );

    sph.SetPgnoParent( pgnoParent );
    sph.SetCpgPrimary( cpgPrimary );

    Assert( sph.FSingleExtent() );  // initialised with these defaults
    Assert( sph.FUnique() );

    sph.SetMultipleExtent();

    if ( !fUnique )
    {
        sph.SetNonUnique();
    }

    sph.SetPgnoOE( pgnoOE );
    Assert( sph.PgnoOE() == pgnoFDP + 1 );

    SPIInitPgnoFDP( pfucb, &csr, sph );

    //  set dbtime to logged dbtime
    //  release page
    //
    Assert( latchWrite == csr.Latch() );
    Assert( csr.FDirty() );
    csr.SetDbtime( dbtime );
    csr.FinalizePreInitPage();
    csr.ReleasePage();

HandleError:
    Assert( !csr.FLatched() );
    return err;
}

ERR LOG::ErrLGRIRedoConvertFDP( PIB *ppib, const LRCONVERTFDP *plrconvertfdp )
{
    ERR             err;
    FUCB *          pfucb               = pfucbNil;
    INST *          pinst               = PinstFromPpib( ppib );
    const PGNO      pgnoFDP             = plrconvertfdp->le_pgno;
    const OBJID     objidFDP            = plrconvertfdp->le_objidFDP;
    const DBID      dbid                = plrconvertfdp->dbid;
    const IFMP      ifmp                = pinst->m_mpdbidifmp[ dbid ];
    const DBTIME    dbtime              = plrconvertfdp->le_dbtime;
    const PGNO      pgnoAE              = plrconvertfdp->le_pgnoAE;
    const PGNO      pgnoOE              = plrconvertfdp->le_pgnoOE;
    const PGNO      pgnoSecondaryFirst  = plrconvertfdp->le_pgnoSecondaryFirst;
    const CPG       cpgSecondary        = plrconvertfdp->le_cpgSecondary;
    BOOL            fRedoRoot           = fTrue;
    BOOL            fRedoAE;
    BOOL            fRedoOE;
    CSR             csrRoot;
    CSR             csrAE;
    CSR             csrOE;
    ULONG           fPageFlags;
    SPACE_HEADER    sph;
    INT             iextMac             = 0;
    EXTENTINFO      rgext[(cpgSmallSpaceAvailMost+1)/2 + 1];

    LGRITraceRedo( plrconvertfdp );

    //  get cursor for operation
    //
    Assert( g_rgfmp[ifmp].Dbid() == dbid );
    Call( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, plrconvertfdp->FUnique(), fTrue, &pfucb ) );
    pfucb->u.pfcb->SetPgnoOE( pgnoOE );
    pfucb->u.pfcb->SetPgnoAE( pgnoAE );
    Assert( pgnoOE + 1 == pgnoAE );

    err = ErrLGIAccessPage( ppib, &csrRoot, ifmp, pgnoFDP, objidFDP, fFalse );

    if ( errSkipLogRedoOperation == err )
    {
        Assert( pagetrimTrimmed == csrRoot.PagetrimState() );
        Assert( FRedoMapNeeded( ifmp ) );
        Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgnoFDP ) );
        err = JET_errSuccess;
        fRedoRoot = fFalse;
    }
    else
    {
        //  Check the return code of ErrLGIAccessPage() for other errors.
        Call( err );

        //  Check if the FDP page need be redone, based on dbtime.
        //
        fRedoRoot = FLGNeedRedoCheckDbtimeBefore( ifmp, csrRoot, dbtime, plrconvertfdp->le_dbtimeBefore, &err );

        // for the FLGNeedRedoCheckDbtimeBefore error code
        Call ( err );
    }

    //  get AvailExt and OwnExt root pages from db
    Call( ErrLGRIAccessNewPage(
                    ppib,
                    &csrOE,
                    ifmp,
                    pgnoOE,
                    objidFDP,
                    dbtime,
                    &fRedoOE ) );
    Call( ErrLGRIAccessNewPage(
                    ppib,
                    &csrAE,
                    ifmp,
                    pgnoAE,
                    objidFDP,
                    dbtime,
                    &fRedoAE ) );

    if ( fRedoRoot || fRedoOE || fRedoAE )
    {
        //  reconstruct page flags and extent info from log record
        //
        fPageFlags = plrconvertfdp->le_fCpageFlags;

        sph.SetCpgPrimary( plrconvertfdp->le_cpgPrimary );
        sph.SetPgnoParent( plrconvertfdp->le_pgnoFDPParent );
        sph.SetRgbitAvail( plrconvertfdp->le_rgbitAvail );

        Assert( sph.FUnique() );
        if ( !plrconvertfdp->FUnique() )
        {
            sph.SetNonUnique();
        }

        SPIConvertCalcExtents( sph, pgnoFDP, rgext, &iextMac );
    }
    else
    {
        //  no pages need to be redone, so bail
        //
        err = JET_errSuccess;
        goto HandleError;
    }

    //  force to multiple extent
    //
    Assert( sph.FSingleExtent() );
    sph.SetMultipleExtent();
    Assert( pgnoOE == pgnoSecondaryFirst );
    sph.SetPgnoOE( pgnoSecondaryFirst );

    //  create new pages for OwnExt and AvailExt root
    //      if redo is needed
    //
    if ( fRedoOE )
    {
        Call( csrOE.ErrGetNewPreInitPageForRedo(
                        ppib,
                        ifmp,
                        pgnoOE,
                        objidFDP,
                        dbtime ) );
        csrOE.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );
        Assert( latchWrite == csrOE.Latch() );
    }

    if ( fRedoAE )
    {
        Call( csrAE.ErrGetNewPreInitPageForRedo(
                        ppib,
                        ifmp,
                        pgnoAE,
                        objidFDP,
                        dbtime ) );
        csrAE.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf | CPAGE::fPageSpaceTree );
        Assert( latchWrite == csrAE.Latch() );
    }

    if ( fRedoRoot )
    {
        Assert( FAssertLGNeedRedo( ifmp, csrRoot, dbtime, plrconvertfdp->le_dbtimeBefore ) );
        csrRoot.UpgradeFromRIWLatch();
    }

    //  reset flag to silence asserts
    //
    Assert( FFUCBOwnExt( pfucb ) );
    FUCBResetOwnExt( pfucb );

    //  dirty pages and set dbtime to logged dbtime
    //
    LGRIRedoDirtyAndSetDbtime( &csrRoot, dbtime );
    LGRIRedoDirtyAndSetDbtime( &csrAE, dbtime );
    LGRIRedoDirtyAndSetDbtime( &csrOE, dbtime );

    SPIPerformConvert( pfucb, &csrRoot, &csrAE, &csrOE, &sph, pgnoSecondaryFirst, cpgSecondary, rgext, iextMac );

    if ( fRedoOE )
    {
        csrOE.FinalizePreInitPage();
    }

    if ( fRedoAE )
    {
        csrAE.FinalizePreInitPage();
    }

HandleError:
    csrRoot.ReleasePage();
    csrOE.ReleasePage();
    csrAE.ReleasePage();

    if ( pfucb != pfucbNil )
    {
        FUCBSetOwnExt( pfucb );
    }

    return err;
}

ERR LOG::ErrLGRIRedoOperation( LR *plr )
{
    ERR     err = JET_errSuccess;
    LEVEL   levelCommitTo;

    Assert( !m_fNeedInitialDbList );

    switch ( plr->lrtyp )
    {

    default:
    {
#ifndef RFS2
        AssertSz( fFalse, "Debug Only, Ignore this Assert" );
#endif
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"e40383d3-f62c-4d33-8047-b1bb5bf38c9d" );
        return ErrERRCheck( JET_errLogCorrupted );
    }

    //  ****************************************************
    //  single-page operations
    //  ****************************************************

    case lrtypInsert:
    case lrtypFlagInsert:
    case lrtypFlagInsertAndReplaceData:
    case lrtypReplace:
    case lrtypReplaceD:
    case lrtypFlagDelete:
    case lrtypDelete:
    case lrtypDelta:
    case lrtypDelta64:
    case lrtypUndo:
    case lrtypUndoInfo:
    case lrtypSetExternalHeader:
    case lrtypEmptyTree:
    {
        err = ErrLGRIRedoNodeOperation( (LRNODE_ * ) plr, &m_errGlobalRedoError );

        //Changing it to use switch so that we can set multiple error codes at the same time..
        switch( err )
        {
            case JET_errReadLostFlushVerifyFailure:
            case JET_errPageNotInitialized:
            case JET_errDbTimeTooNew:
            case JET_errDbTimeTooOld:
                Assert( FNegTest( fCorruptingWithLostFlush ) );
                break;

            case JET_errReadVerifyFailure:
            case JET_errDiskReadVerificationFailure:
            case JET_errRecoveryVerifyFailure:
                break;

            case JET_errDiskIO:
                Assert( FNegTest( fDiskIOError ) );
                break;

            case errSkipLogRedoOperation:
            {
#ifdef DEBUG
                const LRNODE_* plrNode = (LRNODE_ * ) plr;
                const IFMP  ifmp = m_pinst->m_mpdbidifmp[ plrNode->dbid ];
                Assert( FRedoMapNeeded( ifmp ) );
                Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( plrNode->le_pgno ) );
#endif
                break;
            }

            case JET_errOutOfMemory:
            case JET_errOutOfCursors:
                break;

            default:
                Assert( JET_errSuccess == err );
        }
        
        CallR( err );
        break;
    }


    /****************************************************
     *     Transaction Operations                       *
     ****************************************************/

    case lrtypBegin:
    case lrtypBegin0:
    {
        LRBEGINDT   *plrbeginDT = (LRBEGINDT *)plr;
        PIB         *ppib;

        LGRITraceRedo( plr );

        Assert( plrbeginDT->clevelsToBegin >= 0 );
        Assert( plrbeginDT->clevelsToBegin <= levelMax );
        CallR( ErrLGRIPpibFromProcid( plrbeginDT->le_procid, &ppib ) );

        Assert( !ppib->FMacroGoing() );

        /*  do BT only after first BT based on level 0 is executed
        /**/
        if ( ppib->FAfterFirstBT() || 0 == plrbeginDT->levelBeginFrom )
        {
            Assert( ppib->Level() <= plrbeginDT->levelBeginFrom );

            if ( 0 == ppib->Level() )
            {
                Assert( 0 == plrbeginDT->levelBeginFrom );
                Assert( lrtypBegin != plr->lrtyp );
                ppib->trxBegin0 = plrbeginDT->le_trxBegin0;
                ppib->trxCommit0 = trxMax;
                ppib->lgposStart = m_lgposRedo;
                ppib->SetFBegin0Logged();
                if ( !m_pinst->m_fTrxNewestSetByRecovery ||
                     TrxCmp( ppib->trxBegin0, m_pinst->m_trxNewest ) > 0 )
                {
                    AtomicExchange( (LONG *)&m_pinst->m_trxNewest, ppib->trxBegin0 );
                    m_pinst->m_fTrxNewestSetByRecovery = fTrue;
                }
                //  at redo time RCEClean can throw away any committed RCE as they are only
                //  needed for rollback
            }
            else
            {
                Assert( lrtypBegin == plr->lrtyp );
            }

            /*  issue begin transactions
            /**/
            while ( ppib->Level() < plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin )
            {
                VERBeginTransaction( ppib, 61477 );
            }

            /*  assert at correct transaction level
            /**/
            Assert( ppib->Level() == plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin );

            ppib->SetFAfterFirstBT();
        }
        break;
    }

    case lrtypRefresh:
    {
        LRREFRESH   *plrrefresh = (LRREFRESH *)plr;
        PIB         *ppib;

        LGRITraceRedo( plr );

        CallR( ErrLGRIPpibFromProcid( plrrefresh->le_procid, &ppib ) );

        Assert( !ppib->FMacroGoing() );
        if ( !ppib->FAfterFirstBT() )
            break;

        /*  imitate a begin transaction.
         */
        Assert( ppib->Level() <= 1 );
        ppib->SetLevel( 1 );
        ppib->trxBegin0 = plrrefresh->le_trxBegin0;
        ppib->trxCommit0 = trxMax;

        break;
    }

    case lrtypCommit:
    case lrtypCommit0:
    {
        LRCOMMIT0   *plrcommit0 = (LRCOMMIT0 *)plr;
        PIB         *ppib;

        CallR( ErrLGRIPpibFromProcid( plrcommit0->le_procid, &ppib ) );

        if ( !ppib->FAfterFirstBT() )
        {
            // If we have not seen the Begin0 for this transaction, still need to update
            // m_trxNewest based on this. We should still be replaying in the initial required
            // range in this case, so no read-from-passive transactions yet.
            if ( 0 == plrcommit0->levelCommitTo &&
                 ( !m_pinst->m_fTrxNewestSetByRecovery ||
                   TrxCmp( plrcommit0->le_trxCommit0, m_pinst->m_trxNewest ) > 0 ) )
            {
                AtomicExchange( (LONG *)&m_pinst->m_trxNewest, plrcommit0->le_trxCommit0 );
                m_pinst->m_fTrxNewestSetByRecovery = fTrue;
            }

            break;
        }

        /*  check transaction level
        /**/
        Assert( !ppib->FMacroGoing() );
        Assert( ppib->Level() >= 1 );

        LGRITraceRedo( plr );

        levelCommitTo = plrcommit0->levelCommitTo;
        Assert( levelCommitTo <= ppib->Level() );

        while ( ppib->Level() > levelCommitTo )
        {
            Assert( ppib->Level() > 0 );
            if ( 1 == ppib->Level() )
            {
                Assert( lrtypCommit0 == plr->lrtyp );
                if ( ppib->CbClientCommitContextGeneric() )
                {
                    Assert( ppib->FCommitContextNeedPreCommitCallback() );
                    CallR( ErrLGCommitCtxCallback( m_pinst, ppib->PvClientCommitContextGeneric(), ppib->CbClientCommitContextGeneric(), fCommitCtxPreCommitCallback ) );
                }
                ppib->trxCommit0 = plrcommit0->le_trxCommit0;
                if ( !m_pinst->m_fTrxNewestSetByRecovery ||
                     TrxCmp( ppib->trxCommit0, m_pinst->m_trxNewest ) > 0 )
                {
                    AtomicExchange( (LONG *)&m_pinst->m_trxNewest, ppib->trxCommit0 );
                    m_pinst->m_fTrxNewestSetByRecovery = fTrue;
                }
                LGPOS lgposCurrent = m_lgposRedo;
                LGAddLgpos( &lgposCurrent, sizeof( LRCOMMIT0 ) - 1 );
                ppib->lgposCommit0 = lgposCurrent;
                VERCommitTransaction( ppib );
                if ( ppib->CbClientCommitContextGeneric() )
                {
                    Assert( ppib->FCommitContextNeedPreCommitCallback() );
                    CallR( ErrLGCommitCtxCallback( m_pinst, ppib->PvClientCommitContextGeneric(), ppib->CbClientCommitContextGeneric(), fCommitCtxPostCommitCallback ) );
                }
                LGICleanupTransactionToLevel0( ppib, m_pctablehash );
            }
            else
            {
                Assert( lrtypCommit == plr->lrtyp );
                VERCommitTransaction( ppib );
            }
        }

        break;
    }

    case lrtypRollback:
    {
        LRROLLBACK  *plrrollback = (LRROLLBACK *)plr;
        LEVEL       level = plrrollback->levelRollback;
        PIB         *ppib;

        CallR( ErrLGRIPpibFromProcid( plrrollback->le_procid, &ppib ) );

        Assert( !ppib->FMacroGoing() );
        if ( !ppib->FAfterFirstBT() )
            break;

        /*  check transaction level
        /**/
        Assert( ppib->Level() >= level );

        LGRITraceRedo( plr );

        while ( level-- && ppib->Level() > 0 )
        {
            err = ErrVERRollback( ppib );
            CallSx( err, JET_errRollbackError );
            CallR ( err );
        }

        if ( 0 == ppib->Level() )
        {
            LGICleanupTransactionToLevel0( ppib, m_pctablehash );
        }

        break;
    }

    /************************************************************
     * Operations performed via macro (split, merge, root move) *
     ************************************************************/

    case lrtypSplit:
    case lrtypMerge:
        FireWall( OSFormat( "DeprecatedLrType:%d", (INT)plr->lrtyp ) );
        CallR( ErrERRCheck( JET_errBadLogVersion ) );
    case lrtypSplit2:
    case lrtypMerge2:
    case lrtypRootPageMove:
    {
        DBID dbid = dbidMin;
        DBTIME dbtime = dbtimeInvalid;
        PROCID procid = procidNil;
        PIB * ppib = ppibNil;

        switch( plr->lrtyp )
        {
            case lrtypSplit:
            case lrtypSplit2:
            case lrtypMerge:
            case lrtypMerge2:
                {
                const LRPAGE_ * const plrpage = (LRPAGE_ *)plr;
                dbid = plrpage->dbid;
                dbtime = plrpage->le_dbtime;
                procid = plrpage->le_procid;
                }
                break;
            case lrtypRootPageMove:
                {
                const LRROOTPAGEMOVE * const plrrpm = (LRROOTPAGEMOVE *)plr;
                dbid = plrrpm->Dbid();
                dbtime = plrrpm->DbtimeAfter();
                procid = plrrpm->Procid();
                }
                break;
            default:
                Assert( fFalse );
                break;
        }

        CallR( ErrLGRIPpibFromProcid( procid, &ppib ) );

        if ( !ppib->FAfterFirstBT() )
        {
            //  it's possible that there are no Begin0's between the first and
            //  last log records, in which case nothing needs to
            //  get redone.  HOWEVER, the dbtimeLast and objidLast
            //  counters in the db header would not have gotten
            //  flushed (they only get flushed on a DetachDb or a
            //  clean shutdown), so we must still track these
            //  counters during recovery so that we can properly
            //  update the database header on RecoveryQuit (since
            //  we pass TRUE for the fUpdateCountersOnly param to
            //  ErrLGRICheckRedoCondition() below, that function
            //  will do nothing but update the counters for us).

            BOOL fSkip = fFalse;
            OBJID objidFDP = 0xFFFFFFFF;
            switch( plr->lrtyp )
            {
                case lrtypSplit:
                case lrtypSplit2:
                    objidFDP = ( (LRSPLIT_ *)plr )->le_objidFDP;
                    break;
                case lrtypMerge:
                case lrtypMerge2:
                    objidFDP = ( (LRMERGE_ *)plr )->le_objidFDP;
                    break;
                case lrtypRootPageMove:
                    objidFDP = ( (LRROOTPAGEMOVE *)plr )->Objid();
                    break;
                default:
                    Assert( fFalse );
                    break;
            }

            CallS( ErrLGRICheckRedoCondition(
                        dbid,
                        dbtime,
                        objidFDP,
                        ppib,
                        fTrue,
                        &fSkip ) );
            Assert( fSkip );
            break;
        }

        Assert( ppib->FMacroGoing( dbtime ) );

        CallR( ErrLGIStoreLogRec( ppib, dbtime, plr ) );
        break;
    }


    //***************************************************
    //  Misc Operations
    //***************************************************

    case lrtypCreateMultipleExtentFDP:
    {
        LRCREATEMEFDP   *plrcreatemefdp = (LRCREATEMEFDP *)plr;
        const DBID      dbid = plrcreatemefdp->dbid;
        PIB             *ppib;

        INST            *pinst = m_pinst;
        IFMP            ifmp = pinst->m_mpdbidifmp[ dbid ];

        if ( g_ifmpMax == ifmp )
            break;

        FMP::AssertVALIDIFMP( ifmp );

        //  remove FCBs/FUCBs created on pgnoFDP earlier. This call is only needed if active is running older
        //  version without lrtypFreeFDP
        if ( NULL != m_pctablehash )
        {
            CallR( ErrLGRIPurgeFcbs( ifmp, plrcreatemefdp->le_pgno, fFDPTypeUnknown, m_pctablehash ) );
        }

        BOOL fSkip;
        CallR( ErrLGRICheckRedoConditionInTrx(
                plrcreatemefdp->le_procid,
                dbid,
                plrcreatemefdp->le_dbtime,
                plrcreatemefdp->le_objidFDP,
                NULL,   //  can not be in macro.
                &ppib,
                &fSkip ) );
        if ( fSkip )
            break;

        LGRITraceRedo( plrcreatemefdp );
        CallR( ErrLGIRedoFDPPage( m_pctablehash, ppib, plrcreatemefdp ) );

        CallR( ErrLGRIRedoSpaceRootPage( ppib, plrcreatemefdp, fTrue ) );

        CallR( ErrLGRIRedoSpaceRootPage( ppib, plrcreatemefdp, fFalse ) );
        break;
    }

    case lrtypCreateSingleExtentFDP:
    {
        LRCREATESEFDP   *plrcreatesefdp = (LRCREATESEFDP *)plr;
        const PGNO      pgnoFDP = plrcreatesefdp->le_pgno;
        const DBID      dbid = plrcreatesefdp->dbid;
        PIB             *ppib;

        INST            *pinst = m_pinst;
        IFMP            ifmp = pinst->m_mpdbidifmp[ dbid ];

        if ( g_ifmpMax == ifmp )
            break;

        FMP::AssertVALIDIFMP( ifmp );

        //  remove FCBs/FUCBs created on pgnoFDP earlier. This call is only needed if active is running older
        //  version without lrtypFreeFDP
        if ( NULL != m_pctablehash )
        {
            CallR( ErrLGRIPurgeFcbs( ifmp, pgnoFDP, fFDPTypeUnknown, m_pctablehash ) );
        }

        BOOL fSkip;
        CallR( ErrLGRICheckRedoConditionInTrx(
                    plrcreatesefdp->le_procid,
                    dbid,
                    plrcreatesefdp->le_dbtime,
                    plrcreatesefdp->le_objidFDP,
                    NULL,
                    &ppib,
                    &fSkip ) );
        if ( fSkip )
            break;

        //  redo FDP page if needed
        //
        LGRITraceRedo( plrcreatesefdp );
        CallR( ErrLGIRedoFDPPage( m_pctablehash, ppib, plrcreatesefdp ) );
        break;
    }

    case lrtypConvertFDP:
        FireWall( OSFormat( "DeprecatedLrType:%d", (INT)plr->lrtyp ) );

        //  ErrLGRIRedoConvertFDP() can no longer handle lrtypConvertFDP
        //
        CallR( ErrERRCheck( JET_errBadLogVersion ) );
    case lrtypConvertFDP2:
    {
        LRCONVERTFDP    *plrconvertfdp = (LRCONVERTFDP *)plr;
        PIB             *ppib;

        BOOL fSkip;
        CallR( ErrLGRICheckRedoConditionInTrx(
                plrconvertfdp->le_procid,
                plrconvertfdp->dbid,
                plrconvertfdp->le_dbtime,
                plrconvertfdp->le_objidFDP,
                plr,
                &ppib,
                &fSkip ) );
        if ( fSkip )
            break;

        CallR( ErrLGRIRedoConvertFDP( ppib, plrconvertfdp ) );
        break;
    }

    case lrtypFreeFDP:
    {
        const LRFREEFDP *plrFreeFDP = (LRFREEFDP *)plr;
        const PGNO      pgnoFDP = plrFreeFDP->le_pgnoFDP;
        const TRX       trxCommitted = plrFreeFDP->le_trxCommitted;
        const FDPTYPE   fFDPType = plrFreeFDP->FFDPType();
        const DBID      dbid = plrFreeFDP->le_dbid;
        const IFMP      ifmp = m_pinst->m_mpdbidifmp[ dbid ];

        if ( g_ifmpMax == ifmp )
            break;

        Assert( fFDPType == fFDPTypeTable || fFDPType == fFDPTypeIndex );

        if ( !m_pinst->m_fTrxNewestSetByRecovery ||
             TrxCmp( trxCommitted, m_pinst->m_trxNewest ) > 0 )
        {
            AtomicExchange( (LONG *)&m_pinst->m_trxNewest, trxCommitted );
            m_pinst->m_fTrxNewestSetByRecovery = fTrue;
        }

        // First wait for any open transactions which will be able to see the catalog entries for this FDP
        // to go away.
        TICK tickStart = TickOSTimeCurrent();
        while ( TrxCmp( trxCommitted, TrxOldest( m_pinst ) ) >= 0 )
        {
            // Assert that we are not holding any locks
            CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );
            m_pinst->m_sigTrxOldestDuringRecovery.FWait( cmsecMaxReplayDelayDueToReadTrx );
        }

        // Free up all FCBs/FUCBs created against this pgnoFDP as the space can get reused and even though
        // recovery will not access it, we also do not want read from passive transactions to access it.
        if ( NULL != m_pctablehash )
        {
            CallR( ErrLGRIPurgeFcbs( ifmp, pgnoFDP, fFDPType, m_pctablehash ) );
        }

        TICK tickEnd = TickOSTimeCurrent();
        if ( tickEnd - tickStart >= cmsecMaxReplayDelayDueToReadTrx )
        {
            PERFOpt( cLGRecoveryLongStallReadOnly.Inc( m_pinst ) );

            WCHAR szTime[16];
            const WCHAR *rgsz[1] = { szTime };
            OSStrCbFormatW( szTime, sizeof(szTime), L"%d", tickEnd - tickStart );
            UtilReportEvent(
                    eventWarning,
                    LOGGING_RECOVERY_CATEGORY,
                    RECOVERY_LONG_WAIT_READ_ONLY,
                    1,
                    rgsz,
                    0,
                    NULL,
                    m_pinst );
        }
        else if ( tickEnd - tickStart >= cmsecMaxReplayDelayDueToReadTrx / 10 )
        {
            PERFOpt( cLGRecoveryStallReadOnly.Inc( m_pinst ) );

            WCHAR szTime[16];
            const WCHAR *rgsz[1] = { szTime };
            OSStrCbFormatW( szTime, sizeof(szTime), L"%d", tickEnd - tickStart );
            UtilReportEvent(
                    eventInformation,
                    LOGGING_RECOVERY_CATEGORY,
                    RECOVERY_WAIT_READ_ONLY,
                    1,
                    rgsz,
                    0,
                    NULL,
                    m_pinst,
                    JET_EventLoggingLevelMedium );
        }
        if ( tickEnd != tickStart )
        {
            PERFOpt( cLGRecoveryStallReadOnlyTime.Add( m_pinst, tickEnd - tickStart ) );
        }

        break;
    }

    case lrtypScrub:
    {
        const LRSCRUB * const plrscrub = (LRSCRUB *)plr;
        CallR( ErrLGRIRedoScrub( plrscrub ) );
        break;
    }

    case lrtypPageMove:
    {
        const LRPAGEMOVE * const plrpagemove = LRPAGEMOVE::PlrpagemoveFromLr( plr );
        CallR( ErrLGRIRedoPageMove( plrpagemove ) );
        break;
    }

    case lrtypPagePatchRequest:
    {
        // If we have a recovery callback then issue a page patch call. By the time we see the patch request
        // we have replayed all the log records for the page so we can use the current version.
        
        const LRPAGEPATCHREQUEST * const plrpagepatchrequest = (LRPAGEPATCHREQUEST *)plr;
        CallR( ErrLGRIRedoPagePatch( plrpagepatchrequest ) );
        
        break;
    }

    case lrtypScanCheck:    //  originally lrtypIgnored4
    case lrtypScanCheck2:
    {
        LRSCANCHECK2 lrscancheck;
        lrscancheck.InitScanCheck( plr );

        // check whether scanning is enabled and whether this is a
        // pgno we haven't seen yet in this scan
        if ( FLGRICheckRedoScanCheck( &lrscancheck, fFalse /* fEvaluatePrereadLogic */ ) )
        {
            //  we ignore errors if this is DB scan because we do not need to replay this log record to
            //  continue, and to allow HA to fail over to this node without blocking.
            BOOL fBadPage = fFalse;
            const ERR errT = ErrLGRIRedoScanCheck( &lrscancheck, &fBadPage );
            ExpectedSz( ( lrscancheck.BSource() == scsDbScan ) ||
                        ( lrscancheck.BSource() == scsDbShrink ),
                        "Select the appropriate action below for this new ScanCheck source." );
            if ( ( lrscancheck.BSource() == scsDbShrink ) &&
                 ( ( errT < JET_errSuccess ) || fBadPage ) )
            {
                if ( fBadPage )
                {
                    Assert( errT < JET_errSuccess );
                    AssertTrack( FNegTest( fCorruptingWithLostFlush ), OSFormat( "RedoScanCheckFailed:%d", errT ) );
                    OSUHAEmitFailureTag( m_pinst, HaDbFailureTagTargetReplicaShrinkDivergence, L"6a6ad1eb-be3a-4f9c-a80a-d275732d62a7" );
                }

                CallR( errT );
            }
        }

        break;
    }

    case lrtypReAttach:     //  originally lrtypIgnored6
    {
        const LRREATTACHDB * const plrreattach = (LRREATTACHDB *)plr;

        // Databases re-attached (from fast failover)
        IFMP ifmp = m_pinst->m_mpdbidifmp[ plrreattach->le_dbid ];
        if ( ifmp >= g_ifmpMax )
        {
            break;
        }

        FMP *pfmp = &g_rgfmp[ifmp];
        Assert( NULL != pfmp );
        Assert( pfmp->FInUse() );

        // deferred and skipped attach databases can be ignored
        if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
        {
            break;
        }

        Assert( NULL != pfmp->Pdbfilehdr() );
        Assert( NULL != pfmp->Patchchk() );

        { // .ctor acquires PdbfilehdrReadWrite
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

        // Set re-attach time
        pdbfilehdr->le_lgposLastReAttach  = m_lgposRedo;
        LGIGetDateTime( &pdbfilehdr->logtimeLastReAttach );
        } // .dtor releases PdbfilehdrReadWrite

        CallR( ErrUtilWriteAttachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp,  pfmp->Pfapi() ) );

        if ( !pfmp->FContainsDataFromFutureLogs() )
        {
            CallR( ErrLGDbAttachedCallback( m_pinst, pfmp ) );
        }

        // now with active logging dbscan, it is unnecessary to initiate a separate dbscan task
        // unless specified they specially want to override the default follow-active behavior.
        // start checksum on this database - normally done by
        // ErrLGRIRedoAttachDb above
        if ( UlParam( m_pinst, JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryPassiveScan )
        {
            CallR( pfmp->ErrStartDBMScan() );
        }

        break;
    }

    case lrtypSignalAttachDb:
    {
        const LRSIGNALATTACHDB * const plrsattachdb = (LRSIGNALATTACHDB *)plr;

        // Databases re-attached (from fast failover)
        IFMP ifmp = m_pinst->m_mpdbidifmp[ plrsattachdb->Dbid() ];
        if ( ifmp >= g_ifmpMax )
        {
            break;
        }

        FMP *pfmp = &g_rgfmp[ifmp];
        Assert( NULL != pfmp );
        Assert( pfmp->FInUse() );

        // deferred and skipped attach databases can be ignored
        if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
        {
            break;
        }

        Assert( NULL != pfmp->Pdbfilehdr() );
        Assert( NULL != pfmp->Patchchk() );

        if ( !pfmp->FContainsDataFromFutureLogs() )
        {
            CallR( ErrLGDbAttachedCallback( m_pinst, pfmp ) );
        }
        break;
    }

    case lrtypSLVPageAppendOBSOLETE:
    case lrtypSLVSpaceOBSOLETE:
    {
        return ErrERRCheck( JET_wrnNyi );
    }

    case lrtypCommitCtx:    // originally lrtypIgnored3
    {
        const LRCOMMITCTX * const plrCommitC = (LRCOMMITCTX *)plr;
        if ( plrCommitC->FPreCommitCallbackNeeded() )
        {
            // If client wants pre/post commit callback, just store the commit context and do the callback when the commit happens
            PIB *ppib;
            CallR( ErrLGRIPpibFromProcid( plrCommitC->ProcID(), &ppib ) );
            if ( ppib->FAfterFirstBT() )
            {
                Assert( ppib->Level() > 0 );
                CallR( ppib->ErrSetClientCommitContextGeneric( plrCommitC->PbCommitCtx(), plrCommitC->CbCommitCtx() ) );
                ppib->SetFCommitContextNeedPreCommitCallback( fTrue );
            }
        }
        else if ( plrCommitC->FCallbackNeeded() )
        {
            CallR( ErrLGCommitCtxCallback( m_pinst, plrCommitC->PbCommitCtx(), plrCommitC->CbCommitCtx(), fCommitCtxLegacyCommitCallback ) );
        }
        break;
    }

    case lrtypNewPage:
    {
        const LRNEWPAGE* const plrnewpage = (LRNEWPAGE *)plr;
        CallR( ErrLGRIRedoNewPage( plrnewpage ) );
        break;
    }

    case lrtypMacroInfo:    // originally lrtypIgnored1
    case lrtypMacroInfo2:   // originally lrtypIgnored7
    case lrtypIgnored9:
    case lrtypIgnored10:
    case lrtypIgnored11:
    case lrtypIgnored12:
    case lrtypIgnored13:
    case lrtypIgnored14:
    case lrtypIgnored15:
    case lrtypIgnored16:
    case lrtypIgnored17:
    case lrtypIgnored18:
    case lrtypIgnored19:
        break;

    case lrtypShrinkDB:
    case lrtypShrinkDB2:
    case lrtypShrinkDB3:
    case lrtypTrimDB:
    case lrtypExtentFreed:
        AssertSz( fFalse, "lrtyp %d should have been filtered out in ErrLGRIRedoOperations!", (BYTE)plr->lrtyp );
} /*** end of switch statement ***/

    return JET_errSuccess;
}

//  reconstructs rglineinfo during recovery
//      calcualtes kdf and cbPrefix of lineinfo
//      cbSize info is not calculated correctly since it is not needed in redo
//
LOCAL ERR ErrLGIRedoSplitLineinfo( FUCB                 *pfucb,
                                   SPLITPATH            *psplitPath,
                                   DBTIME               dbtime,
                                   const KEYDATAFLAGS&  kdf,
                                   const BOOL           fDebugHasPageBeforeImage )
{
    ERR     err;
    SPLIT * psplit = psplitPath->psplit;

    Assert( psplit != NULL );
    Assert( psplit->psplitPath == psplitPath );
    Assert( NULL == psplit->rglineinfo );
    Assert( FAssertLGNeedRedo( pfucb->ifmp, psplitPath->csr,dbtime, psplitPath->dbtimeBefore )
        || !FBTISplitPageBeforeImageRequired( psplit )
        || fDebugHasPageBeforeImage );

    if ( psplit->clines < 0 || psplit->clines > 1000000 )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagRecoveryRedoLogCorruption, L"2dfb97c9-80ee-4438-ba68-0d4953cf09ad" );
        return ErrERRCheck( JET_errLogFileCorrupt );
    }

    AllocR( psplit->rglineinfo = new LINEINFO[psplit->clines] );
    memset( psplit->rglineinfo, 0, sizeof( LINEINFO ) * psplit->clines );

    if ( !FLGNeedRedoCheckDbtimeBefore( pfucb->ifmp, psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err ) )
    {
        CallS( err );

        //  split page does not need redo but new page needs redo
        //  set rglineinfo and cbPrefix for appended node
        //
        Assert( !FBTISplitPageBeforeImageRequired( psplit ) || fDebugHasPageBeforeImage );
        Assert( psplit->clines - 1 == psplit->ilineOper );
        Assert( psplit->ilineSplit == psplit->ilineOper );
        Assert( FLGNeedRedoPage( psplit->csrNew, dbtime ) );

        psplit->rglineinfo[psplit->ilineOper].kdf = kdf;

        if ( ilineInvalid != psplit->prefixinfoNew.ilinePrefix )
        {
            Assert( 0 == psplit->prefixinfoNew.ilinePrefix );
            Assert( kdf.key.Cb() > cbPrefixOverhead );
            psplit->rglineinfo[psplit->ilineOper].cbPrefix = kdf.key.Cb();
        }

        return JET_errSuccess;
    }
    else
    {
        Call( err );
    }

    INT     ilineFrom;
    INT     ilineTo;

    for ( ilineFrom = 0, ilineTo = 0; ilineTo < psplit->clines; ilineTo++ )
    {
        if ( psplit->ilineOper == ilineTo &&
             splitoperInsert == psplit->splitoper )
        {
            //  place to be inserted node here
            //
            psplit->rglineinfo[ilineTo].kdf = kdf;

            //  do not increment ilineFrom
            //
            continue;
        }

        //  get node from page
        //
        psplitPath->csr.SetILine( ilineFrom );

        Call( ErrNDGet( pfucb, &psplitPath->csr ) );

        if ( ilineTo == psplit->ilineOper &&
             splitoperNone != psplit->splitoper )
        {
            //  get key from node
            //  and data from parameter
            //
            Assert( splitoperInsert != psplit->splitoper );
            Assert( splitoperReplace == psplit->splitoper ||
                    splitoperFlagInsertAndReplaceData == psplit->splitoper );

            psplit->rglineinfo[ilineTo].kdf.key     = pfucb->kdfCurr.key;
            psplit->rglineinfo[ilineTo].kdf.data    = kdf.data;
            psplit->rglineinfo[ilineTo].kdf.fFlags  = pfucb->kdfCurr.fFlags;
        }
        else
        {
            psplit->rglineinfo[ilineTo].kdf         = pfucb->kdfCurr;
        }

        Assert( ilineFrom <= ilineTo &&
                ilineFrom + 1 >= ilineTo );
        ilineFrom++;
    }

    //  set cbPrefixes for nodes in split page
    //
    if ( psplit->prefixinfoSplit.ilinePrefix != ilineInvalid )
    {
        Assert( psplit->prefixSplitNew.Cb() > 0 );

        KEY     keyPrefix;
        keyPrefix.Nullify();
        keyPrefix.suffix = psplit->prefixSplitNew;
        Assert( FKeysEqual( keyPrefix,
                        psplit->rglineinfo[psplit->prefixinfoSplit.ilinePrefix].kdf.key ) );

        AssertPREFIX( psplit->ilineSplit  <= psplit->clines );
        
        for ( INT iline = 0; iline < psplit->ilineSplit ; iline++ )
        {
            LINEINFO    *plineinfo = &psplit->rglineinfo[iline];
            const INT   cbCommon = CbCommonKey( keyPrefix, plineinfo->kdf.key );

            Assert( 0 == plineinfo->cbPrefix );
            if ( cbCommon > cbPrefixOverhead )
            {
                plineinfo->cbPrefix = cbCommon;
            }
        }
    }

    //  set cbPrefixes for nodes in new page
    //
    if ( FLGNeedRedoPage( psplit->csrNew, dbtime )
        && ilineInvalid != psplit->prefixinfoNew.ilinePrefix )
    {
        const INT   ilinePrefix = psplit->ilineSplit +
                                  psplit->prefixinfoNew.ilinePrefix;
        Assert( psplit->clines > ilinePrefix );

        KEY     keyPrefix = psplit->rglineinfo[ilinePrefix].kdf.key;

        for ( INT iline = psplit->ilineSplit; iline < psplit->clines ; iline++ )
        {
            LINEINFO    *plineinfo = &psplit->rglineinfo[iline];
            const INT   cbCommon = CbCommonKey( keyPrefix, plineinfo->kdf.key );

            Assert( 0 == plineinfo->cbPrefix );
            if ( cbCommon > cbPrefixOverhead )
            {
                plineinfo->cbPrefix = cbCommon;
            }
        }
    }
    return JET_errSuccess;

HandleError:
    delete [] psplit->rglineinfo;

    return err;
}


ERR ErrLGIDecompressPreimage(
    DATA &data,
    const LONG cbPage,
    BYTE *pbDataDecompressed,
    IFMP ifmp,
    PGNO pgno,
    BOOL fDehydrated,
    BOOL fXpressed )
{
    ERR err;

    // First xpress decompression
    if ( fXpressed )
    {
        INT cbActual = 0;
        CallR( ErrPKDecompressData( data, ifmp, pgno, pbDataDecompressed, cbPage, &cbActual ) );
        data.SetPv( pbDataDecompressed );
        data.SetCb( cbActual );
    }

    // Then rehydration
    if ( fDehydrated )
    {
        // If xpress also done, then data is already in decompression buffer
        if ( !fXpressed )
        {
            memcpy( pbDataDecompressed, data.Pv(), data.Cb() );
        }
        else
        {
            Assert( data.Pv() == pbDataDecompressed );
        }
        CPAGE cpageT;
        cpageT.LoadDehydratedPage( ifmp, pgno, pbDataDecompressed, data.Cb(), cbPage );
        cpageT.RehydratePage();
        data.SetPv( pbDataDecompressed );
        Assert( cpageT.CbBuffer() == (ULONG)cbPage );
        data.SetCb( cbPage );
        cpageT.UnloadPage();
    }

    return JET_errSuccess;
}


//  reconstructs split structre during recovery
//      access new page and upgrade to write-latch, if necessary
//      access right page and upgrade to write-latch, if necessary
//      update split members from log record
//
ERR LOG::ErrLGRIRedoInitializeSplit( PIB * const ppib, const LRSPLIT_ * const plrsplit, SPLITPATH * const psplitPath )
{
    ERR             err;
    SPLIT *         psplit          = NULL;
    BOOL            fRedoNewPage    = fFalse;
    BYTE *          pbDataDecompressed = NULL;

    const DBID      dbid            = plrsplit->dbid;
    const PGNO      pgnoSplit       = plrsplit->le_pgno;
    const PGNO      pgnoNew         = plrsplit->le_pgnoNew;
    const PGNO      pgnoRight       = plrsplit->le_pgnoRight;
    const OBJID     objidFDP        = plrsplit->le_objidFDP;
    const DBTIME    dbtime          = plrsplit->le_dbtime;
    const SPLITTYPE splittype       = SPLITTYPE( BYTE( plrsplit->splittype ) );

    const ULONG     fNewPageFlags   = plrsplit->le_fNewPageFlags;
    const ULONG     fSplitPageFlags = plrsplit->le_fSplitPageFlags;

    INST            *pinst          = m_pinst;
    IFMP            ifmp            = pinst->m_mpdbidifmp[ dbid ];

    Assert( pgnoNew != pgnoNull );
    Assert( latchRIW == psplitPath->csr.Latch() || pagetrimTrimmed == psplitPath->csr.PagetrimState() );

    //  allocate split structure
    //
    AllocR( psplit = new SPLIT );

    psplit->pgnoSplit   = pgnoSplit;

    psplit->splittype   = splittype;
    psplit->splitoper   = SPLITOPER( BYTE( plrsplit->splitoper ) );

    psplit->ilineOper   = plrsplit->le_ilineOper;
    psplit->clines      = plrsplit->le_clines;
    Assert( psplit->clines < g_cbPage );

    psplit->ilineSplit  = plrsplit->le_ilineSplit;

    psplit->fNewPageFlags   = fNewPageFlags;
    psplit->fSplitPageFlags = fSplitPageFlags;

    psplit->cbUncFreeSrc    = plrsplit->le_cbUncFreeSrc;
    psplit->cbUncFreeDest   = plrsplit->le_cbUncFreeDest;

    psplit->prefixinfoSplit.ilinePrefix = plrsplit->le_ilinePrefixSplit;
    psplit->prefixinfoNew.ilinePrefix   = plrsplit->le_ilinePrefixNew;

    //  latch the new-page

    psplit->pgnoNew     = plrsplit->le_pgnoNew;
    Assert( g_rgfmp[ifmp].Dbid() == dbid );

    BOOL fRedoSplitPage                     = FLGNeedRedoCheckDbtimeBefore( ifmp, psplitPath->csr, dbtime, plrsplit->le_dbtimeBefore, &err );
    Call( err );
    const BOOL fPageBeforeImageRequired     = FBTISplitPageBeforeImageRequired( psplit );
    const BOOL fHasPageBeforeImage          = ( CbPageBeforeImage( plrsplit ) > 0 );
    Assert( !fHasPageBeforeImage || plrsplit->lrtyp == lrtypSplit2 );

    if ( fPageBeforeImageRequired && !fHasPageBeforeImage )
    {
        FireWall( "MissingPreImgSplit" );
        Call( ErrLGRIReportFlushDependencyCorrupted( ifmp, pgnoNew, pgnoSplit ) );
    }

    Call( ErrLGRIAccessNewPage(
                ppib,
                &psplit->csrNew,
                ifmp,
                pgnoNew,
                objidFDP,
                dbtime,
                &fRedoNewPage ) );

    if ( fRedoNewPage
        && !fRedoSplitPage
        && fPageBeforeImageRequired
        && fHasPageBeforeImage )
    {
        const INT cbOffset = plrsplit->le_cbKeyParent + plrsplit->le_cbPrefixSplitOld + plrsplit->le_cbPrefixSplitNew;
        const BYTE * pbBeforeImage =    PbData( plrsplit ) + cbOffset;
        LONG cbBeforeImage = CbPageBeforeImage( plrsplit );
        if ( cbBeforeImage < g_cbPage )
        {
            LRSPLITNEW *plrsplit2 = (LRSPLITNEW *)plrsplit;
            Assert( plrsplit2->FPreimageDehydrated() || plrsplit2->FPreimageXpress() );
            DATA data;
            data.SetPv( (PVOID)pbBeforeImage );
            data.SetCb( cbBeforeImage );

            Alloc( pbDataDecompressed = PbPKAllocCompressionBuffer() );
            Assert( CbPKCompressionBuffer() == g_cbPage );

            Call( ErrLGIDecompressPreimage( data, g_cbPage, pbDataDecompressed, ifmp, plrsplit->le_pgno, plrsplit2->FPreimageDehydrated(), plrsplit2->FPreimageXpress() ) );
            pbBeforeImage = (const BYTE *)data.Pv();
            cbBeforeImage = data.Cb();
        }

        Assert( g_cbPage == cbBeforeImage );

        TraceReplacePageImage( "split", plrsplit->le_pgno );
        Call( ErrReplacePageImage(
                ppib,
                ifmp,
                psplitPath->csr,
                pbBeforeImage,
                cbBeforeImage,
                plrsplit->le_pgno,
                plrsplit->le_dbtime,
                plrsplit->le_dbtimeBefore ) );
        
        Assert( FAssertLGNeedRedo( ifmp, psplitPath->csr, plrsplit->le_dbtime, plrsplit->le_dbtimeBefore ) );
        fRedoSplitPage = fTrue;
    }
    Assert( !( fRedoNewPage && !fRedoSplitPage && fPageBeforeImageRequired ) );
    
    if ( fRedoNewPage )
    {
        //  create new page
        //
        Assert( !psplit->csrNew.FLatched() );
        Call( psplit->csrNew.ErrGetNewPreInitPageForRedo(
                                    ppib,
                                    ifmp,
                                    pgnoNew,
                                    objidFDP,
                                    dbtime ) );
        psplit->csrNew.ConsumePreInitPage( fNewPageFlags );
    }
    else
    {
        // The Latch state is only valid for pagetrimNormal pages.
        Assert( latchRIW == psplit->csrNew.Latch() || pagetrimNormal != psplit->csrNew.PagetrimState() );
        Assert( !FLGNeedRedoPage( psplit->csrNew, dbtime ) );
    }

    if ( pgnoRight != pgnoNull )
    {
        err = ErrLGIAccessPage( ppib, &psplit->csrRight, ifmp, pgnoRight, objidFDP, fFalse );
        if ( errSkipLogRedoOperation == err )
        {
            Assert( pagetrimTrimmed == psplit->csrRight.PagetrimState() );
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgnoRight ) );
            err = JET_errSuccess;
        }
        else
        {
            Call( err );
            // The Latch state is only valid for pagetrimNormal pages.
            Assert( latchRIW == psplit->csrRight.Latch() || pagetrimNormal != psplit->csrRight.PagetrimState() );
            Assert( dbtimeNil != plrsplit->le_dbtimeRightBefore );
            Assert( dbtimeInvalid != plrsplit->le_dbtimeRightBefore );
            psplit->dbtimeRightBefore = plrsplit->le_dbtimeRightBefore;
        }
    }

    if ( plrsplit->le_cbKeyParent > 0 )
    {
        const INT   cbKeyParent = plrsplit->le_cbKeyParent;

        psplit->kdfParent.key.suffix.SetPv( RESBOOKMARK.PvRESAlloc() );
        Alloc( psplit->kdfParent.key.suffix.Pv() );

        psplit->fAllocParent        = fTrue;
        psplit->kdfParent.key.suffix.SetCb( cbKeyParent );
        UtilMemCpy( psplit->kdfParent.key.suffix.Pv(),
                PbData( plrsplit ),
                cbKeyParent );

        psplit->kdfParent.data.SetCb( sizeof( PGNO ) );
        psplit->kdfParent.data.SetPv( &psplit->pgnoSplit );
    }

    if ( plrsplit->le_cbPrefixSplitOld > 0 )
    {
        const INT   cbPrefix = plrsplit->le_cbPrefixSplitOld;

        psplit->prefixSplitOld.SetPv( RESBOOKMARK.PvRESAlloc() );
        Alloc( psplit->prefixSplitOld.Pv() );

        psplit->prefixSplitOld.SetCb( cbPrefix );

        UtilMemCpy( psplit->prefixSplitOld.Pv(),
                PbData( plrsplit ) + plrsplit->le_cbKeyParent,
                cbPrefix );
    }

    if ( plrsplit->le_cbPrefixSplitNew > 0 )
    {
        const INT   cbPrefix = plrsplit->le_cbPrefixSplitNew;

        psplit->prefixSplitNew.SetPv( RESBOOKMARK.PvRESAlloc() );
        Alloc( psplit->prefixSplitNew.Pv() );

        psplit->prefixSplitNew.SetCb( cbPrefix );

        UtilMemCpy( psplit->prefixSplitNew.Pv(),
                    PbData( plrsplit )+ plrsplit->le_cbKeyParent + plrsplit->le_cbPrefixSplitOld,
                    cbPrefix );
    }

    Assert( psplit->csrNew.Pgno() != pgnoNull );
    Assert( plrsplit->le_pgnoNew == psplit->csrNew.Pgno() );
    Assert( plrsplit->le_pgnoRight == psplit->csrRight.Pgno() );

    //  link psplit to psplitPath
    //
    psplitPath->psplit = psplit;
    psplit->psplitPath = psplitPath;

    if ( fRedoNewPage )
    {
        Assert( latchWrite == psplit->csrNew.Latch() );
    }
    else
    {
        // The Latch state is only valid for pagetrimNormal pages.
        Assert( latchRIW == psplit->csrNew.Latch() || pagetrimNormal != psplit->csrNew.PagetrimState() );
    }

HandleError:
    AssertSz( errSkipLogRedoOperation != err, "%s() unexpectedly got errSkipLogRedoOperation from ErrLGIAccessPage(%d=%#x or %d=%#x or %d=%#x).\n",
              __FUNCTION__, pgnoSplit, pgnoSplit, pgnoNew, pgnoNew, pgnoRight, pgnoRight );

    if ( ( err < 0 ) && ( NULL != psplit ) )
    {
        delete psplit;
    }
    PKFreeCompressionBuffer( pbDataDecompressed );
    return err;
}


//  reconstructs splitPath and split
//
ERR LOG::ErrLGRIRedoSplitPath( PIB *ppib, const LRSPLIT_ *plrsplit, SPLITPATH **ppsplitPath )
{
    Assert( lrtypSplit == plrsplit->lrtyp || lrtypSplit2 == plrsplit->lrtyp );
    ERR             err;
    const DBID      dbid        = plrsplit->dbid;
    const PGNO      pgnoSplit   = plrsplit->le_pgno;
    const PGNO      pgnoNew     = plrsplit->le_pgnoNew;
    const OBJID     objidFDP    = plrsplit->le_objidFDP;
    
    INST            *pinst = m_pinst;
    IFMP            ifmp = pinst->m_mpdbidifmp[ dbid ];

    //  allocate new splitPath
    //
    CallR( ErrBTINewSplitPath( ppsplitPath ) );

    SPLITPATH *psplitPath = *ppsplitPath;

    err = ErrLGIAccessPage( ppib, &psplitPath->csr, ifmp, pgnoSplit, objidFDP, fFalse );

    if ( errSkipLogRedoOperation == err )
    {
        Assert( pagetrimTrimmed == psplitPath->csr.PagetrimState() );
        Assert( FRedoMapNeeded( ifmp ) );
        Assert( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( pgnoSplit ) );
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
        Assert( latchRIW == psplitPath->csr.Latch() );
    }

    //  allocate new split if needed
    //
    if ( pgnoNew != pgnoNull )
    {
        Call( ErrLGRIRedoInitializeSplit( ppib, plrsplit, psplitPath ) );
        Assert( NULL != psplitPath->psplit );
    }

    psplitPath->dbtimeBefore = plrsplit->le_dbtimeBefore;
    if ( psplitPath->psplitPathParent != NULL )
    {
        Assert( psplitPath == psplitPath->psplitPathParent->psplitPathChild );
        psplitPath->psplitPathParent->dbtimeBefore = plrsplit->le_dbtimeParentBefore;
    }

HandleError:
    AssertSz( errSkipLogRedoOperation != err, "%s() got errSkipLogRedoOperation from ErrLGIAccessPage(dbtime=%d=%#x, %d=%#x).\n",
              __FUNCTION__, (DBTIME) plrsplit->le_dbtime, (DBTIME) plrsplit->le_dbtime, pgnoSplit, pgnoSplit );

    return err;
}

//  allocate and initialize mergePath structure
//  access merged page
//  if redo is needed,
//      upgrade latch
//
ERR LOG::ErrLGIRedoMergePath( PIB               *ppib,
                               const LRMERGE_   * const plrmerge,
                               _Outptr_ MERGEPATH       **ppmergePath )
{
    ERR             err;
    const DBID      dbid        = plrmerge->dbid;
    const PGNO      pgno        = plrmerge->le_pgno;
    const OBJID     objidFDP    = plrmerge->le_objidFDP;
    INST            *pinst      = PinstFromPpib( ppib );
    IFMP            ifmp        = pinst->m_mpdbidifmp[ dbid ];

    //  initialize merge path
    //
    CallR( ErrBTINewMergePath( ppmergePath ) );

    MERGEPATH *pmergePath = *ppmergePath;

    err = ErrLGIAccessPage( ppib, &pmergePath->csr, ifmp, pgno, objidFDP, fFalse );

    if ( errSkipLogRedoOperation == err )
    {
        err = JET_errSuccess;
        Assert( pagetrimTrimmed == pmergePath->csr.PagetrimState() );
    }
    else
    {
        Call( err );
        Assert( latchRIW == pmergePath->csr.Latch() );
    }

    pmergePath->iLine       = plrmerge->ILine();
    pmergePath->fKeyChange  = ( plrmerge->FKeyChange() ? fTrue : fFalse );
    pmergePath->fDeleteNode = ( plrmerge->FDeleteNode() ? fTrue : fFalse );
    pmergePath->fEmptyPage  = ( plrmerge->FEmptyPage() ? fTrue : fFalse );

    pmergePath->dbtimeBefore = plrmerge->le_dbtimeBefore;
    if ( pmergePath->pmergePathParent != NULL )
    {
        Assert( pmergePath == pmergePath->pmergePathParent->pmergePathChild );
        pmergePath->pmergePathParent->dbtimeBefore = plrmerge->le_dbtimeParentBefore;
    }

HandleError:
    return err;
}


//  allocates and initializes leaf-level merge structure
//  access sibling pages
//  if redo is needed,
//      upgrade to write-latch
//
ERR LOG::ErrLGRIRedoInitializeMerge( PIB            *ppib,
                                     FUCB           *pfucb,
                                     const LRMERGE_ *plrmerge,
                                     MERGEPATH      *pmergePath )
{
    ERR             err;
    BYTE *          pbDataDecompressed = NULL;

    Assert( NULL == pmergePath->pmergePathChild );
    CallR( ErrBTINewMerge( pmergePath ) );

    MERGE           *const pmerge       = pmergePath->pmerge;
    const PGNO      pgnoRight           = plrmerge->le_pgnoRight;
    const PGNO      pgnoLeft            = plrmerge->le_pgnoLeft;
    const DBID      dbid                = plrmerge->dbid;
    const DBTIME    dbtime              = plrmerge->le_dbtime;
    const MERGETYPE mergetype           = MERGETYPE( BYTE( plrmerge->mergetype ) );

    const INST      * const pinst       = PinstFromPpib( ppib );
    const IFMP      ifmp                = pinst->m_mpdbidifmp[ dbid ];

    const BOOL      fHasPageBeforeImage = ( CbPageBeforeImage( plrmerge ) > 0 );
    BOOL            fRedoSourcePage     = FLGNeedRedoCheckDbtimeBefore( ifmp, pmergePath->csr, dbtime, plrmerge->le_dbtimeBefore, &err );
    Call( err );

    BOOL            fRedoLeftPage       = ( pgnoLeft != pgnoNull );
    BOOL            fRedoRightPage      = ( pgnoRight != pgnoNull );

    // Need to set this so that FBTIMergePageBeforeImageRequired can be called
    pmerge->mergetype       = mergetype;

    if ( fRedoLeftPage )
    {
        //  unlike split, the left page should already exist (unless it's been trimmed), so we
        //  shouldn't get errors back from AccessPage()
        err = ErrLGIAccessPage( ppib, &pmerge->csrLeft, ifmp, pgnoLeft, plrmerge->le_objidFDP, fFalse );

        if ( errSkipLogRedoOperation == err )
        {
            Assert( pagetrimTrimmed == pmerge->csrLeft.PagetrimState() );
            fRedoLeftPage = fFalse;
            err = JET_errSuccess;
        }
        else
        {
            Call( err );
            Assert( latchRIW == pmerge->csrLeft.Latch() );
            Assert( dbtimeNil != plrmerge->le_dbtimeLeftBefore );
            Assert( dbtimeInvalid != plrmerge->le_dbtimeLeftBefore );
            pmerge->dbtimeLeftBefore = plrmerge->le_dbtimeLeftBefore;
        }
    }

    if ( fRedoRightPage )
    {
        //  unlike split, the right page should already exist (unless it's been trimmed), so we
        //  shouldn't get errors back from AccessPage()
        err = ErrLGIAccessPage( ppib, &pmerge->csrRight, ifmp, pgnoRight, plrmerge->le_objidFDP, fFalse );
        if ( errSkipLogRedoOperation == err )
        {
            Assert( pagetrimTrimmed == pmerge->csrRight.PagetrimState() );
            fRedoRightPage = fFalse;
            err = JET_errSuccess;
        }
        else
        {
            Call( err );
            Assert( latchRIW == pmerge->csrRight.Latch() );
            Assert( dbtimeNil != plrmerge->le_dbtimeRightBefore );
            Assert( dbtimeInvalid != plrmerge->le_dbtimeRightBefore );
            pmerge->dbtimeRightBefore = plrmerge->le_dbtimeRightBefore;
        }
    }

    CSR * pcsrDest = pcsrNil;
    DBTIME dbtimeDestBefore = dbtimeInvalid;
    switch( mergetype )
    {
        case mergetypePartialLeft:
        case mergetypeFullLeft:
            if ( fRedoLeftPage )
            {
                pcsrDest = &(pmerge->csrLeft);
                dbtimeDestBefore = pmerge->dbtimeLeftBefore;
            }
            else
            {
                Assert( pcsrNil == pcsrDest );
            }
            break;

        case mergetypePartialRight:
        case mergetypeFullRight:
            if ( fRedoRightPage )
            {
                pcsrDest = &(pmerge->csrRight);
                dbtimeDestBefore = pmerge->dbtimeRightBefore;
            }
            else
            {
                Assert( pcsrNil == pcsrDest );
            }
            break;

        default:
            Assert( pcsrNil == pcsrDest );
            break;
    }

    BOOL fRedoDestPage = fFalse;
    if( pcsrDest )
    {
        Assert( latchRIW == pcsrDest->Latch() );
        fRedoDestPage = FLGNeedRedoCheckDbtimeBefore( ifmp, *pcsrDest, dbtime, dbtimeDestBefore, &err );
        Call( err );
    }

    if ( fRedoDestPage )
    {
        Assert( FAssertLGNeedRedo( ifmp, *pcsrDest, dbtime, dbtimeDestBefore ) );
        Assert( mergetype == pmerge->mergetype );

        if ( FBTIMergePageBeforeImageRequired( pmerge ) )
        {
            if ( !fRedoSourcePage )
            {
                if( fHasPageBeforeImage )
                {
                    const INT cbOffset = plrmerge->le_cbKeyParentSep;
                    const BYTE * pbBeforeImage =    PbData( plrmerge ) + cbOffset;
                    LONG cbBeforeImage = CbPageBeforeImage( plrmerge );
                    if ( cbBeforeImage < g_cbPage )
                    {
                        LRMERGENEW *plrmerge2 = (LRMERGENEW *)plrmerge;
                        Assert( plrmerge2->FPreimageDehydrated() || plrmerge2->FPreimageXpress() );
                        DATA data;
                        data.SetPv( (PVOID)pbBeforeImage );
                        data.SetCb( cbBeforeImage );

                        Alloc( pbDataDecompressed = PbPKAllocCompressionBuffer() );
                        Assert( CbPKCompressionBuffer() == g_cbPage );

                        Call( ErrLGIDecompressPreimage( data, g_cbPage, pbDataDecompressed, ifmp, plrmerge->le_pgno, plrmerge2->FPreimageDehydrated(), plrmerge2->FPreimageXpress() ) );
                        pbBeforeImage = (const BYTE *)data.Pv();
                        cbBeforeImage = data.Cb();
                    }
                    Assert( g_cbPage == cbBeforeImage );

                    TraceReplacePageImage( "merge", plrmerge->le_pgno );
                    Call( ErrReplacePageImage(
                        ppib,
                        ifmp,
                        pmergePath->csr,
                        pbBeforeImage,
                        cbBeforeImage,
                        plrmerge->le_pgno,
                        plrmerge->le_dbtime,
                        plrmerge->le_dbtimeBefore ) );

                    Assert( FAssertLGNeedRedo( ifmp, pmergePath->csr, plrmerge->le_dbtime, plrmerge->le_dbtimeBefore ) );
                    fRedoSourcePage = fTrue;
                }
                else
                {
                    Assert( pmergePath->csr.Pgno() == plrmerge->le_pgno );
                    Error( ErrLGRIReportFlushDependencyCorrupted(
                            ifmp,
                            FRightMerge( mergetype ) ? pgnoRight : pgnoLeft,
                            pmergePath->csr.Pgno() ) );
                }
            }

            Assert( fRedoSourcePage );
            Assert( pmergePath->csr.Latch() == latchRIW );

            Assert( pcsrDest->Latch() == latchRIW || pagetrimNormal == pcsrDest->PagetrimState() );

            if( !fHasPageBeforeImage )
            {
                FireWall( "MissingPreImgMerge" );
                Call( ErrLGRIReportFlushDependencyCorrupted( ifmp, pgnoRight, pgnoLeft ) );
            }
        }
    }

    Assert( mergetype == pmerge->mergetype );

    if ( ( pcsrDest == NULL )
         || ( pagetrimNormal == pmergePath->csr.PagetrimState() && pagetrimNormal == pcsrDest->PagetrimState() ) )
    {
        // Only check this condition if neither of the pages was Trimmed out.
        Assert( !( fRedoDestPage && !fRedoSourcePage && FBTIMergePageBeforeImageRequired( pmerge ) ) );
    }

    if ( plrmerge->le_cbKeyParentSep > 0 )
    {
        const INT   cbKeyParentSep = plrmerge->le_cbKeyParentSep;

        pmerge->kdfParentSep.key.suffix.SetPv( RESBOOKMARK.PvRESAlloc() );
        Alloc( pmerge->kdfParentSep.key.suffix.Pv() );

        pmerge->fAllocParentSep     = fTrue;
        pmerge->kdfParentSep.key.suffix.SetCb( cbKeyParentSep );
        UtilMemCpy( pmerge->kdfParentSep.key.suffix.Pv(),
                PbData( plrmerge ),
                cbKeyParentSep );
    }

    Assert( 0 == plrmerge->ILineMerge()
            || mergetypePartialRight == mergetype
            || mergetypePartialLeft == mergetype
            || mergetypeFullLeft == mergetype );

    pmerge->ilineMerge      = plrmerge->ILineMerge();
    pmerge->cbSizeTotal     = plrmerge->le_cbSizeTotal;
    pmerge->cbSizeMaxTotal  = plrmerge->le_cbSizeMaxTotal;
    pmerge->cbUncFreeDest   = plrmerge->le_cbUncFreeDest;

    //  if merged page needs redo
    //      allocate and initialize rglineinfo
    //
    if ( fRedoSourcePage )
    {
        CSR * const pcsr = &pmergePath->csr;
        Assert( latchRIW == pcsr->Latch() );

        const INT   clines  = pmergePath->csr.Cpage().Clines();
        pmerge->clines      = clines;

        Assert( pmerge->rglineinfo == NULL );
        Alloc( pmerge->rglineinfo = new LINEINFO[clines] );

        KEY     keyPrefix;
        keyPrefix.Nullify();
        if ( fRedoDestPage )
        {
            Assert( FAssertLGNeedRedo( ifmp, *pcsrDest, dbtime, dbtimeDestBefore ) );

            NDGetPrefix( pfucb, pcsrDest );
            keyPrefix = pfucb->kdfCurr.key;
            Assert( pfucb->kdfCurr.data.FNull() );
        }

        memset( pmerge->rglineinfo, 0, sizeof( LINEINFO ) * clines );

        for ( INT iline = 0; iline < clines; iline++ )
        {
            LINEINFO * const plineinfo = pmerge->rglineinfo + iline;
            pcsr->SetILine( iline );
            Call( ErrNDGet( pfucb, pcsr ) );
            plineinfo->kdf = pfucb->kdfCurr;

            //  cache node
            //
            BOOKMARK    bm;
            NDGetBookmarkFromKDF( pfucb, plineinfo->kdf, &bm );

            // BTIMergeMoveNodes will delete non-visible flag-deleted nodes, but some of those nodes
            // may still be visible on passives because of read-only transactions.
            // We cannot change BTIMergeMoveNodes to move the node on the passive based on visibility,
            // so we have to stall the move until the nodes are no longer visible to any transaction.
            BOOL fEventloggedLongWait = fFalse;
            TICK tickStart = TickOSTimeCurrent();
            TICK tickEnd;
            while ( fTrue )
            {
                if ( !FNDDeleted( plineinfo->kdf ) || !FVERActive( pfucb, bm ) )
                {
                    break;
                }

                // Ok to sleep with latches held since we only have RIW latch and not Write latch at this point,
                // and read-only transactions only need read latch
                m_pinst->m_sigTrxOldestDuringRecovery.FWait( cmsecMaxReplayDelayDueToReadTrx );

                tickEnd = TickOSTimeCurrent();
                if ( !fEventloggedLongWait &&
                     tickEnd - tickStart >= cmsecMaxReplayDelayDueToReadTrx )
                {
                    PERFOpt( cLGRecoveryLongStallReadOnly.Inc( m_pinst ) );

                    WCHAR szTime[16];
                    const WCHAR *rgsz[1] = { szTime };
                    OSStrCbFormatW( szTime, sizeof(szTime), L"%d", tickEnd - tickStart );
                    UtilReportEvent(
                            eventWarning,
                            LOGGING_RECOVERY_CATEGORY,
                            RECOVERY_LONG_WAIT_READ_ONLY,
                            1,
                            rgsz,
                            0,
                            NULL,
                            m_pinst );
                    fEventloggedLongWait = fTrue;
                }
            }

            tickEnd = TickOSTimeCurrent();
            if ( !fEventloggedLongWait &&
                 tickEnd - tickStart >= cmsecMaxReplayDelayDueToReadTrx / 10 )
            {
                PERFOpt( cLGRecoveryStallReadOnly.Inc( m_pinst ) );

                WCHAR szTime[16];
                const WCHAR *rgsz[1] = { szTime };
                OSStrCbFormatW( szTime, sizeof(szTime), L"%d", tickEnd - tickStart );
                UtilReportEvent(
                        eventInformation,
                        LOGGING_RECOVERY_CATEGORY,
                        RECOVERY_WAIT_READ_ONLY,
                        1,
                        rgsz,
                        0,
                        NULL,
                        m_pinst,
                        JET_EventLoggingLevelMedium );
            }
            if ( tickEnd != tickStart )
            {
                PERFOpt( cLGRecoveryStallReadOnlyTime.Add( m_pinst, tickEnd - tickStart ) );
            }

            if ( fRedoDestPage )
            {
                Assert( FAssertLGNeedRedo( ifmp, *pcsrDest, dbtime, dbtimeDestBefore ) );

                //  calculate cbPrefix for node
                //  with respect to prefix on right page
                //
                const INT cbCommon = CbCommonKey( pfucb->kdfCurr.key, keyPrefix );

                Assert( 0 == plineinfo->cbPrefix );
                if ( cbCommon > cbPrefixOverhead )
                {
                    plineinfo->cbPrefix = cbCommon;
                }
            }
        }
    }

HandleError:
    AssertSz( errSkipLogRedoOperation != err, "%s() got errSkipLogRedoOperation from ErrLGIAccessPage(%d=%#x or %d=%#x).\n",
                __FUNCTION__, pgnoRight, pgnoRight, pgnoLeft, pgnoLeft );

    PKFreeCompressionBuffer( pbDataDecompressed );
    return err;
}


//  reconstructs merge structures and fucb
//
ERR LOG::ErrLGRIRedoMergeStructures( PIB        *ppib,
                                     DBTIME     dbtime,
                                     MERGEPATH  **ppmergePathLeaf,
                                     FUCB       **ppfucb )
{
    ERR             err;
    const LRMERGE_  * plrmerge  = (LRMERGE_ *) ppib->PvLogrec( dbtime );
    const LRMERGE_ * const plrmergeBase     = plrmerge;
    const DBID      dbid        = plrmerge->dbid;
    const PGNO      pgnoFDP     = plrmerge->le_pgnoFDP;
    const OBJID     objidFDP    = plrmerge->le_objidFDP;    // Debug only info
    const BOOL      fUnique     = plrmerge->FUnique();
    const BOOL      fSpace      = plrmerge->FSpace();

    INST            *pinst = PinstFromPpib( ppib );
    IFMP            ifmp = pinst->m_mpdbidifmp[ dbid ];

    for ( UINT ibNextLR = 0;
          ibNextLR < ppib->CbSizeLogrec( dbtime );
          ibNextLR += CbLGSizeOfRec( plrmerge ) )
    {
        plrmerge = (LRMERGE_ *) ( (BYTE *) plrmergeBase + ibNextLR );
        Assert( lrtypMerge == plrmerge->lrtyp || lrtypMerge2 == plrmerge->lrtyp );

        //  insert and initialize mergePath for this level
        //
        err = ErrLGIRedoMergePath( ppib, plrmerge, ppmergePathLeaf );

        // Skipping the whole Merge because of just a single Trimmed page is a bad idea.
        AssertSz( errSkipLogRedoOperation != err, "We should not get errSkipLogRedoOperation at this point." );

        Call( err );
    }

    //  get fucb
    //
    Assert( pfucbNil == *ppfucb );
    Call( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, ppfucb ) );

    //  initialize rglineinfo for leaf level of merge
    //
    Assert( NULL != *ppmergePathLeaf );
    Assert( NULL == (*ppmergePathLeaf)->pmergePathChild );

    Call( ErrLGRIRedoInitializeMerge( ppib, *ppfucb, plrmerge, *ppmergePathLeaf ) );

HandleError:
    return err;
}


//  reconstructs split structures, FUCB, dirflag and kdf for split
//
ERR LOG::ErrLGIRedoSplitStructures(
    PIB             *ppib,
    DBTIME          dbtime,
    SPLITPATH       **ppsplitPathLeaf,
    FUCB            **ppfucb,
    DIRFLAG         *pdirflag,
    KEYDATAFLAGS    *pkdf,
    RCEID           *prceidOper1,
    RCEID           *prceidOper2 )
{
    ERR             err;
    LR              *plr;
    SPLITPATH       *psplitPath;
    const LRSPLIT_  *plrsplit   = (LRSPLIT_ *) ppib->PvLogrec( dbtime );
    const DBID      dbid        = plrsplit->dbid;
    const PGNO      pgnoFDP     = plrsplit->le_pgnoFDP;
    const OBJID     objidFDP    = plrsplit->le_objidFDP;
    IFMP            ifmp;

    Assert( dbtime  == plrsplit->le_dbtime );

    //  split with no oper will use the space fucb
    //
    BOOL            fUnique         = plrsplit->FUnique();
    BOOL            fSpace          = fTrue;

    Assert( rceidNull == *prceidOper1 );
    Assert( rceidNull == *prceidOper2 );

    pkdf->Nullify();
    Assert( fDIRNull == *pdirflag );

    for ( UINT ibNextLR = 0;
          ibNextLR < ppib->CbSizeLogrec( dbtime );
          ibNextLR += CbLGSizeOfRec( plr ) )
    {
        plr = (LR *) ( (BYTE *)ppib->PvLogrec( dbtime ) + ibNextLR );
        switch( plr->lrtyp )
        {
            case lrtypSplit:
            case lrtypSplit2:
            {
                const LRSPLIT_  *plrsplitT = (LRSPLIT_ *) plr;

                Assert( dbtime == plrsplitT->le_dbtime );
                Assert( pgnoFDP == plrsplitT->le_pgnoFDP );
                err = ErrLGRIRedoSplitPath( ppib, plrsplitT, ppsplitPathLeaf );

                // Skipping the whole Split because of just a single Trimmed page is a bad idea.
                AssertSz( errSkipLogRedoOperation != err, "We should not get errSkipLogRedoOperation at this point." );

                Call( err );
            }
                break;

            case lrtypInsert:
            {
                LRINSERT    *plrinsert = (LRINSERT *) plr;

                pkdf->key.suffix.SetPv( (BYTE *) plrinsert + sizeof( LRINSERT ) );
                pkdf->key.suffix.SetCb( plrinsert->CbPrefix() + plrinsert->CbSuffix() );

                pkdf->data.SetPv( (BYTE *) plrinsert +
                                  sizeof( LRINSERT ) +
                                  plrinsert->CbPrefix() +
                                  plrinsert->CbSuffix() );
                pkdf->data.SetCb( plrinsert->CbData() );
                *pdirflag |= fDIRInsert;

                *prceidOper1 = plrinsert->le_rceid;
            }
                break;

            case lrtypFlagInsertAndReplaceData:
            {
                LRFLAGINSERTANDREPLACEDATA  *plrfiard = (LRFLAGINSERTANDREPLACEDATA *) plr;

                pkdf->key.prefix.Nullify();
                pkdf->key.suffix.SetCb( plrfiard->CbKey() );
                pkdf->key.suffix.SetPv( plrfiard->rgbData );
                pkdf->data.SetPv( (BYTE *) plrfiard +
                                  sizeof( LRFLAGINSERTANDREPLACEDATA ) +
                                  plrfiard->CbKey() );
                pkdf->data.SetCb( plrfiard->CbData() );

                *pdirflag       |= fDIRFlagInsertAndReplaceData;
                *prceidOper1    = plrfiard->le_rceid;
                *prceidOper2    = plrfiard->le_rceidReplace;
            }
                break;

            case lrtypReplace:
            {
                LRREPLACE   *plrreplace = (LRREPLACE *) plr;

                pkdf->data.SetPv( (BYTE *) plrreplace + sizeof ( LRREPLACE ) );
                pkdf->data.SetCb( plrreplace->CbNewData() );

                *pdirflag       |= fDIRReplace;
                *prceidOper1    = plrreplace->le_rceid;
            }
                break;

            default:
                Assert( fFalse );
                break;
        }

        if ( lrtypSplit != plr->lrtyp && lrtypSplit2 != plr->lrtyp )
        {
            //  get fUnique and dirflag
            //
            const LRNODE_   *plrnode    = (LRNODE_ *) plr;

            Assert( plrnode->le_pgnoFDP == pgnoFDP );
            Assert( plrnode->dbid == dbid );
            Assert( plrnode->le_dbtime == dbtime );

            fUnique     = plrnode->FUnique();
            fSpace      = plrnode->FSpace();

            if ( !plrnode->FVersioned() )
                *pdirflag |= fDIRNoVersion;
        }
    }

    ifmp = PinstFromPpib( ppib )->m_mpdbidifmp[ dbid ];

    //  get fucb
    //
    Assert( pfucbNil == *ppfucb );
    Call( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, ppfucb ) );

    //  initialize rglineinfo for every level of split
    //
    for ( psplitPath = *ppsplitPathLeaf;
          psplitPath != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
        Assert( latchRIW == psplitPath->csr.Latch() || pagetrimTrimmed == psplitPath->csr.PagetrimState() );

        err = JET_errSuccess;

        if ( psplitPath->psplit != NULL
            && ( FLGNeedRedoCheckDbtimeBefore( ifmp, psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err )
                || FLGNeedRedoPage( psplitPath->psplit->csrNew, dbtime ) ) )
        {
            Call( err );
            const BOOL fHasPageBeforeImage = ( CbPageBeforeImage( plrsplit ) > 0 );

#ifdef DEBUG
            ERR         errT;

            SPLIT*      psplit          = psplitPath->psplit;
            Assert( NULL != psplit );

            const BOOL  fRedoSplitPage  = FLGNeedRedoCheckDbtimeBefore(
                                                ifmp,
                                                psplitPath->csr,
                                                dbtime,
                                                psplitPath->dbtimeBefore,
                                                &errT );
            const BOOL  fRedoNewPage    = FLGNeedRedoPage( psplit->csrNew, dbtime );

            CallS( errT );

            if ( !fRedoSplitPage )
            {
                Assert( fRedoNewPage );
                Assert( !FBTISplitPageBeforeImageRequired( psplit ) || fHasPageBeforeImage );
            }
#endif

            //  if split page needs redo
            //      allocate and set lineinfo for split page
            //
            Call( ErrLGIRedoSplitLineinfo(
                        *ppfucb,
                        psplitPath,
                        dbtime,
                        ( psplitPath == *ppsplitPathLeaf ?
                                *pkdf :
                                psplitPath->psplitPathChild->psplit->kdfParent ),
                        fHasPageBeforeImage ) );
        }
    }

HandleError:
    return err;
}


//  updates dbtime to given value on all write-latched pages
//
LOCAL VOID LGIRedoMergeUpdateDbtime( MERGEPATH *pmergePathTip, DBTIME dbtime )
{
    MERGEPATH   *pmergePath = pmergePathTip;

    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
    {
        LGRIRedoDirtyAndSetDbtime( &pmergePath->csr, dbtime );

        MERGE   *pmerge = pmergePath->pmerge;
        if ( pmerge != NULL )
        {
            LGRIRedoDirtyAndSetDbtime( &pmerge->csrLeft, dbtime );
            LGRIRedoDirtyAndSetDbtime( &pmerge->csrRight, dbtime );
            LGRIRedoDirtyAndSetDbtime( &pmerge->csrNew, dbtime );
        }
    }

    return;
}


//  updates dbtime to given value on all write-latched pages
//
LOCAL VOID LGIRedoSplitUpdateDbtime( SPLITPATH *psplitPathLeaf, DBTIME dbtime )
{
    SPLITPATH   *psplitPath = psplitPathLeaf;

    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathParent )
    {
        LGRIRedoDirtyAndSetDbtime( &psplitPath->csr, dbtime );

        SPLIT   *psplit = psplitPath->psplit;
        if ( psplit != NULL )
        {
            LGRIRedoDirtyAndSetDbtime( &psplit->csrNew, dbtime );
            LGRIRedoDirtyAndSetDbtime( &psplit->csrRight, dbtime );
        }
    }

    return;
}


//  creates version for operation performed atomically with split
//      also links version into appropriate lists
//
ERR ErrLGIRedoSplitCreateVersion(
    SPLIT               *psplit,
    FUCB                *pfucb,
    const KEYDATAFLAGS& kdf,
    const DIRFLAG       dirflag,
    const RCEID         rceidOper1,
    const RCEID         rceidOper2,
    const LEVEL         level )
{
    ERR                 err = JET_errSuccess;


    Assert( splitoperInsert == psplit->splitoper
        || splitoperReplace == psplit->splitoper
        || splitoperFlagInsertAndReplaceData == psplit->splitoper );

    Assert( pfucb->ppib->FAfterFirstBT() );

    Assert( rceidOper1 != rceidNull );
    Assert( splitoperFlagInsertAndReplaceData != psplit->splitoper
        || rceidOper2 != rceidNull );
    Assert( psplit->fNewPageFlags & CPAGE::fPageLeaf );

    RCE         *prceOper1  = prceNil;
    RCE         *prceOper2  = prceNil;
    const BOOL  fNeedRedo   = ( psplit->ilineOper < psplit->ilineSplit ?
                                        latchWrite == psplit->psplitPath->csr.Latch() :
                                        latchWrite == psplit->csrNew.Latch() );

    VERPROXY    verproxy;

    verproxy.rceid = rceidOper1;
    verproxy.level = level;
    verproxy.proxy = proxyRedo;

    if ( splitoperReplace == psplit->splitoper )
    {
        //  create version only if page with oper needs redo
        //
        if ( !fNeedRedo )
        {
            Assert( pfucb->bmCurr.key.FNull() );
            goto HandleError;
        }

        Assert( latchWrite == psplit->psplitPath->csr.Latch() );
        Assert( dirflag & fDIRReplace );
        Assert( FFUCBUnique( pfucb ) );
        Assert( !pfucb->bmCurr.key.FNull() );

        Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, pfucb->bmCurr, operReplace, &prceOper1, &verproxy ) );
    }
    else
    {
        Assert( splitoperFlagInsertAndReplaceData == psplit->splitoper ||
                splitoperInsert == psplit->splitoper );
        Assert( ( dirflag & fDIRInsert ) ||
                ( dirflag & fDIRFlagInsertAndReplaceData ) );

        //  create version for insert even if oper needs no redo
        //
        BOOKMARK    bm;
        NDGetBookmarkFromKDF( pfucb, kdf, &bm );
        Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, bm, operInsert, &prceOper1, &verproxy ) );

        //  create version for replace if oper needs redo
        //
        if ( splitoperFlagInsertAndReplaceData == psplit->splitoper
            && fNeedRedo )
        {
            Assert( dirflag & fDIRFlagInsertAndReplaceData );
            verproxy.rceid = rceidOper2;
            BTISplitGetReplacedNode( pfucb, psplit );
            Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, bm, operReplace, &prceOper2, &verproxy ) );
        }
    }

    //  link RCE(s) to lists
    //
    Assert( prceNil != prceOper1 );
    Assert( splitoperFlagInsertAndReplaceData == psplit->splitoper &&
                ( prceNil != prceOper2 || !fNeedRedo ) ||
            splitoperFlagInsertAndReplaceData != psplit->splitoper &&
                prceNil == prceOper2 );
    BTISplitInsertIntoRCELists( pfucb,
                                psplit->psplitPath,
                                &kdf,
                                prceOper1,
                                prceOper2,
                                &verproxy );

HandleError:
    return err;
}


//  upgrades latches on pages that need redo
//
LOCAL ERR ErrLGIRedoMergeUpgradeLatches( const IFMP ifmp, MERGEPATH *pmergePathLeaf, DBTIME dbtime )
{
    ERR         err         = JET_errSuccess;
    MERGEPATH*  pmergePath;

    //  go to root
    //  since we need to latch top-down
    //
    for ( pmergePath = pmergePathLeaf;
          pmergePath->pmergePathParent != NULL;
          pmergePath = pmergePath->pmergePathParent )
    {
    }

    Assert( NULL == pmergePath->pmergePathParent );
    for ( ; pmergePath != NULL; pmergePath = pmergePath->pmergePathChild )
    {
        MERGE * const pmerge = pmergePath->pmerge;
        if ( pmerge != NULL )
        {
            Assert( pmergePath == pmergePathLeaf );

            if ( pagetrimNormal == pmerge->csrLeft.PagetrimState() )
            {
                Assert( ( pgnoNull == pmerge->csrLeft.Pgno()
                          && !pmerge->csrLeft.FLatched() )
                        || latchRIW == pmerge->csrLeft.Latch() );
                if ( pmerge->csrLeft.FLatched() && FLGNeedRedoCheckDbtimeBefore( ifmp, pmerge->csrLeft, dbtime, pmerge->dbtimeLeftBefore, &err ) )
                {
                    Call( err );
                    pmerge->csrLeft.UpgradeFromRIWLatch();
                }
            }

            // New pages are only consumed during page moves, not merge operations.
            Assert( pmerge->pgnoNew == pgnoNull );
            Assert( !pmerge->csrNew.FLatched() );
            CallS( err );
        }

        if ( pagetrimNormal == pmergePath->csr.PagetrimState() )
        {
            Assert( pmergePath->csr.FLatched() );
            Assert( latchRIW == pmergePath->csr.Latch() );
        }

        if ( FLGNeedRedoCheckDbtimeBefore( ifmp, pmergePath->csr, dbtime, pmergePath->dbtimeBefore, &err ) )
        {
            Call( err );
            pmergePath->csr.UpgradeFromRIWLatch();
        }
        CallS( err );

        if ( pmerge != NULL )
        {
            if ( pagetrimNormal == pmerge->csrRight.PagetrimState() )
            {
                Assert( ( pgnoNull == pmerge->csrRight.Pgno()
                          && !pmerge->csrRight.FLatched() )
                        || latchRIW == pmerge->csrRight.Latch() );

                if ( pmerge->csrRight.FLatched() && FLGNeedRedoCheckDbtimeBefore( ifmp, pmerge->csrRight, dbtime, pmerge->dbtimeRightBefore, &err ) )
                {
                    Call( err );
                    pmerge->csrRight.UpgradeFromRIWLatch();
                }
                CallS( err );
            }
        }
    }
HandleError:
    return err;
}

//  Clears the redo map storing the dbtimerevert pages for the empty merged pages which were skipped from redo operations earlier in the required range.
//
LOCAL VOID LGIRedoMergeClearRedoMapDbtimeRevert( const MERGEPATH* const pmergePathTip, const IFMP ifmp )
{
    // There is no redomap dbtimerevert. So nothing needs to be cleared.
    if ( !g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert() )
    {
        return;
    }

    for ( const MERGEPATH *pmergePath = pmergePathTip; pmergePath != NULL; pmergePath = pmergePath->pmergePathParent )
    {
        Assert( pmergePath->csr.Pgno() != pgnoNull );

        if ( pmergePath->fEmptyPage )
        {
            g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert()->ClearPgno( pmergePath->csr.Pgno() );
        }
    }
}

//  ================================================================
ERR LOG::ErrLGRIRedoNewPage( const LRNEWPAGE * const plrnewpage )
//  ================================================================
{
    ERR err = JET_errSuccess;
    PIB *ppib = ppibNil;
    BOOL fSkip = fFalse;
    const DBID dbid = plrnewpage->Dbid();
    const DBTIME dbtime = plrnewpage->DbtimeAfter();
    const OBJID objid = plrnewpage->Objid();

    CallR( ErrLGRICheckRedoConditionInTrx(
            plrnewpage->Procid(),
            dbid,
            dbtime,
            objid,
            plrnewpage,
            &ppib,
            &fSkip ) );
    if ( fSkip )
    {
        return err;
    }

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
    const PGNO pgno = plrnewpage->Pgno();
    BOOL fRedoNewPage = fFalse;
    CSR csr;

    // Get/latch new page.
    Call( ErrLGRIAccessNewPage(
            ppib,
            &csr,
            ifmp,
            pgno,
            objid,
            dbtime,
            &fRedoNewPage ) );
    if ( fRedoNewPage )
    {
        Assert( !csr.FLatched() );
        Call( csr.ErrGetNewPreInitPageForRedo( ppib, ifmp, pgno, objid, dbtime ) );
        Assert( csr.Cpage().FPreInitPage() );
        Assert( csr.FDirty() );
        csr.SetDbtime( dbtime );
        csr.FinalizePreInitPage( OnDebug( fTrue ) );
    }
    else
    {
        Assert( ( csr.PagetrimState() == pagetrimTrimmed ) ||
                ( csr.Latch() == latchRIW ) );
    }

HandleError:
    csr.ReleasePage();
    return err;
}

//  ================================================================
ERR LOG::ErrLGRIIRedoPageMove( _In_ PIB * const ppib, const LRPAGEMOVE * const plrpagemove )
//  ================================================================
{
    Assert( ppib );
    Assert( plrpagemove );
    Assert( !plrpagemove->FPageMoveRoot() );
    ASSERT_VALID( plrpagemove );
    
    ERR err;

    // recreate the merge structure
    
    MERGEPATH * pmergePath = NULL;
    
    Call( ErrBTINewMergePath( &pmergePath ) );
    Call( ErrBTINewMergePath( &(pmergePath->pmergePathParent) ) );
    Call( ErrBTINewMerge( pmergePath ) );

    pmergePath->pmerge->mergetype               = mergetypePageMove;
    pmergePath->fEmptyPage                      = fTrue;
    pmergePath->pmergePathParent->fKeyChange    = fTrue;
    pmergePath->dbtimeBefore                    = plrpagemove->DbtimeSourceBefore();
    pmergePath->pmergePathParent->dbtimeBefore  = plrpagemove->DbtimeParentBefore();
    pmergePath->pmerge->dbtimeLeftBefore        = plrpagemove->DbtimeLeftBefore();
    pmergePath->pmerge->dbtimeRightBefore       = plrpagemove->DbtimeRightBefore();

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ plrpagemove->Dbid() ];

    BOOL fRedoSource = fFalse;
    BOOL fIgnored = fFalse;

    Call( ErrLGIAccessPageCheckDbtimes(
        ppib,
        &(pmergePath->pmergePathParent->csr),
        ifmp,
        plrpagemove->PgnoParent(),
        plrpagemove->ObjidFDP(),
        plrpagemove->DbtimeParentBefore(),
        plrpagemove->DbtimeAfter(),
        &fIgnored ) );
    
    Call( ErrLGIAccessPageCheckDbtimes(
        ppib,
        &(pmergePath->csr),
        ifmp,
        plrpagemove->PgnoSource(),
        plrpagemove->ObjidFDP(),
        plrpagemove->DbtimeSourceBefore(),
        plrpagemove->DbtimeAfter(),
        &fRedoSource ) );
 
    pmergePath->pmergePathParent->csr.SetILine( plrpagemove->IlineParent() );

    if ( pgnoNull != plrpagemove->PgnoLeft() )
    {
        if ( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( plrpagemove->PgnoLeft() ) )
        {
            pmergePath->pmerge->csrLeft.SetPagetrimState( pagetrimTrimmed, plrpagemove->PgnoLeft() );
        }
        else
        {
            Call( ErrLGIAccessPageCheckDbtimes(
                ppib,
                &(pmergePath->pmerge->csrLeft),
                ifmp,
                plrpagemove->PgnoLeft(),
                plrpagemove->ObjidFDP(),
                plrpagemove->DbtimeLeftBefore(),
                plrpagemove->DbtimeAfter(),
                &fIgnored ) );
        }
    }

    if ( pgnoNull != plrpagemove->PgnoRight() )
    {
        if ( g_rgfmp[ifmp].FPgnoInZeroedOrRevertedMaps( plrpagemove->PgnoRight() ) )
        {
            pmergePath->pmerge->csrRight.SetPagetrimState( pagetrimTrimmed, plrpagemove->PgnoRight() );
        }
        else
        {
            Call( ErrLGIAccessPageCheckDbtimes(
                ppib,
                &(pmergePath->pmerge->csrRight),
                ifmp,
                plrpagemove->PgnoRight(),
                plrpagemove->ObjidFDP(),
                plrpagemove->DbtimeRightBefore(),
                plrpagemove->DbtimeAfter(),
                &fIgnored ) );
        }
    }

    // the destination page might not exist (it could be past the end of the DB)
    BOOL fRedoDest = fTrue;
    Call( ErrLGRIAccessNewPage(
            ppib,
            &(pmergePath->pmerge->csrNew),
            ifmp,
            plrpagemove->PgnoDest(),
            plrpagemove->ObjidFDP(),
            plrpagemove->DbtimeAfter(),
            &fRedoDest ) );

    if ( fRedoDest )
    {
        Assert( !pmergePath->pmerge->csrNew.FLatched() );

        // write-latch the page
        Call( pmergePath->pmerge->csrNew.ErrGetNewPreInitPageForRedo(
                ppib,
                ifmp,
                plrpagemove->PgnoDest(),
                plrpagemove->ObjidFDP(),
                plrpagemove->DbtimeAfter() ) );
    }

    // if the destination page requires redo, but the source doesn't copy the
    // saved before-image into the source page. this reverts the source page
    // so it needs to be redone as well
    if ( fRedoDest && !fRedoSource )
    {
        Call( ErrReplacePageImageHeaderTrailer(
                ppib,
                ifmp,
                pmergePath->csr,
                plrpagemove->PvPageHeader(),
                plrpagemove->CbPageHeader(),
                plrpagemove->PvPageTrailer(),
                plrpagemove->CbPageTrailer(),
                plrpagemove->PgnoSource(),
                plrpagemove->DbtimeAfter(),
                plrpagemove->DbtimeSourceBefore() ) );
        pmergePath->csr.UpgradeFromRIWLatch();

        fRedoSource = fTrue;
    }

    if ( fRedoSource )
    {
        Assert( latchWrite == pmergePath->csr.Latch() );
    }
    else
    {
        // The Latch state is only valid for pagetrimNormal pages.
        Assert( latchRIW == pmergePath->csr.Latch() || pagetrimNormal != pmergePath->csr.PagetrimState() );
    }

    if ( fRedoDest )
    {
        Assert( fRedoSource );
        Assert( latchWrite == pmergePath->pmerge->csrNew.Latch() );
    }
    else
    {
        // The Latch state is only valid for pagetrimNormal pages.
        Assert( latchRIW == pmergePath->pmerge->csrNew.Latch() || pagetrimNormal != pmergePath->pmerge->csrNew.PagetrimState() );
    }
    
    LGIRedoMergeUpdateDbtime( pmergePath, plrpagemove->DbtimeAfter() );
    BTPerformPageMove( pmergePath );

    // Remove the source page from dbtimerevert redo map as it is freed now.
    if ( g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert() )
    {
        g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert()->ClearPgno( plrpagemove->PgnoSource() );
    }

HandleError:

    BTIReleaseMergePaths( pmergePath );
    return err;
}

//  ================================================================
ERR LOG::ErrLGRIRedoPageMove( const LRPAGEMOVE * const plrpagemove )
//  ================================================================
{
    Assert( plrpagemove );
    ASSERT_VALID( plrpagemove );
    
    ERR err;

    PIB *ppib;
    BOOL fSkip;
    Call( ErrLGRICheckRedoConditionInTrx(
            plrpagemove->le_procid,
            plrpagemove->Dbid(),
            plrpagemove->DbtimeAfter(),
            plrpagemove->ObjidFDP(),
            plrpagemove,
            &ppib,
            &fSkip ) );
    if( !fSkip )
    {
        Call( ErrLGRIIRedoPageMove( ppib, plrpagemove ) );
    }

HandleError:
    return err;
}

//  ================================================================
ERR LOG::ErrLGRIRedoPagePatch( const LRPAGEPATCHREQUEST * const plrpagepatchrequest )
//  ================================================================
{
    Assert( plrpagepatchrequest );

    ERR err = JET_errSuccess;
    const DBID dbid     = plrpagepatchrequest->Dbid();

    if ( !FLGRICheckRedoConditionForDb( dbid, m_lgposRedo ) )
    {
        return JET_errSuccess;
    }

    if ( !BoolParam( m_pinst, JET_paramEnableExternalAutoHealing ) )
    {
        return JET_errSuccess;
    }

    const IFMP ifmp     = m_pinst->m_mpdbidifmp[ dbid ];
    FMP* const pfmp     = g_rgfmp + ifmp;
    const PGNO pgno     = plrpagepatchrequest->Pgno();
    const DBTIME dbtime = plrpagepatchrequest->Dbtime();

    //  Keep track of largest dbtime so far, since the number could be
    //  logged out of order, we need to check if dbtime > than dbtimeCurrent.

    Assert( m_fRecoveringMode == fRecoveringRedo );
    if ( dbtime > pfmp->DbtimeCurrentDuringRecovery() )
    {
        pfmp->SetDbtimeCurrentDuringRecovery( dbtime );
    }

    BFLatch bfl;

    err = ErrBFWriteLatchPage( &bfl, ifmp, pgno, bflfUninitPageOk, BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ), TcCurr() );
    if ( err == JET_errPageNotInitialized )
    {
        // empty pages cannot be patched, just ignore - other than dbscan,
        // active does not care about the patch anyway
        return JET_errSuccess;
    }
    CallR( err );

    void * pvData;

    BFAlloc( bfasTemporary, &pvData );
    memcpy( pvData, bfl.pv, g_cbPage );

    // note since we release the write latch, the below callback MUST be synchronous
    BFWriteUnlatch( &bfl );

    CPAGE cpage;
    cpage.LoadPage( ifmp, pgno, pvData, g_cbPage );
    cpage.PreparePageForWrite( CPAGE::pgftRockWrite );

    (void)ErrLGOpenPagePatchRequestCallback( ifmp, pgno, dbtime, pvData );
    BFFree( pvData );

    return JET_errSuccess;
}

//  ================================================================
BOOL LOG::FLGRICheckRedoScanCheck( const LRSCANCHECK2 * const plrscancheck, BOOL fEvaluatePrereadLogic )
//  ================================================================
{
    Assert( plrscancheck );

    const DBID dbid = plrscancheck->Dbid();

    // First, check if the LR is redoable for for the database at this point.
    if ( !FLGRICheckRedoConditionForDb( dbid, m_lgposRedo ) )
    {
        return fFalse;
    }
    else if ( plrscancheck->BSource() != scsDbScan )
    {
        // Always redo it if we're not under DbScan.
        ExpectedSz( plrscancheck->BSource() == scsDbShrink, "ScanCheck not under Shrink. Please, handle." );
        Assert( plrscancheck->Pgno() != pgnoScanLastSentinel );
        return fTrue;
    }

    // WARNING: from this point on, there are only DBScan-specific checks.
    //

    // if not running with external healing (a.k.a. page patching), skip it
    if ( !BoolParam( m_pinst, JET_paramEnableExternalAutoHealing ) )
    {
        return fFalse;
    }

    // if running legacy mode, skip it
    if ( UlParam( m_pinst, JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryPassiveScan )
    {
        return fFalse;
    }

    const PGNO pgno = plrscancheck->Pgno();

    // just the DBM-done marker, not a real page (a database this big would probably be an ESE bug).
    // skip for preread, don't skip for actual processing as there is some logic that's invoked for it
    if ( fEvaluatePrereadLogic && pgno == pgnoScanLastSentinel )
    {
        return fFalse;
    }

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
    FMP* const pfmp = &( g_rgfmp[ifmp] );

    // if scanning is turned off or we've already scanned this page, skip it but signal the follower 
    // code that we are skipping it in case we need to do any special processing (e.g. for start of new pass)
    
    bool fDBMFollowerModeSet = ( UlParam( m_pinst, JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryFollowActive );
    if ( !fDBMFollowerModeSet )
    {
        // Follow-mode is not enabled at the moment, skip the page
        if ( fEvaluatePrereadLogic )
        {
            // this will update the perf counter and will reset header state if necessary
            // don't want to double call this, so only call it on the second check
            CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp, pgno );
        }

        //  DBMScan follow active during recovery is off, but we have a follower, which
        //  means someone disabled the scan mid-way through ...
        if ( pfmp->PdbmFollower() )
        {
            pfmp->PdbmFollower()->DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );
            pfmp->DestroyDBMScanFollower();
        }
        
        return fFalse;
    }
    else if ( pfmp->Pdbfilehdr()->le_pgnoDbscanHighest >= pgno )
    {
        // Follow-mode is enabled, but we've already seen this page, skip
        // unless it's pgno=1
        if ( fEvaluatePrereadLogic )
        {
            // this will update the perf counter and will reset header state if necessary
            // don't want to double call this, so only call it on the second check
            CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp, pgno );
        }

        // Special-case pgno=1, it should return true
        // because the header will get reset and it will be considered
        // a page that needs scanning
        if ( pgno != 1 )
        {
            return fFalse;
        }
        else
        {
            return fTrue;
        }
    }

    // if we have already played this scan check once then there is no need to do it again
    //
    // NOTE:  to avoid gaps due to crashes we will always replay from the max log generation
    // we have ever replayed because we can't guarantee we replayed all those records
    if ( m_lgposRedo.lGeneration < pfmp->Pdbfilehdr()->le_lGenRecovering )
    {
        return fFalse;
    }

    // We've made it this far, which means scanning is on and we haven't seen this page before
    // or it's pgno=1 and we just reset the stats. Either way, below should hold
    //Assert( plrscancheck->Pgno() > pfmp->Pdbfilehdr()->le_pgnoDbscanHighest );

    return fTrue;
}


//  ================================================================
ERR LOG::ErrLGRIRedoScanCheck( const LRSCANCHECK2 * const plrscancheck, BOOL* const pfBadPage )
//  ================================================================
{
    Assert( plrscancheck );
    Assert( pfBadPage );

    ERR err                         = JET_errSuccess;
    ERR errCorruption               = JET_errSuccess;
    const INST * const  pinst       = m_pinst;
    const IFMP          ifmp        = pinst->m_mpdbidifmp[ plrscancheck->Dbid() ];
    FMP * const         pfmp        = g_rgfmp + ifmp;
    const BOOL          fDbScan     = ( plrscancheck->BSource() == scsDbScan );
    const BOOL          fDbShrink   = ( plrscancheck->BSource() == scsDbShrink );

    Assert( ( plrscancheck->Pgno() != pgnoScanLastSentinel ) || fDbScan );

    //  if bad in _any_ way, checksum, dbtime mismatch, db divergence
    *pfBadPage = fFalse;


    Expected( m_fRecoveringMode == fRecoveringRedo );

    if ( fDbScan && ( pfmp->PdbmFollower() == NULL ) )
    {
        CallR( pfmp->ErrCreateDBMScanFollower() );
        Assert( pfmp->PdbmFollower() );
        CallR( pfmp->PdbmFollower()->ErrRegisterFollower( pfmp, plrscancheck->Pgno() ) );
    }

    Assert( plrscancheck->DbtimePage() <= plrscancheck->DbtimeCurrent() );
    const DBTIME dbtimePageInLogRec = plrscancheck->DbtimePage();

    //  as long as this is called before first latch attempt, we can tell a page that 
    //  is newly cached by a preread.

    const BOOL fPreviouslyCached = FBFPreviouslyCached( ifmp, plrscancheck->Pgno() );
    const BOOL fInCache = FBFInCache( ifmp, plrscancheck->Pgno() );

    if ( fDbScan && fPreviouslyCached )
    {
        void * pvPages = NULL;
        if ( pvPages = (BYTE *)PvOSMemoryPageAlloc( g_cbPage * 1, NULL ) )
        {
            (void)pfmp->PdbmFollower()->ErrDBMScanReadThroughCache( ifmp, plrscancheck->Pgno(), pvPages, 1 );
            OSMemoryPageFree( pvPages );
        }
        //  Note: We aren't running full DB divergence checking off the disk image ... because
        //  I believe we can't ... it could be behind the in-cache image which is what we have
        //  to unfortunately check.  The above however will at least checksum and validate the
        //  on disk image isn't corrupt from a -1018, -1021, etc perspective.
    }

    BFLatch bfl = { 0 };
    BOOL fLockedNLoaded = fFalse;   // have latch, AND loaded page ...

    C_ASSERT( pgnoSysMax < pgnoScanLastSentinel );

    // we should have skipped this in ErrLGRIRedoOperations() if this weren't true
    //Assert( !fDbScan || ( plrscancheck->Pgno() > pfmp->Pdbfilehdr()->le_pgnoDbscanHighest ) );

    if ( plrscancheck->Pgno() < pgnoSysMax )
    {
        Assert( plrscancheck->Pgno() != pgnoScanLastSentinel );

        const BOOL fRequiredRange = g_rgfmp[ifmp].FContainsDataFromFutureLogs();
        const BOOL fTrimmedDatabase = pfmp->Pdbfilehdr()->le_ulTrimCount > 0;
        const BOOL fInitDbtimePageInLogRec = plrscancheck->DbtimePage() != 0 && plrscancheck->DbtimePage() != dbtimeShrunk;
        const BOOL fUninitPossible = fRequiredRange || !fInitDbtimePageInLogRec || fTrimmedDatabase;

        if ( fDbScan )
        {
            // Fake the update of PgnoScanMax and PgnoPreReadScanMax, as those as normally updated only during the separate-thread version of DBM.
            // Since those are only used for optics and asserts, 100% correctness is not required, except for those numbers can't be too low, so
            // update them even before regardless of latch success/completion as we don't know if the latch path made it to the point where
            // PgnoLatchedScanMax gets updated.
            pfmp->UpdatePgnoPreReadScanMax( plrscancheck->Pgno() );
            pfmp->UpdatePgnoScanMax( plrscancheck->Pgno() );
        }

        err = ErrBFRDWLatchPage(
                &bfl,
                ifmp,
                plrscancheck->Pgno(),
                BFLatchFlags(
                    bflfNoTouch |
                    ( fUninitPossible ? bflfUninitPageOk : bflfNone ) |
                    bflfExtensiveChecks |
                    ( fDbScan ? bflfDBScan : bflfNone ) ),
                BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ),
                TcCurr() );

        if ( err >= JET_errSuccess )
        {
            //  load the DBTIME from the recovery database, and collect needed information

            CPAGE cpage;
            cpage.ReBufferPage( bfl, ifmp, plrscancheck->Pgno(), bfl.pv, (ULONG)UlParam( PinstFromIfmp( ifmp ), JET_paramDatabasePageSize ) );
            fLockedNLoaded = fTrue; // now we're loaded
            Assert( cpage.CbPage() == UlParam( PinstFromIfmp( ifmp ), JET_paramDatabasePageSize ) );
            const DBTIME dbtimePage = cpage.Dbtime();
            const BOOL fInitDbtimePage = dbtimePage != 0 && dbtimePage != dbtimeShrunk;
            Expected( fInitDbtimePage || ( dbtimePage == dbtimeShrunk ) ); // dbtime 0 only usually comes from a completely uninit page (-1019).

            const DBTIME dbtimeCurrentInLogRec = plrscancheck->DbtimeCurrent();
            const DBTIME dbtimeSeed = plrscancheck->DbtimeCurrent();
            const ULONG ulCompressedScanChecksumPassiveDb =
                plrscancheck->lrtyp == lrtypScanCheck ?
                UsDBMGetCompressedLoggedChecksum( cpage, dbtimeSeed ) :
                UlDBMGetCompressedLoggedChecksum( cpage, dbtimeSeed );
            const OBJID objidPage = cpage.ObjidFDP();

            // If either the dbtime from local page or dbtime from log record is dbtimeRevert, we will skip checking the consistency of the page.
            // We will also consider a page as revert new page, if active is an uninitialized page or a shrunk page but passive is a reverted new page which could happen during increseed.
            //
            const BOOL fDbtimeRevertedNewPage = 
                ( ( CmpLgpos( g_rgfmp[ ifmp ].Pdbfilehdr()->le_lgposCommitBeforeRevert, m_lgposRedo ) > 0 ||
                    plrscancheck->DbtimePage() == 0 ||
                    plrscancheck->DbtimePage() == dbtimeShrunk ) &&
                  CPAGE::FRevertedNewPage( dbtimePage ) ) || 
                CPAGE::FRevertedNewPage( plrscancheck->DbtimePage() );

            // Matching uninitialized state is OK, proceed only if at least one of the dbtimes is initialized
            // If this was a page which was reverted to a new page by RBS, we should ignore dbscan checks till the max log at the time of revert is replayed.
            const BOOL fMayCheckDbtimeConsistency = ( fInitDbtimePageInLogRec || fInitDbtimePage ) && !fDbtimeRevertedNewPage;

            // Dbtime on the page is too advanced compared to the current running dbtime of the databases.
            const BOOL fDbtimePageTooAdvanced = fMayCheckDbtimeConsistency &&
                                                fInitDbtimePage &&
                                                ( dbtimePage > dbtimeCurrentInLogRec ) &&
                                                !fRequiredRange &&
                                                !fTrimmedDatabase;

            // Mismatched initialized case.
            const BOOL fDbtimeInitMismatch = fMayCheckDbtimeConsistency &&
                                             !fDbtimePageTooAdvanced &&
                                             ( !fInitDbtimePageInLogRec != !fInitDbtimePage ) &&
                                             !fRequiredRange &&
                                             !fTrimmedDatabase;

            // Typical divergence case: both dbtimes are initialized and they diverge.
            const BOOL fDivergentDbtimes = fMayCheckDbtimeConsistency &&
                                           !fDbtimeInitMismatch &&
                                           fInitDbtimePageInLogRec &&
                                           fInitDbtimePage &&
                                           ( dbtimePage <= dbtimeCurrentInLogRec ) &&
                                           ( dbtimePage != dbtimePageInLogRec );

            if ( fDbtimePageTooAdvanced || fDbtimeInitMismatch || fDivergentDbtimes )
            {
                MessageId msgid = INTERNAL_TRACE_ID;
                PCWSTR wszDumpType = L"DbDivDbTimeMismatchUnexpected";
#ifdef USE_HAPUBLISH_API
                MessageId haMsgid = INTERNAL_TRACE_ID;
                HaDbFailureTag failureTag = HaDbFailureTagNoOp;
#endif
                if ( fDbtimePageTooAdvanced )
                {
                    Assert( !fDbtimeInitMismatch && !fDivergentDbtimes );
                    err = ErrERRCheck( JET_errDbTimeCorrupted );
                    msgid = DBTIME_CHECK_PASSIVE_CORRUPTED_ID;
                    wszDumpType = L"DbDivDbTimeCorruptedPassive";
#ifdef USE_HAPUBLISH_API
                    haMsgid = HA_DBTIME_CHECK_PASSIVE_CORRUPTED_ID;
                    failureTag = HaDbFailureTagCorruption;
#endif
                    AssertSz( fFalse, "DbScanDbTimeTooHighPassive" );
                }
                else if ( fDbtimeInitMismatch )
                {
                    Assert( !fDivergentDbtimes );
                    err = ErrERRCheck( JET_errPageInitializedMismatch );
                    wszDumpType = L"DbDivDbTimeMismatchUninitialized";

                    if ( !fInitDbtimePageInLogRec )
                    {
                        msgid = DB_DIVERGENCE_UNINIT_PAGE_ACTIVE_DB_ID;
#ifdef USE_HAPUBLISH_API
                        haMsgid = HA_DB_DIVERGENCE_UNINIT_PAGE_ACTIVE_DB_ID;
                        failureTag = HaDbFailureTagReplicaDivergenceDbTimeTooNew;
#endif
                        AssertSz( FNegTest( fCorruptingWithLostFlush ), "Page initialized mismatch, corruption on active." );
                    }
                    else
                    {
                        // The common case of the passive being uninitialized (i.e., !fInitDbtimePage) is
                        // handled further down below in this function, since the page latch attempt would have returned
                        // JET_errPageNotInitialized. We can only get !fInitDbtimePage here if the page is
                        // strangely "initialized" (i.e. with some data), but with a dbtime = 0 or dbtimeShrunk.

                        // It is wrong to log it here because the code below logs it with the wrong number and order of
                        // arguments
                        msgid = DB_DIVERGENCE_UNINIT_PAGE_PASSIVE_DB_ID;
#ifdef USE_HAPUBLISH_API
                        haMsgid = HA_DB_DIVERGENCE_UNINIT_PAGE_PASSIVE_DB_ID;
                        failureTag = HaDbFailureTagReplicaDivergenceDbTimeTooOld;
#endif
                        AssertSz( fFalse, "Page dbtime initialized mismatch, but page has data, corruption on passive." );
                    }
                }
                else if ( fDivergentDbtimes )
                {
                    if ( dbtimePageInLogRec < dbtimePage )
                    {
                        Assert( dbtimePage < dbtimeCurrentInLogRec );
                        err = ErrERRCheck( JET_errDbTimeTooNew );
                        msgid = DBTIME_CHECK_MISMATCH_ACTIVE_DB_BEHIND_ID;
                        wszDumpType = L"DbDivDbTimeMismatchTooNew";
#ifdef USE_HAPUBLISH_API
                        haMsgid = HA_DBTIME_CHECK_MISMATCH_ACTIVE_DB_BEHIND_ID;
                        failureTag = HaDbFailureTagReplicaDivergenceDbTimeTooNew;
#endif
                        AssertSz( FNegTest( fCorruptingWithLostFlush ), "DBTIME mismatch, lost flush corruption on active." );
                    }
                    else if ( dbtimePageInLogRec > dbtimePage )
                    {
                        err = ErrERRCheck( JET_errDbTimeTooOld );
                        msgid = DBTIME_CHECK_MISMATCH_PASSIVE_DB_BEHIND_ID;
                        wszDumpType = L"DbDivDbTimeMismatchTooOld";
#ifdef USE_HAPUBLISH_API
                        haMsgid = HA_DBTIME_CHECK_MISMATCH_PASSIVE_DB_BEHIND_ID;
                        failureTag = HaDbFailureTagReplicaDivergenceDbTimeTooOld;
#endif
                        AssertSz( FNegTest( fCorruptingWithLostFlush ), "DBTIME mismatch, lost flush corruption on passive." );
                    }
                    else
                    {
                        err = ErrERRCheck( errCodeInconsistency );
                        AssertSz( fFalse, "DbtimesNonDivergent" );
                    }
                }
                else
                {
                    err = ErrERRCheck( errCodeInconsistency );
                    AssertSz( fFalse, "DbtimesConsistent" );
                }

                Assert( fLockedNLoaded );       //  should be loaded ...
                if ( fLockedNLoaded )
                {
                    OSTraceSuspendGC();
                    errCorruption = cpage.ErrCaptureCorruptedPageInfoSz(
                                        CPAGE::CheckPageMode::OnErrorReturnError,
                                        wszDumpType,
                                        OSFormatW(
                                            L"DbtimeMismatch[id=%d]: b=0x%I64x; a=%I64x; p=0x%I64x",
                                            msgid,
                                            dbtimePageInLogRec,
                                            dbtimeCurrentInLogRec,
                                            dbtimePage ),
                                        fFalse );
                    //  (we couldn't do anything w/ the error ... and wouldn't want to stop the later event log)
                    OSTraceResumeGC();

                    //  release the page
                    cpage.UnloadPage();
                    BFRDWUnlatch( &bfl );
                    fLockedNLoaded = fFalse;
                }

                {
                OSTraceSuspendGC();

                const WCHAR* rgwszUninitPagePassive[] =
                {
                    g_rgfmp[ifmp].WszDatabaseName(),
                    OSFormatW( L"%I32u (0x%I32x)", plrscancheck->Pgno(), plrscancheck->Pgno() ),
                    OSFormatW( L"0x%I64x", dbtimePageInLogRec ),
                    OSFormatW( L"0x%I64x", dbtimeCurrentInLogRec ),
                    OSFormatW( L"(%08X,%04X,%04X)", m_lgposRedo.lGeneration, m_lgposRedo.isec, m_lgposRedo.ib ),
                    OSFormatW( L"%hhu", plrscancheck->BSource() )
                };

                const WCHAR* rgwszDefault[] =
                {
                    g_rgfmp[ifmp].WszDatabaseName(),
                    OSFormatW( L"%I32u (0x%I32x)", plrscancheck->Pgno(), plrscancheck->Pgno() ),
                    OSFormatW( L"0x%I64x", dbtimePageInLogRec ),
                    OSFormatW( L"0x%I64x", dbtimePage ),
                    OSFormatW( L"%d", err ),
                    OSFormatW( L"0x%I64x", dbtimeCurrentInLogRec ),
                    OSFormatW( L"(%08X,%04X,%04X)", m_lgposRedo.lGeneration, m_lgposRedo.isec, m_lgposRedo.ib ),
                    OSFormatW( L"%hhu", plrscancheck->BSource() ),
                    OSFormatW( L"%u", objidPage )
                };

                const WCHAR** const rgwsz = ( msgid == DB_DIVERGENCE_UNINIT_PAGE_PASSIVE_DB_ID ) ? rgwszUninitPagePassive : rgwszDefault;
                const DWORD cwsz = ( rgwsz == rgwszUninitPagePassive ) ? _countof( rgwszUninitPagePassive ) : _countof( rgwszDefault );

                if ( msgid != INTERNAL_TRACE_ID )
                {
                    UtilReportEvent(
                        eventError,
                        DATABASE_CORRUPTION_CATEGORY,
                        msgid,
                        cwsz,
                        rgwsz,
                        0,
                        NULL,
                        g_rgfmp[ifmp].Pinst() );
                }
                else
                {
                    FireWall( "RedoScanCheckInvalidDbTimeEqual" );
                }

                OSUHAPublishEvent(  failureTag,
                                    g_rgfmp[ifmp].Pinst(),
                                    HA_DATABASE_CORRUPTION_CATEGORY,
                                    HaDbIoErrorNone,
                                    g_rgfmp[ifmp].WszDatabaseName(),
                                    OffsetOfPgno( plrscancheck->Pgno() ),
                                    g_cbPage,
                                    haMsgid,
                                    cwsz,
                                    rgwsz );

                OSTraceResumeGC();
                }

                *pfBadPage = fTrue;
            }   // end if DBTIMEs mismatched

            //  Compare checksums only if both pages are initialized and dbtimes match.
            if ( fInitDbtimePageInLogRec && fInitDbtimePage && ( dbtimePageInLogRec == dbtimePage ) )
            {
                //  DBTIMEs match, so the page contents should match also
                if ( plrscancheck->UlChecksum() == ulCompressedScanChecksumPassiveDb )
                {
                    if ( fDbScan )
                    {
                        //  Everything matched, we've successfully compared two pages, tell perfmon ALL about it!
                        CDBMScanFollower::ProcessedDBMScanCheckRecord( pfmp );
                    }
                }
                else
                {
                    Assert( fLockedNLoaded );       //  should be loaded ...
                    if ( fLockedNLoaded )
                    {
                        OSTraceSuspendGC();
                        errCorruption = cpage.ErrCaptureCorruptedPageInfoSz(
                                            CPAGE::CheckPageMode::OnErrorReturnError,
                                            L"DbDivChecksumMismatch",
                                            OSFormatW(
                                                L"Divergence: c=0x%I32x==0x%I32x; seed=0x%I64x; t=0x%I64x,0x%I64x",
                                                ulCompressedScanChecksumPassiveDb,
                                                plrscancheck->UlChecksum(),
                                                dbtimeSeed,
                                                // These last two are less needed here.
                                                dbtimePageInLogRec,
                                                dbtimeCurrentInLogRec ),
                                            fFalse );
                        //  (we couldn't do anything w/ the error ... and wouldn't want to stop the later event log)
                        OSTraceResumeGC();

                        //  release the page
                        cpage.UnloadPage();
                        BFRDWUnlatch( &bfl );
                        fLockedNLoaded = fFalse;
                    }

                    {
                    OSTraceSuspendGC();

                    const WCHAR* rgwsz[] =
                    {
                        g_rgfmp[ifmp].WszDatabaseName(),
                        OSFormatW( L"%I32u (0x%I32x)", plrscancheck->Pgno(), plrscancheck->Pgno() ),
                        OSFormatW( L"0x%I32x", ulCompressedScanChecksumPassiveDb ),
                        OSFormatW( L"0x%I32x", plrscancheck->UlChecksum() ),
                        OSFormatW( L"0x%I64x", dbtimeSeed ),
                        OSFormatW( L"(%08X,%04X,%04X)", m_lgposRedo.lGeneration, m_lgposRedo.isec, m_lgposRedo.ib ),
                        OSFormatW( L"%hhu", plrscancheck->BSource() ),
                        OSFormatW( L"%u", objidPage )
                    };

                    UtilReportEvent(
                        eventError,
                        DATABASE_CORRUPTION_CATEGORY,
                        DB_DIVERGENCE_ID,
                        _countof( rgwsz ),
                        rgwsz,
                        0,
                        NULL,
                        g_rgfmp[ifmp].Pinst() );

                    OSUHAPublishEvent( HaDbFailureTagReplicaDivergenceDataMismatch,
                                    g_rgfmp[ifmp].Pinst(),
                                    HA_DATABASE_CORRUPTION_CATEGORY,
                                    HaDbIoErrorNone,
                                    g_rgfmp[ ifmp ].WszDatabaseName(),
                                    OffsetOfPgno( plrscancheck->Pgno() ),
                                    g_cbPage,
                                    HA_DB_DIVERGENCE_ID,
                                    _countof( rgwsz ),
                                    rgwsz );

                    OSTraceResumeGC();
                    }

                    AssertSz( FNegTest( fCorruptingPageLogically ), "Database Divergence Check FAILED!  Active and Passive are divergent." );

                    err = ErrERRCheck( JET_errDatabaseCorrupted );
                    *pfBadPage = fTrue;
                }
            }
            else if ( !( *pfBadPage ) )
            {
                //  The DBTIMEs didn't match and it wasn't a bad case / page (mostly likely we're replaying
                //  records we already replayed due to stop and start / re-init of "replication"), so this means
                //  we'll be skipping this DBScan record.
                //  BUT we purposely do not call CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp ), because this
                //  isn't "a real skip", it doesn't necessarily imply we're missing these scans, we should've done
                //  it on the previous recovery.
            }

            if ( fLockedNLoaded )
            {
                //  release the page
                
                cpage.UnloadPage();
                BFRDWUnlatch( &bfl );
                fLockedNLoaded = fFalse;
            }

        }   // end if got RDW latch
        else
        {
            //  Handle errors ...

            BOOL fPageBeyondEof = fFalse;
            switch( err )
            {
                case_AllDatabaseStorageCorruptionErrs:
                    // We shouldn't need to log an event because the RDW latch should've taken care of that.
                    *pfBadPage = fTrue;
                    break;
                case JET_errFileIOBeyondEOF:
                    fPageBeyondEof = fTrue;
                case JET_errPageNotInitialized:
                    if ( fInitDbtimePageInLogRec && !fRequiredRange && ( !fTrimmedDatabase || fPageBeyondEof ) )
                    {
                        *pfBadPage = fTrue;

                        {
                        OSTraceSuspendGC();
                        const WCHAR* rgwsz[] =
                        {
                            pfmp->WszDatabaseName(),
                            OSFormatW( L"%I32u (0x%I32x)", plrscancheck->Pgno(), plrscancheck->Pgno() ),
                            OSFormatW( L"0x%I64x", dbtimePageInLogRec ),
                            OSFormatW( L"0x%I64x", plrscancheck->DbtimeCurrent() ),
                            OSFormatW( L"(%08X,%04X,%04X)", m_lgposRedo.lGeneration, m_lgposRedo.isec, m_lgposRedo.ib ),
                            OSFormatW( L"%hhu", plrscancheck->BSource() )
                        };
                        UtilReportEvent(
                            eventError,
                            DATABASE_CORRUPTION_CATEGORY,
                            fPageBeyondEof ? DB_DIVERGENCE_BEYOND_EOF_PAGE_PASSIVE_DB_ID : DB_DIVERGENCE_UNINIT_PAGE_PASSIVE_DB_ID,
                            _countof( rgwsz ),
                            rgwsz,
                            0,
                            NULL,
                            pfmp->Pinst() );

                        OSTraceResumeGC();
                        }

                        // if this becomes a test problem, someone "sin" this out ... 
                        AssertSz( FNegTest( fCorruptingWithLostFlush ), "We hit a page uninitialized mismatch." );
                    }
                    else
                    {
                        err = JET_errSuccess;
                    }
                    break;
                case JET_errOutOfMemory:
                case JET_errOutOfBuffers:
                    if ( fDbScan )
                    {
                        CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp );    // for now, note it in perfmon
                    }
                    break;

                default:
                    AssertSz( fFalse, "Q: What else wicked this way comes?  A: %d", err );
                    break;
            }   //  switch
        }   //  else err on RDW latch

        Enforce( !fLockedNLoaded );     //  did we leak the lock?
        
        if ( fDbScan )
        {
            //  Register/complete that we've seen this page.
            pfmp->PdbmFollower()->CompletePage( plrscancheck->Pgno(), *pfBadPage );

            if ( !fPreviouslyCached )
            {
                //  since the page was not previously cached, we should super cold it

                if ( m_arrayPagerefSupercold.Capacity() <= m_arrayPagerefSupercold.Size() )
                {
                    (void)m_arrayPagerefSupercold.ErrSetCapacity( LNextPowerOf2( m_arrayPagerefSupercold.Size() + 1 ) );
                }

                (void)m_arrayPagerefSupercold.ErrSetEntry(  m_arrayPagerefSupercold.Size(),
                                                            PageRef( plrscancheck->Dbid(), plrscancheck->Pgno() ) );
            }

            //  Ideally, we should check for the error returned when latching the page to filter out cases where
            //  the read operation itself failed, but the inaccuracy in the perf. counter would be minor, since
            //  those cases should be rare.
            if ( !fInCache )
            {
                pfmp->PdbmFollower()->UpdateIoTracking( ifmp, 1 );
            }
        }
    }   //  end if pgno < pgnoScanLastSentinel
    else if ( plrscancheck->Pgno() == pgnoScanLastSentinel )
    {
        Assert( fDbScan );

        //  No other values should be present if we're at pgnoScanLastSentinel ...
        Expected( plrscancheck->UlChecksum() == 0 && plrscancheck->DbtimePage() == 0 && plrscancheck->DbtimeCurrent() == 0 );
        //  just in case someone decides to extend the LR later.
        if ( fDbScan && plrscancheck->UlChecksum() == 0 && plrscancheck->DbtimePage() == 0 && plrscancheck->DbtimeCurrent() == 0 )
        {
            pfmp->PdbmFollower()->DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );
            pfmp->DestroyDBMScanFollower();
        }
    }   //  end if pgno == pgnoScanLastSentinel
    else
    {
        AssertSz( fFalse, "Huh, we got a pgno larger than pgnoSysMax (and that wasn't pgnoScanLastSentinel)??  pgno = %d", plrscancheck->Pgno() );
    }

    Enforce( !fLockedNLoaded );     //  did we leak the lock?

    // Just in case.
    if ( ( errCorruption < JET_errSuccess ) && ( err >= JET_errSuccess ) )
    {
        Expected( fFalse );
        err = errCorruption;
    }

    Assert( !( *pfBadPage ) || ( err < JET_errSuccess ) );

    return err;
}


//  recovers a merge or an empty page operation
//      with accompanying node delete operations
//
ERR LOG::ErrLGRIRedoMerge( PIB *ppib, DBTIME dbtime )
{
    ERR             err;
    const LRMERGE_  * const plrmerge    = (LRMERGE_ *) ppib->PvLogrec( dbtime );
    const DBID      dbid        = plrmerge->dbid;

    Assert( lrtypMerge == plrmerge->lrtyp || lrtypMerge2 == plrmerge->lrtyp );
    Assert( dbtime  == plrmerge->le_dbtime );

    const OBJID     objidFDP    = plrmerge->le_objidFDP;
    MERGEPATH       *pmergePathLeaf = NULL;

    Assert( ppib->FMacroGoing( dbtime ) );
    BOOL fSkip;
    Call( ErrLGRICheckRedoCondition(
                dbid,
                dbtime,
                objidFDP,
                ppib,
                fFalse,
                &fSkip ) );
    if ( fSkip )
    {
        return JET_errSuccess;
    }

    //  reconstructs merge structures RIW-latching pages that need redo
    //
    FUCB            *pfucb = pfucbNil;

    Call( ErrLGRIRedoMergeStructures( ppib,
                                      dbtime,
                                      &pmergePathLeaf,
                                      &pfucb ) );
    Assert( pmergePathLeaf != NULL );
    Assert( pmergePathLeaf->pmerge != NULL );

    Assert( pfucb->bmCurr.key.FNull() );
    Assert( pfucb->bmCurr.data.FNull() );

    //  write latch pages that need redo
    //
    Call( ErrLGIRedoMergeUpgradeLatches( m_pinst->m_mpdbidifmp[ dbid ], pmergePathLeaf, dbtime ) );

    //  sets dirty and dbtime on all updated pages
    //
    LGIRedoMergeUpdateDbtime( pmergePathLeaf, dbtime );

    //  calls BTIPerformMerge
    //
    BTIPerformMerge( pfucb, pmergePathLeaf );

    //  removes page with db time dbtimerevert from dbtimerevert redomap if page is empty.
    //
    LGIRedoMergeClearRedoMapDbtimeRevert( pmergePathLeaf, m_pinst->m_mpdbidifmp[ dbid ] );

HandleError:
    AssertSz( errSkipLogRedoOperation != err, "%s() got errSkipLogRedoOperation from ErrLGRICheckRedoCondition(dbtime=%d=%#x).\n",
              __FUNCTION__, dbtime, dbtime );

    //  release latches
    //
    if ( pmergePathLeaf != NULL )
    {
        BTIReleaseMergePaths( pmergePathLeaf );
    }

    return err;
}

//  upgrades latches on pages that need redo
//
LOCAL ERR ErrLGIRedoSplitUpgradeLatches( const IFMP ifmp, SPLITPATH *psplitPathLeaf, DBTIME dbtime )
{
    ERR         err         = JET_errSuccess;
    SPLITPATH*  psplitPath;

    //  go to root
    //  since we need to latch top-down
    //
    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    Assert( NULL == psplitPath->psplitPathParent );
    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        Assert( latchRIW == psplitPath->csr.Latch() || pagetrimTrimmed == psplitPath->csr.PagetrimState() );

        if ( FLGNeedRedoCheckDbtimeBefore( ifmp, psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err ) )
        {
            Call ( err );
            psplitPath->csr.UpgradeFromRIWLatch();
        }
        CallS( err );

        SPLIT   *psplit = psplitPath->psplit;
        if ( psplit != NULL )
        {
            //  new page should already be write-latched if redo is needed
            //
#ifdef DEBUG
            if ( FLGNeedRedoPage( psplit->csrNew, dbtime ) )
            {
                Assert( latchWrite == psplit->csrNew.Latch() );
            }
            else
            {
                Assert( latchRIW == psplit->csrNew.Latch() || pagetrimTrimmed == psplit->csrNew.PagetrimState() );
            }

            if ( pgnoNull == psplit->csrRight.Pgno() )
            {
                Assert( !psplit->csrRight.FLatched() );
            }
            else
            {
                Assert( latchRIW == psplit->csrRight.Latch() || pagetrimTrimmed == psplit->csrRight.PagetrimState() );
            }
#endif

            if ( psplit->csrRight.FLatched() && FLGNeedRedoCheckDbtimeBefore( ifmp, psplit->csrRight, dbtime, psplit->dbtimeRightBefore, &err ) )
            {
                Call( err );
                psplit->csrRight.UpgradeFromRIWLatch();
            }
            CallS( err );
        }
    }
HandleError:
    return err;
}


//  recovers split operation
//      reconstructs split structures write-latching pages that need redo
//      creates version for operation
//      calls BTIPerformSplit
//      sets dbtime on all updated pages
//
ERR LOG::ErrLGRIRedoSplit( PIB *ppib, DBTIME dbtime )
{
    ERR             err;
    const LRSPLIT_  *plrsplit   = (LRSPLIT_ *) ppib->PvLogrec( dbtime );
    const DBID      dbid        = plrsplit->dbid;

    Assert( lrtypSplit == plrsplit->lrtyp || lrtypSplit2 == plrsplit->lrtyp );
    Assert( ppib->FMacroGoing( dbtime ) );
    Assert( dbtime == plrsplit->le_dbtime );

    const LEVEL     level       = plrsplit->level;

    //  if operation was performed by concurrent CreateIndex, the
    //  updater could be at a higher trx level than when the
    //  indexer logged the operation
    Assert( level == ppib->Level()
            || ( level < ppib->Level() && plrsplit->FConcCI() ) );

    const OBJID     objidFDP    = plrsplit->le_objidFDP;
    SPLITPATH       *psplitPathLeaf = NULL;

    BOOL fSkip;
    CallR( ErrLGRICheckRedoCondition(
                dbid,
                dbtime,
                objidFDP,
                ppib,
                fFalse,
                &fSkip ) );
    if ( fSkip )
    {
        return JET_errSuccess;
    }

    //  reconstructs split structures write-latching pages that need redo
    //
    FUCB            *pfucb              = pfucbNil;
    KEYDATAFLAGS    kdf;
    DIRFLAG         dirflag             = fDIRNull;
    BOOL            fVersion;
    RCEID           rceidOper1          = rceidNull;
    RCEID           rceidOper2          = rceidNull;
    BOOL            fOperNeedsRedo      = fFalse;
    SPLIT           *psplit;
    BYTE            *rgb                = NULL;

    Call( ErrLGIRedoSplitStructures( ppib,
                                     dbtime,
                                     &psplitPathLeaf,
                                     &pfucb,
                                     &dirflag,
                                     &kdf,
                                     &rceidOper1,
                                     &rceidOper2 ) );
    Assert( psplitPathLeaf != NULL );
    Assert( psplitPathLeaf->psplit != NULL );

    Assert( pfucb->bmCurr.key.FNull() );
    Assert( pfucb->bmCurr.data.FNull() );

    //  upgrade latches on pages that need redo
    //
    Call( ErrLGIRedoSplitUpgradeLatches( m_pinst->m_mpdbidifmp[ dbid ], psplitPathLeaf, dbtime ) );

    psplit = psplitPathLeaf->psplit;

    Assert( !fOperNeedsRedo );      //  initial value
    if ( splitoperNone != psplit->splitoper )
    {
        if ( psplit->ilineOper < psplit->ilineSplit )
        {
            fOperNeedsRedo = ( latchWrite == psplitPathLeaf->csr.Latch() );
        }
        else
        {
            fOperNeedsRedo = ( latchWrite == psplit->csrNew.Latch() );
        }
    }

    if ( splitoperReplace == psplitPathLeaf->psplit->splitoper
        && fOperNeedsRedo )
    {
        //  copy bookmark to FUCB
        //
        BTISplitGetReplacedNode( pfucb, psplitPathLeaf->psplit );

        Assert( FFUCBUnique( pfucb ) );

        BFAlloc( bfasTemporary, (VOID **)&rgb );
        pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );
        pfucb->bmCurr.key.suffix.SetPv( rgb );
        pfucb->bmCurr.key.suffix.SetCb( pfucb->kdfCurr.key.Cb() );
    }
    else if ( splitoperFlagInsertAndReplaceData == psplitPathLeaf->psplit->splitoper )
    {
        NDGetBookmarkFromKDF( pfucb, kdf, &pfucb->bmCurr );
    }

    //  creates version for operation
    //
    fVersion = !( dirflag & fDIRNoVersion ) &&
               !g_rgfmp[ pfucb->ifmp ].FVersioningOff() &&
               splitoperNone != psplitPathLeaf->psplit->splitoper;

    if ( fVersion )
    {
        Assert( rceidNull != rceidOper1 );
        Assert( level > 0 );
        Call( ErrLGIRedoSplitCreateVersion( psplitPathLeaf->psplit,
                                            pfucb,
                                            kdf,
                                            dirflag,
                                            rceidOper1,
                                            rceidOper2,
                                            level ) );
    }

    //  sets dirty and dbtime on all updated pages
    //
    LGIRedoSplitUpdateDbtime( psplitPathLeaf, dbtime );

    //  calls BTIPerformSplit
    //
    BTIPerformSplit( pfucb, psplitPathLeaf, &kdf, dirflag );

HandleError:
    //  release latches
    //
    if ( psplitPathLeaf != NULL )
    {
        BTIReleaseSplitPaths( PinstFromPpib( ppib ), psplitPathLeaf );
    }

    if ( NULL != rgb )
    {
        BFFree( rgb );
    }

    return err;
}


ERR LOG::ErrLGIRedoRootMoveStructures( PIB* const ppib, const DBTIME dbtime, ROOTMOVE* const prm )
{
    ERR err = JET_errSuccess;
    const LR* plr = NULL;
    IFMP ifmp = ifmpNil;

    Assert( dbtime != dbtimeInvalid );
    Assert( dbtime != dbtimeNil );
    Assert( prm->dbtimeAfter == dbtimeInvalid );

    // Process LRs in the macro.
    for ( UINT ibNextLR = 0;
          ibNextLR < ppib->CbSizeLogrec( dbtime );
          ibNextLR += CbLGSizeOfRec( plr ) )
    {
        plr = (LR*)( (BYTE*)ppib->PvLogrec( dbtime ) + ibNextLR );
        switch( plr->lrtyp )
        {
            // We expect one of these LRs, and the first of the macro.
            case lrtypRootPageMove:
            {
                const LRROOTPAGEMOVE* const plrrpm = (LRROOTPAGEMOVE*)plr;
                ASSERT_VALID( plrrpm );
                Assert( plrrpm->DbtimeAfter() == dbtime );
                Assert( prm->dbtimeAfter == dbtimeInvalid );

                ifmp = m_pinst->m_mpdbidifmp[ plrrpm->Dbid() ];

                prm->dbtimeAfter = plrrpm->DbtimeAfter();
                prm->objid = plrrpm->Objid();
            }
            break;

            // We expect three of these LRs (root, OE, AE), right after LRROOTPAGEMOVE.
            case lrtypPageMove:
            {
                const LRPAGEMOVE* const plrpm = (LRPAGEMOVE*)plr;
                ASSERT_VALID( plrpm );
                Assert( plrpm->DbtimeAfter() == dbtime );
                Assert( plrpm->DbtimeSourceBefore() != dbtimeInvalid );
                Assert( plrpm->DbtimeSourceBefore() != dbtimeNil );
                Assert( prm->dbtimeAfter == dbtime );

                PGNO* ppgno = NULL;
                PGNO* ppgnoNew = NULL;
                DBTIME* pdbtimeBefore = NULL;
                DATA* pdataBefore = NULL;
                CSR* pcsr = pcsrNil;
                CSR* pcsrNew = pcsrNil;

                if ( plrpm->FPageMoveRootFDP() )
                {
                    ppgno = &prm->pgnoFDP;
                    ppgnoNew = &prm->pgnoNewFDP;
                    pdbtimeBefore = &prm->dbtimeBeforeFDP;
                    pdataBefore = &prm->dataBeforeFDP;
                    pcsr = &prm->csrFDP;
                    pcsrNew = &prm->csrNewFDP;
                }
                else if ( plrpm->FPageMoveRootOE() )
                {
                    ppgno = &prm->pgnoOE;
                    ppgnoNew = &prm->pgnoNewOE;
                    pdbtimeBefore = &prm->dbtimeBeforeOE;
                    pdataBefore = &prm->dataBeforeOE;
                    pcsr = &prm->csrOE;
                    pcsrNew = &prm->csrNewOE;
                }
                else if ( plrpm->FPageMoveRootAE() )
                {
                    ppgno = &prm->pgnoAE;
                    ppgnoNew = &prm->pgnoNewAE;
                    pdbtimeBefore = &prm->dbtimeBeforeAE;
                    pdataBefore = &prm->dataBeforeAE;
                    pcsr = &prm->csrAE;
                    pcsrNew = &prm->csrNewAE;
                }
                else
                {
                    Assert( fFalse );
                }

                Assert( *ppgno == pgnoNull );
                *ppgno = plrpm->PgnoSource();
                *ppgnoNew = plrpm->PgnoDest();
                *pdbtimeBefore = plrpm->DbtimeSourceBefore();

                // Latch source page.
                if ( ( err = ErrLGIAccessPage( ppib, pcsr, ifmp, *ppgno, plrpm->ObjidFDP(), fFalse ) ) == errSkipLogRedoOperation )
                {
                    Assert( !pcsr->FLatched() );
                    err = JET_errSuccess;
                }
                else
                {
                    Call( err );
                    Assert( pcsr->Latch() == latchRIW );
                }

                // Get/latch new page.
                BOOL fRedoNewPage = fFalse;
                Call( ErrLGRIAccessNewPage(
                        ppib,
                        pcsrNew,
                        ifmp,
                        *ppgnoNew,
                        plrpm->ObjidFDP(),
                        dbtime,
                        &fRedoNewPage ) );
                if ( fRedoNewPage )
                {
                    Assert( !pcsrNew->FLatched() );
                    Call( pcsrNew->ErrGetNewPreInitPageForRedo(
                                        ppib,
                                        ifmp,
                                        *ppgnoNew,
                                        prm->objid,
                                        dbtime ) );

                    // Get source page image.
                    VOID* pv = NULL;
                    BFAlloc( bfasTemporary, &pv, g_cbPage );
                    pdataBefore->SetPv( pv );
                    pdataBefore->SetCb( g_cbPage );
                    RebuildPageImageHeaderTrailer(
                        plrpm->PvPageHeader(),
                        plrpm->CbPageHeader(),
                        plrpm->PvPageTrailer(),
                        plrpm->CbPageTrailer(),
                        pdataBefore->Pv() );
                }
                else
                {
                    Assert( ( pcsrNew->PagetrimState() == pagetrimTrimmed ) ||
                            ( pcsrNew->Latch() == latchRIW ) );
                }

                // Remove the source page from dbtimerevert redo map as it is freed now.
                if ( g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert() )
                {
                    g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert()->ClearPgno( plrpm->PgnoSource() );
                }
            }

            break;

            // We expect N of these LRs, one for each "child" object (secondary index, LV).
            case lrtypSetExternalHeader:
            {
                const LRSETEXTERNALHEADER* const plrseh = (LRSETEXTERNALHEADER*)plr;
                Assert( plrseh->le_dbtime == dbtime );
                Assert( plrseh->le_dbtimeBefore != dbtimeInvalid );
                Assert( plrseh->le_dbtimeBefore != dbtimeNil );
                Assert( prm->dbtimeAfter == dbtime );
                Assert( plrseh->le_pgno == plrseh->le_pgnoFDP );

                // Allocate and set up child root move object.
                ROOTMOVECHILD* const prmc = new ROOTMOVECHILD;
                Alloc( prmc );
                prm->AddRootMoveChild( prmc );

                prmc->dbtimeBeforeChildFDP = plrseh->le_dbtimeBefore;
                prmc->pgnoChildFDP = plrseh->le_pgno;
                prmc->objidChild = plrseh->le_objidFDP;

                // Latch page.
                if ( ( err = ErrLGIAccessPage( ppib, &prmc->csrChildFDP, ifmp, prmc->pgnoChildFDP, plrseh->le_objidFDP, fFalse ) ) == errSkipLogRedoOperation )
                {
                    Assert( !prmc->csrChildFDP.FLatched() );
                    err = JET_errSuccess;
                }
                else
                {
                    Call( err );
                    Assert( prmc->csrChildFDP.Latch() == latchRIW );

                    // Copy new data.
                    const USHORT cbData = plrseh->CbData();
                    Assert( cbData == sizeof( prmc->sphNew ) );
                    Assert( cbData == prmc->dataSphNew.Cb() );
                    UtilMemCpy( prmc->dataSphNew.Pv(), plrseh->rgbData, cbData );
                }
            }
            break;

            // We expect either one or two of these LRs, depending on catalog layout and wether or not
            // the primary index is user-defined or automatic (auto-inc).
            case lrtypReplace:
            {
                const LRREPLACE* const plrr = (LRREPLACE*)plr;
                Assert( plrr->le_dbtime == dbtime );
                Assert( plrr->le_dbtimeBefore != dbtimeInvalid );
                Assert( plrr->le_dbtimeBefore != dbtimeNil );
                Assert( prm->dbtimeAfter == dbtime );

                const OBJID objid = plrr->le_objidFDP;
                Assert( ( objid == objidFDPMSO ) || ( objid == objidFDPMSOShadow ) );
                const int iCat = ( objid == objidFDPMSO ) ? 0 : 1;

                PGNO* ppgno = NULL;
                DBTIME* pdbtimeBefore = NULL;
                DATA* pdataNew = NULL;
                CSR* pcsr = pcsrNil;
                INT* piline = NULL;
                BOOL fCatClustIdx = fFalse;

                // Select whether this is the primary object or an index.
                if ( prm->ilineCatObj[iCat] == -1 )
                {
                    Assert( prm->ilineCatClustIdx[iCat] == -1 );
                    pdbtimeBefore = &prm->dbtimeBeforeCatObj[iCat];
                    pcsr = &prm->csrCatObj[iCat];
                    ppgno = &prm->pgnoCatObj[iCat];
                    piline = &prm->ilineCatObj[iCat];
                    pdataNew = &prm->dataNewCatObj[iCat];
                }
                else if ( prm->ilineCatClustIdx[iCat] == -1 )
                {
                    pdbtimeBefore = &prm->dbtimeBeforeCatClustIdx[iCat];
                    pcsr = &prm->csrCatClustIdx[iCat];
                    ppgno = &prm->pgnoCatClustIdx[iCat];
                    piline = &prm->ilineCatClustIdx[iCat];
                    pdataNew = &prm->dataNewCatClustIdx[iCat];
                    fCatClustIdx = fTrue;
                }
                else
                {
                    Assert( fFalse );
                }

                Assert( *ppgno == pgnoNull );
                Assert( pdataNew->FNull() );
                *ppgno = plrr->le_pgno;
                *pdbtimeBefore = plrr->le_dbtimeBefore;
                *piline = plrr->ILine();

                // Latch page.
                if ( !fCatClustIdx || ( *ppgno != prm->pgnoCatObj[iCat] ) )
                {
                    if ( ( err = ErrLGIAccessPage( ppib, pcsr, ifmp, *ppgno, objid, fFalse ) ) == errSkipLogRedoOperation )
                    {
                        Assert( !pcsr->FLatched() );
                        err = JET_errSuccess;
                    }
                    else
                    {
                        Call( err );
                        Assert( pcsr->Latch() == latchRIW );
                    }
                }

                // Copy new data if we need to redo.
                if ( ( ( !fCatClustIdx && pcsr->FLatched() ) ||
                     ( ( fCatClustIdx &&
                       ( ( ( *ppgno != prm->pgnoCatObj[iCat] ) && pcsr->FLatched() ) ||
                         ( ( *ppgno == prm->pgnoCatObj[iCat] ) && prm->csrCatObj[iCat].FLatched() ) ) ) ) ) )
                {
                    VOID* pv = NULL;
                    const USHORT cbNewData = plrr->CbNewData();
                    BFAlloc( bfasTemporary, &pv, cbNewData );
                    pdataNew->SetPv( pv );
                    pdataNew->SetCb( cbNewData );
                    UtilMemCpy( pv, plrr->szData, cbNewData );
                }
            }
            break;

            default:
                Assert( fFalse );
            break;
        }
    }

HandleError:
    return err;
}


ERR ErrLGIRedoRootMoveUpgradeLatches( const IFMP ifmp, ROOTMOVE* const prm )
{
    ERR err = JET_errSuccess;
    const DBTIME dbtimeAfter = prm->dbtimeAfter;

    // Root.
    if ( FLGNeedRedoCheckDbtimeBefore( ifmp, prm->csrFDP, dbtimeAfter, prm->dbtimeBeforeFDP, &err ) )
    {
        Call ( err );
        prm->csrFDP.UpgradeFromRIWLatch();
    }
    CallS( err );

    // OE.
    if ( FLGNeedRedoCheckDbtimeBefore( ifmp, prm->csrOE, dbtimeAfter, prm->dbtimeBeforeOE, &err ) )
    {
        Call ( err );
        prm->csrOE.UpgradeFromRIWLatch();
    }
    CallS( err );

    // AE.
    if ( FLGNeedRedoCheckDbtimeBefore( ifmp, prm->csrAE, dbtimeAfter, prm->dbtimeBeforeAE, &err ) )
    {
        Call ( err );
        prm->csrAE.UpgradeFromRIWLatch();
    }
    CallS( err );

    // New pages are supposed to be already write-latched if needed.
    if ( FLGNeedRedoPage( prm->csrNewFDP, dbtimeAfter ) )
    {
        Assert( latchWrite == prm->csrNewFDP.Latch() );
    }
    else
    {
        Assert( latchRIW == prm->csrNewFDP.Latch() ||
                pagetrimTrimmed == prm->csrNewFDP.PagetrimState() );
        BFFree( prm->dataBeforeFDP.Pv() );
        prm->dataBeforeFDP.Nullify();
    }

    if ( FLGNeedRedoPage( prm->csrNewOE, dbtimeAfter ) )
    {
        Assert( latchWrite == prm->csrNewOE.Latch() );
    }
    else
    {
        Assert( latchRIW == prm->csrNewOE.Latch() ||
                pagetrimTrimmed == prm->csrNewOE.PagetrimState() );
        BFFree( prm->dataBeforeOE.Pv() );
        prm->dataBeforeOE.Nullify();
    }

    if ( FLGNeedRedoPage( prm->csrNewAE, dbtimeAfter ) )
    {
        Assert( latchWrite == prm->csrNewAE.Latch() );
    }
    else
    {
        Assert( latchRIW == prm->csrNewAE.Latch() ||
                pagetrimTrimmed == prm->csrNewAE.PagetrimState() );
        BFFree( prm->dataBeforeAE.Pv() );
        prm->dataBeforeAE.Nullify();
    }

    // Change children objects to point to new root.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        if ( FLGNeedRedoCheckDbtimeBefore( ifmp, prmc->csrChildFDP, dbtimeAfter, prmc->dbtimeBeforeChildFDP, &err ) )
        {
            Call ( err );
            prmc->csrChildFDP.UpgradeFromRIWLatch();
        }
        CallS( err );
    }

    // Change catalog pages to point to new root.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        if ( FLGNeedRedoCheckDbtimeBefore( ifmp, prm->csrCatObj[iCat], dbtimeAfter, prm->dbtimeBeforeCatObj[iCat], &err ) )
        {
            Call ( err );
            prm->csrCatObj[iCat].UpgradeFromRIWLatch();
        }
        else
        {
            BFFree( prm->dataNewCatObj[iCat].Pv() );
            prm->dataNewCatObj[iCat].Nullify();

            if ( ( prm->pgnoCatClustIdx[iCat] != pgnoNull ) &&
                 ( prm->pgnoCatClustIdx[iCat] == prm->pgnoCatObj[iCat] ) )
            {
                BFFree( prm->dataNewCatClustIdx[iCat].Pv() );
                prm->dataNewCatClustIdx[iCat].Nullify();
            }
        }
        CallS( err );

        if ( ( prm->pgnoCatClustIdx[iCat] != pgnoNull ) &&
             ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) )
        {
            if ( FLGNeedRedoCheckDbtimeBefore( ifmp, prm->csrCatClustIdx[iCat], dbtimeAfter, prm->dbtimeBeforeCatClustIdx[iCat], &err ) )
            {
                Call ( err );
                prm->csrCatClustIdx[iCat].UpgradeFromRIWLatch();
            }
            else
            {
                BFFree( prm->dataNewCatClustIdx[iCat].Pv() );
                prm->dataNewCatClustIdx[iCat].Nullify();
            }
            CallS( err );
        }
    }
HandleError:
    return err;
}


VOID LOG::LGIRedoRootMoveUpdateDbtime( ROOTMOVE* const prm )
{
    const DBTIME dbtimeAfter = prm->dbtimeAfter;

    // Root.
    if ( FBTIUpdatablePage( prm->csrFDP ) )
    {
        prm->csrFDP.Dirty();
        prm->csrFDP.SetDbtime( dbtimeAfter );
    }

    // OE.
    if ( FBTIUpdatablePage( prm->csrOE ) )
    {
        prm->csrOE.Dirty();
        prm->csrOE.SetDbtime( dbtimeAfter );
    }

    // AE.
    if ( FBTIUpdatablePage( prm->csrAE ) )
    {
        prm->csrAE.Dirty();
        prm->csrAE.SetDbtime( dbtimeAfter );
    }

    // New root.
    if ( FBTIUpdatablePage( prm->csrNewFDP ) )
    {
        Assert( prm->csrNewFDP.FDirty() );
        prm->csrNewFDP.SetDbtime( dbtimeAfter );
    }

    // New OE.
    if ( FBTIUpdatablePage( prm->csrNewOE ) )
    {
        Assert( prm->csrNewOE.FDirty() );
        prm->csrNewOE.SetDbtime( dbtimeAfter );
    }

    // New AE.
    if ( FBTIUpdatablePage( prm->csrNewAE ) )
    {
        Assert( prm->csrNewAE.FDirty() );
        prm->csrNewAE.SetDbtime( dbtimeAfter );
    }

    // IMPORTANT: new pages are updated separately upon page initialization in SHKPerformRootMove().

    // Change children objects to point to new root.
    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        if ( FBTIUpdatablePage( prmc->csrChildFDP ) )
        {
            prmc->csrChildFDP.Dirty();
            prmc->csrChildFDP.SetDbtime( dbtimeAfter );
        }
    }

    // Change catalog pages to point to new root.
    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        if ( FBTIUpdatablePage( prm->csrCatObj[iCat] ) )
        {
            prm->csrCatObj[iCat].Dirty();
            prm->csrCatObj[iCat].SetDbtime( dbtimeAfter );
        }

        if ( ( prm->pgnoCatClustIdx[iCat] != pgnoNull ) &&
             ( prm->pgnoCatClustIdx[iCat] != prm->pgnoCatObj[iCat] ) &&
             FBTIUpdatablePage( prm->csrCatClustIdx[iCat] ) )
        {
            prm->csrCatClustIdx[iCat].Dirty();
            prm->csrCatClustIdx[iCat].SetDbtime( dbtimeAfter );
        }
    }
}


//  recovers root page move operation
//      reconstructs root page move structures write-latching pages that need to be redone
//      performs the necessary changes (SHKPerformRootMove)
//      sets dbtime on all updated pages
//
ERR LOG::ErrLGRIRedoRootPageMove( PIB* const ppib, const DBTIME dbtime )
{
    ERR err = JET_errSuccess;
    ROOTMOVE rm;
    BOOL fSkip = fFalse;

    const LRROOTPAGEMOVE* const plrrpm = (LRROOTPAGEMOVE*)( ppib->PvLogrec( dbtime ) );
    ASSERT_VALID( plrrpm );
    Assert( ppib->FMacroGoing( dbtime ) );
    Assert( plrrpm->DbtimeAfter() == dbtime );

    const DBID dbid = plrrpm->Dbid();

    Call( ErrLGRICheckRedoCondition(
            dbid,
            dbtime,
            plrrpm->Objid(),
            ppib,
            fFalse,
            &fSkip ) );
    if ( fSkip )
    {
        goto HandleError;
    }

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
    FMP* const pfmp = &g_rgfmp[ ifmp ];

    // Fire DetachDB callback.
    // Read-from-passive clients might be reading from pages/trees we are about to affect with
    // this root move, so signal a detach callback. Once the overall shrink operation is completed,
    // we log lrtypSignalAttachDb so that they can re-attach the database.
    Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );

    // Rebuild structures to perform the move.
    Call( ErrLGIRedoRootMoveStructures( ppib, dbtime, &rm ) );
    Assert( dbtime == rm.dbtimeAfter );

    // Upgrade latches of pages that need to be redone.
    Call( ErrLGIRedoRootMoveUpgradeLatches( ifmp, &rm ) );

    // Make sure all pages are dirty and have the final dbtime.
    LGIRedoRootMoveUpdateDbtime( &rm );
    OnDebug( rm.AssertValid( fTrue /* fBeforeMove */, fTrue /* fRedo */ ) );

    // Purge all FCBs/FUCBs related to this object and the catalogs.
    Call( ErrLGRIPurgeFcbs( ifmp, rm.pgnoFDP, fFDPTypeUnknown, m_pctablehash ) );
    Call( ErrLGRIPurgeFcbs( ifmp, pgnoFDPMSO, fFDPTypeTable, m_pctablehash ) );
    Call( ErrLGRIPurgeFcbs( ifmp, pgnoFDPMSOShadow, fFDPTypeTable, m_pctablehash ) );

    // Perform the actual operation.
    SHKPerformRootMove(
        &rm,
        ppib,
        ifmp,
        fTrue );    // fRecoveryRedo
    OnDebug( rm.AssertValid( fFalse /* fBeforeMove */, fTrue /* fRedo */ ) );

    // We must not have reloaded these.
    BOOL fUnused = fFalse;
    Assert( FCB::PfcbFCBGet( ifmp, rm.pgnoFDP, &fUnused ) == pfcbNil );
    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPMSO, &fUnused ) == pfcbNil );
    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPMSOShadow, &fUnused ) == pfcbNil );

HandleError:
    rm.ReleaseResources();
    return err;
}


//  redoes macro operation
//      [either a split or a merge]
//
ERR LOG::ErrLGRIRedoMacroOperation( PIB *ppib, DBTIME dbtime )
{
    ERR     err;
    LR      *plr    = (LR *) ppib->PvLogrec( dbtime );
    if ( plr == NULL )
    {
        FireWall( "NullLrOnRedoMacro" );
        return ErrERRCheck( JET_errLogFileCorrupt );
    }

    LRTYP   lrtyp   = plr->lrtyp;

    Assert( lrtypSplit == lrtyp || lrtypSplit2 == lrtyp || lrtypMerge == lrtyp || lrtypMerge2 == lrtyp || lrtypRootPageMove == lrtyp );
    switch( lrtyp )
    {
        case lrtypSplit:
        case lrtypSplit2:
            err = ErrLGRIRedoSplit( ppib, dbtime );
            break;
        case lrtypMerge:
        case lrtypMerge2:
            err = ErrLGRIRedoMerge( ppib, dbtime );
            break;
        case lrtypRootPageMove:
            err = ErrLGRIRedoRootPageMove( ppib, dbtime );
            break;
        default:
            Assert( fFalse );
            err = ErrERRCheck( JET_errInternalError );
            break;
    }

    //  when all done with the macro, we'll consider all skipped
    //  redo operations as success. We registered all the pages
    //  affected in the redo log map and we can keep recovering.
    if ( errSkipLogRedoOperation == err )
    {
        err = JET_errSuccess;
    }

    return err;
}


ERR LOG::ErrLGRIRedoExtendDB( const LREXTENDDB * const plrdbextension )
{
    ERR        err      = JET_errSuccess;
    const DBID dbid     = plrdbextension->Dbid();
    const PGNO pgnoLast = plrdbextension->PgnoLast();
    
    Assert( pgnoLast != pgnoNull );
    AssertPREFIX( dbid > dbidTemp );
    AssertPREFIX( dbid < dbidMax );

    const IFMP ifmp     = m_pinst->m_mpdbidifmp[ dbid ];

    // if there is no attachment for this database, skip it
    if ( g_ifmpMax == ifmp )
    {
        return JET_errSuccess;
    }
    
    FMP::AssertVALIDIFMP( ifmp );
    FMP * const pfmp = &g_rgfmp[ ifmp ];

    // deferred and skipped attach databases can be ignored
    if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
    {
        return JET_errSuccess;
    }

    const BOOL fLgposLastResizeSupported = ( pfmp->ErrDBFormatFeatureEnabled( JET_efvLgposLastResize ) == JET_errSuccess );
    const LGPOS lgposLastResize = pfmp->Pdbfilehdr()->le_lgposLastResize;
    const BOOL fLgposLastResizeSet = ( CmpLgpos( lgposLastResize, lgposMin ) != 0 );
    const INT icmpLgposLastVsCurrent = CmpLgpos( lgposLastResize, m_lgposRedo );

#ifndef DEBUG
    const BOOL fMaySkipOlderResize = fLgposLastResizeSet && pfmp->FShrinkDatabaseEofOnAttach();
#else
    const BOOL fMaySkipOlderResize = fLgposLastResizeSet;
#endif

    Assert( !fLgposLastResizeSet || fLgposLastResizeSupported );
    {
    OnDebug( PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr() );
    Assert( !fLgposLastResizeSet || CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 );
    }

    //  Note that we are going to redo the resizing even if the lgposLastResize matches the current value stamped
    //  in the header (i.e., icmpLgposLastVsCurrent == 0). That is required for correctness of restoring a backup
    //  that may have been initiated after the physical resizing of the file and the stamping of lgposLastResize to
    //  the header, but before the logical file size is updated post-OE operation. In that case, not replaying a
    //  matching lgposLastResize would leave the file with the smaller (logical) size captured by backup-start.
    if ( fMaySkipOlderResize &&
         fLgposLastResizeSet &&
         ( icmpLgposLastVsCurrent > 0 ) )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
            OSFormat( "%hs: Skipping ExtendDB because we're replaying the initial required range and we haven't reached the last resize yet.", __FUNCTION__ ) );
        return JET_errSuccess;
    }

    //  this might double-count DB extensions, but the exact count is not really
    //  important, as this is used for debugging purposes only.
    CallR( ErrIOResizeUpdateDbHdrCount( ifmp, fTrue /* fExtend */ ) );

    //  resize the database file
    TraceContextScope tcScope( iorpLogRecRedo );
    CallR( ErrIONewSize( ifmp, *tcScope, pgnoLast, 0, JET_bitResizeDatabaseOnlyGrow ) );
    pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );

    if ( fLgposLastResizeSupported )
    {
        CallR( ErrIOResizeUpdateDbHdrLgposLast( ifmp, m_lgposRedo ) );
    }

    //  You might think we could use when we started logging lrtypExtendDB, but unfortunately at that time we
    //  didn't have the extend DB that fixes up the initial DB size after create.  So we must depend upon using
    //  logs of that revision or later to know we're all fixed up.

    if( CmpLogFormatVersion( FmtlgverLGCurrent( this ), fmtlgverInitialDbSizeLoggedMain ) < 0 )
    {
        pfmp->ResetOlderDemandExtendDb();
    }

    return err;
}

ERR LOG::ErrLGRIRedoShrinkDB( const LRSHRINKDB3 * const plrdbshrink )
{
    ERR err = JET_errSuccess;
    const DBID dbid = plrdbshrink->Dbid();
    const PGNO pgnoDbLastNew = plrdbshrink->PgnoLast();
    BOOL fTurnDbmScanBackOn = fFalse; // scanning-thread mode.

    Assert( pgnoDbLastNew != pgnoNull );
    AssertPREFIX( dbid > dbidTemp );
    AssertPREFIX( dbid < dbidMax );

    if ( !FLGRICheckRedoConditionForDb( dbid, m_lgposRedo ) )
    {
        return JET_errSuccess;
    }

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
    FMP::AssertVALIDIFMP( ifmp );
    FMP* const pfmp = &g_rgfmp[ ifmp ];

    // Delete any previously saved shrink archive files.
    if ( !BoolParam( m_pinst, JET_paramFlight_EnableShrinkArchiving ) )
    {
        (void)ErrIODeleteShrinkArchiveFiles( ifmp );
    }

    // We don't need to depend on plrdbshrink->CpgShrunk() for algorithmic correctness because
    // we can always use the actual file size to remove pages from the redo maps. In the past,
    // for example, there was a bug in the do-time shrink code that would produce too low a cgpShrunk
    // if we crashed after removing the space from OE/AE, but before logging lrtypShinkDB. In
    // that scenario, the next time we shrank, cpgShrunk would not account for the previous pages
    // which got shrunk.
    const CPG cpgShrunkLR = plrdbshrink->CpgShrunk();

    PGNO pgnoLastFileSystem = pgnoNull;
    Call( pfmp->ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );

    const PGNO pgnoShrinkFirst = pgnoDbLastNew + 1;
    const PGNO pgnoShrinkLast = pgnoShrinkFirst + cpgShrunkLR - 1;
    const PGNO pgnoShrinkLastOrEof = UlFunctionalMax( pgnoShrinkLast, pgnoLastFileSystem );

    //  if there is a redo map, for each page in the shrunk range we
    //  should consider them reconciled and no longer to be tracked.
    if ( pfmp->PLogRedoMapZeroed() )
    {
        pfmp->PLogRedoMapZeroed()->ClearPgno( pgnoShrinkFirst, pgnoShrinkLastOrEof );
    }
    if ( pfmp->PLogRedoMapBadDbTime() )
    {
        pfmp->PLogRedoMapBadDbTime()->ClearPgno( pgnoShrinkFirst, pgnoShrinkLastOrEof );
    }

    pfmp->SetShrinkIsRunning();
    pfmp->SetPgnoShrinkTarget( pgnoDbLastNew );

    Call( ErrFaultInjection( 60418 ) );

    {
    //  Before LgposLastResize tracking, we used to skip the shrink if we were replaying
    //  the initial required range because we needed to keep pages which had current data
    //  and was not going to be redone because we could have started redoing beyond its
    //  begin transaction. LgposLastResize was introduced because the blanket skipping
    //  was causing problems for full backups (seeds) which got interrupted/resumed or even
    //  databases being recovered from scratch which failed to replay a log that contained
    //  multiple shrink operations. In those cases, we would skip the shrink records in
    //  the required range, which could cause DB divergence errors in later on (one database
    //  could have regrown and would see zeroes in the region which was shrunk/regrown, while the copy
    //  that skipped shrink could potentially see non-zeroes).
    //  Now, we will proceed with replaying the shrink even if in the initial required range if the last
    //  resize LGPOS is older than the current LGPOS.
    const BOOL fInitialReqRange = pfmp->FContainsDataFromFutureLogs();
    const LGPOS lgposLastResize = pfmp->Pdbfilehdr()->le_lgposLastResize;
    const BOOL fLgposLastResizeSet = ( CmpLgpos( lgposLastResize, lgposMin ) != 0 );
    const INT icmpLgposLastVsCurrent = CmpLgpos( lgposLastResize, m_lgposRedo );

    {
    OnDebug( PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr() );
    Assert( !fLgposLastResizeSet || CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 );
    }

    // We can assert this because we quiesce LLR on shrink.
    AssertTrack( fInitialReqRange || !fLgposLastResizeSet || ( icmpLgposLastVsCurrent < 0 ), "MatchingShrinkLgposInNonReqRange" );

    if ( pfmp->FDBMScanOn() )
    {
        pfmp->StopDBMScan();
        fTurnDbmScanBackOn = fTrue;
    }

    const DBTIME dbtimeShrink = plrdbshrink->Dbtime();
    const BOOL fDbTimeShrink = ( ( plrdbshrink->lrtyp == lrtypShrinkDB3 ) && ( dbtimeShrink > 0 ) );
    Expected( !!fDbTimeShrink == ( plrdbshrink->lrtyp == lrtypShrinkDB3 ) );
    if ( fDbTimeShrink && ( dbtimeShrink > pfmp->DbtimeCurrentDuringRecovery() ) )
    {
        pfmp->SetDbtimeCurrentDuringRecovery( dbtimeShrink );
    }

    //  Note that we are going to redo the resizing even if the lgposLastResize matches the current value stamped
    //  in the header (i.e., icmpLgposLastVsCurrent == 0). That is required for correctness of restoring a backup
    //  that may have been initiated after the physical resizing of the file and the stamping of lgposLastResize to
    //  the header, but before the logical file size is updated post-OE operation. In that case, not replaying a
    //  matching lgposLastResize would leave the file with the smaller (logical) size captured by backup-start.
    const BOOL fMustSkipShrinkRedo = ( !fLgposLastResizeSet && fInitialReqRange );  // old check (pre JET_efvLgposLastResize), we skip iff we are in the req range.
    const BOOL fMaySkipShrinkRedo = ( fLgposLastResizeSet && ( icmpLgposLastVsCurrent > 0 ) );  // new check (post JET_efvLgposLastResize), we always skip if we already went through this.
    if ( fMaySkipShrinkRedo || fMustSkipShrinkRedo )
    {
        if ( fMustSkipShrinkRedo )
        {
            OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                OSFormat( "%hs: Skipping ShrinkDB because we're replaying the initial required range and we don't have a lgposLastResize.", __FUNCTION__ ) );
            goto HandleError;
        }

        if ( !fDbTimeShrink )
        {
            OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                OSFormat( "%hs: Skipping ShrinkDB because we have already performed it and we don't have a dbtimeShrink.", __FUNCTION__ ) );
            goto HandleError;
        }
    }

    // Fire DetachDB callback.
    // Backup-from-passive clients have the file-size cached, so, we need to dump them. Once the overall
    // shrink operation is completed, we log lrtypSignalAttachDb so that they can re-attach the database.
    Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );

    if ( fMaySkipShrinkRedo )
    {
        // We'll redo the shrink operation granularly.
        Assert( fDbTimeShrink );
        const PGNO pgnoShrinkLastReset = UlFunctionalMin( pgnoShrinkLast, pgnoLastFileSystem );

        if ( pgnoShrinkLastReset < pgnoShrinkFirst )
        {
            goto HandleError;
        }

        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "%hs: Shrinking (resetting) from pgno=%lu to pgno=%lu",
                              __FUNCTION__, pgnoShrinkFirst, pgnoShrinkLastReset ) );

        Call( ErrLGRIRedoShrinkDBPageReset( ifmp, pgnoShrinkFirst, pgnoShrinkLastReset, dbtimeShrink ) );
    }
    else
    {
        // We'll redo the full shrink truncation.
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "%hs: Shrinking (truncating) to pgno=%lu",
                              __FUNCTION__, pgnoDbLastNew ) );

        Call( ErrLGRIRedoShrinkDBFileTruncation( ifmp, pgnoDbLastNew, cpgShrunkLR ) );
    }
    }

HandleError:
    pfmp->ResetPgnoShrinkTarget();
    pfmp->ResetShrinkIsRunning();

    //  Turn DBM back on.
    if ( fTurnDbmScanBackOn )
    {
        (void)pfmp->ErrStartDBMScan();
    }

    return err;
}

ERR LOG::ErrLGRIRedoShrinkDBPageReset( const IFMP ifmp, const PGNO pgnoShrinkFirstReset, const PGNO pgnoShrinkLastReset, const DBTIME dbtimeShrink )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = &g_rgfmp[ ifmp ];
    BFLatch bfl;
    BOOL fPageLatched = fFalse;
    const CPG cpgPrereadMax = LFunctionalMax( 1, (CPG)( UlParam( JET_paramMaxCoalesceReadSize ) / g_cbPage ) );
    PGNO pgnoPrereadWaypoint = pgnoShrinkFirstReset, pgnoPrereadNext = pgnoShrinkFirstReset;
    PGNO pgnoWriteMin = pgnoShrinkLastReset + 1, pgnoWriteMax = pgnoShrinkFirstReset - 1;
    const BFPriority bfprio = BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate );

    Assert( pfmp->FShrinkIsActive() );
    Assert( ( pgnoShrinkFirstReset != pgnoNull ) && ( pgnoShrinkLastReset != pgnoNull ) );
    Assert( pgnoShrinkFirstReset <= pgnoShrinkLastReset );
    Assert( pfmp->FBeyondPgnoShrinkTarget( pgnoShrinkFirstReset ) );

    for ( PGNO pgno = pgnoShrinkFirstReset; pgno <= pgnoShrinkLastReset; pgno++ )
    {
        // Preread ahead.
        if ( ( pgno >= pgnoPrereadWaypoint ) && ( pgnoPrereadNext <= pgnoShrinkLastReset ) )
        {
            const CPG cpgPrereadCurrent = LFunctionalMin( cpgPrereadMax, (CPG)( pgnoShrinkLastReset - pgnoPrereadNext + 1 ) );
            Assert( cpgPrereadCurrent >= 1 );
            BFPrereadPageRange( ifmp,
                                pgnoPrereadNext,
                                cpgPrereadCurrent,
                                bfprfDefault,
                                bfprio,
                                TcCurr() );
            pgnoPrereadNext += cpgPrereadCurrent;
            pgnoPrereadWaypoint = pgnoPrereadNext - ( cpgPrereadCurrent / 2 );
        }

        // Obtain the page.
        Call( ErrBFWriteLatchPage(
                &bfl,
                ifmp,
                pgno,
                BFLatchFlags( bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfExtensiveChecks ),
                bfprio,
                TcCurr() ) );
        fPageLatched = fTrue;
        const ERR errPageStatus = ErrBFLatchStatus( &bfl );
        const BOOL fPageUninit = ( errPageStatus == JET_errPageNotInitialized );
        if ( ( errPageStatus < JET_errSuccess ) && !fPageUninit )
        {
            Call( errPageStatus );
        }

        // Check if we should reset it.
        CPAGE cpageCheck;
        cpageCheck.ReBufferPage( bfl, ifmp, pgno, bfl.pv, (ULONG)g_cbPage );

        if ( !fPageUninit &&
             ( cpageCheck.Dbtime() != dbtimeShrunk ) &&
             ( cpageCheck.Dbtime() > 0 ) &&
             ( cpageCheck.Dbtime() < dbtimeShrink ) )
        {
            // Reset it.
            BFDirty( &bfl, bfdfDirty, TcCurr() );
            CPAGE cpageWrite;
            cpageWrite.GetShrunkPage( ifmp, pgno, bfl.pv, g_cbPage );
            cpageWrite.UnloadPage();

            pgnoWriteMin = UlFunctionalMin( pgnoWriteMin, pgno );
            pgnoWriteMax = UlFunctionalMax( pgnoWriteMax, pgno );

            // Fix up flush map.
            pfmp->PFlushMap()->SetPgnoFlushType( pgno, CPAGE::pgftUnknown );
        }

        // Unload it.
        cpageCheck.UnloadPage();
        BFWriteUnlatch( &bfl );
        fPageLatched = fFalse;
    }

    // Did we reset at least one page?
    if ( pgnoWriteMin <= pgnoWriteMax )
    {
        // Flush/write pages.
        Call( ErrBFFlush( ifmp, objidNil, pgnoWriteMin, pgnoWriteMax ) );

        // Write out flush map with fixed up states.
        Call( pfmp->PFlushMap()->ErrFlushAllSections( OnDebug( fTrue ) ) );
    }

HandleError:
    if ( fPageLatched )
    {
        BFWriteUnlatch( &bfl );
        fPageLatched = fFalse;
    }

    return err;
}

ERR LOG::ErrLGRIRedoShrinkDBFileTruncation( const IFMP ifmp, const PGNO pgnoDbLastNew, const CPG cpgShrunkLR )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = &g_rgfmp[ ifmp ];

    Assert( pfmp->FShrinkIsActive() );
    Assert( pgnoDbLastNew != pgnoNull );
    Assert( !pfmp->FBeyondPgnoShrinkTarget( pgnoDbLastNew ) );

    //  DBM follower consumes the last scanned page persisted in the header,
    //  and that information is fixed up upon initializing the follower, so
    //  having it already initialized at this point would be a problem.
    Assert( !pfmp->PdbmFollower() );        // follower mode (shrink happens before DBM starts on the active, so this should be off).

    // Write out any snapshot for pages about to be shrunk
    if ( pfmp->FRBSOn() )
    {
        // Capture all shrunk pages as if they need to be reverted to empty pages when RBS is applied.
        // If we already captured a preimage for one of those shrunk pages, the revert to an empty page will be ignored for that page when we apply the snapshot.
        Call( pfmp->PRBS()->ErrCaptureEmptyPages( pfmp->Dbid(), pgnoDbLastNew + 1, cpgShrunkLR ) );
        Call( pfmp->PRBS()->ErrFlushAll() );
    }

    //  this might double-count DB shrinkages, but the exact count is not really
    //  important, as this is used for debugging purposes only.
    Call( ErrIOResizeUpdateDbHdrCount( ifmp, fFalse /* fExtend */ ) );

    pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoDbLastNew ) );
    Call( ErrIONewSize( ifmp, TcCurr(), pgnoDbLastNew, 0, JET_bitResizeDatabaseOnlyShrink ) );
    pfmp->ResetPgnoMaxTracking( pgnoDbLastNew );

    if ( pfmp->ErrDBFormatFeatureEnabled( JET_efvLgposLastResize ) == JET_errSuccess )
    {
        Call( ErrIOResizeUpdateDbHdrLgposLast( ifmp, m_lgposRedo ) );
    }

HandleError:
    return err;
}

ERR LOG::ErrLGRIRedoTrimDB(
    _In_ const LRTRIMDB * const plrdbtrim )
{
    ERR        err      = JET_errSuccess;
    const DBID dbid     = plrdbtrim->Dbid();
    const CPG cpgZeroes = plrdbtrim->CpgZeroLength();
    const PGNO pgnoStartZeroes = plrdbtrim->PgnoStartZeroes();
    const PGNO pgnoEndZeroes = pgnoStartZeroes + cpgZeroes - 1;

    Assert( pgnoStartZeroes != pgnoNull );
    AssertPREFIX( dbid > dbidTemp );
    AssertPREFIX( dbid < dbidMax );

    const IFMP ifmp     = m_pinst->m_mpdbidifmp[ dbid ];

    // if there is no attachment for this database, skip it
    if ( g_ifmpMax == ifmp )
    {
        return JET_errSuccess;
    }

    FMP::AssertVALIDIFMP( ifmp );
    FMP * const pfmp = &g_rgfmp[ ifmp ];

    // deferred and skipped attach databases can be ignored
    if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "%hs: Skipping because pfmp->FSkippedAttach()=%d || pfmp->FDeferredAttach()=%d",
                              __FUNCTION__, pfmp->FSkippedAttach(), pfmp->FDeferredAttach() ) );
        return JET_errSuccess;
    }

    //  If there is a redo map, for each page in the trimmed range we
    //  should consider them reconciled and no longer to be tracked.
    if ( pfmp->PLogRedoMapZeroed() )
    {
        pfmp->PLogRedoMapZeroed()->ClearPgno( pgnoStartZeroes, pgnoEndZeroes );
    }
    if ( pfmp->PLogRedoMapBadDbTime() )
    {
        pfmp->PLogRedoMapBadDbTime()->ClearPgno( pgnoStartZeroes, pgnoEndZeroes );
    }

    //  skip if we're replaying the initial required range because we may need to keep pages
    //  which have current data that is not going to be redone because we might have started
    //  redoing beyond its begin transaction.
    if ( pfmp->FContainsDataFromFutureLogs() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
            OSFormat( "%hs: Skipping because we're replaying the initial required range.", __FUNCTION__ ) );
        return JET_errSuccess;
    }

    //  Do not unintentionally grow the file.
    //  Get the filesize from the pfapi which has the most current value.
    QWORD cbSize;
    CallR( pfmp->Pfapi()->ErrSize( &cbSize, IFileAPI::filesizeLogical ) );
    Assert( cbSize > 0 );

    if ( cbSize <= OffsetOfPgno( pgnoEndZeroes + 1 ) )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "%hs: Skipping because cbSize (%I64d) <= OffsetOfPgno(pgnoEndZeroes+1) [%I64d]",
                              __FUNCTION__, cbSize, OffsetOfPgno( pgnoEndZeroes + 1 ) ) );
        return JET_errSuccess;
    }

    //  Trim the database file.
    OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                OSFormat( "%hs: Trimming pgno=%lu, cpg=%lu",
                          __FUNCTION__, pgnoStartZeroes, cpgZeroes ) );

    //  this might double-count DB trims, but the exact count is not really
    //  important, as this is used for debugging purposes only.
    CallR( ErrSPITrimUpdateDatabaseHeader( ifmp ) );

    const QWORD ibStartZeroesAligned = OffsetOfPgno( pgnoStartZeroes );
    const QWORD cbZeroLength = pfmp->CbOfCpg( cpgZeroes );
    CallR( ErrIOTrim( ifmp, ibStartZeroesAligned, cbZeroLength ) );

    return err;
}

//  ================================================================
ERR LOG::ErrLGRIRedoExtentFreed( const LREXTENTFREED * const plrextentfreed )
//  ================================================================
{
    // This is not logged for all free extent operations, only for those related to deleting a whole space tree.
    Assert( plrextentfreed );

    ERR err                 = JET_errSuccess;
    const DBID dbid         = plrextentfreed->Dbid();

    const IFMP ifmp         = m_pinst->m_mpdbidifmp[ dbid ];

    // if there is no attachment for this database, skip it
    if ( g_ifmpMax == ifmp )
    {
        return JET_errSuccess;
    }

    FMP* const pfmp         = g_rgfmp + ifmp;
    // deferred and skipped attach databases can be ignored
    if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
    {
        return JET_errSuccess;
    }

    // If RBS isn't enabled return success.
    if ( !( pfmp->FRBSOn() ) )
    {
        return JET_errSuccess;
    }

    const PGNO pgnoFirst    = plrextentfreed->PgnoFirst();
    const CPG cpgExtent     = plrextentfreed->CpgExtent();

    Assert( m_fRecoveringMode == fRecoveringRedo );

    for( int i = 0; i < cpgExtent; ++i )
    {
        BFLatch bfl;

        err = ErrBFWriteLatchPage( &bfl, ifmp, pgnoFirst + i, bflfUninitPageOk, BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ), TcCurr() );
        if ( err == JET_errPageNotInitialized )
        {
            // pre image for empty pages cannot be captured, just ignore
            // and if we rollback this page need not be patched since it is empty.
            BFMarkAsSuperCold( ifmp, pgnoFirst + i );
            continue;
        }
        else if ( err == JET_errFileIOBeyondEOF && pfmp->FContainsDataFromFutureLogs() && CmpLgpos( pfmp->Pdbfilehdr()->le_lgposLastResize, m_lgposRedo ) > 0 )
        {
            // pre image for page beyond eof cannot be captured, we must have captured
            // and written it out in the past before shrinking the file.
            BFMarkAsSuperCold( ifmp, pgnoFirst + i );
            continue;
        }
        CallR( err );
        BFMarkAsSuperCold( &bfl );
        BFWriteUnlatch( &bfl );
    }

    // Clear the page from dbtimerevert redo map since the pages in the given extent are being freed.
    // Any future operation on this page should be a new page operation and we shouldn't have to worry about dbtime.
    //
    if ( g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert() )
    {
        g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert()->ClearPgno( pgnoFirst, pgnoFirst + cpgExtent - 1 );
    }

    return JET_errSuccess;
}

//  Updates the le_lGenRecovering in the attached database headers ... note does not write
//  the database header out, relies on ErrLGUpdateGenRequired() to do that.  Debate moving 
//  this function into ErrLGUpdateGenRequired() itself ... problem is that function is called
//  from do-time and/or undo-time in a variety of ways and is already complicated enough.

ERR LOG::ErrLGIUpdateGenRecovering(
    const LONG              lGenRecovering,
    __out_ecount_part( dbidMax, *pcifmpsAttached ) IFMP *rgifmpsAttached,
    _Out_ ULONG *           pcifmpsAttached )
{
    ERR err = JET_errSuccess;

    Assert( m_critCheckpoint.FOwner() );

    Assert( m_fRecovering && fRecoveringRedo == m_fRecoveringMode );

    if ( 0 == CmpLgpos( m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryStartMin, lgposMin ) )
    {
        Assert( CmpLgpos( m_lgposRedo, lgposMin ) != 0 );
        m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryStartMin = m_lgposRedo;
    }

    *pcifmpsAttached = 0;

    FMP::EnterFMPPoolAsWriter();

    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        IFMP ifmp;

        ifmp = m_pinst->m_mpdbidifmp[ dbidT ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP         *pfmpT          = &g_rgfmp[ ifmp ];

        if ( pfmpT->FReadOnlyAttach() )
            continue;

        pfmpT->RwlDetaching().EnterAsReader();

        //  need to check ifmp again. Because the db may be cleaned up
        //  and FDetachingDB is reset, and so is its ifmp in m_mpdbidifmp.
        //
        if (   m_pinst->m_mpdbidifmp[ dbidT ] >= g_ifmpMax
            || !pfmpT->FLogOn()
            || pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach()
            || !pfmpT->Pdbfilehdr() )
        {
            pfmpT->RwlDetaching().LeaveAsReader();
            continue;
        }


        if ( !pfmpT->FAllowHeaderUpdate() )
        {
            pfmpT->RwlDetaching().LeaveAsReader();
            continue;
        }

        //
        //  Update the lGenRecovering field in the DB header
        //
        const LONG  lGenRecoveringOld   = pfmpT->Pdbfilehdr()->le_lGenRecovering;
        pfmpT->PdbfilehdrUpdateable()->le_lGenRecovering = max( lGenRecovering, lGenRecoveringOld );

        if ( lGenRecovering > pfmpT->Pdbfilehdr()->le_lGenMaxRequired )
        {
            if ( pfmpT->FContainsDataFromFutureLogs() )
            {
                // This is the point where we transition from having data from future
                // logs to where we are redoing operations for the first time.
                //
                // In other words, all of the potentially outstanding Shrink or Trim
                // records must have been replayed, and the redo maps should be empty.
                //
                err = ErrLGIVerifyRedoMapForIfmp( ifmp );

                Assert( FMP::FWriterFMPPool() );
                pfmpT->ResetContainsDataFromFutureLogs();

                if ( m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryForwardLogs.lGeneration == 0 )
                {
                    //  If this goes off, then I didn't quite understand the lifecycle, and the sprinted
                    //  stats in the end of Init Event / 105 will be wrong ... this is used as the baseline
                    //  step to know how long the "catch up" recovery phase took.
                    Assert( ( m_pinst->m_isdlInit.FTriggeredStep( eForwardLogBaselineStep ) &&
                                // Note: "SilentRedo" may be done already or it may not, but verbose redo
                                // should definitely not be done, and we definitely shouldn't have reopened
                                // the log for undo.
                                !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoveryVerboseRedoDone ) &&
                                !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoveryReopenDone ) ) ||
                                m_pinst->m_isdlInit.FFailedAllocate() ||
                                // JetExternalRestore() doesn't set up the same things and doesn't log the
                                // 105 event, so doesn't need the previous sequence step triggered.
                                m_fHardRestore );
                    //  Also we might think of moving this to actually be once we crest m_lGenMaxCommitted (or
                    //  maybe even done with le_lGenRecovering), and not m_lgenMaxRequired because that is 
                    //  where we really are on true future logs ... max req is just where we could make this
                    //  copy consistent / begin undo.
                    Assert( m_lgposRedo.lGeneration != 0 );
                    m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryForwardLogs = m_lgposRedo;
                    m_pinst->m_isdlInit.FixedData().sInitData.hrtRecoveryForwardLogs = HrtHRTCount();
                }

                if ( err >= JET_errSuccess )
                {
                    rgifmpsAttached[ *pcifmpsAttached ] = ifmp;
                    (*pcifmpsAttached)++;
                }

                // Free the underlying redo maps.
                pfmpT->FreeLogRedoMaps();

                if ( err >= JET_errSuccess && pfmpT->FNeedUpdateDbtimeBeginRBS() )
                {
                    Assert( pfmpT->FRBSOn() );
                    err = pfmpT->PRBS()->ErrSetDbtimeForFmp( pfmpT, pfmpT->DbtimeBeginRBS() );
                    if ( err >= JET_errSuccess )
                    {
                        pfmpT->ResetNeedUpdateDbtimeBeginRBS();
                    }
                }
            }
        }
        else
        {
            Assert( FMP::FWriterFMPPool() );
            pfmpT->SetContainsDataFromFutureLogs();
        }

        pfmpT->RwlDetaching().LeaveAsReader();

        Call( err );
    }

HandleError:
    FMP::LeaveFMPPoolAsWriter();

    if ( ( err >= JET_errSuccess ) && m_fPendingRedoMapEntries && !FMP::FPendingRedoMapEntries() )
    {
        ResetPendingRedoMapEntries();
    }

    return err;
}

//  Updated the DBSTATE of the database from JET_dbstateDirtyAndPatchedShutdown to
//  JET_dbstateDirtyShutdown when we have replayed through the max required log file.
//  Note does not flush the database header, relies on ErrLGUpdateGenRequired() to do 
//  that.

ERR LOG::ErrLGIUpdatePatchedDbstate(
    const LONG              lGenSwitchingTo )
{
    ERR                     errRet          = JET_errSuccess;

    Assert( m_critCheckpoint.FOwner() );

    Assert( m_fRecovering && fRecoveringRedo == m_fRecoveringMode );

    FMP::EnterFMPPoolAsWriter();

    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        ERR err = JET_errSuccess;
        IFMP ifmp;

        ifmp = m_pinst->m_mpdbidifmp[ dbidT ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP         *pfmpT          = &g_rgfmp[ ifmp ];

        if ( pfmpT->FReadOnlyAttach() )
            continue;

        pfmpT->RwlDetaching().EnterAsReader();

        //  need to check ifmp again. Because the db may be cleaned up
        //  and FDetachingDB is reset, and so is its ifmp in m_mpdbidifmp.
        //
        if (   m_pinst->m_mpdbidifmp[ dbidT ] >= g_ifmpMax
            || !pfmpT->FLogOn()
            || pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach()
            || !pfmpT->Pdbfilehdr() )
        {
            pfmpT->RwlDetaching().LeaveAsReader();
            continue;
        }


        if ( !pfmpT->FAllowHeaderUpdate() )
        {
            pfmpT->RwlDetaching().LeaveAsReader();
            continue;
        }

        //
        //  Update the dbstate IF we've outrun the required range
        //
        if ( JET_dbstateDirtyAndPatchedShutdown == pfmpT->Pdbfilehdr()->Dbstate() )
        {
            //  we have a patched database, next check if we've moved to the right gen to
            //  remove the patched state.

            //  Note: This transistion (>=) is needed because we could have lrtypRcvQuit /
            //  lrtypDetach / lrtypTerm2 before the END of the required log.  We could of
            //  course fix up those LRs to move to dirty as well?  This would be safest.

            if ( ( lGenSwitchingTo >= pfmpT->Pdbfilehdr()->le_lGenMaxRequired ) && pfmpT->FRedoMapsEmpty() )
            {
                //  we've decided we should switch to dirty here, however we must ensure
                //  to be in a crash consistent state to bring up the gen min required for the
                //  database to past the last attach (or could be equal as well).

                //  Update the checkpoint (and the gen min on the attached database(s)).

                err = ErrLGIUpdateCheckpointFile( fTrue, NULL );

                if ( ( err >= JET_errSuccess ) &&
                     ( pfmpT->Patchchk()->lgposAttach.lGeneration <= pfmpT->Pdbfilehdr()->le_lGenMinRequired ) )
                {
                    //  Yeah, we've consumed the entire required range, now we can move this
                    //  DB to a regular dirty state.

                    pfmpT->PdbfilehdrUpdateable()->SetDbstate( JET_dbstateDirtyShutdown, lGenerationInvalid, lGenerationInvalid, NULL, fTrue );
                }
            }
        }

        pfmpT->RwlDetaching().LeaveAsReader();

        if ( errRet == JET_errSuccess )
        {
            errRet = err;
        }
    }

    FMP::LeaveFMPPoolAsWriter();

    return errRet;
}



//  Scan from lgposRedoFrom to end of usable log generations.
//  For each log record, perform operations to redo original operation.
//

ERR LOG::ErrLGRIRedoOperations(
    const LE_LGPOS *ple_lgposRedoFrom,
    BYTE *pbAttach,
    BOOL fKeepDbAttached,
    BOOL* const pfRcvCleanlyDetachedDbs,
    LGSTATUSINFO *plgstat,
    TraceContextScope& tcScope )
{
    ERR                 err                     = JET_errSuccess;
    ERR                 errT                    = JET_errSuccess;
    LR                  *plr;
    BOOL                fLastLRIsQuit           = fFalse;
    BOOL                fShowSectorStatus       = fFalse;

    LGPOS               lgposLastRedoLogRec     = lgposMin;

    double secInCallbackBegin, secInCallbackEnd, secThrottledBegin, secThrottledEnd;
    __int64 cCallbacksBegin, cCallbacksEnd, cThrottledBegin, cThrottledEnd;
    CIsamSequenceDiagLog isdlCurrLog( isdltypeLogFile );
    BYTE rgb[CIsamSequenceDiagLog::cbSingleStepPreAlloc];
    USHORT cLRs[lrtypMax];
    secInCallbackBegin = m_pinst->m_isdlInit.GetCallbackTime( &cCallbacksBegin );
    secThrottledBegin = m_pinst->m_isdlInit.GetThrottleTime( &cThrottledBegin );
    isdlCurrLog.InitSingleStep( isdltypeLogFile, rgb, sizeof(rgb) );
    ZeroMemory( cLRs, sizeof(cLRs) );

    *pfRcvCleanlyDetachedDbs = fTrue;

    OSTrace( JET_tracetagInitTerm, OSFormat( "[Recovery] Begin redo operations. [pinst=%p, fKeep=%d]", m_pinst, fKeepDbAttached ) );

    //  initialize global variable
    //
    m_lgposRedoShutDownMarkGlobal = lgposMin;
    m_lgposRedoLastTerm = lgposMin;
    m_lgposRedoLastTermChecksum = lgposMin;

    //  get the size of the log file

    Assert( m_pLogStream->FLGFileOpened() );
    QWORD   cbSize;
    Call( m_pLogStream->ErrFileSize( &cbSize ) );

    Assert( m_pLogStream->CbSec() > 0 );
    Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
    UINT    csecSize;
    csecSize = UINT( cbSize / m_pLogStream->CbSec() );
    Assert( csecSize > m_pLogStream->CSecHeader() );

    //  setup the log reader

    Call( m_pLogReadBuffer->ErrLReaderInit( csecSize ) );
    Call( m_pLogReadBuffer->ErrReaderEnsureLogFile() );

    //  allocate and setup prereader

    Assert( m_plpreread == pNil );
    AllocR( m_plpreread = new LogPrereader( m_pinst ) );
#ifdef DEBUG
    const CPG cpgGrowth = (CPG)( ( cbSize / 1000 ) / sizeof( PGNO ) );
#else // !DEBUG
    const CPG cpgGrowth = (CPG)( ( cbSize / 100 ) / sizeof( PGNO ) );
#endif // DEBUG
    Expected( cpgGrowth > 0 );
    m_plpreread->LGPInit( dbidMax, max( cpgGrowth, 1 ) );

    AllocR( m_plprereadSuppress = new LogPrereaderDummy() );
    m_plprereadSuppress->LGPInit( dbidMax, max( cpgGrowth, 1 ) );

    Assert( m_pPrereadWatermarks == pNil );
    AllocR( m_pPrereadWatermarks = new CSimpleQueue<LGPOSQueueNode>() );

    // determine the log generation when all databases are expected to be attached when redo starts
    LONG lgenHighAtStartOfRedo;
    Call( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenHighAtStartOfRedo ) );
    if ( m_pLogStream->FCurrentLogExists() )
    {
        lgenHighAtStartOfRedo++;
    }

    LONG lgenHighAttach;
    LoadHighestLgenAttachFromRstmap( &lgenHighAttach );
    lgenHighAtStartOfRedo = max( lgenHighAtStartOfRedo, lgenHighAttach );

    //  scan the log to find traces of corruption before going record-to-record
    //  if any corruption is found, an error will be returned
    BOOL fDummy;
    err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fDummy, fTrue );
    //  remember errors about corruption but don't do anything with them yet
    //  we will go up to the point of corruption and then return the right error
    if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
    {
        errT = err;
    }
    else
    {
        Assert( err < 0 );
        Call( err );
    }

    //  now scan the first record
    //  there should be no errors about corruption since they'll be handled by ErrLGCheckReadLastLogRecordFF
    err = m_pLogReadBuffer->ErrLGLocateFirstRedoLogRecFF( (LE_LGPOS *)ple_lgposRedoFrom, (BYTE **) &plr );
    if ( err == errLGNoMoreRecords )
    {
        //  no records existed in this log -- this means that the log is corrupt
        //  translate to the proper corruption message

        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"1bba029f-b68b-458d-82b8-71fe43b3a0aa" );

        //  what recovery mode are we in?

        // SOFT_HARD: per SG flag should be left here, no big deal
        if ( m_fHardRestore )
        {

            //  we are in hard-recovery mode

            if ( m_pLogStream->GetCurrentFileGen() <= m_lGenHighRestore )
            {

                //  this generation is part of a backup set

                Assert( m_pLogStream->GetCurrentFileGen() >= m_lGenLowRestore );
                Call( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
            }
            else
            {

                //  the current log generation is not part of the backup-set

                Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
            }
        }
        else
        {

            //  we are in soft-recovery mode

            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
    }
    else if ( err != JET_errSuccess )
    {
        Call( err );
    }
    //  we don't expect any warnings, so this must be successful
    CallS( err );

#ifdef DEBUG
//  if ( m_pLogStream->LgposFileEnd().isec )
    {
        LGPOS   lgpos;
        m_pLogReadBuffer->GetLgposOfPbNext( &lgpos );
        // When the header sector (m_csecHeader=1) is corrupted, m_lgposLastRec( m_pLogReadBuffer->LgposFileEnd() ) 
        // points to this header and lgpos also points to this header. 
        Assert( CmpLgpos( &lgpos, &m_pLogReadBuffer->LgposFileEnd() ) <= 0 );
    }
#endif

    if ( plgstat )
    {
        fShowSectorStatus = plgstat->fCountingSectors;
        if ( fShowSectorStatus )
        {
            //  reset byte counts
            //
            plgstat->cSectorsSoFar = ple_lgposRedoFrom->le_isec;
            plgstat->cSectorsExpected = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile;
        }
    }


    m_fLastLRIsShutdown = fFalse;

    do
    {
        FMP     *pfmp;          // for db operations
        DBID    dbid;           // for db operations
        IFMP    ifmp;

        if ( errLGNoMoreRecords == err )
        {
            INT fNSNextStep;

            // super cold any pages we don't expect to see again
            for ( size_t i = 0; i < m_arrayPagerefSupercold.Size(); i++ )
            {
                const PageRef& pageref = m_arrayPagerefSupercold.Entry( i );
                const IFMP ifmpT = m_pinst->m_mpdbidifmp[ pageref.dbid ];
                const PGNO pgnoT = pageref.pgno;

                if ( ifmpT < g_ifmpMax && pgnoT != pgnoNull && pgnoT <= g_rgfmp[ifmpT].PgnoLast() )
                {
                    BFMarkAsSuperCold( ifmpT, pgnoT, bflfDBScan );
                }
            }

            const CArray<PageRef>::ERR errArray = m_arrayPagerefSupercold.ErrSetSize( 0 );
            Assert( errArray == CArray<PageRef>::ERR::errSuccess );

            // we report the progress either if this log took too long to replay (at least 5 seconds) or
            // if the control callback says so ...
            //

            // Gather throttling/callback delta from the INST timing sequence (depends on INST timing sequence
            // number not changing since we got the beginning number)
            secInCallbackEnd = m_pinst->m_isdlInit.GetCallbackTime( &cCallbacksEnd );
            secThrottledEnd = m_pinst->m_isdlInit.GetThrottleTime( &cThrottledEnd );
            isdlCurrLog.AddCallbackTime( secInCallbackEnd - secInCallbackBegin, cCallbacksEnd - cCallbacksBegin );
            isdlCurrLog.AddThrottleTime( secThrottledEnd - secThrottledBegin, cThrottledEnd - cThrottledBegin );
            isdlCurrLog.Trigger( 1 );
            __int64 usecTime = isdlCurrLog.UsecTimer( eSequenceStart, 1 );
            BOOL fCallback = ( ErrLGNotificationEventConditionCallback( m_pinst, STATUS_REDO_ID ) >= JET_errSuccess );
            if ( ( usecTime / 1000000 - ( secThrottledEnd - secThrottledBegin ) ) >= 5 || fCallback )
            {
                WCHAR wszLogStats[500], wszLR[16], wszCount[8];

                if ( fCallback && !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoverySilentRedoDone ) )
                {
                    m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoverySilentRedoEnd = m_lgposRedo;
                    m_pinst->m_isdlInit.Trigger( eInitLogRecoverySilentRedoDone );
                }

                USHORT maxCount = 0;
                BYTE maxLR = 0;
                for ( BYTE i=0; i<lrtypMax; i++ )
                {
                    // No one cares if these LRs are the most common, we want to know what operation it is
                    if ( i == lrtypBegin || i == lrtypCommit || i == lrtypBegin0 || i == lrtypCommit0 )
                    {
                        continue;
                    }
                    if ( cLRs[i] > maxCount )
                    {
                        maxCount = cLRs[i];
                        maxLR = i;
                    }
                }

                isdlCurrLog.SprintTimings( wszLogStats, sizeof( wszLogStats ) );
                OSStrCbFormatW( wszCount, sizeof(wszCount), L"%hd", maxCount );
                OSStrCbFormatW( wszLR, sizeof(wszLR), L"%hS", SzLrtyp( maxLR ) );

                const WCHAR * rgwsz [] = { m_pLogStream->LogName(), wszLogStats, wszLR, wszCount };
                C_ASSERT( _countof( rgwsz ) == 4 );

                //  log redo progress.
                UtilReportEvent(
                        eventInformation,
                        LOGGING_RECOVERY_CATEGORY,
                        STATUS_REDO_ID,
                        _countof( rgwsz ),
                        rgwsz,
                        0,
                        NULL,
                        m_pinst );
            }
            isdlCurrLog.TermSequence();

            // Move m_lgposRedo past the last log record
            m_lgposRedo = m_pLogReadBuffer->LgposFileEnd();
            if ( fLastLRIsQuit )
            {
                m_lgposRedoLastTermChecksum = m_lgposRedo;
            }

            //  if we had a corruption error on this generation, do not process other generations

            if ( errT != JET_errSuccess )
            {
                goto Done;
            }

            // check for the potential stopping point. That point might be 
            // the current log file so don't even try to switch to the
            // next one. 
            // It would be easier to allow switching and exit just before we would redo
            // the first log record (existing code below to check agains m_lgposRedo) BUT 
            // if we allow switching we might hit a corrupted log because
            // it might be in the middle of file copy (for log shipping).
            //
            // we the stop position is still in the current log now that we are done with
            // this log, stop here
            if ( m_lgposRecoveryStop.lGeneration == m_lgposRedo.lGeneration )
            {
                // we hit already the desired log stop position


                Error( ErrLGISetQuitWithoutUndo( ErrERRCheck( JET_errRecoveredWithoutUndo ) ) );
            }

            //  bring in the next generation
            err = ErrLGRedoFill( &plr, fLastLRIsQuit, &fNSNextStep );
            //  remember errors about corruption but don't do anything with them yet
            //  we will go up to the point of corruption and then return the right error
            if ( FErrIsLogCorruption( err ) )
            {
                errT = err;

                //  make sure we process this log generation (up to the point of corruption)

                fNSNextStep = fNSGotoCheck;
            }
            else if ( err == errLGNoMoreRecords )
            {
                //  no records existed in this log because ErrLGLocateFirstRedoLogRecFF returned this error
                //      this means that the log is corrupt -- setup the proper corruption message

                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"f25f16bb-40ce-48a6-a31d-5aa98cf2112a" );

                //  what recovery mode are we in?

                // SOFT_HARD: per SG flag should be left here, no big deal
                if ( m_fHardRestore )
                {

                    //  we are in hard-recovery mode

                    if ( m_pLogStream->GetCurrentFileGen() <= m_lGenHighRestore )
                    {

                        //  this generation is part of a backup set

                        Assert( m_pLogStream->GetCurrentFileGen() >= m_lGenLowRestore );
                        errT = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
                    }
                    else
                    {

                        //  the current log generation is not part of the backup-set

                        errT = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
                    }
                }
                else
                {

                    //  we are in soft-recovery mode

                    errT = ErrERRCheck( JET_errLogFileCorrupt );
                }

                //  make sure we process this log generation

                fNSNextStep = fNSGotoCheck;
            }
            else if ( err != JET_errSuccess )
            {
                Assert( err < 0 );
                Call( err );
            }

            (void)ErrIOUpdateCheckpoints( m_pinst );

            switch( fNSNextStep )
            {
                case fNSGotoDone:
                    goto Done;

                case fNSGotoCheck:

                    // if the log is corrupted we don't want to update the header
                    // as in general it will mean that they will need to re-restore
                    // the database file 
                    // Still, someone could force the reply up to a certain log 
                    // position in order to get in the corrupted log as far as 
                    // possible (like to the last detach)
                    // 
                    if ( FErrIsLogCorruption( errT ) )
                    {
                        const LONG lGeneration = m_pLogStream->GetCurrentFileGen();
                        
                        // if this is not the gneration we need to stop at 
                        // (this includes the case where there is no stop position 
                        // which would be m_lgposRecoveryStop.lGeneration == 0) 
                        // 
                        if ( m_lgposRecoveryStop.lGeneration != lGeneration ||
                            // OR we stop at the current log generation  
                            FLGRecoveryLgposStopLogGeneration( ) )
                        {
                            Error( ErrERRCheck( errT ) );
                        }
                    }

                    OnNonRTM( m_lgposRedoPreviousLog = m_lgposRedo );

                    // set the recovery redo mode to new logs if we are out of the required range

                    {
                        // No locking needed since this is recovery thread
                        BOOL fContainsDataFromFutureLogs = fFalse;
                        for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
                        {
                            const IFMP ifmpT = m_pinst->m_mpdbidifmp[dbidT];
                            if ( ifmpT < g_ifmpMax && g_rgfmp[ifmpT].FContainsDataFromFutureLogs() )
                            {
                                fContainsDataFromFutureLogs = fTrue;
                                break;
                            }
                        }

                        if ( tcScope->iorReason.Iort() == iortRecoveryRedo && !fContainsDataFromFutureLogs )
                        {
                            tcScope->iorReason.SetIort( iortRecoveryRedoNewLogs );
                        }
                    }

                    // we will get the current lgpos right here
                    // so we can compare it with the m_lgposRecoveryStop
                    // before doing the EventLog
                    //
                    Call( ErrGetLgposRedoWithCheck() );

                    secInCallbackBegin = m_pinst->m_isdlInit.GetCallbackTime( &cCallbacksBegin );
                    secThrottledBegin = m_pinst->m_isdlInit.GetThrottleTime( &cThrottledBegin );
                    isdlCurrLog.InitSingleStep( isdltypeLogFile, rgb, sizeof(rgb) );
                    ZeroMemory( cLRs, sizeof(cLRs) );

                    if ( !plgstat )
                    {
                        if ( g_fRepair )
                            printf( " Recovering Generation %d.\n", LONG( m_pLogStream->GetCurrentFileGen() ) );
                    }
                    else
                    {
                        JET_SNPROG  *psnprog = &(plgstat->snprog);
                        ULONG       cPercentSoFar;

                        plgstat->cGensSoFar += 1;
                        cPercentSoFar = (ULONG)
                            ((plgstat->cGensSoFar * 100) / plgstat->cGensExpected);

                        // cPercentSoFar can actually exceed 100% if new logfiles appear while we are
                        // replaying. This is OK in the recovery-without-undo case, which doesn't require
                        // and eXX.log. In that case, cap the percentage complete at 100%
                        cPercentSoFar = min( cPercentSoFar, 100 );

                        Assert( cPercentSoFar >= psnprog->cunitDone );
                        if ( cPercentSoFar > psnprog->cunitDone )
                        {
                            psnprog->cunitDone = cPercentSoFar;
                            (plgstat->pfnCallback)( JET_snpRestore,
                                JET_sntProgress, psnprog, plgstat->pvCallbackContext );
                        }

                        if ( fShowSectorStatus )
                        {
                            /*  reset byte counts
                            /**/
                            plgstat->cSectorsSoFar = 0;
                            plgstat->cSectorsExpected = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile;
                        }
                    }
                    goto ProcessNextRec;
            }

            /*  should never get here
            /**/
            Assert( fFalse );
        }

        Call( ErrGetLgposRedoWithCheck() );

ProcessNextRec:

        TLS * const ptls = Ptls();
        ptls->threadstats.cLogRecord++;
        ptls->threadstats.cbLogRecord += (ULONG)CbLGSizeOfRec( plr );

        if ( plr->lrtyp == lrtypNOP2 )
        {
            LGAddWastage( CbLGSizeOfRec( plr ) );
        }
        else
        {
            LGAddUsage( CbLGSizeOfRec( plr ) );
            cLRs[ plr->lrtyp ]++;
        }

        tcScope->iorReason.SetIors( IorsFromLr( plr ) );

#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY
        if ( m_fPreread && !m_fDumpingLogs )
        {
            //  Consume watermarks.
            LGPOSQueueNode* plgposQueueNode = NULL;
            OnDebug( LGPOS lgposPrev = lgposMax );
            while ( ( ( plgposQueueNode = m_pPrereadWatermarks->Head() ) != NULL ) &&
                ( CmpLgpos( plgposQueueNode->m_lgpos, m_lgposRedo ) < 0 ) )
            {
                ExpectedSz( !CmpLgpos( lgposPrev, lgposMax ) || !CmpLgpos( lgposPrev, plgposQueueNode->m_lgpos ), "Multiple preread LGPOS's must be the same (LR types that touch multiple pages)." );
                OnDebug( lgposPrev = plgposQueueNode->m_lgpos );
                
                delete m_pPrereadWatermarks->RemovePrevMost( OffsetOf( LGPOSQueueNode, m_plgposNext ) );
            }

            if ( m_pPrereadWatermarks->CElements() < (UINT)UlParam( m_pinst, JET_paramPrereadIOMax ) )
            {
                Call( ErrLGIPrereadPages( fFalse ) );
            }
        }

#endif  //  MINIMAL_FUNCTIONALITY

        //  if initial DbList has not yet been created,
        //  it must be because we haven't reached
        //  the DbList (or Init) log record yet
        Assert( !m_fNeedInitialDbList
            || lrtypChecksum == plr->lrtyp
            || lrtypInit2 == plr->lrtyp );

        // update genMaxReq for the databases to avoid the following scenario:
        // - backup set 3-5, play forward 6-10
        // - replay up to 8 and crash
        // - delete logs 7-10
        // - run hard recovery w/o restoring the files
        // we also want to update for soft recovery if we have an older than logs database
        // (like during soft recovery of a offline backup or snapshot)
        if ( m_lgposRedo.lGeneration > lgposLastRedoLogRec.lGeneration )
        {
            IFMP rgifmpsAttached[ dbidMax ];
            ULONG cifmpsAttached = 0;

            // Currently flush log if flush-tip falls too far behind waypoint depth. We could also do it time based etc.
            if ( m_lgposRedo.lGeneration > m_lgposFlushTip.lGeneration + LLGElasticWaypointLatency() )
            {
                BOOL fFlushed = fFalse;
                Call( m_pLogStream->ErrLGFlushLogFileBuffers( iofrLogMaxRequired, &fFlushed ) );
                if ( fFlushed )
                {
                    // During recovery redo, consider full log flushed
                    LGPOS lgposNextFlushTip;
                    lgposNextFlushTip.lGeneration = m_lgposRedo.lGeneration;
                    lgposNextFlushTip.isec = lgposMax.isec;
                    lgposNextFlushTip.ib   = lgposMax.ib;
                    LGSetFlushTip( lgposNextFlushTip );
                }
            }

            // this code fires every time we roll a log.  Note this code actually fires 
            // twice, not sure why we're in here twice.
            m_critCheckpoint.Enter();

            err = ErrLGIUpdatePatchedDbstate( m_lgposRedo.lGeneration );
            if ( err < JET_errSuccess )
            {
                m_critCheckpoint.Leave();
                Call( err );
            }

            err = ErrLGIUpdateGenRecovering( m_lgposRedo.lGeneration, rgifmpsAttached, &cifmpsAttached );
            if ( err < JET_errSuccess )
            {
                m_critCheckpoint.Leave();
                Call( err );
            }

            PERFOpt( cLGLogFileCurrentGeneration.Set( m_pinst, m_lgposRedo.lGeneration ) );

            LOGTIME tmCreate;
            m_pLogStream->GetCurrentFileGen( &tmCreate );
            Assert( m_lgposRedo.lGeneration != 0 );
            err = ErrLGUpdateGenRequired(
                        m_pinst->m_pfsapi,
                        0, // pass 0 to preserve the existing value
                        0, // pass 0 to preserve the existing value
                        m_lgposRedo.lGeneration,
                        tmCreate,
                        NULL );

            m_critCheckpoint.Leave();
            Call( err );

            // During recovery, nothing else can change the attached state of the db, so safe to do the callback outside
            // the lock
            for ( ULONG i=0 ; i<cifmpsAttached; i++ )
            {
                Call( ErrLGDbAttachedCallback( m_pinst, &g_rgfmp[rgifmpsAttached[i]] ) );
            }

            // if we are starting to replay a log generation beyond the last consistent gen of all attachments marked
            // as fail fast, then check for any deferred attachment.  if we find any, then we will fail recovery.
            if ( m_lgposRedo.lGeneration > lgenHighAtStartOfRedo )
            {
                Call( ErrLGRICheckForAttachedDatabaseMismatchFailFast( m_pinst ) );
                lgenHighAtStartOfRedo = lMax;
            }
        }

        //  Keep track of last LR to see if it is shutdown mark
        //  Skip those lr that does nothing material. Do not change
        //  m_fLastLRIsShutdown if LR is debug only log record.

        switch ( plr->lrtyp )
        {
        case lrtypNOP2:
            if ( fLastLRIsQuit )
            {
                m_lgposRedoLastTermChecksum = m_lgposRedo;
            }
            __fallthrough;
        case lrtypNOP:
            continue;

            // This may not be optimally efficient to put this
            // so high on the log record processing.
        case lrtypChecksum:
            continue;

        case lrtypTrace:                    /* Debug purpose only log records. */
        case lrtypJetOp:
        case lrtypForceWriteLog:
        case lrtypForceLogRollover:
            break;

        case lrtypRecoveryUndo:
        case lrtypRecoveryUndo2:
            if ( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
            {
                m_pLogStream->SetLgposUndoForOB0( m_lgposRedo );
            }
            break;

        case lrtypFullBackup:               /* Debug purpose only log records */
            m_pinst->m_pbackup->BKSetLgposFullBackup( m_lgposRedo );
            m_pinst->m_pbackup->BKSetLogsTruncated( fFalse );
            break;

        case lrtypIncBackup:                /* Debug purpose only log records */
            m_pinst->m_pbackup->BKSetLgposIncBackup( m_lgposRedo );
            break;

        case lrtypBackup:
        {
            LRLOGBACKUP_OBSOLETE * plrlb = (LRLOGBACKUP_OBSOLETE *) plr;
            if ( plrlb->FFull() )
            {
                m_pinst->m_pbackup->BKSetLgposFullBackup( m_lgposRedo );
            }
            else if ( plrlb->FIncremental() )
            {
                m_pinst->m_pbackup->BKSetLgposIncBackup( m_lgposRedo );
            }
            else if ( plrlb->FSnapshotStart() )
            {
                AssertSz( fFalse, "We never used the non-VSS snapshot in a release product" );
            }
            else if ( plrlb->FSnapshotStop() )
            {
                AssertSz( fFalse, "We never used the non-VSS snapshot in a release product" );
            }
            else if ( plrlb->FTruncateLogs() )
            {
                m_pinst->m_pbackup->BKSetLogsTruncated( fTrue );
            }
            else if ( plrlb->FOSSnapshot() )
            {
                ;
            }
            else if ( plrlb->FOSSnapshotIncremental() )
            {
                ;
            }
            else
            {
                Assert ( fFalse );
            }
            break;
        }
        case lrtypBackup2:
        {
            LRLOGBACKUP * plrlb = (LRLOGBACKUP *) plr;
            if ( plrlb->eBackupPhase == LRLOGBACKUP::fLGBackupPhaseUpdate )
            {
                Call( ErrLGRIRedoBackupUpdate( plrlb ) );
            }
            break;
        }

        case lrtypShutDownMark:         /* Last consistency point */
            m_lgposRedoShutDownMarkGlobal = m_lgposRedo;
            m_fLastLRIsShutdown = fTrue;
            break;

        default:
        {
            m_fLastLRIsShutdown = fFalse;

            //  Check the LR that does the real work from here:

            switch ( plr->lrtyp )
            {
            case lrtypMacroBegin:
            {
                PIB *ppib;

                LRMACROBEGIN *plrMacroBegin = (LRMACROBEGIN *) plr;
                Call( ErrLGRIPpibFromProcid( plrMacroBegin->le_procid, &ppib ) );

                Assert( !ppib->FMacroGoing( plrMacroBegin->le_dbtime ) );
                Call( ppib->ErrSetMacroGoing( plrMacroBegin->le_dbtime ) );
                break;
            }

            case lrtypMacroCommit:
            case lrtypMacroAbort:
            {
                PIB         *ppib;
                LRMACROEND  *plrmend = (LRMACROEND *) plr;
                DBTIME      dbtime = plrmend->le_dbtime;

                Call( ErrLGRIPpibFromProcid( plrmend->le_procid, &ppib ) );

                //  if it is commit, redo all the recorded log records,
                //  otherwise, throw away the logs
                //
                if ( lrtypMacroCommit == plr->lrtyp && ppib->FAfterFirstBT() )
                {
                    Call( ErrLGRIRedoMacroOperation( ppib, dbtime ) );
                }

                //  disable MacroGoing
                //
                ppib->ResetMacroGoing( dbtime );

                break;
            }

            case lrtypInit2:
            case lrtypInit:
            {
                /*  start mark the jet init. Abort all active seesions.
                /**/
                LRINIT2  *plrstart = (LRINIT2 *)plr;

                LGRITraceRedo( plr );
                m_pinst->m_isdlInit.FixedData().sInitData.cReInits++;

                if ( !m_fAfterEndAllSessions )
                {
                    Call( ErrLGRIEndAllSessions( fFalse, fKeepDbAttached, ple_lgposRedoFrom, pbAttach ) );
                    m_fAfterEndAllSessions = fTrue;
                    m_fNeedInitialDbList = fFalse;
#ifdef DEBUG
                    m_lgposRedoShutDownMarkGlobal = lgposMin;
#endif
                    m_lgposRedoLastTerm = lgposMin;
                    m_lgposRedoLastTermChecksum = lgposMin;
                }

                // done with all changes from old format, do not need to keep
                // sector size from old format around
                if ( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
                {
                    m_pLogStream->ResetOldLrckVersionSecSize();
                }

                /*  Check Init session for hard restore only.
                 */
                Call( ErrLGRIInitSession(
                            &plrstart->dbms_param,
                            pbAttach,
                            redoattachmodeInitLR ) );
                m_fAfterEndAllSessions = fFalse;
            }
                break;

            case lrtypRecoveryQuit:
            case lrtypRecoveryQuit2:
            case lrtypRecoveryQuitKeepAttachments:
            case lrtypTerm:
            case lrtypTerm2:
                /*  all records are re/done. all rce entries should be gone now.
                /**/
#ifdef DEBUG
                {
                PIB *ppib = NULL;
                for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
                {
                    RCE *prceT = ppib->prceNewest;
                    while ( prceT != prceNil )
                    {
                        Assert( prceT->FOperNull() );
                        prceT = prceT->PrcePrevOfSession();
                    }
                }
                }
#endif

                /*  quit marks the end of a normal run. All sessions
                /*  have ended or must be forced to end. Any further
                /*  sessions will begin with a BeginT.
                /**/
#ifdef DEBUG
                m_fDBGNoLog = fTrue;
#endif
                /*  set m_lgposLogRec such that later start/shut down
                 *  will put right lgposConsistent into dbfilehdr
                 *  when closing the database.
                 */
                if ( !m_fAfterEndAllSessions )
                {
                    *pfRcvCleanlyDetachedDbs = ( plr->lrtyp != lrtypRecoveryQuitKeepAttachments );
                    
                    Call( ErrLGRIEndAllSessions( fFalse, !( *pfRcvCleanlyDetachedDbs ), ple_lgposRedoFrom, pbAttach ) );
                    m_fAfterEndAllSessions = fTrue;
#ifdef DEBUG
                    m_lgposRedoShutDownMarkGlobal = lgposMin;
#endif
                    m_lgposRedoLastTerm = m_lgposRedo;
                    m_lgposRedoLastTermChecksum = m_lgposRedo;
                    m_pLogStream->AddLgpos( &m_lgposRedoLastTermChecksum, sizeof( LRTERMREC2 ) );
                }
                // done with all changes from old format, do not need to keep
                // sector size from old format around
                if ( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
                {
                    m_pLogStream->ResetOldLrckVersionSecSize();
                }

                fLastLRIsQuit = fTrue;
                continue;

            /****************************************************/
            /*  Database Operations                          */
            /****************************************************/
            case lrtypCreateDB:
            {
                PIB             *ppib;
                REDOATTACH      redoattach;
                LRCREATEDB      *plrcreatedb    = (LRCREATEDB *)plr;

                dbid = plrcreatedb->dbid;
                Assert( dbid != dbidTemp );

                LGRITraceRedo(plr);

                Call( ErrLGRIPpibFromProcid( plrcreatedb->le_procid, &ppib ) );


                // we replay create dbs only on soft recovery so no point
                // in checking the map on hard recovery

                // SOFT_HARD: leave like this!
                if ( !m_fHardRestore && m_irstmapMac > 0 )
                {
                    CAutoWSZ    wszDbName;

                    if ( plrcreatedb->FUnicodeNames( ) )
                    {
                        UnalignedLittleEndian< WCHAR > * lszDbName = plrcreatedb->WszUnalignedNames( );

                        Call( wszDbName.ErrSet( lszDbName ) );

                        err = ErrReplaceRstMapEntryByName( wszDbName, &plrcreatedb->signDb );
                        if ( JET_errFileNotFound == err )
                        {
                            //  database not in restore map, so it won't be restored
                            //
                            err = JET_errSuccess;
                        }
                        Call( err );
                    }
                    else
                    {
                        CHAR * lszDbName = plrcreatedb->SzNames( );
                        Call( wszDbName.ErrSet( lszDbName ) );
                        err = ErrReplaceRstMapEntryByName( wszDbName, &plrcreatedb->signDb );
                        if ( JET_errFileNotFound == err )
                        {
                            //  database not in restore map, so it won't be restored
                            //
                            err = JET_errSuccess;
                        }
                        Call( err );
                    }
                }


                // set-up the FMP
                INT irstmap = -1;
                {
                // build an ATTACHINFO based on this log record
                ATTACHINFO *    pAttachInfo     = NULL;
                const ULONG     cbAttachInfo    = sizeof(ATTACHINFO) + plrcreatedb->CbPath();

                Alloc( pAttachInfo = static_cast<ATTACHINFO *>( PvOSMemoryHeapAlloc( cbAttachInfo ) ) );

                memset( pAttachInfo, 0, cbAttachInfo );
                pAttachInfo->SetDbid( plrcreatedb->dbid );
                pAttachInfo->SetCbNames( plrcreatedb->CbPath() );
                pAttachInfo->SetDbtime( 0 );
                pAttachInfo->SetObjidLast( objidNil );
                pAttachInfo->SetCpgDatabaseSizeMax( plrcreatedb->le_cpgDatabaseSizeMax );
                pAttachInfo->le_lgposAttach = m_lgposRedo;
                pAttachInfo->le_lgposConsistent = lgposMin;
                memcpy( &pAttachInfo->signDb, &plrcreatedb->signDb, sizeof(SIGNATURE) );

                // We will keep ASCII is needed at this point
                // becuase the ATTACHINFO has support for both
                // and we need to deal with both in the below functions
                // anyway
                if ( plrcreatedb->FUnicodeNames( ) )
                {
                    pAttachInfo->SetFUnicodeNames( );
                }
                plrcreatedb->GetNames( (CHAR*)pAttachInfo->szNames );

                if ( plrcreatedb->FSparseEnabledFile() )
                {
                    pAttachInfo->SetFSparseEnabledFile();
                }

                err = ErrLGRIPreSetupFMPFromAttach( ppib, pAttachInfo );
                if ( err >= JET_errSuccess )
                {
                    IFMP ifmpDuplicate;

                    err = ErrLGRISetupFMPFromAttach( ppib, pAttachInfo, plgstat, &ifmpDuplicate, &irstmap );
                    if ( err == JET_errDatabaseDuplicate )
                    {
                        // if we see a duplicate database during recovery and it is for the same dbid and there is a
                        // database in the restore map that is flagged as overwrite on create then detach the database
                        // and allow the create to be processed.  this allows log shipping to replay a delete/create
                        // database sequence in the log stream that otherwise would have failed purely due to the keep
                        // cache alive for recovery feature
                        FMP* pfmpDuplicate = &g_rgfmp[ ifmpDuplicate ];
                        if ( pfmpDuplicate->Dbid() == dbid && irstmap >= 0 && ( m_rgrstmap[irstmap].grbit & JET_bitRestoreMapOverwriteOnCreate ) )
                        {
                            err = ErrIsamDetachDatabase( (JET_SESID)ppib, NULL, pfmpDuplicate->WszDatabaseName() );
                            if ( err >= JET_errSuccess )
                            {
                                err = ErrLGRISetupFMPFromAttach( ppib, pAttachInfo, plgstat, NULL, &irstmap );
                            }
                        }
                    }
                }
                OSMemoryHeapFree( pAttachInfo );
                Call( err );
                }

                ifmp = m_pinst->m_mpdbidifmp[ dbid ];
                FMP::AssertVALIDIFMP( ifmp );
                pfmp = &g_rgfmp[ ifmp ];

                if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
                {
                    break;
                }

                const BOOL  fDBPathValid    = ( ErrUtilDirectoryValidate( m_pinst->m_pfsapi, pfmp->WszDatabaseName() ) >= JET_errSuccess );
                const BOOL  fDBFileMissing  = ( !fDBPathValid || ( ErrUtilPathExists( m_pinst->m_pfsapi, pfmp->WszDatabaseName() ) < JET_errSuccess ) );

                if ( fDBPathValid && fDBFileMissing )
                {
                    //  database missing, so recreate it
                    redoattach = redoattachCreate;
                }
                else
                {
                    Assert( !fDBPathValid || !fDBFileMissing );

                    Assert( !pfmp->FReadOnlyAttach() );
                    Call( ErrLGRICheckAttachedDb(
                                ifmp,
                                NULL,
                                &redoattach,
                                redoattachmodeCreateDbLR ) );
                    
                    Assert( NULL != pfmp->Pdbfilehdr() || redoattachCreate == redoattach );
                    // if missing, it is deferred
                    Assert( !fDBFileMissing || redoattachDefer == redoattach );
                }

                switch( redoattach )
                {
                    case redoattachCreate:
                    {
                        const JET_SETDBPARAM* const rgsetdbparam = ( irstmap >= 0 ) ? m_rgrstmap[irstmap].rgsetdbparam : NULL;
                        const ULONG csetdbparam = ( irstmap >= 0 ) ? m_rgrstmap[irstmap].csetdbparam : 0;

                        //  we've already pre-determined (in ErrLGRICheckRedoCreateDb())
                        //  that any existing database needs to be overwritten, so
                        //  it's okay to unequivocally pass in JET_bitDbOverwriteExisting
                        if ( usDAECreateDbVersion == plrcreatedb->UsVersion() &&
                             usDAECreateDbUpdateMajor >= plrcreatedb->UsUpdateMajor() )
                        {
                            Call( ErrLGRIRedoCreateDb(
                                        ppib,
                                        ifmp,
                                        dbid,
                                        plrcreatedb->le_grbit | JET_bitDbOverwriteExisting,
                                        plrcreatedb->FSparseEnabledFile(),
                                        &plrcreatedb->signDb,
                                        rgsetdbparam,
                                        csetdbparam ) );
                        }
                        else if ( usDAECreateDbVersion_Implicit != plrcreatedb->UsVersion() ||
                                  usDAECreateDbUpdate_Implicit < plrcreatedb->UsUpdateMajor() ||
                                  ( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor == ulLGVersionMajor_Win7 &&
                                    m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor == ulLGVersionMinor_Win7 &&
                                    m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor < ulLGVersionUpdateMajor_Win7 ) )
                        {
                            // we have a CreateDB with an older engine version.
                            // It is a problem because the database operations
                            // during this are not logged so if something
                            // changed in the creation process between version
                            // (and it did), we will generate a different image 
                            // (and have other issues during replay of the next
                            // operations logged)
                            // Note that at this point we know how to replay
                            // win7 CreateDBs and later

                            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagUnrecoverable, L"ea07d1c4-4ed1-4c73-9b88-ee4f4a3e06c9" );
                            Call( ErrERRCheck( JET_errInvalidCreateDbVersion ) );
                        }
                        else
                        {
                            Call( ErrLGRIRedoCreateDb(
                                        ppib,
                                        ifmp,
                                        dbid,
                                        plrcreatedb->le_grbit | JET_bitDbOverwriteExisting | bitCreateDbImplicitly,
                                        plrcreatedb->FSparseEnabledFile(),
                                        &plrcreatedb->signDb,
                                        rgsetdbparam,
                                        csetdbparam ) );
                        }
                    }
                        break;

                    case redoattachNow:
                        Assert( !pfmp->FReadOnlyAttach() );
                        Call( ErrLGRIRedoAttachDb(
                                    ifmp,
                                    plrcreatedb->le_cpgDatabaseSizeMax,
                                    plrcreatedb->FSparseEnabledFile(),
                                    redoattachmodeCreateDbLR ) );
                        break;

                    default:
                        Assert( fFalse );   //  should be impossible, but as a firewall, set to defer the attachment
                    case redoattachDefer:
                    case redoattachDeferConsistentFuture:
                    case redoattachDeferAccessDenied:
                        Assert( !pfmp->FReadOnlyAttach() );
                        LGRISetDeferredAttachment( ifmp, (redoattach == redoattachDeferConsistentFuture), (redoattach == redoattachDeferAccessDenied) );
                        break;
                }
            }
                break;

            case lrtypCreateDBFinish:
            {
                PIB *ppib;
                LRCREATEDBFINISH *plrcreatedbfinish = (LRCREATEDBFINISH *)plr;

                dbid = plrcreatedbfinish->dbid;
                Assert( dbid != dbidTemp );

                LGRITraceRedo( plr );

                Call( ErrLGRIPpibFromProcid( plrcreatedbfinish->le_procid, &ppib ) );

                ifmp = m_pinst->m_mpdbidifmp[ dbid ];
                if( g_ifmpMax == ifmp )
                {
                    break;
                }
                FMP::AssertVALIDIFMP( ifmp );
                pfmp = &g_rgfmp[ ifmp ];

                if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
                {
                    break;
                }

                if ( pfmp->Pdbfilehdr()->Dbstate() != JET_dbstateJustCreated )
                {
                    break;
                }

                Call( ErrDBCreateDBFinish( ppib, ifmp, 0 ) );
            }
                break;

            case lrtypAttachDB:
            {
                PIB         *ppib;
                REDOATTACH  redoattach;
                LRATTACHDB  *plrattachdb    = (LRATTACHDB *)plr;

                dbid = plrattachdb->dbid;
                Assert( dbid != dbidTemp );

                LGRITraceRedo( plr );

                Call( ErrLGRIPpibFromProcid( plrattachdb->le_procid, &ppib ) );

                // set-up the FMP
                {
                // build an ATTACHINFO based on this log record
                ATTACHINFO *    pAttachInfo     = NULL;
                const ULONG     cbAttachInfo    = sizeof(ATTACHINFO) + plrattachdb->CbPath();

                Alloc( pAttachInfo = static_cast<ATTACHINFO *>( PvOSMemoryHeapAlloc( cbAttachInfo ) ) );

                memset( pAttachInfo, 0, cbAttachInfo );
                pAttachInfo->SetDbid( plrattachdb->dbid );

                pAttachInfo->SetCbNames( plrattachdb->CbPath() );
                pAttachInfo->SetDbtime( 0 );
                pAttachInfo->SetObjidLast( objidNil );
                pAttachInfo->SetCpgDatabaseSizeMax( plrattachdb->le_cpgDatabaseSizeMax );
                pAttachInfo->le_lgposAttach = m_lgposRedo;
                // If lgposConsistent is from the future, it is from a different
                // log stream, do not use it (maybe attach should not even log
                // it?)
                if ( CmpLgpos( plrattachdb->lgposConsistent, m_lgposRedo ) >= 0 )
                {
                    pAttachInfo->le_lgposConsistent = lgposMin;
                }
                else
                {
                    pAttachInfo->le_lgposConsistent = plrattachdb->lgposConsistent;
                }
                memcpy( &pAttachInfo->signDb, &plrattachdb->signDb, sizeof(SIGNATURE) );

                // We will keep ASCII if needed at this point
                // becuase the ATTACHINFO has support for both
                // and we need to deal with both in the below functions
                // anyway
                memcpy ( pAttachInfo->szNames, (CHAR *)plrattachdb->rgb, plrattachdb->CbPath() );

                if ( plrattachdb->FUnicodeNames() )
                {
                    pAttachInfo->SetFUnicodeNames();
                }

                if ( plrattachdb->FSparseEnabledFile() )
                {
                    pAttachInfo->SetFSparseEnabledFile();
                }

                err = ErrLGRIPreSetupFMPFromAttach( ppib, pAttachInfo );
                if ( err >= JET_errSuccess )
                {
                    err = ErrLGRISetupFMPFromAttach( ppib, pAttachInfo, plgstat );
                }
                
                Assert ( pAttachInfo );
                OSMemoryHeapFree( pAttachInfo );
                Call( err );
                }

                ifmp = m_pinst->m_mpdbidifmp[ dbid ];
                FMP::AssertVALIDIFMP( ifmp );
                pfmp = &g_rgfmp[ ifmp ];

                if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
                {
                    break;
                }

                Assert( !pfmp->FReadOnlyAttach() );
                Call( ErrLGRICheckAttachedDb(
                            ifmp,
                            &plrattachdb->signLog,
                            &redoattach,
                            redoattachmodeAttachDbLR ) );
                Assert( NULL != pfmp->Pdbfilehdr() );

                switch ( redoattach )
                {
                    case redoattachNow:
                        Assert( !pfmp->FReadOnlyAttach() );
                        Call( ErrLGRIRedoAttachDb(
                                    ifmp,
                                    plrattachdb->le_cpgDatabaseSizeMax,
                                    plrattachdb->FSparseEnabledFile(),
                                    redoattachmodeAttachDbLR ) );
                        break;

                    case redoattachCreate:
                    default:
                        Assert( fFalse );   //  should be impossible, but as a firewall, set to defer the attachment
                    case redoattachDefer:
                    case redoattachDeferConsistentFuture:
                    case redoattachDeferAccessDenied:
                        Assert( !pfmp->FReadOnlyAttach() );
                        LGRISetDeferredAttachment( ifmp, (redoattach == redoattachDeferConsistentFuture), (redoattach == redoattachDeferAccessDenied) );
                        break;
                }
            }
                break;

            case lrtypDetachDB:
            {
                LRDETACHDB      *plrdetachdb = (LRDETACHDB *)plr;
                dbid = plrdetachdb->dbid;

                Assert( dbid != dbidTemp );
                ifmp = m_pinst->m_mpdbidifmp[ dbid ];
                
                // if there is no attachment for this database, skip it
                //
                if( g_ifmpMax == ifmp )
                {
                    break;
                }
                
                FMP::AssertVALIDIFMP( ifmp );

                pfmp = &g_rgfmp[ifmp];

                if ( pfmp->Pdbfilehdr() )
                {
                    if ( !pfmp->FContainsDataFromFutureLogs() )
                    {
                        Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );
                    }

                    /*  close database for all active user.
                     */
                    PIB     *ppib = ppibNil;

                    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
                    {
                        while( FPIBUserOpenedDatabase( ppib, dbid ) )
                        {
                            if ( NULL != m_pctablehash )
                            {
                                //  close all fucb on this database
                                //
                                LGRIPurgeFucbs( ppib, ifmp, m_pctablehash );
                            }
                            Call( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
                        }
                    }


                    Call( ErrLGRIPpibFromProcid( plrdetachdb->le_procid, &ppib ) );
                    Assert( !pfmp->FReadOnlyAttach() );

                    //  make sure the attached database's size is consistent with the file size.

                    Call( ErrLGICheckDatabaseFileSize( ppib, ifmp ) );

                    //  If there is no redo operations on an attached db, then
                    //  pfmp->dbtimeCurrent may == 0, then do not change pdbfilehdr->dbtime

                    if ( pfmp->DbtimeCurrentDuringRecovery() > pfmp->DbtimeLast() )
                    {
                        pfmp->SetDbtimeLast( pfmp->DbtimeCurrentDuringRecovery() );
                    }

                    //  Do not verify the redo maps if we're still replaying the initial
                    //  required range because we may still see shrink/trim log records
                    //  in a subsequent attach. If there are no more attachments, the
                    //  final call to ErrLGIVerifyRedoMapForIfmp() right before this
                    //  function returns will handle any pending pages in the map.

                    if ( !pfmp->FContainsDataFromFutureLogs() )
                    {
                        Call( ErrLGIVerifyRedoMapForIfmp( ifmp ) );

                        if ( m_fPendingRedoMapEntries && !FMP::FPendingRedoMapEntries() )
                        {
                            ResetPendingRedoMapEntries();
                        }
                    }

                    if ( pfmp->FRedoMapsEmpty() )
                    {
                        // Free the underlying redo maps.
                        pfmp->FreeLogRedoMaps();
                    }

                    Call( ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, pfmp->WszDatabaseName(), plrdetachdb->Flags() & ~fLRDetachDBUnicodeNames ) );

                    // DO NOT TOUCH THE pfmp. IT IS NOT YOURS ANYMORE
                    pfmp = NULL;
                }
                else
                {
                    // Cleaning up and releasing an FMP requires multiple steps that
                    // have to be done atomically.
                    
                    FMP::EnterFMPPoolAsWriter();
                    
                    OSMemoryHeapFree( pfmp->WszDatabaseName() );
                    pfmp->SetWszDatabaseName( NULL );
                    
                    pfmp->ResetFlags();
                    pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = g_ifmpMax;

                    OSMemoryHeapFree( pfmp->Patchchk() );
                    pfmp->SetPatchchk( NULL );
                    
                    FMP::LeaveFMPPoolAsWriter();

                    // DO NOT TOUCH THE pfmp. IT IS NOT YOURS ANYMORE
                    pfmp = NULL;
                }

                LGRITraceRedo(plr);
            }
                break;

            case lrtypExtRestore:
            case lrtypExtRestore2:
                // for tracing only, should be at a new log generation
                break;

            case lrtypExtendDB: // originally lrtypIgnored2
            {
                const LREXTENDDB * const plrextenddb = (LREXTENDDB *)plr;
                Call( ErrLGRIRedoExtendDB( plrextenddb ) );
                break;
            }

            case lrtypShrinkDB:
            case lrtypShrinkDB2:
            case lrtypShrinkDB3:
            {
                const LRSHRINKDB3 lrshrinkdb( (LRSHRINKDB *)plr );
                Call( ErrLGRIRedoShrinkDB( &lrshrinkdb ) );
                break;
            }

            case lrtypSetDbVersion:
            {
                const LRSETDBVERSION* const plrsetdbversion = (LRSETDBVERSION *)plr;
                dbid = plrsetdbversion->Dbid();
                ifmp = m_pinst->m_mpdbidifmp[ dbid ];

                // if there is no attachment for this database, skip it
                //
                if( ifmp >= g_ifmpMax )
                {
                    Assert( ifmp == g_ifmpMax );  // other LRs assume this.
                    break;
                }
                FMP::AssertVALIDIFMP( ifmp );

                //  or the database is not currently attached
                //
                pfmp = &g_rgfmp[ ifmp ];
                if ( !pfmp->FAttached() )
                {
                    Assert( pfmp->FDeferredAttach() || pfmp->FCreatingDB() || pfmp->FSkippedAttach() );
                    if ( pfmp->FDeferredAttach() || pfmp->FSkippedAttach() )
                    {
                        break;
                    }
                    // note we let creating DB through, as we're before create DB finish and it needs 
                    // it's version(s) updated.
                }

                if ( pfmp->Pdbfilehdr() )
                {
                    const DbVersion dbvRedo = plrsetdbversion->Dbv();
                    Call( ErrDBRedoSetDbVersion( m_pinst, ifmp, dbvRedo ) );
                }
                else
                {
                    //  Not sure what case this would be where we have a attached or creating DB (and not deferred 
                    //  or skipped) and it has no file header?
                    FireWall( "RedoSetDbVerNoDbHdr" );
                }

                break;
            }
            
            case lrtypTrimDB:
            {
                const LRTRIMDB * const plrtrimdb = (LRTRIMDB *)plr;
                Call( ErrLGRIRedoTrimDB( plrtrimdb ) );
                break;
            }

            case lrtypExtentFreed:
            {
                // This is not logged for all free extent operations, only for those related to deleting a whole space tree.
                const LREXTENTFREED * const plrextentfreed = (LREXTENTFREED *)plr;
                CallR( ErrLGRIRedoExtentFreed( plrextentfreed ) );
        
                break;
            }

            /****************************************************/
            /*  Operations Using ppib (procid)                  */
            /****************************************************/

            default:
                err = ErrLGRIRedoOperation( plr );

                //  for every redo operation, if it was skipped,
                //  we can consider it a success and keep going.
                if ( errSkipLogRedoOperation == err )
                {
                    err = JET_errSuccess;
                }
                
                Call( err );
            } /* switch */
        } /* outer default */
    } /* outer switch */

    tcScope->iorReason.SetIors( iorsNone );

#ifdef DEBUG
        m_fDBGNoLog = fFalse;
#endif
        fLastLRIsQuit = fFalse;
        lgposLastRedoLogRec = m_lgposRedo;

        /*  update sector status, if we moved to a new sector
        /**/
        Assert( !fShowSectorStatus || m_lgposRedo.isec >= plgstat->cSectorsSoFar );
        Assert( m_lgposRedo.isec != 0 );
        if ( fShowSectorStatus && m_lgposRedo.isec > plgstat->cSectorsSoFar )
        {
            ULONG       cPercentSoFar;
            JET_SNPROG  *psnprog = &(plgstat->snprog);

            Assert( plgstat->pfnCallback );

            plgstat->cSectorsSoFar = m_lgposRedo.isec;
            cPercentSoFar = (ULONG)((100 * plgstat->cGensSoFar) / plgstat->cGensExpected);

            cPercentSoFar += (ULONG)((plgstat->cSectorsSoFar * 100) /
                (plgstat->cSectorsExpected * plgstat->cGensExpected));

            // cPercentSoFar can actually exceed 100% if new logfiles appear while we are
            // replaying. This is OK in the recovery-without-undo case, which doesn't require
            // and eXX.log. In that case, cap the percentage complete at 100%
            cPercentSoFar = min( cPercentSoFar, 100 );

            Assert( cPercentSoFar >= psnprog->cunitDone );
            if ( cPercentSoFar > psnprog->cunitDone )
            {
                psnprog->cunitDone = cPercentSoFar;
                (plgstat->pfnCallback)( JET_snpRestore, JET_sntProgress, psnprog, plgstat->pvCallbackContext );
            }
        }
    }
    while ( ( err = m_pLogReadBuffer->ErrLGGetNextRecFF( (BYTE **) &plr ) ) == JET_errSuccess
            || errLGNoMoreRecords == err );

    //  we have dropped out of the replay loop with an unexpected result
    //  this should be some types of error
    Assert( err < 0 );
    //  dispatch the error
    Call( err );

Done:
    err = errT; //JET_errSuccess;

HandleError:
    /*  assert all operations successful for restore from consistent
    /*  backups
    /**/
#ifndef RFS2
    AssertSz( err >= 0,     "Debug Only, Ignore this Assert");
#endif

    //  if the redo operations succeeded, we should now inspect the redo map
    //  to ensure that all redo operations were reconciled.
    if ( err >= JET_errSuccess )
    {
        err = ErrLGIVerifyRedoMaps();

        // Do not clean up redo maps if we failed because checkpoint advancement
        // might kick in and improperly advance it beyond the generation required
        // to reproduce the failure.
        if ( err >= JET_errSuccess )
        {
            LGITermRedoMaps();

            // We normally switch from JET_dbstateDirtyAndPatchedShutdown to JET_dbstateDirtyShutdown
            // when we reach the initial max required lgen, but we may delay it if there are pending
            // redo map entries by that time. Therefore, fix up the DB state here to avoid letting
            // DirtyAndPatched leak beyond redo.
            for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
            {
                const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
                if ( ifmp >= g_ifmpMax )
                {
                    continue;
                }

                FMP* const pfmp = &g_rgfmp[ ifmp ];

                if ( pfmp->FDeferredAttach() || pfmp->FSkippedAttach() )
                {
                    continue;
                }

                if ( pfmp->FNeedUpdateDbtimeBeginRBS() )
                {
                    Assert( pfmp->FRBSOn() );
                    err = pfmp->PRBS()->ErrSetDbtimeForFmp( pfmp, pfmp->DbtimeBeginRBS() );
                    if ( err < JET_errSuccess )
                    {
                        break;
                    }
                    pfmp->ResetNeedUpdateDbtimeBeginRBS();
                }

                PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
                if ( !pdbfilehdr )
                {
                    continue;
                }

                if ( ( pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown ) &&
                     ( m_lgposRedo.lGeneration >= pdbfilehdr->le_lGenMaxRequired ) )
                {
                    Assert( pfmp->FAllowHeaderUpdate() );
                    pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, lGenerationInvalid, lGenerationInvalid, NULL, fTrue );
                }
            }
        }
    }

    // Cleanup preread infra.
    LGPrereadTerm();

    // Deallocate preread-related objects.
    delete m_pPrereadWatermarks;
    m_pPrereadWatermarks = pNil;
    delete m_plpreread;
    m_plpreread = pNil;
    delete m_plprereadSuppress;
    m_plprereadSuppress = pNil;

    // Deallocate LogReader
    if ( err == JET_errSuccess )
    {
        err = m_pLogReadBuffer->ErrLReaderTerm();
    }
    else
    {
        m_pLogReadBuffer->ErrLReaderTerm();
    }

    isdlCurrLog.TermSequence();

    Assert( errSkipLogRedoOperation != err );

    return err;
}

ERR LOG::ErrLGIVerifyRedoMaps()
{
    ERR err = JET_errSuccess;

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
        {
            continue;
        }

        Call( ErrLGIVerifyRedoMapForIfmp( ifmp ) );
    }

    if ( m_fPendingRedoMapEntries && !FMP::FPendingRedoMapEntries() )
    {
        ResetPendingRedoMapEntries();
    }

HandleError:
    return err;
}

ERR LOG::ErrLGIVerifyRedoMapForIfmp( const IFMP ifmp )
{
    Assert( ifmp < g_ifmpMax );

    ERR err = JET_errSuccess;
    PGNO pgno = pgnoNull;
    RedoMapEntry rme;
    CPG cpg = 0;

    const CLogRedoMap* const pLogRedoMapZeroed = g_rgfmp[ ifmp ].PLogRedoMapZeroed();
    const CLogRedoMap* const pLogRedoMapBadDbTime = g_rgfmp[ ifmp ].PLogRedoMapBadDbTime();
    const CLogRedoMap* const pLogRedoMapDbtimeRevert = g_rgfmp[ ifmp ].PLogRedoMapDbtimeRevert();

    if ( NULL != pLogRedoMapZeroed )
    {
        //  oh, oh. An operation was not reconciled. We must fail
        //  recovery accordingly.
        if ( pLogRedoMapZeroed->FAnyPgnoSet() )
        {
            pLogRedoMapZeroed->GetOldestLgposEntry( &pgno, &rme, &cpg );
            Assert( ( rme.dbtimePage == 0 ) || ( rme.dbtimePage == dbtimeShrunk ) );
            Assert( rme.err < JET_errSuccess );
            Expected( ( rme.err == JET_errPageNotInitialized ) || ( rme.err == JET_errFileIOBeyondEOF ) || ( rme.err == JET_errRecoveryVerifyFailure ) );

            OSTrace( JET_tracetagRecoveryValidation,
                     OSFormat( "IFMP %d: Failed recovery as log redo map detected at least one unreconciled entry in pgno: %d.", ifmp, pgno ) );

            err = ErrERRCheck( ( rme.err < JET_errSuccess ) ? rme.err : JET_errRecoveryVerifyFailure );

            if ( ( rme.err == JET_errPageNotInitialized ) || ( rme.err == JET_errFileIOBeyondEOF ) || ( rme.err == JET_errRecoveryVerifyFailure ) )
            {
                err = ErrERRCheck( rme.err );
                LGIReportPageDataMissing(
                    m_pinst,
                    ifmp,
                    pgno,
                    err,
                    rme.lgpos,
                    cpg );
            }
            else
            {
                err = ErrERRCheck( ( rme.err < JET_errSuccess ) ? rme.err : JET_errRecoveryVerifyFailure );
                LGIReportEventOfReadError(
                    m_pinst,
                    ifmp,
                    pgno,
                    err,
                    rme.dbtimePage );
            }
        }
    }

    if ( NULL != pLogRedoMapBadDbTime )
    {
        //  oh, oh. An operation was not reconciled. We must fail
        //  recovery accordingly.
        if ( pLogRedoMapBadDbTime->FAnyPgnoSet() )
        {
            pLogRedoMapBadDbTime->GetOldestLgposEntry( &pgno, &rme, &cpg );
            Assert( rme.err < JET_errSuccess );
            Expected( rme.err == JET_errDbTimeTooOld );
            
            OSTrace( JET_tracetagRecoveryValidation,
                     OSFormat( "IFMP %d: Failed recovery as log redo map of mismatched Dbtimes detected at least one unreconciled entry in pgno: %d.", ifmp, pgno ) );

            // Log the event.
            err = ErrLGRIReportDbtimeMismatch(
                m_pinst,
                ifmp,
                pgno,
                rme.dbtimeBefore,
                rme.dbtimePage,
                rme.dbtimeAfter,
                rme.lgpos,
                cpg );

            Assert( err == JET_errDbTimeTooOld );
        }
    }

    if ( NULL != pLogRedoMapDbtimeRevert )
    {
        //  oh, oh. An operation was not reconciled. We must fail
        //  recovery accordingly.
        if ( pLogRedoMapDbtimeRevert->FAnyPgnoSet() )
        {
            pLogRedoMapDbtimeRevert->GetOldestLgposEntry( &pgno, &rme, &cpg );
            Assert( CPAGE::FRevertedNewPage( rme.dbtimePage ) );
            Assert( rme.err < JET_errSuccess );

            Expected( rme.err == JET_errRecoveryVerifyFailure );

            OSTrace( JET_tracetagRecoveryValidation,
                OSFormat( "IFMP %d: Failed recovery as log redo map for dbtimeRevert detected at least one unreconciled entry in pgno: %d.", ifmp, pgno ) );

            err = ErrERRCheck( rme.err );
            LGIReportBadRevertedPage(
                m_pinst,
                ifmp,
                pgno,
                err,
                rme.lgpos,
                cpg );
        }
    }

    Assert( ( ( err == JET_errSuccess ) && g_rgfmp[ ifmp ].FRedoMapsEmpty() ) ||
            ( ( err < JET_errSuccess ) && !g_rgfmp[ ifmp ].FRedoMapsEmpty() ) );

    return err;
}

VOID LOG::LGITermRedoMaps()
{
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
        {
            continue;
        }

        g_rgfmp[ ifmp ].FreeLogRedoMaps();
    }
}

ERR LOG::ErrLGCheckDatabaseGens(
    const DBFILEHDR_FIX *       pdbfilehdr,
    const LONG                  lGenCurrent
    ) const
{
    Assert( pdbfilehdr->le_lGenMaxRequired <= pdbfilehdr->le_lGenMaxCommitted );

    if ( pdbfilehdr->le_lGenMaxRequired > lGenCurrent )
    {
        return ErrERRCheck( JET_errRequiredLogFilesMissing );
    }
    else if ( pdbfilehdr->le_lGenMaxCommitted > lGenCurrent )
    {
        return ErrERRCheck( JET_errCommittedLogFilesMissing );
    }
    // else ...

    //  We would expect when calling this from ErrLGICheckGenMaxRequiredAfterRedo() ->
    //  ErrLGCheckGenMaxRequired() that we couldn't end with a valid check of lgens and
    //  the DB state left in the patched state.
    //
    Assert( JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->Dbstate() );

    return JET_errSuccess;
}

ERR LOG::ErrLGCheckDBGensRequired( const LONG lGenCurrent )
{
    ERR errWorst = JET_errSuccess;
    ERR errDbid[dbidMax];

    memset( errDbid, 0, sizeof(errDbid) );  // set all to JET_errSuccess

    //  Check lGenMaxRequired before any UNDO operations

    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        const IFMP  ifmp    = m_pinst->m_mpdbidifmp[ dbidT ];

        if ( ifmp >= g_ifmpMax )
            continue;

        FMP         *pfmpT          = &g_rgfmp[ ifmp ];

        Assert( !pfmpT->FReadOnlyAttach() );
        if ( pfmpT->FSkippedAttach()
            || pfmpT->FDeferredAttach() )
        {
            //  skipped attachments is a restore-only (or m_fReplayMissingMapEntryDB analysis flag) concept
            // SOFT_HARD: use per FMP entry
            Assert( !pfmpT->FSkippedAttach() || m_irstmapMac || !m_fReplayMissingMapEntryDB || pfmpT->FHardRecovery() );
            continue;
        }

        //  returns JET_errSuccess, JET_errRequiredLogFilesMissing, or JET_errCommittedLogFilesMissing
        errDbid[dbidT] = ErrLGCheckDatabaseGens( pfmpT->Pdbfilehdr().get(), lGenCurrent );

        if ( errDbid[dbidT] == JET_errRequiredLogFilesMissing )
        {
            errWorst = errDbid[dbidT];
        }
        else if ( errWorst != JET_errRequiredLogFilesMissing &&
                    errDbid[dbidT] != JET_errSuccess )
        {
            errWorst = errDbid[dbidT];
        }
    }

    Assert( errWorst == JET_errRequiredLogFilesMissing ||
            errWorst == JET_errCommittedLogFilesMissing ||
            errWorst == JET_errSuccess );

    return errWorst;
}


VOID LOG::LGIReportMissingHighLog( const LONG lGenCurrent, const IFMP ifmp ) const
{
    Assert( ifmp < g_ifmpMax );
    const FMP * const pfmp = &g_rgfmp[ ifmp ];
    
    const LONG lGenMinRequired = pfmp->Pdbfilehdr()->le_lGenMinRequired;
    const LONG lGenMaxRequired = pfmp->Pdbfilehdr()->le_lGenMaxCommitted;
    WCHAR szT1[16];
    WCHAR szT2[16];
    WCHAR szT3[16];
    WCHAR wszGenMinRequired[IFileSystemAPI::cchPathMax];
    WCHAR wszGenMaxRequired[IFileSystemAPI::cchPathMax];
    WCHAR wszGenCurrent[IFileSystemAPI::cchPathMax];
    const UINT csz = 7;
    const WCHAR *rgszT[csz];

    //  We've lost a log we can't possibly recover from ... we really 
    //  are pooched.

    rgszT[0] = pfmp->WszDatabaseName();
    OSStrCbFormatW( szT1, sizeof(szT1), L"%d", lGenMinRequired );
    rgszT[1] = szT1;
    OSStrCbFormatW( szT2, sizeof(szT2), L"%d", lGenMaxRequired );
    rgszT[2] = szT2;
    OSStrCbFormatW( szT3, sizeof(szT3), L"%d", lGenCurrent );
    rgszT[3] = szT3;

    m_pLogStream->LGMakeLogName( wszGenMinRequired, sizeof(wszGenMinRequired), eArchiveLog, lGenMinRequired );
    rgszT[4] = wszGenMinRequired;

    if ( lGenMaxRequired == pfmp->Pdbfilehdr()->le_lGenMaxCommitted )
    {
        m_pLogStream->LGMakeLogName( wszGenMaxRequired, sizeof(wszGenMaxRequired), eCurrentLog );
    }
    else
    {
        m_pLogStream->LGMakeLogName( wszGenMaxRequired, sizeof(wszGenMaxRequired), eArchiveLog, lGenMaxRequired );
    }
    rgszT[5] = wszGenMaxRequired;

    m_pLogStream->LGMakeLogName( wszGenCurrent, sizeof(wszGenCurrent), eArchiveLog, lGenCurrent );
    rgszT[6] = wszGenCurrent;

    UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                REDO_MISSING_HIGH_LOG_ERROR_ID,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                m_pinst );

    OSUHAPublishEvent( HaDbFailureTagCorruption,
        m_pinst,
        HA_LOGGING_RECOVERY_CATEGORY,
        HaDbIoErrorNone, NULL, 0, 0,
        HA_REDO_MISSING_HIGH_LOG_ERROR_ID,
        sizeof( rgszT ) / sizeof( rgszT[0] ),
        rgszT );
}

VOID LOG::LGIReportMissingCommitedLogsButHasLossyRecoveryOption( const LONG lGenCurrent, const IFMP ifmp ) const
{
    Assert( ifmp < g_ifmpMax );
    const FMP * const pfmp = &g_rgfmp[ ifmp ];
    
    const LONG lGenMinRequired = pfmp->Pdbfilehdr()->le_lGenMinRequired;
    WCHAR wszT1[16];
    WCHAR wszT2[16];
    WCHAR wszT3[16];
    const UINT  csz = 6;
    const WCHAR *rgwszT[csz];

    //  We've lost a log we can recover from, log an event to indicate 
    //  admin may be able to manually recover.

    rgwszT[0] = pfmp->WszDatabaseName();
    OSStrCbFormatW( wszT1, sizeof(wszT1), L"%d", lGenMinRequired );
    rgwszT[1] = wszT1;
    OSStrCbFormatW( wszT2, sizeof(wszT2), L"%d", (LONG) pfmp->Pdbfilehdr()->le_lGenMaxCommitted );
    rgwszT[2] = wszT2;
    rgwszT[3] = m_pLogStream->LogName() ? m_pLogStream->LogName() : L"";
    OSStrCbFormatW( wszT3, sizeof(wszT3), L"%d", lGenCurrent + 1 );
    rgwszT[4] = wszT3;
    rgwszT[5] = m_wszLogCurrent;

    UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                REDO_MISSING_COMMITTED_LOGS_BUT_HAS_LOSSY_RECOVERY_OPTION_ID,
                _countof( rgwszT ),
                rgwszT,
                0,
                NULL,
                m_pinst );
}

VOID LOG::LGIReportCommittedLogsLostButConsistent( const LONG lGenCurrent, const LONG lGenEffectiveCurrent, const IFMP ifmp ) const
{
    Assert( ifmp < g_ifmpMax );
    Assert( lGenEffectiveCurrent <= lGenCurrent );
    
    const FMP * const pfmp = &g_rgfmp[ ifmp ];
    
    const LONG cMissingLogs = pfmp->Pdbfilehdr()->le_lGenMaxCommitted - lGenEffectiveCurrent;
    WCHAR szT1[16];
    WCHAR szT2[16];
    WCHAR szT3[16];
    WCHAR szT4[16];
    const UINT  csz = 6;
    const WCHAR *rgszT[csz];

    //  We lost logs, but it should be ok, they were only committed, not partially
    //  flushed to the database, and the user wants us to try to recover anyway ...

    Assert( m_fIgnoreLostLogs );

    rgszT[0] = pfmp->WszDatabaseName();
    OSStrCbFormatW( szT1, sizeof(szT1), L"%d", cMissingLogs );
    rgszT[1] = szT1;
    //  If we removed current log this is ahead by 1.
    OSStrCbFormatW( szT2, sizeof(szT1), L"%d", lGenCurrent );
    rgszT[2] = szT2;
    OSStrCbFormatW( szT3, sizeof(szT2), L"%d", (LONG) pfmp->Pdbfilehdr()->le_lGenMaxCommitted );
    rgszT[3] = szT3;
    OSStrCbFormatW( szT4, sizeof(szT3), L"%d", lGenEffectiveCurrent );
    rgszT[4] = szT4;
    rgszT[5] = m_wszLogCurrent;

    UtilReportEvent(
                eventWarning,
                LOGGING_RECOVERY_CATEGORY,
                REDO_MISSING_COMMITTED_LOGS_LOST_BUT_CONSISTENT_ID,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                m_pinst );
}

VOID LOG::LGIPossiblyGenerateLogfileMissingEvent(
    const ERR err,
    const LONG lGenCurrent,
    const LONG lGenEffectiveCurrent,
    const IFMP ifmp ) const
{
    Assert( ifmp < g_ifmpMax );
    Assert( lGenEffectiveCurrent <= lGenCurrent );

    if ( JET_errRequiredLogFilesMissing == err )
    {
        //  This would mean we are in trouble, because in recreating a new current log
        //  and subtracting one here, it means we have a DB that needed a log we actually
        //  removed.  We have enforce's protecting against this before we delete the log.
        Assert( !m_pLogStream->FRemovedLogs() );
        LGIReportMissingHighLog( lGenCurrent, ifmp );
    }
    else if ( JET_errCommittedLogFilesMissing == err )
    {
        if ( !m_fIgnoreLostLogs )
        {
            LGIReportMissingCommitedLogsButHasLossyRecoveryOption( lGenCurrent, ifmp );
        }
        else
        {
            if ( ErrLGSignalErrorConditionCallback( m_pinst, JET_errCommittedLogFilesMissing ) < JET_errSuccess )
            {
                LGIReportCommittedLogsLostButConsistent( lGenCurrent, lGenEffectiveCurrent, ifmp );
            }
            // else don't log an event. it is normal for recovery-without-undo to encounter
            // this condition
        }
    }
}

VOID LOG::LGIPossiblyGenerateLogfileMissingEvents(
    const ERR errWorst,
    const LONG lGenCurrent,
    const LONG lGenEffectiveCurrent ) const
{
    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        const IFMP  ifmp = m_pinst->m_mpdbidifmp[ dbidT ];
        if ( ifmp >= g_ifmpMax )
        {
            continue;
        }

        const FMP * const pfmpT = &g_rgfmp[ ifmp ];

        Assert( !pfmpT->FReadOnlyAttach() );
        if ( pfmpT->FSkippedAttach()
            || pfmpT->FDeferredAttach() )
        {
            //  skipped attachments is a restore-only (or m_fReplayMissingMapEntryDB analysis flag) concept
            // SOFT_HARD: use per FMP entry
            Assert( !pfmpT->FSkippedAttach() || m_irstmapMac || !m_fReplayMissingMapEntryDB || pfmpT->FHardRecovery() );
        }
        else
        {
            const ERR errThisDb = ErrLGCheckDatabaseGens( pfmpT->Pdbfilehdr().get(), lGenEffectiveCurrent );
            if ( errWorst == errThisDb )
            {
                LGIPossiblyGenerateLogfileMissingEvent( errWorst, lGenCurrent, lGenEffectiveCurrent, ifmp );
            }
            else if ( errThisDb < JET_errSuccess ) // this DB isn't serious enough to log about ...
            {
                //  This would mean we are in trouble, because in recreating a new current log
                //  and subtracting one here, it means we have a DB that needed a log we actually
                //  removed.  We have enforce's protecting against this before we delete the log.
                Assert( !m_pLogStream->FRemovedLogs() );
            }
        }
    }
}

ERR LOG::ErrLGCheckGenMaxRequired()
{
    ERR errWorst = JET_errSuccess;

    LOGTIME tmCreate;
    const LONG lGenCurrent = m_pLogStream->GetCurrentFileGen( &tmCreate );
    LONG lGenEffectiveCurrent = lGenCurrent;
    if ( m_fLostLogs && m_pLogStream->FCreatedNewLogFileDuringRedo() )
    {
        //  If we removed logs before this point, it means we (possibly removed and) 
        //  recreated the current log meaning the lGenEffectiveCurrent is in fact 1 more 
        //  than it should be, making up for that here ...
        lGenEffectiveCurrent = lGenEffectiveCurrent - 1;
    }

    //  m_fLogslost that m_fEventedLLRDatabases is true too...
    Assert( !m_fEventedLLRDatabases );  // should be called only once w/ fCheckOnly false ... or we'll log events twice ...

    errWorst = ErrLGCheckDBGensRequired( lGenEffectiveCurrent );

    //
    //  bail out if everything is a-ok ...
    //
    if ( errWorst == JET_errSuccess )
    {
        return JET_errSuccess;
    }


    //  Handling error conditions for databases ...

    //
    //  We now go through the databases and log the appropriate errors
    //  for the databases which are in the worst state for this log.
    //
    LGIPossiblyGenerateLogfileMissingEvents( errWorst, lGenCurrent, lGenEffectiveCurrent );
    
    // The log gen we require replay through depends upon the user specification
    //  as to whether it is OK to lose log files...

    if ( m_fIgnoreLostLogs && errWorst == JET_errCommittedLogFilesMissing )
    {
        //  In the event it is ok to lose log files, we down cast this error
        //  to a warning ...

        //  It is far too difficult to weave this warning all the way out
        //  through recovery, so we set something on LOG, and pick it up
        //  in the return from ErrLGSoftStart() ...
        //
        m_fLostLogs = fTrue;
#ifdef DEBUG
        m_fEventedLLRDatabases = fTrue;
#endif
        errWorst = JET_wrnCommittedLogFilesLost;

        // Since, we decided to allow loss of committed logs, fixup max committed generation in database header(s)
        for (DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax )
                continue;

            FMP *pfmp = &g_rgfmp[ ifmp ];

            if ( NULL == pfmp->Pdbfilehdr() )
                continue;

            const LONG lGenRecovering = pfmp->Pdbfilehdr()->le_lGenRecovering;

            Assert( lGenRecovering <= pfmp->Pdbfilehdr()->le_lGenMaxCommitted );

            if ( pfmp->Pdbfilehdr()->le_lGenMaxCommitted <= lGenCurrent )
                continue;

            Assert( pfmp->Pdbfilehdr()->le_lGenMaxRequired <= lGenCurrent );
            {
            PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
            pdbfilehdr->le_lGenMaxCommitted = lGenCurrent;
            memcpy( &pdbfilehdr->logtimeGenMaxCreate, &tmCreate, sizeof( LOGTIME ) );
            pdbfilehdr->le_lGenRecovering = 0;
            }

            ERR err;
            CallR( ErrUtilWriteAttachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp,  pfmp->Pfapi() ) );
        }
    }

    //  We have a strong contract, either we are 
    //      A. missing required logs, or
    //      B. missing committed logs, or
    //      C. missing committed logs AND user asked for us to try to recovery anyway ...
    //  in that order of preference.
    Assert( errWorst == JET_errRequiredLogFilesMissing ||
            errWorst == JET_errCommittedLogFilesMissing ||
            errWorst == JET_wrnCommittedLogFilesLost );

    return errWorst;
}

ERR LOG::ErrLGMostSignificantRecoveryWarning( void )
{
    //  Shouldn't have removed logs set, w/o lost logs also set ...
    Assert( !m_pLogStream->FRemovedLogs() || m_fLostLogs );
    #ifdef DEBUG
    if ( m_pLogStream->FRemovedLogs() )
    {
        Assert( m_fEventedLLRDatabases );
    }
    #endif
    return m_pLogStream->FRemovedLogs() ? ErrERRCheck( JET_wrnCommittedLogFilesRemoved ) : // most significant warning
            m_fLostLogs ? ErrERRCheck( JET_wrnCommittedLogFilesLost ) :
            JET_errSuccess;
}

//  Redoes database operations in log from lgposRedoFrom to end.
//
//  GLOBAL PARAMETERS
//                      szLogName       (IN)        full path to szJetLog (blank if current)
//                      lgposRedoFrom   (INOUT)     starting/ending lGeneration and ilgsec.
//
//  RETURNS
//                      JET_errSuccess
//                      error from called routine
//
ERR LOG::ErrLGRRedo( BOOL fKeepDbAttached, CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat )
{
    ERR     err;
    ERR     errBeforeRedo           = JET_errSuccess;
    ERR     errRedo                 = JET_errSuccess;
    ERR     errAfterRedo            = JET_errSuccess;
    LGPOS   lgposRedoFrom;
    INT     fStatus;
    BOOL    fSkipUndo               = fFalse;   // default is to perform undo
    BOOL    fRcvCleanlyDetachedDbs  = fTrue;

    TraceContextScope tcScope( iortRecoveryRedo );
    tcScope->nParentObjectClass = tceNone;

    //  set flag to suppress logging
    //
    m_fRecovering = fTrue;
    m_fRecoveringMode = fRecoveringRedo;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlRedoBegin|sysosrtlContextInst, m_pinst, (PVOID)&m_pinst->m_iInstance, sizeof(m_pinst->m_iInstance) );

    PERFOpt( cLGRecoveryStallReadOnly.Clear( m_pinst ) );
    PERFOpt( cLGRecoveryLongStallReadOnly.Clear( m_pinst ) );
    PERFOpt( cLGRecoveryStallReadOnlyTime.Clear( m_pinst ) );

    Assert( m_fUseRecoveryLogFileSize == fTrue );

    //  open the proper log file
    //
    // lgposRedoFrom is based on the checkpoint, which is based on
    // lgposStart which is based on the beginning of a log
    // record (beginning of a begin-transaction).
    lgposRedoFrom = pcheckpoint->checkpoint.le_lgposCheckpoint;

    //  Set checkpoint before any logging activity.

    m_pcheckpoint->checkpoint.le_lgposCheckpoint = pcheckpoint->checkpoint.le_lgposCheckpoint;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = pcheckpoint->checkpoint.le_lgposDbConsistency;

    Call( m_pLogStream->ErrLGRIOpenRedoLogFile( &lgposRedoFrom, &fStatus ) );

    //  capture the result of ErrLGRIOpenRedoLogFile; it might have a warning from ErrLGCheckReadLastLogRecordFF
    errBeforeRedo = err;

    if ( fStatus != fRedoLogFile )
    {
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"4c0cccc4-430a-46cc-bfee-5d484e38686c" );
        Call( ErrERRCheck( JET_errMissingPreviousLogFile ) );
    }

    Assert( m_pLogStream->FLGFileOpened() );

    // check for the first time agains the potential stopping point
    if ( FLGRecoveryLgposStopLogRecord( lgposRedoFrom ) )
    {
        // we hit already the desired log stop position
        

        Error( ErrLGISetQuitWithoutUndo( ErrERRCheck( JET_errRecoveredWithoutUndo ) ) );
    }
    
    Assert( m_fRecoveringMode == fRecoveringRedo );

    // XXX
    // The flush point should actually be after the current record
    // because the semantics of m_lgposToFlush are to point to the log
    // record that has not been flushed to disk.
    m_pLogWriteBuffer->SetLgposWriteTip( lgposRedoFrom );

    m_pinst->m_pbackup->BKCopyLastBackupStateFromCheckpoint( &pcheckpoint->checkpoint );

    // the default is that logs didn't got truncated because
    // it is safer, eventualy a new full is needed before an incremental
    // or some logs are lingering around until the next full backup
    m_pinst->m_pbackup->BKSetLogsTruncated( fFalse );

    //  Check attached db already. No need to check in LGInitSession.
    //
    Call( ErrLGRIInitSession(
                &pcheckpoint->checkpoint.dbms_param,
                pcheckpoint->rgbAttach,
                redoattachmodeInitBeforeRedo ) );
    m_fAfterEndAllSessions = fFalse;

    Assert( m_pLogStream->FLGFileOpened() );

    err = ErrLGRIRedoOperations(
                    &pcheckpoint->checkpoint.le_lgposCheckpoint,
                    pcheckpoint->rgbAttach,
                    fKeepDbAttached,
                    &fRcvCleanlyDetachedDbs,
                    plgstat,
                    tcScope );
    //  remember the error code from ErrLGRIRedoOperations() which may have a corruption warning
    //      from ErrLGCheckReadLastLogRecordFF() which it may eventually call
    errRedo = err;

    if ( !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoverySilentRedoDone ) )
    {
        m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoverySilentRedoEnd = m_lgposRedo;
        m_pinst->m_isdlInit.Trigger( eInitLogRecoverySilentRedoDone );
    }
    else
    {
        m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryVerboseRedoEnd = m_lgposRedo;
        m_pinst->m_isdlInit.Trigger( eInitLogRecoveryVerboseRedoDone );
    }

    // we can succeed out of redo without triggering the begin undo callback if
    // the database/log is cleanly terminated ... in such a case we need to make 
    // sure we do not begin undo as well, so set fSkipUndo if the client 
    // indicates undo should be skipped from the JET_sntBeginUndo callback.
    if ( err == JET_errRecoveredWithoutUndo )
    {
        Assert( errRedo == JET_errRecoveredWithoutUndo );
        fSkipUndo = fTrue;
        err = JET_errSuccess;
    }
    else if ( err >= JET_errSuccess )
    {
        AssertRTL( m_errBeginUndo == JET_errSuccess );
        const ERR errBeginUndo = ErrLGBeginUndoCallback();
        AssertRTL( m_errBeginUndo == errBeginUndo );
        fSkipUndo = ( errBeginUndo == JET_errRecoveredWithoutUndo );
        if ( !fSkipUndo )
        {
            //  Any other error, bail out ...
            err = errBeginUndo;
        }
    }

    AssertRTL( FLGILgenHighAtRedoStillHolds() );
    AssertRTL( err < JET_errSuccess ||
                m_fRedidAllLogs ||
                m_fLostLogs ||
                errRedo == JET_errRecoveredWithoutUndo );
    AssertRTL( err == errRedo || errRedo == JET_errRecoveredWithoutUndo );
    AssertRTL( fSkipUndo ||
                m_errBeginUndo != JET_errRecoveredWithoutUndo ||
                errRedo != JET_errSuccess );

    Assert( m_pLogStream->GetCurrentFileHdr() != NULL );

    if ( err < 0 )
    {
        //  LGITryCleanupAfterRedoError is done in HandleError section,
        //  so we are not calling it here.
        //
        //  flush as much as possible before bail out
        //
        //  UNDONE: if there are some deferred attachments,
        //  is the checkpoint going to be erroneously updated
        //  past the genMin of those attachments??
        //
        //  LGITryCleanupAfterRedoError( errRedo );

#ifdef DEBUG
        //  Recovery should never fail unless some hardware problems
        //  or out of memory (mainly with RFS enabled)
        //  NOTE: it can also fail from corruption
        switch( errRedo )
        {
//
//  SEARCH-STRING: SecSizeMismatchFixMe
//
//          case JET_errLogSectorSizeMismatch:
//          case JET_errLogSectorSizeMismatchDatabasesConsistent:
            case JET_errDiskFull:
            case JET_errOutOfBuffers:
            case JET_errOutOfMemory:
            case_AllLogStorageCorruptionErrs:
                break;

            default:
                Assert( JET_errSuccess != errRedo );
                break;
        }
#endif  //  DEBUG

        goto HandleError;
    }

    //  we should have the right sector size by now or an error that will make redo fail

    Assert( m_pLogStream->GetCurrentFileHdr() != NULL );
    Assert( m_pLogStream->CbSec() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );

    //  performing recovery without undo is very like erroring out here, but we do a little
    //  more validation before "erroring" out ...

    if ( fSkipUndo &&
            ( !m_fAfterEndAllSessions || JET_errRecoveredWithoutUndo == errRedo ) )
    {
        //  check lGenMaxRequired to verify we replayed as much as required
        //
        if ( errRedo != JET_errRecoveredWithoutUndo )
        {
            Call( ErrLGICheckGenMaxRequiredAfterRedo() );
        }
        if ( err != JET_wrnCommittedLogFilesLost )
        {
            CallS( err );   //  if we saw a 587 / JET_wrnCommittedLogFilesRemoved we probably have a larger issue
        }

        //  if we are recovering with undo then check for deferred attachments
        //
        if ( errRedo != JET_errRecoveredWithoutUndo )
        {
            //  since this is not the Undo phase of recovery, no
            //  force-detach will actually get logged (can't log
            //  anything during RecoveryRedo)
            //
            Call( ErrLGRICheckForAttachedDatabaseMismatch( m_pinst, !!m_fReplayingIgnoreMissingDB, !!m_fLastLRIsShutdown ) );
        }

        //  flush as much as possible before bail out
        //
        LGITryCleanupAfterRedoError( errRedo );

        if ( m_fTruncateLogsAfterRecovery )
        {
            m_pLogStream->LGTruncateLogsAfterRecovery();
        }

        AssertRTL( FLGILgenHighAtRedoStillHolds() );

        //  generate error indicating Undo was not performed
        //
        Error( ErrERRCheck( JET_errRecoveredWithoutUndo ) );
    }

    AssertRTL( FLGILgenHighAtRedoStillHolds() );
    AssertRTL( m_fRedidAllLogs || m_fLostLogs );


    Assert( FLGILgenHighAtRedoStillHolds() );

    Assert( !fSkipUndo || m_fAfterEndAllSessions );

    OSTrace( JET_tracetagInitTerm, OSFormat( "[Recovery] Begin undo operations. [pinst=%p]", m_pinst ) );

    //  Check lGenMaxRequired/lGenMaxCommitted before any UNDO operations
    //
    Call( ErrLGICheckGenMaxRequiredAfterRedo() );
    Assert( err == JET_errSuccess ||
                ( m_fIgnoreLostLogs && ErrLGMostSignificantRecoveryWarning() > JET_errSuccess ) );

    //  Redo considers the whole file flushed, now that undo may be adding records to the file, that is
    //  obviously not true, so fix that by moving it back to what was actually redone.
    if ( CmpLgpos( m_lgposFlushTip, m_lgposRedo ) > 0 )
    {
        m_pLogWriteBuffer->LockBuffer();
        LGResetFlushTipWithLock( m_lgposRedo );
        m_pLogWriteBuffer->UnlockBuffer();
    }

    // If the current log was not found during replay, create it now
    if ( !m_pLogStream->FLGRedoOnCurrentLog() )
    {
        // update sector size for new format if needed
        m_pLogStream->LGResetSectorGeometry( m_fUseRecoveryLogFileSize ? m_lLogFileSizeDuringRecovery : 0 );

        //  re-initialize the log manager's buffer with user settings
        Call( m_LogBuffer.ErrLGInitLogBuffers( m_pinst, m_pLogStream ) );
        Call( m_pLogStream->ErrLGRStartNewLog( m_pLogStream->GetCurrentFileGen() + 1 ) );
    }

#ifdef DEBUG
    m_fDBGNoLog = fFalse;
#endif

    //  Check that the checkpoint has advanced during recovery.
    //
    Assert( CmpLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint,
                    pcheckpoint->checkpoint.le_lgposCheckpoint )  >= 0 );

    BOOL fDummy;
    // Setup log buffers to have end of the current log file.
    Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fDummy, fTrue ) );
    CallS( err );

    //  capture the result of this operation
    //
    errAfterRedo = err;

    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        // update sector size for new format if needed
        m_pLogStream->LGResetSectorGeometry( m_fUseRecoveryLogFileSize ? m_lLogFileSizeDuringRecovery : 0 );
    }

    //  re-initialize the log manager's buffer with user settings
    Call( m_LogBuffer.ErrLGInitLogBuffers( m_pinst, m_pLogStream ) );

    // allow flush thread to do flushing now.
    m_pLogWriteBuffer->SetFNewRecordAdded( fTrue );

    //  close and reopen log file in R/W mode
    //
    Call( m_pLogStream->ErrLGOpenFile() );

    m_pinst->m_isdlInit.Trigger( eInitLogRecoveryReopenDone );

    //  Signal detach so users who have open sessions during redo
    //  can cleanup before we transition to undo. This is necessary
    //  because some of the calls made from those sessions (backup is
    //  one example) may try to generate LRs if they are not running
    //  during fRecoveringRedo, which might happen if we let those
    //  sessions leak beyond this point.
    //
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( NULL != pfmp->Pdbfilehdr() &&
             !pfmp->FContainsDataFromFutureLogs() )
        {
            //  fire callback for the dbs to be detached
            (void)ErrLGDbDetachingCallback( m_pinst, pfmp );
        }
    }


    //  switch to undo mode

    tcScope->iorReason.SetIort( iortRecoveryUndo );

    m_fRecoveringMode = fRecoveringUndo;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlUndoBegin|sysosrtlContextInst, m_pinst, (PVOID)&m_pinst->m_iInstance, sizeof(m_pinst->m_iInstance) );

    //  we should be able to establish desired log format at this point (or fail)

    const LogVersion * plgvDesired = NULL;
    BOOL fLogVersionNeedsUpdate =
                    ( m_pLogStream->ErrLGGetDesiredLogVersion( (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion ), &plgvDesired ) >= JET_errSuccess ) &&
                    FLGFileVersionUpdateNeeded( *plgvDesired );
    //  Signal log rollover if there's log version change

    //  switch from a finished log file to a new one if necessary
    //  a new log file may be needed if the current log file is full
    Call( m_pLogStream->ErrLGSwitchToNewLogFile( m_pLogWriteBuffer->LgposWriteTip().lGeneration, 0 ) );

    //  we must move the max log required / waypoint up to the current log for undo ...

    Call( ErrLGUpdateWaypointIFMP( m_pinst->m_pfsapi ) );

    //  This may not hold if !m_fAfterEndAllSessions can affect something in the code
    //  above here.
    Assert( FLGILgenHighAtRedoStillHolds() );

    Assert( !m_fAfterEndAllSessions || !m_fLastLRIsShutdown );
    if ( !m_fAfterEndAllSessions )
    {
        Assert( !fSkipUndo );

        //  DbList must have been loaded by now
        //  UNDONE: Is it possible to have the
        //  RcvUndo as the first log record in
        //  a log file (eg. crash after creating
        //  log file, but before anything but
        //  the header can be flushed?)
        Enforce( !m_fNeedInitialDbList );

        if ( !m_fLastLRIsShutdown )
        {
            //  write a RecoveryUndo record to indicate start to undo
            //  this corresponds to the RecoveryQuit record that
            //  will be written out in LGRIEndAllSessions()
            CallR( ErrLGRecoveryUndo(
                        this,
                        fLogVersionNeedsUpdate || BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ) );
            fLogVersionNeedsUpdate = fFalse; // RcvUndo record took care of log version update for us! Yay!
            m_pinst->SetStatusUndo();
            m_lgposRecoveryUndo = m_pLogWriteBuffer->LgposLogTip();
            m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryUndoEnd = m_lgposRecoveryUndo;
            if ( m_pLogStream->FIsOldLrckVersionSeen() )
            {
                m_pLogStream->SetLgposUndoForOB0( m_lgposRecoveryUndo );
            }
        }

        AtomicExchange( (LONG *)&m_fRecoveryUndoLogged, fTrue );

        Call( ErrLGRIEndAllSessions(
                    fTrue,
                    // If we saw a Shutdown but not a Term record, it means that
                    // it was a lossy failover in the middle of JetTerm and the
                    // other side may have already cleanly terminated, so do not
                    // keep db attached at end of recovery
                    fKeepDbAttached && !m_fLastLRIsShutdown,
                    &pcheckpoint->checkpoint.le_lgposCheckpoint,
                    pcheckpoint->rgbAttach ) );
        m_fAfterEndAllSessions = fTrue;

#ifdef DEBUG
        m_lgposRedoShutDownMarkGlobal = lgposMin;
#endif
        m_lgposRedoLastTerm = lgposMin;
        m_lgposRedoLastTermChecksum = lgposMin;
    }
    else
    {
        AtomicExchange( (LONG *)&m_fRecoveryUndoLogged, fTrue );

        Assert( 0 != CmpLgpos( &lgposMin, &m_lgposRedoLastTerm ) );
    }

    Assert( m_pLogStream->FLGFileOpened() || m_pLogStream->FLGProbablyWriting() );

    // ensure that everything hits the disk
    Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );

    // the last thing to do with the checkpoint is to update it to
    // the Term log record. This will make us determine a clean shutdown.
    // The last term found on redo should be set but still check it
    if ( ( 0 != CmpLgpos( &lgposMin, &m_lgposRedoLastTerm ) ) && fRcvCleanlyDetachedDbs )
    {
        // check that the last redo is either the same as the last term redone
        // or it is just a checksum record after it
#ifdef DEBUG
        // If you crash us at end of ErrLGSoftStart while recovery the files 
        // in ShadowLogs in accept DML, AND then recover again, this assert 
        // goes off ... this is b/c the m_lgposRedoLastTerm is at the end
        // of the previous file, and we've switched to a new file, so 
        // m_lgposRedo is at the beginning of it.  Logically it's at the
        // last term in the previous file though ... so this 3rd big clause
        // is essentially the same as the 2nd clause, but with a log roll
        // in-beteween.
        Assert( 0 == CmpLgpos( &m_lgposRedo, &m_lgposRedoLastTerm ) ||
                0 == CmpLgpos( &m_lgposRedo, &m_lgposRedoLastTermChecksum ) ||
                //  We should be on the first sector of the next log file after the last term ...
                //      and the previous log redo should be equal to last term or last term checksum ...
                ( ( ( m_lgposRedoLastTerm.lGeneration + 1 ) == m_lgposRedo.lGeneration ) &&
                  ( m_lgposRedo.isec == m_pLogStream->CSecHeader() ) &&
                  ( m_lgposRedo.ib == 0 ) &&
                        ( 0 == CmpLgpos( &m_lgposRedoPreviousLog, &m_lgposRedoLastTerm ) ||
                          0 == CmpLgpos( &m_lgposRedoPreviousLog, &m_lgposRedoLastTermChecksum ) ) ) );
#endif
        // if we had a clean termination (m_fAfterEndAllSessions is TRUE)
        // then we need to regenerate the checkpoint with the last
        // termination position which is the last thing it got redone
        //  Ignore any error in RTL , because in the worst case, we just end
        //  up rescanning all the log files only to find out nothing
        //  needs to be redone.
        ERR errT = JET_errSuccess;
        LogJETCall( errT = ErrLGIUpdateCheckpointLgposForTerm( m_lgposRedoLastTerm ) );
        if ( JET_errSuccess != errT )
        {
            Assert( ( FNegTest( fLockingCheckpointFile ) && JET_errFileAccessDenied == errT ) ||
                    JET_errOutOfMemory == errT );
        }
    }

    if ( m_fTruncateLogsAfterRecovery )
    {
        m_pLogStream->LGTruncateLogsAfterRecovery();
    }

    //  check the current generation

    Assert( lGenerationMax < lGenerationMaxDuringRecovery );
    if ( m_pLogStream->GetCurrentFileGen() >= lGenerationMax )
    {

        //  the current generation is beyond the last acceptable non-recovery generation
        //      (e.g. we have moved into the reserved generations which are between
        //       lGenerationMax and lGenerationMaxDuringRecovery)

        Assert( m_pLogStream->GetCurrentFileGen() <= lGenerationMaxDuringRecovery );

        //  do not allow any more generations -- user must wipe logs and restart at generation 1

        Error( ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent ) );
    }

    Assert( fSkipUndo || m_errBeginUndo != JET_errRecoveredWithoutUndo );
    if ( fSkipUndo )
    {
        AssertRTL( FLGILgenHighAtRedoStillHolds() );
    }

HandleError:

    if ( err >= JET_errSuccess )
    {
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlUndoSucceed|sysosrtlContextInst, m_pinst, (PVOID)&m_pinst->m_iInstance, sizeof(m_pinst->m_iInstance) );
    }
    else
    {
        ULONG rgul[4] = { ULONG( m_pinst->m_iInstance ), ULONG( err ), PefLastThrow()->UlLine(), UlLineLastCall() };
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlRedoUndoFail|sysosrtlContextInst, m_pinst, rgul, sizeof(rgul) );
    }

    //  there are 4 errors to consolidate here: err, errBeforeRedo, errRedo, errAfterRedo
    //  precedence is as follows: (most) err, errBeforeRedo, errRedo, errAfterRedo (least)

    //  errBeforeRedo can only be success or warning.

    Assert( errBeforeRedo >= JET_errSuccess );

    //  If err is JET_errSuccess or warning, neither of errRedo/errAfterRedo should be an error

    if ( err >= JET_errSuccess )
    {
        Assert( errRedo >= JET_errSuccess);
        Assert( errAfterRedo >= JET_errSuccess );
    }

    if ( err == JET_errSuccess )
    {
        if ( errBeforeRedo == JET_errSuccess )
        {
            if ( errRedo == JET_errSuccess )
            {
                err = errAfterRedo;
            }
            else
            {
                err = errRedo;
            }
        }
        else
        {
            err = errBeforeRedo;
        }
    }

    //  We used to call LGITryCleanupAfterRedoError with parameter errRedo.
    //  In LGITryCleanupAfterRedoErr, if the errRedo parameter passed in is
    //  JET_errInvalidLogSequence, we disable check point file update.
    //  Since we moved the LGITryCleanupAfterRedoError here and pass parameter err
    //  instead of errRedo to LGITryCleanupAfterRedoError, we are making sure the
    //  case is covered with this assert.

    if ( errRedo < JET_errSuccess )
    {
        Assert( err == errRedo );
    }

    //  After ErrLGRIInitSession, we set this flag to fFalse, and set it to fTrue
    //  after we call ErrLGRIEndAllSessions or ErrLGRIEndAllSessionsWithError.
    //  What we do in ErrLGRIEndAllSessions/ErrLGRIEndAllSessionsWithError is cleanup work
    //  such as terminating the instance.
    //
    //  We are passing err instead of errRedo as the parameter here, since this
    //  covers the case that errRedo equals JET_errInvalidLogSequence, and we are
    //  good to disable check point file update in other cases when err is
    //  JET_errInvalidLogSequence while errRedo is not.

    if ( !m_fAfterEndAllSessions )
    {
        Assert( err < JET_errSuccess );
        LGITryCleanupAfterRedoError( err );
    }

    Assert( NULL == m_pctablehash );

    //  set flag to suppress logging
    //
    m_fRecovering = fFalse;
    m_fRecoveringMode = fRecoveringNone;

    if ( err >= JET_errSuccess )
    {
        //  verify the log sector size

        Assert( m_pLogStream->GetCurrentFileHdr() != NULL );
        Assert( m_pLogStream->CbSec() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );
        if ( FDefaultParam( m_pinst, JET_paramLogFileSize ) )
        {
            Assert( m_pLogStream->CSecLGFile() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile );
            Assert( m_pLogStream->CbSec() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );
            Assert( m_pLogStream->CSecHeader() == ( sizeof( LGFILEHDR ) + m_pLogStream->CbSec() - 1 ) / m_pLogStream->CbSec() );
        }

        // Set here, consumed in ErrLGMoveToRunningState ...
        Assert( m_lLogFileSizeDuringRecovery == LONG( ( QWORD( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile ) * m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec ) / 1024 ) );
        // just in case ...
        m_lLogFileSizeDuringRecovery = LONG( ( QWORD( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile ) *
                                          m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec ) / 1024 );
        
    }


    return err;
}


ERR LOG::ErrLGMoveToRunningState(
    ERR     errFromErrLGRRedo,
    BOOL *  pfJetLogGeneratedDuringSoftStart
    )
{
    ERR err = JET_errSuccess;
    BOOL fMismatchedLogChkExt;
    BOOL fMismatchedLogFileSize;
    BOOL fMismatchedLogFileVersion;
    BOOL fDeleteLogs = fFalse;

    Assert( errFromErrLGRRedo == JET_errSuccess );
    if ( errFromErrLGRRedo )
    {
        CallR( ErrERRCheck( errFromErrLGRRedo ) );
    }

    //
    //  Find any mismatched log parameters 
    //
    //  NOTE we can only switch over if we've got circular logging ...
    Assert( m_pLogStream->LogExt() );
    fMismatchedLogChkExt = !FLGIsDefaultExt( fFalse, m_pLogStream->LogExt() );
    fMismatchedLogFileSize = UlParam( m_pinst, JET_paramLogFileSize ) != (ULONG_PTR)m_lLogFileSizeDuringRecovery;
    fMismatchedLogFileVersion = m_pLogStream->FIsOldLrckVersionSeen();

    //
    //  Decide whether or not to delete log files and upgrade various running log params
    //
    if ( fMismatchedLogChkExt )
    {
        //  NOTE we can only switch over if we've got circular logging, and it isn't eseutil running ...
        fDeleteLogs = !g_fEseutil && BoolParam( m_pinst, JET_paramCircularLog );
    }

    //  do not use the recovery log file size anymore
    //
    m_fUseRecoveryLogFileSize = fFalse;

    //  verify the log file size

    Assert( m_lLogFileSizeDuringRecovery != 0 && m_lLogFileSizeDuringRecovery != ~(ULONG)0 );

    if ( FDefaultParam( m_pinst, JET_paramLogFileSize ) )
    {

        //  the user never set the log file size, so we will set it on their behalf

        Call( Param( m_pinst, JET_paramLogFileSize )->Set( m_pinst, ppibNil,  m_lLogFileSizeDuringRecovery, NULL ) );

    }
    else if ( fMismatchedLogFileSize )
    {

        if ( !BoolParam( m_pinst, JET_paramCleanupMismatchedLogFiles ) )
        {
            //  we are not allowed to cleanup the mismatched logfiles
            Error( ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent ) );
        }

        //  the user chose a specific log file size, so we must enforce it
        fDeleteLogs = fTrue;
        // I am surprised we allow this for non-circular logging though ...

    }

    if ( fMismatchedLogFileVersion )
    {
        m_pLogStream->ResetOldLrckVersionSecSize();
        // clean up old version logs now if allowed by the user
        fDeleteLogs = BoolParam( m_pinst, JET_paramCleanupMismatchedLogFiles );
    }

    // update sector size for new format if needed
    m_pLogStream->LGResetSectorGeometry( 0 );

    //
    //  Delete log files ...
    //
    if ( fDeleteLogs )
    {

        //  the logfile size or log extension doesn't match

        //  we can cleanup the old logs/checkpoint and start a new sequence
        //
        //  if we succeed, we will return JET_errSuccess and the user will be at 
        //      if cleaning up b/c of log file size, gen 1 
        //      if cleaning up b/c of log ext change, at the next gen in seq
        //  if we fail, the user will be forced to delete the remaining logs/checkpoint by hand

        Call( m_pLogStream->ErrLGRICleanupMismatchedLogFiles( fMismatchedLogChkExt ) );

        //  we created a new log file, so we should clean it up if we fail
        //  (leaves us with an empty log set which is ok because we are 100% recovered)
        //
        if ( pfJetLogGeneratedDuringSoftStart )
        {
            *pfJetLogGeneratedDuringSoftStart = fTrue;
        }

    }

    //  extract the size of the log in sectors (some codepaths don't set this, so we should do it now)
    //
    m_pLogStream->UpdateCSecLGFile();

    // Make sure that m_trxNewest is in multiples of TRXID_INCR
    //
    m_pinst->m_trxNewest = roundup( m_pinst->m_trxNewest, TRXID_INCR );

HandleError:

    Assert( m_pLogStream->LogExt() );

    return err;
}

VOID LGFakeCheckpointToLogFile( CHECKPOINT * const pcheckpointT, const LGFILEHDR * const plgfilehdrCurrent, ULONG csecHeader )
{
    //  Make the checkpoint point to the start of this log generation so that
    //  we replay all attachment related log records.
    pcheckpointT->checkpoint.dbms_param = plgfilehdrCurrent->lgfilehdr.dbms_param;

    //  Ensure that the checkpoint never reverts.
    Assert( pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration <= plgfilehdrCurrent->lgfilehdr.le_lGeneration );
    pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration = plgfilehdrCurrent->lgfilehdr.le_lGeneration;
    pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec = (WORD) csecHeader;
    pcheckpointT->checkpoint.le_lgposCheckpoint.le_ib = 0;
    pcheckpointT->checkpoint.le_lgposDbConsistency = pcheckpointT->checkpoint.le_lgposCheckpoint;
    UtilMemCpy( pcheckpointT->rgbAttach, plgfilehdrCurrent->rgbAttach, cbAttach );
}

ERR LOG::ErrLGICheckClosedNormallyInPreviousLog(
    _In_ const LONG         lGenPrevious,
    _Out_ BOOL *            pfCloseNormally
    )
{
    ERR err = JET_errSuccess;

    //  reopen the highest previous log gen file
    //
    m_pLogStream->LGMakeLogName( eArchiveLog, lGenPrevious );

    err = m_pLogStream->ErrLGOpenFile();
    if ( err < 0 )
    {
        LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
        goto HandleError;
    }

    Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID ) );

    Assert( m_pLogStream->GetCurrentFileGen() == lGenPrevious );

    //  figure out if it was closed normally

    Assert( !m_fRecovering );
    Assert( m_fRecoveringMode == fRecoveringNone );
    m_fRecovering = fTrue;
    m_fRecoveringMode = fRecoveringRedo;

    Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( pfCloseNormally ) ); // don't update accumulated segment checksum

    m_fRecovering = fFalse;
    m_fRecoveringMode = fRecoveringNone;

HandleError:

    m_pLogStream->LGCloseFile();

    return err;
}

//
//  Soft start tries to start the system from current directory.
//  The database maybe in one of the following state:
//  1) no log files.
//  2) database was shut down normally.
//  3) database was rolled back abruptly.
//  In case 1, a new log file is generated.
//  In case 2, the last log file is opened.
//  In case 3, soft redo is incurred.
//  At the end of the function, it a proper szJetLog must exists.
//
ERR LOG::ErrLGSoftStart( BOOL fKeepDbAttached, BOOL fInferCheckpointFromRstmapDbs, BOOL *pfNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart )
{
    ERR                 err;
    ERR                 errErrLGRRedo = JET_wrnNyi;
    BOOL                fCloseNormally = fTrue;
    //  Initially pfNewCheckpointFile actually only tells us that earlier in the
    //  init code path we could not find an existing checkpoint.  So coming in here
    //  it means we couldn't find a checkpoint (or maybe a valid checkpoint)
    BOOL                fUseCheckpointToStartRedo = !(*pfNewCheckpointFile);
    BOOL                fSoftRecovery = fFalse;
    WCHAR               wszPathJetChkLog[IFileSystemAPI::cchPathMax];
    LGFILEHDR           *plgfilehdrT = NULL;
    CHECKPOINT          *pcheckpointT = NULL;
    BOOL                fCreatedReserveLogs = fFalse;
    BOOL                fDelLog = fFalse;

    // Create a top level scope for all operations belonging to recovery
    TraceContextScope tcScope( iortRecovery );
    // LOG doesn't know TCE during replay;  will pollute stats for activated passive's cache
    tcScope->nParentObjectClass = tceNone;

    //  Indicate we are in SoftStart / Recovery so that callbacks can call back 
    //  into ESE if necessary.

    Ptls()->fInSoftStart = fTrue;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlSoftStartBegin|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );

    //  By default we will start at the checkpoint

    *pfJetLogGeneratedDuringSoftStart = fFalse;
    m_fAbruptEnd = fFalse;

    Assert( !m_fLostLogs && !m_pLogStream->FRemovedLogs() && JET_errSuccess == ErrLGMostSignificantRecoveryWarning() );

    //  use the right log file size for recovery
    //
    Assert( m_fUseRecoveryLogFileSize == fFalse );
    m_fUseRecoveryLogFileSize = fTrue;

    //  set m_fNewCheckpointFile
    //
    m_fNewCheckpointFile = *pfNewCheckpointFile;

    //  set m_wszLogCurrent
    //
    m_wszLogCurrent = SzParam( m_pinst, JET_paramLogFilePath );

    //  if no recovery stop position was set, set it to MAX
    if ( CmpLgpos( &m_lgposRecoveryStop, &lgposMin ) == 0 )
    {
        m_lgposRecoveryStop = lgposMax;
    }
    
    // also make sure that the lgpos is somehow valid ... 
    // at least if is not lgposMin, then it should have a valid generation
    if ( CmpLgpos( &m_lgposRecoveryStop, &lgposMin ) != 0 && m_lgposRecoveryStop.lGeneration == 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    // Stop at certain log means we should not even try to open more logs
    // after that generation
    // Note: This means it would be hard to honor JET_paramDeleteOldLogs 
    // which needs EDB.LOG and also with the need to replay with the ESENT97.DLL (for ESENT)
    //
    // Note: This means that JET_paramDeleteOutOfRangeLogs won't be able to
    // be honor either as we rely on EDB.LOG to find what to delete
    //
    if ( FLGRecoveryLgposStop() &&
        ( BoolParam( m_pinst, JET_paramDeleteOldLogs ) || BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  CONSIDER: for tight check, we may check if all log files are
    //  CONSIDER: continuous by checking the generation number and
    //  CONSIDER: previous gen's creation date.

    //  try to open current log file to decide the status of log files.
    //
    err = m_pLogStream->ErrLGTryOpenJetLog( JET_OpenLogForRecoveryCheckingAndPatching, lGenSignalCurrentID, (m_pLogStream->LogExt() == NULL) );
    if ( err < 0 )
    {
        if ( JET_errFileNotFound != err )
        {

            //  we were unable to open edb.jtx/log or there is a missing edbtmp.jtx/log or unable to 
            //  move edbXXXXX.jtx/log to edb.jtx/log, where XXXXX is the highest log gen.
            //  also, the error doesn't reveal whether or not edb.jtx/log and edbtmp.jtx/log even exist
            //  we'll treat the error as a critical failure during file-open -- we can not proceed with recovery
            goto HandleError;
        }

        //  neither szJetLog nor szJetTmpLog exist. If no old generation
        //  files exists, gen a new logfile at generation 1 (or m_lgenInitial).
        //

        LONG lgenHigh = 0;
        //  Hard recovery can hit here, with the m_wszLogExt set, though regular init with an empty 
        //  log directory, doesn't have the m_wszLogExt set, so set it to the default.
        if ( m_pLogStream->LogExt() == NULL )
        {
            LONG    lgenTLegacy = 0;
            BOOL    fDefaultExt = fTrue;
            Call ( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenTLegacy, fTrue, &fDefaultExt ) );

            // if we found something, then we can try and set the extension to that
            // 
            if ( lgenTLegacy )
            {
                m_wszChkExt = fDefaultExt ? WszLGGetDefaultExt( fTrue ) : WszLGGetOtherExt( fTrue );
                m_pLogStream->LGSetLogExt( FLGIsLegacyExt( fFalse, fDefaultExt ? WszLGGetDefaultExt( fFalse ) : WszLGGetOtherExt( fFalse ) ), fFalse );
                
                // also, avoid the range check below
                //
                lgenHigh = lgenTLegacy;
            }
            else
            {
                m_wszChkExt = WszLGGetDefaultExt( fTrue );
                m_pLogStream->LGSetLogExt( FLGIsLegacyExt( fFalse, WszLGGetDefaultExt( fFalse ) ), fFalse );
            }
        }

        if ( lgenHigh == 0 )
        {
            Call ( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenHigh ) );
        }
        
        if ( lgenHigh != 0 ) // there exists archive log files ...
        {
            const ERR errMissingCurrentLog = m_pLogStream->FTempLogExists() ? JET_errSuccess : JET_errMissingLogFile;
            Call( ErrLGOpenLogMissingCallback( m_pinst, m_pLogStream->LogName(), errMissingCurrentLog, lgenHigh+1, fTrue, JET_MissingLogContinueToRedo ) );

            // we are ok with a missing edb.log but some logs present if the client
            // wishes us to continue (such as HA log shipping).
            // We won't need to patch the named log that we found so we don't need 
            // to go to "CheckLogs", just go to the checkpoint file logic.
            // 

            // The last named log doesn't have a clean shutdown as the end so:
            // 
            fCloseNormally = fFalse;

            OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );

            goto CheckCheckpoint;
            // Note I changed the behavior of eseutil /r /a, we used to open the high log 
            // and jump to "CheckLogs", now we just jump straight to checkpoint validation 
            // and replay. And /a / JET_bitIgnoreLostLogs uses the callback for 
            // JET_sntOpenCheckpoint to ignore the checkpoint during recovery.
        }

        Call( ErrLGOpenLogMissingCallback( m_pinst, m_pLogStream->LogName(), JET_errSuccess, 0, fFalse, JET_MissingLogCreateNewLogStream ) );

        // Delete the leftover checkpoint file before creating new gen 1 log file
        LGFullNameCheckpoint( wszPathJetChkLog );
        m_pinst->m_pfsapi->ErrFileDelete( wszPathJetChkLog );

        // use the right log file size for generating the new log file
        m_fUseRecoveryLogFileSize = fFalse;

        Call( m_pLogStream->ErrLGCreateNewLogStream( pfJetLogGeneratedDuringSoftStart ) );

        Assert( *pfJetLogGeneratedDuringSoftStart );

    }
    else // else err < 0 , where err = ErrLGOpenJetLog()
    {
        //
        //  At this point we have an m_pfapiLog on the highest generation.
        //
//CheckLogs: old label, left in for the referencing comments above.
        LGPOS lgposLastTerm;

        //  read current log file header
        //
        Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID ) );

        //  if we have only one log file and no checkpoint file then the m_lgenInitial can be too
        //  high here ... see ErrLGInit().
        if ( (*pfNewCheckpointFile) &&
            m_lgenInitial > m_pLogStream->GetCurrentFileGen() )
        {
            Assert( m_lgenInitial == m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration );
            //  We have a slight problem in that we've already created a checkpoint / at least 
            //  in memory, and initialized it with m_lgenInitial ... fix this ...
            LGISetInitialGen( m_pLogStream->GetCurrentFileGen() );
        }

        //  verify and patch the current log file

        Assert( !m_fRecovering );
        Assert( m_fRecoveringMode == fRecoveringNone );
        m_fRecovering = fTrue;
        m_fRecoveringMode = fRecoveringRedo;

        err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue, fFalse, NULL, &lgposLastTerm );

        m_fRecovering = fFalse;
        m_fRecoveringMode = fRecoveringNone;


        m_pinst->m_isdlInit.Trigger( eInitLogReadAndPatchFirstLog );

        //  eat errors about corruption -- we will go up to the point of corruption
        //      and then return the right error in ErrLGRRedo()

        if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
        {
            err = JET_errSuccess;
        }
        else
        {
            Assert( err < 0 );
            Call( err );
        }

        m_pLogStream->LGCloseFile();

        //  if delete out of range logs is set then delete all out of range logs
        //
        //  Why do we not delete out of range log files if m_fIgnoreLostLogs?  Because
        //  for the m_fIgnoreLostLogs case is easier to handle deep in ErrLGIRedoFill().
        //
        //  Aside, I think this should also only be if we're in circular logging case?  Right.
        //
        // SOFT_HARD: leave like this, soft recovery only behaviour
        if ( !m_fHardRestore && BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) && !m_fIgnoreLostLogs )
        {
            Assert( !FLGRecoveryLgposStop() );

            //  Note: the current log
            Call ( m_pLogStream->ErrLGDeleteOutOfRangeLogFiles() );
        }
        
        // if we decided so far that we don't need soft recovery,
        // we check if the checkpoint and the clean shutdown
        // really do match
        if ( fCloseNormally && !(*pfNewCheckpointFile) )
        {
            ERR errReadCheckpoint;

            if ( pcheckpointT == NULL )
            {
                Alloc( pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );
            }

            // build the checkpoint name and open the file.
            LGFullNameCheckpoint( wszPathJetChkLog );
            errReadCheckpoint = ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fFalse );

            // we opened fine the checkpoint including the check if the signature is matching the log signature
            if ( errReadCheckpoint >= JET_errSuccess )
            {

                // Check if the actual checkpoint and the last shutdown in the log do match.
                // If they don't, it might be because an older checkpoint is forcing an earlier log start for recovery
                // (like bringing back older files, including checkpoint with OSSnapshot restore as an exemple).
                // In this case we will force "not normal close" so we do start soft recovery.
                // Note: we don't do additional checking at this point (like the checkpoint is ahead)
                // because the existing code should deal with this already
                if ( 0 != CmpLgpos( &pcheckpointT->checkpoint.le_lgposCheckpoint, &lgposLastTerm ) )
                {
                    Assert( fCloseNormally );
                    fCloseNormally = fFalse;
                }
            }
            else
            {
                // we have an error reading the checkpoint (like bad signature.
                // We will just ignore it (the below code will delete the bad checkpoint)
                (*pfNewCheckpointFile) = fTrue;
                m_fNewCheckpointFile = fTrue;
            }
        }

CheckCheckpoint:
    
        //  If the edb.jtx/log was not closed normally or the checkpoint file was
        //  missing and new one is created, then do soft recovery.
        //
        if ( !fCloseNormally || (*pfNewCheckpointFile) )
        {
            //  always redo from beginning of a log generation.
            //  This is needed such that the attach info will be matching
            //  the with the with the redo point. Note that the attach info
            //  is not necessarily consistent with the checkpoint.
            //
            if ( plgfilehdrT == NULL )
            {
                Alloc( plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) );
                
                // we really on the log header below if we had it already opened 
                // (like we found an EDB.LOG above) so initialize it here to 0 
                //
                memset( plgfilehdrT, 0, sizeof(LGFILEHDR) );
            }

            if ( pcheckpointT == NULL )
            {
                Alloc( pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );
            }

            //  did not terminate normally and need to redo from checkpoint
            //
            LGFullNameCheckpoint( wszPathJetChkLog );

            //  Ask the client if they would like us to use the checkpoint.
            err = ErrLGOpenCheckpointCallback( m_pinst );
            if ( err < JET_errSuccess )
            {
                AssertRTL( JET_errFileNotFound == err );
                fUseCheckpointToStartRedo = fFalse;
                (*pfNewCheckpointFile) = fTrue;
                m_fNewCheckpointFile = fTrue;
            }

            if ( !fUseCheckpointToStartRedo )
            {
                //  Delete the newly created empty checkpoint file.
                //  Let redo recreate one.

                (VOID)m_pinst->m_pfsapi->ErrFileDelete( wszPathJetChkLog );
            }

            //  if we should use checkpoint and it could be read, start there
            //  otherwise checkpoint could/should not be read, then revert to redoing
            //  log from first log record in first log generation file.
            //
            if ( fUseCheckpointToStartRedo &&
                 ( JET_errSuccess <= ( err = ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fFalse ) ) ) )
            {
                // we are opening EDB.LOG only if the checkpoint points to it AND
                // we are not having an UndoStop position in which case we already
                // opened that log generation and we have the m_plgfilehdr->lgfilehdr.le_lGeneration set
                // 
                if ( (LONG) m_pLogStream->GetCurrentFileGen() == (LONG) pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration )
                {
                    Call( m_pLogStream->ErrLGOpenJetLog( JET_OpenLogForRedo, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration ) );
                }
                else
                {
                    Call( m_pLogStream->ErrLGOpenLogFile( JET_OpenLogForRedo, eArchiveLog, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration, JET_errMissingLogFile ) );
                }

                //  read log file header
                //
                Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, plgfilehdrT, fCheckLogID ) );

                if ( pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration != plgfilehdrT->lgfilehdr.le_lGeneration ||
                     !( pcheckpointT->checkpoint.fVersion & fCheckpointAttachInfoPresent ) ||
                     !FLGVersionAttachInfoInCheckpoint( &plgfilehdrT->lgfilehdr ) )
                {
                    //  Start checkpoint here so that we replay all attachment related log records.
                    LGFakeCheckpointToLogFile( pcheckpointT, plgfilehdrT, m_pLogStream->CSecHeader() );
                }
                else
                {
                    // even when using LGPOS/attach from checkpoint, get
                    // dbms_param which control max sessions etc from log
                    // (dbms_param should ultimately go away)
                    pcheckpointT->checkpoint.dbms_param = plgfilehdrT->lgfilehdr.dbms_param;
                }

                //  cleanup opened log file
                m_pLogStream->LGCloseFile();
            }
            else // else of the if error from ErrLGReadCheckpoint(), i.e. err < 0
            {
                m_fNewCheckpointFile = fTrue;

                LONG lgenLow = 0;
                if ( fInferCheckpointFromRstmapDbs )
                {
                     LoadCheckpointGenerationFromRstmap( &lgenLow );
                }

                if ( lgenLow == 0 )
                {
                    (void) m_pLogStream->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenLow, NULL );
                }

                // if there are no archived logs, we should start with the current
                // log which we already have in the log header
                // 
                if ( lgenLow == 0 )
                {
                    // set the initial log gen to something sane.
                    if ( m_lgenInitial > m_pLogStream->GetCurrentFileGen() )
                    {
                        m_lgenInitial = m_pLogStream->GetCurrentFileGen();
                    }

                    //  Start checkpoint here so that we replay all attachment related log records.
                    LGFakeCheckpointToLogFile( pcheckpointT, m_pLogStream->GetCurrentFileHdr(), m_pLogStream->CSecHeader() );

                    err = JET_errSuccess;
                }
                else
                {
                    // set the initial log gen to something sane.
                    m_lgenInitial = ( m_lgenInitial > lgenLow ) ? lgenLow : m_lgenInitial;

                    // start with the lowest numbered archive log
                    Call( m_pLogStream->ErrLGOpenLogFile( JET_OpenLogForRedo, eArchiveLog, lgenLow, JET_errMissingPreviousLogFile ) );
                    
                    //  read log file header
                    //
                    Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, plgfilehdrT, fCheckLogID ) );

                    //  Start checkpoint here so that we replay all attachment related log records.
                    LGFakeCheckpointToLogFile( pcheckpointT, plgfilehdrT, m_pLogStream->CSecHeader() );

                    //  cleanup opened log file
                    m_pLogStream->LGCloseFile();

                    err = JET_errSuccess;
                }

                (void)ErrLGIUpdateCheckpointLgposForTerm( pcheckpointT->checkpoint.le_lgposCheckpoint );
            }

            // We have a good checkpoint, see if there is a UndoStop point
            // and where it is compared with the checkpoint
            //
            if ( FLGRecoveryLgposStop() )
            {
                const LGPOS lgposCheckpoint = pcheckpointT->checkpoint.le_lgposCheckpoint;

                // if the checkpoint is already past the UndoStop, finish this recovery
                // 
                if ( lgposCheckpoint.lGeneration > m_lgposRecoveryStop.lGeneration )
                {
                    Error( ErrERRCheck( JET_errRecoveredWithoutUndo ) );
                }
            }
            
            m_pinst->m_pbackup->BKCopyLastBackupStateFromCheckpoint( &pcheckpointT->checkpoint );

            // set the initial log gen to something sane.
            m_lgenInitial = ( m_lgenInitial > pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration )
                ?   pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration
                  : m_lgenInitial;

            m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryStartMin = pcheckpointT->checkpoint.le_lgposCheckpoint;

            //  shouldn't be any errors or warnings at this point
            CallS( err );
            //  ensure we've setup the checkpoint ...
            Assert( pcheckpointT );

            //  set log path to current directory
            //
            Assert( m_wszLogCurrent == SzParam( m_pinst, JET_paramLogFilePath ) );

            //  create the reserve logs

            Assert( m_fUseRecoveryLogFileSize );
            Assert( (UINT)m_pLogStream->CsecLGFromSize( m_lLogFileSizeDuringRecovery ) == m_pLogStream->CSecLGFile() );
            Assert( !fCreatedReserveLogs );
            Call( m_pLogStream->ErrLGInitCreateReserveLogFiles() );
            fCreatedReserveLogs = fTrue;

            UtilReportEvent(
                    eventInformation,
                    LOGGING_RECOVERY_CATEGORY,
                    START_REDO_ID,
                    0,
                    NULL,
                    0,
                    NULL,
                    m_pinst );
            fSoftRecovery = fTrue;

            //  redo from last checkpoint
            //
            m_fAbruptEnd = fFalse;
            m_errGlobalRedoError = JET_errSuccess;

            LGSTATUSINFO        lgstat = { 0 };
            LGSTATUSINFO        *plgstat = NULL;
            
            if ( m_pinst->m_pfnInitCallback != NULL )
            {
                plgstat = &lgstat;
                
                LONG lGenLow    = 0;
                LONG lGenHigh   = 0;
    
                (void) m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, &lGenLow, &lGenHigh );

                // we start from the checkpoint
                lGenLow = max( lGenLow, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration);
                lGenHigh = max( lGenHigh, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration);

                // we stop at the last log (as determined above) or at the Undo Stop point
                if ( FLGRecoveryLgposStop( ) )
                {
                    lGenLow = min( lGenLow, m_lgposRecoveryStop.lGeneration );
                    lGenHigh = min( lGenHigh, m_lgposRecoveryStop.lGeneration );
                }
                
                LGIRSTPrepareCallback( plgstat, lGenHigh, lGenLow, m_lgposRecoveryStop.lGeneration );
            }
            else
            {
                //  zero out to silence the compiler
                //
                Assert( NULL == plgstat );
                memset( &lgstat, 0, sizeof(lgstat) );
            }

            err = errErrLGRRedo = ErrLGRRedo( fKeepDbAttached, pcheckpointT, plgstat );

            Assert( err != JET_errRecoveredWithoutUndoDatabasesConsistent );

            if ( plgstat )
            {
                plgstat->snprog.cunitDone = plgstat->snprog.cunitTotal;     //lint !e644
                if ( plgstat->pfnCallback )
                {
                    (plgstat->pfnCallback)(
                        JET_snpRestore,
                        (err < JET_errSuccess && err != JET_errRecoveredWithoutUndo) ? JET_sntFail : JET_sntComplete,
                        &lgstat.snprog,
                        plgstat->pvCallbackContext );
                }
            }
            Call( err );
            CallS( err );   //  no other warnings expected

            //  sector-size checking should now be on

            fDelLog = fTrue;

            if ( g_fRepair && m_errGlobalRedoError != JET_errSuccess )
            {
                Call( ErrERRCheck( JET_errRecoveredWithErrors ) );
            }

            // we don't report the progress if the control callback doesn't
            // say so ...
            //
            if ( ErrLGNotificationEventConditionCallback( m_pinst, STOP_REDO_ID ) >= JET_errSuccess )
            {
                UtilReportEvent(
                        eventInformation,
                        LOGGING_RECOVERY_CATEGORY,
                        STOP_REDO_ID,
                        0,
                        NULL,
                        0,
                        NULL,
                        m_pinst );
            }
        }
        else // fCloseNormally && !(*pfNewCheckpointFile
        {
            //  we did not need to run recovery
            //
            errErrLGRRedo = JET_errSuccess;
        }

        //  if we have gotten to here, it means that recovery redo (and
        //  possibly undo) was successful and so we are continuing to 
        //  do time, so give the client one more chance to avoid this.
        //
        //  if the client was running with the intention of running
        //  "recovery without undo" (such as Exchange HA log shipping) 
        //  and would failed the JET_sntBeginUndo callback, then we can
        //  still have ended up here because the logs have terminated 
        //  cleanly due to JetTerm() or something.
        //
        //  This is where JET_errRecoveredWithoutUndoDatabasesConsistent 
        //  is typically going to returned.
        //
        //  it is a little weird to ask this here and not below (where we 
        //  actually open the log), but this is where we used to throw this 
        //  specific error. We could also make a BeginDo callback I suppose, 
        //  but I'm going to go with this for now ...

        Call( ErrLGOpenLogCallback( m_pinst, m_pLogStream->LogName(), JET_OpenLogForDo, 0, fTrue ) );

        //  
        //  Move to working log state, update the log sec size, and new extension if necessary ...
        //
        Call( ErrLGMoveToRunningState( errErrLGRRedo, pfJetLogGeneratedDuringSoftStart ) );
        
        Assert( m_pLogStream->FLGFileOpened() || !*pfJetLogGeneratedDuringSoftStart );
        if ( fDelLog || *pfJetLogGeneratedDuringSoftStart )
        {
            //  close the log file (code below expects it to be closed)
            //
            m_pLogStream->LGCloseFile();
        }
        
    }   // if/else err < 0 , where err = ErrLGOpenJetLog(), also jumps into clause ..

    //  if external recovery control indicated anything other than success for
    //  begin undo, we should not have gotten here.

    AssertRTL( m_errBeginUndo == JET_errSuccess );

    //  we should now be using the right log file size
    //
    Assert( m_fUseRecoveryLogFileSize == fFalse );

    if ( !fCreatedReserveLogs || *pfJetLogGeneratedDuringSoftStart )
    {
        //  create the reserve logs
        //
        Assert( m_wszLogCurrent == SzParam( m_pinst, JET_paramLogFilePath ) );
        Call( m_pLogStream->ErrLGInitCreateReserveLogFiles() );
        fCreatedReserveLogs = fTrue;
    }

    //  at this point, we have a szJetLog file, reopen the log files
    //
    m_fAbruptEnd = fFalse;

    //  disabled flushing while we reinit the log buffers and check the last log file

    m_pLogWriteBuffer->SetFNewRecordAdded( fFalse );


    BOOL fHadToRetry = fFalse;
RetryOpenLogForDo:

    //  reopen the log file
    //
    m_pLogStream->LGMakeLogName( eCurrentLog );
    err = m_pLogStream->ErrLGOpenFile();
    if ( err < 0 )
    {
        LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
        goto HandleError;
    }
    err = m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID );
    if ( err == JET_errLogFileSizeMismatch )
    {
        err = ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent );
    }
    else if ( err == JET_errLogSectorSizeMismatch )
    {
        //
        //  SEARCH-STRING: SecSizeMismatchFixMe
        //
        Assert( fFalse );
        err = ErrERRCheck( JET_errLogSectorSizeMismatchDatabasesConsistent );
    }
    Call( err );

    //  set up log variables properly

    BOOL fDummy;

    Assert( !m_fRecovering );
    Assert( m_fRecoveringMode == fRecoveringNone );
    m_fRecovering = fTrue;
    m_fRecoveringMode = fRecoveringRedo;

    err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( fHadToRetry ? &fDummy : &fCloseNormally, fTrue );
    Assert( !fHadToRetry || fCloseNormally );

    m_fRecovering = fFalse;
    m_fRecoveringMode = fRecoveringNone;

    Call( err );
    CallS( err );

    // if that last file was old format, we need to set things back
    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        // update sector size for new format if needed
        m_pLogStream->LGResetSectorGeometry( m_fUseRecoveryLogFileSize ? m_lLogFileSizeDuringRecovery : 0 );
    }

    //  close and reopen log file in R/W mode

    Call( m_pLogStream->ErrLGOpenFile() );

    //  re-initialize the log manager's buffer with user settings
    Call( m_LogBuffer.ErrLGInitLogBuffers( m_pinst, m_pLogStream ) );

    err = m_pLogStream->ErrLGSwitchToNewLogFile( m_pLogWriteBuffer->LgposWriteTip().lGeneration, 0 );
    if ( err == JET_errLogFileSizeMismatch )
    {
        err = ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent );
    }
    else if ( err == JET_errLogSectorSizeMismatch )
    {
        //
        //  SEARCH-STRING: SecSizeMismatchFixMe
        //
        Assert( fFalse );
        err = ErrERRCheck( JET_errLogSectorSizeMismatchDatabasesConsistent );
    }
    else if ( err == JET_errLogSequenceEnd )
    {
        err = ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent );
    }
    Call( err );
    if ( !fCloseNormally && !(*pfJetLogGeneratedDuringSoftStart ) )
    {
        if ( !fHadToRetry )
        {
            fHadToRetry = fTrue;

            m_pLogStream->LGCloseFile();

            Call( ErrLGICheckClosedNormallyInPreviousLog( m_pLogStream->GetCurrentFileGen() - 1, &fCloseNormally ) );

            if ( fCloseNormally )
            {
                OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );
                goto RetryOpenLogForDo;
            }
        }

        Assert( fCloseNormally );
        //  Unknown reason to fail to open for logging, most likely
        //  the file got locked.
        Call( ErrERRCheck( JET_errLogWriteFail ) );
    }

    m_pLogStream->ResetOldLrckVersionSecSize();

    //  check for log-sequence-end
    //
    Assert( m_pLogStream->GetLogSequenceEnd() || m_pLogStream->GetCurrentFileGen() < lGenerationMax );
    if ( m_pLogStream->GetLogSequenceEnd() )
    {
        Assert( m_pLogStream->GetCurrentFileGen() >= lGenerationMax );
        Error( ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent ) );
    }
    else if ( m_pLogStream->GetCurrentFileGen() > lGenerationMaxWarningThreshold )
    {
        WCHAR wszGenCurr[64];
        WCHAR wszGenLast[64];
        WCHAR wszGenDiff[64];
        const WCHAR *rgpszString[] = { wszGenCurr, wszGenLast, wszGenDiff };

        OSStrCbFormatW( wszGenCurr, sizeof(wszGenCurr), L"%d (0x%08X)", m_pLogStream->GetCurrentFileGen(), m_pLogStream->GetCurrentFileGen() );
        OSStrCbFormatW( wszGenLast, sizeof(wszGenLast), L"%d (0x%08X)", lGenerationMax, lGenerationMax);
        OSStrCbFormatW( wszGenDiff, sizeof(wszGenDiff), L"%d (0x%08X)",
                lGenerationMax - m_pLogStream->GetCurrentFileGen(),
                lGenerationMax - m_pLogStream->GetCurrentFileGen() );

        UtilReportEvent(
                eventWarning,
                LOGGING_RECOVERY_CATEGORY,
                ALMOST_OUT_OF_LOG_SEQUENCE_ID,
                _countof(rgpszString),
                rgpszString,
                0,
                NULL,
                m_pinst );
    }

    err = m_pLogStream->ErrLGOpenFile();
    if ( err < 0 )
    {
        LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
        goto HandleError;
    }

    Assert( m_fRecovering == fFalse );
    // SOFT_HARD: whatever, leave like this
    Assert( m_fHardRestore == fFalse );

    m_pLogWriteBuffer->CheckIsReady();

    m_pLogWriteBuffer->SetFNewRecordAdded( fTrue );

HandleError:

    if ( err < 0 )
    {
        ULONG rgul[4] = { (ULONG) m_pinst->m_iInstance, (ULONG) err, PefLastThrow()->UlLine(), UlLineLastCall() };
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlSoftStartFail|sysosrtlContextInst, m_pinst, rgul, sizeof(rgul) );

        // SOFT_HARD: whatever, leave like this
        Assert( m_fHardRestore == fFalse );

        // there may be an LGFlush task outstanding (as part of Undo). holding the LGFlush
        // lock here will ensure that pending tasks will complete *before*
        // or will early out *after* we null m_pfapiLog.
        
        m_pLogStream->LGCloseFile();

        if ( fSoftRecovery )
        {
            if ( err != JET_errRecoveredWithoutUndo &&
                    err != JET_errRecoveredWithoutUndoDatabasesConsistent )
            {
                UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err, m_pinst );

                // JET_errFileIOBeyondEOF is expected during recovery *if* the page is a new page and we're
                // replaying a log stream from a time when there wasn't an lrtypExtendDB. If the page
                // isn't a new page then we need to generate a failure event. We catch the error here instead of
                // trying to find all the places that can generate this error.
                //
                // Most errors (e.g. out of memory) will generate their own failure item at the point of failure
                // which is why we don't universally generate a failure event when recovery fails.
                if ( JET_errFileIOBeyondEOF == err )
                {
                    OSUHAEmitFailureTag( m_pinst, HaDbFailureTagIoHard, L"E82B2003-FB66-4beb-834A-9742E594A629" );
                }
            }
            else
            {
                WCHAR wszLgPos[64];

                OSStrCbFormatW(wszLgPos, sizeof( wszLgPos ), L"(%06X,%04X,%04X)", m_lgposRedo.lGeneration,m_lgposRedo.isec, m_lgposRedo.ib );
                UtilReportEvent(
                                    eventInformation,
                                    LOGGING_RECOVERY_CATEGORY,
                                    STOP_REDO_WITHOUT_UNDO_ID,
                                    0,
                                    NULL,
                                    sizeof( WCHAR ) * ( LOSStrLengthW(wszLgPos) + 1 ),
                                    wszLgPos,
                                    m_pinst );

                AssertRTL( FLGILgenHighAtRedoStillHolds() );
            }
        }
    }

    OSMemoryPageFree( plgfilehdrT );

    OSMemoryPageFree( pcheckpointT );

    m_fUseRecoveryLogFileSize = fFalse;
    Ptls()->fInSoftStart = fFalse;

    return err;
}

