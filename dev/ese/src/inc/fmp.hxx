// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


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

class PdbfilehdrLocked
{
public:
    virtual ~PdbfilehdrLocked();

    operator const void*() const;
    const DBFILEHDR * operator->() const;
    const DBFILEHDR * get() const;

protected:
    PdbfilehdrLocked(DbfilehdrLock * const plock);
    PdbfilehdrLocked(PdbfilehdrLocked& rhs);

    DbfilehdrLock * Plock();
    
private:
    DbfilehdrLock * m_plock;

private:
    PdbfilehdrLocked& operator=(PdbfilehdrLocked& rhs);
};


class PdbfilehdrReadOnly : public PdbfilehdrLocked
{
public:
    PdbfilehdrReadOnly(DbfilehdrLock * const plock);
    PdbfilehdrReadOnly(PdbfilehdrReadOnly& rhs);
    ~PdbfilehdrReadOnly();
};


class PdbfilehdrReadWrite : public PdbfilehdrLocked
{
public:
    PdbfilehdrReadWrite(DbfilehdrLock * const plock);
    PdbfilehdrReadWrite(PdbfilehdrReadWrite& rhs);
    ~PdbfilehdrReadWrite();

    DBFILEHDR * operator->();
    DBFILEHDR * get();
};


class DbfilehdrLock
{
public:
    DbfilehdrLock();
    ~DbfilehdrLock();

    DBFILEHDR * SetPdbfilehdr(DBFILEHDR * const pdbfilehdr);

    PdbfilehdrReadOnly  GetROHeader();
    PdbfilehdrReadWrite GetRWHeader();

private:
    friend class PdbfilehdrLocked;
    friend class PdbfilehdrReadOnly;
    friend class PdbfilehdrReadWrite;
#ifndef MINIMAL_FUNCTIONALITY
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

private:
    DbfilehdrLock(const DbfilehdrLock&);
    DbfilehdrLock& operator=(const DbfilehdrLock&);
};



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
        enum DbHeaderUpdateState
            {
                dbhusNoUpdate       = 0,
                dbhusHdrLoaded      = 1,
                dbhusUpdateLogged   = 2,
                dbhusUpdateSet      = 3,
                dbhusUpdateFlushed  = 4,
            };
        DbHeaderUpdateState m_dbhus;
#endif

    public:

        FMP();
        ~FMP();

    private:
    
        WCHAR               *m_wszDatabaseName;     

        INST                *m_pinst;
        ULONG               m_dbid;                 
        union
        {
            UINT            m_fFlags;
            struct
            {
                UINT        m_fCreatingDB:1;        
                UINT        m_fAttachingDB:1;       
                UINT        m_fDetachingDB:1;       

                UINT        m_fExclusiveOpen:1;     
                UINT        m_fReadOnlyAttach:1;    

                UINT        m_fLogOn:1;             
                UINT        m_fVersioningOff:1;     

                UINT        m_fSkippedAttach:1;     
                UINT        m_fAttached:1;          
                UINT        m_fDeferredAttach:1;    
                UINT        m_fDeferredAttachConsistentFuture:1;    
                UINT        m_fDeferredForAccessDenied:1;   
                UINT        m_fIgnoreDeferredAttach : 1;    
                UINT        m_fFailFastDeferredAttach : 1;  
                UINT        m_fOverwriteOnCreate : 1;   

                UINT        m_fRunningOLD:1;        

                UINT        m_fInBackupSession:1;   

                UINT        m_fAllowHeaderUpdate:1;     

                UINT        m_fDefragPreserveOriginal:1; 

                UINT        m_fCopiedPatchHeader:1; 

                UINT        m_fEDBBackupDone:1; 

                UINT        m_fDontRegisterOLD2Tasks:1; 

                UINT        m_fCacheAvail:1;    

                UINT        m_fMaintainMSObjids:1;  
                UINT        m_fNoWaypointLatency:1; 
                UINT        m_fAttachedForRecovery:1; 
                UINT        m_fRecoveryChecksDone:1; 

                UINT        m_fTrimSupported:1;

                UINT        m_fContainsDataFromFutureLogs:1;

                UINT        m_fOlderDemandExtendDb:1;

            };
        };

        CCriticalSection    m_critOpenDbCheck;

        DBTIME              m_dbtimeLast;           
        DBTIME              m_dbtimeOldestGuaranteed;
        DBTIME              m_dbtimeOldestCandidate;
        DBTIME              m_dbtimeOldestTarget;

        CCriticalSection    m_critDbtime;

        DBTIME              m_dbtimeBeginRBS;                   
        BOOL                m_fRBSOn;                           
        BOOL                m_fNeedUpdateDbtimeBeginRBS;        


        TRX                 m_trxOldestCandidate;
        TRX                 m_trxOldestTarget;

        OBJID               m_objidLast;            

        ULONG               m_ctasksActive;

        IFileAPI            *m_pfapi;               

        DbfilehdrLock       m_dbfilehdrLock;

        CCriticalSection    m_critLatch;            
        CGate               m_gateWriteLatch;       

        QWORD               m_cbOwnedFileSize;      
        QWORD               m_cbFsFileSizeAsyncTarget; 
        CSemaphore          m_semIOSizeChange;      

        UINT                m_cPin;                 
        INT                 m_crefWriteLatch;       
        PIB                 *m_ppibWriteLatch;      
        PIB                 *m_ppibExclusiveOpen;   

        LGPOS               m_lgposAttach;          
        LGPOS               m_lgposDetach;          

        CReaderWriterLock   m_rwlDetaching;

        TRX                 m_trxNewestWhenDiscardsLastReported;
        CPG                 m_cpgDatabaseSizeMax;   
        PGNO                m_pgnoBackupMost;       
        PGNO                m_pgnoBackupCopyMost;   
        PGNO                m_pgnoSnapBackupMost;   

        CSemaphore          m_semRangeLock;
        CMeteredSection     m_msRangeLock;

        RANGELOCK*          m_rgprangelock[ 2 ];

        DWORD_PTR           m_dwBFContext;

        CReaderWriterLock   m_rwlBFContext;

        INT                 m_cpagePatch;           
        ULONG               m_cPatchIO;             
        IFileAPI            *m_pfapiPatch;          
        PATCH_HEADER_PAGE   *m_ppatchhdr;           

        DBTIME              m_dbtimeCurrentDuringRecovery;      

        DBTIME              m_dbtimeLastScrub;
        LOGTIME             m_logtimeScrub;

        WCHAR               *m_wszPatchPath;

        ATCHCHK             *m_patchchk;

        volatile LGPOS      m_lgposWaypoint;

#ifdef DEBUGGER_EXTENSION
    public:
#endif
        static IFMP         s_ifmpMacCommitted;
    private:
        static IFMP         s_ifmpMinInUse;
        static IFMP         s_ifmpMacInUse;

        DATA                m_dataHdrSig;

        LONG                m_lGenMaxCommittedAttachedDuringRecovery;

        IDBMScan *          m_pdbmScan;
        CDBMScanFollower *  m_pdbmFollower;

        CFlushMapForAttachedDb *m_pflushmap;

        CKVPStore *         m_pkvpsMSysLocales;
        
        volatile LONG       m_cAsyncIOForViewCachePending;

        CPG                 m_cpgAvail;

        BYTE                m_fShrinkIsRunning;

        PGNO                m_pgnoShrinkTarget;

        BYTE                m_fLeakReclaimerIsRunning;

        CSemaphore          m_semTrimmingDB;

        CSemaphore          m_semDBM;

        BYTE                m_fDontStartDBM;

        BYTE                m_fDontStartOLD;

        BYTE                m_fDontStartTrimTask;

        BYTE                m_fScheduledPeriodicTrim;

        CLimitedEventSuppressor m_lesEventDbFeatureDisabled;

        CLimitedEventSuppressor m_lesEventBtSkippedNodesCpu;
        CLimitedEventSuppressor m_lesEventBtSkippedNodesDisk;

        LIDMAP              *m_plidmap;

        COSEventTraceIdCheck m_traceidcheckFmp;
        COSEventTraceIdCheck m_traceidcheckDatabase;

    public:
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

        CLogRedoMap *       m_pLogRedoMapZeroed;

        CLogRedoMap *       m_pLogRedoMapBadDbtime;

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
        CReaderWriterLock& RwlIBFContext();
public:
#ifdef DEBUG
        BOOL FBFContextReader() const;
        BOOL FNotBFContextWriter() const;
#endif

        BOOL FTryEnterBFContextAsReader();
        VOID EnterBFContextAsReader();
        VOID LeaveBFContextAsReader();
        VOID EnterBFContextAsWriter();
        VOID LeaveBFContextAsWriter();

        DWORD_PTR DwBFContext();
        BOOL FBFContext() const;

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

        VOID IncrementAsyncIOForViewCache();
        VOID DecrementAsyncIOForViewCache();
        
        VOID WaitForAsyncIOForViewCache();

        PGNO PgnoHighestWriteLatched() const { return m_pgnoHighestWriteLatched; }
        PGNO PgnoDirtiedMax() const { return m_pgnoDirtiedMax; }
        PGNO PgnoWriteLatchedNonScanMax() const { return m_pgnoWriteLatchedNonScanMax; }
        PGNO PgnoLatchedScanMax() const { return m_pgnoLatchedScanMax; }
        PGNO PgnoPreReadScanMax() const { return m_pgnoPreReadScanMax; }
        PGNO PgnoScanMax() const { return m_pgnoScanMax; }

        IFileAPI::FileModeFlags FmfDbDefault() const;

        VOID SetRuntimeDbParams( const CPG cpgDatabaseSizeMax,
                                    const ULONG_PTR pctCachePriority,
                                    const JET_GRBIT grbitShrinkDatabaseOptions,
                                    const LONG dtickShrinkDatabaseTimeQuota,
                                    const CPG cpgShrinkDatabaseSizeLimit,
                                    const BOOL fLeakReclaimerEnabled,
                                    const LONG dtickLeakReclaimerTimeQuota );

        UINT CpgDatabaseSizeMax() const;
        VOID SetDatabaseSizeMax( CPG cpg );
        WORD PctCachePriorityFmp() const;
        VOID SetPctCachePriorityFmp( const ULONG_PTR pctCachePriority );
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
        VOID SetLeakReclaimerEnabled( const BOOL fLeakReclaimerEnabled );
        VOID SetLeakReclaimerTimeQuota( const LONG dtickLeakReclaimerTimeQuota );
        BOOL FLeakReclaimerEnabled() const;
        LONG DtickLeakReclaimerTimeQuota() const;

        CLogRedoMap* PLogRedoMapZeroed() const        { return m_pLogRedoMapZeroed; };
        CLogRedoMap* PLogRedoMapBadDbTime() const     { return m_pLogRedoMapBadDbtime; };

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


    public:

       CPG CpgOfCb( const QWORD cb ) const        { return (CPG) ( cb / CbPage() ); }
       QWORD CbOfCpg( const CPG cpg ) const       { return ( QWORD( cpg ) * CbPage() ); }

    public:

        static ERR ErrFMPInit();
        static VOID Term();

        static FMP * PfmpCreateMockFMP();
        static void FreeMockFMP(FMP * const pfmp);

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
#endif
        VOID AcquireIOSizeChangeLatch();
        VOID ReleaseIOSizeChangeLatch();

        VOID GetPeriodicTrimmingDBLatch( );
        VOID ReleasePeriodicTrimmingDBLatch( );

        VOID GetWriteLatch( PIB *ppib );
        VOID ReleaseWriteLatch( PIB *ppib );

        ERR RegisterTask();
        ERR UnregisterTask();
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

        static BOOL FPendingRedoMapEntries();
        ERR ErrEnsureLogRedoMapsAllocated();
        VOID FreeLogRedoMaps( const BOOL fAllocCleanup = fFalse );
        BOOL FRedoMapsEmpty();

#ifdef DEBUGGER_EXTENSION
        VOID Dump( CPRINTF * pcprintf, DWORD_PTR dwOffset = 0 ) const;
#endif

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
#endif
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

    friend DBFILEHDR * PdbfilehdrEDBGAccessor( const FMP * const pfmp );

};



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
#else
INLINE VOID FMP::AssertLRVFlags() const         {}
#endif
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
#endif

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
#endif

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


    IFileAPI::FileModeFlags fmfDefault =
        ( FReadOnlyAttach() ? IFileAPI::fmfReadOnly : IFileAPI::fmfNone ) |
        ( ( !FReadOnlyAttach() && BoolParam( JET_paramEnableFileCache ) && !FLogOn() ) ? IFileAPI::fmfLossyWriteBack : IFileAPI::fmfNone ) |
        ( ( BoolParam( m_pinst, JET_paramUseFlushForWriteDurability ) && !FReadOnlyAttach() ) ? IFileAPI::fmfStorageWriteBack : IFileAPI::fmfNone ) |
        ( BoolParam( JET_paramEnableFileCache ) ? IFileAPI::fmfCached : IFileAPI::fmfNone );

#ifdef FORCE_STORAGE_WRITEBACK
#ifdef DEBUG
    if ( !( fmfDefault & IFileAPI::fmfStorageWriteBack ) && ( ( TickOSTimeCurrent() % 100 ) < 50 ) )
        fmfDefault = fmfDefault | IFileAPI::fmfStorageWriteBack;
#else
    fmfDefault = fmfDefault | IFileAPI::fmfStorageWriteBack;
#endif
    if ( ( fmfDefault & IFileAPI::fmfStorageWriteBack ) &&
            ( fmfDefault & IFileAPI::fmfReadOnly ) )
    {
        fmfDefault = fmfDefault & ~IFileAPI::fmfStorageWriteBack;
    }
#else

    if ( ( fmfDefault & IFileAPI::fmfLossyWriteBack ) ||
            ( fmfDefault & IFileAPI::fmfReadOnly ) )
    {
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
#endif

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
#endif
    
    m_dwBFContext = dwBFContext;
}

INLINE DBTIME   FMP::DbtimeLastScrub() const        { return m_dbtimeLastScrub; }
INLINE VOID     FMP::SetDbtimeLastScrub( const DBTIME dbtimeLastScrub ) { m_dbtimeLastScrub = dbtimeLastScrub; }

INLINE LOGTIME  FMP::LogtimeScrub() const           { return m_logtimeScrub; }
INLINE VOID     FMP::SetLogtimeScrub( const LOGTIME& logtime ) { m_logtimeScrub = logtime; }

INLINE BOOL FMP::FScheduledPeriodicTrim() const     { return m_fScheduledPeriodicTrim; }

extern FMP  *g_rgfmp;                     
extern IFMP g_ifmpMax;

INLINE FMP* const PfmpFromIfmp( _In_ const IFMP ifmp )
{
    Assert( ifmp >= 1 );

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
    Assert( ifmp >= 1 );
    Assert( ifmp >= IfmpMinInUse() );
    Assert( ifmp <= IfmpMacInUse() );
    return ifmp;
}

INLINE DBID DbidFromIfmp( const IFMP ifmp )
{
    Assert( ifmp >= 1 );
    Assert( ifmp >= FMP::IfmpMinInUse() );
    Assert( ifmp <= FMP::IfmpMacInUse() );
    return g_rgfmp[ ifmp ].Dbid();
}

INLINE VOID FMP::SetWaypoint( LONG lGenWaypoint )
{
    LGPOS lgposWaypoint;
    const IFMP ifmp = Ifmp();

    DBEnforce( ifmp, lGenWaypoint != 0 );

    lgposWaypoint = lgposMax;
    lgposWaypoint.lGeneration = lGenWaypoint;
    SetILgposWaypoint( lgposWaypoint );
}

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

INLINE BOOL FMP::FWriteLatchByOther( PIB *ppib )
{
    Assert( m_critLatch.FOwner() );
    return ( m_crefWriteLatch != 0 && m_ppibWriteLatch != ppib );
}

INLINE BOOL FMP::FWriteLatchByMe( PIB *ppib )
{
    const BOOL fOwned = ( m_crefWriteLatch != 0 && m_ppibWriteLatch == ppib );
    Assert( m_critLatch.FOwner() || fOwned );
    return fOwned;
}

INLINE VOID FMP::SetWriteLatch( PIB * ppib )
{
    Assert( m_critLatch.FOwner() );
    m_crefWriteLatch++;
    Assert( m_crefWriteLatch > 0 );
    m_ppibWriteLatch = ppib;
}

INLINE VOID FMP::ResetWriteLatch( PIB * ppib )
{
    Assert( m_critLatch.FOwner() );
    Assert( m_crefWriteLatch > 0 );
    m_crefWriteLatch--;
    Assert( m_ppibWriteLatch == ppib );
}

INLINE BOOL FMP::FExclusiveByAnotherSession( PIB *ppib )
    { return m_fExclusiveOpen && m_ppibExclusiveOpen != ppib; }

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
}

INLINE VOID FMP::UpdatePgnoDirtiedMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoDirtiedMax, (LONG)pgnoHighestCandidate );
}

INLINE VOID FMP::UpdatePgnoWriteLatchedNonScanMax( const PGNO pgnoHighestCandidate )
{
    (void)AtomicExchangeMax( (LONG*)&m_pgnoWriteLatchedNonScanMax, (LONG)pgnoHighestCandidate );
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
    Assert( FInRangeIFMP( ifmp ) );
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

    if ( tsidr == tsidrPulseInfo )
    {

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


