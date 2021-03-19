// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <ese_common.hxx>
#include <unordered_map>
#define INITGUID
#include <wmistr.h>
#include <evntrace.h>
#include <tdh.h>
#include <Microsoft-ETW-ESE.h>


// Either ESE or ESENT.

#ifdef BUILD_ENV_IS_NT
#define MakeEseEtwSymbolName( name )        (Microsoft_Windows_##name##)
#else
#define MakeEseEtwSymbolName( name )        (Microsoft_Exchange_##name##)
#endif


// Additional definitions.

DEFINE_GUID
    ( /* 3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c */
    MSNT_SystemTrace_Thread_V2,
    0x3d6fa8d1,
    0xfe05,
    0x11d0,
    0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c
    );


static const UCHAR MSNT_SystemTrace_Thread_V2_TStart        = 1;
static const UCHAR MSNT_SystemTrace_Thread_V2_TEnd          = 2;
static const UCHAR MSNT_SystemTrace_Thread_V2_TDCStart      = 3;
static const UCHAR MSNT_SystemTrace_Thread_V2_TDCEnd        = 4;
static const UCHAR MSNT_SystemTrace_Thread_V2_CSwitch       = 36;


// ContextSwitch raw trace.

typedef struct _ContextSwitch
{
    ULONG NewThreadId;
    ULONG OldThreadId;
    BYTE NewThreadPriority;
    BYTE OldThreadPriority;
    BYTE PreviousCState;
    BYTE SpareByte;
    BYTE OldThreadWaitReason;
    BYTE OldThreadWaitMode;
    BYTE OldThreadState;
    BYTE OldThreadWaitIdealProcessor;
    ULONG NewThreadWaitTime;
    ULONG Reserved;
} ContextSwitch;


// ThreadTypeGroup1 raw trace.

typedef struct _ThreadTypeGroup1
{
    ULONG ProcessId;
    ULONG ThreadId;
    ULONG StackBase;
    ULONG StackLimit;
    ULONG UserStackBase;
    ULONG UserStackLimit;
    ULONG StartAddr;
    ULONG Win32StartAddr;
    ULONG TebBase;
    ULONG SubProcessTag;
} ThreadTypeGroup1;


// Callback to query for extra size when processing ESE events.

typedef ULONG (__stdcall* PfnCbExtraDataCallback)(
    const BYTE* const pbUserData,
    const size_t cbUserData );


// Callback to perform custom processing of extra data when processing ESE events.

typedef DWORD (__stdcall* PfnExtraDataProcessingCallback)(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData );


// Structure to define how each ESE event should be processed.

typedef struct _StandardEseEventProcessor
{
    const EVENT_DESCRIPTOR* const evtDescriptor;
    EtwEventType etwEvtType;
    size_t cbStructDefault;
    PfnCbExtraDataCallback pfnCbExtraDataCallback;
    PfnExtraDataProcessingCallback pfnExtraDataProcessingCallback;
} StandardEseEventProcessor;


// Helper functions.

FORCEINLINE EtwEvent* AllocStandardEseEvent(
    _In_ const EtwEventType etwEvtType,
    _In_ const ULONG cbEseEvent,
    _In_ const LONGLONG dftTimestamp,
    _In_ const ULONG ulProcessId,
    _In_ const ULONG ulThreadId,
    _In_ const ULONG cbExtraData )
{
    const ULONG ibExtraData = sizeof(EtwEvent);
    const ULONG cbExtraUserData = cbEseEvent;
    const ULONG cbAlloc = ibExtraData + cbExtraUserData + cbExtraData;
    EtwEvent* const pEtwEvt = (EtwEvent*)( new BYTE[cbAlloc] );
    memset( pEtwEvt, 0, cbAlloc );

    if ( pEtwEvt == NULL )
    {
        goto HandleError;
    }
    
    pEtwEvt->etwEvtType = etwEvtType;
    pEtwEvt->timestamp = dftTimestamp;
    pEtwEvt->ibExtraData = ibExtraData;
    pEtwEvt->cbExtraData = cbExtraUserData;

    size_t ibOffset = 0;
    EventHeader* const pEseEvent = (EventHeader*)( (BYTE*)pEtwEvt + ibExtraData );
    pEseEvent->ulProcessId = ulProcessId;
    pEseEvent->ulThreadId = ulThreadId;

HandleError:
    return pEtwEvt;
}

FORCEINLINE ULONG SafeCopyData(
    _In_ BYTE* const pDest,
    _In_ const size_t cbDest,
    __in_bcount(cbData) const BYTE* const pbData,
    _In_ const size_t cbData,
    __inout size_t* const pibOffset )
{
    const size_t ibOffset = *pibOffset;
    const size_t cbCopy = cbDest;
    
    if ( ( ibOffset + cbCopy ) > cbData )
    {
        return ERROR_INTERNAL_ERROR;
    }

    memcpy( pDest, pbData + ibOffset, cbDest );

    *pibOffset += cbCopy;

    return ERROR_SUCCESS;
}

FORCEINLINE size_t GetSafeStrSize(
    __in_bcount(cbData) const BYTE* const pbData,
    _In_ const size_t cbData,
    _In_ const size_t ibOffset )
{
    const size_t cbRemaining = ( cbData > ibOffset ) ? ( cbData - ibOffset ) : 0;
    const char* const szData = (char*)( pbData + ibOffset );
    const size_t cbSize = sizeof(char) * ( strnlen( szData, cbRemaining ) + 1 );

    if ( cbSize > cbRemaining )
    {
        return 0;
    }

    return cbSize;
}

FORCEINLINE ULONG SafeCopyString(
    _In_ char* const szDest,
    __in_bcount(cbData) const BYTE* const pbData,
    _In_ const size_t cbData,
    __inout size_t* const pibOffset )
{
    // StringCchCopyN() comes close to what we need from this function, but not quite.
    // It makes sure we won't go past the end of both input/output buffers, but won't
    // error out we get to the end of input with no null termination.
    
    DWORD dwErrorCode = ERROR_SUCCESS;

    const size_t ibOffset = *pibOffset;
    const size_t cbRemaining = ( cbData > ibOffset ) ? ( cbData - ibOffset ) : 0;
    const char* const szData = (char*)( pbData + ibOffset);
    const size_t cbCopy = sizeof(char) * ( strnlen( szData, cbRemaining ) + 1 );

    if ( cbCopy > cbRemaining )
    {
        return ERROR_INTERNAL_ERROR;
    }

    memcpy( szDest, szData, cbCopy );

    *pibOffset += cbCopy;

    return ERROR_SUCCESS;
}

FORCEINLINE ULONG UlGetEvtDescriptorId( const EVENT_DESCRIPTOR* const evtDescriptor)
{
    return ( ( evtDescriptor->Id << ( sizeof( USHORT ) * 8 ) ) | evtDescriptor->Task );
}


// Helper macros.

#define EtwCollectionAlloc( pv )            \
{                                           \
    if ( (pv) == NULL )                     \
    {                                       \
        dwErrorCode = ERROR_OUTOFMEMORY;    \
        goto HandleError;                   \
    }                                       \
}

#define EtwCollectionCall( fn )                     \
{                                                   \
    if ( ( dwErrorCode = (fn) ) != ERROR_SUCCESS )  \
    {                                               \
        goto HandleError;                           \
    }                                               \
}


// Callbacks.

ULONG CbExtraDataTestMarker(
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    return GetSafeStrSize( pbUserData, cbUserData, offsetof( EseTestMarker, szAnnotation ) - offsetof( EseTestMarker, ullTestMarkerId ) );
}

DWORD ExtraDataProcessingTestMarker(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseTestMarker* const pEseEvent = (EseTestMarker*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ullTestMarkerId,
            sizeof(pEseEvent->ullTestMarkerId),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    pEseEvent->szAnnotation = (char*)( (BYTE*)pEseEvent + sizeof(*pEseEvent) );

    EtwCollectionCall(
        SafeCopyString(
            pEseEvent->szAnnotation,
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingTraceContextEvent(
    EseTraceContextEvent* const pTraceContextEvent,
    const BYTE* const pbUserData,
    const size_t cbUserData,
    size_t* const pibOffset)
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->ulUserId,
            sizeof(pTraceContextEvent->ulUserId),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bOperationId,
            sizeof(pTraceContextEvent->bOperationId),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bOperationType,
            sizeof(pTraceContextEvent->bOperationType),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bClientType,
            sizeof(pTraceContextEvent->bClientType),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bFlags,
            sizeof(pTraceContextEvent->bFlags),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->ulCorrelationId,
            sizeof(pTraceContextEvent->ulCorrelationId),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bIorp,
            sizeof(pTraceContextEvent->bIorp),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bIors,
            sizeof(pTraceContextEvent->bIors),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bIort,
            sizeof(pTraceContextEvent->bIort),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bIoru,
            sizeof(pTraceContextEvent->bIoru),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bIorf,
            sizeof(pTraceContextEvent->bIorf),
            pbUserData,
            cbUserData,
            pibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pTraceContextEvent->bParentObjectclass,
            sizeof(pTraceContextEvent->bParentObjectclass),
            pbUserData,
            cbUserData,
            pibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingIoCompletion(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseIoCompletion* const pEseEvent = (EseIoCompletion*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ullFileNumber,
            sizeof(pEseEvent->ullFileNumber),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulMultiIor,
            sizeof(pEseEvent->ulMultiIor),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fWrite,
            sizeof(pEseEvent->fWrite),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        ExtraDataProcessingTraceContextEvent(
            &pEseEvent->tcevtIO,
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ibOffset,
            sizeof(pEseEvent->ibOffset),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->cbTransfer,
            sizeof(pEseEvent->cbTransfer),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulError,
            sizeof(pEseEvent->ulError),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulQosHighestFirst,
            sizeof(pEseEvent->ulQosHighestFirst),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->cmsecElapsed,
            sizeof(pEseEvent->cmsecElapsed),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dtickCompletionDelay,
            sizeof(pEseEvent->dtickCompletionDelay),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tidAlloc,
            sizeof(pEseEvent->tidAlloc),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dwEngineFileType,
            sizeof(pEseEvent->dwEngineFileType),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->qwEngineFileId,
            sizeof(pEseEvent->qwEngineFileId),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fmfFile,
            sizeof(pEseEvent->fmfFile),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dwDiskNumber,
            sizeof(pEseEvent->dwDiskNumber),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dwEngineObjid,
            sizeof(pEseEvent->dwEngineObjid),
            pbUserData,
            cbUserData,
            &ibOffset ) );
            
HandleError:
    return dwErrorCode;
/*
    EseIoCompletion* const pEseEvent = (EseIoCompletion*)pEventHeader;
    int size = sizeof(EseIoCompletion) - offsetof(EseIoCompletion, ullFileNumber);
    if ( cbUserData != size )
    {
        return ERROR_INTERNAL_ERROR;
    }

    memcpy(&pEseEvent->ullFileNumber, pbUserData, size);
    return ERROR_SUCCESS;*/
}

DWORD ExtraDataProcessingResMgrInit(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseResMgrInit* const pEseEvent = (EseResMgrInit*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->K,
            sizeof(pEseEvent->K),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->csecCorrelatedTouch,
            sizeof(pEseEvent->csecCorrelatedTouch),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->csecTimeout,
            sizeof(pEseEvent->csecTimeout),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->csecUncertainty,
            sizeof(pEseEvent->csecUncertainty),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dblHashLoadFactor,
            sizeof(pEseEvent->dblHashLoadFactor),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dblHashUniformity,
            sizeof(pEseEvent->dblHashUniformity),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->dblSpeedSizeTradeoff,
            sizeof(pEseEvent->dblSpeedSizeTradeoff),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingResMgrTerm(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseResMgrTerm* const pEseEvent = (EseResMgrTerm*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfCachePage(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfCachePage* const pEseEvent = (EseBfCachePage*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bflf,
            sizeof(pEseEvent->bflf),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bflt,
            sizeof(pEseEvent->bflt),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pctPriority,
            sizeof(pEseEvent->pctPriority),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bfrtf,
            sizeof(pEseEvent->bfrtf),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->clientType,
            sizeof(pEseEvent->clientType),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfRequestPage(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfRequestPage* const pEseEvent = (EseBfRequestPage*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bflf,
            sizeof(pEseEvent->bflf),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulObjId,
            sizeof(pEseEvent->ulObjId),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fPageFlags,
            sizeof(pEseEvent->fPageFlags),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bflt,
            sizeof(pEseEvent->bflt),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pctPriority,
            sizeof(pEseEvent->pctPriority),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bfrtf,
            sizeof(pEseEvent->bfrtf),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->clientType,
            sizeof(pEseEvent->clientType),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfEvictPage(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfEvictPage* const pEseEvent = (EseBfEvictPage*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fCurrentVersion,
            sizeof(pEseEvent->fCurrentVersion),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->errBF,
            sizeof(pEseEvent->errBF),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->bfef,
            sizeof(pEseEvent->bfef),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pctPriority,
            sizeof(pEseEvent->pctPriority),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfMarkPageAsSuperCold(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfMarkPageAsSuperCold* const pEseEvent = (EseBfMarkPageAsSuperCold*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfDirtyPage(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfDirtyPage* const pEseEvent = (EseBfDirtyPage*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->objid,
            sizeof(pEseEvent->objid),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fPageFlags,
            sizeof(pEseEvent->fPageFlags),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulDirtyLevel,
            sizeof(pEseEvent->ulDirtyLevel),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->lgposModify,
            sizeof(pEseEvent->lgposModify),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        ExtraDataProcessingTraceContextEvent(
            &pEseEvent->tcevtBf,
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfWritePage(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    _EseBfWritePage* const pEseEvent = (_EseBfWritePage*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->objid,
            sizeof(pEseEvent->objid),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fPageFlags,
            sizeof(pEseEvent->fPageFlags),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulDirtyLevel,
            sizeof(pEseEvent->ulDirtyLevel),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        ExtraDataProcessingTraceContextEvent(
            &pEseEvent->tcevtBf,
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfSetLgposModify(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfSetLgposModify* const pEseEvent = (EseBfSetLgposModify*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->tick,
            sizeof(pEseEvent->tick),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->lgposModify,
            sizeof(pEseEvent->lgposModify),
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfPageEvent(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfPageEvent* const pEseEvent = (EseBfPageEvent*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ulLatchFlags,
            sizeof(pEseEvent->ulLatchFlags),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->objid,
            sizeof(pEseEvent->objid),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->fPageFlags,
            sizeof(pEseEvent->fPageFlags),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        ExtraDataProcessingTraceContextEvent(
            &pEseEvent->tcevtBf,
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

DWORD ExtraDataProcessingBfPrereadPage(
    EventHeader* const pEventHeader,
    const BYTE* const pbUserData,
    const size_t cbUserData )
{
    DWORD dwErrorCode = ERROR_SUCCESS;

    EseBfPrereadPage* const pEseEvent = (EseBfPrereadPage*)pEventHeader;
    size_t ibOffset = 0;

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->ifmp,
            sizeof(pEseEvent->ifmp),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        SafeCopyData(
            (BYTE*)&pEseEvent->pgno,
            sizeof(pEseEvent->pgno),
            pbUserData,
            cbUserData,
            &ibOffset ) );

    EtwCollectionCall(
        ExtraDataProcessingTraceContextEvent(
            &pEseEvent->tcevtBf,
            pbUserData,
            cbUserData,
            &ibOffset ) );

HandleError:
    return dwErrorCode;
}

// Table that maps event descriptors and ESE event processors.

static const StandardEseEventProcessor g_rgEseEvtProc[] =
{
    { &ESE_TestMarker_Trace,            etwevttEseTestMarker,               sizeof(EseTestMarker),              CbExtraDataTestMarker,  ExtraDataProcessingTestMarker },
    { &ESE_TimerTaskRun_Trace,          etwevttEseTimerTaskRun,             sizeof(EseTimerTaskRun),            NULL,                   NULL },
    { &ESE_TaskManagerRun_Trace,        etwevttEseTaskManagerRun,           sizeof(EseTaskManagerRun),          NULL,                   NULL },
    { &ESE_GPTaskManagerRun_Trace,      etwevttEseGPTaskManagerRun,         sizeof(EseGPTaskManagerRun),        NULL,                   NULL },
    { &ESE_ThreadStart_Trace,           etwevttEseThreadStart,              sizeof(EseThreadStart),             NULL,                   NULL },
    { &ESE_IOCompletion_Trace,          etwevttEseIoCompletion,             sizeof(EseIoCompletion),            NULL,                   ExtraDataProcessingIoCompletion },
    { &ESE_ResMgrInit_Trace,            etwevttEseResMgrInit,               sizeof(EseResMgrInit),              NULL,                   ExtraDataProcessingResMgrInit },
    { &ESE_ResMgrTerm_Trace,            etwevttEseResMgrTerm,               sizeof(EseResMgrTerm),              NULL,                   ExtraDataProcessingResMgrTerm },
    { &ESE_CacheCachePage_Trace,        etwevttEseBfCachePage,              sizeof(EseBfCachePage),             NULL,                   ExtraDataProcessingBfCachePage },
    { &ESE_CacheRequestPage_Trace,      etwevttEseBfRequestPage,            sizeof(EseBfRequestPage),           NULL,                   ExtraDataProcessingBfRequestPage },
    { &ESE_CacheEvictPage_Trace,        etwevttEseBfEvictPage,              sizeof(EseBfEvictPage),             NULL,                   ExtraDataProcessingBfEvictPage },
    { &ESE_MarkPageAsSuperCold_Trace,   etwevttEseBfMarkPageAsSuperCold,    sizeof(EseBfMarkPageAsSuperCold),   NULL,                   ExtraDataProcessingBfMarkPageAsSuperCold },
    { &ESE_CacheDirtyPage_Trace,        etwevttEseBfDirtyPage,              sizeof(EseBfDirtyPage),             NULL,                   ExtraDataProcessingBfDirtyPage },
    { &ESE_CacheWritePage_Trace,        etwevttEseBfWritePage,              sizeof(EseBfWritePage),             NULL,                   ExtraDataProcessingBfWritePage },
    { &ESE_CacheSetLgposModify_Trace,   etwevttEseBfSetLgposModify,         sizeof(EseBfSetLgposModify),        NULL,                   ExtraDataProcessingBfSetLgposModify },
    { &ESE_CacheNewPage_Trace,          etwevttEseBfNewPage,                sizeof(EseBfNewPage),               NULL,                   ExtraDataProcessingBfPageEvent },
    { &ESE_CacheReadPage_Trace,         etwevttEseBfReadPage,               sizeof(EseBfReadPage),              NULL,                   ExtraDataProcessingBfPageEvent },   // NewPage and ReadPage are the same events, and processed by the same method
    { &ESE_CachePrereadPage_Trace,      etwevttEseBfPrereadPage,            sizeof(EseBfPrereadPage),           NULL,                   ExtraDataProcessingBfPrereadPage },
};


// Internal implementation.

class EtwEventProcessor
{
    private:
        typedef std::unordered_map<ULONG, ULONG> ThreadIdProcId;
        typedef std::unordered_map<ULONG, const StandardEseEventProcessor*> EventDescEseProcessor;
    
    private:
        HANDLE hEventGo;                        // Signals the callback to return the next event.
        HANDLE hEventReady;                     // Signals the user-level API that the event is ready for consumption.
        HANDLE hEventStop;                      // Signals the collection thread to stop.
        HANDLE hThreadCollect;                  // Thread to wait on the processing callback.
        EtwEvent* pEtwEvt;                      // Event processed by the callback and to be returned by the GetNextEvent().
        bool fCollecting;                       // Flags if collection is currently under way.
        TRACEHANDLE hTrace;                     // Opaque trace handle.
        DWORD dwProcessError;                   // Error found during event processing.
        bool fFatalError;                       // Indicates a fatal error.
        bool fBaseTimestampInit;                // Whether or not the baseline timestamp is initialized.
        LONGLONG ftBaseTimestamp;               // Base timestamp.
        PTRACE_EVENT_INFO pEvtInfo;             // Pointer to a buffer that receives extra event information.
        ULONG cbEvtInfo;                        // Size of the pEvtInfo buffer.
        ThreadIdProcId th2proc;                 // Dictionary linking thread IDs to process IDs.
        EventDescEseProcessor evtDesc2EseProc;  // Dictionary linking event descriptors and ESE event processors.

    private:
        void CloseHandle_( __in_opt HANDLE* const pHandle );

    public:
        EtwEventProcessor();
        bool OpenTraceFile( _In_ PCWSTR wszFilePath );
        EtwEvent* GetNextEvent();
        void CloseTraceFile();

    public:
        static void FreeEvent( _In_ EtwEvent* const pEtwEvt );
        static DWORD WINAPI WaitForEvent( _In_ LPVOID lpParameter );
        static void WINAPI ProcessEvent( _In_ PEVENT_RECORD pEvtRecord );
};


void EtwEventProcessor::CloseHandle_( _In_ HANDLE* const pHandle )
{
    if ( *pHandle != NULL )
    {
        if ( !CloseHandle( *pHandle ) )
        {
            this->fFatalError = true;
        }

        *pHandle = NULL;
    }
}


EtwEventProcessor::EtwEventProcessor()
{
    this->hEventGo = NULL;
    this->hEventReady = NULL;
    this->hEventStop = NULL;
    this->hThreadCollect = NULL;
    this->pEtwEvt = NULL;
    this->fCollecting = false;
    this->hTrace = INVALID_PROCESSTRACE_HANDLE;
    this->dwProcessError = ERROR_SUCCESS;
    this->fFatalError = false;
    this->fBaseTimestampInit = false;
    this->ftBaseTimestamp = 0LL;
    this->pEvtInfo = NULL;
    this->cbEvtInfo = 0;

    // Build dictionary.
    for ( size_t i = 0; i < _countof( g_rgEseEvtProc ); i++ )
    {
        const StandardEseEventProcessor* const processor = &g_rgEseEvtProc[ i ];
        const ULONG ulEvtDesc = UlGetEvtDescriptorId( processor->evtDescriptor );
        this->evtDesc2EseProc[ ulEvtDesc ] = processor;
    }
}

    
bool EtwEventProcessor::OpenTraceFile( _In_ PCWSTR wszFilePath )
{
    if ( this->fCollecting || this->fFatalError )
    {
        SetLastError( ERROR_INVALID_STATE );
        return false;
    }

    bool fSucceeded = false;

    // First, try to open the trace.
    EVENT_TRACE_LOGFILEW evtTraceLogFile = { 0 };
    evtTraceLogFile.LogFileName = (LPWSTR)wszFilePath;
    evtTraceLogFile.EventRecordCallback = EtwEventProcessor::ProcessEvent;
    evtTraceLogFile.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
    evtTraceLogFile.Context = this;

    this->hTrace = OpenTraceW( &evtTraceLogFile );
    if ( this->hTrace == INVALID_PROCESSTRACE_HANDLE )
    {
        goto HandleError;
    }

    // Create events.
    this->hEventGo = CreateEventW( NULL, TRUE, FALSE, NULL );
    this->hEventReady = CreateEventW( NULL, TRUE, FALSE, NULL );
    this->hEventStop = CreateEventW( NULL, TRUE, FALSE, NULL );
    this->fBaseTimestampInit = false;
    this->ftBaseTimestamp = 0LL;

    // Create and start thread.
    this->hThreadCollect = CreateThread( NULL, 0, EtwEventProcessor::WaitForEvent, this, 0, NULL );
    if ( this->hThreadCollect == NULL )
    {
        goto HandleError;
    }

    this->fCollecting = true;

    fSucceeded = true;

HandleError:
    if ( !fSucceeded )
    {
        const DWORD dwLastError = GetLastError();
        this->CloseTraceFile();
        SetLastError( dwLastError );
    }
    
    return fSucceeded;
}


EtwEvent* EtwEventProcessor::GetNextEvent()
{
    if ( !this->fCollecting || this->fFatalError )
    {
        SetLastError( ERROR_INVALID_STATE );
        return NULL;
    }


    // Reset the acknowlegement and set the "go" signal.
    if ( !ResetEvent( this->hEventReady ) || !SetEvent( this->hEventGo ) )
    {
        this->fFatalError = true;
        return NULL;
    }

    // Wait for the "ready" event.
    const DWORD dwWaitError = WaitForSingleObject( this->hEventReady, INFINITE );
    if ( dwWaitError != WAIT_OBJECT_0 )
    {
        this->fFatalError = true;
        SetLastError( ERROR_INTERNAL_ERROR );
        return NULL;
    }

    // If the retrieved event is NULL, set last error to cached error.
    if ( this->pEtwEvt == NULL )
    {
        SetLastError( this->dwProcessError );
    }

    return this->pEtwEvt;
}


void EtwEventProcessor::CloseTraceFile()
{
    this->fCollecting = false;

    // If this was a fatal error, force the thread to terminate.
    if ( this->fFatalError )
    {
        // Kill the collection thread.
        if ( this->hThreadCollect != NULL )
        {
            (void)TerminateThread( this->hThreadCollect, ERROR_INTERNAL_ERROR );
            this->hThreadCollect = NULL;
        }

        // Set signals that other threads wait on just in case.
        if ( this->hEventGo != NULL )
        {
            (void)SetEvent( this->hEventGo );
        }
        if ( this->hEventStop != NULL )
        {
            (void)SetEvent( this->hEventStop );
        }

        // Close the handle.
        if ( this->hTrace != INVALID_PROCESSTRACE_HANDLE )
        {
            (void)CloseTrace( this->hTrace );
            this->hTrace = INVALID_PROCESSTRACE_HANDLE;
        }
    }
    else
    {
        // Try to terminate nicely.
        if ( this->hThreadCollect != NULL )
        {
            // Set events.
            if ( !SetEvent( this->hEventStop ) || !SetEvent( this->hEventGo ) )
            {
                this->fFatalError = true;
                this->CloseTraceFile();
                return;
            }

            // Wait for thread to go away (10 seconds maximum).
            const DWORD dwWaitError = WaitForSingleObject( this->hThreadCollect, 10000 );
            if ( dwWaitError != WAIT_OBJECT_0 )
            {
                this->fFatalError = true;
                this->CloseTraceFile();
                return;
            }

            // Close the handle.
            if ( this->hTrace != INVALID_PROCESSTRACE_HANDLE )
            {
                if ( !CloseTrace( this->hTrace ) )
                {
                    this->fFatalError = true;
                    this->CloseTraceFile();
                    return;
                }
                this->hTrace = INVALID_PROCESSTRACE_HANDLE;
            }
        }
    }

    // Force-closing other handles.
    this->CloseHandle_( &this->hEventGo );
    this->CloseHandle_( &this->hEventReady);
    this->CloseHandle_( &this->hEventStop );
    this->CloseHandle_( &this->hThreadCollect );
    if ( this->hTrace != INVALID_PROCESSTRACE_HANDLE )
    {
        if ( !CloseTrace( this->hTrace ) )
        {
            this->fFatalError = true;
        }
        this->hTrace = INVALID_PROCESSTRACE_HANDLE;
    }

    // Free memory.
    if ( this->pEvtInfo != NULL )
    {
        delete[] ( (BYTE*)this->pEvtInfo );
        this->pEvtInfo = NULL;
    }
    this->cbEvtInfo = 0;
}


void EtwEventProcessor::FreeEvent( _In_ EtwEvent* const pEtwEvt )
{
    if ( pEtwEvt == NULL )
    {
        return;
    }

    delete[] ( (BYTE*)pEtwEvt );
}


DWORD WINAPI EtwEventProcessor::WaitForEvent( _In_ LPVOID lpParameter )
{
    EtwEventProcessor* const pEtwEvtProc = (EtwEventProcessor*)lpParameter;

    // This call will block until processing is done.
    const DWORD dwProcessTraceReturn = ProcessTrace( &pEtwEvtProc->hTrace, 1, 0, 0 );
    const DWORD dwProcessTraceError = pEtwEvtProc->dwProcessError;
    DWORD dwNextError = ERROR_SUCCESS;
    if ( ( dwProcessTraceReturn == ERROR_SUCCESS ) && ( dwProcessTraceError == ERROR_SUCCESS ) )
    {
        dwNextError = ERROR_NO_MORE_ITEMS;
    }
    else
    {
        dwNextError = ( dwProcessTraceReturn != ERROR_SUCCESS ) ? dwProcessTraceReturn : dwProcessTraceError;
    }

    // Even once the function returns, we will continue to respond to signals from the user,
    // until we get signaled to bail out.
    bool fExitThread = false;
    while ( !fExitThread )
    {
        const HANDLE rgHandle[] = { pEtwEvtProc->hEventStop, pEtwEvtProc->hEventGo };
        const DWORD dwWaitError = WaitForMultipleObjects( _countof( rgHandle ), rgHandle, FALSE, INFINITE );
        const DWORD iHandle = (DWORD)( dwWaitError - WAIT_OBJECT_0 );

        pEtwEvtProc->pEtwEvt = NULL;
        pEtwEvtProc->dwProcessError = ERROR_SUCCESS;

        if ( iHandle == 0 )
        {
            // We got signaled to stop.
            fExitThread = true;
        }
        else if ( iHandle == 1 )
        {
            // We got signaled to collect.
            pEtwEvtProc->dwProcessError = dwNextError;
            if ( !ResetEvent( pEtwEvtProc->hEventGo ) || !SetEvent( pEtwEvtProc->hEventReady ) )
            {
                pEtwEvtProc->dwProcessError = GetLastError();
                fExitThread = true;
                pEtwEvtProc->fFatalError = true;
            }
        }
        else
        {
            pEtwEvtProc->dwProcessError = ERROR_INTERNAL_ERROR;
            fExitThread = true;
            pEtwEvtProc->fFatalError = true;
        }
    }

    return ERROR_SUCCESS;
}


void WINAPI EtwEventProcessor::ProcessEvent( _In_ PEVENT_RECORD pEvtRecord )
{
    EtwEventProcessor* const pEtwEvtProc = (EtwEventProcessor*)pEvtRecord->UserContext;

    // First event is always the trace header. Extract the initial timestamp from there.
    if ( !pEtwEvtProc->fBaseTimestampInit )
    {
        if ( IsEqualGUID( pEvtRecord->EventHeader.ProviderId, EventTraceGuid ) &&
            ( pEvtRecord->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO ) )
        {
            pEtwEvtProc->ftBaseTimestamp = pEvtRecord->EventHeader.TimeStamp.QuadPart;
            pEtwEvtProc->fBaseTimestampInit = true;
        }

        return;
    }

    DWORD dwErrorCode = ERROR_SUCCESS;

    // Now that the baseline timestamp is initialized, wait for the "go" event.
    const HANDLE rgHandle[] = { pEtwEvtProc->hEventStop, pEtwEvtProc->hEventGo };
    const DWORD dwWaitError = WaitForMultipleObjects( _countof( rgHandle ), rgHandle, FALSE, INFINITE );
    const DWORD iHandle = (DWORD)( dwWaitError - WAIT_OBJECT_0 );

    pEtwEvtProc->pEtwEvt = NULL;
    pEtwEvtProc->dwProcessError = ERROR_SUCCESS;

    // If not the "go" event.
    if ( iHandle != 1 )
    {
        // We got signaled to stop.
        if ( iHandle == 0 )
        {
            dwErrorCode = ERROR_NO_MORE_ITEMS;
        }
        else
        {
            dwErrorCode = ERROR_INTERNAL_ERROR;
            pEtwEvtProc->fFatalError = true;
        }

        goto HandleError;
    }

    // At this point, we know that "go" is signaled.

    // Get extra event information.
    DWORD cbEvtInfo = pEtwEvtProc->cbEvtInfo;
    dwErrorCode = TdhGetEventInformation( pEvtRecord, 0, NULL, pEtwEvtProc->pEvtInfo, &cbEvtInfo );
    if ( dwErrorCode == ERROR_INSUFFICIENT_BUFFER )
    {
        if ( pEtwEvtProc->pEvtInfo != NULL )
        {
            delete[] ( (BYTE*)pEtwEvtProc->pEvtInfo );
            pEtwEvtProc->pEvtInfo = NULL;
            pEtwEvtProc->cbEvtInfo = 0;
        }
        
        dwErrorCode = ERROR_SUCCESS;
        pEtwEvtProc->pEvtInfo = (PTRACE_EVENT_INFO)( new BYTE[cbEvtInfo] );
        if ( pEtwEvtProc->pEvtInfo == NULL )
        {
            dwErrorCode = ERROR_OUTOFMEMORY;
        }
        else
        {
            pEtwEvtProc->cbEvtInfo = cbEvtInfo;
            dwErrorCode = TdhGetEventInformation( pEvtRecord, 0, NULL, pEtwEvtProc->pEvtInfo, &cbEvtInfo );
        }
    }

    // There could be problems with some event schemas, so swallow any possible error here.
    if ( dwErrorCode != ERROR_SUCCESS )
    {
        dwErrorCode = ERROR_SUCCESS;
        goto HandleError;
    }

    TRACE_EVENT_INFO* const pEvtInfo = pEtwEvtProc->pEvtInfo;

    // At this point, we have everything we need to process the event.

    const LONGLONG dftTimestamp = pEvtRecord->EventHeader.TimeStamp.QuadPart - pEtwEvtProc->ftBaseTimestamp;
    const ULONG ulProcessId = pEvtRecord->EventHeader.ProcessId;
    const ULONG ulThreadId = pEvtRecord->EventHeader.ThreadId;
    const BYTE* const pbUserData = (BYTE*)pEvtRecord->UserData;
    const USHORT cbUserData = pEvtRecord->UserDataLength;

    // Determine whether the event is defined by a MOF class, in an
    // instrumentation manifest, or a WPP template; to use TDH to decode
    // the event, it must be defined by one of these three sources.
    if ( pEvtInfo->DecodingSource == DecodingSourceWbem )
    {
        // MOF class.
        
        // Kernel events.
        if ( IsEqualGUID( pEvtInfo->ProviderGuid, SystemTraceControlGuid ) )
        {
            // Therad_V2 events.
            if ( IsEqualGUID( pEvtInfo->EventGuid, MSNT_SystemTrace_Thread_V2 ) )
            {
                switch ( pEvtRecord->EventHeader.EventDescriptor.Opcode )
                {
                    case MSNT_SystemTrace_Thread_V2_CSwitch:
                    {
                        if ( sizeof(ContextSwitch) > cbUserData )
                        {
                            dwErrorCode = ERROR_INTERNAL_ERROR;
                            goto HandleError;
                        }

                        const ULONG ibExtraData = sizeof(EtwEvent);
                        const ULONG cbExtraData = sizeof(KernelCSwitch);
                        const ULONG cbAlloc = ibExtraData + cbExtraData;

                        EtwCollectionAlloc( pEtwEvtProc->pEtwEvt = (EtwEvent*)( new BYTE[cbAlloc] ) );

                        const ContextSwitch* const pCSwitch = (ContextSwitch*)pbUserData;
                        const DWORD dwNewThreadId = pCSwitch->NewThreadId;
                        const DWORD dwOldThreadId = pCSwitch->OldThreadId;
                        const EtwEventProcessor::ThreadIdProcId::const_iterator keyValueNew = pEtwEvtProc->th2proc.find( dwNewThreadId );
                        const EtwEventProcessor::ThreadIdProcId::const_iterator keyValueOld = pEtwEvtProc->th2proc.find( dwOldThreadId );
                        const DWORD dwNewProcessId = ( keyValueNew != pEtwEvtProc->th2proc.end() ) ? keyValueNew->second : (ULONG)-1;
                        const DWORD dwOldProcessId = ( keyValueOld != pEtwEvtProc->th2proc.end() ) ? keyValueOld->second : (ULONG)-1;

                        EtwEvent* const pEtwEvt = pEtwEvtProc->pEtwEvt;
                        pEtwEvt->etwEvtType = etwevttKernelCSwitch;
                        pEtwEvt->timestamp = dftTimestamp;
                        pEtwEvt->ibExtraData = ibExtraData;
                        pEtwEvt->cbExtraData = cbExtraData;

                        KernelCSwitch* const pKCSwitch = (KernelCSwitch*)( (BYTE*)pEtwEvt + ibExtraData );
                        pKCSwitch->ulProcessId = dwNewProcessId;
                        pKCSwitch->ulThreadId = dwNewThreadId;
                        pKCSwitch->ulOldProcessId = dwOldProcessId;
                        pKCSwitch->ulOldThreadId = dwOldThreadId;
                        pKCSwitch->thstOldThread = (ThreadState)pCSwitch->OldThreadState;
                    }
                    break;

                    case MSNT_SystemTrace_Thread_V2_TStart:
                    case MSNT_SystemTrace_Thread_V2_TDCStart:
                    {
                        if ( sizeof(ThreadTypeGroup1) > cbUserData )
                        {
                            dwErrorCode = ERROR_INTERNAL_ERROR;
                            goto HandleError;
                        }
                        
                        const ThreadTypeGroup1* const pthtypg1 = (ThreadTypeGroup1*)pbUserData;
                        pEtwEvtProc->th2proc[ pthtypg1->ThreadId ] = pthtypg1->ProcessId;
                    }
                    break;

                    case MSNT_SystemTrace_Thread_V2_TEnd:
                    case MSNT_SystemTrace_Thread_V2_TDCEnd:
                    {
                        // Some traces have references to the thread even after a TEnd gets traced.
                        // Keep the dictionary entry intact.
                        // pEtwEvtProc->th2proc.erase( ... );
                    }
                    break;
                }
            }
        }
    }
    else if ( pEvtInfo->DecodingSource == DecodingSourceXMLFile ) 
    {
        // Instrumentation manifest.
        
        // ESE/ESENT events.
        if ( IsEqualGUID( pEvtInfo->ProviderGuid, MakeEseEtwSymbolName( ESE ) ) )
        {
            // Try and find matching processor.
            const ULONG ulEvtDesc = UlGetEvtDescriptorId( &pEvtInfo->EventDescriptor );
            const EtwEventProcessor::EventDescEseProcessor::const_iterator eseProcessorIter = pEtwEvtProc->evtDesc2EseProc.find( ulEvtDesc );
            if ( eseProcessorIter == pEtwEvtProc->evtDesc2EseProc.end() )
            {
                goto HandleError;
            }

            // Found a processor.
            const StandardEseEventProcessor* const eseProcessor = eseProcessorIter->second;
            const ULONG cbExtraData =
                    eseProcessor->pfnCbExtraDataCallback ?
                    eseProcessor->pfnCbExtraDataCallback( pbUserData, cbUserData ) :
                    0;

            EtwCollectionAlloc( pEtwEvtProc->pEtwEvt = AllocStandardEseEvent(
                eseProcessor->etwEvtType,
                eseProcessor->cbStructDefault,
                dftTimestamp,
                ulProcessId,
                ulThreadId,
                cbExtraData ) );

            // Extra processing.
            if ( eseProcessor->pfnExtraDataProcessingCallback )
            {
                EventHeader* const pEvtHeader = (EventHeader*)( (BYTE*)pEtwEvtProc->pEtwEvt + pEtwEvtProc->pEtwEvt->ibExtraData );
                BYTE* const pbCustomEventData = (BYTE*)pEvtHeader + sizeof(EventHeader);
            
                EtwCollectionCall(
                    eseProcessor->pfnExtraDataProcessingCallback(
                        pEvtHeader,
                        pbUserData,
                        cbUserData ) );
            }
        }
    }
    else
    {
        // Not handling the WPP case.
        goto HandleError;
    }

HandleError:
    // Should we close the trace file?
    if ( dwErrorCode != ERROR_SUCCESS )
    {
        if ( CloseTrace( pEtwEvtProc->hTrace ) )
        {
            pEtwEvtProc->hTrace = INVALID_PROCESSTRACE_HANDLE;
        }
        else
        {
            pEtwEvtProc->fFatalError = true;
        }

        EtwEventProcessor::FreeEvent( pEtwEvtProc->pEtwEvt );
        pEtwEvtProc->pEtwEvt = NULL;
        pEtwEvtProc->dwProcessError = dwErrorCode;
    }
    else if ( pEtwEvtProc->pEtwEvt != NULL )
    {
        if ( !ResetEvent( pEtwEvtProc->hEventGo ) || !SetEvent( pEtwEvtProc->hEventReady ) )
        {
            dwErrorCode = ERROR_INTERNAL_ERROR;
            pEtwEvtProc->fFatalError = true;
            goto HandleError;
        }
    }
}


// API implementation.

HANDLE EtwOpenTraceFile( _In_ PCWSTR wszFilePath )
{
    EtwEventProcessor* pEvtProc = new EtwEventProcessor();

    if ( pEvtProc == NULL )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        goto HandleError;
    }

    if ( !pEvtProc->OpenTraceFile( wszFilePath ) )
    {
        goto HandleError;
    }

    
    return (HANDLE)pEvtProc;

HandleError:

    delete pEvtProc;
    return NULL;
}


EtwEvent* EtwGetNextEvent( _In_ HANDLE handle )
{
    EtwEventProcessor* const pEvtProc = (EtwEventProcessor*)handle;
    EtwEvent* pEtwEvent = NULL;

    if ( pEvtProc == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto HandleError;
    }

    pEtwEvent = (EtwEvent*)pEvtProc->GetNextEvent();

HandleError:
    return pEtwEvent;
}


VOID EtwFreeEvent( _In_ EtwEvent* const pEtwEvt )
{
    EtwEventProcessor::FreeEvent( pEtwEvt );
}


VOID EtwCloseTraceFile( _In_ HANDLE handle )
{
    EtwEventProcessor* const pEvtProc = (EtwEventProcessor*)handle;

    if ( pEvtProc == NULL )
    {
        return;
    }

    pEvtProc->CloseTraceFile();
    delete pEvtProc;
}

