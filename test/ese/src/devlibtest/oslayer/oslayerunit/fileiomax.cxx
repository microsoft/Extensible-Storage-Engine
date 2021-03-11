// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpOSUnitTest
};


void AsyncWriteCompleteDecrementContext(
    const ERR           err,
    IFileAPI* const     pfapi,
    const IOREASON      ior,
    const OSFILEQOS     grbitQOS,
    const QWORD         ibOffset,
    const DWORD         cbData,
    const BYTE* const   pbData,
    const void*     pvContext )
{
    LONG * const pcioOutstandingTestIos = (LONG*) pvContext;
    AtomicDecrement( pcioOutstandingTestIos );
}


CUnitTest( OslayerHandlesLargeIoMax, bitExplicitOnly, "Test to see how much ridiculous IO we can load onto a single file." );
ERR OslayerHandlesLargeIoMax::ErrTest()
{
    ERR err;

    const wchar_t* szFilename = L"TestFileIoMax.tmp";
    DWORD cbIo = 4096;

    ULONG cioTooMany = 140000;

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    TICK tickStart;

    COSLayerPreInit oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

#ifdef DEBUG
    FNegTestSet( fInvalidUsage );
#endif

    COSLayerPreInit::SetIOMaxOutstanding( 65000 );

#ifdef DEBUG
    FNegTestUnset( fInvalidUsage );
#endif

    OSTestCall( ErrOSInit() );

    OSTestCheckErr( ErrOSFSCreate( NULL, &pfsapi ) );

    OSTestCheckErr( pfsapi->ErrFileCreate( szFilename, IFileAPI::fmfOverwriteExisting, &pfapi ) );

    const QWORD cbTotal = (QWORD)cioTooMany * cbIo * 2;
    wprintf( L"\t\tExtending file to %d.%03d MBs ... ", cbTotal / 1024 / 1024, cbTotal / 1024 % 1024 );
    tickStart = TickOSTimeCurrent();
    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbTotal, fFalse, qosIONormal ) );
    wprintf( L"Done(%d ms)\n", TickOSTimeCurrent() - tickStart );

    wprintf( L"\t\tDispatching IO writes ..." );
    tickStart = TickOSTimeCurrent();
    LONG cioOutstandingTestIos = 0;
    for( ULONG iio = 0; iio < cioTooMany; iio++ )
    {
        OSTestCheckErr( pfapi->ErrIOWrite( 
                        *TraceContextScope( iorpOSUnitTest ),
                        iio * cbIo * 2 ,
                        cbIo,
                        g_rgbZero,
                        qosIONormal,
                        IFileAPI::PfnIOComplete( AsyncWriteCompleteDecrementContext ),
                        (DWORD_PTR)&cioOutstandingTestIos ) );
        AtomicIncrement( &cioOutstandingTestIos );
        if( ( iio % 10000 ) == 0 )
        {
            wprintf( L"." );
        }
    }
    OSTestCheckErr( pfapi->ErrIOIssue() );
    wprintf( L"Done(%d ms)\n", TickOSTimeCurrent() - tickStart );

    QWORD qwSize;
    QWORD qwOnDiskSize;
    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbTotal );

    wprintf( L"\t\tWaiting for all IOs to come home ..." );
    TICK tickStart2 = TickOSTimeCurrent();
    while( cioOutstandingTestIos )
    {
        Sleep( 333 );
        wprintf( L"." );
    }
    wprintf( L"Done(%d ms)\n", TickOSTimeCurrent() - tickStart2 );

    TICK dtickTotal = TickOSTimeCurrent() - tickStart;
    const ULONG cbps = cioTooMany * cbIo / ( dtickTotal / 1000 );
    wprintf( L"\t\tIO rate: %d.%03d MBs/sec\n", cbps / 1024 / 1024, cbps / 1024 % 1024 );

HandleError:
    delete pfapi;
    delete pfsapi;

    OSTerm();
    return err;
}

