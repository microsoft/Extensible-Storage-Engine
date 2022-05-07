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

#endif // PERFMON_SUPPORT

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
      //    Asynchronous log file creation
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

    // A clean lifecycle of this object should not allow it to get here with these allocated.
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
    //  The m_plgfilehdr seems to lag the m_plgfilehdrT, so checking the latent m_plgfilehdr would be the safest
    //  hdr to check, as it in worse case would cause an unnecessary log roll.
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

    //  verify the log path

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

    //  get the atomic write size

    CallR( m_pinst->m_pfsapi->ErrFileAtomicWriteSize( rgchFullName, (DWORD*)&m_cbSecVolume ) );
    Assert( FPowerOf2( m_cbSecVolume ) );
    // Sector sizes reported to be greater than 4k are either likely to be bogus
    // or the device may guarantee durability of sub-sector-size writes anyway
    // (since NTFS only uses logical sector size of upto 4k)
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
    if ( // sector size must be at least 512 ...
         ( m_cbSecVolume < 512 ) ||
         // the header must be an even # of sectors
         ( sizeof( LGFILEHDR ) % m_cbSecVolume != 0 ) )
    {
        //  indicate that this is a configuration error
        //
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagConfiguration, L"03059212-80e6-4ff3-a548-19697d44dcfe" );

        return ErrERRCheck( JET_errSectorSizeNotSupported );
    }

    //  Update the log sector size
    m_cbSec_ = LOG_SEGMENT_SIZE;
    m_log2cbSec_ = Log2(m_cbSec_);

    //  we've set the sector size via the log as well.
    
    m_csecHeader = ( sizeof( LGFILEHDR ) + m_cbSec - 1 ) / m_cbSec;

    //  log file size must be larger than a header plus two pages of log data
    //  and must be less than a header plus 65535 sectors of log data.
    //  Prevent underflow and overflow by limiting parameter values
    //
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
    _In_ PCWSTR wszLogFolder,
    _In_ PCWSTR wszBaseName,
    const enum eLGFileNameSpec eLogType,
    LONG lGen,
    _In_ PCWSTR wszLogExt,
    ULONG cLogDigits )
{
    ERR err = JET_errSuccess;
    WCHAR wszLogFileName[20];

    const WCHAR * wszExt = wszLogExt;

    // This is to defend any of the lGen* signals being passed down.
    //  Note: eArchiveLog or eReserveLog + lGen == 0 is used as wildcard.
    Expected( lGen > 0 || eLogType == eCurrentLog || eLogType == eCurrentTmpLog || eLogType == eArchiveLog || eLogType == eReserveLog );
    Expected( lGen != lGenSignalCurrentID ); // use lgen = 0 & eCurrentLog
    Expected( lGen >= 0 ); // actually all signals are unexpected.

    Assert( wszExt );

    OSStrCbCopyW( wszLogFileName, sizeof(wszLogFileName), wszBaseName );

    switch ( eLogType ){

    case eCurrentLog:
        // Already done ...
        Assert( lGen == 0 );
        Assert( 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        break;

    case eCurrentTmpLog:
        Assert( lGen == 0 );
        Assert( 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        OSStrCbAppendW( wszLogFileName, sizeof(wszLogFileName), wszLogTmp );
        break;

    case eReserveLog:
        wszExt = wszResLogExt; // We leave no choice here, legacy reserve logs is handled privately in ErrLGIDeleteLegacyReserveLogFiles().
        cLogDigits = 5;
        OSStrCbAppendW( wszLogFileName, sizeof(wszLogFileName), wszLogRes );
        // fall through ...  the rest of reserve log format is like archive logs ...
    case eArchiveLog:
    case eShadowLog:
        Assert( eLogType != eArchiveLog || 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        Assert( eLogType != eReserveLog || 0==_wcsicmp( wszLogExt, wszNewLogExt) || 0==_wcsicmp( wszLogExt, wszOldLogExt) );
        Assert( eLogType != eShadowLog || 0==_wcsicmp( wszLogExt, wszSecLogExt ) );
        if ( lGen == 0 )
        {
            //  lGen of zero means, make a wildcard ...
            OSStrCbAppendW( wszLogFileName, sizeof(wszLogFileName), L"*" );
        }
        else
        {
            // Fill in the digits part ( either XXXXX | XXXXXXXX ) ...
            LGFileHelper::LGSzLogIdAppend ( wszLogFileName, sizeof(wszLogFileName), lGen, cLogDigits );
        }
        break;

    default:
        AssertSz( false, "Unknown path");
        // This will be treated like current log :P  But this function doesn't really fail.
    }

    Assert( LOSStrLengthW(wszLogFileName) >= 3 );

    CallR( pfsapi->ErrPathBuild( wszLogFolder, wszLogFileName, wszExt, wszLogName, cbLogName) );
    return(JET_errSuccess);

}

void LOG_STREAM::LGSetLogExt(
    BOOL fLegacy, BOOL fReset
    )
{
    // the first two phases of recovery pre-init, and pre-recovery is where we should be discovering
    // and setting the actual log ext ...
    Assert( !m_pLog->FLogInitialized() ||
            (m_pLog->FRecoveringMode() == fRecoveringNone) ||
            m_wszLogExt != NULL );
    Assert( fReset || // if not reset, we must be setting these for the first time.
            m_wszLogExt == NULL );

    m_wszLogExt = (fLegacy) ? wszOldLogExt : wszNewLogExt;
}

ERR LOG_STREAM::ErrLGMakeLogNameBaseless(
    __out_bcount(cbLogName) PWSTR wszLogName,
    ULONG cbLogName,
    _In_ PCWSTR wszLogFolder,
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
#endif // DEBUG

    //  log file header must be aligned on correct boundary for device;
    //  which is 16-byte for MIPS and 512-byte for at least one NT
    //  platform.
    //
    Alloc( m_plgfilehdr = (LGFILEHDR *)PvOSMemoryPageAlloc( sizeof(LGFILEHDR) * 2, NULL ) );
    //  This is * 4 because,
    //      one for the first log file header (m_plgfilehdr), 
    //      one for the second temp log file header (m_plgfilehdrT)
    //      and two for the 2 subsequent LRCK sectors we might write
    //      (see ErrLGIWriteFileHdr()).
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

    //  We will limit the maximum number of pattern-filling writes to half of the file size.
    //
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

/*
 *  Configures the FileModeFlags for all file opens (except log dumper).
 */

IFileAPI::FileModeFlags LOG_STREAM::FmfLGStreamDefault( const IFileAPI::FileModeFlags fmfAddl ) const
{
    Expected( ( fmfAddl & ~IFileAPI::fmfReadOnly ) == 0 );  //  no other options but RO expected

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


/*
 *  This sets the sector size for the log and related parameters (log header
 *  and file size) based upon that sector size.
 */

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

/*
 *  Read log file header, detect any incomplete or
 *  catastrophic write failures.  These failures will be used to
 *  determine if the log file is valid or not.
 *
 *  On error, contents of plgfilehdr are unknown.
 *
 *  fBypassDBPageAndLogSectorSizeChecks - bypasses integrity checks
 *      for the integrity of the database and log files.  Only to be
 *      used by dumping.
 *
 *  RETURNS     JET_errSuccess, or error code from failing routine
 */
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

    //  make sure all the log file sizes match properly (real size, size from header, and system-param size)

    Call( pfapiLog->ErrSize( &qwFileSize, IFileAPI::filesizeLogical ) );
    qwCalculatedFileSize = QWORD( plgfilehdr->lgfilehdr.le_csecLGFile ) *
                           QWORD( plgfilehdr->lgfilehdr.le_cbSec );
    if ( qwFileSize != qwCalculatedFileSize )
    {
        Call( ErrERRCheck( JET_errLogFileSizeMismatch ) );
    }

    Call( m_pLog->ErrLGVerifyFileSize( qwFileSize ) );

    // Now that all that is verified, we can use the new sizes
    LGSetSectorGeometry( plgfilehdr->lgfilehdr.le_cbSec, plgfilehdr->lgfilehdr.le_csecLGFile );
    if ( FIsOldLrckLogFormat( plgfilehdr->lgfilehdr.le_ulMajor ) )
    {
        Assert( m_cbSecSavedForOB0 == 0 || m_cbSecSavedForOB0 == m_cbSec_ );
        m_cbSecSavedForOB0 = m_cbSec_;
        m_csecFileSavedForOB0 = m_csecLGFile;
        m_lGenSavedForOB0 = plgfilehdr->lgfilehdr.le_lGeneration;
    }

    //  We only do this on "recent version" logs ...
    if ( plgfilehdr->lgfilehdr.le_ulMinor == ulLGVersionMinorFinalDeprecatedValue ||    // probably unnecessary check
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

        //  we have successfully opened the log file, and the volume sector size has been
        //      set meaning we are allowed to adjust the sector sizes
        //      (when the volume sector size is not set, we are recovering but we have not
        //       yet started the redo phase -- we are in preparation)

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

    //  Fast path - check assuming all persisted versions are current!
    //

    const FormatVersions * pfmtversMax = PfmtversEngineMax();
    Assert( efvFormatFeature <= pfmtversMax->efv ); // any EFV / feature to check should be < than the engines last EFV in the table.
    if ( efvFormatFeature <= pfmtversMax->efv &&
            //  since _most likely_ we're on the current version (as setting EFVs back is usually temporary) ...
            0 == CmpLgVer( pfmtversMax->lgv, lgvCurrentFromLogGenTip ) )
    {
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "Log Feature test fast path - success: %d <= %d - %d.%d.%d == %d.%d.%d\n", efvFormatFeature, pfmtversMax->efv,
                    pfmtversMax->lgv.ulLGVersionMajor, pfmtversMax->lgv.ulLGVersionUpdateMajor, pfmtversMax->lgv.ulLGVersionUpdateMinor,
                    lgvCurrentFromLogGenTip.ulLGVersionMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMinor ) );
        return JET_errSuccess;  //  yay! fast out ...
    }
    //  else the header version didn't match and/or EFV is lower than last EFV in table ... do slow method

    //  Slow path - search version table for EFV and check directly ...
    //

    const FormatVersions * pfmtversFormatFeature = NULL;
    CallS( ErrGetDesiredVersion( NULL /* must be NULL to bypass staging */, efvFormatFeature, &pfmtversFormatFeature ) );
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
                /*

                Windows did not complain about this one like similar event 642, but for consistency it is also disabled.

                WCHAR wszEfvSetting[cchFormatEfvSetting];
                WCHAR wszEfvDesired[60];
                WCHAR wszEfvCurrent[60];
                const WCHAR * rgszT[3] = { wszEfvSetting, wszEfvDesired, wszEfvCurrent };
                
                FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion ), wszEfvSetting, sizeof(wszEfvSetting) );
                OSStrCbFormatW( wszEfvDesired, _cbrg( wszEfvDesired ), L"%d (0x%x - %d.%d.%d)", pfmtversFormatFeature->efv, pfmtversFormatFeature->efv, pfmtversFormatFeature->lgv.ulLGVersionMajor, pfmtversFormatFeature->lgv.ulLGVersionUpdateMajor, pfmtversFormatFeature->lgv.ulLGVersionUpdateMinor );
                OSStrCbFormatW( wszEfvCurrent, _cbrg( wszEfvCurrent ), L"%d.%d.%d", lgvCurrentFromLogGenTip.ulLGVersionMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMajor, lgvCurrentFromLogGenTip.ulLGVersionUpdateMinor );
                
                UtilReportEvent(
                        eventInformation,
                        GENERAL_CATEGORY,
                        LOG_FORMAT_FEATURE_REFUSED_DUE_TO_ENGINE_FORMAT_VERSION_SETTING,
                        _countof( rgszT ),
                        rgszT,
                        0,
                        NULL,
                        m_pinst );
                */
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


/*  Create the log file name (no extension) corresponding to the lGeneration
/*  in szFName. NOTE: szFName need minimum 12 bytes.
/*
/*  PARAMETERS  rgbLogFileName  holds returned log file name
/*              lGeneration     log generation number to produce    name for
/*  RETURNS     JET_errSuccess
/**/

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

    ichBase = LOSStrLengthW(wszLogFileName); // LOSStrLengthW(wszLogFileName) for log base name or res log base size
    Assert( ichBase == 3 || (ichBase == 6 && 0 == LOSStrCompareW(&(wszLogFileName[3]), wszLogRes)) );
    Assert( cbFName >= ((ichBase+cchLogDigits+1)*sizeof(WCHAR)) );

    if ( cchLogDigits == 0 )
    {
        if ( lGeneration > 0xFFFFF )
        {
            // UNDONE: It was decided no one wanted to prevent the 8.3 to 11.3 rollover.
            //Assert( !((ULONG)UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeCompat) );
            cchLogDigits = 8;
        }
        else
        {
            cchLogDigits = 5;
        }
    }
    Assert( cchLogDigits == 5 || cchLogDigits == 8 );

    ich = cchLogDigits + ichBase - 1;  //  + ichBase name's size - 1 for zero offset

    if ( (sizeof(WCHAR)*(cchLogDigits+ichBase+1)) > cbFName )
    {
        AssertSz(false, "Someone passed in a path too short for ErrLGSzLogIdAppend(), we'll recover, though I don't know how well");
        if ( cbFName >= sizeof(WCHAR) )
        { // lets at least make sure it is NUL terminated.
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

//  open edb.jtx/log and recognize the case where we crashed while creating a new log file
//  if fPickLogExt is set, then try both log extensions, and config the LOG to the existing one.

ERR LOG_STREAM::ErrLGTryOpenJetLog( const ULONG eOpenReason, const LONG lGenNext, BOOL fPickLogExt )
{
    ERR     err;
    WCHAR   wszPathJetTmpLog[IFileSystemAPI::cchPathMax];
    PCWSTR  wszLogExt = m_wszLogExt ? m_wszLogExt : m_pLog->WszLGGetDefaultExt( fFalse );

    Assert( m_wszLogExt || fPickLogExt );
    Assert( lGenNext > 0 || lGenNext == lGenSignalCurrentID );

OpenLog:

    //  create the file name/path for <inst>.<log_ext> and <inst>tmp.<log_ext>

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

    //  try to open edb.jtx/log

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
            //  we failed to open edb.jtx/log, but the failure was not related to the file not being found
            //  we must assume there was a critical failure during file-open such as "out of memory"
            m_pLog->LGReportError( LOG_OPEN_FILE_ERROR_ID, err );
            return err;
        }

        err = ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetTmpLog, NULL );
        if ( err < 0 )
        {
            //  edb.jtx/log does not exist, and there's some problem accessing
            //  edbtmp.jtx/log (most likely it doesn't exist either), so
            //  return file-not-found on edb.jtx/log

            if ( fPickLogExt && JET_errFileNotFound == err && m_pLog->FLGIsDefaultExt( fFalse, wszLogExt ) )
            {
                // We tried the default log extension, to no avail, now lets try the other log extension
                wszLogExt = m_pLog->WszLGGetOtherExt( fFalse );
                Assert( wszLogExt != m_wszLogExt );
                goto OpenLog;
            }

            //
            // Ok, really fail this time, because we've tried both log extensions.
            //

            if ( m_wszLogExt == NULL )
            {
                // If no log extension is choosen yet, set the m_wszLogName back
                // to the default, so no one is confused.
                CallS( ErrLGMakeLogNameBaseless( m_wszLogName, sizeof(m_wszLogName), m_pLog->SzLGCurrentFolder(), eCurrentLog, 0, m_pLog->WszLGGetDefaultExt( fFalse ) ) );
            }
            return ErrERRCheck( JET_errFileNotFound );
        }

        CallS( err );   //  warnings not expected

        //  we don't have edb.jtx/log, but we have edbtmp.jtx/log
        //
        err = ErrERRCheck( JET_errFileNotFound );
    }
    else
    {
        CallS( err );   //  no warnings expected
    }


    if( fPickLogExt )
    {
        Assert( m_wszLogExt == NULL );
        LGSetLogExt( m_pLog->FLGIsLegacyExt( fFalse, wszLogExt ), fFalse );
        m_pLog->LGSetChkExts( m_pLog->FLGIsLegacyExt( fFalse, wszLogExt ), fFalse );
    }

    return err;
}

//  open edb.jtx/log and recognize the case where we crashed while creating a new log file
//  if fPickLogExt is set, then try both log extensions, and config the LOG to the existing one.

ERR LOG_STREAM::ErrLGOpenJetLog( const ULONG eOpenReason, const ULONG lGenNext, BOOL fPickLogExt )
{
    Assert( lGenNext > 0 ); // don't think current gen comes in here ... if so, fix QwInstFileID() to get lGenSignalCurrentID
    const ERR errOpen = ErrLGTryOpenJetLog( eOpenReason, lGenNext, fPickLogExt );
    if ( errOpen == JET_errFileNotFound )
    {
        //  all other errors will be reported in ErrLGTryOpenJetLog()
        //  
        m_pLog->LGReportError( LOG_OPEN_FILE_ERROR_ID, errOpen );
    }
    return errOpen;
}

/*  open a generation file on CURRENT directory
/**/
ERR LOG_STREAM::ErrLGTryOpenLogFile( const ULONG eOpenReason, const enum eLGFileNameSpec eLogType, const LONG lgen )
{
    ERR err = JET_errSuccess;

    Assert( lgen != 0 || eCurrentLog == eLogType );
    Assert( lgen > 0 ); // don't think current gen comes in here ... if so, fix QwInstFileID() to get lGenSignalCurrentID
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
            //  This is more serious than just missing the current log, we're missing a log required 
            //  for recovery, and we used to emit a failure tag for this, and we should continue to
            //  do so.
            OSUHAEmitFailureTag( m_pinst, HaDbFailureTagRecoveryRedoLogCorruption, L"afb4f58d-41af-41ad-a3bb-e22c053839c0" );
        }

        err = ErrERRCheck( errMissingLog );
    }

    return err;
}


/*  open the redo point log file which must be in current directory.
/**/
ERR LOG_STREAM::ErrLGRIOpenRedoLogFile( LGPOS *plgposRedoFrom, INT *pfStatus )
{
    ERR     err;
    BOOL    fJetLog = fFalse;

    /*  try to open the redo from file as a normal generation log file
    /**/
    err = ErrLGTryOpenLogFile( JET_OpenLogForRedo, eArchiveLog, plgposRedoFrom->lGeneration );
    if( err < 0 )
    {
        if ( JET_errFileNotFound != err )
        {
            return err;
        }


        //  gen doesn't exist, so see if redo point is in szJetLog.
        err = ErrLGOpenJetLog( JET_OpenLogForRedo, plgposRedoFrom->lGeneration );
        if ( err < 0 )
        {
            *pfStatus = fNoProperLogFile;
            return JET_errSuccess;
        }

        fJetLog = fTrue;
    }

    /*  read the log file header to verify generation number
    /**/
    CallR( ErrLGReadFileHdr( NULL, IorpLogRead( m_pLog ), m_plgfilehdr, fCheckLogID ) );

    // XXX
    // At this point, if m_lgposLastRec.isec == 0, that means
    // we want to read all of the log records in the file that
    // we just opened up. If m_lgposLastRec.isec != 0, that means
    // that this is the last log file and we only want to read
    // up to a certain amount.

    /*  the following checks are necessary if the szJetLog is opened
    /**/
    if( m_plgfilehdr->lgfilehdr.le_lGeneration == plgposRedoFrom->lGeneration )
    {
        *pfStatus = fRedoLogFile;
    }
    else if ( fJetLog && ( m_plgfilehdr->lgfilehdr.le_lGeneration == plgposRedoFrom->lGeneration + 1 ) )
    {
        /*  this file starts next generation, set start position for redo
        /**/
        plgposRedoFrom->lGeneration++;
        plgposRedoFrom->isec = (WORD) m_csecHeader;
        plgposRedoFrom->ib   = 0;

        *pfStatus = fRedoLogFile;
    }
    else
    {
        /*  log generation gap is found.  Current szJetLog can not be
        /*  continuation of backed up logfile.  Close current logfile
        /*  and return error flag.
        /**/
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
    const LONG              lgenDebug ) // lgenDebug can be signal - lgenSignalReserveID / 0x80000003
{
    ERR                     err;
    IFileAPI *              pfapi   = NULL;

    Assert( lgenDebug > 0 || lgenDebug == lgenSignalReserveID );
    // when lgenDebug is not lgenSignalReserveID, it is from shadow logging ... we could check
    // that wszPath edb00000007.jsl name matches lgenDebug passed in if we needed to be extra sure.

    const QWORD qwSize = QWORD( m_csecLGFile ) * m_cbSec;
    TraceContextScope tcScope( iorpLog );

    //  create the empty file
    //
    CallR( CIOFilePerf::ErrFileCreate(
                        m_pinst->m_pfsapi,
                        m_pinst,
                        wszPath,
                        FmfLGStreamDefault(),
                        iofileLog,
                        QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgenDebug ),
                        &pfapi ) );

    //  Set the size to the expected log file size to avoid extending it needlessly as we 
    //  fill it with data. 
    Call( pfapi->ErrSetSize( *tcScope, qwSize, fTrue, QosSyncDefault( m_pinst ) ) );

HandleError:
    // Do not need to flush the file, if it disappears or loses its size on system crash, we will just recreate it
    pfapi->SetNoFlushNeeded();
    delete pfapi;

    if ( err < JET_errSuccess )
    {
        //  the operation has failed
        //

        //  delete the file
        //  it should be deletable, unless by some random act it was made read-only
        //  or opened by another process; in the failure case, we will leave
        //  the malformed log file there, but it will be reformatted/resized
        //  before we actually use it -- in a sense, it will be "recreated" in-place.
        //
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
    // for legacy {res1.log, res2.log} reserve log files cleanup
    const WCHAR* const  rgwszLegacyResLogBaseNames[]    = { L"res1", L"res2" };
    const size_t        cLegacyResLogBaseNames      = _countof( rgwszLegacyResLogBaseNames );

    for ( size_t iResLogBaseNames = 0; iResLogBaseNames < cLegacyResLogBaseNames; ++iResLogBaseNames )
    {
        CallS( m_pinst->m_pfsapi->ErrPathBuild( m_pLog->SzLGCurrentFolder(), rgwszLegacyResLogBaseNames[ iResLogBaseNames ], wszOldLogExt, wszPathJetLog ) );
        (void)m_pinst->m_pfsapi->ErrFileDelete( wszPathJetLog );
    }

    return( JET_errSuccess ); // best effort ...
}

//  used by ErrLGCheckState

ERR LOG_STREAM::ErrLGCheckState()
{
    ERR     err     = JET_errSuccess;

    m_rwlLGResFiles.EnterAsReader();

    //  re-check status in case disk space has been freed

    if ( m_ls != lsNormal )
    {
        //  try to get the write lock

        m_rwlLGResFiles.LeaveAsReader();
        m_rwlLGResFiles.EnterAsWriter();

        if ( m_ls != lsNormal )
        {
            (void)ErrLGIGrowReserveLogPool( ( BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ? cMinReserveLogsWithAggresiveRollover : cMinReserveLogs ) - m_cReserveLogs,
                                            fTrue );
        }

        m_rwlLGResFiles.LeaveAsWriter();
        m_rwlLGResFiles.EnterAsReader();

        //  if we did not recover from log disk full then reject this update

        if ( m_ls != lsNormal )
        {
            err = ErrERRCheck( JET_errLogDiskFull );
        }
    }

    //  if we are out of log sequence then reject this update

    if ( err >= JET_errSuccess && m_fLogSequenceEnd )
    {
        err = ErrERRCheck( JET_errLogSequenceEnd );
    }

    m_rwlLGResFiles.LeaveAsReader();

    return err;
}


//  Initialize the reserve log pool, and remove legacy res logs (res1.log and res2.log).
ERR LOG_STREAM::ErrLGInitCreateReserveLogFiles()
{
    WCHAR           wszPathJetLog[IFileSystemAPI::cchPathMax];
    WCHAR           wszResBaseName[16];
    const ULONG     cInitialReserveLogs = BoolParam( m_pinst, JET_paramAggressiveLogRollover ) ? cMinReserveLogsWithAggresiveRollover : cMinReserveLogs;

    // UNDONE: it might be very interesting to make the cInitialReserveLogs settable in such
    // a way that we reserve a baseline worth of logs that we don't start quiescing on, we just
    // start excessively start logging events to the event log, but don't start quiescing, until
    // the pool drops below some lower minimum based upon how much we'll need to rollback.

    m_rwlLGResFiles.EnterAsWriter();
    m_ls = lsNormal;

    //
    //  Cleanup any legacy res ext logs, mismatched sized res logs, or if the pool is
    //  excessively large (more than cInitialReserveLogs) ...

    IFileFindAPI*   pffapi          = NULL;
    QWORD           cbSize;
    const QWORD     cbSizeExpected  = QWORD( m_csecLGFile ) * m_cbSec;

    //  Remove legacy reserve log files ...
    ErrLGDeleteLegacyReserveLogFiles();

    //  We need to cleanup the old reserve logs if they are the wrong size
    //  NOTE: if the file-delete call(s) fail, it is ok because this will be
    //      handled later when ErrLGNewLogFile expands/shrinks the reserve
    //      reserve log to its proper size
    //      (this solution is purely proactive)

    LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eReserveLog, 0 ); // <-- zero means make wildcard.
    OSStrCbCopyW( wszResBaseName, sizeof(wszResBaseName), SzParam( m_pinst, JET_paramBaseName ) );
    OSStrCbAppendW( wszResBaseName, sizeof(wszResBaseName), wszLogRes );

    if ( m_pinst->m_pfsapi->ErrFileFind( wszPathJetLog, &pffapi ) == JET_errSuccess )
    {
        while ( pffapi->ErrNext() == JET_errSuccess )
        {
            LONG lResLogNum = 0;
            ULONG cLogDigits = 0;

            // Retrieve enough information to make an intelligent decision on whether to keep the log file
            if ( pffapi->ErrPath( wszPathJetLog ) != JET_errSuccess ||
                pffapi->ErrSize( &cbSize, IFileAPI::filesizeLogical ) != JET_errSuccess ||
                LGFileHelper::ErrLGGetGeneration( m_pinst->m_pfsapi, wszPathJetLog, wszResBaseName, &lResLogNum, wszResLogExt, &cLogDigits ) < JET_errSuccess
                )
            {
                // these are not the droids we're looking for, lets move along ...
                continue;
            }

            // remove the log, if it doesn't meet our requirements ...
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



    //  Init to zero, grow will create any necessary files ...
    m_cReserveLogs = 0;

    //  Grow the pool to an initial size.
    (void)ErrLGIGrowReserveLogPool( cInitialReserveLogs, fFalse );

    //  We tried to ensure that our reserve log files exist and may or may not have succeeded
    //      on failure, m_ls should be set appropriately to quiesce or out of space ...

    m_rwlLGResFiles.LeaveAsWriter();
    return JET_errSuccess;
}

// This grows the reserve log pool by cResLogsInc logs (creates files, and sets m_cReserveLogs).  Note
// it resets m_ls to lsNormal if we were able to grow logs.  If we couldn't grow, it can kick us into
// lsQuiesce if we don't have space, if the file exists, it doesn't create one ...

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
    Assert( cResLogsInc ); // should be growing by at least one log.

    if ( lsBefore == lsOutOfDiskSpace )
        return ErrERRCheck( JET_errLogDiskFull );

    // start with the next log after the # we currently have, and grow til the target # of logs is achieved.

    cTargetReserveLogs = ( m_cReserveLogs + cResLogsInc );
    Assert( cTargetReserveLogs < UlParam( m_pinst, JET_paramCheckpointTooDeep ) ); // That'd just be silly!

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

        // Success, one more res log created, increment global count
        m_cReserveLogs++;

    }

    if ( cResLogsInc )
    {
        m_ls = ( fQuiesce ? lsQuiesce : lsNormal );
    }

    return(err);
}


//  Create Asynchronous Log File Assert

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
    Assert( lgenNextDebug > 0 ); // shouldn't be signal or anything else.

    *ppfapi = NULL;

    Assert( m_rwlLGResFiles.FNotWriter() );
    m_rwlLGResFiles.EnterAsWriter();

    /*  use reserved log file and change to log state
    /**/
    for( ithReserveLog = m_cReserveLogs; ithReserveLog > 0; ithReserveLog-- )
    {

        LGMakeLogName( wszLogName, sizeof(wszLogName), eReserveLog, ithReserveLog );

        err = m_pinst->m_pfsapi->ErrFileMove( wszLogName, wszPathJetTmpLog );
        Assert( err < 0 || err == JET_errSuccess );
        if ( err == JET_errSuccess )
        {

            // successfully used up one of the logs ...
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
                //  We renamed the Nth .jrs to edbtmp.trx/log but were unable to open it.
                //  We should delete edbtmp.trx/log because we can't open it and we still need
                //  to try to use edb<ith>.jrs.
                //  Delete everything except the last reserve log file ...
                if ( ithReserveLog > 1 )
                {
                    (void) m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog );
                }
            }
            else
            {
                // Whew! But we're not out of the deep end yet ...
                Assert( err >= JET_errSuccess );
                break;
            }

        }

    }

    // UNDONE: Once we feel we've protected ourselves against all reasonable
    // out of disk errors, we can remove this last clause.
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

        // We couldn't get ANY of the reserve log files open, or they're all exhausted already ...

        Assert( err < JET_errSuccess || m_cReserveLogs == 0 );
        Assert( m_ls == lsQuiesce || m_ls == lsOutOfDiskSpace );

        //  this function is only called if we're out of log
        //  disk space and trying to resort to the reserved
        //  logs, so if we can't open the reserved logs for
        //  ANY reason (most common reason is that we're
        //  getting FileNotFound because the reserved logs
        //  have already been consumed), then just bail with
        //  LogDiskFull
        //
        err = ErrERRCheck( JET_errLogDiskFull );

        if ( m_ls == lsQuiesce )
        {
            const WCHAR*    rgpsz[ 1 ];
            DWORD           irgpsz      = 0;
            WCHAR           wszJetLog[ IFileSystemAPI::cchPathMax ];

            LGMakeLogName( wszJetLog, sizeof(wszJetLog), eCurrentLog );

            rgpsz[ irgpsz++ ]   = wszJetLog;

            // If we could not open either of the reserved log files,
            // because the disk is full, then we're out of luck and
            // really out of disk space.
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


//  Parameters
//
//      ppfapi
//          [out] file interface to opened edbtmp.jtx/log
//      pszPathJetTmpLog
//          [in] full path to edbtmp.jtx/log that we will be opening.
//      pfResUsed
//          [out] Whether the edbtmp.jtx/log file that was opened was formerly
//          a reserve log file.
//
//  Return Values
//
//      Errors cause *ppfapi to be NULL.

ERR LOG_STREAM::ErrLGIOpenTempLogFile(
    IFileAPI** const        ppfapi,
    const WCHAR* const      wszPathJetTmpLog,
    const LONG              lgenNextDebug, // note: may be lgenSignalTempID / 0x80000001.
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

    //  Delete any existing edbtmp.jtx/log in case it exists.

    if ( ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetTmpLog ) >= JET_errSuccess )
    {
        //  If there exists a temp log file, we don't know what kind of shape
        //  it is in, so we'll truncate it's size to zero, and rebuild it
        //  from scratch.
        //
        //  This is important approach in order to be crash resilient ...
        //  If we were in the middle of rolling a log and there is no edb.log, 
        //  but exists an edbtmp.log then we're in a crash consistent state.
        //  Having no edb.log and edbtmp.log is not consider crash consistent,
        //  as NTFS is supposed to protect us from that state.  SOO... when
        //  we start recovery and we only have an edbtmp.log (and since we
        //  decided to no longer move the highest archive log back to edb.log
        //  ... which was the old behavior in Win7/Exch2k10 RTM and earlier)
        //  then we need to do recovery redo on only archive logs, and then
        //  repair the edbtmp.log to be an appropriately pattern filled log
        //  and then move it to edb.log.

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
        //  No existing edbtmp.log exists, we need to either get one from the archive
        //  logs (in the case of circular logging) or create a new one ourselves.

        err = ErrLGIReuseArchivedLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug );

        //  We don't have an edbtmp.jtx/log open because we couldn't reuse an old log file,
        //  so try to create a new one.

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
                        // Using an archive file of the correct size and version,
                        // do not need to zero fill
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

                //  Error in trying to create a new edbtmp.jtx/log so try to use a reserve log
                //  instead.

                if ( JET_errDiskFull == err )
                {
                    Assert( lgenNextDebug > 0 || lgenNextDebug == lGenSignalTempID );
                    Call( ErrLGIOpenReserveLogFile( ppfapi, wszPathJetTmpLog, lgenNextDebug ) );
                    *pfResUsed = fTrue;
                    // Using a reserve file, do not need to pattern fill
                    *pfZeroFilled = fTrue;
                    Assert( *ppfapi );
                }
            }
            else
            {
                Assert( NULL != *ppfapi );

                //  For the newly created log, set the size to the expected log file size to avoid extending it needlessly as we 
                //  fill it with data. Note that the param for log size is in KB, so we need to convert it to bytes.
                Call( (*ppfapi)->ErrSetSize( *tcScope, qwSize, fFalse, QosSyncDefault( m_pinst ) ) );
            }
        }
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        if ( *ppfapi )
        {
            //  Temp log, we will flush before adding log to required range
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

    //  If circular log file flag set and backup not in progress
    //  then find oldest log file and if no longer needed for
    //  soft-failure recovery, then rename to szJetTempLog.
    //  Also delete any unnecessary log files.

    //  SYNC: No locking necessary for JET_paramCircularLog since it is immutable
    //  while the instance is initialized.

    //  SYNC: We don't need to worry about a backup starting or stopping while
    //  we're executing since if circular logging is enabled, a backup will not mess
    //  with the archived log files.

    if ( BoolParam( m_pinst, JET_paramCircularLog ) )
    {
        //  when find first numbered log file, set lgenLGGlobalOldest to log file number
        //
        LONG lgenLGGlobalOldest = 0;

        (void)ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenLGGlobalOldest, NULL );

        //  if found oldest generation and older than checkpoint,
        //  then move log file to szJetTempLog.  Note that the checkpoint
        //  must be flushed to ensure that the flushed checkpoint is
        //  after then oldest generation.
        //
        //  if a backup is in progress, then also need to verify
        //  that the oldest generation is less than the minimum
        //  logfile to be copied (it should always be less
        //  than the checkpoint, but we perform both checks
        //  just to be safe)
        //
        if ( lgenLGGlobalOldest != 0 // found existing archive logs ...
            && ( !m_pinst->m_pbackup->FBKBackupInProgress( BACKUP_CONTEXT::backupLocalOnly ) || lgenLGGlobalOldest < m_pinst->m_pbackup->BKLgenCopyMic() ) )
        {
            //  UNDONE: related issue of checkpoint write
            //          synchronization with dependent operations
            //  UNDONE: error handling for checkpoint write
            //
            (VOID) m_pLog->ErrLGUpdateCheckpointFile( fFalse );
            if ( lgenLGGlobalOldest < m_pLog->LgposGetCheckpoint().le_lGeneration )
            {
                WCHAR   wszLogName[ IFileSystemAPI::cchPathMax ];

                LGMakeLogName( wszLogName, sizeof(wszLogName), eArchiveLog, lgenLGGlobalOldest );
                err = m_pinst->m_pfsapi->ErrFileMove( wszLogName, wszPathJetTmpLog );
                Assert( err <= JET_errSuccess );
                if ( err == JET_errSuccess )
                {
                    lgenLGGlobalOldest++;   //  Rename was successful above.
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
                        //  We renamed the archived log file to edbtmp.jtx/log but were unable to open it,
                        //  so try to delete it as last resort to stablize filesystem state.
                        //
                        (VOID) m_pinst->m_pfsapi->ErrFileDelete( wszPathJetTmpLog );
                    }

                    else
                    {
                        //  delete unnecessary log files.  At all times, retain
                        //  a continguous and valid sequence.
                        //
                        do
                        {
                            Assert( lgenLGGlobalOldest <= m_pLog->LgposGetCheckpoint().le_lGeneration );

                            //  Three basic conditions prevent deleting this log:
                            //    1- Backup is in progress.
                            //
                            //    2- The log is beyond the checkpoint (with some room added to account
                            //       for fluctuations in checkpoint depth and prevent fresh logs from
                            //       being re-created, thus incurring I/O).
                            //
                            //    3- If we're running with persisted lost flush detection enabled, keep
                            //       a few extra logs because the checkpoint advancement resembles
                            //       a saw-like pattern, which means we'll delete old logs and re-create
                            //       new ones repeatedly. Creating new logs incurs extra I/O due to pattern
                            //       filling.
                            //
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
            //  Everyone else's trigger-checks should fail since this thread will handle this
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
    //  Should not hold log buffer over the issuing of the log extension I/O.
    //  Issuing 1 IO at a time may be insufficient and may cause tmp log extension to fall
    //  behind, which will cause the overall instance to hang when swithcing to a new log.
    //  Issuing multiple IOs at a time seems to be a better approach, but it brings up two
    //  problems: we would have to have as many reserved IOREQs as IOs we we'd want to issue
    //  concurrently (SOMEONE reports the requirement for a reserved IO was added due to a deadlock bug
    //  hit in OOM conditions); the second problem is that no order is guaranteed for the IOs
    //  to complete, what means we may have extra zeroing IOs being issued to fill gaps (that would be
    //  later filled with the pattern) if a higher-offset IO is issued first. One possible approach would
    //  be to issue only 1 async IO and issue the subsequent ones upon each completion. This proved
    //  not to be very efficient because we'll have several asnc IOs which could take, each one, a
    //  a reasonable amount of time to be issued from the heap and to have their completion processed
    //  by the IO manager. The approach we chose was to post a task that will issue the IOs in order
    //  and synchronously.
    //
    // Assert( m_critLGBuf.FNotOwner() );

    //  ensure the log isn't about to be closed (the scenario is that
    //  some thread is in either LGCreateAsynchCancel()
    //  or ErrOpenTempLogFile() and just before it's able to grab
    //  m_critJetTmpLog, this thread schedules an extending write,
    //  but if we don't grab m_critJetTmpLog here and check
    //  m_pfapiJetTmpLog, it's possible that m_pfapiJetTmpLog can
    //  get NULL'ed out and we'll crash below when we try to
    //  deref it in order to call ErrIOWrite())
    //
    QWORD cbIOIssued = 0;
    DWORD cIOIssued = 0;
    if ( m_critJetTmpLog.FTryEnter() )
    {
        if ( NULL != m_pfapiJetTmpLog && !m_fCreateAsynchZeroFilled )
        {
            Assert( m_ibJetTmpLog <= QWORD( CbLGLogFileSize() ) );
            Assert( m_cIOTmpLog == 0 );

            //  Are we falling behind?
            //
            const QWORD cbDeltaLogRec = ( CmpLgpos( lgposLogRec, m_lgposLogRecAsyncIOIssue ) < 0 ) ? 0 : CbOffsetLgpos( lgposLogRec, m_lgposLogRecAsyncIOIssue );
            DWORD cIO = static_cast<DWORD>( min( max( cbDeltaLogRec / cbLogExtendPattern, 1 ), m_cIOTmpLogMax ) );

            //  Compute the chunks.
            //
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
                
                //  Issue a task to do the actual IO synchronously. Give up in case the task failed to be posted.
                //  Note that async log creation will be re-triggered the next time we roll the log.
                //
                if ( g_taskmgrLog.ErrTMPost( (CGPTaskManager::PfnCompletion)LGWriteTmpLog_, this ) < JET_errSuccess )
                {
                    m_cIOTmpLog = cIOIssued = 0;
                    cbIOIssued = 0;
                    m_ibJetTmpLog = ibJetTmpLog;
                }
                
                //  Restore cleanup checking
                //
                FOSSetCleanupState( fCleanUpStateSaved );
            }
        }
        
        m_critJetTmpLog.Leave();
    }

    //  There might be someone waiting for this signal. Also, resume the triggering mechanism.
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
    //  Is anyone else taking care of pattern-filling the log?
    //
    if ( m_critCreateAsynchIOExecuting.FTryEnter() )
    {
        const OSFILEQOS     qos                 = QosSyncDefault( m_pinst ) | qosIOPoolReserved;
        const BYTE* const   pbData              = g_rgbZero;
        const DWORD         cIO                 = m_cIOTmpLog;

        //  Issue the synchronous I/Os.
        //
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

            //  If success, and more I/O to do, set trigger for when next I/O
            //  should occur, otherwise leave trigger as lgposMax to prevent any
            //  further checking or writing.
            //
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
                        m_errCreateAsynch = JET_errSuccess; // ignore this I/O error
                        m_ibJetTmpLog = ibOffset;           // synchronously continue from this point later
                                                            // and do I/O size backoff retry
                        fBackoff = fTrue;
                    }
                }
            }
            m_pLogWriteBuffer->UnlockBuffer();
        }

        //  Make sure we don't have external writers at this point.
        //
        Assert( cIO == m_cIOTmpLog );
        m_cIOTmpLog = 0;

        m_critCreateAsynchIOExecuting.Leave();
    }


    //  Set the completion signal. Since LOG::ErrLGTerm() waits for this signal to make sure no pending
    //  tasks will try to access the LOG*, the "this" pointer is not valid after
    //  m_asigCreateAsynchIOCompleted.Set() below. Do not use "this" after that.
    //
    Assert( !m_asigCreateAsynchIOCompleted.FTryWait() );
    m_asigCreateAsynchIOCompleted.Set();
}

//  Aborts async log creation (edbtmp.jtx/log), then close the file.
//  Optionally, we can wait for any pending tasks to complete.
//

VOID LOG_STREAM::LGCreateAsynchCancel( const BOOL fWaitForTask )
{
    //  SYNC:
    //  Must be inside of m_critLGWrite or in LOG termination because we
    //  can't have the write task concurrently modify m_pfapiJetTmpLog.

    if ( m_pfapiJetTmpLog ||                    // edbtmp.jtx/log opened
        m_errCreateAsynch < JET_errSuccess )    // error trying to open edbtmp.jtx/log
    {
        Assert( BoolParam( m_pinst, JET_paramLogFileCreateAsynch ) );

        m_critCreateAsynchIOExecuting.Enter();

        //  There is a small window where a IOWrite completed and 
        //  called m_asigCreateAsynchIOCompleted.Set(), allowing this
        //  code to NULL m_pfapiJetTmpLog, resulting in an AV in
        //  the subsequent call to ErrIOWrite in LGWriteTmpLog if a task
        //  is posted from LGCreateAsynchIOIssue.
        //  Close the window with m_critJetTmpLog. Additionally, we
        //  also have to grab m_critJetTmpLog here to prevent extra IOs
        //  from being issued.
        //
        m_critJetTmpLog.Enter();

        //  Should we wait for the actual task to complete?
        if ( fWaitForTask )
        {
            //  Wait for any outstanding IO to complete
            //
            m_asigCreateAsynchIOCompleted.Wait();
        }

        //  Cancelling log creation typical for LGTerm, snapshot backup to deleted the
        //  file, or mismatch log cleanup (again to delete file), so we can disable the
        //  need for writing (but we'll just try to write instead).
        if ( m_pfapiJetTmpLog )
        {
            //  Temp log, we will flush before adding log to required range
            m_pfapiJetTmpLog->SetNoFlushNeeded();
        }
        delete m_pfapiJetTmpLog;
        m_pfapiJetTmpLog = NULL;
        m_fCreateAsynchResUsed = fFalse;
        m_fCreateAsynchZeroFilled = fFalse;
        m_errCreateAsynch = JET_errSuccess;
        m_cIOTmpLog = 0;
        m_lgposCreateAsynchTrigger = lgposMax;

        //  m_ibJetTmpLog will be rewinded to the begining of the file. This might make us redo
        //  any work that has already been done on the temp log.
        //
        m_ibJetTmpLog = 0;

        m_critJetTmpLog.Leave();
        m_critCreateAsynchIOExecuting.Leave();
    }
}

//  ================================================================
VOID LOG_STREAM::CheckLgposCreateAsyncTrigger()
//  ================================================================
//
//  It is possible that m_lgposCreateAsynchTrigger != lgposMax if
//  edbtmp.jtx/log hasn't been completely formatted (the last I/O that
//  completed properly set the trigger). This will prevent
//  anyone from trying to wait for the signal since the trigger will
//  be lgpos infinity. Note that in this situation it is ok to read
//  m_lgposCreateAsynchTrigger outside of m_critLGBuf since no
//  one else can write to it (we're in m_critLGWrite which
//  prevents this function from writing to it, and any outstanding
//  I/O has completed so it won't write).
//
//-
{
    if ( CmpLgpos( lgposMax, m_lgposCreateAsynchTrigger ) )
    {
        m_pLogWriteBuffer->LockBuffer();
        m_lgposCreateAsynchTrigger = lgposMax;
        m_pLogWriteBuffer->UnlockBuffer();
    }
}


//  ================================================================
ERR LOG_STREAM::ErrOpenTempLogFile(
    IFileAPI ** const       ppfapi,
    const WCHAR * const     wszPathJetTmpLog,
    const LONG              lgenNextDebug,
    BOOL * const            pfResUsed,
    BOOL * const            pfZeroFilled,
    QWORD * const           pibPattern )
//  ================================================================
//
//  Open a temporary logfile. This involves either:
//      - canceling the background task that creates/pattern-fills the file
//      - opening/creating the file conventionally
//
//-
{
    ERR err = JET_errSuccess;

    Assert( lgenNextDebug > 0 ); // should not be signal lgens / lgenSignal*.

    if ( ( m_errCreateAsynch < JET_errSuccess ) || m_pfapiJetTmpLog )
    {
        Assert( BoolParam( m_pinst, JET_paramLogFileCreateAsynch ) );

        m_critCreateAsynchIOExecuting.Enter();

        //  There is a small window where a IOWrite completed and 
        //  called m_asigCreateAsynchIOCompleted.Set(), allowing this
        //  code to NULL m_pfapiJetTmpLog, resulting in an AV in
        //  the subsequent call to ErrIOWrite in LGWriteTmpLog if a task
        //  is posted from LGCreateAsynchIOIssue.
        //  Close the window with m_critJetTmpLog. Additionally, we
        //  also have to grab m_critJetTmpLog here to prevent extra IOs
        //  from being issued.
        //
        m_critJetTmpLog.Enter();

        //  "Cancel" task to issue async pattern-filling IO, if we need to.
        //
        if ( m_cIOTmpLog != 0 )
    {
            //  We have to rewind m_ibJetTmpLog to make sure it is resumed from the proper offset.
            //
            m_ibJetTmpLog = m_rgibTmpLog[ 0 ];
            m_cIOTmpLog = 0;
    }

        *ppfapi     = m_pfapiJetTmpLog;
        *pfResUsed  = m_fCreateAsynchResUsed;
        *pfZeroFilled = m_fCreateAsynchZeroFilled;

        //  If there was an error, leave *pibPattern to default so we don't
        //  accidentally use the wrong value for the wrong file. Scenario:
        //  some new developer modifies some code below that opens up
        //  a different file, yet leaves *pibPattern set to the previous
        //  high-water mark from the previous file we're err'ing out on.
        //
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

            //  Upon disk full from asynchronous log file creation, there must
            //  be an edbtmp.jtx/log at this point because any creation errors
            //  during asynch creation would have caused the reserves to
            //  be tried. And if the reserves have been tried, we can't be
            //  in this code path.

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

//  ================================================================
ERR LOG_STREAM::ErrFormatLogFile(
    IFileAPI ** const       ppfapi,
    const WCHAR * const     wszPathJetTmpLog,
    const LONG              lgenNextDebug,
    BOOL * const            pfResUsed,
    const QWORD&            cbLGFile,
    const QWORD&            ibPattern )
//  ================================================================
//
//  Synchronously fill the rest of the file.
//
//  If we run out of space on the disk when making a new log file or
//  reusing an archived log file (which might be a different size than
//  our current log files, I believe), then we should delete the file we're
//  trying to write to (which may free up some more space in the case
//  of extending an archived log file).
//
//  Complete formatting synchronously if it hasn't completed and truncate
//  log file to correct size if necessary
//
//-
{
    ERR err = JET_errSuccess;

    TraceContextScope tcScope( iorpLog );
    tcScope->iorReason.AddFlag( iorfFill );
    err = ErrUtilFormatLogFile( m_pinst->m_pfsapi, *ppfapi, cbLGFile, ibPattern, QosSyncDefault( m_pinst ), fTrue, *tcScope );
    if ( JET_errDiskFull == err && ! *pfResUsed )
    {
        delete *ppfapi;
        *ppfapi = NULL;

        //  If we can't delete the current edbtmp.jtx/log, we are in a bad place.

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


/*  Write log file header.
 *
 *  No need to shadow the log file header since it will not be overwritten.
 */

ERR LOG_STREAM::ErrLGIWriteFileHdr(
    IFileAPI* const         pfapi,      // file to write to
    LGFILEHDR* const        plgfilehdr )// log file header to set checksum of and write
{
    ERR err = JET_errSuccess;

    Assert( m_critLGWrite.FOwner() );

    Assert( plgfilehdr == m_plgfilehdrT );  //  This is allocated at least two sectors bigger ...

    Enforce( m_csecHeader * m_cbSec >= sizeof(LGFILEHDR) );

    //  setup the log file header
    const LogVersion * plgvDesired = &( PfmtversEngineSafetyVersion()->lgv );
    //  probably can't fail at this point because we're at least in end of recovery.  maybe for 
    //  a clean recovery with no redo phase.  certainly for engine default case won't fail.
    const JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion );
    const ERR errT = ErrLGGetDesiredLogVersion( efv, &plgvDesired );
    if ( errT < JET_errSuccess )
    {
        FireWall( OSFormat( "GetDesiredLogVerWriteFileHdrFailedVer:%lu", (ULONG)efv ) ); // handled by "SafetyVersion" above.
    }

    Assert( CmpLgVer( LgvFromLgfilehdr( plgfilehdr ), *plgvDesired ) >= 0 );

    SetPageChecksum( plgfilehdr, sizeof( LGFILEHDR ), logfileHeader, 0 );

    //  issue the write

    Call( ErrLGIWriteSectorData( pfapi, IOR( iorpLog, iorsHeader ), plgfilehdr->lgfilehdr.le_lGeneration, 0, sizeof( LGFILEHDR ), (BYTE*)plgfilehdr, LOG_HEADER_WRITE_ERROR_ID, NULL, fTrue ) );
    
    Assert( m_critLGWrite.FOwner() );

HandleError:
    return err;
}


//  ================================================================
ERR LOG_STREAM::ErrStartAsyncLogFileCreation(
    _In_ const WCHAR * const  wszPathJetTmpLog,
    _In_       LONG           lgenDebug    // note: can be lGenSignalTempID
    )
//  ================================================================
//
//  Start working on preparing edbtmp.jtx/log by opening up the file and
//  setting the trigger for the first I/O. Note that we will only do
//  asynch creation if we can extend the log file with arbitrarily big
//  enough I/Os.
//
//-
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

        //  Treat an error in opening like an error in an asynch I/O
        //  (except for the fact that m_pfapiJetTmpLog == NULL which
        //  isn't true in the case of an asynch I/O error).

        // m_critJetTmpLog doesn't need to be held for NULL -> non-NULL
        m_errCreateAsynch = ErrLGIOpenTempLogFile(
                                &m_pfapiJetTmpLog,
                                wszPathJetTmpLog,
                                lgenDebug,
                                &m_fCreateAsynchResUsed,
                                &m_fCreateAsynchZeroFilled );

        if ( m_errCreateAsynch >= JET_errSuccess )
        {
            //  Configure trigger to cause first formatting I/O when a half
            //  chunk of log buffer has been filled. This is because we don't
            //  want to consume the log disk with our edbtmp.jtx/log I/O since
            //  it's pretty likely that someone is waiting for a log write to
            //  edb.jtx/log right now (especially since we've been spending our
            //  time finishing formatting, renaming files, etc.).
            //
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
    OSTrace( JET_tracetagInitTerm, OSFormat( "Desired LG ver (%d) - %d.%d.%d\n\n", // either: Initializing m_plgfilehdrT to (%d) ... or Upgrading m_plgfilehdrT to (%d) ...
                efv,
                pfmtversLogDesired->lgv.ulLGVersionMajor, pfmtversLogDesired->lgv.ulLGVersionUpdateMajor, pfmtversLogDesired->lgv.ulLGVersionUpdateMinor ) );

HandleError:

    return err;
}

//  ================================================================
VOID LOG_STREAM::InitLgfilehdrT()
//  ================================================================
{
    Assert( m_pLogWriteBuffer->FOwnsBufferLock() ); // used to protect static LogVersion lgvLastReported updating here.
    const LogVersion lgvNull = { 0, 0, 0 };
    static LogVersion lgvLastReported = { 0, 0, 0 };

    const LogVersion * plgvDesired = &( PfmtversEngineSafetyVersion()->lgv );
    //  probably can't fail at this point because we're at least in end of recovery.  maybe for 
    //  a clean recovery with no redo phase.  certainly for engine default case won't fail.
    const JET_ENGINEFORMATVERSION efv = (JET_ENGINEFORMATVERSION)UlParam( m_pinst, JET_paramEngineFormatVersion );
    const ERR errT = ErrLGGetDesiredLogVersion( efv, &plgvDesired );
    if ( errT < JET_errSuccess )
    {
        FireWall( OSFormat( "GetDesiredLogVerInitLgHdrVer:%lu", (ULONG)efv ) ); // handled by "SafetyVersion" above.
    }

    m_plgfilehdrT->lgfilehdr.le_ulMajor         = plgvDesired->ulLGVersionMajor;
    C_ASSERT( ulLGVersionMinorFinalDeprecatedValue == 4000 );   // deprecated
    m_plgfilehdrT->lgfilehdr.le_ulMinor         = ulLGVersionMinorFinalDeprecatedValue;
    m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor   = plgvDesired->ulLGVersionUpdateMajor;
    m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor   = plgvDesired->ulLGVersionUpdateMinor;

    m_plgfilehdrT->lgfilehdr.le_efvEngineCurrentDiagnostic = EfvMaxSupported();


    if ( CmpLgVer( LgvFromLgfilehdr( m_plgfilehdrT ), lgvLastReported ) != 0 )
    {
        OSTrace( JET_tracetagUpgrade, OSFormat( "%hs m_plgfilehdrT to (%d) - %d.%d.%d\n", // either: Initializing m_plgfilehdrT to (%d) ... or Upgrading m_plgfilehdrT to (%d) ...
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


//  ================================================================
VOID LOG_STREAM::LGSetFlagsInLgfilehdrT( const BOOL fResUsed )
//  ================================================================
{
    m_plgfilehdrT->lgfilehdr.fLGFlags = 0;
    m_plgfilehdrT->lgfilehdr.fLGFlags |= fResUsed ? fLGReservedLog : 0;
    m_plgfilehdrT->lgfilehdr.fLGFlags |= BoolParam( m_pinst, JET_paramCircularLog ) ? fLGCircularLoggingCurrent : 0;
    //  Note that we check the current log file header for circular logging currently or in the past
    m_plgfilehdrT->lgfilehdr.fLGFlags |= ( m_plgfilehdr->lgfilehdr.fLGFlags & ( fLGCircularLoggingCurrent | fLGCircularLoggingHistory ) ) ? fLGCircularLoggingHistory : 0;
}


//  ================================================================
VOID LOG_STREAM::SetSignatureInLgfilehdrT()
//  ================================================================
//
//  Set the signature in the new logfile header
//  If this is the first logfile (generation 1) this will generate a new signature
//
//-
{
    if ( m_pLog->LgenInitial() == m_plgfilehdrT->lgfilehdr.le_lGeneration )
    {
        //  set the signature for the new log sequence
        
        SIGGetSignature( &m_plgfilehdrT->lgfilehdr.signLog );

        //  first generation, set checkpoint

        //  Ensure that the checkpoint never reverts.
        Assert( m_pLog->LgposGetCheckpoint().le_lGeneration <= m_pLog->LgenInitial() );

        m_pLog->ResetCheckpoint( &m_plgfilehdrT->lgfilehdr.signLog );
        
        //  update checkpoint file before write log file header to make the
        //  attachment information in check point will be consistent with
        //  the newly generated log file.
        
        (VOID) m_pLog->ErrLGUpdateCheckpointFile( fTrue );
    }
    else
    {
        Assert( m_pLog->LgenInitial() < m_plgfilehdrT->lgfilehdr.le_lGeneration );
        m_plgfilehdrT->lgfilehdr.signLog = m_pLog->SignLog();
    }
}


//  ================================================================
VOID LOG_STREAM::CheckForGenerationOverflow()
//  ================================================================
{
    //  generate error on log generation number roll over

    const LONG  lGenCur = m_plgfilehdr->lgfilehdr.le_lGeneration;

    Assert( lGenerationMax < lGenerationMaxDuringRecovery );
    const LONG  lGenMax = ( m_pLog->FRecovering() ? lGenerationMaxDuringRecovery : lGenerationMax );
    if ( lGenCur >= lGenMax )
    {
        //  we should never hit this during recovery because we allocated extra log generations
        //  if we do hit this, it means we need to reserve MORE generations!

        Assert( !m_pLog->FRecovering() );

        //  enter "log sequence end" mode where we prevent all operations except things like rollback/term
        //  NOTE: we allow the new generation to be created so we can use it to make a clean shutdown

        //check if log_Sequence_End already set (already in the "log sequence end" mode): if it already be set, no need to set again.
        if ( !m_fLogSequenceEnd )
        {
            SetLogSequenceEnd();
        }
        else
        {
            Assert( lGenCur > lGenMax );
        }
    }

    //  if we are approaching genMax, periodically report a warning
    //
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

//  ================================================================
ERR LOG_STREAM::ErrLGIStartNewLogFile(
    const LONG              lgenToClose,
    BOOL                    fLGFlags,
    IFileAPI ** const       ppfapiTmpLog
    )
//  ================================================================
//
//  Begin the creation of a new logfile. This leaves us with
//  EDBTMP.JTX/LOG and [archived logs]
//
//-
{
    ERR             err         = JET_errSuccess;
    WCHAR           wszPathJetLog[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszPathJetTmpLog[ IFileSystemAPI::cchPathMax ];
    BOOL            fResUsed    = fFalse;
    BOOL            fZeroFilled = fFalse;
    const QWORD     cbLGFile    = QWORD( m_csecLGFile ) * m_cbSec;
    QWORD           ibPattern   = 0;

    //  all other flags are mutually exclusive, so filter out this flag
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

    // At this point, we better not have a file with a bad size
    QWORD   cbFileSize;
    Call( (*ppfapiTmpLog)->ErrSize( &cbFileSize, IFileAPI::filesizeLogical ) );
    if ( cbLGFile != cbFileSize )
    {
        FireWall( "LogFileSizeMismatch" );
        Error( ErrERRCheck( JET_errLogFileSizeMismatch ) );
    }

    //  edbtmp.jtx/log is still open

    m_pLogWriteBuffer->LockBuffer();

    if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
    {
        m_plgfilehdrT->lgfilehdr.tmPrevGen = m_plgfilehdr->lgfilehdr.tmCreate;
        m_plgfilehdrT->lgfilehdr.checksumPrevLogAllSegments = GetAccumulatedSectorChecksum();
    }
    else
    {
        //  reset file header

        memset( m_plgfilehdr, 0, sizeof( LGFILEHDR ) );
    }

    m_pLogWriteBuffer->AdvanceLgposToWriteToNewGen( lgenToClose + 1 );

    Assert( 0 == CmpLgpos( lgposMax, m_lgposCreateAsynchTrigger ) );

    //  initialize the new szJetTempLog file header
    //  write new header into edbtmp.jtx/log

    InitLgfilehdrT();

    // If we have an existing log, do not let the log version go back even if the configured desired version is lower
    if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
    {
        m_plgfilehdrT->lgfilehdr.le_ulMajor         = max( m_plgfilehdrT->lgfilehdr.le_ulMajor, m_plgfilehdr->lgfilehdr.le_ulMajor );
        m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor   = max( m_plgfilehdrT->lgfilehdr.le_ulUpdateMajor, m_plgfilehdr->lgfilehdr.le_ulUpdateMajor );
        m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor   = max( m_plgfilehdrT->lgfilehdr.le_ulUpdateMinor, m_plgfilehdr->lgfilehdr.le_ulUpdateMinor );
    }

#ifdef DEBUG
    // m_plgfilehdrT->lgfilehdr.le_lGeneration increases in the normal case.
    // But in the case of cleaning up mismatched log files, it is possible to
    // go backwards!

    if ( !m_fResettingLogStream )
    {
        Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration <= lgenToClose + 1 );
    }
#endif

    m_plgfilehdrT->lgfilehdr.le_lGeneration = lgenToClose + 1;
    LGIGetDateTime( &m_plgfilehdrT->lgfilehdr.tmCreate );
    m_pinst->SaveDBMSParams( &m_plgfilehdrT->lgfilehdr.dbms_param );

    //  initialize the log buffer

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

    //  Write the LGFILEHDR to the new file.

    Call( ErrLGIWriteFileHdr( *ppfapiTmpLog, m_plgfilehdrT ) );

    //  remember the last time that we created a new log file

    m_tickNewLogFile = TickOSTimeCurrent();
    Assert( err == JET_errSuccess );
    return err;

HandleError:
    Assert( err < JET_errSuccess );
    //  We are erroring out, so the flushes should not be necessary.
    if ( *ppfapiTmpLog )
    {
        //  Temp log, we will flush before adding log to required range
        (*ppfapiTmpLog)->SetNoFlushNeeded();
    }
    delete *ppfapiTmpLog;
    *ppfapiTmpLog = NULL;
    return err;
}


//  ================================================================
ERR LOG_STREAM::ErrLGIFinishNewLogFile( IFileAPI * const pfapiTmpLog )
//  ================================================================
//
//  When this is called we should have EDBTMP.JTX/LOG and [archived logs]
//  Rename EDBTMP.JTX/LOG to EDB.JTX/LOG
//  Start the async creation of a new temporary logfile
//  Update lGenMin/MaxRequired in database headers
//-
{
    ERR             err         = JET_errSuccess;
    WCHAR           wszPathJetLog[ IFileSystemAPI::cchPathMax ];
    WCHAR           wszPathJetTmpLog[ IFileSystemAPI::cchPathMax ];

    Assert( m_critLGWrite.FOwner() );

    // UNDONE: interestingly, we recompute these EVERYWHERE, we should just precompute these strings.
    LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eCurrentLog );
    LGMakeLogName( wszPathJetTmpLog, sizeof(wszPathJetTmpLog), eCurrentTmpLog );

    //  rename szJetTmpLog to szJetLog, i.e. edbtmp.jtx/log => edb.jtx/log

    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 );
    Assert( pfapiTmpLog->DwEngineFileType() == iofileLog ); // must have same FileType or this next function synchronizes all IO to the disk with disasterous results.
    //  Note: qwLogFileID is same, but lgen = 5 --> lgen = 6 ... so basically the edbtmp.log EngineFileId is wrong / one log behind.
    pfapiTmpLog->UpdateIFilePerfAPIEngineFileTypeId( iofileLog, QwInstFileID( qwLogFileID, m_pinst->m_iInstance, m_plgfilehdrT->lgfilehdr.le_lGeneration ) );
    Call( pfapiTmpLog->ErrRename( wszPathJetLog ) );

    LONG lWaypointDepth, lElasticWaypointDepth;
    m_pLog->LGElasticWaypointLatency( &lWaypointDepth, &lElasticWaypointDepth );
    LGPOS lgposFlushTip;
    m_pLog->LGFlushTip( &lgposFlushTip );
    // Currently flush log if flush-tip falls too far behind waypoint depth. We could also do it time based etc.
    if ( m_plgfilehdrT->lgfilehdr.le_lGeneration > lgposFlushTip.lGeneration + lWaypointDepth + lElasticWaypointDepth )
    {
        BOOL fFlushed = fFalse;
        Call( ErrLGIFlushLogFileBuffers( pfapiTmpLog, iofrLogMaxRequired, &fFlushed ) );

        if ( fFlushed )
        {
            // After the flush, the flush-tip can either point to end of last log or beginning of this log. With no LLR,
            // make it point to this log so it can be added to required range, otherwise leave it behind by configured LLR depth.
            if ( lElasticWaypointDepth == 0 )
            {
                lgposFlushTip.lGeneration = m_plgfilehdrT->lgfilehdr.le_lGeneration;
                lgposFlushTip.isec        = (WORD)m_csecHeader;
                lgposFlushTip.ib          = 0;
            }
            else
            {
                // Setting flush-tip LLR logs back with LLR matches what we do in redo-time in LOG::ErrLGRIRedoOperations
                // and it prevents this scenario, with, say LLR=l, ElasticLLR=e.
                // Current FlushTip=N, once lgenRedo reaches N+l+e+1, we would move FlushTip to N+l+e+1 and lgenMaxRequired to N+e+1.
                // On the next l log roll, we would again move lgenMaxRequired since FlushTip includes that.
                // So, in effect, we would update lgenMaxRequired l+1 times per l+e+1 logs, rather than once per e+1 logs like we meant to.
                // Keeping FlushTip LLR logs back fixes that, lgenMaxRequired would then only move every e+1 logs.
                lgposFlushTip.lGeneration = m_plgfilehdrT->lgfilehdr.le_lGeneration - lWaypointDepth;
                lgposFlushTip.isec        = lgposMax.isec;
                lgposFlushTip.ib          = lgposMax.ib;
            }
            m_pLog->LGSetFlushTip( lgposFlushTip );
        }
    }

    //  FILESYSTEM STATE: edb.jtx/log [archive files including edb00001.jtx/log]

    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration > 0 );
    Call( ErrStartAsyncLogFileCreation( wszPathJetTmpLog, m_plgfilehdrT->lgfilehdr.le_lGeneration ) );

    //  update all the database headers with the new lGenMaxRequired and lGenMaxCommitted

    Call( m_pLog->ErrLGLockCheckpointAndUpdateGenRequired(
        m_plgfilehdrT->lgfilehdr.le_lGeneration,    // log committed.
        m_plgfilehdrT->lgfilehdr.tmCreate ) );

HandleError:
    return err;
}


/*
 *  Creates and initializes a new log generation file in a crash-consistent manner:
 *  1. Acquire a new tmp log edbtmp.jtx. [ErrLGIStartNewLogFile]
 *     a) Get an already prepared edbtmp.log, that was prepared through async log file creation, OR
 *     b) In case of circular logging, rename the oldest archived log to edbtmp.jtx, OR
 *     c) Create a new one if one doesn't exist.
 *
 *  2. Rename the current log edb.jtx (if one exists) to its archived form edb000x.jtx.
 *  3. Rename the tmp log edbtmp.jtx to the current log edb.jtx. [ErrLGIFinishNewLogFile]
 *  4. Optionally, kick off async log file creation for another edbtmp.jtx for the next cycle. [ErrLGIFinishNewLogFile]

 *  Note: m_pfapiLog isn't updated to point to the new log yet. ErrLGUseNewLogFile() will close the
          old handle, and update m_pfapiLog with the new handle.
 *
 *  PARAMETERS  lgenToClose     current generation being closed
 *              fLGFlags        LOG flags to select optional behavior.
 *              ppfapiNewLog:   returns a handle to the new edb.jtx.
 *
 *  RETURNS     JET_errSuccess, or error code from failing routine
 *
 *  COMMENTS    Active log file must be completed before new log file is called.
 *              edbtmp.jtx and edb.jtx are kept opened throughout, and after the function returns.
 *              This is required to let 1 process succeed in the case of multiple processes racing to 'acquire' the same log set
 *              (on the start of recovery e.g.). In such a case, the first process to open a handle to edbtmp.jtx will succeed.
 */
ERR LOG_STREAM::ErrLGINewLogFile(
    const LONG              lgenToClose,
    BOOL                    fLGFlags,
    IFileAPI ** const       ppfapiNewLog
    )
{
    ERR                     err;
    IFileAPI*               pfapiTmpLog = NULL;
    WCHAR                   wszArchivePath[ IFileSystemAPI::cchPathMax ];   // generational name of current log file if any

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
            // Compute a generational-name for the current edb.jtx/log, before ErrLGIStartNewLogFile modifies m_plgfilehdr
            LGMakeLogName( wszArchivePath, sizeof( wszArchivePath ), eArchiveLog, m_plgfilehdr->lgfilehdr.le_lGeneration );
        }

        Call( ErrLGIStartNewLogFile( lgenToClose, fLGFlags, &pfapiTmpLog ) );

        // Notice that we do not do the rename if we're
        // restoring from backup since edb.jtx/log may not exist.
        // (the case where fLGFlags == fLGOldLogInBackup).

        if ( fLGFlags == fLGOldLogExists )
        {
            //  there was a previous szJetLog, rename it to its archive name
            //  i.e. edb.jtx/log => edb00001.jtx/log

            Assert( m_pfapiLog->DwEngineFileType() == iofileLog ); // must have same FileType or this next function synchronizes all IO to the disk with disasterous results.
            // Changes qwLogFileID -> qwLogFileMgmtID
            m_pfapiLog->UpdateIFilePerfAPIEngineFileTypeId( iofileLog, QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lgenPrev ) );
            Call( m_pfapiLog->ErrRename( wszArchivePath ) );
        }

        // we need to stop in the middle of log creation as we are during snapshot
        if ( m_pLogWriteBuffer->FStopOnNewLogGeneration() )
        {
            m_pLogWriteBuffer->LockBuffer();
            m_pLogWriteBuffer->ResetStopOnNewLogGeneration();
            m_pLogWriteBuffer->LGSetLogPaused( fTrue );
            m_pLogWriteBuffer->UnlockBuffer();

            // let snaphot Freeze to continue
            // and exit at this point w/o
            // reporting the error and not
            // allowing more LogWrites
            m_pLogWriteBuffer->LGSignalLogPaused( fTrue );
            delete pfapiTmpLog;
            return ErrERRCheck( errOSSnapshotNewLogStopped );
        }
    }
    else
    {
        // OS Snapshot backup uses FStopOnNewLogGeneration() to halt log roll in the middle. This cause the tmp log handle to be closed.
        // Then it picks up here by calling with fLGLogRenameOnly.
        // At this point, we reopen the tmp log handle and proceed normally.

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

    // Rename EDBTMP.JTX/LOG to EDB.JTX/LOG
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
        // We either flushed when we finished moving eXXtmp.log to eXX.log or we didn't because LLR is in effect in
        // which case we do not need to flush now either.
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
        err = ErrLGIUseNewLogFile( &pfapiNewLog );  // pfapiNewLog is taken over by m_pfapiLog (on success only)`
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



// There are certain possible situations where we just finished reading a
// finished log file (never to be written to again) and it is either
// named edb.jtx/log or edbXXXXX.jtx/log (generational or archive name). We want to
// make ensure that it gets renamed to an archive name if necessary, and then
// a new log file is created as edb.jtx/log that we switch to.

ERR LOG_STREAM::ErrLGSwitchToNewLogFile( LONG lgenToSwitchTo, BOOL fLGFlags )
{
    // special case where the current log file has ended, so we need
    // to create a new log file and write to that instead.
    // Only need to do the switch when we notice that the current log generation
    // is different from the write point generation
    Assert( pNil != m_plgfilehdr );

    if ( m_plgfilehdr->lgfilehdr.le_lGeneration != lgenToSwitchTo )
    {
        //  If this goes off, then we have a bigger problem in ErrLGRRedo() b/c
        //  we could be running without undo, and if so, we should not be rolling
        //  a log.
        Assert( 0 == ( fLGAssertShouldntNeedRoll & fLGFlags ) );

        // If this fires, we probably picked up some other horrible special case or
        // impossible condition.
        Assert( lgenToSwitchTo == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );

        // There must be a log file open from just calling ErrLGCheckReadLastLogRecord()
        Assert( m_pfapiLog );

        ERR         err         = JET_errSuccess;
        WCHAR       wszPathJetLog[IFileSystemAPI::cchPathMax];

        // edb.jtx/log
        LGMakeLogName( wszPathJetLog, sizeof(wszPathJetLog), eCurrentLog );

        m_critLGWrite.Enter();

        err = ErrUtilPathExists( m_pinst->m_pfsapi, wszPathJetLog );
        if ( err < 0 )
        {
            if ( JET_errFileNotFound == err )
            {
                // edb.jtx/log does not exist, so don't try to rename a nonexistant
                // file from edb.jtx/log to generational name
                // Some kind of log file is open, so it must be a generational name
                fLGFlags |= fLGOldLogInBackup;
                // close log file because ErrLGNewLogFile( fLGOldLogInBackup ) does not
                // expect an open log file.
                delete m_pfapiLog;
                m_pfapiLog = NULL;
            }
            else
            {
                // other unexpected error
                Call( err );
            }
        }
        else
        {
            // edb.jtx/log does exist, so we want it renamed to generational name
            fLGFlags |= fLGOldLogExists;
            // generational archive named log file is still open, but ErrLGNewLogFile
            // will close it and rename it
        }

        Assert( lgenToSwitchTo == m_plgfilehdr->lgfilehdr.le_lGeneration + 1 );

        Call( ErrLGNewLogFile( m_plgfilehdr->lgfilehdr.le_lGeneration, fLGFlags ) );

HandleError:
        m_critLGWrite.Leave();

        return err;
    }
    return JET_errSuccess;
}


// Writes the provided sector data and responds to success or error

ERR LOG_STREAM::ErrLGIWriteSectorData(
    IFileAPI * const                                pfapi,
    __in_ecount( cLogData ) const IOREASON          rgIor[],
    const LONG                                      lgenData,       // log gen
    const DWORD                                     ibLogData,      // offset
    __in_ecount( cLogData ) const ULONG             rgcbLogData[],  // size range
    __in_ecount( cLogData ) const BYTE * const      rgpbLogData[],  // buffer range
    const size_t                                    cLogData,       // range length
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
        // After log end has been emitted, only possible log write is for header for new log,
        // do not let it through as it creates bogus divergence.
        AssertTrack( ibLogData == 0 && ibEnd == sizeof(LGFILEHDR), "LogWriteAfterLogEndEmitted" );
        return ErrERRCheck( JET_errLogWriteFail );
    }

    // Initialize here to protect against uninitialized value return.
    // Also protect against multiple WriteSector calls when shadow sectors for partial sectors are involved.
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

    // First ship the data to any passives
    DWORD ibOffset = ibLogData;
    for ( size_t i = 0; i < cLogData; ++i )
    {
        //  emit log data for any 3rd parties or shadow log that is interested
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
        // Use multiple asyncIO/WriteGather when there are 2 or more I/O requests.
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
                                    QosSyncDefault( m_pinst ) | qosIOOptimizeOverrideMaxIOLimits );

        for ( size_t i = 0; i < cLogData; i++ )
        {
            rgtc[i].~TraceContext();
        }
    }
    else
    {
        // only 1 I/O, call ErrIOWrite directly.
        Assert( cLogData == 1 );
        TraceContextScope tcScope;
        tcScope->iorReason = rgIor[ 0 ];
        err = pfapi->ErrIOWrite( *tcScope, ibLogData, rgcbLogData[0], rgpbLogData[0], QosSyncDefault( m_pinst ) );
    }

    if ( err >= JET_errSuccess )
    {
        // update performance counters
        
        PERFOpt( cLGWrite.Inc( m_pinst ) );
        PERFOpt( cbLGWritten.Add( m_pinst, cbLogDataTrace ) );

        IOFLUSHREASON iofr = iofrInvalid;
        switch( rgIor[0].Iorp() )
        {
            case iorpLog:               iofr = iofrInvalid; // We write header to temp log, we will flush before adding log to required range
                // This one special, only for offset = 0 is it write log header, so check that
                Assert( rgIor[0].Iors() == iorsHeader );
                Assert( pfapi != m_pfapiLog );
                Assert( ibLogData == 0 );
                break;
            case iorpLGWriteCommit:     iofr = iofrLogCommitFlush;      break;
            case iorpLGWriteCapacity:   iofr = iofrLogCapacityFlush;    break;
            case iorpLGWriteSignal:     iofr = iofrLogSignalFlush;      break;  // "at least some times this is capacity flush ..." - from tcconst.hxx ...
            case iorpLGFlushAll:        iofr = iofrLogFlushAll;         break;  // Various - ErrLGTerm() / ErrLGWaitAllFlushed / ErrLGFlushLog ...
            case iorpLGWriteNewGen:     iofr = iofrUtility;             break;  //  snapshot / backup operation
            case iorpPatchFix:          iofr = iofrLogPatch;            break;  //  from shadow header patching
            default:
                AssertSz( fFalse, "Saw another kind of flush: %d", rgIor[0].Iorp() );
        }

        err = ErrLGIFlushLogFileBuffers( pfapi, iofr, pfFlushed );
    }

    if ( err < JET_errSuccess )     // handle the write OR flush error
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
        return iorpLog; // guess
    }

    switch( plog->FRecoveringMode() ) {

        case fRecoveringRedo:
        case fRecoveringUndo:
        case fRestoreRedo:
            return iorpLogRecRedo;

        case fRecoveringNone:
            //  Since this happens in things like eseutil /ml (dump trx log), so this is 
            //  iorpDirectAccessUtil.  Would have asserted gGlobalEseutil but things like the inc
            //  reseed test call JetDBUtilities() as well and trigger this.
            return iorpDirectAccessUtil;
            break;
            
        case fRestorePatch:
        default:
            AssertSz( fFalse, "Shouldn't happen." );
            break;
    }
    return iorpLog; // guess
}

ERR LOG_STREAM::ErrLGReadSectorData(
    const QWORD             ibLogData,  // offset
    const ULONG             cbLogData,  // size
    __out_bcount( cbLogData ) BYTE *    pbLogData ) // buffer
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

VOID LGMakeName( IFileSystemAPI *const pfsapi, __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR wszName, _In_ PCWSTR wszPath, _In_ PCWSTR wszFName, _In_ PCWSTR wszExt )
{
    WCHAR   wszDirT[IFileSystemAPI::cchPathMax];
    WCHAR   wszFNameT[IFileSystemAPI::cchPathMax];
    WCHAR   wszExtT[IFileSystemAPI::cchPathMax];

    CallS( pfsapi->ErrPathParse( wszPath, wszDirT, wszFNameT, wszExtT ) );
    CallS( pfsapi->ErrPathBuild( wszDirT, wszFName, wszExt, wszName ) );
}

VOID LOG_STREAM::LGFullLogNameFromLogId( __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName, LONG lGeneration, _In_ PCWSTR wszDirectory )
{
    WCHAR   wszFullPathT[IFileSystemAPI::cchPathMax];

    //  we should always be able to get the full path as it should be the restore path
    //  which is already checked in the calling context.
    //
    CallS( m_pinst->m_pfsapi->ErrPathComplete( wszDirectory, wszFullPathT ) );
    CallS( ErrLGMakeLogNameBaseless( wszFullLogFileName, OSFSAPI_MAX_PATH * sizeof( WCHAR ), wszFullPathT, eArchiveLog, lGeneration ) );

    return;
}

ERR LOG_STREAM::ErrLGRSTOpenLogFile(
    _In_ PCWSTR         wszLogFolder,
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


VOID LOG_STREAM::LGRSTDeleteLogs( _In_ PCWSTR wszLogFolder, INT genLow, INT genHigh, BOOL fIncludeJetLog )
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

// UNDONE: We're calling this WAY too often.  It looks like we're enumerating existing logs every single time we
// roll over a log generation if we have circular logging on?  Anyway, could be improved, by tracking the high, and
// low gen in LOG.

// returns JET_errSuccess even if not found (then lgen will be 0)
//
ERR LOG_STREAM::ErrLGGetGenerationRange( _In_ PCWSTR wszFindPath, LONG* plgenLow, LONG* plgenHigh, _In_ BOOL fLooseExt, __out_opt BOOL * pfDefaultExt )
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

// Try not to use this one ...
ERR LOG_STREAM::ErrLGGetGenerationRangeExt( _In_ PCWSTR wszFindPath, LONG* plgenLow, LONG* plgenHigh, _In_ PCWSTR wszLogExt  )
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

    //  make search string "<search path> \ edb * [.log|.jtx]\0"
    //
    Call( ErrLGMakeLogNameBaseless( wszFind, sizeof(wszFind), wszFindPath, eArchiveLog, 0, wszLogExt ) );

    Call( m_pinst->m_pfsapi->ErrFileFind( wszFind, &pffapi ) );
    while ( ( err = pffapi->ErrNext() ) == JET_errSuccess )
    {
        LONG        lGen = 0;
        // get path
        Call( pffapi->ErrPath( wszFileName ) );
        // get generation from path
        err = LGFileHelper::ErrLGGetGeneration( m_pinst->m_pfsapi, wszFileName, SzParam( m_pinst, JET_paramBaseName ), &lGen, wszLogExt, &cLogDigits );
        if ( err != JET_errInvalidParameter && err != JET_errSuccess )
        {   // not a typical error, lets bail.
            Call( ErrERRCheck( err ) );
        }
        if ( err == JET_errSuccess )
        {
            if ( !(UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) && 5 == cLogDigits )
            {
                // We don't accept 5 digit logs, if soft compat isn't set.
                Call( ErrERRCheck( JET_errLogGenerationMismatch ) );
            }

            Assert(lGen != 0);
            lGenMax = max( lGenMax, lGen );
            lGenMin = min( lGenMin, lGen );
        }
        // this function is robust to trx log files that don't parse, we just ignore them.
        err = JET_errSuccess;
    }
    Call( err == JET_errFileNotFound ? JET_errSuccess : err );

HandleError:

    delete pffapi;

    // JET_errFileNotFound is not an error, we return JET_errSuccess and (0,0) as range

    // on error, we return (0,0)
    if ( err < JET_errSuccess )
    {
        Assert ( JET_errFileNotFound != err );
        lGenMin = 0;
        lGenMax = 0;
    }

    // nothing found
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

ERR LOG_STREAM::ErrLGRemoveCommittedLogs( _In_ LONG lGenToDeleteFrom )
{
    ERR     err = JET_errSuccess;
    ERR     errT = JET_errSuccess;
    LONG    lGenFirst = 0;
    LONG    lGenLast = 0;
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];

    CallR( ErrLGGetGenerationRange( m_pLog->SzLGCurrentFolder(), &lGenFirst, &lGenLast ) );

    if ( lGenFirst == 0 || lGenLast == 0 )
    {
        //  There may only be the current log file.
        Assert( lGenFirst == 0 && lGenLast == 0 );
        return JET_errSuccess;  // other code takes care of current log file if w/in lgen range.
    }

    // UNDONE: already checked by caller immediately before calling this function
    // Note we are removing committed data, make very very sure we are removing
    // only logs we can do without.
    // Assert( m_pLog->m_fIgnoreLostLogs );
    // Assert( lGenFirst < lGenToDeleteFrom );
    // Assert( m_pLog->m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration < lGenToDeleteFrom );
    //  If there are more than committed log files missing, quit.
    // Assert( m_pLog->ErrLGCheckDBGensRequired( lGenToDeleteFrom - 1 ) == JET_errSuccess ||
    //      m_pLog->ErrLGCheckDBGensRequired( lGenToDeleteFrom - 1 ) == JET_errCommittedLogFilesMissing );
    // if ( ( !m_pLog->m_fIgnoreLostLogs ) ||
    //  ( lGenFirst >= lGenToDeleteFrom ) ||
    //  ( m_pLog->m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= lGenToDeleteFrom ) ||
    //  ( m_pLog->ErrLGCheckDBGensRequired( lGenToDeleteFrom - 1 ) != JET_errSuccess &&
    //      m_pLog->ErrLGCheckDBGensRequired( lGenToDeleteFrom - 1 ) != JET_errCommittedLogFilesMissing ) )
    //  {
    //  AssertSz( fFalse, " This would be an unexpected condition here." );
    //  return ErrERRCheck( JET_errInvalidParameter );  // punt
    //  }

    //  delete edbxxxxxxx.jtx/log
    //  start deleting only if we found something (lGen > 0)

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
    _In_ const LONG lgenToCreate
    )
{
    ERR err = JET_errSuccess;
    IFileAPI* pfapiNewLog = NULL;

    /* we run out of log files, create a new edb.jtx/log in current
     * directory for later use.
     */
    m_critLGWrite.Enter();
    Call( ErrLGINewLogFile( lgenToCreate - 1, fLGOldLogInBackup, &pfapiNewLog ) );

    Assert( m_plgfilehdrT->lgfilehdr.le_lGeneration == lgenToCreate );
    Call( ErrLGIUseNewLogFile( &pfapiNewLog ) );    // pfapiNewLog taken over by m_pfapiLog (on success only)

    // Update m_wszLogName because recovery relies on it to open the expected log file
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

//  This deletes the current log and sets up LOG to create a new log file at
//  at the expected generation.
ERR LOG_STREAM::ErrLGRRestartLOGAtLogGen(
    _In_ const LONG lgenToStartAt,
    _In_ const BOOL fLogEvent
    )
{
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];
    ERR err = JET_errSuccess;

    //  close current logfile, open next generation
    //
    delete m_pfapiLog;

    //  set m_pfapiLog as NULL to indicate it is closed
    //
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

    // we need to copy back the previous log log header
    // so that the new edb.jtx/log will have the right prevInfo
    //
    m_pLogWriteBuffer->LockBuffer();
    UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof(LGFILEHDR) );
    m_pLogWriteBuffer->UnlockBuffer();

    return ErrLGRStartNewLog( lgenToStartAt );
}

BOOL LOG_STREAM::FLGRedoOnCurrentLog() const
{
    WCHAR    wszT[IFileSystemAPI::cchPathMax];
    WCHAR    wszFNameT[IFileSystemAPI::cchPathMax];

    /*  split log name into name components
    /**/
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


//  cleanup the current logs/checkpoint and start a new sequence
//
//  if we succeed, we will return JET_errSuccess, and the user will be at gen 1 (of if fExtCleanup we will
//      be at m_lgenInitial.
//  if we fail, the user will be forced to delete the remaining logs/checkpoint by hand

ERR LOG_STREAM::ErrLGRICleanupMismatchedLogFiles( BOOL fExtCleanup )
{
    ERR     err = JET_errSuccess;
    ERR     errT;
    LONG    lGen;
    LONG    lGenLast;
    WCHAR   wszPath[IFileSystemAPI::cchPathMax];

    //  circular logging must be on so that the user knows hard recovery is definitely impossible

    Assert( BoolParam( m_pinst, JET_paramCircularLog ) );

    //  close the current log file (this will prevent any log writing)

    m_critLGWrite.Enter();
    delete m_pfapiLog;
    m_pfapiLog = NULL;

    LGCreateAsynchCancel( fFalse );

    m_critLGWrite.Leave();

    //  find the first and last generations

    CallR( ErrLGGetGenerationRange( m_pLog->SzLGCurrentFolder(), &lGen, &lGenLast ) );

    //  delete edbxxxx.jtx/log
    //  start deleting only if we found something (lGen > 0)

    for ( ; lGen > 0 && lGen <= lGenLast; lGen++ )
    {
        LGMakeLogName( wszPath, sizeof(wszPath), eArchiveLog, lGen );
        errT = m_pinst->m_pfsapi->ErrFileDelete( wszPath );
        if ( errT < JET_errSuccess && JET_errFileNotFound != errT && err >= JET_errSuccess )
        {
            err = errT;
        }
    }

    //  delete edb.jtx/log and edbtmp.jtx/log

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

    //  delete the checkpoint file(s)

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

    //  this is the end of the file-cleanup -- handle the resulting error, if any

    CallR( err );

    //  create the new log (gen 1) using the new log-file size

    m_csecLGFile = CsecLGFromSize( (LONG)UlParam( m_pinst, JET_paramLogFileSize ) );

    //  create a new log and open it (using the new size in lLogFileSize)

    m_critLGWrite.Enter();

    if ( fExtCleanup )
    {
        //
        // Switch over to new (or really default) extensions at this point!
        //
        m_wszLogExt = m_pLog->WszLGGetDefaultExt( fFalse );
        m_pLog->LGSetChkExts( m_pLog->FLGIsLegacyExt( fTrue, m_pLog->WszLGGetDefaultExt( fTrue ) ), fTrue );
        m_pLog->SetLgenInitial( lGenLast+1 ); // This is a cunning way to init the log generation 1 after the last log gen.
        // Should we always set m_lgenInitial?  I think only if we make recovery handle varying log sizes as it recovers.
    }

    // Set the checkpoint log generation to a reasonable number. It might
    // be going backwards because newer log generations could have been
    // deleted.
    m_pLog->ResetCheckpoint();
    m_pLog->ResetLgenLogtimeMapping();

#ifdef DEBUG
    //  Allow the log file stream to go backwards.
    m_fResettingLogStream = fTrue;
#endif

    m_pLog->ResetRecoveryState();

    IFileAPI* pfapiNewLog = NULL;
    err = ErrLGINewLogFile( (m_pLog->LgenInitial()-1), fLGOldLogNotExists, &pfapiNewLog );
    delete pfapiNewLog;
    pfapiNewLog = NULL;

#ifdef DEBUG
    //  The log file stream should be consistent at this point.
    m_fResettingLogStream = fFalse;
#endif

    if ( err >= JET_errSuccess )
    {

        //  we successfully created a new log

        //  reset fSignLogSetGlobal because we have a new log signature now (setup by ErrLGNewLogFile)

        m_pLog->ErrLGSetSignLog( NULL, fFalse );

        m_pLogWriteBuffer->LockBuffer();
        memcpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
        m_pLogWriteBuffer->ResetWritePosition();
        m_pLog->LGResetFlushTipWithLock( lgposMin );
        m_pLogWriteBuffer->UnlockBuffer();

        Assert( m_pLog->LgenInitial() == m_plgfilehdr->lgfilehdr.le_lGeneration );
        //  open the new log file

        LGMakeLogName( eCurrentLog );
        err = CIOFilePerf::ErrFileOpen(
                                m_pinst->m_pfsapi,
                                m_pinst,
                                m_wszLogName,
                                FmfLGStreamDefault(),
                                iofileLog,
                                QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lGenSignalCurrentID /* hard to know current lgen */ ),
                                &m_pfapiLog );
        if ( err >= JET_errSuccess )
        {

            //  read the log file hdr and initialize the log params (including the new log signature)

            err = ErrLGReadFileHdr( NULL, iorpLog, m_plgfilehdr, fFalse );
#ifdef DEBUG
            if ( err >= JET_errSuccess )
            {
                m_pLogWriteBuffer->ResetLgposLastLogRec();  //  reset the log position of the last record
            }
#endif  //  DEBUG
        }

    }
    else
    {

        //  failed to create the new log

        //  this is ok because we have consistent databases and no logs/checkpoint
        //  soft recovery will run and create the first log generation

        //  nop
    }
    m_critLGWrite.Leave();

    //  return a success indicating that the old logs/checkpoint have been cleaned up

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

    //  get range of log files in directory by file enumeration
    //
    CallR( ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenMin, &lgenMax ) );

    if ( lgenMin == 0 )
    {
        Assert ( 0 == lgenMax );
        return JET_errSuccess;
    }

    //  if all we have is below current generation then we are done
    //
    if ( lgenMax < lCurrentGeneration )
    {
        return JET_errSuccess;
    }

    //  start from the current generation number, because the current generation is edb.jtx/log!
    //
    lgenMin = max( lgenMin, lCurrentGeneration );
    lgenMax = max( lgenMax, lCurrentGeneration );

    //  we need a buffer to read the log headers
    //  so that we can check that they do not match the current signature.
    //
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

        //  read and check the log signature
        //
        err = ErrLGReadFileHdr( pfapiT, iorpLog, plgfilehdr, fCheckLogID );
        delete pfapiT;
        pfapiT = NULL;
        Call ( err );

        //  file present with current signature,
        //  checked via ReadFileHdr with fCheckLogID set,
        //  but above the soft-recovery range so delete it.
        //
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

    //  restore the volume sector size for creating the new log file
    
    LGResetSectorGeometry();

    //  NOTE: no need to force the log buffer size here -- we won't be recovering, and
    //        we won't fill up the current buffer (filling up the buffer is a problem
    //        because there could be more than 2 active logs in it if it were too large)
    
    m_critLGWrite.Enter();
    
    IFileAPI* pfapiNewLog = NULL;
    Call( ErrLGINewLogFile( m_pLog->LgenInitial()-1, fLGOldLogNotExists, &pfapiNewLog ) ); // generation 0 + 1

    Call( ErrLGIFlushLogFileBuffers( pfapiNewLog, iofrPersistClosingLogFile ) );

    delete pfapiNewLog;
    pfapiNewLog = NULL;

    m_pLogWriteBuffer->LockBuffer();
    UtilMemCpy( m_plgfilehdr, m_plgfilehdrT, sizeof( LGFILEHDR ) );
    m_pLogWriteBuffer->ResetWritePosition();
    m_pLogWriteBuffer->UnlockBuffer();
    
    //  set flag for initialization
    //
    *pfJetLogGeneratedDuringSoftStart = fTrue;
    
    Assert( m_plgfilehdr->lgfilehdr.le_lGeneration == m_pLog->LgenInitial() );
    
    // XXX
    // Must fix these Assert()'s.
    //Assert( m_pbLastMSFlush == m_pbLGBufMin );
    //Assert( m_lgposLastMSFlush.lGeneration == 1 );

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

    //  delete all the logs
    //

    LONG lGenLow, lGenHigh, lGen;
    WCHAR wszPath[IFileSystemAPI::cchPathMax];
    
    Call( ErrLGGetGenerationRange( m_pLog->SzLGCurrentFolder(), &lGenLow, &lGenHigh ) );
    
    //  delete <inst>xxxx.jtx/log archive logs
    //
    
    //  start deleting only if we found something (lGenLow > 0)
    //
    for ( lGen = lGenLow; lGen > 0 && lGen <= lGenHigh; lGen++ )
    {
        LGMakeLogName( wszPath, sizeof(wszPath), eArchiveLog, lGen );
        Call( m_pinst->m_pfsapi->ErrFileDelete( wszPath ) );
    }
    
    //  delete <inst>.jtx/log, <inst>tmp.jtx/log
    //
    
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
    //  report the deletion range and then perform the file deletions

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

    //  ignore errors (lgenMic will simply come back as 0)
    //
    //  UNDONE: what caused the failure???
    //
    (void)ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lgenMic, NULL );

    if ( lgenMic > 0 )
    {
        LGIReportLogTruncation( lgenMic, lgenMac, fFalse );

        for ( LONG lgen = lgenMic; lgen < lgenMac; lgen++ )
        {
            LGMakeLogName( wszLogName, sizeof(wszLogName), eArchiveLog, lgen );

            //  UNDONE: what caused the deletion failure???
            //
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

VOID DBGBRTrace( _In_ PCSTR sz );

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
        //  nothing to delete

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

        /*  delete logs.  Note that log files must be deleted in
        /*  increasing number order.
        /**/

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
                // if the file we want to delete is missing already
                // just ignore the error. We will report it at the end as a warning
                if ( JET_errFileNotFound == err )
                {
                    cFilesNotFound++;
                    err = JET_errSuccess;
                    continue;
                }

                /*  must maintain a continuous log file sequence,
                /*  No need to clean up (reset m_fBackupInProgress* etc) if fails.
                /**/
                break;
            }
            cDeleted++;
        }
    }

    // report a warning if files were missing
    //
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
    // will return error on "edb.jtx"
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
    //  first lets make sure this version of the log file header is in fact what is written
    //  to m_pfapiLog.
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
            //  If we successfully got it, check it's the same ...
            Enforce( 0 == memcmp( plgfilehdrT, m_plgfilehdr, sizeof(LGFILEHDR) ) );
        }
        //  we don't care about errors b/c it's best effort, if the log hdr is corrupt or something
        //  this I don't think was the path that was supposed to deal with that ... let whatever
        //  happens happen.

        OSMemoryPageFree( plgfilehdrT );
    }

    //  then lets make sure the attach info is nulled out like we expect.
    Enforce( m_plgfilehdr->rgbAttach[0] == 0 );
}

// Flushes suppressed when LLR is enabled
// FFB for capacity and signal flush will probably go away once we focus on windows FFB performance and
// fix recovery of logs with torn write.
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
        // This assert doesn't hold in a rare code path from recovery and shadow sector patching ...
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
        // hopefully by not reporting success this won't advance anything and will just flush again next time.
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

    // During redo or snapshot backup, we do not have a log file open at all times.
    Assert( m_pfapiLog || ( m_pLog->FRecovering() && m_pLog->FRecoveringMode() == fRecoveringRedo ) || m_pLog->FLGLogPaused() );

    if ( m_pfapiLog )
    {
        err = ErrLGIFlushLogFileBuffers( m_pfapiLog, iofr, pfFlushed );
    }
    // If we are just in the middle of recovery, ignore the call, i.e. will flush later, otherwise flush the volume
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
                                    QwInstFileID( qwLogFileMgmtID, m_pinst->m_iInstance, lGenSignalCurrentID /* hard to know current lgen */ ),
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
    //  Note: ignoring a NULL pfapi is also satisfying the intention of the assert/function,
    //  because we are protected from deleting a log until all it's non-flushed IOs have 
    //  been flushed.
    Assert( m_pfapiLog == NULL || m_pfapiLog->CioNonFlushed() == 0 );

    UnlockWrite();
#endif
}

