// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osustd.hxx"
#include "esestd.hxx"




LOCAL const DWORD rgparam[] =
{
    JET_paramSystemPath,
    JET_paramTempPath,
    JET_paramLogFilePath,
    JET_paramLogFileFailoverPath,
    JET_paramBaseName,
    JET_paramMaxSessions,
    JET_paramMaxOpenTables,
    JET_paramPreferredMaxOpenTables,
    JET_paramMaxCursors,
    JET_paramMaxVerPages,
    JET_paramGlobalMinVerPages,
    JET_paramPreferredVerPages,
    JET_paramMaxTemporaryTables,
    JET_paramLogFileSize,
    JET_paramLogBuffers,
    JET_paramLogCheckpointPeriod,
    JET_paramCommitDefault,
    JET_paramCircularLog,
    JET_paramDbExtensionSize,
    JET_paramPageTempDBMin,
    JET_paramPageFragment,
    JET_paramVersionStoreTaskQueueMax,
    JET_paramCacheSizeMin,
    JET_paramCacheSizeMax,
    JET_paramCheckpointDepthMax,
    JET_paramLRUKCorrInterval,
    JET_paramLRUKHistoryMax,
    JET_paramLRUKPolicy,
    JET_paramLRUKTimeout,
    JET_paramStartFlushThreshold,
    JET_paramStopFlushThreshold,
    JET_paramExceptionAction,
    JET_paramEventLogCache,
    JET_paramRecovery,
    JET_paramEnableOnlineDefrag,
    JET_paramAssertAction,
    JET_paramRFS2IOsPermitted,
    JET_paramRFS2AllocsPermitted,
    JET_paramCheckFormatWhenOpenFail,
    JET_paramEnableIndexChecking,
    JET_paramEnableTempTableVersioning,
    JET_paramZeroDatabaseDuringBackup,
    JET_paramIgnoreLogVersion,
    JET_paramDeleteOldLogs,
    JET_paramEnableImprovedSeekShortcut,
    JET_paramBackupChunkSize,
    JET_paramBackupOutstandingReads,
    JET_paramCreatePathIfNotExist,
    JET_paramPageHintCacheSize,
    JET_paramLegacyFileNames,
    JET_paramPeriodicLogRolloverLLR,
    DWORD( ~0 )
};

LOCAL const WCHAR* const rglpwszParam[] =
{
    L"SystemPath",
    L"TempPath",
    L"LogFilePath",
    L"LogFileFailoverPath",
    L"BaseName",
    L"MaxSessions",
    L"MaxOpenTables",
    L"PreferredMaxOpenTables",
    L"MaxCursors",
    L"MaxVerPages",
    L"GlobalMinVerPages",
    L"PreferredVerPages",
    L"MaxTemporaryTables",
    L"LogFileSize",
    L"LogBuffers",
    L"LogCheckpointPeriod",
    L"CommitDefault",
    L"CircularLog",
    L"DbExtensionSize",
    L"PageTempDBMin",
    L"PageFragment",
    L"VERTasksPostMax",
    L"CacheSizeMin",
    L"CacheSizeMax",
    L"CheckpointDepthMax",
    L"LRUKCorrInterval",
    L"LRUKHistoryMax",
    L"LRUKPolicy",
    L"LRUKTimeout",
    L"StartFlushThreshold",
    L"StopFlushThreshold",
    L"ExceptionAction",
    L"EventLogCache",
    L"Recovery",
    L"EnableOnlineDefrag",
    L"AssertAction",
    L"RFS2IOsPermitted",
    L"RFS2AllocsPermitted",
    L"CheckFormatWhenOpenFail",
    L"EnableIndexChecking",
    L"EnableTempTableVersioning",
    L"ZeroDatabaseDuringBackup",
    L"IgnoreLogVersion",
    L"DeleteOldLogs",
    L"EnableImprovedSeekShortcut",
    L"BackupChunkSize",
    L"BackupOutstandingReads",
    L"CreatePathIfNotExist",
    L"PageHintCacheSize",
    L"LegacyFileNames",
    L"PeriodicLogRolloverLLR",
    NULL
};



LOCAL const WCHAR wszParamRoot[] = L"System Parameter Overrides";

LOCAL void OSUConfigLoadParameterOverrides()
{

    for ( INT iparam = 0; rglpwszParam[iparam]; iparam++ )
    {
        WCHAR wszParam[512];
        if ( FOSConfigGet(  wszParamRoot,
                            rglpwszParam[iparam],
                            wszParam,
                            sizeof( wszParam ) ) )
        {
            if ( wszParam[0] )
            {
                extern ERR ErrSetSystemParameter(
                    INST*           pinst,
                    JET_SESID       sesid,
                    ULONG   paramid,
                    ULONG_PTR       ulParam,
                    __in PCWSTR     wszParam,
                    const BOOL      fEnterCritInst = fTrue );

                (VOID)ErrSetSystemParameter(
                                0,
                                0,
                                rgparam[iparam],
                                wcstoul( wszParam, NULL, 0 ),
                                wszParam,
                                fFalse );
            }
        }
        else
        {
        }
    }
}



void OSUConfigTerm()
{
}


ERR ErrOSUConfigInit()
{

    OSUConfigLoadParameterOverrides();

    return JET_errSuccess;
}

ERR ErrLoadDiagOption_( _In_ const INST * const pinst, _In_z_ PCWSTR wszOverrideName, _Out_ BOOL * pfResult )
{
    ERR err = JET_errSuccess;
    CConfigStore * pcs = NULL;

    const WCHAR * const wszConfigStore = SzParam( pinst, JET_paramConfigStoreSpec );
    if ( wszConfigStore )
    {
        Call( ErrOSConfigStoreInit( wszConfigStore, &pcs ) );

        Call( ErrConfigReadValue( pcs, csspDiag, wszOverrideName, (ULONG*)pfResult ) );
    }

HandleError:
    OSConfigStoreTerm( pcs );

    return err;
}

