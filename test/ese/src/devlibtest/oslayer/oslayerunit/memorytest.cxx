// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"


//  There is no config override injection for complex or vector types, so just reach into
//  the OS layer and config override it myself.
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

// Important: By not calling ErrOSInit() we are asserting that the
// OS Layer memory handling facilities work pre-ErrOSInit().
//  OSTestCall( ErrOSInit() );

    COSLayerPreInit oslayer; // FOSPreinit()

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    //  Setting the "Mem Check" debug registry key to "1" is required in order to guarantee this.
    //  We are assuming that the test wrapper that invokes this test will
    //  take care of that.
    // 
    OSTestCheck( g_fMemCheck );

    wprintf( L"\t\tTesting operator new line tracking ...\n" );

    //  Do not separate the next two lines!
    const ULONG ulNewLineExpected = __LINE__ + 1;
    pul = new ULONG;

    if ( NULL == pul )
    {
        wprintf( L"\t\tFailed new alloc!\n" );
        goto HandleError;
    }

    wprintf( L"\t\t\t\tWe had a new @ %hs : %d\n", SzNewFile(), UlNewLine() );
    OSTestCheck( _stricmp( __FILE__, SzNewFile() ) == 0 );
    OSTestCheck( ulNewLineExpected == UlNewLine() );

    //  success!
    err = JET_errSuccess;

HandleError:

    delete pul;
    
    return err;
}
#endif

//  fFalse if writing to at least one byte does not AV (checks most, not all bytes).

bool FAVdWritingToAddressRangeMost( void* pv, DWORD cb )
{
    bool fAVd = true;

    /*
    for ( DWORD ib = 0; ib < cb && fAVd; ib = ib + ( rand() % 1024 ) )
    {
        TRY
        {
            BYTE* pvByte = (BYTE*)pv + ib;
            memset( pvByte, *pvByte, 1 );
            fAVd = false;
        }
        EXCEPT( efaExecuteHandler )
        {
        }
    }
        */

    //  check last byte explicitly

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

//  fFalse if writing to at least one byte AVs.

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

//  Tests OSMemoryPageCommitGranularity().

CUnitTest( TestOSMemoryPageCommitGranularity, 0, "Validates the page commit granularity is expected power of 2." );
ERR TestOSMemoryPageCommitGranularity::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    const DWORD cbPageGranularity = OSMemoryPageCommitGranularity();

    //  Basic assumptions about page granularity.

    OSTestCheck( cbPageGranularity >= 4 );
    OSTestCheck( FPowerOf2( cbPageGranularity ) );
    OSTestCheck( cbPageGranularity <= 4 * 1024 ); 

HandleError:

    return err;
}

//  Tests PvOSMemoryPageAlloc().

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

    //  This is implicitly assumed throughout, so make sure it doesn't change.

    OSTestCheck( cbAlloc == 2 * cbPageGranularity );

    //  Should be page-granularity aligned and completely zeroed out.

    void* pv = PvOSMemoryPageAlloc( cbAlloc, NULL );

    wprintf( L"Mem Status Info of PvOSMemoryPageAlloc():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    //  Test buffer has expected geometry

    OSTestCheck( pv != NULL );
    OSTestCheck( ( (size_t)pv % cbPageGranularity ) == 0 );

    //  Test APIs match state

    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );

    //  Test residency (haven't deref'd yet, so non-resident)

    OSTestCheck( !FOSMemoryPageResident( pv, cbAlloc ) );

    //  Test that it is zero filled memory

    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );

    //  Test residency (haven't deref'd yet, so non-resident)

    OSTestCheck( FOSMemoryPageResident( pv, cbAlloc ) );

    wprintf( L"Mem Status Info after deref:\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    //  Test APIs (still) match state

    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );

    //  Make sure we can write and read from/to it.

    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  It is invalid to re-allocate it.

    void* pvMiddle = PvOSMemoryPageAlloc( cbAlloc, (BYTE*)pv + cbAlloc / 2 );
    OSTestCheck( pvMiddle == NULL );

    OSMemoryPageFree( pv );
    
HandleError:

    delete[] pvPattern0s;
    delete[] pvPattern1s;

    return err;
}

//  Tests PvOSMemoryPageReserve().

CUnitTest( OSLayerPvOSMemoryPageReserveTest, 0, "Tests FTestPvOSMemoryPageReserve()s size granularity and initial state." );
ERR OSLayerPvOSMemoryPageReserveTest::ErrTest()
{
    COSLayerPreInit oslayer;
    JET_ERR err = JET_errSuccess;
    const DWORD cbPageGranularity = OSMemoryPageCommitGranularity();
    const DWORD cbAlloc = 2 * cbPageGranularity;

    //  This is implicitly assumed throughout, so make sure it doesn't change.

    OSTestCheck( cbAlloc == 2 * cbPageGranularity );

    //  Should be page-granularity aligned and we should not be able to write to it (it's not committed).

    void* pv = PvOSMemoryPageReserve( cbAlloc, NULL );

    wprintf( L"Mem Status Info of PvOSMemoryPageReserve():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    OSTestCheck( pv != NULL );
    OSTestCheck( ( (size_t)pv % cbPageGranularity ) == 0 );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbAlloc ) );

    //  Test APIs match state

    OSTestCheck( !FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );

    //  It is invalid to re-reserve it.

    void* pvMiddle = PvOSMemoryPageReserve( cbAlloc, (BYTE*)pv + cbAlloc / 2 );
    OSTestCheck( pvMiddle == NULL );

    OSMemoryPageFree( pv );
    
HandleError:

    return err;
}

//  This is a test version of FOSMemoryPageAllocated() that simply returns true if ANY page,
//  instead of all or every page having to be in an "allocated" state.

BOOL FTestOSMemoryAnyPageAllocated( const BYTE * const pb, const DWORD cb )
{
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    for ( DWORD cbDone = 0; ( cbDone + cbPage ) <= cb; cbDone += cbPage )
    {
        if ( FOSMemoryPageAllocated( pb + cbDone, cbPage ) )
        {
            //wprintf( L"Done FTestOSMemoryAnyPageAllocated() --> fTrue\n ");
            return fTrue;
        }
    }
    //wprintf( L"Done !FTestOSMemoryAnyPageAllocated() --> fFalse\n ");
    return fFalse;
}

CUnitTest( OSLayerFOSMemoryPageAllocatedRequiresAllAllocatedPages, 0, "Tests if functions like FOSMemoryPageAllocated() return the right answers when not all the memory answers the same." );
ERR OSLayerFOSMemoryPageAllocatedRequiresAllAllocatedPages::ErrTest()
{
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();
    const DWORD cbReserve   = OSMemoryPageReserveGranularity();

    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    srand( TickOSTimeCurrent() );

    //  We should be able to reserve, commit and verify that the region is completely zeroed out.

    //
    //  This is the memory we're setting up (broken out by commit granularity) ...
    //  Note: On IA64 due to the 16 KB commit granularity, these offsets KB would be x 4, but
    //  I've worked it out so it _should_ still all work.
    //      Page 0      :   Reserved
    //      Page 1/4KB  :   Reserved
    //          ...     :   ...
    //      Page 15/60KB:   Reserved
    //                          --- + cbReserve ---
    //      Page 16/64KB:   Reserved+Committed
    //      Page 17/68KB:   Reserved+Committed
    //       [Part 2a/b]:   Reserved+Committed  ( - we ReadDeref and WriteDeref for a/b respectively)
    //          [Part 3]:   Reserved            ( - we Decommitted this page)
    //          [Part 5]:   Reserved+Committed  ( - we Re-Commit this page finally)
    //      Page 18/72KB:   Reserved+Committed
    //         [Part 4+]:   Reserved+Committed+Protected(against write)
    //      Page 19/76KB:   Reserved
    //          ...     :   ...
    //      Page 31/124KB:  Reserved
    //                          --- + cbReserve * 2 ---
    //      Page 32/128KB:  Reserved
    //          ...      :  ...
    //      Page 47/192KB:  Reserved
    //

    DWORD cb = cbReserve * 3;
    BYTE * pb = (BYTE*)PvOSMemoryPageReserve( cbReserve * 3, NULL );

    OSTestCheck( !FTestOSMemoryAnyPageAllocated( pb, cb ) );                        //  This doesn't force allocation

    //  Part 1
    //
    Assert( cbPage * 3 < cbReserve );   //  want to ensure testing w/in a reserve size
    OSTestCheck( fTrue == FOSMemoryPageCommit( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );                         //  some pages now allocated
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );            //  check pages 16 - 18

    //  Part 2a
    //
    volatile BYTE bRead = *( pb + cbReserve + cbPage + 1 );
    OSTestCheck( bRead == 0 );

    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );                         //  NO test changes, but additional tests next ...
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve - cbPage, cbPage * 3 ) );  //  first page is not allocated
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve + cbPage, cbPage * 3 ) );  //  last page is not allocated

    //  Part 2b
    //
    *( pb + cbReserve + cbPage + 1 ) = 0xb;

    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );                         //  NO test changes ...
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );

    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve - cbPage, cbPage * 3 ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve + cbPage, cbPage * 3 ) );

    //  Part 3
    //
    OSMemoryPageDecommit( pb + cbReserve + cbPage, cbPage );
    
    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );
    OSTestCheck( FTestOSMemoryAnyPageAllocated( pb, cb ) );
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );           //  now this should fail
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage ) );                //  check first(well 16th) page of the allocated region.
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );   //  check last(well 18th) page of the allocated region.

    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage ) );                //  page 16 should still be allocated.
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );   //  page 18 should still be allocated.

    //  Part 4  - protecting it against write shouldn't stop it from being declared allocated ...
    //
    OSMemoryPageProtect( pb + cbReserve + cbPage * 2, cbPage );

    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );   //  page 18 should still be allocated.
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );           //  should still fail

    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve + cbPage * 2, cbPage ) );   //  page 18 should still be allocated.
    OSTestCheck( !FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );           //  should still fail

    //  Part 5  - protecting it against write shouldn't stop it from being declared allocated ...
    //
    OSTestCheck( fTrue == FOSMemoryPageCommit( pb + cbReserve + cbPage, cbPage ) );

    OSTestCheck( !FOSMemoryPageAllocated( pb, cb ) );                               //  should still fail
    OSTestCheck( FOSMemoryPageAllocated( pb + cbReserve, cbPage * 3 ) );            //  main 3 pages (16 - 18) should be committed again

HandleError:

    delete pfapi;
    delete pfsapi;
    OSTerm();

    return err;
}

//  Note: I'm going to leave this test in, even though in all honesty it didn't turn out so well, since
//  we just don't have two states reserve and committed ... so my attempt to figure out if all pages are
//  mapped is hindered by not having reserve space before after the buffer I am allocating with ErrMMCopy(),
//  so I'm kind of just assuming I got it right.

CUnitTest( OSLayerFOSMemoryFileMappedRequiresAllMappedPages, 0, "Tests if functions like FOSMemoryFileMapped() return the right answers when not all the memory answers the same." );
ERR OSLayerFOSMemoryFileMappedRequiresAllMappedPages::ErrTest()
{
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();
    const DWORD cbReserve   = OSMemoryPageReserveGranularity();

    //  test mode(s)
    const BOOL fMmReadInstead = fFalse;

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMapFirst = NULL, * pbMapMid = NULL, * pbMapLast = NULL;

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    const INT ipageFirst    = 16;
    const INT ipageMid      = 32+6;     //  Note: The mid is a 3 page map
    const INT ipageLast     = 64-7;

    //  We give ourselves opportunity for in-between buffers to allocate such that we
    //  can force none of these buffers to be next to each other.
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

    //  Actual checks.

    OSTestCheck( FOSMemoryFileMapped( pbMapFirst, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapFirst + cbPage, cbPage ) );      //  Overmapping: Expected false, but get true.
                                                                            //      From here on down there are several checks that
                                                                            //      because of how we overmap at the reserve granularity
                                                                            //      the returned data can be overmapped.    
    OSTestCheck( FOSMemoryFileMapped( pbMapFirst, cbPage * 2 ) );           //  Actual check;  Overmapping: Expected false, but get true.
    //  SOOOO ... I added new checks that shift by a cbReserve to get
    //  off the overmapped section, but if the OS just happens to map 
    //  another file there, we'll still get failures.
    //OnDebug( FNegTestSet( fInvalidUsage ) );
    //OSTestCheck( !FOSMemoryFileMapped( pbMapFirst + cbReserve, cbPage ) );    //  <- this check may not hold!
    //OSTestCheck( !FOSMemoryFileMapped( pbMapFirst, cbPage + cbReserve ) );    //  <- this check may not hold!
    //OnDebug( FNegTestUnset( fInvalidUsage ) );

    OSTestCheck( FOSMemoryFileMapped( pbMapMid, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapMid - cbPage, cbPage ) );        //  Overmapping: Expected false, but get true.
    OSTestCheck( FOSMemoryFileMapped( pbMapMid - cbPage, cbPage * 3 ) );    //  Overmapping: Expected false, but get true.
    //OnDebug( FNegTestSet( fInvalidUsage ) );
    //OSTestCheck( !FOSMemoryFileMapped( pbMapMid - cbReserve, cbPage ) );              //  <- this check may not hold!  Did not, extending it.
    //OSTestCheck( !FOSMemoryFileMapped( pbMapMid - cbReserve * 2, cbReserve * 4 ) );   //  Surrounding should be bad;  <- this check may not hold!
    //OnDebug( FNegTestUnset( fInvalidUsage ) );

    OSTestCheck( FOSMemoryFileMapped( pbMapLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapLast - cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapLast - cbPage, cbPage * 2 ) );   //  At end is bad too.
    //OnDebug( FNegTestSet( fInvalidUsage ) );
    // Failed - randomly walked onto some other MM section?  It's possible.
    //OSTestCheck( !FOSMemoryFileMapped( pbMapLast - cbReserve, cbPage ) );                 //  Overmapping: Expected false, but get true.
    //OSTestCheck( !FOSMemoryFileMapped( pbMapLast - cbReserve, cbReserve + cbPage * 2 ) ); //  At end is bad too;  Overmapping: Expected false, but get true.
    //OSTestCheck( !FOSMemoryFileMapped( pbMapLast - cbReserve * 2, cbReserve * 4 ) );        //  Doingsuper large range.
    //OnDebug( FNegTestUnset( fInvalidUsage ) );

    // Double check for copy state as well...

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
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMapCopyFirst = NULL, * pbMapCopyMid = NULL, * pbMapCopyLast = NULL;

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );
    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) ); // required or ErrMMRevert() won't actually revert to a !fCow'd page.
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    const INT ipageFirst    = 16;
    const INT ipageMid      = 32 + 2;
    const INT ipageLast     = 48 + 12;

    OSTestCall( pfapi->ErrMMCopy( ipageFirst * cbPage, cbPage * 4, (void**)&pbMapCopyFirst ) );
    OSTestCall( pfapi->ErrMMCopy( ipageMid * cbPage, cbPage * 4, (void**)&pbMapCopyMid ) );
    OSTestCall( pfapi->ErrMMCopy( ipageLast * cbPage, cbPage * 4, (void**)&pbMapCopyLast ) );

    //  Actual checks.

    OSTestCheck( FOSMemoryFileMapped( pbMapCopyFirst, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapCopyFirst, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    pbMapCopyFirst[1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );          //  Actual check.

    OSTestCheck( FOSMemoryFileMapped( pbMapCopyMid, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    pbMapCopyMid[cbPage * 2 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );            //  Actual check.

    OSTestCheck( FOSMemoryFileMapped( pbMapCopyLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMapped( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    pbMapCopyLast[cbPage * 3 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );           //  Actual check.

    //  Test we can revert the state and mem test operations are reverted

    if ( !FUtilOsIsGeq( 6, 2 ) )
    {
        wprintf(L"Feature ErrMMRevert() is not available on Win NT %d.%d, skipping remainder of test.\n", DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor() );
        err = JET_errSuccess;
        goto HandleError;
    }

    //  Test only the revert to first page (of cbPage * 4, fixes copied state
    OSTestCall( pfapi->ErrMMRevert( ipageFirst * cbPage, pbMapCopyFirst, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );

    //  Test that subsumming revert works as intended
    OSTestCall( pfapi->ErrMMRevert( ipageMid * cbPage, pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );

    //  Test that a bit off target revert doesn't fix cow state
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );

    //  Test we can re-cow after we did a MM revert ...

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
    COSLayerPreInit oslayer; // FOSPreinit()
    JET_ERR err = JET_errSuccess;
    const DWORD cbPage      = OSMemoryPageCommitGranularity();

    WCHAR * wszFileDat = L".\\MappableFile.dat";
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;

    BYTE * pbMapCopyFirst = NULL, * pbMapCopyMid = NULL, * pbMapCopyLast = NULL;

    Call( ErrOSInit() );
    Call( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );
    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) ); // required or ErrMMRevert() won't actually revert to a !fCow'd page.
    Call( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    const INT ipageFirst    = 16;
    const INT ipageMid      = 32 + 2;
    const INT ipageLast     = 48 + 12;

    OSTestCall( pfapi->ErrMMCopy( ipageFirst * cbPage, cbPage * 4, (void**)&pbMapCopyFirst ) );
    OSTestCall( pfapi->ErrMMCopy( ipageMid * cbPage, cbPage * 4, (void**)&pbMapCopyMid ) );
    OSTestCall( pfapi->ErrMMCopy( ipageLast * cbPage, cbPage * 4, (void**)&pbMapCopyLast ) );

    //  Setup three COW'd pages

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    OSTestCheck( pbMapCopyFirst[1] != 0x42 );               
    OSTestCheck( pbMapCopyFirst[1] == (BYTE)ipageFirst );   //  we'll actually be checking in two ways below (to be extra sure)
    const BYTE bFirst = pbMapCopyFirst[1];                  //  save for checking below
    pbMapCopyFirst[1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] != 0x42 );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == (BYTE)ipageMid + 2 );
    const BYTE bMid = pbMapCopyMid[cbPage * 2 + 1];         //  save for checking below
    pbMapCopyMid[cbPage * 2 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] != 0x42 );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == (BYTE)ipageLast + 3 );
    const BYTE bLast = pbMapCopyLast[cbPage * 3 + 1];       //  save for checking below
    pbMapCopyLast[cbPage * 3 + 1] = 0x42;
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );

    //  Now actually test ErrMMRevert() ...

    if ( !FUtilOsIsGeq( 6, 2 ) )
    {
        wprintf(L"Feature ErrMMRevert() is not available on Win NT %d.%d, skipping remainder of test.\n", DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor() );
        err = JET_errSuccess;
        goto HandleError;
    }

    //  Test only the revert to first page (of cbPage * 4, fixes copied state
    OSTestCheck( pbMapCopyFirst[1] == 0x42 );               //  value should be stable
    OSTestCall( pfapi->ErrMMRevert( ipageFirst * cbPage, pbMapCopyFirst, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyFirst, cbPage * 4 ) );
    OSTestCheck( pbMapCopyFirst[1] != 0x42 );                               //  actual checks
    OSTestCheck( pbMapCopyFirst[1] == (BYTE)ipageFirst );                   //  and the full check
    OSTestCheck( pbMapCopyFirst[1] == bFirst );                             //  double check

    //  Test that subsumming revert works as intended
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == 0x42 );    //  value should be stable
    OSTestCall( pfapi->ErrMMRevert( ipageMid * cbPage, pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyMid, cbPage * 4 ) );
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] != 0x42 );                    //  actual checks
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == (BYTE)ipageMid + 2 );      //  and the full check
    OSTestCheck( pbMapCopyMid[cbPage * 2 + 1] == bMid );                    //  double check

    //  Test that a bit off target revert doesn't fix cow state
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == 0x42 );   //  value should be stable
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    //  Now, revert that page for real ...
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == 0x42 );   //  value should STILL be stable
    OSTestCall( pfapi->ErrMMRevert( ipageLast * cbPage, pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbMapCopyLast, cbPage * 4 ) );
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] != 0x42 );                   //  actual checks
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == (BYTE)ipageLast + 3 );    //  and the full check
    OSTestCheck( pbMapCopyLast[cbPage * 3 + 1] == bLast );                  //  double check

    //  Now try re-cowing and re-revert ...

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
    COSLayerPreInit oslayer; // FOSPreinit()
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
    OSTestCall( ErrOSFSCreate( NULL/* pinstNil */, &pfsapi ) );
    WCHAR wszFullPath[IFileSystemAPI::cchPathMax];
    Call( pfsapi->ErrPathComplete( wszFileDat, wszFullPath ) );
    Call( pfsapi->ErrPathParse( wszFullPath, g_rgwchSysDriveTesting, NULL, NULL ) ); // required or ErrMMRevert() won't actually revert to a !fCow'd page.
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfCached, &pfapi ) );

    // Tests with the different page states (committed or COW'd) in various places ... 

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
    //AtomicExchangeAdd( (long * const)pbMMP, 0 );  <-- this crashes properly due to the OSMemoryPageProtect() call.

    wprintf( L"\t\tpbReMM, err = %d", pfapi->ErrMMCopy( cbPage * ( 5 + 96 ), cbPage, (void**)&pbReMM ) );
    AtomicExchangeAdd( (LONG * const)pbReMM, 0 );               // same as pbCOW above
    OSTestCheck( FOSMemoryFileMappedCowed( pbReMM, cbPage ) );  //  should be same as pbCOW at this point
    wprintf( L", %d\n", pfapi->ErrMMRevert( cbPage * ( 5 + 96 ), pbReMM, cbPage ) );

    //  Print meminfo

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
    if ( FUtilOsIsGeq( 6, 2 ) ) //  The ErrMMRevert() above only works on Win8+
    {
        OSTestCheck( !FOSMemoryFileMappedCowed( pbReMM, cbPage ) );
    }

    //  This asserts, but I'm not sure why.  It is properly protected above.

    //  We had trouble with 2 KB pages, so fixing that ... 

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
    OSTestCheck( !FOSMemoryFileMappedCowed( pbCOW - cbPage, cbPage ) );         //  we checked these two things above, should still be true.
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW, cbPage ) );   

    wprintf( L"\tChecking some various boundary-esk mis-aligned sizes ...\n" );

    OSTestCheck( !FOSMemoryFileMappedCowed( pbCOW - cbPage / 2, cbPage / 2 ) ); //  2 KB right before COW'd page
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW - cbPage / 2, cbPage ) );      //  half on half off the COW'd page at beginning
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW, cbPage / 2 ) );               //  first half of COW'd page
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW + cbPage / 2, cbPage / 2 ) );  //  second half of COW'd page
    OSTestCheck( FOSMemoryFileMappedCowed( pbCOW + cbPage / 2, cbPage ) );      //  half on half off the COW'd page at end
    OSTestCheck( !FOSMemoryFileMappedCowed( pbCOW + cbPage, cbPage / 2 ) );     //  2 KB right after COW'd page

        
    //  More tests ... 

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
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );  //  whole check (subsumes)
    pfapi->ErrMMFree( pbTest );
    pbTest = NULL;

    wprintf( L"\tTesting basic mem test APIs on COW'd (3rd) / writable MM: %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 128 ), cbPage * 5, (void**)&pbTest ) );
    AtomicExchangeAdd( (LONG * const)( pbTest + cbPage * 2 ), 0 );
    OSTestCheck( !FOSMemoryPageAllocated( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMapped( pbTest, cbPage * 5 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest, cbPage * 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest + 2 * cbPage, cbPage ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest + 3 * cbPage, cbPage * 2 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );  //  whole check (subsumes)
    pfapi->ErrMMFree( pbTest );
    pbTest = NULL;

    wprintf( L"\tTesting basic mem test APIs on COW'd (5th/Last) / writable MM: %d\n", pfapi->ErrMMCopy( cbPage * ( 5 + 128 ), cbPage * 5, (void**)&pbTest ) );
    AtomicExchangeAdd( (LONG * const)( pbTest + cbPage * 4 ), 0 );
    OSTestCheck( !FOSMemoryPageAllocated( pbTest, cbPage * 5 ) );
    OSTestCheck( FOSMemoryFileMapped( pbTest, cbPage * 5 ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pbTest, cbPage * 4 ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest + 4 * cbPage, cbPage ) );
    OSTestCheck( FOSMemoryFileMappedCowed( pbTest, cbPage * 5 ) );  //  whole check (subsumes)
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


//  This is similar to FOSMemoryPageResident(), but uses the (_theoretically_) more efficient scanning
//  method.  It may also give slightly diff results depending upon memory state, if memory is resident
//  in OS memory, but actually outside our processes working set (i.e. page back innable).

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
    
        //  determine if this VM page is resident.  when in doubt, claim
        //  that it is resident
    
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


//  Tests FOSMemoryPageCommit().

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

    //  This is implicitly assumed throughout, so make sure it doesn't change.

    Assert( cbAlloc == 2 * cbPageGranularity );

    //  Can't commit NULL.

    OSTestCheck( !FOSMemoryPageCommit( NULL, cbAlloc ) );

    //  We should be able to reserve, commit and verify that the region is completely zeroed out.

    void* pv = PvOSMemoryPageReserve( cbAlloc, NULL );

    wprintf( L"Mem Status Info of PvOSMemoryPageReserve():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    //  Test APIs match initial reserve state

    OSTestCheck( !FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryPageResident( pv, cbAlloc ) );
    OSTestCheck( !FTestCheckResidentViaScan( pv, cbAlloc ) );   //  this is actually used by BF to test cache residency

    OSTestCheck( FOSMemoryPageCommit( pv, cbAlloc ) );

    wprintf( L"Mem Status Info of FOSMemoryPageCommit():\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    //  Test APIs match state post commit state

    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );       //  this test flipped states
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryPageResident( pv, cbAlloc ) );
    OSTestCheck( !FTestCheckResidentViaScan( pv, cbAlloc ) );

    //  And should be zero'd memory (note this pages it in, so we'll test that as well).

    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );

    wprintf( L"Mem Status Info post deref:\n" );
    PrintMemInfo( 0x0, (BYTE*)pv );

    OSTestCheck( FOSMemoryPageAllocated( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMapped( pv, cbAlloc ) );
    OSTestCheck( !FOSMemoryFileMappedCowed( pv, cbAlloc ) );
    OSTestCheck( FOSMemoryPageResident( pv, cbAlloc ) );        //  now this test flipped states
    OSTestCheck( FTestCheckResidentViaScan( pv, cbAlloc ) );    //  this test flipped too, and this one actually matters because it is used in BF

    //  Make sure we can write and read from/to it.

    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  It is invalid to re-reserve it, yet the data should still be there.

    void* pvMiddle = PvOSMemoryPageReserve( cbAlloc, (BYTE*)pv + cbAlloc / 2 );
    OSTestCheck( pvMiddle == NULL );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  It is vaild to recommit, and the data should remain there.

    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 ) );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  Resetting should not affect our ability to read/write from/to it and the data should be kept.

    OSMemoryPageReset( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2, fFalse );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    memset( pv, 0, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  Same as previous tests, with the fToss flag enabled.

    OSMemoryPageReset( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2, fTrue );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    memset( pv, 0, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern0s, cbAlloc ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  Decommitting descond half of the region should cause us to AV is we try to write to it, yet
    //  the first half should remain intact.

    OSMemoryPageDecommit( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 );
    OSTestCheck( FNotAVdWritingToAddressRangeAll( pv, cbAlloc / 2 ) );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    OSTestCheck( FAVdWritingToAddressRangeMost( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 ) );

    //  Recommit second half, make sure the first half remains intact and second half
    //  is zeroed out. Also, make sure we can read/write from/to it.

    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbAlloc / 2, cbAlloc / 2 ) );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc / 2 ) == 0 );
    OSTestCheck( memcmp( (BYTE*)pv + cbAlloc / 2, pvPattern0s, cbAlloc / 2 ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  Decommitting one byte at half of the page should decommit the entire page.

    OSMemoryPageDecommit( (BYTE*)pv + cbPageGranularity / 2, 1 );
    OSTestCheck( FNotAVdWritingToAddressRangeAll( (BYTE*)pv + cbPageGranularity, cbAlloc - cbPageGranularity ) );
    OSTestCheck( memcmp( (BYTE*)pv + cbPageGranularity, pvPattern1s, cbAlloc - cbPageGranularity ) == 0 );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbPageGranularity ) );

    //  Recommitting one byte at half of the page should recommit the entire page.
    
    OSTestCheck( FOSMemoryPageCommit( (BYTE*)pv + cbPageGranularity / 2 + 1, 1 ) );
    OSTestCheck( memcmp( pv, pvPattern0s, cbPageGranularity ) == 0 );
    OSTestCheck( memcmp( (BYTE*)pv + cbPageGranularity, pvPattern1s, cbAlloc - cbPageGranularity ) == 0 );
    memset( pv, 1, cbAlloc );
    OSTestCheck( memcmp( pv, pvPattern1s, cbAlloc ) == 0 );

    //  Decommitting two bytes on the border of two pages should decommit both pages.

    OSMemoryPageDecommit( (BYTE*)pv + cbAlloc / 2 - 1, 2 );
    OSTestCheck( FAVdWritingToAddressRangeMost( pv, cbAlloc ) );

    //  Recommitting two bytes on the border of two pages should recommit both pages.

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

//  In scenario FTestPvOSMemoryPageReserve, we try to virtual alloc memory of this size
//  Its value is assigned in ErrMemTestCacheInit
size_t cbCacheTotal = 0;

//  QueryWorkingSetEx API elapsed time
extern double g_dblQWSExTimeLast;
//  How many times QueryWorkingSetEx was run to query the give memory
extern INT g_cQWSExCalls;

size_t g_cHyperCacheNonResidentOSPages;

//  Originally we used a single call of QueryWorkingSet to get memory resilience info
//  Its duration is long and has bigger impact to the concurrency of the entire OS
//
//  Now we use multiple calls of QueryWorkingSetEx to get memory resilience info
//  Duration of each call is short and each call impact a smaller block of memory
//  thus has smaller impact to the concurrency of the OS

ERR ErrTestOSMemoryPageResidentMapCheck( void ** rgpvChunks, INT cChunks, double * pdblScanTime, double * pdblQWSExTime )
{
    ERR err = JET_errSuccess;
    DWORD dwUpdateId;

    INT cTestRuns = 0;

TestRetry:

    const size_t cbChunk = cbCacheTotal / cChunks;

    size_t cResidentPages = 0;
    size_t cNonResidentPages = 0;

    g_dblQWSExTimeLast = 0.0; // just to make sure.
    const HRT hrtBegin = HrtHRTCount();

    Call( ErrOSMemoryPageResidenceMapScanStart( cbChunk, &dwUpdateId ) );

    for( INT iChunk = 0; iChunk < cChunks; iChunk++ )
    {
        IBitmapAPI * pbmapi = NULL;
        Call( ErrOSMemoryPageResidenceMapRetrieve( rgpvChunks[iChunk], cbChunk, &pbmapi ) );
        Assert( pbmapi );

        //  Walk and count resident and non-resident

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

    //  check that the resident and non-resident add up to the cache size and that
    //  the non-resident equals what hyper cache purged
    if ( ( ( cbCacheTotal / OSMemoryPageCommitGranularity() ) != ( cResidentPages + cNonResidentPages ) ) ||
        ( g_cHyperCacheNonResidentOSPages != cNonResidentPages ) )
    {
        printf ( "\n\tERROR: Resident and NonResident values seem to have shifted!  Try again.\n\n" );
        if( cTestRuns < 5 )
        {
            cTestRuns++;
            wprintf( L"Failed run, possibly due to memory usage interference, %d failed runs so far\n", cTestRuns );
            OSMemoryPageResidenceMapScanStop();
            //  Need to also re-init "the cache" ...
            MemTestCacheTerm( rgpvChunks, 1 );
            OSTestCall( ErrMemTestCacheInit( rgpvChunks, 1 ) );
            Assert( cTestRuns < 4 );    //  in debug we'll crash first, retail will just error out ...
            goto TestRetry;
        }
        Error( ErrERRCheck( errNotFound ) );    // just in case someone takes the Assert() or goto out.
    }

HandleError:
    OSMemoryPageResidenceMapScanStop();

    *pdblScanTime = DblHRTElapsedTimeFromHrtStart( hrtBegin );
    *pdblQWSExTime = g_dblQWSExTimeLast;

    return err;
}

//  Flag for do this or not: decommit some memory so some memory is non-resident
const BOOL fUseHyperCache = fTrue;

ERR ErrMemTestCacheInit( void ** rgpvChunks, size_t cChunks )
{
    ERR err = JET_errSuccess;

    //  Set 
    MEMORYSTATUS memstatusT;
    GlobalMemoryStatus( &memstatusT );
    SYSTEM_INFO sinfT;
    GetNativeSystemInfo( &sinfT );
    cbCacheTotal = memstatusT.dwAvailPhys / 4;
#ifdef _WIN64
    // Max out at 6 GB. Having a cache much larger than that just slows down the test.
    // Why 6 GB? We'll still get >32bit addresses.
    cbCacheTotal = min( cbCacheTotal, 6LL * 1024 * 1024 * 1024 );
#endif
    cbCacheTotal = rounddn( cbCacheTotal, sinfT.dwPageSize * cChunks );
    printf( "\tcbCacheTotal = %I64d\n", cbCacheTotal );

    printf( "\tReserving some memory...\n" );

    Assert( ( cbCacheTotal % cChunks ) == 0 );

    const size_t cbChunk = cbCacheTotal / cChunks;

    //  We may fail here because we may not have so much continuous memory
    //  But since we are requesting 1/4 of the available memory, not sure
    //  how much chance we may hit the failure alloc. Let's leave it for now
    //  and tune if we hit the failure
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

        srand( 0x42 );  // seed rand so we get fixed memory scenario so between runs it's comparable
        for( size_t iChunk = 0; iChunk < cChunks; iChunk++ )
        {
            for( size_t ibEsePage = 0; ibEsePage < ( cbChunk - 32 * 1024 - 7 ); ibEsePage += ( 32 * 1024 ) )
            {
                BYTE * pvPage = (BYTE*)( (BYTE*)rgpvChunks[iChunk] + ibEsePage );
                if ( ( rand() % 5 ) == 0 /* 20% dehydrated */ )
                {
                    const INT i = 1 + ( rand() % 10 );
                    const size_t cbShrink =
                            size_t( 
                            ( i <= 7 ) ? 7 /* 70% */ : ( ( i <= 8 ) ? 6 /* 10% */ : 4 /* 20% */ )
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

//  nice little trick to get at OS internal config state for testing ...
extern size_t g_cbQWSChunkSize;
extern INT g_cmsecQWSThrottle;

ERR ErrTestOSMemoryResidentMapPerf( void ** rgpvChunks, size_t cChunks )
{
    ERR err = JET_errSuccess;

    double rgdblScanTimes[5];
    double rgdblQWSExTimes[5];

    //  primer as old version doesn't do anything on initial try

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

//  The object psbm points to is created in ErrOSMemoryPageResidenceMapOLD
//  Need to be deleted at the end
extern CSparseBitmap*   psbm;

CUnitTest( OSLayerResidentMapPerf, 0, "Tests performance of OSLayer residence map scan APIs" );
ERR OSLayerResidentMapPerf::ErrTest()
{
    ERR err = JET_errSuccess;

    // We want to make sure we run the test with the highest amount of memory available.
    // Once that is run, we'll pseudo-randomly skip the mid-range tests.
    BOOL fRanAtLeastOne = fFalse;

    COSLayerPreInit oslayer; // FOSPreinit()

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        goto HandleError;
    }

    void * rgpvChunks[128] = { 0 };

    printf( "Total RAM: %I64d\n\n", OSMemoryTotal() );

    //  
    //  Test single cache segment
    //

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


    //  
    //  Test _countof(rgpvChunks) number of Cache Segments
    //

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
    // Always run the 1 GB testing; don't skip PercentToRunQueryWorkingSetEx().
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
    //  Can't believe we don't have 4K memory available.
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

//  Validates FUtilZeroed().

CUnitTest( ValidateFAllZeros, 0, "Validates FUtilZeroed." );
ERR ValidateFAllZeros::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    BYTE pb[ 32 ] = { 0 };

    OSTestCheck( FUtilZeroed( pb, 0 ) );

    // Increase range to be checked with first byte fixed.
    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ibToChange = cb - 1;

        memset( pb, 0, sizeof( pb ) );

        // All zeroes.
        OSTestCheck( FUtilZeroed( &pb[0], cb ) );

        // Add a non-zero byte inside the region to be checked.
        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[0], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[0], cb ) );

        // Add a non-zero byte after the end, outside the region to be checked.
        if ( ( ibToChange + 1 ) < sizeof( pb ) )
        {
            pb[ ibToChange + 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[0], cb ) );
        }

        // Add a non-zero byte inside the region to be checked.
        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[0], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[0], cb ) );
    }

    // Decrease range to be checked with last byte fixed.
    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ibToChange = sizeof( pb ) - cb;

        memset( pb, 0, sizeof( pb ) );

        // All zeroes.
        OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );

        // Add a non-zero byte inside the region to be checked.
        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );

        // Add a non-zero byte before the start, outside the region to be checked.
        if ( ibToChange > 0 )
        {
            pb[ ibToChange - 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );
        }

        // Add a non-zero byte inside the region to be checked.
        pb[ ibToChange ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange], cb ) );
        pb[ ibToChange ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange], cb ) );
    }

    // Increase range to be checked and vary first byte.
    for ( size_t cb = 1; cb <= sizeof( pb ); cb++ )
    {
        const size_t ibToChange1 = ( sizeof( pb ) / 2 ) - ( cb / 2 );
        const size_t ibToChange2 = ( sizeof( pb ) / 2 ) + ( ( cb - 1 ) / 2 );
        OSTestCheck( ibToChange2 >= ibToChange1 );
        OSTestCheck( ( ibToChange2 - ibToChange1 + 1 ) == cb );

        memset( pb, 0, sizeof( pb ) );

        // All zeroes.
        OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );

        // Add non-zero bytes inside the region to be checked.
        pb[ ibToChange1 ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange1], cb ) );
        pb[ ibToChange2 ] = 1;
        OSTestCheck( !FUtilZeroed( &pb[ibToChange1], cb ) );
        pb[ ibToChange1 ] = 0;
        pb[ ibToChange2 ] = 0;
        OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );

        // Add a non-zero byte before the start, outside the region to be checked.
        if ( ibToChange1 > 0 )
        {
            pb[ ibToChange1 - 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );
        }

        // Add a non-zero byte after the end, outside the region to be checked.
        if ( ( ibToChange2 + 1 ) < sizeof( pb ) )
        {
            pb[ ibToChange2 + 1 ] = 1;
            OSTestCheck( FUtilZeroed( &pb[ibToChange1], cb ) );
        }

        // Add non-zero bytes inside the region to be checked.
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

//  Validates IbUtilLastNonZeroed().

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

        // All zeroes.
        OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == cb );

        // Add a non-zero byte at the end, outside the region to be checked.
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

        // Add a non-zero byte at the end, outside the region to be checked.
        if ( ( ib + 1 ) < sizeof( pb ) )
        {
            pb[ ib + 1 ] = 1;
            OSTestCheck( IbUtilLastNonZeroed( pb, cb ) == ib );
        }
    }

HandleError:
    return err;
}


