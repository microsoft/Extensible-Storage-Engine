// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  ================================================================
namespace MSysDBM
//  ================================================================
{
    bool FIsSystemTable(const char * const szTable);
    ERR ErrDumpTable(const IFMP ifmp);
}


//  ================================================================
class IDBMScan
//  ================================================================
//
//  Abstract class for database scanning.
//
//-
{
public:
    virtual ~IDBMScan();

    // starts a scan, returns JET_errDatabaseAlreadyRunningMaintenance
    // if the scan is already running
    virtual ERR ErrStartDBMScan() = 0;

    // stops a scan, if one is running
    virtual void StopDBMScan() = 0;

protected:
    IDBMScan();
};

//  pre-decls for CDBMScanFollower implementation
class IDBMScanState;
class DBMScanObserver;
class FMP;

//  ================================================================
class CDBMScanFollower
//  ================================================================
//
//  This class holds all the logic for following the pgno ScanCheck records from
//  the active.
//
//-
{
private:
    //  The ContigCompleted variable tracks what we have checked _continuously_ from pgno 1.
    //  Some additional state is tracked in the IDMBScanState (cpg for current pass, highest page)
    PGNO                                m_pgnoHighestContigCompleted;

    IDBMScanState *                     m_pstate;
    DBMScanObserver *                   m_pscanobsFileCheck;
    DBMScanObserver *                   m_pscanobsLgriEvents;
    DBMScanObserver *                   m_pscanobsPerfmon;
    __int64                             m_cFollowerSkipsPrePass;
    UserTraceContext                    m_tc;

public:
    enum DBMScanDeRegisterReason {
        dbmdrrFinishedScan = 1,
        dbmdrrStoppedScan,      //  Intentionally stopped, but not finished or disabled, such as for detach, or term w/o undo.
        dbmdrrDisabledScan,     //  Client changed their mind and dynamically disabled the scan!
    };

#ifdef ENABLE_JET_UNIT_TEST
    VOID TestSetup( IDBMScanState * pstate );
#endif

    CDBMScanFollower();
    ~CDBMScanFollower();

    // Used to indicate that the recovery DBMScan follower (during 
    // recovery) is about to start, or about to stop.
    ERR ErrRegisterFollower( FMP * const pfmp, const PGNO pgnoStart );
    VOID DeRegisterFollower( const FMP * const pfmp, const DBMScanDeRegisterReason eDeRegisterReason );

    BOOL FStopped() const;

    // Required to replay the lrtypScanCheck
    ERR ErrDBMScanReadThroughCache( const IFMP ifmp, const PGNO pgno, void * pvPages, const CPG cpg );

    // Indicate we've completed processing this page
    VOID CompletePage( const PGNO pgno, const BOOL fBadPage = fFalse );

    // Indicate we've completed reading (or issuing prereads against) pages to be consumed by ScanCheck records
    VOID UpdateIoTracking( const IFMP ifmp, const CPG cpg );

    // Indicate we've skipped processing a page
    static VOID SkippedDBMScanCheckRecord( FMP * const pfmp, const PGNO pgno = pgnoNull );

    // Indicate we've processed a page, completely through the actual DB divergence check
    static VOID ProcessedDBMScanCheckRecord( const FMP * const pfmp );
};

//  ================================================================
namespace DBMScanFactory
//  ================================================================
//
//  Create IDBMScan objects. This returns an initialized, but not
//  running, IDBMScan object.
//
//-
{
    // create a new scan configured to do multiple passes of the
    // database, using the system parameters
    ERR ErrPdbmScanCreate(const IFMP ifmp, _Out_ IDBMScan ** pdbmscan);

    // create a new scan configured to do one pass of the database,
    // using the given parameters
    ERR ErrPdbmScanCreateSingleScan(
                    const JET_SESID sesid,
                    const IFMP ifmp,
                    const INT csecMax,
                    const INT cmsecSleep,
                    const JET_CALLBACK pfnCallback,
                    _Out_ IDBMScan ** pdbmscan);
};

VOID DBMScanStopAllScansForInst( INST * pinst, const BOOL fAllowRestarting = fFalse );

//  Initialize/terminate the Database Maintenance subsystem.
//
ERR ErrDBMInit();
void DBMTerm();

//  Types and helpers for ScanCheck consumers.
//
PERSISTED
enum ScanCheckSource : BYTE
{
    scsInvalid  = 0,
    scsDbScan   = 1,
    scsDbShrink = 2,
    scsMax      = 3
};

ERR ErrDBMEmitDivergenceCheck(
    const IFMP ifmp,
    const PGNO pgno,
    const ScanCheckSource scs,
    const CPAGE* const pcpage );
ERR ErrDBMEmitEndScan( const IFMP ifmp );
USHORT UsDBMGetCompressedLoggedChecksum( const CPAGE& cpage, const DBTIME dbtimeSeed );
ULONG UlDBMGetCompressedLoggedChecksum( const CPAGE& cpage, const DBTIME dbtimeSeed );

//  We will leave this implemented just in case we need it.  Move to jethdr.w and
//  expose to HA / Store if necessary.
//
#define bitDBScanInRecoveryPassiveScan      0x2
#define bitDBScanInRecoveryFollowActive     0x1

