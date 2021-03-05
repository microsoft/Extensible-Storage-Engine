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


#define TickRESMGRTimeCurrent       TickGetTickCount
#define ENABLE_JET_UNIT_TEST        1

#include "resmgr.hxx" 

struct RSKEY;
struct RandomStruct;
const LONG      Kmax                    = 2;

struct RSKEY
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


struct RandomStruct 
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


DECLARE_LRUK_RESOURCE_UTILITY_MANAGER( Kmax, RandomStruct, RandomStruct::OffsetOfLRUKIC, RSKEY, RSLRUK );



void Scan( RSLRUK *prslruk, RSKEY *prskeyCold, RSKEY *prskeyHot, INT *pcFound, INT cExpectedMax = 0 )
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

    while ( ( errLRUK = prslruk->ErrGetNextResource( &lockLRUK, &prs ) ) == RSLRUK::ERR::errSuccess )
    {
        prs->lLastScanPosition = *pcFound;

        for ( INT iKey = 0; iKey < cKeys; iKey++ )
        {
            TestAssert( rgKeys[ iKey ] != prs->Key() );
        }
        TestAssert( cKeys < cExpectedMax );
        rgKeys[ cKeys ] = prs->Key();
        cKeys++;

        if ( *pcFound == 0 )
        {
            *prskeyCold = prs->Key();
        }

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



extern void __cdecl OSStrCbFormatA( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...)
{
    
    va_list alist;
    va_start( alist, szFormat );
    vsprintf( szBuffer, szFormat, alist );
    va_end( alist );

    TestAssert( strlen( szBuffer ) <= cbBuffer );
    return;

}


struct WIContext
{
    LPTHREAD_START_ROUTINE pfn;
    HANDLE hEventWorkItem;
    RSLRUK* prslruk;
    RSLRUK::CLock* plock;
    RandomStruct* prs;
};

static DWORD WINAPI AsyncWorkItem( LPVOID lpParameter )
{
    ERR err = JET_errSuccess;
    WIContext* const pcontext = (WIContext*)lpParameter;

    TestCallS( pcontext->pfn( pcontext ) );
    TestCheck( SetEvent( pcontext->hEventWorkItem ) != 0 );

HandleError:
    return ERROR_SUCCESS;
}

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

static DWORD WINAPI EvictResource_( LPVOID lpParameter )
{
    ERR err = JET_errSuccess;
    WIContext* const pcontext = (WIContext*)lpParameter;

    TestCallS( ErrFromLRUK( pcontext->prslruk->ErrEvictResource( pcontext->prs->Key(), pcontext->prs, fTrue ) ) );

HandleError:
    return ERROR_SUCCESS;
}

static void EvictResource( RSLRUK* const prslruk, RandomStruct* const prs )
{
    RunAsync( EvictResource_, prslruk, NULL, prs );
}

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



CUnitTest( SimpleLrukTest, 0, "Simple LRU-K test." );

ERR SimpleLrukTest::ErrTest()
{
    ERR err = JET_errSuccess;
    
    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (basic) ...\n");

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );

    double csecCorrelationInterval  = 0.5;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;
    double dblBoost                 = 100.0;

    const INT cmsecPause = (INT) ( 2 * 1000.0 * csecBFLRUKUncertainty );

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          dblBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs1.Key(),
                                &rs1,
                                TickGetTickCount(),
                                 100 ) ) );

    IncrementTickCount( cmsecPause );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),
                                &rs2,
                                TickGetTickCount(),
                                 100 ) ) );

    IncrementTickCount( cmsecPause );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs3.Key(),
                                &rs3,
                                TickGetTickCount(),
                                 100 ) ) );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );


    rmtf = rslruk.RmtfTouchResource( &rs3,
                           100 );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rmtf == RSLRUK::kNoTouch );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    IncrementTickCount( (DWORD)( 1000.0 * csecCorrelationInterval ) / 5 );
    rmtf = rslruk.RmtfTouchResource( &rs3,
                           100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    rmtf = rslruk.RmtfTouchResource( &rs2,
                           100 );
    TestCheck( rmtf == RSLRUK::k2Touch );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 3 );

    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    rmtf = rslruk.RmtfTouchResource( &rs1,
                           100 );

    
    TestCheck( rmtf == RSLRUK::k2Touch );
    IncrementTickCount( (DWORD)( 1200.0 * csecCorrelationInterval ) );
    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs2,
                           100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    rmtf = rslruk.RmtfTouchResource( &rs1,
                           100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

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



CUnitTest( LrukDifferentPrioritiesTest, 0, "LRU-K test with resources touched with different cache priorities." );

ERR LrukDifferentPrioritiesTest::ErrTest()
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (different cache priorities) ...\n");

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs3.Key() == rs3.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key() != rs3.Key() );
    TestCheck( rs2.Key() != rs3.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );
    TestCheck( rs3.Key().Hash() == rs3.Key().Hash() );

    IncrementTickCount( 100 * 1000 );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );

    IncrementTickCount( 100 * 1000 );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 20 ) ) );

    IncrementTickCount( 100 * 1000 );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs3.Key(), &rs3, TickGetTickCount(), 4 ) ) );

    IncrementTickCount( 100 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs3, 4 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 100 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs2, 20 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 100 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 100 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

HandleError:
    return err;
}



CUnitTest( LrukDifferentPrioritiesCorrelatedTouchesTest, 0, "LRU-K test with resources touched with different cache priorities and correlated touches." );

ERR LrukDifferentPrioritiesCorrelatedTouchesTest::ErrTest()
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

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs3.Key() == rs3.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key() != rs3.Key() );
    TestCheck( rs2.Key() != rs3.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );
    TestCheck( rs3.Key().Hash() == rs3.Key().Hash() );

    IncrementTickCount( 100 * 1000 );

    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 50 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs3.Key(), &rs3, TickGetTickCount(), 40 ) ) );

    IncrementTickCount( 5 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs3, 40 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 4 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 3 );

    rslruk.BeginResourceScan( &lockLRUK );
    TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
    TestCheck( prs == &rs3 );
    EvictCurrentResource( &rslruk, &lockLRUK, &rs3 );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}



CUnitTest( LrukMixedCachePrioritiesTest, 0, "LRU-K test with resources touched with mixed cache priorities against the same resource." );

ERR LrukMixedCachePrioritiesTest::ErrTest()
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (resources touched with mixed cache priorities against the same resource) ...\n");

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    IncrementTickCount( 100 * 1000 );

    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );
    IncrementTickCount( 2 * 1000 );

    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 50 ) ) );
    IncrementTickCount( 2 * 1000 );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 1 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 4 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}



CUnitTest( LrukMixedCachePrioritiesRaisePriorityTest, 0, "LRU-K test with resources touched with mixed cache priorities against the same resource, raising priority for a resource." );

ERR LrukMixedCachePrioritiesRaisePriorityTest::ErrTest()
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (resources touched with mixed cache priorities against the same resource, raising priority for a resource) ...\n");

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    SetTickCount( 100 * 1000 );

    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 60 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 30 ) ) );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 60 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 50 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 60 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 70 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 90 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 80 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    SetTickCount( 112 * 1000 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 90 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 80 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 95 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 110 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}



CUnitTest( LrukMixedCachePrioritiesLowerPriorityTest, 0, "LRU-K test with resources touched with mixed cache priorities against the same resource, lowering priority for a resource." );

ERR LrukMixedCachePrioritiesLowerPriorityTest::ErrTest()
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (resources touched with mixed cache priorities against the same resource, lowering priority for a resource) ...\n");

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );

    double csecCorrelationInterval  = 10.0;
    double csecMultiTouchBoost      = 1000.0;
    double csecBFLRUKUncertainty    = 0.1;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          csecCorrelationInterval,
                          csecMultiTouchBoost,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    TestCheck( rs1.Key() == rs1.Key() );
    TestCheck( rs2.Key() == rs2.Key() );
    TestCheck( rs1.Key() != rs2.Key() );
    TestCheck( rs1.Key().Hash() == rs1.Key().Hash() );
    TestCheck( rs2.Key().Hash() == rs2.Key().Hash() );

    SetTickCount( 100 * 1000 );

    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs1.Key(), &rs1, TickGetTickCount(), 100 ) ) );
    TestCallS( ErrFromLRUK( rslruk.ErrCacheResource( rs2.Key(), &rs2, TickGetTickCount(), 90 ) ) );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 80 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 90 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    SetTickCount( 112 * 1000 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 10 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 70 );
    TestCheck( rmtf == RSLRUK::k2Touch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 0 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 10 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 100 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 110 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    rmtf = rslruk.RmtfTouchResource( &rs1, 120 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    rmtf = rslruk.RmtfTouchResource( &rs2, 110 );
    TestCheck( rmtf == RSLRUK::k1CorrelatedTouch );

    IncrementTickCount( 2 * 1000 );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}


CUnitTest( SimpleLrukDelayedEvictionTest, 0, "Simple LRU-K test with delayed-eviction resources." );

ERR SimpleLrukDelayedEvictionTest::ErrTest()
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

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );
    RandomStruct rs4( 0x40000 );

    double csecBFLRUKUncertainty    = 1.0;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          100.0,
                          1000,
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    IncrementTickCount( 50000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs3.Key(),
                                &rs3,
                                TickGetTickCount(),
                                 100 ) ) );

    IncrementTickCount( 10000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs1.Key(),
                                &rs1,
                                TickGetTickCount(),
                                 100 ) ) );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs3, 55000 );
    rslruk.PuntResource( &rs1, 40000 );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 2 );

    IncrementTickCount( 42000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),
                                &rs2,
                                TickGetTickCount(),
                                 100 ) ) );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs3.Key() );
    TestCheck( cFound == 3 );

    rslruk.BeginResourceScan( &lockLRUK );
    TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
    TestCheck( prs == &rs1 );
    EvictCurrentResource( &rslruk, &lockLRUK, &rs1 );
    rslruk.EndResourceScan( &lockLRUK );


    EvictResource( &rslruk, &rs2 );

    IncrementTickCount( 100 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),
                                &rs2,
                                TickGetTickCount(),
                                 100 ) ) );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( rskeyHot == rs2.Key() );
    TestCheck( cFound == 2 );

    IncrementTickCount( 3000 );

    rslruk.BeginResourceScan( &lockLRUK );
    TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
    TestCheck( prs == &rs3 );
    EvictCurrentResource( &rslruk, &lockLRUK, &rs3 );
    rslruk.EndResourceScan( &lockLRUK );


    IncrementTickCount( 1000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs4.Key(),
                                &rs4,
                                TickGetTickCount(),
                                 100 ) ) );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs4.Key() );
    TestCheck( cFound == 2 );

HandleError:
    return err;
}


CUnitTest( LrukDelayedEvictionMaxPuntTest, 0, "LRU-K test with delayed-eviction resources (max punt)." );

ERR LrukDelayedEvictionMaxPuntTest::ErrTest()
{
    ERR err = JET_errSuccess;
    
    const INT rankRSLRUK = 70;
    RSLRUK rslruk( rankRSLRUK );
    RSLRUK::CLock lockLRUK;
    RSKEY rskeyCold, rskeyHot;
    RSLRUK::ResMgrTouchFlags rmtf;
    INT cFound = 0;

    wprintf( L"\tTesting LRUK (delayed-eviction, max punt) ...\n");

    RandomStruct rs1( 0x10000 );
    RandomStruct rs2( 0x20000 );
    RandomStruct rs3( 0x30000 );
    RandomStruct rs4( 0x40000 );

    double csecBFLRUKUncertainty    = 1.0;
    double dblBFHashLoadFactor      = 5.0;
    double dblBFHashUniformity      = 1.0;
    double dblBFSpeedSizeTradeoff   = 0.0;
    DWORD cmsecTimeout              = 1000000;

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( Kmax,
                          100.0,
                          (double)(cmsecTimeout / 1000),
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    IncrementTickCount( 10000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs1.Key(),
                                &rs1,
                                TickGetTickCount(),
                                 100 ) ) );

    IncrementTickCount( 2000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs2.Key(),
                                &rs2,
                                TickGetTickCount(),
                                 100 ) ) );

    IncrementTickCount( 2000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs3.Key(),
                                &rs3,
                                TickGetTickCount(),
                                 100 ) ) );

    IncrementTickCount( 2000 );

    TestCallS( ErrFromLRUK(
        rslruk.ErrCacheResource( rs4.Key(),
                                &rs4,
                                TickGetTickCount(),
                                 100 ) ) );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs4.Key() );
    TestCheck( cFound == 4 );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs2.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 4 );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs2, cmsecTimeout );
    rslruk.PuntResource( &rs3, cmsecTimeout );
    rslruk.PuntResource( &rs4, cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs1.Key() );
    TestCheck( rskeyHot == rs4.Key() );
    TestCheck( cFound == 4 );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, cmsecTimeout );
    rslruk.PuntResource( &rs2, cmsecTimeout );
    rslruk.PuntResource( &rs3, cmsecTimeout );
    rslruk.PuntResource( &rs4, cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( cFound == 4 );

    IncrementTickCount( cmsecTimeout );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, ( 4 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs2, ( 3 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs3, ( 2 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs4, ( 1 * cmsecTimeout ) / 4 );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs4.Key() );
    TestCheck( rskeyHot == rs1.Key() );
    TestCheck( cFound == 4 );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs4, 2 * cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );    

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( rskeyCold == rs3.Key() );
    TestCheck( ( rskeyHot == rs1.Key() ) || ( rskeyHot == rs4.Key() ) );
    TestCheck( cFound == 4 );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs2, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs3, 2 * cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );    

    IncrementTickCount( cmsecTimeout );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, ( 1 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs2, ( 1 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs3, ( 3 * cmsecTimeout ) / 4 );
    rslruk.PuntResource( &rs4, ( 3 * cmsecTimeout ) / 4 );
    rslruk.EndResourceScan( &lockLRUK );

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( ( rskeyCold == rs1.Key() ) || ( rskeyCold == rs2.Key() ) );
    TestCheck( ( rskeyHot == rs3.Key() ) || ( rskeyHot == rs4.Key() ) );
    TestCheck( cFound == 4 );

    rslruk.BeginResourceScan( &lockLRUK );
    rslruk.PuntResource( &rs1, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs2, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs3, 2 * cmsecTimeout );
    rslruk.PuntResource( &rs4, 2 * cmsecTimeout );
    rslruk.EndResourceScan( &lockLRUK );    

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound );
    TestCheck( cFound == 4 );

HandleError:
    return err;
}


CUnitTest( LrukStuckListDraining, 0, "LRU-K test with resources being moved to the stuck list in the approximate index." );

ERR LrukStuckListDraining::ErrTest()
{
    ERR err = JET_errSuccess;

    const INT rankRSLRUK = 70;
#ifdef DEBUG
    const INT cResources = 200;
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

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( 2,
                          0.128,
                          (double)( cmsecTimeout / 1000 ),
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    IncrementTickCount( 10 * cmsecUncertainty );

    for ( LONG rsid = 0; rsid < cResources; rsid++ )
    {
        IncrementTickCount( cmsecUncertainty + cmsecUncertainty / 10 );
        rgrs[ rsid ].lUniqueKey = rsid;
        TestCallS( ErrFromLRUK(
            rslruk.ErrCacheResource( rgrs[ rsid ].Key(),
                                     &rgrs[ rsid ],
                                     TickGetTickCount(),
                                     100 ) ) );
    }

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound, cResources );
    TestCheck( cFound == cResources );

    for ( INT iResource = 0; iResource < cResources; iResource++ )
    {
        TestCheck( rgrs[ iResource ].lLastScanPosition == iResource );
    }

HandleError:
    delete[] rgrs;
    return err;
}


CUnitTest( LrukApproximateIndexExpansion, 0, "LRU-K test with time advancing." );

ERR LrukApproximateIndexExpansion::ErrTest()
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

    SetTickCount( 0 );
    TestCallS( ErrFromLRUK(
          rslruk.ErrInit( 2,
                          0.128,
                          (double)( cmsecTimeout / 1000 ),
                          csecBFLRUKUncertainty,
                          dblBFHashLoadFactor,
                          dblBFHashUniformity,
                          dblBFSpeedSizeTradeoff ) ) );

    IncrementTickCount( 10 * cmsecUncertainty );

    for ( LONG rsid = 0; rsid < cResources; rsid++ )
    {
        IncrementTickCount( cmsecUncertainty + cmsecUncertainty / 10 );
        rgrs[ rsid ].lUniqueKey = rsid;
        TestCallS( ErrFromLRUK(
            rslruk.ErrCacheResource( rgrs[ rsid ].Key(),
                                     &rgrs[ rsid ],
                                     TickGetTickCount(),
                                     100 ) ) );
    }

    Scan( &rslruk, &rskeyCold, &rskeyHot, &cFound, cResources );
    TestCheck( cFound == cResources );
    for ( INT iResource = 0; iResource < cResources; iResource++ )
    {
        TestCheck( rgrs[ iResource ].lLastScanPosition == iResource );
    }

    for ( INT iCycle = 0; iCycle < cCycles; iCycle++ )
    {
        for ( LONG rsid = 0; rsid < cResources; rsid++ )
        {
            rslruk.BeginResourceScan( &lockLRUK );
            TestCallS( ErrFromLRUK( rslruk.ErrGetNextResource( &lockLRUK, &prs ) ) );
            TestCheck( prs == &rgrs[ rsid ] );
            EvictCurrentResource( &rslruk, &lockLRUK, &rgrs[ rsid ] );
            rslruk.EndResourceScan( &lockLRUK );

            rgrs[ rsid ].lLastScanPosition = -1;

            IncrementTickCount( cmsecUncertainty + cmsecUncertainty / 10 );
            TestCallS( ErrFromLRUK(
                rslruk.ErrCacheResource( rgrs[ rsid ].Key(),
                                         &rgrs[ rsid ],
                                         TickGetTickCount(),
                                         100,
                                         fFalse ) ) );
        }

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

