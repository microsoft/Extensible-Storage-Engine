// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


class INST;
class FMP;
class PIB;
struct FUCB;
class CSR;
struct SPLIT;
struct SPLITPATH;
struct MERGEPATH;
struct ROOTMOVECHILD;
struct ROOTMOVE;
class LR;
class LRNODE_;
class LRCREATEMEFDP;
class LRCONVERTFDP;
class LRSPLIT_;
class LRSPLITOLD;
class LRSPLITNEW;
class LRMERGE_;
class LRMERGEOLD;
class LRMERGENEW;
class LRCHECKSUM;
class LRSCRUB;
class LRPAGEMOVE;
class LRROOTPAGEMOVE;
class LRPAGEPATCHREQUEST;
class LRMACROINFO;
class LRMACROINFO2;
class LREXTENDDB;
class LRSHRINKDB;
class LRSHRINKDB3;
class LRSCANCHECK;
class LRSCANCHECK2;
class LRTRIMDB;
class LRNEWPAGE;
class LRIGNORED;
class LRLOGBACKUP;
class ILogStream;
class LOG_READ_BUFFER;
class LOG_WRITE_BUFFER;
class LRCREATESEFDP;
class LREMPTYTREE;
class LREXTENTFREED;
template< typename TDelta > class _LRDELTA;
struct VERPROXY;

class CTableHash;
class CSessionHash;


extern CCriticalSection     g_critDBGPrint;


#if !defined(MINIMAL_FUNCTIONALITY) && !defined(ARM)
#define ENABLE_LOG_V7_RECOVERY_COMPAT
#endif


PERSISTED
struct PAGE_PATCH_TOKEN
{
    INT         cbStruct;
    DBTIME      dbtime;
    SIGNATURE   signLog;
};

C_ASSERT(48 == sizeof(PAGE_PATCH_TOKEN));

#include <pshpack1.h>

const BYTE fATCHINFOUnicodeNames            = 0x20;
const BYTE fATCHINFOSparseEnabledFile       = 0x40;

PERSISTED
class ATTACHINFO
{
    private:
        UnalignedLittleEndian< DBID >       m_dbid;
        UnalignedLittleEndian< BYTE >       m_fATCHINFOFlags;
        UnalignedLittleEndian< USHORT >     mle_cbNames;
        UnalignedLittleEndian< DBTIME >     mle_dbtime;
        UnalignedLittleEndian< OBJID >      mle_objidLast;
        UnalignedLittleEndian< CPG >        mle_cpgDatabaseSizeMax;

    public:
        LE_LGPOS                            le_lgposAttach;
        LE_LGPOS                            le_lgposConsistent;
        SIGNATURE                           signDb;

        UnalignedLittleEndian< WCHAR >      szNames[];

    public:
        INLINE DBID Dbid() const                            { return m_dbid; }
        INLINE VOID SetDbid( const DBID dbid )              { m_dbid = dbid; }

        INLINE BOOL FUnicodeNames() const                   { return !!( m_fATCHINFOFlags & fATCHINFOUnicodeNames ); }
        INLINE VOID SetFUnicodeNames()                      { m_fATCHINFOFlags = BYTE( m_fATCHINFOFlags | fATCHINFOUnicodeNames ); }

        INLINE BOOL FSparseEnabledFile() const              { return !!( m_fATCHINFOFlags & fATCHINFOSparseEnabledFile ); }
        INLINE VOID SetFSparseEnabledFile()                 { m_fATCHINFOFlags = BYTE( m_fATCHINFOFlags | fATCHINFOSparseEnabledFile ); }

        INLINE USHORT CbNames() const                       { return mle_cbNames; }
        INLINE VOID SetCbNames( const USHORT cb )           { mle_cbNames = cb; }

        INLINE DBTIME Dbtime() const                        { return mle_dbtime; }
        INLINE VOID SetDbtime( const DBTIME dbtime )        { mle_dbtime = dbtime; }

        INLINE OBJID ObjidLast() const                      { return mle_objidLast; }
        INLINE VOID SetObjidLast( const OBJID objid )       { mle_objidLast = objid; }

        INLINE CPG CpgDatabaseSizeMax() const               { return mle_cpgDatabaseSizeMax; }
        INLINE VOID SetCpgDatabaseSizeMax( const CPG cpg )  { mle_cpgDatabaseSizeMax = cpg; }
};



#define cbLogFileHeader 4096
#define cbCheckpoint    4096
#define cbAttach        2048

#define cbLGMSOverhead ( sizeof( LRMS ) + sizeof( LRTYP ) )

const BYTE  fLGReservedLog              = 0x01; 
const BYTE  fLGCircularLoggingCurrent   = 0x02; 
const BYTE  fLGCircularLoggingHistory   = 0x04; 


PERSISTED
struct LGFILEHDR_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;
    LittleEndian<LONG>          le_lGeneration;

    LittleEndian<USHORT>        le_cbSec;
    LittleEndian<USHORT>        le_csecHeader;
    LittleEndian<USHORT>        le_csecLGFile;
    LittleEndian<USHORT>        le_cbPageSize;

    LOGTIME                     tmCreate;
    LOGTIME                     tmPrevGen;

    LittleEndian<ULONG>         le_ulMajor;
    LittleEndian<ULONG>         le_ulMinor;
    LittleEndian<ULONG>         le_ulUpdateMajor;

    SIGNATURE                   signLog;

    DBMS_PARAM                  dbms_param;

    BYTE                        fLGFlags;
    LE_LGPOS                    le_lgposCheckpoint;
    LittleEndian<ULONG>         le_ulUpdateMinor;
    BYTE                        bDiskSecSizeMismatch;
    LittleEndian<XECHECKSUM>    checksumPrevLogAllSegments;
    LittleEndian<JET_ENGINEFORMATVERSION>       le_efvEngineCurrentDiagnostic;

    BYTE                        rgbReserved[2];

    UnalignedLittleEndian<ULONG>    le_filetype;    //
};


C_ASSERT( 667 == offsetof( LGFILEHDR_FIXED, le_filetype ) );

PERSISTED
struct LGFILEHDR
{
    LGFILEHDR_FIXED             lgfilehdr;
    BYTE                        rgbAttach[cbAttach];

    BYTE                        rgbPadding[cbLogFileHeader - sizeof(LGFILEHDR_FIXED) - cbAttach];
};

inline LogVersion LgvFromLgfilehdr( const LGFILEHDR * const plgfilehdr )
{
    Enforce( plgfilehdr->lgfilehdr.le_ulMinor == ulLGVersionMinorFinalDeprecatedValue ||
                plgfilehdr->lgfilehdr.le_ulMinor == ulLGVersionMinor_Win7 );
    LogVersion lgv =
    {
        plgfilehdr->lgfilehdr.le_ulMajor,
        plgfilehdr->lgfilehdr.le_ulUpdateMajor,
        plgfilehdr->lgfilehdr.le_ulUpdateMinor
    };
    return lgv;
}


C_ASSERT( 671 == offsetof( LGFILEHDR, rgbAttach ) );

PERSISTED
struct CHECKPOINT_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;
    LE_LGPOS                    le_lgposLastFullBackupCheckpoint;
    LE_LGPOS                    le_lgposCheckpoint;

    SIGNATURE                   signLog;

    DBMS_PARAM                  dbms_param;

    
    LE_LGPOS                    le_lgposFullBackup;
    LOGTIME                     logtimeFullBackup;
    LE_LGPOS                    le_lgposIncBackup;
    LOGTIME                     logtimeIncBackup;

    BYTE                        fVersion;

    LE_LGPOS                    le_lgposDbConsistency;

    BYTE                        rgbReserved1[11];

    UnalignedLittleEndian<LONG> le_filetype;


    BYTE                        rgbReserved2[8];
};

C_ASSERT( 667 == offsetof( CHECKPOINT_FIXED, le_filetype ) );

PERSISTED
struct CHECKPOINT
{
    CHECKPOINT_FIXED            checkpoint;
    BYTE                        rgbAttach[cbAttach];

    BYTE                        rgbPadding[cbCheckpoint - sizeof(CHECKPOINT_FIXED) - cbAttach];

    ERR     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
};

C_ASSERT( 679 == offsetof( CHECKPOINT, rgbAttach ) );

#define fCheckpointAttachInfoPresent            0x1

ERR ErrUtilWriteCheckpointHeaders(  const INST * const      pinst,
                                    IFileSystemAPI *const   pfsapi,
                                    const WCHAR             *wszFileName,
                                    const IOFLUSHREASON     iofr,
                                    CHECKPOINT              *pChkptHeader,
                                    IFileAPI *const         pfapi = NULL );

typedef struct tagLGSTATUSINFO
{
    ULONG           cSectorsSoFar;
    ULONG           cSectorsExpected;
    ULONG           cGensSoFar;
    ULONG           cGensExpected;
    BOOL            fCountingSectors;
    JET_PFNINITCALLBACK     pfnCallback;
    void *                  pvCallbackContext;
    JET_SNPROG              snprog;
} LGSTATUSINFO;


#include <poppack.h>




struct TEMPTRACE
{
    PIB         *ppib;
    TEMPTRACE   *pttNext;
    CHAR        szData[0];
};


enum REDOATTACH
{
    redoattachDefer = 0,
    redoattachDeferAccessDenied,
    redoattachDeferConsistentFuture,
    redoattachNow,
    redoattachCreate
};

enum eDeferredAttachReason
{
    eDARUnknown = 0,
    eDARIOError,
    eDARLogSignatureMismatch,
    eDARDbSignatureMismatch,
    eDARConsistentFuture,
    eDARAttachFuture,
    eDARHeaderCorrupt,
    eDARHeaderStateBad,
    eDARIncReseedInProgress,
    eDARUnloggedDbUpdate,
    eDARRequiredLogsMissing,
    eDARConsistentTimeMismatch,
    eDARRevertInProgress,
};

enum REDOATTACHMODE
{
    redoattachmodeAttachDbLR = 0,
    redoattachmodeCreateDbLR,
    redoattachmodeInitLR,
    redoattachmodeInitBeforeRedo
};

class ATCHCHK
{
    public:
        LGPOS       lgposAttach;
        LGPOS       lgposConsistent;
        SIGNATURE   signDb;
        SIGNATURE   signLog;

    private:
        DBTIME      m_dbtime;
        OBJID       m_objidLast;
        CPG         m_cpgDatabaseSizeMax;
        BOOL        m_fSparseEnabledFile;

    public:
        DBTIME Dbtime() const                           { return m_dbtime; }
        VOID SetDbtime( const DBTIME dbtime )           { m_dbtime = dbtime; }

        OBJID ObjidLast() const                         { return m_objidLast; }
        VOID SetObjidLast( const OBJID objid )          { m_objidLast = objid; }

        CPG CpgDatabaseSizeMax() const                  { return m_cpgDatabaseSizeMax; }
        VOID SetCpgDatabaseSizeMax( const CPG cpg )     { m_cpgDatabaseSizeMax = cpg; }

        BOOL FSparseEnabledFile() const                 { return m_fSparseEnabledFile; }
        VOID SetFSparseEnabledFile( const BOOL fValue ) { m_fSparseEnabledFile = fValue; }
};





#define fLGCreateNewGen         0x00000001
#define fLGStopOnNewGen         0x00000004
#define fLGFillPartialSector    0x00000008
#define fLGMacroGoing           0x00000010

#define fLGOldLogExists             0x00000001
#define fLGOldLogNotExists          0x00000002
#define fLGOldLogInBackup           0x00000004
#define fLGLogAttachments           0x00000008
#define fLGLogRenameOnly            0x00000010
#define fLGAssertShouldntNeedRoll   0x10000000


#define fCheckLogID             fTrue
#define fNoCheckLogID           fFalse

#define fNoProperLogFile        1
#define fRedoLogFile            2
#define fNormalClose            3

ERR ErrLGRISetupAtchchk(
        const IFMP                  ifmp,
        const SIGNATURE             *psignDb,
        const SIGNATURE             *psignLog,
        const LGPOS                 *plgposAttach,
        const LGPOS                 *plgposConsistent,
        const DBTIME                dbtime,
        const OBJID                 objidLast,
        const CPG                   cpgDatabaseSizeMax,
        const BOOL                  fSparseEnabledFile );


ERR ErrLGIReadFileHeader(
    IFileAPI * const    pfapiLog,
    const TraceContext& tc,
    OSFILEQOS           grbitQOS,
    LGFILEHDR *         plgfilehdr,
    const INST * const  pinst = NULL );

BOOL FLGVersionZeroFilled( const LGFILEHDR_FIXED* const plgfilehdr );
BOOL FLGVersionAttachInfoInCheckpoint( const LGFILEHDR_FIXED* const plgfilehdr );

VOID LGIGetDateTime( LOGTIME *plogtm );


UINT CbLGSizeOfRec( const LR * );
UINT CbLGFixedSizeOfRec( const LR * );
#ifdef DEBUG
VOID AssertLRSizesConsistent();
#endif

ERR ErrLrToLogCsvSimple( CWPRINTFFILE * pcwpfCsvOut, LGPOS lgpos, const LR *plr, LOG * plog );

BOOL FLGDebugLogRec( LR *plr );

const INT   cbRawDataMax        = 16;
const INT   cbFormattedDataMax  = ( ( ( ( cbRawDataMax * 3 ) + 1 )
                                    + (sizeof(DWORD)-1) ) / sizeof(DWORD) ) * sizeof(DWORD);

VOID ShowData(
    _In_reads_( cbData ) const BYTE *pbData,
    _In_range_( 0, ( cbFormattedDataMax - 1 ) / 3 ) INT cbData,
    UINT iVerbosityLevel,
    CPRINTF * const pcprintf );

#define LOG_REC_STRING_MAX 1024
VOID LrToSz( const LR *plr, __out_bcount(cbLR) PSTR szLR, ULONG cbLR, LOG *plog );
VOID ShowLR( const LR *plr, LOG *plog );
VOID CheckEndOfNOPList( const LR *plr, LOG *plog );

struct PageRef
{
    PageRef() {}
    PageRef( DBID dbidIn, PGNO pgnoIn, BOOL fWriteIn = fTrue, BOOL fReadIn = fTrue )
        : dbid( dbidIn ), pgno( pgnoIn ), fWrite( (BYTE)fWriteIn ), fRead( (BYTE)fReadIn )
    {
        Assert( ( ( dbidIn < dbidMax ) && ( pgno != pgnoNull ) && ( pgno < pgnoMax ) ) ||
                ( ( dbidIn == dbidMax ) && ( pgno == pgnoMax ) ) );
    }

    BOOL operator==( const PageRef& other ) const
    {
        return Cmp( this, &other ) == 0;
    }

    BOOL operator!=( const PageRef& other ) const
    {
        return Cmp( this, &other ) != 0;
    }

    BOOL operator<( const PageRef& other ) const
    {
        return Cmp( this, &other ) < 0;
    }

    static INT Cmp( const PageRef* ppageref1, const PageRef* ppageRef2 )
    {
        const INT cmp = (INT)( ppageref1->dbid - ppageRef2->dbid );
        return ( 0 != cmp ? cmp : (INT)( ppageref1->pgno - ppageRef2->pgno ) );
    }

    DBID    dbid;
    BYTE    fWrite;
    BYTE    fRead;
    PGNO    pgno;
};

ERR ErrLrToPageRef( _In_ INST* const                                                                        pinst,
                    _In_ const LR* const                                                                    plr,
                    _Inout_ ULONG* const                                                                    pcPageRef,
                    _Inout_ ULONG* const                                                                    pcPageRefAlloc,
                    _At_(*_Curr_, _Inout_updates_to_opt_( *pcPageRefAlloc, *pcPageRef )) _Inout_ PageRef**  prgPageRef );

INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 );

class LogPrereaderBase
{
protected:
    LogPrereaderBase();

public:
    virtual ~LogPrereaderBase();

protected:
    virtual ERR ErrLGPIPrereadPage( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf ) = 0;
public:
    virtual ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid ) = 0;

public:
    VOID LGPInit( const DBID dbidMaxUsed, const CPG cpgGrowth );
    VOID LGPTerm();
    VOID LGPDBEnable( const DBID dbid );
    VOID LGPDBDisable( const DBID dbid );
    BOOL FLGPEnabled() const;
    BOOL FLGPDBEnabled( const DBID dbid ) const;
    ERR ErrLGPAddPgnoRef( const DBID dbid, const PGNO pgno );
    VOID LGPSortPages();
    ERR ErrLGPPrereadExtendedPageRange( const DBID dbid, const PGNO pgno, CPG* const pcpgPreread, const BFPreReadFlags bfprf = bfprfDefault );
    size_t IpgLGPGetSorted( const DBID dbid, const PGNO pgno ) const;

protected:
    size_t CpgLGPIGetArrayPgnosSize( const DBID dbid ) const;
    size_t IpgLGPIGetUnsorted( const DBID dbid, const PGNO pgno ) const;
    ERR ErrLGPISetEntry( const DBID dbid, const size_t ipg, const PGNO pgno );
    PGNO PgnoLGPIGetEntry( const DBID dbid, const size_t ipg ) const;

private:
    static INT __cdecl ILGPICmpPgno( const PGNO* ppgno1, const PGNO* ppgno2 );

private:
    DBID m_dbidMaxUsed;
    CPG m_cpgGrowth;
    CArray<PGNO>* m_rgArrayPgnos;
};

class LogPrereaderTest : public LogPrereaderBase
{
protected:
    ERR ErrLGPIPrereadPage( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf );
public:
    ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid );

public:
    LogPrereaderTest();

public:
    size_t CpgLGPGetArrayPgnosSize( const DBID dbid ) const;
    VOID LGPAddRgPgnoRef( const DBID dbid, const PGNO* const rgpgno, const CPG cpg );
    BOOL FLGPTestPgnosPresent( const DBID dbid, const PGNO* const rgpgno, const CPG cpg );
};

class LogPrereader : public LogPrereaderBase
{
protected:
    ERR ErrLGPIPrereadPage( __in_range( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf );
public:
    ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid );

public:
    LogPrereader( INST* const pinst );

private:
    INST* m_pinst;
};

class LogPrereaderDummy : public LogPrereaderBase
{
private:
    ERR ErrLGPIPrereadPage( __in_range( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf )
    {
        Assert( fFalse );
        return JET_errSuccess;
    }
    ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid )
    {
        Assert( fFalse );
        return JET_errSuccess;
    }

public:
    LogPrereaderDummy() {}
};



struct LGPOSQueueNode
{
public:
    LGPOSQueueNode* m_plgposNext;
    LGPOS m_lgpos;

    LGPOSQueueNode( const LGPOS& lgpos )
    {
        m_lgpos = lgpos;
        m_plgposNext = NULL;
    }

private:
    LGPOSQueueNode()
    {
    }
};

enum eLGFileNameSpec { eCurrentLog,
                        eCurrentTmpLog,
                        eArchiveLog,
                        eReserveLog,
                        eShadowLog
    };


class LGInitTerm
{
private:
    LGInitTerm();
    ~LGInitTerm();

public:
    static ERR ErrLGSystemInit( void );
    static VOID LGSystemTerm( void );
};

class LGChecksum
{
private:
    LGChecksum();
    ~LGChecksum();

public:

    static ULONG32 UlChecksumBytes( const BYTE* pbMin, const BYTE* pbMax, const ULONG32 ulSeed );

#ifndef RTM
    static BOOL TestChecksumBytes();
#endif

private:
#ifndef RTM
    static ULONG32 UlChecksumBytesNaive( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed );
    static ULONG32 UlChecksumBytesSlow( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed );
    static BOOL TestChecksumBytesIteration( const BYTE * pbMin, const BYTE * pbMax, const ULONG32 ulSeed );
#endif
};

class LGFileHelper
{
private:
    LGFileHelper();
    ~LGFileHelper();

public:
    static ERR ErrLGGetGeneration( IFileSystemAPI* const pfsapi, __in PCWSTR wszFName, __in PCWSTR wszBaseName, LONG* plgen, __in PCWSTR const wszLogExt, ULONG * pcchLogDigits );
    static VOID LGSzLogIdAppend( __inout_bcount_z(cbFName) PWSTR wszLogFileName, size_t cbFName, LONG lgen, ULONG cchLogDigits = 0 );
    static ERR ErrLGMakeLogNameBaselessEx(
        __out_bcount(cbLogName) PWSTR wszLogName,
        __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) ULONG cbLogName,
        IFileSystemAPI *const       pfsapi,
        __in PCWSTR                 wszLogFolder,
        __in PCWSTR                 wszBaseName,
        const enum eLGFileNameSpec eLogType,
        LONG                    lGen,
        __in PCWSTR                 wszLogExt,
        ULONG                   cLogDigits );
};

class CHECKSUMINCREMENTAL
{
public:
    VOID    BeginChecksum( const ULONG32 ulSeed )
    {
        m_ulChecksum = ulSeed;
        m_cLeftRotate = 0;
    }
    VOID    ChecksumBytes( const BYTE* pbMin, const BYTE* pbMax );
    ULONG32 EndChecksum() const
    {
        return m_ulChecksum;
    }

private:
    ULONG32 m_ulChecksum;
    DWORD   m_cLeftRotate;
};

PERSISTED  enum LG_DISK_SEC_SIZE_MISMATCH_DEBUG
{
    bTrueDiskSizeUnknown = 0,
    bTrueDiskSizeMatches = 1,
    bTrueDiskSize512     = 2,
    bTrueDiskSize1024    = 3,
    bTrueDiskSize2048    = 4,
    bTrueDiskSize4096    = 5,
    bTrueDiskSizeDiff    = 255
};


struct RSTMAP
{
    WCHAR           *wszDatabaseName;
    WCHAR           *wszNewDatabaseName;
    JET_GRBIT       grbit;
    WCHAR           *wszGenericName;
    BOOL            fDestDBReady;
    BOOL            fFileNotFound;
    SIGNATURE       signDatabase;
    ULONG           cbDatabaseName;
    JET_SETDBPARAM  *rgsetdbparam;
    ULONG           csetdbparam;

    ULONG           dbstate;
    LONG            lGenMinRequired;
    LONG            lGenMaxRequired;
    LONG            lGenLastConsistent;
    LONG            lGenLastAttach;

    BOOL            fRBSFeatureEnabled;
    SIGNATURE       signDatabaseHdrFlush;
    SIGNATURE       signRBSHdrFlush;
};

class LGEN_LOGTIME_MAP : public CZeroInit
{
 public:
    LGEN_LOGTIME_MAP()
      : CZeroInit( sizeof( LGEN_LOGTIME_MAP ) )
    {
        Reset();
    }

    ~LGEN_LOGTIME_MAP()
    {
        Reset();
    }

    VOID Reset()
    {
        OSMemoryHeapFree( m_pLogtimeMapping );
        m_pLogtimeMapping = NULL;
        m_cLogtimeMappingAlloc = 0;
        m_lGenLogtimeMappingStart = 0;
        m_cLogtimeMappingValid = 0;
    }

    ERR ErrAddLogtimeMapping( const LONG lGen,  const LOGTIME* const pLogtime );
    VOID LookupLogtimeMapping( const LONG lGen, LOGTIME* const pLogtime );
    VOID TrimLogtimeMapping( const LONG lGen );

 private:
    LOGTIME *       m_pLogtimeMapping;
    LONG            m_cLogtimeMappingAlloc;
    LONG            m_lGenLogtimeMappingStart;
    LONG            m_cLogtimeMappingValid;
};

class LOG_BUFFER
{
    friend class LOG_READ_BUFFER;
    friend class LOG_WRITE_BUFFER;
    friend class LOG;

public:
    LOG_BUFFER();
    ERR ErrLGInitLogBuffers( INST *pinst, ILogStream *pLogStream, LONG csecExplicitRequest = 0, BOOL fReadOnly = fFalse );
    void LGTermLogBuffers();

    VOID GetLgpos( BYTE *pb, LGPOS *plgpos, ILogStream *pLogStream ) const;

    BOOL    FIsFreeSpace( const BYTE* const pb, ULONG cb ) const;
    BOOL    FIsUsedSpace( const BYTE* const pb, ULONG cb ) const;
    ERR InitCommit(LONG cBytes);
    BOOL  Commit( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    BOOL  EnsureCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    VOID  Decommit( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    BOOL  FIsCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    LONG  CommittedBufferSize();
    
#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

private:
    const BYTE* PbMaxEntry() const;
    const BYTE* PbMaxWrite() const;

    const BYTE* PbMaxPtrForUsed( const BYTE* const pb ) const;
    const BYTE* PbMaxPtrForFree( const BYTE* const pb ) const;

    ULONG   CbLGUsed() const;
    ULONG   CbLGFree() const;
    BOOL    FLGIIsFreeSpace( const BYTE* const pb, ULONG cb ) const;
    BOOL    FLGIIsUsedSpace( const BYTE* const pb, ULONG cb ) const;
    
    BOOL            _fReadOnly;


    BYTE            *_pbLGBufMin;
#define m_pbLGBufMin    m_pLogBuffer->_pbLGBufMin
    BYTE            *_pbLGBufMax;
#define m_pbLGBufMax    m_pLogBuffer->_pbLGBufMax

    ULONG           _cbLGBuf;
#define m_cbLGBuf       m_pLogBuffer->_cbLGBuf
    UINT            _csecLGBuf;
#define m_csecLGBuf     m_pLogBuffer->_csecLGBuf


    BYTE            *_pbEntry;
#define m_pbEntry       m_pLogBuffer->_pbEntry
    BYTE            *_pbWrite;
#define m_pbWrite       m_pLogBuffer->_pbWrite

    INT             _isecWrite;
#define m_isecWrite     m_pLogBuffer->_isecWrite
    LGPOS           _lgposToWrite;
#define m_lgposToWrite  m_pLogBuffer->_lgposToWrite
    LGPOS           _lgposFlushTip;
#define m_lgposFlushTip m_LogBuffer._lgposFlushTip

    LGPOS           _lgposMaxWritePoint;
#define m_lgposMaxWritePoint    m_pLogBuffer->_lgposMaxWritePoint

    BYTE            *_pbLGFileEnd;
#define m_pbLGFileEnd   m_pLogBuffer->_pbLGFileEnd
    LONG            _isecLGFileEnd;
#define m_isecLGFileEnd m_pLogBuffer->_isecLGFileEnd

    BYTE        *_pbLGCommitStart;
    BYTE        *_pbLGCommitEnd;
    CCriticalSection    _critLGBuf;
#define m_critLGBuf m_pLogBuffer->_critLGBuf
};

#define NUM_WASTAGE_SLOTS       20

class LOG
    :   public CZeroInit
{
 

    friend VOID LrToSz( const LR *plr, __out_bcount(cbLR) PSTR szLR, ULONG cbLR, LOG * plog );

#pragma push_macro( "new" )
#undef new
private:
    void* operator new[]( size_t );
    void operator delete[]( void* );

public:
    void* operator new( size_t cbAlloc )
    {
        return RESLOG.PvRESAlloc_( SzNewFile(), UlNewLine() );
    }
    void operator delete( void* pv )
    {
        RESLOG.Free( pv );
    }
#pragma pop_macro( "new" )
    
    LOG( INST *pinst );
    ~LOG();

    ERR ErrLGPreInit();

    ERR ErrLGInitSetInstanceWiseParameters( JET_GRBIT grbit );

    ERR ErrLGInit( BOOL *pfNewCheckpointFile );
    ERR ErrLGTerm( const BOOL fLogQuitRec );


    VOID LGCloseFile();
    BOOL FLGFileOpened() const;

    ERR ErrLGInitTmpLogBuffers();
    VOID LGTermTmpLogBuffers();



    BOOL FNoMoreLogWrite( ERR *perr = NULL ) const
    {
        if ( perr != NULL )
        {
            *perr = m_errNoMoreLogWrite;
        }
        return m_fLGNoMoreLogWrite;
    }

    BOOL FLogDisabled() const
    {
        return m_fLogDisabled;
    }

    BOOL FLogDisabledDueToRecoveryFailure() const
    {
        return m_fLogDisabledDueToRecoveryFailure;
    }

    BOOL FLogInitialized() const
    {
        return m_fLogInitialized;
    }

    BOOL FRecovering() const
    {
        return m_fRecovering;
    }

    RECOVERING_MODE FRecoveringMode() const
    {
        return m_fRecoveringMode;
    }

    BOOL FRecoveryUndoLogged() const
    {
        return m_fRecoveryUndoLogged;
    }

    BOOL FHardRestore() const
    {
        return m_fHardRestore;
    }

    BOOL FDumpingLogs() const
    {
        return m_fDumpingLogs;
    }

    BOOL FVerboseDump() const
    {
        return m_iDumpVerbosityLevel != LOG::ldvlBasic;
    }

    UINT IDumpVerbosityLevel() const
    {
        return m_iDumpVerbosityLevel;
    }

    SIGNATURE SignLog() const
    {
        Assert( m_fSignLogSet );
        return m_signLog;
    }

    const UINT64 QwSignLogHash() const
    {
        return Ui64FNVHash( &m_signLog, sizeof( m_signLog ) );
    }

    ERR ErrLGSetSignLog(
        SIGNATURE * psignLog,
        BOOL fNeedToCheckLogID );

    LONG LgenInitial() const
    {
        return m_lgenInitial;
    }
    VOID SetLgenInitial( LONG lGen )
    {
        m_lgenInitial = lGen;
    }

    PCWSTR SzLGCurrentFolder() const
    {
        return m_wszLogCurrent;
    }

    BOOL FNewCheckpointFile() const
    {
        return m_fNewCheckpointFile;
    }

    BOOL FRedoMapNeeded( const IFMP ifmp ) const;

    BOOL FIODuringRecovery() const
    {
        return m_fIODuringRecovery;
    }

    QWORD CbLGOffsetLgposForOB0( LGPOS lgpos1, LGPOS lgpos2 ) const;
    LGPOS LgposLGFromIbForOB0( QWORD ib ) const;

    VOID SetNoMoreLogWrite( const ERR err );
    VOID ResetNoMoreLogWrite();
    PCWSTR LogExt() const;
    ULONG CbLGSec() const;
    ULONG CSecLGFile() const;
    VOID SetCSecLGFile( ULONG csecLGFile );
    ULONG CSecLGHeader() const;


    VOID LGWriteTip( LGPOS *plgpos );
    VOID LGFlushTip( LGPOS *plgpos );
    VOID LGWriteAndFlushTip( LGPOS *plgposWrite, LGPOS *plgposFlush );
    VOID LGSetFlushTip( const LGPOS &lgpos );
    VOID LGSetFlushTipWithLock( const LGPOS &lgpos );
    VOID LGResetFlushTipWithLock( const LGPOS &lgpos );
    LGPOS LgposLGLogTipNoLock() const;
    VOID LGLogTipWithLock( LGPOS *plgpos );
    VOID LGGetLgposOfPbEntry( LGPOS *plgpos );
    BOOL FLGSignalWrite();
    ERR ErrLGWaitForWrite( PIB* const ppib, const LGPOS* const plgposLogRec );
    ERR ErrLGFlush( const IOFLUSHREASON iofr, const BOOL fDisableDeadlockDetection = fFalse );
    ERR ErrLGScheduleWrite( DWORD cmsecDurableCommit, LGPOS lgposCommit );
    ERR ErrLGStopAndEmitLog();
    BOOL FLGRolloverInDuration( TICK dtickLogRoll );
    VOID LGLockWrite();
    VOID LGUnlockWrite();




    ERR ErrLGShadowLogAddData(
        JET_EMITDATACTX *   pEmitLogDataCtx,
        void *              pvLogData,
        ULONG       cbLogData );



    ERR ErrLGCheckState();
    ERR ErrLGTruncateLog( const LONG lgenMic, const LONG lgenMac, const BOOL fSnapshot, BOOL fFullBackup );
    ERR ErrLGVerifyFileSize( QWORD qwFileSize );


    VOID LGSetSectorGeometry(
        const ULONG cbSecSize,
        const ULONG csecLGFile );

    ERR ErrLGGetGenerationRange(
            __in PCWSTR wszFindPath,
            LONG* plgenLow,
            LONG* plgenHigh,
            __in BOOL fLooseExt = fFalse,
            __out_opt BOOL * pfDefaultExt = NULL );

    ERR ErrLGWaitForLogGen( const LONG lGeneration );

    LONG LGGetCurrentFileGenWithLock( LOGTIME *ptmCreate = NULL );
    LONG LGGetCurrentFileGenNoLock( LOGTIME *ptmCreate = NULL ) const;
    BOOL FLGProbablyWriting();

    VOID LGCreateAsynchCancel( const BOOL fWaitForPending );
    ERR ErrStartAsyncLogFileCreation( _In_ PCWSTR wszPathJetTmpLog, _In_ const LONG lgenDebug );

    VOID LGAddLgpos( LGPOS * const plgpos, UINT cb ) const;
    
    VOID LGFullLogNameFromLogId(
        __out_ecount(OSFSAPI_MAX_PATH) PWSTR wszFullLogFileName,
        LONG lGeneration,
        __in PCWSTR wszDirectory );

    VOID LGMakeLogName(
        __out_bcount(cbLogName) PWSTR wszLogName,
        ULONG cbLogName,
        const enum eLGFileNameSpec eLogType,
        LONG lGen = 0 ) const;

    ERR ErrLGMakeLogNameBaseless(
        __out_bcount(cbLogName)
        PWSTR wszLogName,
        ULONG cbLogName,
        __in PCWSTR wszLogFolder,
        const enum eLGFileNameSpec eLogType,
        LONG lGen,
        __in_opt PCWSTR wszLogExt = NULL ) const;

    VOID LGVerifyFileHeaderOnInit();

    ERR ErrLGFormatFeatureEnabled( _In_ const JET_ENGINEFORMATVERSION efvFormatFeature ) const;
#ifdef DEBUG
    ERR ErrLGGetPersistedLogVersion( _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const LogVersion ** const pplgv ) const;
#endif

    ERR ErrLGCreateEmptyShadowLogFile( const WCHAR * const wszPath, const LONG lgenShadow );

    BOOL FLGFileVersionUpdateNeeded( _In_ const LogVersion& lgvDesired );


    ERR ErrLGLogRec(
        const DATA * const          rgdata,
        const ULONG                 cdata,
        const BOOL                  fLGFlags,
        const LONG                  lgenBegin0,
        LGPOS * const               plgposLogRec,
        CCriticalSection * const    pcrit = NULL );

    ERR ErrLGTryLogRec(
        const DATA  * const rgdata,
        const ULONG cdata,
        const BOOL  fLGFlags,
        const LONG  lgenBegin0,
        LGPOS       * const plgposLogRec );

    VOID LGSetLgposStart()
    {
        Assert( CmpLgpos( m_lgposStart, lgposMin ) == 0 );
        m_lgposStart = LgposLGLogTipNoLock();
    }

    ERR ErrLGTrace( PIB *ppib, __in PSTR sz );


    ERR ErrLGUpdateWaypointIFMP( IFileSystemAPI *const pfsapi, __in const IFMP ifmpTarget = ifmpNil );
    ERR ErrLGQuiesceWaypointLatencyIFMP( __in const IFMP ifmpTarget );
    LONG LLGElasticWaypointLatency() const;
    BOOL FWaypointLatencyEnabled() const;


    ERR ErrLGLoadFMPFromAttachments( BYTE *pbAttach );
    VOID LGLoadAttachmentsFromFMP(
        LGPOS lgposNext,
        _Out_bytecap_c_( cbAttach ) BYTE *pbBuf );

    ERR ErrLGSoftStart( BOOL fKeepDbAttached, BOOL fInferCheckpointFromRstmapDbs, BOOL *pfNewCheckpointFile, BOOL *pfJetLogGeneratedDuringSoftStart );

    VOID LGSetLgposRecoveryStop( LGPOS lgpos )
    {
        m_lgposRecoveryStop = lgpos;
    }

    BOOL FLastLRIsShutdown() const { return m_fLastLRIsShutdown; }
    LGPOS LgposShutDownMark() const { return m_lgposRedoShutDownMarkGlobal; }

    VOID LGRRemoveFucb( FUCB * pfucb );

    ERR ErrLGMostSignificantRecoveryWarning( void );

    ERR ErrLGLockCheckpointAndUpdateGenRequired( const LONG lGenCommitted, const LOGTIME logtimeGenMaxCreate );
    ERR ErrLGUpdateGenRequired(
        IFileSystemAPI * const  pfsapi,
        const LONG              lGenMinRequired,
        const LONG              lGenMinConsistent,
        const LONG              lGenCommitted,
        const LOGTIME           logtimeGenMaxCreate,
        BOOL * const            pfSkippedAttachDetach,
        const IFMP              ifmpTarget = ifmpNil );

    VOID LGPrereadExecute(
        LGPOS lgposPbNext
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
        ,LGPOS lgposLastChecksum
#endif
        );
    VOID LGPrereadTerm()
    {
        if ( m_pPrereadWatermarks != NULL )
        {
            while ( !m_pPrereadWatermarks->FEmpty() )
            {
                delete m_pPrereadWatermarks->RemovePrevMost( OffsetOf( LGPOSQueueNode, m_plgposNext ) );
            }
        }

        if ( m_plpreread != NULL )
        {
            m_plpreread->LGPTerm();
        }

        if ( m_plprereadSuppress != NULL )
        {
            m_plprereadSuppress->LGPTerm();
        }
    }

    ERR ErrLGNewLogFile( LONG lgen, BOOL fLGFlags );
    VOID LGReportError( const MessageId msgid, const ERR err ) const;
    VOID LGReportError( const MessageId msgid, const ERR err, IFileAPI* const pfapi ) const;

    const LogVersion LgvForReporting() const;

    const LGFILEHDR_FIXED * PlgfilehdrForVerCtrl() const;
    
    #define FmtlgverLGCurrent( plogT )      plogT->PlgfilehdrForVerCtrl()->le_ulMajor,plogT->PlgfilehdrForVerCtrl()->le_ulMinor,plogT->PlgfilehdrForVerCtrl()->le_ulUpdateMajor,plogT->PlgfilehdrForVerCtrl()->le_ulUpdateMinor



    BOOL FLGLogPaused() const;
    VOID LGSetLogPaused( BOOL fValue );
    VOID LGSignalLogPaused( BOOL fSet );
    VOID LGWaitLogPaused();


    ERR ErrLGRestore(
            __in PCWSTR wszBackup,
            __in PCWSTR wszDest,
            JET_PFNINITCALLBACK pfn,
            void * pvContext );

    VOID LGGetRestoreGens( LONG *plgenLowRestore, LONG *plgenHighRestore ) const
    {
        *plgenLowRestore = m_lGenLowRestore;
        *plgenHighRestore = m_lGenHighRestore;
    }

    ERR ErrLGRSTExternalRestore(
            __in PCWSTR                 wszCheckpointFilePath,
            __in PCWSTR                 wszNewLogPath,
            JET_RSTMAP_W                *rgjrstmap,
            INT                         cjrstmap,
            __in PCWSTR                 wszBackupLogPath,
            LONG                        lgenLow,
            LONG                        lgenHigh,
            __in PCWSTR                 wszTargetLogPath,
            LONG                        lGenHighTarget,
            JET_PFNINITCALLBACK         pfn,
            void *                      pvContext);

    BOOL FExternalRestore() const
    {
        return m_fExternalRestore;
    }

    INT IrstmapSearchNewName( _In_z_ const WCHAR *wszName, _Deref_out_opt_z_ WCHAR ** pwszRstmapDbName = NULL );
    ERR ErrBuildRstmapForSoftRecovery( const JET_RSTMAP2_W * const rgjrstmap, const INT cjrstmap );
    VOID LoadCheckpointGenerationFromRstmap( LONG * const plgenLow );
    VOID LoadHighestLgenAttachFromRstmap( LONG * const plgenAttach );
    VOID LoadRBSGenerationFromRstmap( LONG * const plgenLow, LONG * const plgenHigh );
    BOOL FRBSFeatureEnabledFromRstmap();
    VOID FreeRstmap( VOID );
    INT IrstmapMac() const
    {
        return m_irstmapMac;
    }

    RSTMAP  * Rgrstmap() const
    {
        return m_rgrstmap;
    }


    ERR ErrLGUpdateCheckpointFile( const BOOL fForceUpdate );
    LGPOS LgposLGCurrentCheckpointMayFail() ;
    LGPOS LgposLGCurrentDbConsistencyMayFail() ;
    ERR ErrLGDumpCheckpoint( __in PCWSTR wszCheckpoint );
    VOID LGFullNameCheckpoint( __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR wszFullName );
    ERR ErrLGReadCheckpoint( __in PCWSTR wszCheckpointFile, CHECKPOINT *pcheckpoint, const BOOL fReadOnly );

    VOID LGDisableCheckpoint( VOID )
    {
        m_critCheckpoint.Enter();
        m_fDisableCheckpoint = fTrue;
        m_critCheckpoint.Leave();
    }

    VOID SetCheckpointEnabled( BOOL fEnabled )
    {
        m_critCheckpoint.Enter();
        m_fLGFMPLoaded = fEnabled;
        m_critCheckpoint.Leave();
    }

    VOID SetPendingRedoMapEntries()
    {
        m_fPendingRedoMapEntries = fTrue;
    }

    VOID ResetPendingRedoMapEntries()
    {
        m_fPendingRedoMapEntries = fFalse;
    }

    VOID LockCheckpoint()
    {
        m_critCheckpoint.Enter();
    }

    VOID UnlockCheckpoint()
    {
        m_critCheckpoint.Leave();
    }

#ifdef DEBUG
    BOOL FOwnerCheckpoint() const
    {
        return m_critCheckpoint.FOwner();
    }
#endif

    LE_LGPOS LgposGetCheckpoint()
    {
        return m_pcheckpoint->checkpoint.le_lgposCheckpoint;
    }
    VOID ResetCheckpoint( SIGNATURE *psignlog = NULL );

    ULONG_PTR CbLGDesiredCheckpointDepth()
    {
        const ULONG_PTR configuredCheckpointDepth = UlParam( m_pinst, JET_paramCheckpointDepthMax );
        return configuredCheckpointDepth ? configuredCheckpointDepth + m_cbTotalWastage : 0;
    }

    VOID LGAddUsage( const ULONG cbUsage );
    VOID LGAddWastage( const ULONG cbWastage );

    VOID ResetLgenLogtimeMapping()
    {
        m_MaxRequiredMap.Reset();
    }

    VOID ResetRecoveryState()
    {
        m_lgposRecoveryStop = lgposMin;
        m_lgposRecoveryUndo = lgposMin;
        m_lgposRedo = lgposMin;
        m_lgposRedoLastTerm = lgposMin;
        m_lgposRedoLastTermChecksum = lgposMin;
        OnNonRTM( m_lgposRedoPreviousLog = lgposMin );
        m_lgposRedoShutDownMarkGlobal = lgposMin;
    }

    ERR ErrCompareLogs( PCWSTR wszLog1, PCWSTR wszLog2, BOOL fAllowSubsetIfEmpty, BOOL* pfLogsDiverged );


    void LGSetChkExts( BOOL fLegacy, BOOL fReset );
    PCWSTR WszLGGetDefaultExt( BOOL fChk );
    PCWSTR WszLGGetOtherExt( BOOL fChk );
    BOOL FLGIsDefaultExt( BOOL fChk, PCWSTR szExt );
    BOOL FLGIsLegacyExt( BOOL fChk, PCWSTR szExt );



    enum { LOGDUMP_LOGHDR_NOHDR, LOGDUMP_LOGHDR_INVALID, LOGDUMP_LOGHDR_VALID, LOGDUMP_LOGHDR_VALIDADJACENT };
    enum { ldvlBasic, ldvlStructure, ldvlData, ldvlMax };
    typedef struct tagLOGDUMP_OP
    {
        union
        {
            INT m_opts;
            struct
            {
                FLAG32  m_loghdr            :   3;
                FLAG32  m_fPrint            :   1;
                FLAG32  m_iVerbosityLevel   :   2;
                FLAG32  m_fVerifyOnly       :   1;
                FLAG32  m_fPermitPatching   :   1;
                FLAG32  m_fSummary          :   1;
            };
        };
        CWPRINTFFILE* m_pcwpfCsvOut;
    }
    LOGDUMP_OP;

    ERR ErrLGIDumpOneAttachment( const ATTACHINFO * const pattachinfo, const LOGDUMP_OP * const plogdumpOp ) const;
    ERR ErrLGIDumpAttachments( const LOGDUMP_OP * const plogdumpOp ) const;

    ERR ErrLGDumpLog( __in PCWSTR wszLog, LOGDUMP_OP * const plogdumpOp, LGFILEHDR * const plgfilehdr = NULL, XECHECKSUM *pLastSegChecksum = NULL );
    ERR ErrLGDumpLog( IFileAPI *const pfapi, LOGDUMP_OP * const plogdumpOp, LGFILEHDR * const plgfilehdr = NULL, XECHECKSUM *pLastSegChecksum = NULL, const LGPOS * const plgposCheckpoint = NULL );

    VOID    IncNOP() { m_cNOP++; }
    INT     GetNOP() const { return m_cNOP; }
    VOID    SetNOP(INT cNOP) { m_cNOP = cNOP; }

#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif


private:
    INST            *m_pinst;
    ILogStream *    m_pLogStream;
    LOG_READ_BUFFER *m_pLogReadBuffer;
    LOG_WRITE_BUFFER *m_pLogWriteBuffer;
    LOG_BUFFER      m_LogBuffer;

private:

    BOOL            m_fLogInitialized;
    BOOL            m_fLogDisabled;

    BOOL            m_fLogDisabledDueToRecoveryFailure;
    BOOL            m_fLGNoMoreLogWrite;
    ERR             m_errNoMoreLogWrite;
    LONG            m_lgenInitial;

    ERR             m_errCheckpointUpdate;
    LGPOS           m_lgposCheckpointUpdateError;


    BOOL            m_fRecovering;
    RECOVERING_MODE m_fRecoveringMode;
    BOOL            m_fHardRestore;
    BOOL            m_fRecoveryUndoLogged;


    BOOL            m_fLGFMPLoaded;

    SIGNATURE       m_signLog;
    BOOL            m_fSignLogSet;


    PCWSTR          m_wszLogCurrent;

    PCWSTR          m_wszChkExt;


    LGPOS           m_lgposStart;

    LGPOS           m_lgposRecoveryUndo;

    CCriticalSection    m_critLGTrace;




    CHECKPOINT      *m_pcheckpoint;


    CCriticalSection    m_critCheckpoint;

    FLAG32           m_fDisableCheckpoint:1;

    FLAG32           m_fPendingRedoMapEntries:1;


    ERR ErrLGIUpdateCheckpointFile( const BOOL fForceUpdate, CHECKPOINT * pcheckpointT );
    void LGISetInitialGen( __in const LONG lgenStart );
    ERR ErrLGICheckpointInit( BOOL *pfNewCheckpointFile );
    VOID LGICheckpointTerm( VOID );
    ERR ErrLGIWriteCheckpoint( __in PCWSTR wszCheckpointFile, const IOFLUSHREASON iofr, CHECKPOINT *pcheckpoint );
    ERR ErrLGIUpdateCheckpointLgposForTerm( const LGPOS& lgposCheckpoint );
    VOID LGIUpdateCheckpoint( CHECKPOINT *pcheckpoint );

#ifndef RTM
    ERR             m_errBeginUndo;
    LONG            m_lgenHighestAtEndOfRedo;
    FLAG32          m_fCurrentGenExistsAtEndOfRedo:1;
    FLAG32          m_fRedidAllLogs:1;
    LGPOS           m_lgposRedoPreviousLog;
#endif


    ULONG           m_rgWastages[ NUM_WASTAGE_SLOTS ];
    ULONG           m_iNextWastageSlot;
    ULONG           m_cbCurrentUsage;
    ULONG           m_cbCurrentWastage;
    LONG            m_cbTotalWastage;


    FLAG32          m_fTruncateLogsAfterRecovery:1;
    FLAG32          m_fIgnoreLostLogs:1;
    FLAG32          m_fLostLogs:1;
    FLAG32          m_fReplayingIgnoreMissingDB:1;
    FLAG32          m_fReplayMissingMapEntryDB:1;
    FLAG32          m_fReplayIgnoreLogRecordsBeforeMinRequiredLog :1;
#ifdef DEBUG
    FLAG32          m_fEventedLLRDatabases:1;
#endif


    LGPOS           m_lgposRedo;

    LGPOS           m_lgposPbNextPreread;
    LGPOS           m_lgposPbNextNextPreread;
#ifdef ENABLE_LOG_V7_RECOVERY_COMPAT
    LGPOS           m_lgposLastChecksumPreread;
#endif
    CSimpleQueue<LGPOSQueueNode>*   m_pPrereadWatermarks;
    BOOL                            m_fPreread;
    LogPrereader*                   m_plpreread;
    LogPrereaderDummy*              m_plprereadSuppress;
    BOOL            m_fIODuringRecovery;

    BOOL            m_fAbruptEnd;

    ERR             m_errGlobalRedoError;
    FLAG32          m_fAfterEndAllSessions:1;
    FLAG32          m_fLastLRIsShutdown:1;
    FLAG32          m_fNeedInitialDbList:1;
    LGPOS           m_lgposRedoShutDownMarkGlobal;
    LGPOS           m_lgposRedoLastTerm;
    LGPOS           m_lgposRedoLastTermChecksum;

    CTableHash      *m_pctablehash;

    CSessionHash    *m_pcsessionhash;

    BOOL            m_fUseRecoveryLogFileSize;
    LONG            m_lLogFileSizeDuringRecovery;

    LGPOS           m_lgposRecoveryStop;

    WCHAR           m_wszRestorePath[IFileSystemAPI::cchPathMax];
    WCHAR           m_wszNewDestination[IFileSystemAPI::cchPathMax];

    RSTMAP          *m_rgrstmap;
    INT             m_irstmapMac;

    BOOL            m_fNewCheckpointFile;

    LGEN_LOGTIME_MAP m_MaxRequiredMap;


    BOOL            m_fExternalRestore;

    LONG            m_lGenLowRestore;
    LONG            m_lGenHighRestore;

    WCHAR           m_wszTargetInstanceLogPath[IFileSystemAPI::cchPathMax];
    LONG            m_lGenHighTargetInstance;

    FLAG32          m_fDumpingLogs:1;
    FLAG32          m_iDumpVerbosityLevel:2;

    CCriticalSection        m_critShadowLogConsume;
    CShadowLogStream *          m_pshadlog;
    ERR ErrLGIShadowLogInit();
    ERR ErrLGIShadowLogTerm_();
    ERR ErrLGShadowLogTerm();


    TEMPTRACE       *m_pttFirst;
    TEMPTRACE       *m_pttLast;

#ifdef DEBUG
    BOOL            m_fDBGFreezeCheckpoint;
    BOOL            m_fDBGTraceRedo;
    BOOL            m_fDBGTraceBR;
    BOOL            m_fDBGNoLog;
#endif

    INT             m_cNOP;


    ERR  ErrGetLgposRedoWithCheck();

    ERR ErrLGISetQuitWithoutUndo( const ERR errBeginUndo );
    BOOL FLGILgenHighAtRedoStillHolds();
    ERR ErrLGIBeginUndoCallback_( __in const CHAR * szFile, const ULONG lLine );

    ERR ErrLGOpenPagePatchRequestCallback( const IFMP ifmp, const PGNO pgno, const DBTIME dbtime, const void * const pvPage ) const;

    ERR ErrLGEvaluateDestructiveCorrectiveLogOptions(
            __in const LONG lgenBad,
            __in const ERR errCondition );


    ERR ErrLGITrace( PIB *ppib, __in PSTR sz, BOOL fInternal );


    ERR ErrLGIUpdateGenRecovering(
        const LONG              lGenRecovering,
        __out_ecount_part( dbidMax, *pcifmpsAttached ) IFMP *rgifmpsAttached,
        __out ULONG *           pcifmpsAttached );
    
    ERR ErrLGIUpdatePatchedDbstate(
        const LONG              lGenSwitchingTo );

    BOOL FLGIUpdatableWaypoint( __in const LONG lGenCommitted, __in const IFMP ifmpTarget = ifmpNil );
    VOID LGIGetEffectiveWaypoint(
        __in    IFMP    ifmp,
        __in    LONG    lGenCommitted,
        __out   LGPOS * plgposEffWaypoint );



    ERR ErrLGIRSTInitPath(
            __in PCWSTR wszBackupPath,
            __in PCWSTR wszNewLogPath,
            __out_bcount(cbOSFSAPI_MAX_PATHW) PWSTR wszRestorePath,
            __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) const ULONG cbRestorePath,
            __out_bcount(cbOSFSAPI_MAX_PATHW) PWSTR wszLogDirPath,
            __in_range(sizeof(WCHAR), cbOSFSAPI_MAX_PATHW) const ULONG cbLogDirPath );

    VOID LGIRSTPrepareCallback(
                LGSTATUSINFO                *plgstat,
                LONG                        lgenHigh,
                LONG                        lgenLow,
                const LONG                  lgenHighStop);
    ERR ErrLGIRSTSetupCheckpoint(
        LONG lgenLow,
        LONG lgenHigh,
        __in PCWSTR wszCurCheckpoint );

    ERR ErrLGIRSTCheckSignaturesLogSequence(
                __in PCWSTR  wszRestorePath,
                __in PCWSTR  wszLogFilePath,
                INT genLow,
                INT genHigh,
                __in_opt PCWSTR  wszTargetInstanceFilePath,
                INT genHighTarget
                );

    INT IrstmapGetRstMapEntry( const WCHAR *wszName );
    ERR ErrReplaceRstMapEntryBySignature( const WCHAR *wszName, const SIGNATURE * pDbSignature );
    ERR ErrReplaceRstMapEntryByName( const WCHAR *wszName, const SIGNATURE * pDbSignature );
    ERR ErrBuildRstmapForRestore( PCWSTR wszRestorePath );
    ERR ErrBuildRstmapForExternalRestore( JET_RSTMAP_W *rgjrstmap, INT cjrstmap );
    BOOL FRstmapCheckDuplicateSignature();
    ERR ErrGetDestDatabaseName( const WCHAR *wszDatabaseName, PCWSTR wszRestorePath, PCWSTR wszNewDestination, INT *pirstmap, LGSTATUSINFO *plgstat );



    ERR ErrLGRRedo( BOOL fKeepDbAttached, CHECKPOINT *pcheckpoint, LGSTATUSINFO *plgstat );

    ERR ErrLGICheckClosedNormallyInPreviousLog(
        __in const LONG         lGenPrevious,
        __out BOOL *            pfCloseNormally
        );

    ERR ErrLGMoveToRunningState( ERR errFromErrLGRRedo, BOOL * pfJetLogGeneratedDuringSoftStart  );

#ifdef DEBUG
    void LGRITraceRedo(const LR *plr);
#else
    void LGRITraceRedo(const LR *plr) {};
#endif
    void PrintLgposReadLR ( VOID );


    ERR ErrLGRIInitSession(
            DBMS_PARAM                  *pdbms_param,
            BYTE                        *pbAttach,
            const REDOATTACHMODE        redoattachmode );

    ERR ErrLGRedoFill( LR **pplr, BOOL fLastLRIsQuit, INT *pfNSNextStep );

    ERR ErrLGRIPreSetupFMPFromAttach(
            PIB                     *ppib,
            const ATTACHINFO *      pAttachInfo );
    ERR ErrLGRISetupFMPFromAttach(
            PIB                     *ppib,
            const ATTACHINFO *      pAttachInfo,
            LGSTATUSINFO *          plgstat = NULL,
            IFMP*                   pifmp = NULL,
            INT*                    pirstmap = NULL );

    BOOL FLGRecoveryLgposStop( ) const ;
    BOOL FLGRecoveryLgposStopLogGeneration( ) const ;
    BOOL FLGRecoveryLgposStopLogRecord( const LGPOS &lgpos ) const ;

    ERR ErrLGEndAllSessionsMacro( BOOL fLogEndMacro );
    VOID LGITryCleanupAfterRedoError( ERR errRedoOrDbHdrCheck );
    ERR ErrLGICheckGenMaxRequiredAfterRedo();
    ERR ErrLGRIEndAllSessionsWithError();
    ERR ErrLGRIEndEverySession();
    ERR ErrLGRIEndAllSessions(
            const BOOL              fEndOfLog,
            const BOOL              fKeepDbAttached,
            const LE_LGPOS *        plgposRedoFrom,
            BYTE *                  pbAttach );

    ERR ErrLGCheckDatabaseGens( const DBFILEHDR_FIX * pdbfilehdr, const LONG lGenCurrent ) const;
    ERR ErrLGCheckDBGensRequired( const LONG lGenCurrent );
    ERR ErrLGCheckGenMaxRequired();

    ERR ErrLGRIRedoOperations( const LE_LGPOS *ple_lgposRedoFrom, BYTE *pbAttach, BOOL fKeepDbAttached, BOOL* const pfRcvCleanlyDetachedDbs, LGSTATUSINFO *plgstat );
    ERR ErrLGIVerifyRedoMaps();
    ERR ErrLGIVerifyRedoMapForIfmp( const IFMP ifmp );
    VOID LGITermRedoMaps();

    ERR ErrLGIAccessPage(
        PIB             *ppib,
        CSR             *pcsr,
        const IFMP      ifmp,
        const PGNO      pgno,
        const OBJID     objid,
        const BOOL      fUninitPageOk );
    
    ERR ErrLGIAccessPageCheckDbtimes(
        __in    PIB             * const ppib,
        __in    CSR             * const pcsr,
                const IFMP      ifmp,
                const PGNO      pgno,
                const OBJID     objid,
                const DBTIME    dbtimeBefore,
                const DBTIME    dbtimeAfter,
        __out   BOOL * const    pfRedoRequired );


    ERR ErrLGRIPpibFromProcid( PROCID procid, PIB **pppib );
    static BOOL FLGRICheckRedoConditionForAttachedDb(
                const BOOL          fReplayIgnoreLogRecordsBeforeMinRequiredLog,
                const DBFILEHDR *   pdbfilehdr,
                const LGPOS&        lgpos );
    BOOL FLGRICheckRedoConditionForDb(
                const DBID          dbid,
                const LGPOS&        lgpos );
    ERR ErrLGRICheckRedoCondition(
                const DBID      dbid,
                DBTIME          dbtime,
                OBJID           objidFDP,
                PIB             *ppib,
                const BOOL      fUpdateCountersOnly,
                BOOL            *pfSkip );
    ERR ErrLGRICheckRedoConditionInTrx(
                const PROCID    procid,
                const DBID      dbid,
                DBTIME          dbtime,
                OBJID           objidFDP,
                const LR        *plr,
                PIB             **pppib,
                BOOL            *pfSkip );
    BOOL FLGRICheckRedoScanCheck( const LRSCANCHECK2 * const plrscancheck, BOOL fEvaluatePrereadLogic );

private:
    VOID LGIPrereadPage(
        const DBID      dbid,
        const PGNO      pgno,
        const OBJID     objid,
        BOOL *          pfPrereadIssued,
        BOOL * const        pfPrereadFailure,
        const BFPreReadFlags    bfprf = bfprfDefault
        );
    ERR ErrLGIPrereadExecute(
        const BOOL fPgnosOnly
        );

    ERR ErrLGIPrereadPages(
        const BOOL fPgnosOnly
        );


    VOID LGIReportMissingHighLog( const LONG lGenCurrent, const IFMP ifmp ) const;
    VOID LGIReportMissingCommitedLogsButHasLossyRecoveryOption( const LONG lGenCurrent, const IFMP ifmp ) const;
    VOID LGIReportCommittedLogsLostButConsistent( const LONG lGenCurrent, const LONG lGenEffectiveCurrent, const IFMP ifmp ) const;
    VOID LGIPossiblyGenerateLogfileMissingEvent(
                const ERR err,
                const LONG lGenCurrent,
                const LONG lGenEffectiveCurrent,
                const IFMP ifmp ) const;
    VOID LGIPossiblyGenerateLogfileMissingEvents(
        const ERR errWorst,
        const LONG lGenCurrent,
        const LONG lGenEffectiveCurrent ) const;
    VOID LGReportError( const MessageId msgid, const ERR err, const WCHAR* const wszLogFile ) const;


    ERR ErrLGRIRedoOperation( LR *plr );

    ERR ErrLGRICheckRedoCreateDb(
            const IFMP                  ifmp,
            DBFILEHDR                   *pdbfilehdr,
            REDOATTACH                  *predoattach );
    ERR ErrLGRICheckRedoAttachDb(
            const IFMP                  ifmp,
            DBFILEHDR                   *pdbfilehdr,
            const SIGNATURE             *psignLogged,
            REDOATTACH                  *predoattach,
            const REDOATTACHMODE        redoattachmode );
    ERR ErrLGRICheckAttachedDb(
            const IFMP                  ifmp,
            const SIGNATURE             *psignLogged,
            REDOATTACH                  *predoattach,
            const REDOATTACHMODE        redoattachmode );
    VOID LGRIReportRequiredLogFilesMissing(
        DBFILEHDR       *pdbfilehdr,
        const WCHAR     *wszDbName );
    VOID LGRIReportDeferredAttach(
        const WCHAR                 *wszDbName,
        const eDeferredAttachReason reason,
        const BOOL                  fCreateDb,
        const ERR                   errIO );
    ERR ErrLGRICheckAttachNow(
            DBFILEHDR       *pdbfilehdr,
            const WCHAR     *wszDbName );
    ERR ErrLGRIRedoCreateDb(
        _In_ PIB                                                    *ppib,
        _In_ const IFMP                                             ifmp,
        _In_ const DBID                                             dbid,
        _In_ const JET_GRBIT                                        grbit,
        _In_ const BOOL                                             fSparseEnabledFileRedo,
        _In_opt_ SIGNATURE                                          *psignDb,
        _In_reads_opt_( csetdbparam ) const JET_SETDBPARAM* const   rgsetdbparam,
        _In_ const ULONG                                            csetdbparam );
    ERR ErrLGRIRedoAttachDb(
            const IFMP                  ifmp,
            const CPG                   cpgDatabaseSizeMax,
            const BOOL                  fSparseEnabledFile,
            const REDOATTACHMODE        redoattachmode );
    VOID LGRISetDeferredAttachment(
                const IFMP      ifmp,
                const bool      fDeferredAttachFuture,
                const bool      fDeferredForAccessDenied );

    ERR ErrLGRIRedoBackupUpdate(
            LRLOGBACKUP *   plrlb );

    ERR ErrLGRIAccessNewPage(
                PIB *               ppib,
                CSR *               pcsrNew,
                const IFMP          ifmp,
                const PGNO          pgnoNew,
                const OBJID         objid,
                const DBTIME        dbtime,
                BOOL *              pfRedoNewPage );
    ERR ErrLGRIRedoSpaceRootPage(
                PIB *               ppib,
                const LRCREATEMEFDP *plrcreatemefdp,
                BOOL                fAvail );
    ERR ErrLGRIRedoConvertFDP( PIB *ppib, const LRCONVERTFDP *plrconvertfdp );
    ERR ErrLGRIRedoInitializeSplit( PIB *ppib, const LRSPLIT_ *plrsplit, SPLITPATH *psplitPath );
    ERR ErrLGRIRedoSplitPath( PIB *ppib, const LRSPLIT_ *plrsplit, SPLITPATH **ppsplitPath );
    ERR ErrLGRIRedoInitializeMerge(
                PIB             *ppib,
                FUCB            *pfucb,
                const LRMERGE_  *plrmerge,
                MERGEPATH       *pmergePath );
    ERR ErrLGRIRedoMergeStructures(
                PIB             *ppib,
                DBTIME          dbtime,
                MERGEPATH       **ppmergePathLeaf,
                FUCB            **ppfucb );
    ERR ErrLGIRedoSplitStructures(
                PIB             *ppib,
                DBTIME          dbtime,
                SPLITPATH       **ppsplitPathLeaf,
                FUCB            **ppfucb,
                DIRFLAG         *pdirflag,
                KEYDATAFLAGS    *pkdf,
                RCEID           *prceidOper1,
                RCEID           *prceidOper2 );
    ERR ErrLGRIRedoMerge( PIB *ppib, DBTIME dbtime );
    ERR ErrLGRIRedoSplit( PIB *ppib, DBTIME dbtime );
    ERR ErrLGRIRedoMacroOperation( PIB *ppib, DBTIME dbtime );
    ERR ErrLGRIRedoNodeOperation( const LRNODE_ *plrnode, ERR *perr );
    ERR ErrLGRIRedoScrub( const LRSCRUB * const plrscrub );
    ERR ErrLGRIRedoNewPage( const LRNEWPAGE * const plrnewpage );
    ERR ErrLGRIIRedoPageMove( __in PIB * const ppib, const LRPAGEMOVE * const plrpagemove );
    ERR ErrLGRIRedoPageMove( const LRPAGEMOVE * const plrpagemove );
    ERR ErrLGIRedoRootMoveStructures( PIB* const ppib, const DBTIME dbtime, ROOTMOVE* const prm );
    ERR ErrLGIRedoRootMoveUpgradeLatches( ROOTMOVE* const prm );
    VOID LGIRedoRootMoveUpdateDbtime( ROOTMOVE* const prm );
    ERR ErrLGRIRedoRootPageMove( PIB* const ppib, const DBTIME dbtime );
    ERR ErrLGRIRedoPagePatch( const LRPAGEPATCHREQUEST * const plrpagepatchrequest );
    ERR ErrLGRIRedoScanCheck( const LRSCANCHECK2 * const plrscancheck, BOOL* const pfBadPage );
    ERR ErrLGRIRedoExtendDB( const LREXTENDDB * const plrextenddb );
    ERR ErrLGRIRedoShrinkDB( const LRSHRINKDB3 * const plrshrinkdb );
    ERR ErrLGRIRedoShrinkDBPageReset( const IFMP ifmp, const PGNO pgnoShrinkFirstReset, const PGNO pgnoShrinkLastReset, const DBTIME dbtimeShrink );
    ERR ErrLGRIRedoShrinkDBFileTruncation( const IFMP ifmp, const PGNO pgnoDbLastNew );
    ERR ErrLGRIRedoTrimDB( _In_ const LRTRIMDB * const plrtrimdb );
    ERR ErrLGIRedoFDPPage( CTableHash *pctablehash, PIB *ppib, const LRCREATESEFDP *plrcreatesefdp );
    ERR ErrLGIRedoFDPPage( CTableHash *pctablehash, PIB *ppib, const LRCREATEMEFDP *plrcreatemefdp );
    ERR ErrLGRIRedoFreeEmptyPages( FUCB * const pfucb, LREMPTYTREE * const plremptytree );
    ERR ErrLGIRedoMergePath( PIB * ppib, const LRMERGE_  * const plrmerge, _Outptr_ MERGEPATH ** ppmergePath );
    ERR ErrLGRIRedoExtentFreed( const LREXTENTFREED * const plrextentfreed );

    template< typename TDelta >
    ERR ErrLGRIRedoDelta(
                PIB * ppib,
                FUCB * pfucb,
                CSR * pcsr,
                VERPROXY verproxy,
                const _LRDELTA< TDelta > * const plrdelta,
                const DIRFLAG dirflag,
                BOOL fRedoNeeded );

#ifdef ENABLE_JET_UNIT_TEST
    friend class TestLOGCheckRedoConditionForDatabaseTests;
#endif

    friend CHECKPOINT *        PcheckpointEDBGAccessor( const LOG * const plog );
    friend const LOG_BUFFER *  PlogbufferEDBGAddrAccessor( const LOG * const plog );
    friend ILogStream *        PlogstreamEDBGAccessor( const LOG * const plog );
    friend LOG_WRITE_BUFFER *  PlogwritebufferEDBGAccessor( const LOG * const plog );

};

VOID LGSzFromLogId( INST *pinst, __out_bcount( cbFName ) PWSTR wszLogFileName, size_t cbFName, LONG lGeneration );
VOID LGMakeName( IFileSystemAPI *const pfsapi, __out_bcount(OSFSAPI_MAX_PATH*sizeof(WCHAR)) PWSTR wszName, __in PCWSTR wszPath, __in PCWSTR wszFName, __in PCWSTR wszExt );

IOREASONPRIMARY IorpLogRead( LOG * plog );

ERR ErrLGRecoveryControlCallback( INST * pinst,
                                  FMP * pfmp,
                                  const WCHAR * wszLogName,
                                  const JET_SNT sntRecCtrl,
                                  const ERR errDefault,
                                  const LONG lgen,
                                  const BOOL fCurrentLog,
                                  const ULONG eMissingNextAction,
                                  const CHAR * szFile,
                                  const LONG lLine );

ERR ErrLGDbAttachedCallback_( INST *pinst, FMP *pfmp, const CHAR *szFile, const LONG lLine );

ERR ErrLGDbDetachingCallback_( INST *pinst, FMP *pfmp, const CHAR *szFile, const LONG lLine );

#define ErrLGOpenLogCallback( pinst, wszLogName, eOpenReason, lGenNext, fCurrentLog ) \
            ErrLGRecoveryControlCallback( pinst, NULL, wszLogName, JET_sntOpenLog, JET_errSuccess, lGenNext, fCurrentLog, eOpenReason, __FILE__, __LINE__ )

#define ErrLGOpenCheckpointCallback( pinst ) \
            ErrLGRecoveryControlCallback( pinst, NULL, NULL, JET_sntOpenCheckpoint, JET_errSuccess, 0, fFalse, 0, __FILE__, __LINE__ )

#define ErrLGOpenLogMissingCallback( pinst, wszLogName, errDefault, lGenNext, fCurrentLog, eNextAction )        \
            ErrLGRecoveryControlCallback( pinst, NULL, wszLogName, JET_sntMissingLog, errDefault, lGenNext, fCurrentLog, eNextAction, __FILE__, __LINE__ )

#define ErrLGBeginUndoCallback() \
            ErrLGIBeginUndoCallback_( __FILE__, __LINE__ )

#define ErrLGNotificationEventConditionCallback( pinst, eventid ) \
            ErrLGRecoveryControlCallback( pinst, NULL, NULL, JET_sntNotificationEvent, eventid, 0, fFalse, 0, __FILE__, __LINE__ )

#define ErrLGSignalErrorConditionCallback( pinst, errCondition ) \
            ErrLGRecoveryControlCallback( pinst, NULL, NULL, JET_sntSignalErrorCondition, errCondition, 0, fFalse, 0, __FILE__, __LINE__ )

#define ErrLGDbAttachedCallback( pinst, pfmp ) \
            ErrLGDbAttachedCallback_( pinst, pfmp, __FILE__, __LINE__ )

#define ErrLGDbDetachingCallback( pinst, pfmp ) \
            ErrLGDbDetachingCallback_( pinst, pfmp, __FILE__, __LINE__ )

#define ErrLGCommitCtxCallback( pinst, pbCommitCtx, cbCommitCtx ) \
            ErrLGRecoveryControlCallback( pinst, NULL, (WCHAR *)pbCommitCtx, JET_sntCommitCtx, JET_errSuccess, cbCommitCtx, fFalse, 0, __FILE__, __LINE__ )


#if defined( USE_HAPUBLISH_API )
INLINE HaDbFailureTag LGIHaTagOfErr( const ERR err )
{
    switch ( err )
    {
        case JET_errBadLogVersion:
        case JET_errBadLogSignature:
        case JET_errLogSectorSizeMismatch:
        case JET_errLogFileSizeMismatch:
        case JET_errPageSizeMismatch:
            return HaDbFailureTagConfiguration;

        case JET_errFileInvalidType:
        case JET_errDatabase200Format:
        case JET_errDatabase400Format:
        case JET_errDatabase500Format:
            return HaDbFailureTagCorruption;

        case JET_errLogFileCorrupt:
            return HaDbFailureTagRecoveryRedoLogCorruption;

        case JET_errLogDiskFull:
        case JET_errDiskFull:
        case JET_errOutOfMemory:
        case JET_errDiskIO:
        case_AllDatabaseStorageCorruptionErrs:
            return HaDbFailureTagNoOp;

        default:
            return HaDbFailureTagRemount;
    }
}
#endif

INLINE VOID LOG::SetNoMoreLogWrite( const ERR err )
{
    Assert( err < JET_errSuccess );

    m_fLGNoMoreLogWrite = fTrue;
    if ( JET_errSuccess == m_errNoMoreLogWrite )
    {
        m_errNoMoreLogWrite = err;

        if ( err != errLogServiceStopped )
        {
            OSUHAEmitFailureTag( m_pinst, LGIHaTagOfErr( err ), L"7b2b2b94-7a8c-41c7-9284-da92d7001baa" );
        }
    }
}
INLINE VOID LOG::ResetNoMoreLogWrite()
{
    if ( m_errNoMoreLogWrite != errLogServiceStopped )
    {
        m_fLGNoMoreLogWrite = fFalse;
        m_errNoMoreLogWrite = JET_errSuccess;
    }
}

INLINE BOOL INST::FRecovering() const
{
    return m_plog->FRecovering();
}

INLINE BOOL INST::FComputeLogDisabled()
{
    Assert(m_plog);

    if ( _wcsnicmp( SzParam( this, JET_paramRecovery ), L"repair", 6 ) == 0 )
    {
        return ( SzParam( this, JET_paramRecovery )[6] != L'\0' );
    }
    return (    SzParam( this, JET_paramRecovery )[0] == L'\0' ||
                _wcsicmp ( SzParam( this, JET_paramRecovery ), wszOn ) != 0 );
}


#define case_AllLogStorageCorruptionErrs        \
    case JET_errLogFileCorrupt:                 \
    case JET_errLogCorruptDuringHardRestore:    \
    case JET_errLogCorruptDuringHardRecovery:   \
    case JET_errLogTornWriteDuringHardRestore:  \
    case JET_errFileSystemCorruption:           \
    case JET_errLogTornWriteDuringHardRecovery

INLINE BOOL FErrIsLogCorruption( ERR err )
{
    switch( err )
    {
        case_AllLogStorageCorruptionErrs:
            return fTrue;
        default:
            break;
    }
    return fFalse;
}

INLINE INST * INST::GetInstanceByName( PCWSTR wszInstanceName )
{
    Assert ( wszInstanceName );

    Assert ( INST::FOwnerCritInst() );
    if ( g_rgpinst == NULL )
    {
        return NULL;
    }

    for ( ULONG ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        INST *pinst = g_rgpinst[ ipinst ];

        if ( !pinst )
        {
            continue;
        }
        if ( !pinst->m_fJetInitialized || !pinst->m_wszInstanceName )
        {
            continue;
        }

        if ( wcscmp( pinst->m_wszInstanceName, wszInstanceName) )
        {
            continue;
        }
        return pinst;
    }

    return NULL;
}

INLINE INST * INST::GetInstanceByFullLogPath( PCWSTR wszLogPath )
{
    ERR             err                 = JET_errSuccess;
    IFileSystemAPI* pfsapi              = NULL;
    INST*           pinstFound          = NULL;
    WCHAR           rgwchFullNameExist[IFileSystemAPI::cchPathMax+1];
    WCHAR           rgwchFullNameSearch[IFileSystemAPI::cchPathMax+1];

    Assert ( INST::FOwnerCritInst() );
    if ( g_rgpinst == NULL )
    {
        return NULL;
    }


    Call( ErrOSFSCreate( g_pfsconfigGlobal, &pfsapi ) );

    Call( pfsapi->ErrPathComplete( wszLogPath, rgwchFullNameSearch ) );
    CallS( pfsapi->ErrPathFolderNorm( rgwchFullNameSearch, sizeof(rgwchFullNameSearch) ) );

    for ( ULONG ipinst = 0; ipinst < g_cpinstMax; ipinst++ )
    {
        INST *pinst = g_rgpinst[ ipinst ];

        if ( !pinst )
        {
            continue;
        }
        if ( !pinst->m_fJetInitialized )
        {
            continue;
        }

        Call( pfsapi->ErrPathComplete( SzParam( pinst, JET_paramLogFilePath ), rgwchFullNameExist ) );
        CallS( pfsapi->ErrPathFolderNorm( rgwchFullNameExist, sizeof(rgwchFullNameExist) ) );

        if ( UtilCmpFileName( rgwchFullNameExist, rgwchFullNameSearch) )
        {
            continue;
        }

        pinstFound = pinst;
        break;
    }

HandleError:
    delete pfsapi;
    return pinstFound;
}

