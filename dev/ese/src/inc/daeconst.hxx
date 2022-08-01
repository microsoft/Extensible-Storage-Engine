// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

const PGNO  pgnoSystemRoot              = 1;
const OBJID objidSystemRoot             = 1;

const WCHAR wszOn[]                     = L"on";

const WCHAR wszTempDir[]                = L"temp" wszPathDelimiter;
const WCHAR wszPatExt[]                 = L".pat";          //  patch file

// UNDONE: Better would be to move these to like private part of log.hxx (or maybe
// even log.cxx), to ensure isolation...
const WCHAR wszOldLogExt[]              = L".log";          //  log file extension, deprecated
const WCHAR wszNewLogExt[]              = L".jtx";          //  log file extension, preferred
const WCHAR wszResLogExt[]              = L".jrs";          //  reserve log extension
const WCHAR wszOldChkExt[]              = L".chk";          //  checkpoint file extension, deprecated
const WCHAR wszNewChkExt[]              = L".jcp";          //  checkpoint file extension, preferred
const WCHAR wszSecLogExt[]              = L".jsl";          //  shadow|secondary log file extension
const WCHAR wszShrinkArchiveExt[]       = L".jsa";          //  shrink archive file extension
const WCHAR wszRBSExt[]                 = L".rbs";          //  revert snapshot file extension
const WCHAR wszRBSFDPDeleteStateExt[]   = L".jdf";          //  file contain extension for file containing PageFDPDelete state.
const WCHAR wszRBSDirRoot[]             = L"Snapshot";      //  revert snapshot static root directory name
const WCHAR wszRBSBackupDir[]           = L"Backup";        //  revert snapshot static backup directory name where snapshots are backed up, once applied, for future investigations if needed.
const WCHAR wszRBSDirBase[]             = L"RBS";           //  revert snapshot directory static base name
const WCHAR wszRBSLogDir[]             = L"Logs";           //  revert snapshot static log directory base name
const ULONG cMinReserveLogs             = 2;
const ULONG cMinReserveLogsWithAggresiveRollover    = 10;

const WCHAR wszAtomicNew[]              = L"new";
const WCHAR wszAtomicOld[]              = L"old";
const WCHAR wszLogTmp[]                 = L"tmp";
const WCHAR wszLogRes[]                 = L"res";
const WCHAR wszRestoreInstanceName[]        = L"Restore";
const WCHAR wszRestoreInstanceNamePrefix[]      = L" - Restore";

// Interesting side effect of 0x7FFFFFFF is that we get several integral constant overflows as people
// try to add 1 or 2 to this to do an assert() or something and the compiler detects and the overflow
// in a LONG.  We set it a little lower (~2047 lower than 2G, recovery undo can have 512 extra).
const LONG  lGenerationMaxDuringRecovery    = 0x7FFFFA00;
const LONG  lGenerationMax                  = lGenerationMaxDuringRecovery - 0x200; //  save some logs for recovery-undo
const LONG  lGenerationMaxWarningThreshold  = 0x40000000;   //  the point at which we start periodically generating warning eventlog messages that we are approaching the max
const LONG  lGenerationMaxWarningFrequency  = 20000;    //  frequency with which a warning eventlog is generated when we are over the log sequence warning threshold
const LONG  lGenerationMaxPanicThreshold    = 0x7FFFF000;   //  the point at which we start aggressively generation warning eventlog messages that we are dangerously close to the max
const LONG  lGenerationMaxPanicFrequency    = 10;       //  frequency with which a warning eventlog is generated when we are dangerously close to the max
const LONG  lGenerationInvalid              = 0x7FFFFFFF;

const LONG  lGenerationEXXDuringDump        = lGenerationMaxDuringRecovery + 1;
const LONG  lGenerationInvalidDuringDump    = lGenerationEXXDuringDump + 1;
// LEGACY ...
//const LONG    lGenerationMaxDuringRecovery    = 0xFFFFF;
//const LONG    lGenerationMax                  = lGenerationMaxDuringRecovery - 16; // same some logs for recovery-undo
//const LONG    lGenerationMaxWarningThreshold  = 0xE0000;  //  the point at which we start periodically generating warning eventlog messages that we are approaching the max
//const LONG    lGenerationMaxWarningFrequency  = 1000;     //  frequency with which a warning eventlog is generated when we are over the log sequence warning threshold
//const LONG    lGenerationMaxPanicThreshold    = 0xFF000;  //  the point at which we start aggressively generation warning eventlog messages that we are dangerously close to the max
//const LONG    lGenerationMaxPanicFrequency    = 10;       //  frequency with which a warning eventlog is generated when we are dangerously close to the max

//  these numbers are chosen so that even in the worst case of a log file
//  with 0xFFFF sectors each of 0xFFFF bytes and in the worst case of a
//  64 proc machine, we will still have enough bits in the OB0 index to
//  hold the given checkpoint depth in log generations.  you can push these
//  limits around but you must conserve the total number of significant bits
//  between Max and Uncertainty.  Min is chosen to allow enough room for
//  a graceful shutdown in the event of checkpoint too deep
//
const LONG  lgenCheckpointTooDeepMin            = 0x20;     //  we must allow at least this many generations between the checkpoint and the current generation
const LONG  lgenCheckpointTooDeepMax            = 0x10000;  //  max outstanding generations allowed between checkpoint and current generation
const LONG  cbCheckpointTooDeepUncertainty      = 0x10000;      //  uncertainty in the LGPOS required to match lgenCheckpointTooDeepMax

/* the number of pages in the Long Value tree of each table
/**/
const INT   cpgLVTree                   = 1;

//  preread constants
const CPG   cpgPrereadSequential        = 128;  //  number of pages to preread in a table opened sequentially
const CPG   cpgPrereadPredictive        = 16;   //  number of pages to preread if we guess we are prereading
const CPG   cpgPrereadRangesMax         = 128;  //  number of pages to preread in an index range

const LONG  cbSequentialDataPrereadThreshold    = 64 * 1024;

/*  default density
/**/
const ULONG ulFILEDefaultDensity        = 80;       // 80% density
const ULONG ulFILEDensityLeast          = 20;       // 20% density
const ULONG ulFILEDensityMost           = 100;      // 100% density


const DWORD dwCounterMax                = 0x7fffff00;
const QWORD qwCounterMax                = 0x7fffffffffffff00;

const OBJID objidNil                    = 0x00000000;
const OBJID objidFDPMax                 = 0xFFFFFF00;
const OBJID objidFDPOverMax             = objidFDPMax + 1;
const OBJID objidMaxWarningThreshold    = 0x7F000000;   //  the point at which we start periodically generating warning eventlog messages that we are approaching the max
const ULONG ulObjidMaxWarningFrequency  = 100000;       //  frequency with which a warning eventlog is generated when we are over the ObjidFDP warning threshold
const OBJID objidMaxPanicThreshold      = 0xFF000000;   //  the point at which we start aggressively generation warning eventlog messages that we are dangerously close to the max
const ULONG ulObjidMaxPanicFrequency    = 1000;         //  frequency with which a warning eventlog is generated when we are dangerously close to the max
static_assert( objidNil == 0, "There is code with implicit assumptions about either objidNil being zero, or being less than any valid OBJID." );

const CHAR szNull[]                     = "";

/*  transaction level limits
/**/
const LEVEL levelMax                    = 11;       // all level < 11
const LEVEL levelUserMost               = 7;        // max for user
const LEVEL levelMin                    = 0;

/* Start and max waiting period for WaitTillOldest
/**/
const ULONG ulStartTimeOutPeriod        = 20;
const ULONG ulMaxTimeOutPeriod          = 6000; /*  6 seconds */

/* Version store bucket size (used to be in ver.hxx)
/**/
const INT cbBucketLegacy                = 16384;

/*  default resource / parameter settings
/**/
const LONG cpageDbExtensionDefault      = 256;
const LONG cpageSEDefault               = 16;
const LONG cpibDefault                  = 16;
const LONG cfucbDefault                 = 1024;
const LONG cfcbDefault                  = 300;
const LONG cfcbCachedClosedTablesDefault= 64;
const LONG cscbDefault                  = 20;
const LONG csecLogBufferDefault         = 126;
const LONG csecLogFileSizeDefault       = 5120;
const LONG cbucketDefault               = 1 + ( 1024 * 1024 - 1 ) / cbBucketLegacy; // 1MB of version store
const LONG cpageTempDBMinDefault        = 0;
const LONG lPageFragmentDefault         = 8;
const LONG lCacheSizeMinDefault         = 1;
const LONG lCacheSizeDefault            = 512;
const LONG lCheckpointDepthMaxDefault   = 20 * 1024 * 1024;
const LONG lLRUKCorrIntervalDefault     = 128000;
const LONG lLRUKHistoryMaxDefault       = 1024;
const LONG lLRUKPolicyDefault           = 2;
const LONG lLRUKTimeoutDefault          = 100;

const LONG g_pctCachePriorityMin        = 0;
const LONG g_pctCachePriorityMax        = 1000;
const LONG g_pctCachePriorityMaxMax     = (LONG)wMax;
const LONG g_pctCachePriorityDefault    = 100;
#define FIsCachePriorityValid( pctCachePriority )   ( ( (LONG)(pctCachePriority) >= g_pctCachePriorityMin ) && ( (LONG)(pctCachePriority) <= g_pctCachePriorityMax ) )
static_assert( g_pctCachePriorityMin >= 0, "g_pctCachePriorityMin must be >= 0." );
static_assert( g_pctCachePriorityMin <= g_pctCachePriorityMax, "g_pctCachePriorityMin must be <= g_pctCachePriorityMax." );
static_assert( FIsCachePriorityValid( g_pctCachePriorityDefault ), "g_pctCachePriorityDefault must be valid." );
static_assert( g_pctCachePriorityMax < g_pctCachePriorityMaxMax, "g_pctCachePriorityMax must be < g_pctCachePriorityMaxMax." );
static_assert( g_pctCachePriorityMaxMax <= (LONG)wMax, "g_pctCachePriorityMaxMax must be <= wMax (0xFFFF) because that's the size allocated to it in the ResMgr IC and ESE session." );
static_assert( !FIsCachePriorityValid( g_pctCachePriorityMaxMax ), "g_pctCachePriorityMaxMax must not be a valid priority value." );

//  g_pctCachePriorityNeutral is supposed to be used when a cache priority value is required at a specific
//  location in the code but the effect of touching/caching the resource with a given priority is not relevant.
//  When caching a resource, using g_pctCachePriorityMin means the it will be assigned the minimum priority possible
//  and will be evicted soon.
//  If the resource is already cached, it'll keep the current priority, since the ResMgr never downgrades priorities.
const LONG g_pctCachePriorityNeutral    = g_pctCachePriorityMin;
static_assert( g_pctCachePriorityNeutral == g_pctCachePriorityMin, "If you are changing g_pctCachePriorityNeutral, consider the ramifications and assumptions around it being == g_pctCachePriorityMin." );
static_assert( FIsCachePriorityValid( g_pctCachePriorityNeutral ), "g_pctCachePriorityNeutral must be valid." );

//  g_pctCachePriorityUnassigned is supposed to be used when a cache priority value is required at a specific
//  location in the code but the effect of touching/caching the resource with a given priority is not relevant.
const LONG g_pctCachePriorityUnassigned = wMax;
static_assert( g_pctCachePriorityUnassigned > g_pctCachePriorityMax, "g_pctCachePriorityUnassigned must be > g_pctCachePriorityMax." );
static_assert( g_pctCachePriorityUnassigned <= g_pctCachePriorityMaxMax, "g_pctCachePriorityUnassigned must be <= g_pctCachePriorityMaxMax." );
#define FIsCachePriorityAssigned( pctCachePriority )    ( (LONG)(pctCachePriority) != g_pctCachePriorityUnassigned )

const LONG lStartFlushThresholdPercentDefault   = 1;
const LONG lStopFlushThresholdPercentDefault    = 2;
const LONG lStartFlushThresholdDefault  = lCacheSizeDefault * lStartFlushThresholdPercentDefault / 100;
const LONG lStopFlushThresholdDefault   = lCacheSizeDefault * lStopFlushThresholdPercentDefault / 100;
const LONG cbEventHeapMaxDefault        = 0;
const LONG cpgBackupChunkDefault        = 16;
const LONG cBackupReadDefault           = 8;
const LONG cbPageHintCacheDefault       = 256 * 1024;

/*  system resource requirements
/**/
const LONG cpibSystemFudge              = 64;                   // OLD,TTMAPS, async. tasks all use PIBs. This should be enough
const LONG cpibSystem                   = 5 + cpibSystemFudge;  // RCEClean, LV tree creation, backup, callback, sentinel
const LONG cthreadSystem                = 5;                    // RCEClean, LGWrite, BFIO, BFClean, perf
const LONG cbucketSystem                = 2;

/*  minimum resource / parameter settings are defined below:
/**/
const LONG lLogFileSizeMin              = 32;

/*  maximum resource / parameter settings are defined below:
/**/
const LONG cpibMax                      = 32767 - cpibSystem;   //  limited by sync library

/*  wait time for latch/crit conflicts
/**/
const ULONG cmsecWaitGeneric            = 100;
const ULONG cmsecWaitWriteLatch         = 10;
const ULONG cmsecWaitLogWrite           = 2;
#ifdef RFS2
const ULONG cmsecWaitLogWriteMax        = 1000;         // 1 sec
#else  // RFS2
const ULONG cmsecWaitLogWriteMax        = 300000;       // 5 min
#endif // RFS2
const ULONG cmsecWaitIOComplete         = 10;
const ULONG cmsecAsyncBackgroundCleanup = 60000;        //  1 min
const ULONG cmsecWaitForBackup          = 300000;       //  5 min
#ifdef RTM
const ULONG csecOLDMinimumPass          = 3600;         //  1 hr
#else
const ULONG csecOLDMinimumPass          = 300;          //  5 min
#endif
const ULONG cmsecMaxReplayDelayDueToReadTrx = 11*1000;  //  11 seconds

/*  initial thread stack sizes
/**/
const ULONG cbBFCleanStack              = 16384;

const LONG lPrereadMost                 = 64;

//  the number of bytes used to store the length of a key
//  These are not directly persisted values, but they are implicitly built into the structure of NODE offsets in a 
//  way that is persisted ... changing either would definitely change the format.
PERSISTED const INT cbKeyCount                    = sizeof(USHORT);
PERSISTED const INT cbPrefixOverhead              = cbKeyCount;

//  the number of bytes used to store the number of segments in a key
const INT cbKeySegmentsCount            = sizeof(BYTE);


//  the number of instances supported
const ULONG cMaxInstances                       = 10 * 1024;

//  Reserve one slot for ifmp 0, because some clients are checking ifmp (it's presented as JET_DBID at client side)
//  against 0 for validation (although they should initialize ifmp to JET_dbidNil and check it against JET_dbidNil).
//  This reservation became necessary after "Defer Creating Temp DB" feature, because before that feature, ifmp 0
//  was almost always occupied by temp DB so 0 was rarely (only if paramMaxTemporaryTables set to 0) returned to
//  clients as user DB ifmp. After "Defer Creating Temp DB" feature, user DB might be created before temp DB is
//  created, so user DB might get ifmp 0. Reserving slot 0 makes all ifmp start with 1, so it won't break existing
//  clients' code logic which checks ifmp against 0.
const ULONG cfmpReserved                        = 1;

const DBID dbidTemp                     = 0;
const DBID dbidMin                      = 0;
const DBID dbidUserLeast                = 1;

const DBID dbidMax                      = 7;        //  limited by the 6 slots we have in the log header for DB paths (temp DB is 7th, not logged).
const DBID dbidMask                     = 0x7f;     //  WARNING: if increasing dbidMax, consider
                                                    //  enough to accommodate all attachments

//  the number of DBs supported
//const ULONG cMaxDatabasesPerInstance          = 0xf0;     //  UNDONE: if we go any higher, need to change DBID from a byte
const ULONG cMaxDatabasesPerInstanceDefault     = dbidMax;  //  DBIDs current limiting factor

const IFMP ifmpNil                      = 0x7fffffff;       //  UNDONE: should be using this as a sentinel value instead of g_ifmpMax

//  Maximum I/O sizes to be issued by the engine

const ULONG cbReadSizeMax   = 512 * 1024;
const ULONG cbWriteSizeMax  = 512 * 1024;

const ULONG JET_IOPriorityMax = ( JET_IOPriorityLow | JET_IOPriorityLowForCheckpoint | JET_IOPriorityLowForScavenge );

//  Batch size of autoInc pool

const LONG  g_ulAutoIncBatchSize        = 1000;

//  Default version bucket page size

const ULONG cbDefaultVerPageSize        = 16384 * sizeof(VOID*) / 4;

//  Lock ranks

const INT rankDBGPrint                  = 0;
const INT rankBFIssueListSync           = 0;
const INT rankIOThreadInfoTable         = 0;
const INT rankDbtime                    = 1;
#if defined( DEBUG ) && defined( MEM_CHECK )
const INT rankCALGlobal                 = 10;
#endif  //  DEBUG && MEM_CHECK
const INT rankVERPerf                   = 10;
const INT rankLGResFiles                = 10;
const INT rankLGWaitQ                   = 10;
const INT rankLGLazyCommit              = 10;
const INT rankJetTmpLog                 = 10;
const INT rankPIBCursors                = 10;
const INT rankPIBCachePriority          = 10;
const INT rankTrxOldest                 = 10;
const INT rankIntegGlobals              = 10;
const INT rankREPAIROpts                = 10;
const INT rankREPAIRPgnoColl            = 10;
const INT rankUpgradeOpts               = 10;
const INT rankBFMaintScheduleCancel     = 10;
const INT rankDbScan                    = 10;
const INT rankDbfilehdr                 = 10;
const INT rankDBMScanSignalControl      = 10;
const INT rankBFCacheSizeSet            = 10;
const INT rankFMPRedoMaps               = 10;
const INT rankRBSFirstValidGen          = 10;
const INT rankFMPLeakEstimation         = 10;
const INT rankFlushMapAccess            = 13;
const INT rankFlushMapGrowth            = 15;
const INT rankFlushMapAsyncWrite        = 15;
const INT rankTrxOldestCached           = 15;
const INT rankShadowLogBuff             = 18;
const INT rankShadowLogConsume          = 19;
const INT rankCallbacks                 = 20;
const INT rankBucketGlobal              = 20;
const INT rankLGBuf                     = 20;
const INT rankDBMScanSerializer         = 20;
const INT rankRBSBuf                    = 20;
const INT rankAsynchIOExecuting         = 21;
const INT rankLGTrace                   = 22;
const INT rankPRL                       = 23;
const INT rankBFDUI                     = 23;
const INT rankBFDepend                  = 24;
const INT rankBFHash                    = 25;
const INT rankFCBRCEList                = 26;
const INT rankFCBHash                   = 29;
const INT rankFCBFUCBLISTRWL            = 29;
const INT rankFCBSXWL                   = 30;
const INT rankPIBLogBeginTrx            = 30;
const INT rankDBMScanSerializerFactory  = 30;
const INT rankDDLDML                    = 31;
const INT rankCATHash                   = 35;
const INT rankPIBConcurrentDDL          = 40;
const INT rankFCBList                   = 40;
const INT rankFILEUnverCol              = 50;
const INT rankFILEUnverIndex            = 50;
const INT rankFCBCreate                 = 50;
const INT rankFMP                       = 60;
const INT rankBFOB0                     = 65;
const INT rankBFLgposModifyHist         = 65;
const INT rankRBSWrite                  = 65;
const INT rankBFFMPContext              = 66;
const INT rankBFLRUK                    = 70;
const INT rankDefragPauseManager        = 70;
const INT rankFMPDetaching              = 75;
const INT rankFMPGlobal                 = 80;
const INT rankSysParamFixup             = 85;
const INT rankPIBGlobal                 = 90;
const INT rankRCEChain                  = 100;
const INT rankOSUInit                   = 100;

const INT rankBFLatch               = 1000;
const INT rankBFCacheSizeResize     = 1000;
const INT rankIO                    = 1000;
const INT rankBegin0Commit0         = 5000;
const INT rankPIBTrx                = 6000;
const INT rankLVCreate              = 7000;
const INT rankDefragManager         = 7500;
const INT rankIndexingUpdating      = 8000;
const INT rankTTMAP                 = 9000;
const INT rankOLD                   = 9000;
const INT rankRCEClean              = 9000;
const INT rankKVPStore              = 9000;
const INT rankBackupInProcess       = 9600;
const INT rankCheckpoint            = 10000;
const INT rankLGWrite               = 11000;
const INT rankTempDbCreate          = 12000;
const INT rankOpenDbCheck           = 12000;
const INT rankCompact               = 12100;
const INT rankAPI                   = 13000;
const INT rankInstance              = 14000;
const INT rankRestoreInstance       = 14100;
const INT rankOSSnapshot            = 14500;


const char szCheckpoint[]           = "Checkpoint";
const char szCritCallbacks[]            = "Callbacks";
#if defined(DEBUG) && defined(MEM_CHECK)
const char szCALGlobal[]                = "rgCAL";
#endif  //  DEBUG && MEM_CHECK
const char szLGBuf[]                    = "LGBuf";
const char szLGTrace[]              = "LGTrace";
const char szLGResFiles[]           = "LGResFiles";
const char szLGWaitQ[]              = "LGWaitQ";
const char szJetTmpLog[]                = "JetTmpLog";
const char szLGWrite[]              = "LGWrite";
const char szShadowLogConsume[]     = "ShadowLogConsume";
const char szShadowLogBuff[]        = "ShadowLogBuff";
const char szRES[]                  = "RES";
const char szPIBGlobal[]                = "PIBGlobal";
const char szOLDTaskq[]             = "OLDTaskQueue";
const char szFCBCreate[]                = "FCBCreate";
const char szFCBList[]              = "FCBList";
const char szFCBRCEList[]           = "FCBRCEList";
const char szFCBSXWL[]              = "FCB::m_sxwl";
const char szFCBFUCBLISTRWL[]       = "FCB::m_itaFucbListRwl";
const char szFMPGlobal[]            = "FMPGlobal";
const char szFMP[]                  = "FMP";
const char szDbtime[]               = "Dbtime";
const char szFMPDetaching[]         = "FMPDetaching";
const char szFMPRedoMaps[]          = "FMPRedoMaps";
const char szDBGPrint[]             = "DBGPrint";
const char szRCEClean[]             = "RCEClean";
const char szBucketGlobal[]         = "BucketGlobal";
const char szRCEChain[]             = "RCEChain";
const char szPIBTrx[]               = "PIBTrx";
const char szPIBLogBeginTrx[]       = "PIBLogBeginTrx";
const char szPIBConcurrentDDL[]     = "PIBConcurrentDDL";
const char szPIBCursors[]           = "PIBCursors";
const char szPIBCachePriority[]     = "PIBCachePriority";
const char szVERPerf[]              = "VERPerf";
const char szLVCreate[]             = "LVCreate";
const char szTrxOldest[]            = "TrxOldest";
const char szTrxOldestCached[]      = "TrxOldestCached";
const char szFILEUnverCol[]         = "FILEUnverCol";
const char szFILEUnverIndex[]       = "FILEUnverIndex";
const char szInstance[]             = "Instance";
const char szAPI[]                  = "API";
const char szBegin0Commit0[]            = "Begin0/Commit0";
const char szIndexingUpdating[]     = "Indexing/Updating";
const char szDDLDML[]               = "DDL/DML";
const char szTTMAP[]                    = "TTMAP";
const char szCritOLD[]              = "OLD";
const char szIntegGlobals[]         = "IntegGlobals";
const char szREPAIROpts[]           = "REPAIROpts";
const char szUpgradeOpts[]          = "UpgradeOpts";
const char szBFDUI[]                    = "BFDUI";
const char szBFHash[]               = "BFHash";
const char szBFLRUK[]               = "BFLRUK";
const char szBFOB0[]                    = "BFOB0";
const char szBFLgposModifyHist[]        = "LgposModifyHist BF/LOG Histogram";
const char szBFFMPContext[]         = "BFFMPContext Updating/Accessing";
const char szBFAutoEvict[]          = "BFAutoEvict Flushing/Evicting";
const char szBFLatch[]              = "BFLatch Read/RDW/Write";
const char szBFDepend[]             = "BFDepend";
const char szBFParm[]               = "BFParm";
const char szRestoreInstance[]      = "RestoreInstance";
const char szDbScan[]               = "DBSCAN";
const char szDbfilehdr[]            = "Dbfilehdr";
const char szPRL[]                  = "PatchRequestList";
const char szKVPStore[]             = "KVPStore";
const char szTempDbCreate[]         = "TempDbCreate";
const char szOpenDbCheck[]          = "OpenDbCheck";
const char szCompact[]              = "JetCompact";
const char szRBSBuf[]               = "RBSBuffer";
const char szRBSWrite[]             = "RBSWrite";
const char szRBSFirstValidGen[]     = "RBSFirstValidGen";
const char szOSUInit[]              = "OSUInit";

//  Internal user IDs for OPERATION_CONTEXT (required to set identifiable context on internal PIBs)
const DWORD OCUSER_UNINIT           = ( OC_bitInternalUser );
const DWORD OCUSER_INTERNAL         = ( OC_bitInternalUser | 1 );
//const DWORD OCUSER_SHADOWLOG      = ( OC_bitInternalUser | 2 );   // deprecated, reuse later
const DWORD OCUSER_DBSCAN           = ( OC_bitInternalUser | 3 );
const DWORD OCUSER_BACKUP           = ( OC_bitInternalUser | 4 );
const DWORD OCUSER_KVPSTORE         = ( OC_bitInternalUser | 5 );
const DWORD OCUSER_VERSTORE         = ( OC_bitInternalUser | 6 );
//const DWORD OCUSER_FLUSHMAP           = ( OC_bitInternalUser | 7 );   // deprecated, reuse later

