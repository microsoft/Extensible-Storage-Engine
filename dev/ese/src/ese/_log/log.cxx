// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "logstd.hxx"
#include <ctype.h>
#include <_logredomap.hxx>

//  Source Insight users:
//  ------------------
//  If you get parse errors with this file, open up C.tom in your Source
//  Insight directory, comment out the "TRY try {" line with a semi-colon,
//  and then add a line with "PERSISTED" only.
//
//  If you don't use Source Insight, you are either a fool or else there is
//  a better editor by the time you read this.

//  D O C U M E N T A T I O N ++++++++++++++++++++++++++++
//
//  FASTFLUSH Physical Logging Overview
//  ==========================
//  For information on how the FASTFLUSH Physical Logging works, please see
//  ese\doc\ESE Physical Logging.doc.
//
//  Asynchronous Log File Creation Overview
//  ============================
//  Background
//  ----------
//  This text uses 5MB as an example for the log file size (configurable via
//  JET_paramLogFileSize) and 1MB as the maximum write size (configurable via
//  JET_paramMaxCoalesceWriteSize). 512KB is frequently used as being half
//  of the maximum write size.
//  Before asynchronous log file creation, ESE would stall in ErrLGNewLogFile()
//  when it would create and format a new 5MB log file (formatting means
//  applying a special signature pattern to the file which is used to determine
//  if corruption has occurred; log file size is settable via a Jet system
//  parameter). 5MB of I/O isn't that fast, especially considering that NTFS
//  makes extending I/Os synchronous plus each of the 1MB I/Os causes a write
//  to the MFT.
//
//  Solution
//  -------
//  Once ErrLGNewLogFile() is about to return (since it has completed setting
//  up the next log file), we should create edbtmp.jtx/log immediately (or rename
//  an archived edb%05X.jtx/log (or edb%08X.jtx/log if the log generation is over
//  0xFFFFF) file in the case of circular logging). Then we set a "TRIGGER" for the
//  first asynchronous 1MB formatting I/O to start once we have logged 512K to the
//  log buffer.
//
//  What is this TRIGGER about?
//  -------------------------
//  One way we could have done this is just to immediately start a 1MB
//  asynchronous formatting I/O, then once it finishes, issue another one
//  immediately. Unfortunately back-to-back 1MB I/Os basically consume
//  the disk and keep it busy -- thus no logging would be able to be done
//  to edb.jtx/log since we'd be busy using the disk to format edbtmp.jtx/log!
//  (I actually determined this experimentally with a prototype).
//
//  The trigger allows us to throttle I/O to the edbtmp.jtx/log that we're
//  formatting, so that we log 1MB of data to the in-memory log buffer, then
//  write 1MB to edbtmp.jtx/log, then... etc.
//
//  The trigger is handled such that once we pass the trigger, we will
//  issue a task that will perform a synch (or potentially multiple)
//  formatting I/O(s), then once it completes AND we pass the next trigger, we will
//  issue the next, etc. If we reach ErrLGNewLogFile() before edbtmp.jtx/log is
//  completely formatted (or there is currently an outstanding task), we will stop
//  the asynch triggering mechanism and "cancel" any potentially pending
//  tasks (by making the number of pending I/Os equal to zero).
//  Then, we format the rest of the file if necessary.
//
//  Why is the policy based on how much we've logged to the log buffer
//  and not how much we've flushed to edb.jtx/log?
//  --------------------------------------------------------------
//  In the case of many lazy commit transactions, waiting for a log flush
//  may be a long time with a large log buffer (i.e. there are some
//  performance recommendations that Exchange Servers should
//  have log buffers set to `9000' which is 9000 * 512 bytes = ~4.4MB;
//  BTW, do not set the log buffers equal to the log file size or greater or
//  you will get some bizarre problems). So in this case, we should try to
//  format edbtmp.jtx/log while those lazy commit transactions are coming in,
//  especially since they're not causing edb.jtx/log to be used at all.
//
//  Why set the trigger to 512K instead of issuing the first I/O immediately?
//  ----------------------------------------------------------------
//  In ErrLGNewLogFile() we're doing a lot with files, deleting unnecessary
//  log files in some circumstances, creating files, renaming files, etc. In other
//  words, this is going to take a while since we have to wait for NTFS to
//  update metadata by writing to the NTFS transaction log, at a minimum.
//  While we're waiting for NTFS to do this stuff, it is pretty likely that some
//  clients are doing transactions and they are now waiting for their commit
//  records to hit the disk -- in other words, they are waiting for their stuff
//  to be flushed to edb.jtx/log.
//
//  If we write 1MB to edbtmp.jtx/log now, those clients will have to wait until
//  the 1MB hits the disk before their records hit the disk.
//
//  Thus, this is why we wait for 512K to be logged to the log buffer -- as
//  a heuristic wait.
//
//  If back-to-back I/Os are bad, why do we post a task to issue potentially
//  several sequential I/Os at a time?
//  ----------------------------------------------------------------
//  Under nominal load, only one I/O will be issued at a time. However, we
//  have seen cases in which the temp log filling falls behind and by the time
//  we reach ErrLGNewLogFile(), the temp log is still far from completely formatted,
//  causing user threads to hang waiting for the rest of the log to be filled.
//  Issuing multiple back-to-back I/Os is an effort to avoid this dive in terms
//  of user experience. So, we are sacrificing more I/O during normal log filling
//  to provide a smoother and more uniform latency and response time, even though
//  the average use throughput (replaces/sec, for example) might not change
//  significantly.
//
//  What about error handling with this asynch business?
//  -----------------------------------------------
//  If we get an error creating the new edbtmp.jtx/log, we'll handle the error
//  later when ErrLGNewLogFile() is next called. The same with errors from
//  any asynch I/O.
//
//  We handle all the weird errors with disk-full and using reserve log files, etc.



//  constants

const LGPOS     lgposMax = { 0xffff, 0xffff, 0x7fffffff };
const LGPOS     lgposMin = { 0x0,  0x0,  0x0 };

#ifdef DEBUG
CCriticalSection g_critDBGPrint( CLockBasicInfo( CSyncBasicInfo( szDBGPrint ), rankDBGPrint, 0 ) );
#endif  //  DEBUG

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

#endif // PERFMON_SUPPORT

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

    //  These should have been cleaned up earlier, but there are cases where we might not
    //  due to errors in cleanup functions (see LOG::ErrLGRIEndAllSessionsWithError()).
    //  Just making sure we won't leak.

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
        //  If we get an error here, this is the best we can do.

        OSStrCbFormatW( wszAbsPath, sizeof( wszAbsPath ),
                    L"%ws [%i (0x%08x)]",
                    // UNDONE: if we recompile this unicode, since this isn't a _TCHAR, we'll get giberish for the log name.
                    m_pLogStream->LogName(),    // closest thing we have to pfapi's path
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


//********************* INIT/TERM **************************
//**********************************************************

//
//  Initialize global variablas and threads for log manager.
//
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

    //  assuming everything will work out
    //
    ResetNoMoreLogWrite();

    // first signal callback that this is the beginning of logging
    m_pLogStream->ErrEmitSignalLogBegin();

    //  Initialize trace mechanism

    m_pttFirst = NULL;
    m_pttLast = NULL;

    // SOFT_HARD: leave, not important
    Assert( m_fHardRestore || m_pLogStream->LogExt() == NULL );
    Assert( !m_fHardRestore || m_pLogStream->LogExt() != NULL );

    // If they call JetInit AFTER JetRestoreInstance
    // the two log related semaphors will NOT be released.
    // The semaphores were released on LOG::LOG and
    // aquired back at the end of JetRestoreInstance (in ErrLGTerm)
    //
    Call( m_pLogWriteBuffer->ErrLGTasksInit() );

    //  Multiple things make me nervous here, this haphazard way of initing 
    //  the checkpoint memory vs. file, and the fact that we init portions 
    //  of this memory and not others, as well as the fact that we touched 
    //  off LGTasksInit before this is init'd.
    Call( ErrLGICheckpointInit( pfNewCheckpointFile ) );

    memset( &m_signLog, 0, sizeof( m_signLog ) );
    m_fSignLogSet = fFalse;

    //  determine if we are in "log sequence end" mode

    // Note, this code doesn't actually check properly if we ONLY have edb.jtx/log, b/c ErrLGGetGenerationRange()
    // doesn't actually retrieve the max gen from edb.jtx/log, so we could consider removing this code?
    (void)m_pLogStream->ErrLGGetGenerationRange( SzParam( m_pinst, JET_paramLogFilePath ), &lGenMin, &lGenMax, m_pLogStream->LogExt() ? fFalse : fTrue );
    lGenMax += 1;   //  assume edb.jtx/log exists (if not, who cares -- if they hit this, they have to shutdown and wipe the log anyway)
    if ( lGenMax >= lGenerationMax )
    {
        m_pLogStream->SetLogSequenceEnd();
    }

    if ( 0 == lGenMin )
    {
        // There is no min-log to train our m_lgenInitial off of, so we use the registry (if it exists) ...

        // I think this still doesn't handle one case, where we both 1. do not have a checkpoint, 
        // and 2. have only one log (edb.jtx) and thus we couldn't get the the min-log above. Verified,
        // yes this doesn't handle that exact case ... this is fixed up in ErrLGSoftStart().
        WCHAR wsz[11];
        if ( FOSConfigGet( L"DEBUG", L"Initial Log Generation", wsz, sizeof(wsz) ) )
        {
            lGenMin = _wtoi( wsz );
            lGenMin = (lGenMin == 0) ? 1 : lGenMin; // zero is an invalid initial log gen.
        }
        else
        {
            //  no registry setting either, 1 is our default initial log gen ...
            lGenMin = 1;
        }
    }
    // else if there was already a log stream / sequence existing, upgrade our lgenInitial to that.
    Assert( lGenMin );

    //  We might consider moving this into the places ErrLGInit is called, namely ErrLGSoftStart, 
    //  ErrLGRestore, and ErrLGRSTExternalRestore(), all of which have a great deal more context on
    //  which log files actually exist and are valid.
    LGISetInitialGen( lGenMin );

    m_fLogInitialized = fTrue;

    return err;

HandleError:
    m_pLogWriteBuffer->LGTasksTerm();

    m_pLogStream->LGTerm();

    return err;
}


//  Terminates update logging.  Adds quit record to in-memory log,
//  flushes log buffer to disk, updates checkpoint and closes active
//  log generation file.  Frees buffer memory.
//
//  RETURNS    JET_errSuccess, or error code from failing routine
//
ERR LOG::ErrLGTerm( const BOOL fLogQuitRec )
{
    ERR         err             = JET_errSuccess;
    BOOL        fTermFlushLog   = fFalse;
    LE_LGPOS    le_lgposStart;
    LGPOS       lgposQuit;

    //  if logging has been initialized, terminate it!
    //
    if ( !m_fLogInitialized )
    {
        if ( m_pshadlog )
        {
            //  quit the alternate / secondary log
            err = ErrLGShadowLogTerm();
            // assert not thread safe, but on term, so we shouldn't have trouble here.
            Assert( NULL == m_pshadlog );
        }

        // Note: Because ErrLGTerm() is called from JetRestore, ExternalRestore() and for a failing
        // JetInit that is past log initialization, we have to defensively only trigger term sequences
        // when we're actually in / tracking term - which we can tell with .FActiveSequence().
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

    //  last written sector should have been written during final flush
    //
    le_lgposStart = m_lgposStart;
    Call( ErrLGQuit(
                this,
                &le_lgposStart,
                BoolParam( m_pinst, JET_paramAggressiveLogRollover ),
                &lgposQuit ) );

    // Keep doing synchronous flushes until all log data
    // is definitely flushed to disk. With FASTFLUSH, a single call to
    // ErrLGFlushLog() may not flush everything in the log buffers.
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

    // the last thing to do with the checkpoint is to update it to
    // the Term log record. This will determine a clean shutdown
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

    //  flush must have checkpoint log so no need to do checkpoint again
    //
    //  Call( ErrLGWriteFileHdr( m_plgfilehdr ) );


    //  check for log-sequence-end
    //  (since we logged a term/rcvquit record, we know this is a clean shutdown)

    if ( err >= JET_errSuccess )
    {
        if ( m_pLogStream->GetLogSequenceEnd() )
        {
            err = ErrERRCheck( JET_errLogSequenceEndDatabasesConsistent );
        }
        Call( err );
    }

HandleError:    // free resources

    if ( !fTermFlushLog )
    {
        err = m_pLogStream->ErrLGTermFlushLog( fFalse );
        if ( m_pinst->m_isdlTerm.FActiveSequence() )
        {
            m_pinst->m_isdlTerm.Trigger( eTermLogFsFlushed );
        }
    }


    m_pLogWriteBuffer->LGTasksTerm();
    
    //  terminate log checkpoint
    //
    LGDisableCheckpoint();
    LGICheckpointTerm();

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermLogTasksTermed );
    }

    //  close the log file
    //

    m_pLogStream->LGCloseFile();

    // Signal callback that we have logged everything that this instance can possibly 
    // log, so that any shadow log subscribers can clean up thier resources.
    m_pLogStream->ErrEmitSignalLogEnd();

    // We have logged everything we can possibly log to the log files, now
    // we can shutdown the alternate log as we couldn't possibly get any other
    // callbacks.
    //
    if ( m_pshadlog )
    {
        // now terminate the alternate log as well
        const ERR errLGShadow = ErrLGShadowLogTerm();
        // assert not thread safe, but on term, so we shouldn't have trouble here.
        Assert( NULL == m_pshadlog );
        err = ( err == JET_errSuccess ? errLGShadow : err );
    }

    //  If edbtmp.jtx/log is being written to, wait for it to complete, close the file and
    //  free related memory.
    //

    m_pLogStream->LGCreateAsynchCancel( fTrue );

    if ( m_pinst->m_isdlTerm.FActiveSequence() )
    {
        m_pinst->m_isdlTerm.Trigger( eTermLogFilesClosed );
    }

    m_pLogStream->LGTermTmpLogBuffers();

    //  clean up allocated resources
    //
    m_LogBuffer.LGTermLogBuffers();
    m_pLogStream->LGTerm();

    //  check for log-sequence-end

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


//********************* LOGGING ****************************
//**********************************************************

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

    //  try to insert the log record in the log buffer if there is room
    //
    while ( ( err = ErrLGTryLogRec( rgdata, cdata, fLGFlags, lgenBegin0, plgposLogRec ) ) == errLGNotSynchronous )
    {
        if ( NULL != pcrit )
            pcrit->Leave();

        m_pLogWriteBuffer->WriteToClearSpace();

        // Raise firewall for really long stalls
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

        //  There a list of trace in temp memory structure. Log them first.
        do {
            m_critLGTrace.Enter();
            if ( m_pttFirst )
            {
                TEMPTRACE *ptt = m_pttFirst;
                err = ErrLGITrace( ptt->ppib, ptt->szData, fTrue /* fInternal */ );
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

    //  No trace to log or trace list is taken care of, log the normal log record

    return m_pLogWriteBuffer->ErrLGLogRec( rgdata, cdata, fLGFlags, lgenBegin0, plgposLogRec );
}


ERR LOG::ErrLGITrace(
    PIB *ppib,
    // UNDONE: Better to make this PCSTR, but may be impossible b/c we pass it to Data::SetPv() :P
    _In_ PSTR sz,
    BOOL fInternal )
{
    ERR             err;
    const ULONG     cdata           = 3;
    DATA            rgdata[cdata];
    LRTRACE         lrtrace;
    DATETIME        datetime;
    const ULONG     cbDateTimeBuf   = 31;
    CHAR            szDateTimeBuf[cbDateTimeBuf+1];

    //  No trace in recovery mode

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

    //  UNDONE: a better idea would be to add a LOGTIME
    //  field to LRTRACE, but that would require a log
    //  format change, or at least a new lrtyp, so just
    //  add the date/timestamp to the trace string
    //
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
        //  To prevent recursion, do not call ErrLGLogRec which then callback
        //  ErrLGTrace

        return m_pLogWriteBuffer->ErrLGLogRec( rgdata, cdata, 0, 0, pNil );
    }

    err = ErrLGTryLogRec( rgdata, cdata, 0, 0, pNil );
    if ( err == errLGNotSynchronous )
    {
        //  Trace should not block anyone, put the record in a temp
        //  space and log it next time. It is OK to loose trace if the system
        //  crashes.

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
    // UNDONE: Better to make this PCSTR, but may be impossible b/c we pass it to Data::SetPv() :P
    // Can we fix this?  Don't understand the internl ErrLGITrace() it's complicated.
    _In_ PSTR sz )
{
    ERR err = ErrLGITrace( ppib, sz, fFalse );

    if ( ( JET_errLogWriteFail == err || JET_errOutOfMemory == err )
        && FRFSAnyFailureDetected() )
    {
        //  assume these errors were due to RFS and just throw away
        //  the trace log record
        //
        err = JET_errSuccess;
    }

    CallS( err );

    OSTrace( JET_tracetagLogging, sz );

    return err;
}


#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
//  ================================================================
bool FLGIsLVChunkSizeCompatible(
    const ULONG cbPage,
    const ULONG ulVersionMajor,
    const ULONG ulVersionMinor,
    const ULONG ulVersionUpdate )
//  ================================================================
//
//  In version 3,3704,14 of the logs the long-value chunk size
//  changed, but only for large pages (16kb and 32kb pages). The
//  change is _not_ backwards compatible so we have to check for
//  the case where we are using a log that is older than
//  the update.
//
//-
{
    // when the major or minor version changes this function won't be needed
    // any more.  Not true right now since v8 log still supports reading v7 log
    // C_ASSERT( ulLGVersionMajorMax == ulLGVersionMajorNewLVChunk );
    // C_ASSERT( ulLGVersionMinorFinalDeprecatedValue == ulLGVersionMinorNewLVChunk );
    // C_ASSERT( ulLGVersionUpdateMajorMax >= ulLGVersionUpdateNewLVChunk );

    Assert( 2*1024 == cbPage
            || 4*1024 == cbPage
            || 8*1024 == cbPage
            || 16*1024 == cbPage
            || 32*1024 == cbPage );

    // earlier checks should catch these conditions
    // Assert( ulLGVersionMajorMax == ulVersionMajor );
    // Assert( ulLGVersionMinorFinalDeprecatedValue == ulVersionMinor );
    // Assert( ulLGVersionUpdateMajorMax >= ulVersionUpdate );

    if( FIsSmallPage( cbPage ) )
    {
        // small pages are always compatible
        return true;
    }

    // this is a large page database. if it has the
    // old chunk size then it isn't compatible
    if( ulVersionMajor == ulLGVersionMajorNewLVChunk
        && ulVersionMinor == ulLGVersionMinorNewLVChunk
        && ulVersionUpdate < ulLGVersionUpdateNewLVChunk )
    {
        return false;
    }

    return true;
}
    
#endif  // ENABLE_LOG_V7_RECOVERY_COMPAT

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

    /*  read log file header.  Header is written only during
    /*  log file creation and cannot become corrupt unless system
    /*  crash in the middle of file creation.
    /**/

    TraceContextScope tcScope( iorsHeader );
    Call( pfapiLog->ErrIORead( *tcScope, 0, sizeof( LGFILEHDR ), (BYTE* const)plgfilehdr, grbitQOS ) );

    /*  check if the data is bogus.
     */
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

    /*  check for old JET version
    /**/
    if ( *(LONG *)(((char *)plgfilehdr) + 24) == 4
         && ( *(LONG *)(((char *)plgfilehdr) + 28) == 909       //  NT version
              || *(LONG *)(((char *)plgfilehdr) + 28) == 995 )  //  Exchange 4.0
         && *(LONG *)(((char *)plgfilehdr) + 32) == 0
        )
    {
        /*  version 500
        /**/
        err = ErrERRCheck( JET_errDatabase500Format );
    }
    else if ( *(LONG *)(((char *)plgfilehdr) + 20) == 443
         && *(LONG *)(((char *)plgfilehdr) + 24) == 0
         && *(LONG *)(((char *)plgfilehdr) + 28) == 0 )
    {
        /*  version 400
        /**/
        err = ErrERRCheck( JET_errDatabase400Format );
    }
    else if ( *(LONG *)(((char *)plgfilehdr) + 44) == 0
         && *(LONG *)(((char *)plgfilehdr) + 48) == 0x0ca0001 )
    {
        /*  version 200
        /**/
        err = ErrERRCheck( JET_errDatabase200Format );
    }

    //  process error from checksum and/or version validation
    //
    Call( err );

    // check filetype
    if( JET_filetypeUnknown != plgfilehdr->lgfilehdr.le_filetype // old format
        && JET_filetypeLog != plgfilehdr->lgfilehdr.le_filetype )
    {
        // not a log file
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

//TODO: find a good place for this macro.
#define COMPARE_LOG_VERSION(a, b) ((a) == (b) ? eLogVersionCompatible : ((a) < (b) ? eLogVersionTooOld : eLogVersionTooNew) )
    
ERR ErrLGICheckVersionCompatibility( const INST * const pinst, const LGFILEHDR* const plgfilehdr )
{
    ERR err = JET_errSuccess;
    //  verify current engine understands this log file format
    //  This includes the current major/minor version with updateversion <=
    //  current version + (temporarily) all v7 log formats
    //

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
        // Per SOMEONE (SOMEONE) 8.4000.* is compatible with 7.3704.*
        // Assuming current app version is 8.4000.*, then compare log ver == 7.3704.*.
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
        //After app's log version is updated to >8.4000, value of result will not be 0 so this IF block will not be entered.
        
        //Check for LV chunksize compatability
        //This function returns false only when the header version is between 7.3704.0 (included) and 7.3704.14 (not included)
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
#endif //ENABLE_LOG_V7_RECOVERY_COMPAT

    if ( eLogVersionCompatible != result )
    {
        if ( result == eLogVersionTooOld )
        {
            WCHAR wszLogGeneration[20];
            WCHAR wszLogVersion[50];
            WCHAR wszEngineVersion[50];
            const WCHAR * rgszT[3] = { wszLogGeneration, wszLogVersion, wszEngineVersion };

            //  note: here we use plgfilehdr->lgfilehdr.le_ulMinor, le_ulMinor, le_ulUpdate* explicitly because
            //  the version is older, and LgvFromLgfilehdr() insists le_ulMinor (which we've dropped) is in a 
            //  certain range.  But also probably a better idea to get the 4-piece version out since it is an 
            //  older version.
            //const LogVersion lgvLgfilehdr = LgvFromLgfilehdr( plgfilehdr );

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
                //  unknown le_ulMinor version!  Did we start re-using this?
                //  note: here we use plgfilehdr->lgfilehdr.le_ulMinor, le_ulMinor, le_ulUpdate* explicitly because
                //  the version is "newer", and LgvFromLgfilehdr() insists le_ulMinor (which we've dropped) is in a 
                //  certain range, and in fuzzing tests it may not be in that range.
                //const LogVersion lgvLgfilehdr = LgvFromLgfilehdr( plgfilehdr );
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

    //  Evaluate the allowed/desired version.

    const FormatVersions * pfmtversAllowed = NULL;
    JET_ENGINEFORMATVERSION efvUser = pinst ?
                                        (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ) :
                                        EfvMaxSupported();

    if ( efvUser == JET_efvUsePersistedFormat )
    {
        //  Whatever is persisted is fine, so we can just check against the max engine.
        efvUser = EfvMaxSupported();

        const LogVersion lgv = LgvFromLgfilehdr( plgfilehdr );
        const ERR errT = ErrLGFindHighestMatchingLogMajors( lgv, &pfmtversAllowed );
        CallS( errT );
        if ( errT < JET_errSuccess )    // don't think it will fail, but just in case ...
        {
            //  if we fail, we'll just try to lookup EfvMaxSupported(),  but with NULL for pinst to side step
            //  the staging.
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

    //  The current log file's update minor version can be higher than the desired update minor version
    //  because it doesn't signify a format breaking change.
    const INT icmpver = CmpLgVer( LgvFromLgfilehdr( plgfilehdr ), pfmtversAllowed->lgv );
    if ( !fAllowPersistedFormat && ( icmpver > 0 && !FUpdateMinorMismatch( icmpver ) ) )
    {
        WCHAR wszLogGeneration[20];
        WCHAR wszLogVersion[50];
        WCHAR wszParamVersion[50];
        WCHAR wszParamEfv[cchFormatEfvSetting];
        const WCHAR * rgszT[5] = { wszLogGeneration, wszLogVersion, wszParamVersion, wszParamEfv, L"" /* deprecated %8 */ };

        const LogVersion lgvLgfilehdr = LgvFromLgfilehdr( plgfilehdr );
        OSStrCbFormatW( wszLogGeneration, _cbrg( wszLogGeneration ), L"0x%x", (LONG)plgfilehdr->lgfilehdr.le_lGeneration );
        OSStrCbFormatW( wszLogVersion, _cbrg( wszLogVersion ), L"%d.%d.%d", lgvLgfilehdr.ulLGVersionMajor, lgvLgfilehdr.ulLGVersionUpdateMajor, lgvLgfilehdr.ulLGVersionUpdateMinor );
        OSStrCbFormatW( wszParamVersion, _cbrg( wszParamVersion ), L"%d.%d.%d", pfmtversAllowed->lgv.ulLGVersionMajor, pfmtversAllowed->lgv.ulLGVersionUpdateMajor, pfmtversAllowed->lgv.ulLGVersionUpdateMinor );
        FormatEfvSetting( (JET_ENGINEFORMATVERSION)UlParam( pinst, JET_paramEngineFormatVersion ), wszParamEfv, sizeof(wszParamEfv) );
        // deprecated: OSStrCbFormatW( wszAllowPersisted, _cbrg( wszAllowPersisted ), L"%d", fAllowPersistedFormat );
        
        //  Log is > than user requested 
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
            FireWall( "CheckingLogVerWithoutInst" ); // is this even possible
            err = JET_errSuccess;   // should already be true
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
    //  ErrLGGetDesiredLogVersion() only will return the persisted version if EFV param = JET_efvUsePersistedFormat,
    //  so this function is invalid in any other context.
    Assert( UlParam( m_pinst, JET_paramEngineFormatVersion ) == JET_efvUsePersistedFormat );
    return m_pLogStream->ErrLGGetDesiredLogVersion( efv, pplgv );
}
#endif

ERR LOG::ErrLGFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature ) const
{
    Assert( !m_pinst->FRecovering() );

    if ( FLogDisabled() )
    {
        //  We assume if there is no logging that the format feature is on.  If a feature affects
        //  the database, it should also be protected by a DB format check.
        return JET_errSuccess;
    }

    return m_pLogStream->ErrLGFormatFeatureEnabled( efvFormatFeature );
}


/*  copy tm to a smaller structure logtm to save space on disk.
 */
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
    //  This must match ErrLGRecoveryControlCallback()'s decision to allocate
    //  via ErrSetUserDbHeaderInfos().
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

ERR LOG::ErrLGIBeginUndoCallback_( _In_ const CHAR * szFile, const ULONG lLine )
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

        // validate
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
    const ULONG         eDisposition,   //  eReason | eNextAction
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

    //  initialize the callback control structure

    memset( &recctrl, 0, sizeof(JET_RECOVERYCONTROL) );
    recctrl.cbStruct = sizeof( JET_RECOVERYCONTROL );

    //  setup all the global info

    recctrl.instance = (JET_INSTANCE)pinst;
    recctrl.errDefault = errDefault;

    recctrl.sntUnion = sntRecCtrl;

    //  setup the per-control type info

    switch ( sntRecCtrl )
    {
        case JET_sntOpenLog:
            // note that at this point we have the previous log closed
            //
            // Assert( !FPfapiInited() || eDisposition == JET_OpenLogForDo );
            Assert( lgen >= 0 ); // should not be signal / lgenSignal* - client won't understand that.
            Assert( lgen > 0 || fCurrentLog );

            recctrl.OpenLog.cbStruct = sizeof( recctrl.OpenLog );

            recctrl.OpenLog.lGenNext = (ULONG)lgen;
            recctrl.OpenLog.fCurrentLog = (unsigned char)fCurrentLog;
            recctrl.OpenLog.eReason = (unsigned char)eDisposition;
            // UNDONE: would it be better to have a buffer to pass
            // rather then the LOG member?
            recctrl.OpenLog.wszLogFile = (WCHAR*)wszLogName;
            Call( ErrSetUserDbHeaderInfos( pinst, &(recctrl.OpenLog.cdbinfomisc), &(recctrl.OpenLog.rgdbinfomisc) ) );
            break;

        case JET_sntOpenCheckpoint:
            recctrl.OpenCheckpoint.cbStruct = sizeof( recctrl.OpenCheckpoint );
            recctrl.OpenCheckpoint.wszCheckpoint = NULL;
            break;

        case JET_sntMissingLog:
            // note that at this point we have the previous log closed
            //
            // Assert( !FPfapiInited() );
            Assert( lgen >= 0 ); // should not be signal / lgenSignal* - client won't understand that.

            recctrl.MissingLog.cbStruct = sizeof( recctrl.MissingLog );

            recctrl.MissingLog.lGenMissing = lgen;
            recctrl.MissingLog.fCurrentLog = (unsigned char)fCurrentLog;
            // UNDONE: would it be better to have a buffer to pass
            // rather then the LOG member?
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
            Assert( lgen >= 0 ); // lgen being overloaded ... but obviously couldn't be more than 2^31 bytes.
            recctrl.CommitCtx.cbStruct = sizeof( recctrl.CommitCtx );
            recctrl.CommitCtx.pbCommitCtx = (BYTE *)wszLogName;
            recctrl.CommitCtx.cbCommitCtx = lgen;
            recctrl.CommitCtx.fCallbackType = fCurrentLog;
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
            //  we throw the default error from the original throw location.
            ErrERRCheck_( err, szFile, lLine );
        }
        else
        {
            //  we run ErrERRCheck() to notice if the client threw the given 
            //  error we are trying to trap.
            ErrERRCheck( err );
        }
    }

    if ( sntRecCtrl == JET_sntMissingLog &&
            ( err == JET_errFileNotFound || err == JET_errMissingLogFile ) )
    {
        //  if we got a failure for a missing log, log the event now.

        pinst->m_plog->LGReportError( LOG_OPEN_FILE_ERROR_ID, JET_errFileNotFound );

        if ( recctrl.errDefault != JET_errSuccess )
        {
            //  This means it was even more serious, as we expected it to be 
            //  there for sure.
            OSUHAEmitFailureTag( pinst, HaDbFailureTagConfiguration, L"eb1fa469-3926-45a9-8286-075850b5069a" );
        }

        //  we translate all JET_errFileNotFound to JET_errMissingLogFile for log
        err = ErrERRCheck( JET_errMissingLogFile );
    }

HandleError:

    CleanupRecoveryControl( sntRecCtrl, &recctrl );

    return err;
}

ERR ErrLGDbAttachedCallback_( INST *pinst, FMP *pfmp, const CHAR *szFile, const LONG lLine )
{
    const ERR err = ErrLGRecoveryControlCallback( pinst, pfmp, NULL, JET_sntAttachedDb, JET_errSuccess, 0, fFalse, 0, szFile, lLine );

    // reset backup context's suspended flag when ErrLGDbAttachedCallback is fired to allow future backups.
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

    // suspend backup context when ErrLGDbDetachingCallback is fired, and wait for client to close it up.
    if ( BoolParam( pinst, JET_paramFlight_EnableBackupDuringRecovery ) )
    {
        pinst->m_pbackup->BKLockBackup();
        pinst->m_pbackup->BKSetIsSuspended( fTrue );

        if ( pinst->m_pbackup->FBKBackupInProgress() )
        {
            const TICK  tickStart = TickOSTimeCurrent();

            // There shouldn't be any non-internal backups during recovery-redo (i.e. a "passive").
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

            // after client hits failure during reading file, it should close the backup instance.
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

// A page patch request log record has been encountered. Call the recovery callback with the
// current page data.
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
            ErrERRCheck( err ); // fire the error trap on the callback
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
    // the first two phases of recoveyr pre-init, and pre-recovery is where we should be discovering
    // and setting the actual log ext ...
    Assert( !m_fLogInitialized ||
            (m_fRecoveringMode == fRecoveringNone) ||
            m_wszChkExt != NULL );
    Assert( fReset || // if not reset, we must be setting these for the first time.
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

VOID LOG::LGElasticWaypointLatency( LONG *plWaypointLatency, LONG *plElasticWaypointLatency ) const
{
    if ( FWaypointLatencyEnabled() )
    {
        *plWaypointLatency = (LONG)UlParam( m_pinst, JET_paramWaypointLatency );
        *plElasticWaypointLatency = (LONG)UlParam( m_pinst, JET_paramFlight_ElasticWaypointLatency );
    }
    else
    {
        *plWaypointLatency = 0;
        // No elastic LLR without LLR
        *plElasticWaypointLatency = 0;
    }
}

VOID LOG::LGIGetEffectiveWaypoint(
    _In_    IFMP    ifmp,
    _In_    LONG    lGenCommitted,
    _Out_   LGPOS * plgposEffWaypoint )
{
    LGPOS lgposBestWaypoint;

    Assert( plgposEffWaypoint );

#ifdef DEBUG
    // for debugging
    LGPOS lgposCurrentWaypoint;
    lgposCurrentWaypoint = g_rgfmp[ ifmp ].LgposWaypoint();
#endif

    //
    //  we start with what BF thinks is the best waypoint ...
    //

    BFGetBestPossibleWaypoint( ifmp, lGenCommitted, &lgposBestWaypoint );
    Assert( lgposBestWaypoint.ib == lgposMax.ib );
    Assert( lgposBestWaypoint.isec == lgposMax.isec );
    // Should never go backwards from the current waypoint.
    Assert( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint ) >= 0 );

    *plgposEffWaypoint = lgposBestWaypoint;
}

/*  update the appropriate headers for the appropriate[1] DBs ...
 *
 *  [1] appropriate seems to means DBs attached w/ logging,
 *  non-read only, without "skipping attach", non-deferred
 *  attached, and with a Pdbfilehdr().
 *
 *  Parameters:
 *      pfsapi -
 *      lGenMinRequired - if 0, this is not updated.
 *      lGenMinConsistent - if 0, this is not updated.
 *      lGenCommitted - Generally the value of a new log.  If this is
 *              0, we will not update the max gen committed or required.
 *      logtimeGenMaxCreate -
 *      pfSkippedAttachDetach - This may get set if ANY of the DB headers
 *              we skipped updating, because they're in the middle of
 *              attached or detach.
 *      ifmpTarget - The specific database to check for.  ifmpNil is
 *              the default, which means do all databases that
 *              are attached and logged.
/**/
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
    // If we are not looking at all the IFMPs, do not trim logtimeMaxRequired mapping table
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
            continue;   // this is not the IFMP you are looking for.

        FMP         *pfmpT          = &g_rgfmp[ ifmp ];

        if ( pfmpT->FReadOnlyAttach() )
            continue;

        pfmpT->RwlDetaching().EnterAsReader();

        //  need to check ifmp again. Because the db may be cleaned up
        //  and FDetachingDB is reset, and so is its ifmp in m_mpdbidifmp.
        //
        
        //  If FSkippedAttach or FDeferredAttach return true, FAttached must be false
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
        // Should never go backwards from the current waypoint / max log gen required.
        Assert( CmpLgpos( lgposBestWaypoint, pfmpT->LgposWaypoint() ) >= 0 );

        //
        //  Compute and Flush if necessary the deferred DB updates that prevent the 
        //  checkpoint from moving forward.
        //

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
            } // .dtor releases read header lock.

            if ( err >= JET_errSuccess && fPreHeaderUpdateMinRequired )
            {
                Assert( pfmpT == &g_rgfmp[ifmp] );
                // Goes off in read from passive [basic] test
                //Expected( fPreHeaderUpdateMinRequired && fPreHeaderUpdateMinConsistent );
                err = ErrIOFlushDatabaseFileBuffers( ifmp, iofrDbUpdMinRequired );
                fDidPreDbFlush = ( err >= JET_errSuccess );
                if ( err < JET_errSuccess )
                {
                    //  failed to force deferred DB writes to disk, can't move the checkpoints or min-required up...
                    goto FailedSkipDatabase;
                }
            }
        }

        BOOL fHeaderUpdateMaxRequired, fHeaderUpdateMaxCommitted, fHeaderUpdateMinRequired, fHeaderUpdateMinConsistent;
        fHeaderUpdateMaxRequired = fHeaderUpdateMaxCommitted = fHeaderUpdateMinRequired = fHeaderUpdateMinConsistent = fFalse;
        BOOL fHeaderUpdateGenMaxCreateTime = fFalse;
        BOOL fLogtimeGenMaxRequiredEfvEnabled = ( pfmpT->ErrDBFormatFeatureEnabled( JET_efvLogtimeGenMaxRequired ) == JET_errSuccess );

        { // .ctor acquires PdbfilehdrReadWrite
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
            //Assert( max( lgposBestWaypoint.lGeneration, lGenMaxRequiredOld ) == lgposBestWaypoint.lGeneration );

            /*
            We have the following scenario:
            - start new log -> update m_plgfilehdrT -> call this function with genMax = 5 (from m_plgfilehdrT)
            - other thread is updating the checkpoint file setting genMin but is passing genMax from m_plgfilehdr
            which is still 4 because the log thread is moving m_plgfilehdrT into m_plgfilehdr later on

            The header ends up with genMax 4 and data is logged in 5 -> Wrong !!! We need max (old, new)
            SOMEONE: I wonder why have checkpoint pass genMax at all?  Why not pull current value from in here?
            After all the lg buff crit rank sect is < than checkpoint lock.
            */
            pdbfilehdr->le_lGenMaxRequired = max( lgposBestWaypoint.lGeneration, lGenMaxRequiredOld );
            Assert ( lGenMaxRequiredOld <= pdbfilehdr->le_lGenMaxRequired );

            const LONG  lGenCommittedOld    = pdbfilehdr->le_lGenMaxCommitted;

#ifdef DEBUG
            // we should have the same time for the same generation
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

            // if we updated the genCommitted with the value passed in, update the time with the new value as well
            Assert( !fHeaderUpdateMaxCommitted || pdbfilehdr->le_lGenMaxCommitted == lGenCommitted );
            if ( pdbfilehdr->le_lGenMaxCommitted == lGenCommitted )
            {
                LOGTIME logtimeGenMaxCreateOld;
                memcpy( &logtimeGenMaxCreateOld, &pdbfilehdr->logtimeGenMaxCreate, sizeof( LOGTIME ) );

                memcpy( &pdbfilehdr->logtimeGenMaxCreate, &logtimeGenMaxCreate, sizeof( LOGTIME ) );

                fHeaderUpdateGenMaxCreateTime = ( 0 != memcmp( &logtimeGenMaxCreateOld, &pdbfilehdr->logtimeGenMaxCreate, sizeof( LOGTIME ) ) );

                //  This needs some explanation ... so let's switch tracks, in recovery the logtimeGenMaxCreate must 
                //  be equivalent and match that logtime of the le_lGenMaxCommitted log OR tmEmpty is also OK.  So
                //  that means for this code if fHeaderUpdateGenMaxCreateTime is true, it means the logtimeGenMaxCreate 
                //  changed, and then we must have a tmEmpty in the old header (to avoid a crash recovery problem) or
                //  be doing an update b/c lgen for le_lGenMaxCommitted / fHeaderUpdateMaxCommitted actually changed.
                //  I think this is held.
                //  We fix-up le_lGenMaxCommitted/logtimeGenMaxCreate when we hit JET_wrnCommittedLogFilesLost in
                //  ErrLGCheckGenMaxRequired. However, with log replication, one set of committed logs can disappear
                //  underneath us and another set reappear while we are in redo mode, hence the fRecoveringRedo
                //  clause below.
                Assert( fHeaderUpdateGenMaxCreateTime == fFalse ||
                        fHeaderUpdateMaxCommitted ||
                        0 == memcmp( &logtimeGenMaxCreateOld, &tmEmpty, sizeof( LOGTIME ) ) ||
                        m_fRecoveringMode == fRecoveringRedo ||
                        ( m_fRecoveringMode == fRecoveringUndo && !m_fRecoveryUndoLogged ) /* we switch to undo state too early, if undo hasn't been logged, we aren't really in undo */ );
            }
        }

        if ( lGenMinRequired )
        {
            const LONG  lGenMinRequiredOld          = pdbfilehdr->le_lGenMinRequired;
            const LONG  lGenMinRequiredCandidate    = max( lGenMinRequired, pdbfilehdr->le_lgposAttach.le_lGeneration );

            //  UNDONE: hate to lose this valuable assert, but in
            //  LOG::ErrLGUpdateCheckpointFile(), genMin may get
            //  ahead of the checkpoint if we updated some
            //  db headers while there was attach/detach activity
            //  or if the write to the checkpoint failed, and
            //  then we later use the stale checkpoint to try
            //  to update genMin (LOG::ErrLGUpdateGenRequired()
            //  is one culprit, and there may be others)
            //
            //  Assert( lGenMinRequiredOld <= lGenMinRequiredCandidate );

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

        } // .dtor releases PdbfilehdrReadWrite

        //  Just double checking above pre-flush 
        if ( err >= JET_errSuccess && fHeaderUpdateMinRequired )
        {
            //  If this goes off it is concerning, but there are two conceivable possibilities:
            //   1. that the calculations for lgenMinRequiredFuture, fPreHeaderUpdateMinRequired,
            //      lgenMinConsistentFuture, fPreHeaderUpdateMinConsistent above no longer match
            //      the slightly lower recalculations of lGenMinRequiredOld, fHeaderUpdateMinRequired,
            //      lGenMinConsistentOld, lGenMinConsistentCandidate... this is the bad case, make
            //      the two copies of code match. 
            //   2. just the checkpoint lock and RwlDetaching is not enough to protect the state
            //      of pdbfilehdr->le_lGenMinRequired and pdbfilehdr->le_lGenMinConsistent or the
            //      pdbfilehdr->le_lgposAttach from changing state between when we release the hdr
            //      RO lock and grab the RW lock over the FFB call above.  If so the assert is un-
            //      maintainable I think.  Or we'll have to do a FFB in the header lock!  Yuck.
            Assert( fDidPreDbFlush );
        }
        else
        {
            Expected( !fDidPreDbFlush ); // much less important, but above comment applies.
        }

        //  Note: If we're updating fHeaderUpdateMaxRequired, we should have already flushed the respective log

        if ( err >= JET_errSuccess &&
             // Either we are not skipping lgenCommitted only updates, or min/max Required got updated, or this call did not even pass in a lgenCommitted (special call from backup)
             ( !BoolParam( m_pinst, JET_paramFlight_SkipDbHeaderWriteForLgenCommittedUpdate ) || fHeaderUpdateMinRequired || fHeaderUpdateMaxRequired || lGenCommitted == 0 ) )
        {
            err = ErrUtilWriteAttachedDatabaseHeaders( m_pinst, pfsapi, pfmpT->WszDatabaseName(), pfmpT, pfmpT->Pfapi() );

            if ( err >= JET_errSuccess && ( fHeaderUpdateMinRequired || fHeaderUpdateMaxRequired || lGenCommitted == 0 ) )
            {
                const IOFLUSHREASON iofr =
                    IOFLUSHREASON(
                        ( fHeaderUpdateMinRequired ? iofrDbHdrUpdMinRequired : 0 ) |
                        ( ( fHeaderUpdateMaxRequired || lGenCommitted == 0 ) ? iofrDbHdrUpdMaxRequired : 0 ) );
                err = ErrIOFlushDatabaseFileBuffers( ifmp, iofr );
            }
            
            PdbfilehdrReadOnly pdbfilehdr = pfmpT->Pdbfilehdr();
            const LE_LGPOS * ple_lgposGreater = ( CmpLgpos( pdbfilehdr->le_lgposAttach, pdbfilehdr->le_lgposLastReAttach ) > 0 ) ?
                                                    &pdbfilehdr->le_lgposAttach : &pdbfilehdr->le_lgposLastReAttach;

            OSTrace( JET_tracetagCheckpointUpdate, OSFormat( "Writing DB header { %08x.%04x.%04x - %d / %d - %d / %d }",
                (LONG)ple_lgposGreater->le_lGeneration, (USHORT)ple_lgposGreater->le_isec, (USHORT)ple_lgposGreater->le_ib,
                (LONG)pdbfilehdr->le_lGenMinConsistent, (LONG)pdbfilehdr->le_lGenMinRequired, (LONG)pdbfilehdr->le_lGenMaxRequired, (LONG)pdbfilehdr->le_lGenMaxCommitted ) );
        }

        //  if we've committed the new waypoint, update the FMP.

        if ( JET_errSuccess <= err && fUpdateGenMaxRequired )
        {
            // If the current waypoint is behind, improve it.
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




//  ================================================================
ERR LOG::ErrLGLockCheckpointAndUpdateGenRequired( const LONG lGenCommitted, const LOGTIME logtimeGenMaxCreate )
//  ================================================================
//
//  Update all the database headers with proper lGenMaxRequired and lGenMaxCommitted.
//
//-
{
    m_critCheckpoint.Enter();
    Assert( lGenCommitted != 0 );

    const ERR err = ErrLGUpdateGenRequired(
                        m_pinst->m_pfsapi,
                        0,  //  do not update gen min
                        0,  //  do not update gen min consistent
                        lGenCommitted, // logs committed
                        logtimeGenMaxCreate,
                        NULL );
    m_critCheckpoint.Leave();
    //}

    return err;
}



/********************* CHECKPOINT **************************
/***********************************************************
/**/


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
        // following is functionally equivalent to whether we are in redo mode
        const BOOL  fUsePatchchk    = ( pfmpT->Patchchk()
                                        && m_fRecovering
                                        && m_fRecoveringMode == fRecoveringRedo );

        pfmpT->RwlDetaching().EnterAsReader();
        // Assert( lgenNewFile >= pfmpT->LgposDetach().lGeneration );
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
            //  verify we don't overrun buffer (ensure we have enough room for sentinel)
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
                //   relays DBISetHeaderAfterAttach behavior for resetting lgposConsistent
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
                    //  HACK: original database name hangs off the end
                    //  of the relocated database name
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

            // we can't use OSStrCbCopyW because the destination is unaligned
            cbNames = (ULONG)( sizeof(WCHAR) * ( LOSStrLengthW( wszDatabaseName ) + 1 ) );

            //  verify we don't overrun buffer (ensure we have enough room for sentinel)
            EnforceSz( pbBuf + cbAttach >= (BYTE *)pattachinfo + sizeof(ATTACHINFO) + cbNames, "TooManyDbsForAttachInfoAfter" );

            memcpy( pattachinfo->szNames, wszDatabaseName, cbNames );
            pattachinfo->SetCbNames( (USHORT)cbNames );

            //  advance to next attachinfo
            pattachinfo = (ATTACHINFO*)( (BYTE *)pattachinfo + sizeof(ATTACHINFO) + cbNames );
        }
        pfmpT->RwlDetaching().LeaveAsReader();
    }

    FMP::LeaveFMPPoolAsReader();

    /*  put a sentinal
    /**/
    *(BYTE *)pattachinfo = 0;

    //  UNDONE: next version we will allow it go beyond 4kByte limit
    Assert( pbBuf + cbAttach > (BYTE *)pattachinfo );
}

//  Load attachment information - how and what the db is attached.

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

void LOG::LGISetInitialGen( _In_ const LONG lgenStart )
{
    Assert( m_pcheckpoint );
    Assert( lgenStart );

    m_lgenInitial = lgenStart;

    //  Ensure that the checkpoint never reverts.
    Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration <= m_lgenInitial );

    m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = m_lgenInitial; /* "first" generation */
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

    //  Initialize. Used by perfmon counters
    m_pcheckpoint->checkpoint.le_lgposCheckpoint = lgposMin;
    m_pcheckpoint->checkpoint.le_lgposDbConsistency = lgposMin;

TryAnotherExtension:
    //  Intentionally not calling LGFullNameCheckpoint() here, as that doesn't allow a pickable checkpoint
    //  extension, and this is first contact with the file system for our extension, so we need to try both
    //  extensions ... and pick that ext, if we find a checkpoint file ...
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

        // Success, if we haven't settled on a log ext yet, settle it now.
        if ( m_pLogStream->LogExt() == NULL )
        {
            // SOFT_HARD: leave, not important
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

    // technically shouldn't need lock anymore, should be single threaded at this point ...
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


/*  read checkpoint from file.
/**/
ERR LOG::ErrLGReadCheckpoint( _In_ PCWSTR wszCheckpointFile, CHECKPOINT *pcheckpoint, const BOOL fReadOnly )
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
        /*  it should never happen that both checkpoints in the checkpoint
        /*  file are corrupt.  The only time this can happen is with a
        /*  hardware error.
        /**/
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

    // Check if the file indeed is checkpoint file
    if( JET_filetypeUnknown != pcheckpoint->checkpoint.le_filetype // old format
        && JET_filetypeCheckpoint != pcheckpoint->checkpoint.le_filetype )
    {
        // not a checkpoint file
        OSUHAEmitFailureTag( m_pinst, HaDbFailureTagCorruption, L"c0c9b597-c2d5-4dd4-a5e8-9904c00507df" );
        Call( ErrERRCheck( JET_errFileInvalidType ) );
    }

    // Initialize lgposDbConsistency in case this checkpoint file does not have it set yet.
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


/*  write checkpoint to file.
/**/
ERR LOG::ErrLGIWriteCheckpoint( _In_ PCWSTR wszCheckpointFile, const IOFLUSHREASON iofr, CHECKPOINT *pcheckpoint )
{
    ERR     err;

    Assert( m_critCheckpoint.FOwner() );
    Assert( !m_fDisableCheckpoint );
    Assert( CmpLgpos( pcheckpoint->checkpoint.le_lgposCheckpoint, pcheckpoint->checkpoint.le_lgposDbConsistency ) <= 0 );
    Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= 1 );
    Assert( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration >= m_lgenInitial );

    Assert( m_fSignLogSet );
    pcheckpoint->checkpoint.signLog = m_signLog;

    //  write JET_filetypeCheckpoint to header
    //
    pcheckpoint->checkpoint.le_filetype = JET_filetypeCheckpoint;


    err = ErrUtilWriteCheckpointHeaders( m_pinst,
                                         m_pinst->m_pfsapi,
                                         wszCheckpointFile,
                                         iofr,
                                         pcheckpoint );


    //  Ignore errors. Bet on that it may be failed temporily (e.g. being locked)
//  if ( err < 0 )
//      m_fDisableCheckpoint = fTrue;

    return err;
}

ERR LOG::ErrLGIUpdateCheckpointLgposForTerm( const LGPOS& lgposCheckpoint )
{
    ERR     err;
    WCHAR   wszPathJetChkLog[IFileSystemAPI::cchPathMax];


    m_critCheckpoint.Enter();

    Assert( !m_fDisableCheckpoint );

    LGFullNameCheckpoint( wszPathJetChkLog );

    // if we had a clean termination (m_fAfterEndAllSessions is TRUE)
    // then we need to regenerate the checkpoint with the last
    // termination position which is the last thing it got redone
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

/*  Update in memory checkpoint.
/*
/*  Computes log checkpoint, which is the lGeneration, isec and ib
/*  of the oldest transaction which either modified a currently-dirty buffer
/*  an uncommitted version (RCE). Flush map up-to-dateness is also factored
/*  in.  Recovery begins redoing from the checkpoint.
/*
/*  The checkpoint is stored in the checkpoint file, which is rewritten
/*  whenever a isecChekpointPeriod disk sectors are written.
/**/
VOID LOG::LGIUpdateCheckpoint( CHECKPOINT *pcheckpoint )
{
    PIB     *ppibT;
    LGPOS   lgposCheckpoint = lgposMax;
    LGPOS   lgposDbConsistency = lgposMax;

    //  tracing support
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

    //  we start with the most recent log record on disk.
    //  we are starting with m_lgposToFlush because we know that any
    //  new transactions that are started while we are scanning the
    //  session list must get an lgposStart larger than the current
    //  value of m_lgposToFlush (note that we must read this value
    //  inside LockBuffer and it must be a global lock so
    //  that we are guaranteed to get a value that is <= any
    //  lgposStart to be given out in the future)
    //
    //  note that with FASTFLUSH, m_lgposToFlush points to the start
    //  of a log record that didn't make it to disk (or to data that
    //  hasn't yet been added to the log buffer)
    //
    m_pLogWriteBuffer->LockBuffer();

    const LGPOS lgposWrittenTip = ( m_fRecoveringMode == fRecoveringRedo ) ? m_lgposRedo : m_pLogWriteBuffer->LgposWriteTip();

    lgposDbConsistency = lgposWrittenTip;
    lgposToFlushT = lgposWrittenTip;
    lgposLogRecT = m_pLogWriteBuffer->LgposLogTip();

    // Stopped trying to fight this very basic assert ... think this is an
    // terrible feature choice that lgposToFlush is measured as end of last
    // LR, and lgposLogRec is beginning of last log record?  Be better if
    // the lgposLogRec was after last LR.  Or if it is, then I can't explain
    // the last hit I had, and someone who owns log should take this assert.
    /*
    Assert( m_fRecoveringMode == fRecoveringRedo || // lgposLogRecT/Tip only relevant in do-time ...
                CmpLgpos( lgposToFlushT, lgposLogRecT ) <= 0 ||
                //  I just reversed engineered this case, but I find it quite vugly ... we
                //  should just consider moving the LgposLogTip() UP whenever we flush a
                //  buffer such that we move the _next_ LR forward. It's almost as if the
                //  NOP/NOP2s are not counting.
                ( lgposToFlushT.lGeneration == lgposLogRecT.lGeneration &&
                    lgposToFlushT.isec - 1 == lgposLogRecT.isec ) ||
                ( lgposToFlushT.lGeneration - 1 == lgposLogRecT.lGeneration &&
                    lgposToFlushT.isec == 1 &&
                    lgposToFlushT.ib == 0 ) ||
                // during ErrIOTerm(): 19:0039:02a8 < 19:0039:029a (lgposToFlush < lgposLogRecT respectively)
                m_pinst->m_fTermInProgress
                ); // that would be confusing
    */

    m_pLogWriteBuffer->UnlockBuffer();

    //  find the oldest transaction with an uncommitted update
    //
    m_pinst->m_critPIB.Enter();
    for ( ppibT = m_pinst->m_ppibGlobal; ppibT != NULL; ppibT = ppibT->ppibNext )
    {
        //  verify that the PIB is active, but in truth, it should
        //  be impossible to be inactive (because it would have
        //  gotten removed from the global list)
        //
        Assert( levelNil != ppibT->Level() );
        if ( levelNil != ppibT->Level() )
        {
            //  must enter critLogBeginTrx to ensure a new BeginTrx
            //  doesn't get logged while we're performing this check
            //
            ENTERCRITICALSECTION    enterCritLogBeginTrx( &ppibT->critLogBeginTrx );

            //  if this session has a transaction with an
            //  uncommitted update, see if it's the oldest one we've
            //  we've encountered so far (note that we can read the
            //  FBegin0Logged() flag unserialised because even if
            //  the session is concurrently committing or rolling
            //  back, its lgposStart is still valid (it won't be
            //  reset again until it begins a new transaction, which
            //  is blocked because we are holding critLogBeginTrx)
            //
            if ( ppibT->FBegin0Logged()
                && CmpLgpos( &ppibT->lgposStart, &lgposDbConsistency ) < 0 )
            {
                lgposDbConsistency = ppibT->lgposStart;
            }

            //  check for transaction that has been running for too long 
            //  and warn the user / app about the issue. 
            const LONG dlgenTattleThreshold = (LONG)UlParam( m_pinst, JET_paramCheckpointTooDeep ) / 4;
            //  while the above sort of claims we can look at lgposStart, I
            //  am skeptical because PIBResetTrxBegin0() is set to lgposMax
            //  in DIRCommit/Rollback without any protection - I deal with
            //  this by a little volatile and defense in depth to check the
            //  value I read.  This is only for optics so it's fine.
            //  Note: Didn't want to enter ppib->critLogBeginTrx where we 
            //  reset lgposStart due to perf concerns as Begin Trx is a 
            //  moderately hot path.
            const LONG lgenStart = *(volatile LONG *)( &ppibT->lgposStart.lGeneration );
            const LONG dlgenTransactionLength = lgposWrittenTip.lGeneration - lgenStart;
            if ( !m_pinst->m_plog->FRecovering() && 
                   ppibT->FBegin0Logged() && 
                   lgenStart != lGenerationInvalid && // defense in depth
                   dlgenTransactionLength > dlgenTattleThreshold &&
                   //  must be last, sets whether we evented or not
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
                    //  Everything stayed same, we won right to log this event.
                    WCHAR wszTrxLgens[30];
                    WCHAR wszTrxCtxInfo[50];
                    WCHAR wszTrxIdTimeStack[400];
                    const WCHAR * rgpwsz[] = { wszTrxLgens, wszTrxCtxInfo, wszTrxIdTimeStack };

                    //  we only read variables we can read without significant locking (i.e. 
                    //  no pointers) ... remember pib can't go away because m_pinst->m_critPIB.
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

    //  for the most part 0 == CmpLgpos( lgposDbConsistency, lgposToFlushT ) will mean
    //  there is no trx started.  Unless lgposToFlush is actually sitting waiting 
    //  to flush a deferred begin0 LR / lgposStart.
    lgposLowestStartT = ( 0 == CmpLgpos( lgposDbConsistency, lgposToFlushT ) ) ? lgposMax : lgposDbConsistency;

    /*  find the oldest transaction which dirtied a current buffer
    /*  NOTE:  no concurrency is required as all transactions that could
    /*  NOTE:  dirty any BF are already accounted for by the above code
    /**/

    fAdvanceCheckpoint = !m_fPendingRedoMapEntries;

    if ( !fAdvanceCheckpoint )
    {
        return;
    }

    FMP::EnterFMPPoolAsReader();

    // First, compute the DB consistency LGPOS.
    for ( DBID dbid = dbidMin; dbid < dbidMax; dbid++ )
    {
        const IFMP ifmp = m_pinst->m_mpdbidifmp[ dbid ];

        if ( ifmp < g_ifmpMax )
        {
            FMP* const pfmp = &g_rgfmp[ifmp];

            // do not advance the checkpoint if there is a skipped attachment that we are not ignoring
            if ( pfmp->FSkippedAttach() && !m_fReplayingIgnoreMissingDB )
            {
                fAdvanceCheckpoint = fFalse;
                break;
            }

            // do not advance the checkpoint if there is a deferred attachment that we aren't ignoring
            if ( pfmp->FDeferredAttach() && !pfmp->FIgnoreDeferredAttach() && !m_fReplayingIgnoreMissingDB )
            {
                fAdvanceCheckpoint = fFalse;
                break;
            }

            // If any (not deferred or skipped) dbs are still attaching/creating, do not allow checkpoint
            // to move past their minRequired generation
            if ( !pfmp->FAllowHeaderUpdate() && !pfmp->FSkippedAttach() && !pfmp->FDeferredAttach() )
            {
                fAdvanceCheckpoint = fFalse;
                break;
            }

            // Should not advance checkpoint if we are dealing with log redo maps.
            // There is a window where we could:
            // -Replay a log record.
            // -Add an entry to a log redo map.
            // -Advance the checkpoint beyond that point.
            // -Crash!
            // -Subsequent log replay would skip that log record!
            if ( FRecovering() && pfmp->FContainsDataFromFutureLogs() )
            {
                // This would be a problem for backup-restored (i.e., fully reseeded) databases because we would
                // have to hold the checkpoint for potentially thousands of logs. The good news is that we currently
                // only shrink during attach and the database is not reported as attached (for purposes of backing it up)
                // until it has finished shrinking. That means we won't have cases where the maps are non-empty.

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


                //  if we close the hole in the scenario described
                //  in the comment above, then it should not be
                //  possible to have a 0 waypoint by the time we
                //  get here, but this has caused us enough grief
                //  due to other bugs that we should just be safe
                //  about it
                //
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

            //  trace relevant pieces ...
            //
            //  A fairly expected order would be this, so we'll trace this order...
            //      |            |               |             |          |                 |              |     |
            //    existing   flush map         existing       OB0    lgposStart       lgposWaypoint    ToFlush LogRec
            //   checkpoint  min required   DB consistency         (min of all PIBs) 
            //
    
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

            //  update dbtime/trxOldest
            if ( ppibNil != m_pinst->m_ppibGlobal )
            {
                pfmp->UpdateDbtimeOldest();
            }
        } // if ifmp < g_ifmpMax
    } // for each dbid

    // If we should not advance the checkpoint for some reason then we're done.
    if ( !fAdvanceCheckpoint )
    {
        FMP::LeaveFMPPoolAsReader();
        return;
    }

    lgposCheckpoint = lgposDbConsistency;

    // Now, factor in the flush map restriction.
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

    /*  set the new checkpoint if it is advancing
     */
    if ( CmpLgpos( &lgposCheckpoint, &pcheckpoint->checkpoint.le_lgposCheckpoint ) > 0 )
    {
        Assert( lgposCheckpoint.lGeneration != 0 );
        Assert( lgposCheckpoint.lGeneration != ( m_lgenInitial - 1 ) );
        if ( lgposCheckpoint.isec == 0 )
        {
            lgposCheckpoint.isec = (USHORT)m_pLogStream->CSecHeader();
        }
        pcheckpoint->checkpoint.le_lgposCheckpoint = lgposCheckpoint;

        OSTrace( JET_tracetagBufferManagerMaintTasks, // updated, but not written yet.
                OSFormat(   "CPUPD: updating checkpoint to %s",
                            OSFormatLgpos( lgposCheckpoint ) ) );
    }
    else
    {
        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: leaving checkpoint at %s",
                            OSFormatLgpos( LGPOS( pcheckpoint->checkpoint.le_lgposCheckpoint ) ) ) );
    }

    /*  set the new DB consistency LGPOS if it is advancing
     */
    if ( CmpLgpos( &lgposDbConsistency, &pcheckpoint->checkpoint.le_lgposDbConsistency ) > 0 )
    {
        Assert( lgposDbConsistency.lGeneration != 0 );
        Assert( lgposDbConsistency.lGeneration != ( m_lgenInitial - 1 ) );
        if ( lgposDbConsistency.isec == 0 )
        {
            lgposDbConsistency.isec = (USHORT)m_pLogStream->CSecHeader();
        }
        pcheckpoint->checkpoint.le_lgposDbConsistency = lgposDbConsistency;

        OSTrace( JET_tracetagBufferManagerMaintTasks, // updated, but not written yet.
                OSFormat(   "CPUPD: updating DB consistency LGPOS to %s",
                            OSFormatLgpos( lgposDbConsistency ) ) );
    }
    else
    {
        OSTrace( JET_tracetagBufferManagerMaintTasks,
                OSFormat(   "CPUPD: leaving DB consistency LGPOS at %s",
                            OSFormatLgpos( LGPOS( pcheckpoint->checkpoint.le_lgposDbConsistency ) ) ) );
    }

    /*  set DBMS parameters
    /**/
    m_pinst->SaveDBMSParams( &pcheckpoint->checkpoint.dbms_param );

    // if the checkpoint is on a log file we haven't generated (disk full, etc.)
    // move the checkpoint back ONE LOG file
    // anyway the current checkpoint should not be more that one generation ahead of the current log generation
{
    LONG lCurrentLogGeneration;

    m_pLogWriteBuffer->LockBuffer();
    lCurrentLogGeneration = m_pLogStream->GetCurrentFileGen();

    Assert ( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration <= lCurrentLogGeneration + 1 );
    pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration = min ( pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration, lCurrentLogGeneration );
    pcheckpoint->checkpoint.le_lgposDbConsistency.le_lGeneration = min ( pcheckpoint->checkpoint.le_lgposDbConsistency.le_lGeneration, lCurrentLogGeneration );

    m_pLogWriteBuffer->UnlockBuffer();
}

    // just in case one of the adjustments above generated an inconsistency.
    if ( CmpLgpos( pcheckpoint->checkpoint.le_lgposCheckpoint, pcheckpoint->checkpoint.le_lgposDbConsistency ) > 0 )
    {
        pcheckpoint->checkpoint.le_lgposCheckpoint = pcheckpoint->checkpoint.le_lgposDbConsistency;
    }

    /*  set database attachments
    /**/
    LGLoadAttachmentsFromFMP( pcheckpoint->checkpoint.le_lgposCheckpoint, pcheckpoint->rgbAttach );
    pcheckpoint->checkpoint.fVersion |= fCheckpointAttachInfoPresent;

    m_pinst->m_pbackup->BKCopyLastBackupStateToCheckpoint( &pcheckpoint->checkpoint );

    return;
}

//  Parameters:
//      lGenCommitted - if this is zero instead of the current
//              log, then the value returned from this function
//              could be false when the waypoint is updateable.
//      ifmp - The specific database to check for.  ifmpNil is
//              the default, which means do all databases that
//              are attached and logged.
//
BOOL LOG::FLGIUpdatableWaypoint( _In_ const LONG lGenCommitted, _In_ const IFMP ifmpTarget )
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
            continue;   // this is not the IFMP you are looking for.

        if ( g_rgfmp[ifmp].FLogOn() && g_rgfmp[ifmp].FAttached() )
        {
            const LGPOS lgposCurrentWaypoint    = g_rgfmp[ifmp].LgposWaypoint();


            //  if we close the hole in the scenario described
            //  in the comment above, then it should not be
            //  possible to have a 0 waypoint by the time we
            //  get here, but this has caused us enough grief
            //  due to other bugs that we should just be safe
            //  about it
            //
            if ( 0 != CmpLgpos( lgposCurrentWaypoint, lgposMin ) )
            {
                LGPOS   lgposBestWaypoint;

                BFGetBestPossibleWaypoint( ifmp, lGenCommitted, &lgposBestWaypoint );

                // Should never go backwards from the current waypoint.
                Assert( CmpLgpos( lgposBestWaypoint, lgposCurrentWaypoint ) >= 0 );

                //
                //  Check if the waypoint was improved...
                //

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

/*  update checkpoint file.
/**/
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

    /*  save checkpoint
    /**/
    lgposCheckpointT = m_pcheckpoint->checkpoint.le_lgposCheckpoint;
    lgposDbConsistencyT = m_pcheckpoint->checkpoint.le_lgposDbConsistency;
    *pcheckpointT = *m_pcheckpoint;

    /*  update checkpoint
    /**/
    LGIUpdateCheckpoint( pcheckpointT );
    const BOOL fCheckpointUpdated =
        ( ( lgposCheckpointT.lGeneration < pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration ) ||
          ( lgposCheckpointT.isec < pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec ) ||
          ( lgposDbConsistencyT.lGeneration < pcheckpointT->checkpoint.le_lgposDbConsistency.le_lGeneration ) ||
          ( lgposDbConsistencyT.isec < pcheckpointT->checkpoint.le_lgposDbConsistency.le_isec ) );


    const BOOL fUpdatedCheckpointMinRequired =
        ( ( lgposCheckpointT.lGeneration < pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration ) ||
          ( lgposCheckpointT.isec < pcheckpointT->checkpoint.le_lgposCheckpoint.le_isec ) );

    // Some snapshot preimages (like those collected for DeleteTable) are not tied to dirty buffers,
    // so we need to make sure that they are flushed before the checkpoint moves and the lrtypExtentFreed
    // LR moves out of the required range
    if ( fUpdatedCheckpointMinRequired && m_pinst->m_prbs && m_pinst->m_prbs->FInitialized() && !m_pinst->m_prbs->FInvalid() )
    {
        Call( m_pinst->m_prbs->ErrFlushAll() );
    }

    //  if this assert goes off, it means the checkpoint
    //  is moving backward, which is bad, especially if
    //  we're forced to update the checkpoint file
    //
    Assert( lgposCheckpointT.lGeneration <= pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration );

    /*  updatable waypoint
    **/
    const BOOL fUpdateWaypoint = FLGIUpdatableWaypoint( 0 );

    /*  if checkpoint or waypoint unchanged then return JET_errSuccess
    /**/
    if ( fForceUpdate || fCheckpointUpdated || fUpdateWaypoint )
    {
        //  no in-memory checkpoint change if failed to write out to any of
        //  the database headers.

        BOOL fSkippedAttachDetach;

        // Now disallow header update by other threads (log writer or checkpoint advancement)
        // 1. For the log writer it is OK to generate a new log w/o updating the header as no log operations
        // for this db will be logged in new logs
        // 2. For the checkpoint: don't advance the checkpoint if db's header weren't update  <- THIS CASE

        // During Redo, the genMax is updated only when we switch to a new generation,
        // and there is no need to update it here (because we might have not replayed anything from the
        // current log)

        LOGTIME tmCreate;
        LONG lGenCurrent = LGGetCurrentFileGenWithLock( &tmCreate );
        Call( ErrLGUpdateGenRequired(
            m_pinst->m_pfsapi,
            pcheckpointT->checkpoint.le_lgposCheckpoint.le_lGeneration,
            pcheckpointT->checkpoint.le_lgposDbConsistency.le_lGeneration,
            lGenCurrent,
            tmCreate, // if the previous param is 0, this param is ignored
            &fSkippedAttachDetach,
            ifmpNil ) );

        if ( fSkippedAttachDetach )
        {
            //  since we're not going to be updating the checkpoint,
            //  this means some database headers may actually
            //  be ahead of the checkpoint, but that's fine, as all
            //  it means during recovery is that we'll try to
            //  replay a few extra logfiles
            //
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
            //  since we're not going to be updating the checkpoint,
            //  this means the database headers may actually
            //  be ahead of the checkpoint, but that's fine, as all
            //  it means during recovery is that we'll try to
            //  replay a few extra logfiles
            //
            //  for debuggability, record checkpoint update failure
            //  (if repeated failures occur trying to move from a
            //  particular checkpoint, just record the first error
            //  encountered)
            //
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

/*  update checkpoint file.
/**/
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

/*  retrieve the current checkpoint, not function can fail.
/**/
LGPOS LOG::LgposLGCurrentCheckpointMayFail() /* const, m_critCheckpoint.Enter() :P */
{
    // min is not what the one caller would prefer, but it is the safest
    LGPOS lgposRet = lgposMin;

    if ( m_critCheckpoint.FTryEnter() )
    {
        lgposRet = m_pcheckpoint->checkpoint.le_lgposCheckpoint;

        m_critCheckpoint.Leave();
    }

    return lgposRet;
}

/*  retrieve the current DB consistency point, not function can fail.
/**/
LGPOS LOG::LgposLGCurrentDbConsistencyMayFail() /* const, m_critCheckpoint.Enter() :P */
{
    LGPOS lgposRet = lgposMin;

    if ( m_critCheckpoint.FTryEnter() )
    {
        lgposRet = m_pcheckpoint->checkpoint.le_lgposDbConsistency;

        m_critCheckpoint.Leave();
    }

    return lgposRet;
}

/*  update waypoint position of the specified IFMP(s).
 *
 *  Parameters:
 *      pfsapi -
 *      ifmpTarget - The specific database to check for.  ifmpNil is
 *              the default, which means do all databases that
 *              are attached and logged.
/**/
ERR LOG::ErrLGUpdateWaypointIFMP( IFileSystemAPI *const pfsapi, _In_ const IFMP ifmpTarget )
{
    ERR err = JET_errSuccess;
    BOOL fOwnsChkptCritSec = fFalse;
    LOGTIME tmCreate;
    memset( &tmCreate, 0, sizeof(LOGTIME) );

    // First move the flush tip all the way forward so that Waypoint is not inhibited
    Call( ErrLGFlush( iofrLogMaxRequired ) );

    m_critCheckpoint.Enter();
    fOwnsChkptCritSec = fTrue;

    if ( m_fDisableCheckpoint // We're using this var in LGITryCleanupAfterRedoError() to prohibit header update here ...
        || m_fLogDisabled
// I took this check out of original checkpoint function I stole this 
// from (ErrLGUpdateCheckpointFile), it seems to be protecting checkpoint 
// advancement specifically, updating the headers is still allowed ...
//      || !m_fLGFMPLoaded
        || m_fLGNoMoreLogWrite
        || m_pinst->FInstanceUnavailable() )
    {
        if ( m_fLGNoMoreLogWrite
            || m_pinst->FInstanceUnavailable() )
        {
            // the regular checkpoint file upd function ... maybe they should
            // use ErrLGUpdateWaypointIFMP()
            Error( ErrERRCheck( JET_errInstanceUnavailable ) );
        }
        else
        {
            err = JET_errSuccess;
            goto HandleError;
        }
    }

    const LONG lGenCommitted = LGGetCurrentFileGenWithLock( &tmCreate );

    //  updatable waypoint
    //
    const BOOL fUpdateWaypoint = FLGIUpdatableWaypoint( lGenCommitted, ifmpTarget );
    //  Note: This may return a false positive, in that it checks all IFMPs.  However,
    //  we typically call this function when it is likely we need an update (such as
    //  during detach), so false positives are unlikely, and harmeless.

    //  if checkpoint or waypoint unchanged then return JET_errSuccess
    //
    if ( !fUpdateWaypoint )
    {
        //  I believe this can happen, but not too common ... would require checkpoint
        //  update (and thus waypoint update) to kick in just as we're detaching, such 
        //  that the waypoint was updated for detach before we got to the point this
        //  function is called in detach.  if so, we don't need to do anything.  This 
        //  also avoids excess header updates if we make a mistake.
        if ( ( !m_fRecovering || m_fRecoveringMode == fRecoveringUndo ) &&
            lGenCommitted < m_pLogWriteBuffer->LgposLogTip().lGeneration )
        {
            m_pLogWriteBuffer->FLGSignalWrite();    // async
        }
        err = JET_errSuccess;
        goto HandleError;
    }

    BOOL fSkippedAttachDetach;

    Assert( lGenCommitted != 0 );
    Call( ErrLGUpdateGenRequired(
        pfsapi,
        0,      // don't update the gen min required ...
        0,      // don't update the gen min consistent ...
        lGenCommitted,
        tmCreate, // if the previous param is 0, this param is ignored
        &fSkippedAttachDetach,
        ifmpTarget ) );

    // fSkippedAttachDetach can happen if we're trying to update all IFMPs and some of them
    // have not yet been reattached.
    Assert( !fSkippedAttachDetach || ifmpTarget == ifmpNil );

HandleError:
    if ( fOwnsChkptCritSec )
    {
        m_critCheckpoint.Leave();
        fOwnsChkptCritSec = fFalse;
    }
    return err;
}

/*  quiesces the waypoint latency of the specified IFMP, which means we'll
 *  write out the log buffer completely and flush the log, then update the
 *  database header to indicate that all log files are required up to the
 *  current generation.
 *
 *  Parameters:
 *      ifmpTarget - The specific database to quiesce the waypoint latency of.
/**/
ERR LOG::ErrLGQuiesceWaypointLatencyIFMP( _In_ const IFMP ifmpTarget )
{
    ERR err = JET_errSuccess;
    PIB pibFake;

    Assert( ifmpTarget != ifmpNil );
    FMP* const pfmp = g_rgfmp + ifmpTarget;

    if ( !pfmp->FLogOn() )
    {
        goto HandleError;
    }

    // Write out the entire log buffer.
    pibFake.m_pinst = m_pinst;
    Call( ErrLGWaitForWrite( &pibFake, &lgposMax ) );

    // Force an update with LLR disabled.
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

    // check if the stop lgpos is set
    //
    if ( !FLGRecoveryLgposStop() )
    {
        return fFalse;
    }

    Assert( CmpLgpos( &m_lgposRecoveryStop, &lgposMax ) != 0 );


    lgposEndOfLog = lgposMax;
    lgposEndOfLog.lGeneration = m_pLogStream->GetCurrentFileGen();


    // if the stop lgpos is not at log boundary, we do not stop at this generation
    // (we might stop at a log position though)
    //
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

    // check if the stop lgpos is set
    //
    if ( !FLGRecoveryLgposStop() )
    {
        return fFalse;
    }

    // check if the stop lgpos is not at log boundary
    //
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

     // recovery doesn't update the write tip  - or does so oddly, so can't validate flush tip against it.
    Assert( ( m_fRecovering && ( m_fRecoveringMode == fRecoveringRedo ) ) || 
            ( CmpLgpos( lgpos, m_pLogWriteBuffer->LgposWriteTip() ) <= 0 ) );
#endif  // DEBUG

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
        // Unfortunately, we sometimes roll the log at end of redo before changing recovery mode
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
#endif  // DEBUG

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

    //  if we have a checkpoint allocated and it points to the initial checkpoint and it's 
    //  been initialized to the previous sector size, then update it ...
    if ( m_pcheckpoint &&
         ( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec != USHORT( sizeof( LGFILEHDR ) / cbSecSize ) ) ||
         ( m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_isec != USHORT( sizeof( LGFILEHDR ) / cbSecSize ) ) )
    {
        //  should be at most, the le_isec for a 512 byte sector ...
        Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec <= sizeof( LGFILEHDR ) / 512 /* smallest sector size */ );
        Assert( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib == 0 );
        Assert( m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_isec <= sizeof( LGFILEHDR ) / 512 /* smallest sector size */ );
        Assert( m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_ib == 0 );

        // This may not hold ... if not, it's harmless, remove it ...
        Expected( m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_lGeneration == m_lgenInitial );

        //  re-fix up the checkpoint initial ...
        m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_isec = USHORT( sizeof( LGFILEHDR ) / cbSecSize );
        m_pcheckpoint->checkpoint.le_lgposCheckpoint.le_ib = 0;
        m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_isec = USHORT( sizeof( LGFILEHDR ) / cbSecSize );
        m_pcheckpoint->checkpoint.le_lgposDbConsistency.le_ib = 0;
    }
}

VOID LOG::LGFullLogNameFromLogId(
    __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName,
    LONG lGeneration,
    _In_ PCWSTR wszDirectory )
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
    _In_ PCWSTR wszLogFolder,
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

        // Being able to hold BF latches and acquire the write log buffer's critical
        // section (LOG_STREAM::m_critLGWrite) is a requirement we have today. Every
        // code path that emits log records and writes to the log stream today already
        // do that: see LOG_WRITE_BUFFER::ErrLGWriteLog and LOG_WRITE_BUFFER::FLGIAddLogRec.
        // Unfortunately, rankBFLatch is lower than rankLGWrite, which means we have a
        // rank violation situation. Those code paths explicitly disable deadlock detection
        // to avoid hitting that DEBUG assert.
        //
        // Fortunately, in practice, a real deadlock is not going to happen today because
        // the code paths that hold LOG_STREAM::m_critLGWrite are not going to block when
        // trying to latch a BF. They all do try-latches (e.g., checkpoint advancement).
        //
        // Therefore, we are extending this "forgiveness" to flushing the log because some
        // of those code paths that emit log records and write to the log stream may also
        // need to flush the log to make sure log records are persisted to physical storage.

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

    // First prevent any more logging
    m_pLogWriteBuffer->LockBuffer();
    SetNoMoreLogWrite( ErrERRCheck( errLogServiceStopped ) );
    m_pLogWriteBuffer->UnlockBuffer();

    // Wait for everything to be flushed - this will also emit end of log stream before doing the actual write
    CallR( m_pLogWriteBuffer->ErrLGWaitAllFlushed( fTrue ) );

    // In case nothing needed to be written out, we may still need to emit end of log stream
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
    //  set log disable state
    //
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

        // we are recovering which means we must bypass the enforcement of the
        // log file size and allow the user to recover (so long as all the
        // recovery logs are the same size)

        if ( m_lLogFileSizeDuringRecovery == 0 )
        {

            //  this is the first log the user is reading during recovery
            //  initialize the recovery log file size

            Assert( qwFileSize % 1024 == 0 );
            m_lLogFileSizeDuringRecovery = LONG( qwFileSize / 1024 );
        }

        //  enforce the recovery log file size

        qwSystemFileSize = QWORD( m_lLogFileSizeDuringRecovery ) * 1024;
    }
    else if ( !FDefaultParam( m_pinst, JET_paramLogFileSize ) )
    {

        //  we are not recovering, so we must enforce the size set by the user

        qwSystemFileSize = QWORD( UlParam( m_pinst, JET_paramLogFileSize ) ) * 1024;
    }
    else
    {

        // we are not recovering, but the user never set a size for us to
        // enforce. Set the size using the current log file on the user's behalf

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
            _In_ PCWSTR wszFindPath,
            LONG* plgenLow,
            LONG* plgenHigh,
            _In_ BOOL fLooseExt,
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
        // If we fail to allocate, we will just not stamp a valid logtimeGenMaxRequired for that generation
        // and lose that validation, do not treat as fatal error.
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

