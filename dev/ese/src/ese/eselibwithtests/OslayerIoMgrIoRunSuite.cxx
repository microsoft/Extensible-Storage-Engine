// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadRunBasicProperties )
#else
JETUNITTEST( IoRunQop, TestReadRunBasicProperties )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    CHECK( iorun.FEmpty() );

    InitIoreq( &ioreq1, _osfA, fReadIo, 2048, 1024 );
    CHECK( iorun.FAddToRun( &ioreq1 ) );

    CHECK( !iorun.FEmpty() );
    OnDebug( CHECK( iorun.Cioreq() == 1 ) );
    OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
    CHECK( iorun.FWrite() == fFalse );
#ifndef IoRunTQopSuite
    CHECK( iorun.P_OSF() == &_osfA );
    CHECK( iorun.IbOffset() == 2048 );
    CHECK( iorun.IbRunMax() == 3072 );
    CHECK( iorun.FMultiBlock() == fFalse );
#endif
    CHECK( iorun.CbRun() == 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestWriteRunBasicProperties )
#else
JETUNITTEST( IoRunQop, TestWriteRunBasicProperties )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfB, fWriteIo, 65536, 4096 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );

    CHECK( !iorun.FEmpty() );
    OnDebug( CHECK( iorun.Cioreq() == 1 ) );
    OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
    CHECK( iorun.FWrite() == fTrue ); // FWrite() returns 2 if empty (and asserts).
#ifndef IoRunTQopSuite
    CHECK( iorun.P_OSF() == &_osfB );
    CHECK( iorun.IbOffset() == 65536 );
    CHECK( iorun.IbRunMax() == 65536 + 4096 );
    CHECK( iorun.FMultiBlock() == fFalse );
#endif
    CHECK( iorun.CbRun() == 4096 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningInOrder )
#else
JETUNITTEST( IoRunQop, TestReadCombiningInOrder )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 1024, 1024 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1024 + 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );

    CHECK( !iorun.FEmpty() );
    CHECK( iorun.FWrite() == fFalse );
    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 8192 + 3 * 1024 );
    CHECK( iorun.FMultiBlock() );
#endif
    CHECK( iorun.CbRun() == 3 * 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestWriteCombiningInOrder )
#else
JETUNITTEST( IoRunQop, TestWriteCombiningInOrder )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fWriteIo, 4096, 2048 );
    InitIoreq( &ioreq2, _osfA, fWriteIo, 4096 + 1 * 2048, 2048 );
    InitIoreq( &ioreq3, _osfA, fWriteIo, 4096 + 2 * 2048, 2048 );
    InitIoreq( &ioreq4, _osfA, fWriteIo, 4096 + 3 * 2048, 2048 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );
    CHECK( iorun.FAddToRun( &ioreq4 ) );

    CHECK( !iorun.FEmpty() );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 4096 );
    CHECK( iorun.IbRunMax() == 4096 + 3 * 2048 + 2048 );
    CHECK( iorun.FMultiBlock() );
#endif
    CHECK( iorun.CbRun() == 8192 /* 4 x 2 KB IOs */ );
    OnDebug( CHECK( iorun.Cioreq() == 4 ) );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCantGapFillOverlappingBlockOffsets )
#else
JETUNITTEST( IoRunQop, TestReadCantGapFillOverlappingBlockOffsets )
#endif
{
    INIT_IOREQ_JUNK;

    for( ULONG fOneIoreq = fFalse; fOneIoreq < 2; fOneIoreq++ )
    {
        IORunT iorun;

        if ( fOneIoreq )
        {
            InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 4096 );
            CHECK( iorun.FAddToRun( &ioreq1 ) );
            OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
        }
        else
        {
            InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 + 512 );
            InitIoreq( &ioreq2, _osfA, fReadIo, 10240 + 512, 1024 + 512 );
            CHECK( iorun.FAddToRun( &ioreq1 ) );
            CHECK( iorun.FAddToRun( &ioreq2 ) );
            OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
            OnDebug( CHECK( iorun.FRunMember( &ioreq2 ) ) );
        }

        CHECK( !iorun.FEmpty() );
        CHECK( iorun.CbRun() == 4096 );
        OnDebug( const ULONG cioreqBefore = iorun.Cioreq() );

        //  uncombinable IOREQ(s)
        //                                                                             First Test IO      |--------------|
        //                                                                            Second Test IO      |----|    |----|
        InitIoreq( &ioreq3, _osfA, fReadIo, 7168, 2048 );   CHECK( !iorun.FAddToRun( &ioreq3 ) );  // |------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 8192, 1024 );   CHECK( !iorun.FAddToRun( &ioreq3 ) );  //     |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 9216, 1024 );   CHECK( !iorun.FAddToRun( &ioreq3 ) );  //         |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10240, 1024 );  CHECK( !iorun.FAddToRun( &ioreq3 ) );  //             |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10240, 2048 );  CHECK( !iorun.FAddToRun( &ioreq3 ) );  //             |------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10240, 4096 );  CHECK( !iorun.FAddToRun( &ioreq3 ) );  //             |--------------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 7168, 13312 );  CHECK( !iorun.FAddToRun( &ioreq3 ) );  // |----------------------| (totally subsuming)

        OnDebug( CHECK( !iorun.FRunMember( &ioreq3 ) ) );

        //  Nothing should've changed from above ...
        CHECK( !iorun.FEmpty() );
        OnDebug( CHECK( iorun.Cioreq() == cioreqBefore ) );
        CHECK( iorun.FWrite() == fFalse );
#ifndef IoRunTQopSuite
        CHECK( iorun.IbOffset() == 8192 );
#endif
        CHECK( iorun.CbRun() == 4096 );
    }

}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestWriteCantOverlappingBlockOffsets )
#else
JETUNITTEST( IoRunQop, TestWriteCantOverlappingBlockOffsets )
#endif
{
    INIT_IOREQ_JUNK;

    for( ULONG fOneIoreq = fFalse; fOneIoreq < 2; fOneIoreq++ )
    {
        IORunT iorun;

        if ( fOneIoreq )
        {
            InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 4096 );
            CHECK( iorun.FAddToRun( &ioreq1 ) );
            OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
        }
        else
        {
            InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 2048 );
            InitIoreq( &ioreq2, _osfA, fReadIo, 10240, 2048 );
            CHECK( iorun.FAddToRun( &ioreq1 ) );
            CHECK( iorun.FAddToRun( &ioreq2 ) );
            OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
            OnDebug( CHECK( iorun.FRunMember( &ioreq2 ) ) );
        }

        CHECK( !iorun.FEmpty() );
        CHECK( iorun.CbRun() == 4096 );
        OnDebug( const ULONG cioreqBefore = iorun.Cioreq() );

        FNegTestSet( fInvalidUsage );
        //  uncombinable IOREQ(s)
        //                                                                               First Test IO      |--------------|
        //                                                                              Second Test IO      |------||------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 7168, 2048 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  // |------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 8192, 1024 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //      |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 8704, 1024 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //        |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 9216, 1024 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //          |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10240, 1024 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //             |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10240, 2048 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //             |------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10752, 1024 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //               |--|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10752, 2048 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //               |------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 10240, 4096 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  //             |--------------|
        InitIoreq( &ioreq3, _osfA, fReadIo, 7168, 13312 );    CHECK( !iorun.FAddToRun( &ioreq3 ) );  // |----------------------| (totally subsuming)

        FNegTestUnset( fInvalidUsage );

        OnDebug( CHECK( !iorun.FRunMember( &ioreq3 ) ) );

        //  Nothing should've changed from above ...
        CHECK( !iorun.FEmpty() );
        OnDebug( CHECK( iorun.Cioreq() == cioreqBefore ) );
        CHECK( iorun.FWrite() == fFalse );
#ifndef IoRunTQopSuite
        CHECK( iorun.IbOffset() == 8192 );
#endif
        CHECK( iorun.CbRun() == 4096 );
    }
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadAndWriteCombiningRejectDifferentFiles )
#else
JETUNITTEST( IoRunQop, TestReadAndWriteCombiningRejectDifferentFiles )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfB, fReadIo, 8192 + 1024, 1024 );

    InitIoreq( &ioreq3, _osfA, fWriteIo, 8192, 1024 );
    InitIoreq( &ioreq4, _osfB, fWriteIo, 8192 + 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    Postls()->fIOThread = fTrue; // only IO thread is supposed to do this, asserts otherwise
    CHECK( !iorun.FAddToRun( &ioreq2 ) );
    Postls()->fIOThread = fFalse;
    CHECK( iorun.CbRun() == 1024 );

    //  empties the iorun
    (void)iorun.PioreqGetRun();
    CHECK( iorun.FEmpty() );

    CHECK( iorun.FAddToRun( &ioreq3 ) );
    Postls()->fIOThread = fTrue; // only IO thread is supposed to do this, asserts otherwise
    CHECK( !iorun.FAddToRun( &ioreq4 ) );
    Postls()->fIOThread = fFalse;
    CHECK( iorun.CbRun() == 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningHonorsMaxIoLimits )
#else
JETUNITTEST( IoRunQop, TestReadCombiningHonorsMaxIoLimits )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 1 * 1024, 62 * 1024 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 63 * 1024, 1024 );

    //  uncombinable IOREQ(s) - note IO limits set at top of file in INIT_IOREQ_JUNK
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 64 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );
    CHECK( iorun.CbRun() == 64 * 1024 );

    CHECK( !iorun.FAddToRun( &ioreq4 ) );

    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
    CHECK( iorun.CbRun() == 64 * 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningAllowsReadGapping )
#else
JETUNITTEST( IoRunQop, TestReadCombiningAllowsReadGapping )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 5 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );

    OnDebug( CHECK( iorun.Cioreq() == 2 ) );
    CHECK( iorun.CbRun() == 6 * 1024 );

#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 8192 + 5 * 1024 + 1024 );
    CHECK( iorun.FMultiBlock() );
#endif
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningReadGappingStillHonorsMaxIoLimits )
#else
JETUNITTEST( IoRunQop, TestReadCombiningReadGappingStillHonorsMaxIoLimits )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    //InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 1 * 1024, 62 * 1024 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 63 * 1024, 1024 );

    //  uncombinable IOREQ(s)
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 64 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    //CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );
    CHECK( iorun.CbRun() == 64 * 1024 );

    CHECK( !iorun.FAddToRun( &ioreq4 ) );
    OnDebug( CHECK( !iorun.FRunMember( &ioreq4 ) ) );
    OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );  // should still be in run unaffected
    OnDebug( CHECK( iorun.FRunMember( &ioreq3 ) ) );

    OnDebug( CHECK( iorun.Cioreq() == 2 ) );
    CHECK( iorun.CbRun() == 64 * 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningReadGappingAllowsReadFillingNonContiguous )
#else
JETUNITTEST( IoRunQop, TestReadCombiningReadGappingAllowsReadFillingNonContiguous )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 7 * 1024, 1024 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 4 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );

    OnDebug( CHECK( iorun.FRunMember( &ioreq1 ) ) );
    OnDebug( CHECK( iorun.FRunMember( &ioreq2 ) ) );  // should still be in run unaffected
    OnDebug( CHECK( iorun.FRunMember( &ioreq3 ) ) );
    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
    CHECK( iorun.CbRun() == 8 * 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningReadGappingAllowsReadFillingContiguousFit )
#else
JETUNITTEST( IoRunQop, TestReadCombiningReadGappingAllowsReadFillingContiguousFit )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    //  Test contiguous filling (i.e. a perfect fit gap filling)
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 2 * 1024, 1024 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );

    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
    CHECK( iorun.CbRun() == 3 * 1024 );

    (void)iorun.PioreqGetRun(); // empties the iorun
    Assert( iorun.FEmpty() );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningReadGappingAllowsReadFillingWithUnevenBlocks )
#else
JETUNITTEST( IoRunQop, TestReadCombiningReadGappingAllowsReadFillingWithUnevenBlocks )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    //  Test contiguous filling of un-even sizes just for giggles
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 3 * 1024, 1024 + 512 );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1 * 1024, 2 * 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );

    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
    CHECK( iorun.CbRun() == 4 * 1024 + 512 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningReadGappingReadFillingCantOverlap )
#else
JETUNITTEST( IoRunQop, TestReadCombiningReadGappingReadFillingCantOverlap )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    //  Test contiguous filling (i.e. a perfect fit gap filling)
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 2 * 1024, 1024 );

    //  illegal IOREQ(s)
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1 * 1024, 1024 + 512 ); // the 512 overlaps ioreq2,

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    FNegTestSet( fInvalidUsage );
    CHECK( !iorun.FAddToRun( &ioreq3 ) );
    FNegTestUnset( fInvalidUsage );

    OnDebug( CHECK( iorun.Cioreq() == 2 ) );
    CHECK( iorun.CbRun() == 3 * 1024 );
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningTwoExactInterleavedIoruns )
#else
JETUNITTEST( IoRunQop, TestReadCombiningTwoExactInterleavedIoruns )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;
    IORunT iorunAdd;

    //  Prepare 1st run
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );            CHECK( iorun.FAddToRun( &ioreq1 ) );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 2 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq2 ) );

    //  2nd Run ...
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq3 ) );
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 3 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq4 ) );

    CHECK( iorun.FAddToRun( iorunAdd.PioreqGetRun() ) );

    OnDebug( CHECK( iorun.Cioreq() == 4 ) );
    CHECK( iorun.CbRun() == 4 * 1024 );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 12288 );
    CHECK( iorun.FMultiBlock() );
#endif
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningTwoExactInterleavedIorunsReversed )
#else
JETUNITTEST( IoRunQop, TestReadCombiningTwoExactInterleavedIorunsReversed )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;
    IORunT iorunAdd;

    //  Prepare 1st run
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq3 ) );
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 3 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq4 ) );

    //  2nd Run ...
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );            CHECK( iorun.FAddToRun( &ioreq1 ) );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 2 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq2 ) );

    CHECK( iorun.FAddToRun( iorunAdd.PioreqGetRun() ) );

    OnDebug( CHECK( iorun.Cioreq() == 4 ) );
    CHECK( iorun.CbRun() == 4 * 1024 );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 12288 );
    CHECK( iorun.FMultiBlock() );
#endif
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningIntoLongerInterleavedIoruns )
#else
JETUNITTEST( IoRunQop, TestReadCombiningIntoLongerInterleavedIoruns )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;
    IORunT iorunAdd;

    //  Prepare 1st run
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );            CHECK( iorun.FAddToRun( &ioreq1 ) );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 2 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq2 ) );
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 4 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq3 ) );

    //  2nd Run ...
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 1 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq4 ) );
    InitIoreq( &ioreq5, _osfA, fReadIo, 8192 + 3 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq5 ) );

    CHECK( iorun.FAddToRun( iorunAdd.PioreqGetRun() ) );

    OnDebug( CHECK( iorun.Cioreq() == 5 ) );
    CHECK( iorun.CbRun() == 5 * 1024 );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 13312 );
    CHECK( iorun.FMultiBlock() );
#endif
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningExpandingInterleavingIoruns )
#else
JETUNITTEST( IoRunQop, TestReadCombiningExpandingInterleavingIoruns )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;
    IORunT iorunAdd;

    //  Prepare 1st run
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192 + 1 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq1 ) );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 3 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq2 ) );

    //  2nd Run ...
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192, 1024 );            CHECK( iorunAdd.FAddToRun( &ioreq3 ) );
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 2 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq4 ) );
    InitIoreq( &ioreq5, _osfA, fReadIo, 8192 + 4 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq5 ) );

    CHECK( iorun.FAddToRun( iorunAdd.PioreqGetRun() ) );

    OnDebug( CHECK( iorun.Cioreq() == 5 ) );
    CHECK( iorun.CbRun() == 5 * 1024 );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 13312 );
    CHECK( iorun.FMultiBlock() );
#endif
}

#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestReadCombiningTwoIorunsWithGaps )
#else
JETUNITTEST( IoRunQop, TestReadCombiningTwoIorunsWithGaps )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;
    IORunT iorunAdd;

    //  Prepare 1st run
    InitIoreq( &ioreq1, _osfA, fReadIo, 8192, 1024 );            CHECK( iorun.FAddToRun( &ioreq1 ) );
    InitIoreq( &ioreq2, _osfA, fReadIo, 8192 + 3 * 1024, 1024 ); CHECK( iorun.FAddToRun( &ioreq2 ) );

    //  2nd Run ...
    InitIoreq( &ioreq3, _osfA, fReadIo, 8192 + 1 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq3 ) );
    InitIoreq( &ioreq4, _osfA, fReadIo, 8192 + 4 * 1024, 1024 ); CHECK( iorunAdd.FAddToRun( &ioreq4 ) );

    CHECK( iorun.FAddToRun( iorunAdd.PioreqGetRun() ) );

    OnDebug( CHECK( iorun.Cioreq() == 4 ) );
    CHECK( iorun.CbRun() == 5 * 1024 );
#ifndef IoRunTQopSuite
    CHECK( iorun.IbOffset() == 8192 );
    CHECK( iorun.IbRunMax() == 13312 );
    CHECK( iorun.FMultiBlock() );
#endif
}


#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestWriteCombiningHonorsMaxIoLimits )
#else
JETUNITTEST( IoRunQop, TestWriteCombiningHonorsMaxIoLimits )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;

    InitIoreq( &ioreq1, _osfA, fWriteIo, 8192, 1024 );
    InitIoreq( &ioreq2, _osfA, fWriteIo, 8192 + 1 * 1024, 62 * 1024 );
    InitIoreq( &ioreq3, _osfA, fWriteIo, 8192 + 63 * 1024, 1024 );

    //  uncombinable IOREQ(s)
    InitIoreq( &ioreq4, _osfA, fWriteIo, 8192 + 64 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );
    CHECK( iorun.FAddToRun( &ioreq2 ) );
    CHECK( iorun.FAddToRun( &ioreq3 ) );
    CHECK( iorun.CbRun() == 64 * 1024 );

    CHECK( !iorun.FAddToRun( &ioreq4 ) );
    OnDebug( CHECK( !iorun.FRunMember( &ioreq4 ) ) );
    OnDebug( CHECK( iorun.FRunMember( &ioreq2 ) ) );

    OnDebug( CHECK( iorun.Cioreq() == 3 ) );
    CHECK( iorun.CbRun() == 64 * 1024 );
}

// note: but clean page overwrite / write gapping is done at higher (BF layer) in ESE.
#ifndef IoRunTQopSuite
JETUNITTEST( IoRunNative, TestWriteCombiningCantSupportGapping )
#else
JETUNITTEST( IoRunQop, TestWriteCombiningCantSupportGapping )
#endif
{
    INIT_IOREQ_JUNK;
    IORunT iorun;


    InitIoreq( &ioreq1, _osfA, fWriteIo, 8192, 1024 );

    //  uncombinable IOREQ(s)
    InitIoreq( &ioreq2, _osfA, fWriteIo, 8192 + 3 * 1024, 1024 );

    CHECK( iorun.FAddToRun( &ioreq1 ) );

    CHECK( !iorun.FAddToRun( &ioreq2 ) );

    OnDebug( CHECK( iorun.Cioreq() == 1 ) );
    OnDebug( CHECK( !iorun.FRunMember( &ioreq2 ) ) );
    CHECK( iorun.CbRun() == 1024 );
}

