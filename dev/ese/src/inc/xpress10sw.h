// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

HRESULT Xpress10SoftwareCompress(
    _In_reads_bytes_(UncompressedBufferSize) const BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalCompressedSize,
    _Out_ PULONGLONG pCrc
);

HRESULT Xpress10SoftwareDecompress(
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) BYTE * UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) const BYTE * CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONGLONG Crc,
    _Out_ PULONG FinalUncompressedSize
);

