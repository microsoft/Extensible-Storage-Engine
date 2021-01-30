// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define cbRBSSegmentSize                8192
#define cbRBSSegmentSizeMask            0x1fff
#define shfRBSSegmentSize               13
#define cbRBSBufferSize                 512*1024
#define cbRBSAttach                     4480
#define cRBSSegmentMax                  0x7fff0000
#define cbRBSSegmentsInBuffer           (cbRBSBufferSize/cbRBSSegmentSize)
#define csecSpaceUsagePeriodicLog       3600

C_ASSERT( cbRBSSegmentSizeMask == cbRBSSegmentSize - 1 );

INLINE INT CmpRbspos( const RBS_POS& rbspos1, const RBS_POS& rbspos2 )
{
    if constexpr( FHostIsLittleEndian() )
    {
        if ( rbspos1.qw > rbspos2.qw )
            return 1;
        if ( rbspos1.qw < rbspos2.qw )
            return -1;
        return 0;
    }
    else
    {
        INT t;
        if ( 0 == (t = rbspos1.lGeneration - rbspos2.lGeneration) )
        {
            return rbspos1.iSegment - rbspos2.iSegment;
        }
        return t;
    }
}

INLINE INT CmpRbspos( const RBS_POS* prbspos1, const RBS_POS* prbspos2 )
{
    return CmpRbspos( *prbspos1, *prbspos2 );
}

#include <pshpack1.h>


PERSISTED
struct RBSFILEHDR_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;
    LittleEndian<LONG>          le_lGeneration;

    LOGTIME                     tmCreate;
    LOGTIME                     tmPrevGen;

    LittleEndian<ULONG>         le_ulMajor;
    LittleEndian<ULONG>         le_ulMinor;

    LittleEndian<QWORD>         le_cbLogicalFileSize;

    SIGNATURE                   signRBSHdrFlush;

    LittleEndian<USHORT>        le_cbDbPageSize;
    BYTE                        rgbReserved1[2];

    LittleEndian<LONG>          le_lGenMinLogCopied;
    LittleEndian<LONG>          le_lGenMaxLogCopied;
    BYTE                        bLogsCopied;

    BYTE                        rgbReserved[586];

    UnalignedLittleEndian<ULONG>    le_filetype;
};

C_ASSERT( offsetof( RBSFILEHDR_FIXED, le_ulChecksum ) == offsetof( DBFILEHDR, le_ulChecksum ) );
C_ASSERT( offsetof( RBSFILEHDR_FIXED, le_filetype ) == offsetof( DBFILEHDR, le_filetype ) );

PERSISTED
class RBSATTACHINFO
{
    private:
        BYTE                                bPresent;
        UnalignedLittleEndian<DBTIME>       mle_dbtimeDirtied;
        UnalignedLittleEndian<DBTIME>       mle_dbtimePrevDirtied;

        UnalignedLittleEndian<LONG>         mle_lGenMinRequired;

        UnalignedLittleEndian<LONG>         mle_lGenMaxRequired;

    public:
        SIGNATURE                           signDb;

        SIGNATURE                           signDbHdrFlush;

        UnalignedLittleEndian< WCHAR >      wszDatabaseName[IFileSystemAPI::cchPathMax];

        BYTE                                rgbReserved[39];

    public:
        INLINE BYTE FPresent() const                                           { return bPresent; }
        INLINE VOID SetPresent( const BYTE bValue )                            { bPresent = bValue; }

        INLINE DBTIME DbtimeDirtied() const                                     { return mle_dbtimeDirtied; }
        INLINE VOID SetDbtimeDirtied( LittleEndian<DBTIME> dbtime )             { mle_dbtimeDirtied = dbtime; }

        INLINE DBTIME DbtimePrevDirtied() const                                 { return mle_dbtimePrevDirtied; }
        INLINE VOID SetDbtimePrevDirtied( LittleEndian<DBTIME> dbtime )         { mle_dbtimePrevDirtied = dbtime; }

        INLINE LONG LGenMinRequired() const                                     { return mle_lGenMinRequired; }
        INLINE VOID SetLGenMinRequired( LONG lGenMin )                          { mle_lGenMinRequired = lGenMin; }

        INLINE LONG LGenMaxRequired() const                                     { return mle_lGenMaxRequired; }
        INLINE VOID SetLGenMaxRequired( LONG lGenMax )                          { mle_lGenMaxRequired = lGenMax; }

        #ifdef DEBUGGER_EXTENSION
        VOID     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
        #endif
};

PERSISTED
struct RBSFILEHDR
{
    RBSFILEHDR_FIXED            rbsfilehdr;

    BYTE                        rgbAttach[cbRBSAttach];

    BYTE                        rgbPadding[cbRBSSegmentSize - sizeof(RBSFILEHDR_FIXED) - cbRBSAttach];

    #ifdef DEBUGGER_EXTENSION
    VOID     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
    #endif
};

C_ASSERT( sizeof( RBSFILEHDR ) == cbRBSSegmentSize );
C_ASSERT( sizeof( RBSFILEHDR::rgbAttach ) == sizeof(RBSATTACHINFO)*dbidMax );


PERSISTED
struct RBSSEGHDR
{
    LittleEndian<XECHECKSUM>    checksum;
    LittleEndian<ULONG>         le_iSegment;
    LOGTIME                     logtimeSegment;
    BYTE                        rgbReserved[16];
    LittleEndian<ULONG>         fFlags;

    #ifdef DEBUGGER_EXTENSION
    VOID     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
    #endif
};

C_ASSERT( offsetof( RBSSEGHDR, checksum ) == offsetof( CPAGE::PGHDR, checksum ) );
C_ASSERT( offsetof( RBSSEGHDR, fFlags ) == offsetof( CPAGE::PGHDR, fFlags ) );

#include <poppack.h>

INLINE QWORD IbRBSFileOffsetOfSegment( ULONG segment )      { return QWORD( segment + 1 ) << shfRBSSegmentSize; }
INLINE ULONG IsegRBSSegmentOfFileOffset( QWORD ib )         { return ULONG( ( ib >> shfRBSSegmentSize ) - 1 ); }
INLINE ULONG CsegRBSCountSegmentOfOffset( DWORD ib )        { return ib >> shfRBSSegmentSize; }
INLINE ULONG IbRBSSegmentOffsetFromFullOffset( DWORD ib )   { return ib & cbRBSSegmentSizeMask; }

#include "revertsnapshotrecords.h"

const ULONG ulRBSVersionMajor         = 1;
const ULONG ulRBSVersionMinor         = 2;

class CRevertSnapshot;

struct CSnapshotBuffer
{
#pragma push_macro( "new" )
#undef new
    void* operator new( size_t ) = delete;
    void* operator new[]( size_t ) = delete;
    void operator delete[]( void* ) = delete;

    void* operator new( size_t cbAlloc, CResource *pcresRBSBuf )
    {
        Assert( cbAlloc == sizeof(CSnapshotBuffer) );
        return pcresRBSBuf->PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        CSnapshotBuffer *pBuf = (CSnapshotBuffer *)pv;
        pBuf->m_pcresRBSBuf->Free( pv );
    }
#pragma pop_macro( "new" )

    CSnapshotBuffer( ULONG startingSegment, CResource *pcresRBSBuf )
    {
        Reset( startingSegment );
        m_pBuffer = NULL;
        m_pcresRBSBuf = pcresRBSBuf;
    }

    virtual ~CSnapshotBuffer()
    {
        if ( m_pBuffer != NULL )
        {
            if ( AtomicCompareExchangePointer( &s_pReserveBuffer, NULL, m_pBuffer ) != NULL )
            {
                OSMemoryPageFree( m_pBuffer );
            }
            m_pBuffer = NULL;
        }
    }

    VOID Reset( ULONG startingSegment )
    {
        m_cStartSegment = startingSegment;
        m_ibNextRecord = sizeof( RBSSEGHDR );
        m_cbValidData = 0;
        m_pNextBuffer = NULL;
    }

    ERR ErrAllocBuffer()
    {
        Assert( m_pBuffer == NULL );
        ERR err;
        m_pBuffer = (BYTE *)AtomicExchangePointer( &s_pReserveBuffer, NULL );
        if ( m_pBuffer == NULL )
        {
            AllocR( m_pBuffer = (BYTE *)PvOSMemoryPageAlloc( cbRBSBufferSize, NULL ) );
        }
        return JET_errSuccess;
    }

    BYTE *m_pBuffer;
    ULONG m_cStartSegment;
    ULONG m_ibNextRecord;
    ULONG m_cbValidData;
    CSnapshotBuffer *m_pNextBuffer;
    CResource *m_pcresRBSBuf;

    static VOID PreAllocReserveBuffer()
    {
        if ( s_pReserveBuffer == NULL )
        {
            BYTE *pBuffer = (BYTE *)PvOSMemoryPageAlloc( cbRBSBufferSize, NULL );
            if ( pBuffer != NULL &&
                 AtomicCompareExchangePointer( &s_pReserveBuffer, NULL, pBuffer ) != NULL )
            {
                OSMemoryPageFree( pBuffer );
            }
        }
    }

    static VOID FreeReserveBuffer()
    {
        VOID *pBuffer = AtomicExchangePointer( &s_pReserveBuffer, NULL );
        if ( pBuffer != NULL )
        {
            OSMemoryPageFree( pBuffer );
        }
    }

    static VOID *s_pReserveBuffer;
};

struct CSnapshotReadBuffer : public CSnapshotBuffer
{
    CSnapshotReadBuffer( ULONG startingSegment )
        : CSnapshotBuffer( startingSegment, NULL ),
          m_pvAssembledRec( NULL ),
          m_cbAssembledRec( 0 )
    {}

#pragma push_macro( "new" )
#undef new
    void* operator new( size_t size )
    {
        Assert( size == sizeof(CSnapshotReadBuffer) );
        return new BYTE[size];
    }

    void operator delete( void* pv )
    {
        delete[] (BYTE *)pv;
    }
#pragma pop_macro( "new" )

    virtual ~CSnapshotReadBuffer()
    {
        free( m_pvAssembledRec );
    }

    BYTE *m_pvAssembledRec;
    ULONG m_cbAssembledRec;
};

#ifdef DEBUGGER_EXTENSION

INLINE VOID RBSATTACHINFO::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    (*pcprintf)( FORMAT_UINT( RBSATTACHINFO, this, mle_dbtimeDirtied, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RBSATTACHINFO, this, mle_dbtimePrevDirtied, dwOffset ) );    
    (*pcprintf)( FORMAT_INT( RBSATTACHINFO, this, mle_lGenMinRequired, dwOffset ) );
    (*pcprintf)( FORMAT_INT( RBSATTACHINFO, this, mle_lGenMaxRequired, dwOffset ) );
    
    (*pcprintf)( FORMAT_VOID( RBSATTACHINFO, this, signDb, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( RBSATTACHINFO, this, signDbHdrFlush, dwOffset ) );
    (*pcprintf)( FORMAT_VOID( RBSATTACHINFO, this, wszDatabaseName, dwOffset ) );
}

INLINE VOID RBSFILEHDR::Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset ) const
{
    (*pcprintf)( FORMAT_UINT( RBSFILEHDR, this, rbsfilehdr.le_ulChecksum, dwOffset ) );
    (*pcprintf)( FORMAT_INT( RBSFILEHDR, this, rbsfilehdr.le_lGeneration, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( RBSFILEHDR, this, rbsfilehdr.tmCreate, dwOffset ) );
    (*pcprintf)( FORMAT_LOGTIME( RBSFILEHDR, this, rbsfilehdr.tmPrevGen, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RBSFILEHDR, this, rbsfilehdr.le_ulMajor, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RBSFILEHDR, this, rbsfilehdr.le_ulMinor, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RBSFILEHDR, this, rbsfilehdr.le_cbLogicalFileSize, dwOffset ) ); 
    (*pcprintf)( FORMAT_VOID( RBSFILEHDR, this, rbsfilehdr.signRBSHdrFlush, dwOffset ) );
    (*pcprintf)( FORMAT_INT( RBSFILEHDR, this, rbsfilehdr.le_lGenMinLogCopied, dwOffset ) );
    (*pcprintf)( FORMAT_INT( RBSFILEHDR, this, rbsfilehdr.le_lGenMaxLogCopied, dwOffset ) );
    (*pcprintf)( FORMAT_BOOL( RBSFILEHDR, this, rbsfilehdr.bLogsCopied, dwOffset ) );
    (*pcprintf)( FORMAT_UINT( RBSFILEHDR, this, rbsfilehdr.le_filetype, dwOffset ) );

    for( const BYTE * pbT = this->rgbAttach; 0 != *pbT; pbT += sizeof( RBSATTACHINFO ) )
    {
        RBSATTACHINFO* prbsattachinfo = (RBSATTACHINFO*) pbT;

        if ( prbsattachinfo->FPresent() == 0 )
        {
            break;
        }

        prbsattachinfo->Dump( pcprintf );
    }
}

#endif

class IRBSCleanerIOOperator
{
public:
    virtual ~IRBSCleanerIOOperator() {}

    virtual ERR ErrRBSDiskSpace( QWORD* pcbFreeForUser ) = 0;

    virtual ERR ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize ) = 0;

    virtual ERR ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax ) = 0;

    virtual ERR ErrRBSFilePathForGen( __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, LONG cbDirPath, __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, LONG cbFilePath, LONG lRBSGen ) = 0;

    virtual ERR ErrRBSFileHeader( PCWSTR wszRBSFilePath,  _Out_ RBSFILEHDR* prbsfilehdr ) = 0;

    virtual ERR ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason) = 0;

    virtual PCWSTR WSZRBSAbsRootDirPath() = 0;
};

class RBSCleanerIOOperator : public IRBSCleanerIOOperator
{
public:
    RBSCleanerIOOperator( INST* pinst );
    virtual ~RBSCleanerIOOperator();

    ERR ErrRBSInitPaths();
    ERR ErrRBSDiskSpace( QWORD* pcbFreeForUser );
    ERR ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize );
    ERR ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax );
    ERR ErrRBSFilePathForGen( __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, LONG cbDirPath, __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, LONG cbFilePath, LONG lRBSGen );
    ERR ErrRBSFileHeader( PCWSTR wszRBSFilePath, _Out_ RBSFILEHDR* prbsfilehdr );
    ERR ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason);
    PCWSTR WSZRBSAbsRootDirPath();

private:
    INST*                           m_pinst;

    PWSTR                           m_wszRBSAbsRootDirPath;
    PWSTR                           m_wszRBSBaseName;
};

INLINE PCWSTR RBSCleanerIOOperator::WSZRBSAbsRootDirPath() { return m_wszRBSAbsRootDirPath; }

class IRBSCleanerConfig
{
protected:
    IRBSCleanerConfig() {}
public:
    virtual ~IRBSCleanerConfig() {}

    virtual INT CPassesMax() = 0;

    virtual BOOL FEnableCleanup() = 0;

    virtual QWORD CbLowDiskSpaceThreshold() = 0;

    virtual QWORD CbMaxSpaceForRBSWhenLowDiskSpace() = 0;

    virtual INT CSecRBSMaxTimeSpan() = 0;
    
    virtual INT CSecMinCleanupIntervalTime() = 0;

    virtual LONG LFirstValidRBSGen() = 0;
};


class RBSCleanerConfig : public IRBSCleanerConfig
{
private:
    INST* m_pinst;
public:
    RBSCleanerConfig( INST* pinst ) { Assert( pinst ); m_pinst = pinst; }
    virtual ~RBSCleanerConfig() {}
    INT CPassesMax() { return INT_MAX; }
    BOOL FEnableCleanup();
    QWORD CbLowDiskSpaceThreshold();
    QWORD CbMaxSpaceForRBSWhenLowDiskSpace();
    INT CSecRBSMaxTimeSpan();
    INT CSecMinCleanupIntervalTime();
    LONG LFirstValidRBSGen() { return 1; }
};


INLINE BOOL RBSCleanerConfig::FEnableCleanup()
{
    return BoolParam( m_pinst, JET_paramFlight_RBSCleanupEnabled );
}

INLINE QWORD RBSCleanerConfig::CbLowDiskSpaceThreshold()
{
    return ( (QWORD) UlParam( m_pinst, JET_paramFlight_RBSLowDiskSpaceThresholdGb ) ) * 1024 * 1024 * 1024;
}

INLINE QWORD RBSCleanerConfig::CbMaxSpaceForRBSWhenLowDiskSpace()
{
    return ( (QWORD) UlParam( m_pinst, JET_paramFlight_RBSMaxSpaceWhenLowDiskSpaceGb ) ) * 1024 * 1024 * 1024;
}

INLINE INT RBSCleanerConfig::CSecMinCleanupIntervalTime()
{
    return ( INT )UlParam( m_pinst, JET_paramFlight_RBSCleanupIntervalMinSec );
}

INLINE INT RBSCleanerConfig::CSecRBSMaxTimeSpan()
{
    return ( INT )UlParam( m_pinst, JET_paramFlight_RBSMaxTimeSpanSec );
}

class IRBSCleanerState
{
protected:
    IRBSCleanerState() {}
public:
    virtual ~IRBSCleanerState() {}

    virtual __int64 FtPrevPassCompletionTime() = 0;

    virtual __int64 FtPassStartTime() = 0;
    virtual VOID SetPassStartTime() = 0;

    virtual INT CPassesFinished() = 0;

    virtual VOID CompletedPass() = 0;
};

class RBSCleanerState : public IRBSCleanerState
{
private:
    __int64     m_ftPrevPassCompletionTime;
    __int64     m_ftPassStartTime;
    INT         m_cPassesFinished;
public:
    RBSCleanerState();
    virtual ~RBSCleanerState() {}
   
    __int64 FtPrevPassCompletionTime() { return m_ftPrevPassCompletionTime; }
    __int64 FtPassStartTime() { return m_ftPassStartTime; }
    VOID SetPassStartTime() { m_ftPassStartTime = UtilGetCurrentFileTime(); }
    INT CPassesFinished() { return m_cPassesFinished; }
    VOID CompletedPass();
};

INLINE VOID RBSCleanerState::CompletedPass()
{
    m_ftPrevPassCompletionTime = UtilGetCurrentFileTime();
    m_cPassesFinished++;
}

namespace RBSCleanerFactory
{
    ERR ErrRBSCleanerCreate( INST*  pinst, __out RBSCleaner ** prbscleaner);
}

class RBSCleaner : public CZeroInit
{
public:
    RBSCleaner( INST* pinst, IRBSCleanerIOOperator* const prbscleaneriooperator, IRBSCleanerState* const prbscleanerstate, IRBSCleanerConfig* const prbscleanerconfig );
    ~RBSCleaner();

    VOID TermCleaner( );

    ERR ErrStartCleaner( );
    VOID SetFirstValidGen( long lrbsgen );
    BOOL FIsCleanerRunning( ) const { return ( NULL != m_threadRBSCleaner ); }

private:

    INST*                               m_pinst;

    volatile LONG                       m_lFirstValidRBSGen;
    volatile BOOL                       m_fValidRBSGenSet;

    unique_ptr<IRBSCleanerIOOperator>   m_prbscleaneriooperator;
    unique_ptr<IRBSCleanerState>        m_prbscleanerstate;
    unique_ptr<IRBSCleanerConfig>       m_prbscleanerconfig;

    THREAD                              m_threadRBSCleaner;
    CManualResetSignal                  m_msigRBSCleanerStop;

    CCriticalSection                    m_critRBSFirstValidGen;

    static DWORD  DwRBSCleanerThreadProc( DWORD_PTR dwContext );    

    DWORD   DwRBSCleaner( );
    VOID    WaitForMinPassTime( );
    VOID    ComputeFirstValidRBSGen();
    ERR     ErrDoOneCleanupPass( );
    BOOL    FMaxPassesReached( ) const;
    BOOL    FGenValid( long lrbsgen );
};

INLINE VOID RBSCleaner::SetFirstValidGen( long lrbsgen )
{
    ENTERCRITICALSECTION critRBSFirstValidGen( &m_critRBSFirstValidGen );

    if ( lrbsgen > m_lFirstValidRBSGen )
    {
        m_lFirstValidRBSGen = lrbsgen;
        m_fValidRBSGenSet = fTrue;
    }
}

INLINE BOOL RBSCleaner::FGenValid( long lrbsgen )
{
    return lrbsgen >= m_lFirstValidRBSGen;
}

INLINE BOOL RBSCleaner::FMaxPassesReached() const
{
    return m_prbscleanerstate->CPassesFinished() >= m_prbscleanerconfig->CPassesMax();
}

const TICK dtickRBSFlushInterval = 30 * 1000;

class CRevertSnapshot : public CZeroInit
{
 public:
    CRevertSnapshot( __in INST* const pinst );

    ~CRevertSnapshot();

    ERR ErrRBSInit( BOOL fRBSCreateIfMissing );
    ERR ErrRBSRecordDbAttach( __in FMP* const pfmp );
    ERR ErrRBSInitDBFromRstmap( __in const RSTMAP* const prstmap, LONG lgenLow, LONG lgenHigh );

    DBTIME GetDbtimeForFmp( FMP *pfmp );
    ERR ErrSetDbtimeForFmp( FMP *pfmp, DBTIME dbtime );

    VOID Term();

    ERR ErrCapturePreimage(
            DBID dbid,
            PGNO pgno,
            _In_reads_( cbImage ) const BYTE *pbImage,
            ULONG cbImage,
            RBS_POS *prbsposRecord );

    ERR ErrCaptureNewPage(
            DBID dbid,
            PGNO pgno,
            RBS_POS *prbsposRecord );

    RBS_POS RbsposFlushPoint() const
    {
        RBS_POS pos;
        pos.lGeneration = m_prbsfilehdrCurrent->rbsfilehdr.le_lGeneration;
        pos.iSegment = m_cNextFlushSegment - 1;
        return pos;
    }
    ERR ErrFlushAll();
    VOID AssertAllFlushed()
    {
        Assert( m_cNextFlushSegment == m_cNextWriteSegment &&
                ( m_pActiveBuffer == NULL || m_pActiveBuffer->m_ibNextRecord <= sizeof(RBSSEGHDR) ) );
    }

    ERR ErrSetReadBuffer( ULONG iStartSegment );
    ERR ErrGetNextRecord( RBSRecord **ppRecord, RBS_POS* lgposRecStart, _Out_ PWSTR wszErrReason );

    BOOL FRollSnapshot();
    ERR ErrRollSnapshot( BOOL fPrevRBSValid, BOOL fInferFromRstmap );
    
    ERR ErrSetRBSFileApi( __in IFileAPI *pfapirbs );

    RBSFILEHDR*      RBSFileHdr() const          { return m_prbsfilehdrCurrent; }

    VOID SetIsDumping() { m_fDumping = fTrue; }

    BOOL FTooLongSinceLastFlush()
    {
        TICK tickNow, tickLast;
        tickNow = TickOSTimeCurrent();
        tickLast = AtomicRead( &m_tickLastFlush );
        if ( DtickDelta( tickLast, tickNow ) >= dtickRBSFlushInterval &&
             AtomicCompareExchange( &m_tickLastFlush, tickLast, tickNow ) == tickLast )
        {
            return fTrue;
        }
        return fFalse;
    }

    static VOID EnterDbHeaderFlush( CRevertSnapshot* prbs, __out SIGNATURE* const psignRBSHdrFlush );
    static ERR  ErrRBSInitFromRstmap( INST* pinst );

 private:
    INST            *m_pinst;
    RBSFILEHDR      *m_prbsfilehdrCurrent;

    PWSTR           m_wszRBSAbsRootDirPath;
    PWSTR           m_wszRBSBaseName;
    PWSTR           m_wszRBSCurrentFile;
    PWSTR           m_wszRBSCurrentLogDir;
    IFileAPI        *m_pfapiRBS;

    BOOL            m_fInitialized;
    BOOL            m_fInvalid;
    BOOL            m_fDumping;

    ULONG           m_cNextActiveSegment;
    ULONG           m_cNextWriteSegment;
    ULONG           m_cNextFlushSegment;
    BOOL            m_fWriteInProgress;
    CSnapshotBuffer *m_pActiveBuffer;
    CCriticalSection m_critBufferLock;
    CSnapshotBuffer *m_pBuffersToWrite;
    CSnapshotBuffer *m_pBuffersToWriteLast;
    CCriticalSection m_critWriteLock;

    CSnapshotReadBuffer *m_pReadBuffer;

    CResource       m_cresRBSBuf;

    TICK            m_tickLastFlush;

    _int64          m_ftSpaceUsageLastLogged;
    QWORD           m_cbFileSizeSpaceUsageLastRun;
    LONG            m_lGenSpaceUsageLastRun;

    ERR ErrRBSSetRequiredLogs( BOOL fInferFromRstmap );
    ERR ErrRBSCopyRequiredLogs( BOOL fInferFromRstmap );

    ERR ErrCaptureDbHeader( FMP * const pfmp );
    ERR ErrCaptureDbAttach( FMP * const pfmp );
    ERR ErrCaptureRec(
            const RBSRecord * prec,
            const DATA      * pExtraData,
                  RBS_POS   * prbsposRecord );
    ERR ErrQueueCurrentAndAllocBuffer();

    static DWORD WriteBuffers_( VOID * pvThis );
    ERR ErrWriteBuffers();
    ERR ErrFlush();

    ERR ErrRBSInvalidate();
    VOID RBSLogSpaceUsage();

    ERR ErrResetHdr( );
    VOID FreeFileApi( );
    VOID FreeHdr( );
    VOID FreePaths( );
    VOID FreeCurrentFilePath();
    VOID FreeCurrentLogDirPath();

    ERR ErrRBSCreateOrLoadRbsGen(
        long lRBSGen,
        LOGTIME tmPrevGen,
        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsFilePath,
        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsLogDirPath );

    VOID LogCorruptionEvent(
            PCWSTR wszReason,
            __int64 checksumExpected,
            __int64 checksumActual );

 public:
    BOOL FInitialized()                                     { return m_fInitialized; };
    BOOL FInvalid()                                         { return m_fInvalid; };
};

INLINE VOID CRevertSnapshot::FreeFileApi( )
{
    if ( m_pfapiRBS )
    {
        ErrUtilFlushFileBuffers( m_pfapiRBS, iofrDefensiveCloseFlush );
        delete m_pfapiRBS;
        m_pfapiRBS = NULL;
    }
}

INLINE VOID CRevertSnapshot::FreeHdr( )
{
    if ( m_prbsfilehdrCurrent )
    {
        OSMemoryPageFree( m_prbsfilehdrCurrent );
        m_prbsfilehdrCurrent = NULL;
    }
}

INLINE VOID CRevertSnapshot::FreeCurrentFilePath()
{
    if ( m_wszRBSCurrentFile )
    {
        OSMemoryHeapFree( m_wszRBSCurrentFile );
        m_wszRBSCurrentFile = NULL;
    }
}

INLINE VOID CRevertSnapshot::FreeCurrentLogDirPath()
{
    if ( m_wszRBSCurrentLogDir )
    {
        OSMemoryHeapFree( m_wszRBSCurrentLogDir );
        m_wszRBSCurrentLogDir = NULL;
    }
}

INLINE VOID CRevertSnapshot::FreePaths( )
{
    if ( m_wszRBSAbsRootDirPath )
    {
        OSMemoryHeapFree( m_wszRBSAbsRootDirPath );
        m_wszRBSAbsRootDirPath = NULL;
    }

    if ( m_wszRBSBaseName )
    {
        OSMemoryHeapFree( m_wszRBSBaseName );
        m_wszRBSBaseName = NULL;
    }

    FreeCurrentFilePath();
    FreeCurrentLogDirPath();
}

#include <pshpack1.h>

PERSISTED
struct RBSREVERTCHECKPOINT_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;
    LittleEndian<ULONG>         le_rbsrevertstate;
    LE_RBSPOS                   le_rbsposCheckpoint;

    LOGTIME                     tmCreate;
    LOGTIME                     tmCreateCurrentRBSGen;
    LOGTIME                     tmExecuteRevertBegin;

    LittleEndian<ULONG>         le_ulMajor;
    LittleEndian<ULONG>         le_ulMinor;
    
    LittleEndian<ULONG>         le_ulGrbitRevert;

    LittleEndian<QWORD>         le_cPagesReverted;

    LittleEndian<QWORD>         le_cSecInRevert;
    LittleEndian<LONG>          le_lGenMinRevertStart;
    LittleEndian<LONG>          le_lGenMaxRevertStart;

    BYTE                        rgbReserved[591];

    UnalignedLittleEndian<LONG> le_filetype;
};

C_ASSERT( offsetof( RBSREVERTCHECKPOINT_FIXED, le_ulChecksum ) == offsetof( DBFILEHDR, le_ulChecksum ) );
C_ASSERT( offsetof( RBSREVERTCHECKPOINT_FIXED, le_filetype ) == offsetof( DBFILEHDR, le_filetype ) );

PERSISTED
struct RBSREVERTCHECKPOINT
{
    RBSREVERTCHECKPOINT_FIXED           rbsrchkfilehdr;

    BYTE                                rgbPadding[cbRBSSegmentSize - sizeof(RBSREVERTCHECKPOINT_FIXED)];
};

C_ASSERT( sizeof( RBSREVERTCHECKPOINT ) == cbRBSSegmentSize );

#include <poppack.h>

class CFlushMapForUnattachedDb;
typedef INT                IRBSDBRC;
const IRBSDBRC irbsdbrcInvalid = -1;

class CRBSDatabaseRevertContext : public CZeroInit
{
private:
    INST*                       m_pinst;
    DBID                        m_dbidCurrent;
    WCHAR*                      m_wszDatabaseName;
    DBFILEHDR*                  m_pdbfilehdr;
    DBFILEHDR*                  m_pdbfilehdrFromRBS;
    IFileAPI*                   m_pfapiDb;
    CArray< CPagePointer >*     m_rgRBSDbPage;        
    IBitmapAPI*                 m_psbmDbPages;

    DBTIME                      m_dbTimePrevDirtied;
    
    CFlushMapForUnattachedDb*   m_pfm;

    CPG                         m_cpgWritePending;

    CAutoResetSignal            m_asigWritePossible;
    
    CPRINTF*                    m_pcprintfRevertTrace; 

    CPRINTF*                    m_pcprintfIRSTrace;

private:
    ERR ErrFlushDBPage( void* pvPage, PGNO pgno, USHORT cbDbPageSize, const OSFILEQOS qos );
    static INT __cdecl ICRBSDatabaseRevertContextCmpPgRec( const CPagePointer* pppg1, const CPagePointer* pppg2 );
    static void OsWriteIoComplete(
        const ERR errIo,
        IFileAPI* const pfapi,
        const FullTraceContext& tc,
        const OSFILEQOS grbitQOS,
        const QWORD ibOffset,
        const DWORD cbData,
        const BYTE* const pbData,
        const DWORD_PTR keyIOComplete );

public:
    CRBSDatabaseRevertContext( INST* pinst );
    ~CRBSDatabaseRevertContext();
    ERR ErrRBSDBRCInit( RBSATTACHINFO* prbsattachinfo, SIGNATURE* psignRBSHdrFlush, CPG cacheSize );
    ERR ErrSetDbstateForRevert( ULONG rbsrchkstate, LOGTIME logtimeRevertTo );
    ERR ErrSetDbstateAfterRevert( ULONG rbsrchkstate );
    ERR ErrRBSCaptureDbHdrFromRBS( RBSDbHdrRecord* prbsdbhdrrec, BOOL* pfGivenDbfilehdrCaptured );
    ERR ErrAddPage( void* pvPage, PGNO pgno );
    ERR ErrResetSbmDbPages();
    ERR ErrFlushDBPages( USHORT cbDbPageSize, BOOL fFlushDbHdr, CPG* pcpgReverted );
    BOOL FPageAlreadyCaptured( PGNO pgno );
    ERR ErrBeginTracingToIRS();

public:
    WCHAR*      WszDatabaseName() const     { return m_wszDatabaseName; }
    DBID        DBIDCurrent() const         { return m_dbidCurrent; }
    LONG        LgenLastConsistent() const  { return m_pdbfilehdr->le_lgposConsistent.le_lGeneration; }
    LONG        LGenMinRequired() const     { return m_pdbfilehdr->le_lGenMinRequired; }
    LONG        LGenMaxRequired() const     { return m_pdbfilehdr->le_lGenMaxRequired; }
    DBTIME      DbTimePrevDirtied() const   { return m_dbTimePrevDirtied; }
    DBFILEHDR*  PDbfilehdrFromRBS() const   { return m_pdbfilehdrFromRBS; }
    CPRINTF*    CprintfIRSTrace() const     { return m_pcprintfIRSTrace; }

public:

    VOID SetDBIDCurrent( __in DBID dbid )                   { m_dbidCurrent = dbid; }
    VOID SetDbTimePrevDirtied( DBTIME dbTimePrevDirtied )   { m_dbTimePrevDirtied = dbTimePrevDirtied; }
    VOID SetPrintfTrace( CPRINTF* pcprintfRevertTrace )     { Assert( pcprintfRevertTrace ); m_pcprintfRevertTrace = pcprintfRevertTrace; }
};


class CRBSRevertContext : public CZeroInit
{
private:    
    INST*                   m_pinst;
    PWSTR                   m_wszRBSAbsRootDirPath;
    PWSTR                   m_wszRBSBaseName;
    CPG                     m_cpgCacheMax;
    CPG                     m_cpgCached;
    USHORT                  m_cbDbPageSize;

    IRBSDBRC                m_mpdbidirbsdbrc[ dbidMax ];
    CRBSDatabaseRevertContext*               m_rgprbsdbrcAttached[ dbidMax ];
    IRBSDBRC                m_irbsdbrcMaxInUse; 

    RBSREVERTCHECKPOINT*    m_prbsrchk;
    IFileAPI*               m_pfapirbsrchk;
    CPRINTF *               m_pcprintfRevertTrace; 

    LONG                    m_lRBSMinGenToApply;
    LONG                    m_lRBSMaxGenToApply;

    _int64                  m_ftRevertLastUpdate;

    BOOL                    m_fDeleteExistingLogs;
    BOOL                    m_fRevertCancelled;
    BOOL                    m_fExecuteRevertStarted;

    LOGTIME                 m_ltRevertTo;
    QWORD                   m_cPagesRevertedCurRBSGen;

    BOOL FRBSDBRC( PCWSTR wszDatabaseName, IRBSDBRC* pirbsdbrc );

    ERR ErrRBSDBRCInitFromAttachInfo( const BYTE* pbRBSAttachInfo, SIGNATURE* psignRBSHdrFlush );
    ERR ErrComputeRBSRangeToApply( LOGTIME ltRevertExpected, LOGTIME* pltRevertActual );
    ERR ErrRBSGenApply( LONG lRBSGen, BOOL fDbHeaderOnly );
    ERR ErrApplyRBSRecord( RBSRecord* prbsrec, BOOL fCaptureDBHdrFromRBS, BOOL fDbHeaderOnly, BOOL* pfGivenDbfilehdrCaptured );

    ERR ErrRevertCheckpointInit();
    ERR ErrRevertCheckpointCleanup();
    VOID UpdateRBSGenToApplyFromCheckpoint();
    ERR ErrUpdateRevertTimeFromCheckpoint( LOGTIME *pltRevertExpected );
    ERR ErrUpdateRevertCheckpoint( ULONG revertstate, RBS_POS rbspos, LOGTIME tmCreateCurrentRBSGen, BOOL fUpdateRevertedPageCount );
    ERR ErrManageStateAfterRevert( LONG* pLgenNewMinReq, LONG* pLgenNewMaxReq );
    ERR ErrUpdateDbStatesAfterRevert();
    ERR ErrSetLogExt( PCWSTR wszRBSLogDirPath );

    ERR ErrAddPageRecord( void* pvPage, DBID dbid, PGNO pgno );
    ERR ErrFlushPages( BOOL fFlushDbHdr );
    BOOL FPageAlreadyCaptured( DBID dbid, PGNO pgno );

    ERR ErrBeginRevertTracing( bool fDeleteOldTraceFile );

    ERR ErrMakeRevertTracingNames(
        __in IFileSystemAPI* pfsapi,
        __in_range( cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW ) ULONG cbRBSRCRawPath,
        __out_bcount_z( cbRBSRCRawPath ) WCHAR* wszRBSRCRawPath,
        __in_range( cbOSFSAPI_MAX_PATHW, cbOSFSAPI_MAX_PATHW ) ULONG cbRBSRCRawBackupPath,
        __out_bcount_z( cbRBSRCRawBackupPath ) WCHAR* wszRBSRCRawBackupPath );

public:
    CRBSRevertContext( INST* pinst );
    ~CRBSRevertContext();

    ERR ErrInitContext( LOGTIME ltRevertExpected, LOGTIME* pltRevertActual, CPG cpgCache, BOOL fDeleteExistingLogs );    
    ERR ErrExecuteRevert( JET_GRBIT grbit, JET_RBSREVERTINFOMISC*  prbsrevertinfo );
    VOID CancelRevert() { m_fRevertCancelled = fTrue; }
    BOOL FExecuteRevertStarted() { return m_fExecuteRevertStarted; }

    static ERR ErrRBSRevertContextInit(
        INST* pinst,
        __in    LOGTIME     ltRevertExpected,
        __in    CPG         cpgCache,
        __in    JET_GRBIT   grbit,
        _Out_   LOGTIME*    pltRevertActual,
        _Out_   CRBSRevertContext**    pprbsrc );
};


VOID UtilLoadRBSinfomiscFromRBSfilehdr( JET_RBSINFOMISC* prbsinfomisc, const ULONG cbrbsinfomisc, const RBSFILEHDR* prbsfilehdr );
