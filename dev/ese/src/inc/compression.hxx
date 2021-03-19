// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//  used to control compression when setting LVs
enum CompressFlags
{
    compressInvalid = 0,
    compressNone = 0x0002,  // no compression
    compress7Bit = 0x0004,  // 7-bit ASCII/Unicode compression
    compressXpress = 0x0008,    // Xpress (legacy) compression
#ifndef ESENT
    compressXpress9 = 0x0010,
#endif
    compressXpress10 = 0x0020,
};

ERR ErrPKCompressData(
    const DATA& data,
    const CompressFlags compressFlags,
    const INST* const pinst,
    _Out_writes_bytes_to_( cbDataCompressedMax, *pcbDataCompressedActual ) BYTE * const pbDataCompressed,
    const INT cbDataCompressedMax,
    _Out_ INT * const pcbDataCompressedActual );
ERR ErrPKDecompressData(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual );
ERR ErrPKDecompressData(
    const DATA& dataCompressed,
    const IFMP ifmp,
    const PGNO pgno,
    _Out_writes_bytes_to_opt_( cbDataMax, min( cbDataMax, *pcbDataActual ) ) BYTE * const pbData,
    const INT cbDataMax,
    _Out_ INT * const pcbDataActual );

//  This function allocates memory and decompresses the data. The memory is allocated with new[]
//  so it should be freed with delete[]
ERR ErrPKAllocAndDecompressData(
    const DATA& dataCompressed,
    const FUCB* const pfucb,
    __deref_out_bcount( *pcbDataActual ) BYTE ** const ppbData,
    _Out_ INT * const pcbDataActual );

//  Compression may cache some information. Call this termination function to free it
ERR ErrPKInitCompression( const INT cbPage, const INT cbCompressMin, const INT cbCompressMax );
VOID PKTermCompression();

//  To avoid constantly allocating and freeing buffers to compress LVs and split/merge
//  pre-images during insertion, we cache some buffers. The buffers that are returned
//  are always of size g_cbPageSize

BYTE * PbPKAllocCompressionBuffer();
INT CbPKCompressionBuffer();
VOID PKFreeCompressionBuffer( _Post_ptr_invalid_ BYTE * const pb );

