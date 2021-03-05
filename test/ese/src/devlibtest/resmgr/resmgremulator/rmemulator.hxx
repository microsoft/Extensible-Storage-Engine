// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _RMEMULATOR_HXX_INCLUDED
#define _RMEMULATOR_HXX_INCLUDED


#ifdef BUILD_ENV_IS_NT
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
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


#define JET_errTestError        -64



struct PAGE
{
public:
    static SIZE_T OffsetOfILE() { return OffsetOf( PAGE, m_ile ); }

private:
    CInvasiveList<PAGE, OffsetOfILE>::CElement m_ile;

public:
    IFMPPGNO ifmppgno;

    TICK tickLastFaultSim;
    TICK tickLastRequestSim;
};

struct LGPOS
{
public:
    USHORT ib;

    USHORT isec;

    ULONG lgen;

    LGPOS();
    LGPOS( const ULONG lgenT, const USHORT isecT, const USHORT ibT );

    INT FIsSet() const;
};

INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 );

const LGPOS lgposMin( 0, 0, 0 );
const LGPOS lgposMax( 0x7FFFFFFFU, (USHORT)0xFFFFU, (USHORT)0xFFFFU );

struct DirtyPageOp
{
    LGPOS lgpos;

    PGNO pgno;
};

struct PAGEENTRY
{
public:
    PAGE* ppage;

    bool fDirty;

    LGPOS lgposCleanLast;

    ULONG cRequests;

    ULONG cDirties;

    ULONG cWritesReal;

    ULONG cWritesSim;

    ULONG cFaultsReal;

    ULONG cFaultsSim;

    TICK tickLastCachedReal;

    TICK dtickCachedReal;

    TICK dtickCachedSim;


    PAGEENTRY ();
};


enum PageEvictionContextType
{
    pectypPAGE,
    pectypIFMPPGNO
};


class IPageEvictionAlgorithmImplementation
{
protected:

    IPageEvictionAlgorithmImplementation();

public:
    
    virtual ~IPageEvictionAlgorithmImplementation();


    virtual ERR ErrInit( const BFTRACE::BFSysResMgrInit_& bfinit ) = 0;
    virtual void Term( const BFTRACE::BFSysResMgrTerm_& bfterm ) = 0;


    virtual ERR ErrCachePage( PAGE* const ppage, const BFTRACE::BFCache_& bfcache ) = 0;
    virtual ERR ErrTouchPage( PAGE* const ppage, const BFTRACE::BFTouch_& bftouch ) = 0;
    virtual ERR ErrSuperColdPage( PAGE* const ppage, const BFTRACE::BFSuperCold_& bfsupercold ) = 0;
    virtual ERR ErrEvictSpecificPage( PAGE* const ppage, const BFTRACE::BFEvict_& bfevict ) = 0;
    virtual ERR ErrEvictNextPage( void* const pv, const BFTRACE::BFEvict_& bfevict, PageEvictionContextType* const ppectyp ) = 0;
    virtual ERR ErrGetNextPageToEvict( void* const pv, PageEvictionContextType* const ppectyp ) = 0;


    virtual bool FNeedsPreProcessing() const;
    virtual ERR ErrStartProcessing();
    virtual ERR ErrResetProcessing();


    virtual size_t CbPageContext() const = 0;

    virtual void InitPage( PAGE* const ppage ) = 0;

    virtual void DestroyPage( PAGE* const ppage ) = 0;
};


class PageEvictionEmulator
{
public:

    enum PageEvictionEmulatorCacheSizePolicy
    {
        peecspFixed,
        peecspVariable,
        peecspAgeBased,
        peecspMax,
    };

private:

    enum PageEvictionEmulatorState
    {
        peesUninitialized,
        peesInitialized,
        peesRunning,
        peesDone
    };

public:

    struct STATS
    {

        CPG cpgCached;

        CPG cpgCachedMax;

        QWORD cRequestedUnique;

        QWORD cRequested;

        QWORD cFaultsReal;

        QWORD cFaultsSim;

        QWORD cFaultsRealAvoidable;

        QWORD cFaultsSimAvoidable;

        QWORD cCaches;

        QWORD cCachesNew;

        QWORD cCachesNewAlreadyCached;

        QWORD cCachesTurnedTouch;

        QWORD cCachesDbScan;

        QWORD cTouchesReal;

        QWORD cTouchesSim;

        QWORD cTouches;

        QWORD cTouchesNew;

        QWORD cTouchesNoTouch;

        QWORD cTouchesTurnedCache;

        QWORD cTouchesNoTouchTurnedCache;

        QWORD cTouchesNoTouchDropped;

        QWORD cTouchesDbScan;

        QWORD cDirties;

        QWORD cDirtiesUnique;

        QWORD cWritesReal;

        QWORD cWritesRealChkpt;

        QWORD cWritesRealAvailPool;

        QWORD cWritesRealShrink;

        QWORD cWritesSim;

        QWORD cWritesSimChkpt;

        QWORD cWritesSimAvailPool;

        QWORD cWritesSimShrink;

        QWORD cEvictionsReal;

        QWORD cEvictionsSim;

        QWORD cEvictionsCacheTooBig;

        QWORD cEvictionsCacheTooOld;

        QWORD cEvictionsPurge;

        QWORD cSuperColdedReal;

        QWORD cSuperColdedSim;

        DWORD lgenMin;

        DWORD lgenMax;

        double pctCacheFaultRateReal;

        double pctCacheSizeRatioSim;

        double pctCacheFaultRateSim;

        double pctCacheFaultRateRealAvoidable;

        double pctCacheFaultRateSimAvoidable;

        CPerfectHistogramStats histoFaultsReal;

        CPerfectHistogramStats histoFaultsSim;

        CPerfectHistogramStats histoLifetimeReal;

        CPerfectHistogramStats histoLifetimeSim;

        STATS& operator=( const STATS& stats )
        {
            if ( this != &stats )
            {
                *this = stats;
            }

            return *this;
        }
    };



    struct STATS_AGG : public STATS
    {
        DWORD cResMgrCycles;

        DWORD cResMgrAbruptCycles;

        QWORD cDiscardedTraces;

        QWORD cOutOfRangeTraces;

        QWORD cEvictionsFailed;

        TICK dtickDurationReal;

        TICK dtickDurationSimRun;
    };

private:
    

    PageEvictionEmulator();

public:


    ~PageEvictionEmulator();
    

    ERR ErrInit( BFFTLContext* const pbfftlContext,
                    IPageEvictionAlgorithmImplementation* const pipeaImplementation );
    void Term();


    ERR ErrSetReplaySuperCold( const bool fReplaySuperCold );
    ERR ErrSetReplayNoTouch( const bool fReplayNoTouch );
    ERR ErrSetReplayDbScan( const bool fReplayDbScan );
    ERR ErrSetReplayCachePriority( const bool fReplayCachePriority );
    ERR ErrSetEvictNextOnShrink( const bool fEvictNextOnShrink );
    ERR ErrSetReplayInitTerm( const bool fReplayInitTerm );
    ERR ErrSetInitOverrides( const BFTRACE::BFSysResMgrInit_* const pbfinit );
    ERR ErrSetCacheSize( const PageEvictionEmulatorCacheSizePolicy peecsp,
                         const DWORD dwParam = 0 );
    ERR ErrSetCheckpointDepth( const DWORD clogsChkptDepth );
    ERR ErrSetPrintSampleInterval( const TICK dtickPrintSampleInterval );
    void SetPrintHistograms( const bool fPrintHistograms );
    ERR ErrSetFaultsHistoRes( const ULONG cFaultsHistoRes );
    ERR ErrSetLifetimeHistoRes( const TICK dtickLifetimeHistoRes );


    ERR ErrExecute();


    static PageEvictionEmulator& GetEmulatorObj();

    const PageEvictionEmulator::STATS_AGG& GetStats();

    const PageEvictionEmulator::STATS& GetStats( const size_t ifmp );


    static void ResetInitOverrides( BFTRACE::BFSysResMgrInit_* const pbfinit );
    ERR ErrDumpStats();
    ERR ErrDumpStats( const bool fDumpHistogramDetails );


    static TICK __stdcall DwGetTickCount();

    static void ResetGlobalTick();

    static void SetGlobalTick( const TICK tick, const bool fTimeMayGoBack );

private:

    static PageEvictionEmulator s_singleton;

    static TICK s_tickCurrent;

    static TICK s_tickLast;

    static TICK s_dtickAdj;
    

    PageEvictionEmulatorState m_state;

    bool m_fResMgrInit;

    CArray<BYTE*> m_arrayPages;

    CInvasiveList<PAGE, PAGE::OffsetOfILE> m_ilAvailPages;

    CArray<CArray<PAGEENTRY>> m_arrayPageEntries;

    CArray<std::list<DirtyPageOp>*> m_arrayDirtyPageOps;

    PageEvictionEmulator::STATS_AGG m_statsAgg;

    CArray<PageEvictionEmulator::STATS> m_stats;

    BFFTLContext* m_pbfftlContext;

    IPageEvictionAlgorithmImplementation* m_pipeaImplementation;

    bool m_fReplaySuperCold;

    bool m_fReplayNoTouch;

    bool m_fReplayDbScan;

    bool m_fReplayCachePriority;

    bool m_fEvictNextOnShrink;

    bool m_fReplayInitTerm;

    BFTRACE::BFSysResMgrInit_ m_bfinit;

    PageEvictionEmulatorCacheSizePolicy m_peecsp;

    DWORD m_cbfCacheSize;

    TICK m_dtickCacheAge;

    DWORD m_clogsChkptDepth;

    TICK m_dtickPrintSampleInterval;

    bool m_fPrintHistograms;

    ULONG m_cFaultsHistoRes;

    TICK m_dtickLifetimeHistoRes;

    size_t m_cbPage;

    size_t m_cpgChunk;

    bool m_fSetLgposModifySupported;


    void ResetConfig();

    void ResetInitOverrides();

    PAGE* PpgGetAvailablePage_();

    void FreePage_( PAGE* const ppg );

    void PurgeCache_();

    PAGEENTRY* PpgeGetEntry_( const IFMPPGNO& ifmppgno ) const;

    PAGEENTRY* PpgeGetEntry_( const void* const pv, const PageEvictionContextType pectyp ) const;

    void ModifyPage_( const IFMPPGNO& ifmppgno, const LGPOS& lgposModify );

    void InsertDirtyPageOp_( const IFMP ifmp, const PGNO pgno, const LGPOS& lgpos );

    void RemoveOldestDirtyPageOp_( const IFMP ifmp, PGNO* const ppgno, LGPOS* const plgpos );

    void GetOldestDirtyLgpos_( const IFMP ifmp, LGPOS* const plgpos ) const;

    void GetNewestDirtyLgpos_( const IFMP ifmp, LGPOS* const plgpos ) const;

    static void DumpHistogram_( CPerfectHistogramStats& histogram,
                                const SAMPLE sampleMin,
                                const SAMPLE sampleMax,
                                const SAMPLE sampleRes,
                                const bool fDumpDetails );


    ERR ErrCachePage_( PAGEENTRY* const ppge, const IFMPPGNO& ifmppgno, const bool fNewPage );

    void TouchPage_( PAGEENTRY* const ppge, const BFTRACE::BFTouch_& bftouch );

    void DirtyPage_( PAGEENTRY* const ppge );

    void WritePage_( PAGEENTRY* const ppge, const IOREASONPRIMARY iorp, const LGPOS& lgposCurrent ) ;

    void CleanPage_( PAGEENTRY* const ppge, const LGPOS& lgposClean );

    void SupercoldPage_( PAGEENTRY* const ppge );

    void EvictPage_( PAGEENTRY* const ppge, const IOREASONPRIMARY iorp, const LGPOS& lgposCurrent );


    ERR ErrProcessTraceInit_( const BFTRACE& bftrace );

    ERR ErrFakeProcessTraceInit_();

    void ProcessTraceTerm_( const BFTRACE& bftrace );

    void FakeProcessTraceTerm_();

    ERR ErrProcessTraceCache_( BFTRACE& bftrace );

    ERR ErrProcessTraceTouch_( BFTRACE& bftrace );

    ERR ErrProcessTraceSuperCold_( const BFTRACE& bftrace );

    ERR ErrProcessTraceEvict_( BFTRACE& bftrace );

    ERR ErrProcessTraceDirty_( const BFTRACE& bftrace );

    ERR ErrProcessTraceWrite_( const BFTRACE& bftrace );

    ERR ErrProcessTraceSetLgposModify_( const BFTRACE& bftrace );

    ERR ErrProcessTraceDefault_( const BFTRACE& bftrace );

    void MaintainAgeBasedCache_();
};


class SimulationIterator
{
protected:

    typedef std::map<DWORD, QWORD> SimulationSamples;
    typedef std::pair<DWORD, QWORD> SimulationSample;
    typedef SimulationSamples::const_iterator SimulationSamplesIter;
    SimulationSamples m_samples;

public:

    virtual DWORD DwGetNextIterationValue() = 0;

    bool FSimulationSampleExists( const DWORD dwIterationValue ) const;

    virtual void AddSimulationSample( const DWORD dwIterationValue, const QWORD qwSampleValue );

    bool FGetSimulationSample(
        const size_t iSample,
        DWORD* const pdwIterationValue,
        QWORD* const qwSampleValue ) const;
};

class FixedCacheSimulationPresetIterator : public SimulationIterator
{
private:

    std::set<DWORD>::iterator m_cacheSizeIterator;
    std::set<DWORD>::iterator m_cacheSizeIteratorEnd;

public:

    FixedCacheSimulationPresetIterator( std::set<DWORD>& cacheSizes );
    DWORD DwGetNextIterationValue();
};

class FixedCacheSimulationIterator : public SimulationIterator
{
public:

    DWORD DwGetNextIterationValue();
};

class CacheFaultLookupSimulationIterator : public SimulationIterator
{
private:

    QWORD m_cFaultsTarget;

public:

    CacheFaultLookupSimulationIterator( const QWORD cFaultsTarget );


    DWORD DwGetNextIterationValue();
};

class CacheFaultMinSimulationIterator : public SimulationIterator
{
private:

    DWORD m_cIter;
    DWORD m_dtickIterMax;
    double m_q;

public:

    CacheFaultMinSimulationIterator( const DWORD cIter, const DWORD dtickIterMax );


    void AddSimulationSample( const DWORD dwIterationValue, const QWORD cFaults );

    DWORD DwGetNextIterationValue();
};

class ChkptDepthSimulationIterator : public FixedCacheSimulationIterator
{
public:

    DWORD DwGetNextIterationValue();
};

#endif


