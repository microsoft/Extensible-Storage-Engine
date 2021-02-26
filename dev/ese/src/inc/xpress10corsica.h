// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

enum CORSICA_FAILURE_REASON
{
    FailureReasonUnknown = 0,

    MemoryAllocationFailure,
    CorsicaLibraryInitializeFailed,
    CorsicaDeviceEnumeratorCreateFailed,
    CorsicaDeviceEnumeratorGetCountFailed,
    TooManyOrTooFewCorsicaDevicesDetected,
    CorsicaDeviceEnumeratorGetItemFailed,
    CorsicaDeviceOpenFailed,
    CorsicaDeviceQueryInformationFailed,
    CorsicaDeviceIsInMaintenanceMode,
    CorsicaDeviceQueryUserResourcesFailed,
    TooManyOrTooFewQueueGroupsDetected,
    CorsicaChannelCreateFailed,
    CorsicaChannelAddResourceFailed,
    CorsicaChannelFinalizeFailed,

    CorsicaRequestQueryAuxiliaryBufferSizeFailed,
    EngineAuxBufferNotInExpectedFormat,
    UserAuxBufferSizeNotInExpectedFormat,
    CorsicaRequestGetBackingBufferSizeFailed,
    RequestContextSizeTooBig,
    CorsicaRequestInitializeFailed,
    CorsicaRequestGetResponsePointerFailed,
    CorsicaRequestSetBuffersFailed,
    CorsicaRequestExecuteFailed,
    EngineExecutionError,

    CorsicaFailureReasonMax
};

HRESULT Xpress10CorsicaInit( _Out_ CORSICA_FAILURE_REASON *pReason );

VOID Xpress10CorsicaTerm();

BOOL IsXpress10CorsicaHealthy();

HRESULT Xpress10CorsicaCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
    _Out_ PULONGLONG pCrc,
    _Out_ ULONGLONG *phrtHardwareLatency,
    _Out_ CORSICA_FAILURE_REASON *pReason,
    _Out_ USHORT *pEngineErrorCode
);

HRESULT Xpress10CorsicaDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONGLONG Crc,
    _Out_ PULONG FinalUncompressedSize,
    _Out_ ULONGLONG *phrtHardwareLatency,
    _Out_ CORSICA_FAILURE_REASON *pReason,
    _Out_ USHORT *pEngineErrorCode
);

