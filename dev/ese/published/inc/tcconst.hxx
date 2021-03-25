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

//  PLEASE DO NOT SHIFT IOFILE TYPES AROUND ... we're building tools to depend upon these
//  reasons, so they need to be static.

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

enum IOCATEGORY // iocat
{
    iocatUnknown        = 0,
    iocatMaintenance    = 1,
    iocatTransactional  = 2,
    iocatMax
};

const QWORD qwLegacyFileID          = 0x10000001FFFFFFFF;   // "operational" file IDs
const QWORD qwSetDbSizeFileID       = 0x10000002FFFFFFFF;
const QWORD qwDumpingFileID         = 0x10000003FFFFFFFF;
const QWORD qwDefragFileID          = 0x10000004FFFFFFFF;
const QWORD qwPatchFileID           = 0x10000005FFFFFFFF;
const QWORD qwCheckpointFileID      = 0x2000000000000000;   // top DWORD is m_iInstance (except top nibble)
const QWORD qwLogFileMgmtID         = 0x3000000000000000;   // top DWORD is m_iInstance (except top nibble), bottom DWORD is log generation (when it's filled in)
const QWORD qwLogFileID             = 0x4000000000000000;   // top DWORD is m_iInstance (except top nibble), bottom DWORD is log generation (when it's filled in)
//nst QWORD qwBackupFileID          = 0x5000000000000000;   // DEPRECATED - now qwBackupDbID or qwBackupLogID
const QWORD qwBackupDbID            = 0x6000000000000000;   // low DWORD is ifmp.
const QWORD qwBackupLogID           = 0x7000000000000000;   // top DWORD is m_iInstance (except top nibble)
const QWORD qwRBSFileID             = 0x8000000000000000;   // top DWORD is m_iInstance (except top nibble)
const QWORD qwRBSRevertChkFileID    = 0x9000000000000000;   // top DWORD is m_iInstance (except top nibble)

const LONG lGenSignalCurrentID  = 0x80000001;           // signal to indicate current lgen where we can't know the lgen apriori
const LONG lGenSignalTempID     = 0x80000002;           // signal to indicate current lgen where we can't know the lgen apriori
const LONG lgenSignalReserveID  = 0x80000003;           // signal to indicate unknown lgen because the reserve log file hasn't been assigned to one yet

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

    // note: (ULONG)lGen, must cast to unsigned to avoid sign extension from lGenSignal* constants.
    const QWORD qwRet = ( qwInstFileType | ( (QWORD)iInstance << 32 ) | (ULONG)lGen );

    return qwRet;
}


//  The 4 sub-components of an OS File's IO reasons are defined here.  From a high
//  level the ESE sub-reasons will be broken up thusly:
//
//      Primary     - The Buffer Manager / BF and Log / LOG will predominantly own 
//                  the primary reasons for IO, as they are first contact with the 
//                  OS layer when the system does IO.
//      Secondary   - Secondary reasons are owned by BT / Node / the layer above BF that issues IO
//      Tertiary    - Tertiary reasons are owned by layers above BT, and internal clients of the system.
//                  Examples: DBScan, space, verstore etc.
//      User        - This containst the public Jet Api (JetOp) called that caused the system to do IO.
//      Flags       - These are options on IO.
//
//  PLEASE DO NOT SHIFT IOREASONs AROUND ... we're building tools to depend upon these
//  reasons, so they need to be static.

enum IOREASONPRIMARY : BYTE
{
    //iorpNone = 0, defined generically by OS layer
    iorpInvalid = 0,

    iorpHeader = 1,

    // PLEASE attempt not to use values = 2 through 21 as SOMEONE's tool for correlating
    // I/Os depends upon it.
    //

    //  Generic OS Layer reasons
    //
    iorpOsLayerTracing = 24,

    //  Database I Reasons
    //
    iorpBFRead = 32,            // deprecated
    iorpBFLatch = 33,
    iorpBFPreread = 34,
    iorpBFRemapReVerify = 35,
    //  Note: we do pre-read as a flag.

    //  Database O Reasons
    //
    iorpBFCheckpointAdv = 64,
    iorpBFAvailPool = 65,
    iorpBFShrink = 66,
    // iorpBFIdleFlush = 67,        // 67 can be re-used.
    iorpBFFilthyFlush = 68,
    iorpBFDatabaseFlush = 69,
    iorpSPDatabaseInlineZero = 70,  // Used to 'de-sparse' a region.
    iorpBFPurge = 71,
    iorpBFImpedingWriteCleanDoubleIo = 72, // should not happen - see comment above usage.

    //  Transaction Log I/O Reasons
    //
    iorpLog = 80,
    iorpLGWriteCapacity = 81,
    iorpLGWriteCommit = 82,
    iorpLGWriteNewGen = 83,
    iorpLGWriteSignal = 84, // at least some times this is capacity flush ...
    iorpLGFlushAll = 85,
    iorpLogRecRedo = 86,
    //  Alternate Transaction Log I/O Reasons
    iorpShadowLog = 92,

    //  Specialty reasons ...
    //
    iorpCheckpoint = 96,    // the file itself, not the BF process.
    iorpDirectAccessUtil = 97,  // direct data access for clients directly reading on-disk data.
    iorpBackup = 98,
    iorpRestore = 99,
    iorpPatchFix = 100,
    iorpDatabaseExtension = 101,
    iorpDatabaseShrink = 102,
    iorpDbScan = 103,
    iorpCorruptionTestHook = 104,   //  this is only driven by a test hook
    iorpDatabaseTrim = 105,
    iorpFlushMap = 106,
    iorpBlockCache = 107,
    iorpRBS = 108,
    iorpRevertPage = 109,
    iorpRBSRevertCheckpoint = 110,

    // Are you sure you want to append the IO reason at the end? There are other
    // more specific categories above.
};

enum IOREASONSECONDARY : BYTE
{
    //iorsNone = 0, defined generically by OS layer

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

    // Defined by LG as iorsLRNOP + lrtyp, reserved through iorsLRMax
    iorsLRNOP = 128,
    iorsLRMax = 255,
};

enum IOREASONTERTIARY : BYTE
{
    iortISAM = 1,   // An external client called the JET Api to perform an operation, IOREASONUSER tells which Api was called

    // Internal components/reasons that initiated an IO
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
    iortRecoveryRedo = 16,
    iortRecoveryUndo = 17,
    iortRecoveryRedoNewLogs = 18,
    iortMax = 19,
};

enum IOREASONUSER : BYTE
{
    // Will hold the 8-bit Jet op defined in _jet.hxx
    ioruMax = 255,
};

enum IOREASONFLAGS : BYTE
{
    //iorfNone = 0, defined generically by OS layer

    iorfShadow      = 0x01,
    iorfFill        = 0x02,
    iorfOpportune   = 0x04,
    iorfForeground  = 0x08,
    iorfDependantOrVersion  = 0x10,
    iorfReclaimPageFromOS   = 0x20,
    iorfSuperColdOrLowPriority   = 0x40,
};

enum IOFLUSHREASON : ULONG
{
    iofrInvalid                     = 0x0,

    iofrLogPatch                    = 0x00000001,
    iofrLogCommitFlush              = 0x00000002,
    iofrLogCapacityFlush            = 0x00000004,
    iofrLogSignalFlush              = 0x00000008,   //  This seems to be capacity flush in disguise?  from tcconst.hxx on iorpLGFlushSignal: "at least some times this is capacity flush ..." .. always?  what's iorpLGCapacityFlush for?
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

    iofrWriteBackFlushIfmpContext   = 0x00004000,   //  [Default] Only original FFB call (pre the FFB conversion) for write-back DBs (for RW, paramEnableFileCache and no logging)
    iofrFlushIfmpContext            = 0x00008000,

    iofrDbResize                    = 0x00010000,   //  database is being resized (grown or shrunk)
    iofrUtility                     = 0x00020000,   //  trailer / header restore patching, and other misc stuff?
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

    // Reserved by OS Layer:        = 0x80000000,
};

#endif  // !_TCCONST_H

