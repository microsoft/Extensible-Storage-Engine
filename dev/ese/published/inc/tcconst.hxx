// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _TCCONST_H
#define _TCCONST_H

enum IOTYPE
{
    iotypeRead = 0,
    iotypeWrite,
    iotypeMax
};


enum IOFILE
{
    iofileOther         = 0,
    iofileDbAttached    = 1,
    iofileDbRecovery    = 2,
    iofileLog           = 3,
    iofileDbTotal       = 4,
    iofileFlushMap      = 5,
    iofileRBS           = 6,
    iofileMax           = 7
};

enum IOCATEGORY
{
    iocatUnknown        = 0,
    iocatMaintenance    = 1,
    iocatTransactional  = 2,
    iocatMax
};

const QWORD qwLegacyFileID          = 0x10000001FFFFFFFF;
const QWORD qwSetDbSizeFileID       = 0x10000002FFFFFFFF;
const QWORD qwDumpingFileID         = 0x10000003FFFFFFFF;
const QWORD qwDefragFileID          = 0x10000004FFFFFFFF;
const QWORD qwPatchFileID           = 0x10000005FFFFFFFF;
const QWORD qwCheckpointFileID      = 0x2000000000000000;
const QWORD qwLogFileMgmtID         = 0x3000000000000000;
const QWORD qwLogFileID             = 0x4000000000000000;
const QWORD qwBackupDbID            = 0x6000000000000000;
const QWORD qwBackupLogID           = 0x7000000000000000;
const QWORD qwRBSFileID             = 0x8000000000000000;
const QWORD qwRBSRevertChkFileID    = 0x9000000000000000;

const LONG lGenSignalCurrentID  = 0x80000001;
const LONG lGenSignalTempID     = 0x80000002;
const LONG lgenSignalReserveID  = 0x80000003;

INLINE QWORD QwInstFileID( const QWORD qwInstFileType, const INT iInstance, const LONG lGen )
{
#ifdef DEBUG
    Assert( qwInstFileType == qwCheckpointFileID ||
            qwInstFileType == qwLogFileID ||
            qwInstFileType == qwLogFileMgmtID ||
            qwInstFileType == qwBackupLogID ||
            qwInstFileType == qwRBSFileID );
    if ( qwInstFileType == qwCheckpointFileID )
    {
        Expected( lGen == 0 );
    }
    else
    {
        Expected( lGen != 0 );
        Expected( lGen > 0 || 
            lGen == lGenSignalCurrentID || lGen == lGenSignalTempID || lGen == lgenSignalReserveID );
    }
#endif

    const QWORD qwRet = ( qwInstFileType | ( (QWORD)iInstance << 32 ) | (ULONG)lGen );

    return qwRet;
}



enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpHeader = 1,


    iorpOsLayerTracing = 24,

    iorpBFRead = 32,
    iorpBFLatch = 33,
    iorpBFPreread = 34,
    iorpBFRemapReVerify = 35,

    iorpBFCheckpointAdv = 64,
    iorpBFAvailPool = 65,
    iorpBFShrink = 66,
    iorpBFFilthyFlush = 68,
    iorpBFDatabaseFlush = 69,
    iorpSPDatabaseInlineZero = 70,
    iorpBFPurge = 71,
    iorpBFImpedingWriteCleanDoubleIo = 72,

    iorpLog = 80,
    iorpLGWriteCapacity = 81,
    iorpLGWriteCommit = 82,
    iorpLGWriteNewGen = 83,
    iorpLGWriteSignal = 84,
    iorpLGFlushAll = 85,
    iorpLogRecRedo = 86,
    iorpShadowLog = 92,

    iorpCheckpoint = 96,
    iorpDirectAccessUtil = 97,
    iorpBackup = 98,
    iorpRestore = 99,
    iorpPatchFix = 100,
    iorpDatabaseExtension = 101,
    iorpDatabaseShrink = 102,
    iorpDbScan = 103,
    iorpCorruptionTestHook = 104,
    iorpDatabaseTrim = 105,
    iorpFlushMap = 106,
    iorpBlockCache = 107,
    iorpRBS = 108,
    iorpRevertPage = 109,
    iorpRBSRevertCheckpoint = 110,

};

enum IOREASONSECONDARY : BYTE
{

    iorsHeader = 1,
    
    iorsBTOpen = 2,
    iorsBTSeek = 3,
    iorsBTMoveFirst = 4,
    iorsBTMoveNext = 5,
    iorsBTMovePrev = 6,
    iorsBTInsert = 7,
    iorsBTAppend = 8,
    iorsBTReplace = 9,
    iorsBTDelete = 10,
    iorsBTGet = 11,
    iorsBTSplit = 12,
    iorsBTMerge = 13,
    iorsBTRefresh = 14,
    iorsBTPreread = 15,
    iorsBTMax = 16,
};
static_assert( iorsBTMax <= 16, "IOREASONSECONDARY overflow" );

enum IOREASONTERTIARY : BYTE
{
    iortISAM = 1,

    iortSpace = 2,
    iortDbScan = 3,
    iortDbShrink = 4,
    iortRollbackUndo = 5,
    iortScrubbing = 6,
    iortOLD = 7,
    iortOLD2 = 8,
    iortRecovery = 9,
    iortBackupRestore = 10,
    iortPagePatching = 11,
    iortSort = 12,
    iortRepair = 13,
    iortFreeExtSnapshot = 14,
    iortRecTask = 15,
    iortMax = 16,
};
static_assert(iortMax <= 16, "IOREASONTERTIARY overflow");

enum IOREASONUSER : BYTE
{
    ioruMax = 255,
};

enum IOREASONFLAGS : BYTE
{

    iorfShadow      = 0x01,
    iorfFill        = 0x02,
    iorfOpportune   = 0x04,
    iorfForeground  = 0x08,
    iorfDependantOrVersion  = 0x10,
    iorfReclaimPageFromOS   = 0x20,
    iorfSuperCold   = 0x40,
};

enum IOFLUSHREASON
{
    iofrInvalid                     = 0x0,

    iofrLogPatch                    = 0x00000001,
    iofrLogCommitFlush              = 0x00000002,
    iofrLogCapacityFlush            = 0x00000004,
    iofrLogSignalFlush              = 0x00000008,
    iofrPersistClosingLogFile       = 0x00000010,
    iofrLogMaxRequired              = 0x00000020,
    iofrLogFlushAll                 = 0x00000040,
    iofrLogTerm                     = 0x00000080,

    iofrDbHdrUpdMaxRequired         = 0x00000100,
    iofrDbHdrUpdMinRequired         = 0x00000200,
    iofrDbUpdMinRequired            = 0x00000400,
    iofrCheckpointMinRequired       = 0x00000800,
    iofrCheckpointForceWrite        = 0x00001000,

    iofrDbHdrUpdLgposLastResize     = 0x00002000,

    iofrWriteBackFlushIfmpContext   = 0x00004000,
    iofrFlushIfmpContext            = 0x00008000,

    iofrDbResize                    = 0x00010000,
    iofrUtility                     = 0x00020000,
    iofrIncReseedUtil               = 0x00040000,
    iofrSeparateHeaderUpdate        = 0x00080000,

    iofrFmInit                      = 0x00100000,
    iofrFmFlush                     = 0x00200000,
    iofrFmTerm                      = 0x00400000,

    iofrDefensiveCloseFlush         = 0x00800000,
    iofrDefensiveErrorPath          = 0x01000000,
    iofrDefensiveFileHdrWrite       = 0x02000000,

    iofrOpeningFileFlush            = 0x04000000,

    iofrPagePatching                = 0x08000000,

    iofrBlockCache                  = 0x10000000,

    iofrRBS                         = 0x20000000,
    iofrRBSRevertUtil               = 0x40000000,

};

#endif

