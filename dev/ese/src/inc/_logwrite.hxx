// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


class LOG_WRITE_BUFFER : public CZeroInit
{
public:
    LOG_WRITE_BUFFER( INST * pinst, LOG * pLog, ILogStream * pLogStream, LOG_BUFFER * pLogBuffer );
    ~LOG_WRITE_BUFFER();

    ERR ErrLGInit();

    VOID GetLgposOfPbEntry( LGPOS *plgpos ) const;

    ERR ErrLGTasksInit( void );
    VOID LGTasksTerm( void );

    ERR ErrLGLogRec(
        const DATA *rgdata,
        const INT cdata,
        const BOOL fLGFlags,
        const LONG lgenBegin0,
        LGPOS *plgposLogRec );

    VOID WriteToClearSpace();
    BOOL FLGWriteLog( IOREASONPRIMARY iorp, BOOL fHasSignal, const BOOL fDisableDeadlockDetection );
    ERR ErrLGWriteLog( IOREASONPRIMARY iorp, const BOOL fWriteAll = fFalse );

    BOOL FLGSignalWrite();

    ERR ErrLGWaitForLogGen( const LONG lGeneration );
    ERR ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec );
    ERR ErrLGWaitAllFlushed( BOOL fFillPartialSector );
    ERR ErrLGScheduleWrite( DWORD cmsecDurableCommit, LGPOS lgposCommit );
    VOID VerifyAllWritten();

    VOID AdvanceLgposToWriteToNewGen( const LONG lGeneration );

    VOID InitLogBuffer( const BOOL  fLGFlags );

    LGPOS LgposWriteTip()
    {
        return m_lgposToWrite;
    }

    VOID SetLgposWriteTip(LGPOS &lgpos)
    {
        m_lgposToWrite = lgpos;
    }

    LGPOS LgposLogTip()
    {
        return m_lgposLogRec;
    }

    VOID LockBuffer()
    {
        m_critLGBuf.Enter();
    }

    VOID UnlockBuffer()
    {
        m_critLGBuf.Leave();
    }
#ifdef DEBUG
    BOOL FOwnsBufferLock() const
    {
        return m_critLGBuf.FOwner();
    }
#endif

    BOOL FLGProbablyWriting()
    {
        return m_pLogStream->FLGProbablyWriting();
    }
    
    VOID CheckIsReady() const
    {
        //  should be set properly
        //
        Assert( m_isecWrite != 0 );

        //  m_pbEntry and m_pbWrite were set for next record in LocateLastLogRecord

        Assert( m_pbWrite == m_pLogStream->PbSecAligned( m_pbWrite, m_pbLGBufMin ) );
        Assert( m_pbWrite <= m_pbEntry );
        Assert( m_pbEntry <= m_pbWrite + m_pLogStream->CbSec() );
    }

    VOID SetFNewRecordAdded( BOOL fAdded )
    {
        m_pLogStream->LockWrite();
        m_critLGBuf.Enter();
        m_fNewLogRecordAdded = fAdded;
        m_critLGBuf.Leave();
        m_pLogStream->UnlockWrite();
    }

    VOID ResetWritePosition()
    {
        m_isecWrite = m_pLogStream->CSecHeader();
        m_pbLGFileEnd = pbNil;
        m_isecLGFileEnd = 0;

        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlDatapoint|sysosrtlContextInst, m_pinst, (PVOID)&(m_pinst->m_iInstance), sizeof(m_pinst->m_iInstance) );
    }

#ifdef DEBUG
    VOID ResetLgposLastLogRec()
    {
        m_lgposLastLogRec = lgposMin;
    }
#endif

    BOOL FStopOnNewLogGeneration() const
    {
        return m_fStopOnNewLogGeneration;
    }

    VOID ResetStopOnNewLogGeneration()
    {
        m_fStopOnNewLogGeneration = fFalse;
    }

    BOOL FLGLogPaused() const
    {
        return m_fLogPaused;
    }

    VOID LGSetLogPaused( BOOL fValue )
    {
        m_fLogPaused = fValue;
    }

    VOID LGSignalLogPaused( BOOL fSet )
    {
        if ( fSet )
        {
            m_sigLogPaused.Set();
        }
        else
        {
            m_sigLogPaused.Reset();
        }
    }

    VOID LGWaitLogPaused()
    {
        m_sigLogPaused.Wait();
        Assert( m_fLogPaused || m_pLog->FNoMoreLogWrite() );
    }

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

private:
    BOOL FLGIAddLogRec(
        const DATA *        rgdata,
        const INT           cdata,
        BOOL                fAllocOnly,
        __deref BYTE**      ppbET );

    // AKR_TODO
    BOOL FWakeWaitingQueue(
        const LGPOS* const plgposToWrite
        );
    
    ERR ErrLGIWriteFullSectors(
        const IOREASONPRIMARY iorp,
        const UINT            csecFull,
        const UINT            isecWrite,
        const BYTE* const     pbWrite,
        BOOL* const           pfWaitersExist,
        const LGPOS&          lgposMaxWritePoint
        );
    ERR ErrLGIWritePartialSector(
        const IOREASONPRIMARY iorp,
        BYTE*                 pbWriteEnd,
        UINT                  isecWrite,
        BYTE*                 pbWrite,
        const LGPOS&          lgposMaxWritePoint
        );

    ERR ErrLGIDeferredWrite( const IOREASONPRIMARY iorp, const BOOL fWriteAll );

    BYTE* PbGetEndOfLogData();

    static VOID LGWriteLog_( LOG_WRITE_BUFFER* const pLogBuffer );
    static VOID LGLazyCommit_( VOID *pGroupContext, VOID *pLogBuffer );

    ERR ErrLGIWriteLog(
        const IOREASONPRIMARY iorp,
        const BOOL fWriteAll = fFalse );

    BOOL EnsureCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    VOID Decommit( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    static VOID DecommitLogBufferTask( VOID *, VOID * );
    
    INST *          m_pinst;
    LOG *           m_pLog;
    ILogStream *    m_pLogStream;

    LOG_BUFFER *    m_pLogBuffer;

    BYTE *          m_pvPartialSegment;

    LGPOS           m_lgposLogRec;          //  last log record entry, updated by ErrLGLogRec

    LGPOS           m_lgposLogRecBasis;     //  log record used as basis for log bytes generated, updated by ErrLGLogRec

    BOOL            m_fNewLogRecordAdded;   // protected by m_critLGBuf

    //  Protected by m_critLGWrite.
    //  Whether we have a shadow sector on disk at sector ( m_isecWrite + 1 ).
    //  We use this to determine whether we need to split a multi-sector
    //  full sector write into 2 I/Os instead of doing it as 1 I/O.
    BOOL            m_fHaveShadow;

    //  sync mechanism for initiating a log write

    CSemaphore      m_semLogSignal;
    CSemaphore      m_semLogWrite;

    //  semaphore used to wait for available log buffer space

    CSemaphore      m_semWaitForLogBufferSpace;

    //  crit sequence: m_critLGWrite->m_critLGBuf->m_critLGWaitQ

    CCriticalSection    m_critLGWaitQ;

    //  sync mechanism for copying data into log buffers
    //  and writing that data to the logs

    CMeteredSection     m_msLGPendingCopyIntoBuffer;

    //  count of users waiting for log write

    // AKR_TODO
    PIB             *m_ppibLGWriteQHead;
    PIB             *m_ppibLGWriteQTail;

    //  Perfmon stuff: also monitoring statistics

    // Number of times we've used the VM wraparound trick.
    // Also the number of I/Os we've saved by using it.
    ULONG   m_cLGWrapAround;

    //  if TRUE, log write thread should set
    //  m_fAboutToWriteSnapshot to TRUE to
    //  preclude further log writes (until
    //  it gets reset)
    //
    BOOL            m_fStopOnNewLogGeneration;

    //  if TRUE, only the thread requesting to stop
    //  on a new log generation may enter log records
    //  into the log buffer
    //
    BOOL            m_fWaitForLogRecordAfterStopOnNewGen;

    //  if TRUE, log write thread should bail immediately
    //  because a snapshot backup is going to be taken
    //
    //  note that this flag will get reset once the snapshot
    //  backup is thawed
    //
    BOOL                m_fLogPaused;

    CManualResetSignal  m_sigLogPaused;             //  signal that space request loop has terminated

#ifdef DEBUG
    LGPOS           m_lgposLastLogRec;

    DWORD           m_dwDBGLogThreadId;
    BOOL            m_fDBGTraceLog;
#endif

    TICK            m_tickNextLazyCommit;
    LGPOS           m_lgposNextLazyCommit;
    CCriticalSection    m_critNextLazyCommit;
    POSTIMERTASK    m_postLazyCommitTask;
    TICK            m_tickLastWrite;
    TICK            m_tickDecommitTaskSchedule;
    POSTIMERTASK    m_postDecommitTask;
    BOOL            m_fDecommitTaskScheduled;
};

