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
#define cbMaxRBSSizeAllowed             100LL*1024*1024*1024

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

/*  Revert snapshot file header
/**/
PERSISTED
struct RBSFILEHDR_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;          //  must be the first 4 bytes
    LittleEndian<LONG>          le_lGeneration;         //  current rbs generation.
    //  8 bytes

    //  consistency check
    LOGTIME                     tmCreate;               //  date time file creation
    LOGTIME                     tmPrevGen;              //  date time prev file creation
    //  24 bytes

    LittleEndian<ULONG>         le_ulMajor;             //  major version number - used to breaking changes
    LittleEndian<ULONG>         le_ulMinor;             //  minor version number - used for backward compat changes (no need for forward compat changes as data is not shipped between servers)
    //  32 bytes

    LittleEndian<QWORD>         le_cbLogicalFileSize;   //  Logical file size
    // TODO: SOMEONE - add a le_cbLogicalPrevFileSize?
    // 40 bytes

    SIGNATURE                   signRBSHdrFlush;        // Random signature generated at the time of the last RBS header flush. 
                                                        // We maintain two signatures like flush map, so this can be flushed independent of database header flush 
    // 68 bytes

    LittleEndian<USHORT>        le_cbDbPageSize;        //  db page size
    BYTE                        rgbReserved1[2];
    // 72 bytes

    // Range of logs copied with this snapshot
    LittleEndian<LONG>          le_lGenMinLogCopied;
    LittleEndian<LONG>          le_lGenMaxLogCopied;
    BYTE                        bLogsCopied;
    // 81 bytes

    SIGNATURE                   signPrevRBSHdrFlush;    // Signature from the last flush of previous RBS. Used to stamp on database header after we revert to a particular snapshot.
                                                        // This way, even after a revert, we can further revert back since the snapshots will be considered valid.
    // 109 bytes

    BYTE                        rgbReserved[558];
    //  667 bytes

    //  WARNING: MUST be placed at this offset
    //  for uniformity with db/checkpoint headers
    UnalignedLittleEndian<ULONG>    le_filetype;        //  file type = JET_filetypeSnapshot
    //  671 bytes
};

C_ASSERT( offsetof( RBSFILEHDR_FIXED, le_ulChecksum ) == offsetof( DBFILEHDR, le_ulChecksum ) );
C_ASSERT( offsetof( RBSFILEHDR_FIXED, le_filetype ) == offsetof( DBFILEHDR, le_filetype ) );

PERSISTED
class RBSATTACHINFO
{
    private:
        BYTE                                bPresent;               //  bPresent MUST be first byte because we check this to determine end of attachment list
        UnalignedLittleEndian<DBTIME>       mle_dbtimeDirtied;      //  DBTime for this db in this revert snapshot
        UnalignedLittleEndian<DBTIME>       mle_dbtimePrevDirtied;  //  DBTime for this db in the previous revert snapshot (to check the sequence).
        // 17 bytes

        UnalignedLittleEndian<LONG>         mle_lGenMinRequired;    //  the minimum log generation required for bringing database to consistent state before applying the snapshot.
        // 21 bytes

        UnalignedLittleEndian<LONG>         mle_lGenMaxRequired;    //  the maximum log generation required for bringing database to consistent state before applying the snapshot.
        // 25 bytes

    public:
        SIGNATURE                           signDb;                 //  (28 bytes) signature of the db to check if it is for the corresponding db (incl. creation time)
        // 53 bytes

        SIGNATURE                           signDbHdrFlush;         // Random signature generated at the time of the last DB header flush.
        // 81 bytes

        UnalignedLittleEndian< WCHAR >      wszDatabaseName[IFileSystemAPI::cchPathMax];
        // 601 bytes

        BYTE                                rgbReserved[39];
        // 640 bytes

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

    //  padding to sector boundary
    BYTE                        rgbPadding[cbRBSSegmentSize - sizeof(RBSFILEHDR_FIXED) - cbRBSAttach];

    #ifdef DEBUGGER_EXTENSION
    VOID     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
    #endif
};

C_ASSERT( sizeof( RBSFILEHDR ) == cbRBSSegmentSize );
C_ASSERT( sizeof( RBSFILEHDR::rgbAttach ) == sizeof(RBSATTACHINFO)*dbidMax );

/*  Revert snapshot segment header
/**/
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

struct RBSVersion
{
    ULONG   ulMajor;           //  major version number - used to breaking changes
    ULONG   ulMinor;           //  minor version number - used for backward compat changes (no need for forward compat changes as data is not shipped between servers)
};

#define JET_rbsfvSignPrevRbsHdrFlush RBSVersion { 1, 4 }  // RBSFormatVersion supporting SignPrevRbsHdrFlush.

INLINE QWORD IbRBSFileOffsetOfSegment( ULONG segment )      { return QWORD( segment + 1 ) << shfRBSSegmentSize; }
INLINE ULONG IsegRBSSegmentOfFileOffset( QWORD ib )         { return ULONG( ( ib >> shfRBSSegmentSize ) - 1 ); }
INLINE ULONG CsegRBSCountSegmentOfOffset( DWORD ib )        { return ib >> shfRBSSegmentSize; }
INLINE ULONG IbRBSSegmentOffsetFromFullOffset( DWORD ib )   { return ib & cbRBSSegmentSizeMask; }

#include "revertsnapshotrecords.h"

const ULONG ulRBSVersionMajor         = 1;
const ULONG ulRBSVersionMinor         = 4;

class CRevertSnapshot;

struct CSnapshotBuffer
{
#pragma push_macro( "new" )
#undef new
    void* operator new( size_t ) = delete;          //  meaningless without CResource*
    void* operator new[]( size_t ) = delete;        //  meaningless without CResource*
    void operator delete[]( void* ) = delete;       //  not supported

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
        // Just needed for operator delete
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
            AtomicDecrement( &s_cAllocatedBuffers );
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
        AtomicIncrement( &s_cAllocatedBuffers );
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
    static LONG s_cAllocatedBuffers;
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
    (*pcprintf)( FORMAT_VOID( RBSFILEHDR, this, rbsfilehdr.signPrevRBSHdrFlush, dwOffset ) );
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

#endif  //  DEBUGGER_EXTENSION

//  ================================================================
class IRBSCleanerIOOperator
//  ================================================================
//
//  Abstract class for I/O operations related to RBS cleaner
//
//-
{
public:
    virtual ~IRBSCleanerIOOperator() {}

    // calculates the free disk space on the disk of the RBS directory path
    virtual ERR ErrRBSDiskSpace( QWORD* pcbFreeForUser ) = 0;

    // computes the directory size
    virtual ERR ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize ) = 0;

    // calculates the lowest and highest revert snapshot generation in the given path.
    virtual ERR ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax, BOOL fBackupDir ) = 0;

    // Returns the directory and file path for the given revert snapshot generation.
    virtual ERR ErrRBSFilePathForGen( 
        __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, 
        LONG cbDirPath, 
        __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, 
        LONG cbFilePath, 
        LONG lRBSGen, 
        BOOL fBackupDir ) = 0;

    // Reads file create time from revert snapshot file header.
    virtual ERR ErrRBSFileHeader( PCWSTR wszRBSFilePath,  _Out_ RBSFILEHDR* prbsfilehdr ) = 0;

    // Delete the given directory and all files/folders within it and logs event.
    virtual ERR ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason) = 0;

    virtual PCWSTR WszRBSAbsRootDirPath() = 0;

    virtual ERR ErrRBSAbsRootDirPathToUse( __out_bcount( cbDirPath ) WCHAR* wszRBSAbsRootDirPath, LONG cbDirPath, BOOL fBackupDir ) = 0;
};

class RBSCleanerIOOperator : public IRBSCleanerIOOperator
{
public:
    RBSCleanerIOOperator( INST* pinst );
    virtual ~RBSCleanerIOOperator();

    ERR ErrRBSInitPaths();
    ERR ErrRBSDiskSpace( QWORD* pcbFreeForUser );
    ERR ErrGetDirSize( PCWSTR wszDirPath, _Out_ QWORD* pcbSize );
    ERR ErrRBSGetLowestAndHighestGen( LONG* plRBSGenMin, LONG* plRBSGenMax, BOOL fBackupDir );
    ERR ErrRBSFilePathForGen( 
        __out_bcount ( cbDirPath ) WCHAR* wszRBSDirPath, 
        LONG cbDirPath, 
        __out_bcount ( cbFilePath ) WCHAR* wszRBSFilePath, 
        LONG cbFilePath, 
        LONG lRBSGen, 
        BOOL fBackupDir );
    ERR ErrRBSFileHeader( PCWSTR wszRBSFilePath, _Out_ RBSFILEHDR* prbsfilehdr );
    ERR ErrRemoveFolder( PCWSTR wszDirPath, PCWSTR wszRBSRemoveReason );
    PCWSTR WszRBSAbsRootDirPath();
    ERR ErrRBSAbsRootDirPathToUse( __out_bcount( cbDirPath ) WCHAR* wszRBSAbsRootDirPath, LONG cbDirPath, BOOL fBackupDir );

private:
    INST*                           m_pinst;

    PWSTR                           m_wszRBSAbsRootDirPath;
    PWSTR                           m_wszRBSBaseName;
};

INLINE PCWSTR RBSCleanerIOOperator::WszRBSAbsRootDirPath() { return m_wszRBSAbsRootDirPath; }

//  ================================================================
class IRBSCleanerConfig
//  ================================================================
//
//  Abstract class for cleaner configuration
//
//-
{
protected:
    IRBSCleanerConfig() {}
public:
    virtual ~IRBSCleanerConfig() {}

    // the number of passes that should be performed
    virtual INT CPassesMax() = 0;

    // If clean up should run.
    virtual BOOL FEnableCleanup() = 0;

    // Low disk space at which we will start cleaning up RBS aggressively.
    virtual QWORD CbLowDiskSpaceThreshold() = 0;

    // Low disk space at which we will decide to raise failure item to disable RBS provided there is only one RBS.
    virtual QWORD CbLowDiskSpaceDisableRBSThreshold() = 0;

    // Max alloted space for revert snapshots when the disk space is low.
    virtual QWORD CbMaxSpaceForRBSWhenLowDiskSpace() = 0;

    // Time since when we need revert snapshots relative to current time.
    virtual INT CSecRBSMaxTimeSpan() = 0;
    
    // Min time between cleanup attempts of revert snapshots.
    virtual INT CSecMinCleanupIntervalTime() = 0;

    // First valid snapshot. Snapshots older than this will be removed.
    virtual LONG LFirstValidRBSGen() = 0;
};


//  ================================================================
class RBSCleanerConfig : public IRBSCleanerConfig
//  ================================================================
//
//  Configure cleaner using system parameters.
//
//-
{
private:
    INST* m_pinst;
public:
    RBSCleanerConfig( INST* pinst ) { Assert( pinst ); m_pinst = pinst; }
    virtual ~RBSCleanerConfig() {}
    INT CPassesMax() { return INT_MAX; }
    BOOL FEnableCleanup();
    QWORD CbLowDiskSpaceThreshold();
    QWORD CbLowDiskSpaceDisableRBSThreshold();
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

INLINE QWORD RBSCleanerConfig::CbLowDiskSpaceDisableRBSThreshold()
{
    // We will raise a failure item to let HA disable RBS in case of low disk space and there are no older RBS to cleanup.
    // We don't want to reuse the same low disk space threshold since we want to give rbs cleaner a chance to cleanup older RBS, 
    // so we will lower the threshold by further 25GB to disable RBS.
    return ( (QWORD) min( UlParam( m_pinst, JET_paramFlight_RBSLowDiskSpaceThresholdGb ) - 25, 0 ) ) * 1024 * 1024 * 1024;
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

//  ================================================================
class IRBSCleanerState
//  ================================================================
//
//  Abstract class for cleaner state
//
//-
{
protected:
    IRBSCleanerState() {}
public:
    virtual ~IRBSCleanerState() {}

    // Previous pass completion time
    virtual __int64 FtPrevPassCompletionTime() = 0;

    // Previous pass start time
    virtual __int64 FtPassStartTime() = 0;
    virtual VOID SetPassStartTime() = 0;

    // Number of passes finished.
    virtual INT CPassesFinished() = 0;

    // Mark prev pass completed.
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

//  ================================================================
namespace RBSCleanerFactory
//  ================================================================
//
//  Create RBSCleaner objects. This returns an initialized, but not
//  running, RBSCleaner object.
//
//-
{
    // create a new scan configured to do multiple passes of the
    // database, using the system parameters
    ERR ErrRBSCleanerCreate( INST*  pinst, _Out_ RBSCleaner ** prbscleaner);
}

//  ================================================================
class RBSCleaner : public CZeroInit
//  ================================================================
//
//
//
{
public:
    RBSCleaner( INST* pinst, IRBSCleanerIOOperator* const prbscleaneriooperator, IRBSCleanerState* const prbscleanerstate, IRBSCleanerConfig* const prbscleanerconfig );
    ~RBSCleaner();

    VOID TermCleaner( );

    ERR ErrStartCleaner( );
    VOID SetFirstValidGen( long lrbsgen );
    VOID SetFileTimeCreateCurrentRBS( __int64 ftCreate );
    BOOL FIsCleanerRunning( ) const { return ( NULL != m_threadRBSCleaner ); }

private:

    INST*                               m_pinst;

    volatile LONG                       m_lFirstValidRBSGen;
    volatile BOOL                       m_fValidRBSGenSet;

    __int64                             m_ftCreateCurrentRBS;

    unique_ptr<IRBSCleanerIOOperator>   m_prbscleaneriooperator;
    unique_ptr<IRBSCleanerState>        m_prbscleanerstate;
    unique_ptr<IRBSCleanerConfig>       m_prbscleanerconfig;

    // thread and concurrency control
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

    ERR ErrRBSCleanupBackup( QWORD* cbFreeRBSDisk, QWORD* cbTotalRBSDiskSpace );
};

INLINE VOID RBSCleaner::SetFirstValidGen( long lrbsgen )
{
    ENTERCRITICALSECTION critRBSFirstValidGen( &m_critRBSFirstValidGen );

    // Update only if passed generation is greater than already set first valid RBS gen.
    if ( lrbsgen > m_lFirstValidRBSGen )
    {
        m_lFirstValidRBSGen = lrbsgen;
        m_fValidRBSGenSet = fTrue;
    }
}

// Set by main RBS thread when we create/roll/invalidate a snapshot
//
INLINE VOID RBSCleaner::SetFileTimeCreateCurrentRBS( __int64 ftCreate )
{
    m_ftCreateCurrentRBS = ftCreate;
}

INLINE BOOL RBSCleaner::FGenValid( long lrbsgen )
{
    return lrbsgen >= m_lFirstValidRBSGen;
}

INLINE BOOL RBSCleaner::FMaxPassesReached() const
{
    return m_prbscleanerstate->CPassesFinished() >= m_prbscleanerconfig->CPassesMax();
}

const TICK dtickRBSFlushInterval = 30 * 1000;   // Time interval before force flush of snapshot is considered.

class CRevertSnapshot
{
 public:
    CRevertSnapshot( _In_ INST* const pinst );

    virtual ~CRevertSnapshot();

    ERR ErrRBSInit( BOOL fRBSCreateIfMissing, ERR createSkippedError );

    VOID Term();

    ERR ErrCapturePreimage(
            DBID dbid,
            PGNO pgno,
            _In_reads_( cbImage ) const BYTE *pbImage,
            ULONG cbImage,
            RBS_POS *prbsposRecord,
            ULONG fFlags );

    ERR ErrCaptureNewPage(
            DBID dbid,
            PGNO pgno,
            RBS_POS *prbsposRecord );

    ERR ErrCaptureEmptyPages(
            DBID dbid,
            PGNO pgnoFirst,
            CPG  cpg,
            ULONG flags );

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
    
    ERR ErrSetRBSFileApi( _In_ IFileAPI *pfapirbs );

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

    static VOID EnterDbHeaderFlush( CRevertSnapshot* prbs, _Out_ SIGNATURE* const psignRBSHdrFlush );

 private:
    PWSTR           m_wszRBSAbsRootDirPath;
    PWSTR           m_wszRBSBaseName;

    BOOL            m_fInitialized;                     // Whether or not the object is initialized.
    BOOL            m_fInvalid;                         // Whether the snapshot is marked as invalid.
    BOOL            m_fDumping;                         // Whether this is just being used to dump the snapshot
    BOOL            m_fPatching;                        // Whether this is just being used to patch the snapshot.

    ULONG           m_cNextActiveSegment;
    ULONG           m_cNextWriteSegment;
    ULONG           m_cNextFlushSegment;
    BOOL            m_fWriteInProgress;
    CSnapshotBuffer *m_pActiveBuffer;
    CCriticalSection m_critBufferLock;
    CSnapshotBuffer *m_pBuffersToWrite;
    CSnapshotBuffer *m_pBuffersToWriteLast;

    CSnapshotReadBuffer *m_pReadBuffer;

    CResource       m_cresRBSBuf;

    TICK            m_tickLastFlush;

    ERR ErrQueueCurrentAndAllocBuffer();

    static DWORD WriteBuffers_( VOID * pvThis );
    ERR ErrWriteBuffers();
    ERR ErrFlush();

    ERR ErrResetHdr( );
    VOID FreeFileApi( );
    VOID FreeHdr( );
    VOID FreePaths( );

    VOID LogCorruptionEvent(
            PCWSTR wszReason,
            __int64 checksumExpected,
            __int64 checksumActual );

protected:
    INST            *m_pinst;
    RBSFILEHDR      *m_prbsfilehdrCurrent;

    PWSTR           m_wszRBSCurrentFile;                // Path to the current RBS file.
    PWSTR           m_wszRBSCurrentLogDir;              // Path to the current RBS Log dir.
    IFileAPI        *m_pfapiRBS;                        // file handle for read/write the file

    CCriticalSection m_critWriteLock;

    ERR ErrRBSCreateOrLoadRbsGen(
        long lRBSGen,
        LOGTIME tmPrevGen,
        _In_ const SIGNATURE signPrevRBSHdrFlush,
        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsFilePath,
        _Out_bytecap_c_(cbOSFSAPI_MAX_PATHW) PWSTR wszRBSAbsLogDirPath );

    ERR ErrCaptureRec(
        const RBSRecord * prec,
        const DATA      * pExtraData,
        RBS_POS   * prbsposRecord );
    ERR ErrCaptureDbAttach( WCHAR* wszDatabaseName, const DBID dbid );

    ERR ErrRBSInvalidate();

    VOID FreeCurrentFilePath();
    VOID FreeCurrentLogDirPath();

    virtual VOID RBSCheckSpaceUsage() {}

 public:
    BOOL FInitialized() const                               { return m_fInitialized; };
    BOOL FInvalid() const                                   { return m_fInvalid; };
    BOOL FPatching() const                                  { return m_fPatching; };

    VOID SetPatching()                                      { m_fPatching = true; }
};

INLINE VOID CRevertSnapshot::FreeFileApi( )
{
    if ( m_pfapiRBS )
    {
        // TODO SOMEONE: If flush fails do we ignore free'ing or is this best effort?
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

// Used when databases are attached and preimages are captured.
//
class CRevertSnapshotForAttachedDbs : public CRevertSnapshot
{
private:
    QWORD           m_cbFileSizeSpaceUsageLastRun;      // The logical file size during our last run of logging the space usage.
    LONG            m_lGenSpaceUsageLastRun;            // What was the current log generation during the last run of logging the space usage.
    _int64          m_ftSpaceUsageLastLogged;           // Last time we logged the space usage info.

    ERR ErrRBSSetRequiredLogs( BOOL fInferFromRstmap );
    ERR ErrRBSCopyRequiredLogs( BOOL fInferFromRstmap );

    ERR ErrCaptureDbHeader( FMP * const pfmp );

    ERR ErrRBSInvalidateFmps();

    BOOL FRBSRaiseFailureItemIfNeeded();
    VOID RBSCheckSpaceUsage();

public:
    CRevertSnapshotForAttachedDbs( _In_ INST* const pinst );
    ~CRevertSnapshotForAttachedDbs() {}

    ERR ErrRBSRecordDbAttach( _In_ FMP* const pfmp );
    ERR ErrRBSInitDBFromRstmap( _In_ const RSTMAP* const prstmap, LONG lgenLow, LONG lgenHigh );

    DBTIME  GetDbtimeForFmp( FMP *pfmp );
    ERR     ErrSetDbtimeForFmp( FMP *pfmp, DBTIME dbtime );

    BOOL    FRollSnapshot();
    ERR     ErrRollSnapshot( BOOL fPrevRBSValid, BOOL fInferFromRstmap );

    static ERR ErrRBSInitFromRstmap( INST* const pinst );
};

// Used when preimages are captured as part of patching pages (via incremenetal reseed api)
//
class CRevertSnapshotForPatch : public CRevertSnapshot
{
private:
    DBID m_dbidPatchingMaxInUse;

public:
    CRevertSnapshotForPatch( _In_ INST* const pinst );
    ~CRevertSnapshotForPatch() {}

    ERR ErrRBSInitDB( CIrsOpContext* const pirs );
    ERR ErrCaptureFakeDbAttach( CIrsOpContext* const pirs );
    ERR ErrRBSInvalidateIrs( PCWSTR wszReason );

    static ERR ErrRBSInitForPatch( INST* const pinst );
};

#include <pshpack1.h>

PERSISTED
struct RBSREVERTCHECKPOINT_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;
    LittleEndian<ULONG>         le_rbsrevertstate;      //  RBS revert state.
    LE_RBSPOS                   le_rbsposCheckpoint;    //  Last rbs position applied. Today we only set this afer we have completed applying an RBS gen.
                                                        //  Using RBSPOS to keep it extensible for future. If we decide to save the exact position we flushed, we would require to do a few changes to checkpoint. 
                                                        //      1. We would need to store bitmap of the pages flushed for a paritcular RBS generation per database.
                                                        //      2. When applying the lowest gen we would need to store if we have already capture database header per database.
                                                        //      3. We should save the database header state from database header we captured in (2.). 
                                                        //         We would stamp that header state on the respective database header once we have completed the revert successfully.
    //  16 bytes

    LOGTIME                     tmCreate;               //  Checkpoint file create time
    LOGTIME                     tmCreateCurrentRBSGen;  //  Create time of the rbs gen as pointed by le_rbsposCheckpoint.
    LOGTIME                     tmExecuteRevertBegin;   //  Time we first started executing the revert operation.
    //  40 bytes

    LittleEndian<ULONG>         le_ulMajor;             //  major version number - used to breaking changes
    LittleEndian<ULONG>         le_ulMinor;             //  minor version number - used for backward compat changes (no need for forward compat changes as data is not shipped between servers)
    //  48 bytes
    
    LittleEndian<ULONG>         le_ulGrbitRevert;       //  Flags being used for the revert.
    //  52 bytes

    LittleEndian<QWORD>         le_cPagesReverted;      //  Count of total pages we have reverted. Updated only when the generation in rbspos gets updated.
    //  60 bytes

    LittleEndian<QWORD>         le_cSecInRevert;        //  Total seconds spent in the revert. We will use this for reporting.
    LittleEndian<LONG>          le_lGenMinRevertStart;  //  Min log generation required across the databases at start of revert. We will use this for reporting.
    LittleEndian<LONG>          le_lGenMaxRevertStart;  //  Max log generation required across the databases at start of revert. We will use this for reporting.
    //  76 bytes

    BYTE                        rgbReserved[591];
    //  667 bytes

    //  WARNING: MUST be placed at this offset for
    //  uniformity with db/log headers
    UnalignedLittleEndian<LONG> le_filetype;            //  file type = JET_filetypeRBSRevertCheckpoint
    //  671 bytes
};

C_ASSERT( offsetof( RBSREVERTCHECKPOINT_FIXED, le_ulChecksum ) == offsetof( DBFILEHDR, le_ulChecksum ) );
C_ASSERT( offsetof( RBSREVERTCHECKPOINT_FIXED, le_filetype ) == offsetof( DBFILEHDR, le_filetype ) );

PERSISTED
struct RBSREVERTCHECKPOINT
{
    RBSREVERTCHECKPOINT_FIXED           rbsrchkfilehdr;

    //  padding to sector boundary
    BYTE                                rgbPadding[cbRBSSegmentSize - sizeof(RBSREVERTCHECKPOINT_FIXED)];
};

C_ASSERT( sizeof( RBSREVERTCHECKPOINT ) == cbRBSSegmentSize );

#include <poppack.h>

class CFlushMapForUnattachedDb;
typedef INT                IRBSDBRC;
const IRBSDBRC irbsdbrcInvalid = -1;

// Revert snapshot revert context for particular database
class CRBSDatabaseRevertContext : public CZeroInit
{
private:
    INST*                       m_pinst;
    DBID                        m_dbidCurrent;
    WCHAR*                      m_wszDatabaseName;
    DBFILEHDR*                  m_pdbfilehdr;
    DBFILEHDR*                  m_pdbfilehdrFromRBS;                // Database file header from the lowest RBS generation we apply which needs to be stamped on the database at the end of the revert.
                                                                    // Note: Each RBS might have multiple database header records. We capture the first one. So basically going backwards this is the last database header captured.
                                                                    //       Currently it utilizes the fact that RBS gen is applied in the reverse order and if we fail while applying the RBS gen, we start applying that RBS gen all over.
                                                                    //       Store it in checkpoint if we are changing the apply logic.

    ULONG                       m_currentDbHeaderState;             // Current db header state before revert started. This will only be used if we don't get a valid db header from snapshot.

    IFileAPI*                   m_pfapiDb;
    CArray< CPagePointer >*     m_rgRBSDbPage;        
    IBitmapAPI*                 m_psbmDbPages;                      // Sparse bit map indicating whether a given page has been captured.
    IBitmapAPI*                 m_psbmCachedDbPages;                // Sparse bit map indicating whether a given page is currently in our page cache.

    DBTIME                      m_dbTimePrevDirtied;                // Returns previous db time dirtied set for this database in the RBS gen we processed earlier. Used to check for consistency. 
                                                                    // If this doesn't match with dbtime of the database from previous RBS gen, any previous RBS with this database attached should be invalid.
    
    CFlushMapForUnattachedDb*   m_pfm;

    CPG                         m_cpgWritePending;                  // Number of writes pending. We coalesce pages and issue write for them together. We will wait for write pending to be 0 before we continue writing.
                                                                    // We will just let one outstanding I/O to be there.

    CAutoResetSignal            m_asigWritePossible;                // Signal to indicate whether a write is possible. Will be set each time a write completes and reset every time we hit disk tilt error.
    
    CPRINTF*                    m_pcprintfRevertTrace; 

    CPRINTF*                    m_pcprintfIRSTrace;                 // We will log the begin and complete time of the revert alone to the IRS file to have important details in a single place.

    BYTE *                      m_pbDiskPageRead;                   // For table root page, we might have to read the disk image to see if has been marked for delete. We will allocate temp buffer and keep reusing that.

private:
    ERR ErrFlushDBPage( void* pvPage, PGNO pgno, USHORT cbDbPageSize, const OSFILEQOS qos );
    ERR ErrDBDiskPageFDPRootDelete( void* pvPage, PGNO pgno, BOOL fCheckPageFDPRootDelete, BOOL fSetExistingPageFDPRootDelete, USHORT cbDbPageSize, BOOL* pfPgnoFDPRootDelete );    
    static INT __cdecl ICRBSDatabaseRevertContextCmpPgRec( const CPagePointer* pppg1, const CPagePointer* pppg2 );
    static INT __cdecl ICRBSDatabaseRevertContextPgEquals( const CPagePointer* pppg1, const CPagePointer* pppg2 );
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
    ERR ErrRBSDBRCInit( RBSATTACHINFO* prbsattachinfo, SIGNATURE* psignRBSHdrFlush, CPG cacheSize, BOOL fSkipHdrFlushCheck );
    ERR ErrSetDbstateForRevert( ULONG rbsrchkstate, LOGTIME logtimeRevertTo, SIGNATURE* psignRBSHdrFlushMaxGen );
    ERR ErrSetDbstateAfterRevert( SIGNATURE* psignRbsHdrFlush );
    ERR ErrRBSCaptureDbHdrFromRBS( RBSDbHdrRecord* prbsdbhdrrec, BOOL* pfGivenDbfilehdrCaptured );
    ERR ErrAddPage( void* pvPage, PGNO pgno, BOOL fReplaceCached, BOOL fCheckPageFDPRootDelete, BOOL fSetExistingPageFDPRootDelete, USHORT cbDbPageSize, BOOL* pfPageAddedToCache );
    ERR ErrResetSbmPages( IBitmapAPI** ppsbm );
    ERR ErrFlushDBPages( USHORT cbDbPageSize, BOOL fFlushDbHdr, CPG* pcpgReverted );
    BOOL FPageAlreadyCaptured( PGNO pgno );
    ERR ErrBeginTracingToIRS();

    // =====================================================================
    // Member retrieval..
public:
    WCHAR*          WszDatabaseName() const     { return m_wszDatabaseName; }
    DBID            DBIDCurrent() const         { return m_dbidCurrent; }
    LONG            LgenLastConsistent() const  { return m_pdbfilehdr->le_lgposConsistent.le_lGeneration; }
    LONG            LGenMinRequired() const     { return m_pdbfilehdr->le_lGenMinRequired; }
    LONG            LGenMaxRequired() const     { return m_pdbfilehdr->le_lGenMaxRequired; }
    DBTIME          DbTimePrevDirtied() const   { return m_dbTimePrevDirtied; }
    DBFILEHDR*      PDbfilehdrFromRBS() const   { return m_pdbfilehdrFromRBS; }
    CPRINTF*        CprintfIRSTrace() const     { return m_pcprintfIRSTrace; }
    IBitmapAPI**    PpsbmDbPages()              { return &m_psbmDbPages;  }

    // =====================================================================
    // Member manipulation.
public:

    VOID SetDBIDCurrent( _In_ DBID dbid )                   { m_dbidCurrent = dbid; }
    VOID SetDbTimePrevDirtied( DBTIME dbTimePrevDirtied )   { m_dbTimePrevDirtied = dbTimePrevDirtied; }
    VOID SetPrintfTrace( CPRINTF* pcprintfRevertTrace )     { Assert( pcprintfRevertTrace ); m_pcprintfRevertTrace = pcprintfRevertTrace; }
};

// =====================================================================

// Revert snapshot revert context
class CRBSRevertContext : public CZeroInit
{
private:    
    INST*                   m_pinst;
    PWSTR                   m_wszRBSAbsRootDirPath;
    PWSTR                   m_wszRBSBaseName;
    CPG                     m_cpgCacheMax;                      //  Max number of pages we will cache before flushing it to disk. Using JET_paramCacheSizeMax
    CPG                     m_cpgCached;                        //  Number of pages we have currently cache ( sum of the cached pages in all databases' revert context )
    USHORT                  m_cbDbPageSize;                     //  db page size

    IRBSDBRC                m_mpdbidirbsdbrc[ dbidMax ];
    CRBSDatabaseRevertContext*               m_rgprbsdbrcAttached[ dbidMax ];
    IRBSDBRC                m_irbsdbrcMaxInUse; 

    RBSREVERTCHECKPOINT*    m_prbsrchk;                         // Pointer to the rbs revert checkpoint
    IFileAPI*               m_pfapirbsrchk;                     // File pointer to the revert checkpoint file.
    CPRINTF *               m_pcprintfRevertTrace; 

    LONG                    m_lRBSMinGenToApply;                // Min RBS generation we are going to process and apply to the databases
    LONG                    m_lRBSCurrentGenToApply;            // Current RBS generation we are going to process and apply to the databases
    LONG                    m_lRBSMaxGenToApply;                // Min RBS generation we are going to process and apply to the databases

    _int64                  m_ftRevertLastUpdate;               // Last update time used to compute time we spend in revert.

    BOOL                    m_fDeleteExistingLogs;
    BOOL                    m_fRevertCancelled;                 // Whether the ongoing revert has been cancelled.
    BOOL                    m_fExecuteRevertStarted;            // Whether execution of the revert has begun.

    LOGTIME                 m_ltRevertTo;                       // The time we are reverting the database files to.
    QWORD                   m_cPagesRevertedCurRBSGen;          // Number of pages reverted as part of current RBS gen.

    SIGNATURE               m_signRBSHdrFlushMaxGen;            // RBS header flush of the max gen being applied. We will use this to set it on db header if db header has it not already set.
                                                                // Usually db header should have it set unless we crashed before the first flush to db header after the snapshot was created.
                                                                // When we start the revert (setting dbhdr state), dbhdr flush will mismatch. So we need to make sure RBSHdr flush signature still matches.

    BOOL FRBSDBRC( PCWSTR wszDatabaseName, IRBSDBRC* pirbsdbrc );
    BOOL FAllDbHeaderCaptured();

    ERR ErrRBSDBRCInitFromAttachInfo( const BYTE* pbRBSAttachInfo, SIGNATURE* psignRBSHdrFlush );
    ERR ErrComputeRBSRangeToApply( PCWSTR wszRBSAbsRootDirPath, LOGTIME ltRevertExpected, LOGTIME* pltRevertActual );
    ERR ErrRBSGenApply( LONG lRBSGen, RBSFILEHDR* prbsfilehdr, BOOL fDbHeaderOnly, BOOL fUseBackupDir );
    ERR ErrApplyRBSRecord( RBSRecord* prbsrec, BOOL fCaptureDBHdrFromRBS, BOOL fDbHeaderOnly, BOOL* pfGivenDbfilehdrCaptured );
    ERR ErrCheckApplyRBSContinuation();
    ERR ErrAddRevertedNewPage( DBID dbid, PGNO pgnoRevertNew, const BOOL fPageFDPNonRevertableDelete );

    ERR ErrRevertCheckpointInit();
    ERR ErrRevertCheckpointCleanup();
    VOID UpdateRBSGenToApplyFromCheckpoint();
    ERR ErrUpdateRevertTimeFromCheckpoint( LOGTIME *pltRevertExpected );
    ERR ErrUpdateRevertCheckpoint( ULONG revertstate, RBS_POS rbspos, LOGTIME tmCreateCurrentRBSGen, BOOL fUpdateRevertedPageCount );
    ERR ErrManageStateAfterRevert( LONG* pLgenNewMinReq, LONG* pLgenNewMaxReq );
    ERR ErrCopyRequiredLogsAfterRevert( LONG lgenMinToCopy, LONG lgenMaxToCopy );
    ERR ErrBackupRBSAfterRevert();
    ERR ErrRemoveRBSAfterRevert();
    ERR ErrUpdateDbStatesAfterRevert( SIGNATURE* psignRbsHdrFlush );
    ERR ErrSetLogExt( PCWSTR wszRBSLogDirPath );

    ERR ErrAddPageRecord( void* pvPage, DBID dbid, PGNO pgno, BOOL fReplaceCached, BOOL fCheckPageFDPRootDelete, BOOL fSetExistingPageFDPRootDelete, USHORT cbDbPageSize );
    ERR ErrFlushPages( BOOL fFlushDbHdr );
    BOOL FPageAlreadyCaptured( DBID dbid, PGNO pgno );

    ERR ErrBeginRevertTracing( bool fDeleteOldTraceFile );

    ERR ErrMakeRevertTracingNames(
        _In_ IFileSystemAPI* pfsapi,
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
        _In_    LOGTIME     ltRevertExpected,
        _In_    CPG         cpgCache,
        _In_    JET_GRBIT   grbit,
        _Out_   LOGTIME*    pltRevertActual,
        _Out_   CRBSRevertContext**    pprbsrc );
};

// =====================================================================

VOID UtilLoadRBSinfomiscFromRBSfilehdr( JET_RBSINFOMISC* prbsinfomisc, const ULONG cbrbsinfomisc, const RBSFILEHDR* prbsfilehdr );
VOID RBSResourcesCleanUpFromInst( _In_ INST * const pinst );
ERR ErrRBSRDWLatchAndCapturePreImage( _In_ const IFMP ifmp, _In_ const PGNO pgno, _In_ const DBTIME dbtimeLast, ULONG fPreImageFlags, _In_ const BFPriority bfpri, _In_ const TraceContext& tc );