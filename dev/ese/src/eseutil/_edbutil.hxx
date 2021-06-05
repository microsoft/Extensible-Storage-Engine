// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)

#include "os.hxx"

#include "jet.h"
#ifdef ESENT
#include "ntverp.h"
#else
#include "bldver.h"
#endif


enum UTILMODE
{
    modeIntegrity,
    modeDefragment,
    modeRecovery,
    modeBackup,
    modeDump,
    modeRepair,
    modeScrub,
    modeHardRecovery,
    modeDumpRestoreEnv,
    modeChecksum,
    modeCopyFile
};

//
//  ESE Shadow Library info
//

#define ESESHADOW_LOCAL_DEFERRED_DLL

class EseShadowInformation;
typedef EseShadowInformation* EseShadowContext;

struct UTILOPTS
{
    const WCHAR *wszConfigStoreSpec;
    WCHAR       *wszSourceDB;
    WCHAR       *wszDbAltRecoveryDir;
    WCHAR       *wszLogfilePath;
    WCHAR       *wszSystemPath;
    WCHAR       *wszTempDB;
    WCHAR       *wszBackup;
    WCHAR       *wszRestore;
    WCHAR       *wszBase;
    WCHAR       *wszIntegPrefix;
    void        *pv;                    // Points to mode-specific structures.
    
    UTILMODE    mode;
    INT         fUTILOPTSFlags;
    UINT        fUTILOPTSCopyFileIgnoreIoErrors : 1;
    UINT        fUTILOPTSTrimDatabase : 1;

    JET_GRBIT   grbitInit;
    LONG        cLogDigits;
    BOOL        fIncrementalBackup;
    LONG        cpageBuffers;
    LONG        cpageBatchIO;
    LONG        cpageDbExtension;
    LONG        lDirtyLevel;
    LONG        pageTempDBMin;
    LONG        lMaxCacheSize;

    LONG            crstmap;                // values for JetInit3
    LONG            irstmap;
    JET_RSTMAP2_W   *prstmap;

    JET_GRBIT   grbitShrink;
    BOOL        fRunLeakReclaimer;

    //
    //  VSS Snapshot
    //
    // note: Only valid if fUTILOPTSOperateTempVss
    EseShadowContext    eseShadowContext;

    JET_ENGINEFORMATVERSION     efvUserSpecified;
};


// Flags:
const INT   fUTILOPTSSuppressLogo               = 0x00000001;
const INT   fUTILOPTSDefragRepair               = 0x00000002;       // Defrag mode only.
const INT   fUTILOPTSPreserveTempDB             = 0x00000004;       // Defrag and upgrade modes. In HardRecovery use for KeepLogs
const INT   fUTILOPTSDefragInfo                 = 0x00000008;       // Defrag and upgrade modes.
const INT   fUTILOPTSIncrBackup                 = 0x00000010;       // Backup only.
// const INT    fUTILOPTSInPlaceUpgrade             = 0x00000020;       // Upgrade only.
const INT   fUTILOPTSVerbose                    = 0x00000040;       // Repair/Integrity only
const INT   fUTILOPTSReportErrors               = 0x00000080;       // Repair/Integrity only
const INT   fUTILOPTSDontRepair                 = 0x00000100;       // Repair only
const INT   fUTILOPTSDumpStats                  = 0x00000200;       // Integrity only
const INT   fUTILOPTSDontBuildIndexes           = 0x00000400;       // Integrity only
const INT   fUTILOPTSDebugMode                  = 0x00000800;       // Debug mode. Behavior depends on the primary option.
const INT   fUTILOPTS8KPage                     = 0x00001000;
const INT   fUTILOPTSDumpRestoreEnv             = 0x00004000;       // HardRecovery - dump Restore.env
const INT   fUTILOPTSServerSim                  = 0x00008000;       // HardRecovery - simulate server
const INT   fUTILOPTSChecksumEDB                = 0x00010000;       // checksum EDB
const INT   fUTILOPTSRepairCheckOnly            = 0x00100000;       // Repair only
const INT   fUTILOPTS4KPage                     = 0x00200000;
const INT   fUTILOPTSDelOutOfRangeLogs          = 0x00400000;       // Recovery only
const INT   fUTILOPTS32KPage                    = 0x00800000;
const INT   fUTILOPTS16KPage                    = 0x01000000;
const INT   fUTILOPTS2KPage                     = 0x02000000;
const INT   fUTILOPTSMaybeTolerateCorruption    = 0x04000000;
const INT   fUTILOPTSOperateTempVss             = 0x08000000;
const INT   fUTILOPTSOperateTempVssRec          = 0x10000000;
const INT   fUTILOPTSOperateTempVssPause        = 0x20000000;
const INT   fUTILOPTSRecoveryWithoutUndoForReal = 0x40000000;

#define FUTILOPTSSuppressLogo( fFlags )             ( (fFlags) & fUTILOPTSSuppressLogo )
#define UTILOPTSSetSuppressLogo( fFlags )           ( (fFlags) |= fUTILOPTSSuppressLogo )
#define UTILOPTSResetSuppressLogo( fFlags )         ( (fFlags) &= ~fUTILOPTSSuppressLogo )

#define FUTILOPTS2KPage( fFlags )                   ( (fFlags) & fUTILOPTS2KPage )
#define UTILOPTSSet2KPage( fFlags )                 ( (fFlags) |= fUTILOPTS2KPage )
#define UTILOPTSReset2KPage( fFlags )               ( (fFlags) &= ~fUTILOPTS2KPage )

#define FUTILOPTS4KPage( fFlags )                   ( (fFlags) & fUTILOPTS4KPage )
#define UTILOPTSSet4KPage( fFlags )                 ( (fFlags) |= fUTILOPTS4KPage )
#define UTILOPTSReset4KPage( fFlags )               ( (fFlags) &= ~fUTILOPTS4KPage )

#define FUTILOPTS8KPage( fFlags )                   ( (fFlags) & fUTILOPTS8KPage )
#define UTILOPTSSet8KPage( fFlags )                 ( (fFlags) |= fUTILOPTS8KPage )
#define UTILOPTSReset8KPage( fFlags )               ( (fFlags) &= ~fUTILOPTS8KPage )

#define FUTILOPTS16KPage( fFlags )                  ( (fFlags) & fUTILOPTS16KPage )
#define UTILOPTSSet16KPage( fFlags )                ( (fFlags) |= fUTILOPTS16KPage )
#define UTILOPTSReset16KPage( fFlags )              ( (fFlags) &= ~fUTILOPTS16KPage )

#define FUTILOPTS32KPage( fFlags )                  ( (fFlags) & fUTILOPTS32KPage )
#define UTILOPTSSet32KPage( fFlags )                ( (fFlags) |= fUTILOPTS32KPage )
#define UTILOPTSReset32KPage( fFlags )              ( (fFlags) &= ~fUTILOPTS32KPage )

#define FUTILOPTSExplicitPageSet( fFlags )          (   FUTILOPTS2KPage((fFlags)) ||    \
                                                        FUTILOPTS4KPage((fFlags)) ||    \
                                                        FUTILOPTS8KPage((fFlags)) ||    \
                                                        FUTILOPTS16KPage((fFlags)) ||   \
                                                        FUTILOPTS32KPage((fFlags)) )

#define FUTILOPTSMaybeTolerateCorruption( fFlags )      ( (fFlags) & fUTILOPTSMaybeTolerateCorruption )
#define UTILOPTSSetMaybeTolerateCorruption( fFlags )    ( (fFlags) |= fUTILOPTSMaybeTolerateCorruption )
#define UTILOPTSResetMaybeTolerateCorruption( fFlags )  ( (fFlags) &= ~fUTILOPTSMaybeTolerateCorruption )

#define FIsCorruptionError( err )                   ( ( (err) == JET_errDatabaseCorrupted ) ||              \
                                                        ( (err) == JET_errDatabaseCorruptedNoRepair ) ||    \
                                                        ( (err) == JET_errReadVerifyFailure ) ||            \
                                                        ( (err) == JET_errDiskReadVerificationFailure ) ||      \
                                                        ( (err) == JET_errReadLostFlushVerifyFailure ) ||       \
                                                        ( (err) == JET_errFileSystemCorruption ) ||     \
                                                        ( (err) == JET_errReadPgnoVerifyFailure ) )

#define FUTILOPTSTolerateCorruption( popts, err )   (   FUTILOPTSMaybeTolerateCorruption( (popts)->fUTILOPTSFlags ) &&  \
                                                        FUTILOPTSExplicitPageSet( (popts)->fUTILOPTSFlags ) &&          \
                                                        FIsCorruptionError( (err) ) )

#define FUTILOPTSDefragRepair( fFlags )             ( (fFlags) & fUTILOPTSDefragRepair )
#define UTILOPTSSetDefragRepair( fFlags )           ( (fFlags) |= fUTILOPTSDefragRepair )
#define UTILOPTSResetDefragRepair( fFlags )         ( (fFlags) &= ~fUTILOPTSDefragRepair )

#define FUTILOPTSPreserveTempDB( fFlags )           ( (fFlags) & fUTILOPTSPreserveTempDB )
#define UTILOPTSSetPreserveTempDB( fFlags )         ( (fFlags) |= fUTILOPTSPreserveTempDB )
#define UTILOPTSResetPreserveTempDB( fFlags )       ( (fFlags) &= ~fUTILOPTSPreserveTempDB )

#define FUTILOPTSDefragInfo( fFlags )               ( (fFlags) & fUTILOPTSDefragInfo )
#define UTILOPTSSetDefragInfo( fFlags )             ( (fFlags) |= fUTILOPTSDefragInfo )
#define UTILOPTSResetDefragInfo( fFlags )           ( (fFlags) &= ~fUTILOPTSDefragInfo )

#define FUTILOPTSIncrBackup( fFlags )               ( (fFlags) & fUTILOPTSIncrBackup )
#define UTILOPTSSetIncrBackup( fFlags )             ( (fFlags) |= fUTILOPTSIncrBackup )
#define UTILOPTSResetIncrBackup( fFlags )           ( (fFlags) &= ~fUTILOPTSIncrBackup )

#define FUTILOPTSVerbose( fFlags )                  ( (fFlags) & fUTILOPTSVerbose )
#define UTILOPTSSetVerbose( fFlags )                ( (fFlags) |= fUTILOPTSVerbose )
#define UTILOPTSResetVerbose( fFlags )              ( (fFlags) &= ~fUTILOPTSVerbose )

#define FUTILOPTSReportErrors( fFlags )             ( (fFlags) & fUTILOPTSReportErrors )
#define UTILOPTSSetReportErrors( fFlags )           ( (fFlags) |= fUTILOPTSReportErrors )
#define UTILOPTSResetReportErrors( fFlags )         ( (fFlags) &= ~fUTILOPTSReportErrors )

#define FUTILOPTSDontRepair( fFlags )               ( (fFlags) & fUTILOPTSDontRepair )
#define UTILOPTSSetDontRepair( fFlags )             ( (fFlags) |= fUTILOPTSDontRepair )
#define UTILOPTSResetDontRepair( fFlags )           ( (fFlags) &= ~fUTILOPTSDontRepair )

#define FUTILOPTSDumpStats( fFlags )                ( (fFlags) & fUTILOPTSDumpStats )
#define UTILOPTSSetDumpStats( fFlags )              ( (fFlags) |= fUTILOPTSDumpStats )
#define UTILOPTSResetDumpStats( fFlags )            ( (fFlags) &= ~fUTILOPTSDumpStats )

#define FUTILOPTSDontBuildIndexes( fFlags )         ( (fFlags) & fUTILOPTSDontBuildIndexes )
#define UTILOPTSSetDontBuildIndexes( fFlags )       ( (fFlags) |= fUTILOPTSDontBuildIndexes )
#define UTILOPTSResetDontBuildIndexes( fFlags )     ( (fFlags) &= ~fUTILOPTSDontBuildIndexes )

#define FUTILOPTSDebugMode( fFlags )                ( (fFlags) & fUTILOPTSDebugMode )
#define UTILOPTSSetDebugMode( fFlags )              ( (fFlags) |= fUTILOPTSDebugMode )
#define UTILOPTSResetDebugMode( fFlags )            ( (fFlags) &= ~fUTILOPTSDebugMode )

#define FUTILOPTSDumpRestoreEnv( fFlags )           ( (fFlags) & fUTILOPTSDumpRestoreEnv )
#define UTILOPTSSetDumpRestoreEnv( fFlags )         ( (fFlags) |= fUTILOPTSDumpRestoreEnv )
#define UTILOPTSResetDumpRestoreEnv( fFlags )       ( (fFlags) &= ~fUTILOPTSDumpRestoreEnv )

#define FUTILOPTSServerSim( fFlags )                ( (fFlags) & fUTILOPTSServerSim )
#define UTILOPTSSetServerSim( fFlags )              ( (fFlags) |= fUTILOPTSServerSim )
#define UTILOPTSResetServerSim( fFlags )            ( (fFlags) &= ~fUTILOPTSServerSim )

#define FUTILOPTSChecksumEDB( fFlags )              ( (fFlags) & fUTILOPTSChecksumEDB )
#define UTILOPTSSetChecksumEDB( fFlags )            ( (fFlags) |= fUTILOPTSChecksumEDB )
#define UTILOPTSResetChecksumEDB( fFlags )          ( (fFlags) &= ~fUTILOPTSChecksumEDB )

#define FUTILOPTSRepairCheckOnly( fFlags )          ( (fFlags) & fUTILOPTSRepairCheckOnly )
#define UTILOPTSSetRepairCheckOnly( fFlags )        ( (fFlags) |= fUTILOPTSRepairCheckOnly )
#define UTILOPTSResetRepairCheckOnly( fFlags )      ( (fFlags) &= ~fUTILOPTSRepairCheckOnly )

#define FUTILOPTSDelOutOfRangeLogs( fFlags )    ( (fFlags) & fUTILOPTSDelOutOfRangeLogs )
#define UTILOPTSSetDelOutOfRangeLogs( fFlags )  ( (fFlags) |= fUTILOPTSDelOutOfRangeLogs )
#define UTILOPTSResetDelOutOfRangeLogs( fFlags ) ( (fFlags) &= ~fUTILOPTSDelOutOfRangeLogs )

#define FUTILOPTSOperateTempVss( fFlags )           ( (fFlags) & fUTILOPTSOperateTempVss )
#define UTILOPTSSetOperateTempVss( fFlags )         ( (fFlags) |= fUTILOPTSOperateTempVss )
#define UTILOPTSResetOperateTempVss( fFlags )       ( (fFlags) &= ~fUTILOPTSOperateTempVss )

#define FUTILOPTSOperateTempVssRec( fFlags )            ( (fFlags) & fUTILOPTSOperateTempVssRec )
#define UTILOPTSSetOperateTempVssRec( fFlags )          ( (fFlags) |= fUTILOPTSOperateTempVssRec )
#define UTILOPTSResetOperateTempVssRec( fFlags )        ( (fFlags) &= fUTILOPTSOperateTempVssRec )

#define FUTILOPTSOperateTempVssPause( fFlags )      ( (fFlags) & fUTILOPTSOperateTempVssPause )
#define UTILOPTSSetOperateTempVssPause( fFlags )        ( (fFlags) |= fUTILOPTSOperateTempVssPause )
#define UTILOPTSResetOperateTempVssPause( fFlags )  ( (fFlags) &= fUTILOPTSOperateTempVssPause )

#define FUTILOPTSRecoveryWithoutUndoForReal( fFlags )       ( (fFlags) & fUTILOPTSRecoveryWithoutUndoForReal )
#define UTILOPTSSetRecoveryWithoutUndoForReal( fFlags )     ( (fFlags) |= fUTILOPTSRecoveryWithoutUndoForReal )
#define UTILOPTSResetRecoveryWithoutUndoForReal( fFlags )   ( (fFlags) &= fUTILOPTSRecoveryWithoutUndoForReal )

inline BOOL FUTILOPTSTrimDatabase( _In_ const UTILOPTS* const putilopts )       { return !!putilopts->fUTILOPTSTrimDatabase; }
inline VOID UTILOPTSSetTrimDatabase( _In_ UTILOPTS* const putilopts )           { putilopts->fUTILOPTSTrimDatabase = fTrue; }
inline VOID UTILOPTSResetTrimDatabase( _In_ UTILOPTS* const putilopts )         { putilopts->fUTILOPTSTrimDatabase = fFalse; }

#define PUBLIC    extern
#define INLINE    inline

//
//  Minimalistic assert() functionality.
//
#ifdef DEBUG
#define assert( expr )              if ( !(expr) )                      \
                                    {                                   \
                                        wprintf(L"EseutilAssert(%S:%d): %S\n", __FILE__, __LINE__, #expr ); \
                                        DebugBreak();                   \
                                    }
#define assertSz( cond, sz )            if( !(cond) ) assert( cond && sz )
#else
#define assert( expr )
#define assertSz( cond, sz )
#endif

