// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


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


MSINTERNAL enum class MJET_WRN
{

    None = 0,
    RemainingVersions = 321,
    UniqueKey = 345,
    SeparateLongValue = 406,
    NoMoreRecords = 428,
    ExistingLogFileHasBadSignature = 558,
    ExistingLogFileIsNotContiguous = 559,
    SkipThisRecord = 564,
    TargetInstanceRunning = 578,
    CommittedLogFilesLost = 585,
    CommittedLogFilesRemoved = 587,
    FinishWithUndo = 588,
    DatabaseRepaired = 595,
    ColumnNull = 1004,
    BufferTruncated = 1006,
    DatabaseAttached = 1007,
    SortOverflow = 1009,
    SeekNotEqual = 1039,
    NoErrorInfo = 1055,
    NoIdleActivity = 1058,
    NoWriteLock = 1067,
    ColumnSetNull = 1068,
    ShrinkNotPossible = 1122,
    DTCCommitTransaction = 1163,
    DTCRollbackTransaction = 1164,
    TableEmpty = 1301,
    TableInUseBySystem = 1327,
    CorruptIndexDeleted = 1415,
    PrimaryIndexOutOfDate = 1417,
    SecondaryIndexOutOfDate = 1418,
    ColumnMaxTruncated = 1512,
    CopyLongValue = 1520,
    TaggedColumnsRemaining = 1523,
    ColumnSkipped = 1531,
    ColumnNotLocal = 1532,
    ColumnMoreTags = 1533,
    ColumnTruncated = 1534,
    ColumnPresent = 1535,
    ColumnSingleValue = 1536,
    ColumnDefault = 1537,
    ColumnNotInRecord = 1539,
    ColumnReference = 1541,
    DataHasChanged = 1610,
    KeyChanged = 1618,
    FileOpenReadOnly = 1813,
    IdleFull = 1908,
    DefragAlreadyRunning = 2000,
    DefragNotRunning = 2001,
    DatabaseScanAlreadyRunning = 2002,
    DatabaseScanNotRunning = 2003,
    CallbackNotRegistered = 2100,
    PreviousLogFileIncomplete = 2602,

    RecordFoundGreater = 1039,
    RecordFoundLess = 1039,

};

}
}
}
