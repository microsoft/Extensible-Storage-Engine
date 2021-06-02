// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgrreplay.hxx"

//  ================================================================
//  Helper Functions

static void PrintHelp( const WCHAR * const wszApplication )
{
    fprintf( stderr, "Usage: %ws <[/Ftl <FtlFile1>[[,<FtlFile2>][,<FtlFile3>]...]] | [/Etl <EtlFile>]> [Options]\n", wszApplication );
    fprintf( stderr, "  /Ftl <FtlFile1>[[,<FtlFile2>][,<FtlFile3>]...: specifies one or multple FTL trace log files with the BF/resmgr traces.\n" );
    fprintf( stderr, "  /Etl <EtlFile>: specifies the ETL trace log file with the BF/resmgr traces.\n" );
    fprintf( stderr, "  Options:\n" );
    fprintf( stderr, "    /Dump: indicates we should dump the entire trace file. (/Ftl only)\n" );
    fprintf( stderr, "    /PostProcess: post-processes an FTL file (required prior to to running simulations). (/Ftl only)\n" );
    fprintf( stderr, "    /Ftlstats: indicates we should accumulate and print generic FTL statistics. (/Ftl only)\n" );
    fprintf( stderr, "    /BfStats: indicates we should accumulate and print generic BF statistics. (/Ftl or /Etl)\n" );
    fprintf( stderr, "    /FromEtl <ETL file> <PID1>[,<PID2>][,<PID3>]...: generates an FTL file from the provided ETL file (PID of zero means \"all processes\"). (/Ftl only)\n" );
    fprintf( stderr, "    /CmpEtl <ETL file> <PID>: compares and FTL file against an ETL file (PID of zero means \"all processes\"). (/Ftl only)\n" );
    fprintf( stderr, "    /Emulate <Algorithm> [Options]: indicates we should run through an emulator/ Supported algorithms: LRUTest, LRUKTest, LRUKESE, Beladys, BeladysInverse. (/Ftl only)\n" );
    fprintf( stderr, "    Options:\n" );
    fprintf( stderr, "      /K [Value]: k-ness of the LRU-K algorithm (LRUKTest and LRUKESE only).\n" );
    fprintf( stderr, "      /CorrelatedTouch [Value]: correlation interval of the LRU-K algorithm (in seconds) (LRUKTest and LRUKESE only).\n" );
    fprintf( stderr, "      /Timeout [Value]: additional boost given to multi-touched resources in the the LRU-K algorithm (LRUKESE only).\n" );
    fprintf( stderr, "    /DontReplSupercold: super-cold traces should not be replayed. (/Emulate only)\n" );
    fprintf( stderr, "    /DontReplCachePri: traced cache priority should not be input into the simulation. (/Emulate only)\n" );
    fprintf( stderr, "    /ReplNoTouch: no-touch requests should be replayed as normal requests. (/Emulate only)\n" );
    fprintf( stderr, "    /DontReplDbScan: DBM-related requests should not be replayed. (/Emulate only)\n" );
    fprintf( stderr, "    /DontEvictNextOnShrink: simulation should not evict the next target when shrinking. (/Emulate only)\n" );
    fprintf( stderr, "    /DontReplInitTerm: Init and Term ResMgr events should be ignored. This avoids the simulated cache to be purged on init/term cycles, to simulate very long trace is being processed. (/Emulate only)\n" );
    fprintf( stderr, "    /CacheSize <Size1>[,<Size2>][,<Size3>]...: iterates through all cache sizes, running the simulation with fixed numbers of resources, i.e., not following eviction patterns (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CacheAge <AgeInSeconds>: sizes the cache accordingly to keep only resources which are newer than a certain age, i.e., eviction does not follow eviction patterns (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CacheSizeIteration <SizeMin> <SizeMax> <Count>: iterates between min/max using total number of faults, running the /CacheSize option (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CacheSizeIterationAvoidable <SizeMin> <SizeMax> <Count>: iterates between min/max using avoidable number of faults, running the /CacheSize option (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CacheFaultIteration <SizeMin> <SizeMax> <Faults>: iterates between min/max using total number of faults, looking for the cache size that provides that number of faults (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CacheFaultIterationAvoidable <SizeMin> <SizeMax> <Faults>: iterates between min/max using avoidable number of faults, looking for the cache size that provides that number of faults (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CorrelatedTouchIteration <CacheSize> <CorrTouchMin> <CorrTouchMax> <Count>: iterates between min/max using total number of faults, running the /CorrelatedTouch option (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CorrelatedTouchIterationAvoidable <CacheSize> <CorrTouchMin> <CorrTouchMax> <Count>: iterates between min/max using total number of faults, running the /CorrelatedTouch option (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /ChkptDepth <Depth>: checkpoint depth used in the simulation, in number of logs (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /ChkptDepthIteration <CacheSize> <DepthMin> <DepthMax> <Count>: iterates between min/max using total number of writes, running the /ChkptDepth option (/Emulate only, > 0).\n" );
    fprintf( stderr, "    /CacheCountRes <Res>: resolution to be used in the cache-count histograms (default %hu). (/Emulate only)\n", g_cCachedHistoResDefault );
    fprintf( stderr, "    /CacheLifetimeRes <Res>: resolution to be used in the cache-lifetime histograms, in msec (default %lu). (/Emulate only)\n", g_dtickLifetimeHistoResDefault );
    fprintf( stderr, "    /PrintSamples <IntervalInSec>: prints out simulation samples, in sec (default: do not print samples). (/Emulate only)\n" );
    fprintf( stderr, "    /NoHisto: do not print out histograms. (default: print histograns). (/Emulate only)\n" );
    fprintf( stderr, "    /Ifmp <IfmpFilter>: specific IFMP to report results on (default: all IFMPs). Note, the simulation is always performed with all IFMPs. (/Emulate only)\n" );
}

//  ================================================================
//  Main

INT _cdecl wmain( INT argc, __in_ecount(argc) LPWSTR argv[] )
{
    ERR err = JET_errSuccess;
    BFFTLContext* pbfftlc = NULL;
    BFETLContext* pbfetlc = NULL;
    IPageEvictionAlgorithmImplementation* pipea = NULL;
    SimulationIterator* psimulationIterator = NULL;
    FixedCacheSimulationIterator* psimulationFaultSamples = NULL;
    FixedCacheSimulationIterator* psimulationFaultAvoidableSamples = NULL;
    ChkptDepthSimulationIterator* psimulationWriteSamples = NULL;
    ChkptDepthSimulationIterator* psimulationWriteChkptSamples = NULL;
    ChkptDepthSimulationIterator* psimulationWriteScavengeSamples = NULL;
    SimulationIterator* psimulationCacheSizeMaxSamples = NULL;
    PageEvictionEmulator& emulator = PageEvictionEmulator::GetEmulatorObj();
    BFTRACE::BFSysResMgrInit_ bfsysresmgrinit =
    {
        -1,     // K.
        -1.0,   // csecCorrelatedTouch.
        -1.0,   // csecTimeout.
        -1.0,   // dblHashLoadFactor.
        -1.0,   // dblHashUniformity.
        -1.0    // dblSpeedSizeTradeoff.
    };
    std::set<DWORD>* pids = NULL;
    std::set<DWORD>* cacheSizes = NULL;

    //
    //  Initialize the OS Layer
    //
    //  note: not needed by resmgr.hxx directly, needed by the FTL trace log
    //  infrastructure.
    //
    COSLayerPreInit oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    //
    //  configure OS layer
    //
    Call( ErrOSInit() );

    //
    //  Process Generic Args
    //
    if( argc == 2
        && ( 0 == _wcsicmp( argv[1], L"-h" )
            || 0 == _wcsicmp( argv[1], L"/h" )
            || 0 == _wcsicmp( argv[1], L"-?" )
            || 0 == _wcsicmp( argv[1], L"/?" ) ) )
    {
        PrintHelp( argv[0] );
        wprintf( L"Bad first argument.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    // FUTURE-2010/08/26 - Do dup args and consume model, to make it easier to deal with
    // the remaining args being processed by the emulator.

    //
    //  Process args 
    //
    if( argc < 3 )
    {
        PrintHelp( argv[0] );
        wprintf( L"Insufficient number of arguments.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }


    WCHAR * wszFtlTraceLogFiles = NULL;
    WCHAR * wszEtlTraceLogFile = NULL;

    //
    //  Process the option and command args
    //
    ResMgrReplayAlgorithm rmralg        = rmralgInvalid;
    ResMgrReplayEmulationMode rmmode    = rmemInvalid;
    bool fDumpTraces                    = false;
    bool fPostProcess                   = false;
    bool fCollectFtlStats               = false;
    bool fCollectBfStats                = false;
    bool fRunResMgrEmulation            = false;
    bool fConvertFromEtl                = false;
    bool fCmpAgainstEtl                 = false;
    bool fReplaySuperCold               = true;
    bool fReplayCachePriority           = true;
    bool fReplayNoTouch                 = false;
    bool fReplayDbScan                  = true;
    bool fEvictNextOnShrink             = true;
    bool fReplayInitTerm                = true;
    bool fPrintHistograms               = true;
    USHORT cCachedHistoRes              = g_cCachedHistoResDefault;
    TICK dtickLifetimeHistoRes          = g_dtickLifetimeHistoResDefault;
    size_t ifmpFilter                   = upMax;
    DWORD clogsChkptDepth               = ulMax;
    DWORD cbfCacheSize                  = 0;
    TICK dtickCacheAge                  = 0;
    DWORD dwIterationMin                = 0;
    DWORD dwIterationMax                = 0;
    DWORD cIteration                    = 0;
    QWORD cFaultsTarget                 = 0;
    TICK dtickPrintSampleInterval       = 0;
    pids                                = new std::set<DWORD>();
    Alloc( pids );
    cacheSizes                          = new std::set<DWORD>();
    Alloc( cacheSizes );

    if ( ( 0 == _wcsicmp( argv[1], L"/Ftl" ) ) || ( 0 == _wcsicmp( argv[1], L"-Ftl" ) ) )
    {
        wszFtlTraceLogFiles = argv[2];
    }
    else if ( ( 0 == _wcsicmp( argv[1], L"/Etl" ) ) || ( 0 == _wcsicmp( argv[1], L"-Etl" ) ) )
    {
        wszEtlTraceLogFile = argv[2];
    }
    else
    {
        PrintHelp( argv[0] );
        wprintf( L"Bad first argument.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    for ( INT iarg = 3; iarg < argc; iarg++ )
    {
        if ( 0 == _wcsicmp( argv[iarg], L"/Dump" ) || 0 == _wcsicmp( argv[iarg], L"-Dump" ) )
        {
            fDumpTraces = true;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/PostProcess" ) || 0 == _wcsicmp( argv[iarg], L"-PostProcess" ) )
        {
            fPostProcess = true;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/Ftlstats" ) || 0 == _wcsicmp( argv[iarg], L"-Ftlstats" ) )
        {
            fCollectFtlStats = true;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/BfStats" ) || 0 == _wcsicmp( argv[iarg], L"-BfStats" ) )
        {
            fCollectBfStats = true;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/FromEtl" ) || 0 == _wcsicmp( argv[iarg], L"-FromEtl" ) )
        {
            fConvertFromEtl = true;

            //  ETL file must have been passed in, as well as PID.

            if ( ( iarg + 2 ) < argc )
            {
                wszEtlTraceLogFile = argv[++iarg];
                const WCHAR* wszPid = wcstok( argv[++iarg], L"," );
                while ( wszPid != NULL )
                {
                    pids->insert( (DWORD)_wtoi( wszPid ) );
                    wszPid = wcstok( NULL, L"," );
                }

                if ( pids->size() < 1 )
                {
                    wprintf( L"Too few PIDs.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Bad ETL file options.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CmpEtl" ) || 0 == _wcsicmp( argv[iarg], L"-CmpEtl" ) )
        {
            fCmpAgainstEtl = true;

            //  ETL file must have been passed in, as well as PID.

            if ( ( iarg + 2 ) < argc )
            {
                wszEtlTraceLogFile = argv[++iarg];
                pids->insert( (DWORD)_wtoi( argv[++iarg] ) );
            }
            else
            {
                wprintf( L"Bad ETL file options.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( ( 0 == _wcsicmp( argv[iarg], L"/Emulate" ) ) || ( 0 == _wcsicmp( argv[iarg], L"-Emulate" ) ) )
        {
            iarg++;

            //  algorithm must have been passed in

            if ( iarg < argc )
            {
                if ( 0 == _wcsicmp( argv[iarg], L"LRUTest" ) )
                {
                    rmralg = rmralgLruTest;
                }
                else if ( 0 == _wcsicmp( argv[iarg], L"LRUKTest" ) )
                {
                    rmralg = rmralgLrukTest;
                }
                else if ( 0 == _wcsicmp( argv[iarg], L"LRUKESE" ) )
                {
                    rmralg = rmralgLrukEse;
                }
                else if ( 0 == _wcsicmp( argv[iarg], L"Beladys" ) )
                {
                    rmralg = rmralgBeladys;
                }
                else if ( 0 == _wcsicmp( argv[iarg], L"BeladysInverse" ) )
                {
                    rmralg = rmralgBeladysInverse;
                }
                else
                {
                    wprintf( L"Unknown algorithm.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /Emulate option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            fRunResMgrEmulation = true;
            rmmode = rmemNormal;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/DontReplSupercold" ) || 0 == _wcsicmp( argv[iarg], L"-DontReplSupercold" ) )
        {
            fReplaySuperCold = false;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/DontReplCachePri" ) || 0 == _wcsicmp( argv[iarg], L"-DontReplCachePri" ) )
        {
            fReplayCachePriority = false;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/ReplNoTouch" ) || 0 == _wcsicmp( argv[iarg], L"-ReplNoTouch" ) )
        {
            fReplayNoTouch = true;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/DontReplDbScan" ) || 0 == _wcsicmp( argv[iarg], L"-DontReplDbScan" ) )
        {
            fReplayDbScan = false;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/DontEvictNextOnShrink" ) || 0 == _wcsicmp( argv[iarg], L"-DontEvictNextOnShrink" ) )
        {
            fEvictNextOnShrink = false;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/DontReplInitTerm" ) || 0 == _wcsicmp( argv[iarg], L"-DontReplInitTerm" ) )
        {
            fReplayInitTerm = false;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CacheSize" ) || 0 == _wcsicmp( argv[iarg], L"-CacheSize" ) )
        {
            if ( rmmode != rmemNormal )
            {
                wprintf( L"/CacheSize must be specified for emulation mode and not mixed with other modes.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( ( iarg + 1 ) < argc )
            {
                const WCHAR* wszCacheSize = wcstok( argv[++iarg], L"," );
                while ( wszCacheSize != NULL )
                {
                    const DWORD cbfCacheSizeT = (DWORD)_wtoi( wszCacheSize );
                    if ( cbfCacheSizeT < 1 )
                    {
                        wprintf( L"Invalid cache size.\n" );
                        Error( ErrERRCheck( JET_errInvalidParameter ) );
                    }

                    cacheSizes->insert( cbfCacheSizeT );
                    wszCacheSize = wcstok( NULL, L"," );
                }

                if ( cacheSizes->size() < 1 )
                {
                    wprintf( L"Too few cache sizes.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /CacheSize option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( cacheSizes->size() == 1 )
            {
                cbfCacheSize = *( cacheSizes->begin() );
                rmmode = rmemCacheSizeFixed;
            }
            else
            {
                rmmode = rmemCacheSizeFixedIteration;
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CacheAge" ) || 0 == _wcsicmp( argv[iarg], L"-CacheAge" ) )
        {
            if ( rmmode != rmemNormal )
            {
                wprintf( L"/CacheAge must be specified for emulation mode and not mixed with other modes.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            iarg++;
            if ( iarg < argc )
            {
                dtickCacheAge = (TICK)( _wtof( argv[iarg] ) * 1000.0 );

                if ( dtickCacheAge == 0 )
                {
                    wprintf( L"Invalid cache age option.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for this option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            rmmode = rmemCacheSizeAgeBased;
        }


        else if ( 0 == _wcsicmp( argv[iarg], L"/CacheSizeIteration" ) || 0 == _wcsicmp( argv[iarg], L"-CacheSizeIteration" ) ||
                    0 == _wcsicmp( argv[iarg], L"/CacheSizeIterationAvoidable" ) || 0 == _wcsicmp( argv[iarg], L"-CacheSizeIterationAvoidable" ) )
        {
            if ( rmmode != rmemNormal )
            {
                wprintf( L"/CacheSizeIteration and /CacheSizeIterationAvoidable must be specified for emulation mode and not mixed with other modes.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( 0 == _wcsicmp( argv[iarg], L"/CacheSizeIteration" ) || 0 == _wcsicmp( argv[iarg], L"-CacheSizeIteration" ) )
            {
                rmmode = rmemCacheSizeIteration;
            }
            else
            {
                rmmode = rmemCacheSizeIterationAvoidable;
            }

            if ( ( iarg + 3 ) < argc )
            {
                dwIterationMin = (DWORD)_wtoi( argv[++iarg] );
                dwIterationMax = (DWORD)_wtoi( argv[++iarg] );
                cIteration = (DWORD)_wtoi( argv[++iarg] );

                if ( ( dwIterationMin < 1 ) || ( dwIterationMax < 1 ) || ( cIteration < 2 ) || ( dwIterationMin > dwIterationMax ) )
                {
                    wprintf( L"Invalid cache size iteration options.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for this option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CacheFaultIteration" ) || 0 == _wcsicmp( argv[iarg], L"-CacheFaultIteration" ) ||
                    0 == _wcsicmp( argv[iarg], L"/CacheFaultIterationAvoidable" ) || 0 == _wcsicmp( argv[iarg], L"-CacheFaultIterationAvoidable" ) )
        {
            if ( rmmode != rmemNormal )
            {
                wprintf( L"/CacheFaultIteration and /CacheFaultIterationAvoidable must be specified for emulation mode and not mixed with other modes.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( 0 == _wcsicmp( argv[iarg], L"/CacheFaultIteration" ) || 0 == _wcsicmp( argv[iarg], L"-CacheFaultIteration" ) )
            {
                rmmode = rmemCacheFaultIteration;
            }
            else
            {
                rmmode = rmemCacheFaultIterationAvoidable;
            }

            if ( ( iarg + 3 ) < argc )
            {
                dwIterationMin = (DWORD)_wtoi( argv[++iarg] );
                dwIterationMax = (DWORD)_wtoi( argv[++iarg] );
                cFaultsTarget = (QWORD)_wtoi64( argv[++iarg] );

                if ( ( dwIterationMin < 1 ) || ( dwIterationMax < 1 ) || ( dwIterationMin >= dwIterationMax ) )
                {
                    wprintf( L"Invalid cache fault iteration options.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for this option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CorrelatedTouchIteration" ) || 0 == _wcsicmp( argv[iarg], L"-CorrelatedTouchIteration" ) ||
                    0 == _wcsicmp( argv[iarg], L"/CorrelatedTouchIterationAvoidable" ) || 0 == _wcsicmp( argv[iarg], L"-CorrelatedTouchIterationAvoidable" ) )
        {
            if ( rmmode != rmemNormal )
            {
                wprintf( L"/CorrelatedTouchIteration and /CorrelatedTouchIterationAvoidable must be specified for emulation mode and not mixed with other modes.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            if ( 0 == _wcsicmp( argv[iarg], L"/CorrelatedTouchIteration" ) || 0 == _wcsicmp( argv[iarg], L"-CorrelatedTouchIteration" ) )
            {
                rmmode = rmemCorrelatedTouchIteration;
            }
            else
            {
                rmmode = rmemCorrelatedTouchIterationAvoidable;
            }

            if ( ( iarg + 4 ) < argc )
            {
                cbfCacheSize = (DWORD)_wtoi( argv[++iarg] );
                dwIterationMin = (DWORD)( _wtof( argv[++iarg] ) * 1000.0 );
                dwIterationMax = (DWORD)( _wtof( argv[++iarg] ) * 1000.0 );
                cIteration = (DWORD)_wtoi( argv[++iarg] );

                if ( ( dwIterationMax < 1 ) || ( cIteration < 2 ) || ( dwIterationMin > dwIterationMax ) )
                {
                    wprintf( L"Invalid correlated-touch iteration options.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for this option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/ChkptDepth" ) || 0 == _wcsicmp( argv[iarg], L"-ChkptDepth" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                clogsChkptDepth = (USHORT)_wtoi( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /ChkptDepth option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }


        else if ( 0 == _wcsicmp( argv[iarg], L"/ChkptDepthIteration" ) || 0 == _wcsicmp( argv[iarg], L"-ChkptDepthIteration" ) )
        {
            if ( rmmode != rmemNormal )
            {
                wprintf( L"/ChkptDepthIteration must be specified for emulation mode and not mixed with other modes.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }

            rmmode = rmemChkptDepthIteration;

            if ( ( iarg + 4 ) < argc )
            {
                cbfCacheSize = (DWORD)_wtoi( argv[++iarg] );
                dwIterationMin = (DWORD)_wtoi( argv[++iarg] );
                dwIterationMax = (DWORD)_wtoi( argv[++iarg] );
                cIteration = (DWORD)_wtoi( argv[++iarg] );

                if ( ( dwIterationMax < 1 ) || ( cIteration < 2 ) || ( dwIterationMin > dwIterationMax ) )
                {
                    wprintf( L"Invalid checkpoint-depth iteration options.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for this option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CacheCountRes" ) || 0 == _wcsicmp( argv[iarg], L"-CacheCountRes" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                cCachedHistoRes = (USHORT)_wtoi( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /CacheCountRes option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/PrintSamples" ) || 0 == _wcsicmp( argv[iarg], L"-PrintSamples" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                dtickPrintSampleInterval = (TICK)( _wtof( argv[iarg] ) * 1000.0 );

                if ( dtickPrintSampleInterval == 0 )
                {
                    wprintf( L"Invalid sample interval option.\n" );
                    Error( ErrERRCheck( JET_errInvalidParameter ) );
                }
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /PrintSamples.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CacheLifetimeRes" ) || 0 == _wcsicmp( argv[iarg], L"-CacheLifetimeRes" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                dtickLifetimeHistoRes = _wtoi( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /CacheLifetimeRes option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/NoHisto" ) || 0 == _wcsicmp( argv[iarg], L"-NoHisto" ) )
        {
            fPrintHistograms = false;
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/Ifmp" ) || 0 == _wcsicmp( argv[iarg], L"-Ifmp" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                ifmpFilter = (size_t)_wtoi( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /Ifmp option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/K" ) || 0 == _wcsicmp( argv[iarg], L"-K" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                bfsysresmgrinit.K = _wtoi( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /K option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/CorrelatedTouch" ) || 0 == _wcsicmp( argv[iarg], L"-CorrelatedTouch" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                bfsysresmgrinit.csecCorrelatedTouch = _wtof( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /CorrelatedTouch option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else if ( 0 == _wcsicmp( argv[iarg], L"/Timeout" ) || 0 == _wcsicmp( argv[iarg], L"-Timeout" ) )
        {
            iarg++;
            if ( iarg < argc )
            {
                bfsysresmgrinit.csecTimeout = _wtof( argv[iarg] );
            }
            else
            {
                wprintf( L"Insufficient number of arguments for /Timeout option.\n" );
                Error( ErrERRCheck( JET_errInvalidParameter ) );
            }
        }
        else
        {
            wprintf( L"Invalid argument: %ws.\n", argv[iarg] );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }
    }

    //
    //  At least one operation mode must have been selected.
    //
    if ( !fCollectFtlStats &&
        !fCollectBfStats &&
        !fRunResMgrEmulation &&
        !fDumpTraces &&
        !fPostProcess &&
        !fConvertFromEtl &&
        !fCmpAgainstEtl )
    {
        PrintHelp( argv[0] );
        wprintf( L"At least one operation mode must have been selected.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //
    //  These options are valid for FTL only.
    //
    if ( ( fCollectFtlStats || fRunResMgrEmulation || fDumpTraces || fPostProcess ) &&
        ( wszFtlTraceLogFiles == NULL ) )
    {
        PrintHelp( argv[0] );
        wprintf( L"Some options are valid FTL files only.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //
    //  Collecting FTL stats changes the state of the reader. Collecting stats and running
    //  simulation are not supported at this point.
    //
    if ( fCollectFtlStats && fRunResMgrEmulation  )
    {
        wprintf( L"Cannot collect FTL stats and run simulation in the same command.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //
    //  These options require either FTL or ETL, but not both.
    //
    if ( fCollectBfStats &&
        ( ( ( wszFtlTraceLogFiles == NULL ) && ( wszEtlTraceLogFile == NULL ) ) ||
        ( ( wszFtlTraceLogFiles != NULL ) && ( wszEtlTraceLogFile != NULL ) ) ) )
    {
        PrintHelp( argv[0] );
        wprintf( L"Some options are valid for FTL or ETL files only, not both.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //
    //  These options require both FTL and ETL.
    //
    if ( ( fConvertFromEtl || fCmpAgainstEtl ) &&
        ( ( wszFtlTraceLogFiles == NULL ) || ( wszEtlTraceLogFile == NULL ) ) )
    {
        PrintHelp( argv[0] );
        wprintf( L"Some options are valid for an FTL and an ETL file.\n" );
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    //
    //  Convert from ETL trace.
    //
    if ( fConvertFromEtl )
    {
        wprintf( L"Converting ETL into FTL...\n" );
        Call( ErrBFETLConvertToFTL( wszEtlTraceLogFile, wszFtlTraceLogFiles, *pids ) );
    }

    //
    //  Compare against ETL trace.
    //
    if ( fCmpAgainstEtl )
    {
        wprintf( L"Comparing ETL against FTL...\n" );
        AssertSz( pids->size() == 1, "There must be only one PID to compare." );
        Call( ErrBFETLFTLCmp( wszEtlTraceLogFile, wszFtlTraceLogFiles, *( pids->begin() ) ) );
    }

    //
    //  Process FTL.
    //
    if ( wszFtlTraceLogFiles != NULL )
    {
        //
        //  Don't bother going further if not needed.
        //
        if ( !( fRunResMgrEmulation || fCollectFtlStats || fCollectBfStats || fDumpTraces || fPostProcess ) )
        {
            goto HandleError;
        }

        //
        //  Initialize the BF/ResMgr driver.
        //
        const DWORD grbitFTLTracesFilter = ( fRunResMgrEmulation ?
                                               fBFFTLDriverResMgrTraces :
                                               ( fBFFTLDriverResMgrTraces | fBFFTLDriverDirtyTraces ) );
        const DWORD grbitFTL = ( fCollectFtlStats ? fBFFTLDriverCollectFTLStats : 0 ) |
                               ( ( fCollectBfStats || fPostProcess ) ? fBFFTLDriverCollectBFStats : 0 ) | 
                               grbitFTLTracesFilter;
        Call( ErrBFFTLInit( wszFtlTraceLogFiles, grbitFTL, &pbfftlc ) );

        if ( fRunResMgrEmulation && pbfftlc->cIFMP == 0 )
        {
            wprintf( L"Error: File has not been post processed, and res mgr emulation requires this!\n" );
            wprintf( L"Run: ResMgrReplay /Ftl %ws /PostProcess!\n", wszFtlTraceLogFiles );
            Error( ErrERRCheck( JET_errInvalidParameter ) );
        }

        //
        //  Process BF FTL file: pre-processing or stats.
        //
        if ( fCollectFtlStats || fCollectBfStats || fDumpTraces || fPostProcess )
        {
            Call( ErrResMgrAccumFtlStats( pbfftlc, fDumpTraces ) );
        }

        //
        //  Dump any stats
        //
        Call( ErrBFFTLDumpStats( pbfftlc, 
                        ( fCollectFtlStats ? fBFFTLDriverCollectFTLStats : 0 ) |
                        ( fCollectBfStats ? fBFFTLDriverCollectBFStats : 0 ) ) );

        if ( fPostProcess )
        {
            Call( ErrBFFTLPostProcess( pbfftlc ) );
        }

        //
        //  Process BF FTL file: run the resmgr emulator.
        //
        if ( fRunResMgrEmulation )
        {
            switch ( rmralg )
            {
                case rmralgLruTest:
                    pipea = new PageEvictionAlgorithmLRUTest();
                    break;

                case rmralgLrukTest:
                    pipea = new PageEvictionAlgorithmLRUKTest();
                    break;

                case rmralgLrukEse:
                    pipea = new PageEvictionAlgorithmLRUKESE();
                    break;

                case rmralgBeladys:
                    pipea = new PageEvictionAlgorithmBeladys( true );
                    break;

                case rmralgBeladysInverse:
                    pipea = new PageEvictionAlgorithmBeladys( false );
                    break;

                default:
                    AssertSz( fFalse, "We should not get here with an unknown algorithm." );
                    break;
            }

            Alloc( pipea );

            //
            //  May need to pre-process.
            //
            if ( pipea->FNeedsPreProcessing() )
            {
                wprintf( L"Pre-processing traces...\n" );
                BFFTLTerm( pbfftlc );
                pbfftlc = NULL;
                Call( ErrBFFTLInit( wszFtlTraceLogFiles, grbitFTLTracesFilter, &pbfftlc ) );

                Call( emulator.ErrSetReplaySuperCold( fReplaySuperCold ) );
                Call( emulator.ErrSetReplayNoTouch( fReplayNoTouch ) );
                Call( emulator.ErrSetReplayDbScan( fReplayDbScan ) );
                Call( emulator.ErrSetReplayCachePriority( fReplayCachePriority ) );
                emulator.SetPrintHistograms( false );

                if ( rmmode == rmemNormal )
                {
                    Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspVariable ) );
                }
                else if ( ( rmmode == rmemCacheSizeFixed ) ||
                          ( rmmode == rmemCorrelatedTouchIteration ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) ||
                          ( rmmode == rmemChkptDepthIteration ) )
                {
                    Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, cbfCacheSize ) );
                }
                else if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheSizeIterationAvoidable ) ||
                          ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) )
                {
                    Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, dwIterationMax ) );
                }
                else if ( rmmode == rmemCacheSizeFixedIteration )
                {
                    Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, *( cacheSizes->end()-- ) ) );
                }
                else if ( rmmode == rmemCacheSizeAgeBased )
                {
                    Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspAgeBased, dtickCacheAge ) );
                }

                Call( emulator.ErrInit( pbfftlc, pipea ) );
                Call( emulator.ErrExecute() );
                emulator.Term();
                BFFTLTerm( pbfftlc );
                pbfftlc = NULL;

                wprintf( L"Pre-processing finished. Start processing...\n" );
                Call( pipea->ErrStartProcessing() );
                Call( ErrBFFTLInit( wszFtlTraceLogFiles, grbitFTL, &pbfftlc ) );
            }

            //
            //  For normal, fixed-cache-size and age-based-cache-size modes, we'll leverage the previous BF driver init and perhaps
            //  even proceed with dumping stats.
            //
            if ( ( rmmode == rmemNormal ) || ( rmmode == rmemCacheSizeFixed ) || ( rmmode == rmemCacheSizeAgeBased ) )
            {
                Call( emulator.ErrSetReplaySuperCold( fReplaySuperCold ) );
                Call( emulator.ErrSetReplayNoTouch( fReplayNoTouch ) );
                Call( emulator.ErrSetReplayDbScan( fReplayDbScan ) );
                Call( emulator.ErrSetReplayCachePriority( fReplayCachePriority ) );
                Call( emulator.ErrSetEvictNextOnShrink( fEvictNextOnShrink ) );
                Call( emulator.ErrSetReplayInitTerm( fReplayInitTerm ) );

                const PageEvictionEmulator::PageEvictionEmulatorCacheSizePolicy peecsp =
                    ( rmmode == rmemNormal ) ?
                        PageEvictionEmulator::peecspVariable :
                        ( ( rmmode == rmemCacheSizeFixed ) ? PageEvictionEmulator::peecspFixed : PageEvictionEmulator::peecspAgeBased );
                const DWORD dwParam =
                    ( rmmode == rmemNormal ) ?
                        0 :
                        ( ( rmmode == rmemCacheSizeFixed ) ? cbfCacheSize : dtickCacheAge );
                Call( emulator.ErrSetCacheSize( peecsp, dwParam ) );

                if ( clogsChkptDepth != ulMax )
                {
                    Call( emulator.ErrSetCheckpointDepth( clogsChkptDepth ) );
                }
                Call( emulator.ErrSetInitOverrides( &bfsysresmgrinit ) );
                Call( emulator.ErrSetPrintSampleInterval( dtickPrintSampleInterval ) );
                emulator.SetPrintHistograms( fPrintHistograms );
                Call( emulator.ErrSetFaultsHistoRes( cCachedHistoRes ) );
                Call( emulator.ErrSetLifetimeHistoRes( dtickLifetimeHistoRes ) );
                
                Call( emulator.ErrInit( pbfftlc, pipea ) );

                wprintf( L"Running simulation...\n" );

                Call( emulator.ErrExecute() );

                Call( emulator.ErrDumpStats() );
            }
            else if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheSizeFixedIteration ) || ( rmmode == rmemCacheSizeIterationAvoidable ) ||
                    ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) ||
                    ( rmmode == rmemCorrelatedTouchIteration ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) ||
                    ( rmmode == rmemChkptDepthIteration ) )
            {
                QWORD cRequested = 0, cRequestedUnique = 0;
                
                //
                //  Will get re-initialized at each iteration.
                //
                BFFTLTerm( pbfftlc );
                pbfftlc = NULL;

                if ( rmmode == rmemCacheSizeFixedIteration )
                {
                    Alloc( psimulationIterator = new FixedCacheSimulationPresetIterator( *cacheSizes ) );
                }
                else if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheSizeIterationAvoidable ) )
                {
                    Alloc( psimulationIterator = new FixedCacheSimulationIterator );
                }
                else if ( ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) )
                {
                    Alloc( psimulationIterator = new CacheFaultLookupSimulationIterator( cFaultsTarget ) );
                }
                else if ( ( rmmode == rmemCorrelatedTouchIteration ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) )
                {
                    Alloc( psimulationIterator = new CacheFaultMinSimulationIterator( cIteration, dwIterationMax ) );
                }
                else if ( rmmode == rmemChkptDepthIteration )
                {
                    Alloc( psimulationIterator = new ChkptDepthSimulationIterator );
                }
                else
                {
                    AssertSz( fFalse, "Invalid emulation mode." );
                }

                Alloc( psimulationFaultSamples = new FixedCacheSimulationIterator );
                Alloc( psimulationFaultAvoidableSamples = new FixedCacheSimulationIterator );
                Alloc( psimulationWriteSamples = new ChkptDepthSimulationIterator );
                Alloc( psimulationWriteChkptSamples = new ChkptDepthSimulationIterator );
                Alloc( psimulationWriteScavengeSamples = new ChkptDepthSimulationIterator );
                Alloc( psimulationCacheSizeMaxSamples = new FixedCacheSimulationIterator );

                DWORD dwIteration = 0;
                for ( DWORD i = 0; ( ( cIteration == 0 ) || ( i < cIteration ) ); i++ )
                {
                    //
                    //  Decide what the next iteration will be.
                    //
                    dwIteration = 0;

                    if ( rmmode == rmemCacheSizeFixedIteration )
                    {
                        dwIteration = psimulationIterator->DwGetNextIterationValue();

                        if ( dwIteration == ulMax )
                        {
                            break;
                        }
                    }
                    else if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheSizeIterationAvoidable ) ||
                            ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) ||
                            ( rmmode == rmemChkptDepthIteration ) )
                    {
                        if ( i == 0 )
                        {
                            dwIteration = dwIterationMin;
                        }
                        else if ( i == 1 )
                        {
                            dwIteration = dwIterationMax;
                        }
                        else
                        {
                            dwIteration = psimulationIterator->DwGetNextIterationValue();
                        }

                        if ( ( dwIteration == ulMax ) || psimulationIterator->FSimulationSampleExists( dwIteration ) )
                        {
                            break;
                        }
                    }
                    else if ( ( rmmode == rmemCorrelatedTouchIteration ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) )
                    {
                        if ( i == 0 )
                        {
                            dwIteration = dwIterationMin;
                        }
                        else if ( ( i == 1 ) && ( dwIterationMin == 0 ) )
                        {
                            dwIteration = dwIterationMin + 16;
                        }
                        else
                        {
                            dwIteration = psimulationIterator->DwGetNextIterationValue();
                        }

                        if ( ( ( dwIteration == ulMax ) && ( i != 0 ) ) || psimulationIterator->FSimulationSampleExists( dwIteration ) )
                        {
                            break;
                        }
                    }
                    else
                    {
                        AssertSz( fFalse, "Invalid emulation mode." );
                    }
                    
                    Call( ErrBFFTLInit( wszFtlTraceLogFiles, grbitFTLTracesFilter, &pbfftlc ) );

                    Call( emulator.ErrSetReplaySuperCold( fReplaySuperCold ) );
                    Call( emulator.ErrSetReplayNoTouch( fReplayNoTouch ) );
                    Call( emulator.ErrSetReplayDbScan( fReplayDbScan ) );
                    Call( emulator.ErrSetReplayCachePriority( fReplayCachePriority ) );
                    Call( emulator.ErrSetEvictNextOnShrink( fEvictNextOnShrink ) );
                    Call( emulator.ErrSetReplayInitTerm( fReplayInitTerm ) );
                    Call( emulator.ErrSetInitOverrides( &bfsysresmgrinit ) );
                    Call( emulator.ErrSetPrintSampleInterval( dtickPrintSampleInterval ) );
                    emulator.SetPrintHistograms( fPrintHistograms );
                    Call( emulator.ErrSetFaultsHistoRes( cCachedHistoRes ) );
                    Call( emulator.ErrSetLifetimeHistoRes( dtickLifetimeHistoRes ) );

                    if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheSizeFixedIteration ) || ( rmmode == rmemCacheSizeIterationAvoidable ) ||
                            ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) )
                    {
                        wprintf( L"Running simulation with a cache size of %u buffers (iteration # %u)...\n", dwIteration, i + 1 );
                        if ( clogsChkptDepth != ulMax )
                        {
                            Call( emulator.ErrSetCheckpointDepth( clogsChkptDepth ) );
                        }

                        Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, dwIteration ) );
                    }
                    else if ( ( rmmode == rmemCorrelatedTouchIteration ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) )
                    {
                        wprintf( L"Running simulation with a correlated-touch interval of %u msec (iteration # %u)...\n", dwIteration, i + 1 );
                        if ( clogsChkptDepth != ulMax )
                        {
                            Call( emulator.ErrSetCheckpointDepth( clogsChkptDepth ) );
                        }
                        Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, cbfCacheSize ) );
                        bfsysresmgrinit.csecCorrelatedTouch = ( (double)dwIteration / 1000.0 );
                        Call( emulator.ErrSetInitOverrides( &bfsysresmgrinit ) );
                    }
                    else if ( rmmode == rmemChkptDepthIteration )
                    {
                        wprintf( L"Running simulation with a checkpoint depth of %u log files (iteration # %u)...\n", dwIteration, i + 1 );
                        Call( emulator.ErrSetCheckpointDepth( dwIteration ) );
                        Call( emulator.ErrSetCacheSize( PageEvictionEmulator::peecspFixed, cbfCacheSize ) );
                    }
                    else
                    {
                        AssertSz( fFalse, "Invalid emulation mode." );
                    }

                    Call( emulator.ErrInit( pbfftlc, pipea ) );

                    Call( emulator.ErrExecute() );

                    Call( emulator.ErrDumpStats( false ) );

                    const PageEvictionEmulator::STATS& stats = ( ifmpFilter != upMax ) ? emulator.GetStats( ifmpFilter ) : emulator.GetStats();

                    if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCorrelatedTouchIteration ) )
                    {
                        psimulationIterator->AddSimulationSample( dwIteration, stats.cFaultsSim );
                    }
                    else if ( ( rmmode == rmemCacheSizeIterationAvoidable ) || ( rmmode == rmemCacheFaultIterationAvoidable ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) )
                    {
                        psimulationIterator->AddSimulationSample( dwIteration, stats.cFaultsSimAvoidable );
                    }
                    else if ( rmmode == rmemChkptDepthIteration )
                    {
                        psimulationIterator->AddSimulationSample( dwIteration, stats.cWritesSim );
                    }
                    else
                    {
                        AssertSz( fFalse, "Invalid emulation mode." );
                    }

                    psimulationFaultSamples->AddSimulationSample( dwIteration, stats.cFaultsSim );
                    psimulationFaultAvoidableSamples->AddSimulationSample( dwIteration, stats.cFaultsSimAvoidable );
                    psimulationWriteSamples->AddSimulationSample( dwIteration, stats.cWritesSim );
                    psimulationWriteChkptSamples->AddSimulationSample( dwIteration, stats.cWritesSimChkpt );
                    psimulationWriteScavengeSamples->AddSimulationSample( dwIteration, stats.cWritesSimAvailPool + ( fEvictNextOnShrink ? stats.cWritesSimShrink : 0 ) );
                    psimulationCacheSizeMaxSamples->AddSimulationSample( dwIteration, (QWORD)stats.cpgCachedMax );

                    if ( i == 0 )
                    {
                        cRequested = stats.cRequested;
                        cRequestedUnique = stats.cRequestedUnique;
                    }
                    else
                    {
                        Assert( cRequested == stats.cRequested );
                        Assert( cRequestedUnique == stats.cRequestedUnique );
                    }

                    emulator.Term();

                    BFFTLTerm( pbfftlc );
                    pbfftlc = NULL;

                    wprintf( L"\n\n" );

                    if ( pipea->FNeedsPreProcessing() )
                    {
                        Call( pipea->ErrResetProcessing() );
                    }
                }

                //
                //  Print a summary with all iterations.
                //
                if ( ( rmmode == rmemCacheSizeIteration ) || ( rmmode == rmemCacheSizeFixedIteration ) || ( rmmode == rmemCacheSizeIterationAvoidable ) ||
                        ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) ||
                        ( rmmode == rmemChkptDepthIteration ) )
                {
                    DWORD dwIterationValue = 0;
                    QWORD cFaults = 0, cpgCachedMax = 0, cFaultsAvoidable = 0, cWrites = 0, cWritesChkpt = 0, cWritesScavenge = 0;
                    
                    const WCHAR* const wszIterationVariableName = ( rmmode == rmemChkptDepthIteration ) ? L"clogsChkptDepth" : L"cbfCacheSize";

                    wprintf( L"\n" );
                    wprintf( L"%ws,cFaults,cFaultsAvoidable,cWrites,cWritesChkpt,cWritesScavenge,cFaults+cWrites,cpgCachedMax\n", wszIterationVariableName );
                    for ( size_t i = 0; psimulationFaultSamples->FGetSimulationSample( i, &dwIterationValue, &cFaults ); i++ )
                    {
                        psimulationFaultAvoidableSamples->FGetSimulationSample( i, &dwIterationValue, &cFaultsAvoidable );
                        psimulationWriteSamples->FGetSimulationSample( i, &dwIterationValue, &cWrites );
                        psimulationWriteChkptSamples->FGetSimulationSample( i, &dwIterationValue, &cWritesChkpt );
                        psimulationWriteScavengeSamples->FGetSimulationSample( i, &dwIterationValue, &cWritesScavenge );
                        psimulationCacheSizeMaxSamples->FGetSimulationSample( i, &dwIterationValue, &cpgCachedMax );

                        wprintf( L"%u,%I64u,%I64u,%I64u,%I64u,%I64u,%I64u,%I64u\n", dwIterationValue, cFaults, cFaultsAvoidable, cWrites, cWritesChkpt, cWritesScavenge, cFaults + cWrites, cpgCachedMax );
                    }

                    //
                    //  Check the best match if we're running fault lookup.
                    //
                    if ( ( rmmode == rmemCacheFaultIteration ) || ( rmmode == rmemCacheFaultIterationAvoidable ) )
                    {
                        wprintf( L"\n" );
                        if ( dwIteration == ulMax )
                        {
                            wprintf( L"cbfCacheSizeFound:NotFound\n" );
                            wprintf( L"cbfFaultsFound:NotFound\n" );
                        }
                        else
                        {
                            QWORD qwSampleValue = 0;
                            SimulationIterator* psimulationFaultSamplesT = ( rmmode == rmemCacheFaultIteration ) ? psimulationFaultSamples : psimulationFaultAvoidableSamples;

                            for ( size_t i = 0; psimulationFaultSamplesT->FGetSimulationSample( i, &dwIterationValue, &qwSampleValue ); i++ )
                            {
                                if ( dwIterationValue == dwIteration )
                                {
                                    wprintf( L"cbfCacheSizeFound:%u\n", dwIterationValue );
                                    wprintf( L"cbfFaultsFound:%I64u\n", qwSampleValue );
                                }
                            }
                        }
                    }
                }
                else if ( ( rmmode == rmemCorrelatedTouchIteration ) || ( rmmode == rmemCorrelatedTouchIterationAvoidable ) )
                {
                    DWORD dwIterationValue = 0;
                    QWORD cFaults = 0, cpgCachedMax = 0, cFaultsAvoidable = 0, cWrites = 0, cWritesChkpt = 0, cWritesScavenge = 0;
                    
                    wprintf( L"\n" );
                    wprintf( L"csecCorrelatedTouch,cFaults,cFaultsAvoidable,cWrites,cWritesChkpt,cWritesScavenge,cFaults+cWrites,cpgCachedMax\n" );
                    for ( size_t i = 0; psimulationFaultSamples->FGetSimulationSample( i, &dwIterationValue, &cFaults ); i++ )
                    {
                        psimulationFaultAvoidableSamples->FGetSimulationSample( i, &dwIterationValue, &cFaultsAvoidable );
                        psimulationWriteSamples->FGetSimulationSample( i, &dwIterationValue, &cWrites );
                        psimulationWriteChkptSamples->FGetSimulationSample( i, &dwIterationValue, &cWritesChkpt );
                        psimulationWriteScavengeSamples->FGetSimulationSample( i, &dwIterationValue, &cWritesScavenge );
                        psimulationCacheSizeMaxSamples->FGetSimulationSample( i, &dwIterationValue, &cpgCachedMax );

                        wprintf( L"%.3f,%I64u,%I64u,%I64u,%I64u,%I64u,%I64u,%I64u\n", (double)dwIterationValue / 1000.0, cFaults, cFaultsAvoidable, cWrites, cWritesChkpt, cWritesScavenge, cFaults + cWrites, cpgCachedMax );
                    }

                    //
                    //  Check the minimum found.
                    //
                    DWORD dtickCorrelatedTouchFound = UINT_MAX;
                    QWORD cFaultsSampleFound = UINT_MAX;
                    QWORD cFaultsSample = 0;
                    
                    wprintf( L"\n" );
                    SimulationIterator* psimulationFaultSamplesT = ( rmmode == rmemCorrelatedTouchIteration ) ? psimulationFaultSamples : psimulationFaultAvoidableSamples;
                    for ( size_t i = 0; psimulationFaultSamplesT->FGetSimulationSample( i, &dwIterationValue, &cFaultsSample ); i++ )
                    {
                        if ( cFaultsSample < cFaultsSampleFound )
                        {
                            cFaultsSampleFound = cFaultsSample;
                            dtickCorrelatedTouchFound = dwIterationValue;
                        }
                    }

                    wprintf( L"csecCorrelatedTouchFound:%.3f\n", (double)dtickCorrelatedTouchFound / 1000.0 );
                    wprintf( L"cbfFaultsFound:%I64u\n", cFaultsSampleFound );
                }
                else
                {
                    AssertSz( fFalse, "Invalid emulation mode." );
                }
            }
            else
            {
                AssertSz( fFalse, "Invalid emulation mode." );
            }
        }

        goto HandleError;
    }

    //
    //  Process ETL.
    //
    if ( wszEtlTraceLogFile != NULL )
    {
        //
        //  Don't bother going further if not needed.
        //
        if ( !fCollectBfStats )
        {
            goto HandleError;
        }

        //
        //  Initialize the BF/ResMgr driver.
        //
        Call( ErrBFETLInit( wszEtlTraceLogFile, 0, 0, fCollectBfStats ? fBFETLDriverCollectBFStats : 0, &pbfetlc ) );

        if ( fCollectBfStats )
        {
            Call( ErrResMgrAccumEtlStats( pbfetlc ) );
        }

        //
        //  Dump any stats
        //
        Call( ErrBFETLDumpStats( pbfetlc, fCollectBfStats ? fBFETLDriverCollectBFStats : 0 ) );

        goto HandleError;
    }

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(Failed!).\n" );
        wprintf( L"\t\tDetails:\n" );
        wprintf( L"\t\t\terrRet   = %d\n", err );
        if ( PefLastThrow() )
        {
            wprintf( L"\t\t\terrLast  = %d\n", PefLastThrow()->Err() );
            wprintf( L"\t\t\tszFile   = %hs\n", PefLastThrow()->SzFile() );
            wprintf( L"\t\t\tulLine   = %d\n", PefLastThrow()->UlLine() );
        }
        else
        {
            wprintf( L"\t\t\tLast Throw not defined!\n" );
        }
    }
    else
    {
        wprintf( L"\tDone(success).\n" );
    }

    emulator.Term();

    delete pipea;
    pipea = NULL;

    if ( pbfftlc )
    {
        BFFTLTerm( pbfftlc );
    }
    pbfftlc = NULL;

    if ( pbfetlc )
    {
        BFETLTerm( pbfetlc );
    }
    pbfetlc = NULL;

    delete psimulationCacheSizeMaxSamples;
    psimulationCacheSizeMaxSamples = NULL;

    delete psimulationWriteScavengeSamples;
    psimulationWriteScavengeSamples = NULL;

    delete psimulationWriteChkptSamples;
    psimulationWriteChkptSamples = NULL;

    delete psimulationWriteSamples;
    psimulationWriteSamples = NULL;

    delete psimulationFaultAvoidableSamples;
    psimulationFaultAvoidableSamples = NULL;

    delete psimulationFaultSamples;
    psimulationFaultSamples = NULL;

    delete psimulationIterator;
    psimulationIterator = NULL;

    delete cacheSizes;
    cacheSizes = NULL;

    delete pids;
    pids = NULL;

    OSTerm();

    return err;
}

