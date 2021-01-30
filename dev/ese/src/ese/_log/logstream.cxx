// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"
#include <ctype.h>

extern CGPTaskManager g_taskmgrLog;

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotal<QWORD, INST, fFalse> cLGLogFileGenerated;
LONG LLGLogFileGeneratedCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGLogFileGenerated.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGWrite;
LONG LLGWriteCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGWrite.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceLiveTotal<> cbLGWritten;
LONG LLGBytesWrittenCEFLPv( LONG iInstance, void *pvBuf )
{
    cbLGWritten.PassTo( iInstance, pvBuf );
    return 0;
}

#endif

LOG_STREAM::LOG_STREAM( INST * pinst, LOG * pLog )
    : m_pLog( pLog ),
      m_pinst( pinst ),
      m_pLogWriteBuffer( NULL ),
      m_plgfilehdr( NULL ),
      m_plgfilehdrT( NULL ),
      m_fLogSequenceEnd( fFalse ),
      m_ls( lsNormal ),
      m_csecLGFile( 0 ),
      m_cbSecSavedForOB0( 0 ),
      m_csecFileSavedForOB0( 0 ),
      m_lGenSavedForOB0( 0 ),
      m_lgposUndoForOB0( lgposMin ),
      m_pfapiLog( NULL ),
      m_pfapiJetTmpLog( NULL ),
      m_fCreateAsynchResUsed( fFalse ),
      m_fCreateAsynchZeroFilled( fFalse ),
      m_errCreateAsynch( JET_errSuccess ),
      m_asigCreateAsynchIOCompleted( CSyncBasicInfo( _T( "LOG::m_asigCreateAsynchIOCompleted" ) ) ),
      m_critCreateAsynchIOExecuting( CLockBasicInfo( CSyncBasicInfo( "LOG::m_critCreateAsynchIOExecuting" ), rankAsynchIOExecuting, 0 ) ),
      m_lgposCreateAsynchTrigger( lgposMax ),
      m_critJetTmpLog( CLockBasicInfo( CSyncBasicInfo( szJetTmpLog ), rankJetTmpLog, 0 ) ),
      m_ibJetTmpLog( 0 ),
      m_cIOTmpLog( 0 ),
      m_cIOTmpLogMax( 0 ),
      m_rgibTmpLog( NULL ),
      m_rgcbTmpLog( NULL ),
      m_wszLogExt( NULL ),
      m_cbSecVolume( 0 ),
      m_cbSec_( 0 ),
      m_log2cbSec_( 0 ),
      m_csecHeader( 0 ),
      m_critLGWrite( CLockBasicInfo( CSyncBasicInfo( szLGWrite ), rankLGWrite, CLockDeadlockDetectionInfo::subrankNoDeadlock ) ),
      m_cRemovedLogs( 0 ),
      m_rwlLGResFiles( CLockBasicInfo( CSyncBasicInfo( szLGResFiles ), rankLGResFiles, 0 ) ),
      m_cbReservePoolHigh( 0 ),
      m_cReserveLogs( 0 ),
      m_qwSequence( 0 ),
      m_checksumAccSectors( 0 ),
      m_checksumAccSectorsPrev( 0 ),
#ifdef DEBUG
      m_fResettingLogStream( fFalse ),
      m_fDBGTraceBR( 0 ),
#endif
      m_tickNewLogFile( ~TICK( 0 ) ),
      m_fCreatedNewLogFileDuringRedo( fFalse ),
      m_fLogEndEmitted( fFalse )
{
    m_wszLogName[0] = 0;

    memset( &m_lgvHighestRecovered, 0, sizeof(m_lgvHighestRecovered) );

#ifdef DEBUG
    ErrOSTraceCreateRefLog( 1000, 0, &m_pEmitTraceLog );
#else
    m_pEmitTraceLog = NULL;
#endif

    PERFOpt( cLGLogFileGenerated.Clear( m_pinst ) );
    PERFOpt( cLGWrite.Clear( m_pinst ) );
    PERFOpt( cbLGWritten.Clear( m_pinst ) );
}

LOG_STREAM::~LOG_STREAM()
{
    PERFOpt( cLGLogFileGenerated.Clear( m_pinst ) );
    PERFOpt( cLGWrite.Clear( m_pinst ) );
    PERFOpt( cbLGWritten.Clear( m_pinst ) );

    if ( m_pEmitTraceLog != NULL )
    {
        OSTraceDestroyRefLog( m_pEmitTraceLog );
        m_pEmitTraceLog = NULL;
    }

    Expected( m_rgibTmpLog == NULL );
    Expected( m_rgcbTmpLog == NULL );
}

VOID LOG_STREAM::SetLogBuffers(
    LOG_WRITE_BUFFER *  pWriteBuffer
    )
{
    m_pLogWriteBuffer = pWriteBuffer;
}

LONG LOG_STREAM::GetCurrentFileGen( LOGTIME * ptmCreate ) const
{
    if ( ptmCreate != NULL && m_plgfilehdr != NULL )
    {
        *ptmCreate = m_plgfilehdr->lgfilehdr.tmCreate;
    }

    return m_plgfilehdr ? m_plgfilehdr->lgfilehdr.le_lGeneration : 0;
}

const LGFILEHDR* LOG_STREAM::GetCurrentFileHdr() const
{
    return m_plgfilehdr;
}

VOID LOG_STREAM::SaveCurrentFileHdr()
{
    Assert( m_pLog->FRecovering() && m_pLog->FRecoveringMode() == fRecoveringRedo );
    m_pLogWriteBuffer->LockBuffer();
    UtilMemCpy( m_plgfilehdrT, m_plgfilehdr, sizeof(LGFILEHDR) );
    m_pLogWriteBuffer->UnlockBuffer();
}

BOOL LOG_STREAM::FLGFileVersionUpdateNeeded( _In_ const LogVersion& lgvDesired )
{
    m_pLogWriteBuffer->LockBuffer();
    LogVersion lgvCurrent = LgvFromLgfilehdr( m_plgfilehdr );
    m_pLogWriteBuffer->UnlockBuffer();

    const BOOL fVersionUpdateNeeded = ( CmpLgVer( lgvCurrent, lgvDesired ) < 0 );

    if ( fVersionUpdateNeeded )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "Log roll upgrade needed: %d.%d.%d vs. %d.%d.%d\n",
                    lgvCurrent.ulLGVersionMajor, lgvCurrent.ulLGVersionUpdateMajor, lgvCurrent.ulLGVersionUpdateMinor,
                    lgvDesired.ulLGVersionMajor, lgvDesired.ulLGVersionUpdateMajor, lgvDesired.ulLGVersionUpdateMinor ) );
    }

    return fVersionUpdateNeeded;
}

ERR LOG_STREAM::ErrLGInitSetInstanceWiseParameters()
{
    ERR     err;
    WCHAR   rgchFullName[IFileSystemAPI::cchPathMax];


    if ( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), rgchFullName ) == JET_errInvalidPath )
    {
        const WCHAR *szPathT[1] = { SzParam( m_pinst, JET_paramLogFilePath ) };

        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                FILE_NOT_FOUND_ERROR_ID,
                1,
                szPathT,
                0,
                NULL,
                m_pinst );

        return ErrERRCheck( JET_errFileNotFound );
    }


    CallR( m_pinst->m_pfsapi->ErrFileAtomicWriteSize( rgchFullName, (DWORD*)&m_cbSecVolume ) );
    Assert( FPowerOf2( m_cbSecVolume ) );
    if ( m_cbSecVolume > 4096 )
    {
        WCHAR szSecVolume[16];
        OSStrCbFormatW( szSecVolume, sizeof( szSecVolume ), L"%d", m_cbSecVolume );

        const WCHAR *rgszT[2];
        rgszT[0] = SzParam( m_pinst, JET_paramLogFilePath );
        rgszT[1] = szSecVolume;

        UtilReportEvent(
                eventWarning,
                LOGGING_RECOVERY_CATEGORY,
                LOG_SECTOR_SIZE_MISMATCH,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                m_pinst );

        m_cbSecVolume = 4096;
    }
    if (
         ( m_cbSecVolume < 512 ) ||
         ( sizeof( LGFILEHDR ) % m_cbSecVolume != 0 ) )
    {
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagConfiguration, L"03059212-80e6-4ff3-a548-19697d44dcfe" );

        return ErrERRCheck( JET_errSectorSizeNotSupported );
    }

    m_cbSec_ = LOG_SEGMENT_SIZE;
    m_log2cbSec_ = Log2(m_cbSec_);

    
    m_csecHeader = ( sizeof( LGFILEHDR ) + m_cbSec - 1 ) / m_cbSec;

    m_csecLGFile = CsecLGFromSize( (LONG)UlParam( m_pinst, JET_paramLogFileSize ) );

    return JET_errSuccess;
}

VOID LOG_STREAM::LGMakeLogName( const enum eLGFileNameSpec eLogType, LONG lGen, BOOL fUseDefaultFolder )
{
    CallS ( ErrLGMakeLogNameBaseless(
                m_wszLogName,
                sizeof( m_wszLogName ),
                fUseDefaultFolder ? SzParam( m_pinst, JET_paramLogFilePath ) : m_pLog->SzLGCurrentFolder(),
                eLogType,
                lGen ) );
}

VOID LOG_STREAM::LGMakeLogName( __out_bcount(cbLogName) PWSTR wszLogName, ULONG cbLogName, const enum eLGFileNameSpec eLogType, LONG lGen, BOOL fUseDefaultFolder ) const
{
    CallS ( ErrLGMakeLogNameBaseless(
                wszLogName,
                cbLogName,
                fUseDefaultFolder ? SzParam( m_pinst, JET_paramLogFilePath ) : m_pLog->SzLGCurrentFolder(),
                eLogType,
                lGen ) );
}

ERR LGFileHelper::ErrLGMakeLogNameBaselessEx(
    __out_bcount(cbLogName) PWSTR wszLogName,
    __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) ULONG cbLogName,
    IFileSystemAPI *const   pfsapi,
    __in PCWSTR wszLogFolder,
    __in PCWSTR wszBaseName,
    const enum eLGFileNameSpec eLogType,
    LONG lGen,
    __in PCWSTR wszLogExt,
    ULONG cLogDigits )
{
    ERR err = JET_errSuccess;
    WCHAR wszLogFileName[20];

    const WCHAR * wszExt = wszLogExt;

    Expected( lGen > 0 || eLogType == eCurrentLog || eLogType == eCurrentTmpLog || eLogType == eArchiveLog || eLogType == eReserveLog );
    Expected( lGen != lGenSignalCurrentID );
    Expected( lGen >= 0 );

    Assert( wszExt );

    OSStrCbCopyW( wszLogFileName, sizeof(wszLogFileName), wszBaseName );

    switch ( eLogType ){

    case eCurrentLog:
        Assert( lGen == 0 );
        Assert( 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        break;

    case eCurrentTmpLog:
        Assert( lGen == 0 );
        Assert( 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        OSStrCbAppendW( wszLogFileName, sizeof(wszLogFileName), wszLogTmp );
        break;

    case eReserveLog:
        wszExt = wszResLogExt;
        cLogDigits = 5;
        OSStrCbAppendW( wszLogFileName, sizeof(wszLogFileName), wszLogRes );
    case eArchiveLog:
    case eShadowLog:
        Assert( eLogType != eArchiveLog || 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        Assert( eLogType != eReserveLog || 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        Assert( eLogType != eShadowLog || 0==_wcsicmp( wszLogExt, wszSecLogExt ) );
        if ( lGen == 0 )
        {
            OSStrCbAppendW( wszLogFileName, sizeof(wszLogFileName), L"*" );
        }
        else
        {
            LGFileHelper::LGSzLogIdAppend ( wszLogFileName, sizeof(wszLogFileName), lGen, cLogDigits );
        }
        break;

    default:
        AssertSz( false, "Unknown path");
    }

    Assert( LOSStrLengthW(wszLogFileName) >= 3 );

    CallR( pfsapi->ErrPathBuild( wszLogFolder, wszLogFileName, wszExt, wszLogName, cbLogName) );
    return(JET_errSuccess);

}

void LOG_STREAM::LGSetLogExt(
    BOOL fLegacy, BOOL fReset
    )
{
    Assert( !m_pLog->FLogInitialized() ||
            (m_pLog->FRecoveringMode() == fRecoveringNone) ||
            m_wszLogExt != NULL );
    Assert( fReset ||
            m_wszLogExt == NULL );

    m_wszLogExt = (fLegacy) ? wszOldLogExt : wszNewLogExt;
}

ERR LOG_STREAM::ErrLGMakeLogNameBaseless(
    __out_bcount(cbLogName) PWSTR wszLogName,
    ULONG cbLogName,
    __in PCWSTR wszLogFolder,
    const enum eLGFileNameSpec eLogType,
    LONG lGen,
    __in_opt PCWSTR wszLogExt ) const
{
    ERR         err         = JET_errSuccess;
    const WCHAR * wszExt = wszLogExt ? wszLogExt : m_wszLogExt;

    if ( !wszExt )
    {
        AssertSz( fFalse, "We should have a log extension by now." );
        CallR( JET_errInternalError );
    }

    return( LGFileHelper::ErrLGMakeLogNameBaselessEx( wszLogName, cbLogName,
                                        m_pinst->m_pfsapi, wszLogFolder,
                                        SzParam( m_pinst, JET_paramBaseName ),
                                        eLogType, lGen, wszExt,
                                        ( UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8) );
}

ERR LOG_STREAM::ErrLGInit()
{
    ERR err = JET_errSuccess;

#ifdef DEBUG
    WCHAR * wsz;

    if ( ( wsz = GetDebugEnvValue ( L"TRACEBR" ) ) != NULL )
    {
        m_fDBGTraceBR = _wtoi( wsz );
        delete[] wsz;
    }
    else
        m_fDBGTraceBR = 0;
#endif

    Alloc( m_plgfilehdr = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR) * 2, NULL ) );
    m_plgfilehdrT = m_plgfilehdr + 1;

HandleError:
    return err;
}

ERR LOG_STREAM::ErrLGInitTmpLogBuffers()
{
    Assert( m_cIOTmpLogMax == 0 );
    Assert( m_rgibTmpLog == NULL );
    Assert( m_rgcbTmpLog == NULL );
    Assert( cbLogExtendPattern != 0 );

    ERR err = JET_errSuccess;

    LGTermTmpLogBuffers();

    const DWORD cIOTmpLog = max( CbLGLogFileSize() / ( 2 * cbLogExtendPattern ), 1 );
    m_rgibTmpLog = new QWORD[ cIOTmpLog ];
    m_rgcbTmpLog = new DWORD[ cIOTmpLog ];
    if ( m_rgibTmpLog != NULL && m_rgcbTmpLog != NULL )
    {
        m_cIOTmpLogMax = cIOTmpLog;
        m_asigCreateAsynchIOCompleted.Set();
    }
    else
    {
        LGTermTmpLogBuffers();
        err = ErrERRCheck( JET_errOutOfMemory );
    }

    return err;
}

VOID LOG_STREAM::LGTermTmpLogBuffers()
{
    m_cIOTmpLogMax = 0;
    delete[] m_rgibTmpLog;
    m_rgibTmpLog = NULL;
    delete[] m_rgcbTmpLog;
    m_rgcbTmpLog = NULL;
}



IFileAPI::FileModeFlags LOG_STREAM::FmfLGStreamDefault( const IFileAPI::FileModeFlags fmfAddl ) const
{
    Expected( ( fmfAddl & ~IFileAPI::fmfReadOnly ) == 0 );

    BOOL fStorageWriteBackSetting = BoolParam( m_pinst, JET_paramUseFlushForWriteDurability ) && !BoolParam( JET_paramEnableFileCache );

    IFileAPI::FileModeFlags fmfSettingInjection = IFileAPI::fmfNone;
#ifdef FORCE_STORAGE_WRITEBACK
#ifdef DEBUG
    fmfSettingInjection = ( ( TickOSTimeCurrent() % 100 ) < 50 ) ?
                    IFileAPI::fmfStorageWriteBack :
                    IFileAPI::fmfNone;
#else
    fmfSettingInjection = IFileAPI::fmfStorageWriteBack;
#endif
#endif
    const IFileAPI::FileModeFlags fmfRet =
            ( fmfAddl |
                fmfSettingInjection |
                ( BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfRegular ) |
                ( fStorageWriteBackSetting ? IFileAPI::fmfStorageWriteBack : IFileAPI::fmfNone )
                );
    
    if ( fmfAddl & IFileAPI::fmfReadOnly )
    {
        return ( fmfRet & ~IFileAPI::fmfReadOnly ) | IFileAPI::fmfReadOnlyClient;
    }
    return fmfRet;
}




VOID LOG_STREAM::LGSetSectorGeometry( const ULONG cbSecSize, const ULONG csecLGFile )
{
    Assert( cbSecSize >= 512 && FPowerOf2( cbSecSize ) );
    m_cbSec_ = cbSecSize;
    m_log2cbSec_ = Log2(m_cbSec_);
    m_csecHeader = ( sizeof( LGFILEHDR ) + cbSecSize - 1 ) / cbSecSize;
    m_csecLGFile = csecLGFile;
}

VOID LOG_STREAM::LGResetSectorGeometry(
    LONG lLogFileSize )
{
    m_cbSec_ = LOG_SEGMENT_SIZE;
    m_log2cbSec_ = Log2(m_cbSec_);
    m_csecHeader = ( sizeof( LGFILEHDR ) + m_cbSec - 1 ) / m_cbSec;
    if ( lLogFileSize == 0 )
    {
        lLogFileSize = (LONG)UlParam( m_pinst, JET_paramLogFileSize );
    }
    m_csecLGFile = CsecLGFromSize( lLogFileSize );
}


ERR LOG_STREAM::ErrLGReadFileHdr(
    IFileAPI *          pfapiLog,
    IOREASONPRIMARY     iorp,
    LGFILEHDR *         plgfilehdr,
    const BOOL          fNeedToCheckLogID,
    const BOOL          fBypassDbPageAndLogSectorSizeChecks )
{
    ERR     err;
    QWORD   qwFileSize;
    QWORD   qwCalculatedFileSize;
    BOOL    fLogWriteBufferLocked = fFalse;

    if ( pfapiLog == NULL )
    {
        pfapiLog = m_pfapiLog;
    }
    if ( plgfilehdr == NULL )
    {
        plgfilehdr = m_plgfilehdr;
    }

    if ( plgfilehdr == m_plgfilehdr )
    {
        m_pLogWriteBuffer->LockBuffer();
        fLogWriteBufferLocked = fTrue;
    }

    TraceContextScope tcScope( iorp );
    Call( ErrLGIReadFileHeader( pfapiLog, *tcScope, QosSyncDefault( m_pinst ), plgfilehdr, m_pinst ) );

    if ( fLogWriteBufferLocked )
    {
        m_pLogWriteBuffer->UnlockBuffer();
        fLogWriteBufferLocked = fFalse;
    }

    if ( !FIsOldLrckLogFormat( plgfilehdr->lgfilehdr.le_ulMajor ) &&
         plgfilehdr->lgfilehdr.le_cbSec != LOG_SEGMENT_SIZE )
    {
        Error( ErrERRCheck( JET_errLogFileCorrupt ) );
    }

    Call( m_pLog->ErrLGSetSignLog( &plgfilehdr->lgfilehdr.signLog,
                                   fNeedToCheckLogID ) );

    if ( !fBypassDbPageAndLogSectorSizeChecks )
    {
        const UINT  cbPageT = ( plgfilehdr->lgfilehdr.le_cbPageSize == 0 ?
                                    g_cbPageDefault :
                                    plgfilehdr->lgfilehdr.le_cbPageSize );
        if ( cbPageT != (UINT)g_cbPage )
            Call( ErrERRCheck( JET_errPageSizeMismatch ) );
    }


    Call( pfapiLog->ErrSize( &qwFileSize, IFileAPI::filesizeLogical ) );
    qwCalculatedFileSize = QWORD( plgfilehdr->lgfilehdr.le_csecLGFile ) *
                           QWORD( plgfilehdr->lgfilehdr.le_cbSec );
    if ( qwFileSize != qwCalculatedFileSize )
    {
        Call( ErrERRCheck( JET_errLogFileSizeMismatch ) );
    }

    Call( m_pLog->ErrLGVerifyFileSize( qwFileSize ) );

    LGSetSectorGeometry( plgfilehdr->lgfilehdr.le_cbSec, plgfilehdr->lgfilehdr.le_csecLGFile );
    if ( FIsOldLrckLogFormat( plgfilehdr->lgfilehdr.le_ulMajor ) )
    {
        Assert( m_cbSecSavedForOB0 == 0 || m_cbSecSavedForOB0 == m_cbSec_ );
        m_cbSecSavedForOB0 = m_cbSec_;
        m_csecFileSavedForOB0 = m_csecLGFile;
        m_lGenSavedForOB0 = plgfilehdr->lgfilehdr.le_lGeneration;
    }

    if ( plgfilehdr->lgfilehdr.le_ulMinor == ulLGVersionMinorFinalDeprecatedValue ||
            plgfilehdr->lgfilehdr.le_ulMinor == ulLGVersionMinor_Win7 )
    {
        LogVersion lgvCurr = LgvFromLgfilehdr( plgfilehdr );
        if ( CmpLgVer( lgvCurr, m_lgvHighestRecovered ) > 0 )
        {
            OSTrace( JET_tracetagUpgrade, OSFormat( "Updating highest recovered log ver: %d.%d.%d\n", lgvCurr.ulLGVersionMajor, lgvCurr.ulLGVersionUpdateMajor, lgvCurr.ulLGVersionUpdateMinor ) );
            m_lgvHighestRecovered = lgvCurr;
        }
    }

HandleError:

    if ( fLogWriteBufferLocked )
    {
        m_pLogWriteBuffer->UnlockBuffer();
        fLogWriteBufferLocked = fFalse;
    }

    if ( err == JET_errSuccess )
    {


        Assert( m_cbSec == plgfilehdr->lgfilehdr.le_cbSec );
        Assert( m_cbSec >= 512 );
        m_csecHeader = sizeof( LGFILEHDR ) / m_cbSec;
        m_csecLGFile = plgfilehdr->lgfilehdr.le_csecLGFile;
    }
    else
    {
        WCHAR       szLogfile[IFileSystemAPI::cchPathMax];
        WCHAR       szErr[16];
        const UINT  csz = 2;
        const WCHAR * rgszT[csz] = { szLogfile, szErr };

        if ( pfapiLog->ErrPath( szLogfile ) < 0 )
        {
            szLogfile[0] = 0;
        }
        OSStrCbFormatW( szErr, sizeof( szErr ), L"%d", err );

        if ( JET_errBadLogVersion == err )
        {
            UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                LOG_BAD_VERSION_ERROR_ID,
                csz - 1,
                rgszT,
                0,
                NULL,
                m_pinst );
        }
        else
        {
            UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                LOG_HEADER_READ_ERROR_ID,
                csz,
                rgszT,
                0,
                NULL,
                m_pinst );
        }

        m_pLog->SetNoMoreLogWrite( err );
    }

    return err;
}

ERR LOG_STREAM::ErrLGIFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature, const LogVersion& lgvCurrentFromLogGenTip )
{


    const FormatVersions * pfmtversMax = PfmtversEngineMax();
    Assert( efvFormatFeature <= pfmtversMax->efv );
    if ( efvFormatFeature <= pfmtversMax->efv &&
            0 == CmpLgVer( pfmtversMax->lgv, lgvCurrentFromLogGenTip ) )
    {
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "Log Feature test fast path - success: %d <= %d - %d.%d.%d == %d.%d.%d\n", efvFormatFeature, pfmtversMax->efv,
                    pfmtversMax->lgv.ulLGVersionMajor, pfmtversMax->lgv.ulLGVersionUpdateMajor, pfmtversMax->lgv.ulLGVersionUpdateMinor,
                    lgvCurrentFromLogGenTip.ulLGVersionMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMinor ) );
        return JET_errSuccess;
    }


    const FormatVersions * pfmtversFormatFeature = NULL;
    CallS( ErrGetDesiredVersion( NULL , efvFormatFeature, &pfmtversFormatFeature ) );
    if ( pfmtversFormatFeature )
    {
        if ( CmpLgVer( lgvCurrentFromLogGenTip, pfmtversFormatFeature->lgv ) >= 0 )
        {
            OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "Log Feature test slow path - success: %d <= %d - %d.%d.%d == %d.%d.%d\n", efvFormatFeature, pfmtversFormatFeature->efv,
                        lgvCurrentFromLogGenTip.ulLGVersionMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMinor,
                        pfmtversFormatFeature->lgv.ulLGVersionMajor, pfmtversFormatFeature->lgv.ulLGVersionUpdateMajor, pfmtversFormatFeature->lgv.ulLGVersionUpdateMinor ) );
            return JET_errSuccess;
        }
        else
        {
            if ( m_lesEventLogFeatureDisabled.FNeedToLog( efvFormatFeature ) )
            {
                
            }
            OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "Log Feature test fast path - FAIL too low: %d <= %d - %d.%d.%d == %d.%d.%d\n", efvFormatFeature, pfmtversFormatFeature->efv,
                        lgvCurrentFromLogGenTip.ulLGVersionMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMinor,
                        pfmtversFormatFeature->lgv.ulLGVersionMajor, pfmtversFormatFeature->lgv.ulLGVersionUpdateMajor, pfmtversFormatFeature->lgv.ulLGVersionUpdateMinor ) );
            return ErrERRCheck( JET_errEngineFormatVersionParamTooLowForRequestedFeature );
        }
    }

    AssertTrack( fFalse, "UnknownLogFormatFeatureDisabled" );

    return ErrERRCheck( errCodeInconsistency );
}

ERR LOG_STREAM::ErrLGFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature )
{
    Assert( !m_pinst->FRecovering() );
    const LogVersion lgvCurrentLog = LgvFromLgfilehdr( GetCurrentFileHdr() );
    return ErrLGIFormatFeatureEnabled( efvFormatFeature, lgvCurrentLog );
}




const WCHAR rgchLGFileDigits[] =
{
    L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
    L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F',
};
const LONG lLGFileBase = sizeof(rgchLGFileDigits)/sizeof(rgchLGFileDigits[0]);

VOID LGFileHelper::LGSzLogIdAppend( __inout_bcount_z( cbFName ) PWSTR wszLogFileName, size_t cbFName, LONG lGeneration, ULONG cchLogDigits )
{
    LONG    ich;
    LONG    ichBase;

    Assert( wszLogFileName );
    Assert( cchLogDigits == 0 || cchLogDigits == 5 || cchLogDigits == 8 );
    Assert( cchLogDigits != 5 || lGeneration <= 0xFFFFF );

    ichBase = wcslen(wszLogFileName);
    Assert( ichBase == 3 || (ichBase == 6 && 0 == wcscmp(&(wszLogFileName[3]), wszLogRes)) );
    Assert( cbFName >= ((ichBase+cchLogDigits+1)*sizeof(WCHAR)) );

    if ( cchLogDigits == 0 )
    {
        if ( lGeneration > 0xFFFFF )
        {
            cchLogDigits = 8;
        }
        else
        {
            cchLogDigits = 5;
        }
    }
    Assert( cchLogDigits == 5 || cchLogDigits == 8 );

    ich = cchLogDigits + ichBase - 1;

    if ( (sizeof(WCHAR)*(cchLogDigits+ichBase+1)) > cbFName )
    {
        AssertSz(false, "Someone passed in a path too short for ErrLGSzLogIdAppend(), we'll recover, though I don't know how well");
        if ( cbFName >= sizeof(WCHAR) )
        {
            OSStrCbAppendW( wszLogFileName, cbFName, L"" );
        }
        return;
    }

    for ( ; ich > (ichBase-1); ich-- )
    {
        wszLogFileName[ich] = rgchLGFileDigits[lGeneration % lLGFileBase];
        lGeneration = lGeneration / lLGFileBase;
    }
    Assert(lGeneration == 0);
    wszLogFileName[ichBase+cchLogDigits] = 0;

    return;
}


ERR LOG_STREAM::ErrLGTryOpenJetLog( const ULONG eOpenReason, const LONG lGenNext, BOOL fPickLogExt )
{
    ERR     err;
    WCHAR   wszPathJetTmpLog[IFileSystemAPI::cchPathMax];
    PCWSTR  wszLogExt = m_wszLogExt ? m_wszLogExt : m_pLog->WszLGGetDefaultExt( fFalse );

    Assert( m_wszLogExt || fPickLogExt );
    Assert( lGenNext > 0 || lGenNext == lGenSignalCurrentID );

OpenLog:


    CallS( ErrLGMakeLogNameBaseless( m_wszLogName, sizeof(m_wszLogName), m_pLog->SzLGCurrentFolder(), eCurrentLog, 0, wszLogExt  ) );
    CallS( ErrLGMakeLogNameBaseless( wszPathJetTmpLog, sizeof(wszPathJetTmpLog), m_pLog->SzLGCurrentFolder(), eCurrentTmpLog, 0, wszLogExt ) );

    if ( eOpenReason != JET_OpenLogForRecoveryCheckingAndPatching )
    {
        Assert( JET_OpenLogForRedo == eOpenReason );
        Assert( lGenNext > 0 );
        CallR( ErrLGOpenLogCallback( m_pinst, m_wszLogName, eOpenReason, lGenNext, fTrue ) );
    }
    else
    {
        Assert( lGenNext == lGenSignalCurrentID );
    }


    err = CIOFilePerf::ErrFileOpen(
                            m_pinst->m_pfsapi,
                            m_pinst,
                            m_wszLogName,
                            FmfLGStreamDefault( IFileAPI::fmfReadOnly ),
                            iofileLog,
                            QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lGenNext ),
                            &m_pfapiLog );
    if ( err < 0 )
    {
        if ( JET_errFileNotFound != err )
        {
            m_pLog->LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
            return err;
        }

        err = ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetTmpLog, NULL );
        if ( err < 0 )
        {

            if ( fPickLogExt && JET_errFileNotFound == err && m_pLog->FLGIsDefaultExt( fFalse, wszLogExt ) )
            {
                wszLogExt = m_pLog->WszLGGetOtherExt( fFalse );
                Assert( wszLogExt != m_wszLogExt );
                goto OpenLog;
            }


            if ( m_wszLogExt == NULL )
            {
                CallS( ErrLGMakeLogNameBaseless( m_wszLogName, sizeof(m_wszLogName), m_pLog->SzLGCurrentFolder(), eCurrentLog, 0, m_pLog->WszLGGetDefaultExt( fFalse ) ) );
            }
            return ErrERRCheck( JET_errFileNotFound );
        }

        CallS( err );

        err = ErrERRCheck( JET_errFileNotFound );
    }
    else
    {
        CallS( err );
    }


    if( fPickLogExt )
    {
        Assert( m_wszLogExt == NULL );
        LGSetLogExt( m_pLog->FLGIsLegacyExt( fFalse, wszLogExt ), fFalse );
        m_pLog->LGSetChkExts( m_pLog->FLGIsLegacyExt( fFalse, wszLogExt ), fFalse );
    }

    return err;
}


ERR LOG_STREAM::ErrLGOpenJetLog( const ULONG eOpenReason, const ULONG lGenNext, BOOL fPickLogExt )
{
    Assert( lGenNext > 0 );
    const ERR errOpen = ErrLGTryOpenJetLog( eOpenReason, lGenNext, fPickLogExt );
    if ( errOpen == JET_errFileNotFound )
    {
        m_pLog->LGReportError( LOG_OPEN_FILE_ERROR_ID, errOpen );
    }
    return errOpen;
}


ERR LOG_STREAM::ErrLGTryOpenLogFile( const ULONG eOpenReason, const enum eLGFileNameSpec eLogType, const LONG lgen )
{
    ERR err = JET_errSuccess;

    Assert( lgen != 0 || eCurrentLog == eLogType );
    Assert( lgen > 0 );
    Assert( JET_OpenLogForRedo == eOpenReason );
    Assert( eCurrentLog == eLogType || eArchiveLog == eLogType );
    Assert( NULL == m_pfapiLog );

    LGMakeLogName( eLogType, ( eLogType != eCurrentLog ) ? lgen : 0 );

    CallR( ErrLGOpenLogCallback( m_pinst, m_wszLogName, eOpenReason, lgen, ( eLogType == eCurrentLog ) ) );

    Call( CIOFilePerf::ErrFileOpen(
                            m_pinst->m_pfsapi,
                            m_pinst,
                            m_wszLogName,
                            FmfLGStreamDefault( IFileAPI::fmfReadOnly ),
                            iofileLog,
                            QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgen ),
                            &m_pfapiLog ) );

HandleError:

    if ( err < JET_errSuccess &&
            JET_errFileNotFound != err )
    {
        m_pLog->LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
    }

    Assert( JET_errMissingLogFile != err &&
            JET_errMissingPreviousLogFile != err &&
            JET_errMissingCurrentLogFiles != err &&
            JET_errMissingRestoreLogFiles != err );

    return err;
}

ERR LOG_STREAM::ErrLGOpenLogFile( const ULONG eOpenReason, const enum eLGFileNameSpec eLogType, const LONG lgen, const ERR errMissingLog )
{
    ERR err = ErrLGTryOpenLogFile( eOpenReason, eLogType, lgen );

    if ( err == JET_errFileNotFound )
    {
        Assert( JET_errMissingLogFile == errMissingLog ||
                JET_errMissingPreviousLogFile == errMissingLog ||
                JET_errMissingCurrentLogFiles == errMissingLog ||
                JET_errMissingRestoreLogFiles == errMissingLog );
        
        m_pLog->LGReportError( LOG_OPEN_FILE_ERROR_ID, err );

        if ( errMissingLog == JET_errMissingPreviousLogFile )
        {
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"afb4f58d-41af-41ad-a3bb-e22c053839c0" );
        }

        err = ErrERRCheck( errMissingLog );
    }

    return err;
}



ERR LOG_STREAM::ErrLGRIOpenRedoLogFile( LGPOS *plgposRedoFrom, INT *pfStatus )
{
    ERR     err;
    BOOL    fJetLog = fFalse;

    
    err = ErrLGTryOpenLogFile( JET_OpenLogForRedo, eArchiveLog, plgposRedoFrom->lGeneration );
    if( err < 0 )
    {
        if ( JET_errFileNotFound != err )
        {
            return err;
        }


        err = ErrLGOpenJetLog( JET_OpenLogForRedo, plgposRedoFrom->lGeneration );
        if ( err < 0 )
        {
            *pfStatus = fNoProperLogFile;
            return JET_errSuccess;
        }

        fJetLog = fTrue;
    }

    
    CallR( ErrLGReadFileHdr( NULL, IorpLogRead( m_pLog ), m_plgfilehdr, fCheckLogID ) );


    
    if( m_plgfilehdr->lgfilehdr.le_lGeneration == plgposRedoFrom->lGeneration )
    {
        *pfStatus = fRedoLogFile;
    }
    else if ( fJetLog && ( m_plgfilehdr->lgfilehdr.le_lGeneration == plgposRedoFrom->lGeneration + 1 ) )
    {
        
        plgposRedoFrom->lGeneration++;
        plgposRedoFrom->isec = (WORD) m_csecHeader;
        plgposRedoFrom->ib   = 0;

        *pfStatus = fRedoLogFile;
    }
    else
    {
        
        delete m_pfapiLog;
        m_pfapiLog = NULL;

        *pfStatus = fNoProperLogFile;
    }

    Assert( err >= 0 );
    return err;
}

ERR
LOG_STREAM::ErrLGICreateOneReservedLogFile(
    const WCHAR * const     wszPath,
    const LONG              lgenDebug )
{
    ERR                     err;
    IFileAPI *              pfapi   = NULL;

    Assert( lgenDebug > 0 || lgenDebug == lgenSignalReserveID );

    const QWORD qwSize = QWORD( m_csecLGFile ) * m_cbSec;
    TraceContextScope tcScope( iorpLog );

    CallR( CIOFilePerf::ErrFileCreate(
                        m_pinst->m_pfsapi,
                        m_pinst,
                        wszPath,
                        FmfLGStreamDefault(),
                        iofileLog,
                        QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgenDebug ),
                        &pfapi ) );

    Call( pfapi->ErrSetSize( *tcScope, qwSize, fTrue, QosSyncDefault( m_pinst ) ) );

HandleError:
    pfapi->SetNoFlushNeeded();
    delete pfapi;

    if ( err < JET_errSuccess )
    {

        const ERR   errDelFile  = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
#ifdef DEBUG
        if ( JET_errSuccess != errDelFile
            && !FRFSFailureDetected( OSFileDelete ) )
        {
            CallS( errDelFile );
        }
#endif
    }

    return err;
}


ERR LOG_STREAM::ErrLGDeleteLegacyReserveLogFiles()
{
    WCHAR           wszPathJetLog[IFileSystemAPI::cchPathMax];
    const WCHAR* const  rgwszLegacyResLogBaseNames[]    = { L"res1", L"res2" };
    const size_t        cLegacyResLogBaseNames      = _countof( rgwszLegacyResLogBaseNames );

    for ( size_t iResLogBaseNames = 0; iResLogBaseNames < cLegacyResLogBaseNames; ++iResLogBaseNames )
    {
        CallS( m_pinst->m_pfsapi->ErrPathBuild( m_pLog->SzLGCurrentFolder(), rgwszLegacyResLogBaseNames[ iResLogBaseNames ], wszOldLogExt, wszPathJetLog ) );
        (void)m_pinst->m_pfsapi->ErrFileDelete( wszPathJetLog );
    }

    return( JET_errSuccess );
}


ERR LOG_STREAM::ErrLGCheckState()
{
    ERR     err     = JET_errSuccess;

    m_rwlLGResFiles.EnterAsReader();


    if ( m_ls != lsNormal )
    {

        m_rwlLGResFiles.LeaveAsReader();
        m_rwlLGResFiles.EnterAsWriter();

        if ( m_ls != lsNormal )
        {
            (void)ErrLGIGrowReserveLogPool( ( BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ? cMinReserveLogsWithAggresiveRollover : cMinReserveLogs ) - m_cReserveLogs,
                                            fTrue );
        }

        m_rwlLGResFiles.LeaveAsWriter();
        m_rwlLGResFiles.EnterAsReader();


        if ( m_ls != lsNormal )
        {
            err = ErrERRCheck( JET_errLogDiskFull );
        }
    }


    if ( err >= JET_errSuccess && m_fLogSequenceEnd )
    {
        err = ErrERRCheck( JET_errLogSequenceEnd );
    }

    m_rwlLGResFiles.LeaveAsReader();

    return err;
}


ERR LOG_STREAM::ErrLGInitCreateReserveLogFiles()
{
    WCHAR           wszPathJetLog[IFileSystemAPI::cchPathMax];
    WCHAR           wszResBaseName[16];
    const ULONG     cInitialReserveLogs = BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ? cMinReserveLogsWithAggresiveRollover : cMinReserveLogs;


    m_rwlLGResFiles.EnterAsWriter();
    m_ls = lsNormal;


    IFileFindAPI*   pffapi          = NULL;
    QWORD           cbSize;
    const QWORD     cbSizeExpected  = QWORD( m_csecLGFile ) * m_cbSec;

    ErrLGDeleteLegacyReserveLogFiles();


    LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eReserveLog, 0 );
    OSStrCbCopyW( wszResBaseName, sizeof(wszResBaseName), SzParam( m_pinst, JET_paramBaseName ) );
    OSStrCbAppendW( wszResBaseName, sizeof(wszResBaseName), wszLogRes );

    if ( m_pinst->m_pfsapi->ErrFileFind( wszPathJetLog, &pffapi ) == JET_errSuccess )
    {
        while ( pffapi->ErrNext() == JET_errSuccess )
        {
            LONG lResLogNum = 0;
            ULONG cLogDigits = 0;

            if ( pffapi->ErrPath( wszPathJetLog ) != JET_errSuccess ||
                pffapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) != JET_errSuccess ||
                LGFileHelper::ErrLGGetGeneration( m_pinst->m_pfsapi, wszPathJetLog, wszResBaseName, &lResLogNum, wszResLogExt, &cLogDigits ) < JET_errSuccess
                )
            {
                continue;
            }

            if ( cbSizeExpected != cbSize
                || lResLogNum > (LONG)cInitialReserveLogs
                || cLogDigits != 5 )
            {
                (void)m_pinst->m_pfsapi->ErrFileDelete( wszPathJetLog );
            }


        }

        delete pffapi;
        pffapi = NULL;
    }



    m_cReserveLogs = 0;

    (void)ErrLGIGrowReserveLogPool( cInitialReserveLogs, fFalse );


    m_rwlLGResFiles.LeaveAsWriter();
    return JET_errSuccess;
}


ERR LOG_STREAM::ErrLGIGrowReserveLogPool(
    ULONG       cResLogsInc,
    BOOL        fUseDefaultFolder
    )
{
    ERR         err = JET_errSuccess;
    const LS        lsBefore                = m_ls;
    BOOL        fQuiesce                = fFalse;
    ULONG       ithReserveLog;
    ULONG       cTargetReserveLogs;
    WCHAR       wszPathJetLog[IFileSystemAPI::cchPathMax];

    Assert( m_rwlLGResFiles.FWriter() );
    Assert( cResLogsInc );

    if ( lsBefore == lsOutOfDiskSpace )
        return ErrERRCheck( JET_errLogDiskFull );


    cTargetReserveLogs = ( m_cReserveLogs + cResLogsInc );
    Assert( cTargetReserveLogs < UlParam( m_pinst, JET_paramCheckpointTooDeep ) );

    for ( ithReserveLog = m_cReserveLogs+1; ithReserveLog <= cTargetReserveLogs; ithReserveLog++ )
    {
        LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eReserveLog, ithReserveLog, fUseDefaultFolder );

        err     = ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetLog );

        if ( JET_errFileNotFound == err )
        {
            err = ErrLGICreateOneReservedLogFile( wszPathJetLog, lgenSignalReserveID );
        }

        if ( err < 0 )
        {
            if ( m_ls == lsNormal )
            {
                UtilReportEvent(
                        eventError,
                        LOGGING_RECOVERY_CATEGORY,
                        LOW_LOG_DISK_SPACE,
                        0,
                        NULL,
                        0,
                        NULL,
                        m_pinst );
            }

            fQuiesce = fTrue;
            break;
        }

        m_cReserveLogs++;

    }

    if ( cResLogsInc )
    {
        m_ls = ( fQuiesce ? lsQuiesce : lsNormal );
    }

    return(err);
}



ERR LOG_STREAM::ErrLGIOpenReserveLogFile(
    IFileAPI** const        ppfapi,
    const WCHAR* const      wszPathJetTmpLog,
    const LONG              lgenNextDebug
    )
{
    ERR     err = JET_errSuccess;
    WCHAR   wszLogName[ IFileSystemAPI::cchPathMax ];
    ULONG   ithReserveLog;

    Assert( m_pinst->m_pfsapi );
    Assert( ppfapi );
    Assert( wszPathJetTmpLog );
    Assert( lgenNextDebug > 0 );

    *ppfapi = NULL;

    Assert( m_rwlLGResFiles.FNotWriter() );
    m_rwlLGResFiles.EnterAsWriter();

    
    for( ithReserveLog = m_cReserveLogs; ithReserveLog > 0; ithReserveLog-- )
    {

        LGMakeLogName( wszLogName, sizeof(wszLogName), eReserveLog, ithReserveLog );

        err = m_pinst->m_pfsapi->ErrFileMove( wszLogName, wszPathJetTmpLog );
        Assert( err < 0 || err == JET_errSuccess );
        if ( err == JET_errSuccess )
        {

            m_cReserveLogs--;

            Assert( lgenNextDebug > 0 );
            err = CIOFilePerf::ErrFileOpen(
                                    m_pinst->m_pfsapi,
                                    m_pinst,
                                    wszPathJetTmpLog,
                                    FmfLGStreamDefault(),
                                    iofileLog,
                                    QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgenNextDebug ),
                                    ppfapi );
            if ( ! *ppfapi )
            {
                if ( ithReserveLog > 1 )
                {
                    (void) m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog );
                }
            }
            else
            {
                Assert( err >= JET_errSuccess );
                break;
            }

        }

    }

    Assert( m_cReserveLogs == ithReserveLog - 1 || m_cReserveLogs == 0 );

    if ( m_ls == lsNormal )
    {
        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                LOW_LOG_DISK_SPACE,
                0,
                NULL,
                0,
                NULL,
                m_pinst );

        m_ls = lsQuiesce;
    }

    if ( ! *ppfapi )
    {


        Assert( err < JET_errSuccess || m_cReserveLogs == 0 );
        Assert( m_ls == lsQuiesce || m_ls == lsOutOfDiskSpace );

        err = ErrERRCheck( JET_errLogDiskFull );

        if ( m_ls == lsQuiesce )
        {
            const WCHAR*    rgpsz[ 1 ];
            DWORD           irgpsz      = 0;
            WCHAR           wszJetLog[ IFileSystemAPI::cchPathMax ];

            LGMakeLogName( wszJetLog, sizeof(wszJetLog), eCurrentLog );

            rgpsz[ irgpsz++ ]   = wszJetLog;

            UtilReportEvent(
                    eventError,
                    LOGGING_RECOVERY_CATEGORY,
                    LOG_DISK_FULL,
                    irgpsz,
                    rgpsz,
                    0,
                    NULL,
                    m_pinst );

            m_ls = lsOutOfDiskSpace;
        }
    }
    else
    {
        Assert( err >= JET_errSuccess );
    }

    Assert( m_ls == lsQuiesce || m_ls == lsOutOfDiskSpace );

    m_rwlLGResFiles.LeaveAsWriter();

    return err;
}



ERR LOG_STREAM::ErrLGIOpenTempLogFile(
    IFileAPI** const        ppfapi,
    const WCHAR* const      wszPathJetTmpLog,
    const LONG              lgenNextDebug,
    BOOL* const             pfResUsed,
    BOOL* const             pfZeroFilled
    )
{
    ERR err;

    Assert( m_pinst->m_pfsapi );
    Assert( ppfapi );
    Assert( wszPathJetTmpLog );
    Assert( pfResUsed );
    Assert( pfZeroFilled );
    Assert( lgenNextDebug > 0 || lgenNextDebug == lGenSignalTempID );

    const QWORD qwSize = QWORD( m_csecLGFile ) * m_cbSec;
    TraceContextScope tcScope( iorpLog );

    *ppfapi = NULL;
    *pfResUsed = fFalse;
    *pfZeroFilled = fFalse;


    if ( ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetTmpLog ) >= JET_errSuccess )
    {

        Call( CIOFilePerf::ErrFileOpen(
                                m_pinst->m_pfsapi,
                                m_pinst,
                                wszPathJetTmpLog,
                                FmfLGStreamDefault(),
                                iofileLog,
                                QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgenNextDebug ),
                                ppfapi ) );

        Call( (*ppfapi)->ErrSetSize( *tcScope, 0, fTrue, QosSyncDefault( m_pinst ) ) );
        Call( (*ppfapi)->ErrSetSize( *tcScope, qwSize, fFalse, QosSyncDefault( m_pinst ) ) );
    }
    else
    {

        err = ErrLGIReuseArchivedLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug );


        if ( NULL != *ppfapi )
        {
            CallS( err );

            QWORD   qwFileSize;
            Call( (*ppfapi)->ErrSize( &qwFileSize, IFileAPI::filesizeLogical ) );
            if ( qwFileSize == qwSize )
            {
                LGFILEHDR * const plgfilehdr = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof( LGFILEHDR ), NULL );
                if ( plgfilehdr != NULL )
                {
                    TraceContextScope tcScopeT( iorpHeader );
                    if ( ErrLGIReadFileHeader( *ppfapi, *tcScopeT, QosSyncDefault( m_pinst ), plgfilehdr ) >= JET_errSuccess &&
                         FLGVersionZeroFilled( &plgfilehdr->lgfilehdr ) )
                    {
                        *pfZeroFilled = fTrue;
                    }

                    OSMemoryPageFree( plgfilehdr );
                }
            }
        }
        else
        {
            Assert( lgenNextDebug > 0 || lgenNextDebug == lGenSignalTempID );
            err = CIOFilePerf::ErrFileCreate(
                                    m_pinst->m_pfsapi,
                                    m_pinst,
                                    wszPathJetTmpLog,
                                    FmfLGStreamDefault(),
                                    iofileLog,
                                    QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgenNextDebug ),
                                    ppfapi );
            if ( err < JET_errSuccess )
            {
                Assert( NULL == *ppfapi );


                if ( JET_errDiskFull == err )
                {
                    Assert( lgenNextDebug > 0 || lgenNextDebug == lGenSignalTempID );
                    Call( ErrLGIOpenReserveLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug ) );
                    *pfResUsed = fTrue;
                    *pfZeroFilled = fTrue;
                    Assert( *ppfapi );
                }
            }
            else
            {
                Assert( NULL != *ppfapi );

                Call( (*ppfapi)->ErrSetSize( *tcScope, qwSize, fFalse, QosSyncDefault( m_pinst ) ) );
            }
        }
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( *ppfapi )
        {
            (*ppfapi)->SetNoFlushNeeded();
            delete *ppfapi;
            *ppfapi = NULL;

            (void)m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog );
        }
        *pfResUsed = fFalse;
    }

    Assert( ( err >= JET_errSuccess && NULL != *ppfapi )
            || ( err < JET_errSuccess && NULL == *ppfapi ) );

    return err;
}

ERR LOG_STREAM::ErrLGIReuseArchivedLogFile(
    IFileAPI** const        ppfapi,
    const WCHAR* const      wszPathJetTmpLog,
    const LONG              lgenNextDebug
    )
{
    ERR     err = JET_errSuccess;

    Assert( m_pinst->m_pfsapi );
    Assert( ppfapi );
    Assert( wszPathJetTmpLog );
    Assert( lgenNextDebug > 0 || lgenNextDebug == lGenSignalTempID );

    *ppfapi = NULL;




    if ( BoolParam( m_pinst, JET_paramCircularLog ) )
    {
        LONG lgenLGGlobalOldest = 0;

        (void)ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenLGGlobalOldest, NULL );

        if ( lgenLGGlobalOldest != 0
            && ( !m_pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) || lgenLGGlobalOldest < m_pinst->m_pbackup->BKLgenCopyMic() ) )
        {
            (VOID) m_pLog->ErrLGUpdateCheckpointFile( fFalse );
            if ( lgenLGGlobalOldest < m_pLog->LgposGetCheckpoint().le_lGeneration )
            {
                WCHAR   wszLogName[ IFileSystemAPI::cchPathMax ];

                LGMakeLogName( wszLogName, sizeof(wszLogName), eArchiveLog, lgenLGGlobalOldest );
                err = m_pinst->m_pfsapi->ErrFileMove( wszLogName, wszPathJetTmpLog );
                Assert( err <= JET_errSuccess );
                if ( err == JET_errSuccess )
                {
                    lgenLGGlobalOldest++;
                    err = CIOFilePerf::ErrFileOpen(
                                            m_pinst->m_pfsapi,
                                            m_pinst,
                                            wszPathJetTmpLog,
                                            FmfLGStreamDefault(),
                                            iofileLog,
                                            QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgenNextDebug ),
                                            ppfapi );
                    if ( err < JET_errSuccess )
                    {
                        (VOID) m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog );
                    }

                    else
                    {
                        do
                        {
                            Assert( lgenLGGlobalOldest <= m_pLog->LgposGetCheckpoint().le_lGeneration );

                            if ( m_pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) )
                            {
                                break;
                            }

                            LONG cExtraLogsToKeep = 2;
                            if ( ( UlParam( m_pinst, JET_paramPersistedLostFlushDetection ) & JET_bitPersistedLostFlushDetectionEnabled ) != 0 )
                            {
                                const LE_LGPOS lgposChkpt = m_pLog->LgposGetCheckpoint();
                                LGPOS lgposTip;
                                m_pLog->LGLogTipWithLock( &lgposTip );
                                LONG lCurrentChkptDepth = lgposTip.lGeneration - lgposChkpt.le_lGeneration;
                                Expected( lCurrentChkptDepth >= 0 );
                                lCurrentChkptDepth = LFunctionalMax( lCurrentChkptDepth, 0 );
                                cExtraLogsToKeep += ( ( lCurrentChkptDepth * CFlushMap::s_pctCheckpointDelay ) / 100 );
                            }

                            if ( lgenLGGlobalOldest >= ( m_pLog->LgposGetCheckpoint().le_lGeneration - cExtraLogsToKeep ) )
                            {
                                break;
                            }

                            LGMakeLogName( wszLogName, sizeof(wszLogName), eArchiveLog, lgenLGGlobalOldest );
                            lgenLGGlobalOldest++;
                        }
                        while ( JET_errSuccess == m_pinst->m_pfsapi->ErrFileDelete( wszLogName ) );
                    }
                }
            }
        }
    }

    return err;
}

DWORD LOG_STREAM::CbLGICreateAsynchIOSize()
{
    Assert( m_ibJetTmpLog <= QWORD( CbLGLogFileSize() ) );
    const QWORD cbIO = min(
        cbLogExtendPattern - m_ibJetTmpLog % cbLogExtendPattern,
        QWORD( CbLGLogFileSize() ) - m_ibJetTmpLog );
    Assert( DWORD( cbIO ) == cbIO );
    return DWORD( cbIO );
}

BOOL LOG_STREAM::FShouldCreateTmpLog( const LGPOS &lgposLogRec )
{
    BOOL fRetVal = fFalse;
    if ( CmpLgpos( lgposLogRec, m_lgposCreateAsynchTrigger ) >= 0
        && m_asigCreateAsynchIOCompleted.FTryWait() )
    {
        Assert( BoolParam( m_pinst, JET_paramLogFileCreateAsynch ) );

        if ( m_pfapiJetTmpLog )
        {
            m_lgposCreateAsynchTrigger = lgposMax;
            fRetVal = fTrue;
        }
        else
        {
            m_asigCreateAsynchIOCompleted.Set();
        }
    }

    return fRetVal;
}

VOID LOG_STREAM::LGCreateAsynchIOIssue( const LGPOS& lgposLogRec )
{

    QWORD cbIOIssued = 0;
    DWORD cIOIssued = 0;
    if ( m_critJetTmpLog.FTryEnter() )
    {
        if ( NULL != m_pfapiJetTmpLog && !m_fCreateAsynchZeroFilled )
        {
            Assert( m_ibJetTmpLog <= QWORD( CbLGLogFileSize() ) );
            Assert( m_cIOTmpLog == 0 );

            const QWORD cbDeltaLogRec = ( CmpLgpos( lgposLogRec, m_lgposLogRecAsyncIOIssue ) < 0 ) ? 0 : CbOffsetLgpos( lgposLogRec, m_lgposLogRecAsyncIOIssue );
            DWORD cIO = static_cast<DWORD>( min( max( cbDeltaLogRec / cbLogExtendPattern, 1 ), m_cIOTmpLogMax ) );

            const QWORD ibJetTmpLog = m_ibJetTmpLog;
            for ( DWORD iIO = 0 ; ( iIO < cIO ) && ( m_ibJetTmpLog < QWORD( CbLGLogFileSize() ) ) ; iIO++ )
            {
                m_rgibTmpLog[ iIO ] = m_ibJetTmpLog;
                m_rgcbTmpLog[ iIO ] = CbLGICreateAsynchIOSize();
                m_ibJetTmpLog += m_rgcbTmpLog[ iIO ];
                cbIOIssued += m_rgcbTmpLog[ iIO ];
                cIOIssued++;
            }

            if ( cIOIssued > 0 )
            {
                m_cIOTmpLog = cIOIssued;

                const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
                
                if ( g_taskmgrLog.ErrTMPost( (CGPTaskManager::PfnCompletion)LGWriteTmpLog_, this ) < JET_errSuccess )
                {
                    m_cIOTmpLog = cIOIssued = 0;
                    cbIOIssued = 0;
                    m_ibJetTmpLog = ibJetTmpLog;
                }
                
                FOSSetCleanupState( fCleanUpStateSaved );
            }
        }
        
        m_critJetTmpLog.Leave();
    }

    if ( cIOIssued == 0 )
    {
        m_asigCreateAsynchIOCompleted.Set();
    }
}

VOID LOG_STREAM::LGWriteTmpLog_( LOG_STREAM* const pLogStream )
{
    pLogStream->LGWriteTmpLog();
}

VOID LOG_STREAM::LGWriteTmpLog()
{
    if ( m_critCreateAsynchIOExecuting.FTryEnter() )
    {
        const OSFILEQOS     qos                 = QosSyncDefault( m_pinst ) | qosIOPoolReserved;
        const BYTE* const   pbData              = g_rgbZero;
        const DWORD         cIO                 = m_cIOTmpLog;

        BOOL fBackoff = fFalse;
        TraceContextScope tcScope( iorpLog );
        tcScope->iorReason.AddFlag( iorfFill );

        for ( DWORD iIO = 0 ; iIO < cIO ; )
        {
            const QWORD ibOffset    = m_rgibTmpLog[ iIO ];
            const DWORD cbData      = m_rgcbTmpLog[ iIO ];

            const JET_ERR err = m_pfapiJetTmpLog->ErrIOWrite( *tcScope, ibOffset, cbData, pbData, qos );
            const BOOL fAsyncIOCompleted = ++iIO == cIO;
            Assert( m_cIOTmpLog > 0 );

            m_pLogWriteBuffer->LockBuffer();
            if ( err >= JET_errSuccess )
            {
                if ( !fBackoff )
                {
                    AddLgposCanSwitchLogFile( &m_lgposLogRecAsyncIOIssue, cbData );
                    if ( ( m_ibJetTmpLog < QWORD( CbLGLogFileSize() ) ) && fAsyncIOCompleted )
                    {
                        m_lgposCreateAsynchTrigger = m_lgposLogRecAsyncIOIssue;
                    }

                }
            }
            else
            {
                if ( m_errCreateAsynch == JET_errSuccess )
                {
                    m_errCreateAsynch = err;
                    if ( err == JET_errOutOfMemory && ibOffset < m_ibJetTmpLog )
                    {
                        m_errCreateAsynch = JET_errSuccess;
                        m_ibJetTmpLog = ibOffset;
                        fBackoff = fTrue;
                    }
                }
            }
            m_pLogWriteBuffer->UnlockBuffer();
        }

        Assert( cIO == m_cIOTmpLog );
        m_cIOTmpLog = 0;

        m_critCreateAsynchIOExecuting.Leave();
    }


    Assert( !m_asigCreateAsynchIOCompleted.FTryWait() );
    m_asigCreateAsynchIOCompleted.Set();
}


VOID LOG_STREAM::LGCreateAsynchCancel( const BOOL fWaitForTask )
{

    if ( m_pfapiJetTmpLog ||
        m_errCreateAsynch < JET_errSuccess )
    {
        Assert( BoolParam( m_pinst, JET_paramLogFileCreateAsynch ) );

        m_critCreateAsynchIOExecuting.Enter();

        m_critJetTmpLog.Enter();

        if ( fWaitForTask )
        {
            m_asigCreateAsynchIOCompleted.Wait();
        }

        if ( m_pfapiJetTmpLog )
        {
            m_pfapiJetTmpLog->SetNoFlushNeeded();
        }
        delete m_pfapiJetTmpLog;
        m_pfapiJetTmpLog = NULL;
        m_fCreateAsynchResUsed = fFalse;
        m_fCreateAsynchZeroFilled = fFalse;
        m_errCreateAsynch = JET_errSuccess;
        m_cIOTmpLog = 0;
        m_lgposCreateAsynchTrigger = lgposMax;

        m_ibJetTmpLog = 0;

        m_critJetTmpLog.Leave();
        m_critCreateAsynchIOExecuting.Leave();
    }
}

VOID LOG_STREAM::CheckLgposCreateAsyncTrigger()
{
    if ( CmpLgpos( lgposMax, m_lgposCreateAsynchTrigger ) )
    {
        m_pLogWriteBuffer->LockBuffer();
        m_lgposCreateAsynchTrigger = lgposMax;
        m_pLogWriteBuffer->UnlockBuffer();
    }
}


ERR LOG_STREAM::ErrOpenTempLogFile(
    IFileAPI ** const       ppfapi,
    const WCHAR * const     wszPathJetTmpLog,
    const LONG              lgenNextDebug,
    BOOL * const            pfResUsed,
    BOOL * const            pfZeroFilled,
    QWORD * const           pibPattern )
{
    ERR err = JET_errSuccess;

    Assert( lgenNextDebug > 0 );

    if ( ( m_errCreateAsynch < JET_errSuccess ) || m_pfapiJetTmpLog )
    {
        Assert( BoolParam( m_pinst, JET_paramLogFileCreateAsynch ) );

        m_critCreateAsynchIOExecuting.Enter();

        m_critJetTmpLog.Enter();

        if ( m_cIOTmpLog != 0 )
    {
            m_ibJetTmpLog = m_rgibTmpLog[ 0 ];
            m_cIOTmpLog = 0;
    }

        *ppfapi     = m_pfapiJetTmpLog;
        *pfResUsed  = m_fCreateAsynchResUsed;
        *pfZeroFilled = m_fCreateAsynchZeroFilled;

        if ( m_errCreateAsynch >= JET_errSuccess )
        {
            *pibPattern = m_ibJetTmpLog;
        }

        err         = m_errCreateAsynch;

        m_pfapiJetTmpLog        = NULL;
        m_fCreateAsynchResUsed  = fFalse;
        m_fCreateAsynchZeroFilled = fFalse;
        m_errCreateAsynch       = JET_errSuccess;
        m_ibJetTmpLog           = 0;
        m_critJetTmpLog.Leave();
        m_critCreateAsynchIOExecuting.Leave();

        CheckLgposCreateAsyncTrigger();

        if ( JET_errDiskFull == err && ! *pfResUsed )
        {
            Assert( JET_errDiskFull == err );
            Assert( ! *pfResUsed );

            delete *ppfapi;
            *ppfapi = NULL;


            Call( m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog ) );

            Call( ErrLGIOpenReserveLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug ) );
            *pfResUsed = fTrue;
            *pfZeroFilled = fTrue;
        }
        else
        {
            Call( err );
        }
    }
    else
    {
        Call( ErrLGIOpenTempLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug, pfResUsed, pfZeroFilled ) );
    }

    Assert( *ppfapi );

HandleError:
    return err;
}

ERR LOG_STREAM::ErrFormatLogFile(
    IFileAPI ** const       ppfapi,
    const WCHAR * const     wszPathJetTmpLog,
    const LONG              lgenNextDebug,
    BOOL * const            pfResUsed,
    const QWORD&            cbLGFile,
    const QWORD&            ibPattern )
{
    ERR err = JET_errSuccess;

    TraceContextScope tcScope( iorpLog );
    tcScope->iorReason.AddFlag( iorfFill );
    err = ErrUtilFormatLogFile( m_pinst->m_pfsapi, *ppfapi, cbLGFile, ibPattern, QosSyncDefault( m_pinst ), fTrue, *tcScope );
    if ( JET_errDiskFull == err && ! *pfResUsed )
    {
        delete *ppfapi;
        *ppfapi = NULL;


        Call( m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog ) );

        Call( ErrLGIOpenReserveLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug ) );
        *pfResUsed = fTrue;
    }
    else
    {
        Call( err );
    }

HandleError:
    return err;
}




ERR LOG_STREAM::ErrLGIWriteFileHdr(
    IFileAPI* const         pfapi,
    LGFILEHDR* const        plgfilehdr)
{
    ERR err = JET_errSuccess;

    Assert( m_critLGWrite.FOwner() );

    Assert( plgfilehdr == m_plgfilehdrT );

    Enforce( m_csecHeader * m_cbSec >= sizeof(LGFILEHDR) );

    const LogVersion * plgvDesired = &( PfmtversEngineSafetyVersion()->lgv );
    const JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion );
    const ERR errT = ErrLGGetDesiredLogVersion( efv, &plgvDesired );
    if ( errT < JET_errSuccess )
    {
        FireWall( OSFormat( "GetDesiredLogVerWriteFileHdrFailedVer:%lu", (ULONG)efv ) );
    }

    Assert( CmpLgVer( LgvFromLgfilehdr( plgfilehdr ), *plgvDesired ) >= 0 );

    SetPageChecksum( plgfilehdr, sizeof( LGFILEHDR ), logfileHeader, 0 );


    Call( ErrLGIWriteSectorData( pfapi, IOR( iorpLog, iorsHeader ), plgfilehdr->lgfilehdr.le_lGeneration, 0, sizeof( LGFILEHDR ), (BYTE*)plgfilehdr, LOG_HEADER_WRITE_ERROR_ID, NULL, fTrue ) );
    
    Assert( m_critLGWrite.FOwner() );

HandleError:
    return err;
}


ERR LOG_STREAM::ErrStartAsyncLogFileCreation(
    _In_ const WCHAR * const  wszPathJetTmpLog,
    _In_       LONG           lgenDebug
    )
{
    Assert( m_critLGWrite.FOwner() );

    if ( m_cIOTmpLogMax > 0 )
    {
        Assert( !m_pfapiJetTmpLog );
        Assert( !m_fCreateAsynchResUsed );
        Assert( !m_fCreateAsynchZeroFilled );
        Assert( JET_errSuccess == m_errCreateAsynch );
        Assert( 0 == m_ibJetTmpLog );
        Assert( NULL != m_rgibTmpLog );
        Assert( NULL != m_rgcbTmpLog );

        Expected( m_plgfilehdrT->lgfilehdr.le_lGeneration == lgenDebug || 
                    ( lGenSignalTempID == lgenDebug && m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 ) );
        if ( lGenSignalTempID == lgenDebug )
        {
            lgenDebug = ( m_plgfilehdrT && m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 ) ? m_plgfilehdrT->lgfilehdr.le_lGeneration : lGenSignalTempID;
        }


        m_errCreateAsynch = ErrLGIOpenTempLogFile(
                                &m_pfapiJetTmpLog,
                                wszPathJetTmpLog,
                                lgenDebug,
                                &m_fCreateAsynchResUsed,
                                &m_fCreateAsynchZeroFilled );

        if ( m_errCreateAsynch >= JET_errSuccess )
        {
            m_pLogWriteBuffer->LockBuffer();
            LGPOS lgpos = m_pLogWriteBuffer->LgposLogTip();
            m_lgposLogRecAsyncIOIssue = lgpos;
            const DWORD cbDelta = CbLGICreateAsynchIOSize() / 2;
            AddLgposCanSwitchLogFile( &lgpos, cbDelta );
            m_lgposCreateAsynchTrigger = lgpos;
            m_pLogWriteBuffer->UnlockBuffer();
        }
    }

    return JET_errSuccess;
}


ERR LOG_STREAM::ErrLGGetDesiredLogVersion( _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const LogVersion ** const pplgv )
{
    ERR err = JET_errSuccess;
    Assert( pplgv );

    const FormatVersions * pfmtversLogDesired = NULL;
    if ( (LONG)UlParam( m_pinst, JET_paramEngineFormatVersion ) == JET_efvUsePersistedFormat )
    {
        Call( ErrLGFindHighestMatchingLogMajors( m_lgvHighestRecovered, &pfmtversLogDesired ) );
    }
    else
    {
        Call( ErrGetDesiredVersion( m_pinst, (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion ), &pfmtversLogDesired ) );
    }

    *pplgv = &( pfmtversLogDesired->lgv );
    OSTrace( JET_tracetagInitTerm, OSFormat( "Desired LG ver (%d) - %d.%d.%d\n\n",
                efv,
                pfmtversLogDesired->lgv.ulLGVersionMajor, pfmtversLogDesired->lgv.ulLGVersionUpdateMajor, pfmtversLogDesired->lgv.ulLGVersionUpdateMinor ) );

HandleError:

    return err;
}

VOID LOG_STREAM::InitLgfilehdrT()
{
    Assert( m_pLogWriteBuffer->FOwnsBufferLock() );
    const LogVersion lgvNull = { 0, 0, 0 };
    static LogVersion lgvLastReported = { 0, 0, 0 };

    const LogVersion * plgvDesired = &( PfmtversEngineSafetyVersion()->lgv );
    const JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion );
    const ERR errT = ErrLGGetDesiredLogVersion( efv, &plgvDesired );
    if ( errT < JET_errSuccess )
    {
        FireWall( OSFormat( "GetDesiredLogVerInitLgHdrVer:%lu", (ULONG)efv ) );
    }

    m_plgfilehdrT->lgfilehdr.le_ulMajor         = plgvDesired->ulLGVersionMajor;
    C_ASSERT( ulLGVersionMinorFinalDeprecatedValue == 4000 );
    m_plgfilehdrT->lgfilehdr.le_ulMinor         = ulLGVersionMinorFinalDeprecatedValue;
    m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor   = plgvDesired->ulLGVersionUpdateMajor;
    m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor   = plgvDesired->ulLGVersionUpdateMinor;

    m_plgfilehdrT->lgfilehdr.le_efvEngineCurrentDiagnostic = EfvMaxSupported();


    if ( CmpLgVer( LgvFromLgfilehdr( m_plgfilehdrT ), lgvLastReported ) != 0 )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "%hs m_plgfilehdrT to (%d) - %d.%d.%d\n",
                    CmpLgVer( lgvLastReported, lgvNull ) == 0 ? "Initializing" : "Upgrading",
                    (ULONG)UlParam( m_pinst, JET_paramEngineFormatVersion ),
                    (ULONG)m_plgfilehdrT->lgfilehdr.le_ulMajor, (ULONG)m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor, (ULONG)m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor ) );
        lgvLastReported = LgvFromLgfilehdr( m_plgfilehdrT );
    }

    Assert( m_cbSec == LOG_SEGMENT_SIZE );
    m_plgfilehdrT->lgfilehdr.le_cbSec       = USHORT( m_cbSec );
    m_plgfilehdrT->lgfilehdr.le_csecHeader  = USHORT( m_csecHeader );
    m_plgfilehdrT->lgfilehdr.le_csecLGFile  = USHORT( m_csecLGFile );
    m_plgfilehdrT->lgfilehdr.bDiskSecSizeMismatch = bTrueDiskSizeUnknown;

    m_plgfilehdrT->lgfilehdr.le_cbPageSize = USHORT( g_cbPage );

    m_plgfilehdrT->lgfilehdr.le_lgposCheckpoint = m_pLog->LgposGetCheckpoint();
}


VOID LOG_STREAM::LGSetFlagsInLgfilehdrT( const BOOL fResUsed )
{
    m_plgfilehdrT->lgfilehdr.fLGFlags = 0;
    m_plgfilehdrT->lgfilehdr.fLGFlags |= fResUsed ? fLGReservedLog : 0;
    m_plgfilehdrT->lgfilehdr.fLGFlags |= BoolParam( m_pinst, JET_paramCircularLog ) ? fLGCircularLoggingCurrent : 0;
    m_plgfilehdrT->lgfilehdr.fLGFlags |= ( m_plgfilehdr->lgfilehdr.fLGFlags & ( fLGCircularLoggingCurrent | fLGCircularLoggingHistory ) ) ? fLGCircularLoggingHistory : 0;
}


VOID LOG_STREAM::SetSignatureInLgfilehdrT()
{
    if ( m_pLog->LgenInitial() == m_plgfilehdrT->lgfilehdr.le_lGeneration )
    {
        
        SIGGetSignature( &m_plgfilehdrT->lgfilehdr.signLog );


        Assert( m_pLog->LgposGetCheckpoint().le_lGeneration <= m_pLog->LgenInitial() );

        m_pLog->ResetCheckpoint( &m_plgfilehdrT->lgfilehdr.signLog );
        
        
        (VOID) m_pLog->ErrLGUpdateCheckpointFile( fTrue );
    }
    else
    {
        Assert( m_pLog->LgenInitial() < m_plgfilehdrT->lgfilehdr.le_lGeneration );
        m_plgfilehdrT->lgfilehdr.signLog = m_pLog->SignLog();
    }
}


VOID LOG_STREAM::CheckForGenerationOverflow()
{

    const LONG  lGenCur = m_plgfilehdr->lgfilehdr.le_lGeneration;

    Assert( lGenerationMax < lGenerationMaxDuringRecovery );
    const LONG  lGenMax = ( m_pLog->FRecovering() ? lGenerationMaxDuringRecovery : lGenerationMax );
    if ( lGenCur >= lGenMax )
    {

        Assert( !m_pLog->FRecovering() );


        if ( !m_fLogSequenceEnd )
        {
            SetLogSequenceEnd();
        }
        else
        {
            Assert( lGenCur > lGenMax );
        }
    }

    else if ( lGenCur > lGenerationMaxWarningThreshold )
    {
        const LONG  lFrequency  = ( lGenCur > lGenerationMaxPanicThreshold ?
                                            lGenerationMaxPanicFrequency :
                                            lGenerationMaxWarningFrequency );
        if ( lGenCur % lFrequency == 0 )
        {
            WCHAR szGenCurr[64];
            WCHAR szGenLast[64];
            WCHAR szGenDiff[64];
            const WCHAR *rgpszString[] = { szGenCurr, szGenLast, szGenDiff };

            OSStrCbFormatW( szGenCurr, sizeof(szGenCurr), L"%d (0x%08X)", lGenCur, lGenCur);
            OSStrCbFormatW( szGenLast, sizeof(szGenLast), L"%d (0x%08X)", lGenMax, lGenMax);
            OSStrCbFormatW( szGenDiff, sizeof(szGenDiff), L"%d (0x%08X)", lGenMax - lGenCur, lGenMax - lGenCur );

            UtilReportEvent(
                    eventWarning,
                    LOGGING_RECOVERY_CATEGORY,
                    ALMOST_OUT_OF_LOG_SEQUENCE_ID,
                    _countof( rgpszString ),
                    rgpszString,
                    0,
                    NULL,
                    m_pinst );

            OSUHAPublishEvent(
                HaDbFailureTagAlertOnly, m_pinst, HA_LOGGING_RECOVERY_CATEGORY,
                HaDbIoErrorNone, NULL, 0, 0,
                HA_ALMOST_OUT_OF_LOG_SEQUENCE_ID, _countof( rgpszString ), rgpszString );
        }
    }
}

ERR LOG_STREAM::ErrLGIStartNewLogFile(
    const LONG              lgenToClose,
    BOOL                    fLGFlags,
    IFileAPI ** const       ppfapiTmpLog
    )
{
    ERR             err         = JET_errSuccess;
    WCHAR           wszPathJetLog[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszPathJetTmpLog[ IFileSystemAPI::cchPathMax ];
    BOOL            fResUsed    = fFalse;
    BOOL            fZeroFilled = fFalse;
    const QWORD     cbLGFile    = QWORD( m_csecLGFile ) * m_cbSec;
    QWORD           ibPattern   = 0;

    fLGFlags &= ~fLGLogAttachments;

    Assert( 0 == fLGFlags
        || fLGOldLogExists == fLGFlags
        || fLGOldLogNotExists == fLGFlags
        || fLGOldLogInBackup == fLGFlags );

    Assert( m_critLGWrite.FOwner() );

    CheckForGenerationOverflow();

    LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eCurrentLog );
    LGMakeLogName( wszPathJetTmpLog, sizeof(wszPathJetTmpLog), eCurrentTmpLog );

    Call( ErrOpenTempLogFile(
            ppfapiTmpLog,
            wszPathJetTmpLog,
            lgenToClose + 1,
            &fResUsed,
            &fZeroFilled,
            &ibPattern ) );

    if ( !fZeroFilled )
    {
        Call( ErrFormatLogFile(
                ppfapiTmpLog,
                wszPathJetTmpLog,
                lgenToClose + 1,
                &fResUsed,
                cbLGFile,
                ibPattern ) );
    }

    QWORD   cbFileSize;
    Call( (*ppfapiTmpLog)->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
    if ( cbLGFile != cbFileSize )
    {
        FireWall( "LogFileSizeMismatch" );
        Error( ErrERRCheck( JET_errLogFileSizeMismatch ) );
    }


    m_pLogWriteBuffer->LockBuffer();

    if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
    {
        m_plgfilehdrT->lgfilehdr.tmPrevGen = m_plgfilehdr->lgfilehdr.tmCreate;
        m_plgfilehdrT->lgfilehdr.checksumPrevLogAllSegments = GetAccumulatedSectorChecksum();
    }
    else
    {

        memset( m_plgfilehdr, 0, sizeof( LGFILEHDR ) );
    }

    m_pLogWriteBuffer->AdvanceLgposToWriteToNewGen( lgenToClose + 1 );

    Assert( 0 == CmpLgpos( lgposMax, m_lgposCreateAsynchTrigger ) );


    InitLgfilehdrT();

    if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
    {
        m_plgfilehdrT->lgfilehdr.le_ulMajor         = max( m_plgfilehdrT->lgfilehdr.le_ulMajor, m_plgfilehdr->lgfilehdr.le_ulMajor );
        m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor   = max( m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor, m_plgfilehdr->lgfilehdr.le_ulUpdateMajor );
        m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor   = max( m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor, m_plgfilehdr->lgfilehdr.le_ulUpdateMinor );
    }

#if DEBUG

    if ( !m_fResettingLogStream )
    {
        Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration <= lgenToClose + 1 );
    }
#endif

    m_plgfilehdrT->lgfilehdr.le_lGeneration = lgenToClose + 1;
    LGIGetDateTime( &m_plgfilehdrT->lgfilehdr.tmCreate );
    m_pinst->SaveDBMSParams( &m_plgfilehdrT->lgfilehdr.dbms_param );


    m_pLogWriteBuffer->InitLogBuffer( fLGFlags );

    m_pLogWriteBuffer->UnlockBuffer();

    LGPOS lgposFileT;
    lgposFileT.lGeneration = m_plgfilehdrT->lgfilehdr.le_lGeneration;
    lgposFileT.isec = 0;
    lgposFileT.ib = 0;
    m_pLog->LGLoadAttachmentsFromFMP( lgposFileT, m_plgfilehdrT->rgbAttach );

    SetSignatureInLgfilehdrT();
    LGSetFlagsInLgfilehdrT( fResUsed );
    m_plgfilehdrT->lgfilehdr.le_filetype = JET_filetypeLog;


    Call( ErrLGIWriteFileHdr( *ppfapiTmpLog, m_plgfilehdrT ) );


    m_tickNewLogFile = TickOSTimeCurrent();
    Assert( err == JET_errSuccess );
    return err;

HandleError:
    Assert( err < JET_errSuccess );
    if ( *ppfapiTmpLog )
    {
        (*ppfapiTmpLog)->SetNoFlushNeeded();
    }
    delete *ppfapiTmpLog;
    *ppfapiTmpLog = NULL;
    return err;
}


ERR LOG_STREAM::ErrLGIFinishNewLogFile( IFileAPI * const pfapiTmpLog )
{
    ERR             err         = JET_errSuccess;
    WCHAR           wszPathJetLog[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszPathJetTmpLog[ IFileSystemAPI::cchPathMax ];

    Assert( m_critLGWrite.FOwner() );

    LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eCurrentLog );
    LGMakeLogName( wszPathJetTmpLog, sizeof(wszPathJetTmpLog), eCurrentTmpLog );


    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 );
    Assert( pfapiTmpLog->DwEngineFileType() == iofileLog );
    pfapiTmpLog->UpdateIFilePerfAPIEngineFileTypeId( iofileLog, QwInstFileID( qwLogFileID, m_pinst->m_iInstance, m_plgfilehdrT->lgfilehdr.le_lGeneration ) );
    Call( pfapiTmpLog->ErrRename( wszPathJetLog ) );

    LONG lElasticWaypointDepth = m_pLog->LLGElasticWaypointLatency();
    LGPOS lgposFlushTip;
    m_pLog->LGFlushTip( &lgposFlushTip );
    if ( m_plgfilehdrT->lgfilehdr.le_lGeneration > lgposFlushTip.lGeneration + lElasticWaypointDepth )
    {
        BOOL fFlushed = fFalse;
        Call( ErrLGIFlushLogFileBuffers( pfapiTmpLog, iofrLogMaxRequired, &fFlushed ) );

        if ( fFlushed )
        {
            if ( lElasticWaypointDepth == 0 )
            {
                lgposFlushTip.lGeneration = m_plgfilehdrT->lgfilehdr.le_lGeneration;
                lgposFlushTip.isec        = (WORD)m_csecHeader;
                lgposFlushTip.ib          = 0;
            }
            else
            {
                lgposFlushTip.lGeneration = m_plgfilehdrT->lgfilehdr.le_lGeneration - 1;
                lgposFlushTip.isec        = lgposMax.isec;
                lgposFlushTip.ib          = lgposMax.ib;
            }
            m_pLog->LGSetFlushTip( lgposFlushTip );
        }
    }


    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 );
    Call( ErrStartAsyncLogFileCreation( wszPathJetTmpLog, m_plgfilehdrT->lgfilehdr.le_lGeneration ) );


    Call( m_pLog->ErrLGLockCheckpointAndUpdateGenRequired(
        m_plgfilehdrT->lgfilehdr.le_lGeneration,
        m_plgfilehdrT->lgfilehdr.tmCreate ) );

HandleError:
    return err;
}



ERR LOG_STREAM::ErrLGINewLogFile(
    const LONG              lgenToClose,
    BOOL                    fLGFlags,
    IFileAPI ** const       ppfapiNewLog
    )
{
    ERR                     err;
    IFileAPI*               pfapiTmpLog = NULL;
    WCHAR                   wszArchivePath[ IFileSystemAPI::cchPathMax ];

    Assert( m_critLGWrite.FOwner() );

    if ( !( fLGLogRenameOnly & fLGFlags ) )
    {
        if ( 0 != lgenToClose )
        {
            ErrEmitCompleteLog( lgenToClose );
        }

        const LONG lgenPrev = m_plgfilehdr->lgfilehdr.le_lGeneration;
        if ( fLGFlags == fLGOldLogExists )
        {
            LGMakeLogName( wszArchivePath, sizeof( wszArchivePath ), eArchiveLog, m_plgfilehdr->lgfilehdr.le_lGeneration );
        }

        Call( ErrLGIStartNewLogFile( lgenToClose, fLGFlags, &pfapiTmpLog ) );


        if ( fLGFlags == fLGOldLogExists )
        {

            Assert( m_pfapiLog->DwEngineFileType() == iofileLog );
            m_pfapiLog->UpdateIFilePerfAPIEngineFileTypeId( iofileLog, QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lgenPrev ) );
            Call( m_pfapiLog->ErrRename( wszArchivePath ) );
        }

        if ( m_pLogWriteBuffer->FStopOnNewLogGeneration() )
        {
            m_pLogWriteBuffer->LockBuffer();
            m_pLogWriteBuffer->ResetStopOnNewLogGeneration();
            m_pLogWriteBuffer->LGSetLogPaused( fTrue );
            m_pLogWriteBuffer->UnlockBuffer();

            m_pLogWriteBuffer->LGSignalLogPaused( fTrue );
            delete pfapiTmpLog;
            return ErrERRCheck( errOSSnapshotNewLogStopped );
        }
    }
    else
    {

        Assert( !(fLGFlags & ~fLGLogRenameOnly) );

        WCHAR           wszPathJetTmpLog[ IFileSystemAPI::cchPathMax ];

        LGMakeLogName( wszPathJetTmpLog, sizeof( wszPathJetTmpLog ), eCurrentTmpLog );
        Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 );
        Call( CIOFilePerf::ErrFileOpen(
                                m_pinst->m_pfsapi,
                                m_pinst,
                                wszPathJetTmpLog,
                                FmfLGStreamDefault(),
                                iofileLog,
                                QwInstFileID( qwLogFileID, m_pinst->m_iInstance, m_plgfilehdrT->lgfilehdr.le_lGeneration ),
                                &pfapiTmpLog ) );
    }

    Call( ErrLGIFinishNewLogFile( pfapiTmpLog ) );

    PERFOpt( cLGLogFileGenerated.Inc( m_pinst ) );
    PERFOpt( cLGLogFileCurrentGeneration.Set( m_pinst, lgenToClose + 1 ) );

    *ppfapiNewLog = pfapiTmpLog;
    Assert( err == JET_errSuccess );
    return err;

HandleError:
    delete pfapiTmpLog;

    if ( JET_errFileAlreadyExists == err )
    {
        err = ErrERRCheck( JET_errFileAccessDenied );
    }

    if ( JET_errFileNotFound == err )
    {
        err = ErrERRCheck( JET_errMissingLogFile );
    }

    if ( JET_errDiskFull == err )
    {
        err = ErrERRCheck( JET_errLogDiskFull );
    }

    if ( err < 0 )
    {
        UtilReportEventOfError( LOGGING_RECOVERY_CATEGORY, NEW_LOG_ERROR_ID, err, m_pinst );
        m_pLog->SetNoMoreLogWrite( err );
    }
    else
    {
        m_pLog->ResetNoMoreLogWrite();
    }

    return err;
}

ERR LOG_STREAM::ErrLGIUseNewLogFile( _Inout_ IFileAPI ** ppfapiNewLog )
{
    Assert( m_critLGWrite.FOwner() );
    Assert( ppfapiNewLog != NULL );
    Assert( *ppfapiNewLog != NULL );

    if ( m_pfapiLog )
    {
        m_pfapiLog->SetNoFlushNeeded();
    }

    delete m_pfapiLog;
    m_pfapiLog = *ppfapiNewLog;
    *ppfapiNewLog = NULL;

    m_pLogWriteBuffer->LockBuffer();

    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );
    UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
    m_pLogWriteBuffer->ResetWritePosition();

    m_pLogWriteBuffer->UnlockBuffer();

    ResetAccumulatedSectorChecksum();

    return JET_errSuccess;
}


ERR LOG_STREAM::ErrLGNewLogFile( LONG lgen, BOOL fLGFlags )
{
    ERR err;
    IFileAPI* pfapiNewLog = NULL;
    Assert( m_critLGWrite.FOwner() );

    err = ErrLGINewLogFile( lgen, fLGFlags, &pfapiNewLog );

    if ( err >= JET_errSuccess )
    {
        err = ErrLGIUseNewLogFile( &pfapiNewLog );
        Assert( err < JET_errSuccess || pfapiNewLog == NULL );
        if ( err < JET_errSuccess && pfapiNewLog )
        {
            (void)ErrLGIFlushLogFileBuffers( pfapiNewLog, iofrDefensiveErrorPath );
            delete pfapiNewLog;
            pfapiNewLog = NULL;
        }
    }

    if ( err < JET_errSuccess )
    {
        Assert( pfapiNewLog == NULL );
        if ( err == errOSSnapshotNewLogStopped )
        {
            (void)ErrLGIFlushLogFileBuffers( m_pfapiLog, iofrPersistClosingLogFile );
            delete m_pfapiLog;
            m_pfapiLog = NULL;
        }
        else if ( !m_pLog->FNoMoreLogWrite() )
        {
            m_pLog->LGReportError( LOG_FLUSH_OPEN_NEW_FILE_ERROR_ID, err );
            m_pLog->SetNoMoreLogWrite( err );
        }
    }

    (void)ErrIOUpdateCheckpoints( m_pinst );

    return err;
}




ERR LOG_STREAM::ErrLGSwitchToNewLogFile( LONG lgenToSwitchTo, BOOL fLGFlags )
{
    Assert( pNil != m_plgfilehdr );

    if ( m_plgfilehdr->lgfilehdr.le_lGeneration != lgenToSwitchTo )
    {
        Assert( 0 == ( fLGAssertShouldntNeedRoll & fLGFlags ) );

        Assert( lgenToSwitchTo == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );

        Assert( m_pfapiLog );

        ERR         err         = JET_errSuccess;
        WCHAR       wszPathJetLog[IFileSystemAPI::cchPathMax];

        LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eCurrentLog );

        m_critLGWrite.Enter();

        err = ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetLog );
        if ( err < 0 )
        {
            if ( JET_errFileNotFound == err )
            {
                fLGFlags |= fLGOldLogInBackup;
                delete m_pfapiLog;
                m_pfapiLog = NULL;
            }
            else
            {
                Call( err );
            }
        }
        else
        {
            fLGFlags |= fLGOldLogExists;
        }

        Assert( lgenToSwitchTo == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );

        Call( ErrLGNewLogFile( m_plgfilehdr->lgfilehdr.le_lGeneration, fLGFlags ) );

HandleError:
        m_critLGWrite.Leave();

        return err;
    }
    return JET_errSuccess;
}



ERR LOG_STREAM::ErrLGIWriteSectorData(
    IFileAPI * const                                pfapi,
    __in_ecount( cLogData ) const IOREASON          rgIor[],
    const LONG                                      lgenData,
    const DWORD                                     ibLogData,
    __in_ecount( cLogData ) const ULONG             rgcbLogData[],
    __in_ecount( cLogData ) const BYTE * const      rgpbLogData[],
    const size_t                                    cLogData,
    const DWORD                                     msgidErr,
    BOOL * const                                    pfFlushed,
    const BOOL                                      fGeneralizeError,
    const LGPOS&                                    lgposWriteEnd
    )
{
    ERR err = JET_errSuccess;
    ERR errNoMoreWrite = JET_errSuccess;

    DWORD ibEnd = ibLogData;
    for ( size_t i = 0; i < cLogData; ++i )
    {
        ibEnd += rgcbLogData[i];
    }
    Enforce( ( ( ibEnd + CbSec() - 1 ) >> Log2CbSec() ) <= m_csecLGFile );

    if ( m_fLogEndEmitted )
    {
        AssertTrack( ibLogData == 0 && ibEnd == sizeof(LGFILEHDR), "LogWriteAfterLogEndEmitted" );
        return ErrERRCheck( JET_errLogWriteFail );
    }

    if ( pfFlushed )
    {
        *pfFlushed = fFalse;
    }

    DWORD cbLogDataTrace = 0;
    for ( size_t i = 0; i < cLogData; ++i )
    {
        cbLogDataTrace += rgcbLogData[i];
    }
    ETLogWrite( lgenData, ibLogData, cbLogDataTrace );

    DWORD ibOffset = ibLogData;
    for ( size_t i = 0; i < cLogData; ++i )
    {
        (void)ErrEmitLogData( pfapi, rgIor[i], lgenData, ibOffset, rgcbLogData[i], rgpbLogData[i] );
        ibOffset += rgcbLogData[i];
    }
    Assert( ibLogData + cbLogDataTrace == ibOffset );

    if ( m_pLog->FNoMoreLogWrite( &errNoMoreWrite ) && errNoMoreWrite == errLogServiceStopped )
    {
        m_pLogWriteBuffer->LockBuffer();

        LGPOS lgposEndOfData;
        m_pLogWriteBuffer->GetLgposOfPbEntry ( &lgposEndOfData );
        if ( CmpLgpos( &lgposEndOfData, &lgposWriteEnd ) <= 0 )
        {
            (VOID)ErrEmitSignalLogEnd();
        }

        m_pLogWriteBuffer->UnlockBuffer();
    }

    if ( cLogData >  1 )
    {
        TraceContextScope tcScope;
        TraceContext* rgtc = (TraceContext*) _alloca( sizeof( TraceContext ) * cLogData );
        for ( size_t i = 0; i < cLogData; i++ )
        {
            TraceContext* ptc = new( &rgtc[i] ) TraceContext( *tcScope );
            ptc->iorReason = rgIor[i];
        }
        
        err = ErrIOWriteContiguous( pfapi,
                                    rgtc,
                                    ibLogData,
                                    rgcbLogData,
                                    rgpbLogData,
                                    cLogData,
                                    QosSyncDefault( m_pinst ) | qosIOOptimizeCombinable | qosIOOptimizeOverrideMaxIOLimits );

        for ( size_t i = 0; i < cLogData; i++ )
        {
            rgtc[i].~TraceContext();
        }
    }
    else
    {
        Assert( cLogData == 1 );
        TraceContextScope tcScope;
        tcScope->iorReason = rgIor[ 0 ];
        err = pfapi->ErrIOWrite( *tcScope, ibLogData, rgcbLogData[0], rgpbLogData[0], QosSyncDefault( m_pinst ) );
    }

    if ( err >= JET_errSuccess )
    {
        
        PERFOpt( cLGWrite.Inc( m_pinst ) );
        PERFOpt( cbLGWritten.Add( m_pinst, cbLogDataTrace ) );

        IOFLUSHREASON iofr = iofrInvalid;
        switch( rgIor[0].Iorp() )
        {
            case iorpLog:               iofr = iofrInvalid;
                Assert( rgIor[0].Iors() == iorsHeader );
                Assert( pfapi != m_pfapiLog );
                Assert( ibLogData == 0 );
                break;
            case iorpLGWriteCommit:     iofr = iofrLogCommitFlush;      break;
            case iorpLGWriteCapacity:   iofr = iofrLogCapacityFlush;    break;
            case iorpLGWriteSignal:     iofr = iofrLogSignalFlush;      break;
            case iorpLGFlushAll:        iofr = iofrLogFlushAll;         break;
            case iorpLGWriteNewGen:     iofr = iofrUtility;             break;
            case iorpPatchFix:          iofr = iofrLogPatch;            break;
            default:
                AssertSz( fFalse, "Saw another kind of flush: %d", rgIor[0].Iorp() );
        }

        err = ErrLGIFlushLogFileBuffers( pfapi, iofr, pfFlushed );
    }

    if ( err < JET_errSuccess )
    {
        for ( size_t i = 0; i < cLogData; ++i )
        {
            if ( !( ( rgIor[i].Iorp() == iorpPatchFix ) && !( rgIor[i].Iorf() & iorfFill ) ) )
            {
                m_pLog->SetNoMoreLogWrite( err );
                break;
            }
        }

        if ( fGeneralizeError )
        {
            m_pLog->LGReportError( LOG_FILE_SYS_ERROR_ID, err, pfapi );
    
            err = ErrERRCheck( JET_errLogWriteFail );
        }
    
        m_pLog->LGReportError( msgidErr, err, pfapi );
    }

    return err;
}


INLINE IOREASONPRIMARY IorpLogRead( LOG * plog )
{
    if ( !plog->FRecovering() )
    {
        AssertSz( fFalse, "Could not prove this can happen." );
        return iorpLog;
    }

    switch( plog->FRecoveringMode() ) {

        case fRecoveringRedo:
        case fRecoveringUndo:
        case fRestoreRedo:
            return iorpLogRecRedo;

        case fRecoveringNone:
            return iorpDirectAccessUtil;
            break;
            
        case fRestorePatch:
        default:
            AssertSz( fFalse, "Shouldn't happen." );
            break;
    }
    return iorpLog;
}

ERR LOG_STREAM::ErrLGReadSectorData(
    const QWORD             ibLogData,
    const ULONG             cbLogData,
    __out_bcount( cbLogData ) BYTE *    pbLogData )
{
    TraceContextScope tcScope( IorpLogRead( m_pLog ) );
    return m_pfapiLog->ErrIORead( *tcScope,
                                  ibLogData,
                                  cbLogData,
                                  pbLogData,
                                  QosSyncDefault( m_pinst ) );
}

VOID LOG_STREAM::LGSzFromLogId( __out_bcount( cbFName ) PWSTR wszLogFileName, size_t cbFName, LONG lGeneration )
{
    Assert( wszLogFileName );
    Assert( cbFName >= 12 * sizeof( WCHAR ) );
    OSStrCbCopyW( wszLogFileName, cbFName, SzParam( m_pinst, JET_paramBaseName ) );
    LGFileHelper::LGSzLogIdAppend( wszLogFileName, cbFName, lGeneration, ( UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8  );
    return;
}

VOID LGMakeName( IFileSystemAPI *const pfsapi, __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR wszName, __in PCWSTR wszPath, __in PCWSTR wszFName, __in PCWSTR wszExt )
{
    WCHAR   wszDirT[IFileSystemAPI::cchPathMax];
    WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];
    WCHAR   wszExtT[IFileSystemAPI::cchPathMax];

    CallS( pfsapi->ErrPathParse( wszPath, wszDirT, wszFNameT, wszExtT ) );
    CallS( pfsapi->ErrPathBuild( wszDirT, wszFName, wszExt, wszName ) );
}

VOID LOG_STREAM::LGFullLogNameFromLogId( __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName, LONG lGeneration, __in PCWSTR wszDirectory )
{
    WCHAR   wszFullPathT[IFileSystemAPI::cchPathMax];

    CallS( m_pinst->m_pfsapi->ErrPathComplete( wszDirectory, wszFullPathT ) );
    CallS( ErrLGMakeLogNameBaseless( wszFullLogFileName, OSFSAPI_MAX_PATH * sizeof( WCHAR ), wszFullPathT, eArchiveLog, lGeneration ) );

    return;
}

ERR LOG_STREAM::ErrLGRSTOpenLogFile(
    __in PCWSTR         wszLogFolder,
    INT                 gen,
    IFileAPI **const    ppfapi,
    __in_opt PCWSTR     wszLogExt
    )
{
    ERR     err = JET_errSuccess;
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];

    Assert( gen > 0 || gen == lGenSignalCurrentID );
    if ( gen == lGenSignalCurrentID )
    {
        CallR( ErrLGMakeLogNameBaseless( wszPath, sizeof(wszPath), wszLogFolder, eCurrentLog, 0, wszLogExt ) );
    }
    else
    {
        CallR( ErrLGMakeLogNameBaseless( wszPath, sizeof(wszPath), wszLogFolder, eArchiveLog, gen, wszLogExt ) );
    }

    return CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi,
                m_pinst,
                wszPath,
                FmfLGStreamDefault(),
                iofileLog,
                QwInstFileID( qwLogFileID, m_pinst->m_iInstance, gen ),
                ppfapi );
}


VOID LOG_STREAM::LGRSTDeleteLogs( __in PCWSTR wszLogFolder, INT genLow, INT genHigh, BOOL fIncludeJetLog )
{
    INT     gen;
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];

    for ( gen = genLow; gen <= genHigh; gen++ )
    {
        CallS( ErrLGMakeLogNameBaseless( wszPath, sizeof(wszPath), wszLogFolder, eArchiveLog, gen ) );
        CallSx( m_pinst->m_pfsapi->ErrFileDelete( wszPath ), JET_errFileNotFound );
    }

    if ( fIncludeJetLog )
    {
        CallS( ErrLGMakeLogNameBaseless( wszPath, sizeof(wszPath), wszLogFolder, eCurrentLog, 0 ) );
        CallSx( m_pinst->m_pfsapi->ErrFileDelete( wszPath ), JET_errFileNotFound );
    }
}


ERR LOG_STREAM::ErrLGGetGenerationRange( __in PCWSTR wszFindPath, LONG* plgenLow, LONG* plgenHigh, __in BOOL fLooseExt, __out_opt BOOL * pfDefaultExt )
{
    ERR         err         = JET_errSuccess;
    BOOL        fDefaultExt     = fTrue;

    Assert( fLooseExt || m_wszLogExt );
    Assert( !fLooseExt || m_wszLogExt == NULL );

    err = ErrLGGetGenerationRangeExt( wszFindPath, plgenLow, plgenHigh, fLooseExt ? m_pLog->WszLGGetDefaultExt( fFalse ) : m_wszLogExt );
    if ( fLooseExt &&  (0 == (plgenLow ? *plgenLow : 1)  || 0 == (plgenHigh ? *plgenHigh : 1 ) ) )
    {
        fDefaultExt = fFalse;
        (void)ErrLGGetGenerationRangeExt( wszFindPath, plgenLow, plgenHigh, m_pLog->WszLGGetOtherExt( fFalse ) );
    }

    if ( pfDefaultExt )
    {
        *pfDefaultExt = fDefaultExt;
    }

    return err;
}

ERR LOG_STREAM::ErrLGGetGenerationRangeExt( __in PCWSTR wszFindPath, LONG* plgenLow, LONG* plgenHigh, __in PCWSTR wszLogExt  )
{
    ERR             err         = JET_errSuccess;
    WCHAR           wszFind[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszFileName[ IFileSystemAPI::cchPathMax ];
    IFileFindAPI*   pffapi      = NULL;

    LONG            lGenMax     = 0;
    LONG            lGenMin     = lGenerationMaxDuringRecovery + 1;

    ULONG           cLogDigits = 0;

    Assert ( wszLogExt );
    Assert ( wszFindPath );
    Assert ( m_pinst->m_pfsapi );

    Call( ErrLGMakeLogNameBaseless( wszFind, sizeof(wszFind), wszFindPath, eArchiveLog, 0, wszLogExt ) );

    Call( m_pinst->m_pfsapi->ErrFileFind( wszFind, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        LONG        lGen = 0;
        Call( pffapi->ErrPath( wszFileName ) );
        err = LGFileHelper::ErrLGGetGeneration( m_pinst->m_pfsapi, wszFileName, SzParam( m_pinst, JET_paramBaseName ), &lGen, wszLogExt, &cLogDigits );
        if ( err != JET_errInvalidParameter && err != JET_errSuccess )
        {
            Call( ErrERRCheck( err ) );
        }
        if ( err == JET_errSuccess )
        {
            if ( !(UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) && 5 == cLogDigits )
            {
                Call( ErrERRCheck( JET_errLogGenerationMismatch ) );
            }

            Assert(lGen != 0);
            lGenMax = max( lGenMax, lGen );
            lGenMin = min( lGenMin, lGen );
        }
        err = JET_errSuccess;
    }
    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

HandleError:

    delete pffapi;


    if ( err < JET_errSuccess )
    {
        Assert ( JET_errFileNotFound != err );
        lGenMin = 0;
        lGenMax = 0;
    }

    if ( lGenerationMaxDuringRecovery + 1 == lGenMin )
    {
        CallS( err );
        Assert( 0 == lGenMax );
        lGenMin = 0;
    }


    Assert( 0 <= lGenMin );
    Assert( 0 <= lGenMax );

    Assert( lGenMin <= lGenerationMaxDuringRecovery );
    Assert( lGenMax <= lGenerationMaxDuringRecovery );
    Assert( lGenMin <= lGenMax );

    if ( plgenLow )
    {
        *plgenLow = lGenMin;
    }

    if ( plgenHigh )
    {
        *plgenHigh = lGenMax;
    }

    Assert( err != JET_errFileNotFound );

    return err;
}

ERR LOG_STREAM::ErrLGRemoveCommittedLogs( __in LONG lGenToDeleteFrom )
{
    ERR     err = JET_errSuccess;
    ERR     errT = JET_errSuccess;
    LONG    lGenFirst = 0;
    LONG    lGenLast = 0;
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];

    CallR( ErrLGGetGenerationRange( m_pLog->SzLGCurrentFolder(), &lGenFirst, &lGenLast ) );

    if ( lGenFirst == 0 || lGenLast == 0 )
    {
        Assert( lGenFirst == 0 && lGenLast == 0 );
        return JET_errSuccess;
    }



    for ( LONG lGen = lGenToDeleteFrom; lGen > 0 && lGen <= lGenLast; lGen++ )
    {
        LGMakeLogName( wszPath, sizeof(wszPath), eArchiveLog, lGen );
        errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
        if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
        {
            err = errT;
        }
        if ( errT >= JET_errSuccess )
        {
            m_cRemovedLogs++;
        }
    }

    if ( m_cRemovedLogs >= 2 )
    {
        WCHAR szT1[16];
        WCHAR szT2[16];
        WCHAR szT3[16];
        WCHAR szT4[16];
        const WCHAR * rgszT[5];
        OSStrCbFormatW( szT1, sizeof( szT1 ), L"%d", m_cRemovedLogs );
        rgszT[0] = szT1;
        OSStrCbFormatW( szT2, sizeof( szT2 ), L"0x%x", lGenToDeleteFrom );
        rgszT[1] = szT2;
        OSStrCbFormatW( szT3, sizeof( szT3 ), L"0x%x", lGenLast );
        rgszT[2] = szT3;
        OSStrCbFormatW( szT4, sizeof( szT4 ), L"0x%x", lGenToDeleteFrom - 1 );
        rgszT[3] = szT4;
        rgszT[4] = m_pLog->SzLGCurrentFolder();
        UtilReportEvent(    eventWarning,
                            LOGGING_RECOVERY_CATEGORY,
                            REDO_MORE_COMMITTED_LOGS_REMOVED_ID,
                            _countof( rgszT ),
                            rgszT,
                            0,
                            NULL,
                            m_pinst );
    }

    return err;
}


ERR LOG_STREAM::ErrLGRStartNewLog(
    __in const LONG lgenToCreate
    )
{
    ERR err = JET_errSuccess;
    IFileAPI* pfapiNewLog = NULL;

    
    m_critLGWrite.Enter();
    Call( ErrLGINewLogFile( lgenToCreate - 1, fLGOldLogInBackup, &pfapiNewLog ) );

    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration == lgenToCreate );
    Call( ErrLGIUseNewLogFile( &pfapiNewLog ) );

    LGMakeLogName( eCurrentLog );

    m_fCreatedNewLogFileDuringRedo = fTrue;

HandleError:
    Assert( err < JET_errSuccess || pfapiNewLog == NULL );
    if ( err < JET_errSuccess && pfapiNewLog )
    {
        (void)ErrLGIFlushLogFileBuffers( pfapiNewLog, iofrDefensiveErrorPath );
        delete pfapiNewLog;
        pfapiNewLog = NULL;
    }
    m_critLGWrite.Leave();
    return err;
}

ERR LOG_STREAM::ErrLGRRestartLOGAtLogGen(
    __in const LONG lgenToStartAt,
    __in const BOOL fLogEvent
    )
{
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];
    ERR err = JET_errSuccess;

    delete m_pfapiLog;

    m_pfapiLog = NULL;

    LGMakeLogName( wszPath, sizeof(wszPath), eCurrentLog );

    if ( fLogEvent )
    {
        WCHAR szGenLast[32];
        WCHAR szGenDelete[32];
        const WCHAR *rgszT[] = { szGenLast, wszPath, szGenDelete };

        OSStrCbFormatW( szGenLast, sizeof( szGenLast ), L"%d", lgenToStartAt - 1);
        OSStrCbFormatW( szGenDelete, sizeof( szGenDelete ), L"%d", LONG( m_plgfilehdr->lgfilehdr.le_lGeneration ) );

        UtilReportEvent(
                eventWarning,
                LOGGING_RECOVERY_CATEGORY,
                DELETE_LAST_LOG_FILE_TOO_OLD_ID,
                sizeof(rgszT) / sizeof(rgszT[0]),
                rgszT,
                0,
                NULL,
                m_pinst );
    }

    CallR ( m_pinst->m_pfsapi->ErrFileDelete( wszPath ) );
    m_cRemovedLogs++;

    m_pLogWriteBuffer->LockBuffer();
    UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof(LGFILEHDR) );
    m_pLogWriteBuffer->UnlockBuffer();

    return ErrLGRStartNewLog( lgenToStartAt );
}

BOOL LOG_STREAM::FLGRedoOnCurrentLog() const
{
    WCHAR    wszT[IFileSystemAPI::cchPathMax];
    WCHAR    wszFNameT[IFileSystemAPI::cchPathMax];

    
    CallS( m_pinst->m_pfsapi->ErrPathParse( m_wszLogName, wszT, wszFNameT, wszT ) );

    return UtilCmpFileName( wszFNameT, SzParam( m_pinst, JET_paramBaseName ) ) == 0;
}

BOOL LOG_STREAM::FTempLogExists() const
{
    WCHAR   wszPathJetTmpLog[IFileSystemAPI::cchPathMax];

    LGMakeLogName( wszPathJetTmpLog, sizeof(wszPathJetTmpLog), eCurrentTmpLog );

    return ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetTmpLog ) >= JET_errSuccess;
}

BOOL LOG_STREAM::FCurrentLogExists() const
{
    WCHAR   wszPathJetLog[IFileSystemAPI::cchPathMax];

    LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eCurrentLog );

    return ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetLog ) >= JET_errSuccess;
}



ERR LOG_STREAM::ErrLGRICleanupMismatchedLogFiles( BOOL fExtCleanup )
{
    ERR     err = JET_errSuccess;
    ERR     errT;
    LONG    lGen;
    LONG    lGenLast;
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];


    Assert( BoolParam( m_pinst, JET_paramCircularLog ) );


    m_critLGWrite.Enter();
    delete m_pfapiLog;
    m_pfapiLog = NULL;

    LGCreateAsynchCancel( fFalse );

    m_critLGWrite.Leave();


    CallR( ErrLGGetGenerationRange( m_pLog->SzLGCurrentFolder(), &lGen, &lGenLast ) );


    for ( ; lGen > 0 && lGen <= lGenLast; lGen++ )
    {
        LGMakeLogName( wszPath, sizeof(wszPath), eArchiveLog, lGen );
        errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
        if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
        {
            err = errT;
        }
    }


    LGMakeLogName( wszPath, sizeof(wszPath), eCurrentLog );
    errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
    if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
    {
        err = errT;
    }

    LGMakeLogName( wszPath, sizeof(wszPath), eCurrentTmpLog );
    errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
    if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
    {
        err = errT;
    }


    if( fExtCleanup )
    {
        m_pLog->LGSetChkExts( m_pLog->FLGIsLegacyExt( fTrue, m_pLog->WszLGGetOtherExt( fTrue ) ), fTrue );
        m_pLog->LGFullNameCheckpoint( wszPath );
        errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
        if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
        {
            err = errT;
        }
        m_pLog->LGSetChkExts( m_pLog->FLGIsLegacyExt( fTrue, m_pLog->WszLGGetDefaultExt( fTrue ) ), fTrue );
    }
    
    m_pLog->LGFullNameCheckpoint( wszPath );
    errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
    if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
    {
        err = errT;
    }


    CallR( err );


    m_csecLGFile = CsecLGFromSize( (LONG)UlParam( m_pinst, JET_paramLogFileSize ) );


    m_critLGWrite.Enter();

    if ( fExtCleanup )
    {
        m_wszLogExt = m_pLog->WszLGGetDefaultExt( fFalse );
        m_pLog->LGSetChkExts( m_pLog->FLGIsLegacyExt( fTrue, m_pLog->WszLGGetDefaultExt( fTrue ) ), fTrue );
        m_pLog->SetLgenInitial( lGenLast+1 );
    }

    m_pLog->ResetCheckpoint();
    m_pLog->ResetLgenLogtimeMapping();

#if DEBUG
    m_fResettingLogStream = fTrue;
#endif

    m_pLog->ResetRecoveryState();

    IFileAPI* pfapiNewLog = NULL;
    err = ErrLGINewLogFile( (m_pLog->LgenInitial()-1), fLGOldLogNotExists, &pfapiNewLog );
    delete pfapiNewLog;
    pfapiNewLog = NULL;

#if DEBUG
    m_fResettingLogStream = fFalse;
#endif

    if ( err >= JET_errSuccess )
    {



        m_pLog->ErrLGSetSignLog( NULL, fFalse );

        m_pLogWriteBuffer->LockBuffer();
        memcpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
        m_pLogWriteBuffer->ResetWritePosition();
        m_pLog->LGResetFlushTipWithLock( lgposMin );
        m_pLogWriteBuffer->UnlockBuffer();

        Assert( m_pLog->LgenInitial() == m_plgfilehdr->lgfilehdr.le_lGeneration );

        LGMakeLogName( eCurrentLog );
        err = CIOFilePerf::ErrFileOpen(
                                m_pinst->m_pfsapi,
                                m_pinst,
                                m_wszLogName,
                                FmfLGStreamDefault(),
                                iofileLog,
                                QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lGenSignalCurrentID  ),
                                &m_pfapiLog );
        if ( err >= JET_errSuccess )
        {


            err = ErrLGReadFileHdr( NULL, iorpLog, m_plgfilehdr, fFalse );
#ifdef DEBUG
            if ( err >= JET_errSuccess )
            {
                m_pLogWriteBuffer->ResetLgposLastLogRec();
            }
#endif
        }

    }
    else
    {



    }
    m_critLGWrite.Leave();


    return ( err );
}


ERR LOG_STREAM::ErrLGDeleteOutOfRangeLogFiles()
{
    ERR             err = JET_errSuccess;
    LGFILEHDR *     plgfilehdr = NULL;
    const LONG      lCurrentGeneration = m_plgfilehdr->lgfilehdr.le_lGeneration;

    Assert( lCurrentGeneration != 0 );

    LONG            lgenMin = 0;
    LONG            lgenMax = 0;
    LONG            lgen = 0;

    Assert ( m_wszLogExt != NULL );
    Assert ( BoolParam( m_pinst, JET_paramDeleteOutOfRangeLogs ) );

    CallR( ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenMin, &lgenMax ) );

    if ( lgenMin == 0 )
    {
        Assert ( 0 == lgenMax );
        return JET_errSuccess;
    }

    if ( lgenMax < lCurrentGeneration )
    {
        return JET_errSuccess;
    }

    lgenMin = max( lgenMin, lCurrentGeneration );
    lgenMax = max( lgenMax, lCurrentGeneration );

    AllocR( plgfilehdr = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL ) );

    WCHAR           wszFileFrom[16];
    WCHAR           wszFileTo[16];
    WCHAR           wszGeneration[32];
    const WCHAR *   rgszT[]     = { wszGeneration, wszFileFrom, wszFileTo };

    LGSzFromLogId( wszFileFrom, sizeof( wszFileFrom), lgenMin );
    LGSzFromLogId( wszFileTo, sizeof(wszFileTo),  lgenMax );
    OSStrCbFormatW( wszGeneration, sizeof( wszGeneration ), L"%d", lCurrentGeneration );

    UtilReportEvent(
            eventWarning,
            LOGGING_RECOVERY_CATEGORY,
            DELETE_LOG_FILE_TOO_NEW_ID,
            sizeof(rgszT) / sizeof(rgszT[0]),
            rgszT,
            0,
            NULL,
            m_pinst );

    for ( lgen = lgenMin; lgen <= lgenMax; lgen++ )
    {
        WCHAR       wszFullName[IFileSystemAPI::cchPathMax];
        IFileAPI *  pfapiT = NULL;

        Assert( lgen > 0 );
        err = ErrLGRSTOpenLogFile( SzParam( m_pinst, JET_paramLogFilePath ), lgen, &pfapiT );
        if ( JET_errFileNotFound == err )
        {
            continue;
        }
        Call ( err );

        err = ErrLGReadFileHdr( pfapiT, iorpLog, plgfilehdr, fCheckLogID );
        delete pfapiT;
        pfapiT = NULL;
        Call ( err );

        LGMakeLogName( wszFullName, sizeof(wszFullName), eArchiveLog, lgen );
        err = m_pinst->m_pfsapi->ErrFileDelete( wszFullName );
        if ( JET_errFileNotFound == err )
        {
            err = JET_errSuccess;
        }
        Call( err );
    }

    err = JET_errSuccess;

HandleError:
    OSMemoryPageFree( plgfilehdr );
    return err;
}

ERR LOG_STREAM::ErrLGCreateNewLogStream( BOOL * const pfJetLogGeneratedDuringSoftStart )
{
    ERR                 err = JET_errSuccess;

    
    LGResetSectorGeometry();

    
    m_critLGWrite.Enter();
    
    IFileAPI* pfapiNewLog = NULL;
    Call( ErrLGINewLogFile( m_pLog->LgenInitial()-1, fLGOldLogNotExists, &pfapiNewLog ) );

    Call( ErrLGIFlushLogFileBuffers( pfapiNewLog, iofrPersistClosingLogFile ) );

    delete pfapiNewLog;
    pfapiNewLog = NULL;

    m_pLogWriteBuffer->LockBuffer();
    UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
    m_pLogWriteBuffer->ResetWritePosition();
    m_pLogWriteBuffer->UnlockBuffer();
    
    *pfJetLogGeneratedDuringSoftStart = fTrue;
    
    Assert( m_plgfilehdr->lgfilehdr.le_lGeneration == m_pLog->LgenInitial() );
    

HandleError:

    if ( err < JET_errSuccess && pfapiNewLog )
    {
        (void)ErrLGIFlushLogFileBuffers( pfapiNewLog, iofrDefensiveErrorPath );
        delete pfapiNewLog;
        pfapiNewLog = NULL;
    }

    m_critLGWrite.Leave();

    return err;
}


ERR LOG_STREAM::ErrLGIDeleteOldLogStream()
{
    ERR err = JET_errSuccess;


    LONG lGenLow, lGenHigh, lGen;
    WCHAR wszPath[IFileSystemAPI::cchPathMax];
    
    Call( ErrLGGetGenerationRange( m_pLog->SzLGCurrentFolder(), &lGenLow, &lGenHigh ) );
    
    
    for ( lGen = lGenLow; lGen > 0 && lGen <= lGenHigh; lGen++ )
    {
        LGMakeLogName( wszPath, sizeof(wszPath), eArchiveLog, lGen );
        Call( m_pinst->m_pfsapi->ErrFileDelete( wszPath ) );
    }
    
    
    LGMakeLogName( wszPath, sizeof(wszPath), eCurrentLog );
    Call( m_pinst->m_pfsapi->ErrFileDelete( wszPath ) );
    
    LGMakeLogName( wszPath, sizeof(wszPath), eCurrentTmpLog );
    Call( m_pinst->m_pfsapi->ErrFileDelete( wszPath ) );

HandleError:

    return err;
}

VOID LOG_STREAM::LGIReportLogTruncation(
    const LONG          lgenMic,
    const LONG          lgenMac,
    const BOOL          fSnapshot )
{

    WCHAR           szFullLogNameDeleteMic[IFileSystemAPI::cchPathMax];
    WCHAR           szFullLogNameDeleteMac[IFileSystemAPI::cchPathMax];
    const WCHAR *   rgszT[] = { szFullLogNameDeleteMic, szFullLogNameDeleteMac };
    WCHAR           szFullPathName[IFileSystemAPI::cchPathMax];
    WCHAR   szDirT[IFileSystemAPI::cchPathMax];
    WCHAR   szFNameT2[IFileSystemAPI::cchPathMax];
    WCHAR   szExtT[IFileSystemAPI::cchPathMax];

    if ( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), szFullPathName ) < JET_errSuccess )
    {
        OSStrCbCopyW( szFullPathName, sizeof(szFullPathName), L"" );
    }
    CallS( m_pinst->m_pfsapi->ErrPathParse( szFullPathName, szDirT, szFNameT2, szExtT ) );

    Assert( lgenMic < lgenMac );
    CallS( ErrLGMakeLogNameBaseless( szFullLogNameDeleteMic, sizeof(szFullLogNameDeleteMic), szDirT, eArchiveLog, lgenMic ) );
    CallS( ErrLGMakeLogNameBaseless( szFullLogNameDeleteMac, sizeof(szFullLogNameDeleteMac), szDirT, eArchiveLog, lgenMac - 1 ) );

    UtilReportEvent(
            eventInformation,
            ( fSnapshot ? OS_SNAPSHOT_BACKUP : LOGGING_RECOVERY_CATEGORY ),
            BACKUP_TRUNCATE_LOG_FILES,
            2,
            rgszT,
            0,
            NULL,
            m_pinst );
}

VOID LOG_STREAM::LGTruncateLogsAfterRecovery()
{
    WCHAR       wszLogName[ IFileSystemAPI::cchPathMax ];
    LONG        lgenMic     = 0;
    const LONG  lgenMac     = m_plgfilehdr->lgfilehdr.le_lGeneration;

    (void)ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenMic, NULL );

    if ( lgenMic > 0 )
    {
        LGIReportLogTruncation( lgenMic, lgenMac, fFalse );

        for ( LONG lgen = lgenMic; lgen < lgenMac; lgen++ )
        {
            LGMakeLogName( wszLogName, sizeof(wszLogName), eArchiveLog, lgen );

            if ( JET_errSuccess != m_pinst->m_pfsapi->ErrFileDelete( wszLogName ) )
                break;
        }
    }
    else
    {
        UtilReportEvent(
                eventInformation,
                LOGGING_RECOVERY_CATEGORY,
                BACKUP_NO_TRUNCATE_LOG_FILES,
                0,
                NULL,
                0,
                NULL,
                m_pinst );
    }
}

VOID DBGBRTrace( __in PCSTR sz );

ERR LOG_STREAM::ErrLGTruncateLog(
    const LONG lgenMic,
    const LONG lgenMac,
    const BOOL fSnapshot,
    BOOL fFullBackup )
{
    ERR     err = JET_errSuccess;
    WCHAR   szDeleteFile[IFileSystemAPI::cchPathMax];
    LONG    cDeleted = 0;
    LONG    cFilesNotFound = 0;
    LGPOS   lgposT;

    ErrLGBackupTruncate( m_pLog, fSnapshot ? DBFILEHDR::backupOSSnapshot : DBFILEHDR::backupNormal,
                            !fFullBackup, lgenMic, lgenMac, &lgposT );
    
    if ( lgenMic >= lgenMac )
    {

        UtilReportEvent(
                eventInformation,
                fSnapshot?OS_SNAPSHOT_BACKUP:LOGGING_RECOVERY_CATEGORY,
                BACKUP_NO_TRUNCATE_LOG_FILES,
                0,
                NULL,
                0,
                NULL,
                m_pinst );
    }

    else
    {
        LGIReportLogTruncation( lgenMic, lgenMac, fSnapshot );

        

        for ( LONG lT = lgenMic; lT < lgenMac; lT++ )
        {
            CallS( ErrLGMakeLogNameBaseless(
                        szDeleteFile,
                        sizeof(szDeleteFile),
                        SzParam( m_pinst, JET_paramLogFilePath ),
                        eArchiveLog,
                        lT ) );
            err = m_pinst->m_pfsapi->ErrFileDelete( szDeleteFile );
            if ( err != JET_errSuccess )
            {
                if ( JET_errFileNotFound == err )
                {
                    cFilesNotFound++;
                    err = JET_errSuccess;
                    continue;
                }

                
                break;
            }
            cDeleted++;
        }
    }

    if ( cFilesNotFound )
    {
        WCHAR           szMissingCount[64];
        const WCHAR *   rgszT[] = { szMissingCount };

        OSStrCbFormatW( szMissingCount, sizeof(szMissingCount), L"%d", cFilesNotFound );

        UtilReportEvent(
                eventWarning,
                fSnapshot?OS_SNAPSHOT_BACKUP:LOGGING_RECOVERY_CATEGORY,
                BACKUP_TRUNCATE_FOUND_MISSING_LOG_FILES,
                _countof( rgszT ),
                rgszT,
                0,
                NULL,
                m_pinst );

    }

#ifdef DEBUG
    if ( m_fDBGTraceBR )
    {
        CHAR sz[256];

        OSStrCbFormatA( sz, sizeof(sz), "** TruncateLog (%d) %d - %d.\n", err, lgenMic, lgenMac );
        Assert( strlen( sz ) <= sizeof( sz ) - 1 );
        DBGBRTrace( sz );
    }
#endif

    return err;
}

ERR LOG_STREAM::ErrLGOpenFile( PCWSTR pszPath, BOOL fReadOnly )
{
    ERR err;

    pszPath = pszPath ? pszPath : m_wszLogName;
    
    m_critLGWrite.Enter();

    delete m_pfapiLog;
    m_pfapiLog = NULL;

    ULONG cLogDigits;
    LONG lgen;
    (void)LGFileHelper::ErrLGGetGeneration( m_pinst->m_pfsapi, pszPath ? pszPath : m_wszLogName, SzParam( m_pinst, JET_paramBaseName ), &lgen, m_wszLogExt, &cLogDigits );
    if ( lgen == 0 )
    {
        lgen = lGenSignalCurrentID;
    }

    err = CIOFilePerf::ErrFileOpen( m_pinst->m_pfsapi,
                                    m_pinst,
                                    pszPath ? pszPath : m_wszLogName,
                                    FmfLGStreamDefault( fReadOnly ? IFileAPI::fmfReadOnly : IFileAPI::fmfNone ),
                                    iofileLog,
                                    QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgen ),
                                    &m_pfapiLog );
    m_critLGWrite.Leave();
    return err;
}

VOID LOG_STREAM::LGCloseFile()
{
    m_critLGWrite.Enter();

    delete m_pfapiLog;
    m_pfapiLog = NULL;

    m_critLGWrite.Leave();
}

VOID
LOG_STREAM::LGVerifyFileHeaderOnInit()
{
    LGFILEHDR * plgfilehdrT = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR), NULL );
    if ( plgfilehdrT )
    {
        const ERR errT = ErrLGReadFileHdr(
                NULL,
                iorpHeader,
                plgfilehdrT,
                fFalse,
                fFalse );
        if ( errT >= JET_errSuccess )
        {
            C_ASSERT( sizeof(LGFILEHDR) == cbLogFileHeader );
            Enforce( 0 == memcmp( plgfilehdrT, m_plgfilehdr, sizeof(LGFILEHDR) ) );
        }

        OSMemoryPageFree( plgfilehdrT );
    }

    Enforce( m_plgfilehdr->rgbAttach[0] == 0 );
}

const IOFLUSHREASON g_iofrSuppressLLR = IOFLUSHREASON( iofrLogCommitFlush | iofrLogCapacityFlush | iofrLogSignalFlush );

ERR LOG_STREAM::ErrLGIFlushLogFileBuffers( IFileAPI * pfapiLog, const IOFLUSHREASON iofr, BOOL * const pfFlushed )
{
    ERR err = JET_errSuccess;

    if ( pfFlushed )
    {
        *pfFlushed = fFalse;
    }

    if ( pfapiLog == m_pfapiLog &&
         iofr != iofrLogPatch )
    {
        Assert( m_critLGWrite.FOwner() );
    }

    if ( pfapiLog )
    {
        if ( iofr == iofrInvalid ||
             ( m_pLog->FWaypointLatencyEnabled() &&
               ( g_iofrSuppressLLR | iofr ) == g_iofrSuppressLLR ) )
        {
            OSTrace( JET_tracetagIO, OSFormat( "Flush Operation Suppression: 0x%x\n", iofr ) );
            pfapiLog->SetNoFlushNeeded();
        }
        else
        {
            err = ErrUtilFlushFileBuffers( pfapiLog, iofr );

            if ( err >= JET_errSuccess )
            {
                if ( pfFlushed )
                {
                    *pfFlushed = fTrue;
                }
            }
            else
            {
                m_pLog->LGReportError( LOG_FILE_SYS_ERROR_ID, err, pfapiLog );
                m_pLog->SetNoMoreLogWrite( err );
            }
        }
    }
    else
    {
        AssertTrack( fFalse, "NullFileObjectOnFlushLgBuffers" );
        err = errCodeInconsistency;
    }

    return err;
}

ERR LOG_STREAM::ErrLGFlushLogFileBuffers( const IOFLUSHREASON iofr, BOOL * const pfFlushed )
{
    ERR err = JET_errSuccess;

    if ( pfFlushed )
    {
        *pfFlushed = fFalse;
    }

    LockWrite();

    Assert( m_pfapiLog || ( m_pLog->FRecovering() && m_pLog->FRecoveringMode() == fRecoveringRedo ) || m_pLog->FLGLogPaused() );

    if ( m_pfapiLog )
    {
        err = ErrLGIFlushLogFileBuffers( m_pfapiLog, iofr, pfFlushed );
    }
    else if ( m_pLog->FLogDisabledDueToRecoveryFailure() || m_pLog->FLGLogPaused() )
    {
        IFileAPI *pfapiLog = NULL;
        LGMakeLogName( eCurrentLog );
        err = CIOFilePerf::ErrFileOpen(
                                    m_pinst->m_pfsapi,
                                    m_pinst,
                                    m_wszLogName,
                                    FmfLGStreamDefault(),
                                    iofileLog,
                                    QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lGenSignalCurrentID  ),
                                    &pfapiLog );
        if ( err < JET_errSuccess )
        {
            LGPOS lgposWriteTip;
            m_pLog->LGWriteTip( &lgposWriteTip );

            LGMakeLogName( eArchiveLog, lgposWriteTip.lGeneration );
            err = CIOFilePerf::ErrFileOpen(
                                        m_pinst->m_pfsapi,
                                        m_pinst,
                                        m_wszLogName,
                                        FmfLGStreamDefault(),
                                        iofileLog,
                                        QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lgposWriteTip.lGeneration ),
                                        &pfapiLog );
        }
        if ( pfapiLog )
        {
            err = ErrLGIFlushLogFileBuffers( pfapiLog, iofr, pfFlushed );
        }
        delete pfapiLog;
    }

    UnlockWrite();

    return err;
}

void LOG_STREAM::LGAssertFullyFlushedBuffers()
{
#ifdef DEBUG
    LockWrite();

    Expected( m_pfapiLog );
    Assert( m_pfapiLog == NULL || m_pfapiLog->CioNonFlushed() == 0 );

    UnlockWrite();
#endif
}

