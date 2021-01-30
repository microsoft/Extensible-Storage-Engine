// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

BOOL g_fSystemInit = fFalse;

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotal<> cUserWaitAllTrxCommit;
LONG LUserWaitAllTrxCommitCEFLPv(LONG iInstance,void *pvBuf)
{
    cUserWaitAllTrxCommit.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cUserWaitLastTrxCommit;
LONG LUserWaitLastTrxCommitCEFLPv(LONG iInstance,void *pvBuf)
{
    cUserWaitLastTrxCommit.PassTo( iInstance, pvBuf );
    return 0;
}

#endif


LONG CbSYSMaxPageSize()
{
    return (LONG)UlParam( JET_paramDatabasePageSize );
}


LONG CbINSTMaxPageSize( const INST * const pinst )
{
    return (LONG)UlParam( pinst, JET_paramDatabasePageSize );
}



ERR ISAMAPI ErrIsamBeginSession( JET_INSTANCE inst, JET_SESID *psesid )
{
    ERR     err;
    PIB     **pppib;
    INST    *pinst = (INST *)inst;

    Assert( psesid != NULL );
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
    pppib = (PIB **)psesid;

    Call( ErrPIBBeginSession( pinst, pppib, procidNil, fFalse ) );
    (*pppib)->grbitCommitDefault = pinst->m_grbitCommitDefault;    
    (*pppib)->SetFUserSession();

HandleError:
    return err;
}


ERR ISAMAPI ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit )
{
    ERR     err;
    PIB     *ppib   = (PIB *)sesid;

    CallR( ErrPIBCheck( ppib ) );

    err = ppib->ErrPIBSetSessionContext( dwPIBSessionContextUnusable );
    if ( err < 0 )
    {
        if ( JET_errSessionContextAlreadySet == err )
            err = ErrERRCheck( JET_errSessionInUse );
        return err;
    }

    INST    *pinst  = PinstFromPpib( ppib );

    if ( ppib->Level() > 0 )
    {
        ppib->PrepareTrxContextForEnd();

        Assert( sizeof(JET_SESID) == sizeof(ppib) );
        Call( ErrIsamRollback( (JET_SESID)ppib, JET_bitRollbackAll ) );
        Assert( 0 == ppib->Level() );
    }

    Call( ErrDBCloseAllDBs( ppib ) );

    while( ppib->pfucbOfSession != pfucbNil )
    {
        FUCB    *pfucb  = ppib->pfucbOfSession;

        if ( FFUCBSort( pfucb ) )
        {
            Assert( !( FFUCBIndex( pfucb ) ) );
            Call( ErrIsamSortClose( ppib, pfucb ) );
        }
        else if ( pinst->FRecovering() )
        {
            DIRClose( pfucb );
        }
        else
        {
            Assert( FFMPIsTempDB( pfucb->ifmp ) );

            while ( FFUCBSecondary( pfucb ) || FFUCBLongValue( pfucb ) )
            {
                pfucb = pfucb->pfucbNextOfSession;
                Assert( FFMPIsTempDB( pfucb->ifmp ) );

                AssertRTL( pfucbNil != pfucb );
            }

            Assert( FFUCBIndex( pfucb ) );

            if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
            {
                CallS( ErrDispCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
            }
            else
            {
                CallS( ErrFILECloseTable( ppib, pfucb ) );
            }
        }
    }
    Assert( pfucbNil == ppib->pfucbOfSession );

    PIBEndSession( ppib );

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );

    ppib->PIBResetSessionContext( fFalse );

    return err;
}

ERR ErrIsamGetSessionInfo(
    JET_SESID                           sesid,
    _Out_bytecap_(cbMax) VOID *         pvResult,
    const ULONG                         cbMax,
    const ULONG                         ulInfoLevel )
{
    ERR                 err         = JET_errSuccess;
    PIB *               ppib        = (PIB *)sesid;
    JET_SESSIONINFO *   psessinfo   = (JET_SESSIONINFO *)pvResult;
    
    CallR( ErrPIBCheck( ppib ) );
    Assert( ppibNil != ppib );

    if ( JET_SessionInfo == ulInfoLevel )
    {
        if ( cbMax < sizeof(JET_SESSIONINFO) )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        ppib->PIBGetSessionInfoV1( psessinfo );
        err = JET_errSuccess;
    }
    else
    {
        err = ErrERRCheck( JET_errInvalidParameter );
    }

    return err;
}

ERR ISAMAPI ErrIsamIdle( JET_SESID sesid, JET_GRBIT grbit )
{
    ERR     err = JET_errSuccess;
    ERR     wrnFirst = JET_errSuccess;

#ifndef RTM
    if ( grbit & JET_bitIdleVersionStoreTest )
    {
        VER *pver = PverFromPpib( (PIB *) sesid );
        Call( pver->ErrInternalCheck() );
    }
#endif

    if ( grbit & JET_bitIdleStatus )
    {
        VER *pver = PverFromPpib( (PIB *) sesid );
        Call( pver->ErrVERStatus() );
    }
    else if ( 0 == grbit || JET_bitIdleCompact & grbit  )
    {
        if ( grbit & JET_bitIdleCompactAsync )
        {
            VERSignalCleanup( (PIB *)sesid );
        }
        else
        {
            (void) PverFromPpib( (PIB *)sesid )->ErrVERRCEClean();
        }
        err = ErrERRCheck( JET_wrnNoIdleActivity );
    }
    else if ( grbit & JET_bitIdleCompactAsync )
    {
        err = ErrERRCheck( JET_errInvalidGrbit );
    }
    else if ( grbit & JET_bitIdleWaitForAsyncActivity )
    {
        PIB* const ppib = (PIB*)sesid;

        {
        VER* const pver = PverFromPpib( ppib );
        DWORD_PTR cVerBuckets = 0;
        ENTERCRITICALSECTION enterCritRCEBucketGlobal( &pver->m_critBucketGlobal );
        CallS( pver->m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cVerBuckets ) );
        err = ( cVerBuckets != 0 ) ? ErrERRCheck( JET_wrnRemainingVersions ) : JET_errSuccess;
        }

        INST* const pinst = PinstFromPpib( ppib );
        FMP::EnterFMPPoolAsReader();
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
            if ( ifmp >= g_ifmpMax  )
            {
                continue;
            }

            FMP::LeaveFMPPoolAsReader();

            if ( FMP::ErrWriteLatchByIfmp( ifmp, ppib ) < JET_errSuccess )
            {
                FMP::EnterFMPPoolAsReader();
                continue;
            }

            FMP* const pfmp = &g_rgfmp[ ifmp ];

            if ( pfmp->FReadOnlyAttach() || pfmp->FIsTempDB() )
            {
                pfmp->ReleaseWriteLatch( ppib );
                FMP::EnterFMPPoolAsReader();
                continue;
            }

            pfmp->IncCPin();
            pfmp->ReleaseWriteLatch( ppib );

            pfmp->WaitForTasksToComplete();

            pfmp->DecCPin();
            FMP::EnterFMPPoolAsReader();
        }
        FMP::LeaveFMPPoolAsReader();
    }

    Call( err );
    wrnFirst = err;

    if ( grbit & JET_bitIdleFlushBuffers )
    {
        Call( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    if ( grbit & JET_bitIdleAvailBuffersStatus )
    {
        Call( ErrBFCheckMaintAvailPoolStatus() );
        if ( wrnFirst == JET_errSuccess )
        {
            wrnFirst = err;
        }
    }

HandleError:
    return err < JET_errSuccess ? err : wrnFirst;
}


ERR VTAPI ErrIsamCapability( JET_SESID vsesid,
    JET_DBID vdbid,
    ULONG ulArea,
    ULONG ulFunction,
    ULONG *pgrbitFeature )
{
    return ErrERRCheck( JET_errFeatureNotAvailable );
}


BOOL g_fRepair              = fFalse;
BOOL g_fEseutil             = fFalse;

BOOL g_fPeriodicTrimEnabled = fFalse;

DWORD g_dwGlobalMajorVersion;
DWORD g_dwGlobalMinorVersion;
DWORD g_dwGlobalBuildNumber;
LONG g_lGlobalSPNumber;


ERR ISAMAPI ErrIsamSystemInit()
{
    ERR err;

    Call( ErrNORMCheckLocaleName( pinstNil, wszLocaleNameDefault ) );
    AssertNORMConstants();
    QWORD qwSortVersion = 0;
    err = ErrNORMGetSortVersion( wszLocaleNameDefault, &qwSortVersion, NULL );
    CallS( err );
    Call( err );
    Assert( qwSortVersion != 0 );
    Assert( qwSortVersion != qwMax );
    Assert( qwSortVersion != g_qwUpgradedLocalesTable );


    g_dwGlobalMajorVersion = DwUtilSystemVersionMajor();
    g_dwGlobalMinorVersion = DwUtilSystemVersionMinor();
    g_dwGlobalBuildNumber = DwUtilSystemBuildNumber();
    g_lGlobalSPNumber = DwUtilSystemServicePackNumber();


    Call( ErrITSetConstants( ) );


    Call( INST::ErrINSTSystemInit() );

    CallJ( LGInitTerm::ErrLGSystemInit(), TermInstInit );


    CallJ( ErrIOInit(), TermLOGTASK )

    CallJ( FMP::ErrFMPInit(), TermIO );

    CallJ( ErrCALLBACKInit(), TermFMP );

    CallJ( CPAGE::ErrInit(), TermCALLBACK );

    CallJ( ErrBFInit( CbSYSMaxPageSize() ), TermCPAGE );

    CallJ( ErrCATInit(), TermBF );

    CallJ( ErrSNAPInit(), TermCAT );

    CallJ( ErrOLDInit(), TermSNAP );

    CallJ( ErrDBMInit(), TermOLD );

    CallJ( ErrSPInit(), TermDBM );

    CallJ( ErrPKInitCompression( g_cbPage, (INT)UlParam( JET_paramMinDataForXpress ), g_cbPage ), TermSP );

    g_fSystemInit = fTrue;

    return JET_errSuccess;

TermSP:
    SPTerm();

TermDBM:
    DBMTerm();

TermOLD:
    OLDTerm();

TermSNAP:
    SNAPTerm();
    
TermCAT:
    CATTerm();
    
TermBF:
    BFTerm();

TermCPAGE:
    CPAGE::Term();

TermCALLBACK:
    CALLBACKTerm();

TermFMP:
    FMP::Term();

TermIO:
    IOTerm();

TermLOGTASK:
    LGInitTerm::LGSystemTerm();

TermInstInit:
    INST::INSTSystemTerm();

HandleError:
    return err;
}


VOID ISAMAPI IsamSystemTerm()
{
    if ( !g_fSystemInit )
    {
        return;
    }

    g_fSystemInit = fFalse;

    SPTerm();

    DBMTerm();

    OLDTerm();
    
    SNAPTerm();
    
    CATTerm();

    BFTerm( );

    CPAGE::Term();

    CALLBACKTerm();

    FMP::Term();

    IOTerm();

    LGInitTerm::LGSystemTerm();

    PKTermCompression();

    INST::INSTSystemTerm();
}


BOOL FEnsureLogStreamMustExist( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, __out JET_ERR * const perr )
{
    if ( snt == JET_sntMissingLog &&
            pRecCtrl->MissingLog.eNextAction == JET_MissingLogCreateNewLogStream )
    {
        AssertRTL( pRecCtrl->MissingLog.lGenMissing == 0 );

        Assert( pRecCtrl->errDefault == JET_errSuccess );
        *perr = ErrERRCheckDefault( JET_errFileNotFound );
        return fTrue;
    }

    return fFalse;
}

BOOL FAllowMissingCurrentLog( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, __out JET_ERR * const perr )
{
    if ( snt == JET_sntMissingLog )
    {
        if ( ( pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToRedo ||
                    pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToUndo ) &&
                pRecCtrl->MissingLog.fCurrentLog )
        {
            AssertRTL( pRecCtrl->errDefault == JET_errMissingLogFile ||
                        pRecCtrl->errDefault == JET_errFileNotFound ||
                        pRecCtrl->errDefault == JET_errSuccess );
            *perr = JET_errSuccess;
            return fTrue;
        }
        else
        {
            AssertRTL( !pRecCtrl->MissingLog.fCurrentLog );
        }
    }

    return fFalse;
}

BOOL FReplayIgnoreLostLogs( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, __out JET_ERR * const perr )
{
    if ( snt == JET_sntMissingLog )
    {
        if ( ( pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToRedo ||
                    pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToUndo ) &&
                pRecCtrl->MissingLog.fCurrentLog )
        {
            AssertRTL( pRecCtrl->errDefault == JET_errMissingLogFile ||
                        pRecCtrl->errDefault == JET_errFileNotFound ||
                        pRecCtrl->errDefault == JET_errSuccess );
            *perr = JET_errSuccess;
            return fTrue;
        }
        else
        {
            AssertRTL( !pRecCtrl->MissingLog.fCurrentLog );
        }
    }
    if ( snt == JET_sntOpenCheckpoint )
    {
        AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
    }

    return fFalse;
}

BOOL FAllowSoftRecoveryOnBackup( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, __out JET_ERR * const perr )
{
    if ( snt == JET_sntSignalErrorCondition &&
            pRecCtrl->errDefault == JET_errSoftRecoveryOnBackupDatabase )
    {
        *perr = JET_errSuccess;
        return fTrue;
    }

    return fFalse;
}

BOOL FSkipLostLogsEvent( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, __out JET_ERR * const perr )
{
    if ( snt == JET_sntSignalErrorCondition &&
            pRecCtrl->errDefault == JET_errCommittedLogFilesMissing )
    {
        *perr = JET_errSuccess;
        return fTrue;
    }

    return fFalse;
}


struct TM_REC_CTRL_DEFAULT
{
    INST *                  pinst;
    JET_GRBIT               grbit;
    JET_PFNINITCALLBACK     pfnUserControl;
    void *                  pvUserContext;

    BOOL                    fSawBeginUndo;
    BOOL                    fSawBeginDo;
    BOOL    FRecoveryWithoutUndo()
    {
        Assert( fSawBeginUndo || fSawBeginDo );
        return grbit & JET_bitRecoveryWithoutUndo;
    }
};

JET_ERR ErrRecoveryControlDefaultCallback(
    __in JET_SNP    snp,
    __in JET_SNT    snt,
    __in_opt void * pvData,
    __in_opt void * pvContext )
{
    JET_ERR errDefaultAction = JET_wrnNyi;

    struct TM_REC_CTRL_DEFAULT * pRecCtrlDefaults = reinterpret_cast<struct TM_REC_CTRL_DEFAULT *>( pvContext );

    OSTrace( JET_tracetagCallbacks, OSFormat( "Executing ErrRecoveryControlDefaultCallback() before user callback\n" ) );

    if( snp == JET_snpRecoveryControl )
    {
        JET_RECOVERYCONTROL * pRecCtrl = reinterpret_cast<JET_RECOVERYCONTROL *>( pvData );


        switch( snt )
        {
            case JET_sntOpenLog:
                if ( pRecCtrl->OpenLog.eReason == JET_OpenLogForDo )
                {
                    pRecCtrlDefaults->fSawBeginDo = fTrue;
                    if ( pRecCtrlDefaults->FRecoveryWithoutUndo() )
                    {
                        AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
                        errDefaultAction = ErrERRCheckDefault( JET_errRecoveredWithoutUndoDatabasesConsistent );
                    }
                }
                break;
            case JET_sntOpenCheckpoint:
                break;
            case JET_sntMissingLog:
                if ( pRecCtrl->MissingLog.lGenMissing == 0 )
                {
                    AssertRTL( pRecCtrl->MissingLog.eNextAction == JET_MissingLogCreateNewLogStream );
                }
                break;
            case JET_sntBeginUndo:
                AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
                
                pRecCtrlDefaults->fSawBeginUndo = fTrue;
                Assert( pRecCtrlDefaults->fSawBeginUndo );
                if ( pRecCtrlDefaults->FRecoveryWithoutUndo() )
                {
                    AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
                    errDefaultAction = ErrERRCheckDefault( JET_errRecoveredWithoutUndo );
                }
                break;

            case JET_sntSignalErrorCondition:
                Expected( pRecCtrl->errDefault == JET_errSoftRecoveryOnBackupDatabase ||
                            pRecCtrl->errDefault == JET_errCommittedLogFilesMissing );
                break;

            case JET_sntNotificationEvent:
            case JET_sntAttachedDb:
            case JET_sntDetachingDb:
            case JET_sntCommitCtx:
                Expected( pRecCtrl->errDefault == JET_errSuccess );
                break;

            default:
                AssertSzRTL( fFalse, "Unknown snt = %d for JET_snpRecoveryControl", snt );
                return ErrERRCheck( JET_wrnNyi );
        }


        JET_ERR errT = JET_wrnNyi;
        
        if ( pRecCtrlDefaults->grbit & JET_bitLogStreamMustExist && FEnsureLogStreamMustExist( snt, pRecCtrl, &errT ) )
        {
            AssertSz( errDefaultAction == JET_wrnNyi || errDefaultAction == errT, "Competing opinions about which error action to take!" );
            errDefaultAction = errT;
        }

        if ( pRecCtrlDefaults->grbit & JET_bitAllowMissingCurrentLog && FAllowMissingCurrentLog( snt, pRecCtrl, &errT ) )
        {
            AssertSz( errDefaultAction == JET_wrnNyi || errDefaultAction == errT, "Competing opinions about which error action to take!" );
            errDefaultAction = errT;
        }

        if ( pRecCtrlDefaults->grbit & JET_bitReplayIgnoreLostLogs && FReplayIgnoreLostLogs( snt, pRecCtrl, &errT ) )
        {
            AssertSz( errDefaultAction == JET_wrnNyi || errDefaultAction == errT, "Competing opinions about which error action to take!" );
            errDefaultAction = errT;
        }

        if ( pRecCtrlDefaults->grbit & JET_bitAllowSoftRecoveryOnBackup && FAllowSoftRecoveryOnBackup( snt, pRecCtrl, &errT ) )
        {
            AssertSz( errDefaultAction == JET_wrnNyi || errDefaultAction == errT, "Competing opinions about which error action to take!" );
            errDefaultAction = errT;
        }

        if ( pRecCtrlDefaults->grbit & JET_bitSkipLostLogsEvent && FSkipLostLogsEvent( snt, pRecCtrl, &errT ) )
        {
            AssertSz( errDefaultAction == JET_wrnNyi || errDefaultAction == errT, "Competing opinions about which error action to take!" );
            errDefaultAction = errT;
        }


        if ( errDefaultAction != JET_wrnNyi )
        {
            OSTrace( JET_tracetagCallbacks, OSFormat( "ErrRecoveryControlDefaultCallback() transforming errDefault from %d to %d\n", pRecCtrl->errDefault, errDefaultAction ) );
            pRecCtrl->errDefault = errDefaultAction;
        }
    }


    JET_ERR errRet = JET_wrnNyi;

    if ( pRecCtrlDefaults->pfnUserControl &&
            ( pRecCtrlDefaults->grbit & JET_bitExternalRecoveryControl ||
                snp != JET_snpRecoveryControl ) )
    {

        OSTrace( JET_tracetagCallbacks, OSFormat( "ErrRecoveryControlDefaultCallback() issuing user callback\n" ) );

        errRet = pRecCtrlDefaults->pfnUserControl( snp, snt, pvData, pRecCtrlDefaults->pvUserContext );

        OSTrace( JET_tracetagCallbacks, OSFormat( "ErrRecoveryControlDefaultCallback() returned --> %d\n", errRet ) );
    }
    else
    {

        if ( snp == JET_snpRecoveryControl )
        {
            errRet = reinterpret_cast<JET_RECOVERYCONTROL *>( pvData )->errDefault;
        }
        else
        {
            errRet = JET_errSuccess;
        }
    }


    if ( errDefaultAction != JET_wrnNyi &&
            errDefaultAction != JET_errSuccess &&
            errDefaultAction == errRet )
    {
        ErrERRCheck( errRet );
    }

    return errRet;
}

ERR ISAMAPI ErrIsamInit(    JET_INSTANCE    inst,
                            JET_GRBIT       grbit )
{
    ERR         err;
    INST        *pinst                          = (INST *)inst;
    LOG         *plog                           = pinst->m_plog;
    BOOL        fLGInitIsDone = fFalse;
    BOOL        fTmpLogBuffersInitialized       = fFalse;
    BOOL        fJetLogGeneratedDuringSoftStart = fFalse;
    BOOL        fNewCheckpointFile              = fTrue;
    const ULONG csz                             = 5;
    WCHAR       rgwszDw[csz][16];
    const WCHAR *rgcwszT[csz];
    struct TM_REC_CTRL_DEFAULT  RecCtrlDefaults = { 0 };

    Assert( pinst->m_plog );
    Assert( pinst->m_pver );
    Assert( pinst->m_pfsapi );
    Assert( !pinst->FRecovering() );
    Assert( !FNegTest( fLeakingUnflushedIos ) );


    if ( BoolParam( pinst, JET_paramCreatePathIfNotExist ) )
    {


        Assert( !FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramTempPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramSystemPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramLogFilePath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramRBSFilePath ) ) );


        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramTempPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramSystemPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramLogFilePath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramRBSFilePath ), NULL, 0 ) );
    }


    CallR( plog->ErrLGInitSetInstanceWiseParameters( grbit ) );


    if ( 0 != ( __int64( ( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 ) % plog->CSecLGFile() ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    if ( plog->CbLGSec() != (ULONG) ( __int64( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 / plog->CSecLGFile() ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    if ( BoolParam( pinst, JET_paramCleanupMismatchedLogFiles ) && !BoolParam( pinst, JET_paramCircularLog ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    if ( BoolParam( pinst, JET_paramCleanupMismatchedLogFiles ) && ( grbit & JET_bitKeepDbAttachedAtEndOfRecovery ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }


    Assert( pinst->m_updateid == updateidMin );

    if ( BoolParam( pinst, JET_paramLogFileCreateAsynch ) &&
            cbLogExtendPattern >= UlParam( JET_paramMaxCoalesceWriteSize ) &&
            !plog->FLogDisabled() &&
            pinst->ErrReserveIOREQ() >= JET_errSuccess )
    {
        if ( plog->ErrLGInitTmpLogBuffers() < JET_errSuccess )
        {
            pinst->UnreserveIOREQ();
        }
        else
        {
            fTmpLogBuffersInitialized = fTrue;
        }
    }

    WCHAR   wszBuf[ 16 ]            = { 0 };
    wszBuf[0] = L'\0';
    if (    FOSConfigGet( L"BF", L"Waypoint Depth", wszBuf, sizeof( wszBuf ) ) &&
            wszBuf[ 0 ] )
    {
        LONG lWaypointDepth = _wtol( wszBuf );
        if ( lWaypointDepth < 1023 && lWaypointDepth >= 0 )
        {
            (void)Param( pinst, JET_paramWaypointLatency )->Set( pinst, ppibNil, lWaypointDepth, NULL );
        }
    }

    wszBuf[0] = L'\0';
    if (    FOSConfigGet( L"BF", L"Checkpoint Dependancy Overscanning", wszBuf, sizeof( wszBuf ) ) &&
            wszBuf[ 0 ] )
    {
        extern BOOL g_fBFOverscanCheckpoint;
        g_fBFOverscanCheckpoint = _wtol( wszBuf );
    }

    wszBuf[0] = L'\0';
    if (    FOSConfigGet( L"BF", L"Enable Foreground Checkpoint Maint", wszBuf, sizeof( wszBuf ) ) &&
            wszBuf[ 0 ] )
    {
        extern BOOL g_fBFEnableForegroundCheckpointMaint;
        g_fBFEnableForegroundCheckpointMaint = 1 == _wtol( wszBuf ) ? fTrue : fFalse;
    }

    pinst->m_isdlInit.Trigger( eInitConfigInitDone );

    

    OSStrCbFormatW( rgwszDw[0], sizeof(rgwszDw[0]), L"%d", IpinstFromPinst( pinst ) );
    OSStrCbFormatW( rgwszDw[1], sizeof(rgwszDw[1]), L"%d", DwUtilImageVersionMajor() );
    OSStrCbFormatW( rgwszDw[2], sizeof(rgwszDw[2]), L"%02d", DwUtilImageVersionMinor() );
    OSStrCbFormatW( rgwszDw[3], sizeof(rgwszDw[3]), L"%04d", DwUtilImageBuildNumberMajor() );
    OSStrCbFormatW( rgwszDw[4], sizeof(rgwszDw[4]), L"%04d", DwUtilImageBuildNumberMinor() );

    rgcwszT[0] = rgwszDw[0];
    rgcwszT[1] = rgwszDw[1];
    rgcwszT[2] = rgwszDw[2];
    rgcwszT[3] = rgwszDw[3];
    rgcwszT[4] = rgwszDw[4];

    UtilReportEvent(
        eventInformation,
        GENERAL_CATEGORY,
        START_INSTANCE_ID,
        _countof( rgcwszT ),
        rgcwszT,
        0,
        NULL,
        pinst );

    
    pinst->SetStatusRedo();

    if ( !plog->FLogDisabled() )
    {
        DBMS_PARAM  dbms_param;

        
        CallJ( plog->ErrLGInit( &fNewCheckpointFile ), Uninitialize );
        fLGInitIsDone = fTrue;

        
        pinst->SaveDBMSParams( &dbms_param );

        pinst->m_isdlInit.Trigger( eInitBaseLogInitDone );

        
        if ( grbit & ( JET_bitExternalRecoveryControl |
                        JET_bitLogStreamMustExist |
                        JET_bitRecoveryWithoutUndo |
                        JET_bitReplayIgnoreLostLogs |
                        JET_bitAllowSoftRecoveryOnBackup |
                        JET_bitAllowMissingCurrentLog |
                        JET_bitSkipLostLogsEvent ) )
        {
            RecCtrlDefaults.grbit = grbit;
            if ( grbit & JET_bitRecoveryWithoutUndo )
            {
                RecCtrlDefaults.grbit |= ( JET_bitAllowSoftRecoveryOnBackup |
                                            JET_bitAllowMissingCurrentLog |
                                            JET_bitSkipLostLogsEvent );
            }
            RecCtrlDefaults.pinst = pinst;
            RecCtrlDefaults.pfnUserControl = pinst->m_pfnInitCallback;
            RecCtrlDefaults.pvUserContext = pinst->m_pvInitCallbackContext;
            pinst->m_pfnInitCallback = ErrRecoveryControlDefaultCallback;
            pinst->m_pvInitCallbackContext = &RecCtrlDefaults;
        }

        err = plog->ErrLGSoftStart(
                !!(grbit & JET_bitKeepDbAttachedAtEndOfRecovery),
                !!(grbit & JET_bitReplayInferCheckpointFromRstmapDbs),
                &fNewCheckpointFile,
                &fJetLogGeneratedDuringSoftStart );

        CallJ( err, TermLG );

        Assert( plog->CSecLGFile() == __int64( ( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 ) / plog->CbLGSec() );

        
        CallJ( ErrLGStart( pinst ), TermLG );
    }
    pinst->m_isdlInit.Trigger( eInitLogInitDone );

    pinst->SetStatusRuntime();

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    
    CallJ( pinst->ErrINSTInit(), TermLG );
    
    if ( !pinst->m_prbs )
    {
        CallJ( CRevertSnapshot::ErrRBSInitFromRstmap( pinst ), TermIT );

        if ( pinst->m_prbs && pinst->m_prbs->FRollSnapshot() )
        {
            CallJ( pinst->m_prbs->ErrRollSnapshot( fTrue, fTrue ), TermIT );
        }
    }

    Assert( !pinst->FRecovering() );
    Assert( !fJetLogGeneratedDuringSoftStart || !plog->FLogDisabled() );

    
    if ( !plog->FLogDisabled() )
    {
        WCHAR   wszPathJetChkLog[IFileSystemAPI::cchPathMax];

        plog->LGFullNameCheckpoint( wszPathJetChkLog );
        err = plog->ErrLGReadCheckpoint( wszPathJetChkLog, NULL, fTrue );
        if ( JET_errCheckpointFileNotFound == err
            && ( fNewCheckpointFile || fJetLogGeneratedDuringSoftStart ) )
        {
            plog->SetCheckpointEnabled( fTrue );
            (VOID) plog->ErrLGUpdateCheckpointFile( fTrue );

            if ( fJetLogGeneratedDuringSoftStart )
            {
                Assert( plog->LGGetCurrentFileGenNoLock() == plog->LgenInitial() );

                plog->LGVerifyFileHeaderOnInit();
            }

                err = JET_errSuccess;
        }
        else
        {
            CallJ( err, TermIT );
        }

        if ( err >= JET_errSuccess )
        {
            CallS( err );
            err = plog->ErrLGMostSignificantRecoveryWarning();
            Assert( err >= JET_errSuccess );
        }
    }

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    return err;

TermIT:
    const ERR errT = pinst->ErrINSTTerm( termtypeError );
    Assert( errT == JET_errSuccess || pinst->m_fTermAbruptly );

TermLG:
    plog->LGDisableCheckpoint();

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        const IFMP  ifmp    = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp < g_ifmpMax )
        {
            g_rgfmp[ifmp].SetFDontStartDBM();
            g_rgfmp[ifmp].StopDBMScan();

            BFPurge( ifmp );
        }
    }

    (VOID)ErrIOTerm( pinst, fFalse  );

    if ( fLGInitIsDone )
    {
        (VOID)plog->ErrLGTerm( fFalse  );
    }

    if ( fJetLogGeneratedDuringSoftStart )
    {
        WCHAR   wszLogName[ IFileSystemAPI::cchPathMax ];

        plog->LGMakeLogName( wszLogName, sizeof(wszLogName), eCurrentLog );
        (VOID)pinst->m_pfsapi->ErrFileDelete( wszLogName );
    }

Uninitialize:
    if ( fTmpLogBuffersInitialized )
    {
        plog->LGTermTmpLogBuffers();
    }
    
    return err;
}

ERR ISAMAPI ErrIsamTerm( JET_INSTANCE instance, JET_GRBIT grbit )
{
    ERR         err                     = JET_errSuccess;
    INST* const pinst                   = (INST *)instance;
    const BOOL  fInstanceUnavailable    = pinst->FInstanceUnavailable();
    TERMTYPE    termtype                = termtypeCleanUp;


    if ( fInstanceUnavailable )
    {
        termtype = termtypeError;
    }
    else if ( grbit & JET_bitTermDirty )
    {
        termtype = termtypeError;
    }
    else if ( grbit & JET_bitTermAbrupt )
    {
        termtype = termtypeNoCleanUp;
    }


    err = pinst->ErrINSTTerm( termtype );

    if ( pinst->m_fSTInit != fSTInitNotDone )
    {
        
        Assert( err < 0 );
        pinst->m_isdlTerm.TermSequence();
        return err;
    }
    if ( err >= JET_errSuccess && ( grbit & JET_bitTermDirty ) )
    {
        err = ErrERRCheck( JET_errDirtyShutdown );
    }

    const ERR   errT    = pinst->m_plog->ErrLGTerm(
                                            ( err >= JET_errSuccess && termtypeError != termtype ) );
    if ( err >= JET_errSuccess && errT < JET_errSuccess )
    {
        err = errT;
    }

    if( pinst->m_prbs )
    {
        delete pinst->m_prbs;
        pinst->m_prbs = NULL;
    }

    if ( pinst->m_prbscleaner )
    {
        delete pinst->m_prbscleaner;
        pinst->m_prbscleaner = NULL;
    }

    if ( pinst->m_prbsrc )
    {
        delete pinst->m_prbsrc;
        pinst->m_prbsrc = NULL;
    }

    pinst->m_isdlTerm.Trigger( eTermLogTermDone );


    delete pinst->m_pfsapi;
    pinst->m_pfsapi = NULL;

    IRSCleanUpAllIrsResources( pinst );

    pinst->m_isdlTerm.Trigger( eTermDone );

    const ULONG cbTimingResourceDataSequence = pinst->m_isdlTerm.CbSprintTimings();
    Expected( cbTimingResourceDataSequence < 8192 );
    WCHAR * wszTimingResourceDataSequence = (WCHAR *)_alloca( cbTimingResourceDataSequence );
    pinst->m_isdlTerm.SprintTimings( wszTimingResourceDataSequence, cbTimingResourceDataSequence );

    
    if (    err >= JET_errSuccess && termtypeError != termtype ||
            err == JET_errDirtyShutdown && termtypeError == termtype )
    {
        WCHAR       wszT[16];
        WCHAR       wszDirty[16];
        const WCHAR *rgcwszT[3];

        Assert( !fInstanceUnavailable || ( err == JET_errDirtyShutdown && termtypeError == termtype ) );

        OSStrCbFormatW( wszT, sizeof( wszT ), L"%d", IpinstFromPinst( pinst ) );
        OSStrCbFormatW( wszDirty, sizeof( wszDirty ), L"%d",
                ( err == JET_errDirtyShutdown && termtypeError == termtype ) ? 1 : 0 );

        rgcwszT[0] = wszT;
        rgcwszT[1] = wszTimingResourceDataSequence;
        rgcwszT[2] = wszDirty;
        
        UtilReportEvent(
            eventInformation,
            GENERAL_CATEGORY,
            STOP_INSTANCE_ID,
            _countof( rgcwszT ),
            rgcwszT,
            0,
            NULL,
            pinst );

        ULONG rgul[2] = { (ULONG)pinst->m_iInstance, (ULONG)err  };
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlTermSucceed|sysosrtlContextInst, pinst, rgul, sizeof(rgul) );
    }
    else
    {
        WCHAR       wsz1[16];
        WCHAR       wsz2[16];
        const WCHAR *rgcwszT[3];

        OSStrCbFormatW( wsz1, sizeof( wsz1 ), L"%d", IpinstFromPinst( pinst ) );
        OSStrCbFormatW( wsz2, sizeof( wsz2 ), L"%d", fInstanceUnavailable ? pinst->ErrInstanceUnavailableErrorCode() : err );
        rgcwszT[0] = wsz1;
        rgcwszT[1] = wsz2;
        rgcwszT[2] = wszTimingResourceDataSequence;
        UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            STOP_INSTANCE_ID_WITH_ERROR,
            _countof( rgcwszT ),
            rgcwszT,
            0,
            NULL,
            pinst );
    }

    pinst->m_isdlTerm.TermSequence();

    if ( err < JET_errSuccess && err != JET_errDirtyShutdown )
    {
        ULONG rgul[2] = { (ULONG)pinst->m_iInstance, (ULONG)err };
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlTermFail|sysosrtlContextInst, pinst, rgul, sizeof(rgul) );
    }
    return err;
}


ERR ErrIsamSetSessionParameter(
    _In_opt_ JET_SESID                          vsesid,
    _In_ ULONG                          sesparamid,
    _In_reads_bytes_opt_( cbParam ) void *      pvParam,
    _In_ ULONG                      cbParam )
{
    PIB     * ppib  = (PIB *)vsesid;

    if ( pvParam == NULL && cbParam != 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    switch( sesparamid )
    {

    case JET_sesparamCommitDefault:
        if ( cbParam != sizeof(JET_GRBIT) )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        ppib->grbitCommitDefault = *(JET_GRBIT *)pvParam;
        break;

    case JET_sesparamCommitGenericContext:
        return ppib->ErrSetClientCommitContextGeneric( pvParam, cbParam );

    case JET_sesparamTransactionLevel:
        return ErrERRCheck( JET_errFeatureNotAvailable );

    case JET_sesparamOperationContext:
        return ppib->ErrSetOperationContext( pvParam, cbParam );

    case JET_sesparamCorrelationID:
        return ppib->ErrSetCorrelationID( pvParam, cbParam );

    case JET_sesparamCachePriority:
        return ppib->ErrSetCachePriority( pvParam, cbParam );

    case JET_sesparamClientComponentDesc:
        return ppib->ErrSetClientComponentDesc( pvParam, cbParam );

    case JET_sesparamClientActionDesc:
        return ppib->ErrSetClientActionDesc( pvParam, cbParam );

    case JET_sesparamClientActionContextDesc:
        return ppib->ErrSetClientActionContextDesc( pvParam, cbParam );

    case JET_sesparamClientActivityId:
        return ppib->ErrSetClientActivityID( pvParam, cbParam );

    case JET_sesparamIOSessTraceFlags:
        return ppib->ErrSetIOSESSTraceFlags( pvParam, cbParam );

    case JET_sesparamIOPriority:
        return ppib->ErrSetUserIoPriority( pvParam, cbParam );

    case JET_sesparamCommitContextContainsCustomerData:
        return ppib->ErrSetCommitContextContainsCustomerData( pvParam, cbParam );

    default:
        Expected( ( sesparamid >= JET_sesparamCommitDefault )  && ( sesparamid < ( JET_sesparamCommitDefault + 1024 ) ) );
        return ErrERRCheck( JET_errInvalidSesparamId );
    }

    return JET_errSuccess;
}


ERR ErrIsamGetSessionParameter(
    _In_opt_ JET_SESID                                          vsesid,
    _In_ ULONG                                          sesparamid,
    _Out_cap_post_count_(cbParamMax, *pcbParamActual) void *    pvParam,
    _In_ ULONG                                          cbParamMax,
    _Out_opt_ ULONG *                                   pcbParamActual )
{
    PIB     * ppib  = (PIB *)vsesid;

    if ( pvParam == NULL && pcbParamActual == NULL )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    switch( sesparamid )
    {

    case JET_sesparamCommitDefault:
        if ( pcbParamActual )
        {
            *pcbParamActual = sizeof(JET_GRBIT);
        }
        if ( pvParam && cbParamMax != sizeof(JET_GRBIT) )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            (*(JET_GRBIT*)pvParam) = ppib->grbitCommitDefault;
        }
        break;

    case JET_sesparamCommitGenericContext:
    {
        const DWORD cbParamActual = ppib->CbClientCommitContextGeneric();
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT)cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        if ( pvParam )
        {
            memcpy( pvParam, ppib->PvClientCommitContextGeneric(), cbParamActual );
        }
        break;
    }

    case JET_sesparamTransactionLevel:
        if ( pcbParamActual )
        {
            *pcbParamActual = sizeof(LONG);
        }
        if ( pvParam && cbParamMax != sizeof(LONG) )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            (*(LONG*)pvParam) = ppib->Level();
        }
        break;

    case JET_sesparamOperationContext:
    {
        static_assert( sizeof( JET_OPERATIONCONTEXT ) == sizeof( OPERATION_CONTEXT ), "JET_OPERATIONCONTEXT and OPERATION_CONTEXT should be the same size" );

        const DWORD cbParamActual = sizeof( JET_OPERATIONCONTEXT );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT)cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            memcpy( pvParam, ppib->PvOperationContext( ), cbParamActual );
        }
        break;
    }

    case JET_sesparamCorrelationID:
    {
        const DWORD cbParamActual = sizeof(DWORD);
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT)cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            memcpy( pvParam, ppib->PvCorrelationID( ), cbParamActual );
        }
        break;
    }

    case JET_sesparamCachePriority:
    {
        const DWORD cbParamActual = sizeof(DWORD);
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT)cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            const ULONG_PTR pctCachePriorityT = ppib->PctCachePriorityPib();
            const DWORD pctCachePriority = (DWORD)pctCachePriorityT;
            Assert( pctCachePriorityT == (ULONG_PTR)pctCachePriority );
            Assert( FIsCachePriorityValid( pctCachePriority ) );
            memcpy( pvParam, &pctCachePriority, cbParamActual );
        }
        break;
    }

    case JET_sesparamClientComponentDesc:
    {
        const char* const pszClientComponent = ppib->PszClientComponentDesc();
        const DWORD cbParamActual = LOSStrLengthA( pszClientComponent ) + sizeof( pszClientComponent[ 0 ] );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT) cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        if ( pvParam )
        {
            OSStrCbCopyA( (char*) pvParam, cbParamMax, pszClientComponent );
        }
        break;
    }

    case JET_sesparamClientActionDesc:
    {
        const char* const pszClientAction = ppib->PszClientActionDesc();
        const DWORD cbParamActual = LOSStrLengthA( pszClientAction ) + sizeof( pszClientAction[ 0 ] );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT) cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        if ( pvParam )
        {
            OSStrCbCopyA( (char*) pvParam, cbParamMax, pszClientAction );
        }
        break;
    }

    case JET_sesparamClientActionContextDesc:
    {
        const char* const pszClientActionContext = ppib->PszClientActionContextDesc();
        const DWORD cbParamActual = LOSStrLengthA( pszClientActionContext ) + sizeof( pszClientActionContext[ 0 ] );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT) cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errBufferTooSmall );
        }
        if ( pvParam )
        {
            OSStrCbCopyA( (char*) pvParam, cbParamMax, pszClientActionContext );
        }
        break;
    }

    case JET_sesparamClientActivityId:
    {
        const DWORD cbParamActual = sizeof( GUID );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT) cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            memcpy( pvParam, ppib->PguidClientActivityID(), cbParamActual );
        }
        break;
    }

    case JET_sesparamIOSessTraceFlags:
    {
        const DWORD cbParamActual = sizeof( DWORD );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && (INT) cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            memcpy( pvParam, ppib->PvIOSessTraceFlags(), cbParamActual );
        }
        break;
    }

    case JET_sesparamIOPriority:
        if ( pcbParamActual )
        {
            *pcbParamActual = sizeof(JET_GRBIT);
        }
        if ( pvParam && cbParamMax != sizeof(JET_GRBIT) )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            (*(JET_GRBIT*)pvParam) = ppib->GrbitUserIoPriority();
        }
        break;

    case JET_sesparamCommitContextContainsCustomerData:
    {
        const DWORD cbParamActual = sizeof( DWORD );
        if ( pcbParamActual )
        {
            *pcbParamActual = cbParamActual;
        }
        if ( pvParam && cbParamMax < cbParamActual )
        {
            return ErrERRCheck( JET_errInvalidBufferSize );
        }
        if ( pvParam )
        {
            *(DWORD *)pvParam = ppib->FCommitContextContainsCustomerData();
        }
        break;
    }

    default:
        Expected( ( sesparamid >= JET_sesparamCommitDefault )  && ( sesparamid < ( JET_sesparamCommitDefault + 1024 ) ) );
        return ErrERRCheck( JET_errInvalidSesparamId );
    }

    return JET_errSuccess;
}


ERR ISAMAPI ErrIsamBeginTransaction( JET_SESID vsesid, TRXID trxid, JET_GRBIT grbit )
{
    PIB     * ppib  = (PIB *)vsesid;
    ERR     err;

    CallR( ErrPIBCheck( ppib ) );
    Assert( ppibNil != ppib );
    Assert( ppib->Level() > 0 || !ppib->FMustRollbackToLevel0() );
    
    if ( ppib->Level() < levelUserMost )
    {
        ppib->ptlsTrxBeginLast = Ptls();

        const JET_GRBIT     grbitsSupported     = JET_bitTransactionReadOnly;
        err = ErrDIRBeginTransaction(
                    ppib,
                    trxid,
                    ( grbit & grbitsSupported ) );
    }
    else
    {
        Assert( levelUserMost == ppib->Level() );
        err = ErrERRCheck( JET_errTransTooDeep );
    }

    return err;
}

extern JET_GRBIT g_grbitCommitFlagsMsk;

ERR ISAMAPI ErrIsamCommitTransaction( JET_SESID vsesid, JET_GRBIT grbit, DWORD cmsecDurableCommit, JET_COMMIT_ID *pCommitId )
{
    ERR     err;
    PIB     *ppib = (PIB *)vsesid;
    FUCB    *pfucb = pfucbNil;
    
    CallR( ErrPIBCheck( ppib ) );

    if ( grbit & ~g_grbitCommitFlagsMsk )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    if ( JET_bitForceNewLog & grbit )
    {
        if ( JET_bitForceNewLog != grbit )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        LGPOS lgpos;
        CallR( ErrLGForceLogRollover( ppib, __FUNCTION__, &lgpos ) );
        return ErrLGWaitForWrite( ppib, &lgpos );
    }

    if ( JET_bitWaitAllLevel0Commit & grbit )
    {
        if ( JET_bitWaitAllLevel0Commit != grbit )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        PERFOpt( cUserWaitAllTrxCommit.Inc( ppib->m_pinst ) );

        return ErrLGForceWriteLog( ppib );
    }

    if ( grbit & JET_bitWaitLastLevel0Commit )
    {
        Expected( ppib->Level() == 0 || !ppib->FBegin0Logged() );

        if ( JET_bitWaitLastLevel0Commit != grbit )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        if ( CmpLgpos( &ppib->lgposCommit0, &lgposMax ) == 0 )
        {
            return JET_errSuccess;
        }

        PERFOpt( cUserWaitLastTrxCommit.Inc( ppib->m_pinst ) );

        err = ErrLGWaitForWrite( ppib, &ppib->lgposCommit0 );
        Assert( err >= 0 || PinstFromPpib( ppib )->m_plog->FNoMoreLogWrite() );

        return err;
    }

    if ( ppib->Level() == 0 )
    {
        return ErrERRCheck( JET_errNotInTransaction );
    }

    if ( ppib->FMustRollbackToLevel0() )
    {
        return ErrERRCheck( JET_errMustRollback );
    }

    err = ErrDIRCommitTransaction( ppib, grbit, cmsecDurableCommit, pCommitId );

    if ( ppib->Level() == 0 )
    {
        for ( pfucb = ppib->pfucbOfSession; pfucb != pfucbNil; pfucb = pfucb->pfucbNextOfSession )
        {
            if ( FFUCBSecondary( pfucb ) || FFUCBLongValue( pfucb ) )
                continue;
            if ( pfucb->u.pfcb->FUncommitted() )
            {
                Assert( FFMPIsTempDB( pfucb->ifmp ) );
                pfucb->u.pfcb->Lock();
                pfucb->u.pfcb->ResetUncommitted();
                pfucb->u.pfcb->Unlock();
            }
        }
    }


    return err;
}


ERR ISAMAPI ErrIsamRollback( JET_SESID vsesid, JET_GRBIT grbit )
{
    ERR         err;
    PIB         * ppib          = (PIB *)vsesid;
    FUCB        * pfucb;
    FUCB        * pfucbNext;


    
    CallR( ErrPIBCheck( ppib ) );

    if ( ppib->Level() == 0 )
    {
        return ErrERRCheck( JET_errNotInTransaction );
    }

    do
    {
        const LEVEL levelRollback   = LEVEL( ppib->Level() - 1 );
        
        
        for ( pfucb = ppib->pfucbOfSession;
            pfucb != pfucbNil && ( FFUCBSecondary( pfucb ) || FFUCBLongValue( pfucb ) );
            pfucb = pfucb->pfucbNextOfSession )
            NULL;

        
        for ( ; pfucb != pfucbNil; pfucb = pfucbNext )
        {
            
            for ( pfucbNext = pfucb->pfucbNextOfSession;
                pfucbNext != pfucbNil && ( FFUCBSecondary( pfucbNext ) || FFUCBLongValue( pfucbNext ) );
                pfucbNext = pfucbNext->pfucbNextOfSession )
                NULL;

            
            if ( FFUCBDeferClosed( pfucb ) )
                continue;

            if ( FFUCBUpdatePreparedLevel( pfucb, pfucb->ppib->Level() ) )
            {
                RECIFreeCopyBuffer( pfucb );
                FUCBResetUpdateFlags( pfucb );
            }

            Assert( 0 == pfucb->levelReuse || pfucb->levelReuse >= pfucb->levelOpen );

            
            if ( FFUCBIndex( pfucb ) && pfucb->u.pfcb->FPrimaryIndex() )
            {
                if ( pfucb->levelOpen > levelRollback ||
                    pfucb->levelReuse > levelRollback )
                {
                    if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
                    {
                        CallS( ErrDispCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
                    }
                    else
                    {
                        CallS( ErrFILECloseTable( ppib, pfucb ) );
                    }
                    continue;
                }
                
                
                if ( pfucb->pfucbCurIndex != pfucbNil )
                {
                    if ( pfucb->pfucbCurIndex->levelOpen > levelRollback )
                    {
                        CallS( ErrRECSetCurrentIndex( pfucb, NULL, NULL ) );
                    }
                }
            }

            if ( pfucbNil != pfucb->pfucbLV
                && ( pfucb->pfucbLV->levelOpen > levelRollback ||
                     pfucb->pfucbLV->levelReuse > levelRollback ) )
            {
                DIRClose( pfucb->pfucbLV );
                pfucb->pfucbLV = pfucbNil;
            }

            
            if ( FFUCBSort( pfucb ) )
            {
                if ( pfucb->levelOpen > levelRollback ||
                    pfucb->levelReuse > levelRollback )
                {
                    SORTClose( pfucb );
                    continue;
                }
            }

            
            if ( pfucb->levelOpen > levelRollback ||
                pfucb->levelReuse > levelRollback )
            {
                DIRClose( pfucb );
                continue;
            }
        }

        
        err = ErrDIRRollback( ppib, grbit );
        if ( JET_errRollbackError == err )
        {
            err = ppib->ErrRollbackFailure();

            Assert( err < JET_errSuccess );
        }
        CallR( err );
    }
    while ( ( grbit & JET_bitRollbackAll ) != 0 && ppib->Level() > 0 );

    if ( ppib->Level() == 0 )
    {
        ppib->ResetMustRollbackToLevel0();
    }
    
    return JET_errSuccess;
}

