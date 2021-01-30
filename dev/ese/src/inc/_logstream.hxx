// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

enum LS
{
    lsNormal,
    lsQuiesce,
    lsOutOfDiskSpace
};

PERSISTED
struct LGSEGHDR
{
    LittleEndian<XECHECKSUM>    checksum;
    LE_LGPOS                    le_lgposSegment;
    LOGTIME                     logtimeSegmentStart;
    BYTE                        rgbReserved[12];
    LittleEndian<ULONG>         fFlags;
};

C_ASSERT( offsetof( LGSEGHDR, checksum ) == offsetof( CPAGE::PGHDR, checksum ) );
C_ASSERT( offsetof( LGSEGHDR, fFlags ) == offsetof( CPAGE::PGHDR, fFlags ) );

#define LOG_SEGMENT_SIZE        4096
#define LOG_MIN_FRAGMENT_SIZE   32

#define FIsOldLrckLogFormat( ulMajor )      ( ulMajor <= ulLGVersionMajor_OldLrckFormat )

class LOG_WRITE_BUFFER;

class ILogStream
{
public:
    virtual ~ILogStream() {}

    virtual ERR ErrLGInit() = 0;
    virtual VOID LGTerm() = 0;

    virtual INT CsecLGFromSize( LONG lLogFileSize ) const = 0;

    virtual IFileAPI::FileModeFlags FmfLGStreamDefault( const IFileAPI::FileModeFlags fmfAddl = IFileAPI::fmfRegular ) const = 0;

    virtual QWORD CbOffsetLgpos( LGPOS lgpos1, LGPOS lgpos2 ) const = 0;
    virtual QWORD CbOffsetLgpos( LGPOS lgpos1, LE_LGPOS lgpos2 ) const = 0;

    virtual VOID AddLgpos( LGPOS * const plgpos, UINT cb ) const = 0;

    virtual QWORD CbOffsetLgposForOB0( LGPOS lgpos1, LGPOS lgpos2 ) const = 0;
    virtual LGPOS LgposFromIbForOB0( QWORD ib ) const = 0;
    virtual VOID SetLgposUndoForOB0( LGPOS lgpos ) = 0;

    virtual BOOL FIsOldLrckVersionSeen() const = 0;
    virtual VOID ResetOldLrckVersionSecSize() = 0;

    virtual LONG GetCurrentFileGen( LOGTIME * plogtime = NULL ) const = 0;
    virtual const LGFILEHDR *GetCurrentFileHdr() const = 0;
    virtual VOID SaveCurrentFileHdr() = 0;
    virtual ERR ErrLGGetDesiredLogVersion( _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const LogVersion ** const pplgv ) = 0;
    virtual BOOL FLGFileVersionUpdateNeeded( _In_ const LogVersion& lgvDesired ) = 0;
    virtual ERR ErrLGInitSetInstanceWiseParameters() = 0;
    virtual ERR ErrLGInitTmpLogBuffers() = 0;
    virtual VOID LGTermTmpLogBuffers() = 0;

    virtual ERR ErrLGNewLogFile( LONG lgen, BOOL fLGFlags ) = 0;

    virtual ERR ErrStartAsyncLogFileCreation( _In_ const WCHAR * const wszPathJetTmpLog, _In_ LONG lgenDebug ) = 0;
    virtual BOOL FShouldCreateTmpLog( const LGPOS &lgposLogRec ) = 0;
    virtual VOID LGCreateAsynchIOIssue( const LGPOS& lgposLogRec ) = 0;
    virtual VOID LGCreateAsynchCancel( const BOOL fWaitForPending ) = 0;

    virtual ERR ErrLGCreateNewLogStream( BOOL * const pfJetLogGeneratedDuringSoftStart ) = 0;

    virtual VOID LGFullLogNameFromLogId( __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName, LONG lGeneration, __in PCWSTR wszDirectory ) = 0;

    virtual VOID LGMakeLogName( const enum eLGFileNameSpec eLogType, LONG lGen = 0, BOOL fUseDefaultFolder = fFalse ) = 0;
    virtual VOID LGMakeLogName( __out_bcount(cbLogName) PWSTR wszLogName, ULONG cbLogName, const enum eLGFileNameSpec eLogType, LONG lGen = 0, BOOL fUseDefaultFolder = fFalse ) const = 0;

    virtual PCWSTR LogName() const = 0;
    virtual void LGSetLogExt( BOOL fLegacy, BOOL fReset ) = 0;
    virtual PCWSTR LogExt() const = 0;

    virtual BOOL GetLogSequenceEnd() = 0;
    virtual VOID SetLogSequenceEnd() = 0;

    virtual ULONG CbSec() const = 0;
    virtual ULONG Log2CbSec() const = 0;
    virtual ULONG CbLGLogFileSize() const = 0;
    virtual UINT CSecLGFile() const = 0;
    virtual VOID SetCSecLGFile( ULONG csecLGFile ) = 0;
    virtual VOID UpdateCSecLGFile() = 0;
    virtual ULONG CbSecVolume() const = 0;
    virtual VOID SetCbSecVolume( ULONG cbSecVolume ) = 0;
    virtual ULONG CSecHeader() const = 0;

    virtual BYTE *PbSecAligned( BYTE *pb, BYTE * pbLGBufMin ) const = 0;
    virtual const BYTE *PbSecAligned( const BYTE * pb, BYTE * pbLGBufMin ) const = 0;

    virtual VOID LGSetSectorGeometry(
            const ULONG cbSecSize,
            const ULONG csecLGFile ) = 0;
    virtual VOID LGResetSectorGeometry(
            LONG lLogFileSize = 0 ) = 0;

    virtual BOOL FRolloverInDuration( TICK dtickThreshold ) = 0;

    virtual VOID LockWrite() = 0;
    virtual VOID UnlockWrite() = 0;
    virtual VOID LGAssertWriteOwner() = 0;
    virtual BOOL FLGProbablyWriting() = 0;

    virtual BOOL FRemovedLogs() = 0;
    virtual VOID ResetRemovedLogs() = 0;

    virtual ERR ErrLGOpenFile( PCWSTR pszPath = NULL, BOOL fReadOnly = fFalse ) = 0;
    virtual VOID LGCloseFile() = 0;
    virtual ERR SetPfapi( IFileAPI * pfapi ) = 0;
    virtual VOID ResetPfapi() = 0;
    virtual BOOL FLGFileOpened() const = 0;
    virtual ERR ErrFileSize( QWORD *pqwSize ) = 0;
    
    virtual ERR ErrLGWriteSectorData(
        __in_ecount( cLogData ) const IOREASON      rgIor[],
        const LONG                                  lgenData,
        const DWORD                                 ibLogData,
        __in_ecount( cLogData ) const ULONG         rgcbLogData[],
        __in_ecount( cLogData ) const BYTE * const  rgpbLogData[],
        const size_t                                cLogData,
        const DWORD                                 dwLogIdErr,
        BOOL * const                                pfFlushed = NULL,
        const BOOL                                  fGeneralizeError = fFalse,
        const LGPOS&                                lgposWriteEnd = lgposMin
        ) = 0;
    virtual ERR ErrLGWriteSectorData(
        const IOREASON          ior,
        const LONG              lgenData,
        const DWORD             ibLogData,
        const ULONG             cbLogData,
        const BYTE * const      pbLogData,
        const DWORD             dwLogIdErr,
        BOOL * const            pfFlushed = NULL,
        const BOOL              fGeneralizeError = fFalse,
        const LGPOS&            lgposWriteEnd = lgposMin
        ) = 0;
    virtual ERR ErrLGFlushLogFileBuffers( const IOFLUSHREASON iofr, BOOL * const pfFlushed = NULL ) = 0;
    virtual void LGAssertFullyFlushedBuffers() = 0;
    virtual ERR ErrLGTermFlushLog( const BOOL fCleanTerm ) = 0;
    virtual ERR ErrLGReadSectorData(
        const QWORD             ibLogData,
        const ULONG             cbLogData,
        __out_bcount( cbLogData ) BYTE *    pbLogData
        ) = 0;

    virtual ERR ErrLGRStartNewLog(
            __in const LONG lgenToCreate ) = 0;
    virtual ERR ErrLGRRestartLOGAtLogGen(
            __in const LONG lgenToStartAt,
            __in const BOOL fLogEvent ) = 0;
    virtual ERR ErrLGDeleteOutOfRangeLogFiles() = 0;

    virtual ERR ErrLGTryOpenJetLog(
            const ULONG eOpenReason,
            const LONG lGenNext,
            BOOL fPickLogExt = fFalse ) = 0;
    virtual ERR ErrLGOpenJetLog(
            const ULONG eOpenReason,
            const ULONG lGenNext,
            BOOL fPickLogExt = fFalse ) = 0;

    virtual ERR ErrLGDeleteLegacyReserveLogFiles() = 0;

    virtual ERR ErrLGInitCreateReserveLogFiles() = 0;

    virtual ERR ErrLGCreateEmptyShadowLogFile(
            const WCHAR * const     wszPath,
            const LONG              lgenShadowDebug ) = 0;

    virtual ERR ErrLGSwitchToNewLogFile(
            LONG lgenSwitchTo,
            BOOL fLGFlags ) = 0;

    virtual ERR ErrLGReadFileHdr(
        IFileAPI *          pfapiLog,
        IOREASONPRIMARY     iorp,
        LGFILEHDR *         plgfilehdr,
        const BOOL          fNeedToCheckLogID,
        const BOOL          fBypassDbPageAndLogSectorSizeChecks = fFalse ) = 0;
    virtual VOID LGVerifyFileHeaderOnInit() = 0;
    virtual ERR ErrLGFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature ) = 0;

    virtual BOOL FLGRedoOnCurrentLog() const = 0;
    virtual BOOL FCurrentLogExists() const = 0;
    virtual BOOL FTempLogExists() const = 0;

    virtual ERR ErrLGTryOpenLogFile(
            const ULONG eOpenReason,
            const enum eLGFileNameSpec eArchiveLog,
            const LONG lgen ) = 0;
    virtual ERR ErrLGOpenLogFile(
            const ULONG eOpenReason,
            const enum eLGFileNameSpec eLogType,
            const LONG lgen,
            const ERR errMissingLog ) = 0;
    virtual ERR ErrLGRIOpenRedoLogFile(
            LGPOS *plgposRedoFrom,
            INT *pfStatus ) = 0;

    virtual VOID LGTruncateLogsAfterRecovery() = 0;

    virtual ERR ErrLGTruncateLog(
            const LONG lgenMic,
            const LONG lgenMac,
            const BOOL fSnapshot,
            const BOOL fFullBackup ) = 0;

    virtual ERR ErrLGRICleanupMismatchedLogFiles( BOOL fExtCleanup ) = 0;

    virtual ERR ErrLGCheckState() = 0;

    virtual ERR ErrLGRemoveCommittedLogs(
            __in LONG lGenToDeleteFrom ) = 0;

    virtual ERR ErrLGMakeLogNameBaseless(
        __out_bcount(cbLogName) PWSTR wszLogName,
        ULONG cbLogName,
        __in PCWSTR wszLogFolder,
        const enum eLGFileNameSpec eLogType,
        LONG lGen,
        __in_opt PCWSTR wszLogExt = NULL ) const = 0;

    virtual ERR ErrLGGetGenerationRange(
            __in PCWSTR wszFindPath,
            LONG* plgenLow,
            LONG* plgenHigh,
            __in BOOL fLooseExt = fFalse,
            __out_opt BOOL * pfDefaultExt = NULL ) = 0;
    virtual ERR ErrLGGetGenerationRangeExt(
            __in PCWSTR wszFindPath,
            LONG* plgenLow,
            LONG* plgenHigh,
            __in PCWSTR wszLogExt ) = 0;

    virtual ERR ErrLGRSTOpenLogFile(
            __in PCWSTR             wszLogFolder,
            INT                     gen,
            IFileAPI **const        ppfapi,
            __in_opt PCWSTR         wszLogExt = NULL ) = 0;
    virtual VOID LGRSTDeleteLogs(
            __in PCWSTR wszLog, INT genLow,
            INT genHigh,
            BOOL fIncludeJetLog ) = 0;

    virtual ERR ErrEmitSignalLogBegin() = 0;
    virtual ERR ErrEmitSignalLogEnd() = 0;
    virtual BOOL FLogEndEmitted() const = 0;

    virtual BOOL FCreatedNewLogFileDuringRedo() = 0;

    virtual void AccumulateSectorChecksum( QWORD checksumToAdd, bool fSectorOverwrite = false ) = 0;
    virtual QWORD GetAccumulatedSectorChecksum() const = 0;
    virtual void ResetAccumulatedSectorChecksum() = 0;
};

class LOG_STREAM : public ILogStream
{
public:
    LOG_STREAM( INST * pinst, LOG * pLog );
    ~LOG_STREAM();

    VOID SetLogBuffers(
        LOG_WRITE_BUFFER *  pWriteBuffer
        );

    ERR ErrLGInit();
    VOID LGTerm()
    {
        if ( m_plgfilehdr )
        {
            OSMemoryPageFree( m_plgfilehdr );
            m_plgfilehdr = NULL;
        }
    }

    IFileAPI::FileModeFlags FmfLGStreamDefault( const IFileAPI::FileModeFlags fmfAddl = IFileAPI::fmfRegular ) const;

    QWORD CbOffsetLgpos( LGPOS lgpos1, LGPOS lgpos2 ) const
    {

        return  (QWORD) ( lgpos1.ib - lgpos2.ib )
            + ( (QWORD)( lgpos1.isec - lgpos2.isec ) << Log2CbSec() )
            + (( m_csecLGFile * (QWORD) ( lgpos1.lGeneration - lgpos2.lGeneration ) ) << Log2CbSec() );
    }

    QWORD CbOffsetLgpos( LGPOS lgpos1, LE_LGPOS lgpos2 ) const
    {

        return  (QWORD) ( lgpos1.ib - lgpos2.le_ib )
            + ( (QWORD)( lgpos1.isec - lgpos2.le_isec ) << Log2CbSec() )
            + (( m_csecLGFile * (QWORD) ( lgpos1.lGeneration - lgpos2.le_lGeneration ) ) << Log2CbSec() );
    }

    VOID AddLgpos( LGPOS * const plgpos, UINT cb ) const
    {
        const UINT      ib      = plgpos->ib + cb;

        plgpos->isec = (USHORT)( plgpos->isec + (USHORT)( ib / CbSec() ) );

        plgpos->ib = (USHORT)( ib % CbSec() );

        Assert( ( plgpos->isec < ( m_csecLGFile - 1 ) ) || ( plgpos->isec == ( m_csecLGFile - 1 ) && 0 == plgpos->ib ) );
    }

    VOID AddLgposCanSwitchLogFile( LGPOS * const plgpos, const QWORD cb ) const
    {
        const QWORD     cbLogFile           = CbLGLogFileSize();
        const QWORD     ibOffsetLgposNew    = (QWORD)plgpos->ib +
                                                CbSec() * (QWORD)plgpos->isec +
                                                cbLogFile * (QWORD)plgpos->lGeneration +
                                                cb;
        const QWORD     ib                  = ibOffsetLgposNew % CbSec();
        const QWORD     isec                = ( ibOffsetLgposNew % cbLogFile ) / CbSec();
        const QWORD     lGeneration         = ibOffsetLgposNew / cbLogFile;

        plgpos->ib          = USHORT( ib );
        plgpos->isec        = (USHORT)( isec );
        plgpos->lGeneration = (LONG)( lGeneration );
        Assert( (QWORD)plgpos->ib == ib );
        Assert( (QWORD)plgpos->isec == isec );
        Assert( (QWORD)plgpos->lGeneration == lGeneration );
    }

    QWORD CbOffsetLgposForOB0( LGPOS lgpos1, LGPOS lgpos2 ) const
    {
        if ( m_cbSecSavedForOB0 != 0 )
        {
            Assert( m_csecFileSavedForOB0 != 0 );
            Assert( m_pLog->FRecovering() );
            Assert( lgpos2.lGeneration <= m_lGenSavedForOB0 );

            return (QWORD)( lgpos1.ib - lgpos2.ib ) +
                m_cbSecSavedForOB0 * ( (QWORD)( lgpos1.isec - lgpos2.isec ) +
                                       m_csecFileSavedForOB0 * (QWORD)( lgpos1.lGeneration - lgpos2.lGeneration ) );
        }

        return CbOffsetLgpos( lgpos1, lgpos2 );
    }

    LGPOS LgposFromIbForOB0( QWORD ib ) const
    {
        LGPOS lgpos;
        if ( m_cbSecSavedForOB0 != 0 )
        {
            Assert( m_csecFileSavedForOB0 != 0 );
            lgpos.ib = (USHORT)( ib % m_cbSecSavedForOB0 );
            ib /= m_cbSecSavedForOB0;
            lgpos.isec = (USHORT)( ib % m_csecFileSavedForOB0 );
            ib /= m_csecFileSavedForOB0;
            lgpos.lGeneration = (LONG)ib;

            Assert( lgpos.lGeneration <= m_lGenSavedForOB0 );
        }
        else
        {
            lgpos = lgposMin;
            AddLgposCanSwitchLogFile( &lgpos, ib );
        }

        return lgpos;
    }

    VOID SetLgposUndoForOB0( LGPOS lgpos )
    {
        m_lgposUndoForOB0 = lgpos;
    }

    BOOL FIsOldLrckVersionSeen() const
    {
        return ( m_cbSecSavedForOB0 != 0 );
    }

    VOID ResetOldLrckVersionSecSize()
    {
        m_cbSecSavedForOB0 = 0;
        m_csecFileSavedForOB0 = 0;
        m_lGenSavedForOB0 = 0;
        m_lgposUndoForOB0 = lgposMin;
    }

    ULONG CbLGLogFileSize( ) const
    {
        const ULONG cbLogFileSize = m_csecLGFile * CbSec();
        Assert( cbLogFileSize == ( ( (QWORD)m_csecLGFile ) * CbSec() ) );
        return cbLogFileSize;
    }

    LONG GetCurrentFileGen( LOGTIME * ptmCreate = NULL ) const;
    const LGFILEHDR *GetCurrentFileHdr() const;
    VOID SaveCurrentFileHdr();

    ERR ErrLGGetDesiredLogVersion( _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const LogVersion ** const pplgv );
    BOOL FLGFileVersionUpdateNeeded( _In_ const LogVersion& lgvDesired );

    ERR ErrLGInitSetInstanceWiseParameters();
    ERR ErrLGInitTmpLogBuffers();
    VOID LGTermTmpLogBuffers();

    ERR ErrLGCheckState();

    ERR ErrLGNewLogFile( LONG lgen, BOOL fLGFlags );

    ERR ErrStartAsyncLogFileCreation( _In_ const WCHAR * const wszPathJetTmpLog, _In_ LONG lgenDebug );
    BOOL FShouldCreateTmpLog( const LGPOS &lgposLogRec );
    VOID LGCreateAsynchIOIssue( const LGPOS& lgposLogRec );
    VOID LGCreateAsynchCancel( const BOOL fWaitForPending );

    ERR ErrLGCreateNewLogStream( BOOL * const pfJetLogGeneratedDuringSoftStart );

    VOID LGFullLogNameFromLogId( __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName, LONG lGeneration, __in PCWSTR wszDirectory );

    void LGSetLogExt( BOOL fLegacy, BOOL fReset );

    VOID LGMakeLogName( const enum eLGFileNameSpec eLogType, LONG lGen = 0, BOOL fUseDefaultFolder = fFalse );
    VOID LGMakeLogName( __out_bcount(cbLogName) PWSTR wszLogName, ULONG cbLogName, const enum eLGFileNameSpec eLogType, LONG lGen = 0, BOOL fUseDefaultFolder = fFalse ) const;

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

    PCWSTR
    LogName() const
    {
        return m_wszLogName;
    }

    PCWSTR
    LogExt() const
    {
        return m_wszLogExt;
    }

    VOID
    SetLogSequenceEnd()
    {
        m_rwlLGResFiles.EnterAsWriter();
        Assert( !m_fLogSequenceEnd );
        m_fLogSequenceEnd = fTrue;
        m_rwlLGResFiles.LeaveAsWriter();
    }

    BOOL
    GetLogSequenceEnd()
    {
        BOOL fLogSequenceEnd;

        m_rwlLGResFiles.EnterAsReader();
        fLogSequenceEnd = m_fLogSequenceEnd;
        m_rwlLGResFiles.LeaveAsReader();

        return fLogSequenceEnd;
    }

    UINT CSecLGFile() const
    {
        return m_csecLGFile;
    }

    VOID SetCSecLGFile( ULONG csecLGFile )
    {
        m_csecLGFile = csecLGFile;
    }

    VOID UpdateCSecLGFile()
    {
        m_csecLGFile = CsecLGFromSize( (LONG)UlParam( m_pinst, JET_paramLogFileSize ) );
    }

    ULONG CbSecVolume() const
    {
        return m_cbSecVolume;
    }

    VOID SetCbSecVolume( ULONG cbSecVolume )
    {
        m_cbSecVolume = cbSecVolume;
    }

    BOOL FRolloverInDuration( TICK dtickThreshold )
    {
        const TICK tickNow = TickOSTimeCurrent();
        return TickCmp( tickNow - dtickThreshold, m_tickNewLogFile ) <= 0 &&
                TickCmp( tickNow + dtickThreshold, m_tickNewLogFile ) > 0;
    }

#define m_cbSec     CbSec()

    ULONG CbSec() const
    {
        Assert( m_cbSec_ && FPowerOf2( m_cbSec_ ) );
        return m_cbSec_;
    }

    ULONG Log2CbSec() const
    {
        Assert( m_cbSec_ && m_log2cbSec_ && FPowerOf2( m_cbSec_ ) );
        return m_log2cbSec_;
    }

    VOID LockWrite()
    {
        m_critLGWrite.Enter();
    }

    VOID UnlockWrite()
    {
        m_critLGWrite.Leave();
    }

    VOID LGAssertWriteOwner()
    {
        Assert( m_critLGWrite.FOwner() );
    }

    BOOL FLGProbablyWriting()
    {
        if ( m_critLGWrite.FTryEnter() )
        {
            m_critLGWrite.Leave();
            return fFalse;
        }
        return fTrue;
    }

    BOOL FRemovedLogs()
    {
        return ( m_cRemovedLogs != 0 );
    }

    VOID ResetRemovedLogs()
    {
        m_cRemovedLogs = 0;
    }

    ULONG CSecHeader() const
    {
        return m_csecHeader;
    }

    INT CsecLGFromSize( LONG lLogFileSize ) const
    {
        INT csec = (INT)max( (INT)( ( sizeof( LGFILEHDR ) + CbSec() - 1 + 2 * g_cbPage ) / CbSec() ),
                         min( ( __int64( lLogFileSize ) * 1024 ) / CbSec(),
                              65535  ) );
        AssertRTL( csec == min( ( __int64( lLogFileSize ) * 1024 ) / CbSec(), 65535  ) );
        return csec;
    }

    BYTE *PbSecAligned( BYTE *pb, BYTE * pbLGBufMin ) const
    {
        ULONG log2cbSec = Log2CbSec();
        UINT_PTR mask = UINT_PTR_MAX << log2cbSec;
        return (BYTE*) ( UINT_PTR( pb ) & mask );
    }

    const BYTE *PbSecAligned( const BYTE * pb, BYTE * pbLGBufMin ) const
    {
        ULONG log2cbSec = Log2CbSec();
        UINT_PTR mask = UINT_PTR_MAX << log2cbSec;
        return (BYTE*) ( UINT_PTR( pb ) & mask );
    }

    ERR ErrLGOpenFile( PCWSTR pszPath = NULL, BOOL fReadOnly = fFalse );
    VOID LGCloseFile();

    ERR SetPfapi( IFileAPI * pfapi )
    {
        Assert( !m_pfapiLog );
        m_pfapiLog = pfapi;
        return pfapi->ErrPath( m_wszLogName );
    }

    VOID ResetPfapi()
    {
        m_pfapiLog = NULL;
    }

    BOOL FLGFileOpened() const
    {
        return (m_pfapiLog != NULL);
    }

    ERR ErrFileSize( QWORD *pqwSize )
    {
        return m_pfapiLog->ErrSize( pqwSize, IFileAPI::filesizeLogical );
    }

    ERR ErrLGWriteSectorData(
        __in_ecount( cLogData ) const IOREASON          rgIor[],
        const LONG                                      lgenData,
        const DWORD                                     ibLogData,
        __in_ecount( cLogData ) const ULONG             rgcbLogData[],
        __in_ecount( cLogData ) const BYTE * const      rgpbLogData[],
        const size_t                                    cLogData,
        const DWORD                                     dwLogIdErr,
        BOOL * const                                    pfFlushed = NULL,
        const BOOL                                      fGeneralizeError = fFalse,
        const LGPOS&                                    lgposWriteEnd = lgposMin
        )
    {
        return ErrLGIWriteSectorData( m_pfapiLog, rgIor, lgenData, ibLogData, rgcbLogData, rgpbLogData, cLogData, dwLogIdErr, pfFlushed, fGeneralizeError, lgposWriteEnd );
    }

    ERR ErrLGFlushLogFileBuffers( const IOFLUSHREASON iofr, BOOL * const pfFlushed = NULL );
    void LGAssertFullyFlushedBuffers();
    ERR ErrLGTermFlushLog( const BOOL fCleanTerm )
    {
        ERR err = JET_errSuccess;
        LockWrite();
        if ( m_pfapiLog )
        {
            if ( fCleanTerm )
            {
                err = ErrLGIFlushLogFileBuffers( m_pfapiLog, iofrLogTerm );
            }
            else
            {
                m_pfapiLog->SetNoFlushNeeded();
            }
        }
        UnlockWrite();
        return err;
    }

    ERR ErrLGWriteSectorData(
        const IOREASON          ior,
        const LONG              lgenData,
        const DWORD             ibLogData,
        const ULONG             cbLogData,
        const BYTE * const      pbLogData,
        const DWORD             dwLogIdErr,
        BOOL * const            pfFlushed = NULL,
        const BOOL              fGeneralizeError = fFalse,
        const LGPOS&            lgposWriteEnd = lgposMin
        )
    {
        return ErrLGIWriteSectorData( m_pfapiLog, &ior, lgenData, ibLogData, &cbLogData, &pbLogData, 1, dwLogIdErr, pfFlushed, fGeneralizeError, lgposWriteEnd );
    }
    
    ERR ErrLGReadSectorData(
        const QWORD             ibLogData,
        const ULONG             cbLogData,
        __out_bcount( cbLogData ) BYTE *    pbLogData
        );

    ERR ErrLGRStartNewLog( __in const LONG lgenToCreate );
    ERR ErrLGRRestartLOGAtLogGen( __in const LONG lgenToStartAt, __in const BOOL fLogEvent );
    ERR ErrLGDeleteOutOfRangeLogFiles();

    ERR ErrLGTryOpenJetLog( const ULONG eOpenReason, const LONG lGenNext, BOOL fPickLogExt = fFalse );
    ERR ErrLGOpenJetLog( const ULONG eOpenReason, const ULONG lGenNext, BOOL fPickLogExt = fFalse );

    ERR ErrLGDeleteLegacyReserveLogFiles();

    ERR ErrLGInitCreateReserveLogFiles();

    ERR ErrLGCreateEmptyShadowLogFile( const WCHAR * const wszPath, const LONG lgenShadowDebug )
    {
        Assert( lgenShadowDebug > 0 );
        return ErrLGICreateOneReservedLogFile( wszPath, lgenShadowDebug );
    }

    ERR ErrLGSwitchToNewLogFile( LONG lgenSwitchTo, BOOL fLGFlags );

    ERR ErrEmitSignalLogBegin();
    ERR ErrEmitSignalLogEnd();
    BOOL FLogEndEmitted() const
    {
        return m_fLogEndEmitted;
    }

    VOID LGSetSectorGeometry( const ULONG cbSecSize, const ULONG csecLGFile );
    VOID LGResetSectorGeometry( LONG lLogFileSize = 0 );

    ERR ErrLGReadFileHdr(
        IFileAPI *          pfapiLog,
        IOREASONPRIMARY     iorp,
        LGFILEHDR *         plgfilehdr,
        const BOOL          fNeedToCheckLogID,
        const BOOL          fBypassDbPageAndLogSectorSizeChecks = fFalse );
    ERR ErrLGFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature );
    VOID LGVerifyFileHeaderOnInit();

    BOOL FLGRedoOnCurrentLog() const;
    BOOL FCurrentLogExists() const;
    BOOL FTempLogExists() const;

    ERR ErrLGTryOpenLogFile( const ULONG eOpenReason, const enum eLGFileNameSpec eArchiveLog, const LONG lgen );
    ERR ErrLGOpenLogFile(  const ULONG eOpenReason, const enum eLGFileNameSpec eLogType, const LONG lgen, const ERR errMissingLog );
    ERR ErrLGRIOpenRedoLogFile( LGPOS *plgposRedoFrom, INT *pfStatus );

    VOID LGTruncateLogsAfterRecovery();

    ERR ErrLGTruncateLog(
        const LONG lgenMic,
        const LONG lgenMac,
        const BOOL fSnapshot,
        const BOOL fFullBackup );

    ERR ErrLGRICleanupMismatchedLogFiles( BOOL fExtCleanup );

    ERR ErrLGRemoveCommittedLogs( __in LONG lGenToDeleteFrom );

    ERR ErrLGMakeLogNameBaseless(
        __out_bcount(cbLogName) PWSTR wszLogName,
        ULONG cbLogName,
        __in PCWSTR wszLogFolder,
        const enum eLGFileNameSpec eLogType,
        LONG lGen,
        __in_opt PCWSTR wszLogExt = NULL ) const;

    ERR ErrLGGetGenerationRange( __in PCWSTR wszFindPath, LONG* plgenLow, LONG* plgenHigh, __in BOOL fLooseExt = fFalse, __out_opt BOOL * pfDefaultExt = NULL );
    ERR ErrLGGetGenerationRangeExt( __in PCWSTR wszFindPath, LONG* plgenLow, LONG* plgenHigh, __in PCWSTR wszLogExt );

    BOOL FCreatedNewLogFileDuringRedo()
    {
        return m_fCreatedNewLogFileDuringRedo;
    }

    virtual void AccumulateSectorChecksum( QWORD checksumToAdd, bool fSectorOverwrite )
    {
        if ( fSectorOverwrite )
        {
            m_checksumAccSectors = m_checksumAccSectorsPrev ^ checksumToAdd;
        }
        else
        {
            m_checksumAccSectorsPrev = m_checksumAccSectors;
            m_checksumAccSectors ^= checksumToAdd;
        }
    }

    virtual QWORD GetAccumulatedSectorChecksum() const
    {
        return m_checksumAccSectors;
    }

    virtual void ResetAccumulatedSectorChecksum()
    {
        m_checksumAccSectors = 0;
        m_checksumAccSectorsPrev = 0;
    }

private:
    ERR ErrLGRSTOpenLogFile(
        __in PCWSTR             wszLogFolder,
        INT                         gen,
        IFileAPI **const        ppfapi,
        __in_opt PCWSTR         wszLogExt = NULL );
    VOID LGRSTDeleteLogs( __in PCWSTR wszLog, INT genLow, INT genHigh, BOOL fIncludeJetLog );


private:
    LOG *           m_pLog;
    INST *          m_pinst;
    LOG_WRITE_BUFFER *m_pLogWriteBuffer;

    BOOL            m_fLogSequenceEnd;

    LS              m_ls;

    IFileAPI        *m_pfapiLog;

    WCHAR           m_wszLogName[IFileSystemAPI::cchPathMax];

    TICK            m_tickNewLogFile;


    IFileAPI*       m_pfapiJetTmpLog;

    BOOL            m_fCreateAsynchResUsed;
    BOOL            m_fCreateAsynchZeroFilled;

    ERR             m_errCreateAsynch;

    CAutoResetSignal    m_asigCreateAsynchIOCompleted;

    CCriticalSection    m_critCreateAsynchIOExecuting;

    CCriticalSection    m_critJetTmpLog;
    QWORD           m_ibJetTmpLog;
    DWORD           m_cIOTmpLog;
    DWORD           m_cIOTmpLogMax;
    QWORD*          m_rgibTmpLog;
    DWORD*          m_rgcbTmpLog;

    LGPOS           m_lgposCreateAsynchTrigger;

    LGPOS           m_lgposLogRecAsyncIOIssue;

    PCWSTR          m_wszLogExt;


    LGFILEHDR       *m_plgfilehdr;
    LGFILEHDR       *m_plgfilehdrT;


    QWORD       m_checksumAccSectors;
    QWORD       m_checksumAccSectorsPrev;

#ifdef DEBUG
    BOOL            m_fDBGTraceBR;
#endif

    ULONG           m_cbSecVolume;
    ULONG           m_cbSec_;
    ULONG           m_log2cbSec_;

    ULONG           m_csecHeader;

    UINT            m_csecLGFile;

    ULONG           m_cbSecSavedForOB0;
    ULONG           m_csecFileSavedForOB0;
    LONG            m_lGenSavedForOB0;
    LGPOS           m_lgposUndoForOB0;

    CCriticalSection m_critLGWrite;

    LogVersion      m_lgvHighestRecovered;

#ifdef DEBUG
    BOOL            m_fResettingLogStream;
#endif

    BOOL            m_cRemovedLogs;


    CReaderWriterLock       m_rwlLGResFiles;
    size_t                  m_cbReservePoolHigh;
    ULONG                   m_cReserveLogs;

    POSTRACEREFLOG          m_pEmitTraceLog;
    QWORD                   m_qwSequence;
    BOOL                    m_fLogEndEmitted;

    BOOL                    m_fCreatedNewLogFileDuringRedo;

    CLimitedEventSuppressor m_lesEventLogFeatureDisabled;

#ifdef FORCE_THE_FIRST_GEN_ATTACH_LIST_TO_ZERO
    ERR ErrLGWriteFileHdr( LGFILEHDR* const plgfilehdr );
#endif

    ERR ErrLGIWriteFileHdr(
        IFileAPI * const        pfapi,
        LGFILEHDR* const        plgfilehdr );

    VOID CheckForGenerationOverflow();

    ERR ErrLGIDeleteOldLogStream();

    static VOID LGWriteTmpLog_( LOG_STREAM* const pLogStream );
    
    static void LGICreateAsynchIOComplete_(
        const ERR           err,
        IFileAPI* const     pfapi,
        const IOREASON      ior,
        const OSFILEQOS     grbitQOS,
        const QWORD         ibOffset,
        const DWORD         cbData,
        const BYTE* const   pbData,
        const DWORD_PTR     keyIOComplete
        );
    void LGICreateAsynchIOComplete(
        const ERR           err,
        IFileAPI* const     pfapi,
        const QWORD         ibOffset,
        const DWORD         cbData,
        const BYTE* const   pbData
        );

    VOID LGWriteTmpLog();

    ERR ErrLGIWriteSectorData(
        IFileAPI * const        pfapi,
        __in_ecount( cLogData ) const IOREASON          rgIor[],
        const LONG                                      lgenData,
        const DWORD                                     ibLogData,
        __in_ecount( cLogData ) const ULONG             rgcbLogData[],
        __in_ecount( cLogData ) const BYTE * const      rgpbLogData[],
        const size_t                                    cLogData,
        const DWORD                                     msgidErr,
        BOOL *                                          pfFlushed,
        const BOOL                                      fGeneralizeError = fFalse,
        const LGPOS&                                    lgposWriteEnd = lgposMin
        );

    ERR ErrLGIWriteSectorData(
            IFileAPI * const        pfapi,
            const IOREASON          ior,
            const LONG              lgenData,
            const DWORD             ibLogData,
            const ULONG             cbLogData,
            const BYTE * const      pbLogData,
            const DWORD             dwLogIdErr,
            BOOL *                  pfFlushed,
            const BOOL              fGeneralizeError
            )
    {
        return ErrLGIWriteSectorData( pfapi, &ior, lgenData, ibLogData, &cbLogData, &pbLogData, 1, dwLogIdErr, pfFlushed, fGeneralizeError );
    };

    ERR ErrLGIFlushLogFileBuffers( IFileAPI * const pfapiLog, const IOFLUSHREASON iofr, BOOL *pfFlushed = fFalse );

    VOID LGSzFromLogId( __out_bcount( cbFName ) PWSTR wszLogFileName, size_t cbFName, LONG lGeneration );

    ERR ErrLGIGrowReserveLogPool(
        ULONG cResLogsInc,
        BOOL fUseDefaultFolder );
    ERR ErrLGIOpenReserveLogFile(
        IFileAPI** const        ppfapi,
        const WCHAR* const      wszPathJetTmpLog,
        const LONG              lgenNextDebug
        );
    ERR ErrLGICreateOneReservedLogFile(
        const WCHAR * const     wszPath,
        const LONG              lgenDebug  );

    VOID InitLgfilehdrT();
    VOID SetSignatureInLgfilehdrT();
    VOID LGSetFlagsInLgfilehdrT( const BOOL fResUsed );

    ERR ErrLGIFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature, const LogVersion&  );

    ERR ErrLGIStartNewLogFile(
        const LONG              lgenToClose,
        BOOL                    fLGFlags,
        IFileAPI ** const       ppfapiNewLog
        );
    ERR ErrLGIFinishNewLogFile( IFileAPI * const pfapiTmpLog );

    ERR ErrLGINewLogFile( LONG lgen, BOOL fLGFlags, IFileAPI ** const ppfapiNewLog );
    ERR ErrLGIUseNewLogFile( _Inout_ IFileAPI ** ppfapiNewLog );

    VOID CheckLgposCreateAsyncTrigger();

    ERR ErrOpenTempLogFile(
        IFileAPI ** const       ppfapi,
        const WCHAR * const     wszPathJetTmpLog,
        const LONG              lgenNextDebug,
        BOOL * const            pfResUsed,
        BOOL * const            pfZeroFilled,
        QWORD * const           pibPattern );

    ERR ErrFormatLogFile(
        IFileAPI ** const       ppfapi,
        const WCHAR * const     wszPathJetTmpLog,
        const LONG              lgenNextDebug,
        BOOL * const            pfResUsed,
        const QWORD&            cbLGFile,
        const QWORD&            ibPattern );

    ERR ErrLGIOpenTempLogFile(
        IFileAPI** const        ppfapi,
        const WCHAR* const      wszPathJetTmpLog,
        const LONG              lgenNextDebug,
        BOOL* const             pfResUsed,
        BOOL* const             pfZeroFilled
        );
    ERR ErrLGIReuseArchivedLogFile(
        IFileAPI** const        ppfapi,
        const WCHAR* const      wszPathJetTmpLog,
        const LONG              lgenNextDebug
        );

    DWORD CbLGICreateAsynchIOSize();

    VOID LGIReportLogTruncation(
        const LONG          lgenMic,
        const LONG          lgenMac,
        const BOOL          fSnapshot );

    ERR ErrEmitLogData(
        IFileAPI * const        pfapi,
        const IOREASON          ior,
        const LONG              lgenData,
        const DWORD             ibLogData,
        const ULONG             cbLogData,
        const BYTE * const      pbLogData
        );
    ERR ErrEmitCompleteLog( LONG lgenToClose );
};

extern PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cbLGFileSize;
extern PERFInstanceDelayedTotal<> cLGUsersWaiting;

class CShadowLogStream
{

#ifdef DEBUG
    static const LONG c_dtickWriteLogBuff = 16;
#else
    static const LONG c_dtickWriteLogBuff = 2000;
#endif
    public:
        enum WriteFlags
        {
            ffNone = 0,
            ffForceResetIgnoreFailures = 1,
        };

    private:
        INST *              m_pinst;
        LGPOS               m_lgposCurrent;
        unsigned __int64    m_qwSequenceNumCurrent;
        IFileAPI *          m_pfapiCurrent;
        POSTIMERTASK        m_postWriteLogBuffTask;
        bool                m_fWriteLogBuffScheduled;
        CCriticalSection    m_critLogBuff;
        BYTE *              m_rgbLogBuff;
        ULONG               m_cbLogBuffMax;
        ULONG               m_ibLogBuffPending;
        ULONG               m_cbLogBuffPending;
        ULONG               m_ibExtendedFile;
        bool                m_fConsumedBegin;

    private:
        CShadowLogStream( );

    public:
        static ERR ErrCreate(
            INST *              pinst,
            CShadowLogStream ** ppshadowLog
            );
        ~CShadowLogStream( );

    private:
        ERR ErrWriteLogData_(
            void *              pvLogData,
            ULONG       ibOffset,
            ULONG       cbLogData );
        ERR ErrUpdateCurrentPosition_(
            LGPOS               lgposLogData,
            ULONG       cbLogData );
        ERR ErrResetSequence_(
            unsigned __int64    qwNextSequenceNumber );
        ERR ErrWriteLogBuff_();
        static void WriteLogBuffTask(
            void *              pvGroupContext,
            void *              pvRuntimeContext);

    public:
        ERR ErrWriteAndResetLogBuff( WriteFlags flags = ffNone );
        ERR ErrAddBegin();
        ERR ErrAddData(
            JET_GRBIT           grbitOpFlags,
            unsigned __int64    qwSequenceNumber,
            LGPOS               lgposLogData,
            void *              pvLogData,
            ULONG       cbLogData );
        ERR ErrCompleteLog(
            JET_GRBIT           grbitOpFlags,
            unsigned __int64    qwSequenceNumber,
            LGPOS               lgposLogData );
};


extern PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> ibLGTip;
extern PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cLGLogFileCurrentGeneration;

