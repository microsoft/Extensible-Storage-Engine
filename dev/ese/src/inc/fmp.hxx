// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
//  DBEnforce
//  ================================================================

#if ( defined _PREFIX_ || defined _PREFAST_ )
#define DBEnforceSz( ifmp, exp, sz )            ( ( exp ) ? (void) 0 : exit(-1) )
#else
#define DBEnforceSz( ifmp, exp, sz )            ( ( exp ) ? (void) 0 : DBEnforceFail( ifmp, sz, __FILE__, __LINE__ ) )
#endif

#define DBEnforce( ifmp, exp )      DBEnforceSz( ifmp, exp, #exp )

VOID __stdcall DBEnforceFail( const IFMP ifmp, const CHAR* szMessage, const CHAR* szFilename, LONG lLine );


class DbfilehdrLock;
class CFlushMapForAttachedDb;
struct PATCHHDR;
typedef PATCHHDR        PATCH_HEADER_PAGE;
class LIDMAP;
class CLogRedoMap;

//  ================================================================
class PdbfilehdrLocked
//  ================================================================
//
//  Gets the lock of the DbfilehdrLock and releases it in 
//  the destructor. Provides an -> operator so it works like
//  a pointer to a DBFILEHDR.
//
//  There are a few subtle points about this:
//
//  There is not a conversion operator for 'DBFILEHDR*', that
//  prevents unexpected automatic conversions. Instead the get()
//  method should be used.
//
//-
{
public:
    virtual ~PdbfilehdrLocked();

    // used for 'if( pdbfilehdr ) {'    
    operator const void*() const;
    const DBFILEHDR * operator->() const;
    const DBFILEHDR * get() const;

protected:
    PdbfilehdrLocked(DbfilehdrLock * const plock);
    PdbfilehdrLocked(PdbfilehdrLocked& rhs);

    DbfilehdrLock * Plock();
    
private:
    DbfilehdrLock * m_plock;

private:    // not implemented
    PdbfilehdrLocked& operator=(PdbfilehdrLocked& rhs);
};


//  ================================================================
class PdbfilehdrReadOnly : public PdbfilehdrLocked
//  ================================================================
{
public:
    PdbfilehdrReadOnly(DbfilehdrLock * const plock);
    PdbfilehdrReadOnly(PdbfilehdrReadOnly& rhs);
    ~PdbfilehdrReadOnly();
};


//  ================================================================
class PdbfilehdrReadWrite : public PdbfilehdrLocked
//  ================================================================
{
public:
    PdbfilehdrReadWrite(DbfilehdrLock * const plock);
    PdbfilehdrReadWrite(PdbfilehdrReadWrite& rhs);
    ~PdbfilehdrReadWrite();

    DBFILEHDR * operator->();
    DBFILEHDR * get();
};


//  ================================================================
class DbfilehdrLock
//  ================================================================
//
//  Stores a pointer to a DBFILEHDR and provides locking.
//
//  This returns lock objects, which may be temporary. MSDN says
//  the lifetime is until the end of the statement (the semi-colon).
//
//-
{
public:
    DbfilehdrLock();
    ~DbfilehdrLock();

    // replaces the value and returns the old value
    DBFILEHDR * SetPdbfilehdr(DBFILEHDR * const pdbfilehdr);

    PdbfilehdrReadOnly  GetROHeader();
    PdbfilehdrReadWrite GetRWHeader();

private:
    friend class PdbfilehdrLocked;
    friend class PdbfilehdrReadOnly;
    friend class PdbfilehdrReadWrite;
#ifndef MINIMAL_FUNCTIONALITY   // debugger ext
    friend class FMP;
    friend DBFILEHDR * PdbfilehdrEDBGAccessor( const FMP * const pfmp );
#endif

    DBFILEHDR * Pdbfilehdr();
    const DBFILEHDR * Pdbfilehdr() const;
    
    void EnterAsReader();
    void LeaveAsReader();
    void EnterAsWriter();
    void LeaveAsWriter();
    
private:
    DBFILEHDR           *       m_pdbfilehdr;
    CReaderWriterLock           m_rwl;
    TLS                 *       m_ptlsWriter;
    INT                         m_cRecursion;

private:    // not implemented
    DbfilehdrLock(const DbfilehdrLock&);
    DbfilehdrLock& operator=(const DbfilehdrLock&);
};

/*
 *  FMP
 *
 *  Basic critical sections:
 *      critFMPPool     - get an uninitialized entry or initialized one if existing.
 *                        used to check/set/reset the id (szDatabaseName) of the fmp.
 *      pfmp->critLatch - serve as the cs for read/change of each FMP structure.
 *
 *  Supported gates:
 *      gateExtendingDB - allow users to wait for DB extension.
 *      fExtendingDB    - associated with gate extendingDB.
 *
 *      gateWriteLatch  - allow users to wait for writelatch to be released.
 *      fWriteLatch     - associated with gate writelatch. Write latch mainly
 *                      - protect the database operation related fields
 *                      - such as pfapi, fAttached etc. Once a database is
 *                      - opened (cPin != 0) then the fields will be not be
 *                      - changed since no writelatch will be allowed if cPin != 0.
 *
 *  Read/Write sequence control:
 *      WriteLatch      - use the gate above.
 *      ReadLatch       - use critLatch3 to protect since the read is so short.
 *
 *  Derived data structure protection:
 *      cPin            - to make sure the database is used and can not be freed.
 *                        such that the pointers in the fmp will be effective.
 */

class FMP
    :   public CZeroInit
{
    private:

        struct RANGE
        {
            PGNO    pgnoStart;
            PGNO    pgnoEnd;
        };

        struct RANGELOCK
        {
            SIZE_T  crange;
            SIZE_T  crangeMax;
            RANGE   rgrange[];
        };


#ifdef DEBUG
    public:
        enum DbHeaderUpdateState    // dbhus
            {
                dbhusNoUpdate       = 0,
                dbhusHdrLoaded      = 1,
                dbhusUpdateLogged   = 2,
                dbhusUpdateSet      = 3,    // set in DBFILEHDR - analgous to "dirty" for a page
                dbhusUpdateFlushed  = 4,
            };
        //  rotated between states for logged DB header updates
        //
        DbHeaderUpdateState m_dbhus;
#endif

    public:
        // Constructor/destructor

        FMP();
        ~FMP();

    private:
    
        WCHAR               *m_wszDatabaseName;     /*  id identity - database file name        */

        INST                *m_pinst;
        ULONG               m_dbid;                 /*  s  instance dbid */
        union
        {
            UINT            m_fFlags;
            struct
            {
                UINT        m_fCreatingDB:1;        /*  st DBG: DB is being created             */
                UINT        m_fAttachingDB:1;       /*  st DBG: DB is being Attached            */
                UINT        m_fDetachingDB:1;       /*  st DBG: DB is being Detached            */

                UINT        m_fExclusiveOpen:1;     /*  l  DB Opened exclusively                */
                UINT        m_fReadOnlyAttach:1;    /*  f  ReadOnly database?                   */

                UINT        m_fLogOn:1;             /*  f  logging enabled flag                 */
                UINT        m_fVersioningOff:1;     /*  f  disable versioning flag              */

                UINT        m_fSkippedAttach:1;     /*  f  db is missing, so attach was skipped (hard recovery only) */
                UINT        m_fAttached:1;          /*  f  DB is in attached state.             */
                UINT        m_fDeferredAttach:1;    /*  f  deferred attachment during recovery  */
                UINT        m_fDeferredAttachConsistentFuture:1;    /*  f  deferred attachment during recovery because of being consistent in future    */
                UINT        m_fDeferredForAccessDenied:1;   /*  f   deferred attachment during recovery because of access denied error  */
                UINT        m_fIgnoreDeferredAttach : 1;    /*  f  ignore this deferred attachment during recovery  */
                UINT        m_fFailFastDeferredAttach : 1;  /*  f  fail fast on a deferred attachment during recovery   */
                UINT        m_fOverwriteOnCreate : 1;   /*  f  allow overwrite of an existing db on create during recovery  */

                UINT        m_fRunningOLD:1;        /*  st Is online defrag currently running on this DB? */

                UINT        m_fInBackupSession:1;   /*  f the db was involved in a backup process in the last external backup session */

                UINT        m_fAllowHeaderUpdate:1;     /*  st : checkpoint advancement should ignore this db, the db is advanced in the detach process, no more logging */

                UINT        m_fDefragPreserveOriginal:1; /* f do we want to preserve the original db after a defrag? */

                UINT        m_fCopiedPatchHeader:1; /*  for backup: has patch header been appended to end of backup database? */

                UINT        m_fEDBBackupDone:1; /*  f the db was involved in a backup process in the last external backup session */

                UINT        m_fDontRegisterOLD2Tasks:1; /*  f ErrOLDRegisterTableForOLD2() shouldn't register tasks on this FMP */

                UINT        m_fCacheAvail:1;    /*  f Is FMP tracking database Avail space */

                UINT        m_fMaintainMSObjids:1;  /*  f Update MSObjids when creating or destroying b-trees */
                UINT        m_fNoWaypointLatency:1; /* allow waypoint to advance to current position */
                UINT        m_fAttachedForRecovery:1; /* attached for recovery */
                UINT        m_fRecoveryChecksDone:1; /* additional checks for using db during recovery done */

                UINT        m_fTrimSupported:1; //  Does the file system support trimming the file (sparse files).

                //  On if required range is greater than current log being replayed.
                //  It indicates whether there are any pages flushed to the database
                //  from the future.
                //
                UINT        m_fContainsDataFromFutureLogs:1;

                //  when we recover a DB from older log files (from before we started logging the initial DB
                //  size during create db), then we have a DB that may need to be demand extended.
                //
                UINT        m_fOlderDemandExtendDb:1;

                // 30 bits used.
                // Don't forget to update edbg.cxx!
            };
        };

        CCriticalSection    m_critOpenDbCheck;

        DBTIME              m_dbtimeLast;           /*  s  timestamp the DB */
        DBTIME              m_dbtimeOldestGuaranteed;
        DBTIME              m_dbtimeOldestCandidate;
        DBTIME              m_dbtimeOldestTarget;

        CCriticalSection    m_critDbtime;   // used to increment and retrieve m_dbtimeLast on platforms that don't support
                                            // atomic 64-bit operations

        DBTIME              m_dbtimeBeginRBS;                   /*  max dbtime at the beginning of a revert snapshot */
        BOOL                m_fRBSOn;                           /*  revert snapshot enabled for db flag                 */
        BOOL                m_fNeedUpdateDbtimeBeginRBS;        /*  Do we need to update the revert snapshot dbtime for this database at the end of required range */


        TRX                 m_trxOldestCandidate;
        TRX                 m_trxOldestTarget;

        OBJID               m_objidLast;            /*  s  use locked increment. */

        ULONG               m_ctasksActive;

        IFileAPI            *m_pfapi;               /*  f  file handle for read/write the file  */

        DbfilehdrLock       m_dbfilehdrLock;        //  fl pointer to the DBFILEHDR and a reader-writer lock

        CCriticalSection    m_critLatch;            /*     critical section of this FMP, rank 3 */
        CGate               m_gateWriteLatch;       /*  l  gate for waiting fWriteLatch         */

        QWORD               m_cbOwnedFileSize;      /*  s  database owned file size (including reserved / i.e. DB headers) */
        QWORD               m_cbFsFileSizeAsyncTarget; /*  s  async extension target, used during async file extension */
        CSemaphore          m_semIOSizeChange;      /*  l  semaphore for extending or shrinking the DB size (physical size, IO level), protects setting m_cbFsFileSizeAsyncTarget */

        UINT                m_cPin;                 /*  l  the fmp entry is Opened and in Use   */
        INT                 m_crefWriteLatch;       /*  l  the fmp entry is being setup         */
        PIB                 *m_ppibWriteLatch;      /*  Who get the write latch             */
        PIB                 *m_ppibExclusiveOpen;   /*  l  exclusive open session               */

        LGPOS               m_lgposAttach;          /*  s  attach/create lgpos */
        LGPOS               m_lgposDetach;          /*  s  detach lgpos */

        CReaderWriterLock   m_rwlDetaching;

        TRX                 m_trxNewestWhenDiscardsLastReported;    //  for tracking when discarded deletes should be reported to eventlog
        CPG                 m_cpgDatabaseSizeMax;   /*  s  maximum database size allowed */
        PGNO                m_pgnoBackupMost;       /*  s  pgno of last database page to track during backup */
        PGNO                m_pgnoBackupCopyMost;   /*  s  pgno of last page copied during backup, 0 == no backup */
        PGNO                m_pgnoSnapBackupMost;   /*  s  pgno of last database page upon freezing in a snapshot backup */

        CSemaphore          m_semRangeLock;         //  acquire to change the range lock
        CMeteredSection     m_msRangeLock;          //  used to count pending writes

        RANGELOCK*          m_rgprangelock[ 2 ];    //  range locks

        DWORD_PTR           m_dwBFContext;          //  BF context for this FMP

        CReaderWriterLock   m_rwlBFContext;         //  BF context lock

        INT                 m_cpagePatch;           /*  s  patch page count                     */
        ULONG               m_cPatchIO;             /*  s  active IO on patch file              */
        IFileAPI            *m_pfapiPatch;          /*  s  file handle for patch file           */
        PATCH_HEADER_PAGE   *m_ppatchhdr;           /*  l  buffer for backup patch page         */

        DBTIME              m_dbtimeCurrentDuringRecovery;      /*  s  timestamp from DB redo operations    */

        DBTIME              m_dbtimeLastScrub;
        LOGTIME             m_logtimeScrub;

        WCHAR               *m_wszPatchPath;

        ATCHCHK             *m_patchchk;

        volatile LGPOS      m_lgposWaypoint;

#ifdef DEBUGGER_EXTENSION
    public: // only public to address FMP::s_ifmpMacCommitted from edbg symbols table.
#endif
        static IFMP         s_ifmpMacCommitted; // Note: Proper Mac var in that it is 1 greater than last valid index (and unlike s_ifmpMacInUse which is == last valid index).
    private:
        static IFMP         s_ifmpMinInUse;
        static IFMP         s_ifmpMacInUse;

        // a small buffer to hold useful portion of db header, used for retain cache across recovery
        // we only allocate up to the last non-zeroed byte
        DATA                m_dataHdrSig;

        // The maxCommitted LGPOS when the database was attached. Used for idempotent operations during recovery.
        LONG                m_lGenMaxCommittedAttachedDuringRecovery;

        // object for async DBScan-maintenance or recovery-follower(i.e. passive)-dbscan
        IDBMScan *          m_pdbmScan;
        CDBMScanFollower *  m_pdbmFollower;

        // a map of the flush bits from the pages within the database
        CFlushMapForAttachedDb *m_pflushmap;

        //  KVPStore of index locales/LCIDs
        CKVPStore *         m_pkvpsMSysLocales;
        
        // a count of asynch IO that are pending to this ifmp without taking a BF lock (ViewCache case)
        volatile LONG       m_cAsyncIOForViewCachePending;

        //  cached value for owned space; only valid if m_fCacheAvail true
        //
        CPG                 m_cpgAvail;

        //  Set if Shrink is running.
        //
        BYTE                m_fShrinkIsRunning;

        //  Target last pgno of the database we're currently trying to shrink.
        //  Setting this has the effect of preventing space from being allocated above that page.
        //  This only returns a valid page number while a shrink operation is in progress (pgnoNull is returned otherwise).
        //
        PGNO                m_pgnoShrinkTarget;

        //  Set if the leak reclaimer is running.
        //
        BYTE                m_fLeakReclaimerIsRunning;

        //  semaphore for trimming the database.
        //
        CSemaphore          m_semTrimmingDB;

        //  semaphore for DB Maintenance.
        //
        CSemaphore          m_semDBM;

        //  on if the DBM should not run.
        //
        BYTE                m_fDontStartDBM;

        //  on if OLD should not run.
        //
        BYTE                m_fDontStartOLD;

        //  on if the Trim Task should not run.
        //
        BYTE                m_fDontStartTrimTask;

        //  on if there is a scheduled periodic trim task.
        //
        BYTE                m_fScheduledPeriodicTrim;

        //  used to suppress events related to format features
        //
        CLimitedEventSuppressor m_lesEventDbFeatureDisabled;

        //  used to suppress # of trees we report skipped nodes for
        //
        CLimitedEventSuppressor m_lesEventBtSkippedNodesCpu;
        CLimitedEventSuppressor m_lesEventBtSkippedNodesDisk;

        //  LIDMAP used by JetCompact
        //
        LIDMAP              *m_plidmap;

        //  Used for station identification, to suppress a trace until ETW tracing is actually on
        //
        COSEventTraceIdCheck m_traceidcheckFmp;
        COSEventTraceIdCheck m_traceidcheckDatabase;

    public:
        //  timing sequence for create, attach, and detach
        //
        CIsamSequenceDiagLog    m_isdlCreate;
        CIsamSequenceDiagLog    m_isdlAttach;
        CIsamSequenceDiagLog    m_isdlDetach;

    private:
        volatile PGNO       m_pgnoHighestWriteLatched;
        volatile PGNO       m_pgnoDirtiedMax;
        volatile PGNO       m_pgnoWriteLatchedNonScanMax;
        volatile PGNO       m_pgnoLatchedScanMax;
        volatile PGNO       m_pgnoPreReadScanMax;
        volatile PGNO       m_pgnoScanMax;

        WORD                m_pctCachePriority;
        JET_GRBIT           m_grbitShrinkDatabaseOptions;
        LONG                m_dtickShrinkDatabaseTimeQuota;
        CPG                 m_cpgShrinkDatabaseSizeLimit;

        BOOL                m_fLeakReclaimerEnabled;
        LONG                m_dtickLeakReclaimerTimeQuota;

        CIoStats *          m_rgpiostats[iotypeMax];

        CSXWLatch           m_sxwlRedoMaps;

        // Keeps track of which pages were zeroed-out (either Sparse/Zeroed, or beyond EOF).
        // This implies that there is a Trim or Shrink log record later in the Required Range
        // that will reconcile this erroneous page.
        CLogRedoMap *       m_pLogRedoMapZeroed;

        // Keeps track of which pages have invalid DB times while redoing the Required Range.
        // Similar to the Zeroed redo maps, this implies there is a Trim or Shrink log record
        // later in the Required Range that will reconcile this erroneous page.
        //
        // This scenario happens in the following:
        // -Restore a database.
        // -Begin0 session1. <-- this lr is in a gen OUTSIDE of the required range, and therefore
        //    is SKIPPED by log-replay. Or it may have even been deleted.
        // -Start required range.
        // -Begin0 session2.
        // -Session2: New-page operation on a zeroed-out page. <-- Gets redone, since we can't
        //    definitively know if it NEEDS to get redone, or will be later trimmed out.
        // -Session1: Update page. <-- Gets SKIPPED, because we have no Begin0 for Session1.
        // -Session2: Update page. <-- Dbtime mismatch, because the previous LR was skipped!
        //
        // The above gets hit occasionally when running `accept nocopy dml onetest forever`.
        CLogRedoMap *       m_pLogRedoMapBadDbtime;

        // Keeps track of which pages have a dbtime equal to dbtimerevert
        // dbtimerevert indicates a page which was reverted back to a new page using revert snapshot.
        // This implies that there is a log record later in the required range which frees up 
        // this erroneous page.
        CLogRedoMap *       m_pLogRedoMapDbtimeRevert;

    // =====================================================================
    // Member retrieval..
    public:

        IFMP Ifmp() const;
        WCHAR * WszDatabaseName() const;
        BOOL FInUse() const;
        CCriticalSection& CritLatch();
        CCriticalSection& CritOpenDbCheck();
        CSemaphore& SemIOSizeChange();
        CSemaphore& SemPeriodicTrimmingDB();
        CSemaphore& SemDBM();
        CGate& GateWriteLatch();
        UINT CPin() const;
        PIB * PpibWriteLatch() const;
        QWORD CbOwnedFileSize() const;
        QWORD CbFsFileSizeAsyncTarget() const;
        PGNO PgnoLast() const;
        IFileAPI *const Pfapi() const;
        UINT CrefWriteLatch() const;
        UINT FCreatingDB() const;
        UINT FAttachingDB() const;
        UINT FDetachingDB() const;
        UINT FNoWaypointLatency() const;
        UINT FAttachedForRecovery() const;
        UINT FRecoveryChecksDone() const;
        UINT FAllowHeaderUpdate() const;
        UINT FExclusiveOpen() const;
        UINT FReadOnlyAttach() const;
        UINT FLogOn() const;
        UINT FVersioningOff() const;
        UINT FShadowingOff() const;
        UINT FSkippedAttach() const;
        UINT FAttached() const;
        UINT FDeferredAttach() const;
        UINT FDeferredAttachConsistentFuture() const;
        UINT FDeferredForAccessDenied() const;
        UINT FIgnoreDeferredAttach() const;
        UINT FFailFastDeferredAttach() const;
        UINT FOverwriteOnCreate() const;
        UINT FRunningOLD() const;
        UINT FInBackupSession() const;
        UINT FOlderDemandExtendDb() const;
        UINT FRBSOn() const;
        UINT FNeedUpdateDbtimeBeginRBS() const;
        DBTIME DbtimeCurrentDuringRecovery() const;
        const LGPOS &LgposAttach() const;
        LGPOS* PlgposAttach();
        const LGPOS &LgposDetach() const;
        LGPOS* PlgposDetach();
        DBTIME DbtimeLast() const;
        DBTIME DbtimeBeginRBS() const;
        DBTIME DbtimeOldestGuaranteed() const;
        DBTIME DbtimeOldestCandidate() const;
        DBTIME DbtimeOldestTarget() const;
        TRX TrxOldestCandidate() const;
        TRX TrxOldestTarget() const;
        TRX TrxNewestWhenDiscardsLastReported() const;
        PATCH_HEADER_PAGE * Ppatchhdr() const;
        PIB * PpibExclusiveOpen() const;
        IFileAPI *const PfapiPatch() const;
        WCHAR * WszPatchPath() const;
        INT CpagePatch() const;
        ULONG CPatchIO() const;
        PGNO PgnoBackupMost() const;
        PGNO PgnoBackupCopyMost() const;
        PGNO PgnoSnapBackupMost() rtlconst;
        ATCHCHK * Patchchk() const;
        OBJID ObjidLast() const;
        INST * Pinst() const;
        DBID Dbid() const;
        CReaderWriterLock& RwlDetaching();
        LONG CbPage() const;
        BOOL FSmallPageDb() const;
        ULONG_PTR UlDiskId() const;
        BOOL FSeekPenalty() const;
        ERR ErrDBFormatFeatureEnabled( const DBFILEHDR* const pdbfilehdr, const JET_ENGINEFORMATVERSION efvFormatFeature );
        ERR ErrDBFormatFeatureEnabled( const JET_ENGINEFORMATVERSION efvFormatFeature );
        BOOL FScheduledPeriodicTrim() const;
        BOOL FTrimSupported() const;
        BOOL FEfvSupported( const JET_ENGINEFORMATVERSION efvFormatFeature );
        CIoStats * Piostats( const IOTYPE iotype ) const;

        BOOL FContainsDataFromFutureLogs() const;
        VOID SetContainsDataFromFutureLogs();
        VOID ResetContainsDataFromFutureLogs();
        
        PdbfilehdrReadOnly Pdbfilehdr() const;
        PdbfilehdrReadWrite PdbfilehdrUpdateable();
        CRevertSnapshot* PRBS();

private:
        // all access to this R/W lock must go through wrapper functions below
        CReaderWriterLock& RwlIBFContext();
public:
        // add more wrappers if more CReaderWriterLock member functions are needed
#ifdef DEBUG
        BOOL FBFContextReader() const;
        BOOL FNotBFContextWriter() const;
#endif  //  DEBUG

        BOOL FTryEnterBFContextAsReader();
        VOID EnterBFContextAsReader();
        VOID LeaveBFContextAsReader();
        VOID EnterBFContextAsWriter();
        VOID LeaveBFContextAsWriter();

        DWORD_PTR DwBFContext();
        BOOL FBFContext() const;

        // assert corectness of LogOn && ReadOnlyAttach && VersioningOff flags combination
        VOID AssertLRVFlags() const;

        LGPOS LgposWaypoint() const;
        const DATA& DataHeaderSignature() const;

        LONG LGenMaxCommittedAttachedDuringRecovery() const;

        ERR ErrStartDBMScan();
        ERR ErrStartDBMScanSinglePass(
            JET_SESID sesid,
            const INT csecMax,
            const INT cmsecSleep,
            const JET_CALLBACK pfnCallback);

        VOID StopDBMScan();
        BOOL FDBMScanOn();

        ERR ErrCreateDBMScanFollower();
        VOID DestroyDBMScanFollower();
        CDBMScanFollower * PdbmFollower() const;

        CKVPStore * PkvpsMSysLocales() const;

        CFlushMapForAttachedDb * PFlushMap() const;
    
        LIDMAP * Plidmap() const;

        // register non-BF IO operations coming and going
        VOID IncrementAsyncIOForViewCache();
        VOID DecrementAsyncIOForViewCache();
        
        // wait for all pending non-BF IO operations to complete
        VOID WaitForAsyncIOForViewCache();

        PGNO PgnoHighestWriteLatched() const { return m_pgnoHighestWriteLatched; }
        PGNO PgnoDirtiedMax() const { return m_pgnoDirtiedMax; }
        PGNO PgnoWriteLatchedNonScanMax() const { return m_pgnoWriteLatchedNonScanMax; }
        PGNO PgnoLatchedScanMax() const { return m_pgnoLatchedScanMax; }
        PGNO PgnoPreReadScanMax() const { return m_pgnoPreReadScanMax; }
        PGNO PgnoScanMax() const { return m_pgnoScanMax; }

        IFileAPI::FileModeFlags FmfDbDefault() const;

        // Runtime database parameters.
        //
        VOID SetRuntimeDbParams( const CPG cpgDatabaseSizeMax,
                                    const ULONG_PTR pctCachePriority,
                                    const JET_GRBIT grbitShrinkDatabaseOptions,
                                    const LONG dtickShrinkDatabaseTimeQuota,
                                    const CPG cpgShrinkDatabaseSizeLimit,
                                    const BOOL fLeakReclaimerEnabled,
                                    const LONG dtickLeakReclaimerTimeQuota );

        // Max DB size.
        UINT CpgDatabaseSizeMax() const;
        VOID SetDatabaseSizeMax( CPG cpg );
        // Cache priority.
        WORD PctCachePriorityFmp() const;
        VOID SetPctCachePriorityFmp( const ULONG_PTR pctCachePriority );
        // Shrink.
        VOID SetShrinkDatabaseOptions( const JET_GRBIT grbitShrinkDatabaseOptions );
        VOID SetShrinkDatabaseTimeQuota( const LONG dtickShrinkDatabaseTimeQuota );
        VOID SetShrinkDatabaseSizeLimit( const CPG cpgShrinkDatabaseSizeLimit );
        BOOL FShrinkDatabaseEofOnAttach() const;
        BOOL FRunShrinkDatabaseFullCatOnAttach() const;
        BOOL FShrinkDatabaseDontMoveRootsOnAttach() const;
        BOOL FShrinkDatabaseDontTruncateLeakedPagesOnAttach() const;
        BOOL FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() const;
        LONG DtickShrinkDatabaseTimeQuota() const;
        CPG CpgShrinkDatabaseSizeLimit() const;
        // Leak reclaimer.
        VOID SetLeakReclaimerEnabled( const BOOL fLeakReclaimerEnabled );
        VOID SetLeakReclaimerTimeQuota( const LONG dtickLeakReclaimerTimeQuota );
        BOOL FLeakReclaimerEnabled() const;
        LONG DtickLeakReclaimerTimeQuota() const;

        // Redo maps.
        //
        CLogRedoMap* PLogRedoMapZeroed() const        { return m_pLogRedoMapZeroed; };
        CLogRedoMap* PLogRedoMapBadDbTime() const     { return m_pLogRedoMapBadDbtime; };
        CLogRedoMap* PLogRedoMapDbtimeRevert() const  { return m_pLogRedoMapDbtimeRevert; };

    // =====================================================================
    // Member manipulation.
    public:

        VOID SetWszDatabaseName( __in PWSTR wsz );
        VOID IncCPin();
        VOID DecCPin();
        VOID SetCPin( INT c );
        VOID SetPpibWriteLatch( PIB * ppib);
        VOID SetOwnedFileSize( const QWORD cb );
        VOID SetFsFileSizeAsyncTarget( const QWORD cb );
        VOID SetPfapi( IFileAPI *const pfapi );
        VOID SetDbtimeCurrentDuringRecovery( DBTIME dbtime );
        VOID SetLgposAttach( const LGPOS &lgposAttach );
        VOID SetLgposDetach( const LGPOS &lgposDetach );
        VOID SetDbtimeLast( const DBTIME dbtime );
        VOID SetDbtimeBeginRBS( const DBTIME dbtime );
        DBTIME DbtimeGet();
        DBTIME DbtimeIncrementAndGet();
        VOID SetDbtimeOldestGuaranteed( const DBTIME dbtime );
        VOID SetDbtimeOldestCandidate( const DBTIME dbtime );
        VOID SetDbtimeOldestTarget( const DBTIME dbtime );
        VOID SetTrxOldestCandidate( const TRX trx );
        VOID SetTrxOldestTarget( const TRX trx );
        VOID InitializeDbtimeOldest();
        VOID UpdateDbtimeOldest();
        VOID SetTrxNewestWhenDiscardsLastReported( const TRX trx );
        ERR ErrSetPdbfilehdr( DBFILEHDR_FIX * pdbfilehdr, __out DBFILEHDR ** ppdbfilehdr );
        VOID SetPpatchhdr( PATCH_HEADER_PAGE * ppatchhdr );
        VOID SetPpibExclusiveOpen( PIB * ppib);
        VOID SetPgnoBackupMost( PGNO pgno );
        VOID SetPgnoBackupCopyMost( PGNO pgno );
        VOID SetPgnoSnapBackupMost( PGNO pgno );
        VOID SetPatchchk( ATCHCHK * patchchk );
        VOID SetPatchchkRestored( ATCHCHK * patchchk );
        VOID SetObjidLast( OBJID objid );
        ERR ErrObjidLastIncrementAndGet( OBJID *pobjid );
        VOID SetPinst( INST *pinst );
        VOID SetDbid( DBID dbid );
        VOID SetDwBFContext( DWORD_PTR dwBFContext );

        DBTIME  DbtimeLastScrub() const;
        VOID    SetDbtimeLastScrub( const DBTIME dbtimeLastScrub );
        LOGTIME LogtimeScrub() const;
        VOID    SetLogtimeScrub( const LOGTIME& logtime );

    private:
        VOID SetILgposWaypoint( const LGPOS &lgposWaypoint );
    public:
        VOID SetWaypoint( LONG lGenWaypoint );
        VOID ResetWaypoint( VOID );
        VOID SetDataHeaderSignature( DBFILEHDR_FIX* const pdbhdrsig, const ULONG cbdbhdrsig );
        VOID SetLGenMaxCommittedAttachedDuringRecovery( const LONG lGenMaxCommittedAttachedDuringRecovery );
        VOID SetKVPMSysLocales( CKVPStore * const pkvpsMSysLocales );
        VOID UpdatePgnoHighestWriteLatched( const PGNO pgnoHighestCandidate );
        VOID UpdatePgnoDirtiedMax( const PGNO pgnoHighestCandidate );
        VOID UpdatePgnoWriteLatchedNonScanMax( const PGNO pgnoHighestCandidate );
        VOID UpdatePgnoLatchedScanMax( const PGNO pgnoHighestCandidate );
        VOID UpdatePgnoPreReadScanMax( const PGNO pgnoHighestCandidate );
        VOID UpdatePgnoScanMax( const PGNO pgnoHighestCandidate );
        VOID ResetPgnoMaxTracking( const PGNO pgnoReq );

        VOID SetLidmap( LIDMAP * const plidmap );

#ifdef DEBUG
        BOOL FHeaderUpdatePending() const;
        BOOL FHeaderUpdateInProgress() const;
        VOID SetDbHeaderUpdateState( _In_ const DbHeaderUpdateState dbhusNewState );
#endif
    private:
        VOID ResetPctCachePriorityFmp();

    // =====================================================================
    // Flags fields
    public:

        VOID ResetFlags();
        VOID SetCrefWriteLatch( UINT cref );

        VOID SetCreatingDB();
        VOID ResetCreatingDB();
        VOID SetAttachingDB();
        VOID ResetAttachingDB();
        VOID SetDetachingDB();
        VOID ResetDetachingDB();
        VOID SetNoWaypointLatency();
        VOID ResetNoWaypointLatency();
        VOID SetAttachedForRecovery();
        VOID ResetAttachedForRecovery();
        VOID SetRecoveryChecksDone();
        VOID ResetRecoveryChecksDone();
        VOID SetAllowHeaderUpdate();
        VOID ResetAllowHeaderUpdate();
        VOID SetExclusiveOpen( PIB *ppib );
        VOID ResetExclusiveOpen( );
        VOID SetReadOnlyAttach();
        VOID ResetReadOnlyAttach();
        VOID SetLogOn();
        VOID ResetLogOn();
        VOID SetVersioningOff();
        VOID ResetVersioningOff();
        VOID SetSkippedAttach();
        VOID ResetSkippedAttach();
        VOID SetAttached();
        VOID ResetAttached();
        VOID SetDeferredAttach( const bool fDeferredAttachConsistentFuture, const bool fDeferredForAccessDenied );
        VOID ResetDeferredAttach();
        VOID SetIgnoreDeferredAttach();
        VOID ResetIgnoreDeferredAttach();
        VOID SetFailFastDeferredAttach();
        VOID ResetFailFastDeferredAttach();
        VOID SetOverwriteOnCreate();
        VOID ResetOverwriteOnCreate();
        VOID SetRunningOLD();
        VOID ResetRunningOLD();
        VOID SetInBackupSession();
        VOID ResetInBackupSession();
        VOID SetOlderDemandExtendDb();
        VOID ResetOlderDemandExtendDb();
        VOID SetScheduledPeriodicTrim();
        VOID ResetScheduledPeriodicTrim();
        VOID SetTrimSupported();
        VOID ResetTrimSupported();
        VOID SetPiostats( CIoStats * const piostatsDbRead, CIoStats * const piostatsDbWrite );
        VOID FreeIostats();
        VOID SetRBSOn();
        VOID ResetRBSOn();
        VOID SetNeedUpdateDbtimeBeginRBS();
        VOID ResetNeedUpdateDbtimeBeginRBS();

        BOOL FWriteLatchByOther( PIB *ppib );
        BOOL FWriteLatchByMe( PIB *ppib );
        VOID SetWriteLatch( PIB * ppib );
        VOID ResetWriteLatch( PIB * ppib );
        BOOL FExclusiveByAnotherSession( PIB *ppib );
        BOOL FExclusiveBySession( PIB *ppib );

        BOOL FDefragPreserveOriginal()      { return m_fDefragPreserveOriginal; }
        VOID SetDefragPreserveOriginal()    { m_fDefragPreserveOriginal = fTrue; }
        VOID ResetDefragPreserveOriginal()  { m_fDefragPreserveOriginal = fFalse; }

        BOOL FCopiedPatchHeader() const     { return m_fCopiedPatchHeader; }
        VOID SetFCopiedPatchHeader()        { m_fCopiedPatchHeader = fTrue; }
        VOID ResetFCopiedPatchHeader()      { m_fCopiedPatchHeader = fFalse; }

        BOOL FEDBBackupDone() const         { return m_fEDBBackupDone; }
        VOID SetEDBBackupDone()             { m_fEDBBackupDone = fTrue; }
        VOID ResetEDBBackupDone()           { m_fEDBBackupDone = fFalse; }

        BOOL FDontRegisterOLD2Tasks() const { return m_fDontRegisterOLD2Tasks; }
        VOID SetFDontRegisterOLD2Tasks()    { m_fDontRegisterOLD2Tasks = fTrue; }
        VOID ResetFDontRegisterOLD2Tasks()  { m_fDontRegisterOLD2Tasks = fFalse; }

        BOOL FDontStartDBM() const  { return m_fDontStartDBM; }
        VOID SetFDontStartDBM() { m_fDontStartDBM = fTrue; }
        VOID ResetFDontStartDBM()   { m_fDontStartDBM = fFalse; }

        BOOL FDontStartOLD() const  { return m_fDontStartOLD; }
        VOID SetFDontStartOLD() { m_fDontStartOLD = fTrue; }
        VOID ResetFDontStartOLD()   { m_fDontStartOLD = fFalse; }

        BOOL FDontStartTrimTask() const     { return m_fDontStartTrimTask; }
        VOID SetFDontStartTrimTask()        { m_fDontStartTrimTask = fTrue; }
        VOID ResetFDontStartTrimTask()      { m_fDontStartTrimTask = fFalse; }

        BOOL FCacheAvail() const                    { return m_fCacheAvail; }
        VOID SetFCacheAvail()                   { m_fCacheAvail = fTrue; }
        VOID ResetFCacheAvail()                 { m_fCacheAvail = fFalse; }

        BOOL FMaintainMSObjids() const      { return m_fMaintainMSObjids; }
        VOID SetFMaintainMSObjids()         { m_fMaintainMSObjids = fTrue; }
        VOID ResetFMaintainMSObjids()       { m_fMaintainMSObjids = fFalse; }

        BOOL FBackupFileCopyDone() const    { return FEDBBackupDone(); }

        BOOL FHardRecovery( const DBFILEHDR* const pdbfilehdr ) const;
        BOOL FHardRecovery() const;


    // =====================================================================
    // Physical File I/O Conversions
    public:

       CPG CpgOfCb( const QWORD cb ) const        { return (CPG) ( cb / CbPage() ); }
       QWORD CbOfCpg( const CPG cpg ) const       { return ( QWORD( cpg ) * CbPage() ); }

    // =====================================================================
    // Initialize/terminate FMP array
    public:

        static ERR ErrFMPInit();
        static VOID Term();

        // these are used by testing routines that need a mock FMP
        static FMP * PfmpCreateMockFMP();
        static void FreeMockFMP(FMP * const pfmp);

    // =====================================================================
    // FMP management
    public:
        ERR ErrStoreDbName(
            const INST * const          pinst,
            IFileSystemAPI * const      pfsapi,
            const WCHAR *               wszDatabaseName,
            const BOOL                  fValidatePaths );

        ERR ErrCreateFlushMap( const JET_GRBIT grbit );
        VOID DestroyFlushMap();

        VOID FreePdbfilehdr();

        static ERR ErrWriteLatchByNameWsz( const WCHAR *wszFileName, IFMP *pifmp, PIB *ppib );
        static ERR ErrWriteLatchByIfmp( IFMP ifmp, PIB *ppib );
        static ERR ErrResolveByNameWsz( const _In_z_ WCHAR *wszDatabaseName, _Out_writes_z_( cwchFullName ) WCHAR *wszFullName, const size_t cwchFullName );
        static ERR ErrNewAndWriteLatch( _Out_ IFMP                      *pifmp,
                                        _In_z_ const WCHAR              *wszDatabaseName,
                                        _In_ PIB                        *ppib,
                                        _In_ INST                       *pinst,
                                        _In_ IFileSystemAPI * const     pfsapi,
                                        _In_ DBID                       dbidGiven,
                                        _In_ const BOOL                 fForcePurgeCache,
                                        _In_ const BOOL                 fCheckForAttachInfoOverflow,
                                        _Out_opt_ BOOL * const          pfCacheAlive );
    private:
        static ERR ErrInitializeOneFmp(
            _In_ INST* const pinst,
            _In_ PIB* const ppib,
            _In_ const IFMP ifmp,
            _In_ const DBID dbidGiven,
            _In_z_ const WCHAR* wszDatabaseName,
            _In_ IFileSystemAPI * const pfsapi,
            _In_ const BOOL fForcePurgeCache,
            _In_ const BOOL fDeferredAttach );
    
        static ERR ErrNewOneFmp( const IFMP ifmp );

    public:
        static BOOL FAllocatedFmp( IFMP ifmp );
        static BOOL FAllocatedFmp( const FMP * const pfmp );
        static IFMP IfmpMinInUse();
        static IFMP IfmpMacInUse();
        static VOID AssertVALIDIFMP( IFMP ifmp );

        BOOL FIsTempDB() const;

        VOID ReleaseWriteLatchAndFree( PIB *ppib );

#ifdef DEBUG
        BOOL FIOSizeChangeLatchAvail();
        VOID AssertSafeToChangeOwnedSize();
        VOID AssertSizesConsistent( const BOOL fHasIoSizeLockDebugOnly );
#endif // DEBUG
        VOID AcquireIOSizeChangeLatch();
        VOID ReleaseIOSizeChangeLatch();

        VOID GetPeriodicTrimmingDBLatch( );
        VOID ReleasePeriodicTrimmingDBLatch( );

        // FMPGetWriteLatch( IFMP ifmp, PIB *ppib )
        VOID GetWriteLatch( PIB *ppib );
        // FMPReleaseWriteLatch( IFMP ifmp, PIB *ppib )
        VOID ReleaseWriteLatch( PIB *ppib );

        //  used when DBTASK's are created/destroyed on this database
        ERR RegisterTask();
        ERR UnregisterTask();
        //  wait for all tasks to complete
        VOID WaitForTasksToComplete();

    private:
        ERR ErrIAddRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd );
        VOID IRemoveRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd );

    public:
        ERR ErrRangeLock( const PGNO pgnoStart, const PGNO pgnoEnd );
        VOID RangeUnlock( const PGNO pgnoStart, const PGNO pgnoEnd );

        VOID AssertRangeLockEmpty() const;
        BOOL FEnterRangeLock( const PGNO pgno, CMeteredSection::Group* const pirangelock );
        VOID LeaveRangeLock( const INT irangelock );
        VOID LeaveRangeLock( const PGNO pgnoDebugOnly, const INT irangelock );

        ERR ErrRangeLockAndEnter( const PGNO pgnoStart, const PGNO pgnoEnd, CMeteredSection::Group* const pirangelock );
        VOID RangeUnlockAndLeave( const PGNO pgnoStart, const PGNO pgnoEnd, const CMeteredSection::Group irangelock );

        ERR ErrDBReadPages(     const PGNO pgnoFirst,
                                VOID* const pvPageFirst,
                                const CPG cpg,
                                CPG* const pcpgActual,
                                const TraceContext& tc,
                                const BOOL fExtensiveChecks = fFalse );

        void ResetEventThrottles()
        {
            m_lesEventDbFeatureDisabled.Reset();
            m_lesEventBtSkippedNodesDisk.Reset();
            m_lesEventBtSkippedNodesCpu.Reset();
        }
        BOOL FCheckTreeSkippedNodesDisk( const OBJID objidFDP )
        {
            return m_lesEventBtSkippedNodesDisk.FNeedToLog( objidFDP );
        }
        BOOL FCheckTreeSkippedNodesCpu( const OBJID objidFDP )
        {
            return m_lesEventBtSkippedNodesCpu.FNeedToLog( objidFDP );
        }

        void TraceStationId( const TraceStationIdentificationReason tsidr );
        void TraceDbfilehdrInfo( const TraceStationIdentificationReason tsidr );

        // Redo maps.
        //
        static BOOL FPendingRedoMapEntries();
        ERR ErrEnsureLogRedoMapsAllocated();
        VOID FreeLogRedoMaps( const BOOL fAllocCleanup = fFalse );
        BOOL FRedoMapsEmpty();

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif  //  DEBUGGER_EXTENSION

    // =====================================================================
    // FMP management
    private:
        static CReaderWriterLock rwlFMPPool;
    public:
        static void EnterFMPPoolAsReader() { rwlFMPPool.EnterAsReader(); }
        static void LeaveFMPPoolAsReader() { rwlFMPPool.LeaveAsReader(); }
        static void EnterFMPPoolAsWriter() { rwlFMPPool.EnterAsWriter(); }
        static void LeaveFMPPoolAsWriter() { rwlFMPPool.LeaveAsWriter(); }
#ifdef DEBUG
        static BOOL FWriterFMPPool() { return rwlFMPPool.FWriter(); }
        static BOOL FReaderFMPPool() { return rwlFMPPool.FReader(); }
#endif  //  DEBUG
        VOID FreeHeaderSignature();
        VOID SnapshotHeaderSignature();

    CPG CpgAvail() const
    {
        Assert( FCacheAvail() );
        return m_cpgAvail;
    }
    
    VOID SetCpgAvail( CPG cpgAvail )
    {
        m_cpgAvail = cpgAvail;
        SetFCacheAvail();
    }
    
    VOID ResetCpgAvail()
    {
        ResetFCacheAvail();
        m_cpgAvail = 0;
    }
    
    void AdjustCpgAvail( LONG lAvailAdd )
    {
        Assert( FCacheAvail() );
        (void)AtomicExchangeAdd( &m_cpgAvail, lAvailAdd );
    }

    VOID SetPgnoShrinkTarget( const PGNO pgnoShrinkTarget )
    {
        Assert( m_fShrinkIsRunning );
        Assert( pgnoShrinkTarget != pgnoNull );
        Expected( !FPgnoShrinkTargetIsSet() || ( pgnoShrinkTarget <= m_pgnoShrinkTarget )  );
        m_pgnoShrinkTarget = pgnoShrinkTarget;
    }

    VOID ResetPgnoShrinkTarget()
    {
        m_pgnoShrinkTarget = pgnoNull;
    }

    BOOL FPgnoShrinkTargetIsSet() const
    {
        return ( m_pgnoShrinkTarget != pgnoNull );
    }

    BOOL FBeyondPgnoShrinkTarget( const PGNO pgnoFirst, const CPG cpg = 1 ) const
    {
        Assert( !FPgnoShrinkTargetIsSet() || m_fShrinkIsRunning );

        if ( !m_fShrinkIsRunning )
        {
            return fFalse;
        }
        
        if ( !FPgnoShrinkTargetIsSet() || ( pgnoFirst == pgnoNull )  )
        {
            return fFalse;
        }

        if ( cpg == 0 )
        {
            return ( pgnoFirst > m_pgnoShrinkTarget );
        }

        return ( ( pgnoFirst + cpg - 1 ) > m_pgnoShrinkTarget );
    }

    VOID SetShrinkIsRunning()
    {
        Assert( !FPgnoShrinkTargetIsSet() );
        m_fShrinkIsRunning = fTrue;
    }

    VOID ResetShrinkIsRunning()
    {
        Assert( !FPgnoShrinkTargetIsSet() );
        m_fShrinkIsRunning = fFalse;
    }

    BOOL FShrinkIsRunning() const
    {
        Assert( !FPgnoShrinkTargetIsSet() || m_fShrinkIsRunning );
        return m_fShrinkIsRunning;
    }

    BOOL FShrinkIsActive() const
    {
        return ( FShrinkIsRunning() && FPgnoShrinkTargetIsSet() );
    }

    VOID SetLeakReclaimerIsRunning()
    {
        m_fLeakReclaimerIsRunning = fTrue;
    }

    VOID ResetLeakReclaimerIsRunning()
    {
        m_fLeakReclaimerIsRunning = fFalse;
    }

    BOOL FLeakReclaimerIsRunning() const
    {
        return m_fLeakReclaimerIsRunning;
    }

    ERR ErrPgnoLastFileSystem( PGNO* const ppgnoLast ) const;

    BOOL FPgnoInZeroedOrRevertedMaps( const PGNO pgno ) const;

    friend DBFILEHDR * PdbfilehdrEDBGAccessor( const FMP * const pfmp );

}; // class FMP


// =====================================================================
// Member retrieval.

INLINE WCHAR * FMP::WszDatabaseName() const     { return m_wszDatabaseName; }
INLINE BOOL FMP::FInUse() const                 { Assert( FMP::FAllocatedFmp( this ) ); return NULL != WszDatabaseName(); }
INLINE CCriticalSection& FMP::CritLatch()       { return m_critLatch; }
INLINE CCriticalSection& FMP::CritOpenDbCheck() { return m_critOpenDbCheck; }
INLINE CSemaphore& FMP::SemIOSizeChange()       { return m_semIOSizeChange; }
INLINE CSemaphore& FMP::SemPeriodicTrimmingDB() { return m_semTrimmingDB; }
INLINE CSemaphore& FMP::SemDBM()                { return m_semDBM; }
INLINE CGate& FMP::GateWriteLatch()             { return m_gateWriteLatch; }
INLINE UINT FMP::CPin() const                   { return m_cPin; }
INLINE PIB * FMP::PpibWriteLatch() const        { return m_ppibWriteLatch; }

// =====================================================================
//
// FMP file size helpers:
//
//  - FMP::CbOwnedFileSize() / m_cbOwnedFileSize: size, in bytes, of the tracked
//    owned database size, including the initial reserved pages. Note that this
//    variable is not guaranteed to be in perfect sync with the last page owned
//    by the database as represented in the root OE during recovery redo. There
//    are four different phases/points in which this variable is maintained:
//
//      o During recovery/redo: since redo is not aware of the logical meaning
//        of the operations it replays, this value is an aproximation based on
//        file size changes, which happen by replaying ExtendDB or ShrinkDB log
//        records.
//
//      o At the end of recovery/redo: once we have clean databases, we then read
//        the actual last owned page from the OE and set to this variable.
//
//      o At DB attachment: the precise value is read from the OE and set to this
//        variable.
//
//      o During do-time: this variable is updated upon addition or removal of a
//        the last node in the DB root's OE tree.
//            - When we're growing: it is updated after we successfully grow the
//              DB root's owned space, i.e., after a new OE node is appended. It
//              therefore happens after we have grown the physical file.
//            - When we're shrinking: it is updated before we reduce the
//              DB root's owned space, i.e., before the last OE node is removed
//              during a DB shrink operation. It therefore happens before the
//              physical file is truncated.
//
//
//  - FMP::CbFsFileSizeAsyncTarget() / m_cbFsFileSizeAsyncTarget: current target
//    database size in bytes (physical file system size). If there is no pending
//    async extension, this value matches the physical file size. When shrink
//    truncates the file, this value is set to the new size.
//
//
//  - FMP::PgnoLast(): page number of the last tracked owned page of the database.
//    This is calculated off m_cbOwnedFileSize, so it's subject to the limitations
//    described above for databases undergoing recovery redo.
//
//
//  - EOF / EOF target: refer to the end of the physical database file and its
//    target, respectively.
//
//
//    ib = 0                                                              EOF    EOF target
//      |                                                                  |        |
//      --------------------------------------------------------------------
//      |  pgno -1  |  pgno 0  |  pgno 1  |  ...  |  PgnoLast()  |         |........|
//      |  (DbHDr)  | (ShdHdr) |          |       |              |         |        |
//      --------------------------------------------------------------------
//      [--- cpgDBReserved ----]
//      [------------------ FMP::CbOwnedFileSize() --------------]
//      [-------------------- FMP::m_pfapi->ErrSize( &cb ) ----------------]
//      [----------------------- FMP::CbFsFileSizeAsyncTarget() --------------------]
//
INLINE QWORD FMP::CbOwnedFileSize() const       { return (QWORD)AtomicRead( (__int64*)&m_cbOwnedFileSize ); }
INLINE QWORD FMP::CbFsFileSizeAsyncTarget() const { return (QWORD)AtomicRead( (__int64*)&m_cbFsFileSizeAsyncTarget ); }

INLINE IFileAPI *const FMP::Pfapi() const       { return m_pfapi; }
INLINE UINT FMP::CrefWriteLatch() const         { return m_crefWriteLatch; }
INLINE UINT FMP::FCreatingDB() const            { return m_fCreatingDB; }
INLINE UINT FMP::FAttachingDB() const           { return m_fAttachingDB; }
INLINE UINT FMP::FDetachingDB() const           { return m_fDetachingDB; }
INLINE UINT FMP::FNoWaypointLatency() const     { return m_fNoWaypointLatency; }
INLINE UINT FMP::FAttachedForRecovery() const   { return m_fAttachedForRecovery; }
INLINE UINT FMP::FRecoveryChecksDone() const    { return m_fRecoveryChecksDone; }
INLINE UINT FMP::FAllowHeaderUpdate() const     { return m_fAllowHeaderUpdate; }
INLINE UINT FMP::FExclusiveOpen() const         { return m_fExclusiveOpen; }
INLINE UINT FMP::FReadOnlyAttach() const        { AssertLRVFlags(); return m_fReadOnlyAttach; }
INLINE UINT FMP::FLogOn() const
    { AssertLRVFlags(); Assert( !m_fLogOn || !m_pinst->m_plog->FLogDisabled() ); return m_fLogOn; }
INLINE UINT FMP::FVersioningOff() const         { AssertLRVFlags(); return m_fVersioningOff; }
#ifdef DEBUG
INLINE VOID FMP::AssertLRVFlags() const
    { Assert( !m_fLogOn || !( m_fVersioningOff || m_fReadOnlyAttach ) );
      Assert( !m_fReadOnlyAttach || m_fVersioningOff );
}
#else // DEBUG
INLINE VOID FMP::AssertLRVFlags() const         {}
#endif // DEBUG
INLINE UINT FMP::FShadowingOff() const          { return Pdbfilehdr()->FShadowingDisabled(); }
INLINE UINT FMP::FSkippedAttach() const         { return m_fSkippedAttach; }
INLINE UINT FMP::FAttached() const              { return m_fAttached; }
INLINE UINT FMP::FDeferredAttach() const        { return m_fDeferredAttach; }
INLINE UINT FMP::FDeferredAttachConsistentFuture() const    { return m_fDeferredAttachConsistentFuture; }
INLINE UINT FMP::FDeferredForAccessDenied() const   { return m_fDeferredForAccessDenied; }
INLINE UINT FMP::FIgnoreDeferredAttach() const  { return m_fIgnoreDeferredAttach; }
INLINE UINT FMP::FFailFastDeferredAttach() const { return m_fFailFastDeferredAttach; }
INLINE UINT FMP::FOverwriteOnCreate() const     { return m_fOverwriteOnCreate; }
INLINE UINT FMP::FRunningOLD() const            { return m_fRunningOLD; }
INLINE UINT FMP::FInBackupSession() const       { return m_fInBackupSession; }
INLINE UINT FMP::FOlderDemandExtendDb() const
{
    //  During do-time, it should only be used in Assert()s, i.e., do not depend on it for
    //  controlling actual behavior, so we're going to enforce in do-time retail.
    //  I do not believe we fix the size at the end of recovery redo on an older log, so you
    //  can create a situation where recovery runs onto new log data, but started and did
    //  the extend via the older method - creating a demand extend DB.
#ifndef DEBUG
    DBEnforceSz( Ifmp(), FAttachedForRecovery(), "OlderDemandExtDbDoTime" );
#endif

    return m_fOlderDemandExtendDb;
}
INLINE UINT FMP::FRBSOn() const                 { return m_fRBSOn; }
INLINE UINT FMP::FNeedUpdateDbtimeBeginRBS() const { return m_fNeedUpdateDbtimeBeginRBS; }

INLINE DBTIME FMP::DbtimeCurrentDuringRecovery() const  { return m_dbtimeCurrentDuringRecovery; }
INLINE LGPOS* FMP::PlgposAttach()               { return &m_lgposAttach; }
INLINE const LGPOS &FMP::LgposAttach() const    { return m_lgposAttach; }
INLINE LGPOS* FMP::PlgposDetach()               { return &m_lgposDetach; }
INLINE const LGPOS &FMP::LgposDetach() const    { return m_lgposDetach; }
INLINE DBTIME FMP::DbtimeLast() const           { return m_dbtimeLast; }
INLINE DBTIME FMP::DbtimeBeginRBS() const  { return m_dbtimeBeginRBS; }
INLINE DBTIME FMP::DbtimeOldestGuaranteed() const
{
    if ( FOS64Bit() )
    {
        return m_dbtimeOldestGuaranteed;
    }
    else
    {
        const QWORDX * const    pqw     = (QWORDX *)&m_dbtimeOldestGuaranteed;
        return DBTIME( pqw->QwHighLow() );
    }
}
INLINE DBTIME FMP::DbtimeOldestCandidate() const{ return m_dbtimeOldestCandidate; }
INLINE DBTIME FMP::DbtimeOldestTarget() const   { return m_dbtimeOldestTarget; }
INLINE TRX FMP::TrxOldestCandidate() const      { return m_trxOldestCandidate; }
INLINE TRX FMP::TrxOldestTarget() const         { return m_trxOldestTarget; }
INLINE TRX FMP::TrxNewestWhenDiscardsLastReported() const   { return m_trxNewestWhenDiscardsLastReported; }
INLINE PATCH_HEADER_PAGE * FMP::Ppatchhdr() const   { return m_ppatchhdr; }
INLINE PIB * FMP::PpibExclusiveOpen() const     { return m_ppibExclusiveOpen; }
INLINE IFileAPI *const FMP::PfapiPatch() const  { return m_pfapiPatch; }
INLINE WCHAR * FMP::WszPatchPath() const        { return m_wszPatchPath; }
INLINE INT FMP::CpagePatch() const              { return m_cpagePatch; }
INLINE ULONG FMP::CPatchIO() const              { return m_cPatchIO; }
INLINE PGNO FMP::PgnoBackupMost() const         { return m_pgnoBackupMost; }
INLINE PGNO FMP::PgnoBackupCopyMost() const     { return m_pgnoBackupCopyMost; }
INLINE PGNO FMP::PgnoSnapBackupMost() rtlconst  { OnDebug( AssertSizesConsistent( fTrue ) ); return m_pgnoSnapBackupMost; }
INLINE ATCHCHK * FMP::Patchchk() const          { return m_patchchk; }
INLINE OBJID FMP::ObjidLast() const             { return m_objidLast; }
INLINE INST * FMP::Pinst() const                { return m_pinst; }
INLINE DBID FMP::Dbid() const                   { return DBID( m_dbid ); }
INLINE CReaderWriterLock& FMP::RwlDetaching()   { return m_rwlDetaching; }
INLINE CReaderWriterLock& FMP::RwlIBFContext()  { return m_rwlBFContext; }
INLINE LONG FMP::CbPage() const                 { return (LONG)UlParam( m_pinst, JET_paramDatabasePageSize ); }
INLINE BOOL FMP::FSmallPageDb() const           { return FIsSmallPage( (LONG)UlParam( m_pinst, JET_paramDatabasePageSize ) ); }
INLINE CRevertSnapshot * FMP::PRBS()            { return m_pinst->m_prbs; }

#ifdef DEBUG
INLINE BOOL FMP::FBFContextReader() const
{
    return m_rwlBFContext.FReader();
}
INLINE BOOL FMP::FNotBFContextWriter() const
{
    BOOL fNotWriter = m_rwlBFContext.FNotWriter();
    Assert( fNotWriter || this == Ptls()->PFMP() );

    return fNotWriter;
}
#endif  //  DEBUG

INLINE BOOL FMP::FTryEnterBFContextAsReader()
{
    BOOL fRet = RwlIBFContext().FTryEnterAsReader();
    if ( fRet )
    {
        Ptls()->SetPFMP( this );
    }

    return fRet;
}
INLINE VOID FMP::EnterBFContextAsReader()
{
    RwlIBFContext().EnterAsReader();
    Ptls()->SetPFMP( this );
}
INLINE VOID FMP::LeaveBFContextAsReader()
{
    RwlIBFContext().LeaveAsReader();
    Ptls()->ResetPFMP( this );
}
INLINE VOID FMP::EnterBFContextAsWriter()
{
    RwlIBFContext().EnterAsWriter();
    Ptls()->SetPFMP( this );
}
INLINE VOID FMP::LeaveBFContextAsWriter()
{
    RwlIBFContext().LeaveAsWriter();
    Ptls()->ResetPFMP( this );
}

INLINE DWORD_PTR FMP::DwBFContext()
{
#ifdef SYNC_DEADLOCK_DETECTION
    Assert( Ptls()->PFMP() == this || RwlIBFContext().FReader() || RwlIBFContext().FWriter() );
#endif  //  SYNC_DEADLOCK_DETECTION

    return m_dwBFContext;
}

INLINE BOOL FMP::FBFContext() const
{
    return NULL != m_dwBFContext;
}

INLINE const DATA& FMP::DataHeaderSignature() const { return m_dataHdrSig; }

INLINE LONG FMP::LGenMaxCommittedAttachedDuringRecovery() const { return m_lGenMaxCommittedAttachedDuringRecovery; }

INLINE CKVPStore * FMP::PkvpsMSysLocales() const { return m_pkvpsMSysLocales; }

INLINE CFlushMapForAttachedDb * FMP::PFlushMap() const  { return m_pflushmap; }

inline IFileAPI::FileModeFlags FMP::FmfDbDefault() const
{
    Assert( m_pinst );

    //  NOTE:  we can remove the !pfmp->FLogOn() condition if we call FlushFileBuffers() before we advance the checkpoint

    IFileAPI::FileModeFlags fmfDefault =
        ( FReadOnlyAttach() ? IFileAPI::fmfReadOnly : IFileAPI::fmfNone ) |
        ( ( !FReadOnlyAttach() && BoolParam( JET_paramEnableFileCache ) && !FLogOn() ) ? IFileAPI::fmfLossyWriteBack : IFileAPI::fmfNone ) |
        ( ( BoolParam( m_pinst, JET_paramUseFlushForWriteDurability ) && !FReadOnlyAttach() ) ? IFileAPI::fmfStorageWriteBack : IFileAPI::fmfNone ) |
        ( BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone );

#ifdef FORCE_STORAGE_WRITEBACK
#ifdef DEBUG
    //  In debug we want to actually test both so drop it in only half the time ... 
    if ( !( fmfDefault & IFileAPI::fmfStorageWriteBack ) && ( ( TickOSTimeCurrent() % 100 ) < 50 ) )
        fmfDefault = fmfDefault | IFileAPI::fmfStorageWriteBack;
#else
    fmfDefault = fmfDefault | IFileAPI::fmfStorageWriteBack;
#endif
    if ( ( fmfDefault & IFileAPI::fmfStorageWriteBack ) &&
            ( fmfDefault & IFileAPI::fmfReadOnly ) )
    {
        //  If you want to force fmfStorageWriteBack over RO instead for testing ... but dangerous for real
        //  clients which may depend upon real RO behavior.
        //fmfDefault = ( fmfDefault & ~IFileAPI::fmfReadOnly ) | IFileAPI::fmfReadOnlyClient;
        fmfDefault = fmfDefault & ~IFileAPI::fmfStorageWriteBack;
    }
#else // !FORCE_STORAGE_WRITEBACK

    //  couple tweaks
    if ( ( fmfDefault & IFileAPI::fmfLossyWriteBack ) ||
            ( fmfDefault & IFileAPI::fmfReadOnly ) )
    {
        //  I am not ready to handle / test storage write back in these modes, so remove that flag
        fmfDefault = fmfDefault & ~IFileAPI::fmfStorageWriteBack;
    }

    if ( fmfDefault & IFileAPI::fmfCached )
    {
        fmfDefault = fmfDefault & ~IFileAPI::fmfStorageWriteBack;
    }

    if ( Dbid() == dbidTemp )
    {
        fmfDefault = fmfDefault & ~IFileAPI::fmfStorageWriteBack;
    }
#endif // FORCE_STORAGE_WRITEBACK

    return fmfDefault;
}

INLINE LIDMAP * FMP::Plidmap() const { return m_plidmap; }

INLINE VOID FMP::SetRuntimeDbParams( const CPG cpgDatabaseSizeMax,
                                        const ULONG_PTR pctCachePriority,
                                        const JET_GRBIT grbitShrinkDatabaseOptions,
                                        const LONG dtickShrinkDatabaseTimeQuota,
                                        const CPG cpgShrinkDatabaseSizeLimit,
                                        const BOOL fLeakReclaimerEnabled,
                                        const LONG dtickLeakReclaimerTimeQuota )
{
    SetDatabaseSizeMax( cpgDatabaseSizeMax );
    SetPctCachePriorityFmp( pctCachePriority );
    SetShrinkDatabaseOptions( grbitShrinkDatabaseOptions );
    SetShrinkDatabaseTimeQuota( dtickShrinkDatabaseTimeQuota );
    SetShrinkDatabaseSizeLimit( cpgShrinkDatabaseSizeLimit );
    SetLeakReclaimerEnabled( fLeakReclaimerEnabled );
    SetLeakReclaimerTimeQuota( dtickLeakReclaimerTimeQuota );
}

INLINE UINT FMP::CpgDatabaseSizeMax() const     { return m_cpgDatabaseSizeMax; }

INLINE VOID FMP::SetDatabaseSizeMax( CPG cpg )      { m_cpgDatabaseSizeMax = cpg; }

INLINE VOID FMP::SetPctCachePriorityFmp( const ULONG_PTR pctCachePriority )
{
    Assert( FIsCachePriorityValid( pctCachePriority ) || !FIsCachePriorityAssigned( pctCachePriority ) );
    m_pctCachePriority = (WORD)pctCachePriority;
    Assert( FIsCachePriorityValid( m_pctCachePriority ) || !FIsCachePriorityAssigned( m_pctCachePriority ) );

    //  Resolve cache priority back to the sessions.
    m_pinst->m_critPIB.Enter();
    for ( PIB* ppibCurr = m_pinst->m_ppibGlobal; ppibCurr != NULL; ppibCurr = ppibCurr->ppibNext )
    {
        ppibCurr->ResolveCachePriorityForDb( Dbid() );
    }
    m_pinst->m_critPIB.Leave();
}

INLINE WORD FMP::PctCachePriorityFmp() const
{
    return m_pctCachePriority;
}

INLINE VOID FMP::SetShrinkDatabaseOptions( const JET_GRBIT grbitShrinkDatabaseOptions )
{
    m_grbitShrinkDatabaseOptions = grbitShrinkDatabaseOptions;
}

INLINE VOID FMP::SetShrinkDatabaseTimeQuota( const LONG dtickShrinkDatabaseTimeQuota )
{
    m_dtickShrinkDatabaseTimeQuota = dtickShrinkDatabaseTimeQuota;
}

INLINE VOID FMP::SetShrinkDatabaseSizeLimit( const CPG cpgShrinkDatabaseSizeLimit )
{
    m_cpgShrinkDatabaseSizeLimit = cpgShrinkDatabaseSizeLimit;
}

INLINE BOOL FMP::FShrinkDatabaseEofOnAttach() const
{
    return ( m_grbitShrinkDatabaseOptions & JET_bitShrinkDatabaseEofOnAttach ) != 0;
}

INLINE BOOL FMP::FRunShrinkDatabaseFullCatOnAttach() const
{
    return ( m_grbitShrinkDatabaseOptions & JET_bitShrinkDatabaseFullCategorizationOnAttach ) != 0;
}

INLINE BOOL FMP::FShrinkDatabaseDontMoveRootsOnAttach() const
{
    return ( m_grbitShrinkDatabaseOptions & JET_bitShrinkDatabaseDontMoveRootsOnAttach ) != 0;
}

INLINE BOOL FMP::FShrinkDatabaseDontTruncateLeakedPagesOnAttach() const
{
    return ( m_grbitShrinkDatabaseOptions & JET_bitShrinkDatabaseDontTruncateLeakedPagesOnAttach ) != 0;
}

INLINE BOOL FMP::FShrinkDatabaseDontTruncateIndeterminatePagesOnAttach() const
{
    return ( m_grbitShrinkDatabaseOptions & JET_bitShrinkDatabaseDontTruncateIndeterminatePagesOnAttach ) != 0;
}

INLINE LONG FMP::DtickShrinkDatabaseTimeQuota() const
{
    return m_dtickShrinkDatabaseTimeQuota;
}

INLINE CPG FMP::CpgShrinkDatabaseSizeLimit() const
{
    return m_cpgShrinkDatabaseSizeLimit;
}

INLINE VOID FMP::SetLeakReclaimerEnabled( const BOOL fLeakReclaimerEnabled )
{
    m_fLeakReclaimerEnabled = fLeakReclaimerEnabled;
}

INLINE VOID FMP::SetLeakReclaimerTimeQuota( const LONG dtickLeakReclaimerTimeQuota )
{
    m_dtickLeakReclaimerTimeQuota = dtickLeakReclaimerTimeQuota;
}

INLINE BOOL FMP::FLeakReclaimerEnabled() const
{
    return m_fLeakReclaimerEnabled;
}

INLINE LONG FMP::DtickLeakReclaimerTimeQuota() const
{
    return m_dtickLeakReclaimerTimeQuota;
}

// =====================================================================
// Member manipulation.

INLINE VOID FMP::SetWszDatabaseName( __in PWSTR wsz )
{
    Assert( FMP::FWriterFMPPool() );
    m_wszDatabaseName = wsz;
}
INLINE VOID FMP::IncCPin( )                         { m_cPin++; }
INLINE VOID FMP::DecCPin( )                         { Assert( m_cPin > 0 ); m_cPin--; }
INLINE VOID FMP::SetCPin( INT c )                   { m_cPin = c; }
INLINE VOID FMP::SetPpibWriteLatch( PIB * ppib)     { m_ppibWriteLatch = ppib; }
INLINE VOID FMP::SetFsFileSizeAsyncTarget( const QWORD cb )
{
    // Check that we "own" this latch.
    Assert( !FIOSizeChangeLatchAvail() );
    (VOID)AtomicExchange( (__int64*)&m_cbFsFileSizeAsyncTarget, (__int64)cb );
}
INLINE VOID FMP::SetPfapi( IFileAPI *const pfapi ) { m_pfapi = pfapi; }
INLINE VOID FMP::SetDbtimeCurrentDuringRecovery( DBTIME dbtime )    { m_dbtimeCurrentDuringRecovery = dbtime; }
INLINE VOID FMP::SetLgposAttach( const LGPOS &lgposAttach ) { m_lgposAttach = lgposAttach; }
INLINE VOID FMP::SetLgposDetach( const LGPOS &lgposDetach ) { m_lgposDetach = lgposDetach; }
INLINE VOID FMP::SetDbtimeLast( const DBTIME dbtime )       { m_dbtimeLast = dbtime; }
INLINE VOID FMP::SetDbtimeBeginRBS( const DBTIME dbtime ){ m_dbtimeBeginRBS = dbtime; }
INLINE DBTIME FMP::DbtimeGet()
{
    ENTERCRITICALSECTION    enterCritFMP( &m_critDbtime, !FOS64Bit() );
    return m_dbtimeLast;
}
INLINE DBTIME FMP::DbtimeIncrementAndGet()
{
    //  UNDONE: check for JET_errOutOfDbtimeValues

    if ( FOS64Bit() )
    {
        return DBTIME( AtomicExchangeAddPointer( (VOID **)&m_dbtimeLast, (VOID *)1 ) ) + 1;
    }
    else
    {
        ENTERCRITICALSECTION    enterCritFMP( &m_critDbtime );
        return ++m_dbtimeLast;
    }
}
INLINE VOID FMP::SetDbtimeOldestGuaranteed( const DBTIME dbtime )
{
    if ( FOS64Bit() )
    {
        m_dbtimeOldestGuaranteed = dbtime;
    }
    else
    {
        QWORDX * const  pqw     = (QWORDX *)&m_dbtimeOldestGuaranteed;
        pqw->SetQwLowHigh( (QWORD)dbtime );
    }
}
INLINE VOID FMP::SetDbtimeOldestCandidate( const DBTIME dbtime )    { m_dbtimeOldestCandidate = dbtime; }
INLINE VOID FMP::SetDbtimeOldestTarget( const DBTIME dbtime )   { m_dbtimeOldestTarget = dbtime; }
INLINE VOID FMP::SetTrxOldestCandidate( const TRX trx )         { m_trxOldestCandidate = trx; }
INLINE VOID FMP::SetTrxOldestTarget( const TRX trx )            { m_trxOldestTarget = trx; }
INLINE VOID FMP::SetTrxNewestWhenDiscardsLastReported( const TRX trx )      { m_trxNewestWhenDiscardsLastReported = trx; }
INLINE VOID FMP::SetPpatchhdr( PATCH_HEADER_PAGE * ppatchhdr ) { m_ppatchhdr = ppatchhdr; }
INLINE VOID FMP::SetPpibExclusiveOpen( PIB * ppib)  { m_ppibExclusiveOpen = ppib; }

INLINE VOID FMP::SetPgnoBackupMost( PGNO pgno )     { Assert( m_critLatch.FOwner() ); m_pgnoBackupMost = pgno; }
INLINE VOID FMP::SetPgnoBackupCopyMost( PGNO pgno ) { m_pgnoBackupCopyMost = pgno; }
INLINE VOID FMP::SetPgnoSnapBackupMost( PGNO pgno ) { OnDebug( AssertSizesConsistent( fTrue ) ); m_pgnoSnapBackupMost = pgno; }
INLINE VOID FMP::SetPatchchk( ATCHCHK * patchchk )  { m_patchchk = patchchk; }
INLINE VOID FMP::SetObjidLast( OBJID objid )        { m_objidLast = objid; }
INLINE VOID FMP::SetPinst( INST *pinst )            { m_pinst = pinst; }
INLINE VOID FMP::SetDbid( DBID dbid )               { m_dbid = dbid; }
INLINE VOID FMP::SetDwBFContext( DWORD_PTR dwBFContext )
{
#ifdef SYNC_DEADLOCK_DETECTION
    Assert( RwlIBFContext().FWriter() );
#endif  //  SYNC_DEADLOCK_DETECTION
    
    m_dwBFContext = dwBFContext;
}

INLINE DBTIME   FMP::DbtimeLastScrub() const        { return m_dbtimeLastScrub; }
INLINE VOID     FMP::SetDbtimeLastScrub( const DBTIME dbtimeLastScrub ) { m_dbtimeLastScrub = dbtimeLastScrub; }

INLINE LOGTIME  FMP::LogtimeScrub() const           { return m_logtimeScrub; }
INLINE VOID     FMP::SetLogtimeScrub( const LOGTIME& logtime ) { m_logtimeScrub = logtime; }

INLINE BOOL FMP::FScheduledPeriodicTrim() const     { return m_fScheduledPeriodicTrim; }

extern FMP  *g_rgfmp;                     /* database file map */
extern IFMP g_ifmpMax;

INLINE FMP* const PfmpFromIfmp( _In_ const IFMP ifmp )
{
    Assert( ifmp >= 1 );    //  don't give out 0 anymore

    Assert( ifmp >= FMP::IfmpMinInUse() );
    Assert( ifmp <= FMP::IfmpMacInUse() );

    if ( ifmp < FMP::IfmpMinInUse() || ifmp > FMP::IfmpMacInUse() )
    {
        Assert( ifmpNil == ifmp );
        return pfmpNil;
    }
    return &g_rgfmp[ ifmp ];
}

INLINE IFMP FMP::Ifmp() const
{
    const IFMP ifmp = (IFMP) (this - g_rgfmp);
    Assert( ifmp >= 1 );    //  don't give out 0 anymore IIRC
    Assert( ifmp >= IfmpMinInUse() );
    Assert( ifmp <= IfmpMacInUse() );
    return ifmp;
}

INLINE DBID DbidFromIfmp( const IFMP ifmp )
{
    Assert( ifmp >= 1 );    //  don't give out 0 anymore IIRC
    Assert( ifmp >= FMP::IfmpMinInUse() );
    Assert( ifmp <= FMP::IfmpMacInUse() );
    return g_rgfmp[ ifmp ].Dbid();
}

// This function should only be set off a recently committed/written 
// dbfilehdr.le_lGenMaxRequired field.  This function sets the waypoint on the FMP, and 
// the waypoint off the FMP is what holds back the buffer manager from writing pages it 
// should not.
INLINE VOID FMP::SetWaypoint( LONG lGenWaypoint )
{
    LGPOS lgposWaypoint;
    const IFMP ifmp = Ifmp();

    // We always explicitly call ResetWaypoint() instead of passing in a zero log generation.
    DBEnforce( ifmp, lGenWaypoint != 0 );

    // we jump whole logs at a time, so we use max isec/ib.
    lgposWaypoint = lgposMax;
    lgposWaypoint.lGeneration = lGenWaypoint;
    SetILgposWaypoint( lgposWaypoint );
}

// This function "resets" the waypoint to a "nil" value.
INLINE VOID FMP::ResetWaypoint( VOID )
{
    LGPOS lgposWaypoint;
    lgposWaypoint = lgposMin;
    FMP::EnterFMPPoolAsWriter();
    SetILgposWaypoint( lgposWaypoint );
    FMP::LeaveFMPPoolAsWriter();
}

INLINE VOID FMP::SetDataHeaderSignature( DBFILEHDR_FIX* const pdbhdrsig, const ULONG cbdbhdrsig )
{
    OnDebug( m_dataHdrSig.AssertValid() );
    Assert( m_dataHdrSig.FNull() );
    Assert( NULL != pdbhdrsig && 0 != cbdbhdrsig );
    m_dataHdrSig.SetPv( pdbhdrsig );
    m_dataHdrSig.SetCb( cbdbhdrsig );
    OnDebug( m_dataHdrSig.AssertValid() );
    Assert( !m_dataHdrSig.FNull() );
}

INLINE VOID FMP::SetLGenMaxCommittedAttachedDuringRecovery( const LONG lGenMaxCommittedAttachedDuringRecovery )
{
    m_lGenMaxCommittedAttachedDuringRecovery = lGenMaxCommittedAttachedDuringRecovery;
}

INLINE VOID FMP::SetKVPMSysLocales( CKVPStore * const pkvpsMSysLocales )
{
    Assert( NULL == m_pkvpsMSysLocales || NULL == pkvpsMSysLocales );
    m_pkvpsMSysLocales = pkvpsMSysLocales;
}

INLINE VOID FMP::SetLidmap( LIDMAP * const plidmap )
{
    Assert( NULL == m_plidmap || NULL == plidmap );
    m_plidmap = plidmap;
}

INLINE VOID FMP::ResetPctCachePriorityFmp()
{
    m_pctCachePriority = (WORD)g_pctCachePriorityUnassigned;
}

// =====================================================================
// Flags fields

INLINE VOID FMP::ResetFlags()
{
    Assert( FMP::FWriterFMPPool() );
    m_fFlags = 0;
}

INLINE VOID FMP::SetCrefWriteLatch( UINT cref )     { m_crefWriteLatch = cref; }

INLINE VOID FMP::SetCreatingDB()                    { m_fCreatingDB = fTrue; }
INLINE VOID FMP::ResetCreatingDB()                  { m_fCreatingDB = fFalse; }

INLINE VOID FMP::SetAttachingDB()                   { m_fAttachingDB = fTrue; }
INLINE VOID FMP::ResetAttachingDB()                 { m_fAttachingDB = fFalse; }

INLINE VOID FMP::SetDetachingDB()                   { m_fDetachingDB = fTrue; }
INLINE VOID FMP::ResetDetachingDB()                 { m_fDetachingDB = fFalse; }

INLINE VOID FMP::SetNoWaypointLatency()                     { m_fNoWaypointLatency = fTrue; }
INLINE VOID FMP::ResetNoWaypointLatency()                   { m_fNoWaypointLatency = fFalse; }
INLINE VOID FMP::SetAttachedForRecovery()                   { m_fAttachedForRecovery = fTrue; }
INLINE VOID FMP::ResetAttachedForRecovery()                 { m_fAttachedForRecovery = fFalse; }
INLINE VOID FMP::SetRecoveryChecksDone()                    { m_fRecoveryChecksDone = fTrue; }
INLINE VOID FMP::ResetRecoveryChecksDone()                  { m_fRecoveryChecksDone = fFalse; }

INLINE VOID FMP::SetAllowHeaderUpdate()             { m_fAllowHeaderUpdate = fTrue; }
INLINE VOID FMP::ResetAllowHeaderUpdate()           { m_fAllowHeaderUpdate = fFalse; }

INLINE BOOL FMP::FContainsDataFromFutureLogs() const            { return m_fContainsDataFromFutureLogs; }
INLINE VOID FMP::SetContainsDataFromFutureLogs()                { Assert( FWriterFMPPool() ); m_fContainsDataFromFutureLogs = fTrue; }
INLINE VOID FMP::ResetContainsDataFromFutureLogs( )         { Assert( FWriterFMPPool() ); m_fContainsDataFromFutureLogs = fFalse; }

INLINE BOOL FMP::FTrimSupported() const             { return m_fTrimSupported; }
INLINE VOID FMP::SetTrimSupported()                 { Assert( FWriterFMPPool() ); m_fTrimSupported = fTrue; }
INLINE VOID FMP::ResetTrimSupported()               { Assert( FWriterFMPPool() ); m_fTrimSupported = fFalse; }

INLINE BOOL FMP::FEfvSupported( const JET_ENGINEFORMATVERSION efvFormatFeature )
{
    if ( ( ErrDBFormatFeatureEnabled( efvFormatFeature ) == JET_errSuccess ) &&
        ( !FLogOn() || ( m_pinst->m_plog->ErrLGFormatFeatureEnabled( efvFormatFeature ) == JET_errSuccess ) ) )
    {
        return fTrue;
    }

    return fFalse;
}

INLINE CIoStats * FMP::Piostats( const IOTYPE iotype ) const    { Assert( iotype < _countof(m_rgpiostats) ); Assert( m_rgpiostats[iotype] ); return m_rgpiostats[iotype]; }
INLINE VOID FMP::SetPiostats( CIoStats * const piostatsDbRead, CIoStats * const piostatsDbWrite )
{
    Assert( FWriterFMPPool() );
    Assert( m_rgpiostats[iotypeRead] == NULL );
    Assert( m_rgpiostats[iotypeWrite] == NULL );
    m_rgpiostats[iotypeRead] = piostatsDbRead;
    m_rgpiostats[iotypeWrite] = piostatsDbWrite;
}
INLINE VOID FMP::FreeIostats()
{
    delete m_rgpiostats[iotypeRead];
    m_rgpiostats[iotypeRead] = NULL;
    delete m_rgpiostats[iotypeWrite];
    m_rgpiostats[iotypeWrite] = NULL;
}

INLINE VOID FMP::SetExclusiveOpen( PIB *ppib )
{
    Assert( m_fExclusiveOpen == 0 || m_ppibExclusiveOpen == ppib );
    m_fExclusiveOpen = fTrue;
    m_ppibExclusiveOpen = ppib;
}
INLINE VOID FMP::ResetExclusiveOpen( )              { m_fExclusiveOpen = fFalse; }

INLINE VOID FMP::SetReadOnlyAttach()
{
    Assert( !m_fLogOn );
    Assert( m_fVersioningOff );
    AssertSz( FDontRegisterOLD2Tasks(), "Read-only attaches must have OLD disabled first." );
    m_fReadOnlyAttach = fTrue;
}
INLINE VOID FMP::ResetReadOnlyAttach()              { m_fReadOnlyAttach = fFalse; }

INLINE VOID FMP::SetLogOn()
{
    Assert( !m_fReadOnlyAttach );
    Assert( !m_fVersioningOff );
    if ( m_pinst->m_plog->FLogDisabled() == fFalse &&
         m_fLogOn == fFalse )
    {
        Assert( FMP::FWriterFMPPool() );
        m_fLogOn = fTrue;
    }
}
INLINE VOID FMP::ResetLogOn()
{
    if ( m_fLogOn == fTrue )
    {
        Assert( FMP::FWriterFMPPool() );
        m_fLogOn = fFalse;
    }
}

INLINE VOID FMP::SetVersioningOff()                 { Assert( !m_fLogOn ); m_fVersioningOff = fTrue; }
INLINE VOID FMP::ResetVersioningOff()               { Assert( !m_fReadOnlyAttach ); m_fVersioningOff = fFalse; }

INLINE VOID FMP::SetSkippedAttach()                 { m_fSkippedAttach = fTrue; }
INLINE VOID FMP::ResetSkippedAttach()               { m_fSkippedAttach = fFalse; }

INLINE VOID FMP::SetAttached()
{
    Assert( FMP::FWriterFMPPool() );
    m_fAttached = fTrue;
}
INLINE VOID FMP::ResetAttached()
{
    Assert( FMP::FWriterFMPPool() );
    m_fAttached = fFalse;
}

INLINE VOID FMP::SetDeferredAttach( const bool fDeferredAttachConsistentFuture, const bool fDeferredForAccessDenied )               { m_fDeferredAttach = fTrue; m_fDeferredAttachConsistentFuture = fDeferredAttachConsistentFuture; m_fDeferredForAccessDenied = fDeferredForAccessDenied; }
INLINE VOID FMP::ResetDeferredAttach()              { m_fDeferredAttach = fFalse; m_fDeferredAttachConsistentFuture = fFalse; m_fDeferredForAccessDenied = fFalse; }

INLINE VOID FMP::SetIgnoreDeferredAttach()          { m_fIgnoreDeferredAttach = fTrue; }
INLINE VOID FMP::ResetIgnoreDeferredAttach()        { m_fIgnoreDeferredAttach = fFalse; }

INLINE VOID FMP::SetFailFastDeferredAttach()        { m_fFailFastDeferredAttach = fTrue; }
INLINE VOID FMP::ResetFailFastDeferredAttach()      { m_fFailFastDeferredAttach = fFalse; }

INLINE VOID FMP::SetOverwriteOnCreate()             { m_fOverwriteOnCreate = fTrue; }
INLINE VOID FMP::ResetOverwriteOnCreate()           { m_fOverwriteOnCreate = fFalse; }

INLINE VOID FMP::SetRunningOLD()                    { Assert( !m_fDontStartOLD ); m_fRunningOLD = fTrue; }
INLINE VOID FMP::ResetRunningOLD()                  { m_fRunningOLD = fFalse; }

INLINE VOID FMP::SetInBackupSession()               { m_fInBackupSession = fTrue; }
INLINE VOID FMP::ResetInBackupSession()             { m_fInBackupSession = m_fEDBBackupDone = fFalse; }

INLINE VOID FMP::SetOlderDemandExtendDb()           { m_fOlderDemandExtendDb = fTrue; }
INLINE VOID FMP::ResetOlderDemandExtendDb()         { m_fOlderDemandExtendDb = fFalse; }

INLINE VOID FMP::SetScheduledPeriodicTrim()         { m_fScheduledPeriodicTrim = fTrue; }
INLINE VOID FMP::ResetScheduledPeriodicTrim()       { m_fScheduledPeriodicTrim = fFalse; }

INLINE VOID FMP::SetRBSOn()                         { Assert( m_pinst->m_prbs != NULL ); m_fRBSOn = fTrue; }
INLINE VOID FMP::ResetRBSOn()                       { m_fRBSOn = fFalse; }

INLINE VOID FMP::SetNeedUpdateDbtimeBeginRBS() { Assert( m_fRBSOn ); m_fNeedUpdateDbtimeBeginRBS = fTrue; }
INLINE VOID FMP::ResetNeedUpdateDbtimeBeginRBS() { m_fNeedUpdateDbtimeBeginRBS = fFalse; }

// FDBIDWriteLatchByOther( PIB *ppib )
INLINE BOOL FMP::FWriteLatchByOther( PIB *ppib )
{
    Assert( m_critLatch.FOwner() );
    return ( m_crefWriteLatch != 0 && m_ppibWriteLatch != ppib );
}

// FDBIDWriteLatchByMe( ifmp, ppib )
INLINE BOOL FMP::FWriteLatchByMe( PIB *ppib )
{
    const BOOL fOwned = ( m_crefWriteLatch != 0 && m_ppibWriteLatch == ppib );
    // must either own critical section to check (in case we're not) or be the owner of the w-latch
    Assert( m_critLatch.FOwner() || fOwned );
    return fOwned;
}

// DBIDSetWriteLatch( ifmp, ppib )
INLINE VOID FMP::SetWriteLatch( PIB * ppib )
{
    Assert( m_critLatch.FOwner() );
    m_crefWriteLatch++;
    Assert( m_crefWriteLatch > 0 );
    m_ppibWriteLatch = ppib;
}

// DBIDResetWriteLatch( ifmp, ppib )
INLINE VOID FMP::ResetWriteLatch( PIB * ppib )
{
    Assert( m_critLatch.FOwner() );
    Assert( m_crefWriteLatch > 0 );
    m_crefWriteLatch--;
    Assert( m_ppibWriteLatch == ppib );
}

// FDBIDExclusiveByAnotherSession( ifmp, ppib )
INLINE BOOL FMP::FExclusiveByAnotherSession( PIB *ppib )
    { return m_fExclusiveOpen && m_ppibExclusiveOpen != ppib; }

// FDBIDExclusiveBySession( ifmp, ppib )
INLINE BOOL FMP::FExclusiveBySession( PIB *ppib )
    { return m_fExclusiveOpen && m_ppibExclusiveOpen == ppib; }

INLINE VOID FMP::FreePdbfilehdr()
{
    Assert( RwlDetaching().FWriter() );
    Assert( NULL != Pdbfilehdr() );
    DBFILEHDR *pdbfilehdr;
    CallS( ErrSetPdbfilehdr( NULL, &pdbfilehdr ) );
    OSMemoryPageFree( pdbfilehdr );
    SetDbtimeLast( 0 );
    SetObjidLast( 0 );
}

INLINE BOOL FMP::FHardRecovery( const DBFILEHDR* const pdbfilehdr ) const
{
    Assert( m_pinst );
    Assert( m_pinst->m_plog );

    if ( m_pinst->m_plog->FHardRestore() )
    {
        return fTrue;
    }

    // we might be able to remove this assert as the 
    // return value takes care if it. For the moment
    // I would like to leave it just because I don't
    // expect people checking on this if there is no
    // database attached.
    // 
    Assert( pdbfilehdr != NULL );
    Assert( ( m_dbfilehdrLock.m_pdbfilehdr == NULL ) || ( m_dbfilehdrLock.m_pdbfilehdr == pdbfilehdr ) );
    const BOOL fHR = ( pdbfilehdr && ( pdbfilehdr->bkinfoFullCur.le_genLow != 0 ) );
    return fHR;
}

INLINE BOOL FMP::FHardRecovery() const
{
    return FHardRecovery( Pdbfilehdr().get() );
}

INLINE VOID FMP::UpdatePgnoHighestWriteLatched( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoHighestWriteLatched, (LONG)pgnoHighestCandidate );
    // do not try assert, we don't want to try to sync a reset
}

INLINE VOID FMP::UpdatePgnoDirtiedMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoDirtiedMax, (LONG)pgnoHighestCandidate );
    // do not try assert, we don't want to try to sync a reset
}

INLINE VOID FMP::UpdatePgnoWriteLatchedNonScanMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoWriteLatchedNonScanMax, (LONG)pgnoHighestCandidate );
    // do not try assert, we don't want to try to sync a reset
}

INLINE VOID FMP::UpdatePgnoLatchedScanMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoLatchedScanMax, (LONG)pgnoHighestCandidate );
    Assert( m_pgnoLatchedScanMax >= pgnoHighestCandidate );
}

INLINE VOID FMP::UpdatePgnoPreReadScanMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoPreReadScanMax, (LONG)pgnoHighestCandidate );
    Assert( m_pgnoPreReadScanMax >= pgnoHighestCandidate );
}

INLINE VOID FMP::UpdatePgnoScanMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoScanMax, (LONG)pgnoHighestCandidate );
    Assert( m_pgnoScanMax >= pgnoHighestCandidate );
}

INLINE VOID FMP::ResetPgnoMaxTracking( const PGNO pgnoMaxSet = 0 )
{
    if ( pgnoMaxSet == 0 )
    {
        (void)AtomicExchange( (LONG*)&m_pgnoHighestWriteLatched, (LONG)0 );
        (void)AtomicExchange( (LONG*)&m_pgnoDirtiedMax, (LONG)0 );
        (void)AtomicExchange( (LONG*)&m_pgnoWriteLatchedNonScanMax, (LONG)0 );
        (void)AtomicExchange( (LONG*)&m_pgnoLatchedScanMax, (LONG)0 );
        (void)AtomicExchange( (LONG*)&m_pgnoPreReadScanMax, (LONG)0 );
        (void)AtomicExchange( (LONG*)&m_pgnoScanMax, (LONG)0 );
    }
    else
    {
        (void)AtomicExchangeMin( (LONG*)&m_pgnoHighestWriteLatched, (LONG)pgnoMaxSet );
        (void)AtomicExchangeMin( (LONG*)&m_pgnoDirtiedMax, (LONG)pgnoMaxSet );
        (void)AtomicExchangeMin( (LONG*)&m_pgnoWriteLatchedNonScanMax, (LONG)pgnoMaxSet );
        (void)AtomicExchangeMin( (LONG*)&m_pgnoLatchedScanMax, (LONG)pgnoMaxSet );
        (void)AtomicExchangeMin( (LONG*)&m_pgnoPreReadScanMax, (LONG)pgnoMaxSet );
        (void)AtomicExchangeMin( (LONG*)&m_pgnoScanMax, (LONG)pgnoMaxSet );
    }
}

#define FInRangeIFMP( ifmp )                                \
    ( ifmp != IFMP(JET_dbidNil) && ifmp >= cfmpReserved && ifmp < g_ifmpMax )

INLINE VOID FMP::AssertVALIDIFMP( IFMP ifmp )
{
    Assert( ifmpNil != ifmp );
    Assert( ifmp < g_ifmpMax );
    Assert( FInRangeIFMP( ifmp ) ); // essentially == as previous two asserts, just want to double check
    Assert( IfmpMinInUse() <= ifmp );
    Assert( IfmpMacInUse() >= ifmp );
    Assert( g_rgfmp[ifmp].FInUse() );
}

INLINE INST *PinstFromIfmp( IFMP ifmp )
{
    return g_rgfmp[ ifmp ].Pinst();
}

INLINE ULONG_PTR PctFMPCachePriority( const IFMP ifmp )
{
    Assert( ( g_rgfmp != NULL ) && ( ifmp != ifmpNil ) && FMP::FAllocatedFmp( ifmp ) );
    const WORD pctFmpCachePriority = g_rgfmp[ ifmp ].PctCachePriorityFmp();

    if ( FIsCachePriorityAssigned( pctFmpCachePriority ) )
    {
        Assert( FIsCachePriorityValid( pctFmpCachePriority ) );
        return pctFmpCachePriority;
    }

    return PctINSTCachePriority( PinstFromIfmp( ifmp ) );
}

INLINE IFMP IfmpFirstDatabaseOpen( const PIB *ppib, const IFMP ifmpExcept = g_ifmpMax )
{
    for ( DBID dbidT = dbidUserLeast; dbidT < dbidMax; dbidT++ )
    {
        if ( ppib->rgcdbOpen[dbidT] )
        {
            const IFMP  ifmpT   = PinstFromPpib( ppib )->m_mpdbidifmp[ dbidT ];

            FMP::AssertVALIDIFMP( ifmpT );

            // if the one found is excepted from the search, continue
            // (default (=g_ifmpMax) will just continue;
            if ( g_ifmpMax == ifmpExcept || ifmpT != ifmpExcept )
                return ifmpT;
        }
    }

    return g_ifmpMax;
}

INLINE BOOL FSomeDatabaseOpen( const PIB *ppib, const IFMP ifmpExcept = g_ifmpMax )
{
    return ( g_ifmpMax != IfmpFirstDatabaseOpen( ppib, ifmpExcept ) );
}

INLINE VOID FMP::TraceDbfilehdrInfo( const TraceStationIdentificationReason tsidr )
{
    //  Called (with tsidrPulseInfo) for every DB cache read/write.

    if ( !m_traceidcheckDatabase.FAnnounceTime< _etguidIsamDbfilehdrInfo >( tsidr ) )
    {
        return;
    }

    Expected( tsidr == tsidrPulseInfo ||
                tsidr == tsidrEngineFmpDbHdr1st );

    PdbfilehdrReadOnly pdbfilehdr = this->Pdbfilehdr();
    if ( pdbfilehdr )
    {
        ULONG filetype = pdbfilehdr->le_filetype;
        ULONG ulMagic = pdbfilehdr->le_ulMagic;
        ULONG ulChecksum = pdbfilehdr->le_ulChecksum;
        ULONG cbPageSize = pdbfilehdr->le_cbPageSize;
        ULONG ulDbFlags = pdbfilehdr->m_ulDbFlags;
        Assert( filetype );
        Assert( ulMagic );
        Assert( ulChecksum );
        Assert( cbPageSize );

        const IFMP ifmp = Ifmp();

        BYTE * pbSignDb = (BYTE*)&( pdbfilehdr->signDb );

        ETIsamDbfilehdrInfo( tsidr, ifmp, filetype, ulMagic, ulChecksum, cbPageSize, ulDbFlags, pbSignDb );
    }
}

INLINE VOID FMP::TraceStationId( const TraceStationIdentificationReason tsidr )
{
    //  Called (with tsidrPulseInfo) for every DB cache read/write.

    if ( tsidr == tsidrPulseInfo )
    {
        //  vector off to other components to give it a chance to announce as well  

        m_pinst->TraceStationId( tsidr );
        TraceDbfilehdrInfo( tsidr );
    }

    if ( !m_traceidcheckFmp.FAnnounceTime< _etguidFmpStationId >( tsidr ) )
    {
        return;
    }

    Expected( tsidr == tsidrPulseInfo ||
                tsidr == tsidrOpenInit ||
                tsidr == tsidrCloseTerm ||
                tsidr == tsidrEngineFmpDbHdr1st );

    ETFmpStationId( tsidr, Ifmp(), Pinst()->m_iInstance, Dbid(), m_wszDatabaseName );
}

INLINE BOOL FMP::FIsTempDB() const
{
    return dbidTemp == Dbid();
}

INLINE BOOL FFMPIsTempDB( const IFMP ifmp )
{
    return PfmpFromIfmp(ifmp)->FIsTempDB();
}


extern IFMP g_ifmpTrace;
#define OSTraceFMP( __ifmp, __tag, __szTrace )      OSTraceFiltered( __tag, __szTrace, ( (IFMP)JET_dbidNil == g_ifmpTrace || (__ifmp) == g_ifmpTrace ) )


