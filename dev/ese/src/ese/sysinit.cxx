// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include <ctype.h>
#include <io.h>


#if defined( DEBUG ) || defined( PERFDUMP )
BOOL    g_fDBGPerfOutput = fFalse;
#endif  /* DEBUG || PERFDUMP */


#ifdef DEBUG

WCHAR* GetDebugEnvValue( _In_ PCWSTR cwszEnvVar )
{
    WCHAR   wszBufTemp[ DEBUG_ENV_VALUE_LEN ];

    if ( FOSConfigGet( wszDEBUGRoot, cwszEnvVar, wszBufTemp, sizeof( wszBufTemp ) ) )
    {
        if ( wszBufTemp[0] )
        {
            // UNDONE  we don't really want to deal with an OutOfMemory
            // error at this point. Anyway it is debug only.

            const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
            ULONG cchBuf = LOSStrLengthW( wszBufTemp ) + 1;
            WCHAR* wszBuf = new WCHAR[ cchBuf ];
            RFSThreadReEnable( cRFSCountdownOld );

            //  if we really are OutOfMemory we return NULL as the EnvVariable is not set
            //  we do the same also if the one set in Env too long so ...
            //  and probably we exit soon anyway as we are OutOfMemory
            if ( wszBuf )
            {
                OSStrCbCopyW( wszBuf, sizeof(WCHAR)*cchBuf, wszBufTemp );
            }
            return wszBuf;
        }
        else
        {
//          FOSConfigGet() will create the key if it doesn't exist
        }
    }
    else
    {
        //  UNDONE:  gripe in the event log that the value was too big
    }

    return NULL;
}


static const WCHAR * g_rgcwszDebugOptionEnvVar[] =
{
    L"Invalid",
    L"DEBUG_PAGE_SHAKE",
    L"DEBUG_NODE_SEEK",
    L"DEBUG_NODE_INSERT",
    L"DEBUG_BT_SPLIT",
    L"DEBUG_VER_ASSERTVALID",
};

C_ASSERT( _countof( g_rgcwszDebugOptionEnvVar ) == Debug_Max );

static bool g_rgfDebugOptionEnabled[Debug_Max];

//  ================================================================
bool FExpensiveDebugCodeEnabled( __in_range( Debug_Min, Debug_Max - 1 ) const ExpensiveDebugCode code )
//  ================================================================
{
    return g_rgfDebugOptionEnabled[code];
}

//  ================================================================
static void ITDBGGetExpensiveDebugOptions()
//  ================================================================
{
    for( ExpensiveDebugCode code = Debug_Min; code < Debug_Max; code = (ExpensiveDebugCode)(code + 1) )
    {
        const WCHAR * const cwszValue = GetDebugEnvValue( g_rgcwszDebugOptionEnvVar[code] );
        g_rgfDebugOptionEnabled[code] = ( NULL != cwszValue) &&  ( 0 != _wtol( cwszValue ) );
        delete[] cwszValue;
    }
}

VOID ITDBGSetConstants( INST * pinst )
{
    WCHAR * wsz;

    if ( ( wsz = GetDebugEnvValue( L"Redo Trap" ) ) != NULL )
    {
        ULONG lGeneration;
        ULONG isec;
        ULONG ib;
        swscanf_s( wsz,
                L"%06lX,%04lX,%04lX",
                &lGeneration,
                &isec,
                &ib );

        extern LGPOS g_lgposRedoTrap;
        g_lgposRedoTrap.lGeneration = lGeneration;
        g_lgposRedoTrap.isec        = USHORT( isec );
        g_lgposRedoTrap.ib          = USHORT( ib );

        delete[] wsz;
    }

#ifdef PROFILE_JET_API
    if ( ( wsz = GetDebugEnvValue ( L"JETProfileName" ) ) != NULL )
    {
        extern WCHAR profile_wszFileName[];
        OSStrCbCopyW( profile_szFileName,  IFileSystemAPI::cchPathMax * sizeof(WCHAR), wsz );
        profile_szFileName[IFileSystemAPI::cchPathMax-1] = 0;
        delete[] wsz;
    }
    if ( ( wsz = GetDebugEnvValue ( L"JETProfileOptions" ) ) != NULL )
    {
        extern INT profile_detailLevel;
        profile_detailLevel = _wtoi( wsz );
        delete[] wsz;
    }
#endif // PROFILE_JET_API

    if ( ( wsz = GetDebugEnvValue ( L"PERFOUTPUT" ) ) != NULL )
    {
        g_fDBGPerfOutput = fTrue;
        delete[] wsz;
    }
    else
    {
        g_fDBGPerfOutput = fFalse;
    }

    ITDBGGetExpensiveDebugOptions();
}
#endif  //  DEBUG


ERR ErrITSetConstants( INST * pinst )
{
#ifdef RTM
    //  Redo Trap is disabled in RTM builds
#else

#ifdef DEBUG
    ITDBGSetConstants( pinst );
#endif  //  DEBUG

    WCHAR       wszBufRedoTrap[ 30 /* more than enough for worst case L"%06X,%04X,%04X" */  ];

    if ( FOSConfigGet( wszDEBUGRoot, L"Redo Trap", wszBufRedoTrap, sizeof(wszBufRedoTrap) )
        && 0 != wszBufRedoTrap[0] )
    {
        ULONG lGeneration;
        ULONG isec;
        ULONG ib;
        swscanf_s(  wszBufRedoTrap,
                L"%06X,%04X,%04X",
                &lGeneration,
                &isec,
                &ib );

        extern LGPOS g_lgposRedoTrap;
        g_lgposRedoTrap.lGeneration = lGeneration;
        g_lgposRedoTrap.isec        = USHORT( isec );
        g_lgposRedoTrap.ib          = USHORT( ib );
    }

#endif  //  RTM

    FixDefaultSystemParameters();

#ifndef DEBUG
    extern BOOL g_fNodeMiscMemoryTrashedDefenseInDepthTemp;
    (void)ErrLoadDiagOption( pinst, g_fNodeMiscMemoryTrashedDefenseInDepthTemp );
#endif

    // Since production builds will be released regularly, the default
    // will be OFF in retail builds.
#ifdef DEBUG
    const BOOL fPeriodicTrimEnabledDefault = fTrue;
#else
    const BOOL fPeriodicTrimEnabledDefault = fFalse;
#endif

    g_fPeriodicTrimEnabled = (BOOL) UlConfigOverrideInjection( 35632, fPeriodicTrimEnabledDefault );

    return JET_errSuccess;
}


//+API
//  ErrINSTInit
//  ========================================================
//  ERR ErrINSTInit( )
//
//  Initialize the storage system: page buffers, log buffers, and the
//  database files.
//
//  RETURNS     JET_errSuccess
//-
ERR INST::ErrINSTInit( )
{
    ERR     err;
    BOOL    fFCBInitialized = fFalse;
    VER     *pver           = m_pver;

    //  sleep while initialization is in progress
    //
    while ( m_fSTInit == fSTInitInProgress )
    {
        UtilSleep( 1000 );
    }

    //  serialize system initialization
    //
    if ( m_fSTInit == fSTInitDone )
    {
        return JET_errSuccess;
    }

    //  initialization in progress
    //
    m_fSTInit = fSTInitInProgress;

    //  initialize Global variables
    //
    Assert( m_ppibGlobal == ppibNil );

    // Set to FALSE (may have gotten set to TRUE by recovery).
    m_fTermAbruptly = fFalse;

    //  initialize subcomponents
    //
    Call( ErrIOInit( this ) );

    CallJ( ErrPIBInit( this ), TermIO )

    // initialize FCB, TDB, and IDB.

    CallJ( FCB::ErrFCBInit( this ), TermPIB );
    fFCBInitialized = fTrue;

    // Initialize PRL. (Must be done prior to creating the temp DB,
    // since AutoHealing might be enabled.)
    CallJ( PagePatching::ErrPRLInit( this ), TermPIB );

    // initialize SCB

    CallJ( ErrSCBInit( this ), TermPIB );

    CallJ( ErrFUCBInit( this ), TermSCB );

    if ( !FRecovering() )
    {
        // bit of trickyness in here, we can call this during recovery ... maybe some
        // day I will do something to handle and track recovery ErrINSTInit/ErrINSTTerm
        // time, but today is not that day ... as a result this time will be swallowed
        // by recovery redo / recovery undo.
        m_isdlInit.Trigger( eInitSubManagersDone );
    }

    //  begin backup session
    //
    CallJ( m_pbackup->ErrBKBackupPibInit(), TermFUCB );

    //  init task manager
    //
    CallJ( m_taskmgr.ErrTMInit(), CloseBackupPIB );

    //  intialize version store

    CallJ( pver->ErrVERInit( this ), TermTM );

    // initialize LV critical section

    CallJ( ErrLVInit( this ), TermVER );

    // If revert snapshot cleaner is not initialized and RBS file path was set, initialize them.
    if ( m_prbscleaner == NULL )
    {
        CallJ( RBSCleanerFactory::ErrRBSCleanerCreate( this, &m_prbscleaner ), TermRBS );
    }

    this->m_fSTInit = fSTInitDone;

    if ( !FRecovering() )
    {
        m_isdlInit.Trigger( eInitBigComponentsDone );
    }

    return JET_errSuccess;

TermRBS:
    if ( m_prbscleaner )
    {
        delete m_prbscleaner;
        m_prbscleaner = NULL;
    }

    LVTerm( this );

TermVER:
    m_pver->m_fSyncronousTasks = fTrue; // avoid an assert in VERTerm()
    m_pver->VERTerm( fFalse );  //  not normal

TermTM:
    m_taskmgr.TMTerm();

CloseBackupPIB:
    //  terminate backup session
    //
    m_pbackup->BKBackupPibTerm();

TermFUCB:
    FUCBTerm( this );

TermSCB:
    SCBTerm( this );

TermPIB:
    PIBTerm( this );

TermIO:
    (VOID)ErrIOTerm( this, fFalse );    //  not normal

    //  must defer FCB temination to the end because IOTerm() will clean up FCB's
    //  allocated for the temp. database
    if ( fFCBInitialized )
    {
        FCB::Term( this );
    }

HandleError:
    this->m_fSTInit = fSTInitNotDone;
    return err;
}

ERR INST::ErrINSTCreateTempDatabase_( void* const pvInst )
{
    IFMP        ifmp                = g_ifmpMax;
    PIB*        ppib                = ppibNil;
    ERR         err                 = JET_errSuccess;

    Assert( !g_fRepair );
    Assert( pvInst );
    INST* const pInst = static_cast<INST* const>( pvInst );
    const JET_GRBIT grbit = JET_bitDbRecoveryOff
                            | JET_bitDbShadowingOff
                            | ( BoolParam( pInst, JET_paramEnableTempTableVersioning ) ? 0 : JET_bitDbVersioningOff );

    //  This function is the only placer to create temp DB, and it's exected only once,
    //  the temp DB should not be created at this point.
    Assert( pInst->m_mpdbidifmp[ dbidTemp ] >= g_ifmpMax );

    OPERATION_CONTEXT opContext = { OCUSER_INTERNAL, 0, 0, 0, 0 };
    Call( ErrPIBBeginSession( pInst, &ppib, procidNil, fTrue ) );
    Call( ppib->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );

    //  Open and set size of temp database
    Call( ErrFaultInjection( 41776 ) );
    Call( ErrDBCreateDatabase(
                ppib,
                NULL,
                SzParam( pInst, JET_paramTempPath ),
                &ifmp,
                dbidTemp,
                (CPG)max( cpgMultipleExtentMin, UlParam( pInst, JET_paramPageTempDBMin ) ),
                fFalse, // fSparseEnabledFile
                NULL,
                NULL,
                0,
                grbit ) );
    Assert( ifmp != g_ifmpMax );
    Assert( FFMPIsTempDB( ifmp ) );
    
    //  Temp db flag going from false to true.
    Assert( fFalse == pInst->m_fTempDBCreated );

    //  pInst->m_fTempDBCreated = fTrue;
    AtomicExchange( (LONG *)&(pInst->m_fTempDBCreated), fTrue );
    
HandleError:
    if ( ppibNil != ppib )
    {
        PIBEndSession( ppib );
    }

    return err;
}

ERR INST::ErrINSTCreateTempDatabase()
{
    ERR err = JET_errSuccess;

    //  Only attempt to create temp database if temp tables greater than 0.  Ignore whether
    //  or not we are not recovering
    //
    if ( UlParam( this, JET_paramMaxTemporaryTables ) <= 0 )
    {
        return JET_errSuccess;
    }

    //  performance optimization for already created case
    //
    if ( m_fTempDBCreated )
    {
        return JET_errSuccess;
    }
    
    //  enter critical section and test condition
    //
    m_critTempDbCreate.FEnter( cmsecInfinite );
    if ( !m_fTempDBCreated )
    {
        Call( ErrINSTCreateTempDatabase_( this ) );
    }
    Assert( m_fTempDBCreated );

HandleError:
    //  leave critical section 
    //
    m_critTempDbCreate.Leave();

    if ( JET_errSuccess != err )
    {
        WCHAR wszErrorCode[32];
        const WCHAR *  rgcwszT[2];
        rgcwszT[0] = SzParam( this, JET_paramTempPath );
        rgcwszT[1] = wszErrorCode;
        OSStrCbFormatW( wszErrorCode, sizeof(wszErrorCode), L"%d", err );
        UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            CREATING_TEMP_DB_FAILED_ID,
            sizeof(rgcwszT)/sizeof(WCHAR *),
            rgcwszT,
            0,
            this );
    }
    
    return err;
}

//+api------------------------------------------------------
//
//  ErrINSTTerm
//  ========================================================
//
//  ERR ErrITTerm( VOID )
//
//  Flush the page buffers to disk so that database file be in
//  consistent state.  If error in RCCleanUp or in BFFlush, then DO NOT
//  terminate log, thereby forcing redo on next initialization.
//
//----------------------------------------------------------

ERR INST::ErrINSTTerm( TERMTYPE termtype )
{
    ERR         err;
    ERR         errRet = JET_errSuccess;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(this->m_iInstance), sizeof(this->m_iInstance) );


    Assert( m_plog->FRecovering() || m_fTermInProgress || termtypeError == termtype );

    //  Normally we're in here for JetTerm(), but we can also end up in here if JetInit
    //  fails out at the right point.

    Assert( m_isdlTerm.FActiveSequence() || m_isdlInit.FActiveSequence() );

    //  sleep while initialization is in progress
    //
    while ( m_fSTInit == fSTInitInProgress )
    {
        UtilSleep( 1000 );
    }

    //  make sure no other transactions in progress
    //
    //  if write error on page, RCCleanup will return -err
    //  if write error on buffer, BFFlush will return -err
    //  -err passed to LGTerm will cause correctness flag to
    //  be omitted from log thereby forcing recovery on next
    //  startup, and causing LGTerm to return +err, which
    //  may be used by JetQuit to show error
    //
    if ( m_fSTInit == fSTInitNotDone )
    {
        return ErrERRCheck( JET_errNotInitialized );
    }

    // If no error (termtype != termError), then need to check if we can continue.

    if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && m_pbackup->FBKBackupInProgress() )
    {
        if ( m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) )
        {
            return ErrERRCheck( JET_errBackupInProgress );
        }
        else
        {
            return ErrERRCheck( JET_errSurrogateBackupInProgress );
        }
    }

    if ( g_fPeriodicTrimEnabled )
    {
        SPTrimDBTaskStop( this );
    }

    //  we should set DBM so it can start again in the
    //  case where we're recovering. When recovering,
    //  DBScan may be trigged if JET_paramEnableDBScanInRecovery is
    //  specified. Additionally, in the recovery case an explicit
    //  API call to trigger DBM is not possible, so we need not worry
    //  with concurrency below.
    DBMScanStopAllScansForInst( this, m_plog->FRecovering() );
    OLDTermInst( this );

    //  force the version store into synchronous-cleanup mode
    //  (e.g. circumvent TASKMGR because it is about to go away)
    m_pver->m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
    m_pver->m_fSyncronousTasks = fTrue;
    m_pver->m_critRCEClean.Leave();

    //  Cleanup all the tasks. Tasks will not be accepted by the TASKMGR
    //  until it is re-inited
    //  OLD2 may still issue tasks, which can be ignored safely
    m_taskmgr.TMTerm();

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermWaitedForSystemWorkerThreads );
    }
    
    //  clean up all entries
    //
    err = m_pver->ErrVERRCEClean();

    if ( termtype == termtypeCleanUp )
    {
        if ( err == JET_wrnRemainingVersions )
        {
            UtilSleep( 3000 );
            err = m_pver->ErrVERRCEClean();
        }
        if ( err < JET_errSuccess )
        {
            termtype = termtypeError;
            if ( errRet >= JET_errSuccess )
            {
                errRet = err;
            }
        }
        else
        {
            // allow for improper usage in test (e.g.: terminating the instance
            // with an outstanding transaction).
            if ( !FNegTest( fInvalidUsage ) )
            {
                FCBAssertAllClean( this );
            }
        }
    }
    else
    {
        Expected( ( err == JET_errSuccess ) || ( err == JET_wrnRemainingVersions ) || ( err == JET_errOutOfMemory ) || ( err == JET_errInstanceUnavailable ) );
        if ( err < JET_errSuccess )
        {
            termtype = termtypeError;
        }
    }

    if ( ( err != JET_errSuccess ) && FRFSAnyFailureDetected() )
    {
        RFSSetKnownResourceLeak();
    }

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermVersionStoreDone );
    }

    //  once the version store has stopped we can terminate OLD2 as no new tables
    //  will be registered for OLD2 beyond this point.
    OLD2TermInst( this );

    //  fail if there are still active transactions and we are doing a
    //  clean shutdown

    if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && TrxOldest( this ) != trxMax )
    {
        err = m_taskmgr.ErrTMInit();
        Assert( JET_errSuccess == err );

        m_pver->m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
        m_pver->m_fSyncronousTasks = fFalse;
        m_pver->m_critRCEClean.Leave();

        return ErrERRCheck( JET_errTooManyActiveUsers );
    }

    //  No new threads can come in and find corrupt pages (generating new patch requests)
    //  so we can now cancel outstanding patch requests.
    PagePatching::TermInst( this );

    //  Enter no-returning point. Once we kill one thread, we kill them all !!!!
    //
    m_fSTInit = fSTInitNotDone;

    m_fTermAbruptly = ( termtype == termtypeNoCleanUp || termtype == termtypeError );

    //  terminate MSysLocales KVP-Store(s) as we can no longer create / delete indices ...
    //
    // note: must be term'd before PIBTerm()/FUCBTerm() below because it is using them.
    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP * const pfmp = &g_rgfmp[ ifmp ];

        CATTermMSLocales( pfmp );
        Assert( NULL == pfmp->PkvpsMSysLocales() );

        CATTermMSDeferredPopulateKeys( pfmp );
        Assert( NULL == pfmp->PkvpsMSysDeferredPopulateKeys() );
    }

    //  terminate backup session
    //
    m_pbackup->BKBackupPibTerm();

    //  close LV critical section and remove session
    //  do not try to insert long values after calling this
    //
    LVTerm( this );

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermRestNoReturning );
    }


    //  update gen required / waypoint for term
    //
    if ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp )
    {
        // If we have some dirty-keep-cache-alive databases which were never explicitly attached,
        // allow header updates now so they can be marked clean and we can terminate the instance.
        for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmpT = m_mpdbidifmp[ dbid ];
            if ( ifmpT >= g_ifmpMax )
                continue;

            FMP * const pfmp = &g_rgfmp[ ifmpT ];
            if ( pfmp->FAttachedForRecovery() && !pfmp->FAllowHeaderUpdate() )
            {
                pfmp->RwlDetaching().EnterAsWriter();
                pfmp->SetAllowHeaderUpdate();
                pfmp->RwlDetaching().LeaveAsWriter();
            }
        }

        err = m_plog->ErrLGUpdateCheckpointFile( fTrue );
        if ( err < JET_errSuccess )
        {
            if ( m_plog->FRecovering() )
            {
                //  disable log writing
                m_plog->SetNoMoreLogWrite( err );
            }
            termtype = termtypeError;
            m_fTermAbruptly = fTrue;
            if ( errRet >= JET_errSuccess )
            {
                errRet = err;
            }
        }
    }

    //  flush and purge all buffers
    //
    if ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp )
    {
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            const IFMP  ifmp    = m_mpdbidifmp[ dbid ];

            if ( ifmp < g_ifmpMax )
            {
                err = ErrBFFlush( ifmp );
                if ( err < JET_errSuccess )
                {
                    termtype = termtypeError;
                    m_fTermAbruptly = fTrue;
                    if ( errRet >= JET_errSuccess )
                    {
                        errRet = err;
                    }
                }
                Assert( ( termtype != termtypeCleanUp && termtype != termtypeNoCleanUp ) ||
                            g_rgfmp[ifmp].Pfapi() == NULL ||
                            g_rgfmp[ifmp].Pfapi()->CioNonFlushed() <= cioAllowLogRollHeaderUpdates );
            }
        }
    }
    Assert( termtype == termtypeError || termtype == termtypeRecoveryQuitKeepAttachments || FIOCheckUserDbNonFlushedIos( this, cioAllowLogRollHeaderUpdates ) );

    // Verify that all snapshot data is flushed by buffer flush above.
    if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && m_prbs && m_prbs->FInitialized() )
    {
        m_prbs->AssertAllFlushed();
    }

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermBufferManagerFlushDone );
    }

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(m_iInstance), sizeof(m_iInstance) );

    //  before we term BF, we disable the checkpoint if error occurred
    //
    if ( errRet < JET_errSuccess || termtype == termtypeError )
    {
        m_plog->LGDisableCheckpoint();
    }

    // only try to retain cache when ABSOLUTELY nothing has gone wrong and
    // not using file caching.
    if ( termtype != termtypeRecoveryQuitKeepAttachments &&
         ( termtype == termtypeError || errRet < JET_errSuccess || BoolParam( JET_paramEnableViewCache ) ) )
    {
        for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
        {
            const IFMP  ifmp    = m_mpdbidifmp[ dbid ];

            if ( ifmp < g_ifmpMax )
            {
                BFPurge( ifmp );
            }
        }
    }

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermBufferManagerPurgeDone );
    }

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(m_iInstance), sizeof(m_iInstance) );
    
    // terminate the version store only after buffer manager as there are links to undo info RCE's
    // that will point to freed memory if we end the version store before the BF
    m_pver->VERTerm( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp );

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermVersionStoreDoneAgain );
    }

    //  reset initialization flag
    
    FCB::PurgeAllDatabases( this );

    // Delete any system pib list-entries allocated (and reset linked list)

    Assert( 0 == m_cUsedSystemPibs );
    ListNodePPIB *plnppib = m_plnppibEnd->pNext;
    while ( plnppib != m_plnppibEnd )
    {
        ListNodePPIB *plnppibNext = plnppib->pNext;
        delete plnppib;
        plnppib = plnppibNext;
    }
#ifdef DEBUG
    m_plnppibEnd->ppib = NULL;
#endif // DEBUG
    m_plnppibEnd->pNext = m_plnppibEnd;
    m_plnppibBegin = m_plnppibEnd;

    // PIBTerm also acts as a form of Sort Purge, similar to FCB::PurgeAllDatabases().

    PIBTerm( this );

    // We always purge the temporary database. This is important because the temporary 
    // database isn't flushed so it will still have dirty pages, and thus is not eligible
    // for retain cache feature.
    // Move the purge of temporary database below PIBTerm because, if there is any sort table left
    // open, PIBTerm will call ErrIsamSortClose which needs to read b-tree pages of sort table.

    const IFMP ifmp = m_mpdbidifmp[dbidTemp];
    if( ifmp < g_ifmpMax )
    {
        Assert( FFMPIsTempDB( ifmp ) );
        BFPurge( ifmp );
    }

    FUCBTerm( this );
    SCBTerm( this );
    
    //  clean up the fmp entries

    if ( termtype == termtypeRecoveryQuitKeepAttachments )
    {
        // Do not allow any header update until either re-attached either by recovery or explicitly
        for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmpT = m_mpdbidifmp[ dbid ];
            if ( ifmpT >= g_ifmpMax )
                continue;

            FMP * const pfmp = &g_rgfmp[ ifmpT ];
            pfmp->RwlDetaching().EnterAsWriter();
            pfmp->ResetAllowHeaderUpdate();
            pfmp->RwlDetaching().LeaveAsWriter();
        }
    }
    else
    {
        err = ErrIOTerm( this, termtype == termtypeCleanUp || termtype == termtypeNoCleanUp );
        if ( err < JET_errSuccess )
        {
            termtype = termtypeError;
            m_fTermAbruptly = fTrue;
            if ( errRet >= JET_errSuccess )
            {
                errRet = err;
            }
        }
    }

    //  Mostly pointless as all DBs are detached - EXCEPT if termtype == termtypeRecoveryQuitKeepAttachments  (i.e. RW keep cache alive) ...
    Assert( termtype == termtypeError || termtype == termtypeRecoveryQuitKeepAttachments || FIOCheckUserDbNonFlushedIos( this ) );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(m_iInstance), sizeof(m_iInstance) );

    //  terminate FCB

    FCB::Term( this );

    //  delete temp file. Temp file should be cleaned up in IOTerm.
    //
    //  NOTE: the temp db is now created with FILE_FLAG_DELETE_ON_CLOSE,
    //  so it should no longer be necessary to manually delete it anymore

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermCleanupAndResetting );
    }

    return errRet;
}

