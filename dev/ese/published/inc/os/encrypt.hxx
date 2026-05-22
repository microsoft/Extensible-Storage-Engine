// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_ENCRYPT_HXX_INCLUDED
#define _OS_ENCRYPT_HXX_INCLUDED

ULONG
Crc32Checksum(
    _In_reads_bytes_( cbData )  const   BYTE *pbData,
    _In_                        ULONG cbData );

ULONG CbOSEncryptAes256SizeNeeded( ULONG cbDataLen );

enum AES256_IMPLEMENTATION
{
    AES256_CAPI_IMPLEMENTATION,
    AES256_CNG_IMPLEMENTATION
};

ERR
ErrOSCreateAes256Key(
    _In_                                                AES256_IMPLEMENTATION impl,
    _Out_writes_bytes_to_opt_(*pcbKeySize, *pcbKeySize) BYTE *pbKey,
    _Inout_                                             ULONG *pcbKeySize );

ERR
ErrOSEncryptionVerifyKey(
    _In_                                                AES256_IMPLEMENTATION impl,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey );

// Encrypt using AES256 encryption in CBC mode with PKCS5 padding.
// Also, there is initial padding for checksum, InitVector in the output data.
// Use CbOSEncryptAes256SizeNeeded above to figure out how big the output buffer needs to be.
ERR
ErrOSEncryptWithAes256(
    _In_                                                AES256_IMPLEMENTATION impl,
    _Inout_updates_bytes_to_(cbDataBufLen, *pcbDataLen) BYTE *pbData,
    _Inout_                                             ULONG *pcbDataLen,
    _In_                                                ULONG cbDataBufLen,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey );

ERR
ErrOSDecryptWithAes256(
    _In_                                                AES256_IMPLEMENTATION impl,
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey );

#endif // _OS_ENCRYPT_HXX_INCLUDED

