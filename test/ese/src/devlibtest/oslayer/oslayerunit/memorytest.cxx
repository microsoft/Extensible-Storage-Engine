// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"


extern WCHAR g_rgwchSysDriveTesting[IFileSystemAPI::cchPathMax];


INT PercentToRunQueryWorkingSetEx()
{
    return 10;
}

#ifdef MEM_CHECK
CUnitTest( OSLayerNewCanTrackAllocLine, 0, "Tracks the line of the file the new happens at is accurate." );
ERR OSLayerNewCanTrackAllocLine::ErrTest()
{
    JET_ERR err;
    ULONG * pul = NULL;


    COSLayerPreInit oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    OSTestCheck( g_fMemCheck );

    wprintf( L"\t\tTesting operator new line tracking ...\n" );

    const ULONG ulNewLineExpected = __LINE__ + 1;
    pul = new ULONG;

    if ( NULL == pul )
    {
        wprintf( L"\t\tFailed new alloc!\n" );
        err = -64;
        goto HandleError;
    }

    wprintf( L"\t\t\t\tWe had a new @ %hs : %d\n", SzNewFile(), UlNewLine() );
    OSTestCheck( _stricmp( __FILE__, SzNewFile() ) == 0 );
    OSTestCheck( ulNewLineExpected == UlNewLine() );

    err = JET_errSuccess;

HandleError:

    delete pul;
    
    return err;
}
#endif


bool FAVdWritingToAddressRangeMost( void* pv, DWORD cb )
{
    bool fAVd = true;

    


    TRY
    {
        BYTE* pvByte = (BYTE*)pv + cb - 1;
        memset( pvByte, *pvByte, 1 );
        fAVd = false;
    }
    EXCEPT( efaExecuteHandler )
    {
    }

    return fAVd;
}


bool FNotAVdWritingToAddressRangeAll( void* pv, DWORD cb )
{
    bool fNotAVd = true;

    for ( DWORD ib = 0; ib < cb && fNotAVd; ib++ )
    {
        TRY
        {
            BYTE* pvByte = (BYTE*)pv + ib;
            memset( pvByte, *pvByte, 1 );
        }
        EXCEPT( efaExecuteHandler )
        {
            fNotAVd = false;
        }
    }

    return fNotAVd;
}


CUnitTest( TestOSMemoryPageCommitGranularity, 0, "Validates the page commit granularity is expected power of 2." );
ERR TestOSMemoryPageCommitGranularity::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    const DWORD cbPageGranularity = OSMemoryPageCommitGranularity();


    OSTestCheck( cbPageGranularity >= 4 );
    OSTestCheck( FPowerOf2( cbPageGranularity ) );
    OSTestCheck( cbPageGranularity <= 4 * 1024 ); 

HandleError:

    return err;
}


CUnitTest( OSLayerPvOSMemoryPageAllocTest, 0, "Tests PvOSMemoryPageAlloc()s size granularity and initial state." );
ERR OSLayerPvOSMemoryPageAllocTest::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPageGranularity = OSMemoryPageCommitGranularity();
    const DWORD cbAlloc = 2 * cbPageGranularity;
    BYTE* pvPattern0s = new BYTE[ cbAlloc ];
    BYTE* pvPattern1s = new BYTE[ cbAlloc ];

    memset( pvPattern0s, 0, cbAlloc );
    memset( pvPattern1s, 1, cbAlloc );


    OSTestCheck( cbAlloc == 2 * cbPageGranularity );


    void* pv = PvOSMemoryPageAlloc( cbAlloc, NULL );

    wprintf( L"Mem Status Info of PvOSMemoryPageAlloc():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );


    OSTestCheck( pv != NULL );
    OSTestCheck( ( (size_t)pv % cbPageGranularity ) == 0 );


    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );


    OSTestCheck( !FOSMemoryPageResident( pv, cbAlloc ) );


    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );


    OSTestCheck( FOSMemoryPageResident( pv, cbAlloc ) );

    wprintf( L"Mem Status Info after deref:\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );


    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );


    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    void* pvMiddle = PvOSMemoryPageAlloc( cbAlloc, (BYTE*)pv + cbAlloc / 2 );
    OSTestCheck( pvMiddle == NULL );

    OSMemoryPageFree( pv );
    
HandleError:

    delete[] pvPattern0s;
    delete[] pvPattern1s;

    return err;
}


CUnitTest( OSLayerPvOSMemoryPageReserveTest, 0, "Tests FTestPvOSMemoryPageReserve()s size granularity and initial state." );
ERR OSLayerPvOSMemoryPageReserveTest::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPageGranularity = OSMemoryPageCommitGranularity();
    const DWORD cbAlloc = 2 * cbPageGranularity;


    OSTestCheck( cbAlloc == 2 * cbPageGranularity );


    void* pv = PvOSMemoryPageReserve( cbAlloc, NULL );

    wprintf( L"Mem Status Info of PvOSMemoryPageReserve():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    OSTestCheck( pv != NULL );
    OSTestCheck( ( (size_t)pv % cbPageGranularity ) == 0 );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbAlloc ) );


    OSTestCheck( !FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );


    void* pvMiddle = PvOSMemoryPageReserve( cbAlloc, (BYTE*)pv + cbAlloc / 2 );
    OSTestCheck( pvMiddle == NULL );

    OSMemoryPageFree( pv );
    
HandleError:

    return err;
}


BOOL FTestOSMemoryAnyPageAllocated( const BYTE * const pb, const DWORD cb )
{
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    for ( DWORD cbDone = 0; ( cbDone + cbPage ) <= cb; cbDone += cbPage )
    {
        if ( FOSMemoryPageAllocated( pb + cbDone, cbPage ) )
        {
            return fTrue;
        }
    }
    return fFalse;
}

CUnitTest( OSLayerFOSMemoryPageAllocatedRequiresAllAllocatedPages, 0, "Tests if functions like FOSMemoryPageAllocated() return the right answers when not all the memory answers the same." );
ERR OSLayerFOSMemoryPageAllocatedRequiresAllAllocatedPages::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();
    const DWORD cbReserve   = OSMemoryPageReserveGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    srand( TickOSTimeCurrent() );



    DWORD cb = cbReserve * 3;
    BYTE * pb = (BYTE*)PvOSMemoryPageReserve( cbReserve * 3, NULL );

    OSTestCheck( !FTestOSMemoryAnyPageAllocated( pb, cb ) );

    Assert( cbPage * 3 < cbReserve );
    OSTestCheck( fTrue == FOSMemoryPageCommit( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    volatile BYTE bRead = *( pb + cbReserve + cbPage + 1 );
    OSTestCheck( bRead == 0 );

    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve - cbPage, cbPage * 3 ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve + cbPage, cbPage * 3 ) );

    *( pb + cbReserve + cbPage + 1 ) = 0xb;

    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve - cbPage, cbPage * 3 ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve + cbPage, cbPage * 3 ) );

    OSMemoryPageDecommit( pb + cbReserve + cbPage, cbPage );
    
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );

    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );

    OSMemoryPageProtect( pb + cbReserve + cbPage * 2, cbPage );

    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( fTrue == FOSMemoryPageCommit( pb + cbReserve + cbPage, cbPage ) );

    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

HandleError:

    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}


CUnitTest( OSLayerFOSMemoryFileMappedRequiresAllMappedPages, 0, "Tests if functions like FOSMemoryFileMapped() return the right answers when not all the memory answers the same." );
ERR OSLayerFOSMemoryFileMappedRequiresAllMappedPages::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();
    const DWORD cbReserve   = OSMemoryPageReserveGranularity();

    const BOOL fMmReadInstead = fFalse;

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMapFirst = NULL, * pbMapMid = NULL, * pbMapLast = NULL;

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    const INT ipageFirst    = 16;
    const INT ipageMid      = 32+6;
    const INT ipageLast     = 64-7;

    BYTE * pbFree1 = NULL, * pbFree2 = NULL;

    const __int64 cbFirstMidContiguousOffset = ( 16 + 6 ) * cbPage;
    const __int64 cbMidLastContiguousOffset = ( 16 + 3 ) * cbPage;
    wprintf( L"\t\tContig: %I64d, %I64d\n", cbFirstMidContiguousOffset, cbMidLastContiguousOffset );

    if ( fMmReadInstead )
    {
        OSTestCall( pfapi->ErrMMRead( ipageFirst * cbPage, cbPage, (void**)&pbMapFirst ) );
        OSTestCall( pfapi->ErrMMRead( ipageMid * cbPage, cbPage, (void**)&pbMapMid ) );
        if ( ( ( pbMapMid - pbMapFirst ) == cbFirstMidContiguousOffset ) ||
                ( ( pbMapFirst - pbMapMid ) == cbFirstMidContiguousOffset ) )
        {
            wprintf( L"\t\t\t\tpbFree1 = %p\n", pbMapMid );
            pbFree1 = pbMapMid;
            pbMapMid = NULL;
            OSTestCall( pfapi->ErrMMRead( ipageMid * cbPage, cbPage, (void**)&pbMapMid ) );
            OSTestCheck( pbMapMid != pbFree1 );
        }
        OSTestCall( pfapi->ErrMMRead( ipageLast * cbPage, cbPage, (void**)&pbMapLast ) );
        if ( ( ( pbMapLast - pbMapMid ) == cbMidLastContiguousOffset ) ||
                ( ( pbMapMid - pbMapLast ) == cbMidLastContiguousOffset ) )
        {
            wprintf( L"\t\t\t\tpbFree2 = %p\n", pbMapMid );
            pbFree2 = pbMapLast;
            pbMapLast = NULL;
            OSTestCall( pfapi->ErrMMRead( ipageLast * cbPage, cbPage, (void**)&pbMapLast ) );
            OSTestCheck( pbMapLast != pbFree2 );
        }
    }
    else
    {
        OSTestCall( pfapi->ErrMMCopy( ipageFirst * cbPage, cbPage, (void**)&pbMapFirst ) );
        OSTestCall( pfapi->ErrMMCopy( ipageMid * cbPage, cbPage * 4, (void**)&pbMapMid ) );
        if ( ( ( pbMapMid - pbMapFirst ) == cbFirstMidContiguousOffset ) ||
                ( ( pbMapFirst - pbMapMid ) == cbFirstMidContiguousOffset ) )
        {
            wprintf( L"\t\t\t\tpbFree1 = %p\n", pbMapMid );
            pbFree1 = pbMapMid;
            pbMapMid = NULL;
            OSTestCall( pfapi->ErrMMCopy( ipageMid * cbPage, cbPage, (void**)&pbMapMid ) );
            OSTestCheck( pbMapMid != pbFree1 );
        }
        OSTestCall( pfapi->ErrMMCopy( ipageLast * cbPage, cbPage, (void**)&pbMapLast ) );
        if ( ( ( pbMapLast - pbMapMid ) == cbMidLastContiguousOffset ) ||
                ( ( pbMapMid - pbMapLast ) == cbMidLastContiguousOffset ) )
        {
            wprintf( L"\t\t\t\tpbFree2 = %p\n", pbMapMid );
            pbFree2 = pbMapLast;
            pbMapLast = NULL;
            OSTestCall( pfapi->ErrMMRead( ipageLast * cbPage, cbPage, (void**)&pbMapLast ) );
            OSTestCheck( pbMapLast != pbFree2 );
        }
    }
    pfapi->ErrMMFree( pbFree1 );
    pfapi->ErrMMFree( pbFree2 );

    wprintf( L"\t\t\tpbMapFirst  = %p\n", pbMapFirst );
    wprintf( L"\t\t\tpbMapMid    = %p + %I64d\n", pbMapMid, pbMapMid - pbMapFirst );
    wprintf( L"\t\t\tpbMapLast   = %p + %I64d\n", pbMapLast, pbMapLast - pbMapMid );

    OSTestCheck( !FOSMemoryPageAllocated( pbMapFirst, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMapFirst + cbPage, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMapMid, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMapMid + cbPage, cbPage ) );

    OSTestCheck( !FOSMemoryPageAllocated( pbMapLast, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMapLast - cbPage, cbPage ) );


    OSTestCheck( FOSMemoryFileMapped( pbMapFirst, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapFirst + cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapFirst, cbPage * 2 ) );

    OSTestCheck( FOSMemoryFileMapped( pbMapMid, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapMid - cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapMid - cbPage, cbPage * 3 ) );

    OSTestCheck( FOSMemoryFileMapped( pbMapLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapLast - cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapLast - cbPage, cbPage * 2 ) );


    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapFirst, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapFirst + cbPage, cbPage ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapMid, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapMid - cbPage, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapMid - cbPage, cbPage * 3 ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapLast, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapLast - cbPage, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapLast - cbPage, cbPage * 2 ) );

HandleError:

    pfapi->ErrMMFree( pbMapFirst );
    pfapi->ErrMMFree( pbMapMid );
    pfapi->ErrMMFree( pbMapLast );

    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}

CUnitTest( OSLayerFOSMemoryFileMappedCowedRequiresAnyCopiedPage, 0, "Tests if functions like FOSMemoryFileMappedCowed() return the right answers when not all the memory answers the same." );
ERR OSLayerFOSMemoryFileMappedCowedRequiresAnyCopiedPage::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMapCopyFirst = NULL, * pbMapCopyMid = NULL, * pbMapCopyLast = NULL;

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );
    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    const INT ipageFirst    = 16;
    const INT ipageMid      = 32 + 2;
    const INT ipageLast     = 48 + 12;

    OSTestCall( pfapi->ErrMMCopy( ipageFirst * cbPage, cbPage * 4, (void**)&pbMapCopyFirst ) );
    OSTestCall( pfapi->ErrMMCopy( ipageMid * cbPage, cbPage * 4, (void**)&pbMapCopyMid ) );
    OSTestCall( pfapi->ErrMMCopy( ipageLast * cbPage, cbPage * 4, (void**)&pbMapCopyLast ) );


    OSTestCheck( FOSMemoryFileMapped( pbMapCopyFirst, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapCopyFirst, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    pbMapCopyFirst[1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );

    OSTestCheck( FOSMemoryFileMapped( pbMapCopyMid, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    pbMapCopyMid[cbPage * 2 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );

    OSTestCheck( FOSMemoryFileMapped( pbMapCopyLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    pbMapCopyLast[cbPage * 3 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );


    if ( !FUtilOsIsGeq( 6, 2 ) )
    {
        wprintf(L"Feature ErrMMRevert() is not available on Win NT %d.%d, skipping remainder of test.\n", DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor() );
        err = JET_errSuccess;
        goto HandleError;
    }

    OSTestCall( pfapi->ErrMMRevert( ipageFirst * cbPage, pbMapCopyFirst, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );

    OSTestCall( pfapi->ErrMMRevert( ipageMid * cbPage, pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );

    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );


    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    pbMapCopyFirst[1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    pfapi->ErrMMFree( pbMapCopyFirst );
    pfapi->ErrMMFree( pbMapCopyMid );
    pfapi->ErrMMFree( pbMapCopyLast );

    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}

CUnitTest( OSLayerMappedCowedPagesRevertToOriginalPageImages, 0, "Tests if functions ->ErrMMRevert() correctly reverts the page images to those that were originally read in." );
ERR OSLayerMappedCowedPagesRevertToOriginalPageImages::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMapCopyFirst = NULL, * pbMapCopyMid = NULL, * pbMapCopyLast = NULL;

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL, &pfsapi ) );
    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    const INT ipageFirst    = 16;
    const INT ipageMid      = 32 + 2;
    const INT ipageLast     = 48 + 12;

    OSTestCall( pfapi->ErrMMCopy( ipageFirst * cbPage, cbPage * 4, (void**)&pbMapCopyFirst ) );
    OSTestCall( pfapi->ErrMMCopy( ipageMid * cbPage, cbPage * 4, (void**)&pbMapCopyMid ) );
    OSTestCall( pfapi->ErrMMCopy( ipageLast * cbPage, cbPage * 4, (void**)&pbMapCopyLast ) );


    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    OSTestCheck( pbMapCopyFirst[1] != 0x42 );               
    OSTestCheck( pbMapCopyFirst[1] == (BYTE)ipageFirst );
    const BYTE bFirst = pbMapCopyFirst[1];
    pbMapCopyFirst[1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] != 0x42 );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == (BYTE)ipageMid + 2 );
    const BYTE bMid = pbMapCopyMid[cbPage * 2 + 1];
    pbMapCopyMid[cbPage * 2 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] != 0x42 );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == (BYTE)ipageLast + 3 );
    const BYTE bLast = pbMapCopyLast[cbPage * 3 + 1];
    pbMapCopyLast[cbPage * 3 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );


    if ( !FUtilOsIsGeq( 6, 2 ) )
    {
        wprintf(L"Feature ErrMMRevert() is not available on Win NT %d.%d, skipping remainder of test.\n", DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor() );
        err = JET_errSuccess;
        goto HandleError;
    }

    OSTestCheck( pbMapCopyFirst[1] == 0x42 );
    OSTestCall( pfapi->ErrMMRevert( ipageFirst * cbPage, pbMapCopyFirst, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    OSTestCheck( pbMapCopyFirst[1] != 0x42 );
    OSTestCheck( pbMapCopyFirst[1] == (BYTE)ipageFirst );
    OSTestCheck( pbMapCopyFirst[1] == bFirst );

    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == 0x42 );
    OSTestCall( pfapi->ErrMMRevert( ipageMid * cbPage, pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] != 0x42 );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == (BYTE)ipageMid + 2 );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == bMid );

    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == 0x42 );
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == 0x42 );
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] != 0x42 );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == (BYTE)ipageLast + 3 );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == bLast );


    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    pbMapCopyMid[cbPage * 2 + 1] = 0x43;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );

    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == 0x43 );
    OSTestCall( pfapi->ErrMMRevert( ipageMid * cbPage, pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );   
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == bMid );

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    pfapi->ErrMMFree( pbMapCopyFirst );
    pfapi->ErrMMFree( pbMapCopyMid );
    pfapi->ErrMMFree( pbMapCopyLast );

    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}

CUnitTest( OSLayerTestMemoryMappedStatesAreCorrect, 0, "Tests if memory mapping under a variety of conditions all return the same FOSMemoryFileMapped() test results." );
ERR OSLayerTestMemoryMappedStatesAreCorrect::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMM = NULL, * pbMMC = NULL, * pbMMR = NULL, * pbCOW = NULL, * pbMMP = NULL, * pbReMM = NULL;

    BYTE * pbTest = NULL;

    srand( TickOSTimeCurrent() );

    Call( ErrCreateIncrementFile( wszFileDat ) );

    OSTestCall( ErrOSInit() );
    OSTestCall( ErrOSFSCreate( NULL, &pfsapi ) );
    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) );
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );


    wprintf( L"\t\tpbMM, err = %d\n", pfapi->ErrMMRead( cbPage * ( 5 + 16 ), cbPage, (void**)&pbMM ) );
    wprintf( L"\t\tpbMMC, err = %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 32 ), cbPage, (void**)&pbMMC ) );
    wprintf( L"\t\tpbMMR, err = %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 48 ), cbPage, (void**)&pbMMR ) );
    wprintf( L"\t\tpbCOW, err = %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 64 ), cbPage, (void**)&pbCOW ) );
    wprintf( L"\t\tpbMMP, err = %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 80 ), cbPage, (void**)&pbMMP ) );

    volatile LONG forceread;
    forceread = *(LONG*)pbMMR;
    AtomicExchangeAdd( (LONG * const)pbCOW, 0 );
    forceread += *(LONG*)pbMMP;
    OSMemoryPageProtect( pbMMP, cbPage );

    wprintf( L"\t\tpbReMM, err = %d", pfapi->ErrMMCopy( cbPage * ( 5 + 96 ), cbPage, (void**)&pbReMM ) );
    AtomicExchangeAdd( (LONG * const)pbReMM, 0 );
    OSTestCheck( FOSMemoryFileMappedCowed( pbReMM, cbPage ) );
    wprintf( L", %d\n", pfapi->ErrMMRevert( cbPage * ( 5 + 96 ), pbReMM, cbPage ) );


    PrintMemInfo( 0, pbMemInfoHeaders );
    PrintMemInfo( 0, pbMM );
    PrintMemInfo( 0, pbMMC );
    PrintMemInfo( 0, pbMMR );
    PrintMemInfo( 0, pbCOW );
    PrintMemInfo( 0, pbMMP );
    PrintMemInfo( 0, pbReMM );
    PrintMemInfo( 0, NULL );

    wprintf( L"\tChecking no MM pages are PageAlloc'd\n" );
    OSTestCheck( !FOSMemoryPageAllocated( pbMM, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMMC, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMMR, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbCOW, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbMMP, cbPage ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbReMM, cbPage ) );

    wprintf( L"\tChecking all MM pages are MM'd\n" );
    OSTestCheck( FOSMemoryFileMapped( pbMM, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMC, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMR, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbCOW, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMP, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbReMM, cbPage ) );

    wprintf( L"\tChecking just the right MM pages are copied/COW'd\n" );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMM, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMMC, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMMR, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMMP, cbPage ) );
    if ( FUtilOsIsGeq( 6, 2 ) )
    {
        OSTestCheck( !FOSMemoryFileMappedCowed( pbReMM, cbPage ) );
    }



    wprintf( L"\tChecking first 2 KB of all MM pages are MM'd\n" );
    OSTestCheck( FOSMemoryFileMapped( pbMM, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMC, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMR, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbCOW, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMP, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbReMM, cbPage / 2 ) );

    wprintf( L"\tChecking second 2 KB of all MM pages are MM'd\n" );
    OSTestCheck( FOSMemoryFileMapped( pbMM + cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMC + cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMR + cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbCOW + cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbMMP + cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMapped( pbReMM + cbPage / 2, cbPage / 2 ) );

    wprintf( L"\tChecking a mis-aligned page / 4 KB of all MM pages are MM'd\n" );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbCOW - cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW, cbPage ) );   

    wprintf( L"\tChecking some various boundary-esk mis-aligned sizes ...\n" );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbCOW - cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW - cbPage / 2, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW + cbPage / 2, cbPage / 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW + cbPage / 2, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbCOW + cbPage, cbPage / 2 ) );

        

    wprintf( L"\tTesting basic mem test APIs on simple RO MM: %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 128 ), cbPage * 5, (void**)&pbTest ) );
    OSTestCheck( !FOSMemoryPageAllocated( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMapped( pbTest, cbPage * 5 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );
    pfapi->ErrMMFree( pbTest );
    pbTest = NULL;

    wprintf( L"\tTesting basic mem test APIs on COW'd (1st) / writable MM: %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 128 ), cbPage * 5, (void**)&pbTest ) );
    AtomicExchangeAdd( (LONG * const)( pbTest ), 0 );
    OSTestCheck( !FOSMemoryPageAllocated( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMapped( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest + cbPage, cbPage * 4 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );
    pfapi->ErrMMFree( pbTest );
    pbTest = NULL;

    wprintf( L"\tTesting basic mem test APIs on COW'd (3rd) / writable MM: %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 128 ), cbPage * 5, (void**)&pbTest ) );
    AtomicExchangeAdd( (LONG * const)( pbTest + cbPage * 2 ), 0 );
    OSTestCheck( !FOSMemoryPageAllocated( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMapped( pbTest, cbPage * 5 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest, cbPage * 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest + 2 * cbPage, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest + 3 * cbPage, cbPage * 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );
    pfapi->ErrMMFree( pbTest );
    pbTest = NULL;

    wprintf( L"\tTesting basic mem test APIs on COW'd (5th/Last) / writable MM: %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 128 ), cbPage * 5, (void**)&pbTest ) );
    AtomicExchangeAdd( (LONG * const)( pbTest + cbPage * 4 ), 0 );
    OSTestCheck( !FOSMemoryPageAllocated( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMapped( pbTest, cbPage * 5 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest, cbPage * 4 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest + 4 * cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );
    pfapi->ErrMMFree( pbTest );
    pbTest = NULL;

HandleError:

    g_rgwchSysDriveTesting[0] = L'\0';

    pfapi->ErrMMFree( pbMM );
    pfapi->ErrMMFree( pbMMC );
    pfapi->ErrMMFree( pbMMR );
    pfapi->ErrMMFree( pbCOW );
    pfapi->ErrMMFree( pbMMP );
    pfapi->ErrMMFree( pbReMM );

    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}



BOOL FTestCheckResidentViaScan( void * const pv, ULONG cb )
{
    ERR         err         = JET_errSuccess;
    IBitmapAPI* pbmapi      = NULL;
    DWORD       cUpdate     = 0;

    Call( ErrOSMemoryPageResidenceMapScanStart( cb, &cUpdate ) );

    Call( ErrOSMemoryPageResidenceMapRetrieve( pv, cb, &pbmapi ) );

    for ( size_t iVMPage = 0; iVMPage < ( cb / OSMemoryPageCommitGranularity() ); iVMPage++ )
    {
        BOOL            fResident   = fTrue;
        IBitmapAPI::ERR errBM       = IBitmapAPI::ERR::errSuccess;
    
    
        errBM = pbmapi->ErrGet( iVMPage, &fResident );
        if ( errBM != IBitmapAPI::ERR::errSuccess || fResident )
        {
            OSMemoryPageResidenceMapScanStop();
            return fTrue;
        }
    }

    OSMemoryPageResidenceMapScanStop();

HandleError:

    return fFalse;
}



CUnitTest( OSLayerFOSMemoryPageCommitTest, 0, "Tests FOSMemoryPageCommit()s size granularity and initial state." );
ERR OSLayerFOSMemoryPageCommitTest::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPageGranularity = OSMemoryPageCommitGranularity();
    const DWORD cbAlloc = 2 * cbPageGranularity;
    BYTE* pvPattern0s = new BYTE[ cbAlloc ];
    BYTE* pvPattern1s = new BYTE[ cbAlloc ];

    srand( TickOSTimeCurrent() );

    memset( pvPattern0s, 0, cbAlloc );
    memset( pvPattern1s, 1, cbAlloc );


    Assert( cbAlloc == 2 * cbPageGranularity );


    OSTestCheck( !FOSMemoryPageCommit( NULL, cbAlloc ) );


    void* pv = PvOSMemoryPageReserve( cbAlloc, NULL );

    wprintf( L"Mem Status Info of PvOSMemoryPageReserve():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );


    OSTestCheck( !FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryPageResident( pv, cbAlloc ) );
    OSTestCheck( !FTestCheckResidentViaScan( pv, cbAlloc ) );

    OSTestCheck( FOSMemoryPageCommit( pv, cbAlloc ) );

    wprintf( L"Mem Status Info of FOSMemoryPageCommit():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );


    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryPageResident( pv, cbAlloc ) );
    OSTestCheck( !FTestCheckResidentViaScan( pv, cbAlloc ) );


    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );

    wprintf( L"Mem Status Info post deref:\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );
    OSTestCheck( FOSMemoryPageResident( pv, cbAlloc ) );
    OSTestCheck( FTestCheckResidentViaScan( pv, cbAlloc ) );


    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    void* pvMiddle = PvOSMemoryPageReserve( cbAlloc, (BYTE*)pv + cbAlloc / 2 );
    OSTestCheck( pvMiddle == NULL );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 ) );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    OSMemoryPageReset( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2, fFalse );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    memset( pv, 0, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    OSMemoryPageReset( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2, fTrue );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    memset( pv, 0, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    OSMemoryPageDecommit( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 );
    OSTestCheck( FNotAVdWritingToAddressRangeAll( pv, cbAlloc / 2 ) );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    OSTestCheck( FAVdWritingToAddressRangeMost( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 ) );


    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 ) );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    OSTestCheck( memcmp( (BYTE*)pv + cbAlloc / 2, pvPattern0s, cbAlloc / 2 ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    OSMemoryPageDecommit( (BYTE*)pv + cbPageGranularity / 2, 1 );
    OSTestCheck( FNotAVdWritingToAddressRangeAll( (BYTE*)pv + cbPageGranularity, cbAlloc - cbPageGranularity ) );
    OSTestCheck( memcmp( (BYTE*)pv + cbPageGranularity, pvPattern1s, cbAlloc - cbPageGranularity ) == 0 );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbPageGranularity ) );

    
    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbPageGranularity / 2 + 1, 1 ) );
    OSTestCheck( memcmp( pv, pvPattern0s, cbPageGranularity ) == 0 );
    OSTestCheck( memcmp( (BYTE*)pv + cbPageGranularity, pvPattern1s, cbAlloc - cbPageGranularity ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );


    OSMemoryPageDecommit( (BYTE*)pv + cbAlloc / 2 - 1, 2 );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbAlloc ) );


    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbAlloc / 2 - 1, 2 ) );
    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    OSMemoryPageFree( pv );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbAlloc ) );

HandleError:

    delete[] pvPattern0s;
    delete[] pvPattern1s;

    return err;
}


VOID MemTestCacheTerm( void ** rgpvChunks, size_t cChunks );
ERR ErrMemTestCacheInit( void ** rgpvChunks, size_t cChunks );

size_t cbCacheTotal = 0;

extern double g_dblQWSExTimeLast;
extern INT g_cQWSExCalls;

size_t g_cHyperCacheNonResidentOSPages;


ERR ErrTestOSMemoryPageResidentMapCheck( void ** rgpvChunks, INT cChunks, double * pdblScanTime, double * pdblQWSExTime )
{
    ERR err = JET_errSuccess;
    DWORD dwUpdateId;

    INT cTestRuns = 0;

TestRetry:

    const size_t cbChunk = cbCacheTotal / cChunks;

    size_t cResidentPages = 0;
    size_t cNonResidentPages = 0;

    g_dblQWSExTimeLast = 0.0;
    const HRT hrtBegin = HrtHRTCount();

    Call( ErrOSMemoryPageResidenceMapScanStart( cbChunk, &dwUpdateId ) );

    for( INT iChunk = 0; iChunk < cChunks; iChunk++ )
    {
        IBitmapAPI * pbmapi = NULL;
        Call( ErrOSMemoryPageResidenceMapRetrieve( rgpvChunks[iChunk], cbChunk, &pbmapi ) );
        Assert( pbmapi );


        for( size_t ibPage = 0; ibPage < cbChunk; ibPage += OSMemoryPageCommitGranularity() )
        {
            BOOL fResident = fFalse;
            IBitmapAPI::ERR errBM = pbmapi->ErrGet( ibPage / OSMemoryPageCommitGranularity(), &fResident );
            if ( errBM != IBitmapAPI::ERR::errSuccess )
            {
                printf( "ErrGet() returned %d\n", errBM );
                Assert( !"Huh2?" );
                Error( ErrERRCheck( JET_errOutOfMemory ) );
            }
            if ( fResident )
            {
                cResidentPages++;
            }
            else
            {
                cNonResidentPages++;
            }
        }
    }

    printf ( "QWSEx: Resident/NonResident = %I64d / %I64d; cCacheTotal/cExpectedNonResident = %I64d / %I64d\n", 
            cResidentPages, cNonResidentPages, ( cbCacheTotal / OSMemoryPageCommitGranularity() ), g_cHyperCacheNonResidentOSPages );

    if ( ( ( cbCacheTotal / OSMemoryPageCommitGranularity() ) != ( cResidentPages + cNonResidentPages ) ) ||
        ( g_cHyperCacheNonResidentOSPages != cNonResidentPages ) )
    {
        printf ( "\n\tERROR: Resident and NonResident values seem to have shifted!  Try again.\n\n" );
        if( cTestRuns < 5 )
        {
            cTestRuns++;
            wprintf( L"Failed run, possibly due to memory usage interference, %d failed runs so far\n", cTestRuns );
            OSMemoryPageResidenceMapScanStop();
            MemTestCacheTerm( rgpvChunks, 1 );
            OSTestCall( ErrMemTestCacheInit( rgpvChunks, 1 ) );
            Assert( cTestRuns < 4 );
            goto TestRetry;
        }
        Error( ErrERRCheck( errNotFound ) );
    }

HandleError:
    OSMemoryPageResidenceMapScanStop();

    *pdblScanTime = DblHRTElapsedTimeFromHrtStart( hrtBegin );
    *pdblQWSExTime = g_dblQWSExTimeLast;

    return err;
}

const BOOL fUseHyperCache = fTrue;

ERR ErrMemTestCacheInit( void ** rgpvChunks, size_t cChunks )
{
    ERR err = JET_errSuccess;

    MEMORYSTATUS memstatusT;
    GlobalMemoryStatus( &memstatusT );
    SYSTEM_INFO sinfT;
    GetNativeSystemInfo( &sinfT );
    cbCacheTotal = memstatusT.dwAvailPhys / 4;
#ifdef _WIN64
    cbCacheTotal = min( cbCacheTotal, 6LL * 1024 * 1024 * 1024 );
#endif
    cbCacheTotal = rounddn( cbCacheTotal, sinfT.dwPageSize * cChunks );
    printf( "\tcbCacheTotal = %I64d\n", cbCacheTotal );

    printf( "\tReserving some memory...\n" );

    Assert( ( cbCacheTotal % cChunks ) == 0 );

    const size_t cbChunk = cbCacheTotal / cChunks;

    size_t cbTotal;
    for( cbTotal = 0; cbTotal < cbCacheTotal; cbTotal += cbChunk )
    {
        Alloc( rgpvChunks[cbTotal/cbChunk] = PvOSMemoryPageReserve( cbChunk, NULL ) );
        printf( "\t\trgpvChunk[%d] = %p\n", cbTotal/cbChunk, rgpvChunks[cbTotal/cbChunk] );
    }

    printf( "\tCommitting some memory...\n" );

    for( size_t iChunk = 0; iChunk < cChunks; iChunk++ )
    {
        if ( !FOSMemoryPageCommit( rgpvChunks[iChunk], cbChunk ) )
        {
            printf( "Chunk commit failure!\n" );
            Error( ErrERRCheck( JET_errOutOfMemory ) );
        }
    }

    printf( "\tUsing some memory...\n" );

    for( size_t iChunk = 0; iChunk < cChunks; iChunk++ )
    {
        for( size_t cbPage = 0; cbPage < cbChunk; cbPage += OSMemoryPageCommitGranularity() )
        {
            *(INT*)( (BYTE*)rgpvChunks[iChunk] + cbPage ) = 0x42;
        }
    }

    size_t cDehydrated = 0;
    size_t cbDehydrated = 0;

    if ( fUseHyperCache )
    {
        printf( "\tHyper-Caching the memory...\n" );

        srand( 0x42 );
        for( size_t iChunk = 0; iChunk < cChunks; iChunk++ )
        {
            for( size_t ibEsePage = 0; ibEsePage < ( cbChunk - 32 * 1024 - 7 ); ibEsePage += ( 32 * 1024 ) )
            {
                BYTE * pvPage = (BYTE*)( (BYTE*)rgpvChunks[iChunk] + ibEsePage );
                if ( ( rand() % 5 ) == 0  )
                {
                    const INT i = 1 + ( rand() % 10 );
                    const size_t cbShrink =
                            size_t( 
                            ( i <= 7 ) ? 7  : ( ( i <= 8 ) ? 6  : 4  )
                            )* OSMemoryPageCommitGranularity();
                    OSMemoryPageDecommit( pvPage + (32 * 1024) - cbShrink, cbShrink );
                    cbDehydrated += cbShrink;
                    cDehydrated++;
                }
            }
        }
    }

    g_cHyperCacheNonResidentOSPages = cbDehydrated / OSMemoryPageCommitGranularity();
    printf( "\t\tcDehydrated = %I64d, cbDehydrated = %I64d, g_cHyperCacheNonResidentOSPages = %I64d\n", cDehydrated, cbDehydrated, g_cHyperCacheNonResidentOSPages );
    

    printf( "\tcbCacheReserve = %I64d MB, cbCacheSize = %I64d MB\n", cbCacheTotal / 1024 / 1024, ( cbCacheTotal - cbDehydrated ) / 1024 / 1024 );

HandleError:

    return err;
}

VOID MemTestCacheTerm( void ** rgpvChunks, size_t cChunks )
{
    const size_t cbChunk = cbCacheTotal / cChunks;

    for( size_t iChunk = 0; iChunk < cChunks; iChunk++ )
    {
        OSMemoryPageDecommit( rgpvChunks[iChunk], cbChunk );
        OSMemoryPageFree( rgpvChunks[iChunk] );
    }

}

extern size_t g_cbQWSChunkSize;
extern INT g_cmsecQWSThrottle;

ERR ErrTestOSMemoryResidentMapPerf( void ** rgpvChunks, size_t cChunks )
{
    ERR err = JET_errSuccess;

    double rgdblScanTimes[5];
    double rgdblQWSExTimes[5];


    Call( ErrTestOSMemoryPageResidentMapCheck( rgpvChunks, cChunks, &(rgdblScanTimes[0]), &(rgdblQWSExTimes[0]) ) );
    Call( ErrTestOSMemoryPageResidentMapCheck( rgpvChunks, cChunks, &(rgdblScanTimes[1]), &(rgdblQWSExTimes[1]) ) );

    Call( ErrTestOSMemoryPageResidentMapCheck( rgpvChunks, cChunks, &(rgdblScanTimes[2]), &(rgdblQWSExTimes[2]) ) );

    Call( ErrTestOSMemoryPageResidentMapCheck( rgpvChunks, cChunks, &(rgdblScanTimes[3]), &(rgdblQWSExTimes[3]) ) );

    Call( ErrTestOSMemoryPageResidentMapCheck( rgpvChunks, cChunks, &(rgdblScanTimes[4]), &(rgdblQWSExTimes[4]) ) );

    double dblScanAve = (rgdblScanTimes[0] + rgdblScanTimes[1] + rgdblScanTimes[2] + rgdblScanTimes[3] + rgdblScanTimes[4]) / 5;

    wprintf(L"\tChunk %5d MB (%d kB), Throttle: 0x%08x ms: Scan %2.6f, v2 QWSEx(%6d): %f, %f, %f, %f, %f\n",
        g_cbQWSChunkSize / 1024 / 1024, g_cbQWSChunkSize / 1024, g_cmsecQWSThrottle, 
        dblScanAve, g_cQWSExCalls,
        rgdblQWSExTimes[0], rgdblQWSExTimes[1], rgdblQWSExTimes[2], rgdblQWSExTimes[3], rgdblQWSExTimes[4] );
    printf( " Indiv Scan Times: %f %f %f %f %f \n", rgdblScanTimes[0], rgdblScanTimes[1],  rgdblScanTimes[2], rgdblScanTimes[3], rgdblScanTimes[4] );

HandleError:

    return err;
}

extern CSparseBitmap*   psbm;

CUnitTest( OSLayerResidentMapPerf, 0, "Tests performance of OSLayer residence map scan APIs" );
ERR OSLayerResidentMapPerf::ErrTest()
{
    ERR err = JET_errSuccess;

    BOOL fRanAtLeastOne = fFalse;

    COSLayerPreInit oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        goto HandleError;
    }

    void * rgpvChunks[128] = { 0 };

    printf( "Total RAM: %I64d\n\n", OSMemoryTotal() );


    printf( "Setting up the \"cache\" w/ %d chunks for %d GBs\n", 1, cbCacheTotal / 1024 / 1024 / 1024 );
    OSTestCall( ErrMemTestCacheInit( rgpvChunks, 1 ) );

    printf( "Testing residency as 1 single chunk ...\n" );

    g_cbQWSChunkSize = cbCacheTotal;
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, 1 ) );

    g_cbQWSChunkSize = cbCacheTotal - OSMemoryPageCommitGranularity();
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, 1 ) );

    g_cbQWSChunkSize = cbCacheTotal + OSMemoryPageCommitGranularity();
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, 1 ) );
        
    if ( cbCacheTotal > (size_t)1 * 1024 * 1024 * 1024 )
    {
        g_cbQWSChunkSize = (size_t)1 * 1024 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, 1 ) );
    }

    g_cbQWSChunkSize = cbCacheTotal;
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, 1 ) );

    printf( "DONE!!\n" );

    MemTestCacheTerm( rgpvChunks, 1 );



    printf( "Setting up the \"cache\" w/ %d chunks for %d GBs\n", _countof(rgpvChunks), cbCacheTotal / 1024 / 1024 / 1024 );
    OSTestCall( ErrMemTestCacheInit( rgpvChunks, _countof(rgpvChunks) ) );

    if ( g_cbQWSChunkSize > (size_t)1024 * 1024 * 1024 )
    {
        goto Test1G;
    }
    if ( g_cbQWSChunkSize > (size_t)512 * 1024 * 1024 )
    {
        goto Test512M;
    }
    if ( g_cbQWSChunkSize > (size_t)256 * 1024 * 1024 )
    {
        goto Test256M;
    }
    if ( g_cbQWSChunkSize > (size_t)128 * 1024 * 1024 )
    {
        goto Test128M;
    }
    if ( g_cbQWSChunkSize > (size_t)64 * 1024 * 1024 )
    {
        goto Test64M;
    }
    if ( g_cbQWSChunkSize > (size_t)32 * 1024 * 1024 )
    {
        goto Test32M;
    }
    if ( g_cbQWSChunkSize > (size_t)16 * 1024 * 1024 )
    {
        goto Test16M;
    }
    if ( g_cbQWSChunkSize > (size_t)8 * 1024 * 1024 )
    {
        goto Test8M;
    }
    if ( g_cbQWSChunkSize > (size_t)4 * 1024 * 1024 )
    {
        goto Test4M;
    }
    if ( g_cbQWSChunkSize > (size_t)2 * 1024 * 1024 )
    {
        goto Test2M;
    }
    if ( g_cbQWSChunkSize > (size_t)1 * 1024 * 1024 )
    {
        goto Test1M;
    }
    if ( g_cbQWSChunkSize > (size_t)512 * 1024 )
    {
        goto Test512K;
    }
    if ( g_cbQWSChunkSize > (size_t)256 * 1024 )
    {
        goto Test256K;
    }
    if ( g_cbQWSChunkSize > (size_t)128 * 1024 )
    {
        goto Test128K;
    }
    if ( g_cbQWSChunkSize > (size_t)64 * 1024 )
    {
        goto Test64K;
    }
    goto Test4K;

Test1G:
    g_cbQWSChunkSize = (size_t)1024 * 1024 * 1024;
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );

    g_cbQWSChunkSize = (size_t)1024 * 1024 * 1024 - OSMemoryPageCommitGranularity();
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );

Test512M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)512 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test256M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)256 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test128M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)128 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test64M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)64 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test32M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)32 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test16M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)16 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test8M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)8 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test4M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)4 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test2M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)2 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test1M:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)1 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test512K:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)512 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test256K:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)256 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test128K:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)128 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test64K:
    if ( !fRanAtLeastOne || ( rand() % 100 ) < PercentToRunQueryWorkingSetEx() )
    {
        g_cbQWSChunkSize = (size_t)64 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
        fRanAtLeastOne = fTrue;
    }

Test4K:
    Assert( g_cbQWSChunkSize > (size_t)4 * 1024 );
    printf( "\tDrop all the way to 1 page...\n" );
    g_cbQWSChunkSize = (size_t)4 * 1024;
    Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );

    if ( cbCacheTotal > (size_t)1024 * 1024 * 1024 )
    {
        g_cbQWSChunkSize = (size_t)1024 * 1024 * 1024;
        Call( ErrTestOSMemoryResidentMapPerf( rgpvChunks, _countof(rgpvChunks) ) );
    }

    printf( "DONE!!\n" );

    MemTestCacheTerm( rgpvChunks, _countof(rgpvChunks) );

HandleError:
    srand( (DWORD)HrtHRTCount() );

    if ( err )
    {
        printf ( " Something failed ... %d ... PefLastThrow: %d \n", err, PefLastThrow()->UlLine() );
    }

    return err;
}


CUnitTest( ValidateFAllZeros, 0, "Validates FUtilZeroed." );
ERR ValidateFAllZeros::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE pb[ 32 ] = { 0 };

    OSTestCheck( FUtilZeroed( pb, 0 ) );

    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ibToChange = cb - 1;

        memset( pb, 0, sizeof( pb ) );

        OSTestCheck( FUtilZeroed( &pb[0], cb ) );

        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[0], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[0], cb ) );

        if ( ( ibToChange + 1 ) < sizeof( pb ) )
        {
            pb[ ibToChange + 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[0], cb ) );
        }

        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[0], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[0], cb ) );
    }

    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ibToChange = sizeof( pb ) - cb;

        memset( pb, 0, sizeof( pb ) );

        OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );

        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );

        if ( ibToChange > 0 )
        {
            pb[ ibToChange - 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );
        }

        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );
    }

    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ibToChange1 = ( sizeof( pb ) / 2 ) - ( cb / 2 );
        const size_t ibToChange2 = ( sizeof( pb ) / 2 ) + ( ( cb - 1 ) / 2 );
        OSTestCheck( ibToChange2 >= ibToChange1 );
        OSTestCheck( ( ibToChange2 - ibToChange1 + 1 ) == cb );

        memset( pb, 0, sizeof( pb ) );

        OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );

        pb[ ibToChange1 ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange1], cb ) );
        pb[ ibToChange2 ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange1], cb ) );
        pb[ ibToChange1 ] = 0;
        pb[ ibToChange2 ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );

        if ( ibToChange1 > 0 )
        {
            pb[ ibToChange1 - 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );
        }

        if ( ( ibToChange2 + 1 ) < sizeof( pb ) )
        {
            pb[ ibToChange2 + 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );
        }

        pb[ ibToChange1 ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange1], cb ) );
        pb[ ibToChange2 ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange1], cb ) );
        pb[ ibToChange1 ] = 0;
        pb[ ibToChange2 ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );
    }

HandleError:
    return err;
}


CUnitTest( ValidateIbLastNonZeroedByte, 0, "Validates IbUtilLastNonZeroed." );
ERR ValidateIbLastNonZeroedByte::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE pb[ 32 ] = { 0 };

    OSTestCheck( IbUtilLastNonZeroed( pb, 0 ) == 0 );

    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ib = cb - 1;

        memset( pb, 0, sizeof( pb ) );

        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == cb );

        if ( ( ib + 1 ) < sizeof( pb ) )
        {
            pb[ ib + 1 ] = 1;
            OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == cb );
            pb[ ib + 1 ] = 0;
        }

        pb[ 0 ] = 1;
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == 0 );

        pb[ ib / 5 ] = 1;
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ( ib / 5 ) );

        pb[ ib / 4 ] = 1;
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ( ib / 4 ) );

        pb[ ib / 3 ] = 1;
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ( ib / 3 ) );

        pb[ ib / 2 ] = 1;
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ( ib / 2 ) );

        pb[ ib ] = 1;
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ib );

        if ( ( ib + 1 ) < sizeof( pb ) )
        {
            pb[ ib + 1 ] = 1;
            OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ib );
        }
    }

HandleError:
    return err;
}


