// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file is generated by params.pl

namespace Microsoft
{
#if (ESENT)
namespace Windows
#else
namespace Exchange
#endif
{
namespace Isam
{

// JET_param* appears as #defines in jet.h, we rename them slightly (dropping JET_param) to
// avoid a preprocessor nightmare

MSINTERNAL enum class MJET_PARAM
{

    SystemPath = 0, // path to check point file
    TempPath = 1, // path to the temporary database
    LogFilePath = 2, // path to the log file directory
    BaseName = 3, // base name for all DBMS object names
    EventSource = 4, // language independent process descriptor string
    MaxSessions = 5, // maximum number of sessions
    MaxOpenTables = 6, // maximum number of open directories
    PreferredMaxOpenTables = 7, // preferred maximum number of open directories
    CachedClosedTables = 125, // number of closed tables to cache the meta-data for
    MaxCursors = 8, // maximum number of open cursors
    MaxVerPages = 9, // maximum version store size in version pages
    PreferredVerPages = 63, // preferred version store size in version pages
    GlobalMinVerPages = 81, // minimum version store size for all instances in version pages
    VersionStoreTaskQueueMax = 105, // maximum number of tasks in the task queue before start dropping the tasks
    MaxTemporaryTables = 10, // maximum concurrent open temporary table/index creation
    LogFileSize = 11, // log file size in kBytes
    LogBuffers = 12, // log buffers in 512 byte units.
    WaitLogFlush = 13, // log flush wait time in milliseconds
    LogCheckpointPeriod = 14, // checkpoint period in sectors
    LogWaitingUserMax = 15, // maximum sessions waiting log flush
    CommitDefault = 16, // default grbit for JetCommitTransaction
    CircularLog = 17, // boolean flag for circular logging
    DbExtensionSize = 18, // database extension size in pages
    PageTempDBMin = 19, // minimum size temporary database in pages
    PageFragment = 20, // maximum disk extent considered fragment in pages
    PageReadAheadMax = 21, // maximum read-ahead in pages
    EnableFileCache = 126, // enable the use of the OS file cache for all managed files
    VerPageSize = 128, // the version store page size
    Configuration = 129, // RESETs all parameters to their default for a given configuration
    EnableAdvanced = 130, // enables the modification of advanced settings
    MaxColtyp = 131, // maximum coltyp supported by this version of ESE
    BatchIOBufferMax = 22, // maximum batch I/O buffers in pages
    CacheSize = 41, // current cache size in pages
    CacheSizeMin = 60, // minimum cache size in pages
    CacheSizeMax = 23, // maximum cache size in pages
    CheckpointDepthMax = 24, // maximum checkpoint depth in bytes
    LRUKCorrInterval = 25, // time (usec) under which page accesses are correlated
    LRUKHistoryMax = 26, // maximum LRUK history records
    LRUKPolicy = 27, // K-ness of LRUK page eviction algorithm (1...2)
    LRUKTimeout = 28, // time (sec) after which cached pages are always evictable
    LRUKTrxCorrInterval = 29, // Not Used: time (usec) under which page accesses by the same transaction are correlated
    OutstandingIOMax = 30, // maximum outstanding I/Os
    StartFlushThreshold = 31, // evictable pages at which to start a flush (proportional to CacheSizeMax)
    StopFlushThreshold = 32, // evictable pages at which to stop a flush (proportional to CacheSizeMax)
    TableClassName = 33, // table stats class name (class #, string)
    IdleFlushTime = 106, // time interval (msec) over which all dirty pages should be written to disk after idle conditions are detected.
    VAReserve = 109, // amount of address space (bytes) to reserve from use by the cache
    DBAPageAvailMin = 120, // limit of VM pages available at which NT starts to page out processes
    DBAPageAvailThreshold = 121, // constant used in internal calculation of page eviction rate *THIS IS A DOUBLE, PASS A POINTER*
    DBAK1 = 122, // constant used in internal DBA calculation *THIS IS A DOUBLE, PASS A POINTER*
    DBAK2 = 123, // constant used in internal DBA calculation *THIS IS A DOUBLE, PASS A POINTER*
    MaxRandomIOSize = 124, // maximum size of scatter/gather I/O in bytes
    EnableViewCache = 127, // enable the use of memory mapped file I/O for database files
    CheckpointIOMax = 135, // maxiumum number of pending flush writes
    TableClass1Name = 137, // name of tableclass1
    TableClass2Name = 138, // name of tableclass2
    TableClass3Name = 139, // name of tableclass3
    TableClass4Name = 140, // name of tableclass4
    TableClass5Name = 141, // name of tableclass5
    TableClass6Name = 142, // name of tableclass6
    TableClass7Name = 143, // name of tableclass7
    TableClass8Name = 144, // name of tableclass8
    TableClass9Name = 145, // name of tableclass9
    TableClass10Name = 146, // name of tableclass10
    TableClass11Name = 147, // name of tableclass11
    TableClass12Name = 148, // name of tableclass12
    TableClass13Name = 149, // name of tableclass13
    TableClass14Name = 150, // name of tableclass14
    TableClass15Name = 151, // name of tableclass15
    TableClass16Name = 196, // name of tableclass16
    TableClass17Name = 197, // name of tableclass17
    TableClass18Name = 198, // name of tableclass18
    TableClass19Name = 199, // name of tableclass19
    TableClass20Name = 200, // name of tableclass20
    TableClass21Name = 201, // name of tableclass21
    TableClass22Name = 202, // name of tableclass22
    TableClass23Name = 203, // name of tableclass23
    TableClass24Name = 204, // name of tableclass24
    TableClass25Name = 205, // name of tableclass25
    TableClass26Name = 206, // name of tableclass26
    TableClass27Name = 207, // name of tableclass27
    TableClass28Name = 208, // name of tableclass28
    TableClass29Name = 209, // name of tableclass29
    TableClass30Name = 210, // name of tableclass30
    TableClass31Name = 211, // name of tableclass31
    IOPriority = 152, // adjust IO priority per instance, anytime. Mainly for background recovery
    Recovery = 34, // enable recovery via setting the string "On" or "Off"
    OnLineCompact = 35, // enable online defrag
    EnableOnlineDefrag = 35, // enable online defrag
    AssertAction = 36, // action on assert
    PrintFunction = 37, // synched print function
    TransactionLevel = 38, // transaction level of session [deprecated - use JetGetSessionParameter( sesid, JET_sesparamTransactionLevel ... )]
    RFS2IOsPermitted = 39, // #IOs permitted to succeed [-1 = all]
    RFS2AllocsPermitted = 40, // #allocs permitted to success [-1 = all]
    CacheRequests = 42, // #cache requests (Read Only)
    CacheHits = 43, // #cache hits (Read Only)
    CheckFormatWhenOpenFail = 44, // JetInit may return JET_errDatabaseXXXformat instead of database corrupt when it is set
    EnableTempTableVersioning = 46, // Enable versioning of temp tables
    IgnoreLogVersion = 47, // Do not check the log version
    DeleteOldLogs = 48, // Delete the log files if the version is old, after deleting may make database non-recoverable
    EventSourceKey = 49, // Event source registration key value
    NoInformationEvent = 50, // Disable logging information event
    EventLoggingLevel = 51, // Set the type of information that goes to event log
    DeleteOutOfRangeLogs = 52, // Delete the log files that are not matching (generation wise) during soft recovery
    AccessDeniedRetryPeriod = 53, // Number of milliseconds to retry when about to fail with AccessDenied
    EnableIndexChecking = 45, // Enable checking OS version for indexes (false by default).
    EnableIndexCleanup = 54, // Enable cleanup of out-of-date index entries (Windows 2003 through Windows 7); Does NLS version checking (Windows 2003 and later).
    Flight_SmoothIoTestPermillage = 55, // The per mille of total (or one thousandths, or tenths of a percent) of IO should be made smooth.  Ex(s): 995‰ = 99.5% smooth, 10‰ = 1%, etc.  0 = disabled.
    Flight_ElasticWaypointLatency = 56, // Amount of extra elastic waypoint latency
    Flight_SynchronousLVCleanup = 57, // Perform synchronous cleanup (actual delete) of LVs instead of flag delete with cleanup happening later
    Flight_RBSRevertIOUrgentLevel = 58, // IO urgent level for reverting the databases using RBS. Used to decide how many outstanding I/Os will be allowed.
    Flight_EnableXpress10Compression = 59, // Enable Xpress10 compression using corsica hardware
    LogFileFailoverPath = 61, // path to use if the log file disk should fail
    EnableImprovedSeekShortcut = 62, // check to see if we are seeking for the record we are currently on
    DatabasePageSize = 64, // set database page size
    DisableCallbacks = 65, // turn off callback resolution (for defrag/repair)
    AbortRetryFailCallback = 108, // I/O error callback (JET_ABORTRETRYFAILCALLBACK)
    BackupChunkSize = 66, // backup read size in pages
    BackupOutstandingReads = 67, // backup maximum reads outstanding
    LogFileCreateAsynch = 69, // prepares next log file while logging to the current one to smooth response time
    ErrorToString = 70, // turns a JET_err into a string (taken from the comment in jet.h)
    ZeroDatabaseDuringBackup = 71, // Overwrite deleted records/LVs during backup
    UnicodeIndexDefault = 72, // default LCMapString() lcid and flags to use for CreateIndex() and unique multi-values check
    RuntimeCallback = 73, // pointer to runtime-only callback function
    Flight_EnableReattachRaceBugFix = 74, // Enable bug fix for race between dirty-cache-keep-alive database reattach and checkpoint update
    EnableSortedRetrieveColumns = 76, // internally sort (in a dynamically allocated parallel array) JET_RETRIEVECOLUMN structures passed to JetRetrieveColumns()
    CleanupMismatchedLogFiles = 77, // instead of erroring out after a successful recovery with JET_errLogFileSizeMismatchDatabasesConsistent, ESE will silently delete the old log files and checkpoint file and continue operations
    RecordUpgradeDirtyLevel = 78, // how aggresively should pages with their record format converted be flushed (0-3)
    RecoveryCurrentLogfile = 79, // which generation is currently being replayed (read only)
    ReplayingReplicatedLogfiles = 80, // if a logfile doesn't exist, wait for it to be created
    OSSnapshotTimeout = 82, // timeout for the freeze period in msec
    Flight_NewQueueOptions = 84, // Controls options for new Meted IO Queue
    Flight_ConcurrentMetedOps = 85, // Controls how many IOs we leave out at once for the new Meted IO Queue.
    Flight_LowMetedOpsThreshold = 86, // Controls the transition from 1 meted op to JET_paramFlight_ConcurrentMetedOps (which is the max).
    Flight_MetedOpStarvedThreshold = 87, // Milliseconds until a meted IO op is considered starved and dispatched no matter what.
    Flight_EnableShrinkArchiving = 89, // Turns on archiving truncated data when shrinking a database (subject to efv).
    Flight_EnableBackupDuringRecovery = 90, // Turns on backup during recovery (i.e. seed from passive copy).
    Flight_RBSRollIntervalSec = 91, // Time after which we should roll into new revert snapshot.
    Flight_RBSMaxRequiredRange = 92, // Max required range allowed for revert snapshot. If combined required range of the dbs is greater than this we will skip creating the revert snapshot 
    Flight_RBSCleanupEnabled = 93, // Turns on clean up for revert snapshot.
    Flight_RBSLowDiskSpaceThresholdGb = 94, // Low disk space in gigabytes at which we will start cleaning up RBS aggressively.
    Flight_RBSMaxSpaceWhenLowDiskSpaceGb = 95, // Max alloted space in gigabytes for revert snapshots when the disk space is low.
    Flight_RBSMaxTimeSpanSec = 96, // Max timespan of a revert snapshot
    Flight_RBSCleanupIntervalMinSec = 97, // Min time between cleanup attempts of revert snapshots.
    ExceptionAction = 98, // what to do with exceptions generated within JET
    EventLogCache = 99, // number of bytes of eventlog records to cache if service is not available
    CreatePathIfNotExist = 100, // create system/temp/log/log-failover paths if they do not exist
    PageHintCacheSize = 101, // maximum size of the fast page latch hint cache in bytes
    OneDatabasePerSession = 102, // allow just one open user database per session
    MaxDatabasesPerInstance = 103, // maximum number of databases per instance
    MaxInstances = 104, // maximum number of instances per process
    DisablePerfmon = 107, // disable perfmon support for this process
    IndexTuplesLengthMin = 110, // for tuple indexes, minimum length of a tuple
    IndexTuplesLengthMax = 111, // for tuple indexes, maximum length of a tuple
    IndexTuplesToIndexMax = 112, // for tuple indexes, maximum number of characters in a given string to index
    AlternateDatabaseRecoveryPath = 113, // recovery-only - search for dirty-shutdown databases in specified location only
    IndexTupleIncrement = 132, // for tuple indexes, offset increment for each succesive tuple
    IndexTupleStart = 133, // for tuple indexes, offset to start tuple indexing
    KeyMost = 134, // read only maximum settable key length before key trunctation occurs
    LegacyFileNames = 136, // Legacy  file name characteristics to preserve ( JET_bitESE98FileNames | JET_bitEightDotThreeSoftCompat )
    EnablePersistedCallbacks = 156, // allow the database engine to resolve and use callbacks persisted in a database
    WaypointLatency = 153, // The latency (in logs) behind the tip / highest committed log to defer database page flushes.
    CheckpointTooDeep = 154, // The maximum allowed depth (in logs) of the checkpoint.  Once this limit is reached, updates will fail with JET_errCheckpointDepthTooDeep.
    DisableBlockVerification = 155, // TEST ONLY:  use to disable block checksum verification for file fuzz testing
    AggressiveLogRollover = 157, // force log rollover after certain operations
    PeriodicLogRolloverLLR = 158, // force log rollover after a certain length of inactivity (currently requires setting a waypoint to take affect)
    UsePageDependencies = 159, // use page dependencies when logging splits/merges (reduces data logged)
    DefragmentSequentialBTrees = 160, // Turn on/off automatic sequential B-tree defragmentation tasks (On by default, but also requires JET_SPACEHINTS flags / JET_bitRetrieveHintTableScan* to trigger on any given tables).
    DefragmentSequentialBTreesDensityCheckFrequency = 161, // Determine how frequently B-tree density is checked
    IOThrottlingTimeQuanta = 162, // Max time (in MS) that the I/O throttling mechanism gives a task to run for it to be considered 'completed'.
    LVChunkSizeMost = 163, // Max LV chunk size supported wrt the chosen page size (R/O)
    MaxCoalesceReadSize = 164, // Max number of bytes that can be grouped for a coalesced read operation.
    MaxCoalesceWriteSize = 165, // Max number of bytes that can be grouped for a coalesced write operation.
    MaxCoalesceReadGapSize = 166, // Max number of bytes that can be gapped for a coalesced read IO operation.
    MaxCoalesceWriteGapSize = 167, // Max number of bytes that can be gapped for a coalesced write IO operation.
    EnableHaPublish = 168, // Event through HA publishing mechanism.
    EnableDBScanInRecovery = 169, // Do checksumming of the database during recovery.
    DbScanThrottle = 170, // throttle (mSec).
    DbScanIntervalMinSec = 171, // Min internal to repeat checksumming (Sec).
    DbScanIntervalMaxSec = 172, // Max internal checksumming must finish (Sec).
    EmitLogDataCallback = 173, // Set the callback for emitting log data to an external 3rd party.
    EmitLogDataCallbackCtx = 174, // Context for the the callback for emitting log data to an external 3rd party.
    EnableExternalAutoHealing = 175, // Enable logging of page patch request and callback on page patch request processing (and corrupt page notification) during recovery.
    PatchRequestTimeout = 176, // Time before an outstanding patch request is considered stale (Seconds).
    CachePriority = 177, // Per-instance property for relative cache priorities (default = 100).
    MaxTransactionSize = 178, // Percentage of version store that can be used by oldest transaction before JET_errVersionStoreOutOfMemory (default = 100).
    PrereadIOMax = 179, // Maximum number of I/O operations dispatched for a given purpose.
    EnableDBScanSerialization = 180, // Database Maintenance serialization is enabled for databases sharing the same disk.
    HungIOThreshold = 181, // The threshold for what is considered a hung IO that should be acted upon.
    HungIOActions = 182, // A set of actions to be taken on IOs that appear hung.
    MinDataForXpress = 183, // Smallest amount of data that should be compressed with xpress compression.
    EnableShrinkDatabase = 184, // Release space back to the OS when deleting data. This may require an OS feature of Sparse Files, and is subject to change.
    ProcessFriendlyName = 186, // Friendly name for this instance of the process (e.g. performance counter global instance name, event logs).
    DurableCommitCallback = 187, // callback for when log is flushed
    EnableSqm = 188, // Deprecated / ignored param.
    ConfigStoreSpec = 189, // Custom path that allows the consumer to specify a path (currently from in the registry) from which to pull custom ESE configuration.
    StageFlighting = 190, // It's like stage fright-ing but different - use JET_bitStage* bits.
    ZeroDatabaseUnusedSpace = 191, // Controls scrubbing of unused database space.
    DisableVerifications = 192, // Verification modes disabled
    PersistedLostFlushDetection = 193, // Configures persisted lost flush detection for databases while attached to an instance.
    EngineFormatVersion = 194, // Engine format version - specifies the maximum format version the engine should allow, ensuring no format features are used beyond this (allowing the DB / logs to be forward compatible).
    ReplayThrottlingLevel = 195, // Should replay be throttled so as not to generate too much disk IO
    BlockCacheConfiguration = 212, // Configuration for the ESE Block Cache via an IBlockCacheConfiguration* (optional).
    RecordSizeMost = 213, // Read only param that returns the maximum record size supported by the current pagesize.
    UseFlushForWriteDurability = 214, // This controls whether ESE uses Flush or FUA to make sure a write to disk is durable.
    EnableRBS = 215, // Turns on revert snapshot. Not an ESE flight as we will let the variant be controlled outside ESE (like HA can enable this when lag is disabled)
    RBSFilePath = 216, // path to the revert snapshot directory
    MaxValueInvalid = 217, // This is not a valid parameter. It can change from release to release!
};

}
}
}
