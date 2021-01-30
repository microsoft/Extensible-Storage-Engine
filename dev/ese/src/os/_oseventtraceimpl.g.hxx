// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


void ETW_TraceBaseId_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TraceBaseId_Trace(
 );
}

void ETW_BF_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_BF_Trace(
 );
}

void ETW_Block_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_Block_Trace(
 );
}

void ETW_CacheNewPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheNewPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(DWORD *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        *(BYTE *)rgparg[15].DataPtr,
        *(BYTE *)rgparg[16].DataPtr,
        *(QWORD *)rgparg[17].DataPtr,
        *(USHORT *)rgparg[18].DataPtr,
        *(USHORT *)rgparg[19].DataPtr );
}

void ETW_CacheReadPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheReadPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(DWORD *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        *(BYTE *)rgparg[15].DataPtr,
        *(BYTE *)rgparg[16].DataPtr,
        *(QWORD *)rgparg[17].DataPtr,
        *(USHORT *)rgparg[18].DataPtr,
        *(USHORT *)rgparg[19].DataPtr );
}

void ETW_CachePrereadPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CachePrereadPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(BYTE *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr );
}

void ETW_CacheWritePage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheWritePage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(DWORD *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        *(BYTE *)rgparg[15].DataPtr,
        *(BYTE *)rgparg[16].DataPtr,
        *(BYTE *)rgparg[17].DataPtr );
}

void ETW_CacheEvictPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheEvictPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(INT *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr );
}

void ETW_CacheRequestPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheRequestPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr );
}

void ETW_LatchPageDeprecated_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_LatchPageDeprecated_Trace(
 );
}

void ETW_CacheDirtyPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheDirtyPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(QWORD *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(DWORD *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        *(BYTE *)rgparg[15].DataPtr,
        *(BYTE *)rgparg[16].DataPtr,
        *(BYTE *)rgparg[17].DataPtr,
        *(BYTE *)rgparg[18].DataPtr );
}

void ETW_TransactionBegin_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TransactionBegin_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr );
}

void ETW_TransactionCommit_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TransactionCommit_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr );
}

void ETW_TransactionRollback_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TransactionRollback_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr );
}

void ETW_SpaceAllocExt_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_SpaceAllocExt_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr );
}

void ETW_SpaceFreeExt_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_SpaceFreeExt_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr );
}

void ETW_SpaceAllocPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_SpaceAllocPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr );
}

void ETW_SpaceFreePage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_SpaceFreePage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr );
}

void ETW_IorunEnqueue_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IorunEnqueue_Trace(
        *(QWORD *)rgparg[0].DataPtr,
        *(QWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr,
        *(QWORD *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr );
}

void ETW_IorunDequeue_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IorunDequeue_Trace(
        *(QWORD *)rgparg[0].DataPtr,
        *(QWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr,
        *(DWORD *)rgparg[9].DataPtr,
        *(DWORD *)rgparg[10].DataPtr,
        *(QWORD *)rgparg[11].DataPtr,
        *(DWORD *)rgparg[12].DataPtr,
        *(QWORD *)rgparg[13].DataPtr,
        *(QWORD *)rgparg[14].DataPtr,
        *(USHORT *)rgparg[15].DataPtr,
        *(DWORD *)rgparg[16].DataPtr );
}

void ETW_IOCompletion_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOCompletion_Trace(
        *(QWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        *(QWORD *)rgparg[15].DataPtr,
        *(DWORD *)rgparg[16].DataPtr,
        *(DWORD *)rgparg[17].DataPtr,
        *(DWORD *)rgparg[18].DataPtr,
        *(QWORD *)rgparg[19].DataPtr,
        *(DWORD *)rgparg[20].DataPtr,
        *(DWORD *)rgparg[21].DataPtr,
        *(DWORD *)rgparg[22].DataPtr,
        *(QWORD *)rgparg[23].DataPtr,
        *(DWORD *)rgparg[24].DataPtr,
        *(DWORD *)rgparg[25].DataPtr,
        *(DWORD *)rgparg[26].DataPtr,
        *(QWORD *)rgparg[27].DataPtr );
}

void ETW_LogStall_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_LogStall_Trace(
 );
}

void ETW_LogWrite_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_LogWrite_Trace(
        *(INT *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr );
}

void ETW_EventLogInfo_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_EventLogInfo_Trace(
        (PCWSTR)rgparg[0].DataPtr );
}

void ETW_EventLogWarn_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_EventLogWarn_Trace(
        (PCWSTR)rgparg[0].DataPtr );
}

void ETW_EventLogError_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_EventLogError_Trace(
        (PCWSTR)rgparg[0].DataPtr );
}

void ETW_TimerQueueScheduleDeprecated_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TimerQueueScheduleDeprecated_Trace(
 );
}

void ETW_TimerQueueRunDeprecated_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TimerQueueRunDeprecated_Trace(
 );
}

void ETW_TimerQueueCancelDeprecated_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TimerQueueCancelDeprecated_Trace(
 );
}

void ETW_TimerTaskSchedule_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TimerTaskSchedule_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr,
        *(const void * *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr );
}

void ETW_TimerTaskRun_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TimerTaskRun_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr,
        *(const void * *)rgparg[3].DataPtr,
        *(QWORD *)rgparg[4].DataPtr );
}

void ETW_TimerTaskCancel_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TimerTaskCancel_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr );
}

void ETW_TaskManagerPost_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TaskManagerPost_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(const void * *)rgparg[3].DataPtr );
}

void ETW_TaskManagerRun_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TaskManagerRun_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(const void * *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(const void * *)rgparg[5].DataPtr );
}

void ETW_GPTaskManagerPost_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_GPTaskManagerPost_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr,
        *(const void * *)rgparg[3].DataPtr );
}

void ETW_GPTaskManagerRun_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_GPTaskManagerRun_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr,
        *(const void * *)rgparg[3].DataPtr );
}

void ETW_TestMarker_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_TestMarker_Trace(
        *(QWORD *)rgparg[0].DataPtr,
        (PCSTR)rgparg[1].DataPtr );
}

void ETW_ThreadCreate_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_ThreadCreate_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr );
}

void ETW_ThreadStart_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_ThreadStart_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(const void * *)rgparg[1].DataPtr,
        *(const void * *)rgparg[2].DataPtr );
}

void ETW_CacheVersionPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheVersionPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr );
}

void ETW_CacheVersionCopyPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheVersionCopyPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr );
}

void ETW_CacheResize_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheResize_Trace(
        *(__int64 *)rgparg[0].DataPtr,
        *(__int64 *)rgparg[1].DataPtr,
        *(__int64 *)rgparg[2].DataPtr,
        *(__int64 *)rgparg[3].DataPtr );
}

void ETW_CacheLimitResize_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheLimitResize_Trace(
        *(__int64 *)rgparg[0].DataPtr,
        *(__int64 *)rgparg[1].DataPtr );
}

void ETW_CacheScavengeProgress_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheScavengeProgress_Trace(
        *(__int64 *)rgparg[0].DataPtr,
        *(INT *)rgparg[1].DataPtr,
        *(INT *)rgparg[2].DataPtr,
        *(INT *)rgparg[3].DataPtr,
        *(INT *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(INT *)rgparg[6].DataPtr,
        *(INT *)rgparg[7].DataPtr,
        *(INT *)rgparg[8].DataPtr,
        *(INT *)rgparg[9].DataPtr,
        *(INT *)rgparg[10].DataPtr,
        *(INT *)rgparg[11].DataPtr,
        *(INT *)rgparg[12].DataPtr,
        *(INT *)rgparg[13].DataPtr,
        *(INT *)rgparg[14].DataPtr,
        *(INT *)rgparg[15].DataPtr );
}

void ETW_ApiCall_Start_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_ApiCall_Start_Trace(
        *(DWORD *)rgparg[0].DataPtr );
}

void ETW_ApiCall_Stop_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_ApiCall_Stop_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(INT *)rgparg[1].DataPtr );
}

void ETW_ResMgrInit_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_ResMgrInit_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(INT *)rgparg[1].DataPtr,
        *(double *)rgparg[2].DataPtr,
        *(double *)rgparg[3].DataPtr,
        *(double *)rgparg[4].DataPtr,
        *(double *)rgparg[5].DataPtr,
        *(double *)rgparg[6].DataPtr,
        *(double *)rgparg[7].DataPtr );
}

void ETW_ResMgrTerm_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_ResMgrTerm_Trace(
        *(DWORD *)rgparg[0].DataPtr );
}

void ETW_CacheCachePage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheCachePage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr );
}

void ETW_MarkPageAsSuperCold_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_MarkPageAsSuperCold_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr );
}

void ETW_CacheMissLatency_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheMissLatency_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(BYTE *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(QWORD *)rgparg[14].DataPtr,
        *(BYTE *)rgparg[15].DataPtr,
        *(BYTE *)rgparg[16].DataPtr );
}

void ETW_BTreePrereadPageRequest_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_BTreePrereadPageRequest_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(BYTE *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr );
}

void ETW_DiskFlushFileBuffers_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DiskFlushFileBuffers_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        (PCWSTR)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(QWORD *)rgparg[3].DataPtr,
        *(QWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr );
}

void ETW_DiskFlushFileBuffersBegin_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DiskFlushFileBuffersBegin_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(QWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr );
}

void ETW_CacheFirstDirtyPage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheFirstDirtyPage_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(QWORD *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(BYTE *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(DWORD *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        *(BYTE *)rgparg[15].DataPtr,
        *(BYTE *)rgparg[16].DataPtr,
        *(BYTE *)rgparg[17].DataPtr,
        *(BYTE *)rgparg[18].DataPtr );
}

void ETW_SysStationId_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_SysStationId_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        (PCWSTR)rgparg[5].DataPtr );
}

void ETW_InstStationId_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_InstStationId_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr,
        (PCWSTR)rgparg[3].DataPtr,
        (PCWSTR)rgparg[4].DataPtr );
}

void ETW_FmpStationId_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_FmpStationId_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(BYTE *)rgparg[3].DataPtr,
        (PCWSTR)rgparg[4].DataPtr );
}

void ETW_DiskStationId_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DiskStationId_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        (PCWSTR)rgparg[2].DataPtr,
        (PCSTR)rgparg[3].DataPtr,
        (PCSTR)rgparg[4].DataPtr,
        (PCSTR)rgparg[5].DataPtr );
}

void ETW_FileStationId_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_FileStationId_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(QWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(QWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(QWORD *)rgparg[6].DataPtr,
        (PCWSTR)rgparg[7].DataPtr );
}

void ETW_IsamDbfilehdrInfo_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IsamDbfilehdrInfo_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(DWORD *)rgparg[6].DataPtr,
        (const BYTE *)rgparg[7].DataPtr );
}

void ETW_DiskOsDiskCacheInfo_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DiskOsDiskCacheInfo_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        (const BYTE *)rgparg[1].DataPtr );
}

void ETW_DiskOsStorageWriteCacheProp_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DiskOsStorageWriteCacheProp_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        (const BYTE *)rgparg[1].DataPtr );
}

void ETW_DiskOsDeviceSeekPenaltyDesc_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DiskOsDeviceSeekPenaltyDesc_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        (const BYTE *)rgparg[1].DataPtr );
}

void ETW_DirtyPage2Deprecated_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_DirtyPage2Deprecated_Trace(
 );
}

void ETW_IOCompletion2_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOCompletion2_Trace(
        (PCWSTR)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        (PCSTR)rgparg[15].DataPtr,
        (PCSTR)rgparg[16].DataPtr,
        (PCSTR)rgparg[17].DataPtr,
        (const GUID *)rgparg[18].DataPtr,
        *(QWORD *)rgparg[19].DataPtr,
        *(DWORD *)rgparg[20].DataPtr,
        *(DWORD *)rgparg[21].DataPtr,
        *(DWORD *)rgparg[22].DataPtr,
        *(QWORD *)rgparg[23].DataPtr,
        *(DWORD *)rgparg[24].DataPtr,
        *(DWORD *)rgparg[25].DataPtr,
        *(DWORD *)rgparg[26].DataPtr,
        *(QWORD *)rgparg[27].DataPtr,
        *(DWORD *)rgparg[28].DataPtr,
        *(DWORD *)rgparg[29].DataPtr,
        *(DWORD *)rgparg[30].DataPtr );
}

void ETW_FCBPurgeFailure_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_FCBPurgeFailure_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(BYTE *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr,
        *(BYTE *)rgparg[3].DataPtr );
}

void ETW_IOLatencySpikeNotice_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOLatencySpikeNotice_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr );
}

void ETW_IOCompletion2Sess_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOCompletion2Sess_Trace(
        (PCWSTR)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(BYTE *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(BYTE *)rgparg[4].DataPtr,
        *(BYTE *)rgparg[5].DataPtr,
        *(BYTE *)rgparg[6].DataPtr,
        *(BYTE *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr,
        *(BYTE *)rgparg[9].DataPtr,
        *(BYTE *)rgparg[10].DataPtr,
        *(BYTE *)rgparg[11].DataPtr,
        *(BYTE *)rgparg[12].DataPtr,
        *(BYTE *)rgparg[13].DataPtr,
        *(BYTE *)rgparg[14].DataPtr,
        (PCSTR)rgparg[15].DataPtr,
        (PCSTR)rgparg[16].DataPtr,
        (PCSTR)rgparg[17].DataPtr,
        (const GUID *)rgparg[18].DataPtr,
        *(QWORD *)rgparg[19].DataPtr,
        *(DWORD *)rgparg[20].DataPtr,
        *(DWORD *)rgparg[21].DataPtr,
        *(DWORD *)rgparg[22].DataPtr,
        *(QWORD *)rgparg[23].DataPtr,
        *(DWORD *)rgparg[24].DataPtr,
        *(DWORD *)rgparg[25].DataPtr,
        *(DWORD *)rgparg[26].DataPtr,
        *(QWORD *)rgparg[27].DataPtr,
        *(DWORD *)rgparg[28].DataPtr,
        *(DWORD *)rgparg[29].DataPtr,
        *(DWORD *)rgparg[30].DataPtr );
}

void ETW_IOIssueThreadPost_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOIssueThreadPost_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr );
}

void ETW_IOIssueThreadPosted_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOIssueThreadPosted_Trace(
        *(const void * *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(QWORD *)rgparg[2].DataPtr );
}

void ETW_IOThreadIssueStart_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOThreadIssueStart_Trace(
 );
}

void ETW_IOThreadIssuedDisk_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOThreadIssuedDisk_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(BYTE *)rgparg[1].DataPtr,
        *(__int64 *)rgparg[2].DataPtr,
        *(INT *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(QWORD *)rgparg[5].DataPtr );
}

void ETW_IOThreadIssueProcessedIO_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOThreadIssueProcessedIO_Trace(
        *(INT *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(QWORD *)rgparg[2].DataPtr );
}

void ETW_IOIoreqCompletion_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_IOIoreqCompletion_Trace(
        *(BYTE *)rgparg[0].DataPtr,
        *(QWORD *)rgparg[1].DataPtr,
        *(QWORD *)rgparg[2].DataPtr,
        *(DWORD *)rgparg[3].DataPtr,
        *(DWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr,
        *(QWORD *)rgparg[6].DataPtr,
        *(DWORD *)rgparg[7].DataPtr,
        *(DWORD *)rgparg[8].DataPtr,
        *(QWORD *)rgparg[9].DataPtr );
}

void ETW_CacheMemoryUsage_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheMemoryUsage_Trace(
        (PCWSTR)rgparg[0].DataPtr,
        *(BYTE *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(QWORD *)rgparg[3].DataPtr,
        *(QWORD *)rgparg[4].DataPtr,
        *(DWORD *)rgparg[5].DataPtr );
}

void ETW_CacheSetLgposModify_Trace( const MOF_FIELD * rgparg )
{
    EventWriteESE_CacheSetLgposModify_Trace(
        *(DWORD *)rgparg[0].DataPtr,
        *(DWORD *)rgparg[1].DataPtr,
        *(DWORD *)rgparg[2].DataPtr,
        *(QWORD *)rgparg[3].DataPtr );
}

#define ESE_ETW_AUTOGEN_PFN_ARRAY \
        { 0, &ETW_TraceBaseId_Trace }, \
        { 0, &ETW_BF_Trace }, \
        { 0, &ETW_Block_Trace }, \
        { 20, &ETW_CacheNewPage_Trace }, \
        { 20, &ETW_CacheReadPage_Trace }, \
        { 14, &ETW_CachePrereadPage_Trace }, \
        { 18, &ETW_CacheWritePage_Trace }, \
        { 7, &ETW_CacheEvictPage_Trace }, \
        { 10, &ETW_CacheRequestPage_Trace }, \
        { 0, &ETW_LatchPageDeprecated_Trace }, \
        { 19, &ETW_CacheDirtyPage_Trace }, \
        { 3, &ETW_TransactionBegin_Trace }, \
        { 3, &ETW_TransactionCommit_Trace }, \
        { 3, &ETW_TransactionRollback_Trace }, \
        { 6, &ETW_SpaceAllocExt_Trace }, \
        { 6, &ETW_SpaceFreeExt_Trace }, \
        { 5, &ETW_SpaceAllocPage_Trace }, \
        { 5, &ETW_SpaceFreePage_Trace }, \
        { 9, &ETW_IorunEnqueue_Trace }, \
        { 17, &ETW_IorunDequeue_Trace }, \
        { 28, &ETW_IOCompletion_Trace }, \
        { 0, &ETW_LogStall_Trace }, \
        { 3, &ETW_LogWrite_Trace }, \
        { 1, &ETW_EventLogInfo_Trace }, \
        { 1, &ETW_EventLogWarn_Trace }, \
        { 1, &ETW_EventLogError_Trace }, \
        { 0, &ETW_TimerQueueScheduleDeprecated_Trace }, \
        { 0, &ETW_TimerQueueRunDeprecated_Trace }, \
        { 0, &ETW_TimerQueueCancelDeprecated_Trace }, \
        { 6, &ETW_TimerTaskSchedule_Trace }, \
        { 5, &ETW_TimerTaskRun_Trace }, \
        { 3, &ETW_TimerTaskCancel_Trace }, \
        { 4, &ETW_TaskManagerPost_Trace }, \
        { 6, &ETW_TaskManagerRun_Trace }, \
        { 4, &ETW_GPTaskManagerPost_Trace }, \
        { 4, &ETW_GPTaskManagerRun_Trace }, \
        { 2, &ETW_TestMarker_Trace }, \
        { 3, &ETW_ThreadCreate_Trace }, \
        { 3, &ETW_ThreadStart_Trace }, \
        { 2, &ETW_CacheVersionPage_Trace }, \
        { 2, &ETW_CacheVersionCopyPage_Trace }, \
        { 4, &ETW_CacheResize_Trace }, \
        { 2, &ETW_CacheLimitResize_Trace }, \
        { 16, &ETW_CacheScavengeProgress_Trace }, \
        { 1, &ETW_ApiCall_Start_Trace }, \
        { 2, &ETW_ApiCall_Stop_Trace }, \
        { 8, &ETW_ResMgrInit_Trace }, \
        { 1, &ETW_ResMgrTerm_Trace }, \
        { 8, &ETW_CacheCachePage_Trace }, \
        { 3, &ETW_MarkPageAsSuperCold_Trace }, \
        { 17, &ETW_CacheMissLatency_Trace }, \
        { 15, &ETW_BTreePrereadPageRequest_Trace }, \
        { 6, &ETW_DiskFlushFileBuffers_Trace }, \
        { 3, &ETW_DiskFlushFileBuffersBegin_Trace }, \
        { 19, &ETW_CacheFirstDirtyPage_Trace }, \
        { 6, &ETW_SysStationId_Trace }, \
        { 5, &ETW_InstStationId_Trace }, \
        { 5, &ETW_FmpStationId_Trace }, \
        { 6, &ETW_DiskStationId_Trace }, \
        { 8, &ETW_FileStationId_Trace }, \
        { 8, &ETW_IsamDbfilehdrInfo_Trace }, \
        { 2, &ETW_DiskOsDiskCacheInfo_Trace }, \
        { 2, &ETW_DiskOsStorageWriteCacheProp_Trace }, \
        { 2, &ETW_DiskOsDeviceSeekPenaltyDesc_Trace }, \
        { 0, &ETW_DirtyPage2Deprecated_Trace }, \
        { 31, &ETW_IOCompletion2_Trace }, \
        { 4, &ETW_FCBPurgeFailure_Trace }, \
        { 2, &ETW_IOLatencySpikeNotice_Trace }, \
        { 31, &ETW_IOCompletion2Sess_Trace }, \
        { 2, &ETW_IOIssueThreadPost_Trace }, \
        { 3, &ETW_IOIssueThreadPosted_Trace }, \
        { 0, &ETW_IOThreadIssueStart_Trace }, \
        { 6, &ETW_IOThreadIssuedDisk_Trace }, \
        { 3, &ETW_IOThreadIssueProcessedIO_Trace }, \
        { 10, &ETW_IOIoreqCompletion_Trace }, \
        { 6, &ETW_CacheMemoryUsage_Trace }, \
        { 4, &ETW_CacheSetLgposModify_Trace }, \


