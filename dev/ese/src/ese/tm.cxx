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

#endif // PERFMON_SUPPORT

//  BF needs to know global / multi-inst worst case page size we may use.

LONG CbSYSMaxPageSize()
{
    return (LONG)UlParam( JET_paramDatabasePageSize );
}

//  Log needs to know inst-wide what is the worst case page size we may use.

LONG CbINSTMaxPageSize( const INST * const pinst )
{
    return (LONG)UlParam( pinst, JET_paramDatabasePageSize );
}



//+api
//  ErrIsamBeginSession
//  ========================================================
//  ERR ErrIsamBeginSession( PIB **pppib )
//
//  Begins a session with DAE.  Creates and initializes a PIB for the
//  user and returns a pointer to it.  Calls system initialization.
//
//  PARAMETERS  pppib           Address of a PIB pointer.  On return, *pppib
//                              will point to the new PIB.
//
//  RETURNS     Error code, one of:
//                  JET_errSuccess
//                  JET_errOutOfSessions
//
//  SEE ALSO        ErrIsamEndSession
//-
ERR ISAMAPI ErrIsamBeginSession( JET_INSTANCE inst, JET_SESID *psesid )
{
    ERR     err;
    PIB     **pppib;
    INST    *pinst = (INST *)inst;

    Assert( psesid != NULL );
    Assert( sizeof(JET_SESID) == sizeof(PIB *) );
    pppib = (PIB **)psesid;

    //  allocate process information block
    //
    Call( ErrPIBBeginSession( pinst, pppib, procidNil, fFalse ) );
    (*pppib)->grbitCommitDefault = pinst->m_grbitCommitDefault;    /* set default commit flags */
    (*pppib)->SetFUserSession();

HandleError:
    return err;
}


//+api
//  ErrIsamEndSession
//  =========================================================
//  ERR ErrIsamEndSession( PIB *ppib, JET_GRBIT grbit )
//
//  Ends the session associated with a PIB.
//
//  PARAMETERS  ppib        Pointer to PIB for ending session.
//
//  RETURNS     JET_errSuccess
//
//  SIDE EFFECTS
//      Rolls back all transaction levels active for this PIB.
//      Closes all FUCBs for files and sorts open for this PIB.
//
//  SEE ALSO    ErrIsamBeginSession
//-
ERR ISAMAPI ErrIsamEndSession( JET_SESID sesid, JET_GRBIT grbit )
{
    ERR     err;
    PIB     *ppib   = (PIB *)sesid;

    CallR( ErrPIBCheck( ppib ) );

    //  lock session
    err = ppib->ErrPIBSetSessionContext( dwPIBSessionContextUnusable );
    if ( err < 0 )
    {
        if ( JET_errSessionContextAlreadySet == err )
            err = ErrERRCheck( JET_errSessionInUse );
        return err;
    }

    INST    *pinst  = PinstFromPpib( ppib );

    //  rollback all transactions
    //
    if ( ppib->Level() > 0 )
    {
        ppib->PrepareTrxContextForEnd();

        Assert( sizeof(JET_SESID) == sizeof(ppib) );
        Call( ErrIsamRollback( (JET_SESID)ppib, JET_bitRollbackAll ) );
        Assert( 0 == ppib->Level() );
    }

    //  close all databases for this PIB
    //
    Call( ErrDBCloseAllDBs( ppib ) );

    //  close all cursors still open
    //
    while( ppib->pfucbOfSession != pfucbNil )
    {
        FUCB    *pfucb  = ppib->pfucbOfSession;

        //  close materialized or unmaterialized temporary tables
        //
        if ( FFUCBSort( pfucb ) )
        {
            Assert( !( FFUCBIndex( pfucb ) ) );
            Call( ErrIsamSortClose( ppib, pfucb ) );
        }
        else if ( pinst->FRecovering() )
        {
            //  If the fucb is used for redo, then it is
            //  always being opened as a primary FUCB with default index.
            //  Use DIRClose to close such a fucb.
            DIRClose( pfucb );
        }
        else
        {
            //  should only be sort and temporary file cursors
            //  (cursors for user databases should already have
            //  been closed by the call above to ErrDBCloseAllDBs())
            //
            Assert( FFMPIsTempDB( pfucb->ifmp ) );

            while ( FFUCBSecondary( pfucb ) || FFUCBLongValue( pfucb ) )
            {
                pfucb = pfucb->pfucbNextOfSession;
                Assert( FFMPIsTempDB( pfucb->ifmp ) );

                //  a table cursor that owns this index/lv cursor must
                //  be ahead somewhere
                AssertRTL( pfucbNil != pfucb );
            }

            Assert( FFUCBIndex( pfucb ) );

            if ( pfucb->pvtfndef != &vtfndefInvalidTableid )
            {
                CallS( ErrDispCloseTable( (JET_SESID)ppib, (JET_TABLEID) pfucb ) );
            }
            else
            {
                //  Open internally, not exported to user
                //
                CallS( ErrFILECloseTable( ppib, pfucb ) );
            }
        }
    }
    Assert( pfucbNil == ppib->pfucbOfSession );

    PIBEndSession( ppib );

    return JET_errSuccess;

HandleError:
    Assert( err < 0 );

    //  could not properly terminate session, must leave it in usable state
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

    //  currently only support JET_SessionInfo
    //
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
    //  Special verstore test bit 
    //
    //  This validates that all the RCE hashes are correct in current verstore state.  
    //  Note: There is no usage of this in test ese today / May 2017.
    if ( grbit & JET_bitIdleVersionStoreTest )
    {
        //  return error code for status
        VER *pver = PverFromPpib( (PIB *) sesid );
        Call( pver->ErrInternalCheck() );
    }
#endif  //  !RTM

    //  Handle verstore idle / maintenance tasks
    //
    if ( grbit & JET_bitIdleStatus )
    {
        //  return error code for status
        VER *pver = PverFromPpib( (PIB *) sesid );
        Call( pver->ErrVERStatus() );
    }
    else if ( 0 == grbit || JET_bitIdleCompact & grbit  )
    {
        //  clean all version buckets
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

        // Test for remaining version store buckets before waiting for DB tasks
        // because version store cleanup might issue DB tasks and cleanup an RCE
        // before the DB task is actually completed.
        {
        VER* const pver = PverFromPpib( ppib );
        DWORD_PTR cVerBuckets = 0;
        ENTERCRITICALSECTION enterCritRCEBucketGlobal( &pver->m_critBucketGlobal );
        CallS( pver->m_cresBucket.ErrGetParam( JET_resoperCurrentUse, &cVerBuckets ) );
        err = ( cVerBuckets != 0 ) ? ErrERRCheck( JET_wrnRemainingVersions ) : JET_errSuccess;
        }

        // Wait for DB tasks to quiesce.
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

    //  Handle db cache idle / maintenance tasks
    //
    //  Possibly implement on need
    //      o Triggering + wait avail pool maintenance
    //      o Triggering avail pool maintenance async
    //      o Checking checkpoint depth status (other methods for this as well)
    //      o Triggering + wait checkpoint advancement
    //      o Triggering checkpoint advancement async
    if ( grbit & JET_bitIdleFlushBuffers )
    {
        //  flush some dirty buffers
        Call( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    if ( grbit & JET_bitIdleAvailBuffersStatus )
    {
        Call( ErrBFCheckMaintAvailPoolStatus() );
        if ( wrnFirst == JET_errSuccess )
        {
            //  Upgrade warning
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

// Controls whether periodic database trim is available.
BOOL g_fPeriodicTrimEnabled = fFalse;   // Configured in ErrITSetConstants()

DWORD g_dwGlobalMajorVersion;
DWORD g_dwGlobalMinorVersion;
DWORD g_dwGlobalBuildNumber;
LONG g_lGlobalSPNumber;


ERR ISAMAPI ErrIsamSystemInit()
{
    ERR err;

    //  US English MUST be installed (to ensure
    //  against different LCMapString() results based on
    //  whether the specified lcid is installed
    Call( ErrNORMCheckLocaleName( pinstNil, wszLocaleNameDefault ) );
    AssertNORMConstants();
    QWORD qwSortVersion = 0;
    err = ErrNORMGetSortVersion( wszLocaleNameDefault, &qwSortVersion, NULL );
    CallS( err );
    Call( err );
    Assert( qwSortVersion != 0 );
    Assert( qwSortVersion != qwMax );
    Assert( qwSortVersion != g_qwUpgradedLocalesTable );

    //  Get OS info, LocalID etc

    g_dwGlobalMajorVersion = DwUtilSystemVersionMajor();
    g_dwGlobalMinorVersion = DwUtilSystemVersionMinor();
    g_dwGlobalBuildNumber = DwUtilSystemBuildNumber();
    g_lGlobalSPNumber = DwUtilSystemServicePackNumber();

    //  Set JET constant.

    Call( ErrITSetConstants( ) );

    //  Initialize instances

    Call( INST::ErrINSTSystemInit() );

    CallJ( LGInitTerm::ErrLGSystemInit(), TermInstInit );

    //  initialize file map

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

//  The contract is that if the function wishes to alter behavior it will set
//  *perr and return fTrue, otherwise return fFalse for no interest.

BOOL FEnsureLogStreamMustExist( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, _Out_ JET_ERR * const perr )
{
    if ( snt == JET_sntMissingLog &&
            pRecCtrl->MissingLog.eNextAction == JET_MissingLogCreateNewLogStream )
    {
        AssertRTL( pRecCtrl->MissingLog.lGenMissing == 0 );

        //  Should be default action of success, but we want to fail the application in this case
        Assert( pRecCtrl->errDefault == JET_errSuccess );
        *perr = ErrERRCheckDefault( JET_errFileNotFound );
        return fTrue;
    }

    return fFalse;
}

BOOL FAllowMissingCurrentLog( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, _Out_ JET_ERR * const perr )
{
    if ( snt == JET_sntMissingLog )
    {
        if ( ( pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToRedo ||
                    pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToUndo ) &&
                pRecCtrl->MissingLog.fCurrentLog )
        {
            //  Should be default action to fail, but if users want, we can continue to recovery        
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

BOOL FReplayIgnoreLostLogs( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, _Out_ JET_ERR * const perr )
{
    if ( snt == JET_sntMissingLog )
    {
        if ( ( pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToRedo ||
                    pRecCtrl->MissingLog.eNextAction == JET_MissingLogContinueToUndo ) &&
                pRecCtrl->MissingLog.fCurrentLog )
        {
            //  Should be default action to fail, but we allow loss here, so we want to continue to recovery
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
        //  Should be successful, but if we're allowing loss, we want the code to be
        //  robust so we're just going to ignore the checkpoint and regenerate it.
        AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
    }

    return fFalse;
}

BOOL FAllowSoftRecoveryOnBackup( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, _Out_ JET_ERR * const perr )
{
    if ( snt == JET_sntSignalErrorCondition &&
            pRecCtrl->errDefault == JET_errSoftRecoveryOnBackupDatabase )
    {
        //  Should be a default action to fail in this case, but allowed if the user specifies it.
        *perr = JET_errSuccess;
        return fTrue;
    }

    return fFalse;
}

BOOL FSkipLostLogsEvent( const JET_SNT snt, const JET_RECOVERYCONTROL * const pRecCtrl, _Out_ JET_ERR * const perr )
{
    if ( snt == JET_sntSignalErrorCondition &&
            pRecCtrl->errDefault == JET_errCommittedLogFilesMissing )
    {
        //  Default action when this fails we report the event, success squelches the event
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

    //  Redo (With Recovery Without Undo) to Undo Transition debugging 
    BOOL                    fSawBeginUndo;
    BOOL                    fSawBeginDo;
    BOOL    FRecoveryWithoutUndo()
    {
        Assert( fSawBeginUndo || fSawBeginDo );
        return grbit & JET_bitRecoveryWithoutUndo;
    }
};

JET_ERR ErrRecoveryControlDefaultCallback(
    _In_ JET_SNP    snp,
    _In_ JET_SNT    snt,
    __in_opt void * pvData,             // depends on the snp, snt
    __in_opt void * pvContext )
{
    JET_ERR errDefaultAction = JET_wrnNyi;  //  This tracks the default error based upon the grbit options provided.

    struct TM_REC_CTRL_DEFAULT * pRecCtrlDefaults = reinterpret_cast<struct TM_REC_CTRL_DEFAULT *>( pvContext );

    OSTrace( JET_tracetagCallbacks, OSFormat( "Executing ErrRecoveryControlDefaultCallback() before user callback\n" ) );

    if( snp == JET_snpRecoveryControl )
    {
        JET_RECOVERYCONTROL * pRecCtrl = reinterpret_cast<JET_RECOVERYCONTROL *>( pvData );

        //  Validation and Processing for Recovery Without Undo is special ...
        //

        switch( snt )
        {
            case JET_sntOpenLog:
                //  Process Recovery Without Undo special handling ...
                //
                if ( pRecCtrl->OpenLog.eReason == JET_OpenLogForDo )
                {
                    // It is possible on a clean log we skip undo, so we track that
                    // we saw "begin do" as well, to validate we don't utilize the
                    // FRecoveryWithoutUndo() option too early.
                    pRecCtrlDefaults->fSawBeginDo = fTrue;
                    if ( pRecCtrlDefaults->FRecoveryWithoutUndo() )
                    {
                        AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
                        errDefaultAction = ErrERRCheckDefault( JET_errRecoveredWithoutUndoDatabasesConsistent );
                    }
                }
                break;
            case JET_sntOpenCheckpoint:
                //  We do nothing for this, this is fine.
                break;
            case JET_sntMissingLog:
                //  Validation:
                //
                if ( pRecCtrl->MissingLog.lGenMissing == 0 )
                {
                    AssertRTL( pRecCtrl->MissingLog.eNextAction == JET_MissingLogCreateNewLogStream );
                }
                break;
            case JET_sntBeginUndo:
                //  Validation:
                //
                AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
                
                //  Process Recovery Without Undo special handling ...
                //
                pRecCtrlDefaults->fSawBeginUndo = fTrue;
                Assert( pRecCtrlDefaults->fSawBeginUndo );
                if ( pRecCtrlDefaults->FRecoveryWithoutUndo() )
                {
                    AssertRTL( pRecCtrl->errDefault == JET_errSuccess );
                    errDefaultAction = ErrERRCheckDefault( JET_errRecoveredWithoutUndo );
                }
                break;

            case JET_sntSignalErrorCondition:
                //  Validation: one of the two expected cases from the code so far
                //
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
                // our own code should handle all cases
                AssertSzRTL( fFalse, "Unknown snt = %d for JET_snpRecoveryControl", snt );
                return ErrERRCheck( JET_wrnNyi );
        }

        //  Processing for all other special behaviors is broken out into sample functions
        //

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

        //  If our grbit processing updated the default action, change the recovery control callback

        if ( errDefaultAction != JET_wrnNyi )
        {
            OSTrace( JET_tracetagCallbacks, OSFormat( "ErrRecoveryControlDefaultCallback() transforming errDefault from %d to %d\n", pRecCtrl->errDefault, errDefaultAction ) );
            pRecCtrl->errDefault = errDefaultAction;
        }
    }

    //  Changes after this point must be carefully plied, as the user callback may except out of
    //  this function and be caught by the LOG callback.

    JET_ERR errRet = JET_wrnNyi;

    if ( pRecCtrlDefaults->pfnUserControl &&
            ( pRecCtrlDefaults->grbit & JET_bitExternalRecoveryControl ||
                snp != JET_snpRecoveryControl ) )
    {
        //  perform the user callback

        OSTrace( JET_tracetagCallbacks, OSFormat( "ErrRecoveryControlDefaultCallback() issuing user callback\n" ) );

        errRet = pRecCtrlDefaults->pfnUserControl( snp, snt, pvData, pRecCtrlDefaults->pvUserContext );

        OSTrace( JET_tracetagCallbacks, OSFormat( "ErrRecoveryControlDefaultCallback() returned --> %d\n", errRet ) );
    }
    else
    {
        //  no user interest in this callback ...

        if ( snp == JET_snpRecoveryControl )
        {
            //  map the error if necesary ...
            errRet = reinterpret_cast<JET_RECOVERYCONTROL *>( pvData )->errDefault;
        }
        else
        {
            //  or succeed the callback like nothing is wrong
            errRet = JET_errSuccess;
        }
    }

    //  if we performed the default action error, then we should trap the error here

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
    Assert( !FNegTest( fLeakingUnflushedIos ) );    // only valid on OS layer or embedded unit tests

    //  create paths now if they do not exist

    if ( BoolParam( pinst, JET_paramCreatePathIfNotExist ) )
    {

        //  make the temp path does NOT have a trailing '\' and the log/sys paths do

        Assert( !FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramTempPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramSystemPath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramLogFilePath ) ) );
        Assert( FOSSTRTrailingPathDelimiterW( SzParam( pinst, JET_paramRBSFilePath ) ) );

        //  create paths

        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramTempPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramSystemPath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramLogFilePath ), NULL, 0 ) );
        CallR( ErrUtilCreatePathIfNotExist( pinst->m_pfsapi, SzParam( pinst, JET_paramRBSFilePath ), NULL, 0 ) );
    }

    //  Get basic global parameters for checking LG parameters

    CallR( plog->ErrLGInitSetInstanceWiseParameters( grbit ) );

    //  Check all the system parameter before we continue

    // log file size should be multiple of sector size
    if ( 0 != ( __int64( ( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 ) % plog->CSecLGFile() ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    // number of sectors should be correctly set based on sector size
    if ( plog->CbLGSec() != (ULONG) ( __int64( UlParam( pinst, JET_paramLogFileSize ) ) * 1024 / plog->CSecLGFile() ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    // cannot delete log files if not doing circular logging
    if ( BoolParam( pinst, JET_paramCleanupMismatchedLogFiles ) && !BoolParam( pinst, JET_paramCircularLog ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    // cannot delete log files if dbs still attached (with required range
    // potentially within those files)
    if ( BoolParam( pinst, JET_paramCleanupMismatchedLogFiles ) && ( grbit & JET_bitKeepDbAttachedAtEndOfRecovery ) )
    {
        return ErrERRCheck( JET_errInvalidSettings );
    }

    //  Set other variables global to this instance

    Assert( pinst->m_updateid == updateidMin );

    //  Reserve IOREQ for LOG::LGICreateAsynchIOIssue. Note that we will only do
    //  asynch creation if we can extend the log file with arbitrarily big
    //  enough I/Os.
    //
    if ( BoolParam( pinst, JET_paramLogFileCreateAsynch ) &&
            cbLogExtendPattern >= UlParam( JET_paramMaxCoalesceWriteSize ) &&
            !plog->FLogDisabled() &&
            pinst->ErrReserveIOREQ() >= JET_errSuccess )
    {
        if ( plog->ErrLGInitTmpLogBuffers() < JET_errSuccess )
        {
            //  we don't need the reserved IOREQ anymore
            //
            pinst->UnreserveIOREQ();
        }
        else
        {
            fTmpLogBuffersInitialized = fTrue;
        }
    }

    //  set waypoint depth override if applicable
    //
    WCHAR   wszBuf[ 16 ]            = { 0 };
    wszBuf[0] = L'\0';
    if (    FOSConfigGet( L"BF", L"Waypoint Depth", wszBuf, sizeof( wszBuf ) ) &&
            wszBuf[ 0 ] )
    {
        LONG lWaypointDepth = _wtol( wszBuf );
        if ( lWaypointDepth < 1023 && lWaypointDepth >= 0 )
        {
            // waypoint between 0 and 1023, set it.
            //
            // We can ignore the error, chances are we'll be successful eventually.  Heck can this fail?
            (void)Param( pinst, JET_paramWaypointLatency )->Set( pinst, ppibNil, lWaypointDepth, NULL );
        }
    }

    //  this allows us to actually turn of checkpoint overscanning ... not recommended.
    //
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

    /*  write jet instance start event */

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

    /*  initialize system according to logging disabled
    /**/
    pinst->SetStatusRedo();

    if ( !plog->FLogDisabled() )
    {
        DBMS_PARAM  dbms_param;

        /*  initialize log manager, and check the last generation
        /*  of log files to determine if recovery needed.
        /*  (do not create the reserve logs -- this is done later during soft recovery)
        /**/
        CallJ( plog->ErrLGInit( &fNewCheckpointFile ), Uninitialize );
        fLGInitIsDone = fTrue;

        /*  store the system parameters
         */
        pinst->SaveDBMSParams( &dbms_param );

        pinst->m_isdlInit.Trigger( eInitBaseLogInitDone );

        /*  recover attached databases to consistent state
        /**/
        if ( grbit & ( JET_bitExternalRecoveryControl |
                        // Former bits that are controlled by our ErrRecoveryControlDefaultCallback
                        JET_bitLogStreamMustExist |
                        JET_bitRecoveryWithoutUndo |
                        JET_bitReplayIgnoreLostLogs |   // this one only partially handled by grbits callback.
                        // New bad behaviors, that we should try to groom out of the system.
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

        /*  add the first log record
        /**/
        CallJ( ErrLGStart( pinst ), TermLG );
    }
    pinst->m_isdlInit.Trigger( eInitLogInitDone );

    pinst->SetStatusRuntime();

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    /*  initialize remaining system
    /**/
    CallJ( pinst->ErrINSTInit(), TermLG );
    
    // If we haven't initialized snapshot yet, it is either due to conditions not being met or there was no recovery done. No harm in retrying.
    if ( !pinst->m_prbs )
    {
        // We will initialize the revert snapshot from Rstmap during LGRIInitSession.
        // Also, check if we need to roll the snapshot and roll it if required.
        CallJ( CRevertSnapshot::ErrRBSInitFromRstmap( pinst ), TermIT );

        // If required range was a problem or if db's are not on the required efv m_prbs would be null in ErrRBSInitFromRstmap
        if ( pinst->m_prbs && pinst->m_prbs->FRollSnapshot() )
        {
            CallJ( pinst->m_prbs->ErrRollSnapshot( fTrue, fTrue ), TermIT );
        }
    }

    Assert( !pinst->FRecovering() );
    Assert( !fJetLogGeneratedDuringSoftStart || !plog->FLogDisabled() );

    /*  set up FMP from checkpoint.
    /**/
    if ( !plog->FLogDisabled() )
    {
        WCHAR   wszPathJetChkLog[IFileSystemAPI::cchPathMax];

        plog->LGFullNameCheckpoint( wszPathJetChkLog );
        err = plog->ErrLGReadCheckpoint( wszPathJetChkLog, NULL, fTrue );
        if ( JET_errCheckpointFileNotFound == err
            && ( fNewCheckpointFile || fJetLogGeneratedDuringSoftStart ) )
        {
            //  could not locate checkpoint file, but we had previously
            //  deemed it necessary to create a new one (either because it
            //  was missing or because we're beginning from gen 1)
            plog->SetCheckpointEnabled( fTrue );
            (VOID) plog->ErrLGUpdateCheckpointFile( fTrue );

            if ( fJetLogGeneratedDuringSoftStart )
            {
                Assert( plog->LGGetCurrentFileGenNoLock() == plog->LgenInitial() );

                plog->LGVerifyFileHeaderOnInit();
            }

                //  must make sure we wipe out the JET_errCheckpointFileNotFound that is
                //  in err.
                err = JET_errSuccess;
        }
        else
        {
            CallJ( err, TermIT );
        }

        if ( err >= JET_errSuccess )
        {
            CallS( err );   // ensure we don't clobber a previously returned warning (should be none)
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
    // there may be databases still attached from recovery - even if this
    // JetInit call did not pass in JET_bitKeepAttachedAtEndOfRecovery, previous
    // calls may have done so and we replay those recovery-quit the same way
    // based on what flag was passed in at that point
    //
    // If no dbs are attached, the below code is basically a nop
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

    (VOID)ErrIOTerm( pinst, fFalse /* fNormal */ );

    if ( fLGInitIsDone )
    {
        (VOID)plog->ErrLGTerm( fFalse /* do not flush log */ );
    }

    if ( fJetLogGeneratedDuringSoftStart )
    {
        WCHAR   wszLogName[ IFileSystemAPI::cchPathMax ];

        //  Instead of using m_wszLogName (part of the shared-state monster),
        //  generate edb.jtx/log's filename now.
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

    //  select our termination mode

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

    //  term the instance

    err = pinst->ErrINSTTerm( termtype );

    if ( pinst->m_fSTInit != fSTInitNotDone )
    {
        /*  before getting an error before reaching no-returning point in ITTerm().
         */
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

    //  term the file-system

    delete pinst->m_pfsapi;
    pinst->m_pfsapi = NULL;

    IRSCleanUpAllIrsResources( pinst );

    pinst->m_isdlTerm.Trigger( eTermDone );

    const ULONG cbTimingResourceDataSequence = pinst->m_isdlTerm.CbSprintTimings();
    //  let's keep it rational, note we're at 6.8k right now, and while 8k might seem 
    //  like a lot, we're likely near the top of the stacks.
    Expected( cbTimingResourceDataSequence < 8192 );
    WCHAR * wszTimingResourceDataSequence = (WCHAR *)_alloca( cbTimingResourceDataSequence );
    pinst->m_isdlTerm.SprintTimings( wszTimingResourceDataSequence, cbTimingResourceDataSequence );

    /*  write jet stop event */
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

        ULONG rgul[2] = { (ULONG)pinst->m_iInstance, (ULONG)err /* for term dirty */ };
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

//+api
//  ErrIsamSetSessionParameter
//  =========================================================
//
//  Allows a client to set a parameter that has a session-only
//  scope.
//
//  PARAMETERS
//      ppib            pointer to PIB for user
//      sesparamid      the parameter "ID" that the client is trying to set
//      pvParam         the value to set in this parameter
//      cbParam         the size of the value of this parameter
//
//  RETURNS     JET_errSuccess | JET_errInvalidParameter
//
//-

ERR ErrIsamSetSessionParameter(
    _In_opt_ JET_SESID                          vsesid,
    _In_ ULONG                          sesparamid,
    _In_reads_bytes_opt_( cbParam ) void *      pvParam,
    _In_ ULONG                      cbParam )
{
    PIB     * ppib  = (PIB *)vsesid;

    //  Can do a common check for NULL pvParam, if the cbParam != 0 (essentially 
    //  saying we have a valid buffer).  The cbParam validity has to be checked
    //  on a per sesparamid basis below obviously.
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
        Expected( ( sesparamid >= JET_sesparamCommitDefault ) /* min value */ && ( sesparamid < ( JET_sesparamCommitDefault + 1024 ) ) );   // or they're passing a sysparam or dbparam?
        return ErrERRCheck( JET_errInvalidSesparamId );
    }

    return JET_errSuccess;
}

//+api
//  ErrIsamGetSessionParameter
//  =========================================================
//
//  Allows a client to retrieve a parameter from the specified
//  session.
//
//  PARAMETERS
//      ppib            pointer to PIB for user
//      sesparamid      the parameter "ID" that the client is trying to get
//      pvParam         the output buffer
//      cbParamMax      the max size that can be set in the output buffer
//
//  RETURNS     JET_errSuccess | JET_errInvalidParameter
//
//-

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
        // Should cbParamMax check for maybe just >= sizeof(JET_GRBIT)? probably fine.
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
        //  note: technically I didn't fix GetTransactionLevel() to refer to this, but in
        //  that API it's a ULONG_PTR / so 64-bits on 64-bit archs.  Someone should fix this
        //  if we ever update GetTransactionLevel() - though hoping more we can just deprecate
        //  that API.
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
        //  note: technically I didn't fix GetTransactionLevel() to refer to this, but in
        //  that API it's a ULONG_PTR / so 64-bits on 64-bit archs.  Someone should fix this
        //  if we ever update GetTransactionLevel() - though hoping more we can just deprecate
        //  that API.
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
        Expected( ( sesparamid >= JET_sesparamCommitDefault ) /* min value */ && ( sesparamid < ( JET_sesparamCommitDefault + 1024 ) ) );   // or they're passing a sysparam or dbparam?
        return ErrERRCheck( JET_errInvalidSesparamId );
    }

    return JET_errSuccess;
}


//+api
//  ErrIsamBeginTransaction
//  =========================================================
//  ERR ErrIsamBeginTransaction( PIB *ppib )
//
//  Starts a transaction for the current user.  The user's transaction
//  level increases by one.
//
//  PARAMETERS  ppib            pointer to PIB for user
//
//  RETURNS     JET_errSuccess
//
//  SIDE EFFECTS
//      The CSR stack for each active FUCB of this user is copied
//      to the new transaction level.
//
// SEE ALSO     ErrIsamCommitTransaction, ErrIsamRollback
//-
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
                    ( grbit & grbitsSupported ) );  //  filter out unsupported grbits
    }
    else
    {
        Assert( levelUserMost == ppib->Level() );
        err = ErrERRCheck( JET_errTransTooDeep );
    }

    return err;
}

extern JET_GRBIT g_grbitCommitFlagsMsk;

//+api
//  ErrIsamCommitTransaction
//  ========================================================
//  ERR ErrIsamCommitTransaction( JET_SESID vsesid, JET_GRBIT grbit, DWORD cmsecDurableCommit, JET_COMMIT_ID *pCommitId )
//
//  Commits the current transaction for this user.  The transaction level
//  for this user is decreased by the number of levels committed.
//
//  PARAMETERS
//
//  RETURNS     JET_errSuccess
//
//  SIDE EFFECTS
//      The CSR stack for each active FUCB of this user is copied
//      from the old ( higher ) transaction level to the new ( lower )
//      transaction level.
//
//  SEE ALSO    ErrIsamBeginTransaction, ErrIsamRollback
//-
ERR ISAMAPI ErrIsamCommitTransaction( JET_SESID vsesid, JET_GRBIT grbit, DWORD cmsecDurableCommit, JET_COMMIT_ID *pCommitId )
{
    ERR     err;
    PIB     *ppib = (PIB *)vsesid;
    FUCB    *pfucb = pfucbNil;
    
    CallR( ErrPIBCheck( ppib ) );

    //  validate grbits
    //
    if ( grbit & ~g_grbitCommitFlagsMsk )
    {
        return ErrERRCheck( JET_errInvalidGrbit );
    }

    //  may not be in a transaction, but force a new log to be created
    //
    if ( JET_bitForceNewLog & grbit )
    {
        //  no other grbits may be specified in conjunction with JET_bitForceNewLog
        if ( JET_bitForceNewLog != grbit )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        LGPOS lgpos;
        CallR( ErrLGForceLogRollover( ppib, __FUNCTION__, &lgpos ) );
        return ErrLGWaitForWrite( ppib, &lgpos );
    }

    //  may not be in a transaction, but wait for flush of all
    //  currently commited transactions.
    //
    if ( JET_bitWaitAllLevel0Commit & grbit )
    {
        //  no other grbits may be specified in conjunction with JET_bitWaitAllLevel0Commit
        if ( JET_bitWaitAllLevel0Commit != grbit )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        PERFOpt( cUserWaitAllTrxCommit.Inc( ppib->m_pinst ) );

        return ErrLGForceWriteLog( ppib );
    }

    //  may not be in a transaction, but wait for flush of previous
    //  lazy committed transactions.
    //
    if ( grbit & JET_bitWaitLastLevel0Commit )
    {
        //  If we are not at level 0 AND the user succeeds here with a wait last on 
        //  this session, we would still rollback this open trx if we crash ... this
        //  would be probably not what the client hoped for.
        Expected( ppib->Level() == 0 || !ppib->FBegin0Logged() );

        //  no other grbits may be specified in conjunction with WaitLastLevel0Commit
        if ( JET_bitWaitLastLevel0Commit != grbit )
        {
            return ErrERRCheck( JET_errInvalidGrbit );
        }

        //  wait for last level 0 commit and rely on good user behavior
        //
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

    //  reset uncommitted flags on any materialized sorts committed to level 0.
    //  Base tables are shared objects and have their bits reset when
    //  operCreateTable is committed to level 0.  Temporary tables are specific
    //  to a session and do not have operCreateTables and so are reset here.
    //
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


//+api
//  ErrIsamRollback
//  ========================================================
//  ERR ErrIsamRollback( PIB *ppib, JET_GRBIT grbit )
//
//  Rolls back transactions for the current user.  The transaction level of
//  the current user is decreased by the number of levels aborted.
//
//  PARAMETERS  ppib        pointer to PIB for user
//              grbit       unused
//
//  RETURNS
//      JET_errSuccess
//-
ERR ISAMAPI ErrIsamRollback( JET_SESID vsesid, JET_GRBIT grbit )
{
    ERR         err;
    PIB         * ppib          = (PIB *)vsesid;
    FUCB        * pfucb;
    FUCB        * pfucbNext;


    /*  check session id before using it
    /**/
    CallR( ErrPIBCheck( ppib ) );

    if ( ppib->Level() == 0 )
    {
        return ErrERRCheck( JET_errNotInTransaction );
    }

    do
    {
        const LEVEL levelRollback   = LEVEL( ppib->Level() - 1 );
        
        /*  get first primary index cusor
        /**/
        for ( pfucb = ppib->pfucbOfSession;
            pfucb != pfucbNil && ( FFUCBSecondary( pfucb ) || FFUCBLongValue( pfucb ) );
            pfucb = pfucb->pfucbNextOfSession )
            NULL;

        /*  LOOP 1 -- first go through all open cursors, and close them
        /*  or reset secondary index cursors, if opened in transaction
        /*  rolled back.  Reset copy buffer status and move before first.
        /*  Some cursors will be fully closed, if they have not performed any
        /*  updates.  This will include secondary index cursors
        /*  attached to primary index cursors, so pfucbNext must
        /*  always be a primary index cursor, to ensure that it will
        /*  be valid for the next loop iteration.  Note that no information
        /*  necessary for subsequent rollback processing is lost, since
        /*  the cursors will only be released if they have performed no
        /*  updates including DDL.
        /**/
        for ( ; pfucb != pfucbNil; pfucb = pfucbNext )
        {
            /*  get next primary index cusor
            /**/
            for ( pfucbNext = pfucb->pfucbNextOfSession;
                pfucbNext != pfucbNil && ( FFUCBSecondary( pfucbNext ) || FFUCBLongValue( pfucbNext ) );
                pfucbNext = pfucbNext->pfucbNextOfSession )
                NULL;

            /*  if defer closed then continue
            /**/
            if ( FFUCBDeferClosed( pfucb ) )
                continue;

            //  reset copy buffer status for each cursor on rollback
            if ( FFUCBUpdatePreparedLevel( pfucb, pfucb->ppib->Level() ) )
            {
                RECIFreeCopyBuffer( pfucb );
                FUCBResetUpdateFlags( pfucb );
            }

            Assert( 0 == pfucb->levelReuse || pfucb->levelReuse >= pfucb->levelOpen );

            /*  if current cursor is a table, and was opened in rolled back
            /*  transaction, then close cursor. Additionally, if this cursor was
            /*  reused at higher level than the rollback, we need to close it as
            /*  well as any opened cursor (opened or dupe'd) needs to be closed
            /*  when the transaction rolls back.
            /**/
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
                        //  Open internally, not exported to user.
                        CallS( ErrFILECloseTable( ppib, pfucb ) );
                    }
                    continue;
                }
                
                /*  if primary index cursor, and secondary index set
                /*  in rolled back transaction, then change index to primary
                /*  index.  This must be done, since secondary index
                /*  definition may be rolled back, if the index was created
                /*  in the rolled back transaction.
                /**/
                if ( pfucb->pfucbCurIndex != pfucbNil )
                {
                    if ( pfucb->pfucbCurIndex->levelOpen > levelRollback )
                    {
                        CallS( ErrRECSetCurrentIndex( pfucb, NULL, NULL ) );
                    }
                }
            }

            //  if LV cursor was opened at this level, close it
            if ( pfucbNil != pfucb->pfucbLV
                && ( pfucb->pfucbLV->levelOpen > levelRollback ||
                     pfucb->pfucbLV->levelReuse > levelRollback ) )
            {
                DIRClose( pfucb->pfucbLV );
                pfucb->pfucbLV = pfucbNil;
            }

            /*  if current cursor is a sort, and was opened in rolled back
            /*  transaction, then close cursor.
            /**/
            if ( FFUCBSort( pfucb ) )
            {
                if ( pfucb->levelOpen > levelRollback ||
                    pfucb->levelReuse > levelRollback )
                {
                    SORTClose( pfucb );
                    continue;
                }
            }

            /*  if not sort and not index, and was opened in rolled back
            /*  transaction, then close DIR cursor directly.
            /**/
            if ( pfucb->levelOpen > levelRollback ||
                pfucb->levelReuse > levelRollback )
            {
                DIRClose( pfucb );
                continue;
            }
        }

        /*  call lower level abort routine
        /**/
        err = ErrDIRRollback( ppib, grbit );
        if ( JET_errRollbackError == err )
        {
            err = ppib->ErrRollbackFailure();

            // recover the error from rollback here
            // and return that one
            Assert( err < JET_errSuccess );
        }
        CallR( err );
    }
    while ( ( grbit & JET_bitRollbackAll ) != 0 && ppib->Level() > 0 );

    //  reset must rollback if rolled back all the way to level 0
    //
    if ( ppib->Level() == 0 )
    {
        ppib->ResetMustRollbackToLevel0();
    }
    
    return JET_errSuccess;
}

