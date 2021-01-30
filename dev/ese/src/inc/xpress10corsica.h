// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

ERR ErrXpress10CorsicaInit();

VOID Xpress10CorsicaTerm();

BOOL FXpress10CorsicaHealthy();

ERR ErrXpress10CorsicaCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
    _Out_ PULONGLONG pCrc,
    _Out_ QWORD *phrtHardwareLatency
);

ERR ErrXpress10CorsicaDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONGLONG Crc,
    _Out_ PULONG FinalUncompressedSize,
    _Out_ QWORD *phrtHardwareLatency
);

