// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include <ctype.h>
#include <io.h>


#if defined( DEBUG ) || defined( PERFDUMP )
BOOL    g_fDBGPerfOutput = fFalse;
#endif  


#ifdef DEBUG

WCHAR* GetDebugEnvValue( __in PCWSTR cwszEnvVar )
{
    WCHAR   wszBufTemp[ DEBUG_ENV_VALUE_LEN ];

    if ( FOSConfigGet( wszDEBUGRoot, cwszEnvVar, wszBufTemp, sizeof( wszBufTemp ) ) )
    {
        if ( wszBufTemp[0] )
        {

            const LONG cRFSCountdownOld = RFSThreadDisable( 0 );
            ULONG cchBuf = LOSStrLengthW( wszBufTemp ) + 1;
            WCHAR* wszBuf = new WCHAR[ cchBuf ];
            RFSThreadReEnable( cRFSCountdownOld );

            if ( wszBuf )
            {
                OSStrCbCopyW( wszBuf, sizeof(WCHAR)*cchBuf, wszBufTemp );
            }
            return wszBuf;
        }
        else
        {
        }
    }
    else
    {
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

bool FExpensiveDebugCodeEnabled( __in_range( Debug_Min, Debug_Max - 1 ) const ExpensiveDebugCode code )
{
    return g_rgfDebugOptionEnabled[code];
}

static void ITDBGGetExpensiveDebugOptions()
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
#endif

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
#endif


ERR ErrITSetConstants( INST * pinst )
{
#ifdef RTM
#else

#ifdef DEBUG
    ITDBGSetConstants( pinst );
#endif

    WCHAR       wszBufRedoTrap[ 30   ];

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

#endif

    FixDefaultSystemParameters();

#ifndef DEBUG
    extern BOOL g_fNodeMiscMemoryTrashedDefenseInDepthTemp;
    (void)ErrLoadDiagOption( pinst, g_fNodeMiscMemoryTrashedDefenseInDepthTemp );
#endif

#ifdef DEBUG
    const BOOL fPeriodicTrimEnabledDefault = fTrue;
#else
    const BOOL fPeriodicTrimEnabledDefault = fFalse;
#endif

    g_fPeriodicTrimEnabled = (BOOL) UlConfigOverrideInjection( 35632, fPeriodicTrimEnabledDefault );

    return JET_errSuccess;
}


ERR INST::ErrINSTInit( )
{
    ERR     err;
    BOOL    fFCBInitialized = fFalse;
    VER     *pver           = m_pver;

    while ( m_fSTInit == fSTInitInProgress )
    {
        UtilSleep( 1000 );
    }

    if ( m_fSTInit == fSTInitDone )
    {
        return JET_errSuccess;
    }

    m_fSTInit = fSTInitInProgress;

    Assert( m_ppibGlobal == ppibNil );

    m_fTermAbruptly = fFalse;

    Call( ErrIOInit( this ) );

    CallJ( ErrPIBInit( this ), TermIO )


    CallJ( FCB::ErrFCBInit( this ), TermPIB );
    fFCBInitialized = fTrue;

    CallJ( PagePatching::ErrPRLInit( this ), TermPIB );


    CallJ( ErrSCBInit( this ), TermPIB );

    CallJ( ErrFUCBInit( this ), TermSCB );

    if ( !FRecovering() )
    {
        m_isdlInit.Trigger( eInitSubManagersDone );
    }

    CallJ( m_pbackup->ErrBKBackupPibInit(), TermFUCB );

    CallJ( m_taskmgr.ErrTMInit(), CloseBackupPIB );


    CallJ( pver->ErrVERInit( this ), TermTM );


    CallJ( ErrLVInit( this ), TermVER );

    if ( m_prbscleaner == NULL )
    {
        CallJ( RBSCleanerFactory::ErrRBSCleanerCreate( this, &m_prbscleaner ), TermRBS );
        CallJ( m_prbscleaner->ErrStartCleaner(), TermRBS );
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
    m_pver->m_fSyncronousTasks = fTrue;
    m_pver->VERTerm( fFalse );

TermTM:
    m_taskmgr.TMTerm();

CloseBackupPIB:
    m_pbackup->BKBackupPibTerm();

TermFUCB:
    FUCBTerm( this );

TermSCB:
    SCBTerm( this );

TermPIB:
    PIBTerm( this );

TermIO:
    (VOID)ErrIOTerm( this, fFalse );

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

    Assert( pInst->m_mpdbidifmp[ dbidTemp ] >= g_ifmpMax );

    OPERATION_CONTEXT opContext = { OCUSER_INTERNAL, 0, 0, 0, 0 };
    Call( ErrPIBBeginSession( pInst, &ppib, procidNil, fTrue ) );
    Call( ppib->ErrSetOperationContext( &opContext, sizeof( opContext ) ) );

    Call( ErrFaultInjection( 41776 ) );
    Call( ErrDBCreateDatabase(
                ppib,
                NULL,
                SzParam( pInst, JET_paramTempPath ),
                &ifmp,
                dbidTemp,
                (CPG)max( cpgMultipleExtentMin, UlParam( pInst, JET_paramPageTempDBMin ) ),
                fFalse,
                NULL,
                NULL,
                0,
                grbit ) );
    Assert( ifmp != g_ifmpMax );
    Assert( FFMPIsTempDB( ifmp ) );
    
    Assert( fFalse == pInst->m_fTempDBCreated );

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

    if ( UlParam( this, JET_paramMaxTemporaryTables ) <= 0 )
    {
        return JET_errSuccess;
    }

    if ( m_fTempDBCreated )
    {
        return JET_errSuccess;
    }
    
    m_critTempDbCreate.FEnter( cmsecInfinite );
    if ( !m_fTempDBCreated )
    {
        Call( ErrINSTCreateTempDatabase_( this ) );
    }
    Assert( m_fTempDBCreated );

HandleError:
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


ERR INST::ErrINSTTerm( TERMTYPE termtype )
{
    ERR         err;
    ERR         errRet = JET_errSuccess;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(this->m_iInstance), sizeof(this->m_iInstance) );


    Assert( m_plog->FRecovering() || m_fTermInProgress || termtypeError == termtype );


    Assert( m_isdlTerm.FActiveSequence() || m_isdlInit.FActiveSequence() );

    while ( m_fSTInit == fSTInitInProgress )
    {
        UtilSleep( 1000 );
    }

    if ( m_fSTInit == fSTInitNotDone )
    {
        return ErrERRCheck( JET_errNotInitialized );
    }


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

    DBMScanStopAllScansForInst( this, m_plog->FRecovering() );
    OLDTermInst( this );

    m_pver->m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
    m_pver->m_fSyncronousTasks = fTrue;
    m_pver->m_critRCEClean.Leave();

    m_taskmgr.TMTerm();

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermWaitedForSystemWorkerThreads );
    }
    
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

    OLD2TermInst( this );


    if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && TrxOldest( this ) != trxMax )
    {
        err = m_taskmgr.ErrTMInit();
        Assert( JET_errSuccess == err );

        m_pver->m_critRCEClean.FEnter( cmsecInfiniteNoDeadlock );
        m_pver->m_fSyncronousTasks = fFalse;
        m_pver->m_critRCEClean.Leave();

        return ErrERRCheck( JET_errTooManyActiveUsers );
    }

    PagePatching::TermInst( this );

    m_fSTInit = fSTInitNotDone;

    m_fTermAbruptly = ( termtype == termtypeNoCleanUp || termtype == termtypeError );

    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        FMP * const pfmp = &g_rgfmp[ ifmp ];

        CATTermMSLocales( pfmp );
        Assert( NULL == pfmp->PkvpsMSysLocales() );
    }

    m_pbackup->BKBackupPibTerm();

    LVTerm( this );

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermRestNoReturning );
    }


    if ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp )
    {
        for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmpT = m_mpdbidifmp[ dbid ];
            if ( ifmpT >= g_ifmpMax )
                continue;

            if ( BoolParam( this, JET_paramFlight_EnableReattachRaceBugFix ) )
            {
                FMP * const pfmp = &g_rgfmp[ ifmpT ];
                if ( pfmp->FAttachedForRecovery() && !pfmp->FAllowHeaderUpdate() )
                {
                    pfmp->RwlDetaching().EnterAsWriter();
                    pfmp->SetAllowHeaderUpdate();
                    pfmp->RwlDetaching().LeaveAsWriter();
                }
            }
        }

        err = m_plog->ErrLGUpdateCheckpointFile( fTrue );
        if ( err < JET_errSuccess )
        {
            if ( m_plog->FRecovering() )
            {
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

    if ( ( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp ) && m_prbs && m_prbs->FInitialized() )
    {
        m_prbs->AssertAllFlushed();
    }

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermBufferManagerFlushDone );
    }

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(m_iInstance), sizeof(m_iInstance) );

    if ( errRet < JET_errSuccess || termtype == termtypeError )
    {
        m_plog->LGDisableCheckpoint();
    }

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
    
    m_pver->VERTerm( termtype == termtypeCleanUp || termtype == termtypeNoCleanUp );

    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermVersionStoreDoneAgain );
    }

    
    FCB::PurgeAllDatabases( this );


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
#endif
    m_plnppibEnd->pNext = m_plnppibEnd;
    m_plnppibBegin = m_plnppibEnd;


    PIBTerm( this );


    const IFMP ifmp = m_mpdbidifmp[dbidTemp];
    if( ifmp < g_ifmpMax )
    {
        Assert( FFMPIsTempDB( ifmp ) );
        BFPurge( ifmp );
    }

    FUCBTerm( this );
    SCBTerm( this );
    

    if ( termtype == termtypeRecoveryQuitKeepAttachments )
    {
        for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
        {
            const IFMP ifmpT = m_mpdbidifmp[ dbid ];
            if ( ifmpT >= g_ifmpMax )
                continue;

            if ( BoolParam( this, JET_paramFlight_EnableReattachRaceBugFix ) )
            {
                FMP * const pfmp = &g_rgfmp[ ifmpT ];
                pfmp->RwlDetaching().EnterAsWriter();
                pfmp->ResetAllowHeaderUpdate();
                pfmp->RwlDetaching().LeaveAsWriter();
            }
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

    Assert( termtype == termtypeError || termtype == termtypeRecoveryQuitKeepAttachments || FIOCheckUserDbNonFlushedIos( this ) );

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, this, (PVOID)&(m_iInstance), sizeof(m_iInstance) );


    FCB::Term( this );


    if ( m_isdlTerm.FActiveSequence() )
    {
        m_isdlTerm.Trigger( eTermCleanupAndResetting );
    }

    return errRet;
}

