// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if defined(DEBUG) && !defined(ESENT)
#define RESTORE_SERVER_SIMULATION
#define RESTORE_SERVER_SIMULATION_HELP
#endif

#ifdef RESTORE_SERVER_SIMULATION
#include <eseback2.h>
#endif

#include "_edbutil.hxx"
#include "_dbspacedump.hxx"
#include "stat.hxx"

#include "esefile.hxx"
#undef UNICODE

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

#endif

#ifdef ESESHADOW

#include "eseshadow.h"

ESESHADOW_LOCAL_DEFERRED_DLL_STATE
#endif

#define ESEUTIL_FAKE_RECOVERY_WITHOUT_UNDO 1


LOCAL const WCHAR   * const wszUser         = L"user";
LOCAL const WCHAR   * const wszPassword     = L"";



LOCAL const WCHAR   * const wszCurrDir                  = L".";

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
#else
LOCAL const WCHAR   * const wszHelpDesc1        = L"DESCRIPTION:  Database utilities for the Extensible Storage Engine for Microsoft(R) Exchange Server.";
#endif
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
LOCAL const WCHAR   * const wszHelpModes12  = L"               Secure:  %s /z <database name> [options]";

LOCAL const WCHAR   * const wszHelpPrompt1  = L"<<<<<  Press a key for more help  >>>>>";
LOCAL const WCHAR   * const wszHelpPrompt2  = L"D=Defragmentation, R=Recovery, G=inteGrity, K=checKsum,";

#ifdef ESENT
LOCAL const WCHAR   * const wszHelpPrompt3  = L"P=rePair, M=file duMp, Y=copY file";
#else
LOCAL const WCHAR   * const wszHelpPrompt3  = L"P=rePair, M=file duMp, Y=copY file, C=restore";
#endif

#ifdef DEBUG
LOCAL const WCHAR   * const wszHelpPrompt4  = L"B=Backup, F=record Format upgrade, U=upgrade, Z=secure";
#endif
LOCAL const WCHAR   * const wszHelpPromptCursor = L"=> ";

LOCAL const WCHAR * const   wszOperSuccess          = L"Operation completed successfully in %d.%d seconds.";
LOCAL const WCHAR * const   wszOperWarn             = L"Operation completed successfully with %d (%s) after %d.%d seconds.";
LOCAL const WCHAR * const   wszOperFailWithError        = L"Operation terminated with error %d (%s) after %d.%d seconds.";
LOCAL const WCHAR * const   wszOperFailUnknownError = L"Operation terminated unsuccessfully after %d.%d seconds.";


const JET_LGPOS lgposMax = { 0xffff, 0xffff, 0x7fffffff };

JET_ERR ErrGetProcAddress(
            const HMODULE hmod,
            const CHAR * const szFunc,
            VOID ** const pvPfn )
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
#else
    StringCbPrintfW( wszVersion, sizeof(wszVersion), L"%hs.%hs", PRODUCT_MAJOR, PRODUCT_MINOR );
    wprintf( L"Extensible Storage Engine Utilities for Microsoft(R) Exchange Server%c", wchNewLine );
#endif
    wprintf( L"Version %s%c", wszVersion, wchNewLine );
    wprintf( L"Copyright (c) Microsoft Corporation.\nLicensed under the MIT License.%c", wchNewLine );
    wprintf( L"%c", wchNewLine );
}

LOCAL VOID EDBUTLHelpDefrag( __in PCWSTR wszAppName )
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
#ifdef DEBUG
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

LOCAL VOID EDBUTLHelpRecovery( __in PCWSTR wszAppName )
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
    wprintf( L"                  /u[log]           - stop recovery when the Undo phase is reached with the option%c", wchNewLine );
    wprintf( L"                                      to stop when a certain log generation is recovered.%c", wchNewLine );
    wprintf( L"                                      [log] is the log generation number and if not specified %c", wchNewLine );
    wprintf( L"                                      the replay will go to the end of the existing logs.%c", wchNewLine );
#ifdef DEBUG
    wprintf( L"                                      [log] can be a log position in the N:N:N form in which case %c", wchNewLine );
    wprintf( L"                                      recovery will stop after the specified position %c", wchNewLine );
#endif
#endif
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

LOCAL VOID EDBUTLHelpIntegrity( __in PCWSTR wszAppName )
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
#ifdef DEBUG
    wprintf( L"                  /b        - don't rebuild and compare indexes%c", wchNewLine );
    wprintf( L"                  /n        - dump table statistics to INTGINFO.TXT%c", wchNewLine );
#endif
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

LOCAL VOID EDBUTLHelpChecksum( __in PCWSTR wszAppName )
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


LOCAL VOID EDBUTLHelpCopyFile( __in PCWSTR wszAppName )
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

LOCAL VOID EDBUTLHelpRepair( __in PCWSTR wszAppName )
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
#ifdef DEBUG
    wprintf( L"                  /n           - dump table statistics to INTGINFO.TXT%c", wchNewLine );
#endif
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

LOCAL VOID EDBUTLHelpDump( __in PCWSTR wszAppName )
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

    PrintSpaceDumpHelp( 80, wchNewLine, L"                  ", L"           " );

    wprintf( L"%c", wchNewLine );
    wprintf( L"%*cSPACE CATEGORIZATION OPTIONS:%c", 18, L' ', wchNewLine );
    wprintf( L"%*c     /f            - Runs full space categorization. This may be expensive, but%c", 29, L' ', wchNewLine );
    wprintf( L"%*c                     required to categorize some pages.%c", 29, L' ', wchNewLine );
}

#ifndef ESENT
LOCAL VOID EDBUTLHelpHardRecovery( __in PCWSTR wszAppName )
{
    wprintf( L"%c", wchNewLine );
    wprintf( L"RESTORE:%c", wchNewLine );
    wprintf( L"    DESCRIPTION:  Restore information and completion.%c", wchNewLine );
#ifdef RESTORE_SERVER_SIMULATION_HELP
    wprintf( L"         SYNTAX:  %s /c[mode-modifier] <path name|file name> [options]%c", wszAppName, wchNewLine );
#else
    wprintf( L"         SYNTAX:  %s /c[mode-modifier] <path name> [options]%c", wszAppName, wchNewLine );
#endif
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
#else
    wprintf( L"                  <path name>     - directory of the restore%c", wchNewLine );
    wprintf( L"                                    (Restore.Env location)%c", wchNewLine );
#endif
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
#endif

#ifdef DEBUG

LOCAL VOID EDBUTLHelpBackup( __in PCWSTR wszAppName )
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

#endif

LOCAL VOID EDBUTLHelpScrub( __in PCWSTR const wszAppName )
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


LOCAL VOID EDBUTLHelp( __in PCWSTR wszAppName )
{
    wint_t c;

    EDBUTLPrintLogo();
    wprintf( L"%s%c%c", wszHelpDesc1, wchNewLine, wchNewLine );
    wprintf( L"%s%c", wszHelpSyntax, wchNewLine );
    wprintf( wszHelpModes1, wszAppName );
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes2, wszAppName );
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes3, wszAppName );
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes4, wszAppName );
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes5, wszAppName );
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes6, wszAppName );
    wprintf( L"%c", wchNewLine );
    wprintf( wszHelpModes7, wszAppName );
    wprintf( L"%c", wchNewLine );
#ifndef ESENT
    wprintf( wszHelpModes8, wszAppName );
    wprintf( L"%c", wchNewLine );
#endif

#ifdef DEBUG
    wprintf( wszHelpModes9, wszAppName );
    wprintf( L"%c", wchNewLine );

    wprintf( wszHelpModes12, wszAppName );
    wprintf( L"%c", wchNewLine );
#endif

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
#endif

#ifdef DEBUG
        case L'b':
        case L'B':
            EDBUTLHelpBackup( wszAppName );
            break;
#endif

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
        case JET_SNP( -1 ):
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

                    cchOper = wcslen( wszOperation );
                    assert( cchOper + (ULONG)wcslen( wszStatusMsg ) <= 51 );
                    cchPadding = ( 51 - ( cchOper + (ULONG)wcslen( wszStatusMsg ) ) ) / 2;

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

                case 8:
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

    const BOOL fVerbose = FUTILOPTSVerbose( popts->fUTILOPTSFlags );

    const WCHAR wszStatusIndent [] = L"        ";

    Assert(  8 != snt );

    switch ( snp )
    {
        case JET_snpRestore:

            if ( !fVerbose )
            {
                PrintStatus( JET_sesidNil, snp, snt, pvData );
            }
            else
            {
                switch ( snt )
                {
                    case JET_sntBegin:
                        wprintf( L"    Beginning Verbose Recovery ---------------- \n" );
                        break;
                    case JET_sntProgress:
                    {
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

                    if ( precctrl->MissingLog.fCurrentLog &&
                            precctrl->MissingLog.eNextAction == JET_MissingLogCreateNewLogStream )
                    {
                        if ( popts->grbitInit & JET_bitLogStreamMustExist )
                        {
                            Assert( precctrl->errDefault != JET_errSuccess );
                        }
                        else
                        {
                            Assert( precctrl->errDefault == JET_errSuccess );
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
                    ExpectedSz( fFalse, "Unknown JET_SNT snt = %d\n", snt );
            }
            break;
        }

        case JET_SNP( -1 ):
            AssertSz( fFalse, "Do we really get -1 from here!?  What would that mean" );
        default:
            ExpectedSz( fFalse, "Unknown JET_SNP snp = %d", snp );
    }

    return errRet;
}

const BYTE EDBUTL_errInvalidPath = 1;
const BYTE EDBUTL_errSharedName = 2;
const BYTE EDBUTL_errInvalidDB  = 3;

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

LOCAL JET_ERR ErrEDBUTLCheckDBName(
    UTILOPTS* const             popts,
    PCWSTR const                wszTempDB,
    LONG * const                pfiletype )
{
    assert( NULL != popts );
    assert( NULL != wszTempDB );

    WCHAR   wszFullpathSrcDB[ _MAX_PATH + 1] = L"";
    WCHAR   wszFullpathTempDB[ _MAX_PATH + 1 ] = L"";

    if ( NULL == popts->wszTempDB )
    {
        popts->wszTempDB = (WCHAR *)wszTempDB;
    }

    if ( NULL == _wfullpath( wszFullpathTempDB, popts->wszTempDB, _MAX_PATH ) )
    {
        PrintErrorMessage( EDBUTL_errInvalidPath, popts, EDBUTL_paramTempDB );
        return ErrERRCheck( JET_errInvalidPath );
    }

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

    ULONG   cbPageSize  = 0;
    if (    !FUTILOPTSExplicitPageSet( popts->fUTILOPTSFlags ) &&
            JET_errSuccess == JetGetDatabaseFileInfoW( popts->wszSourceDB, &cbPageSize, sizeof( cbPageSize ), JET_DbInfoPageSize ) )
    {
        const JET_ERR err = JetSetSystemParameterW( NULL, 0, JET_paramDatabasePageSize, cbPageSize, NULL );
        if ( err < JET_errSuccess &&
            ( popts->mode != modeDump || cbPageSize != 2048 || err != JET_errInvalidParameter) )
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

    LONG    filetype;
    JET_ERR err = JetGetDatabaseFileInfoW( popts->wszSourceDB, &filetype, sizeof( filetype ), JET_DbInfoFileType );
    if ( JET_errSuccess != err )
    {
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

LOCAL BOOL FEDBUTLParsePath(    __in PCWSTR             arg,
                                __deref_inout_z WCHAR **    pwszParam,
                                __in_z PCWSTR               wszParamDesc,
                                __in BOOL               fAllowEmpty     = fFalse )
{
    BOOL    fResult = fTrue;

    if ( L'\0' == *arg )
    {
        const WCHAR *argT = arg;
        arg = GetNextArg();
        if ( NULL == arg || NULL != wcschr( wszSwitches, *arg ) )
        {
            arg = argT;
            SetCurArgID( GetCurArgID() - 1 );
        }
    }

    while ( L':' == *arg )
    {
        arg++;
    }


    if ( L'\0' == *arg && !fAllowEmpty )
    {
        wprintf( wszUsageErr1, wszParamDesc );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        fResult = fFalse;
    }
    else if ( NULL == *pwszParam )
    {
        *pwszParam = (WCHAR *) arg;
    }
    else
    {
        wprintf( wszUsageErr2, wszParamDesc );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        fResult = fFalse;
    }

    return fResult;
}

LOCAL BOOL FEDBUTLParseDoublePath(  __in PCWSTR                 arg,
                                    __deref_inout_z PWSTR*      pwszParam1,
                                    __deref_opt_out_opt PWSTR*  pwszParam2,
                                    __in_z PCWSTR                   wszParamDesc,
                                    __in BOOL                   fAllowEmpty     = fFalse )
{
    BOOL fResult;

    fResult = FEDBUTLParsePath( arg, pwszParam1, wszParamDesc, fAllowEmpty );

    if ( fResult && pwszParam2 )
    {
        assert( *pwszParam1 );

        *pwszParam2 = NULL;
        if ( wcslen( *pwszParam1 ) >= 2 )
        {
            WCHAR * wszDelim = wcschr( (*pwszParam1) + 2, L':' );

            if ( wszDelim )
            {
                *pwszParam2 = wszDelim + 1;
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
    C_ASSERT( _countof(wszEfvOptionPrefix) == g_cchEfvOptionPrefix + 1  );
    if ( _wcsnicmp( wszArg, wszEfvOptionPrefix, g_cchEfvOptionPrefix ) == 0 ||
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
        if ( pgno == pdbutil->spcatOptions.pgnoFirst )
        {
            (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntBegin, NULL );
        }

        JET_SNPROG snprog =
        {
            sizeof( snprog ),
            pgno - pdbutil->spcatOptions.pgnoFirst + 1,
            pdbutil->spcatOptions.pgnoLast - pdbutil->spcatOptions.pgnoFirst + 1
        };
        (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntProgress, &snprog );

        if ( pgno == pdbutil->spcatOptions.pgnoLast )
        {
            (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntComplete, NULL );
        }
    }
    else
    {
        if ( pgno == pdbutil->spcatOptions.pgnoFirst )
        {
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            wprintf( L"Pgno,Objid,Unkn,NotOwnedEof,NotOwned,Leaked,Inc,Ind,Avail,SpaceAE,SpaceOE,SmallSpace,SpBuffer,Root,Internal,Leaf%c", wchNewLine );
        }

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

        if ( pgno == pdbutil->spcatOptions.pgnoLast )
        {
            wprintf( L"%c", wchNewLine );
        }
    }

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

LOCAL BOOL FEDBUTLParseDefragment( __in PCWSTR arg, UTILOPTS *popts )
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
            if ( FEseutilIsEfvArg( arg ) )
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

LOCAL BOOL FEDBUTLParseRecovery( __in PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fTrue;


    switch( arg[1] )
    {
        case L'i':
        case L'I':
            popts->grbitInit |= JET_bitReplayIgnoreMissingDB;
            break;

        case L'e':
        case L'E':
            fResult = fFalse;
            if ( FEseutilIsEfvArg( arg ) )
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
                popts->grbitInit &= ~JET_bitLogStreamMustExist;
                fResult = fTrue;
            }
            break;

        case L'a':
        case L'A':
            popts->grbitInit |= JET_bitReplayIgnoreLostLogs;
            UTILOPTSSetDelOutOfRangeLogs( popts->fUTILOPTSFlags );
#ifdef DEBUG
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

            lgposStop = lgposMax;

            assert( wcslen(arg) >= 2 );

            if ( arg[2] == L'\0' )
            {
                prstinfo->lgposStop = lgposStop;
                fResult = fTrue;
            }
#ifdef DEBUG
            else if ( swscanf_s( &(arg[2]), L"%u:%hu:%hu", &lgposStop.lGeneration, &lgposStop.isec, &lgposStop.ib ) == 3 )
            {
                prstinfo->lgposStop = lgposStop;
                fResult = fTrue;
            }
            
#endif
            else if ( swscanf_s( &(arg[2]), L"%u", &lgposStop.lGeneration ) == 1 )
            {
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
                        cMaxDbs = 6;
                    }

                    popts->crstmap = (LONG) cMaxDbs;
                    popts->prstmap = (JET_RSTMAP2_W *) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, popts->crstmap * sizeof(JET_RSTMAP2_W) );

                    if ( NULL == popts->prstmap )
                    {
                        wprintf( L"Out of memory error trying to allocate JET_RSTMAP of %zd bytes.", popts->crstmap * sizeof(JET_RSTMAP2_W) );
                        wprintf( L"%c%c", wchNewLine, wchNewLine );
                        popts->crstmap = 0;
                        fResult = fFalse;
                        break;
                    }

                    popts->irstmap = 0;
                }


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

LOCAL BOOL FEDBUTLParseIntegrity( __in PCWSTR arg, UTILOPTS *popts )
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
            if ( FEseutilIsEfvArg( arg ) )
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

LOCAL BOOL FEDBUTLParseChecksum( __in PCWSTR arg, UTILOPTS *popts )
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

LOCAL BOOL FEDBUTLParseRepair( __in PCWSTR arg, UTILOPTS *popts )
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
            if ( FEseutilIsEfvArg( arg ) )
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

LOCAL VOID EDBUTLGetBaseName( __in PCWSTR const wszLogfile, __out_ecount(4) WCHAR * const wszBaseName )
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
    return ( 3 == wcslen( wszFileT ) && 0 == wcslen( wszExtT ) );
}

LOCAL BOOL FEDBUTLParseDump( __in PCWSTR arg, UTILOPTS *popts )
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
                    fResult = fFalse;
                }
            }
            else
            {
                pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpVerboseLevel1;

                if ( arg[2] == L'\0'  || arg[2] == L'2' )
                {
                    pdbutil->grbitOptions |= JET_bitDBUtilOptionDumpVerboseLevel2;
                }
                else if ( arg[2] != L'1' )
                {
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
                    fResult = fFalse;
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

            assert( wcslen(arg) >= 2 );

            fResult = fTrue;
            if ( pdbutil->op != opDBUTILDumpSpaceCategory && pdbutil->op != opDBUTILDumpRBSHeader ) 
            {
                pdbutil->pgno = wcstol( arg + 2, NULL, 0 );
            }
            else if ( pdbutil->op == opDBUTILDumpRBSHeader ) 
            {
                pdbutil->op = opDBUTILDumpRBSPages;
                pdbutil->rbsOptions.pgnoFirst = 1;
                pdbutil->rbsOptions.pgnoLast = ulMax;

                fResult = fFalse;
                
                const SIZE_T cchArg = wcslen( arg + 2 );
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
                
                const SIZE_T cchArg = wcslen( arg + 2 );
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
            if ( opDBUTILDumpPage == pdbutil->op
                && NULL == pdbutil->szIndex
                && NULL == pdbutil->szTable )
            {
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
            if ( opDBUTILDumpPage == pdbutil->op
                && NULL != pdbutil->szIndex
                && NULL == pdbutil->szTable )
            {
                pdbutil->szTable = arg+2;
                fResult = fTrue;
            }
            else if ( opDBUTILDumpSpace == pdbutil->op )
            {
                UTILOPTSSetDebugMode( popts->fUTILOPTSFlags );
                SPDUMPOPTS fSPDumpOpts;
                if ( arg[2] == L'\0'  || arg[2] == L'1' )
                {
                    fSPDumpOpts = fSPDumpPrintSpaceTrees;
                }
                else if ( arg[2] == L'2' )
                {
                    fSPDumpOpts = SPDUMPOPTS( fSPDumpPrintSpaceTrees | fSPDumpPrintSpaceNodes );
                }
                else
                {
                    fResult = fFalse;
                    break;
                }
                fResult = fTrue;
                if( JET_errSuccess != ErrSpaceDumpCtxSetOptions(
                            pdbutil->pvCallback,
                            NULL, NULL, NULL, fSPDumpOpts ) )
                {
                    fResult = fFalse;
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
                    fResult = fFalse;
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
                    fResult = fFalse;
                }
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

LOCAL BOOL FEDBUTLParseHardRecovery( __in PCWSTR arg, UTILOPTS *popts )
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
        case L't':
        case L'T':
            if ( !FUTILOPTSDumpRestoreEnv(popts->fUTILOPTSFlags) )
            {
                if ( !popts->wszLogfilePath )
                {
                    fResult = FEDBUTLParsePath( arg+2, &popts->wszRestore, L"target instance", fTrue );
                }
                else
                {
                    wprintf( wszUsageErr4, arg );
                    wprintf( L"%c%c", wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
                break;
            }
        case L'f':
        case L'F':
            if ( !FUTILOPTSDumpRestoreEnv(popts->fUTILOPTSFlags) )
            {
                if ( !popts->wszRestore )
                {
                    fResult = FEDBUTLParsePath( arg+2, &popts->wszLogfilePath, L"logfile path", fFalse );
                }
                else
                {
                    wprintf( wszUsageErr4, arg );
                    wprintf( L"%c%c", wchNewLine, wchNewLine );
                    fResult = fFalse;
                }
                break;
            }
        default:
            wprintf( wszUsageErr4, arg );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            fResult = fFalse;
            break;
    }

    return fResult;
}

#ifdef DEBUG

LOCAL BOOL FEDBUTLParseBackup( __in PCWSTR arg, UTILOPTS *popts )
{
    BOOL    fResult = fFalse;

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

#endif

LOCAL BOOL FEDBUTLParseScrub( __in PCWSTR arg, UTILOPTS *popts )
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

LOCAL VOID EDBUTLGetUnpathedFilename( __in PCWSTR const wszFilename, __in_bcount(cbUnpathedFilename) PWSTR const wszUnpathedFilename, ULONG cbUnpathedFilename)
{
    WCHAR   wszFile[_MAX_FNAME+1];
    WCHAR   wszExt[_MAX_EXT+1];

    _wsplitpath_s( wszFilename, NULL, 0, NULL, 0, wszFile, sizeof( wszFile ) / sizeof( wszFile[0] ), wszExt, sizeof( wszExt ) / sizeof( wszExt[0] ) );
    _wmakepath_s( wszUnpathedFilename, cbUnpathedFilename / sizeof(WCHAR), NULL, NULL, wszFile, wszExt );
}

LOCAL BOOL FEDBUTLParseCopyFile( __in PCWSTR arg, UTILOPTS *popts )
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

LOCAL VOID InitArg( INT argc, __in_ecount(argc) LPWSTR argv[] )
{
    AssertPREFIX( argc > 0 );
    AssertPREFIX( NULL != argv );

    g_argMaxID = argc;
    g_argv = argv;
    g_argCurID = -1;
}

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
#endif

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
            pdbutil->pgno = -1;
            break;

        case L's':
        case L'S':
            pdbutil->op = opDBUTILDumpSpace;
            pdbutil->pfnCallback = EseutilEvalBTreeData;
            if ( ErrSpaceDumpCtxInit( &(pdbutil->pvCallback) ) < JET_errSuccess )
            {
                return fFalse;
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
#endif
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
    BOOL        (*pFEDBUTLParseMode)( __in PCWSTR arg, UTILOPTS *popts ) )
{
    BOOL        fResult = fTrue;
    WCHAR       *arg = GetCurArg();
    INT         iSkipID = -1;

    AssertPREFIX( NULL != arg );
    assert( NULL != arg );
    assert( NULL != wcschr( wszSwitches, *arg ) );
    assert( '\0' != *arg + 1 );

    if ( modeDump == popts->mode )
    {
        if ( L'\0' != arg[2] && L'\0' == arg[3] )
        {
            fResult = FSetDumpModeModifier( arg[2], (JET_DBUTIL_W *)popts->pv );
            arg++;
        }
        else
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
    else if ( modeHardRecovery == popts->mode )
    {
        if ( L'\0' != arg[2] && L'\0' == arg[3] )
        {
            fResult = FSetHardRecoveryModeModifier( arg[2], popts );
            arg ++;
        }
        else
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

    for ( ; fResult && NULL != arg; arg = GetNextArg() )
    {
        if ( GetCurArgID() == iSkipID )
        {
            continue;
        }

        if ( wcschr( wszSwitches, arg[0] ) == NULL )
        {
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

                if ( popts->wszBase != NULL && popts->wszBase[0] == '\0' )
                {
                    popts->wszBase = NULL;
                }

                popts->wszLogfilePath = GetNextArg();
                if ( !popts->wszLogfilePath )
                {
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
        else if ( 0 == _wcsnicmp( &(arg[1]), wszConfigArgPrefix, wcslen( wszConfigArgPrefix ) ) )
        {
            const WCHAR * wszConfigSpec = &( arg[ 1 + wcslen( wszConfigArgPrefix ) ] );
            if ( wszConfigSpec[0] == L':' )
            {
                wszConfigSpec++;
                fResult = ( wszConfigSpec[0] != L'\0' );
            }
            else
            {
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


LOCAL JET_ERR ErrEDBUTLRepair(
    const JET_SESID sesid,
    const WCHAR * const wszDatabase,
    const WCHAR * const wszBackup,
    const WCHAR * const wszTable,
    const WCHAR * const wszIntegPrefix,
    const JET_PFNSTATUS pfnStatus,
    const JET_GRBIT grbit
    )
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


LOCAL JET_ERR ErrEDBUTLScrub(
    JET_SESID   sesid,
    const WCHAR * const wszDatabase,
    JET_PFNSTATUS pfnStatus,
    JET_GRBIT grbit
    )
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


LOCAL DWORD CALLBACK DwEDBUTILCopyProgressRoutine(
    LARGE_INTEGER   cTotalFileSize,
    LARGE_INTEGER   cTotalBytesTransferred,
    LARGE_INTEGER   cStreamSize,
    LARGE_INTEGER   cStreamBytesTransferred,
    DWORD           dwStreamNumber,
    DWORD           dwCallbackReason,
    HANDLE          hSourceFile,
    HANDLE          hDestinationFile,
    INT             *pcShift
)
{
    enum { LITOI_LOW = 1, LITOI_HIGH, LITOI_SHIFT };
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
        if ( 0 == (ULONG)cTotalFileSize.HighPart )
        {
            *pcShift = LITOI_LOW;
        }
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
        case LITOI_LOW:
            assert( 0 == cTotalFileSize.HighPart );
            snpprog.cunitTotal = cTotalFileSize.LowPart;
            snpprog.cunitDone = cTotalBytesTransferred.LowPart;
            break;
        case LITOI_HIGH:
            assert( 100 <= cTotalFileSize.HighPart );
            snpprog.cunitTotal = cTotalFileSize.HighPart;
            snpprog.cunitDone = cTotalBytesTransferred.HighPart;
            break;
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
    const WCHAR * const wszExistingFileName,
    const WCHAR * const wszNewFileName,
    const DWORD dwFlags )
{
    wprintf( wszMoveFile, wszExistingFileName, wszNewFileName );
    if ( !MoveFileExW( wszExistingFileName, wszNewFileName, dwFlags ) )
    {
        DWORD dw = GetLastError();

        if ( ( dwFlags & ~MOVEFILE_REPLACE_EXISTING ) != 0 )
        {
            assert( fFalse );
            wprintf( L"%s%c", wszMoveFailed, wchNewLine );
            return ErrERRCheck( JET_errFileAccessDenied );
        }

        if ( dw == ERROR_NOT_SAME_DEVICE )
        {
            BOOL        fCancel     = fFalse;
            INT         cShift      = -1;
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

        EDBUTLDeleteTemp( popts );
    }

HandleError:
    return err;
}


LOCAL JET_ERR ErrEDBUTLUserSystemParameters( JET_INSTANCE *pinstance, UTILOPTS *popts )
{
    JET_ERR err = JET_errSuccess;

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

#ifdef ESENT
#else

LOCAL BOOL FAquireBackupRestoreRights()
{
    BOOL        ret_val = TRUE ;
    HANDLE  ProcessHandle;
    DWORD   DesiredAccess;
    HANDLE  TokenHandle;
    LUID        BackupValue;
    LUID        RestoreValue;
    TOKEN_PRIVILEGES NewState;



    ProcessHandle = GetCurrentProcess();


    DesiredAccess = MAXIMUM_ALLOWED;

    if ( ! OpenProcessToken( ProcessHandle, DesiredAccess, &TokenHandle ) )
    {
        return FALSE;
    }

    if ( ! LookupPrivilegeValueW( NULL, L"SeRestorePrivilege", &RestoreValue ) )
    {
        ret_val = FALSE;
    }

    if ( ! LookupPrivilegeValueW( NULL, L"SeBackupPrivilege", &BackupValue ) )
    {
        ret_val = FALSE;
    }


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

    if ( GetLastError() != ERROR_SUCCESS )
    {
        ret_val = FALSE;
    }


    CloseHandle( TokenHandle );
    return( ret_val );
}

#endif

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
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
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
        size_t cbFinalMsg = sizeof( WCHAR ) * ( wcslen( (WCHAR *)lpMsgBuf ) + 1 ) + 32;
        wszFinalMsg = (WCHAR *) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, cbFinalMsg );
        if ( wszFinalMsg )
        {
            StringCbPrintfW( wszFinalMsg, cbFinalMsg, (WCHAR *)lpMsgBuf, hrGLE );
            LocalFree( lpMsgBuf );
        }
        else
        {
            wszFinalMsg = (WCHAR *)lpMsgBuf;
        }
    }
    else
    {
        wszFinalMsg = (WCHAR *)lpMsgBuf;
    }


    hrESEBACK = hr;

    wszESEBACKError = wszFinalMsg;
    assert ( hrNone != hr );

    return JET_errSuccess;
}

typedef HRESULT (ESEBACK_API * PfnHrESERestoreReopen)(
    IN  WCHAR *                 wszServerName,
    IN  WCHAR *                 wszServiceAnnotation,
    IN  WCHAR *                 wszRestoreLogPath,
    OUT HCCX *                  phccxRestoreContext);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreClose)(
    IN HCCX hccxRestoreContext,
    IN ULONG fRestoreAbort);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreComplete)(
    IN  HCCX                hccxRestoreContext,
    IN  WCHAR *             wszRestoreInstanceSystemPath,
    IN  WCHAR *             wszRestoreInstanceLogPath,
    IN  WCHAR *             wszTargetInstanceName,
    IN  ULONG       fFlags);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreLoadEnvironment)(
    IN  WCHAR *             wszServerName,
    IN  WCHAR *             wszRestoreLogPath,
    OUT RESTORE_ENVIRONMENT **  ppRestoreEnvironment);

typedef HRESULT (ESEBACK_API * PfnHrESERestoreGetEnvironment)(
    IN  HCCX                    hccxRestoreContext,
    OUT RESTORE_ENVIRONMENT **  ppRestoreEnvironment);

typedef void (ESEBACK_API * PfnESERestoreFreeEnvironment)(
    IN  RESTORE_ENVIRONMENT *   pRestoreEnvironment);

typedef HRESULT (ESEBACK_API * PfnHrESERecoverAfterRestore2)(
    IN  WCHAR *         wszRestoreLogPath,
    IN  WCHAR *         wszCheckpointFilePath,
    IN  WCHAR *         wszLogFilePath,
    IN  WCHAR *         wszTargetInstanceCheckpointFilePath,
    IN  WCHAR *         wszTargetInstanceLogPath);

#endif


WCHAR * WszCopy( const WCHAR *  wsz )
{
    WCHAR *  wszCopy;
    LONG cb;

    assert ( wsz );
    cb = sizeof(WCHAR) * ((ULONG)wcslen( wsz ) + 1);

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
    CHAR * const szBeginErrExplanation = strchr( szFormatted, ',' );
    Assert( szBeginErrExplanation );
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

    return hr;
}


LOCAL VOID DBUTLIDumpRestoreEnv( RESTORE_ENVIRONMENT * pREnv, INT   cDesc = 0 )
{
    WCHAR           wszBuffer[64];

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
            wszStreams += wcslen( wszStreams ) + 1;
        }
        wprintf( L"\n" );
        wszStreams = pREnv->m_wszDatabaseStreamsD[iDb];
        PrintField( L"Destination Files:", cDesc, NULL, FALSE );
        while ( L'\0' != wszStreams[0] )
        {
            wprintf( L"%s ", wszStreams );
            wszStreams += wcslen( wszStreams ) + 1;
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

    CallHr ( HrDBUTLLoadRestoreEnv( hESEBCLI2, wszFullRestorePath, &pREnv, cDesc ) );

    assert ( pREnv );
    DBUTLIDumpRestoreEnv( pREnv, cDesc + 10 );

    assert ( pfnErrESERestoreReopen );
    PrintField( L"Restoring ....", 0, NULL );

    if ( wszPlayForwardLogs )
    {

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
                            (WCHAR*) wszFullRestorePath,
                            (WCHAR*) wszPlayForwardLogs,
                            (WCHAR*) wszPlayForwardLogs,
                            (WCHAR*) wszPlayForwardLogs,
                            (WCHAR*) wszPlayForwardLogs ) );
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
                wszTargetInstance = NULL;
            }
        }
        else
        {

            if ( NULL == pREnv )
            {
                assert ( pfnHrESERestoreGetEnv );
                CallHr ( (*pfnHrESERestoreGetEnv)(  hccxRestoreContext,
                                                    &pREnv ) );
            }

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
                                        fKeepLogs?ESE_RESTORE_KEEP_LOG_FILES:0
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

#endif

#endif



struct DUMPCOLUMN
{
    JET_COLUMNID    columnid;
    JET_COLTYP      coltyp;
    INT             cbMax;
    INT             cchPrint;
    CHAR            szColumnName[JET_cbNameMost+1];
};


LOCAL INT CchPrintForColtyp( IN const JET_COLTYP coltyp, IN const INT cchColumnName, IN const INT cbMax )
{
    INT cchPrint = 0;

    switch( coltyp )
    {
        case JET_coltypBit:
            cchPrint = 1;
            break;
        case JET_coltypUnsignedByte:
            cchPrint = 2;
            break;
        case JET_coltypShort:
            cchPrint = 6;
            break;
        case JET_coltypUnsignedShort:
            cchPrint = 6;
            break;
        case JET_coltypLong:
            cchPrint = 10;
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


LOCAL void PrintColumn( IN const JET_COLTYP coltyp, IN const INT cchMax, IN const void * const pv, IN const INT cb )
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
            wprintf( L"% *.4g", cchMax - 6, f );
            break;
        }
        case JET_coltypIEEEDouble:
        {
            const double d = *((double *)pv);
            wprintf( L"% *.4g", cchMax - 6, d );
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
            assert( 0 );
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


LOCAL JET_ERR ErrDumpcolumnFromColumnlist(
    IN const JET_SESID          sesid,
    IN const JET_COLUMNLIST&    columnlist,
    OUT DUMPCOLUMN * const      rgdumpcolumn,
    IN const ULONG      cdumpcolumnMax,
    OUT ULONG&          cdumpcolumn )
{
    JET_ERR err     = JET_errSuccess;
    const JET_TABLEID tableid = columnlist.tableid;

    cdumpcolumn = 0;


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


    while( 1 )
    {
        if( cdumpcolumn >= cdumpcolumnMax )
        {
            break;
        }

        memset( rgdumpcolumn[cdumpcolumn].szColumnName, 0, sizeof( rgdumpcolumn[0].szColumnName ) );

        rgretrievecolumn[iretcolName].pvData        = (void *)(rgdumpcolumn[cdumpcolumn].szColumnName);
        rgretrievecolumn[iretcolColumnid].pvData    = (void *)(&(rgdumpcolumn[cdumpcolumn].columnid));
        rgretrievecolumn[iretcolColtyp].pvData      = (void *)(&(rgdumpcolumn[cdumpcolumn].coltyp));
        rgretrievecolumn[iretcolCbMax].pvData       = (void *)(&(rgdumpcolumn[cdumpcolumn].cbMax));

        Call( JetRetrieveColumns( sesid, tableid, rgretrievecolumn, cretcol ) );


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
            err = JET_errSuccess;
            break;
        }
        Call( err );

        assert( cdumpcolumn <= columnlist.cRecord );
    }

HandleError:
    return err;
}


LOCAL void DBUTLDumpTableRecordsHeader(
    IN const DUMPCOLUMN * const rgdumpcolumn,
    IN const INT ccolumns )
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


LOCAL VOID EDBUTLPrintOneTableRecord(
    IN const DUMPCOLUMN * const     rgdumpcolumn,
    IN JET_RETRIEVECOLUMN * const   rgretcol,
    IN const INT                    ccolumns )
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


LOCAL JET_ERR ErrEDBUTLDumpOneTableRecord(
    IN const JET_SESID              sesid,
    IN const JET_TABLEID            tableid,
    IN const DUMPCOLUMN * const     rgdumpcolumn,
    IN JET_RETRIEVECOLUMN * const   rgretcol,
    IN const INT                    ccolumns )
{
    JET_ERR                 err         = JET_errSuccess;

    Call( JetRetrieveColumns( sesid, tableid, rgretcol, ccolumns ) );
    EDBUTLPrintOneTableRecord( rgdumpcolumn, rgretcol, ccolumns );

HandleError:
    return err;
}


LOCAL JET_ERR ErrEDBUTLDumpTableRecords(
    IN const JET_SESID          sesid,
    IN const JET_TABLEID        tableid,
    IN const DUMPCOLUMN * const rgdumpcolumn,
    IN const INT                ccolumns,
    IN const INT                crecordsMax )
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


LOCAL_BROKEN JET_ERR ErrEDBUTLDumpTable(
    IN const JET_INSTANCE   instance,
    IN const JET_SESID      sesid,
    IN const JET_DBID       dbid,
    IN const WCHAR * const  wszTable,
    IN const INT            crecordsMax )
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

LOCAL JET_ERR ErrEDBUTLDumpFTLHeader( IN const WCHAR * const wszFTLFile )
{
    JET_ERR         err         = JET_errSuccess;
#ifdef MINIMAL_FUNCTIONALITY
#else
    CFastTraceLog *                 pftl = NULL;
    CFastTraceLog::CFTLReader *     pftlr = NULL;

    Alloc( pftl = new CFastTraceLog( NULL ) );

    Call( pftl->ErrFTLInitReader( wszFTLFile,
                                    &g_iorThunk,
                                    CFastTraceLog::ftlifNone,
                                    &pftlr ) );

    
    Call( pftl->ErrFTLDumpHeader() );

HandleError:

    if ( err )
    {
        wprintf( L"ERROR: Processing FTL file header, %d\n", err );
    }

    if ( pftl )
    {
        pftl->FTLTerm();
        delete pftl;
        pftl = NULL;
        pftlr = NULL;
    }
#endif
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

        Assert( dwGLE != ERROR_INSUFFICIENT_BUFFER );
        if ( dwGLE == ERROR_MORE_DATA || dwGLE == ERROR_SUCCESS )
        {
            RETRIEVAL_POINTERS_BUFFER *pRetPtrs = (RETRIEVAL_POINTERS_BUFFER*)pData;

            for ( ULONG iExtent = 0; iExtent < pRetPtrs->ExtentCount; iExtent++ )
            {
                const LONGLONG iVCN = startingVCN.StartingVcn.QuadPart;
                const LONGLONG cbExtent = ( pRetPtrs->Extents[iExtent].NextVcn.QuadPart - iVCN ) * cbCluster;
                const LONGLONG iLCN = pRetPtrs->Extents[iExtent].Lcn.QuadPart;

                startingVCN.StartingVcn = pRetPtrs->Extents[iExtent].NextVcn;

                if ( iLCN == -1 )
                {
                    Assert( extentCount > 0 );
                    wprintf( L" (c)" );
                }
                else if ( iLCN >= 0 )
                {
                    const LONGLONG ibExtent = iLCN * cbCluster;
                    const LONGLONG ibFile = iVCN * cbCluster;

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

            if ( pctPercentile != pctPercentileLast )
            {
                wprintf( L"      Percentile:%-3u Length:%11.3f%c", pctPercentile, DblMBs( cbPercentileExtent ), wchNewLine );
                pctPercentileLast = pctPercentile;
            }

            if ( pctPercentile >= 100 )
            {
                Expected( pctPercentile == 100 );
                break;
            }

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
    wprintf( L"  Volume Name: %s%c", 0 == wcslen( wszVolumeName ) ? wszVolumeRoot : wszVolumeName, wchNewLine );
    wprintf( L"  File System: %s%c", wszFSName, wchNewLine );
    wprintf( L"  Cluster Size: %u bytes%c", cbCluster, wchNewLine );

    if ( 0 == _wcsicmp( wszFSName, L"NTFS" ) )
    {
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


    Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 0, NULL ) );


    Call( ErrEDBUTLUserSystemParameters( &instance, popts ) );

    if ( opDBUTILDumpFTLHeader == pdbutil->op )
    {
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
                pdbutil->pfnCallback = NULL;
             }
        }

        Call( JetDBUtilitiesW( pdbutil ) );
    }

HandleError:

    if ( opDBUTILDumpSpace == pdbutil->op )
    {
        err = ErrSpaceDumpCtxComplete( pdbutil->pvCallback, err  );
    }

    return err;
}

LOCAL JET_ERR ErrEDBUTLCheckLogStream( JET_INSTANCE* pInst, UTILOPTS* pOpts )
{
    JET_ERR err = JET_errSuccess;

    if (    modeRecovery != pOpts->mode ||
            FUTILOPTSExplicitPageSet( pOpts->fUTILOPTSFlags ) )
    {
        return JET_errSuccess;
    }

    WCHAR wszFName[ _MAX_PATH + 1 ];
    Call( JetGetSystemParameterW( *pInst, JET_sesidNil, JET_paramBaseName, NULL, wszFName, sizeof( wszFName ) ) );
    StringCchCatW( wszFName, _countof( wszFName ), L"*" );

    WCHAR wszLogFilePath[ _MAX_PATH + 1 ];
    Call( JetGetSystemParameterW( *pInst, JET_sesidNil, JET_paramLogFilePath, NULL, wszLogFilePath, sizeof( wszLogFilePath ) ) );

    WCHAR wszDrive[ _MAX_DRIVE ];
    WCHAR wszDir[ _MAX_DIR ];
    _wsplitpath_s( wszLogFilePath, wszDrive, _countof( wszDrive ), wszDir, _countof( wszDir ), NULL, 0, NULL, 0 );

    DWORD_PTR ulLegacy = 0;
    Call( JetGetSystemParameterW( *pInst, JET_sesidNil, JET_paramLegacyFileNames, &ulLegacy, NULL, sizeof( ulLegacy ) ) );

    const WCHAR* rgwszExt[] = { L"log", L"jtx", };
    INT iExtIdx = !( ulLegacy & JET_bitESE98FileNames );

    assert( 2 == _countof( rgwszExt ) );
    for ( size_t i = 0; i < _countof( rgwszExt ); ++i, iExtIdx = 1 - iExtIdx )
    {
        WCHAR wszPath[ _MAX_PATH ];
        _wmakepath_s( wszPath, _countof( wszPath ), wszDrive, wszDir, wszFName, rgwszExt[ iExtIdx ] );

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
        cbAllArgs += sizeof(WCHAR) * ( wcslen( argv[iarg] ) + 2  );
    }
    cbAllArgs += sizeof(WCHAR) + 783;
    CHAR * szEseutilCmd = (CHAR*)alloca( cbAllArgs );
    if ( szEseutilCmd )
    {
        CHAR * szT = szEseutilCmd;

        (void)ErrOSStrCbFormatA( szT, cbAllArgs, "Command:     \"" );
        size_t cbUsed = strlen( szT );
        szT += cbUsed;
        cbAllArgs -= cbUsed;

        for( INT iarg = 0; iarg < argc; iarg++ )
        {
            (void)ErrOSStrCbFormatA( szT, cbAllArgs, "%ws ", argv[iarg] );
            cbUsed = strlen( szT );
            szT += cbUsed;
            cbAllArgs -= cbUsed;
        }
        if ( argc )
        {
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
    COSLayerPreInit         oslayer;

    memset( &opts, 0, sizeof(UTILOPTS) );

    ULONG                   timer                   = 0;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        return ErrERRCheck( JET_errOutOfMemory );
    }


    COSLayerPreInit::DisablePerfmon();
    COSLayerPreInit::DisableTracing();

    Call( (JET_ERR)ErrOSInit() );


    timer = TickOSTimeCurrent();


#ifdef USE_WATSON_API
    RegisterWatson();
#endif

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
        case L'd':
        case L'D':
            opts.mode   = modeDefragment;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseDefragment );
            break;

        case L'r':
        case L'R':
            opts.mode = modeRecovery;
            opts.grbitInit = JET_bitLogStreamMustExist;
            opts.pv = &rstInfo;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseRecovery );
            break;

        case L'g':
        case L'G':
            opts.mode = modeIntegrity;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseIntegrity );
            break;

        case L'k':
        case L'K':
            opts.mode = modeChecksum;
            UTILOPTSSetChecksumEDB( opts.fUTILOPTSFlags );
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseChecksum );
            break;

        case L'p':
        case L'P':
            opts.mode   = modeRepair;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseRepair );
            break;

        case L'm':
        case L'M':
            opts.mode = modeDump;
            opts.pv = &dbutil;

            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseDump );
            break;

        case L'y':
        case L'Y':
            opts.mode = modeCopyFile;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseCopyFile );
            break;

#ifndef ESENT
        case L'c':
        case L'C':
            opts.mode = modeHardRecovery;
            opts.pv = &dbutil;

            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseHardRecovery );
            break;
#endif

#ifdef DEBUG
        case L'b':
        case L'B':
            opts.mode = modeBackup;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseBackup );
            break;
#endif

        case L'z':
        case L'Z':
            opts.mode   = modeScrub;
            fResult = FEDBUTLParseOptions( &opts, FEDBUTLParseScrub );
            break;

        case L'?':
            goto Usage;

        default:
            wprintf( L"%s%c%c", wszUsageErr12, wchNewLine, wchNewLine );
            fResult = fFalse;
    }

    if ( !fResult )
        goto Usage;

    (void)JetSetSystemParameterW( &instance, 0, JET_paramConfiguration, 1, NULL );

    if ( opts.wszConfigStoreSpec )
    {
        err = JetSetSystemParameterW( &instance, 0, JET_paramConfigStoreSpec, 0, opts.wszConfigStoreSpec );
        if ( err < JET_errSuccess )
        {
            wprintf( wszUsageErr21 );
            wprintf( L"%c%c", wchNewLine, wchNewLine );
            Call( err );
        }
    }

    Call( JetSetSystemParameterW( &instance, 0, JET_paramRecovery, 0, L"ESEUTIL" ) );


    Call( JetSetSystemParameterW( &instance, 0, JET_paramEnablePersistedCallbacks, 1, NULL ) );


    Call( JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, wszDefaultTempDB ) );

    Call( JetSetSystemParameter( &instance, 0, JET_paramPersistedLostFlushDetection, JET_bitPersistedLostFlushDetectionEnabled, NULL ) );

    if ( !FUTILOPTSSuppressLogo( opts.fUTILOPTSFlags ) )
    {
        EDBUTLPrintLogo();
    }

    if ( modeHardRecovery == opts.mode || modeBackup == opts.mode )
    {
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

    timer = TickOSTimeCurrent();

#ifdef ESESHADOW
    WCHAR wszShadowedFile[ _MAX_PATH ];
    WCHAR wszShadowedLog[ _MAX_PATH ];
    WCHAR wszShadowedSystem[ _MAX_PATH ];

    if ( FUTILOPTSOperateTempVss( opts.fUTILOPTSFlags ) &&
        NULL != opts.wszSourceDB )
    {


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

            Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableShrinkDatabase, JET_bitShrinkDatabaseOff, NULL ) );

            Assert( opts.efvUserSpecified == 0 );
            Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, opts.efvUserSpecified == 0 ? ( JET_efvExchange2016Rtm | JET_efvAllowHigherPersistedFormat ) : opts.efvUserSpecified, NULL ) );

            wprintf( L"%c", wchNewLine );

            Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );

            Call( ErrEDBUTLCheckLogStream( &instance, &opts ) );

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
                fWhitespaceOnErr = fTrue;
                wprintf( L"Performing soft recovery..." );
                if ( FUTILOPTSVerbose( opts.fUTILOPTSFlags ) )
                {
                    wprintf( L"\n" );
                }

                err = JetInit4W( &instance, &rstInfo, opts.grbitInit );

                Assert( sesid == JET_sesidNil );

                if ( ( err >= JET_errSuccess ) &&
                    ( ( opts.grbitShrink & JET_bitShrinkDatabaseEofOnAttach ) || ( opts.fRunLeakReclaimer ) ) &&
                    ( ( err = JetBeginSessionW( instance, &sesid, NULL, NULL ) ) >= JET_errSuccess ) )
                {
                    wprintf( L"%c", wchNewLine );
                    wprintf( L"Attaching for additional database operations...%c", wchNewLine );
                    wprintf( L"             Database: %s%c", opts.wszSourceDB, wchNewLine );
                    wprintf( L"             Shrink options: 0x%I32x%c", opts.grbitShrink, wchNewLine );
                    wprintf( L"             Leakage reclaimer: %d%c", (int)opts.fRunLeakReclaimer, wchNewLine );

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
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 128, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10240, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, JET_efvUsePersistedFormat, NULL ) );

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

                if ( JET_errDatabaseCorruptedNoRepair == err )
                {
                    err                 = JET_errSuccess;
                    ErrEDBUTLCleanup( instance, sesid, err );
                    instance            = 0;
                    sesid               = JET_sesidNil;
                    Call( JetSetSystemParameterW( &instance, 0, JET_paramTempPath, 0, wszTempDatabaseDefault ) );

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

                Call( ErrEDBUTLUserSystemParameters( &instance, &opts ) );
                Call( JetInit( &instance ) );
                CallJ( JetBeginSessionW( instance, &sesid, wszUser, wszPassword ), Cleanup );

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

                        UTILOPTSResetChecksumEDB( opts.fUTILOPTSFlags );

                        opts.pv = &dbutil;
                        opts.mode = modeDump;
                        dbutil.op = opDBUTILDumpLogfile;
                        opts.wszBase = wszBaseName;
                        dbutil.grbitOptions = JET_bitDBUtilOptionVerify;
                        EDBUTLGetBaseName( opts.wszSourceDB, opts.wszBase );

                        Call( JetSetSystemParameterW( &instance, 0, JET_paramEnableIndexChecking, JET_IndexCheckingOff, NULL ) );
                        Call( JetSetSystemParameterW( &instance, 0, JET_paramDisableCallbacks, fTrue, NULL ) );

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

                        UTILOPTSResetChecksumEDB( opts.fUTILOPTSFlags );

                        opts.pv = &dbutil;
                        opts.mode = modeDump;
                        dbutil.op = opDBUTILDumpFlushMapFile;
                        dbutil.grbitOptions = JET_bitDBUtilOptionVerify;

                        Call( ErrEDBUTLDump( &dbutil, &opts ) );
                    }
                        break;

                    default:
                        AssertSz( false, "Invalid file type %d found.", filetype );


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

                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 16, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10240, NULL ) );

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
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxSessions, 128, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxCursors, 10240, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramMaxTemporaryTables, 10000, NULL ) );
                Call( JetSetSystemParameterW( &instance, 0, JET_paramEngineFormatVersion, JET_efvUsePersistedFormat, NULL ) );

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

            if ( _wfullpath( wszFullRestorePath, opts.wszSourceDB?opts.wszSourceDB:L".", _MAX_PATH ) == NULL )
            {
                Call ( JET_errInvalidPath );
            }

            hrESEBACK = hrNone;

            if ( FUTILOPTSServerSim ( opts.fUTILOPTSFlags ) )
            {
#ifdef RESTORE_SERVER_SIMULATION

                err = ErrDBUTLServerSim( opts.wszSourceDB?wszFullRestorePath:NULL );

#else
                assert ( fFalse );
#endif
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
#endif

        case modeDump:
        default:
        {
            WCHAR   wszBaseName[4];

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
                    wprintf( wszUsageErr1, L"database/filename" );
                    wprintf( L"%c%c", wchNewLine, wchNewLine );
                    Call( ErrERRCheck( JET_errInvalidParameter ) );
                }

                wprintf( L"Initiating FILE DUMP mode...%c", wchNewLine );

                switch( dbutil.op )
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

                        if ( dbutil.op == opDBUTILDumpSpace &&
                            ( err == JET_errSuccess || err == JET_errFileAccessDenied || err == JET_errDatabaseCorrupted ) )
                        {
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

            err = ErrEDBUTLDump( &dbutil, &opts );

            if ( ( err < JET_errSuccess ) && ( err != JET_errInvalidParameter ) &&
                    ( dbutil.op == opDBUTILDumpSpaceCategory ) && !( dbutil.grbitOptions & JET_bitDBUtilOptionDumpVerbose ) )
            {
                (void)PrintStatus( JET_sesidNil, JET_snpSpaceCategorization, JET_sntFail, NULL );
            }

            wprintf( L"%c", wchNewLine );

            dbutil.op = opDBUTILDumpCachedFileHeader;
            (void)ErrEDBUTLDump( &dbutil, &opts );

            Call( err );
            break;
        }
    }

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

    if( JET_errSuccess != errRepaired )
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
    else
    {
        wprintf( wszOperSuccess, iSec, iMSec );
        wprintf( L"%c%c", wchNewLine, wchNewLine );
        PushEseutilEndMarker( iSec, iMSec, JET_errSuccess, L"JET_errSuccess, Operation was successful." );
    }

    OSTerm();
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

    wprintf( L"%c%c", wchNewLine, wchNewLine );
    return err;

#ifndef ESENT
ESEBACKError:
    assert ( hrNone != hrESEBACK );
    assert ( wszESEBACKError );
    PrintESEUTILError( hrESEBACK, wszESEBACKError, fWhitespaceOnErr, timer );
    LocalFree( wszESEBACKError );
    OSTerm();
    return hrESEBACK;
#endif


Usage:
    SetCurArgID( 0 );
    assert( GetCurArgID() == 0 );

    _wcsupr_s( GetCurArg(), wcslen( GetCurArg() ) + 1 );
    EDBUTLHelp( GetCurArg() );
    
    OSTerm();
    return -1;
}

