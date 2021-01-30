// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



#include "logstd.hxx"





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
    const DWORD             ibLogData,
    const ULONG             cbLogData,
    const BYTE * const      pbLogData
    )
{
    JET_PFNEMITLOGDATA  pfnErrEmit  = (JET_PFNEMITLOGDATA)PvParam( m_pinst, JET_paramEmitLogDataCallback );
    LONG                lGenEmit    = lgenData;



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
        return ErrERRCheck( JET_wrnCallbackNotRegistered );
    }

    if ( m_pLog->FHardRestore() )
    {
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

    emitCtx.lgposLogData.lGeneration    = lGenEmit;
    Assert( 0 != emitCtx.lgposLogData.lGeneration );
    emitCtx.lgposLogData.isec           = (USHORT) ( ibLogData / m_cbSec );
    Assert( (QWORD)emitCtx.lgposLogData.isec == ibLogData / m_cbSec );
    emitCtx.lgposLogData.ib             = (USHORT) ( ibLogData % m_cbSec );
    Assert( (QWORD)emitCtx.lgposLogData.ib == ibLogData % m_cbSec );

    emitCtx.cbLogData                   = cbLogData;

    if ( m_pEmitTraceLog != NULL )
    {
        OSTraceWriteRefLog( m_pEmitTraceLog,
                                 lGenEmit,
                                 NULL );
    }

    Ptls()->fInCallback = fTrue;

    const ERR err = (*pfnErrEmit)( (JET_INSTANCE)m_pinst, &emitCtx, (void *)pbLogData, cbLogData, pvCallBackCtx );

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

    m_qwSequence++;

    Ptls()->fInCallback = fFalse;

    return err;
}




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
    CallS( err );
    return err;

HandleError:
    delete pshadowLog;
    return err;
}

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


    QWORD cbLogFileSize = 0;
    Call( m_pfapiCurrent->ErrSize( &cbLogFileSize, IFileAPI::filesizeLogical ) );
    if ( ibOffset + cbLogData > cbLogFileSize )
    {
        Assert( FNegTestSet( fCorruptingLogFiles ) || ibOffset + cbLogData > cbLogFileSize );
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


    m_lgposCurrent.lGeneration = lgposLogData.lGeneration;
    m_lgposCurrent.isec = lgposLogData.isec + (USHORT) ( cbLogData / m_pinst->m_plog->CbLGSec() );
    m_lgposCurrent.ib = 0;


    m_qwSequenceNumCurrent++;
    return JET_errSuccess;
}

ERR CShadowLogStream::ErrResetSequence_( unsigned __int64   qwNextSequenceNumber )
{
    if ( qwNextSequenceNumber == 0 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }


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

    ULONG cbLogWrite = m_cbLogBuffPending;
    if ( m_ibLogBuffPending + cbLogWrite > m_ibExtendedFile )
    {
        cbLogWrite = roundup( cbLogWrite, (ULONG) UlParam( m_pinst, JET_paramMaxCoalesceWriteSize ) );
        cbLogWrite = min( m_cbLogBuffMax - m_ibLogBuffPending, cbLogWrite );
    }

    Call( ErrWriteLogData_( m_rgbLogBuff + m_ibLogBuffPending, m_ibLogBuffPending, cbLogWrite ) );

    m_ibExtendedFile = m_ibLogBuffPending + cbLogWrite;
    m_ibLogBuffPending += m_cbLogBuffPending;
    m_cbLogBuffPending = 0;

HandleError:
    return err;
}

ERR CShadowLogStream::ErrWriteAndResetLogBuff( WriteFlags flags  )
{
    m_critLogBuff.Enter();
    ERR err = ErrWriteLogBuff_();
    if ( !( flags & ffForceResetIgnoreFailures ) )
    {
        Call( err );
    }

    m_ibLogBuffPending = m_cbLogBuffPending = m_ibExtendedFile = 0;
    memset( m_rgbLogBuff, 0, m_cbLogBuffMax );
    Expected( FNegTestSet( fCorruptingLogFiles ) || !m_fWriteLogBuffScheduled );

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
    Assert( err == JET_errSuccess || err == JET_errDiskIO );
    pshadowLog->m_critLogBuff.Leave();
}


ERR CShadowLogStream::ErrAddBegin()
{

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
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }



    const bool bInSequencePerfect =
                qwSequenceNumber == ( m_qwSequenceNumCurrent + 1 ) &&
                m_lgposCurrent.lGeneration == lgposLogData.lGeneration &&
                m_lgposCurrent.isec == lgposLogData.isec &&
                m_lgposCurrent.ib == lgposLogData.ib;

    const bool bInSequenceSectorOverwrite =
                qwSequenceNumber == ( m_qwSequenceNumCurrent + 1 ) &&
                m_lgposCurrent.lGeneration == lgposLogData.lGeneration &&
                (   ( ( m_lgposCurrent.isec >= 1 ) && ( ( m_lgposCurrent.isec - 1 ) == lgposLogData.isec ) ) ||
                    ( ( m_lgposCurrent.isec >= 2 ) && ( ( m_lgposCurrent.isec - 2 ) == lgposLogData.isec ) )
                    ) &&
                0 == lgposLogData.ib;

    const bool bInNewSequence = FAtBeginningOfLogFile( &lgposLogData ) &&
                m_lgposCurrent.lGeneration != lgposLogData.lGeneration;

    const bool bMidSequenceFirstData = m_fConsumedBegin &&
                !FAtBeginningOfLogFile( &lgposLogData ) &&
                NULL == m_pfapiCurrent;


    if ( bInNewSequence || bMidSequenceFirstData )
    {
        WCHAR wszLogFile[ OSFSAPI_MAX_PATH ] = L"";

        Call( ErrWriteAndResetLogBuff() );


        if ( m_pfapiCurrent )
        {
            Assert( 0 != m_lgposCurrent.lGeneration );

            OSTrace( JET_tracetagLogging, OSFormat( "Alt log writer aborting log gen 0x%x, because we switched to a new log gen (0x%x) without completing the previous one.\n", m_lgposCurrent.lGeneration, lgposLogData.lGeneration ) );

            m_pfapiCurrent->SetNoFlushNeeded();
            delete m_pfapiCurrent;
            m_pfapiCurrent = NULL;


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
            wszLogFile[0] = L'\0';

            Assert( JET_errSuccess == wrn );
            wrn = ErrERRCheck( JET_wrnPreviousLogFileIncomplete );
        }

        if ( lgposLogData.isec == 0 && lgposLogData.ib == 0 )
        {

            if ( cbLogData < 512 ||
                    NULL == pvLogData )
            {
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
            C_ASSERT( OffsetOf(LGFILEHDR, lgfilehdr.le_cbSec) < (size_t)512 );

            const LGFILEHDR * const plgfilehdr = (LGFILEHDR*)pvLogData;
            if ( plgfilehdr->lgfilehdr.le_cbSec != 0 &&
                 plgfilehdr->lgfilehdr.le_cbSec != m_pinst->m_plog->CbLGSec() )
            {
                Error( ErrERRCheck( JET_errLogSectorSizeMismatch ) );
            }
        }
        else
        {
            Assert( bMidSequenceFirstData );
        }


        Call( ErrResetSequence_( qwSequenceNumber ) );


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
            Assert( lgposLogData.lGeneration > 0 );
            Call( m_pinst->m_plog->ErrLGCreateEmptyShadowLogFile( wszLogFile, lgposLogData.lGeneration ) );
        }

        Assert( lgposLogData.lGeneration > 0 );
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
            Error( ErrERRCheck( JET_errInvalidLogDataSequence ) );
        }
        Call( err )
    }


    if ( bInNewSequence || bInSequencePerfect || bInSequenceSectorOverwrite || bMidSequenceFirstData )
    {

        if ( bInSequenceSectorOverwrite )
        {


            m_lgposCurrent = lgposLogData;
        }


        ULONG ibOffset = IbFromLgpos( m_pinst, lgposLogData );
        if ( ibOffset + cbLogData > m_cbLogBuffMax )
        {
            Assert( FNegTestSet( fCorruptingLogFiles ) || ibOffset + cbLogData <= m_cbLogBuffMax );
            Error( ErrERRCheck( JET_errInvalidLogDataSequence ) );
        }

        ENTERCRITICALSECTION ecs( &m_critLogBuff );

        Assert( ibOffset == m_ibLogBuffPending + m_cbLogBuffPending || bMidSequenceFirstData || bInSequenceSectorOverwrite );

        if ( bMidSequenceFirstData )
        {
            Assert( m_ibLogBuffPending == 0 && m_cbLogBuffPending == 0 );
            m_ibLogBuffPending = ibOffset;
            m_ibExtendedFile = ibOffset;
        }

        if ( bInSequenceSectorOverwrite )
        {
            if ( m_ibLogBuffPending > ibOffset )
            {
                m_ibLogBuffPending = ibOffset;
                m_cbLogBuffPending = 0;
            }
            else
            {
                Assert( ibOffset < m_ibLogBuffPending + m_cbLogBuffPending );
                m_cbLogBuffPending = ibOffset - m_ibLogBuffPending;
            }
        }

        memcpy( m_rgbLogBuff + ibOffset, pvLogData, cbLogData );
        m_cbLogBuffPending += cbLogData;

        if ( !m_fWriteLogBuffScheduled )
        {
            OSTimerTaskScheduleTask( m_postWriteLogBuffTask, NULL, c_dtickWriteLogBuff, 0 );
            m_fWriteLogBuffScheduled = true;
        }


        Call( ErrUpdateCurrentPosition_( lgposLogData, cbLogData ) );
    }
    else
    {

        Assert( FNegTestSet( fCorruptingLogFiles ) || bInNewSequence || bInSequencePerfect || bInSequenceSectorOverwrite || bMidSequenceFirstData );
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

    if ( qwSequenceNumber == ( m_qwSequenceNumCurrent + 1 ) &&
        m_lgposCurrent.lGeneration == lgposLogData.lGeneration &&
        m_pfapiCurrent )
    {
        Assert( 0 != m_lgposCurrent.lGeneration );

        Call( ErrWriteAndResetLogBuff() );

        OSTrace( JET_tracetagLogging, OSFormat( "Alt log writer has successfully completed log gen 0x%x\n", lgposLogData.lGeneration ) );

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


    m_critShadowLogConsume.Enter();


    if ( pEmitLogDataCtx->cbStruct != sizeof(JET_EMITDATACTX) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    if ( pEmitLogDataCtx->dwVersion != g_dwNativeSemiSyncVersion )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    LGPOS lgposLogData;
    lgposLogData.lGeneration    = pEmitLogDataCtx->lgposLogData.lGeneration;
    lgposLogData.isec           = pEmitLogDataCtx->lgposLogData.isec;
    lgposLogData.ib             = pEmitLogDataCtx->lgposLogData.ib;


    if ( !FPowerOf2( pEmitLogDataCtx->grbitOperationalFlags ) )
    {
        
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
        Call( ErrLGIShadowLogTerm_() );
    }
    else if ( JET_bitShadowLogEmitDataBuffers & pEmitLogDataCtx->grbitOperationalFlags )
    {


        if ( NULL == m_pshadlog )
        {
            Call( ErrLGIShadowLogInit() );
        }
        Assert( NULL != m_pshadlog );


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

