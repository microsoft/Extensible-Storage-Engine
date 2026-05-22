// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"
#include <bcrypt.h>
// Need to include CAPI header for some CAPI constants/structs to maintain compatibility with CAPI implementation.
#include <wincrypt.h>
#include <winternl.h>

BCRYPT_ALG_HANDLE g_hBCryptAesAlg = NULL;
ULONG g_cbKeyObject = 0;

// PERSISTED
#define BLOCK_SIZE_AES256 16
#define KEY_SIZE_AES256   32

static NTOSFuncError( g_pfnRtlNtStatusToDosError, g_mwszzNtdllLibs, RtlNtStatusToDosError, oslfExpectedOnWin5x );

// Don't want to add bcrypt.lib in 200 different vcxproj's
static NTOSFuncNtStd( g_pfnBCryptOpenAlgorithmProvider, g_mwszzBCryptLib, BCryptOpenAlgorithmProvider, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptCloseAlgorithmProvider, g_mwszzBCryptLib, BCryptCloseAlgorithmProvider, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptGetProperty, g_mwszzBCryptLib, BCryptGetProperty, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptSetProperty, g_mwszzBCryptLib, BCryptSetProperty, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptGenRandom, g_mwszzBCryptLib, BCryptGenRandom, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptGenerateSymmetricKey, g_mwszzBCryptLib, BCryptGenerateSymmetricKey, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptDestroyKey, g_mwszzBCryptLib, BCryptDestroyKey, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptEncrypt, g_mwszzBCryptLib, BCryptEncrypt, oslfExpectedOnWin7 );
static NTOSFuncNtStd( g_pfnBCryptDecrypt, g_mwszzBCryptLib, BCryptDecrypt, oslfExpectedOnWin7 );

ERR
ErrOSIBCryptAESProviderInit()
{
    ERR err = JET_errSuccess;
    NTSTATUS status;

    if ( !NT_SUCCESS( status = g_pfnBCryptOpenAlgorithmProvider( &g_hBCryptAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0 )))
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )));
    }

    if ( !NT_SUCCESS( status = g_pfnBCryptSetProperty(
                                g_hBCryptAesAlg, 
                                BCRYPT_CHAINING_MODE, 
                                (PBYTE)BCRYPT_CHAIN_MODE_CBC, 
                                sizeof(BCRYPT_CHAIN_MODE_CBC), 
                                0 )))
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )));
    }

    ULONG cbData = 0;
    if( !NT_SUCCESS( status = g_pfnBCryptGetProperty(
                                g_hBCryptAesAlg, 
                                BCRYPT_OBJECT_LENGTH, 
                                (PBYTE)&g_cbKeyObject, 
                                sizeof(ULONG), 
                                &cbData, 
                                0 )))
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )));
    }
    if ( cbData != sizeof( g_cbKeyObject ) )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

    ULONG cbBlockSize = 0;
    if( !NT_SUCCESS( status = g_pfnBCryptGetProperty(
                                g_hBCryptAesAlg, 
                                BCRYPT_BLOCK_LENGTH, 
                                (PBYTE)&cbBlockSize, 
                                sizeof(ULONG), 
                                &cbData, 
                                0 )))
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )));
    }
    Assert( cbBlockSize == BLOCK_SIZE_AES256 );
    if ( cbData != sizeof( cbBlockSize ) || cbBlockSize != BLOCK_SIZE_AES256 )
    {
        Error( ErrERRCheck( JET_errInvalidParameter ) );
    }

HandleError:
    return err;
}

CInitOnce< ERR, decltype(&ErrOSIBCryptAESProviderInit) > g_BCryptInitOnce;

void
OSBCryptEncryptionTerm()
{
    if ( g_hBCryptAesAlg != NULL )
    {
        g_pfnBCryptCloseAlgorithmProvider( g_hBCryptAesAlg, 0 );
        g_hBCryptAesAlg = NULL;
    }
    g_BCryptInitOnce.Reset();
}

// Use an expanded structure that is identical to the CAPI exported key
#include <pshpack1.h>
// PERSISTED
struct AES256KEYEXPANDED
{
    BYTE                            Version;
    UnalignedLittleEndian<ULONG>    Checksum;
    PUBLICKEYSTRUC                  blobHeader;
    UnalignedLittleEndian<ULONG>    keySize;
    BYTE                            pbKey[KEY_SIZE_AES256];
};

ERR ErrOSBCryptEncryptionVerifyKey(
        _In_reads_bytes_(cbKey)                         const   BYTE *pbKey,
        _In_                                                    ULONG cbKey )
{
    AES256KEYEXPANDED *pKey = (AES256KEYEXPANDED *)pbKey;

    if ( cbKey != sizeof(AES256KEYEXPANDED) ||
         pKey->Version != JET_EncryptionAlgorithmAes256 ||
         pKey->blobHeader.bType != PLAINTEXTKEYBLOB ||
         pKey->blobHeader.bVersion != CUR_BLOB_VERSION ||
         pKey->blobHeader.reserved != 0 ||
         pKey->blobHeader.aiKeyAlg != CALG_AES_256 ||
         pKey->keySize != KEY_SIZE_AES256 ||
         pKey->Checksum != Crc32Checksum( (BYTE *)&pKey->blobHeader, sizeof( AES256KEYEXPANDED ) - OffsetOf( AES256KEYEXPANDED, blobHeader ) ) )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }

    return JET_errSuccess;
}

ERR
ErrOSBCryptCreateAes256Key(
    _Out_writes_bytes_to_opt_(*pcbKeySize, *pcbKeySize) BYTE *pbKey,
    _Inout_                                             ULONG *pcbKeySize )
{
    ERR err = JET_errSuccess;
    NTSTATUS status;

    if ( *pcbKeySize < sizeof( AES256KEYEXPANDED ) )
    {
        *pcbKeySize = sizeof( AES256KEYEXPANDED );
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    AES256KEYEXPANDED *pKey = (AES256KEYEXPANDED *)pbKey;
    pKey->Version = JET_EncryptionAlgorithmAes256;
    pKey->blobHeader.bType = PLAINTEXTKEYBLOB;
    pKey->blobHeader.bVersion = CUR_BLOB_VERSION;
    pKey->blobHeader.reserved = 0;
    pKey->blobHeader.aiKeyAlg = CALG_AES_256;
    pKey->keySize = KEY_SIZE_AES256;
    if ( !NT_SUCCESS( status = g_pfnBCryptGenRandom( NULL, pKey->pbKey, sizeof( pKey->pbKey ), BCRYPT_USE_SYSTEM_PREFERRED_RNG ) ) )
    {
        return ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status ));
    }
    pKey->Checksum = Crc32Checksum( (BYTE *)&pKey->blobHeader, sizeof( AES256KEYEXPANDED ) - OffsetOf( AES256KEYEXPANDED, blobHeader ) );
 
    *pcbKeySize = sizeof( AES256KEYEXPANDED );

#ifdef DEBUG
    CallS( ErrOSBCryptEncryptionVerifyKey( pbKey, *pcbKeySize ) );
#endif

    return err;
}

// PERSISTED
struct AES256BLOBTRAILER
{
    BYTE    Version;
    BYTE    InitVector[BLOCK_SIZE_AES256];
};

ERR
ErrOSBCryptEncryptWithAes256(
    _Inout_updates_bytes_to_(cbDataBufLen, *pcbDataLen)     BYTE *pbData,
    _Inout_                                                 ULONG *pcbDataLen,
    _In_                                                    ULONG cbDataBufLen,
    _In_reads_bytes_(cbKey)                         const   BYTE *pbKey,
    _In_                                                    ULONG cbKey )
{
    ERR err = JET_errSuccess;
    NTSTATUS status;
    AES256KEYEXPANDED *pKey = (AES256KEYEXPANDED *)pbKey;
    BCRYPT_KEY_HANDLE hKey = NULL;
    ULONG checksum;
    AES256BLOBTRAILER trailer;
    ULONG cbNeeded;
    BYTE InitVector[BLOCK_SIZE_AES256];
    BYTE *pbKeyObject;

    CallR( g_BCryptInitOnce.Init( ErrOSIBCryptAESProviderInit ) );

    CallR( ErrOSBCryptEncryptionVerifyKey( pbKey, cbKey ) );

    cbNeeded = CbOSEncryptAes256SizeNeeded( *pcbDataLen );
    if ( cbNeeded > cbDataBufLen )
    {
        *pcbDataLen = cbNeeded;
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    checksum = Crc32Checksum( pbData, *pcbDataLen );
    // checksum is appended to the plaintext
    *(UnalignedLittleEndian<ULONG> *)(pbData + *pcbDataLen) = checksum;
    *pcbDataLen += sizeof(checksum);

    pbKeyObject = (BYTE *)_alloca( g_cbKeyObject );
    if ( !NT_SUCCESS( status = g_pfnBCryptGenerateSymmetricKey( 
                                    g_hBCryptAesAlg,
                                    &hKey,
                                    pbKeyObject,
                                    g_cbKeyObject,
                                    pKey->pbKey,
                                    sizeof( pKey->pbKey ),
                                    0 )))
    {
        return ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status ));
    }
#ifdef DEBUG
    ULONG cbKeyLength, cbOut;
    if( !NT_SUCCESS( status = g_pfnBCryptGetProperty(
                                hKey, 
                                BCRYPT_KEY_LENGTH, 
                                (PBYTE)&cbKeyLength, 
                                sizeof(ULONG), 
                                &cbOut, 
                                0 )))
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )) );
    }
    Assert( cbKeyLength == KEY_SIZE_AES256*8 );
#endif

    if ( !NT_SUCCESS( status = g_pfnBCryptGenRandom( NULL, InitVector, sizeof( InitVector ), BCRYPT_USE_SYSTEM_PREFERRED_RNG ) ) )
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )));
    }
    // Make copy of InitVector before calling into BCryptEncrypt as it is will modify the init-vector buffer
    memcpy_s( trailer.InitVector, sizeof( trailer.InitVector ), InitVector, sizeof( InitVector ));

    if ( !NT_SUCCESS( status = g_pfnBCryptEncrypt( hKey, pbData, *pcbDataLen, NULL, InitVector, sizeof( InitVector ), pbData, cbDataBufLen, pcbDataLen, BCRYPT_BLOCK_PADDING ) ) )
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )));
    }

    // Version+InitVector is appended to the ciphertext
    if ( *pcbDataLen + sizeof(trailer) > cbDataBufLen )
    {
        Assert( fFalse ); // Messed up the calculation above?
        *pcbDataLen += sizeof(trailer);
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }
    Assert( *pcbDataLen + sizeof(trailer) == cbNeeded );

    trailer.Version = JET_EncryptionAlgorithmAes256;
    memcpy_s( pbData + *pcbDataLen, cbDataBufLen - *pcbDataLen, &trailer, sizeof(trailer) );
    *pcbDataLen += sizeof(trailer);

HandleError:

    Assert( err < JET_errSuccess || *pcbDataLen <= cbDataBufLen );

    if ( hKey )
    {
        g_pfnBCryptDestroyKey( hKey );
    }

    return err;
}

ERR
ErrOSBCryptDecryptWithAes256(
    _In_reads_( *pcbDataLen )                           BYTE *pbDataIn,
    _Out_writes_bytes_to_(*pcbDataLen, *pcbDataLen)     BYTE *pbDataOut,
    _Inout_                                             ULONG *pcbDataLen,
    _In_reads_bytes_(cbKey)                     const   BYTE *pbKey,
    _In_                                                ULONG cbKey )
{
    ERR err = JET_errSuccess;
    NTSTATUS status;
    AES256KEYEXPANDED *pKey = (AES256KEYEXPANDED *)pbKey;
    BCRYPT_KEY_HANDLE hKey = NULL;
    ULONG checksum;
    AES256BLOBTRAILER *ptrailer;
    BYTE InitVector[BLOCK_SIZE_AES256];
    BYTE *pbKeyObject;

    CallR( g_BCryptInitOnce.Init( ErrOSIBCryptAESProviderInit ) );

    CallR( ErrOSBCryptEncryptionVerifyKey( pbKey, cbKey ) );

    if ( *pcbDataLen < BLOCK_SIZE_AES256 + sizeof(AES256BLOBTRAILER) ||
         *pcbDataLen % BLOCK_SIZE_AES256 != sizeof(AES256BLOBTRAILER) % BLOCK_SIZE_AES256 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    // Version+InitVector is appended to the ciphertext
    ptrailer = (AES256BLOBTRAILER *)(pbDataIn + *pcbDataLen) - 1;
    if ( ptrailer->Version != JET_EncryptionAlgorithmAes256 )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    // Make copy of InitVector before calling into BCryptDecrypt as it is will modify the init-vector buffer
    memcpy_s( InitVector, sizeof( InitVector ), ptrailer->InitVector, sizeof( ptrailer->InitVector ));
    *pcbDataLen -= sizeof(AES256BLOBTRAILER);

    pbKeyObject = (BYTE *)_alloca( g_cbKeyObject );
    if ( !NT_SUCCESS( status = g_pfnBCryptGenerateSymmetricKey( 
                                    g_hBCryptAesAlg,
                                    &hKey,
                                    pbKeyObject,
                                    g_cbKeyObject,
                                    pKey->pbKey,
                                    sizeof( pKey->pbKey ),
                                    0 )))
    {
        return ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status ));
    }
#ifdef DEBUG
    ULONG cbKeyLength, cbOut;
    if( !NT_SUCCESS( status = g_pfnBCryptGetProperty(
                                hKey, 
                                BCRYPT_KEY_LENGTH, 
                                (PBYTE)&cbKeyLength, 
                                sizeof(ULONG), 
                                &cbOut, 
                                0 )))
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status )) );
    }
    Assert( cbKeyLength == KEY_SIZE_AES256*8 );
#endif

    // assert that the in/out buffers are not overlapping
    Assert( pbDataOut < pbDataIn || pbDataOut >= ( pbDataIn + *pcbDataLen ) );
    Assert( pbDataIn < pbDataOut || pbDataIn >= ( pbDataOut + *pcbDataLen ) );

    if ( !NT_SUCCESS( status = g_pfnBCryptDecrypt( hKey, pbDataIn, *pcbDataLen, NULL, InitVector, sizeof( InitVector ), pbDataOut, *pcbDataLen, pcbDataLen, BCRYPT_BLOCK_PADDING ) ) )
    {
        Error( ErrOSErrFromWin32Err( g_pfnRtlNtStatusToDosError( status ), JET_errDecryptionFailed ));
    }
    if ( *pcbDataLen < sizeof(checksum) )
    {
        Error( ErrERRCheck( JET_errDecryptionFailed ) );
    }
    *pcbDataLen -= sizeof(checksum);
    // checksum is appended to the plaintext
    checksum = *(UnalignedLittleEndian<ULONG> *)(pbDataOut + *pcbDataLen);

    if ( checksum != Crc32Checksum( pbDataOut, *pcbDataLen ) )
    {
        Error( ErrERRCheck( JET_errDecryptionFailed ) );
    }

HandleError:

    if ( hKey )
    {
        g_pfnBCryptDestroyKey( hKey );
    }

    return err;
}

