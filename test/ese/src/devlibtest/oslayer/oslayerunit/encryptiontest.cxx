// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "osunitstd.hxx"

CUnitTest( Aes256Encryption, 0, "Test for AES256 Encryption" );
ERR Aes256Encryption::ErrTest()
{
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    BYTE key[64];
    ULONG keySize;
    ULONG sizesToTry[] = { 1, 1024, 8120 };

    Call( ErrOSInit() );

    // Try null buffer
    wprintf( L"  Generate key: null buffer\n" );
    keySize = 0;
    OSTestCheckExpectedErr( JET_errBufferTooSmall, ErrOSCreateAes256Key( NULL, &keySize ) );
    OSTestCheck( keySize == 49 );

    // Try too small key buffer
    wprintf( L"  Generate key: small buffer\n" );
    keySize = 4;
    OSTestCheckExpectedErr( JET_errBufferTooSmall, ErrOSCreateAes256Key( key, &keySize ) );
    OSTestCheck( keySize == 49 );

    keySize = 47;
    OSTestCheckExpectedErr( JET_errBufferTooSmall, ErrOSCreateAes256Key( key, &keySize ) );
    OSTestCheck( keySize == 49 );

    // Try big enough key buffer
    wprintf( L"  Generate key: good buffer\n" );
    keySize = 49;
    OSTestCheckErr( ErrOSCreateAes256Key( key, &keySize ) );
    OSTestCheck( keySize == 49 );

    keySize = sizeof(key);
    OSTestCheckErr( ErrOSCreateAes256Key( key, &keySize ) );
    OSTestCheck( keySize == 49 );

    wprintf( L"  Test encryption/decryption\n" );

    for ( INT i=0; i<_countof(sizesToTry); i++ )
    {
        BYTE pbData[8192];
        BYTE pbData2[8192];
        ULONG j;
        BYTE wrongKey[49];
        ULONG dataLength = sizesToTry[i];
        ULONG cbNeeded = CbOSEncryptAes256SizeNeeded( dataLength );
        OSTestCheck( cbNeeded <= sizeof(pbData) );
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }

        wprintf( L"    data size %d\n", dataLength );

        // Too small buffer
        wprintf( L"      Too small buffer\n" );
        OSTestCheckExpectedErr( JET_errBufferTooSmall, ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded - 1, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );

        // Just right
        wprintf( L"      Just Right buffer\n" );
        dataLength = sizesToTry[i];
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );

        // decrypt
        wprintf( L"      Decrypt\n" );
        OSTestCheckErr( ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );
        OSTestCheck( dataLength == sizesToTry[i] );
        for ( j=0; j<dataLength; j++ )
        {
            OSTestCheck( pbData2[j] == j % 256 );
        }

        // Use unaligned buffer
        wprintf( L"      Decrypt: unaligned buffer\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j+1] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData+1, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        OSTestCheckErr( ErrOSDecryptWithAes256( pbData+1, pbData2+1, &dataLength, key, keySize ) );
        OSTestCheck( dataLength == sizesToTry[i] );
        for ( j=0; j<dataLength; j++ )
        {
            OSTestCheck( pbData2[j+1] == j % 256 );
        }

        // decrypt with wrong key
        wprintf( L"      Decrypt: wrong key\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        OSTestCheckErr( ErrOSCreateAes256Key( wrongKey, &keySize ) );
        OSTestCheckExpectedErr( JET_errDecryptionFailed, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, wrongKey, keySize ) );

        // decrypt with corrupt key
        wprintf( L"      Decrypt: corrupt key\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, wrongKey, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        wrongKey[48] ^= 0xff;
        OSTestCheckExpectedErr( JET_errInvalidParameter, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, wrongKey, keySize ) );

        // corrupt IV at end of data
        wprintf( L"      Decrypt: corrupt IV\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        pbData[ dataLength - 1 ] ^= 0xff;
        OSTestCheckExpectedErr( JET_errDecryptionFailed, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );

        // corrupt version at end of data
        wprintf( L"      Decrypt: corrupt checksum\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        pbData[ dataLength - 17 ] ^= 0xff;
        OSTestCheckExpectedErr( JET_errInvalidParameter, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );

        // corrupt checksum at end of data
        wprintf( L"      Decrypt: corrupt checksum\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        pbData[ dataLength - 18 ] ^= 0xff;
        OSTestCheckExpectedErr( JET_errDecryptionFailed, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );

        // corrupt data
        wprintf( L"      Decrypt: corrupt data\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        pbData[ 0 ] ^= 0xff;
        OSTestCheckExpectedErr( JET_errDecryptionFailed, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );

        // truncate buffer
        wprintf( L"      Decrypt: truncate buffer\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        dataLength -= 8;
        OSTestCheckExpectedErr( JET_errInvalidParameter, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );

        // buffer too big
        wprintf( L"      Decrypt: buffer too big\n" );
        dataLength = sizesToTry[i];
        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }
        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );
        for ( j=dataLength; j<dataLength+32; j++ )
            pbData[j] = 0;
        dataLength += 32;
        OSTestCheckExpectedErr( JET_errInvalidParameter, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );
    }
    err = JET_errSuccess;

HandleError:

    OSTerm();
    return err;
}

DWORD WINAPI
DoEncryptDecrypt( PVOID )
{
    JET_ERR err = JET_errSuccess;

    for ( INT i=0; i<1000; i++ )
    {
        BYTE key[49];
        ULONG keySize;
        BYTE pbData[257];
        BYTE pbData2[257];
        ULONG dataLength;
        ULONG cbNeeded;
        ULONG j;

        keySize = 49;
        OSTestCheckErr( ErrOSCreateAes256Key( key, &keySize ) );
        OSTestCheck( keySize == 49 );

        dataLength = 224;
        cbNeeded = CbOSEncryptAes256SizeNeeded( dataLength );
        OSTestCheck( cbNeeded <= sizeof(pbData) );

        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }

        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );

        OSTestCheckErr( ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, key, keySize ) );
        OSTestCheck( dataLength == 224 );
        for ( j=0; j<dataLength; j++ )
        {
            OSTestCheck( pbData2[j] == j % 256 );
        }
    }

HandleError:
    return 0;
}

CUnitTest( Aes256EncryptionMultiThreaded, 0, "Test for AES256 Encryption from multiple threads" );
ERR Aes256EncryptionMultiThreaded::ErrTest()
{
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const INT NumThreads = 4;
    HANDLE hThreads[ NumThreads ];
    INT i;

    Call( ErrOSInit() );

    wprintf( L"  Creating %d threads\n", NumThreads );
    for ( i=0; i<NumThreads; i++ )
        hThreads[i] = CreateThread( NULL, 0, DoEncryptDecrypt, NULL, 0, NULL );

    wprintf( L"  Waiting for threads to finish\n" );
    WaitForMultipleObjects( NumThreads, hThreads, TRUE, INFINITE );
    wprintf( L"  All done\n" );

    for ( i=0; i<NumThreads; i++ )
        CloseHandle( hThreads[i] );

HandleError:

    OSTerm();
    return err;
}

CUnitTest( Aes256EncryptionWrongKey, 0, "Test for AES256 Encryption and decryption with wrong key" );
ERR Aes256EncryptionWrongKey::ErrTest()
{
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;

    Call( ErrOSInit() );

    for ( INT i=0; i<1000; i++ )
    {
        BYTE key[49];
        BYTE wrongKey[49];
        ULONG keySize;
        BYTE pbData[257];
        BYTE pbData2[257];
        ULONG dataLength;
        ULONG cbNeeded;
        ULONG j;

        keySize = 49;
        OSTestCheckErr( ErrOSCreateAes256Key( key, &keySize ) );
        OSTestCheck( keySize == 49 );
        OSTestCheckErr( ErrOSCreateAes256Key( wrongKey, &keySize ) );
        OSTestCheck( keySize == 49 );

        dataLength = 224;
        cbNeeded = CbOSEncryptAes256SizeNeeded( dataLength );
        OSTestCheck( cbNeeded <= sizeof(pbData) );

        for ( j=0; j<dataLength; j++ )
        {
            pbData[j] = j % 256;
        }

        OSTestCheckErr( ErrOSEncryptWithAes256( pbData, &dataLength, cbNeeded, key, keySize ) );
        OSTestCheck( dataLength == cbNeeded );

        OSTestCheckExpectedErr( JET_errDecryptionFailed, ErrOSDecryptWithAes256( pbData, pbData2, &dataLength, wrongKey, keySize ) );
    }

HandleError:

    OSTerm();
    return err;
}


