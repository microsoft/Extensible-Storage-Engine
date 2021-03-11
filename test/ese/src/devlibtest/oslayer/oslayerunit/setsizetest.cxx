// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpOSUnitTest
};


CUnitTest( TestErrSetSize, 0, "Test for interaction between zero-filling in using the ErrSetSize API" );
ERR TestErrSetSize::ErrTest()
{
    ERR err;
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;
    QWORD dwSize;
    QWORD dwSizeOnDisk;
    BYTE rgbTestPattern[4096];
    DWORD dwPattern = 0xbaadf00d;
    DWORD cb;
    BYTE rgbReadBuffer[4096];

    memcpy( rgbTestPattern, &dwPattern, sizeof( dwPattern ) );
    for ( cb = 4; cb < sizeof(rgbTestPattern); cb *= 2 )
    {
        memcpy( rgbTestPattern + cb, rgbTestPattern, cb );
    }

    COSLayerPreInit oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }
    OSTestCall( ErrOSInit() );

    OSTestCheckErr( ErrOSFSCreate( NULL, &pfsapi ) );

    OSTestCheckErr( pfsapi->ErrFileCreate( L"TestErrSetSize1.tmp", IFileAPI::fmfOverwriteExisting, &pfapi ) );

    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), 65536, fFalse, qosIONormal ) );
    OSTestCheckErr( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), 0, 65536, g_rgbZero, qosIONormal ) );

    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), 131072, fTrue, qosIONormal ) );
    OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeLogical ) );
    OSTestCheck( dwSize == 131072 );

    OSTestCheckErr( pfapi->ErrSize( &dwSizeOnDisk, IFileAPI::filesizeOnDisk ) );
    OSTestCheck( dwSize == dwSizeOnDisk || dwSizeOnDisk == 65536 );

    for ( cb = 0; cb < 131072; cb += sizeof(rgbReadBuffer) )
    {
        OSTestCheckErr( pfapi->ErrIORead( *TraceContextScope( iorpOSUnitTest ), cb, sizeof(rgbReadBuffer), rgbReadBuffer, qosIONormal ) );
        OSTestCheck( memcmp( rgbReadBuffer, g_rgbZero, sizeof(rgbReadBuffer) ) == 0 );
    }

    delete pfapi;
    pfapi = NULL;

    OSTestCheckErr( pfsapi->ErrFileOpen( L"TestErrSetSize1.tmp", IFileAPI::fmfNone, &pfapi ) );

    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), 196608, fFalse, qosIONormal ) );

    for ( cb = 0; cb < 131072; cb += sizeof(rgbTestPattern) )
    {
        OSTestCheckErr( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), 65536 + cb, sizeof(rgbTestPattern), rgbTestPattern, qosIONormal ) );
    }

    OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeLogical ) );
    OSTestCheck( dwSize == 196608 );

    OSTestCheckErr( pfapi->ErrSize( &dwSizeOnDisk, IFileAPI::filesizeOnDisk ) );
    OSTestCheck( dwSize == dwSizeOnDisk );

    delete pfapi;
    pfapi = NULL;

    OSTestCheckErr( pfsapi->ErrFileOpen( L"TestErrSetSize1.tmp", IFileAPI::fmfNone, &pfapi ) );

    OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeLogical ) );
    OSTestCheck( dwSize == 196608 );

    OSTestCheckErr( pfapi->ErrSize( &dwSizeOnDisk, IFileAPI::filesizeOnDisk ) );
    OSTestCheck( dwSize == dwSizeOnDisk );

    for ( cb = 0; cb < 65536; cb += sizeof(rgbReadBuffer) )
    {
        OSTestCheckErr( pfapi->ErrIORead( *TraceContextScope( iorpOSUnitTest ), cb, sizeof(rgbReadBuffer), rgbReadBuffer, qosIONormal ) );
        OSTestCheck( memcmp( rgbReadBuffer, g_rgbZero, sizeof(rgbReadBuffer) ) == 0 );
    }

    for ( cb = 0; cb < 131072; cb += sizeof(rgbReadBuffer) )
    {
        OSTestCheckErr( pfapi->ErrIORead( *TraceContextScope( iorpOSUnitTest ), 65536 + cb, sizeof(rgbReadBuffer), rgbReadBuffer, qosIONormal ) );
        OSTestCheck( memcmp( rgbReadBuffer, rgbTestPattern, sizeof(rgbReadBuffer) ) == 0 );
    }

    QWORD cbExpectedHuge = INVALID_FILE_SIZE;
    err = pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbExpectedHuge, fFalse, qosIONormal );
    if ( err == JET_errDiskFull )
    {
        err = JET_errSuccess;
    }
    else if ( err == JET_errInvalidParameter )
    {
        wprintf( L"Received JET_errInvalidParameter when setting the file size to almost 4 GB; this is likely a file system that does not support large files (e.g. FAT).\n" );

        delete pfapi;
        pfapi = NULL;

        OSTestCheckErr( pfsapi->ErrFileDelete( L"TestErrSetSize1.tmp" ) );

        goto HandleError;
    }
    else
    {
        OSTestCheckErr( err );

        dwSize = 0;
        OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeLogical ) );
        OSTestCheck( cbExpectedHuge == dwSize );

#if ESE_SUPPORTS_SPARSE_FILES
        dwSize = 0;
        OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeOnDisk ) );
        OSTestCheck( cbExpectedHuge == dwSize );
#endif
    }

    cbExpectedHuge = INVALID_FILE_SIZE + 0x100000000LL;
    err = pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbExpectedHuge, fFalse, qosIONormal );
    if ( err == JET_errDiskFull )
    {
        err = JET_errSuccess;
    }
    else
    {
        dwSize = 0;
        OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeLogical ) );
        OSTestCheck( cbExpectedHuge == dwSize );

#if ESE_SUPPORTS_SPARSE_FILES
        dwSize = 0;
        OSTestCheckErr( pfapi->ErrSize( &dwSize, IFileAPI::filesizeOnDisk ) );
        OSTestCheck( cbExpectedHuge == dwSize );
#endif
    }

    delete pfapi;
    pfapi = NULL;

    pfsapi->ErrFileDelete( L"TestErrSetSize1.tmp" );

HandleError:
    delete pfapi;
    delete pfsapi;

    OSTerm();
    return err;
}

