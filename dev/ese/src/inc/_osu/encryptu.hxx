// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "encrypt.hxx"
#include "perfctrs.hxx"

struct FUCB;

ERR
ErrOSUEncrypt(
    _Inout_updates_bytes_to_(cbDataBufLen, *pcbDataLen)     BYTE *pbData,
    _Inout_                                                 ULONG *pcbDataLen,
    _In_                                                    ULONG cbDataBufLen,
    _In_                                            const   FUCB *pfucb,
    _In_                                            const   FUCB *pfucbTable = NULL );

ERR
ErrOSUDecrypt(
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_                                        const   FUCB *pfucb,
    _In_                                        const   FUCB *pfucbTable = NULL );

