// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include <windows.h>
#include "rmemulator.hxx"


#ifdef DEBUG
static const CPG g_cpgChunk                     = 10;
#else
static const CPG g_cpgChunk                     = 32 * 1024;
#endif

static const ULONG g_pctCachePriorityDflt       = 100;

static const CPG g_cpgReserved                  = 2;

static const ULONG g_pctPercentileRes           = 5;

static const BYTE g_ifmpInvalid                 = 0xFF;
static const BYTE g_ifmpCachePriFilter          = g_ifmpInvalid;
static const ULONG g_pctIfmpCachePriFilter      = 5;
static const BYTE g_ifmpFilter                  = g_ifmpInvalid;

static const BYTE g_clientInvalid               = 0xFF;
static const BYTE g_clientCachePriFilter        = g_clientInvalid;
static const ULONG g_pctClientCachePriFilter    = 10;


#define absdiff( x, y ) ( ( (x) > (y) ) ? ( (x) - (y) ) : ( (y) - (x) ) )



LGPOS::LGPOS()
{
    ib = 0;
    isec = 0;
    lgen = 0;
}

LGPOS::LGPOS( const ULONG lgenT, const USHORT isecT, const USHORT ibT )
{
    ib = ibT;
    isec = isecT;
    lgen = lgenT;
}

INT LGPOS::FIsSet() const
{
    return ( CmpLgpos( *this, lgposMin ) != 0 && CmpLgpos( *this, lgposMax ) != 0 );
}

INT CmpLgpos( const LGPOS& lgpos1, const LGPOS& lgpos2 )
{
    if ( lgpos1.lgen < lgpos2.lgen )
{
        return -1;
}
    else if ( lgpos1.lgen > lgpos2.lgen )
{
        return 1;
}

    if ( lgpos1.isec < lgpos2.isec )
{
        return -1;
}
    else if ( lgpos1.isec > lgpos2.isec )
{
        return 1;
}

    if ( lgpos1.ib < lgpos2.ib )
{
        return -1;
}
    else if ( lgpos1.ib > lgpos2.ib )
{
        return 1;
}

    return 0;
}


PAGEENTRY::PAGEENTRY() :
    ppage( NULL ),
    fDirty( false ),
    lgposCleanLast(),
    cRequests( 0 ),
    cFaultsReal( 0 ),
    cFaultsSim( 0 ),
    cDirties( 0 ),
    cWritesReal( 0 ),
    cWritesSim( 0 ),
    tickLastCachedReal( 0 ),
    dtickCachedReal( 0 ),
    dtickCachedSim( 0 )
{
}



IPageEvictionAlgorithmImplementation::IPageEvictionAlgorithmImplementation()
{
}

IPageEvictionAlgorithmImplementation::~IPageEvictionAlgorithmImplementation()
{
}

bool IPageEvictionAlgorithmImplementation::FNeedsPreProcessing() const
{
    return false;
}

ERR IPageEvictionAlgorithmImplementation::ErrStartProcessing()
{
    return JET_errTestError;
}

ERR IPageEvictionAlgorithmImplementation::ErrResetProcessing()
{
    return JET_errTestError;
}



PageEvictionEmulator PageEvictionEmulator::s_singleton;

TICK PageEvictionEmulator::s_tickCurrent = 0;

TICK PageEvictionEmulator::s_tickLast = 0;

TICK PageEvictionEmulator::s_dtickAdj = 0;

PageEvictionEmulator::PageEvictionEmulator() :
    m_state( PageEvictionEmulator::peesUninitialized ),
    m_fResMgrInit( false ),
    m_arrayPages(),
    m_ilAvailPages(),
    m_arrayPageEntries(),
    m_arrayDirtyPageOps(),
    m_statsAgg(),
    m_stats(),
    m_pbfftlContext( NULL ),
    m_pipeaImplementation( NULL ),
    m_cbPage( 0 ),
    m_cpgChunk( 0 )
{
    m_arrayDirtyPageOps.SetEntryDefault( NULL );
    ResetConfig();
}

PageEvictionEmulator::~PageEvictionEmulator()
{
    Term();
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
}

ERR PageEvictionEmulator::ErrInit( BFFTLContext* const pbfftlContext,
                                    IPageEvictionAlgorithmImplementation* const pipeaImplementation )
{
    ERR err = JET_errSuccess;
    SYSTEM_INFO systemInfo = { 0 };


    if ( m_state != PageEvictionEmulator::peesUninitialized )
    {
        CallR( ErrERRCheck( JET_errTestError ) );
    }


    if ( pbfftlContext == NULL || pipeaImplementation == NULL )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if ( pipeaImplementation->CbPageContext() < sizeof(PAGE) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    Enforce( sizeof(PAGE) >= 1 );


    GetSystemInfo( &systemInfo );


    m_cbPage = roundup( pipeaImplementation->CbPageContext(), 64 );


    m_cpgChunk = roundup( m_cbPage * g_cpgChunk, systemInfo.dwPageSize ) / m_cbPage;



    CArray<PageEvictionEmulator::STATS>::ERR errStats = m_stats.ErrSetSize( pbfftlContext->cIFMP );
    if ( errStats != CArray<PageEvictionEmulator::STATS>::ERR::errSuccess )
    {
        Enforce( errStats == CArray<PageEvictionEmulator::STATS>::ERR::errOutOfMemory );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }


    CArray<CArray<PAGEENTRY>>::ERR errPageEntries = m_arrayPageEntries.ErrSetSize( pbfftlContext->cIFMP );
    if ( errPageEntries != CArray<CArray<PAGEENTRY>>::ERR::errSuccess )
    {
        Enforce( errPageEntries == CArray<CArray<PAGEENTRY>>::ERR::errOutOfMemory );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    for ( size_t ifmp = 0; ifmp < m_arrayPageEntries.Size(); ifmp++ )
    {
        CArray<PAGEENTRY>& pages = const_cast<CArray<PAGEENTRY>&>( m_arrayPageEntries[ ifmp ] );

        CArray<PAGEENTRY>::ERR errPages = pages.ErrSetSize( pbfftlContext->rgpgnoMax[ ifmp ] + g_cpgReserved );
        if ( errPages != CArray<PAGEENTRY>::ERR::errSuccess )
        {
            Enforce( errPages == CArray<PAGEENTRY>::ERR::errOutOfMemory );
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        m_arrayPageEntries[ ifmp ] = pages;
    }


    CArray<std::list<DirtyPageOp>*>::ERR errDirtyPageOps = m_arrayDirtyPageOps.ErrSetSize( pbfftlContext->cIFMP );
    if ( errDirtyPageOps != CArray<std::list<DirtyPageOp>*>::ERR::errSuccess )
    {
        Enforce( errDirtyPageOps == CArray<std::list<DirtyPageOp>*>::ERR::errOutOfMemory );
        Error( ErrERRCheck( JET_errOutOfMemory ) );
    }
    for ( size_t ifmp = 0; ifmp < m_arrayDirtyPageOps.Size(); ifmp++ )
    {
        std::list<DirtyPageOp>* const pDirtyPageOps = new std::list<DirtyPageOp>();

        if ( pDirtyPageOps == NULL )
        {
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }

        m_arrayDirtyPageOps[ ifmp ] = pDirtyPageOps;
    }


    m_pbfftlContext = pbfftlContext;
    m_pipeaImplementation = pipeaImplementation;


    for ( size_t ifmp = 0; ifmp <= m_stats.Size(); ifmp++ )
    {
        const bool fAggregated = ( ifmp == m_stats.Size() );
        PageEvictionEmulator::STATS& stats = fAggregated ? m_statsAgg : m_stats[ ifmp ];
        stats.cpgCached = 0;
        stats.cpgCachedMax = 0;
        stats.cRequestedUnique = 0;
        stats.cRequested = 0;
        stats.cFaultsReal = 0;
        stats.cFaultsSim = 0;
        stats.cFaultsRealAvoidable = 0;
        stats.cFaultsSimAvoidable = 0;
        stats.cCaches = 0;
        stats.cCachesNew = 0;
        stats.cCachesNewAlreadyCached = 0;
        stats.cCachesTurnedTouch = 0;
        stats.cCachesDbScan = 0;
        stats.cTouchesReal = 0;
        stats.cTouchesSim = 0;
        stats.cTouches = 0;
        stats.cTouchesNew = 0;
        stats.cTouchesNoTouch = 0;
        stats.cTouchesTurnedCache = 0;
        stats.cTouchesNoTouchTurnedCache = 0;
        stats.cTouchesNoTouchDropped = 0;
        stats.cTouchesDbScan = 0;
        stats.cDirties = 0;
        stats.cDirtiesUnique = 0;
        stats.cWritesReal = 0;
        stats.cWritesRealChkpt = 0;
        stats.cWritesRealAvailPool = 0;
        stats.cWritesRealShrink = 0;
        stats.cWritesSim = 0;
        stats.cWritesSimChkpt = 0;
        stats.cWritesSimAvailPool = 0;
        stats.cWritesSimShrink = 0;
        stats.cEvictionsReal = 0;
        stats.cEvictionsPurge = 0;
        stats.cEvictionsCacheTooBig = 0;
        stats.cEvictionsCacheTooOld = 0;
        stats.cEvictionsSim = 0;
        stats.cSuperColdedReal = 0;
        stats.cSuperColdedSim = 0;
        stats.lgenMin = ulMax;
        stats.lgenMax = 0;
        stats.pctCacheFaultRateReal = -1.0;
        stats.pctCacheSizeRatioSim = -1.0;
        stats.pctCacheFaultRateSim = -1.0;
        stats.pctCacheFaultRateRealAvoidable = -1.0;
        stats.pctCacheFaultRateSimAvoidable = -1.0;
        stats.histoFaultsReal.Zero();
        stats.histoFaultsSim.Zero();
        stats.histoLifetimeReal.Zero();
        stats.histoLifetimeSim.Zero();
    }

    m_statsAgg.cResMgrCycles = 0;
    m_statsAgg.cResMgrAbruptCycles = 0;
    m_statsAgg.cDiscardedTraces = 0;
    m_statsAgg.cOutOfRangeTraces = 0;
    m_statsAgg.cEvictionsFailed = 0;
    m_statsAgg.dtickDurationReal = 0;
    m_statsAgg.dtickDurationSimRun = 0;

    m_state = PageEvictionEmulator::peesInitialized;

    Enforce( err >= JET_errSuccess );

HandleError:

    if ( err < JET_errSuccess )
    {
        Term();
    }

    return err;
}

void PageEvictionEmulator::Term()
{

    if ( m_state == PageEvictionEmulator::peesUninitialized )
    {
        return;
    }

    Enforce( m_state != PageEvictionEmulator::peesRunning );


    PurgeCache_();


    m_ilAvailPages.Empty();


    for ( size_t iChunk = 0; iChunk < m_arrayPages.Size(); iChunk++ )
    {
        BYTE* const pbPageChunk = m_arrayPages[ iChunk ];
        if ( pbPageChunk != NULL )
        {
            const BOOL fFree = VirtualFree( pbPageChunk, 0, MEM_RELEASE );
            Enforce( fFree );
        }
    }
    const CArray<BYTE*>::ERR errSetCapacityPages = m_arrayPages.ErrSetCapacity( 0 );
    Enforce( errSetCapacityPages == CArray<BYTE*>::ERR::errSuccess );


    for ( size_t ifmp = 0; ifmp < m_arrayPageEntries.Size(); ifmp++ )
    {
        CArray<PAGEENTRY>& pages = const_cast<CArray<PAGEENTRY>&>( m_arrayPageEntries[ ifmp ] );
        const CArray<PAGEENTRY>::ERR errSetCapacityEntry = pages.ErrSetCapacity( 0 );
        Enforce( errSetCapacityEntry == CArray<PAGEENTRY>::ERR::errSuccess );
    }
    const CArray<CArray<PAGEENTRY>>::ERR errSetCapacityPageEntries = m_arrayPageEntries.ErrSetCapacity( 0 );
    Enforce( errSetCapacityPageEntries == CArray<CArray<PAGEENTRY>>::ERR::errSuccess );


    for ( size_t ifmp = 0; ifmp < m_arrayDirtyPageOps.Size(); ifmp++ )
    {
        std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];

        if ( pDirtyPageOps == NULL )
        {
            continue;
        }

        delete pDirtyPageOps;
        m_arrayDirtyPageOps[ ifmp ] = NULL;
    }
    const CArray<std::list<DirtyPageOp>*>::ERR errSetCapacityDirtyOpsEntries = m_arrayDirtyPageOps.ErrSetCapacity( 0 );
    Enforce( errSetCapacityDirtyOpsEntries == CArray<std::list<DirtyPageOp>*>::ERR::errSuccess );


    const CArray<PageEvictionEmulator::STATS>::ERR errSetCapacityStats = m_stats.ErrSetCapacity( 0 );
    Enforce( errSetCapacityStats == CArray<PageEvictionEmulator::STATS>::ERR::errSuccess );


    ResetConfig();
    m_state = PageEvictionEmulator::peesUninitialized;
}

ERR PageEvictionEmulator::ErrSetReplaySuperCold( const bool fReplaySuperCold )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_fReplaySuperCold = fReplaySuperCold;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetReplayNoTouch( const bool fReplayNoTouch )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_fReplayNoTouch = fReplayNoTouch;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetReplayDbScan( const bool fReplayDbScan )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_fReplayDbScan = fReplayDbScan;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetReplayCachePriority( const bool fReplayCachePriority )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_fReplayCachePriority = fReplayCachePriority;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetEvictNextOnShrink( const bool fEvictNextOnShrink )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_fEvictNextOnShrink = fEvictNextOnShrink;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetReplayInitTerm( const bool fReplayInitTerm )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_fReplayInitTerm= fReplayInitTerm;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetInitOverrides( const BFTRACE::BFSysResMgrInit_* const pbfinit )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    if ( pbfinit == NULL )
    {
        ResetInitOverrides();
    }
    else
    {
        m_bfinit = *pbfinit;
    }

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetCacheSize( const PageEvictionEmulatorCacheSizePolicy peecsp,
                                           const DWORD dwParam )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    Enforce( ( peecsp == peecspFixed ) || ( peecsp == peecspVariable ) || ( peecsp == peecspAgeBased ) );
    Enforce( ( peecsp == peecspVariable ) == ( dwParam == 0 ) );
    m_peecsp = peecsp;

    if ( m_peecsp == peecspFixed )
    {
        m_cbfCacheSize = dwParam;
    }
    else if ( m_peecsp == peecspAgeBased )
    {
        m_dtickCacheAge = dwParam;
    }

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetCheckpointDepth( const DWORD clogsChkptDepth )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    m_clogsChkptDepth = clogsChkptDepth;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetPrintSampleInterval( const TICK dtickPrintSampleInterval )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );

    m_dtickPrintSampleInterval = dtickPrintSampleInterval;

    return JET_errSuccess;
}

void PageEvictionEmulator::SetPrintHistograms( const bool fPrintHistograms )
{
    m_fPrintHistograms = fPrintHistograms;
}

ERR PageEvictionEmulator::ErrSetFaultsHistoRes( const ULONG cFaultsHistoRes )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    if ( cFaultsHistoRes == 0 )
    {
        return JET_errInvalidParameter;
    }

    m_cFaultsHistoRes = cFaultsHistoRes;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrSetLifetimeHistoRes( const TICK dtickLifetimeHistoRes )
{
    Enforce( m_state == PageEvictionEmulator::peesUninitialized );
    if ( dtickLifetimeHistoRes == 0 )
    {
        return JET_errInvalidParameter;
    }

    m_dtickLifetimeHistoRes = dtickLifetimeHistoRes;

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrExecute()
{
    ERR err = JET_errSuccess;
    QWORD cTracesReplayed = 0;
    TICK tickPrintNext = 0;
    BFTRACE bftrace;


    if ( m_state != PageEvictionEmulator::peesInitialized )
    {
        Error( ErrERRCheck( JET_errTestError ) );
    }

    Enforce( !m_fResMgrInit );
    Enforce( m_peecsp != peecspMax );

    printf( "\r\n" );


    m_state = PageEvictionEmulator::peesRunning;
    ResetGlobalTick();
    m_statsAgg.dtickDurationSimRun = TickOSTimeCurrent();

    if ( m_dtickPrintSampleInterval > 0 )
    {
        wprintf( L"\n" );
        wprintf( L"tick,cFaults,cFaultsAvoidable,cWrites,cWritesChkpt,cWritesScavenge,cFaults+cWrites,cpgCached,cpgCachedMax\n" );
    }


    ULONG iTraceLogFileLast = m_pbfftlContext->iTraceLogFile;
    while ( ( ( err = ErrBFFTLGetNext( m_pbfftlContext, &bftrace ) ) >= JET_errSuccess ) || ( err == errNotFound ) )
    {
        const bool fEndOfTrace = ( err == errNotFound );

        if ( !fEndOfTrace )
        {
            const ULONG iTraceLogFile = m_pbfftlContext->iTraceLogFile;
            SetGlobalTick( bftrace.tick, ( iTraceLogFile != iTraceLogFileLast ) );
            iTraceLogFileLast = iTraceLogFile;

            if ( m_statsAgg.dtickDurationReal == 0 )
            {
                m_statsAgg.dtickDurationReal = s_tickCurrent;
            }


            if ( !m_fResMgrInit && ( bftrace.traceid != bftidSysResMgrInit ) && ( m_peecsp != peecspVariable ) )
            {
                Call( ErrFakeProcessTraceInit_() );
            }


            if ( m_fResMgrInit || ( bftrace.traceid == bftidSysResMgrInit ) )
            {
                if ( m_peecsp == peecspAgeBased )
                {
                    MaintainAgeBasedCache_();
                }

                switch ( bftrace.traceid )
                {
                    case bftidSysResMgrInit:
                        if ( m_fReplayInitTerm || !m_fResMgrInit )
                        {
                            Call( ErrProcessTraceInit_( bftrace ) );
                        }
                        break;

                    case bftidSysResMgrTerm:
                        if ( m_fReplayInitTerm )
                        {
                            ProcessTraceTerm_( bftrace );
                        }
                        break;

                    case bftidCache:
                        Call( ErrProcessTraceCache_( bftrace ) );
                        break;

                    case bftidTouch:
                    case bftidTouchLong:
                        Call( ErrProcessTraceTouch_( bftrace ) );
                        break;

                    case bftidSuperCold:
                        Call( ErrProcessTraceSuperCold_( bftrace ) );
                        break;

                    case bftidEvict:
                        Call( ErrProcessTraceEvict_( bftrace ) );
                        break;

                    case bftidDirty:
                        Call( ErrProcessTraceDirty_( bftrace ) );
                        break;

                    case bftidWrite:
                        Call( ErrProcessTraceWrite_( bftrace ) );
                        break;

                    case bftidSetLgposModify:
                        Call( ErrProcessTraceSetLgposModify_( bftrace ) );
                        break;

                    default:
                        Call( ErrProcessTraceDefault_( bftrace ) );
                        break;
                }
            }
            else
            {
                m_statsAgg.cOutOfRangeTraces++;
            }
        }

        if ( m_dtickPrintSampleInterval > 0 )
        {
            const TICK tickPrint = s_tickCurrent - m_statsAgg.dtickDurationReal;
            if ( ( tickPrint >= tickPrintNext ) || fEndOfTrace )
            {
                for ( size_t ifmp = 0; ifmp < m_arrayPageEntries.Size(); ifmp++ )
                {
                    PageEvictionEmulator::STATS& stats = m_stats[ ifmp ];

                    m_statsAgg.cFaultsSim += stats.cFaultsSim;
                    m_statsAgg.cFaultsSimAvoidable += stats.cFaultsSimAvoidable;
                    m_statsAgg.cWritesSim += stats.cWritesSim;
                    m_statsAgg.cWritesSimChkpt += stats.cWritesSimChkpt;
                    m_statsAgg.cWritesSimAvailPool += stats.cWritesSimAvailPool;
                }

                wprintf( L"%I32u,%I64u,%I64u,%I64u,%I64u,%I64u,%I64u,%I32u,%I32u\n",
                         tickPrint,
                         m_statsAgg.cFaultsSim,
                         m_statsAgg.cFaultsSimAvoidable,
                         m_statsAgg.cWritesSim,
                         m_statsAgg.cWritesSimChkpt,
                         m_statsAgg.cWritesSimAvailPool,
                         m_statsAgg.cFaultsSim + m_statsAgg.cWritesSim,
                         m_statsAgg.cpgCached,
                         m_statsAgg.cpgCachedMax );

                m_statsAgg.cFaultsSim = 0;
                m_statsAgg.cFaultsSimAvoidable = 0;
                m_statsAgg.cWritesSim = 0;
                m_statsAgg.cWritesSimChkpt = 0;
                m_statsAgg.cWritesSimAvailPool = 0;

                tickPrintNext += m_dtickPrintSampleInterval;
                tickPrintNext = max( tickPrintNext, tickPrint + 1 );
            }
        }
        else
        {
            if ( ( ( cTracesReplayed++ % 100000 ) == 0 ) || fEndOfTrace )
            {
                printf( "." );
            }
        }

        if ( fEndOfTrace )
        {
            break;
        }
    }


    if ( err == errNotFound )
    {
        err = JET_errSuccess;
    }

HandleError:


    if ( m_fResMgrInit )
    {
        FakeProcessTraceTerm_();
    }


    if ( ( m_state == PageEvictionEmulator::peesRunning ) && ( err >= JET_errSuccess ) )
    {

        m_statsAgg.dtickDurationReal = ( s_tickCurrent - m_statsAgg.dtickDurationReal );
        m_statsAgg.dtickDurationSimRun = ( TickOSTimeCurrent() - m_statsAgg.dtickDurationSimRun );


        for ( size_t ifmp = 0; ifmp < m_arrayPageEntries.Size(); ifmp++ )
        {
            PageEvictionEmulator::STATS& stats = m_stats[ ifmp ];


            const CArray<PAGEENTRY>& pages = m_arrayPageEntries[ ifmp ];
            for ( size_t ipg = 0; ipg < pages.Size(); ipg++ )
            {
                CStats::ERR errStats = CStats::ERR::errSuccess;
                const PAGEENTRY* const ppge = pages.PEntry( ipg );


                if ( ppge->cRequests > 0 )
                {
                    stats.cRequestedUnique++;
                }


                if ( ppge->cDirties > 0 )
                {
                    stats.cDirtiesUnique++;
                }

                if ( m_fPrintHistograms )
                {

                    if ( ( ppge->cFaultsReal > 0 ) && ( ppge->cFaultsSim > 0 ) )
                    {
                        const ULONG cFaultsReal = m_cFaultsHistoRes * ( ppge->cFaultsReal / m_cFaultsHistoRes );
                        const ULONG cFaultsSim = m_cFaultsHistoRes * ( ppge->cFaultsSim / m_cFaultsHistoRes );
                        const TICK dtickLifetimeReal = m_dtickLifetimeHistoRes * ( ppge->dtickCachedReal / m_dtickLifetimeHistoRes );


                        const TICK dtickCachedSim = ( ppge->dtickCachedSim == 0 ) ? 1 : ppge->dtickCachedSim;
                        const TICK dtickLifetimeSim = m_dtickLifetimeHistoRes * ( dtickCachedSim / m_dtickLifetimeHistoRes );


                        errStats = stats.histoFaultsReal.ErrAddSample( cFaultsReal );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                        errStats = stats.histoFaultsSim.ErrAddSample( cFaultsSim );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                        errStats = stats.histoLifetimeReal.ErrAddSample( dtickLifetimeReal );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                        errStats = stats.histoLifetimeSim.ErrAddSample( dtickLifetimeSim );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }


                        errStats = m_statsAgg.histoFaultsReal.ErrAddSample( cFaultsReal );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                        errStats = m_statsAgg.histoFaultsSim.ErrAddSample( cFaultsSim );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                        errStats = m_statsAgg.histoLifetimeReal.ErrAddSample( dtickLifetimeReal );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                        errStats = m_statsAgg.histoLifetimeSim.ErrAddSample( dtickLifetimeSim );
                        if ( errStats != CStats::ERR::errSuccess )
                        {
                            Enforce( errStats == CStats::ERR::errOutOfMemory );
                            err = ErrERRCheck( JET_errOutOfMemory );
                            goto HandleHistogramOOM;
                        }
                    }
                }
            }


            m_statsAgg.cRequestedUnique += stats.cRequestedUnique;
            m_statsAgg.cRequested += stats.cRequested;
            m_statsAgg.cFaultsReal += stats.cFaultsReal;
            m_statsAgg.cFaultsSim += stats.cFaultsSim;
            m_statsAgg.cFaultsRealAvoidable += stats.cFaultsRealAvoidable;
            m_statsAgg.cFaultsSimAvoidable += stats.cFaultsSimAvoidable;
            m_statsAgg.cCaches += stats.cCaches;
            m_statsAgg.cCachesNew += stats.cCachesNew;
            m_statsAgg.cCachesNewAlreadyCached += stats.cCachesNewAlreadyCached;
            m_statsAgg.cCachesTurnedTouch += stats.cCachesTurnedTouch;
            m_statsAgg.cCachesDbScan += stats.cCachesDbScan;
            m_statsAgg.cTouchesReal += stats.cTouchesReal;
            m_statsAgg.cTouchesSim += stats.cTouchesSim;
            m_statsAgg.cTouches += stats.cTouches;
            m_statsAgg.cTouchesNew += stats.cTouchesNew;
            m_statsAgg.cTouchesNoTouch += stats.cTouchesNoTouch;
            m_statsAgg.cTouchesTurnedCache += stats.cTouchesTurnedCache;
            m_statsAgg.cTouchesNoTouchTurnedCache += stats.cTouchesNoTouchTurnedCache;
            m_statsAgg.cTouchesNoTouchDropped += stats.cTouchesNoTouchDropped;
            m_statsAgg.cTouchesDbScan += stats.cTouchesDbScan;
            m_statsAgg.cDirties += stats.cDirties;
            m_statsAgg.cDirtiesUnique += stats.cDirtiesUnique;
            m_statsAgg.cWritesReal += stats.cWritesReal;
            m_statsAgg.cWritesRealChkpt += stats.cWritesRealChkpt;
            m_statsAgg.cWritesRealAvailPool += stats.cWritesRealAvailPool;
            m_statsAgg.cWritesRealShrink += stats.cWritesRealShrink;
            m_statsAgg.cWritesSim += stats.cWritesSim;
            m_statsAgg.cWritesSimChkpt += stats.cWritesSimChkpt;
            m_statsAgg.cWritesSimAvailPool += stats.cWritesSimAvailPool;
            m_statsAgg.cWritesSimShrink += stats.cWritesSimShrink;
            m_statsAgg.cEvictionsReal += stats.cEvictionsReal;
            m_statsAgg.cEvictionsSim += stats.cEvictionsSim;
            m_statsAgg.cEvictionsCacheTooBig += stats.cEvictionsCacheTooBig;
            m_statsAgg.cEvictionsCacheTooOld += stats.cEvictionsCacheTooOld;
            m_statsAgg.cEvictionsPurge += stats.cEvictionsPurge;
            m_statsAgg.cSuperColdedReal += stats.cSuperColdedReal;
            m_statsAgg.cSuperColdedSim += stats.cSuperColdedSim;
        }


        for ( size_t ifmp = 0; ifmp <= m_stats.Size(); ifmp++ )
        {
            const bool fAggregated = ( ifmp == m_stats.Size() );
            PageEvictionEmulator::STATS& stats = fAggregated ? m_statsAgg : m_stats[ ifmp ];

            if ( stats.cRequested > 0 )
            {
                stats.pctCacheFaultRateReal = ( (double)stats.cFaultsReal / (double)stats.cRequested ) * 100.0;
                stats.pctCacheFaultRateSim = ( (double)stats.cFaultsSim / (double)stats.cRequested ) * 100.0;
                stats.pctCacheFaultRateRealAvoidable = ( (double)stats.cFaultsRealAvoidable / (double)stats.cRequested ) * 100.0;
                stats.pctCacheFaultRateSimAvoidable = ( (double)stats.cFaultsSimAvoidable / (double)stats.cRequested ) * 100.0;
            }

            if ( ( m_peecsp == peecspFixed ) && ( stats.cRequestedUnique > 0 ) )
            {
                stats.pctCacheSizeRatioSim = ( (double)m_cbfCacheSize / (double)stats.cRequestedUnique ) * 100.0;
            }
        }


        m_state = PageEvictionEmulator::peesDone;
    }

HandleHistogramOOM:

    printf( "\r\n" );

    Enforce( !m_fResMgrInit );

    return err;
}

PageEvictionEmulator& PageEvictionEmulator::GetEmulatorObj()
{
    return s_singleton;
}

const PageEvictionEmulator::STATS_AGG& PageEvictionEmulator::GetStats()
{
    Enforce( m_state == PageEvictionEmulator::peesDone );

    return m_statsAgg;
}

const PageEvictionEmulator::STATS& PageEvictionEmulator::GetStats( const size_t ifmp )
{
    Enforce( m_state == PageEvictionEmulator::peesDone );

    return m_stats[ ifmp ];
}

void PageEvictionEmulator::ResetInitOverrides( BFTRACE::BFSysResMgrInit_* const pbfinit )
{
    pbfinit->K = -1;
    pbfinit->csecCorrelatedTouch = -1.0;
    pbfinit->csecTimeout = -1.0;
    pbfinit->csecUncertainty = -1.0;
    pbfinit->dblHashLoadFactor = -1.0;
    pbfinit->dblHashUniformity = -1.0;
    pbfinit->dblSpeedSizeTradeoff = -1.0;
}

ERR PageEvictionEmulator::ErrDumpStats()
{
    return ErrDumpStats( true );
}

ERR PageEvictionEmulator::ErrDumpStats( const bool fDumpHistogramDetails )
{
    if ( m_state != PageEvictionEmulator::peesDone )
    {
        return ErrERRCheck( JET_errTestError );
    }

    wprintf( L"Resource manager emulator statistics...\n\n" );


    wprintf( L"General statistics...\n" );
    wprintf( L"  cResMgrCycles: %lu\n", m_statsAgg.cResMgrCycles );
    wprintf( L"  cResMgrAbruptCycles: %lu\n", m_statsAgg.cResMgrAbruptCycles );
    wprintf( L"  cDiscardedTraces: %I64u\n", m_statsAgg.cDiscardedTraces );
    wprintf( L"  cOutOfRangeTraces: %I64u\n", m_statsAgg.cOutOfRangeTraces );
    wprintf( L"  cEvictionsFailed: %I64u\n", m_statsAgg.cEvictionsFailed );
    wprintf( L"  dtickDurationReal: %lu\n", m_statsAgg.dtickDurationReal );
    wprintf( L"  dtickDurationSimRun: %lu\n", m_statsAgg.dtickDurationSimRun );
    wprintf( L"\n\n" );


    for ( size_t ifmpT = 0; ifmpT <= m_stats.Size(); ifmpT++ )
    {
        const bool fAggregated = ( ifmpT == 0 );
        const size_t ifmp = ifmpT - 1;
        PageEvictionEmulator::STATS& stats = fAggregated ? m_statsAgg : m_stats[ ifmp ];

        if ( fAggregated )
        {
            wprintf( L"Per-IFMP statistics... (aggregated)\n" );
        }
        else
        {
            wprintf( L"Per-IFMP statistics... (IFMP %Iu)\n", ifmp );
            if ( stats.cpgCachedMax == 0 )
            {
                wprintf( L"  No pages processed for this IFMP.\n\n" );
                continue;
            }
        }

        wprintf( L"  cpgCached: %ld\n", stats.cpgCached );
        wprintf( L"  cpgCachedMax: %ld\n", stats.cpgCachedMax );
        wprintf( L"  cRequestedUnique: %I64u\n", stats.cRequestedUnique );
        wprintf( L"  cRequested: %I64u\n", stats.cRequested );
        wprintf( L"  cFaultsReal: %I64u\n", stats.cFaultsReal );
        wprintf( L"  cFaultsSim: %I64u\n", stats.cFaultsSim );
        wprintf( L"  cFaultsRealAvoidable: %I64u\n", stats.cFaultsRealAvoidable );
        wprintf( L"  cFaultsSimAvoidable: %I64u\n", stats.cFaultsSimAvoidable );
        wprintf( L"  cCaches: %I64u\n", stats.cCaches );
        wprintf( L"  cCachesNew: %I64u\n", stats.cCachesNew );
        wprintf( L"  cCachesNewAlreadyCached: %I64u\n", stats.cCachesNewAlreadyCached );
        wprintf( L"  cCachesDbScan: %I64u\n", stats.cCachesDbScan );
        wprintf( L"  cTouchesTurnedCache: %I64u\n", stats.cTouchesTurnedCache );
        wprintf( L"  cTouchesNoTouchTurnedCache: %I64u\n", stats.cTouchesNoTouchTurnedCache );
        wprintf( L"  cTouchesNoTouchDropped: %I64u\n", stats.cTouchesNoTouchDropped );
        wprintf( L"  cTouchesDbScan: %I64u\n", stats.cTouchesDbScan );
        wprintf( L"  cTouchesReal: %I64u\n", stats.cTouchesReal );
        wprintf( L"  cTouchesSim: %I64u\n", stats.cTouchesSim );
        wprintf( L"  cTouches: %I64u\n", stats.cTouches );
        wprintf( L"  cTouchesNew: %I64u\n", stats.cTouchesNew );
        wprintf( L"  cTouchesNoTouch: %I64u\n", stats.cTouchesNoTouch );
        wprintf( L"  cCachesTurnedTouch: %I64u\n", stats.cCachesTurnedTouch );
        wprintf( L"  cDirties: %I64u\n", stats.cDirties );
        wprintf( L"  cDirtiesUnique: %I64u\n", stats.cDirtiesUnique );
        wprintf( L"  cWritesReal: %I64u\n", stats.cWritesReal );
        wprintf( L"  cWritesRealChkpt: %I64u\n", stats.cWritesRealChkpt );
        wprintf( L"  cWritesRealAvailPool: %I64u\n", stats.cWritesRealAvailPool );
        wprintf( L"  cWritesRealShrink: %I64u\n", stats.cWritesRealShrink );
        wprintf( L"  cWritesReal (others): %I64u\n", stats.cWritesReal - ( stats.cWritesRealChkpt + stats.cWritesRealAvailPool + stats.cWritesRealShrink ) );
        wprintf( L"  cWritesSim: %I64u\n", stats.cWritesSim );
        wprintf( L"  cWritesSimChkpt: %I64u\n", stats.cWritesSimChkpt );
        wprintf( L"  cWritesSimAvailPool: %I64u\n", stats.cWritesSimAvailPool );
        wprintf( L"  cWritesSimShrink: %I64u\n", stats.cWritesSimShrink );
        wprintf( L"  cWritesSimScavenge: %I64u\n", stats.cWritesSimAvailPool + ( m_fEvictNextOnShrink ? stats.cWritesSimShrink : 0 ) );
        wprintf( L"  cWritesSim (others): %I64u\n", stats.cWritesSim - ( stats.cWritesSimChkpt + stats.cWritesSimAvailPool + stats.cWritesSimShrink ) );
        wprintf( L"  cEvictionsReal: %I64u\n", stats.cEvictionsReal );
        wprintf( L"  cEvictionsPurge: %I64u\n", stats.cEvictionsPurge );
        wprintf( L"  cEvictionsCacheTooBig: %I64u\n", stats.cEvictionsCacheTooBig );
        wprintf( L"  cEvictionsCacheTooOld: %I64u\n", stats.cEvictionsCacheTooOld );
        wprintf( L"  cEvictionsSim: %I64u\n", stats.cEvictionsSim );
        wprintf( L"  cSuperColdedReal: %I64u\n", stats.cSuperColdedReal );
        wprintf( L"  cSuperColdedSim: %I64u\n", stats.cSuperColdedSim );
        if ( !fAggregated )
        {
            if ( stats.lgenMin != ulMax )
            {
                wprintf( L"  lgen: %I32u (%I32u - %I32u)\n", stats.lgenMax - stats.lgenMin + 1, stats.lgenMin, stats.lgenMax );
            }
            else
            {
                wprintf( L"  lgen: N/A\n" );
            }
        }

        Enforce( stats.cTouches >= stats.cTouchesTurnedCache );
        Enforce( stats.cTouches >= stats.cTouchesNew );
        Enforce( stats.cTouches >= stats.cTouchesNoTouch );
        Enforce( stats.cTouches >= stats.cTouchesNoTouchTurnedCache );
        Enforce( stats.cTouches >= stats.cTouchesNoTouchDropped );
        Enforce( stats.cTouches >= stats.cTouchesDbScan );
        Enforce( stats.cCaches >= stats.cCachesNew );
        Enforce( stats.cCachesNew >= stats.cCachesNewAlreadyCached );
        Enforce( stats.cCaches >= stats.cCachesTurnedTouch );
        Enforce( stats.cCaches >= stats.cCachesDbScan );
        Enforce( stats.cWritesReal >= ( stats.cWritesRealChkpt + stats.cWritesRealAvailPool + stats.cWritesRealShrink ) );
        Enforce( stats.cWritesSim >= ( stats.cWritesSimChkpt + stats.cWritesSimAvailPool + stats.cWritesSimShrink ) );
        Enforce( !m_fReplayNoTouch || ( stats.cTouchesNoTouchDropped == 0 ) );


        if ( stats.pctCacheFaultRateReal >= 0.0 )
        {
            wprintf( L"  Cache fault rate (real): %.6f%%\n", stats.pctCacheFaultRateReal );
        }

        if ( stats.pctCacheFaultRateRealAvoidable >= 0.0 )
        {
            wprintf( L"  Cache fault rate (real, avoidable): %.6f%%\n", stats.pctCacheFaultRateRealAvoidable );
        }


        if ( stats.pctCacheSizeRatioSim >= 0.0 )
        {
            wprintf( L"  Cache size by data size ratio (simulated): %.6f%%\n", stats.pctCacheSizeRatioSim );
        }

        if ( stats.pctCacheFaultRateSim >= 0.0 )
        {
            wprintf( L"  Cache fault rate (simulated): %.6f%%\n", stats.pctCacheFaultRateSim );
        }

        if ( stats.pctCacheFaultRateSimAvoidable >= 0.0 )
        {
            wprintf( L"  Cache fault rate (simulated, avoidable): %.6f%%\n", stats.pctCacheFaultRateSimAvoidable );
        }


        if ( stats.cCaches != stats.cEvictionsReal )
        {
            wprintf( L"WARNING: mismatching number of page caches versus page evictions.\n" );
        }

        if ( ( stats.cFaultsReal + stats.cTouchesReal + stats.cTouchesNoTouch ) !=
                ( stats.cFaultsSim + stats.cTouchesSim + stats.cTouchesNoTouchDropped ) )
        {
            wprintf( L"WARNING: mismatching number of page requests (faults + touches) between real and simulated runs.\n" );
        }

        if ( m_peecsp == peecspFixed )
        {
            Enforce( (DWORD)stats.cpgCachedMax <= m_cbfCacheSize );
            Enforce( stats.cEvictionsCacheTooOld == 0 );
        }
        else if ( m_peecsp == peecspVariable )
        {
            Enforce( stats.cEvictionsCacheTooBig == 0 );
            Enforce( stats.cEvictionsCacheTooOld == 0 );

            if ( fAggregated )
            {
                const QWORD cEvictionsEffective = stats.cEvictionsReal - m_statsAgg.cEvictionsFailed + stats.cEvictionsPurge;
                const QWORD cCachesIndeed = stats.cCaches - stats.cCachesTurnedTouch - ( m_fReplayDbScan ? 0 : stats.cCachesDbScan );
                const QWORD cCachesEffective = cCachesIndeed + stats.cTouchesTurnedCache;

                if ( cCachesEffective != cEvictionsEffective )
                {
                    wprintf( L"WARNING: mismatching number of effective page caches versus effective page evictions.\n" );
                }
            }
        }
        else if ( m_peecsp == peecspAgeBased )
        {
            Enforce( stats.cEvictionsCacheTooBig == 0 );
        }
        else
        {
            Enforce( fFalse );
        }

        if ( m_fPrintHistograms )
        {

            const SAMPLE sampleFaultsMin = min( stats.histoFaultsReal.Min(), stats.histoFaultsSim.Min() );
            const SAMPLE sampleFaultsMax = max( stats.histoFaultsReal.Max(), stats.histoFaultsSim.Max() );

            wprintf( L"  histoFaultsReal:\n" ); 
            DumpHistogram_( stats.histoFaultsReal, sampleFaultsMin, sampleFaultsMax, m_cFaultsHistoRes, fDumpHistogramDetails );
            wprintf( L"  histoFaultsSim:\n" );
            DumpHistogram_( stats.histoFaultsSim, sampleFaultsMin, sampleFaultsMax, m_cFaultsHistoRes, fDumpHistogramDetails );

            const TICK dtickLifetimeMin = (TICK)min( stats.histoLifetimeReal.Min(), stats.histoLifetimeSim.Min() );
            const TICK dtickLifetimeMax = (TICK)max( stats.histoLifetimeReal.Max(), stats.histoLifetimeSim.Max() );

            wprintf( L"  histoLifetimeReal:\n" );
            DumpHistogram_( stats.histoLifetimeReal, dtickLifetimeMin, dtickLifetimeMax, m_dtickLifetimeHistoRes, fDumpHistogramDetails );
            wprintf( L"  histoLifetimeSim:\n" );
            DumpHistogram_( stats.histoLifetimeSim, dtickLifetimeMin, dtickLifetimeMax, m_dtickLifetimeHistoRes, fDumpHistogramDetails );
        }

        wprintf( L"\n" );
    }

    return JET_errSuccess;
}

TICK PageEvictionEmulator::DwGetTickCount()
{
    return s_tickCurrent;
}

void PageEvictionEmulator::ResetGlobalTick()
{
    s_tickCurrent = 0;
    s_tickLast = 0;
    s_dtickAdj = 0;
}

void PageEvictionEmulator::SetGlobalTick( const TICK tick, const bool fTimeMayGoBack )
{

    const TICK tickLastNew = !tick ? 1 : tick;

    if ( s_tickLast == 0 )
    {
        s_tickLast = tickLastNew;
    }

    const TICK tickLastOld = s_tickLast;


    const TICK dtick = tickLastNew - tickLastOld;
    const bool fTimeGoesBack = ( (LONG)dtick < 0 );
    Enforce( !fTimeGoesBack || fTimeMayGoBack );

    if ( fTimeGoesBack )
    {
        s_dtickAdj -= dtick;
    }

    s_tickLast = tickLastNew;
    s_tickCurrent = s_tickLast + s_dtickAdj;
}

void PageEvictionEmulator::ResetConfig()
{
    m_fReplaySuperCold = true;
    m_fReplayNoTouch = false;
    m_fReplayDbScan = true;
    m_fReplayCachePriority = true;
    m_fEvictNextOnShrink = true;
    m_fReplayInitTerm = true;
    m_peecsp = peecspVariable;
    m_cbfCacheSize = 0;
    m_dtickCacheAge = 0;
    m_clogsChkptDepth = 4;
    ResetInitOverrides();
    m_fPrintHistograms = true;
    m_cFaultsHistoRes = 1;
    m_dtickPrintSampleInterval = 0;
    m_dtickLifetimeHistoRes = 1;
    m_fSetLgposModifySupported = false;
}

void PageEvictionEmulator::ResetInitOverrides()
{
    ResetInitOverrides( &m_bfinit );
}

PAGE* PageEvictionEmulator::PpgGetAvailablePage_()
{
    PAGE* ppage = NULL;


    if ( m_ilAvailPages.FEmpty() )
    {

        CArray<BYTE*>::ERR errPages = m_arrayPages.ErrSetSize( m_arrayPages.Size() + 1 );

        if ( errPages == CArray<BYTE*>::ERR::errSuccess )
        {
            BYTE* const * const ppbPageChunk = m_arrayPages.PEntry( m_arrayPages.Size() - 1 );
            m_arrayPages.SetEntry( ppbPageChunk, NULL );

            const size_t cbPages = m_cpgChunk * m_cbPage;
            BYTE* const pbPageChunk = (BYTE*)VirtualAlloc( NULL, cbPages, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );

            if ( pbPageChunk != NULL )
            {

                for ( size_t ipgChunk = 0; ipgChunk < m_cpgChunk; ipgChunk++ )
                {
                    PAGE* const ppg = (PAGE*)( pbPageChunk + ipgChunk * m_cbPage );
                    m_pipeaImplementation->InitPage( ppg );
                    FreePage_( ppg );
                }

                m_arrayPages.SetEntry( ppbPageChunk, pbPageChunk );
            }
        }
    }


    ppage = m_ilAvailPages.NextMost();


    if ( ppage != NULL )
    {
        m_ilAvailPages.Remove( ppage );
        memset( ppage, 0, m_cbPage );
        m_pipeaImplementation->InitPage( ppage );
    }

    return ppage;
}

void PageEvictionEmulator::FreePage_( PAGE* const ppg )
{
    m_pipeaImplementation->DestroyPage( ppg );
    m_ilAvailPages.InsertAsNextMost( ppg );
}

void PageEvictionEmulator::PurgeCache_()
{
    for ( size_t ifmp = 0; ifmp < m_arrayPageEntries.Size(); ifmp++ )
    {
        const CArray<PAGEENTRY>& pages = m_arrayPageEntries[ ifmp ];
        LGPOS lgposNewest;
        GetNewestDirtyLgpos_( ifmp, &lgposNewest );

        for ( size_t ipg = 0; ipg < pages.Size(); ipg++ )
        {
            PAGEENTRY* const ppge = const_cast<PAGEENTRY*>( pages.PEntry( ipg ) );


            if ( ppge->tickLastCachedReal != 0 )
            {
                const TICK dtickCachedReal = ppge->dtickCachedReal;
                ppge->dtickCachedReal += ( s_tickCurrent - ppge->tickLastCachedReal );
                Enforce( ppge->dtickCachedReal >= dtickCachedReal );
                ppge->tickLastCachedReal = 0;
            }


            if ( ppge->ppage != NULL )
            {

                BFTRACE::BFEvict_ bfevict = { 0 };
                bfevict.ifmp = (BYTE)ppge->ppage->ifmppgno.ifmp;
                bfevict.pgno = ppge->ppage->ifmppgno.pgno;

                const ERR errEvict = m_pipeaImplementation->ErrEvictSpecificPage( ppge->ppage, bfevict );
                Enforce( errEvict == JET_errSuccess );


                EvictPage_( ppge, iorpBFPurge, lgposNewest );

                m_stats[ ifmp ].cEvictionsPurge++;
            }
        }

        Enforce( m_stats[ ifmp ].cpgCached == 0 );
    }

    Enforce( m_statsAgg.cpgCached == 0 );

    for ( size_t ifmp = 0; ifmp < m_arrayDirtyPageOps.Size(); ifmp++ )
    {
        std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];
        pDirtyPageOps->clear();
    }
}

PAGEENTRY* PageEvictionEmulator::PpgeGetEntry_( const IFMPPGNO& ifmppgno ) const
{
    const CArray<PAGEENTRY>* const pageEntries = m_arrayPageEntries.PEntry( ifmppgno.ifmp );
    return const_cast<PAGEENTRY*>( pageEntries->PEntry( ifmppgno.pgno + g_cpgReserved - 1 ) );
}

PAGEENTRY* PageEvictionEmulator::PpgeGetEntry_( const void* const pv, const PageEvictionContextType pectyp ) const
{
    PAGEENTRY* ppgeEvict = NULL;

    if ( pectyp == pectypPAGE )
    {
        PAGE* ppageEvict = *(PAGE**)pv;

        if ( ppageEvict != NULL )       
        {
            ppgeEvict = PpgeGetEntry_( ppageEvict->ifmppgno );
            Enforce( ppgeEvict->ppage == ppageEvict );
        }
    }
    else if ( pectyp == pectypIFMPPGNO )
    {
        ppgeEvict = PpgeGetEntry_( *( (IFMPPGNO*)pv ) );
    }

    return ppgeEvict;
}

void PageEvictionEmulator::ModifyPage_( const IFMPPGNO& ifmppgno, const LGPOS& lgposModify )
{
    PAGEENTRY* const ppge = PpgeGetEntry_( ifmppgno );
    PAGE* const ppage = ppge->ppage;
    const IFMP ifmp = ifmppgno.ifmp;

    m_stats[ ifmp ].cDirties++;

    Enforce( lgposModify.FIsSet() );
    m_stats[ ifmp ].lgenMin = min( m_stats[ ifmp ].lgenMin, lgposModify.lgen );
    m_stats[ ifmp ].lgenMax = max( m_stats[ ifmp ].lgenMax, lgposModify.lgen );


    if ( ppage != NULL )
    {
        const PGNO pgno = ifmppgno.pgno;

        DirtyPage_( ppge );
        InsertDirtyPageOp_( ifmp, pgno, lgposModify );


        do
        {
            LGPOS lgposNewest, lgposOldestAdjusted;

            GetNewestDirtyLgpos_( ifmp, &lgposNewest );

            if ( CmpLgpos( lgposNewest, lgposMin ) == 0 )
            {
                break;
            }

            GetOldestDirtyLgpos_( ifmp, &lgposOldestAdjusted );

            lgposOldestAdjusted.lgen += m_clogsChkptDepth;

            if ( CmpLgpos( lgposNewest, lgposOldestAdjusted ) < 0 )
            {
                break;
            }

            PGNO pgnoToClean;
            LGPOS lgposOldest;
            RemoveOldestDirtyPageOp_( ifmp, &pgnoToClean, &lgposOldest );

            const IFMPPGNO ifmppgnoToClean( ifmp, pgnoToClean );
            PAGEENTRY* const ppgeToClean = PpgeGetEntry_( ifmppgnoToClean );


            if ( ( ppgeToClean->ppage != NULL ) && ppgeToClean->fDirty && ( CmpLgpos( ppgeToClean->lgposCleanLast, lgposOldest ) <= 0 ) )
            {
                WritePage_( ppgeToClean, iorpBFCheckpointAdv, lgposNewest );
            }
        } while ( true );
    }
}

void PageEvictionEmulator::InsertDirtyPageOp_( const IFMP ifmp, const PGNO pgno, const LGPOS& lgpos )
{
    Enforce( lgpos.FIsSet() );

    std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];


    LGPOS lgposNewest;
    GetNewestDirtyLgpos_( ifmp, &lgposNewest );

    DirtyPageOp dirtyPageOp;
    dirtyPageOp.lgpos = ( ( CmpLgpos( lgpos, lgposNewest ) >= 0 ) ? lgpos : lgposNewest );
    dirtyPageOp.pgno = pgno;
    pDirtyPageOps->push_back( dirtyPageOp );
}

void PageEvictionEmulator::RemoveOldestDirtyPageOp_( const IFMP ifmp, PGNO* const ppgno, LGPOS* const plgpos )
{
    std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];
    Enforce( !pDirtyPageOps->empty() );

    const DirtyPageOp& dirtyPageOp = pDirtyPageOps->front();
    *ppgno = dirtyPageOp.pgno;
    *plgpos = dirtyPageOp.lgpos;

    pDirtyPageOps->pop_front();
}

void PageEvictionEmulator::GetOldestDirtyLgpos_( const IFMP ifmp, LGPOS* const plgpos ) const
{
    const std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];

    if ( pDirtyPageOps->empty() )
    {
        *plgpos = lgposMin;
    }
    else
    {
        *plgpos = pDirtyPageOps->front().lgpos;
    }
}

void PageEvictionEmulator::GetNewestDirtyLgpos_( const IFMP ifmp, LGPOS* const plgpos ) const
{
    const std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];

    if ( pDirtyPageOps->empty() )
    {
        *plgpos = lgposMin;
    }
    else
    {
        *plgpos = pDirtyPageOps->back().lgpos;
    }
}

ERR PageEvictionEmulator::ErrCachePage_( PAGEENTRY* const ppge, const IFMPPGNO& ifmppgno, const bool fNewPage )
{
    ERR err = JET_errSuccess;

    Enforce( ppge->ppage == NULL );
    Enforce( !ppge->fDirty );


    if ( ( m_peecsp == peecspFixed ) && ( (DWORD)m_statsAgg.cpgCached >= m_cbfCacheSize ) )
    {
        Enforce( (DWORD)m_statsAgg.cpgCached == m_cbfCacheSize );


        BFTRACE::BFEvict_ bfevict = { 0 };
        bfevict.fCurrentVersion = fTrue;
        bfevict.errBF = JET_errSuccess;
        bfevict.bfef = bfefReasonAvailPool | bfefKeepHistory;
        bfevict.pctPri = g_pctCachePriorityDflt;

        BYTE pv[ max( sizeof(PAGE*), sizeof(IFMPPGNO) ) ];
        PageEvictionContextType pectyp;
        const ERR errEvict = m_pipeaImplementation->ErrEvictNextPage( pv, bfevict, &pectyp );
        Enforce( errEvict == JET_errSuccess );

        PAGEENTRY* const ppgeEvict = PpgeGetEntry_( pv, pectyp );

        LGPOS lgposNewest;
        GetNewestDirtyLgpos_( ifmppgno.ifmp, &lgposNewest );
        EvictPage_( ppgeEvict, iorpBFAvailPool, lgposNewest );

        m_stats[ ifmppgno.ifmp ].cEvictionsCacheTooBig++;
    }

    PAGE* const ppage = PpgGetAvailablePage_();

    Alloc( ppage );


    Enforce( m_statsAgg.cpgCached < LONG_MAX );
    m_statsAgg.cpgCached++;
    if ( m_statsAgg.cpgCached > m_statsAgg.cpgCachedMax )
    {
        m_statsAgg.cpgCachedMax = m_statsAgg.cpgCached;
    }


    Enforce( m_stats[ ifmppgno.ifmp ].cpgCached < LONG_MAX );
    m_stats[ ifmppgno.ifmp ].cpgCached++;
    if ( m_stats[ ifmppgno.ifmp ].cpgCached > m_stats[ ifmppgno.ifmp ].cpgCachedMax )
    {
        m_stats[ ifmppgno.ifmp ].cpgCachedMax = m_stats[ ifmppgno.ifmp ].cpgCached;
    }


    if ( !fNewPage )
    {
        m_stats[ ifmppgno.ifmp ].cFaultsSim++;


        if ( ppge->cRequests > 1 )
        {
            m_stats[ ifmppgno.ifmp ].cFaultsSimAvoidable++;
        }

        ppge->cFaultsSim++;
        Enforce( ppge->cFaultsSim > 0 );
    }


    ppage->tickLastFaultSim = s_tickCurrent;


    ppage->ifmppgno = ifmppgno;
    ppge->ppage = ppage;

HandleError:

    return err;
}

void PageEvictionEmulator::DumpHistogram_( CPerfectHistogramStats& histogram,
                                            const SAMPLE sampleMin,
                                            const SAMPLE sampleMax,
                                            const SAMPLE sampleRes,
                                            const bool fDumpDetails )
{
    wprintf( L"    Samples: %I64d\n", histogram.C() );


    if ( histogram.C() == 0 )
    {
        return;
    }

    wprintf( L"    Min: %I64u\n", histogram.Min() );
    wprintf( L"    Max: %I64u\n", histogram.Max() );
    wprintf( L"    Avg: %.3f\n", histogram.DblAve() );

    if ( !fDumpDetails )
    {
        return;
    }

    CStats::ERR errStats;
    CHITS cHits;
    SAMPLE sample;
    DWORD cSegments;


    sample = min( sampleMin + sampleRes, sampleMax );
    wprintf( L"    Histogram:\n" );
    wprintf( L"      %I64u:%I64u", sampleMin, sample );
    cSegments = 1;
    while ( sample < sampleMax )
    {
        sample += sampleRes;
        sample = min( sample, sampleMax );
        wprintf( L"\t:%I64u", sample );
        cSegments++;
    }
    wprintf( L"\n" );


    errStats = histogram.ErrReset();
    Enforce( errStats == CStats::ERR::errSuccess );
    sample = min( sampleMin + sampleRes, sampleMax );
    wprintf( L"      " );
    while ( cSegments > 0 )
    {
        errStats = histogram.ErrGetSampleHits( sample, &cHits );
        if ( errStats != CStats::ERR::errSuccess )
        {
            Enforce( errStats == CStats::ERR::wrnOutOfSamples );
            cHits = 0;
        }
        wprintf( L"%I64d\t", cHits );
        sample += sampleRes;
        cSegments--;
    }
    wprintf( L"\n" );

    const ULONG percentileMin = 0;
    const ULONG percentileMax = 100;
    const ULONG percentileRes = g_pctPercentileRes;
    ULONG percentile, percentileT;


    percentile = min( percentileMin + percentileRes, percentileMax );
    wprintf( L"    Percentiles:\n" );
    wprintf( L"      %lu:%lu", percentileMin, percentile );
    cSegments = 1;
    while ( percentile < percentileMax )
    {
        percentile += percentileRes;
        percentile = min( percentile, percentileMax );
        wprintf( L"\t:%lu", percentile );
        cSegments++;
    }
    wprintf( L"\n" );


    bool fPrintSample = false;
    bool fFoundSample = false;
    errStats = histogram.ErrReset();
    Enforce( errStats == CStats::ERR::errSuccess );
    percentile = min( percentileMin + percentileRes, percentileMax );
    wprintf( L"      " );
    sample = 0;
    percentileT = percentile;
    while ( cSegments > 0 )
    {
        ULONG percentileActual = percentileT;

        errStats = histogram.ErrGetPercentileHits( &percentileActual, &sample );
        if ( errStats == CStats::ERR::wrnOutOfSamples )
        {
            fPrintSample = true;
            fFoundSample = false;
        }
        else
        {
            Enforce( errStats == CStats::ERR::errSuccess );

            if ( percentileActual > percentile )
            {

                percentileT--;
                fPrintSample = percentileT == percentileMin;
                fFoundSample = false;
            }
            else
            {
                fPrintSample = true;
                fFoundSample = true;
            }
        }

        if ( fPrintSample )
        {
            if ( fFoundSample )
            {
                wprintf( L"%I64u\t", sample );
            }
            else
            {
                wprintf( L"NoSamp\t" );
            }
            percentile += percentileRes;
            percentile = min( percentile, percentileMax );
            percentileT = percentile;
            fPrintSample = false;
            fFoundSample = false;
            cSegments--;
        }
    }
    wprintf( L"\n" );

    errStats = histogram.ErrReset();
    Enforce( errStats == CStats::ERR::errSuccess );
}

void PageEvictionEmulator::TouchPage_( PAGEENTRY* const ppge, const BFTRACE::BFTouch_& bftouch )
{
    Enforce( ppge->ppage != NULL );

    if ( !bftouch.fNewPage )
    {
        m_stats[ ppge->ppage->ifmppgno.ifmp ].cTouchesSim++;
    }
}

void PageEvictionEmulator::DirtyPage_( PAGEENTRY* const ppge )
{
    Enforce( ppge->ppage != NULL );
    ppge->fDirty = true;
    ppge->cDirties++;
}

void PageEvictionEmulator::WritePage_( PAGEENTRY* const ppge, const IOREASONPRIMARY iorp, const LGPOS& lgposCurrent )
{
    Enforce( ppge->ppage != NULL );

    PageEvictionEmulator::STATS& stats = m_stats[ ppge->ppage->ifmppgno.ifmp ];
    stats.cWritesSim++;
    switch ( iorp )
    {
        case iorpBFCheckpointAdv:
            stats.cWritesSimChkpt++;
            break;

        case iorpBFAvailPool:
            stats.cWritesSimAvailPool++;
            break;

        case iorpBFShrink:
            stats.cWritesSimShrink++;
            break;
    }

    CleanPage_( ppge, lgposCurrent );
}

void PageEvictionEmulator::CleanPage_( PAGEENTRY* const ppge, const LGPOS& lgposClean )
{
    Enforce( ppge->ppage != NULL );
    Enforce( ppge->fDirty );
    ppge->fDirty = false;
    ppge->lgposCleanLast = lgposClean;


    const IFMP ifmp = ppge->ppage->ifmppgno.ifmp;
    std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];

    while ( !pDirtyPageOps->empty() )
    {
        const DirtyPageOp& dirtyPageOp = pDirtyPageOps->front();
        const PGNO pgnoOldest = dirtyPageOp.pgno;
        const IFMPPGNO ifmppgnoOldest( ifmp, pgnoOldest );
        PAGEENTRY* const ppgeOldest = PpgeGetEntry_( ifmppgnoOldest );

        if ( ppgeOldest->fDirty && ( CmpLgpos( ppgeOldest->lgposCleanLast, dirtyPageOp.lgpos ) <= 0 ) )
        {
            break;
        }

        pDirtyPageOps->pop_front();
    }
}

void PageEvictionEmulator::SupercoldPage_( PAGEENTRY* const ppge )
{
    Enforce( ppge->ppage != NULL );
    Enforce( m_fReplaySuperCold );
    m_stats[ ppge->ppage->ifmppgno.ifmp ].cSuperColdedSim++;
}

void PageEvictionEmulator::EvictPage_( PAGEENTRY* const ppge, const IOREASONPRIMARY iorp, const LGPOS& lgposCurrent )
{
    PAGE* const ppage = ppge->ppage;

    Enforce( ppage != NULL );

    if ( ppge->fDirty )
    {
        WritePage_( ppge, iorp, lgposCurrent );
    }

    Enforce( m_statsAgg.cpgCached >= 0 );
    m_statsAgg.cpgCached--;
    const IFMP ifmp = ppge->ppage->ifmppgno.ifmp;
    Enforce( m_stats[ ifmp ].cpgCached >= 0 );
    m_stats[ ifmp ].cpgCached--;
    m_stats[ ifmp ].cEvictionsSim++;
    const TICK dtickCachedSim = ppge->dtickCachedSim;
    ppge->dtickCachedSim += ( s_tickCurrent - ppage->tickLastFaultSim );
    Enforce( ppge->dtickCachedSim >= dtickCachedSim );


    Enforce( !ppge->fDirty );
    ppge->ppage = NULL;
    FreePage_( ppage );
}

ERR PageEvictionEmulator::ErrProcessTraceInit_( const BFTRACE& bftrace )
{
    ERR err = JET_errSuccess;

    Enforce( bftrace.traceid == bftidSysResMgrInit );


    if ( m_fResMgrInit )
    {
        FakeProcessTraceTerm_();
        Enforce( !m_fResMgrInit );
    }


    for ( size_t ifmp = 0; ifmp < m_arrayDirtyPageOps.Size(); ifmp++ )
    {
        const std::list<DirtyPageOp>* const pDirtyPageOps = m_arrayDirtyPageOps[ ifmp ];
        Enforce( pDirtyPageOps->empty() );
    }


    const BFTRACE::BFSysResMgrInit_* const pbfinit = &bftrace.bfinit;
    BFTRACE::BFSysResMgrInit_ bfinit;
    bfinit.K = m_bfinit.K >= 0 ? m_bfinit.K : pbfinit->K;
    bfinit.csecCorrelatedTouch = m_bfinit.csecCorrelatedTouch >= 0.0 ? m_bfinit.csecCorrelatedTouch : pbfinit->csecCorrelatedTouch;
    bfinit.csecTimeout = m_bfinit.csecTimeout >= 0.0 ? m_bfinit.csecTimeout : pbfinit->csecTimeout;


    bfinit.csecUncertainty = pbfinit->csecUncertainty;
    bfinit.dblHashLoadFactor = pbfinit->dblHashLoadFactor;
    bfinit.dblHashUniformity = pbfinit->dblHashUniformity;
    bfinit.dblSpeedSizeTradeoff = pbfinit->dblSpeedSizeTradeoff;


    Call( m_pipeaImplementation->ErrInit( bfinit ) );
    m_fResMgrInit = true;

HandleError:

    return err;
}

ERR PageEvictionEmulator::ErrFakeProcessTraceInit_()
{
    ERR err = JET_errSuccess;

    Enforce( m_peecsp != peecspVariable );

    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidSysResMgrInit;
    bftrace.tick = s_tickCurrent;
    bftrace.bfinit.K = 2;
    bftrace.bfinit.csecCorrelatedTouch = 0.128;
    bftrace.bfinit.csecTimeout = 100.0;
    bftrace.bfinit.csecUncertainty = 1.0;
    bftrace.bfinit.dblHashLoadFactor = 5.0;
    bftrace.bfinit.dblHashUniformity = 1.0;
    bftrace.bfinit.dblSpeedSizeTradeoff = 0.0;

    Call( ErrProcessTraceInit_( bftrace ) );

    Enforce( m_fResMgrInit );
    m_statsAgg.cResMgrAbruptCycles++;

HandleError:

    return err;
}

void PageEvictionEmulator::ProcessTraceTerm_( const BFTRACE& bftrace )
{
    Enforce( m_fResMgrInit );
    Enforce( bftrace.traceid == bftidSysResMgrTerm );

    PurgeCache_();
    m_pipeaImplementation->Term( bftrace.bfterm );
    m_fResMgrInit = false;
    m_statsAgg.cResMgrCycles++;
}

void PageEvictionEmulator::FakeProcessTraceTerm_()
{
    BFTRACE bftrace = { 0 };
    bftrace.traceid = bftidSysResMgrTerm;
    bftrace.tick = s_tickCurrent;
    ProcessTraceTerm_( bftrace );
    Enforce( !m_fResMgrInit );
    m_statsAgg.cResMgrAbruptCycles++;
}

ERR PageEvictionEmulator::ErrProcessTraceCache_( BFTRACE& bftrace )
{
    ERR err = JET_errSuccess;

    Enforce( bftrace.traceid == bftidCache );

    BFTRACE::BFCache_ bfcache = bftrace.bfcache;
    const IFMPPGNO ifmppgno( bfcache.ifmp, bfcache.pgno );
    PAGEENTRY* const ppge = PpgeGetEntry_( ifmppgno );
    PageEvictionEmulator::STATS& stats = m_stats[ ifmppgno.ifmp ];

    if ( ( g_ifmpFilter != g_ifmpInvalid ) && ( bfcache.ifmp != g_ifmpFilter ) )
    {
        Enforce( bfcache.ifmp != g_ifmpInvalid );
        goto HandleError;
    }

    Expected( !!bfcache.fUseHistory == !bfcache.fNewPage );

    stats.cCaches++;

    if ( bfcache.fDBScan )
    {
        stats.cCachesDbScan++;

        if ( !m_fReplayDbScan )
        {
            goto HandleError;
        }
    }


    if ( !bfcache.fNewPage )
    {
        stats.cFaultsReal++;


        if ( ppge->cRequests > 0 )
        {
            stats.cFaultsRealAvoidable++;
        }

        ppge->cFaultsReal++;
        Enforce( ppge->cFaultsReal > 0 );
    }
    else
    {
        stats.cCachesNew++;
    }

    if ( ppge->tickLastCachedReal != 0 )
    {
        const TICK dtickCachedReal = ppge->dtickCachedReal;
        ppge->dtickCachedReal += ( s_tickCurrent - ppge->tickLastCachedReal );
        Enforce( ppge->dtickCachedReal >= dtickCachedReal );
    }

    ppge->tickLastCachedReal = s_tickCurrent;

    ppge->cRequests++;
    Enforce( ppge->cRequests > 0 );
    stats.cRequested++;


    if ( !m_fReplayCachePriority )
    {
        bfcache.pctPri = g_pctCachePriorityDflt;
    }
    else
    {
        ULONG pctPriOverride = ulMax;

        if ( bfcache.ifmp == g_ifmpCachePriFilter )
        {
            Enforce( g_ifmpCachePriFilter != g_ifmpInvalid );
            Enforce( pctPriOverride == ulMax );
            pctPriOverride = g_pctIfmpCachePriFilter;
        }

        if ( bfcache.clientType == g_clientCachePriFilter )
        {
            Enforce( g_clientCachePriFilter != g_clientInvalid );
            pctPriOverride = ( pctPriOverride == ulMax ) ? g_pctClientCachePriFilter : min( pctPriOverride, g_pctClientCachePriFilter );
        }

        if ( pctPriOverride != ulMax )
        {
            bfcache.pctPri = pctPriOverride;
        }
    }


    PAGE* const ppage = ppge->ppage;
    if ( ppage != NULL )
    {

        BFTRACE::BFTouch_ bftouch = { 0 };
        bftouch.ifmp = bfcache.ifmp;
        bftouch.pgno = bfcache.pgno;
        bftouch.bflt= bfcache.bflt;
        bftouch.clientType = bfcache.clientType;
        bftouch.pctPri = bfcache.pctPri;
        bftouch.fUseHistory = bfcache.fUseHistory;
        bftouch.fNewPage = bfcache.fNewPage;
        bftouch.fNoTouch = fFalse;


        Expected( !!bftouch.fUseHistory == !bftouch.fNewPage );

        TouchPage_( ppge, bftouch );
        Call( m_pipeaImplementation->ErrTouchPage( ppage, bftouch ) );
        stats.cCachesTurnedTouch++;

        if ( bfcache.fNewPage )
        {
            stats.cCachesNewAlreadyCached++;
        }
    }
    else
    {

        Call( ErrCachePage_( ppge, ifmppgno, !!bfcache.fNewPage ) );
        Call( m_pipeaImplementation->ErrCachePage( ppge->ppage, bfcache ) );
    }

    Enforce( ppge->ppage != NULL );
    ppge->ppage->tickLastRequestSim = s_tickCurrent;

HandleError:

    return err;
}

ERR PageEvictionEmulator::ErrProcessTraceTouch_( BFTRACE& bftrace )
{
    ERR err = JET_errSuccess;

    Enforce( bftrace.traceid == bftidTouch || bftrace.traceid == bftidTouchLong );

    BFTRACE::BFTouch_ bftouch = bftrace.bftouch;
    const IFMPPGNO ifmppgno( bftouch.ifmp, bftouch.pgno );
    PAGEENTRY* const ppge = PpgeGetEntry_( ifmppgno );
    PageEvictionEmulator::STATS& stats = m_stats[ ifmppgno.ifmp ];

    if ( ( g_ifmpFilter != g_ifmpInvalid ) && ( bftouch.ifmp != g_ifmpFilter ) )
    {
        Enforce( bftouch.ifmp != g_ifmpInvalid );
        goto HandleError;
    }

    stats.cTouches++;

    if ( bftouch.fDBScan )
    {
        stats.cTouchesDbScan++;

        if ( !m_fReplayDbScan )
        {
            goto HandleError;
        }
    }

    if ( bftouch.fNoTouch )
    {
        stats.cTouchesNoTouch++;
    }
    else if ( !bftouch.fNewPage )
    {
        stats.cTouchesReal++;
    }

    if ( bftouch.fNewPage )
    {
        stats.cTouchesNew++;
    }

    Expected( !!bftouch.fUseHistory == !bftouch.fNewPage );

    ppge->cRequests++;
    Enforce( ppge->cRequests > 0 );
    stats.cRequested++;


    if ( !m_fReplayCachePriority )
    {
        bftouch.pctPri = g_pctCachePriorityDflt;
    }
    else
    {
        ULONG pctPriOverride = ulMax;

        if ( bftouch.ifmp == g_ifmpCachePriFilter )
        {
            Enforce( g_ifmpCachePriFilter != g_ifmpInvalid );
            Enforce( pctPriOverride == ulMax );
            pctPriOverride = g_pctIfmpCachePriFilter;
        }

        if ( bftouch.clientType == g_clientCachePriFilter )
        {
            Enforce( g_clientCachePriFilter != g_clientInvalid );
            pctPriOverride = ( pctPriOverride == ulMax ) ? g_pctClientCachePriFilter : min( pctPriOverride, g_pctClientCachePriFilter );
        }

        if ( pctPriOverride != ulMax )
        {
            bftouch.pctPri = pctPriOverride;
        }
    }


    PAGE* const ppage = ppge->ppage;
    if ( ppage != NULL )
    {

        if ( bftouch.fNoTouch && !m_fReplayNoTouch )
        {
            stats.cTouchesNoTouchDropped++;
            goto HandleError;
        }


        Expected( !!bftouch.fUseHistory == !bftouch.fNewPage );


        TouchPage_( ppge, bftouch );
        Call( m_pipeaImplementation->ErrTouchPage( ppage, bftouch ) );
    }
    else
    {

        BFTRACE::BFCache_ bfcache = { 0 };
        bfcache.ifmp = bftouch.ifmp;
        bfcache.pgno = bftouch.pgno;
        bfcache.bflt= bftouch.bflt;
        bfcache.clientType = bftouch.clientType;
        bfcache.pctPri = bftouch.pctPri;
        bfcache.fUseHistory = bftouch.fUseHistory;
        bfcache.fNewPage = bftouch.fNewPage;
        Call( ErrCachePage_( ppge, ifmppgno, !!bfcache.fNewPage ) );
        Call( m_pipeaImplementation->ErrCachePage( ppge->ppage, bfcache ) );
        stats.cTouchesTurnedCache++;

        if ( bftouch.fNoTouch && !m_fReplayNoTouch )
        {
            stats.cTouchesNoTouchTurnedCache++;
        }
    }

    Enforce( ppge->ppage != NULL );
    ppge->ppage->tickLastRequestSim = s_tickCurrent;

HandleError:

    return err;
}

ERR PageEvictionEmulator::ErrProcessTraceSuperCold_( const BFTRACE& bftrace )
{
    ERR err = JET_errSuccess;

    Enforce( bftrace.traceid == bftidSuperCold );

    const BFTRACE::BFSuperCold_& bfsupercold = bftrace.bfsupercold;
    const IFMPPGNO ifmppgno( bfsupercold.ifmp, bfsupercold.pgno );
    PAGEENTRY* const ppge = PpgeGetEntry_( ifmppgno );

    m_stats[ ifmppgno.ifmp ].cSuperColdedReal++;


    PAGE* const ppage = ppge->ppage;
    if ( ppage != NULL && m_fReplaySuperCold )
    {
        SupercoldPage_( ppge );
        Call( m_pipeaImplementation->ErrSuperColdPage( ppage, bfsupercold ) );
    }

HandleError:

    return err;
}

ERR PageEvictionEmulator::ErrProcessTraceEvict_( BFTRACE& bftrace )
{
    ERR err = JET_errSuccess;

    Enforce( bftrace.traceid == bftidEvict );

    BFTRACE::BFEvict_& bfevict = bftrace.bfevict;

    if ( !bfevict.fCurrentVersion )
    {
        return JET_errSuccess;
    }

    const IFMPPGNO ifmppgno( bfevict.ifmp, bfevict.pgno );
    PAGEENTRY* const ppgeOriginal = PpgeGetEntry_( ifmppgno );

    m_stats[ ifmppgno.ifmp ].cEvictionsReal++;

    if ( ppgeOriginal->tickLastCachedReal != 0 )
    {
        const TICK dtickCachedReal = ppgeOriginal->dtickCachedReal;
        ppgeOriginal->dtickCachedReal += ( s_tickCurrent - ppgeOriginal->tickLastCachedReal );
        Enforce( ppgeOriginal->dtickCachedReal >= dtickCachedReal );
        ppgeOriginal->tickLastCachedReal = 0;
    }


    if ( m_peecsp != peecspVariable )
    {
        goto HandleError;
    }


    const bool fEvictNext = ( ( bfevict.bfef & bfefReasonMask ) == bfefReasonAvailPool ) ||
                            ( ( ( bfevict.bfef & bfefReasonMask ) == bfefReasonShrink ) && m_fEvictNextOnShrink ) || 
                            ( ppgeOriginal->ppage == NULL );
    const IOREASONPRIMARY iorp = ( ( bfevict.bfef & bfefReasonMask ) == bfefReasonAvailPool ) ? iorpBFAvailPool :
                            ( ( ( bfevict.bfef & bfefReasonMask ) == bfefReasonShrink ) ? iorpBFShrink :
                            ( ( ( ( bfevict.bfef & bfefReasonMask ) == bfefReasonPurgeContext ) ||
                                ( ( bfevict.bfef & bfefReasonMask ) == bfefReasonPurgePage ) ) ? iorpBFPurge : iorpBFDatabaseFlush ) );


    if ( !m_fReplayCachePriority )
    {
        bfevict.pctPri = g_pctCachePriorityDflt;
    }
    else
    {
        if ( bfevict.ifmp == g_ifmpCachePriFilter )
        {
            Enforce( g_ifmpCachePriFilter != g_ifmpInvalid );
            bfevict.pctPri = g_pctIfmpCachePriFilter;
        }
    }


    if ( fEvictNext )
    {
        BYTE pv[ max( sizeof(PAGE*), sizeof(IFMPPGNO) ) ];
        PageEvictionContextType pectyp;
        Call( m_pipeaImplementation->ErrEvictNextPage( pv, bfevict, &pectyp ) );

        PAGEENTRY* const ppgeEvict = PpgeGetEntry_( pv, pectyp );


        if ( ppgeEvict != NULL )
        {
            LGPOS lgposNewest;
            GetNewestDirtyLgpos_( ifmppgno.ifmp, &lgposNewest );
            EvictPage_( ppgeEvict, iorp, lgposNewest );
        }
        else
        {
            m_statsAgg.cEvictionsFailed++;
        }
    }
    else
    {
        PAGE* const ppage = ppgeOriginal->ppage;


        if ( ppage != NULL )
        {

            Call( m_pipeaImplementation->ErrEvictSpecificPage( ppage, bfevict ) );

            LGPOS lgposNewest;
            GetNewestDirtyLgpos_( ifmppgno.ifmp, &lgposNewest );
            EvictPage_( ppgeOriginal, iorp, lgposNewest );
        }
    }

HandleError:

    return err;
}

ERR PageEvictionEmulator::ErrProcessTraceDirty_( const BFTRACE& bftrace )
{
    Enforce( bftrace.traceid == bftidDirty );


    if ( m_fSetLgposModifySupported )
    {
        return JET_errSuccess;
    }

    LGPOS lgposModify( bftrace.bfdirty.lgenModify, bftrace.bfdirty.isecModify, bftrace.bfdirty.ibModify );
    const IFMPPGNO ifmppgno( bftrace.bfdirty.ifmp, bftrace.bfdirty.pgno );


    LGPOS lgposNewest;
    GetNewestDirtyLgpos_( ifmppgno.ifmp, &lgposNewest );
    if ( ( lgposNewest.FIsSet() && lgposModify.FIsSet() && ( CmpLgpos( lgposModify, lgposNewest ) < 0 ) ) ||
         ( lgposNewest.FIsSet() && !lgposModify.FIsSet() ) )
    {
        lgposModify.lgen = lgposNewest.lgen;
        lgposModify.isec = lgposNewest.isec;
        lgposModify.ib = lgposNewest.ib;
    }


    if ( !lgposModify.FIsSet() )
    {
        return JET_errSuccess;
    }

    ModifyPage_( ifmppgno, lgposModify );

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrProcessTraceWrite_( const BFTRACE& bftrace )
{
    Enforce( bftrace.traceid == bftidWrite );

    const BFTRACE::BFWrite_& bfwrite = bftrace.bfwrite;
    bool fReplayTrace = false;
    const IFMPPGNO ifmppgno( bfwrite.ifmp, bfwrite.pgno );
    PAGEENTRY* const ppge = PpgeGetEntry_( ifmppgno );

    m_stats[ ifmppgno.ifmp ].cWritesReal++;
    switch ( bfwrite.iorp )
    {
        case iorpBFCheckpointAdv:
            m_stats[ ifmppgno.ifmp ].cWritesRealChkpt++;
            fReplayTrace = false;
            break;

        case iorpBFAvailPool:
            m_stats[ ifmppgno.ifmp ].cWritesRealAvailPool++;
            fReplayTrace = ( m_peecsp == peecspVariable );
            break;

        case iorpBFShrink:
            m_stats[ ifmppgno.ifmp ].cWritesRealShrink++;
            fReplayTrace = ( m_peecsp == peecspVariable );
            break;

        default:
            fReplayTrace = true;
            break;
    }


    if ( fReplayTrace && ppge->fDirty )
    {
        PAGE* const ppage = ppge->ppage;

        if ( ppage != NULL )
        {
            LGPOS lgposNewest;
            GetNewestDirtyLgpos_( bfwrite.ifmp, &lgposNewest );
            WritePage_( ppge, (IOREASONPRIMARY)bfwrite.iorp, lgposNewest );
        }
    }

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrProcessTraceSetLgposModify_( const BFTRACE& bftrace )
{
    Enforce( bftrace.traceid == bftidSetLgposModify );

    m_fSetLgposModifySupported = true;

    LGPOS lgposModify( bftrace.bfsetlgposmodify.lgenModify, bftrace.bfsetlgposmodify.isecModify, bftrace.bfsetlgposmodify.ibModify );
    const IFMPPGNO ifmppgno( bftrace.bfsetlgposmodify.ifmp, bftrace.bfsetlgposmodify.pgno );

    Enforce( lgposModify.FIsSet() );

    ModifyPage_( ifmppgno, lgposModify );

    return JET_errSuccess;
}

ERR PageEvictionEmulator::ErrProcessTraceDefault_( const BFTRACE& bftrace )
{
    Enforce( bftrace.traceid != bftidSysResMgrInit );
    Enforce( bftrace.traceid != bftidSysResMgrTerm );
    Enforce( bftrace.traceid != bftidCache );
    Enforce( bftrace.traceid != bftidTouch );
    Enforce( bftrace.traceid != bftidDirty );
    Enforce( bftrace.traceid != bftidWrite );
    Enforce( bftrace.traceid != bftidSetLgposModify );
    Enforce( bftrace.traceid != bftidSuperCold );
    Enforce( bftrace.traceid != bftidEvict );

    m_statsAgg.cDiscardedTraces++;

    return JET_errSuccess;
}

void PageEvictionEmulator::MaintainAgeBasedCache_()
{
    while ( true )
    {
        BYTE pv[ max( sizeof(PAGE*), sizeof(IFMPPGNO) ) ];
        PageEvictionContextType pectyp;
        const ERR errNextToEvict = m_pipeaImplementation->ErrGetNextPageToEvict( pv, &pectyp );
        Enforce( errNextToEvict == JET_errSuccess );
        PAGEENTRY* const ppgeNextToEvict = PpgeGetEntry_( pv, pectyp );
        const bool fNothingToEvict = ( ppgeNextToEvict == NULL );
        Enforce( fNothingToEvict == ( m_statsAgg.cpgCached == 0 ) );


        if ( fNothingToEvict )
        {
            break;
        }


        if ( (LONG)( s_tickCurrent - ppgeNextToEvict->ppage->tickLastRequestSim ) <= (LONG)m_dtickCacheAge )
        {
            break;
        }


        BFTRACE::BFEvict_ bfevict = { 0 };
        bfevict.fCurrentVersion = fTrue;
        bfevict.errBF = JET_errSuccess;
        bfevict.bfef = bfefReasonAvailPool | bfefKeepHistory;
        bfevict.pctPri = g_pctCachePriorityDflt;

        const ERR errEvict = m_pipeaImplementation->ErrEvictNextPage( pv, bfevict, &pectyp );
        Enforce( errEvict == JET_errSuccess );

        PAGEENTRY* const ppgeEvict = PpgeGetEntry_( pv, pectyp );
        Enforce( ppgeNextToEvict == ppgeEvict );

        LGPOS lgposNewest;
        const IFMP ifmp = ppgeEvict->ppage->ifmppgno.ifmp;
        GetNewestDirtyLgpos_( ifmp, &lgposNewest );
        EvictPage_( ppgeEvict, iorpBFAvailPool, lgposNewest );

        m_stats[ ifmp ].cEvictionsCacheTooOld++;
    }
}


bool SimulationIterator::FSimulationSampleExists( const DWORD dwIterationValue ) const
{
    return ( m_samples.find( dwIterationValue ) != m_samples.end() );
}

void SimulationIterator::AddSimulationSample( const DWORD dwIterationValue, const QWORD qwSampleValue )
{
    Enforce( !FSimulationSampleExists( dwIterationValue ) );
    m_samples.insert( SimulationSample( dwIterationValue, qwSampleValue ) );
}

bool SimulationIterator::FGetSimulationSample(
        const size_t iSample,
        DWORD* const pdwIterationValue,
        QWORD* const pqwSampleValue ) const
{
    if ( m_samples.size() <= iSample )
    {
        return false;
    }

    Enforce( pdwIterationValue != NULL );
    Enforce( pqwSampleValue != NULL );

    size_t i = 0;

    for ( SimulationSamplesIter iter = m_samples.begin(); iter != m_samples.end(); iter++, i++ )
    {
        if ( i == iSample )
        {
            *pdwIterationValue = iter->first;
            *pqwSampleValue = iter->second;

            break;
        }
    }

    return true;
}



FixedCacheSimulationPresetIterator::FixedCacheSimulationPresetIterator( std::set<DWORD>& cacheSizes )
{
    m_cacheSizeIterator = cacheSizes.begin();
    m_cacheSizeIteratorEnd = cacheSizes.end();
}

DWORD FixedCacheSimulationPresetIterator::DwGetNextIterationValue()
{
    if ( m_cacheSizeIterator == m_cacheSizeIteratorEnd )
    {
        return ulMax;
    }

    const DWORD dwNext = *m_cacheSizeIterator;
    m_cacheSizeIterator++;
    return dwNext;
}



DWORD FixedCacheSimulationIterator::DwGetNextIterationValue()
{

    if ( m_samples.size() < 2 )
    {
        return ulMax;
    }

    bool fFound = false;


    SimulationSamplesIter iter = m_samples.begin();
    DWORD cbfCacheSize1 = 0, cbfCacheSize2 = 0, cbfCacheSize1Found = 0, cbfCacheSize2Found = 0, dcbfCacheSizeFound = 0;
    QWORD cFaults1 = 0, cFaults2 = 0, cFaults1Found = 0, cFaults2Found = 0, dcFaultsFound = 0;
    cbfCacheSize2 = iter->first;
    cFaults2 = iter->second;
    Enforce( cbfCacheSize2 != ulMax );


    while ( ++iter != m_samples.end() )
    {
        cbfCacheSize1 = cbfCacheSize2;
        cFaults1 = cFaults2;
        cbfCacheSize2 = iter->first;
        cFaults2 = iter->second;
        Enforce( cbfCacheSize2 != ulMax );
        Enforce( cbfCacheSize1 < cbfCacheSize2 );


        if ( !( cbfCacheSize2 > ( cbfCacheSize1 + 1 ) ) )
        {
            continue;
        }


        const QWORD dcFaults = absdiff( cFaults2, cFaults1 );

        if ( fFound && ( dcFaults < dcFaultsFound ) )
        {
            continue;
        }

        const DWORD dcbfCacheSize = cbfCacheSize2 - cbfCacheSize1;


        if ( !fFound || ( dcFaults > dcFaultsFound ) || ( dcbfCacheSize > dcbfCacheSizeFound ) )
        {
            fFound = true;
            cbfCacheSize1Found = cbfCacheSize1;
            cbfCacheSize2Found = cbfCacheSize2;
            dcbfCacheSizeFound = cbfCacheSize2Found - cbfCacheSize1Found;
            cFaults1Found = cFaults1;
            cFaults2Found = cFaults2;
            dcFaultsFound = absdiff( cFaults2Found, cFaults1Found );
        }
    }

    if ( !fFound )
    {
        return ulMax;
    }

    Enforce( cbfCacheSize1Found < cbfCacheSize2Found );

    return ( cbfCacheSize1Found + ( cbfCacheSize2Found - cbfCacheSize1Found ) / 2 );
}



CacheFaultLookupSimulationIterator::CacheFaultLookupSimulationIterator( const QWORD cFaultsTarget ) :
    m_cFaultsTarget( cFaultsTarget )
{
}

DWORD CacheFaultLookupSimulationIterator::DwGetNextIterationValue()
{


    SimulationSamplesIter iter = m_samples.begin();

    if ( m_samples.size() < 2 )
    {
        return ulMax;
    }


    DWORD cbfCacheSize1 = 0, cbfCacheSize2 = 0;
    QWORD cFaults1 = 0, cFaults2 = 0, cFaultsMin = 0, cFaultsMax = 0;
    cbfCacheSize2 = iter->first;
    cFaults2 = iter->second;
    Enforce( cbfCacheSize2 != ulMax );


    if ( cFaults2 == m_cFaultsTarget )
    {
        return cbfCacheSize2;
    }

    while ( ++iter != m_samples.end() )
    {
        cbfCacheSize1 = cbfCacheSize2;
        cFaults1 = cFaults2;
        cbfCacheSize2 = iter->first;
        cFaults2 = iter->second;
        Enforce( cbfCacheSize2 != ulMax );
        Enforce( cbfCacheSize1 < cbfCacheSize2 );


        if ( cFaults2 == m_cFaultsTarget )
        {
            return cbfCacheSize2;
        }

        if ( cFaults1 < cFaults2 )
        {
            cFaultsMin = cFaults1;
            cFaultsMax = cFaults2;
        }
        else
        {
            cFaultsMin = cFaults2;
            cFaultsMax = cFaults1;
        }

        Enforce( cFaultsMin <= cFaultsMax );


        if ( ( m_cFaultsTarget < cFaultsMin ) || ( m_cFaultsTarget > cFaultsMax ) )
        {
            continue;
        }

        if ( cFaults1 < cFaults2 )
        {
            cFaultsMin = cFaults1;
            cFaultsMax = cFaults2;
        }
        else
        {
            cFaultsMin = cFaults2;
            cFaultsMax = cFaults1;
        }


        return ( cbfCacheSize1 + ( cbfCacheSize2 - cbfCacheSize1 ) / 2 );
    }

    return ulMax;
}


CacheFaultMinSimulationIterator::CacheFaultMinSimulationIterator( const DWORD cIter, const DWORD dtickIterMax ) :
    m_cIter( cIter ),
    m_dtickIterMax( dtickIterMax ),
    m_q( -1.0 )
{
}

void CacheFaultMinSimulationIterator::AddSimulationSample( const DWORD dwIterationValue, const QWORD cFaults )
{
    SimulationIterator::AddSimulationSample( dwIterationValue, cFaults );


    if ( m_q > 0.0 )
    {
        return;
    }

    const size_t cSamples = m_samples.size();
    Enforce( ( cSamples == 1 ) || ( cSamples == 2 ) );
    SimulationSamplesIter iter = m_samples.begin();

    if ( ( cSamples == 1 ) && ( iter->first == 0 ) )
    {
        return;
    }

    if ( cSamples == 2 )
    {
        iter++;
    }

    m_q = pow( (double)m_dtickIterMax / (double)iter->first, 1.0 / (double)( m_cIter - cSamples ) );
    Enforce( m_q > 1.0 );
}

DWORD CacheFaultMinSimulationIterator::DwGetNextIterationValue()
{


    if ( ( m_q < 0.0 ) || ( m_samples.size() >= m_cIter ) )
    {
        return ulMax;
    }


    SimulationSamplesIter iter = m_samples.end();
    iter--;

    const DWORD dwLastIter = iter->first;
    const DWORD dwNextIter = (DWORD)( m_q * (double)dwLastIter + 0.5 );
    Enforce( dwNextIter >= dwLastIter );


    return ( dwNextIter - dwLastIter < 16 ) ? ( dwLastIter + 16 ) : dwNextIter;
}


DWORD ChkptDepthSimulationIterator::DwGetNextIterationValue()
{
    return FixedCacheSimulationIterator::DwGetNextIterationValue();
}


