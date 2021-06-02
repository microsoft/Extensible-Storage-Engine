// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "resmgremulatorunit.hxx"

// Unit test class

//  ================================================================
class ResMgrEmulatorLRU1Test : public UNITTEST
//  ================================================================
{
    private:
        static ResMgrEmulatorLRU1Test s_instance;

    protected:
        ResMgrEmulatorLRU1Test() {}
    public:
        ~ResMgrEmulatorLRU1Test() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        BFTRACE* RgBFTracePrepareSuperColdTest_( const bool fReplaySuperCold );
        BFTRACE* RgBFTracePrepareSuperColdTestOrderNotGuranteed_();
        BFTRACE* RgBFTracePrepareCachePriorityTest_( const bool fReplayCachePriority );
        BFTRACE* RgBFTracePrepareEvictNextOnShrinkTest_( const bool fEvictNextOnShrink );
        ERR ErrSuperColdYesLRU1Test_();
        ERR ErrSuperColdNoLRU1Test_();
        ERR ErrCachePriorityYesLRU1Test_();
        ERR ErrCachePriorityNoLRU1Test_();
        ERR ErrEvictNextOnShrinkYesLRU1Test_();
        ERR ErrEvictNextOnShrinkNoLRU1Test_();
        ERR ErrSuperColdYesLRU1ESE_();
        ERR ErrSuperColdNoLRU1ESE_();
        ERR ErrCachePriorityYesLRU1ESE_();
        ERR ErrCachePriorityNoLRU1ESE_();
};

ResMgrEmulatorLRU1Test ResMgrEmulatorLRU1Test::s_instance;

const char * ResMgrEmulatorLRU1Test::SzName() const         { return "ResMgrEmulatorLRU1Test"; };
const char * ResMgrEmulatorLRU1Test::SzDescription() const  { return "LRU1 tests for PageEvictionEmulator."; }
bool ResMgrEmulatorLRU1Test::FRunUnderESE98() const         { return true; }
bool ResMgrEmulatorLRU1Test::FRunUnderESENT() const         { return true; }
bool ResMgrEmulatorLRU1Test::FRunUnderESE97() const         { return true; }


//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrSuperColdYesLRU1Test_() );
    TestCall( ErrSuperColdNoLRU1Test_() );
    TestCall( ErrCachePriorityYesLRU1Test_() );
    TestCall( ErrCachePriorityNoLRU1Test_() );
    TestCall( ErrEvictNextOnShrinkYesLRU1Test_() );
    TestCall( ErrEvictNextOnShrinkNoLRU1Test_() );
    TestCall( ErrSuperColdYesLRU1ESE_() );
    TestCall( ErrSuperColdNoLRU1ESE_() );
    TestCall( ErrCachePriorityYesLRU1ESE_() );
    TestCall( ErrCachePriorityNoLRU1ESE_() );

HandleError:
    return err;
}

//  ================================================================
BFTRACE* ResMgrEmulatorLRU1Test::RgBFTracePrepareSuperColdTest_( const bool fReplaySuperCold )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 101 pages; (102)
    //  - Supercold 2nd half of the pages; (152)
    //  - Evict/purge first page; (153)
    //  - Evict/scavenge 1st or 2nd half of the pages; (203)
    //  - Evict/purge 1st or 2nd half of the pages; (253)
    //  - Term; (254)
    //  - Sentinel. (255)

    BFTRACE* const rgbftrace = new BFTRACE[ 255 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }
    
    memset( rgbftrace, 0, 255 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1000;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    BFTRACE::BFSysResMgrInit_* pbfinit = &rgbftrace[ iTrace ].bfinit;
    pbfinit->K = 1;
    pbfinit->csecCorrelatedTouch = 0.001;
    pbfinit->csecTimeout = 1000.0;
    pbfinit->csecUncertainty = 1.0;
    pbfinit->dblHashLoadFactor = 5.0;
    pbfinit->dblHashUniformity = 1.0;
    pbfinit->dblSpeedSizeTradeoff = 0.0;
    iTrace++;
    tick += 2000;

    //  - Cache 101 pages; (102)

    for ( PGNO pgno = 0; iTrace < 102; iTrace++ )
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

    //  - Supercold 2nd half of the pages; (152)

    for ( PGNO pgno = 51; iTrace < 152; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidSuperCold;
        BFTRACE::BFSuperCold_* pbfsupercold = &rgbftrace[ iTrace ].bfsupercold;
        pbfsupercold->ifmp = pgno % 2;
        pbfsupercold->pgno = pgno;
        pgno++;
        tick += 2000;
    }

    //  - Evict/purge first page; (153)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 0 % 2;
    pbfevict->pgno = 0;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonPurgePage;
    iTrace++;
    tick += 2000;

    //  - Evict/scavenge 1st or 2nd half of the pages; (203)

    for ( PGNO pgno = fReplaySuperCold ? 51 : 1; iTrace < 203; iTrace++ )
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

    //  - Evict/purge 1st or 2nd half of the pages; (253)

    for ( PGNO pgno = fReplaySuperCold ? 1 : 51; iTrace < 253; iTrace++ )
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

    //  - Term; (254)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (255)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorLRU1Test::RgBFTracePrepareSuperColdTestOrderNotGuranteed_()
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 101 pages; (102)
    //  - Supercold 2nd half of the pages; (152)
    //  - Evict/purge first page; (153)
    //  - Evict/scavenge 2nd half of the pages; (203)
    //  - Touch 1st half of the pages; (253)
    //  - Evict/purge 1st half of the pages; (303)
    //  - Term; (304)
    //  - Sentinel. (305)

    BFTRACE* const rgbftrace = new BFTRACE[ 305 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }
    
    memset( rgbftrace, 0, 305 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1000;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    BFTRACE::BFSysResMgrInit_* pbfinit = &rgbftrace[ iTrace ].bfinit;
    pbfinit->K = 1;
    pbfinit->csecCorrelatedTouch = 0.001;
    pbfinit->csecTimeout = 1000.0;
    pbfinit->csecUncertainty = 1.0;
    pbfinit->dblHashLoadFactor = 5.0;
    pbfinit->dblHashUniformity = 1.0;
    pbfinit->dblSpeedSizeTradeoff = 0.0;
    iTrace++;
    tick += 2000;

    //  - Cache 101 pages; (102)

    for ( PGNO pgno = 0; iTrace < 102; iTrace++ )
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

    //  - Supercold 2nd half of the pages; (152)

    for ( PGNO pgno = 51; iTrace < 152; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidSuperCold;
        BFTRACE::BFSuperCold_* pbfsupercold = &rgbftrace[ iTrace ].bfsupercold;
        pbfsupercold->ifmp = pgno % 2;
        pbfsupercold->pgno = pgno;
        pgno++;
        tick += 2000;
    }

    //  - Evict/purge first page; (153)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidEvict;
    BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
    pbfevict->ifmp = 0 % 2;
    pbfevict->pgno = 0;
    pbfevict->fCurrentVersion = fTrue;
    pbfevict->pctPri = 100;
    pbfevict->bfef = bfefReasonPurgePage;
    iTrace++;
    tick += 2000;

    //  - Evict/scavenge 2nd half of the pages; (203)

    for ( PGNO pgno = 51; iTrace < 203; iTrace++ )
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

    //  - Touch 1st half of the pages; (253)

    for ( PGNO pgno = 1; iTrace < 253; iTrace++ )
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

    //  - Evict/purge 1st half of the pages; (303)

    for ( PGNO pgno = 1; iTrace < 303; iTrace++ )
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

    //  - Term; (304)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (305)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorLRU1Test::RgBFTracePrepareCachePriorityTest_( const bool fReplayCachePriority )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 101 pages, pgno 50 page with a cache priority of 15; (102)
    //  - Touch pgno 50 with a cache priority of 15; (103)
    //  - Evict/scavenge all pages; (204)
    //  - Term; (205)
    //  - Sentinel. (206)

    BFTRACE* const rgbftrace = new BFTRACE[ 206 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }
    
    memset( rgbftrace, 0, 206 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    BFTRACE::BFSysResMgrInit_* pbfinit = &rgbftrace[ iTrace ].bfinit;
    pbfinit->K = 1;
    pbfinit->csecCorrelatedTouch = 0.001;
    pbfinit->csecTimeout = 1000.0;
    pbfinit->csecUncertainty = 1.0;
    pbfinit->dblHashLoadFactor = 5.0;
    pbfinit->dblHashUniformity = 1.0;
    pbfinit->dblSpeedSizeTradeoff = 0.0;
    iTrace++;

    //  - Cache 101 pages, pgno 50 page with a cache priority of 15; (102)

    for ( PGNO pgno = 0; iTrace < 102; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = pgno == 50 ? 15 : 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick += 1500;
    }

    //  - Touch the pgno 50 with a cache priority of 15; (103)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidTouch;
    BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
    pbftouch->ifmp = 50 % 2;
    pbftouch->pgno = 50;
    pbftouch->pctPri = 15;
    pbftouch->fUseHistory = fTrue;
    iTrace++;
    tick += 1500;

    //  - Evict/scavenge all pages; (204)

    PGNO rgpgno[ 101 ] = { 0 };

    for ( PGNO ipg = 0; ipg < 16; ipg++ )
    {
        rgpgno[ ipg ] = ipg;
    }

    if ( fReplayCachePriority )
    {
        rgpgno[ 16 ] = 50;
        for ( PGNO ipg = 17; ipg < 51; ipg++ )
        {
            rgpgno[ ipg ] = ipg - 1;
        }
        for ( PGNO ipg = 51; ipg < 101; ipg++ )
        {
            rgpgno[ ipg ] = ipg;
        }
    }
    else
    {
        for ( PGNO ipg = 16; ipg < 50; ipg++ )
        {
            rgpgno[ ipg ] = ipg;
        }
        for ( PGNO ipg = 50; ipg < 100; ipg++ )
        {
            rgpgno[ ipg ] = ipg + 1;
        }
        rgpgno[ 100 ] = 50;
    }

    for ( PGNO ipg = 0; iTrace < 204; iTrace++ )
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

    //  - Term; (205)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (206)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
BFTRACE* ResMgrEmulatorLRU1Test::RgBFTracePrepareEvictNextOnShrinkTest_( const bool fEvictNextOnShrink )
//  ================================================================
{
    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/shrink all pages; (201)
    //  - Term; (202)
    //  - Sentinel. (203)

    BFTRACE* const rgbftrace = new BFTRACE[ 203 ];

    if ( rgbftrace == NULL )
    {
        return NULL;
    }
    
    memset( rgbftrace, 0, 203 * sizeof(BFTRACE) );
    size_t iTrace = 0;
    TICK tick = 1000;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    BFTRACE::BFSysResMgrInit_* pbfinit = &rgbftrace[ iTrace ].bfinit;
    pbfinit->K = 1;
    pbfinit->csecCorrelatedTouch = 0.001;
    pbfinit->csecTimeout = 1000.0;
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

    //  - Evict/shrink all pages; (201)

    for ( PGNO pgno = fEvictNextOnShrink ? 1 : 100; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidEvict;
        BFTRACE::BFEvict_* pbfevict = &rgbftrace[ iTrace ].bfevict;
        pbfevict->ifmp = pgno % 2;
        pbfevict->pgno = pgno;
        pbfevict->fCurrentVersion = fTrue;
        pbfevict->pctPri = 100;
        pbfevict->bfef = bfefReasonShrink;
        if ( fEvictNextOnShrink )
        {
            pgno++;
        }
        else
        {
            pgno--;
        }
        tick += 2000;
    }

    //  - Term; (202)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (203)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    return rgbftrace;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrSuperColdYesLRU1Test_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTest_( true );
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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 101 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 50 );
    TestCheck( stats.cSuperColdedSim == 50 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 506000 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 202000 );
    TestCheck( stats.histoLifetimeReal.Max() == 402000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 302000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 302000 );
    TestCheck( stats.histoLifetimeSim.Max() == 302000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 302000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrSuperColdNoLRU1Test_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTest_( false );
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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 101 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 50 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 506000 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 302000 );
    TestCheck( stats.histoLifetimeReal.Max() == 302000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 302000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 302000 );
    TestCheck( stats.histoLifetimeSim.Max() == 302000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 302000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrCachePriorityYesLRU1Test_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 102 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 1 );
    TestCheck( stats.cTouchesSim == 1 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 1 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 304500 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 102000 );
    TestCheck( stats.histoLifetimeReal.Max() == 154500 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 153000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 102000 );
    TestCheck( stats.histoLifetimeSim.Max() == 154500 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 153000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrCachePriorityNoLRU1Test_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 102 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 1 );
    TestCheck( stats.cTouchesSim == 1 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 1 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 304500 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 151500 );
    TestCheck( stats.histoLifetimeReal.Max() == 228000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 153000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 151500 );
    TestCheck( stats.histoLifetimeSim.Max() == 228000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 153000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrEvictNextOnShrinkYesLRU1Test_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareEvictNextOnShrinkTest_( true );
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
    TestCheck( stats.cRequested == 100 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 402000 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 200000 );
    TestCheck( stats.histoLifetimeReal.Max() == 200000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 200000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 200000 );
    TestCheck( stats.histoLifetimeSim.Max() == 200000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 200000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrEvictNextOnShrinkNoLRU1Test_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUTest algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareEvictNextOnShrinkTest_( false );
    TestCheck( rgbftrace != NULL );

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
    TestCheck( stats.cRequested == 100 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 100 );
    TestCheck( stats.cFaultsSim == 100 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 100 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cEvictionsReal == 100 );
    TestCheck( stats.cEvictionsSim == 100 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 402000 );
    TestCheck( stats.histoFaultsReal.C() == 100 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 100 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 100 );
    TestCheck( stats.histoLifetimeReal.Min() == 2000 );
    TestCheck( stats.histoLifetimeReal.Max() == 398000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 200000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 100 );
    TestCheck( stats.histoLifetimeSim.Min() == 2000 );
    TestCheck( stats.histoLifetimeSim.Max() == 398000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 200000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrSuperColdYesLRU1ESE_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTestOrderNotGuranteed_();
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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 151 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 50 );
    TestCheck( stats.cTouchesSim == 50 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 50);
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 50 );
    TestCheck( stats.cSuperColdedSim == 50 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 606000 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 202000 );
    TestCheck( stats.histoLifetimeReal.Max() == 502000 );
    TestCheck( stats.histoLifetimeReal.DblAve() >= 351504.950 && stats.histoLifetimeReal.DblAve() < 351504.951 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrSuperColdNoLRU1ESE_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmLRUKESE algorithm;

    BFTRACE* const rgbftrace = RgBFTracePrepareSuperColdTest_( false );
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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 101 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 0 );
    TestCheck( stats.cTouchesSim == 0 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 0 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 50 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 506000 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 302000 );
    TestCheck( stats.histoLifetimeReal.Max() == 302000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 302000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 302000 );
    TestCheck( stats.histoLifetimeSim.Max() == 302000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 302000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrCachePriorityYesLRU1ESE_()
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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 102 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 1 );
    TestCheck( stats.cTouchesSim == 1 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 1 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 304500 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 102000 );
    TestCheck( stats.histoLifetimeReal.Max() == 154500 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 153000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 102000 );
    TestCheck( stats.histoLifetimeSim.Max() == 154500 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 153000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

//  ================================================================
ERR ResMgrEmulatorLRU1Test::ErrCachePriorityNoLRU1ESE_()
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

    TestCheck( stats.cpgCachedMax == 101 );
    TestCheck( stats.cRequestedUnique == 101 );
    TestCheck( stats.cRequested == 102 );
    TestCheck( stats.cResMgrCycles == 1 );
    TestCheck( stats.cResMgrAbruptCycles == 0 );
    TestCheck( stats.cDiscardedTraces == 0 );
    TestCheck( stats.cOutOfRangeTraces == 0 );
    TestCheck( stats.cFaultsReal == 101 );
    TestCheck( stats.cFaultsSim == 101 );
    TestCheck( stats.cTouchesReal == 1 );
    TestCheck( stats.cTouchesSim == 1 );
    TestCheck( stats.cCaches == 101 );
    TestCheck( stats.cTouches == 1 );
    TestCheck( stats.cEvictionsReal == 101 );
    TestCheck( stats.cEvictionsSim == 101 );
    TestCheck( stats.cSuperColdedReal == 0 );
    TestCheck( stats.cSuperColdedSim == 0 );
    TestCheck( stats.cCachesTurnedTouch == 0 );
    TestCheck( stats.cTouchesTurnedCache == 0 );
    TestCheck( stats.cEvictionsFailed == 0 );
    TestCheck( stats.cEvictionsPurge == 0 );
    TestCheck( stats.dtickDurationReal == 304500 );
    TestCheck( stats.histoFaultsReal.C() == 101 );
    TestCheck( stats.histoFaultsReal.Min() == 1 );
    TestCheck( stats.histoFaultsReal.Max() == 1 );
    TestCheck( stats.histoFaultsReal.DblAve() == 1.000 );
    TestCheck( stats.histoFaultsSim.C() == 101 );
    TestCheck( stats.histoFaultsSim.Min() == 1 );
    TestCheck( stats.histoFaultsSim.Max() == 1 );
    TestCheck( stats.histoFaultsSim.DblAve() == 1.000 );
    TestCheck( stats.histoLifetimeReal.C() == 101 );
    TestCheck( stats.histoLifetimeReal.Min() == 151500 );
    TestCheck( stats.histoLifetimeReal.Max() == 228000 );
    TestCheck( stats.histoLifetimeReal.DblAve() == 153000.000 );
    TestCheck( stats.histoLifetimeSim.C() == 101 );
    TestCheck( stats.histoLifetimeSim.Min() == 151500 );
    TestCheck( stats.histoLifetimeSim.Max() == 228000 );
    TestCheck( stats.histoLifetimeSim.DblAve() == 153000.000 );

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );
    delete[] rgbftrace;

    return err;
}

