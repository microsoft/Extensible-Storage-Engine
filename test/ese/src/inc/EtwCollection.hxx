// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _ETWCOLLECTION_HXX_INCLUDED
#define _ETWCOLLECTION_HXX_INCLUDED

#include <windows.h>



typedef enum _EtwEventType : USHORT
{
    etwevttKernelCSwitch = 0,
    etwevttEseTestMarker,
    etwevttEseTimerTaskRun,
    etwevttEseTaskManagerRun,
    etwevttEseGPTaskManagerRun,
    etwevttEseThreadStart,
    etwevttEseIoCompletion,
    etwevttEseResMgrInit,
    etwevttEseResMgrTerm,
    etwevttEseBfCachePage,
    etwevttEseBfRequestPage,
    etwevttEseBfEvictPage,
    etwevttEseBfMarkPageAsSuperCold,
    etwevttEseBfDirtyPage,
    etwevttEseBfWritePage,
    etwevttEseBfSetLgposModify,
    etwevttEseBfNewPage,
    etwevttEseBfReadPage,
    etwevttEseBfPrereadPage,
    etwevttEseTaskMaxMax
} EtwEventType;



typedef struct _EtwEvent
{
    EtwEventType etwEvtType;
    LONGLONG timestamp;
    ULONG ibExtraData;
    ULONG cbExtraData;
} EtwEvent;



typedef enum _ThreadState : BYTE
{
    thstInitialized = 0,
    thstReady,
    thstRunning,
    thstStandby,
    thstTerminated,
    thstWaiting,
    thstTransition,
    thstDeferredReady
} ThreadState;


typedef struct _EventHeader
{
    ULONG ulProcessId;
    ULONG ulThreadId;
} EventHeader;


typedef struct _KernelCSwitch : EventHeader
{
    ULONG ulOldProcessId;
    ULONG ulOldThreadId;
    ThreadState thstOldThread;
} KernelCSwitch;


typedef struct _EseTestMarker : EventHeader
{
    ULONGLONG ullTestMarkerId;
    CHAR* szAnnotation;
} EseTestMarker;


typedef struct _EseTimerTaskRun : EventHeader
{
} EseTimerTaskRun;


typedef struct _EseTaskManagerRun : EventHeader
{
} EseTaskManagerRun;


typedef struct _EseGPTaskManagerRun : EventHeader
{
} EseGPTaskManagerRun;


typedef struct _EseThreadStart : EventHeader
{
} EseThreadStart;


typedef struct _EseTraceContextEvent
{
    ULONG       ulUserId;
    BYTE        bOperationId;
    BYTE        bOperationType;
    BYTE        bClientType;
    BYTE        bFlags;
    ULONG       ulCorrelationId;
    BYTE        bIorp;
    BYTE        bIors;
    BYTE        bIort;
    BYTE        bIoru;
    BYTE        bIorf;
    BYTE        bParentObjectclass;
} EseTraceContextEvent;


typedef struct _EseIoCompletion : EventHeader
{
    ULONGLONG   ullFileNumber;
    ULONG       ulMultiIor;
    BYTE        fWrite;
    EseTraceContextEvent tcevtIO;
    ULONGLONG   ibOffset;
    ULONG       cbTransfer;
    ULONG       ulError;
    ULONG       ulQosHighestFirst;
    ULONGLONG   cmsecElapsed;
    ULONG       dtickCompletionDelay;
    ULONG       tidAlloc;
    ULONG       dwEngineFileType;
    ULONGLONG   qwEngineFileId;
    ULONG       fmfFile;
    ULONG       dwDiskNumber;
    ULONG       dwEngineObjid;
} EseIoCompletion;


typedef struct _EseBfResMgrEvent : EventHeader
{
    ULONG tick;
} EseBfResMgrEvent;


typedef struct _EseResMgrInit : EseBfResMgrEvent
{
    LONG K;
    double csecCorrelatedTouch;
    double csecTimeout;
    double csecUncertainty;
    double dblHashLoadFactor;
    double dblHashUniformity;
    double dblSpeedSizeTradeoff;
} EseResMgrInit;


typedef struct _EseResMgrTerm : EseBfResMgrEvent
{
} EseResMgrTerm;


typedef struct _EseBfCachePage : EseBfResMgrEvent
{
    ULONG ifmp;
    ULONG pgno;
    ULONG bflf;
    ULONG bflt;
    ULONG pctPriority;
    ULONG bfrtf;
    BYTE clientType;
} EseBfCachePage;


typedef struct _EseBfRequestPage : EseBfResMgrEvent
{
    ULONG ifmp;
    ULONG pgno;
    ULONG bflf;
    ULONG ulObjId;
    ULONG fPageFlags;
    ULONG bflt;
    ULONG pctPriority;
    ULONG bfrtf;
    BYTE clientType;
} EseBfRequestPage;


typedef struct _EseBfEvictPage : EseBfResMgrEvent
{
    ULONG ifmp;
    ULONG pgno;
    ULONG fCurrentVersion;
    LONG errBF;
    ULONG bfef;
    ULONG pctPriority;
} EseBfEvictPage;


typedef struct _EseBfMarkPageAsSuperCold : EseBfResMgrEvent
{
    ULONG ifmp;
    ULONG pgno;
} EseBfMarkPageAsSuperCold;


typedef struct _EseBfDirtyPage : EseBfResMgrEvent
{
    UINT        ifmp;
    ULONG       pgno;
    ULONG       objid;
    ULONG       fPageFlags;
    ULONG       ulDirtyLevel;
    union
    {
        struct
        {
            USHORT  ib;
            USHORT  isec;
            ULONG   lGen;
        };
        ULONG64 qw;
    } lgposModify;
    EseTraceContextEvent tcevtBf;
} EseBfDirtyPage;


typedef struct _EseBfWritePage : EseBfResMgrEvent
{
    UINT        ifmp;
    ULONG       pgno;
    ULONG       objid;
    ULONG       fPageFlags;
    ULONG       ulDirtyLevel;
    EseTraceContextEvent tcevtBf;
} EseBfWritePage;


typedef struct _EseBfSetLgposModify : EseBfResMgrEvent
{
    UINT        ifmp;
    ULONG       pgno;
    union
    {
        struct
        {
            USHORT  ib;
            USHORT  isec;
            ULONG   lGen;
        };
        ULONG64 qw;
    } lgposModify;
} EseBfSetLgposModify;


typedef struct _EseBfPageEvent : EventHeader
{
    UINT        ifmp;
    ULONG       pgno;
    ULONG       ulLatchFlags;
    ULONG       objid;
    ULONG       fPageFlags;
    EseTraceContextEvent tcevtBf;
} EseBfPageEvent;


typedef struct _EseBfNewPage : EseBfPageEvent
{
} EseBfNewPage;


typedef struct _EseBfReadPage : EseBfPageEvent
{
} EseBfReadPage;


typedef struct _EseBfPrereadPage : EventHeader
{
    UINT        ifmp;
    ULONG       pgno;
    EseTraceContextEvent tcevtBf;
} EseBfPrereadPage;



HANDLE EtwOpenTraceFile( __in PCWSTR wszFilePath );



EtwEvent* EtwGetNextEvent( __in HANDLE handle );



VOID EtwFreeEvent( __in EtwEvent* const pEtwEvt );



VOID EtwCloseTraceFile( __in HANDLE handle );

#endif _ETWCOLLECTION_HXX_INCLUDED
