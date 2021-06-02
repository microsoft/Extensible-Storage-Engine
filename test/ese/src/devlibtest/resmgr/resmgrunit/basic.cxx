// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "resmgrunittest.hxx"

#include <tchar.h>
#include "sync.hxx"
#include "collection.hxx"

#ifdef DEBUG
#define OnDebug( x )        x
#else
#define OnDebug( x )
#endif

#ifndef RTM
#define OnNonRTM( x )       x
#else
#define OnNonRTM( x )
#endif

//  Helper functions.

static DWORD g_tickNow = 0;
static void SetTickCount( const DWORD tickNow )
{
    g_tickNow = tickNow;
}

static void IncrementTickCount( const DWORD dtick )
{
    g_tickNow += dtick;
}

static DWORD TickGetTickCount()
{
    return g_tickNow;
}

//  Redefining macros.

#define TickRESMGRTimeCurrent       TickGetTickCount
#define ENABLE_JET_UNIT_TEST        1

#include "resmgr.hxx" 

// forward decls
struct RSKEY;
struct RandomStruct;
const LONG      Kmax                    = 2;        //  maximum K for LRUK

//  ================================================================
struct RSKEY                                        //  RSKEY -- Key
//  ================================================================
{
    RSKEY() {}
    RSKEY( LONG indexIn )
        :   index( indexIn )
    {
    }

    BOOL operator==( const RSKEY& rskey) const
    {
        return rskey.index == index;
    }

    BOOL operator!=( const RSKEY& rskey) const
    {
        return rskey.index != index;
    }

    const RSKEY& operator=( const RSKEY& rskey )
    {
        index = rskey.index;
        return *this;
    }

    ULONG_PTR Hash() const
    {
        return (ULONG_PTR)index;
    }

    LONG index;
};


//  ================================================================
struct RandomStruct 
//  ================================================================
{
    LONG lUniqueKey;
    LONG lLastScanPosition;

    static SIZE_T OffsetOfLRUKIC()
    { 
        return offsetof( RandomStruct, lrukic );
    }

    CLRUKResourceUtilityManager< Kmax, RandomStruct, OffsetOfLRUKIC, RSKEY >::CInvasiveContext lrukic;

    RandomStruct( const LONG lUniqueKeyNew ) : 
        lrukic()
    {
        lUniqueKey = lUniqueKeyNew;
    }

    RandomStruct() : 
        lrukic()
    {
        lUniqueKey = 0;
        lLastScanPosition = -1;
    }

    RSKEY& Key()
    {
        return *(RSKEY*)&lUniqueKey;
    }
};


// Declare the LRUK class first
DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( Kmax, RandomStruct, RandomStruct::OffsetOfLRUKIC, RSKEY, RSLRUK );


// Helper functions.

//  ================================================================
void Scan( RSLRUK *prslruk, RSKEY *prskeyCold, RSKEY *prskeyHot, INT *pcFound, INT cExpectedMax = 0 )
//  ================================================================
{
    ERR err = JET_errSuccess;
    RSKEY rskeyUninit( -1 );
    RSLRUK::ERR     errLRUK;
    RSLRUK::CLock   lockLRUK;
    INT cKeys = 0;

    cExpectedMax = ( cExpectedMax == 0 ) ? 4 : cExpectedMax;
    RSKEY* rgKeys = new RSKEY[ cExpectedMax ];

    *pcFound = 0;
    *prskeyCold = rskeyUninit;
    *prskeyHot = rskeyUninit;

    prslruk->BeginResourceScan( &lockLRUK );

    RandomStruct *prs;

    // Remember the first and last ones.
    while ( ( errLRUK = prslruk->ErrGetNextResource( &lockLRUK, &prs ) ) == RSLRUK::ERR::errSuccess )
    {
        prs->lLastScanPosition = *pcFound;

        // Make sure we don't get the same resource more than once.
        for ( INT iKey = 0; iKey < cKeys; iKey++ )
        {
            TestAssert( rgKeys[ iKey ] != prs->Key() );
        }
        TestAssert( cKeys < cExpectedMax );
        rgKeys[ cKeys ] = prs->Key();
        cKeys++;

        // First resource (coldest resrouce).
        if ( *pcFound == 0 )
        {
            *prskeyCold = prs->Key();
        }

        // Current resource (will be the hottest when we finish).
        *prskeyHot = prs->Key();
        ( *pcFound )++;
    }

    TestCheck( errLRUK == RSLRUK::ERR::errNoCurrentResource );
    TestCheck( prslruk->ErrGetNextResource( &lockLRUK, &prs ) == RSLRUK::ERR::errNoCurrentResource );
    
    prslruk->EndResourceScan( &lockLRUK );

HandleError:

    delete[] rgKeys;
    return; 
}


// This function is needed to allow DHT (nested inside LRUK) to compile. (possibly debug-only)

extern void __cdecl OSStrCbFormatA( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...)
{
    
    va_list alist;
    va_start( alist, szFormat );
    vsprintf( szBuffer, szFormat, alist );
    va_end( alist );

    TestAssert( strlen( szBuffer ) <= cbBuffer );
    return;

}


// Work item context.
struct WIContext
{
    LPTHREAD_START_ROUTINE pfn;
    HANDLE hEventWorkItem;
    RSLRUK* prslruk;
    RSLRUK::CLock* plock;
    RandomStruct* prs;
};

// Generic async work item.
static DWORD WINAPI AsyncWorkItem( LPVOID lpParameter )
{
    ERR err = JET_errSuccess;
    WIContext* const pcontext = (WIContext*)lpParameter;

    TestCallS( pcontext->pfn( pcontext ) );
    TestCheck( SetEvent( pcontext->hEventWorkItem ) != 0 );

HandleError:
    return ERROR_SUCCESS;
}

// Run asynchronously..
static void RunAsync( const LPTHREAD_START_ROUTINE pfn, RSLRUK* const prslruk, RSLRUK::CLock* const plock, RandomStruct* const prs )
{
    ERR err = JET_errSuccess;
    WIContext context;

    context.pfn = pfn;
    context.hEventWorkItem = CreateEvent( NULL, TRUE, FALSE, NULL );
    context.prslruk = prslruk;
    context.plock = plock;
    context.prs = prs;

    TestCheck( context.hEventWorkItem != NULL );
    TestCheck( ResetEvent( context.hEventWorkItem ) != 0 );
    TestCheck( QueueUserWorkItem( AsyncWorkItem, &context, WT_EXECUTEDEFAULT ) != 0 );
    TestCheck( WaitForSingleObject( context.hEventWorkItem, INFINITE ) == WAIT_OBJECT_0 );
    TestCheck( CloseHandle( context.hEventWorkItem ) != 0 );

HandleError:
    ;
}

ERR ErrFromLRUK( const RSLRUK::ERR err )
{
    switch ( err )
    {
        case RSLRUK::ERR::errSuccess:
            return JET_errSuccess;

        default:
            return JET_errInternalError;
    }
}

// Evict a resource.
static DWORD WINAPI EvictResource_( LPVOID lpParameter )
{
    ERR err = JET_errSuccess;
    WIContext* const pcontext = (WIContext*)lpParameter;

    TestCallS( ErrFromLRUK( pcontext->prslruk->ErrEvictResource( pcontext->prs->Key(), pcontext->prs, fTrue ) ) );

HandleError:
    return ERROR_SUCCESS;
}

// Asynchronously evict a resource.
static void EvictResource( RSLRUK* const prslruk, RandomStruct* const prs )
{
    RunAsync( EvictResource_, prslruk, NULL, prs );
}

// Asynchronously evict current resource.
static void EvictCurrentResource( RSLRUK* const prslruk, RSLRUK::CLock* const plock, RandomStruct* const prs )
{
    ERR err = JET_errSuccess;
    RandomStruct* prsT;

    TestCallS( ErrFromLRUK( prslruk->ErrGetCurrentResource( plock, &prsT ) ) );
    TestCheck( prsT == prs );
    TestCallS( ErrFromLRUK( prslruk->ErrEvictCurrentResource( plock, prs->Key(), fFalse ) ) );

HandleError:
    ;
}


// Simple LRU-K test.

CUnitTest( SimpleLrukTest, 0, "Simple LRU-K test." );

//  ================================================================
ERR SimpleLrukTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (basic) ...\n");

    // Create elements
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );

    double csecCorrelationInterval  = 0.5;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;
    double dblBoost                 = 100.0;

    // Choose a pause delta so that we aren't testing time gaps that fall into
    // the uncertainty window.
    const INT cmsecPause = (INT) ( 2 * 1000.0 * csecBFLRUKUncertainty );

    // Initialize the lruk
    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,                         // K
                          csecCorrelationInterval,      // correlation internval
                          dblBoost,                     // double-touch boost
                          csecBFLRUKUncertainty,        // bucketing for LRUK (Accuracy/memory trade-off)
                          dblBFHashLoadFactor,          // number of elements per hash-bucket (CPU/memory trade-off)
                          dblBFHashUniformity,          // FUTURE: what does this number do?
                          dblBFSpeedSizeTradeoff ) ) ); // 0 == Multiplies number of buckets by number of processors. 

    // Test RandomStruct and RSKey                       
    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    // Cache several resources spaced in time
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs1.Key(),     // Key
                                &rs1,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    IncrementTickCount( cmsecPause );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),     // Key
                                &rs2,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    IncrementTickCount( cmsecPause );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs3.Key(),     // Key
                                &rs3,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    // Cold to Hot should be RS1, RS2, RS3
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    // Sanity check that stuff didn't move around
    // NB: Can't sleep here because we want to test a non-correlated touch next
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    // Touch resources to move things around

    // This touch should be correlated (thus ignored); RS1, RS2, RS3
    rmtf = rslruk.RmtfTouchResource( &rs3,  // Object
                           100 );           // pctCachePriority
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rmtf == RSLRUK::kNoTouch );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    // This touch should definitely be a regular correlated (instead of a no-touch)
    IncrementTickCount( (DWORD)( 1000.0 * csecCorrelationInterval ) / 5 );
    rmtf = rslruk.RmtfTouchResource( &rs3,  // Object
                           100 );           // pctCachePriority
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    // This touch will be considered a double touch; RS1, RS3, RS2
    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    rmtf = rslruk.RmtfTouchResource( &rs2,  // Object
                           100 );           // pctCachePriority
    TestCheck( rmtf == RSLRUK::k2Touch );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 3 );

    // This touch will also be considered a double touch
    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    rmtf = rslruk.RmtfTouchResource( &rs1,  // Object
                           100 );           // pctCachePriority

    // One would think that Cold to Hot should be RS3, RS2, RS1 because
    // we've touched RS1 more recently than RS2. However, LRU-2 gives the
    // boost not to the most recent touch time, but rather to previous last
    // touch. 
    
    TestCheck( rmtf == RSLRUK::k2Touch );
    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 3 );

    // Now we touch RS2 and RS1 again; now it should be RS3, RS2, RS1
    rmtf = rslruk.RmtfTouchResource( &rs2,  // Object
                           100 );           // pctCachePriority
    TestCheck( rmtf == RSLRUK::k2Touch );

    rmtf = rslruk.RmtfTouchResource( &rs1,  // Object
                           100 );           // pctCachePriority
    TestCheck( rmtf == RSLRUK::k2Touch );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    // Punt RS3; now it should be RS2, RS1, RS3
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs3, (DWORD)( 2 * dblBoost * 1000 ) );
    rslruk.EndResourceScan( &lockLRUK );
    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

HandleError:
    return err;
}


// LRU-K test with resources touched with different cache priorities.

CUnitTest( LrukDifferentPrioritiesTest, 0, "LRU-K test with resources touched with different cache priorities." );

//  ================================================================
ERR LrukDifferentPrioritiesTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (different cache priorities) ...\n");

    // Create elements.
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    // Initialize the lruk.
    SetTickCount( 0 );  // t = 0 sec
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    // Test RandomStruct and RSKey.                  
    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs3.Key() == rs3.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key() != rs3.Key() );
    TestCheck( rs2.Key() != rs3.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );
    TestCheck( rs3.Key().Hash() == rs3.Key().Hash() );

    // Cache resources spaced in time with different cache priorities.
    IncrementTickCount( 100 * 1000 );   // t = 100 sec
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );

    IncrementTickCount( 100 * 1000 );   // t = 200 sec
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 20 ) ) );

    IncrementTickCount( 100 * 1000 );   // t = 300 sec
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs3.Key(), &rs3, TickGetTickCount(), 4 ) ) );

    IncrementTickCount( 100 * 1000 );   // t = 400 sec

    // At this point (index position in seconds): RS3(12), RS2(40), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs3, 4 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 100 * 1000 );   // t = 500 sec

    // At this point (index position in seconds): RS2(40), RS3(52), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs2, 20 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 100 * 1000 );   // t = 600 sec

    // At this point (index position in seconds): RS3(52), RS1(100), RS2(240).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 100 * 1000 );   // t = 70 sec

    // At this point (index position in seconds): RS3(52), RS2(240), RS1(1100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

HandleError:
    return err;
}


// LRU-K test with resources touched with different cache priorities and correlated touches.

CUnitTest( LrukDifferentPrioritiesCorrelatedTouchesTest, 0, "LRU-K test with resources touched with different cache priorities and correlated touches." );

//  ================================================================
ERR LrukDifferentPrioritiesCorrelatedTouchesTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RandomStruct* prs = NULL;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (different cache priorities and correlated touches) ...\n");

    // Create elements.
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    // Initialize the lruk.
    SetTickCount( 0 );  // t = 0 sec
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    // Test RandomStruct and RSKey.
    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs3.Key() == rs3.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key() != rs3.Key() );
    TestCheck( rs2.Key() != rs3.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );
    TestCheck( rs3.Key().Hash() == rs3.Key().Hash() );

    IncrementTickCount( 100 * 1000 );   // t = 100 sec

    // Cache resources with different cache priorities.
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 50 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs3.Key(), &rs3, TickGetTickCount(), 40 ) ) );

    IncrementTickCount( 5 * 1000 ); // t = 105 sec

    // At this point (index position in seconds): RS3(40), RS2(50), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    // Correlated touches.
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs3, 40 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 4 * 1000 ); // t = 109 sec

    // At this point (index position in seconds): RS3(40), RS2(50), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    // Evict current resource to force cache priority baseline recalculation.
    rslruk.BeginResourceScan( &lockLRUK );
    TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
    TestCheck( prs == &rs3 );
    EvictCurrentResource( &rslruk, &lockLRUK, &rs3 );
    rslruk.EndResourceScan( &lockLRUK );

    // At this point (index position in seconds): RS2(50), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Still within the correlation window (up to 110 sec), so touches should still be correlated.
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 111 sec

    // At this point (index position in seconds): RS2(50), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Touch RS2 out of correlation window.
    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 ); // t = 113 sec

    // At this point (index position in seconds): RS1(100), RS2(550).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    // Touch RS1 out of correlation window.
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 ); // t = 113 sec

    // At this point (index position in seconds): RS2(550), RS1(1100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}


// LRU-K test with resources touched with mixed cache priorities against the same resource.

CUnitTest( LrukMixedCachePrioritiesTest, 0, "LRU-K test with resources touched with mixed cache priorities against the same resource." );

//  ================================================================
ERR LrukMixedCachePrioritiesTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (resources touched with mixed cache priorities against the same resource) ...\n");

    // Create elements.
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    // Initialize the lruk.
    SetTickCount( 0 );  // t = 0 sec
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    // Test RandomStruct and RSKey.
    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    IncrementTickCount( 100 * 1000 );   // t = 100 sec

    // Cache resources spaced in time with different cache priorities.
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );
    IncrementTickCount( 2 * 1000 ); // t = 102 sec

    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 50 ) ) );
    IncrementTickCount( 2 * 1000 ); // t = 104 sec

    IncrementTickCount( 2 * 1000 ); // t = 106 sec

    // At this point (index position in seconds): RS2(51), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches.
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 1 * 1000 ); // t = 107 sec

    // At this point (index position in seconds): RS2(51), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );

    // Still within the correlation window (up to 110 sec), so touches should still be correlated, even though we're changing priorities.
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 109 sec

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 4 * 1000 ); // t = 113 sec

    // At this point (index position in seconds): RS1(100), RS2(102).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    // Touch RS1 out of correlation window.
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 ); // t = 115 sec

    // At this point (index position in seconds): RS2(102), RS1(1107).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Touch RS2 out of correlation window.
    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 ); // t = 117 sec

    // At this point (index position in seconds):  RS1(1107), RS2(1109).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}


// LRU-K test with resources touched with mixed cache priorities against the same resource, raising priority for a resource.

CUnitTest( LrukMixedCachePrioritiesRaisePriorityTest, 0, "LRU-K test with resources touched with mixed cache priorities against the same resource, raising priority for a resource." );

//  ================================================================
ERR LrukMixedCachePrioritiesRaisePriorityTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (resources touched with mixed cache priorities against the same resource, raising priority for a resource) ...\n");

    // Create elements.
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    // Initialize the lruk.
    SetTickCount( 0 );  // t = 0 sec
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    // Test RandomStruct and RSKey.
    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    SetTickCount( 100 * 1000 ); // t = 100 sec

    // Cache resources with different cache priorities.
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 60 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 30 ) ) );

    IncrementTickCount( 2 * 1000 ); // t = 102 sec

    // At this point (index position in seconds): RS2(30), RS1(60).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches (RS2 gets priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 60 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 104 sec

    // At this point (index position in seconds): RS2(50), RS1(60).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches (RS2 gets priority bump again).
    rmtf = rslruk.RmtfTouchResource( &rs1, 60 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 70 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 106 sec

    // At this point (index position in seconds): RS1(60), RS2(70).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches (both gets priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 90 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 80 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 108 sec

    // At this point (index position in seconds): RS2(80), RS1(90).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    SetTickCount( 112 * 1000 ); // t = 112 sec

    // Uncorrelated touches (no priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 90 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 80 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 ); // t = 114 sec

    // At this point (index position in seconds): RS2(880), RS1(990).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches (both get priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 95 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 116 sec

    // At this point (index position in seconds): RS1(950), RS2(1100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches (RS1 gets priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 110 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 118 sec

    // At this point (index position in seconds): RS2(1100), RS1(1210).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}


// LRU-K test with resources touched with mixed cache priorities against the same resource, lowering priority for a resource.

CUnitTest( LrukMixedCachePrioritiesLowerPriorityTest, 0, "LRU-K test with resources touched with mixed cache priorities against the same resource, lowering priority for a resource." );

//  ================================================================
ERR LrukMixedCachePrioritiesLowerPriorityTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (resources touched with mixed cache priorities against the same resource, lowering priority for a resource) ...\n");

    // Create elements.
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    // Initialize the lruk.
    SetTickCount( 0 );  // t = 0 sec
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    // Test RandomStruct and RSKey.
    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    SetTickCount( 100 * 1000 ); // t = 100 sec

    // Cache resources with different cache priorities.
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 90 ) ) );

    IncrementTickCount( 2 * 1000 ); // t = 102 sec

    // At this point (index position in seconds): RS2(90), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Correlated touches (RS1 gets priority loss).
    rmtf = rslruk.RmtfTouchResource( &rs1, 80 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 90 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 104 sec

    // At this point (index position in seconds): RS2(90), RS1(100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    SetTickCount( 112 * 1000 ); // t = 112 sec

    // Uncorrelated touches (both get priority loss).
    rmtf = rslruk.RmtfTouchResource( &rs1, 10 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 70 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 ); // t = 114 sec

    // At this point (index position in seconds): RS2(990), RS1(1100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Uncorrelated touches (both get priority loss).
    rmtf = rslruk.RmtfTouchResource( &rs1, 0 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 10 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 116 sec

    // At this point (index position in seconds): RS2(990), RS1(1100).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    // Uncorrelated touches (RS2 gets priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 110 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 118 sec

    // At this point (index position in seconds): RS1(1100), RS2(1210).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    // Uncorrelated touches (RS1 gets priority bump).
    rmtf = rslruk.RmtfTouchResource( &rs1, 120 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 110 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 ); // t = 120 sec

    // At this point (index position in seconds): RS2(1210), RS2(1320).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}

// Simple LRU-K test with delayed-eviction resources.

CUnitTest( SimpleLrukDelayedEvictionTest, 0, "Simple LRU-K test with delayed-eviction resources." );

//  ================================================================
ERR SimpleLrukDelayedEvictionTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RandomStruct* prs = NULL;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (delayed-eviction) ...\n");

    // Create elements
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );
    RandomStruct rs4( 0x40000 );

    double csecBFLRUKUncertainty    = 1.0;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    // Initialize the lruk
    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,                         // K
                          100.0,                        // correlation internval
                          1000,                         // double-touch boost
                          csecBFLRUKUncertainty,        // bucketing for LRUK (Accuracy/memory trade-off)
                          dblBFHashLoadFactor,          // number of elements per hash-bucket (CPU/memory trade-off)
                          dblBFHashUniformity,          // FUTURE: what does this number do?
                          dblBFSpeedSizeTradeoff ) ) ); // 0 == Multiplies number of buckets by number of processors. 

    IncrementTickCount( 50000 );    // t = 50s.

    // Cache RS3.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs3.Key(),     // Key
                                &rs3,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    IncrementTickCount( 10000 );    // t = 60s.

    // Cache RS1.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs1.Key(),     // Key
                                &rs1,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    // Punt RS3 to 105s and RS1 to 100s.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs3, 55000 );
    rslruk.PuntResource( &rs1, 40000 );
    rslruk.EndResourceScan( &lockLRUK );

    // Cold to Hot should be RS1(100s), RS3(105s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 2 );

    IncrementTickCount( 42000 );    // t = 102s.

    // Cache RS2.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),     // Key
                                &rs2,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    // Cold to Hot should still be RS1(100s), RS2(102s), RS3(105s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    // Evict RS1 via scavenging (updates oldest estimate).
    rslruk.BeginResourceScan( &lockLRUK );
    TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
    TestCheck( prs == &rs1 );
    EvictCurrentResource( &rslruk, &lockLRUK, &rs1 );
    rslruk.EndResourceScan( &lockLRUK );

    // Now, oldest estimate should be 100s.

    // Evict RS2 explicitly (doesn't update oldest estimate).
    EvictResource( &rslruk, &rs2 );

    IncrementTickCount( 100 );  // t = 102.1s.

    // Recaching RS2 (will be added to t = 110s).
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),     // Key
                                &rs2,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    // Cold to Hot should be RS3(105s), RS2(110s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    IncrementTickCount( 3000 ); // t = 105.1s.

    // Evict RS3 via scavenging (updates oldest estimate).
    rslruk.BeginResourceScan( &lockLRUK );
    TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
    TestCheck( prs == &rs3 );
    EvictCurrentResource( &rslruk, &lockLRUK, &rs3 );
    rslruk.EndResourceScan( &lockLRUK );

    // Now, oldest estimate should be 105s.

    IncrementTickCount( 1000 ); // t = 104s.

    // Cache RS4 (will be added to t = 115s).
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs4.Key(),     // Key
                                &rs4,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    // Cold to Hot should be RS2(110s), RS4(115s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs4.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}

// LRU-K test with delayed-eviction resources (max punt).

CUnitTest( LrukDelayedEvictionMaxPuntTest, 0, "LRU-K test with delayed-eviction resources (max punt)." );

//  ================================================================
ERR LrukDelayedEvictionMaxPuntTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;
    
    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (delayed-eviction, max punt) ...\n");

    // Create elements
    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );
    RandomStruct rs4( 0x40000 );

    double csecBFLRUKUncertainty    = 1.0;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;
    DWORD cmsecTimeout              = 1000000;

    // Initialize the lruk
    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,                             // K
                          100.0,                            // correlation internval
                          (double)(cmsecTimeout / 1000),    // double-touch boost
                          csecBFLRUKUncertainty,            // bucketing for LRUK (Accuracy/memory trade-off)
                          dblBFHashLoadFactor,              // number of elements per hash-bucket (CPU/memory trade-off)
                          dblBFHashUniformity,              // FUTURE: what does this number do?
                          dblBFSpeedSizeTradeoff ) ) );     // 0 == Multiplies number of buckets by number of processors. 

    IncrementTickCount( 10000 );    // t = 10s.

    // Cache RS1.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs1.Key(),     // Key
                                &rs1,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    IncrementTickCount( 2000 ); // t = 12s.

    // Cache RS2.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),     // Key
                                &rs2,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    IncrementTickCount( 2000 ); // t = 14s.

    // Cache RS3.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs3.Key(),     // Key
                                &rs3,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    IncrementTickCount( 2000 ); // t = 16s.

    // Cache RS4.
    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs4.Key(),     // Key
                                &rs4,           // Object
                                TickGetTickCount(),
                                 100 ) ) );     // pctCachePriority

    // Cold to hot should be RS1(10s), RS2(12s), RS3(14s), RS4(16s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs4.Key() );
    TestCheck( cFound == 4 );

    // Punt RS1.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );

    // Cold to hot should be RS2(12s), RS3(14s), RS4(16s), RS1(1010s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 4 );

    // Punt RS2, RS3, RS4.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs2, cmsecTimeout );
    rslruk.PuntResource( &rs3, cmsecTimeout );
    rslruk.PuntResource( &rs4, cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );

    // Cold to hot should be RS1(1010s), RS2(1012s), RS3(1014s), RS4(1016s).
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs4.Key() );
    TestCheck( cFound == 4 );

    // Punt everybody beyond the max punt timestamp (1000s beyond current time)
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, cmsecTimeout );
    rslruk.PuntResource( &rs2, cmsecTimeout );
    rslruk.PuntResource( &rs3, cmsecTimeout );
    rslruk.PuntResource( &rs4, cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );

    // Order is not deterministic.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( cFound == 4 );

    IncrementTickCount( cmsecTimeout ); // t = 1016s.

    // Staggered punt.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, ( 4 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs2, ( 3 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs3, ( 2 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs4, ( 1 * cmsecTimeout ) / 4 );
    rslruk.EndResourceScan( &lockLRUK );

    // Order is deterministic.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs4.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 4 );

    // Punt RS1 and RS4 to max.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs4, 2 * cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );    

    // Hottest is not deterministic.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( ( rskeyHot == rs1.Key() ) || ( rskeyHot == rs4.Key() ) );
    TestCheck( cFound == 4 );

    // Punt RS2 and RS3 to max.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs2, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs3, 2 * cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );    

    IncrementTickCount( cmsecTimeout ); // t = 2016s.

    // Staggered punt.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, ( 1 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs2, ( 1 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs3, ( 3 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs4, ( 3 * cmsecTimeout ) / 4 );
    rslruk.EndResourceScan( &lockLRUK );

    // Order is not deterministic.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( ( rskeyCold == rs1.Key() ) || ( rskeyCold == rs2.Key() ) );
    TestCheck( ( rskeyHot == rs3.Key() ) || ( rskeyHot == rs4.Key() ) );
    TestCheck( cFound == 4 );

    // Punt all to max.
    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs2, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs3, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs4, 2 * cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );    

    // Order is not deterministic.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( cFound == 4 );

HandleError:
    return err;
}

// LRU-K test with resources being moved to the stuck list in the approximate index.

CUnitTest( LrukStuckListDraining, 0, "LRU-K test with resources being moved to the stuck list in the approximate index." );

//  ================================================================
ERR LrukStuckListDraining::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
#ifdef DEBUG
    const INT cResources = 200;     // Wrap starts ar around 220 resources.
#else
    const INT cResources = 10000;
#endif
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RandomStruct* rgrs = new RandomStruct[ cResources ];
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (stuck list) ...\n");

    double csecBFLRUKUncertainty    = 1.0;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;
    DWORD cmsecTimeout              = 100000;
    DWORD cmsecUncertainty          = (DWORD)( csecBFLRUKUncertainty * 1000.0 );

    // Initialize the lruk
    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( 2,                                // K
                          0.128,                            // correlation internval
                          (double)( cmsecTimeout / 1000 ),  // double-touch boost
                          csecBFLRUKUncertainty,            // bucketing for LRUK (Accuracy/memory trade-off)
                          dblBFHashLoadFactor,              // number of elements per hash-bucket (CPU/memory trade-off)
                          dblBFHashUniformity,              // FUTURE: what does this number do?
                          dblBFSpeedSizeTradeoff ) ) );     // 0 == Multiplies number of buckets by number of processors. 

    IncrementTickCount( 10 * cmsecUncertainty );    // t = 10s.

    // Cache all resources, one on each bucket.
    for ( LONG rsid = 0; rsid < cResources; rsid++ )
    {
        IncrementTickCount( cmsecUncertainty + cmsecUncertainty / 10 );
        rgrs[ rsid ].lUniqueKey = rsid;
        TestCallS( ErrFromLRUK(
            rslruk.ErrCacheResource( rgrs[ rsid ].Key(),    // Key
                                     &rgrs[ rsid ],         // Object
                                     TickGetTickCount(),    // Timestamp
                                     100 ) ) );             // pctCachePriority
    }

    // Full scan, make sure we find exactly the same number of resources we put in.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound, cResources );
    TestCheck( cFound == cResources );

    // Exact order.
    for ( INT iResource = 0; iResource < cResources; iResource++ )
    {
        TestCheck( rgrs[ iResource ].lLastScanPosition == iResource );
    }

HandleError:
    delete[] rgrs;
    return err;
}

// LRU-K test with time advancing.

CUnitTest( LrukApproximateIndexExpansion, 0, "LRU-K test with time advancing." );

//  ================================================================
ERR LrukApproximateIndexExpansion::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    const INT cCycles = 100;
    const INT cResources = 200;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RandomStruct* rgrs = new RandomStruct[ cResources ];
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    RandomStruct* prs = NULL;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (index expansion) ...\n");

    double csecBFLRUKUncertainty    = 1.0;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;
    DWORD cmsecTimeout              = 100000;
    DWORD cmsecUncertainty          = (DWORD)( csecBFLRUKUncertainty * 1000.0 );

    // Initialize the lruk
    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( 2,                                // K
                          0.128,                            // correlation internval
                          (double)( cmsecTimeout / 1000 ),  // double-touch boost
                          csecBFLRUKUncertainty,            // bucketing for LRUK (Accuracy/memory trade-off)
                          dblBFHashLoadFactor,              // number of elements per hash-bucket (CPU/memory trade-off)
                          dblBFHashUniformity,              // FUTURE: what does this number do?
                          dblBFSpeedSizeTradeoff ) ) );     // 0 == Multiplies number of buckets by number of processors. 

    IncrementTickCount( 10 * cmsecUncertainty );    // t = 10s.

    // Cache all resources, one on each bucket.
    for ( LONG rsid = 0; rsid < cResources; rsid++ )
    {
        IncrementTickCount( cmsecUncertainty + cmsecUncertainty / 10 );
        rgrs[ rsid ].lUniqueKey = rsid;
        TestCallS( ErrFromLRUK(
            rslruk.ErrCacheResource( rgrs[ rsid ].Key(),    // Key
                                     &rgrs[ rsid ],         // Object
                                     TickGetTickCount(),    // Timestamp
                                     100 ) ) );             // pctCachePriority
    }

    // Scan, assert exact order.
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound, cResources );
    TestCheck( cFound == cResources );
    for ( INT iResource = 0; iResource < cResources; iResource++ )
    {
        TestCheck( rgrs[ iResource ].lLastScanPosition == iResource );
    }

    // Cycle multiple times, advancing time.
    for ( INT iCycle = 0; iCycle < cCycles; iCycle++ )
    {
        // Replace all resources with more recent ones.
        for ( LONG rsid = 0; rsid < cResources; rsid++ )
        {
            // Evict coldest.
            rslruk.BeginResourceScan( &lockLRUK );
            TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
            TestCheck( prs == &rgrs[ rsid ] );
            EvictCurrentResource( &rslruk, &lockLRUK, &rgrs[ rsid ] );
            rslruk.EndResourceScan( &lockLRUK );

            // Reset scan marker.
            rgrs[ rsid ].lLastScanPosition = -1;

            // Insert new resource.
            IncrementTickCount( cmsecUncertainty + cmsecUncertainty / 10 );
            TestCallS( ErrFromLRUK(
                rslruk.ErrCacheResource( rgrs[ rsid ].Key(),    // Key
                                         &rgrs[ rsid ],         // Object
                                         TickGetTickCount(),    // Timestamp
                                         100,                   // pctCachePriority
                                         fFalse ) ) );          // fUseHistory
        }

        // Scan, assert exact order.
        Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound, cResources );
        TestCheck( cFound == cResources );
        for ( INT iResource = 0; iResource < cResources; iResource++ )
        {
            TestCheck( rgrs[ iResource ].lLastScanPosition == iResource );
        }
    }

HandleError:
    delete[] rgrs;
    return err;
}

