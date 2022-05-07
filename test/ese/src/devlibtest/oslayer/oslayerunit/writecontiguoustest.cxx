// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "osunitstd.hxx"

CUnitTest( WRITECONTIGUOUSTESTS, 0, "Tests COSFile::ErrIOWriteContiguous functionality." );

INT cWriteSectors = 4;
const INT cbSectorSize = 4096;
const INT cbFileMaxSecs = 258;
BYTE pbData[cbSectorSize * cbFileMaxSecs];
const BYTE* rgpbLogData[cbFileMaxSecs]; 
ULONG rgcbLogData[cbFileMaxSecs];
TraceContext rgtc[cbFileMaxSecs];
// Make sure the start writing point is aligned with sector size.
const BYTE* pbStart = (BYTE *) roundup((QWORD) pbData, cbSectorSize);
void InitializeLogData( INT sectors )
{
    for (INT i = 0; i < sectors; ++i )
    {
        // interleave the memory
        if ( i % 2 )
        {
            rgpbLogData[i] = pbStart + (i + 1) * cbSectorSize;
        }
        else
        {
            rgpbLogData[i] = pbStart + ( i + 10 ) * cbSectorSize;   
        }
        rgcbLogData[i] = cbSectorSize;
        rgtc[i].iorReason.SetIorp( IOREASONPRIMARY( 82 ) );
    }
}
//  ================================================================
ERR WRITECONTIGUOUSTESTS::ErrTest()
//  ================================================================
{
    JET_ERR  err = JET_errSuccess;
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi    = NULL;

    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration()
            {
                m_dtickAccessDeniedRetryPeriod = 2000;

                //  Must set these, as the FTL layer initializes it's buffers off them.
                m_cbMaxReadSize = 512 * 1024;
                m_cbMaxWriteSize = 384 * 1024;
            }
    } fsconfig;

    InitializeLogData(cWriteSectors);

    COSLayerPreInit     oslayer;
    COSFilePerfTest *perfTest = NULL;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    Call( ErrOSInit() );

    OSTestCheckErr( ErrOSFSCreate( &fsconfig, &pfsapi ) );

    const WCHAR * wszPath = L"testwrite.edb";

    // Clean up the old file (if it happens to be there).
    (void) pfsapi->ErrFileDelete( wszPath );

    OSTestCheckErr( pfsapi->ErrFileCreate( wszPath, IFileAPI::fmfNone, &pfapi ) );
    
    // Initialize the file size to be cbSectorSize sectors.
    OSTestCheckErr( pfapi->ErrIOWrite(*TraceContextScope( IOREASONPRIMARY( 82 ) ), 0, cbFileMaxSecs * cbSectorSize, pbData, qosIODispatchImmediate) );
    delete pfapi;
    pfapi = NULL;
    // finished initializing.

    // Test IO write gather scenario for regular size write
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( ErrIOWriteContiguous( pfapi, rgtc, cbSectorSize, rgcbLogData, rgpbLogData, cWriteSectors, qosIODispatchImmediate ) );
    OSTestCheck( 1 == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cWriteSectors * cbSectorSize == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;

    // Test file cache enabled scenario that IO cannot be combined
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfCached | IFileAPI::fmfLossyWriteBack, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( ErrIOWriteContiguous( pfapi, rgtc, cbSectorSize, rgcbLogData, rgpbLogData, cWriteSectors, qosIODispatchImmediate ) );
    OSTestCheck( cWriteSectors == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cWriteSectors * cbSectorSize == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;

    printf("Testing Sync IOs");
    // Test writing two contiguous IOs via ErrIOWrite will not combine 
    INT cbTotal = 20 * cbSectorSize;
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( pfapi->ErrIOWrite(*TraceContextScope( IOREASONPRIMARY( 82 ) ), 0, 10 * cbSectorSize, pbStart, qosIODispatchImmediate) );
    OSTestCheckErr( pfapi->ErrIOWrite(*TraceContextScope( IOREASONPRIMARY( 82 ) ), 10 * cbSectorSize, 10 * cbSectorSize, pbStart, qosIODispatchImmediate) );
    OSTestCheck( 2 == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cbTotal == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;
    
    
    // Test writing more than cbMaxWriteSize can still be combined if qosIOOptimizeOverrideMaxIOLimits is set
    // Scenario: first write exceeds cbWriteIOMax 
    rgpbLogData[0] = pbStart + 200 * cbSectorSize;
    rgpbLogData[1] = pbStart;
    rgcbLogData[0] = 10 * cbSectorSize;
    rgcbLogData[1] = 200 * cbSectorSize;
    cbTotal = 210 * cbSectorSize;
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( ErrIOWriteContiguous( pfapi, rgtc, cbSectorSize, rgcbLogData, rgpbLogData, 2, qosIODispatchImmediate | qosIOOptimizeOverrideMaxIOLimits ) );
    OSTestCheck( 1 == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cbTotal == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;


    // Scenario: second write exceeds cbWriteIOMax
    rgpbLogData[0] = pbStart;
    rgpbLogData[1] = pbStart + 200 * cbSectorSize;
    rgcbLogData[0] = 200 * cbSectorSize;
    rgcbLogData[1] = 10 * cbSectorSize;
    cbTotal = 210 * cbSectorSize;
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( ErrIOWriteContiguous( pfapi, rgtc, cbSectorSize, rgcbLogData, rgpbLogData, 2, qosIODispatchImmediate | qosIOOptimizeOverrideMaxIOLimits ) );
    OSTestCheck( 1 == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cbTotal == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;


    // Test writing more than cbMaxWriteSize cannot be combined if qosIOOptimizeOverrideMaxIOLimits is not set
    // Scenario: first write exceeds cbWriteIOMax 
    rgpbLogData[0] = pbStart + 200 * cbSectorSize;
    rgpbLogData[1] = pbStart;
    rgcbLogData[0] = 10 * cbSectorSize;
    rgcbLogData[1] = 200 * cbSectorSize;
    cbTotal = 210 * cbSectorSize;
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( ErrIOWriteContiguous( pfapi, rgtc, cbSectorSize, rgcbLogData, rgpbLogData, 2, qosIODispatchImmediate ) );
    OSTestCheck( 2 == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cbTotal == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;
    // Scenario: first write exceeds cbWriteIOMax 
    rgpbLogData[0] = pbStart;
    rgpbLogData[1] = pbStart + 200 * cbSectorSize;
    rgcbLogData[0] = 200 * cbSectorSize;
    rgcbLogData[1] = 10 * cbSectorSize;
    cbTotal = 210 * cbSectorSize;
    OSTestCheckErr( pfsapi->ErrFileOpen( wszPath, IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );
    // The offset cannot be zero since that must be written by a sync IO.
    OSTestCheckErr( ErrIOWriteContiguous( pfapi, rgtc, cbSectorSize, rgcbLogData, rgpbLogData, 2, qosIODispatchImmediate ) );
    OSTestCheck( 2 == perfTest->NumberOfIOCompletions() );
    OSTestCheck( cbTotal == perfTest->NumberOfBytesWritten() );
    delete pfapi;
    pfapi = NULL;

HandleError:
    if( pfapi != NULL )
    {
        delete pfapi;
        pfapi = NULL;
    }
    OSTerm();
    if( pfsapi != NULL )
    {
        delete pfsapi;
        pfsapi = NULL;
    }
    
    return err;
}

//
//  Infrastructure to support Async IOs
//

typedef struct {
    BYTE *                          pbData;
    LONG                            cioInProgress;
    TICK                            tickStart;
    TICK                            tickCompleted;  //  time of last completion
} FILEAPI_WAIT_ASYNC_IO_CTX;

void FileApiWaitAsyncIO(    const ERR           err,
                                IFileAPI* const     pfapi,
                                const IOREASON      ior,
                                const OSFILEQOS     grbitQOS,
                                const QWORD         ibOffset,
                                const DWORD         cbData,
                                const BYTE* const   pbData,
                                const void *        pvCtx )
{
    FILEAPI_WAIT_ASYNC_IO_CTX * pComplete = (FILEAPI_WAIT_ASYNC_IO_CTX*) pvCtx;

    Assert( pComplete->pbData == pbData );

    wprintf( L" (AsyncIO: Finished IO dtickIO = %d, error = %d) ", DtickDelta( pComplete->tickStart, TickOSTimeCurrent() ), err );

    // "complete" the IO

    const TICK cioAfter = AtomicDecrement( &pComplete->cioInProgress );
    if ( cioAfter == 0 )
    {
        pComplete->tickCompleted = TickOSTimeCurrent();
    }

    return;
}

void WaitAsyncIO( FILEAPI_WAIT_ASYNC_IO_CTX * pWaitAsyncIoCtx )
{
    if ( pWaitAsyncIoCtx->cioInProgress )
    {
        wprintf( L" Warning: Waiting for async IOs: ." );
        while ( pWaitAsyncIoCtx->cioInProgress )
        {
            wprintf( L"." );
            Sleep(500);
        }
        wprintf( L"Done.\n" );
    }
    Sleep( 0 ); Sleep( 1 ); Sleep( 2 ); Sleep( 128 );   //  should guarantee the other thread ran
}

#ifdef DEBUG
#define OnDebugOrRetail( dbg, fre )     dbg
#else
#define OnDebugOrRetail( dbg, fre )     fre
#endif

#define COMMON_CONCURRENT_IO_INIT                                                   \
    const ULONG cbDataDefault = 4096;                                               \
    const WCHAR * const wszFileOne = L"FileOne.mdb";                                \
    const WCHAR * const wszFileTwo = L"FileTwo.mdb";                                \
                                                                                    \
    IFileSystemAPI * pfsapi = NULL;                                                 \
                                                                                    \
    IFileAPI * pfapiOne = NULL;                                                     \
    IFileAPI * pfapiTwo = NULL;                                                     \
                                                                                    \
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration         \
    {                                                                               \
        public:                                                                     \
            CFileSystemConfiguration()                                              \
            {                                                                       \
                m_dtickAccessDeniedRetryPeriod = 2000;                              \
                m_cbMaxReadSize = 64 * 1024;                                        \
                m_cbMaxWriteSize = 64 * 1024;                                       \
                m_cbMaxReadGapSize = 64 * 1024;                                     \
                /*  auto-testing sync IOs as async IOs messes up this test.  */     \
                m_permillageSmoothIo = 0;                                           \
            }                                                                       \
    } fsconfig;                                                                     \
                                                                                    \
    FILEAPI_WAIT_ASYNC_IO_CTX WaitAsyncIoCtxOne = { 0 };                            \
    FILEAPI_WAIT_ASYNC_IO_CTX WaitAsyncIoCtxTwo = { 0 };                            \
                                                                                    \
    COSLayerPreInit     oslayer;                                                    \
    if ( !oslayer.FInitd() )                                                        \
        {                                                                           \
        wprintf( L"Out of memory error during OS Layer pre-init." );                \
        err = JET_errOutOfMemory;                                                   \
        goto HandleError;                                                           \
    }                                                                               \
                                                                                    \
    Call( ErrCreateIncrementFile( wszFileOne, fFalse ) );                           \
    Call( ErrCreateIncrementFile( wszFileTwo, fFalse ) );                           \
                                                                                    \
    Call( ErrOSInit() );                                                            \
    Call( ErrOSFSCreate( &fsconfig, &pfsapi ) );                                    \
                                                                                    \
    WaitAsyncIoCtxOne.pbData = (BYTE*)PvOSMemoryPageAlloc( cbDataDefault, NULL );   \
    Alloc( WaitAsyncIoCtxOne.pbData );                                              \
    WaitAsyncIoCtxTwo.pbData = (BYTE*)PvOSMemoryPageAlloc( cbDataDefault, NULL );   \
    Alloc( WaitAsyncIoCtxTwo.pbData );                                              \
                                                                                    \
    Call( pfsapi->ErrFileOpen( wszFileOne, IFileAPI::fmfNone, &pfapiOne ) );        \
    Call( pfsapi->ErrFileOpen( wszFileTwo, IFileAPI::fmfNone, &pfapiTwo ) );        \


#define COMMON_CONCURRENT_IO_TERM   \
    delete pfapiTwo;                \
    delete pfapiOne;                \
    delete pfsapi;                  \
    OSTerm();                       \


//  In case it isn't obvious for this set of tests, the first / tens digit of IOPRIMARY will
//  be the file number we're issuing against, and the singles digit will be the step / order
//  in which the IOs were enqueued.


CUnitTest( OslayerIoMgrCanBuildMultipleConcurrentReadIosOnMultipleFiles, 0x0, "Tests that ESE can build two concurrent IOs (simplest read case) against two separate concurrent files." );
ERR OslayerIoMgrCanBuildMultipleConcurrentReadIosOnMultipleFiles::ErrTest()
{
    ERR err = JET_errSuccess;

    COMMON_CONCURRENT_IO_INIT;

    wprintf( L"  Begin enqueing IOs ...\n" );

    WaitAsyncIoCtxOne.tickStart = TickOSTimeCurrent();
    WaitAsyncIoCtxOne.cioInProgress++;
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 11 ) ),
                        12 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    WaitAsyncIoCtxOne.cioInProgress++;
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 12 ) ),
                        13 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    UtilSleep( rand() % OnDebugOrRetail( 10, 2000 ) );  //  make debug test faster ...

    WaitAsyncIoCtxTwo.tickStart = TickOSTimeCurrent();
    WaitAsyncIoCtxTwo.cioInProgress++;
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 23 ) ),
                        14 * cbDataDefault, //  designed to nefariously align offset-wise w/ previous file's IOs
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );
    WaitAsyncIoCtxTwo.cioInProgress++;
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 24 ) ),
                        15 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 2 );
    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 2 );

    wprintf( L"  Begin issuing IOs ...\n" );

    //  Now issue Two first, even though we started by enqueing One first ...

    const TICK tickIoTwoIssued = TickOSTimeCurrent();
    OSTestCall( pfapiTwo->ErrIOIssue() );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );

    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 2 );        // Should still be in progress.

    WaitAsyncIO( &WaitAsyncIoCtxTwo );

    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 0 );        // two is done
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 2 );        // Should still be in progress.

    //  Now issue One....

    OSTestCall( pfapiOne->ErrIOIssue() );
    WaitAsyncIO( &WaitAsyncIoCtxOne );

    //  Check expected timings, due to order of issue

    OSTestCheck( DtickDelta( tickIoTwoIssued, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );
    OSTestCheck( DtickDelta( WaitAsyncIoCtxTwo.tickCompleted, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );


HandleError:

    COMMON_CONCURRENT_IO_TERM;
    return err;
}

CUnitTest( OslayerIoMgrCanBuildMultipleConcurrentInterleavedReadIosOnMultipleFiles, 0x0, "Tests that ESE can build two concurrent Read IOs runs against two separate files, but interleaving between the files." );
ERR OslayerIoMgrCanBuildMultipleConcurrentInterleavedReadIosOnMultipleFiles::ErrTest()
{
    ERR err = JET_errSuccess;

    COMMON_CONCURRENT_IO_INIT;

    wprintf( L"  Begin enqueing Write IOs (one for each file) ...\n" );

    WaitAsyncIoCtxOne.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 11 ) ),
                        12 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    UtilSleep( rand() % OnDebugOrRetail( 10, 2000 ) );  //  make debug test faster ...

    WaitAsyncIoCtxTwo.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 22 ) ),
                        13 * cbDataDefault, //  designed to nefariously align offset-wise w/ previous file's IOs
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    //  Now switch back to updating file one ...
    wprintf( L"  Begin enqueing more Write IOs (switching back to the first file ... one for each file) ...\n" );

    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 13 ) ),
                        13 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );

    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 24 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    wprintf( L"  One more set of IOs / 2nd with a read-gap ...\n" );
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 15 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );

    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 26 ) ),
                        // Adding a +2 "page" gap
                        (15+2) * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );
    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 3 );

    wprintf( L"  Begin issuing IOs ...\n" );

    //  Now issue Two first, even though we started by enqueing One first ...

    const TICK tickIoTwoIssued = TickOSTimeCurrent();
    OSTestCall( pfapiTwo->ErrIOIssue() );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );

    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    WaitAsyncIO( &WaitAsyncIoCtxTwo );

    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 0 );        // two is done
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    //  Now issue One....

    OSTestCall( pfapiOne->ErrIOIssue() );
    WaitAsyncIO( &WaitAsyncIoCtxOne );

    //  Check expected timings, due to order of issue

    OSTestCheck( DtickDelta( tickIoTwoIssued, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );
    OSTestCheck( DtickDelta( WaitAsyncIoCtxTwo.tickCompleted, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );


HandleError:

    COMMON_CONCURRENT_IO_TERM;
    return err;
}




// SOMEONE need one where writes and reads try to interfer with each other ...
// SOMEONE rename for read ...
CUnitTest( OslayerIoMgrCanBuildMultipleConcurrentInterleavedWriteIosOnMultipleFiles, 0x0, "Tests that ESE can build two concurrent Write IOs runs against two different files." );
ERR OslayerIoMgrCanBuildMultipleConcurrentInterleavedWriteIosOnMultipleFiles::ErrTest()
{
    ERR err = JET_errSuccess;

    COMMON_CONCURRENT_IO_INIT;

    wprintf( L"  Begin enqueing Write IOs (one for each file) ...\n" );

    WaitAsyncIoCtxOne.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 11 ) ),
                        12 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    UtilSleep( rand() % OnDebugOrRetail( 10, 2000 ) );  //  make debug test faster ...

    wprintf( L"  Switch to enqueing Write IOs to file two ...\n" );
    WaitAsyncIoCtxTwo.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 22 ) ),
        // SOMEONE should make this 13 right ... then what are the below???
                        14 * cbDataDefault, //  designed to nefariously align offset-wise w/ previous file's IOs
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    //  Now switch back to updating file one ...
    wprintf( L"  Switching to more Write IOs (switching back to the first file ... one for each file) ...\n" );

    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 13 ) ),
                        13 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );

    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 24 ) ),
                        15 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 2 );
    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 2 );

    wprintf( L"  Begin issuing IOs ...\n" );

    //  Now issue Two first, even though we started by enqueing One first ...

    const TICK tickIoTwoIssued = TickOSTimeCurrent();
    OSTestCall( pfapiTwo->ErrIOIssue() );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );

    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 2 );        // Should still be in progress.

    WaitAsyncIO( &WaitAsyncIoCtxTwo );

    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 0 );        // two is done
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 2 );        // Should still be in progress.

    //  Now issue One....

    OSTestCall( pfapiOne->ErrIOIssue() );
    WaitAsyncIO( &WaitAsyncIoCtxOne );

    //  Check expected timings, due to order of issue

    OSTestCheck( DtickDelta( tickIoTwoIssued, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );
    OSTestCheck( DtickDelta( WaitAsyncIoCtxTwo.tickCompleted, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );


HandleError:

    COMMON_CONCURRENT_IO_TERM;
    return err;
}

// SOMEONE need one where writes and reads try to interfer with each other ...
// SOMEONE rename for read ...
// SOMEONE do one for sync read ...
CUnitTest( OslayerIoMgrCanBuildMultipleConcurrentWriteAndAsyncReadIosOnMultipleFiles, 0x0, "Tests that ESE can build one Write IO and a concurrent Read IO runs against two different files." );
ERR OslayerIoMgrCanBuildMultipleConcurrentWriteAndAsyncReadIosOnMultipleFiles::ErrTest()
{
    ERR err = JET_errSuccess;

    COMMON_CONCURRENT_IO_INIT;

    wprintf( L"  Begin enqueing Write IOs (one for each file) ...\n" );

    WaitAsyncIoCtxOne.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 11 ) ),
                        12 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    UtilSleep( rand() % OnDebugOrRetail( 10, 2000 ) );  //  make debug test faster ...

    wprintf( L"  Now switch to reading file two ...\n" );
    WaitAsyncIoCtxTwo.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 22 ) ),
                        13 * cbDataDefault, //  designed to nefariously align offset-wise w/ previous file's IOs
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    wprintf( L"  Now switching back to writing file one ...\n" );
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 13 ) ),
                        13 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );

    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 24 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 15 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal /* forgetting qosIOOptimizeCombinable should be fine and still combine */,
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 26 ) ),
                        15 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );



    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );
    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 3 );

    wprintf( L"  Begin issuing IOs ...\n" );

    //  Now issue Two first, even though we started by enqueing One first ...

    const TICK tickIoTwoIssued = TickOSTimeCurrent();
    OSTestCall( pfapiTwo->ErrIOIssue() );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );

    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    WaitAsyncIO( &WaitAsyncIoCtxTwo );

    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 0 );        // two is done
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    //  Now issue One....

    OSTestCall( pfapiOne->ErrIOIssue() );
    WaitAsyncIO( &WaitAsyncIoCtxOne );

    //  Check expected timings, due to order of issue

    OSTestCheck( DtickDelta( tickIoTwoIssued, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );
    OSTestCheck( DtickDelta( WaitAsyncIoCtxTwo.tickCompleted, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );


HandleError:

    COMMON_CONCURRENT_IO_TERM;
    return err;
}


CUnitTest( OslayerIoMgrCanBuildMultipleConcurrentWriteAndAsyncReadIosOnMultipleFilesSwapped, 0x0, "Same as previous test, but swapped read and write IOs." );
ERR OslayerIoMgrCanBuildMultipleConcurrentWriteAndAsyncReadIosOnMultipleFilesSwapped::ErrTest()
{
    ERR err = JET_errSuccess;

    COMMON_CONCURRENT_IO_INIT;

    wprintf( L"  Begin enqueing Write IOs (one for each file) ...\n" );

    WaitAsyncIoCtxOne.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 11 ) ),
                        12 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    UtilSleep( rand() % OnDebugOrRetail( 10, 2000 ) );  //  make debug test faster ...

    wprintf( L"  Now switch to reading file two ...\n" );
    WaitAsyncIoCtxTwo.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 22 ) ),
                        13 * cbDataDefault, //  designed to nefariously align offset-wise w/ previous file's IOs
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    wprintf( L"  Now switching back to writing file one ...\n" );
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 13 ) ),
                        13 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );

    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 24 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 15 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal /* forgetting qosIOOptimizeCombinable should be fine and still combine */,
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 26 ) ),
                        15 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );



    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );
    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 3 );

    wprintf( L"  Begin issuing IOs ...\n" );

    //  Now issue Two first, even though we started by enqueing One first ...

    const TICK tickIoTwoIssued = TickOSTimeCurrent();
    OSTestCall( pfapiTwo->ErrIOIssue() );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );

    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    WaitAsyncIO( &WaitAsyncIoCtxTwo );

    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 0 );        // two is done
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    //  Now issue One....

    OSTestCall( pfapiOne->ErrIOIssue() );
    WaitAsyncIO( &WaitAsyncIoCtxOne );

    //  Check expected timings, due to order of issue

    OSTestCheck( DtickDelta( tickIoTwoIssued, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );
    OSTestCheck( DtickDelta( WaitAsyncIoCtxTwo.tickCompleted, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );


HandleError:

    COMMON_CONCURRENT_IO_TERM;
    return err;
}

__int64 memcnt( const BYTE * const pb, const BYTE bTarget, const size_t cb )
{
    __int64 cnt = 0;
    for( size_t ib = 0; ib < cb; ib++ )
    {
        if( bTarget == pb[ib] )
        {
            cnt++;
        }
    }
    return cnt;
}

// SOMEONE need one where writes and reads try to interfer with each other ...
// SOMEONE rename for read ...
// SOMEONE do one for sync read ...
//  This ensure an occasional sync read IO (to the flushmap) or (probably more importantly) a sync 
//  write IO (to the DB header) will not disrupt building IOs.
CUnitTest( OslayerIoMgrCanBuildMultipleConcurrentAsyncRandAndWriteIosWithNonInterferringSyncReadIosOnMultipleFiles, 0x0, 
            "Tests that ESE can build a Write IO and Read IO runs and do concurrent _Synchronous_ (Read and Write) IOs against two different files." );
ERR OslayerIoMgrCanBuildMultipleConcurrentAsyncRandAndWriteIosWithNonInterferringSyncReadIosOnMultipleFiles::ErrTest()
{
    ERR err = JET_errSuccess;

    COMMON_CONCURRENT_IO_INIT;

    wprintf( L"  Begin enqueing Write IOs (one for each file) ...\n" );

    WaitAsyncIoCtxOne.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 11 ) ),
                        12 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    UtilSleep( rand() % OnDebugOrRetail( 10, 2000 ) );  //  make debug test faster ...

    wprintf( L"  Starting a async read IO to file two ...\n" );
    WaitAsyncIoCtxTwo.tickStart = TickOSTimeCurrent();
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 22 ) ),
                        13 * cbDataDefault, //  designed to nefariously align offset-wise w/ previous file's IOs
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    wprintf( L"  Now switching back to writing file one ...\n" );
    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 31 ) ),
                        13 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );

    wprintf( L"  Finally doing a sync read IO...\n" );
    //  Doing sync IO here ... 
    BYTE * pbData = (BYTE*)PvOSMemoryPageAlloc( cbDataDefault, NULL );
    //AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 42 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        pbData,
                        qosIONormal ) );
    __int64 cFourteens = memcnt( pbData, 0x14, cbDataDefault );
    OSTestCheck( cFourteens = cbDataDefault );

    AtomicIncrement( &( WaitAsyncIoCtxOne.cioInProgress ) );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 51 ) ),
                        14 * cbDataDefault, //  offset contiguos with previous write to same file
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal | qosIOOptimizeCombinable,
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxOne ) );
    
    wprintf( L"  One more async read IO...\n" );
    AtomicIncrement( &( WaitAsyncIoCtxTwo.cioInProgress ) );
    OSTestCall( pfapiTwo->ErrIORead( *TraceContextScope( IOREASONPRIMARY( 62 ) ),
                        14 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxTwo.pbData,
                        qosIONormal | qosIOOptimizeCombinable, 
                        IFileAPI::PfnIOComplete( FileApiWaitAsyncIO ),
                        (DWORD_PTR)&WaitAsyncIoCtxTwo ) );

    wprintf( L"  And finally a sync _write_ IO to the same file as well ...\n" );
    OSTestCall( pfapiOne->ErrIOWrite( *TraceContextScope( IOREASONPRIMARY( 71 ) ),
                        20 * cbDataDefault,
                        cbDataDefault,
                        WaitAsyncIoCtxOne.pbData,
                        qosIONormal ) );


    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );
    //  the async read IOs to the same file should be orthogonal maintained (because sync never enters the queue) ...
    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 2 );

    wprintf( L"  Begin issuing IOs ...\n" );

    //  Now issue Two first, even though we started by enqueing One first ...

    const TICK tickIoTwoIssued = TickOSTimeCurrent();
    OSTestCall( pfapiTwo->ErrIOIssue() );

    UtilSleep( rand() % OnDebugOrRetail( 3, 2000 ) );

    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    WaitAsyncIO( &WaitAsyncIoCtxTwo );

    OSTestCheck( WaitAsyncIoCtxTwo.cioInProgress == 0 );        // two is done
    OSTestCheck( WaitAsyncIoCtxOne.cioInProgress == 3 );        // Should still be in progress.

    //  Now issue One....

    OSTestCall( pfapiOne->ErrIOIssue() );
    WaitAsyncIO( &WaitAsyncIoCtxOne );

    //  Check expected timings, due to order of issue

    OSTestCheck( DtickDelta( tickIoTwoIssued, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );
    OSTestCheck( DtickDelta( WaitAsyncIoCtxTwo.tickCompleted, WaitAsyncIoCtxOne.tickCompleted ) >= 0 );


HandleError:

    COMMON_CONCURRENT_IO_TERM;
    return err;
}


// SOMEONE need one where writes and reads try to interfer with each other ...



