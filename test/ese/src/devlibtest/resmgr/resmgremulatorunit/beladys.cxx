// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"

// Unit test class

//  ================================================================
class ResMgrEmulatorBeladysTest : public UNITTEST
//  ================================================================
{
    private:
        static ResMgrEmulatorBeladysTest s_instance;

    protected:
        ResMgrEmulatorBeladysTest() {}
    public:
        ~ResMgrEmulatorBeladysTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:
        ERR ErrUnitBasicBestNext_();
        ERR ErrUnitBasicWorstNext_();
        ERR ErrUnitMultipleCycles_();
        ERR ErrBasicBestNext_();
        ERR ErrBasicWorstNext_();
        ERR ErrMultipleCycles_();
};

ResMgrEmulatorBeladysTest ResMgrEmulatorBeladysTest::s_instance;

const char * ResMgrEmulatorBeladysTest::SzName() const          { return "ResMgrEmulatorBeladysTest"; };
const char * ResMgrEmulatorBeladysTest::SzDescription() const   { return "Bélády's-algorithm tests for PageEvictionEmulator."; }
bool ResMgrEmulatorBeladysTest::FRunUnderESE98() const          { return true; }
bool ResMgrEmulatorBeladysTest::FRunUnderESENT() const          { return true; }
bool ResMgrEmulatorBeladysTest::FRunUnderESE97() const          { return true; }

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    TestCall( ErrUnitBasicBestNext_() );
    TestCall( ErrUnitBasicWorstNext_() );
    TestCall( ErrUnitMultipleCycles_() );
    TestCall( ErrBasicBestNext_() );
    TestCall( ErrBasicWorstNext_() );
    TestCall( ErrMultipleCycles_() );
  
  
HandleError:
    return err;
}

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrUnitBasicBestNext_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CBeladysResourceUtilityManager<QWORD, TICK> beladys;
    TICK tick = 1000;

    TestCheck( ( beladys.ErrResetProcessing() == CBeladysResourceUtilityManager<QWORD, TICK>::errInvalidOperation ) );

    //  Init.

    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Evict last 50 pages.

    for ( QWORD pgno = 51; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrEvictResource( pgno ) );
    }

    //  Touch first 50 pages.

    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
    }

    //  Evict (scavenge-best) last 25 pages currently cached.

    for ( INT i = 1; i <= 25; i++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == 0 );
    }

    //  Touch first 25 pages.

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
    }

    //  Evict (scavenge-worst) last 25 pages currently cached.

    for ( INT i = 1; i <= 25; i++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == 0 );
    }

    //  Terminate and switch to processing mode.

    beladys.Term();
    TestCheckErr( beladys.ErrStartProcessing() );
    TestCheck( ( beladys.ErrStartProcessing() == CBeladysResourceUtilityManager<QWORD, TICK>::errInvalidOperation ) );

    //  Init.

    tick = 1000;
    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheck( ( beladys.ErrTouchResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict page 70 explicitly.

    TestCheckErr( beladys.ErrEvictResource( 70 ) );
    TestCheck( ( beladys.ErrEvictResource( 70 ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    //  Evict (scavenge-best) 49 pages (should evict last ones, except for pgno 70).

    for ( QWORD pgno = 100; pgno >= 51; pgno-- )
    {
        if ( pgno == 70 )
        {
            pgno--;
        }
        
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    TestCheckErr( beladys.ErrResetProcessing() );

    tick = 1000;
    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheck( ( beladys.ErrTouchResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict page 70 explicitly.

    TestCheckErr( beladys.ErrEvictResource( 70 ) );
    TestCheck( ( beladys.ErrEvictResource( 70 ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    //  Evict (scavenge-best) 49 pages (should evict last ones, except for pgno 70).

    for ( QWORD pgno = 100; pgno >= 51; pgno-- )
    {
        if ( pgno == 70 )
        {
            pgno--;
        }
        
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    //  Touch first 50 pages.

    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict (scavenge-best) 25 pages (should evict last ones currently cached).

    for ( QWORD pgno = 50; pgno >= 26; pgno-- )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    //  Touch first 25 pages.

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict (scavenge-best) 25 pages (should evict last ones currently cached).

    for ( QWORD pgno = 25; pgno >= 1; pgno-- )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    QWORD pgnoNext = (QWORD)-1;
    TestCheck( ( beladys.ErrEvictBestNextResource( &pgnoNext ) == CBeladysResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    beladys.Term();

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrUnitBasicWorstNext_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CBeladysResourceUtilityManager<QWORD, TICK> beladys;
    TICK tick = 1000;

    TestCheck( ( beladys.ErrResetProcessing() == CBeladysResourceUtilityManager<QWORD, TICK>::errInvalidOperation ) );

    //  Init.

    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Evict last 50 pages.

    for ( QWORD pgno = 51; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrEvictResource( pgno ) );
    }

    //  Touch first 50 pages.

    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
    }

    //  Evict (scavenge-best) last 25 pages currently cached.

    for ( INT i = 1; i <= 25; i++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == 0 );
    }

    //  Touch first 25 pages.

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
    }

    //  Evict (scavenge-worst) last 25 pages currently cached.

    for ( INT i = 1; i <= 25; i++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == 0 );
    }

    //  Terminate and switch to processing mode.

    beladys.Term();
    TestCheckErr( beladys.ErrStartProcessing() );
    TestCheck( ( beladys.ErrStartProcessing() == CBeladysResourceUtilityManager<QWORD, TICK>::errInvalidOperation ) );

    //  Init.

    tick = 1000;
    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheck( ( beladys.ErrTouchResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict page 20 explicitly.

    TestCheckErr( beladys.ErrEvictResource( 20 ) );
    TestCheck( ( beladys.ErrEvictResource( 20 ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    //  Evict (scavenge-worst) 49 pages (should evict first ones, except for pgno 20).

    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        if ( pgno == 20 )
        {
            pgno++;
        }
        
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    TestCheckErr( beladys.ErrResetProcessing() );

    tick = 1000;
    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheck( ( beladys.ErrTouchResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict page 20 explicitly.

    TestCheckErr( beladys.ErrEvictResource( 20 ) );
    TestCheck( ( beladys.ErrEvictResource( 20 ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );

    //  Evict (scavenge-worst) 49 pages (should evict first ones, except for pgno 20).

    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        if ( pgno == 20 )
        {
            pgno++;
        }
        
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    //  Cache first 50 pages.

    for ( QWORD pgno = 1; pgno <= 50; pgno++ )
    {
        TestCheck( ( beladys.ErrTouchResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict (scavenge-worst) 25 pages (should evict first ones currently cached).

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    //  Cache first 25 pages again.

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        TestCheck( ( beladys.ErrTouchResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceNotCached ) );
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
        TestCheck( ( beladys.ErrCacheResource( pgno, tick ) == CBeladysResourceUtilityManager<QWORD, TICK>::errResourceAlreadyCached ) );
    }

    //  Evict (scavenge-worst) all pages.

    for ( QWORD pgno = 51; pgno <= 100; pgno++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    for ( QWORD pgno = 26; pgno <= 50; pgno++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictWorstNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    QWORD pgnoNext = (QWORD)-1;
    TestCheck( ( beladys.ErrEvictWorstNextResource( &pgnoNext ) == CBeladysResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    beladys.Term();

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrUnitMultipleCycles_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );

    CBeladysResourceUtilityManager<QWORD, TICK> beladys;
    TICK tick = 1000;

    //  Init.

    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Touch first 25 pages.

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        TestCheckErr( beladys.ErrTouchResource( pgno, tick++ ) );
    }

    //  Term/re-init.

    beladys.Term();
    TestCheckErr( beladys.ErrInit() );

    //  Cache last 25 pages.

    for ( QWORD pgno = 76; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Terminate and switch to processing mode.

    beladys.Term();
    TestCheckErr( beladys.ErrStartProcessing() );

    //  Init.

    tick = 1000;
    TestCheckErr( beladys.ErrInit() );

    //  Cache 100 pages.

    for ( QWORD pgno = 1; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Evict (scavenge-best) all pages.

    for ( QWORD pgno = 100; pgno >= 1; pgno-- )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    QWORD pgnoNext = (QWORD)-1;
    TestCheck( ( beladys.ErrEvictBestNextResource( &pgnoNext ) == CBeladysResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

    //  Cache first 25 pages.

    for ( QWORD pgno = 1; pgno <= 25; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Term/re-init.

    beladys.Term();
    TestCheckErr( beladys.ErrInit() );

    //  Cache last 25 pages.

    for ( QWORD pgno = 76; pgno <= 100; pgno++ )
    {
        TestCheckErr( beladys.ErrCacheResource( pgno, tick++ ) );
    }

    //  Evict (scavenge-best) 25 pages.

    for ( QWORD pgno = 100; pgno >= 76; pgno-- )
    {
        QWORD pgnoNext = (QWORD)-1;
        TestCheckErr( beladys.ErrEvictBestNextResource( &pgnoNext ) );
        TestCheck( pgnoNext == pgno );
    }

    pgnoNext = (QWORD)-1;
    TestCheck( ( beladys.ErrEvictBestNextResource( &pgnoNext ) == CBeladysResourceUtilityManager<QWORD, TICK>::errNoCurrentResource ) );

HandleError:

    beladys.Term();

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrBasicBestNext_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );
    
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmBeladys algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge 25 first pages in reverse order; (126)
    //  - Evict/scavenge 25 last pages in reverse order; (151)
    //  - Touch 50 middle pages; (201)
    //  - Cache 25 last pages; (226)
    //  - Evict/scavenge 25 last pages in reverse order; (251)
    //  - Evict/scavenge 50 middle pages in reverse order; (301)
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
    tick++;

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
        tick++;
    }

    //  - Evict/scavenge 25 first pages in reverse order; (126)

    for ( PGNO pgno = 25; iTrace < 126; iTrace++ )
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
        tick++;
    }

    //  - Evict/scavenge 25 last pages in reverse order; (151)

    for ( PGNO pgno = 100; iTrace < 151; iTrace++ )
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
        tick++;
    }

    //  - Touch 50 middle pages; (201)

    for ( PGNO pgno = 26; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
        tick++;
    }

    //  - Cache 25 last pages; (226)

    for ( PGNO pgno = 76; iTrace < 226; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick++;
    }

    //  - Evict/scavenge 25 last pages in reverse order; (251)

    for ( PGNO pgno = 100; iTrace < 251; iTrace++ )
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
        tick++;
    }

    //  - Evict/scavenge 50 middle pages in reverse order; (301)

    for ( PGNO pgno = 75; iTrace < 301; iTrace++ )
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
        tick++;
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

    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Switch to processing mode.

    TestCheck( algorithm.FNeedsPreProcessing() );
    TestCall( algorithm.ErrStartProcessing() );

    //  Validation (do it twice to validate resetting).

    for ( INT i = 1; i <= 2; i++ )
    {
        emulator.Term();
        BFFTLTerm( pbfftlc );
        pbfftlc = NULL;

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
        TestCheck( stats.cRequested == 175 );
        TestCheck( stats.cResMgrCycles == 1 );
        TestCheck( stats.cResMgrAbruptCycles == 0 );
        TestCheck( stats.cDiscardedTraces == 0 );
        TestCheck( stats.cOutOfRangeTraces == 0 );
        TestCheck( stats.cFaultsReal == 125 );
        TestCheck( stats.cFaultsSim == 125 );
        TestCheck( stats.cTouchesReal == 50 );
        TestCheck( stats.cTouchesSim == 50 );
        TestCheck( stats.cCaches == 125 );
        TestCheck( stats.cTouches == 50 );
        TestCheck( stats.cEvictionsReal == 125 );
        TestCheck( stats.cEvictionsSim == 125 );
        TestCheck( stats.cCachesTurnedTouch == 0 );
        TestCheck( stats.cTouchesTurnedCache == 0 );
        TestCheck( stats.cEvictionsFailed == 0 );
        TestCheck( stats.cEvictionsPurge == 0 );
        TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );

        TestCall( algorithm.ErrResetProcessing() );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrBasicWorstNext_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );
    
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmBeladys algorithm( false );

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge 50 middle pages in order; (151)
    //  - Cache 50 middle pages; (201)
    //  - Touch 25 last pages; (226)
    //  - Evict/scavenge all pages in order; (326)
    //  - Term; (327)
    //  - Sentinel. (328)

    BFTRACE rgbftrace[ 328 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick++;

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
        tick++;
    }

    //  - Evict/scavenge 50 middle pages in order; (151)

    for ( PGNO pgno = 26; iTrace < 151; iTrace++ )
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
        tick++;
    }

    //  - Cache 50 middle pages; (201)

    for ( PGNO pgno = 26; iTrace < 201; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick++;
    }

    //  - Touch 25 last pages; (226)

    for ( PGNO pgno = 76; iTrace < 226; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidTouch;
        BFTRACE::BFTouch_* pbftouch = &rgbftrace[ iTrace ].bftouch;
        pbftouch->ifmp = pgno % 2;
        pbftouch->pgno = pgno;
        pbftouch->pctPri = 100;
        pbftouch->fUseHistory = fTrue;
        pgno++;
        tick++;
    }

    //  - Evict/scavenge all pages in order; (326)

    for ( PGNO pgno = 1; iTrace < 326; iTrace++ )
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
        tick++;
    }

    //  - Term; (327)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (328)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Switch to processing mode.

    TestCheck( algorithm.FNeedsPreProcessing() );
    TestCall( algorithm.ErrStartProcessing() );

    //  Validation (do it twice to validate resetting).

    for ( INT i = 1; i <= 2; i++ )
    {
        emulator.Term();
        BFFTLTerm( pbfftlc );
        pbfftlc = NULL;

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
        TestCheck( stats.cRequested == 175 );
        TestCheck( stats.cResMgrCycles == 1 );
        TestCheck( stats.cResMgrAbruptCycles == 0 );
        TestCheck( stats.cDiscardedTraces == 0 );
        TestCheck( stats.cOutOfRangeTraces == 0 );
        TestCheck( stats.cFaultsReal == 150 );
        TestCheck( stats.cFaultsSim == 150 );
        TestCheck( stats.cTouchesReal == 25 );
        TestCheck( stats.cTouchesSim == 25 );
        TestCheck( stats.cCaches == 150 );
        TestCheck( stats.cTouches == 25 );
        TestCheck( stats.cEvictionsReal == 150 );
        TestCheck( stats.cEvictionsSim == 150 );
        TestCheck( stats.cCachesTurnedTouch == 0 );
        TestCheck( stats.cTouchesTurnedCache == 0 );
        TestCheck( stats.cEvictionsFailed == 0 );
        TestCheck( stats.cEvictionsPurge == 0 );
        TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );

        TestCall( algorithm.ErrResetProcessing() );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

//  ================================================================
ERR ResMgrEmulatorBeladysTest::ErrMultipleCycles_()
//  ================================================================
{
    ERR err = JET_errSuccess;

    printf( "\t%s\r\n", __FUNCTION__ );
    
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFFTLContext* pbfftlc = NULL;
    PageEvictionAlgorithmBeladys algorithm;

    //  Scenario:
    //  - Init; (1)
    //  - Cache 100 pages; (101)
    //  - Evict/scavenge all pages in reverse order; (201)
    //  - Term; (202)
    //  - Init; (203)
    //  - Cache 75 last pages; (278)
    //  - Evict/scavenge all remaining last pages in reverse order; (353)
    //  - Term; (354)
    //  - Sentinel. (355)

    BFTRACE rgbftrace[ 355 ] = { 0 };
    size_t iTrace = 0;
    TICK tick = 1;
    const TICK tickBegin = tick;

    //  - Init; (1)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick++;

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
        tick++;
    }

    //  - Evict/scavenge all pages in reverse order; (201)

    for ( PGNO pgno = 100; iTrace < 201; iTrace++ )
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
        tick++;
    }

    //  - Term; (202)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;
    tick++;

    //  - Init; (203)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrInit;
    iTrace++;
    tick++;

    //  - Cache 75 last pages; (278)

    for ( PGNO pgno = 26; iTrace < 278; iTrace++ )
    {
        rgbftrace[ iTrace ].tick = tick;
        rgbftrace[ iTrace ].traceid = bftidCache;
        BFTRACE::BFCache_* pbfcache = &rgbftrace[ iTrace ].bfcache;
        pbfcache->ifmp = pgno % 2;
        pbfcache->pgno = pgno;
        pbfcache->pctPri = 100;
        pbfcache->fUseHistory = fTrue;
        pgno++;
        tick++;
    }

    //  - Evict/scavenge all remaining last pages in reverse order; (353)

    for ( PGNO pgno = 100; iTrace < 353; iTrace++ )
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
        tick++;
    }

    //  - Term; (354)

    rgbftrace[ iTrace ].tick = tick;
    rgbftrace[ iTrace ].traceid = bftidSysResMgrTerm;
    iTrace++;

    //  - Sentinel. (355)

    rgbftrace[ iTrace ].traceid = bftidInvalid;

    const TICK tickEnd = tick;

    //  Init driver.

    TestCall( ErrBFFTLInit( rgbftrace, fBFFTLDriverTestMode, &pbfftlc ) );

    //  Fill in database info.

    pbfftlc->cIFMP = 2;
    pbfftlc->rgpgnoMax[ 0 ] = 100;
    pbfftlc->rgpgnoMax[ 1 ] = 100;

    //  Init./run.

    TestCall( emulator.ErrInit( pbfftlc, &algorithm ) );
    TestCall( emulator.ErrExecute() );

    //  Switch to processing mode.

    TestCheck( algorithm.FNeedsPreProcessing() );
    TestCall( algorithm.ErrStartProcessing() );

    //  Validation (do it twice to validate resetting).

    for ( INT i = 1; i <= 2; i++ )
    {
        emulator.Term();
        BFFTLTerm( pbfftlc );
        pbfftlc = NULL;

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
        TestCheck( stats.cRequested == 175 );
        TestCheck( stats.cResMgrCycles == 2 );
        TestCheck( stats.cResMgrAbruptCycles == 0 );
        TestCheck( stats.cDiscardedTraces == 0 );
        TestCheck( stats.cOutOfRangeTraces == 0 );
        TestCheck( stats.cFaultsReal == 175 );
        TestCheck( stats.cFaultsSim == 175 );
        TestCheck( stats.cTouchesReal == 0 );
        TestCheck( stats.cTouchesSim == 0 );
        TestCheck( stats.cCaches == 175 );
        TestCheck( stats.cTouches == 0 );
        TestCheck( stats.cEvictionsReal == 175 );
        TestCheck( stats.cEvictionsSim == 175 );
        TestCheck( stats.cCachesTurnedTouch == 0 );
        TestCheck( stats.cTouchesTurnedCache == 0 );
        TestCheck( stats.cEvictionsFailed == 0 );
        TestCheck( stats.cEvictionsPurge == 0 );
        TestCheck( stats.dtickDurationReal == ( tickEnd - tickBegin ) );

        TestCall( algorithm.ErrResetProcessing() );
    }

HandleError:

    emulator.Term();
    BFFTLTerm( pbfftlc );

    return err;
}

