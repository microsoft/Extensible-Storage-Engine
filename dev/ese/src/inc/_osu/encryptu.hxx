// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "encrypt.hxx"
#include "perfctrs.hxx"

ERR
ErrOSUEncrypt(
    _Inout_updates_bytes_to_(cbDataBufLen, *pcbDataLen)     BYTE *pbData,
    _Inout_                                                 ULONG *pcbDataLen,
    _In_                                                    ULONG cbDataBufLen,
    _In_reads_bytes_(cbKey)                         const   BYTE *pbKey,
    _In_                                                    ULONG cbKey,
    _In_                                            const   INT iInstance,
    _In_                                            const   TCE tce );

ERR
ErrOSUDecrypt(
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey,
    _In_                                        const   INT iInstance,
    _In_                                        const   TCE tce );

