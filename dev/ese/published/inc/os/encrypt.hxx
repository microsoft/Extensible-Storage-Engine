// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_ENCRYPT_HXX_INCLUDED
#define _OS_ENCRYPT_HXX_INCLUDED

ULONG
Crc32Checksum(
    _In_reads_bytes_( cbData )  const   BYTE *pbData,
    _In_                        ULONG cbData );

ERR
ErrOSCreateAes256Key(
    _Out_writes_bytes_to_opt_(*pcbKeySize, *pcbKeySize) BYTE *pbKey,
    _Inout_                                             ULONG *pcbKeySize );

ULONG CbOSEncryptAes256SizeNeeded( ULONG cbDataLen );

ERR ErrOSEncryptionVerifyKey(
    _In_reads_bytes_(cbKey)                         const   BYTE *pbKey,
    _In_                                                    ULONG cbKey );

ERR
ErrOSEncryptWithAes256(
    _Inout_updates_bytes_to_(cbDataBufLen, *pcbDataLen)     BYTE *pbData,
    _Inout_                                                 ULONG *pcbDataLen,
    _In_                                                    ULONG cbDataBufLen,
    _In_reads_bytes_(cbKey)                         const   BYTE *pbKey,
    _In_                                                    ULONG cbKey );

ERR
ErrOSDecryptWithAes256(
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey );

#endif

