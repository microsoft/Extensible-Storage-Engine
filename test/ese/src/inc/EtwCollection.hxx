// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _ETWCOLLECTION_HXX_INCLUDED
#define _ETWCOLLECTION_HXX_INCLUDED

#include <windows.h>

//================================================================
// ETW tracing helper functions.
//================================================================
//

// Event types.

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


// Generic ETW event.

typedef struct _EtwEvent
{
    EtwEventType etwEvtType;    // Event type.
    LONGLONG timestamp;         // Timestamp in units of 100 nano-seconds, counted from the first event in the file.
    ULONG ibExtraData;          // Offset of the extra data. Always points to the first byte after this structure.
    ULONG cbExtraData;          // Size of the extra data. 0 if no extra data.
} EtwEvent;


// Thread states.

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

// Virtually all event types carry at least process and thread IDs.

typedef struct _EventHeader
{
    ULONG ulProcessId;
    ULONG ulThreadId;
} EventHeader;

// Specific data for etwevttKernelCSwitch.

typedef struct _KernelCSwitch : EventHeader
{
    ULONG ulOldProcessId;
    ULONG ulOldThreadId;
    ThreadState thstOldThread;
} KernelCSwitch;

// Specific data for etwevttEseTestMarker.

typedef struct _EseTestMarker : EventHeader
{
    ULONGLONG ullTestMarkerId;
    CHAR* szAnnotation;
} EseTestMarker;

// Specific data for etwevttEseTimerTaskRun.

typedef struct _EseTimerTaskRun : EventHeader
{
} EseTimerTaskRun;

// Specific data for etwevttEseTaskManagerRun.

typedef struct _EseTaskManagerRun : EventHeader
{
} EseTaskManagerRun;

// Specific data for etwevttEseGPTaskManagerRun.

typedef struct _EseGPTaskManagerRun : EventHeader
{
} EseGPTaskManagerRun;

// Specific data for etwevttEseThreadStart.

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

// Specific data for etwevttEseIoCompletion.

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

// Generic BFRESMGR event.

typedef struct _EseBfResMgrEvent : EventHeader
{
    ULONG tick;
} EseBfResMgrEvent;

// Specific data for etwevttEseResMgrInit.

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

// Specific data for etwevttEseResMgrTerm.

typedef struct _EseResMgrTerm : EseBfResMgrEvent
{
} EseResMgrTerm;

// Specific data for etwevttEseBfCachePage.

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

// Specific data for etwevttEseBfRequestPage.

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

// Specific data for etwevttEseBfEvictPage.

typedef struct _EseBfEvictPage : EseBfResMgrEvent
{
    ULONG ifmp;
    ULONG pgno;
    ULONG fCurrentVersion;
    LONG errBF;
    ULONG bfef;
    ULONG pctPriority;  // OBSOLETE (always 0)
} EseBfEvictPage;

// Specific data for etwevttEseBfMarkPageAsSuperCold.

typedef struct _EseBfMarkPageAsSuperCold : EseBfResMgrEvent
{
    ULONG ifmp;
    ULONG pgno;
} EseBfMarkPageAsSuperCold;

// Specific data for etwevttEseBfDirtyPage.

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

// Specific data for etwevttEseBfWritePage.

typedef struct _EseBfWritePage : EseBfResMgrEvent
{
    UINT        ifmp;
    ULONG       pgno;
    ULONG       objid;
    ULONG       fPageFlags;
    ULONG       ulDirtyLevel;
    EseTraceContextEvent tcevtBf;
} EseBfWritePage;

// Specific data for etwevttEseBfSetLgposModify.

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

// Specific data for etwevttEseBfNewPage and etwevttEseBfReadPage.

typedef struct _EseBfPageEvent : EventHeader
{
    UINT        ifmp;
    ULONG       pgno;
    ULONG       ulLatchFlags;
    ULONG       objid;
    ULONG       fPageFlags;
    EseTraceContextEvent tcevtBf;
} EseBfPageEvent;

// Specific data for etwevttEseBfNewPage.

typedef struct _EseBfNewPage : EseBfPageEvent
{
} EseBfNewPage;

// Specific data for etwevttEseBfReadPage.

typedef struct _EseBfReadPage : EseBfPageEvent
{
} EseBfReadPage;

// Specific data for etwevttEseBfPrereadPage.

typedef struct _EseBfPrereadPage : EventHeader
{
    UINT        ifmp;
    ULONG       pgno;
    EseTraceContextEvent tcevtBf;
} EseBfPrereadPage;

// Functions to read and process ETW events.

// EtwOpenTraceFile
//
// Description: opens an ETL trace file for processing.
//
// wszFilePath: path to the trace file.
//
// Return value: opaque handle to be used in other functions of this API set.
// NULL if the function fails. Call GetlastError() to get specifics about the error.
//

HANDLE EtwOpenTraceFile( __in PCWSTR wszFilePath );


// EtwGetNextEvent
//
// Description: retrieves the next known event in the file.
//
// handle: opaque handle returned by EtwOpenTraceFile.
//
// Return value: the next known event in the file. NULL if the function fails.
// Call GetlastError() to get specifics about the error. ERROR_NO_MORE_ITEMS means
// there are no more events to be returned.
//

EtwEvent* EtwGetNextEvent( __in HANDLE handle );


// EtwFreeEvent
//
// Description: frees memory allocated to hold event information.
//
// pEtwEvt: pointer to the event.
//

VOID EtwFreeEvent( __in EtwEvent* const pEtwEvt );


// EtwCloseTraceFile
//
// Description: closes the file.
//
// handle: opaque handle returned by EtwOpenTraceFile.
//

VOID EtwCloseTraceFile( __in HANDLE handle );

#endif _ETWCOLLECTION_HXX_INCLUDED
