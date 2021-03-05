// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#include "_ostls.hxx"
#include "_osfs.hxx"
#include "_osfile.hxx"

#pragma warning( push )
#pragma warning( disable: 4101 4189 )

#ifdef DEBUG
#define CHECKDBG( x )       Assert( x )
#else
#define CHECKDBG( x )
#endif


void CleanIoreq( _Out_ IOREQ * pioreq )
{
    memset( pioreq, 0, sizeof(*pioreq) );
    new (pioreq) IOREQ();
    pioreq->m_tc.etc.nParentObjectClass = pocNone;
    pioreq->m_tc.etc.SetDwEngineObjid( dwEngineObjidNone );
    pioreq->hrtIOStart = HrtHRTCount();
}

#define INIT_IOREQ_JUNK                                                     \
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration \
    {                                                                       \
        public:                                                             \
            CFileSystemConfiguration()                                      \
            {                                                               \
                m_cbMaxReadSize = 64 * 1024;                                \
                m_cbMaxWriteSize = 64 * 1024;                               \
                m_cbMaxReadGapSize = 64 * 1024;                             \
            }                                                               \
    } fsconfig;                                                             \
    _OSFILE _osfA, _osfB, _osfC, _osfD;                                     \
    _osfA.pfsconfig = &fsconfig;                                            \
    _osfA.pfpapi = &g_cosfileperfDefault;                                   \
    _osfB.pfsconfig = &fsconfig;                                            \
    _osfB.pfpapi = &g_cosfileperfDefault;                                   \
    _osfC.pfsconfig = &fsconfig;                                            \
    _osfC.pfpapi = &g_cosfileperfDefault;                                   \
    _osfD.pfsconfig = &fsconfig;                                            \
    _osfD.pfpapi = &g_cosfileperfDefault;                                   \
    IOREQ ioreq1, ioreq2, ioreq3, ioreq4, ioreq5;                           \
    CleanIoreq( &ioreq1 );                                                  \
    CleanIoreq( &ioreq2 );                                                  \
    CleanIoreq( &ioreq3 );                                                  \
    CleanIoreq( &ioreq4 );                                                  \
    CleanIoreq( &ioreq5 );                                                  \

#define INIT_IOREQ_EXTRAS                               \
    IOREQ ioreq6, ioreq7, ioreq8, ioreq9;               \
    IOREQ ioreq101, ioreq102, ioreq103, ioreq104;       \
    CleanIoreq( &ioreq6 );                              \
    CleanIoreq( &ioreq7 );                              \
    CleanIoreq( &ioreq8 );                              \
    CleanIoreq( &ioreq9 );                              \
    CleanIoreq( &ioreq101 );                            \
    CleanIoreq( &ioreq102 );                            \
    CleanIoreq( &ioreq103 );                            \
    CleanIoreq( &ioreq104 );                            \

#define TERM_IOREQ_JUNK                                                             \
    IOREQ * pioreqCleanup = NULL;                                                   \
    while( pioreqCleanup = Postls()->iorunpool.PioreqEvictFileRuns( NULL ) )        \
    {                                                                               \
        ;                                                                           \
    }                                                                               \

void InitIoreq( IOREQ * const pioreq, _OSFILE& _osf, const BOOL fWrite, const QWORD ibOffset, const DWORD cbData )
{
    pioreq->m_fHasHeapReservation = fFalse;
    pioreq->m_fCanCombineIO = fFalse;
    pioreq->p_osf = NULL;

    pioreq->SetIOREQType( IOREQ::ioreqInAvailPool );
    pioreq->SetIOREQType( IOREQ::ioreqAllocFromAvail );

    pioreq->p_osf = &_osf;
    pioreq->fWrite = fWrite;
    pioreq->ibOffset = ibOffset;
    pioreq->cbData = cbData;
    pioreq->pioreqIorunNext = NULL;

    pioreq->m_fHasHeapReservation = fTrue;
    pioreq->m_fCanCombineIO = fTrue;

    pioreq->m_tc.etc.iorReason.SetIorp( (IOREASONPRIMARY)0x44 );

    pioreq->pbData = (BYTE*)( rand () * 4096
#ifdef _WIN64
                        | 0xC000420000000000
#else
                        | 0xC0000000 
#endif
            );
}

#define fReadIo  (fFalse)
#define fWriteIo (fTrue)


JETUNITTEST( IoRunOffsetCmp, TestCmpIOREQOffsetBasicComparesWork )
{
    INIT_IOREQ_JUNK;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 - 1, 1024 );
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 1, 1024 );

    CHECK( CmpIOREQOffset( &ioreq1, &ioreq2 ) == 0 );
    CHECK( CmpIOREQOffset( &ioreq1, &ioreq3 ) > 0 );
    CHECK( CmpIOREQOffset( &ioreq1, &ioreq4 ) < 0 );

    CHECK( CmpIOREQOffset( &ioreq3, &ioreq4 ) < 0 );
    CHECK( CmpIOREQOffset( &ioreq4, &ioreq3 ) > 0 );
}


#define IORunT          COSDisk::IORun
#include "OslayerIoMgrIoRunSuite.cxx"

#define IoRunTQopSuite
#undef IORunT
#define IORunT          COSDisk::QueueOp
#include "OslayerIoMgrIoRunSuite.cxx"


#define INIT_SGIO_PREP_JUNK                                             \
            INIT_IOREQ_JUNK                                         \
            const DWORD cfseTests = 256;  \
            FILE_SEGMENT_ELEMENT rgfseTests[cfseTests];         \
            const ULONG cbMinIo = OSMemoryPageCommitGranularity();  \
            BOOL fDontCare;

void OSDiskIIOPrepareScatterGatherIO(
    __in IOREQ * const                  pioreqHead,
    __in const DWORD                    cbRun,
    __in const DWORD                    cfse,
    __out PFILE_SEGMENT_ELEMENT const   rgfse,
    __out BOOL *                        pfIOOSLowPriority
    );

JETUNITTEST( IoRunQopSgio, TestReadCombiningProducesSgioPreparableIorun )
{
    INIT_SGIO_PREP_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, cbMinIo );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + cbMinIo, cbMinIo );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 2 * cbMinIo, cbMinIo );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );

    CHECK( !iorun.FEmpty() );
    CHECK( iorun.FWrite() == fFalse );
    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
#endif
    CHECK( iorun.CbRun() == 3 * cbMinIo );

    OSDiskIIOPrepareScatterGatherIO( &ioreq1, 3 * cbMinIo, cfseTests, rgfseTests, &fDontCare );
}

QWORD IbEnd( QWORD ibOffsetBegin, QWORD cbData )
{
    return ibOffsetBegin + cbData;
}

BOOL FCheckOffsetInChainOrGap( ULONGLONG ulpTest, const IOREQ * const pioreqHead )
{
    extern BYTE* g_rgbGapping;
    if ( ulpTest == (ULONGLONG)g_rgbGapping )
    {
        return fTrue;
    }
    
    for( const IOREQ * pioreqT = pioreqHead; pioreqT; pioreqT = pioreqT->pioreqIorunNext )
    {
        ULONGLONG ulpLow = (ULONGLONG)(pioreqT->pbData);
        ULONGLONG ulpHigh = ulpLow + pioreqT->cbData;
        if ( ulpTest >= ulpLow && ulpTest < ulpHigh )
        {
            return fTrue;
        }
    }

    return fFalse;
}

JETUNITTEST( IoRunQopSgio, TestReadCombiningProducesSgioPreparableIorunCombinatorial )
{
    INIT_SGIO_PREP_JUNK;

    const BOOL fPrintMoreStatus = fFalse;
    wprintf( L"\n" );

    ULONG cTestScenarios = 0;
    ULONG cAddsSucceeded = 0;
    ULONG cAddsFailed = 0;
    ULONG cMultiBlocks = 0;
    const QWORD ibBaseOffset = 256 * 1024;
    for( ULONG cioInitial = 1; cioInitial <= 3; cioInitial++ )
    {
        wprintf( L"\t IOs = %d\n", cioInitial );
        for( ULONG cbInitialIo = cbMinIo; cbInitialIo <= 64 * 1024; cbInitialIo += cbMinIo )
        {
            if ( fPrintMoreStatus ) wprintf( L"\t\t cbInitialIo = %d\n", cbInitialIo );
            for( ULONG cbGap = 0; cbGap <= 64 * 1024; cbGap += cbMinIo )
            {
                if ( fPrintMoreStatus ) wprintf( L"\t\t\t cbGap = %d\n", cbGap );
                for( LONG64 ibTestShift = (LONG64)cbMinIo * -4; ibTestShift <= LONG64( 80 * 1024 ); ibTestShift += cbMinIo )
                {
                    for( ULONG cbTestIo = cbMinIo; cbTestIo <= 4 * cbMinIo; cbTestIo += cbMinIo )
                    {
                        COSDisk::IORun iorun;

                        InitIoreq( &ioreq1, _osfA, fReadIo, ibBaseOffset + 0 * cbInitialIo + 0 * cbGap, cbInitialIo );
                        InitIoreq( &ioreq2, _osfA, fReadIo, ibBaseOffset + 1 * cbInitialIo + 1 * cbGap, cbInitialIo );
                        InitIoreq( &ioreq3, _osfA, fReadIo, ibBaseOffset + 2 * cbInitialIo + 2 * cbGap, cbInitialIo );

                        ioreq1.m_fHasHeapReservation = fTrue;
                        CHECK( iorun.FAddToRun( &ioreq1 ) );
                        QWORD ibMin = ioreq1.ibOffset;
                        QWORD ibMax = IbEnd( ioreq1.ibOffset, cbInitialIo );
                        ULONG cioreq = 1;
                        CHECK( ibBaseOffset + 0 * cbInitialIo + 0 * cbGap == ibMin );

                        if ( iorun.FAddToRun( &ioreq2 ) )
                        {
                            OnDebug( CHECK( iorun.FRunMember( &ioreq2 ) ) );

                            ibMin = min( ibMin, ioreq2.ibOffset );
                            ibMax = max( ibMax, IbEnd( ioreq2.ibOffset, ioreq2.cbData ) );
                            cioreq++;
                        }
                        else
                        {
                            OnDebug( CHECK( !iorun.FRunMember( &ioreq3 ) ) );

                            cAddsFailed++;
                        }

                        if( iorun.FAddToRun( &ioreq3 ) )
                        {
                            OnDebug( CHECK( iorun.FRunMember( &ioreq3 ) ) );

                            ibMin = min( ibMin, ioreq3.ibOffset );
                            ibMax = max( ibMax, IbEnd( ioreq3.ibOffset, ioreq3.cbData ) );
                            cioreq++;
                        }
                        else
                        {
                            OnDebug( CHECK( !iorun.FRunMember( &ioreq3 ) ) );

                            cAddsFailed++;
                        }

                        const QWORD ibTestOffset = ibBaseOffset + ibTestShift;

                        InitIoreq( &ioreq4, _osfA, fReadIo, ibTestOffset, cbTestIo );
                        ioreq4.m_fCanCombineIO = fTrue;
                        if ( iorun.FAddToRun( &ioreq4 ) )
                        {
                            OnDebug( CHECK( iorun.FRunMember( &ioreq4 ) ) );

                            ibMin = min( ibMin, ioreq4.ibOffset );
                            ibMax = max( ibMax, IbEnd( ioreq4.ibOffset, ioreq4.cbData ) );
                            cioreq++;
                            cAddsSucceeded++;
                        }
                        else
                        {
                            OnDebug( CHECK( !iorun.FRunMember( &ioreq4 ) ) );

                            cAddsFailed++;
                        }

                        cTestScenarios++;

                        if ( iorun.FMultiBlock() )
                        {
                            cMultiBlocks++;
                            OnDebug( CHECK( iorun.Cioreq() > 1 ) );


                            IOREQ * pioreqHead = iorun.PioreqGetRun();
                            CHECK( pioreqHead->pioreqIorunNext != NULL );

                            OSDiskIIOPrepareScatterGatherIO( pioreqHead, 3 * cbMinIo, cfseTests, rgfseTests, &fDontCare );

                            QWORD ibOffsetLast = 0;
                            for( IOREQ * pioreqT = pioreqHead; pioreqT; pioreqT = pioreqT->pioreqIorunNext )
                            {
                                CHECK( ibOffsetLast <= pioreqT->ibOffset );
                                ibOffsetLast = pioreqT->ibOffset;
                            }

                            IOREQ * pioreqT = pioreqHead;
                            ULONG ifse;
                            for( ifse = 0; ifse < cfseTests && rgfseTests[ifse].Buffer; ifse++ )
                            {
                                CHECK( FCheckOffsetInChainOrGap( (ULONGLONG)rgfseTests[ifse].Buffer, pioreqHead ) );
                            }

                            CHECK( ifse == ( ibMax - ibMin ) / OSMemoryPageCommitGranularity() );
                        }
                    }
                }
            }
        }
    }
    wprintf( L"\tcTestScenarios enumerated: %d (%d / %d, %d)\n", cTestScenarios, cAddsSucceeded, cAddsFailed, cMultiBlocks );
    
}


JETUNITTEST( IoRunPool, TestSimpleSingleIoreqChain )
{
    INIT_IOREQ_JUNK;

    COSDisk::CIoRunPool iorunpool;

    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );

    ioreq1.p_osf = &_osfA;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );

    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );

    ioreq2.p_osf = &_osfA;
    ioreq2.ibOffset = ioreq1.ibOffset + ioreq1.cbData;
    ioreq2.cbData = 1024;
    ioreq2.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq2 ) );

    ioreq3.p_osf = &_osfA;
    ioreq3.ibOffset = ioreq2.ibOffset + ioreq2.cbData;
    ioreq3.cbData = 1024;
    ioreq3.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq3 ) );

    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoRunPool, TestTwoFilesTwoIoreqChains )
{
    INIT_IOREQ_JUNK;

    COSDisk::CIoRunPool iorunpool;

    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 0 );

    ioreq1.p_osf = &_osfA;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );

    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );

    ioreq2.p_osf = &_osfA;
    ioreq2.ibOffset = ioreq1.ibOffset + ioreq1.cbData;
    ioreq2.cbData = 1024;
    ioreq2.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq2 ) );

    ioreq3.p_osf = &_osfA;
    ioreq3.ibOffset = ioreq2.ibOffset + ioreq2.cbData;
    ioreq3.cbData = 1024;
    ioreq3.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq3 ) );


    ioreq4.p_osf = &_osfB;
    ioreq4.ibOffset = ioreq3.ibOffset + ioreq3.cbData;
    ioreq4.cbData = 1024;
    ioreq4.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq4 ) );
}

JETUNITTEST( IoRunPool, TestCapacityEvictOfOriginalIoreqChainOn3rdIo )
{
    INIT_IOREQ_JUNK;

    COSDisk::CIoRunPool iorunpool;

    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 0 );

    ioreq1.p_osf = &_osfA;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );

    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq2.p_osf = &_osfA;
    ioreq2.ibOffset = ioreq1.ibOffset + ioreq1.cbData;
    ioreq2.cbData = 1024;
    ioreq2.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq2 ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq3.p_osf = &_osfA;
    ioreq3.ibOffset = ioreq2.ibOffset + ioreq2.cbData;
    ioreq3.cbData = 1024;
    ioreq3.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq3 ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq4.p_osf = &_osfB;
    ioreq4.ibOffset = ioreq3.ibOffset + ioreq3.cbData;
    ioreq4.cbData = 1024;
    ioreq4.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq4 ) );

    ioreq5.p_osf = &_osfA;
    ioreq5.ibOffset = 12311 * 1024;
    ioreq5.cbData = 1024;
    ioreq5.m_fHasHeapReservation = fTrue;
    CHECK( &ioreq1 == iorunpool.PioreqSwapAddIoreq( &ioreq5 ) );

    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoRunPool, TestCapacityEvictOldestIorun )
{
    INIT_IOREQ_JUNK;

    COSDisk::CIoRunPool iorunpool;

    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 0 );

    ioreq1.p_osf = &_osfA;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.hrtIOStart = 0x100;
    ioreq1.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );

    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq2.p_osf = &_osfA;
    ioreq2.ibOffset = ioreq1.ibOffset + ioreq1.cbData;
    ioreq2.cbData = 1024;
    ioreq2.hrtIOStart = 0x110;
    ioreq2.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq2 ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq3.p_osf = &_osfA;
    ioreq3.ibOffset = ioreq2.ibOffset + ioreq2.cbData;
    ioreq3.cbData = 1024;
    ioreq3.hrtIOStart = 0x120;
    ioreq3.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq3 ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq4.p_osf = &_osfB;
    ioreq4.ibOffset = ioreq3.ibOffset + ioreq3.cbData;
    ioreq4.cbData = 1024;
    ioreq4.hrtIOStart = 0x200;
    ioreq4.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq4 ) );

    ioreq5.p_osf = &_osfC;
    ioreq5.ibOffset = 12311 * 1024;
    ioreq5.cbData = 1024;
    ioreq5.hrtIOStart = 0x300;
    ioreq5.m_fHasHeapReservation = fTrue;

    CHECK( &ioreq1 == iorunpool.PioreqSwapAddIoreq( &ioreq5 ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfA ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfB ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfC ) );

    ioreq1.p_osf = &_osfD;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.hrtIOStart = 0x400;
    ioreq1.m_fHasHeapReservation = fTrue;

    CHECK( &ioreq4 == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfB ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfC ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfD ) );

    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoRunPool, TestCapacityEvictOldestIorunOutOfOrderInsert )
{
    INIT_IOREQ_JUNK;

    COSDisk::CIoRunPool iorunpool;

    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 0 );


    ioreq4.p_osf = &_osfB;
    ioreq4.ibOffset = ioreq3.ibOffset + ioreq3.cbData;
    ioreq4.cbData = 1024;
    ioreq4.hrtIOStart = 0x200;
    ioreq4.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq4 ) );

    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq1.p_osf = &_osfA;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.hrtIOStart = 0x100;
    ioreq1.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );

    CHECKDBG( iorunpool.Cioruns() == 2 );

    ioreq2.p_osf = &_osfA;
    ioreq2.ibOffset = ioreq1.ibOffset + ioreq1.cbData;
    ioreq2.cbData = 1024;
    ioreq2.hrtIOStart = 0x110;
    ioreq2.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq2 ) );

    ioreq3.p_osf = &_osfA;
    ioreq3.ibOffset = ioreq2.ibOffset + ioreq2.cbData;
    ioreq3.cbData = 1024;
    ioreq3.hrtIOStart = 0x120;
    ioreq3.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq3 ) );

    ioreq5.p_osf = &_osfC;
    ioreq5.ibOffset = 12311 * 1024;
    ioreq5.cbData = 1024;
    ioreq5.hrtIOStart = 0x300;
    ioreq5.m_fHasHeapReservation = fTrue;

    CHECK( &ioreq1 == iorunpool.PioreqSwapAddIoreq( &ioreq5 ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfA ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfB ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfC ) );

    ioreq1.p_osf = &_osfD;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.hrtIOStart = 0x400;
    ioreq1.m_fHasHeapReservation = fTrue;

    CHECK( &ioreq4 == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfB ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfC ) );
    CHECK( iorunpool.FContainsFileIoRun( &_osfD ) );

    TERM_IOREQ_JUNK;
}


JETUNITTEST( IoRunPool, TestExplicitEvictFileIorunsWorks )
{
    INIT_IOREQ_JUNK;
    IOREQ * pioreqT = NULL;

    COSDisk::CIoRunPool iorunpool;

    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 0 );

    ioreq1.p_osf = &_osfA;
    ioreq1.ibOffset = 8192;
    ioreq1.cbData = 1024;
    ioreq1.hrtIOStart = 0x100;
    ioreq1.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq1 ) );

    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq2.p_osf = &_osfA;
    ioreq2.ibOffset = ioreq1.ibOffset + ioreq1.cbData;
    ioreq2.cbData = 1024;
    ioreq2.hrtIOStart = 0x110;
    ioreq2.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq2 ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq3.p_osf = &_osfA;
    ioreq3.ibOffset = ioreq2.ibOffset + ioreq2.cbData;
    ioreq3.cbData = 1024;
    ioreq3.hrtIOStart = 0x120;
    ioreq3.m_fCanCombineIO = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq3 ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq4.p_osf = &_osfB;
    ioreq4.ibOffset = ioreq3.ibOffset + ioreq3.cbData;
    ioreq4.cbData = 1024;
    ioreq4.hrtIOStart = 0x200;
    ioreq4.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq4 ) );
    CHECKDBG( iorunpool.Cioruns() == 2 );

    INT c = 0;
    while( pioreqT = iorunpool.PioreqEvictFileRuns( &_osfB ) )
    {
        CHECK( pioreqT->p_osf == &_osfB );
        CHECK( pioreqT == &ioreq4 );
        c++;
    }
    CHECK( c == 1 );
    CHECK( iorunpool.FContainsFileIoRun( &_osfA ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfB ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );
    CHECK( !iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 1 );

    ioreq4.p_osf = &_osfC;
    ioreq4.ibOffset = ioreq3.ibOffset + ioreq3.cbData;
    ioreq4.cbData = 1024;
    ioreq4.hrtIOStart = 0x300;
    ioreq4.m_fHasHeapReservation = fTrue;
    CHECK( NULL == iorunpool.PioreqSwapAddIoreq( &ioreq4 ) );

    c = 0;
    while( pioreqT = iorunpool.PioreqEvictFileRuns( NULL ) )
    {
        c++;
    }
    CHECK( c == 2 );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfA ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfB ) );
    CHECK( !iorunpool.FContainsFileIoRun( &_osfC ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckOneEmpty ) );
    CHECK( iorunpool.FEmpty( COSDisk::CIoRunPool::fCheckAllEmpty ) );
    CHECKDBG( iorunpool.Cioruns() == 0 );

    TERM_IOREQ_JUNK;
}


JETUNITTEST( IoQueue, TestBasicQopOperations )
{
    INIT_IOREQ_JUNK;
    COSDisk::QueueOp qop1;

    qop1.FAddToRun( &ioreq1 );
    CHECK( !qop1.FEmpty() );

    CHECK( &ioreq1 == qop1.PioreqGetRun() );
    CHECK( qop1.FEmpty() );

    TERM_IOREQ_JUNK;
}


#include "osstd_.hxx"

void FlightConcurrentMetedOps( INT cioOpsMax, INT cioLowThreshold, TICK dtickStarvation );

extern LONG g_cioConcurrentMetedOpsMax;
extern LONG g_cioLowQueueThreshold;
extern LONG g_dtickStarvedMetedOpThreshold;


JETUNITTEST( IoQueue, TestQueueSingleIoInsertExtract )
{
    INIT_IOREQ_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;

    CCriticalSection critQ( CLockBasicInfo( CSyncBasicInfo( "IO Queue/Heap" ), rankOSDiskIOQueueCrit, 0 ) );
    COSDisk::IOQueueToo q( &critQ, COSDisk::IOQueueToo::qitRead );
    Postls()->fIOThread = fTrue;

    CHECK( q.CioEnqueued() == 0 );
    CHECK( !q.FStarvedMetedOp() );

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );

    qop1.FAddToRun( &ioreq1 );
    CHECK( !qop1.FEmpty() );
    qop1.SetIOREQType( IOREQ::ioreqEnqueuedInMetedQ );

    IOREQ * pioreqT = NULL;
    critQ.Enter();
    q.FInsertIo( qop1.PioreqGetRun(), 1, &pioreqT );
    critQ.Leave();
    CHECK( qop1.FEmpty() );

    CHECK( q.CioEnqueued() == 1 );
    
    CHECK( qop2.FEmpty() );
    critQ.Enter();
    BOOL fCycling = fTrue;
    q.ExtractIo( &qop2, &pioreqT, &fCycling );
    critQ.Leave();
    CHECK( !qop2.FEmpty() );
    
    CHECK( q.CioEnqueued() == 0 );

    CHECK( &ioreq1 == qop2.PioreqGetRun() );
    CHECK( qop2.FEmpty() );

    Postls()->fIOThread = fFalse;
    TERM_IOREQ_JUNK;
}


JETUNITTEST( IoQueue, TestQueueSingleIoGetsStarved )
{
    INIT_IOREQ_JUNK;
    COSDisk::QueueOp qop1;

    _osfA.m_posd = (COSDisk*)0x42;

    const LONG cioOpsMaxSaved = g_cioConcurrentMetedOpsMax;
    const LONG cioThresholdSaved = g_cioLowQueueThreshold;
    const LONG dtickStarvedSaved = g_dtickStarvedMetedOpThreshold;

    CCriticalSection critQ( CLockBasicInfo( CSyncBasicInfo( "IO Queue/Heap" ), rankOSDiskIOQueueCrit, 0 ) );
    COSDisk::IOQueueToo q( &critQ, COSDisk::IOQueueToo::qitRead );
    Postls()->fIOThread = fTrue;

    CHECK( !q.FStarvedMetedOp() );

    TICK dtickStarved = 15;
    ULONG cTries = 0;

RetryRaceToAvoidStarved:

    cTries++;
    dtickStarved *= 2;

    FlightConcurrentMetedOps( 2, 10, dtickStarved );

    TICK tickStart = TickOSTimeCurrent();

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );

    qop1.FAddToRun( &ioreq1 );
    CHECK( !qop1.FEmpty() );
    qop1.SetIOREQType( IOREQ::ioreqEnqueuedInMetedQ );

    IOREQ * pioreqT = NULL;
    critQ.Enter();
    q.FInsertIo( qop1.PioreqGetRun(), 1, &pioreqT );
    critQ.Leave();
    CHECK( qop1.FEmpty() );

    if ( q.FStarvedMetedOp() )
    {
        CHECK( dtickStarved < 2000 );
        critQ.Enter();
        pioreqT = NULL;
        BOOL fCycling = fTrue;
        q.ExtractIo( &qop1, &pioreqT, &fCycling );
        critQ.Leave();
        pioreqT = qop1.PioreqGetRun();
        pioreqT->SetIOREQType( IOREQ::ioreqRemovedFromQueue );
        pioreqT->BeginIO( IOREQ::iomethodAsync, HrtHRTCount(), fTrue );
        pioreqT->CompleteIO( IOREQ::CompletedIO );
        goto RetryRaceToAvoidStarved;
    }

    ULONG cSleeps = 0;
    while( !q.FStarvedMetedOp() )
    {
        UtilSleep( dtickStarved / 2 );
        cSleeps++;
        CHECK( cSleeps < 10 );
    }
    const TICK dtickTotalWait = DtickDelta( tickStart, TickOSTimeCurrent() );
    wprintf( L"\n\t cTries = %d    { dtickStarved = %d, cSleepAttempts = %d, dtickTotalWait = %d }\n", cTries, dtickStarved, cSleeps, dtickTotalWait );

    CHECK( dtickTotalWait >= ( dtickStarved - 1 ) );

    CHECK( qop1.FEmpty() );
    critQ.Enter();
    BOOL fCycling = fTrue;
    q.ExtractIo( &qop1, &pioreqT, &fCycling );
    critQ.Leave();
    CHECK( !qop1.FEmpty() );
    
    FlightConcurrentMetedOps( cioOpsMaxSaved, cioThresholdSaved, dtickStarvedSaved );

    Postls()->fIOThread = fFalse;
    TERM_IOREQ_JUNK;
}


ERR ErrOSDiskConnect( __in_z const WCHAR * const wszDiskPathId, __in const DWORD dwDiskNumber, __out IDiskAPI ** ppdiskapi );
void OSDiskDisconnect( __inout IDiskAPI * pdiskapi, __in const _OSFILE * p_osf );
extern COSFilePerfDummy g_cosfileperfDefault;

extern LONG g_cioConcurrentMetedOpsMax;
extern LONG g_cioLowQueueThreshold;
extern LONG g_dtickStarvedMetedOpThreshold;

#define ibGB    (QWORD)(1024 * 1024 * 1024)

#define INIT_OSDISK_QUEUE_JUNK                                          \
    const LONG cioOpsMax = g_cioConcurrentMetedOpsMax;                  \
    const LONG cioThreshold = g_cioLowQueueThreshold;                   \
    const LONG dtickStarved = g_dtickStarvedMetedOpThreshold;           \
    Postls()->fIOThread = fTrue;                                        \
    COSDisk * posd = NULL;                                              \
    CallS( ErrOSDiskConnect( L"TestDisk", 0x42, (IDiskAPI**)&posd ) );  \
    _OSFILE _osf;    \
    _osf.m_posd = posd;                                                 \
    _osf.fRegistered = fTrue;       \
    _osf.pfsconfig = &fsconfig;                                         \
    _osf.pfpapi = &g_cosfileperfDefault;

#define TERM_OSDISK_QUEUE_JUNK                  \
    Postls()->fIOThread = fFalse;               \
    OSDiskDisconnect( (IDiskAPI*)posd, NULL );  \
    FlightConcurrentMetedOps( cioOpsMax, cioThreshold, dtickStarved );

JETUNITTEST( IoQueue, TestIoreqFullCycle )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop;

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );
    ioreq1.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1024, 1024 );
    ioreq2.grbitQOS = qosIODispatchImmediate;

    CallS( posd->ErrReserveQueueSpace( ioreq1.grbitQOS, &ioreq1 ) );

    posd->EnqueueIORun( &ioreq1 );
    CHECK( ioreq1.FEnqueuedInMetedQ() );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioAllowedMetedOps( 1 ) == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );

    const ERR errD = posd->ErrDequeueIORun( &qop );
    CHECK( JET_errSuccess == errD );
    CHECK( ioreq1.FRemovedFromQueue() );
    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioAllowedMetedOps( 0 ) == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    ioreq1.BeginIO( IOREQ::iomethodAsync, HrtHRTCount(), fTrue );
    CHECK( !ioreq1.FRemovedFromQueue() );
    CHECK( ioreq1.FIssuedAsyncIO() );
    CHECK( ioreq1.FOSIssuedState() );

    posd->QueueCompleteIORun( qop.PioreqGetRun() );
    CHECK( !ioreq1.FRemovedFromQueue() );
    CHECK( !ioreq1.FIssuedAsyncIO() );
    CHECK( !ioreq1.FOSIssuedState() );
    CHECK( ioreq1.FCompleted() );
    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioAllowedMetedOps( 0 ) == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    BOOL f = posd->FQueueCompleteIOREQ( &ioreq1 );
    CHECK( !f );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueMetedBasicIoInsertExtract )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop;

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );
    ioreq1.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1024, 1024 );
    ioreq2.grbitQOS = qosIODispatchImmediate;

    posd->EnqueueIORun( &ioreq1 );
    CHECK( ioreq1.FEnqueuedInMetedQ() );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioAllowedMetedOps( 1 ) == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );

    const ERR errD = posd->ErrDequeueIORun( &qop );
    CHECK( JET_errSuccess == errD );
    CHECK( ioreq1.FRemovedFromQueue() );
    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioAllowedMetedOps( 0 ) == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    posd->QueueCompleteIORun( qop.PioreqGetRun() );
    CHECK( !ioreq1.FRemovedFromQueue() );
    CHECK( ioreq1.FCompleted() );
    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioAllowedMetedOps( 0 ) == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueSingleIoreqChainInsertExtractForAllDispatchLevels )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop3;
    COSDisk::QueueOp qop5;

    FlightConcurrentMetedOps( 2, 0, 5000 );

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );        ioreq1.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1024, 1024 );   ioreq2.grbitQOS = qosIODispatchImmediate;
    ioreq1.pioreqIorunNext = &ioreq2;

    InitIoreq( &ioreq3, _osf, fReadIo, 1*ibGB, 1024 );      ioreq3.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq4, _osf, fReadIo, 1*ibGB+1024, 1024 ); ioreq4.grbitQOS = qosIODispatchBackground;
    ioreq3.pioreqIorunNext = &ioreq4;

    InitIoreq( &ioreq5, _osf, fReadIo, 3*ibGB, 1024 );      ioreq5.grbitQOS = qosIODispatchImmediate;
    ioreq5.m_fCanCombineIO = fFalse;
    ioreq5.m_fHasHeapReservation = fFalse;

    posd->EnqueueIORun( &ioreq1 );  CHECK( ioreq1.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq3 );  CHECK( ioreq3.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq5 );  CHECK( ioreq5.FEnqueuedInVIPList() );

    CHECK( posd->CioAllEnqueued() == 3 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );

    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 2 );
    CHECK( posd->CioUrgentEnqueued() == 1 );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 1 );
    CHECK( posd->ErrDequeueIORun( &qop5 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    IOREQ * pioreq5 = qop1.PioreqGetRun();
    IOREQ * pioreq3 = qop3.PioreqGetRun();
    IOREQ * pioreq1 = qop5.PioreqGetRun();
    CHECK( pioreq5 == &ioreq5 );
    CHECK( pioreq3 == &ioreq3 );
    CHECK( pioreq3->pioreqIorunNext == &ioreq4 );
    CHECK( pioreq1 == &ioreq1 );
    CHECK( pioreq1->pioreqIorunNext == &ioreq2 );

    posd->QueueCompleteIORun( pioreq1 );
    posd->QueueCompleteIORun( pioreq3 );
    posd->QueueCompleteIORun( pioreq5 );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

void RandomSwapIoreqArray( IOREQ ** prgioreq, LONG cioreq )
{
    for( LONG iter = 0; iter < 2 * cioreq; iter++ )
    {
        const LONG iioreqOne = rand() % cioreq;
        const LONG iioreqTwo = rand() % cioreq;
        IOREQ * pioreqTemp = prgioreq[ iioreqOne ];
        prgioreq[ iioreqOne ] = prgioreq[ iioreqTwo ];
        prgioreq[ iioreqTwo ] = pioreqTemp;
    }
}

JETUNITTEST( IoQueue, TestQueueIODispatchMetedQAndWritesDequeuedInOffsetOrder )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;
    COSDisk::QueueOp qop3;
    COSDisk::QueueOp qop4;
    COSDisk::QueueOp qop5;

    FlightConcurrentMetedOps( 2, 0, 5000 );


    BOOL fReadOrWrite = rand() % 2;
    InitIoreq( &ioreq1, _osf, fReadOrWrite,  8192, 1024 );        ioreq1.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq2, _osf, !fReadOrWrite, 8192+1*ibGB, 1024 ); ioreq2.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq3, _osf, fReadOrWrite,  8192+2*ibGB, 1024 ); ioreq3.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq4, _osf, !fReadOrWrite, 8192+3*ibGB, 1024 ); ioreq4.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq5, _osf, fWriteIo,      8192+4*ibGB, 1024 ); ioreq5.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;


    IOREQ * rgpioreqEngOrder[5] = { &ioreq1, &ioreq2, &ioreq3, &ioreq4, &ioreq5 };
    RandomSwapIoreqArray( rgpioreqEngOrder, _countof( rgpioreqEngOrder ) );
    wprintf( L"   Enqueue order (%hs) \n \t ioreq1 = %p -> %p \n \t ioreq2 = %p -> %p \n \t ioreq3 = %p -> %p \n \t ioreq4 = %p -> %p \n \t ioreq5 = %p -> %p \n",
        fReadOrWrite ? "WriteFirst" : "ReadFirst",
        &ioreq1, rgpioreqEngOrder[0], &ioreq2, rgpioreqEngOrder[1], &ioreq3, rgpioreqEngOrder[2], &ioreq4, rgpioreqEngOrder[3], &ioreq5, rgpioreqEngOrder[4] );

    posd->EnqueueIORun( rgpioreqEngOrder[0] );  CHECK( rgpioreqEngOrder[0]->FEnqueuedInMetedQ() );
    posd->EnqueueIORun( rgpioreqEngOrder[1] );  CHECK( rgpioreqEngOrder[1]->FEnqueuedInMetedQ() );
    posd->EnqueueIORun( rgpioreqEngOrder[2] );  CHECK( rgpioreqEngOrder[2]->FEnqueuedInMetedQ() );
    posd->EnqueueIORun( rgpioreqEngOrder[3] );  CHECK( rgpioreqEngOrder[3]->FEnqueuedInMetedQ() );
    posd->EnqueueIORun( rgpioreqEngOrder[4] );  CHECK( rgpioreqEngOrder[4]->FEnqueuedInMetedQ() );

    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 2 );
    CHECK( posd->CioUrgentEnqueued() == 3 );

    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    CHECK( 4 == posd->CioAllEnqueued() );
    CHECK( 4 == posd->CioReadyMetedEnqueued() + posd->CioUrgentEnqueued() );
    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );
    CHECK( 3 == posd->CioAllEnqueued() );
    CHECK( 3 == posd->CioReadyMetedEnqueued() + posd->CioUrgentEnqueued() );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( 2 == posd->CioAllEnqueued() );
    CHECK( 2 == posd->CioReadyMetedEnqueued() + posd->CioUrgentEnqueued() );
    CHECK( posd->ErrDequeueIORun( &qop4 ) == JET_errSuccess );
    CHECK( 1 == posd->CioAllEnqueued() );
    CHECK( 1 == posd->CioReadyMetedEnqueued() + posd->CioUrgentEnqueued() );
    CHECK( posd->ErrDequeueIORun( &qop5 ) == JET_errSuccess );

    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    IOREQ * pioreqT;

    pioreqT = qop1.PioreqGetRun();
    CHECK( &ioreq1 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop2.PioreqGetRun();
    CHECK( &ioreq2 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop3.PioreqGetRun();
    CHECK( &ioreq3 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop4.PioreqGetRun();
    CHECK( &ioreq4 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop5.PioreqGetRun();
    CHECK( &ioreq5 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueIODispatchImmediateAllDequeue )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;
    COSDisk::QueueOp qop3;
    COSDisk::QueueOp qop4;
    COSDisk::QueueOp qop5;
    COSDisk::QueueOp qop6;

    FlightConcurrentMetedOps( 2, 0, 5000 );

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );        ioreq1.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1*ibGB, 1024 ); ioreq2.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq3, _osf, fReadIo, 8192+2*ibGB, 1024 ); ioreq3.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq4, _osf, fReadIo, 8192+3*ibGB, 1024 ); ioreq4.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq5, _osf, fReadIo, 8192+4*ibGB, 1024 ); ioreq5.grbitQOS = qosIODispatchImmediate;

    posd->EnqueueIORun( &ioreq1 );  CHECK( ioreq1.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq2 );  CHECK( ioreq2.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq3 );  CHECK( ioreq3.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq4 );  CHECK( ioreq4.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq5 );  CHECK( ioreq5.FEnqueuedInHeap() );

    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 4 );
    CHECK( posd->CioUrgentEnqueued() == 4 );

    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 3 );
    CHECK( posd->CioUrgentEnqueued() == 3 );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 2 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->ErrDequeueIORun( &qop4 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 1 );
    CHECK( posd->ErrDequeueIORun( &qop5 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    FNegTestSet( fInvalidUsage );
    CHECK( posd->ErrDequeueIORun( &qop6 ) == errDiskTilt );
    FNegTestUnset( fInvalidUsage );

    posd->QueueCompleteIORun( qop1.PioreqGetRun() );
    posd->QueueCompleteIORun( qop2.PioreqGetRun() );
    posd->QueueCompleteIORun( qop3.PioreqGetRun() );
    posd->QueueCompleteIORun( qop4.PioreqGetRun() );
    posd->QueueCompleteIORun( qop5.PioreqGetRun() );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueIODispatchAllReadAndWritesInOrderWithinOneCycle )
{
    INIT_IOREQ_JUNK;
    INIT_IOREQ_EXTRAS;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;
    COSDisk::QueueOp qop3;
    COSDisk::QueueOp qop4;
    COSDisk::QueueOp qop5;
    COSDisk::QueueOp qop6;
    COSDisk::QueueOp qop7;

    COSDisk::QueueOp qop101;
    COSDisk::QueueOp qop102;
    COSDisk::QueueOp qop103;
    COSDisk::QueueOp qop104;
    COSDisk::QueueOp qopExtra;

    FlightConcurrentMetedOps( 2, 0, 900000  );

    InitIoreq( &ioreq1, _osf, fReadIo,  8192, 1024 );        ioreq1.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq2, _osf, fWriteIo, 8192+1*ibGB, 1024 ); ioreq2.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq3, _osf, fReadIo,  8192+2*ibGB, 1024 ); ioreq3.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq4, _osf, fWriteIo, 8192+3*ibGB, 1024 ); ioreq4.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq5, _osf, fReadIo,  8192+4*ibGB, 1024 ); ioreq5.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq6, _osf, fWriteIo, 8192+5*ibGB, 1024 ); ioreq6.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq7, _osf, fReadIo,  8192+6*ibGB, 1024 ); ioreq7.grbitQOS = qosIODispatchBackground;

    const QWORD ibShifted = 50*1024*1024;
    InitIoreq( &ioreq101, _osf, fReadIo,  ibShifted+8192+1*ibGB, 1024 ); ioreq101.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq102, _osf, fWriteIo, ibShifted+8192+2*ibGB, 1024 ); ioreq102.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq103, _osf, fReadIo,  ibShifted+8192+10*ibGB, 1024 ); ioreq103.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;
    InitIoreq( &ioreq104, _osf, fWriteIo, ibShifted+8192+11*ibGB, 1024 ); ioreq104.grbitQOS = qosIODispatchBackground | qosIODispatchWriteMeted;



    posd->EnqueueIORun( &ioreq1 );  CHECK( ioreq1.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq2 );  CHECK( ioreq2.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq3 );  CHECK( ioreq3.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq4 );  CHECK( ioreq4.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq5 );  CHECK( ioreq5.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq6 );  CHECK( ioreq6.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq7 );  CHECK( ioreq7.FEnqueuedInMetedQ() );

    CHECK( posd->CioAllEnqueued() == 7 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 2 );
    CHECK( posd->CioUrgentEnqueued() == 3 );

    IOREQ * pioreqT;


    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    OnDebug( CHECK( qop1.PiorunIO()->PioreqHead() == &ioreq1 ) );

    CHECK( posd->CioAllEnqueued() == 6 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 3 );


    posd->EnqueueIORun( &ioreq101 );  CHECK( ioreq101.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq102 );  CHECK( ioreq102.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq103 );  CHECK( ioreq103.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq104 );  CHECK( ioreq104.FEnqueuedInMetedQ() );

    CHECK( posd->CioAllEnqueued() == 10 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 5 );

    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );
    CHECK( qop2.FWrite() );
    OnDebug( CHECK( qop2.PiorunIO()->PioreqHead() == &ioreq2 ) );

    CHECK( posd->CioAllEnqueued() == 9 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 4 );

    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( !qop3.FWrite() );
    OnDebug( CHECK( qop3.PiorunIO()->PioreqHead() == &ioreq3 ) );

    CHECK( posd->CioAllEnqueued() == 8 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 4 );

    CHECK( posd->ErrDequeueIORun( &qop4 ) == JET_errSuccess );
    CHECK( qop4.FWrite() );
    OnDebug( CHECK( qop4.PiorunIO()->PioreqHead() == &ioreq4 ) );

    CHECK( posd->CioAllEnqueued() == 7 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 3 );


    CHECK( posd->ErrDequeueIORun( &qop6 ) == JET_errSuccess );
    CHECK( qop6.FWrite() );
    OnDebug( CHECK( qop6.PiorunIO()->PioreqHead() == &ioreq6 ) );

    CHECK( posd->CioAllEnqueued() == 6 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );


    CHECK( posd->ErrDequeueIORun( &qop7 ) == errDiskTilt );
    CHECK( qop7.FEmpty() );
    CHECK( posd->ErrDequeueIORun( &qopExtra ) == errDiskTilt );
    CHECK( qopExtra.FEmpty() );

    CHECK( posd->CioAllEnqueued() == 6 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );


    pioreqT = qop1.PioreqGetRun();
    CHECK( !pioreqT->fWrite );
    CHECK( &ioreq1 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    CHECK( qop1.FEmpty() );



    CHECK( posd->CioReadyMetedEnqueued() == 1 );

    CHECK( posd->ErrDequeueIORun( &qop5 ) == JET_errSuccess );
    CHECK( !qop5.FWrite() );
    OnDebug( CHECK( qop5.PiorunIO()->PioreqHead() == &ioreq5 ) );

    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    if ( rand() % 2 )
    {
        pioreqT = qop4.PioreqGetRun();
        CHECK( pioreqT->fWrite );
        CHECK( &ioreq4 == pioreqT );
        posd->QueueCompleteIORun( pioreqT );
        CHECK( qop4.FEmpty() );
    }

    CHECK( posd->ErrDequeueIORun( &qop7 ) == errDiskTilt );
    CHECK( qop7.FEmpty() );
    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );


    pioreqT = qop3.PioreqGetRun();
    posd->QueueCompleteIORun( pioreqT );
    CHECK( !pioreqT->fWrite );
    CHECK( &ioreq3 == pioreqT );

    CHECK( posd->CioReadyMetedEnqueued() == 1 );

    CHECK( posd->ErrDequeueIORun( &qop7 ) == JET_errSuccess );
    CHECK( !qop7.FWrite() );
    OnDebug( CHECK( qop7.PiorunIO()->PioreqHead() == &ioreq7 ) );

    CHECK( posd->CioAllEnqueued() == 4 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );




    if ( rand() % 2 )
    {

        CHECK( posd->CioReadyMetedEnqueued() == 0 );

        CHECK( posd->ErrDequeueIORun( &qop102 ) == JET_errSuccess );
        CHECK( qop102.FWrite() );
        OnDebug( CHECK( qop102.PiorunIO()->PioreqHead() == &ioreq102 ) );

        CHECK( posd->CioAllEnqueued() == 3 );
        CHECK( posd->CioUrgentEnqueued() == 1 );
        CHECK( posd->CioReadyMetedEnqueued() == 0 );

        if ( rand() % 2 )
        {
            CHECK( posd->ErrDequeueIORun( &qop104 ) == JET_errSuccess );
            CHECK( qop104.FWrite() );

            CHECK( posd->CioAllEnqueued() == 2 );
            CHECK( posd->CioUrgentEnqueued() == 0 );
            CHECK( posd->CioReadyMetedEnqueued() == 0 );

            FNegTestSet( fInvalidUsage );
            CHECK( posd->ErrDequeueIORun( &qopExtra ) == errDiskTilt );
            FNegTestUnset( fInvalidUsage );


            CHECK( posd->CioAllEnqueued() == 2 );
            CHECK( posd->CioUrgentEnqueued() == 0 );
            CHECK( posd->CioReadyMetedEnqueued() == 0 );

            CHECK( posd->CioReadyMetedEnqueued() == 0 );
            pioreqT = qop7.PioreqGetRun();
            posd->QueueCompleteIORun( pioreqT );
            CHECK( !pioreqT->fWrite );
            CHECK( &ioreq7 == pioreqT );
            CHECK( posd->CioReadyMetedEnqueued() == 1 );

            CHECK( posd->ErrDequeueIORun( &qop101 ) == JET_errSuccess );
            CHECK( !qop101.FWrite() );

            CHECK( posd->CioAllEnqueued() == 1 );
            CHECK( posd->CioUrgentEnqueued() == 0 );
            CHECK( posd->CioReadyMetedEnqueued() == 0 );
        }
        else
        {
            CHECK( posd->CioReadyMetedEnqueued() == 0 );
            pioreqT = qop7.PioreqGetRun();
            posd->QueueCompleteIORun( pioreqT );
            CHECK( !pioreqT->fWrite );
            CHECK( &ioreq7 == pioreqT );
            CHECK( posd->CioReadyMetedEnqueued() == 1 );

            CHECK( posd->ErrDequeueIORun( &qop101 ) == JET_errSuccess );
            CHECK( !qop101.FWrite() );

            CHECK( posd->CioAllEnqueued() == 2 );
            CHECK( posd->CioUrgentEnqueued() == 1 );
            CHECK( posd->CioReadyMetedEnqueued() == 0 );

            CHECK( posd->ErrDequeueIORun( &qop104 ) == JET_errSuccess );
            CHECK( qop104.FWrite() );

            CHECK( posd->CioAllEnqueued() == 1 );
            CHECK( posd->CioUrgentEnqueued() == 0 );
            CHECK( posd->CioReadyMetedEnqueued() == 0 );
        }
    }
    else
    {

        CHECK( posd->CioReadyMetedEnqueued() == 0 );
        pioreqT = qop7.PioreqGetRun();
        posd->QueueCompleteIORun( pioreqT );
        CHECK( !pioreqT->fWrite );
        CHECK( &ioreq7 == pioreqT );
        CHECK( posd->CioReadyMetedEnqueued() == 1 );

        CHECK( posd->ErrDequeueIORun( &qop101 ) == JET_errSuccess );
        CHECK( !qop101.FWrite() );

        CHECK( posd->CioAllEnqueued() == 3 );
        CHECK( posd->CioUrgentEnqueued() == 2 );
        CHECK( posd->CioReadyMetedEnqueued() == 0 );

        CHECK( posd->ErrDequeueIORun( &qop102 ) == JET_errSuccess );
        CHECK( qop102.FWrite() );

        CHECK( posd->CioAllEnqueued() == 2 );
        CHECK( posd->CioUrgentEnqueued() == 1 );

        CHECK( posd->ErrDequeueIORun( &qop104 ) == JET_errSuccess );
        CHECK( qop104.FWrite() );

        CHECK( posd->CioAllEnqueued() == 1 );
        CHECK( posd->CioUrgentEnqueued() == 0 );
    }

    FNegTestSet( fInvalidUsage );
    CHECK( posd->ErrDequeueIORun( &qopExtra ) == errDiskTilt );
    FNegTestUnset( fInvalidUsage );


    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 0 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );

    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    posd->QueueCompleteIORun( qop5.PioreqGetRun() );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );

    CHECK( posd->ErrDequeueIORun( &qop103 ) == JET_errSuccess );
    CHECK( !qop103.FWrite() );

    FNegTestSet( fInvalidUsage );
    CHECK( posd->ErrDequeueIORun( &qopExtra ) == errDiskTilt );
    FNegTestUnset( fInvalidUsage );


    CHECK( qop1.FEmpty() );
    CHECK( qop3.FEmpty() );
    CHECK( qop5.FEmpty() );
    CHECK( qop7.FEmpty() );

    pioreqT = qop2.PioreqGetRun();
    CHECK( &ioreq2 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    if ( !qop4.FEmpty() )
    {
        pioreqT = qop4.PioreqGetRun();
        CHECK( &ioreq4 == pioreqT );
        posd->QueueCompleteIORun( pioreqT );
    }

    pioreqT = qop6.PioreqGetRun();
    CHECK( &ioreq6 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    pioreqT = qop101.PioreqGetRun();
    CHECK( &ioreq101 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    pioreqT = qop102.PioreqGetRun();
    CHECK( &ioreq102 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    pioreqT = qop103.PioreqGetRun();
    CHECK( &ioreq103 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    pioreqT = qop104.PioreqGetRun();
    CHECK( &ioreq104 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueIODispatchBackgroundPushesBackOnDequeue )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;
    COSDisk::QueueOp qop3;

    FlightConcurrentMetedOps( 2, 0, 5000 );

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );        ioreq1.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1*ibGB, 1024 ); ioreq2.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq3, _osf, fReadIo, 8192+2*ibGB, 1024 ); ioreq3.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq4, _osf, fReadIo, 8192+3*ibGB, 1024 ); ioreq4.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq5, _osf, fReadIo, 8192+4*ibGB, 1024 ); ioreq5.grbitQOS = qosIODispatchBackground;

    posd->EnqueueIORun( &ioreq1 );  CHECK( ioreq1.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq2 );  CHECK( ioreq2.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq3 );  CHECK( ioreq3.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq4 );  CHECK( ioreq4.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq5 );  CHECK( ioreq5.FEnqueuedInMetedQ() );

    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 2 );

    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioAllEnqueued() == 4 );
    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );

    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );
    CHECK( posd->CioAllEnqueued() == 3 );
    FNegTestSet( fInvalidUsage );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == errDiskTilt );
    FNegTestUnset( fInvalidUsage );
    CHECK( posd->CioAllEnqueued() == 3 );
    CHECK( qop3.FEmpty() );

    posd->QueueCompleteIORun( qop1.PioreqGetRun() );

    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioAllEnqueued() == 3 );

    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 2 );
    CHECK( !qop3.FEmpty() );

    posd->QueueCompleteIORun( qop2.PioreqGetRun() );
    posd->QueueCompleteIORun( qop3.PioreqGetRun() );
    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    posd->QueueCompleteIORun( qop2.PioreqGetRun() );
    posd->QueueCompleteIORun( qop3.PioreqGetRun() );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueIODispatchMixedEmptiesMetedThenHeapAndStalls )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;
    COSDisk::QueueOp qop3;
    COSDisk::QueueOp qop4;
    COSDisk::QueueOp qop5;

    FlightConcurrentMetedOps( 2, 0, 5000 );

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );        ioreq1.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1*ibGB, 1024 ); ioreq2.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq3, _osf, fReadIo, 8192+2*ibGB, 1024 ); ioreq3.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq4, _osf, fReadIo, 8192+3*ibGB, 1024 ); ioreq4.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq5, _osf, fReadIo, 8192+4*ibGB, 1024 ); ioreq5.grbitQOS = qosIODispatchImmediate;

    posd->EnqueueIORun( &ioreq1 );  CHECK( ioreq1.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq2 );  CHECK( ioreq2.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq3 );  CHECK( ioreq3.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq4 );  CHECK( ioreq4.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq5 );  CHECK( ioreq5.FEnqueuedInHeap() );

    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 2 );
    CHECK( posd->CioUrgentEnqueued() == 2 );

    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 4 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 3 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 1 );
    CHECK( posd->ErrDequeueIORun( &qop4 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    FNegTestSet( fInvalidUsage );
    CHECK( posd->ErrDequeueIORun( &qop5 ) == errDiskTilt );
    FNegTestUnset( fInvalidUsage );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 0 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    IOREQ * pioreqT;

    pioreqT = qop1.PioreqGetRun();
    CHECK( &ioreq2 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop2.PioreqGetRun();
    CHECK( &ioreq3 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop3.PioreqGetRun();
    CHECK( &ioreq1 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    pioreqT = qop4.PioreqGetRun();
    CHECK( &ioreq5 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    CHECK( posd->ErrDequeueIORun( &qop5 ) == JET_errSuccess );
    pioreqT = qop5.PioreqGetRun();
    CHECK( &ioreq4 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}

JETUNITTEST( IoQueue, TestQueueIODispatchMixedEmptiesVipOpsFirst )
{
    INIT_IOREQ_JUNK;
    INIT_OSDISK_QUEUE_JUNK;
    COSDisk::QueueOp qop1;
    COSDisk::QueueOp qop2;
    COSDisk::QueueOp qop3;
    COSDisk::QueueOp qop4;
    COSDisk::QueueOp qop5;

    FlightConcurrentMetedOps( 2, 0, 5000 );

    InitIoreq( &ioreq1, _osf, fReadIo, 8192, 1024 );        ioreq1.grbitQOS = qosIODispatchImmediate;
    InitIoreq( &ioreq2, _osf, fReadIo, 8192+1*ibGB, 1024 ); ioreq2.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq3, _osf, fReadIo, 8192+2*ibGB, 1024 ); ioreq3.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq4, _osf, fReadIo, 8192+3*ibGB, 1024 ); ioreq4.grbitQOS = qosIODispatchBackground;
    InitIoreq( &ioreq5, _osf, fReadIo, 8192+4*ibGB, 1024 ); ioreq5.grbitQOS = qosIODispatchImmediate;

    posd->EnqueueIORun( &ioreq1 );  CHECK( ioreq1.FEnqueuedInHeap() );
    posd->EnqueueIORun( &ioreq2 );  CHECK( ioreq2.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq3 );  CHECK( ioreq3.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq4 );  CHECK( ioreq4.FEnqueuedInMetedQ() );
    posd->EnqueueIORun( &ioreq5 );  CHECK( ioreq5.FEnqueuedInHeap() );

    CHECK( posd->CioAllEnqueued() == 5 );
    CHECK( posd->CioAllowedMetedOps( 5 ) == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 2 );
    CHECK( posd->CioUrgentEnqueued() == 2 );

    CHECK( posd->ErrDequeueIORun( &qop1 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 4 );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->ErrDequeueIORun( &qop2 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 3 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 2 );
    CHECK( posd->ErrDequeueIORun( &qop3 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 2 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 1 );
    CHECK( posd->ErrDequeueIORun( &qop4 ) == JET_errSuccess );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    FNegTestSet( fInvalidUsage );
    CHECK( posd->ErrDequeueIORun( &qop5 ) == errDiskTilt );
    FNegTestUnset( fInvalidUsage );
    CHECK( posd->CioAllEnqueued() == 1 );
    CHECK( posd->CioUrgentEnqueued() == 0 );
    CHECK( posd->CioReadyMetedEnqueued() == 0 );
    CHECK( posd->CioUrgentEnqueued() == 0 );

    IOREQ * pioreqT;

    pioreqT = qop1.PioreqGetRun();
    CHECK( &ioreq2 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop2.PioreqGetRun();
    CHECK( &ioreq3 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop3.PioreqGetRun();
    CHECK( &ioreq1 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );
    pioreqT = qop4.PioreqGetRun();
    CHECK( &ioreq5 == pioreqT );
    CHECK( posd->CioReadyMetedEnqueued() == 1 );
    posd->QueueCompleteIORun( pioreqT );
    CHECK( posd->ErrDequeueIORun( &qop5 ) == JET_errSuccess );
    pioreqT = qop5.PioreqGetRun();
    CHECK( &ioreq4 == pioreqT );
    posd->QueueCompleteIORun( pioreqT );

    TERM_OSDISK_QUEUE_JUNK;
    TERM_IOREQ_JUNK;
}


#pragma warning( pop )

