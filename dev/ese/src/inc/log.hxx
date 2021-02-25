// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Forward delcaration

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

//  Cross instance global variables

extern CCriticalSection     g_critDBGPrint;


// Only build v7 recovery compat for full functionality
#if !defined(MINIMAL_FUNCTIONALITY) && !defined(ARM)
#define ENABLE_LOG_V7_RECOVERY_COMPAT
#endif

//------ types ----------------------------------------------------------

// This structure isn't actually persisted but it is passed from one process to
// another, so it should have the same layout on 32-bit/64-bit flavors and changing
// the structure does create upgrade incompatabilities.
PERSISTED
struct PAGE_PATCH_TOKEN
{
    INT         cbStruct;       // size of this structure
    DBTIME      dbtime;         // dbtime when the page was requested
    SIGNATURE   signLog;        // signature of the log containing the request
};

C_ASSERT(48 == sizeof(PAGE_PATCH_TOKEN));

#include <pshpack1.h>

const BYTE fATCHINFOUnicodeNames            = 0x20;
const BYTE fATCHINFOSparseEnabledFile       = 0x40;

PERSISTED
class ATTACHINFO
{
    private:
        UnalignedLittleEndian< DBID >       m_dbid;             // dbid MUST be first byte because we check this to determine end of attachment list
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


//  UNDONE: allow larger attach sizes to support greater number of
//          attached databases.

#define cbLogFileHeader 4096            // big enough to hold cbAttach
#define cbCheckpoint    4096            // big enough to hold cbAttach
#define cbAttach        2048

#define cbLGMSOverhead ( sizeof( LRMS ) + sizeof( LRTYP ) )

const BYTE  fLGReservedLog              = 0x01; /* Is using one of the reserved log? */
const BYTE  fLGCircularLoggingCurrent   = 0x02; /* Is Circular Logging on? */
const BYTE  fLGCircularLoggingHistory   = 0x04; /* Was Circular Logging on? */

/*  log file header
/**/
PERSISTED
struct LGFILEHDR_FIXED
{
    LittleEndian<ULONG>         le_ulChecksum;          //  must be the first 4 bytes
    LittleEndian<LONG>          le_lGeneration;         //  current log generation.
//  8 bytes

    LittleEndian<USHORT>        le_cbSec;               //  bytes per sector
    LittleEndian<USHORT>        le_csecHeader;          //  log header size
    LittleEndian<USHORT>        le_csecLGFile;          //  log file size.
    LittleEndian<USHORT>        le_cbPageSize;          //  db page size (0 == 4096 bytes)
//  16 bytes

    //  log consistency check
    LOGTIME                     tmCreate;               //  date time log file creation
    LOGTIME                     tmPrevGen;              //  date time prev log file creation
//  32 bytes

    LittleEndian<ULONG>         le_ulMajor;             //  major version number
    LittleEndian<ULONG>         le_ulMinor;             //  minor version number
    LittleEndian<ULONG>         le_ulUpdateMajor;        // update major version number
                                                        //  An engine will recognise log
                                                        //  formats with the same major/minor
                                                        //  version# and update version# less
                                                        //  than or equal to the engine's
                                                        //  update version#
//  44 bytes

    SIGNATURE                   signLog;                //  log signature
//  72 bytes

    //  run-time evironment
    DBMS_PARAM                  dbms_param;
//  639 bytes

    BYTE                        fLGFlags;
    LE_LGPOS                    le_lgposCheckpoint;     //  checkpoint at the time the log was created
    LittleEndian<ULONG>         le_ulUpdateMinor;       //  update minor version number when a forward
                                                        //  compatible change is taken. for example,
                                                        //  when info-only information is added to the
                                                        //  the end of a variable length Record.    
    BYTE                        bDiskSecSizeMismatch;   //  an enum giving the true sector size of the
                                                        //  disk, that we're not using.
    LittleEndian<XECHECKSUM>    checksumPrevLogAllSegments;
    LittleEndian<JET_ENGINEFORMATVERSION>       le_efvEngineCurrentDiagnostic;  //  Current version of engine binary that generated this log file.
                                                            //      NOTE: This is NOT necessarily the log format ESE should maintain if JET_paramEnableFormatVersion is set.

    BYTE                        rgbReserved[2];
//  667 bytes

    //  WARNING: MUST be placed at this offset
    //  for uniformity with db/checkpoint headers
    UnalignedLittleEndian<ULONG>    le_filetype;    //  //  file type = JET_filetypeLog
//  671 bytes
};


C_ASSERT( 667 == offsetof( LGFILEHDR_FIXED, le_filetype ) );

PERSISTED
struct LGFILEHDR
{
    LGFILEHDR_FIXED             lgfilehdr;
    BYTE                        rgbAttach[cbAttach];

    //  padding to m_plog->m_cbSec boundary
    BYTE                        rgbPadding[cbLogFileHeader - sizeof(LGFILEHDR_FIXED) - cbAttach];
};

//  Should only be used on known non-legacy logs ...
inline LogVersion LgvFromLgfilehdr( const LGFILEHDR * const plgfilehdr )
{
    //  I used the constant ulLGVersionMinor_Win7 / 3704 because it was convienent, but this actually
    //  covers back to "sorted tagged columns [1999/06/24]", which would mean all the way through WinXP.
    //  Note also the reason we can go back to 3704 and still drop the minor version is because conviently
    //  the last time we took a minor change, we ALSO took a major ver change, so semantically they can
    //  be treated as the same.
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
    LE_LGPOS                    le_lgposLastFullBackupCheckpoint;   // checkpoint of last full backup
    LE_LGPOS                    le_lgposCheckpoint;
//  20 bytes

    SIGNATURE                   signLog;                //  log gene
//  48 bytes

    DBMS_PARAM                  dbms_param;
//  615 bytes

    /*  debug fields
    /**/
    LE_LGPOS                    le_lgposFullBackup;
    LOGTIME                     logtimeFullBackup;
    LE_LGPOS                    le_lgposIncBackup;
    LOGTIME                     logtimeIncBackup;
//  647 bytes

    BYTE                        fVersion;
//  648 bytes

    LE_LGPOS                    le_lgposDbConsistency;
//  656 bytes

    BYTE                        rgbReserved1[11];
//  667 bytes

    //  WARNING: MUST be placed at this offset for
    //  uniformity with db/log headers
    UnalignedLittleEndian<LONG> le_filetype;            //  file type = JET_filetypeCheckpoint
//  671 bytes


    BYTE                        rgbReserved2[8];
//  679 bytes
};

C_ASSERT( 667 == offsetof( CHECKPOINT_FIXED, le_filetype ) );

PERSISTED
struct CHECKPOINT
{
    CHECKPOINT_FIXED            checkpoint;
    BYTE                        rgbAttach[cbAttach];

    //  padding to cbSec boundary
    BYTE                        rgbPadding[cbCheckpoint - sizeof(CHECKPOINT_FIXED) - cbAttach];

    ERR     Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
};

C_ASSERT( 679 == offsetof( CHECKPOINT, rgbAttach ) );

// PERSISTED
#define fCheckpointAttachInfoPresent            0x1

ERR ErrUtilWriteCheckpointHeaders(  const INST * const      pinst,
                                    IFileSystemAPI *const   pfsapi,
                                    const WCHAR             *wszFileName,
                                    const IOFLUSHREASON     iofr,
                                    CHECKPOINT              *pChkptHeader,
                                    IFileAPI *const         pfapi = NULL );

typedef struct tagLGSTATUSINFO
{
    ULONG           cSectorsSoFar;      // Sectors already processed in current gen.
    ULONG           cSectorsExpected;       // Sectors expected in current generation.
    ULONG           cGensSoFar;         // Generations already processed.
    ULONG           cGensExpected;      // Generations expected.
    BOOL            fCountingSectors;       // Are we counting bytes as well as generations?
    JET_PFNINITCALLBACK     pfnCallback;        // Status callback function.
    void *                  pvCallbackContext;  // Context for status callback function
    JET_SNPROG              snprog;             // Progress notification structure.
} LGSTATUSINFO;


#include <poppack.h>


//------ redo.c -------------------------------------------------------------


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
    redoattachmodeAttachDbLR = 0,       //  redo the attachment because we are replaying an AttachDb log record
    redoattachmodeCreateDbLR,           //  redo the attachment because we are replaying a CreateDb log record
    redoattachmodeInitLR,               //  redo the attachment because we are replaying an Init log record
    redoattachmodeInitBeforeRedo        //  redo the attachment because we are initialising before redoing logged operations
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


//------ function headers ---------------------------------------------------

//  external utility functions


//  flags for ErrLGLogRec()
#define fLGCreateNewGen         0x00000001
#define fLGStopOnNewGen         0x00000004  //  currently, CANNOT be used concurrently and MUST be used with fLGCreateNewGen - stops after the new gen has been created
#define fLGFillPartialSector    0x00000008  //  not logging a real log record, just filling any current partial sector
#define fLGMacroGoing           0x00000010  //  In a macro, loosen transaction size limits

//  flags for ErrLGNewLogFile()
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

//  internal utility functions

ERR ErrLGIReadFileHeader(
    IFileAPI * const    pfapiLog,
    const TraceContext& tc,
    OSFILEQOS           grbitQOS,
    LGFILEHDR *         plgfilehdr,
    const INST * const  pinst = NULL );

BOOL FLGVersionZeroFilled( const LGFILEHDR_FIXED* const plgfilehdr );
BOOL FLGVersionAttachInfoInCheckpoint( const LGFILEHDR_FIXED* const plgfilehdr );

VOID LGIGetDateTime( LOGTIME *plogtm );

//  logical logging related

UINT CbLGSizeOfRec( const LR * );
UINT CbLGFixedSizeOfRec( const LR * );
#ifdef DEBUG
VOID AssertLRSizesConsistent();
#endif

ERR ErrLrToLogCsvSimple( CWPRINTFFILE * pcwpfCsvOut, LGPOS lgpos, const LR *plr, LOG * plog );

BOOL FLGDebugLogRec( LR *plr );

// In order to output 1 byte of raw data, need 3 bytes - two to
// represent the data and a trailing space.
// Also need a null-terminator to mark the end of the data stream.
// Finally, DWORD-align the buffer.
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
        Assert( ( ( dbidIn < dbidMax ) && ( pgno != pgnoNull ) && ( pgno < pgnoMax ) ) ||  // Normal case.
                ( ( dbidIn == dbidMax ) && ( pgno == pgnoMax ) ) );  // Used as a sentinel in the log dump code.
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

//  Helper class base for log pre-reading during recovery.
class LogPrereaderBase
{
protected:
    //  Constructor is not available.
    LogPrereaderBase();

public:
    //  Destructor may be overriden.
    virtual ~LogPrereaderBase();

    //  Pure virtual methods.
protected:
    virtual ERR ErrLGPIPrereadPage( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf ) = 0;
public:
    virtual ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid ) = 0;

    //  Public methods.
public:
    VOID LGPInit( const DBID dbidMaxUsed, const CPG cpgGrowth );
    VOID LGPTerm();
    VOID LGPDBEnable( const DBID dbid );
    VOID LGPDBDisable( const DBID dbid );
    BOOL FLGPEnabled() const;
    BOOL FLGPDBEnabled( const DBID dbid ) const;
    ERR ErrLGPAddPgnoRef( const DBID dbid, const PGNO pgno, const LR* const plr = NULL );
    VOID LGPSortPages();
    ERR ErrLGPPrereadExtendedPageRange( const DBID dbid, const PGNO pgno, CPG* const pcpgPreread, const BFPreReadFlags bfprf = bfprfDefault );
    size_t IpgLGPGetSorted( const DBID dbid, const PGNO pgno ) const;

    //  Protected methods.
protected:
    size_t CpgLGPIGetArrayPgnosSize( const DBID dbid ) const;
    size_t IpgLGPIGetUnsorted( const DBID dbid, const PGNO pgno ) const;
    ERR ErrLGPISetEntry( const DBID dbid, const size_t ipg, const PGNO pgno, const IOREASONSECONDARY iors = iorsNone );
    PGNO PgnoLGPIGetEntry( const DBID dbid, const size_t ipg ) const;
    IOREASONSECONDARY IorsLGPIGetEntry( const DBID dbid, const size_t ipg ) const;

private:

    struct PageRef
    {
        PageRef()
        {
            pgno = pgnoNull;
            iors = iorsNone;
        }

        PageRef( PGNO pgnoIn, IOREASONSECONDARY iorsIn = iorsNone )
        {
            pgno = pgnoIn;
            iors = iorsIn;
        }

        PGNO                pgno;
        IOREASONSECONDARY   iors;
    };

    //  Private methods.
private:
    static INT __cdecl ILGPICmpPagerefs( const PageRef* ppageref1, const PageRef* ppageref2 );

    //  Private state.
private:
    DBID m_dbidMaxUsed;
    CPG m_cpgGrowth;
    CArray<PageRef>* m_rgArrayPagerefs;
};

//  Helper test class for log pre-reading during recovery.
class LogPrereaderTest : public LogPrereaderBase
{
    //  Pure virtual implementation.
protected:
    ERR ErrLGPIPrereadPage( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf );
public:
    ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid );

    //  Public constructor.
public:
    LogPrereaderTest();

    //  Public methods.
public:
    size_t CpgLGPGetArrayPgnosSize( const DBID dbid ) const;
    VOID LGPAddRgPgnoRef( const DBID dbid, const PGNO* const rgpgno, const CPG cpg );
    BOOL FLGPTestPgnosPresent( const DBID dbid, const PGNO* const rgpgno, const CPG cpg );
};

//  Helper class for log pre-reading during recovery
class LogPrereader : public LogPrereaderBase
{
    //  Pure virtual implementation.
protected:
    ERR ErrLGPIPrereadPage( __in_range( dbidUserLeast, dbidMax - 1 ) const DBID dbid, const PGNO pgno, const BFPreReadFlags bfprf );
public:
    ERR ErrLGPIPrereadIssue( _In_range_( dbidUserLeast, dbidMax - 1 ) const DBID dbid );

    //  Public constructor.
public:
    LogPrereader( INST* const pinst );

    //  Private state.
private:
    INST* m_pinst;
};

//  Helper class for log pre-reading during recovery
class LogPrereaderDummy : public LogPrereaderBase
{
    //  Pure virtual implementation.
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

    //  Public constructor.
public:
    LogPrereaderDummy() {}
};


//  Struct used to build a queue of LGPOS's.

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

enum eLGFileNameSpec { eCurrentLog, // This is the current transaction log, will get rolled to an eArchiveLog at the right time.
                        eCurrentTmpLog, // This is building the next log, by scribbling a fill pattern in this file
                        eArchiveLog,    // This is log that is closed (i.e. with it's final name)
                        eReserveLog,    // This is the logs for the reserve log pool.
                        eShadowLog      // This is a log under construction from an external source
    };


//  ================================================================
class LGInitTerm
//  ================================================================
//
//  Static class containing methods to init/term log
//
//-
{
private:
    LGInitTerm();
    ~LGInitTerm();

public:
    static ERR ErrLGSystemInit( void );
    static VOID LGSystemTerm( void );
};

//  ================================================================
class LGChecksum
//  ================================================================
//
//  Static class containing methods for log checksumming
//
//-
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

//  ================================================================
class LGFileHelper
//  ================================================================
//
//  External static name/file making routines
//
//-
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

//  ================================================================
class CHECKSUMINCREMENTAL
//  ================================================================
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

PERSISTED /* cast to byte */ enum LG_DISK_SEC_SIZE_MISMATCH_DEBUG
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
    BOOL            fDestDBReady;           //  non-ext-restore, dest db copied?
    BOOL            fFileNotFound;          //  the destination file not found, assume will replay CreateDB
    SIGNATURE       signDatabase;           //  database signature
    ULONG           cbDatabaseName;         //  cached database name size, in bytes, including the 0
    JET_SETDBPARAM  *rgsetdbparam;          //  array of database parameters
    ULONG           csetdbparam;            //  number of elements in rgsetdbparam

    // Stuff to calculate checkpoint from db state
    ULONG           dbstate;
    LONG            lGenMinRequired;
    LONG            lGenMaxRequired;
    LONG            lGenLastConsistent;
    LONG            lGenLastAttach;

    // Stuff to initialize rollback snapshot
    BOOL            fRBSFeatureEnabled;     //  whether the DB format supports rollback snapshot
    SIGNATURE       signDatabaseHdrFlush;   //  database signature during last databse header flush.
    SIGNATURE       signRBSHdrFlush;        //  RBS signature during last database header flush.
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

    // Lookup and update mapping for lGen to LOGTIME for stamping logtimeMaxGenRequired
    ERR ErrAddLogtimeMapping( const LONG lGen,  const LOGTIME* const pLogtime );
    VOID LookupLogtimeMapping( const LONG lGen, LOGTIME* const pLogtime );
    VOID TrimLogtimeMapping( const LONG lGen );

 private:
    // stuff to store history of LOGTIME of recent logs for stamping logtimeMaxGenRequired
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
    // Initialize the committed area from _pbLGBufMin with cBytes.
    ERR InitCommit(LONG cBytes);
    // Commit memory between pb and pb+cb.
    BOOL  Commit( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    // Ensures that buffer between pb and pb+cb (may wrap around) are committed. 
    // This function is not multi-thread safe and caller should guarantee that.
    BOOL  EnsureCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    // Decommits buffer between pb and pb+cb (may wrap around).
    // This function is not multi-thread safe and caller should guarantee that.
    VOID  Decommit( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    BOOL  FIsCommitted( _In_reads_( cb ) const BYTE *pb, ULONG cb );
    LONG  CommittedBufferSize();
    
#ifdef DEBUGGER_EXTENSION
    VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

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

    //  in memory log buffer related pointers

    BYTE            *_pbLGBufMin;
#define m_pbLGBufMin    m_pLogBuffer->_pbLGBufMin
    BYTE            *_pbLGBufMax;
#define m_pbLGBufMax    m_pLogBuffer->_pbLGBufMax

    // Size of log buffer, for convenience.
    ULONG           _cbLGBuf;
#define m_cbLGBuf       m_pLogBuffer->_cbLGBuf
    UINT            _csecLGBuf;             // available buffer
#define m_csecLGBuf     m_pLogBuffer->_csecLGBuf

    // Following variables are strictly used by writer only, but also updated
    // by the reader
    // m_pbEntry, m_pbWrite, m_isecWrite, m_lgposToWrite are all
    // protected by m_critLGBuf.

    BYTE            *_pbEntry;          //  location of next buffer entry
#define m_pbEntry       m_pLogBuffer->_pbEntry
    BYTE            *_pbWrite;          //  location of next sec to write
#define m_pbWrite       m_pLogBuffer->_pbWrite

    INT             _isecWrite;         //  next disk sector to write
#define m_isecWrite     m_pLogBuffer->_isecWrite
    LGPOS           _lgposToWrite;      //  first log record to write
#define m_lgposToWrite  m_pLogBuffer->_lgposToWrite
    LGPOS           _lgposFlushTip;     //  LGPOS upto which the LOG is flushed already
#define m_lgposFlushTip m_LogBuffer._lgposFlushTip

    // Maximum LGPOS to set m_lgposToWrite when a log record crosses a sector boundary
    // of a full sector write and the subsequent partial sector write (or full sector write).
    // See log.cxx:ErrLGLogRec() for more detailed info.
    LGPOS           _lgposMaxWritePoint;
#define m_lgposMaxWritePoint    m_pLogBuffer->_lgposMaxWritePoint

    BYTE            *_pbLGFileEnd;
#define m_pbLGFileEnd   m_pLogBuffer->_pbLGFileEnd
    LONG            _isecLGFileEnd;
#define m_isecLGFileEnd m_pLogBuffer->_isecLGFileEnd

    BYTE        *_pbLGCommitStart;  // inclusive
    BYTE        *_pbLGCommitEnd;    // inclusive, otherwise, from any point inside the buffer, it is impossible to represent fully committed case.
    CCriticalSection    _critLGBuf;
#define m_critLGBuf m_pLogBuffer->_critLGBuf
};

#define NUM_WASTAGE_SLOTS       20

class LOG
    :   public CZeroInit
{
    //  LOG controls a few API sets ... which are roughly organized into this hierarchy.
    //
    //   - Alloc/Init/Term
    //   - Log Stream Control
    //      ---- Generic ----
    //      - Checkpoint
    //      - Stream Management (rolling logs, freezing, truncation)
    //      - Replay Control (future)
    //          - Input
    //          - Redo / Replay
    //          - Undo
    //      - Recovery Support
    //      - Backup Support (deprecated)
    //      - Restore Support (deprecated???)
    //   - Logging API (logapi.hxx outside of LOG)
    //   - Log Buffer (writing LRs, asking for commit flush, etc)
    //   - Dumping / Debugging
    //
 
    //  ================================================================
    //      Public Alloc/Init/Term
    //

    friend VOID LrToSz( const LR *plr, __out_bcount(cbLR) PSTR szLR, ULONG cbLR, LOG * plog );

#pragma push_macro( "new" )
#undef new
private:
    void* operator new[]( size_t );         //  not supported
    void operator delete[]( void* );        //  not supported

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


    //  ================================================================
    //      Generic Log Properties and Manipulators
    //

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

    //  used by BF to turn a LGPOS into a continuous number space to help w/ datastructure support
    QWORD CbLGOffsetLgposForOB0( LGPOS lgpos1, LGPOS lgpos2 ) const;
    LGPOS LgposLGFromIbForOB0( QWORD ib ) const;

    VOID SetNoMoreLogWrite( const ERR err );
    VOID ResetNoMoreLogWrite();
    PCWSTR LogExt() const;
    ULONG CbLGSec() const;
    ULONG CSecLGFile() const;
    VOID SetCSecLGFile( ULONG csecLGFile );
    ULONG CSecLGHeader() const;

    //  ================================================================
    //      Log Buffer
    //

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



    //  ================================================================
    //      Log Control :: Replay Control
    //

    ERR ErrLGShadowLogAddData(
        JET_EMITDATACTX *   pEmitLogDataCtx,
        void *              pvLogData,
        ULONG       cbLogData );


    //  ================================================================
    //      Log Control :: Stream Management
    //

    ERR ErrLGCheckState();
    ERR ErrLGTruncateLog( const LONG lgenMic, const LONG lgenMac, const BOOL fSnapshot, BOOL fFullBackup );
    ERR ErrLGVerifyFileSize( QWORD qwFileSize );


    VOID LGSetSectorGeometry(
        const ULONG cbSecSize,
        const ULONG csecLGFile );

    // used by backup
    ERR ErrLGGetGenerationRange(
            __in PCWSTR wszFindPath,
            LONG* plgenLow,
            LONG* plgenHigh,
            __in BOOL fLooseExt = fFalse,
            __out_opt BOOL * pfDefaultExt = NULL );

    //  used by backup & restore
    ERR ErrLGWaitForLogGen( const LONG lGeneration );

    //  used in a few places, breaks encapsulation ... most users (except one) only want current lgen
    LONG LGGetCurrentFileGenWithLock( LOGTIME *ptmCreate = NULL );
    LONG LGGetCurrentFileGenNoLock( LOGTIME *ptmCreate = NULL ) const;
    BOOL FLGProbablyWriting();

    //  used by backup - maybe should be named StopLogging/ResumeLogging?
    VOID LGCreateAsynchCancel( const BOOL fWaitForPending );
    ERR ErrStartAsyncLogFileCreation( _In_ PCWSTR wszPathJetTmpLog, _In_ const LONG lgenDebug );

    //  used in only 4 places, to make sure something is written ... a different contract may be interesting
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
    // Only for debug diagnostics validating DB upgrades.
    ERR ErrLGGetPersistedLogVersion( _In_ const JET_ENGINEFORMATVERSION efv, _Out_ const LogVersion ** const pplgv ) const;
#endif

    ERR ErrLGCreateEmptyShadowLogFile( const WCHAR * const wszPath, const LONG lgenShadow );

    //  used to upgrade log version
    BOOL FLGFileVersionUpdateNeeded( _In_ const LogVersion& lgvDesired );

    //  ================================================================
    //      Basic Logging 
    //

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

    //  ================================================================
    //      waypoint?  log control?  log management
    //

    ERR ErrLGUpdateWaypointIFMP( IFileSystemAPI *const pfsapi, __in const IFMP ifmpTarget = ifmpNil );
    ERR ErrLGQuiesceWaypointLatencyIFMP( __in const IFMP ifmpTarget );
    LONG LLGElasticWaypointLatency() const;
    BOOL FWaypointLatencyEnabled() const;

    //  ================================================================
    //      Logical Logging / Recovery::Redo related
    //

    // externally dump using this to load attachments off checkpoint ... consider how this is used.
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

    // Same functionality, the second one requires checkpoint lock to be held
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


    //  ================================================================
    //      Backup Support
    //

    BOOL FLGLogPaused() const;
    VOID LGSetLogPaused( BOOL fValue );
    VOID LGSignalLogPaused( BOOL fSet );
    VOID LGWaitLogPaused();

    //  ================================================================
    //      Restore Support
    //

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

    //  ================================================================
    //      checkpoint control / update / read functions
    //

    ERR ErrLGUpdateCheckpointFile( const BOOL fForceUpdate );
    LGPOS LgposLGCurrentCheckpointMayFail() /* would be const, but m_critCheckpoint.Enter() "modifies state" :P */;
    LGPOS LgposLGCurrentDbConsistencyMayFail() /* would be const, but m_critCheckpoint.Enter() "modifies state" :P */;
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


    // The log and checkpoint extension shouldn't be set directly, use LGSetLogChkExts(), and
    // once set (with one exception in ErrLGMoveToRunningState (calls ErrLGRICleanupMismatchedLogFiles)
    // it can't be reset after that.  The two extensions should always change together.
    //
    // The log ext will auto configure itself to the available log files' extension during init/restore, but
    // will try to use / prefer the extension per set the JET_paramLegacyFileNames param first.
    // After the log extension is initally configured, it can change to the preferred extension in
    // ErrLGRICleanupMismatchedLogFiles(), but only if we're using circular logging and not running
    // in ese[nt]ut[i]l.
    //
    void LGSetChkExts( BOOL fLegacy, BOOL fReset );
    PCWSTR WszLGGetDefaultExt( BOOL fChk );
    PCWSTR WszLGGetOtherExt( BOOL fChk );
    BOOL FLGIsDefaultExt( BOOL fChk, PCWSTR szExt );
    BOOL FLGIsLegacyExt( BOOL fChk, PCWSTR szExt );


    //  ================================================================
    //      Log File Dumping & LOG Debugging
    //

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
                FLAG32  m_fPermitPatching   :   1;  //  permit fixup of torn writes at the end of edb.log
                FLAG32  m_fSummary          :   1;  //  output the IO summary at end of log dumps
            };
        };
        CWPRINTFFILE* m_pcwpfCsvOut; // non-NULL indicates do CSV output.
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
#endif  //  DEBUGGER_EXTENSION


private:
    INST            *m_pinst;
    ILogStream *    m_pLogStream;
    LOG_READ_BUFFER *m_pLogReadBuffer;
    LOG_WRITE_BUFFER *m_pLogWriteBuffer;
    LOG_BUFFER      m_LogBuffer;

private:
    //  status variable for logging

    BOOL            m_fLogInitialized;
    BOOL            m_fLogDisabled;

    BOOL            m_fLogDisabledDueToRecoveryFailure;
    BOOL            m_fLGNoMoreLogWrite;
    ERR             m_errNoMoreLogWrite;
    LONG            m_lgenInitial;

    ERR             m_errCheckpointUpdate;
    LGPOS           m_lgposCheckpointUpdateError;

    //  status variable RECOVERY

    BOOL            m_fRecovering;
    RECOVERING_MODE m_fRecoveringMode;
    BOOL            m_fHardRestore;
    BOOL            m_fRecoveryUndoLogged;

    //  File MAP

    BOOL            m_fLGFMPLoaded;

    SIGNATURE       m_signLog;
    BOOL            m_fSignLogSet;

    // XXX
    // It may be desirable to change this to "m_pszLogCurrent" to prevent
    // conflict with "differing" Hungarians (plus to make some
    // "interesting" pointer comparison code easier to read).

    // Current log file directory. Either the standard
    // log file directory or the restore directory.
    PCWSTR          m_wszLogCurrent;

    //  LOG's checkpoint extension (.jcp | .chk respectively)
    PCWSTR          m_wszChkExt;

    //  variables used in logging only, for debug

    LGPOS           m_lgposStart;               //  when lrStart is added

    LGPOS           m_lgposRecoveryUndo;

    CCriticalSection    m_critLGTrace;

    // ****************** members for checkpointing ******************

    //  this checkpoint design is an optimization.  JET logging/recovery
    //  can still recover a database without a checkpoint, but the checkpoint
    //  allows faster recovery by directing recovery to begin closer to
    //  logged operations which must be redone.

    //  in-memory checkpoint

    CHECKPOINT      *m_pcheckpoint;

    //  critical section to serialize read/write of in-memory and on-disk
    //  checkpoint.  This critical section can be held during IO.

    CCriticalSection    m_critCheckpoint;

    //  disable checkpoint write during key moments of engine termination.
    //  Note: used to disable if checkpoint shadow sector corrupted, but that code appears disabled.
    //  Default to true until checkpoint initialization.
    FLAG32           m_fDisableCheckpoint:1;

    //  there are pending redo map entries to reconcile.
    FLAG32           m_fPendingRedoMapEntries:1;

    //  checkpoint implementation functions

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

    // ****************** members for wastage calculation ******************

    ULONG           m_rgWastages[ NUM_WASTAGE_SLOTS ];
    ULONG           m_iNextWastageSlot;
    ULONG           m_cbCurrentUsage;
    ULONG           m_cbCurrentWastage;
    LONG            m_cbTotalWastage;

    // ****************** members for redo ******************

    FLAG32          m_fTruncateLogsAfterRecovery:1;
    FLAG32          m_fIgnoreLostLogs:1;
    FLAG32          m_fLostLogs:1;
    FLAG32          m_fReplayingIgnoreMissingDB:1;
    FLAG32          m_fReplayMissingMapEntryDB:1;
    FLAG32          m_fReplayIgnoreLogRecordsBeforeMinRequiredLog :1;
    // Please keep debug only bits at end, so allow debug/retail binaries to be able to debug each other
#ifdef DEBUG
    FLAG32          m_fEventedLLRDatabases:1;
#endif

    //  variables used during redo operations

    LGPOS           m_lgposRedo;

    //  Database page preread state for redo
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

    LGPOS           m_lgposRecoveryStop;                // position where to sop recovery, valid only with m_fRecoveryWithoutUndo set

    WCHAR           m_wszRestorePath[IFileSystemAPI::cchPathMax];
    WCHAR           m_wszNewDestination[IFileSystemAPI::cchPathMax];

    RSTMAP          *m_rgrstmap;
    INT             m_irstmapMac;

    BOOL            m_fNewCheckpointFile;

    LGEN_LOGTIME_MAP m_MaxRequiredMap;

    // ****************** members for restore ******************

    BOOL            m_fExternalRestore;

    LONG            m_lGenLowRestore;
    LONG            m_lGenHighRestore;

    // after restoring from the backup set
    // we can switch to a new directory where to replay logs
    // until a certain generation (soft recovery after restore)
    WCHAR           m_wszTargetInstanceLogPath[IFileSystemAPI::cchPathMax];
    LONG            m_lGenHighTargetInstance;

    FLAG32          m_fDumpingLogs:1;
    FLAG32          m_iDumpVerbosityLevel:2;

    //  Log Write / Flush
    //
    CCriticalSection        m_critShadowLogConsume;
    CShadowLogStream *          m_pshadlog;
    ERR ErrLGIShadowLogInit();
    ERR ErrLGIShadowLogTerm_();
    ERR ErrLGShadowLogTerm();

    // ****************** debugging variables ******************

    TEMPTRACE       *m_pttFirst;
    TEMPTRACE       *m_pttLast;

#ifdef DEBUG
    BOOL            m_fDBGFreezeCheckpoint;
    BOOL            m_fDBGTraceRedo;
    BOOL            m_fDBGTraceBR;
    BOOL            m_fDBGNoLog;
#endif

    INT             m_cNOP;

    //  ---------------------------------------------------------
    //      Log Control :: Replay Control (INTERNAL)
    //

    ERR  ErrGetLgposRedoWithCheck();

    ERR ErrLGISetQuitWithoutUndo( const ERR errBeginUndo );
    BOOL FLGILgenHighAtRedoStillHolds();
    ERR ErrLGIBeginUndoCallback_( __in const CHAR * szFile, const ULONG lLine );

    ERR ErrLGOpenPagePatchRequestCallback( const IFMP ifmp, const PGNO pgno, const DBTIME dbtime, const void * const pvPage ) const;

    ERR ErrLGEvaluateDestructiveCorrectiveLogOptions(
            __in const LONG lgenBad,
            __in const ERR errCondition );

    //  ---------------------------------------------------------
    //      (Do)Logging Support
    //

    ERR ErrLGITrace( PIB *ppib, __in PSTR sz, BOOL fInternal );

    //  ---------------------------------------------------------
    //      inter-file / format management
    //

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


    //  ---------------------------------------------------------
    //      Restore Support (INTERNAL)
    //

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

    //  ---------------------------------------------------------
    //      Restore map for recovery/restore
    //
    INT IrstmapGetRstMapEntry( const WCHAR *wszName );
    ERR ErrReplaceRstMapEntryBySignature( const WCHAR *wszName, const SIGNATURE * pDbSignature );
    ERR ErrReplaceRstMapEntryByName( const WCHAR *wszName, const SIGNATURE * pDbSignature );
    ERR ErrBuildRstmapForRestore( PCWSTR wszRestorePath );
    ERR ErrBuildRstmapForExternalRestore( JET_RSTMAP_W *rgjrstmap, INT cjrstmap );
    BOOL FRstmapCheckDuplicateSignature();
    ERR ErrGetDestDatabaseName( const WCHAR *wszDatabaseName, PCWSTR wszRestorePath, PCWSTR wszNewDestination, INT *pirstmap, LGSTATUSINFO *plgstat );


    //  ---------------------------------------------------------
    //      Recovery::Redo/Undo (INTERNAL)
    //

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

    //      Init/Term/Undo Transition management
    //

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

    ERR ErrLGRIRedoOperations( const LE_LGPOS *ple_lgposRedoFrom, BYTE *pbAttach, BOOL fKeepDbAttached, BOOL* const pfRcvCleanlyDetachedDbs, LGSTATUSINFO *plgstat, TraceContextScope& tcScope );
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

    //      Recovery::Redo Internal Management
    //

    ERR ErrLGRIPpibFromProcid( PROCID procid, PIB **pppib );
    static BOOL FLGRICheckRedoConditionForAttachedDb(
                const BOOL          fReplayIgnoreLogRecordsBeforeMinRequiredLog,
                const DBFILEHDR *   pdbfilehdr,
                const LGPOS&        lgpos );
    BOOL FLGRICheckRedoConditionForDb(
                const DBID          dbid,
                const LGPOS&        lgpos );
    ERR ErrLGRICheckRedoCondition(
                const DBID      dbid,                   //  dbid from the log record.
                DBTIME          dbtime,                 //  dbtime from the log record.
                OBJID           objidFDP,               //  objid so far,
                PIB             *ppib,                  //  returned ppib
                const BOOL      fUpdateCountersOnly,    //  if TRUE, operation will NOT be redone, but still need to update dbtimeLast and objidLast counters
                BOOL            *pfSkip );              //  returned skip flag
    ERR ErrLGRICheckRedoConditionInTrx(
                const PROCID    procid,
                const DBID      dbid,                   //  dbid from the log record.
                DBTIME          dbtime,                 //  dbtime from the log record.
                OBJID           objidFDP,
                const LR        *plr,
                PIB             **pppib,                //  returned ppib
                BOOL            *pfSkip );              //  returned skip flag
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

    //      Reporting
    //

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

    //      Recovery::Redo Operations
    //

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
#endif  // ENABLE_JET_UNIT_TEST

    //  Debugger accessors
    friend CHECKPOINT *        PcheckpointEDBGAccessor( const LOG * const plog );
    friend const LOG_BUFFER *  PlogbufferEDBGAddrAccessor( const LOG * const plog );
    friend ILogStream *        PlogstreamEDBGAccessor( const LOG * const plog );
    friend LOG_WRITE_BUFFER *  PlogwritebufferEDBGAccessor( const LOG * const plog );

};  // end of class LOG

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
        //  if we see these errors we assume that something is not configured
        //  properly
        //
        case JET_errBadLogVersion:
        case JET_errBadLogSignature:
        case JET_errLogSectorSizeMismatch:
        case JET_errLogFileSizeMismatch:
        case JET_errPageSizeMismatch:
            return HaDbFailureTagConfiguration;

        //  if we see these errors then we assume something is corrupted
        //
        case JET_errFileInvalidType:
        case JET_errDatabase200Format:
        case JET_errDatabase400Format:
        case JET_errDatabase500Format:
            return HaDbFailureTagCorruption;

        case JET_errLogFileCorrupt:
            return HaDbFailureTagRecoveryRedoLogCorruption;

        //  these are handled elsewhere so don't give conflicting guidance
        //
        case JET_errLogDiskFull:
        case JET_errDiskFull:
        case JET_errOutOfMemory:
        case JET_errDiskIO:
        case_AllDatabaseStorageCorruptionErrs:
            return HaDbFailureTagNoOp;

        //  if anything else makes the log go down, try a remount to fix it
        //
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
        //  the flag may get set multiple times,
        //  so only record the error the very
        //  first time the flag was set
        //  UNDONE: there's a concurrency hole
        //  where multiple threads may try to set
        //  this for the first time, but it's not
        //  a big deal
        //
        m_errNoMoreLogWrite = err;

        if ( err != errLogServiceStopped )
        {
            //  emit a failure tag appropriate to the failure
            //
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
        // If JET_paramRecovery is exactly "repair", then enable logging.  If anything
        // follows "repair", then disable logging.
        return ( SzParam( this, JET_paramRecovery )[6] != L'\0' );
    }
    return (    SzParam( this, JET_paramRecovery )[0] == L'\0' ||
                _wcsicmp ( SzParam( this, JET_paramRecovery ), wszOn ) != 0 );
}

//  A place with all the storage / disk / controller induced errors that occur against the transaction log.
//  Note: this is for the log only, another set of errors is in FErrIsDbCorruption().

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

    Assert ( INST::FOwnerCritInst() );  //  checking that we've locked g_rgpinst in memory
    if ( g_rgpinst == NULL )
    {
        //  calling before even system init, by definition there is no matching instance.
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
        // found !
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

    Assert ( INST::FOwnerCritInst() );  //  checking that we've locked g_rgpinst in memory
    if ( g_rgpinst == NULL )
    {
        //  calling before even system init, by definition there is no matching instance.
        return NULL;
    }

    //  we need an FS to perform path validation

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

        // found
        pinstFound = pinst;
        break;
    }

HandleError:
    delete pfsapi;
    return pinstFound;
}

