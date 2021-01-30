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
#endif

#include <windows.h>
#include <winnt.h>
#include "NTSecAPI.h"

#include "xp10.h"

#include "osu.hxx"
#include "jet.h"

#define UNCOMPRESSED_CHUNK_SIZE 4096

#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)

#define CORSICA_CRC64_POLY 0x9a6c9329ac4bc9b5ULL
LOCAL ULONGLONG
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

ERR ErrXpress10SoftwareCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
	_Out_ ULONGLONG *pCrc
)
{
    ERR err = JET_errSuccess;
    ULONG cbCompressionWorkspaceSize, cbDecompressionWorkspaceSize;

    RtlCompressWorkSpaceSizeXp10(
            COMPRESSION_ENGINE_STANDARD,
            &cbCompressionWorkspaceSize,
            &cbDecompressionWorkspaceSize );

    BYTE *workspace = NULL;
    AllocR( workspace = (BYTE *)PvOSMemoryHeapAlloc( cbCompressionWorkspaceSize ));

    NTSTATUS status = RtlCompressBufferXp10(
                            COMPRESSION_ENGINE_STANDARD,
                            (BYTE *)UncompressedBuffer,
                            UncompressedBufferSize,
                            CompressedBuffer,
                            CompressedBufferSize,
                            UNCOMPRESSED_CHUNK_SIZE,
                            FinalCompressedSize,
                            workspace );
    if ( status != STATUS_SUCCESS )
    {
        err = ErrERRCheck( errRECCannotCompress );
    }
    else
    {
        Assert( *FinalCompressedSize <= CompressedBufferSize );
        *pCrc = UtilGenCorsicaCrc64( CompressedBuffer, *FinalCompressedSize );
    }

    OSMemoryHeapFree( workspace );
    return err;
}

ERR ErrXpress10SoftwareDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONGLONG Crc,
    _Out_ PULONG FinalUncompressedSize
)
{
    ERR err = JET_errSuccess;
    ULONG cbCompressionWorkspaceSize, cbDecompressionWorkspaceSize;

#ifdef DEBUG
    if ( Crc != UtilGenCorsicaCrc64( CompressedBuffer, CompressedBufferSize ) )
    {
        return ErrERRCheck( JET_errCompressionIntegrityCheckFailed );
    }
#endif

    RtlCompressWorkSpaceSizeXp10(
            COMPRESSION_ENGINE_STANDARD,
            &cbCompressionWorkspaceSize,
            &cbDecompressionWorkspaceSize );

    BYTE *workspace = NULL;
    AllocR( workspace = (BYTE *)PvOSMemoryHeapAlloc( cbDecompressionWorkspaceSize ));

    NTSTATUS status = RtlDecompressBufferXp10(
                        UncompressedBuffer,
                        UncompressedBufferSize,
                        (BYTE *)CompressedBuffer,
                        CompressedBufferSize,
                        UNCOMPRESSED_CHUNK_SIZE,
                        FinalUncompressedSize,
                        workspace );
    if ( status != STATUS_SUCCESS )
    {
        err = ErrERRCheck( JET_errDecompressionFailed );
    }
    else
    {
        Assert( *FinalUncompressedSize <= UncompressedBufferSize );
    }

    OSMemoryHeapFree( workspace );
    return err;
}

