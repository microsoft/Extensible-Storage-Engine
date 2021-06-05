// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

enum LOG_VERIFY_STATUS
{
    LogVerifyHeader,
    LogVerifyLogSegments,
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    LogVerifyLRCKStart,
    LogVerifyLRCKContinue,
#endif
    LogVerifyDone
};

struct LOG_VERIFY_STATE
{
    LOG_VERIFY_STATE()
        : m_state( LogVerifyHeader )
    {
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        memset( &m_lrck, 0, sizeof( m_lrck ) );
#endif
    }

    ERR ErrVerifyHeader( INST *pinst, IFileAPI *pfapi, __deref_in_bcount(*pcb) BYTE **ppb, DWORD *pcb );
    ERR ErrVerifyLogSegments( _Inout_updates_bytes_(cb) BYTE *pb, DWORD cb );
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    ERR ErrVerifyLRCKStart( __in_bcount(cb) BYTE *pb, DWORD cb );
    ERR ErrVerifyLRCKContinue( __in_bcount(cb) BYTE *pb, DWORD cb );
    ERR ErrVerifyLRCKEnd();
#endif

    // state for new format
    LOG_VERIFY_STATUS   m_state;
    DWORD               m_cbSeg;
    DWORD               m_iSeg;

#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    // maintains state across multiple file reads
    DWORD               m_cbSeen;
    DWORD               m_ibLRCK;
    ULONG               m_ibChecksumIncremental;
    CHECKSUMINCREMENTAL m_ck;
    LRCHECKSUM          m_lrck;
    ULONG               m_lGeneration;
#endif
};

ERR ErrLGReadAndVerifyFile(
    INST *              pinst,
    IFileAPI *          pfapi,
    DWORD               cbOffset,
    DWORD               cbFileSize,
    LOG_VERIFY_STATE *  pState,
    const TraceContext& tc,
    BYTE *              pb,
    DWORD               cbToRead );

