// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"

#include "..\PageSizeClean.hxx"

#ifdef PERFMON_SUPPORT

//  monitoring statistics
//
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

#endif // PERFMON_SUPPORT

LOG_WRITE_BUFFER::LOG_WRITE_BUFFER( INST * pinst, LOG * pLog, ILogStream * pLogStream, LOG_BUFFER *pLogBuffer )
    : CZeroInit( sizeof( LOG_WRITE_BUFFER ) ),
      m_pLog( pLog ),
      m_pinst( pinst ),
      m_pLogStream( pLogStream ),
      m_pLogBuffer( pLogBuffer ),
      // we always start writing to a new sector, so we never have a shadow sector to start with
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

    //  log write sync

    m_semLogSignal.Release();
    m_semLogWrite.Release();

    //  log perf counters
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
    //  log perf counters
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

    //  _pbWrite is always aligned
    //
    Assert( _pbWrite != NULL );
    Assert( _pbWrite == pLogStream->PbSecAligned( _pbWrite, _pbLGBufMin ) );
    Assert( _isecWrite >= (INT)pLogStream->CSecHeader() );

    // pb is a pointer into the log buffer, so it should be valid.
    Assert( pb >= _pbLGBufMin );
    Assert( pb < _pbLGBufMax );
    // m_pbWrite should also be valid since we're using it for comparisons here
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
        // We do not expect noncontinuous write in log buffer.  
        // But for the safety purpose in production, we commit the whole buffer 
        // in case noncontinuous write happens.
        Assert( fFalse );

        pb = _pbLGBufMin;
        cb = _cbLGBuf;
        _pbLGCommitStart = _pbLGBufMin;
    }

    // All the adjustments of _pbLGCommitStart and _pbLGCommitEnd is based on the fact
    // that pb is inside or immediately next to committed area. 
    if ( pb + cb <= _pbLGBufMax )
    {
        if ( !FOSMemoryPageCommit( ( void * )pb, cb ) )
        {
            // Restore cleanup checking before existing.
            RFSThreadReEnable( cRFSCountdownOld );
            FOSSetCleanupState( fCleanUpStateSaved );
            return fFalse;
        }
        BYTE * pbFirstUncommitted = _pbLGBufMin + roundup( pb + cb - _pbLGBufMin, OSMemoryPageCommitGranularity() );

        if ( pbFirstUncommitted >= _pbLGCommitStart &&
            ( ( _pbLGCommitStart > _pbLGCommitEnd && pb <= _pbLGCommitEnd + 1 )
                || ( _pbLGCommitStart <= _pbLGCommitEnd && _pbLGCommitEnd == _pbLGBufMax - 1 && pb == _pbLGBufMin ) ) )
        {
            // fully committed
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
            // Restore cleanup checking before existing.
            RFSThreadReEnable( cRFSCountdownOld );
            FOSSetCleanupState( fCleanUpStateSaved );
            return fFalse;
        }
        if ( _pbLGCommitStart <= _pbLGCommitEnd || pb >= _pbLGCommitStart )
        {
            BYTE *pbFirstUncommitted = _pbLGBufMin + roundup( pb + cb - _pbLGBufMax, OSMemoryPageCommitGranularity() ) - 1;
            if ( pbFirstUncommitted >= _pbLGCommitStart )
            {
                // fully committed
                _pbLGCommitEnd = _pbLGCommitStart - 1;
            }
            else
            {
                _pbLGCommitEnd = pbFirstUncommitted;
            }
        }
        else
        {
            // fully committed
            _pbLGCommitEnd = _pbLGCommitStart - 1;
        }
    }
    // Restore cleanup checking
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
        // Commit an additional 16KB.
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
            // fully committed
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
        return LONG( _pbLGCommitEnd - _pbLGCommitStart ) + 1; // _pbLGCommitEnd is inclusive
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
    // Do not decommit for LOG_READ_BUFFER or empty buffer
    if ( _fReadOnly || _pbLGBufMin == NULL || _pbLGBufMax == NULL )
    {
        return;
    }
    // Make sure we decommit at the next boundary, otherwise valid buffer may be decommitted.
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
        // When the whole buffer is decommitted.
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


// Deallocates any storage for the log buffer, if necessary.

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
        //  Internal code decided the number of sectors / log buffers, should 
        //  know what it's doing ...
        lLogBuffers = csecExplicitRequest;
#ifdef DEBUG
#endif
    }
    else
    {
        //  Get the users requested number of log buffers, which has a unit size
        //  of 512 byte "sectors".
        lLogBuffers = (LONG)UlParam( pinst, JET_paramLogBuffers );

        //  Take the intended log buffers in 512 byte "sectors", and turn them into 
        //  the intended # of real log buffers given the native sector size of the 
        //  drive.  This may increase the size of the log buffers we allocate from 
        //  what the client asked for.  
        lLogBuffers = roundup( ( lLogBuffers * cbLogBuffersParamSize ), pLogStream->CbSec() ) / pLogStream->CbSec();
    }
    Assert( lLogBuffers ); // should have something to start with ...

    //  ensure log buffers > a page because we must be able to log a split/merge
    //  in one log record and log records cannot span writes

    lLogBuffers = max( lLogBuffers, LONG( (CbINSTMaxPageSize( pinst ) * 2) / pLogStream->CbSec() + 4 ) );

    //  ensure that the log buffers are aligned as determined

    lLogBuffers = roundup( lLogBuffers, csecAlign );

    if ( fReadOnly )
    {

        //  ensure log buffers are >= log file size if in redo because we rely on
        //  having the entire log file cached during redo
        
        lLogBuffers = max( lLogBuffers, LONG( roundup( pLogStream->CSecLGFile(), csecAlign ) ) );
    }
    else
    {

        // no point having log buffer greater than logfile size - sectors for
        // header - shadow sector since it would not get used

        lLogBuffers = min( lLogBuffers, LONG( roundup( pLogStream->CSecLGFile() - ( pLogStream->CSecHeader() + 1 ), csecAlign ) ) );
    }

    if ( fReadOnly == _fReadOnly &&
         (ULONG)lLogBuffers == _csecLGBuf &&
         lLogBuffers * pLogStream->CbSec() == _cbLGBuf )
    {
        return JET_errSuccess;
    }
    
    _critLGBuf.Enter();

    //  reset log buffer
    //
    LGTermLogBuffers();

    //  save our chosen log buffer parameters

    _fReadOnly  = fReadOnly;
    _csecLGBuf  = lLogBuffers;
    _cbLGBuf    = lLogBuffers * pLogStream->CbSec();

    PERFOpt( cbLGBufferSize.Set( pinst, _cbLGBuf ) );
    PERFOpt( cbLGFileSize.Set( pinst, pLogStream->CSecLGFile() * pLogStream->CbSec() ) );
    PERFOpt( cLGCheckpointTooDeep.Set( pinst, UlParam( pinst, JET_paramCheckpointTooDeep ) ) );
    PERFOpt( cLGWaypointDepth.Set( pinst, UlParam( pinst, JET_paramWaypointLatency ) ) );

    //  allocate our log buffers of size _cbLGBuf and reserve an extra guard page for debugging purpose

    if ( ! ( _pbLGBufMin = (BYTE*)PvOSMemoryPageReserve( roundup( _cbLGBuf + OSMemoryPageCommitGranularity(), OSMemoryPageReserveGranularity() ), NULL ) ) )
    {
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    
    //  Ensure the log buffers are aligned to the page commit granularity   
    Expected( _cbLGBuf == rounddn( _cbLGBuf, OSMemoryPageCommitGranularity() ) );

    //  Ensure that page reserve granularity is aligned to sector size
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
        // Initially commit one page for WRITE_BUFFER.
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

    //  try to acquire the right to signal a log write
    //
    if ( fSignaled )
    {
        // retry for 5 minutes or until we successfully post a write task
        // to the thread pool.  we must do this because we can temporarily fail
        // to post due to low resources and we don't want to make a thread poll
        // to see if it missed any signals.
        //
        ERR         err;
        const TICK  tickEnd = TickOSTimeCurrent() + cmsecWaitLogWriteMax;
        
        const BOOL fCleanUpStateSaved = FOSSetCleanupState( fFalse );

        while ( ( err = g_taskmgrLog.ErrTMPost( (CGPTaskManager::PfnCompletion)LGWriteLog_, this ) ) < JET_errSuccess )
        {
            UtilSleep( cmsecWaitLogWrite );

            // Couldn't signal the log write for 5 minutes.
            // Make the log unavailable and return.
            if ( TickCmp(TickOSTimeCurrent(), tickEnd) >= 0 )
            {
                Assert( err < JET_errSuccess );

                // this is the only expected error
                //
                Assert( err == JET_errOutOfMemory );

                m_pLog->LGReportError( LOG_FILE_SIGNAL_ERROR_ID, err );

                m_pLog->SetNoMoreLogWrite( err );

                m_semLogSignal.Release( );
                fSignaled = fFalse;
                break;
            }
        }

        // Restore cleanup checking
        FOSSetCleanupState( fCleanUpStateSaved );
    }

    return fSignaled;
}

// Log buffer utilities to make it easy to verify that we're
// using data in the log buffer that is "used" (allocated)
// (between m_pbWrite and m_pbEntry) and "free" (unallocated).
// Takes into consideration VM wrap-around and the circularity
// of the log buffer.


// We should not reference data > PbMaxEntry() when dealing
// with valid data in the log buffer.

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


// When adding log records to the log buffer, we should not copy data
// into the region past PbMaxWrite().

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

// Normalizes a pointer into the log buffer for use with
// comparisons to test whether the data the pointer points to
// is used.

const BYTE* LOG_BUFFER::PbMaxPtrForUsed( const BYTE* const pb ) const
{
    Assert( _pbEntry >= _pbLGBufMin && _pbEntry < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin && _pbWrite < _pbLGBufMax );
    // below may need to be pb <= m_pbLGBufMax
    Assert( pb >= _pbLGBufMin && pb < _pbLGBufMax );
    if ( pb < _pbEntry )
        return pb + _cbLGBuf;
    else
        return pb;
}

// Normalizes a pointer into the log buffer for use with
// comparisons to test whether the data the pointer points to
// is free.

const BYTE* LOG_BUFFER::PbMaxPtrForFree( const BYTE* const pb ) const
{
    Assert( _pbEntry >= _pbLGBufMin && _pbEntry < _pbLGBufMax );
    Assert( _pbWrite >= _pbLGBufMin && _pbWrite < _pbLGBufMax );
    // below may need to be pb <= m_pbLGBufMax
    Assert( pb >= _pbLGBufMin && pb < _pbLGBufMax );
    if ( pb < _pbWrite )
        return pb + _cbLGBuf;
    else
        return pb;
}

// In use data in log buffer is between m_pbWrite and PbMaxEntry().

ULONG LOG_BUFFER::CbLGUsed() const
{
    return ULONG( PbMaxEntry() - _pbWrite );
}

// Available space in log buffer is between the entry point
// and the start of the real data

ULONG   LOG_BUFFER::CbLGFree() const
{
    return ULONG( PbMaxWrite() - _pbEntry );
}

// Internal implementation function to determine whether
// cb bytes at pb are in the free portion of the log buffer.

BOOL LOG_BUFFER::FLGIIsFreeSpace( const BYTE* const pb, ULONG cb ) const
{
    Assert( cb < _cbLGBuf );
    // There is never a time when the entire log buffer
    // is free. This state never occurs.
    Assert( cb != _cbLGBuf );

    if ( cb == 0 )
        return fTrue;

    return ( CbLGFree() >= cb ) &&
        ( PbMaxPtrForUsed( pb ) >= _pbEntry && PbMaxPtrForUsed( pb ) < PbMaxWrite() ) &&
        ( PbMaxPtrForUsed( pb ) + cb > _pbEntry && PbMaxPtrForUsed( pb ) + cb <= PbMaxWrite() );
}

// Returns whether cb bytes at pb are in the free portion of the log buffer.

BOOL LOG_BUFFER::FIsFreeSpace( const BYTE* const pb, ULONG cb ) const
{
    const BOOL fResult = FLGIIsFreeSpace( pb, cb );
    // verify with cousin
    Assert( fResult == ! FLGIIsUsedSpace( pb, cb ) );
    return fResult;
}

// Internal implementation function to determine whether cb
// bytes at pb are in the used portion of the log buffer.

BOOL LOG_BUFFER::FLGIIsUsedSpace( const BYTE* const pb, ULONG cb ) const
{
    Assert( cb <= _cbLGBuf );

    if ( cb == 0 )
        return fFalse;

    return ( CbLGUsed() >= cb ) &&
            ( PbMaxPtrForFree( pb ) >= _pbWrite && PbMaxPtrForFree( pb ) < PbMaxEntry() ) &&
            ( PbMaxPtrForFree( pb ) + cb > _pbWrite && PbMaxPtrForFree( pb ) + cb <= PbMaxEntry() );
}

// Returns whether cb bytes at pb are in the used portion of the log buffer.

BOOL LOG_BUFFER::FIsUsedSpace( const BYTE* const pb, ULONG cb ) const
{
    const BOOL fResult = FLGIIsUsedSpace( pb, cb );
    // verify with cousin
    // If the user is asking if all of the log buffer is used,
    // we don't want to verify with FLGIIsFreeSpace() because that
    // is an impossibility for us.
    if ( cb != _cbLGBuf )
    {
        Assert( fResult == ! FLGIIsFreeSpace( pb, cb ) );
    }
    return fResult;
}

//  adds log record defined by (pb,cb)
//  at *ppbET, wrapping around if necessary

BOOL
LOG_WRITE_BUFFER::FLGIAddLogRec(
    const DATA *        rgdata,
    const INT           cdata,
    BOOL                fAllocOnly,
    __deref BYTE**      ppbET )
{
    BYTE *pbET = *ppbET;
    INT idataStart = 0;

    // We must be pointing into the main mapping of the log buffer.
    Assert( pbET >= m_pbLGBufMin );
    Assert( pbET < m_pbLGBufMax );

    //  since we may not be in critLGBuf, take a snapshot
    //  of m_pbWrite in case it is currently being modified
    //  (note that even if it is being modified, the asserts
    //  below should still be valid)
    //
    BYTE * pbWrite          = m_pbWrite;

    //  m_pbWrite should be pointing inside the log buffer
    //
    Assert( pbWrite >= m_pbLGBufMin );
    Assert( pbWrite < m_pbLGBufMax );

    BYTE * pbWrapWrite      = ( pbWrite <= pbET ? ( pbWrite + m_cbLGBuf ) : pbWrite );

    // records are split into multiple data's, try to combine them
    while ( idataStart < cdata )
    {
        INT idataEnd = idataStart;
        INT cbNeeded = 0;
        INT cbRecordLeft = 0;

        // find the set of rgdata that constitutes a single log record
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

        // now copy the rgdata's constituting those log records
        BOOL fFragmented = fFalse;
        INT dataOffset = 0;
        while ( cbNeeded != 0 )
        {
            //  ensure that we don't write into space where valid
            //  log records already exist.
            if ( pbET + cbNeeded >= pbWrapWrite )
            {
                Assert( fAllocOnly );
                return fFalse;
            }

            INT cbSegRemain = m_pLogStream->CbSec() - ( ( pbET - m_pbLGBufMin ) % m_pLogStream->CbSec() );
            if ( cbSegRemain == (INT)m_pLogStream->CbSec() )
            {
                // leave space for header
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
                // If remaining segment is too small to hold either the record
                // or a fragment, just fill it with NOPs and move on to next
                // segment
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
                    // If we're now pointing past the end, fix us up
                    // so we point into the main mapping of the log buffer.
                    if ( pbET >= m_pbLGBufMax )
                    {
                        // We should never be pointing past the 2nd mapping.
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

                    // cannot fit the record in the segment, fragment it
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
            else /* fFragmented */
            {
                Assert( cbSegRemain == (INT)( m_pLogStream->CbSec() - sizeof( LGSEGHDR ) ) );

                // write header for next fragment
                if ( !fAllocOnly )
                {
                    LRFRAGMENT lrFrag;
                    lrFrag.le_cbFragmentSize = (USHORT)min( cbNeeded, (INT)( cbSegRemain - sizeof( lrFrag ) ) );
                    UtilMemCpy( pbET, &lrFrag, sizeof( lrFrag ) );
                }
                pbET += sizeof( LRFRAGMENT );
                cbSegRemain -= sizeof( LRFRAGMENT );
            }

            // do copying of actual log record
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

            // If we're now pointing past the end, fix us up
            // so we point into the main mapping of the log buffer.
            if ( pbET >= m_pbLGBufMax )
            {
                // We should never be pointing past the 2nd mapping.
                Assert( pbET < m_pbLGBufMax + m_cbLGBuf );
                pbET -= m_cbLGBuf;
                pbWrapWrite = pbWrite;
            }
        }

        idataStart = idataEnd;
    }

    // Point inside first mapping.
    Assert( pbET >= m_pbLGBufMin );
    Assert( pbET < m_pbLGBufMax );
    Assert( pbET <= pbWrapWrite );
    // At least leave one byte free since we cannot distinguish between
    // completely full and completely empty buffer
    if ( pbET == pbWrapWrite )
    {
        Assert( fAllocOnly );
        return fFalse;
    }
    *ppbET = pbET;
    return fTrue;
}


//
//  Add log record to circular log buffer. Signal write thread to write log
//  buffer if at least (g_lLogBuffers / 2) disk sectors are ready for writing.
//  Return error if we run out of log buffer space.
//
//  RETURNS     JET_errSuccess, or error code from failing routine
//              or errLGNotSynchronous if there's not enough space,
//              or JET_errLogWriteFail if the log is down.
//

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
    const BOOL  fStopOnNewGen       = ( fNewGen && ( fLGFlags & fLGStopOnNewGen ) );    //  currently, only support ability to stop on new gen if also forcing new gen
    BYTE*       pbSectorBoundary;
    BOOL        fFormatJetTmpLog    = fFalse;
    BYTE*       pbLogRec            = pbNil;
    BOOL        fForcedNewGen       = fFalse;
    LGPOS       lgposLogRecTmpLog   = lgposMax;

    Assert( ( fLGFlags & fLGFillPartialSector ) || rgdata[0].Pv() != NULL );
    Expected( !(( fLGFlags & fLGFillPartialSector ) && ( fLGFlags & fLGCreateNewGen )) ); // untested
    Expected( !(( fLGFlags & fLGFillPartialSector ) && ( fLGFlags & fLGStopOnNewGen )) ); // untested

    Assert( m_pLog->FLogDisabled() == fFalse );

    if ( m_pLog->FLogDisabledDueToRecoveryFailure() )
    {
        return ErrERRCheck( JET_errLogDisabledDueToRecoveryFailure );
    }

    // No one should be adding log records if we're in redo mode.
    Assert( !m_pLog->FRecovering() || fRecoveringRedo != m_pLog->FRecoveringMode() );

    // RecoveryUndo2 should be the first log record logged during undo
    if ( fRecoveringUndo == m_pLog->FRecoveringMode() &&
         !m_pLog->FRecoveryUndoLogged() )
    {
        Assert( *(LRTYP *)rgdata[0].Pv() == lrtypRecoveryUndo2 );
    }

    Assert( !FIsOldLrckLogFormat( m_pLogStream->GetCurrentFileHdr()->lgfilehdr.le_ulMajor ) );
    Assert( m_cbLGBuf / m_csecLGBuf == m_pLogStream->CbSec() );
    Assert( m_pLogStream->CbSec() == LOG_SEGMENT_SIZE );

    //  get m_pbEntry in order to add log record
    //
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
             // Allow partial sector fill when we have deliberately stopped logging, but not yet emitted log end
             ( !( fLGFlags & fLGFillPartialSector ) || errNoMoreWrite != errLogServiceStopped || m_pLogStream->FLogEndEmitted() ) )
        {
            m_critLGBuf.Leave();
            return ErrERRCheck( JET_errLogWriteFail );
        }

        // Horrible layering violation, but we need to make sure that commit0's are always logged in trxid order
        //
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

            //  bail out if we're getting too close to the max
            //  precision for the BFOB0 approximate index
            //
            if ( lgenOutstanding > ( lgenTooDeepLimit - lgenCheckpointTooDeepMin / 2 ) ||
                 lgenTrxOutstanding > lgenTooDeepLimit / 2 )
            {
                switch ( lrtyp )
                {
                    case lrtypInit2:
                        //  the checkpoint may not be initialized if the last shutdown
                        //  was clean and we are initializing.  in this case, always
                        //  allow the init to succeed because we will update the
                        //  checkpoint shortly thereafter
                        //
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
                        //  attempt to permit clean shutdown, by letting
                        //  rollback and term log records go a little further
                        //
                        if ( lgenOutstanding <= ( lgenTooDeepLimit - lgenCheckpointTooDeepMin / 4 ) )
                        {
                            break;
                        }

                        //  FALL THROUGH

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

        //  if just initialized or no input since last write
        //

        // We should always be dealing with valid pointers.
        Assert( m_pbWrite >= m_pbLGBufMin );
        Assert( m_pbWrite < m_pbLGBufMax );
        Assert( m_pbEntry >= m_pbLGBufMin );
        Assert( m_pbEntry < m_pbLGBufMax );

        if ( m_pbLGFileEnd != pbNil )
        {
            Assert( m_isecLGFileEnd != 0 );
            if ( fNewGen && !fForcedNewGen )
            {
                //  we're trying to force a new gen, but a new gen is
                //  already in the midst of being created (for someone
                //  else), so wait for that to be completed and then
                //  try again (ie. return errLGNotSynchronous to the
                //  caller)
                //
                goto Restart;
            }
            else if ( m_fWaitForLogRecordAfterStopOnNewGen && !fStopOnNewGen )
            {
                //  the log stream is being stopped on a new generation,
                //  but we're not the thread that made the request, so
                //  to ensure that the thread that made the request can
                //  get its log record into the log buffer, don't fill
                //  up the log buffer
                //
                goto Restart;
            }
        }

        //  calculate available space
        //
        // m_pbWrite == m_pbEntry means log buffer is EMPTY
        if ( m_pbWrite > m_pbEntry )
        {
            cbAvail = ULONG( m_pbWrite - m_pbEntry );
        }
        else
        {
            cbAvail = ULONG( m_cbLGBuf + m_pbWrite - m_pbEntry );
        }

        Assert( cbAvail <= m_cbLGBuf );

        //  calculate sectors of buffer ready to write. Excluding
        //  the half filled sector.

        csecToExclude = ( cbAvail - 1 ) / m_pLogStream->CbSec() + 1;
        csecReady = m_csecLGBuf - csecToExclude;
        ibEntry = ULONG( m_pbEntry - m_pLogStream->PbSecAligned( m_pbEntry, m_pbLGBufMin ) );

        if ( !fNewGen || fForcedNewGen )
        {
            //  check if add this record, it will reach the point of
            //  the sector before last sector of current log file. Note we always
            //  reserve the last sector as a shadow sector.
            //  if it reach the point, check if there is enough space to put
            //  NOP to the end of log file and cbLGMSOverhead of NOP's for the
            //  first sector of next generation log file. And then set m_pbEntry
            //  there and continue adding log record.

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
                // If we failed to add the log records even when the log buffer
                // is already empty, it means we're trying to log too much at once.
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

                    const LONG csecAvailMax = m_pLog->CSecLGFile() - m_pLog->CSecLGHeader() - 4 /* margin */;
                    Assert( csecAvailMax > 0 );

                    // Assert that we're trying to log something which is truly big.
                    Assert( cbData > ( csecAvailMax * (LONG)m_pLog->CbLGSec() ) );
#endif // DEBUG

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
                //  if fNewGen==FALSE, someone else must have forced a new
                //  gen, but the contents of the log buffer haven't been
                //  written to it yet, so since we don't care to create
                //  a new gen ourselves, we can just add on to it
                //
                //  if fNewGen==TRUE, then fForcedNewGen==TRUE, which means
                //  that we've already been through the main loop once and
                //  forced a new generation, and now that we've forced
                //  the new generation, we're adding the log record to
                //  the log buffer
                //
                DWORD cbFileUsed;
                if ( m_pbLGFileEnd < pbLogRec )
                {
                    cbFileUsed = DWORD( pbLogRec - m_pbLGFileEnd );
                }
                else
                {
                    cbFileUsed = DWORD( m_cbLGBuf + pbLogRec - m_pbLGFileEnd );
                }
                // check if new gen is necessary - leave at least one byte free
                if ( cbFileUsed >= ( m_pLogStream->CSecLGFile() - 1 - m_pLogStream->CSecHeader() ) * m_pLogStream->CbSec() )
                {
                    // the new gen is full, we have to wait
                    goto Restart;
                }

                if  ( FLGSignalWrite() )
                {
                    PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
                }

                // There is enough space in the new gen
                break;
            }

            //  check if after adding this record, the sector will reach
            //  the sector before last sector. If it does, let's patch NOP.

            // check if new gen is necessary - leave at least one byte free

            cbFraction = ibEntry + cbReq;
            csecToAdd = ( cbFraction + m_pLogStream->CbSec() ) / m_pLogStream->CbSec();
            Assert( csecToAdd <= 0x7FFFFFFF );

            // The m_pLogStream->CSecLGFile() - 1 is because the last sector of the log file
            // is reserved for the shadow sector.
            if ( csecReady + m_isecWrite + csecToAdd > m_pLogStream->CSecLGFile() - 1 )
            {
                fNewGen = fTrue;
            }
            else
            {
                // Because m_csecLGBuf differs from JET_paramLogBuffers
                // because the log buffers have to be a multiple of the system
                // memory allocation granularity, the write threshold should be
                // half of the actual size of the log buffer (not the logical
                // user requested size).
                if ( csecReady >= m_csecLGBuf / 2 )
                {
                    //  reach the threshold, write before adding new record
                    //
                    // XXX
                    // The above comment seems misleading because although
                    // this tries to signal the write thread, the actual writing
                    // doesn't happen until we release m_critLGBuf which
                    // doesn't happen until we add the new log records and exit.
                    if  ( FLGSignalWrite() )
                    {
                        PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
                    }
                }

                // we can fit record in current gen, fall out of loop
                break;
            }
        }

        Assert( fNewGen );

        // last sector is the one that will be patched with NOP
        cbFraction = ibEntry;
        csecToAdd = ( cbFraction + m_pLogStream->CbSec() - 1 ) / m_pLogStream->CbSec();
        Assert( csecToAdd == 0 || csecToAdd == 1 );
        // we need to write at least one sector while rolling the log file
        // below or the write code gets very unhappy
        if ( csecReady == 0 && csecToAdd == 0 )
        {
            csecToAdd = 1;
        }
        isecLGFileEndT = m_isecWrite + csecReady + csecToAdd;
        Assert( isecLGFileEndT < m_pLogStream->CSecLGFile() );

        DWORD cbToFill = csecToAdd * m_pLogStream->CbSec() - ibEntry;


        // We fill up to the end of the file

        // This implicitly takes advantage of the knowledge that
        // we'll have free space up to a sector boundary -- in other
        // words, m_pbWrite is always sector aligned.

        // At least leave one byte free since we cannot distinguish between
        // completely full and completely empty buffer

        if ( cbAvail <= cbToFill )
        {
            //  available space is not enough to fill to end of file plus
            //  first sector of next generation. Wait for write to generate
            //  more space.
            goto Restart;
        }

        if ( cbToFill > 0 )
        {
            //  now we have enough space to patch.
            //  Let's set m_pbLGFileEnd for log write to generate new log file.

            pbLogRec = m_pbEntry;

            // We must be pointing into the main mapping of the log buffer.
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

        // Setup m_pbLGFileEnd to point to the sector boundary of the last
        // stuff that should be written to the log file. This is noticed
        // by ErrLGIWriteFullSectors() to switch to the next log file.
        Assert( pbNil == m_pbLGFileEnd );
        m_pbLGFileEnd = m_pbEntry;
        // m_pbLGFileEnd should be sector aligned
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

        // No need to setup m_lgposMaxWritePoint here since
        // writing thread will do that. Notice that another
        // thread can get in to m_critLGBuf before writing thread
        // and they can increase m_lgposMaxWritePoint.

        // send signal to generate new log file
        //
        //  UNDONE: does it matter if we do this before or
        //  after leaving critLGBuf?
        //
        if ( FLGSignalWrite() )
        {
            PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
        }

        // start all over again with new m_pbEntry, cbAvail etc.
        m_critLGBuf.Leave();

        if ( fStopOnNewGen )
        {
            //  we've scheduled a new gen, but might have failed to signal
            //  for a write, which is normally not a problem,
            //  (since whoever owns the signal will do the write
            //  for us) except in the case where:
            //      - whoever owns the signal has already written
            //        and is about to release it
            //      - fStopOnNewGen was specified, so we NEED
            //        the write to happen eventually
            //
            while ( m_fStopOnNewLogGeneration && !m_pLog->FNoMoreLogWrite() )
            {
                UtilSleep( cmsecWaitLogWrite );
                (void)FLGWriteLog( iorpLGWriteNewGen, fFalse, fFalse );
            }
        }

        fForcedNewGen = fTrue;

        // loop to retry adding the log record in the new log file just created
    } // end forever

    //  now we are holding m_pbEntry, let's add the log record.
    //
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


    //  setup m_lgposMaxWritePoint so it points to the first byte of log data that will NOT be making it to disk
    //
    //  in the case of a multi-sector write, this will point to the log record which "hangs over" the sector
    //      boundary; it will mark that log record as not being written and will thus prevent the buffer manager
    //      from writing the database page
    //
    //  in the case of a single-sector write, this will not be used at all

    //  calculate the sector boundary where the partial data begins
    //      (full-sector data is before this point -- there may not be any)

    pbSectorBoundary = m_pLogStream->PbSecAligned( m_pbEntry + cbReq, m_pbLGBufMin );

    if ( m_pLogStream->PbSecAligned( m_pbEntry, m_pbLGBufMin ) == pbSectorBoundary )
    {
        //  the new log record did not put us past a sector boundary
        //
        //  it was put into a partially full sector and does not hang over, so do not bother settting
        //      m_lgposMaxWritePoint
    }
    else
    {
        //  the new log record is part of a multi-sector write
        //
        //  if it hangs over the edge, then we cannot include it when calculating m_lgposMaxWritePoint

#ifdef DEBUG
        LGPOS lgposOldMaxWritePoint = m_lgposMaxWritePoint;
#endif  //  DEBUG

        GetLgposOfPbEntry( &m_lgposMaxWritePoint );
        if ( pbSectorBoundary == m_pbEntry + cbReq )
        {
            m_pLogStream->AddLgpos( &m_lgposMaxWritePoint, cbReq );
        }

        Assert( CmpLgpos( &m_lgposMaxWritePoint, &m_lgposLogRec ) >= 0 );

        // max write point must always be increasing.
        Assert( CmpLgpos( &lgposOldMaxWritePoint, &m_lgposMaxWritePoint ) <= 0 );

#ifdef DEBUG
        //  m_lgposMaxWritePoint should be pointing to the beginning OR the end of the last log record

        LGPOS lgposLogRecEndT = m_lgposLogRec;
        m_pLogStream->AddLgpos( &lgposLogRecEndT, cbReq );
        Assert( CmpLgpos( &m_lgposMaxWritePoint, &m_lgposLogRec ) == 0 ||
                CmpLgpos( &m_lgposMaxWritePoint, &lgposLogRecEndT ) == 0 );
#endif  //  DEBUG
    }

    pbLogRec = m_pbEntry;
    Assert( m_pLogBuffer->FIsFreeSpace( m_pbEntry, cbReq ) );

    m_pbEntry += cbReq;
    if ( m_pbEntry >= m_pbLGBufMax )
    {
        m_pbEntry -= m_cbLGBuf;
    }
    Assert( m_pLogBuffer->FIsUsedSpace( pbLogRec, cbReq ) );

    //  add a dummy fill record to indicate end-of-data
    //
    // XXX
    // Since we just did a bunch of AddLogRec()'s, how can m_pbEntry be
    // equal to m_pbLGBufMax? This seems like it should be an Assert().
    // Maybe in the case where the number of log records added is zero?
    // That itself should be an Assert()

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

        //  must access g_rgbDumpLogRec in g_critDBGPrint.

        pb = g_rgbDumpLogRec;
        for ( INT idata = 0; idata < cdata; idata++ )
        {
            UtilMemCpy( pb, rgdata[idata].Pv(), rgdata[idata].Cb() );
            pb += rgdata[idata].Cb();
        }

        LR  *plr    = (LR *)g_rgbDumpLogRec;

        //  we never explicitly log NOPs
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

//  GetLgposOfPbEntry( &lgposEntryT );

    //  monitor statistics
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

    //  indicate that we are about to copy our log data into the log buffer
    //
    const INT   iGroup  = m_msLGPendingCopyIntoBuffer.Enter();

    if ( fLGFlags & fLGFillPartialSector )
    {
        m_pLog->LGAddWastage( cbReq );
    }
    else
    {
        m_pLog->LGAddUsage( cbReq );
    }

    //  now we are done with allocating space for the log data in the log buffer.
    //
    m_critLGBuf.Leave();

    //  add all the data streams to the log buffer.  Cannot fail now since
    //  space is already reserved.
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

    //  done copying the log data into the log buffer
    //
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
        //  unconditionally reset m_fWaitForLogRecordAfterStopOnNewGen
        //  (should be safe, because we don't support concurrent
        //  use of fLGStopOnNewGen)
        //
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
        // This flag should always be reset on exit
        Assert( !m_fWaitForLogRecordAfterStopOnNewGen );
    }

    return err;

Restart:
    //  WARNING: a log write is not guaranteed,
    //  because a log write won't be signalled if
    //  we can't acquire the right to signal a log
    //  write (typically because a log write is
    //  already under way, but note that it could
    //  be at the very end of that process)
    //
    if ( FLGSignalWrite() )
    {
        PERFOpt( cLGCapacityWrite.Inc( m_pinst ) );
    }

    //  UNDONE: Does this check ever succeed?  If we only
    //  got to this point, I don't think m_lgposLogRec
    //  would have had a chance to advance.
    //
    fFormatJetTmpLog = m_pLogStream->FShouldCreateTmpLog( m_lgposLogRec );
    if ( fFormatJetTmpLog )
    {
        lgposLogRecTmpLog = m_lgposLogRec;
    }

    //  if we requested to stop on a new gen, then
    //  we should have been able to fit our log record
    //  into the log buffer
    //
    Assert( !fStopOnNewGen || !m_fWaitForLogRecordAfterStopOnNewGen );

    m_critLGBuf.Leave();

    PERFOpt( cLGStall.Inc( m_pinst ) );
    ETLogStall();
    err = ErrERRCheck( errLGNotSynchronous );
    goto FormatJetTmpLog;
}

//  waits until the log generation indicated becomes the current log ... only fails if the log becomes unwritable ...
//
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

//  waits until the log record indicated in the session has been written to disk
//
ERR LOG_WRITE_BUFFER::ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec )
{
    ERR     err         = JET_errSuccess;
    BOOL    fFillPartialSector = fFalse;

    //  if the log is disabled or we are in redo, skip the wait

    if ( m_pLog->FLogDisabled() || m_pLog->FRecovering() && m_pLog->FRecoveringMode() != fRecoveringUndo )
    {
        goto Done;
    }

    //  if our record has already been written to the log, no need to wait

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

    //  if the log is down, you're hosed

    if ( m_pLog->FNoMoreLogWrite() )
    {
        err = ErrERRCheck( JET_errLogWriteFail );
        m_critLGBuf.Leave();
        goto Done;
    }

    //  add this session to the log write wait queue

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

    // if the lgpos being written is in a partial sector, fill up the sector
    // before writing
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

    // Fill partial sector if needed before requesting write, ignore error
    // Have released buffer lock here, so someone else may have filled our
    // partial sector already and we may end up filling the current partial
    // sector, not really worth protecting it
    if ( fFillPartialSector )
    {
        (VOID)ErrLGLogRec( NULL, 0, fLGFillPartialSector, 0, NULL );
    }

    //  wait forever for our record to be written to the log
    //
    if ( ppib->asigWaitLogWrite.FTryWait() )
    {
        //  we were lucky in that the log was written out from under us concurrently.
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

    //  the log write failed

    if ( m_pLog->FNoMoreLogWrite() )
    {
        err = ErrERRCheck( JET_errLogWriteFail );
    }

    //  the log write succeeded

    else
    {
#ifdef DEBUG

        //  verify that our record is now on disk

        m_critLGBuf.Enter();
        Assert( CmpLgpos( &m_lgposToWrite, &ppib->lgposWaitLogWrite ) > 0 );
        m_critLGBuf.Leave();

#endif  //  DEBUG
    }

Done:
    Assert( !ppib->FLGWaiting() );
    Assert( err == JET_errLogWriteFail || err == JET_errSuccess );

    return err;
}

VOID
LOG_WRITE_BUFFER::VerifyAllWritten()
{
    // verify that everything in the log buffers have been
    // written to disk.
    m_pLogStream->LockWrite();
    m_critLGBuf.Enter();

    // we create a torn-write after a clean shutdown because we don't write the last LRCK record
    // (we weren't seeing it because PbGetEndOfLogData() was pointing AT the LRCK instead of PAST it)
    //
    //      make sure we have written up to m_pbEntry (instead of PbGetEndOfLogData())

    LGPOS lgposEndOfData;
    GetLgposOfPbEntry( &lgposEndOfData );
    // Everything in the log buffer better be written out to disk,
    // otherwise we're hosed.
    Assert( CmpLgpos( &lgposEndOfData, &m_lgposToWrite ) <= 0 );

    m_critLGBuf.Leave();
    m_pLogStream->UnlockWrite();
}

// Kicks off synchronous log writes until all log data is written

ERR LOG_WRITE_BUFFER::ErrLGWaitAllFlushed( BOOL fFillPartialSector )
{
    forever
    {
        LGPOS   lgposEndOfData;
        INT     cmp;

        m_critLGBuf.Enter();

        //      we create a torn-write after a clean shutdown because we don't write the last LRCK record
        //      (we weren't seeing it because PbGetEndOfLogData() was pointing AT the LRCK instead of PAST it)
        //
        //      make sure we wait until we write up to m_pbEntry (rather than PbGetEndOfLogData())

        GetLgposOfPbEntry ( &lgposEndOfData );
        cmp = CmpLgpos( &lgposEndOfData, &m_lgposToWrite );
        fFillPartialSector = fFillPartialSector && ( lgposEndOfData.ib != 0 );

        m_critLGBuf.Leave();

        if ( cmp > 0 )
        {
            // fill partial sector if needed
            if ( fFillPartialSector )
            {
                (VOID)ErrLGLogRec( NULL, 0, fLGFillPartialSector, 0, NULL );
            }

            // synchronously ask for write
            ERR err = ErrLGWriteLog( iorpLGFlushAll, fTrue );
            if ( err < 0 )
            {
                return err;
            }
        }
        else
        {
            // All log data is written, so we're done.
            break;
        }
    }

    m_pLogStream->LGAssertFullyFlushedBuffers();

    return JET_errSuccess;
}

//  ================================================================
VOID LOG_WRITE_BUFFER::AdvanceLgposToWriteToNewGen( const LONG lGeneration )
//  ================================================================
{
    Assert( m_critLGBuf.FOwner() );
    m_lgposToWrite.lGeneration = lGeneration;
    m_lgposToWrite.isec = (WORD) m_pLogStream->CSecHeader();
    m_lgposToWrite.ib = 0;
    // max write point may already be beyond the start of the file (if
    // log records were added) - do not move it back in that case
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
LOCAL LONG g_dtickNoWriteInterval = 1000;  // 1 second for retail
LOCAL LONG g_dtickDecommitTaskDelay = 2000; // 2 seconds timer for retail
#endif

// When there are no write activities in the past g_dtickNoWriteInterval, do decomit.
// Otherwise, reschedule the decomit task with g_dtickDecommitTaskDelay delay. 
VOID LOG_WRITE_BUFFER::DecommitLogBufferTask( VOID * pvGroupContext, VOID * pvLogWriteBuffer )
{
    LOG_WRITE_BUFFER *pLogWriteBuffer = (LOG_WRITE_BUFFER *) pvLogWriteBuffer;
    BOOL bShouldDecommit = fFalse;
    
    pLogWriteBuffer->m_critLGWaitQ.Enter();
    // No Decommit should be done if there are write activities in the past g_dtickNoWriteInterval period.
    if ( TickCmp( TickOSTimeCurrent(), pLogWriteBuffer->m_tickLastWrite ) < g_dtickNoWriteInterval )
    {
        OSTimerTaskScheduleTask( pLogWriteBuffer->m_postDecommitTask, pLogWriteBuffer, g_dtickDecommitTaskDelay, g_dtickDecommitTaskDelay );
    }
    else
    {
        bShouldDecommit = fTrue;
        // m_fDecommitTaskScheduled is protected by m_critLGWaitQ
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

/*
 *  Log write thread is signalled to write log asynchronously when at least
 *  cThreshold disk sectors have been filled since last write.
 */
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
        //  there is no room in the log buffer and someone else is currently
        //  writing the log or we failed to write the log ourselves due to some
        //  kind of transient condition
        //  wait for the log to be written and for more log buffer space
        //
        m_semWaitForLogBufferSpace.FAcquire( cmsecWaitLogWrite );
    }

    //  Restore state checking
    //
    FOSSetCleanupState( fCleanUpStateSaved );
}

BOOL LOG_WRITE_BUFFER::FLGWriteLog( const IOREASONPRIMARY iorp, BOOL fHasSignal, const BOOL fDisableDeadlockDetection )
{
    BOOL fWritten = fFalse;
    
    //  try to grab the right to signal a log write to prevent others from
    //  trying to signal a log write.  we do this because we are going to
    //  try to write the log ourselves and don't want a new async write to
    //  beat us to the punch, unless this is coming from the write task
    //
    if ( !fHasSignal )
    {
        fHasSignal = m_semLogSignal.FTryAcquire();
    }

    //  assert that the signal is not available anymore. if this function is being called
    //  by the log write task (i.e., from LOG::LGWriteLog_()) we didn't need to acquire it
    //  above because the right to signal a write has already been acquired by the signaler, so
    //  we will also assert this condition. the signal will then be released so that a new/
    //  write task can be signaled again by a foreground thread.
    //
    Assert( !fHasSignal || ( m_semLogSignal.CAvail() == 0 ) );

    //  try to write the log if no one else is currently writing it.  if
    //  someone else is already writing it then this signal will be ignored
    //
    if ( m_semLogWrite.FTryAcquire() )
    {
        //  we may need to disable sync debugging below because we may be logically
        //  becoming another context and the sync library is not flexible enough to
        //  understand this
        //

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

    //  signal that this log write request has been completed and permit a new
    //  log write request to be made
    //
    //  NOTE:  this must be the last reference made to the LOG object if we are
    //         running from the write task

    if ( fHasSignal )
    {
        m_semLogSignal.Release();
    }

    return fWritten;
}

VOID LOG_WRITE_BUFFER::LGLazyCommit_( VOID *pGroupContext, VOID *pLogBuffer )
{
    LOG_WRITE_BUFFER *pLogWriteBuffer = (LOG_WRITE_BUFFER *)pLogBuffer;

    // zero out the next lazy commit tick, so the next guy reschedules this
    //
    // Note that it is important to do this before reading the lgposCommit
    // otherwise, someone may update the lgposCommit expecting this callback
    // to pick it up which may never happen
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

    // update LGPOS to the max LGPOS of transactions needing lazy commit
    //
    // Note that it is important to update this before checking for whether
    // there is already a pending callback because, otherwise the callback can
    // finish executing before you may get a chance to update this
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

    // see if there is a lazy commit write already scheduled and whether it
    // needs to be scheduled for an earlier time
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

// Returns fTrue if there are people waiting for log records to
// be written after *plgposToWrite

BOOL LOG_WRITE_BUFFER::FWakeWaitingQueue( const LGPOS * const plgposToWrite )
{
    /*  go through the waiting list and wake those whose log records
    /*  were written in this batch.
    /*
    /*  also wake all threads if the log has gone down
    /**/
    // If anyone is interested in log commits, notify them now
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

    /*  wake it up!
    /**/
    m_critLGWaitQ.Enter();

    PIB *   ppibT               = m_ppibLGWriteQHead;
    BOOL    fWaitersExist       = fFalse;

    while ( ppibNil != ppibT )
    {
        //  WARNING: need to save off ppibNextWaitWrite
        //  pointer because once we release any waiter,
        //  the PIB may get released
        PIB * const ppibNext    = ppibT->ppibNextWaitWrite;

        // XXX
        // This should be < because the LGPOS we give clients to wait on
        // is the LGPOS of the start of the log record (the first byte of
        // the log record). When we write, the write LGPOS points to the
        // first byte of the log record that did *NOT* make it to disk.
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

            //  WARNING: cannot reference ppibT after this point
            //  because once we free the waiter, the PIB may
            //  get released
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

// Writes all the full sectors we have in the log buffer to disk.

ERR LOG_WRITE_BUFFER::ErrLGIWriteFullSectors(
    const IOREASONPRIMARY iorp,
    const UINT          csecFull,
    // Sector to start the write at
    const UINT          isecWrite,
    // What to write
    const BYTE* const   pbWrite,
    // Whether there are clients waiting for log records to be written
    // after we finish our write
    BOOL* const         pfWaitersExist,
    // The LGPOS of the last record not completely written out in this write.
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

    // m_critLGWrite guards m_fHaveShadow
    m_pLogStream->LGAssertWriteOwner();

    // checksum each page before writing
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

        //  if the first sector to be written had a shadow sector (i.e. it was a partial sector and now the first sector of the current write will overwrite that partial sector)
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
        // shadow was written, which means, we need to write
        // out the first sector of our bunch of full sectors,
        // then the rest.

        // start of write has to be in data portion of log file.
        // last sector of log file is reserved for shadow sector
        Assert( isecToWrite >= m_pLogStream->CSecHeader() && isecToWrite < ( m_pLogStream->CSecLGFile() - 1 ) );
        // make sure we don't write past end
        Assert( isecToWrite + 1 <= ( m_pLogStream->CSecLGFile() - 1 ) );

        if ( pbToWrite == m_pbLGBufMax )
        {
            pbToWrite = m_pbLGBufMin;
        }
        
        // We must be pointing into the main mapping of the log buffer.
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
            //  If there are no more sectors available to overwrite the shadow sector, then we must overwrite the shadow with the copy of the current sector.
            //  m_fHaveShadow is set to false in this case because the next log flush will not overwrite the original. It will just overwrite the shadow sector.

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
            // write out rest of the full sectors, if any

            // start of write has to be in data portion of log file.
            Assert( isecToWrite >= m_pLogStream->CSecHeader() && isecToWrite < ( m_pLogStream->CSecLGFile() - 1 ) );
            // make sure we don't write past end
            Assert( isecToWrite + csecToWrite <= ( m_pLogStream->CSecLGFile() - 1 ) );

            // If the end of the block to write goes past the end of the
            // first mapping of the log buffer, we write two blocks asynchronously.
            if ( pbToWrite + ( m_pLogStream->CbSec() * csecToWrite ) > m_pbLGBufMax )
            {
                m_cLGWrapAround++;
                
                // use two AsycIO/GatherIO to write the wrapped around sector data
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
        // no shadow, which means the last write ended "perfectly"
        // on a sector boundary, so we can just blast out in 1 I/O

        // start of write has to be in data portion of log file.
        Assert( isecToWrite >= m_pLogStream->CSecHeader() && isecToWrite < ( m_pLogStream->CSecLGFile() - 1 ) );
        // make sure we don't write past end
        Assert( isecToWrite + csecToWrite <= ( m_pLogStream->CSecLGFile() - 1 ) );

        // If the end of the block to write goes past the end of the
        // first mapping of the log buffer, this is considered using
        // the VM wraparound trick.
        if ( pbToWrite + ( m_pLogStream->CbSec() * csecToWrite ) > m_pbLGBufMax )
        {
            m_cLGWrapAround++;

            // use two AsycIO/GatherIO to write the wrapped around sector data
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

    // There is no shadow sector on the disk for the last chunk of data on disk.
    m_fHaveShadow = fFalse;

    // Free up space in the log buffer since we don't need it
    // anymore.
    // Once these new log records hit the disk, we should
    // release the clients waiting.

    m_critLGBuf.Enter();    // <====================

    // what we wrote was used space
    Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, csecFull * m_pLogStream->CbSec() ) );

    // The new on disk LGPOS should be increasing or staying
    // the same (case of a partial sector write, then a really full
    // buffer with a big log record, then a full sector write that
    // doesn't write all of the big log record).
    Assert( CmpLgpos( &m_lgposToWrite, &lgposWriteEnd ) <= 0 );
    // remember the write point we setup earlier at log adding time
    m_lgposToWrite = lgposWriteEnd;
    if ( fFlushed )
    {
        m_pLog->LGSetFlushTipWithLock( lgposWriteEnd );
    }

#ifdef DEBUG
    LGPOS lgposEndOfWrite;
    // Position right after what we wrote up to.
    m_pLogBuffer->GetLgpos( const_cast< BYTE* >( pbToWrite ), &lgposEndOfWrite, m_pLogStream );
    // The write point should be less than or equal to the LGPOS of
    // the end of what we physically put to disk.
    Assert( CmpLgpos( &lgposWriteEnd, &lgposEndOfWrite ) <= 0 );
#endif
    m_isecWrite = isecToWrite;
    // free space in log buffer
    m_pbWrite = const_cast< BYTE* >( pbToWrite );

    // now freed
    Assert( m_pLogBuffer->FIsFreeSpace( pbWrite, csecFull * m_pLogStream->CbSec() ) );

    pbFileEndT = m_pbLGFileEnd;

    PERFOpt( cbLGBufferUsed.Set(
                    m_pinst,
                    LONG( m_pbWrite <= m_pbEntry ?
                                m_pbEntry - m_pbWrite :
                                m_cbLGBuf - ( m_pbWrite - m_pbEntry ) ) ) );

    m_critLGBuf.Leave();    // <====================

    // We want to wake clients before we do other time consuming
    // operations like updating checkpoint file, creating new generation, etc.

    (VOID) FWakeWaitingQueue( &lgposWriteEnd );

    // Now that we have produced more empty buffer space, release any threads
    // that are waiting on space in the log buffer

    m_semWaitForLogBufferSpace.ReleaseAllWaiters();

    Assert( pbNil != pbToWrite );
    // If we wrote up to the end of the log file
    if ( pbFileEndT == pbToWrite )
    {
        (VOID) m_pLog->ErrLGUpdateCheckpointFile( fFalse );

        Call( m_pLogStream->ErrLGNewLogFile( m_pLogStream->GetCurrentFileGen(), fLGOldLogExists ) );

    }

HandleError:
    // If there was an error, log is down and this will wake
    // everyone, else wake the people we put to disk.
    Assert( err >= JET_errSuccess || err == errOSSnapshotNewLogStopped || m_pLog->FNoMoreLogWrite() );
    const BOOL fWaitersExist = FWakeWaitingQueue( &lgposWriteEnd );
    if ( pfWaitersExist )
    {
        *pfWaitersExist = fWaitersExist;
    }

    return err;
}

// Write the last sector in the log buffer which happens to
// be not completely full.

ERR LOG_WRITE_BUFFER::ErrLGIWritePartialSector(
    const IOREASONPRIMARY iorp,
    // Pointer to the end of real data in this last sector
    // of the log buffer
    BYTE*   pbWriteEnd,
    UINT    isecWrite,
    BYTE*   pbWrite,
    const LGPOS& lgposWriteEnd
    )
{
    ERR     err = JET_errSuccess;
    BOOL    fFlushed = fFalse;

    PERFOpt( cLGPartialSegmentWrite.Inc( m_pinst ) );

    // data writes must be past the header of the log file.
    Assert( isecWrite >= m_pLogStream->CSecHeader() );
    // The real data sector can be at most the 2nd to last sector
    // in the log file.
    Assert( isecWrite < m_pLogStream->CSecLGFile() - 1 );

    // checksum page before writing (need to make copy since we do not hold
    // buffer lock
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
    // write real data and the shawdow sector.
    // The shadow can be at most the last sector in the log file.
    Assert( isecWrite + 1 < m_pLogStream->CSecLGFile() );
    Assert( m_pLogStream->PbSecAligned( pbWrite, m_pbLGBufMin ) == pbWrite );

    if ( m_fHaveShadow )
    {
        m_pLogStream->AccumulateSectorChecksum( pSegHdr->checksum, true );

        // write real data
        Call( m_pLogStream->ErrLGWriteSectorData(
                    ior,
                    m_pLogStream->GetCurrentFileGen(),
                    QWORD( isecWrite ) * m_pLogStream->CbSec(),
                    m_pLogStream->CbSec() * 1,
                    m_pvPartialSegment,
                    LOG_FLUSH_WRITE_3_ERROR_ID,
                    &fFlushed ) );

        m_pLogStream->LGAssertWriteOwner();

        // write shadow sector
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

        // use two AsycIO/GatherIO to write the real data and shadow sector
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

    // Flag for ErrLGIWriteFullSectors() to split up a big I/O
    // into 2 I/Os to prevent from overwriting shadow and then killing
    // the real last data sector (because order of sector updating is
    // not guaranteed when writing multiple sectors to disk).
    m_fHaveShadow = fTrue;

    // Free up buffer space
    m_critLGBuf.Enter();    // <====================

    Assert( pbWrite < pbWriteEnd );
    // make sure we wrote stuff that was valid in the log buffer.
    // We are not going to write garbage!!
    Assert( m_pLogBuffer->FIsUsedSpace( pbWrite, ULONG( pbWriteEnd - pbWrite ) ) );

    // write end is end of real data in log buffer.
    m_lgposToWrite = lgposWriteEnd;
    if ( fFlushed )
    {
        m_pLog->LGSetFlushTipWithLock( lgposWriteEnd );
    }

    // m_pbWrite and m_isecWrite are already setup fine.
    // m_pbWrite is still pointing to the beginning of this
    // partial sector which will need to be written again
    // once it fills up.
    // m_isecWrite is still pointing to this sector on disk
    // and that makes sense.

    m_critLGBuf.Leave();    // <====================

HandleError:

    // We don't care if anyone is waiting, since they can
    // ask for a write, themselves.
    // Wake anyone we written to disk.
    Assert( err >= JET_errSuccess || m_pLog->FNoMoreLogWrite() );
    (VOID) FWakeWaitingQueue( &lgposWriteEnd );

    return err;
}

// Called to write out more left over log buffer data after doing a
// full sector write. The hope was that while the full sector write
// was proceeding, more log records were added and we now have another
// full sector write ready to go. If no more stuff was added, just
// finish writing whatever's left in the log buffer.

ERR LOG_WRITE_BUFFER::ErrLGIDeferredWrite( const IOREASONPRIMARY iorp, const BOOL fWriteAll )
{
    ERR     err = JET_errSuccess;
    BYTE*   pbEndOfData = pbNil;
    UINT    isecWrite;
    BYTE*   pbWrite = pbNil;
    LGPOS   lgposWriteEnd = lgposMax;

    m_critLGBuf.Enter();

    //  before writing, must wait for everyone with
    //  pending writes to the log buffer (we're still in critLGBuf,
    //  so we're guaranteed no new writes will come in)
    //
    m_msLGPendingCopyIntoBuffer.Partition();    //  no completion function, so this will sync wait

    if ( pbNil == m_pbLGFileEnd )
    {
        // get a pointer to the end of real log data.
        pbEndOfData = m_pbEntry;
        // need max write point for possible full sector write coming up
        lgposWriteEnd = m_lgposMaxWritePoint;
    }
    else
    {
        pbEndOfData = m_pbLGFileEnd;
        Assert( m_pLogStream->PbSecAligned( pbEndOfData, m_pbLGBufMin ) == pbEndOfData );
        // max write point for end of file
        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposWriteEnd, m_pLogStream );
    }

    // Get current values.
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
    // Follow same algorithm as below, but just make sure space is used properly
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
        // we don't care if anyone is waiting, since they'll ask for
        // a write themselves, since this means they got into the buffer
        // during our I/O.
        Call( ErrLGIWriteFullSectors( iorp, csecFull, isecWrite, pbWrite, NULL, lgposWriteEnd ) );
    }

HandleError:

    return err;
}

// Write has been requested. Write some of the data that we have in
// the log buffer. This is *NOT* guaranteed to write everything in the
// log buffer. We'll only write everything if it goes right up to
// a sector boundary, or if the entire log buffer is being waited on
// (ppib->lgposWaitLogWrite).

ERR LOG_WRITE_BUFFER::ErrLGWriteLog( const IOREASONPRIMARY iorp, const BOOL fWriteAll )
{
    ERR                     err     = JET_errSuccess;

    m_pLogStream->LockWrite();

#ifdef DEBUG
    m_dwDBGLogThreadId = DwUtilThreadId();
#endif

    //  try to write the log
    //
    const BOOL fLogDownBeforeWrite = m_pLog->FNoMoreLogWrite();
    err = ErrLGIWriteLog( iorp, fWriteAll );
    const BOOL fLogDownAfterWrite = m_pLog->FNoMoreLogWrite();

    //  if the log just went down for whatever reason, report it
    //
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
    // Set this to max so we'll Assert() or hang elsewhere if this
    // is used without really being initialized.
    lgposWriteEnd = lgposMax;

    m_critLGBuf.Enter();    // <===================================


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

    // XXX
    // gross temp hack to prevent trying to write during ErrLGSoftStart
    // and recovery redo time.
    if ( ! m_fNewLogRecordAdded )
    {
        m_critLGBuf.Leave();
        err = JET_errSuccess;
        goto HandleError;
    }

    //  before writing, must wait for everyone with
    //  pending writes to the log buffer (we're still in critLGBuf,
    //  so we're guaranteed no new writes will come in)
    //
    m_msLGPendingCopyIntoBuffer.Partition();    //  no completion function, so this will sync wait

    if ( pbNil != m_pbLGFileEnd )
    {
        fNewGeneration = fTrue;
        pbEndOfData = m_pbLGFileEnd;
        Assert( m_pLogStream->PbSecAligned( pbEndOfData, m_pbLGBufMin ) == pbEndOfData );

        // set write point for full sector write coming up
        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposWriteEnd, m_pLogStream );
    }
    else
    {
        LGPOS   lgposEndOfData = lgposMin;

        // get a pointer to the end of real log data.
        pbEndOfData = m_pbEntry;

        m_pLogBuffer->GetLgpos( pbEndOfData, &lgposEndOfData, m_pLogStream );

        // If all real data in the log buffer has been written,
        // we're done.
        if ( CmpLgpos( &lgposEndOfData, &m_lgposToWrite ) <= 0 )
        {
            m_critLGBuf.Leave();
            err = JET_errSuccess;
            goto HandleError;
        }

        // need max write point for possible full sector write coming up
        lgposWriteEnd = m_lgposMaxWritePoint;
    }

    // Get current values.
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

    // Follow same algorithm as below to do assertions inside of crit. section

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
        // No full sectors, so just write the partial and be done
        // with it.
        Assert( fPartialSector );
        Call( ErrLGIWritePartialSector( iorp, pbEndOfData, isecWrite, pbWrite, lgposWriteEnd ) );
    }
    else
    {
        // Some full sectors to write once, so blast them out
        // now and hope that someone adds log records while we're writing.
        BOOL fWaitersExist = fFalse;
        Call( ErrLGIWriteFullSectors( iorp, csecFull, isecWrite, pbWrite,
            &fWaitersExist, lgposWriteEnd ) );

        //  HACK (SOMEONE): we assume that we can remove following fPartial sector verification
        //  but SOMEONE is not sure. So when we create new generation and there are waiters
        //  left repeat the write check.
        if ( fWaitersExist && fNewGeneration )
        {
            goto Repeat;
        }

        // find out if we still got data from old generation
        m_critLGBuf.Enter();
        fNewGeneration = ( pbNil != m_pbLGFileEnd );
        m_critLGBuf.Leave();
        if ( fNewGeneration )
        {
            goto Repeat;
        }

        // Only write some more if we have some more, and if
        // there are people waiting for it to be written.
        if ( fPartialSector && fWaitersExist )
        {
            // If there was previously a partial sector in the log buffer
            // the user probably wanted the log recs to disk, so do the
            // smart deferred write.
            Call( ErrLGIDeferredWrite( iorp, fWriteAll ) );
        }
    }


HandleError:

    //  should not need to wake anyone up except possibly
    //  the snapshot freeze thread on error -- other waiters
    //  should be handled by the other functions
    //
    if ( m_pLog->FNoMoreLogWrite() && m_fStopOnNewLogGeneration )
    {
        //  release thread waiting on snapshot freeze (note: I
        //  didn't bother resetting m_fStopOnNewLogGeneration
        //  because the instance is being forced to shut down
        //  anyway and leaving the flag alone may provide us with
        //  some forensic evidence in case we ever need to debug
        //  something in the future and we need to piece together
        //  what happened)
        //
        Assert( !m_fLogPaused );
        m_sigLogPaused.Set();
    }

    return err;
}

ERR LGInitTerm::ErrLGSystemInit( void )
{
    ERR err = JET_errSuccess;

    //  init our global task manager which we use to async write the log
    //
    err = g_taskmgrLog.ErrTMInit( min( 8 * CUtilProcessProcessor(), 100 ) );

    return err;
}

VOID LGInitTerm::LGSystemTerm( void )
{
    //  term our global task manager
    //
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

    // Create m_postDecommitTask.
    Assert( m_postDecommitTask == NULL );
    // Not much benefit for decommitting log buffer for server scenarios.
    // Leave it on for low/medium config and debug builds.
    if ( OnDebugOrRetail( fTrue, fFalse ) || FJetConfigMedMemory() || FJetConfigLowMemory() )
    {
        CallR( ErrOSTimerTaskCreate( DecommitLogBufferTask, this, &m_postDecommitTask ) );
    }

    // Create m_postLazyCommitTask.
    Assert( m_postLazyCommitTask == NULL );
    CallR( ErrOSTimerTaskCreate( LGLazyCommit_, this, &m_postLazyCommitTask ) );

    return JET_errSuccess;
}

VOID LOG_WRITE_BUFFER::LGTasksTerm( void )
{
    AtomicExchange( (LONG *)&m_tickNextLazyCommit, 0 );
    m_semLogWrite.Acquire();
    m_semLogSignal.Acquire();
    
    // Stop and delete m_postLazyCommitTask.
    if ( m_postLazyCommitTask )
    {
        OSTimerTaskCancelTask( m_postLazyCommitTask );
        OSTimerTaskDelete( m_postLazyCommitTask );
        m_postLazyCommitTask = NULL;
    }
    
    // Stop and delete m_postDecommitTask.
    if ( m_postDecommitTask )
    {
        OSTimerTaskCancelTask( m_postDecommitTask );
        OSTimerTaskDelete( m_postDecommitTask );
        m_postDecommitTask = NULL;
    }
}

