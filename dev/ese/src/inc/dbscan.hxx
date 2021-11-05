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

typedef enum class ObjidState
{
    Unknown,    // In the object cache, it's the uninitialized state of an entry. In the calling code,
                // it is used to signal that we don't know about the object's existence and it need
                // to look it up in MSysObjids. It is expected that the accompanying FUCB* is pfucbNil.

    Valid,      // The object is known to still exist. It is expected that the accompanying
                // FUCB* is not pfucbNil.

    ValidNoFUCB,// The object is known to still exist. But the accompanying FUCB* is pfucbNil.

    Invalid,    // The object is known to have been deleted. It is expected that the accompanying
                // FUCB* is pfucbNil.

    ToBeDeleted // The object is to be deleted soon, so we can skip any cleanup opeartions.
                // This happens when we revert a deleted table but just the root page and space tree pages.
                // It is expected that the accompanying FUCB* is pfucbNil.
} ois;

//  ================================================================
class DBMObjectCache
    //  ================================================================
    //
    //  Cache object FUCBs for tables and LVs, and validity information for indices
    //  that DBM is working on.
    //
    //  Even though secondary indices are also maintained in the cache, we don't
    //  keep FUCBs for those objects cached, for two reasons:
    //
    //    1 - DBM does not need, at any point, to open an FUCB to a secondary index
    //        to do its work.
    //
    //    2 - Index deletion might fail to the client because its FCB will have been
    //        referenced by the cache. A similar situation exists for tables but, in
    //        that case, table deletion handles it by waiting until all FUCBs have been
    //        closed, which happens when DBM senses that a deletion is pending.
    //
    //  Also, note that, because FUCBs against secondary indices are not cached, there
    //  is no way for this cache to detect that an index deletion is pending. That means
    //  that a secondary index entry in the cache is useful to short-circuit looking for
    //  the object when the index has already been deleted, but not the other way around.
    //  This causes an MSysObjids lookup every time a secondary index page is found, which
    //  is not as expensive as loading up an FUCB for a table, for example, and chances are
    //  those few MSysObjids pages will remain in the cache. Clients that control progress
    //  of DBM via suspend/resume already lose the entire cache in-between batches, so we
    //  already have to refresh the cache periodically anyways.
    //
    //-
{
public:
    DBMObjectCache();

    // the destructor closes all the cached objects
    ~DBMObjectCache();

    ObjidState OisGetObjidState( const OBJID objid );

    // Returns NULL if there is no cached FUCB
    FUCB * PfucbGetCachedObject( const OBJID objid );

    void CacheObjectFucb( FUCB * const pfucb, const OBJID objid );
    void MarkCacheObjectInvalid( const OBJID objid );
    void MarkCacheObjectToBeDeleted( const OBJID objid );
    void MarkCacheObjectValidNoFUCB( const OBJID objid );

    void CloseCachedObjectWithObjid( const OBJID objid );
    void CloseCachedObjectsWithPendingDeletes();
    void CloseAllCachedObjects();

private:
    bool FContainsObjid_( const OBJID objid ) const;
    INT IndexOfObjid_( const OBJID objid ) const;
    INT IndexOfLeastRecentlyUsedObject_( const OBJID objidIgnore = objidNil ) const;
    void CloseObjectAt_( const INT index );

private:
    static const INT m_cobjectsMax = 64;
    struct
    {
        OBJID objid;
        ObjidState ois;
        FUCB * pfucb;
        __int64 ftAccess;
    } m_rgstate[m_cobjectsMax];

private:    // not implemented
    DBMObjectCache( const DBMObjectCache& );
    DBMObjectCache& operator=( const DBMObjectCache& );
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
//  LRSCANCHECK2 uses only 2 bits for source. So scsMax shouldn't cross 3.
//  Update LRSCANCHECK2 and use a new efv if we want to add more sources than that.
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
    const CPAGE* const pcpage,
    const ObjidState objidState );
ERR ErrDBMEmitEndScan( const IFMP ifmp );
USHORT UsDBMGetCompressedLoggedChecksum( const CPAGE& cpage, const DBTIME dbtimeSeed );
ULONG UlDBMGetCompressedLoggedChecksum( const CPAGE& cpage, const DBTIME dbtimeSeed );

//  We will leave this implemented just in case we need it.  Move to jethdr.w and
//  expose to HA / Store if necessary.
//
#define bitDBScanInRecoveryPassiveScan      0x2
#define bitDBScanInRecoveryFollowActive     0x1

