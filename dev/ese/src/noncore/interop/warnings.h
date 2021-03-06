// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// This file is generated by warnings.pl

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

// JET_wrn* appears as #defines in jet.h, we rename them slightly (dropping JET_wrn) to
// avoid a preprocessor nightmare

MSINTERNAL enum class MJET_WRN
{

    None = 0,
    RemainingVersions = 321, // The version store is still active
    UniqueKey = 345, // seek on non-unique index yielded a unique key
    SeparateLongValue = 406, // Column is a separated long-value
    NoMoreRecords = 428, // No more records to stream
    ExistingLogFileHasBadSignature = 558, // Existing log file has bad signature
    ExistingLogFileIsNotContiguous = 559, // Existing log file is not contiguous
    SkipThisRecord = 564, // INTERNAL ERROR
    TargetInstanceRunning = 578, // TargetInstance specified for restore is running
    CommittedLogFilesLost = 585, // One or more logs that were committed to this database, were not recovered.  The database is still clean/consistent, as though the lost log's transactions were committed lazily (and lost).
    CommittedLogFilesRemoved = 587, // One or more logs that were committed to this database, were no recovered.  The database is still clean/consistent, as though the corrupted log's transactions were committed lazily (and lost).
    FinishWithUndo = 588, // Signal used by clients to indicate JetInit() finished with undo
    DatabaseRepaired = 595, // Database corruption has been repaired
    ColumnNull = 1004, // Column is NULL-valued
    BufferTruncated = 1006, // Buffer too small for data
    DatabaseAttached = 1007, // Database is already attached
    SortOverflow = 1009, // Sort does not fit in memory
    SeekNotEqual = 1039, // Exact match not found during seek
    NoErrorInfo = 1055, // No extended error information
    NoIdleActivity = 1058, // No idle activity occurred
    NoWriteLock = 1067, // No write lock at transaction level 0
    ColumnSetNull = 1068, // Column set to NULL-value
    ShrinkNotPossible = 1122, // Database file could not be shrunk because there is not enough internal free space available or there is unmovable data present.
    DTCCommitTransaction = 1163, // Warning code DTC callback should return if the specified transaction is to be committed
    DTCRollbackTransaction = 1164, // Warning code DTC callback should return if the specified transaction is to be rolled back
    TableEmpty = 1301, // Opened an empty table
    TableInUseBySystem = 1327, // System cleanup has a cursor open on the table
    CorruptIndexDeleted = 1415, // Out of date index removed
    PrimaryIndexOutOfDate = 1417, // The Primary index is created with an incompatible OS sort version. The table can not be safely modified.
    SecondaryIndexOutOfDate = 1418, // One or more Secondary index is created with an incompatible OS sort version. Any index over Unicode text should be deleted.
    ColumnMaxTruncated = 1512, // Max length too big, truncated
    CopyLongValue = 1520, // Single instance column bursted
    TaggedColumnsRemaining = 1523, // RetrieveTaggedColumnList ran out of copy buffer before retrieving all tagged columns
    ColumnSkipped = 1531, // Column value(s) not returned because the corresponding column id or itagSequence requested for enumeration was null
    ColumnNotLocal = 1532, // Column value(s) not returned because they could not be reconstructed from the data at hand
    ColumnMoreTags = 1533, // Column values exist that were not requested for enumeration
    ColumnTruncated = 1534, // Column value truncated at the requested size limit during enumeration
    ColumnPresent = 1535, // Column values exist but were not returned by request
    ColumnSingleValue = 1536, // Column value returned in JET_COLUMNENUM as a result of JET_bitEnumerateCompressOutput
    ColumnDefault = 1537, // Column value(s) not returned because they were set to their default value(s) and JET_bitEnumerateIgnoreDefault was specified
    ColumnNotInRecord = 1539, // Column value(s) not returned because they could not be reconstructed from the data in the record
    ColumnReference = 1541, // Column value returned as a reference because it could not be reconstructed from the data in the record
    DataHasChanged = 1610, // Data has changed
    KeyChanged = 1618, // Moved to new key
    FileOpenReadOnly = 1813, // Database file is read only
    IdleFull = 1908, // Idle registry full
    DefragAlreadyRunning = 2000, // Online defrag already running on specified database
    DefragNotRunning = 2001, // Online defrag not running on specified database
    DatabaseScanAlreadyRunning = 2002, // JetDatabaseScan already running on specified database
    DatabaseScanNotRunning = 2003, // JetDatabaseScan not running on specified database
    CallbackNotRegistered = 2100, // Unregistered a non-existent callback function
    PreviousLogFileIncomplete = 2602, // The log data provided jumped to the next log suddenly, we have deleted the incomplete log file as a precautionary measure

    RecordFoundGreater = 1039,
    RecordFoundLess = 1039,

};

}
}
}
