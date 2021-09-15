// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
//  This is for secondary / shadow log stream support.
//

//
//  This is primarily separated into two parts:
//      A. Emitting / Providing Log Data.
//      B. Consuming / Writing Log Data.
//      (oh yeah and Init/Term logic of each)
//

#include "logstd.hxx"


//  -----------------------------------------------------------------------------------------
//      Emitting / Providing Log data
//  -----------------------------------------------------------------------------------------


//  -----------------------------------------------------------------------------------------
//  Init / Term Log Data
//

const ULONG g_dwNativeSemiSyncVersion = 1;

ERR LOG_STREAM::ErrEmitSignalLogBegin()
{
    JET_PFNEMITLOGDATA  pfnErrEmit  = (JET_PFNEMITLOGDATA)PvParam( m_pinst, JET_paramEmitLogDataCallback );
    if ( NULL == pfnErrEmit )
    {
        return ErrERRCheck( JET_wrnCallbackNotRegistered );
    }

    if ( m_pLog->FHardRestore() )
    {
        //  Squelch hard restore cases...
        return JET_errSuccess;
    }

    void* pvCallBackCtx = (void*)PvParam( m_pinst, JET_paramEmitLogDataCallbackCtx );

    Assert( 0 == m_qwSequence );

    JET_EMITDATACTX     emitCtx         = { 0 };
    emitCtx.cbStruct                    = sizeof(emitCtx);
    emitCtx.dwVersion                   = g_dwNativeSemiSyncVersion;
    emitCtx.qwSequenceNum               = 0;
    emitCtx.grbitOperationalFlags       = JET_bitShadowLogEmitFirstCall;
    LGIGetDateTime( (LOGTIME*)&(emitCtx.logtimeEmit) );

    Ptls()->fInCallback = fTrue;

    const ERR err = (*pfnErrEmit)( (JET_INSTANCE)m_pinst, &emitCtx, NULL, 0, pvCallBackCtx );

    //  Increment to the next sequence number

    m_qwSequence = 1;

    Ptls()->fInCallback = fFalse;

    return err;
}

ERR LOG_STREAM::ErrEmitSignalLogEnd()
{
    JET_PFNEMITLOGDATA  pfnErrEmit  = (JET_PFNEMITLOGDATA)PvParam( m_pinst, JET_paramEmitLogDataCallback );
    if ( NULL == pfnErrEmit )
    {
        return ErrERRCheck( JET_wrnCallbackNotRegistered );
    }

    if ( m_pLog->FHardRestore() )
    {
        //  Squelch hard restore cases...
        return JET_errSuccess;
    }

    if ( m_fLogEndEmitted )
    {
        return JET_errSuccess;
    }

    void* pvCallBackCtx = (void*)PvParam( m_pinst, JET_paramEmitLogDataCallbackCtx );

    JET_EMITDATACTX     emitCtx         = { 0 };
    emitCtx.cbStruct                    = sizeof(emitCtx);
    emitCtx.dwVersion                   = g_dwNativeSemiSyncVersion;
    emitCtx.qwSequenceNum               = 0xFFFFFFFFFFFFFFF0;
    emitCtx.grbitOperationalFlags       = JET_bitShadowLogEmitLastCall;
    LGIGetDateTime( (LOGTIME*)&(emitCtx.logtimeEmit) );

    Ptls()->fInCallback = fTrue;

    const ERR err = (*pfnErrEmit)( (JET_INSTANCE)m_pinst, &emitCtx, NULL, 0, pvCallBackCtx );

    Ptls()->fInCallback = fFalse;

    m_fLogEndEmitted = fTrue;

    return err;
}

ERR LOG_STREAM::ErrEmitLogData(
    IFileAPI * const        pfapiLog,
    const IOREASON          ior,
    const LONG              lgenData,
    const DWORD             ibLogData,  // offset
    const ULONG             cbLogData,  // size
    const BYTE * const      pbLogData   // buffer
    )
{
    JET_PFNEMITLOGDATA  pfnErrEmit  = (JET_PFNEMITLOGDATA)PvParam( m_pinst, JET_paramEmitLogDataCallback );
    LONG                lGenEmit    = lgenData;

    //  It is SUPER CRITICALLY IMPORTANT that we must get the lGeneration right
    //  for any shadow logging emitted data.  In order to facilitate this, even if we
    //  do not have a registered callback, we will check that the lGen matches.


    if ( ibLogData == 0 )
    {
        Assert( lGenEmit == ((LGFILEHDR *)pbLogData)->lgfilehdr.le_lGeneration );
    }
    else if ( !FIsOldLrckLogFormat( GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        Assert( lGenEmit == ((LGSEGHDR *)pbLogData)->le_lgposSegment.le_lGeneration );
    }

    Assert( lGenEmit > 0 );


    if ( NULL == pfnErrEmit )
    {
        //  No emit callback registered, return success-ish.
        return ErrERRCheck( JET_wrnCallbackNotRegistered );
    }

    if ( m_pLog->FHardRestore() )
    {
        //  Squelch hard restore cases...
        return JET_errSuccess;
    }

    Assert( ( m_pLog->FRecovering() && m_pLog->FRecoveringMode() == fRecoveringRedo ) ||
            m_critLGWrite.FOwner() );

    void* pvCallBackCtx = (void*)PvParam( m_pinst, JET_paramEmitLogDataCallbackCtx );

    JET_EMITDATACTX     emitCtx         = { 0 };
    emitCtx.cbStruct                    = sizeof(emitCtx);
    emitCtx.dwVersion                   = g_dwNativeSemiSyncVersion;
    Assert( 0 != m_qwSequence );
    emitCtx.qwSequenceNum               = m_qwSequence;
    emitCtx.grbitOperationalFlags       = JET_bitShadowLogEmitDataBuffers;

    LGIGetDateTime( (LOGTIME*)&(emitCtx.logtimeEmit) );

    //  LGPOS can be calculated off the LGFILEHDR of the log we're writing 
    //  and the offset.
    emitCtx.lgposLogData.lGeneration    = lGenEmit;
    Assert( 0 != emitCtx.lgposLogData.lGeneration );
    emitCtx.lgposLogData.isec           = (USHORT) ( ibLogData / m_cbSec );
    Assert( (QWORD)emitCtx.lgposLogData.isec == ibLogData / m_cbSec );  // check for under|overflow ...
    emitCtx.lgposLogData.ib             = (USHORT) ( ibLogData % m_cbSec );
    Assert( (QWORD)emitCtx.lgposLogData.ib == ibLogData % m_cbSec );        // check for under|overflow ...

    emitCtx.cbLogData                   = cbLogData;

    if ( m_pEmitTraceLog != NULL )
    {
        OSTraceWriteRefLog( m_pEmitTraceLog,
                                 lGenEmit,
                                 NULL );
    }

    Ptls()->fInCallback = fTrue;

    const ERR err = (*pfnErrEmit)( (JET_INSTANCE)m_pinst, &emitCtx, (void *)pbLogData, cbLogData, pvCallBackCtx );

    //  Increment the next ...
    m_qwSequence++;

    Ptls()->fInCallback = fFalse;

    return err;
}


ERR LOG_STREAM::ErrEmitCompleteLog( LONG lgenToClose )
{
    JET_PFNEMITLOGDATA  pfnErrEmit  = (JET_PFNEMITLOGDATA)PvParam( m_pinst, JET_paramEmitLogDataCallback );
    if ( NULL == pfnErrEmit )
    {
        return ErrERRCheck( JET_wrnCallbackNotRegistered );
    }

    if ( m_pLog->FHardRestore() )
    {
        //  Squelch hard restore cases...
        return JET_errSuccess;
    }

    Assert( m_critLGWrite.FOwner() );

    void* pvCallBackCtx = (void*)PvParam( m_pinst, JET_paramEmitLogDataCallbackCtx );

    JET_EMITDATACTX     emitCtx         = { 0 };
    emitCtx.cbStruct                    = sizeof(emitCtx);
    emitCtx.dwVersion                   = g_dwNativeSemiSyncVersion;
    emitCtx.qwSequenceNum               = m_qwSequence;
    emitCtx.grbitOperationalFlags       = JET_bitShadowLogEmitLogComplete;
    LGIGetDateTime( (LOGTIME*)&(emitCtx.logtimeEmit) );

    emitCtx.lgposLogData.lGeneration    = lgenToClose;
    Assert( 0 != emitCtx.lgposLogData.lGeneration );

    if ( m_pEmitTraceLog != NULL )
    {
        OSTraceWriteRefLog( m_pEmitTraceLog,
                                 lgenToClose,
                                 NULL );
    }

    Ptls()->fInCallback = fTrue;

    const ERR err = (*pfnErrEmit)( (JET_INSTANCE)m_pinst, &emitCtx, NULL, 0, pvCallBackCtx );

    //  Increment the next ...
    m_qwSequence++;

    Ptls()->fInCallback = fFalse;

    return err;
}



//  -----------------------------------------------------------------------------------------
//      Consuming / Writting Log data
//  -----------------------------------------------------------------------------------------

bool FAtBeginningOfLogFile( LGPOS * plgpos )
{
    return ( plgpos->isec == 0 ) && ( plgpos->ib == 0 );
}

ULONG IbFromLgpos( INST * const pinst, LGPOS lgpos )
{
    return lgpos.isec * pinst->m_plog->CbLGSec() + lgpos.ib;
}

CShadowLogStream::CShadowLogStream( ) :
    m_pinst( NULL ),
    m_qwSequenceNumCurrent( 0 ),
    m_pfapiCurrent( NULL ),
    m_postWriteLogBuffTask( NULL ),
    m_fWriteLogBuffScheduled( false ),
    m_critLogBuff( CLockBasicInfo( CSyncBasicInfo( szShadowLogBuff ), rankShadowLogBuff, 0 ) ),
    m_rgbLogBuff( NULL ),
    m_cbLogBuffMax( 0 ),
    m_ibLogBuffPending( 0 ),
    m_cbLogBuffPending( 0 ),
    m_ibExtendedFile( 0 )
{
    m_lgposCurrent.lGeneration = 0;
    m_lgposCurrent.isec = 0;
    m_lgposCurrent.ib = 0;
}

ERR CShadowLogStream::ErrCreate( INST * pinst, CShadowLogStream ** ppshadowLog )
{
    ERR err = JET_errSuccess;
    CShadowLogStream* pshadowLog = NULL;

    Alloc( pshadowLog = new CShadowLogStream() );
    pshadowLog->m_pinst = pinst;
    pshadowLog->m_cbLogBuffMax = (ULONG) UlParam( pinst, JET_paramLogFileSize ) * 1024;
    Alloc( pshadowLog->m_rgbLogBuff = (BYTE*) PvOSMemoryPageAlloc( pshadowLog->m_cbLogBuffMax, NULL ) );

    Call( ErrOSTimerTaskCreate( WriteLogBuffTask, pshadowLog, &pshadowLog->m_postWriteLogBuffTask ) );

    *ppshadowLog = pshadowLog;
    CallS( err );   // no warnings expected
    return err;

HandleError:
    delete pshadowLog;
    return err;
}

// The shadow log buffer m_rgbLogBuff must be written completely before calling the destructor, or we will Enforce()
//
CShadowLogStream::~CShadowLogStream()
{
    if ( m_postWriteLogBuffTask )
    {
        OSTimerTaskCancelTask( m_postWriteLogBuffTask );
        OSTimerTaskDelete( m_postWriteLogBuffTask );
        m_postWriteLogBuffTask = NULL;
    }

    m_critLogBuff.Enter();
    if ( m_rgbLogBuff )
    {
        Enforce( m_cbLogBuffPending == 0 );
        OSMemoryPageFree( m_rgbLogBuff );
    }
    m_critLogBuff.Leave();

    if ( m_pfapiCurrent )
    {
        m_pfapiCurrent->SetNoFlushNeeded();
    }
    //  Note this closes the file handle to the .jsl / shadow log file
    delete m_pfapiCurrent;
}

ERR CShadowLogStream::ErrWriteLogData_(
    void *              pvLogData,
    ULONG       ibOffset,
    ULONG       cbLogData )
{
    ERR     err = JET_errSuccess;
    TraceContextScope tcScope( iorpShadowLog );

    Assert( m_pfapiCurrent );

    //  Defensively avoid writing higher than the log file size

    QWORD cbLogFileSize = 0;
    Call( m_pfapiCurrent->ErrSize( &cbLogFileSize, IFileAPI::filesizeLogical ) );
    if ( ibOffset + cbLogData > cbLogFileSize )
    {
        Assert( FNegTest( fCorruptingLogFiles ) || ibOffset + cbLogData <= cbLogFileSize );
        Error( ErrERRCheck( JET_errFileIOBeyondEOF ) );
    }

    Call( m_pfapiCurrent->ErrIOWrite(
                *tcScope,
                ibOffset,
                cbLogData,
                (BYTE*)pvLogData,
                QosSyncDefault( m_pinst ) ) );

HandleError:

    return err;
}

ERR CShadowLogStream::ErrUpdateCurrentPosition_(
    LGPOS               lgposLogData,
    ULONG       cbLogData )
{
    Assert( ( IbFromLgpos( m_pinst, lgposLogData ) + cbLogData ) <= 1024 * UlParam( m_pinst, JET_paramLogFileSize ) );

    if ( m_lgposCurrent.lGeneration != lgposLogData.lGeneration &&
        0 != lgposLogData.isec &&
        0 != lgposLogData.ib )
    {
        return ErrERRCheck( JET_errInvalidLogDataSequence );
    }
    Assert( (ULONG)(USHORT) ( cbLogData / m_pinst->m_plog->CbLGSec() ) == cbLogData / m_pinst->m_plog->CbLGSec() );

    //  Improve the current position in the log that we know we've written through

    m_lgposCurrent.lGeneration = lgposLogData.lGeneration;
    m_lgposCurrent.isec = lgposLogData.isec + (USHORT) ( cbLogData / m_pinst->m_plog->CbLGSec() );
    m_lgposCurrent.ib = 0;
    //  For now we only whole sectors, so we only track whole sectors improvement ... but in the future we
    // may need to enhance this to handle partial sectors.  Something like this:
    //m_lgposCurrent.isec = lgposLogData.isec + 
    //                          (USHORT) ( cbLogData / m_pinst->m_plog->CbLGSec() ) + 
    //                          ( ( lgposLogData.ib + cbLogData % plog->CbLGSec() ) / m_pinst->m_plog->CbLGSec() );
    //m_lgposCurrent.ib = lgposLogData.ib + cbLogData % plog->CbLGSec();

    //  Improve the sequence number we expect from the log data producer.

    m_qwSequenceNumCurrent++;
    return JET_errSuccess;
}

ERR CShadowLogStream::ErrResetSequence_( unsigned __int64   qwNextSequenceNumber )
{
    if ( qwNextSequenceNumber == 0 )
    {
        // only the init callback ctx should have a 0 sequence number
        return ErrERRCheck( JET_errInvalidParameter );
    }

    //  Note: set it 1 lower, b/c about to absorb this sequence number.

    m_qwSequenceNumCurrent = qwNextSequenceNumber - 1;

    return JET_errSuccess;
}

ERR CShadowLogStream::ErrWriteLogBuff_()
{
    ERR err = JET_errSuccess;

    Assert( m_critLogBuff.FOwner() );
    if ( m_cbLogBuffPending == 0 )
    {
        return JET_errSuccess;
    }

    //  To minimize filesystem IOs for extending the file on each log write, we opportunistically extend the file by rounding up current IO to max write size
    //  The log buffer always starts out zeroed when a new log begins, making the extended IO a zeroing write
    //
    ULONG cbLogWrite = m_cbLogBuffPending;
    if ( m_ibLogBuffPending + cbLogWrite > m_ibExtendedFile )
    {
        cbLogWrite = roundup( cbLogWrite, (ULONG) UlParam( m_pinst, JET_paramMaxCoalesceWriteSize ) );
        cbLogWrite = min( m_cbLogBuffMax - m_ibLogBuffPending, cbLogWrite );    // don't go beyond the buffer size
    }

    Call( ErrWriteLogData_( m_rgbLogBuff + m_ibLogBuffPending, m_ibLogBuffPending, cbLogWrite ) );

    m_ibExtendedFile = m_ibLogBuffPending + cbLogWrite;
    m_ibLogBuffPending += m_cbLogBuffPending;
    m_cbLogBuffPending = 0;

HandleError:
    return err;
}

ERR CShadowLogStream::ErrWriteAndResetLogBuff( WriteFlags flags /* = ffNone */ )
{
    m_critLogBuff.Enter();
    ERR err = ErrWriteLogBuff_();
    if ( !( flags & ffForceResetIgnoreFailures ) )
    {
        Call( err );
    }

    // If ffForceResetIgnoreFailures was specified, and we encountered an error (most likely an IO error), we will go ahead and reset the buffer losing buffered data
    // and leaving the shadow log in an incomplete/corrupted state. This is only expected to be called in the ErrLGTerm() code path
    // where we can't do anything about the failure.
    //
    m_ibLogBuffPending = m_cbLogBuffPending = m_ibExtendedFile = 0;
    memset( m_rgbLogBuff, 0, m_cbLogBuffMax );
    Expected( FNegTest( fCorruptingLogFiles ) || !m_cbLogBuffPending );    // we should have written out all the buffers

HandleError:
    m_critLogBuff.Leave();
    return err;
}

void CShadowLogStream::WriteLogBuffTask(
    void * pvGroupContext,
    void * pvRuntimeContext )
{
    ERR err = JET_errSuccess;
    CShadowLogStream * pshadowLog = (CShadowLogStream *) pvGroupContext;
    pshadowLog->m_critLogBuff.Enter();

    pshadowLog->m_fWriteLogBuffScheduled = false;
    Call( pshadowLog->ErrWriteLogBuff_() );

HandleError:
    //  We ignore disk errors, a failed buffer write will be retried later and will make it to the client eventually when the buffer is written on a client thread
    Assert( err == JET_errSuccess || err == JET_errDiskIO );
    pshadowLog->m_critLogBuff.Leave();
}


ERR CShadowLogStream::ErrAddBegin()
{
    //  We want to know if the source sent us a begin ( "init" ) as we allow ourselves to start in the
    //  middle of a log in this rare case, to handle init/term.

    m_fConsumedBegin = true;

    return JET_errSuccess;
}

ERR CShadowLogStream::ErrAddData(
    JET_GRBIT           grbitOpFlags,
    unsigned __int64    qwSequenceNumber,
    LGPOS               lgposLogData,
    void *              pvLogData,
    ULONG       cbLogData )
{
    ERR err = JET_errSuccess;
    ERR wrn = JET_errSuccess;

    Assert( m_pinst );
    Assert( m_pinst->m_pfsapi );
    Assert( m_pinst->m_plog );

    Assert( pvLogData );
    Assert( cbLogData );

    if ( lgposLogData.ib != 0 )
    {
        // We aren't able to handle partial sector writes / updates yet.
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    //
    //  Evaluate the context information, what case is it ...
    //

    const bool bInSequencePerfect =
                qwSequenceNumber == ( m_qwSequenceNumCurrent + 1 ) &&           // in sequence order
                m_lgposCurrent.lGeneration == lgposLogData.lGeneration &&       // same log file
                m_lgposCurrent.isec == lgposLogData.isec &&                     // next avail sector
                m_lgposCurrent.ib == lgposLogData.ib;

    const bool bInSequenceSectorOverwrite =
                qwSequenceNumber == ( m_qwSequenceNumCurrent + 1 ) &&           // in sequence order
                m_lgposCurrent.lGeneration == lgposLogData.lGeneration &&       // same log file
                (   ( ( m_lgposCurrent.isec >= 1 ) && ( ( m_lgposCurrent.isec - 1 ) == lgposLogData.isec ) ) || // overwriting previous sector
                    ( ( m_lgposCurrent.isec >= 2 ) && ( ( m_lgposCurrent.isec - 2 ) == lgposLogData.isec ) )    // overwriting previous 2 sectors
                    ) &&
                0 == lgposLogData.ib;                                           // beginning of sector

    const bool bInNewSequence = FAtBeginningOfLogFile( &lgposLogData ) &&       // do not care about sequence in this case
                m_lgposCurrent.lGeneration != lgposLogData.lGeneration;         // and we're working on a different log file.
                // 2nd clause, see tm.cxx call to plog->ErrLGWriteFileHdr().

    const bool bMidSequenceFirstData = m_fConsumedBegin &&                      // we got a begin/init signal and this is the first data after
                !FAtBeginningOfLogFile( &lgposLogData ) &&                      // not at beginning, otherwise can treat as bInNewSequeunce
                NULL == m_pfapiCurrent;                                         // should not have the file open already (just checking)

    //
    //  Switch log files if necessary ...
    //

    if ( bInNewSequence || bMidSequenceFirstData )  // beginning of a new log
    {
        WCHAR wszLogFile[ OSFSAPI_MAX_PATH ] = L"";

        Call( ErrWriteAndResetLogBuff() );

        //
        //  Close down previous log file if open
        //

        if ( m_pfapiCurrent )
        {
            Assert( 0 != m_lgposCurrent.lGeneration );
            //  We have a current log file open, so we need to close the log file.

            OSTrace( JET_tracetagLogging, OSFormat( "Alt log writer aborting log gen 0x%x, because we switched to a new log gen (0x%x) without completing the previous one.\n", m_lgposCurrent.lGeneration, lgposLogData.lGeneration ) );

            //  Note: We can avoid flush of the log file here, because we will flush before adding the file to 
            //  the DB header max required.
            m_pfapiCurrent->SetNoFlushNeeded();
            delete m_pfapiCurrent;
            m_pfapiCurrent = NULL;

            //  delete log file ...

            Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
                                    wszLogFile, sizeof(wszLogFile),
                                    m_pinst->m_pfsapi,
                                    SzParam( m_pinst, JET_paramLogFilePath ),
                                    SzParam( m_pinst, JET_paramBaseName ),
                                    eShadowLog,
                                    m_lgposCurrent.lGeneration,
                                    wszSecLogExt,
                                    ( UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8) );

            Call( m_pinst->m_pfsapi->ErrFileDelete( wszLogFile ) );
            wszLogFile[0] = L'\0';  // reset, jic

            Assert( JET_errSuccess == wrn );
            wrn = ErrERRCheck( JET_wrnPreviousLogFileIncomplete );
        }

        if ( lgposLogData.isec == 0 && lgposLogData.ib == 0 )
        {
            //
            //  Validate the sector size matches
            //

            // Technically we should be checking against cbLogFileHeader, but for testing we'll just make sure
            // it is big enough to dereference the le_cbSec.
            // Also are there any other parts of the LGFILEHDR we should be checking?  Not that I think actually
            // affect this write code path.
            if ( cbLogData < 512 ||
                    NULL == pvLogData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            C_ASSERT( OffsetOf(LGFILEHDR, lgfilehdr.le_cbSec) < (size_t)512 );

            const LGFILEHDR * const plgfilehdr = (LGFILEHDR*)pvLogData;
            // if header has no sector size (then we're in a unit test case
            // and) we'll pretend we have 512-bytes sectors ... otherwise we'll
            // use the sector size reported by the remote side as it's required
            // to know where to set / store the data.
            if ( plgfilehdr->lgfilehdr.le_cbSec != 0 && // for testing we allow through 0
                 plgfilehdr->lgfilehdr.le_cbSec != m_pinst->m_plog->CbLGSec() )
            {
                Error( ErrERRCheck( JET_errLogSectorSizeMismatch ) );
            }
        }
        else
        {
            Assert( bMidSequenceFirstData );
        }

        //
        //  Reset the sequence we're consuming ... 
        //

        Call( ErrResetSequence_( qwSequenceNumber ) );

        //
        //  Create a new log file to write new data to
        //

        Call( LGFileHelper::ErrLGMakeLogNameBaselessEx(
                                wszLogFile, sizeof(wszLogFile),
                                m_pinst->m_pfsapi,
                                SzParam( m_pinst, JET_paramLogFilePath ),
                                SzParam( m_pinst, JET_paramBaseName ),
                                eShadowLog,
                                lgposLogData.lGeneration,
                                wszSecLogExt,
                                ( UlParam( m_pinst, JET_paramLegacyFileNames ) & JET_bitEightDotThreeSoftCompat ) ? 0 : 8) );

        if ( !bMidSequenceFirstData )
        {
            //  Create log file. The log file isn't physically zeroed, but its size is extended to JET_paramLogFileSize and is considered zeroed logically.
            //
            Assert( lgposLogData.lGeneration > 0 );
            Call( m_pinst->m_plog->ErrLGCreateEmptyShadowLogFile( wszLogFile, lgposLogData.lGeneration ) );
        }

        //  Open the log file
        Assert( lgposLogData.lGeneration > 0 ); // don't want 0 lgen passed to QwInstFileID()
        err = CIOFilePerf::ErrFileOpen(
                                m_pinst->m_pfsapi,
                                m_pinst,
                                wszLogFile,
                                BoolParam( m_pinst, JET_paramUseFlushForWriteDurability ) ? IFileAPI::fmfStorageWriteBack : IFileAPI::fmfNone,
                                iofileLog,
                                QwInstFileID( qwLogFileID, m_pinst->m_iInstance, lgposLogData.lGeneration ),
                                &m_pfapiCurrent );

        if ( bMidSequenceFirstData && JET_errFileNotFound == err )
        {
            //  This could mean we restarted on a file that doesn't exist, this is a potentially common
            //  case, and we wanted treated as a the client just being out of sequence.
            Error( ErrERRCheck( JET_errInvalidLogDataSequence ) );
        }
        Call( err )
    }

    //
    //  Now try to add data to the current log file if appropriate...
    //

    if ( bInNewSequence || bInSequencePerfect || bInSequenceSectorOverwrite || bMidSequenceFirstData )
    {
        //  Are we doing sector overwrite?

        if ( bInSequenceSectorOverwrite )
        {
            //  set back the current log position


            m_lgposCurrent = lgposLogData;
        }

        //  Add the log data to the log file buffer

        ULONG ibOffset = IbFromLgpos( m_pinst, lgposLogData );
        if ( ibOffset + cbLogData > m_cbLogBuffMax )
        {
            Assert( FNegTest( fCorruptingLogFiles ) || ibOffset + cbLogData <= m_cbLogBuffMax );
            Error( ErrERRCheck( JET_errInvalidLogDataSequence ) );
        }

        ENTERCRITICALSECTION ecs( &m_critLogBuff );

        //  Make sure that we are appending data to exactly the end of the buffer
        //  OR we are starting to consume a mid-sequence log, in which case, data is added to the mid of the buffer (at the same offset)
        //  OR we are overwriting a partial sector
        Assert( ibOffset == m_ibLogBuffPending + m_cbLogBuffPending || bMidSequenceFirstData || bInSequenceSectorOverwrite );

        if ( bMidSequenceFirstData )
        {
            Assert( m_ibLogBuffPending == 0 && m_cbLogBuffPending == 0 );
            m_ibLogBuffPending = ibOffset;  //  the start of the buffer is empty in case of mid-sequence data
            m_ibExtendedFile = ibOffset;    //  we presume that anything before ibOffset is valid data
        }

        if ( bInSequenceSectorOverwrite )
        {
            if ( m_ibLogBuffPending > ibOffset )
            {
                m_ibLogBuffPending = ibOffset;  //  make sure that the sector is overwritten on disk, if the buffer has already been written passed the sector in question
                m_cbLogBuffPending = 0; // discard all existing data (it will be overwritten with newer data)
            }
            else
            {
                Assert( ibOffset < m_ibLogBuffPending + m_cbLogBuffPending );
                m_cbLogBuffPending = ibOffset - m_ibLogBuffPending; // adjust m_cbLogBuffPending to discard part of log that is being overwritten.
            }
        }

        memcpy( m_rgbLogBuff + ibOffset, pvLogData, cbLogData );
        m_cbLogBuffPending += cbLogData;

        if ( !m_fWriteLogBuffScheduled )
        {
            OSTimerTaskScheduleTask( m_postWriteLogBuffTask, NULL, c_dtickWriteLogBuff, 0 );
            m_fWriteLogBuffScheduled = true;
        }

        //  Update our position

        Call( ErrUpdateCurrentPosition_( lgposLogData, cbLogData ) );
    }
    else
    {
        //  This basically does nothing in this case, which is what we want and eventually
        //  we will transition to a new log and things will get in wack.

        Assert( FNegTest( fCorruptingLogFiles ) || bInNewSequence || bInSequencePerfect || bInSequenceSectorOverwrite || bMidSequenceFirstData );
        err = ErrERRCheck( JET_errInvalidLogDataSequence );
    }

HandleError:
    
    m_fConsumedBegin = false;

    return err ?  err : wrn;
}


ERR CShadowLogStream::ErrCompleteLog(
    JET_GRBIT           grbitOpFlags,
    unsigned __int64    qwSequenceNumber,
    LGPOS               lgposLogData )
{
    ERR err = JET_errSuccess;

    Assert( JET_bitShadowLogEmitLogComplete & grbitOpFlags );

    if ( qwSequenceNumber == ( m_qwSequenceNumCurrent + 1 ) &&      // sequence matches
        m_lgposCurrent.lGeneration == lgposLogData.lGeneration &&   // knows the log file we're working on
        m_pfapiCurrent )                                            // and we're working on a log file
    {
        Assert( 0 != m_lgposCurrent.lGeneration );

        Call( ErrWriteAndResetLogBuff() );

        //  Success!!  We've shipped our log file.
        OSTrace( JET_tracetagLogging, OSFormat( "Alt log writer has successfully completed log gen 0x%x\n", lgposLogData.lGeneration ) );

        //  We have a current log file open, so we need to close the log file.
        //  Note: We can avoid flush of the log file here, because we will flush before adding the file to 
        //  the DB header max required.
        m_pfapiCurrent->SetNoFlushNeeded();
        delete m_pfapiCurrent;
        m_pfapiCurrent = NULL;
    }
    else
    {
        Error( ErrERRCheck( JET_errInvalidLogDataSequence ) );
    }

HandleError:

    m_fConsumedBegin = false;

    return err;
}

ERR LOG::ErrLGShadowLogAddData(
    JET_EMITDATACTX *   pEmitLogDataCtx,
    void *              pvLogData,
    ULONG       cbLogData )
{
    ERR err = JET_errSuccess;

    Assert( pEmitLogDataCtx );

    //  Enter the critical section so no one can cause us problems 

    m_critShadowLogConsume.Enter();

    //  Validate the emit data context ...

    if ( pEmitLogDataCtx->cbStruct != sizeof(JET_EMITDATACTX) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //  Validate we are speaking the same language

    if ( pEmitLogDataCtx->dwVersion != g_dwNativeSemiSyncVersion )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    LGPOS lgposLogData;
    lgposLogData.lGeneration    = pEmitLogDataCtx->lgposLogData.lGeneration;
    lgposLogData.isec           = pEmitLogDataCtx->lgposLogData.isec;
    lgposLogData.ib             = pEmitLogDataCtx->lgposLogData.ib;

    //  Process the operational flag

    if ( !FPowerOf2( pEmitLogDataCtx->grbitOperationalFlags ) )
    {
        //  For right now we only support one grbit at a time.
        
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }
    else if ( JET_bitShadowLogEmitFirstCall & pEmitLogDataCtx->grbitOperationalFlags )
    {
        if ( NULL == m_pshadlog )
        {
            Call( ErrLGIShadowLogInit() );
        }
        Assert( NULL != m_pshadlog );

        Call( m_pshadlog->ErrAddBegin() );
    }
    else if ( JET_bitShadowLogEmitLastCall & pEmitLogDataCtx->grbitOperationalFlags )
    {
        //  Source terminated thier log stream, so terminate our, make it ready for 3rd part to consume.
        Call( ErrLGIShadowLogTerm_() );
    }
    else if ( JET_bitShadowLogEmitDataBuffers & pEmitLogDataCtx->grbitOperationalFlags )
    {

        //  If we have not created an alternate log stream, then enduce the deferred init now to get one.

        if ( NULL == m_pshadlog )
        {
            Call( ErrLGIShadowLogInit() );
        }
        Assert( NULL != m_pshadlog );

        //  Actual add data to the log stream (and write it out)

        Call( m_pshadlog->ErrAddData( pEmitLogDataCtx->grbitOperationalFlags,
                                    pEmitLogDataCtx->qwSequenceNum,
                                    lgposLogData,
                                    pvLogData, cbLogData ) );
    }
    else if ( JET_bitShadowLogEmitLogComplete & pEmitLogDataCtx->grbitOperationalFlags )
    {
        if ( NULL == m_pshadlog )
        {
            Error( ErrERRCheck( JET_errInvalidLogDataSequence ) );
        }
        Call( m_pshadlog->ErrCompleteLog( pEmitLogDataCtx->grbitOperationalFlags,
                                    pEmitLogDataCtx->qwSequenceNum,
                                    lgposLogData ) );
    }
    else
    {
        Error( ErrERRCheck( JET_errInvalidGrbit ) );
    }

HandleError:

    m_critShadowLogConsume.Leave();

    return err;
}


//  -----------------------------------------------------------------------------------------
//      Init / Term
//  -----------------------------------------------------------------------------------------

ERR LOG::ErrLGIShadowLogInit()
{
    ERR err = JET_errSuccess;

    Assert( m_critShadowLogConsume.FOwner() );
    Call( CShadowLogStream::ErrCreate( m_pinst, &m_pshadlog ) );

HandleError:
    return err;
}

ERR LOG::ErrLGIShadowLogTerm_()
{
    Assert( m_critShadowLogConsume.FOwner() );
    Assert( m_pshadlog != NULL );

    ERR err = m_pshadlog->ErrWriteAndResetLogBuff( CShadowLogStream::ffForceResetIgnoreFailures );
    delete m_pshadlog;
    m_pshadlog = NULL;
    return err;
}

ERR LOG::ErrLGShadowLogTerm()
{
    ENTERCRITICALSECTION ecs( &m_critShadowLogConsume );
    return ErrLGIShadowLogTerm_();
}

