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
#include <winnt.h>
#include "NTSecAPI.h" // for NTSTATUS typedef

#include "xp10.h"

#define UNCOMPRESSED_CHUNK_SIZE 4096

#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)

// CRC64 implementation compatible with what Corsica generates
#define CORSICA_CRC64_POLY 0x9a6c9329ac4bc9b5ULL
static ULONGLONG
UtilGenCorsicaCrc64(
    _In_reads_bytes_(BufferLength) const BYTE * Buffer,
    _In_ ULONGLONG BufferLength
    )
{
    ULONGLONG crc = 0xFFFFFFFFFFFFFFFFULL;
    for (ULONGLONG i = 0; i < BufferLength; i++) 
    {
        crc ^= Buffer[i];
        for (int j = 0; j < 8; j++) 
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ CORSICA_CRC64_POLY;
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }
    return (crc ^ 0xFFFFFFFFFFFFFFFFULL);
}

HRESULT Xpress10SoftwareCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
	_Out_ ULONGLONG *pCrc
)
{
    ULONG cbCompressionWorkspaceSize, cbDecompressionWorkspaceSize;

    RtlCompressWorkSpaceSizeXp10(
            COMPRESSION_ENGINE_STANDARD,
            &cbCompressionWorkspaceSize,
            &cbDecompressionWorkspaceSize );

    BYTE *workspace = new BYTE[cbCompressionWorkspaceSize];
    if ( workspace == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    NTSTATUS status = RtlCompressBufferXp10(
                            COMPRESSION_ENGINE_STANDARD,
                            (BYTE *)UncompressedBuffer,
                            UncompressedBufferSize,
                            CompressedBuffer,
                            CompressedBufferSize,
                            UNCOMPRESSED_CHUNK_SIZE,
                            FinalCompressedSize,
                            workspace );
    if ( status == STATUS_SUCCESS )
    {
        *pCrc = UtilGenCorsicaCrc64( CompressedBuffer, *FinalCompressedSize );
    }

    delete[] workspace;
    return status;
}

HRESULT Xpress10SoftwareDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
#ifdef DEBUG
    _In_ ULONGLONG Crc,
#else
    _In_ ULONGLONG,
#endif
    _Out_ PULONG FinalUncompressedSize
)
{
    ULONG cbCompressionWorkspaceSize, cbDecompressionWorkspaceSize;

#ifdef DEBUG
    // Both the xp10 library and our decompression code is doing verification of uncompressed
    // data's CRC, no need to verify compressed data's CRC. Just leave it in for debug.
    if ( Crc != UtilGenCorsicaCrc64( CompressedBuffer, CompressedBufferSize ) )
    {
        return HRESULT_FROM_WIN32( ERROR_DATA_CHECKSUM_ERROR );
    }
#endif

    RtlCompressWorkSpaceSizeXp10(
            COMPRESSION_ENGINE_STANDARD,
            &cbCompressionWorkspaceSize,
            &cbDecompressionWorkspaceSize );

    BYTE *workspace = new BYTE[cbDecompressionWorkspaceSize];
    if ( workspace == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    NTSTATUS status = RtlDecompressBufferXp10(
                        UncompressedBuffer,
                        UncompressedBufferSize,
                        (BYTE *)CompressedBuffer,
                        CompressedBufferSize,
                        UNCOMPRESSED_CHUNK_SIZE,
                        FinalUncompressedSize,
                        workspace );

    delete[] workspace;
    return status;
}

