// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    // iorpNone publically defined
    iorpInvalid = 0,

    iorpOSUnitTest
};

DWORD g_cIOsCompleted = 0;
void IOCompletion(
    const ERR                   err,
    IFileAPI* const             pfapi,
    const FullTraceContext&     tc,
    const OSFILEQOS             grbitQOS,
    const QWORD                 ibOffset,
    const DWORD                 cbData,
    const BYTE* const           pbData,
    const DWORD_PTR             keyIOComplete )
{
    InterlockedIncrement( &g_cIOsCompleted );
}

CUnitTest( TestAllocIOReq, 0, "Test for pre-allocing I/O Req for ErrIORead" );
ERR TestAllocIOReq::ErrTest()
{
    ERR err;
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;
    BYTE rgbTestPattern[4096];
    DWORD dwPattern = 0xbaadf00d;
    DWORD cb;
    DWORD i;
    BYTE * pbReadBuffer = NULL;
    VOID *pioreq = NULL;
    COSFilePerfTest *perfTest = NULL;

    memcpy( rgbTestPattern, &dwPattern, sizeof( dwPattern ) );
    for ( cb = 4; cb < sizeof(rgbTestPattern); cb *= 2 )
    {
        memcpy( rgbTestPattern + cb, rgbTestPattern, cb );
    }

    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration
    {
        public:
            CFileSystemConfiguration()
            {
                m_cbMaxReadSize = 262144;
                m_cbMaxReadGapSize = 262144;
            }
    } fsconfig;

    COSLayerPreInit oslayer;
    OSTestCall( ErrOSInit() );

    OSTestCheck( sizeof( rgbTestPattern ) >= OSMemoryPageCommitGranularity() );
    pbReadBuffer = (BYTE*)PvOSMemoryPageAlloc( 100 * sizeof( rgbTestPattern ), NULL );
    OSTestCheck( pbReadBuffer != NULL );
    OSTestCheck( ( (ULONG_PTR)pbReadBuffer % OSMemoryPageCommitGranularity() ) == 0 );

    OSTestCheckErr( ErrOSFSCreate( &fsconfig, &pfsapi ) );

    // create and pattern fill the file
    OSTestCheckErr( pfsapi->ErrFileCreate( L"TestAllocIOREQ1.tmp", IFileAPI::fmfOverwriteExisting, &pfapi ) );
    for ( cb = 0; cb < 262144; cb += sizeof(rgbTestPattern) )
    {
        OSTestCheckErr( pfapi->ErrIOWrite( *TraceContextScope( iorpOSUnitTest ), cb, sizeof(rgbTestPattern), rgbTestPattern, qosIODispatchImmediate ) );
    }

    delete pfapi;
    pfapi = NULL;

    OSTestCheckErr( pfsapi->ErrFileOpen( L"TestAllocIOREQ1.tmp", IFileAPI::fmfNone, &pfapi ) );
    perfTest = new COSFilePerfTest;
    pfapi->RegisterIFilePerfAPI( perfTest );

    // First just try to alloc/free io-reqs a few times
    for ( i=0; i<100; i++ )
    {
        OSTestCheckErr( pfapi->ErrReserveIOREQ( 0, 4096, qosIODispatchImmediate, &pioreq ) );
        pfapi->ReleaseUnusedIOREQ( pioreq );
        pioreq = NULL;
    }

    g_cIOsCompleted = 0;

    // Actually use pre-reserved ioreqs for doing I/O
    for ( i=1; i<32; i++ )
    {
        OSTestCheckErr( pfapi->ErrReserveIOREQ( 4096*i, 4096, qosIODispatchImmediate | ((i == 1) ? 0 : qosIOOptimizeCombinable), &pioreq ) );
        OSTestCheckErr( pfapi->ErrIORead( *TraceContextScope( iorpOSUnitTest ), 4096*i, 4096, pbReadBuffer + i * 4096, qosIODispatchImmediate | ((i == 1) ? 0 : qosIOOptimizeCombinable), IOCompletion, NULL, NULL, pioreq ) );
        pioreq = NULL;
    }
    for ( i=63; i>=32; i-- )
    {
        OSTestCheckErr( pfapi->ErrReserveIOREQ( 4096*i, 4096, qosIODispatchImmediate | ((i == 1) ? 0 : qosIOOptimizeCombinable), &pioreq ) );
        OSTestCheckErr( pfapi->ErrIORead( *TraceContextScope( iorpOSUnitTest ), 4096*i, 4096, pbReadBuffer + i * 4096, qosIODispatchImmediate | ((i == 1) ? 0 : qosIOOptimizeCombinable), IOCompletion, NULL, NULL, pioreq ) );
        pioreq = NULL;
    }
    // Make sure we are not able to read beyond the max read size when asking for combinable I/O
    OSTestCheck( pfapi->ErrReserveIOREQ( 4096*99, 4096, qosIODispatchImmediate | ((i == 1) ? 0 : qosIOOptimizeCombinable), &pioreq ) == errDiskTilt );
    OSTestCheckErr( pfapi->ErrIOIssue() );

    while ( g_cIOsCompleted != 63 )
    {
        Sleep( 1 );
    }

    for ( i=1; i<=63; i++ )
    {
        OSTestCheck( memcmp( pbReadBuffer + i * 4096, rgbTestPattern, sizeof(rgbTestPattern) ) == 0 );
    }
    OSTestCheck( 1 == perfTest->NumberOfIOIssues() );
    OSTestCheck( 1 == perfTest->NumberOfIOCompletions() );

    delete pfapi;
    pfapi = NULL;

    pfsapi->ErrFileDelete( L"TestAllocIOREQ1.tmp" );

HandleError:
    delete pfapi;
    delete pfsapi;

    OSMemoryPageFree( pbReadBuffer );

    OSTerm();
    return err;
}

