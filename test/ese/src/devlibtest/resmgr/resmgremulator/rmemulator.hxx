// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RMEMULATOR_HXX_INCLUDED
#define _RMEMULATOR_HXX_INCLUDED

//  Header files.

#ifdef BUILD_ENV_IS_NT
#include <esent_x.h>    //  Needed for ESE errors and types.
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>        //  Needed for ESE errors and types.
#endif
#include <tchar.h>
#include "os.hxx"
#include "bfreqs.hxx"
#include "_bfconst.hxx"
#include "bfftl.hxx"
#include "bfftldriver.hxx"
#include "collection.hxx"
#include "stat.hxx"
using namespace STAT;

#include <set>
#include <map>
#include <list>

//  Definitions.

#define JET_errTestError        -64     //  Generic test error.


//  Data structures.

//  ================================================================
struct PAGE
//  ================================================================
{
public:
    static SIZE_T OffsetOfILE() { return OffsetOf( PAGE, m_ile ); }     //  CInvasiveList glue so these objects can be used in
                                                                        //  PageEvictionEmulator as part of the available page
                                                                        //  list.

private:
    CInvasiveList<PAGE, OffsetOfILE>::CElement m_ile;   //  CInvasiveList glue so these objects can be used in
                                                        //  PageEvictionEmulator as part of the available page
                                                        //  list.

public:
    IFMPPGNO ifmppgno;          //  Key that uniquely identifies the page.

    TICK tickLastFaultSim;      //  Timestamp of when the page was last cached in the simulation.
    TICK tickLastRequestSim;    //  Timestamp of when the page was last requested (cached or touched) in the simulation.
};

//  ================================================================
struct LGPOS    // Originally declared in daedef.hxx (not an exact copy).
//  ================================================================
{
public:
    USHORT ib;      //  Offset withing sector.

    USHORT isec;    //  Sector number.

    ULONG lgen;     //  Log generation number.

    LGPOS();
    LGPOS( const ULONG lgenT, const USHORT isecT, const USHORT ibT );

    INT FIsSet() const;                         //  Whether or not this is considered a valid/set LGPOS.
};

INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 );   //  Compares two LGPOS elements.
                                                            //  Returns < 0 if lgpos1 is older, == 0 if they are the same or >= 0  if lgpos1 is more recent.

const LGPOS lgposMin( 0, 0, 0 );
const LGPOS lgposMax( 0x7FFFFFFFU, (USHORT)0xFFFFU, (USHORT)0xFFFFU );

//  ================================================================
struct DirtyPageOp
//  ================================================================
{
    LGPOS lgpos;    //  Log position of the operation.

    PGNO pgno;      //  Page number.
};

//  ================================================================
struct PAGEENTRY
//  ================================================================
{
public:
    PAGE* ppage;                //  Pointer to the page (NULL if the page is not currently cached).

    bool fDirty;                //  Whether or not the page is currently dirty in the simulation.

    LGPOS lgposCleanLast;       //  Log position the page was last cleaned at.

    ULONG cRequests;            //  Number of times the page was cached/touched (i.e., requested) in the trace.

    ULONG cDirties;             //  Number of times the page was dirtied in the trace.

    ULONG cWritesReal;          //  Number of times the page was written out in the trace.

    ULONG cWritesSim;           //  Number of times the page was written out in the simulation.

    ULONG cFaultsReal;          //  Number of times the page was faulted-in in the trace. 

    ULONG cFaultsSim;           //  Number of times the page was faulted-in in the simulation.

    TICK tickLastCachedReal;    //  Timestamp of when the page was last cached in the trace. 

    TICK dtickCachedReal;       //  Amount of time the page remained cached in the trace.

    TICK dtickCachedSim;        //  Amount of time the page remained cached in the simulation.

    //  Ctor.

    PAGEENTRY ();
};


//  ================================================================
enum PageEvictionContextType
//  ================================================================
{
    pectypPAGE,         //  A pointer to PAGE.
    pectypIFMPPGNO      //  An IFMPPGNO struct.
};


//  ================================================================
class IPageEvictionAlgorithmImplementation
//  ================================================================
{
protected:
    //  Ctor.

    IPageEvictionAlgorithmImplementation();

public:
    // Dtor.
    
    virtual ~IPageEvictionAlgorithmImplementation();

    //  Init./term.

    virtual ERR ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit ) = 0;
    virtual void Term( const BFTRACE::BFSysResMgrTerm_& bfterm ) = 0;

    //  Cache/touch/evict.

    virtual ERR ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache ) = 0;
    virtual ERR ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch ) = 0;
    virtual ERR ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold ) = 0;
    virtual ERR ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict ) = 0;
    virtual ERR ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp ) = 0;
    virtual ERR ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp ) = 0;

    //  Non-required funtions.

    virtual bool FNeedsPreProcessing() const;
    virtual ERR ErrStartProcessing();
    virtual ERR ErrResetProcessing();

    //  Helpers.

    virtual size_t CbPageContext() const = 0;           //  Returns the resource size (must be always >= PAGE and must countain PAGE
                                                        //  in the offset 0 of the data structure).

    virtual void InitPage( PAGE* const ppage ) = 0;     //  Initializes a page that is about to be used.

    virtual void DestroyPage( PAGE* const ppage ) = 0;  //  Destroys a page that is about to be evicted.
};


//  ================================================================
class PageEvictionEmulator
//  ================================================================
{
public:
    //  Possible cache sizing policies.

    enum PageEvictionEmulatorCacheSizePolicy
    {
        peecspFixed,        //  Fixed cache size.
        peecspVariable,     //  Variable cache size, follows eviciton traces.
        peecspAgeBased,     //  Keeps only resources newer than a certain age (last reference time).
        peecspMax,          //  Sentinel.
    };

private:
    //  Possible states.

    enum PageEvictionEmulatorState
    {
        peesUninitialized,      //  Just instantiated or terminated.
        peesInitialized,        //  Post successful ErrInit().
        peesRunning,            //  Undergoing ErrExecute().
        peesDone                //  Post successful or unsuccessful ErrExecute(), ready
                                //  to collect statistics.
    };

public:
    //  Stats.

    struct STATS
    {
        //  Data.

        CPG cpgCached;                                      //  Number of pages currently cached.

        CPG cpgCachedMax;                                   //  Peak number of pages cached.

        QWORD cRequestedUnique;                             //  Number of unique resources/pages requested during the simulation.

        QWORD cRequested;                                   //  Number of resources/pages requested during the simulation.

        QWORD cFaultsReal;                                  //  Number of times pages were faulted-in in the trace.

        QWORD cFaultsSim;                                   //  Number of times pages were faulted-in in the simulation.

        QWORD cFaultsRealAvoidable;                         //  Number of times pages were faulted-in in the trace and could be avoided if cache was larger or a better eviction policy was used.

        QWORD cFaultsSimAvoidable;                          //  Number of times pages were faulted-in in the simulation and could be avoided if cache was larger or a better eviction policy was used.

        QWORD cCaches;                                      //  Number of times pages were cached in the trace.

        QWORD cCachesNew;                                   //  Number of times pages were cached in the trace with the NewPage flag.

        QWORD cCachesNewAlreadyCached;                      //  Number of times pages were cached in the trace with the NewPage flag and the page was already cached.

        QWORD cCachesTurnedTouch;                           //  Number of times pages were cached in the trace, but touched in the simulation.

        QWORD cCachesDbScan;                                //  Number of times pages were cached in the trace with the DBM flag.

        QWORD cTouchesReal;                                 //  Number of times pages were touched in the trace (actual touches, i.e., new pages don't count).

        QWORD cTouchesSim;                                  //  Number of times pages were touched in the simulation.

        QWORD cTouches;                                     //  Number of times pages were touched in the trace.

        QWORD cTouchesNew;                                  //  Number of times pages were touched in the trace with the NewPage flag.

        QWORD cTouchesNoTouch;                              //  Number of times pages were touched (techincally, requested) in the trace with the no-touch flag.

        QWORD cTouchesTurnedCache;                          //  Number of times pages were touched in the trace, but cached in the simulation.

        QWORD cTouchesNoTouchTurnedCache;                   //  Number of times pages were touched in the trace with the no-touch flag, but were cached in the simulation because the page wasn't present.

        QWORD cTouchesNoTouchDropped;                       //  Number of times pages were touched in the trace with the no-touch flag and were discarded during the simulation.

        QWORD cTouchesDbScan;                               //  Number of times pages were touched in the trace with the DBM flag.

        QWORD cDirties;                                     //  Number of times pages were dirtied.

        QWORD cDirtiesUnique;                               //  Number of unique resources/pages dirtied during the simulation.

        QWORD cWritesReal;                                  //  Number of times pages were written-out in the trace.

        QWORD cWritesRealChkpt;                             //  Number of times pages were written-out in the trace due to checkpoint advancement.

        QWORD cWritesRealAvailPool;                         //  Number of times pages were written-out in the trace due to avail pool maintenance

        QWORD cWritesRealShrink;                            //  Number of times pages were written-out in the trace due to cache shrinkage.

        QWORD cWritesSim;                                   //  Number of times pages were written-out in the simulation.

        QWORD cWritesSimChkpt;                              //  Number of times pages were written-out in the simulation due to checkpoint advancement.

        QWORD cWritesSimAvailPool;                          //  Number of times pages were written-out in the simulation due to avail pool maintenance.

        QWORD cWritesSimShrink;                             //  Number of times pages were written-out in the simulation due to cache shrinkage.

        QWORD cEvictionsReal;                               //  Number of times pages were evicted in the trace.

        QWORD cEvictionsSim;                                //  Number of times pages were evicted in the simulation.

        QWORD cEvictionsCacheTooBig;                        //  Number of times pages were evicted because we needed to make room for other pages (fixed-cache-size mode only).

        QWORD cEvictionsCacheTooOld;                        //  Number of times pages were evicted because we needed to make room for newer pages (age-based-cache-size mode only).

        QWORD cEvictionsPurge;                              //  Number of times pages were evicted by purging them.

        QWORD cSuperColdedReal;                             //  Number of times pages were super-colded in the trace.

        QWORD cSuperColdedSim;                              //  Number of times pages were super-colded in the simulation.

        DWORD lgenMin;                                      //  Minimum log generation seen during the simulation.

        DWORD lgenMax;                                      //  Maximum log generation seen during the simulation.

        double pctCacheFaultRateReal;                       //  Cache fault rate in the trace.

        double pctCacheSizeRatioSim;                        //  Ratio of cache size, compared to data set (unique pages) in the simulation.

        double pctCacheFaultRateSim;                        //  Cache fault rate in the simulation.

        double pctCacheFaultRateRealAvoidable;              //  Cache fault rate in the trace and could be avoided if cache was larger or a better eviction policy was used.

        double pctCacheFaultRateSimAvoidable;               //  Cache fault rate in the simulation and could be avoided if cache was larger or a better eviction policy was used.

        CPerfectHistogramStats histoFaultsReal;             //  Histogram of number of caches in the trace.

        CPerfectHistogramStats histoFaultsSim;              //  Histogram of number of caches in the simulation.

        CPerfectHistogramStats histoLifetimeReal;           //  Histogram of page lifetime in the trace.

        CPerfectHistogramStats histoLifetimeSim;            //  Histogram of page lifetime in the simulation.

        STATS& operator=( const STATS& stats )
        {
            if ( this != &stats )
            {
                *this = stats;
            }

            return *this;
        }
    };


    //  Stats (aggregated).

    struct STATS_AGG : public STATS
    {
        DWORD cResMgrCycles;            //  Number of resource manager cycles (init/term, includes abrupt cycles).

        DWORD cResMgrAbruptCycles;      //  Number of resource manager abrupt cycles (init with no term).

        QWORD cDiscardedTraces;         //  Number of discarded trace records (don't understand or are not interested in processing, but within
                                        //  valid cycles).

        QWORD cOutOfRangeTraces;        //  Number of trace records that fell outside of init/term calls.

        QWORD cEvictionsFailed;         //  Number of times pages could not get evicted in the simulation.

        TICK dtickDurationReal;         //  Duration of the trace.

        TICK dtickDurationSimRun;       //  Duration the actual simulation.
    };

private:
    
    //  Ctor.

    PageEvictionEmulator();

public:

    //  Dtor.

    ~PageEvictionEmulator();
    
    //  Init./term.

    ERR ErrInit( BFFTLContext* const pbfftlContext,                                     //  See m_pbfftlContext below.
                    IPageEvictionAlgorithmImplementation* const pipeaImplementation );  //  See m_pipeaImplementation below.
    void Term();

    //  Configuration.

    ERR ErrSetReplaySuperCold( const bool fReplaySuperCold );                       //  See m_fReplaySuperCold below, default is true.
    ERR ErrSetReplayNoTouch( const bool fReplayNoTouch );                           //  See m_fReplayNoTouch below, default is true.
    ERR ErrSetReplayDbScan( const bool fReplayDbScan );                             //  See m_fReplayDbScan below, default is true.
    ERR ErrSetReplayCachePriority( const bool fReplayCachePriority );               //  See m_fReplayCachePriority below, default is true.
    ERR ErrSetEvictNextOnShrink( const bool fEvictNextOnShrink );                   //  See m_fEvictNextOnShrink below, default is true.
    ERR ErrSetReplayInitTerm( const bool fReplayInitTerm );                         //  See m_fReplayInitTerm below, default is true.
    ERR ErrSetInitOverrides( const BFTRACE::BFSysResMgrInit_* const pbfinit );      //  See m_bfinit below, default is not to override.
    ERR ErrSetCacheSize( const PageEvictionEmulatorCacheSizePolicy peecsp,
                         const DWORD dwParam = 0 );                                 //  Cache sizing policy and extra parameter (peecspFixed only).
    ERR ErrSetCheckpointDepth( const DWORD clogsChkptDepth );                       //  See m_clogsChkptDepth below.
    ERR ErrSetPrintSampleInterval( const TICK dtickPrintSampleInterval );           //  See m_cFaultsHistoRes below, default is 0 (do not print samples).
    void SetPrintHistograms( const bool fPrintHistograms );                         //  Whether or not to print histograms.
    ERR ErrSetFaultsHistoRes( const ULONG cFaultsHistoRes );                        //  See m_cFaultsHistoRes below, default is 1.
    ERR ErrSetLifetimeHistoRes( const TICK dtickLifetimeHistoRes );                 //  See m_dtickLifetimeHistoRes below, default is 1.

    //  Runs through a BFFTL trace.

    ERR ErrExecute();

    //  Accessors.

    static PageEvictionEmulator& GetEmulatorObj();

    const PageEvictionEmulator::STATS_AGG& GetStats();

    const PageEvictionEmulator::STATS& GetStats( const size_t ifmp );

    //  Helpers.

    static void ResetInitOverrides( BFTRACE::BFSysResMgrInit_* const pbfinit );     //  Resets overrides to their default state.
    ERR ErrDumpStats();
    ERR ErrDumpStats( const bool fDumpHistogramDetails );

    //  Allows for hooking of the time retrieval function in certain algorithms.

    static TICK __stdcall DwGetTickCount();

    static void ResetGlobalTick();

    static void SetGlobalTick( const TICK tick, const bool fTimeMayGoBack );

private:
    //  Static members.

    static PageEvictionEmulator s_singleton;                        //  Singleton.

    static TICK s_tickCurrent;                                      //  Current tick tracked in the simulation.

    static TICK s_tickLast;                                         //  Last tick seen in the trace.

    static TICK s_dtickAdj;                                         //  Tick adjustment, in case of it going backwards during the simulation.
    
    //  Private members.

    PageEvictionEmulatorState m_state;                              //  Current state of the emulator.

    bool m_fResMgrInit;                                             //  Current state of the resource manager.

    CArray<BYTE*> m_arrayPages;                                     //  Array of page chunks (each element represents a page chunk of cpgChunk pages).

    CInvasiveList<PAGE, PAGE::OffsetOfILE> m_ilAvailPages;          //  List of available pages.

    CArray<CArray<PAGEENTRY>> m_arrayPageEntries;                   //  Array of page entry arrays (each element represents an IFMP).

    CArray<std::list<DirtyPageOp>*> m_arrayDirtyPageOps;            //  Array of dirty-page operation lists (each element represents an IFMP).

    PageEvictionEmulator::STATS_AGG m_statsAgg;                     //  Statistics (aggregated for all IFMPs).

    CArray<PageEvictionEmulator::STATS> m_stats;                    //  Statistics (each element represents an IFMP).

    BFFTLContext* m_pbfftlContext;                                  //  BF tracer handle.

    IPageEvictionAlgorithmImplementation* m_pipeaImplementation;    //  Page eviction algorithm.

    bool m_fReplaySuperCold;                                        //  If we should replay super-cold traces, in case the algorithm supports.

    bool m_fReplayNoTouch;                                          //  If we should replay page requests that didn't generate a touch.

    bool m_fReplayDbScan;                                           //  If we should replay page requests coming from DBM (Database Maintenance).

    bool m_fReplayCachePriority;                                    //  If we should replay with the traced cached priority or use the default.

    bool m_fEvictNextOnShrink;                                      //  In case of a page eviction due to cache shrinkage, if we should evict
                                                                    //  the next targeted page or the original / explicitly specified page.

    bool m_fReplayInitTerm;                                         //  If we should replay init and term ResMgr traces.

    BFTRACE::BFSysResMgrInit_ m_bfinit;                             //  Override initialization parameters.

    PageEvictionEmulatorCacheSizePolicy m_peecsp;                   //  Cache sizing policy.

    DWORD m_cbfCacheSize;                                           //  Fixed cache size to use during simulations, if the cache sizing policy is peecspFixed.

    TICK m_dtickCacheAge;                                           //  Maximum age of resources to keep in the cache during simulations, if the cache sizing policy is peecspAgeBased.

    DWORD m_clogsChkptDepth;                                        //  Fixed checkpoint depth in logs to use during simulations.

    TICK m_dtickPrintSampleInterval;                                //  Interval to print simulation samples at.

    bool m_fPrintHistograms;                                        //  Whether or not to print histograms.

    ULONG m_cFaultsHistoRes;                                        //  Resolution of the cFaults histogram.

    TICK m_dtickLifetimeHistoRes;                                   //  Resolution of the dtickLifetime histogram.

    size_t m_cbPage;                                                //  Actual size of the pages in the m_arrayPages array.

    size_t m_cpgChunk;                                              //  Actual page count used to grow the pool of pages in m_arrayPages.

    bool m_fSetLgposModifySupported;                                //  Whether or not the traces being processed support SetLgposModify.

    //  Helpers.

    void ResetConfig();                                                     //  Resets the emulator configuration.

    void ResetInitOverrides();                                              //  Resets m_bfinit to its default value.

    PAGE* PpgGetAvailablePage_();                                           //  Returns a pointer to an availble page.

    void FreePage_( PAGE* const ppg );                                      //  Frees a page to the list of available pages.

    void PurgeCache_();                                                     //  Evicts all the pages that are currently cached in the simulation.

    PAGEENTRY* PpgeGetEntry_( const IFMPPGNO& ifmppgno ) const;             //  Returns a pointer to a page entry.

    PAGEENTRY* PpgeGetEntry_( const void* const pv, const PageEvictionContextType pectyp ) const;   //  Returns a pointer to a page entry.

    void ModifyPage_( const IFMPPGNO& ifmppgno, const LGPOS& lgposModify );                     //  Modifies a page, either based on a dirty-page or a set-lgpos-modify trace.

    void InsertDirtyPageOp_( const IFMP ifmp, const PGNO pgno, const LGPOS& lgpos );            //  Inserts a dirty-page operation in the corresponding list.

    void RemoveOldestDirtyPageOp_( const IFMP ifmp, PGNO* const ppgno, LGPOS* const plgpos );   //  Removes a dirty-page operation from the front of the list (oldest).

    void GetOldestDirtyLgpos_( const IFMP ifmp, LGPOS* const plgpos ) const;    //  Returns the oldest log position from dirty operations for that IFMP.

    void GetNewestDirtyLgpos_( const IFMP ifmp, LGPOS* const plgpos ) const;    //  Returns the most recent log position from dirty operations for that IFMP.

    static void DumpHistogram_( CPerfectHistogramStats& histogram,          //  Prints a histogram.
                                const SAMPLE sampleMin,
                                const SAMPLE sampleMax,
                                const SAMPLE sampleRes,
                                const bool fDumpDetails );

    //  These functions update the state of the pages in the internal emulator
    //  data structures and accumulate statistics. They do not replay the traces.

    ERR ErrCachePage_( PAGEENTRY* const ppge, const IFMPPGNO& ifmppgno, const bool fNewPage );          //  Caches a page, allocating memory if necessary.

    void TouchPage_( PAGEENTRY* const ppge, const BFTRACE::BFTouch_& bftouch );                         //  Touches a page in our cache.

    void DirtyPage_( PAGEENTRY* const ppge );                                                           //  Dirties a page in our cache.

    void WritePage_( PAGEENTRY* const ppge, const IOREASONPRIMARY iorp, const LGPOS& lgposCurrent ) ;   //  Writes a page from our cache.

    void CleanPage_( PAGEENTRY* const ppge, const LGPOS& lgposClean );                                  //  Cleans a page in our cache.

    void SupercoldPage_( PAGEENTRY* const ppge );                                                       //  Supercolds a page in our cache.

    void EvictPage_( PAGEENTRY* const ppge, const IOREASONPRIMARY iorp, const LGPOS& lgposCurrent );    //  Evicts a page from our cache.

    //  These functions process and replay each of the trace types.

    ERR ErrProcessTraceInit_( const BFTRACE& bftrace );             //  Processes a bftidSysResMgrInit trace record.

    ERR ErrFakeProcessTraceInit_();                                 //  Fakes the occurence of a bftidSysResMgrInit trace record.

    void ProcessTraceTerm_( const BFTRACE& bftrace );               //  Processes a bftidSysResMgrTerm trace record.

    void FakeProcessTraceTerm_();                                   //  Fakes the occurence of a bftidSysResMgrTerm trace record.

    ERR ErrProcessTraceCache_( BFTRACE& bftrace );                  //  Processes a bftidCache trace record.

    ERR ErrProcessTraceTouch_( BFTRACE& bftrace );                  //  Processes a bftidTouch trace record.

    ERR ErrProcessTraceSuperCold_( const BFTRACE& bftrace );        //  Processes a bftidSuperCold trace record.

    ERR ErrProcessTraceEvict_( BFTRACE& bftrace );                  //  Processes a bftidEvict trace record.

    ERR ErrProcessTraceDirty_( const BFTRACE& bftrace );            //  Processes a bftidDirty trace record.

    ERR ErrProcessTraceWrite_( const BFTRACE& bftrace );            //  Processes a bftidWrite trace record.

    ERR ErrProcessTraceSetLgposModify_( const BFTRACE& bftrace );   //  Processes a bftidSetLgposModify trace record.

    ERR ErrProcessTraceDefault_( const BFTRACE& bftrace );          //  Processes any unrecognized trace records for statistical purposes.

    void MaintainAgeBasedCache_();                                  //  Maintains the cache size if we are running with an age-based policy.
};


//  ================================================================
class SimulationIterator
//  ================================================================
{
protected:
    //  Types and objects.

    typedef std::map<DWORD, QWORD> SimulationSamples;
    typedef std::pair<DWORD, QWORD> SimulationSample;
    typedef SimulationSamples::const_iterator SimulationSamplesIter;
    SimulationSamples m_samples;

public:
    //  Public API.

    virtual DWORD DwGetNextIterationValue() = 0;                            //  Returns the next iteration value. "-1" (ulMax) means the end of the iteration.

    bool FSimulationSampleExists( const DWORD dwIterationValue ) const;         //  Returns whether or not the sample already exists.

    virtual void AddSimulationSample( const DWORD dwIterationValue, const QWORD qwSampleValue );    //  Adds a sample to the iterator object.

    bool FGetSimulationSample(                                                  //  Gets the ith simulation sample.
        const size_t iSample,
        DWORD* const pdwIterationValue,
        QWORD* const qwSampleValue ) const;
};

//  ================================================================
class FixedCacheSimulationPresetIterator : public SimulationIterator
//  ================================================================
{
private:
    //  State.

    std::set<DWORD>::iterator m_cacheSizeIterator;
    std::set<DWORD>::iterator m_cacheSizeIteratorEnd;

public:
    //  Public API.

    FixedCacheSimulationPresetIterator( std::set<DWORD>& cacheSizes );
    DWORD DwGetNextIterationValue();        //  Returns the next cache size. "-1" (ulMax) means the end of the iteration.
};

//  ================================================================
class FixedCacheSimulationIterator : public SimulationIterator
//  ================================================================
{
public:
    //  Public API.

    DWORD DwGetNextIterationValue();        //  Returns the next cache size. "-1" (ulMax) means the end of the iteration.
};

//  ================================================================
class CacheFaultLookupSimulationIterator : public SimulationIterator
//  ================================================================
{
private:
    //  State.

    QWORD m_cFaultsTarget;

public:
    //  Ctor.

    CacheFaultLookupSimulationIterator( const QWORD cFaultsTarget );

    //  Public API.

    DWORD DwGetNextIterationValue();        //  Returns the next cache size. "-1" (ulMax) means the end of the iteration.
};

//  ================================================================
class CacheFaultMinSimulationIterator : public SimulationIterator
//  ================================================================
{
private:
    //  State.

    DWORD m_cIter;
    DWORD m_dtickIterMax;
    double m_q;

public:
    //  Ctor.

    CacheFaultMinSimulationIterator( const DWORD cIter, const DWORD dtickIterMax );

    //  Public API.

    void AddSimulationSample( const DWORD dwIterationValue, const QWORD cFaults );  //  Adds a sample to the iterator object.

    DWORD DwGetNextIterationValue();        //  Returns the next correlation interval. "-1" (ulMax) means the end of the iteration.
};

//  ================================================================
class ChkptDepthSimulationIterator : public FixedCacheSimulationIterator
//  ================================================================
{
public:
    //  Public API.

    DWORD DwGetNextIterationValue();        //  Returns the next cache size. "-1" (ulMax) means the end of the iteration.
};

#endif  //  _RMEMULATOR_HXX_INCLUDED


