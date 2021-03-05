// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


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

    if ( FLGRecoveryLgposStopLogRecord( m_lgposRedo ) )
    {


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

    Assert( csr.PagetrimState() == pagetrimNormal || csr.PagetrimState() == pagetrimTrimmed );

    if ( pagetrimTrimmed == csr.PagetrimState() )
    {
#if DEBUG
        Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() &&
                g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( pgno ) );
#endif
    }
    else
    {
#ifdef DEBUG
        Assert( csr.Latch() == latchRIW );
        Expected( csr.Dbtime() != dbtimeBefore );

        Assert( ( csr.Dbtime() > dbtimeBefore ) ||
                ( g_rgfmp[ ifmp ].PLogRedoMapBadDbTime() && g_rgfmp[ ifmp ].PLogRedoMapBadDbTime()->FPgnoSet( pgno ) ) );

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


        csr.ReleasePage();
    }

    Assert( !csr.FLatched() );

    Call( csr.ErrLoadPage( ppib, ifmp, pgno, pbBeforeImage, cb, latchWrite ) );
    csr.RestoreDbtime( dbtimeBefore );
    csr.Downgrade( latchRIW );

    Assert( csr.Pgno() == pgno );
    Assert( csr.Latch() == latchRIW );
    Assert( csr.Dbtime() == dbtimeBefore );

HandleError:
    return err;
}

LOCAL VOID RebuildPageImageHeaderTrailer(
    __in_bcount( cbHeader )     const VOID * const pvHeader,
                                const INT cbHeader,
    __in_bcount( cbTrailer )    const VOID * const pvTrailer,
                                const INT cbTrailer,
    __out                       VOID * const pvDest )
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

INLINE BOOL FLGINeedRedo( const CSR& csr, const DBTIME dbtime )
{
    if ( pagetrimTrimmed == csr.PagetrimState() )
    {
        return fFalse;
    }

    Assert( csr.FLatched() );
    Assert( csr.Dbtime() == csr.Cpage().Dbtime() );

    return dbtime > csr.Dbtime();
}

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

    const ERR       err         = ErrERRCheck( dbtimeOnPage < dbtimeBeforeInLogRec ? JET_errDbTimeTooOld : JET_errDbTimeTooNew );
    const FMP* const pfmp       = FMP::FAllocatedFmp( ifmp ) ? &g_rgfmp[ ifmp ] : NULL;
    const LGPOS     lgposStop   = pinst->m_plog->LgposLGLogTipNoLock();

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
    OSUHAPublishEvent(  ( err == JET_errDbTimeTooOld ?
                            HaDbFailureTagLostFlushDbTimeTooOld :
                            HaDbFailureTagLostFlushDbTimeTooNew ),
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

INLINE BOOL FLGNeedRedoCheckDbtimeBefore(
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
                if ( csr.Dbtime() != dbtimeBefore )
                {
                    FMP* const pfmp = &g_rgfmp[ csr.Cpage().Ifmp() ];
                    LOG* const plog = pfmp->Pinst()->m_plog;


                    if ( csr.Dbtime() < dbtimeBefore
                         && plog->FRedoMapNeeded( csr.Cpage().Ifmp() ) )
                    {
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

                                fRedoNeeded = fFalse;
                            }
                        }
                    }
                    else
                    {
                        *perr = ErrLGRIReportDbtimeMismatch(
                            pfmp->Pinst(),
                            csr.Cpage().Ifmp(),
                            csr.Pgno(),
                            dbtimeBefore,
                            csr.Dbtime(),
                            dbtimeAfter,
                            plog->LgposLGLogTipNoLock(),
                            1 );

                        Assert( csr.Dbtime() == dbtimeBefore || FNegTest( fCorruptingWithLostFlush ) );
                    }
                }
            }
            else
            {
                Assert( dbtimeAfter > dbtimeBefore );
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
    return fRedoNeeded;
}

INLINE BOOL FLGNeedRedoPage( const CSR& csr, const DBTIME dbtime )
{
    return FLGINeedRedo( csr, dbtime );
}

#ifdef DEBUG
INLINE BOOL FAssertLGNeedRedo( const CSR& csr, const DBTIME dbtime, const DBTIME dbtimeBefore )
{
    ERR         err;
    const BOOL  fRedoNeeded = FLGNeedRedoCheckDbtimeBefore( csr, dbtime, dbtimeBefore, &err );

    return ( fRedoNeeded && JET_errSuccess == err );
}
#endif


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

INLINE ERR LOG::ErrLGIAccessPage(
    PIB             *ppib,
    CSR             *pcsr,
    const IFMP      ifmp,
    const PGNO      pgno,
    const OBJID     objid,
    const BOOL      fUninitPageOk )
{
    ERR err = JET_errSuccess, errPage = JET_errSuccess;
    DBTIME dbtime = 0;
    BOOL fBeyondEofPageOk = fFalse;
    PIBTraceContextScope tcScope = ppib->InitTraceContextScope();

    Assert( pgnoNull != pgno );
    Assert( NULL != ppib );
    Assert( !pcsr->FLatched() );

    CLogRedoMap* pLogRedoMapZeroed = g_rgfmp[ifmp].PLogRedoMapZeroed();
    if ( pLogRedoMapZeroed && pLogRedoMapZeroed->FPgnoSet( pgno ) )
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
    }

    fBeyondEofPageOk = fUninitPageOk && g_rgfmp[ifmp].FOlderDemandExtendDb();
    if ( ( ( ( errPage == JET_errFileIOBeyondEOF ) && !fBeyondEofPageOk ) ||
           ( ( errPage == JET_errPageNotInitialized ) && !fUninitPageOk ) ||
           ( ( errPage >= JET_errSuccess ) && pcsr->Cpage().FShrunkPage() && !fUninitPageOk ) ) &&
         FRedoMapNeeded( ifmp ) )
    {
        if ( errPage >= JET_errSuccess )
        {
            pcsr->ReleasePage();
        }


        Call( g_rgfmp[ifmp].ErrEnsureLogRedoMapsAllocated() );
        pLogRedoMapZeroed = g_rgfmp[ifmp].PLogRedoMapZeroed();
        Call( pLogRedoMapZeroed->ErrSetPgno(
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
        if ( ( errPage >= JET_errSuccess ) && ( pcsr->Cpage().FShrunkPage() ) )
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
            LGIReportPageDataMissing( m_pinst, ifmp, pgno, err, LgposLGLogTipNoLock(), 1 );
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

ERR LOG::ErrLGIAccessPageCheckDbtimes(
    __in    PIB             * const ppib,
    __in    CSR             * const pcsr,
            const IFMP      ifmp,
            const PGNO      pgno,
            const OBJID     objid,
            const DBTIME    dbtimeBefore,
            const DBTIME    dbtimeAfter,
    __out   BOOL * const    pfRedoRequired )
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

    *pfRedoRequired = FLGNeedRedoCheckDbtimeBefore( *pcsr, dbtimeAfter, dbtimeBefore, &err );
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
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( pgno ) );

            err = JET_errSuccess;
        }
        pcsr->ReleasePage();
        Assert( !pcsr->FLatched() );
    }

    return err;
}


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

ERR LOG::ErrLGRIAccessNewPage(
    PIB *               ppib,
    CSR *               pcsrNew,
    const IFMP          ifmp,
    const PGNO          pgnoNew,
    const OBJID         objid,
    const DBTIME        dbtime,
    BOOL *              pfRedoNewPage )
{

    *pfRedoNewPage = fTrue;

    if ( g_rgfmp[ ifmp ].FContainsDataFromFutureLogs() )
    {
        ERR err = ErrLGIAccessPage( ppib, pcsrNew, ifmp, pgnoNew, objid, fTrue );

        if ( errSkipLogRedoOperation == err )
        {
            err = JET_errSuccess;

            Assert( pagetrimTrimmed == pcsrNew->PagetrimState() );
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( pgnoNew ) );

            *pfRedoNewPage = fFalse;

            return err;
        }

        if ( err >= 0 )
        {
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


    if ( *pppib == ppibNil )
    {
        Call( ErrPIBBeginSession( m_pinst, pppib, procid, fFalse ) );
        Assert( procid == (*pppib)->procid );
        m_pcsessionhash->InsertPib( *pppib );
    }

HandleError:
    return err;
}


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

    Call( ErrFUCBOpen( ppib, ifmp, &pfucb ) );
    pfucb->pvtfndef = &vtfndefIsam;

    pfucb->ifmp = ifmp;

    Assert( pfcbNil == pfucb->u.pfcb );

    Assert( PinstFromIfmp( ifmp )->m_isdlInit.FActiveSequence() );

Restart:
    AssertTrack( cRetries != 100000, "StalledCreateFucbRetries" );

    pfcb = FCB::PfcbFCBGet( ifmp, pgnoFDP, &fState, fTrue , fTrue );
    Assert( pfcbNil == pfcb || fFCBStateInitialized == fState );
    if ( pfcbNil == pfcb )
    {
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

        Assert( pfcb->FUnique() );
        if ( !fUnique )
            pfcb->SetNonUnique();

        pfcb->Unlock();

        Call( pfcb->ErrLink( pfucb ) );


        pfcb->InsertList();

        Assert( pfcb->WRefCount() >= 1 );

        pfcb->Lock();
        pfcb->CreateComplete();
        pfcb->Unlock();
    }
    else
    {
        err = pfcb->ErrLink( pfucb );
        Assert( pfcb->WRefCount() > 1 );
        pfcb->Release();
        Call( err );
    }

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
                pfcb->Unlink( pfucb );
            }

            FUCBClose( pfucb );
        }

        if ( fCreatedNewFCB )
        {
            pfcb->PrepareForPurge( fFalse );
            pfcb->Purge( fFalse );
        }
    }

    return err;
}



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

            pfucb->u.pfcb->Unlink( pfucb );
            FUCBClose( pfucb );
        }
    }
}

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
            Assert( !FCATHashActive( PinstFromIfmp( pfcbTable->Ifmp() ) ) );
            

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

        pctablehash->PurgeTablesForFCB( pfcb );

        pfcbTable->EnterDDL();

        pfcb->Pidb()->SetFDeleted();

        pfcb->Lock();
        pfcb->SetDeletePending();
        pfcb->SetDeleteCommitted();
        pfcb->Unlock();

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

                pfucb->u.pfcb->Unlink( pfucb );
                FUCBClose( pfucb );
            }
        }
    }
}


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

                pfucb->u.pfcb->Unlink( pfucb );
                FUCBClose( pfucb );
            }
        }
    }
}



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

    if ( NULL == pfucb )
    {
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

    
    Assert( NULL != pdbms_param );
    m_pinst->RestoreDBMSParams( pdbms_param );

    CallR( ErrITSetConstants( m_pinst ) );

    CallR( m_pinst->ErrINSTInit() );

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

    Assert( pbAttach );
    if ( redoattachmodeInitBeforeRedo == redoattachmode )
    {
        Call( CRevertSnapshot::ErrRBSInitFromRstmap( m_pinst ) );

        if ( m_pinst->m_prbs && m_pinst->m_prbs->FRollSnapshot() )
        {
            Call( m_pinst->m_prbs->ErrRollSnapshot( fTrue, fTrue ) );
        }

        err = ErrLGLoadFMPFromAttachments( pbAttach );
        CallS( err );
        Call( err );

        
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

            if ( ( m_fHardRestore || m_irstmapMac > 0 ) && !m_fReplayMissingMapEntryDB )
            {
                if ( 0 > IrstmapSearchNewName( wszDbName ) )
                {
                    
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
                    Assert( fFalse );
                case redoattachDefer:
                case redoattachDeferConsistentFuture:
                case redoattachDeferAccessDenied:
                    Assert( !pfmp->FReadOnlyAttach() );
                    LGRISetDeferredAttachment( ifmp, (redoattach == redoattachDeferConsistentFuture), (redoattach == redoattachDeferAccessDenied) );
                    break;
            }

            
            Assert( pfmp->Patchchk() != NULL );
        }
    }
    else if ( redoattachmodeInitLR == redoattachmode )
    {
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

    LGDisableCheckpoint();

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
        fAdjSize = fFalse;
    }
    else
    {
        err = ErrDBSetLastPage( ppib, ifmp );
        fAdjSize = fTrue;
    }
    if ( JET_errFileNotFound == err )
    {
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


        Assert( 0 != pfmp->CbOwnedFileSize() );

        QWORD cbFileSize;
        CallR( pfmp->Pfapi()->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
        if ( pfmp->CbOwnedFileSize() != cbFileSize )
        {

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
                PIBTraceContextScope tcScope = ppib->InitTraceContextScope();
                tcScope->iorReason.SetIort( iortRecovery );
                err = ErrIONewSize( ifmp, *tcScope, pfmp->PgnoLast(), 0, JET_bitNil );
            }
        }
    }

    return err;
}

LOCAL VOID LGICleanupTransactionToLevel0( PIB * const ppib, CTableHash * pctablehash )
{
    const ULONG ulStartCleanCursorsThreshold    = 64;
    const ULONG ulStopCleanCursorsThreshold     = 16;
    const BOOL  fCleanupCursors                 = ( ppib->cCursors > ulStartCleanCursorsThreshold );
    DWORD_PTR   cStaleCursors                   = 0;
    FUCB *      pfucbPrev                       = pfucbNil;

    Assert( 0 == ppib->Level() );
    Assert( prceNil == ppib->prceNewest );

    for ( FUCB * pfucb = ppib->pfucbOfSession; pfucbNil != pfucb; )
    {
        FUCB * const    pfucbNext   = pfucb->pfucbNextOfSession;

        Assert( pfucb->ppib == ppib );

        if ( fCleanupCursors
            && !FFUCBVersioned( pfucb )
            && ++cStaleCursors > ulStopCleanCursorsThreshold )
        {
            Assert( pfucb->fInRecoveryTableHash );
            pctablehash->RemoveFucb( pfucb );
            pfucb->fInRecoveryTableHash = fFalse;

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

    ppib->RemoveAllDeferredRceid();
    ppib->RemoveAllRceid();
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
            (void)ErrLGDbDetachingCallback( m_pinst, pfmp );
        }
    }

    CallR( ErrLGEndAllSessionsMacro( fFalse  ) );

    (VOID) m_pinst->m_pver->ErrVERRCEClean();

    delete m_pctablehash;
    m_pctablehash = NULL;

    delete m_pcsessionhash;
    m_pcsessionhash = NULL;

    lgposWriteTip = m_lgposRedo;
    m_pinst->m_critPIB.Enter();
    for ( PIB * ppibT = m_pinst->m_ppibGlobal; ppibT != NULL; ppibT = ppibT->ppibNext )
    {
        if ( ppibT->FAfterFirstBT()
            && ppibT->Level() > 0
            && CmpLgpos( &ppibT->lgposStart, &lgposWriteTip ) < 0 )
        {
            lgposWriteTip = ppibT->lgposStart;
        }
    }
    m_pinst->m_critPIB.Leave();
    m_pLogWriteBuffer->SetLgposWriteTip( lgposWriteTip );

    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
    {
        ppib->RemoveAllDeferredRceid();
        ppib->RemoveAllRceid();
    }

    m_fLGFMPLoaded = fTrue;
    const ERR errT = m_pinst->ErrINSTTerm( termtypeError );
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


VOID LOG::LGITryCleanupAfterRedoError( ERR errRedoOrDbHdrCheck )
{
    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );


    m_fLogDisabledDueToRecoveryFailure = fTrue;
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP  ifmp    = m_pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp < g_ifmpMax )
        {
            const BOOL fTestPreimageRedo = !!UlConfigOverrideInjection( 62228, 0 );

            if ( !fTestPreimageRedo && (LONG)UlParam( m_pinst, JET_paramWaypointLatency ) == 0 )
            {
                (VOID)ErrBFFlush( ifmp );
            }

            if ( g_rgfmp[ifmp].FDeferredAttach()
                && !g_rgfmp[ifmp].FIgnoreDeferredAttach()
                && !m_fReplayingIgnoreMissingDB
                && !m_fAfterEndAllSessions )
            {
                LGDisableCheckpoint();
            }
        }
    }
    m_fLogDisabledDueToRecoveryFailure = fFalse;

    m_pinst->m_isdlInit.Trigger( eInitLogRecoveryBfFlushDone );

    if ( errRedoOrDbHdrCheck == JET_errInvalidLogSequence )
    {
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

    if ( err < JET_errSuccess )
    {
        Assert( err == JET_errCommittedLogFilesMissing ||
                err == JET_errRequiredLogFilesMissing );
        Assert( !m_fIgnoreLostLogs || err != JET_errCommittedLogFilesMissing );
    
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

ERR LOG::ErrLGRIEndEverySession()
{
    ERR err = JET_errSuccess;

    PIB * ppib = m_pinst->m_ppibGlobal;
    PIB * ppibNext;
    PROCID procidT = 0;
    while( ppib != ppibNil )
    {
        Assert( procidNil != ppib->procid );
        Assert( NULL != m_pcsessionhash );
        if ( m_pcsessionhash->PpibHashFind( ppib->procid ) == ppibNil )
        {
            ppib = ppib->ppibNext;
            continue;
        }
        m_pcsessionhash->RemovePib( ppib );

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



    for (dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( fKeepDbAttached && pfmp->FCreatingDB() )
        {
            (VOID)ErrIOTermFMP( pfmp, m_lgposRedo, fFalse );

            continue;
        }


        if ( pfmp->Pdbfilehdr() != NULL &&
             pfmp->DbtimeCurrentDuringRecovery() > pfmp->DbtimeLast() )
        {
            pfmp->SetDbtimeLast( pfmp->DbtimeCurrentDuringRecovery() );
        }

        if ( !pfmp->FSkippedAttach() &&
                !pfmp->FDeferredAttach() &&
                m_fRecoveringMode == fRecoveringUndo )
        {
            Assert( pfmp->Pdbfilehdr() );

            if ( NULL != pfmp->Pdbfilehdr() )
            {
                pfmp->PdbfilehdrUpdateable()->le_lGenRecovering = 0;
                
                DBEnforce( ifmp, pfmp->Pdbfilehdr()->Dbstate() != JET_dbstateDirtyAndPatchedShutdown );
            }
        }

        if ( NULL != pfmp->Pdbfilehdr() &&
             !pfmp->FContainsDataFromFutureLogs() )
        {

            Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );
        }
    }


    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
    {
#ifndef RTM
        ppib->AssertNoDeferredRceid();
#endif
        ppib->RemoveAllRceid();
    }

    Assert( !fEndOfLog || m_fRecovering );
    Assert( !fEndOfLog || fRecoveringUndo == m_fRecoveringMode );

    Call( ErrLGEndAllSessionsMacro( fTrue  ) );

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
            if ( CmpLgpos( &lgposMax, &pfmp->Pdbfilehdr()->le_lgposAttach ) != 0 )
            {

                Call( ErrLGICheckDatabaseFileSize( ppib, ifmp ) );
            }
        }

        if ( fEndOfLog )
        {
            
            if ( !pfmp->Pdbfilehdr() && pfmp->Patchchk() )
            {
                FMP::EnterFMPPoolAsWriter();

                Assert( pfmp->FInUse() );
                Assert( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() );

                
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
        m_pinst->m_fTrxNewestSetByRecovery = fTrue;
    }

    
    err = m_pinst->ErrINSTTerm( fKeepDbAttached ? termtypeRecoveryQuitKeepAttachments : termtypeNoCleanUp );
    if ( err < JET_errSuccess )
    {
        return err;
    }
    fNeedCallINSTTerm = fFalse;

    
    m_fLGFMPLoaded = fFalse;

    if ( m_fRecovering && fRecoveringRedo == m_fRecoveringMode )
    {
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
                    m_fHardRestore,
                    BoolParam( m_pinst, JET_paramAggressiveLogRollover ),
                    &lgposRecoveryQuit ) );

        if ( !fKeepDbAttached )
        {
            

            Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );

            Call( ErrLGIUpdateCheckpointLgposForTerm( lgposRecoveryQuit ) );
        }
    }
    else if ( !fKeepDbAttached )
    {
        Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );
    }

HandleError:

    (void)ErrLGEndAllSessionsMacro( fFalse  );

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

    if ( NULL == pdbfilehdr )
    {
        return fFalse;
    }


    if ( CmpLgpos( &lgpos, &pdbfilehdr->le_lgposAttach ) <= 0 )
    {
        Expected( CmpLgpos( lgpos, lgposMin ) == 0 );

        return fFalse;
    }


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


ERR LOG::ErrLGRICheckRedoCondition(
    const DBID      dbid,
    DBTIME          dbtime,
    OBJID           objidFDP,
    PIB             *ppib,
    BOOL            fUpdateCountersOnly,
    BOOL            *pfSkip )
{
    ERR             err;
    INST * const    pinst   = PinstFromPpib( ppib );
    const IFMP      ifmp    = pinst->m_mpdbidifmp[ dbid ];


    *pfSkip = fTrue;


    if ( ifmp >= g_ifmpMax )
    {
        return JET_errSuccess;
    }

    if ( !FLGRICheckRedoConditionForDb( dbid, m_lgposRedo ) )
    {
        fUpdateCountersOnly = fTrue;
    }

    FMP * const pfmp = g_rgfmp + ifmp;


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
        *pfSkip = fFalse;
    }

    return JET_errSuccess;
}


ERR LOG::ErrLGRICheckRedoConditionInTrx(
    const PROCID    procid,
    const DBID      dbid,
    DBTIME          dbtime,
    OBJID           objidFDP,
    const LR        *plr,
    PIB             **pppib,
    BOOL            *pfSkip )
{
    ERR             err;
    PIB             *ppib;
    BOOL            fUpdateCountersOnly     = fFalse;

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
            err = JET_errSuccess;

            Assert( pagetrimTrimmed == pcsrEmpty->PagetrimState() );
            Assert( FRedoMapNeeded( ifmp ) );
            Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( rgemptypage[i].pgno ) );
            Assert( !pcsrEmpty->FLatched() );
        }
        else
        {
            Call( err );
            Assert( latchRIW == pcsrEmpty->Latch() );
        }

        Assert( rgemptypage[i].pgno == pcsrEmpty->Pgno() );

        fRedoNeeded = FLGNeedRedoCheckDbtimeBefore( *pcsrEmpty,
                                                    plremptytree->le_dbtime,
                                                    rgemptypage[i].dbtimeBefore,
                                                    &err );

        if ( err < 0 )
        {
            pcsrEmpty->ReleasePage();
            goto HandleError;
        }

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
    const OBJID     objidFDP    = plrnode->le_objidFDP;
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
                (LR *) plrnode,
                &ppib,
                &fSkip ) );
    if ( fSkip )
        return JET_errSuccess;

    if ( ppib->Level() > plrnode->level )
    {
        Assert( plrnode->FConcCI()
            || lrtypUndoInfo == plrnode->lrtyp
            || lrtypUndo == plrnode->lrtyp );
    }
    else
    {
        Assert( ppib->Level() == plrnode->level
            || lrtypUndoInfo == plrnode->lrtyp );
    }

    INST    *pinst = PinstFromPpib( ppib );
    IFMP    ifmp = pinst->m_mpdbidifmp[ dbid ];
    FUCB    *pfucb;

    CallR( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, &pfucb ) );

    Assert( pfucb->ppib == ppib );

    CSR     csr;
    BOOL    fRedoNeeded;

    err = ErrLGIAccessPage( ppib, &csr, ifmp, pgno, plrnode->le_objidFDP, fFalse );

    if ( errSkipLogRedoOperation == err )
    {
        err = JET_errSuccess;

        Assert( pagetrimTrimmed == csr.PagetrimState() );
        Assert( FRedoMapNeeded( ifmp ) );
        Assert( g_rgfmp[ifmp].PLogRedoMapZeroed() && g_rgfmp[ifmp].PLogRedoMapZeroed()->FPgnoSet( pgno ) );
        Assert( !csr.FLatched() );
    }
    else
    {
        Call( err );
        Assert( latchRIW == csr.Latch() );
    }


    fRedoNeeded = lrtypUndoInfo == plrnode->lrtyp ?
                                fFalse :
                                FLGNeedRedoCheckDbtimeBefore( csr, dbtime, plrnode->le_dbtimeBefore, &err );

    Call( err );

    err = JET_errSuccess;

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
            LRUNDOINFO  *plrundoinfo = (LRUNDOINFO *) plrnode;
            RCE         *prce;
            const OPER  oper = plrundoinfo->le_oper;

            Assert( !fRedoNeeded );

            const TRX   trxOld      = ppib->trxBegin0;
            const LEVEL levelOld    = ppib->Level();

            Assert( pfucb->ppib == ppib );

            if ( 0 == ppib->Level() )
            {
                goto HandleError;
            }

            Call( ppib->ErrDeregisterDeferredRceid( plrundoinfo->le_rceid ) );

            Assert( trxOld == plrundoinfo->le_trxBegin0 );
            ppib->SetLevel( min( plrundoinfo->level, ppib->Level() ) );
            ppib->trxBegin0 = plrundoinfo->le_trxBegin0;

            Assert( trxMax == ppib->trxCommit0 );

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

            if ( operReplace == oper )
            {
                pfucb->kdfCurr.Nullify();
                pfucb->kdfCurr.key = bm.key;
                pfucb->kdfCurr.data.SetPv( plrundoinfo->rgbData +
                                             plrundoinfo->CbBookmarkKey() +
                                             plrundoinfo->CbBookmarkData() );
                pfucb->kdfCurr.data.SetCb( plrundoinfo->le_cbData );
            }

            Assert( plrundoinfo->le_pgnoFDP == PgnoFDP( pfucb ) );

            CallJ( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, oper, &prce, &verproxy ), RestorePIB );
            Assert( prceNil != prce );

            if ( prce->FOperReplace() )
            {
                VERREPLACE* pverreplace = (VERREPLACE*)prce->PbData();

                pverreplace->cbMaxSize  = plrundoinfo->le_cbMaxSize;
                pverreplace->cbDelta    = 0;
            }

            VERInsertRCEIntoLists( pfucb, pcsrNil, prce );

        RestorePIB:
            ppib->SetLevel( levelOld );
            ppib->trxBegin0 = trxOld;

            if ( JET_errPreviousVersion == err )
            {
                err = JET_errSuccess;
            }
            Call( err );

            goto HandleError;
        }
            break;

        case lrtypUndo:
        {
            LRUNDO  *plrundo = (LRUNDO *)plrnode;

            CallR( ErrLGRIPpibFromProcid( plrundo->le_procid, &ppib ) );

            Assert( !ppib->FMacroGoing( dbtime ) );

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

            if ( plrinsert->FVersioned() )
            {
                RCE         *prce = prceNil;
                BOOKMARK    bm;

                NDGetBookmarkFromKDF( pfucb, kdf, &bm );
                Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operInsert, &prce, &verproxy ) );
                Assert( prceNil != prce );
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

            if( fRedoNeeded )
            {
                Assert( plrinsert->ILine() == csr.ILine() );
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


                BYTE *rgb;
                BFAlloc( bfasTemporary, (VOID **)&rgb );


                pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );

                kdf.key.prefix.SetCb( pfucb->kdfCurr.key.prefix.Cb() );
                kdf.key.prefix.SetPv( rgb );
                kdf.key.suffix.SetCb( pfucb->kdfCurr.key.suffix.Cb() );
                kdf.key.suffix.SetPv( rgb + kdf.key.prefix.Cb() );
                Assert( FKeysEqual( kdf.key, pfucb->kdfCurr.key ) );

                if( plrfiard->FVersioned() )
                {
                    verproxy.rceid = plrfiard->le_rceidReplace;

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

            BYTE *rgb;
            BFAlloc( bfasTemporary, (VOID **)&rgb );
            pfucb->kdfCurr.key.CopyIntoBuffer( rgb, g_cbPage );

            Assert( FFUCBUnique( pfucb ) );
            pfucb->bmCurr.data.Nullify();

            pfucb->bmCurr.key.prefix.SetCb( pfucb->kdfCurr.key.prefix.Cb() );
            pfucb->bmCurr.key.prefix.SetPv( rgb );
            pfucb->bmCurr.key.suffix.SetCb( pfucb->kdfCurr.key.suffix.Cb() );
            pfucb->bmCurr.key.suffix.SetPv( rgb + pfucb->kdfCurr.key.prefix.Cb() );
            Assert( CmpKey( pfucb->bmCurr.key, pfucb->kdfCurr.key ) == 0 );

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


                if ( plrflagdelete->FVersioned() )
                {
                    Call( ppib->ErrRegisterDeferredRceid( plrnode->le_rceid, pgno ) );
                }

                CallS( err );
                goto HandleError;
            }

            Assert( plrflagdelete->ILine() == csr.ILine() );
            Call( ErrNDGet( pfucb, &csr ) );

            if( plrflagdelete->FVersioned() )
            {
                BOOKMARK    bm;
                RCE         *prce = prceNil;

                NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );
                Call( PverFromPpib( ppib )->ErrVERModify( pfucb, bm, operFlagDelete, &prce, &verproxy ) );
                Assert( prceNil != prce );
                VERInsertRCEIntoLists( pfucb, &csr, prce );
            }

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
                BOOKMARK    bm;
                Assert( plrdelete->ILine() == csr.ILine() );
                Call( ErrNDGet( pfucb, &csr ) );
                NDGetBookmarkFromKDF( pfucb, pfucb->kdfCurr, &bm );

                if ( !FVERActive( pfucb, bm ) )
                {
                    break;
                }

                csr.ReleasePage();

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
        Call( ErrLGIAccessPage( ppib, &csr, ifmp, plrscrub->le_pgno, plrscrub->le_objidFDP, fFalse ) );
        const BOOL fRedo = FLGNeedRedoCheckDbtimeBefore( csr, plrscrub->le_dbtime, plrscrub->le_dbtimeBefore, &err );
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

ERR LOG::ErrLGEvaluateDestructiveCorrectiveLogOptions(
    __in const LONG lgenBad,
    __in const ERR errCondition
    )
{
    ERR     err = errCondition;
    LONG    lGenFirst = 0;
    LONG    lGenLast = 0;

    Assert( errCondition < JET_errSuccess );
    Assert( errCondition == JET_errInvalidLogSequence ||
            FErrIsLogCorruption( errCondition ) );

    Call( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, &lGenFirst, &lGenLast ) );

    ERR errGenRequiredCheck = ErrLGCheckDBGensRequired( lgenBad - 1 );
    Assert( errGenRequiredCheck != JET_wrnCommittedLogFilesLost );

    if ( errGenRequiredCheck != JET_errCommittedLogFilesMissing )
    {
        Call( errCondition );
    }


    if ( !m_fIgnoreLostLogs || !BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) )
    {


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



    Assert( m_fIgnoreLostLogs );
    Assert( lGenFirst < lgenBad );
    Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration < lgenBad );
    Assert( ErrLGCheckDBGensRequired( lgenBad - 1 ) == JET_errSuccess ||
            ErrLGCheckDBGensRequired( lgenBad - 1 ) == JET_errCommittedLogFilesMissing );
    if ( !m_fIgnoreLostLogs ||
        ( lGenFirst >= lgenBad ) ||
        ( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= lgenBad ) ||
        ( ErrLGCheckDBGensRequired( lgenBad - 1 ) != JET_errSuccess &&
          ErrLGCheckDBGensRequired( lgenBad - 1 ) != JET_errCommittedLogFilesMissing ) )
    {
        AssertSz( fFalse, " This would be an unexpected condition." );
        Call( errCondition );
    }


    if ( FErrIsLogCorruption( errCondition ) )
    {
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
    

    CallR( m_pLogStream->ErrLGRemoveCommittedLogs( lgenBad ) );

    m_fLostLogs = fTrue;

    err = ErrERRCheck( JET_wrnCommittedLogFilesRemoved );


HandleError:

    Assert( err != JET_errSuccess );

    if ( err < JET_errSuccess &&
            err != JET_errCommittedLogFilesMissing &&
            err != JET_errCommittedLogFileCorrupt )
    {
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
    BOOL    fJetLog = fFalse;
    WCHAR   wszOldLogName[ IFileSystemAPI::cchPathMax ];
    LGPOS   lgposOldLastRec = m_pLogReadBuffer->LgposFileEnd();
    QWORD checksumAccPrevLog = m_pLogStream->GetAccumulatedSectorChecksum();

    
    if ( m_pLogStream->FLGRedoOnCurrentLog() )
    {
        Assert( m_wszLogCurrent != m_wszRestorePath );

        
        *pfNSNextStep = fNSGotoDone;
        OnNonRTM( m_fRedidAllLogs = fTrue );
        err = JET_errSuccess;
        goto CheckGenMaxReq;
    }

    
    m_pLogStream->LGCloseFile();

    OSStrCbCopyW( wszOldLogName, sizeof( wszOldLogName ), m_pLogStream->LogName() );

    lgen = m_pLogStream->GetCurrentFileGen() + 1;

    if ( m_wszLogCurrent == m_wszTargetInstanceLogPath && m_lGenHighTargetInstance == m_pLogStream->GetCurrentFileGen() )
    {
        Assert ( L'\0' != m_wszTargetInstanceLogPath[0] );
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
            if ( m_wszTargetInstanceLogPath[0] && m_wszLogCurrent != m_wszTargetInstanceLogPath )
                m_wszLogCurrent = m_wszTargetInstanceLogPath;
            else
                m_wszLogCurrent = SzParam( m_pinst, JET_paramLogFilePath );

            err = m_pLogStream->ErrLGTryOpenLogFile( JET_OpenLogForRedo, eArchiveLog, lgen );
        }

    }

    if ( err == JET_errFileNotFound )
    {

        err = m_pLogStream->ErrLGTryOpenLogFile( JET_OpenLogForRedo, eCurrentLog, lgen );

        if ( JET_errSuccess == err )
        {
            fJetLog = fTrue;
        }
    }

    if ( err < 0 )
    {

        if ( err == JET_errFileNotFound )
        {
            if ( FLGRecoveryLgposStop() )
            {
                Assert( m_lgposRecoveryStop.lGeneration >= ( lgen - 1 ) );

                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"07dd83a7-c6e8-413a-beb9-e934ea597918" );
                return ErrERRCheck( JET_errMissingLogFile );
            }

            LONG    lgenHigh = 0;
            
            CallR ( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenHigh ) );

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


            if ( m_fHardRestore || m_fIgnoreLostLogs )
            {
                err = ErrLGCheckDBGensRequired( lgen - 1 );
                if ( m_fIgnoreLostLogs &&
                        ( err == JET_errSuccess || err == JET_errCommittedLogFilesMissing ) )
                {
                    m_pLogStream->ResetRemovedLogs();
                    m_fLostLogs = ( err == JET_errCommittedLogFilesMissing );
                    err = JET_errSuccess;
                }
                CallR( err );
            }

            const ERR errMissingCurrentLog = m_fHardRestore || m_pLogStream->FTempLogExists() ? JET_errSuccess : ErrERRCheckDefault( JET_errMissingLogFile );

            err = ErrLGOpenLogMissingCallback( m_pinst, m_pLogStream->LogName(), errMissingCurrentLog, lgen, fTrue, JET_MissingLogContinueToUndo );
            Assert( err != JET_errFileNotFound );
            CallR( err );

            OnNonRTM( m_fRedidAllLogs = fTrue );

            m_pLogStream->LGMakeLogName( eArchiveLog, lgen - 1 );

            *pfNSNextStep = fNSGotoDone;
            return JET_errSuccess;
        }

        
        Assert( !m_pLogStream->FLGFileOpened() );
        return err;
    }


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

    if ( !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoverySilentRedoDone ) )
    {
        BOOL fCallback = ( ErrLGNotificationEventConditionCallback( m_pinst, STATUS_REDO_ID ) >= JET_errSuccess );
        if ( fCallback )
        {
            m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoverySilentRedoEnd = m_lgposRedo;
            m_pinst->m_isdlInit.Trigger( eInitLogRecoverySilentRedoDone );
        }
    }

    m_pLogStream->SaveCurrentFileHdr();

    
    CallR( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID ) );

    const ULONG ulMajorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor;
    const ULONG ulMinorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMinor;
    const ULONG ulUpdateMajorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMajor;
    const ULONG ulUpdateMinorCurrLog = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulUpdateMinor;

    if ( !FSameTime( &tmOldLog, &m_pLogStream->GetCurrentFileHdr()->lgfilehdr.tmPrevGen ) ||
         ( lgen != m_pLogStream->GetCurrentFileGen() ) )
    {


        Assert( !m_fIgnoreLostLogs || !m_fHardRestore );

        if ( m_fHardRestore && BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) && fJetLog && m_pLogStream->GetCurrentFileGen() < lgen )
        {

            #ifdef DEBUG
            WCHAR   wszNameDebug[IFileSystemAPI::cchPathMax];
            m_pLogStream->LGMakeLogName( wszNameDebug, sizeof(wszNameDebug), eArchiveLog );
            Assert( 0 == wcscmp( wszNameDebug, m_pLogStream->LogName() ) );
            #endif

            CallR( m_pLogStream->ErrLGRRestartLOGAtLogGen( lgen, fTrue ) );
            fJetLog = fFalse;
            *pfNSNextStep = fNSGotoDone;
            return err;
        }

        CallR( ErrLGEvaluateDestructiveCorrectiveLogOptions( lgen, ErrERRCheck( JET_errInvalidLogSequence ) ) );

        Assert( err == JET_wrnCommittedLogFilesRemoved );
        Assert( m_fLostLogs );
        err = JET_errSuccess;
        *pfNSNextStep = fNSGotoDone;
        return err;
    }


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
            FMP * const pfmp = &g_rgfmp[ ifmp ];
            pfmp->SetOlderDemandExtendDb();
        }
    }

    le_lgposFirstT.le_lGeneration = m_pLogStream->GetCurrentFileGen();
    le_lgposFirstT.le_isec = (WORD)m_pLogStream->CSecHeader();
    le_lgposFirstT.le_ib = 0;

    m_pLogReadBuffer->ResetLgposFileEnd();

    err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue );
    if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
    {

        if ( FErrIsLogCorruption( err ) )
        {
            err = ErrLGEvaluateDestructiveCorrectiveLogOptions( lgen, ErrERRCheck( err ) );
            if ( err >= JET_errSuccess )
            {
                Assert( err == JET_wrnCommittedLogFilesRemoved );
                Assert( m_fLostLogs );
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


    QWORD cbSize;
    CallR( m_pLogStream->ErrFileSize( &cbSize ) );
    if ( cbSize != m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec * QWORD( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile ) )
    {
        m_fAbruptEnd = fTrue;
    }

    CallR( m_pLogReadBuffer->ErrLGLocateFirstRedoLogRecFF( &le_lgposFirstT, (BYTE **)pplr ) );
    CallS( err );
    *pfNSNextStep = fNSGotoCheck;



    if ( m_fHardRestore && lgen <= m_lGenHighRestore
         && m_fAbruptEnd )
    {
        goto AbruptEnd;
    }

CheckGenMaxReq:
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
                Assert( !pfmpT->FSkippedAttach() || m_fHardRestore || m_irstmapMac || !m_fReplayMissingMapEntryDB );
                continue;
            }

            LONG lGenCommitted = pfmpT->Pdbfilehdr()->le_lGenMaxCommitted;
            LONG lGenRequired = pfmpT->Pdbfilehdr()->le_lGenMaxRequired;
            LOGTIME tmCreate;
            LONG lGenCurrent = m_pLogStream->GetCurrentFileGen( &tmCreate );
            BOOL fLogtimeGenMaxRequiredEfvEnabled = ( pfmpT->ErrDBFormatFeatureEnabled( JET_efvLogtimeGenMaxRequired ) == JET_errSuccess );

            PdbfilehdrReadOnly pdbfilehdr = pfmpT->Pdbfilehdr();

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

    CallS( err );

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


    if ( m_fHardRestore || m_irstmapMac > 0 )
    {
        err = ErrReplaceRstMapEntryBySignature( wszDbName, &pAttachInfo->signDb );
        if ( JET_errFileNotFound == err )
        {
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

    err = ErrGetDestDatabaseName( (WCHAR*)wszDbName, m_wszRestorePath, m_wszNewDestination, pirstmap, plgstat );
    if ( JET_errFileNotFound == err )
    {
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

        Assert( *pirstmap >= 0 );
    }

    psrtmap = ( *pirstmap >= 0 ) ? &m_rgrstmap[ *pirstmap ] : NULL;


    Call( ErrDBParseDbParams(
                psrtmap ? psrtmap->rgsetdbparam : NULL,
                psrtmap ? psrtmap->csetdbparam : 0,
                NULL,
                &pctCachePriority,
                &grbitShrinkDatabaseOptions,
                &dtickShrinkDatabaseTimeQuota,
                &cpgShrinkDatabaseSizeLimit,
                &fLeakReclaimerEnabled,
                &dtickLeakReclaimerTimeQuota
                ) );


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

    if ( err == JET_wrnDatabaseAttached )
    {
        Call( ErrERRCheck( JET_errDatabaseDuplicate ) );
    }

    FMP     *pfmpT;
    pfmpT = &g_rgfmp[ *pifmp ];

    Assert( !pfmpT->Pfapi() );
    Assert( NULL == pfmpT->Pdbfilehdr() );
    Assert( pfmpT->FInUse() );
    Assert( pfmpT->Dbid() == pAttachInfo->Dbid() );

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

        *predoattach = redoattachCreate;
        err = JET_errSuccess;
        goto HandleError;
    }

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
        Assert( JET_errReadVerifyFailure == err || pdbfilehdr->Dbstate() != JET_dbstateJustCreated );
        Assert( JET_errReadVerifyFailure == err || pdbfilehdr->Dbstate() != JET_dbstateIncrementalReseedInProgress );
        Assert( JET_errReadVerifyFailure == err || pdbfilehdr->Dbstate() != JET_dbstateRevertInProgress );

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
                *predoattach = redoattachDeferAccessDenied;
                __fallthrough;

            case JET_errFileNotFound:
            case JET_errInvalidPath:
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
            reason = eDARLogSignatureMismatch;
            goto HandleError;
        }
        else
        {
            Call( ErrIODeleteDatabase( m_pinst->m_pfsapi, ifmp ) );

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
                *predoattach = redoattachDeferConsistentFuture;
                reason = eDARConsistentFuture;
            }
            else
            {
                Call( ErrLGRICheckAttachNow( pdbfilehdr, wszDbName ) );

                Assert( 0 == CmpLgpos( &lgposMin, &pdbfilehdr->le_lgposConsistent ) );
                Assert( JET_dbstateDirtyShutdown == pdbfilehdr->Dbstate() ||
                        JET_dbstateDirtyAndPatchedShutdown == pdbfilehdr->Dbstate() );
                *predoattach = redoattachNow;
            }
        }
        else
        {

            reason = eDARDbSignatureMismatch;
        }
    }
    else if ( i > 0 )
    {
        reason = eDARAttachFuture;
    }
    else
    {
        reason = eDARDbSignatureMismatch;
        Assert( 0 != memcmp( &pdbfilehdr->signDb, &patchchk->signDb, sizeof(SIGNATURE) ) );
        
        if ( !pfmp->FOverwriteOnCreate() )
        {
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

            UtilReportEvent( eventWarning, LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_PARTIALLY_ERROR_ID, 3, rgszT, sizeof( szTTime ), szTTime, m_pinst );
        }
        else
        {
            Call( ErrIODeleteDatabase( m_pinst->m_pfsapi, ifmp ) );

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

    *predoattach = redoattachDefer;
    reason = eDARUnknown;

    err = ErrUtilReadShadowedHeader(
            m_pinst,
            m_pinst->m_pfsapi,
            wszDbName,
            (BYTE*)pdbfilehdr,
            g_cbPage,
            OffsetOf( DBFILEHDR, le_cbPageSize ) );

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
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"9106f5c1-2f93-479b-a12a-c93c6ab3de68" );
            goto HandleError;
        }
    }
    else if ( err >= 0 )
    {
        if ( pdbfilehdr->Dbstate() == JET_dbstateJustCreated )
        {
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
                *predoattach = redoattachDeferAccessDenied;
                __fallthrough;

            case JET_errFileNotFound:
            case JET_errInvalidPath:
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

    if ( ( pdbfilehdr->Dbstate() == JET_dbstateDirtyAndPatchedShutdown ) && !pfmp->FPendingRedoMapEntries() )
    {
        if ( !fMatchingSignDb || ( !fMatchingSignLog && !fMatchingLoggedSignLog ) )
        {
            goto PostDirtyAndPatchedFixup;
        }

        if ( ( pdbfilehdr->le_lGenMinRequired <= 0 ) ||
             ( 0 == CmpLgpos( &pdbfilehdr->le_lgposAttach, &lgposMin ) ) ||
             ( 0 == CmpLgpos( &patchchk->lgposAttach, &lgposMin ) ) )
        {
            reason = eDARHeaderStateBad;
            err = ErrERRCheck( JET_errDatabaseCorrupted );
            goto HandleError;
        }

        if ( ( pdbfilehdr->le_lGenMinRequired > pdbfilehdr->le_lgposAttach.le_lGeneration ) ||
             ( pdbfilehdr->le_lGenMinRequired < patchchk->lgposAttach.lGeneration ) ||
             ( CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->le_lgposAttach ) > 0 ) )
        {
            if ( ( pdbfilehdr->le_lGenMinRequired < pdbfilehdr->le_lgposAttach.le_lGeneration ) &&
                 ( CmpLgpos( &patchchk->lgposAttach, &pdbfilehdr->le_lgposAttach ) > 0 ) &&
                 ( CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->le_lgposConsistent ) != 0 ) )
            {
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

        Assert( fMatchingLoggedSignLog || redoattachmodeAttachDbLR == redoattachmode );
    }
    else if ( fMatchingLoggedSignLog )
    {
        if ( 0 == CmpLgpos( &patchchk->lgposConsistent, &pdbfilehdr->le_lgposConsistent ) )
        {
            if ( fMatchingSignDb )
            {
                Assert( !pfmp->FReadOnlyAttach() );
                Call( ErrLGRICheckAttachNow( pdbfilehdr, wszDbName ) );

                Assert( redoattachmodeAttachDbLR == redoattachmode );
                Assert( 0 == CmpLgpos( &patchchk->lgposAttach, &m_lgposRedo ) );

                Assert( 0 == CmpLgpos( patchchk->lgposAttach, pfmp->LgposAttach() ) );
                pfmp->SetLgposAttach( patchchk->lgposAttach );
                DBISetHeaderAfterAttach( pdbfilehdr, patchchk->lgposAttach, NULL, ifmp, fFalse );
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
        reason = eDARLogSignatureMismatch;
        err = JET_errSuccess;
        goto HandleError;
    }

    const INT   i   = CmpLgpos( &pdbfilehdr->le_lgposConsistent, &patchchk->lgposConsistent );

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
                        LGReportAttachedDbMismatch( m_pinst, wszDbName, fFalse );

                        FireWall( "AttachedDbMismatchOnRedoAttachDb" );
                        err = ErrERRCheck( JET_errAttachedDatabaseMismatch );
                        goto HandleError;
                    }
                }
            }
            else
            {
                reason = eDARAttachFuture;
            }
        }
        else
        {
            reason = eDARDbSignatureMismatch;
#ifdef DEBUG
            if ( 0 == CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) )
            {
            }
            else
            {

            }
#endif
        }
    }
    else if ( i > 0
        && ( redoattachmodeInitBeforeRedo == redoattachmode
            || CmpLgpos( &pdbfilehdr->le_lgposConsistent, &m_lgposRedo ) > 0 ) )
    {
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
            reason = eDARConsistentTimeMismatch;

            if ( !pfmp->FIgnoreDeferredAttach() )
            {
                LGReportConsistentTimeMismatch( m_pinst, wszDbName );
                err = ErrERRCheck( JET_errConsistentTimeMismatch );
                goto HandleError;
            }
        }
        else
        {
            Assert( 0 != CmpLgpos( &pdbfilehdr->le_lgposConsistent, &lgposMin ) );


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
    const SIGNATURE             *psignLogged,
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

ERR LOG::ErrLGRICheckAttachNow(
    DBFILEHDR   * pdbfilehdr,
    const WCHAR * wszDbName )
{
    ERR         err                 = JET_errSuccess;
    const LONG  lGenMinRequired     = pdbfilehdr->le_lGenMinRequired;
    const LONG  lGenCurrent         = m_pLogStream->GetCurrentFileGen();

    if ( lGenMinRequired
        && JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->Dbstate()
        && lGenMinRequired < lGenCurrent )
    {
        err = ErrERRCheck( JET_errRequiredLogFilesMissing );
    }

    else if ( pdbfilehdr->bkinfoFullCur.le_genLow && !m_fHardRestore )
    {


        CallS( err );
        if ( 0 == pdbfilehdr->bkinfoFullCur.le_genHigh )
        {
            err = ErrLGSignalErrorConditionCallback( m_pinst, JET_errSoftRecoveryOnBackupDatabase );
            if ( err < JET_errSuccess )
            {
                const WCHAR *rgszT[1];
                rgszT[0] = wszDbName;
                
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

    
    CallR( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
    CallSx( err, JET_wrnDatabaseAttached );

    
    pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genLow = m_lGenLowRestore;
    pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genHigh = m_lGenHighRestore;

    pfmp->SetDbtimeCurrentDuringRecovery( pfmp->DbtimeLast() );
    pfmp->SetAttachedForRecovery();

    if( CmpLogFormatVersion( FmtlgverLGCurrent( this ), fmtlgverInitialDbSizeLoggedMain ) < 0 )
    {
        pfmp->SetOlderDemandExtendDb();
    }

    Assert( pfmp->Pdbfilehdr()->le_objidLast == pfmp->ObjidLast() );

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

    FMP::EnterFMPPoolAsWriter();
    pfmp->ResetFlags();
    pfmp->SetAttached();
    pfmp->SetAttachedForRecovery();
    FMP::LeaveFMPPoolAsWriter();

    pfmp->SetDatabaseSizeMax( cpgDatabaseSizeMax );
    Assert( pfmp->CpgDatabaseSizeMax() != 0xFFFFFFFF );

    Assert( !pfmp->FVersioningOff() );
    pfmp->ResetVersioningOff();

    FMP::EnterFMPPoolAsWriter();
    Assert( !pfmp->FLogOn() );
    pfmp->SetLogOn();
    FMP::LeaveFMPPoolAsWriter();

    Assert( !pfmp->FReadOnlyAttach() );
    BOOL fUpdateHeader = fFalse;
    OnDebug( pfmp->SetDbHeaderUpdateState( FMP::DbHeaderUpdateState::dbhusHdrLoaded ) );
    
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

        if ( g_rgfmp[ifmp].FHardRecovery() )
        {
            const INT   irstmap = IrstmapSearchNewName( wszDbName );

            if ( irstmap >= 0 )
            {
                fKeepBackupInfo = ( 0 == UtilCmpFileName( m_rgrstmap[irstmap].wszDatabaseName, m_rgrstmap[irstmap].wszNewDatabaseName ) );
            }
            else
            {
                Assert( ErrLGSignalErrorConditionCallback( m_pinst, JET_errSoftRecoveryOnBackupDatabase ) == JET_errSuccess );
                fKeepBackupInfo = fTrue;
            }
        }

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
            FireWall( OSFormat( "InvalidDbStateOnRedoAttachDb:%d", (INT)pdbfilehdr->Dbstate() ) );
            LOGTIME tmCreate;
            LONG lGenCurrent = m_pLogStream->GetCurrentFileGen( &tmCreate );
            pdbfilehdr->SetDbstate( JET_dbstateDirtyShutdown, lGenCurrent, &tmCreate, fTrue );
            fUpdateHeader = fTrue;
        }
    }

    {
    OnDebug( PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr() );
    Assert( 0 == pdbfilehdr->bkinfoFullCur.le_genLow ||
            m_fHardRestore ||
            ( ErrLGSignalErrorConditionCallback( m_pinst, JET_errSoftRecoveryOnBackupDatabase ) == JET_errSuccess ) ||
            ( 0 != pdbfilehdr->bkinfoFullCur.le_genLow && 0 != pdbfilehdr->bkinfoFullCur.le_genHigh ) );
    }

    Call( ErrDBIValidateUserVersions( m_pinst, wszDbName, ifmp, pfmp->Pdbfilehdr().get(), NULL ) );

    const BOOL fLowerMinReqLogGenOnRedo = ( pfmp->ErrDBFormatFeatureEnabled( JET_efvLowerMinReqLogGenOnRedo ) == JET_errSuccess );
    {

    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();
    Assert( ( pdbfilehdr->le_lGenMinRequired > 0 ) == ( pdbfilehdr->le_lGenMaxRequired > 0 ) );

    const LONG lGenCurrent = m_pLogStream->GetCurrentFileGen();
    Assert( lGenCurrent > 0 );
    Assert( lGenCurrent >= pdbfilehdr->le_lgposAttach.le_lGeneration );

    if ( ( CmpLgpos( pdbfilehdr->le_lgposLastResize, lgposMin ) != 0 ) &&
         ( pdbfilehdr->le_lGenMaxRequired > 0 ) &&
         ( pdbfilehdr->le_lgposLastResize.le_lGeneration > pdbfilehdr->le_lGenMaxRequired ) )
    {
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
    if ( pdbfilehdr->le_lGenMinConsistent != pdbfilehdr->le_lGenMinRequired )
    {
        pdbfilehdr->le_lGenMinConsistent = pdbfilehdr->le_lGenMinRequired;
        fUpdateHeader = fTrue;
    }
    }

    Call( pfmp->ErrCreateFlushMap( JET_bitNil ) );

    if ( fUpdateHeader )
    {
        Assert( pfmp->Pdbfilehdr()->le_objidLast > 0 );
        Call( ErrUtilWriteAttachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, wszDbName, pfmp ) );
    }

    OnDebug( const ULONG dbstate = pfmp->Pdbfilehdr()->Dbstate() );
    Assert( JET_dbstateDirtyShutdown == dbstate ||
            JET_dbstateDirtyAndPatchedShutdown == dbstate );

    
    if ( m_fHardRestore )
    {
        pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genLow = m_lGenLowRestore;
        pfmp->PdbfilehdrUpdateable()->bkinfoFullCur.le_genHigh = m_lGenHighRestore;
    }

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

    FMP::EnterFMPPoolAsWriter();
    pfmp->SetWaypoint( pfmp->LgposAttach().lGeneration );
    FMP::LeaveFMPPoolAsWriter();

    Assert( !pfmp->FContainsDataFromFutureLogs() );

    if ( m_lgposRedo.lGeneration == 0 || m_lgposRedo.lGeneration <= pfmp->Pdbfilehdr()->le_lGenMaxRequired )
    {
        FMP::EnterFMPPoolAsWriter();
        pfmp->SetContainsDataFromFutureLogs();
        FMP::LeaveFMPPoolAsWriter();
    }

    pfmp->SetAllowHeaderUpdate();

    if ( !pfmp->FContainsDataFromFutureLogs() )
    {
        CallR( ErrLGDbAttachedCallback( m_pinst, pfmp ) );
    }

    if( CmpLogFormatVersion( FmtlgverLGCurrent( this ), fmtlgverInitialDbSizeLoggedMain ) < 0 )
    {
        pfmp->SetOlderDemandExtendDb();
    }

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

    Assert( !pfmp->FVersioningOff() );
    pfmp->ResetVersioningOff();

    Assert( !pfmp->FReadOnlyAttach() );

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


    LGPOS lgposT = plrlb->phaseDetails.le_lgposMark;

    m_pinst->m_pbackup->BKSetLgposFullBackup( lgposMin );
    m_pinst->m_pbackup->BKSetLgposIncBackup( lgposMin );
    m_pinst->m_pbackup->BKSetLogsTruncated( fFalse );

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

    pfucb->u.pfcb->SetObjidFDP( objidFDP );
    if ( fUnique )
        pfucb->u.pfcb->SetUnique();
    else
        pfucb->u.pfcb->SetNonUnique();

    pfucb->u.pfcb->Unlock();

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

    Assert( latchWrite == csr.Latch() );
    csr.SetDbtime( dbtime );
    csr.ReleasePage();

HandleError:
    Assert( !csr.FLatched() );
    return err;
}

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

    pfucb->u.pfcb->SetObjidFDP( objidFDP );
    if ( fUnique )
        pfucb->u.pfcb->SetUnique();
    else
        pfucb->u.pfcb->SetNonUnique();

    pfucb->u.pfcb->Unlock();

    Call( csr.ErrGetNewPreInitPageForRedo(
                    pfucb->ppib,
                    pfucb->ifmp,
                    pgnoFDP,
                    objidFDP,
                    dbtime ) );
    csr.ConsumePreInitPage( fPageFlags | CPAGE::fPageRoot | CPAGE::fPageLeaf );

    sph.SetPgnoParent( pgnoParent );
    sph.SetCpgPrimary( cpgPrimary );

    Assert( sph.FSingleExtent() );
    Assert( sph.FUnique() );

    sph.SetMultipleExtent();

    if ( !fUnique )
    {
        sph.SetNonUnique();
    }

    sph.SetPgnoOE( pgnoOE );
    Assert( sph.PgnoOE() == pgnoFDP + 1 );

    SPIInitPgnoFDP( pfucb, &csr, sph );

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
        Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( pgnoFDP ) );
        err = JET_errSuccess;
        fRedoRoot = fFalse;
    }
    else
    {
        Call( err );

        fRedoRoot = FLGNeedRedoCheckDbtimeBefore( csrRoot, dbtime, plrconvertfdp->le_dbtimeBefore, &err );

        Call ( err );
    }

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
        err = JET_errSuccess;
        goto HandleError;
    }

    Assert( sph.FSingleExtent() );
    sph.SetMultipleExtent();
    Assert( pgnoOE == pgnoSecondaryFirst );
    sph.SetPgnoOE( pgnoSecondaryFirst );

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
        Assert( FAssertLGNeedRedo( csrRoot, dbtime, plrconvertfdp->le_dbtimeBefore ) );
        csrRoot.UpgradeFromRIWLatch();
    }

    Assert( FFUCBOwnExt( pfucb ) );
    FUCBResetOwnExt( pfucb );

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
                Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( plrNode->le_pgno ) );
#endif
                break;
            }
            case JET_errOutOfMemory:
                break;

            default:
                Assert( JET_errSuccess == err );
        }
        
        CallR( err );
        break;
    }


    

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
            }
            else
            {
                Assert( lrtypBegin == plr->lrtyp );
            }

            
            while ( ppib->Level() < plrbeginDT->levelBeginFrom + plrbeginDT->clevelsToBegin )
            {
                VERBeginTransaction( ppib, 61477 );
            }

            
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
            if ( 0 == plrcommit0->levelCommitTo &&
                 ( !m_pinst->m_fTrxNewestSetByRecovery ||
                   TrxCmp( plrcommit0->le_trxCommit0, m_pinst->m_trxNewest ) > 0 ) )
            {
                AtomicExchange( (LONG *)&m_pinst->m_trxNewest, plrcommit0->le_trxCommit0 );
                m_pinst->m_fTrxNewestSetByRecovery = fTrue;
            }

            break;
        }

        
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
                NULL,
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

        LGRITraceRedo( plrcreatesefdp );
        CallR( ErrLGIRedoFDPPage( m_pctablehash, ppib, plrcreatesefdp ) );
        break;
    }

    case lrtypConvertFDP:
        FireWall( OSFormat( "DeprecatedLrType:%d", (INT)plr->lrtyp ) );

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

        TICK tickStart = TickOSTimeCurrent();
        while ( TrxCmp( trxCommitted, TrxOldest( m_pinst ) ) >= 0 )
        {
            CLockDeadlockDetectionInfo::AssertCleanApiExit( 0, 0, 0 );
            m_pinst->m_sigTrxOldestDuringRecovery.FWait( cmsecMaxReplayDelayDueToReadTrx );
        }

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
        
        const LRPAGEPATCHREQUEST * const plrpagepatchrequest = (LRPAGEPATCHREQUEST *)plr;
        CallR( ErrLGRIRedoPagePatch( plrpagepatchrequest ) );
        
        break;
    }

    case lrtypScanCheck:
    case lrtypScanCheck2:
    {
        LRSCANCHECK2 lrscancheck;
        lrscancheck.InitScanCheck( plr );

        if ( FLGRICheckRedoScanCheck( &lrscancheck, fFalse  ) )
        {
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

    case lrtypReAttach:
    {
        const LRREATTACHDB * const plrreattach = (LRREATTACHDB *)plr;

        IFMP ifmp = m_pinst->m_mpdbidifmp[ plrreattach->le_dbid ];
        if ( ifmp >= g_ifmpMax )
        {
            break;
        }

        FMP *pfmp = &g_rgfmp[ifmp];
        Assert( NULL != pfmp );
        Assert( pfmp->FInUse() );

        if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
        {
            break;
        }

        Assert( NULL != pfmp->Pdbfilehdr() );
        Assert( NULL != pfmp->Patchchk() );

        {
        PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

        pdbfilehdr->le_lgposLastReAttach  = m_lgposRedo;
        LGIGetDateTime( &pdbfilehdr->logtimeLastReAttach );
        }

        CallR( ErrUtilWriteAttachedDatabaseHeaders( m_pinst, m_pinst->m_pfsapi, pfmp->WszDatabaseName(), pfmp,  pfmp->Pfapi() ) );

        if ( !pfmp->FContainsDataFromFutureLogs() )
        {
            CallR( ErrLGDbAttachedCallback( m_pinst, pfmp ) );
        }

        if ( UlParam( m_pinst, JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryPassiveScan )
        {
            CallR( pfmp->ErrStartDBMScan() );
        }

        break;
    }

    case lrtypSignalAttachDb:
    {
        const LRSIGNALATTACHDB * const plrsattachdb = (LRSIGNALATTACHDB *)plr;

        IFMP ifmp = m_pinst->m_mpdbidifmp[ plrsattachdb->Dbid() ];
        if ( ifmp >= g_ifmpMax )
        {
            break;
        }

        FMP *pfmp = &g_rgfmp[ifmp];
        Assert( NULL != pfmp );
        Assert( pfmp->FInUse() );

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

    case lrtypCommitCtx:
    {
        const LRCOMMITCTX * const plrCommitC = (LRCOMMITCTX *)plr;
        if ( plrCommitC->FCallbackNeeded() )
        {
            CallR( ErrLGCommitCtxCallback( m_pinst, plrCommitC->PbCommitCtx(), plrCommitC->CbCommitCtx() ) );
        }
        break;
    }

    case lrtypNewPage:
    {
        const LRNEWPAGE* const plrnewpage = (LRNEWPAGE *)plr;
        CallR( ErrLGRIRedoNewPage( plrnewpage ) );
        break;
    }

    case lrtypMacroInfo:
    case lrtypMacroInfo2:
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
} 

    return JET_errSuccess;
}

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
    Assert( FAssertLGNeedRedo( psplitPath->csr,dbtime, psplitPath->dbtimeBefore )
        || !FBTISplitPageBeforeImageRequired( psplit )
        || fDebugHasPageBeforeImage );

    if ( psplit->clines < 0 || psplit->clines > 1000000 )
    {
        OSUHAEmitFailureTag( PinstFromPfucb( pfucb ), HaDbFailureTagRecoveryRedoLogCorruption, L"2dfb97c9-80ee-4438-ba68-0d4953cf09ad" );
        return ErrERRCheck( JET_errLogFileCorrupt );
    }

    AllocR( psplit->rglineinfo = new LINEINFO[psplit->clines] );
    memset( psplit->rglineinfo, 0, sizeof( LINEINFO ) * psplit->clines );

    if ( !FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err ) )
    {
        CallS( err );

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
            psplit->rglineinfo[ilineTo].kdf = kdf;

            continue;
        }

        psplitPath->csr.SetILine( ilineFrom );

        Call( ErrNDGet( pfucb, &psplitPath->csr ) );

        if ( ilineTo == psplit->ilineOper &&
             splitoperNone != psplit->splitoper )
        {
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

    if ( fXpressed )
    {
        INT cbActual = 0;
        CallR( ErrPKDecompressData( data, ifmp, pgno, pbDataDecompressed, cbPage, &cbActual ) );
        data.SetPv( pbDataDecompressed );
        data.SetCb( cbActual );
    }

    if ( fDehydrated )
    {
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


    psplit->pgnoNew     = plrsplit->le_pgnoNew;
    Assert( g_rgfmp[ifmp].Dbid() == dbid );

    BOOL fRedoSplitPage                     = FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, plrsplit->le_dbtimeBefore, &err );
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
        
        Assert( FAssertLGNeedRedo( psplitPath->csr, plrsplit->le_dbtime, plrsplit->le_dbtimeBefore ) );
        fRedoSplitPage = fTrue;
    }
    Assert( !( fRedoNewPage && !fRedoSplitPage && fPageBeforeImageRequired ) );
    
    if ( fRedoNewPage )
    {
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
            Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( pgnoRight ) );
            err = JET_errSuccess;
        }
        else
        {
            Call( err );
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

    psplitPath->psplit = psplit;
    psplit->psplitPath = psplitPath;

    if ( fRedoNewPage )
    {
        Assert( latchWrite == psplit->csrNew.Latch() );
    }
    else
    {
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

    CallR( ErrBTINewSplitPath( ppsplitPath ) );

    SPLITPATH *psplitPath = *ppsplitPath;

    err = ErrLGIAccessPage( ppib, &psplitPath->csr, ifmp, pgnoSplit, objidFDP, fFalse );

    if ( errSkipLogRedoOperation == err )
    {
        Assert( pagetrimTrimmed == psplitPath->csr.PagetrimState() );
        Assert( FRedoMapNeeded( ifmp ) );
        Assert( g_rgfmp[ ifmp ].PLogRedoMapZeroed() && g_rgfmp[ ifmp ].PLogRedoMapZeroed()->FPgnoSet( pgnoSplit ) );
        err = JET_errSuccess;
    }
    else
    {
        Call( err );
        Assert( latchRIW == psplitPath->csr.Latch() );
    }

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
    BOOL            fRedoSourcePage     = FLGNeedRedoCheckDbtimeBefore( pmergePath->csr, dbtime, plrmerge->le_dbtimeBefore, &err );
    Call( err );

    BOOL            fRedoLeftPage       = ( pgnoLeft != pgnoNull );
    BOOL            fRedoRightPage      = ( pgnoRight != pgnoNull );

    pmerge->mergetype       = mergetype;

    if ( fRedoLeftPage )
    {
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
        fRedoDestPage = FLGNeedRedoCheckDbtimeBefore( *pcsrDest, dbtime, dbtimeDestBefore, &err );
        Call( err );
    }

    if ( fRedoDestPage )
    {
        Assert( FAssertLGNeedRedo( *pcsrDest, dbtime, dbtimeDestBefore ) );
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

                    Assert( FAssertLGNeedRedo( pmergePath->csr, plrmerge->le_dbtime, plrmerge->le_dbtimeBefore ) );
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
            Assert( FAssertLGNeedRedo( *pcsrDest, dbtime, dbtimeDestBefore ) );

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

            BOOKMARK    bm;
            NDGetBookmarkFromKDF( pfucb, plineinfo->kdf, &bm );

            BOOL fEventloggedLongWait = fFalse;
            TICK tickStart = TickOSTimeCurrent();
            TICK tickEnd;
            while ( fTrue )
            {
                if ( !FNDDeleted( plineinfo->kdf ) || !FVERActive( pfucb, bm ) )
                {
                    break;
                }

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
                Assert( FAssertLGNeedRedo( *pcsrDest, dbtime, dbtimeDestBefore ) );

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
    const OBJID     objidFDP    = plrmerge->le_objidFDP;
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

        err = ErrLGIRedoMergePath( ppib, plrmerge, ppmergePathLeaf );

        AssertSz( errSkipLogRedoOperation != err, "We should not get errSkipLogRedoOperation at this point." );

        Call( err );
    }

    Assert( pfucbNil == *ppfucb );
    Call( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, ppfucb ) );

    Assert( NULL != *ppmergePathLeaf );
    Assert( NULL == (*ppmergePathLeaf)->pmergePathChild );

    Call( ErrLGRIRedoInitializeMerge( ppib, *ppfucb, plrmerge, *ppmergePathLeaf ) );

HandleError:
    return err;
}


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

    Assert( pfucbNil == *ppfucb );
    Call( ErrLGRIGetFucb( m_pctablehash, ppib, ifmp, pgnoFDP, objidFDP, fUnique, fSpace, ppfucb ) );

    for ( psplitPath = *ppsplitPathLeaf;
          psplitPath != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
        Assert( latchRIW == psplitPath->csr.Latch() || pagetrimTrimmed == psplitPath->csr.PagetrimState() );

        err = JET_errSuccess;

        if ( psplitPath->psplit != NULL
            && ( FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err )
                || FLGNeedRedoPage( psplitPath->psplit->csrNew, dbtime ) ) )
        {
            Call( err );
            const BOOL fHasPageBeforeImage = ( CbPageBeforeImage( plrsplit ) > 0 );

#ifdef DEBUG
            ERR         errT;

            SPLIT*      psplit          = psplitPath->psplit;
            Assert( NULL != psplit );

            const BOOL  fRedoSplitPage  = FLGNeedRedoCheckDbtimeBefore(
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

        BOOKMARK    bm;
        NDGetBookmarkFromKDF( pfucb, kdf, &bm );
        Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, bm, operInsert, &prceOper1, &verproxy ) );

        if ( splitoperFlagInsertAndReplaceData == psplit->splitoper
            && fNeedRedo )
        {
            Assert( dirflag & fDIRFlagInsertAndReplaceData );
            verproxy.rceid = rceidOper2;
            BTISplitGetReplacedNode( pfucb, psplit );
            Call( PverFromPpib( pfucb->ppib )->ErrVERModify( pfucb, bm, operReplace, &prceOper2, &verproxy ) );
        }
    }

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


LOCAL ERR ErrLGIRedoMergeUpgradeLatches( MERGEPATH *pmergePathLeaf, DBTIME dbtime )
{
    ERR         err         = JET_errSuccess;
    MERGEPATH*  pmergePath;

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
                if ( pmerge->csrLeft.FLatched() && FLGNeedRedoCheckDbtimeBefore( pmerge->csrLeft, dbtime, pmerge->dbtimeLeftBefore, &err ) )
                {
                    Call( err );
                    pmerge->csrLeft.UpgradeFromRIWLatch();
                }
            }

            Assert( pmerge->pgnoNew == pgnoNull );
            Assert( !pmerge->csrNew.FLatched() );
            CallS( err );
        }

        if ( pagetrimNormal == pmergePath->csr.PagetrimState() )
        {
            Assert( pmergePath->csr.FLatched() );
            Assert( latchRIW == pmergePath->csr.Latch() );
        }

        if ( FLGNeedRedoCheckDbtimeBefore( pmergePath->csr, dbtime, pmergePath->dbtimeBefore, &err ) )
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

                if ( pmerge->csrRight.FLatched() && FLGNeedRedoCheckDbtimeBefore( pmerge->csrRight, dbtime, pmerge->dbtimeRightBefore, &err ) )
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

ERR LOG::ErrLGRIRedoNewPage( const LRNEWPAGE * const plrnewpage )
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

ERR LOG::ErrLGRIIRedoPageMove( __in PIB * const ppib, const LRPAGEMOVE * const plrpagemove )
{
    Assert( ppib );
    Assert( plrpagemove );
    Assert( !plrpagemove->FPageMoveRoot() );
    ASSERT_VALID( plrpagemove );
    
    ERR err;

    
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
        if ( g_rgfmp[ifmp].PLogRedoMapZeroed() &&
             g_rgfmp[ifmp].PLogRedoMapZeroed()->FPgnoSet( plrpagemove->PgnoLeft() ) )
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
        if ( g_rgfmp[ifmp].PLogRedoMapZeroed() &&
             g_rgfmp[ifmp].PLogRedoMapZeroed()->FPgnoSet( plrpagemove->PgnoRight() ) )
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

        Call( pmergePath->pmerge->csrNew.ErrGetNewPreInitPageForRedo(
                ppib,
                ifmp,
                plrpagemove->PgnoDest(),
                plrpagemove->ObjidFDP(),
                plrpagemove->DbtimeAfter() ) );
    }

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
        Assert( latchRIW == pmergePath->csr.Latch() || pagetrimNormal != pmergePath->csr.PagetrimState() );
    }

    if ( fRedoDest )
    {
        Assert( fRedoSource );
        Assert( latchWrite == pmergePath->pmerge->csrNew.Latch() );
    }
    else
    {
        Assert( latchRIW == pmergePath->pmerge->csrNew.Latch() || pagetrimNormal != pmergePath->pmerge->csrNew.PagetrimState() );
    }
    
    LGIRedoMergeUpdateDbtime( pmergePath, plrpagemove->DbtimeAfter() );
    BTPerformPageMove( pmergePath );

HandleError:

    BTIReleaseMergePaths( pmergePath );
    return err;
}

ERR LOG::ErrLGRIRedoPageMove( const LRPAGEMOVE * const plrpagemove )
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

ERR LOG::ErrLGRIRedoPagePatch( const LRPAGEPATCHREQUEST * const plrpagepatchrequest )
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


    Assert( m_fRecoveringMode == fRecoveringRedo );
    if ( dbtime > pfmp->DbtimeCurrentDuringRecovery() )
    {
        pfmp->SetDbtimeCurrentDuringRecovery( dbtime );
    }

    BFLatch bfl;
    TraceContextScope tcScope( iortPagePatching );
    tcScope->nParentObjectClass = tceNone;

    err = ErrBFWriteLatchPage( &bfl, ifmp, pgno, bflfUninitPageOk, BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ), *tcScope );
    if ( err == JET_errPageNotInitialized )
    {
        return JET_errSuccess;
    }
    CallR( err );

    void * pvData;

    BFAlloc( bfasTemporary, &pvData );
    memcpy( pvData, bfl.pv, g_cbPage );

    BFWriteUnlatch( &bfl );

    CPAGE cpage;
    cpage.LoadPage( ifmp, pgno, pvData, g_cbPage );
    cpage.PreparePageForWrite( CPAGE::pgftRockWrite );

    (void)ErrLGOpenPagePatchRequestCallback( ifmp, pgno, dbtime, pvData );
    BFFree( pvData );

    return JET_errSuccess;
}

BOOL LOG::FLGRICheckRedoScanCheck( const LRSCANCHECK2 * const plrscancheck, BOOL fEvaluatePrereadLogic )
{
    Assert( plrscancheck );

    const DBID dbid = plrscancheck->Dbid();

    if ( !FLGRICheckRedoConditionForDb( dbid, m_lgposRedo ) )
    {
        return fFalse;
    }
    else if ( plrscancheck->BSource() != scsDbScan )
    {
        ExpectedSz( plrscancheck->BSource() == scsDbShrink, "ScanCheck not under Shrink. Please, handle." );
        Assert( plrscancheck->Pgno() != pgnoScanLastSentinel );
        return fTrue;
    }


    if ( !BoolParam( m_pinst, JET_paramEnableExternalAutoHealing ) )
    {
        return fFalse;
    }

    if ( UlParam( m_pinst, JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryPassiveScan )
    {
        return fFalse;
    }

    const PGNO pgno = plrscancheck->Pgno();

    if ( fEvaluatePrereadLogic && pgno == pgnoScanLastSentinel )
    {
        return fFalse;
    }

    const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
    FMP* const pfmp = &( g_rgfmp[ifmp] );

    
    bool fDBMFollowerModeSet = ( UlParam( m_pinst, JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryFollowActive );
    if ( !fDBMFollowerModeSet )
    {
        if ( fEvaluatePrereadLogic )
        {
            CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp, pgno );
        }

        if ( pfmp->PdbmFollower() )
        {
            pfmp->PdbmFollower()->DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );
            pfmp->DestroyDBMScanFollower();
        }
        
        return fFalse;
    }
    else if ( pfmp->Pdbfilehdr()->le_pgnoDbscanHighest >= pgno )
    {
        if ( fEvaluatePrereadLogic )
        {
            CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp, pgno );
        }

        if ( pgno != 1 )
        {
            return fFalse;
        }
    }


    return fTrue;
}


ERR LOG::ErrLGRIRedoScanCheck( const LRSCANCHECK2 * const plrscancheck, BOOL* const pfBadPage )
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
    }

    BFLatch bfl = { 0 };
    BOOL fLockedNLoaded = fFalse;
    Assert( fDbScan || fDbShrink );
    TraceContextScope tcScope( fDbScan ? iortDbScan : ( fDbShrink ? iortDbShrink : iortRecovery ) );
    tcScope->nParentObjectClass = tceNone;

    C_ASSERT( pgnoSysMax < pgnoScanLastSentinel );


    if ( plrscancheck->Pgno() < pgnoSysMax )
    {
        Assert( plrscancheck->Pgno() != pgnoScanLastSentinel );

        const BOOL fRequiredRange = g_rgfmp[ifmp].FContainsDataFromFutureLogs();
        const BOOL fTrimmedDatabase = pfmp->Pdbfilehdr()->le_ulTrimCount > 0;
        const BOOL fInitDbtimePageInLogRec = plrscancheck->DbtimePage() != 0 && plrscancheck->DbtimePage() != dbtimeShrunk;
        const BOOL fUninitPossible = fRequiredRange || !fInitDbtimePageInLogRec || fTrimmedDatabase;

        if ( fDbScan )
        {
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
                *tcScope );

        if ( err >= JET_errSuccess )
        {

            CPAGE cpage;
            cpage.ReBufferPage( bfl, ifmp, plrscancheck->Pgno(), bfl.pv, (ULONG)UlParam( PinstFromIfmp( ifmp ), JET_paramDatabasePageSize ) );
            fLockedNLoaded = fTrue;
            Assert( cpage.CbPage() == UlParam( PinstFromIfmp( ifmp ), JET_paramDatabasePageSize ) );
            const DBTIME dbtimePage = cpage.Dbtime();
            const BOOL fInitDbtimePage = dbtimePage != 0 && dbtimePage != dbtimeShrunk;
            Expected( fInitDbtimePage || ( dbtimePage == dbtimeShrunk ) );

            const DBTIME dbtimeCurrentInLogRec = plrscancheck->DbtimeCurrent();
            const DBTIME dbtimeSeed = plrscancheck->DbtimeCurrent();
            const ULONG ulCompressedScanChecksumPassiveDb =
                plrscancheck->lrtyp == lrtypScanCheck ?
                UsDBMGetCompressedLoggedChecksum( cpage, dbtimeSeed ) :
                UlDBMGetCompressedLoggedChecksum( cpage, dbtimeSeed );
            const OBJID objidPage = cpage.ObjidFDP();

            const BOOL fDbtimeRevertedNewPage = 
                ( CmpLgpos( g_rgfmp[ ifmp ].Pdbfilehdr()->le_lgposCommitBeforeRevert, m_lgposRedo ) > 0 &&
                    dbtimePage == dbtimeRevert ) || 
                plrscancheck->DbtimePage() == dbtimeRevert;

            const BOOL fMayCheckDbtimeConsistency = ( fInitDbtimePageInLogRec || fInitDbtimePage ) && !fDbtimeRevertedNewPage;

            const BOOL fDbtimePageTooAdvanced = fMayCheckDbtimeConsistency &&
                                                fInitDbtimePage &&
                                                ( dbtimePage > dbtimeCurrentInLogRec ) &&
                                                !fRequiredRange &&
                                                !fTrimmedDatabase;

            const BOOL fDbtimeInitMismatch = fMayCheckDbtimeConsistency &&
                                             !fDbtimePageTooAdvanced &&
                                             ( !fInitDbtimePageInLogRec != !fInitDbtimePage ) &&
                                             !fRequiredRange &&
                                             !fTrimmedDatabase;

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

                Assert( fLockedNLoaded );
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
                    OSTraceResumeGC();

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
            }

            if ( fInitDbtimePageInLogRec && fInitDbtimePage && ( dbtimePageInLogRec == dbtimePage ) )
            {
                if ( plrscancheck->UlChecksum() == ulCompressedScanChecksumPassiveDb )
                {
                    if ( fDbScan )
                    {
                        CDBMScanFollower::ProcessedDBMScanCheckRecord( pfmp );
                    }
                }
                else
                {
                    Assert( fLockedNLoaded );
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
                                                dbtimePageInLogRec,
                                                dbtimeCurrentInLogRec ),
                                            fFalse );
                        OSTraceResumeGC();

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
            }

            if ( fLockedNLoaded )
            {
                
                cpage.UnloadPage();
                BFRDWUnlatch( &bfl );
                fLockedNLoaded = fFalse;
            }

        }
        else
        {

            BOOL fPageBeyondEof = fFalse;
            switch( err )
            {
                case_AllDatabaseStorageCorruptionErrs:
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
                        CDBMScanFollower::SkippedDBMScanCheckRecord( pfmp );
                    }
                    break;

                default:
                    AssertSz( fFalse, "Q: What else wicked this way comes?  A: %d", err );
                    break;
            }
        }

        Enforce( !fLockedNLoaded );
        
        if ( fDbScan )
        {
            pfmp->PdbmFollower()->CompletePage( plrscancheck->Pgno(), *pfBadPage );

            if ( !fPreviouslyCached )
            {

                BFMarkAsSuperCold( ifmp, plrscancheck->Pgno(), bflfDBScan );
            }

            if ( !fInCache )
            {
                pfmp->PdbmFollower()->UpdateIoTracking( ifmp, 1 );
            }
        }
    }
    else if ( plrscancheck->Pgno() == pgnoScanLastSentinel )
    {
        Assert( fDbScan );

        Expected( plrscancheck->UlChecksum() == 0 && plrscancheck->DbtimePage() == 0 && plrscancheck->DbtimeCurrent() == 0 );
        if ( fDbScan && plrscancheck->UlChecksum() == 0 && plrscancheck->DbtimePage() == 0 && plrscancheck->DbtimeCurrent() == 0 )
        {
            pfmp->PdbmFollower()->DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );
            pfmp->DestroyDBMScanFollower();
        }
    }
    else
    {
        AssertSz( fFalse, "Huh, we got a pgno larger than pgnoSysMax (and that wasn't pgnoScanLastSentinel)??  pgno = %d", plrscancheck->Pgno() );
    }

    Enforce( !fLockedNLoaded );

    if ( ( errCorruption < JET_errSuccess ) && ( err >= JET_errSuccess ) )
    {
        Expected( fFalse );
        err = errCorruption;
    }

    Assert( !( *pfBadPage ) || ( err < JET_errSuccess ) );

    return err;
}


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

    FUCB            *pfucb = pfucbNil;

    Call( ErrLGRIRedoMergeStructures( ppib,
                                      dbtime,
                                      &pmergePathLeaf,
                                      &pfucb ) );
    Assert( pmergePathLeaf != NULL );
    Assert( pmergePathLeaf->pmerge != NULL );

    Assert( pfucb->bmCurr.key.FNull() );
    Assert( pfucb->bmCurr.data.FNull() );

    Call( ErrLGIRedoMergeUpgradeLatches( pmergePathLeaf, dbtime ) );

    LGIRedoMergeUpdateDbtime( pmergePathLeaf, dbtime );

    BTIPerformMerge( pfucb, pmergePathLeaf );

HandleError:
    AssertSz( errSkipLogRedoOperation != err, "%s() got errSkipLogRedoOperation from ErrLGRICheckRedoCondition(dbtime=%d=%#x).\n",
              __FUNCTION__, dbtime, dbtime );

    if ( pmergePathLeaf != NULL )
    {
        BTIReleaseMergePaths( pmergePathLeaf );
    }

    return err;
}

LOCAL ERR ErrLGIRedoSplitUpgradeLatches( SPLITPATH *psplitPathLeaf, DBTIME dbtime )
{
    ERR         err         = JET_errSuccess;
    SPLITPATH*  psplitPath;

    for ( psplitPath = psplitPathLeaf;
          psplitPath->psplitPathParent != NULL;
          psplitPath = psplitPath->psplitPathParent )
    {
    }

    Assert( NULL == psplitPath->psplitPathParent );
    for ( ; psplitPath != NULL; psplitPath = psplitPath->psplitPathChild )
    {
        Assert( latchRIW == psplitPath->csr.Latch() || pagetrimTrimmed == psplitPath->csr.PagetrimState() );

        if ( FLGNeedRedoCheckDbtimeBefore( psplitPath->csr, dbtime, psplitPath->dbtimeBefore, &err ) )
        {
            Call ( err );
            psplitPath->csr.UpgradeFromRIWLatch();
        }
        CallS( err );

        SPLIT   *psplit = psplitPath->psplit;
        if ( psplit != NULL )
        {
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

            if ( psplit->csrRight.FLatched() && FLGNeedRedoCheckDbtimeBefore( psplit->csrRight, dbtime, psplit->dbtimeRightBefore, &err ) )
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


ERR LOG::ErrLGRIRedoSplit( PIB *ppib, DBTIME dbtime )
{
    ERR             err;
    const LRSPLIT_  *plrsplit   = (LRSPLIT_ *) ppib->PvLogrec( dbtime );
    const DBID      dbid        = plrsplit->dbid;

    Assert( lrtypSplit == plrsplit->lrtyp || lrtypSplit2 == plrsplit->lrtyp );
    Assert( ppib->FMacroGoing( dbtime ) );
    Assert( dbtime == plrsplit->le_dbtime );

    const LEVEL     level       = plrsplit->level;

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

    Call( ErrLGIRedoSplitUpgradeLatches( psplitPathLeaf, dbtime ) );

    psplit = psplitPathLeaf->psplit;

    Assert( !fOperNeedsRedo );
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

    LGIRedoSplitUpdateDbtime( psplitPathLeaf, dbtime );

    BTIPerformSplit( pfucb, psplitPathLeaf, &kdf, dirflag );

HandleError:
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

    for ( UINT ibNextLR = 0;
          ibNextLR < ppib->CbSizeLogrec( dbtime );
          ibNextLR += CbLGSizeOfRec( plr ) )
    {
        plr = (LR*)( (BYTE*)ppib->PvLogrec( dbtime ) + ibNextLR );
        switch( plr->lrtyp )
        {
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
            }
            break;

            case lrtypSetExternalHeader:
            {
                const LRSETEXTERNALHEADER* const plrseh = (LRSETEXTERNALHEADER*)plr;
                Assert( plrseh->le_dbtime == dbtime );
                Assert( plrseh->le_dbtimeBefore != dbtimeInvalid );
                Assert( plrseh->le_dbtimeBefore != dbtimeNil );
                Assert( prm->dbtimeAfter == dbtime );
                Assert( plrseh->le_pgno == plrseh->le_pgnoFDP );

                ROOTMOVECHILD* const prmc = new ROOTMOVECHILD;
                Alloc( prmc );
                prm->AddRootMoveChild( prmc );

                prmc->dbtimeBeforeChildFDP = plrseh->le_dbtimeBefore;
                prmc->pgnoChildFDP = plrseh->le_pgno;
                prmc->objidChild = plrseh->le_objidFDP;

                if ( ( err = ErrLGIAccessPage( ppib, &prmc->csrChildFDP, ifmp, prmc->pgnoChildFDP, plrseh->le_objidFDP, fFalse ) ) == errSkipLogRedoOperation )
                {
                    Assert( !prmc->csrChildFDP.FLatched() );
                    err = JET_errSuccess;
                }
                else
                {
                    Call( err );
                    Assert( prmc->csrChildFDP.Latch() == latchRIW );

                    const USHORT cbData = plrseh->CbData();
                    Assert( cbData == sizeof( prmc->sphNew ) );
                    Assert( cbData == prmc->dataSphNew.Cb() );
                    UtilMemCpy( prmc->dataSphNew.Pv(), plrseh->rgbData, cbData );
                }
            }
            break;

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


ERR LOG::ErrLGIRedoRootMoveUpgradeLatches( ROOTMOVE* const prm )
{
    ERR err = JET_errSuccess;
    const DBTIME dbtimeAfter = prm->dbtimeAfter;

    if ( FLGNeedRedoCheckDbtimeBefore( prm->csrFDP, dbtimeAfter, prm->dbtimeBeforeFDP, &err ) )
    {
        Call ( err );
        prm->csrFDP.UpgradeFromRIWLatch();
    }
    CallS( err );

    if ( FLGNeedRedoCheckDbtimeBefore( prm->csrOE, dbtimeAfter, prm->dbtimeBeforeOE, &err ) )
    {
        Call ( err );
        prm->csrOE.UpgradeFromRIWLatch();
    }
    CallS( err );

    if ( FLGNeedRedoCheckDbtimeBefore( prm->csrAE, dbtimeAfter, prm->dbtimeBeforeAE, &err ) )
    {
        Call ( err );
        prm->csrAE.UpgradeFromRIWLatch();
    }
    CallS( err );

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

    for ( ROOTMOVECHILD* prmc = prm->prootMoveChildren;
            prmc != NULL;
            prmc = prmc->prootMoveChildNext )
    {
        if ( FLGNeedRedoCheckDbtimeBefore( prmc->csrChildFDP, dbtimeAfter, prmc->dbtimeBeforeChildFDP, &err ) )
        {
            Call ( err );
            prmc->csrChildFDP.UpgradeFromRIWLatch();
        }
        CallS( err );
    }

    for ( int iCat = 0; iCat < 2; iCat++ )
    {
        if ( FLGNeedRedoCheckDbtimeBefore( prm->csrCatObj[iCat], dbtimeAfter, prm->dbtimeBeforeCatObj[iCat], &err ) )
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
            if ( FLGNeedRedoCheckDbtimeBefore( prm->csrCatClustIdx[iCat], dbtimeAfter, prm->dbtimeBeforeCatClustIdx[iCat], &err ) )
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

    if ( FBTIUpdatablePage( prm->csrFDP ) )
    {
        prm->csrFDP.Dirty();
        prm->csrFDP.SetDbtime( dbtimeAfter );
    }

    if ( FBTIUpdatablePage( prm->csrOE ) )
    {
        prm->csrOE.Dirty();
        prm->csrOE.SetDbtime( dbtimeAfter );
    }

    if ( FBTIUpdatablePage( prm->csrAE ) )
    {
        prm->csrAE.Dirty();
        prm->csrAE.SetDbtime( dbtimeAfter );
    }

    if ( FBTIUpdatablePage( prm->csrNewFDP ) )
    {
        Assert( prm->csrNewFDP.FDirty() );
        prm->csrNewFDP.SetDbtime( dbtimeAfter );
    }

    if ( FBTIUpdatablePage( prm->csrNewOE ) )
    {
        Assert( prm->csrNewOE.FDirty() );
        prm->csrNewOE.SetDbtime( dbtimeAfter );
    }

    if ( FBTIUpdatablePage( prm->csrNewAE ) )
    {
        Assert( prm->csrNewAE.FDirty() );
        prm->csrNewAE.SetDbtime( dbtimeAfter );
    }


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

    Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );

    Call( ErrLGIRedoRootMoveStructures( ppib, dbtime, &rm ) );
    Assert( dbtime == rm.dbtimeAfter );

    Call( ErrLGIRedoRootMoveUpgradeLatches( &rm ) );

    LGIRedoRootMoveUpdateDbtime( &rm );
    OnDebug( rm.AssertValid( fTrue , fTrue  ) );

    Call( ErrLGRIPurgeFcbs( ifmp, rm.pgnoFDP, fFDPTypeUnknown, m_pctablehash ) );
    Call( ErrLGRIPurgeFcbs( ifmp, pgnoFDPMSO, fFDPTypeTable, m_pctablehash ) );
    Call( ErrLGRIPurgeFcbs( ifmp, pgnoFDPMSOShadow, fFDPTypeTable, m_pctablehash ) );

    SHKPerformRootMove(
        &rm,
        ppib,
        ifmp,
        fTrue );
    OnDebug( rm.AssertValid( fFalse , fTrue  ) );

    BOOL fUnused = fFalse;
    Assert( FCB::PfcbFCBGet( ifmp, rm.pgnoFDP, &fUnused ) == pfcbNil );
    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPMSO, &fUnused ) == pfcbNil );
    Assert( FCB::PfcbFCBGet( ifmp, pgnoFDPMSOShadow, &fUnused ) == pfcbNil );

HandleError:
    rm.ReleaseResources();
    return err;
}


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

    if ( g_ifmpMax == ifmp )
    {
        return JET_errSuccess;
    }
    
    FMP::AssertVALIDIFMP( ifmp );
    FMP * const pfmp = &g_rgfmp[ ifmp ];

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

    if ( fMaySkipOlderResize &&
         fLgposLastResizeSet &&
         ( icmpLgposLastVsCurrent > 0 ) )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
            OSFormat( "%hs: Skipping ExtendDB because we're replaying the initial required range and we haven't reached the last resize yet.", __FUNCTION__ ) );
        return JET_errSuccess;
    }

    CallR( ErrIOResizeUpdateDbHdrCount( ifmp, fTrue  ) );

    TraceContextScope tcScope( iorpLogRecRedo );
    CallR( ErrIONewSize( ifmp, *tcScope, pgnoLast, 0, JET_bitResizeDatabaseOnlyGrow ) );
    pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoLast ) );

    if ( fLgposLastResizeSupported )
    {
        CallR( ErrIOResizeUpdateDbHdrLgposLast( ifmp, m_lgposRedo ) );
    }


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
    BOOL fTurnDbmScanBackOn = fFalse;

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

    if ( !BoolParam( m_pinst, JET_paramFlight_EnableShrinkArchiving ) )
    {
        (void)ErrIODeleteShrinkArchiveFiles( ifmp );
    }

    const CPG cpgShrunkLR = plrdbshrink->CpgShrunk();

    PGNO pgnoLastFileSystem = pgnoNull;
    Call( pfmp->ErrPgnoLastFileSystem( &pgnoLastFileSystem ) );

    const PGNO pgnoShrinkFirst = pgnoDbLastNew + 1;
    const PGNO pgnoShrinkLast = pgnoShrinkFirst + cpgShrunkLR - 1;
    const PGNO pgnoShrinkLastOrEof = UlFunctionalMax( pgnoShrinkLast, pgnoLastFileSystem );

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
    const BOOL fInitialReqRange = pfmp->FContainsDataFromFutureLogs();
    const LGPOS lgposLastResize = pfmp->Pdbfilehdr()->le_lgposLastResize;
    const BOOL fLgposLastResizeSet = ( CmpLgpos( lgposLastResize, lgposMin ) != 0 );
    const INT icmpLgposLastVsCurrent = CmpLgpos( lgposLastResize, m_lgposRedo );

    {
    OnDebug( PdbfilehdrReadOnly pdbfilehdr = pfmp->Pdbfilehdr() );
    Assert( !fLgposLastResizeSet || CmpLgpos( pdbfilehdr->le_lgposLastResize, pdbfilehdr->le_lgposAttach ) >= 0 );
    }

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

    const BOOL fMustSkipShrinkRedo = ( !fLgposLastResizeSet && fInitialReqRange );
    const BOOL fMaySkipShrinkRedo = ( fLgposLastResizeSet && ( icmpLgposLastVsCurrent > 0 ) );
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

    Call( ErrLGDbDetachingCallback( m_pinst, pfmp ) );

    if ( fMaySkipShrinkRedo )
    {
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
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "%hs: Shrinking (truncating) to pgno=%lu",
                              __FUNCTION__, pgnoDbLastNew ) );

        Call( ErrLGRIRedoShrinkDBFileTruncation( ifmp, pgnoDbLastNew ) );
    }
    }

HandleError:
    pfmp->ResetPgnoShrinkTarget();
    pfmp->ResetShrinkIsRunning();

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
    TraceContextScope tcScope( iortDbShrink );

    Assert( pfmp->FShrinkIsActive() );
    Assert( ( pgnoShrinkFirstReset != pgnoNull ) && ( pgnoShrinkLastReset != pgnoNull ) );
    Assert( pgnoShrinkFirstReset <= pgnoShrinkLastReset );
    Assert( pfmp->FBeyondPgnoShrinkTarget( pgnoShrinkFirstReset ) );

    for ( PGNO pgno = pgnoShrinkFirstReset; pgno <= pgnoShrinkLastReset; pgno++ )
    {
        if ( ( pgno >= pgnoPrereadWaypoint ) && ( pgnoPrereadNext <= pgnoShrinkLastReset ) )
        {
            const CPG cpgPrereadCurrent = LFunctionalMin( cpgPrereadMax, (CPG)( pgnoShrinkLastReset - pgnoPrereadNext + 1 ) );
            Assert( cpgPrereadCurrent >= 1 );
            BFPrereadPageRange( ifmp,
                                pgnoPrereadNext,
                                cpgPrereadCurrent,
                                bfprfDefault,
                                bfprio,
                                *tcScope );
            pgnoPrereadNext += cpgPrereadCurrent;
            pgnoPrereadWaypoint = pgnoPrereadNext - ( cpgPrereadCurrent / 2 );
        }

        Call( ErrBFWriteLatchPage(
                &bfl,
                ifmp,
                pgno,
                BFLatchFlags( bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfExtensiveChecks ),
                bfprio,
                *tcScope ) );
        fPageLatched = fTrue;
        const ERR errPageStatus = ErrBFLatchStatus( &bfl );
        const BOOL fPageUninit = ( errPageStatus == JET_errPageNotInitialized );
        if ( ( errPageStatus < JET_errSuccess ) && !fPageUninit )
        {
            Call( errPageStatus );
        }

        CPAGE cpageCheck;
        cpageCheck.ReBufferPage( bfl, ifmp, pgno, bfl.pv, (ULONG)g_cbPage );

        if ( !fPageUninit &&
             ( cpageCheck.Dbtime() != dbtimeShrunk ) &&
             ( cpageCheck.Dbtime() > 0 ) &&
             ( cpageCheck.Dbtime() < dbtimeShrink ) )
        {
            BFDirty( &bfl, bfdfDirty, *tcScope );
            CPAGE cpageWrite;
            cpageWrite.GetShrunkPage( ifmp, pgno, bfl.pv, g_cbPage );
            cpageWrite.UnloadPage();

            pgnoWriteMin = UlFunctionalMin( pgnoWriteMin, pgno );
            pgnoWriteMax = UlFunctionalMax( pgnoWriteMax, pgno );

            pfmp->PFlushMap()->SetPgnoFlushType( pgno, CPAGE::pgftUnknown );
        }

        cpageCheck.UnloadPage();
        BFWriteUnlatch( &bfl );
        fPageLatched = fFalse;
    }

    if ( pgnoWriteMin <= pgnoWriteMax )
    {
        Call( ErrBFFlush( ifmp, objidNil, pgnoWriteMin, pgnoWriteMax ) );

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

ERR LOG::ErrLGRIRedoShrinkDBFileTruncation( const IFMP ifmp, const PGNO pgnoDbLastNew )
{
    ERR err = JET_errSuccess;
    FMP* const pfmp = &g_rgfmp[ ifmp ];
    TraceContextScope tcScope( iorpDatabaseShrink );

    Assert( pfmp->FShrinkIsActive() );
    Assert( pgnoDbLastNew != pgnoNull );
    Assert( !pfmp->FBeyondPgnoShrinkTarget( pgnoDbLastNew ) );

    Assert( !pfmp->PdbmFollower() );

    if ( pfmp->FRBSOn() )
    {
        Call( pfmp->PRBS()->ErrFlushAll() );
    }

    Call( ErrIOResizeUpdateDbHdrCount( ifmp, fFalse  ) );

    pfmp->SetOwnedFileSize( CbFileSizeOfPgnoLast( pgnoDbLastNew ) );
    Call( ErrIONewSize( ifmp, *tcScope, pgnoDbLastNew, 0, JET_bitResizeDatabaseOnlyShrink ) );
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

    if ( g_ifmpMax == ifmp )
    {
        return JET_errSuccess;
    }

    FMP::AssertVALIDIFMP( ifmp );
    FMP * const pfmp = &g_rgfmp[ ifmp ];

    if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                    OSFormat( "%hs: Skipping because pfmp->FSkippedAttach()=%d || pfmp->FDeferredAttach()=%d",
                              __FUNCTION__, pfmp->FSkippedAttach(), pfmp->FDeferredAttach() ) );
        return JET_errSuccess;
    }

    if ( pfmp->PLogRedoMapZeroed() )
    {
        pfmp->PLogRedoMapZeroed()->ClearPgno( pgnoStartZeroes, pgnoEndZeroes );
    }
    if ( pfmp->PLogRedoMapBadDbTime() )
    {
        pfmp->PLogRedoMapBadDbTime()->ClearPgno( pgnoStartZeroes, pgnoEndZeroes );
    }

    if ( pfmp->FContainsDataFromFutureLogs() )
    {
        OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
            OSFormat( "%hs: Skipping because we're replaying the initial required range.", __FUNCTION__ ) );
        return JET_errSuccess;
    }

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

    OSTraceFMP( ifmp, JET_tracetagSpaceManagement,
                OSFormat( "%hs: Trimming pgno=%lu, cpg=%lu",
                          __FUNCTION__, pgnoStartZeroes, cpgZeroes ) );

    CallR( ErrSPITrimUpdateDatabaseHeader( ifmp ) );

    const QWORD ibStartZeroesAligned = OffsetOfPgno( pgnoStartZeroes );
    const QWORD cbZeroLength = pfmp->CbOfCpg( cpgZeroes );
    CallR( ErrIOTrim( ifmp, ibStartZeroesAligned, cbZeroLength ) );

    return err;
}

ERR LOG::ErrLGRIRedoExtentFreed( const LREXTENTFREED * const plrextentfreed )
{
    Assert( plrextentfreed );

    ERR err                 = JET_errSuccess;
    const DBID dbid         = plrextentfreed->Dbid();

    const IFMP ifmp         = m_pinst->m_mpdbidifmp[ dbid ];

    if ( g_ifmpMax == ifmp )
    {
        return JET_errSuccess;
    }

    FMP* const pfmp         = g_rgfmp + ifmp;
    if ( pfmp->FSkippedAttach() || pfmp->FDeferredAttach() )
    {
        return JET_errSuccess;
    }

    if ( !( pfmp->FRBSOn() ) )
    {
        return JET_errSuccess;
    }

    const PGNO pgnoFirst    = plrextentfreed->PgnoFirst();
    const CPG cpgExtent     = plrextentfreed->CpgExtent();

    Assert( m_fRecoveringMode == fRecoveringRedo );
    TraceContextScope tcScope( iortRecovery );
    tcScope->nParentObjectClass = tceNone;

    for( int i = 0; i < cpgExtent; ++i )
    {
        BFLatch bfl;

        err = ErrBFWriteLatchPage( &bfl, ifmp, pgnoFirst + i, bflfUninitPageOk, BfpriBFMake( PctFMPCachePriority( ifmp ), (BFTEMPOSFILEQOS)qosIODispatchImmediate ), *tcScope );
        if ( err == JET_errPageNotInitialized )
        {
            BFMarkAsSuperCold( ifmp, pgnoFirst + i );
            continue;
        }
        else if ( err == JET_errFileIOBeyondEOF && pfmp->FContainsDataFromFutureLogs() && CmpLgpos( pfmp->Pdbfilehdr()->le_lgposLastResize, m_lgposRedo ) > 0 )
        {
            BFMarkAsSuperCold( ifmp, pgnoFirst + i );
            continue;
        }
        CallR( err );
        BFMarkAsSuperCold( &bfl );
        BFWriteUnlatch( &bfl );
    }

    return JET_errSuccess;
}


ERR LOG::ErrLGIUpdateGenRecovering(
    const LONG              lGenRecovering,
    __out_ecount_part( dbidMax, *pcifmpsAttached ) IFMP *rgifmpsAttached,
    __out ULONG *           pcifmpsAttached )
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

        const LONG  lGenRecoveringOld   = pfmpT->Pdbfilehdr()->le_lGenRecovering;
        pfmpT->PdbfilehdrUpdateable()->le_lGenRecovering = max( lGenRecovering, lGenRecoveringOld );

        if ( lGenRecovering > pfmpT->Pdbfilehdr()->le_lGenMaxRequired )
        {
            if ( pfmpT->FContainsDataFromFutureLogs() )
            {
                err = ErrLGIVerifyRedoMapForIfmp( ifmp );

                Assert( FMP::FWriterFMPPool() );
                pfmpT->ResetContainsDataFromFutureLogs();

                if ( m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryForwardLogs.lGeneration == 0 )
                {
                    Assert( ( m_pinst->m_isdlInit.FTriggeredStep( eForwardLogBaselineStep ) &&
                                !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoveryVerboseRedoDone ) &&
                                !m_pinst->m_isdlInit.FTriggeredStep( eInitLogRecoveryReopenDone ) ) ||
                                m_pinst->m_isdlInit.FFailedAllocate() ||
                                m_fHardRestore );
                    Assert( m_lgposRedo.lGeneration != 0 );
                    m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryForwardLogs = m_lgposRedo;
                    m_pinst->m_isdlInit.FixedData().sInitData.hrtRecoveryForwardLogs = HrtHRTCount();
                }

                if ( err >= JET_errSuccess )
                {
                    rgifmpsAttached[ *pcifmpsAttached ] = ifmp;
                    (*pcifmpsAttached)++;
                }

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

        if ( JET_dbstateDirtyAndPatchedShutdown == pfmpT->Pdbfilehdr()->Dbstate() )
        {


            if ( ( lGenSwitchingTo >= pfmpT->Pdbfilehdr()->le_lGenMaxRequired ) && pfmpT->FRedoMapsEmpty() )
            {


                err = ErrLGIUpdateCheckpointFile( fTrue, NULL );

                if ( ( err >= JET_errSuccess ) &&
                     ( pfmpT->Patchchk()->lgposAttach.lGeneration <= pfmpT->Pdbfilehdr()->le_lGenMinRequired ) )
                {

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




ERR LOG::ErrLGRIRedoOperations(
    const LE_LGPOS *ple_lgposRedoFrom,
    BYTE *pbAttach,
    BOOL fKeepDbAttached,
    BOOL* const pfRcvCleanlyDetachedDbs,
    LGSTATUSINFO *plgstat )
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

    TraceContextScope tcScope( iortRecovery );
    tcScope->nParentObjectClass = tceNone;

    OSTrace( JET_tracetagInitTerm, OSFormat( "[Recovery] Begin redo operations. [pinst=%p, fKeep=%d]", m_pinst, fKeepDbAttached ) );

    m_lgposRedoShutDownMarkGlobal = lgposMin;
    m_lgposRedoLastTerm = lgposMin;
    m_lgposRedoLastTermChecksum = lgposMin;


    Assert( m_pLogStream->FLGFileOpened() );
    QWORD   cbSize;
    Call( m_pLogStream->ErrFileSize( &cbSize ) );

    Assert( m_pLogStream->CbSec() > 0 );
    Assert( ( cbSize % m_pLogStream->CbSec() ) == 0 );
    UINT    csecSize;
    csecSize = UINT( cbSize / m_pLogStream->CbSec() );
    Assert( csecSize > m_pLogStream->CSecHeader() );


    Call( m_pLogReadBuffer->ErrLReaderInit( csecSize ) );
    Call( m_pLogReadBuffer->ErrReaderEnsureLogFile() );


    Assert( m_plpreread == pNil );
    AllocR( m_plpreread = new LogPrereader( m_pinst ) );
#ifdef DEBUG
    const CPG cpgGrowth = (CPG)( ( cbSize / 1000 ) / sizeof( PGNO ) );
#else
    const CPG cpgGrowth = (CPG)( ( cbSize / 100 ) / sizeof( PGNO ) );
#endif
    Expected( cpgGrowth > 0 );
    m_plpreread->LGPInit( dbidMax, max( cpgGrowth, 1 ) );

    AllocR( m_plprereadSuppress = new LogPrereaderDummy() );
    m_plprereadSuppress->LGPInit( dbidMax, max( cpgGrowth, 1 ) );

    Assert( m_pPrereadWatermarks == pNil );
    AllocR( m_pPrereadWatermarks = new CSimpleQueue<LGPOSQueueNode>() );

    LONG lgenHighAtStartOfRedo;
    Call( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenHighAtStartOfRedo ) );
    if ( m_pLogStream->FCurrentLogExists() )
    {
        lgenHighAtStartOfRedo++;
    }

    LONG lgenHighAttach;
    LoadHighestLgenAttachFromRstmap( &lgenHighAttach );
    lgenHighAtStartOfRedo = max( lgenHighAtStartOfRedo, lgenHighAttach );

    BOOL fDummy;
    err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fDummy, fTrue );
    if ( err == JET_errSuccess || FErrIsLogCorruption( err ) )
    {
        errT = err;
    }
    else
    {
        Assert( err < 0 );
        Call( err );
    }

    err = m_pLogReadBuffer->ErrLGLocateFirstRedoLogRecFF( (LE_LGPOS *)ple_lgposRedoFrom, (BYTE **) &plr );
    if ( err == errLGNoMoreRecords )
    {

        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"1bba029f-b68b-458d-82b8-71fe43b3a0aa" );


        if ( m_fHardRestore )
        {


            if ( m_pLogStream->GetCurrentFileGen() <= m_lGenHighRestore )
            {


                Assert( m_pLogStream->GetCurrentFileGen() >= m_lGenLowRestore );
                Call( ErrERRCheck( JET_errLogCorruptDuringHardRestore ) );
            }
            else
            {


                Call( ErrERRCheck( JET_errLogCorruptDuringHardRecovery ) );
            }
        }
        else
        {


            Call( ErrERRCheck( JET_errLogFileCorrupt ) );
        }
    }
    else if ( err != JET_errSuccess )
    {
        Call( err );
    }
    CallS( err );

#ifdef DEBUG
    {
        LGPOS   lgpos;
        m_pLogReadBuffer->GetLgposOfPbNext( &lgpos );
        Assert( CmpLgpos( &lgpos, &m_pLogReadBuffer->LgposFileEnd() ) <= 0 );
    }
#endif

    if ( plgstat )
    {
        fShowSectorStatus = plgstat->fCountingSectors;
        if ( fShowSectorStatus )
        {
            plgstat->cSectorsSoFar = ple_lgposRedoFrom->le_isec;
            plgstat->cSectorsExpected = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile;
        }
    }


    m_fLastLRIsShutdown = fFalse;

    do
    {
        FMP     *pfmp;
        DBID    dbid;
        IFMP    ifmp;

        if ( errLGNoMoreRecords == err )
        {
            INT fNSNextStep;


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

            m_lgposRedo = m_pLogReadBuffer->LgposFileEnd();
            if ( fLastLRIsQuit )
            {
                m_lgposRedoLastTermChecksum = m_lgposRedo;
            }


            if ( errT != JET_errSuccess )
            {
                goto Done;
            }

            if ( m_lgposRecoveryStop.lGeneration == m_lgposRedo.lGeneration )
            {


                Error( ErrLGISetQuitWithoutUndo( ErrERRCheck( JET_errRecoveredWithoutUndo ) ) );
            }

            err = ErrLGRedoFill( &plr, fLastLRIsQuit, &fNSNextStep );
            if ( FErrIsLogCorruption( err ) )
            {
                errT = err;


                fNSNextStep = fNSGotoCheck;
            }
            else if ( err == errLGNoMoreRecords )
            {

                OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"f25f16bb-40ce-48a6-a31d-5aa98cf2112a" );


                if ( m_fHardRestore )
                {


                    if ( m_pLogStream->GetCurrentFileGen() <= m_lGenHighRestore )
                    {


                        Assert( m_pLogStream->GetCurrentFileGen() >= m_lGenLowRestore );
                        errT = ErrERRCheck( JET_errLogCorruptDuringHardRestore );
                    }
                    else
                    {


                        errT = ErrERRCheck( JET_errLogCorruptDuringHardRecovery );
                    }
                }
                else
                {


                    errT = ErrERRCheck( JET_errLogFileCorrupt );
                }


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

                    if ( FErrIsLogCorruption( errT ) )
                    {
                        const LONG lGeneration = m_pLogStream->GetCurrentFileGen();
                        
                        if ( m_lgposRecoveryStop.lGeneration != lGeneration ||
                            FLGRecoveryLgposStopLogGeneration( ) )
                        {
                            Error( ErrERRCheck( errT ) );
                        }
                    }

                    OnNonRTM( m_lgposRedoPreviousLog = m_lgposRedo );

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
                            
                            plgstat->cSectorsSoFar = 0;
                            plgstat->cSectorsExpected = m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile;
                        }
                    }
                    goto ProcessNextRec;
            }

            
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

#ifdef MINIMAL_FUNCTIONALITY
#else
        if ( m_fPreread && !m_fDumpingLogs )
        {
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

#endif

        Assert( !m_fNeedInitialDbList
            || lrtypChecksum == plr->lrtyp
            || lrtypInit2 == plr->lrtyp );

        if ( m_lgposRedo.lGeneration > lgposLastRedoLogRec.lGeneration )
        {
            IFMP rgifmpsAttached[ dbidMax ];
            ULONG cifmpsAttached = 0;

            if ( m_lgposRedo.lGeneration > m_lgposFlushTip.lGeneration + LLGElasticWaypointLatency() )
            {
                BOOL fFlushed = fFalse;
                Call( m_pLogStream->ErrLGFlushLogFileBuffers( iofrLogMaxRequired, &fFlushed ) );
                if ( fFlushed )
                {
                    LGPOS lgposNextFlushTip;
                    lgposNextFlushTip.lGeneration = m_lgposRedo.lGeneration;
                    lgposNextFlushTip.isec = lgposMax.isec;
                    lgposNextFlushTip.ib   = lgposMax.ib;
                    LGSetFlushTip( lgposNextFlushTip );
                }
            }

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
                        0,
                        0,
                        m_lgposRedo.lGeneration,
                        tmCreate,
                        NULL );

            m_critCheckpoint.Leave();
            Call( err );

            for ( ULONG i=0 ; i<cifmpsAttached; i++ )
            {
                Call( ErrLGDbAttachedCallback( m_pinst, &g_rgfmp[rgifmpsAttached[i]] ) );
            }

            if ( m_lgposRedo.lGeneration > lgenHighAtStartOfRedo )
            {
                Call( ErrLGRICheckForAttachedDatabaseMismatchFailFast( m_pinst ) );
                lgenHighAtStartOfRedo = lMax;
            }
        }


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

        case lrtypChecksum:
            continue;

        case lrtypTrace:                    
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

        case lrtypFullBackup:               
            m_pinst->m_pbackup->BKSetLgposFullBackup( m_lgposRedo );
            m_pinst->m_pbackup->BKSetLogsTruncated( fFalse );
            break;

        case lrtypIncBackup:                
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

        case lrtypShutDownMark:         
            m_lgposRedoShutDownMarkGlobal = m_lgposRedo;
            m_fLastLRIsShutdown = fTrue;
            break;

        default:
        {
            m_fLastLRIsShutdown = fFalse;


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

                if ( lrtypMacroCommit == plr->lrtyp && ppib->FAfterFirstBT() )
                {
                    Call( ErrLGRIRedoMacroOperation( ppib, dbtime ) );
                }

                ppib->ResetMacroGoing( dbtime );

                break;
            }

            case lrtypInit2:
            case lrtypInit:
            {
                
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

                if ( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
                {
                    m_pLogStream->ResetOldLrckVersionSecSize();
                }

                
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

                
#ifdef DEBUG
                m_fDBGNoLog = fTrue;
#endif
                
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
                if ( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
                {
                    m_pLogStream->ResetOldLrckVersionSecSize();
                }

                fLastLRIsQuit = fTrue;
                continue;

            
            
            
            case lrtypCreateDB:
            {
                PIB             *ppib;
                REDOATTACH      redoattach;
                LRCREATEDB      *plrcreatedb    = (LRCREATEDB *)plr;

                dbid = plrcreatedb->dbid;
                Assert( dbid != dbidTemp );

                LGRITraceRedo(plr);

                Call( ErrLGRIPpibFromProcid( plrcreatedb->le_procid, &ppib ) );



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
                            err = JET_errSuccess;
                        }
                        Call( err );
                    }
                }


                INT irstmap = -1;
                {
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
                    Assert( !fDBFileMissing || redoattachDefer == redoattach );
                }

                switch( redoattach )
                {
                    case redoattachCreate:
                    {
                        const JET_SETDBPARAM* const rgsetdbparam = ( irstmap >= 0 ) ? m_rgrstmap[irstmap].rgsetdbparam : NULL;
                        const ULONG csetdbparam = ( irstmap >= 0 ) ? m_rgrstmap[irstmap].csetdbparam : 0;

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
                        Assert( fFalse );
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

                {
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
                if ( CmpLgpos( plrattachdb->lgposConsistent, m_lgposRedo ) >= 0 )
                {
                    pAttachInfo->le_lgposConsistent = lgposMin;
                }
                else
                {
                    pAttachInfo->le_lgposConsistent = plrattachdb->lgposConsistent;
                }
                memcpy( &pAttachInfo->signDb, &plrattachdb->signDb, sizeof(SIGNATURE) );

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
                        Assert( fFalse );
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

                    
                    PIB     *ppib = ppibNil;

                    for ( ppib = m_pinst->m_ppibGlobal; NULL != ppib; ppib = ppib->ppibNext )
                    {
                        while( FPIBUserOpenedDatabase( ppib, dbid ) )
                        {
                            if ( NULL != m_pctablehash )
                            {
                                LGRIPurgeFucbs( ppib, ifmp, m_pctablehash );
                            }
                            Call( ErrDBCloseDatabase( ppib, ifmp, 0 ) );
                        }
                    }


                    Call( ErrLGRIPpibFromProcid( plrdetachdb->le_procid, &ppib ) );
                    Assert( !pfmp->FReadOnlyAttach() );


                    Call( ErrLGICheckDatabaseFileSize( ppib, ifmp ) );


                    if ( pfmp->DbtimeCurrentDuringRecovery() > pfmp->DbtimeLast() )
                    {
                        pfmp->SetDbtimeLast( pfmp->DbtimeCurrentDuringRecovery() );
                    }


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
                        pfmp->FreeLogRedoMaps();
                    }

                    Call( ErrIsamDetachDatabase( (JET_SESID) ppib, NULL, pfmp->WszDatabaseName(), plrdetachdb->Flags() & ~fLRDetachDBUnicodeNames ) );

                    pfmp = NULL;
                }
                else
                {
                    
                    FMP::EnterFMPPoolAsWriter();
                    
                    OSMemoryHeapFree( pfmp->WszDatabaseName() );
                    pfmp->SetWszDatabaseName( NULL );
                    
                    pfmp->ResetFlags();
                    pfmp->Pinst()->m_mpdbidifmp[ pfmp->Dbid() ] = g_ifmpMax;

                    OSMemoryHeapFree( pfmp->Patchchk() );
                    pfmp->SetPatchchk( NULL );
                    
                    FMP::LeaveFMPPoolAsWriter();

                    pfmp = NULL;
                }

                LGRITraceRedo(plr);
            }
                break;

            case lrtypExtRestore:
            case lrtypExtRestore2:
                break;

            case lrtypExtendDB:
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

                if( ifmp >= g_ifmpMax )
                {
                    Assert( ifmp == g_ifmpMax );
                    break;
                }
                FMP::AssertVALIDIFMP( ifmp );

                pfmp = &g_rgfmp[ ifmp ];
                if ( !pfmp->FAttached() )
                {
                    Assert( pfmp->FDeferredAttach() || pfmp->FCreatingDB() || pfmp->FSkippedAttach() );
                    if ( pfmp->FDeferredAttach() || pfmp->FSkippedAttach() )
                    {
                        break;
                    }
                }

                if ( pfmp->Pdbfilehdr() )
                {
                    const DbVersion dbvRedo = plrsetdbversion->Dbv();
                    Call( ErrDBRedoSetDbVersion( m_pinst, ifmp, dbvRedo ) );
                }
                else
                {
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
                const LREXTENTFREED * const plrextentfreed = (LREXTENTFREED *)plr;
                CallR( ErrLGRIRedoExtentFreed( plrextentfreed ) );
        
                break;
            }

            
            
            

            default:
                err = ErrLGRIRedoOperation( plr );

                if ( errSkipLogRedoOperation == err )
                {
                    err = JET_errSuccess;
                }
                
                Call( err );
            } 
        } 
    } 

#ifdef DEBUG
        m_fDBGNoLog = fFalse;
#endif
        fLastLRIsQuit = fFalse;
        lgposLastRedoLogRec = m_lgposRedo;

        
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

    Assert( err < 0 );
    Call( err );

Done:
    err = errT;

HandleError:
    
#ifndef RFS2
    AssertSz( err >= 0,     "Debug Only, Ignore this Assert");
#endif

    if ( err >= JET_errSuccess )
    {
        err = ErrLGIVerifyRedoMaps();

        if ( err >= JET_errSuccess )
        {
            LGITermRedoMaps();

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

    LGPrereadTerm();

    delete m_pPrereadWatermarks;
    m_pPrereadWatermarks = pNil;
    delete m_plpreread;
    m_plpreread = pNil;
    delete m_plprereadSuppress;
    m_plprereadSuppress = pNil;

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

    if ( NULL != pLogRedoMapZeroed )
    {
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
        if ( pLogRedoMapBadDbTime->FAnyPgnoSet() )
        {
            pLogRedoMapBadDbTime->GetOldestLgposEntry( &pgno, &rme, &cpg );
            Assert( rme.err < JET_errSuccess );
            Expected( rme.err == JET_errDbTimeTooOld );
            
            OSTrace( JET_tracetagRecoveryValidation,
                     OSFormat( "IFMP %d: Failed recovery as log redo map of mismatched Dbtimes detected at least one unreconciled entry in pgno: %d.", ifmp, pgno ) );

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

    Assert( JET_dbstateDirtyAndPatchedShutdown != pdbfilehdr->Dbstate() );

    return JET_errSuccess;
}

ERR LOG::ErrLGCheckDBGensRequired( const LONG lGenCurrent )
{
    ERR errWorst = JET_errSuccess;
    ERR errDbid[dbidMax];

    memset( errDbid, 0, sizeof(errDbid) );


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
            Assert( !pfmpT->FSkippedAttach() || m_irstmapMac || !m_fReplayMissingMapEntryDB || pfmpT->FHardRecovery() );
            continue;
        }

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


    Assert( m_fIgnoreLostLogs );

    rgszT[0] = pfmp->WszDatabaseName();
    OSStrCbFormatW( szT1, sizeof(szT1), L"%d", cMissingLogs );
    rgszT[1] = szT1;
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
            Assert( !pfmpT->FSkippedAttach() || m_irstmapMac || !m_fReplayMissingMapEntryDB || pfmpT->FHardRecovery() );
        }
        else
        {
            const ERR errThisDb = ErrLGCheckDatabaseGens( pfmpT->Pdbfilehdr().get(), lGenEffectiveCurrent );
            if ( errWorst == errThisDb )
            {
                LGIPossiblyGenerateLogfileMissingEvent( errWorst, lGenCurrent, lGenEffectiveCurrent, ifmp );
            }
            else if ( errThisDb < JET_errSuccess )
            {
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
        lGenEffectiveCurrent = lGenEffectiveCurrent - 1;
    }

    Assert( !m_fEventedLLRDatabases );

    errWorst = ErrLGCheckDBGensRequired( lGenEffectiveCurrent );

    if ( errWorst == JET_errSuccess )
    {
        return JET_errSuccess;
    }



    LGIPossiblyGenerateLogfileMissingEvents( errWorst, lGenCurrent, lGenEffectiveCurrent );
    

    if ( m_fIgnoreLostLogs && errWorst == JET_errCommittedLogFilesMissing )
    {

        m_fLostLogs = fTrue;
#ifdef DEBUG
        m_fEventedLLRDatabases = fTrue;
#endif
        errWorst = JET_wrnCommittedLogFilesLost;

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

    Assert( errWorst == JET_errRequiredLogFilesMissing ||
            errWorst == JET_errCommittedLogFilesMissing ||
            errWorst == JET_wrnCommittedLogFilesLost );

    return errWorst;
}

ERR LOG::ErrLGMostSignificantRecoveryWarning( void )
{
    Assert( !m_pLogStream->FRemovedLogs() || m_fLostLogs );
    #ifdef DEBUG
    if ( m_pLogStream->FRemovedLogs() )
    {
        Assert( m_fEventedLLRDatabases );
    }
    #endif
    return m_pLogStream->FRemovedLogs() ? ErrERRCheck( JET_wrnCommittedLogFilesRemoved ) :
            m_fLostLogs ? ErrERRCheck( JET_wrnCommittedLogFilesLost ) :
            JET_errSuccess;
}

ERR LOG::ErrLGRRedo( BOOL fKeepDbAttached, CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat )
{
    ERR     err;
    ERR     errBeforeRedo           = JET_errSuccess;
    ERR     errRedo                 = JET_errSuccess;
    ERR     errAfterRedo            = JET_errSuccess;
    LGPOS   lgposRedoFrom;
    INT     fStatus;
    BOOL    fSkipUndo               = fFalse;
    BOOL    fRcvCleanlyDetachedDbs  = fTrue;

    m_fRecovering = fTrue;
    m_fRecoveringMode = fRecoveringRedo;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlRedoBegin|sysosrtlContextInst, m_pinst, (PVOID)&m_pinst->m_iInstance, sizeof(m_pinst->m_iInstance) );

    PERFOpt( cLGRecoveryStallReadOnly.Clear( m_pinst ) );
    PERFOpt( cLGRecoveryLongStallReadOnly.Clear( m_pinst ) );
    PERFOpt( cLGRecoveryStallReadOnlyTime.Clear( m_pinst ) );

    Assert( m_fUseRecoveryLogFileSize == fTrue );

    lgposRedoFrom = pcheckpoint->checkpoint.le_lgposCheckpoint;


    m_pcheckpoint->checkpoint.le_lgposCheckpoint = pcheckpoint->checkpoint.le_lgposCheckpoint;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = pcheckpoint->checkpoint.le_lgposDbConsistency;

    Call( m_pLogStream->ErrLGRIOpenRedoLogFile( &lgposRedoFrom, &fStatus ) );

    errBeforeRedo = err;

    if ( fStatus != fRedoLogFile )
    {
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"4c0cccc4-430a-46cc-bfee-5d484e38686c" );
        Call( ErrERRCheck( JET_errMissingPreviousLogFile ) );
    }

    Assert( m_pLogStream->FLGFileOpened() );

    if ( FLGRecoveryLgposStopLogRecord( lgposRedoFrom ) )
    {
        

        Error( ErrLGISetQuitWithoutUndo( ErrERRCheck( JET_errRecoveredWithoutUndo ) ) );
    }
    
    Assert( m_fRecoveringMode == fRecoveringRedo );

    m_pLogWriteBuffer->SetLgposWriteTip( lgposRedoFrom );

    m_pinst->m_pbackup->BKCopyLastBackupStateFromCheckpoint( &pcheckpoint->checkpoint );

    m_pinst->m_pbackup->BKSetLogsTruncated( fFalse );

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
                    plgstat );
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

#ifdef DEBUG
        switch( errRedo )
        {
            case JET_errDiskFull:
            case JET_errOutOfBuffers:
            case JET_errOutOfMemory:
            case_AllLogStorageCorruptionErrs:
                break;

            default:
                Assert( JET_errSuccess != errRedo );
                break;
        }
#endif

        goto HandleError;
    }


    Assert( m_pLogStream->GetCurrentFileHdr() != NULL );
    Assert( m_pLogStream->CbSec() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );


    if ( fSkipUndo &&
            ( !m_fAfterEndAllSessions || JET_errRecoveredWithoutUndo == errRedo ) )
    {
        if ( errRedo != JET_errRecoveredWithoutUndo )
        {
            Call( ErrLGICheckGenMaxRequiredAfterRedo() );
        }
        if ( err != JET_wrnCommittedLogFilesLost )
        {
            CallS( err );
        }

        if ( errRedo != JET_errRecoveredWithoutUndo )
        {
            Call( ErrLGRICheckForAttachedDatabaseMismatch( m_pinst, !!m_fReplayingIgnoreMissingDB, !!m_fLastLRIsShutdown ) );
        }

        LGITryCleanupAfterRedoError( errRedo );

        if ( m_fTruncateLogsAfterRecovery )
        {
            m_pLogStream->LGTruncateLogsAfterRecovery();
        }

        AssertRTL( FLGILgenHighAtRedoStillHolds() );

        Error( ErrERRCheck( JET_errRecoveredWithoutUndo ) );
    }

    AssertRTL( FLGILgenHighAtRedoStillHolds() );
    AssertRTL( m_fRedidAllLogs || m_fLostLogs );


    Assert( FLGILgenHighAtRedoStillHolds() );

    Assert( !fSkipUndo || m_fAfterEndAllSessions );

    OSTrace( JET_tracetagInitTerm, OSFormat( "[Recovery] Begin undo operations. [pinst=%p]", m_pinst ) );

    Call( ErrLGICheckGenMaxRequiredAfterRedo() );
    Assert( err == JET_errSuccess ||
                ( m_fIgnoreLostLogs && ErrLGMostSignificantRecoveryWarning() > JET_errSuccess ) );

    if ( CmpLgpos( m_lgposFlushTip, m_lgposRedo ) > 0 )
    {
        m_pLogWriteBuffer->LockBuffer();
        LGResetFlushTipWithLock( m_lgposRedo );
        m_pLogWriteBuffer->UnlockBuffer();
    }

    if ( !m_pLogStream->FLGRedoOnCurrentLog() )
    {
        m_pLogStream->LGResetSectorGeometry( m_fUseRecoveryLogFileSize ? m_lLogFileSizeDuringRecovery : 0 );

        Call( m_LogBuffer.ErrLGInitLogBuffers( m_pinst, m_pLogStream ) );
        Call( m_pLogStream->ErrLGRStartNewLog( m_pLogStream->GetCurrentFileGen() + 1 ) );
    }

#ifdef DEBUG
    m_fDBGNoLog = fFalse;
#endif

    Assert( CmpLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint,
                    pcheckpoint->checkpoint.le_lgposCheckpoint )  >= 0 );

    BOOL fDummy;
    Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fDummy, fTrue ) );
    CallS( err );

    errAfterRedo = err;

    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        m_pLogStream->LGResetSectorGeometry( m_fUseRecoveryLogFileSize ? m_lLogFileSizeDuringRecovery : 0 );
    }

    Call( m_LogBuffer.ErrLGInitLogBuffers( m_pinst, m_pLogStream ) );

    m_pLogWriteBuffer->SetFNewRecordAdded( fTrue );

    Call( m_pLogStream->ErrLGOpenFile() );

    m_pinst->m_isdlInit.Trigger( eInitLogRecoveryReopenDone );

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP *pfmp = &g_rgfmp[ ifmp ];

        if ( NULL != pfmp->Pdbfilehdr() &&
             !pfmp->FContainsDataFromFutureLogs() )
        {
            (void)ErrLGDbDetachingCallback( m_pinst, pfmp );
        }
    }



    m_fRecoveringMode = fRecoveringUndo;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlUndoBegin|sysosrtlContextInst, m_pinst, (PVOID)&m_pinst->m_iInstance, sizeof(m_pinst->m_iInstance) );


    const LogVersion * plgvDesired = NULL;
    BOOL fLogVersionNeedsUpdate =
                    ( m_pLogStream->ErrLGGetDesiredLogVersion( (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion ), &plgvDesired ) >= JET_errSuccess ) &&
                    FLGFileVersionUpdateNeeded( *plgvDesired );

    Call( m_pLogStream->ErrLGSwitchToNewLogFile( m_pLogWriteBuffer->LgposWriteTip().lGeneration, 0 ) );


    Call( ErrLGUpdateWaypointIFMP( m_pinst->m_pfsapi ) );

    Assert( FLGILgenHighAtRedoStillHolds() );

    Assert( !m_fAfterEndAllSessions || !m_fLastLRIsShutdown );
    if ( !m_fAfterEndAllSessions )
    {
        Assert( !fSkipUndo );

        Enforce( !m_fNeedInitialDbList );

        if ( !m_fLastLRIsShutdown )
        {
            CallR( ErrLGRecoveryUndo(
                        this,
                        fLogVersionNeedsUpdate || BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ) );
            fLogVersionNeedsUpdate = fFalse;
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

    Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );

    if ( ( 0 != CmpLgpos( &lgposMin, &m_lgposRedoLastTerm ) ) && fRcvCleanlyDetachedDbs )
    {
#ifdef DEBUG
        Assert( 0 == CmpLgpos( &m_lgposRedo, &m_lgposRedoLastTerm ) ||
                0 == CmpLgpos( &m_lgposRedo, &m_lgposRedoLastTermChecksum ) ||
                ( ( ( m_lgposRedoLastTerm.lGeneration + 1 ) == m_lgposRedo.lGeneration ) &&
                  ( m_lgposRedo.isec == m_pLogStream->CSecHeader() ) &&
                  ( m_lgposRedo.ib == 0 ) &&
                        ( 0 == CmpLgpos( &m_lgposRedoPreviousLog, &m_lgposRedoLastTerm ) ||
                          0 == CmpLgpos( &m_lgposRedoPreviousLog, &m_lgposRedoLastTermChecksum ) ) ) );
#endif
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


    Assert( lGenerationMax < lGenerationMaxDuringRecovery );
    if ( m_pLogStream->GetCurrentFileGen() >= lGenerationMax )
    {


        Assert( m_pLogStream->GetCurrentFileGen() <= lGenerationMaxDuringRecovery );


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



    Assert( errBeforeRedo >= JET_errSuccess );


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


    if ( errRedo < JET_errSuccess )
    {
        Assert( err == errRedo );
    }


    if ( !m_fAfterEndAllSessions )
    {
        Assert( err < JET_errSuccess );
        LGITryCleanupAfterRedoError( err );
    }

    Assert( NULL == m_pctablehash );

    m_fRecovering = fFalse;
    m_fRecoveringMode = fRecoveringNone;

    if ( err >= JET_errSuccess )
    {

        Assert( m_pLogStream->GetCurrentFileHdr() != NULL );
        Assert( m_pLogStream->CbSec() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );
        if ( FDefaultParam( m_pinst, JET_paramLogFileSize ) )
        {
            Assert( m_pLogStream->CSecLGFile() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile );
            Assert( m_pLogStream->CbSec() == m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec );
            Assert( m_pLogStream->CSecHeader() == ( sizeof( LGFILEHDR ) + m_pLogStream->CbSec() - 1 ) / m_pLogStream->CbSec() );
        }

        Assert( m_lLogFileSizeDuringRecovery == LONG( ( QWORD( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_csecLGFile ) * m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_cbSec ) / 1024 ) );
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

    Assert( m_pLogStream->LogExt() );
    fMismatchedLogChkExt = !FLGIsDefaultExt( fFalse, m_pLogStream->LogExt() );
    fMismatchedLogFileSize = UlParam( m_pinst, JET_paramLogFileSize ) != (ULONG_PTR)m_lLogFileSizeDuringRecovery;
    fMismatchedLogFileVersion = m_pLogStream->FIsOldLrckVersionSeen();

    if ( fMismatchedLogChkExt )
    {
        fDeleteLogs = !g_fEseutil && BoolParam( m_pinst, JET_paramCircularLog );
    }

    m_fUseRecoveryLogFileSize = fFalse;


    Assert( m_lLogFileSizeDuringRecovery != 0 && m_lLogFileSizeDuringRecovery != ~(ULONG)0 );

    if ( FDefaultParam( m_pinst, JET_paramLogFileSize ) )
    {


        Call( Param( m_pinst, JET_paramLogFileSize )->Set( m_pinst, ppibNil,  m_lLogFileSizeDuringRecovery, NULL ) );

    }
    else if ( fMismatchedLogFileSize )
    {

        if ( !BoolParam( m_pinst, JET_paramCleanupMismatchedLogFiles ) )
        {
            Error( ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent ) );
        }

        fDeleteLogs = fTrue;

    }

    if ( fMismatchedLogFileVersion )
    {
        m_pLogStream->ResetOldLrckVersionSecSize();
        fDeleteLogs = BoolParam( m_pinst, JET_paramCleanupMismatchedLogFiles );
    }

    m_pLogStream->LGResetSectorGeometry( 0 );

    if ( fDeleteLogs )
    {



        Call( m_pLogStream->ErrLGRICleanupMismatchedLogFiles( fMismatchedLogChkExt ) );

        if ( pfJetLogGeneratedDuringSoftStart )
        {
            *pfJetLogGeneratedDuringSoftStart = fTrue;
        }

    }

    m_pLogStream->UpdateCSecLGFile();

    m_pinst->m_trxNewest = roundup( m_pinst->m_trxNewest, TRXID_INCR );

HandleError:

    Assert( m_pLogStream->LogExt() );

    return err;
}

VOID LGFakeCheckpointToLogFile( CHECKPOINT * const pcheckpointT, const LGFILEHDR * const plgfilehdrCurrent, ULONG csecHeader )
{
    pcheckpointT->checkpoint.dbms_param = plgfilehdrCurrent->lgfilehdr.dbms_param;

    Assert( pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration <= plgfilehdrCurrent->lgfilehdr.le_lGeneration );
    pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration = plgfilehdrCurrent->lgfilehdr.le_lGeneration;
    pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec = (WORD) csecHeader;
    pcheckpointT->checkpoint.le_lgposCheckpoint.le_ib = 0;
    pcheckpointT->checkpoint.le_lgposDbConsistency = pcheckpointT->checkpoint.le_lgposCheckpoint;
    UtilMemCpy( pcheckpointT->rgbAttach, plgfilehdrCurrent->rgbAttach, cbAttach );
}

ERR LOG::ErrLGICheckClosedNormallyInPreviousLog(
    __in const LONG         lGenPrevious,
    __out BOOL *            pfCloseNormally
    )
{
    ERR err = JET_errSuccess;

    m_pLogStream->LGMakeLogName( eArchiveLog, lGenPrevious );

    err = m_pLogStream->ErrLGOpenFile();
    if ( err < 0 )
    {
        LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
        goto HandleError;
    }

    Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID ) );

    Assert( m_pLogStream->GetCurrentFileGen() == lGenPrevious );


    Assert( !m_fRecovering );
    Assert( m_fRecoveringMode == fRecoveringNone );
    m_fRecovering = fTrue;
    m_fRecoveringMode = fRecoveringRedo;

    Call( m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( pfCloseNormally ) );

    m_fRecovering = fFalse;
    m_fRecoveringMode = fRecoveringNone;

HandleError:

    m_pLogStream->LGCloseFile();

    return err;
}

ERR LOG::ErrLGSoftStart( BOOL fKeepDbAttached, BOOL fInferCheckpointFromRstmapDbs, BOOL *pfNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart )
{
    ERR                 err;
    ERR                 errErrLGRRedo = JET_wrnNyi;
    BOOL                fCloseNormally = fTrue;
    BOOL                fUseCheckpointToStartRedo = !(*pfNewCheckpointFile);
    BOOL                fSoftRecovery = fFalse;
    WCHAR               wszPathJetChkLog[IFileSystemAPI::cchPathMax];
    LGFILEHDR           *plgfilehdrT = NULL;
    CHECKPOINT          *pcheckpointT = NULL;
    BOOL                fCreatedReserveLogs = fFalse;
    BOOL                fDelLog = fFalse;

    TraceContextScope tcScope( iortRecovery );
    tcScope->nParentObjectClass = tceNone;


    Ptls()->fInSoftStart = fTrue;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlSoftStartBegin|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );


    *pfJetLogGeneratedDuringSoftStart = fFalse;
    m_fAbruptEnd = fFalse;

    Assert( !m_fLostLogs && !m_pLogStream->FRemovedLogs() && JET_errSuccess == ErrLGMostSignificantRecoveryWarning() );

    Assert( m_fUseRecoveryLogFileSize == fFalse );
    m_fUseRecoveryLogFileSize = fTrue;

    m_fNewCheckpointFile = *pfNewCheckpointFile;

    m_wszLogCurrent = SzParam( m_pinst, JET_paramLogFilePath );

    if ( CmpLgpos( &m_lgposRecoveryStop, &lgposMin ) == 0 )
    {
        m_lgposRecoveryStop = lgposMax;
    }
    
    if ( CmpLgpos( &m_lgposRecoveryStop, &lgposMin ) != 0 && m_lgposRecoveryStop.lGeneration == 0 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }
    
    if ( FLGRecoveryLgposStop() &&
        ( BoolParam( m_pinst, JET_paramDeleteOldLogs ) || BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    err = m_pLogStream->ErrLGTryOpenJetLog( JET_OpenLogForRecoveryCheckingAndPatching, lGenSignalCurrentID, (m_pLogStream->LogExt() == NULL) );
    if ( err < 0 )
    {
        if ( JET_errFileNotFound != err )
        {

            goto HandleError;
        }


        LONG lgenHigh = 0;
        if ( m_pLogStream->LogExt() == NULL )
        {
            LONG    lgenTLegacy = 0;
            BOOL    fDefaultExt = fTrue;
            Call ( m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenTLegacy, fTrue, &fDefaultExt ) );

            if ( lgenTLegacy )
            {
                m_wszChkExt = fDefaultExt ? WszLGGetDefaultExt( fTrue ) : WszLGGetOtherExt( fTrue );
                m_pLogStream->LGSetLogExt( FLGIsLegacyExt( fFalse, fDefaultExt ? WszLGGetDefaultExt( fFalse ) : WszLGGetOtherExt( fFalse ) ), fFalse );
                
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
        
        if ( lgenHigh != 0 )
        {
            const ERR errMissingCurrentLog = m_pLogStream->FTempLogExists() ? JET_errSuccess : JET_errMissingLogFile;
            Call( ErrLGOpenLogMissingCallback( m_pinst, m_pLogStream->LogName(), errMissingCurrentLog, lgenHigh+1, fTrue, JET_MissingLogContinueToRedo ) );


            fCloseNormally = fFalse;

            OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );

            goto CheckCheckpoint;
        }

        Call( ErrLGOpenLogMissingCallback( m_pinst, m_pLogStream->LogName(), JET_errSuccess, 0, fFalse, JET_MissingLogCreateNewLogStream ) );

        LGFullNameCheckpoint( wszPathJetChkLog );
        m_pinst->m_pfsapi->ErrFileDelete( wszPathJetChkLog );

        m_fUseRecoveryLogFileSize = fFalse;

        Call( m_pLogStream->ErrLGCreateNewLogStream( pfJetLogGeneratedDuringSoftStart ) );

        Assert( *pfJetLogGeneratedDuringSoftStart );

    }
    else
    {
        LGPOS lgposLastTerm;

        Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, NULL, fCheckLogID ) );

        if ( (*pfNewCheckpointFile) &&
            m_lgenInitial > m_pLogStream->GetCurrentFileGen() )
        {
            Assert( m_lgenInitial == m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration );
            LGISetInitialGen( m_pLogStream->GetCurrentFileGen() );
        }


        Assert( !m_fRecovering );
        Assert( m_fRecoveringMode == fRecoveringNone );
        m_fRecovering = fTrue;
        m_fRecoveringMode = fRecoveringRedo;

        err = m_pLogReadBuffer->ErrLGCheckReadLastLogRecordFF( &fCloseNormally, fTrue, fFalse, NULL, &lgposLastTerm );

        m_fRecovering = fFalse;
        m_fRecoveringMode = fRecoveringNone;


        m_pinst->m_isdlInit.Trigger( eInitLogReadAndPatchFirstLog );


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

        if ( !m_fHardRestore && BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) && !m_fIgnoreLostLogs )
        {
            Assert( !FLGRecoveryLgposStop() );

            Call ( m_pLogStream->ErrLGDeleteOutOfRangeLogFiles() );
        }
        
        if ( fCloseNormally && !(*pfNewCheckpointFile) )
        {
            ERR errReadCheckpoint;

            if ( pcheckpointT == NULL )
            {
                Alloc( pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );
            }

            LGFullNameCheckpoint( wszPathJetChkLog );
            errReadCheckpoint = ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fFalse );

            if ( errReadCheckpoint >= JET_errSuccess )
            {

                if ( 0 != CmpLgpos( &pcheckpointT->checkpoint.le_lgposCheckpoint, &lgposLastTerm ) )
                {
                    Assert( fCloseNormally );
                    fCloseNormally = fFalse;
                }
            }
            else
            {
                (*pfNewCheckpointFile) = fTrue;
                m_fNewCheckpointFile = fTrue;
            }
        }

CheckCheckpoint:
    
        if ( !fCloseNormally || (*pfNewCheckpointFile) )
        {
            if ( plgfilehdrT == NULL )
            {
                Alloc( plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) );
                
                memset( plgfilehdrT, 0, sizeof(LGFILEHDR) );
            }

            if ( pcheckpointT == NULL )
            {
                Alloc( pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );
            }

            LGFullNameCheckpoint( wszPathJetChkLog );

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

                (VOID)m_pinst->m_pfsapi->ErrFileDelete( wszPathJetChkLog );
            }

            if ( fUseCheckpointToStartRedo &&
                 ( JET_errSuccess <= ( err = ErrLGReadCheckpoint( wszPathJetChkLog, pcheckpointT, fFalse ) ) ) )
            {
                if ( (LONG) m_pLogStream->GetCurrentFileGen() == (LONG) pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration )
                {
                    Call( m_pLogStream->ErrLGOpenJetLog( JET_OpenLogForRedo, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration ) );
                }
                else
                {
                    Call( m_pLogStream->ErrLGOpenLogFile( JET_OpenLogForRedo, eArchiveLog, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration, JET_errMissingLogFile ) );
                }

                Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, plgfilehdrT, fCheckLogID ) );

                if ( pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration != plgfilehdrT->lgfilehdr.le_lGeneration ||
                     !( pcheckpointT->checkpoint.fVersion & fCheckpointAttachInfoPresent ) ||
                     !FLGVersionAttachInfoInCheckpoint( &plgfilehdrT->lgfilehdr ) )
                {
                    LGFakeCheckpointToLogFile( pcheckpointT, plgfilehdrT, m_pLogStream->CSecHeader() );
                }
                else
                {
                    pcheckpointT->checkpoint.dbms_param = plgfilehdrT->lgfilehdr.dbms_param;
                }

                m_pLogStream->LGCloseFile();
            }
            else
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

                if ( lgenLow == 0 )
                {
                    if ( m_lgenInitial > m_pLogStream->GetCurrentFileGen() )
                    {
                        m_lgenInitial = m_pLogStream->GetCurrentFileGen();
                    }

                    LGFakeCheckpointToLogFile( pcheckpointT, m_pLogStream->GetCurrentFileHdr(), m_pLogStream->CSecHeader() );

                    err = JET_errSuccess;
                }
                else
                {
                    m_lgenInitial = ( m_lgenInitial > lgenLow ) ? lgenLow : m_lgenInitial;

                    Call( m_pLogStream->ErrLGOpenLogFile( JET_OpenLogForRedo, eArchiveLog, lgenLow, JET_errMissingPreviousLogFile ) );
                    
                    Call( m_pLogStream->ErrLGReadFileHdr( NULL, iorpLogRecRedo, plgfilehdrT, fCheckLogID ) );

                    LGFakeCheckpointToLogFile( pcheckpointT, plgfilehdrT, m_pLogStream->CSecHeader() );

                    m_pLogStream->LGCloseFile();

                    err = JET_errSuccess;
                }

                (void)ErrLGIUpdateCheckpointLgposForTerm( pcheckpointT->checkpoint.le_lgposCheckpoint );
            }

            if ( FLGRecoveryLgposStop() )
            {
                const LGPOS lgposCheckpoint = pcheckpointT->checkpoint.le_lgposCheckpoint;

                if ( lgposCheckpoint.lGeneration > m_lgposRecoveryStop.lGeneration )
                {
                    Error( ErrERRCheck( JET_errRecoveredWithoutUndo ) );
                }
            }
            
            m_pinst->m_pbackup->BKCopyLastBackupStateFromCheckpoint( &pcheckpointT->checkpoint );

            m_lgenInitial = ( m_lgenInitial > pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration )
                ?   pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration
                  : m_lgenInitial;

            m_pinst->m_isdlInit.FixedData().sInitData.lgposRecoveryStartMin = pcheckpointT->checkpoint.le_lgposCheckpoint;

            CallS( err );
            Assert( pcheckpointT );

            Assert( m_wszLogCurrent == SzParam( m_pinst, JET_paramLogFilePath ) );


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

                lGenLow = max( lGenLow, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration);
                lGenHigh = max( lGenHigh, pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration);

                if ( FLGRecoveryLgposStop( ) )
                {
                    lGenLow = min( lGenLow, m_lgposRecoveryStop.lGeneration );
                    lGenHigh = min( lGenHigh, m_lgposRecoveryStop.lGeneration );
                }
                
                LGIRSTPrepareCallback( plgstat, lGenHigh, lGenLow, m_lgposRecoveryStop.lGeneration );
            }
            else
            {
                Assert( NULL == plgstat );
                memset( &lgstat, 0, sizeof(lgstat) );
            }

            err = errErrLGRRedo = ErrLGRRedo( fKeepDbAttached, pcheckpointT, plgstat );

            Assert( err != JET_errRecoveredWithoutUndoDatabasesConsistent );

            if ( plgstat )
            {
                plgstat->snprog.cunitDone = plgstat->snprog.cunitTotal;
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
            CallS( err );


            fDelLog = fTrue;

            if ( g_fRepair && m_errGlobalRedoError != JET_errSuccess )
            {
                Call( ErrERRCheck( JET_errRecoveredWithErrors ) );
            }

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
        else
        {
            errErrLGRRedo = JET_errSuccess;
        }


        Call( ErrLGOpenLogCallback( m_pinst, m_pLogStream->LogName(), JET_OpenLogForDo, 0, fTrue ) );

        Call( ErrLGMoveToRunningState( errErrLGRRedo, pfJetLogGeneratedDuringSoftStart ) );
        
        Assert( m_pLogStream->FLGFileOpened() || !*pfJetLogGeneratedDuringSoftStart );
        if ( fDelLog || *pfJetLogGeneratedDuringSoftStart )
        {
            m_pLogStream->LGCloseFile();
        }
        
    }


    AssertRTL( m_errBeginUndo == JET_errSuccess );

    Assert( m_fUseRecoveryLogFileSize == fFalse );

    if ( !fCreatedReserveLogs || *pfJetLogGeneratedDuringSoftStart )
    {
        Assert( m_wszLogCurrent == SzParam( m_pinst, JET_paramLogFilePath ) );
        Call( m_pLogStream->ErrLGInitCreateReserveLogFiles() );
        fCreatedReserveLogs = fTrue;
    }

    m_fAbruptEnd = fFalse;


    m_pLogWriteBuffer->SetFNewRecordAdded( fFalse );


    BOOL fHadToRetry = fFalse;
RetryOpenLogForDo:

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
        Assert( fFalse );
        err = ErrERRCheck( JET_errLogSectorSizeMismatchDatabasesConsistent );
    }
    Call( err );


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

    if ( FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        m_pLogStream->LGResetSectorGeometry( m_fUseRecoveryLogFileSize ? m_lLogFileSizeDuringRecovery : 0 );
    }


    Call( m_pLogStream->ErrLGOpenFile() );

    Call( m_LogBuffer.ErrLGInitLogBuffers( m_pinst, m_pLogStream ) );

    err = m_pLogStream->ErrLGSwitchToNewLogFile( m_pLogWriteBuffer->LgposWriteTip().lGeneration, 0 );
    if ( err == JET_errLogFileSizeMismatch )
    {
        err = ErrERRCheck( JET_errLogFileSizeMismatchDatabasesConsistent );
    }
    else if ( err == JET_errLogSectorSizeMismatch )
    {
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
        Call( ErrERRCheck( JET_errLogWriteFail ) );
    }

    m_pLogStream->ResetOldLrckVersionSecSize();

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
    Assert( m_fHardRestore == fFalse );

    m_pLogWriteBuffer->CheckIsReady();

    m_pLogWriteBuffer->SetFNewRecordAdded( fTrue );

HandleError:

    if ( err < 0 )
    {
        ULONG rgul[4] = { (ULONG) m_pinst->m_iInstance, (ULONG) err, PefLastThrow()->UlLine(), UlLineLastCall() };
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlSoftStartFail|sysosrtlContextInst, m_pinst, rgul, sizeof(rgul) );

        Assert( m_fHardRestore == fFalse );

        
        m_pLogStream->LGCloseFile();

        if ( fSoftRecovery )
        {
            if ( err != JET_errRecoveredWithoutUndo &&
                    err != JET_errRecoveredWithoutUndoDatabasesConsistent )
            {
                UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, RESTORE_DATABASE_FAIL_ID, err, m_pinst );

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

