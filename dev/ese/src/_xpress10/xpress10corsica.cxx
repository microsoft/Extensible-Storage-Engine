// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define UNICODE
#define _UNICODE

#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  //  WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "CorsicaApi.h"

#include "xpress10corsica.h"

// Number of compression-encrytion and decompression-decryption queues that the current hardware
// revision supports. Given that our payloads are at most 8k, unclear if we actually need 8 queues
// per process for optimal performance, but that is what we are going with right now.
constexpr ULONGLONG NUM_CE_QUEUES = 8;
constexpr ULONGLONG NUM_DD_QUEUES = 8;

struct CORSICA_WRAPPER
{
    CORSICA_WRAPPER()
    {
        ZeroMemory(this, sizeof(CORSICA_WRAPPER));
    }

    ~CORSICA_WRAPPER()
    {
        if (m_hChannel != NULL)
        {
            CorsicaChannelDelete(m_hChannel);
            m_hChannel = NULL;
        }

        if (m_hDevice != NULL)
        {
            CorsicaDeviceClose(m_hDevice);
            m_hDevice = NULL;
        }
    }

    // servicing encode/decode requests
    CORSICA_HANDLE m_hDevice;
    CORSICA_HANDLE m_hChannel;

    USHORT         m_rgusDefaultCeQueueId[NUM_CE_QUEUES];
    LONG           m_lLastUsedCeQueueId;
    USHORT         m_rgusDefaultDdQueueId[NUM_DD_QUEUES];
    LONG           m_lLastUsedDdQueueId;

    bool m_fHealthy;
};

CORSICA_WRAPPER *g_pWrapper = NULL;

BOOL IsXpress10CorsicaHealthy()
{
    return g_pWrapper != NULL && g_pWrapper->m_fHealthy;
}

VOID Xpress10CorsicaTerm()
{
    delete g_pWrapper;
    g_pWrapper = NULL;
}

HRESULT Xpress10CorsicaInit( _Out_ CORSICA_FAILURE_REASON *pReason )
{
    CORSICA_STATUS status = 0;
    CORSICA_FAILURE_REASON reason = FailureReasonUnknown;
    ULONG count;
    CORSICA_HANDLE hDeviceEnum = NULL;
    CORSICA_HANDLE hLibrary = NULL;
    CORSICA_DEVICE_SETUP_INFORMATION deviceSetupInfo;
    CORSICA_DEVICE_USER_RESOURCE_INFORMATION userResources;
    CORSICA_DEVICE_INFORMATION deviceInformation;
    CORSICA_CHANNEL_ADD_RESOURCE_INPUT addResource;
    CORSICA_CHANNEL_ADD_RESOURCE_OUTPUT addResourceOut;

    g_pWrapper = new CORSICA_WRAPPER;
    if ( g_pWrapper == NULL )
    {
        status = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        reason = MemoryAllocationFailure;
        goto HandleError;
    }

    g_pWrapper->m_lLastUsedCeQueueId = 0;
    g_pWrapper->m_lLastUsedDdQueueId = 0;

    __try
    {
        status = CorsicaLibraryInitialize( CORSICA_LIBRARY_VERSION_CURRENT, &hLibrary );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
    }
    if (FAILED(status))
    {
        reason = CorsicaLibraryInitializeFailed;
        goto HandleError;
    }

    status = CorsicaDeviceEnumeratorCreate( hLibrary, NULL, 0, &hDeviceEnum );
    if (FAILED(status))
    {
        reason = CorsicaDeviceEnumeratorCreateFailed;
        goto HandleError;
    }

    status = CorsicaDeviceEnumeratorGetCount( hDeviceEnum, &count );
    if (FAILED(status))
    {
        reason = CorsicaDeviceEnumeratorGetCountFailed;
        goto HandleError;
    }

    if ( count != 1 )
    {
        status = HRESULT_FROM_WIN32( ERROR_DEV_NOT_EXIST );
        reason = TooManyOrTooFewCorsicaDevicesDetected;
        goto HandleError;
    }

    CORSICA_DEVICE_SETUP_INFORMATION_INIT( &deviceSetupInfo );
    status = CorsicaDeviceEnumeratorGetItem( hDeviceEnum, 0 /* DeviceIndex */, &deviceSetupInfo );
    if (FAILED(status))
    {
        reason = CorsicaDeviceEnumeratorGetItemFailed;
        goto HandleError;
    }

    status = CorsicaDeviceOpen( hLibrary, deviceSetupInfo.DeviceId, nullptr, 0 /* DeviceOpenFlags */, &g_pWrapper->m_hDevice );
    if (FAILED(status))
    {
        reason = CorsicaDeviceOpenFailed;
        goto HandleError;
    }

    CORSICA_DEVICE_INFORMATION_INIT(&deviceInformation);
    status = CorsicaDeviceQueryInformation( g_pWrapper->m_hDevice, &deviceInformation );
    if (FAILED(status))
    {
        reason = CorsicaDeviceQueryInformationFailed;
        goto HandleError;
    }

    if ( deviceInformation.Flags.MaintenanceMode == TRUE )
    {
        status = HRESULT_FROM_WIN32( ERROR_DEVICE_IN_MAINTENANCE );
        reason = CorsicaDeviceIsInMaintenanceMode;
        goto HandleError;
    }

    CORSICA_DEVICE_USER_RESOURCE_INFORMATION_INIT(&userResources);
    status = CorsicaDeviceQueryUserResources( g_pWrapper->m_hDevice, &userResources );
    if (FAILED(status))
    {
        reason = CorsicaDeviceQueryUserResourcesFailed;
        goto HandleError;
    }

    if ( userResources.QueueGroupCount <= 1 )
    {
        status = HRESULT_FROM_WIN32( ERROR_DEVICE_NO_RESOURCES );
        reason = TooManyOrTooFewQueueGroupsDetected;
        goto HandleError;
    }

    status = CorsicaChannelCreate( g_pWrapper->m_hDevice, CORSICA_CHANNEL_CREATE_QUEUE_GROUP_ANY, 0 /* ChannelCreateFlags */, &g_pWrapper->m_hChannel );
    if (FAILED(status))
    {
        reason = CorsicaChannelCreateFailed;
        goto HandleError;
    }

    CORSICA_CHANNEL_ADD_RESOURCE_INPUT_INIT(&addResource);
    addResource.ResourceType = CorsicaChannelAddResourceTypeQueue;
    addResource.Queue.EngineClass = CorsicaEngineClassHigh;
    addResource.Queue.EngineType = CorsicaEngineTypeCE;
    addResource.Queue.Priority = CorsicaQueuePriority0;

    for (ULONG i = 0; i < NUM_CE_QUEUES; i++)
    {
        CORSICA_CHANNEL_ADD_RESOURCE_OUTPUT_INIT(&addResourceOut);
        status = CorsicaChannelAddResource( g_pWrapper->m_hChannel, &addResource, &addResourceOut );
        if (FAILED(status))
        {
            reason = CorsicaChannelAddResourceFailed;
            goto HandleError;
        }

        g_pWrapper->m_rgusDefaultCeQueueId[i] = addResourceOut.Queue.QueueId;
    }

    addResource.Queue.EngineType = CorsicaEngineTypeDD;

    for (ULONG i = 0; i < NUM_DD_QUEUES; i++)
    {
        CORSICA_CHANNEL_ADD_RESOURCE_OUTPUT_INIT(&addResourceOut);
        status = CorsicaChannelAddResource( g_pWrapper->m_hChannel, &addResource, &addResourceOut );
        if (FAILED(status))
        {
            reason = CorsicaChannelAddResourceFailed;
            goto HandleError;
        }

        g_pWrapper->m_rgusDefaultDdQueueId[i] = addResourceOut.Queue.QueueId;
    }

    status = CorsicaChannelFinalize( g_pWrapper->m_hChannel );
    if (FAILED(status))
    {
        reason = CorsicaChannelFinalizeFailed;
        goto HandleError;
    }

    g_pWrapper->m_fHealthy = true;

HandleError:
    if ( hDeviceEnum != NULL )
    {
        CorsicaDeviceEnumeratorDestroy( hDeviceEnum );
        hDeviceEnum = NULL;
    }

    if ( hLibrary != NULL )
    {
        CorsicaLibraryUninitialize( hLibrary );
        hLibrary = NULL;
    }

    *pReason = reason;
    if (FAILED(status))
    {
        Xpress10CorsicaTerm();
    }
    return status;
}

// Temp: error code for when output buffer is not big enough to hold the output
// Adding until Corsica folks add it to their redist header
#define CORSICA_BUFFER_OVERRUN 192

static CORSICA_STATUS InternalCorsicaRequest(
    CORSICA_REQUEST_PARAMETERS* pRequestParameters,
    CORSICA_REQUEST_BUFFERS* pRequestBuffers,
    ULONG* pEngineOutputCb,
    ULONGLONG* pdhrtHardwareLatency,
    CORSICA_FAILURE_REASON *pReason,
    USHORT *pEngineErrorCode
)
{
    CORSICA_FAILURE_REASON reason = FailureReasonUnknown;
    CORSICA_STATUS status = -1;
    CORSICA_HANDLE hRequest = (CORSICA_HANDLE)-1;
    PCORSICA_ENGINE_RESPONSE pEngineResponsePointer = NULL;

    size_t uRequestContextSize = 0;
    BYTE requestContext[1024];

    size_t engineAuxBufferSize;
    size_t userAuxBufferSize;

    status = CorsicaRequestQueryAuxiliaryBufferSize( g_pWrapper->m_hChannel, pRequestParameters, pRequestBuffers->SourceBufferList[0].DataSize, &userAuxBufferSize, &engineAuxBufferSize );
    if (FAILED(status))
    {
        reason = CorsicaRequestQueryAuxiliaryBufferSizeFailed;
        goto HandleError;
    }

    if ( engineAuxBufferSize != sizeof(CORSICA_AUX_FRAME_DATA_IN_PLACE_SHORT_IM) )
    {
        status = HRESULT_FROM_WIN32( ERROR_INCORRECT_SIZE );
        reason = EngineAuxBufferNotInExpectedFormat;
        goto HandleError;
    }

    if ( userAuxBufferSize != 0 )
    {
        status = HRESULT_FROM_WIN32( ERROR_INCORRECT_SIZE );
        reason = UserAuxBufferSizeNotInExpectedFormat;
        goto HandleError;
    }

    status = CorsicaRequestGetBackingBufferSize( g_pWrapper->m_hChannel, &uRequestContextSize );
    if (FAILED(status))
    {
        reason = CorsicaRequestGetBackingBufferSizeFailed;
        goto HandleError;
    }

    if ( uRequestContextSize > sizeof(requestContext) )
    {
        status = HRESULT_FROM_WIN32( ERROR_INCORRECT_SIZE );
        reason = RequestContextSizeTooBig;
        goto HandleError;
    }

    status = CorsicaRequestInitialize( g_pWrapper->m_hChannel, requestContext, uRequestContextSize, pRequestParameters, &hRequest );
    if (FAILED(status))
    {
        reason = CorsicaRequestInitializeFailed;
        goto HandleError;
    }

    status = CorsicaRequestGetResponsePointer( hRequest, &pEngineResponsePointer );
    if (FAILED(status))
    {
        reason = CorsicaRequestGetResponsePointerFailed;
        goto HandleError;
    }

    status = CorsicaRequestSetBuffers( hRequest, pRequestBuffers );
    if (FAILED(status))
    {
        reason = CorsicaRequestSetBuffersFailed;
        goto HandleError;
    }

    status = CorsicaRequestExecute( hRequest, NULL );
    if (FAILED(status))
    {
        reason = CorsicaRequestExecuteFailed;
        goto HandleError;
    }

    if ( pEngineResponsePointer->StatusCode != CORSICA_REQUEST_STATUS_SUCCESS )
    {
        if ( pEngineResponsePointer->StatusCode == CORSICA_BUFFER_OVERRUN )
        {
            status = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        else
        {
            status = HRESULT_FROM_WIN32( pEngineResponsePointer->StatusCode );
        }
        reason = EngineExecutionError;
        goto HandleError;
    }

    *pEngineOutputCb = pEngineResponsePointer->OutputDataCb;
    *pdhrtHardwareLatency = pEngineResponsePointer->Latency;

 HandleError:
    if ( hRequest != NULL )
    {
        CorsicaRequestUninitialize( hRequest );
        hRequest = NULL;
    }

    *pReason = reason;
    *pEngineErrorCode = pEngineResponsePointer ? pEngineResponsePointer->EngineErrorCode : -1;

    if ( FAILED(status) && reason == EngineExecutionError && pEngineResponsePointer->EngineErrorCode != 0 )
    {
        g_pWrapper->m_fHealthy = false;
    }

    return status;
}

HRESULT Xpress10CorsicaCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
    _Out_ PULONGLONG pCrc,
    _Out_ ULONGLONG *pdhrtHardwareLatency,
    _Out_ CORSICA_FAILURE_REASON *pReason,
    _Out_ USHORT *pEngineErrorCode
)
{
    CORSICA_REQUEST_PARAMETERS requestParameters;
    CORSICA_REQUEST_PARAMETERS_INIT(&requestParameters);
    // Just round-robin the queues rather than keeping track and trying to find an empty queue.
    // Maybe if we started compressing/decompressing larger payload, we may need a different allocation mechanism.
    requestParameters.QueueId = g_pWrapper->m_rgusDefaultCeQueueId[ InterlockedIncrement( &g_pWrapper->m_lLastUsedCeQueueId ) % NUM_CE_QUEUES ];
    requestParameters.DecryptDecompress = false;
    requestParameters.FrameParameters.FrameType = CorsicaFrameTypeNone;

    requestParameters.CompressionParameters.CompressionAlgorithm = CorsicaCompressionAlgorithmXp10;
    requestParameters.CompressionParameters.Lz77WindowSize = CorsicaCompressionLz77WindowSize64K;
    requestParameters.CompressionParameters.Lz77MinMatch = CorsicaCompressionLz77MinMatchLengthChar4;
    requestParameters.CompressionParameters.Lz77MaxSymbolLength = CorsicaCompressionLz77MaxSymbolLengthUnrestricted;
    requestParameters.CompressionParameters.Lz77DelayedWindowSelect = CorsicaCompressionLz77DelayedMatchWindowSelectChar2;
    requestParameters.CompressionParameters.Xp10PrefixMode = CorsicaCompressionXp10PrefixModeNone;
    requestParameters.CompressionParameters.Xp10CrcSize = CorsicaCompressionXp10Crc64B;

    CORSICA_REQUEST_BUFFERS requestBuffers;
    CORSICA_REQUEST_BUFFERS_INIT(&requestBuffers);

    CORSICA_REQUEST_BUFFER_ELEMENT sourceBufferElement;
    sourceBufferElement.DataPointer = (PUCHAR)UncompressedBuffer;
    sourceBufferElement.DataSize = UncompressedBufferSize;
    requestBuffers.SourceBufferList = &sourceBufferElement;
    requestBuffers.SourceBufferListCount = 1;

    CORSICA_REQUEST_BUFFER_ELEMENT destinationBufferElement;
    destinationBufferElement.DataPointer = CompressedBuffer;
    destinationBufferElement.DataSize = CompressedBufferSize;
    requestBuffers.DestinationBufferList = &destinationBufferElement;
    requestBuffers.DestinationBufferListCount = 1;

    requestBuffers.SourceAuxFrameBufferListCount = 0;

    CORSICA_AUX_FRAME_DATA_IN_PLACE_SHORT_IM auxFrame = {0};
    CORSICA_REQUEST_BUFFER_ELEMENT destinationAuxBufferElement;
    destinationAuxBufferElement.DataPointer = (PUCHAR)&auxFrame;
    destinationAuxBufferElement.DataSize = sizeof(auxFrame);
    requestBuffers.DestinationAuxFrameBufferList = &destinationAuxBufferElement;
    requestBuffers.DestinationAuxFrameBufferListCount = 1;

    CORSICA_STATUS status =
        InternalCorsicaRequest(
                &requestParameters,
                &requestBuffers,
                FinalCompressedSize,
                pdhrtHardwareLatency,
                pReason,
                pEngineErrorCode);
    *pCrc = auxFrame.CipherDataChecksum;

    return status;
}

HRESULT Xpress10CorsicaDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONGLONG Crc,
    _Out_ PULONG FinalUncompressedSize,
    _Out_ ULONGLONG *pdhrtHardwareLatency,
    _Out_ CORSICA_FAILURE_REASON *pReason,
    _Out_ USHORT *pEngineErrorCode
)
{
    CORSICA_REQUEST_PARAMETERS requestParameters;
    CORSICA_REQUEST_PARAMETERS_INIT(&requestParameters);
    // Just round-robin the queues rather than keeping track and trying to find an empty queue.
    // Maybe if we started compressing/decompressing larger payload, we may need a different allocation mechanism.
    requestParameters.QueueId = g_pWrapper->m_rgusDefaultDdQueueId[ InterlockedIncrement( &g_pWrapper->m_lLastUsedDdQueueId ) % NUM_DD_QUEUES ];
    requestParameters.DecryptDecompress = true;
    requestParameters.FrameParameters.FrameType = CorsicaFrameTypeNone;

    requestParameters.CompressionParameters.CompressionAlgorithm = CorsicaCompressionAlgorithmXp10;
    requestParameters.CompressionParameters.Lz77WindowSize = CorsicaCompressionLz77WindowSize64K;
    requestParameters.CompressionParameters.Lz77MinMatch = CorsicaCompressionLz77MinMatchLengthChar4;
    requestParameters.CompressionParameters.Lz77MaxSymbolLength = CorsicaCompressionLz77MaxSymbolLengthUnrestricted;
    requestParameters.CompressionParameters.Lz77DelayedWindowSelect = CorsicaCompressionLz77DelayedMatchWindowSelectChar2;
    requestParameters.CompressionParameters.Xp10PrefixMode = CorsicaCompressionXp10PrefixModeNone;
    requestParameters.CompressionParameters.Xp10CrcSize = CorsicaCompressionXp10Crc64B;

    CORSICA_REQUEST_BUFFERS requestBuffers;
    CORSICA_REQUEST_BUFFERS_INIT(&requestBuffers);

    CORSICA_REQUEST_BUFFER_ELEMENT sourceBufferElement;
    sourceBufferElement.DataPointer = (PUCHAR)CompressedBuffer;
    sourceBufferElement.DataSize = CompressedBufferSize;
    requestBuffers.SourceBufferList = &sourceBufferElement;
    requestBuffers.SourceBufferListCount = 1;

    CORSICA_REQUEST_BUFFER_ELEMENT destinationBufferElement;
    destinationBufferElement.DataPointer = UncompressedBuffer;
    destinationBufferElement.DataSize = UncompressedBufferSize;
    requestBuffers.DestinationBufferList = &destinationBufferElement;
    requestBuffers.DestinationBufferListCount = 1;

    CORSICA_AUX_FRAME_DATA_IN_PLACE_SHORT_IM auxFrame = {0};
    auxFrame.CipherDataChecksum = Crc;
    auxFrame.DataLength = CompressedBufferSize;

    CORSICA_REQUEST_BUFFER_ELEMENT sourceAuxBufferElement;
    sourceAuxBufferElement.DataPointer = (PUCHAR)&auxFrame;
    sourceAuxBufferElement.DataSize = sizeof(auxFrame);
    requestBuffers.SourceAuxFrameBufferList = &sourceAuxBufferElement;
    requestBuffers.SourceAuxFrameBufferListCount = 1;

    requestBuffers.DestinationAuxFrameBufferListCount = 0;

    return InternalCorsicaRequest(
            &requestParameters,
            &requestBuffers,
            FinalUncompressedSize,
            pdhrtHardwareLatency,
            pReason,
            pEngineErrorCode);
}

