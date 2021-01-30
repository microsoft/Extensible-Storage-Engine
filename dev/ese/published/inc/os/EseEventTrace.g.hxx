// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "EseEventTraceType.g.hxx"

#ifdef DEBUG
inline void FakeUseTrace( void * pv )
{
}
#endif

INLINE void ETTraceBaseId(
 )
{

#ifdef DEBUG
    EseTraceBaseIdNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTraceBaseId );
}

INLINE void ETBF(
 )
{

#ifdef DEBUG
    EseBFNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidBF );
}

INLINE void ETBlock(
 )
{

#ifdef DEBUG
    EseBlockNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidBlock );
}

INLINE void ETCacheNewPage(
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    LatchFlags,
    DWORD    objid,
    DWORD    PageFlags,
    DWORD    UserId,
    BYTE    OperationId,
    BYTE    OperationType,
    BYTE    ClientType,
    BYTE    Flags,
    DWORD    CorrelationId,
    BYTE    Iorp,
    BYTE    Iors,
    BYTE    Iort,
    BYTE    Ioru,
    BYTE    Iorf,
    BYTE    ParentObjectClass,
    QWORD    dbtimeDirtied,
    USHORT    itagMicFree,
    USHORT    cbFree )
{

#ifdef DEBUG
    EseCacheNewPageNative ettT = {
        ifmp,
        pgno,
        LatchFlags,
        objid,
        PageFlags,
        UserId,
        OperationId,
        OperationType,
        ClientType,
        Flags,
        CorrelationId,
        Iorp,
        Iors,
        Iort,
        Ioru,
        Iorf,
        ParentObjectClass,
        dbtimeDirtied,
        itagMicFree,
        cbFree };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheNewPage,
        20,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&LatchFlags,
        (DWORD *)&objid,
        (DWORD *)&PageFlags,
        (DWORD *)&UserId,
        (BYTE *)&OperationId,
        (BYTE *)&OperationType,
        (BYTE *)&ClientType,
        (BYTE *)&Flags,
        (DWORD *)&CorrelationId,
        (BYTE *)&Iorp,
        (BYTE *)&Iors,
        (BYTE *)&Iort,
        (BYTE *)&Ioru,
        (BYTE *)&Iorf,
        (BYTE *)&ParentObjectClass,
        (QWORD *)&dbtimeDirtied,
        (USHORT *)&itagMicFree,
        (USHORT *)&cbFree );
}

INLINE void ETCacheReadPage(
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    LatchFlags,
    DWORD    objid,
    DWORD    PageFlags,
    DWORD    UserId,
    BYTE    OperationId,
    BYTE    OperationType,
    BYTE    ClientType,
    BYTE    Flags,
    DWORD    CorrelationId,
    BYTE    Iorp,
    BYTE    Iors,
    BYTE    Iort,
    BYTE    Ioru,
    BYTE    Iorf,
    BYTE    ParentObjectClass,
    QWORD    dbtimeDirtied,
    USHORT    itagMicFree,
    USHORT    cbFree )
{

#ifdef DEBUG
    EseCacheReadPageNative ettT = {
        ifmp,
        pgno,
        LatchFlags,
        objid,
        PageFlags,
        UserId,
        OperationId,
        OperationType,
        ClientType,
        Flags,
        CorrelationId,
        Iorp,
        Iors,
        Iort,
        Ioru,
        Iorf,
        ParentObjectClass,
        dbtimeDirtied,
        itagMicFree,
        cbFree };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheReadPage,
        20,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&LatchFlags,
        (DWORD *)&objid,
        (DWORD *)&PageFlags,
        (DWORD *)&UserId,
        (BYTE *)&OperationId,
        (BYTE *)&OperationType,
        (BYTE *)&ClientType,
        (BYTE *)&Flags,
        (DWORD *)&CorrelationId,
        (BYTE *)&Iorp,
        (BYTE *)&Iors,
        (BYTE *)&Iort,
        (BYTE *)&Ioru,
        (BYTE *)&Iorf,
        (BYTE *)&ParentObjectClass,
        (QWORD *)&dbtimeDirtied,
        (USHORT *)&itagMicFree,
        (USHORT *)&cbFree );
}

INLINE void ETCachePrereadPage(
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    UserId,
    BYTE    OperationId,
    BYTE    OperationType,
    BYTE    ClientType,
    BYTE    Flags,
    DWORD    CorrelationId,
    BYTE    Iorp,
    BYTE    Iors,
    BYTE    Iort,
    BYTE    Ioru,
    BYTE    Iorf,
    BYTE    ParentObjectClass )
{

#ifdef DEBUG
    EseCachePrereadPageNative ettT = {
        ifmp,
        pgno,
        UserId,
        OperationId,
        OperationType,
        ClientType,
        Flags,
        CorrelationId,
        Iorp,
        Iors,
        Iort,
        Ioru,
        Iorf,
        ParentObjectClass };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCachePrereadPage,
        14,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&UserId,
        (BYTE *)&OperationId,
        (BYTE *)&OperationType,
        (BYTE *)&ClientType,
        (BYTE *)&Flags,
        (DWORD *)&CorrelationId,
        (BYTE *)&Iorp,
        (BYTE *)&Iors,
        (BYTE *)&Iort,
        (BYTE *)&Ioru,
        (BYTE *)&Iorf,
        (BYTE *)&ParentObjectClass );
}

INLINE void ETCacheWritePage(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    objid,
    DWORD    PageFlags,
    DWORD    DirtyLevel,
    DWORD    UserId,
    BYTE    OperationId,
    BYTE    OperationType,
    BYTE    ClientType,
    BYTE    Flags,
    DWORD    CorrelationId,
    BYTE    Iorp,
    BYTE    Iors,
    BYTE    Iort,
    BYTE    Ioru,
    BYTE    Iorf,
    BYTE    ParentObjectClass )
{

#ifdef DEBUG
    EseCacheWritePageNative ettT = {
        tick,
        ifmp,
        pgno,
        objid,
        PageFlags,
        DirtyLevel,
        UserId,
        OperationId,
        OperationType,
        ClientType,
        Flags,
        CorrelationId,
        Iorp,
        Iors,
        Iort,
        Ioru,
        Iorf,
        ParentObjectClass };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheWritePage,
        18,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&objid,
        (DWORD *)&PageFlags,
        (DWORD *)&DirtyLevel,
        (DWORD *)&UserId,
        (BYTE *)&OperationId,
        (BYTE *)&OperationType,
        (BYTE *)&ClientType,
        (BYTE *)&Flags,
        (DWORD *)&CorrelationId,
        (BYTE *)&Iorp,
        (BYTE *)&Iors,
        (BYTE *)&Iort,
        (BYTE *)&Ioru,
        (BYTE *)&Iorf,
        (BYTE *)&ParentObjectClass );
}

INLINE void ETCacheEvictPage(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    fCurrentVersion,
    INT    errBF,
    DWORD    bfef,
    DWORD    pctPriority )
{

#ifdef DEBUG
    EseCacheEvictPageNative ettT = {
        tick,
        ifmp,
        pgno,
        fCurrentVersion,
        errBF,
        bfef,
        pctPriority };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheEvictPage,
        7,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&fCurrentVersion,
        (INT *)&errBF,
        (DWORD *)&bfef,
        (DWORD *)&pctPriority );
}

INLINE void ETCacheRequestPage(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    bflf,
    DWORD    objid,
    DWORD    PageFlags,
    DWORD    bflt,
    DWORD    pctPriority,
    DWORD    bfrtf,
    BYTE    ClientType )
{

#ifdef DEBUG
    EseCacheRequestPageNative ettT = {
        tick,
        ifmp,
        pgno,
        bflf,
        objid,
        PageFlags,
        bflt,
        pctPriority,
        bfrtf,
        ClientType };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheRequestPage,
        10,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&bflf,
        (DWORD *)&objid,
        (DWORD *)&PageFlags,
        (DWORD *)&bflt,
        (DWORD *)&pctPriority,
        (DWORD *)&bfrtf,
        (BYTE *)&ClientType );
}

INLINE void ETLatchPageDeprecated(
 )
{

#ifdef DEBUG
    EseLatchPageDeprecatedNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidLatchPageDeprecated );
}

INLINE void ETCacheDirtyPage(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    objid,
    DWORD    PageFlags,
    DWORD    DirtyLevel,
    QWORD    LgposModify,
    DWORD    UserId,
    BYTE    OperationId,
    BYTE    OperationType,
    BYTE    ClientType,
    BYTE    Flags,
    DWORD    CorrelationId,
    BYTE    Iorp,
    BYTE    Iors,
    BYTE    Iort,
    BYTE    Ioru,
    BYTE    Iorf,
    BYTE    ParentObjectClass )
{

#ifdef DEBUG
    EseCacheDirtyPageNative ettT = {
        tick,
        ifmp,
        pgno,
        objid,
        PageFlags,
        DirtyLevel,
        LgposModify,
        UserId,
        OperationId,
        OperationType,
        ClientType,
        Flags,
        CorrelationId,
        Iorp,
        Iors,
        Iort,
        Ioru,
        Iorf,
        ParentObjectClass };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheDirtyPage,
        19,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&objid,
        (DWORD *)&PageFlags,
        (DWORD *)&DirtyLevel,
        (QWORD *)&LgposModify,
        (DWORD *)&UserId,
        (BYTE *)&OperationId,
        (BYTE *)&OperationType,
        (BYTE *)&ClientType,
        (BYTE *)&Flags,
        (DWORD *)&CorrelationId,
        (BYTE *)&Iorp,
        (BYTE *)&Iors,
        (BYTE *)&Iort,
        (BYTE *)&Ioru,
        (BYTE *)&Iorf,
        (BYTE *)&ParentObjectClass );
}

INLINE void ETTransactionBegin(
    const void *    SessionNumber,
    const void *    TransactionNumber,
    BYTE    TransactionLevel )
{

#ifdef DEBUG
    EseTransactionBeginNative ettT = {
        SessionNumber,
        TransactionNumber,
        TransactionLevel };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTransactionBegin,
        3,
        (const void * *)&SessionNumber,
        (const void * *)&TransactionNumber,
        (BYTE *)&TransactionLevel );
}

INLINE void ETTransactionCommit(
    const void *    SessionNumber,
    const void *    TransactionNumber,
    BYTE    TransactionLevel )
{

#ifdef DEBUG
    EseTransactionCommitNative ettT = {
        SessionNumber,
        TransactionNumber,
        TransactionLevel };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTransactionCommit,
        3,
        (const void * *)&SessionNumber,
        (const void * *)&TransactionNumber,
        (BYTE *)&TransactionLevel );
}

INLINE void ETTransactionRollback(
    const void *    SessionNumber,
    const void *    TransactionNumber,
    BYTE    TransactionLevel )
{

#ifdef DEBUG
    EseTransactionRollbackNative ettT = {
        SessionNumber,
        TransactionNumber,
        TransactionLevel };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTransactionRollback,
        3,
        (const void * *)&SessionNumber,
        (const void * *)&TransactionNumber,
        (BYTE *)&TransactionLevel );
}

INLINE void ETSpaceAllocExt(
    DWORD    ifmp,
    DWORD    pgnoFDP,
    DWORD    pgnoFirst,
    DWORD    cpg,
    DWORD    objidFDP,
    BYTE    tce )
{

#ifdef DEBUG
    EseSpaceAllocExtNative ettT = {
        ifmp,
        pgnoFDP,
        pgnoFirst,
        cpg,
        objidFDP,
        tce };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidSpaceAllocExt,
        6,
        (DWORD *)&ifmp,
        (DWORD *)&pgnoFDP,
        (DWORD *)&pgnoFirst,
        (DWORD *)&cpg,
        (DWORD *)&objidFDP,
        (BYTE *)&tce );
}

INLINE void ETSpaceFreeExt(
    DWORD    ifmp,
    DWORD    pgnoFDP,
    DWORD    pgnoFirst,
    DWORD    cpg,
    DWORD    objidFDP,
    BYTE    tce )
{

#ifdef DEBUG
    EseSpaceFreeExtNative ettT = {
        ifmp,
        pgnoFDP,
        pgnoFirst,
        cpg,
        objidFDP,
        tce };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidSpaceFreeExt,
        6,
        (DWORD *)&ifmp,
        (DWORD *)&pgnoFDP,
        (DWORD *)&pgnoFirst,
        (DWORD *)&cpg,
        (DWORD *)&objidFDP,
        (BYTE *)&tce );
}

INLINE void ETSpaceAllocPage(
    DWORD    ifmp,
    DWORD    pgnoFDP,
    DWORD    pgnoAlloc,
    DWORD    objidFDP,
    BYTE    tce )
{

#ifdef DEBUG
    EseSpaceAllocPageNative ettT = {
        ifmp,
        pgnoFDP,
        pgnoAlloc,
        objidFDP,
        tce };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidSpaceAllocPage,
        5,
        (DWORD *)&ifmp,
        (DWORD *)&pgnoFDP,
        (DWORD *)&pgnoAlloc,
        (DWORD *)&objidFDP,
        (BYTE *)&tce );
}

INLINE void ETSpaceFreePage(
    DWORD    ifmp,
    DWORD    pgnoFDP,
    DWORD    pgnoFree,
    DWORD    objidFDP,
    BYTE    tce )
{

#ifdef DEBUG
    EseSpaceFreePageNative ettT = {
        ifmp,
        pgnoFDP,
        pgnoFree,
        objidFDP,
        tce };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidSpaceFreePage,
        5,
        (DWORD *)&ifmp,
        (DWORD *)&pgnoFDP,
        (DWORD *)&pgnoFree,
        (DWORD *)&objidFDP,
        (BYTE *)&tce );
}

INLINE void ETIorunEnqueue(
    QWORD    iFile,
    QWORD    ibOffset,
    DWORD    cbData,
    DWORD    tidAlloc,
    DWORD    fHeapA,
    DWORD    fWrite,
    DWORD    EngineFileType,
    QWORD    EngineFileId,
    DWORD    cusecEnqueueLatency )
{

#ifdef DEBUG
    EseIorunEnqueueNative ettT = {
        iFile,
        ibOffset,
        cbData,
        tidAlloc,
        fHeapA,
        fWrite,
        EngineFileType,
        EngineFileId,
        cusecEnqueueLatency };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIorunEnqueue,
        9,
        (QWORD *)&iFile,
        (QWORD *)&ibOffset,
        (DWORD *)&cbData,
        (DWORD *)&tidAlloc,
        (DWORD *)&fHeapA,
        (DWORD *)&fWrite,
        (DWORD *)&EngineFileType,
        (QWORD *)&EngineFileId,
        (DWORD *)&cusecEnqueueLatency );
}

INLINE void ETIorunDequeue(
    QWORD    iFile,
    QWORD    ibOffset,
    DWORD    cbData,
    DWORD    tidAlloc,
    DWORD    fHeapA,
    DWORD    fWrite,
    DWORD    Iorp,
    DWORD    Iors,
    DWORD    Ioru,
    DWORD    Iorf,
    DWORD    grbitQos,
    QWORD    cmsecTimeInQueue,
    DWORD    EngineFileType,
    QWORD    EngineFileId,
    QWORD    cDispatchPass,
    USHORT    cIorunCombined,
    DWORD    cusecDequeueLatency )
{

#ifdef DEBUG
    EseIorunDequeueNative ettT = {
        iFile,
        ibOffset,
        cbData,
        tidAlloc,
        fHeapA,
        fWrite,
        Iorp,
        Iors,
        Ioru,
        Iorf,
        grbitQos,
        cmsecTimeInQueue,
        EngineFileType,
        EngineFileId,
        cDispatchPass,
        cIorunCombined,
        cusecDequeueLatency };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIorunDequeue,
        17,
        (QWORD *)&iFile,
        (QWORD *)&ibOffset,
        (DWORD *)&cbData,
        (DWORD *)&tidAlloc,
        (DWORD *)&fHeapA,
        (DWORD *)&fWrite,
        (DWORD *)&Iorp,
        (DWORD *)&Iors,
        (DWORD *)&Ioru,
        (DWORD *)&Iorf,
        (DWORD *)&grbitQos,
        (QWORD *)&cmsecTimeInQueue,
        (DWORD *)&EngineFileType,
        (QWORD *)&EngineFileId,
        (QWORD *)&cDispatchPass,
        (USHORT *)&cIorunCombined,
        (DWORD *)&cusecDequeueLatency );
}

INLINE void ETIOCompletion(
    QWORD    iFile,
    DWORD    fMultiIor,
    BYTE    fWrite,
    DWORD    UserId,
    BYTE    OperationId,
    BYTE    OperationType,
    BYTE    ClientType,
    BYTE    Flags,
    DWORD    CorrelationId,
    BYTE    Iorp,
    BYTE    Iors,
    BYTE    Iort,
    BYTE    Ioru,
    BYTE    Iorf,
    BYTE    ParentObjectClass,
    QWORD    ibOffset,
    DWORD    cbTransfer,
    DWORD    error,
    DWORD    qosHighestFirst,
    QWORD    cmsecIOElapsed,
    DWORD    dtickQueueDelay,
    DWORD    tidAlloc,
    DWORD    EngineFileType,
    QWORD    EngineFileId,
    DWORD    fmfFile,
    DWORD    DiskNumber,
    DWORD    dwEngineObjid,
    QWORD    qosIOComplete )
{

#ifdef DEBUG
    EseIOCompletionNative ettT = {
        iFile,
        fMultiIor,
        fWrite,
        UserId,
        OperationId,
        OperationType,
        ClientType,
        Flags,
        CorrelationId,
        Iorp,
        Iors,
        Iort,
        Ioru,
        Iorf,
        ParentObjectClass,
        ibOffset,
        cbTransfer,
        error,
        qosHighestFirst,
        cmsecIOElapsed,
        dtickQueueDelay,
        tidAlloc,
        EngineFileType,
        EngineFileId,
        fmfFile,
        DiskNumber,
        dwEngineObjid,
        qosIOComplete };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOCompletion,
        28,
        (QWORD *)&iFile,
        (DWORD *)&fMultiIor,
        (BYTE *)&fWrite,
        (DWORD *)&UserId,
        (BYTE *)&OperationId,
        (BYTE *)&OperationType,
        (BYTE *)&ClientType,
        (BYTE *)&Flags,
        (DWORD *)&CorrelationId,
        (BYTE *)&Iorp,
        (BYTE *)&Iors,
        (BYTE *)&Iort,
        (BYTE *)&Ioru,
        (BYTE *)&Iorf,
        (BYTE *)&ParentObjectClass,
        (QWORD *)&ibOffset,
        (DWORD *)&cbTransfer,
        (DWORD *)&error,
        (DWORD *)&qosHighestFirst,
        (QWORD *)&cmsecIOElapsed,
        (DWORD *)&dtickQueueDelay,
        (DWORD *)&tidAlloc,
        (DWORD *)&EngineFileType,
        (QWORD *)&EngineFileId,
        (DWORD *)&fmfFile,
        (DWORD *)&DiskNumber,
        (DWORD *)&dwEngineObjid,
        (QWORD *)&qosIOComplete );
}

INLINE void ETLogStall(
 )
{

#ifdef DEBUG
    EseLogStallNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidLogStall );
}

INLINE void ETLogWrite(
    INT    lgenData,
    DWORD    ibLogData,
    DWORD    cbLogData )
{

#ifdef DEBUG
    EseLogWriteNative ettT = {
        lgenData,
        ibLogData,
        cbLogData };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidLogWrite,
        3,
        (INT *)&lgenData,
        (DWORD *)&ibLogData,
        (DWORD *)&cbLogData );
}

INLINE void ETEventLogInfo(
    PCWSTR    szTrace )
{

#ifdef DEBUG
    EseEventLogInfoNative ettT = {
        szTrace };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidEventLogInfo,
        1,
        szTrace );
}

INLINE void ETEventLogWarn(
    PCWSTR    szTrace )
{

#ifdef DEBUG
    EseEventLogWarnNative ettT = {
        szTrace };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidEventLogWarn,
        1,
        szTrace );
}

INLINE void ETEventLogError(
    PCWSTR    szTrace )
{

#ifdef DEBUG
    EseEventLogErrorNative ettT = {
        szTrace };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidEventLogError,
        1,
        szTrace );
}

INLINE void ETTimerQueueScheduleDeprecated(
 )
{

#ifdef DEBUG
    EseTimerQueueScheduleDeprecatedNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTimerQueueScheduleDeprecated );
}

INLINE void ETTimerQueueRunDeprecated(
 )
{

#ifdef DEBUG
    EseTimerQueueRunDeprecatedNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTimerQueueRunDeprecated );
}

INLINE void ETTimerQueueCancelDeprecated(
 )
{

#ifdef DEBUG
    EseTimerQueueCancelDeprecatedNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTimerQueueCancelDeprecated );
}

INLINE void ETTimerTaskSchedule(
    const void *    posttTimerHandle,
    const void *    pfnTask,
    const void *    pvTaskGroupContext,
    const void *    pvRuntimeContext,
    DWORD    dtickMinDelay,
    DWORD    dtickSlopDelay )
{

#ifdef DEBUG
    EseTimerTaskScheduleNative ettT = {
        posttTimerHandle,
        pfnTask,
        pvTaskGroupContext,
        pvRuntimeContext,
        dtickMinDelay,
        dtickSlopDelay };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTimerTaskSchedule,
        6,
        (const void * *)&posttTimerHandle,
        (const void * *)&pfnTask,
        (const void * *)&pvTaskGroupContext,
        (const void * *)&pvRuntimeContext,
        (DWORD *)&dtickMinDelay,
        (DWORD *)&dtickSlopDelay );
}

INLINE void ETTimerTaskRun(
    const void *    posttTimerHandle,
    const void *    pfnTask,
    const void *    pvTaskGroupContext,
    const void *    pvRuntimeContext,
    QWORD    cRuns )
{

#ifdef DEBUG
    EseTimerTaskRunNative ettT = {
        posttTimerHandle,
        pfnTask,
        pvTaskGroupContext,
        pvRuntimeContext,
        cRuns };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTimerTaskRun,
        5,
        (const void * *)&posttTimerHandle,
        (const void * *)&pfnTask,
        (const void * *)&pvTaskGroupContext,
        (const void * *)&pvRuntimeContext,
        (QWORD *)&cRuns );
}

INLINE void ETTimerTaskCancel(
    const void *    posttTimerHandle,
    const void *    pfnTask,
    const void *    pvTaskGroupContext )
{

#ifdef DEBUG
    EseTimerTaskCancelNative ettT = {
        posttTimerHandle,
        pfnTask,
        pvTaskGroupContext };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTimerTaskCancel,
        3,
        (const void * *)&posttTimerHandle,
        (const void * *)&pfnTask,
        (const void * *)&pvTaskGroupContext );
}

INLINE void ETTaskManagerPost(
    const void *    ptm,
    const void *    pfnCompletion,
    DWORD    dwCompletionKey1,
    const void *    dwCompletionKey2 )
{

#ifdef DEBUG
    EseTaskManagerPostNative ettT = {
        ptm,
        pfnCompletion,
        dwCompletionKey1,
        dwCompletionKey2 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTaskManagerPost,
        4,
        (const void * *)&ptm,
        (const void * *)&pfnCompletion,
        (DWORD *)&dwCompletionKey1,
        (const void * *)&dwCompletionKey2 );
}

INLINE void ETTaskManagerRun(
    const void *    ptm,
    const void *    pfnCompletion,
    DWORD    dwCompletionKey1,
    const void *    dwCompletionKey2,
    DWORD    gle,
    const void *    dwThreadContext )
{

#ifdef DEBUG
    EseTaskManagerRunNative ettT = {
        ptm,
        pfnCompletion,
        dwCompletionKey1,
        dwCompletionKey2,
        gle,
        dwThreadContext };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTaskManagerRun,
        6,
        (const void * *)&ptm,
        (const void * *)&pfnCompletion,
        (DWORD *)&dwCompletionKey1,
        (const void * *)&dwCompletionKey2,
        (DWORD *)&gle,
        (const void * *)&dwThreadContext );
}

INLINE void ETGPTaskManagerPost(
    const void *    pgptm,
    const void *    pfnCompletion,
    const void *    pvParam,
    const void *    pTaskInfo )
{

#ifdef DEBUG
    EseGPTaskManagerPostNative ettT = {
        pgptm,
        pfnCompletion,
        pvParam,
        pTaskInfo };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidGPTaskManagerPost,
        4,
        (const void * *)&pgptm,
        (const void * *)&pfnCompletion,
        (const void * *)&pvParam,
        (const void * *)&pTaskInfo );
}

INLINE void ETGPTaskManagerRun(
    const void *    pgptm,
    const void *    pfnCompletion,
    const void *    pvParam,
    const void *    pTaskInfo )
{

#ifdef DEBUG
    EseGPTaskManagerRunNative ettT = {
        pgptm,
        pfnCompletion,
        pvParam,
        pTaskInfo };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidGPTaskManagerRun,
        4,
        (const void * *)&pgptm,
        (const void * *)&pfnCompletion,
        (const void * *)&pvParam,
        (const void * *)&pTaskInfo );
}

INLINE void ETTestMarker(
    QWORD    qwMarkerID,
    PCSTR    szAnnotation )
{

#ifdef DEBUG
    EseTestMarkerNative ettT = {
        qwMarkerID,
        szAnnotation };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidTestMarker,
        2,
        (QWORD *)&qwMarkerID,
        szAnnotation );
}

INLINE void ETThreadCreate(
    const void *    Thread,
    const void *    pfnStart,
    const void *    dwParam )
{

#ifdef DEBUG
    EseThreadCreateNative ettT = {
        Thread,
        pfnStart,
        dwParam };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidThreadCreate,
        3,
        (const void * *)&Thread,
        (const void * *)&pfnStart,
        (const void * *)&dwParam );
}

INLINE void ETThreadStart(
    const void *    Thread,
    const void *    pfnStart,
    const void *    dwParam )
{

#ifdef DEBUG
    EseThreadStartNative ettT = {
        Thread,
        pfnStart,
        dwParam };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidThreadStart,
        3,
        (const void * *)&Thread,
        (const void * *)&pfnStart,
        (const void * *)&dwParam );
}

INLINE void ETCacheVersionPage(
    DWORD    ifmp,
    DWORD    pgno )
{

#ifdef DEBUG
    EseCacheVersionPageNative ettT = {
        ifmp,
        pgno };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheVersionPage,
        2,
        (DWORD *)&ifmp,
        (DWORD *)&pgno );
}

INLINE void ETCacheVersionCopyPage(
    DWORD    ifmp,
    DWORD    pgno )
{

#ifdef DEBUG
    EseCacheVersionCopyPageNative ettT = {
        ifmp,
        pgno };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheVersionCopyPage,
        2,
        (DWORD *)&ifmp,
        (DWORD *)&pgno );
}

INLINE void ETCacheResize(
    __int64    cbfCacheAddressableInitial,
    __int64    cbfCacheSizeInitial,
    __int64    cbfCacheAddressableFinal,
    __int64    cbfCacheSizeFinal )
{

#ifdef DEBUG
    EseCacheResizeNative ettT = {
        cbfCacheAddressableInitial,
        cbfCacheSizeInitial,
        cbfCacheAddressableFinal,
        cbfCacheSizeFinal };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheResize,
        4,
        (__int64 *)&cbfCacheAddressableInitial,
        (__int64 *)&cbfCacheSizeInitial,
        (__int64 *)&cbfCacheAddressableFinal,
        (__int64 *)&cbfCacheSizeFinal );
}

INLINE void ETCacheLimitResize(
    __int64    cbfCacheSizeLimitInitial,
    __int64    cbfCacheSizeLimitFinal )
{

#ifdef DEBUG
    EseCacheLimitResizeNative ettT = {
        cbfCacheSizeLimitInitial,
        cbfCacheSizeLimitFinal };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheLimitResize,
        2,
        (__int64 *)&cbfCacheSizeLimitInitial,
        (__int64 *)&cbfCacheSizeLimitFinal );
}

INLINE void ETCacheScavengeProgress(
    __int64    iRun,
    INT    cbfVisited,
    INT    cbfCacheSize,
    INT    cbfCacheTarget,
    INT    cbfCacheSizeStartShrink,
    DWORD    dtickShrinkDuration,
    INT    cbfAvail,
    INT    cbfAvailPoolLow,
    INT    cbfAvailPoolHigh,
    INT    cbfFlushPending,
    INT    cbfFlushPendingSlow,
    INT    cbfFlushPendingHung,
    INT    cbfOutOfMemory,
    INT    cbfPermanentErrs,
    INT    eStopReason,
    INT    errRun )
{

#ifdef DEBUG
    EseCacheScavengeProgressNative ettT = {
        iRun,
        cbfVisited,
        cbfCacheSize,
        cbfCacheTarget,
        cbfCacheSizeStartShrink,
        dtickShrinkDuration,
        cbfAvail,
        cbfAvailPoolLow,
        cbfAvailPoolHigh,
        cbfFlushPending,
        cbfFlushPendingSlow,
        cbfFlushPendingHung,
        cbfOutOfMemory,
        cbfPermanentErrs,
        eStopReason,
        errRun };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheScavengeProgress,
        16,
        (__int64 *)&iRun,
        (INT *)&cbfVisited,
        (INT *)&cbfCacheSize,
        (INT *)&cbfCacheTarget,
        (INT *)&cbfCacheSizeStartShrink,
        (DWORD *)&dtickShrinkDuration,
        (INT *)&cbfAvail,
        (INT *)&cbfAvailPoolLow,
        (INT *)&cbfAvailPoolHigh,
        (INT *)&cbfFlushPending,
        (INT *)&cbfFlushPendingSlow,
        (INT *)&cbfFlushPendingHung,
        (INT *)&cbfOutOfMemory,
        (INT *)&cbfPermanentErrs,
        (INT *)&eStopReason,
        (INT *)&errRun );
}

INLINE void ETApiCall_Start(
    DWORD    opApi )
{

#ifdef DEBUG
    EseApiCall_StartNative ettT = {
        opApi };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidApiCall_Start,
        1,
        (DWORD *)&opApi );
}

INLINE void ETApiCall_Stop(
    DWORD    opApi,
    INT    err )
{

#ifdef DEBUG
    EseApiCall_StopNative ettT = {
        opApi,
        err };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidApiCall_Stop,
        2,
        (DWORD *)&opApi,
        (INT *)&err );
}

INLINE void ETResMgrInit(
    DWORD    tick,
    INT    K,
    double    csecCorrelatedTouch,
    double    csecTimeout,
    double    csecUncertainty,
    double    dblHashLoadFactor,
    double    dblHashUniformity,
    double    dblSpeedSizeTradeoff )
{

#ifdef DEBUG
    EseResMgrInitNative ettT = {
        tick,
        K,
        csecCorrelatedTouch,
        csecTimeout,
        csecUncertainty,
        dblHashLoadFactor,
        dblHashUniformity,
        dblSpeedSizeTradeoff };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidResMgrInit,
        8,
        (DWORD *)&tick,
        (INT *)&K,
        (double *)&csecCorrelatedTouch,
        (double *)&csecTimeout,
        (double *)&csecUncertainty,
        (double *)&dblHashLoadFactor,
        (double *)&dblHashUniformity,
        (double *)&dblSpeedSizeTradeoff );
}

INLINE void ETResMgrTerm(
    DWORD    tick )
{

#ifdef DEBUG
    EseResMgrTermNative ettT = {
        tick };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidResMgrTerm,
        1,
        (DWORD *)&tick );
}

INLINE void ETCacheCachePage(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    bflf,
    DWORD    bflt,
    DWORD    pctPriority,
    DWORD    bfrtf,
    BYTE    ClientType )
{

#ifdef DEBUG
    EseCacheCachePageNative ettT = {
        tick,
        ifmp,
        pgno,
        bflf,
        bflt,
        pctPriority,
        bfrtf,
        ClientType };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheCachePage,
        8,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&bflf,
        (DWORD *)&bflt,
        (DWORD *)&pctPriority,
        (DWORD *)&bfrtf,
        (BYTE *)&ClientType );
}

INLINE void ETMarkPageAsSuperCold(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno )
{

#ifdef DEBUG
    EseMarkPageAsSuperColdNative ettT = {
        tick,
        ifmp,
        pgno };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidMarkPageAsSuperCold,
        3,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno );
}

INLINE void ETCacheMissLatency(
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    dwUserId,
    BYTE    bOperationId,
    BYTE    bOperationType,
    BYTE    bClientType,
    BYTE    bFlags,
    DWORD    dwCorrelationId,
    BYTE    iorp,
    BYTE    iors,
    BYTE    iort,
    BYTE    ioru,
    BYTE    iorf,
    BYTE    tce,
    QWORD    usecsWait,
    BYTE    bftcmr,
    BYTE    bUserPriorityTag )
{

#ifdef DEBUG
    EseCacheMissLatencyNative ettT = {
        ifmp,
        pgno,
        dwUserId,
        bOperationId,
        bOperationType,
        bClientType,
        bFlags,
        dwCorrelationId,
        iorp,
        iors,
        iort,
        ioru,
        iorf,
        tce,
        usecsWait,
        bftcmr,
        bUserPriorityTag };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheMissLatency,
        17,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&dwUserId,
        (BYTE *)&bOperationId,
        (BYTE *)&bOperationType,
        (BYTE *)&bClientType,
        (BYTE *)&bFlags,
        (DWORD *)&dwCorrelationId,
        (BYTE *)&iorp,
        (BYTE *)&iors,
        (BYTE *)&iort,
        (BYTE *)&ioru,
        (BYTE *)&iorf,
        (BYTE *)&tce,
        (QWORD *)&usecsWait,
        (BYTE *)&bftcmr,
        (BYTE *)&bUserPriorityTag );
}

INLINE void ETBTreePrereadPageRequest(
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    dwUserId,
    BYTE    bOperationId,
    BYTE    bOperationType,
    BYTE    bClientType,
    BYTE    bFlags,
    DWORD    dwCorrelationId,
    BYTE    iorp,
    BYTE    iors,
    BYTE    iort,
    BYTE    ioru,
    BYTE    iorf,
    BYTE    tce,
    BYTE    fOpFlags )
{

#ifdef DEBUG
    EseBTreePrereadPageRequestNative ettT = {
        ifmp,
        pgno,
        dwUserId,
        bOperationId,
        bOperationType,
        bClientType,
        bFlags,
        dwCorrelationId,
        iorp,
        iors,
        iort,
        ioru,
        iorf,
        tce,
        fOpFlags };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidBTreePrereadPageRequest,
        15,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&dwUserId,
        (BYTE *)&bOperationId,
        (BYTE *)&bOperationType,
        (BYTE *)&bClientType,
        (BYTE *)&bFlags,
        (DWORD *)&dwCorrelationId,
        (BYTE *)&iorp,
        (BYTE *)&iors,
        (BYTE *)&iort,
        (BYTE *)&ioru,
        (BYTE *)&iorf,
        (BYTE *)&tce,
        (BYTE *)&fOpFlags );
}

INLINE void ETDiskFlushFileBuffers(
    DWORD    Disk,
    PCWSTR    wszFileName,
    DWORD    iofr,
    QWORD    cioreqFileFlushing,
    QWORD    usFfb,
    DWORD    error )
{

#ifdef DEBUG
    EseDiskFlushFileBuffersNative ettT = {
        Disk,
        wszFileName,
        iofr,
        cioreqFileFlushing,
        usFfb,
        error };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDiskFlushFileBuffers,
        6,
        (DWORD *)&Disk,
        wszFileName,
        (DWORD *)&iofr,
        (QWORD *)&cioreqFileFlushing,
        (QWORD *)&usFfb,
        (DWORD *)&error );
}

INLINE void ETDiskFlushFileBuffersBegin(
    DWORD    dwDisk,
    QWORD    hFile,
    DWORD    iofr )
{

#ifdef DEBUG
    EseDiskFlushFileBuffersBeginNative ettT = {
        dwDisk,
        hFile,
        iofr };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDiskFlushFileBuffersBegin,
        3,
        (DWORD *)&dwDisk,
        (QWORD *)&hFile,
        (DWORD *)&iofr );
}

INLINE void ETCacheFirstDirtyPage(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    DWORD    objid,
    DWORD    fFlags,
    DWORD    bfdfNew,
    QWORD    lgposModify,
    DWORD    dwUserId,
    BYTE    bOperationId,
    BYTE    bOperationType,
    BYTE    bClientType,
    BYTE    bFlags,
    DWORD    dwCorrelationId,
    BYTE    iorp,
    BYTE    iors,
    BYTE    iort,
    BYTE    ioru,
    BYTE    iorf,
    BYTE    tce )
{

#ifdef DEBUG
    EseCacheFirstDirtyPageNative ettT = {
        tick,
        ifmp,
        pgno,
        objid,
        fFlags,
        bfdfNew,
        lgposModify,
        dwUserId,
        bOperationId,
        bOperationType,
        bClientType,
        bFlags,
        dwCorrelationId,
        iorp,
        iors,
        iort,
        ioru,
        iorf,
        tce };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheFirstDirtyPage,
        19,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (DWORD *)&objid,
        (DWORD *)&fFlags,
        (DWORD *)&bfdfNew,
        (QWORD *)&lgposModify,
        (DWORD *)&dwUserId,
        (BYTE *)&bOperationId,
        (BYTE *)&bOperationType,
        (BYTE *)&bClientType,
        (BYTE *)&bFlags,
        (DWORD *)&dwCorrelationId,
        (BYTE *)&iorp,
        (BYTE *)&iors,
        (BYTE *)&iort,
        (BYTE *)&ioru,
        (BYTE *)&iorf,
        (BYTE *)&tce );
}

INLINE void ETSysStationId(
    BYTE    tsidr,
    DWORD    dwImageVerMajor,
    DWORD    dwImageVerMinor,
    DWORD    dwImageBuildMajor,
    DWORD    dwImageBuildMinor,
    PCWSTR    wszDisplayName )
{

#ifdef DEBUG
    EseSysStationIdNative ettT = {
        tsidr,
        dwImageVerMajor,
        dwImageVerMinor,
        dwImageBuildMajor,
        dwImageBuildMinor,
        wszDisplayName };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidSysStationId,
        6,
        (BYTE *)&tsidr,
        (DWORD *)&dwImageVerMajor,
        (DWORD *)&dwImageVerMinor,
        (DWORD *)&dwImageBuildMajor,
        (DWORD *)&dwImageBuildMinor,
        wszDisplayName );
}

INLINE void ETInstStationId(
    BYTE    tsidr,
    DWORD    iInstance,
    BYTE    perfstatusEvent,
    PCWSTR    wszInstanceName,
    PCWSTR    wszDisplayName )
{

#ifdef DEBUG
    EseInstStationIdNative ettT = {
        tsidr,
        iInstance,
        perfstatusEvent,
        wszInstanceName,
        wszDisplayName };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidInstStationId,
        5,
        (BYTE *)&tsidr,
        (DWORD *)&iInstance,
        (BYTE *)&perfstatusEvent,
        wszInstanceName,
        wszDisplayName );
}

INLINE void ETFmpStationId(
    BYTE    tsidr,
    DWORD    ifmp,
    DWORD    iInstance,
    BYTE    dbid,
    PCWSTR    wszDatabaseName )
{

#ifdef DEBUG
    EseFmpStationIdNative ettT = {
        tsidr,
        ifmp,
        iInstance,
        dbid,
        wszDatabaseName };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidFmpStationId,
        5,
        (BYTE *)&tsidr,
        (DWORD *)&ifmp,
        (DWORD *)&iInstance,
        (BYTE *)&dbid,
        wszDatabaseName );
}

INLINE void ETDiskStationId(
    BYTE    tsidr,
    DWORD    dwDiskNumber,
    PCWSTR    wszDiskPathId,
    PCSTR    szDiskModel,
    PCSTR    szDiskFirmwareRev,
    PCSTR    szDiskSerialNumber )
{

#ifdef DEBUG
    EseDiskStationIdNative ettT = {
        tsidr,
        dwDiskNumber,
        wszDiskPathId,
        szDiskModel,
        szDiskFirmwareRev,
        szDiskSerialNumber };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDiskStationId,
        6,
        (BYTE *)&tsidr,
        (DWORD *)&dwDiskNumber,
        wszDiskPathId,
        szDiskModel,
        szDiskFirmwareRev,
        szDiskSerialNumber );
}

INLINE void ETFileStationId(
    BYTE    tsidr,
    QWORD    hFile,
    DWORD    dwDiskNumber,
    DWORD    dwEngineFileType,
    QWORD    qwEngineFileId,
    DWORD    fmf,
    QWORD    cbFileSize,
    PCWSTR    wszAbsPath )
{

#ifdef DEBUG
    EseFileStationIdNative ettT = {
        tsidr,
        hFile,
        dwDiskNumber,
        dwEngineFileType,
        qwEngineFileId,
        fmf,
        cbFileSize,
        wszAbsPath };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidFileStationId,
        8,
        (BYTE *)&tsidr,
        (QWORD *)&hFile,
        (DWORD *)&dwDiskNumber,
        (DWORD *)&dwEngineFileType,
        (QWORD *)&qwEngineFileId,
        (DWORD *)&fmf,
        (QWORD *)&cbFileSize,
        wszAbsPath );
}

INLINE void ETIsamDbfilehdrInfo(
    BYTE    tsidr,
    DWORD    ifmp,
    DWORD    filetype,
    DWORD    ulMagic,
    DWORD    ulChecksum,
    DWORD    cbPageSize,
    DWORD    ulDbFlags,
    const BYTE *    psignDb )
{

#ifdef DEBUG
    EseIsamDbfilehdrInfoNative ettT = {
        tsidr,
        ifmp,
        filetype,
        ulMagic,
        ulChecksum,
        cbPageSize,
        ulDbFlags,
        psignDb };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIsamDbfilehdrInfo,
        8,
        (BYTE *)&tsidr,
        (DWORD *)&ifmp,
        (DWORD *)&filetype,
        (DWORD *)&ulMagic,
        (DWORD *)&ulChecksum,
        (DWORD *)&cbPageSize,
        (DWORD *)&ulDbFlags,
        psignDb );
}

INLINE void ETDiskOsDiskCacheInfo(
    BYTE    tsidr,
    const BYTE *    posdci )
{

#ifdef DEBUG
    EseDiskOsDiskCacheInfoNative ettT = {
        tsidr,
        posdci };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDiskOsDiskCacheInfo,
        2,
        (BYTE *)&tsidr,
        posdci );
}

INLINE void ETDiskOsStorageWriteCacheProp(
    BYTE    tsidr,
    const BYTE *    posswcp )
{

#ifdef DEBUG
    EseDiskOsStorageWriteCachePropNative ettT = {
        tsidr,
        posswcp };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDiskOsStorageWriteCacheProp,
        2,
        (BYTE *)&tsidr,
        posswcp );
}

INLINE void ETDiskOsDeviceSeekPenaltyDesc(
    BYTE    tsidr,
    const BYTE *    posdspd )
{

#ifdef DEBUG
    EseDiskOsDeviceSeekPenaltyDescNative ettT = {
        tsidr,
        posdspd };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDiskOsDeviceSeekPenaltyDesc,
        2,
        (BYTE *)&tsidr,
        posdspd );
}

INLINE void ETDirtyPage2Deprecated(
 )
{

#ifdef DEBUG
    EseDirtyPage2DeprecatedNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidDirtyPage2Deprecated );
}

INLINE void ETIOCompletion2(
    PCWSTR    wszFilename,
    DWORD    fMultiIor,
    BYTE    fWrite,
    DWORD    dwUserId,
    BYTE    bOperationId,
    BYTE    bOperationType,
    BYTE    bClientType,
    BYTE    bFlags,
    DWORD    dwCorrelationId,
    BYTE    iorp,
    BYTE    iors,
    BYTE    iort,
    BYTE    ioru,
    BYTE    iorf,
    BYTE    tce,
    PCSTR    szClientComponent,
    PCSTR    szClientAction,
    PCSTR    szClientActionContext,
    const GUID *    guidActivityId,
    QWORD    ibOffset,
    DWORD    cbTransfer,
    DWORD    dwError,
    DWORD    qosHighestFirst,
    QWORD    cmsecIOElapsed,
    DWORD    dtickQueueDelay,
    DWORD    tidAlloc,
    DWORD    dwEngineFileType,
    QWORD    dwEngineFileId,
    DWORD    fmfFile,
    DWORD    dwDiskNumber,
    DWORD    dwEngineObjid )
{

#ifdef DEBUG
    EseIOCompletion2Native ettT = {
        wszFilename,
        fMultiIor,
        fWrite,
        dwUserId,
        bOperationId,
        bOperationType,
        bClientType,
        bFlags,
        dwCorrelationId,
        iorp,
        iors,
        iort,
        ioru,
        iorf,
        tce,
        szClientComponent,
        szClientAction,
        szClientActionContext,
        guidActivityId,
        ibOffset,
        cbTransfer,
        dwError,
        qosHighestFirst,
        cmsecIOElapsed,
        dtickQueueDelay,
        tidAlloc,
        dwEngineFileType,
        dwEngineFileId,
        fmfFile,
        dwDiskNumber,
        dwEngineObjid };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOCompletion2,
        31,
        wszFilename,
        (DWORD *)&fMultiIor,
        (BYTE *)&fWrite,
        (DWORD *)&dwUserId,
        (BYTE *)&bOperationId,
        (BYTE *)&bOperationType,
        (BYTE *)&bClientType,
        (BYTE *)&bFlags,
        (DWORD *)&dwCorrelationId,
        (BYTE *)&iorp,
        (BYTE *)&iors,
        (BYTE *)&iort,
        (BYTE *)&ioru,
        (BYTE *)&iorf,
        (BYTE *)&tce,
        szClientComponent,
        szClientAction,
        szClientActionContext,
        guidActivityId,
        (QWORD *)&ibOffset,
        (DWORD *)&cbTransfer,
        (DWORD *)&dwError,
        (DWORD *)&qosHighestFirst,
        (QWORD *)&cmsecIOElapsed,
        (DWORD *)&dtickQueueDelay,
        (DWORD *)&tidAlloc,
        (DWORD *)&dwEngineFileType,
        (QWORD *)&dwEngineFileId,
        (DWORD *)&fmfFile,
        (DWORD *)&dwDiskNumber,
        (DWORD *)&dwEngineObjid );
}

INLINE void ETFCBPurgeFailure(
    DWORD    iInstance,
    BYTE    grbitPurgeFlags,
    BYTE    fcbpfr,
    BYTE    tce )
{

#ifdef DEBUG
    EseFCBPurgeFailureNative ettT = {
        iInstance,
        grbitPurgeFlags,
        fcbpfr,
        tce };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidFCBPurgeFailure,
        4,
        (DWORD *)&iInstance,
        (BYTE *)&grbitPurgeFlags,
        (BYTE *)&fcbpfr,
        (BYTE *)&tce );
}

INLINE void ETIOLatencySpikeNotice(
    DWORD    dwDiskNumber,
    DWORD    dtickSpikeLength )
{

#ifdef DEBUG
    EseIOLatencySpikeNoticeNative ettT = {
        dwDiskNumber,
        dtickSpikeLength };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOLatencySpikeNotice,
        2,
        (DWORD *)&dwDiskNumber,
        (DWORD *)&dtickSpikeLength );
}

INLINE void ETIOCompletion2Sess(
    PCWSTR    wszFilename,
    DWORD    fMultiIor,
    BYTE    fWrite,
    DWORD    dwUserId,
    BYTE    bOperationId,
    BYTE    bOperationType,
    BYTE    bClientType,
    BYTE    bFlags,
    DWORD    dwCorrelationId,
    BYTE    iorp,
    BYTE    iors,
    BYTE    iort,
    BYTE    ioru,
    BYTE    iorf,
    BYTE    tce,
    PCSTR    szClientComponent,
    PCSTR    szClientAction,
    PCSTR    szClientActionContext,
    const GUID *    guidActivityId,
    QWORD    ibOffset,
    DWORD    cbTransfer,
    DWORD    dwError,
    DWORD    qosHighestFirst,
    QWORD    cmsecIOElapsed,
    DWORD    dtickQueueDelay,
    DWORD    tidAlloc,
    DWORD    dwEngineFileType,
    QWORD    dwEngineFileId,
    DWORD    fmfFile,
    DWORD    dwDiskNumber,
    DWORD    dwEngineObjid )
{

#ifdef DEBUG
    EseIOCompletion2SessNative ettT = {
        wszFilename,
        fMultiIor,
        fWrite,
        dwUserId,
        bOperationId,
        bOperationType,
        bClientType,
        bFlags,
        dwCorrelationId,
        iorp,
        iors,
        iort,
        ioru,
        iorf,
        tce,
        szClientComponent,
        szClientAction,
        szClientActionContext,
        guidActivityId,
        ibOffset,
        cbTransfer,
        dwError,
        qosHighestFirst,
        cmsecIOElapsed,
        dtickQueueDelay,
        tidAlloc,
        dwEngineFileType,
        dwEngineFileId,
        fmfFile,
        dwDiskNumber,
        dwEngineObjid };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOCompletion2Sess,
        31,
        wszFilename,
        (DWORD *)&fMultiIor,
        (BYTE *)&fWrite,
        (DWORD *)&dwUserId,
        (BYTE *)&bOperationId,
        (BYTE *)&bOperationType,
        (BYTE *)&bClientType,
        (BYTE *)&bFlags,
        (DWORD *)&dwCorrelationId,
        (BYTE *)&iorp,
        (BYTE *)&iors,
        (BYTE *)&iort,
        (BYTE *)&ioru,
        (BYTE *)&iorf,
        (BYTE *)&tce,
        szClientComponent,
        szClientAction,
        szClientActionContext,
        guidActivityId,
        (QWORD *)&ibOffset,
        (DWORD *)&cbTransfer,
        (DWORD *)&dwError,
        (DWORD *)&qosHighestFirst,
        (QWORD *)&cmsecIOElapsed,
        (DWORD *)&dtickQueueDelay,
        (DWORD *)&tidAlloc,
        (DWORD *)&dwEngineFileType,
        (QWORD *)&dwEngineFileId,
        (DWORD *)&fmfFile,
        (DWORD *)&dwDiskNumber,
        (DWORD *)&dwEngineObjid );
}

INLINE void ETIOIssueThreadPost(
    const void *    p_osf,
    DWORD    cioDiskEnqueued )
{

#ifdef DEBUG
    EseIOIssueThreadPostNative ettT = {
        p_osf,
        cioDiskEnqueued };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOIssueThreadPost,
        2,
        (const void * *)&p_osf,
        (DWORD *)&cioDiskEnqueued );
}

INLINE void ETIOIssueThreadPosted(
    const void *    p_osf,
    DWORD    cDispatchAttempts,
    QWORD    usPosted )
{

#ifdef DEBUG
    EseIOIssueThreadPostedNative ettT = {
        p_osf,
        cDispatchAttempts,
        usPosted };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOIssueThreadPosted,
        3,
        (const void * *)&p_osf,
        (DWORD *)&cDispatchAttempts,
        (QWORD *)&usPosted );
}

INLINE void ETIOThreadIssueStart(
 )
{

#ifdef DEBUG
    EseIOThreadIssueStartNative ettT = {
 };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOThreadIssueStart );
}

INLINE void ETIOThreadIssuedDisk(
    DWORD    dwDiskId,
    BYTE    fFromCompletion,
    __int64    ipass,
    INT    err,
    DWORD    cioProcessed,
    QWORD    usRuntime )
{

#ifdef DEBUG
    EseIOThreadIssuedDiskNative ettT = {
        dwDiskId,
        fFromCompletion,
        ipass,
        err,
        cioProcessed,
        usRuntime };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOThreadIssuedDisk,
        6,
        (DWORD *)&dwDiskId,
        (BYTE *)&fFromCompletion,
        (__int64 *)&ipass,
        (INT *)&err,
        (DWORD *)&cioProcessed,
        (QWORD *)&usRuntime );
}

INLINE void ETIOThreadIssueProcessedIO(
    INT    err,
    DWORD    cDisksProcessed,
    QWORD    usRuntime )
{

#ifdef DEBUG
    EseIOThreadIssueProcessedIONative ettT = {
        err,
        cDisksProcessed,
        usRuntime };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOThreadIssueProcessedIO,
        3,
        (INT *)&err,
        (DWORD *)&cDisksProcessed,
        (QWORD *)&usRuntime );
}

INLINE void ETIOIoreqCompletion(
    BYTE    fWrite,
    QWORD    iFile,
    QWORD    ibOffset,
    DWORD    cbData,
    DWORD    dwDiskNumber,
    DWORD    tidAlloc,
    QWORD    qos,
    DWORD    iIoreq,
    DWORD    err,
    QWORD    usCompletionDelay )
{

#ifdef DEBUG
    EseIOIoreqCompletionNative ettT = {
        fWrite,
        iFile,
        ibOffset,
        cbData,
        dwDiskNumber,
        tidAlloc,
        qos,
        iIoreq,
        err,
        usCompletionDelay };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidIOIoreqCompletion,
        10,
        (BYTE *)&fWrite,
        (QWORD *)&iFile,
        (QWORD *)&ibOffset,
        (DWORD *)&cbData,
        (DWORD *)&dwDiskNumber,
        (DWORD *)&tidAlloc,
        (QWORD *)&qos,
        (DWORD *)&iIoreq,
        (DWORD *)&err,
        (QWORD *)&usCompletionDelay );
}

INLINE void ETCacheMemoryUsage(
    PCWSTR    wszFilename,
    BYTE    tce,
    DWORD    dwEngineFileType,
    QWORD    dwEngineFileId,
    QWORD    cbMemory,
    DWORD    cmsecReferenceIntervalMax )
{

#ifdef DEBUG
    EseCacheMemoryUsageNative ettT = {
        wszFilename,
        tce,
        dwEngineFileType,
        dwEngineFileId,
        cbMemory,
        cmsecReferenceIntervalMax };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheMemoryUsage,
        6,
        wszFilename,
        (BYTE *)&tce,
        (DWORD *)&dwEngineFileType,
        (QWORD *)&dwEngineFileId,
        (QWORD *)&cbMemory,
        (DWORD *)&cmsecReferenceIntervalMax );
}

INLINE void ETCacheSetLgposModify(
    DWORD    tick,
    DWORD    ifmp,
    DWORD    pgno,
    QWORD    lgposModify )
{

#ifdef DEBUG
    EseCacheSetLgposModifyNative ettT = {
        tick,
        ifmp,
        pgno,
        lgposModify };
    FakeUseTrace( (void*)&ettT );
#endif

    OSEventTrace(
        _etguidCacheSetLgposModify,
        4,
        (DWORD *)&tick,
        (DWORD *)&ifmp,
        (DWORD *)&pgno,
        (QWORD *)&lgposModify );
}

