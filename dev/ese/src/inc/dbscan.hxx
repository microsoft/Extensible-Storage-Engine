// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace MSysDBM
{
    bool FIsSystemTable(const char * const szTable);
    ERR ErrDumpTable(const IFMP ifmp);
}


class IDBMScan
{
public:
    virtual ~IDBMScan();

    virtual ERR ErrStartDBMScan() = 0;

    virtual void StopDBMScan() = 0;

protected:
    IDBMScan();
};

class IDBMScanState;
class DBMScanObserver;
class FMP;

class CDBMScanFollower
{
private:
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
        dbmdrrStoppedScan,
        dbmdrrDisabledScan,
    };

#ifdef ENABLE_JET_UNIT_TEST
    VOID TestSetup( IDBMScanState * pstate );
#endif

    CDBMScanFollower();
    ~CDBMScanFollower();

    ERR ErrRegisterFollower( FMP * const pfmp, const PGNO pgnoStart );
    VOID DeRegisterFollower( const FMP * const pfmp, const DBMScanDeRegisterReason eDeRegisterReason );

    BOOL FStopped() const;

    ERR ErrDBMScanReadThroughCache( const IFMP ifmp, const PGNO pgno, void * pvPages, const CPG cpg );

    VOID CompletePage( const PGNO pgno, const BOOL fBadPage = fFalse );

    VOID UpdateIoTracking( const IFMP ifmp, const CPG cpg );

    static VOID SkippedDBMScanCheckRecord( FMP * const pfmp, const PGNO pgno = pgnoNull );

    static VOID ProcessedDBMScanCheckRecord( const FMP * const pfmp );
};

namespace DBMScanFactory
{
    ERR ErrPdbmScanCreate(const IFMP ifmp, __out IDBMScan ** pdbmscan);

    ERR ErrPdbmScanCreateSingleScan(
                    const JET_SESID sesid,
                    const IFMP ifmp,
                    const INT csecMax,
                    const INT cmsecSleep,
                    const JET_CALLBACK pfnCallback,
                    __out IDBMScan ** pdbmscan);
};

VOID DBMScanStopAllScansForInst( INST * pinst, const BOOL fAllowRestarting = fFalse );

ERR ErrDBMInit();
void DBMTerm();

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

#define bitDBScanInRecoveryPassiveScan      0x2
#define bitDBScanInRecoveryFollowActive     0x1

