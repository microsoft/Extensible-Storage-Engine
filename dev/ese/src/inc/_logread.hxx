// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


struct SAVED_LOG_DATA
{
    VOID *  m_pbSavedLogBuffer;
    UINT    m_iSavedsecStart;
    UINT    m_cSavedsec;
    WCHAR   m_wszSavedLogName[ IFileSystemAPI::cchPathMax ];
};

#define NUM_SAVED_LOGS      3

enum eLogCorruptReason
{
    eLCUnknown = 0,
    eLCEmptySegmentHigherLgen,
    eLCCorruptAfterEmptySegment,
    eLCBadSegmentLgpos,
    eLCValidSegmentAfterEmpty,
    eLCIOError,
    eLCSegmentCorrupt,
    eLCCannotPatchHardRecovery,
    eLCCorruptHeader,
    eLCMax,
};

class LOG_READ_BUFFER : public CZeroInit
{
public:
    LOG_READ_BUFFER( INST * pinst, LOG * pLog, ILogStream * pLogStream, LOG_BUFFER *pLogBuffer );
    ~LOG_READ_BUFFER();

    VOID GetLgposOfPbNext(LGPOS *plgpos) const;
    VOID GetLgposOfPbNextNext(LGPOS *plgpos) const;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    VOID GetLgposOfLastChecksum(LGPOS *plgpos) const;
#endif

    ERR ErrLReaderInit( UINT csecBufSize );
    ERR ErrLReaderTerm();
    VOID LReaderReset();

    ERR ErrReaderEnsureLogFile();

    ERR ErrReaderRestoreState(
            const LGPOS* const plgposPbNext,
            const LGPOS* const plgposPbNextNext
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
            ,const LGPOS* const plgposChecksum
#endif
            );

    ERR ErrLGCheckReadLastLogRecordFF(
            BOOL                    *pfCloseNormally,
            const BOOL              fUpdateAccSegChecksum = fFalse,
            const BOOL              fReadOnly = fFalse,
            BOOL                    *pfIsPatchable = NULL,
            LGPOS                   *plgposLastTerm = NULL);
    ERR ErrLGLocateFirstRedoLogRecFF( LE_LGPOS *plgposRedo, BYTE **ppbLR );
    ERR ErrLGGetNextRecFF( BYTE **ppbLR, BOOL fPreread = fFalse );

    const LGPOS& LgposFileEnd()
    {
        return m_lgposLastRec;
    }

    VOID ResetLgposFileEnd()
    {
        m_lgposLastRec.isec = 0;
    }

private:
    VOID GetLgposDuringReading( const BYTE * pb, LGPOS * plgpos ) const;

    BYTE* PbReaderGetEndOfData() const;
    UINT IsecGetNextReadSector() const;

    ERR ErrReaderEnsureSector( const UINT isec, const UINT csec, BYTE** ppb );
    VOID ReaderSectorModified( const UINT   isec, const BYTE* const pb );

    ERR ErrLGIGetRecordAtPbNext( BYTE **ppbLR, BOOL fPreread, BOOL fSkipPartialLR = fFalse );

    VOID ReportSegmentCorrection_( USHORT isec, ULONG cbOffset, INT ibitCorrupted );

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    ERR ErrLGCheckReadLastLogRecordFF_Legacy(
            BOOL                    *pfCloseNormally,
            const BOOL              fReadOnly       = fFalse,
            BOOL                    *pfIsPatchable  = NULL,
            LGPOS                   *plgposLastTerm = NULL);
    ERR ErrLGLocateFirstRedoLogRecFF_Legacy( LE_LGPOS *plgposRedo, BYTE **ppbLR );
    ERR ErrLGGetNextRecFF_Legacy( BYTE **ppbLR );
    ERR ErrLGIGetNextChecksumFF( LGPOS *plgposNextRec, BYTE **ppbLR );

    ULONG32 UlComputeChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration );
    ULONG32 UlComputeShadowChecksum( const ULONG32 ulOriginalChecksum );
    ULONG32 UlComputeShortChecksum( const LRCHECKSUM* const plrck, const ULONG32 lGeneration );

    BOOL FValidLRCKRecord(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos
        );
    BOOL FValidLRCKShadow(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos,
        const LONG lGeneration
        );
    BOOL FValidLRCKShadowWithoutCheckingCBNext(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos,
        const LONG lGeneration
        );
    BOOL FValidLRCKShortChecksum(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration
        );
    BOOL FValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration
        );

    void AssertValidLRCKRecord(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos
        );
    void AssertValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration
        );
    void AssertRTLValidLRCKRecord(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos
        );
    void AssertRTLValidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration
        );
    void AssertRTLValidLRCKShadow(
        const LRCHECKSUM * const plrck,
        const LGPOS * const plgpos,
        const LONG lGeneration
        );
    void AssertInvalidLRCKRange(
        const LRCHECKSUM * const plrck,
        const LONG lGeneration
        );

    //  location of last LRCHECKSUM record in the log buffer.
    //  It either hasn't been written out to disk yet, or it's
    //  been written to disk on one sector and with a shadow.
    //  Protected by m_critLGBuf
    BYTE        *m_pbLastChecksum;
    // Only used for recovery:
    LGPOS       m_lgposLastChecksum;

#endif  // ENABLE_LOG_V7_RECOVERY_COMPAT

    INST *      m_pinst;
    LOG *       m_pLog;
    ILogStream *m_pLogStream;

    LOG_BUFFER *m_pLogBuffer;

    //  in memory log buffer variables for reading log records
    BOOL        m_fReaderInited;
    BYTE        *m_pbNext;      // location of current log record
    BYTE        *m_pbNextNext;  // location of next log record to read

    // copy of assembled fragmented LR - need to maintain 2 copies, one for
    // normal reading and another for preread
    BYTE *      m_pvAssembledLR[2];
    DWORD       m_cbAssembledLR[2];
    LRNOP       m_lrNop;

    // timestamp of first segment, to assist with recovery IO smoothing
    __int64     m_ftFirstSegment;
    HRT         m_hrtFirstSegment;

    BOOL        m_fReadSectorBySector;
    UINT        m_isecReadStart;    // first sector we have from disk
    UINT        m_csecReader;       // number of sectors we have from disk
    BOOL    m_fIgnoreNextSectorReadFailure; // can we ignore the next sector read failure, fixable with shadow sector
    WCHAR       m_wszReaderLogName[ IFileSystemAPI::cchPathMax ];   // the log that we have in our buffers

#ifdef DEBUG
    // saved log buffer from last log for debugging purposes
    SAVED_LOG_DATA  m_SavedLogData[NUM_SAVED_LOGS];
    UINT            m_cNextSavedLog;
#endif

    //  variables for scanning during redo

    // During redo, if m_lgposLastRec.isec != 0, we don't want to use
    // any log record whose LGPOS is greater than or equal to m_lgposLastRec.
    LGPOS           m_lgposLastRec; // sentinal for last log record for redo */
};

