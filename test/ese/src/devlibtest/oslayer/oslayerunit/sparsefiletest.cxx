// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpOSUnitTest
};

CUnitTest( TestSparseFile, 0, "Test for sparse files using the ErrIOTrim API" );
ERR TestSparseFile::ErrTest()
{
    ERR err;
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;
    QWORD cbTotal = 0;
    QWORD ibStartAllocatedRegion = 0;
    QWORD cbAllocated = 0;
    QWORD qwSize;
    QWORD qwOnDiskSize;
    QWORD cbExpectedSize;
    BYTE rgbTestPattern[4096];
    DWORD dwPattern = 0xbaadf00d;
    DWORD cb;
    BYTE rgbReadBuffer[4096];
    const wchar_t* szFilename = L"TestSparseFile.tmp";
    CArray<SparseFileSegment>* parrsparseseg = NULL;

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

    parrsparseseg = new CArray<SparseFileSegment>();
    OSTestCheck( NULL != parrsparseseg );

    OSTestCheckErr( ErrOSFSCreate( NULL, &pfsapi ) );

    OSTestCheckErr( pfsapi->ErrFileCreate( szFilename, IFileAPI::fmfOverwriteExisting, &pfapi ) );


    cbTotal = 0;
    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbTotal, fFalse, qosIONormal ) );

    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbTotal );

    err = pfapi->ErrRetrieveAllocatedRegion( 0, &ibStartAllocatedRegion, &cbAllocated );
    if ( JET_errUnloadableOSFunctionality == err )
    {
        wprintf( L"Received JET_errUnloadableOSFunctionality when querying the file for sparseness; this is likely a file system that does not support Sparse files (e.g. FAT).\n" );

        delete pfapi;
        pfapi = NULL;

        OSTestCheckErr( pfsapi->ErrFileDelete( szFilename ) );
        goto HandleError;
    }
    OSTestCheckErr( err );

    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    Call( pfapi->ErrRetrieveAllocatedRegion( 1, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    Call( pfapi->ErrRetrieveAllocatedRegion( 1000, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, 0, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 0 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 0 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 0 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, 1000, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 0 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 1000 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1, 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 1 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1, 2, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 1 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 2 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1, 1000, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 1 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 1000 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1000, 1000, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 1000 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 1000 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1000, 1001, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 1000 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 1001 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1000, 2000, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 1000 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 2000 == (*parrsparseseg)[0].ibLast );


    cbTotal = 1048576;
    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbTotal, fFalse, qosIONormal ) );
    OSTestCheckErr( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), 0, (DWORD)cbTotal, g_rgbZero, qosIONormal ) );

    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbTotal );

    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072, &ibStartAllocatedRegion, &cbAllocated ) );

    OSTestCheck( 131072 == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072 + 511, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 131072 + 511 == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

#ifdef DEBUG
    FNegTestSet( fInvalidUsage );
#endif
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 131072, 65536 ) );
#ifdef DEBUG
    FNegTestUnset( fInvalidUsage );
#endif
    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    cbExpectedSize = cbTotal;
    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );
    OSTestCheck( qwOnDiskSize == qwSize );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 65536, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    OSTestCheckErr( pfapi->ErrSetSparseness() );

    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 131072, 65536 ) );


    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    cbExpectedSize = cbTotal - 65536;
    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (128*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (128+64)*1024 - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 65536, 65536 ) );


    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    cbExpectedSize -= 65536;
    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );

    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072, &ibStartAllocatedRegion, &cbAllocated ) );

    OSTestCheck( ( 131072 + 65536 ) == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 65536, &ibStartAllocatedRegion, &cbAllocated ) );

    OSTestCheck( ( 131072 + 65536 ) == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 65536, &ibStartAllocatedRegion, &cbAllocated ) );

    OSTestCheck( ( 131072 + 65536 ) == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072 * 100, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 65536 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (64+128)*1024 - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 65536, 131072 - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 65536 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 131072 - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, (192*1024) - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (64*1024) - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 10, (64*1024) - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (64*1024), parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (64*1024) + 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (64*1024) + 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (192*1024) - 2, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 2 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (192*1024) - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (192*1024), parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (64*1024) + 10, (192*1024) - 10, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) + 10 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 10 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (64*1024) + 10, (192*1024) + 10, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) + 10 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 65536, 131072 ) );


    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, cbTotal - 65536, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( cbTotal == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( cbTotal == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[1].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (64*1024) + 10, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) + 10 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( cbTotal == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[1].ibLast );

    QWORD ibAlmostEnd = qwSize - ( 64*1024 );
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), ibAlmostEnd, 65536 ) );


    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( ibAlmostEnd, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, ibAlmostEnd - 65536, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( qwSize - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, ibAlmostEnd, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( qwSize - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[1].ibLast );


    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, ibAlmostEnd - 65536, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 65*1024 + 1, 100*1024, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 65*1024 + 1 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 100*1024 == (*parrsparseseg)[0].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1, 200*1024, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 64*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );


    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 512*1024, 65536 ) );


    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 64*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[2].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal + 10, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 64*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal + 10 == (*parrsparseseg)[2].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[2].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, ibAlmostEnd - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, cbTotal - 32768 - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal-32768 - 1 == (*parrsparseseg)[2].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, (512+32)*1024 - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( (512+32)*1024 - 1 == (*parrsparseseg)[1].ibLast );

    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (128+64)*1024, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 512*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[1].ibLast );

    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 0, 65536 ) );


    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 0 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[2].ibLast );

    delete pfapi;
    pfapi = NULL;

    pfsapi->ErrFileDelete( szFilename );

HandleError:
    delete pfapi;
    delete pfsapi;
    delete parrsparseseg;
    
    OSTerm();
    return err;
}

