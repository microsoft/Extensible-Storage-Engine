// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"

// Unit test class

//  ================================================================
class ResMgrEmulatorBasicTest : public UNITTEST
//  ================================================================
{
    private:
        static ResMgrEmulatorBasicTest s_instance;

    protected:
        ResMgrEmulatorBasicTest() {}
    public:
        ~ResMgrEmulatorBasicTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        BFTRACE* RgBFTracePrepareSuperColdTest_();
        BFTRACE* RgBFTracePrepareParamOverrideTest_();
        BFTRACE* RgBFTracePrepareFixedCacheModeTest_( const bool fSkipInit = false );
        BFTRACE* RgBFTracePrepareAgeBasedCacheModeTest_( const bool fSkipInit = false );
        BFTRACE* RgBFTracePrepareFixedChkptDepthModeTest_( const bool fUseSetLgposModifyTrace );
        ERR ErrStatsBasic_( const bool fNewPage, const bool fPrintHisto );
        ERR ErrStatsBasicExistingPage_();
        ERR ErrStatsBasicNewPage_();
        ERR ErrStatsBasicNoHistograms_();
        ERR ErrSuperColdYes_();
        ERR ErrSuperColdNo_();
        ERR ErrTouchTurnedCache_();
        ERR ErrCacheTurnedTouch_();
        ERR ErrTouchNoTouchTurnedCache_();
        ERR ErrNoTouch_( const bool fReplayNoTouch );
        ERR ErrReplayNoTouch_();
        ERR ErrDoNotReplayNoTouch_();
        ERR ErrDbScan_( const bool fReplayDbScan );
        ERR ErrReplayDbScan_();
        ERR ErrDoNotReplayDbScan_();
        ERR ErrInitParamOverrideK_();
        ERR ErrInitParamOverrideCorrelatedTouch_();
        ERR ErrInitParamOverrideTimeout_();
        ERR ErrFixedCacheModeBaseline_();
        ERR ErrFixedCacheModeSmallCache_();
        ERR ErrFixedCacheModeHotPages_();
        ERR ErrFixedCacheModeBigCache_();
        ERR ErrFixedCacheModeCacheMatchesData_();
        ERR ErrFixedCacheModeCacheBiggerThanData_();
        ERR ErrFixedCacheAbruptInit_( const bool fReplayInitTerm );
        ERR ErrAgeBasedCacheModeShort_();
        ERR ErrAgeBasedCacheModeMedium_();
        ERR ErrAgeBasedCacheModeLong_();
        ERR ErrFixedChkptDepthModeZeroDepth_( const bool fUseSetLgposModifyTrace );
        ERR ErrFixedChkptDepthModeMediumDepth_( const bool fUseSetLgposModifyTrace );
        ERR ErrFixedChkptDepthModeBigDepth_( const bool fUseSetLgposModifyTrace );
        ERR ErrFixedChkptDepthModeBigDepthSmallCache_( const bool fUseSetLgposModifyTrace );
};

ResMgrEmulatorBasicTest ResMgrEmulatorBasicTest::s_instance;

const char * ResMgrEmulatorBasicTest::SzName() const            { return "ResMgrEmulatorBasicTest"; };
const char * ResMgrEmulatorBasicTest::SzDescription() const     { return "Basic tests for PageEvictionEmulator."; }
bool ResMgrEmulatorBasicTest::FRunUnderESE98() const            { return true; }
bool ResMgrEmulatorBasicTest::FRunUnderESENT() const            { return true; }
bool ResMgrEmulatorBasicTest::FRunUnderESE97() const            { return true; }


//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    // Stats.
    TestCall( ErrStatsBasicExistingPage_() );
    TestCall( ErrStatsBasicNewPage_() );
    TestCall( ErrStatsBasicNoHistograms_() );

    // Supercold.
    TestCall( ErrSuperColdYes_() );
    TestCall( ErrSuperColdNo_() );

    // Cache/touch.
    TestCall( ErrTouchTurnedCache_() );
    TestCall( ErrCacheTurnedTouch_() );
    TestCall( ErrTouchNoTouchTurnedCache_() );
    TestCall( ErrReplayNoTouch_() );
    TestCall( ErrDoNotReplayNoTouch_() );

    // DbScan.
    TestCall( ErrReplayDbScan_() );
    TestCall( ErrDoNotReplayDbScan_() );

    // Init.
    TestCall( ErrInitParamOverrideK_() );
    TestCall( ErrInitParamOverrideCorrelatedTouch_() );
    TestCall( ErrInitParamOverrideTimeout_() );

    // Cache size variation.
    TestCall( ErrFixedCacheModeBaseline_() );
    TestCall( ErrFixedCacheModeSmallCache_() );
    TestCall( ErrFixedCacheModeHotPages_() );
    TestCall( ErrFixedCacheModeBigCache_() );
    TestCall( ErrFixedCacheModeCacheMatchesData_() );
    TestCall( ErrFixedCacheModeCacheBiggerThanData_() );
    TestCall( ErrFixedCacheAbruptInit_( true ) );
    TestCall( ErrFixedCacheAbruptInit_( false ) );

    // Cache age variation.
    TestCall( ErrAgeBasedCacheModeShort_() );
    TestCall( ErrAgeBasedCacheModeMedium_() );
    TestCall( ErrAgeBasedCacheModeLong_() );

    // Checkpoint depth variation (without SetLgposModify).
    TestCall( ErrFixedChkptDepthModeZeroDepth_( false /* fUseSetLgposModifyTrace */ ) );
    TestCall( ErrFixedChkptDepthModeMediumDepth_( false /* fUseSetLgposModifyTrace */ ) );
    TestCall( ErrFixedChkptDepthModeBigDepth_( false /* fUseSetLgposModifyTrace */ ) );
    TestCall( ErrFixedChkptDepthModeBigDepthSmallCache_( false /* fUseSetLgposModifyTrace */ ) );

    // Checkpoint depth variation (with SetLgposModify).
    TestCall( ErrFixedChkptDepthModeZeroDepth_( true /* fUseSetLgposModifyTrace */ ) );
    TestCall( ErrFixedChkptDepthModeMediumDepth_( true /* fUseSetLgposModifyTrace */ ) );
    TestCall( ErrFixedChkptDepthModeBigDepth_( true /* fUseSetLgposModifyTrace */ ) );
    TestCall( ErrFixedChkptDepthModeBigDepthSmallCache_( true /* fUseSetLgposModifyTrace */ ) );

HandleError:
    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrStatsBasic_( const bool fNewPage, const bool fPrintHisto )
//  ================================================================
{
    ERR err = JET_errSuccess;
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    //  Scenario:
    //  - 10 out-of-range traces; (10)
    //  - Init; (11)
    //  - Cache 100 pages; (111)
    //  - 10 discardable traces; (121)
    //  - Touch all pages; (221)
    //  - Evict/purge all pages; (321)
    //  - Term; (322)
    //  - 10 out-of-range traces; (332)
    //  - Init; (333)
    //  - Cache 100 pages; (433)
    //  - Init (abrupt cycle); (434)
    //  - Cache 100 pages; (534)
    //  - Touch all pages; (634)
    //  - Evict/scavenge all pages; (734)
    //  - Term; (735)
    //  - 10 out-of-range traces; (745)
    //  - Term (also considered out-of-range); (746)
    //  - 10 out-of-range traces; (756)
    //  - Init; (757)
    //  - Cache 100 pages; (857)
    //  - Sentinel (abrupt cycle). (858)

    BFTRACE rgbftrace[ 858 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - 10 out-of-range traces; (10)

    for ( PGNO pgno = 1; iTrace < 10; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;

    //  - Init; (11)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (111)

    for ( PGNO pgno = 1; iTrace < 111; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = !fNewPage;
        pbfcache->fNewPage = fNewPage;
        pgno++;
    }
    tick += 2;

    //  - 10 discardable traces; (121)

    for (; iTrace < 121; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidFTLReserved;
    }
    tick += 2;

    //  - Touch all pages; (221)

    for ( PGNO pgno = 1; iTrace < 221; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = ( pgno % 3 ) / 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;

    //  - Evict/purge all pages; (321)

    for ( PGNO pgno = 1; iTrace < 321; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = ( pgno % 3 ) / 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonPurgeContext;
        pgno++;
    }
    tick += 2;

    //  - Term; (322)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;
    tick += 2;

    //  - 10 out-of-range traces; (332)

    for ( PGNO pgno = 1; iTrace < 332; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;

    //  - Init; (333)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (433)

    for ( PGNO pgno = 1; iTrace < 433; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;

    //  - Init (abrupt cycle); (434)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (534)

    for ( PGNO pgno = 1; iTrace < 534; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;
    
    //  - Touch all pages; (634)

    for ( PGNO pgno = 1; iTrace < 634; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = ( pgno % 3 ) / 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = !fNewPage;
        pbftouch->fNewPage = fNewPage;
        pgno++;
    }
    tick += 2;

    //  - Evict/scavenge all pages; (734)

    for ( PGNO pgno = 1; iTrace < 734; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = ( pgno % 3 ) / 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
    }
    tick += 2;

    //  - Term; (735)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;
    tick += 2;

    //  - 10 out-of-range traces; (745)

    for ( PGNO pgno = 1; iTrace < 745; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;

    //  - Term; (746)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;
    tick += 2;

    //  - 10 out-of-range traces; (756)

    for ( PGNO pgno = 1; iTrace < 756; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }
    tick += 2;

    //  - Init; (757)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (857)

    for ( PGNO pgno = 1; iTrace < 857; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = ( pgno % 3 ) / 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
    }

    //  - Sentinel (abrupt cycle). (858)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    emulator.SetPrintHistograms( fPrintHisto );
    TestCall( emulator.ErrSetEvictNextOnShrink( false ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Dump stats.
    TestCall( emulator.ErrDumpStats( false ) );

    //  Validation (aggregated).

    const PageEvictionEmulator::STATS_AGG& statsAgg = emulator.GetStats();

    TestCheck( statsAgg.cpgCached == 0 );

    TestCheck( statsAgg.cResMgrCycles == 4 );
    TestCheck( statsAgg.cResMgrAbruptCycles == 2 );
    TestCheck( statsAgg.cDiscardedTraces == 10 );
    TestCheck( statsAgg.cOutOfRangeTraces == 41 );
    TestCheck( statsAgg.cEvictionsFailed == 0 );
    TestCheck( statsAgg.dtickDurationReal == ( tickEnd - tickBegin ) );

    TestCheck( statsAgg.cpgCached == 0 );
    TestCheck( statsAgg.cpgCachedMax == 100 );
    TestCheck( statsAgg.cRequestedUnique == 100 );
    TestCheck( statsAgg.cRequested == 600 );
    TestCheck( statsAgg.cFaultsRealAvoidable == 300 );
    TestCheck( statsAgg.cFaultsSimAvoidable == 300 );
    TestCheck( statsAgg.cCaches == 400 );
    TestCheck( statsAgg.cCachesDbScan == 0 );
    TestCheck( statsAgg.cTouchesTurnedCache == 0 );
    TestCheck( statsAgg.cTouchesNoTouchTurnedCache == 0 );
    TestCheck( statsAgg.cTouchesNoTouchDropped == 0 );
    TestCheck( statsAgg.cTouchesDbScan == 0 );
    TestCheck( statsAgg.cTouches == 200 );
    TestCheck( statsAgg.cTouchesNoTouch == 0 );
    TestCheck( statsAgg.cCachesTurnedTouch == 0 );
    TestCheck( statsAgg.cDirties == 0 );
    TestCheck( statsAgg.cDirtiesUnique == 0 );
    TestCheck( statsAgg.cWritesReal == 0 );
    TestCheck( statsAgg.cWritesRealChkpt == 0 );
    TestCheck( statsAgg.cWritesRealAvailPool == 0 );
    TestCheck( statsAgg.cWritesRealShrink == 0 );
    TestCheck( statsAgg.cWritesSim == 0 );
    TestCheck( statsAgg.cWritesSimChkpt == 0 );
    TestCheck( statsAgg.cWritesSimAvailPool == 0 );
    TestCheck( statsAgg.cWritesSimShrink == 0 );
    TestCheck( statsAgg.cEvictionsReal == 200 );
    TestCheck( statsAgg.cEvictionsPurge == 200 );
    TestCheck( statsAgg.cEvictionsCacheTooBig == 0 );
    TestCheck( statsAgg.cEvictionsCacheTooOld == 0 );
    TestCheck( statsAgg.cEvictionsSim == 400 );
    TestCheck( statsAgg.cSuperColdedReal == 0 );
    TestCheck( statsAgg.cSuperColdedSim == 0 );

    if ( fNewPage )
    {
        TestCheck( statsAgg.cFaultsReal == 300 );
        TestCheck( statsAgg.cFaultsSim == 300 );
        TestCheck( statsAgg.cTouchesReal == 100 );
        TestCheck( statsAgg.cTouchesSim == 100 );

        if ( fPrintHisto )
        {
            TestCheck( statsAgg.histoFaultsReal.C() == 100 );
            TestCheck( statsAgg.histoFaultsReal.Min() == 3 );
            TestCheck( statsAgg.histoFaultsReal.Max() == 3 );
            TestCheck( statsAgg.histoFaultsReal.DblAve() == 3.000 );
            TestCheck( statsAgg.histoFaultsSim.C() == 100 );
            TestCheck( statsAgg.histoFaultsSim.Min() == 3 );
            TestCheck( statsAgg.histoFaultsSim.Max() == 3 );
            TestCheck( statsAgg.histoFaultsSim.DblAve() == 3.000 );
            TestCheck( statsAgg.histoLifetimeReal.C() == 100 );
            TestCheck( statsAgg.histoLifetimeReal.Min() == 12 );
            TestCheck( statsAgg.histoLifetimeReal.Max() == 12 );
            TestCheck( statsAgg.histoLifetimeReal.DblAve() == 12.000 );
            TestCheck( statsAgg.histoLifetimeSim.C() == 100 );
            TestCheck( statsAgg.histoLifetimeSim.Min() == 12 );
            TestCheck( statsAgg.histoLifetimeSim.Max() == 12 );
            TestCheck( statsAgg.histoLifetimeSim.DblAve() == 12.000 );
        }
    }
    else
    {
        TestCheck( statsAgg.cFaultsReal == 400 );
        TestCheck( statsAgg.cFaultsSim == 400 );
        TestCheck( statsAgg.cTouchesReal == 200 );
        TestCheck( statsAgg.cTouchesSim == 200 );

        if ( fPrintHisto )
        {
            TestCheck( statsAgg.histoFaultsReal.C() == 100 );
            TestCheck( statsAgg.histoFaultsReal.Min() == 4 );
            TestCheck( statsAgg.histoFaultsReal.Max() == 4 );
            TestCheck( statsAgg.histoFaultsReal.DblAve() == 4.000 );
            TestCheck( statsAgg.histoFaultsSim.C() == 100 );
            TestCheck( statsAgg.histoFaultsSim.Min() == 4 );
            TestCheck( statsAgg.histoFaultsSim.Max() == 4 );
            TestCheck( statsAgg.histoFaultsSim.DblAve() == 4.000 );
            TestCheck( statsAgg.histoLifetimeReal.C() == 100 );
            TestCheck( statsAgg.histoLifetimeReal.Min() == 12 );
            TestCheck( statsAgg.histoLifetimeReal.Max() == 12 );
            TestCheck( statsAgg.histoLifetimeReal.DblAve() == 12.000 );
            TestCheck( statsAgg.histoLifetimeSim.C() == 100 );
            TestCheck( statsAgg.histoLifetimeSim.Min() == 12 );
            TestCheck( statsAgg.histoLifetimeSim.Max() == 12 );
            TestCheck( statsAgg.histoLifetimeSim.DblAve() == 12.000 );
        }
    }

    if ( !fPrintHisto )
    {
        TestCheck( statsAgg.histoFaultsReal.C() == 0 );
        TestCheck( statsAgg.histoFaultsReal.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsAgg.histoFaultsReal.Max() == 0 );
        TestCheck( statsAgg.histoFaultsReal.DblAve() == 0.000 );
        TestCheck( statsAgg.histoFaultsSim.C() == 0 );
        TestCheck( statsAgg.histoFaultsSim.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsAgg.histoFaultsSim.Max() == 0 );
        TestCheck( statsAgg.histoFaultsSim.DblAve() == 0.000 );
        TestCheck( statsAgg.histoLifetimeReal.C() == 0 );
        TestCheck( statsAgg.histoLifetimeReal.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsAgg.histoLifetimeReal.Max() == 0 );
        TestCheck( statsAgg.histoLifetimeReal.DblAve() == 0.000 );
        TestCheck( statsAgg.histoLifetimeSim.C() == 0 );
        TestCheck( statsAgg.histoLifetimeSim.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsAgg.histoLifetimeSim.Max() == 0 );
        TestCheck( statsAgg.histoLifetimeSim.DblAve() == 0.000 );
    }

    //  Validation (IFMP 0).

    const PageEvictionEmulator::STATS& statsIfmp0 = emulator.GetStats( 0 );

    TestCheck( statsIfmp0.cpgCached == 0 );
    TestCheck( statsIfmp0.cpgCachedMax == 67 );
    TestCheck( statsIfmp0.cRequestedUnique == 67 );
    TestCheck( statsIfmp0.cRequested == 402 );
    TestCheck( statsIfmp0.cFaultsRealAvoidable == 201 );
    TestCheck( statsIfmp0.cFaultsSimAvoidable == 201 );
    TestCheck( statsIfmp0.cCaches == 268 );
    TestCheck( statsIfmp0.cCachesDbScan == 0 );
    TestCheck( statsIfmp0.cTouchesTurnedCache == 0 );
    TestCheck( statsIfmp0.cTouchesNoTouchTurnedCache == 0 );
    TestCheck( statsIfmp0.cTouchesNoTouchDropped == 0 );
    TestCheck( statsIfmp0.cTouchesDbScan == 0 );
    TestCheck( statsIfmp0.cTouches == 134 );
    TestCheck( statsIfmp0.cTouchesNoTouch == 0 );
    TestCheck( statsIfmp0.cCachesTurnedTouch == 0 );
    TestCheck( statsIfmp0.cDirties == 0 );
    TestCheck( statsIfmp0.cDirtiesUnique == 0 );
    TestCheck( statsIfmp0.cWritesReal == 0 );
    TestCheck( statsIfmp0.cWritesRealChkpt == 0 );
    TestCheck( statsIfmp0.cWritesRealAvailPool == 0 );
    TestCheck( statsIfmp0.cWritesRealShrink == 0 );
    TestCheck( statsIfmp0.cWritesSim == 0 );
    TestCheck( statsIfmp0.cWritesSimChkpt == 0 );
    TestCheck( statsIfmp0.cWritesSimAvailPool == 0 );
    TestCheck( statsIfmp0.cWritesSimShrink == 0 );
    TestCheck( statsIfmp0.cEvictionsReal == 134 );
    TestCheck( statsIfmp0.cEvictionsPurge == 134 );
    TestCheck( statsIfmp0.cEvictionsCacheTooBig == 0 );
    TestCheck( statsIfmp0.cEvictionsCacheTooOld == 0 );
    TestCheck( statsIfmp0.cEvictionsSim == 268 );
    TestCheck( statsIfmp0.cSuperColdedReal == 0 );
    TestCheck( statsIfmp0.cSuperColdedSim == 0 );

    if ( fNewPage )
    {
        TestCheck( statsIfmp0.cFaultsReal == 201 );
        TestCheck( statsIfmp0.cFaultsSim == 201 );
        TestCheck( statsIfmp0.cTouchesReal == 67 );
        TestCheck( statsIfmp0.cTouchesSim == 67 );

        if ( fPrintHisto )
        {
            TestCheck( statsIfmp0.histoFaultsReal.C() == 67 );
            TestCheck( statsIfmp0.histoFaultsReal.Min() == 3 );
            TestCheck( statsIfmp0.histoFaultsReal.Max() == 3 );
            TestCheck( statsIfmp0.histoFaultsReal.DblAve() == 3.000 );
            TestCheck( statsIfmp0.histoFaultsSim.C() == 67 );
            TestCheck( statsIfmp0.histoFaultsSim.Min() == 3 );
            TestCheck( statsIfmp0.histoFaultsSim.Max() == 3 );
            TestCheck( statsIfmp0.histoFaultsSim.DblAve() == 3.000 );
            TestCheck( statsIfmp0.histoLifetimeReal.C() == 67 );
            TestCheck( statsIfmp0.histoLifetimeReal.Min() == 12 );
            TestCheck( statsIfmp0.histoLifetimeReal.Max() == 12 );
            TestCheck( statsIfmp0.histoLifetimeReal.DblAve() == 12.000 );
            TestCheck( statsIfmp0.histoLifetimeSim.C() == 67 );
            TestCheck( statsIfmp0.histoLifetimeSim.Min() == 12 );
            TestCheck( statsIfmp0.histoLifetimeSim.Max() == 12 );
            TestCheck( statsIfmp0.histoLifetimeSim.DblAve() == 12.000 );
        }
    }
    else
    {
        TestCheck( statsIfmp0.cFaultsReal == 268 );
        TestCheck( statsIfmp0.cFaultsSim == 268 );
        TestCheck( statsIfmp0.cTouchesReal == 134 );
        TestCheck( statsIfmp0.cTouchesSim == 134 );

        if ( fPrintHisto )
        {
            TestCheck( statsIfmp0.histoFaultsReal.C() == 67 );
            TestCheck( statsIfmp0.histoFaultsReal.Min() == 4 );
            TestCheck( statsIfmp0.histoFaultsReal.Max() == 4 );
            TestCheck( statsIfmp0.histoFaultsReal.DblAve() == 4.000 );
            TestCheck( statsIfmp0.histoFaultsSim.C() == 67 );
            TestCheck( statsIfmp0.histoFaultsSim.Min() == 4 );
            TestCheck( statsIfmp0.histoFaultsSim.Max() == 4 );
            TestCheck( statsIfmp0.histoFaultsSim.DblAve() == 4.000 );
            TestCheck( statsIfmp0.histoLifetimeReal.C() == 67 );
            TestCheck( statsIfmp0.histoLifetimeReal.Min() == 12 );
            TestCheck( statsIfmp0.histoLifetimeReal.Max() == 12 );
            TestCheck( statsIfmp0.histoLifetimeReal.DblAve() == 12.000 );
            TestCheck( statsIfmp0.histoLifetimeSim.C() == 67 );
            TestCheck( statsIfmp0.histoLifetimeSim.Min() == 12 );
            TestCheck( statsIfmp0.histoLifetimeSim.Max() == 12 );
            TestCheck( statsIfmp0.histoLifetimeSim.DblAve() == 12.000 );
        }
    }

    if ( !fPrintHisto )
    {
        TestCheck( statsIfmp0.histoFaultsReal.C() == 0 );
        TestCheck( statsIfmp0.histoFaultsReal.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp0.histoFaultsReal.Max() == 0 );
        TestCheck( statsIfmp0.histoFaultsReal.DblAve() == 0.000 );
        TestCheck( statsIfmp0.histoFaultsSim.C() == 0 );
        TestCheck( statsIfmp0.histoFaultsSim.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp0.histoFaultsSim.Max() == 0 );
        TestCheck( statsIfmp0.histoFaultsSim.DblAve() == 0.000 );
        TestCheck( statsIfmp0.histoLifetimeReal.C() == 0 );
        TestCheck( statsIfmp0.histoLifetimeReal.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp0.histoLifetimeReal.Max() == 0 );
        TestCheck( statsIfmp0.histoLifetimeReal.DblAve() == 0.000 );
        TestCheck( statsIfmp0.histoLifetimeSim.C() == 0 );
        TestCheck( statsIfmp0.histoLifetimeSim.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp0.histoLifetimeSim.Max() == 0 );
        TestCheck( statsIfmp0.histoLifetimeSim.DblAve() == 0.000 );
    }

    //  Validation (IFMP 1), indirectly.

    const PageEvictionEmulator::STATS& statsIfmp1 = emulator.GetStats( 1 );

    TestCheck( statsIfmp1.cpgCached == ( statsAgg.cpgCached - statsIfmp0.cpgCached ) );
    TestCheck( statsIfmp1.cpgCachedMax == ( statsAgg.cpgCachedMax - statsIfmp0.cpgCachedMax ) );
    TestCheck( statsIfmp1.cRequestedUnique == ( statsAgg.cRequestedUnique - statsIfmp0.cRequestedUnique ) );
    TestCheck( statsIfmp1.cRequested == ( statsAgg.cRequested - statsIfmp0.cRequested ) );
    TestCheck( statsIfmp1.cFaultsRealAvoidable == ( statsAgg.cFaultsRealAvoidable - statsIfmp0.cFaultsRealAvoidable ) );
    TestCheck( statsIfmp1.cFaultsSimAvoidable == ( statsAgg.cFaultsSimAvoidable - statsIfmp0.cFaultsSimAvoidable ) );
    TestCheck( statsIfmp1.cCaches == ( statsAgg.cCaches - statsIfmp0.cCaches ) );
    TestCheck( statsIfmp1.cCachesDbScan == ( statsAgg.cCachesDbScan - statsIfmp0.cCachesDbScan ) );
    TestCheck( statsIfmp1.cTouchesTurnedCache == ( statsAgg.cTouchesTurnedCache - statsIfmp0.cTouchesTurnedCache ) );
    TestCheck( statsIfmp1.cTouchesNoTouchTurnedCache == ( statsAgg.cTouchesNoTouchTurnedCache - statsIfmp0.cTouchesNoTouchTurnedCache ) );
    TestCheck( statsIfmp1.cTouchesNoTouchDropped == ( statsAgg.cTouchesNoTouchDropped - statsIfmp0.cTouchesNoTouchDropped ) );
    TestCheck( statsIfmp1.cTouchesDbScan == ( statsAgg.cTouchesDbScan - statsIfmp0.cTouchesDbScan ) );
    TestCheck( statsIfmp1.cTouches == ( statsAgg.cTouches - statsIfmp0.cTouches ) );
    TestCheck( statsIfmp1.cTouchesNoTouch == ( statsAgg.cTouchesNoTouch - statsIfmp0.cTouchesNoTouch ) );
    TestCheck( statsIfmp1.cCachesTurnedTouch == ( statsAgg.cCachesTurnedTouch - statsIfmp0.cCachesTurnedTouch ) );
    TestCheck( statsIfmp1.cDirties == ( statsAgg.cDirties - statsIfmp0.cDirties ) );
    TestCheck( statsIfmp1.cDirtiesUnique == ( statsAgg.cDirtiesUnique - statsIfmp0.cDirtiesUnique ) );
    TestCheck( statsIfmp1.cWritesReal == ( statsAgg.cWritesReal - statsIfmp0.cWritesReal ) );
    TestCheck( statsIfmp1.cWritesRealChkpt == ( statsAgg.cWritesRealChkpt - statsIfmp0.cWritesRealChkpt ) );
    TestCheck( statsIfmp1.cWritesRealAvailPool == ( statsAgg.cWritesRealAvailPool - statsIfmp0.cWritesRealAvailPool ) );
    TestCheck( statsIfmp1.cWritesRealShrink == ( statsAgg.cWritesRealShrink - statsIfmp0.cWritesRealShrink ) );
    TestCheck( statsIfmp1.cWritesSim == ( statsAgg.cWritesSim - statsIfmp0.cWritesSim ) );
    TestCheck( statsIfmp1.cWritesSimChkpt == ( statsAgg.cWritesSimChkpt - statsIfmp0.cWritesSimChkpt ) );
    TestCheck( statsIfmp1.cWritesSimAvailPool == ( statsAgg.cWritesSimAvailPool - statsIfmp0.cWritesSimAvailPool ) );
    TestCheck( statsIfmp1.cWritesSimShrink == ( statsAgg.cWritesSimShrink - statsIfmp0.cWritesSimShrink ) );
    TestCheck( statsIfmp1.cEvictionsReal == ( statsAgg.cEvictionsReal - statsIfmp0.cEvictionsReal ) );
    TestCheck( statsIfmp1.cEvictionsPurge == ( statsAgg.cEvictionsPurge - statsIfmp0.cEvictionsPurge ) );
    TestCheck( statsIfmp1.cEvictionsCacheTooBig == ( statsAgg.cEvictionsCacheTooBig - statsIfmp0.cEvictionsCacheTooBig ) );
    TestCheck( statsIfmp1.cEvictionsSim == ( statsAgg.cEvictionsSim - statsIfmp0.cEvictionsSim ) );
    TestCheck( statsIfmp1.cSuperColdedReal == ( statsAgg.cSuperColdedReal - statsIfmp0.cSuperColdedReal ) );
    TestCheck( statsIfmp1.cSuperColdedSim == ( statsAgg.cSuperColdedSim - statsIfmp0.cSuperColdedSim ) );

    TestCheck( statsIfmp1.cFaultsReal == ( statsAgg.cFaultsReal - statsIfmp0.cFaultsReal ) );
    TestCheck( statsIfmp1.cFaultsSim == ( statsAgg.cFaultsSim - statsIfmp0.cFaultsSim ) );
    TestCheck( statsIfmp1.cTouchesReal == ( statsAgg.cTouchesReal - statsIfmp0.cTouchesReal ) );
    TestCheck( statsIfmp1.cTouchesSim == ( statsAgg.cTouchesSim - statsIfmp0.cTouchesSim ) );

    TestCheck( statsIfmp1.histoFaultsReal.C() == ( statsAgg.histoFaultsReal.C() - statsIfmp0.histoFaultsReal.C() ) );
    TestCheck( statsIfmp1.histoFaultsSim.C() == ( statsAgg.histoFaultsSim.C() - statsIfmp0.histoFaultsSim.C() ) );
    TestCheck( statsIfmp1.histoLifetimeReal.C() == ( statsAgg.histoLifetimeReal.C() - statsIfmp0.histoLifetimeReal.C() ) );
    TestCheck( statsIfmp1.histoLifetimeSim.C() == ( statsAgg.histoLifetimeSim.C() - statsIfmp0.histoLifetimeSim.C() ) );

    if ( fPrintHisto )
    {

        if ( fNewPage )
        {
            TestCheck( statsIfmp1.histoFaultsReal.Min() == 3 );
            TestCheck( statsIfmp1.histoFaultsReal.Max() == 3 );
            TestCheck( statsIfmp1.histoFaultsReal.DblAve() == 3.000 );
            TestCheck( statsIfmp1.histoFaultsSim.Min() == 3 );
            TestCheck( statsIfmp1.histoFaultsSim.Max() == 3 );
            TestCheck( statsIfmp1.histoFaultsSim.DblAve() == 3.000 );
            TestCheck( statsIfmp1.histoLifetimeReal.Min() == 12 );
            TestCheck( statsIfmp1.histoLifetimeReal.Max() == 12 );
            TestCheck( statsIfmp1.histoLifetimeReal.DblAve() == 12.000 );
            TestCheck( statsIfmp1.histoLifetimeSim.Min() == 12 );
            TestCheck( statsIfmp1.histoLifetimeSim.Max() == 12 );
            TestCheck( statsIfmp1.histoLifetimeSim.DblAve() == 12.000 );
        }
        else
        {
            TestCheck( statsIfmp1.histoFaultsReal.Min() == 4 );
            TestCheck( statsIfmp1.histoFaultsReal.Max() == 4 );
            TestCheck( statsIfmp1.histoFaultsReal.DblAve() == 4.000 );
            TestCheck( statsIfmp1.histoFaultsSim.Min() == 4 );
            TestCheck( statsIfmp1.histoFaultsSim.Max() == 4 );
            TestCheck( statsIfmp1.histoFaultsSim.DblAve() == 4.000 );
            TestCheck( statsIfmp1.histoLifetimeReal.Min() == 12 );
            TestCheck( statsIfmp1.histoLifetimeReal.Max() == 12 );
            TestCheck( statsIfmp1.histoLifetimeReal.DblAve() == 12.000 );
            TestCheck( statsIfmp1.histoLifetimeSim.Min() == 12 );
            TestCheck( statsIfmp1.histoLifetimeSim.Max() == 12 );
            TestCheck( statsIfmp1.histoLifetimeSim.DblAve() == 12.000 );
        }
    }
    else
    {
        TestCheck( statsIfmp1.histoFaultsReal.C() == 0 );
        TestCheck( statsIfmp1.histoFaultsReal.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp1.histoFaultsReal.Max() == 0 );
        TestCheck( statsIfmp1.histoFaultsReal.DblAve() == 0.000 );
        TestCheck( statsIfmp1.histoFaultsSim.C() == 0 );
        TestCheck( statsIfmp1.histoFaultsSim.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp1.histoFaultsSim.Max() == 0 );
        TestCheck( statsIfmp1.histoFaultsSim.DblAve() == 0.000 );
        TestCheck( statsIfmp1.histoLifetimeReal.C() == 0 );
        TestCheck( statsIfmp1.histoLifetimeReal.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp1.histoLifetimeReal.Max() == 0 );
        TestCheck( statsIfmp1.histoLifetimeReal.DblAve() == 0.000 );
        TestCheck( statsIfmp1.histoLifetimeSim.C() == 0 );
        TestCheck( statsIfmp1.histoLifetimeSim.Min() == 0xFFFFFFFFFFFFFFFF );
        TestCheck( statsIfmp1.histoLifetimeSim.Max() == 0 );
        TestCheck( statsIfmp1.histoLifetimeSim.DblAve() == 0.000 );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrStatsBasicExistingPage_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrStatsBasic_( false, true ) );

HandleError:

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrStatsBasicNewPage_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrStatsBasic_( true, true ) );

HandleError:

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrStatsBasicNoHistograms_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrStatsBasic_( true, false ) );

HandleError:

    return err;
}

//  ================================================================
BFTRACE* ResMgrEmulatorBasicTest::RgBFTracePrepareSuperColdTest_()
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Touch all pages; (201)
    //  - Supercold second half of the pages less 1; (250)
    //  - Evict/purge first page; (251)
    //  - Evict/scavenge second half of the pages less 1; (300)
    //  - Touch first half of the pages, except for the first one; (349)
    //  - Touch last page; (350)
    //  - Evict/shrink/next first half of the pages, except for the first one; (399)
    //  - Evict/purge last page; (400)
    //  - Term; (401)
    //  - Sentinel. (402)

    BFTRACE* const rgbftrace = new BFTRACE[ 402 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }

    memset( rgbftrace, 0, 402 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Touch all pages; (201)

    for ( PGNO pgno = 1; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Supercold second half of the pages less 1; (250)

    for ( PGNO pgno = 51; iTrace < 250; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidSuperCold;
        BFTRACE::BFSuperCold_* pbfsupercold = &rgbftrace[ iTrace ].bfsupercold;
        pbfsupercold->ifmp = pgno % 2;
        pbfsupercold->pgno = pgno;
        pgno++;
        tick += 2;
    }

    //  - Evict/purge first page; (251)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 1 % 2;
    pbfevict->pgno = 1;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonPurgePage;
    iTrace++;
    tick += 2;

    //  - Evict/scavenge second half of the pages less 1; (300)

    for ( PGNO pgno = 51; iTrace < 300; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
        tick += 2;
    }

    //  - Touch first half of the pages, except for the first one; (349)

    for ( PGNO pgno = 2; iTrace < 349; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Touch last page; (350)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidTouch;
    BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
    pbftouch->ifmp = 100 % 2;
    pbftouch->pgno = 100;
    pbftouch->pctPri = 100;
    pbftouch->fUseHistory = fTrue;
    iTrace++;
    tick += 2;
    
    //  - Evict/shrink/next first half of the pages, except for the first one; (399)

    for ( PGNO pgno = 2; iTrace < 399; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonShrink;
        pgno++;
        tick += 2;
    }

    //  - Evict/purge last page; (400)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 100 % 2;
    pbfevict->pgno = 100;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonPurgeContext;
    iTrace++;
    tick += 2;

    //  - Term; (401)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (402)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorBasicTest::RgBFTracePrepareParamOverrideTest_()
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Touch first 50 pages in reverse order; (151)
    //  - Evict/scavenge last 50 pages (LRU-1 behavior); (201)
    //  - Evict/scavenge last 10 pages of the first half, reverse order (LRU-1 behavior); (211)
    //  - Cache last 60 pages; (271)
    //  - Term; (272)
    //  - Sentinel. (273)

    BFTRACE* const rgbftrace = new BFTRACE[ 273 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }

    memset( rgbftrace, 0, 273 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 0;

    //  - Init; (1)

    tick += 200;
    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    rgbftrace[ iTrace ].bfinit.K = 1;
    rgbftrace[ iTrace ].bfinit.csecCorrelatedTouch = 0.128;
    rgbftrace[ iTrace ].bfinit.csecTimeout = 100.0;
    rgbftrace[ iTrace ].bfinit.csecUncertainty = 0.1;
    rgbftrace[ iTrace ].bfinit.dblHashLoadFactor = 5.0;
    rgbftrace[ iTrace ].bfinit.dblHashUniformity = 1.0;
    rgbftrace[ iTrace ].bfinit.dblSpeedSizeTradeoff = 0.0;
    iTrace++;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = 0;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = false;
        pbfcache->fNewPage = true;
        pgno++;
    }

    //  - Touch first 50 pages in reverse order; (151)

    for ( PGNO pgno = 50; iTrace < 151; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = 0;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = true;
        pgno--;
    }

    //  - Evict/scavenge last 50 pages (LRU-1 behavior); (201)

    for ( PGNO pgno = 51; iTrace < 201; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = 0;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
    }

    //  - Evict/scavenge last 10 pages of the first half, reverse order (LRU-1 behavior); (211)

    for ( PGNO pgno = 50; iTrace < 211; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = 0;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno--;
    }

    //  - Cache last 60 pages; (271)

    for ( PGNO pgno = 41; iTrace < 271; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = 0;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = true;
        pbfcache->fNewPage = false;
        pgno++;
    }

    //  - Term; (272)

    tick += 200;
    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (273)

    rgbftrace[ iTrace ].traceid = bftidInvalid;
    
    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorBasicTest::RgBFTracePrepareFixedCacheModeTest_( const bool fSkipInit )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge first 95 pages (LRU-1 behavior); (196)
    //  - Cache pages 91-95; (201)
    //  - Touch pages 96-100; (206)
    //  - Term; (207)
    //  - Sentinel. (208)

    BFTRACE* const rgbftrace = new BFTRACE[ 208 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }

    memset( rgbftrace, 0, 208 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 0;

    //  - Init; (1)

    tick += 200;
    rgbftrace[ iTrace ].tick = tick;

    if ( !fSkipInit )
    {
        rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
        rgbftrace[ iTrace ].bfinit.K = 1;
        rgbftrace[ iTrace ].bfinit.csecCorrelatedTouch = 0.128;
        rgbftrace[ iTrace ].bfinit.csecTimeout = 100.0;
        rgbftrace[ iTrace ].bfinit.csecUncertainty = 0.1;
        rgbftrace[ iTrace ].bfinit.dblHashLoadFactor = 5.0;
        rgbftrace[ iTrace ].bfinit.dblHashUniformity = 1.0;
        rgbftrace[ iTrace ].bfinit.dblSpeedSizeTradeoff = 0.0;
    }
    else
    {
        rgbftrace[ iTrace ].traceid = bftidFTLReserved;
    }
    
    iTrace++;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = 0;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = true;
        pbfcache->fNewPage = false;
        pgno++;
    }

    //  - Evict/scavenge first 95 pages (LRU-1 behavior); (196)

    for ( PGNO pgno = 1; iTrace < 196; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = 0;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
    }

    //  - Cache pages 91-95; (201)

    for ( PGNO pgno = 91; iTrace < 201; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = 0;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = true;
        pbfcache->fNewPage = false;
        pgno++;
    }

    //  - Touch pages 96-100; (206)

    for ( PGNO pgno = 96; iTrace < 206; iTrace++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = 0;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = true;
        pgno++;
    }


    //  - Term; (207)

    tick += 200;
    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (208)

    rgbftrace[ iTrace ].traceid = bftidInvalid;
    
    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorBasicTest::RgBFTracePrepareAgeBasedCacheModeTest_( const bool fSkipInit )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Touch all 100 pages (inverse order); (201)
    //  - Term; (202)
    //  - Sentinel. (203)

    BFTRACE* const rgbftrace = new BFTRACE[ 203 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }

    memset( rgbftrace, 0, 203 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 0;

    //  - Init; (1)

    tick += 2;
    rgbftrace[ iTrace ].tick = tick;

    if ( !fSkipInit )
    {
        rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
        rgbftrace[ iTrace ].bfinit.K = 1;
        rgbftrace[ iTrace ].bfinit.csecCorrelatedTouch = 0.128;
        rgbftrace[ iTrace ].bfinit.csecTimeout = 100.0;
        rgbftrace[ iTrace ].bfinit.csecUncertainty = 0.1;
        rgbftrace[ iTrace ].bfinit.dblHashLoadFactor = 5.0;
        rgbftrace[ iTrace ].bfinit.dblHashUniformity = 1.0;
        rgbftrace[ iTrace ].bfinit.dblSpeedSizeTradeoff = 0.0;
    }
    else
    {
        rgbftrace[ iTrace ].traceid = bftidFTLReserved;
    }
    
    iTrace++;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        tick += 2;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = 0;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = true;
        pbfcache->fNewPage = false;
        pgno++;
    }

    //  - Touch all 100 pages (inverse order); (201)

    for ( PGNO pgno = 100; iTrace < 201; iTrace++ )
    {
        tick += 2;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = 0;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = true;
        pgno--;
    }


    //  - Term; (202)

    tick += 2;
    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (203)

    rgbftrace[ iTrace ].traceid = bftidInvalid;
    
    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorBasicTest::RgBFTracePrepareFixedChkptDepthModeTest_( const bool fUseSetLgposModifyTrace )
//  ================================================================
{
    ERR err = JET_errSuccess;    

    //  Scenario:
    //  - Init; (1)
    //  - Cache and dirty 20 pages, including one unset dirty for each, pgno 1 is always dirtied (81 / 121);
    //  - Write out pages 1-10, each one with a different reason (91 / 131);
    //  - Term, will write out some pages left dirty; (92 / 132)
    //  - Sentinel. (93 / 133)

    const size_t cbftrace = fUseSetLgposModifyTrace ? 133 : 93;
    BFTRACE* const rgbftrace = new BFTRACE[ cbftrace ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }

    memset( rgbftrace, 0, cbftrace * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 0;

    //  - Init; (1)

    tick += 200;
    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    rgbftrace[ iTrace ].bfinit.K = 1;
    rgbftrace[ iTrace ].bfinit.csecCorrelatedTouch = 0.128;
    rgbftrace[ iTrace ].bfinit.csecTimeout = 100.0;
    rgbftrace[ iTrace ].bfinit.csecUncertainty = 0.1;
    rgbftrace[ iTrace ].bfinit.dblHashLoadFactor = 5.0;
    rgbftrace[ iTrace ].bfinit.dblHashUniformity = 1.0;
    rgbftrace[ iTrace ].bfinit.dblSpeedSizeTradeoff = 0.0;
    iTrace++;

    //  - Cache and dirty 20 pages, including one unset dirty for each (81 / 121);

    for ( PGNO pgno = 1; pgno <= 20; pgno++ )
    {
        tick += 200;

        BFTRACE::BFDirty_* pbfdirty = NULL;
        BFTRACE::BFSetLgposModify_* pbfsetlgposmodify = NULL;

        //  Cache.

        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = 0;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = true;
        pbfcache->fNewPage = false;
        iTrace++;

        //  Unset dirty.

        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidDirty;
        pbfdirty = &rgbftrace[ iTrace ].bfdirty;
        pbfdirty->ifmp = 0;
        pbfdirty->pgno = pgno;
        pbfdirty->bfdf = bfdfDirty;
        pbfdirty->lgenModify = 0;
        pbfdirty->isecModify = 0;
        pbfdirty->ibModify = 0;
        iTrace++;

        if ( fUseSetLgposModifyTrace )
        {
            //  SetLgposModify.

            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidSetLgposModify;
            pbfsetlgposmodify = &rgbftrace[ iTrace ].bfsetlgposmodify;
            pbfsetlgposmodify->ifmp = 0;
            pbfsetlgposmodify->pgno = pgno;
            pbfsetlgposmodify->lgenModify = (ULONG)pgno;
            pbfsetlgposmodify->isecModify = 0;
            pbfsetlgposmodify->ibModify = 0;
            iTrace++;
        }

        //  Dirty.

        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidDirty;
        pbfdirty = &rgbftrace[ iTrace ].bfdirty;
        pbfdirty->ifmp = 0;
        pbfdirty->pgno = pgno;
        pbfdirty->bfdf = bfdfDirty;
        pbfdirty->lgenModify = (ULONG)pgno;
        pbfdirty->isecModify = 0;
        pbfdirty->ibModify = 0;
        iTrace++;

        if ( fUseSetLgposModifyTrace )
        {
            //  SetLgposModify pgno 1.

            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidSetLgposModify;
            pbfsetlgposmodify = &rgbftrace[ iTrace ].bfsetlgposmodify;
            pbfsetlgposmodify->ifmp = 0;
            pbfsetlgposmodify->pgno = 1;
            pbfsetlgposmodify->lgenModify = (ULONG)pgno;
            pbfsetlgposmodify->isecModify = 0;
            pbfsetlgposmodify->ibModify = 0;
            iTrace++;
        }

        //  Dirty pgno 1.

        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidDirty;
        pbfdirty = &rgbftrace[ iTrace ].bfdirty;
        pbfdirty->ifmp = 0;
        pbfdirty->pgno = 1;
        pbfdirty->bfdf = bfdfDirty;
        pbfdirty->lgenModify = (ULONG)pgno;
        pbfdirty->isecModify = 0;
        pbfdirty->ibModify = 0;
        iTrace++;
    }

    //  - Write out pages 1-10, each one with a different reason (91 / 131);

    for ( PGNO pgno = 1; pgno <= 10; pgno++ )
    {
        tick += 200;
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidWrite;
        BFTRACE::BFWrite_* pbfwrite = &rgbftrace[ iTrace ].bfwrite;
        pbfwrite->ifmp = 0;
        pbfwrite->pgno = pgno;
        pbfwrite->bfdf = bfdfDirty;
        pbfwrite->iorp = (BYTE)iorpBFCheckpointAdv + ( pgno - 1 ) % 8;
        iTrace++;
    }

    //  - Term, will write out some pages left dirty; (92 / 132)

    tick += 200;
    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (93 / 133)

    rgbftrace[ iTrace ].traceid = bftidInvalid;
    iTrace++;

    TestCheck( iTrace == cbftrace );

HandleError:
    return rgbftrace;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrSuperColdYes_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetLifetimeHistoRes( 2 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 250 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 149 );
    TestCheck( stats.cTouchesReal == 150 );
    TestCheck( stats.cTouchesSim == 101 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 150 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 149 );
    TestCheck( stats.cSuperColdedReal == 49 );
    TestCheck( stats.cSuperColdedSim == 49 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 49 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 49 );
    TestCheck( stats.dtickDurationReal == 800 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 2 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.490 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 400 );
    TestCheck( stats.histoLifetimeReal.Max() == 696 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 548.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 498 );
    TestCheck( stats.histoLifetimeSim.Max() == 698 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 622.480 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrSuperColdNo_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetReplaySuperCold( false ) );
    TestCall( emulator.ErrSetLifetimeHistoRes( 2 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 250 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 149 );
    TestCheck( stats.cTouchesReal == 150 );
    TestCheck( stats.cTouchesSim == 101 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 150 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 149 );
    TestCheck( stats.cSuperColdedReal == 49 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 49 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 49 );
    TestCheck( stats.dtickDurationReal == 800 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 2 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.490 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 400 );
    TestCheck( stats.histoLifetimeReal.Max() == 696 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 548.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 498 );
    TestCheck( stats.histoLifetimeSim.Max() == 698 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 622.480 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrTouchTurnedCache_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );
    
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge 2nd half of the pages (MRU order); (151)
    //  - Touch 1st quarter of the pages (would have been evicted previously in LRU); (176)
    //  - Evict/purge 1st half of the pages (remaining on MRU-based algorithm); (226)
    //  - Term; (227)
    //  - Sentinel. (228)

    BFTRACE rgbftrace[ 228 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/scavenge 2nd half of the pages (MRU order); (151)

    for ( PGNO pgno = 51; iTrace < 151; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
        tick += 2;
    }

    //  - Touch 1st quarter of the pages (would have been evicted previously in LRU); (176)

    for ( PGNO pgno = 1; iTrace < 176; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/purge 1st half of the pages (remaining on MRU-based algorithm); (226)

    for ( PGNO pgno = 1; iTrace < 226; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonPurgePage;
        pgno++;
        tick += 2;
    }

    //  - Term; (227)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (228)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetEvictNextOnShrink( false ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 125 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 125 );
    TestCheck( stats.cTouchesReal == 25 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 25 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 125 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 25 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 25 );
    TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 2 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.250 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 100 );
    TestCheck( stats.histoLifetimeReal.Max() == 350 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 225.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 200 );
    TestCheck( stats.histoLifetimeSim.Max() == 300 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 256.500 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrCacheTurnedTouch_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );
    
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge 2nd half of the pages (MRU order); (151)
    //  - Cache 2nd half of the pages (would not have been evicted previously in LRU); (201)
    //  - Cache additional 25 pages; (226)
    //  - Evict/purge all of the pages (remaining on MRU-based algorithm); (351)
    //  - Term; (352)
    //  - Sentinel. (353)

    BFTRACE rgbftrace[ 353 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/scavenge 2nd half of the pages (MRU order); (151)

    for ( PGNO pgno = 51; iTrace < 151; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
        tick += 2;
    }

    //  - Cache 2nd half of the pages (would not have been evicted previously in LRU); (201)

    for ( PGNO pgno = 51; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Cache additional 25 pages; (226)

    for ( PGNO pgno = 101; iTrace < 226; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/purge all of the pages (remaining on MRU-based algorithm); (351)

    for ( PGNO pgno = 1; iTrace < 351; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonPurgeContext;
        pgno++;
        tick += 2;
    }

    //  - Term; (352)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (353)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 125;
    pbfftlc->rgpgnoMax[ 1 ] = 125;

    //  Init./run.

    TestCall( emulator.ErrSetEvictNextOnShrink( false ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 125 );
    TestCheck( stats.cRequested == 175 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 175 );
    TestCheck( stats.cFaultsSim == 125 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 50 );
    TestCheck( stats.cCaches == 175 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cEvictionsReal == 175 );
    TestCheck( stats.cEvictionsSim == 125 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 50 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 50 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );
    TestCheck( stats.histoFaultsReal.C() == 125 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 2 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.400 );
    TestCheck( stats.histoFaultsSim.C() == 125 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 125 );
    TestCheck( stats.histoLifetimeReal.Min() == 250 ) ;
    TestCheck( stats.histoLifetimeReal.Max() == 450 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 370.000 );
    TestCheck( stats.histoLifetimeSim.C() == 125 );
    TestCheck( stats.histoLifetimeSim.Min() == 150 );
    TestCheck( stats.histoLifetimeSim.Max() == 350 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 250.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrTouchNoTouchTurnedCache_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge 2nd half of the pages (MRU order); (151)
    //  - Touch 1st 30 pages with no-touch (would have been evicted previously in LRU); (181)
    //  - Evict/purge 1st half of the pages (remaining on MRU-based algorithm); (231)
    //  - Term; (232)
    //  - Sentinel. (233)

    BFTRACE rgbftrace[ 233 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/scavenge 2nd half of the pages (MRU order); (151)

    for ( PGNO pgno = 51; iTrace < 151; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno++;
        tick += 2;
    }

    //  - Touch 1st 30 pages with no-touch (would have been evicted previously in LRU); (181)

    for ( PGNO pgno = 1; iTrace < 181; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pbftouch->fNoTouch = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/purge 1st half of the pages (remaining on MRU-based algorithm); (231)

    for ( PGNO pgno = 1; iTrace < 231; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonPurgePage;
        pgno++;
        tick += 2;
    }

    //  - Term; (232)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (233)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetEvictNextOnShrink( false ) );
    TestCall( emulator.ErrSetReplayNoTouch( false ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 130 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 130 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 30 );
    TestCheck( stats.cTouchesNoTouch == 30 );
    TestCheck( stats.cTouchesNoTouchTurnedCache == 30 );
    TestCheck( stats.cTouchesNoTouchDropped == 0 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 130 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 30 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 30 );
    TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 2 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.300 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 100 );
    TestCheck( stats.histoLifetimeReal.Max() == 360 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 230.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 200 );
    TestCheck( stats.histoLifetimeSim.Max() == 320 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 269.300 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrNoTouch_( const bool fReplayNoTouch )
//  ================================================================
{
    ERR err = JET_errSuccess;

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Touch all pages with no-touch in reverse order; (201)
    //  - Evict/scavenge all pages (order will depend on do/do-not replay no-touch); (301)
    //  - Term; (302)
    //  - Sentinel. (303)

    BFTRACE rgbftrace[ 303 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache 100 pages; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Touch all pages with no-touch in reverse order; (201)

    for ( PGNO pgno = 100; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pbftouch->fNoTouch = fTrue;
        pgno--;
        tick += 2;
    }

    //  - Evict/scavenge all pages (order will depend on do/do-not replay no-touch); (301)

    for ( PGNO pgno = ( fReplayNoTouch ? 100 : 1 ); iTrace < 301; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        pgno += ( fReplayNoTouch ? -1 : +1 );
        tick += 2;
    }

    //  - Term; (302)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (303)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetReplayNoTouch( fReplayNoTouch ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 200 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cTouchesNoTouch == 100 );
    TestCheck( stats.cTouchesNoTouchTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );

    if ( fReplayNoTouch )
    {
        TestCheck( stats.cTouchesNoTouchDropped == 0 );
        TestCheck( stats.cTouchesSim == 100 );
        TestCheck( stats.histoLifetimeReal.C() == 100 );
        TestCheck( stats.histoLifetimeReal.Min() == 202 );
        TestCheck( stats.histoLifetimeReal.Max() == 598 );
        TestCheck( stats.histoLifetimeReal.DblAve() == 400.000 );
        TestCheck( stats.histoLifetimeSim.C() == 100 );
        TestCheck( stats.histoLifetimeSim.Min() == 202 );
        TestCheck( stats.histoLifetimeSim.Max() == 598 );
        TestCheck( stats.histoLifetimeSim.DblAve() == 400.000 );
    }
    else
    {
        TestCheck( stats.cTouchesNoTouchDropped == 100 );
        TestCheck( stats.cTouchesSim == 0 );
        TestCheck( stats.histoLifetimeReal.C() == 100 );
        TestCheck( stats.histoLifetimeReal.Min() == 400 );
        TestCheck( stats.histoLifetimeReal.Max() == 400 );
        TestCheck( stats.histoLifetimeReal.DblAve() == 400.000 );
        TestCheck( stats.histoLifetimeSim.C() == 100 );
        TestCheck( stats.histoLifetimeSim.Min() == 400 );
        TestCheck( stats.histoLifetimeSim.Max() == 400 );
        TestCheck( stats.histoLifetimeSim.DblAve() == 400.000 );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrReplayNoTouch_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrNoTouch_( true ) );

HandleError:

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrDoNotReplayNoTouch_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrNoTouch_( false ) );

HandleError:

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrDbScan_( const bool fReplayDbScan )
//  ================================================================
{
    ERR err = JET_errSuccess;

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache first 50 pages from DBM; (51)
    //  - Cache last 50 pages not from DBM; (101)
    //  - Touch all pages in reverse order not from DBM; (201)
    //  - Touch pages 31 through 70 from DBM; (241)
    //  - Evict/scavenge all pages (order will depend on do/do-not replay no-touch); (341)
    //  - Term; (342)
    //  - Sentinel. (343)

    BFTRACE rgbftrace[ 343 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick += 2;

    //  - Cache first 50 pages from DBM; (51)
    //  - Cache last 50 pages not from DBM; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pbfcache->fDBScan = ( pgno > 50 );
        pgno++;
        tick += 2;
    }

    //  - Touch all pages in reverse order not from DBM; (201)

    for ( PGNO pgno = 100; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pbftouch->fDBScan = fFalse;
        pgno--;
        tick += 2;
    }

    //  - Touch pages 31 through 70 from DBM; (241)

    for ( PGNO pgno = 31; iTrace < 241; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pbftouch->fDBScan = fTrue;
        pgno++;
        tick += 2;
    }

    //  - Evict/scavenge all pages (order will depend on do/do-not replay no-touch); (341)

    if ( fReplayDbScan )
    {
        for ( PGNO pgno = 100; iTrace < 301; iTrace++ )
        {
            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidEvict;
            BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
            pbfevict->ifmp = pgno % 2;
            pbfevict->pgno = pgno;
            pbfevict->fCurrentVersion = fTrue;
            pbfevict->pctPri = 100;
            pbfevict->bfef = bfefReasonAvailPool;
            pgno--;
            tick += 2;

            if ( pgno == 70 )
            {
                pgno = 30;
            }
        }

        for ( PGNO pgno = 31; iTrace < 341; iTrace++ )
        {
            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidEvict;
            BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
            pbfevict->ifmp = pgno % 2;
            pbfevict->pgno = pgno;
            pbfevict->fCurrentVersion = fTrue;
            pbfevict->pctPri = 100;
            pbfevict->bfef = bfefReasonAvailPool;
            pgno++;
            tick += 2;
        }
    }
    else
    {
        for ( PGNO pgno = 100; iTrace < 341; iTrace++ )
        {
            rgbftrace[ iTrace ].tick = tick;
            rgbftrace[ iTrace ].traceid = bftidEvict;
            BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
            pbfevict->ifmp = pgno % 2;
            pbfevict->pgno = pgno;
            pbfevict->fCurrentVersion = fTrue;
            pbfevict->pctPri = 100;
            pbfevict->bfef = bfefReasonAvailPool;
            pgno--;
            tick += 2;
        }
    }

    //  - Term; (342)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (343)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetReplayDbScan( fReplayDbScan ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cCachesDbScan == 50 );
    TestCheck( stats.cTouches == 140 );
    TestCheck( stats.cTouchesDbScan == 40 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );

    if ( fReplayDbScan )
    {
        TestCheck( stats.cFaultsReal == 100 );
        TestCheck( stats.cRequested == 240 );
        TestCheck( stats.cTouchesReal == 140 );
        TestCheck( stats.cTouchesSim == 140 );
        TestCheck( stats.cCachesTurnedTouch == 0 );
        TestCheck( stats.cTouchesTurnedCache == 0 );
        TestCheck( stats.histoFaultsReal.C() == 100 );
        TestCheck( stats.histoFaultsSim.C() == 100 );
        TestCheck( stats.histoLifetimeReal.C() == 100 );
        TestCheck( stats.histoLifetimeReal.Min() == 282 );
        TestCheck( stats.histoLifetimeReal.Max() == 598 );
        TestCheck( stats.histoLifetimeReal.DblAve() == 480.000 );
        TestCheck( stats.histoLifetimeSim.C() == 100 );
        TestCheck( stats.histoLifetimeSim.Min() == 282 );
        TestCheck( stats.histoLifetimeSim.Max() == 598 );
        TestCheck( stats.histoLifetimeSim.DblAve() == 480.000 );
    }
    else
    {
        TestCheck( stats.cFaultsReal == 50 );
        TestCheck( stats.cRequested == 150 );
        TestCheck( stats.cTouchesReal == 100 );
        TestCheck( stats.cTouchesSim == 50 );
        TestCheck( stats.cCachesTurnedTouch == 0 );
        TestCheck( stats.cTouchesTurnedCache == 50 );
        TestCheck( stats.histoFaultsReal.C() == 50 );
        TestCheck( stats.histoFaultsSim.C() == 50 );
        TestCheck( stats.histoLifetimeReal.C() == 50 );
        TestCheck( stats.histoLifetimeReal.Min() == 482 );
        TestCheck( stats.histoLifetimeReal.Max() == 678 );
        TestCheck( stats.histoLifetimeReal.DblAve() == 580.000 );
        TestCheck( stats.histoLifetimeSim.C() == 50 );
        TestCheck( stats.histoLifetimeSim.Min() == 482 );
        TestCheck( stats.histoLifetimeSim.Max() == 678 );
        TestCheck( stats.histoLifetimeSim.DblAve() == 580.000 );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrReplayDbScan_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrDbScan_( true ) );

HandleError:

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrDoNotReplayDbScan_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    TestCall( ErrDbScan_( false ) );

HandleError:

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrInitParamOverrideK_()
//  ================================================================
{
    ERR err = JET_errSuccess;
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;
    BFTRACE::BFSysResMgrInit_ bfinitOverride;
    PageEvictionEmulator::ResetInitOverrides( &bfinitOverride );

    printf( "\t%s\r\n", __FUNCTION__ );

    BFTRACE* const rgbftrace = RgBFTracePrepareParamOverrideTest_();
    TestCheck( rgbftrace != NULL );

    //  Start simulations and validation.

    printf( "\tInit./run (original).\r\n" );
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsOriginal = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsOriginal.cFaultsReal == 60 );
    TestCheck( statsOriginal.cFaultsSim == 60 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

    printf( "\tOverriding.\r\n" );
    bfinitOverride.K = 2;
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrSetInitOverrides( &bfinitOverride ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsOverride = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsOverride.cFaultsReal == 60 );
    TestCheck( statsOverride.cFaultsSim == 50 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

    printf( "\tReset overrides.\r\n" );
    bfinitOverride.K = 2;
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrSetInitOverrides( &bfinitOverride ) );
    TestCall( emulator.ErrSetInitOverrides( NULL ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsRestore = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsRestore.cFaultsReal == 60 );
    TestCheck( statsRestore.cFaultsSim == 60 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

HandleError:

    emulator.Term();
    if ( pbfftlc )
    {
        BFFTLTerm( pbfftlc );
    }
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrInitParamOverrideCorrelatedTouch_()
//  ================================================================
{
    ERR err = JET_errSuccess;
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;
    BFTRACE::BFSysResMgrInit_ bfinitOverride;
    PageEvictionEmulator::ResetInitOverrides( &bfinitOverride );

    printf( "\t%s\r\n", __FUNCTION__ );

    BFTRACE* const rgbftrace = RgBFTracePrepareParamOverrideTest_();
    TestCheck( rgbftrace != NULL );

    //  Start simulations and validation.

    printf( "\tInit./run (original).\r\n" );
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsOriginal = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsOriginal.cFaultsReal == 60 );
    TestCheck( statsOriginal.cFaultsSim == 60 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

    printf( "\tOverriding.\r\n" );
    bfinitOverride.csecCorrelatedTouch = 100.0;
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrSetInitOverrides( &bfinitOverride ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsOverride = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsOverride.cFaultsReal == 60 );
    TestCheck( statsOverride.cFaultsSim == 20 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

    printf( "\tReset overrides.\r\n" );
    bfinitOverride.csecCorrelatedTouch = 100.0;
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrSetInitOverrides( &bfinitOverride ) );
    TestCall( emulator.ErrSetInitOverrides( NULL ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsRestore = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsRestore.cFaultsReal == 60 );
    TestCheck( statsRestore.cFaultsSim == 60 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

HandleError:

    emulator.Term();
    if ( pbfftlc )
    {
        BFFTLTerm( pbfftlc );
    }
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrInitParamOverrideTimeout_()
//  ================================================================
{
    ERR err = JET_errSuccess;
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;
    BFTRACE::BFSysResMgrInit_ bfinitOverride;
    PageEvictionEmulator::ResetInitOverrides( &bfinitOverride );

    printf( "\t%s\r\n", __FUNCTION__ );

    BFTRACE* const rgbftrace = RgBFTracePrepareParamOverrideTest_();
    TestCheck( rgbftrace != NULL );

    //  Start simulations and validation.

    printf( "\tInit./run (original).\r\n" );
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsOriginal = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsOriginal.cFaultsReal == 60 );
    TestCheck( statsOriginal.cFaultsSim == 60 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

    printf( "\tOverriding.\r\n" );
    bfinitOverride.K = 2;
    bfinitOverride.csecTimeout = 0.02;
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrSetInitOverrides( &bfinitOverride ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsOverride = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsOverride.cFaultsReal == 60 );
    TestCheck( statsOverride.cFaultsSim == 20 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

    printf( "\tReset overrides.\r\n" );
    bfinitOverride.K = 2;
    bfinitOverride.csecTimeout = 0.02;
    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );
    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    TestCall( emulator.ErrSetInitOverrides( &bfinitOverride ) );
    TestCall( emulator.ErrSetInitOverrides( NULL ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );
    const PageEvictionEmulator::STATS_AGG& statsRestore = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );
    TestCheck( statsRestore.cFaultsReal == 60 );
    TestCheck( statsRestore.cFaultsSim == 60 );
    emulator.Term();
    BFFTLTerm( pbfftlc );
    pbfftlc = NULL;

HandleError:

    emulator.Term();
    if ( pbfftlc )
    {
        BFFTLTerm( pbfftlc );
    }
    delete[] rgbftrace;

    return err;
}


//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheModeBaseline_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspVariable ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 105 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 5 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 5 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 105 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 0 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 10 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == -1.0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheModeSmallCache_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 1 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 1 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 110 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 10 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 5 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 110 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 109 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 1 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 110.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 10.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == 100.0 * 1.0 / 100.0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheModeHotPages_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 10 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 10 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 0 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 10 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 5 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 90 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 10 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 100.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 0.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == 100.0 * 10.0 / 100.0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheModeBigCache_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 50 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 50 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 0 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 10 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 5 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 50 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 50 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 100.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 0.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == 100.0 * 50.0 / 100.0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheModeCacheMatchesData_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 100 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 0 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 10 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 5 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 0 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 100 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 100.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 0.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == 100.0 * 100.0 / 100.0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheModeCacheBiggerThanData_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 101 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 0 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 10 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 5 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 0 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 100 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 100.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 0.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == 100.0 * 101.0 / 100.0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedCacheAbruptInit_( const bool fReplayInitTerm )
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s (fReplayInitTerm = %d)\r\n", __FUNCTION__, (int)fReplayInitTerm );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedCacheModeTest_( true );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 10 ) );
    TestCall( emulator.ErrSetReplayInitTerm( fReplayInitTerm ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 10 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 110 );
    TestCheck( stats.cDiscardedTraces == 1 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 105 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cFaultsRealAvoidable == 5 );
    TestCheck( stats.cFaultsSimAvoidable == 0 );
    TestCheck( stats.cTouchesReal == 5 );
    TestCheck( stats.cTouchesSim == 10 );
    TestCheck( stats.cCaches == 105 );
    TestCheck( stats.cTouches == 5 );
    TestCheck( stats.cCachesTurnedTouch == 5 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 95 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 90 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 10 );
    TestCheck( stats.pctCacheFaultRateReal == 100.0 * 105.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSim == 100.0 * 100.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateRealAvoidable == 100.0 * 5.0 / 110.0 );
    TestCheck( stats.pctCacheFaultRateSimAvoidable == 100.0 * 0.0 / 110.0 );
    TestCheck( stats.pctCacheSizeRatioSim == 100.0 * 10.0 / 100.0 );
    TestCheck( stats.cResMgrCycles == 1 );
    if ( fReplayInitTerm )
    {
        TestCheck( stats.cResMgrAbruptCycles == 1 );
    }
    else
    {
        TestCheck( stats.cResMgrAbruptCycles == 2 );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrAgeBasedCacheModeShort_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareAgeBasedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspAgeBased, 1 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 1 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 200 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 200 );
    TestCheck( stats.cFaultsRealAvoidable == 0 );
    TestCheck( stats.cFaultsSimAvoidable == 100 );
    TestCheck( stats.cTouchesReal == 100 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 100 );
    TestCheck( stats.cEvictionsReal == 0 );
    TestCheck( stats.cEvictionsSim == 200 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 0 );
    TestCheck( stats.cEvictionsCacheTooOld == 200 );
    TestCheck( stats.cEvictionsPurge == 0 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrAgeBasedCacheModeMedium_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareAgeBasedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspAgeBased, 99 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 50 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 200 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 175 );
    TestCheck( stats.cFaultsRealAvoidable == 0 );
    TestCheck( stats.cFaultsSimAvoidable == 75 );
    TestCheck( stats.cTouchesReal == 100 );
    TestCheck( stats.cTouchesSim == 25 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 75 );
    TestCheck( stats.cEvictionsReal == 0 );
    TestCheck( stats.cEvictionsSim == 175 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 0 );
    TestCheck( stats.cEvictionsCacheTooOld == 126 );
    TestCheck( stats.cEvictionsPurge == 49 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrAgeBasedCacheModeLong_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareAgeBasedCacheModeTest_();
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspAgeBased, 1000 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 100 );
    TestCheck( stats.cRequestedUnique == 100 );
    TestCheck( stats.cRequested == 200 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cFaultsRealAvoidable == 0 );
    TestCheck( stats.cFaultsSimAvoidable == 0 );
    TestCheck( stats.cTouchesReal == 100 );
    TestCheck( stats.cTouchesSim == 100 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsReal == 0 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsCacheTooBig == 0 );
    TestCheck( stats.cEvictionsCacheTooOld == 0 );
    TestCheck( stats.cEvictionsPurge == 100 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedChkptDepthModeZeroDepth_( const bool fUseSetLgposModifyTrace )
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s (fUseSetLgposModifyTrace = %d)\r\n", __FUNCTION__, (int)fUseSetLgposModifyTrace );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedChkptDepthModeTest_( fUseSetLgposModifyTrace );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 20;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 20 ) );
    TestCall( emulator.ErrSetCheckpointDepth( 0 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 20 );
    TestCheck( stats.cRequestedUnique == 20 );
    TestCheck( stats.cRequested == 20 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cCaches == 20 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cDirties == 40 );
    TestCheck( stats.cDirtiesUnique == 20 );
    TestCheck( stats.cWritesReal == 10 );
    TestCheck( stats.cWritesRealChkpt == 2 );
    TestCheck( stats.cWritesRealAvailPool == 2 );
    TestCheck( stats.cWritesRealShrink == 1 );
    TestCheck( stats.cWritesSim == 40 );
    TestCheck( stats.cWritesSimChkpt == 40 );
    TestCheck( stats.cWritesSimAvailPool == 0 );
    TestCheck( stats.cWritesSimShrink == 0 );

    const PageEvictionEmulator::STATS& statsIfmp = emulator.GetStats( 0 );
    TestCheck( statsIfmp.lgenMin == 1 );
    TestCheck( statsIfmp.lgenMax == 20 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedChkptDepthModeMediumDepth_( const bool fUseSetLgposModifyTrace )
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s (fUseSetLgposModifyTrace = %d)\r\n", __FUNCTION__, (int)fUseSetLgposModifyTrace );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedChkptDepthModeTest_( fUseSetLgposModifyTrace );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 20;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 20 ) );
    TestCall( emulator.ErrSetCheckpointDepth( 4 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 20 );
    TestCheck( stats.cRequestedUnique == 20 );
    TestCheck( stats.cRequested == 20 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cCaches == 20 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cWritesReal == 10 );
    TestCheck( stats.cWritesRealChkpt == 2 );
    TestCheck( stats.cWritesRealAvailPool == 2 );
    TestCheck( stats.cWritesRealShrink == 1 );
    TestCheck( stats.cWritesSim == 24 ); 
    TestCheck( stats.cWritesSimAvailPool == 0 );
    TestCheck( stats.cWritesSimShrink == 0 );
    if ( fUseSetLgposModifyTrace )
    {
        TestCheck( stats.cDirties == 40 );
        TestCheck( stats.cWritesSimChkpt == 19 );
    }
    else
    {
        TestCheck( stats.cDirties == 59 );
        TestCheck( stats.cWritesSimChkpt == 20 );
    }
    TestCheck( stats.cDirtiesUnique == 20 );

    const PageEvictionEmulator::STATS& statsIfmp = emulator.GetStats( 0 );
    TestCheck( statsIfmp.lgenMin == 1 );
    TestCheck( statsIfmp.lgenMax == 20 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedChkptDepthModeBigDepth_( const bool fUseSetLgposModifyTrace )
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s (fUseSetLgposModifyTrace = %d)\r\n", __FUNCTION__, (int)fUseSetLgposModifyTrace );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedChkptDepthModeTest_( fUseSetLgposModifyTrace );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 20;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 20 ) );
    TestCall( emulator.ErrSetCheckpointDepth( 20 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 20 );
    TestCheck( stats.cRequestedUnique == 20 );
    TestCheck( stats.cRequested == 20 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cCaches == 20 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cWritesReal == 10 );
    TestCheck( stats.cWritesRealChkpt == 2 );
    TestCheck( stats.cWritesRealAvailPool == 2 );
    TestCheck( stats.cWritesRealShrink == 1 );
    TestCheck( stats.cWritesSim == 20 );
    TestCheck( stats.cWritesSimChkpt == 0 );
    TestCheck( stats.cWritesSimAvailPool == 0 );
    TestCheck( stats.cWritesSimShrink == 0 );
    if ( fUseSetLgposModifyTrace )
    {
        TestCheck( stats.cDirties == 40 );
    }
    else
    {
        TestCheck( stats.cDirties == 59 );
    }
    TestCheck( stats.cDirtiesUnique == 20 );

    const PageEvictionEmulator::STATS& statsIfmp = emulator.GetStats( 0 );
    TestCheck( statsIfmp.lgenMin == 1 );
    TestCheck( statsIfmp.lgenMax == 20 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBasicTest::ErrFixedChkptDepthModeBigDepthSmallCache_( const bool fUseSetLgposModifyTrace )
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s (fUseSetLgposModifyTrace = %d)\r\n", __FUNCTION__, (int)fUseSetLgposModifyTrace );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareFixedChkptDepthModeTest_( fUseSetLgposModifyTrace );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 1;
    pbfftlc->rgpgnoMax[ 0 ] = 20;

    //  Init./run.

    TestCall( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, 1 ) );
    TestCall( emulator.ErrSetCheckpointDepth( 20 ) );
    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Validation.

    const PageEvictionEmulator::STATS_AGG& stats = emulator.GetStats();
    TestCall( emulator.ErrDumpStats( false ) );

    TestCheck( stats.cpgCachedMax == 1 );
    TestCheck( stats.cRequestedUnique == 20 );
    TestCheck( stats.cRequested == 20 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cFaultsReal == 20 );
    TestCheck( stats.cCaches == 20 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cDirties == 40 );
    TestCheck( stats.cDirtiesUnique == 20 );
    TestCheck( stats.cWritesReal == 10 );
    TestCheck( stats.cWritesRealChkpt == 2 );
    TestCheck( stats.cWritesRealAvailPool == 2 );
    TestCheck( stats.cWritesRealShrink == 1 );
    TestCheck( stats.cWritesSim == 20 );
    TestCheck( stats.cWritesSimChkpt == 0 );
    TestCheck( stats.cWritesSimAvailPool == 19 );
    TestCheck( stats.cWritesSimShrink == 0 );

    const PageEvictionEmulator::STATS& statsIfmp = emulator.GetStats( 0 );
    TestCheck( statsIfmp.lgenMin == 1 );
    TestCheck( statsIfmp.lgenMax == 20 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}
