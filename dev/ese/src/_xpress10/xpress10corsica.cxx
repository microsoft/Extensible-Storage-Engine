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

#include "osu.hxx"
#include "jet.h"

// Number of compression-encrytion and decompression-decryption queues that the current hardware
// revision supports. Given that our payloads are at most 8k, unclear if we actually need 8 queues
// per process for optimal performance, but that is what we are going with right now.
constexpr ULONGLONG NUM_CE_QUEUES = 8;
constexpr ULONGLONG NUM_DD_QUEUES = 8;

struct CORSICA_WRAPPER : public CZeroInit
{
    CORSICA_WRAPPER()
      : CZeroInit( sizeof( CORSICA_WRAPPER ) )
    {}

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
};

PCWSTR ReasonToString[] =
{
    L"UnknownFailure",

    L"MemoryAllocationFailure",
    L"CorsicaLibraryInitializeFailed",
    L"CorsicaDeviceEnumeratorCreateFailed",
    L"CorsicaDeviceEnumeratorGetCountFailed",
    L"TooManyOrTooFewCorsicaDevicesDetected",
    L"CorsicaDeviceEnumeratorGetItemFailed",
    L"CorsicaDeviceOpenFailed",
    L"CorsicaDeviceQueryInformationFailed",
    L"CorsicaDeviceIsInMaintenanceMode",
    L"CorsicaDeviceQueryUserResourcesFailed",
    L"TooManyOrTooFewQueueGroupsDetected",
    L"CorsicaChannelCreateFailed",
    L"CorsicaChannelAddResourceFailed",
    L"CorsicaChannelFinalizeFailed",

    L"CorsicaRequestQueryAuxiliaryBufferSizeFailed",
    L"EngineAuxBufferNotInExpectedFormat",
    L"UserAuxBufferSizeNotInExpectedFormat",
    L"CorsicaRequestGetBackingBufferSizeFailed",
    L"RequestContextSizeTooBig",
    L"CorsicaRequestInitializeFailed",
    L"CorsicaRequestGetResponsePointerFailed",
    L"CorsicaRequestSetBuffersFailed",
    L"CorsicaRequestExecuteFailed",
    L"EngineExecutionError",
};

BOOL FXpress10CorsicaHealthy()
{
    return g_pWrapper != NULL && g_pWrapper->m_fHealthy;
}

VOID Xpress10CorsicaTerm()
{
    delete g_pWrapper;
    g_pWrapper = NULL;
}

CORSICA_STATUS ChannelRegister( ULONG ChannelId, CORSICA_FAILURE_REASON *preason )
{
    CORSICA_STATUS status;
    CORSICA_CHANNEL_ADD_RESOURCE_INPUT addResource;
    CORSICA_CHANNEL_ADD_RESOURCE_OUTPUT addResourceOut;

    status = CorsicaChannelCreate( g_pWrapper->m_hDevice, ChannelId, 0 /* ChannelCreateFlags */, &g_pWrapper->m_hChannel );
    if (FAILED(status))
    {
        *preason = CorsicaChannelCreateFailed;
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
            *preason = CorsicaChannelAddResourceFailed;
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
            *preason = CorsicaChannelAddResourceFailed;
            goto HandleError;
        }

        g_pWrapper->m_rgusDefaultDdQueueId[i] = addResourceOut.Queue.QueueId;
    }

    status = CorsicaChannelFinalize( g_pWrapper->m_hChannel );
    if (FAILED(status))
    {
        *preason = CorsicaChannelFinalizeFailed;
        goto HandleError;
    }

    return status;

HandleError:
    if (g_pWrapper->m_hChannel != NULL)
    {
        CorsicaChannelDelete(g_pWrapper->m_hChannel);
        g_pWrapper->m_hChannel = NULL;
    }

    return status;
}


ERR ErrXpress10CorsicaInit()
{
    ERR err = ErrERRCheck( JET_errDeviceFailure );
    CORSICA_STATUS status = 0;
    CORSICA_FAILURE_REASON reason = FailureReasonUnknown;
    ULONG count;
    CORSICA_HANDLE hDeviceEnum = NULL;
    CORSICA_HANDLE hLibrary = NULL;
    CORSICA_DEVICE_SETUP_INFORMATION deviceSetupInfo;
    CORSICA_DEVICE_USER_RESOURCE_INFORMATION userResources;
    CORSICA_DEVICE_INFORMATION deviceInformation;

    g_pWrapper = new CORSICA_WRAPPER;
    if ( g_pWrapper == NULL )
    {
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
        reason = TooManyOrTooFewQueueGroupsDetected;
        goto HandleError;
    }

    ULONG attempt = 0;
    do
    {
        // Temp fix for channel-id conflict between multiple processes to try random channels,
        // corsica team working on a better API to checkout channel.
        ULONG ChannelId = rand() % userResources.QueueGroupCount;
        status = ChannelRegister( ChannelId, &reason );
        attempt++;
    }
    while ( status == HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES) && attempt < 10 );
    if (FAILED(status))
    {
        goto HandleError;
    }

    g_pWrapper->m_fHealthy = fTrue;
    err = JET_errSuccess;

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

    if ( err < JET_errSuccess )
    {
        WCHAR wszStatus[16];
        PCWSTR rgwsz[2];
        OSStrCbFormatW( wszStatus, sizeof(wszStatus), L"%d", status );
        rgwsz[0] = wszStatus;
        rgwsz[1] = ReasonToString[ reason ];
        UtilReportEvent(
            eventWarning,
            GENERAL_CATEGORY,
            CORSICA_INIT_FAILED_ID,
            2,
            rgwsz );

        Xpress10CorsicaTerm();
    }
    return err;
}

// Temp: error code for when output buffer is not big enough to hold the output
// Adding until Corsica folks add it to their redist header
#define CORSICA_BUFFER_OVERRUN 192

LOCAL ERR InternalCorsicaRequest(
    CORSICA_REQUEST_PARAMETERS* pRequestParameters,
    CORSICA_REQUEST_BUFFERS* pRequestBuffers,
    ULONG* pEngineOutputCb,
    QWORD* pdhrtHardwareLatency )
{
    ERR err = ErrERRCheck( JET_errDeviceFailure );
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
        reason = EngineAuxBufferNotInExpectedFormat;
        goto HandleError;
    }

    if ( userAuxBufferSize != 0 )
    {
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
        status = pEngineResponsePointer->StatusCode;
        reason = EngineExecutionError;
        goto HandleError;
    }

    *pEngineOutputCb = pEngineResponsePointer->OutputDataCb;
    *pdhrtHardwareLatency = pEngineResponsePointer->Latency;

    err = JET_errSuccess;

 HandleError:
    if ( hRequest != NULL )
    {
        CorsicaRequestUninitialize( hRequest );
        hRequest = NULL;
    }

    if ( err < JET_errSuccess &&
         // compression can report buffer overrun for uncompressible input, no need to event for that.
         ( status != CORSICA_BUFFER_OVERRUN || pRequestParameters->DecryptDecompress ) )
    {
        if ( reason == EngineExecutionError && pEngineResponsePointer->EngineErrorCode != 0 )
        {
            g_pWrapper->m_fHealthy = fFalse;
        }

        WCHAR wszStatus[16], wszEngineStatus[16];
        PCWSTR rgwsz[4];
        OSStrCbFormatW( wszStatus, sizeof(wszStatus), L"%d", status );
        rgwsz[0] = wszStatus;
        OSStrCbFormatW( wszEngineStatus, sizeof(wszEngineStatus), L"%d", pEngineResponsePointer ? pEngineResponsePointer->EngineErrorCode : -1 );
        rgwsz[1] = wszEngineStatus;
        rgwsz[2] = ReasonToString[ reason ];
        rgwsz[3] = pRequestParameters->DecryptDecompress ? L"Decompress" : L"Compress";
        UtilReportEvent(
            eventWarning,
            GENERAL_CATEGORY,
            CORSICA_REQUEST_FAILED_ID,
            _countof(rgwsz),
            rgwsz );
    }

    return err;
}

ERR ErrXpress10CorsicaCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
    _Out_ PULONGLONG pCrc,
    _Out_ QWORD *pdhrtHardwareLatency
)
{
    ERR err = JET_errSuccess;

    CORSICA_REQUEST_PARAMETERS requestParameters;
    CORSICA_REQUEST_PARAMETERS_INIT(&requestParameters);
    // Just round-robin the queues rather than keeping track and trying to find an empty queue.
    // Maybe if we started compressing/decompressing larger payload, we may need a different allocation mechanism.
    requestParameters.QueueId = g_pWrapper->m_rgusDefaultCeQueueId[ AtomicIncrement( &g_pWrapper->m_lLastUsedCeQueueId ) % NUM_CE_QUEUES ];
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

    Call( InternalCorsicaRequest(
                &requestParameters,
                &requestBuffers,
                FinalCompressedSize,
                pdhrtHardwareLatency ) );
    *pCrc = auxFrame.CipherDataChecksum;

HandleError:
    if ( err < JET_errSuccess )
    {
        err = errRECCannotCompress;
    }
    return err;
}

ERR ErrXpress10CorsicaDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONGLONG Crc,
    _Out_ PULONG FinalUncompressedSize,
    _Out_ QWORD *pdhrtHardwareLatency
)
{
    ERR err = JET_errSuccess;

    CORSICA_REQUEST_PARAMETERS requestParameters;
    CORSICA_REQUEST_PARAMETERS_INIT(&requestParameters);
    // Just round-robin the queues rather than keeping track and trying to find an empty queue.
    // Maybe if we started compressing/decompressing larger payload, we may need a different allocation mechanism.
    requestParameters.QueueId = g_pWrapper->m_rgusDefaultDdQueueId[ AtomicIncrement( &g_pWrapper->m_lLastUsedDdQueueId ) % NUM_DD_QUEUES ];
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

    err = InternalCorsicaRequest(
            &requestParameters,
            &requestBuffers,
            FinalUncompressedSize,
            pdhrtHardwareLatency );

    return err;
}

