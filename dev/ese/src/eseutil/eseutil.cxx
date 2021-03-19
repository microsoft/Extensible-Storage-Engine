// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// the sever simulation functionality will be DEBUG only
#if defined(DEBUG) && !defined(ESENT)
#define RESTORE_SERVER_SIMULATION
#define RESTORE_SERVER_SIMULATION_HELP
#endif

#ifdef RESTORE_SERVER_SIMULATION
#include <eseback2.h>
#endif // RESTORE_SERVER_SIMULATION

#include "_edbutil.hxx"
#include "_dbspacedump.hxx"
#include "stat.hxx"

#include "esefile.hxx"
#undef UNICODE              //  esefile.hxx enables UNICODE

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)


#ifndef ESENT
#include <esebcli2.h>
#define ESEBCLI2_DLL_NAME   L"ESEBCLI2.DLL"

#ifdef USE_WATSON_API
#include <NativeWatson.h>
#endif

#define ESEBACK2_DLL_NAME   L"ESEBACK2.DLL"

#endif // ESENT

#ifdef ESESHADOW

#include "eseshadow.h"

//  Required for indirection (ESESHADOW_LOCAL_DEFERRED_DLL).
//
ESESHADOW_LOCAL_DEFERRED_DLL_STATE
#endif

//  In fake recovery without undo, we set the grbit for JET_bitRecoveryWithoutUndo, but 
//  then the our callback control decides to do undo after all, unless the "/u" option 
//  was passed.
#define ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO 1


LOCAL const WCHAR   * const wszUser         = L"user";
LOCAL const WCHAR   * const wszPassword     = L"";


// UNDONE:  Must still be localised, but centralising all the localisable strings
// here makes my job easier.

LOCAL const WCHAR   * const wszCurrDir                  = L".";

// Specify .\ to guarantee it's the local directory. ErrInit will stick bare path names in
// the System directory, which will fail when using a /vss snap.
LOCAL const WCHAR   * const wszDefaultTempDBFormat      = L"TEMP%d.EDB";
LOCAL const WCHAR   * const wszDefaultDefragDBFormat    = L".\\TEMPDFRG%d.EDB";
LOCAL const WCHAR   * const wszDefaultUpgradeDBFormat   = L"TEMPUPGD%d.EDB";
LOCAL const WCHAR   * const wszDefaultRepairDBFormat    = L"TEMPREPAIR%d.EDB";
LOCAL const WCHAR   * const wszDefaultIntegDBFormat     = L".\\TEMPINTEG%d.EDB";
LOCAL const WCHAR   * const wszDefaultScrubDBFormat     = L"TEMPSCRUB%d.EDB";
LOCAL const WCHAR   * const wszDefaultChecksumDBFormat  = L"TEMPCHKSUM%d.EDB";

#ifdef ESENT
const ULONG cbPageDefault = 4096;
#else
const ULONG cbPageDefault = 32768;
#endif
const ULONG ckbPageDefault = cbPageDefault / 1024;

LOCAL WCHAR wszDefaultTempDB[64];
LOCAL WCHAR wszDefaultDefragDB[64];
LOCAL WCHAR wszDefaultUpgradeDB[64];
LOCAL WCHAR wszDefaultRepairDB[64];
LOCAL WCHAR wszDefaultIntegDB[64];
LOCAL WCHAR wszDefaultScrubDB[64];
LOCAL WCHAR wszDefaultChecksumDB[64];

LOCAL const WCHAR   * const wszDefrag               = L"Defragmentation";
LOCAL const WCHAR   * const wszUpgrade              = L"Upgrade";
LOCAL const WCHAR   * const wszRestore              = L"Restore";
LOCAL const WCHAR   * const wszRepair               = L"Scanning";
LOCAL const WCHAR   * const wszScrub                = L"Securing";
LOCAL const WCHAR   * const wszUpgradeRecordFormat  = L"Upgrading Record Format";
LOCAL const WCHAR   * const wszIntegrity            = L"Integrity";
LOCAL const WCHAR   * const wszChecksum             = L"Checksum";
LOCAL const WCHAR   * const wszSpaceCat             = L"Space Categorization";

LOCAL const WCHAR   * const wszStatusMsg        = L" Status (% complete)";

LOCAL const WCHAR   * const wszMoveFile     = L"\nMoving '%s' to '%s'...";
LOCAL const WCHAR   * const wszMoveDone     = L" DONE!";
LOCAL const WCHAR   * const wszMoveFailed   = L" FAILED!";
LOCAL const WCHAR   * const wszCopyFile     = L" Copying...";
LOCAL const WCHAR   * const wszCopyFileStatus= L"File Copy";

LOCAL const WCHAR   wchNewLine              = L'\n';

LOCAL const WCHAR   * const wszSwitches     = L"-/";

LOCAL const WCHAR   * wszConfigArgPrefix    = L"config";

LOCAL const WCHAR   * const wszRegistryMsg  = L"Using Registry environment...";

LOCAL const WCHAR   * const wszErr1         = L"Error: Source database specification '%s' is invalid.";
LOCAL const WCHAR   * const wszErr2         = L"Error: Temporary database specification '%s' is invalid.";
LOCAL const WCHAR   * const wszErr3         = L"Error: Source database '%s' cannot be the same as the temporary database.";
LOCAL const WCHAR   * const wszErr4         = L"Error: Could not backup to '%s'.";
LOCAL const WCHAR   * const wszErr5         = L"Error: Could not re-instate '%s'. It may be manually re-instated by manually copying '%s' to '%s' (this will overwrite the original copy of the file with the defragmented copy).";
LOCAL const WCHAR   * const wszErr6         = L"Error: Failed loading Registry Environment.";

LOCAL const WCHAR   * const wszErr7         = L"Error: Failed to load DLL %s.\r\n";
LOCAL const WCHAR   * const wszErr8         = L"Error: Failed to load function %hs from DLL.\r\n";

LOCAL const WCHAR   * const wszUsageErr1    = L"Usage Error: Missing %s specification.";
LOCAL const WCHAR   * const wszUsageErr2    = L"Usage Error: Duplicate %s specification.";
LOCAL const WCHAR   * const wszUsageErr3    = L"Usage Error: Only one type of recovery allowed.";
LOCAL const WCHAR   * const wszUsageErr4    = L"Usage Error: Invalid option '%s'.";
LOCAL const WCHAR   * const wszUsageErr5    = L"Usage Error: Invalid argument '%s'. Options must be preceded by '-' or '/'.";
LOCAL const WCHAR   * const wszUsageErr6    = L"Usage Error: Invalid buffer cache size.";
LOCAL const WCHAR   * const wszUsageErr7    = L"Usage Error: Invalid batch I/O size.";
LOCAL const WCHAR   * const wszUsageErr8    = L"Usage Error: Invalid database extension size.";
LOCAL const WCHAR   * const wszUsageErr9    = L"Usage Error: No mode specified.";
LOCAL const WCHAR   * const wszUsageErr10   = L"Usage Error: Mode selection must be preceded by '-' or '/'.";
LOCAL const WCHAR   * const wszUsageErr11   = L"Usage Error: Old .DLL required in order to upgrade.";
LOCAL const WCHAR   * const wszUsageErr12   = L"Usage Error: Invalid mode.";
LOCAL const WCHAR   * const wszUsageErr13   = L"Usage Error: Invalid node/lgpos specification.";
LOCAL const WCHAR   * const wszUsageErr14   = L"Usage Error: Invalid logfile size: %s kbytes.";
LOCAL const WCHAR   * const wszUsageErr15   = L"Usage Error: Unable to allocate enough space for database names: %s kbytes.";
LOCAL const WCHAR   * const wszUsageErr16   = L"Usage Error: Invalid stop recovery option.";
LOCAL const WCHAR   * const wszUsageErr17   = L"Usage Error: Invalid default restore location option.";

LOCAL const WCHAR   * const wszUsageErr19   = L"Usage Error: Invalid maximum cache size specification.";

LOCAL const WCHAR   * const wszUsageErr20   = L"Usage Error: Invalid api version specification.";
LOCAL const WCHAR   * const wszUsageErr21   = L"Usage Error: Config store spec not accepted, generally takes form of /config reg:HKLM\\<RegPathToUse>.";

LOCAL const WCHAR   * const wszUsageErr22   = L"Usage Error: Invalid log generation range specification.";

#ifdef ESENT
LOCAL const WCHAR   * const wszHelpDesc1        = L"DESCRIPTION:  Database utilities for the Extensible Storage Engine for Microsoft(R) Windows(R).";
#else  //  !ESENT
LOCAL const WCHAR   * const wszHelpDesc1        = L"DESCRIPTION:  Database utilities for the Extensible Storage Engine for Microsoft(R) Exchange Server.";
#endif  //  ESENT
LOCAL const WCHAR   * const wszHelpSyntax   = L"MODES OF OPERATION:";
LOCAL const WCHAR   * const wszHelpModes1   = L"      Defragmentation:  %s /d <database name> [options]";
LOCAL const WCHAR   * const wszHelpModes2   = L"             Recovery:  %s /r <logfile base name> [options]";
LOCAL const WCHAR   * const wszHelpModes3   = L"            Integrity:  %s /g <database name> [options]";
LOCAL const WCHAR   * const wszHelpModes4   = L"             Checksum:  %s /k <file name> [options]";
LOCAL const WCHAR   * const wszHelpModes5   = L"               Repair:  %s /p <database name> [options]";
LOCAL const WCHAR   * const wszHelpModes6   = L"            File Dump:  %s /m[mode-modifier] <filename>";
LOCAL const WCHAR   * const wszHelpModes7   = L"            Copy File:  %s /y <source file> [options]";
LOCAL const WCHAR   * const wszHelpModes8   = L"              Restore:  %s /c[mode-modifier] <path name> [options]";
LOCAL const WCHAR   * const wszHelpModes9   = L"               Backup:  %s /b <backup path> [options]";
                        //  wszHelpModes10 was Record Format Upgrade. (/f)
                        //  wszHelpModes11 was Upgrade. (/u)
LOCAL const WCHAR   * const wszHelpModes12  = L"               Secure:  %s /z <database name> [options]";

LOCAL const WCHAR   * const wszHelpPrompt1  = L"<<<<<  Press a key for more help  >>>>>";
LOCAL const WCHAR   * const wszHelpPrompt2  = L"D=Defragmentation, R=Recovery, G=inteGrity, K=checKsum,";

#ifdef ESENT
LOCAL const WCHAR   * const wszHelpPrompt3  = L"P=rePair, M=file duMp, Y=copY file";
#else //ESENT
LOCAL const WCHAR   * const wszHelpPrompt3  = L"P=rePair, M=file duMp, Y=copY file, C=restore";
#endif // ESENT

#ifdef DEBUG
LOCAL const WCHAR   * const wszHelpPrompt4  = L"B=Backup, F=record Format upgrade, U=upgrade, Z=secure";
#endif
LOCAL const WCHAR   * const wszHelpPromptCursor = L"=> ";

LOCAL const WCHAR * const   wszOperSuccess          = L"Operation completed successfully in %d.%d seconds.";
LOCAL const WCHAR * const   wszOperWarn             = L"Operation completed successfully with %d (%s) after %d.%d seconds.";
LOCAL const WCHAR * const   wszOperFailWithError        = L"Operation terminated with error %d (%s) after %d.%d seconds.";
LOCAL const WCHAR * const   wszOperFailUnknownError = L"Operation terminated unsuccessfully after %d.%d seconds.";


const JET_LGPOS lgposMax = { 0xffff, 0xffff, 0x7fffffff };

//  ================================================================
JET_ERR ErrGetProcAddress(
            const HMODULE hmod,
            const CHAR * const szFunc,
            VOID ** const pvPfn )
//  ================================================================
{
    if( NULL == ( (*pvPfn) = GetProcAddress( hmod, szFunc ) ) )
    {
        wprintf( wszErr8, szFunc );
        return ErrERRCheck( JET_errCallbackNotResolved );
    }
    return JET_errSuccess;
}

LOCAL WCHAR *GetNextArg();
LOCAL_BROKEN WCHAR  *GetPrevArg();
LOCAL VOID  SetCurArgID( const INT id );
LOCAL INT   GetArgCount();
LOCAL INT   GetCurArgID();
LOCAL WCHAR *GetCurArg();


LOCAL VOID EDBUTLPrintLogo( void )
{
    WCHAR   wszVersion[16];

#ifdef ESENT
    StringCbPrintfW( wszVersion, sizeof(wszVersion), L"%d.%d", VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION );
    wprintf( L"Extensible Storage Engine Utilities for Microsoft(R) Windows(R)%c", wchNewLine );
#else  //  !ESENT
    StringCbPrintfW( wszVersion, sizeof(wszVersion), L"%hs.%hs", PRODUCT_MAJOR, PRODUCT_MINOR );
    wprintf( L"Extensible Storage Engine Utilities for Microsoft(R) Exchange Server%c", wchNewLine );
#endif  //  ESENT
    wprintf( L"Version %s%c", wszVersion, wchNewLine );
    wprintf( L"Copyright (c) Microsoft Corporation.\nLicensed under the MIT License.%c", wchNewLine );
    wprintf( L"%c", wchNewLine );
}

LOCAL VOID EDBUTLHelpDefrag( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"DEFRAGMENTATION/COMPACTION:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Performs off-line compaction of a database.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /d <database name> [options]%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  <database name> - filename of database to compact%c", wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /t<db>     - set temp. database name (default: TEMPDFRG*.EDB)%c", wchNewLine );
    wprintf( L"                  /p         - preserve temporary database (ie. don't instate)%c", wchNewLine );
    wprintf( L"                  /b<db>     - make backup copy under the specified name%c", wchNewLine );
#ifdef DEBUG    // Undocumented switches.
    wprintf( L"                  /n         - dump defragmentation information to DFRGINFO.TXT%c", wchNewLine );
    wprintf( L"                  /x<#>      - database extension size, in pages (default: 256)%c", wchNewLine );
    wprintf( L"                  /w         - set batch IO size%c", wchNewLine );
    wprintf( L"                  /c<nnnnn>  - set the maximum cache size (in page)%c", wchNewLine );
#endif
    wprintf( L"                  /2         - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4         - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8         - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16        - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32        - set 32k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /u<#>      - set Engine Format Version parameter%c", wchNewLine );
    wprintf( L"                  /v         - verbose output%c", wchNewLine );
    wprintf( L"                  /o         - suppress logo%c", wchNewLine );
    wprintf( L"          NOTES:  1) If instating is disabled (ie. /p), the original database%c", wchNewLine );
    wprintf( L"                     is preserved uncompacted, and the temporary database will%c", wchNewLine );
    wprintf( L"                     contain the defragmented version of the database.%c", wchNewLine );
}

LOCAL VOID EDBUTLHelpRecovery( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"RECOVERY:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Performs recovery, bringing all databases to a%c", wchNewLine );
    wprintf( L"                  clean-shutdown state.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /r <3-character logfile base name> [options]%c", wszAppName, wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /l<path>          - location of log files.%c", wchNewLine );
    wprintf( L"                                      (default: current directory)%c", wchNewLine );
    wprintf( L"                  /s<path>          - location of system files (eg. checkpoint file).%c", wchNewLine );
    wprintf( L"                                      (default: current directory).%c", wchNewLine );
    wprintf( L"                  /i                - ignore mismatched/missing database attachments.%c", wchNewLine );
    wprintf( L"                  /t                - on successful recovery, truncate log files.%c", wchNewLine );
    wprintf( L"                  /k[f] <database>  - on successful recovery, shrink the database. /kf enables%c", wchNewLine );
    wprintf( L"                                      full space categorization on each page, which may substantially%c", wchNewLine );
    wprintf( L"                                      increase the duration of the shrink operation for databases with%c", wchNewLine );
    wprintf( L"                                      a large number of tables/indexes, but may improve performance on%c", wchNewLine );
    wprintf( L"                                      the resulting database. Cannot be combined with /p.%c", wchNewLine );
    wprintf( L"                  /p    <database>  - on successful recovery, run space leakage reclaimer. Cannot be%c", wchNewLine );
    wprintf( L"                                      combined with /k.%c", wchNewLine );
#if defined( ESENT ) || defined( DEBUG )
    //  Note: Available in retail, but not documented.
    wprintf( L"                  /u[log]           - stop recovery when the Undo phase is reached with the option%c", wchNewLine );
    wprintf( L"                                      to stop when a certain log generation is recovered.%c", wchNewLine );
    wprintf( L"                                      [log] is the log generation number and if not specified %c", wchNewLine );
    wprintf( L"                                      the replay will go to the end of the existing logs.%c", wchNewLine );
#ifdef DEBUG
    wprintf( L"                                      [log] can be a log position in the N:N:N form in which case %c", wchNewLine );
    wprintf( L"                                      recovery will stop after the specified position %c", wchNewLine );
#endif  //  DEBUG
#endif  //  ESENT || DEBUG
    wprintf( L"                  /d[path]          - location of database files, or current directory%c", wchNewLine );
    wprintf( L"                                      if [path] not specified (default: directory%c", wchNewLine );
    wprintf( L"                                      originally logged in log files)%c", wchNewLine );
    wprintf( L"                  /n<path1[:path2]> - new location of database file and optional old location%c", wchNewLine );
    wprintf( L"                                      if the database file location changed.%c", wchNewLine );
    wprintf( L"                                      Can be specified for each database file.%c", wchNewLine );
    wprintf( L"                                      If a certain database is not in the list, it won't get recovered.%c", wchNewLine );
    wprintf( L"                                      To allow recovery in the original location for all other database, use /n*.%c", wchNewLine );
    wprintf( L"                                      (not valid with /d switch, not valid with /b switch)%c", wchNewLine );
#ifdef DEBUG
    wprintf( L"                  /b<path>          - restore from backup (ie. hard recovery) from the%c", wchNewLine );
    wprintf( L"                                      specified location.%c", wchNewLine );
    wprintf( L"                  /r<path>          - restore to specified location (only valid when%c", wchNewLine );
    wprintf( L"                                      the /b switch is also specified).%c", wchNewLine );
#endif
    wprintf( L"                  /a                - allow recovery to lose committed data if database%c", wchNewLine );
    wprintf( L"                                      integrity can still be maintained.%c", wchNewLine );
    wprintf( L"                  /2                - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4                - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8                - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16               - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32               - set 32k database page size (default: auto-detect)%c", wchNewLine );
#ifdef DEBUG
    wprintf( L"                  /e                - Do not specify JET_paramLogStreamMustExist.%c", wchNewLine );
    wprintf( L"                  /c(5|8)           - Force using 5 or 8 digit log sequence number on the log stream.%c", wchNewLine );
#endif
    wprintf( L"                  /o                - suppress logo.%c", wchNewLine );
#ifdef DEBUG
    wprintf( L"          NOTES:  1) Soft recovery is always performed unless the /b switch is%c", wchNewLine );
    wprintf( L"                     specified, in which case hard recovery is performed.%c", wchNewLine );
#endif
}

LOCAL VOID EDBUTLHelpIntegrity( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"INTEGRITY:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Verifies integrity of a database.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /g <database name> [options]%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  <database name> - filename of database to verify%c", wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /t<db>    - set temp. database name (default: TEMPINTEG*.EDB)%c", wchNewLine );
    wprintf( L"                  /f<name>  - set prefix to use for name of report files%c", wchNewLine );
    wprintf( L"                              (default: <database>.integ.raw)%c", wchNewLine );
#ifdef DEBUG    // Undocumented switches.
    wprintf( L"                  /b        - don't rebuild and compare indexes%c", wchNewLine );
    wprintf( L"                  /n        - dump table statistics to INTGINFO.TXT%c", wchNewLine );
#endif  //  DEBUG
#ifdef ESESHADOW
    wprintf( L"                  /vssrec <basename> <logpath>%c", wchNewLine );
    wprintf( L"                            - verifies a snapshot of a live database. This%c", wchNewLine );
    wprintf( L"                              option must be used in conjunction with /t<db>%c", wchNewLine );
    wprintf( L"                              so that the path to the temporary database is%c", wchNewLine );
    wprintf( L"                              redirected to a path with read/write permissions.%c", wchNewLine );
    wprintf( L"                  /vsssystempath <systempath>%c", wchNewLine );
    wprintf( L"                            - location of system files (eg. checkpoint file)%c", wchNewLine );
    wprintf( L"                              (default: log file path)%c", wchNewLine );
#endif
    wprintf( L"                  /2        - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4        - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8        - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16       - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32       - set 32k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /o        - suppress logo%c", wchNewLine );
    wprintf( L"          NOTES:  1) Integrity-check does not run database recovery. If a%c", wchNewLine );
    wprintf( L"                     database is in a \"Dirty Shutdown\" state it is strongly%c", wchNewLine );
    wprintf( L"                     recommended that before proceeding with an integrity-%c", wchNewLine );
    wprintf( L"                     check, recovery is first run to properly complete%c", wchNewLine );
    wprintf( L"                     database operations for the previous shutdown.%c", wchNewLine );
}

LOCAL VOID EDBUTLHelpChecksum( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"CHECKSUM:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Verifies the checksums of a database,%c", wchNewLine );
    wprintf( L"                  checkpoint file, or log file (or set of log files).%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /k <file name> [options]%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  <file name> - file name to verify%c", wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /t<db>   - set temp. database name (default: TEMPCHKSUM*.EDB)%c", wchNewLine );
    wprintf( L"                  /e       - don't checksum database file%c", wchNewLine );
    wprintf( L"                  /2       - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4       - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8       - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16      - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32      - set 32k database page size (default: auto-detect)%c", wchNewLine );
#ifdef ESESHADOW
    wprintf( L"                  /vss     - checksums a snapshot of a live database, does not%c", wchNewLine );
    wprintf( L"                             replay logs.%c", wchNewLine );
    wprintf( L"                  /vssrec <basename> <logpath>%c", wchNewLine );
    wprintf( L"                           - checksums a snapshot of a live database, replays%c", wchNewLine );
    wprintf( L"                             logs.%c", wchNewLine );
    wprintf( L"                  /vsssystempath <systempath>%c", wchNewLine );
    wprintf( L"                           - location of system files (eg. checkpoint file)%c", wchNewLine );
    wprintf( L"                             (default: log file path)%c", wchNewLine );
#endif
    wprintf( L"                  /o       - suppress logo%c", wchNewLine );
    wprintf( L"          NOTES:  1) This operation does not run database recovery.%c", wchNewLine );
    wprintf( L"                  2) If the file is not a database file, the options are%c", wchNewLine );
    wprintf( L"                     ignored.%c", wchNewLine );
    wprintf( L"                  3) The pause (/p) option is provided as a throttling%c", wchNewLine );
    wprintf( L"                     mechanism. It only applies when verifying checksums%c", wchNewLine );
    wprintf( L"                     of a database file.%c", wchNewLine );
}


LOCAL VOID EDBUTLHelpCopyFile( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"COPY FILE:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Copies a database or log file.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /y <source file> [options]%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  <source file> - name of file to copy%c", wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /d<file> - destination file (default: copy source file to%c", wchNewLine );
    wprintf( L"                             current directory)%c", wchNewLine );
    wprintf( L"                  /i       - ignore IO read errors%c", wchNewLine );
    wprintf( L"                  /o       - suppress logo%c", wchNewLine );
#ifdef ESESHADOW
    wprintf( L"                  /vss     - copies a snapshot of the file, does not replay%c", wchNewLine );
    wprintf( L"                             logs.%c", wchNewLine );
    wprintf( L"                  /vssrec <basename> <logpath>%c", wchNewLine );
    wprintf( L"                           - copies a snapshot of a live database, replays%c", wchNewLine );
    wprintf( L"                             logs.%c", wchNewLine );
    wprintf( L"                  /vsssystempath <systempath>%c", wchNewLine );
    wprintf( L"                           - location of system files (eg. checkpoint file)%c", wchNewLine );
    wprintf( L"                             (default: log file path)%c", wchNewLine );
#endif
}

LOCAL VOID EDBUTLHelpRepair( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"REPAIR:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Repairs a corrupted or damaged database.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /p <database name> [options]%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  <database name> - filename of database to repair%c", wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /t<db>       - set temp. database name%c", wchNewLine );
    wprintf( L"                                 (default: TEMPREPAIR*.EDB)%c", wchNewLine );
    wprintf( L"                  /f<name>     - set prefix to use for name of report files%c", wchNewLine );
    wprintf( L"                                 (default: <database>.integ.raw)%c", wchNewLine );
#ifdef DEBUG    // Undocumented switches.
    wprintf( L"                  /n           - dump table statistics to INTGINFO.TXT%c", wchNewLine );
#endif  //  DEBUG
    wprintf( L"                  /g           - run integrity check before repairing%c", wchNewLine );
    wprintf( L"                  /2           - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4           - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8           - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16          - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32          - set 32k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /u<#>        - set Engine Format Version parameter%c", wchNewLine );
    wprintf( L"                  /o           - suppress logo%c", wchNewLine );
    wprintf( L"          NOTES:  1) Repair does not run database recovery. If a database%c", wchNewLine );
    wprintf( L"                     is in a \"Dirty Shutdown\" state it is strongly%c", wchNewLine );
    wprintf( L"                     recommended that before proceeding with repair,%c", wchNewLine );
    wprintf( L"                     recovery is first run to properly complete database%c", wchNewLine );
    wprintf( L"                     operations for the previous shutdown.%c", wchNewLine );
    wprintf( L"                  2) The /g option pauses the utility for user input before%c", wchNewLine );
    wprintf( L"                     repair is performed if corruption is detected. This option%c", wchNewLine );
    wprintf( L"                     overrides the /o option.%c", wchNewLine );
}

LOCAL VOID EDBUTLHelpDump( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"FILE DUMP:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Generates formatted output of various database file types.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /m[mode-modifier] <filename> [options]%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  [mode-modifier] - an optional letter designating the type of%c", wchNewLine );
    wprintf( L"                                    file dump to perform. Valid values are:%c", wchNewLine );
    wprintf( L"                                    h - dump database header (default)%c", wchNewLine );
    wprintf( L"                                    k - dump checkpoint file%c", wchNewLine );
    wprintf( L"                                    l - dump log file or set of logs%c", wchNewLine );
    wprintf( L"                                    p - dump flush map file%c", wchNewLine );
#ifdef DEBUG
    wprintf( L"                                    f - force database to a clean-shutdown state%c", wchNewLine );
#endif
    wprintf( L"                                    m - dump meta-data%c", wchNewLine );
    wprintf( L"                                    s - dump space usage%c", wchNewLine );
    wprintf( L"                                    c - dump space category for a page or page range, which can%c", wchNewLine );
    wprintf( L"                                        be specified by the /p option. If /p is not specified,%c", wchNewLine );
    wprintf( L"                                        all pages in the database will be analyzed.%c", wchNewLine );
    wprintf( L"                                    t - for FTL trace file.%c", wchNewLine );
    wprintf( L"                                    b - dump block cache file.%c", wchNewLine );
    wprintf( L"                                    r - dump pages of rollback snapshot, which can be specified%c", wchNewLine );
    wprintf( L"                                        by the /p option. If /p is not specified, header of the rollback %c", wchNewLine );
    wprintf( L"                                        snapshot will be dumped. If /p* is specified all pages in the snapshot %c", wchNewLine );
    wprintf( L"                                        will be dumped. To specify a page range, use /p<pgnoFirst>-<pgnoLast>, %c", wchNewLine );
    wprintf( L"                                        where a pgnoLast of 'max' indicates the last page. %c", wchNewLine );
    wprintf( L"                                    n - dump nodes%c", wchNewLine );
    wprintf( L"                  <filename>      - name of file to dump. The type of the%c", wchNewLine );
    wprintf( L"                                    specified file should match the dump type%c", wchNewLine );
    wprintf( L"                                    being requested (eg. if using /mh, then%c", wchNewLine );
    wprintf( L"                                    <filename> must be the name of a database).%c", wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
#if 0
    wprintf( L"                  /k<node>@<lgpos>%c", wchNewLine );
    wprintf( L"                             - track a specific node in the logfile%c", wchNewLine );
    wprintf( L"                               starting from the given lgpos%c", wchNewLine );
#endif
    wprintf( L"                  /p<[pgno]|[<pgnoFirst>:<pgnoLast>]> [/k<key> [/d<data>]]%c", wchNewLine );
    wprintf( L"                             - dump the specified page from the database, or if%c", wchNewLine );
    wprintf( L"                               an optional bookmark (key plus optional data) is%c", wchNewLine );
    wprintf( L"                               specified, seek to the bookmark starting at the%c", wchNewLine );
    wprintf( L"                               specified page, then dump the leaf page where%c", wchNewLine );
    wprintf( L"                               the seek ended up.%c", wchNewLine );
    wprintf( L"                               If using space categoriztion (/mc), it can take%c", wchNewLine );
    wprintf( L"                               a range in the format /p<pgnoFirst>:<pgnoLast>,%c", wchNewLine );
    wprintf( L"                               where a pgnoLast of 'max' indicates the last page%c", wchNewLine );
    wprintf( L"                               of the database.%c", wchNewLine );
    wprintf( L"                  /n<node>   - dump the specified node from the database%c", wchNewLine );
    wprintf( L"                  /t<table>  - perform dump for specified table only%c", wchNewLine );
    wprintf( L"                  /a         - dump all nodes including deleted ones%c", wchNewLine );
    wprintf( L"                               (dump-nodes mode only)%c", wchNewLine );
#ifdef ESESHADOW
    wprintf( L"                  /vss       - dumps a snapshot of the file, does not replay%c", wchNewLine );
    wprintf( L"                               logs.%c", wchNewLine );
    wprintf( L"                  /vssrec <basename> <logpath>%c", wchNewLine );
    wprintf( L"                             - dumps a snapshot of a live database, replays%c", wchNewLine );
    wprintf( L"                               logs.%c", wchNewLine );
    wprintf( L"                  /vsssystempath <systempath>%c", wchNewLine );
    wprintf( L"                             - location of system files (eg. checkpoint file)%c", wchNewLine );
    wprintf( L"                               (default: log file path)%c", wchNewLine );
    wprintf( L"                  /vsspause  - pause after the snapshot completes.%c", wchNewLine );
#endif
    wprintf( L"                  /v[level]  - verbose (some file types do not offer different verbosity levels)%c", wchNewLine );
    wprintf( L"                  /2         - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4         - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8         - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16        - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32        - set 32k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /o         - suppress logo%c", wchNewLine );
    wprintf( L"                  /c<file>   - dump the log file, in CSV format, to <file>%c", wchNewLine );
    wprintf( L"                  /x         - for dump of a single log file only, permits%c", wchNewLine );
    wprintf( L"                               fixup of torn writes at the end of the log file%c", wchNewLine );
    wprintf( L"                               if necessary (NOTE: requires read/write access%c", wchNewLine );
    wprintf( L"                               to the log file)%c", wchNewLine );
    wprintf( L"                  /r<start gen>[-<end gen>]%c", wchNewLine );
    wprintf( L"                             - for dump of a set of log files only, specifies%c", wchNewLine );
    wprintf( L"                               the starting log generation (as a hex number) and%c", wchNewLine );
    wprintf( L"                               optional ending log generation (also as a hex%c", wchNewLine );
    wprintf( L"                               number)%c", wchNewLine );

    //  Prints the space usage options ...
    PrintSpaceDumpHelp( 80, wchNewLine, L"                  ", L"           " );

    //  Space categorization options.
    wprintf( L"%c", wchNewLine );
    wprintf( L"%*cSPACE CATEGORIZATION OPTIONS:%c", 18, L' ', wchNewLine );
    wprintf( L"%*c     /f            - Runs full space categorization. This may be expensive, but%c", 29, L' ', wchNewLine );
    wprintf( L"%*c                     required to categorize some pages.%c", 29, L' ', wchNewLine );
}

#ifndef ESENT
LOCAL VOID EDBUTLHelpHardRecovery( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"RESTORE:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Restore information and completion.%c", wchNewLine );
#ifdef RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"         SYNTAX:  %s /c[mode-modifier] <path name|file name> [options]%c", wszAppName, wchNewLine );
#else // RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"         SYNTAX:  %s /c[mode-modifier] <path name> [options]%c", wszAppName, wchNewLine );
#endif // RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"     PARAMETERS:  [mode-modifier] - a letter designating the type of operation%c", wchNewLine );
    wprintf( L"                                    to be done%c", wchNewLine );
    wprintf( L"                                    m - dump Restore.Env %c", wchNewLine );
    wprintf( L"                                    c - start recovery for a Restore.Env%c", wchNewLine );
#ifdef RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"                                    s - simulate server based on description%c", wchNewLine );
    wprintf( L"                  <path name>     - (/cm and /cc) directory of the restore%c", wchNewLine );
    wprintf( L"                                    (Restore.Env location)%c", wchNewLine );
    wprintf( L"                                  OR%c", wchNewLine );
    wprintf( L"                  <file name>     - (/cs) name of the server description file%c", wchNewLine );
#else // RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"                  <path name>     - directory of the restore%c", wchNewLine );
    wprintf( L"                                    (Restore.Env location)%c", wchNewLine );
#endif // RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /t[instance]    - name of the instance containing the log%c", wchNewLine );
    wprintf( L"                                    files to play forward, or if [instance] is%c", wchNewLine );
    wprintf( L"                                    not specified, don't play forward any log%c", wchNewLine );
    wprintf( L"                                    files unless they are in the restore%c", wchNewLine );
    wprintf( L"                                    directory (default: use instance specified%c", wchNewLine );
    wprintf( L"                                    by Restore.Env)%c", wchNewLine );
    wprintf( L"                  /f<path name>   - directory containing the log files to play%c", wchNewLine );
    wprintf( L"                                    forward (note: doesn't work with /t)%c", wchNewLine );
    wprintf( L"                  /k              - preserves the log files used for recovery%c", wchNewLine );
    wprintf( L"                  /2              - set 2k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /4              - set 4k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /8              - set 8k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /16             - set 16k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /32             - set 32k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /o              - suppress logo%c", wchNewLine );
}
#endif // ESENT

#ifdef DEBUG

LOCAL VOID EDBUTLHelpBackup( _In_ PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"BACKUP:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Performs backup, bringing all databases to a%c", wchNewLine );
    wprintf( L"                  clean-shutdown state.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /b <backup path> [options]%c", wszAppName, wchNewLine );
    wprintf( L"        OPTIONS:  zero or more of the following switches, separated by a space:%c", wchNewLine );
    wprintf( L"                  /l<path>   - location of log files%c", wchNewLine );
    wprintf( L"                               (default: current directory)%c", wchNewLine );
    wprintf( L"                  /s<path>   - location of system files (eg. checkpoint file)%c", wchNewLine );
    wprintf( L"                               (default: current directory)%c", wchNewLine );
    wprintf( L"                  /c<path>   - incremental backup%c", wchNewLine );
    wprintf( L"                  /2         - set 2k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /4         - set 4k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /8         - set 8k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /16        - set 16k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /32        - set 32k database page size (default: %dk)%c", ckbPageDefault, wchNewLine );
    wprintf( L"                  /o         - suppress logo%c", wchNewLine );
}

#endif // DEBUG

LOCAL VOID EDBUTLHelpScrub( _In_ PCWSTR const wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"SECURE:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Removes all deleted records from database.%c", wchNewLine );
    wprintf( L"         SYNTAX:  %s /z <database name>%c", wszAppName, wchNewLine );
    wprintf( L"     PARAMETERS:  <database name> - filename of database to secure%c", wchNewLine );
    wprintf( L"                  /t   - trim unused database pages, using sparse files%c", wchNewLine );
    wprintf( L"                  /2   - set 2k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /4   - set 4k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /8   - set 8k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /16  - set 16k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /32  - set 32k database page size (default: auto-detect)%c", wchNewLine );
    wprintf( L"                  /o   - suppress logo%c", wchNewLine );
}


LOCAL VOID EDBUTLHelp( _In_ PCWSTR wszAppName )
{
    wint_t c;

    EDBUTLPrintLogo();
    wprintf( L"%s%c%c", wszHelpDesc1, wchNewLine, wchNewLine );
    wprintf( L"%s%c", wszHelpSyntax, wchNewLine );
    wprintf( wszHelpModes1, wszAppName );   //  mode 1 is Defrag
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes2, wszAppName );   //  mode 2 is Recovery
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes3, wszAppName );   //  mode 3 is Integrity-Check
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes4, wszAppName );   //  mode 4 is Checksum
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes5, wszAppName );   //  mode 5 is Repair
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes6, wszAppName );   //  mode 6 is File Dump
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes7, wszAppName );   //  mode 7 is Copy File
    wprintf( L"%c", wchNewLine );
#ifndef ESENT
    wprintf( wszHelpModes8, wszAppName );   //  mode 8 is Restore
    wprintf( L"%c", wchNewLine );
#endif // ESENT

#ifdef DEBUG
    wprintf( wszHelpModes9, wszAppName );   //  mode 9 is Backup
    wprintf( L"%c", wchNewLine );

    wprintf( wszHelpModes12, wszAppName );  //  mode 12 is Scrub (undocumented in RETAIL )
    wprintf( L"%c", wchNewLine );
#endif  //  DEBUG

    wprintf( L"%c", wchNewLine );
    wprintf( L"%s%c", wszHelpPrompt1, wchNewLine );
    wprintf( L"%s%c", wszHelpPrompt2, wchNewLine );
#ifdef DEBUG
    wprintf( L"%s,%c", wszHelpPrompt3, wchNewLine );
    wprintf( L"%s%c", wszHelpPrompt4, wchNewLine );
#else
    wprintf( L"%s%c", wszHelpPrompt3, wchNewLine );
#endif
    wprintf( L"%s", wszHelpPromptCursor );
    c = (wint_t)_getch();

    wprintf( L"%c%c", wchNewLine, wchNewLine );

    switch ( c )
    {
        case L'd':
        case L'D':
            EDBUTLHelpDefrag( wszAppName );
            break;
        case L'r':
        case L'R':
            EDBUTLHelpRecovery( wszAppName );
            break;
        case L'g':
        case L'G':
            EDBUTLHelpIntegrity( wszAppName );
            break;
        case L'k':
        case L'K':
            EDBUTLHelpChecksum( wszAppName );
            break;
        case L'p':
        case L'P':
            EDBUTLHelpRepair( wszAppName );
            break;
        case L'm':
        case L'M':
            EDBUTLHelpDump( wszAppName );
            break;

        case L'y':
        case L'Y':
            EDBUTLHelpCopyFile( wszAppName );
        break;

#ifndef ESENT
        case L'c':
        case L'C':
            EDBUTLHelpHardRecovery( wszAppName );
            break;
#endif // ESENT

#ifdef DEBUG
        case L'b':
        case L'B':
            EDBUTLHelpBackup( wszAppName );
            break;
#endif

        //  NOTE: Scrub is undocumented in RETAIL
        case L'z':
        case L'Z':
            EDBUTLHelpScrub( wszAppName );
            break;
    }
}

LOCAL VOID EDBUTLGetTime( ULONG timerStart, INT *piSec, INT *piMSec )
{
    ULONG   timerEnd;

    timerEnd = TickOSTimeCurrent();

    *piSec = ( timerEnd - timerStart ) / 1000;
    *piMSec = ( timerEnd - timerStart ) % 1000;
}

LOCAL JET_ERR __stdcall PrintStatus( JET_SESID sesid, JET_SNP snp, JET_SNT snt, void *pv )
{
    static INT  iLastPercentage;
    INT         iPercentage;
    INT         dPercentage;

    switch ( snp )
    {
        case JET_SNP( -1 ):             // during Begin pv will point to szOperation
            AssertSz( fFalse, "Do we really get -1 from here!?  What would that mean" );
        case JET_snpCompact:
        case JET_snpUpgrade:
        case JET_snpRestore:
        case JET_snpRepair:
        case JET_snpScrub:
        case JET_snpUpgradeRecordFormat:
        case JET_snpSpaceCategorization:
            switch( snt )
            {
                case JET_sntProgress:
                    assert( pv );
                    iPercentage = static_cast< INT >( ( __int64( reinterpret_cast< JET_SNPROG* >( pv )->cunitDone ) * __int64( 100 ) ) / __int64( reinterpret_cast< JET_SNPROG* >( pv )->cunitTotal ) );
                    dPercentage = iPercentage - iLastPercentage;
                    assert( dPercentage >= 0 );
                    while ( dPercentage >= 2 )
                    {
                        wprintf( L"." );
                        iLastPercentage += 2;
                        dPercentage -= 2;
                    }
                    break;

                case JET_sntBegin:
                {
                    const WCHAR*    wszOperation;
                    SIZE_T          cchPadding, cchOper;

                    switch ( snp )
                    {
                        default:
                            wszOperation = wszDefrag;
                            break;
                        case JET_snpUpgrade:
                            wszOperation = wszUpgrade;
                            break;
                        case JET_snpRestore:
                            wszOperation = wszRestore;
                            break;
                        case JET_snpRepair:
                            wszOperation = wszRepair;
                            break;
                        case JET_snpScrub:
                            wszOperation = wszScrub;
                            break;
                        case JET_snpUpgradeRecordFormat:
                            wszOperation = wszUpgradeRecordFormat;
                            break;
                        case JET_snpSpaceCategorization:
                            wszOperation = wszSpaceCat;
                            break;
                        case -1:
                            assert( NULL != pv );
                            wszOperation = (WCHAR *)pv;
                    }

                    wprintf( L"%c", wchNewLine );

                    // Center the status message above the status bar.
                    // Formula is: ( length of status bar - length of message ) / 2
                    cchOper = LOSStrLengthW( wszOperation );
                    assert( cchOper + (ULONG)LOSStrLengthW( wszStatusMsg ) <= 51 );
                    cchPadding = ( 51 - ( cchOper + (ULONG)LOSStrLengthW( wszStatusMsg ) ) ) / 2;

                    wprintf( L"          %*s%s%c%c", (INT)(cchPadding+cchOper), wszOperation, wszStatusMsg, wchNewLine, wchNewLine );
                    wprintf( L"          0    10   20   30   40   50   60   70   80   90  100\n" );
                    wprintf( L"          |----|----|----|----|----|----|----|----|----|----|\n" );
                    wprintf( L"          " );

                    iLastPercentage = 0;
                    break;
                }

                case JET_sntComplete:
                    dPercentage = 100 - iLastPercentage;
                    assert( dPercentage >= 0 );
                    while ( dPercentage >= 2 )
                    {
                        wprintf( L"." );
                        iLastPercentage += 2;
                        dPercentage -= 2;
                    }

                    wprintf( L".%c%c", wchNewLine, wchNewLine );
                    break;

                case JET_sntFail:
                    wprintf( L"X" );
                    break;

                // Deprecated ...
                case 8: // deprecated JET_sntRecoveryStep:
                    AssertSz( fFalse, "We do not want to do JET_sntRecoveryStep anymore ... old ese[nt].dll?" );
                    break;

            }
            break;
    }

    return JET_errSuccess;
}

const WCHAR * WszReasonFromEnum( ULONG eReason )
{
    switch( eReason )
    {
        case JET_OpenLogForRecoveryCheckingAndPatching:
            return L"Patching";
        case JET_OpenLogForRedo:
            return L"ForRedo ";
        case JET_OpenLogForUndo:
            return L"ForUndo ";
        case JET_OpenLogForDo:
            return L"ForDo   ";
        default:
            AssertSz( fFalse, "Unknown OpenLog reason" );
            return L"Unknown ";
    }
}

const WCHAR * WszNextActionFromEnum( ULONG eNextAction )
{
    switch( eNextAction )
    {
        case JET_MissingLogContinueToRedo:
            return L"JET_MissingLogContinueToRedo";
        case JET_MissingLogContinueTryCurrentLog:
            return L"JET_MissingLogContinueTryCurrentLog";
        case JET_MissingLogContinueToUndo:
            return L"JET_MissingLogContinueToUndo";
        case JET_MissingLogCreateNewLogStream:
            return L"JET_MissingLogCreateNewLogStream";
        default:
            AssertSz( fFalse, "Unknown MissingLog NextAction reason" );
            return L"Unknown ";
    }
}

LOCAL JET_ERR __stdcall ProcessRecoveryControlAndRestoreStatus( JET_SNP snp, JET_SNT snt, void *pvData, void * pvContext )
{
    JET_ERR errRet = JET_errSuccess;

    const UTILOPTS * const popts = reinterpret_cast< UTILOPTS* >( pvContext );
    // in case we need it, I believe the prstinfo is available here:
    //const JET_RSTINFO2_W popts = reinterpret_cast<JET_RSTINFO_W *>( popts->pv );

    const BOOL fVerbose = FUTILOPTSVerbose( popts->fUTILOPTSFlags );

    const WCHAR wszStatusIndent [] = L"        ";

    Assert( /* Deprecated JET_sntRecoveryStep */ 8 != snt );

    switch ( snp )
    {
        case JET_snpRestore:
            // I don't think I need the other cases here b/c they use callback v1

            if ( !fVerbose )
            {
                PrintStatus( JET_sesidNil, snp, snt, pvData );
            }
            else    // fVerbose = fTrue ...
            {
                switch ( snt )
                {
                    case JET_sntBegin:
                        wprintf( L"    Beginning Verbose Recovery ---------------- \n" );
                        break;
                    case JET_sntProgress:
                    {
                        // int iPercentage = static_cast< int >( ( __int64( reinterpret_cast< JET_SNPROG* >( pvData )->cunitDone ) * __int64( 100 ) ) / __int64( reinterpret_cast< JET_SNPROG* >( pvData )->cunitTotal ) );
                        // this one is a little noisy and breaks up the table affect I'm going for in /v(erbose) mode.
                        //wprintf( L"%wsStatus: %d%% complete.\n", wszStatusIndent, iPercentage );
                    }
                        break;
                    case JET_sntComplete:
                        wprintf( L"%wsStatus: Complete!\n\n", wszStatusIndent );
                        break;
                    case JET_sntFail:
                        wprintf( L"%wsStatus: Got FAIL status!\n", wszStatusIndent );
                        break;
                }
            }
            break;

        case JET_snpRecoveryControl:
        {
            Assert( snt > 1000 && snt < 1100 );
            Assert( popts->grbitInit & JET_bitExternalRecoveryControl );
#ifdef ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO
            Assert( popts->grbitInit & JET_bitRecoveryWithoutUndo );
#endif

            JET_RECOVERYCONTROL *   precctrl = reinterpret_cast< JET_RECOVERYCONTROL* >( pvData );

            if ( precctrl->cbStruct >= ( OffsetOf( JET_RECOVERYCONTROL, errDefault ) + sizeof(JET_ERR) ) )
            {
                //  Setup the return error as instructed by ESE.
                errRet = precctrl->errDefault;
            }

            if ( !precctrl || precctrl->cbStruct != sizeof(JET_RECOVERYCONTROL) )
            {
                wprintf( L"%cJET_snpRecoveryControl - invalid callback data %c", wchNewLine, wchNewLine );
                break;
            }

            switch( snt )
            {
                case JET_sntOpenLog:
                    if ( fVerbose )
                    {
                        wprintf( L"%wsOpenLog   : %ws: errDefault=%d, lGenNext=%d, %ws, %d DBs, szFile=%ws\n", wszStatusIndent,
                                WszReasonFromEnum( precctrl->OpenLog.eReason ),
                                precctrl->errDefault, precctrl->OpenLog.lGenNext,
                                precctrl->OpenLog.fCurrentLog ? L"fCurrent" : L"eArchive",
                                precctrl->OpenLog.cdbinfomisc, precctrl->OpenLog.wszLogFile );
                    }
#ifdef ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO
                    if ( !FUTILOPTSRecoveryWithoutUndoForReal( popts->fUTILOPTSFlags  )&&
                            precctrl->OpenLog.eReason == JET_OpenLogForDo )
                    {
                        Assert( precctrl->errDefault == JET_errRecoveredWithoutUndoDatabasesConsistent );
                        errRet = JET_errSuccess;
                    }
#endif
                    break;
                case JET_sntOpenCheckpoint:
                    if ( fVerbose )
                    {
                        wprintf( L"%wsOpenCheckp: errDefault=%d\n", wszStatusIndent,
                                precctrl->errDefault );
                    }
                    break;

                case JET_sntMissingLog:
                {
                    if ( fVerbose )
                    {
                        wprintf( L"%wsMissingLog: errDefault=%d, lGenNext=%d, %ws, %ws, %d DBs, szFile=%ws\n", wszStatusIndent,
                                precctrl->errDefault, precctrl->MissingLog.lGenMissing,
                                precctrl->MissingLog.fCurrentLog ? L"fCurrent" : L"eArchive",
                                WszNextActionFromEnum( precctrl->MissingLog.eNextAction ),
                                precctrl->MissingLog.cdbinfomisc, precctrl->MissingLog.wszLogFile );
                    }

                    //  validation
                    if ( precctrl->MissingLog.fCurrentLog &&
                            precctrl->MissingLog.eNextAction == JET_MissingLogCreateNewLogStream )
                    {
                        if ( popts->grbitInit & JET_bitLogStreamMustExist )
                        {
                            Assert( precctrl->errDefault != JET_errSuccess ); // typical case for eseutil
                        }
                        else
                        {
                            Assert( precctrl->errDefault == JET_errSuccess ); // yes, create a new stream
                        }
                    }
                }
                    break;

                case JET_sntBeginUndo:
                {
                    if ( fVerbose )
                    {
                        wprintf( L"%wsBeginUndo : errDefault=%d, %d DBs\n", wszStatusIndent,
                                precctrl->errDefault, precctrl->BeginUndo.cdbinfomisc );
                    }
#ifdef ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO
                    if ( !FUTILOPTSRecoveryWithoutUndoForReal( popts->fUTILOPTSFlags ) )
                    {
                        Assert( precctrl->errDefault == JET_errRecoveredWithoutUndo ||
                                precctrl->errDefault == JET_errRecoveredWithoutUndoDatabasesConsistent );
                        errRet = JET_errSuccess;
                    }
#endif
                }
                    break;

                case JET_sntSignalErrorCondition:
                {
                    if ( fVerbose )
                    {
                        wprintf( L"%wsErrCond   : errDefault=%d\n", wszStatusIndent,
                                precctrl->errDefault );
                    }
                }
                    break;

                case JET_sntNotificationEvent:
                case JET_sntAttachedDb:
                case JET_sntDetachingDb:
                case JET_sntCommitCtx:
                    break;

                default:
                    //  We should keep eseutil up-to-date with know possibilities ...
                    ExpectedSz( fFalse, "Unknown JET_SNT snt = %d\n", snt );
            }
            break;
        }

        case JET_SNP( -1 ):             // during Begin pv will point to szOperation
            AssertSz( fFalse, "Do we really get -1 from here!?  What would that mean" );
        default:
            //  We should keep eseutil up-to-date with known possibilities ...
            ExpectedSz( fFalse, "Unknown JET_SNP snp = %d", snp );
    }

    return errRet;
}

const BYTE EDBUTL_errInvalidPath = 1;   //  Does not form right path. param1 = file type
const BYTE EDBUTL_errSharedName = 2;    //  Two file types share one file name. param1, param2 - files types
const BYTE EDBUTL_errInvalidDB  = 3;    //  cannot read source database header. param1 - error code

const BYTE EDBUTL_paramSrcDB        = 1;
const BYTE EDBUTL_paramTempDB   = 2;
const BYTE EDBUTL_paramLast     = 3;

LOCAL VOID PrintErrorMessage( INT const err, const UTILOPTS* const popts, INT param1 = 0, INT param2 = 0 )
{
    assert( NULL != popts );

    static const WCHAR * const wszObjectName[2*EDBUTL_paramLast] =
    {
        L"",
        L"Source database",
        L"Temporary database",
        L"",
        L"source database",
        L"temporary database"
    };

    const WCHAR *const wszObjectData[EDBUTL_paramLast] =
    {
        L"",
        popts->wszSourceDB,
        popts->wszTempDB
    };

    switch ( err )
    {
        case EDBUTL_errInvalidPath:
            AssertPREFIX( 0 < param1 );
            AssertPREFIX( EDBUTL_paramLast > param1 );
            wprintf( L"Error: %s specification '%s' is invalid.",
                wszObjectName[param1], wszObjectData[param1] );
            break;
        case EDBUTL_errSharedName:
            AssertPREFIX( 0 < param1 );
            AssertPREFIX( EDBUTL_paramLast > param1 );
            AssertPREFIX( 0 < param2 );
            AssertPREFIX( EDBUTL_paramLast > param2 );
            wprintf( L"Error: %s '%s' cannot be the same as %s.",
                wszObjectName[param1], wszObjectData[param1], wszObjectName[param2+EDBUTL_paramLast] );
            break;
        case EDBUTL_errInvalidDB:
            wprintf( L"Error: Access to source database '%s' failed with Jet error %i.",
                wszObjectData[EDBUTL_paramSrcDB], param1 );
            if ( FUTILOPTSMaybeTolerateCorruption( popts->fUTILOPTSFlags ) &&
                    FIsCorruptionError( param1 ) &&
                    !FUTILOPTSTolerateCorruption( popts, param1 ) )
            {
                wprintf( L"%c%cThis may have happenned due to a corrupted database header.", wchNewLine, wchNewLine );
                wprintf( L" Explicitly setting a page size might bypass this failure." );
            }
            break;
        default:
            assert( 0 );
            return;
    }
    wprintf( L"%c%c", wchNewLine, wchNewLine );
}


//  Set default cache size max
LOCAL JET_ERR ErrEDBUTLSetCacheSizeMax( UTILOPTS* pOpts )
{
    JET_ERR err = JET_errSuccess;

    if ( 0 == pOpts->lMaxCacheSize )
    {
        SYSTEM_INFO info;
        GetSystemInfo( &info );

        DWORD_PTR cbTotalAvailable = ( DWORD_PTR )info.lpMaximumApplicationAddress - ( DWORD_PTR )info.lpMinimumApplicationAddress;
        DWORD cbReserve = 1024 * 1024 * 512;

        JET_INSTANCE instance = JET_instanceNil;
        DWORD_PTR cbPage = 0;
        JetGetSystemParameterW( instance, JET_sesidNil, JET_paramDatabasePageSize, &cbPage, NULL, sizeof( cbPage ) );

        DWORD_PTR cpgCacheSizeMax = ( cbTotalAvailable - cbReserve ) / cbPage;
        Call( JetSetSystemParameterW( &instance, JET_sesidNil, JET_paramCacheSizeMax, cpgCacheSizeMax, NULL ) );
    }

HandleError:
    return err;
}

//  Check database file:
LOCAL JET_ERR ErrEDBUTLCheckDBName(
    UTILOPTS* const             popts,
    PCWSTR const                wszTempDB,
    LONG * const                pfiletype )
//  Parameters:
//      options structure
//      default temporary database name
{
    assert( NULL != popts );
    assert( NULL != wszTempDB );

    WCHAR   wszFullpathSrcDB[ _MAX_PATH + 1] = L"";
    WCHAR   wszFullpathTempDB[ _MAX_PATH + 1 ] = L"";

    //  if TempDB is not defined
    if ( NULL == popts->wszTempDB )
    {
        //  set TempDB to DefaultTempDB
        popts->wszTempDB = (WCHAR *)wszTempDB;
    }

    //  if temp db is not valid path then ERROR
    if ( NULL == _wfullpath( wszFullpathTempDB, popts->wszTempDB, _MAX_PATH ) )
    {
        PrintErrorMessage( EDBUTL_errInvalidPath, popts, EDBUTL_paramTempDB );
        return ErrERRCheck( JET_errInvalidPath );
    }

    //  if SrcDB is not defined or is invalid path then ERROR
    if ( NULL == popts->wszSourceDB )
    {
        wprintf( wszUsageErr1, L"source database" );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    if ( NULL == _wfullpath( wszFullpathSrcDB, popts->wszSourceDB, _MAX_PATH ) )
    {
        PrintErrorMessage( EDBUTL_errInvalidPath, popts, EDBUTL_paramSrcDB );
        return ErrERRCheck( JET_errInvalidPath );
    }

    //  if the page size is NOT specified, try to consult the database header
    //  to determine the page size
    ULONG   cbPageSize  = 0;
    if (    !FUTILOPTSExplicitPageSet( popts->fUTILOPTSFlags ) &&
            JET_errSuccess == JetGetDatabaseFileInfoW( popts->wszSourceDB, &cbPageSize, sizeof( cbPageSize ), JET_DbInfoPageSize ) )
    {
        const JET_ERR err = JetSetSystemParameterW( NULL, 0, JET_paramDatabasePageSize, cbPageSize, NULL );
        if ( err < JET_errSuccess &&
            ( popts->mode != modeDump || cbPageSize != 2048 || err != JET_errInvalidParameter) ) //special case for 2k pagesize headers
        {
            return err;
        }
        if ( 2 * 1024 == cbPageSize )
        {
            UTILOPTSSet2KPage( popts->fUTILOPTSFlags );
        }
        else if ( 4 * 1024 == cbPageSize )
        {
            UTILOPTSSet4KPage( popts->fUTILOPTSFlags );
        }
        else if ( 8 * 1024 == cbPageSize )
        {
            UTILOPTSSet8KPage( popts->fUTILOPTSFlags );
        }
        else if ( 16 * 1024 == cbPageSize )
        {
            UTILOPTSSet16KPage( popts->fUTILOPTSFlags );
        }
        else if ( 32 * 1024 == cbPageSize )
        {
            UTILOPTSSet32KPage( popts->fUTILOPTSFlags );
        }
        else
        {
            AssertSz( false, "Invalid page size %u found.", cbPageSize );
        }
    }

    //  try to find the file type
    LONG    filetype;
    JET_ERR err = JetGetDatabaseFileInfoW( popts->wszSourceDB, &filetype, sizeof( filetype ), JET_DbInfoFileType );
    if ( JET_errSuccess != err )
    {
        // From an user-perspective, this means the database is effectivelly corrupted.
        if ( err == JET_errReadVerifyFailure ||
            err == JET_errFileSystemCorruption ||
            err == JET_errDiskReadVerificationFailure )
        {
            err = ErrERRCheck( JET_errDatabaseCorrupted );
        }
        if ( err < JET_errSuccess && !FUTILOPTSTolerateCorruption( popts, err ) )
        {
            PrintErrorMessage( EDBUTL_errInvalidDB, popts, err );
            return err;
        }
            filetype = JET_filetypeUnknown;
    }
    if ( NULL != pfiletype )
    {
        *pfiletype = filetype;
    }

    //  if TempDB is the same as SrcDB then ERROR
    if ( 0 == _wcsicmp( wszFullpathSrcDB, wszFullpathTempDB ) )
    {
        PrintErrorMessage( EDBUTL_errSharedName, popts, EDBUTL_paramSrcDB, EDBUTL_paramTempDB );
        return ErrERRCheck( JET_errInvalidDatabase );
    }

    ErrEDBUTLSetCacheSizeMax( popts );

    return JET_errSuccess;
}

#ifdef DEBUG
LOCAL JET_ERR ErrEDBUTLCheckBackupPath( UTILOPTS *popts )
{
    WCHAR   wszFullpathBackup[ _MAX_PATH + 1];

    if ( popts->wszBackup == NULL )
    {
        wprintf( wszUsageErr1, L"backup path" );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    if ( _wfullpath( wszFullpathBackup, popts->wszBackup, _MAX_PATH ) == NULL )
    {
        wprintf( wszErr1, popts->wszBackup );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        return ErrERRCheck( JET_errInvalidPath );
    }

    return JET_errSuccess;
}
#endif

LOCAL BOOL FEDBUTLParsePath(    _In_ PCWSTR             arg,
                                __deref_inout_z WCHAR **    pwszParam,
                                __in_z PCWSTR               wszParamDesc,
                                _In_ BOOL               fAllowEmpty     = fFalse )
{
    BOOL    fResult = fTrue;

    // if the argument is empty try to read next one argument
    if ( L'\0' == *arg )
    {
        const WCHAR *argT = arg;
        arg = GetNextArg();
        // if it was last argument or option follows it means we passed empty argument
        if ( NULL == arg || NULL != wcschr( wszSwitches, *arg ) )
        {
            arg = argT;
            SetCurArgID( GetCurArgID() - 1 );
        }
    }

    // no path should contain leading ':'(s)
    while ( L':' == *arg )
    {
        arg++;
    }


    if ( L'\0' == *arg && !fAllowEmpty )
    {
        wprintf( wszUsageErr1, wszParamDesc );          // Missing spec.
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        fResult = fFalse;
    }
    else if ( NULL == *pwszParam )
    {
        *pwszParam = (WCHAR *) arg;
    }
    else
    {
        wprintf( wszUsageErr2, wszParamDesc );          // Duplicate spec.
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        fResult = fFalse;
    }

    return fResult;
}

// will look for something like <path1>[:<path2>].
// Note that if <path1> is just one char long, we have a problem ...
// The workaround for the user would be to provide full path instead.
//
LOCAL BOOL FEDBUTLParseDoublePath(  _In_ PCWSTR                 arg,
                                    __deref_inout_z PWSTR*      pwszParam1,
                                    __deref_opt_out_opt PWSTR*  pwszParam2,
                                    __in_z PCWSTR                   wszParamDesc,
                                    _In_ BOOL                   fAllowEmpty     = fFalse )
{
    BOOL fResult;

    fResult = FEDBUTLParsePath( arg, pwszParam1, wszParamDesc, fAllowEmpty );

    if ( fResult && pwszParam2 )
    {
        // let's check if there are 2 parts in the path found
        assert( *pwszParam1 );

        *pwszParam2 = NULL;
        if ( LOSStrLengthW( *pwszParam1 ) >= 2 )
        {
            // we start to look for the ":" with the 3rd position (it might be \0 but that's ok)
            WCHAR * wszDelim = wcschr( (*pwszParam1) + 2, L':' );

            if ( wszDelim )
            {
                *pwszParam2 = wszDelim + 1;
                // replace the ':' with a '\0' such that we have 2
                // zero terminated strings now
                *wszDelim = L'\0';
            }
        }
    }

    return fResult;
}

LOCAL const INT g_cchEfvOptionPrefix = 4;
LOCAL BOOL FEseutilIsEfvArg( PCWSTR wszArg )
{
    #define wszEfvOptionPrefix L"/efv"
    C_ASSERT( _countof(wszEfvOptionPrefix) == g_cchEfvOptionPrefix + 1 /* NUL char */ );
    if ( _wcsnicmp( wszArg, wszEfvOptionPrefix, g_cchEfvOptionPrefix ) == 0 ||
        //  some people like minuses ...
        _wcsnicmp( wszArg, L"-efv", g_cchEfvOptionPrefix ) == 0 )
    {
        return fTrue;
    }

    return fFalse;
}

LOCAL ERR ErrEseutilScanEfv( PCWSTR wszParam, JET_ENGINEFORMATVERSION * const pefv )
{
    if ( 0 == _wcsicmp( wszParam, L"UsePersistedFormat" ) )
    {
        *pefv = JET_efvUsePersistedFormat;
    }
    else if ( 0 == _wcsicmp( wszParam, L"UseEngineDefault" ) )
    {
        *pefv = JET_efvUseEngineDefault;
    }
    else if ( 1 == swscanf_s( wszParam, L"%d", pefv ) &&
                *pefv > 0 )
    {
        //  set by swscanf_s() and > 0 ... success!
    }
    else
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    return JET_errSuccess;
}

typedef struct
{
    ULONG cpgUnknown;
    ULONG cpgNotOwnedEof;
    ULONG cpgNotOwned;
    ULONG cpgLeaked;
    ULONG cpgInconsistent;
    ULONG cpgIndeterminate;
    ULONG cpgAvailable;
    ULONG cpgSpaceAE;
    ULONG cpgSpaceOE;
    ULONG cpgSmallSpace;
    ULONG cpgSplitBuffer;
    ULONG cpgRoot;
    ULONG cpgStrictlyInternal;
    ULONG cpgStrictlyLeaf;
    ULONG cpgTotal;
} SpaceCatStats;

LOCAL BYTE BEseUtilSpaceCatFlagToByte( _In_ const SpaceCategoryFlags spcatfGroup, _In_ const SpaceCategoryFlags spcatf )
{
    return (BYTE)( ( spcatfGroup & spcatf ) != 0 );
}

LOCAL VOID EseutilDumpSpaceCat(
    _In_ const unsigned long pgno,
    _In_ const unsigned long objid,
    _In_ const SpaceCategoryFlags spcatf,
    _In_opt_ void* const pvContext )
{
    static SpaceCatStats spcatStats = { 0 };
    const JET_DBUTIL_W* const pdbutil = (JET_DBUTIL_W*)pvContext;
    const BOOL fVerbose = ( pdbutil->grbitOptions & JET_bitDBUtilOptionDumpVerbose ) != 0;

    if ( !fVerbose )
    {
        // First page.
        if ( pgno == pdbutil->spcatOptions.pgnoFirst )
        {
            // Print progress bar initilizer.
            (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntBegin, NULL );
        }

        // Report progress bar step.
        JET_SNPROG snprog =
        {
            sizeof( snprog ),
            pgno - pdbutil->spcatOptions.pgnoFirst + 1,
            pdbutil->spcatOptions.pgnoLast - pdbutil->spcatOptions.pgnoFirst + 1
        };
        (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntProgress, &snprog );

        // Last page.
        if ( pgno == pdbutil->spcatOptions.pgnoLast )
        {
            // Print progress bar finalizer.
            (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntComplete, NULL );
        }
    }
    else
    {
        // First page.
        if ( pgno == pdbutil->spcatOptions.pgnoFirst )
        {
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            wprintf( L"Pgno,Objid,Unkn,NotOwnedEof,NotOwned,Leaked,Inc,Ind,Avail,SpaceAE,SpaceOE,SmallSpace,SpBuffer,Root,Internal,Leaf%c", wchNewLine );
        }

        // Print flags which are set.
        wprintf( L"%I32u,%I32u,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu%c",
                    pgno,
                    objid,
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfUnknown ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfNotOwnedEof ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfNotOwned ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfLeaked ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfInconsistent ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfIndeterminate ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfAvailable ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfSpaceAE ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfSpaceOE ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfSmallSpace ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfSplitBuffer ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfRoot ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfStrictlyInternal ),
                    BEseUtilSpaceCatFlagToByte( spcatf, spcatfStrictlyLeaf ),
                    wchNewLine );

        // Last page.
        if ( pgno == pdbutil->spcatOptions.pgnoLast )
        {
            wprintf( L"%c", wchNewLine );
        }
    }

    // Accumulate stats.
    if ( pgno == pdbutil->spcatOptions.pgnoFirst )
    {
        memset( &spcatStats, 0, sizeof( spcatStats ) );
    }
#ifdef DEBUG
    assert( spcatf != spcatfNone );
    const SpaceCatStats spcatStatsT = spcatStats;
#endif
    if ( spcatf & spcatfUnknown ) spcatStats.cpgUnknown++;
    if ( spcatf & spcatfNotOwnedEof ) spcatStats.cpgNotOwnedEof++;
    if ( spcatf & spcatfNotOwned ) spcatStats.cpgNotOwned++;
    if ( spcatf & spcatfLeaked ) spcatStats.cpgLeaked++;
    if ( spcatf & spcatfInconsistent ) spcatStats.cpgInconsistent++;
    if ( spcatf & spcatfIndeterminate ) spcatStats.cpgIndeterminate++;
    if ( spcatf & spcatfAvailable ) spcatStats.cpgAvailable++;
    if ( spcatf & spcatfSpaceAE ) spcatStats.cpgSpaceAE++;
    if ( spcatf & spcatfSpaceOE ) spcatStats.cpgSpaceOE++;
    if ( spcatf & spcatfSmallSpace ) spcatStats.cpgSmallSpace++;
    if ( spcatf & spcatfSplitBuffer ) spcatStats.cpgSplitBuffer++;
    if ( spcatf & spcatfRoot ) spcatStats.cpgRoot++;
    if ( spcatf & spcatfStrictlyInternal ) spcatStats.cpgStrictlyInternal++;
    if ( spcatf & spcatfStrictlyLeaf ) spcatStats.cpgStrictlyLeaf++;
#ifdef DEBUG
    assert( memcmp( &spcatStatsT, &spcatStats, sizeof( SpaceCatStats ) ) != 0 );
#endif
    spcatStats.cpgTotal++;

    // Print stats if we're done.
    if ( pgno == pdbutil->spcatOptions.pgnoLast )
    {
        wprintf( L"Space Categorization Statistics:%c", wchNewLine );
        wprintf( L"     Total Page Count: %I32u%c", spcatStats.cpgTotal, wchNewLine );
        wprintf( L"        Strictly Leaf: %I32u%c", spcatStats.cpgStrictlyLeaf, wchNewLine );
        wprintf( L"    Strictly Internal: %I32u%c", spcatStats.cpgStrictlyInternal, wchNewLine );
        wprintf( L"                 Root: %I32u%c", spcatStats.cpgRoot, wchNewLine );
        wprintf( L"         Split Buffer: %I32u%c", spcatStats.cpgSplitBuffer, wchNewLine );
        wprintf( L"          Small Space: %I32u%c", spcatStats.cpgSmallSpace, wchNewLine );
        wprintf( L"             Space OE: %I32u%c", spcatStats.cpgSpaceOE, wchNewLine );
        wprintf( L"             Space AE: %I32u%c", spcatStats.cpgSpaceAE, wchNewLine );
        wprintf( L"            Available: %I32u%c", spcatStats.cpgAvailable, wchNewLine );
        wprintf( L"        Indeterminate: %I32u%c", spcatStats.cpgIndeterminate, wchNewLine );
        wprintf( L"         Inconsistent: %I32u%c", spcatStats.cpgInconsistent, wchNewLine );
        wprintf( L"               Leaked: %I32u%c", spcatStats.cpgLeaked, wchNewLine );
        wprintf( L"            Not Owned: %I32u%c", spcatStats.cpgNotOwned, wchNewLine );
        wprintf( L"        Not Owned EOF: %I32u%c", spcatStats.cpgNotOwnedEof, wchNewLine );
        wprintf( L"              Unknown: %I32u%c", spcatStats.cpgUnknown, wchNewLine );
    }
}

LOCAL BOOL FEDBUTLParseDefragment( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fTrue;

    switch( arg[1] )
    {
        case L'b':
        case L'B':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszBackup, L"backup database" );
            break;

        case 'e':
        case 'E':
            fResult = fFalse;
            if ( FEseutilIsEfvArg( arg ) ) //   /efvX
            {
                fResult = fTrue;
                if ( ErrEseutilScanEfv( arg + g_cchEfvOptionPrefix, &popts->efvUserSpecified ) < JET_errSuccess )
                {
                    wprintf( L"%s%c%c", wszUsageErr20, wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
            }
            break;

        case L'n':
        case L'N':
            UTILOPTSSetDefragInfo( popts->fUTILOPTSFlags );
            break;

        case L'p':
        case L'P':
            UTILOPTSSetPreserveTempDB( popts->fUTILOPTSFlags );
            break;

        case L't':
        case L'T':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszTempDB, L"temporary database" );
            break;

        case L'w':
        case L'W':
            popts->cpageBatchIO = _wtol( arg + 2 );
            if ( popts->cpageBatchIO <= 0 )
            {
                wprintf( L"%s%c%c", wszUsageErr7, wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            break;

        case L'x':
        case L'X':
            popts->cpageDbExtension = _wtol( arg + 2 );
            if ( popts->cpageDbExtension <= 0 )
            {
                wprintf( L"%s%c%c", wszUsageErr8, wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            break;

        case 'c':
        case 'C':
            if ( 1 != swscanf_s( arg + 2, L"%d", &popts->lMaxCacheSize ) ||
                popts->lMaxCacheSize < 0 )
            {
                wprintf( L"%s%c%c", wszUsageErr19, wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL BOOL FEDBUTLParseRecovery( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fTrue;

    // convenient for debugging to always set verbose ...
    //UTILOPTSSetVerbose( popts->fUTILOPTSFlags );

    switch( arg[1] )
    {
        case L'i':
        case L'I':
            popts->grbitInit |= JET_bitReplayIgnoreMissingDB;
            break;

        case L'e':
        case L'E':
            fResult = fFalse;
            if ( FEseutilIsEfvArg( arg ) ) //   /efvX
            {
                fResult = fTrue;
                if ( ErrEseutilScanEfv( arg + g_cchEfvOptionPrefix, &popts->efvUserSpecified ) < JET_errSuccess )
                {
                    wprintf( L"%s%c%c", wszUsageErr20, wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
            }
            else
            {
                //  Unset this bit, thats set by default ...
                popts->grbitInit &= ~JET_bitLogStreamMustExist;
                fResult = fTrue;
            }
            break;

        case L'a':
        case L'A':
            popts->grbitInit |= JET_bitReplayIgnoreLostLogs;
            UTILOPTSSetDelOutOfRangeLogs( popts->fUTILOPTSFlags );
#ifdef DEBUG
            //  This is for being able to triger the ignore lost logs without 
            //  delete out of range logs being set ... "/a-" which is like /a 
            //  but slightly less, thus the minus.
            if ( arg[2] == L'-' )
            {
                UTILOPTSResetDelOutOfRangeLogs( popts->fUTILOPTSFlags );
            }
#endif
            break;

        case L'u':
        case L'U':
            popts->grbitInit |= JET_bitRecoveryWithoutUndo;
#ifdef ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO
            UTILOPTSSetRecoveryWithoutUndoForReal( popts->fUTILOPTSFlags );
#endif

            JET_RSTINFO2_W * prstinfo;
            prstinfo = reinterpret_cast<JET_RSTINFO2_W *>( popts->pv );

            JET_LGPOS      lgposStop;

            // if no log or log position specified, then we will play to the end but no Undo
            //
            lgposStop = lgposMax;

            assert( LOSStrLengthW(arg) >= 2 );

            if ( arg[2] == L'\0' )
            {
                prstinfo->lgposStop = lgposStop;
                fResult = fTrue;
            }
// the option to stop at a log position is not requested and not tested
// so we will make this DEBUG only
//
#ifdef DEBUG
            // try to see if there is a log position
            //
            else if ( swscanf_s( &(arg[2]), L"%u:%hu:%hu", &lgposStop.lGeneration, &lgposStop.isec, &lgposStop.ib ) == 3 )
            {
                prstinfo->lgposStop = lgposStop;
                fResult = fTrue;
            }
            
#endif // DEBUG
            // try to see if there is a log generation
            //
            else if ( swscanf_s( &(arg[2]), L"%u", &lgposStop.lGeneration ) == 1 )
            {
                // set the stop at the end of log
                prstinfo->lgposStop.lGeneration = lgposStop.lGeneration;
                fResult = fTrue;
            }
            else
            {
                wprintf( L"%s%c%c", wszUsageErr16, wchNewLine, wchNewLine );
                fResult = fFalse;
            }

            if ( arg[2] != '\0' && prstinfo->lgposStop.lGeneration <= 0 )
            {
                wprintf( L"%s%c%c", wszUsageErr16, wchNewLine, wchNewLine );
                fResult = fFalse;
            }

            break;

        case L't':
        case L'T':
            popts->grbitInit |= JET_bitTruncateLogsAfterRecovery;
            break;

        case L'l':
        case L'L':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszLogfilePath, L"logfile path" );
            break;

        case L's':
        case L'S':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszSystemPath, L"system path" );
            break;

        case L'm':
        case L'M':
            //  UNDONE: temp db is not used during recovery, so why is this useful???
            //
            popts->pageTempDBMin = _wtol( arg + 2 );
            fResult = fTrue;
            break;

        case L'd':
        case L'D':
            if (    NULL != popts->wszBackup ||
                    NULL != popts->wszRestore ||
                    0 != popts->crstmap )
            {
                wprintf( L"%s%c%c", wszUsageErr3, wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                //  if no directory specified, use current directory
                fResult = FEDBUTLParsePath( arg+2, &popts->wszDbAltRecoveryDir, L"database directory", fTrue );
                if ( fResult && L'\0' == popts->wszDbAltRecoveryDir[0] )
                    popts->wszDbAltRecoveryDir = (WCHAR *)wszCurrDir;
            }
            break;

#ifdef DEBUG
        case L'b':
        case L'B':
            if ( NULL != popts->wszDbAltRecoveryDir )
            {
                wprintf( wszUsageErr3 );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                fResult = FEDBUTLParsePath( arg+2, &popts->wszBackup, L"backup directory" );
            }
            break;

        case L'r':
        case L'R':
            if ( NULL != popts->wszDbAltRecoveryDir )
            {
                wprintf( wszUsageErr3 );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                fResult = FEDBUTLParsePath( arg+2, &popts->wszRestore, L"destination directory" );
            }
            break;
#endif
        case L'n':
        case L'N':
            if (    NULL != popts->wszBackup ||
                    NULL != popts->wszRestore ||
                    NULL != popts->wszDbAltRecoveryDir )
            {
                wprintf( L"%s%c%c", wszUsageErr3, wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                assert( ( 0 != popts->crstmap && NULL != popts->prstmap ) ||
                    ( 0 == popts->crstmap && NULL == popts->prstmap ) );

                if ( 0 == popts->crstmap )
                {
                    ULONG_PTR cMaxDbs = 0;
                    if ( JET_errSuccess >= JetGetSystemParameterW( JET_instanceNil, JET_sesidNil, JET_paramMaxDatabasesPerInstance, &cMaxDbs, NULL, sizeof( cMaxDbs ) ) )
                    {
                        // default to 6 (the current ESE default max user db)
                        cMaxDbs = 6;
                    }

                    // we allocate space for each database EDB
                    popts->crstmap = (LONG) cMaxDbs; // truncate, hope we never end up w/ more than 2 billion DBs attachable ...
                    popts->prstmap = (JET_RSTMAP2_W *) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, popts->crstmap * sizeof(JET_RSTMAP2_W) );

                    // we will error out
                    if ( NULL == popts->prstmap )
                    {
                        wprintf( L"Out of memory error trying to allocate JET_RSTMAP of %zd bytes.", popts->crstmap * sizeof(JET_RSTMAP2_W) );
                        wprintf( L"%c%c", wchNewLine, wchNewLine );
                        popts->crstmap = 0;
                        fResult = fFalse;
                        // exit the case
                        break;
                    }

                    popts->irstmap = 0;
                }


                // the * is a special mark to use JET_bitReplayMissingMapEntryDB
                if ( *(arg+2) == L'*' )
                {
                    if ( *(arg+3) != L'\0' )
                    {
                        wprintf( wszUsageErr17 );
                        wprintf( L"%c%c", wchNewLine, wchNewLine );
                        fResult = fFalse;
                    }
                    else
                    {
                        fResult = fTrue;
                        popts->grbitInit |= JET_bitReplayMissingMapEntryDB;
                    }
                }
                else
                {
                    assert ( 0 != popts->crstmap );
                    assert ( 0 != popts->prstmap );

                    //  if no directory specified, use current directory
                    popts->prstmap[popts->irstmap].cbStruct = sizeof(JET_RSTMAP2_W);
                    popts->prstmap[popts->irstmap].szDatabaseName = NULL;
                    popts->prstmap[popts->irstmap].szNewDatabaseName = NULL;
                    popts->prstmap[popts->irstmap].rgsetdbparam = NULL;
                    popts->prstmap[popts->irstmap].csetdbparam = 0;
                    popts->prstmap[popts->irstmap].grbit = 0;

                    fResult = FEDBUTLParseDoublePath(   arg+2,
                                                        &(popts->prstmap[popts->irstmap].szNewDatabaseName),
                                                        &(popts->prstmap[popts->irstmap].szDatabaseName),
                                                        L"database location",
                                                        fTrue );

                    if ( fResult && L'\0' == popts->prstmap[popts->irstmap].szNewDatabaseName )
                        popts->prstmap[popts->irstmap].szNewDatabaseName = (WCHAR *)wszCurrDir;

                    if ( fResult )
                    {
                        popts->irstmap++;
                    }
                }
            }

            break;

        case L'v':
        case L'V':
            UTILOPTSSetVerbose( popts->fUTILOPTSFlags );
            break;

        case L'c':
            if ( arg[2] == L'5' )
            {
                popts->cLogDigits = 5;
            }
            else if ( arg[2] == L'8' )
            {
                popts->cLogDigits = 8;
            }
            break;

        case L'k':
        case L'K':
            if ( popts->fRunLeakReclaimer )
            {
                fResult = fFalse;
                break;
            }
            popts->grbitShrink = JET_bitShrinkDatabaseEofOnAttach;
            if ( arg[2] != L'\0' )
            {
                if ( ( ( arg[2] == L'f' ) || ( arg[2] == L'F' ) ) && ( arg[3] == L'\0' ) )
                {
                    popts->grbitShrink |= JET_bitShrinkDatabaseFullCategorizationOnAttach;
                    arg++;
                }
                else
                {
                    fResult = fFalse;
                    break;
                }
            }
            fResult = FEDBUTLParsePath( arg + 2, &popts->wszSourceDB, L"database to shrink" );
            break;

        case L'p':
        case L'P':
            if ( popts->grbitShrink != 0 )
            {
                fResult = fFalse;
                break;
            }
            popts->fRunLeakReclaimer = fTrue;
            fResult = FEDBUTLParsePath( arg + 2, &popts->wszSourceDB, L"database to run space leakage reclaimer on" );
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL BOOL FEDBUTLParseIntegrity( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL        fResult = fTrue;

    switch( arg[1] )
    {
        case L'b':
        case L'B':
            UTILOPTSSetDontBuildIndexes( popts->fUTILOPTSFlags );
            break;

        case L'n':
        case L'N':
            UTILOPTSSetDumpStats( popts->fUTILOPTSFlags );
            break;

        case L't':
        case L'T':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszTempDB, L"temporary database" );
            break;

        case L'f':
        case L'F':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszIntegPrefix, L"report file name prefix" );
            break;

        case 'e':
        case 'E':
            fResult = fFalse;
            if ( FEseutilIsEfvArg( arg ) ) //   /efvX
            {
                fResult = fTrue;
                if ( ErrEseutilScanEfv( arg + g_cchEfvOptionPrefix, &popts->efvUserSpecified ) < JET_errSuccess )
                {
                    wprintf( L"%s%c%c", wszUsageErr20, wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
            }
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL BOOL FEDBUTLParseChecksum( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL        fResult = fTrue;

    switch( arg[1] )
    {
        case L'e':
        case L'E':
            UTILOPTSResetChecksumEDB( popts->fUTILOPTSFlags );
            break;

        case L'i':
        case L'I':
            //  APPCOMPAT:  DPM:  ignore this deprecated flag rather than tripping a usage error
            break;

        case L't':
        case L'T':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszTempDB, L"temporary database" );
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL BOOL FEDBUTLParseRepair( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fTrue;

    switch( arg[1] )
    {
        case L't':
        case L'T':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszTempDB, L"temporary database" );
            break;

        case L'n':
        case L'N':
            UTILOPTSSetDumpStats( popts->fUTILOPTSFlags );
            break;

        case L'f':
        case L'F':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszIntegPrefix, L"report file name prefix" );
            break;

        case L'g':
        case L'G':
            UTILOPTSSetRepairCheckOnly( popts->fUTILOPTSFlags );
            break;

        case 'e':
        case 'E':
            fResult = fFalse;
            if ( FEseutilIsEfvArg( arg ) ) //   /efvX
            {
                fResult = fTrue;
                if ( ErrEseutilScanEfv( arg + g_cchEfvOptionPrefix, &popts->efvUserSpecified ) < JET_errSuccess )
                {
                    wprintf( L"%s%c%c", wszUsageErr20, wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
            }
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL VOID EDBUTLGetBaseName( _In_ PCWSTR const wszLogfile, __out_ecount(4) WCHAR * const wszBaseName )
{
    WCHAR   wszNameT[_MAX_FNAME+1];

    assert( wszBaseName != NULL );

    _wsplitpath_s( wszLogfile, NULL, 0, NULL, 0, wszNameT, _countof(wszNameT), NULL, 0);
    StringCbCopyW( wszBaseName, 4 * sizeof(WCHAR), wszNameT );
}

LOCAL BOOL FEDBUTLBaseNameOnly( const WCHAR * const wszName )
{
    WCHAR   wszFileT[_MAX_FNAME+1];
    WCHAR   wszExtT[_MAX_EXT+1];

    if ( NULL == wszName )
        return fFalse;

    _wsplitpath_s( wszName, NULL, 0, NULL, 0, wszFileT, sizeof( wszFileT ) / sizeof( wszFileT[0] ), wszExtT, sizeof( wszExtT ) / sizeof( wszExtT[0] ) );
    return ( 3 == LOSStrLengthW( wszFileT ) && 0 == LOSStrLengthW( wszExtT ) );
}

LOCAL BOOL FEDBUTLParseDump( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fFalse;

    JET_DBUTIL_W * pdbutil;
    pdbutil = reinterpret_cast<JET_DBUTIL_W *>( popts->pv );

    switch( arg[1] )
    {
        case L'a':
        case L'A':
            if (pdbutil->op != opDBUTILDumpData)
                fResult = fFalse;
            else
            {
                pdbutil->grbitOptions |= JET_bitDBUtilOptionAllNodes;
                fResult = fTrue;
            }
            break;

        case L'v':
        case L'V':
            UTILOPTSSetVerbose( popts->fUTILOPTSFlags );
            if ( opDBUTILDumpSpace == pdbutil->op )
            {
                fResult = fTrue;
                if( JET_errSuccess != ErrSpaceDumpCtxSetOptions(
                            pdbutil->pvCallback,
                            NULL, NULL, NULL, fSPDumpPrintSmallTrees ) )
                {
                    fResult = fFalse;   // whoops problem.
                }
            }
            else
            {
                pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpVerboseLevel1;

                if ( arg[2] == L'\0' /* default is /v2 */ || arg[2] == L'2' )
                {
                    pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpVerboseLevel2;
                }
                else if ( arg[2] != L'1' )
                {
                    //  unknown verbosity level
                    fResult = fFalse;
                    break;
                }
            }
            fResult = fTrue;
            break;

        case L's':
        case L'S':
            if ( opDBUTILDumpLogfile == pdbutil->op )
            {
                pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpLogSummary;
            }

            fResult = fTrue;
            break;

        case L't':
        case L'T':
            switch ( pdbutil->op )
            {
                case opDBUTILDumpData:
                case opDBUTILDumpMetaData:
                    pdbutil->szTable = arg+2;
                    fResult = fTrue;
                    break;
                case opDBUTILDumpSpace:
                    pdbutil->szTable = arg+2;
                    JET_ERR errS;
                    errS = ErrSpaceDumpCtxSetOptions( pdbutil->pvCallback, NULL, NULL, NULL,
                                        SPDUMPOPTS( fSPDumpPrintSmallTrees | fSPDumpSelectOneTable ) );
                    assert( JET_errSuccess == errS );
                    fResult = fTrue;
                    break;
                default:
                    fResult = fFalse;
            }
            break;

        case L'f':
        case L'F':
            if ( opDBUTILDumpSpace == pdbutil->op )
            {
                assert( pdbutil->pfnCallback == EseutilEvalBTreeData );
                assert( pdbutil->pvCallback );
                fResult = fTrue;
                if ( ErrSpaceDumpCtxSetFields( pdbutil->pvCallback, arg+2 ) < JET_errSuccess )
                {
                    fResult = fFalse;   // ErrSpaceDumpCtxSetFields() printed error.
                }
            }
            else if ( opDBUTILDumpSpaceCategory == pdbutil->op )
            {
                pdbutil->grbitOptions |= JET_bitDBUtilFullCategorization;
                fResult = fTrue;
            }
            break;

        case L'p':
        case L'P':
            if ( ( pdbutil->op != opDBUTILDumpFlushMapFile ) && ( pdbutil->op != opDBUTILDumpSpaceCategory ) && ( pdbutil->op != opDBUTILDumpRBSHeader ) )
            {
                pdbutil->op = opDBUTILDumpPage;
            }

            assert( LOSStrLengthW(arg) >= 2 );

            fResult = fTrue;
            if ( pdbutil->op != opDBUTILDumpSpaceCategory && pdbutil->op != opDBUTILDumpRBSHeader ) 
            {
                // The jet api is responsible for validating pgno because the
                // pgno range is not exported. But fyi:
                //             -1 = header
                //              0 = shadow header
                //     1..7fffffe = valid page numbers
                //
                // wcstol returns LONG _MIN..LONG _MAX (even if overflow)
                pdbutil->pgno = wcstol( arg + 2, NULL, 0 );
            }
            else if ( pdbutil->op == opDBUTILDumpRBSHeader ) 
            {
                pdbutil->op = opDBUTILDumpRBSPages;
                pdbutil->rbsOptions.pgnoFirst = 1;
                pdbutil->rbsOptions.pgnoLast = ulMax;

                fResult = fFalse;
                
                // opDBUTILDumpSpaceCategory can take a range.
                const SIZE_T cchArg = LOSStrLengthW( arg + 2 );
                LONG pgnoFirst, pgnoLast;

                if ( arg[2] == '*' )
                {
                    fResult = fTrue;
                }
                else if ( wcsstr( arg + 2, L"-max" ) != NULL )
                {
                    if ( ( _snwscanf_s( arg + 2, cchArg, L"%d", &pgnoFirst ) == 1 ) && ( pgnoFirst >= 1 ) )
                    {
                        fResult = fTrue;
                        pdbutil->rbsOptions.pgnoFirst = (ULONG)pgnoFirst;
                        pdbutil->rbsOptions.pgnoLast = ulMax;
                    }
                }
                else if ( _snwscanf_s( arg + 2, cchArg, L"%d-%d", &pgnoFirst, &pgnoLast ) == 2 )
                {
                    if ( ( pgnoFirst >= 1 ) && ( pgnoLast >= 1 ) )
                    {
                        fResult = fTrue;
                        pdbutil->rbsOptions.pgnoFirst = (ULONG)pgnoFirst;
                        pdbutil->rbsOptions.pgnoLast = (ULONG)pgnoLast;
                    }
                }
                else if ( ( _snwscanf_s( arg + 2, cchArg, L"%d", &pgnoFirst ) == 1 ) && ( pgnoFirst >= 1 ) )
                {
                    fResult = fTrue;
                    pdbutil->rbsOptions.pgnoFirst = (ULONG)pgnoFirst;
                    pdbutil->rbsOptions.pgnoLast = (ULONG)pgnoFirst;
                }
            }
            else
            {
                 fResult = fFalse;
                
                // opDBUTILDumpSpaceCategory can take a range.
                const SIZE_T cchArg = LOSStrLengthW( arg + 2 );
                LONG pgnoFirst, pgnoLast;
                if ( wcsstr( arg + 2, L":max" ) != NULL )
                {
                    if ( ( _snwscanf_s( arg + 2, cchArg, L"%d", &pgnoFirst ) == 1 ) && ( pgnoFirst >= 1 ) )
                    {
                        fResult = fTrue;
                        pdbutil->spcatOptions.pgnoFirst = (ULONG)pgnoFirst;
                        pdbutil->spcatOptions.pgnoLast = ulMax;
                    }
                }
                else if ( _snwscanf_s( arg + 2, cchArg, L"%d:%d", &pgnoFirst, &pgnoLast ) == 2 )
                {
                    if ( ( pgnoFirst >= 1 ) && ( pgnoLast >= 1 ) )
                    {
                        fResult = fTrue;
                        pdbutil->spcatOptions.pgnoFirst = (ULONG)pgnoFirst;
                        pdbutil->spcatOptions.pgnoLast = (ULONG)pgnoLast;
                    }
                }
                else if ( ( _snwscanf_s( arg + 2, cchArg, L"%d", &pgnoFirst ) == 1 ) && ( pgnoFirst >= 1 ) )
                {
                    fResult = fTrue;
                    pdbutil->spcatOptions.pgnoFirst = (ULONG)pgnoFirst;
                    pdbutil->spcatOptions.pgnoLast = (ULONG)pgnoFirst;
                }
            }
            break;

        case L'k':
        case L'K':
            //  MUST specify /p before /k and /k before /d
            //
            if ( opDBUTILDumpPage == pdbutil->op
                && NULL == pdbutil->szIndex
                && NULL == pdbutil->szTable )
            {
                //  overload szIndex
                //
                pdbutil->szIndex = arg+2;
                fResult = fTrue;
            }
            else
            {
                wprintf( wszUsageErr4, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            break;

        case L'd':
        case L'D':
            //  MUST specify /p and /k before /d
            //
            if ( opDBUTILDumpPage == pdbutil->op
                && NULL != pdbutil->szIndex
                && NULL == pdbutil->szTable )
            {
                //  overload szTable
                //
                pdbutil->szTable = arg+2;
                fResult = fTrue;
            }
            else if ( opDBUTILDumpSpace == pdbutil->op )
            {
                UTILOPTSSetDebugMode( popts->fUTILOPTSFlags );
                SPDUMPOPTS fSPDumpOpts;
                if ( arg[2] == L'\0' /* default is /d1 */ || arg[2] == L'1' )
                {
                    fSPDumpOpts = fSPDumpPrintSpaceTrees;
                }
                else if ( arg[2] == L'2' )  //  /d2 - adds space nodes
                {
                    fSPDumpOpts = SPDUMPOPTS( fSPDumpPrintSpaceTrees | fSPDumpPrintSpaceNodes );
                }
                else
                {
                    //  unknown debug level
                    fResult = fFalse;
                    break;
                }
                fResult = fTrue;
                if( JET_errSuccess != ErrSpaceDumpCtxSetOptions(
                            pdbutil->pvCallback,
                            NULL, NULL, NULL, fSPDumpOpts ) )
                {
                    fResult = fFalse;   // whoops problem.
                }
            }
            else
            {
                wprintf( wszUsageErr4, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            break;

        case L'n':
        case L'N':
        {
            const size_t cchSzNode = 256;
            WCHAR   wszNode[cchSzNode];
            StringCbCopyW( wszNode, sizeof(wszNode), arg+2 );

            INT     dbid    = 0;
            INT     pgno    = 0;
            INT     iline   = 0;
            if(
                _snwscanf_s( wszNode, cchSzNode, L"[%d:%d:%d]", &dbid, &pgno, &iline ) != 3
                && _snwscanf_s( wszNode, cchSzNode, L"%d:%d:%d", &dbid, &pgno, &iline ) != 3
                && _snwscanf_s( wszNode, cchSzNode, L"[%d:%d]", &pgno, &iline ) != 2
                && _snwscanf_s( wszNode, cchSzNode, L"%d:%d", &pgno, &iline ) != 2
                )
            {
                wprintf( L"%s (2)", wszUsageErr13 );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
                break;
            }

            pdbutil->op     = opDBUTILDumpNode;
            pdbutil->dbid   = dbid;
            pdbutil->pgno   = pgno;
            pdbutil->iline  = iline;

            fResult         = fTrue;
        }
            break;

        case L'l':
        case L'L':
            if ( opDBUTILDumpSpace == pdbutil->op )
            {
                fResult = fTrue;
                if( JET_errSuccess != ErrSpaceDumpCtxSetOptions(
                            pdbutil->pvCallback,
                            NULL, NULL, NULL, fSPDumpPrintSpaceLeaks ) )
                {
                    fResult = fFalse;   // whoops problem.
                }
            }
            break;


        case L'c':
        case L'C':
            if ( opDBUTILDumpSpace == pdbutil->op )
            {
                fResult = fTrue;
                if( JET_errSuccess != ErrSpaceDumpCtxSetOptions(
                            pdbutil->pvCallback,
                            NULL, NULL, L",", fSPDumpNoOpts ) )
                {
                    fResult = fFalse;   // whoops problem.
                }
                //  For now, we'll not support separate CSV file.
            }
            else
            {
                pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpLogInfoCSV;
                fResult = FEDBUTLParsePath( arg+2, &(pdbutil->szIntegPrefix), L"CSV Output Data File", fFalse );
            }
            break;

        case L'x':
        case L'X':
            pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpLogPermitPatching;
            fResult = fTrue;
            break;

        case L'r':
        case L'R':
            // This option permits specifying a log generation range
            // when used in log-dumper mode.
            if ( opDBUTILDumpLogfile == pdbutil->op )
            {
                const size_t    cchSzLogRange   = 256;
                WCHAR           wszLogRange[cchSzLogRange];
                StringCbCopyW( wszLogRange, sizeof( wszLogRange ), arg + 2 );
                
                LONG    lgenStart   = 0;
                LONG    lgenEnd     = 0;

                if ( _snwscanf_s( wszLogRange, cchSzLogRange, L"%x-%x", &lgenStart, &lgenEnd ) != 2
                    && _snwscanf_s( wszLogRange, cchSzLogRange, L"%x", &lgenStart ) != 1 )
                {
                    wprintf( L"%s%c%c", wszUsageErr22, wchNewLine, wchNewLine );
                    break;
                }
                else if ( lgenStart < 0
                    || ( lgenEnd != 0 && lgenEnd < lgenStart ) )
                {
                    wprintf( L"%s%c%c", wszUsageErr22, wchNewLine, wchNewLine );
                    break;
                }

                // lGeneration is used for the starting generation and isec is
                // overloaded with the ending generation.
                pdbutil->lGeneration    = lgenStart;
                pdbutil->isec           = lgenEnd;

                fResult = true;
            }
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL BOOL FEDBUTLParseHardRecovery( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fTrue;

    switch( arg[1] )
    {
        case L'k':
        case L'K':
            if ( !FUTILOPTSDumpRestoreEnv(popts->fUTILOPTSFlags) )
            {
                UTILOPTSSetPreserveTempDB( popts->fUTILOPTSFlags );
                break;
            }
            // no /t allowed if dump mode specified
        case L't':
        case L'T':
            if ( !FUTILOPTSDumpRestoreEnv(popts->fUTILOPTSFlags) )
            {
                if ( !popts->wszLogfilePath )
                {
                    // allow NULL target name. It means : no play forward
                    fResult = FEDBUTLParsePath( arg+2, &popts->wszRestore, L"target instance", fTrue );
                }
                else // don't allow /f and /t
                {
                    wprintf( wszUsageErr4, arg );
                    wprintf( L"%c%c", wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
                break;
            }
            // no /t allowed if dump mode specified
        case L'f':
        case L'F':
            if ( !FUTILOPTSDumpRestoreEnv(popts->fUTILOPTSFlags) )
            {
                if ( !popts->wszRestore )
                {
                    fResult = FEDBUTLParsePath( arg+2, &popts->wszLogfilePath, L"logfile path", fFalse );
                }
                else // don't allow /f and /t
                {
                    wprintf( wszUsageErr4, arg );
                    wprintf( L"%c%c", wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
                break;
            }
            // no /t allowed if dump mode specified
        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

#ifdef DEBUG

LOCAL BOOL FEDBUTLParseBackup( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fFalse;   // backup directory must be set at least.

    switch( arg[1] )
    {
        case L'l':
        case L'L':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszLogfilePath, L"logfile path" );
            break;

        case L's':
        case L'S':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszSystemPath, L"system path" );
            break;

        case L'c':
        case L'C':
            UTILOPTSSetIncrBackup( popts->fUTILOPTSFlags );
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

#endif  //  DEBUG

LOCAL BOOL FEDBUTLParseScrub( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fTrue;

    switch( arg[1] )
    {
        case L't':
        case L'T':
            UTILOPTSSetTrimDatabase( popts );
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

LOCAL VOID EDBUTLGetUnpathedFilename( _In_ PCWSTR const wszFilename, __in_bcount(cbUnpathedFilename) PWSTR const wszUnpathedFilename, ULONG cbUnpathedFilename)
{
    WCHAR   wszFile[_MAX_FNAME+1];
    WCHAR   wszExt[_MAX_EXT+1];

    _wsplitpath_s( wszFilename, NULL, 0, NULL, 0, wszFile, sizeof( wszFile ) / sizeof( wszFile[0] ), wszExt, sizeof( wszExt ) / sizeof( wszExt[0] ) );
    _wmakepath_s( wszUnpathedFilename, cbUnpathedFilename / sizeof(WCHAR), NULL, NULL, wszFile, wszExt );
}

LOCAL BOOL FEDBUTLParseCopyFile( _In_ PCWSTR arg, UTILOPTS *popts )
{
    BOOL        fResult = fTrue;

    switch( arg[1] )
    {
        case L'd':
        case L'D':
            fResult = FEDBUTLParsePath( arg+2, &popts->wszTempDB, L"destination file" );
            break;

        case L'i':
        case L'I':
            popts->fUTILOPTSCopyFileIgnoreIoErrors = TRUE;
            break;

        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}



LOCAL WCHAR **g_argv = NULL;
LOCAL INT   g_argMaxID = 0;
LOCAL INT   g_argCurID = -1;

//  initalizes argument index, and set to point before firts index
LOCAL VOID InitArg( INT argc, __in_ecount(argc) LPWSTR argv[] )
{
    AssertPREFIX( argc > 0 );
    AssertPREFIX( NULL != argv );

    g_argMaxID = argc;
    g_argv = argv;
    g_argCurID = -1;
}

//  returns current argument index
//  returns -1 if is before first argument
//  return  ArgCount after last argument
LOCAL INT GetCurArgID()
{
    AssertPREFIX( g_argCurID >= -1 );
    return g_argCurID;
}

LOCAL INT GetArgCount()
{
    AssertPREFIX( g_argMaxID >= 0 );
    return g_argMaxID;
}

LOCAL VOID SetCurArgID( const INT id )
{
    if ( id < -1 )
    {
        g_argCurID = -1;
    }
    else if ( id > g_argMaxID )
    {
        AssertPREFIX( g_argMaxID >= 0 );
        g_argCurID = g_argMaxID;
    }
    else
    {
        g_argCurID = id;
    }
}

LOCAL WCHAR *GetCurArg()
{
    AssertPREFIX( g_argCurID >= -1 );
    AssertPREFIX( g_argMaxID >= 0 );
    AssertPREFIX( g_argMaxID >= g_argCurID );
    if ( -1 == g_argCurID || g_argMaxID == g_argCurID )
    {
        return NULL;
    }
    return g_argv[g_argCurID];
}

LOCAL WCHAR *GetNextArg()
{
    SetCurArgID( GetCurArgID()+1 );
    return GetCurArg();
}

LOCAL_BROKEN WCHAR *GetPrevArg()
{
    SetCurArgID( GetCurArgID()-1 );
    return GetCurArg();
}

LOCAL BOOL FSetDumpModeModifier( WCHAR chMode, JET_DBUTIL_W *pdbutil )
{
    switch( chMode )
    {
        case 0:
        case L'h':
        case L'H':
            pdbutil->op = opDBUTILDumpHeader;
            break;

        case L't':
        case L'T':
            pdbutil->op = opDBUTILDumpFTLHeader;
            break;

        case L'k':
        case L'K':
            pdbutil->op = opDBUTILDumpCheckpoint;
            break;

        case L'm':
        case L'M':
            pdbutil->op = opDBUTILDumpMetaData;
            break;

#ifdef DEBUG
        case L'f':
        case L'F':
            pdbutil->op = opDBUTILSetHeaderState;
            break;
#endif // DEBUG

        case L'n':
        case L'N':
            pdbutil->op = opDBUTILDumpData;
            break;

        case L'l':
        case L'L':
            pdbutil->op = opDBUTILDumpLogfile;
            break;

        case L'p':
        case L'P':
            pdbutil->op = opDBUTILDumpFlushMapFile;
            pdbutil->pgno = -1; // default to dumping header.
            break;

        case L's':
        case L'S':
            pdbutil->op = opDBUTILDumpSpace;
            pdbutil->pfnCallback = EseutilEvalBTreeData;
            if ( ErrSpaceDumpCtxInit( &(pdbutil->pvCallback) ) < JET_errSuccess )
            {
                return fFalse;  //ErrSpaceDumpCtxInit() printed error.
            }
            
            break;

        case L'c':
        case L'C':
            pdbutil->op = opDBUTILDumpSpaceCategory;
            pdbutil->spcatOptions.pfnSpaceCatCallback = EseutilDumpSpaceCat;
            pdbutil->spcatOptions.pvContext = pdbutil;
            pdbutil->spcatOptions.pgnoFirst = 1;
            pdbutil->spcatOptions.pgnoLast = ulMax;
            break;

        case L'b':
        case L'B':
            pdbutil->op = opDBUTILDumpCacheFile;
            break;

        case L'r':
        case L'R':
            // By default dump RBS header. If /p is specified we will dump specific pages.
            pdbutil->op = opDBUTILDumpRBSHeader;
            break;
        default:
            return fFalse;
    }
        return fTrue;
}

LOCAL BOOL FSetHardRecoveryModeModifier( WCHAR chMode, UTILOPTS *popts )
{
    switch( chMode )
    {
        case L'c':
        case L'C':
            break;
        case L'm':
        case L'M':
            UTILOPTSSetDumpRestoreEnv( popts->fUTILOPTSFlags );
            break;
#ifdef RESTORE_SERVER_SIMULATION
        case L's':
        case L'S':
            UTILOPTSSetServerSim ( popts->fUTILOPTSFlags );
            break;
#endif // RESTORE_SERVER_SIMULATION
        default:
            return fFalse;
    }
    return fTrue;
}

#ifdef ESESHADOW
LOCAL BOOL FVssSupported( const UTILOPTS* const popts )
{
    if ( popts->mode == modeChecksum ||
            popts->mode == modeCopyFile ||
            popts->mode == modeDump )
{
        return fTrue;
}

    return fFalse;
}

LOCAL BOOL FVssRecSupported( const UTILOPTS* const popts )
{
    if ( popts->mode == modeIntegrity ||
            popts->mode == modeChecksum ||
            popts->mode == modeCopyFile ||
            popts->mode == modeDump )
{
        return fTrue;
}

    return fFalse;
}

LOCAL BOOL FVssSystemPathSupported( const UTILOPTS* const popts )
{
    return FVssRecSupported( popts );
}

LOCAL BOOL FVssPauseSupported( const UTILOPTS* const popts )
{
    if ( popts->mode == modeDump )
{
        return fTrue;
}

    return fFalse;
}
#endif

LOCAL BOOL FEDBUTLParseOptions(
    UTILOPTS    *popts,
    BOOL        (*pFEDBUTLParseMode)( _In_ PCWSTR arg, UTILOPTS *popts ) )
{
    BOOL        fResult = fTrue;
    WCHAR       *arg = GetCurArg();
    INT         iSkipID = -1; // argument ID to skip. Related to dump and hard recovery hacks

    AssertPREFIX( NULL != arg );
    assert( NULL != arg );
    assert( NULL != wcschr( wszSwitches, *arg ) );
    assert( '\0' != *arg + 1 );

    //  HACK for dump
    if ( modeDump == popts->mode )
    {
        // when we have only one char after mode it is mode modifier
        if ( L'\0' != arg[2] && L'\0' == arg[3] )
        {
            fResult = FSetDumpModeModifier( arg[2], (JET_DBUTIL_W *)popts->pv );
            arg++;
        }
        else    // run search to set mode specifier
        {
            INT curid = GetCurArgID();
            WCHAR *argT;
            fResult = fFalse;
            while ( !fResult && NULL != ( argT = GetNextArg() ) )
            {
                if ( NULL != wcschr( wszSwitches, *argT ) && L'\0' != argT[1] && L'\0' == argT[2] )
                {
                    fResult = FSetDumpModeModifier( argT[1], (JET_DBUTIL_W *)popts->pv );
                }
            }
            if ( !fResult )
            {
                fResult = FSetDumpModeModifier( L'\0', (JET_DBUTIL_W *)popts->pv );
            }
            iSkipID = GetCurArgID();
            SetCurArgID( curid );
            assert( GetCurArgID() == curid );
        }
    }
    //  HACK for hard recovery
    else if ( modeHardRecovery == popts->mode )
    {
        // when we have only one char after mode it is mode modifier
        if ( L'\0' != arg[2] && L'\0' == arg[3] )
        {
            fResult = FSetHardRecoveryModeModifier( arg[2], popts );
            arg ++; // ignore mode modifier
        }
        else    // run search to set mode specifier
        {
            INT curid = GetCurArgID();
            WCHAR *argT;
            fResult = fFalse;
            while ( !fResult && NULL != ( argT = GetNextArg() ) )
            {
                if ( NULL != wcschr( wszSwitches, *argT ) && L'\0' != argT[1] && L'\0' == argT[2] )
                {
                    fResult = FSetHardRecoveryModeModifier( argT[1], popts );
                }
            }
            iSkipID = GetCurArgID();
            SetCurArgID( curid );
            assert( GetCurArgID() == curid );
        }
    }

    if ( !fResult )
    {
        wprintf( L"%s%c%c", wszUsageErr12, wchNewLine, wchNewLine );
    }

    arg += 2;
    if ( L'\0' == *arg )
    {
        arg = GetNextArg();
    }

    // First option specifies the mode, so start with the second option.
    for ( ; fResult && NULL != arg; arg = GetNextArg() )
    {
        //  dump and hard recovery hack
        if ( GetCurArgID() == iSkipID )
        {
            continue;
        }

        if ( wcschr( wszSwitches, arg[0] ) == NULL )
        {
            // SPECIAL CASE: Backup mode does not DB specification.
            switch ( popts->mode )
            {
                case modeRecovery:
                    if ( fResult = ( NULL == popts->wszBase ) )
                    {
                        popts->wszBase = arg;
                    }
                    break;
                case modeBackup:
                    if ( fResult = ( NULL == popts->wszBackup ) )
                    {
                        popts->wszBackup = arg;
                    }
                    break;
                default:
                    if ( fResult = ( NULL == popts->wszSourceDB ) )
                    {
                        popts->wszSourceDB = arg;
                    }
                    break;
            }

            if ( !fResult )
            {
                wprintf( wszUsageErr5, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
            }
        }
#ifdef ESESHADOW
        else if ( 0 == _wcsicmp( &(arg[1]), L"vss" ) )
        {
            if ( !FVssSupported( popts ) )
            {
                wprintf( wszUsageErr4, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                UTILOPTSSetOperateTempVss( popts->fUTILOPTSFlags );
            }
        }
        else if ( 0 == _wcsicmp( &(arg[1]), L"vssrec" ) )
        {
            if ( !FVssRecSupported( popts ) )
            {
                wprintf( wszUsageErr4, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                UTILOPTSSetOperateTempVss( popts->fUTILOPTSFlags );
                UTILOPTSSetOperateTempVssRec( popts->fUTILOPTSFlags );
                popts->wszBase = GetNextArg();

                //  if this is passed in as an empty string, consider it NULL so we don't fail
                //  when setting the system parameters and are still able to take a snapshot
                //  without actually replaying the logs.
                if ( popts->wszBase != NULL && popts->wszBase[0] == '\0' )
                {
                    popts->wszBase = NULL;
                }

                popts->wszLogfilePath = GetNextArg();
                if ( !popts->wszLogfilePath )
                {
                    // Default to the current working directory. This allows "-vssrec edb" to work,
                    // instead of requiring "-vssrec edb e:\bla".
                    popts->wszLogfilePath = L".";
                }
            }
        }
        else if ( 0 == _wcsicmp( &(arg[1]), L"vsssystempath" ) )
        {
            if ( !FVssSystemPathSupported( popts ) )
            {
                wprintf( wszUsageErr4, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                popts->wszSystemPath = GetNextArg();
            }
        }
        else if ( 0 == _wcsicmp( &(arg[1]), L"vsspause" ) )
        {
            if ( !FVssPauseSupported( popts ) )
            {
                wprintf( wszUsageErr4, arg );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                fResult = fFalse;
            }
            else
            {
                UTILOPTSSetOperateTempVssPause( popts->fUTILOPTSFlags );
            }
        }
#endif
        else if ( 0 == _wcsnicmp( &(arg[1]), wszConfigArgPrefix, LOSStrLengthW( wszConfigArgPrefix ) ) )   // /config -config /config: -config:
        {
            const WCHAR * wszConfigSpec = &( arg[ 1 + LOSStrLengthW( wszConfigArgPrefix ) ] );
            if ( wszConfigSpec[0] == L':' )
            {
                //  inline arg, increment past ":"
                wszConfigSpec++;
                fResult = ( wszConfigSpec[0] != L'\0' );
            }
            else
            {
                //  or spec is next arg
                wszConfigSpec = GetNextArg();
                fResult = ( wszConfigSpec != NULL && wszConfigSpec[0] != L'\0' );
            }
            if ( fResult )
            {
                popts->wszConfigStoreSpec = wszConfigSpec;
            }
        }
        else
        {
            // Parse options common to all modes.  Pass off unique options to the
            // custom parsers.
            switch ( arg[1] )
            {
                case L'2':
                    UTILOPTSSet2KPage( popts->fUTILOPTSFlags );
                    break;
                    
                case L'4':
                    UTILOPTSSet4KPage( popts->fUTILOPTSFlags );
                    break;
                    
                case L'8':
                    UTILOPTSSet8KPage( popts->fUTILOPTSFlags );
                    break;

                case L'1':
                    UTILOPTSSet16KPage( popts->fUTILOPTSFlags );
                    break;

                case L'3':
                    UTILOPTSSet32KPage( popts->fUTILOPTSFlags );
                    break;

                case L'o':
                case L'O':
                    UTILOPTSSetSuppressLogo( popts->fUTILOPTSFlags );
                    break;

                case L'!':
                    //  logfile size param no longer needed (Jet now
                    //  auto-detects logfile size when needed)
                    break;

                default:
                    if ( pFEDBUTLParseMode )
                    {
                        fResult = (*pFEDBUTLParseMode)( arg, popts );
                    }
                    else
                    {
                        wprintf( wszUsageErr4, arg );
                        wprintf( L"%c%c", wchNewLine, wchNewLine );
                        fResult = fFalse;
                    }
                    break;
            }
        }
    }

    return fResult;
}


//  ================================================================
LOCAL JET_ERR ErrEDBUTLRepair(
    const JET_SESID sesid,
    const WCHAR * const wszDatabase,
    const WCHAR * const wszBackup,
    const WCHAR * const wszTable,
    const WCHAR * const wszIntegPrefix,
    const JET_PFNSTATUS pfnStatus,
    const JET_GRBIT grbit
    )
//  ================================================================
{
    JET_DBUTIL_W dbutil;
    memset( &dbutil, 0, sizeof( dbutil ) );

    dbutil.cbStruct         = sizeof( JET_DBUTIL_W );
    dbutil.op               = opDBUTILEDBRepair;
    dbutil.szDatabase       = const_cast<WCHAR *>( wszDatabase );
    dbutil.szBackup         = const_cast<WCHAR *>( wszBackup );
    dbutil.szTable          = const_cast<WCHAR *>( wszTable );
    dbutil.szIntegPrefix    = const_cast<WCHAR *>( wszIntegPrefix );
    dbutil.sesid            = sesid;
    dbutil.grbitOptions     = grbit;
    dbutil.pfnCallback      = pfnStatus;

    const JET_ERR err = JetDBUtilitiesW( &dbutil );
    return err;
}


//  ================================================================
LOCAL JET_ERR ErrEDBUTLScrub(
    JET_SESID   sesid,
    const WCHAR * const wszDatabase,
    JET_PFNSTATUS pfnStatus,
    JET_GRBIT grbit
    )
//  ================================================================
{
    JET_DBUTIL_W dbutil;
    memset( &dbutil, 0, sizeof( dbutil ) );

    dbutil.cbStruct         = sizeof( JET_DBUTIL_W );
    dbutil.op               = opDBUTILEDBScrub;
    dbutil.szDatabase       = const_cast<WCHAR *>( wszDatabase );
    dbutil.sesid            = sesid;
    dbutil.grbitOptions     = grbit;
    dbutil.pfnCallback      = pfnStatus;

    const JET_ERR err = JetDBUtilitiesW( &dbutil );
    return err;
}


//  callback function used by CopyFileEx
LOCAL DWORD CALLBACK DwEDBUTILCopyProgressRoutine(
    LARGE_INTEGER   cTotalFileSize,          // file size
    LARGE_INTEGER   cTotalBytesTransferred,  // bytes transferred
    LARGE_INTEGER   cStreamSize,             // bytes in stream
    LARGE_INTEGER   cStreamBytesTransferred, // bytes transferred for stream
    DWORD           dwStreamNumber,         // current stream
    DWORD           dwCallbackReason,       // callback reason
    HANDLE          hSourceFile,            // handle to source file
    HANDLE          hDestinationFile,       // handle to destination file
    INT             *pcShift                // from CopyFileEx. How much to shift file size to fit in INT value
)
{
    enum { LITOI_LOW = 1, LITOI_HIGH, LITOI_SHIFT };
    //  zero sized file?
    if ( 0 == cTotalFileSize.QuadPart )
    {
        return PROGRESS_CONTINUE;
    }
    JET_SNPROG snpprog;
    assert( NULL != pcShift );
    if ( -1 == *pcShift )
    {
        PrintStatus( 0, JET_SNP( -1 ), JET_sntBegin, (void *)wszCopyFileStatus );
        assert( sizeof( ULONG ) == sizeof( INT ) );
        assert( sizeof( LARGE_INTEGER ) > sizeof( INT ) );
        //  if all data fits in INT value
        if ( 0 == (ULONG)cTotalFileSize.HighPart )
        {
            *pcShift = LITOI_LOW;
        }
        //  if High part can be used as 1% change detector
        else if ( 100 <= (ULONG)cTotalFileSize.HighPart )
        {
            *pcShift = LITOI_HIGH;
        }
        else
        {
            *pcShift = LITOI_SHIFT;
        }
    }
    snpprog.cbStruct = sizeof( snpprog );
    switch ( *pcShift )
    {
        //  if all data fits in INT value
        case LITOI_LOW:
            assert( 0 == cTotalFileSize.HighPart );
            snpprog.cunitTotal = cTotalFileSize.LowPart;
            snpprog.cunitDone = cTotalBytesTransferred.LowPart;
            break;
        //  if High part can be used as 1% change detector
        case LITOI_HIGH:
            assert( 100 <= cTotalFileSize.HighPart );
            snpprog.cunitTotal = cTotalFileSize.HighPart;
            snpprog.cunitDone = cTotalBytesTransferred.HighPart;
            break;
        //  if none of the above shift it 7 times because 2^7 > 100 and it will move
        //  all High Part data to Low Part
        case LITOI_SHIFT:
            assert( 100 > cTotalFileSize.HighPart && 0 < cTotalFileSize.HighPart );
            snpprog.cunitTotal = (INT)( cTotalFileSize.QuadPart >> 7 );
            snpprog.cunitDone = (INT)( cTotalBytesTransferred.QuadPart >> 7 );
            break;
        default:
            assert( fFalse );
    }
    if ( cTotalBytesTransferred.QuadPart == cTotalFileSize.QuadPart )
    {
        PrintStatus( 0, JET_SNP( -1 ), JET_sntComplete, &snpprog );
    }
    else
    {
        PrintStatus( 0, JET_SNP( -1 ), JET_sntProgress, &snpprog );
    }
    return PROGRESS_CONTINUE;
}

LOCAL JET_ERR ErrEDBUTLGetFmPathFromDbPath(
    _Out_writes_z_(OSFSAPI_MAX_PATH) WCHAR* const wszFmPath,
    _In_ const WCHAR* const wszDbPath,
    _Out_opt_ BOOL* const pfFmExists = NULL )
{
    JET_ERR err = JET_errSuccess;
    IFileSystemAPI* pfsapi = NULL;
    WCHAR wszDbFolder[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbFileName[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    WCHAR wszDbExtension[ IFileSystemAPI::cchPathMax ] = { L"\0" };
    BOOL fIsDirectory = fFalse;

    Call( ErrOSFSCreate( &pfsapi ) );
    Call( pfsapi->ErrPathParse( wszDbPath, wszDbFolder, wszDbFileName, wszDbExtension ) );
    Call( pfsapi->ErrPathBuild( wszDbFolder, wszDbFileName, L".jfm", wszFmPath ) );

    if ( pfFmExists != NULL )
    {
        *pfFmExists = ( pfsapi->ErrPathExists( wszFmPath, &fIsDirectory ) == JET_errSuccess ) && !fIsDirectory;
    }

HandleError:

    delete pfsapi;
    return err;
}

LOCAL JET_ERR ErrEDBUTLMoveFile(
    const WCHAR * const wszExistingFileName,  // file name
    const WCHAR * const wszNewFileName,       // new file name
    const DWORD dwFlags )
{
    wprintf( wszMoveFile, wszExistingFileName, wszNewFileName );
    if ( !MoveFileExW( wszExistingFileName, wszNewFileName, dwFlags ) )
    {
        DWORD dw = GetLastError();

        if ( ( dwFlags & ~MOVEFILE_REPLACE_EXISTING ) != 0 )
        {
            // unsupported move flag
            assert( fFalse );
            wprintf( L"%s%c", wszMoveFailed, wchNewLine );
            return ErrERRCheck( JET_errFileAccessDenied );
        }

        if ( dw == ERROR_NOT_SAME_DEVICE )
        {
            //  the source file is on a different device -- we must copy it instead
            BOOL        fCancel     = fFalse;
            INT         cShift      = -1;       // used from progress routine
            const BOOL  fSuccess    = CopyFileExW(  wszExistingFileName,
                                                        wszNewFileName,
                                                        LPPROGRESS_ROUTINE( DwEDBUTILCopyProgressRoutine ),
                                                        &cShift,
                                                        &fCancel,
                                                        (   dwFlags & MOVEFILE_REPLACE_EXISTING ?
                                                                0 :
                                                                COPY_FILE_FAIL_IF_EXISTS ) );
            return ( fSuccess ? JET_errSuccess : JET_errFileAccessDenied );
        }

        if ( dw != 0 )
        {
            wprintf( L"%s%c", wszMoveFailed, wchNewLine );
            return ErrERRCheck( JET_errFileAccessDenied );
        }
    }

    wprintf( L"%s%c", wszMoveDone, wchNewLine );
    return JET_errSuccess;
}


LOCAL VOID EDBUTLDeleteTemp( const UTILOPTS * const popts )
{
    assert( NULL != popts->wszTempDB );
    DeleteFileW( popts->wszTempDB );

    WCHAR wszFmFilePath[ IFileSystemAPI::cchPathMax ] = { L'\0' };
    BOOL fFmExists = fFalse;
    if( ( ErrEDBUTLGetFmPathFromDbPath( wszFmFilePath, popts->wszTempDB, &fFmExists ) >= JET_errSuccess ) && fFmExists )
    {
        DeleteFileW( wszFmFilePath );
    }
}

// Backs up source database if required, then copies temporary database over
// source database if required.  Should be called after Jet has terminated.
LOCAL JET_ERR ErrEDBUTLBackupAndInstateDB(
    JET_SESID   sesid,
    UTILOPTS    *popts )
{
    JET_ERR     err = JET_errSuccess;
    WCHAR       wszFmFilePathSrc[ IFileSystemAPI::cchPathMax ] = { L'\0' };
    WCHAR       wszFmFilePathDst[ IFileSystemAPI::cchPathMax ] = { L'\0' };
    BOOL        fFmExists = fFalse;

    assert( popts->wszSourceDB != NULL );
    assert( popts->wszTempDB != NULL );

    //  upgrade from 8.3 to full filename

    WCHAR   wszSourceDB[_MAX_PATH+1];

    WIN32_FIND_DATAW wfd;
    HANDLE          hFind;

    hFind = FindFirstFileW( popts->wszSourceDB, &wfd );
    if( INVALID_HANDLE_VALUE != hFind )
    {
        WCHAR   wszDrive[_MAX_PATH+1];
        WCHAR   wszDir[_MAX_PATH+1];

        _wsplitpath_s( popts->wszSourceDB, wszDrive, _countof(wszDrive), wszDir, _countof(wszDir), NULL, 0, NULL, 0 );
        _wmakepath_s( wszSourceDB, _countof(wszSourceDB), wszDrive, wszDir, NULL, NULL );
        StringCbCatW( wszSourceDB, sizeof(wszSourceDB), wfd.cFileName );
        FindClose( hFind );
    }
    else
    {
        StringCbCopyW( wszSourceDB, sizeof(wszSourceDB), popts->wszSourceDB );
    }

    // Make backup before instating, if requested.

    if ( popts->wszBackup != NULL )
    {
        err = ErrEDBUTLMoveFile( wszSourceDB, popts->wszBackup, 0 );
        if ( err < 0 )
        {
            wprintf( wszErr4, popts->wszBackup );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            Call( err );
        }

        if( ( ErrEDBUTLGetFmPathFromDbPath( wszFmFilePathSrc, wszSourceDB, &fFmExists ) >= JET_errSuccess ) && fFmExists &&
            ( ErrEDBUTLGetFmPathFromDbPath( wszFmFilePathDst, popts->wszBackup ) >= JET_errSuccess ) )
        {
            (void)ErrEDBUTLMoveFile( wszFmFilePathSrc, wszFmFilePathDst, 0 );
        }
    }

    if ( !FUTILOPTSPreserveTempDB( popts->fUTILOPTSFlags ) )
    {
        err = ErrEDBUTLMoveFile( popts->wszTempDB, wszSourceDB, MOVEFILE_REPLACE_EXISTING );
        if ( err < 0 )
        {
            wprintf( wszErr5, wszSourceDB, popts->wszTempDB, wszSourceDB );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            Call( err );
        }

        if( ( ErrEDBUTLGetFmPathFromDbPath( wszFmFilePathSrc, popts->wszTempDB, &fFmExists ) >= JET_errSuccess ) && fFmExists &&
            ( ErrEDBUTLGetFmPathFromDbPath( wszFmFilePathDst, wszSourceDB ) >= JET_errSuccess ) )
        {
            (void)ErrEDBUTLMoveFile( wszFmFilePathSrc, wszFmFilePathDst, MOVEFILE_REPLACE_EXISTING );
        }

        // Delete temporary database only if everything was successful.
        EDBUTLDeleteTemp( popts );
    }

HandleError:
    return err;
}


// Load registry environment, if enabled.  Then load command-line overrides.
LOCAL JET_ERR ErrEDBUTLUserSystemParameters( JET_INSTANCE *pinstance, UTILOPTS *popts )
{
    JET_ERR err = JET_errSuccess;

    // Command-line parameters override all default and registry values.
    if ( popts->wszLogfilePath != NULL )
    {
        Call( JetSetSystemParameterW( pinstance, 0, JET_paramLogFilePath, 0, popts->wszLogfilePath ) );
    }
    if ( popts->wszSystemPath != NULL )
    {
        Call( JetSetSystemParameterW( pinstance, 0, JET_paramSystemPath, 0, popts->wszSystemPath ) );
    }
    if ( popts->cpageBatchIO != 0 )
    {
        Call( JetSetSystemParameterW( pinstance, 0, JET_paramBatchIOBufferMax, popts->cpageBatchIO * 4, NULL ) );
    }
    if ( popts->cpageDbExtension != 0 )
    {
        Call( JetSetSystemParameterW( pinstance, 0, JET_paramDbExtensionSize, popts->cpageDbExtension, NULL ) );
    }

    if ( NULL != popts->wszBase )
    {
        Call( JetSetSystemParameterW( pinstance, 0, JET_paramBaseName, NULL, popts->wszBase ) );
    }

    if ( popts->lMaxCacheSize != 0 )
    {
        Call( JetSetSystemParameter( pinstance, 0, JET_paramCacheSizeMax, popts->lMaxCacheSize, NULL ) );
    }

HandleError:
    return err;
}


// Teminate Jet, either normally or abnormally.
LOCAL JET_ERR ErrEDBUTLCleanup( JET_INSTANCE instance, JET_SESID sesid, JET_ERR err )
{
    JET_ERR wrn = err;

    if ( 0 != sesid && JET_sesidNil != sesid )
    {
        JET_ERR errT = JetEndSession( sesid, 0 );

        if ( err >= 0 )
            err = errT;
    }

    if ( err < 0 )
    {
        // On error, terminate abruptly and throw out return code from JetTerm2().
        JetTerm2( instance, JET_bitTermAbrupt );
    }
    else
    {
        err = JetTerm2( instance, JET_bitTermComplete );
        if ( err == JET_errSuccess && wrn > JET_errSuccess )
        {
            err = wrn;
        }
    }

    return err;
}

//  ErrDBUTLRestoreComplete 
#ifdef ESENT
//  The FUtilSystemRestrictIdleActivity() is only utilized in higher level ESE code for non-NT builds of the 
//  engine, and further the function is not available in ESENT.
#else //  !ESENT

LOCAL BOOL FAquireBackupRestoreRights()
{
    BOOL        ret_val = TRUE ;
    HANDLE  ProcessHandle;
    DWORD   DesiredAccess;
    HANDLE  TokenHandle;
    LUID        BackupValue;
    LUID        RestoreValue;
    TOKEN_PRIVILEGES NewState;


    // get process handle

    ProcessHandle = GetCurrentProcess();

    // open process token

    DesiredAccess = MAXIMUM_ALLOWED;

    if ( ! OpenProcessToken( ProcessHandle, DesiredAccess, &TokenHandle ) )
    {
        return FALSE;
    }

    // adjust backup token privileges
    if ( ! LookupPrivilegeValueW( NULL, L"SeRestorePrivilege", &RestoreValue ) )
    {
        ret_val = FALSE;
    }

    if ( ! LookupPrivilegeValueW( NULL, L"SeBackupPrivilege", &BackupValue ) )
    {
        ret_val = FALSE;
    }

    // Enable backup privilege for this process

    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = BackupValue;
    NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if ( ! AdjustTokenPrivileges( TokenHandle, FALSE, &NewState, (DWORD)0, NULL, NULL ) )
    {
        ret_val = FALSE;
    }


    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = RestoreValue;
    NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if ( ! AdjustTokenPrivileges( TokenHandle, FALSE, &NewState, (DWORD)0, NULL, NULL ) )
    {
        ret_val = FALSE;
    }
    // AdjustTokenPriv always returns SUCCESS, call GetLast to see if it worked.

    if ( GetLastError() != ERROR_SUCCESS )
    {
        ret_val = FALSE;
    }

    // close process token

    CloseHandle( TokenHandle );
    return( ret_val );
}

#endif //  !ESENT

#define JET_errReturnedForESEBCLI2      JET_errInternalError
#define JET_errReturnedForESEBACK2      JET_errInternalError

#define CallHr( func )                                  \
{                                                       \
    hr = func;                                          \
    hrGLE = GetLastError();                             \
    if ( hrNone != hr )                                 \
    {                                                   \
        goto HandleError;                               \
    }                                                   \
}

#ifndef ESENT
HRESULT hrESEBACK = hrNone;
WCHAR * wszESEBACKError = NULL;

ERR ErrPrintESEBCLI2Error ( HRESULT hr, HRESULT hrGLE, HMODULE hESEBCLI2 )
{
    LPVOID      lpMsgBuf                = NULL;
    WCHAR *     wszFinalMsg                 = NULL;

    // return all the way to main() the JET error
    if ( hr == hrErrorFromESECall )
    {
        assert( hrGLE );
        return hrGLE;
    }

    hrESEBACK = hr;

    if ( hrNone == hr )
    {
        return JET_errSuccess;
    }

    if ( 0 == FormatMessageW(
                FORMAT_MESSAGE_FROM_HMODULE |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                hESEBCLI2,
                hr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPWSTR) &lpMsgBuf,
                0,
                NULL ) )
    {
        lpMsgBuf = NULL;
    }

    if ( NULL == lpMsgBuf )
    {
        lpMsgBuf = (LPVOID) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, sizeof( WCHAR ) );
        if ( NULL == lpMsgBuf )
        {
            return ErrERRCheck( JET_errOutOfMemory );
        }
        ((WCHAR*)lpMsgBuf)[0] = L'\0';
    }

    if ( hr == hrErrorFromESECall || hr == hrErrorFromCallbackCall )
    {
        size_t cbFinalMsg = sizeof( WCHAR ) * ( LOSStrLengthW( (WCHAR *)lpMsgBuf ) + 1 ) + 32;
        wszFinalMsg = (WCHAR *) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, cbFinalMsg );
        if ( wszFinalMsg )
        {
            StringCbPrintfW( wszFinalMsg, cbFinalMsg, (WCHAR *)lpMsgBuf, hrGLE );
            LocalFree( lpMsgBuf );
        }
        else
        {
            // print the message without error number
            wszFinalMsg = (WCHAR *)lpMsgBuf;
        }
    }
    else
    {
        wszFinalMsg = (WCHAR *)lpMsgBuf;
    }


//  LocalFree ( szFinalMsg );
    hrESEBACK = hr;

    // will be freed at the end in main after printing the result
    wszESEBACKError = wszFinalMsg;
    assert ( hrNone != hr );

    return JET_errSuccess;
}

typedef HRESULT (ESEBACK_API * PfnHrESERestoreReopen)(
    _In_  WCHAR *               wszServerName,
    _In_  WCHAR *               wszServiceAnnotation,
    _In_  WCHAR *               wszRestoreLogPath,
    _Out_ HCCX *                phccxRestoreContext);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreClose)(
    _In_ HCCX hccxRestoreContext,
    _In_ ULONG fRestoreAbort);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreComplete)(
    _In_  HCCX                  hccxRestoreContext,
    _In_  WCHAR *               wszRestoreInstanceSystemPath,
    _In_  WCHAR *               wszRestoreInstanceLogPath,
    _In_  WCHAR *               wszTargetInstanceName,
    _In_  ULONG                 fFlags);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreLoadEnvironment)(
    _In_  WCHAR *               wszServerName,
    _In_  WCHAR *               wszRestoreLogPath,
    _Out_ RESTORE_ENVIRONMENT **  ppRestoreEnvironment);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreGetEnvironment)(
    _In_  HCCX                  hccxRestoreContext,
    _Out_ RESTORE_ENVIRONMENT **  ppRestoreEnvironment);

typedef void (ESEBACK_API * PfnESERestoreFreeEnvironment)(
    _In_  RESTORE_ENVIRONMENT * pRestoreEnvironment);

typedef HRESULT (ESEBACK_API * PfnHrESERecoverAfterRestore2)(
    _In_  WCHAR *               wszRestoreLogPath,
    _In_  WCHAR *               wszCheckpointFilePath,
    _In_  WCHAR *               wszLogFilePath,
    _In_  WCHAR *               wszTargetInstanceCheckpointFilePath,
    _In_  WCHAR *               wszTargetInstanceLogPath);

#endif // ESENT


WCHAR * WszCopy( const WCHAR *  wsz )
{
    WCHAR *  wszCopy;
    LONG cb;

    assert ( wsz );
    cb = sizeof(WCHAR) * ((ULONG)LOSStrLengthW( wsz ) + 1);

    if ( ( wszCopy = (WCHAR *) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, cb ) ) == NULL )
        return(NULL);

    StringCbCopyW( wszCopy, cb, wsz );
    return( wszCopy );
}

void FormatErrorInfoString(
    __out_bcount(cbFormatted) CHAR * szFormatted,
    size_t cbFormatted,
    const ERR err,
    const WCHAR * const wszErrStrings,
    const JET_ERRINFOBASIC_W * perrinfo )
{
    Assert( szFormatted[0] == '\0' );
    (void)ErrOSStrCbFormatA( szFormatted, cbFormatted, "err = %d (%ws", err, wszErrStrings ? wszErrStrings : L"unknown, unknown"  );
    // Truncate off the ", <Full error message>" to get the constant name only.
    CHAR * const szBeginErrExplanation = strchr( szFormatted, ',' );
    Assert( szBeginErrExplanation ); // implies we didn't get the , before we potentially truncated!?
    if ( szBeginErrExplanation )
    {
        Assert( szBeginErrExplanation[0] == ',' );
        szBeginErrExplanation[0] = '\0';
    }
#ifdef DEBUG
    if ( perrinfo && perrinfo->lSourceLine != 0 )
    {
        (void)ErrOSStrCbFormatA( szFormatted + strlen(szFormatted), cbFormatted - strlen(szFormatted),
                                    " - %ws:%d", perrinfo->rgszSourceFile, perrinfo->lSourceLine );
    }
#endif
    OSStrCbAppendA( szFormatted, cbFormatted, ")" );
}

void PushEseutilEndMarker( const INT csec, const INT cmsec, const ERR err, const WCHAR * const wszErrStrings )
{
    JET_ERRINFOBASIC_W  errinfo = { sizeof(JET_ERRINFOBASIC_W), 0 };
    CHAR                szEseutilEndMarker[300];

    (void)JetGetErrorInfoW( (void*)&err, &errinfo, sizeof(errinfo), JET_ErrorInfoSpecificErr, 0x0 );

    OSStrCbFormatA( szEseutilEndMarker, sizeof(szEseutilEndMarker), "Eseutil Cmd End: time %02d:%02d, ", csec, cmsec );
    FormatErrorInfoString( szEseutilEndMarker + strlen(szEseutilEndMarker), _cbrg(szEseutilEndMarker) - strlen(szEseutilEndMarker), err, wszErrStrings, &errinfo );

    JET_TESTHOOKTRACETESTMARKER mark = { sizeof(mark), szEseutilEndMarker, 100 };
    (void)JetTestHook( opTestHookTraceTestMarker, &mark );
}

void PrintESEUTILError(
    const JET_ERR   err,
    const WCHAR *   wszMessage,
    const BOOL      fWhitespaceOnErr,
    const ULONG     timer )
{
    INT                 iSec, iMSec;

    EDBUTLGetTime( timer, &iSec, &iMSec );

    if ( fWhitespaceOnErr )
    {
        wprintf( L"%c%c", wchNewLine, wchNewLine );
    }

    if ( 0 != err && NULL != wszMessage )
    {
        wprintf( wszOperFailWithError, err, wszMessage, iSec, iMSec );
    }
    else
    {
        wprintf( wszOperFailUnknownError, iSec, iMSec );
    }

    PushEseutilEndMarker( iSec, iMSec, err,
#ifndef ESENT
            ( err == hrESEBACK || hrESEBACK != hrNone ) ? NULL :
#endif
            wszMessage );

    wprintf( L"%c%c", wchNewLine, wchNewLine );
}


LOCAL void PrintField( const WCHAR * wszDesc, INT cDesc, const WCHAR * wszData, const BOOL fNewLine = TRUE )
{
    wprintf( L"%*s %s%s", cDesc, wszDesc, wszData?wszData:L"", fNewLine?L"\n":L"" );
}

WCHAR * rwszRecoverStatus[] = {
    L"recoverInvalid",
    L"recoverNotStarted",
    L"recoverStarted",
    L"recoverEnded"
};


LOCAL BOOL FDBUTLLoadLibrary( const WCHAR* wszLibrary, HMODULE *plibrary )
{
    while ( NULL == ( *plibrary = LoadLibraryExW( wszLibrary, NULL, 0 ) ) )
    {
#ifdef MINIMAL_FUNCTIONALITY
        break;
#else
        wprintf(
           L"Unable to find the callback library %s (or one of its dependencies).\r\n"
           L"Copy in the file and hit a key to retry, or hit q to abort.\r\n",
           wszLibrary );

        wprintf( L"%s", wszHelpPromptCursor );
        const wint_t ch = (wint_t)_getch();

        if ( L'q' == ch )
        {
            break;
        }
#endif
    }

    return ( NULL != *plibrary );
}

#ifndef ESENT

LOCAL HRESULT HrDBUTLLoadRestoreEnv( const HMODULE  hESEBCLI2, const WCHAR * wszRestorePath, RESTORE_ENVIRONMENT ** ppREnv, INT cDesc = 0)
{
    HRESULT         hr                  = hrNone;
    HRESULT         hrGLE               = hrNone;
    PfnHrESERestoreLoadEnvironment      pfnHrESERestoreLoadEnv  = NULL;

    assert ( hESEBCLI2 );
    assert ( ppREnv );
    assert ( wszRestorePath );

    pfnHrESERestoreLoadEnv = (PfnHrESERestoreLoadEnvironment) GetProcAddress( hESEBCLI2, "HrESERestoreLoadEnvironment" );

    if ( !pfnHrESERestoreLoadEnv )
    {
        hr = hrLoadCallbackFunctionFailed;
        goto HandleError;
    }

    PrintField( L"Restore log file:", cDesc, wszRestorePath );
    PrintField( L"", cDesc, NULL );

    assert ( pfnHrESERestoreLoadEnv );
    CallHr ( (*pfnHrESERestoreLoadEnv)(     NULL,
                                            (WCHAR*) wszRestorePath,
                                            ppREnv ) );
HandleError:
    // SOMEONE commented this out, this func doesn't allocate this variable, and I found at least one path
    // where this is a stack allocated MAX_PATH variable.
    //LocalFree ( wszRestorePath );

    return hr;
}


LOCAL VOID DBUTLIDumpRestoreEnv( RESTORE_ENVIRONMENT * pREnv, INT   cDesc = 0 )
{
    WCHAR           wszBuffer[64];

    // dump Restore.Env
    assert ( pREnv );

    PrintField( L"Restore Path:", cDesc, pREnv->m_wszRestoreLogPath );
    PrintField( L"Annotation:", cDesc, pREnv->m_wszAnnotation );
    PrintField( L"Server:", cDesc, pREnv->m_wszRestoreServerName?pREnv->m_wszRestoreServerName:L"" );

    PrintField( L"Backup Instance:", cDesc, pREnv->m_wszSrcInstanceName );
    PrintField( L"Target Instance:", cDesc, pREnv->m_wszTargetInstanceName );

    PrintField( L"Restore Instance System Path:", cDesc, pREnv->m_wszRestoreInstanceSystemPath );
    PrintField( L"Restore Instance Log Path:", cDesc, pREnv->m_wszRestoreInstanceLogPath );

    PrintField( L"", cDesc, NULL );

    StringCbPrintfW( wszBuffer, sizeof( wszBuffer ), L"%d database(s)", pREnv->m_cDatabases );
    PrintField( L"Databases:", cDesc, wszBuffer );

    assert ( pREnv->m_wszDatabaseDisplayName || 0 == pREnv->m_cDatabases);
    assert ( pREnv->m_rguidDatabase || 0 == pREnv->m_cDatabases);
    assert ( pREnv->m_wszDatabaseStreamsS || 0 == pREnv->m_cDatabases);
    assert ( pREnv->m_wszDatabaseStreamsD || 0 == pREnv->m_cDatabases);

    cDesc += 8;
    for (ULONG iDb = 0; iDb < pREnv->m_cDatabases; iDb++)
    {
        assert ( pREnv->m_wszDatabaseDisplayName[iDb] );
        assert ( pREnv->m_wszDatabaseStreamsS[iDb] );
        assert ( pREnv->m_wszDatabaseStreamsS[iDb] );

        WCHAR guidStr[256];
        WCHAR * wszStreams;
        GUID guid = pREnv->m_rguidDatabase[iDb];

        PrintField( L"Database Name:", cDesc, pREnv->m_wszDatabaseDisplayName[iDb] );
        // like: 6B29FC40-CA47-1067-B31D-00DD010662DA
        StringCbPrintfW( guidStr, sizeof( guidStr ),
                    L"%08X-%04X-%04X-%08X%08X",
                    guid.Data1, guid.Data2, guid.Data3,
                    *(DWORD *)&guid.Data3,*( 1 + (DWORD *)&guid.Data3 ) );

        PrintField( L"GUID:", cDesc, guidStr );

        wszStreams = pREnv->m_wszDatabaseStreamsS[iDb];
        PrintField( L"Source Files:", cDesc, NULL, FALSE );
        while ( L'\0' != wszStreams[0] )
        {
            wprintf( L"%s ", wszStreams );
            wszStreams += LOSStrLengthW( wszStreams ) + 1;
        }
        wprintf( L"\n" );
        wszStreams = pREnv->m_wszDatabaseStreamsD[iDb];
        PrintField( L"Destination Files:", cDesc, NULL, FALSE );
        while ( L'\0' != wszStreams[0] )
        {
            wprintf( L"%s ", wszStreams );
            wszStreams += LOSStrLengthW( wszStreams ) + 1;
        }
        PrintField( L"", cDesc, NULL );
        PrintField( L"", cDesc, NULL );
    }

    cDesc -= 8;
    PrintField( L"", cDesc, NULL );
    PrintField( L"", cDesc, NULL );

    if ( pREnv->m_wszLogBaseName )
    {
        assert ( 0 != pREnv->m_ulGenLow );
        assert ( 0 != pREnv->m_ulGenHigh );
        assert ( pREnv->m_ulGenLow <= pREnv->m_ulGenHigh );

        // UNDONE: we need to fix this to detect 8.3 or 11.3 and
        // JTX or LOG extension. This will be done by adding this info
        // in restore.env at parsing point (HrESERestoreOpenFile)
        // For now, we will display the Store defaults as this is
        // the only client using the eseback2/esebcli2 anyway
        StringCbPrintfW( wszBuffer, sizeof( wszBuffer ), L"%s%08X.log - %s%08X.log",
            pREnv->m_wszLogBaseName,
            pREnv->m_ulGenLow,
            pREnv->m_wszLogBaseName,
            pREnv->m_ulGenHigh);
    }
    else
    {
        assert ( 0 == pREnv->m_ulGenLow );
        assert ( 0 == pREnv->m_ulGenHigh );
        StringCbPrintfW( wszBuffer, sizeof( wszBuffer ), L"no log files restored");
    }
    PrintField( L"Log files range:", cDesc, wszBuffer );

    PrintField( L"Last Restore Time:", cDesc, ( 0 != pREnv->m_timeLastRestore ? _wctime( &pREnv->m_timeLastRestore ) : L"<none>" ) );

    RECOVER_STATUS status = pREnv->m_statusLastRecover;

    if ( status >= sizeof(rwszRecoverStatus)/ sizeof(rwszRecoverStatus[0] ) )
        status = recoverInvalid;

    PrintField( L"Recover Status:", cDesc, rwszRecoverStatus [ status ] );

    StringCbPrintfW( wszBuffer, sizeof( wszBuffer ), L"0x%08X", pREnv->m_hrLastRecover);
    PrintField( L"Recover Error:", cDesc, wszBuffer );

    PrintField( L"Recover Time:", cDesc, ( 0 != pREnv->m_timeLastRecover ? _wctime( &pREnv->m_timeLastRecover ) : L"<none>" ) );

}

LOCAL JET_ERR ErrDBUTLDumpRestoreEnv( const WCHAR * wszRestorePath )
{
    JET_ERR         err                 = JET_errSuccess;
    HRESULT         hr                  = hrNone;
    HRESULT         hrGLE               = hrNone;
    HMODULE         hESEBCLI2           = NULL;
    INT             cDesc               = 30;

    RESTORE_ENVIRONMENT *               pREnv                   = NULL;
    PfnESERestoreFreeEnvironment        pfnESERestoreFreeEnv    = NULL;

    assert ( wszRestorePath );

    if ( !FDBUTLLoadLibrary( ESEBCLI2_DLL_NAME, &hESEBCLI2 ) )
    {
        Call ( JET_errCallbackNotResolved );
    }

    pfnESERestoreFreeEnv = (PfnESERestoreFreeEnvironment) GetProcAddress( hESEBCLI2, "ESERestoreFreeEnvironment" );
    if ( !pfnESERestoreFreeEnv )
    {
        Call ( JET_errCallbackNotResolved );
    }

    CallHr ( HrDBUTLLoadRestoreEnv( hESEBCLI2, wszRestorePath, &pREnv, cDesc ) );

    assert ( pREnv );
    DBUTLIDumpRestoreEnv( pREnv, cDesc );


HandleError:

    if ( pREnv )
    {
        assert ( pfnESERestoreFreeEnv );
        (*pfnESERestoreFreeEnv)( pREnv );
        pREnv = NULL;
    }

    if ( hrNone != hr )
    {
        assert ( hESEBCLI2 );
        err = ErrPrintESEBCLI2Error ( hr, hrGLE, hESEBCLI2 );
    }

    if ( NULL != hESEBCLI2 )
    {
        FreeLibrary( hESEBCLI2 );
        hESEBCLI2 = NULL;
    }

    return err;
}

LOCAL JET_ERR ErrDBUTLRestoreComplete( const WCHAR * wszFullRestorePath, const WCHAR * wszTargetInstance, const WCHAR* wszPlayForwardLogs, BOOL fKeepLogs )
{
    JET_ERR         err                 = JET_errSuccess;
    HRESULT         hr                  = hrNone;
    HRESULT         hrGLE               = hrNone;
    INT             cDesc               = 20;

    DWORD           chSize ;
    WCHAR           wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    HCCX            hccxRestoreContext  = NULL;
    HMODULE         hESEBCLI2 = NULL;
    HMODULE         hESEBACK2 = NULL;

    PfnHrESERestoreReopen               pfnErrESERestoreReopen =     NULL;
    PfnHrESERestoreClose                pfnErrESERestoreClose       = NULL;
    PfnHrESERestoreComplete             pfnErrESERestoreComplete    = NULL;

    RESTORE_ENVIRONMENT *               pREnv                       = NULL;
    PfnHrESERestoreGetEnvironment       pfnHrESERestoreGetEnv       = NULL;
    PfnESERestoreFreeEnvironment        pfnESERestoreFreeEnv        = NULL;

    assert ( wszFullRestorePath );

    // if target instance is used, the should be no play forward directory set
    assert ( NULL == wszTargetInstance ||  NULL == wszPlayForwardLogs );

    if ( !FDBUTLLoadLibrary( ESEBCLI2_DLL_NAME, &hESEBCLI2 ) )
    {
        Call ( JET_errCallbackNotResolved );
    }

    pfnErrESERestoreReopen = (PfnHrESERestoreReopen) GetProcAddress( hESEBCLI2, "HrESERestoreReopen" );
    pfnErrESERestoreClose = (PfnHrESERestoreClose) GetProcAddress( hESEBCLI2, "HrESERestoreClose" );
    pfnErrESERestoreComplete = (PfnHrESERestoreComplete) GetProcAddress( hESEBCLI2, "HrESERestoreComplete" );
    pfnHrESERestoreGetEnv = (PfnHrESERestoreGetEnvironment) GetProcAddress( hESEBCLI2, "HrESERestoreGetEnvironment" );
    pfnESERestoreFreeEnv = (PfnESERestoreFreeEnvironment) GetProcAddress( hESEBCLI2, "ESERestoreFreeEnvironment" );

    if (    !pfnErrESERestoreReopen || !pfnErrESERestoreClose || !pfnErrESERestoreComplete ||
            !pfnHrESERestoreGetEnv || !pfnESERestoreFreeEnv)
    {
        Call ( JET_errCallbackNotResolved );
    }

    if ( !FAquireBackupRestoreRights() )
    {
        Call ( JET_errReturnedForESEBCLI2 );
    }

    chSize = sizeof(wszComputerName) / sizeof(wszComputerName[0]);
    if ( !GetComputerNameW ( wszComputerName, &chSize ) )
    {
        Call ( JET_errNTSystemCallFailed );
    }


    PrintField( L"Using restore environment", cDesc, NULL );

    // we did the attempt using GetComputerName which may not work on
    // clusters where you have to use the virtual node  (x5:160326)
    // We will try to use the server name in the restore.env which was
    // set at HrESERestoreOpen time (the node the restore took place on)
    CallHr ( HrDBUTLLoadRestoreEnv( hESEBCLI2, wszFullRestorePath, &pREnv, cDesc ) );

    assert ( pREnv );
    DBUTLIDumpRestoreEnv( pREnv, cDesc + 10 );

    assert ( pfnErrESERestoreReopen );
    PrintField( L"Restoring ....", 0, NULL );

    if ( wszPlayForwardLogs )
    {
        // /f set, we will run recovery right here
        // (as oposed with signaling the running server process using RestoreComplete)
        // so we need the call into eseback2.dll

        if ( !FDBUTLLoadLibrary( ESEBACK2_DLL_NAME, &hESEBACK2 ) )
        {
            Call ( JET_errCallbackNotResolved );
        }

        PfnHrESERecoverAfterRestore2  pfnHrESERecoverAfterRestore2 = NULL;

        pfnHrESERecoverAfterRestore2 = (PfnHrESERecoverAfterRestore2) GetProcAddress( hESEBACK2,
            "HrESERecoverAfterRestore2" );

        if ( !pfnHrESERecoverAfterRestore2 )
        {
            Call ( JET_errCallbackNotResolved );
        }

        CallHr( (*pfnHrESERecoverAfterRestore2)(
                            (WCHAR*) wszFullRestorePath,                //  restore directory
                            (WCHAR*) wszPlayForwardLogs,            //  log path for recovery instance
                            (WCHAR*) wszPlayForwardLogs,            //  system path for recovery instance
                            (WCHAR*) wszPlayForwardLogs,            //  play-forward system path
                            (WCHAR*) wszPlayForwardLogs ) );        //  play-forward log path
    }
    else
    {

        PrintField( L"Restore to server:", cDesc, pREnv->m_wszRestoreServerName?pREnv->m_wszRestoreServerName:wszComputerName );
        CallHr ((*pfnErrESERestoreReopen)(  pREnv->m_wszRestoreServerName?pREnv->m_wszRestoreServerName:wszComputerName,
                                            NULL,
                                            (WCHAR*)wszFullRestorePath,
                                            &hccxRestoreContext ) );
        if ( wszTargetInstance )
        {
            if ( wszTargetInstance[0] == L'\0' )
            {
                // no play forward
                wszTargetInstance = NULL;
            }
        }
        else
        {
            // get the instance form the Restore.Env, as the Source Instance Name

            // get restore.env if not read already
            if ( NULL == pREnv )
            {
                assert ( pfnHrESERestoreGetEnv );
                CallHr ( (*pfnHrESERestoreGetEnv)(  hccxRestoreContext,
                                                    &pREnv ) );
            }

            // dump Restore.Env
            assert ( pREnv );
            assert ( pREnv->m_wszSrcInstanceName );
            wszTargetInstance = WszCopy( pREnv->m_wszSrcInstanceName );

            assert ( pfnESERestoreFreeEnv );
            (*pfnESERestoreFreeEnv)( pREnv );
            pREnv = NULL;

            if ( !wszTargetInstance )
            {
                Call ( JET_errOutOfMemory );
            }
        }

        PrintField( L"Target Instance:", cDesc, wszTargetInstance?wszTargetInstance:L"" );

        assert ( pfnErrESERestoreComplete );
        CallHr ( (*pfnErrESERestoreComplete)(   hccxRestoreContext,
                                        (WCHAR*) wszFullRestorePath,
                                        (WCHAR*) wszFullRestorePath,
                                        (WCHAR*) wszTargetInstance,
                                        fKeepLogs?ESE_RESTORE_KEEP_LOG_FILES:0 // no db mount, wait restore complete
                                        ) );
    }
HandleError:

    if ( pREnv )
    {
        assert ( pfnESERestoreFreeEnv );
        (*pfnESERestoreFreeEnv)( pREnv );
        pREnv = NULL;
    }

    if ( hccxRestoreContext )
    {
        assert ( pfnErrESERestoreClose );
        (void) (*pfnErrESERestoreClose)( hccxRestoreContext, (hrNone == hr) ? RESTORE_CLOSE_NORMAL:RESTORE_CLOSE_ABORT );
        hccxRestoreContext = NULL;
    }

    if ( hrNone != hr )
    {
        assert ( hESEBCLI2 );
        err = ErrPrintESEBCLI2Error ( hr, hrGLE, hESEBCLI2 );
    }

    if ( NULL != hESEBCLI2 )
    {
        FreeLibrary( hESEBCLI2 );
        hESEBCLI2 = NULL;
    }

    if ( NULL != hESEBACK2 )
    {
        FreeLibrary( hESEBACK2 );
        hESEBACK2 = NULL;
    }

    return err;
}

#ifdef RESTORE_SERVER_SIMULATION

// must match the definition from ESEBACK2\srvsim.cxx
typedef HRESULT (__stdcall * PfnServerSim)( const char * szFileDef );

LOCAL JET_ERR ErrDBUTLServerSim( const WCHAR * wszSimulationDef )
{
    JET_ERR         err             = JET_errSuccess;
    HRESULT         hr              = hrNone;
    HRESULT         hrGLE           = hrNone;
    HMODULE         hESEBACK2       = NULL;
    PfnServerSim    pfnServerSim    = NULL;
    CHAR        szSimulationDef[_MAX_PATH + 1];

    Call( ErrOSSTRUnicodeToAscii( wszSimulationDef, szSimulationDef, _countof(szSimulationDef), NULL, OSSTR_NOT_LOSSY ) );

    if ( !FDBUTLLoadLibrary( ESEBACK2_DLL_NAME, &hESEBACK2 ) )
    {
        Call ( JET_errCallbackNotResolved );
    }

    pfnServerSim = (PfnServerSim) GetProcAddress( hESEBACK2, "ServerSim" );
    if ( !pfnServerSim )
    {
        Call ( JET_errCallbackNotResolved );
    }

    CallHr ( (*pfnServerSim)( szSimulationDef ) );

HandleError:

    if ( hrNone != hr )
    {
        assert ( hESEBACK2 );
        err = ErrPrintESEBCLI2Error ( hr, hrGLE, hESEBACK2 );
    }

    if ( NULL != hESEBACK2 )
    {
        FreeLibrary( hESEBACK2 );
        hESEBACK2 = NULL;
    }

    return err;

}

#endif // RESTORE_SERVER_SIMULATION

#endif // ESENT

/*
typedef struct
{
    JET_COLUMNID        columnid;
    void                *pvData;
    unsigned long       cbData;
    unsigned long       cbActual;
    JET_GRBIT           grbit;
    unsigned long       ibLongValue;
    unsigned long       itagSequence;
    JET_COLUMNID        columnidNextTagged;
    JET_ERR             err;
} JET_RETRIEVECOLUMN;
*/

struct DUMPCOLUMN
{
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    INT             cbMax;
    INT             cchPrint;
    CHAR            szColumnName[JET_cbNameMost+1];
};


//  ================================================================
LOCAL INT CchPrintForColtyp( _In_ const JET_COLTYP coltyp, _In_ const INT cchColumnName, _In_ const INT cbMax )
//  ================================================================
{
    INT cchPrint = 0;

    switch( coltyp )
    {
        case JET_coltypBit:
            cchPrint = 1; // 0 => 1
            break;
        case JET_coltypUnsignedByte:
            cchPrint = 2; // 00 => ff
            break;
        case JET_coltypShort:
            cchPrint = 6; // -32768 => 32767
            break;
        case JET_coltypUnsignedShort:
            cchPrint = 6;
            break;
        case JET_coltypLong:
            cchPrint = 10; // -2147483649 => 2147483648
            break;
        case JET_coltypUnsignedLong:
            cchPrint = 10;
            break;
        case JET_coltypLongLong:
            cchPrint = 20;
            break;
        case JET_coltypUnsignedLongLong:
            cchPrint = 20;
            break;
        case JET_coltypCurrency:
            cchPrint = 20;
            break;
        case JET_coltypIEEESingle:
            cchPrint = 10;
            break;
        case JET_coltypIEEEDouble:
            cchPrint = 10;
            break;
        case JET_coltypDateTime:
            cchPrint = 16;
            break;
        case JET_coltypGUID:
            cchPrint = 32;
            break;
        case JET_coltypBinary:
        case JET_coltypText:
        case JET_coltypLongBinary:
        case JET_coltypLongText:
            cchPrint = min( cbMax, 32 );
            break;
        default:
            assert( 0 );
            cchPrint = 16;
            break;
    }

    cchPrint = max( cchPrint, cchColumnName );
    cchPrint = min( 32, cchPrint );

    return cchPrint;
}


//  ================================================================
LOCAL void PrintColumn( _In_ const JET_COLTYP coltyp, _In_ const INT cchMax, _In_ const void * const pv, _In_ const INT cb )
//  ================================================================
{
    switch( coltyp )
    {
        case JET_coltypBit:
        {
            const BYTE b = *((BYTE *)pv);
            wprintf( L"%c", b ? L'1' : L'0' );
            break;
        }
        case JET_coltypUnsignedByte:
        {
            const BYTE b = *((BYTE *)pv);
            wprintf( L"%2.2x", b );
            break;
        }
        case JET_coltypShort:
        {
            const SHORT s = *((SHORT *)pv);
            wprintf( L"% *.*d", cchMax-1, cchMax-1, s );
            break;
        }
        case JET_coltypUnsignedShort:
        {
            const USHORT us = *((USHORT *)pv);
            wprintf( L"% *.*u", cchMax-1, cchMax-1, us );
            break;
        }
        case JET_coltypLong:
        {
            const LONG l = *((LONG *)pv);
            wprintf( L"% *.*d", cchMax-1, cchMax-1, l );
            break;
        }
        case JET_coltypUnsignedLong:
        {
            const ULONG ul = *((ULONG *)pv);
            wprintf( L"% *.*u", cchMax-1, cchMax-1, ul );
            break;
        }
        case JET_coltypLongLong:
        case JET_coltypCurrency:
        {
            const __int64 q = *((__int64 *)pv);
            wprintf( L"% *.*I64d", cchMax-1, cchMax-1, q );
            break;
        }
        case JET_coltypUnsignedLongLong:
        {
            const __int64 q = *((__int64 *)pv);
            wprintf( L"% *.*I64u", cchMax-1, cchMax-1, q );
            break;
        }
        case JET_coltypIEEESingle:
        {
            const float f = *((float *)pv);
            wprintf( L"% *.4g", cchMax - 6, f );    //  precision = 4, decimal = 1, minus = 1
            break;
        }
        case JET_coltypIEEEDouble:
        {
            const double d = *((double *)pv);
            wprintf( L"% *.4g", cchMax - 6, d ); // precision = 4, decimal = 1, minus = 1
            break;
        }
        case JET_coltypText:
        case JET_coltypLongText:
        {
            const char * const sz = (char *)pv;
            const INT cch = min( cchMax, cb );
            wprintf( L"%-*.*hs", cch, cch, sz );
            break;
        }
        default:
            assert( 0 );    //  missed case
        case JET_coltypDateTime:
        case JET_coltypGUID:
        case JET_coltypBinary:
        case JET_coltypLongBinary:
        {
            const BYTE * pb = (BYTE *)pv;
            const INT cchMaxT = min( cchMax, cb * 2 );
            INT cch;
            for( cch = 0; cch < cchMaxT; )
            {
                cch += wprintf( L"%2.2X", *pb );
                ++pb;
            }
            while( cch < cchMax )
            {
                cch += wprintf( L" " );
            }
            break;
        }
    }
}


//  ================================================================
LOCAL JET_ERR ErrDumpcolumnFromColumnlist(
    _In_ const JET_SESID          sesid,
    _In_ const JET_COLUMNLIST&    columnlist,
    _Out_ DUMPCOLUMN * const      rgdumpcolumn,
    _In_ const ULONG      cdumpcolumnMax,
    _Out_ ULONG&          cdumpcolumn )
//  ================================================================
{
    JET_ERR err     = JET_errSuccess;
    const JET_TABLEID tableid = columnlist.tableid;

    cdumpcolumn = 0;

    //  need to know the name, columnid, coltyp, cbMax

    enum {
        iretcolName = 0,
        iretcolColumnid,
        iretcolColtyp,
        iretcolCbMax,
        cretcol
    };

    JET_RETRIEVECOLUMN  rgretrievecolumn[cretcol];

    for( INT iretcol = 0; iretcol < cretcol; ++iretcol )
    {
        rgretrievecolumn[iretcol].grbit                 = 0;
        rgretrievecolumn[iretcol].ibLongValue           = 0;
        rgretrievecolumn[iretcol].itagSequence          = 1;
        rgretrievecolumn[iretcol].columnidNextTagged    = 0;
        rgretrievecolumn[iretcol].err                   = JET_errSuccess;
    }

    rgretrievecolumn[iretcolName].columnid      = columnlist.columnidcolumnname;
    rgretrievecolumn[iretcolName].cbData        = sizeof( rgdumpcolumn[0].szColumnName );
    rgretrievecolumn[iretcolColumnid].columnid  = columnlist.columnidcolumnid;
    rgretrievecolumn[iretcolColumnid].cbData    = sizeof( rgdumpcolumn[0].columnid );
    rgretrievecolumn[iretcolColtyp].columnid    = columnlist.columnidcoltyp;
    rgretrievecolumn[iretcolColtyp].cbData      = sizeof( rgdumpcolumn[0].coltyp );
    rgretrievecolumn[iretcolCbMax].columnid     = columnlist.columnidcbMax;
    rgretrievecolumn[iretcolCbMax].cbData       = sizeof( rgdumpcolumn[0].cbMax );

    Call( JetMove( sesid, tableid, JET_MoveFirst, 0 ) );

    //  there are three ways to leave this loop
    //      - an error from a Jet call
    //      - getting JET_errNoCurrentRecord from JetMove( MoveNext )
    //      - retrieving the maximum number of columns allowed

    while( 1 )
    {
        if( cdumpcolumn >= cdumpcolumnMax )
        {
            //  we have hit the maximum number of columns to dump
            break;
        }

        memset( rgdumpcolumn[cdumpcolumn].szColumnName, 0, sizeof( rgdumpcolumn[0].szColumnName ) );

        rgretrievecolumn[iretcolName].pvData        = (void *)(rgdumpcolumn[cdumpcolumn].szColumnName);
        rgretrievecolumn[iretcolColumnid].pvData    = (void *)(&(rgdumpcolumn[cdumpcolumn].columnid));
        rgretrievecolumn[iretcolColtyp].pvData      = (void *)(&(rgdumpcolumn[cdumpcolumn].coltyp));
        rgretrievecolumn[iretcolCbMax].pvData       = (void *)(&(rgdumpcolumn[cdumpcolumn].cbMax));

        Call( JetRetrieveColumns( sesid, tableid, rgretrievecolumn, cretcol ) );

        //  the column names are not NULL-terminated. Add a terminator

        rgdumpcolumn[cdumpcolumn].szColumnName[rgretrievecolumn[iretcolName].cbActual] = 0;

        rgdumpcolumn[cdumpcolumn].cchPrint          = CchPrintForColtyp(
                                                            rgdumpcolumn[cdumpcolumn].coltyp,
                                                            (INT) strlen( rgdumpcolumn[cdumpcolumn].szColumnName ),
                                                            rgdumpcolumn[cdumpcolumn].cbMax );

        ++cdumpcolumn;

        err = JetMove( sesid, tableid, JET_MoveNext, 0 );
        if( JET_errNoCurrentRecord == err )
        {
            assert( columnlist.cRecord == cdumpcolumn );
            //  we have hit the end of the table
            err = JET_errSuccess;
            break;
        }
        Call( err );

        assert( cdumpcolumn <= columnlist.cRecord );
    }

HandleError:
    return err;
}


//  ================================================================
LOCAL void DBUTLDumpTableRecordsHeader(
    _In_ const DUMPCOLUMN * const rgdumpcolumn,
    _In_ const INT ccolumns )
//  ================================================================
//
//  Print the name of each column, followed by a space
//  Keep track of the number of characters and print a line of '-'s
//
//-
{
    INT icolumn;
    INT ich;
    INT cchTotal = 0;

    for( icolumn = 0; icolumn < ccolumns; ++icolumn )
    {
        const INT cchPrint = rgdumpcolumn[icolumn].cchPrint;
        cchTotal += printf( "%-*.*s ", cchPrint, cchPrint, rgdumpcolumn[icolumn].szColumnName );
    }
    wprintf( L"%c", wchNewLine );

    for( ich = 0; ich < cchTotal; ++ich )
    {
        wprintf( L"-" );
    }
    wprintf( L"%c", wchNewLine );
}


//  ================================================================
LOCAL VOID EDBUTLPrintOneTableRecord(
    _In_ const DUMPCOLUMN * const     rgdumpcolumn,
    _In_ JET_RETRIEVECOLUMN * const   rgretcol,
    _In_ const INT                    ccolumns )
//  ================================================================
{
    INT icolumn;
    for( icolumn = 0; icolumn < ccolumns; ++icolumn )
    {
        const JET_COLTYP coltyp = rgdumpcolumn[icolumn].coltyp;
        const INT cchMax        = rgdumpcolumn[icolumn].cchPrint;
        const void * const pv   = rgretcol[icolumn].pvData;
        const INT cb            = rgretcol[icolumn].cbActual;

        PrintColumn( coltyp, cchMax, pv, cb );
        wprintf( L" " );
    }
    wprintf( L"%c", wchNewLine );
}


//  ================================================================
LOCAL JET_ERR ErrEDBUTLDumpOneTableRecord(
    _In_ const JET_SESID              sesid,
    _In_ const JET_TABLEID            tableid,
    _In_ const DUMPCOLUMN * const     rgdumpcolumn,
    _In_ JET_RETRIEVECOLUMN * const   rgretcol,
    _In_ const INT                    ccolumns )
//  ================================================================
{
    JET_ERR                 err         = JET_errSuccess;

    Call( JetRetrieveColumns( sesid, tableid, rgretcol, ccolumns ) );
    EDBUTLPrintOneTableRecord( rgdumpcolumn, rgretcol, ccolumns );

HandleError:
    return err;
}


//  ================================================================
LOCAL JET_ERR ErrEDBUTLDumpTableRecords(
    _In_ const JET_SESID          sesid,
    _In_ const JET_TABLEID        tableid,
    _In_ const DUMPCOLUMN * const rgdumpcolumn,
    _In_ const INT                ccolumns,
    _In_ const INT                crecordsMax )
//  ================================================================
{
    JET_ERR                 err         = JET_errSuccess;
    JET_RETRIEVECOLUMN  *   rgretcol    = NULL;
    BYTE                *   pb          = NULL;
    INT                     iretcol;
    const INT               cbPerColumn = 256;
    INT                     crecordsDumped;

    if ( ccolumns < 0 || ccolumns > JET_ccolMost )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    DBUTLDumpTableRecordsHeader( rgdumpcolumn, ccolumns );

    pb          = new BYTE[ccolumns*cbPerColumn];
    rgretcol    = new JET_RETRIEVECOLUMN[ccolumns];
    if( NULL == rgretcol || NULL == pb )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    for( iretcol = 0; iretcol < ccolumns; ++iretcol )
    {
        rgretcol[iretcol].columnid      = rgdumpcolumn[iretcol].columnid;
        rgretcol[iretcol].pvData        = pb + (iretcol * cbPerColumn );
        rgretcol[iretcol].cbData        = cbPerColumn;
        rgretcol[iretcol].grbit         = 0;
        rgretcol[iretcol].ibLongValue   = 0;
        rgretcol[iretcol].itagSequence  = 1;
    }

    for( err = JetMove( sesid, tableid, JET_MoveFirst, 0 ), crecordsDumped = 0;
         err >= JET_errSuccess && ( crecordsDumped < crecordsMax );
         err = JetMove( sesid, tableid, JET_MoveNext, 0 ), ++crecordsDumped )
    {
        Call( ErrEDBUTLDumpOneTableRecord( sesid, tableid, rgdumpcolumn, rgretcol, ccolumns ) );
    }

    if( JET_errNoCurrentRecord == err )
    {
        err = JET_errSuccess;
    }

HandleError:
    if( NULL != pb )
    {
        delete [] pb;
    }
    if( rgretcol )
    {
        delete [] rgretcol;
    }

    return err;
}


//  ================================================================
LOCAL_BROKEN JET_ERR ErrEDBUTLDumpTable(
    _In_ const JET_INSTANCE   instance,
    _In_ const JET_SESID      sesid,
    _In_ const JET_DBID       dbid,
    _In_ const WCHAR * const  wszTable,
    _In_ const INT            crecordsMax )
//  ================================================================
{
    JET_ERR         err         = JET_errSuccess;
    JET_TABLEID     tableid     = JET_tableidNil;
    JET_COLUMNLIST  columnlist;

    INT                     ccolumns        = 0;
    DUMPCOLUMN          *   rgdumpcolumn    = NULL;

    columnlist.cbStruct = sizeof( columnlist );
    columnlist.tableid  = JET_tableidNil;

    Call( JetOpenTableW( sesid, dbid, wszTable, NULL, 0, JET_bitTableReadOnly, &tableid ) );

    Call( JetGetTableColumnInfo(
            sesid,
            tableid,
            NULL,
            &columnlist,
            sizeof( columnlist ),
            JET_ColInfoList ) );

    ccolumns = columnlist.cRecord;
    rgdumpcolumn = new DUMPCOLUMN[ccolumns];

    if( NULL == rgdumpcolumn )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }

    ULONG ccolumnsT;
    Call( ErrDumpcolumnFromColumnlist(
            sesid,
            columnlist,
            rgdumpcolumn,
            ccolumns,
            ccolumnsT ) );
    assert( ccolumnsT == (ULONG)ccolumns );

    Call( ErrEDBUTLDumpTableRecords(
            sesid,
            tableid,
            rgdumpcolumn,
            ccolumns,
            crecordsMax ) );

HandleError:
    if( rgdumpcolumn )
    {
        delete [] rgdumpcolumn;
    }
    if( JET_tableidNil != columnlist.tableid )
    {
        (void)JetCloseTable( sesid, columnlist.tableid );
    }
    if( JET_tableidNil != tableid )
    {
        (void)JetCloseTable( sesid, tableid );
    }
    return err;
}


IOREASON g_iorThunk( (IOREASONPRIMARY) 1 );

//  ================================================================
LOCAL JET_ERR ErrEDBUTLDumpFTLHeader( _In_ const WCHAR * const wszFTLFile )
//  ================================================================
{
    JET_ERR         err         = JET_errSuccess;
#ifdef MINIMAL_FUNCTIONALITY
#else  //  !MINIMAL_FUNCTIONALITY
    CFastTraceLog *                 pftl = NULL;
    CFastTraceLog::CFTLReader *     pftlr = NULL;

    Alloc( pftl = new CFastTraceLog( NULL ) );

    Call( pftl->ErrFTLInitReader( wszFTLFile,
                                    &g_iorThunk,
                                    CFastTraceLog::ftlifNone,
                                    &pftlr ) );

    // consider dumping something more intelligent for each schema ID, like "BF" ...
    //pftl->PftlhdrFTLTraceLogHeader()->le_ulSchemaIDs
    
    Call( pftl->ErrFTLDumpHeader() );

HandleError:

    if ( err )
    {
        wprintf( L"ERROR: Processing FTL file header, %d\n", err );
    }

    if ( pftl )
    {
        pftl->FTLTerm();    // cleanup open files
        delete pftl;        // cleanup memory
        pftl = NULL;
        pftlr = NULL;
    }
#endif  //  MINIMAL_FUNCTIONALITY
    return err;
}


LOCAL DWORD GetAttributeListSize( const WCHAR* const wszFilename, INT *pAttrListSize )
{

    DWORD           dwGLE = 0;
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    WCHAR           wszAttrList[MAX_PATH];
    LARGE_INTEGER   li;
    Assert( pAttrListSize != NULL );

    wcscpy_s( wszAttrList, _countof( wszAttrList ), wszFilename );
    wcscat_s( wszAttrList, _countof( wszAttrList ), L"::$ATTRIBUTE_LIST" );
    hFile = CreateFileW( wszAttrList, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        dwGLE = GetLastError();
        goto HandleError;
    }

    if ( !GetFileSizeEx( hFile, &li ) )
    {
        dwGLE = GetLastError();
        goto HandleError;
    }

    Assert( li.HighPart == 0 );
    *pAttrListSize = li.LowPart;

HandleError:
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    return dwGLE;
}

LOCAL DWORD DumpExtentCount( const WCHAR* const wszFilename, const DWORD cbCluster, const BOOL fStatistics, const BOOL fEnumerateExtents )
{
    DWORD                       dwGLE = 0;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    HANDLE                      hProcHeap = NULL;
    void*                       pData = NULL;
    ULONG                       extentCount = 0;
    LONGLONG                    ibExtentEndLast = 0;
    STARTING_VCN_INPUT_BUFFER   startingVCN = { 0 };
    ULONG                       dataSize = 0x100000;
    ULONG                       retSize = 0;
    const BOOL                  fPrintStatistics = fStatistics || fEnumerateExtents;
    CPerfectHistogramStats      histo;
    BOOL                        fStatError = fFalse;

    hFile = CreateFileW( wszFilename, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        dwGLE = GetLastError();
        goto HandleError;
    }

    hProcHeap = GetProcessHeap();
    pData = HeapAlloc( hProcHeap, 0, dataSize );
    if ( pData == NULL )
    {
        //  Can't use GetLastError for HeapAlloc, oddly enough.
        dwGLE = ERROR_NOT_ENOUGH_MEMORY;
        goto HandleError;
    }

    if ( fEnumerateExtents )
    {
        wprintf( L"  Extents (bytes):" );
    }

    do
    {
        dwGLE = 0;
        if ( !DeviceIoControl(
            hFile,
            FSCTL_GET_RETRIEVAL_POINTERS,
            &startingVCN, sizeof( startingVCN ),
            (RETRIEVAL_POINTERS_BUFFER *) pData, dataSize,
            &retSize, NULL ) )
        {
            dwGLE = GetLastError();
        }

        Assert( dwGLE != ERROR_INSUFFICIENT_BUFFER );   // Shouldn't return that with a 1MB buffer.
        if ( dwGLE == ERROR_MORE_DATA || dwGLE == ERROR_SUCCESS )
        {
            RETRIEVAL_POINTERS_BUFFER *pRetPtrs = (RETRIEVAL_POINTERS_BUFFER*)pData;

            //  Go through all the extents to accumulate and/or print out detailed information.
            for ( ULONG iExtent = 0; iExtent < pRetPtrs->ExtentCount; iExtent++ )
            {
                const LONGLONG iVCN = startingVCN.StartingVcn.QuadPart;
                const LONGLONG cbExtent = ( pRetPtrs->Extents[iExtent].NextVcn.QuadPart - iVCN ) * cbCluster;
                const LONGLONG iLCN = pRetPtrs->Extents[iExtent].Lcn.QuadPart;

                startingVCN.StartingVcn = pRetPtrs->Extents[iExtent].NextVcn;

                //  There is a special case where LCN is negative, which happens when a file is compressed.
                //  In this case, the extent with a negative LCN is actually an extension of the previous
                //  LCN to describe the compressed extent.
                if ( iLCN == -1 )
                {
                    Assert( extentCount > 0 );
                    wprintf( L" (c)" );
                }
                else if ( iLCN >= 0 )
                {
                    const LONGLONG ibExtent = iLCN * cbCluster;
                    const LONGLONG ibFile = iVCN * cbCluster;

                    //  If this is the first extent, we must initialize the last offset.
                    if ( extentCount == 0 )
                    {
                        ibExtentEndLast = ibExtent;
                    }

                    const LONGLONG ibExtentDelta = ibExtent - ibExtentEndLast;
                    ibExtentEndLast = ibExtent + cbExtent;

                    if ( fPrintStatistics && !fStatError )
                    {
                        if ( histo.ErrAddSample( (SAMPLE)cbExtent ) != CStats::ERR::errSuccess )
                        {
                            fStatError = fTrue;
                        }
                    }

                    if ( fEnumerateExtents )
                    {
                        wprintf(    L"%c    FileOffset:%013I64d Offset:%013I64d Length:%013I64d Delta:%+013I64d",
                                    wchNewLine,
                                    ibFile,
                                    ibExtent,
                                    cbExtent ,
                                    ibExtentDelta );
                    }

                    extentCount++;
                }
                else
                {
                    AssertSz( fFalse, "Unrecognized iLCN (%I64d)", iLCN );
                    dwGLE = ERROR_INTERNAL_ERROR;;
                    goto HandleError;
                }
            }
        }
    }
    while ( dwGLE == ERROR_MORE_DATA );

    if ( fEnumerateExtents )
    {
        wprintf( L"%c", wchNewLine );
    }

    wprintf( L"  Extents Enumerated: %ld%c", extentCount, wchNewLine );

    if ( fPrintStatistics && !fStatError && dwGLE == ERROR_SUCCESS && extentCount > 0 )
    {
        CStats::ERR errStats;
        
        wprintf( L"  Extent Statistics:%c", wchNewLine );
        wprintf( L"    Smallest: %.3f MB%c", DblMBs( histo.Min() ), wchNewLine );
        wprintf( L"    Largest: %.3f MB%c", DblMBs( histo.Max() ), wchNewLine );
        wprintf( L"    Average: %.3f MB%c", DblMBs( histo.Ave() ), wchNewLine );
        wprintf( L"    Total Size: %.3f MB%c", DblMBs( histo.Total() ), wchNewLine );

        wprintf( L"    Histogram (MB,Extents):%c", wchNewLine );
        const DWORD cHistoSegments = 20;
        const SAMPLE cbHistoExtentSegment = ( max( ( histo.Max() - histo.Min() ) / cHistoSegments / cbCluster, 1 ) ) * cbCluster;
        SAMPLE cbHistoExtentLast = ( histo.Min() / cbCluster ) * cbCluster;
        SAMPLE cbHistoExtent = cbHistoExtentLast + cbHistoExtentSegment;
        CHITS cHistoHits;
        errStats = histo.ErrReset();
        Assert( errStats == CStats::ERR::errSuccess );
        while ( ( errStats = histo.ErrGetSampleHits( cbHistoExtent, &cHistoHits ) ) == CStats::ERR::errSuccess )
        {
            wprintf( L"      Lengths:%11.3f-%11.3f Extents:%I64d%c", DblMBs( cbHistoExtentLast ), DblMBs( cbHistoExtent ), cHistoHits, wchNewLine );
            cbHistoExtentLast = cbHistoExtent + 1;
            cbHistoExtent += cbHistoExtentSegment;
            cbHistoExtent = min( cbHistoExtent, histo.Max() );
        }
        Assert( errStats == CStats::ERR::wrnOutOfSamples );

        wprintf( L"    Percentiles (%% Extents,MB):%c", wchNewLine );
        const DWORD cPercentileSegments = 20;
        const ULONG pctPercentileSegment = 100 / cPercentileSegments;
        Assert( pctPercentileSegment > 0 );
        errStats = histo.ErrReset();
        Assert( errStats == CStats::ERR::errSuccess );
        ULONG pctPercentile = pctPercentileSegment;
        ULONG pctPercentileLast = ulMax;
        while ( true )
        {
            SAMPLE cbPercentileExtent;
            const ULONG pctPercentileT = pctPercentile;
            errStats = histo.ErrGetPercentileHits( &pctPercentile, &cbPercentileExtent );
            Assert( errStats == CStats::ERR::errSuccess );
            Assert( pctPercentile <= 100 );

            //  Don't print out repeated percentiles.
            if ( pctPercentile != pctPercentileLast )
            {
                wprintf( L"      Percentile:%-3u Length:%11.3f%c", pctPercentile, DblMBs( cbPercentileExtent ), wchNewLine );
                pctPercentileLast = pctPercentile;
            }

            //  We are done.
            if ( pctPercentile >= 100 )
            {
                Expected( pctPercentile == 100 );
                break;
            }

            //  Can never go back.
            while ( pctPercentile <= pctPercentileT )
            {
                pctPercentile = ( ( pctPercentile + pctPercentileSegment ) / pctPercentileSegment ) * pctPercentileSegment;
            }
        }
    }

HandleError:
    if ( pData )
    {
        HeapFree( hProcHeap, 0, pData );
    }
    
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
    
    return dwGLE;
}

ERR LOCAL ErrDUMPFileSize(
    _In_ const QWORD cbSizeLogical,
    _In_ const QWORD cbSizeOnDisk )
{
    ERR             err     = JET_errSuccess;

    Assert( cbSizeLogical >= cbSizeOnDisk );
    Assert( cbSizeLogical > 0 );
    const QWORD cbSizeSaved = ( cbSizeLogical - cbSizeOnDisk );

    wprintf( L"  Logical Size: 0x%08I64x bytes (%I64d kB)%c", cbSizeLogical, cbSizeLogical / 1024, wchNewLine );
    wprintf( L"  On-disk Size: 0x%08I64x bytes (%I64d kB)%c", cbSizeOnDisk, cbSizeOnDisk / 1024, wchNewLine );
    wprintf( L"   Space Saved: 0x%08I64x bytes (%I64d kB) (%2d %%)%c", cbSizeSaved , cbSizeSaved / 1024, (INT) ( ( cbSizeSaved * 100 ) / cbSizeLogical ), wchNewLine );

    return err;
}

LOCAL JET_ERR ErrFileSystemDump( const WCHAR* const wszFilename, const BOOL fVerbose, const BOOL fDebugMode )
{
    JET_ERR         err = JET_errSuccess;
    IFileSystemAPI* pFSApi = NULL;
    IFileAPI *      pfapi = NULL;
    WCHAR           wszVolumeRoot[MAX_PATH];
    WCHAR           wszVolumeName[MAX_PATH];
    DWORD           dwVolumeSerial = 0;
    DWORD           dwFSFlags = 0;
    WCHAR           wszFSName[50];
    INT             attrListSize = 0;
    UINT            uiDriveType = 0;
    DWORD           cbSector = 0;
    DWORD           cbCluster = 0;
    DWORD           dwDummy = 0;

    Call( ErrOSFSCreate( &pFSApi ) );
    Call( pFSApi->ErrPathRoot( wszFilename, wszVolumeRoot ) );

    if ( !GetVolumeInformationW(    wszVolumeRoot,
                                    wszVolumeName,
                                    _countof( wszVolumeName ),
                                    &dwVolumeSerial,
                                    NULL,
                                    &dwFSFlags,
                                    wszFSName,
                                    _countof( wszFSName ) ) )
    {
        Call( pFSApi->ErrGetLastError( GetLastError() ) );
    }

    if ( !GetDiskFreeSpaceW(    wszVolumeRoot,
                                &cbCluster,
                                &cbSector,
                                &dwDummy,
                                &dwDummy ) )
    {
        Call( pFSApi->ErrGetLastError( GetLastError() ) );
    }
    cbCluster *= cbSector;

    wprintf( L"File Information:%c", wchNewLine );
    wprintf( L"  File Name: %s%c", wszFilename, wchNewLine );
    wprintf( L"  Volume Name: %s%c", 0 == LOSStrLengthW( wszVolumeName ) ? wszVolumeRoot : wszVolumeName, wchNewLine );
    wprintf( L"  File System: %s%c", wszFSName, wchNewLine );
    wprintf( L"  Cluster Size: %u bytes%c", cbCluster, wchNewLine );

    if ( 0 == _wcsicmp( wszFSName, L"NTFS" ) )
    {
        // print attribute list size        
        err = pFSApi->ErrGetLastError( GetAttributeListSize( wszFilename, &attrListSize ) );
        if ( err == JET_errFileNotFound )
        {
            attrListSize = -1;
        }
        else
        {
            Call( err );
        }

        wprintf( L"  NTFS Attribute List Size: " );
        if ( attrListSize >= 10 * 1024 )
        {
            wprintf( L"%ld KB%c", attrListSize / 1024, wchNewLine );
        }
        else if ( attrListSize >= 0 )
        {
            wprintf( L"%ld bytes%c", attrListSize, wchNewLine );
        }
        else
        {
            wprintf( L"N/A%c", wchNewLine );
        }
    }

    if ( 0 == _wcsicmp( wszFSName, L"NTFS" ) || 0 == _wcsicmp( wszFSName, L"ReFS" ) )
    {
        // Print extent count
        // This uses FSCTL_GET_RETRIEVAL_POINTERS
        // Fails on network shares (DRIVE_REMOTE)
        uiDriveType = GetDriveTypeW( wszVolumeRoot );
        if ( uiDriveType != DRIVE_UNKNOWN && uiDriveType != DRIVE_REMOTE )
        {
            Call( pFSApi->ErrGetLastError( DumpExtentCount( wszFilename, cbCluster, fVerbose, fDebugMode ) ) );
        }

        {
            QWORD cbSizeOnDisk          = 0;
            QWORD cbSizeLogical         = 0;

            Call( pFSApi->ErrFileOpen( wszFilename, IFileAPI::fmfReadOnly, &pfapi ) );
            Call( pfapi->ErrSize( &cbSizeLogical, IFileAPI::filesizeLogical ) );
            Call( pfapi->ErrSize( &cbSizeOnDisk, IFileAPI::filesizeOnDisk ) );
            Call( ErrDUMPFileSize( cbSizeLogical, cbSizeOnDisk ) );
        }
    }

HandleError:
    if ( err != JET_errSuccess )
    {
        wprintf( L"Error while retrieving file system information.%c", wchNewLine );
    }

    wprintf( L"%c", wchNewLine );

    delete pfapi;
    delete pFSApi;

    return err;
}

LOCAL JET_ERR ErrEDBUTLDump(
    JET_DBUTIL_W *  pdbutil,
    UTILOPTS *      popts )
{
    JET_ERR         err         = JET_errSuccess;
    JET_INSTANCE    instance    = 0;

    assert( NULL != pdbutil );
    assert( NULL != popts );

    wprintf( L"%c", wchNewLine );

    Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"off" ) );
    Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );

    //  set temp table size to be 0 so that no temp db will be created

    Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 0, NULL ) );

    //  set user overrides.

    Call( ErrEDBUTLUserSystemParameters( &instance, popts ) );

    if ( opDBUTILDumpFTLHeader == pdbutil->op )
    {
        //  we want to handle this one externally
        Call( ErrEDBUTLDumpFTLHeader( popts->wszSourceDB ) );
    }
    else
    {
        if ( opDBUTILDumpSpaceCategory == pdbutil->op )
        {
            pdbutil->spcatOptions.szDatabase = popts->wszSourceDB;
        }
        else if ( opDBUTILDumpRBSPages == pdbutil->op )
        {
            pdbutil->rbsOptions.szDatabase = popts->wszSourceDB;
        }
        else
        {
            pdbutil->szDatabase = popts->wszSourceDB;
        }

        if ( opDBUTILDumpSpace == pdbutil->op )
        {

            ULONG cbPageSize = 0;
            if ( FUTILOPTS2KPage( popts->fUTILOPTSFlags ) )
            {
                cbPageSize = 2 * 1024;
            }
            else if ( FUTILOPTS4KPage( popts->fUTILOPTSFlags ) )
            {
                cbPageSize = 4 * 1024;
            }
            if ( FUTILOPTS8KPage( popts->fUTILOPTSFlags ) )
            {
                cbPageSize = 8 * 1024;
            }
            else if ( FUTILOPTS16KPage( popts->fUTILOPTSFlags ) )
            {
                cbPageSize = 16 * 1024;
            }
            else if ( FUTILOPTS32KPage( popts->fUTILOPTSFlags ) )
            {
                cbPageSize = 32 * 1024;
            }

            Call( ErrSpaceDumpCtxSetOptions( pdbutil->pvCallback, &cbPageSize, NULL, NULL, fSPDumpNoOpts ) );
            Call( ErrSpaceDumpCtxGetGRBIT( pdbutil->pvCallback, &(pdbutil->grbitOptions) ) );
        }
        else
        {
             if ( opDBUTILDumpSpaceCategory != pdbutil->op )
             {
                // why were we setting it to NULL here for all dump operations?
                pdbutil->pfnCallback = NULL;
             }
        }

        Call( JetDBUtilitiesW( pdbutil ) );
    }

HandleError:

    if ( opDBUTILDumpSpace == pdbutil->op )
    {
        //  Note this prints final statistics if necessary, as well as deallocates.
        err = ErrSpaceDumpCtxComplete( pdbutil->pvCallback, err /* passes through */ );
    }

    return err;
}

LOCAL JET_ERR ErrEDBUTLCheckLogStream( JET_INSTANCE* pInst, UTILOPTS* pOpts )
{
    JET_ERR err = JET_errSuccess;

    if (    modeRecovery != pOpts->mode ||
            FUTILOPTSExplicitPageSet( pOpts->fUTILOPTSFlags ) )
    {
        // no need to do auto-detection
        return JET_errSuccess;
    }

    //================================
    // file name (with wild card)
    WCHAR wszFName[ _MAX_PATH + 1 ];
    Call( JetGetSystemParameterW( *pInst, JET_sesidNil, JET_paramBaseName, NULL, wszFName, sizeof( wszFName ) ) );
    StringCchCatW( wszFName, _countof( wszFName ), L"*" );

    //================================
    // drive and dir
    WCHAR wszLogFilePath[ _MAX_PATH + 1 ];
    Call( JetGetSystemParameterW( *pInst, JET_sesidNil, JET_paramLogFilePath, NULL, wszLogFilePath, sizeof( wszLogFilePath ) ) );

    WCHAR wszDrive[ _MAX_DRIVE ];
    WCHAR wszDir[ _MAX_DIR ];
    _wsplitpath_s( wszLogFilePath, wszDrive, _countof( wszDrive ), wszDir, _countof( wszDir ), NULL, 0, NULL, 0 );

    //================================
    // ext
    DWORD_PTR ulLegacy = 0;
    Call( JetGetSystemParameterW( *pInst, JET_sesidNil, JET_paramLegacyFileNames, &ulLegacy, NULL, sizeof( ulLegacy ) ) );

    //================================
    // try both log extensions
    const WCHAR* rgwszExt[] = { L"log", L"jtx", };
    INT iExtIdx = !( ulLegacy & JET_bitESE98FileNames );

    assert( 2 == _countof( rgwszExt ) );    // because of "iExtIdx = 1 - iExtIdx"
    for ( size_t i = 0; i < _countof( rgwszExt ); ++i, iExtIdx = 1 - iExtIdx )
    {
        WCHAR wszPath[ _MAX_PATH ];
        _wmakepath_s( wszPath, _countof( wszPath ), wszDrive, wszDir, wszFName, rgwszExt[ iExtIdx ] );

        //================================
        // scan all log files to find database page size
        BOOL fFound = fTrue;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATAW wfd;
        for ( hFind = FindFirstFileW( wszPath, &wfd );
            INVALID_HANDLE_VALUE != hFind && fFound;
            fFound = FindNextFileW( hFind, &wfd ) )
        {
            WCHAR wszExt[ _MAX_EXT ];
            _wsplitpath_s( wfd.cFileName, NULL, 0, NULL, 0, wszFName, _countof( wszFName ), wszExt, _countof( wszExt ) );
            _wmakepath_s( wszLogFilePath, _countof( wszLogFilePath ), wszDrive, wszDir, wszFName, wszExt );
            
            // database page size
            JET_LOGINFOMISC logInfo;
            if ( JET_errSuccess <= JetGetLogFileInfoW( wszLogFilePath, &logInfo, sizeof( logInfo ), JET_LogInfoMisc ) )
            {
                FindClose( hFind );
                Call( JetSetSystemParameterW( pInst, 0, JET_paramDatabasePageSize, logInfo.cbDatabasePageSize, NULL ) );
                return JET_errSuccess;
            }
        }

        FindClose( hFind );
    }

HandleError:
    return err;
}

#ifndef DEBUG
#define PushEseutilArgTrace( argc, argv )
#else
void PushEseutilArgTrace( INT argc, __in_ecount(argc) LPWSTR argv[] )
{
    size_t cbAllArgs = 0;
    for( INT iarg = 0; iarg < argc; iarg++ )
    {
        cbAllArgs += sizeof(WCHAR) * ( LOSStrLengthW( argv[iarg] ) + 2 /* one for space, one more because of .... ahhhh<fell off SOMEONE> */ );
    }
    cbAllArgs += sizeof(WCHAR) + 783; // + NUL terminator space + Buffer for potential MBCS expansion (of 3 full paths) ... at top of stack, so 783 is fine.
    CHAR * szEseutilCmd = (CHAR*)alloca( cbAllArgs );
    if ( szEseutilCmd )
    {
        CHAR * szT = szEseutilCmd;

        (void)ErrOSStrCbFormatA( szT, cbAllArgs, "Command:     \"" );   // 5 spaces is _perfect_. Excercise to reader as to why.
        size_t cbUsed = strlen( szT );
        szT += cbUsed;
        cbAllArgs -= cbUsed;

        for( INT iarg = 0; iarg < argc; iarg++ )
        {
            //  Not sure if MBCS can expand this, but left buffer anyways, may truncate but we'd survive.
            (void)ErrOSStrCbFormatA( szT, cbAllArgs, "%ws ", argv[iarg] );
            cbUsed = strlen( szT );
            szT += cbUsed;
            cbAllArgs -= cbUsed;
        }
        if ( argc )
        {
            //  if we had any args wipe out the last space with the end quote.
            (void)ErrOSStrCbFormatA( szT - 1, cbAllArgs + 1, "\"" );
        }
        JET_TESTHOOKTRACETESTMARKER mark = { sizeof(mark), szEseutilCmd, 1 };
        JetTestHook( opTestHookTraceTestMarker, &mark );
    }
}
#endif

INT __cdecl wmain( INT argc, __in_ecount(argc) LPWSTR argv[] )
{
    JET_INSTANCE            instance                = 0;
    JET_SESID               sesid                   = JET_sesidNil;
    JET_ERR                 err                     = JET_errSuccess;
    JET_ERR                 errRepaired             = JET_errSuccess;
    BOOL                    fResult                 = fTrue;
    UTILOPTS                opts                    = {0};
    JET_DBUTIL_W            dbutil;
    INT                     iSec, iMSec;
    BOOL                    fWhitespaceOnErr        = fFalse;
    BOOL                    fUnknownError           = fFalse;
    JET_RSTINFO2_W          rstInfo;
#ifdef ESESHADOW
    BOOL                    fMountedShadow          = fFalse;
#endif
    COSLayerPreInit         oslayer;                // FOSPreinit()

    memset( &opts, 0, sizeof(UTILOPTS) );

    ULONG                   timer                   = 0;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        return ErrERRCheck( JET_errOutOfMemory );
    }

    //  configure OS layer

    COSLayerPreInit::DisablePerfmon();  //  for now we will avoid dealing with perfmon, note ese[nt].dll will still use perfmon, just not eseutil.exe
    COSLayerPreInit::DisableTracing();  //  for now we will avoid dealing with tracing, note ese[nt].dll will still use tracing, just not eseutil.exe

    Call( (JET_ERR)ErrOSInit() );

    //  must be after init so that Time Inj has a chance to adjust the ticks
    //  we're using

    timer = TickOSTimeCurrent();


#ifdef USE_WATSON_API
    // Register Watson for error reporting.
    RegisterWatson();
#endif

    //  Force the process to terminate if heap corruption is detected
    //
    HeapSetInformation( NULL, HeapEnableTerminationOnCorruption, NULL, 0 );

    PushEseutilArgTrace( argc, argv );

    InitArg( argc, argv );
    opts.lDirtyLevel = 2;
    memset( &dbutil, 0, sizeof(JET_DBUTIL_W) );
    dbutil.cbStruct = sizeof( dbutil );

    memset( &rstInfo, 0, sizeof( rstInfo ) );
    rstInfo.cbStruct = sizeof( rstInfo );

    wprintf( L"%c", wchNewLine );

    const INT pid = GetCurrentProcessId();

    if ( GetArgCount() < 2 )
    {
        wprintf( L"%s%c%c", wszUsageErr9, wchNewLine, wchNewLine );
        goto Usage;
    }

    SetCurArgID(1);
    assert( GetCurArgID() == 1 );
    if ( wcschr( wszSwitches, GetCurArg()[0] ) == NULL )
    {
        wprintf( L"%s%c%c", wszUsageErr10, wchNewLine, wchNewLine );
        goto Usage;
    }
    swprintf_s( wszDefaultTempDB, RTL_NUMBER_OF( wszDefaultTempDB ), wszDefaultTempDBFormat, pid );
    swprintf_s( wszDefaultDefragDB, RTL_NUMBER_OF( wszDefaultDefragDB ), wszDefaultDefragDBFormat, pid );
    swprintf_s( wszDefaultUpgradeDB, RTL_NUMBER_OF( wszDefaultUpgradeDB ), wszDefaultUpgradeDBFormat, pid );
    swprintf_s( wszDefaultRepairDB, RTL_NUMBER_OF( wszDefaultRepairDB ), wszDefaultRepairDBFormat, pid );
    swprintf_s( wszDefaultIntegDB, RTL_NUMBER_OF( wszDefaultIntegDB ), wszDefaultIntegDBFormat, pid );
    swprintf_s( wszDefaultScrubDB, RTL_NUMBER_OF( wszDefaultScrubDB ), wszDefaultScrubDBFormat, pid );
    swprintf_s( wszDefaultChecksumDB, RTL_NUMBER_OF( wszDefaultChecksumDB ), wszDefaultChecksumDBFormat, pid );

    assert( NULL == opts.pv );

    switch( GetCurArg()[1] )
    {
        case L'd':      // Defragment
        case L'D':
            opts.mode   = modeDefragment;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseDefragment );
            break;

        case L'r':      // Recovery
        case L'R':
            opts.mode = modeRecovery;
            opts.grbitInit = JET_bitLogStreamMustExist;
            opts.pv = &rstInfo;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseRecovery );
            break;

        case L'g':      // inteGrity
        case L'G':
            opts.mode = modeIntegrity;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseIntegrity );
            break;

        case L'k':      // esefile - checKsum
        case L'K':
            opts.mode = modeChecksum;
            // by default try to checksum EDB
            UTILOPTSSetChecksumEDB( opts.fUTILOPTSFlags );
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseChecksum );
            break;

        case L'p':      // rePair
        case L'P':
            opts.mode   = modeRepair;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseRepair );
            break;

        case L'm':      // file duMp.
        case L'M':
            opts.mode = modeDump;
            opts.pv = &dbutil;

            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseDump );
            break;

        case L'y':      // esefile - copYfile
        case L'Y':
            opts.mode = modeCopyFile;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseCopyFile );
            break;

#ifndef ESENT
        case L'c':      // Hard Recovery (dump Restore.Env or/and RestoreComplete)
        case L'C':
            opts.mode = modeHardRecovery;
            opts.pv = &dbutil;

            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseHardRecovery );
            break;
#endif // ESENT

#ifdef DEBUG
        case L'b':      // Backup
        case L'B':
            opts.mode = modeBackup;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseBackup );
            break;
#endif

        case L'z':      // Zero out deleted portions of the database
        case L'Z':
            opts.mode   = modeScrub;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseScrub );
            break;

        case L'?':
            goto Usage;

            //  else FALLTHRU
        default:
            wprintf( L"%s%c%c", wszUsageErr12, wchNewLine, wchNewLine );
            fResult = fFalse;
    }

    if ( !fResult )
        goto Usage;

    // turn on legacy config if implemented (or use fast config if ever created)
    (void)JetSetSystemParameterW( &instance, 0, JET_paramConfiguration, 1, NULL );

    if ( opts.wszConfigStoreSpec )
    {
        // arg is of form /config:reg:HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\ESENT\IntegCheck
        err = JetSetSystemParameterW( &instance, 0, JET_paramConfigStoreSpec, 0, opts.wszConfigStoreSpec );
        if ( err < JET_errSuccess )
        {
            wprintf( wszUsageErr21 );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            Call( err );
        }
    }

    Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"ESEUTIL" ) );

    //  enable persisted callbacks because we have to be able to maintain
    //  databases where they are used

    Call( JetSetSystemParameterW( &instance, 0, JET_paramEnablePersistedCallbacks, 1, NULL ) );

    //  generate a new temporary database name
    //  this may be overwritten later

    Call( JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, wszDefaultTempDB ) );

    //  enable persisted lost flush detection
    Call( JetSetSystemParameter( &instance, 0, JET_paramPersistedLostFlushDetection, JET_bitPersistedLostFlushDetectionEnabled, NULL ) );

    if ( !FUTILOPTSSuppressLogo( opts.fUTILOPTSFlags ) )
    {
        EDBUTLPrintLogo();
    }

    if ( modeHardRecovery == opts.mode || modeBackup == opts.mode )
    {
        // these modes cannot auto-detect page size
        //
        //  at this point all commandline options have been parsed.  if a page size
        //  wasn't chosen then choose the default
        //
        if ( !FUTILOPTSExplicitPageSet( opts.fUTILOPTSFlags ) )
        {
            switch ( cbPageDefault )
            {
                case 2 * 1024:
                    UTILOPTSSet2KPage( opts.fUTILOPTSFlags );
                    break;
                case 4 * 1024:
                    UTILOPTSSet4KPage( opts.fUTILOPTSFlags );
                    break;
                case 8 * 1024:
                    UTILOPTSSet8KPage( opts.fUTILOPTSFlags );
                    break;
                case 16 * 1024:
                    UTILOPTSSet16KPage( opts.fUTILOPTSFlags );
                    break;
                case 32 * 1024:
                    UTILOPTSSet32KPage( opts.fUTILOPTSFlags );
                    break;
            }
        }
    }
    
    if ( FUTILOPTS2KPage( opts.fUTILOPTSFlags ) )
    {
        Call( JetSetSystemParameterW( &instance, 0, JET_paramDatabasePageSize, 2 * 1024, NULL ) );
    }
    else if ( FUTILOPTS4KPage( opts.fUTILOPTSFlags ) )
    {
        Call( JetSetSystemParameterW( &instance, 0, JET_paramDatabasePageSize, 4 * 1024, NULL ) );
    }
    if ( FUTILOPTS8KPage( opts.fUTILOPTSFlags ) )
    {
        Call( JetSetSystemParameterW( &instance, 0, JET_paramDatabasePageSize, 8 * 1024, NULL ) );
    }
    else if ( FUTILOPTS16KPage( opts.fUTILOPTSFlags ) )
    {
        Call( JetSetSystemParameterW( &instance, 0, JET_paramDatabasePageSize, 16 * 1024, NULL ) );
    }
    else if ( FUTILOPTS32KPage( opts.fUTILOPTSFlags ) )
    {
        Call( JetSetSystemParameterW( &instance, 0, JET_paramDatabasePageSize, 32 * 1024, NULL ) );
    }

    Call( ErrEDBUTLSetCacheSizeMax( &opts ) );

    // Lights, cameras, action...
    timer = TickOSTimeCurrent();

#ifdef ESESHADOW
    WCHAR wszShadowedFile[ _MAX_PATH ];
    WCHAR wszShadowedLog[ _MAX_PATH ];
    WCHAR wszShadowedSystem[ _MAX_PATH ];

    if ( FUTILOPTSOperateTempVss( opts.fUTILOPTSFlags ) &&
        NULL != opts.wszSourceDB )
    {

        //
        //  User requested us work off snapshot data
        //

        wprintf( L"Initializing VSS subsystem...%c%c", wchNewLine, wchNewLine );

        HRESULT hr = DelayLoadEseShadowInit( &(opts.eseShadowContext) );
        if ( hr == hrEseSnapshotNotYetImplemented )
        {
            wprintf( L"   VSS Subsystem Init failed, requires esevss.dll and OS support (Windows 2008 or higher).\n" );
            Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
        }
        if ( !SUCCEEDED( hr ) )
        {
            wprintf( L"   VSS Subsystem Init failed, 0x%x\n", hr );
            Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
        }

        assert( opts.eseShadowContext );

        if ( FUTILOPTSOperateTempVssRec( opts.fUTILOPTSFlags ) )
        {
            hr = DelayLoadEseShadowCreateShadow( opts.eseShadowContext, opts.wszSourceDB, opts.wszLogfilePath, opts.wszSystemPath, opts.wszBase, fTrue, fTrue );
        }
        else
        {
            hr = DelayLoadEseShadowCreateSimpleShadow( opts.eseShadowContext, opts.wszSourceDB );
        }
        if ( !SUCCEEDED( hr ) )
        {
            wprintf( L"   VSS Subsystem Init failed, 0x%x\n", hr );
            Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
        }

        if ( FUTILOPTSOperateTempVssRec( opts.fUTILOPTSFlags ) )
        {
            hr = DelayLoadEseShadowMountShadow(
                        opts.eseShadowContext,
                        wszShadowedFile,
                        _countof( wszShadowedFile ),
                        wszShadowedLog,
                        _countof( wszShadowedLog ),
                        wszShadowedSystem,
                        _countof( wszShadowedSystem ) );
        }
        else
        {
            hr = DelayLoadEseShadowMountSimpleShadow( opts.eseShadowContext, wszShadowedFile, _countof( wszShadowedFile ) );
        }
        if ( !SUCCEEDED( hr ) )
        {
            wprintf( L"   VSS Subsystem Init failed, 0x%x\n", hr );
            Error( ErrERRCheck( JET_errOSSnapshotNotAllowed ) );
        }

        //  Snapshot created and mounted, configure DB utility options for this ...
        //

        fMountedShadow = fTrue;
        opts.wszSourceDB = wszShadowedFile;
        if ( FUTILOPTSOperateTempVssRec( opts.fUTILOPTSFlags ) )
        {
            opts.wszLogfilePath = wszShadowedLog;
        }
    }
#endif

    switch ( opts.mode )
    {
        case modeRecovery:
            if ( NULL == opts.wszBase )
            {
                wprintf( wszUsageErr1, L"logfile base name" );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                Call( ErrERRCheck( JET_errInvalidParameter ) );
            }

            wprintf( L"Initiating RECOVERY mode...%c", wchNewLine );
            wprintf( L"    Logfile base name: %s%c", opts.wszBase, wchNewLine );

            if( 0 != opts.pageTempDBMin )
            {
                wprintf( L"   Temp database size: %d%c", opts.pageTempDBMin, wchNewLine );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramPageTempDBMin , opts.pageTempDBMin, NULL ) );
            }

            wprintf( L"            Log files: %s%c", opts.wszLogfilePath ? opts.wszLogfilePath : L"<current directory>", wchNewLine );
            wprintf( L"         System files: %s%c", opts.wszSystemPath ? opts.wszSystemPath : L"<current directory>", wchNewLine );

            if ( NULL != opts.wszDbAltRecoveryDir )
            {
                wprintf( L"   Database Directory: %s%c", opts.wszDbAltRecoveryDir != wszCurrDir ? opts.wszDbAltRecoveryDir : L"<current directory>", wchNewLine );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramAlternateDatabaseRecoveryPath, 0, opts.wszDbAltRecoveryDir ) );
            }

            if ( FUTILOPTSDelOutOfRangeLogs( opts.fUTILOPTSFlags ) )
            {
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDeleteOutOfRangeLogs, fTrue, NULL ) );
            }

            if ( 0 != opts.irstmap )
            {
                LONG iDb;
                for( iDb = 0 ; iDb < opts.irstmap; iDb++ )
                {
                    assert( opts.prstmap[iDb].szNewDatabaseName );
                    wprintf( L"   New Database Location: %s", opts.prstmap[iDb].szNewDatabaseName ? opts.prstmap[iDb].szNewDatabaseName : L"<empty>" );
                    if ( opts.prstmap[iDb].szDatabaseName )
                    {
                        wprintf( L", Original Database Location: %s%c",
                            opts.prstmap[iDb].szDatabaseName, wchNewLine );
                    }
                    else
                    {
                        wprintf( L"%c", wchNewLine );
                    }
                }

                if ( opts.grbitInit & JET_bitReplayMissingMapEntryDB )
                {
                    wprintf( L"   Default Database Location: yes%c", wchNewLine );
                }
            }

            Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"on" ) );

            // Currently the engine defaults to having trim enabled, and a periodic task running
            // in DEBUG builds. Turn all of that off to ensure that recovery will work
            // consistently (that it replays TRIM log records even when the system parameter is off).
            Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableShrinkDatabase, JET_bitShrinkDatabaseOff, NULL ) );

            Assert( opts.efvUserSpecified == 0 ); // do not take this as an argument yet
            Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, opts.efvUserSpecified == 0 ? ( JET_efvExchange2016Rtm | JET_efvAllowHigherPersistedFormat ) : opts.efvUserSpecified, NULL ) );

///         Call( JetSetSystemParameterW( &instance, 0, JET_paramCacheSizeMax, 500, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10000, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 16, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxVerPages, 128, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramLogBuffers, 41, NULL ) );
            wprintf( L"%c", wchNewLine );

            // Set user overrides.
            Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

            // get database page size from log stream if necessary
            Call( ErrEDBUTLCheckLogStream( &instance, &opts ) );

            // allow recovery of 5-digit log files
            if ( opts.cLogDigits == 5 )
            {
                Call( JetSetSystemParameter( &instance, 0, JET_paramLegacyFileNames, JET_bitESE98FileNames | JET_bitEightDotThreeSoftCompat, NULL ) );
            }
            else if ( opts.cLogDigits == 8 )
            {
                Call( JetSetSystemParameter( &instance, 0, JET_paramLegacyFileNames, JET_bitESE98FileNames, NULL ) );
            }

            if ( opts.wszBackup == NULL )
            {
                rstInfo.crstmap = opts.irstmap;
                rstInfo.rgrstmap = opts.prstmap;

                // if there is nothing in the map, replay to default location
                if ( !opts.irstmap )
                {
                    opts.grbitInit |= JET_bitReplayMissingMapEntryDB;
                }

#ifdef ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO
                opts.grbitInit |= JET_bitRecoveryWithoutUndo;
#endif
                opts.grbitInit |= JET_bitExternalRecoveryControl;
                rstInfo.pfnCallback = ProcessRecoveryControlAndRestoreStatus;
                rstInfo.pvCallbackContext = &opts;
                // Soft recovery.
                fWhitespaceOnErr = fTrue;
                wprintf( L"Performing soft recovery..." );
                if ( FUTILOPTSVerbose( opts.fUTILOPTSFlags ) )
                {
                    wprintf( L"\n" );
                }

                err = JetInit4W( &instance, &rstInfo, opts.grbitInit );

                Assert( sesid == JET_sesidNil );

                // Attach to shrink or reclaim leaked space, if requested.
                if ( ( err >= JET_errSuccess ) &&
                    ( ( opts.grbitShrink & JET_bitShrinkDatabaseEofOnAttach ) || ( opts.fRunLeakReclaimer ) ) &&
                    ( ( err = JetBeginSessionW( instance, &sesid, NULL, NULL ) ) >= JET_errSuccess ) )
                {
                    wprintf( L"%c", wchNewLine );
                    wprintf( L"Attaching for additional database operations...%c", wchNewLine );
                    wprintf( L"             Database: %s%c", opts.wszSourceDB, wchNewLine );
                    wprintf( L"             Shrink options: 0x%I32x%c", opts.grbitShrink, wchNewLine );
                    wprintf( L"             Leakage reclaimer: %d%c", (int)opts.fRunLeakReclaimer, wchNewLine );

                    // Set DB parameters.
                    JET_SETDBPARAM rgsetdbparam[] =
                    {
                        {
                            JET_dbparamShrinkDatabaseOptions,
                            &opts.grbitShrink,
                            sizeof( opts.grbitShrink )
                        },
                        {
                            JET_dbparamLeakReclaimerEnabled,
                            &opts.fRunLeakReclaimer,
                            sizeof( opts.fRunLeakReclaimer )
                        },
                    };
                    err = JetAttachDatabase3W( sesid, opts.wszSourceDB, rgsetdbparam, _countof( rgsetdbparam ), JET_bitDbExclusive );

                    wprintf( L"Done (error code %d).%c", err, wchNewLine );
                    wprintf( L"For details about the result of the additional operations, please consult the Application event log,%c", wchNewLine );
                    wprintf( L"under the ESE/ESENT provider.%c", wchNewLine );

                    if ( err >= JET_errSuccess )
                    {
                        (void)JetDetachDatabaseW( sesid, opts.wszSourceDB );
                    }
                }

                Call( ErrEDBUTLCleanup( instance, sesid, err ) );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
            }
            else
            {

                // Hard recovery.

                if ( opts.wszRestore )
                {
                    wprintf( L"Restoring to '%s' from '%s'...", opts.wszRestore, opts.wszBackup );
                }
                else
                {
                    wprintf( L"Restoring to <current directory> from '%s'...", opts.wszBackup );
                }
                err = JetRestore2W( opts.wszBackup, opts.wszRestore, PrintStatus );
                wprintf( L"%c%c", wchNewLine, wchNewLine );
                Call( err );
            }
            break;

#ifdef DEBUG
        case modeBackup:

            Call( ErrEDBUTLCheckBackupPath( &opts ) );
            wprintf( L"Initiating BACKUP mode...%c", wchNewLine );
            wprintf( L"       Log files: %s%c", opts.wszLogfilePath ? opts.wszLogfilePath : L"<current directory>", wchNewLine );
            wprintf( L"    System files: %s%c%c", opts.wszSystemPath ? opts.wszSystemPath : L"<current directory>", wchNewLine, wchNewLine );

            Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"on" ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramCacheSizeMax, 500, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10000, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 16, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxVerPages, 128, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
///         Call( JetSetSystemParameterW( &instance, 0, JET_paramLogBuffers, 41, NULL ) );

            // Set user overrides.
            Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

            fWhitespaceOnErr = fTrue;
            if ( FUTILOPTSIncrBackup( opts.fUTILOPTSFlags ) )
            {
                wprintf( L"Performing incremental backup..." );
                Call( JetInit( &instance ) );
                CallJ( JetBackupW( opts.wszBackup, JET_bitBackupIncremental | JET_bitBackupAtomic, NULL ), Cleanup );
            }
            else
            {
                wprintf( L"Performing full backup..." );
                Call( JetInit( &instance ) );
                CallJ( JetBackupW( opts.wszBackup, JET_bitBackupAtomic, NULL ), Cleanup );
            }
            fWhitespaceOnErr = fFalse;

            wprintf( L"%c%c", wchNewLine, wchNewLine );
            Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );
            break;
#endif

        case modeRepair:
            {
                BOOL fTryDefrag = fFalse;

                Call( ErrEDBUTLCheckDBName( &opts, wszDefaultRepairDB, NULL ) );

                wprintf( L"Initiating REPAIR mode...%c", wchNewLine );
                wprintf( L"        Database: %s%c", opts.wszSourceDB, wchNewLine );
                wprintf( L"  Temp. Database: %s%c", opts.wszTempDB, wchNewLine );

                fWhitespaceOnErr = fTrue;

                Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"repair_off" ) );

                // save the default temp dbg name to restore it if needed for defrag
                WCHAR   wszTempDatabaseDefault[_MAX_PATH + 1];
                Call( JetGetSystemParameterW( instance, 0, JET_paramTempPath, 0, wszTempDatabaseDefault, sizeof(wszTempDatabaseDefault) ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, opts.wszTempDB ) );

                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableTempTableVersioning, fFalse, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDbExtensionSize, 256, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexCleanup, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramPreferredMaxOpenTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );
                //  create plenty of sessions for multi-threaded integrity/repair
                //  need to have plenty of pages to have that many sessions
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 128, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10240, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, JET_efvUsePersistedFormat, NULL ) );

                // Set user overrides.
                Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

                Call( JetInit( &instance ) );
                CallJ( JetBeginSessionW( instance, &sesid, wszUser, wszPassword ), Cleanup );

                JET_GRBIT grbit;
                grbit = 0;
                if( FUTILOPTSDontRepair( opts.fUTILOPTSFlags ) )
                {
                    grbit |= JET_bitDBUtilOptionDontRepair;
                }
                if( FUTILOPTSDumpStats( opts.fUTILOPTSFlags ) )
                {
                    grbit |= JET_bitDBUtilOptionStats;
                }
                if( FUTILOPTSRepairCheckOnly( opts.fUTILOPTSFlags ) )
                {
                    grbit |= JET_bitDBUtilOptionRepairCheckOnly;
                }
                else if( FUTILOPTSSuppressLogo( opts.fUTILOPTSFlags ) )
                {
                    // if /g is chosen, repair will pop up a message box
                    // so that a user won't accidently repair a dirty-
                    // shutdown database
                    grbit |= JET_bitDBUtilOptionSuppressLogo;
                }

                err = ErrEDBUTLRepair(
                    sesid,
                    opts.wszSourceDB,
                    opts.wszBackup,
                    (WCHAR *)(opts.pv),
                    opts.wszIntegPrefix,
                    PrintStatus,
                    grbit );

                // if the repair detected corruptions but
                // data lossing repair is not allowed (ie NTDS.DIT)
                // then we will try defrag as all we can do
                //
                if ( JET_errDatabaseCorruptedNoRepair == err )
                {
                    // in order to run defrag we need to clean-up the current instance
                    // and set back JET_paramTempPath to the default
                    //
                    err                 = JET_errSuccess;
                    ErrEDBUTLCleanup( instance, sesid, err );
                    instance            = 0;
                    sesid               = JET_sesidNil;
                    Call( JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, wszTempDatabaseDefault ) );

                    // fall through to defrag
                    fTryDefrag = fTrue;
                }
                else
                {
                    CallJ( err, Cleanup );

                    errRepaired = err;

                    Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );

                    wprintf( L"Note:%c", wchNewLine );
                    wprintf( L"  It is recommended that you immediately perform a full backup%c", wchNewLine );
                    wprintf( L"  of this database. If you restore a backup made before the%c", wchNewLine );
                    wprintf( L"  repair, the database will be rolled back to the state%c", wchNewLine );
                    wprintf( L"  it was in at the time of that backup.%c%c", wchNewLine, wchNewLine );
                }

                if ( !fTryDefrag )
                {
                    break;
                }

            }

        case modeDefragment:
            {
                JET_GRBIT   grbitDefrag = 0;
                if( FUTILOPTSDefragInfo( opts.fUTILOPTSFlags ) )
                    grbitDefrag |= JET_bitCompactStats;
                if( FUTILOPTSDefragRepair( opts.fUTILOPTSFlags ) )
                    grbitDefrag |= JET_bitCompactRepair;
                if ( FUTILOPTSPreserveTempDB( opts.fUTILOPTSFlags ) )
                    grbitDefrag |= JET_bitCompactPreserveOriginal;

                CallJ( ErrEDBUTLCheckDBName( &opts, wszDefaultDefragDB, NULL ), Cleanup );

                wprintf( L"Initiating DEFRAGMENTATION mode...%c", wchNewLine );
                wprintf( L"            Database: %s%c", opts.wszSourceDB, wchNewLine );

                fWhitespaceOnErr = fTrue;

                // Restart with logging/recovery disabled.
                Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"off" ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDbExtensionSize, 256, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableTempTableVersioning, fFalse, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexCleanup, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramPreferredMaxOpenTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, opts.efvUserSpecified == 0 ? JET_efvUsePersistedFormat : opts.efvUserSpecified, NULL ) );

                // Set user overrides.
                Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );
                Call( JetInit( &instance ) );
                CallJ( JetBeginSessionW( instance, &sesid, wszUser, wszPassword ), Cleanup );

                // Detach temporary database and delete file if present (ignore errors).
                EDBUTLDeleteTemp( &opts );

                dbutil.sesid        = sesid;
                dbutil.op           = opDBUTILDBDefragment;
                dbutil.szDatabase   = opts.wszSourceDB;
                dbutil.szTable      = opts.wszTempDB;
                dbutil.szIndex      = NULL;
                dbutil.grbitOptions = grbitDefrag;
                dbutil.pfnCallback  = PrintStatus;

                CallJ( JetDBUtilitiesW( &dbutil ), Cleanup );

                Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );

                Call( ErrEDBUTLBackupAndInstateDB( sesid, &opts ) );

                wprintf( L"%cNote:%c", wchNewLine, wchNewLine );
                wprintf( L"  It is recommended that you immediately perform a full backup%c", wchNewLine );
                wprintf( L"  of this database. If you restore a backup made before the%c", wchNewLine );
                wprintf( L"  defragmentation, the database will be rolled back to the state%c", wchNewLine );
                wprintf( L"  it was in at the time of that backup.%c%c", wchNewLine, wchNewLine );
            }
            break;


        case modeChecksum:
            {
                BOOL        fBadDbChecksumDetected      = fFalse;
                ULONG       cPagesToChecksum            = 0;
                LONG        filetype                    = JET_filetypeUnknown;
                const BOOL  fCheckLogSet                = FEDBUTLBaseNameOnly( opts.wszSourceDB );

                UTILOPTSSetMaybeTolerateCorruption( opts.fUTILOPTSFlags );
                err = ErrEDBUTLCheckDBName(
                                &opts,
                                wszDefaultChecksumDB,
                                &filetype );

                // if there is no such a file and the file name was just a 3 letter name
                // probably it is the set of logs with that extension
                //
                if ( JET_filetypeUnknown == filetype && fCheckLogSet )
                {
                    filetype = JET_filetypeLog;
                    err = JET_errSuccess;
                }
                Call( err );

                wprintf( L"Initiating CHECKSUM mode...%c", wchNewLine );
                fWhitespaceOnErr = fTrue;

                switch ( filetype )
                {
                    case JET_filetypeCheckpoint:
                        // we have a checkpoint file. We will checksum it as a database.
                        wprintf( L" Checkpoint File: %s%c", opts.wszSourceDB, wchNewLine );
                        UTILOPTSSetChecksumEDB( opts.fUTILOPTSFlags );
                        break;

                    case JET_filetypeLog:
                    {
                        WCHAR   wszBaseName[4];

                        if ( !fCheckLogSet )
                        {
                            wprintf( L"      Log File: %s%c", opts.wszSourceDB, wchNewLine );
                        }

                        // we will reset the flags below in order to avoid additional
                        // checksuming once we go past the log file checking below
                        UTILOPTSResetChecksumEDB( opts.fUTILOPTSFlags );

                        // we prepare the variables in order to
                        // call the same code the modeDump for log files
                        opts.pv = &dbutil;
                        opts.mode = modeDump;
                        dbutil.op = opDBUTILDumpLogfile;
                        opts.wszBase = wszBaseName;
                        dbutil.grbitOptions = JET_bitDBUtilOptionVerify;
                        EDBUTLGetBaseName( opts.wszSourceDB, opts.wszBase );

                        Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                        Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );

                        //  UNDONE: all the other /k cases except log will have
                        //  a status bar printing. Can we get one
                        //  for the logs as well?
                        Call( ErrEDBUTLDump( &dbutil, &opts ) );
                    }
                        break;

                    case JET_filetypeTempDatabase:
                        wprintf( L"        Database: %s%c%c", opts.wszSourceDB, wchNewLine, wchNewLine );
                        wprintf( L"This is a temporary database. Checksums are not maintained for such databases.%c", wchNewLine );
                        UTILOPTSResetChecksumEDB( opts.fUTILOPTSFlags );
                        break;

                    case JET_filetypeFTL:
                        wprintf( L"        Trace File: %s%c%c", opts.wszSourceDB, wchNewLine, wchNewLine );
                        wprintf( L"This is a FTL trace log.  Checksums are not maintained for such a file.%c", wchNewLine );
                        UTILOPTSResetChecksumEDB( opts.fUTILOPTSFlags );
                        break;

                    case JET_filetypeFlushMap:
                    {
                        wprintf( L"      Flush Map File: %s%c", opts.wszSourceDB, wchNewLine );

                        // we will reset the flags below in order to avoid additional
                        // checksuming once we go past the log file checking below
                        UTILOPTSResetChecksumEDB( opts.fUTILOPTSFlags );

                        // we prepare the variables in order to
                        // call the same code the modeDump for flush map files
                        opts.pv = &dbutil;
                        opts.mode = modeDump;
                        dbutil.op = opDBUTILDumpFlushMapFile;
                        dbutil.grbitOptions = JET_bitDBUtilOptionVerify;

                        Call( ErrEDBUTLDump( &dbutil, &opts ) );
                    }
                        break;

                    default:
                        //  New type at ESE level? Must be added here!
                        AssertSz( false, "Invalid file type %d found.", filetype );

                        //  FALL THROUGH

                    case JET_filetypeUnknown:
                    case JET_filetypeDatabase:
                        wprintf( L"        Database: %s%c", opts.wszSourceDB, wchNewLine );
                        wprintf( L"  Temp. Database: %s%c", opts.wszTempDB, wchNewLine );
                        break;
                }

                if ( FUTILOPTSChecksumEDB ( opts.fUTILOPTSFlags ) )
                {
                    wprintf( L"%c%c", wchNewLine, wchNewLine );

                    ULONG_PTR cbPage = 0;
                    Call( JetGetSystemParameterW( JET_instanceNil, JET_sesidNil, JET_paramDatabasePageSize, &cbPage, NULL, sizeof( cbPage ) ) );

                    BOOL fSuccessfulChecksum = FALSE;
                    fSuccessfulChecksum = FChecksumFile(
                                opts.wszSourceDB,
                                ULONG( cbPage ),
                                cPagesToChecksum,
                                &fBadDbChecksumDetected );

                    if ( fSuccessfulChecksum )
                    {
                        err = JET_errSuccess;
                    }
                    else
                    {
                        fUnknownError = fTrue;
                        goto HandleError;
                    }
                }

                //  now go back and check if we detected any
                //  checksum mismatches in the database
                //
                if ( fBadDbChecksumDetected )
                {
                    Call( ErrERRCheck( JET_errDatabaseCorrupted ) );
                }

                wprintf( L"%c%c", wchNewLine, wchNewLine );
            }
            break;

        case modeCopyFile:
            {
                WCHAR   wszFileT[_MAX_PATH+1];

                fWhitespaceOnErr = fTrue;

                if ( NULL == opts.wszSourceDB )
                {
                    wprintf( wszUsageErr1, L"source file" );
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }
                if ( NULL == _wfullpath( wszFileT, opts.wszSourceDB, _MAX_PATH ) )
                {
                    wprintf( L"Error: Source file specification.'%s' is invalid.", opts.wszSourceDB );
                    Call( ErrERRCheck( JET_errInvalidPath ) );
                }

                if ( NULL == opts.wszTempDB )
                {
                    EDBUTLGetUnpathedFilename( opts.wszSourceDB, wszFileT, sizeof(wszFileT) );
                    opts.wszTempDB = wszFileT;
                }
                else if ( NULL == _wfullpath( wszFileT, opts.wszTempDB, _MAX_PATH ) )
                {
                    wprintf( L"Error: Destination file specification.'%s' is invalid.", opts.wszTempDB );
                    Call( ErrERRCheck( JET_errInvalidPath ) );
                }

                wprintf( L"Initiating COPY FILE mode...%c", wchNewLine );

                err = ErrCopyFile( opts.wszSourceDB, opts.wszTempDB, opts.fUTILOPTSCopyFileIgnoreIoErrors );
                if ( err > 0 )
                {
                    goto HandleError;
                }
                Call( err );

                wprintf( L"%c%c", wchNewLine, wchNewLine );
            }
            break;

        case modeScrub:
            {
                Call( ErrEDBUTLCheckDBName( &opts, wszDefaultScrubDB, NULL ) );

                wprintf( L"Initiating SECURE mode...%c", wchNewLine );
                wprintf( L"        Database: %s%c", opts.wszSourceDB, wchNewLine );

                fWhitespaceOnErr = fTrue;

                Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"scrub_off" ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexCleanup, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );

                if ( FUTILOPTSTrimDatabase( &opts ) )
                {
                    Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableShrinkDatabase, JET_bitShrinkDatabaseOn, NULL ) );
                }
                else
                {
                    Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableShrinkDatabase, JET_bitShrinkDatabaseOff, NULL ) );
                }

                //  create enough sessions for multi-threadeding
                //  need to have plenty of pages to have that many sessions
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 16, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10240, NULL ) );

                // Set user overrides.
                Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

                Call( JetInit( &instance ) );
                CallJ( JetBeginSessionW( instance, &sesid, wszUser, wszPassword ), Cleanup );

                if ( FUTILOPTSTrimDatabase( &opts ) )
                {
                    dbutil.sesid        = sesid;
                    dbutil.op           = opDBUTILDBTrim;
                    dbutil.szDatabase   = opts.wszSourceDB;
                    dbutil.szTable      = NULL;
                    dbutil.szIndex      = NULL;
                    dbutil.grbitOptions = JET_bitNil;
                    dbutil.pfnCallback  = PrintStatus;

                    CallJ( JetDBUtilitiesW( &dbutil ), Cleanup );
                }
                else
                {
                    CallJ( ErrEDBUTLScrub(
                        sesid,
                        opts.wszSourceDB,
                        PrintStatus,
                        0 ), Cleanup );
                }

                Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );

                wprintf( L"Warning:%c", wchNewLine );
                wprintf( L"  You MUST delete the logfiles for this database%c%c", wchNewLine, wchNewLine );

                wprintf( L"Note:%c", wchNewLine );
                wprintf( L"  It is recommended that you immediately perform a full backup%c", wchNewLine );
                wprintf( L"  of this database. If you restore a backup made before the%c", wchNewLine );
                wprintf( L"  repair, the database will be rolled back to the state%c", wchNewLine );
                wprintf( L"  it was in at the time of that backup.%c%c", wchNewLine, wchNewLine );
            }
            break;

        case modeIntegrity:
            {
                Call( ErrEDBUTLCheckDBName( &opts, wszDefaultIntegDB, NULL ) );

                wprintf( L"Initiating INTEGRITY mode...%c", wchNewLine );
                wprintf( L"        Database: %s%c", opts.wszSourceDB, wchNewLine );
                wprintf( L"  Temp. Database: %s%c", opts.wszTempDB, wchNewLine );
#ifdef ESESHADOW
                if ( FUTILOPTSOperateTempVssRec( opts.fUTILOPTSFlags ) )
                {
                    wprintf( L"       Log files: %s%c", opts.wszLogfilePath ? opts.wszLogfilePath : L"<current directory>", wchNewLine );
                    wprintf( L"    System files: %s%c", opts.wszSystemPath ? opts.wszSystemPath : L"<current directory>", wchNewLine );
                }
#endif

                if( NULL != opts.pv )
                {
                    wprintf( L"           Table: %s%c", (WCHAR *)opts.pv, wchNewLine );
                }

                fWhitespaceOnErr = fTrue;

                Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"off" ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, opts.wszTempDB ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableOnlineDefrag, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableTempTableVersioning, fFalse, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexCleanup, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxOpenTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramPreferredMaxOpenTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );
                //  create plenty of sessions for multi-threaded integrity/repair
                //  need to have plenty of pages to have that many sessions
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 128, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10240, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, JET_efvUsePersistedFormat, NULL ) );

                // Set user overrides.
                Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

                Call( JetInit( &instance ) );
                CallJ( JetBeginSessionW( instance, &sesid, wszUser, wszPassword ), Cleanup );

                JET_GRBIT grbit;
                grbit = JET_bitDBUtilOptionDontRepair;
                if( FUTILOPTSDumpStats( opts.fUTILOPTSFlags ) )
                {
                    grbit |= JET_bitDBUtilOptionStats;
                }
                if( FUTILOPTSDontBuildIndexes( opts.fUTILOPTSFlags ) )
                {
                    grbit |= JET_bitDBUtilOptionDontBuildIndexes;
                }
                if( FUTILOPTSSuppressLogo( opts.fUTILOPTSFlags ) )
                {
                    grbit |= JET_bitDBUtilOptionSuppressLogo;
                }

                assert( grbit & JET_bitDBUtilOptionDontRepair );

                CallJ( ErrEDBUTLRepair(
                    sesid,
                    opts.wszSourceDB,
                    opts.wszBackup,
                    (WCHAR *)(opts.pv),
                    opts.wszIntegPrefix,
                    PrintStatus,
                    grbit ), Cleanup );

                Call( ErrEDBUTLCleanup( instance, sesid, JET_errSuccess ) );
            }
            break;

#ifndef ESENT
        case modeHardRecovery:
        {
            WCHAR *     wszTargetInstance = opts.wszRestore;
            WCHAR       wszFullRestorePath[_MAX_PATH + 1];

            // if missing, get current directory
            if ( _wfullpath( wszFullRestorePath, opts.wszSourceDB?opts.wszSourceDB:L".", _MAX_PATH ) == NULL )
            {
                Call ( JET_errInvalidPath );
            }

            hrESEBACK = hrNone;

            if ( FUTILOPTSServerSim ( opts.fUTILOPTSFlags ) )
            {
#ifdef RESTORE_SERVER_SIMULATION
                // if no definition file was provided, call with NULL and
                // it will print a sample of such a file

                err = ErrDBUTLServerSim( opts.wszSourceDB?wszFullRestorePath:NULL );

#else // RESTORE_SERVER_SIMULATION
                // FUTILOPTSServerSim() set only with RESTORE_SERVER_SIMULATION
                // defined, check command line parsing above
                assert ( fFalse );
#endif // RESTORE_SERVER_SIMULATION
            }
            else if ( FUTILOPTSDumpRestoreEnv( opts.fUTILOPTSFlags ) )
            {
                err = ErrDBUTLDumpRestoreEnv(wszFullRestorePath);
            }
            else
            {
                err = ErrDBUTLRestoreComplete( wszFullRestorePath, wszTargetInstance, opts.wszLogfilePath, FUTILOPTSPreserveTempDB( opts.fUTILOPTSFlags ) );
            }
        }

            if ( hrNone != hrESEBACK )
            {
                goto ESEBACKError;
            }

            Call( err );
            break;
#endif // ESENT

        case modeDump:
        default:
        {
            WCHAR   wszBaseName[4];

            // Make the most innocuous operation the fall-through (to cover
            // ourselves in the unlikely event we messed up the modes).
            assert( opts.mode == modeDump );
            assert( opts.pv == &dbutil );

            if ( opts.mode == modeDump )
            {
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexCleanup, 0, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramAccessDeniedRetryPeriod, 0, NULL ) );

                if ( opts.wszSourceDB == NULL )
                {
                    wprintf( wszUsageErr1, L"database/filename" );          // Missing spec.
                    wprintf( L"%c%c", wchNewLine, wchNewLine );
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }

                wprintf( L"Initiating FILE DUMP mode...%c", wchNewLine );

                switch( dbutil.op ) //lint !e644
                {
                    case opDBUTILDumpLogfile:
                        opts.wszBase = wszBaseName;
                        EDBUTLGetBaseName( opts.wszSourceDB, opts.wszBase );
                        break;

                    case opDBUTILDumpFlushMapFile:
                        wprintf( L"Flush map file: %s%c", opts.wszSourceDB, wchNewLine );
                        break;

                    case opDBUTILDumpLogfileTrackNode:
                        wprintf( L"      Log file: %s%c", opts.wszSourceDB, wchNewLine );
                        wprintf( L"          Node: %d:%d:%d%c", dbutil.dbid, dbutil.pgno, dbutil.iline, wchNewLine );
                        wprintf( L"         Lgpos: %06X,%04X,%04X%c", dbutil.lGeneration, dbutil.isec, dbutil.ib, wchNewLine );
                        break;

                    case opDBUTILDumpCheckpoint:
                        wprintf( L"      Checkpoint file: %s%c", opts.wszSourceDB, wchNewLine );
                        break;

                    case opDBUTILSetHeaderState:
                        Call( ErrEDBUTLCheckDBName( &opts, wszDefaultTempDB, NULL ) );
                        wprintf( L"      Database %s will be forced to a clean-shutdown state.%c",
                            opts.wszSourceDB, wchNewLine );
                        break;

                    case opDBUTILDumpPage:
                        UTILOPTSSetMaybeTolerateCorruption( opts.fUTILOPTSFlags );
                        Call( ErrEDBUTLCheckDBName( &opts, wszDefaultTempDB, NULL ) );
                        wprintf( L"      Database: %s%c", opts.wszSourceDB, wchNewLine );
                        if ( NULL == dbutil.szIndex )
                        {
                            wprintf( L"          Page: %d%c", dbutil.pgno, wchNewLine );
                        }
                        else
                        {
                            //  non-NULL szTable means we'll be seeking down
                            //  the btree searching for a specified key
                        }
                        break;

                    case opDBUTILDumpNode:
                        Call( ErrEDBUTLCheckDBName( &opts, wszDefaultTempDB, NULL ) );
                        wprintf( L"      Database: %s%c", opts.wszSourceDB, wchNewLine );
                        break;

                    case opDBUTILDumpHeader:
                        UTILOPTSSetMaybeTolerateCorruption( opts.fUTILOPTSFlags );
                    case opDBUTILDumpMetaData:
                    case opDBUTILDumpSpace:
                    case opDBUTILDumpSpaceCategory:
                    case opDBUTILDumpData:
                    case opDBUTILDumpRBSHeader:
                    case opDBUTILDumpRBSPages:
                        err = ErrEDBUTLCheckDBName( &opts, wszDefaultTempDB, NULL );

                        //  we may still want to dump file system information if the file is locked
                        if ( dbutil.op == opDBUTILDumpSpace &&
                            ( err == JET_errSuccess || err == JET_errFileAccessDenied || err == JET_errDatabaseCorrupted ) )
                        {
                            //  dump NTFS information
                            (void)ErrFileSystemDump( opts.wszSourceDB, FUTILOPTSVerbose( opts.fUTILOPTSFlags ), FUTILOPTSDebugMode( opts.fUTILOPTSFlags ) );
                        }
                        Call( err );
                        if ( dbutil.op == opDBUTILDumpRBSHeader || dbutil.op == opDBUTILDumpRBSPages )
                        {
                            wprintf( L"         Snapshot: %s%c", opts.wszSourceDB, wchNewLine );
                        }
                        else
                        {
                            wprintf( L"         Database: %s%c", opts.wszSourceDB, wchNewLine );
                        }
                        if ( dbutil.op == opDBUTILDumpSpaceCategory )
                        {
                            wprintf( L"       Page range: %I32u:", dbutil.spcatOptions.pgnoFirst );
                            ( dbutil.spcatOptions.pgnoLast != ulMax ) ? wprintf( L"%I32u", dbutil.spcatOptions.pgnoLast ) : wprintf( L"max" );
                            wprintf( L"%c", wchNewLine );
                        }
                        else if ( dbutil.op == opDBUTILDumpRBSPages )
                        {
                            wprintf( L"       Page range: %I32u:", dbutil.rbsOptions.pgnoFirst );
                            ( dbutil.rbsOptions.pgnoLast != ulMax ) ? wprintf( L"%I32u", dbutil.rbsOptions.pgnoLast ) : wprintf( L"max" );
                            wprintf( L"%c", wchNewLine );
                        }
                        break;

                    case opDBUTILDumpFTLHeader:
                        wprintf( L"             FTL file: %s%c", opts.wszSourceDB, wchNewLine );
                        break;

                    case opDBUTILDumpCacheFile:
                        wprintf( L"    Cache File: %s%c", opts.wszSourceDB, wchNewLine );
                        break;

                    default:
                        assert( 0 );
                }
            }

            assert( instance == 0 );

            //
            // Actually dump the relevant info ...
            //
            err = ErrEDBUTLDump( &dbutil, &opts );

            // Print progress failure for space categorization.
            if ( ( err < JET_errSuccess ) && ( err != JET_errInvalidParameter ) &&
                    ( dbutil.op == opDBUTILDumpSpaceCategory ) && !( dbutil.grbitOptions & JET_bitDBUtilOptionDumpVerbose ) )
            {
                (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntFail, NULL );
            }

            wprintf( L"%c", wchNewLine );

            //  dump the cached file header if present
            dbutil.op = opDBUTILDumpCachedFileHeader;
            (void)ErrEDBUTLDump( &dbutil, &opts );

            Call( err );
            break;
        }
    } // switch( opts.mode )

#ifdef ESESHADOW
    if ( FUTILOPTSOperateTempVss( opts.fUTILOPTSFlags ) )
    {
        if ( fMountedShadow )
        {
            if ( FUTILOPTSOperateTempVssPause( opts.fUTILOPTSFlags ) )
            {
                wprintf( L"Mounted shadow volumes located at:%c", wchNewLine );
                wprintf( L"        File: %s%c", wszShadowedFile, wchNewLine );
                if ( FUTILOPTSOperateTempVssRec( opts.fUTILOPTSFlags ) )
                {
                    wprintf( L"    Log path: %s%c", wszShadowedLog, wchNewLine );
                    wprintf( L" System path: %s%c", wszShadowedSystem, wchNewLine );
                }
                wprintf( L"%c<<<<<  Press a key to continue  >>>>>%c%c", wchNewLine, wchNewLine, wchNewLine );
                _getch();
            }

            (void) DelayLoadEseShadowPurgeShadow( opts.eseShadowContext );
            fMountedShadow = fFalse;
        }
        
        if ( opts.eseShadowContext )
        {
            (void) DelayLoadEseShadowTerm( opts.eseShadowContext );
            opts.eseShadowContext = NULL;
        }
    }
#endif

    EDBUTLGetTime( timer, &iSec, &iMSec );

    if( JET_errSuccess != errRepaired ) //db is repaired
    {
        ULONG_PTR   ulWarn      = errRepaired;
        WCHAR       wszWarn[512];
        wszWarn[0] = 0;
        (void)JetGetSystemParameterW(
            instance,
            JET_sesidNil,
            JET_paramErrorToString,
            (ULONG_PTR *)&ulWarn,
            wszWarn,
            sizeof( wszWarn ) );
        if ( fWhitespaceOnErr )
            wprintf( L"%c%c", wchNewLine, wchNewLine );
        wprintf( wszOperWarn, errRepaired, wszWarn, iSec, iMSec );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        PushEseutilEndMarker( iSec, iMSec, err, wszWarn );
    }
    else // JET_errSuccess == err
    {
        wprintf( wszOperSuccess, iSec, iMSec );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        PushEseutilEndMarker( iSec, iMSec, JET_errSuccess, L"JET_errSuccess, Operation was successful." );
    }

    OSTerm();
    // OSPostterm() implicit with oslayer destructor.
    return err;


Cleanup:
    ErrEDBUTLCleanup( instance, sesid, err );

HandleError:
    EDBUTLGetTime( timer, &iSec, &iMSec );

    if ( fWhitespaceOnErr )
        wprintf( L"%c%c", wchNewLine, wchNewLine );

#ifdef ESESHADOW
    if ( FUTILOPTSOperateTempVss( opts.fUTILOPTSFlags ) )
    {
        if ( fMountedShadow )
        {
            (void) DelayLoadEseShadowPurgeShadow( opts.eseShadowContext );
        }
        
        if ( opts.eseShadowContext )
        {
            (void) DelayLoadEseShadowTerm( opts.eseShadowContext );
            opts.eseShadowContext = NULL;
        }
    }
#endif

    if ( fUnknownError )
    {
        wprintf( wszOperFailUnknownError, iSec, iMSec );
        err = -1;
    }
    else
    {
        ULONG_PTR   ulErr       = err;
        WCHAR       wszError[512];

        assert( err != 0 );

        wszError[0] = 0;
        (void)JetGetSystemParameterW(
            instance,
            JET_sesidNil,
            JET_paramErrorToString,
            (ULONG_PTR *)&ulErr,
            wszError,
            sizeof( wszError ) );

        if( opts.mode == modeRecovery )
        {
            if( err == JET_errCommittedLogFilesMissing ||
                err == JET_errCommittedLogFileCorrupt )
            {
                wprintf(L"%wc"
                        L"  \tRecovery has indicated that there is a lossy recovery option.  Run recovery with the /a argument.%wc", wchNewLine, wchNewLine );
            }
            else if ( err == JET_errMissingLogFile )
            {
                wprintf(L"%wc"
                        L"  \tRecovery has indicated that there might be a lossy recovery option.  Run recovery with the /a argument.%wc", wchNewLine, wchNewLine );
            }
        }
        PrintESEUTILError( err, wszError, fWhitespaceOnErr, timer );
    }

    OSTerm();
    // OSPostterm() implicit with oslayer destructor.

    wprintf( L"%c%c", wchNewLine, wchNewLine );
    return err;

#ifndef ESENT
ESEBACKError:
    assert ( hrNone != hrESEBACK );
    assert ( wszESEBACKError );
    PrintESEUTILError( hrESEBACK, wszESEBACKError, fWhitespaceOnErr, timer );
    LocalFree( wszESEBACKError );
    OSTerm();
    // OSPostterm() implicit with oslayer destructor.
    return hrESEBACK;
#endif // ESENT


Usage:
    SetCurArgID( 0 );
    assert( GetCurArgID() == 0 );

    _wcsupr_s( GetCurArg(), LOSStrLengthW( GetCurArg() ) + 1 );
    EDBUTLHelp( GetCurArg() );
    
    OSTerm();
    // OSPostterm() implicit with oslayer destructor.
    return -1;
}

