// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"
#include <ctype.h>
#include <_logredomap.hxx>






const LGPOS     lgposMax = { 0xffff, 0xffff, 0x7fffffff };
const LGPOS     lgposMin = { 0x0,  0x0,  0x0 };

#ifdef DEBUG
CCriticalSection g_critDBGPrint( CLockBasicInfo( CSyncBasicInfo( szDBGPrint ), rankDBGPrint, 0 ) );
#endif

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> ibLGCheckpoint;
PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> ibLGDbConsistency;
PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> ibLGTip;

LONG LLGCheckpointDepthCEFLPv( LONG iInstance, VOID *pvBuf )
{
    if ( NULL != pvBuf )
    {
        const QWORD ibTip           = ibLGTip.Get( iInstance );
        const QWORD ibCheckpoint    = ibLGCheckpoint.Get( iInstance );

        *(QWORD *)pvBuf = QWORD( ibTip > ibCheckpoint ? ibTip - ibCheckpoint : 0 );
    }
    return 0;
}

LONG LLGLogGenerationCheckpointDepthCEFLPv( LONG iInstance, VOID *pvBuf )
{
    if ( NULL != pvBuf )
    {
        const QWORD cbLogFileSize   = cbLGFileSize.Get( iInstance );

        if ( cbLogFileSize > 0 )
        {
            const DWORD lgenCurrent     = DWORD( ibLGTip.Get( iInstance ) / cbLogFileSize );
            const DWORD lgenCheckpoint  = DWORD( ibLGCheckpoint.Get( iInstance ) / cbLogFileSize );

            *(LONG*)pvBuf = LONG( lgenCurrent >= lgenCheckpoint ? ( lgenCurrent - lgenCheckpoint + 1 ) : 0 );
        }
        else
        {
            *(LONG *)pvBuf = LONG( 0 );
        }
    }
    return 0;
}

LONG LLGLogGenerationDatabaseConsistencyDepthCEFLPv( LONG iInstance, VOID *pvBuf )
{
    if ( NULL != pvBuf )
    {
        const QWORD cbLogFileSize   = cbLGFileSize.Get( iInstance );

        if ( cbLogFileSize > 0 )
        {
            const DWORD lgenCurrent         = DWORD( ibLGTip.Get( iInstance ) / cbLogFileSize );
            const DWORD lgenDbConsistentcy  = DWORD( ibLGDbConsistency.Get( iInstance ) / cbLogFileSize );

            *(LONG*)pvBuf = LONG( lgenCurrent >= lgenDbConsistentcy ? ( lgenCurrent - lgenDbConsistentcy + 1 ) : 0 );
        }
        else
        {
            *(LONG *)pvBuf = LONG( 0 );
        }
    }
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cbLGCheckpointDepthMax;
LONG LLGLogGenerationCheckpointDepthTargetCEFLPv( LONG iInstance, VOID *pvBuf )
{
    if ( NULL != pvBuf )
    {
        const QWORD cbLogFileSize = cbLGFileSize.Get( iInstance );

        *(LONG*)pvBuf = LONG( cbLogFileSize ? cbLGCheckpointDepthMax.Get( iInstance ) / cbLogFileSize : 0 );
    }
    return 0;
}

#endif

LOG::LOG( INST *pinst )
    :   CZeroInit( sizeof( LOG ) ),
        m_pinst( pinst ),
        m_critLGTrace( CLockBasicInfo( CSyncBasicInfo( szLGTrace ), rankLGTrace, 0 ) ),
        m_critCheckpoint( CLockBasicInfo( CSyncBasicInfo( szCheckpoint ), rankCheckpoint, CLockDeadlockDetectionInfo::subrankNoDeadlock ) ),
        m_fDisableCheckpoint( fTrue ),
        m_lgenInitial( 1 ),
        m_critShadowLogConsume( CLockBasicInfo( CSyncBasicInfo( szShadowLogConsume ), rankShadowLogConsume, 0 ) ),
        m_pshadlog( NULL ),
        m_fAfterEndAllSessions( fTrue )
{
    m_pLogStream = NULL;
    m_pLogReadBuffer = NULL;
    m_pLogWriteBuffer = NULL;
    m_pctablehash = NULL;
    m_pcsessionhash = NULL;
    PERFOpt( ibLGCheckpoint.Clear( m_pinst ) );
    PERFOpt( ibLGDbConsistency.Clear( m_pinst ) );
    PERFOpt( cbLGCheckpointDepthMax.Clear( m_pinst ) );

    m_lgposRecoveryStop = lgposMax;
}

LOG::~LOG()
{
    m_LogBuffer.LGTermLogBuffers();
    delete m_pLogWriteBuffer;
    delete m_pLogReadBuffer;
    delete m_pLogStream;

    PERFOpt( ibLGCheckpoint.Clear( m_pinst ) );
    PERFOpt( ibLGDbConsistency.Clear( m_pinst ) );
    PERFOpt( cbLGCheckpointDepthMax.Clear( m_pinst ) );


    Assert( m_pctablehash == NULL || m_fLGNoMoreLogWrite );
    Assert( m_pcsessionhash == NULL || m_fLGNoMoreLogWrite );

    delete m_pctablehash;
    delete m_pcsessionhash;
}
ERR
LOG::ErrLGPreInit()
{
    ERR err = JET_errSuccess;

    LOG_STREAM *pLogStream;

    Alloc( pLogStream = new LOG_STREAM( m_pinst, this ) );
    Assert( m_pLogStream == NULL );
    m_pLogStream = pLogStream;

    Assert( m_pLogWriteBuffer == NULL );
    Alloc( m_pLogWriteBuffer = new LOG_WRITE_BUFFER( m_pinst, this, m_pLogStream, &m_LogBuffer ) );
    Call( m_pLogWriteBuffer->ErrLGInit() );

    Assert( m_pLogReadBuffer == NULL );
    Alloc( m_pLogReadBuffer = new LOG_READ_BUFFER( m_pinst, this, m_pLogStream, &m_LogBuffer ) );

    pLogStream->SetLogBuffers( m_pLogWriteBuffer );

HandleError:
    return err;
}

VOID LOG::LGReportError( const MessageId msgid, const ERR err, const WCHAR* const wszLogFile ) const
{
    WCHAR           wszError[ 64 ];
    const WCHAR*    rgpsz[] = { wszLogFile, wszError };

    OSStrCbFormatW( wszError, sizeof( wszError ), L"%i (0x%08x)", err, err );

    UtilReportEvent(    eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        msgid,
                        _countof( rgpsz ),
                        rgpsz,
                        0,
                        NULL,
                        m_pinst );
}

VOID LOG::LGReportError( const MessageId msgid, const ERR err ) const
{
    WCHAR   wszAbsPath[ IFileSystemAPI::cchPathMax ];

    if ( m_pinst->m_pfsapi->ErrPathComplete( m_pLogStream->LogName(), wszAbsPath ) < JET_errSuccess )
    {
        OSStrCbCopyW( wszAbsPath, sizeof( wszAbsPath ), m_pLogStream->LogName() );
    }
    LGReportError( msgid, err, wszAbsPath );
}

VOID LOG::LGReportError( const MessageId msgid, const ERR err, IFileAPI* const pfapi ) const
{
    WCHAR       wszAbsPath[ IFileSystemAPI::cchPathMax ];
    const ERR   errPath = pfapi->ErrPath( wszAbsPath );
    if ( errPath < JET_errSuccess )
    {

        OSStrCbFormatW( wszAbsPath, sizeof( wszAbsPath ),
                    L"%ws [%i (0x%08x)]",
                    m_pLogStream->LogName(),
                    errPath,
                    errPath );
    }
    LGReportError( msgid, err, wszAbsPath );
}

const LogVersion LOG::LgvForReporting() const
{
    return LgvFromLgfilehdr( m_pLogStream->GetCurrentFileHdr() );
}

const LGFILEHDR_FIXED * LOG::PlgfilehdrForVerCtrl() const
{
#ifndef DEBUG
    EnforceSz( FRecovering() && ( FRecoveringMode() == fRecoveringRedo ), "PlgfilehdrForVerCtrlDoTime" );
#endif

    return &( m_pLogStream->GetCurrentFileHdr()->lgfilehdr );
}



ERR LOG::ErrLGInit( BOOL *pfNewCheckpointFile )
{
    ERR err;
    LONG    lGenMin = 0;
    LONG    lGenMax = 0;

    if ( m_fLogInitialized )
        return JET_errSuccess;

    Assert( m_fLogDisabled == fFalse );

    PERFOpt( cLGUsersWaiting.Clear( m_pinst ) );

#ifdef DEBUG
    AssertLRSizesConsistent();
#endif

#ifdef DEBUG
{
    WCHAR * wsz;

    if ( ( wsz = GetDebugEnvValue ( L"FREEZECHECKPOINT" ) ) != NULL )
    {
        m_fDBGFreezeCheckpoint = fTrue;
        delete[] wsz;
    }
    else
        m_fDBGFreezeCheckpoint = fFalse;

    if ( ( wsz = GetDebugEnvValue ( L"TRACEREDO" ) ) != NULL )
    {
        m_fDBGTraceRedo = fTrue;
        delete[] wsz;
    }
    else
        m_fDBGTraceRedo = fFalse;

    if ( ( wsz = GetDebugEnvValue ( L"TRACEBR" ) ) != NULL )
    {
        m_fDBGTraceBR = _wtoi( wsz );
        delete[] wsz;
    }
    else
        m_fDBGTraceBR = 0;
}
#endif

    CallR( m_pLogStream->ErrLGInit() );

    ResetNoMoreLogWrite();

    m_pLogStream->ErrEmitSignalLogBegin();


    m_pttFirst = NULL;
    m_pttLast = NULL;

    Assert( m_fHardRestore || m_pLogStream->LogExt() == NULL );
    Assert( !m_fHardRestore || m_pLogStream->LogExt() != NULL );

    Call( m_pLogWriteBuffer->ErrLGTasksInit() );

    Call( ErrLGICheckpointInit( pfNewCheckpointFile ) );

    memset( &m_signLog, 0, sizeof( m_signLog ) );
    m_fSignLogSet = fFalse;


    (void)m_pLogStream->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lGenMin, &lGenMax, m_pLogStream->LogExt() ? fFalse : fTrue );
    lGenMax += 1;
    if ( lGenMax >= lGenerationMax )
    {
        m_pLogStream->SetLogSequenceEnd();
    }

    if ( 0 == lGenMin )
    {

        WCHAR wsz[11];
        if ( FOSConfigGet( L"DEBUG", L"Initial Log Generation", wsz, sizeof(wsz) ) )
        {
            lGenMin = _wtoi( wsz );
            lGenMin = (lGenMin == 0) ? 1 : lGenMin;
        }
        else
        {
            lGenMin = 1;
        }
    }
    Assert( lGenMin );

    LGISetInitialGen( lGenMin );

    m_fLogInitialized = fTrue;

    return err;

HandleError:
    m_pLogWriteBuffer->LGTasksTerm();

    m_pLogStream->LGTerm();

    return err;
}


ERR LOG::ErrLGTerm( const BOOL fLogQuitRec )
{
    ERR         err             = JET_errSuccess;
    BOOL        fTermFlushLog   = fFalse;
    LE_LGPOS    le_lgposStart;
    LGPOS       lgposQuit;

    if ( !m_fLogInitialized )
    {
        if ( m_pshadlog )
        {
            err = ErrLGShadowLogTerm();
            Assert( NULL == m_pshadlog );
        }

        if ( m_pinst->m_isdlTerm.FActiveSequence() )
        {
            m_pinst->m_isdlTerm.Trigger( eTermLogFilesClosed );
        }
        return err;
    }

    if ( !fLogQuitRec
        || m_fLGNoMoreLogWrite )
    {
        goto HandleError;
    }

    Assert( !m_fRecovering );
    Expected( m_pinst->m_isdlTerm.FActiveSequence() );

    le_lgposStart = m_lgposStart;
    Call( ErrLGQuit(
                this,
                &le_lgposStart,
                BoolParam( m_pinst, JET_paramAggressiveLogRollover ),
                &lgposQuit ) );

    Call( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fFalse ) );

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermLogFlushed );
    }

    Call( m_pLogStream->ErrLGTermFlushLog( fTrue ) );

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermLogFsFlushed );
    }

    fTermFlushLog = fTrue;

    Call( ErrLGIUpdateCheckpointLgposForTerm( lgposQuit ) );

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermCheckpointFlushed );
    }

#ifdef DEBUG
    {
        m_pLogWriteBuffer->VerifyAllWritten();
    }
#endif




    if ( err >= JET_errSuccess )
    {
        if ( m_pLogStream->GetLogSequenceEnd() )
        {
            err = ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent );
        }
        Call( err );
    }

HandleError:

    if ( !fTermFlushLog )
    {
        err = m_pLogStream->ErrLGTermFlushLog( fFalse );
        if ( m_pinst->m_isdlTerm.FActiveSequence() )
        {
            m_pinst->m_isdlTerm.Trigger( eTermLogFsFlushed );
        }
    }


    m_pLogWriteBuffer->LGTasksTerm();
    
    LGDisableCheckpoint();
    LGICheckpointTerm();

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermLogTasksTermed );
    }


    m_pLogStream->LGCloseFile();

    m_pLogStream->ErrEmitSignalLogEnd();

    if ( m_pshadlog )
    {
        const ERR errLGShadow = ErrLGShadowLogTerm();
        Assert( NULL == m_pshadlog );
        err = ( err == JET_errSuccess ? errLGShadow : err );
    }


    m_pLogStream->LGCreateAsynchCancel( fTrue );

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermLogFilesClosed );
    }

    m_pLogStream->LGTermTmpLogBuffers();

    m_LogBuffer.LGTermLogBuffers();
    m_pLogStream->LGTerm();


    if ( err >= JET_errSuccess )
    {
        if ( m_pLogStream->GetLogSequenceEnd() )
        {
            err = ErrERRCheck( JET_errLogSequenceEnd );
        }
    }

    m_fLogInitialized = fFalse;

    return err;
}



BOOL g_fStallFirewallFired = fFalse;

ERR LOG::ErrLGLogRec(   const DATA* const           rgdata,
                        const ULONG                 cdata,
                        const BOOL                  fLGFlags,
                        const LONG                  lgenBegin0,
                        LGPOS* const                plgposLogRec,
                        CCriticalSection * const    pcrit )
{
    ERR err = JET_errSuccess;

    TICK tickStart = TickOSTimeCurrent();

    while ( ( err = ErrLGTryLogRec( rgdata, cdata, fLGFlags, lgenBegin0, plgposLogRec ) ) == errLGNotSynchronous )
    {
        if ( NULL != pcrit )
            pcrit->Leave();

        m_pLogWriteBuffer->WriteToClearSpace();

        if ( !g_fStallFirewallFired &&
#ifdef DEBUG
             TickOSTimeCurrent() - tickStart > 20 * 60 * 1000 &&
#else
             TickOSTimeCurrent() - tickStart > 5 * 60 * 1000 &&
#endif
             !FNegTest( fHangingIOs ) )
        {
            AssertTrack( fFalse, "LogRecRetryStalled" );
            g_fStallFirewallFired = fTrue;
        }

        if ( NULL != pcrit )
            pcrit->Enter();
    }

    return err;
}

ERR LOG::ErrLGTryLogRec(    const DATA* const   rgdata,
                            const ULONG         cdata,
                            const BOOL          fLGFlags,
                            const LONG          lgenBegin0,
                            LGPOS* const        plgposLogRec )
{
    if ( m_pttFirst )
    {
        ERR err = JET_errSuccess;

        do {
            m_critLGTrace.Enter();
            if ( m_pttFirst )
            {
                TEMPTRACE *ptt = m_pttFirst;
                err = ErrLGITrace( ptt->ppib, ptt->szData, fTrue  );
                if ( err >= JET_errSuccess )
                {
                    m_pttFirst = ptt->pttNext;
                    if ( m_pttFirst == NULL )
                    {
                        Assert( m_pttLast == ptt );
                        m_pttLast = NULL;
                    }
                    OSMemoryHeapFree( ptt );
                }
            }
            m_critLGTrace.Leave();
        } while ( m_pttFirst != NULL && err == JET_errSuccess );

        if ( err != JET_errSuccess )
            return err;
    }


    return m_pLogWriteBuffer->ErrLGLogRec( rgdata, cdata, fLGFlags, lgenBegin0, plgposLogRec );
}


ERR LOG::ErrLGITrace(
    PIB *ppib,
    __in PSTR sz,
    BOOL fInternal )
{
    ERR             err;
    const ULONG     cdata           = 3;
    DATA            rgdata[cdata];
    LRTRACE         lrtrace;
    DATETIME        datetime;
    const ULONG     cbDateTimeBuf   = 31;
    CHAR            szDateTimeBuf[cbDateTimeBuf+1];


    if ( m_fRecovering && m_fRecoveringMode == fRecoveringRedo )
        return JET_errSuccess;

    if ( m_fLogDisabled )
        return JET_errSuccess;

    lrtrace.lrtyp = lrtypTrace;
    if ( ppib != ppibNil )
    {
        Assert( ppib->procid < 64000 );
        lrtrace.le_procid = ppib->procid;
    }
    else
    {
        lrtrace.le_procid = procidNil;
    }

    UtilGetCurrentDateTime( &datetime );

    szDateTimeBuf[cbDateTimeBuf] = 0;
    OSStrCbFormatA(
        szDateTimeBuf,
        cbDateTimeBuf,
        "[%u/%u/%u %u:%02u:%02u.%3.3u] ",
        datetime.month,
        datetime.day,
        datetime.year,
        datetime.hour,
        datetime.minute,
        datetime.second,
        datetime.millisecond);

    rgdata[0].SetPv( (BYTE *) &lrtrace );
    rgdata[0].SetCb( sizeof( lrtrace ) );

    rgdata[1].SetPv( reinterpret_cast<BYTE *>( szDateTimeBuf ) );
    rgdata[1].SetCb( strlen( szDateTimeBuf ) );

    rgdata[2].SetPv( reinterpret_cast<BYTE *>( sz ) );
    rgdata[2].SetCb( strlen( sz ) + 1 );

    lrtrace.le_cb = USHORT( rgdata[1].Cb() + rgdata[2].Cb() );

    if ( fInternal )
    {

        return m_pLogWriteBuffer->ErrLGLogRec( rgdata, cdata, 0, 0, pNil );
    }

    err = ErrLGTryLogRec( rgdata, cdata, 0, 0, pNil );
    if ( err == errLGNotSynchronous )
    {

        TEMPTRACE *ptt;

        AllocR( ptt = (TEMPTRACE *) PvOSMemoryHeapAlloc( sizeof( TEMPTRACE ) + strlen(sz) + 1 ) );
        UtilMemCpy( ptt->szData, sz, strlen(sz) + 1 );
        ptt->ppib = ppib;
        ptt->pttNext = NULL;

        m_critLGTrace.Enter();
        if ( m_pttLast != NULL )
            m_pttLast->pttNext = ptt;
        else
            m_pttFirst = ptt;
        m_pttLast = ptt;
        m_critLGTrace.Leave();

        err = JET_errSuccess;
    }

    return err;
}

ERR LOG::ErrLGTrace(
    PIB *ppib,
    __in PSTR sz )
{
    ERR err = ErrLGITrace( ppib, sz, fFalse );

    if ( ( JET_errLogWriteFail == err || JET_errOutOfMemory == err )
        && FRFSAnyFailureDetected() )
    {
        err = JET_errSuccess;
    }

    CallS( err );

    OSTrace( JET_tracetagLogging, sz );

    return err;
}


#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
bool FLGIsLVChunkSizeCompatible(
    const ULONG cbPage,
    const ULONG ulVersionMajor,
    const ULONG ulVersionMinor,
    const ULONG ulVersionUpdate )
{

    Assert( 2*1024 == cbPage
            || 4*1024 == cbPage
            || 8*1024 == cbPage
            || 16*1024 == cbPage
            || 32*1024 == cbPage );


    if( FIsSmallPage( cbPage ) )
    {
        return true;
    }

    if( ulVersionMajor == ulLGVersionMajorNewLVChunk
        && ulVersionMinor == ulLGVersionMinorNewLVChunk
        && ulVersionUpdate < ulLGVersionUpdateNewLVChunk )
    {
        return false;
    }

    return true;
}
    
#endif

ERR ErrLGICheckVersionCompatibility( const INST * const pinst, const LGFILEHDR* const plgfilehdr );

ERR ErrLGIReadFileHeader(
    IFileAPI * const    pfapiLog,
    const TraceContext& tc,
    OSFILEQOS           grbitQOS,
    LGFILEHDR *         plgfilehdr,
    const INST * const  pinst )
{
    ERR                 err                 = JET_errSuccess;
    PAGECHECKSUM        checksumExpected    = 0;
    PAGECHECKSUM        checksumActual      = 0;

    

    TraceContextScope tcScope( iorsHeader );
    Call( pfapiLog->ErrIORead( *tcScope, 0, sizeof( LGFILEHDR ), (BYTE* const)plgfilehdr, grbitQOS ) );

    
    ChecksumPage(
                plgfilehdr,
                sizeof(LGFILEHDR),
                logfileHeader,
                0,
                &checksumExpected,
                &checksumActual );

    if( checksumExpected != checksumActual )
    {
        if ( !BoolParam( JET_paramDisableBlockVerification ) )
        {
            err = ErrERRCheck( JET_errLogFileCorrupt );
        }
    }

    
    if ( *(LONG *)(((char *)plgfilehdr) + 24) == 4
         && ( *(LONG *)(((char *)plgfilehdr) + 28) == 909
              || *(LONG *)(((char *)plgfilehdr) + 28) == 995 )
         && *(LONG *)(((char *)plgfilehdr) + 32) == 0
        )
    {
        
        err = ErrERRCheck( JET_errDatabase500Format );
    }
    else if ( *(LONG *)(((char *)plgfilehdr) + 20) == 443
         && *(LONG *)(((char *)plgfilehdr) + 24) == 0
         && *(LONG *)(((char *)plgfilehdr) + 28) == 0 )
    {
        
        err = ErrERRCheck( JET_errDatabase400Format );
    }
    else if ( *(LONG *)(((char *)plgfilehdr) + 44) == 0
         && *(LONG *)(((char *)plgfilehdr) + 48) == 0x0ca0001 )
    {
        
        err = ErrERRCheck( JET_errDatabase200Format );
    }

    Call( err );

    if( JET_filetypeUnknown != plgfilehdr->lgfilehdr.le_filetype
        && JET_filetypeLog != plgfilehdr->lgfilehdr.le_filetype )
    {
        Call( ErrERRCheck( JET_errFileInvalidType ) );
    }

    Call( ErrLGICheckVersionCompatibility( pinst, plgfilehdr ) );

    CallS( err );
    err = JET_errSuccess;

HandleError:
    return err;
}

enum eLogVersionCompatibility{
    eLogVersionCompatible,
    eLogVersionTooOld,
    eLogVersionTooNew,
};

#define COMPARE_LOG_VERSION(a, b) ((a) == (b) ? eLogVersionCompatible : ((a) < (b) ? eLogVersionTooOld : eLogVersionTooNew) )
    
ERR ErrLGICheckVersionCompatibility( const INST * const pinst, const LGFILEHDR* const plgfilehdr )
{
    ERR err = JET_errSuccess;

    Assert ( NULL != plgfilehdr );
    eLogVersionCompatibility result = eLogVersionCompatible;
    if( eLogVersionCompatible == ( result = COMPARE_LOG_VERSION ( plgfilehdr->lgfilehdr.le_ulMajor, ulLGVersionMajorMax ) ) )
    {
        if( eLogVersionCompatible == ( result = COMPARE_LOG_VERSION ( plgfilehdr->lgfilehdr.le_ulMinor, ulLGVersionMinorFinalDeprecatedValue ) ) )
        {
                if( plgfilehdr->lgfilehdr.le_ulUpdateMajor > ulLGVersionUpdateMajorMax )
                {
                    OSTrace( JET_tracetagUpgrade, OSFormat( "Returning eLogVersionTooNew -- %d.%d.%d vs. %d.%d.%d \n",
                                (ULONG)plgfilehdr->lgfilehdr.le_ulMajor, (ULONG)plgfilehdr->lgfilehdr.le_ulMinor, (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMajor,
                                ulLGVersionMajorMax, ulLGVersionMinorFinalDeprecatedValue, ulLGVersionUpdateMajorMax ) );
                    result = eLogVersionTooNew;
                }
        }
    }

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    if( eLogVersionCompatible != result )
    {
        Assert( 8 == ulLGVersionMajorMax );
        Assert( 4000 == ulLGVersionMinorFinalDeprecatedValue );
        if( ulLGVersionMajor_OldLrckFormat == plgfilehdr->lgfilehdr.le_ulMajor &&
            ulLGVersionMinor_OldLrckFormat == plgfilehdr->lgfilehdr.le_ulMinor )
        {
            result = eLogVersionCompatible;
        }
    }
    
    if( eLogVersionCompatible == result )
    {
        
        if( !FLGIsLVChunkSizeCompatible(
                g_cbPage,
                plgfilehdr->lgfilehdr.le_ulMajor,
                plgfilehdr->lgfilehdr.le_ulMinor,
                plgfilehdr->lgfilehdr.le_ulUpdateMajor ) )
        {
            OSTrace( JET_tracetagUpgrade, OSFormat( "Returning eLogVersionTooOld -- %d.%d.%d vs. %d.%d.%d\n",
                        (ULONG)plgfilehdr->lgfilehdr.le_ulMajor, (ULONG)plgfilehdr->lgfilehdr.le_ulMinor, (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMajor,
                        ulLGVersionMajorNewLVChunk, ulLGVersionMinorNewLVChunk, ulLGVersionUpdateNewLVChunk ) );
            result = eLogVersionTooOld;
        }
    }
#endif

    if ( eLogVersionCompatible != result )
    {
        if ( result == eLogVersionTooOld )
        {
            WCHAR wszLogGeneration[20];
            WCHAR wszLogVersion[50];
            WCHAR wszEngineVersion[50];
            const WCHAR * rgszT[3] = { wszLogGeneration, wszLogVersion, wszEngineVersion };


            const LogVersion lgvEngineMax = PfmtversEngineMax()->lgv;
            OSStrCbFormatW( wszLogGeneration, _cbrg( wszLogGeneration ), L"0x%x", (LONG)plgfilehdr->lgfilehdr.le_lGeneration );
            OSStrCbFormatW( wszLogVersion, _cbrg( wszLogVersion ), L"%d.%d.%d.%d", (ULONG)plgfilehdr->lgfilehdr.le_ulMajor, (ULONG)plgfilehdr->lgfilehdr.le_ulMinor, (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMajor, (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMinor );
            OSStrCbFormatW( wszEngineVersion, _cbrg( wszEngineVersion ), L"%d.%d.%d", lgvEngineMax.ulLGVersionMajor, lgvEngineMax.ulLGVersionUpdateMajor, lgvEngineMax.ulLGVersionUpdateMinor );
            
            UtilReportEvent( eventError,
                    GENERAL_CATEGORY,
                    LOG_VERSION_TOO_LOW_FOR_ENGINE_ID,
                    _countof( rgszT ),
                    rgszT,
                    0,
                    NULL,
                    pinst );
        }
        else
        {
            WCHAR wszLogGeneration[20];
            WCHAR wszLogVersion[50];
            WCHAR wszEngineVersion[50];
            const WCHAR * rgszT[3] = { wszLogGeneration, wszLogVersion, wszEngineVersion };
            
            const LogVersion lgvEngineMax = PfmtversEngineMax()->lgv;
            OSStrCbFormatW( wszLogGeneration, _cbrg( wszLogGeneration ), L"0x%x", (LONG)plgfilehdr->lgfilehdr.le_lGeneration );
            if ( plgfilehdr->lgfilehdr.le_ulMinor != ulLGVersionMinorFinalDeprecatedValue &&
                plgfilehdr->lgfilehdr.le_ulMinor != ulLGVersionMinor_Win7 )
            {
                AssertSz( fFalse, "Confused newer version of log is re-using le_ulMinor again after we deprecated it?" );
                OSStrCbFormatW( wszLogVersion, _cbrg( wszLogVersion ), L"%d.%d.%d.%d", (ULONG)plgfilehdr->lgfilehdr.le_ulMajor, (ULONG)plgfilehdr->lgfilehdr.le_ulMinor, (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMajor, (ULONG)plgfilehdr->lgfilehdr.le_ulUpdateMinor );
            }
            else
            {
                const LogVersion lgvLgfilehdr = LgvFromLgfilehdr( plgfilehdr );
                OSStrCbFormatW( wszLogVersion, _cbrg( wszLogVersion ), L"%d.%d.%d", lgvLgfilehdr.ulLGVersionMajor, lgvLgfilehdr.ulLGVersionUpdateMajor, lgvLgfilehdr.ulLGVersionUpdateMinor );
            }

            OSStrCbFormatW( wszEngineVersion, _cbrg( wszEngineVersion ), L"%d.%d.%d", lgvEngineMax.ulLGVersionMajor, lgvEngineMax.ulLGVersionUpdateMajor, lgvEngineMax.ulLGVersionUpdateMinor );
            
            UtilReportEvent( eventError,
                    GENERAL_CATEGORY,
                    LOG_VERSION_TOO_HIGH_FOR_ENGINE_ID,
                    _countof( rgszT ),
                    rgszT,
                    0,
                    NULL,
                    pinst );
        }

        Call( ErrERRCheck( JET_errBadLogVersion ) );
    }


    const FormatVersions * pfmtversAllowed = NULL;
    JET_ENGINEFORMATVERSION efvUser = pinst ?
                                        (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ) :
                                        EfvMaxSupported();

    if ( efvUser == JET_efvUsePersistedFormat )
    {
        efvUser = EfvMaxSupported();

        const LogVersion lgv = LgvFromLgfilehdr( plgfilehdr );
        const ERR errT = ErrLGFindHighestMatchingLogMajors( lgv, &pfmtversAllowed );
        CallS( errT );
        if ( errT < JET_errSuccess )
        {
            FireWall( OSFormat( "GetHighestLgMajorFailedVer:%lu.%lu.%lu", (ULONG)lgv.ulLGVersionMajor, (ULONG)lgv.ulLGVersionUpdateMajor, (ULONG)lgv.ulLGVersionUpdateMinor ) );
            CallS( ErrGetDesiredVersion( NULL, efvUser, &pfmtversAllowed ) );
        }
    }

    const BOOL fAllowPersistedFormat = ( efvUser & JET_efvAllowHigherPersistedFormat ) == JET_efvAllowHigherPersistedFormat;
    if ( fAllowPersistedFormat )
    {
        efvUser = efvUser & ~JET_efvAllowHigherPersistedFormat;
    }

    if ( pfmtversAllowed == NULL )
    {
        CallS( ErrGetDesiredVersion( pinst, efvUser, &pfmtversAllowed ) );
        if ( CmpLgVer( LgvFromLgfilehdr( plgfilehdr ), PfmtversEngineMax()->lgv ) > 0 )
        {
            AssertSz( fFalse, "This really should be protected by previous checks, but just in case we return the right error." );
            Call( ErrERRCheck( JET_errBadLogVersion ) );
        }
    }

    const INT icmpver = CmpLgVer( LgvFromLgfilehdr( plgfilehdr ), pfmtversAllowed->lgv );
    if ( !fAllowPersistedFormat && ( icmpver > 0 && !FUpdateMinorMismatch( icmpver ) ) )
    {
        WCHAR wszLogGeneration[20];
        WCHAR wszLogVersion[50];
        WCHAR wszParamVersion[50];
        WCHAR wszParamEfv[cchFormatEfvSetting];
        const WCHAR * rgszT[5] = { wszLogGeneration, wszLogVersion, wszParamVersion, wszParamEfv, L""  };

        const LogVersion lgvLgfilehdr = LgvFromLgfilehdr( plgfilehdr );
        OSStrCbFormatW( wszLogGeneration, _cbrg( wszLogGeneration ), L"0x%x", (LONG)plgfilehdr->lgfilehdr.le_lGeneration );
        OSStrCbFormatW( wszLogVersion, _cbrg( wszLogVersion ), L"%d.%d.%d", lgvLgfilehdr.ulLGVersionMajor, lgvLgfilehdr.ulLGVersionUpdateMajor, lgvLgfilehdr.ulLGVersionUpdateMinor );
        OSStrCbFormatW( wszParamVersion, _cbrg( wszParamVersion ), L"%d.%d.%d", pfmtversAllowed->lgv.ulLGVersionMajor, pfmtversAllowed->lgv.ulLGVersionUpdateMajor, pfmtversAllowed->lgv.ulLGVersionUpdateMinor );
        FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ), wszParamEfv, sizeof(wszParamEfv) );
        
        UtilReportEvent( eventError,
                GENERAL_CATEGORY,
                LOG_VERSION_TOO_HIGH_FOR_PARAM_ID,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                pinst );

        
        if( pinst == NULL )
        {
            FireWall( "CheckingLogVerWithoutInst" );
            err = JET_errSuccess;
            goto HandleError;
        }
        
        Call( ErrERRCheck( JET_errEngineFormatVersionSpecifiedTooLowForLogVersion ) );
    }

HandleError:

    return err;
}

BOOL FLGVersionZeroFilled( const LGFILEHDR_FIXED* const plgfilehdr )
{
    if ( plgfilehdr->le_ulMajor > ulLGVersionMajor_ZeroFilled ) return fTrue;
    if ( plgfilehdr->le_ulMajor < ulLGVersionMajor_ZeroFilled ) return fFalse;
    if ( plgfilehdr->le_ulMinor > ulLGVersionMinor_ZeroFilled ) return fTrue;
    if ( plgfilehdr->le_ulMinor < ulLGVersionMinor_ZeroFilled ) return fFalse;
    if ( plgfilehdr->le_ulUpdateMajor > ulLGVersionUpdateMajor_ZeroFilled ) return fTrue;
    if ( plgfilehdr->le_ulUpdateMajor < ulLGVersionUpdateMajor_ZeroFilled ) return fFalse;

    return fTrue;
}

BOOL FLGVersionAttachInfoInCheckpoint( const LGFILEHDR_FIXED* const plgfilehdr )
{
    if ( plgfilehdr->le_ulMajor > ulLGVersionMajor_AttachChkpt ) return fTrue;
    if ( plgfilehdr->le_ulMajor < ulLGVersionMajor_AttachChkpt ) return fFalse;
    if ( plgfilehdr->le_ulMinor > ulLGVersionMinor_AttachChkpt ) return fTrue;
    if ( plgfilehdr->le_ulMinor < ulLGVersionMinor_AttachChkpt ) return fFalse;
    if ( plgfilehdr->le_ulUpdateMajor > ulLGVersionUpdateMajor_AttachChkpt ) return fTrue;
    if ( plgfilehdr->le_ulUpdateMajor < ulLGVersionUpdateMajor_AttachChkpt ) return fFalse;
    if ( plgfilehdr->le_ulUpdateMinor >= ulLGVersionUpdateMinor_AttachChkpt ) return fTrue;

    return fFalse;
}

#ifdef DEBUG
ERR LOG::ErrLGGetPersistedLogVersion( _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const LogVersion ** const pplgv ) const
{
    Assert( UlParam( m_pinst, JET_paramEngineFormatVersion ) == JET_efvUsePersistedFormat );
    return m_pLogStream->ErrLGGetDesiredLogVersion( efv, pplgv );
}
#endif

ERR LOG::ErrLGFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature ) const
{
    Assert( !m_pinst->FRecovering() );

    if ( FLogDisabled() )
    {
        return JET_errSuccess;
    }

    return m_pLogStream->ErrLGFormatFeatureEnabled( efvFormatFeature );
}



VOID LGIGetDateTime( LOGTIME *plogtm )
{
    DATETIME tm;

    UtilGetCurrentDateTime( &tm );

    plogtm->bSeconds = BYTE( tm.second );
    plogtm->bMinutes = BYTE( tm.minute );
    plogtm->bHours = BYTE( tm.hour );
    plogtm->bDay = BYTE( tm.day );
    plogtm->bMonth = BYTE( tm.month );
    plogtm->bYear = BYTE( tm.year - 1900 );
    plogtm->fTimeIsUTC = fTrue;
    plogtm->SetMilliseconds( tm.millisecond );
    Assert( plogtm->Milliseconds() == tm.millisecond );
}
    
ERR ErrSetUserDbHeaderInfos(
    INST *              pinst,
    ULONG *     pcbinfomisc,
    JET_DBINFOMISC7 **  ppdbinfomisc
    )
{
    Assert( pcbinfomisc );
    Assert( ppdbinfomisc );

    JET_DBINFOMISC7 * const pdbinfomisc = new JET_DBINFOMISC7[ dbidMax ];
    if( 0 == pdbinfomisc )
    {
        return ErrERRCheck( JET_errOutOfMemory );
    }

    INT cdbinfomisc = 0;

    FMP::EnterFMPPoolAsReader();
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        if ( pinst->m_mpdbidifmp[ dbid ] >= g_ifmpMax )
        {
            continue;
        }

        FMP * const pfmpT = g_rgfmp + pinst->m_mpdbidifmp[ dbid ];
        if ( pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach() || !pfmpT->FAttached() )
        {
            continue;
        }

        if ( ErrDBSetUserDbHeaderInfo( pfmpT, sizeof(pdbinfomisc[cdbinfomisc]), &pdbinfomisc[cdbinfomisc] ) >= JET_errSuccess )
        {
            ++cdbinfomisc;
        }
    }
    FMP::LeaveFMPPoolAsReader();

    *pcbinfomisc = cdbinfomisc;
    *ppdbinfomisc = pdbinfomisc;

    return JET_errSuccess;
}

VOID CleanupRecoveryControl(
    const JET_SNT           snt,
    JET_RECOVERYCONTROL *   precctrl )
{
    if ( snt == JET_sntOpenLog )
    {
        precctrl->OpenLog.cdbinfomisc = 0;
        delete[] precctrl->OpenLog.rgdbinfomisc;
        precctrl->OpenLog.rgdbinfomisc = NULL;
    }
    else if ( snt == JET_sntMissingLog )
    {
        precctrl->MissingLog.cdbinfomisc = 0;
        delete[] precctrl->MissingLog.rgdbinfomisc;
        precctrl->MissingLog.rgdbinfomisc = NULL;
    }
    else if ( snt == JET_sntBeginUndo )
    {
        precctrl->BeginUndo.cdbinfomisc = 0;
        delete[] precctrl->BeginUndo.rgdbinfomisc;
        precctrl->BeginUndo.rgdbinfomisc = NULL;
    }
}

ERR LOG::ErrLGISetQuitWithoutUndo( const ERR errBeginUndo )
{
#ifndef RTM
    if ( errBeginUndo == JET_errRecoveredWithoutUndo )
    {
        m_errBeginUndo = errBeginUndo;
        (void)m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &m_lgenHighestAtEndOfRedo );
        m_fCurrentGenExistsAtEndOfRedo = m_pLogStream->FCurrentLogExists();
    }
#endif
    return errBeginUndo;
}

ERR LOG::ErrLGIBeginUndoCallback_( __in const CHAR * szFile, const ULONG lLine )
{
    const ERR err = ErrLGRecoveryControlCallback( m_pinst, NULL, NULL, JET_sntBeginUndo, JET_errSuccess, 0, fFalse, 0, szFile, lLine );
    return ErrLGISetQuitWithoutUndo( err );
}

BOOL LOG::FLGILgenHighAtRedoStillHolds()
{
#ifndef RTM
    if ( m_errBeginUndo == JET_errRecoveredWithoutUndo )
    {
        LONG lgenHighCurrent;
        (void)m_pLogStream->ErrLGGetGenerationRange( m_wszLogCurrent, NULL, &lgenHighCurrent );
        const BOOL fCurrentLogExists = m_pLogStream->FCurrentLogExists();

        AssertRTL( lgenHighCurrent == m_lgenHighestAtEndOfRedo ||
                   lgenHighCurrent == m_lgenHighestAtEndOfRedo + 1 );
        AssertRTL( fCurrentLogExists == !!m_fCurrentGenExistsAtEndOfRedo );
        return ( lgenHighCurrent == m_lgenHighestAtEndOfRedo ||
                 lgenHighCurrent == m_lgenHighestAtEndOfRedo + 1 ) &&
               fCurrentLogExists == !!m_fCurrentGenExistsAtEndOfRedo;
    }
#endif
    return fTrue;
}


ERR ErrLGRecoveryControlCallback(
    INST *              pinst,
    FMP *               pfmp,
    const WCHAR *       wszLogName,
    const JET_SNT       sntRecCtrl,
    const ERR           errDefault,
    const LONG          lgen,
    const BOOL          fCurrentLog,
    const ULONG         eDisposition,
    const CHAR *        szFile,
    const LONG          lLine )
{

    ERR                     err             = JET_errSuccess;
    JET_RECOVERYCONTROL     recctrl;

    if ( !pinst->m_pfnInitCallback )
    {
        if ( errDefault != JET_errSuccess )
        {
            ErrERRCheck_( errDefault, szFile, lLine );
        }
        return errDefault;
    }


    memset( &recctrl, 0, sizeof(JET_RECOVERYCONTROL) );
    recctrl.cbStruct = sizeof( JET_RECOVERYCONTROL );


    recctrl.instance = (JET_INSTANCE)pinst;
    recctrl.errDefault = errDefault;

    recctrl.sntUnion = sntRecCtrl;


    switch ( sntRecCtrl )
    {
        case JET_sntOpenLog:
            Assert( lgen >= 0 );
            Assert( lgen > 0 || fCurrentLog );

            recctrl.OpenLog.cbStruct = sizeof( recctrl.OpenLog );

            recctrl.OpenLog.lGenNext = (ULONG)lgen;
            recctrl.OpenLog.fCurrentLog = (unsigned char)fCurrentLog;
            recctrl.OpenLog.eReason = (unsigned char)eDisposition;
            recctrl.OpenLog.wszLogFile = (WCHAR*)wszLogName;
            Call( ErrSetUserDbHeaderInfos( pinst, &(recctrl.OpenLog.cdbinfomisc), &(recctrl.OpenLog.rgdbinfomisc) ) );
            break;

        case JET_sntOpenCheckpoint:
            recctrl.OpenCheckpoint.cbStruct = sizeof( recctrl.OpenCheckpoint );
            recctrl.OpenCheckpoint.wszCheckpoint = NULL;
            break;

        case JET_sntMissingLog:
            Assert( lgen >= 0 );

            recctrl.MissingLog.cbStruct = sizeof( recctrl.MissingLog );

            recctrl.MissingLog.lGenMissing = lgen;
            recctrl.MissingLog.fCurrentLog = (unsigned char)fCurrentLog;
            recctrl.MissingLog.wszLogFile = (WCHAR*)wszLogName;
            recctrl.MissingLog.eNextAction = (unsigned char)eDisposition;
            Call( ErrSetUserDbHeaderInfos( pinst, &(recctrl.MissingLog.cdbinfomisc), &(recctrl.MissingLog.rgdbinfomisc) ) );
            break;

        case JET_sntBeginUndo:
            recctrl.BeginUndo.cbStruct = sizeof( recctrl.BeginUndo );
            Call( ErrSetUserDbHeaderInfos( pinst, &(recctrl.BeginUndo.cdbinfomisc), &(recctrl.BeginUndo.rgdbinfomisc) ) );
            break;

        case JET_sntNotificationEvent:
            recctrl.NotificationEvent.cbStruct = sizeof( recctrl.NotificationEvent );
            recctrl.NotificationEvent.EventID = errDefault;
            recctrl.errDefault = JET_errSuccess;
            break;

        case JET_sntSignalErrorCondition:
            recctrl.SignalErrorCondition.cbStruct = sizeof( recctrl.SignalErrorCondition );
            break;

        case JET_sntAttachedDb:
            recctrl.AttachedDb.cbStruct = sizeof( recctrl.AttachedDb );
            recctrl.AttachedDb.wszDbPath = pfmp->WszDatabaseName();
            break;

        case JET_sntDetachingDb:
            recctrl.DetachingDb.cbStruct = sizeof( recctrl.DetachingDb );
            recctrl.DetachingDb.wszDbPath = pfmp->WszDatabaseName();
            break;

        case JET_sntCommitCtx:
            Assert( lgen >= 0 );
            recctrl.CommitCtx.cbStruct = sizeof( recctrl.CommitCtx );
            recctrl.CommitCtx.pbCommitCtx = (BYTE *)wszLogName;
            recctrl.CommitCtx.cbCommitCtx = lgen;
            break;

        default:
            AssertSz( fFalse, "Unknown JET_SNT = %d\n", sntRecCtrl );
    }

    OSTrace( JET_tracetagCallbacks, OSFormat( "ErrLGRecoveryControlCallback( snt=%d, errDefault=%d, lgen=%d ) @ %hs:%d \n",
                sntRecCtrl, errDefault, lgen, szFile, lLine ) );

    const HRT hrtStart = HrtHRTCount();
    Ptls()->fInCallback = fTrue;
    TRY
    {
        err = (*pinst->m_pfnInitCallback)( JET_snpRecoveryControl, sntRecCtrl, &recctrl, pinst->m_pvInitCallbackContext );
    }
    EXCEPT( efaExecuteHandler )
    {
        err = ErrERRCheck( JET_errCallbackFailed );
        OSTrace( JET_tracetagCallbacks, OSFormat( "Caught exception from ErrLGRecoveryControlCallback( ... )! @ %hs:%d\n", szFile, lLine ) );
    }
    Ptls()->fInCallback = fFalse;
    pinst->m_isdlInit.AddCallbackTime( DblHRTElapsedTimeFromHrtStart( hrtStart ) );

    OSTrace( JET_tracetagCallbacks, OSFormat( "ErrLGRecoveryControlCallback( ... ) @ %hs:%d returned --> %d\n", szFile, lLine, err ) );

    if ( sntRecCtrl == JET_sntAttachedDb || sntRecCtrl == JET_sntDetachingDb )
    {
        const LGPOS lgposRedo = pinst->m_plog->LgposLGLogTipNoLock();
        WCHAR szlgposRedo[64], szErr[32];
        OSStrCbFormatW( szlgposRedo, sizeof( szlgposRedo ), L"(%08X,%04X,%04X)", lgposRedo.lGeneration, lgposRedo.isec, lgposRedo.ib );
        OSStrCbFormatW( szErr, sizeof( szErr ), L"%X", err );
        const WCHAR *rgsz[] = { pfmp->WszDatabaseName(), szlgposRedo, szErr };
        UtilReportEvent(
                eventInformation,
                LOGGING_RECOVERY_CATEGORY,
                ( sntRecCtrl == JET_sntAttachedDb ) ? RECOVERY_DB_ATTACH_ID : RECOVERY_DB_DETACH_ID,
                _countof(rgsz),
                rgsz,
                0,
                NULL,
                pinst );
    }

    if ( err != JET_errSuccess )
    {
        if ( errDefault == err )
        {
            ErrERRCheck_( err, szFile, lLine );
        }
        else
        {
            ErrERRCheck( err );
        }
    }

    if ( sntRecCtrl == JET_sntMissingLog &&
            ( err == JET_errFileNotFound || err == JET_errMissingLogFile ) )
    {

        pinst->m_plog->LGReportError( LOG_OPEN_FILE_ERROR_ID, JET_errFileNotFound );

        if ( recctrl.errDefault != JET_errSuccess )
        {
            OSUHAEmitFailureTag( pinst, HaDbFailureTagConfiguration, L"eb1fa469-3926-45a9-8286-075850b5069a" );
        }

        err = ErrERRCheck( JET_errMissingLogFile );
    }

HandleError:

    CleanupRecoveryControl( sntRecCtrl, &recctrl );

    return err;
}

ERR ErrLGDbAttachedCallback_( INST *pinst, FMP *pfmp, const CHAR *szFile, const LONG lLine )
{
    const ERR err = ErrLGRecoveryControlCallback( pinst, pfmp, NULL, JET_sntAttachedDb, JET_errSuccess, 0, fFalse, 0, szFile, lLine );

    if ( BoolParam( pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
    {
        pinst->m_pbackup->BKLockBackup();
        pinst->m_pbackup->BKSetIsSuspended( fFalse );
        pinst->m_pbackup->BKUnlockBackup();
    }

    return err;
}

ERR ErrLGDbDetachingCallback_( INST *pinst, FMP *pfmp, const CHAR *szFile, const LONG lLine )
{
    const ERR err = ErrLGRecoveryControlCallback( pinst, pfmp, NULL, JET_sntDetachingDb, JET_errSuccess, 0, fFalse, 0, szFile, lLine );

    if ( BoolParam( pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
    {
        pinst->m_pbackup->BKLockBackup();
        pinst->m_pbackup->BKSetIsSuspended( fTrue );

        if ( pinst->m_pbackup->FBKBackupInProgress() )
        {
            const TICK  tickStart = TickOSTimeCurrent();

            Assert( pinst->m_pbackup->FBKBackupIsInternal() );

            BOOL fBackupInProgress = fTrue;
            while ( fBackupInProgress )
            {
                fBackupInProgress = pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly );
                if ( fBackupInProgress )
                {
                    pinst->m_pbackup->BKUnlockBackup();
                    UtilSleep( cmsecWaitGeneric );
                    pinst->m_pbackup->BKLockBackup();
                }
            }

            pinst->m_pbackup->BKAssertNoBackupInProgress();

            const UINT  csz = 2;
            const WCHAR * rgszT[csz];
            WCHAR   szSeconds[64];

            OSStrCbFormatW( szSeconds, sizeof( szSeconds ), L"%g", ( TickOSTimeCurrent() - tickStart ) / 1000.0 );
            rgszT[0] = pfmp->WszDatabaseName();
            rgszT[1] = szSeconds;

            UtilReportEvent(
                eventInformation,
                LOGGING_RECOVERY_CATEGORY,
                SUSPEND_INTERNAL_COPY_DURING_RECOVERY_INSTANCE_ID,
                csz,
                rgszT,
                0,
                NULL,
                pinst );
        }

        pinst->m_pbackup->BKUnlockBackup();
    }

    return err;
}

ERR LOG::ErrLGOpenPagePatchRequestCallback(
    const IFMP ifmp,
    const PGNO pgno,
    const DBTIME dbtime,
    const void * const pvPage ) const
{
    ERR err;
    
    Assert(pgnoNull != pgno);
    Assert(dbtimeInvalid != pgno);
    Assert(NULL != pvPage);
    Assert(m_fRecovering);

    if ( m_pinst->m_pfnInitCallback &&
        BoolParam( PinstFromIfmp( ifmp ), JET_paramEnableExternalAutoHealing ) &&
        m_fRecovering &&
        m_fRecoveringMode == fRecoveringRedo )
    {
        PAGE_PATCH_TOKEN token;
        token.cbStruct = sizeof(token);
        token.dbtime = dbtime;
        memcpy(&token.signLog, &m_signLog, sizeof(SIGNATURE));

        JET_SNPATCHREQUEST snPatch;
        snPatch.cbStruct = sizeof(snPatch);
        snPatch.pageNumber = pgno;
        snPatch.szLogFile = m_pLogStream->LogName();
        snPatch.instance = (JET_INSTANCE)m_pinst;
        snPatch.pvToken = &token;
        snPatch.cbToken = sizeof(token);
        snPatch.pvData = pvPage;
        snPatch.cbData = g_cbPage;
        snPatch.dbid = (JET_DBID)ifmp;

        FMP::EnterFMPPoolAsWriter();
        err = ErrDBSetUserDbHeaderInfo( &(g_rgfmp[ifmp]), sizeof( snPatch.dbinfomisc ), &snPatch.dbinfomisc );
        FMP::LeaveFMPPoolAsWriter();
        CallS( err );
        CallR( err );

        const HRT hrtStart = HrtHRTCount();
        TRY
        {
            err = (*m_pinst->m_pfnInitCallback)( JET_snpExternalAutoHealing, JET_sntPagePatchRequest, &snPatch, m_pinst->m_pvInitCallbackContext );
        }
        EXCEPT( efaExecuteHandler )
        {
            err = ErrERRCheck( JET_errCallbackFailed );
        }
        m_pinst->m_isdlInit.AddCallbackTime( DblHRTElapsedTimeFromHrtStart( hrtStart ) );

        if ( err != JET_errSuccess )
        {
            ErrERRCheck( err );
        }
    }
    
    return JET_errSuccess;
}


PCWSTR LOG::WszLGGetDefaultExt( BOOL fChk )
{
    if ( fChk )
    {
        return (UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames) ? wszOldChkExt : wszNewChkExt;
    }
    else
    {
        return (UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames) ? wszOldLogExt : wszNewLogExt;
    }
}

PCWSTR LOG::WszLGGetOtherExt( BOOL fChk )
{
    if ( fChk )
    {
        return (UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames) ? wszNewChkExt : wszOldChkExt;
    }
    else
    {
        return (UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitESE98FileNames) ? wszNewLogExt : wszOldLogExt;
    }
}


BOOL LOG::FLGIsDefaultExt( BOOL fChk, PCWSTR wszExt )
{
    PCWSTR wszDefExt = WszLGGetDefaultExt( fChk );
    return ( 0 == _wcsicmp( wszDefExt, wszExt ) );
}

BOOL LOG::FLGIsLegacyExt( BOOL fChk, PCWSTR wszExt )
{
    if ( fChk )
    {
        return ( 0 == _wcsicmp( wszOldChkExt, wszExt ) );
    }
    else
    {
        return ( 0 == _wcsicmp( wszOldLogExt, wszExt ) );
    }
}

void LOG::LGSetChkExts(
    BOOL fLegacy, BOOL fReset
    )
{
    Assert( !m_fLogInitialized ||
            (m_fRecoveringMode == fRecoveringNone) ||
            m_wszChkExt != NULL );
    Assert( fReset ||
            m_wszChkExt == NULL );

    m_wszChkExt = (fLegacy) ? wszOldChkExt : wszNewChkExt;
}

BOOL LOG::FWaypointLatencyEnabled() const
{
    if ( UlParam( m_pinst, JET_paramWaypointLatency ) == 0 )
    {
        return fFalse;
    }

    if ( m_pinst->m_fNoWaypointLatency )
    {
        return fFalse;
    }

    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp >= g_ifmpMax )
            continue;

        if ( g_rgfmp[ ifmp ].m_fNoWaypointLatency )
        {
            return fFalse;
        }
    }

    return fTrue;
}

LONG LOG::LLGElasticWaypointLatency() const
{
    LONG lElasticWaypointDepth;
    if ( FWaypointLatencyEnabled() && UlParam( m_pinst, JET_paramFlight_ElasticWaypointLatency ) )
    {
        lElasticWaypointDepth = (LONG)UlParam( m_pinst, JET_paramWaypointLatency ) + (LONG)UlParam( m_pinst, JET_paramFlight_ElasticWaypointLatency );
    }
    else
    {
        lElasticWaypointDepth = FWaypointLatencyEnabled() ? (LONG)UlParam( m_pinst, JET_paramWaypointLatency ) : 0;
    }

    return lElasticWaypointDepth;
}

VOID LOG::LGIGetEffectiveWaypoint(
    __in    IFMP    ifmp,
    __in    LONG    lGenCommitted,
    __out   LGPOS * plgposEffWaypoint )
{
    LGPOS lgposBestWaypoint;

    Assert( plgposEffWaypoint );

#if DEBUG
    LGPOS lgposCurrentWaypoint;
    lgposCurrentWaypoint = g_rgfmp[ ifmp ].LgposWaypoint();
#endif


    BFGetBestPossibleWaypoint( ifmp, lGenCommitted, &lgposBestWaypoint );
    Assert( lgposBestWaypoint.ib == lgposMax.ib );
    Assert( lgposBestWaypoint.isec == lgposMax.isec );
    Assert( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint ) >= 0 );

    *plgposEffWaypoint = lgposBestWaypoint;
}


ERR LOG::ErrLGUpdateGenRequired(
    IFileSystemAPI * const  pfsapi,
    const LONG              lGenMinRequired,
    const LONG              lGenMinConsistent,
    const LONG              lGenCommitted,
    const LOGTIME           logtimeGenMaxCreate,
    BOOL * const            pfSkippedAttachDetach,
    const IFMP              ifmpTarget
    )
{
    ERR                     errRet          = JET_errSuccess;
    LOGTIME                 tmEmpty;
    const BOOL fUpdateGenMaxRequired = ( lGenCommitted != 0 );
    LONG                    lGenMaxRequiredMin = ( ifmpTarget == ifmpNil ) ? lMax : 0;
    memset( &tmEmpty, '\0', sizeof( LOGTIME ) );

    Assert( m_critCheckpoint.FOwner() );

    Assert( lGenMinRequired <= lGenCommitted || lGenCommitted == 0 );
    Assert( lGenMinConsistent <= lGenCommitted || lGenMinConsistent == 0 );
    Assert( lGenMinRequired <= lGenMinConsistent || lGenMinRequired == 0 || lGenMinConsistent == 0 );

    if ( pfSkippedAttachDetach )
    {
        *pfSkippedAttachDetach = fFalse;
    }

    if ( lGenCommitted != 0 )
    {
        (VOID)m_MaxRequiredMap.ErrAddLogtimeMapping( lGenCommitted, &logtimeGenMaxCreate );
    }

    FMP::EnterFMPPoolAsReader();

    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        ERR err = JET_errSuccess;
        IFMP ifmp;

        ifmp = m_pinst->m_mpdbidifmp[ dbidT ];
        if ( ifmp >= g_ifmpMax )
            continue;

        if ( ifmpTarget != ifmpNil &&
            ifmp != ifmpTarget )
            continue;

        FMP         *pfmpT          = &g_rgfmp[ ifmp ];

        if ( pfmpT->FReadOnlyAttach() )
            continue;

        pfmpT->RwlDetaching().EnterAsReader();

        
        Assert( ( !pfmpT->FSkippedAttach() && !pfmpT->FDeferredAttach() ) || !pfmpT->FAttached() );

        if (   m_pinst->m_mpdbidifmp[ dbidT ] >= g_ifmpMax
            || !pfmpT->FLogOn()
            || pfmpT->FSkippedAttach() || pfmpT->FDeferredAttach() || !pfmpT->FAttached()
            || !pfmpT->Pdbfilehdr() )
        {
            pfmpT->RwlDetaching().LeaveAsReader();
            continue;
        }


        if ( !pfmpT->FAllowHeaderUpdate() )
        {
            if ( pfSkippedAttachDetach )
            {
                *pfSkippedAttachDetach = fTrue;
            }
            pfmpT->RwlDetaching().LeaveAsReader();
            continue;
        }

        LGPOS lgposBestWaypoint;
        LGIGetEffectiveWaypoint( ifmp, lGenCommitted, &lgposBestWaypoint );
        Assert( CmpLgpos( lgposBestWaypoint, pfmpT->LgposWaypoint() ) >= 0 );


        BOOL fPreHeaderUpdateMinRequired, fPreHeaderUpdateMinConsistent, fDidPreDbFlush;
        fPreHeaderUpdateMinRequired = fPreHeaderUpdateMinConsistent = fDidPreDbFlush = fFalse;
        {
        PdbfilehdrReadOnly pdbfilehdr = pfmpT->Pdbfilehdr();


        if ( lGenMinRequired )
        {
            const LONG lgenMinRequiredFuture = max( max( lGenMinRequired, pdbfilehdr->le_lgposAttach.le_lGeneration ), pdbfilehdr->le_lGenMinRequired );
            fPreHeaderUpdateMinRequired = ( pdbfilehdr->le_lGenMinRequired != lgenMinRequiredFuture );

            const LONG lgenMinConsistentFuture = max( max( lGenMinConsistent, pdbfilehdr->le_lgposAttach.le_lGeneration ), pdbfilehdr->le_lGenMinConsistent );
            fPreHeaderUpdateMinConsistent = ( pdbfilehdr->le_lGenMinConsistent != lgenMinConsistentFuture );
            }

            if ( err >= JET_errSuccess && fPreHeaderUpdateMinRequired )
            {
                Assert( pfmpT == &g_rgfmp[ifmp] );
                err = ErrIOFlushDatabaseFileBuffers( ifmp, iofrDbUpdMinRequired );
                fDidPreDbFlush = ( err >= JET_errSuccess );
                if ( err < JET_errSuccess )
                {
                    goto FailedSkipDatabase;
                }
            }
        }

        BOOL fHeaderUpdateMaxRequired, fHeaderUpdateMaxCommitted, fHeaderUpdateMinRequired, fHeaderUpdateMinConsistent;
        fHeaderUpdateMaxRequired = fHeaderUpdateMaxCommitted = fHeaderUpdateMinRequired = fHeaderUpdateMinConsistent = fFalse;
        BOOL fHeaderUpdateGenMaxCreateTime = fFalse;
        BOOL fLogtimeGenMaxRequiredEfvEnabled = ( pfmpT->ErrDBFormatFeatureEnabled( JET_efvLogtimeGenMaxRequired ) == JET_errSuccess );

        {
        PdbfilehdrReadWrite pdbfilehdr = pfmpT->PdbfilehdrUpdateable();

#ifdef DEBUG
        BYTE * pdbfilehdrPreimage = NULL;
        BFAlloc( bfasTemporary, (void**)&pdbfilehdrPreimage, g_cbPage );
        memcpy( pdbfilehdrPreimage, pdbfilehdr, g_cbPage );
        Assert( 0 == memcmp( pdbfilehdrPreimage, pdbfilehdr, g_cbPage ) );
#endif

        lGenMaxRequiredMin = min( lGenMaxRequiredMin, pdbfilehdr->le_lGenMaxRequired );

        if ( fUpdateGenMaxRequired )
        {
            const LONG  lGenMaxRequiredOld  = pdbfilehdr->le_lGenMaxRequired;

            
            pdbfilehdr->le_lGenMaxRequired = max( lgposBestWaypoint.lGeneration, lGenMaxRequiredOld );
            Assert ( lGenMaxRequiredOld <= pdbfilehdr->le_lGenMaxRequired );

            const LONG  lGenCommittedOld    = pdbfilehdr->le_lGenMaxCommitted;

#ifdef DEBUG
            Assert( m_fIgnoreLostLogs
                || lGenCommittedOld != lGenCommitted
                || 0 == lGenCommittedOld
                || 0 == memcmp( &pdbfilehdr->logtimeGenMaxCreate, &tmEmpty, sizeof( LOGTIME ) )
                || 0 == memcmp( &pdbfilehdr->logtimeGenMaxCreate, &logtimeGenMaxCreate, sizeof( LOGTIME ) ) );
#endif

            pdbfilehdr->le_lGenMaxCommitted = max( lGenCommitted, lGenCommittedOld );

            fHeaderUpdateMaxRequired = ( lGenMaxRequiredOld != pdbfilehdr->le_lGenMaxRequired );
            fHeaderUpdateMaxCommitted = ( lGenCommittedOld  != pdbfilehdr->le_lGenMaxCommitted );

            if ( fHeaderUpdateMaxRequired )
            {
                AssertTrack( m_lgposFlushTip.lGeneration >= pdbfilehdr->le_lGenMaxRequired, "GenMaxRequiredBeyondFlushTip" );
                if ( fLogtimeGenMaxRequiredEfvEnabled )
                {
                    if ( lGenCommitted == pdbfilehdr->le_lGenMaxRequired )
                    {
                        memcpy( &pdbfilehdr->logtimeGenMaxRequired, &logtimeGenMaxCreate, sizeof( LOGTIME ) );
                    }
                    else
                    {
                        m_MaxRequiredMap.LookupLogtimeMapping( pdbfilehdr->le_lGenMaxRequired, &pdbfilehdr->logtimeGenMaxRequired );
                    }
                }
                else
                {
                    memcpy( &pdbfilehdr->logtimeGenMaxRequired, &tmEmpty, sizeof( LOGTIME ) );
                }
            }

            Assert( !fHeaderUpdateMaxCommitted || pdbfilehdr->le_lGenMaxCommitted == lGenCommitted );
            if ( pdbfilehdr->le_lGenMaxCommitted == lGenCommitted )
            {
                LOGTIME logtimeGenMaxCreateOld;
                memcpy( &logtimeGenMaxCreateOld, &pdbfilehdr->logtimeGenMaxCreate, sizeof( LOGTIME ) );

                memcpy( &pdbfilehdr->logtimeGenMaxCreate, &logtimeGenMaxCreate, sizeof( LOGTIME ) );

                fHeaderUpdateGenMaxCreateTime = ( 0 != memcmp( &logtimeGenMaxCreateOld, &pdbfilehdr->logtimeGenMaxCreate, sizeof( LOGTIME ) ) );

                Assert( fHeaderUpdateGenMaxCreateTime == fFalse ||
                        fHeaderUpdateMaxCommitted ||
                        0 == memcmp( &logtimeGenMaxCreateOld, &tmEmpty, sizeof( LOGTIME ) ) ||
                        m_fRecoveringMode == fRecoveringRedo ||
                        ( m_fRecoveringMode == fRecoveringUndo && !m_fRecoveryUndoLogged )  );
            }
        }

        if ( lGenMinRequired )
        {
            const LONG  lGenMinRequiredOld          = pdbfilehdr->le_lGenMinRequired;
            const LONG  lGenMinRequiredCandidate    = max( lGenMinRequired, pdbfilehdr->le_lgposAttach.le_lGeneration );


            pdbfilehdr->le_lGenMinRequired = max( lGenMinRequiredCandidate, lGenMinRequiredOld );

            fHeaderUpdateMinRequired = ( lGenMinRequiredOld != pdbfilehdr->le_lGenMinRequired );
        }

        if ( lGenMinConsistent )
        {
            const LONG  lGenMinConsistentOld        = pdbfilehdr->le_lGenMinConsistent;
            const LONG  lGenMinConsistentCandidate  = max( lGenMinConsistent, pdbfilehdr->le_lgposAttach.le_lGeneration );

            pdbfilehdr->le_lGenMinConsistent = max( lGenMinConsistentCandidate, lGenMinConsistentOld );

            fHeaderUpdateMinConsistent = ( lGenMinConsistentOld != pdbfilehdr->le_lGenMinConsistent );
        }

        if ( fHeaderUpdateMinRequired &&
             ( pdbfilehdr->le_lGenPreRedoMinRequired != 0 ) &&
             ( pdbfilehdr->le_lGenMinRequired >= pdbfilehdr->le_lGenPreRedoMinRequired ) )
        {
            pdbfilehdr->le_lGenPreRedoMinRequired = 0;
        }

        if ( fHeaderUpdateMinConsistent &&
             ( pdbfilehdr->le_lGenPreRedoMinConsistent != 0 ) &&
             ( pdbfilehdr->le_lGenMinConsistent >= pdbfilehdr->le_lGenPreRedoMinConsistent ) )
        {
            pdbfilehdr->le_lGenPreRedoMinConsistent = 0;
        }

        const BOOL fHeaderUpdated = fHeaderUpdateMaxRequired || fHeaderUpdateMaxCommitted ||
                                    fHeaderUpdateMinRequired || fHeaderUpdateMinConsistent ||
                                    fHeaderUpdateGenMaxCreateTime;
#ifdef DEBUG
        Assert( fHeaderUpdated || 0 == memcmp( pdbfilehdrPreimage, pdbfilehdr, g_cbPage ) );
        BFFree( pdbfilehdrPreimage );
#endif

        }

        if ( err >= JET_errSuccess && fHeaderUpdateMinRequired )
        {
            Assert( fDidPreDbFlush );
        }
        else
        {
            Expected( !fDidPreDbFlush );
        }


        if ( err >= JET_errSuccess )
        {
            err = ErrUtilWriteAttachedDatabaseHeaders( m_pinst, pfsapi, pfmpT->WszDatabaseName(), pfmpT, pfmpT->Pfapi() );

            if ( err >= JET_errSuccess && ( fHeaderUpdateMinRequired || fHeaderUpdateMaxRequired ) )
            {
                const IOFLUSHREASON iofr =
                    IOFLUSHREASON(
                        ( fHeaderUpdateMinRequired ? iofrDbHdrUpdMinRequired : 0 ) |
                        ( fHeaderUpdateMaxRequired ? iofrDbHdrUpdMaxRequired : 0 ) );
                err = ErrIOFlushDatabaseFileBuffers( ifmp, iofr );
            }
            
            PdbfilehdrReadOnly pdbfilehdr = pfmpT->Pdbfilehdr();
            const LE_LGPOS * ple_lgposGreater = ( CmpLgpos( pdbfilehdr->le_lgposAttach, pdbfilehdr->le_lgposLastReAttach ) > 0 ) ?
                                                    &pdbfilehdr->le_lgposAttach : &pdbfilehdr->le_lgposLastReAttach;

            OSTrace( JET_tracetagCheckpointUpdate, OSFormat( "Writing DB header { %08x.%04x.%04x - %d / %d - %d / %d }",
                (LONG)ple_lgposGreater->le_lGeneration, (USHORT)ple_lgposGreater->le_isec, (USHORT)ple_lgposGreater->le_ib,
                (LONG)pdbfilehdr->le_lGenMinConsistent, (LONG)pdbfilehdr->le_lGenMinRequired, (LONG)pdbfilehdr->le_lGenMaxRequired, (LONG)pdbfilehdr->le_lGenMaxCommitted ) );
        }


        if ( JET_errSuccess <= err && fUpdateGenMaxRequired )
        {
            const LONG lgenMaxRequired = pfmpT->Pdbfilehdr()->le_lGenMaxRequired;
            pfmpT->SetWaypoint( lgenMaxRequired );
        }

    FailedSkipDatabase:

        pfmpT->RwlDetaching().LeaveAsReader();

        if ( errRet == JET_errSuccess )
            errRet = err;
    }

    FMP::LeaveFMPPoolAsReader();

    if ( lGenMaxRequiredMin != 0 && lGenMaxRequiredMin != lMax )
    {
        m_MaxRequiredMap.TrimLogtimeMapping( lGenMaxRequiredMin );
    }

    return errRet;
}




ERR LOG::ErrLGLockCheckpointAndUpdateGenRequired( const LONG lGenCommitted, const LOGTIME logtimeGenMaxCreate )
{
    m_critCheckpoint.Enter();
    Assert( lGenCommitted != 0 );

    const ERR err = ErrLGUpdateGenRequired(
                        m_pinst->m_pfsapi,
                        0,
                        0,
                        lGenCommitted,
                        logtimeGenMaxCreate,
                        NULL );
    m_critCheckpoint.Leave();

    return err;
}






VOID
LOG::LGLoadAttachmentsFromFMP(
    LGPOS lgposNext,
    _Out_bytecap_c_( 2048 ) BYTE *pbBuf )
{
    ATTACHINFO  *pattachinfo    = (ATTACHINFO*)pbBuf;

    FMP::EnterFMPPoolAsReader();

    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        IFMP ifmp = m_pinst->m_mpdbidifmp[ dbidT ];

        if ( ifmp >= g_ifmpMax )
            continue;

        FMP         *pfmpT          = &g_rgfmp[ ifmp ];
        ULONG       cbNames;
        const BOOL  fUsePatchchk    = ( pfmpT->Patchchk()
                                        && m_fRecovering
                                        && m_fRecoveringMode == fRecoveringRedo );

        pfmpT->RwlDetaching().EnterAsReader();
        if ( pfmpT->FLogOn() &&
             NULL != pfmpT->Pdbfilehdr() &&
             ( ( !fUsePatchchk &&
                 0 != CmpLgpos( lgposMin, pfmpT->LgposAttach() ) &&
                 CmpLgpos( lgposNext, pfmpT->LgposAttach() ) > 0 &&
                 ( 0 == CmpLgpos( lgposMin, pfmpT->LgposDetach() ) ||
                   CmpLgpos( lgposNext, pfmpT->LgposDetach() ) <= 0 ) ) ||
               ( fUsePatchchk &&
                 0 != CmpLgpos( lgposMin, pfmpT->Patchchk()->lgposAttach ) &&
                 CmpLgpos( lgposNext, pfmpT->Patchchk()->lgposAttach ) > 0 &&
                 CmpLgpos( lgposNext, pfmpT->Patchchk()->lgposConsistent ) > 0 ) ) )
        {
            EnforceSz( pbBuf + cbAttach > (BYTE *)pattachinfo + sizeof(ATTACHINFO), "TooManyDbsForAttachInfoBefore" );

            memset( pattachinfo, 0, sizeof(ATTACHINFO) );

            Assert( !pfmpT->FVersioningOff() );
            Assert( !pfmpT->FReadOnlyAttach() );

            pattachinfo->SetDbid( dbidT );

            Assert( pfmpT->FLogOn() );
            Assert( !pfmpT->FReadOnlyAttach() );

            if ( fUsePatchchk )
            {
                pattachinfo->SetDbtime( max( pfmpT->Patchchk()->Dbtime(), pfmpT->DbtimeCurrentDuringRecovery() ) );
                pattachinfo->SetCpgDatabaseSizeMax( pfmpT->Patchchk()->CpgDatabaseSizeMax() );
                pattachinfo->le_lgposAttach = pfmpT->Patchchk()->lgposAttach;
                pattachinfo->le_lgposConsistent = pfmpT->Patchchk()->lgposConsistent;
                UtilMemCpy( &pattachinfo->signDb, &pfmpT->Patchchk()->signDb, sizeof( SIGNATURE ) );
            }
            else
            {
                PdbfilehdrReadOnly pdbfilehdr = pfmpT->Pdbfilehdr();
                pattachinfo->SetDbtime( pfmpT->DbtimeLast() );
                pattachinfo->SetCpgDatabaseSizeMax( pfmpT->CpgDatabaseSizeMax() );
                pattachinfo->le_lgposAttach = pfmpT->LgposAttach();
                if ( 0 == memcmp( &pdbfilehdr->signLog, &m_signLog, sizeof(SIGNATURE) ) &&
                     CmpLgpos( lgposNext, pdbfilehdr->le_lgposConsistent ) > 0 )
                {
                    pattachinfo->le_lgposConsistent = pdbfilehdr->le_lgposConsistent;
                }
                else
                {
                    pattachinfo->le_lgposConsistent = lgposMin;
                }
                UtilMemCpy( &pattachinfo->signDb, &pdbfilehdr->signDb, sizeof( SIGNATURE ) );
            }
            pattachinfo->SetObjidLast( pfmpT->ObjidLast() );
            if ( pfmpT->FTrimSupported() )
            {
                pattachinfo->SetFSparseEnabledFile();
            }

            WCHAR * wszDatabaseName = pfmpT->WszDatabaseName();
            Assert ( wszDatabaseName );

            if ( m_fRecovering )
            {
                if ( !FDefaultParam( m_pinst, JET_paramAlternateDatabaseRecoveryPath ) )
                {
                    wszDatabaseName += LOSStrLengthW( wszDatabaseName ) + 1;
                }
                else
                {
                    WCHAR *wszRstmapDbName;
                    INT irstmap = IrstmapSearchNewName( wszDatabaseName, &wszRstmapDbName );
                    if ( irstmap >= 0 )
                    {
                        wszDatabaseName = wszRstmapDbName;
                    }
                }
            }
            Assert( wszDatabaseName );

            pattachinfo->SetFUnicodeNames();

            cbNames = (ULONG)( sizeof(WCHAR) * ( LOSStrLengthW( wszDatabaseName ) + 1 ) );

            EnforceSz( pbBuf + cbAttach >= (BYTE *)pattachinfo + sizeof(ATTACHINFO) + cbNames, "TooManyDbsForAttachInfoAfter" );

            memcpy( pattachinfo->szNames, wszDatabaseName, cbNames );
            pattachinfo->SetCbNames( (USHORT)cbNames );

            pattachinfo = (ATTACHINFO*)( (BYTE *)pattachinfo + sizeof(ATTACHINFO) + cbNames );
        }
        pfmpT->RwlDetaching().LeaveAsReader();
    }

    FMP::LeaveFMPPoolAsReader();

    
    *(BYTE *)pattachinfo = 0;

    Assert( pbBuf + cbAttach > (BYTE *)pattachinfo );
}


ERR LOG::ErrLGLoadFMPFromAttachments( BYTE *pbAttach )
{
    ERR                             err             = JET_errSuccess;
    const ATTACHINFO                *pattachinfo    = NULL;
    const BYTE                      *pbT;

    for ( pbT = pbAttach; 0 != *pbT; pbT += sizeof(ATTACHINFO) + pattachinfo->CbNames() )
    {
        Assert( pbT - pbAttach < cbAttach );
        pattachinfo = (ATTACHINFO*)pbT;

        CallR ( ErrLGRIPreSetupFMPFromAttach( ppibSurrogate, pattachinfo ) );
    }

    for ( pbT = pbAttach; 0 != *pbT; pbT += sizeof(ATTACHINFO) + pattachinfo->CbNames() )
    {
        Assert( pbT - pbAttach < cbAttach );
        pattachinfo = (ATTACHINFO*)pbT;

        CallR ( ErrLGRISetupFMPFromAttach( ppibSurrogate, pattachinfo ) );
    }

    return JET_errSuccess;
}

VOID LOG::LGFullNameCheckpoint( __out_bcount(OSFSAPI_MAX_PATH * sizeof(WCHAR) ) PWSTR wszFullName )
{
    Assert( m_wszChkExt != NULL );
    CallS( m_pinst->m_pfsapi->ErrPathBuild( SzParam( m_pinst, JET_paramSystemPath ), SzParam( m_pinst, JET_paramBaseName ), m_wszChkExt, wszFullName ) );
}

void LOG::LGISetInitialGen( __in const LONG lgenStart )
{
    Assert( m_pcheckpoint );
    Assert( lgenStart );

    m_lgenInitial = lgenStart;

    Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration <= m_lgenInitial );

    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = m_lgenInitial; 
    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec = USHORT( sizeof( LGFILEHDR ) / m_pLogStream->CbSec() );
    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib = 0;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = m_pcheckpoint->checkpoint.le_lgposCheckpoint;

    PERFOpt( ibLGCheckpoint.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) ) );
    PERFOpt( ibLGDbConsistency.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposDbConsistency, lgposMin ) ) );
}

ERR LOG::ErrLGICheckpointInit( BOOL *pfGlobalNewCheckpointFile )
{
    ERR         err;
    IFileAPI *  pfapiCheckpoint = NULL;
    WCHAR   wszPathJetChkLog[IFileSystemAPI::cchPathMax];
    PCWSTR  wszChkExt = WszLGGetDefaultExt( fTrue );

    *pfGlobalNewCheckpointFile = fFalse;

    Assert( m_pcheckpoint == NULL );
    Alloc( m_pcheckpoint = (CHECKPOINT *)PvOSMemoryPageAlloc( sizeof(CHECKPOINT), NULL ) );

    m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposMin;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = lgposMin;

TryAnotherExtension:
    CallS( m_pinst->m_pfsapi->ErrPathBuild( SzParam( m_pinst, JET_paramSystemPath ), SzParam( m_pinst, JET_paramBaseName ), m_pLogStream->LogExt() ? m_pLogStream->LogExt() : wszChkExt, wszPathJetChkLog ) );

    err = CIOFilePerf::ErrFileOpen(
                            m_pinst->m_pfsapi,
                            m_pinst,
                            wszPathJetChkLog,
                            BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone,
                            iofileOther,
                            QwInstFileID( qwCheckpointFileID, m_pinst->m_iInstance, 0 ),
                            &pfapiCheckpoint );
    if ( err == JET_errFileNotFound )
    {
        if ( m_pLogStream->LogExt() == NULL && FLGIsDefaultExt( fTrue, wszChkExt ) )
        {
            wszChkExt = WszLGGetOtherExt( fTrue );
            goto TryAnotherExtension;
        }
        *pfGlobalNewCheckpointFile = fTrue;
    }
    else
    {
        Call( err );

        if ( m_pLogStream->LogExt() == NULL )
        {
            Assert( !m_fHardRestore );
            LGSetChkExts( FLGIsLegacyExt( fTrue, wszChkExt ), fFalse);
            m_pLogStream->LGSetLogExt( FLGIsLegacyExt( fTrue, wszChkExt ), fFalse);
        }
        delete pfapiCheckpoint;
        pfapiCheckpoint = NULL;
    }

    m_fDisableCheckpoint = fFalse;
    err = JET_errSuccess;

HandleError:
    if ( err < 0 )
    {
        if ( m_pcheckpoint != NULL )
        {
            OSMemoryPageFree( m_pcheckpoint );
            m_pcheckpoint = NULL;
        }
    }

    Assert( !pfapiCheckpoint );
    return err;
}

VOID LOG::LGICheckpointTerm( VOID )
{

    m_critCheckpoint.Enter();

    Assert( m_fDisableCheckpoint );

    if ( m_pcheckpoint != NULL )
    {
        OSMemoryPageFree( m_pcheckpoint );
        m_pcheckpoint = NULL;
    }

    m_critCheckpoint.Leave();

    return;
}



ERR LOG::ErrLGReadCheckpoint( __in PCWSTR wszCheckpointFile, CHECKPOINT *pcheckpoint, const BOOL fReadOnly )
{
    ERR     err;
    BOOL    fReadRealCheckpoint = fFalse;

    if ( pcheckpoint == NULL )
    {
        fReadRealCheckpoint = fTrue;
        pcheckpoint = m_pcheckpoint;
    }

    m_critCheckpoint.Enter();

    err = ErrUtilReadShadowedHeader(
            m_pinst,
            m_pinst->m_pfsapi,
            wszCheckpointFile,
            (BYTE*)pcheckpoint,
            sizeof(CHECKPOINT),
            -1,
            UtilReadHeaderFlags( ( fReadOnly ? urhfReadOnly : urhfNone ) | urhfNoAutoDetectPageSize ) );
    
    if ( err < JET_errSuccess )
    {
        
        if ( JET_errFileNotFound == err )
        {
            err = ErrERRCheck( JET_errCheckpointFileNotFound );
        }
        else if ( FErrIsDbHeaderCorruption( err ) )
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"9f82b8e3-9bf3-417a-9692-3feb27850381" );
            err = ErrERRCheck( JET_errCheckpointCorrupt );
        }
    }
    else if ( m_fSignLogSet )
    {
        if ( memcmp( &m_signLog, &pcheckpoint->checkpoint.signLog, sizeof( m_signLog ) ) != 0 )
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"7f84ad7e-f173-4f25-9971-c2264abc0827" );
            err = ErrERRCheck( JET_errBadCheckpointSignature );
        }
    }

    Call( err );

    if( JET_filetypeUnknown != pcheckpoint->checkpoint.le_filetype
        && JET_filetypeCheckpoint != pcheckpoint->checkpoint.le_filetype )
    {
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"c0c9b597-c2d5-4dd4-a5e8-9904c00507df" );
        Call( ErrERRCheck( JET_errFileInvalidType ) );
    }

    if ( !CmpLgpos( pcheckpoint->checkpoint.le_lgposDbConsistency, lgposMin ) )
    {
        pcheckpoint->checkpoint.le_lgposDbConsistency = pcheckpoint->checkpoint.le_lgposCheckpoint;
    }

    if ( fReadRealCheckpoint )
    {
        m_pinst->m_pbackup->BKCopyLastBackupStateFromCheckpoint( &m_pcheckpoint->checkpoint );
        m_fLGFMPLoaded      = fTrue;
    }

    PERFOpt( ibLGCheckpoint.Set( m_pinst, m_pLogStream->CbOffsetLgpos( pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) ) );
    PERFOpt( ibLGDbConsistency.Set( m_pinst, m_pLogStream->CbOffsetLgpos( pcheckpoint->checkpoint.le_lgposDbConsistency, lgposMin ) ) );
    PERFOpt( cbLGCheckpointDepthMax.Set( m_pinst, UlParam( m_pinst, JET_paramCheckpointDepthMax ) ) );
    
HandleError:
    m_critCheckpoint.Leave();
    return err;
}



ERR LOG::ErrLGIWriteCheckpoint( __in PCWSTR wszCheckpointFile, const IOFLUSHREASON iofr, CHECKPOINT *pcheckpoint )
{
    ERR     err;

    Assert( m_critCheckpoint.FOwner() );
    Assert( !m_fDisableCheckpoint );
    Assert( CmpLgpos( pcheckpoint->checkpoint.le_lgposCheckpoint, pcheckpoint->checkpoint.le_lgposDbConsistency ) <= 0 );
    Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= 1 );
    Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= m_lgenInitial );

    Assert( m_fSignLogSet );
    pcheckpoint->checkpoint.signLog = m_signLog;

    pcheckpoint->checkpoint.le_filetype = JET_filetypeCheckpoint;


    err = ErrUtilWriteCheckpointHeaders( m_pinst,
                                         m_pinst->m_pfsapi,
                                         wszCheckpointFile,
                                         iofr,
                                         pcheckpoint );



    return err;
}

ERR LOG::ErrLGIUpdateCheckpointLgposForTerm( const LGPOS& lgposCheckpoint )
{
    ERR     err;
    WCHAR   wszPathJetChkLog[IFileSystemAPI::cchPathMax];


    m_critCheckpoint.Enter();

    Assert( !m_fDisableCheckpoint );

    LGFullNameCheckpoint( wszPathJetChkLog );

    m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = lgposCheckpoint;

    m_pcheckpoint->rgbAttach[0] = 0;
    m_pcheckpoint->checkpoint.fVersion &= ~fCheckpointAttachInfoPresent;

    OSTrace( JET_tracetagBufferManagerMaintTasks,
            OSFormat(   "ChkptUpdTerm: writing out explicit checkpoint %s",
                        OSFormatLgpos( lgposCheckpoint ) ) );

    err = ErrLGIWriteCheckpoint( wszPathJetChkLog, iofrLogTerm, m_pcheckpoint );
    m_critCheckpoint.Leave();

    return err;
}


VOID LOG::LGIUpdateCheckpoint( CHECKPOINT *pcheckpoint )
{
    PIB     *ppibT;
    LGPOS   lgposCheckpoint = lgposMax;
    LGPOS   lgposDbConsistency = lgposMax;

    LGPOS   lgposToFlushT;
    LGPOS   lgposLogRecT;
    LGPOS   lgposLowestStartT;
    BOOL    fAdvanceCheckpoint = fTrue;

    Assert( !m_fLogDisabled );
    Assert( m_critCheckpoint.FOwner() );

#ifdef DEBUG
    if ( m_fDBGFreezeCheckpoint )
        return;
#endif

    m_pLogWriteBuffer->LockBuffer();

    const LGPOS lgposWrittenTip = ( m_fRecoveringMode == fRecoveringRedo ) ? m_lgposRedo : m_pLogWriteBuffer->LgposWriteTip();

    lgposDbConsistency = lgposWrittenTip;
    lgposToFlushT = lgposWrittenTip;
    lgposLogRecT = m_pLogWriteBuffer->LgposLogTip();

    

    m_pLogWriteBuffer->UnlockBuffer();

    m_pinst->m_critPIB.Enter();
    for ( ppibT = m_pinst->m_ppibGlobal; ppibT != NULL; ppibT = ppibT->ppibNext )
    {
        Assert( levelNil != ppibT->Level() );
        if ( levelNil != ppibT->Level() )
        {
            ENTERCRITICALSECTION    enterCritLogBeginTrx( &ppibT->critLogBeginTrx );

            if ( ppibT->FBegin0Logged()
                && CmpLgpos( &ppibT->lgposStart, &lgposDbConsistency ) < 0 )
            {
                lgposDbConsistency = ppibT->lgposStart;
            }

            const LONG dlgenTattleThreshold = (LONG)UlParam( m_pinst, JET_paramCheckpointTooDeep ) / 4;
            const LONG lgenStart = *(volatile LONG *)( &ppibT->lgposStart.lGeneration );
            const LONG dlgenTransactionLength = lgposWrittenTip.lGeneration - lgenStart;
            if ( !m_pinst->m_plog->FRecovering() && 
                   ppibT->FBegin0Logged() && 
                   lgenStart != lGenerationInvalid &&
                   dlgenTransactionLength > dlgenTattleThreshold &&
                   ppibT->FCheckSetLoggedCheckpointGettingDeep() )
            {
                const LONG lgenStartRecheck = *(volatile LONG *)( &ppibT->lgposStart.lGeneration );
                if ( lgenStart != lgenStartRecheck )
                {
                    Assert( lgenStartRecheck == lGenerationInvalid );
                    ppibT->ResetLoggedCheckpointGettingDeep();
                }
                else
                {
                    WCHAR wszTrxLgens[30];
                    WCHAR wszTrxCtxInfo[50];
                    WCHAR wszTrxIdTimeStack[400];
                    const WCHAR * rgpwsz[] = { wszTrxLgens, wszTrxCtxInfo, wszTrxIdTimeStack };

                    OSStrCbFormatW( wszTrxLgens, sizeof( wszTrxLgens ), L"%d (0x%x)", dlgenTransactionLength, dlgenTransactionLength );
                    CallS( ppibT->TrxidStack().ErrDump( wszTrxIdTimeStack, _countof( wszTrxIdTimeStack ), L", " ) );
                    const UserTraceContext * const putc = ppibT->Putc();
                    OSStrCbFormatW( wszTrxCtxInfo, sizeof( wszTrxCtxInfo ), L"%d.%d.%d - %d(0x%x) - 0x%x", 
                                    (DWORD)putc->context.nClientType, (DWORD)putc->context.nOperationType, (DWORD)putc->context.nOperationID, 
                                    putc->context.dwUserID, putc->context.dwUserID,
                                    (DWORD)putc->context.fFlags );

                    UtilReportEvent(
                            eventWarning,
                            TRANSACTION_MANAGER_CATEGORY,
                            LOG_CHECKPOINT_GETTING_DEEP_ID,
                            _countof( rgpwsz ),
                            rgpwsz,
                            0,
                            NULL,
                            m_pinst );
                }
            }
        }
    }
    m_pinst->m_critPIB.Leave();

    lgposLowestStartT = ( 0 == CmpLgpos( lgposDbConsistency, lgposToFlushT ) ) ? lgposMax : lgposDbConsistency;

    

    fAdvanceCheckpoint = !m_fPendingRedoMapEntries;

    if ( !fAdvanceCheckpoint )
    {
        return;
    }

    FMP::EnterFMPPoolAsReader();

    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp < g_ifmpMax )
        {
            FMP* const pfmp = &g_rgfmp[ifmp];

            if ( pfmp->FSkippedAttach() && !m_fReplayingIgnoreMissingDB )
            {
                fAdvanceCheckpoint = fFalse;
                break;
            }

            if ( pfmp->FDeferredAttach() && !pfmp->FIgnoreDeferredAttach() && !m_fReplayingIgnoreMissingDB )
            {
                fAdvanceCheckpoint = fFalse;
                break;
            }

            if ( !pfmp->FAllowHeaderUpdate() && !pfmp->FSkippedAttach() && !pfmp->FDeferredAttach() )
            {
                fAdvanceCheckpoint = fFalse;
                break;
            }

            if ( FRecovering() && pfmp->FContainsDataFromFutureLogs() )
            {

                if ( !pfmp->FRedoMapsEmpty() )
                {
                    fAdvanceCheckpoint = fFalse;
                    break;
                }
            }

            LGPOS lgposOldestBegin0;
            LGPOS lgposWaypoint = lgposMax;

            BFGetLgposOldestBegin0( ifmp, &lgposOldestBegin0, lgposDbConsistency );
            if ( CmpLgpos( &lgposDbConsistency, &lgposOldestBegin0 ) > 0 )
            {
                lgposDbConsistency = lgposOldestBegin0;
            }

            if ( pfmp->FLogOn() )
            {
                lgposWaypoint   = pfmp->LgposWaypoint();


                if ( CmpLgpos( lgposDbConsistency, lgposWaypoint ) > 0
                    && 0 != CmpLgpos( lgposWaypoint, lgposMin ) )
                {
                    lgposDbConsistency = lgposWaypoint;
                }
            }

            CFlushMap* const pfm = pfmp->PFlushMap();
            LGPOS lgposFmMinRequiredT = lgposMin;
            if ( ( pfm != NULL ) && pfm->FRecoverable() )
            {
                lgposFmMinRequiredT.lGeneration = pfm->LGetFmGenMinRequired();
            }

    
            LGPOS lgposExistingCheckpointT = pcheckpoint->checkpoint.le_lgposCheckpoint;
            LGPOS lgposExistingDbConsistencyT = pcheckpoint->checkpoint.le_lgposDbConsistency;

            OSTraceFMP( ifmp, JET_tracetagBufferManagerMaintTasks,
                    OSFormat(   "CPUPD:========================================\r\n"
                                "CPUPD:                ifmp=%s\r\n"
                                "CPUPD:                dbid=%s\r\n"
                                "CPUPD:      existing chkpt=%s\r\n"
                                "CPUPD:     FM min required=%s\r\n"
                                "CPUPD:existing consistency=%s\r\n"
                                "CPUPD:        oldestBegin0=%s\r\n"
                                "CPUPD:  min(E(lgposStart))=%s\r\n"
                                "CPUPD:       lgposWaypoint=%s\r\n"
                                "CPUPD:      m_lgposToFlush=%s\r\n"
                                "CPUPD:       m_lgposLogRec=%s\r\n"
                                "CPUPD:========================================\r\n",
                                OSFormatUnsigned( ifmp ),
                                OSFormatUnsigned( dbid ),
                                OSFormatLgpos( lgposExistingCheckpointT ),
                                OSFormatLgpos( lgposFmMinRequiredT ),
                                OSFormatLgpos( lgposExistingDbConsistencyT ),
                                OSFormatLgpos( lgposOldestBegin0 ),
                                OSFormatLgpos( lgposLowestStartT ),
                                OSFormatLgpos( lgposWaypoint ),
                                OSFormatLgpos( lgposToFlushT ),
                                OSFormatLgpos( lgposLogRecT ) ) );

            if ( ppibNil != m_pinst->m_ppibGlobal )
            {
                pfmp->UpdateDbtimeOldest();
            }
        }
    }

    if ( !fAdvanceCheckpoint )
    {
        FMP::LeaveFMPPoolAsReader();
        return;
    }

    lgposCheckpoint = lgposDbConsistency;

    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp < g_ifmpMax )
        {
            FMP* const pfmp = &g_rgfmp[ifmp];
            CFlushMap* const pfm = pfmp->PFlushMap();
            LGPOS lgposFmMinRequired = lgposMin;
            if ( ( pfm != NULL ) && pfm->FRecoverable() )
            {
                lgposFmMinRequired.lGeneration = pfm->LGetFmGenMinRequired();
                lgposFmMinRequired.isec = (USHORT)m_pLogStream->CSecHeader();
                lgposFmMinRequired.ib = 0;

                if ( ( CmpLgpos( lgposCheckpoint, lgposFmMinRequired ) > 0 ) &&
                    ( lgposFmMinRequired.lGeneration > 0 ) )
                {
                    lgposCheckpoint = lgposFmMinRequired;
                }
            }
        }
    }

    FMP::LeaveFMPPoolAsReader();

    
    if ( CmpLgpos( &lgposCheckpoint, &pcheckpoint->checkpoint.le_lgposCheckpoint ) > 0 )
    {
        Assert( lgposCheckpoint.lGeneration != 0 );
        Assert( lgposCheckpoint.lGeneration != ( m_lgenInitial - 1 ) );
        if ( lgposCheckpoint.isec == 0 )
        {
            lgposCheckpoint.isec = (USHORT)m_pLogStream->CSecHeader();
        }
        pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;

        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: updating checkpoint to %s",
                            OSFormatLgpos( lgposCheckpoint ) ) );
    }
    else
    {
        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: leaving checkpoint at %s",
                            OSFormatLgpos( LGPOS( pcheckpoint->checkpoint.le_lgposCheckpoint ) ) ) );
    }

    
    if ( CmpLgpos( &lgposDbConsistency, &pcheckpoint->checkpoint.le_lgposDbConsistency ) > 0 )
    {
        Assert( lgposDbConsistency.lGeneration != 0 );
        Assert( lgposDbConsistency.lGeneration != ( m_lgenInitial - 1 ) );
        if ( lgposDbConsistency.isec == 0 )
        {
            lgposDbConsistency.isec = (USHORT)m_pLogStream->CSecHeader();
        }
        pcheckpoint->checkpoint.le_lgposDbConsistency = lgposDbConsistency;

        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: updating DB consistency LGPOS to %s",
                            OSFormatLgpos( lgposDbConsistency ) ) );
    }
    else
    {
        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: leaving DB consistency LGPOS at %s",
                            OSFormatLgpos( LGPOS( pcheckpoint->checkpoint.le_lgposDbConsistency ) ) ) );
    }

    
    m_pinst->SaveDBMSParams( &pcheckpoint->checkpoint.dbms_param );

{
    LONG lCurrentLogGeneration;

    m_pLogWriteBuffer->LockBuffer();
    lCurrentLogGeneration = m_pLogStream->GetCurrentFileGen();

    Assert ( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration <= lCurrentLogGeneration + 1 );
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = min ( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration, lCurrentLogGeneration );
    pcheckpoint->checkpoint.le_lgposDbConsistency.le_lGeneration = min ( pcheckpoint->checkpoint.le_lgposDbConsistency.le_lGeneration, lCurrentLogGeneration );

    m_pLogWriteBuffer->UnlockBuffer();
}

    if ( CmpLgpos( pcheckpoint->checkpoint.le_lgposCheckpoint, pcheckpoint->checkpoint.le_lgposDbConsistency ) > 0 )
    {
        pcheckpoint->checkpoint.le_lgposCheckpoint = pcheckpoint->checkpoint.le_lgposDbConsistency;
    }

    
    LGLoadAttachmentsFromFMP( pcheckpoint->checkpoint.le_lgposCheckpoint, pcheckpoint->rgbAttach );
    pcheckpoint->checkpoint.fVersion |= fCheckpointAttachInfoPresent;

    m_pinst->m_pbackup->BKCopyLastBackupStateToCheckpoint( &pcheckpoint->checkpoint );

    return;
}

BOOL LOG::FLGIUpdatableWaypoint( __in const LONG lGenCommitted, __in const IFMP ifmpTarget )
{
    BOOL    fUpdatableWaypoint = fFalse;

    Assert( !m_fLogDisabled );
    Assert( m_critCheckpoint.FOwner() );

    FMP::EnterFMPPoolAsReader();
    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        const IFMP  ifmp    = m_pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp >= g_ifmpMax )
            continue;

        if ( ifmpTarget != ifmpNil &&
            ifmp != ifmpTarget )
            continue;

        if ( g_rgfmp[ifmp].FLogOn() && g_rgfmp[ifmp].FAttached() )
        {
            const LGPOS lgposCurrentWaypoint    = g_rgfmp[ifmp].LgposWaypoint();


            if ( 0 != CmpLgpos( lgposCurrentWaypoint, lgposMin ) )
            {
                LGPOS   lgposBestWaypoint;

                BFGetBestPossibleWaypoint( ifmp, lGenCommitted, &lgposBestWaypoint );

                Assert( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint ) >= 0 );


                if ( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint ) > 0 )
                {
                    fUpdatableWaypoint = fTrue;
                    break;
                }
            }
        }
    }

    FMP::LeaveFMPPoolAsReader();

    return( fUpdatableWaypoint );
}


ERR LOG::ErrLGIUpdateCheckpointFile( const BOOL fForceUpdate, CHECKPOINT * pcheckpointT )
{
    ERR             err             = JET_errSuccess;
    LGPOS           lgposCheckpointT;
    LGPOS           lgposDbConsistencyT;
    BOOL            fCheckpointTAlloc = fFalse;
    WCHAR           wszPathJetChkLog[IFileSystemAPI::cchPathMax];

    Assert( m_critCheckpoint.FOwner() );

    if ( NULL == pcheckpointT )
    {
        AllocR( pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc(  sizeof( CHECKPOINT ), NULL ) );
        fCheckpointTAlloc = fTrue;
    }

    if ( m_fDisableCheckpoint
        || m_fLogDisabled
        || !m_fLGFMPLoaded
        || m_fLGNoMoreLogWrite
        || m_pinst->FInstanceUnavailable() )
    {
        err = JET_errSuccess;
        goto HandleError;
    }

    
    lgposCheckpointT = m_pcheckpoint->checkpoint.le_lgposCheckpoint;
    lgposDbConsistencyT = m_pcheckpoint->checkpoint.le_lgposDbConsistency;
    *pcheckpointT = *m_pcheckpoint;

    
    LGIUpdateCheckpoint( pcheckpointT );
    const BOOL fCheckpointUpdated =
        ( ( lgposCheckpointT.lGeneration < pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration ) ||
          ( lgposCheckpointT.isec < pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec ) ||
          ( lgposDbConsistencyT.lGeneration < pcheckpointT->checkpoint.le_lgposDbConsistency.le_lGeneration ) ||
          ( lgposDbConsistencyT.isec < pcheckpointT->checkpoint.le_lgposDbConsistency.le_isec ) );


    const BOOL fUpdatedCheckpointMinRequired =
        ( ( lgposCheckpointT.lGeneration < pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration ) ||
          ( lgposCheckpointT.isec < pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec ) );

    if ( fUpdatedCheckpointMinRequired && m_pinst->m_prbs && m_pinst->m_prbs->FInitialized() && !m_pinst->m_prbs->FInvalid() )
    {
        Call( m_pinst->m_prbs->ErrFlushAll() );
    }

    Assert( lgposCheckpointT.lGeneration <= pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration );

    
    const BOOL fUpdateWaypoint = FLGIUpdatableWaypoint( 0 );

    
    if ( fForceUpdate || fCheckpointUpdated || fUpdateWaypoint )
    {

        BOOL fSkippedAttachDetach;



        LOGTIME tmCreate;
        LONG lGenCurrent = LGGetCurrentFileGenWithLock( &tmCreate );
        Call( ErrLGUpdateGenRequired(
            m_pinst->m_pfsapi,
            pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration,
            pcheckpointT->checkpoint.le_lgposDbConsistency.le_lGeneration,
            lGenCurrent,
            tmCreate,
            &fSkippedAttachDetach,
            ifmpNil ) );

        if ( fSkippedAttachDetach )
        {
            Error( ErrERRCheck( errSkippedDbHeaderUpdate ) );
        }

        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: writing out checkpoint %s",
                            OSFormatLgpos( LGPOS( pcheckpointT->checkpoint.le_lgposCheckpoint ) ) ) );


        const IOFLUSHREASON iofr = IOFLUSHREASON( ( fUpdatedCheckpointMinRequired ? iofrCheckpointMinRequired : 0 ) |
                                                  ( fForceUpdate ? iofrCheckpointForceWrite : 0 ) );

        LGFullNameCheckpoint( wszPathJetChkLog );
        err = ErrLGIWriteCheckpoint( wszPathJetChkLog, iofr, pcheckpointT );
        if ( err < 0 )
        {
            if ( JET_errSuccess == m_errCheckpointUpdate
                || 0 != CmpLgpos( lgposCheckpointT, m_lgposCheckpointUpdateError ) )
            {
                m_errCheckpointUpdate = err;
                m_lgposCheckpointUpdateError = lgposCheckpointT;
            }

            goto HandleError;
        }
        else
        {
            *m_pcheckpoint = *pcheckpointT;
            PERFOpt( ibLGCheckpoint.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) ) );
            PERFOpt( ibLGDbConsistency.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposDbConsistency, lgposMin ) ) );
            PERFOpt( cbLGCheckpointDepthMax.Set( m_pinst, CbLGDesiredCheckpointDepth() ) );
        }
    }
    else
    {
        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: skipping checkpoint update for %d/%d/%d, leaving at %s",
                            fForceUpdate,
                            fCheckpointUpdated,
                            fUpdateWaypoint,
                            OSFormatLgpos( LGPOS( pcheckpointT->checkpoint.le_lgposCheckpoint ) ) ) );
    }

HandleError:
    if ( fCheckpointTAlloc )
    {
        OSMemoryPageFree( pcheckpointT );
    }
    return err;
}


ERR LOG::ErrLGUpdateCheckpointFile( const BOOL fForceUpdate )
{
    ERR             err             = JET_errSuccess;
    CHECKPOINT *    pcheckpointT;
    
    AllocR( pcheckpointT = (CHECKPOINT *)PvOSMemoryPageAlloc(  sizeof( CHECKPOINT ), NULL ) );

    m_critCheckpoint.Enter();

    err = ErrLGIUpdateCheckpointFile( fForceUpdate, pcheckpointT );

    m_critCheckpoint.Leave();
    OSMemoryPageFree( pcheckpointT );
    return err;
}


LGPOS LOG::LgposLGCurrentCheckpointMayFail() 
{
    LGPOS lgposRet = lgposMin;

    if ( m_critCheckpoint.FTryEnter() )
    {
        lgposRet = m_pcheckpoint->checkpoint.le_lgposCheckpoint;

        m_critCheckpoint.Leave();
    }

    return lgposRet;
}


LGPOS LOG::LgposLGCurrentDbConsistencyMayFail() 
{
    LGPOS lgposRet = lgposMin;

    if ( m_critCheckpoint.FTryEnter() )
    {
        lgposRet = m_pcheckpoint->checkpoint.le_lgposDbConsistency;

        m_critCheckpoint.Leave();
    }

    return lgposRet;
}


ERR LOG::ErrLGUpdateWaypointIFMP( IFileSystemAPI *const pfsapi, __in const IFMP ifmpTarget )
{
    ERR err = JET_errSuccess;
    BOOL fOwnsChkptCritSec = fFalse;
    LOGTIME tmCreate;
    memset( &tmCreate, 0, sizeof(LOGTIME) );

    Call( ErrLGFlush( iofrLogMaxRequired ) );

    m_critCheckpoint.Enter();
    fOwnsChkptCritSec = fTrue;

    if ( m_fDisableCheckpoint
        || m_fLogDisabled
        || m_fLGNoMoreLogWrite
        || m_pinst->FInstanceUnavailable() )
    {
        if ( m_fLGNoMoreLogWrite
            || m_pinst->FInstanceUnavailable() )
        {
            Error( ErrERRCheck( JET_errInstanceUnavailable ) );
        }
        else
        {
            err = JET_errSuccess;
            goto HandleError;
        }
    }

    const LONG lGenCommitted = LGGetCurrentFileGenWithLock( &tmCreate );

    const BOOL fUpdateWaypoint = FLGIUpdatableWaypoint( lGenCommitted, ifmpTarget );

    if ( !fUpdateWaypoint )
    {
        if ( ( !m_fRecovering || m_fRecoveringMode == fRecoveringUndo ) &&
            lGenCommitted < m_pLogWriteBuffer->LgposLogTip().lGeneration )
        {
            m_pLogWriteBuffer->FLGSignalWrite();
        }
        err = JET_errSuccess;
        goto HandleError;
    }

    BOOL fSkippedAttachDetach;

    Assert( lGenCommitted != 0 );
    Call( ErrLGUpdateGenRequired(
        pfsapi,
        0,
        0,
        lGenCommitted,
        tmCreate,
        &fSkippedAttachDetach,
        ifmpTarget ) );

    if ( fSkippedAttachDetach )
    {
        Assert( ifmpTarget == ifmpNil );
        Error( ErrERRCheck( JET_errInternalError ) );
    }

HandleError:
    if ( fOwnsChkptCritSec )
    {
        m_critCheckpoint.Leave();
        fOwnsChkptCritSec = fFalse;
    }
    return err;
}


ERR LOG::ErrLGQuiesceWaypointLatencyIFMP( __in const IFMP ifmpTarget )
{
    ERR err = JET_errSuccess;
    PIB pibFake;

    Assert( ifmpTarget != ifmpNil );
    FMP* const pfmp = g_rgfmp + ifmpTarget;

    if ( !pfmp->FLogOn() )
    {
        goto HandleError;
    }

    pibFake.m_pinst = m_pinst;
    Call( ErrLGWaitForWrite( &pibFake, &lgposMax ) );

    Assert( !pfmp->FNoWaypointLatency() );
    pfmp->SetNoWaypointLatency();
    Call( ErrLGUpdateWaypointIFMP( m_pinst->m_pfsapi, ifmpTarget ) );

HandleError:

    pfmp->ResetNoWaypointLatency();

    return err;
}

BOOL LOG::FLGRecoveryLgposStopLogGeneration(  ) const
{
    LGPOS lgposEndOfLog;

    if ( !FLGRecoveryLgposStop() )
    {
        return fFalse;
    }

    Assert( CmpLgpos( &m_lgposRecoveryStop, &lgposMax ) != 0 );


    lgposEndOfLog = lgposMax;
    lgposEndOfLog.lGeneration = m_pLogStream->GetCurrentFileGen();


    return ( CmpLgpos( &m_lgposRecoveryStop, &lgposEndOfLog ) == 0 );
}

BOOL LOG::FLGRecoveryLgposStop( ) const
{
    Assert( 0 != CmpLgpos( &m_lgposRecoveryStop, &lgposMin ) );

    if ( CmpLgpos( &m_lgposRecoveryStop, &lgposMax ) != 0 )
    {
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}

BOOL LOG::FLGRecoveryLgposStopLogRecord( const LGPOS &lgpos ) const
{

    if ( !FLGRecoveryLgposStop() )
    {
        return fFalse;
    }

    if ( m_lgposRecoveryStop.isec == 0 && m_lgposRecoveryStop.ib == 0 )
    {
        return fFalse;
    }

    return ( CmpLgpos( &m_lgposRecoveryStop, &lgpos ) < 0 );
}


ULONG LOG::CbLGSec() const
{
    return m_pLogStream->CbSec();
}

ULONG LOG::CSecLGFile() const
{
    return m_pLogStream->CSecLGFile();
}

VOID LOG::SetCSecLGFile( ULONG csecLGFile )
{
    m_pLogStream->SetCSecLGFile( csecLGFile );
}

ULONG LOG::CSecLGHeader() const
{
    return m_pLogStream->CSecHeader();
}

VOID LOG::LGWriteTip( LGPOS *plgpos )
{
    LGWriteAndFlushTip( plgpos, NULL );
}

VOID LOG::LGFlushTip( LGPOS *plgpos )
{
    LGWriteAndFlushTip( NULL, plgpos );
}

VOID LOG::LGWriteAndFlushTip( LGPOS *plgposWrite, LGPOS *plgposFlush )
{
    m_pLogWriteBuffer->LockBuffer();

    if ( plgposWrite )
    {
        if ( m_fRecovering && m_fRecoveringMode == fRecoveringRedo )
        {
            *plgposWrite = m_lgposRedo;
        }
        else
        {
            *plgposWrite = m_pLogWriteBuffer->LgposWriteTip();
        }
    }

    if ( plgposFlush )
    {
        *plgposFlush = m_lgposFlushTip;
    }

    m_pLogWriteBuffer->UnlockBuffer();
}

VOID LOG::LGSetFlushTip( const LGPOS &lgpos )
{
    m_pLogWriteBuffer->LockBuffer();
    LGSetFlushTipWithLock( lgpos );
    m_pLogWriteBuffer->UnlockBuffer();
}

VOID LOG::LGSetFlushTipWithLock( const LGPOS &lgpos )
{
#ifdef DEBUG
    Assert( m_pLogWriteBuffer->FOwnsBufferLock() );

    Assert( ( m_fRecovering && ( m_fRecoveringMode == fRecoveringRedo ) ) || 
            ( CmpLgpos( lgpos, m_pLogWriteBuffer->LgposWriteTip() ) <= 0 ) );
#endif

    if ( CmpLgpos( lgpos, m_lgposFlushTip ) > 0 )
    {
        m_lgposFlushTip = lgpos;
    }
}

VOID LOG::LGResetFlushTipWithLock( const LGPOS &lgpos )
{
#ifdef DEBUG
    Assert( m_pLogWriteBuffer->FOwnsBufferLock() );
    Expected( ( !m_fRecovering &&
                !m_pinst->m_fJetInitialized &&
                ( CmpLgpos( lgpos, lgposMin ) == 0 ) ) ||
              ( ( m_fRecovering &&
                ( m_fRecoveringMode == fRecoveringRedo ) &&
                ( CmpLgpos( lgpos, m_lgposFlushTip ) < 0 ) &&
                ( CmpLgpos( lgpos, m_lgposRedo ) == 0 ) ) ) );
    if ( m_fRecovering && m_fRecoveringMode == fRecoveringRedo )
    {
        Assert( lgpos.lGeneration <= m_lgposRedo.lGeneration ||
                ( lgpos.lGeneration == m_lgposRedo.lGeneration + 1 &&
                  lgpos.isec == m_pLogStream->CSecHeader() &&
                  lgpos.ib == 0 ) );
        Assert( ( ( m_lgposFlushTip.lGeneration == lgpos.lGeneration ) &&
                  ( m_lgposFlushTip.isec == lgposMax.isec ) &&
                  ( m_lgposFlushTip.ib == lgposMax.ib ) ) ||
                ( ( m_lgposFlushTip.lGeneration == lgpos.lGeneration + 1 ) &&
                  ( m_lgposFlushTip.isec == m_pLogStream->CSecHeader() ) &&
                  ( m_lgposFlushTip.ib == 0 ) ) );
    }
    else
    {
        Assert( CmpLgpos( lgpos, m_pLogWriteBuffer->LgposWriteTip() ) <= 0 );
    }
#endif

    m_lgposFlushTip = lgpos;
}

LGPOS LOG::LgposLGLogTipNoLock() const
{
    if ( m_fRecovering &&
        ( ( m_fRecoveringMode == fRecoveringRedo ) ||
            ( ( m_fRecoveringMode == fRecoveringUndo ) &&
            ( CmpLgpos( m_lgposRecoveryUndo, lgposMin ) == 0 ) ) ) )
    {
        return m_lgposRedo;
    }
    else
    {
        return m_pLogWriteBuffer->LgposLogTip();
    }
}

VOID LOG::LGLogTipWithLock( LGPOS * plgpos )
{
    m_pLogWriteBuffer->LockBuffer();
    *plgpos = LgposLGLogTipNoLock();
    m_pLogWriteBuffer->UnlockBuffer();
}

VOID LOG::LGGetLgposOfPbEntry( LGPOS *plgpos )
{
    m_pLogWriteBuffer->LockBuffer();
    m_pLogWriteBuffer->GetLgposOfPbEntry( plgpos );
    m_pLogWriteBuffer->UnlockBuffer();
}

VOID LOG::LGAddLgpos( LGPOS * const plgpos, UINT cb ) const
{
    m_pLogStream->AddLgpos( plgpos, cb );
}

QWORD LOG::CbLGOffsetLgposForOB0( LGPOS lgpos1, LGPOS lgpos2 ) const
{
    return m_pLogStream->CbOffsetLgposForOB0( lgpos1, lgpos2 );
}

LGPOS LOG::LgposLGFromIbForOB0( QWORD ib ) const
{
    return m_pLogStream->LgposFromIbForOB0( ib );
}

VOID LOG::LGSetSectorGeometry(
    const ULONG cbSecSize,
    const ULONG csecLGFile )
{
    m_pLogStream->LGSetSectorGeometry( cbSecSize, csecLGFile );

    if ( m_pcheckpoint &&
         ( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec != USHORT( sizeof( LGFILEHDR ) / cbSecSize ) ) ||
         ( m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_isec != USHORT( sizeof( LGFILEHDR ) / cbSecSize ) ) )
    {
        Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec <= sizeof( LGFILEHDR ) / 512  );
        Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib == 0 );
        Assert( m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_isec <= sizeof( LGFILEHDR ) / 512  );
        Assert( m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_ib == 0 );

        Expected( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration == m_lgenInitial );

        m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec = USHORT( sizeof( LGFILEHDR ) / cbSecSize );
        m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib = 0;
        m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_isec = USHORT( sizeof( LGFILEHDR ) / cbSecSize );
        m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_ib = 0;
    }
}

VOID LOG::LGFullLogNameFromLogId(
    __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName,
    LONG lGeneration,
    __in PCWSTR wszDirectory )
{
    m_pLogStream->LGFullLogNameFromLogId( wszFullLogFileName, lGeneration, wszDirectory );
}

VOID LOG::LGMakeLogName(
    __out_bcount(cbLogName) PWSTR wszLogName,
    ULONG cbLogName,
    const enum eLGFileNameSpec eLogType,
    LONG lGen ) const
{
    m_pLogStream->LGMakeLogName( wszLogName, cbLogName, eLogType, lGen );
}

ERR LOG::ErrLGMakeLogNameBaseless(
    __out_bcount(cbLogName)
    PWSTR wszLogName,
    ULONG cbLogName,
    __in PCWSTR wszLogFolder,
    const enum eLGFileNameSpec eLogType,
    LONG lGen,
    __in_opt PCWSTR wszLogExt ) const
{
    return m_pLogStream->ErrLGMakeLogNameBaseless( wszLogName, cbLogName, wszLogFolder, eLogType, lGen, wszLogExt );
}

ERR LOG::ErrLGWaitForLogGen( const LONG lGeneration )
{
    return m_pLogWriteBuffer->ErrLGWaitForLogGen( lGeneration );
}

BOOL LOG::FLGSignalWrite()
{
    return m_pLogWriteBuffer->FLGSignalWrite();
}

ERR LOG::ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec )
{
    return m_pLogWriteBuffer->ErrLGWaitForWrite( ppib, plgposLogRec );
}

ERR LOG::ErrLGFlush( const IOFLUSHREASON iofr, const BOOL fMayOwnBFLatch )
{
    ERR err = JET_errSuccess;

    LGPOS lgposWriteTip, lgposFlushTip;
    LGWriteAndFlushTip( &lgposWriteTip, &lgposFlushTip );
    if ( CmpLgpos( lgposFlushTip, lgposWriteTip ) < 0 )
    {
        BOOL fFlushed = fFalse;


        if ( fMayOwnBFLatch )
        {
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
        }

        err = m_pLogStream->ErrLGFlushLogFileBuffers( iofr, &fFlushed );

        if ( fMayOwnBFLatch )
        {
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }

        if ( err >= JET_errSuccess && fFlushed )
        {
            LGSetFlushTip( lgposWriteTip );
        }

        AssertTrack( ( err < JET_errSuccess ) || fFlushed, OSFormat( "LGFlushNoFlushIofr:%d", (INT)iofr ) );
    }

    return err;
}

ERR LOG::ErrLGScheduleWrite( DWORD cmsecDurableCommit, LGPOS lgposCommit )
{
    return m_pLogWriteBuffer->ErrLGScheduleWrite( cmsecDurableCommit, lgposCommit );
}

ERR LOG::ErrLGStopAndEmitLog()
{
    ERR err = JET_errSuccess;

    m_pLogWriteBuffer->LockBuffer();
    SetNoMoreLogWrite( ErrERRCheck( errLogServiceStopped ) );
    m_pLogWriteBuffer->UnlockBuffer();

    CallR( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );

    return m_pLogStream->ErrEmitSignalLogEnd();
}

BOOL LOG::FLGRolloverInDuration( TICK dtickLogRoll )
{
    return m_pLogStream->FRolloverInDuration( dtickLogRoll );
}

VOID LOG::LGLockWrite()
{
    m_pLogStream->LockWrite();
}

VOID LOG::LGUnlockWrite()
{
    m_pLogStream->UnlockWrite();
}

VOID LOG::LGCloseFile()
{
    m_pLogStream->LGCloseFile();
}

BOOL LOG::FLGFileOpened() const
{
    return m_pLogStream->FLGFileOpened();
}

ERR LOG::ErrLGInitTmpLogBuffers()
{
    return m_pLogStream->ErrLGInitTmpLogBuffers();
}

VOID LOG::LGTermTmpLogBuffers()
{
    m_pLogStream->LGTermTmpLogBuffers();
}

ERR LOG::ErrLGInitSetInstanceWiseParameters( JET_GRBIT grbit )
{
    m_fLogDisabled = m_pinst->FComputeLogDisabled();
    m_fReplayMissingMapEntryDB = ( grbit & JET_bitReplayMissingMapEntryDB ? fTrue : fFalse );
    if ( !m_fLogDisabled )
    {
        m_fReplayingIgnoreMissingDB = ( grbit & JET_bitReplayIgnoreMissingDB ? fTrue : fFalse );

        m_fTruncateLogsAfterRecovery = ( grbit & JET_bitTruncateLogsAfterRecovery ? fTrue : fFalse );
        m_fIgnoreLostLogs = ( grbit & JET_bitReplayIgnoreLostLogs ? fTrue : fFalse );
        m_fReplayIgnoreLogRecordsBeforeMinRequiredLog = ( grbit & JET_bitReplayIgnoreLogRecordsBeforeMinRequiredLog ? fTrue : fFalse );
    }

    return m_pLogStream->ErrLGInitSetInstanceWiseParameters();
}

LONG LOG::LGGetCurrentFileGenWithLock( LOGTIME *ptmCreate )
{
    m_pLogWriteBuffer->LockBuffer();
    LONG lGen = m_pLogStream->GetCurrentFileGen( ptmCreate );
    m_pLogWriteBuffer->UnlockBuffer();

    return lGen;
}

LONG LOG::LGGetCurrentFileGenNoLock( LOGTIME *ptmCreate ) const
{
    return m_pLogStream->GetCurrentFileGen( ptmCreate );
}

BOOL LOG::FLGProbablyWriting()
{
    return m_pLogWriteBuffer->FLGProbablyWriting();
}

VOID LOG::ResetCheckpoint( SIGNATURE *psignlog )
{
    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = m_lgenInitial;
    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec = (WORD)m_pLogStream->CSecHeader();
    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib = 0;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = m_pcheckpoint->checkpoint.le_lgposCheckpoint;
    PERFOpt( ibLGCheckpoint.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposCheckpoint, lgposMin ) ) );
    PERFOpt( ibLGDbConsistency.Set( m_pinst, m_pLogStream->CbOffsetLgpos( m_pcheckpoint->checkpoint.le_lgposDbConsistency, lgposMin ) ) );

    if ( psignlog != NULL )
    {
        m_pcheckpoint->checkpoint.signLog = *psignlog;
    }
}

ERR LOG::ErrLGSetSignLog(
    SIGNATURE * psignLog,
    BOOL fNeedToCheckLogID )
{
    ERR err = JET_errSuccess;

    if ( psignLog == NULL )
    {
        m_fSignLogSet = fFalse;
    }
    else if ( !m_fSignLogSet )
    {
        m_signLog = *psignLog;
        m_fSignLogSet = fTrue;
    }
    else if ( fNeedToCheckLogID &&
              memcmp( &m_signLog, psignLog, sizeof( m_signLog ) ) != 0 )
    {
        Error( ErrERRCheck( JET_errBadLogSignature ) );
    }

HandleError:
    return err;
}

ERR LOG::ErrLGVerifyFileSize( QWORD qwFileSize )
{
    ERR err = JET_errSuccess;
    QWORD qwSystemFileSize;

    if ( m_fUseRecoveryLogFileSize )
    {


        if ( m_lLogFileSizeDuringRecovery == 0 )
        {


            Assert( qwFileSize % 1024 == 0 );
            m_lLogFileSizeDuringRecovery = LONG( qwFileSize / 1024 );
        }


        qwSystemFileSize = QWORD( m_lLogFileSizeDuringRecovery ) * 1024;
    }
    else if ( !FDefaultParam( m_pinst, JET_paramLogFileSize ) )
    {


        qwSystemFileSize = QWORD( UlParam( m_pinst, JET_paramLogFileSize ) ) * 1024;
    }
    else
    {


        qwSystemFileSize = qwFileSize;
        Call( Param( m_pinst, JET_paramLogFileSize )->Set( m_pinst, ppibNil, LONG( qwFileSize / 1024 ), NULL ) );
    }
    if ( qwFileSize != qwSystemFileSize )
    {
        Error( ErrERRCheck( JET_errLogFileSizeMismatch ) );
    }

HandleError:
    return err;
}

ERR LOG::ErrLGCheckState()
{
    return m_pLogStream->ErrLGCheckState();
}

ERR LOG::ErrLGCreateEmptyShadowLogFile( const WCHAR * const wszPath, const LONG lgenShadowDebug )
{
    Assert( lgenShadowDebug > 0 );
    return m_pLogStream->ErrLGCreateEmptyShadowLogFile( wszPath, lgenShadowDebug );
}

BOOL LOG::FLGFileVersionUpdateNeeded( _In_ const LogVersion& lgvDesired )
{
    return m_pLogStream->FLGFileVersionUpdateNeeded( lgvDesired );
}

ERR LOG::ErrLGGetGenerationRange(
            __in PCWSTR wszFindPath,
            LONG* plgenLow,
            LONG* plgenHigh,
            __in BOOL fLooseExt,
            __out_opt BOOL * pfDefaultExt )
{
    return m_pLogStream->ErrLGGetGenerationRange(
            wszFindPath,
            plgenLow,
            plgenHigh,
            fLooseExt,
            pfDefaultExt );
}

PCWSTR LOG::LogExt() const
{
    return m_pLogStream->LogExt();
}

VOID LOG::LGCreateAsynchCancel( const BOOL fWaitForPending )
{
    m_pLogStream->LGCreateAsynchCancel( fWaitForPending );
}

ERR LOG::ErrStartAsyncLogFileCreation( _In_ PCWSTR wszPathJetTmpLog, _In_ const LONG lgenDebug )
{
    return m_pLogStream->ErrStartAsyncLogFileCreation( wszPathJetTmpLog, lgenDebug );
}

ERR LOG::ErrLGNewLogFile( LONG lgen, BOOL fLGFlags )
{
    return m_pLogStream->ErrLGNewLogFile( lgen, fLGFlags );
}

ERR LOG::ErrLGTruncateLog(
    const LONG lgenMic,
    const LONG lgenMac,
    const BOOL fSnapshot,
    BOOL fFullBackup )
{
    return m_pLogStream->ErrLGTruncateLog(
            lgenMic,
            lgenMac,
            fSnapshot,
            fFullBackup );
}

VOID LOG::LGVerifyFileHeaderOnInit()
{
    m_pLogStream->LGVerifyFileHeaderOnInit();
}

BOOL LOG::FLGLogPaused() const
{
    return m_pLogWriteBuffer->FLGLogPaused();
}

VOID LOG::LGSetLogPaused( BOOL fValue )
{
    m_pLogWriteBuffer->LGSetLogPaused( fValue );
}

VOID LOG::LGSignalLogPaused( BOOL fSet )
{
    m_pLogWriteBuffer->LGSignalLogPaused( fSet );
}

VOID LOG::LGWaitLogPaused()
{
    m_pLogWriteBuffer->LGWaitLogPaused();
}

BOOL LOG::FRedoMapNeeded( const IFMP ifmp ) const
{
    const BOOL fDirtyAndPatched = g_rgfmp[ifmp].Pdbfilehdr()->Dbstate() == JET_dbstateDirtyAndPatchedShutdown;
    const BOOL fDataFromFutureLogs = g_rgfmp[ifmp].FContainsDataFromFutureLogs();
    Assert( !fDirtyAndPatched || fDataFromFutureLogs );
    return fDirtyAndPatched || fDataFromFutureLogs;
}

VOID LOG::LGAddWastage( const ULONG cbWastage )
{
    m_cbCurrentWastage += cbWastage;
}

VOID LOG::LGAddUsage( const ULONG cbUsage )
{
    const ULONG cbChkptDepthSlot = (ULONG)( UlParam( m_pinst, JET_paramCheckpointDepthMax ) / NUM_WASTAGE_SLOTS );
    if ( cbChkptDepthSlot == 0 )
    {
        return;
    }

    m_cbCurrentUsage += cbUsage;
    while ( m_cbCurrentUsage >= cbChkptDepthSlot )
    {
        m_cbTotalWastage += m_cbCurrentWastage - m_rgWastages[ m_iNextWastageSlot ];
        Assert( m_cbTotalWastage >= 0 );
        m_rgWastages[ m_iNextWastageSlot ] = m_cbCurrentWastage;
        m_iNextWastageSlot = ( m_iNextWastageSlot + 1 ) % NUM_WASTAGE_SLOTS;
        m_cbCurrentWastage = 0;
        m_cbCurrentUsage -= cbChkptDepthSlot;
    }
}

ERR
LGEN_LOGTIME_MAP::ErrAddLogtimeMapping( const LONG lGen, const LOGTIME* const pLogtime )
{
    ERR err;

    if ( lGen < m_lGenLogtimeMappingStart )
    {
        return JET_errSuccess;
    }

    Assert( pLogtime->FIsSet() );

    if ( m_lGenLogtimeMappingStart == 0 )
    {
        m_lGenLogtimeMappingStart = lGen;
    }
    LONG cSlotsNeeded = lGen - m_lGenLogtimeMappingStart + 1;
    if ( cSlotsNeeded > m_cLogtimeMappingAlloc )
    {
        LONG cSlotsAllocated = max( max( cSlotsNeeded, 4 ), 2 * m_cLogtimeMappingAlloc );
        LOGTIME *pNewSlots;
        AllocR( pNewSlots = (LOGTIME *)PvOSMemoryHeapAlloc( cSlotsAllocated * sizeof(LOGTIME) ) );
        memset( pNewSlots, '\0', cSlotsAllocated * sizeof(LOGTIME) );
        if ( m_cLogtimeMappingAlloc != 0 )
        {
            memcpy( pNewSlots, m_pLogtimeMapping, m_cLogtimeMappingValid * sizeof(LOGTIME) );
            OSMemoryHeapFree( m_pLogtimeMapping );
        }
        m_pLogtimeMapping = pNewSlots;
        m_cLogtimeMappingAlloc = cSlotsAllocated;
    }
    if ( cSlotsNeeded > m_cLogtimeMappingValid )
    {
        Expected( cSlotsNeeded == ( m_cLogtimeMappingValid + 1 ) );
        memset( m_pLogtimeMapping + m_cLogtimeMappingValid, '\0', ( cSlotsNeeded - m_cLogtimeMappingValid - 1 ) * sizeof(LOGTIME) );
        memcpy( m_pLogtimeMapping + cSlotsNeeded - 1, pLogtime, sizeof(LOGTIME) );
        m_cLogtimeMappingValid = cSlotsNeeded;
    }
    else
    {
        LOGTIME tmEmpty;
        memset( &tmEmpty, '\0', sizeof( LOGTIME ) );
        if ( memcmp( m_pLogtimeMapping + cSlotsNeeded - 1, &tmEmpty, sizeof(LOGTIME) ) == 0 )
        {
            memcpy( m_pLogtimeMapping + cSlotsNeeded - 1, pLogtime, sizeof(LOGTIME) );
        }
    }
    Assert( m_cLogtimeMappingAlloc >= m_cLogtimeMappingValid );
    Assert( m_cLogtimeMappingValid > lGen - m_lGenLogtimeMappingStart );
    Assert( memcmp( m_pLogtimeMapping + lGen - m_lGenLogtimeMappingStart, pLogtime, sizeof(LOGTIME) ) == 0 );

    return JET_errSuccess;
}

VOID
LGEN_LOGTIME_MAP::LookupLogtimeMapping( const LONG lGen, LOGTIME* const pLogtime )
{
    Assert( lGen >= m_lGenLogtimeMappingStart );
    if ( lGen >= m_lGenLogtimeMappingStart && m_cLogtimeMappingValid > lGen - m_lGenLogtimeMappingStart )
    {
        memcpy( pLogtime, m_pLogtimeMapping + lGen - m_lGenLogtimeMappingStart, sizeof(LOGTIME) );
    }
    else
    {
        memset( pLogtime, '\0', sizeof(LOGTIME) );
    }
}

VOID
LGEN_LOGTIME_MAP::TrimLogtimeMapping( const LONG lGen )
{
    if ( lGen > m_lGenLogtimeMappingStart )
    {
        LONG cSlotsToDelete = min( lGen + 1 - m_lGenLogtimeMappingStart, m_cLogtimeMappingValid );
        LONG cSlotsToKeep = m_cLogtimeMappingValid - cSlotsToDelete;
        if ( cSlotsToKeep > 0 )
        {
            memmove( m_pLogtimeMapping, m_pLogtimeMapping + cSlotsToDelete, cSlotsToKeep * sizeof(LOGTIME) );
        }
        m_lGenLogtimeMappingStart += cSlotsToDelete;
        Assert( m_lGenLogtimeMappingStart <= lGen + 1 );
        m_cLogtimeMappingValid = cSlotsToKeep;
    }
}

