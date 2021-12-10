// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
// This is not safe to do in unit tests where NULL and non-NULL but bad pointers are used.
#define ENABLE_FUCB_IN_DBSCAN_CACHE_PROTECTION 1
#endif

//  ****************************************************************
//  Performance Counters
//  ****************************************************************

#ifdef PERFMON_SUPPORT

PERFInstanceDelayedTotal< LONG, INST, fFalse > perfctrDBMDuration;
PERFInstanceDelayedTotal<> perfctrDBMPagesRead;
PERFInstanceDelayedTotal<> perfctrDBMPagesZeroed;
PERFInstanceDelayedTotal<> perfctrDBMZeroRefCountLvsDeleted;
PERFInstanceDelayedTotal<> perfctrDBMFDeletedLvPagesReclaimed;
PERFInstanceDelayedTotal<> perfctrDBMIOReads;
PERFInstanceDelayedTotal<> perfctrDBMIOReadSize;
PERFInstanceDelayedTotal< LONG, INST, fFalse > perfctrDBMThrottleSetting;
PERFInstanceDelayedTotal<> perfctrDBMIOReReads;
PERFInstanceDelayedTotal<> perfctrDBMFollowerSkips;
PERFInstanceDelayedTotal<> perfctrDBMFollowerDivChecked;


LONG LDBMaintDurationCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMDuration.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintPagesReadCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMPagesRead.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintPagesZeroedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMPagesZeroed.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintFDeletedLvPagesReclaimedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMFDeletedLvPagesReclaimed.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintZeroRefCountLvsDeletedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMZeroRefCountLvsDeleted.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintIOReadsCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMIOReads.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintIOReadSizeCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMIOReadSize.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintThrottleSettingCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMThrottleSetting.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintIOReReadsCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMIOReReads.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintFollowerSkipsCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMFollowerSkips.PassTo( iInstance, pvBuf );
    return 0;
}

LONG LDBMaintFollowerDivergentCheckedCEFLPv( LONG iInstance, VOID * pvBuf )
{
    perfctrDBMFollowerDivChecked.PassTo( iInstance, pvBuf );
    return 0;
}

#endif // PERFMON_SUPPORT


//  ================================================================
class IDBMScanConfig
//  ================================================================
//
//  Abstract class for scan configuration
//
//-
{
public:
    virtual ~IDBMScanConfig() {}

    // the number of scans that should be performed
    virtual INT CScansMax() = 0;

    // how long the scans should run for
    virtual INT CSecMax() = 0;
    
    // if a scan completes in less than the minimum we wait
    // for the rest of the interval to pass before starting
    // a new scan
    virtual INT     CSecMinScanTime() = 0;

    // read this many pages in one batch
    virtual CPG CpgBatch() = 0;

    // sleep this long between batches of page reads
    virtual DWORD   DwThrottleSleep() = 0;

    // serialize database maintenance
    virtual bool    FSerializeScan() = 0;

    // required to get perf counter information
    virtual INST *  Pinst() = 0;

    // disk id
    virtual ULONG_PTR UlDiskId() const = 0;

    // periodic timeout to notify progress in one pass
    virtual INT     CSecNotifyInterval() = 0;

protected:
    IDBMScanConfig() {}
};


//  ================================================================
class IDBMScanReader
//  ================================================================
//
//  Abstract class for reading pages from the database
//
//-
{
protected:
    IDBMScanReader() {}
    
public:
    virtual ~IDBMScanReader() {}

    virtual ERR InitDBMScanReader() = NULL;

    // issue a preread for a set of pages
    virtual void PrereadPages( const PGNO pgnoFirst, const CPG cpg ) = 0;

    // actually read one page. can return a checksum error, 
    // I/O error or EOF
    virtual ERR ErrReadPage( const PGNO pgno, PIB* const ppib, DBMObjectCache* const pdbmObjectCache ) = 0;

    // indicate that pre-read page is no long needed
    virtual void DoneWithPreread( const PGNO pgno ) = 0;

    // max pgno to scan
    virtual PGNO PgnoLast() const = 0;

    // constants
    enum
    {
        cpgPrereadMax = cbReadSizeMax / g_cbPageMin
    };
};

//  ================================================================
struct DBMScanStats
//  ================================================================
//
//  Holds stats related to the scan, just makes it easier to pass
//  around the collection of stats that is needed by things like
//  DBMScanObserverEvents.
//
//-
{
        DBMScanStats() :
            m_cbAveFreeSpaceLVPages( 0 ),
            m_cbAveFreeSpaceRecPages( 0 ),
            m_cpgLVPagesScanned( 0 ),
            m_cpgRecPagesScanned( 0 )
            {}

        ULONGLONG m_cbAveFreeSpaceLVPages;
        ULONGLONG m_cbAveFreeSpaceRecPages;

        CPG m_cpgLVPagesScanned;
        CPG m_cpgRecPagesScanned;
};

//  ================================================================
class IDBMScanState
//  ================================================================
//
//  The base class for objects which keep track of the state of the
//  scan. This class provides basic state properties and subclasses
//  persist the state in either the database header or the database
//  (possibly adding other properties).
//
//-
{
public:
    virtual ~IDBMScanState() {}

    virtual __int64 FtCurrPassStartTime() const = 0;
    virtual CPG CpgScannedCurrPass() const = 0;
    virtual PGNO PgnoContinuousHighWatermark() const = 0;
    virtual PGNO PgnoHighestChecked() const = 0;
    virtual DBMScanStats DbmScanStatsCurrPass() const = 0;
    
    virtual __int64 FtPrevPassStartTime() const = 0;
    virtual __int64 FtPrevPassCompletionTime() const = 0;
    virtual CPG CpgScannedPrevPass() const = 0;

    // events which can cause the state to be updated
    virtual void StartedPass() = 0;
    virtual void ResumedPass() = 0;
    virtual void FinishedPass() = 0;
    virtual void SuspendedPass() = 0;
    virtual void ReadPages( const PGNO pgnoStart, const CPG cpgRead ) = 0;
    virtual void BadChecksum( const PGNO pgnoBadChecksum ) = 0;
    virtual void CalculatePageLevelStats( const CPAGE& cpage ) = 0;

protected:
    IDBMScanState() {}
};


//  ================================================================
class DBMScanObserver
//  ================================================================
//
//  The base class that all DBMScanObserver classes inherit from. The
//  public methods perform some housekeeping and then call the protected
//  virtual methods. Subclasses can override the virtual methods to
//  receive messages when certain scan events happen.
//
//-
{
public:
    virtual ~DBMScanObserver() {}

    void StartedPass( const IDBMScanState * const pstate );
    void ResumedPass( const IDBMScanState * const pstate );
    void PrepareToTerm( const IDBMScanState * const pstate );
    void FinishedPass( const IDBMScanState * const pstate );
    void SuspendedPass( const IDBMScanState * const pstate );
    void NotifyStats( const IDBMScanState * const pstate );

    void ReadPage( const IDBMScanState * const pstate, const PGNO pgno, DBMObjectCache* const pdbmObjectCache );
    void BadChecksum( const IDBMScanState * const pstate, const PGNO pgnoBadChecksum );

protected:
    virtual void StartedPass_( const IDBMScanState * const pstate ) = 0;
    virtual void ResumedPass_( const IDBMScanState * const pstate ) = 0;
    virtual void PrepareToTerm_( const IDBMScanState * const pstate ) = 0;
    virtual void FinishedPass_( const IDBMScanState * const pstate ) = 0;
    virtual void SuspendedPass_( const IDBMScanState * const pstate ) = 0;
    virtual void ReadPage_( const IDBMScanState * const pstate, const PGNO pgno, DBMObjectCache* const pdbmObjectCache ) = 0;
    virtual void BadChecksum_( const IDBMScanState * const pstate, const PGNO pgnoBadChecksum ) = 0;
    virtual void NotifyStats_( const IDBMScanState * const pstate ) {}
    
protected:
    DBMScanObserver();
    bool FScanInProgress_() const { return m_fScanInProgress; }
    
private:
    bool m_fScanInProgress;
};


//  Forward declaration.
class IDBMScanSerializer;


//  ================================================================
class DBMScan : public IDBMScan
//  ================================================================
//
//  Main scanning object. Owns these objects:
//      - DBMScanConfig: controls scan duration and speed
//      - DBMScanReader: gets the pages from the database
//      - DBMScanState: stores the current state of the scan
//
//  DBMScanObservers can be added to this object, their methods 
//  will be called as the scan progresses.
//
//-
{
    //  CInvasiveList glue.
public:
    static SIZE_T OffsetOfILE() { return OffsetOf( DBMScan, m_ile ); }
private:
    CInvasiveList<DBMScan, OffsetOfILE>::CElement m_ile;

public:
    // these objects will be owned and deleted by the DBMScan
    DBMScan( IDBMScanState * const pscanstate, IDBMScanReader * const pscanreader, IDBMScanConfig * const pscanconfig, PIB* const ppib );
    ~DBMScan();

    // once added, the observer will be owned and deleted by the DBMScan
    void AddObserver( DBMScanObserver * const pscanobserver );

    ERR ErrStartDBMScan();
    void StopDBMScan();
    void TrySetScanGo();
    void TryResetScanGo();
    void WaitForScanStop();
    void WaitForScanGo( const INT msec );

private:
    // thread procedures
    static DWORD DwDBMScanThreadProc_( DWORD_PTR dwContext );
    DWORD DwDBMScan_();

    // starting/stopping
    bool FIsDBMScanRunning_() const { return ( NULL != m_threadDBMScan ); }
    ERR ErrStartDBMScan_();
    void PrepareToTerm_();
    void TermDBMScan_();
    void SetScanStop_();
    void ResetScanStop_();

    // running a pass
    bool FResumingPass_() const;
    void ResumePass_();
    void StartNewPass_();
    void FinishPass_();
    void SuspendPass_();
    void BadChecksum_( const PGNO pgno );
    void PassReadPage_( const PGNO pgno );
    void DoOnePass_();
    void PrereadDone_( _In_ const PGNO pgnoFirst, _In_ const CPG cpgScan );
    void WaitForMinPassTime_();

    // The status methods (ResumePass_, StartNewPass_, etc.) all have to call a member function of all
    // the observers. That functionality is provided by the ForEachObserver_ methods. The first variation
    // calls an observer method which just expects the state. The second variation calls an observer method
    // which expects the state and an additional argument (e.g. the pgno of the page with the bad checksum).
    // The second method is a template method so that the second argument can be of different types (e.g.
    // objid, pgno, cpg). 
    //
    // The pfn declaration might look strange at first:
    //
    //  void (DBMScanObserver::* const pfn)(const IDBMScanState * const)
    //
    // Reading complex declarations should be done from the inside out:
    //
    //  pfn is a const
    //      pointer
    //          to a member of DBMScanObserver
    //              which is a function
    //                  taking 'const IDBMScanState * const'
    //                      and returning void
    //  
    
    void ForEachObserverCall_( void ( DBMScanObserver::* const pfn )( const IDBMScanState * const ) ) const;
    template<class Arg> void ForEachObserverCall_(
        void ( DBMScanObserver::* const pfn )( const IDBMScanState * const, const Arg ),
        const Arg arg ) const;

    template<class Arg, class Arg1> void ForEachObserverCall_(
        void ( DBMScanObserver::* const pfn )( const IDBMScanState * const, const Arg, const Arg1 ),
        const Arg arg, const Arg1 arg1 ) const;

    bool FTimeLimitReached_() const;
    INT CMSecBeforeTimeLimit_() const;
    bool FMaxScansReached_() const;
    
private:
    unique_ptr<IDBMScanConfig>    m_pscanconfig;
    unique_ptr<IDBMScanReader>    m_pscanreader;
    
    // state and observers
    static const INT            m_cscanobserversMax = 8;
    INT m_cscanobservers;

    PIB*                        m_ppib;

    // Caches both open regular table clustered indices and open LV trees, in addition
    // to validity information about secondary indices.
    DBMObjectCache              m_objectCache;

    unique_ptr<IDBMScanState>   m_pscanstate;
    unique_ptr<DBMScanObserver> m_rgpscanobservers[m_cscanobserversMax];

    // thread and concurrency control
    THREAD                      m_threadDBMScan;
    CCriticalSection            m_critSignalControl;
    CManualResetSignal          m_msigDBScanStop;
    CManualResetSignal          m_msigDBScanGo;

    // serialization object
    IDBMScanSerializer *    m_pidbmScanSerializationObj;

    INT m_cscansFinished;
    __int64 m_ftTimeLimit;

    bool m_fNeedToSuspendPass;
    bool m_fSerializeScan;

private: // not implemented
    DBMScan( const DBMScan& );
    DBMScan& operator=( const DBMScan& );
};


//  Forward declaration.
class DBMScanSerializerFactory;


//  ================================================================
class IDBMScanSerializer
//  ================================================================
//
//  This abstract class is used to avoid branching the DBMScan code
//  very often whether JET_paramEnableDBScanSerialization is enabled
//  or not.
//
//-
{
protected:
    //  The factory is a friend.
    friend class DBMScanSerializerFactory;
    
    //  CInvasiveList glue.
public:
    static SIZE_T OffsetOfILE() { return OffsetOf( IDBMScanSerializer, m_ile ); }
private:
    CInvasiveList<IDBMScanSerializer, OffsetOfILE>::CElement m_ile;

public:
    typedef enum
    {
        idbmstypDummy,
        idbmstypReal,
        idbmstypMax,
    } IDBMScanSerializerType;

protected:
    IDBMScanSerializer( const ULONG_PTR ulKey, const IDBMScanSerializerType idbmsType );

public:
    //  Interface.
    virtual ~IDBMScanSerializer();
    virtual bool FEnqueueAndWait( DBMScan * const pdbmScan, const INT cmsec ) = 0;
    virtual void Dequeue( DBMScan * const pdbmScan ) = 0;
    virtual bool FDBMScanCurrent( DBMScan * const pdbmScan ) = 0;
    virtual DWORD DwTimeSlice() const = 0;

    //  Other public methods.
    IDBMScanSerializer::IDBMScanSerializerType GetSerializerType() const;
    ULONG_PTR UlSerializerKey() const;
    LONG CSerializerRef() const;

protected:
    //  Only the factory can use these.
    LONG CIncrementSerializerRef();
    LONG CDecrementSerializerRef();

private:
    //  Object type.
    IDBMScanSerializerType m_idbmsType;

    //  Key.
    ULONG_PTR m_ulKey;

    //  Ref count.
    LONG m_cRef;
};


//  ================================================================
class DBMScanSerializerDummy : public IDBMScanSerializer
//  ================================================================
//
//  This class is a dummy implementation of IDBMScanSerializer
//  which will always succeed in enqueuing but won't do any wait
//  or useful work.
//
//-
{
protected:
    //  The factory is a friend.
    friend class DBMScanSerializerFactory;
    
public:
    //  Interface implementation.
    bool FEnqueueAndWait( DBMScan * const pdbmScan, const INT cmsec );
    void Dequeue( DBMScan * const pdbmScan );
    bool FDBMScanCurrent( DBMScan * const pdbmScan );
    DWORD DwTimeSlice() const;

protected:
    //  Constructor/destructor. Only the factory can use these.
    DBMScanSerializerDummy( const ULONG_PTR ulKey );
    ~DBMScanSerializerDummy();

private:
    //  "Enqueued" object.
    DBMScan * m_pdbmScan;
};


//  ================================================================
class DBMScanSerializer : public IDBMScanSerializer
//  ================================================================
//
//  This class maintains a list of DBMScan objects interested
//  in entering a running state and excuting for a given timeslice.
//
//-
{
protected:
    //  The factory is a friend.
    friend class DBMScanSerializerFactory;
    
private:
    const static DWORD s_cmsecTimeSlice = 10 * 1000;

public:
    //  Interface implementation.
    bool FEnqueueAndWait( DBMScan * const pdbmScan, const INT cmsec );
    void Dequeue( DBMScan * const pdbmScan );
    bool FDBMScanCurrent( DBMScan * const pdbmScan );
    DWORD DwTimeSlice() const;

protected:
    //  Constructor/destructor. Only the factory can use these.
    DBMScanSerializer( const ULONG_PTR ulKey );
    ~DBMScanSerializer();

private:
    //  Synchronization.
    CCriticalSection m_critSerializer;

    //  List of DBMScans.
    CInvasiveList<DBMScan, DBMScan::OffsetOfILE> m_ilDbmScans;
};


//  ================================================================
class DBMScanSerializerFactory
//  ================================================================
//
//  This class maintains a list of DBMScanSerializer objects
//  that will be consumed by DBMScan objects for the purpose of
//  serializing Database Maintenance threads that scan databases
//  that share the same disk. This class works as a factory that
//  returns either DBMScanSerializer or DBMScanSerializerDummy
//  objects. DBMScanSerializerDummy objects are not kept on a list,
//  but will be ref counted for debugging purposes.
//
//-
{
public:
    //  Initialize/terminate the serializer.
    ERR ErrInitSerializerFactory();
    void TermSerializerFactory();

    //  Register/unregister.
    ERR ErrRegisterRealSerializer( const ULONG_PTR ulKey, IDBMScanSerializer ** const ppidbmSerializer );
    ERR ErrRegisterDummySerializer( const ULONG_PTR ulKey, IDBMScanSerializer ** const ppidbmSerializer );
    void UnregisterSerializer( IDBMScanSerializer * const pidbmSerializer );

    //  Return if the list of registered DBMScanSerializers is empty.
    bool FSerializerFactoryEmpty();
    
public:
    //  Constructor/destructor.
    DBMScanSerializerFactory();
    ~DBMScanSerializerFactory();

protected:
    //  Builds different types of objects.
    ERR ErrRegisterSerializer_( const ULONG_PTR ulKey, const bool fDummy, IDBMScanSerializer ** const ppidbmSerializer );

private:
    //  Synchronization.
    CCriticalSection m_critSerializer;

    //  List of DBMScanSerializer objects.
    CInvasiveList<DBMScanSerializer, DBMScanSerializer::OffsetOfILE> m_ilSerializers;

    //  Ref count of DBMScanSerializerDummy objects.
    LONG m_cDummySerializers;
};


//  Global DBM serializer.
LOCAL DBMScanSerializerFactory g_dbmSerializerFactory;


//  ================================================================
class DBMScanReader : public IDBMScanReader
//  ================================================================
//
//  Read pages from the given FMP
//
//-
{
public:
    DBMScanReader( const IFMP ifmp );
    virtual ~DBMScanReader();

    ERR InitDBMScanReader();
    void PrereadPages( const PGNO pgnoFirst, const CPG cpg );
    ERR ErrReadPage( const PGNO pgno, PIB* const ppib, DBMObjectCache* const pdbmObjectCache );
    void DoneWithPreread( const PGNO pgno );
    PGNO PgnoLast() const;

private:
    const IFMP m_ifmp;
    PGNO       m_pgnoLastPreread;
    INT        m_cpgLastPreread;
    TICK       m_tickLastPreread;
    BYTE       m_rgfPageAlreadyCached[IDBMScanReader::cpgPrereadMax];
    BYTE *     m_pvPages;
    BOOL       m_fPagesReRead;
    OBJID      m_objidMSysObjids;
};


//  ================================================================
class DBMSimpleScanState : public IDBMScanState
//  ================================================================
//
//  A simple scan state, which tracks the variables. There are 
//  two subclasses of this, one saves the state to a database header
//  and the other is used for testing the class.
//
//-
{
public:
    DBMSimpleScanState();
    virtual ~DBMSimpleScanState() {}

    virtual __int64 FtCurrPassStartTime() const         { return m_ftCurrPassStartTime; }
    virtual CPG CpgScannedCurrPass() const              { return m_cpgScannedCurrPass; }
    virtual PGNO PgnoContinuousHighWatermark() const    { return m_pgnoContinuousHighWatermark; }
    virtual PGNO PgnoHighestChecked() const             { return m_pgnoHighestChecked; }
    
    virtual __int64 FtPrevPassStartTime() const         { return m_ftPrevPassStartTime; }
    virtual __int64 FtPrevPassCompletionTime() const    { return m_ftPrevPassCompletionTime; }
    virtual CPG CpgScannedPrevPass() const              { return m_cpgScannedPrevPass; }

    virtual void CalculatePageLevelStats( const CPAGE& cpage ) { return; }
    virtual DBMScanStats DbmScanStatsCurrPass() const { return DBMScanStats(); }

    // events which can cause the state to be updated
    virtual void StartedPass();
    virtual void ResumedPass() {}
    virtual void FinishedPass();
    virtual void SuspendedPass() {}
    virtual void ReadPages( const PGNO pgnoStart, const CPG cpgRead );
    virtual void BadChecksum( const PGNO pgnoBadChecksum ) {}
    
protected:
    __int64 m_ftCurrPassStartTime;
    __int64 m_ftPrevPassStartTime;
    __int64 m_ftPrevPassCompletionTime;
    CPG     m_cpgScannedCurrPass;
    PGNO    m_pgnoContinuousHighWatermark;
    PGNO    m_pgnoHighestChecked;
    CPG     m_cpgScannedPrevPass;
    
private:    // not implemented
    DBMSimpleScanState( const DBMSimpleScanState& );
    DBMSimpleScanState& operator=( const DBMSimpleScanState& );
};


//  ================================================================
class DBMScanStateHeader : public DBMSimpleScanState
//  ================================================================
//
//  Sets and retrieves the state in the DB header.
//
//-
{
public:
    DBMScanStateHeader( FMP * const pfmp );
    virtual ~DBMScanStateHeader() {}

    virtual void StartedPass();
    virtual void FinishedPass();
    virtual void SuspendedPass();
    virtual void ReadPages( const PGNO pgnoStart, const CPG cpgRead );

    static void ResetScanStats( FMP * const pfmp );

private:
    void LoadStateFromHeader_();
    void SaveStateToHeader_();
    
private:
    // update the header once this many pages have been scanned
#ifdef ENABLE_JET_UNIT_TEST
    static const CPG m_cpgUpdateInterval = 32;
#else
    static const CPG m_cpgUpdateInterval = 256;
#endif

    // the number of pages that were scanned the last time the
    // header was updated
    CPG     m_cpgScannedLastUpdate;

    // FMP containing the header
    FMP * const m_pfmp;
};


//  ================================================================
PERSISTED struct MSysDatabaseScanRecord
//  ================================================================
//
//  This defines the record format in MSysDatabaseScan. All the 
//  members that start with p_ are stored in MSysDatabaseScan, 
//  according to the given bindings.
//
//-
{
public:
    MSysDatabaseScanRecord();
    ~MSysDatabaseScanRecord();
    
    __int64 p_ftLastUpdateTime;         // time this structure was updated
    
    __int64 p_ftPassStartTime;          // start time of the current pass
    CPG     p_cpgPassPagesRead;         // pages read for this pass
    
    __int64 p_ftPrevPassStartTime;      // start time of the previous pass
    __int64 p_ftPrevPassEndTime;        // end time of the previous pass
    CPG     p_cpgPrevPassPagesRead;     // pages read by the previous pass
    
    LONG    p_csecPassElapsed;          // elapsed seconds this pass
    LONG    p_cInvocationsPass;
    CPG     p_cpgBadChecksumsPass;
    LONG    p_cInvocationsTotal;
    CPG     p_cpgBadChecksumsTotal;

    CPG     p_cpgLVPages;
    CPG     p_cpgRecPages;
	__int64 p_cbFreeLVPages;
	__int64 p_cbFreeRecPages;

    DataSerializer& Serializer() { return m_serializer; }
    
private:
    DataBindingOf<__int64>  m_bindingFtLastUpdateTime;
    DataBindingOf<__int64>  m_bindingFtPassStartTime;
    DataBindingOf<CPG>      m_bindingCpgPassPagesRead;
    
    DataBindingOf<__int64>  m_bindingFtPrevPassStartTime;
    DataBindingOf<__int64>  m_bindingFtPrevPassEndTime;
    DataBindingOf<CPG>      m_bindingCpgPrevPassPagesRead;
    
    DataBindingOf<LONG>     m_bindingCsecPassElapsed;
    DataBindingOf<LONG>     m_bindingCInvocationsPass;
    DataBindingOf<CPG>      m_bindingCpgBadChecksumsPass;
    DataBindingOf<LONG>     m_bindingCInvocationsTotal;
    DataBindingOf<CPG>      m_bindingCpgBadChecksumsTotal;

    DataBindingOf<CPG>      m_bindingCpgLVPages;
    DataBindingOf<CPG>      m_bindingCpgRecPages;
    DataBindingOf<__int64>  m_bindingCbFreeLVPages;
    DataBindingOf<__int64>  m_bindingCbFreeRecPages;

    DataBindings            m_bindings;
    DataSerializer          m_serializer;
};


//  ================================================================
class DBMScanStateMSysDatabaseScan : public IDBMScanState
//  ================================================================
//
//  Sets and retrieves the state in MSysDatabaseScan. Errors 
//  loading/saving the state are ignored.
//
//-
{
public:
    // the store passed into this object will be owned by the object
    // and deleted by its destructor
    DBMScanStateMSysDatabaseScan( const IFMP ifmp, IDataStore * const pstore );
    virtual ~DBMScanStateMSysDatabaseScan();

    virtual __int64 FtCurrPassStartTime() const { return m_record.p_ftPassStartTime; }
    virtual CPG CpgScannedCurrPass() const { return m_record.p_cpgPassPagesRead; }
    virtual PGNO PgnoContinuousHighWatermark() const { return ( PGNO )m_record.p_cpgPassPagesRead; }
    virtual PGNO PgnoHighestChecked() const { return ( PGNO )m_record.p_cpgPassPagesRead; }
    
    virtual __int64 FtPrevPassStartTime() const { return m_record.p_ftPrevPassStartTime; }
    virtual __int64 FtPrevPassCompletionTime() const { return m_record.p_ftPrevPassEndTime; }
    virtual CPG CpgScannedPrevPass() const { return m_record.p_cpgPrevPassPagesRead; }
    virtual DBMScanStats DbmScanStatsCurrPass() const;
    virtual void CalculatePageLevelStats( const CPAGE& cpage );

    virtual void StartedPass();
    virtual void ResumedPass();
    virtual void FinishedPass();
    virtual void SuspendedPass();
    virtual void ReadPages( const PGNO pgnoStart, const CPG cpgRead );
    virtual void BadChecksum( const PGNO pgnoBadChecksum );

private:
    void LoadStateFromTable();
    void SaveStateToTable();
    
protected:
    // update the table once this many pages have been scanned
#ifdef DEBUG
    static const CPG m_cpgUpdateInterval = 32;
#else
    static const CPG m_cpgUpdateInterval = 256;
#endif
    
    // the number of pages that were scanned the last time the
    // header was updated
    CPG m_cpgScannedLastUpdate;

    // Space analysis data
    CReloadableAveStats m_histoCbFreeLVPages;
    CReloadableAveStats m_histoCbFreeRecPages;

    // used to persist items
    MSysDatabaseScanRecord m_record;
    unique_ptr<IDataStore> m_pstore;
    IFMP m_ifmp;
};


//  ================================================================
class DBMScanConfig : public IDBMScanConfig
//  ================================================================
//
//  Configure scans using system parameters.
//
//-
{
public:
    DBMScanConfig( INST * const pinst, FMP * const pfmp );
    virtual ~DBMScanConfig() {}

    virtual INT CScansMax() { return INT_MAX; }
    virtual INT CSecMax() { return INT_MAX; }
    virtual INT CSecMinScanTime();
    virtual CPG CpgBatch();
    virtual DWORD DwThrottleSleep();
    virtual bool FSerializeScan();
    virtual INST * Pinst();
    virtual ULONG_PTR UlDiskId() const;
    virtual INT CSecNotifyInterval() { return 60 * 60 * 6; }

private:
    INST * const m_pinst;
    FMP * const m_pfmp;
};


//  ================================================================
class DBMSingleScanConfig : public DBMScanConfig
//  ================================================================
//
//  Configure a single scan (i.e. JetDatabaseScan) using passed in
//  parameters to override some of the system parameters.
//
//-
{
public:
    DBMSingleScanConfig( INST * const pinst, FMP * const pfmp, const INT csecMax, const DWORD dwThrottleSleep );
    virtual ~DBMSingleScanConfig()  {}

    virtual INT CScansMax()         { return 1; }
    virtual INT CSecMax()           { return m_csecMax; }
    virtual INT CSecMinScanTime()   { return 0; }
    virtual DWORD DwThrottleSleep() { return m_dwThrottleSleep; }
    virtual INST * Pinst()          { return NULL; }

private:
    const INT       m_csecMax;
    const DWORD     m_dwThrottleSleep;
};


//  ================================================================
class DBMScanObserverCallback : public DBMScanObserver
//  ================================================================
//
//  Invoke a callback when a scan finishes.
//
//-
{
public:
    DBMScanObserverCallback(
        const JET_CALLBACK pfnCallback,
        const JET_SESID sesid,
        const JET_DBID dbid );
    virtual ~DBMScanObserverCallback();

protected:
    virtual void StartedPass_( const IDBMScanState * const ) {}
    virtual void ResumedPass_( const IDBMScanState * const )  {}
    virtual void SuspendedPass_( const IDBMScanState * const ) {}
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache ) {}
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO ) {}
    
    virtual void PrepareToTerm_( const IDBMScanState * const ) {}
    virtual void FinishedPass_( const IDBMScanState * const pstate );

private:
    const JET_CALLBACK m_pfnCallback;
    const JET_SESID m_sesid;
    const JET_DBID m_dbid;
};

//  ================================================================
class DBMScanObserverFileCheck : public DBMScanObserver
//  ================================================================
//
//  Do file level operations whenever a scan is started
//
//-
{
public:
    DBMScanObserverFileCheck( const IFMP ifmp );
    virtual ~DBMScanObserverFileCheck() {};

protected:
    virtual void StartedPass_( const IDBMScanState * const );
    virtual void ResumedPass_( const IDBMScanState * const )  {}
    virtual void SuspendedPass_( const IDBMScanState * const ) {}
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache ) {}
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO ) {}
    
    virtual void PrepareToTerm_( const IDBMScanState * const ) {}
    virtual void FinishedPass_( const IDBMScanState * const pstate ) {}

private:
    const IFMP m_ifmp;
};

//  ================================================================
class DBMScanObserverEvents : public DBMScanObserver
//  ================================================================
//
//  Log events when a scan starts and stops.
//
//-
{
public:
    DBMScanObserverEvents(
        FMP * const pfmp,
        const INT csecMaxPassTime = INT_MAX );
    virtual ~DBMScanObserverEvents();

protected:
    virtual void SuspendedPass_( const IDBMScanState * const );
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache );
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO );
    
    virtual void StartedPass_( const IDBMScanState * const pstate );
    virtual void ResumedPass_( const IDBMScanState * const pstate );
    virtual void PrepareToTerm_( const IDBMScanState * const ) {}
    virtual void FinishedPass_( const IDBMScanState * const pstate );
    virtual void NotifyStats_( const IDBMScanState * const pstate );
    
private:
    FMP * Pfmp_() const     { return m_pfmp; }
    INST * Pinst_() const   { return m_pfmp->Pinst(); }

    bool FPassTookTooLong_( const IDBMScanState * const pstate ) const;
    
private:
    const CategoryId m_categoryId;
    const INT m_csecMaxPassTime;
    PGNO m_pgnoStart;
    PGNO m_pgnoLast;
    CPG m_cpgBadChecksums;
    FMP * const m_pfmp;
};


//  ================================================================
class DBMScanObserverPerfmon : public DBMScanObserver
//  ================================================================
//
//  Increments perfmon counters for various events.
//
//-
{
public:
    DBMScanObserverPerfmon( INST * const pinst );
    virtual ~DBMScanObserverPerfmon();

protected:
    virtual void StartedPass_( const IDBMScanState * const );
    virtual void ResumedPass_( const IDBMScanState * const );
    virtual void PrepareToTerm_( const IDBMScanState * const ) {}
    virtual void FinishedPass_( const IDBMScanState * const );
    virtual void SuspendedPass_( const IDBMScanState * const );
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache );
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO pgnoBadChecksum ) {}

private:
    INST * Pinst_() const { return m_pinst; }
    
private:
    INST * const m_pinst;
    __int64 m_ftTimeStart;
};


//  ================================================================
class DBMScanObserverRecoveryEvents : public DBMScanObserver
//  ================================================================
//
//  Log events when a scan starts and stops during recovery.
//
//-
{
public:
    DBMScanObserverRecoveryEvents(
        FMP * const pfmp,
        const INT csecMaxPassTime );
    virtual ~DBMScanObserverRecoveryEvents();

protected:
    virtual void SuspendedPass_( const IDBMScanState * const );
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO );
    
    virtual void StartedPass_( const IDBMScanState * const pstate );
    virtual void ResumedPass_( const IDBMScanState * const pstate );
    virtual void PrepareToTerm_( const IDBMScanState * const ) {}
    virtual void FinishedPass_( const IDBMScanState * const pstate );
    virtual void ReadPage_( const IDBMScanState * const pstate, const PGNO, DBMObjectCache* const pdbmObjectCache );

private:
    FMP * Pfmp_() const { return m_pfmp; }
    INST * Pinst_() const { return m_pfmp->Pinst(); }

    bool FPassIsOverdue_() const;
    void ReportPassIsOverdue_( const IDBMScanState * const pstate ) const;
    void SetPassDeadline_( const IDBMScanState * const pstate );
    void ExtendPassDeadline_( const IDBMScanState * const pstate );

    
private:
    const CategoryId m_categoryId;

    //  Stats
    PGNO m_pgnoStart;
    PGNO m_pgnoLast;
    CPG m_cpgBadChecksums;

    FMP * const m_pfmp;

    // a pass that takes longer than this is overdue
    const INT m_csecMaxPassTime;

    // deadline for scan completion. exceeding this deadline means
    // the scan is overdue
    __int64         m_ftDeadline;
};


//  ================================================================
class DBMScanObserverDebugPrint : public DBMScanObserver
//  ================================================================
//
//  Used for debugging.
//
//-
{
public:
    DBMScanObserverDebugPrint() { printf("%s\n", __FUNCTION__ ); }
    virtual ~DBMScanObserverDebugPrint() { printf("%s\n", __FUNCTION__ ); }

protected:
    virtual void StartedPass_( const IDBMScanState * const ) { printf("%s\n", __FUNCTION__ ); }
    virtual void ResumedPass_( const IDBMScanState * const ) { printf("%s\n", __FUNCTION__ ); }
    virtual void PrepareToTerm_( const IDBMScanState * const ) { printf("%s\n", __FUNCTION__ ); }
    virtual void FinishedPass_( const IDBMScanState * const ) { printf("%s\n", __FUNCTION__ ); }
    virtual void SuspendedPass_( const IDBMScanState * const ) { printf("%s\n", __FUNCTION__ ); }
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO ) { printf("%s\n", __FUNCTION__ ); }
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache ) { printf("%s\n", __FUNCTION__ ); }
};

//  ================================================================
namespace DBMScanObserverCleanupFactory
//  ================================================================
{
    // Create a DBMScanObserverCleanup object. This involves allocating
    // a new PIB.
    ERR ErrCreateDBMScanObserverCleanup(
            PIB * const ppib,
            const IFMP ifmp,
            _Out_ DBMScanObserver ** pobserver );
}

//  ================================================================
class DBMScanObserverCleanup : public DBMScanObserver
//  ================================================================
//
//  Performs logical cleanup on a page.
//
//-
{
public:
    ~DBMScanObserverCleanup();

protected:
    virtual void StartedPass_( const IDBMScanState * const ) {}
    virtual void ResumedPass_( const IDBMScanState * const ) {}
    virtual void PrepareToTerm_( const IDBMScanState * const );
    virtual void FinishedPass_( const IDBMScanState * const );
    virtual void SuspendedPass_( const IDBMScanState * const );
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO ) {}
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache );

protected:
    friend ERR DBMScanObserverCleanupFactory::ErrCreateDBMScanObserverCleanup(
            PIB * const ppib,
            const IFMP ifmp,
            _Out_ DBMScanObserver ** pobserver );
    DBMScanObserverCleanup( PIB * const ppib, const IFMP ifmp );

    ERR ErrGetHighestCommittedObjid_();
    ERR ErrOpenTableGetFucb_( const OBJID objid, BOOL* const pfExists, FUCB ** ppfucbTable );

    bool FIgnorablePage_( const CSR& csr ) const;
    ObjidState OisGetObjectIdState_( CSR& csr, DBMObjectCache* const pdbmObjectCache );
    ERR ErrCleanupUnusedPage_( CSR * const pcsr );
    ERR ErrCleanupDeletedNodes_( CSR * const pcsr );
    ERR ErrScrubOneUsedPage(
            CSR * const pcsr,
            __in_ecount( cscrubOper ) const SCRUBOPER * const rgscruboper,
            const INT cscrubOper );

    ERR ErrDeleteZeroRefCountLV( FCB *pfcbLV, const BOOKMARK& bm );
    ERR ErrCleanupLVPage_( CSR * const pcsr, DBMObjectCache* const pdbmObjectCache );
    ERR ErrZeroLV_( CSR * const pcsr, const INT iline );
    ERR ErrZeroLVChunks_(
            _In_ CSR * const    pcsrRoot,
            const OBJID         objid,
            const LvId          lid,
            const ULONG         ulSize );

    ERR ErrCleanupPrimaryPage_( CSR * const pcsr, DBMObjectCache* const pdbmObjectCache );
    ERR ErrCleanupIndexPage_( CSR * const pcsr );

    VOID RedeleteRevertedFDP_( CSR * const pcsr, DBMObjectCache* const pdbmObjectCache );

protected:
    PIB * m_ppib;
    const IFMP m_ifmp;
    OBJID m_objidMaxCommitted;

    bool m_fPrepareToTerm;

private: // not implemented
    DBMScanObserverCleanup( const DBMScanObserverCleanup& );
    DBMScanObserverCleanup& operator=( const DBMScanObserverCleanup& );
};


//  ================================================================
//  IDBMScan
//  ================================================================

IDBMScan::IDBMScan()
{
}

IDBMScan::~IDBMScan()
{
}

//  ================================================================
//  MSysDBM
//  ================================================================

// namespace members that should not appear in the header
namespace MSysDBM
{
    const char * const szMSysDBM = "MSysDatabaseMaintenance";
}

// Returns true if the given tablename is the DBM system table
bool MSysDBM::FIsSystemTable( const char * const szTable )
{
    return ( 0 == UtilCmpName( szTable, szMSysDBM ) );
}

// Dump the MSysDatabaseMaintenance table
ERR MSysDBM::ErrDumpTable( const IFMP ifmp )
{
    ERR err;

    INST * const pinst = PinstFromIfmp( ifmp );
    const wchar_t * const wszDatabase = g_rgfmp[ifmp].WszDatabaseName();
    
    IDataStore * ptableStore = NULL;
    
    err = TableDataStoreFactory::ErrOpenExisting( pinst, wszDatabase, szMSysDBM, &ptableStore );
    if ( JET_errObjectNotFound == err )
    {
        err = JET_errSuccess;
    }
    else if ( JET_errSuccess == err )
    {
        CPRINTF * const pcprintf = CPRINTFSTDOUT::PcprintfInstance();
        ( *pcprintf )("***************************** %s DUMP ********************************\n", szMSysDBM );

        MemoryDataStore memoryStore;
        MSysDatabaseScanRecord record;

        Call( record.Serializer().ErrLoadBindings( ptableStore ) );
        record.Serializer().Print( pcprintf );

        ( *pcprintf )("\n" );
    }
    Call( err );
    
HandleError:
    delete ptableStore;
    return err;
}


//  ================================================================
//  DBMScanFactory
//  ================================================================

// namespace members that should not appear in the header
namespace DBMScanFactory
{
    ERR ErrPdbmScanCreateForRecovery_( const IFMP ifmp, _Out_ IDBMScan ** pdbmscan );
    ERR ErrPdbmScanCreate_( const IFMP ifmp, _Out_ IDBMScan ** pdbmscan );
}

// Create a scan object to perform a scan during recovery. Cleanup is not performed.
ERR DBMScanFactory::ErrPdbmScanCreateForRecovery_( const IFMP ifmp, _Out_ IDBMScan ** pdbmscan )
{
    ERR err = JET_errSuccess;
    
    INST * const pinst = PinstFromIfmp( ifmp );
    FMP * const pfmp = g_rgfmp+ifmp;
    const INT csecLatePass = ( INT )UlParam( pinst, JET_paramDbScanIntervalMaxSec );
    
    unique_ptr<DBMScanConfig>                     pconfig( new DBMScanConfig( pinst, pfmp ) );
    unique_ptr<DBMScanStateHeader>                pstate( new DBMScanStateHeader( pfmp ) );
    unique_ptr<DBMScanReader>                     preader( new DBMScanReader( ifmp ) );
    unique_ptr<DBMScanObserver>                   pevents( new DBMScanObserverRecoveryEvents( pfmp, csecLatePass ) );
    unique_ptr<DBMScanObserver>                   pperfmon( new DBMScanObserverPerfmon( pinst ) );
    unique_ptr<DBMScanObserver>                   pfilecheck( new DBMScanObserverFileCheck( ifmp ) );

    Alloc( pconfig.get() );
    Alloc( pstate.get() );
    Alloc( preader.get() );
    Call( preader.get()->InitDBMScanReader() );
    Alloc( pevents.get() );
    Alloc( pperfmon.get() );
    Alloc( pfilecheck.get() );
    
    DBMScan * pscan;
    Alloc( pscan = new DBMScan(
            pstate.get(),
            preader.get(),
            pconfig.get(),
            ppibNil ) );
    pstate.release();
    preader.release();
    pconfig.release();

    pscan->AddObserver( pevents.release() );
    pscan->AddObserver( pperfmon.release() );
    pscan->AddObserver( pfilecheck.release() );
    
    *pdbmscan = pscan;
    
HandleError:
    return err;
}

// Create a scan object to perform a scan during runtime. This is used when a
// scan is started at database attach time. 
ERR DBMScanFactory::ErrPdbmScanCreate_( const IFMP ifmp, _Out_ IDBMScan ** pdbmscan )
{
    ERR err = JET_errSuccess;
    
    *pdbmscan = NULL;
    
    INST * const pinst = PinstFromIfmp( ifmp );
    FMP * const pfmp = g_rgfmp+ifmp;
    const wchar_t * const wszDatabase = pfmp->WszDatabaseName();

    PIB * ppib = ppibNil;
    OPERATION_CONTEXT ocDbScan = { OCUSER_DBSCAN, 0, 0, 0, 0 };

    unique_ptr<IDBMScanConfig>    pconfig( new DBMScanConfig( pinst, pfmp ) );
    unique_ptr<IDBMScanReader>    preader( new DBMScanReader( ifmp ) );
    unique_ptr<DBMScanObserver>   pevents( new DBMScanObserverEvents(
        pfmp,
        ( INT )UlParam( pinst, JET_paramDbScanIntervalMaxSec ) ) );
    unique_ptr<DBMScanObserver>   pperfmon( new DBMScanObserverPerfmon( pinst ) );
    unique_ptr<DBMScanObserver>   pfilecheck( new DBMScanObserverFileCheck( ifmp ) );

    unique_ptr<IDataStore>        pstore;
    unique_ptr<IDBMScanState>     pstate;
    unique_ptr<DBMScanObserver>   pcleanup;

    Alloc( pconfig.get() );
    Alloc( preader.get() );
    Call( preader.get()->InitDBMScanReader() );
    Alloc( pevents.get() );
    Alloc( pperfmon.get() );
    Alloc( pfilecheck.get() );

    Call( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
    Call( ppib->ErrSetOperationContext( &ocDbScan, sizeof( ocDbScan ) ) );
    Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );

    ppib->SetFSessionDBScan();

    DBMScanObserver * pcleanupT;
    Call( DBMScanObserverCleanupFactory::ErrCreateDBMScanObserverCleanup( ppib, ifmp, &pcleanupT ) );
    pcleanup = unique_ptr<DBMScanObserver>( pcleanupT );

    IDataStore * pstoreT;
    Call( TableDataStoreFactory::ErrOpenOrCreate( pinst, wszDatabase, MSysDBM::szMSysDBM, &pstoreT ) );
    pstore = unique_ptr<IDataStore>( pstoreT );

    pstate = unique_ptr<IDBMScanState>( new DBMScanStateMSysDatabaseScan( ifmp, pstore.get() ) );
    Alloc( pstate.get() );
    pstore.release();

    Call( ErrFaultInjection( 55221 ) );
    
    DBMScan * pscan;
    Alloc( pscan = new DBMScan(
            pstate.get(),
            preader.get(),
            pconfig.get(),
            ppib ) );
    pstate.release();
    preader.release();
    pconfig.release();

    pscan->AddObserver( pperfmon.release() );
    pscan->AddObserver( pevents.release() );
    pscan->AddObserver( pcleanup.release() );
    pscan->AddObserver( pfilecheck.release() );

    *pdbmscan = pscan;

    // The PIB is now owned by the DBMScan
    ppib = ppibNil;

HandleError:
    if ( ppibNil != ppib )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
        PIBEndSession( ppib );
    }
    return err;
}

// Create a scan object to perform a scan when JetDatabaseScan is called. This also
// takes a callback.
ERR DBMScanFactory::ErrPdbmScanCreateSingleScan(
    const JET_SESID sesid,
    const IFMP ifmp,
    const INT csecMax,
    const INT cmsecSleep,
    const JET_CALLBACK pfnCallback,
    _Out_ IDBMScan ** pdbmscan )
{
    ERR err = JET_errSuccess;
    
    *pdbmscan = NULL;
    
    INST * const pinst = PinstFromIfmp( ifmp );
    FMP * const pfmp = g_rgfmp+ifmp;
    const wchar_t * const wszDatabase = pfmp->WszDatabaseName();

    PIB * ppib = ppibNil;
    OPERATION_CONTEXT ocDbScan = { OCUSER_DBSCAN, 0, 0, 0, 0 };

    unique_ptr<IDBMScanConfig>    pconfig( new DBMSingleScanConfig( pinst, pfmp, csecMax, cmsecSleep ) );
    unique_ptr<IDBMScanReader>    preader( new DBMScanReader( ifmp ) );
    unique_ptr<DBMScanObserver>   pevents( new DBMScanObserverEvents( pfmp ) );
    unique_ptr<DBMScanObserver>   pperfmon( new DBMScanObserverPerfmon( pinst ) );
    unique_ptr<DBMScanObserver>   pcallback( new DBMScanObserverCallback( pfnCallback, sesid, pfmp->Dbid() ) );
    unique_ptr<DBMScanObserver>   pfilecheck( new DBMScanObserverFileCheck( ifmp ) );

    unique_ptr<IDataStore>        pstore;
    unique_ptr<IDBMScanState>     pstate;
    unique_ptr<DBMScanObserver>   pcleanup;

    Alloc( pconfig.get() );
    Alloc( preader.get() );
    Call( preader.get()->InitDBMScanReader() );
    Alloc( pevents.get() );
    Alloc( pperfmon.get() );
    Alloc( pcallback.get() );
    Alloc( pfilecheck.get() );

    Call( ErrPIBBeginSession( pinst, &ppib, procidNil, fFalse ) );
    Call( ppib->ErrSetOperationContext( &ocDbScan, sizeof( ocDbScan ) ) );
    Call( ErrDBOpenDatabaseByIfmp( ppib, ifmp ) );

    ppib->SetFSessionDBScan();

    DBMScanObserver * pcleanupT;
    Call( DBMScanObserverCleanupFactory::ErrCreateDBMScanObserverCleanup( ppib, ifmp, &pcleanupT ) );
    pcleanup = unique_ptr<DBMScanObserver>( pcleanupT );

    IDataStore * pstoreT;
    Call( TableDataStoreFactory::ErrOpenOrCreate( pinst, wszDatabase, MSysDBM::szMSysDBM, &pstoreT ) );
    pstore = unique_ptr<IDataStore>( pstoreT );
    
    pstate = unique_ptr<IDBMScanState>( new DBMScanStateMSysDatabaseScan( ifmp, pstore.get() ) );
    Alloc( pstate.get() );
    pstore.release();

    Call( ErrFaultInjection( 42933 ) );
    
    DBMScan * pscan;
    Alloc( pscan = new DBMScan(
            pstate.get(),
            preader.get(),
            pconfig.get(),
            ppib ) );
    pstate.release();
    preader.release();
    pconfig.release();

    pscan->AddObserver( pperfmon.release() );
    pscan->AddObserver( pevents.release() );
    pscan->AddObserver( pcallback.release() );
    pscan->AddObserver( pcleanup.release() );
    pscan->AddObserver( pfilecheck.release() );
    
    *pdbmscan = pscan;
    
    // The PIB is now owned by the DBMScan
    ppib = ppibNil;

HandleError:
    if ( ppibNil != ppib )
    {
        CallS( ErrDBCloseAllDBs( ppib ) );
        PIBEndSession( ppib );
    }

    return err;
}

// Create a scan object for automatic scan (i.e. a scan not started by JetDatabaseScan, or started by JetDatabaseScan with JET_bitDatabaseScanBatchStartContinuous)
ERR DBMScanFactory::ErrPdbmScanCreate( const IFMP ifmp, _Out_ IDBMScan ** pdbmscan )
{
    *pdbmscan = NULL;
    if ( PinstFromIfmp( ifmp )->FRecovering() )
    {
        return ErrPdbmScanCreateForRecovery_( ifmp, pdbmscan );
    }
    return ErrPdbmScanCreate_( ifmp, pdbmscan );
}


//  ================================================================
//  DBMScanObserver
//  ================================================================

DBMScanObserver::DBMScanObserver() :
    m_fScanInProgress( false )
{
}

// Called when a pass starts. Call the protected method that subclasses override
void DBMScanObserver::StartedPass( const IDBMScanState * const pstate )
{
    Assert( !FScanInProgress_() );
    m_fScanInProgress = true;
    StartedPass_( pstate );
    Assert( FScanInProgress_() );
}

// Called when a pass resumes. Call the protected method that subclasses override
void DBMScanObserver::ResumedPass( const IDBMScanState * const pstate )
{
    Assert( !FScanInProgress_() );
    m_fScanInProgress = true;
    ResumedPass_( pstate );
    Assert( FScanInProgress_() );
}

// Called when preparing to terminate. Calls the protected method that subclasses override.
void DBMScanObserver::PrepareToTerm( const IDBMScanState * const pstate )
{
    PrepareToTerm_( pstate );
}

// Called when a pass finishes. Call the protected method that subclasses override
void DBMScanObserver::FinishedPass( const IDBMScanState * const pstate )
{
    Assert( FScanInProgress_() );
    FinishedPass_( pstate );
    m_fScanInProgress = false;
    Assert( !FScanInProgress_() );
}

// Called when a pass is suspended. Call the protected method that subclasses override
void DBMScanObserver::SuspendedPass( const IDBMScanState * const pstate )
{
    Assert( FScanInProgress_() );
    SuspendedPass_( pstate );
    m_fScanInProgress = false;
    Assert( !FScanInProgress_() );
}

// Called inside a pass when a certain interval is passed.
void DBMScanObserver::NotifyStats( const IDBMScanState * const pstate )
{
    Assert( FScanInProgress_() );
    NotifyStats_( pstate );
}

// Called when a page is read. Call the protected method that subclasses override
void DBMScanObserver::ReadPage( const IDBMScanState * const pstate, const PGNO pgno, DBMObjectCache* const pdbmObjectCache )
{
    Assert( FScanInProgress_() );
    ReadPage_( pstate, pgno, pdbmObjectCache );
}

// Called when a bad checksum is found. Call the protected method that subclasses override
void DBMScanObserver::BadChecksum( const IDBMScanState * const pstate, const PGNO pgnoBadChecksum )
{
    Assert( FScanInProgress_() );
    BadChecksum_( pstate, pgnoBadChecksum );
}


//  ================================================================
//  DBMSimpleScanState
//  ================================================================

DBMSimpleScanState::DBMSimpleScanState() :
    m_ftCurrPassStartTime( 0 ),
    m_ftPrevPassStartTime( 0 ),
    m_ftPrevPassCompletionTime( 0 ),
    m_cpgScannedCurrPass( 0 ),
    m_pgnoContinuousHighWatermark( 0 ),
    m_cpgScannedPrevPass( 0 ),
    m_pgnoHighestChecked( 0 )
{
}

// when a pass starts the start time should be recorded
void DBMSimpleScanState::StartedPass()
{
    m_ftCurrPassStartTime = UtilGetCurrentFileTime();
    m_cpgScannedCurrPass = 0;
    m_pgnoHighestChecked = 0;
}

// when a pass completes the completion time should be recorded
void DBMSimpleScanState::FinishedPass()
{
    m_ftPrevPassStartTime = m_ftCurrPassStartTime;
    m_cpgScannedPrevPass = m_pgnoHighestChecked;
    m_ftPrevPassCompletionTime = UtilGetCurrentFileTime();
    m_ftCurrPassStartTime = 0;
    m_pgnoContinuousHighWatermark = pgnoNull;
    m_cpgScannedCurrPass = pgnoNull;
    m_pgnoHighestChecked = pgnoNull;
}

// when pages are read increment the number of scanned pages
// in current pass and the counter for the highest page scanned
// so far
void DBMSimpleScanState::ReadPages( const PGNO pgnoStart, const CPG cpgRead )
{
    m_cpgScannedCurrPass += cpgRead;

    // We will only update this counter if we incremented by 1 page, meaning we have continuously covered
    // every page.  This has some ramifications, the most significant is, if redo (i.e. repl) crashes, 
    // we will not be able to prove we covered every page, so we won't log a true finished event. We will
    // however, save the state (as long as it's continuous) and make steady progress towards a true finish
    if ( ( pgnoStart - m_pgnoContinuousHighWatermark ) == 1 )
    {
        m_pgnoContinuousHighWatermark += cpgRead;
    }

    if ( ( pgnoStart + ( cpgRead - 1 ) ) > m_pgnoHighestChecked )
    {
        m_pgnoHighestChecked = pgnoStart + ( cpgRead - 1 );
    }
}


//  ================================================================
//  DBMScanReader
//  ================================================================

DBMScanReader::DBMScanReader( const IFMP ifmp ) :
    m_ifmp( ifmp ),
    m_pgnoLastPreread( 0 ),
    m_cpgLastPreread( 0 ),
    m_pvPages( NULL ),
    m_fPagesReRead( fFalse ),
    m_objidMSysObjids( objidNil )
{
}

DBMScanReader::~DBMScanReader()
{
#ifdef DEBUG
    // Verify that all previously preread pages have had DoneWithPreread called
    for ( INT i = 0; i < m_cpgLastPreread; i++ )
    {
        Assert( m_rgfPageAlreadyCached[i] == 'D' );
    }
#endif // DEBUG

    if ( m_pvPages != NULL )
    {
        OSMemoryPageFree( m_pvPages );
        m_pvPages = NULL;
    }
}

ERR
DBMScanReader::InitDBMScanReader()
{
    ERR err = JET_errSuccess;
    Alloc( m_pvPages = ( BYTE * )PvOSMemoryPageAlloc( g_cbPage * cpgPrereadMax, NULL ) );

HandleError:
    return err;
}

// Issue a preread for a set of pages.  
void DBMScanReader::PrereadPages( const PGNO pgnoFirst, const CPG cpg )
{
    Assert( cpg <= cpgPrereadMax );
    Assert( cpg > 0 );

#ifdef DEBUG
    // Verify that all previously preread pages have had DoneWithPreread called
    for ( INT i = 0; i < m_cpgLastPreread; i++ )
    {
        Assert( m_rgfPageAlreadyCached[i] == 'D' );
        m_rgfPageAlreadyCached[i] = 'U';
    }
#endif // DEBUG
    
    m_pgnoLastPreread = pgnoFirst;
    m_cpgLastPreread = cpg;
    m_tickLastPreread = TickOSTimeCurrent();    // for debugging ...

    TraceContextScope tcScope( iortDbScan );
    tcScope->nParentObjectClass = tceNone;
    BFPrereadPageRange( m_ifmp, pgnoFirst, cpg, NULL, m_rgfPageAlreadyCached, bfprfDefault | bfprfDBScan, BfpriBackgroundRead( m_ifmp, ppibNil ), *tcScope );
    g_rgfmp[m_ifmp].UpdatePgnoPreReadScanMax( pgnoFirst + cpg - 1 );

    INT cpgRun = 0;
    for ( INT i = 0; i <= m_cpgLastPreread; i++ )
    {
        if ( i == m_cpgLastPreread || m_rgfPageAlreadyCached[i] )
        {
            if ( cpgRun )
            {
                PERFOpt( perfctrDBMIOReads.Inc( PinstFromIfmp( m_ifmp ) ) );
                PERFOpt( perfctrDBMIOReadSize.Add( PinstFromIfmp( m_ifmp ), cpgRun * g_cbPage ) );
                cpgRun = 0;
            }
        }
        else
        {
            cpgRun++;
        }
    }
}

//  This is a function to consolidate the ways in which we dump time to the event logs, so that the datacenter optics
//  we build upon this will be more likely to succeed.

VOID DBMScanFormatOpticalTime_( const __int64 ft, __out_ecount( cchLastFullCompletionTime ) WCHAR * wszLastFullCompletionTime, const ULONG cchLastFullCompletionTime )
{
    Assert( cchLastFullCompletionTime >= 2 );
    if ( ft )
    {
        size_t cwchRequired;
        CallS( ErrUtilFormatFileTimeAsDate( ft, wszLastFullCompletionTime, cchLastFullCompletionTime, &cwchRequired ) );
        ULONG ichCurr = LOSStrLengthW( wszLastFullCompletionTime );
        if ( ichCurr < cchLastFullCompletionTime - 2 )  // should always be true
        {
            wszLastFullCompletionTime[ichCurr] = L' ';
            wszLastFullCompletionTime[ichCurr+1] = L'\0';
            ichCurr++;
        }
        Assert( ichCurr == (size_t)LOSStrLengthW( wszLastFullCompletionTime ) );
        CallS( ErrUtilFormatFileTimeAsTimeWithSeconds( ft,
                        wszLastFullCompletionTime + ichCurr,
                        cchLastFullCompletionTime - ichCurr,
                        &cwchRequired ) );
    }
    else
    {
        OSStrCbCopyW( wszLastFullCompletionTime, cchLastFullCompletionTime, L"-" );
    }
}

CDBMScanFollower::CDBMScanFollower()
{
    m_pgnoHighestContigCompleted = pgnoNull;
    m_pstate = NULL;
    m_pscanobsFileCheck = NULL;
    m_pscanobsLgriEvents = NULL;
}

CDBMScanFollower::~CDBMScanFollower()
{
    // shouldn't have any memory leaks
    Assert( m_pstate == NULL );
    Assert( m_pscanobsFileCheck == NULL );
    Assert( m_pscanobsLgriEvents == NULL );
}

ERR CDBMScanFollower::ErrRegisterFollower( FMP * const pfmp, const PGNO pgnoStart )
{
    ERR                         err = JET_errSuccess;
    DBMScanObserver *           pscanobsPerfmon = NULL;
    DBMScanObserver *           pscanobsLgriEvents = NULL;
    DBMScanObserver *           pscanobsFileCheck = NULL;
    IDBMScanState *             pstate = NULL;

    if ( m_pstate )
    {
#ifdef ENABLE_JET_UNIT_TEST
        //  setup by test hook
        pstate = m_pstate;
        Alloc( pscanobsLgriEvents = new DBMScanObserverRecoveryEvents( pfmp, 2 * 60 ) );
        Alloc( pscanobsPerfmon = new DBMScanObserverPerfmon( NULL ) );
#else
        AssertSz( fFalse, "We shouldn't be re-registering twice w/o deregistering first!" );
        return ErrERRCheck( errCodeInconsistency );
#endif
    }
    else
    {
        const IFMP ifmp = pfmp->Ifmp();
        Assert( ( ( INT_PTR )ifmp ) >= cfmpReserved && ifmp < g_ifmpMax );
        Alloc( pstate = new DBMScanStateHeader( pfmp ) );   //  loads state from header.
        Alloc( pscanobsFileCheck = new DBMScanObserverFileCheck( ifmp ) );
        Alloc( pscanobsLgriEvents = new DBMScanObserverRecoveryEvents( pfmp , ( INT )UlParam( pfmp->Pinst(), JET_paramDbScanIntervalMaxSec ) ) );
        Alloc( pscanobsPerfmon = new DBMScanObserverPerfmon( pfmp->Pinst() ) );
    }

    m_pstate = pstate;
    pstate = NULL;
    m_pscanobsFileCheck = pscanobsFileCheck;
    pscanobsFileCheck = NULL;
    m_pscanobsLgriEvents = pscanobsLgriEvents;
    pscanobsLgriEvents = NULL;
    m_pscanobsPerfmon = pscanobsPerfmon;
    pscanobsPerfmon = NULL;

    //  Must initialize m_pgnoHighestContigCompleted to allow our tracking of the current progress towards
    //  a complete DBScan pass
    m_pgnoHighestContigCompleted = m_pstate->PgnoContinuousHighWatermark();

    if( pgnoStart == 1 )
    {
        m_pstate->StartedPass();
        if ( m_pscanobsFileCheck )
        {
            m_pscanobsFileCheck->StartedPass( m_pstate );
        }
        m_pscanobsLgriEvents->StartedPass( m_pstate );
        m_pscanobsPerfmon->StartedPass( m_pstate );
        PERFOpt( m_cFollowerSkipsPrePass = perfctrDBMFollowerSkips.Get( pfmp->Pinst() ) );
        PERFOpt( perfctrDBMFollowerDivChecked.Set( pfmp->Pinst(), 0 ) );
    }
    else
    {
        if ( pgnoStart == pgnoScanLastSentinel || m_pgnoHighestContigCompleted < pgnoStart - 1 )
        {
            WCHAR wszCpgSkipped[32];
            WCHAR wszCpgPctFile[48];
            WCHAR wszPgnoLast[32];
            WCHAR wszPgnoStart[32];

            const CPG cpgOwned = (CPG)pfmp->PgnoLast();
            Assert( cpgOwned > 0 );
            CPG cpgSkipped = -1;
            PGNO pgnoStartReported = pgnoStart;
            if ( pgnoStart != pgnoScanLastSentinel )
            {
                cpgSkipped = pgnoStart - m_pgnoHighestContigCompleted;
            }
            else
            {
                //  The unlikely event that the very first ScanCheck LR we see is the actual sentinel, so we haven't
                //  actually started a proper scan yet.
                Assert( pgnoStartReported == pgnoScanLastSentinel );    // well that doesn't make much sense, so fix it ...
                pgnoStartReported = m_pgnoHighestContigCompleted;

                Assert( cpgOwned >= m_pstate->CpgScannedCurrPass() );
                cpgSkipped = cpgOwned - m_pgnoHighestContigCompleted;
                if ( cpgSkipped < 1 )
                {
                    cpgSkipped = 1; // hack to avoid assert 3 lines below, just in case.
                }
            }
            const double pctFile = double( cpgSkipped ) * 100.0 / double( cpgOwned );
            Assert( cpgSkipped >= 1 );

            OSStrCbFormatW( wszCpgSkipped, sizeof( wszCpgSkipped ), L"%d", cpgSkipped );
            OSStrCbFormatW( wszCpgPctFile, sizeof( wszCpgPctFile ), L" / %d = %5.2f", cpgOwned, pctFile );
            OSStrCbFormatW( wszPgnoLast, sizeof( wszPgnoLast ), L"%d (0x%x)", m_pgnoHighestContigCompleted, m_pgnoHighestContigCompleted );
            OSStrCbFormatW( wszPgnoStart, sizeof( wszPgnoStart ), L"%d (0x%x)", pgnoStart, pgnoStart );

            const WCHAR* rgwsz[] = { pfmp->WszDatabaseName(), wszCpgSkipped, wszCpgPctFile, wszPgnoLast, wszPgnoStart };
            UtilReportEvent( eventInformation, ONLINE_DEFRAG_CATEGORY,
                SCAN_CHECKSUM_RESUME_SKIPPED_PAGES_ID, _countof( rgwsz ), rgwsz, 0, NULL, pfmp->Pinst() );
        }
        m_pstate->ResumedPass();
        m_pscanobsLgriEvents->ResumedPass( m_pstate );
        m_pscanobsPerfmon->ResumedPass( m_pstate );
    }

    // PgnoContinuousHighWatermark() should only get reset during FinishedPass()
    Assert( m_pstate->PgnoContinuousHighWatermark() == m_pgnoHighestContigCompleted );

HandleError:

    delete pstate;
    delete pscanobsFileCheck;
    delete pscanobsLgriEvents;
    delete pscanobsPerfmon;

    return err;
}

VOID CDBMScanFollower::DeRegisterFollower( const FMP * const pfmp, const DBMScanDeRegisterReason eDeRegisterReason )
{
    switch( eDeRegisterReason )
    {
        case dbmdrrFinishedScan:

            //  A pass should only be considered complete if we truly started from the beginning and
            //  checksummed every matching page the active sent down.
            if ( m_pstate->PgnoHighestChecked() == m_pgnoHighestContigCompleted )
            {
                //  We completed the pass, examining and checksumming every pgno provied from 1 to the
                //  DB end.
                Assert( m_pstate->PgnoContinuousHighWatermark() == m_pgnoHighestContigCompleted );
                m_pstate->FinishedPass();
                m_pscanobsLgriEvents->FinishedPass( m_pstate );
                m_pscanobsPerfmon->FinishedPass( m_pstate );
            }
            else
            {
                //  We completed the pass, but DID NOT examine and checksum every page due to background
                //  DB maintenance being disabled for some period of time.
                //  It seems weird to hide this in a func: m_pscanobsLgriEvents->IncompleteFinishPass( m_pstate );,
                //  that would apply to no other observers, so I'll just log an event.
                WCHAR wszLastFullCompletionTime[100];
                DBMScanFormatOpticalTime_( m_pstate->FtPrevPassCompletionTime(), wszLastFullCompletionTime, _countof( wszLastFullCompletionTime ) );
                const WCHAR* rgwsz[] = { pfmp->WszDatabaseName(), wszLastFullCompletionTime };
                UtilReportEvent( eventInformation, ONLINE_DEFRAG_CATEGORY,
                    SCAN_CHECKSUM_INCOMPLETE_PASS_FINISHED_ID, _countof( rgwsz ), rgwsz, 0, NULL, pfmp->Pinst() );

                //  Not technically accurate but leaves the pass variable at the suspended 
                //  value of 1, which is more interesting.
                m_pscanobsPerfmon->SuspendedPass( m_pstate );
            }
            break;

        case dbmdrrStoppedScan:
            // We should still be at the highest contig, as SuspendedPass() will save the state
            Assert( m_pstate->PgnoContinuousHighWatermark() == m_pgnoHighestContigCompleted );
            m_pstate->SuspendedPass();
            //  We don't report it to observers, b/c we don't want an event for this case.
            //m_pscanobsLgriEvents->SuspendedPass( m_pstate );
            m_pscanobsPerfmon->SuspendedPass( m_pstate );
            break;

        case dbmdrrDisabledScan:

            // We should still be at the highest contig, as SuspendedPass() will save the state
            Assert( m_pstate->PgnoContinuousHighWatermark() == m_pgnoHighestContigCompleted );
            m_pstate->SuspendedPass();
            m_pscanobsLgriEvents->SuspendedPass( m_pstate );
            m_pscanobsPerfmon->SuspendedPass( m_pstate );
            break;

        default:
            AssertSz( fFalse, "DeRegisterReason %d Unknown!\n", eDeRegisterReason );
    }

    delete m_pscanobsPerfmon;
    m_pscanobsPerfmon = NULL;

    delete m_pscanobsLgriEvents;
    m_pscanobsLgriEvents = NULL;

    delete m_pscanobsFileCheck;
    m_pscanobsFileCheck = NULL;

#ifdef ENABLE_JET_UNIT_TEST
    //  m_pstate setup by test hook, don't free
#else
    delete m_pstate;
#endif
    m_pstate = NULL;
}

BOOL CDBMScanFollower::FStopped() const
{
    //  If we don't have any m_pstate, we're either not started or stopped.
    return m_pstate == NULL;
}

VOID CDBMScanFollower::CompletePage( const PGNO pgno, const BOOL fBadPage )
{
    // I can conceive of a way this would go off, but probably it will be a bug the first time it does.
    // These can happen in mixed upgrade env, which I was testing in for compat ... ugh, ok, leaving off for now.
    //Expected( pgno != 1 || m_pgnoHighestContigCompleted == 0 /* should be starting first pass if pgno == 1 */ );
    //if ( m_pstate->PgnoHighestChecked() )
    //  {
    //  //  Once we've started checking, we should never go backwards without destroying 
    //  //  the CDBMScanFollower object and Re-Registering a new follower.
    //  Assert( m_pstate->PgnoHighestChecked() < pgno );
    //  }

    // This will increment the CpgScannedCurrPass counter, set the PgnoHighestChecked
    // tracker to this pgno (unless it's already higher), and if it's continuous it will
    // update the PgnoContinuousHighWatermark counter
    m_pstate->ReadPages( pgno, 1 );
    Assert( m_pstate->PgnoHighestChecked() >= pgno );

    //  Log an event if we are taking too long in this scan
    m_pscanobsLgriEvents->ReadPage( m_pstate, pgno, NULL );

    // We will only update state if we incremented by 1 page, meaning we have continuously covered
    // every page.  This has some ramifications, the most significant is, if redo (i.e. repl) crashes, 
    // we will not be able to prove we covered every page, so we won't log a true finished event. We will
    // however, save the state (as long as it's continuous) and make steady progress towards a true finish
    if ( ( pgno - m_pgnoHighestContigCompleted ) == 1 )
    {
        m_pgnoHighestContigCompleted = pgno;
    }

    Assert( m_pstate->PgnoContinuousHighWatermark() == m_pgnoHighestContigCompleted );

    m_pscanobsPerfmon->ReadPage( m_pstate, pgno, NULL );

    if ( fBadPage )
    {
        m_pscanobsPerfmon->BadChecksum( m_pstate, pgno );
    }
}

VOID CDBMScanFollower::UpdateIoTracking( const IFMP ifmp, const CPG cpg )
{
    Assert( cpg > 0 );
    PERFOpt( perfctrDBMIOReads.Inc( PinstFromIfmp( ifmp ) ) );
    PERFOpt( perfctrDBMIOReadSize.Add( PinstFromIfmp( ifmp ), cpg * g_cbPage ) );
}

ERR ErrDBMScanReadThroughCache_( const IFMP ifmp, const PGNO pgno, void * pvPages, const CPG cpg, const TraceContext& tc )
{
    ERR err = JET_errSuccess;

    CPG cpgActual;
    err = g_rgfmp[ ifmp ].ErrDBReadPages( pgno, pvPages, cpg, &cpgActual, tc, fTrue );

    PERFOpt( perfctrDBMIOReReads.Inc( PinstFromIfmp( ifmp ) ) );

    return err;
}

ERR CDBMScanFollower::ErrDBMScanReadThroughCache( const IFMP ifmp, const PGNO pgno, void * pvPages, const CPG cpg )
{
    TraceContextScope tcScope( iorpDbScan );
    return ErrDBMScanReadThroughCache_( ifmp, pgno, pvPages, cpg, *tcScope );
}

// Increment the perf counter for skipped pages, and check if we need to reset the header state
// Even if scanning is disabled or we are re-scanning things (which is how we get here), we want
// to reset the header state when we hit pgno=1
VOID CDBMScanFollower::SkippedDBMScanCheckRecord( FMP * const pfmp, const PGNO pgno )
{
    // increment the perf counter
    PERFOpt( perfctrDBMFollowerSkips.Add( pfmp->Pinst(), 1 ) );

    if ( pgno == 1 )
    {
        DBMScanStateHeader::ResetScanStats( pfmp );
    }
}

VOID CDBMScanFollower::ProcessedDBMScanCheckRecord( const FMP * const pfmp )
{
    PERFOpt( perfctrDBMFollowerDivChecked.Add( pfmp->Pinst(), 1 ) );
}

// Try to read a page.
ERR DBMScanReader::ErrReadPage( const PGNO pgno, PIB* const ppib, DBMObjectCache* const pdbmObjectCache )
{
    ERR err;

    // Verify that reads have all been pre-read.
    Assert( pgno >= m_pgnoLastPreread );
    Assert( pgno < m_pgnoLastPreread + m_cpgLastPreread );
    Assert( m_rgfPageAlreadyCached[pgno - m_pgnoLastPreread] == fTrue ||
           m_rgfPageAlreadyCached[pgno - m_pgnoLastPreread] == fFalse );
    Assert( ppib != ppibNil || PinstFromIfmp( m_ifmp )->FRecovering() );

    g_rgfmp[ m_ifmp ].UpdatePgnoScanMax( pgno );

    // If we use a shared latch, this page may not be verified due to latch conflicts, which would cause
    // the corruption event to be discarded. The case we've seen was that the DBM thread had the shared latch
    // and the eviction code had the exclusive latch at first, so it returned the error, but did not fire the
    // event. In addition to that, the error was not be saved to the BF. So, now we grab the RDW latch to force
    // having the ability to verify the page and fire the event.

    TraceContextScope tcScope( iortDbScan );
    tcScope->nParentObjectClass = tceNone;

    // If we aren't aware of the objid of MsysObjids we will compute it before we latch the page to avoid deadlocking.
    if ( m_objidMSysObjids == objidNil && !PinstFromIfmp( m_ifmp )->FRecovering() )
    {
        Call( ErrCATSeekTable( ppib, m_ifmp, szMSObjids, NULL, &m_objidMSysObjids ) );
        Assert( m_objidMSysObjids != objidNil );
    }

    BFLatch bfl;
    Call( ErrBFRDWLatchPage( &bfl, m_ifmp, pgno, BFLatchFlags( bflfNoTouch | bflfNoFaultFail | bflfUninitPageOk | bflfExtensiveChecks | bflfDBScan ), BfpriBackgroundRead( m_ifmp, ppibNil ), *tcScope ) );
    Assert( FBFRDWLatched( m_ifmp, pgno ) );
    const ERR errPageStatus = ErrBFLatchStatus( &bfl );
    if( errPageStatus < JET_errSuccess && errPageStatus != JET_errPageNotInitialized )
    {
        BFRDWUnlatch( &bfl );
        Call( errPageStatus );
    }
    const BOOL fPageUninit = ( errPageStatus == JET_errPageNotInitialized );

    OnDebug( const ULONG_PTR bitDbScan = UlParam( PinstFromIfmp( m_ifmp ), JET_paramEnableDBScanInRecovery ) );
    Expected( !PinstFromIfmp( m_ifmp )->m_plog->FRecovering() ||
                ( bitDbScan & bitDBScanInRecoveryPassiveScan ) ||
                ( 0 == bitDbScan ) );
    
    if ( !PinstFromIfmp( m_ifmp )->FRecovering() &&
         ( BoolParam( PinstFromIfmp( m_ifmp ), JET_paramEnableExternalAutoHealing ) ) )
    {
        CPAGE cpage;

        // The page should checksum and be valid or uninitialized, or we shouldn't log it
        Assert( ErrBFLatchStatus( &bfl ) >= JET_errSuccess || ErrBFLatchStatus( &bfl ) == JET_errPageNotInitialized );

        Assert( !PinstFromIfmp( m_ifmp )->m_plog->FRecovering() );

        cpage.ReBufferPage( bfl, m_ifmp, pgno, bfl.pv, ( ULONG )UlParam( PinstFromIfmp( m_ifmp ), JET_paramDatabasePageSize ) );
        Assert( cpage.CbPage() == UlParam( PinstFromIfmp( m_ifmp ), JET_paramDatabasePageSize ) );
        Assert( !fPageUninit || cpage.Dbtime() == 0 /* must be zero if page is non-init, for redo code to work */ );
        Assert( cpage.Dbtime() > 0 || cpage.Dbtime() == dbtimeShrunk || cpage.Dbtime() == dbtimeRevert || fPageUninit );
        Assert( cpage.Dbtime() > dbtimeStart || cpage.Dbtime() == dbtimeShrunk || cpage.Dbtime() == dbtimeRevert || fPageUninit );
        Assert( cpage.Dbtime() != dbtimeNil );
        Assert( cpage.Dbtime() != dbtimeInvalid );
        Assert( cpage.ObjidFDP() != objidNil || cpage.Dbtime() == dbtimeShrunk || cpage.Dbtime() == dbtimeRevert || fPageUninit );
        OBJID objid             = cpage.ObjidFDP();
        ObjidState objidState   = ois::Unknown;

        if ( objid != objidNil )
        {
            if ( FCATISystemObjid( objid ) || objid == m_objidMSysObjids )
            {
                objidState = ois::ValidNoFUCB;
            }
            else
            {
                objidState = pdbmObjectCache->OisGetObjidState( objid );

                if ( objidState == ois::Unknown )
                {
                    OBJID objidTable = objidNil;
                    SYSOBJ sysobj = sysobjNil;

                    Call( ErrCATGetObjidMetadata( ppib, m_ifmp, objid, &objidTable, &sysobj ) );
                    if ( sysobj != sysobjNil )
                    {
                        pdbmObjectCache->MarkCacheObjectValidNoFUCB( objid );
                        objidState = ois::ValidNoFUCB;
                    }
                    else
                    {
                        objidState = ois::Invalid;
                    }
                }
            }
        }
        else
        {
            // Page has no objid set. We will consider the objidState as invalid.
            objidState = ois::Invalid;
        }

        (void)ErrDBMEmitDivergenceCheck(
            m_ifmp,
            pgno,
            scsDbScan,
            &cpage,
            objidState );

        cpage.UnloadPage();
    }


    BFRDWUnlatch( &bfl );

    // Also, if the page was already cached in buffer manager, also read from
    // disk
    if ( !m_fPagesReRead &&
         m_rgfPageAlreadyCached[pgno - m_pgnoLastPreread] )
    {
        // figure out how many pages to read
        PGNO pgnoLast = pgno;
        for ( INT i = pgno - m_pgnoLastPreread + 1; i < m_cpgLastPreread; i++ )
        {
            if ( m_rgfPageAlreadyCached[i] )
            {
                pgnoLast = m_pgnoLastPreread + i;
            }
        }
        Assert( pgnoLast >= pgno );
        Assert( pgnoLast - pgno + 1 <= cpgPrereadMax );

        // read the actual pages of data off disk

        TraceContextScope tcScopeT( iorpDbScan );
        err = ErrDBMScanReadThroughCache_( m_ifmp, pgno, m_pvPages, pgnoLast - pgno + 1/*cpg*/, *tcScopeT );

        // Either everything was successful or
        // BF marked filthy (or page request issued in case BF evicted already)
        m_fPagesReRead = fTrue;
    }

HandleError:

    Expected( err != JET_errFileIOBeyondEOF );

    return err;
}

void DBMScanReader::DoneWithPreread( const PGNO pgno )
{
    const INT ipgno = pgno - m_pgnoLastPreread;

    // Verify that reads have all been pre-read.
    Assert( pgno >= m_pgnoLastPreread );
    Assert( pgno < m_pgnoLastPreread + m_cpgLastPreread );
    Assert( m_rgfPageAlreadyCached[ipgno] == fTrue ||
           m_rgfPageAlreadyCached[ipgno] == fFalse );

    // Reset m_fPagesReRead only if it's the last page of the batch.

    if ( ( ipgno + 1 ) == m_cpgLastPreread )
    {
        m_fPagesReRead = fFalse;
    }

    // Any pages that we read in just for scanning, we can fast-evict

    if ( m_rgfPageAlreadyCached[ipgno] == fFalse )
    {
        BFMarkAsSuperCold( m_ifmp, pgno, bflfDBScan );
    }
    else
    {
        Assert( m_rgfPageAlreadyCached[ipgno] == fTrue );
    }

#ifdef DEBUG
    // Mark the value as Done so that asserts will fire if we try to read the page again
    m_rgfPageAlreadyCached[ipgno] = ( BOOL )'D';
#endif // DEBUG

    return;
}

PGNO DBMScanReader::PgnoLast() const
{
    return g_rgfmp[m_ifmp].PgnoLast();
}


//  ================================================================
//  DBMScanStateHeader
//  ================================================================

DBMScanStateHeader::DBMScanStateHeader( FMP * const pfmp ) :
    m_pfmp( pfmp ),
    m_cpgScannedLastUpdate( -1 )
{
    LoadStateFromHeader_();
    Assert( -1 != m_cpgScannedLastUpdate );
}

void DBMScanStateHeader::StartedPass()
{
    // need to call the inherited method so the counts are updated
    DBMSimpleScanState::StartedPass();
    m_cpgScannedLastUpdate = 0;
}

void DBMScanStateHeader::FinishedPass()
{
    // need to call the inherited method so the counts are updated
    DBMSimpleScanState::FinishedPass();
    SaveStateToHeader_();
}

void DBMScanStateHeader::SuspendedPass()
{
    // need to call the inherited method so the counts are updated
    DBMSimpleScanState::SuspendedPass();
    SaveStateToHeader_();
}

void DBMScanStateHeader::ReadPages( const PGNO pgnoStart, const CPG cpgRead )
{
    // need to call the inherited method so the counts are updated
    DBMSimpleScanState::ReadPages( pgnoStart, cpgRead );

    Assert( ( CPG )m_pgnoHighestChecked >= m_cpgScannedLastUpdate );
    if( ( m_pgnoHighestChecked - m_cpgScannedLastUpdate ) >= m_cpgUpdateInterval )
    {
        SaveStateToHeader_();
    }
}

// Get the internal state from the database header
void DBMScanStateHeader::LoadStateFromHeader_()
{
    // Timestamps.
    m_ftCurrPassStartTime                       = ConvertLogTimeToFileTime( &( m_pfmp->Pdbfilehdr()->logtimeDbscanStart ) );
    m_ftPrevPassCompletionTime                  = ConvertLogTimeToFileTime( &( m_pfmp->Pdbfilehdr()->logtimeDbscanPrev ) );

    // Page numbers.
    m_pgnoContinuousHighWatermark               = m_pfmp->Pdbfilehdr()->le_pgnoDbscanHighestContinuous;
    m_pgnoHighestChecked                        = m_pfmp->Pdbfilehdr()->le_pgnoDbscanHighest;
    if ( FMP::FAllocatedFmp( m_pfmp ) && m_pfmp->FInUse() )
    {
        const PGNO pgnoHighestMax = m_pfmp->PgnoLast() - 1;  // shrink might have lowered the threshold.
        m_pgnoContinuousHighWatermark = UlFunctionalMin( m_pgnoContinuousHighWatermark, pgnoHighestMax );
        m_pgnoHighestChecked = UlFunctionalMin( m_pgnoHighestChecked, pgnoHighestMax );
    }

    // Page counts.
    m_cpgScannedLastUpdate                      = m_pgnoHighestChecked;
    m_cpgScannedCurrPass                        = m_pgnoHighestChecked;
}

// Save the internal state to the database header
void DBMScanStateHeader::SaveStateToHeader_()
{
    PdbfilehdrReadWrite pdbfilehdr = m_pfmp->PdbfilehdrUpdateable();
    ConvertFileTimeToLogTime( m_ftCurrPassStartTime, &( pdbfilehdr->logtimeDbscanStart ) );
    ConvertFileTimeToLogTime( m_ftPrevPassCompletionTime, &( pdbfilehdr->logtimeDbscanPrev ) );
    pdbfilehdr->le_pgnoDbscanHighestContinuous = m_pgnoContinuousHighWatermark;
    pdbfilehdr->le_pgnoDbscanHighest = m_pgnoHighestChecked;
    m_cpgScannedLastUpdate = m_pgnoHighestChecked;
}

// This is called when we need to reset the stats, but aren't currently scanning, for
// example, if scanning is turned off but we replay the sentinel, we want to reset the counters
// as if we ended that scan (as we effectively did), so if we pick it up at some point, we have
// correct data saved about what time this iteration started. The two fields that matter for this
// context are the PgnoHighestChecked and the CurrPassStartTime
void DBMScanStateHeader::ResetScanStats( FMP * const pfmp )
{
    PdbfilehdrReadWrite pdbfilehdr = pfmp->PdbfilehdrUpdateable();

    // We intentionally do *not* reset the lg_pgnoDbscanHighestContinuous here as it tracks
    // over multiple passes until we are able to complete a scan of every page in the db
    pdbfilehdr->le_pgnoDbscanHighest = 0;
    ConvertFileTimeToLogTime( UtilGetCurrentFileTime(), &( pdbfilehdr->logtimeDbscanStart ) );
}


//  ================================================================
//  DBMScanStateMSysDatabaseScan
//  ================================================================

DBMScanStateMSysDatabaseScan::DBMScanStateMSysDatabaseScan( const IFMP ifmp, IDataStore * const pstore ) :
    IDBMScanState(),
    m_ifmp( ifmp ),
    m_pstore( unique_ptr<IDataStore>( pstore ) ),
    m_cpgScannedLastUpdate( 0 )
{
    m_histoCbFreeLVPages = CReloadableAveStats();
    m_histoCbFreeRecPages = CReloadableAveStats();
    LoadStateFromTable();
}

DBMScanStateMSysDatabaseScan::~DBMScanStateMSysDatabaseScan()
{
}

void DBMScanStateMSysDatabaseScan::StartedPass()
{
    m_record.p_cInvocationsPass = 1;
    m_record.p_cInvocationsTotal++;
    m_record.p_ftPassStartTime  = UtilGetCurrentFileTime();
    m_record.p_cpgBadChecksumsPass = 0;
    m_record.p_cpgPassPagesRead = 0;

    m_histoCbFreeLVPages.Zero();
    m_histoCbFreeRecPages.Zero();
}

void DBMScanStateMSysDatabaseScan::ResumedPass()
{
    m_record.p_cInvocationsPass++;
    m_record.p_cInvocationsTotal++;
}
    
void DBMScanStateMSysDatabaseScan::FinishedPass()
{
    Expected( m_ifmp == ifmpNil || !PinstFromIfmp( m_ifmp )->FRecovering() );
    if ( m_ifmp != ifmpNil /* means we're unit testing */ &&
         !PinstFromIfmp( m_ifmp )->FRecovering() &&
         ( BoolParam( PinstFromIfmp( m_ifmp ), JET_paramEnableExternalAutoHealing ) ) )
    {
        //  We log a sentinel PGNO which gets picked up by the other side to know that the checksumming of
        //  the DB pass has finished.
        (void)ErrDBMEmitEndScan( m_ifmp );
    }

    m_record.p_ftPrevPassStartTime = m_record.p_ftPassStartTime;
    m_record.p_ftPrevPassEndTime = UtilGetCurrentFileTime();
    m_record.p_cpgPrevPassPagesRead = m_record.p_cpgPassPagesRead;
    
    m_record.p_cInvocationsPass = 0;
    m_record.p_ftPassStartTime  = 0;
    m_record.p_cpgBadChecksumsPass = 0;
    m_record.p_cpgPassPagesRead = 0;

    SaveStateToTable();
}

void DBMScanStateMSysDatabaseScan::SuspendedPass()
{
    SaveStateToTable();
}

void DBMScanStateMSysDatabaseScan::ReadPages( const PGNO pgnoStart, const CPG cpgRead )
{
    m_record.p_cpgPassPagesRead += cpgRead;

    if( ( CpgScannedCurrPass() - m_cpgScannedLastUpdate ) > m_cpgUpdateInterval )
    {
        SaveStateToTable();
    }
}

void DBMScanStateMSysDatabaseScan::BadChecksum( const PGNO pgnoBadChecksum )
{
    m_record.p_cpgBadChecksumsPass++;
    m_record.p_cpgBadChecksumsTotal++;
}

void DBMScanStateMSysDatabaseScan::LoadStateFromTable()
{
    ( void )m_record.Serializer().ErrLoadBindings( m_pstore.get() );

    if ( ( m_ifmp <= g_ifmpMax ) && FMP::FAllocatedFmp( m_ifmp ) && g_rgfmp[m_ifmp].FInUse() )
    {
        // Shrink might have lowered the threshold.
        m_record.p_cpgPassPagesRead = UlFunctionalMin( m_record.p_cpgPassPagesRead, g_rgfmp[m_ifmp].PgnoLast() - 1 );
    }

    m_cpgScannedLastUpdate = CpgScannedCurrPass();

    m_histoCbFreeLVPages.Init( m_record.p_cpgLVPages, m_record.p_cbFreeLVPages );
    m_histoCbFreeRecPages.Init( m_record.p_cpgRecPages, m_record.p_cbFreeRecPages );
}

void DBMScanStateMSysDatabaseScan::SaveStateToTable()
{
    m_record.p_ftLastUpdateTime = UtilGetCurrentFileTime();
    m_record.p_cpgLVPages = ( CPG )m_histoCbFreeLVPages.C();
    m_record.p_cbFreeLVPages = m_histoCbFreeLVPages.Total();
    m_record.p_cpgRecPages = ( CPG )m_histoCbFreeRecPages.C();
    m_record.p_cbFreeRecPages = m_histoCbFreeRecPages.Total();

    ( void )m_record.Serializer().ErrSaveBindings( m_pstore.get() );
    m_cpgScannedLastUpdate = CpgScannedCurrPass();
}

// Return stats about the current pass. Eventually, we could move all stats into this struct
// and simplify the interface
DBMScanStats DBMScanStateMSysDatabaseScan::DbmScanStatsCurrPass() const
{
    DBMScanStats dbmScanStats = DBMScanStats();
    dbmScanStats.m_cbAveFreeSpaceLVPages = m_histoCbFreeLVPages.Ave();
    dbmScanStats.m_cbAveFreeSpaceRecPages = m_histoCbFreeRecPages.Ave();
    dbmScanStats.m_cpgLVPagesScanned = ( CPG )m_histoCbFreeLVPages.C();
    dbmScanStats.m_cpgRecPagesScanned = ( CPG )m_histoCbFreeRecPages.C();

    return dbmScanStats;
}

// Calculate any page-level stats that we care about. Current stats being tracked:
//    - Average free space per page (tracked for LV pages and Record pages)
void DBMScanStateMSysDatabaseScan::CalculatePageLevelStats( const CPAGE& cpage )
{
    // Check the free space on this page, keep track of how much page "fragmentation" we've seen
    // Track LV and non-LV leaf pages separately
    if ( cpage.FLeafPage() &&           // Only look at leaf pages
         !cpage.FSpaceTree() &&         // Space tree pages aren't about data layout efficacy
         !cpage.FEmptyPage() &&         // Empty pages are outside the tree and available for other use
         !cpage.FRootPage() &&          // Root pages are required, so free space there can't be reduced
         cpage.PgnoNext() != pgnoNull ) // Last page is the typical append point and often only partially full
    {
        Assert( !cpage.FPreInitPage() );
        if ( cpage.FLongValuePage() )

        {
            m_histoCbFreeLVPages.ErrAddSample( cpage.CbPageFree() );
        }
        else
        {
            m_histoCbFreeRecPages.ErrAddSample( cpage.CbPageFree() );
        }
    }
}

//  ================================================================
//  MSysDatabaseScanRecord
//  ================================================================

MSysDatabaseScanRecord::MSysDatabaseScanRecord() :
    m_bindingFtLastUpdateTime( &p_ftLastUpdateTime, "LastUpdateTime" ),
    m_bindingFtPassStartTime( &p_ftPassStartTime, "PassStartTime" ),
    m_bindingCpgPassPagesRead( &p_cpgPassPagesRead, "PassPagesRead" ),
    m_bindingFtPrevPassStartTime( &p_ftPrevPassStartTime, "PrevPassStartTime" ),
    m_bindingFtPrevPassEndTime( &p_ftPrevPassEndTime, "PrevPassEndTime" ),
    m_bindingCpgPrevPassPagesRead( &p_cpgPrevPassPagesRead, "PrevPassPagesRead" ),
    m_bindingCsecPassElapsed( &p_csecPassElapsed, "ElapsedSeconds" ),
    m_bindingCInvocationsPass( &p_cInvocationsPass, "PassInvocations" ),
    m_bindingCpgBadChecksumsPass( &p_cpgBadChecksumsPass, "PassBadChecksum" ),
    m_bindingCInvocationsTotal( &p_cInvocationsTotal, "TotalInvocations" ),
    m_bindingCpgBadChecksumsTotal( &p_cpgBadChecksumsTotal, "TotalBadChecksums" ),
    m_bindingCpgLVPages( &p_cpgLVPages, "TotalLVPages" ),
    m_bindingCpgRecPages( &p_cpgRecPages, "TotalRecPages" ),
    m_bindingCbFreeLVPages( &p_cbFreeLVPages, "FreeBytesLVPages" ),
    m_bindingCbFreeRecPages( &p_cbFreeRecPages, "FreeBytesRecPages" ),
    m_serializer( m_bindings )
{
    m_bindings.AddBinding( &m_bindingFtLastUpdateTime );
    m_bindings.AddBinding( &m_bindingFtPassStartTime );
    m_bindings.AddBinding( &m_bindingCpgPassPagesRead );
    m_bindings.AddBinding( &m_bindingFtPrevPassStartTime );
    m_bindings.AddBinding( &m_bindingFtPrevPassEndTime );
    m_bindings.AddBinding( &m_bindingCpgPrevPassPagesRead );
    m_bindings.AddBinding( &m_bindingCsecPassElapsed );
    m_bindings.AddBinding( &m_bindingCInvocationsPass );
    m_bindings.AddBinding( &m_bindingCpgBadChecksumsPass );
    m_bindings.AddBinding( &m_bindingCInvocationsTotal );
    m_bindings.AddBinding( &m_bindingCpgBadChecksumsTotal );
    m_bindings.AddBinding( &m_bindingCpgLVPages );
    m_bindings.AddBinding( &m_bindingCpgRecPages );
    m_bindings.AddBinding( &m_bindingCbFreeLVPages );
    m_bindings.AddBinding( &m_bindingCbFreeRecPages );
    m_serializer.SetBindingsToDefault();
}

MSysDatabaseScanRecord::~MSysDatabaseScanRecord()
{
}

//  ================================================================
//  DBMScanConfig
//  ================================================================

DBMScanConfig::DBMScanConfig( INST * const pinst, FMP * const pfmp ) :
    m_pinst( pinst ),
    m_pfmp( pfmp )
{
}

INT DBMScanConfig::CSecMinScanTime()
{
    return ( INT )UlParam( m_pinst, JET_paramDbScanIntervalMinSec );
}

CPG DBMScanConfig::CpgBatch()
{
    size_t  cbScanBuffer    = ( size_t )UlParam( JET_paramMaxCoalesceReadSize );
            cbScanBuffer    -= cbScanBuffer % ( size_t )g_cbPage;
            cbScanBuffer    = max( cbScanBuffer, 64 * 1024 );
            cbScanBuffer    = max( cbScanBuffer, ( size_t )g_cbPage );
    return cbScanBuffer / ( size_t )g_cbPage;
}

DWORD DBMScanConfig::DwThrottleSleep()
{
    return ( DWORD )UlParam( m_pinst, JET_paramDbScanThrottle );
}

bool DBMScanConfig::FSerializeScan()
{
    return BoolParam( m_pinst, JET_paramEnableDBScanSerialization ) != fFalse;
}

INST * DBMScanConfig::Pinst()
{
    return m_pinst;
}

ULONG_PTR DBMScanConfig::UlDiskId() const
{
    return m_pfmp->UlDiskId();
}

//  ================================================================
//  DBMSingleScanConfig
//  ================================================================

DBMSingleScanConfig::DBMSingleScanConfig( INST * const pinst, FMP * const pfmp, const INT csecMax, const DWORD dwThrottleSleep ) :
    DBMScanConfig( pinst, pfmp ),
    m_csecMax( csecMax ),
    m_dwThrottleSleep( dwThrottleSleep )
{
}


//  ================================================================
//  DBMScanObserverCallback
//  ================================================================

DBMScanObserverCallback::DBMScanObserverCallback(
    const JET_CALLBACK pfnCallback,
    const JET_SESID sesid,
    const JET_DBID dbid ) :
    m_pfnCallback( pfnCallback ),
    m_sesid( sesid ),
    m_dbid( dbid )
{
    Assert( m_pfnCallback );
}

DBMScanObserverCallback::~DBMScanObserverCallback()
{
}

void DBMScanObserverCallback::FinishedPass_( const IDBMScanState * const pstate )
{
    Assert( m_pfnCallback );
    Assert( pstate );

    const __int64 ftPass = pstate->FtPrevPassCompletionTime()- pstate->FtPrevPassStartTime();
    __int64 csecPass = UtilConvertFileTimeToSeconds( ftPass );

    CPG cpgPass = pstate->CpgScannedPrevPass();
    
    ( *m_pfnCallback )( m_sesid, m_dbid, JET_tableidNil, JET_cbtypScanCompleted, &csecPass, &cpgPass, NULL, 0 );
}

//  ================================================================
//  DBMScanObserverFileCheck
//  ================================================================

DBMScanObserverFileCheck::DBMScanObserverFileCheck( const IFMP ifmp ) :
    m_ifmp( ifmp )
{
    FMP::AssertVALIDIFMP( m_ifmp );
}

void DBMScanObserverFileCheck::StartedPass_( const IDBMScanState * const )
{
    ERR err;
    QWORD cbAttributeListSize;

    Call( g_rgfmp[m_ifmp].Pfapi()->ErrNTFSAttributeListSize( &cbAttributeListSize ) );

    // NTFS attribute list sizes are limited by 256K; which is why we need to event if are
    // approaching the limit.
    Expected( cbAttributeListSize <= 256 * 1024 );

    const QWORD cbAttributeListSizeThreshold = UlConfigOverrideInjection( 38735, 200 * 1024 );

    if ( cbAttributeListSize >= cbAttributeListSizeThreshold )
    {

        // The NTFS File Attributes size for database '%1%' is %2% bytes which exceeds the threshold of %3% bytes. Be afraid.

        WCHAR wszSize[10];
        OSStrCbFormatW( wszSize, sizeof( wszSize ), L"%I64d", cbAttributeListSize );

        WCHAR wszLimit[10];
        OSStrCbFormatW( wszLimit, sizeof( wszLimit ), L"%I64d", cbAttributeListSizeThreshold );
        
        const WCHAR* rgwsz[] = {g_rgfmp[m_ifmp].WszDatabaseName(), wszSize, wszLimit};

        UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            NTFS_FILE_ATTRIBUTE_MAX_SIZE_EXCEEDED,
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            PinstFromIfmp( m_ifmp ) );
        
        OSUHAEmitFailureTag( PinstFromIfmp( m_ifmp ), HaDbFailureTagAlertOnly, L"56b7d404-da5c-4065-8104-9a1449cd1ce5" );
    }

HandleError:
    // Failures are ignored because the attribute list may not exist for valid reasons.
    return;
}

//  ================================================================
//  DBMScanObserverEvents
//  ================================================================

DBMScanObserverEvents::DBMScanObserverEvents( FMP * const pfmp, const INT csecMaxPassTime ) :
    m_categoryId( ONLINE_DEFRAG_CATEGORY ),
    m_pfmp( pfmp ),
    m_csecMaxPassTime( csecMaxPassTime ),
    m_pgnoStart( 0 ),
    m_pgnoLast( 0 ),
    m_cpgBadChecksums( 0 )
{
}

DBMScanObserverEvents::~DBMScanObserverEvents()
{
}

void DBMScanObserverEvents::SuspendedPass_( const IDBMScanState * const )
{
}

void DBMScanObserverEvents::ReadPage_( const IDBMScanState * const, const PGNO pgno, DBMObjectCache* const pdbmObjectCache )
{
    Assert( pgno != 0 );
    if ( m_pgnoStart == 0 )
    {
        m_pgnoStart = pgno;
    }
    
    m_pgnoLast = pgno;
}

void DBMScanObserverEvents::BadChecksum_( const IDBMScanState * const, const PGNO pgno )
{
    m_cpgBadChecksums++;
}

void DBMScanObserverEvents::StartedPass_( const IDBMScanState * const pstate )
{
    // %1 (%2) %3 Database Maintenance is starting for database '%4'.
    const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), };
    UtilReportEvent(
        eventInformation,
        m_categoryId,
        DB_MAINTENANCE_START_PASS_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        Pinst_() );
}

void DBMScanObserverEvents::ResumedPass_( const IDBMScanState * const pstate )
{
    // %1 (%2) %3 Database Maintenance is resuming for database '%4',
    // starting from page %5. This pass started on %6 and has been
    // running for %7 days.
    
    const PGNO pgnoResume = pstate->CpgScannedCurrPass();

    WCHAR wszPgno[ 32 ];
    OSStrCbFormatW( wszPgno, sizeof( wszPgno ), L"%d", pgnoResume );
    
    WCHAR wszStartTime[ 64 ];
    size_t cwchRequired = 0;
    CallS( ErrUtilFormatFileTimeAsDate(
        pstate->FtCurrPassStartTime(),
        wszStartTime,
        _countof( wszStartTime ),
        &cwchRequired ) );

    const _int64 cdayPass = UtilConvertFileTimeToDays(
        pstate->FtPrevPassCompletionTime() - pstate->FtCurrPassStartTime() );
    WCHAR wszcdayPass[ 32 ];
    OSStrCbFormatW( wszcdayPass, sizeof( wszcdayPass ), L"%I64d", cdayPass );

    const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszPgno, wszStartTime, wszcdayPass, };
    UtilReportEvent(
        eventInformation,
        m_categoryId,
        DB_MAINTENANCE_RESUME_PASS_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        Pinst_(),
        JET_EventLoggingLevelMedium );
}

bool DBMScanObserverEvents::FPassTookTooLong_( const IDBMScanState * const pstate ) const
{
    const __int64 ftPass = pstate->FtPrevPassCompletionTime() - pstate->FtPrevPassStartTime();
    Assert( ftPass >= 0 );
    const __int64 csecPass = UtilConvertFileTimeToSeconds( ftPass );
    Assert( csecPass >= 0 );
    return ( csecPass > m_csecMaxPassTime );
}

const INT cchScanStats = 100;
void FormatScanStats( WCHAR * const wszScanStats, const INT cbScanStats, const PGNO pgnoStart, const PGNO pgnoLast, const CPG cpgBadChecksums )
{
    if ( pgnoStart == 0 )
    {
        OSStrCbFormatW( wszScanStats, cbScanStats, L"-, -" );
    }
    else
    {
        OSStrCbFormatW( wszScanStats, cbScanStats, L"%u, %u", pgnoLast - pgnoStart, cpgBadChecksums );
    }
}

void DBMScanObserverEvents::FinishedPass_( const IDBMScanState * const pstate )
{
    DBMScanStateHeader  statePassiveHdrInfo( Pfmp_() );

    //  Before logging the finish event (inaccurately), we check that this last pass we just did
    //  _began_ AFTER the completion of the last passive pass in the header.  If it didn't, it
    //  suggests that this ESE instance spent time as active, failed off, became passive for a
    //  while (completing one or more full checksum passes) and then became active, and as such
    //  completed a super long pass. It's better in this case to not claim we have actually
    //  completed a pass, so we'll squash it.  This happens BTW b/c we keep separate table and
    //  header state for active and passive, so an active doesn't pick up where a passive left
    //  off.

    if ( pstate->FtPrevPassStartTime() >= statePassiveHdrInfo.FtPrevPassCompletionTime() )
    {
        WCHAR wszStartTime[ 64 ];
        DBMScanFormatOpticalTime_( pstate->FtPrevPassStartTime(), wszStartTime, _countof( wszStartTime ) );
        
        const __int64 ftPass = pstate->FtPrevPassCompletionTime() - pstate->FtPrevPassStartTime();

        WCHAR wszcsecPass[ 32 ];
        OSStrCbFormatW( wszcsecPass, sizeof( wszcsecPass ), L"%I64d", UtilConvertFileTimeToSeconds( ftPass ) );

        WCHAR wszcpgScanned[ 16 ];
        OSStrCbFormatW( wszcpgScanned, sizeof( wszcpgScanned ), L"%d", pstate->CpgScannedPrevPass() ) ;

        WCHAR wszScanStats[ cchScanStats ];
        FormatScanStats( wszScanStats, sizeof( wszScanStats ), m_pgnoStart, m_pgnoLast, m_cpgBadChecksums );

        // Get the space tracking stats
        DBMScanStats dbmScanStats = pstate->DbmScanStatsCurrPass();

        WCHAR wszCbFreeRec[ 32 ];
        OSStrCbFormatW( wszCbFreeRec, sizeof( wszCbFreeRec ), L"%I64u", dbmScanStats.m_cbAveFreeSpaceRecPages );

        WCHAR wszCpgScannedRecPages[16];
        OSStrCbFormatW( wszCpgScannedRecPages, sizeof( wszcpgScanned ), L"%d", dbmScanStats.m_cpgRecPagesScanned );

        WCHAR wszCbFreeLV[32];
        OSStrCbFormatW( wszCbFreeLV, sizeof( wszCbFreeLV ), L"%I64u", dbmScanStats.m_cbAveFreeSpaceLVPages );

        WCHAR wszCpgScannedLVPages[16];
        OSStrCbFormatW( wszCpgScannedLVPages, sizeof( wszcpgScanned ), L"%d", dbmScanStats.m_cpgLVPagesScanned );

        if ( FPassTookTooLong_( pstate ) )
        {
            
            WCHAR wszcdayPass[ 32 ];
            OSStrCbFormatW( wszcdayPass, sizeof( wszcdayPass ), L"%I64d", UtilConvertFileTimeToDays( ftPass ) );
            
            const INT csecIn1Day = 60 * 60 * 24;
            WCHAR wszcdayMaxPassTime[ 32 ];
            OSStrCbFormatW( wszcdayMaxPassTime, sizeof( wszcdayMaxPassTime ), L"%d", m_csecMaxPassTime / csecIn1Day );

            const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszStartTime, wszcsecPass, wszcdayPass, wszcdayMaxPassTime, wszcpgScanned, wszScanStats, wszCbFreeRec, wszCpgScannedRecPages, wszCbFreeLV, wszCpgScannedLVPages };
            UtilReportEvent(
                eventWarning,
                m_categoryId,
                DB_MAINTENANCE_LATE_PASS_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                Pinst_() );
        }
        else
        {
            const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszStartTime, wszcsecPass, wszcpgScanned, wszScanStats, wszCbFreeRec, wszCpgScannedRecPages, wszCbFreeLV, wszCpgScannedLVPages };
            UtilReportEvent(
                eventInformation,
                m_categoryId,
                DB_MAINTENANCE_COMPLETE_PASS_ID,
                _countof( rgwsz ),
                rgwsz,
                0,
                NULL,
                Pinst_() );
        }
    }
}

void DBMScanObserverEvents::NotifyStats_( const IDBMScanState * const pstate )
{
    // %1 (%2) %3 Database Maintenance is running on database '%4'.
    // This pass started on %5 and has been running for %6 hours.
    // %7 pages seen
    
    WCHAR wszcpgScanned[ 16 ];
    OSStrCbFormatW( wszcpgScanned, sizeof( wszcpgScanned ), L"%d", pstate->CpgScannedCurrPass() );
    
    WCHAR wszStartTime[ 64 ];
    size_t cwchRequired = 0;
    CallS( ErrUtilFormatFileTimeAsDate(
           pstate->FtCurrPassStartTime(),
           wszStartTime,
           _countof( wszStartTime ),
           &cwchRequired ) );

    const _int64 chourPass = UtilConvertFileTimeToSeconds( UtilGetCurrentFileTime() - pstate->FtCurrPassStartTime() ) / 3600;
    WCHAR wszchourPass[ 32 ];
    OSStrCbFormatW( wszchourPass, sizeof( wszchourPass ), L"%I64d", chourPass );

    const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszStartTime, wszchourPass, wszcpgScanned };
    UtilReportEvent(
        eventInformation,
        m_categoryId,
        DB_MAINTENANCE_NOTIFY_RUNNING_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        Pinst_(),
        JET_EventLoggingLevelMedium );
}


//  ================================================================
//  DBMScanObserverRecoveryEvents
//  ================================================================

DBMScanObserverRecoveryEvents::DBMScanObserverRecoveryEvents( FMP * const pfmp, const INT csecMaxPassTime ) :
    m_categoryId( ONLINE_DEFRAG_CATEGORY ),
    m_pfmp( pfmp ),
    m_csecMaxPassTime( csecMaxPassTime ),
    m_pgnoStart( 0 ),
    m_pgnoLast( 0 ),
    m_cpgBadChecksums( 0 )
{
}

DBMScanObserverRecoveryEvents::~DBMScanObserverRecoveryEvents()
{
}

void DBMScanObserverRecoveryEvents::BadChecksum_( const IDBMScanState * const, const PGNO pgno )
{
    m_cpgBadChecksums++;
}

void DBMScanObserverRecoveryEvents::StartedPass_( const IDBMScanState * const pstate )
{
    // %1 (%2) %3Online Maintenance is starting Database Checksumming background task for database '%4'.
    const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), };
    UtilReportEvent(
        eventInformation,
        m_categoryId,
        SCAN_CHECKSUM_START_PASS_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        Pinst_() );
    SetPassDeadline_( pstate );
}

void DBMScanObserverRecoveryEvents::ResumedPass_( const IDBMScanState * const pstate )
{
    // %1 (%2) %3Online Maintenance is resuming Database Checksumming background task for database '%4'. 
    // This pass started on %5 and has been running for %6 days.
    WCHAR wszStartTime[ 64 ];
    size_t cwchRequired = 0;
    if( pstate->FtCurrPassStartTime() )
    {
        CallS( ErrUtilFormatFileTimeAsDate(
            pstate->FtCurrPassStartTime(),
            wszStartTime,
            _countof( wszStartTime ),
            &cwchRequired ) );
    }
    else
    {
        //  Because we follow the active, the very first time we mount as a passive we might 
        //  get DBM ScanCheck LRs from the middle of a scan, that would not have started a pass
        //  normally in such a way as to set pstate->FtCurrPassStartTime() / in the DB header
        //  previously.
        wszStartTime[0] = L'-';
        wszStartTime[0] = L'\0';
    }

    const _int64 cdayPass = UtilConvertFileTimeToDays(
        pstate->FtPrevPassCompletionTime() - pstate->FtCurrPassStartTime() );
    WCHAR wszcdayPass[ 32 ];
    OSStrCbFormatW( wszcdayPass, sizeof( wszcdayPass ), L"%I64d", cdayPass );

    const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszStartTime, wszcdayPass };
    UtilReportEvent(
        eventInformation,
        m_categoryId,
        SCAN_CHECKSUM_RESUME_PASS_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        Pinst_(),
        JET_EventLoggingLevelMedium );
    SetPassDeadline_( pstate );
}

void DBMScanObserverRecoveryEvents::SuspendedPass_( const IDBMScanState * const pstate )
{
    if ( Pfmp_()->Pinst() == NULL /* the unit test uses a mock FMP, that doesn't have a real inst */ ||
        UlParam( Pfmp_()->Pinst(), JET_paramEnableDBScanInRecovery ) & bitDBScanInRecoveryFollowActive )
    {
        //  In the FollowActive mode a suspended DBScan results in an actually incomplete DBScan so we want to 
        //  log about this condition.
        WCHAR wszPgnoHighest[32];
        //  I am conflicted on whether we should use CpgScannedCurrPass() or the highest pgno we checked
        //  from the regular pass.
        OSStrCbFormatW( wszPgnoHighest, sizeof( wszPgnoHighest ), L"%d", pstate->PgnoContinuousHighWatermark() );
        WCHAR wszLastFullCompletionTime[100];
        DBMScanFormatOpticalTime_( pstate->FtPrevPassCompletionTime(), wszLastFullCompletionTime, _countof( wszLastFullCompletionTime ) );
        WCHAR wszScanStats[cchScanStats];
        FormatScanStats( wszScanStats, sizeof( wszScanStats ), m_pgnoStart, m_pgnoLast, m_cpgBadChecksums );

        const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszPgnoHighest, wszLastFullCompletionTime, wszScanStats };
        UtilReportEvent(
            eventInformation,
            m_categoryId,
            SCAN_CHECKSUM_CRITICAL_ABORT_ID,
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            Pinst_() );
        
    }
}

void DBMScanObserverRecoveryEvents::FinishedPass_( const IDBMScanState * const pstate )
{
    // %1 (%2) %3 Online Maintenance Database Checksumming background task has completed for database '%4'.
    // This pass started on %5 and ran for a total of %6 seconds (over %7 days) on %8 pages.

    //  NOTE: There WAS a fundamental weirdness in our event time b/c the Active keeps state
    //  in a table, and the passive keeps state in a header ... so a passive could do say 25%
    //  of a pass, get switched to being an active, complete 15 passes over 1 month, then go
    //  back to passive and register a 1 MONTH long dbscan pass.  The opposite issue event is
    //  suppressed in DBMScanObserverEvents::FinishedPass_.  The reason it is no longer an
    //  issue with the passive DBScan pass here, is b/c now that the scan follows the active
    //  our passive start time will be caused by the latest active start time.  BTW, in the
    //  old / non-"follow" mode, it is still a problem ... one that's hard to solve as the
    //  table is not available during replay.

    WCHAR wszStartTime[ 64 ];
    DBMScanFormatOpticalTime_( pstate->FtPrevPassStartTime(), wszStartTime, _countof( wszStartTime ) );

    const _int64 ftPass = pstate->FtPrevPassCompletionTime() - pstate->FtPrevPassStartTime();

    WCHAR wszcsecPass[ 32 ];
    OSStrCbFormatW( wszcsecPass, sizeof( wszcsecPass ), L"%I64d", UtilConvertFileTimeToSeconds( ftPass ) );

    WCHAR wszcdayPass[ 32 ];
    OSStrCbFormatW( wszcdayPass, sizeof( wszcdayPass ), L"%I64d", UtilConvertFileTimeToDays( ftPass ) );

    WCHAR wszcpgScanned[ 16 ];
    OSStrCbFormatW( wszcpgScanned, sizeof( wszcpgScanned ), L"%d", pstate->CpgScannedPrevPass() );

    WCHAR wszScanStats[cchScanStats];
    FormatScanStats( wszScanStats, sizeof( wszScanStats ), m_pgnoStart, m_pgnoLast, m_cpgBadChecksums );

    const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszStartTime, wszcsecPass, wszcdayPass, wszcpgScanned, wszScanStats };
    UtilReportEvent(
        eventInformation,
        m_categoryId,
        SCAN_CHECKSUM_ONTIME_PASS_ID,
        _countof( rgwsz ),
        rgwsz,
        0,
        NULL,
        Pinst_() );
}

void DBMScanObserverRecoveryEvents::ReadPage_( const IDBMScanState * const pstate, const PGNO pgno, DBMObjectCache* const pdbmObjectCache )
{
    if ( m_pgnoStart == 0 )
    {
        m_pgnoStart = pgno;
    }
    m_pgnoLast = pgno;

    if ( FPassIsOverdue_() )
    {
        ReportPassIsOverdue_( pstate );
        ExtendPassDeadline_( pstate );
    }
}

void DBMScanObserverRecoveryEvents::ReportPassIsOverdue_( const IDBMScanState * const pstate ) const
{
    // %1 (%2) %3 Online Maintenance Database Checksumming background task is not finishing on time for database '%4'.
    // This pass started on %5 and has been running for %6 seconds (over %7 days) so far.
    
    //  Because we follow the active, the very first time we mount as a passive we might 
    //  get DBM ScanCheck LRs from the middle of a scan, that would not have started a pass
    //  normally in such a way as to set pstate->FtCurrPassStartTime() / in the DB header
    //  previously. If this is the case, don't bother logging the event since it's not
    //  meaningful.
    if ( pstate->FtCurrPassStartTime() )
    {
        WCHAR wszStartTime[ 64 ];
        size_t cwchRequired = 0;
        CallS( ErrUtilFormatFileTimeAsDate(
            pstate->FtCurrPassStartTime(),
            wszStartTime,
            _countof( wszStartTime ),
            &cwchRequired ) );

        const _int64 ftPass = UtilGetCurrentFileTime() - pstate->FtCurrPassStartTime();

        WCHAR wszcsecPass[ 32 ];
        OSStrCbFormatW( wszcsecPass, sizeof( wszcsecPass ), L"%I64d", UtilConvertFileTimeToSeconds( ftPass ) );

        WCHAR wszcdayPass[ 32 ];
        OSStrCbFormatW( wszcdayPass, sizeof( wszcdayPass ), L"%I64d", UtilConvertFileTimeToDays( ftPass ) );

        const WCHAR* rgwsz[] = { Pfmp_()->WszDatabaseName(), wszStartTime, wszcsecPass, wszcdayPass, };

        UtilReportEvent(
            eventWarning,
            m_categoryId,
            SCAN_CHECKSUM_LATE_PASS_ID,
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            Pinst_() );
    }
}

bool DBMScanObserverRecoveryEvents::FPassIsOverdue_() const
{
    return UtilGetCurrentFileTime() > m_ftDeadline;
}

void DBMScanObserverRecoveryEvents::SetPassDeadline_( const IDBMScanState * const pstate )
{
    m_ftDeadline = pstate->FtCurrPassStartTime() + UtilConvertSecondsToFileTime( m_csecMaxPassTime );
}
    
void DBMScanObserverRecoveryEvents::ExtendPassDeadline_( const IDBMScanState * const pstate )
{
    // issue a 1 day extension  
    const INT csecIn1Day = 60 * 60 * 24;
    m_ftDeadline = UtilGetCurrentFileTime() + UtilConvertSecondsToFileTime( csecIn1Day );
}

//  ================================================================
//  DBMScanObserverPerfmon
//  ================================================================

DBMScanObserverPerfmon::DBMScanObserverPerfmon( INST * const pinst ) :
    m_pinst( pinst )
{
}
    
DBMScanObserverPerfmon::~DBMScanObserverPerfmon()
{
}

void DBMScanObserverPerfmon::StartedPass_( const IDBMScanState * const )
{
    if ( Pinst_() && 1 == Pinst_()->CAttachedUserDBs() )
    {
        m_ftTimeStart = UtilGetCurrentFileTime();
        Assert( m_ftTimeStart );
    }
}
void DBMScanObserverPerfmon::ResumedPass_( const IDBMScanState * const )
{
    if ( Pinst_() && 1 == Pinst_()->CAttachedUserDBs() )
    {
        m_ftTimeStart = UtilGetCurrentFileTime();
        Assert( m_ftTimeStart );
    }
}
void DBMScanObserverPerfmon::FinishedPass_( const IDBMScanState * const )
{
    if ( Pinst_() )
    {
        PERFOpt( perfctrDBMDuration.Set( Pinst_(), 0 ) );
    }
}
void DBMScanObserverPerfmon::SuspendedPass_( const IDBMScanState * const )
{
    if ( Pinst_() )
    {
        //  We'll use 1 for a special value to mean suspended ...
        PERFOpt( perfctrDBMDuration.Set( Pinst_(), 1 ) );
    }
}

void DBMScanObserverPerfmon::ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache )
{
    PERFOpt( perfctrDBMPagesRead.Add( Pinst_(), 1 ) );
    
    if ( Pinst_() && m_ftTimeStart )
    {
        PERFOptDeclare( const LONG csecTimeDuration = ( LONG )UtilConvertFileTimeToSeconds( UtilGetCurrentFileTime() - m_ftTimeStart ) );
        PERFOpt( perfctrDBMDuration.Set( Pinst_(), csecTimeDuration ) );
    }
}


//  ================================================================
//  DBMScanObserverCleanupFactory
//  ================================================================

ERR DBMScanObserverCleanupFactory::ErrCreateDBMScanObserverCleanup(
        PIB * const ppib,
        const IFMP ifmp,
        _Out_ DBMScanObserver ** pobserver )
{
    ERR err;

    Alloc( *pobserver = new DBMScanObserverCleanup( ppib, ifmp ) );
    
HandleError:
    return err;
}


//  ================================================================
//  DBMScanObserverCleanup
//  ================================================================

DBMScanObserverCleanup::DBMScanObserverCleanup(
        PIB * const ppib,
        const IFMP ifmp ) :
    m_ppib( ppib ),
    m_ifmp( ifmp ),
    m_objidMaxCommitted( objidNil ),
    m_fPrepareToTerm( false )
{
}

DBMScanObserverCleanup::~DBMScanObserverCleanup()
{
    // Session is owned by DBMScan which will end it.
    Assert( ppibNil != m_ppib );
    m_ppib = ppibNil;
}

void DBMScanObserverCleanup::PrepareToTerm_( const IDBMScanState * const )
{
    m_fPrepareToTerm = true;
}

void DBMScanObserverCleanup::FinishedPass_( const IDBMScanState * const )
{
}

void DBMScanObserverCleanup::SuspendedPass_( const IDBMScanState * const )
{
}

// Called when Database Maintenance has read and verified a page. The cleanup
// code will latch the page and then perform the cleanup.
void DBMScanObserverCleanup::ReadPage_( const IDBMScanState * const pstate, const PGNO pgno, DBMObjectCache* pdbmObjectCache )
{
    ERR err;
    CSR csr;

    PIBTraceContextScope tcRef = m_ppib->InitTraceContextScope();
    tcRef->nParentObjectClass = tceNone;
    tcRef->iorReason.SetIort( iortScrubbing );

    pdbmObjectCache->CloseCachedObjectsWithPendingDeletes();
    
    if ( objidNil == m_objidMaxCommitted )
    {
        Call( ErrGetHighestCommittedObjid_() );
    }

    Call( csr.ErrGetRIWPage( m_ppib, m_ifmp, pgno, BFLatchFlags( bflfNoTouch | bflfNoEventLogging | bflfDBScan ) ) );

    ObjidState ois = ois::Unknown;
    if ( !FIgnorablePage_( csr ) )
    {
        ois = OisGetObjectIdState_( csr, pdbmObjectCache );
    }

    switch ( ois )
    {
        case ois::Unknown:
            // Ignorable or unknown: nothing needs to be done with this page .
            break;

        case ois::Invalid:
            // Important: this call may release the page in case of failure.
            //
            // Now that we are processing table root pages as well for redeleting reverted FDPs, we will check once again before we try to clean up.
            if ( csr.Cpage().FLeafPage() )
            {
                Call( ErrCleanupUnusedPage_( &csr ) );
            }
            break;

        case ois::ToBeDeleted:
            // Currently possible to be set for a LV table which was deleted and later reverted by RBS.
            //
            // TODO: Should we get the table objid and attempt to delete here? For now, we will delete when we read the table root page.
            //       Can ErrFILEOpenLVRoot be improved to not touch the root page by using DirOpenNoTouch instead?
            break;

        case ois::Valid:
            // Save the page-level stats for this page
            const_cast<IDBMScanState *>( pstate )->CalculatePageLevelStats( csr.Cpage() );

            if ( csr.Cpage().FPageFDPRootDelete() )
            {
                // Need to delete the table which was reverted back by RBS partially
                // i.e., the table was deleted but then reverted back. Only root page and space tree pages are reverted though.
                // We will just initiate a best effort delete on the table.
                RedeleteRevertedFDP_( &csr, pdbmObjectCache );
            }
            else if ( csr.Cpage().Dbtime() >= g_rgfmp[m_ifmp].DbtimeOldestGuaranteed() )
            {
                // updates on this page are recent enough that we won't process it
                // runtime cleanup should deal with this page, otherwise we will
                // process it on the next pass.
            }
            else if ( csr.Cpage().FLongValuePage() )
            {
                // Important: this call may release the page in case of failure.
                Call( ErrCleanupLVPage_( &csr, pdbmObjectCache ) );
            }
            else if ( csr.Cpage().FPrimaryPage() )
            {
                Call( ErrCleanupPrimaryPage_( &csr, pdbmObjectCache ) );
            }
            else
            {
                // Important: this call may release the page in case of failure.
                Call( ErrCleanupIndexPage_( &csr ) );
            }
            break;

        default:
            AssertSz( fFalse, "UnknownObjidState" );
            break;
    }

HandleError:

    // If there are failures above, we may have released the page
    // in order to rollback, so it would not be latched. 
    if ( csr.FLatched() )
    {
        csr.ReleasePage();
        csr.Reset();
    }
}

// Get objidMax. To do this we wait to become the oldest transaction in the
// system, at which point all objids < objidMax have been committed to the
// catalog. Then we commit our transaction and read the catalog.
ERR DBMScanObserverCleanup::ErrGetHighestCommittedObjid_()
{
    ERR err;

    INST * const pinst = PinstFromPpib( m_ppib );
    
    Call( ErrDIRBeginTransaction( m_ppib, 56613, JET_bitTransactionReadOnly ) );

    m_objidMaxCommitted = g_rgfmp[m_ifmp].ObjidLast();

    INT cLoop = 0;
    while ( TrxOldest( pinst ) != m_ppib->trxBegin0 )
    {
        if ( pinst->m_fTermInProgress )
        {
            err = ErrERRCheck( JET_errTermInProgress ); // don't use Call() -- we need to commit the transaction
            break;
        }

        if ( pinst->FInstanceUnavailable() )
        {
            err = pinst->ErrInstanceUnavailableErrorCode(); // don't use Call() -- we need to commit the transaction
            break;
        }

        if ( m_fPrepareToTerm )
        {
            err = ErrERRCheck( JET_errTermInProgress ); // don't use Call() -- we need to commit the transaction
            break;
        }

        err = ErrFaultInjection( 50248 );
        
        if ( err < JET_errSuccess )
        {
            break;
        }
        
        UtilSleep( 100 );
        if ( ++cLoop > 10*120 ) // wait up to 2 minutes
        {
            err = ErrERRCheck( JET_errTooManyActiveUsers ); // don't use Call() -- we need to commit the transaction
            break;
        }
    }
    
    CallS( ErrDIRCommitTransaction( m_ppib, NO_GRBIT ) );
    Call( err );
    
HandleError:

    if ( err < JET_errSuccess )
    {
        m_objidMaxCommitted = objidNil;
    }
    
    return err;
}

// Determine if cleanup on this page can be safely skipped
bool DBMScanObserverCleanup::FIgnorablePage_( const CSR& csr ) const
{
    const OBJID objid = csr.Cpage().ObjidFDP();

    if ( !csr.Cpage().FLeafPage() && ( !csr.Cpage().FPageFDPRootDelete() || !csr.Cpage().FPrimaryPage() ) )
    {
        // non-leaf pages aren't cleanable, unless it is a table root page marked to be deleted in which case dbscan will try to clean up the table.
        return true;
    }
    else if( csr.Cpage().FSpaceTree() )
    {
        // do not scrub space tree pages
        return true;
    }
    else if ( objidSystemRoot == objid )
    {
        // the system root doesn't need to be scrubbed
        return true;
    }
    else if ( csr.Cpage().FEmptyPage() )
    {
        // empty pages are scrubbed when they are freed
        return true;
    }
    else if ( csr.Cpage().FPreInitPage() )
    {
        // pre-init pages do not hold and have never held any data
        return true;
    }
    else if ( FCATMsysObjectsTableIndex( objid ) )
    {
        // We can open the catalog/shadow catalog with some special cases, but the
        // indices are much harder to open without deadlocking.
        return true;
    }
    else if ( objid > m_objidMaxCommitted )
    {
        // Without the catalog info we can't determine the pgnoFDP, which means
        // version info can't be looked up, so we can't process this page
        // CONSIDER: recalculate the max committed objid if we have been running
        // for a long time
        return true;
    }
    else if ( csr.Cpage().FPageFDPRootDelete() && ( !csr.Cpage().FPrimaryPage() || csr.Cpage().FLongValuePage() ) )
    {
        // For now, we will only allow reverted deleted table cleanup from table's root pages
        return true;
    }

    return false;
}

// See if the objid has been deleted. This method also caches the FUCB for the table.
ObjidState DBMScanObserverCleanup::OisGetObjectIdState_( CSR& csr, DBMObjectCache* const pdbmObjectCache )
{
    ERR err = JET_errSuccess;
    ObjidState ois = ois::Unknown;

    const OBJID objid = csr.Cpage().ObjidFDP();
    Assert( objid > objidNil ); // otherwise I think this is an empty page ...

    const bool fCacheCheck = (bool)UlConfigOverrideInjection( 51678, false );
    const ObjidState oisCached = pdbmObjectCache->OisGetObjidState( objid );

    // If cached object states indicates there is no associated FUCB, we will try to initialize one.
    if ( oisCached != ois::Unknown && oisCached != ois::ValidNoFUCB )
    {
        if ( !fCacheCheck )
        {
            ois = oisCached;
            goto HandleError;
        }
    }

    if ( csr.Cpage().FPrimaryPage() && !csr.Cpage().FLongValuePage() )
    {
        FUCB * pfucbTable = pfucbNil;
        BOOL fExists = fTrue;
        err = ErrOpenTableGetFucb_( objid, &fExists, &pfucbTable );
        if ( ( err >= JET_errSuccess ) && !fExists )
        {
            Assert( pfucbTable == pfucbNil );
            
            // The table doesn't exist, the objid is less than the committed max, and the page
            // has data on it. Clean the page
            Assert( objid <= m_objidMaxCommitted );
            Assert( csr.Cpage().FLeafPage() || csr.Cpage().FPageFDPRootDelete() );
            if ( oisCached == ois::Unknown || oisCached == ois::ValidNoFUCB )
            {
                pdbmObjectCache->MarkCacheObjectInvalid( objid );
            }
            ois = ois::Invalid;
            err = JET_errSuccess;
        }
        else if ( err >= JET_errSuccess )
        {
            Assert( fExists );
            Assert( pfucbTable != pfucbNil );
            if ( oisCached == ois::Unknown || oisCached == ois::ValidNoFUCB )
            {
                pdbmObjectCache->CacheObjectFucb( pfucbTable, objid );
            }
            else
            {
                 CallS( ErrFILECloseTable( m_ppib, pfucbTable ) );   
                 pfucbTable = pfucbNil;
            }
            ois = ois::Valid;
        }
        Call( err );
    }
    else
    {
        // This code can't be used for all cases -- if we are scrubbing the MSObjids table then this
        // can deadlock. We avoid this by using the above code for primary index pages, but we could
        // save the objid of MSysObjids in the FMP at database attach time.
        // This code can also deadlock trying to look up / open the MSysObjids table from the 
        // catalog, but we protect this by ignoring such pages in FIgnorablePage_()
        OBJID objidTable = objidNil;
        SYSOBJ sysobj = sysobjNil;
        Call( ErrCATGetObjidMetadata( m_ppib, m_ifmp, objid, &objidTable, &sysobj ) );
        if ( sysobj != sysobjNil )
        {
            Assert( objidTable != objidNil );
            Assert( objidTable != objid );
            Assert( ( sysobj == sysobjLongValue ) || ( sysobj == sysobjIndex ) );

            if ( oisCached == ois::Unknown || oisCached == ois::ValidNoFUCB )
            {
                FUCB * pfucbTable = pfucbNil;

                // Get table fucb.
                if ( pfucbNil == ( pfucbTable = pdbmObjectCache->PfucbGetCachedObject( objidTable ) ) )
                {
                    BOOL fExists = fTrue;
                    Call( ErrOpenTableGetFucb_( objidTable, &fExists, &pfucbTable ) );
                    if ( !fExists )
                    {
                        goto HandleError;
                    }

                    Assert( pfucbTable != pfucbNil );
                    Assert( objidTable == pfucbTable->u.pfcb->ObjidFDP() );
                    pdbmObjectCache->CacheObjectFucb( pfucbTable, objidTable );
                }

                if ( sysobj == sysobjLongValue )
                {
                    FUCB * pfucbLV = pfucbNil;

                    // Release the latch on the page as ErrGetFUCBForLV needs to m_critLV.Enter() with 
                    // rankLVCreate 7000
                    const DBTIME dbtime = csr.Dbtime();
                    const PGNO pgno = csr.Pgno();
                    csr.ReleasePage();
                    csr.Reset();

                    Call( ErrFILEOpenLVRoot( pfucbTable, &pfucbLV, fFalse ) );
                    Assert( pfucbLV != pfucbNil );

                    pdbmObjectCache->CacheObjectFucb( pfucbLV, objid );

                    Call( csr.ErrGetRIWPage( m_ppib, m_ifmp, pgno, BFLatchFlags( bflfNoTouch | bflfNoEventLogging | bflfDBScan ) ) );
                    const DBTIME dbtimeRelatch = csr.Dbtime();
                    if ( dbtime != dbtimeRelatch )
                    {
                        Error( ErrERRCheck( JET_errDatabaseInUse ) );
                    }
                }
                else if ( sysobj == sysobjIndex )
                {
                    // We do not cache FUCBs for secondary indices.
                    // Please, read detailed comment above DBMObjectCache.
                }
                else
                {
                    AssertSz( fFalse, "UnknownSysObj" );
                    Error( ErrERRCheck( JET_errInternalError ) );
                }
            }
            ois = ois::Valid;
        }
        else
        {
            if ( oisCached == ois::Unknown || oisCached == ois::ValidNoFUCB )
            {
                pdbmObjectCache->MarkCacheObjectInvalid( objid );
            }
            ois = ois::Invalid;
        }
    }

    err = JET_errSuccess;

HandleError:
    Assert( err <= JET_errSuccess );
    if ( err == JET_errSuccess )
    {
        if ( oisCached != ois::Unknown && oisCached != ois::ValidNoFUCB )
        {
            Assert( ois == oisCached );
        }
    }
    else if ( err == JET_errRBSFDPToBeDeleted )
    {
        // TODO: This is thrown when we try to open LV root. Should OpenLVRoot be fixed to do DirOpenNoTouch?
        pdbmObjectCache->MarkCacheObjectToBeDeleted( objid );
    }
    else
    {
        Assert( ois == ois::Unknown );
    }

    return ois;
}

// Called on a page whose b-tree has been deleted
// 
// Important: this call may release the page in case of failure.
ERR DBMScanObserverCleanup::ErrCleanupUnusedPage_( CSR * const pcsr )
{
    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().FAssertRDWLatch( ) );

    ERR err = JET_errSuccess;
    bool fInTransaction = false;

    // ErrNDScrubOneUnusedPage below performs node deletion in addition to scrubbing,,
    // but we're skipping the entire operation because there is no penalty in
    // not cleaning up unused pages because they are not supposed to be consumed anyways.
    if ( !BoolParam( PinstFromPpib( m_ppib ), JET_paramZeroDatabaseUnusedSpace ) )
    {
        goto HandleError;
    }

    if ( !pcsr->Cpage().FScrubbed() )
    {
        Call( ErrDIRBeginTransaction( m_ppib, 44325, NO_GRBIT ) );
        fInTransaction = fTrue;

        pcsr->UpgradeFromRIWLatch();
        err = ErrNDScrubOneUnusedPage( m_ppib, m_ifmp, pcsr, fDIRNull );
        pcsr->Downgrade( latchRIW );
        Call( err );
    
        PERFOpt( perfctrDBMPagesZeroed.Inc( PinstFromPpib( m_ppib ) ) );

        Call( ErrDIRCommitTransaction( m_ppib, JET_bitCommitLazyFlush ) );
    }
    fInTransaction = fFalse;

HandleError:
    if( fInTransaction )
    {
        Assert( pcsr->FLatched() );

        // First we must release the page. This is not actually strictly needed
        // since there is nothing to rollback, but it's good form to release the
        // page before we latch in case we end up tweaking the code later
        // on and doing an actual operation that would require rolling back.
        pcsr->ReleasePage();
        pcsr->Reset();
        
        CallSx( ErrDIRRollback( m_ppib ), JET_errRollbackError );
    }
    return err;
}

// Cleanup LV with zero ref count(orphan LV).
ERR DBMScanObserverCleanup::ErrDeleteZeroRefCountLV( FCB *pfcbLV, const BOOKMARK& bm )
{
    ERR err = JET_errSuccess;
    DELETELVTASK * ptask = new DELETELVTASK( pfcbLV->PgnoFDP(), pfcbLV, m_ifmp, bm );
    Alloc( ptask );
    // Assert PIB is not in a transaction
    Assert( m_ppib->Level() == levelMin );
    Call( ptask->ErrExecute( m_ppib ) );
HandleError:
    if ( ptask != NULL )
    {
        delete ptask;
    }
    return err;
}

// Cleanup an LV page. All versions on the page are committed
// 
// Important: this call may release the page in case of failure.
ERR DBMScanObserverCleanup::ErrCleanupLVPage_( CSR * const pcsr, DBMObjectCache* pdbmObjectCache )
{
    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().FLongValuePage() );
    Assert( pcsr->Cpage().Dbtime() < g_rgfmp[m_ifmp].DbtimeOldestGuaranteed() );

    ERR err = JET_errSuccess;
    FUCB * pfucbLV = pfucbNil;
    bool fBTDeleteNeeded = fFalse;
    BOOKMARK bmBTDelete;
    CArray<LvId> arrLid;
    LVKEY_BUFFER lvkeyToDelete;
    OBJID objidLV = pcsr->Cpage().ObjidFDP();

    for ( INT iline = 0; iline < pcsr->Cpage().Clines(); ++iline )
    {
        KEYDATAFLAGS kdf;
        NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
    
        if ( !FNDDeleted( kdf ) && FIsLVRootKey( kdf.key ) )
        {
            if ( sizeof( LVROOT ) != kdf.data.Cb() && sizeof( LVROOT2 ) != kdf.data.Cb() )
            {
                AssertSz( fFalse, "Corrupted LV: corrupted LVROOT" );
                LvId lidT;
                LidFromKey( &lidT, kdf.key );
                LVReportAndTrapCorruptedLV2( PinstFromIfmp( m_ifmp ), g_rgfmp[m_ifmp].WszDatabaseName(), "", pcsr->Pgno(), lidT, L"a760770f-35cd-45b8-b832-bd4f78237c20" );
                Call( ErrERRCheck( JET_errLVCorrupted ) );
            }
            const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
            if ( 0 == plvroot->ulReference || FPartiallyDeletedLV( plvroot->ulReference ) )
            {
                if ( 0 == arrLid.Capacity() )
                {
                    if ( arrLid.ErrSetCapacity( pcsr->Cpage().Clines() - iline ) != CArray<LvId>::ERR::errSuccess )
                    {
                        Error( ErrERRCheck( JET_errOutOfMemory ) );
                    }
                }
                LvId lid;
                LidFromKey( &lid, kdf.key );
                CArray<LvId>::ERR errT = arrLid.ErrSetEntry( arrLid.Size(), lid );
                Assert( errT == CArray<LvId>::ERR::errSuccess );
            }
        }
        else if ( !fBTDeleteNeeded && FNDDeleted( kdf ) )
        {
            fBTDeleteNeeded = fTrue;

            // Get the bookmark for BTDelete

            kdf.key.CopyIntoBuffer( &lvkeyToDelete, sizeof( lvkeyToDelete ) );
            bmBTDelete.key.prefix.Nullify();
            bmBTDelete.key.suffix.SetPv( &lvkeyToDelete );
            bmBTDelete.key.suffix.SetCb( kdf.key.Cb() );
            bmBTDelete.data.Nullify();
        }
    }

    if ( arrLid.Size() > 0 || fBTDeleteNeeded )
    {
        // Release the latch on the page as the functions to clean up the page below will need to
        // acquire it.
        pcsr->ReleasePage();
        pcsr->Reset();

        pfucbLV = pdbmObjectCache->PfucbGetCachedObject( objidLV );
        Assert( pfucbLV != pfucbNil );

        if ( arrLid.Size() > 0 )
        {
            // Delete zero-ref count LVs
            LVKEY_BUFFER lvkeyZeroRef;  // to hold the pv of bmZeroRef.key temporarily
            BOOKMARK bmZeroRef;
            bmZeroRef.key.prefix.Nullify();
            bmZeroRef.data.Nullify();
            for ( size_t i = 0; i < arrLid.Size(); i++ )
            {
                Assert( bmZeroRef.key.prefix.FNull() );
                Assert( bmZeroRef.data.FNull() );
                LVRootKeyFromLid( &lvkeyZeroRef, &bmZeroRef.key, arrLid[ i ] );
                Assert( !pcsr->FLatched() );
                Call( ErrDeleteZeroRefCountLV( pfucbLV->u.pfcb, bmZeroRef ) );  // copies the bookmark and kicks off a DELETELVTASK
                // Increase the perf count
                PERFOpt( perfctrDBMZeroRefCountLvsDeleted.Inc( PinstFromPpib( m_ppib ) ) );
            }
            // ErrDeleteZeroRefCountLV only flag delete zero-refcount LVs. We still need
            // to wait till next round for ErrBTDelete to expunge all flag-deleted LVs.
        }
        else if ( fBTDeleteNeeded )
        {
            // Delete flag deleted LVs.
            if ( pfucbLV != pfucbNil )
            {
                Assert( !pcsr->FLatched() );
                const ULONG cpgDirtiedBeforeBTDelete = Ptls()->threadstats.cPageDirtied;
                
                // ErrBTDelete deletes all flag-deleted nodes in the page that contains the bm.
                // Warning: Deletes all nodes, even those unrelated to the bm, that just happen
                // to be on the same page as the bm.
                Call( ErrBTDelete( pfucbLV, bmBTDelete ) );
                if ( cpgDirtiedBeforeBTDelete < Ptls()->threadstats.cPageDirtied )
                {
                    PERFOpt( perfctrDBMFDeletedLvPagesReclaimed.Inc( PinstFromPpib( m_ppib ) ) );
                }
            }
        }
    }
HandleError:
    return err;
}

// Given a CSR and the iline of an LVROOT on the page, zero out all
// LVCHUNKs belonging to that LV.
// 
// Important: this call may release the page in case of failure.
ERR DBMScanObserverCleanup::ErrZeroLV_( CSR * const pcsr, const INT iline )
{
    Assert( pcsr->Cpage().FLongValuePage() );
    Assert( pcsr->Cpage().FAssertRDWLatch() );

    ERR err;

    KEYDATAFLAGS kdf;
    NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
    Assert( sizeof( LVROOT ) == kdf.data.Cb() || sizeof( LVROOT2 ) == kdf.data.Cb() );

    const LVROOT * const plvroot = reinterpret_cast<LVROOT *>( kdf.data.Pv() );
    Assert( 0 == plvroot->ulReference );

    const ULONG ulSize = plvroot->ulSize;
    LvId lid;
    LidFromKey( &lid, kdf.key );

    //  we need to keep the root of the LV latched for further processing
    //  use a new CSR to venture onto new pages
    //
    // Important: this call may release the page in case of failure, so we should not use
    // it in failure mode.
    Call( ErrZeroLVChunks_( pcsr, pcsr->Cpage().ObjidFDP(), lid, ulSize ) );

HandleError:
    Assert( err < JET_errSuccess || pcsr->Cpage().FAssertRDWLatch() );
    return err;
}

// Zero out all of the LVCHUNKS for the given LID. This may latch pages to the right of the current page.
// The latch on the current page will not be released.
// 
// Important: this call may release the page in case of failure.
ERR DBMScanObserverCleanup::ErrZeroLVChunks_(
    _In_ CSR * const    pcsrRoot,
    const OBJID         objid,
    const LvId          lid,
    const ULONG         ulSize )
{
    Assert( pcsrRoot->Cpage().FAssertRDWLatch() );
    Expected( m_ppib->Level() == 0 ); // about to scrub an undeterminedly long LV, we shouldn't be in a trx

    ERR err;

    CSR csrT;
    CSR * pcsrT = pcsrRoot;
    
    SCRUBOPER * rgscruboper = NULL;
    while ( true )
    {
        bool fSawLV = fFalse;
        bool fSawGreaterLV = fFalse;
        
        delete [] rgscruboper;
        Alloc( rgscruboper = new SCRUBOPER[pcsrT->Cpage().Clines()] );
        
        for ( INT iline = 0; iline < pcsrT->Cpage().Clines(); iline++ )
        {
            rgscruboper[iline] = scrubOperNone;
            
            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( pcsrT->Cpage(), iline, &kdf );
            LvId lidT;
            LidFromKey( &lidT, kdf.key );
            
            if ( lidT > lid )
            {
                Assert( scrubOperNone == rgscruboper[iline] );
                fSawGreaterLV = fTrue;
            }
            else if ( lidT < lid )
            {
                Assert( scrubOperNone == rgscruboper[iline] );
                Assert( !fSawGreaterLV );
            }
            else if ( FIsLVChunkKey( kdf.key ) )
            {
                fSawLV = fTrue;
                rgscruboper[iline] = scrubOperScrubLVData;
            }
            else
            {
                // the LVROOT can't be scrubbed, so skip it
                Assert( FIsLVRootKey( kdf.key ) );
                Assert( sizeof( LVROOT ) == kdf.data.Cb() || sizeof( LVROOT2 ) == kdf.data.Cb() );
                Assert( scrubOperNone == rgscruboper[iline] );
                fSawLV = fTrue;
            }
        }

        if ( !fSawLV )
        {
            // no need to scrub this page and we have seen all of the LV
            break;
        }

        // Important: this call may release the page from pcsrT. So we should not use
        // it in failure mode.
        Call( ErrScrubOneUsedPage( pcsrT, rgscruboper, pcsrT->Cpage().Clines() ) );
        
        const PGNO pgnoNext = pcsrT->Cpage().PgnoNext();
        if ( pgnoNull == pgnoNext || fSawGreaterLV )
        {
            // reached the end of the tree/LV
            break;
        }

        if ( pcsrT == pcsrRoot )
        {
            // we have to leave the root of the LV latched, so use a temporary CSR to move to the next page
            Call( csrT.ErrGetRIWPage( m_ppib, m_ifmp, pgnoNext ) );
            pcsrT = &csrT;
        }
        else
        {
            Assert( pcsrT == &csrT );
            Assert( pcsrT->Cpage().FAssertRDWLatch() );
            Call( pcsrT->ErrSwitchPage( m_ppib, m_ifmp, pgnoNext ) );
        }

        // Keeping the latch while moving from one page to the next protects us from the case where a
        // page is freed. The case we are looking for here is wholesale space reuse. The scenario is:
        //  1. We decide to scrub an LV.
        //  2. While scrubbing the LV the table containing the LV is deleted, freeing all the pages.
        //  3. One of the pages is used by a different b-tree
        // We can recognize the case by looking at the objid to see if it changed. If so, we abandon
        // the scrub. The LV b-tree is now deleted so it will be scrubbed the next time.
        if ( objid != pcsrT->Cpage().ObjidFDP() )
        {
#ifndef RTM
            bool fExists;
            CallS( ErrCATMSObjidsRecordExists( m_ppib, m_ifmp, objid, &fExists ) );
            AssertRTL( false == fExists );
#endif
            goto HandleError;
        }
    }

HandleError:
    csrT.ReleasePage( fTrue );
    delete[] rgscruboper;
    Assert( err < JET_errSuccess || pcsrRoot->Cpage().FAssertRDWLatch() );
    return err;
}


// Cleanup a primary index page. All versions on the page are committed.
ERR DBMScanObserverCleanup::ErrCleanupPrimaryPage_( CSR * const pcsr, DBMObjectCache* const pdbmObjectCache )
{
    Assert( pcsr->Cpage().FPrimaryPage() );
    Assert( pcsr->Cpage().Dbtime() < g_rgfmp[m_ifmp].DbtimeOldestGuaranteed() );
    
    ERR err = JET_errSuccess;
    
    const OBJID objid = pcsr->Cpage().ObjidFDP();
    FUCB * pfucbTable = pfucbNil;

    if ( pcsr->Cpage().FAnyLineHasFlagSet( fNDDeleted ) )
    {
        pfucbTable = pdbmObjectCache->PfucbGetCachedObject( objid );
        Assert( pfucbTable != pfucbNil );
        for ( INT iline = 0; iline < pcsr->Cpage().Clines(); ++iline )
        {
            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
        
            if ( FNDDeleted( kdf ) )
            {
                // Two cases of scrubbing which cause the number of dirty pages to increase after ErrBTDelete:
                // 1. This node (flag-deleted) is the only node of a single page b-tree, and the new dirty 
                //    page is caused by ErrNDReplace inside ErrBTISPCDeleteNodes by replacing/scrubbing a 
                //    single byte NULL (chSCRUBDBMaintEmptyPageLastNodeFill) of the node's data.
                // 2. Otherwise, some nodes are scrubbed by ErrNDDelete in ErrBTDelete.
                //
                // Reason why ErrBTDelete cannot merge case 1 pages:
                //    In ErrBTISinglePageCleanup, ErrBTISPCDeleteNodes will nullify the node's data (replace with
                //    a single byte NULL chSCRUBDBMaintEmptyPageLastNodeFill) but it can't remove the only node in 
                //    the page (b-tree pages can't be empty), and return MultipageOLC. Then ErrBTIMultipageCleanup will 
                //    return wrnBTShallowTree without doing anything.
                //

                // Avoid repeated replacing/scrubbing of case 1 pages 
                if ( pcsr->Cpage().FLeafPage() &&
                     pcsr->Cpage().FRootPage() && pcsr->Cpage().Clines() == 1 &&
                     kdf.data.Cb() == sizeof( chSCRUBDBMaintEmptyPageLastNodeFill ) &&
                     ( ( BYTE * )kdf.data.Pv() )[0] == chSCRUBDBMaintEmptyPageLastNodeFill )
                {
                    // If the b-tree is a single page tree, whose last flag-deleted node is already replaced with 
                    // bNULL by ErrNDReplace inside ErrBTISPCDeleteNodes, we skip the scrubbing. 
                    goto HandleError;
                }

                // Extract the bookmark
                // Call ErrBTDelete
                unique_ptr<BYTE> pbBookmark( new BYTE[kdf.key.Cb()] );
                Alloc( pbBookmark.get() );
                kdf.key.CopyIntoBuffer( pbBookmark.get(), kdf.key.Cb() );
                
                BOOKMARK bm;
                bm.key.prefix.Nullify();
                bm.key.suffix.SetPv( pbBookmark.get() );
                bm.key.suffix.SetCb( kdf.key.Cb() );
                bm.data.Nullify();

                // Release the page before calling ErrBTDelete.
                pcsr->ReleasePage();
                pcsr->Reset();

                const ULONG cpgDirtiedBeforeBTDelete = Ptls()->threadstats.cPageDirtied;

                Call( ErrBTDelete( pfucbTable, bm ) );

                // Scrubbing is performed by lower layers, which are already honoring JET_paramZeroDatabaseUnusedSpace.
                // Therefore, do not increment the counter if scrubbing is not enabled, even if some form of cleanup
                // was done.
                if ( BoolParam( PinstFromPpib( m_ppib ), JET_paramZeroDatabaseUnusedSpace ) && ( cpgDirtiedBeforeBTDelete < Ptls()->threadstats.cPageDirtied ) )
                {
                    PERFOpt( perfctrDBMPagesZeroed.Inc( PinstFromPpib( m_ppib ) ) );
                }
                // We can break since ErrBTDelete processes all records/lines on this page.
                break;
            }
        }
    }

HandleError:
    return err;
}

// Cleanup an index page. All versions on the page are committed
// 
// Important: this call may release the page in case of failure.
ERR DBMScanObserverCleanup::ErrCleanupIndexPage_( CSR * const pcsr )
{
    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().FIndexPage() );
    Assert( pcsr->Cpage().Dbtime() < g_rgfmp[m_ifmp].DbtimeOldestGuaranteed() );
    
    ERR err;
    Call( ErrCleanupDeletedNodes_( pcsr ) );

HandleError:
    return err;
}

//  for each node on the page:
//    if it is flag-deleted zero out the data
ERR DBMScanObserverCleanup::ErrCleanupDeletedNodes_( CSR * const pcsr )
{
    Assert( pcsr->FLatched() );
    Assert( pcsr->Cpage().Dbtime() < g_rgfmp[m_ifmp].DbtimeOldestGuaranteed() );
    Assert( pcsr->Cpage().FLeafPage() );
    Assert( pcsr->Cpage().FAssertRDWLatch() );

    ERR err = JET_errSuccess;
    SCRUBOPER * rgscruboper = NULL;

    if ( !BoolParam( PinstFromPpib( m_ppib ), JET_paramZeroDatabaseUnusedSpace ) )
    {
        goto HandleError;
    }

    // Don't zero out the data for non-unique indices. The data is part of the bookmark
    // and can't be removed or the sort order will be wrong.
    if ( pcsr->Cpage().FAnyLineHasFlagSet( fNDDeleted ) && !pcsr->Cpage().FNonUniqueKeys() )
    {
        Alloc( rgscruboper = new SCRUBOPER[pcsr->Cpage().Clines()] );
        
        for ( INT iline = 0; iline < pcsr->Cpage().Clines(); ++iline )
        {
            KEYDATAFLAGS kdf;
            NDIGetKeydataflags( pcsr->Cpage(), iline, &kdf );
        
            if ( FNDDeleted( kdf ) )
            {
                rgscruboper[iline] = scrubOperScrubData;
            }
            else
            {
                rgscruboper[iline] = scrubOperNone;
            }
        }
        
        Call( ErrScrubOneUsedPage( pcsr, rgscruboper, pcsr->Cpage().Clines() ) );
    }
    
HandleError:
    Assert( err < JET_errSuccess || pcsr->Cpage().FAssertRDWLatch( ) );
    delete[] rgscruboper;
    return err;
}

VOID DBMScanObserverCleanup::RedeleteRevertedFDP_( CSR* const pcsr, DBMObjectCache* const pdbmObjectCache )
{
    // Only the root page of the table should be marked with this special flag warranting redelete.
    Assert( pcsr->Cpage().FRootPage() );
    Assert( pcsr->Cpage().FPrimaryPage() );
    Assert( !FCATSystemTableByObjid( pcsr->Cpage().ObjidFDP() ) );

    Assert( FPIBSessionSystemCleanup( m_ppib ) );

    BOOL fFDPExists     = fTrue;
    BOOL fInTransaction = fFalse;
    ERR err             = JET_errSuccess;
    OBJID objidTable    = pcsr->Cpage().ObjidFDP();

    CHAR szTableName[ JET_cbNameMost + 1 ];
    PGNO pgnoTable          = pcsr->Cpage().PgnoThis();
    PGNO pgnoTableFromCat   = pgnoNull;

    // Close any FUCBs we have open for the table we are about to delete.
    pdbmObjectCache->CloseCachedObjectWithObjid( objidTable );
    Assert( pdbmObjectCache->PfucbGetCachedObject( objidTable ) == pfucbNil );

    // Lets pull information needed for FDP delete operation.
    Call( ErrDIRBeginTransaction( m_ppib, 54684, NO_GRBIT ) );
    fInTransaction = fTrue;

    // The given page should be a table root page since for LV and Secondary Index table root pages we ignore clean up.
    Assert( !pcsr->Cpage().FLongValuePage() );
    Assert( !pcsr->Cpage().FIndexPage() );

    // Release page, so that delete table can operate on it.
    pcsr->ReleasePage();
    pcsr->Reset();

    err = ErrCATSeekTableByObjid(
        m_ppib,
        m_ifmp,
        objidTable,
        szTableName,
        sizeof( szTableName ),
        &pgnoTableFromCat );

    Assert( pgnoTableFromCat == pgnoTable );

    if ( err == JET_errObjectNotFound )
    {
        // Table already cleaned up.
        fFDPExists = fFalse;
        err = JET_errSuccess;
        goto HandleError;
    }

    // Re-deleting table.
    const JET_SESID sesid = (JET_SESID) m_ppib;
    const JET_DBID  dbid = (JET_DBID) m_ifmp;

    Call( ErrIsamDeleteTable( sesid, dbid, szTableName, fFalse, JET_bitNonRevertableTableDelete ) );
    Call( ErrDIRCommitTransaction( m_ppib, NO_GRBIT ) );
    fInTransaction = fFalse;

    // We have marked table for delete. Mark cached object invalid, if it exists.
    Assert( pdbmObjectCache->PfucbGetCachedObject( objidTable ) == pfucbNil );

HandleError:
    if ( fFDPExists )
    {
        WCHAR wszPgnoTable[16], wszObjidTable[16], wszErr[16];
        OSStrCbFormatW( wszPgnoTable, sizeof(wszPgnoTable), L"%ld", pgnoTable );
        OSStrCbFormatW( wszObjidTable, sizeof(wszObjidTable), L"%ld", objidTable );
        OSStrCbFormatW( wszErr, sizeof(wszErr), L"%d", err );

        BOOL fSuccess = err >= JET_errSuccess;

        const WCHAR* rgcwsz[] =
        {
            wszPgnoTable,
            wszObjidTable,
            wszErr
        }; 

        UtilReportEvent(
            fSuccess ? eventInformation : eventError,
            GENERAL_CATEGORY,
            fSuccess ? DBSCAN_REDELETE_REVERTED_TABLE_SUCCESS : DBSCAN_REDELETE_REVERTED_TABLE_FAILURE,
            3,
            rgcwsz,
            0,
            NULL,
            m_ppib->m_pinst );
    }

    if( fInTransaction )
    {
        CallSx( ErrDIRRollback( m_ppib ), JET_errRollbackError );
    }
}

// Begin a transaction, upgrade the latch and call ErrNDScrubOneUsedPage
// 
// Important: this call may release the page in case of failure.
ERR DBMScanObserverCleanup::ErrScrubOneUsedPage(
        CSR * const pcsr,
        __in_ecount( cscrubOper ) const SCRUBOPER * const rgscruboper,
        const INT cscrubOper )
{
    ERR err = JET_errSuccess;
    
    Assert( pcsr->FLatched() );

    bool fInTransaction = false;
    if ( !pcsr->Cpage().FScrubbed() )
    {
        Call( ErrDIRBeginTransaction( m_ppib, 60709, NO_GRBIT ) );
        fInTransaction = fTrue;

        pcsr->UpgradeFromRIWLatch();
        err = ErrNDScrubOneUsedPage( m_ppib, m_ifmp, pcsr, rgscruboper, pcsr->Cpage().Clines(), fDIRNull );
        pcsr->Downgrade( latchRIW );
        Call( err );

        PERFOpt( perfctrDBMPagesZeroed.Inc( PinstFromPpib( m_ppib ) ) );

        Call( ErrDIRCommitTransaction( m_ppib, JET_bitCommitLazyFlush ) );
    }
    fInTransaction = fFalse;
    
HandleError:
    if( fInTransaction )
    {
        Assert( pcsr->FLatched() );

        // First we must release the page. This is not actually strictly needed
        // since there is nothing to rollback, but it's good form to release the
        // page before we latch in case we end up tweaking the code later
        // on and doing an actual operation that would require rolling back.
        pcsr->ReleasePage();
        pcsr->Reset();
        
        CallSx( ErrDIRRollback( m_ppib ), JET_errRollbackError );
    }
    return err;
}

// Use the objid of the table to open an FUCB
ERR DBMScanObserverCleanup::ErrOpenTableGetFucb_( const OBJID objid, BOOL* const pfExists, FUCB ** ppfucbTable )
{
    ERR err;

    PGNO pgnoFDP;

    CHAR szTableName[JET_cbNameMost+1];

    *pfExists = fTrue;
    *ppfucbTable = pfucbNil;

    // If the PIB isn't marked as a system cleanup session then we will conflict with
    // JetDeleteTable.
    Assert( FPIBSessionSystemCleanup( m_ppib ) );
    Call( ErrDIRBeginTransaction( m_ppib, 54683, JET_bitTransactionReadOnly ) );

    err = ErrCATSeekTableByObjid(
                m_ppib,
                m_ifmp,
                objid,
                szTableName,
                sizeof( szTableName ),
                &pgnoFDP );

    if ( err == JET_errObjectNotFound )
    {
        *pfExists = fFalse;
        err = JET_errSuccess;
    }
    else
    {
        if ( err >= JET_errSuccess )
        {
            err = ErrFILEOpenTable( m_ppib, m_ifmp, ppfucbTable, szTableName, JET_bitTableAllowSensitiveOperation );
        }
    }

    CallS( ErrDIRCommitTransaction( m_ppib, NO_GRBIT ) );

HandleError:
    return err;
}

//  ================================================================
//  DBMObjectCache
//  ================================================================

DBMObjectCache::DBMObjectCache()
{
    for( INT i = 0; i < m_cobjectsMax; ++i )
    {
        m_rgstate[i].objid = objidNil;
        m_rgstate[i].ois = ois::Unknown;
        m_rgstate[i].pfucb = pfucbNil;
        m_rgstate[i].ftAccess = 0;
    }
}

DBMObjectCache::~DBMObjectCache()
{
    CloseAllCachedObjects();
}

ObjidState DBMObjectCache::OisGetObjidState( const OBJID objid )
{
    Assert( objidNil != objid );
    const INT index = IndexOfObjid_( objid );
    if( index >= 0 )
    {
        AssertPREFIX( index < m_cobjectsMax );
        m_rgstate[index].ftAccess = UtilGetCurrentFileTime();
        const ObjidState ois = m_rgstate[index].ois;
        const FUCB * const pfucb = m_rgstate[index].pfucb;
        Assert( ( ois == ois::Valid ) == ( pfucb != pfucbNil ) );
        return ois;
    }
    else
    {
        return ois::Unknown;
    }
}

FUCB * DBMObjectCache::PfucbGetCachedObject( const OBJID objid )
{
    Assert( objidNil != objid );
    const INT index = IndexOfObjid_( objid );
    if( index >= 0 )
    {
        AssertPREFIX( index < m_cobjectsMax );
        m_rgstate[index].ftAccess = UtilGetCurrentFileTime();
        const ObjidState ois = m_rgstate[index].ois;
        FUCB * const pfucb = m_rgstate[index].pfucb;
        Assert( ( ois == ois::Valid ) == ( pfucb != pfucbNil ) );
        return pfucb;
    }
    else
    {
        return pfucbNil;
    }
}

void DBMObjectCache::CacheObjectFucb( FUCB * const pfucb, const OBJID objid )
{
    Assert( objidNil != objid );
    Assert( pfucbNil != pfucb );
    
    INT index = IndexOfObjid_( objid );
    if ( index < 0 )
    {
#ifndef ENABLE_JET_UNIT_TEST
        BOOL fIsLV = pfucb->u.pfcb->FTypeLV();
        Assert( fIsLV ? ( NULL != pfucb->u.pfcb->PfcbTable() ) : ( NULL == pfucb->u.pfcb->PfcbTable() ) );
#else
        // Unit tests occasionally get here with a non-null but bad pfucb->u.pfcb.  None of the
        // unit tests rely on the specific LV tree behavior.
        BOOL fIsLV = fFalse;
#endif
        // Not already cached.
        if ( fIsLV )
        {
            // If we're caching an LV tree, we need to already have the table in the cache, and
            // we can't evict it.
            const OBJID objidTableForLVTree = pfucb->u.pfcb->PfcbTable()->ObjidFDP();
            Assert( ois::Valid == OisGetObjidState( objidTableForLVTree ) );
            index = IndexOfLeastRecentlyUsedObject_( objidTableForLVTree );
        }
        else
        {
            index = IndexOfLeastRecentlyUsedObject_();
        }
        CloseObjectAt_( index );
    }
    else
    {
        Assert( m_rgstate[index].objid == objid );
        Assert( m_rgstate[index].ois == ois::Unknown || m_rgstate[index].ois == ois::ValidNoFUCB );
        Assert( m_rgstate[index].pfucb == pfucbNil );
    }

#ifdef ENABLE_FUCB_IN_DBSCAN_CACHE_PROTECTION
    // Need the lock so we don't race with someone growing or reordering the FucbList.
    pfucb->u.pfcb->Lock();
    pfucb->u.pfcb->FucbList().LockForEnumeration();
    Assert( !pfucb->m_iae.FProtected() );
    pfucb->m_iae.SetProtected();
    pfucb->u.pfcb->FucbList().UnlockForEnumeration();
    pfucb->u.pfcb->Unlock();
#endif

    m_rgstate[index].objid = objid;
    m_rgstate[index].ois = ois::Valid;
    m_rgstate[index].pfucb = pfucb;
    m_rgstate[index].ftAccess = UtilGetCurrentFileTime();
    Assert( FContainsObjid_( objid ) );
    Assert( pfucb == PfucbGetCachedObject( objid ) );
    Assert( ois::Valid == OisGetObjidState( objid ) );
}

void DBMObjectCache::MarkCacheObjectInvalid( const OBJID objid )
{
    Assert( objidNil != objid );

    INT index = IndexOfObjid_( objid );
    if ( index < 0 )
    {
        index = IndexOfLeastRecentlyUsedObject_();
        CloseObjectAt_( index );
    }
    else
    {
        Assert( m_rgstate[index].objid == objid );
        Assert( m_rgstate[index].ois != ois::Valid );
        Assert( m_rgstate[index].pfucb == pfucbNil );
    }

    m_rgstate[index].objid = objid;
    m_rgstate[index].ois = ois::Invalid;
    m_rgstate[index].pfucb = pfucbNil;
    m_rgstate[index].ftAccess = UtilGetCurrentFileTime();
    Assert( FContainsObjid_( objid ) );
    Assert( pfucbNil == PfucbGetCachedObject( objid ) );
    Assert( ois::Invalid == OisGetObjidState( objid ) );
}

void DBMObjectCache::MarkCacheObjectToBeDeleted( const OBJID objid )
{
    Assert( objidNil != objid );

    INT index = IndexOfObjid_( objid );
    if ( index < 0 )
    {
        index = IndexOfLeastRecentlyUsedObject_();
        CloseObjectAt_( index );
    }
    else
    {
        Assert( m_rgstate[index].objid == objid );
        Assert( m_rgstate[index].ois != ois::Valid );
        Assert( m_rgstate[index].pfucb == pfucbNil );
    }

    m_rgstate[index].objid = objid;
    m_rgstate[index].ois = ois::ToBeDeleted;
    m_rgstate[index].pfucb = pfucbNil;
    m_rgstate[index].ftAccess = UtilGetCurrentFileTime();
    Assert( FContainsObjid_( objid ) );
    Assert( pfucbNil == PfucbGetCachedObject( objid ) );
    Assert( ois::ToBeDeleted == OisGetObjidState( objid ) );
}

void DBMObjectCache::MarkCacheObjectValidNoFUCB( const OBJID objid )
{
    Assert( objidNil != objid );

    INT index = IndexOfObjid_( objid );
    if ( index < 0 )
    {
        index = IndexOfLeastRecentlyUsedObject_();
        CloseObjectAt_( index );
    }
    else
    {
        Assert( m_rgstate[index].objid == objid );
        Assert( m_rgstate[index].ois != ois::Valid );
        Assert( m_rgstate[index].pfucb == pfucbNil );
    }

    m_rgstate[index].objid = objid;
    m_rgstate[index].ois = ois::ValidNoFUCB;
    m_rgstate[index].pfucb = pfucbNil;
    m_rgstate[index].ftAccess = UtilGetCurrentFileTime();
    Assert( FContainsObjid_( objid ) );
    Assert( pfucbNil == PfucbGetCachedObject( objid ) );
    Assert( ois::ValidNoFUCB == OisGetObjidState( objid ) );
}

void DBMObjectCache::CloseCachedObjectWithObjid( const OBJID objid )
{
    Assert( objidNil != objid );

    INT index = IndexOfObjid_( objid );

    if ( index < 0 )
    {
        return;
    }

    Assert( m_rgstate[index].objid == objid );
    Assert( m_rgstate[index].ois == ois::Valid );
    Assert( m_rgstate[index].pfucb != pfucbNil );

    CloseObjectAt_( index );
}

void DBMObjectCache::CloseCachedObjectsWithPendingDeletes()
{
    for( INT i = 0; i < m_cobjectsMax; ++i )
    {
        if ( ( pfucbNil != m_rgstate[i].pfucb ) && m_rgstate[i].pfucb->u.pfcb->FDeletePending() )
        {
            CloseObjectAt_( i );
        }
    }
}

void DBMObjectCache::CloseAllCachedObjects()
{
    for( INT i = 0; i < m_cobjectsMax; ++i )
    {
#ifndef ENABLE_JET_UNIT_TEST
        // Some unit tests get here with a non-NULL but garbage pfucb in m_rgstate.  This hang
        // injection is not used in unit tests.
        if ( ( pfucbNil != m_rgstate[i].pfucb ) && ( m_rgstate[i].pfucb->u.pfcb->FTypeLV() ) )
        {
            HangInjection( 41270 );
        }
#endif
        CloseObjectAt_( i );
    }
}

bool DBMObjectCache::FContainsObjid_( const OBJID objid ) const
{
    return ( -1 != IndexOfObjid_( objid ) );
}

INT DBMObjectCache::IndexOfObjid_( const OBJID objid ) const
{
    for( INT i = 0; i < m_cobjectsMax; ++i )
    {
        if( m_rgstate[i].objid == objid )
        {
            return i;
        }
    }
    return -1;
}

INT DBMObjectCache::IndexOfLeastRecentlyUsedObject_( const OBJID objidIgnore ) const
{
    static_assert( 1 != m_cobjectsMax, "Algorithm assumes at least two entries" );
    const __int64 ftMax = 0x7FFFFFFFFFFFFFFF;
    __int64 ftLeast = ftMax;
    INT indexLeast = -1;
    for( INT i = 0; i < m_cobjectsMax; ++i )
    {
        if ( ( objidIgnore != objidNil ) && ( objidIgnore == m_rgstate[i].objid ) )
        {
            // We don't want to find this one.
            continue;
        }
        
        if( ( m_rgstate[i].ftAccess == 0 ) || ( m_rgstate[i].ftAccess < ftLeast ) )
        {
            indexLeast = i;
            ftLeast = m_rgstate[i].ftAccess;

            // Unused entry.
            if( m_rgstate[i].ftAccess == 0 )
            {
                break;
            }
        }
    }
    Assert( -1 != indexLeast );
    Assert( ftMax != ftLeast );
    return indexLeast;
}

void DBMObjectCache::CloseObjectAt_( const INT index )
{
    if ( pfucbNil != m_rgstate[index].pfucb )
    {
        Assert( ois::Valid == m_rgstate[index].ois );

#ifdef ENABLE_FUCB_IN_DBSCAN_CACHE_PROTECTION
        // Need the lock so we don't race with someone growing or reordering the FucbList.
        m_rgstate[index].pfucb->u.pfcb->Lock();
        m_rgstate[index].pfucb->u.pfcb->FucbList().LockForEnumeration();
        Assert( m_rgstate[index].pfucb->m_iae.FProtected() );
        m_rgstate[index].pfucb->m_iae.ResetProtected();
        m_rgstate[index].pfucb->u.pfcb->FucbList().UnlockForEnumeration();
        m_rgstate[index].pfucb->u.pfcb->Unlock();
#endif

#ifndef ENABLE_JET_UNIT_TEST
        if ( m_rgstate[index].pfucb->u.pfcb->FTypeLV() )
        {
            DIRClose( m_rgstate[index].pfucb );
        }
        else
        {
            const FCB *pfcb = m_rgstate[index].pfucb->u.pfcb;
            Assert( pfcb->FTypeTable() );

            const TDB *ptdb = pfcb->Ptdb();
            Assert( ptdb );
            if ( NULL != ptdb )
            {
                // If we're closing a table with an LV tree that is also cached, we need to
                // close the LV tree also.  If we don't, we end up with problems if someone deletes
                // the table (deleting the the LV tree), leaving an orphaned FUCB for the LV tree
                // here that points to an FCB that may have been purged.
                const FCB *pfcbLV = ptdb->PfcbLV();
                if ( NULL != pfcbLV )
                {
                    // If this is cached, we need to close it also.
                    INT indexLV = IndexOfObjid_( pfcbLV->ObjidFDP() );
                    if ( -1 != indexLV )
                    {
                        CloseObjectAt_( indexLV );
                    }
                }
            }
            CallS( ErrFILECloseTable( m_rgstate[index].pfucb->ppib, m_rgstate[index].pfucb ) );
        }
#endif // ENABLE_JET_UNIT_TEST
    }
    m_rgstate[index].objid = objidNil;
    m_rgstate[index].ois = ois::Unknown;
    m_rgstate[index].pfucb = pfucbNil;
    m_rgstate[index].ftAccess = 0;
}


//  ================================================================
//  DBMScan
//  ================================================================
    
DBMScan::DBMScan(
        IDBMScanState * const pscanstate,
        IDBMScanReader * const pscanreader,
        IDBMScanConfig * const pscanconfig,
        PIB* ppib ) :
    IDBMScan(),
    m_pscanstate( pscanstate ),
    m_pscanreader( pscanreader ),
    m_pscanconfig( pscanconfig ),
    m_ppib( ppib ),
    m_cscanobservers( 0 ),
    m_threadDBMScan( 0 ),
    m_critSignalControl( CLockBasicInfo( CSyncBasicInfo( _T("DBMScan::m_critSignalControl" ) ), rankDBMScanSignalControl, 0 ) ),
    m_msigDBScanStop( CSyncBasicInfo( _T("DBMScan::m_msigDBScanStop" ) ) ),
    m_msigDBScanGo( CSyncBasicInfo( _T("DBMScan::m_msigDBScanGo" ) ) ),
    m_pidbmScanSerializationObj( NULL ),
    m_cscansFinished( 0 ),
    m_fNeedToSuspendPass( false ),
    m_fSerializeScan( false )
{
    Assert( NULL != pscanstate );
    Assert( NULL != pscanconfig );
    Assert( NULL != pscanreader );
}

DBMScan::~DBMScan()
{
    TermDBMScan_();

    m_objectCache.CloseAllCachedObjects();

    if ( m_ppib != ppibNil )
    {
        CallS( ErrDBCloseAllDBs( m_ppib ) );
        PIBEndSession( m_ppib );
        m_ppib = ppibNil;
    }
}

void DBMScan::AddObserver( DBMScanObserver * const pscanobserver )
{
    Assert( NULL != pscanobserver );
    Assert( !FIsDBMScanRunning_() );
    AssertRTL( m_cscanobservers < m_cscanobserversMax );
    m_rgpscanobservers[m_cscanobservers++] = unique_ptr<DBMScanObserver>( pscanobserver );
    AssertRTL( m_cscanobservers < m_cscanobserversMax );
}

ERR DBMScan::ErrStartDBMScan()
{
    if ( FIsDBMScanRunning_() )
    {
        return ErrERRCheck( JET_errDatabaseAlreadyRunningMaintenance );
    }
    return ErrStartDBMScan_();
}

ERR DBMScan::ErrStartDBMScan_()
{
    ERR err;

    Assert( !FIsDBMScanRunning_() );
    Assert( m_pidbmScanSerializationObj == NULL );

    const ULONG_PTR ulDiskId = m_pscanconfig->UlDiskId();
    Expected( ulDiskId != 0 );

    m_fSerializeScan = m_pscanconfig->FSerializeScan();
    if ( m_fSerializeScan )
    {
        Call( g_dbmSerializerFactory.ErrRegisterRealSerializer( ulDiskId, &m_pidbmScanSerializationObj ) );
    }
    else
    {
        Call( g_dbmSerializerFactory.ErrRegisterDummySerializer( ulDiskId, &m_pidbmScanSerializationObj ) );
    }

    Assert( !g_dbmSerializerFactory.FSerializerFactoryEmpty() );
    Assert( m_pidbmScanSerializationObj != NULL );
    Assert( m_pidbmScanSerializationObj->UlSerializerKey() == ulDiskId );
    Assert( m_pidbmScanSerializationObj->CSerializerRef() > 0 );

    ResetScanStop_();

    Call( ErrUtilThreadCreate( DwDBMScanThreadProc_, 0, priorityNormal, &m_threadDBMScan, ( DWORD_PTR )this ) );

    Assert( FIsDBMScanRunning_() );

HandleError:
    if ( err < JET_errSuccess )
    {
        TermDBMScan_();
    }

    return err;
}

void DBMScan::StopDBMScan()
{
    TermDBMScan_();
}

void DBMScan::TrySetScanGo()
{
    Expected( m_fSerializeScan );

    //  m_msigDBScanStop has priority.
    m_critSignalControl.Enter();
    if ( !m_msigDBScanStop.FIsSet() )
    {
        m_msigDBScanGo.Set();
    }
    else
    {
        Assert( m_msigDBScanGo.FIsSet() );
    }
    m_critSignalControl.Leave();
}

void DBMScan::TryResetScanGo()
{
    Expected( m_fSerializeScan );

    //  m_msigDBScanStop has priority.
    m_critSignalControl.Enter();
    if ( !m_msigDBScanStop.FIsSet() )
    {
        m_msigDBScanGo.Reset();
    }
    else
    {
        Assert( m_msigDBScanGo.FIsSet() );
    }
    m_critSignalControl.Leave();
}

void DBMScan::WaitForScanStop()
{
    m_msigDBScanStop.Wait();
}

void DBMScan::WaitForScanGo( const INT msec )
{
    Expected( m_fSerializeScan );
    //  Disable deadlock detection.
    const INT cmsecTimeout = msec < 0 ? msec : -msec;
    ( void )m_msigDBScanGo.FWait( cmsecTimeout );
}

void DBMScan::TermDBMScan_()
{
    SetScanStop_();

    // Let the observers know that we are terminating.
    PrepareToTerm_();

    if ( m_threadDBMScan != NULL )
    {
        UtilThreadEnd( m_threadDBMScan );
    }
    m_threadDBMScan = NULL;
    ResetScanStop_();
    if ( m_pidbmScanSerializationObj != NULL )
    {
        Assert( !g_dbmSerializerFactory.FSerializerFactoryEmpty() );
        Assert( m_pidbmScanSerializationObj->CSerializerRef() > 0 );
        g_dbmSerializerFactory.UnregisterSerializer( m_pidbmScanSerializationObj );
    }
    m_pidbmScanSerializationObj = NULL;
    Assert( !FIsDBMScanRunning_() );
}

void DBMScan::SetScanStop_()
{
    m_critSignalControl.Enter();
    m_msigDBScanStop.Set();
    m_msigDBScanGo.Set();
    m_critSignalControl.Leave();
}

void DBMScan::ResetScanStop_()
{
    m_critSignalControl.Enter();
    m_msigDBScanStop.Reset();
    m_msigDBScanGo.Reset();
    m_critSignalControl.Leave();
}

DWORD DBMScan::DwDBMScanThreadProc_( DWORD_PTR dwContext )
{
    DBMScan * const pdbmscan = reinterpret_cast<DBMScan *>( dwContext );
    Assert( NULL != pdbmscan );
    return pdbmscan->DwDBMScan_();
}

DWORD DBMScan::DwDBMScan_()
{
    m_ftTimeLimit = UtilGetCurrentFileTime() + UtilConvertSecondsToFileTime( m_pscanconfig->CSecMax() );

    if ( m_pscanconfig->Pinst() )
    {
        PERFOpt( perfctrDBMThrottleSetting.Set( m_pscanconfig->Pinst(), m_pscanconfig->DwThrottleSleep() ) );
    }

    WaitForMinPassTime_();
    while ( !m_msigDBScanStop.FIsSet() && !FTimeLimitReached_() )
    {
        if ( FResumingPass_() )
        {
            ResumePass_();
        }
        else
        {
            StartNewPass_();
        }
        DoOnePass_();
        if ( FMaxScansReached_() )
        {
            break;
        }
        WaitForMinPassTime_();
    }

    if ( m_pscanconfig->Pinst() )
    {
        PERFOpt( perfctrDBMThrottleSetting.Set( m_pscanconfig->Pinst(), 0 ) );
    }

    // we are exiting. if the pass is still in progress then suspend it
    if ( m_fNeedToSuspendPass )
    {
        SuspendPass_();
    }
    return 0;
}

void DBMScan::DoOnePass_()
{
    bool fRunnable = false;
    CPG cpgBatch = m_pscanconfig->CpgBatch();
    Assert( cpgBatch > 0 );
    DWORD tickStart = 0;
    DWORD dtickTimeSlice = 0;
    __int64 iSecNotifyStart = UtilGetCurrentFileTime();
    // Choose a reasonable maximum-granularity of work
    cpgBatch = min( cpgBatch, m_pscanreader->cpgPrereadMax );

    while ( !m_msigDBScanStop.FWait( m_pscanconfig->DwThrottleSleep() ) && !FTimeLimitReached_() )
        {
            //  ask permission to run, if necessary.
            if ( !fRunnable )
            {
                fRunnable = m_pidbmScanSerializationObj->FEnqueueAndWait( this, CMSecBeforeTimeLimit_() );

                //  Compute our timeslice.
                if ( fRunnable )
                {
                    tickStart = TickOSTimeCurrent();
                    dtickTimeSlice = ( DWORD )UlConfigOverrideInjection( 41007, m_pidbmScanSerializationObj->DwTimeSlice() );
                    Assert( m_pidbmScanSerializationObj->FDBMScanCurrent( this ) );
                }
                else
                {
                    Assert( !m_pidbmScanSerializationObj->FDBMScanCurrent( this ) );
                }
            }

            //  May we run?
            if ( fRunnable )
            {
                const PGNO pgnoFirst = m_pscanstate->CpgScannedCurrPass() + 1;
                const PGNO pgnoLast = UlFunctionalMin( m_pscanreader->PgnoLast(), pgnoFirst + cpgBatch - 1 );
                const CPG cpgScan = pgnoLast - pgnoFirst + 1;

                if ( cpgScan > 0 )
                {
                    m_pscanreader->PrereadPages( pgnoFirst, cpgScan );
                    PGNO pgno = pgnoNull;
                    for ( pgno = pgnoFirst; pgno < pgnoFirst + cpgScan; ++pgno )
                    {
                        const ERR err = m_pscanreader->ErrReadPage( pgno, m_ppib, &m_objectCache );
                        switch ( err )
                        {
                            case JET_errFileIOBeyondEOF:
                                Expected( fFalse );   // Unexpected because we're not reading past the last page above.
                                PrereadDone_( pgnoFirst, pgno - pgnoFirst );
                                FinishPass_();
                                goto Finished;

                            case JET_errReadPgnoVerifyFailure:
                            case JET_errReadVerifyFailure:
                                BadChecksum_( pgno );
                                break;

                            default:
                                break;
                        }
                        PassReadPage_( pgno );
                    }
                    PrereadDone_( pgnoFirst, pgno - pgnoFirst );
                }
                else
                {
                    FinishPass_();
                    goto Finished;
                }
            }

            //  We may have to give up our timeslice.
            if ( fRunnable && ( ( TickOSTimeCurrent() - tickStart ) >= dtickTimeSlice ) )
            {
                Assert( m_pidbmScanSerializationObj->FDBMScanCurrent( this ) );
                m_pidbmScanSerializationObj->Dequeue( this );
                fRunnable = false;
            }
            
            if ( ( UtilGetCurrentFileTime() - iSecNotifyStart ) >=
                  UtilConvertSecondsToFileTime( m_pscanconfig->CSecNotifyInterval() ) )
            {
                iSecNotifyStart = UtilGetCurrentFileTime();
                ForEachObserverCall_( &DBMScanObserver::NotifyStats );
            }
        }

Finished:

    if ( fRunnable )
    {
        Assert( m_pidbmScanSerializationObj->FDBMScanCurrent( this ) );
        m_pidbmScanSerializationObj->Dequeue( this );
    }
    else
    {
        Assert( !m_pidbmScanSerializationObj->FDBMScanCurrent( this ) );
    }

    return;
}

void DBMScan::PrereadDone_( _In_ const PGNO pgnoFirst, _In_ const CPG cpgScan )
{
    // mark all the pages as done at once to mark them all super cold at once to maximize the chance that scrubbed
    // pages will be written out in one IO
    for ( PGNO pgno = pgnoFirst; pgno < pgnoFirst + cpgScan; ++pgno )
    {
        m_pscanreader->DoneWithPreread( pgno );
    }
}

// we are resuming a pass if the state tells us that we have scanned
// some pages already
bool DBMScan::FResumingPass_() const
{
    return m_pscanstate->CpgScannedCurrPass() > 0;
}

void DBMScan::ResumePass_()
{
    m_pscanstate->ResumedPass();
    ForEachObserverCall_( &DBMScanObserver::ResumedPass );
    m_fNeedToSuspendPass = true;
}

void DBMScan::StartNewPass_()
{
    m_pscanstate->StartedPass();
    ForEachObserverCall_( &DBMScanObserver::StartedPass );
    m_fNeedToSuspendPass = true;
}

void DBMScan::FinishPass_()
{
    ++m_cscansFinished;
    m_pscanstate->FinishedPass();
    m_objectCache.CloseAllCachedObjects();
    ForEachObserverCall_( &DBMScanObserver::FinishedPass );
    m_fNeedToSuspendPass = false;
}

void DBMScan::SuspendPass_()
{
    m_pscanstate->SuspendedPass();
    m_objectCache.CloseAllCachedObjects();
    ForEachObserverCall_( &DBMScanObserver::SuspendedPass );
}

void DBMScan::PrepareToTerm_()
{
    // No need to update m_pscanstate.
    ForEachObserverCall_( &DBMScanObserver::PrepareToTerm );
}

void DBMScan::BadChecksum_( const PGNO pgno )
{
    m_pscanstate->BadChecksum( pgno );
    ForEachObserverCall_( &DBMScanObserver::BadChecksum, pgno );
}

void DBMScan::PassReadPage_( const PGNO pgno )
{
    const CPG cpgRead = 1;
    m_pscanstate->ReadPages( pgno, cpgRead );
    ForEachObserverCall_( &DBMScanObserver::ReadPage, pgno, &m_objectCache );
}

void DBMScan::ForEachObserverCall_( void ( DBMScanObserver::* const pfn )( const IDBMScanState * const ) ) const
{
    const IDBMScanState * const pstate = m_pscanstate.get();
    for( INT iscanobserver = 0; iscanobserver < m_cscanobservers; ++iscanobserver )
    {
        DBMScanObserver * const pobserver = m_rgpscanobservers[iscanobserver].get();
        ( pobserver->*pfn )( pstate );
    }
}

template<class Arg>
void DBMScan::ForEachObserverCall_(
    void ( DBMScanObserver::* const pfn )( const IDBMScanState * const, const Arg ),
    const Arg arg ) const
{
    const IDBMScanState * const pstate = m_pscanstate.get();
    for( INT iscanobserver = 0; iscanobserver < m_cscanobservers; ++iscanobserver )
    {
        DBMScanObserver * const pobserver = m_rgpscanobservers[iscanobserver].get();
        ( pobserver->*pfn )( pstate, arg );
    }
}

template<class Arg, class Arg1>
void DBMScan::ForEachObserverCall_(
    void ( DBMScanObserver::* const pfn )( const IDBMScanState * const, const Arg, const Arg1 ),
    const Arg arg, const Arg1 arg1 ) const
{
    const IDBMScanState * const pstate = m_pscanstate.get();
    for( INT iscanobserver = 0; iscanobserver < m_cscanobservers; ++iscanobserver )
    {
        DBMScanObserver * const pobserver = m_rgpscanobservers[iscanobserver].get();
        ( pobserver->*pfn )( pstate, arg, arg1 );
    }
}

void DBMScan::WaitForMinPassTime_()
{
    const __int64 ftNow = UtilGetCurrentFileTime();
    const __int64 csecSincePrevScanCompleted = UtilConvertFileTimeToSeconds(
                                                ftNow - m_pscanstate->FtPrevPassCompletionTime() );
    const __int64 csecBeforeNextPassCanStart = m_pscanconfig->CSecMinScanTime() - csecSincePrevScanCompleted;
    const __int64 csecBeforeTimeLimit        = UtilConvertFileTimeToSeconds( m_ftTimeLimit - ftNow );
    const __int64 csecToWait                 = min( csecBeforeTimeLimit, csecBeforeNextPassCanStart );

    if ( csecToWait > 0 )
    {
        // negative wait time turns off deadlock detection
        Assert( csecToWait <= ( INT_MAX / 1000 ) ); // overflow?
        ( void )m_msigDBScanStop.FWait( -1000 * ( INT )csecToWait );
    }
}

bool DBMScan::FTimeLimitReached_() const
{
    return UtilGetCurrentFileTime() >= m_ftTimeLimit;
}

INT DBMScan::CMSecBeforeTimeLimit_() const
{
    __int64 cmsecBeforeTimeLimit = 1000 * UtilConvertFileTimeToSeconds( m_ftTimeLimit - UtilGetCurrentFileTime() );
    if ( cmsecBeforeTimeLimit > 0 )
    {
        cmsecBeforeTimeLimit = min( INT_MAX, cmsecBeforeTimeLimit );
        Assert( cmsecBeforeTimeLimit <= INT_MAX );    // overflow?
        return ( INT )cmsecBeforeTimeLimit;
    }
    else
    {
        return 0;
    }
}

bool DBMScan::FMaxScansReached_() const
{
    return m_cscansFinished >= m_pscanconfig->CScansMax();
}


//  ================================================================
//  IDBMScanSerializer
//  ================================================================

IDBMScanSerializer::IDBMScanSerializer( const ULONG_PTR ulKey, const IDBMScanSerializerType idbmsType ) :
    m_idbmsType( idbmsType ),
    m_ulKey( ulKey ),
    m_cRef( 0 )
{
    Assert( idbmsType < idbmstypMax );
}

IDBMScanSerializer::~IDBMScanSerializer()
{
}

IDBMScanSerializer::IDBMScanSerializerType IDBMScanSerializer::GetSerializerType() const
{
    return m_idbmsType;
}

ULONG_PTR IDBMScanSerializer::UlSerializerKey() const
{
    return m_ulKey;
}

LONG IDBMScanSerializer::CIncrementSerializerRef()
{
    const LONG cRef = AtomicIncrement( &m_cRef );
    Assert( cRef > 0 );

    return cRef;
}

LONG IDBMScanSerializer::CDecrementSerializerRef()
{
    const LONG cRef = AtomicDecrement( &m_cRef );
    Assert( cRef >= 0 );

    return cRef;
}

LONG IDBMScanSerializer::CSerializerRef() const
{
    return m_cRef;
}


//  ================================================================
//  DBMScanSerializerDummy
//  ================================================================

bool DBMScanSerializerDummy::FEnqueueAndWait( DBMScan * const pdbmScan, const INT cmsec )
{
    Assert( FDBMScanCurrent( NULL ) );
    m_pdbmScan = pdbmScan;
    return true;
}

void DBMScanSerializerDummy::Dequeue( DBMScan * const pdbmScan )
{
    Assert( FDBMScanCurrent( pdbmScan ) );
    m_pdbmScan = NULL;
}

bool DBMScanSerializerDummy::FDBMScanCurrent( DBMScan * const pdbmScan )
{
    return m_pdbmScan == pdbmScan;
}

DWORD DBMScanSerializerDummy::DwTimeSlice() const
{
#ifdef DEBUG
    return 1000;
#else
    return ulMax;
#endif  // DEBUG
}

DBMScanSerializerDummy::DBMScanSerializerDummy( const ULONG_PTR ulKey ) :
    IDBMScanSerializer( ulKey, IDBMScanSerializer::idbmstypDummy ),
    m_pdbmScan( NULL )
{
}

DBMScanSerializerDummy::~DBMScanSerializerDummy()
{
    Assert( FDBMScanCurrent( NULL ) );
}


//  ================================================================
//  DBMScanSerializer
//  ================================================================

bool DBMScanSerializer::FEnqueueAndWait( DBMScan * const pdbmScan, const INT cmsec )
{
    bool fSuccess = false;
    Assert( pdbmScan != NULL );

    //  First, enter the critical section.
    m_critSerializer.Enter();

    //  Are we re-enqueuing? Is the queue empty?
    const BOOL fReEnqueuing = m_ilDbmScans.FMember( pdbmScan );
    const BOOL fEmpty = m_ilDbmScans.FEmpty();
    Assert( !fReEnqueuing );
    Assert( CSerializerRef() > 0 );

    if ( !fReEnqueuing )
    {
        //  Insert ourselves as a waiter. We may be already first at this point.
        m_ilDbmScans.InsertAsNextMost( pdbmScan );
        
        //  If the waiting queue is empty, then we can immediately start.
        if ( fEmpty )
        {
            fSuccess = true;
        }
        else
        {
            //  Try and reset the signal so that we can wait.
            pdbmScan->TryResetScanGo();
            m_critSerializer.Leave();

            //  Wait.
            pdbmScan->WaitForScanGo( cmsec );

            //  Enter the critical section again so that we can evalute if we are first now.
            m_critSerializer.Enter();

            fSuccess = m_ilDbmScans.PrevMost() == pdbmScan;

            //  If we are not the first, remove ourselves from the queue.
            if ( !fSuccess )
            {
                Assert( m_ilDbmScans.FMember( pdbmScan ) );
                m_ilDbmScans.Remove( pdbmScan );
            }
        }
    }
    else
    {
        Assert( !fEmpty );
    }

    //  Leave the critical section.
    m_critSerializer.Leave();

    return fSuccess;
}

void DBMScanSerializer::Dequeue( DBMScan * const pdbmScan )
{
    //  First, enter the critical section.
    m_critSerializer.Enter();

    const BOOL fPresent = m_ilDbmScans.FMember( pdbmScan );
    Assert( fPresent );

    const bool fCurrent = m_ilDbmScans.PrevMost() == pdbmScan;

    if ( fPresent )
    {
        m_ilDbmScans.Remove( pdbmScan );
        if ( fCurrent )
        {
            DBMScan * const pdbmScanNext = m_ilDbmScans.PrevMost();
            Assert( pdbmScan != pdbmScanNext );

            //  If there's someone waiting, we need to signal it.
            if ( pdbmScanNext != NULL )
            {
                pdbmScanNext->TrySetScanGo();
            }
        }
    }
    else
    {
        Assert( !fCurrent );
    }
    
    //  Leave the critical section.
    m_critSerializer.Leave();
}

bool DBMScanSerializer::FDBMScanCurrent( DBMScan * const pdbmScan )
{
    m_critSerializer.Enter();
    const bool fCurrent = m_ilDbmScans.PrevMost() == pdbmScan;
    m_critSerializer.Leave();

    return fCurrent;
}

DWORD DBMScanSerializer::DwTimeSlice() const
{
    return s_cmsecTimeSlice;
}

DBMScanSerializer::DBMScanSerializer( const ULONG_PTR ulKey ) :
    IDBMScanSerializer( ulKey, IDBMScanSerializer::idbmstypReal ),
    m_critSerializer( CLockBasicInfo( CSyncBasicInfo( _T("DBMScanSerializer::m_critSerializer" ) ), rankDBMScanSerializer, 0 ) ),
    m_ilDbmScans()
{
}

DBMScanSerializer::~DBMScanSerializer()
{
    Assert( CSerializerRef() == 0 || FUtilProcessAbort() );
}


//  ================================================================
//  DBMScanSerializerFactory
//  ================================================================

ERR DBMScanSerializerFactory::ErrInitSerializerFactory()
{
    m_critSerializer.Enter();
    Assert( m_ilSerializers.FEmpty() );
    m_ilSerializers.Empty();
    Assert( m_ilSerializers.FEmpty() );
    Assert( m_cDummySerializers == 0 );
    m_cDummySerializers = 0;
    m_critSerializer.Leave();
    
    return JET_errSuccess;
}

void DBMScanSerializerFactory::TermSerializerFactory()
{
    m_critSerializer.Enter();
    Assert( m_ilSerializers.FEmpty() || FUtilProcessAbort() );
    Assert( m_cDummySerializers == 0 || FUtilProcessAbort() );

    //  Iterate and free memory.
    for ( DBMScanSerializer * pdbmSerializer = m_ilSerializers.PrevMost();
            pdbmSerializer != NULL;
            pdbmSerializer = m_ilSerializers.PrevMost() )
    {
        m_ilSerializers.Remove( pdbmSerializer );
        delete pdbmSerializer;
    }

    m_ilSerializers.Empty();
    Assert( m_ilSerializers.FEmpty() );
    m_cDummySerializers = 0;
    m_critSerializer.Leave();
}

ERR DBMScanSerializerFactory::ErrRegisterSerializer_( const ULONG_PTR ulKey, const bool fDummy, IDBMScanSerializer ** const ppidbmSerializer )
{
    ERR err = JET_errSuccess;

    Assert( ppidbmSerializer != NULL );
    *ppidbmSerializer = NULL;
    
    m_critSerializer.Enter();

    DBMScanSerializer * pdbmSerializer = NULL;
    if ( !fDummy )
    {
        //  Key lookup.
        for ( pdbmSerializer = m_ilSerializers.PrevMost();
                pdbmSerializer != NULL;
                pdbmSerializer = m_ilSerializers.Next( pdbmSerializer ) )
        {
            if ( pdbmSerializer->UlSerializerKey() == ulKey )
            {
                break;
            }
        }
    }

    //  Have we found the object?
    if ( pdbmSerializer != NULL )
    {
        Assert( !fDummy );
        //  We just need to increment the ref count.
        const LONG cRefOld = pdbmSerializer->CSerializerRef();
        const LONG cRefNew = pdbmSerializer->CIncrementSerializerRef();
        Assert( cRefOld > 0 );
        Assert( cRefNew == cRefOld + 1 );
        *ppidbmSerializer = pdbmSerializer;
    }
    else
    {
        //  Allocate a new one and insert it into the list.
        IDBMScanSerializer* pidbmSerializerNew = NULL;
        if ( fDummy )
        {
            pidbmSerializerNew = new DBMScanSerializerDummy( ulKey );
        }
        else
        {
            pidbmSerializerNew = new DBMScanSerializer( ulKey );
        }
        Alloc( pidbmSerializerNew );

        const LONG cRefOld = pidbmSerializerNew->CSerializerRef();
        const LONG cRefNew = pidbmSerializerNew->CIncrementSerializerRef();
        Assert( cRefOld == 0 );
        Assert( cRefNew == 1 );
        if ( fDummy )
        {
            m_cDummySerializers++;
            Assert( m_cDummySerializers > 0 );
        }
        else
        {
            m_ilSerializers.InsertAsNextMost( reinterpret_cast<DBMScanSerializer *>( pidbmSerializerNew ) );
            Assert( !m_ilSerializers.FEmpty() );
        }

        *ppidbmSerializer = pidbmSerializerNew;
    }
    
HandleError:

    m_critSerializer.Leave();

    return err;
}

ERR DBMScanSerializerFactory::ErrRegisterRealSerializer( const ULONG_PTR ulKey, IDBMScanSerializer ** const ppidbmSerializer )
{
    ERR err = JET_errSuccess;

    Call( ErrRegisterSerializer_( ulKey, false, ppidbmSerializer ) );
    
HandleError:

    return err;
}

ERR DBMScanSerializerFactory::ErrRegisterDummySerializer( const ULONG_PTR ulKey, IDBMScanSerializer ** const ppidbmSerializer )
{
    ERR err = JET_errSuccess;

    Call( ErrRegisterSerializer_( ulKey, true, ppidbmSerializer ) );
    
HandleError:

    return err;
}

void DBMScanSerializerFactory::UnregisterSerializer( IDBMScanSerializer * const pidbmSerializer )
{
    Assert( pidbmSerializer != NULL );

    const bool fDummy = pidbmSerializer->GetSerializerType() == IDBMScanSerializer::idbmstypDummy;
    
    m_critSerializer.Enter();

    if ( fDummy )
    {
        Assert( m_cDummySerializers > 0 );

        if ( m_cDummySerializers > 0 )
        {
            //  We just need to decrement the ref count.
            const LONG cRefOld = pidbmSerializer->CSerializerRef();
            const LONG cRefNew = pidbmSerializer->CDecrementSerializerRef();
            Assert( cRefOld == 1 );
            Assert( cRefNew == 0 );

            //  We'll delete this unreferenced object.
            if ( cRefNew == 0 )
            {
                m_cDummySerializers--;
                delete pidbmSerializer;
            }
        }
    }
    else
    {
        DBMScanSerializer * const pdbmSerializer = reinterpret_cast<DBMScanSerializer *>( pidbmSerializer );
        
        const bool fPresent = m_ilSerializers.FMember( pdbmSerializer ) == fTrue;

        Assert( fPresent );

        //  Have we found the object?
        if ( fPresent )
        {
            Assert( !m_ilSerializers.FEmpty() );

            //  We just need to decrement the ref count.
            const LONG cRefOld = pdbmSerializer->CSerializerRef();
            const LONG cRefNew = pdbmSerializer->CDecrementSerializerRef();
            Assert( cRefOld > 0 );
            Assert( cRefNew == cRefOld - 1 );

            //  We'll remove and delete this unreferenced object.
            if ( cRefNew == 0 )
            {
                m_ilSerializers.Remove( pdbmSerializer );
                Assert( !m_ilSerializers.FMember( pdbmSerializer ) );
                delete pdbmSerializer;
            }
        }
    }

    m_critSerializer.Leave();
}

bool DBMScanSerializerFactory::FSerializerFactoryEmpty()
{
    m_critSerializer.Enter();
    const bool fEmpty = ( m_ilSerializers.FEmpty() == fTrue ) && ( m_cDummySerializers == 0 );
    m_critSerializer.Leave();

    return fEmpty;
}

DBMScanSerializerFactory::DBMScanSerializerFactory() :
    m_critSerializer( CLockBasicInfo( CSyncBasicInfo( _T("DBMScanSerializerFactory::m_critSerializer" ) ), rankDBMScanSerializerFactory, 0 ) ),
    m_ilSerializers(),
    m_cDummySerializers( 0 )
{
}

DBMScanSerializerFactory::~DBMScanSerializerFactory()
{
    TermSerializerFactory();
}

//  ================================================================
VOID DBMScanStopAllScansForInst( INST * pinst, const BOOL fAllowRestarting )
//  ================================================================
{
    for ( DBID dbid = dbidUserLeast; dbid < dbidMax; dbid++ )
    {
        Assert( dbidTemp != dbid );
        const IFMP ifmp = pinst->m_mpdbidifmp[ dbid ];
        if ( ifmp < g_ifmpMax &&
            g_rgfmp[ifmp].FInUse() )
        {
            //  if it cannot be restarted, set it not to 
            //  start again.
            if ( !fAllowRestarting )
            {
                g_rgfmp[ifmp].SetFDontStartDBM();
            }
            else
            {
                Expected( pinst->m_plog->FRecovering() );
                //  This is tantamount to plog->FRecovering() = fTrue, this means that
                //  we are quiting redo with error (or at end of undo) and so we MUST
                //  stop the follower (to cause it to save the progress state).
                if ( g_rgfmp[ifmp].PdbmFollower() )
                {
                    g_rgfmp[ifmp].PdbmFollower()->DeRegisterFollower( &g_rgfmp[ifmp], CDBMScanFollower::dbmdrrStoppedScan );
                    g_rgfmp[ifmp].DestroyDBMScanFollower();
                }
            }
            
            g_rgfmp[ifmp].StopDBMScan();
        }
    }
}

//  ================================================================
ERR ErrDBMInit()
//  ================================================================
{
    ERR err = JET_errSuccess;

    Call( g_dbmSerializerFactory.ErrInitSerializerFactory() );
    
HandleError:
    return err;
}

//  ================================================================
void DBMTerm()
//  ================================================================
{
    g_dbmSerializerFactory.TermSerializerFactory();
}

//  ================================================================
ERR ErrDBMEmitDivergenceCheck(
    const IFMP ifmp,
    const PGNO pgno,
    const ScanCheckSource scs,
    const CPAGE* const pcpage,
    const ObjidState objidState )
//  ================================================================
{
    Assert( ( scs != scsInvalid ) && ( scs < scsMax ) );
    Expected( ( scs == scsDbScan ) || ( scs == scsDbShrink ) );
    Assert( pgno != pgnoScanLastSentinel );
    Assert( pgno != pgnoMax );
    Assert( pcpage != NULL );
    Assert( objidState != ois::Unknown );

    // In order to ensure we can compare the same version of page, we must have RDW
    // latch so that the page dbtime is serialized for us (and to get a stable checksum as well).
    Assert( pcpage->FAssertRDWLatch() );

    // Note that FMP::FEfvSupported() below checks for both DB and log versions, but JET_efvScanCheck2
    // only upgrades the log version so technically, we wouldn't need to check for the DB version.
    const BOOL fScanCheck2Supported = g_rgfmp[ifmp].FEfvSupported( JET_efvScanCheck2 );
    Assert( fScanCheck2Supported || ( scs == scsDbScan ) );

    const DBTIME dbtimeCurrent = g_rgfmp[ifmp].DbtimeGet();
    const ULONG ulChecksum = fScanCheck2Supported ?
                                UlDBMGetCompressedLoggedChecksum( *pcpage, dbtimeCurrent ) :
                                UsDBMGetCompressedLoggedChecksum( *pcpage, dbtimeCurrent );

    const DBTIME dbtimePage = pcpage->Dbtime();
    const BOOL fEmptyPage   = pcpage->FEmptyPage();
    Assert( ( dbtimePage == 0 && pcpage->ObjidFDP() == 0 ) || /* zero'd page */
            ( pcpage->PgnoThis() == pgno ) /* or the pgno should match */ );
    Assert( ( dbtimePage != dbtimeShrunk ) || ( pcpage->ObjidFDP() == 0 ) );

    LGPOS lgposLogRec = lgposMin;
    const ERR err =  ErrLGScanCheck(
                        ifmp,
                        pgno,
                        scs,
                        dbtimePage,
                        dbtimeCurrent,
                        ulChecksum,
                        objidState == ObjidState::Invalid,
                        fEmptyPage,
                        &lgposLogRec );

    // Check if the persisted dbtime is ahead of the running dbtime.
    if ( ( dbtimePage != 0 ) && ( dbtimePage != dbtimeShrunk )  && ( dbtimePage != dbtimeRevert ) && ( dbtimePage > dbtimeCurrent ) )
    {
        OSTraceSuspendGC();
        const WCHAR* rgwsz[] =
        {
            g_rgfmp[ifmp].WszDatabaseName(),
            OSFormatW( L"%I32u (0x%08x)", pgno, pgno ),
            OSFormatW( L"(%08X,%04X,%04X)", lgposLogRec.lGeneration, lgposLogRec.isec, lgposLogRec.ib ),
            OSFormatW( L"0x%I64x", dbtimePage ),
            OSFormatW( L"0x%I64x", dbtimeCurrent ),
            OSFormatW( L"%hhu", scs ),
            OSFormatW( L"%u", pcpage->ObjidFDP() ),
            OSFormatW( L"%d", (INT)( objidState == ObjidState::Invalid ) )
        };

        UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            DBTIME_CHECK_ACTIVE_CORRUPTED_ID,
            _countof( rgwsz ),
            rgwsz,
            0,
            NULL,
            g_rgfmp[ifmp].Pinst() );

        OSUHAPublishEvent(  HaDbFailureTagCorruption,
                            g_rgfmp[ifmp].Pinst(),
                            HA_DATABASE_CORRUPTION_CATEGORY,
                            HaDbIoErrorNone,
                            g_rgfmp[ifmp].WszDatabaseName(),
                            OffsetOfPgno( pgno ),
                            g_cbPage,
                            HA_DBTIME_CHECK_ACTIVE_CORRUPTED_ID,
                            _countof( rgwsz ),
                            rgwsz );

        OSTraceResumeGC();

        AssertSz( fFalse, "DbScanDbTimeTooHighActive" );
    }

    return err;
}

//  ================================================================
ERR ErrDBMEmitEndScan( const IFMP ifmp )
//  ================================================================
{
    return ErrLGScanCheck(
            ifmp,
            pgnoScanLastSentinel,
            scsDbScan,
            0, // dbtimePage
            0, // dbtimeCurrent
            0, // ulChecksum
            fFalse,     // objidInvalid
            fFalse );   // EmptyPage

}

//  ================================================================
USHORT UsDBMGetCompressedLoggedChecksum( const CPAGE& cpage, const DBTIME dbtimeSeed )
//  ================================================================
{
    const ULONG cRotate = dbtimeSeed & 0x1f;    // 0x3f might be even better?
    QWORD seed = _rotl64( dbtimeSeed, cRotate );
    QWORD qwPreCompressed = cpage.LoggedDataChecksum().rgChecksum[0] ^ seed;

    return USHORT( ( qwPreCompressed >> 48 ) ^ ( qwPreCompressed >> 32 ) ^
                   ( qwPreCompressed >> 16 ) ^ ( qwPreCompressed & 0xffff ) );
}

//  ================================================================
ULONG UlDBMGetCompressedLoggedChecksum( const CPAGE& cpage, const DBTIME dbtimeSeed )
//  ================================================================
{
    const ULONG cRotate = dbtimeSeed & 0x1f;    // 0x3f might be even better?
    QWORD seed = _rotl64( dbtimeSeed, cRotate );
    QWORD qwPreCompressed = cpage.LoggedDataChecksum().rgChecksum[0] ^ seed;

    return ULONG( ( qwPreCompressed >> 32 ) ^ ( qwPreCompressed & 0xffffffff ) );
}

#ifdef ENABLE_JET_UNIT_TEST

//  ================================================================
//  DBMObjectCache tests
//  ================================================================

//  ================================================================
JETUNITTEST( DBMObjectCache, UnknownObjidReturnsUnknown )
//  ================================================================
{
    DBMObjectCache cache;

    CHECK( ois::Unknown == cache.OisGetObjidState( 1 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, SetObjidToValid )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1 );

    CHECK( ois::Valid == cache.OisGetObjidState( 1 ) );
    CHECK( (FUCB*)0x12345678 == cache.PfucbGetCachedObject( 1 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, CloseObjects )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1 );
    cache.CloseAllCachedObjects();

    CHECK( ois::Unknown == cache.OisGetObjidState( 1 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, CloseObjectWithObjid )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1 );
    cache.CacheObjectFucb( (FUCB*)0x12345678 - 1, 2 );
    cache.CacheObjectFucb( (FUCB*)0x12345678 - 2, 3 );

    cache.CloseCachedObjectWithObjid( 1 );
    cache.CloseCachedObjectWithObjid( 3 );

    CHECK( ois::Unknown == cache.OisGetObjidState( 1 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1 ) );

    CHECK( ois::Valid == cache.OisGetObjidState( 2 ) );
    CHECK( ( (FUCB*)0x12345678 - 1 ) == cache.PfucbGetCachedObject( 2 ) );

    CHECK( ois::Unknown == cache.OisGetObjidState( 3 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 3 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, SetObjidToInvalid )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1 );
    cache.CloseAllCachedObjects();
    cache.MarkCacheObjectInvalid( 1 );

    CHECK( ois::Invalid == cache.OisGetObjidState( 1 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, SetObjidToBeDeleted )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1 );
    cache.CloseAllCachedObjects();
    cache.MarkCacheObjectToBeDeleted( 1 );

    CHECK( ois::ToBeDeleted == cache.OisGetObjidState( 1 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, SetObjidValidNoFucb )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1 );
    cache.CloseAllCachedObjects();
    cache.MarkCacheObjectValidNoFUCB( 1 );

    CHECK( ois::ValidNoFUCB == cache.OisGetObjidState( 1 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, SetMultipleObjids )
//  ================================================================
{
    DBMObjectCache cache;
    cache.CacheObjectFucb( (FUCB*)0x12345678 - 1, 1 );
    cache.MarkCacheObjectInvalid( 2 );
    cache.CacheObjectFucb( (FUCB*)0x12345678 - 2, 3 );
    cache.MarkCacheObjectInvalid( 4 );
    cache.MarkCacheObjectToBeDeleted( 5 );
    cache.MarkCacheObjectValidNoFUCB( 6 );

    CHECK( ois::Valid == cache.OisGetObjidState( 1 ) );
    CHECK( (FUCB*)0x12345678 - 1 == cache.PfucbGetCachedObject( 1 ) );
    CHECK( ois::Invalid == cache.OisGetObjidState( 2 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 2 ) );
    CHECK( ois::Valid == cache.OisGetObjidState( 3 ) );
    CHECK( (FUCB*)0x12345678 - 2 == cache.PfucbGetCachedObject( 3 ) );
    CHECK( ois::Invalid == cache.OisGetObjidState( 4 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 4 ) );
    CHECK( ois::ToBeDeleted == cache.OisGetObjidState( 5 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 5 ) );
    CHECK( ois::ValidNoFUCB == cache.OisGetObjidState( 6 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 6 ) );
    CHECK( ois::Unknown == cache.OisGetObjidState( 7 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 7 ) );
}

//  ================================================================
JETUNITTEST( DBMObjectCache, SetTooManyObjids )
//  ================================================================
{
    DBMObjectCache cache;
    for ( INT i = 1; i <= 1000; ++i )
    {
        cache.MarkCacheObjectInvalid( i );
    }
    UtilSleep( 16 );
    cache.MarkCacheObjectInvalid( 1001 );
    cache.CacheObjectFucb( (FUCB*)0x12345678, 1002 );

    CHECK( ois::Invalid == cache.OisGetObjidState( 1001 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1001 ) );
    CHECK( ois::Valid == cache.OisGetObjidState( 1002 ) );
    CHECK( (FUCB*)0x12345678 == cache.PfucbGetCachedObject( 1002 ) );
    CHECK( ois::Unknown == cache.OisGetObjidState( 1003 ) );
    CHECK( pfucbNil == cache.PfucbGetCachedObject( 1003 ) );
}

//  ================================================================
//  DBMScanState tests
//  ================================================================

// test that creating a scan object zeroes out the members
JETUNITTEST( DBMSimpleScanState, ConstructorZeroesMembers )
{
    DBMSimpleScanState state;

    CHECK( 0 == state.FtCurrPassStartTime() );
    CHECK( 0 == state.FtPrevPassStartTime() );
    CHECK( 0 == state.FtPrevPassCompletionTime() );
    CHECK( 0 == state.PgnoContinuousHighWatermark() );
    CHECK( 0 == state.CpgScannedCurrPass() );
    CHECK( 0 == state.CpgScannedPrevPass() );
}

// test that calling StartedPass stores the start time
JETUNITTEST( DBMSimpleScanState, StartedPassRecordsTime )
{
    DBMSimpleScanState state;

    const __int64 ftTimeBefore = UtilGetCurrentFileTime();
    state.StartedPass();
    const __int64 ftTimeAfter = UtilGetCurrentFileTime();

    CHECK( state.FtCurrPassStartTime() >= ftTimeBefore );
    CHECK( state.FtCurrPassStartTime() <= ftTimeAfter );
}

// test that calling ReadPages increments CpgScannedCurrPass
JETUNITTEST( DBMSimpleScanState, ReadPagesIncrementsCpgScanned )
{
    DBMSimpleScanState state;

    const CPG cpgRead = 10;
    
    state.StartedPass();
    state.ReadPages( 1, cpgRead );

    CHECK( cpgRead == state.CpgScannedCurrPass() );
    CHECK( cpgRead == state.PgnoContinuousHighWatermark() );
}

// test that calling FinishedPass stores the completion time
JETUNITTEST( DBMSimpleScanState, FinishedPassRecordsCompletionTime )
{
    DBMSimpleScanState state;
    state.StartedPass();
    
    const __int64 ftTimeBefore = UtilGetCurrentFileTime();
    state.FinishedPass();
    const __int64 ftTimeAfter = UtilGetCurrentFileTime();

    CHECK( state.FtPrevPassCompletionTime() >= ftTimeBefore );
    CHECK( state.FtPrevPassCompletionTime() <= ftTimeAfter );
}

// test that calling FinishedPass stores the old start time
JETUNITTEST( DBMSimpleScanState, FinishedPassRecordsStartTime )
{
    DBMSimpleScanState state;
    state.StartedPass();
    const __int64 ftStartTime = state.FtCurrPassStartTime();
    state.FinishedPass();

    CHECK( state.FtPrevPassStartTime() == ftStartTime );
}

// test that calling FinishedPass stores the number of pages scanned
JETUNITTEST( DBMSimpleScanState, FinishedPassRecordsCpgScanned )
{
    DBMSimpleScanState state;

    const CPG cpgRead = 20;
    
    state.StartedPass();
    state.ReadPages( 1, cpgRead );
    state.FinishedPass();

    CHECK( cpgRead == state.CpgScannedPrevPass() );
}

// test that calling FinishedPass clears the start time
JETUNITTEST( DBMSimpleScanState, FinishedPassClearsStartTime )
{
    DBMSimpleScanState state;

    state.StartedPass();
    state.FinishedPass();

    CHECK( 0 == state.FtCurrPassStartTime() );
}

// test that calling FinishedPass clears CpgScannedCurrPass and
// PgnoContinuousHighWatermark
JETUNITTEST( DBMSimpleScanState, FinishedPassClearsCounters )
{
    DBMSimpleScanState state;

    state.StartedPass();
    state.ReadPages( 1, 10 );
    state.FinishedPass();

    CHECK( 0 == state.CpgScannedCurrPass() );
    CHECK( 0 == state.PgnoContinuousHighWatermark() );
}

JETUNITTEST( DBMSimpleScanState, TestPgnoHighestChecked )
{
    DBMSimpleScanState state;
    
    state.StartedPass();
    state.ReadPages( 1, 75 );

    CHECK( state.PgnoHighestChecked() == 75 );

    state.ReadPages( 5, 1 );
    CHECK( state.PgnoHighestChecked() == 75 );

    state.ReadPages( 75, 1 );
    CHECK( state.PgnoHighestChecked() == 75 );

    state.ReadPages( 76, 1 );
    CHECK( state.PgnoHighestChecked() == 76 );

    state.ReadPages( 80, 1 );
    CHECK( state.PgnoHighestChecked() == 80 );

    state.ReadPages( 70, 21 );
    CHECK( state.PgnoHighestChecked() == 90 );
}


//  ================================================================
//  DBMScanStateHeader tests
//  ================================================================

JETUNITTEST( DBMScanStateHeader, ConstructorLoadsPgnoChecksumCur )
{
    const PGNO pgnoCurr = 99;
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->PdbfilehdrUpdateable()->le_pgnoDbscanHighestContinuous = pgnoCurr;
    
    DBMScanStateHeader state( pfmp );
    CHECK( pgnoCurr == state.PgnoContinuousHighWatermark() );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( DBMScanStateHeader, ConstructorLoadsPgnoChecksumHigh )
{
    const PGNO pgnoCurr = 99;
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->PdbfilehdrUpdateable()->le_pgnoDbscanHighest = pgnoCurr;
    
    DBMScanStateHeader state( pfmp );
    CHECK( ( CPG )pgnoCurr == state.CpgScannedCurrPass() );
    CHECK( pgnoCurr == state.PgnoHighestChecked() );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( DBMScanStateHeader, ConstructorLoadsLogtimeChecksumStart )
{
    const LOGTIME logtime = { 1, 2, 4, 5, 6, 108, fTrue, 0 }; // 04:02.01 June 5th, 2008
    const __int64 filetime = ConvertLogTimeToFileTime( &logtime );
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->PdbfilehdrUpdateable()->logtimeDbscanStart = logtime;
    
    DBMScanStateHeader state( pfmp );
    CHECK( filetime == state.FtCurrPassStartTime() );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( DBMScanStateHeader, ConstructorLoadsLogtimeChecksumPrev )
{
    const LOGTIME logtime = { 0, 4, 11, 13, 3, 96, fTrue, 0 }; // 11:04.00 March 13th, 1996
    const __int64 filetime = ConvertLogTimeToFileTime( &logtime );
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->PdbfilehdrUpdateable()->logtimeDbscanPrev = logtime;
    
    DBMScanStateHeader state( pfmp );
    CHECK( filetime == state.FtPrevPassCompletionTime() );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( DBMScanStateHeader, ReadPagesUpdatesHeader )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    CHECK( 0 == pfmp->Pdbfilehdr()->le_pgnoDbscanHighestContinuous );
    
    DBMScanStateHeader state( pfmp );
    state.StartedPass();
    state.ReadPages( 1, 1000 );
    CHECK( 0 != pfmp->Pdbfilehdr()->le_pgnoDbscanHighestContinuous );
    CHECK( 0 != pfmp->Pdbfilehdr()->le_pgnoDbscanHighest );
    CHECK( 0 != pfmp->Pdbfilehdr()->logtimeDbscanStart.bYear );
    CHECK( 0 == pfmp->Pdbfilehdr()->logtimeDbscanPrev.bYear );
    
    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( DBMScanStateHeader, SuspendUpdatesHeader )
{
    const CPG cpg = 3;
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    
    DBMScanStateHeader state( pfmp );
    state.StartedPass();
    for( INT i = 0; i < cpg; ++i )
    {
        state.ReadPages( i+1, 1 );
    }
    state.SuspendedPass();
    CHECK( cpg == pfmp->Pdbfilehdr()->le_pgnoDbscanHighestContinuous );
    CHECK( cpg == pfmp->Pdbfilehdr()->le_pgnoDbscanHighest );
    CHECK( 0 != pfmp->Pdbfilehdr()->logtimeDbscanStart.bYear );
    CHECK( 0 == pfmp->Pdbfilehdr()->logtimeDbscanPrev.bYear );
    
    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( DBMScanStateHeader,FinishedUpdatesHeader )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    
    DBMScanStateHeader state( pfmp );
    state.StartedPass();
    state.ReadPages( 1, 1 );
    state.FinishedPass();
    CHECK( 0 == pfmp->Pdbfilehdr()->le_pgnoDbscanHighestContinuous );
    CHECK( 0 == pfmp->Pdbfilehdr()->le_pgnoDbscanHighest );
    CHECK( 0 == pfmp->Pdbfilehdr()->logtimeDbscanStart.bYear );
    CHECK( 0 != pfmp->Pdbfilehdr()->logtimeDbscanPrev.bYear );
    
    FMP::FreeMockFMP( pfmp );
}

#endif // ENABLE_JET_UNIT_TEST

//  ================================================================
//  MSysDatabaseScanRecord tests
//  ================================================================

void SetMSysDatabaseScanRecord( MSysDatabaseScanRecord * precord )
{
    precord->p_ftLastUpdateTime     = 0xFF;
    precord->p_ftPassStartTime  = 0xFF;
    precord->p_ftPrevPassStartTime = 0xFF;
    precord->p_ftPrevPassEndTime    = 0xFF;
    precord->p_cpgPrevPassPagesRead = 0xFF;
    precord->p_csecPassElapsed  = 0xFF;
    precord->p_cpgPassPagesRead     = 0xFF;
    precord->p_cInvocationsPass     = 0xFF;
    precord->p_cpgBadChecksumsPass = 0xFF;
    precord->p_cInvocationsTotal    = 0xFF;
    precord->p_cpgBadChecksumsTotal = 0xFF;
    precord->p_cpgLVPages = 0xFF;
    precord->p_cpgRecPages = 0xFF;
    precord->p_cbFreeLVPages = 0xFF;
    precord->p_cbFreeRecPages = 0xFF;
}

JETUNITTEST( MSysDatabaseScanRecord, ConstructorZeroesMembers )
{
    MSysDatabaseScanRecord record;
    CHECK( 0 == record.p_ftLastUpdateTime );
    CHECK( 0 == record.p_ftPassStartTime );
    CHECK( 0 == record.p_ftPrevPassStartTime );
    CHECK( 0 == record.p_ftPrevPassEndTime );
    CHECK( 0 == record.p_cpgPrevPassPagesRead );
    CHECK( 0 == record.p_csecPassElapsed );
    CHECK( 0 == record.p_cpgPassPagesRead );
    CHECK( 0 == record.p_cInvocationsPass );
    CHECK( 0 == record.p_cpgBadChecksumsPass );
    CHECK( 0 == record.p_cInvocationsTotal );
    CHECK( 0 == record.p_cpgBadChecksumsTotal );
    CHECK( 0 == record.p_cpgLVPages );
    CHECK( 0 == record.p_cpgRecPages );
    CHECK( 0 == record.p_cbFreeLVPages );
    CHECK( 0 == record.p_cbFreeRecPages );
}

JETUNITTEST( MSysDatabaseScanRecord, LoadFromEmptyStore )
{
    MSysDatabaseScanRecord record;
    SetMSysDatabaseScanRecord( &record );
    
    MemoryDataStore store;
    CHECK( JET_errSuccess == record.Serializer().ErrLoadBindings( &store ) );
    CHECK( 0 == record.p_ftLastUpdateTime );
    CHECK( 0 == record.p_ftPassStartTime );
    CHECK( 0 == record.p_ftPrevPassStartTime );
    CHECK( 0 == record.p_ftPrevPassEndTime );
    CHECK( 0 == record.p_cpgPrevPassPagesRead );
    CHECK( 0 == record.p_csecPassElapsed );
    CHECK( 0 == record.p_cpgPassPagesRead );
    CHECK( 0 == record.p_cInvocationsPass );
    CHECK( 0 == record.p_cpgBadChecksumsPass );
    CHECK( 0 == record.p_cInvocationsTotal );
    CHECK( 0 == record.p_cpgBadChecksumsTotal );
    CHECK( 0 == record.p_cpgLVPages );
    CHECK( 0 == record.p_cpgRecPages );
    CHECK( 0 == record.p_cbFreeLVPages );
    CHECK( 0 == record.p_cbFreeRecPages );
}

JETUNITTEST( MSysDatabaseScanRecord, LoadAndSave )
{
    MemoryDataStore store;

    MSysDatabaseScanRecord record;
    record.p_ftLastUpdateTime   = 1;
    record.p_ftPassStartTime    = 2;
    record.p_ftPrevPassStartTime = 3;
    record.p_ftPrevPassEndTime  = 4;
    record.p_cpgPrevPassPagesRead = 5;
    record.p_csecPassElapsed    = 6;
    record.p_cpgPassPagesRead   = 7;
    record.p_cInvocationsPass   = 8;
    record.p_cpgBadChecksumsPass = 9;
    record.p_cInvocationsTotal  = 10;
    record.p_cpgBadChecksumsTotal = 11;
    record.p_cpgLVPages = 12;
    record.p_cpgRecPages = 13;
    record.p_cbFreeLVPages = 14;
    record.p_cbFreeRecPages = 15;
    
    CHECK( JET_errSuccess == record.Serializer().ErrSaveBindings( &store ) );
    SetMSysDatabaseScanRecord( &record );

    CHECK( JET_errSuccess == record.Serializer().ErrLoadBindings( &store ) );
    CHECK( 1 == record.p_ftLastUpdateTime );
    CHECK( 2 == record.p_ftPassStartTime );
    CHECK( 3 == record.p_ftPrevPassStartTime );
    CHECK( 4 == record.p_ftPrevPassEndTime );
    CHECK( 5 == record.p_cpgPrevPassPagesRead );
    CHECK( 6 == record.p_csecPassElapsed );
    CHECK( 7 == record.p_cpgPassPagesRead );
    CHECK( 8 == record.p_cInvocationsPass );
    CHECK( 9 == record.p_cpgBadChecksumsPass );
    CHECK( 10 == record.p_cInvocationsTotal );
    CHECK( 11 == record.p_cpgBadChecksumsTotal );
    CHECK( 12 == record.p_cpgLVPages );
    CHECK( 13 == record.p_cpgRecPages );
    CHECK( 14 == record.p_cbFreeLVPages );
    CHECK( 15 == record.p_cbFreeRecPages );
}


#ifdef ENABLE_JET_UNIT_TEST

//  ================================================================
//  DBMScanStateMSysDatabaseScan tests
//  ================================================================

class TestDBMScanStateMSysDatabaseScan : public DBMScanStateMSysDatabaseScan
{
public:
    TestDBMScanStateMSysDatabaseScan( IDataStore * const pstore )
        : DBMScanStateMSysDatabaseScan( ifmpNil, pstore ) {}
    virtual ~TestDBMScanStateMSysDatabaseScan() {}

    const MSysDatabaseScanRecord& Record() const { return m_record; }
    MSysDatabaseScanRecord& Record() { return m_record; }
    void InitStats( CPG cpgLVPages, CPG cpgRecPages, ULONGLONG cbFreeLVPages, ULONGLONG cbFreeRecPages );
};

void TestDBMScanStateMSysDatabaseScan::InitStats( CPG cpgLVPages, CPG cpgRecPages, ULONGLONG cbFreeLVPages, ULONGLONG cbFreeRecPages )
    {
    m_histoCbFreeLVPages.Init( cpgLVPages, cbFreeLVPages );
    m_histoCbFreeRecPages.Init( cpgRecPages, cbFreeRecPages );
    }

JETUNITTEST( DBMScanStateMSysDatabaseScan, FtCurrPassStartTime )
{
    const __int64 ft = 0x2000;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.Record().p_ftPassStartTime = ft;
    CHECK( state.FtCurrPassStartTime() == ft );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, FtPrevPassStartTime )
{
    const __int64 ft = 0x3000;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.Record().p_ftPrevPassStartTime = ft;
    CHECK( state.FtPrevPassStartTime() == ft );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, FtPrevPassCompletionTime )
{
    const __int64 ft = 0x4000;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.Record().p_ftPrevPassEndTime = ft;
    CHECK( state.FtPrevPassCompletionTime() == ft );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, CpgScannedCurrPass )
{
    const CPG cpg = 10;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.Record().p_cpgPassPagesRead = cpg;
    CHECK( state.CpgScannedCurrPass() == cpg );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, PgnoContinuousHighWatermark )
{
    const CPG cpg = 10;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.Record().p_cpgPassPagesRead = cpg;
    CHECK( state.PgnoContinuousHighWatermark() == ( PGNO )cpg );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, CpgScannedPrevPass )
{
    const CPG cpg = 20;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.Record().p_cpgPrevPassPagesRead = cpg;
    CHECK( state.CpgScannedPrevPass() == cpg );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, StartPass )
{
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    const __int64 ftBefore = UtilGetCurrentFileTime();
    state.StartedPass();
    const __int64 ftAfter = UtilGetCurrentFileTime();
    
    CHECK( state.FtCurrPassStartTime() <= ftAfter );
    CHECK( state.FtCurrPassStartTime() >= ftBefore );
    CHECK( 0 == state.Record().p_cpgPassPagesRead );
    CHECK( 1 == state.Record().p_cInvocationsPass );
    CHECK( 0 == state.Record().p_cpgBadChecksumsPass );
    CHECK( 0 == state.CpgScannedCurrPass() );
    CHECK( 1 == state.Record().p_cInvocationsTotal );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, ReadPages )
{
    const CPG cpg = 10;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    for( INT i = 0; i < cpg; ++i )
    {
        state.ReadPages( i+1, 1 );
    }
    
    CHECK( cpg == state.CpgScannedCurrPass() );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, BadChecksumCount )
{
    const CPG cpg = 10;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    for( INT i = 0; i < cpg; ++i )
    {
        const PGNO pgno = 100+i;
        state.BadChecksum( pgno );
    }
    
    CHECK( cpg == state.Record().p_cpgBadChecksumsPass );
    CHECK( cpg == state.Record().p_cpgBadChecksumsTotal );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, ResumeKeepsStartTime )
{
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.StartedPass();
    const __int64 ft = state.FtCurrPassStartTime();
    state.SuspendedPass();
    state.ResumedPass();
    
    CHECK( ft == state.FtCurrPassStartTime() );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, ResumeKeepsCpgScanned )
{
    const CPG cpg = 300;
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.StartedPass();
    state.ReadPages( 1, cpg );
    state.SuspendedPass();
    state.ResumedPass();
    
    CHECK( cpg == state.CpgScannedCurrPass() );
}

JETUNITTEST( DBMScanStateMSysDatabaseScan, ResumeIncrementsInvocations )
{
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.StartedPass();
    state.SuspendedPass();
    state.ResumedPass();
    
    CHECK( 2 == state.Record().p_cInvocationsPass );
    CHECK( 2 == state.Record().p_cInvocationsTotal );
}

// FinishedPass() should clear the current pass state
JETUNITTEST( DBMScanStateMSysDatabaseScan, FinishedClearsState )
{
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.StartedPass();
    state.ReadPages( 1, 500 );
    state.BadChecksum( 40 );
    state.FinishedPass();
    
    CHECK( 0 == state.FtCurrPassStartTime() );
    CHECK( 0 == state.CpgScannedCurrPass() );
    CHECK( 0 == state.Record().p_cpgBadChecksumsPass );
    CHECK( 0 == state.Record().p_cInvocationsPass );
}

// FinishedPass() should not clear global counters
JETUNITTEST( DBMScanStateMSysDatabaseScan, FinishedSavesTotalCounters )
{
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.StartedPass();
    state.BadChecksum( 40 );
    state.SuspendedPass();
    state.ResumedPass();
    state.FinishedPass();
    
    CHECK( 1 == state.Record().p_cpgBadChecksumsTotal );
    CHECK( 2 == state.Record().p_cInvocationsTotal );
}

// FinishedPass() should set the previous pass counters
JETUNITTEST( DBMScanStateMSysDatabaseScan, FinishedSavesPrevPassCounters )
{
    const CPG cpg = 123;
    
    TestDBMScanStateMSysDatabaseScan state( new MemoryDataStore() );
    state.StartedPass();
    const __int64 ftStart = state.FtCurrPassStartTime();
    state.ReadPages( 1, cpg );
    const __int64 ftBefore = UtilGetCurrentFileTime();
    state.FinishedPass();
    const __int64 ftAfter = UtilGetCurrentFileTime();
    
    CHECK( ftStart == state.FtPrevPassStartTime() );
    CHECK( cpg == state.CpgScannedPrevPass() );
    CHECK( state.FtPrevPassCompletionTime() >= ftBefore );
    CHECK( state.FtPrevPassCompletionTime() <= ftAfter );
}

// FinishedPass() should save the state
JETUNITTEST( DBMScanStateMSysDatabaseScan, FinishedSavesState )
{
    // this store is deleted by the destructor of the state
    IDataStore * pstore = new MemoryDataStore();
    TestDBMScanStateMSysDatabaseScan state( pstore );
    state.StartedPass();
    state.ReadPages( 1, 100 );
    state.BadChecksum( 1 );
    const __int64 ft = state.FtCurrPassStartTime();
    const CPG cpgRead = state.CpgScannedCurrPass();
    const CPG cpgChecksum = state.Record().p_cpgBadChecksumsPass;
    const __int64 ftBefore = UtilGetCurrentFileTime();
    state.FinishedPass();
    const __int64 ftAfter = UtilGetCurrentFileTime();

    // force a reload
    SetMSysDatabaseScanRecord( &state.Record() );
    state.Record().Serializer().ErrLoadBindings( pstore );

    CHECK( state.Record().p_ftLastUpdateTime >= ftBefore );
    CHECK( state.Record().p_ftLastUpdateTime <= ftAfter );
    CHECK( ft == state.FtPrevPassStartTime() );
    CHECK( cpgRead == state.CpgScannedPrevPass() );
    CHECK( 0 == state.Record().p_cpgBadChecksumsPass );
    CHECK( cpgChecksum == state.Record().p_cpgBadChecksumsTotal );
    CHECK( 0 == state.Record().p_cInvocationsPass );
    CHECK( 1 == state.Record().p_cInvocationsTotal );
}

// SuspendedPass() should save the state
JETUNITTEST( DBMScanStateMSysDatabaseScan, SuspendedSavesState )
{
    // this store is deleted by the destructor of the state
    IDataStore * pstore = new MemoryDataStore();
    TestDBMScanStateMSysDatabaseScan state( pstore );
    state.StartedPass();
    state.ReadPages( 1, 100 );
    state.BadChecksum( 1 );
    const __int64 ft = state.FtCurrPassStartTime();
    const CPG cpgRead = state.CpgScannedCurrPass();
    const CPG cpgChecksum = state.Record().p_cpgBadChecksumsPass;
    const __int64 ftBefore = UtilGetCurrentFileTime();
    state.SuspendedPass();
    const __int64 ftAfter = UtilGetCurrentFileTime();

    // force a reload
    SetMSysDatabaseScanRecord( &state.Record() );
    state.Record().Serializer().ErrLoadBindings( pstore );
    
    CHECK( state.Record().p_ftLastUpdateTime >= ftBefore );
    CHECK( state.Record().p_ftLastUpdateTime <= ftAfter );
    CHECK( ft == state.FtCurrPassStartTime() );
    CHECK( cpgRead == state.CpgScannedCurrPass() );
    CHECK( cpgChecksum == state.Record().p_cpgBadChecksumsPass );
    CHECK( cpgChecksum == state.Record().p_cpgBadChecksumsTotal );
    CHECK( 1 == state.Record().p_cInvocationsPass );
    CHECK( 1 == state.Record().p_cInvocationsTotal );
}

// Reading a lot of pages should save the state
JETUNITTEST( DBMScanStateMSysDatabaseScan, ReadingPagesSavesState )
{
    // this store is deleted by the destructor of the state
    IDataStore * pstore = new MemoryDataStore();
    TestDBMScanStateMSysDatabaseScan state( pstore );
    state.StartedPass();
    const __int64 ft = state.FtCurrPassStartTime();
    state.ReadPages( 1, 10000 );

    // force a reload
    SetMSysDatabaseScanRecord( &state.Record() );
    state.Record().Serializer().ErrLoadBindings( pstore );

    CHECK( 0 != state.Record().p_ftLastUpdateTime );
    CHECK( ft == state.FtCurrPassStartTime() );
    CHECK( 0 != state.CpgScannedCurrPass() );
    CHECK( 1 == state.Record().p_cInvocationsPass );
    CHECK( 1 == state.Record().p_cInvocationsTotal );
}

// Page level stats
JETUNITTEST( DBMScanStateMSysDatabaseScan, CalculateAverageFreeBytes )
    {
    // this store is deleted by the destructor of the state
    IDataStore * pstore = new MemoryDataStore();
    TestDBMScanStateMSysDatabaseScan state( pstore );
    state.InitStats( 100,  // cpgLV
                     200,  // cpgRec
                     1000, // cbFreeLV
                     500 );// cbFreeRec
    DBMScanStats dbmScanStats = state.DbmScanStatsCurrPass();

    CHECK( 1000 / 100 == dbmScanStats.m_cbAveFreeSpaceLVPages );
    CHECK( 500 / 200 == dbmScanStats.m_cbAveFreeSpaceRecPages );
    CHECK( 100 == dbmScanStats.m_cpgLVPagesScanned );
    CHECK( 200 == dbmScanStats.m_cpgRecPagesScanned );
    }

JETUNITTEST( DBMScanStateMSysDatabaseScan, SinglePageCalculation )
    {
    // this store is deleted by the destructor of the state
    IDataStore * pstore = new MemoryDataStore();
    TestDBMScanStateMSysDatabaseScan state( pstore );
    state.InitStats( 1,  // cpgLV
        1,  // cpgRec
        500, // cbFreeLV
        700 );// cbFreeRec
    DBMScanStats dbmScanStats = state.DbmScanStatsCurrPass();

    CHECK( 500 / 1 == dbmScanStats.m_cbAveFreeSpaceLVPages );
    CHECK( 700 / 1 == dbmScanStats.m_cbAveFreeSpaceRecPages );
    CHECK( 1 == dbmScanStats.m_cpgLVPagesScanned );
    CHECK( 1 == dbmScanStats.m_cpgRecPagesScanned );
    }

//  ================================================================
//  DBMScanConfig tests
//  ================================================================

// This config scans until shutdown
JETUNITTEST( DBMScanConfig, NumberOfScansIsIntMax )
{
    DBMScanConfig config( NULL, NULL );
    CHECK( INT_MAX == config.CScansMax() );
}

// This config scans until shutdown
JETUNITTEST( DBMScanConfig, RuntimeIsIntMax )
{
    DBMScanConfig config( NULL, NULL );
    CHECK( INT_MAX == config.CSecMax() );
}


//  ================================================================
//  DBMSingleScanConfig tests
//  ================================================================

JETUNITTEST( DBMSingleScanConfig, ConstructorSetsMembers )
{
    const INT csecMax           = 100;
    const DWORD dwThrottleSleep = 256;
    DBMSingleScanConfig config( NULL, NULL, csecMax, dwThrottleSleep );

    CHECK( csecMax == config.CSecMax() );
    CHECK( dwThrottleSleep == config.DwThrottleSleep() );
}

// only one scan
JETUNITTEST( DBMSingleScanConfig, NumberOfScansIsOne )
{
    DBMSingleScanConfig config( NULL, NULL, 0, 0 );
    CHECK( 1 == config.CScansMax() );
}

// start a new scan
JETUNITTEST( DBMSingleScanConfig, MinScanTimeIsZero )
{
    DBMSingleScanConfig config( NULL, NULL, 0, 0 );
    CHECK( 0 == config.CSecMinScanTime() );
}


//  ================================================================
//  TestDBMScanConfig
//  ================================================================

class TestDBMScanConfig : public IDBMScanConfig
{
public:
    TestDBMScanConfig();
    virtual ~TestDBMScanConfig() {}
    virtual INT CScansMax() { return m_cscansMax; }
    virtual INT CSecMax() { return m_csecMax; }
    virtual INT     CSecMinScanTime() { return m_csecMinScanTime; }
    virtual CPG     CpgBatch() { return m_cpgBatch; }
    virtual DWORD   DwThrottleSleep() { return m_dwThrottleSleep; }
    virtual bool    FSerializeScan() { return m_fSerializeScan; }
    virtual INST * Pinst() { return NULL; }
    virtual ULONG_PTR UlDiskId() const { return SIZE_MAX; }
    virtual INT     CSecNotifyInterval() { return 60 * 60 * 6; }

    void SetCScansMax( const INT cscans ) { m_cscansMax = cscans; }
    void SetCSecMax( const INT csec ) { m_csecMax = csec; }
    void SetCSecMinScanTime( const ULONG csec ) { m_csecMinScanTime = csec; }

    // DBMScanReader may cap this value at IDBMScanReader::cpgPrereadMax.
    void SetCpgBatch( const CPG cpg ) { m_cpgBatch = cpg; }
    void SetDwThrottleSleep( const DWORD dw ) { m_dwThrottleSleep = dw; }
    void SetFSerializeScan( const bool f ) { m_fSerializeScan = f; }
    
private:
    INT m_cscansMax;
    INT m_csecMax;
    INT m_csecMinScanTime;
    CPG m_cpgBatch;
    DWORD m_dwThrottleSleep;
    bool m_fSerializeScan;
};

TestDBMScanConfig::TestDBMScanConfig() :
    IDBMScanConfig(),
    m_cscansMax( INT_MAX ),
    m_csecMax( INT_MAX ),
    m_csecMinScanTime( 0 ),
    m_cpgBatch( 16 ),
    m_dwThrottleSleep( 0 ),
    m_fSerializeScan( false )
{
}

JETUNITTEST( TestDBMScanConfig, ConstructorSetsMembers )
{
    TestDBMScanConfig config;

    // by default the scan should run forever
    CHECK( INT_MAX == config.CScansMax() );
    CHECK( INT_MAX == config.CSecMax() );

    // by default the scan should run as quickly as possible
    CHECK( 0 == config.CSecMinScanTime() );
    CHECK( 0 == config.DwThrottleSleep() );
    CHECK( false == config.FSerializeScan() );

    // and the scan should read some pages, otherwise no 
    // progress will be made
    CHECK( config.CpgBatch() > 0 );
}

JETUNITTEST( TestDBMScanConfig, SetCScansMax )
{
    const INT cscans = 17;
    TestDBMScanConfig config;
    config.SetCScansMax( cscans );
    CHECK( cscans == config.CScansMax() );
}

JETUNITTEST( TestDBMScanConfig, SetCSecMax )
{
    const INT csec = 16;
    TestDBMScanConfig config;
    config.SetCSecMax( csec );
    CHECK( csec == config.CSecMax() );
}

JETUNITTEST( TestDBMScanConfig, SetCSecMinScanTime )
{
    const INT csec = 17;
    TestDBMScanConfig config;
    config.SetCSecMinScanTime( csec );
    CHECK( csec == config.CSecMinScanTime() );
}

JETUNITTEST( TestDBMScanConfig, SetCpgBatch )
{
    const CPG cpg = 17;
    TestDBMScanConfig config;
    config.SetCpgBatch( cpg );
    CHECK( cpg == config.CpgBatch() );
}

JETUNITTEST( TestDBMScanConfig, SetDwThrottleSleep )
{
    const DWORD dw = 17;
    TestDBMScanConfig config;
    config.SetDwThrottleSleep( dw );
    CHECK( dw == config.DwThrottleSleep() );
}

JETUNITTEST( TestDBMScanConfig, SetFSerializeScan )
{
    const bool f = true;
    TestDBMScanConfig config;
    config.SetFSerializeScan( f );
    CHECK( f == config.FSerializeScan() );
}

    
//  ================================================================
//  DBMScanObserver tests
//  ================================================================

// test class allowing sensing
class TestDBMScanObserver : public DBMScanObserver
{
public:
    TestDBMScanObserver();
    virtual ~TestDBMScanObserver() {}

    bool FStartedPassCalled() const { return m_fStartedPassCalled; }
    bool FResumedPassCalled() const { return m_fResumedPassCalled; }
    bool FPrepareToTermCalled() const { return m_fPrepareToTermCalled; }
    bool FFinishedPassCalled() const { return m_fFinishedPassCalled; }
    bool FSuspendedPassCalled() const { return m_fSuspendedPassCalled; }
    CPG  CpgRead() const { return m_cpgRead; }
    PGNO PgnoBadChecksum() const { return m_pgnoBadChecksum; }

    // waits for FinishedPass_ to be called
    void WaitForFinishedPass() { m_asigFinishedPass.Wait(); }
    
protected:
    virtual void StartedPass_( const IDBMScanState * const ) { m_fStartedPassCalled = true; }
    virtual void ResumedPass_( const IDBMScanState * const ) { m_fResumedPassCalled = true; }
    virtual void FinishedPass_( const IDBMScanState * const );
    virtual void SuspendedPass_( const IDBMScanState * const ) { m_fSuspendedPassCalled = true; }
    virtual void PrepareToTerm_( const IDBMScanState * const ) { m_fPrepareToTermCalled = true; }
    virtual void ReadPage_( const IDBMScanState * const, const PGNO, DBMObjectCache* const pdbmObjectCache ) { m_cpgRead += 1; }
    virtual void BadChecksum_( const IDBMScanState * const, const PGNO pgnoBadChecksum )
    {
        m_pgnoBadChecksum = pgnoBadChecksum;
    }

private:
    bool m_fStartedPassCalled;
    bool m_fResumedPassCalled;
    bool m_fFinishedPassCalled;
    bool m_fSuspendedPassCalled;
    bool m_fPrepareToTermCalled;
    CPG m_cpgRead;
    PGNO m_pgnoBadChecksum;

    CAutoResetSignal m_asigFinishedPass;
};

TestDBMScanObserver::TestDBMScanObserver() :
    DBMScanObserver(),
    m_fStartedPassCalled( false ),
    m_fResumedPassCalled( false ),
    m_fFinishedPassCalled( false ),
    m_fSuspendedPassCalled( false ),
    m_fPrepareToTermCalled( false ),
    m_cpgRead( 0 ),
    m_pgnoBadChecksum( pgnoNull ),
    m_asigFinishedPass( CSyncBasicInfo( _T("TestDBMScanObserver::asigFinishedPass" ) ) )
{
}

void TestDBMScanObserver::FinishedPass_( const IDBMScanState * const )
{
    m_fFinishedPassCalled = true;
    m_asigFinishedPass.Set();
}

// test that all the sensing variables are false
JETUNITTEST( DBMScanObserver, SensingVariablesAreFalseAfterConstruction )
{
    TestDBMScanObserver observer;

    CHECK( false == observer.FStartedPassCalled() );
    CHECK( false == observer.FResumedPassCalled() );
    CHECK( false == observer.FFinishedPassCalled() );
    CHECK( false == observer.FSuspendedPassCalled() );
    CHECK( false == observer.FPrepareToTermCalled() );
    CHECK( 0 == observer.CpgRead() );
    CHECK( pgnoNull == observer.PgnoBadChecksum() );
}

// test that calling StartedPass calls the inherited StartedPass_ method
JETUNITTEST( DBMScanObserver, StartedPass )
{
    TestDBMScanObserver observer;

    observer.StartedPass( NULL );
    CHECK( true == observer.FStartedPassCalled() );
}

// test that calling ResumedPass calls the inherited ResumedPass_ method
JETUNITTEST( DBMScanObserver, ResumedPass )
{
    TestDBMScanObserver observer;

    observer.ResumedPass( NULL );
    CHECK( true == observer.FResumedPassCalled() );
}

// test that calling FinishedPass calls the inherited FinishedPass_ method
JETUNITTEST( DBMScanObserver, FinishedPass )
{
    TestDBMScanObserver observer;
    observer.StartedPass( NULL );

    observer.FinishedPass( NULL );
    CHECK( true == observer.FFinishedPassCalled() );
}

// test that calling SuspendedPass calls the inherited SuspendedPass_ method
JETUNITTEST( DBMScanObserver, SuspendedPass )
{
    TestDBMScanObserver observer;
    observer.StartedPass( NULL );

    observer.SuspendedPass( NULL );
    CHECK( true == observer.FSuspendedPassCalled() );
}

// test that calling PrepareToTerm calls the inherited PrepareToTerm_ method
JETUNITTEST( DBMScanObserver, PrepareToTerm )
{
    TestDBMScanObserver observer;
    observer.StartedPass( NULL );

    observer.PrepareToTerm( NULL );
    CHECK( true == observer.FPrepareToTermCalled() );
}

// test that calling ReadPages calls the inherited ReadPages_ method
JETUNITTEST( DBMScanObserver, ReadPages )
{
    TestDBMScanObserver observer;
    observer.StartedPass( NULL );

    const PGNO pgnoToRead = 12;
    observer.ReadPage( NULL, pgnoToRead, NULL );
    CHECK( 1 == observer.CpgRead() );
}

// test that calling BadChecksum calls the inherited BadChecksum_ method
JETUNITTEST( DBMScanObserver, BadChecksum )
{
    TestDBMScanObserver observer;
    observer.StartedPass( NULL );

    const PGNO pgnoBadChecksum = 19;
    observer.BadChecksum( NULL, pgnoBadChecksum );
    CHECK( pgnoBadChecksum == observer.PgnoBadChecksum() );
}


//  ================================================================
//  TestDBMScanState
//  ================================================================

class TestDBMScanState : public DBMSimpleScanState
{
public:
    TestDBMScanState() : DBMSimpleScanState() {}
    virtual ~TestDBMScanState() {}

    void SetFtCurrPassStartTime( const __int64 ft ) { m_ftCurrPassStartTime = ft; }
    void SetFtPrevPassStartTime( const __int64 ft ) { m_ftPrevPassStartTime = ft; }
    void SetFtPrevPassCompletionTime( const __int64 ft ) { m_ftPrevPassCompletionTime = ft; }
    void SetCpgScannedCurrPass( const CPG cpg ) { m_cpgScannedCurrPass = cpg; }
    void SetPgnoContinuousHighWatermark( const PGNO pgno ) { m_pgnoContinuousHighWatermark = pgno; }
    void SetCpgScannedPrevPass( const CPG cpg ) { m_cpgScannedPrevPass = cpg; }
};

JETUNITTEST( TestDBMScanState, SetFtCurrPassStartTime )
{
    TestDBMScanState state;
    const __int64 ft = 0x100;
    state.SetFtCurrPassStartTime( ft );
    CHECK( ft == state.FtCurrPassStartTime() );
}

JETUNITTEST( TestDBMScanState, SetFtPrevPassStartTime )
{
    TestDBMScanState state;
    const __int64 ft = 0x100;
    state.SetFtPrevPassStartTime( ft );
    CHECK( ft == state.FtPrevPassStartTime() );
}

JETUNITTEST( TestDBMScanState, SetFtPrevPassCompletionTime )
{
    TestDBMScanState state;
    const __int64 ft = 0x100;
    state.SetFtPrevPassCompletionTime( ft );
    CHECK( ft == state.FtPrevPassCompletionTime() );
}

JETUNITTEST( TestDBMScanState, SetCpgScannedCurrPass )
{
    TestDBMScanState state;
    const __int64 ft = 0x100;
    state.SetCpgScannedCurrPass( ft );
    CHECK( ft == state.CpgScannedCurrPass() );
}

JETUNITTEST( TestDBMScanState, SetPgnoHighWatermark )
{
    TestDBMScanState state;
    const __int64 ft = 0x100;
    state.SetPgnoContinuousHighWatermark( ft );
    CHECK( ft == state.PgnoContinuousHighWatermark() );
}

JETUNITTEST( TestDBMScanState, SetCpgScannedPrevPass )
{
    TestDBMScanState state;
    const __int64 ft = 0x100;
    state.SetCpgScannedPrevPass( ft );
    CHECK( ft == state.CpgScannedPrevPass() );
}


//  ================================================================
//  DBMScanObserverCallback tests
//  ================================================================

// namespace to enclose test methods
namespace TestDBMScanObserverCallback {
    
    static bool fCallbackCalled;
    static JET_SESID sesidCallback;
    static JET_DBID dbidCallback;
    static INT csecCallback;
    static CPG cpgCallback;

    void Reset()
    {
        fCallbackCalled     = false;
        sesidCallback       = JET_sesidNil;
        dbidCallback        = JET_dbidNil;
        csecCallback        = 0;
        cpgCallback         = 0;
    }

    JET_ERR TestCallback(
        JET_SESID   sesid,
        JET_DBID    dbid,
        JET_TABLEID tableid,
        JET_CBTYP   cbtyp,
        void *      pvArg1, // pointer to # of seconds
        void *      pvArg2, // pointer to # of pages
        void *      pvContext,
        JET_API_PTR ulUnused )
    {
        Assert( pvArg1 );
        Assert( pvArg2 );

        fCallbackCalled = true;
        sesidCallback = sesid;
        dbidCallback = dbid;
        csecCallback = *( ( INT * )pvArg1 );
        cpgCallback = *( ( CPG * )pvArg2 );
        return JET_errSuccess;
    }
}; // namespace TestDBMScanObserverCallback

// test that Reset() resets all members
JETUNITTEST( DBMScanObserverCallback, ResetSensingVariables )
{
    TestDBMScanObserverCallback::fCallbackCalled = true;
    TestDBMScanObserverCallback::sesidCallback = 0xff;
    TestDBMScanObserverCallback::dbidCallback = 0x1;
    TestDBMScanObserverCallback::csecCallback = 0x1;
    TestDBMScanObserverCallback::cpgCallback = 0x1;
    TestDBMScanObserverCallback::Reset();

    CHECK( false == TestDBMScanObserverCallback::fCallbackCalled );
    CHECK( JET_sesidNil == TestDBMScanObserverCallback::sesidCallback );
    CHECK( JET_dbidNil == TestDBMScanObserverCallback::dbidCallback );
    CHECK( 0 == TestDBMScanObserverCallback::csecCallback );
    CHECK( 0 == TestDBMScanObserverCallback::cpgCallback );
}

// test that calling StartedPass doesn't invoke the callback
JETUNITTEST( DBMScanObserverCallback, StartedPassDoesNotInvokeCallback )
{
    TestDBMScanObserverCallback::Reset();
    TestDBMScanState state;
    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, JET_dbidNil );
    observer.StartedPass( &state );
    
    CHECK( false == TestDBMScanObserverCallback::fCallbackCalled );
}

// test that calling FinishedPass invokes the callback
JETUNITTEST( DBMScanObserverCallback, FinishedPassInvokesCallback )
{
    TestDBMScanObserverCallback::Reset();
    TestDBMScanState state;
    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, JET_dbidNil );
    observer.StartedPass( &state );
    observer.FinishedPass( &state );

    CHECK( true == TestDBMScanObserverCallback::fCallbackCalled );
}

// test that calling SuspendPass doesn't invoke the callback
JETUNITTEST( DBMScanObserverCallback, SuspendedPassDoesNotInvokeCallback )
{
    TestDBMScanObserverCallback::Reset();
    TestDBMScanState state;
    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, JET_dbidNil );
    observer.StartedPass( &state );
    observer.SuspendedPass( &state );

    CHECK( false == TestDBMScanObserverCallback::fCallbackCalled );
}

// test that calling PrepareToTerm doesn't invoke the callback
JETUNITTEST( DBMScanObserverCallback, PrepareToTermDoesNotInvokeCallback )
{
    TestDBMScanObserverCallback::Reset();
    TestDBMScanState state;
    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, JET_dbidNil );
    observer.StartedPass( &state );
    observer.PrepareToTerm( &state );

    CHECK( false == TestDBMScanObserverCallback::fCallbackCalled );
}

// test that the callback passes in the correct sesid
JETUNITTEST( DBMScanObserverCallback, CallbackPassesSesid )
{
    TestDBMScanObserverCallback::Reset();
    const JET_SESID sesid = 0x1;
    TestDBMScanState state;
    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, sesid, JET_dbidNil );
    observer.StartedPass( &state );
    observer.FinishedPass( &state );

    CHECK( sesid == TestDBMScanObserverCallback::sesidCallback );
}

// test that the callback passes in the correct dbid
JETUNITTEST( DBMScanObserverCallback, CallbackPassesDbid )
{
    TestDBMScanObserverCallback::Reset();
    const JET_DBID dbid = 0x2;
    TestDBMScanState state;
    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, dbid );
    observer.StartedPass( &state );
    observer.FinishedPass( &state );

    CHECK( dbid == TestDBMScanObserverCallback::dbidCallback );
}

// test that the callback passes in the elapsed seconds
JETUNITTEST( DBMScanObserverCallback, CallbackPassesElapsedSeconds )
{
    TestDBMScanObserverCallback::Reset();

    const INT csec = 5;
    const __int64 ftStarted = UtilGetCurrentFileTime();
    const __int64 ftCompleted = ftStarted + UtilConvertSecondsToFileTime( csec );

    TestDBMScanState state;
    state.SetFtPrevPassStartTime( ftStarted );
    state.SetFtPrevPassCompletionTime( ftCompleted );

    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, JET_dbidNil );
    observer.StartedPass( &state );
    observer.FinishedPass( &state );

    CHECK( csec == TestDBMScanObserverCallback::csecCallback );
}

// test that the callback passes in the number of pages
JETUNITTEST( DBMScanObserverCallback, CallbackPassesCpgScanned )
{
    TestDBMScanObserverCallback::Reset();

    const INT cpg = 10;
    TestDBMScanState state;
    state.SetCpgScannedPrevPass( cpg );

    DBMScanObserverCallback observer( TestDBMScanObserverCallback::TestCallback, JET_sesidNil, JET_dbidNil );
    observer.StartedPass( &state );
    observer.FinishedPass( &state );

    CHECK( cpg == TestDBMScanObserverCallback::cpgCallback );
}


//  ================================================================
//  TestDBMScanReader
//  ================================================================

class TestDBMScanReader : public IDBMScanReader
{
public:
    TestDBMScanReader( const PGNO pgnoLast = pgnoMax, const PGNO pgnoBadChecksum = pgnoNull );
    
    virtual ~TestDBMScanReader() {}
    ERR InitDBMScanReader() { return JET_errSuccess; }
    void PrereadPages( const PGNO pgnoFirst, const CPG cpg );
    ERR ErrReadPage( const PGNO pgno, PIB* const ppib, DBMObjectCache* const pdbmObjectCache );
    void DoneWithPreread( const PGNO pgno );
    PGNO PgnoLast() const;

    PGNO PgnoLastPreread() const { return m_pgnoLastPreread; }
    CPG CpgLastPreread() const { return m_cpgLastPreread; }
    PGNO PgnoLastRead() const { return m_pgnoLastRead; }

    CPG CpgReadTotal() const { return m_cpgReadTotal; }
    CPG CpgPrereadTotal() const { return m_cpgPrereadTotal; }
    CPG CpgDoneWithPrereadTotal() const { return m_cpgDoneTotal; }
    
    // waits for the first call to ErrReadPage()
    void WaitForFirstPageRead() { m_msigReadPageCalled.Wait(); }

    BOOL FInError() { return m_fInError; }


private:
    // sensing variables
    PGNO    m_pgnoLastPreread;
    CPG     m_cpgLastPreread;
    PGNO    m_pgnoLastRead;
    BOOL    m_fInError;
    CPG     m_cpgReadTotal;
    CPG     m_cpgPrereadTotal;
    CPG     m_cpgDoneTotal;

    // controlling the number of pages and checksum errors
    const PGNO  m_pgnoLast;
    const PGNO  m_pgnoBadChecksum;
    
    CManualResetSignal  m_msigReadPageCalled;


};


TestDBMScanReader::TestDBMScanReader( const PGNO pgnoLast, const PGNO pgnoBadChecksum ) :
    m_pgnoLastPreread( 0 ),
    m_cpgLastPreread( 0 ),
    m_pgnoLastRead( pgnoNull ),
    m_cpgReadTotal( 0 ),
    m_cpgPrereadTotal( 0 ),
    m_cpgDoneTotal( 0 ),
    m_fInError( fFalse ),
    m_pgnoLast( pgnoLast ),
    m_pgnoBadChecksum( pgnoBadChecksum ),
    m_msigReadPageCalled( CSyncBasicInfo( _T("TestDBMScanReader::msigReadPageCalled" ) ) )
{
}

void TestDBMScanReader::PrereadPages( const PGNO pgnoFirst, const CPG cpg )
{
    m_pgnoLastPreread = pgnoFirst;
    m_cpgLastPreread = cpg;
    m_cpgPrereadTotal += cpg;
}

ERR TestDBMScanReader::ErrReadPage( const PGNO pgno, PIB* const ppib, DBMObjectCache* const pdbmObjectCache )
{
    if ( pgno < m_pgnoLastPreread )
    {
        m_fInError = fTrue;
    }
    if ( pgno > m_pgnoLastPreread + m_cpgLastPreread )
    {
        m_fInError = fTrue;
    }
    if ( pgno != m_pgnoLastRead    &&
        pgno != m_pgnoLastPreread &&
        pgno != m_pgnoLastRead + 1 )
    {
        m_fInError = fTrue;
    }

    PGNO pgnoPrevious = m_pgnoLastRead;
    m_pgnoLastRead = pgno;
    m_msigReadPageCalled.Set();

    if ( m_pgnoBadChecksum == pgno )
    {
        return ErrERRCheck( JET_errReadVerifyFailure );
    }

    if ( pgno != pgnoPrevious )
    {
        m_cpgReadTotal++;
    }

    return JET_errSuccess;
}

void TestDBMScanReader::DoneWithPreread( const PGNO pgno )
{
    if ( pgno < m_pgnoLastPreread )
    {
        m_fInError = fTrue;
    }
    if ( pgno > m_pgnoLastPreread + m_cpgLastPreread )
    {
        m_fInError = fTrue;
    }
    if ( pgno != m_pgnoLastRead - m_cpgReadTotal + m_cpgDoneTotal + 1 )
    {
        m_fInError = fTrue;
    }

    m_cpgDoneTotal++;
    return;
}

PGNO TestDBMScanReader::PgnoLast() const
{
    return m_pgnoLast;
}

JETUNITTEST( TestDBMScanReader, InfiniteReadPages )
{
    TestDBMScanReader reader;
    CHECK( JET_errSuccess == reader.ErrReadPage( 1, NULL, NULL ) );
    CHECK( JET_errSuccess == reader.ErrReadPage( 1000, NULL, NULL ) );
    CHECK( JET_errSuccess == reader.ErrReadPage( 1000000000, NULL, NULL ) );
}

JETUNITTEST( TestDBMScanReader, ChecksumError )
{
    PGNO pgno = 700;
    TestDBMScanReader reader( pgno*10, pgno );
    CHECK( JET_errSuccess == reader.ErrReadPage( pgno-1, NULL, NULL ) );
    CHECK( JET_errReadVerifyFailure == reader.ErrReadPage( pgno, NULL, NULL ) );
    CHECK( JET_errSuccess == reader.ErrReadPage( pgno+1, NULL, NULL ) );
}

JETUNITTEST( TestDBMScanReader, VerifyReadsWithoutPrereadsSetsFInError )
{
    TestDBMScanReader reader;
    CHECK( JET_errSuccess == reader.ErrReadPage( 1, NULL, NULL ) );
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyReadsBeforePrereadsSetsFInError )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    CHECK( JET_errSuccess == reader.ErrReadPage( 99, NULL, NULL ) );
    
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyReadsAfterPrereadsSetsFInError )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    CHECK( JET_errSuccess == reader.ErrReadPage( 111, NULL, NULL ) );
    
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyMultipleReadsWithinPrereadIsOk )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );

    CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
    CHECK( reader.FInError() == fFalse );

    CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
    CHECK( reader.FInError() == fFalse );

    CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
    CHECK( reader.FInError() == fFalse );
}

JETUNITTEST( TestDBMScanReader, VerifyOutOfOrderReadsSetsErrorSimple )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );

    CHECK( JET_errSuccess == reader.ErrReadPage( pgno + 1, NULL, NULL ) );
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyOutOfOrderReadsSetsErrorComplex )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
    CHECK( reader.FInError() == fFalse );
    CHECK( JET_errSuccess == reader.ErrReadPage( pgno + 1, NULL, NULL ) );
    CHECK( reader.FInError() == fFalse );
    CHECK( JET_errSuccess == reader.ErrReadPage( pgno + 3, NULL, NULL ) );
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyDoneCalledBelowRangeSetsError )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    reader.DoneWithPreread( pgno - 1 );
    
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyDoneCalledAboveRangeSetsError )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    reader.DoneWithPreread( pgno + 11 );
    
    CHECK( reader.FInError() == fTrue );
}

JETUNITTEST( TestDBMScanReader, VerifyDoneCalledOnNonLastPageSetsError )
{
    PGNO pgno = 100;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
    CHECK( reader.FInError() == fFalse );
    
    reader.DoneWithPreread( pgno + 1 );
    CHECK( reader.FInError() == fTrue );
}


JETUNITTEST( TestDBMScanReader, PrereadTest )
{
    PGNO pgno = 700;
    TestDBMScanReader reader;

    reader.PrereadPages( pgno, 10 );
    CHECK( reader.CpgPrereadTotal() == 10 );
    CHECK( reader.CpgReadTotal() == 0 );

    for ( pgno; pgno < 710; pgno++ )
    {
        CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
        CHECK( reader.FInError() == fFalse );
        CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
        CHECK( reader.FInError() == fFalse );
        CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
        CHECK( reader.FInError() == fFalse );
        CHECK( JET_errSuccess == reader.ErrReadPage( pgno, NULL, NULL ) );
        reader.DoneWithPreread( pgno );
        CHECK( reader.FInError() == fFalse );
    }

    CHECK( reader.CpgPrereadTotal() == 10 );
    CHECK( reader.CpgReadTotal() == 10 );
    CHECK( reader.CpgDoneWithPrereadTotal() == 10 );
    CHECK( reader.FInError() == fFalse );
}


//  ================================================================
//  DBMScan tests
//  ================================================================

// test that starting a scan calls the observers
JETUNITTEST( DBMScan, StartScanCallsObservers )
{
    TestDBMScanReader * preader = new TestDBMScanReader();
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        preader,
        new TestDBMScanConfig(),
        ppibNil ) );
    pscan->AddObserver( new TestDBMScanObserver() );
    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );
    pscan->AddObserver( new TestDBMScanObserver() );

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    preader->WaitForFirstPageRead();
    CHECK( true == pobserver->FStartedPassCalled() );
}

// test that starting a scan updates the state
JETUNITTEST( DBMScan, StartScanUpdatesState )
{
    TestDBMScanReader * preader = new TestDBMScanReader();
    TestDBMScanState * pstate = new TestDBMScanState();
    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate,
        preader,
        new TestDBMScanConfig(),
        ppibNil ) );

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    preader->WaitForFirstPageRead();
    CHECK( 0 != pstate->FtCurrPassStartTime() );
}

// test that a finished scan updates the state
JETUNITTEST( DBMScan, FinishedScanUpdateState )
{
    unique_ptr<TestDBMScanConfig> pconfig( new TestDBMScanConfig() );
    pconfig->SetCScansMax( 1 );
    TestDBMScanState * pstate = new TestDBMScanState();
    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate,
        new TestDBMScanReader( 10 ),
        pconfig.release(),
        ppibNil ) );
    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    pobserver->WaitForFinishedPass();

    CHECK( 0 == pstate->FtCurrPassStartTime() );
    CHECK( 0 != pstate->FtPrevPassStartTime() );
    CHECK( 0 != pstate->FtPrevPassCompletionTime() );
}


// test that starting a scan twice returns an error
JETUNITTEST( DBMScan, StartScanTwiceReturnsError )
{
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        new TestDBMScanReader(),
        new TestDBMScanConfig(),
        ppibNil ) );
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    
    CHECK( JET_errDatabaseAlreadyRunningMaintenance == pscan->ErrStartDBMScan() );
}
    
// test that stopping a scan calls the suspend method of the observers
JETUNITTEST( DBMScan, StopScanCallsSuspend )
{
    TestDBMScanReader * preader = new TestDBMScanReader();
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        preader,
        new TestDBMScanConfig(),
        ppibNil ) );
    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    preader->WaitForFirstPageRead();

    pscan->StopDBMScan();
    CHECK( true == pobserver->FSuspendedPassCalled() );
    CHECK( false == pobserver->FFinishedPassCalled() );
    CHECK( true == pobserver->FPrepareToTermCalled() );
}

// test that suspending a scan updates CpgScannedCurrPass in the state
JETUNITTEST( DBMScan, StopScanUpdatesPgnoLastRead )
{
    TestDBMScanReader * preader = new TestDBMScanReader();
    TestDBMScanState * pstate = new TestDBMScanState();
    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate,
        preader,
        new TestDBMScanConfig(),
        ppibNil ) );
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    preader->WaitForFirstPageRead();

    pscan->StopDBMScan();
    CHECK( pstate->CpgScannedCurrPass() == ( CPG )preader->PgnoLastRead() );
}

// test that scan reads only pages known to exist in the file
JETUNITTEST( DBMScan, ScanDoesNotGoBeyondEof )
{
    const PGNO pgnoLast = 123;
    TestDBMScanReader * preader = new TestDBMScanReader( pgnoLast );
    TestDBMScanState * pstate = new TestDBMScanState();
    TestDBMScanConfig * pconfig = new TestDBMScanConfig();
    pconfig->SetCScansMax( 1 );

    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate,
        preader,
        pconfig,
        ppibNil ) );

    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    pobserver->WaitForFinishedPass();
    CHECK( pgnoLast == ( CPG )preader->PgnoLastRead() );
}

// test that a scan will run repeatedly
JETUNITTEST( DBMScan, ScanRepeats )
{
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        new TestDBMScanReader( 100 ),
        new TestDBMScanConfig(),
        ppibNil ) );
    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );

    // if the scan doesn't repeat this will hang
    for( INT i = 0; i < 3; ++i )
    {
        pobserver->WaitForFinishedPass();
    }
}

// test that a bad checksum is reported to the observers
JETUNITTEST( DBMScan, BadChecksumCallsObservers )
{
    PGNO pgnoBadChecksum = 70;
    TestDBMScanReader * preader = new TestDBMScanReader( 100, pgnoBadChecksum );
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        preader,
        new TestDBMScanConfig(),
        ppibNil ) );
    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    pobserver->WaitForFinishedPass();
    pscan->StopDBMScan();
    CHECK( pgnoBadChecksum == pobserver->PgnoBadChecksum() );
}

// test that a scan can be started after it stops
JETUNITTEST( DBMScan, StartAfterStop )
{
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        new TestDBMScanReader(),
        new TestDBMScanConfig(),
        ppibNil ) );
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    
    pscan->StopDBMScan();
    
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
}

// when starting a new scan, the previous completion
// time should be respected
JETUNITTEST( DBMScan, StartWaitsForScanInterval )
{
    unique_ptr<TestDBMScanConfig> pconfig( new TestDBMScanConfig() );
    pconfig->SetCSecMinScanTime( 3600 );
    
    unique_ptr<TestDBMScanState> pstate( new TestDBMScanState() );
    pstate->SetFtPrevPassCompletionTime( UtilGetCurrentFileTime() );
    
    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate.release(),
        new TestDBMScanReader(),
        pconfig.release(),
        ppibNil ) );

    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );

    // start a scan and stop before the scan interval passes.
    // the scan shouldn't have started
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    UtilSleep( 2 );
    pscan->StopDBMScan();
    
    CHECK( false == pobserver->FResumedPassCalled() );
    CHECK( false == pobserver->FStartedPassCalled() );
}

// test the max scan count is respected
JETUNITTEST( DBMScan, MaxScans )
{
    const INT cscans = 2;
    unique_ptr<TestDBMScanConfig> pconfig( new TestDBMScanConfig() );
    pconfig->SetCScansMax( cscans );
    
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        new TestDBMScanReader( 500 ),
        pconfig.release(),
        ppibNil ) );

    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );
    
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );

    // let the scans complete
    for( INT i = 0; i < cscans; ++i )
    {
        pobserver->WaitForFinishedPass();
    }

    // wait a short while to make sure a new scan doesn't start
    UtilSleep( 2 );

    // if a new scan started then stopping it will suspend the pass 
    pscan->StopDBMScan();
    CHECK( false == pobserver->FSuspendedPassCalled() );
    CHECK( true == pobserver->FPrepareToTermCalled() );
}

// test resuming a scan
JETUNITTEST( DBMScan, ResumeScanCallsObserver )
{
    unique_ptr<TestDBMScanConfig> pconfig( new TestDBMScanConfig() );
    pconfig->SetCScansMax( 1 );
    
    unique_ptr<TestDBMScanState> pstate( new TestDBMScanState() );
    pstate->SetFtCurrPassStartTime( 0x400 );
    pstate->SetCpgScannedCurrPass( 100 );
    
    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate.release(),
        new TestDBMScanReader( 500 ),
        pconfig.release(),
        ppibNil ) );

    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );
    
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    
    pobserver->WaitForFinishedPass();
    CHECK( true == pobserver->FResumedPassCalled() );
    CHECK( false == pobserver->FStartedPassCalled() );
    CHECK( true == pobserver->FFinishedPassCalled() );
    CHECK( false == pobserver->FSuspendedPassCalled() );
    CHECK( false == pobserver->FPrepareToTermCalled() );
}

// make sure that the CpgBatch member of the config is used
JETUNITTEST( DBMScan, CpgBatchIsUsed )
{
    // cpgPrereadMax is limited to 16 on ese.dll/x86.
    const CPG cpgBatch = 15;
    unique_ptr<TestDBMScanConfig> pconfig( new TestDBMScanConfig() );
    pconfig->SetCpgBatch( cpgBatch );

    TestDBMScanReader * preader = new TestDBMScanReader();
    
    unique_ptr<DBMScan> pscan( new DBMScan(
        new TestDBMScanState(),
        preader,
        pconfig.release(),
        ppibNil ) );
    
    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );
    preader->WaitForFirstPageRead();
    CHECK( cpgBatch == preader->CpgLastPreread() );
}

JETUNITTEST( DBMScan, PrereadPagesAreReleased )
{
    const CPG cpgTotal = 20;
    TestDBMScanReader * preader = new TestDBMScanReader( cpgTotal );

    unique_ptr<TestDBMScanConfig> pconfig( new TestDBMScanConfig() );
    pconfig->SetCScansMax( 1 );

    // cpgPrereadMax is limited to 16 on ese.dll/x86.
    const CPG cpgBatch = 15;
    pconfig->SetCpgBatch( cpgBatch );
    pconfig->SetDwThrottleSleep( 1000 );

    C_ASSERT( cpgTotal > cpgBatch );

    TestDBMScanState * pstate = new TestDBMScanState();
    unique_ptr<DBMScan> pscan( new DBMScan(
        pstate,
        preader,
        pconfig.release(),
        ppibNil ) );

    TestDBMScanObserver * pobserver = new TestDBMScanObserver();
    pscan->AddObserver( pobserver );

    // wait for the pass to finish

    CHECK( JET_errSuccess == pscan->ErrStartDBMScan() );

    // check that we pre-read a full batch (cpgBatch pages)
    CHECK( 0 == preader->CpgPrereadTotal() );
    while ( preader->CpgPrereadTotal() == 0 )
    {
        UtilSleep( 1 );
    }
    CHECK( cpgBatch == preader->CpgPrereadTotal() );

    // wait for pass to finish
    pobserver->WaitForFinishedPass();

    // check that we have actually read 'cpgTotal' unique pages 
    CHECK( cpgTotal == preader->CpgPrereadTotal() );
    CHECK( cpgTotal == preader->CpgReadTotal() );

    // check that we called done with preread 'cpgTotal' times.
    CHECK( cpgTotal == preader->CpgDoneWithPrereadTotal() );

    // check that we didn't hit any illegal uses of the TestDBMScanReader
    CHECK( fFalse == preader->FInError() );
    
}

VOID CDBMScanFollower::TestSetup( IDBMScanState * pstate )
{
    m_pstate = pstate;
}

JETUNITTEST( CDBMScanFollower, TestOneCompletePass )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    DBMScanStateHeader state( pfmp );

    //  These 5 checks are assumed not just by this test, but excluded from subsequent tests for brevity
    CHECK( state.FtCurrPassStartTime() == 0 );
    CHECK( state.FtPrevPassCompletionTime() == 0 );
    CHECK( state.CpgScannedPrevPass() == 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 0 );
    CHECK( state.CpgScannedCurrPass() == 0 );
    
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CHECKCALLS( dbscanfollower.ErrRegisterFollower( pfmp, 1 ) );

    // Check again that reading from the header left these counters still at 0
    CHECK( state.PgnoContinuousHighWatermark() == 0 );
    CHECK( state.CpgScannedCurrPass() == 0 );

    for( ULONG pgno = 1; pgno <= 100; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
        CHECK( state.FtCurrPassStartTime() != 0 );
        CHECK( pgno - pfmp->PdbfilehdrUpdateable()->le_pgnoDbscanHighestContinuous < 33 /* the DB hdr update frequencey */ );
    }
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );

    CHECK( state.FtPrevPassCompletionTime() != 0 );
    CHECK( state.CpgScannedPrevPass() == 100 );

    FMP::FreeMockFMP( pfmp );
}

VOID DoHalfPassAndDisable( FMP * const pfmp, const PGNO pgnoHalfSpan )
{
    DBMScanStateHeader state( pfmp );
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CallS( dbscanfollower.ErrRegisterFollower( pfmp, 1 ) );
    for( ULONG pgno = 1; pgno <= pgnoHalfSpan; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
    }
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );

    Assert( state.FtPrevPassCompletionTime() == 0 );
    Assert( state.PgnoContinuousHighWatermark() >= pgnoHalfSpan );
    Assert( state.CpgScannedCurrPass() == ( CPG )pgnoHalfSpan );
}

JETUNITTEST( CDBMScanFollower, TestTwoPhaseCompletePass )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();

    DoHalfPassAndDisable( pfmp, 75 );

    DBMScanStateHeader state( pfmp );   //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CHECK( state.FtPrevPassCompletionTime() == 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 75 );

    //  Pick up where we left off ...
    CHECK( JET_errSuccess == dbscanfollower.ErrRegisterFollower( pfmp, 76 ) );
    for( ULONG pgno = 76; pgno <= 150; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
    }
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );

    //  Check we finished.
    CHECK( state.FtPrevPassCompletionTime() != 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 0 );
    CHECK( state.CpgScannedPrevPass() == 150 );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( CDBMScanFollower, TestTwoPhaseBackStartCompletePass )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();

    DoHalfPassAndDisable( pfmp, 75 );

    DBMScanStateHeader state( pfmp );   //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CHECK( state.FtPrevPassCompletionTime() == 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 75 );

    //  Pick up BACK BEFORE we left off (such as if we crash and re-start recovery from checkpoint) ...
    CHECK( JET_errSuccess == dbscanfollower.ErrRegisterFollower( pfmp, 70 ) );
    for( ULONG pgno = 70; pgno <= 150; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
    }

    CHECK( state.PgnoContinuousHighWatermark() == 150 );
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );

    //  Check we finished.
    CHECK( state.FtPrevPassCompletionTime() != 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 0 );
    CHECK( state.CpgScannedPrevPass() == 150 );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( CDBMScanFollower, TestTwoPhaseBrokenThenCompletePass )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->AcquireIOSizeChangeLatch();
    pfmp->SetFsFileSizeAsyncTarget( 150 * 4096 );
    pfmp->ReleaseIOSizeChangeLatch();
    pfmp->SetOwnedFileSize( pfmp->CbFsFileSizeAsyncTarget() );

    DoHalfPassAndDisable( pfmp, 75 );

    DBMScanStateHeader state( pfmp );   //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CHECK( state.FtPrevPassCompletionTime() == 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 75 );

    //  Pick up after skipping ... bad mojo.
    //  Note these numbers aren't magic, they're picked to be > m_cpgUpdateInterval / 32 pages apart
    CHECK( JET_errSuccess == dbscanfollower.ErrRegisterFollower( pfmp, 115 ) );
    for( ULONG pgno = 115; pgno <= 150; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
    }
    //  Check we made no progress on high watermark, b/c it wasn't continuous
    CHECK( state.PgnoContinuousHighWatermark() == 75 );

    //  Check we made progress on CpgScannedCurrPass, because it just tracks reads
    CHECK( state.CpgScannedCurrPass() == 111 );
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );

    //  Check we DID NOT finish.
    CHECK( state.FtPrevPassCompletionTime() == 0 );
 
    //
    //      Next, do complete passs
    //

    DBMScanStateHeader state2ndPass( pfmp );    //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower2ndPass;
    dbscanfollower2ndPass.TestSetup( &state2ndPass );

    CHECK( state2ndPass.FtPrevPassCompletionTime() == 0 );
    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 75 );

    //  Restart all the way from pgno 1.
    CHECK( JET_errSuccess == dbscanfollower2ndPass.ErrRegisterFollower( pfmp, 1 ) );
    for( ULONG pgno = 1; pgno <= 150; pgno++ )
    {
        dbscanfollower2ndPass.CompletePage( pgno );
    }

    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 150 );
    dbscanfollower2ndPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );

    //  Check we DID finish after all.
    CHECK( state2ndPass.FtPrevPassCompletionTime() != 0 );
    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 0 );
    CHECK( state2ndPass.CpgScannedPrevPass() == 150 );

    //
    //      Next, do broken pass again to get the prev time in the finished broken event.
    //

    DBMScanStateHeader state3rdPass( pfmp );
    CDBMScanFollower dbscanfollower3rdPass;
    dbscanfollower3rdPass.TestSetup( &state3rdPass );
    // We would like to use this next line, but the storage to the disk HDR, and back again is
    // lossy for the FILETIME
    //const __int64 ftPrev = state2ndPass.FtPrevPassCompletionTime();
    const __int64 ftPrev = state3rdPass.FtPrevPassCompletionTime();

    CallS( dbscanfollower3rdPass.ErrRegisterFollower( pfmp, 1 ) );
    for( ULONG pgno = 1; pgno <= 75; pgno++ )
    {
        dbscanfollower3rdPass.CompletePage( pgno );
    }
    CHECK( state3rdPass.PgnoContinuousHighWatermark() == 75 );
    dbscanfollower3rdPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );

    DBMScanStateHeader state4thPass( pfmp );    //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower4thPass;
    dbscanfollower4thPass.TestSetup( &state4thPass );
    CHECK( JET_errSuccess == dbscanfollower4thPass.ErrRegisterFollower( pfmp, 115 ) );
    CHECK( state4thPass.PgnoContinuousHighWatermark() == 75 );
    for( ULONG pgno = 115; pgno <= 150; pgno++ )
    {
        dbscanfollower4thPass.CompletePage( pgno );
    }
    //  Check we made no progress, b/c it wasn't continuous
    CHECK( state4thPass.PgnoContinuousHighWatermark() == 75 );
    dbscanfollower4thPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );
    CHECK( ftPrev == state4thPass.FtPrevPassCompletionTime() );

    FMP::FreeMockFMP( pfmp );
}

VOID DoFullCompletePass( FMP * const pfmp, const PGNO pgnoLast )
{
    DBMScanStateHeader state( pfmp );
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CallS( dbscanfollower.ErrRegisterFollower( pfmp, 1 ) );
    for( ULONG pgno = 1; pgno <= pgnoLast; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
    }
    Assert( state.PgnoContinuousHighWatermark() == pgnoLast );
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );

    Assert( state.FtPrevPassCompletionTime() != 0 );
    Assert( state.CpgScannedPrevPass() == ( CPG )pgnoLast );
}

JETUNITTEST( CDBMScanFollower, TestCompletedTimeStaysBackOnBadPasses )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->AcquireIOSizeChangeLatch();
    pfmp->SetFsFileSizeAsyncTarget( 150 * 4096 );
    pfmp->ReleaseIOSizeChangeLatch();
    pfmp->SetOwnedFileSize( pfmp->CbFsFileSizeAsyncTarget() );

    const ULONG cmsecTimeSep = 50;

    DoFullCompletePass( pfmp, 150 );

    DBMScanStateHeader stateCheckOne( pfmp );   //  re-establish from fresh memory state
    const __int64 ftPrevComplete = stateCheckOne.FtPrevPassCompletionTime();
    CHECK( ftPrevComplete != 0 );
    // We can't check this, b/c this isn't saved to the header!  So it's only in memory 
    // in the DBMScanStateHeader var we used IN DoFullCompletePass() above.  Well we
    // could scan the freed stack above us for a 150 I suppose ... (I'm teasing ;-)
    //CHECK( stateCheckOne.CpgScannedPrevPass() == 150 );

    //  Now test that FT moves forward w/ 2 sec.

    UtilSleep( cmsecTimeSep );

    DoFullCompletePass( pfmp, 150 );
    DBMScanStateHeader stateCheckTwo( pfmp );   //  re-establish from fresh memory state
    const __int64 ftPrevCompleteTwo = stateCheckTwo.FtPrevPassCompletionTime();

    CHECK( ftPrevComplete < ftPrevCompleteTwo );
    
    UtilSleep( cmsecTimeSep );

    //  Do a couple incomplete passes.
    
    DBMScanStateHeader state( pfmp );
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CHECK( JET_errSuccess == dbscanfollower.ErrRegisterFollower( pfmp, 1 ) );
    for( ULONG pgno = 1; pgno <= 85; pgno++ )
    {
        dbscanfollower.CompletePage( pgno );
    }
    CHECK( state.PgnoContinuousHighWatermark() == 85 );
    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );

    DBMScanStateHeader state4thPass( pfmp );    //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower4thPass;
    dbscanfollower4thPass.TestSetup( &state4thPass );
    CHECK( JET_errSuccess == dbscanfollower4thPass.ErrRegisterFollower( pfmp, 1 ) );

    for( ULONG pgno = 1; pgno <= 67; pgno++ )
    {
        dbscanfollower4thPass.CompletePage( pgno );
    }
    //  Check we made no progress, b/c it wasn't continuous
    CHECK( state4thPass.PgnoContinuousHighWatermark() == 85 );
    dbscanfollower4thPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );

    //  Check pass time did not improve.
    DBMScanStateHeader stateInterimNoTimeCheck( pfmp );
    CHECK( stateInterimNoTimeCheck.FtPrevPassCompletionTime() == ftPrevCompleteTwo );

    //  Do a broken/intrupted pass

    DBMScanStateHeader state5thPass( pfmp );    //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower5thPass;
    dbscanfollower5thPass.TestSetup( &state5thPass );
    CHECK( JET_errSuccess == dbscanfollower5thPass.ErrRegisterFollower( pfmp, 1 ) );
    for( ULONG pgno = 1; pgno <= 75; pgno++ )
    {
        dbscanfollower5thPass.CompletePage( pgno );
    }
    //  Check we made no progress, b/c it wasn't continuous
    CHECK( state5thPass.PgnoContinuousHighWatermark() == 85 );
    dbscanfollower5thPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );
    DBMScanStateHeader state6thPass( pfmp );    //  re-establish from fresh memory state
    CDBMScanFollower dbscanfollower6thPass;
    dbscanfollower6thPass.TestSetup( &state6thPass );
    CHECK( JET_errSuccess == dbscanfollower6thPass.ErrRegisterFollower( pfmp, 110 ) );
    for( ULONG pgno = 110; pgno <= 150; pgno++ )
    {
        dbscanfollower6thPass.CompletePage( pgno );
    }
    dbscanfollower6thPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrDisabledScan );
    CHECK( state6thPass.PgnoContinuousHighWatermark() == 85 );

    //  Check pass time did not improve AGAIN.
    DBMScanStateHeader stateFinalNoTimeCheck( pfmp );
    CHECK( stateFinalNoTimeCheck.FtPrevPassCompletionTime() == ftPrevCompleteTwo );

    UtilSleep( cmsecTimeSep );
    DoFullCompletePass( pfmp, 150 );

    DBMScanStateHeader stateTimeMovedCheck( pfmp );
    CHECK( stateTimeMovedCheck.FtPrevPassCompletionTime() != ftPrevCompleteTwo );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( CDBMScanFollower, TestScanCompletesBasedOnHighestContigCompleted )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->AcquireIOSizeChangeLatch();
    pfmp->SetFsFileSizeAsyncTarget( 150 * 4096 );
    pfmp->ReleaseIOSizeChangeLatch();
    pfmp->SetOwnedFileSize( pfmp->CbFsFileSizeAsyncTarget() );

    DoHalfPassAndDisable( pfmp, 100 );

    DBMScanStateHeader state( pfmp );   //  re-establish from fresh memory state
    CHECK( state.FtPrevPassCompletionTime() == 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 100 );

    // Now do a shorter pass, then skip ahead some
    DoHalfPassAndDisable( pfmp, 75 );

    DBMScanStateHeader state2ndPass( pfmp );
    CDBMScanFollower dbscanfollower2ndPass;
    dbscanfollower2ndPass.TestSetup( &state2ndPass );

    CHECK( state2ndPass.FtPrevPassCompletionTime() == 0 );
    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 100 );

    // Now "skip" to a number below the highest contig completed
    // Scan should still resume
    dbscanfollower2ndPass.ErrRegisterFollower( pfmp, 95 );
    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 100 );
    for ( ULONG pgno = 95; pgno <= 150; pgno++ )
    {
        dbscanfollower2ndPass.CompletePage( pgno );
    }

    // Check we made progress, because of the continuous scan
    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 150 );

    // Check we did finish the pass
    dbscanfollower2ndPass.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrFinishedScan );
    CHECK( state2ndPass.FtPrevPassCompletionTime() != 0 );
    CHECK( state2ndPass.PgnoContinuousHighWatermark() == 0 );
    CHECK( state2ndPass.CpgScannedPrevPass() == 150 );

    FMP::FreeMockFMP( pfmp );
}

JETUNITTEST( CDBMScanFollower, TestLateScanEventStartsInMiddle )
{
    FMP * const pfmp = FMP::PfmpCreateMockFMP();
    pfmp->AcquireIOSizeChangeLatch();
    pfmp->SetFsFileSizeAsyncTarget( 150 * 4096 );
    pfmp->ReleaseIOSizeChangeLatch();
    pfmp->SetOwnedFileSize( pfmp->CbFsFileSizeAsyncTarget() );

    DBMScanStateHeader state( pfmp );
    
    CDBMScanFollower dbscanfollower;
    dbscanfollower.TestSetup( &state );

    CHECKCALLS( dbscanfollower.ErrRegisterFollower( pfmp, 25 ) );

    // Check again that reading from the header left these counters still at 0
    CHECK( state.PgnoContinuousHighWatermark() == 0 );
    CHECK( state.CpgScannedCurrPass() == 0 );

    dbscanfollower.CompletePage( 25 );

    dbscanfollower.DeRegisterFollower( pfmp, CDBMScanFollower::dbmdrrStoppedScan );

    // No valid scan was started, check that this is true
    CHECK( state.FtPrevPassCompletionTime() == 0 );
    CHECK( state.PgnoContinuousHighWatermark() == 0 );
    CHECK( state.CpgScannedCurrPass() == 1 );

    FMP::FreeMockFMP( pfmp );
}

#endif // ENABLE_JET_UNIT_TEST

