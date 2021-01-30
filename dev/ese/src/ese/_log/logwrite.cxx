// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"

#include "..\PageSizeClean.hxx"

#ifdef PERFMON_SUPPORT

PERFInstanceLiveTotal<> cbLGGenerated;
LONG LLGBytesGeneratedCEFLPv( LONG iInstance, void *pvBuf )
{
    cbLGGenerated.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<LONG, INST, fFalse> cbLGBufferSize;
LONG LLGBufferSizeCEFLPv( LONG iInstance, void *pvBuf )
{
    cbLGBufferSize.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<LONG, INST, fFalse> cLGBufferCommitted;
LONG LLGBufferBytesCommittedCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGBufferCommitted.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<LONG, INST, fFalse> cbLGBufferUsed;
LONG LLGBufferBytesUsedCEFLPv( LONG iInstance, void *pvBuf )
{
    cbLGBufferUsed.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LLGBufferBytesFreeCEFLPv( LONG iInstance, void *pvBuf )
{
    if ( NULL != pvBuf )
    {
        *(LONG *)pvBuf = cbLGBufferSize.Get( iInstance ) - cbLGBufferUsed.Get( iInstance );
    }
    return 0;
}

PERFInstanceDelayedTotal<> cLGUsersWaiting;
LONG LLGUsersWaitingCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGUsersWaiting.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGRecord;
LONG LLGRecordCEFLPv(LONG iInstance,void *pvBuf)
{
    cLGRecord.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGCapacityWrite;
LONG LLGCapacityWriteCEFLPv(LONG iInstance,void *pvBuf)
{
    cLGCapacityWrite.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGCommitWrite;
LONG LLGCommitWriteCEFLPv(LONG iInstance,void *pvBuf)
{
    cLGCommitWrite.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGWriteSkipped;
LONG LLGWriteSkippedCEFLPv(LONG iInstance,void *pvBuf)
{
    cLGWriteSkipped.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGWriteBlocked;
LONG LLGWriteBlockedCEFLPv(LONG iInstance,void *pvBuf)
{
    cLGWriteBlocked.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGStall;
LONG LLGStallCEFLPv(LONG iInstance,void *pvBuf)
{
    cLGStall.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cbLGFileSize;
LONG LLGFileSizeCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cbLGFileSize.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cLGCheckpointTooDeep;
LONG LLGLogGenerationCheckpointDepthMaxCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cLGCheckpointTooDeep.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cLGWaypointDepth;
LONG LLGLogGenerationWaypointDepthCEFLPv( LONG iInstance, VOID *pvBuf )
{
    cLGWaypointDepth.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse> cLGLogFileGeneratedPrematurely;
LONG LLGLogFileGeneratedPrematurelyCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGLogFileGeneratedPrematurely.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<QWORD, INST, fFalse, fFalse> cLGLogFileCurrentGeneration;
LONG LLGLogFileCurrentGenerationCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGLogFileCurrentGeneration.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGFullSegmentWrite;
LONG LLGFullSegmentWriteCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGFullSegmentWrite.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cLGPartialSegmentWrite;
LONG LLGPartialSegmentWriteCEFLPv( LONG iInstance, void *pvBuf )
{
    cLGPartialSegmentWrite.PassTo( iInstance, pvBuf );
    return 0;
}

PERFInstanceDelayedTotal<> cbLGBytesWasted;
LONG LLGBytesWastedCEFLPv( LONG iInstance, void *pvBuf )
{
    cbLGBytesWasted.PassTo( iInstance, pvBuf );
    return 0;
}

#endif

LOG_WRITE_BUFFER::LOG_WRITE_BUFFER( INST * pinst, LOG * pLog, ILogStream * pLogStream, LOG_BUFFER *pLogBuffer )
    : CZeroInit( sizeof( LOG_WRITE_BUFFER ) ),
      m_pLog( pLog ),
      m_pinst( pinst ),
      m_pLogStream( pLogStream ),
      m_pLogBuffer( pLogBuffer ),
      m_fHaveShadow( fFalse ),
      m_sigLogPaused( CSyncBasicInfo( "LOG_WRITE_BUFFER::sigLogPaused" ) ),
      m_semLogSignal( CSyncBasicInfo( _T( "LOG::m_semLogSignal" ) ) ),
      m_semLogWrite( CSyncBasicInfo( _T( "LOG::m_semLogWrite" ) ) ),
      m_semWaitForLogBufferSpace( CSyncBasicInfo( _T( "LOG::m_semWaitForLogBufferSpace" ) ) ),
      m_critLGWaitQ( CLockBasicInfo( CSyncBasicInfo( szLGWaitQ ), rankLGWaitQ, 0 ) ),
      m_tickNextLazyCommit( 0 ),
      m_lgposNextLazyCommit( lgposMin ),
      m_critNextLazyCommit( CLockBasicInfo( CSyncBasicInfo( "m_critNextLazyCommit" ), rankLGLazyCommit, 0 ) ),
      m_postDecommitTask( NULL ),
      m_postLazyCommitTask( NULL ),
      m_tickDecommitTaskSchedule( TickOSTimeCurrent() ),
      m_fDecommitTaskScheduled( fFalse ),
      m_tickLastWrite( 0 )
{
    m_pvPartialSegment = NULL;
#ifdef DEBUG
    WCHAR * wsz;
    if ( ( wsz = GetDebugEnvValue ( L"TRACELOG" ) ) != NULL )
    {
        m_fDBGTraceLog = fTrue;
        delete[] wsz;
    }
    else
        m_fDBGTraceLog = fFalse;
#endif


    m_semLogSignal.Release();
    m_semLogWrite.Release();

    PERFOpt( cLGUsersWaiting.Clear( m_pinst ) );
    PERFOpt( cLGCapacityWrite.Clear( m_pinst ) );
    PERFOpt( cLGCommitWrite.Clear( m_pinst ) );
    PERFOpt( cLGStall.Clear( m_pinst ) );
    PERFOpt( cLGRecord.Clear( m_pinst ) );
    PERFOpt( cbLGGenerated.Clear( m_pinst ) );
    PERFOpt( cbLGBufferSize.Clear( m_pinst ) );
    PERFOpt( cbLGBufferUsed.Clear( m_pinst ) );
    PERFOpt( cbLGFileSize.Clear( m_pinst ) );
    PERFOpt( cLGCheckpointTooDeep.Clear( m_pinst ) );
    PERFOpt( cLGWaypointDepth.Clear( m_pinst ) );
    PERFOpt( ibLGTip.Clear( m_pinst ) );
    PERFOpt( cLGLogFileGeneratedPrematurely.Clear( m_pinst ) );
    PERFOpt( cLGLogFileCurrentGeneration.Clear( m_pinst ) );
    PERFOpt( cLGFullSegmentWrite.Clear( m_pinst ) );
    PERFOpt( cLGPartialSegmentWrite.Clear( m_pinst ) );
    PERFOpt( cLGBufferCommitted.Clear( m_pinst ) );
}

LOG_WRITE_BUFFER::~LOG_WRITE_BUFFER()
{
    PERFOpt( cLGUsersWaiting.Clear( m_pinst ) );
    PERFOpt( cLGCapacityWrite.Clear( m_pinst ) );
    PERFOpt( cLGCommitWrite.Clear( m_pinst ) );
    PERFOpt( cLGStall.Clear( m_pinst ) );
    PERFOpt( cLGRecord.Clear( m_pinst ) );
    PERFOpt( cbLGBufferSize.Clear( m_pinst ) );
    PERFOpt( cbLGGenerated.Clear( m_pinst ) );
    PERFOpt( cbLGBufferUsed.Clear( m_pinst ) );
    PERFOpt( cbLGFileSize.Clear( m_pinst ) );
    PERFOpt( cLGCheckpointTooDeep.Clear( m_pinst ) );
    PERFOpt( cLGWaypointDepth.Clear( m_pinst ) );
    PERFOpt( ibLGTip.Clear( m_pinst ) );
    PERFOpt( cLGLogFileGeneratedPrematurely.Clear( m_pinst ) );
    PERFOpt( cLGLogFileCurrentGeneration.Clear( m_pinst ) );
    PERFOpt( cLGFullSegmentWrite.Clear( m_pinst ) );
    PERFOpt( cLGPartialSegmentWrite.Clear( m_pinst ) );
    PERFOpt( cLGBufferCommitted.Clear( m_pinst ) );

    if ( m_pvPartialSegment != NULL )
    {
        OSMemoryPageFree( m_pvPartialSegment );
        m_pvPartialSegment = NULL;
    }
}

ERR
LOG_WRITE_BUFFER::ErrLGInit()
{
    ERR err;
    Alloc( m_pvPartialSegment = (BYTE *)PvOSMemoryPageAlloc( LOG_SEGMENT_SIZE, NULL ) );

HandleError:
    return err;
}

VOID SIGGetSignature( SIGNATURE *psign )
{
    LGIGetDateTime( &psign->logtimeCreate );

    UINT uiRandom;
    errno_t err = rand_s( &uiRandom );
    Assert( err == 0 );
    psign->le_ulRandom = uiRandom;
    
    memset( psign->szComputerName, 0, sizeof( psign->szComputerName ) );
}

BOOL FSIGSignSet( const SIGNATURE *psign )
{
    SIGNATURE   signNull;

    memset( &signNull, 0, sizeof(SIGNATURE) );
    return ( 0 != memcmp( psign, &signNull, sizeof(SIGNATURE) ) );
}

VOID SIGResetSignature( SIGNATURE *psign )
{
    memset( psign, 0, sizeof(SIGNATURE) );
}

VOID LOG_WRITE_BUFFER::GetLgposOfPbEntry( LGPOS *plgpos ) const
{
    m_pLogBuffer->GetLgpos( m_pbEntry, plgpos, m_pLogStream );
}

LOG_BUFFER::LOG_BUFFER() : _critLGBuf( CLockBasicInfo( CSyncBasicInfo( szLGBuf ), rankLGBuf, 0 ) )

{
}

VOID LOG_BUFFER::GetLgpos( BYTE *pb, LGPOS *plgpos, ILogStream * pLogStream ) const
{
    BYTE    *pbAligned;
    INT     csec;
    INT     isecCurrentFileEnd;

    Assert( _pbWrite != NULL );
    Assert( _pbWrite == pLogStream->PbSecAligned( _pbWrite, _pbLGBufMin ) );
    Assert( _isecWrite >= (INT)pLogStream->CSecHeader() );

    Assert( pb >= _pbLGBufMin );
    Assert( pb < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin );
    Assert( _pbWrite < _pbLGBufMax );

    pbAligned = pLogStream->PbSecAligned( pb, _pbLGBufMin );

    Assert( pbAligned >= _pbLGBufMin );
    Assert( pbAligned < _pbLGBufMax );

    plgpos->ib = USHORT( pb - pbAligned );
    if ( pbAligned < _pbWrite )
    {
        csec = _csecLGBuf - ULONG( _pbWrite - pbAligned ) / pLogStream->CbSec();
    }
    else
    {
        csec = ULONG( pbAligned - _pbWrite ) / pLogStream->CbSec();
    }

    INT isec = _isecWrite + csec;

    isecCurrentFileEnd = _isecLGFileEnd ? _isecLGFileEnd : pLogStream->CSecLGFile() - 1;
    if ( isec >= isecCurrentFileEnd )
    {
        isec = (INT) ( isec - isecCurrentFileEnd + pLogStream->CSecHeader() );
        plgpos->lGeneration = pLogStream->GetCurrentFileGen() + 1;
    }
    else
    {
        plgpos->lGeneration = pLogStream->GetCurrentFileGen();
    }

    Assert( USHORT( isec ) == isec );
    plgpos->isec = USHORT( isec );

    return;
}

BOOL LOG_BUFFER::Commit( _In_reads_( cb ) const BYTE *pb, ULONG cb )
{
    Assert ( pb >= _pbLGBufMin && pb < _pbLGBufMax );

    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
    const LONG cRFSCountdownOld = RFSThreadDisable( 10 );
    
    BOOL bPbNextToCommitted = ( _pbLGCommitStart <= _pbLGCommitEnd ?
        ( pb >= _pbLGCommitStart && pb <= _pbLGCommitEnd + 1) || ( _pbLGCommitEnd == _pbLGBufMax - 1 && pb == _pbLGBufMin )
        : pb >= _pbLGCommitStart || pb <= _pbLGCommitEnd + 1 );

    if ( !bPbNextToCommitted )
    {
        Assert( fFalse );

        pb = _pbLGBufMin;
        cb = _cbLGBuf;
        _pbLGCommitStart = _pbLGBufMin;
    }

    if ( pb + cb <= _pbLGBufMax )
    {
        if ( !FOSMemoryPageCommit( ( void * )pb, cb ) )
        {
            RFSThreadReEnable( cRFSCountdownOld );
            FOSSetCleanupState( fCleanUpStateSaved );
            return fFalse;
        }
        BYTE * pbFirstUncommitted = _pbLGBufMin + roundup( pb + cb - _pbLGBufMin, OSMemoryPageCommitGranularity() );

        if ( pbFirstUncommitted >= _pbLGCommitStart &&
            ( ( _pbLGCommitStart > _pbLGCommitEnd && pb <= _pbLGCommitEnd + 1 )
                || ( _pbLGCommitStart <= _pbLGCommitEnd && _pbLGCommitEnd == _pbLGBufMax - 1 && pb == _pbLGBufMin ) ) )
        {
            _pbLGCommitEnd = _pbLGCommitStart - 1;
        }
        else
        {
            _pbLGCommitEnd = pbFirstUncommitted - 1;
        }
    }
    else
    {
        if ( !( pb == _pbLGBufMax || FOSMemoryPageCommit( ( void * )pb, _pbLGBufMax - pb ) ) || !FOSMemoryPageCommit( _pbLGBufMin, pb + cb - _pbLGBufMax ) )
        {
            RFSThreadReEnable( cRFSCountdownOld );
            FOSSetCleanupState( fCleanUpStateSaved );
            return fFalse;
        }
        if ( _pbLGCommitStart <= _pbLGCommitEnd || pb >= _pbLGCommitStart )
        {
            BYTE *pbFirstUncommitted = _pbLGBufMin + roundup( pb + cb - _pbLGBufMax, OSMemoryPageCommitGranularity() ) - 1;
            if ( pbFirstUncommitted >= _pbLGCommitStart )
            {
                _pbLGCommitEnd = _pbLGCommitStart - 1;
            }
            else
            {
                _pbLGCommitEnd = pbFirstUncommitted;
            }
        }
        else
        {
            _pbLGCommitEnd = _pbLGCommitStart - 1;
        }
    }
    RFSThreadReEnable( cRFSCountdownOld );
    FOSSetCleanupState( fCleanUpStateSaved );
    
    return true;
}

ERR LOG_BUFFER::InitCommit( LONG cBytes )
{
    ERR err = JET_errSuccess;

    _pbLGCommitStart = _pbLGBufMin;
    _pbLGCommitEnd = _pbLGBufMin;
    if ( !Commit( _pbLGBufMin, roundup( cBytes, OSMemoryPageCommitGranularity() ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    Assert( _pbLGCommitStart = _pbLGBufMin );
    Assert( _pbLGCommitEnd = _pbLGBufMin + roundup( cBytes, OSMemoryPageCommitGranularity() ) - 1 );

HandleError:
    return err;
}

LOCAL ULONG cBytesAdditionalCommit = 16 * 1024;
BOOL LOG_BUFFER::EnsureCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb )
{
    if ( FIsCommitted( pb, cb ) )
    {
        return fTrue;
    }
    else
    {
        return Commit( pb, cb + cBytesAdditionalCommit );
    }
}

BOOL LOG_BUFFER::FIsCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb )
{
    if ( _pbLGCommitStart <= _pbLGCommitEnd )
    {
        return ( pb >= _pbLGCommitStart && pb + cb - 1 <= _pbLGCommitEnd );
    }
    else
    {
        if ( _pbLGCommitEnd == _pbLGCommitStart - 1 )
        {
            return fTrue;
        }
        if ( pb <= _pbLGCommitEnd )
        {
            return pb + cb - 1 <= _pbLGCommitEnd;
        }
        else if ( pb >= _pbLGCommitStart )
        {
            return pb + cb - 1 <= _pbLGCommitEnd + _cbLGBuf;
        }
        else
        {
            return fFalse;
        }
    }
}

LONG LOG_BUFFER::CommittedBufferSize()
{
    if ( _pbLGCommitEnd >= _pbLGCommitStart )
    {
        return LONG( _pbLGCommitEnd - _pbLGCommitStart ) + 1;
    }
    else
    {
        return _cbLGBuf + LONG( _pbLGCommitEnd - _pbLGCommitStart ) + 1;
    }
}

BOOL LOG_WRITE_BUFFER::EnsureCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb )
{
    Assert( m_critLGBuf.FOwner() );
    if ( m_pLogBuffer->EnsureCommitted( pb, cb) )
    {
        PERFOpt( cLGBufferCommitted.Set( m_pinst, m_pLogBuffer->CommittedBufferSize() ) );
        return fTrue;
    }
    else
    {
        return fFalse;
    }
}

VOID LOG_BUFFER::Decommit( _In_reads_( cb ) const BYTE *pb, ULONG cb )
{
    if ( _fReadOnly || _pbLGBufMin == NULL || _pbLGBufMax == NULL )
    {
        return;
    }
    BYTE *pbStart = _pbLGBufMin + roundup( pb - _pbLGBufMin, OSMemoryPageCommitGranularity() );
    BYTE *pbEnd = _pbLGBufMin + rounddn( pb + cb- _pbLGBufMin, OSMemoryPageCommitGranularity() );

    if ( pbEnd <= _pbLGBufMax )
    {
        if ( pbEnd > pbStart )
        {
            OSMemoryPageDecommit( pbStart, pbEnd - pbStart );
            if ( pbEnd == _pbLGBufMax )
            {
                _pbLGCommitStart = _pbLGBufMin;
            }
            else
            {
                _pbLGCommitStart = pbEnd;
            }
        }
    }
    else if ( pbEnd > _pbLGBufMax )
    {
        if ( _pbLGBufMax > pbStart )
        {
            OSMemoryPageDecommit( pbStart, _pbLGBufMax- pbStart );
        }
        OSMemoryPageDecommit( _pbLGBufMin, pbEnd - _pbLGBufMax );
        _pbLGCommitStart = _pbLGBufMin + ( pbEnd - _pbLGBufMax );
    }
    if ( ULONG( pbEnd - pbStart ) == _cbLGBuf )
    {
        _pbLGCommitEnd = _pbLGCommitStart;
    }
    else
    {
        _pbLGCommitEnd = pbStart - 1;
    }
    if ( _pbLGCommitStart == _pbLGCommitEnd )
    {
        Commit( _pbLGCommitStart, OSMemoryPageCommitGranularity() );
    }
}


VOID LOG_WRITE_BUFFER::Decommit( _In_reads_( cb ) const BYTE *pb, ULONG cb )
{
    Assert( m_critLGBuf.FOwner() );
    m_pLogBuffer->Decommit( pb, cb );
    PERFOpt( cLGBufferCommitted.Set( m_pinst, m_pLogBuffer->CommittedBufferSize() ) );
}



void LOG_BUFFER::LGTermLogBuffers()
{
    if ( _pbLGBufMin != pbNil )
    {
        OSMemoryPageFree(_pbLGBufMin);
    }
    _pbLGBufMin = pbNil;
    _pbLGBufMax = pbNil;
    _csecLGBuf  = 0;
    _cbLGBuf    = 0;
    _pbEntry    = pbNil;
    _pbWrite    = pbNil;
    _pbLGFileEnd = pbNil;
}

ERR LOG_BUFFER::ErrLGInitLogBuffers( INST *pinst, ILogStream *pLogStream, LONG csecExplicitRequest, BOOL fReadOnly )
{
    ERR         err         = JET_errSuccess;
    LONG        lLogBuffers = 0;
    const LONG  cbLogBuffersParamSize = 512;
    const LONG  csecAlign   = OSMemoryPageReserveGranularity() / pLogStream->CbSec();
    Assert ( csecAlign > 0 );
    Assert ( OSMemoryPageCommitGranularity() % pLogStream->CbSec() == 0 );
    
    if ( csecExplicitRequest )
    {
        lLogBuffers = csecExplicitRequest;
#ifdef DEBUG
#endif
    }
    else
    {
        lLogBuffers = (LONG)UlParam( pinst, JET_paramLogBuffers );

        lLogBuffers = roundup( ( lLogBuffers * cbLogBuffersParamSize ), pLogStream->CbSec() ) / pLogStream->CbSec();
    }
    Assert( lLogBuffers );


    lLogBuffers = max( lLogBuffers, LONG( (CbINSTMaxPageSize( pinst ) * 2) / pLogStream->CbSec() + 4 ) );


    lLogBuffers = roundup( lLogBuffers, csecAlign );

    if ( fReadOnly )
    {

        
        lLogBuffers = max( lLogBuffers, LONG( roundup( pLogStream->CSecLGFile(), csecAlign ) ) );
    }
    else
    {


        lLogBuffers = min( lLogBuffers, LONG( roundup( pLogStream->CSecLGFile() - ( pLogStream->CSecHeader() + 1 ), csecAlign ) ) );
    }

    if ( fReadOnly == _fReadOnly &&
         (ULONG)lLogBuffers == _csecLGBuf &&
         lLogBuffers * pLogStream->CbSec() == _cbLGBuf )
    {
        return JET_errSuccess;
    }
    
    _critLGBuf.Enter();

    LGTermLogBuffers();


    _fReadOnly  = fReadOnly;
    _csecLGBuf  = lLogBuffers;
    _cbLGBuf    = lLogBuffers * pLogStream->CbSec();

    PERFOpt( cbLGBufferSize.Set( pinst, _cbLGBuf ) );
    PERFOpt( cbLGFileSize.Set( pinst, pLogStream->CSecLGFile() * pLogStream->CbSec() ) );
    PERFOpt( cLGCheckpointTooDeep.Set( pinst, UlParam( pinst, JET_paramCheckpointTooDeep ) ) );
    PERFOpt( cLGWaypointDepth.Set( pinst, UlParam( pinst, JET_paramWaypointLatency ) ) );


    if ( ! ( _pbLGBufMin = (BYTE*)PvOSMemoryPageReserve( roundup( _cbLGBuf + OSMemoryPageCommitGranularity(), OSMemoryPageReserveGranularity() ), NULL ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    
    Expected( _cbLGBuf == rounddn( _cbLGBuf, OSMemoryPageCommitGranularity() ) );

    Assert( OSMemoryPageReserveGranularity() == rounddn( OSMemoryPageReserveGranularity(), pLogStream->CbSec() ) );
    
    _pbLGBufMax = _pbLGBufMin + _cbLGBuf;
    _pbEntry = _pbLGBufMin;
    _pbWrite = _pbLGBufMin;

    OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, pinst, (PVOID)&(pinst->m_iInstance), sizeof(pinst->m_iInstance) );

    if ( fReadOnly )
    {
        Call( InitCommit( _cbLGBuf ) );
        PERFOpt( cLGBufferCommitted.Set( pinst, _cbLGBuf ) );
    }
    else
    {
        Call( InitCommit( OSMemoryPageCommitGranularity() ) );
        PERFOpt( cLGBufferCommitted.Set( pinst, OSMemoryPageCommitGranularity() ) );
    }

HandleError:
    if ( err < 0 )
    {
        LGTermLogBuffers();
    }
    else
    {
        Assert( _cbLGBuf == ( _csecLGBuf * pLogStream->CbSec() ) );
        OSTrace( JET_tracetagLogging,
                OSFormat( "LogBuffers Initd req|param=%d|%d, m_cbLGBuf=%d = ( %d * %d ), pLogStream->CSecLGFile()=%d",
                            csecExplicitRequest, (LONG)UlParam( pinst, JET_paramLogBuffers ),
                            _cbLGBuf, _csecLGBuf, pLogStream->CbSec(), pLogStream->CSecLGFile() ) );
    }
    _critLGBuf.Leave();
    return err;
}


CGPTaskManager g_taskmgrLog;
BOOL LOG_WRITE_BUFFER::FLGSignalWrite()
{
    BOOL    fSignaled   = m_semLogSignal.FTryAcquire();

    if ( fSignaled )
    {
        ERR         err;
        const TICK  tickEnd = TickOSTimeCurrent() + cmsecWaitLogWriteMax;
        
        const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

        while ( ( err = g_taskmgrLog.ErrTMPost( (CGPTaskManager::PfnCompletion)LGWriteLog_, this ) ) < JET_errSuccess )
        {
            UtilSleep( cmsecWaitLogWrite );

            if ( TickCmp(TickOSTimeCurrent(), tickEnd) >= 0 )
            {
                Assert( err < JET_errSuccess );

                Assert( err == JET_errOutOfMemory );

                m_pLog->LGReportError( LOG_FILE_SIGNAL_ERROR_ID, err );

                m_pLog->SetNoMoreLogWrite( err );

                m_semLogSignal.Release( );
                fSignaled = fFalse;
                break;
            }
        }

        FOSSetCleanupState( fCleanUpStateSaved );
    }

    return fSignaled;
}




const BYTE* LOG_BUFFER::PbMaxEntry() const
{
#if 0
    if ( !m_pLog->FRecovering() )
    {
        Assert( m_critLGBuf.FOwner() );
    }
#endif

    Assert( _pbEntry >= _pbLGBufMin && _pbEntry < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin && _pbWrite < _pbLGBufMax );
    if ( _pbEntry >= _pbWrite )
        return _pbEntry;
    else
        return _pbEntry + _cbLGBuf;
}



const BYTE* LOG_BUFFER::PbMaxWrite() const
{
#if 0
    if ( !m_pLog->FRecovering() )
    {
        Assert( m_critLGBuf.FOwner() );
    }
#endif

    Assert( _pbEntry >= _pbLGBufMin && _pbEntry < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin && _pbWrite < _pbLGBufMax );
    if ( _pbWrite > _pbEntry )
        return _pbWrite;
    else
        return _pbWrite + _cbLGBuf;
}


const BYTE* LOG_BUFFER::PbMaxPtrForUsed( const BYTE* const pb ) const
{
    Assert( _pbEntry >= _pbLGBufMin && _pbEntry < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin && _pbWrite < _pbLGBufMax );
    Assert( pb >= _pbLGBufMin && pb < _pbLGBufMax );
    if ( pb < _pbEntry )
        return pb + _cbLGBuf;
    else
        return pb;
}


const BYTE* LOG_BUFFER::PbMaxPtrForFree( const BYTE* const pb ) const
{
    Assert( _pbEntry >= _pbLGBufMin && _pbEntry < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin && _pbWrite < _pbLGBufMax );
    Assert( pb >= _pbLGBufMin && pb < _pbLGBufMax );
    if ( pb < _pbWrite )
        return pb + _cbLGBuf;
    else
        return pb;
}


ULONG LOG_BUFFER::CbLGUsed() const
{
    return ULONG( PbMaxEntry() - _pbWrite );
}


ULONG   LOG_BUFFER::CbLGFree() const
{
    return ULONG( PbMaxWrite() - _pbEntry );
}


BOOL LOG_BUFFER::FLGIIsFreeSpace( const BYTE* const pb, ULONG cb ) const
{
    Assert( cb < _cbLGBuf );
    Assert( cb != _cbLGBuf );

    if ( cb == 0 )
        return fTrue;

    return ( CbLGFree() >= cb ) &&
        ( PbMaxPtrForUsed( pb ) >= _pbEntry && PbMaxPtrForUsed( pb ) < PbMaxWrite() ) &&
        ( PbMaxPtrForUsed( pb ) + cb > _pbEntry && PbMaxPtrForUsed( pb ) + cb <= PbMaxWrite() );
}


BOOL LOG_BUFFER::FIsFreeSpace( const BYTE* const pb, ULONG cb ) const
{
    const BOOL fResult = FLGIIsFreeSpace( pb, cb );
    Assert( fResult == ! FLGIIsUsedSpace( pb, cb ) );
    return fResult;
}


BOOL LOG_BUFFER::FLGIIsUsedSpace( const BYTE* const pb, ULONG cb ) const
{
    Assert( cb <= _cbLGBuf );

    if ( cb == 0 )
        return fFalse;

    return ( CbLGUsed() >= cb ) &&
            ( PbMaxPtrForFree( pb ) >= _pbWrite && PbMaxPtrForFree( pb ) < PbMaxEntry() ) &&
            ( PbMaxPtrForFree( pb ) + cb > _pbWrite && PbMaxPtrForFree( pb ) + cb <= PbMaxEntry() );
}


BOOL LOG_BUFFER::FIsUsedSpace( const BYTE* const pb, ULONG cb ) const
{
    const BOOL fResult = FLGIIsUsedSpace( pb, cb );
    if ( cb != _cbLGBuf )
    {
        Assert( fResult == ! FLGIIsFreeSpace( pb, cb ) );
    }
    return fResult;
}


BOOL
LOG_WRITE_BUFFER::FLGIAddLogRec(
    const DATA *        rgdata,
    const INT           cdata,
    BOOL                fAllocOnly,
    __deref BYTE**      ppbET )
{
    BYTE *pbET = *ppbET;
    INT idataStart = 0;

    Assert( pbET >= m_pbLGBufMin );
    Assert( pbET < m_pbLGBufMax );

    BYTE * pbWrite          = m_pbWrite;

    Assert( pbWrite >= m_pbLGBufMin );
    Assert( pbWrite < m_pbLGBufMax );

    BYTE * pbWrapWrite      = ( pbWrite <= pbET ? ( pbWrite + m_cbLGBuf ) : pbWrite );

    while ( idataStart < cdata )
    {
        INT idataEnd = idataStart;
        INT cbNeeded = 0;
        INT cbRecordLeft = 0;

        do
        {
            if ( cbRecordLeft == 0 &&
                 rgdata[idataEnd].Cb() != 0 )
            {
                cbRecordLeft = CbLGSizeOfRec( (LR *)rgdata[idataEnd].Pv() );
            }

            cbNeeded += rgdata[idataEnd].Cb();

            Assert( cbRecordLeft >= rgdata[idataEnd].Cb() );
            cbRecordLeft -= rgdata[idataEnd].Cb();
            idataEnd++;
        }
        while ( idataEnd < cdata &&
                ( cbRecordLeft > 0 || rgdata[idataEnd].Cb() == 0 ) );

        Assert( cbRecordLeft == 0 );

        if ( cbNeeded >= m_pbLGBufMax - m_pbLGBufMin )
        {
            Assert( fAllocOnly );
            return fFalse;
        }

        BOOL fFragmented = fFalse;
        INT dataOffset = 0;
        while ( cbNeeded != 0 )
        {
            if ( pbET + cbNeeded >= pbWrapWrite )
            {
                Assert( fAllocOnly );
                return fFalse;
            }

            INT cbSegRemain = m_pLogStream->CbSec() - ( ( pbET - m_pbLGBufMin ) % m_pLogStream->CbSec() );
            if ( cbSegRemain == (INT)m_pLogStream->CbSec() )
            {
                cbSegRemain -= sizeof( LGSEGHDR );
                if ( !fAllocOnly )
                {
                    LGIGetDateTime( &((LGSEGHDR *)pbET)->logtimeSegmentStart );
                }
                pbET += sizeof( LGSEGHDR );
            }
            Assert( cbSegRemain <= (INT)( m_pLogStream->CbSec() - sizeof( LGSEGHDR ) ) );
            if ( !fFragmented )
            {
                if ( cbSegRemain < cbNeeded &&
                     cbSegRemain < LOG_MIN_FRAGMENT_SIZE )
                {
                    if ( !fAllocOnly )
                    {
                        memset( pbET, lrtypNOP, cbSegRemain );
                    }
                    pbET += cbSegRemain;
                    if ( pbET >= pbWrapWrite )
                    {
                        Assert( fAllocOnly );
                        return fFalse;
                    }
                    if ( pbET >= m_pbLGBufMax )
                    {
                        Assert( pbET < m_pbLGBufMax + m_cbLGBuf );
                        pbET -= m_cbLGBuf;
                        pbWrapWrite = pbWrite;
                    }
                    if ( !fAllocOnly )
                    {
                        LGIGetDateTime( &((LGSEGHDR *)pbET)->logtimeSegmentStart );
                    }
                    pbET += sizeof( LGSEGHDR );
                    cbSegRemain = m_pLogStream->CbSec() - sizeof( LGSEGHDR );
                }

                if ( cbSegRemain < cbNeeded )
                {
                    Assert( cbSegRemain >= sizeof( LRFRAGMENTBEGIN ) );

                    fFragmented = fTrue;

                    if ( !fAllocOnly )
                    {
                        LRFRAGMENTBEGIN lrBegin;
                        lrBegin.le_cbTotalLRSize = cbNeeded;
                        lrBegin.le_cbFragmentSize = (USHORT)( cbSegRemain - sizeof( lrBegin ) );
                        UtilMemCpy( pbET, &lrBegin, sizeof( lrBegin ) );
                    }
                    pbET += sizeof( LRFRAGMENTBEGIN );
                    cbSegRemain -= sizeof( LRFRAGMENTBEGIN );
                }
            }
            else 
            {
                Assert( cbSegRemain == (INT)( m_pLogStream->CbSec() - sizeof( LGSEGHDR ) ) );

                if ( !fAllocOnly )
                {
                    LRFRAGMENT lrFrag;
                    lrFrag.le_cbFragmentSize = (USHORT)min( cbNeeded, (INT)( cbSegRemain - sizeof( lrFrag ) ) );
                    UtilMemCpy( pbET, &lrFrag, sizeof( lrFrag ) );
                }
                pbET += sizeof( LRFRAGMENT );
                cbSegRemain -= sizeof( LRFRAGMENT );
            }

            while ( cbNeeded != 0 && cbSegRemain != 0 )
            {
                DWORD cbToCopy = min( rgdata[idataStart].Cb() - dataOffset, cbSegRemain );
                if ( !fAllocOnly )
                {
                    UtilMemCpy( pbET, (BYTE *)rgdata[idataStart].Pv() + dataOffset, cbToCopy );
                }
                pbET += cbToCopy;
                cbNeeded -= cbToCopy;
                cbSegRemain -= cbToCopy;
                dataOffset += cbToCopy;
                if ( dataOffset == rgdata[idataStart].Cb() )
                {
                    idataStart++;
                    dataOffset = 0;
                }
            }

            if ( pbET >= m_pbLGBufMax )
            {
                Assert( pbET < m_pbLGBufMax + m_cbLGBuf );
                pbET -= m_cbLGBuf;
                pbWrapWrite = pbWrite;
            }
        }

        idataStart = idataEnd;
    }

    Assert( pbET >= m_pbLGBufMin );
    Assert( pbET < m_pbLGBufMax );
    Assert( pbET <= pbWrapWrite );
    if ( pbET == pbWrapWrite )
    {
        Assert( fAllocOnly );
        return fFalse;
    }
    *ppbET = pbET;
    return fTrue;
}



#ifdef DEBUG
BYTE g_rgbDumpLogRec[ g_cbPageMax ];
#endif

ERR
LOG_WRITE_BUFFER::ErrLGLogRec(
    const DATA *rgdata,
    const INT cdata,
    const BOOL fLGFlags,
    const LONG lgenBegin0,
    LGPOS *plgposLogRec )
{
    ERR         err                 = JET_errSuccess;
    DWORD       cbReq;
    BOOL        fNewGen             = ( fLGFlags & fLGCreateNewGen );
    const BOOL  fStopOnNewGen       = ( fNewGen && ( fLGFlags & fLGStopOnNewGen ) );
    BYTE*       pbSectorBoundary;
    BOOL        fFormatJetTmpLog    = fFalse;
    BYTE*       pbLogRec            = pbNil;
    BOOL        fForcedNewGen       = fFalse;
    LGPOS       lgposLogRecTmpLog   = lgposMax;

    Assert( ( fLGFlags & fLGFillPartialSector ) || rgdata[0].Pv() != NULL );
    Expected( !(( fLGFlags & fLGFillPartialSector ) && ( fLGFlags & fLGCreateNewGen )) );
    Expected( !(( fLGFlags & fLGFillPartialSector ) && ( fLGFlags & fLGStopOnNewGen )) );

    Assert( m_pLog->FLogDisabled() == fFalse );

    if ( m_pLog->FLogDisabledDueToRecoveryFailure() )
    {
        return ErrERRCheck( JET_errLogDisabledDueToRecoveryFailure );
    }

    Assert( !m_pLog->FRecovering() || fRecoveringRedo != m_pLog->FRecoveringMode() );

    if ( fRecoveringUndo == m_pLog->FRecoveringMode() &&
         !m_pLog->FRecoveryUndoLogged() )
    {
        Assert( *(LRTYP *)rgdata[0].Pv() == lrtypRecoveryUndo2 );
    }

    Assert( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) );
    Assert( m_cbLGBuf / m_csecLGBuf == m_pLogStream->CbSec() );
    Assert( m_pLogStream->CbSec() == LOG_SEGMENT_SIZE );

    forever
    {
        DWORD   ibEntry;
        DWORD   csecReady;
        DWORD   cbAvail;
        DWORD   csecToExclude;
        DWORD   cbFraction;
        DWORD   csecToAdd;
        DWORD   isecLGFileEndT = 0;

        m_critLGBuf.Enter();
        ERR errNoMoreWrite = JET_errSuccess;
        if ( m_pLog->FNoMoreLogWrite( &errNoMoreWrite ) &&
             ( !( fLGFlags & fLGFillPartialSector ) || errNoMoreWrite != errLogServiceStopped || m_pLogStream->FLogEndEmitted() ) )
        {
            m_critLGBuf.Leave();
            return ErrERRCheck( JET_errLogWriteFail );
        }

        const LRTYP lrtyp = rgdata ? *( (LRTYP *)rgdata[0].Pv() ) : lrtypNOP;
        if ( lrtyp == lrtypCommit0 )
        {
            LRCOMMIT0 *plrCommit0 = (LRCOMMIT0 *)rgdata[0].Pv();
            plrCommit0->le_trxCommit0 = TRX( AtomicExchangeAdd( (LONG *)&m_pinst->m_trxNewest, TRXID_INCR ) ) + TRXID_INCR;
        }

        if ( !m_pLog->FRecovering() )
        {
            const LONG  lgenTooDeepLimit = (LONG)UlParam( m_pinst, JET_paramCheckpointTooDeep );
            const LONG  lgenCurr        = m_pLogStream->GetCurrentFileGen();
            const LONG  lgenCheckpoint  = m_pLog->LgposGetCheckpoint().le_lGeneration;
            const LONG  lgenOutstanding = lgenCurr - lgenCheckpoint;
            const LONG  lgenTrxOutstanding = ( lgenBegin0 == lgposMax.lGeneration || lgenBegin0 == lgposMin.lGeneration ) ? 0 : ( lgenCurr - lgenBegin0 );

            if ( lgenOutstanding > ( lgenTooDeepLimit - lgenCheckpointTooDeepMin / 2 ) ||
                 ( lgenTrxOutstanding > lgenTooDeepLimit / 2 && !( fLGFlags & fLGMacroGoing ) ) )
            {
                switch ( lrtyp )
                {
                    case lrtypInit2:
                        break;
                        
                    case lrtypCommit0:
                    case lrtypRollback:
                    case lrtypUndo:
                    case lrtypUndoInfo:
                    case lrtypShutDownMark:
                    case lrtypTerm2:
                    case lrtypTrace:
                    case lrtypNOP:
                    case lrtypRecoveryUndo2:
                    case lrtypRecoveryQuit2:
                    case lrtypRecoveryQuitKeepAttachments:
                    case lrtypMacroAbort:
                    case lrtypDetachDB:
                    case lrtypForceLogRollover:
                        if ( lgenOutstanding <= ( lgenTooDeepLimit - lgenCheckpointTooDeepMin / 4 ) )
                        {
                            break;
                        }


                    default:
                        m_critLGBuf.Leave();
                        if ( lgenOutstanding > ( lgenTooDeepLimit - lgenCheckpointTooDeepMin / 2 ) )
                        {
                            return ErrERRCheck( JET_errCheckpointDepthTooDeep );
                        }
                        else
                        {
                            return ErrERRCheck( JET_errTransactionTooLong );
                        }
                }
            }
        }


        Assert( m_pbWrite >= m_pbLGBufMin );
        Assert( m_pbWrite < m_pbLGBufMax );
        Assert( m_pbEntry >= m_pbLGBufMin );
        Assert( m_pbEntry < m_pbLGBufMax );

        if ( m_pbLGFileEnd != pbNil )
        {
            Assert( m_isecLGFileEnd != 0 );
            if ( fNewGen && !fForcedNewGen )
            {
                goto Restart;
            }
            else if ( m_fWaitForLogRecordAfterStopOnNewGen && !fStopOnNewGen )
            {
                goto Restart;
            }
        }

        if ( m_pbWrite > m_pbEntry )
        {
            cbAvail = ULONG( m_pbWrite - m_pbEntry );
        }
        else
        {
            cbAvail = ULONG( m_cbLGBuf + m_pbWrite - m_pbEntry );
        }

        Assert( cbAvail <= m_cbLGBuf );


        csecToExclude = ( cbAvail - 1 ) / m_pLogStream->CbSec() + 1;
        csecReady = m_csecLGBuf - csecToExclude;
        ibEntry = ULONG( m_pbEntry - m_pLogStream->PbSecAligned( m_pbEntry, m_pbLGBufMin ) );

        if ( !fNewGen || fForcedNewGen )
        {

            pbLogRec = m_pbEntry;
            if ( fLGFlags & fLGFillPartialSector )
            {
                cbReq = ( m_pLogStream->CbSec() - ibEntry ) % m_pLogStream->CbSec();
                if ( cbReq == 0 ||
                     cbReq >= cbAvail ||
                     cbReq < sizeof( LRIGNORED ) )
                {
                    m_critLGBuf.Leave();
                    return JET_errSuccess;
                }
                pbLogRec += cbReq;
                if ( pbLogRec >= m_pbLGBufMax )
                {
                    Assert( pbLogRec == m_pbLGBufMax );
                    pbLogRec -= m_cbLGBuf;
                }
            }
            else if ( !FLGIAddLogRec( rgdata, cdata, fTrue, &pbLogRec ) )
            {
                LGPOS lgposEndOfData = lgposMin;
                GetLgposOfPbEntry( &lgposEndOfData );
                if ( CmpLgpos( &lgposEndOfData, &m_lgposToWrite ) <= 0 )
                {
#ifdef DEBUG
                    LONG cbData = 0;
                    for ( INT idata = 0; idata < cdata; idata++ )
                    {
                        cbData += rgdata[idata].Cb();
                        Assert( cbData > 0 );
                    }

                    const LONG csecAvailMax = m_pLog->CSecLGFile() - m_pLog->CSecLGHeader() - 4 ;
                    Assert( csecAvailMax > 0 );

                    Assert( cbData > ( csecAvailMax * (LONG)m_pLog->CbLGSec() ) );
#endif

                    m_critLGBuf.Leave();
                    return ErrERRCheck( JET_errLogBufferTooSmall );
                }

                goto Restart;
            }
            if ( pbLogRec > m_pbEntry )
            {
                cbReq = DWORD( pbLogRec - m_pbEntry );
            }
            else
            {
                cbReq = DWORD( m_cbLGBuf + pbLogRec - m_pbEntry );
            }
        
            if ( m_pbLGFileEnd != pbNil )
            {
                Assert( m_isecLGFileEnd != 0 );
                DWORD cbFileUsed;
                if ( m_pbLGFileEnd < pbLogRec )
                {
                    cbFileUsed = DWORD( pbLogRec - m_pbLGFileEnd );
                }
                else
                {
                    cbFileUsed = DWORD( m_cbLGBuf + pbLogRec - m_pbLGFileEnd );
                }
                if ( cbFileUsed >= ( m_pLogStream->CSecLGFile() - 1 - m_pLogStream->CSecHeader() ) * m_pLogStream->CbSec() )
                {
                    goto Restart;
                }

                if  ( FLGSignalWrite() )
                {
                    PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
                }

                break;
            }



            cbFraction = ibEntry + cbReq;
            csecToAdd = ( cbFraction + m_pLogStream->CbSec() ) / m_pLogStream->CbSec();
            Assert( csecToAdd <= 0x7FFFFFFF );

            if ( csecReady + m_isecWrite + csecToAdd > m_pLogStream->CSecLGFile() - 1 )
            {
                fNewGen = fTrue;
            }
            else
            {
                if ( csecReady >= m_csecLGBuf / 2 )
                {
                    if  ( FLGSignalWrite() )
                    {
                        PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
                    }
                }

                break;
            }
        }

        Assert( fNewGen );

        cbFraction = ibEntry;
        csecToAdd = ( cbFraction + m_pLogStream->CbSec() - 1 ) / m_pLogStream->CbSec();
        Assert( csecToAdd == 0 || csecToAdd == 1 );
        if ( csecReady == 0 && csecToAdd == 0 )
        {
            csecToAdd = 1;
        }
        isecLGFileEndT = m_isecWrite + csecReady + csecToAdd;
        Assert( isecLGFileEndT < m_pLogStream->CSecLGFile() );

        DWORD cbToFill = csecToAdd * m_pLogStream->CbSec() - ibEntry;





        if ( cbAvail <= cbToFill )
        {
            goto Restart;
        }

        if ( cbToFill > 0 )
        {

            pbLogRec = m_pbEntry;

            Assert( m_pbEntry >= m_pbLGBufMin );
            Assert( m_pbEntry < m_pbLGBufMax );
            Assert( m_pLogBuffer->FIsFreeSpace( m_pbEntry, cbToFill ) );

            if ( !EnsureCommitted( m_pbEntry, cbToFill ) )
            {
                goto Restart;
            }

            memset( m_pbEntry, lrtypNOP, cbToFill );
            
            m_pbEntry += cbToFill;
            if ( m_pbEntry >= m_pbLGBufMax )
            {
                m_pbEntry -= m_cbLGBuf;
            }
            Assert( m_pbEntry >= m_pbLGBufMin );
            Assert( m_pbEntry < m_pbLGBufMax );
            Assert( m_pbEntry == m_pLogStream->PbSecAligned( m_pbEntry, m_pbLGBufMin ) );
            Assert( m_pLogBuffer->FIsUsedSpace( pbLogRec, cbToFill ) );
        }

        Assert( pbNil == m_pbLGFileEnd );
        m_pbLGFileEnd = m_pbEntry;
        Assert( m_pLogStream->PbSecAligned( m_pbLGFileEnd, m_pbLGBufMin ) == m_pbLGFileEnd );
        m_isecLGFileEnd = isecLGFileEndT;

        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );

        Assert( !m_fStopOnNewLogGeneration );
        Assert( !m_fWaitForLogRecordAfterStopOnNewGen );
        if ( fStopOnNewGen )
        {
            m_fStopOnNewLogGeneration = fTrue;
            m_fWaitForLogRecordAfterStopOnNewGen = fTrue;

        }


        if ( FLGSignalWrite() )
        {
            PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
        }

        m_critLGBuf.Leave();

        if ( fStopOnNewGen )
        {
            while ( m_fStopOnNewLogGeneration && !m_pLog->FNoMoreLogWrite() )
            {
                UtilSleep( cmsecWaitLogWrite );
                (void)FLGWriteLog( iorpLGWriteNewGen, fFalse, fFalse );
            }
        }

        fForcedNewGen = fTrue;

    }

    GetLgposOfPbEntry( &m_lgposLogRec );
    if ( CmpLgpos( m_lgposLogRecBasis, lgposMin ) == 0 )
    {
        m_lgposLogRecBasis = m_lgposLogRec;
    }

    if ( fLGFlags & fLGCreateNewGen )
    {
        PERFOpt( cLGLogFileGeneratedPrematurely.Inc( m_pinst ) );
    }

    if ( !EnsureCommitted( m_pbEntry, cbReq ) )
    {
        goto Restart;
    }

#ifdef DEBUG
    Assert( CmpLgpos( &m_lgposLastLogRec, &m_lgposLogRec ) < 0 );
    m_lgposLastLogRec = m_lgposLogRec;
#endif

    if ( plgposLogRec )
        *plgposLogRec = m_lgposLogRec;

    Assert( m_pbEntry >= m_pbLGBufMin );
    Assert( m_pbEntry < m_pbLGBufMax );




    pbSectorBoundary = m_pLogStream->PbSecAligned( m_pbEntry + cbReq, m_pbLGBufMin );

    if ( m_pLogStream->PbSecAligned( m_pbEntry, m_pbLGBufMin ) == pbSectorBoundary )
    {
    }
    else
    {

#ifdef DEBUG
        LGPOS lgposOldMaxWritePoint = m_lgposMaxWritePoint;
#endif

        GetLgposOfPbEntry( &m_lgposMaxWritePoint );
        if ( pbSectorBoundary == m_pbEntry + cbReq )
        {
            m_pLogStream->AddLgpos( &m_lgposMaxWritePoint, cbReq );
        }

        Assert( CmpLgpos( &m_lgposMaxWritePoint, &m_lgposLogRec ) >= 0 );

        Assert( CmpLgpos( &lgposOldMaxWritePoint, &m_lgposMaxWritePoint ) <= 0 );

#ifdef DEBUG

        LGPOS lgposLogRecEndT = m_lgposLogRec;
        m_pLogStream->AddLgpos( &lgposLogRecEndT, cbReq );
        Assert( CmpLgpos( &m_lgposMaxWritePoint, &m_lgposLogRec ) == 0 ||
                CmpLgpos( &m_lgposMaxWritePoint, &lgposLogRecEndT ) == 0 );
#endif
    }

    pbLogRec = m_pbEntry;
    Assert( m_pLogBuffer->FIsFreeSpace( m_pbEntry, cbReq ) );

    m_pbEntry += cbReq;
    if ( m_pbEntry >= m_pbLGBufMax )
    {
        m_pbEntry -= m_cbLGBuf;
    }
    Assert( m_pLogBuffer->FIsUsedSpace( pbLogRec, cbReq ) );


    Assert( m_pbEntry < m_pbLGBufMax );
    Assert( m_pbEntry >= m_pbLGBufMin );

    PERFOpt( cbLGBufferUsed.Set(
                    m_pinst,
                    LONG( m_pbWrite <= m_pbEntry ?
                                m_pbEntry - m_pbWrite :
                                m_pbEntry + m_cbLGBuf - m_pbWrite ) ) );

#ifdef DEBUG
    if ( m_fDBGTraceLog )
    {
        DWORD dwCurrentThreadId = DwUtilThreadId();
        BYTE *pb;

        g_critDBGPrint.Enter();


        pb = g_rgbDumpLogRec;
        for ( INT idata = 0; idata < cdata; idata++ )
        {
            UtilMemCpy( pb, rgdata[idata].Pv(), rgdata[idata].Cb() );
            pb += rgdata[idata].Cb();
        }

        LR  *plr    = (LR *)g_rgbDumpLogRec;

        Assert( lrtypNOP != plr->lrtyp );
        Assert( 0 == m_pLog->GetNOP() );

        if ( dwCurrentThreadId == m_dwDBGLogThreadId )
            DBGprintf( "$");
        else if ( FLGDebugLogRec( plr ) )
            DBGprintf( "#");
        else
            DBGprintf( "<");

        DBGprintf( " {%u} %u,%u,%u",
                    dwCurrentThreadId,
                    m_lgposLogRec.lGeneration,
                    m_lgposLogRec.isec,
                    m_lgposLogRec.ib );
        ShowLR( plr, m_pLog );

        g_critDBGPrint.Leave();
    }
#endif


    PERFOpt( cLGRecord.Inc( m_pinst ) );

    LGPOS lgposLogRecNext;
    GetLgposOfPbEntry( &lgposLogRecNext );
    PERFOpt( cbLGGenerated.Add( m_pinst, LONG( m_pLogStream->CbOffsetLgpos( lgposLogRecNext, m_lgposLogRecBasis ) ) ) );
    m_lgposLogRecBasis = lgposLogRecNext;

    PERFOpt( ibLGTip.Set( m_pinst, m_pLogStream->CbOffsetLgpos( lgposLogRecNext, lgposMin ) ) );

    fFormatJetTmpLog = m_pLogStream->FShouldCreateTmpLog( m_lgposLogRec );
    if ( fFormatJetTmpLog )
    {
        lgposLogRecTmpLog = m_lgposLogRec;
    }

    const INT   iGroup  = m_msLGPendingCopyIntoBuffer.Enter();

    if ( fLGFlags & fLGFillPartialSector )
    {
        m_pLog->LGAddWastage( cbReq );
    }
    else
    {
        m_pLog->LGAddUsage( cbReq );
    }

    m_critLGBuf.Leave();

    if ( fLGFlags & fLGFillPartialSector )
    {
        Enforce( cbReq >= sizeof( LRIGNORED ) );
        LRIGNORED lrNop( lrtypNOP2 );
        lrNop.SetCb( cbReq - sizeof( LRIGNORED ) );
        memcpy( pbLogRec, &lrNop, sizeof( LRIGNORED ) );
        memset( pbLogRec + sizeof( LRIGNORED ), 0, cbReq - sizeof( LRIGNORED ) );
        PERFOpt( cbLGBytesWasted.Add( m_pinst, cbReq ) );
    }
    else
    {
        const BOOL fCopiedLR = FLGIAddLogRec( rgdata, cdata, fFalse, &pbLogRec );
        Enforce( fCopiedLR );
    }

    m_msLGPendingCopyIntoBuffer.Leave( iGroup );

    TLS* ptls;
    ptls = Ptls();
    ptls->threadstats.cLogRecord++;
    ptls->threadstats.cbLogRecord += (ULONG)cbReq;

    OSTrace( JET_tracetagLogging, OSFormat( "Logged %d (0x%x) bytes", cbReq, cbReq ) );

FormatJetTmpLog:
    Assert( m_critLGBuf.FNotOwner() );

    if ( fStopOnNewGen )
    {
        m_critLGBuf.Enter();
        m_fWaitForLogRecordAfterStopOnNewGen = fFalse;
        m_critLGBuf.Leave();
    }

    if ( fFormatJetTmpLog )
    {
        m_pLogStream->LGCreateAsynchIOIssue( lgposLogRecTmpLog );
    }

    if ( fStopOnNewGen )
    {
        Assert( !m_fWaitForLogRecordAfterStopOnNewGen );
    }

    return err;

Restart:
    if ( FLGSignalWrite() )
    {
        PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
    }

    fFormatJetTmpLog = m_pLogStream->FShouldCreateTmpLog( m_lgposLogRec );
    if ( fFormatJetTmpLog )
    {
        lgposLogRecTmpLog = m_lgposLogRec;
    }

    Assert( !fStopOnNewGen || !m_fWaitForLogRecordAfterStopOnNewGen );

    m_critLGBuf.Leave();

    PERFOpt( cLGStall.Inc( m_pinst ) );
    ETLogStall();
    err = ErrERRCheck( errLGNotSynchronous );
    goto FormatJetTmpLog;
}

ERR LOG_WRITE_BUFFER::ErrLGWaitForLogGen( const LONG lGeneration )
{
    
    while ( lGeneration > m_pLogStream->GetCurrentFileGen() )
    {
        if ( m_pLog->FNoMoreLogWrite() )
        {
            return( ErrERRCheck( JET_errLogWriteFail ) );
        }
        FLGSignalWrite();
        UtilSleep( cmsecWaitGeneric );
    }

    return JET_errSuccess;
}

ERR LOG_WRITE_BUFFER::ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec )
{
    ERR     err         = JET_errSuccess;
    BOOL    fFillPartialSector = fFalse;


    if ( m_pLog->FLogDisabled() || m_pLog->FRecovering() && m_pLog->FRecoveringMode() != fRecoveringUndo )
    {
        goto Done;
    }


    m_critLGBuf.Enter();
    if (    (   CmpLgpos( plgposLogRec, &lgposMax ) == 0 &&
                CmpLgpos( &m_lgposLogRec, &m_lgposToWrite ) < 0 ) ||
            (   CmpLgpos( plgposLogRec, &lgposMax ) != 0 &&
                CmpLgpos( plgposLogRec, &m_lgposToWrite ) < 0 ) )
    {
        m_critLGBuf.Leave();
        PERFOpt( cLGWriteSkipped.Inc( m_pinst ) );
        goto Done;
    }


    if ( m_pLog->FNoMoreLogWrite() )
    {
        err = ErrERRCheck( JET_errLogWriteFail );
        m_critLGBuf.Leave();
        goto Done;
    }


    m_critLGWaitQ.Enter();
    PERFOpt( cLGUsersWaiting.Inc( m_pinst ) );

    Assert( !ppib->FLGWaiting() );
    ppib->SetFLGWaiting();
    if ( CmpLgpos( plgposLogRec, &lgposMax ) == 0 )
    {
        ppib->lgposWaitLogWrite = m_lgposLogRec;
    }
    else
    {
        ppib->lgposWaitLogWrite = *plgposLogRec;
    }
    ppib->ppibNextWaitWrite = ppibNil;

    if ( m_ppibLGWriteQHead == ppibNil )
    {
        m_ppibLGWriteQTail = m_ppibLGWriteQHead = ppib;
        ppib->ppibPrevWaitWrite = ppibNil;
    }
    else
    {
        Assert( m_ppibLGWriteQTail != ppibNil );
        ppib->ppibPrevWaitWrite = m_ppibLGWriteQTail;
        m_ppibLGWriteQTail->ppibNextWaitWrite = ppib;
        m_ppibLGWriteQTail = ppib;
    }

    if ( pbNil == m_pbLGFileEnd )
    {
        LGPOS lgposEndOfData = lgposMin;
        GetLgposOfPbEntry( &lgposEndOfData );
        fFillPartialSector =
            ( lgposEndOfData.lGeneration == plgposLogRec->lGeneration &&
              lgposEndOfData.isec == plgposLogRec->isec );
    }

    m_critLGWaitQ.Leave();
    m_critLGBuf.Leave();

    if ( fFillPartialSector )
    {
        (VOID)ErrLGLogRec( NULL, 0, fLGFillPartialSector, 0, NULL );
    }

    if ( ppib->asigWaitLogWrite.FTryWait() )
    {
        PERFOpt( cLGWriteSkipped.Inc( m_pinst ) );
    }
    else
    {
        BOOL    fWrittenSelf = fFalse;
        do
        {
            if ( FLGWriteLog( iorpLGWriteCommit, fFalse, fTrue ) )
            {
                PERFOpt( cLGCommitWrite.Inc( m_pinst ) );
                fWrittenSelf = fTrue;
            }
        }
        while ( !ppib->asigWaitLogWrite.FWait( cmsecWaitLogWrite ) );
        if ( !fWrittenSelf )
        {
            PERFOpt( cLGWriteBlocked.Inc( m_pinst ) );
        }
    }


    if ( m_pLog->FNoMoreLogWrite() )
    {
        err = ErrERRCheck( JET_errLogWriteFail );
    }


    else
    {
#ifdef DEBUG


        m_critLGBuf.Enter();
        Assert( CmpLgpos( &m_lgposToWrite, &ppib->lgposWaitLogWrite ) > 0 );
        m_critLGBuf.Leave();

#endif
    }

Done:
    Assert( !ppib->FLGWaiting() );
    Assert( err == JET_errLogWriteFail || err == JET_errSuccess );

    return err;
}

VOID
LOG_WRITE_BUFFER::VerifyAllWritten()
{
    m_pLogStream->LockWrite();
    m_critLGBuf.Enter();


    LGPOS lgposEndOfData;
    GetLgposOfPbEntry( &lgposEndOfData );
    Assert( CmpLgpos( &lgposEndOfData, &m_lgposToWrite ) <= 0 );

    m_critLGBuf.Leave();
    m_pLogStream->UnlockWrite();
}


ERR LOG_WRITE_BUFFER::ErrLGWaitAllFlushed( BOOL fFillPartialSector )
{
    forever
    {
        LGPOS   lgposEndOfData;
        INT     cmp;

        m_critLGBuf.Enter();


        GetLgposOfPbEntry ( &lgposEndOfData );
        cmp = CmpLgpos( &lgposEndOfData, &m_lgposToWrite );
        fFillPartialSector = fFillPartialSector && ( lgposEndOfData.ib != 0 );

        m_critLGBuf.Leave();

        if ( cmp > 0 )
        {
            if ( fFillPartialSector )
            {
                (VOID)ErrLGLogRec( NULL, 0, fLGFillPartialSector, 0, NULL );
            }

            ERR err = ErrLGWriteLog( iorpLGFlushAll, fTrue );
            if ( err < 0 )
            {
                return err;
            }
        }
        else
        {
            break;
        }
    }

    m_pLogStream->LGAssertFullyFlushedBuffers();

    return JET_errSuccess;
}

VOID LOG_WRITE_BUFFER::AdvanceLgposToWriteToNewGen( const LONG lGeneration )
{
    Assert( m_critLGBuf.FOwner() );
    m_lgposToWrite.lGeneration = lGeneration;
    m_lgposToWrite.isec = (WORD) m_pLogStream->CSecHeader();
    m_lgposToWrite.ib = 0;
    if ( CmpLgpos( &m_lgposMaxWritePoint, &m_lgposToWrite ) < 0 ||
         FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) )
    {
        m_lgposMaxWritePoint = m_lgposToWrite;
    }
}

#ifdef DEBUG
LOCAL LONG g_dtickNoWriteInterval = 0;
LOCAL LONG g_dtickDecommitTaskDelay = 200;
#else
LOCAL LONG g_dtickNoWriteInterval = 1000;
LOCAL LONG g_dtickDecommitTaskDelay = 2000;
#endif

VOID LOG_WRITE_BUFFER::DecommitLogBufferTask( VOID * pvGroupContext, VOID * pvLogWriteBuffer )
{
    LOG_WRITE_BUFFER *pLogWriteBuffer = (LOG_WRITE_BUFFER *) pvLogWriteBuffer;
    BOOL bShouldDecommit = fFalse;
    
    pLogWriteBuffer->m_critLGWaitQ.Enter();
    if ( TickCmp( TickOSTimeCurrent(), pLogWriteBuffer->m_tickLastWrite ) < g_dtickNoWriteInterval )
    {
        OSTimerTaskScheduleTask( pLogWriteBuffer->m_postDecommitTask, pLogWriteBuffer, g_dtickDecommitTaskDelay, g_dtickDecommitTaskDelay );
    }
    else
    {
        bShouldDecommit = fTrue;
        pLogWriteBuffer->m_fDecommitTaskScheduled = fFalse;
    }
    pLogWriteBuffer->m_critLGWaitQ.Leave();
    
    if ( bShouldDecommit )
    {
        ULONG lBytesToDecommit = 0;
        pLogWriteBuffer->m_pLogBuffer->_critLGBuf.Enter();
        if ( pLogWriteBuffer->m_pbEntry >= pLogWriteBuffer->m_pbWrite )
        {
            lBytesToDecommit = pLogWriteBuffer->m_cbLGBuf - ULONG( pLogWriteBuffer->m_pbEntry - pLogWriteBuffer->m_pbWrite );
        }
        else
        {
            lBytesToDecommit = ULONG( pLogWriteBuffer->m_pbWrite - pLogWriteBuffer->m_pbEntry );
        }
        if ( lBytesToDecommit > 0 )
        {
            pLogWriteBuffer->Decommit( pLogWriteBuffer->m_pbEntry, lBytesToDecommit );
        }
        pLogWriteBuffer->m_pLogBuffer->_critLGBuf.Leave();
    }
}

VOID LOG_WRITE_BUFFER::InitLogBuffer( const BOOL fLGFlags )
{
    Assert( m_critLGBuf.FOwner() );
    
    if ( fLGFlags == fLGOldLogExists || fLGFlags == fLGOldLogInBackup )
    {
        if ( m_pLog->FRecovering() && m_pLog->FRecoveringMode() == fRecoveringRedo )
        {
            m_pbWrite = m_pbLGBufMin;
            m_pbEntry = m_pbLGBufMin;
        }
        else
        {
            Assert( m_pbEntry >= m_pbLGBufMin && m_pbEntry < m_pbLGBufMax );
            Assert( m_pbWrite >= m_pbLGBufMin && m_pbWrite < m_pbLGBufMax );
            Assert( m_pLogStream->PbSecAligned( m_pbWrite, m_pbLGBufMin ) == m_pbWrite );
        }
    }
    else
    {
        m_pbWrite = m_pbLGBufMin;
        m_pbEntry = m_pbLGBufMin;
    }
}


VOID LOG_WRITE_BUFFER::LGWriteLog_( LOG_WRITE_BUFFER* const pLogBuffer )
{
    (VOID)pLogBuffer->FLGWriteLog( iorpLGWriteSignal, fTrue, fFalse );
}

VOID LOG_WRITE_BUFFER::WriteToClearSpace()
{
    const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );
    
    if ( FLGWriteLog( iorpLGWriteCapacity, fFalse, fTrue ) )
    {
        PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
    }
    else
    {
        m_semWaitForLogBufferSpace.FAcquire( cmsecWaitLogWrite );
    }

    FOSSetCleanupState( fCleanUpStateSaved );
}

BOOL LOG_WRITE_BUFFER::FLGWriteLog( const IOREASONPRIMARY iorp, BOOL fHasSignal, const BOOL fDisableDeadlockDetection )
{
    BOOL fWritten = fFalse;
    
    if ( !fHasSignal )
    {
        fHasSignal = m_semLogSignal.FTryAcquire();
    }

    Assert( !fHasSignal || ( m_semLogSignal.CAvail() == 0 ) );

    if ( m_semLogWrite.FTryAcquire() )
    {

        if ( fDisableDeadlockDetection )
        {
            CLockDeadlockDetectionInfo::DisableOwnershipTracking();
            CLockDeadlockDetectionInfo::DisableDeadlockDetection();
        }

        fWritten = ErrLGWriteLog( iorp ) >= JET_errSuccess;

        if ( fDisableDeadlockDetection )
        {
            CLockDeadlockDetectionInfo::EnableDeadlockDetection();
            CLockDeadlockDetectionInfo::EnableOwnershipTracking();
        }
        m_semLogWrite.Release();
    }


    if ( fHasSignal )
    {
        m_semLogSignal.Release();
    }

    return fWritten;
}

VOID LOG_WRITE_BUFFER::LGLazyCommit_( VOID *pGroupContext, VOID *pLogBuffer )
{
    LOG_WRITE_BUFFER *pLogWriteBuffer = (LOG_WRITE_BUFFER *)pLogBuffer;

    AtomicExchange( (LONG *)&pLogWriteBuffer->m_tickNextLazyCommit, 0 );

    C_ASSERT( sizeof(LGPOS) == sizeof(__int64) );
    LGPOS lgposCommit;
    *(__int64 *)&lgposCommit = AtomicCompareExchange( (__int64 *)&pLogWriteBuffer->m_lgposNextLazyCommit, 0, 0 );

    PIB pibFake;
    pibFake.m_pinst = pLogWriteBuffer->m_pinst;
    pLogWriteBuffer->ErrLGWaitForWrite( &pibFake, &lgposCommit );
}

ERR LOG_WRITE_BUFFER::ErrLGScheduleWrite( DWORD cmsecDurableCommit, LGPOS lgposCommit )
{
    ERR err = JET_errSuccess;

    C_ASSERT( sizeof(LGPOS) == sizeof(__int64) );
    forever
    {
        LGPOS lgposCurrent;
        *(__int64 *)&lgposCurrent = AtomicCompareExchange( (__int64 *)&m_lgposNextLazyCommit, 0, 0 );
        if ( CmpLgpos( lgposCommit, lgposCurrent ) <= 0 ||
             AtomicCompareExchange( (__int64 *)&m_lgposNextLazyCommit,
                                    *(__int64 *)&lgposCurrent,
                                    *(__int64 *)&lgposCommit ) == *(__int64 *)&lgposCurrent )
        {
            break;
        }
    }

    TICK tickLazyCommitNew = TickOSTimeCurrent() + cmsecDurableCommit;
    if ( m_tickNextLazyCommit != 0 &&
         TickCmp( m_tickNextLazyCommit, tickLazyCommitNew ) <= 0 )
    {
        return JET_errSuccess;
    }

    m_critNextLazyCommit.Enter();

    forever
    {
        TICK tickLazyCommitCurrent = m_tickNextLazyCommit;
        if ( tickLazyCommitCurrent != 0 &&
             TickCmp( tickLazyCommitCurrent, tickLazyCommitNew ) <= 0 )
        {
            err = JET_errSuccess;
            goto HandleError;
        }

        if ( AtomicCompareExchange( (LONG *)&m_tickNextLazyCommit, tickLazyCommitCurrent, tickLazyCommitNew ) == (LONG)tickLazyCommitCurrent )
        {
            break;
        }
    }

    OSTimerTaskScheduleTask( m_postLazyCommitTask, this, cmsecDurableCommit, 0 );

HandleError:
    m_critNextLazyCommit.Leave();
    return err;
}


BOOL LOG_WRITE_BUFFER::FWakeWaitingQueue( const LGPOS * const plgposToWrite )
{
    
    JET_PFNDURABLECOMMITCALLBACK pfnWrite = (JET_PFNDURABLECOMMITCALLBACK)PvParam( m_pinst, JET_paramDurableCommitCallback );
    if ( pfnWrite != NULL )
    {
        JET_COMMIT_ID commitId;
        JET_GRBIT grbit;

        commitId.signLog = *(JET_SIGNATURE *)&m_pLog->SignLog();
        commitId.commitId = (__int64)plgposToWrite->qw;
        grbit = m_pLog->FNoMoreLogWrite() ? JET_bitDurableCommitCallbackLogUnavailable : 0;

        pfnWrite( (JET_INSTANCE)m_pinst, &commitId, grbit );
    }

    
    m_critLGWaitQ.Enter();

    PIB *   ppibT               = m_ppibLGWriteQHead;
    BOOL    fWaitersExist       = fFalse;

    while ( ppibNil != ppibT )
    {
        PIB * const ppibNext    = ppibT->ppibNextWaitWrite;

        if ( CmpLgpos( &ppibT->lgposWaitLogWrite, plgposToWrite ) < 0 || m_pLog->FNoMoreLogWrite() )
        {
            Assert( ppibT->FLGWaiting() );
            ppibT->ResetFLGWaiting();

            if ( ppibT->ppibPrevWaitWrite )
            {
                ppibT->ppibPrevWaitWrite->ppibNextWaitWrite = ppibT->ppibNextWaitWrite;
            }
            else
            {
                m_ppibLGWriteQHead = ppibT->ppibNextWaitWrite;
            }

            if ( ppibT->ppibNextWaitWrite )
            {
                ppibT->ppibNextWaitWrite->ppibPrevWaitWrite = ppibT->ppibPrevWaitWrite;
            }
            else
            {
                m_ppibLGWriteQTail = ppibT->ppibPrevWaitWrite;
            }

            ppibT->asigWaitLogWrite.Set();
            PERFOpt( cLGUsersWaiting.Dec( m_pinst ) );
        }
        else
        {
            fWaitersExist = fTrue;
        }

        ppibT = ppibNext;
    }
    
    m_tickLastWrite = TickOSTimeCurrent();
    
    if ( !m_fDecommitTaskScheduled && m_postDecommitTask != NULL )
    {
        OSTimerTaskScheduleTask( m_postDecommitTask, this, g_dtickDecommitTaskDelay, g_dtickDecommitTaskDelay );
        m_fDecommitTaskScheduled = fTrue;
    }

    m_critLGWaitQ.Leave();

    return fWaitersExist;
}


ERR LOG_WRITE_BUFFER::ErrLGIWriteFullSectors(
    const IOREASONPRIMARY iorp,
    const UINT          csecFull,
    const UINT          isecWrite,
    const BYTE* const   pbWrite,
    BOOL* const         pfWaitersExist,
    const LGPOS&        lgposWriteEnd
    )
{
    ERR             err = JET_errSuccess;
    UINT            csecToWrite = csecFull;
    UINT            isecToWrite = isecWrite;
    const BYTE      *pbToWrite = pbWrite;
    BYTE            *pbFileEndT = pbNil;
    BOOL            fFlushed = fFalse;

    PERFOpt( cLGFullSegmentWrite.Inc( m_pinst ) );
    Assert( isecWrite + csecFull <= m_pLogStream->CSecLGFile() - 1 );

    m_pLogStream->LGAssertWriteOwner();

    for ( DWORD isecChecksum = 0; isecChecksum < csecFull; isecChecksum++ )
    {
        BYTE *pvSegHdr = (BYTE *)(pbWrite + m_pLogStream->CbSec() * isecChecksum);
        if ( pvSegHdr >= m_pbLGBufMax )
        {
            pvSegHdr -= m_cbLGBuf;
        }
        Assert( pvSegHdr >= m_pbLGBufMin && pvSegHdr < m_pbLGBufMax );
        LGSEGHDR *pSegHdr = (LGSEGHDR *)pvSegHdr;
        memset( pSegHdr->rgbReserved, 0, sizeof( pSegHdr->rgbReserved ) );
        pSegHdr->fFlags = 0;
        pSegHdr->le_lgposSegment.le_lGeneration = m_pLogStream->GetCurrentFileGen();
        pSegHdr->le_lgposSegment.le_isec = (USHORT)(isecWrite + isecChecksum);
        pSegHdr->le_lgposSegment.le_ib = 0;
        SetPageChecksum( pvSegHdr,
                         m_pLogStream->CbSec(),
                         logfilePage,
                         pSegHdr->le_lgposSegment.le_isec );

        if ( isecChecksum == 0 && m_fHaveShadow )
        {
            m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum, true );
        }
        else
        {
            m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum, false );
        }
    }

    if ( m_fHaveShadow )
    {

        Assert( isecToWrite >= m_pLogStream->CSecHeader() && isecToWrite < ( m_pLogStream->CSecLGFile() - 1 ) );
        Assert( isecToWrite + 1 <= ( m_pLogStream->CSecLGFile() - 1 ) );

        if ( pbToWrite == m_pbLGBufMax )
        {
            pbToWrite = m_pbLGBufMin;
        }
        
        Assert( pbToWrite >= m_pbLGBufMin && pbToWrite < m_pbLGBufMax );
        
        Call( m_pLogStream->ErrLGWriteSectorData(
            IOR( iorp, iorfShadow ),
            m_pLogStream->GetCurrentFileGen(),
            QWORD( isecToWrite ) * m_pLogStream->CbSec(),
            m_pLogStream->CbSec() * 1,
            pbToWrite,
            LOG_FLUSH_WRITE_0_ERROR_ID,
            &fFlushed ) );

        isecToWrite++;
        const BYTE* pbFirstSec = pbToWrite;
        pbToWrite += m_pLogStream->CbSec();
        csecToWrite--;
        if ( pbToWrite == m_pbLGBufMax )
        {
            pbToWrite = m_pbLGBufMin;
        }

        if ( 0 == csecToWrite )
        {

            Call( m_pLogStream->ErrLGWriteSectorData(
                IOR( iorp, iorfShadow ),
                m_pLogStream->GetCurrentFileGen(),
                QWORD( isecToWrite ) * m_pLogStream->CbSec(),
                m_pLogStream->CbSec() * 1,
                pbFirstSec,
                LOG_FLUSH_WRITE_0_ERROR_ID,
                &fFlushed ) );
        }
        else
        {

            Assert( isecToWrite >= m_pLogStream->CSecHeader() && isecToWrite < ( m_pLogStream->CSecLGFile() - 1 ) );
            Assert( isecToWrite + csecToWrite <= ( m_pLogStream->CSecLGFile() - 1 ) );

            if ( pbToWrite + ( m_pLogStream->CbSec() * csecToWrite ) > m_pbLGBufMax )
            {
                m_cLGWrapAround++;
                
                const IOREASON rgIor[] = { IOR( iorp ), IOR( iorp ) };
                const BYTE * rgpbLogData[] = { pbToWrite, m_pbLGBufMin };
                const ULONG rgcbLogData[] = {(ULONG)(m_pbLGBufMax - pbToWrite),
                                            m_pLogStream->CbSec() * csecToWrite - rgcbLogData[0]};
                Call( m_pLogStream->ErrLGWriteSectorData(
                    rgIor,
                    m_pLogStream->GetCurrentFileGen(),
                    QWORD( isecToWrite ) * m_pLogStream->CbSec(),
                    rgcbLogData,
                    rgpbLogData,
                    2,
                    LOG_FLUSH_WRITE_1_ERROR_ID,
                    &fFlushed,
                    fFalse,
                    lgposWriteEnd ) );
            }
            else
            {
                Call( m_pLogStream->ErrLGWriteSectorData(
                    IOR( iorp ),
                    m_pLogStream->GetCurrentFileGen(),
                    QWORD( isecToWrite ) * m_pLogStream->CbSec(),
                    m_pLogStream->CbSec() * csecToWrite,
                    pbToWrite,
                    LOG_FLUSH_WRITE_1_ERROR_ID,
                    &fFlushed,
                    fFalse,
                    lgposWriteEnd ) );
            }
            isecToWrite += csecToWrite;
            pbToWrite += m_pLogStream->CbSec() * csecToWrite;
            if ( pbToWrite >= m_pbLGBufMax )
            {
                pbToWrite -= m_cbLGBuf;
            }
        }
    }
    else
    {

        Assert( isecToWrite >= m_pLogStream->CSecHeader() && isecToWrite < ( m_pLogStream->CSecLGFile() - 1 ) );
        Assert( isecToWrite + csecToWrite <= ( m_pLogStream->CSecLGFile() - 1 ) );

        if ( pbToWrite + ( m_pLogStream->CbSec() * csecToWrite ) > m_pbLGBufMax )
        {
            m_cLGWrapAround++;

            const IOREASON rgIor[] = { IOR( iorp ), IOR( iorp ) };
            const BYTE * rgpbLogData[] = { pbToWrite, m_pbLGBufMin };
            const ULONG rgcbLogData[] = { (ULONG)(m_pbLGBufMax - pbToWrite),
                                        m_pLogStream->CbSec() * csecToWrite - rgcbLogData[0] };
            Call( m_pLogStream->ErrLGWriteSectorData(
                rgIor,
                m_pLogStream->GetCurrentFileGen(),
                QWORD( isecToWrite ) * m_pLogStream->CbSec(),
                rgcbLogData,
                rgpbLogData,
                2,
                LOG_FLUSH_WRITE_2_ERROR_ID,
                &fFlushed,
                fFalse,
                lgposWriteEnd ) );
        }
        else
        {
            Call( m_pLogStream->ErrLGWriteSectorData(
                IOR( iorp ),
                m_pLogStream->GetCurrentFileGen(),
                QWORD( isecToWrite ) * m_pLogStream->CbSec(),
                m_pLogStream->CbSec() * csecToWrite,
                pbToWrite,
                LOG_FLUSH_WRITE_2_ERROR_ID,
                &fFlushed,
                fFalse,
                lgposWriteEnd ) );
        }
        isecToWrite += csecToWrite;
        pbToWrite += m_pLogStream->CbSec() * csecToWrite;
        if ( pbToWrite >= m_pbLGBufMax )
        {
            pbToWrite -= m_cbLGBuf;
        }
    }

    m_fHaveShadow = fFalse;


    m_critLGBuf.Enter();

    Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, csecFull * m_pLogStream->CbSec() ) );

    Assert( CmpLgpos( &m_lgposToWrite, &lgposWriteEnd ) <= 0 );
    m_lgposToWrite = lgposWriteEnd;
    if ( fFlushed )
    {
        m_pLog->LGSetFlushTipWithLock( lgposWriteEnd );
    }

#ifdef DEBUG
    LGPOS lgposEndOfWrite;
    m_pLogBuffer->GetLgpos( const_cast< BYTE* >( pbToWrite ), &lgposEndOfWrite, m_pLogStream );
    Assert( CmpLgpos( &lgposWriteEnd, &lgposEndOfWrite ) <= 0 );
#endif
    m_isecWrite = isecToWrite;
    m_pbWrite = const_cast< BYTE* >( pbToWrite );

    Assert( m_pLogBuffer->FIsFreeSpace( pbWrite, csecFull * m_pLogStream->CbSec() ) );

    pbFileEndT = m_pbLGFileEnd;

    PERFOpt( cbLGBufferUsed.Set(
                    m_pinst,
                    LONG( m_pbWrite <= m_pbEntry ?
                                m_pbEntry - m_pbWrite :
                                m_cbLGBuf - ( m_pbWrite - m_pbEntry ) ) ) );

    m_critLGBuf.Leave();


    (VOID) FWakeWaitingQueue( &lgposWriteEnd );


    m_semWaitForLogBufferSpace.ReleaseAllWaiters();

    Assert( pbNil != pbToWrite );
    if ( pbFileEndT == pbToWrite )
    {
        (VOID) m_pLog->ErrLGUpdateCheckpointFile( fFalse );

        Call( m_pLogStream->ErrLGNewLogFile( m_pLogStream->GetCurrentFileGen(), fLGOldLogExists ) );

    }

HandleError:
    Assert( err >= JET_errSuccess || err == errOSSnapshotNewLogStopped || m_pLog->FNoMoreLogWrite() );
    const BOOL fWaitersExist = FWakeWaitingQueue( &lgposWriteEnd );
    if ( pfWaitersExist )
    {
        *pfWaitersExist = fWaitersExist;
    }

    return err;
}


ERR LOG_WRITE_BUFFER::ErrLGIWritePartialSector(
    const IOREASONPRIMARY iorp,
    BYTE*   pbWriteEnd,
    UINT    isecWrite,
    BYTE*   pbWrite,
    const LGPOS& lgposWriteEnd
    )
{
    ERR     err = JET_errSuccess;
    BOOL    fFlushed = fFalse;

    PERFOpt( cLGPartialSegmentWrite.Inc( m_pinst ) );

    Assert( isecWrite >= m_pLogStream->CSecHeader() );
    Assert( isecWrite < m_pLogStream->CSecLGFile() - 1 );

    Assert( m_pLogStream->CbSec() == LOG_SEGMENT_SIZE );
    Assert( ( pbWriteEnd - pbWrite ) < LOG_SEGMENT_SIZE );
    UtilMemCpy( m_pvPartialSegment, pbWrite, ( pbWriteEnd - pbWrite ) );
    memset( m_pvPartialSegment + ( pbWriteEnd - pbWrite ),
            0,
            m_pLogStream->CbSec() - ( pbWriteEnd - pbWrite ) );
    LGSEGHDR *pSegHdr = (LGSEGHDR *)m_pvPartialSegment;
    memset( pSegHdr->rgbReserved, 0, sizeof( pSegHdr->rgbReserved ) );
    pSegHdr->fFlags = 0;
    pSegHdr->le_lgposSegment.le_lGeneration = m_pLogStream->GetCurrentFileGen();
    pSegHdr->le_lgposSegment.le_isec = (USHORT)isecWrite;
    pSegHdr->le_lgposSegment.le_ib = 0;
    SetPageChecksum( m_pvPartialSegment,
                     m_pLogStream->CbSec(),
                     logfilePage,
                     pSegHdr->le_lgposSegment.le_isec );

    IOREASON ior( iorp, ( m_pLog->FRecovering() && iorpPatchFix == iorp ) ? iorfFill : iorfNone );
    Assert( isecWrite + 1 < m_pLogStream->CSecLGFile() );
    Assert( m_pLogStream->PbSecAligned( pbWrite, m_pbLGBufMin ) == pbWrite );

    if ( m_fHaveShadow )
    {
        m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum, true );

        Call( m_pLogStream->ErrLGWriteSectorData(
                    ior,
                    m_pLogStream->GetCurrentFileGen(),
                    QWORD( isecWrite ) * m_pLogStream->CbSec(),
                    m_pLogStream->CbSec() * 1,
                    m_pvPartialSegment,
                    LOG_FLUSH_WRITE_3_ERROR_ID,
                    &fFlushed ) );

        m_pLogStream->LGAssertWriteOwner();

        Call( m_pLogStream->ErrLGWriteSectorData(
                    IOR( iorp, iorfShadow ),
                    m_pLogStream->GetCurrentFileGen(),
                    QWORD( isecWrite + 1 ) * m_pLogStream->CbSec(),
                    m_pLogStream->CbSec() * 1,
                    m_pvPartialSegment,
                    LOG_FLUSH_WRITE_4_ERROR_ID,
                    &fFlushed,
                    fFalse,
                    lgposWriteEnd ) );

        m_pLogStream->LGAssertWriteOwner();
    }
    else
    {
        const IOREASON rgIor[] = { ior, IOR( iorp, iorfShadow ) };
        const BYTE * rgpbLogData[] = { m_pvPartialSegment, m_pvPartialSegment };
        const ULONG rgcbLogData[] = { m_pLogStream->CbSec(), m_pLogStream->CbSec() };

        m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum, false );

        Call( m_pLogStream->ErrLGWriteSectorData(
                    rgIor,
                    m_pLogStream->GetCurrentFileGen(),
                    QWORD( isecWrite ) * m_pLogStream->CbSec(),
                    rgcbLogData,
                    rgpbLogData,
                    2,
                    LOG_FLUSH_WRITE_3_ERROR_ID,
                    &fFlushed,
                    fFalse,
                    lgposWriteEnd ) );

        m_pLogStream->LGAssertWriteOwner();
    }

    m_fHaveShadow = fTrue;

    m_critLGBuf.Enter();

    Assert( pbWrite < pbWriteEnd );
    Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, ULONG( pbWriteEnd - pbWrite ) ) );

    m_lgposToWrite = lgposWriteEnd;
    if ( fFlushed )
    {
        m_pLog->LGSetFlushTipWithLock( lgposWriteEnd );
    }


    m_critLGBuf.Leave();

HandleError:

    Assert( err >= JET_errSuccess || m_pLog->FNoMoreLogWrite() );
    (VOID) FWakeWaitingQueue( &lgposWriteEnd );

    return err;
}


ERR LOG_WRITE_BUFFER::ErrLGIDeferredWrite( const IOREASONPRIMARY iorp, const BOOL fWriteAll )
{
    ERR     err = JET_errSuccess;
    BYTE*   pbEndOfData = pbNil;
    UINT    isecWrite;
    BYTE*   pbWrite = pbNil;
    LGPOS   lgposWriteEnd = lgposMax;

    m_critLGBuf.Enter();

    m_msLGPendingCopyIntoBuffer.Partition();

    if ( pbNil == m_pbLGFileEnd )
    {
        pbEndOfData = m_pbEntry;
        lgposWriteEnd = m_lgposMaxWritePoint;
    }
    else
    {
        pbEndOfData = m_pbLGFileEnd;
        Assert( m_pLogStream->PbSecAligned( pbEndOfData, m_pbLGBufMin ) == pbEndOfData );
        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposWriteEnd, m_pLogStream );
    }

    isecWrite = m_isecWrite;
    pbWrite = m_pbWrite;

    UINT csecFull;
    BOOL fPartialSector;

    if ( pbEndOfData < pbWrite )
    {
        csecFull = ULONG( ( pbEndOfData + m_cbLGBuf ) - pbWrite ) / m_pLogStream->CbSec();
    }
    else
    {
        csecFull = ULONG( pbEndOfData - pbWrite ) / m_pLogStream->CbSec();
    }

    fPartialSector = ( 0 !=
        ( ( pbEndOfData - m_pLogStream->PbSecAligned( pbEndOfData, m_pbLGBufMin ) ) % m_pLogStream->CbSec() ) );

#ifdef DEBUG
    if ( 0 == csecFull )
    {
        Assert( fPartialSector );
        Assert( pbEndOfData > pbWrite );
        Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, ULONG( pbEndOfData - pbWrite ) ) );
    }
    else
    {
        Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, csecFull * m_pLogStream->CbSec() ) );
        Assert( fPartialSector || m_pbLGFileEnd || ( isecWrite + csecFull == lgposWriteEnd.isec ) );
    }
#endif

    if ( 0 == csecFull )
    {
        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposWriteEnd, m_pLogStream );
    }

    m_critLGBuf.Leave();

    if ( 0 == csecFull )
    {
        Assert( fPartialSector );
        Call( ErrLGIWritePartialSector( iorp, pbEndOfData, isecWrite, pbWrite, lgposWriteEnd ) );
    }
    else
    {
        Call( ErrLGIWriteFullSectors( iorp, csecFull, isecWrite, pbWrite, NULL, lgposWriteEnd ) );
    }

HandleError:

    return err;
}


ERR LOG_WRITE_BUFFER::ErrLGWriteLog( const IOREASONPRIMARY iorp, const BOOL fWriteAll )
{
    ERR                     err     = JET_errSuccess;

    m_pLogStream->LockWrite();

#ifdef DEBUG
    m_dwDBGLogThreadId = DwUtilThreadId();
#endif

    const BOOL fLogDownBeforeWrite = m_pLog->FNoMoreLogWrite();
    err = ErrLGIWriteLog( iorp, fWriteAll );
    const BOOL fLogDownAfterWrite = m_pLog->FNoMoreLogWrite();

    if ( !fLogDownBeforeWrite && fLogDownAfterWrite )
    {
        const WCHAR*    rgpsz[ 1 ];
        DWORD           irgpsz      = 0;
        WCHAR           wszAbsPath[ IFileSystemAPI::cchPathMax ];

        if ( m_pinst->m_pfsapi->ErrPathComplete( SzParam( m_pinst, JET_paramLogFilePath ), wszAbsPath ) < JET_errSuccess )
        {
            OSStrCbCopyW( wszAbsPath, sizeof(wszAbsPath), SzParam( m_pinst, JET_paramLogFilePath ) );
        }

        rgpsz[ irgpsz++ ]   = wszAbsPath;

        UtilReportEvent(
                eventError,
                LOGGING_RECOVERY_CATEGORY,
                LOG_DOWN_ID,
                irgpsz,
                rgpsz,
                0,
                NULL,
                m_pinst );
    }

    m_pLogStream->UnlockWrite();

    return err;
}

ERR LOG_WRITE_BUFFER::ErrLGIWriteLog( const IOREASONPRIMARY iorp, const BOOL fWriteAll )
{
    ERR     err;
    BOOL    fPartialSector;
    UINT    isecWrite;
    BYTE*   pbWrite;
    UINT    csecFull;
    BYTE*   pbEndOfData;
    LGPOS   lgposWriteEnd;
    BOOL    fNewGeneration;

Repeat:
    fNewGeneration  = fFalse;
    err             = JET_errSuccess;
    fPartialSector  = fFalse;
    pbWrite         = pbNil;
    csecFull        = 0;
    pbEndOfData     = pbNil;
    lgposWriteEnd = lgposMax;

    m_critLGBuf.Enter();


    if ( m_pLog->FNoMoreLogWrite( &err ) && err != errLogServiceStopped )
    {
        m_critLGBuf.Leave();

        LGPOS lgposToWriteT = lgposMin;
        (VOID)FWakeWaitingQueue( &lgposToWriteT );

        Call( ErrERRCheck( JET_errLogWriteFail ) );
    }

    if ( !m_pLogStream->FLGFileOpened() )
    {
        m_critLGBuf.Leave();
        err = JET_errSuccess;
        goto HandleError;
    }

    if ( m_fLogPaused )
    {
        m_critLGBuf.Leave();
        err = JET_errSuccess;
        goto HandleError;
    }

    if ( ! m_fNewLogRecordAdded )
    {
        m_critLGBuf.Leave();
        err = JET_errSuccess;
        goto HandleError;
    }

    m_msLGPendingCopyIntoBuffer.Partition();

    if ( pbNil != m_pbLGFileEnd )
    {
        fNewGeneration = fTrue;
        pbEndOfData = m_pbLGFileEnd;
        Assert( m_pLogStream->PbSecAligned( pbEndOfData, m_pbLGBufMin ) == pbEndOfData );

        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposWriteEnd, m_pLogStream );
    }
    else
    {
        LGPOS   lgposEndOfData = lgposMin;

        pbEndOfData = m_pbEntry;

        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposEndOfData, m_pLogStream );

        if ( CmpLgpos( &lgposEndOfData, &m_lgposToWrite ) <= 0 )
        {
            m_critLGBuf.Leave();
            err = JET_errSuccess;
            goto HandleError;
        }

        lgposWriteEnd = m_lgposMaxWritePoint;
    }

    isecWrite = m_isecWrite;
    pbWrite = m_pbWrite;

    if ( pbEndOfData < pbWrite )
    {
        csecFull = ULONG( ( pbEndOfData + m_cbLGBuf ) - pbWrite ) / m_pLogStream->CbSec();
    }
    else
    {
        csecFull = ULONG( pbEndOfData - pbWrite ) / m_pLogStream->CbSec();
    }

    fPartialSector = ( 0 !=
        ( ( pbEndOfData - m_pLogStream->PbSecAligned( pbEndOfData, m_pbLGBufMin ) ) % m_pLogStream->CbSec() ) );

    Assert( csecFull + ( fPartialSector ? 1 : 0 ) <= m_csecLGBuf );

    if ( pbNil != m_pbLGFileEnd )
    {
        Assert( csecFull > 0 );
        Assert( ! fPartialSector );
    }


    if ( 0 == csecFull )
    {
        Assert( fPartialSector );
        Assert( pbEndOfData > pbWrite );
        Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, ULONG( pbEndOfData - pbWrite ) ) );
    }
    else
    {
        Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, csecFull * m_pLogStream->CbSec() ) );
        Assert( fPartialSector || m_pbLGFileEnd || ( isecWrite + csecFull == lgposWriteEnd.isec ) );
    }

    if ( 0 == csecFull )
    {
        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposWriteEnd, m_pLogStream );
    }

    m_critLGBuf.Leave();

    if ( 0 == csecFull )
    {
        Assert( fPartialSector );
        Call( ErrLGIWritePartialSector( iorp, pbEndOfData, isecWrite, pbWrite, lgposWriteEnd ) );
    }
    else
    {
        BOOL fWaitersExist = fFalse;
        Call( ErrLGIWriteFullSectors( iorp, csecFull, isecWrite, pbWrite,
            &fWaitersExist, lgposWriteEnd ) );

        if ( fWaitersExist && fNewGeneration )
        {
            goto Repeat;
        }

        m_critLGBuf.Enter();
        fNewGeneration = ( pbNil != m_pbLGFileEnd );
        m_critLGBuf.Leave();
        if ( fNewGeneration )
        {
            goto Repeat;
        }

        if ( fPartialSector && fWaitersExist )
        {
            Call( ErrLGIDeferredWrite( iorp, fWriteAll ) );
        }
    }


HandleError:

    if ( m_pLog->FNoMoreLogWrite() && m_fStopOnNewLogGeneration )
    {
        Assert( !m_fLogPaused );
        m_sigLogPaused.Set();
    }

    return err;
}

ERR LGInitTerm::ErrLGSystemInit( void )
{
    ERR err = JET_errSuccess;

    err = g_taskmgrLog.ErrTMInit( min( 8 * CUtilProcessProcessor(), 100 ) );

    return err;
}

VOID LGInitTerm::LGSystemTerm( void )
{
    g_taskmgrLog.TMTerm();
}

ERR LOG_WRITE_BUFFER::ErrLGTasksInit( void )
{
    ERR err;
#ifdef DEBUG
    m_lgposLastLogRec = lgposMin;
#endif

    if ( !m_semLogWrite.CAvail( ) )
    {
        m_semLogWrite.Release();
    }

    if ( !m_semLogSignal.CAvail( ) )
    {
        m_semLogSignal.Release();
    }

    Assert( m_postDecommitTask == NULL );
    if ( OnDebugOrRetail( fTrue, fFalse ) || FJetConfigMedMemory() || FJetConfigLowMemory() )
    {
        CallR( ErrOSTimerTaskCreate( DecommitLogBufferTask, this, &m_postDecommitTask ) );
    }

    Assert( m_postLazyCommitTask == NULL );
    CallR( ErrOSTimerTaskCreate( LGLazyCommit_, this, &m_postLazyCommitTask ) );

    return JET_errSuccess;
}

VOID LOG_WRITE_BUFFER::LGTasksTerm( void )
{
    AtomicExchange( (LONG *)&m_tickNextLazyCommit, 0 );
    m_semLogWrite.Acquire();
    m_semLogSignal.Acquire();
    
    if ( m_postLazyCommitTask )
    {
        OSTimerTaskCancelTask( m_postLazyCommitTask );
        OSTimerTaskDelete( m_postLazyCommitTask );
        m_postLazyCommitTask = NULL;
    }
    
    if ( m_postDecommitTask )
    {
        OSTimerTaskCancelTask( m_postDecommitTask );
        OSTimerTaskDelete( m_postDecommitTask );
        m_postDecommitTask = NULL;
    }
}

