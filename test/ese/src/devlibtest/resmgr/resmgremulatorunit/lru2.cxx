// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"

// Unit test class

//  ================================================================
class ResMgrEmulatorLRU2Test : public UNITTEST
//  ================================================================
{
    private:
        static ResMgrEmulatorLRU2Test s_instance;

    protected:
        ResMgrEmulatorLRU2Test() {}
    public:
        ~ResMgrEmulatorLRU2Test() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        BFTRACE* RgBFTracePrepareSuperColdTestOrderNotGuranteed_( const bool fReplaySuperCold );
        BFTRACE* RgBFTracePrepareCachePriorityTest_( const bool fReplayCachePriority );
        ERR ErrSuperColdYesLRU2ESE_();
        ERR ErrSuperColdNoLRU2ESE_();
        ERR ErrCachePriorityYesLRU2ESE_();
        ERR ErrCachePriorityNoLRU2ESE_();
};

ResMgrEmulatorLRU2Test ResMgrEmulatorLRU2Test::s_instance;

const char * ResMgrEmulatorLRU2Test::SzName() const         { return "ResMgrEmulatorLRU2Test"; };
const char * ResMgrEmulatorLRU2Test::SzDescription() const  { return "LRU2 tests for PageEvictionEmulator."; }
bool ResMgrEmulatorLRU2Test::FRunUnderESE98() const         { return true; }
bool ResMgrEmulatorLRU2Test::FRunUnderESENT() const         { return true; }
bool ResMgrEmulatorLRU2Test::FRunUnderESE97() const         { return true; }


//  ================================================================
ERR ResMgrEmulatorLRU2Test::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrSuperColdYesLRU2ESE_() );
    TestCall( ErrSuperColdNoLRU2ESE_() );
    TestCall( ErrCachePriorityYesLRU2ESE_() );
    TestCall( ErrCachePriorityNoLRU2ESE_() );

HandleError:
    return err;
}

//  ================================================================
BFTRACE* ResMgrEmulatorLRU2Test::RgBFTracePrepareSuperColdTestOrderNotGuranteed_( const bool fReplaySuperCold )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Touch all pages in reverse order; (201)
    //  - Supercold 2nd half of the pages; (251)
    //  - Evict/scavenge 1st or 2nd half of the pages; (301)
    //  - Touch 1st or 2nd half of the pages; (351)
    //  - Evict/purge 1st or 2nd half of the pages; (401)
    //  - Term; (402)
    //  - Sentinel. (403)

    BFTRACE* const rgbftrace = new BFTRACE[ 403 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }
    
    memset( rgbftrace, 0, 403 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1000;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    BFTRACE::BFSysResMgrInit_* pbfinit = &rgbftrace[ iTrace ].bfinit;
    pbfinit->K = 2;
    pbfinit->csecCorrelatedTouch = 0.001;
    pbfinit->csecTimeout = 100.0;
    pbfinit->csecUncertainty = 1.0;
    pbfinit->dblHashLoadFactor = 5.0;
    pbfinit->dblHashUniformity = 1.0;
    pbfinit->dblSpeedSizeTradeoff = 0.0;
    iTrace++;
    tick += 2000;

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
        tick += 2000;
    }

    //  - Touch all pages in reverse order; (201)

    for ( PGNO pgno = 100; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno--;
        tick += 2000;
    }

    //  - Supercold 2nd half of the pages; (251)

    for ( PGNO pgno = 51; iTrace < 251; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidSuperCold;
        BFTRACE::BFSuperCold_* pbfsupercold = &rgbftrace[ iTrace ].bfsupercold;
        pbfsupercold->ifmp = pgno % 2;
        pbfsupercold->pgno = pgno;
        pgno++;
        tick += 2000;
    }

    //  - Evict/scavenge 1st or 2nd half of the pages; (301)

    for ( PGNO pgno = fReplaySuperCold ? 51 : 1; iTrace < 301; iTrace++ )
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
        tick += 2000;
    }

    //  - Touch 1st or 2nd half of the pages; (351)

    for ( PGNO pgno = fReplaySuperCold ? 1 : 51; iTrace < 351; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
        tick += 2000;
    }

    //  - Evict/purge 1st or 2nd half of the pages; (401)

    for ( PGNO pgno = fReplaySuperCold ? 1 : 51; iTrace < 401; iTrace++ )
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
        tick += 2000;
    }

    //  - Term; (402)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (403)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorLRU2Test::RgBFTracePrepareCachePriorityTest_( const bool fReplayCachePriority )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages, pgno 50 page with a cache priority of 75; (101)
    //  - Touch all pages in reverse order, pgno 50 page with a cache priority of 75; (201)
    //  - Evict/scavenge all pages; (301)
    //  - Term; (302)
    //  - Sentinel. (303)

    BFTRACE* const rgbftrace = new BFTRACE[ 303 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }
    
    memset( rgbftrace, 0, 303 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    BFTRACE::BFSysResMgrInit_* pbfinit = &rgbftrace[ iTrace ].bfinit;
    pbfinit->K = 2;
    pbfinit->csecCorrelatedTouch = 0.001;
    pbfinit->csecTimeout = 50.0;
    pbfinit->csecUncertainty = 1.0;
    pbfinit->dblHashLoadFactor = 5.0;
    pbfinit->dblHashUniformity = 1.0;
    pbfinit->dblSpeedSizeTradeoff = 0.0;
    iTrace++;

    //  - Cache 100 pages, pgno 50 page with a cache priority of 75; (101)

    for ( PGNO pgno = 1; iTrace < 101; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = pgno == 50 ? 75 : 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 1500;
    }

    //  - Touch all pages in reverse order, pgno 50 page with a cache priority of 75; (201)

    for ( PGNO pgno = 100; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = pgno == 50 ? 75 : 100;
        pbftouch->fUseHistory = fTrue;
        pgno--;
        tick += 1500;
    }

    //  - Evict/scavenge all pages; (301)

    PGNO rgpgno[ 100 ] = { 0 };

    for ( PGNO ipg = 0; ipg < 29; ipg++ )
    {
        rgpgno[ ipg ] = ipg + 1;
    }

    if ( fReplayCachePriority )
    {
        rgpgno[ 29 ] = 50;
        for ( PGNO ipg = 30; ipg < 50; ipg++ )
        {
            rgpgno[ ipg ] = ipg;
        }
        for ( PGNO ipg = 50; ipg < 100; ipg++ )
        {
            rgpgno[ ipg ] = ipg + 1;
        }
    }
    else
    {
        for ( PGNO ipg = 29; ipg < 100; ipg++ )
        {
            rgpgno[ ipg ] = ipg + 1;
        }
    }

    for ( PGNO ipg = 0; iTrace < 301; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = rgpgno[ ipg ] % 2;
        pbfevict->pgno = rgpgno[ ipg ];
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonAvailPool;
        ipg++;
        tick += 1500;
    }

    //  - Term; (302)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (303)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
ERR ResMgrEmulatorLRU2Test::ErrSuperColdYesLRU2ESE_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTestOrderNotGuranteed_( true );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.
    
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
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cTouchesReal == 150 );
    TestCheck( stats.cTouchesSim == 150 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 150 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 50 );
    TestCheck( stats.cSuperColdedSim == 50 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 802000 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 400000 );
    TestCheck( stats.histoLifetimeReal.Max() == 700000 );
    TestCheck( stats.histoLifetimeReal.DblAve() >= 550000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU2Test::ErrSuperColdNoLRU2ESE_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTestOrderNotGuranteed_( false );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.
    
    TestCall( emulator.ErrSetReplaySuperCold( false ) );
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
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cTouchesReal == 150 );
    TestCheck( stats.cTouchesSim == 150 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 150 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 50 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 802000 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 500000 );
    TestCheck( stats.histoLifetimeReal.Max() == 600000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 550000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 500000 );
    TestCheck( stats.histoLifetimeSim.Max() == 600000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 550000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU2Test::ErrCachePriorityYesLRU2ESE_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareCachePriorityTest_( true );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.
    
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
    TestCheck( stats.cTouchesReal == 100 );
    TestCheck( stats.cTouchesSim == 100 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 450000 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 270000 );
    TestCheck( stats.histoLifetimeReal.Max() == 301500 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 300000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 270000 );
    TestCheck( stats.histoLifetimeSim.Max() == 301500 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 300000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU2Test::ErrCachePriorityNoLRU2ESE_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareCachePriorityTest_( false );
    TestCheck( rgbftrace != NULL );

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.
    
    TestCall( emulator.ErrSetReplayCachePriority( false ) );
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
    TestCheck( stats.cTouchesReal == 100 );
    TestCheck( stats.cTouchesSim == 100 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 100 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 450000 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 300000 );
    TestCheck( stats.histoLifetimeReal.Max() == 300000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 300000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 300000 );
    TestCheck( stats.histoLifetimeSim.Max() == 300000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 300000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

