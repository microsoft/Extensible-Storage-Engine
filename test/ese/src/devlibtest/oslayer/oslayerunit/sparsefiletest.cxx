// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    // iorpNone publically defined
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

    // File State:
    //  Size = 0 bytes,
    //  Sparse = {empty}.

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

        // Close the file.
        delete pfapi;
        pfapi = NULL;

        OSTestCheckErr( pfsapi->ErrFileDelete( szFilename ) );
        goto HandleError;
    }
    // Check for other failures.
    OSTestCheckErr( err );

    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    Call( pfapi->ErrRetrieveAllocatedRegion( 1, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    Call( pfapi->ErrRetrieveAllocatedRegion( 1000, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    // Beyond EOF sparse segments only.
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

    // File State:
    //  Size = 1 MB,
    //  Sparse = {empty}.

    cbTotal = 1048576;
    OSTestCheckErr( pfapi->ErrSetSize( *TraceContextScope( iorpOSUnitTest ), cbTotal, fFalse, qosIONormal ) );
    OSTestCheckErr( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), 0, (DWORD)cbTotal, g_rgbZero, qosIONormal ) );

    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbTotal );

    // Confirm that the allocated region works when the file is 'full'.
    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072, &ibStartAllocatedRegion, &cbAllocated ) );

    // Looks like it returns whatever was specified as the offset, not necessarily the start of the region.
    // (If the entire file is allcoated, it will just return the offset).
    OSTestCheck( 131072 == ibStartAllocatedRegion );
    // The 1024 is an implementation detail.
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    // Even though MSDN says that it will round down to a convenient boundary, it does not appear to do so.
    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072 + 511, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 131072 + 511 == ibStartAllocatedRegion );
    // The 1024 is an implementation detail.
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    // Write a 64k block at the 128k mark. The file isn't marked as sparse yet, so the file doesn't shrink.
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

    // When the entire file is allocated, there should be no sparse segments.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    // Also check if the region does not start at the beginning.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 65536, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    // Mark the file as Sparse.
    OSTestCheckErr( pfapi->ErrSetSparseness() );

    // Free up a 64k block at the 128k mark.
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 131072, 65536 ) );

    // File State:
    //  Size = 1 MB
    //  Sparse = { [128k,192k] }.

    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    cbExpectedSize = cbTotal - 65536;
    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );

    // We should only see the single 64k block.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (128*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (128+64)*1024 - 1 == (*parrsparseseg)[0].ibLast );

    // Free up another 64k block at the 64k mark.
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 65536, 65536 ) );

    // File State:
    //  Size = 1 MB
    //  Sparse = { [64k,192k] }.

    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    cbExpectedSize -= 65536;
    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );

    // Check what the allocated region is when in the middle (the sparse region is currently 64-128k).
    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072, &ibStartAllocatedRegion, &cbAllocated ) );

    // Looks like it returns whatever the next non-sparse region is.
    OSTestCheck( ( 131072 + 65536 ) == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    // Check what the allocated region is when in the middle (the sparse region is currently 64-128k).
    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 65536, &ibStartAllocatedRegion, &cbAllocated ) );

    // Looks like it returns whatever was specified as the start, not necessarily the start of the region.
    OSTestCheck( ( 131072 + 65536 ) == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    // And when the offset is in the middle of the allocated 
    ibStartAllocatedRegion = 0;
    cbAllocated = 0;
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 65536, &ibStartAllocatedRegion, &cbAllocated ) );

    // Looks like it returns whatever was specified as the start, not necessarily the start of the region.
    OSTestCheck( ( 131072 + 65536 ) == ibStartAllocatedRegion );
    OSTestCheck( ( qwSize - ibStartAllocatedRegion ) == cbAllocated );

    // Beyond EOF.
    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( 131072 * 100, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    // It will be at offset 64k, until 192k
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 65536 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (64+128)*1024 - 1 == (*parrsparseseg)[0].ibLast );

    // The sparse range is [64k,192k]. Asking for [64k,128k] and [128k,192k] should return just that range.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 65536, 131072 - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 65536 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 131072 - 1 == (*parrsparseseg)[0].ibLast );

    // The sparse range is [64k,192k]. Asking for [64k,128k] and [128k,192k] should return just that range.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, (192*1024) - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );

    // Query up to immediately before.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (64*1024) - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    // Query up to immediately before.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 10, (64*1024) - 1, parrsparseseg ) );
    OSTestCheck( 0 == parrsparseseg->Size() );

    // Query up to the first byte.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (64*1024), parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibLast );

    // Query up to the second byte.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (64*1024) + 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (64*1024) + 1 == (*parrsparseseg)[0].ibLast );

    // Query up to one byte shy of the last.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (192*1024) - 2, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 2 == (*parrsparseseg)[0].ibLast );

    // Query up to the last byte.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (192*1024) - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );

    // Query up to one byte past the last.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, (192*1024), parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );

    // Segment is fully contained.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (64*1024) + 10, (192*1024) - 10, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) + 10 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 10 == (*parrsparseseg)[0].ibLast );

    // Segment is partially contained.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (64*1024) + 10, (192*1024) + 10, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) + 10 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );

    // Specify an already-sparse region. The size should remain the same.
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 65536, 131072 ) );

    // File State
    //  Size = 1 MB
    //  Sparse = { [64k,192k] }.

    OSTestCheckErr( pfapi->ErrSize( &qwSize, IFileAPI::filesizeLogical ) );
    OSTestCheckErr( pfapi->ErrSize( &qwOnDiskSize, IFileAPI::filesizeOnDisk ) );

    OSTestCheck( qwSize == cbTotal );
    OSTestCheck( qwOnDiskSize == cbExpectedSize );

    // Specifying a range that goes beyond EOF:
    // It believes that anything beyond EOF is 'sparse', and in a way it is: There is no
    // backing storage at the moment, and the FS will allocate storage for a write.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, cbTotal - 65536, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( cbTotal == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[0].ibLast );

    // Truly sparse + EOF.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( cbTotal == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[1].ibLast );

    // Truly sparse + EOF.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (64*1024) + 10, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( (64*1024) + 10 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( (192*1024) - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( cbTotal == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[1].ibLast );

    // What about when the sparse region is at the end of the file?
    QWORD ibAlmostEnd = qwSize - ( 64*1024 );
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), ibAlmostEnd, 65536 ) );

    // File State
    //  Size = 1 MB
    //  Sparse = { [64k,192k], [960k,1024k] }.

    OSTestCheckErr( pfapi->ErrRetrieveAllocatedRegion( ibAlmostEnd, &ibStartAllocatedRegion, &cbAllocated ) );
    OSTestCheck( 0 == ibStartAllocatedRegion );
    OSTestCheck( 0 == cbAllocated );

    // It will be at offset ibAlmostEnd.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, ibAlmostEnd - 65536, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( qwSize - 1 == (*parrsparseseg)[0].ibLast );

    // It will be at offset ibAlmostEnd.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, ibAlmostEnd, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( qwSize - 1 == (*parrsparseseg)[0].ibLast );

    // There are two different sparse regions. We should see both of them:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[1].ibLast );

    //
    // Some not-particularly-valid scenarios.
    //

    // Specifying a range that goes beyond EOF will not be well defined.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, ibAlmostEnd - 65536, cbTotal + 65536 - 1, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( cbTotal + 65536 - 1 == (*parrsparseseg)[0].ibLast );

    // Specifying unaligned ranges:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 65*1024 + 1, 100*1024, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 65*1024 + 1 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 100*1024 == (*parrsparseseg)[0].ibLast );

    // Specifying unaligned start/end (but outside of the sparse region):
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 1, 200*1024, parrsparseseg ) );
    OSTestCheck( 1 == parrsparseseg->Size() );
    OSTestCheck( 64*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );

    //
    // And now back to well-defined behaviours.
    //

    // Create a hole at 512k
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 512*1024, 65536 ) );

    // File State
    //  Size = 1 MB
    //  Sparse = { [64k,192k], [512k,576k], [960k,1024k] }.

    // There are multiple regions. We should see all of them.
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 64*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[2].ibLast );

    // There are multiple regions. We should see all of them (+ EOF).
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal + 10, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 64*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal + 10 == (*parrsparseseg)[2].ibLast );

    // There are multiple regions. We should see all of them (start from middle of first one).
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[2].ibLast );

    // The query range stopping at the beginning of the last sparse region should not get confused:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, ibAlmostEnd - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );

    // The query range stopping in the middle of the last sparse region should not get confused:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, cbTotal - 32768 - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal-32768 - 1 == (*parrsparseseg)[2].ibLast );

    // The query range stopping in the middle of the sparse region in the middle of the file should not get confused:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 131072, (512+32)*1024 - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 131072 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( (512+32)*1024 - 1 == (*parrsparseseg)[1].ibLast );

    // The query range starting at the beginning of an allocated region should not get confused:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, (128+64)*1024, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 2 == parrsparseseg->Size() );
    OSTestCheck( 512*1024 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[1].ibLast );

    // Create a hole at the beginning of the file.
    OSTestCheckErr( pfapi->ErrIOTrim( *TraceContextScope( iorpOSUnitTest ), 0, 65536 ) );

    // File State
    //  Size = 1 MB
    //  Sparse = { [0k,192k], [512k,576k], [960k,1024k] }.

    // The query range starting at the beginning of an allocated regionshould not get confused:
    OSTestCheck( CArray<SparseFileSegment>::ERR::errSuccess == parrsparseseg->ErrSetSize( 0 ) );
    OSTestCheckErr( ErrIORetrieveSparseSegmentsInRegion( pfapi, 0, cbTotal - 1, parrsparseseg ) );
    OSTestCheck( 3 == parrsparseseg->Size() );
    OSTestCheck( 0 == (*parrsparseseg)[0].ibFirst );
    OSTestCheck( 192*1024 - 1 == (*parrsparseseg)[0].ibLast );
    OSTestCheck( 512*1024 == (*parrsparseseg)[1].ibFirst );
    OSTestCheck( 576*1024 - 1 == (*parrsparseseg)[1].ibLast );
    OSTestCheck( ibAlmostEnd == (*parrsparseseg)[2].ibFirst );
    OSTestCheck( cbTotal - 1 == (*parrsparseseg)[2].ibLast );

    // Close the file.
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

