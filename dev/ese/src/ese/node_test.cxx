// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif // ENABLE_JET_UNIT_TEST

//  We have moved all globals / statics involving g_cbPage out of 
//  node_test, and new usage is variabla non-grata.
//
#undef g_cbPage
#define g_cbPage g_cbPage_CPAGE_NOT_ALLOWED_TO_USE_THIS_VARIABLE

//
//      Internal node function decls
//

//  Note: Apparently I can define local twice in two files! Why? Go look it up and find out. Scary.
LOCAL_BROKEN INT CdataNDIPrefixAndKeydataflagsToDataflags(
    KEYDATAFLAGS    * const pkdf,
    DATA            * const rgdata,
    INT             * const pfFlags,
    LE_KEYLEN       * const ple_keylen );


//
//      Helpers
//

inline BYTE ByteRandNoFF()
{
    return (BYTE) rand() % 255;
}

//  The page has 6 lines, all with ascending keys, and properly formed cbPrefix/cbSuffix.
//  The first 2 lines are guaranteed to be compressed.
//  The split moves up the prefix as we go through the page.

const INT g_clinesTest = 6;

void NDITestFillOutSimplePage( CPAGE& cpage )
{
    const INT clines = g_clinesTest;
    //  8-bytes of potential prefix ...
    BYTE rgbKey [] = { BYTE( ByteRandNoFF() % ( 0x20 - clines /* stay away from flags */ ) ), ByteRandNoFF(), ByteRandNoFF(), ByteRandNoFF(),
                        ByteRandNoFF(), ByteRandNoFF(), ByteRandNoFF(), ByteRandNoFF() };
    DATA data;
    data.SetPv( rgbKey );
    data.SetCb( sizeof(rgbKey) );
    cpage.SetExternalHeader( &data, 1, 0x0 );

    INT ibKeySplit = 5 + rand() % 3;

    for( INT iline = 0; iline < clines; iline++ )
    {
        if ( ibKeySplit <= 2 )  //  the key split has to have a prefix big enough to save space
        {
            ibKeySplit = 0;
        }

        KEYDATAFLAGS    kdf;
        kdf.Nullify();
        kdf.key.prefix.SetPv( rgbKey );
        kdf.key.prefix.SetCb( ibKeySplit );
        kdf.key.suffix.SetPv( rgbKey+ibKeySplit );
        kdf.key.suffix.SetCb( sizeof(rgbKey)-ibKeySplit );
        kdf.data.SetPv( rgbKey );
        kdf.data.SetCb( sizeof(rgbKey) );

        DATA            rgdata[5];
        INT             fFlagsLine;
        LE_KEYLEN       le_keylen;
        le_keylen.le_cbPrefix = (USHORT)ibKeySplit;
        const INT cdata = CdataNDIPrefixAndKeydataflagsToDataflags( &kdf, rgdata, &fFlagsLine, &le_keylen );
        cpage.Insert( iline, rgdata, cdata, fFlagsLine );

        BYTE cbDeltaMax = ibKeySplit ? ( (BYTE)0xff - rgbKey[ibKeySplit] ) : 2 /* to keep first byte out of flags */ ;
        rgbKey[ibKeySplit] += BYTE( rand() % cbDeltaMax );

        ibKeySplit = max( 0, ibKeySplit - rand() % 3 );
    }
}

//
//      Tests
//

JETUNITTEST ( Node, BasicNodePageChecks )
{
    const ULONG cbSmallPage = 4 * 1024;
    const ULONG cbLargePage = 32 * 1024;

    CPAGE cpageSmall;
    CPAGE cpageLarge;

    cpageSmall.LoadNewTestPage( cbSmallPage );
    cpageLarge.LoadNewTestPage( cbLargePage );

    NDITestFillOutSimplePage( cpageSmall );
    NDITestFillOutSimplePage( cpageLarge );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
}

class CpageDeallocator
{
private:
    CPAGE * m_pcpage1;
    CPAGE * m_pcpage2;

public:
    CpageDeallocator( CPAGE * const pcpage1, CPAGE * const pcpage2 )
    {
        m_pcpage1 = pcpage1;
        m_pcpage2 = pcpage2;
    }

    ~CpageDeallocator()
    {
        m_pcpage1->UnloadPage();
        m_pcpage2->UnloadPage();
    }
};

#define CreateSmallLargePage    \
            const ULONG cbSmallPage = 4 * 1024;             \
            const ULONG cbLargePage = 32 * 1024;            \
            CPAGE cpageSmall;                               \
            CPAGE cpageLarge;                               \
            cpageSmall.LoadNewTestPage( cbSmallPage );      \
            cpageLarge.LoadNewTestPage( cbLargePage );      \
            CpageDeallocator d( &cpageSmall, &cpageLarge ); \
            NDITestFillOutSimplePage( cpageSmall );         \
            NDITestFillOutSimplePage( cpageLarge );

// This is testing that the "test page" we construct has correct reserved and committed memory to allow us to AV if we accidentally
// index off the page (above or below) in like the Fuzz'ing tests far below.
JETUNITTEST ( Node, TestTestPageAvsAboveAndBelow )
{
    CreateSmallLargePage;
    
    ULONG cAccum = 0;

    BOOL fExcepted = fFalse;

    BYTE * pbPage = (BYTE*)cpageSmall.PvBuffer();
    fExcepted = fFalse;  __try {  cAccum += pbPage[ -1 ];               } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // under read AV
    fExcepted = fFalse;  __try {  cAccum += pbPage[ 0 ];                } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( !fExcepted );
    fExcepted = fFalse;  __try {  cAccum += pbPage[ cbSmallPage - 1 ];  } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( !fExcepted );
    fExcepted = fFalse;  __try {  cAccum += pbPage[ cbSmallPage ];      } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // over read AV

    // check way over and under - test page guarantees 64 KB of no-go zone.
    fExcepted = fFalse;  __try {  cAccum += pbPage[ -32 * 1024 ];               } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // way under read AV
    fExcepted = fFalse;  __try {  cAccum += pbPage[ cbSmallPage + 32 * 1024 ];  } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // way over read AV

    pbPage = (BYTE*)cpageLarge.PvBuffer();
    fExcepted = fFalse;  __try {  cAccum += pbPage[ -1 ];               } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // under read AV
    fExcepted = fFalse;  __try {  cAccum += pbPage[ 0 ];                } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( !fExcepted );
    fExcepted = fFalse;  __try {  cAccum += pbPage[ cbLargePage - 1 ];  } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( !fExcepted );
    fExcepted = fFalse;  __try {  cAccum += pbPage[ cbLargePage ];      } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // over read AV

    // check way over and under - test page guarantees 64 KB of no-go zone.
    fExcepted = fFalse;  __try {  cAccum += pbPage[ -32 * 1024 ];               } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // way under read AV
    fExcepted = fFalse;  __try {  cAccum += pbPage[ cbLargePage + 32 * 1024 ];  } __except( efaExecuteHandler ){ fExcepted = fTrue; }  CHECK( fExcepted );    // way over read AV

// I do not know this is safe, but I think it is.  I think it is because we're not letting any exceptions go, and we're not 
// creating any objects within the try clauses.
#pragma warning (suppress: 4509) // error C4509: nonstandard extension used: 'TestNodePrefixCorruptionMinor::Run_' uses SEH and 'cpageLarge' has destructor
}

#ifdef DEBUG

JETUNITTEST ( Node, PrefixCorruptionMinor )
{
    CreateSmallLargePage;

    //  lines 1 & 2 is guaranteed to have a prefix ...
    NDCorruptNodePrefixSize( cpageSmall, rand() % 2, JET_bitTestHookCorruptSizeLargerThanNode );
    NDCorruptNodePrefixSize( cpageLarge, rand() % 2, JET_bitTestHookCorruptSizeLargerThanNode );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Expected( fPreviouslySet == fFalse );

    CHECK( JET_errDatabaseCorrupted == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
    CHECK( JET_errDatabaseCorrupted == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );

    if ( !fPreviouslySet )
    {
        FNegTestUnset( fCorruptingPageLogically );
    }
}

JETUNITTEST ( Node, PrefixCorruptionShortWrap )
{
    CreateSmallLargePage;

    //  lines 1 & 2 is guaranteed to have a prefix ...
    NDCorruptNodePrefixSize( cpageSmall, rand() % 2, JET_bitTestHookCorruptSizeShortWrapLarge );
    NDCorruptNodePrefixSize( cpageLarge, rand() % 2, JET_bitTestHookCorruptSizeShortWrapLarge );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Expected( fPreviouslySet == fFalse );

    CHECK( JET_errDatabaseCorrupted == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
    CHECK( JET_errDatabaseCorrupted == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );

    if ( !fPreviouslySet )
    {
        FNegTestUnset( fCorruptingPageLogically );
    }
}

JETUNITTEST ( Node, SuffixCorruptionMinor )
{
    CreateSmallLargePage;

    NDCorruptNodeSuffixSize( cpageSmall, rand() % cpageSmall.Clines(), JET_bitTestHookCorruptSizeLargerThanNode );
    NDCorruptNodeSuffixSize( cpageLarge, rand() % cpageSmall.Clines(), JET_bitTestHookCorruptSizeLargerThanNode );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Expected( fPreviouslySet == fFalse );

    CHECK( JET_errDatabaseCorrupted == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
    CHECK( JET_errDatabaseCorrupted == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );

    if ( !fPreviouslySet )
    {
        FNegTestUnset( fCorruptingPageLogically );
    }
}

JETUNITTEST ( Node, SuffixCorruptionShortWrap )
{
    CreateSmallLargePage;

    NDCorruptNodeSuffixSize( cpageSmall, rand() % cpageSmall.Clines(), JET_bitTestHookCorruptSizeShortWrapLarge );
    NDCorruptNodeSuffixSize( cpageLarge, rand() % cpageSmall.Clines(), JET_bitTestHookCorruptSizeShortWrapLarge );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Expected( fPreviouslySet == fFalse );

    CHECK( JET_errDatabaseCorrupted == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
    CHECK( JET_errDatabaseCorrupted == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );

    if ( !fPreviouslySet )
    {
        FNegTestUnset( fCorruptingPageLogically );
    }
}

JETUNITTEST ( Node, PrefixSuffixCorruptionMinor )
{
    CreateSmallLargePage;

    const INT ilineCorrupted = rand() % 2;
    NDCorruptNodePrefixSize( cpageSmall, ilineCorrupted, JET_bitTestHookCorruptSizeLargerThanNode );
    NDCorruptNodePrefixSize( cpageLarge, ilineCorrupted, JET_bitTestHookCorruptSizeLargerThanNode );
    NDCorruptNodeSuffixSize( cpageSmall, ilineCorrupted, JET_bitTestHookCorruptSizeLargerThanNode );
    NDCorruptNodeSuffixSize( cpageLarge, ilineCorrupted, JET_bitTestHookCorruptSizeLargerThanNode );

    CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
    CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Expected( fPreviouslySet == fFalse );

    CHECK( JET_errDatabaseCorrupted == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );
    CHECK( JET_errDatabaseCorrupted == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) );

    if ( !fPreviouslySet )
    {
        FNegTestUnset( fCorruptingPageLogically );
    }
}

#endif // DEBUG

extern CCondition g_condGetKdfBadGetPtrNullPv;
extern CCondition g_condGetKdfBadGetPtrNonNullPv;
extern CCondition g_condGetKdfBadGetPtrOffPage;
extern CCondition g_condGetKdfBadGetPtrExtHdrOffPage;

JETUNITTEST ( Node, NDIGetKeydataflagsAgainstHighItagValue )
{
    CreateSmallLargePage;

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    const ULONG ctagsBad = 0xFFFC - g_clinesTest;

    cpageSmall.CorruptHdr( ipgfldCorruptItagMicFree, (USHORT)ctagsBad );
    cpageLarge.CorruptHdr( ipgfldCorruptItagMicFree, (USHORT)ctagsBad );

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    KEYDATAFLAGS kdf; kdf.Nullify();

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySet == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    ERR errGetKdf = JET_errSuccess;

    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrNullPv.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageSmall, 0xFFFB, &kdf, &errGetKdf );
    CHECK( errGetKdf == JET_errPageTagCorrupted );
    CHECK( g_condGetKdfBadGetPtrNullPv.FCheckHit() );

    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrNullPv.Reset();
    OnDebug( CHECK( g_condGetKdfBadGetPtrNullPv.FCheckNotHit() ) ); // This is a test that reset works, but it only does in DEBUG.
    NDIGetKeydataflags< pgnbcChecked >( cpageLarge, 0xFFFB, &kdf, &errGetKdf );
    CHECK( errGetKdf == JET_errPageTagCorrupted );
    CHECK( g_condGetKdfBadGetPtrNullPv.FCheckHit() );

    FNegTestUnset( fCorruptingPageLogically );
}

JETUNITTEST ( Node, NDIGetKeydataflagsAgainstTagIbOffPage )
{
    CreateSmallLargePage;

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    const ULONG itagTarget = 1 + rand() % ( g_clinesTest - 1 );

    CPAGE::TAG * ptagSmallPage = cpageSmall.PtagFromItag_( itagTarget );
    ULONG_PTR ibTargetSmall = cpageSmall.CbBuffer() - sizeof( CPAGE::PGHDR ) + 10;  // +10 to put node / line complete off the end of page.
    CHECK( ibTargetSmall < 0x10000 );

    CPAGE::TAG * ptagLargePage = cpageLarge.PtagFromItag_( itagTarget );
    ULONG_PTR ibTargetLarge = cpageLarge.CbBuffer() - sizeof( CPAGE::PGHDR2 ) + 10;  // +10 to put node / line complete off the end of page.
    CHECK( ibTargetLarge < 0x10000 );

    ptagSmallPage->SetIb( &cpageSmall, (USHORT)ibTargetSmall );
    ptagLargePage->SetIb( &cpageLarge, (USHORT)ibTargetLarge );

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    ERR errGetKdf = JET_errSuccess;
    KEYDATAFLAGS kdf; kdf.Nullify();

    g_condGetKdfBadGetPtrNullPv.Reset();
    g_condGetKdfBadGetPtrNonNullPv.Reset();

    //  Checking that pulling the itag above or below the target can be pulled without error.
    if ( itagTarget > 2 )
    {
        NDIGetKeydataflags< pgnbcChecked >( cpageSmall, itagTarget - 1 /* CPAGE::ctagReserved */ - 1, &kdf, &errGetKdf );
        OnDebug( CHECK( g_condGetKdfBadGetPtrNullPv.FCheckNotHit() ) );
        CHECK( errGetKdf == JET_errSuccess );
        kdf.Nullify();
    }
    if ( itagTarget < ( g_clinesTest - 3 ) )
    {
        NDIGetKeydataflags< pgnbcChecked >( cpageSmall, itagTarget - 1 /* CPAGE::ctagReserved */ + 1, &kdf, &errGetKdf );
        OnDebug( CHECK( g_condGetKdfBadGetPtrNullPv.FCheckNotHit() ) );
        CHECK( errGetKdf == JET_errSuccess );
        kdf.Nullify();
    }

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySet == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.


    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrNonNullPv.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageSmall, itagTarget - 1 /* CPAGE::ctagReserved */, &kdf, &errGetKdf );
    CHECK( errGetKdf == JET_errPageTagCorrupted );
    CHECK( g_condGetKdfBadGetPtrNonNullPv.FCheckHit() );

    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrNonNullPv.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageLarge, itagTarget - 1 /* CPAGE::ctagReserved */, &kdf, &errGetKdf );
    CHECK( errGetKdf == JET_errPageTagCorrupted );
    CHECK( g_condGetKdfBadGetPtrNonNullPv.FCheckHit() );

    FNegTestUnset( fCorruptingPageLogically );
}

JETUNITTEST ( Node, NDIGetKeydataflagsAgainstTagIbHalfOffPage )
{
    CreateSmallLargePage;

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    const ULONG itagTarget = 1 + rand() % ( g_clinesTest - 1 );

    CPAGE::TAG * ptagSmallPage = cpageSmall.PtagFromItag_( itagTarget );
    ULONG_PTR ibTargetSmall = cpageSmall.CbBuffer() - sizeof( CPAGE::PGHDR ) - 7;  // -7 to put node / line "half" off page.
    CHECK( ibTargetSmall < 0x10000 );

    CPAGE::TAG * ptagLargePage = cpageLarge.PtagFromItag_( itagTarget );
    ULONG_PTR ibTargetLarge = cpageLarge.CbBuffer() - sizeof( CPAGE::PGHDR2 ) - 7;  // -7 to put node / line "half" off page.
    CHECK( ibTargetLarge < 0x10000 );

    ERR errGetKdf = JET_errSuccess;

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySet == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    ptagSmallPage->SetIb( &cpageSmall, (USHORT)ibTargetSmall );
    ptagLargePage->SetIb( &cpageLarge, (USHORT)ibTargetLarge );

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    KEYDATAFLAGS kdf; kdf.Nullify();

    g_condGetKdfBadGetPtrNonNullPv.Reset();

    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrNonNullPv.Reset();
    g_condGetKdfBadGetPtrOffPage.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageSmall, itagTarget - 1 /* CPAGE::ctagReserved */, &kdf, &errGetKdf );
    CHECK( errGetKdf == JET_errPageTagCorrupted );
    CHECK( g_condGetKdfBadGetPtrNonNullPv.FCheckHit() );
    CHECK( g_condGetKdfBadGetPtrOffPage.FCheckNotHit() );


    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrNonNullPv.Reset();
    g_condGetKdfBadGetPtrOffPage.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageLarge, itagTarget - 1 /* CPAGE::ctagReserved */, &kdf, &errGetKdf );
    CHECK( errGetKdf == JET_errPageTagCorrupted );
    CHECK( g_condGetKdfBadGetPtrNonNullPv.FCheckHit() );
    CHECK( g_condGetKdfBadGetPtrOffPage.FCheckNotHit() );

    FNegTestUnset( fCorruptingPageLogically );
}

JETUNITTEST ( Node, NDIGetKeydataflagsAgainstExtHdrIbOffPage )
{
    CreateSmallLargePage;

    //cpageSmall.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageSmall.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageLarge.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );


    CPAGE::TAG * ptagSmallPage = cpageSmall.PtagFromItag_( 0 );
    ULONG_PTR ibTargetSmall = cpageSmall.CbBuffer() - sizeof( CPAGE::PGHDR ) + 10;  // +10 to put node / line complete off the end of page.
    CHECK( ibTargetSmall < 0x10000 );

    CPAGE::TAG * ptagLargePage = cpageLarge.PtagFromItag_( 0 );
    ULONG_PTR ibTargetLarge = cpageLarge.CbBuffer() - sizeof( CPAGE::PGHDR2 ) + 10;  // +10 to put node / line complete off the end of page.
    CHECK( ibTargetLarge < 0x10000 );

    ptagSmallPage->SetIb( &cpageSmall, (USHORT)ibTargetSmall );
    ptagLargePage->SetIb( &cpageLarge, (USHORT)ibTargetLarge );

    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );

    ERR errGetKdf = JET_errSuccess;
    KEYDATAFLAGS kdf; kdf.Nullify();

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySet == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    // I believe tag 1 always has some compression ... 
    kdf.Nullify();
    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrExtHdrOffPage.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageSmall, 1, &kdf, &errGetKdf );
    CHECK( FNDCompressed( kdf ) );
    OnDebug( CHECK( g_condGetKdfBadGetPtrExtHdrOffPage.FCheckNotHit() ) );

    kdf.Nullify();
    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrExtHdrOffPage.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageLarge, 1, &kdf, &errGetKdf );
    CHECK( FNDCompressed( kdf ) );
    OnDebug( CHECK( g_condGetKdfBadGetPtrExtHdrOffPage.FCheckNotHit() ) );

    FNegTestUnset( fCorruptingPageLogically );
}

JETUNITTEST ( Node, NDIGetKeydataflagsAgainstExtHdrIbOffPageCb0 )
{
    CreateSmallLargePage;

    //cpageSmall.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageSmall.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageLarge.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );


    CPAGE::TAG * ptagSmallPage = cpageSmall.PtagFromItag_( 0 );
    ULONG_PTR ibTargetSmall = cpageSmall.CbBuffer() - sizeof( CPAGE::PGHDR ) + 70;  // +10 to put node / line complete off the end of page.
    CHECK( ibTargetSmall < 0x10000 );

    CPAGE::TAG * ptagLargePage = cpageLarge.PtagFromItag_( 0 );
    ULONG_PTR ibTargetLarge = cpageLarge.CbBuffer() - sizeof( CPAGE::PGHDR2 ) + 70;  // +10 to put node / line complete off the end of page.
    CHECK( ibTargetLarge < 0x10000 );

    ptagSmallPage->SetIb( &cpageSmall, (USHORT)ibTargetSmall );
    ptagLargePage->SetIb( &cpageLarge, (USHORT)ibTargetLarge );

    ptagSmallPage->SetCb( &cpageSmall, 2 );
    ptagLargePage->SetCb( &cpageLarge, 2 );

    const bool fPreviouslySet = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySet == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    //cpageSmall.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageSmall.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageLarge.DumpHeader( CPRINTFSTDOUT::PcprintfInstance() );
    //cpageLarge.DumpTags( CPRINTFSTDOUT::PcprintfInstance() );

    ERR errGetKdf = JET_errSuccess;
    KEYDATAFLAGS kdf; kdf.Nullify();

    // I believe tag 1 always has some compression ... 
    kdf.Nullify();
    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrExtHdrOffPage.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageSmall, 1, &kdf, &errGetKdf );
    CHECK( FNDCompressed( kdf ) );
    OnDebug( CHECK( g_condGetKdfBadGetPtrExtHdrOffPage.FCheckNotHit() ) );

    kdf.Nullify();
    errGetKdf = JET_errSuccess;
    g_condGetKdfBadGetPtrExtHdrOffPage.Reset();
    NDIGetKeydataflags< pgnbcChecked >( cpageLarge, 1, &kdf, &errGetKdf );
    CHECK( FNDCompressed( kdf ) );
    OnDebug( CHECK( g_condGetKdfBadGetPtrExtHdrOffPage.FCheckNotHit() ) );

    FNegTestUnset( fCorruptingPageLogically );
}

//
//     Node Full Fuzz Tests
//

//  These tests are (at least beginning!) to fully fuzz all aspects of Cpage, prefix, suffix, and basic node layout corruptions 
//  to be safely handled in certain corruption resilient paths such as:  ErrCheckPage, GetPtr_, and NDIGetKeydataflags.
//
//  This also allows such corruptions to be caught (as in without an Enforce) within ErrCheckPage().  And by providing a bound 
//  against these corruptions at Read  IO, we can then know if they later Enforce() it is due to in-memory corruptions.
//
//  These tests are in Node instead of CPAGE because ErrCheckPage() has extensive checks at the node level (a layering violation 
//  admittedly) for key prefix, suffix, etc not overstepping the line size and so we want to test node-level corruptions, so we
//  need node level knowledge so the tests live here.

#define Pct( base, num )	( ((double) num) / ((double) base) * 100.0 )

void TestGetPtrAll( CPAGE * pcpage )
{
    LINE lineEH = { 0 };

    const ERR errEH = pcpage->ErrGetPtrExternalHeader( &lineEH );

    for( LONG iline = 0; iline < pcpage->Clines(); iline++ )
    {
        LINE lineN = { 0 };
        const ERR errGP = pcpage->ErrGetPtr( iline, &lineN );
    }
}

void TestGetKdfAll( CPAGE * pcpage )
{
    for( LONG iline = 0; iline < pcpage->Clines(); iline++ )
    {
        KEYDATAFLAGS kdf; kdf.Nullify();
        const ERR errGkdf = ErrNDIGetKeydataflags( *pcpage, iline, &kdf );
    }
}

#define FsecsTestTime()     ( (float)DtickDelta( tickStart, TickOSTimeCurrent() ) / 1000.0 )

#ifdef DEBUG


JETUNITTESTEX ( Node, TestCorruptPrefixCbFullFuzzResilientCodePaths, JetSimpleUnitTest::dwDontRunByDefault )
{
    const bool fPreviouslySetOuter = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySetOuter == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    TICK tickStart = TickOSTimeCurrent();
    CPRINTINTRINBUF prtbuf;
    CPRINTF * pcp = &prtbuf;

    ULONG cChecks = 0;
    ULONG cGoodSmall = 0;
    ULONG cGoodLarge = 0;

    ULONG cPrefixUsageIsLargerThanPrefixNode = 0;
    ULONG cSuffixSizeLargerThanTagSize = 0;
    ULONG cDataSizeLargerThanTagSize = 0;
    ULONG cDataSizeIsZero = 0;

    for( ULONG i = 1; i <= (ULONG)0xFFFF; i++ )
    {
        //  lines "1" & "2" is guaranteed to have a prefix ...
        for( ULONG iline = 0; iline <= 1; iline++ )
        {
            CreateSmallLargePage;
            //wprintf( L"\t  add offset:  %03d\n", i );

            NDCorruptNodePrefixSize( cpageSmall, iline, 0, (USHORT)i );
            NDCorruptNodePrefixSize( cpageLarge, iline, 0, (USHORT)i );

            //  Basic can't catch these corruptions.
            CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFNULL::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
            CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFNULL::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

            FNegTestSet( fCorruptingPageLogically );
            const ERR errSmall = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckDefault );
            const ERR errLarge = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckDefault );
            FNegTestUnset( fCorruptingPageLogically );

            if ( errSmall >= JET_errSuccess )
            {
                cGoodSmall++;
            }
            if ( errLarge >= JET_errSuccess )
            {
                cGoodLarge++;
            }

            cPrefixUsageIsLargerThanPrefixNode += prtbuf.CContains( "prefix usage is larger than prefix node (prefix.Cb()" );
            cSuffixSizeLargerThanTagSize += prtbuf.CContains( "suffix size is larger than the actual tag size (suffix.Cb()" );
            cDataSizeLargerThanTagSize += prtbuf.CContains( "data size is larger than actual tag size (data.Cb()" );
            cDataSizeIsZero += prtbuf.CContains( "data size is zero ... we don't have non-data nodes" );

            cChecks += 2; // for small & large page Extended / non-Basic checks only to measure success & failure rate.

            if ( cChecks != ( cGoodSmall + cGoodLarge + cPrefixUsageIsLargerThanPrefixNode + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanTagSize + cDataSizeIsZero ) )
            {
                prtbuf.Print( *CPRINTFSTDOUT::PcprintfInstance() );
                CHECK( fFalse ); // update the list of errors checked.
            }

            prtbuf.Reset();

            //  Test primitive functions

            TestGetPtrAll( &cpageSmall );
            TestGetPtrAll( &cpageLarge );

            FNegTestSet( fCorruptingPageLogically );

            TestGetKdfAll( &cpageSmall );
            TestGetKdfAll( &cpageLarge );

            FNegTestUnset( fCorruptingPageLogically );
        }
    }

    //  NOTE: These numbers are based upon random data generated in the nodes, however I ran 60k iterations 
    //  over 5 days and got:
    //    cGoodSmall(pages) min: 124  max: 162   cGoodLarge min: 158 max: 197
    //  ... so the numbers should be pretty stable but may increase by a few over time, but should not jump 
    //  largely. If they jump largely it is likely someone broke the strictness of the checks.
    wprintf( L"\n   cChecks = %u ... cGoodSmall = %u (%1.3f%%), cGoodLarge = %u (%1.3f%%) ... %1.3f secs\n", cChecks, cGoodSmall, Pct( cChecks, cGoodSmall ), cGoodLarge, Pct( cChecks, cGoodLarge ), FsecsTestTime() );
    wprintf( L"       Errors: %u ... %u ... %u ... %u --> %u \n", cPrefixUsageIsLargerThanPrefixNode, cSuffixSizeLargerThanTagSize, cDataSizeLargerThanTagSize, cDataSizeIsZero,
                ( cGoodSmall + cGoodLarge + cPrefixUsageIsLargerThanPrefixNode + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanTagSize + cDataSizeIsZero ) );
    CHECK( cGoodSmall <= 162 ); // generally fine if validation shrinks this value, should not regress / grow check.
    CHECK( cGoodLarge <= 197 ); // generally fine if validation shrinks this value, should not regress / grow check.

    // if these are offended, someone has made us stricter (that's good), but consider re-running this test infinitely 
    // to find the new max values for above checks.
    CHECK( cGoodSmall > 100 );  
    CHECK( cGoodSmall > 125 );

    // last modification is + 0x10000, which doesn't modify a USHORT, so should have min two successes.
    CHECK( cGoodSmall > 2 );
    CHECK( cGoodLarge > 2 );

    // Error counts are currently roughly 2 x this big, cut in half for margin
    CHECK( cPrefixUsageIsLargerThanPrefixNode >= 65000 );
    CHECK( cSuffixSizeLargerThanTagSize >= 32500 );
    // We do not count this one, because it doesn't happen every time ... requires right data at head of line.
    //CHECK( cDataSizeLargerThanTagSize >= 1 );
    CHECK( cDataSizeIsZero > 2 );

    CHECK( cChecks == ( cGoodSmall + cGoodLarge + cPrefixUsageIsLargerThanPrefixNode + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanTagSize + cDataSizeIsZero ) );
}

JETUNITTESTEX ( Node, TestCorruptSuffixCbFullFuzzResilientCodePaths, JetSimpleUnitTest::dwDontRunByDefault )
{
    const bool fPreviouslySetOuter = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySetOuter == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    TICK tickStart = TickOSTimeCurrent();
    CPRINTINTRINBUF prtbuf;
    CPRINTF * pcp = &prtbuf;

    ULONG cChecks = 0;
    ULONG cGoodSmall = 0;
    ULONG cGoodLarge = 0;

    ULONG cSuffixSizeLargerThanTagSize = 0;
    ULONG cDataSizeLargerThanActualTagSize = 0;
    ULONG cPrefixUsageIsLargerThanPrefixNode = 0;
    ULONG cDataSizeIsZero = 0;

    for( ULONG i = 1; i <= (ULONG)0xFFFF; i++ )
    {
        for( ULONG iline = 0; iline <= 5; iline++ )
        {
            CreateSmallLargePage;
            //wprintf( L"\t  add offset:  %03d\n", i );

            NDCorruptNodeSuffixSize( cpageSmall, iline, 0, (USHORT)i );
            NDCorruptNodeSuffixSize( cpageLarge, iline, 0, (USHORT)i );

            //  Basic can't catch these corruptions.
            CHECK( JET_errSuccess == cpageSmall.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );
            CHECK( JET_errSuccess == cpageLarge.ErrCheckPage( CPRINTFSTDOUT::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) );

            FNegTestSet( fCorruptingPageLogically );
            const ERR errSmall = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckDefault );
            const ERR errLarge = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckDefault );
            FNegTestUnset( fCorruptingPageLogically );

            if ( errSmall >= JET_errSuccess )
            {
                cGoodSmall++;
            }
            if ( errLarge >= JET_errSuccess )
            {
                cGoodLarge++;
            }

            cSuffixSizeLargerThanTagSize += prtbuf.CContains( "suffix size is larger than the actual tag size (suffix.Cb()" );
            cDataSizeLargerThanActualTagSize += prtbuf.CContains( "data size is larger than actual tag size (data.Cb()" );
            cPrefixUsageIsLargerThanPrefixNode += prtbuf.CContains( "prefix usage is larger than prefix node (prefix.Cb()" );
            cDataSizeIsZero += prtbuf.CContains( "data size is zero ... we don't have non-data nodes" );

            if ( !prtbuf.FContains( "suffix size is larger than the actual tag size (suffix.Cb()" ) &&
                 !prtbuf.FContains( "data size is larger than actual tag size (data.Cb()" ) &&
                 !prtbuf.FContains( "prefix usage is larger than prefix node (prefix.Cb()" ) &&
                 !prtbuf.FContains( "data size is zero ... we don't have non-data nodes" ) &&
                 ( errSmall < JET_errSuccess || errLarge < JET_errSuccess ) )
            {
                prtbuf.Print( *CPRINTFSTDOUT::PcprintfInstance() );
                CHECK( fFalse ); // update the list of errors checked.
            }

            cChecks += 2; // for small & large page Extended / non-Basic checks only to measure success & failure rate.

            prtbuf.Reset();

            //  Test primitive functions

            TestGetPtrAll( &cpageSmall );
            TestGetPtrAll( &cpageLarge );

            FNegTestSet( fCorruptingPageLogically );

            TestGetKdfAll( &cpageSmall );
            TestGetKdfAll( &cpageLarge );

            FNegTestUnset( fCorruptingPageLogically );
        }
    }

    //  NOTE: These numbers are based upon random data generated in the nodes, however I ran 19k iterations 
    //  over 5 days and got:
    //    cGoodSmall(pages) min: 620  max: 689   cGoodLarge min: 486 max: 566
    //  ... so the numbers should be pretty stable but may increase by a few over time, but should not jump 
    //  largely. If they jump largely it is likely someone broke the strictness of the checks.
    wprintf( L"\n   cChecks = %u ... cGoodSmall = %u (%1.3f%%), cGoodLarge = %u (%1.3f%%) ... %1.3f secs\n", cChecks, cGoodSmall, Pct( cChecks, cGoodSmall ), cGoodLarge, Pct( cChecks, cGoodLarge ), FsecsTestTime() );
    wprintf( L"       Errors: %d ... %d ... %d ... %d\n", cSuffixSizeLargerThanTagSize, cDataSizeLargerThanActualTagSize, cPrefixUsageIsLargerThanPrefixNode, cDataSizeIsZero );
    CHECK( cGoodSmall <= 689 ); // generally fine if validation shrinks this value, should not regress / grow check.
    CHECK( cGoodLarge <= 566 ); // generally fine if validation shrinks this value, should not regress / grow check.

    // if these are offended, someone has made us stricter (that's good), but consider re-running this test infinitely 
    // to find the new max values for above checks.
    CHECK( cGoodSmall > 550 );
    CHECK( cGoodSmall > 450 );

    // last modification is + 0x10000, which doesn't modify a USHORT, so should have min two successes.
    CHECK( cGoodSmall > 2 );
    CHECK( cGoodLarge > 2 );

    // Error counts are currently roughly 2 x this big, cut in half for margin
    CHECK( cSuffixSizeLargerThanTagSize >= 195000 );
    CHECK( cDataSizeLargerThanActualTagSize >= 125 );
    CHECK( cPrefixUsageIsLargerThanPrefixNode >= 250 );
    CHECK( cDataSizeIsZero > 10 ); // saw 92 - 100 ish

    CHECK( cChecks == ( cGoodSmall + cGoodLarge + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanActualTagSize + cPrefixUsageIsLargerThanPrefixNode + cDataSizeIsZero ) );
}

JETUNITTESTEX ( Node, TestCorruptItagMicFreeFullFuzzResilientCodePaths, JetSimpleUnitTest::dwDontRunByDefault )
{
    const bool fPreviouslySetOuter = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySetOuter == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    TICK tickStart = TickOSTimeCurrent();
    CPRINTINTRINBUF prtbuf;
    CPRINTF * pcp = &prtbuf;

    ULONG cChecks = 0;
    ULONG cGoodSmall = 0;
    ULONG cGoodLarge = 0;

    ULONG cTagHasZeroCb = 0;
    ULONG cTagArrTooLargeForRealData = 0;
    ULONG cItagMicFreeTooLarge = 0;
    ULONG cSpaceMismatch = 0;
    ULONG cZeroTagsWithoutEmptyPageFlag = 0;

    ULONG idivFullCheckRate = 100;
    ULONG iModTarget = TickOSTimeCurrent() * rand() % idivFullCheckRate;

    for( ULONG i = 1; i <= (ULONG)0xFFFF; i++ )
    {
        CreateSmallLargePage;
        //wprintf( L"\t  add offset:  %03d\n", i );

        cpageSmall.CorruptHdr( ipgfldCorruptItagMicFree, (USHORT)i );
        cpageLarge.CorruptHdr( ipgfldCorruptItagMicFree, (USHORT)i );

        ERR errSmall, errLarge;

        FNegTestSet( fCorruptingPageLogically );
        errSmall = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckBasics );
        errLarge = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckBasics );

        if ( errSmall >= JET_errSuccess )
        {
            cGoodSmall++;
        }
        if ( errLarge >= JET_errSuccess )
        {
            cGoodLarge++;
        }

        errSmall = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckDefault );
        errLarge = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckDefault );
        FNegTestUnset( fCorruptingPageLogically );

        if ( errSmall >= JET_errSuccess )
        {
            cGoodSmall++;
        }
        if ( errLarge >= JET_errSuccess )
        {
            cGoodLarge++;
        }
        cTagHasZeroCb += prtbuf.CContains( "page corruption (2): Non TAG-0 - TAG 7 has zero cb(cb = 0, ib = 0)" );
        cTagArrTooLargeForRealData += prtbuf.CContains( "page corruption (2): itagMicFree / tag array too large for real data left over" );
        cItagMicFreeTooLarge += prtbuf.CContains( "page corruption (2): itagMicFree too large" );
        cSpaceMismatch += prtbuf.CContains( "page corruption (2): space mismatch (" );
        if ( prtbuf.FContains( "Empty page without fPageEmpty or fPagePreInit" ) )
        {
            cZeroTagsWithoutEmptyPageFlag += prtbuf.CContains( "Empty page without fPageEmpty or fPagePreInit" );
        }
        if ( !prtbuf.FContains( "page corruption (2): Non TAG-0 - TAG 7 has zero cb(cb = 0, ib = 0)" ) &&
             !prtbuf.FContains( "page corruption (2): itagMicFree / tag array too large for real data left over" ) &&
             !prtbuf.FContains( "page corruption (2): itagMicFree too large" ) &&
             !prtbuf.FContains( "page corruption (2): space mismatch (" ) &&
             !prtbuf.FContains( "Empty page without fPageEmpty or fPagePreInit" ) &&
             ( errSmall < JET_errSuccess || errLarge < JET_errSuccess ) )
        {
             prtbuf.Print( *CPRINTFSTDOUT::PcprintfInstance() );
             CHECK( fFalse ); // update the list of errors checked.
        }


        cChecks += 4; // for small & large page both basic and default level.

        prtbuf.Reset();

        //  Test primitive functions

        FNegTestSet( fCorruptingPageLogically );

        if ( ( i % idivFullCheckRate ) == iModTarget )
        {
            // ErrCheckPage(), TestGetPtrAll(), and TestGetKdfAll() are all O(n) with number of nodes and this test constantly
            // adds nodes, making this N^2 ... so we do full test only 1 in 100 times.
            TestGetPtrAll( &cpageSmall );
            TestGetPtrAll( &cpageLarge );

            TestGetKdfAll( &cpageSmall );
            TestGetKdfAll( &cpageLarge );
        }
        else if ( cpageSmall.Clines() >= 2 )
        {
            CHECK( cpageSmall.Clines() == cpageLarge.Clines() );
            // and ever other time, we'll test last few nodes.
            for( INT iline = cpageSmall.Clines() - 2; iline <= cpageSmall.Clines() && iline <= 0xFFFF; iline++ )
            {
                LINE lineN = { 0 };
                ERR errGP = JET_errSuccess;
                errGP = cpageSmall.ErrGetPtr( iline, &lineN );

                memset( &lineN, 0, sizeof( lineN ) );
                errGP = cpageLarge.ErrGetPtr( iline, &lineN );

                KEYDATAFLAGS kdf; kdf.Nullify();
                ERR errGkdf;
                errGkdf = ErrNDIGetKeydataflags( cpageSmall, iline, &kdf );

                kdf.Nullify();
                errGkdf = ErrNDIGetKeydataflags( cpageLarge, iline, &kdf );
            }
        }

        FNegTestUnset( fCorruptingPageLogically );

    }

    wprintf( L"\n   cChecks = %u ... cGoodSmall = %u (%1.3f%%), cGoodLarge = %u (%1.3f%%) ... %1.3f secs\n", cChecks, cGoodSmall, Pct( cChecks, cGoodSmall ), cGoodLarge, Pct( cChecks, cGoodLarge ), FsecsTestTime() );
    wprintf( L"            Errors: %d ... %d ... %d ... %d ... %d \n", cTagHasZeroCb, cTagArrTooLargeForRealData, cItagMicFreeTooLarge, cSpaceMismatch, cZeroTagsWithoutEmptyPageFlag );

    //  It is impossible for modifying only the itagMicFree to come out consistent, b/c even reducing the 
    //  value by 1, means that with one less node that the cbFree will be too low.
    CHECK( cGoodSmall == 0 );
    CHECK( cGoodLarge == 0 );

    CHECK( cTagHasZeroCb == 9158 /* 65535 */ );
    CHECK( cTagArrTooLargeForRealData == 9182 /* 65535 */ );
    CHECK( cItagMicFreeTooLarge == 243772 /* 129044 */ );
    CHECK( cSpaceMismatch == 24 /* 65535 */ );
    CHECK( cZeroTagsWithoutEmptyPageFlag == 4 /* 2 */ );

    CHECK( cChecks == ( cGoodSmall + cGoodLarge + cTagHasZeroCb + cTagArrTooLargeForRealData + cItagMicFreeTooLarge + cSpaceMismatch + cZeroTagsWithoutEmptyPageFlag ) );
}

JETUNITTESTEX ( Node, TestCorruptTagIbFullFuzzResilientCodePaths, JetSimpleUnitTest::dwDontRunByDefault )
{
    const bool fPreviouslySetOuter = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySetOuter == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    TICK tickStart = TickOSTimeCurrent();
    CPRINTINTRINBUF prtbuf;
    CPRINTF * pcp = &prtbuf;

    ULONG cChecks = 0;
    ULONG cGoodSmall = 0;
    ULONG cGoodLarge = 0;

    ULONG cPrefixUsageIsLargerThanPrefixNode = 0;
    ULONG cTagEndsInFreeSpace = 0;
    ULONG cSuffixSizeLargerThanTagSize = 0;
    ULONG cDataSizeLargerThanTagSize = 0;
    ULONG cDataZeroOnNonTag0 = 0;

    for( ULONG i = 1; i <= (ULONG)0xFFFF; i++ )
    {
        const ULONG ctags = 1 /* ctagReserved */ + g_clinesTest;
        for( ULONG itag = 0; itag < ctags; itag++ )
        {
            CreateSmallLargePage;
            //wprintf( L"\t  add offset:  %03d\n", i );

            cpageSmall.CorruptTag( itag, fFalse, (USHORT)i );
            cpageLarge.CorruptTag( itag, fFalse, (USHORT)i );

            ERR errSmall, errLarge;

            errSmall = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckBasics );
            if ( errSmall >= JET_errSuccess )
            {
                cGoodSmall++;
            }

            errLarge = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckBasics );
            if ( errLarge >= JET_errSuccess )
            {
                cGoodLarge++;
            }

            FNegTestSet( fCorruptingPageLogically );

            errSmall = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag /* don't even need CheckTagsNonOverlapping from CheckDefault */ );
            if ( errSmall >= JET_errSuccess )
            {
                cGoodSmall++;
            }

            errLarge = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag /* don't even need CheckTagsNonOverlapping from CheckDefault */ );
            if ( errLarge >= JET_errSuccess )
            {
                cGoodLarge++;
            }

            FNegTestUnset( fCorruptingPageLogically );

            //(*pcp)( "Before: %d ... %d ... %d ... %d\n", cPrefixUsageIsLargerThanPrefixNode, cSuffixSizeLargerThanTagSize, cTagEndsInFreeSpace, cDataSizeLargerThanTagSize );

            cPrefixUsageIsLargerThanPrefixNode += prtbuf.CContains( "prefix usage is larger than prefix node (prefix.Cb()" );
            cSuffixSizeLargerThanTagSize += prtbuf.CContains( "suffix size is larger than the actual tag size (suffix.Cb()" );
            cTagEndsInFreeSpace += prtbuf.CContains( "ends in free space (cb =" );
            cDataSizeLargerThanTagSize += prtbuf.CContains( "data size is larger than actual tag size (data.Cb()" );
            cDataZeroOnNonTag0 += prtbuf.CContains( "data size is zero ... we don't have non-data nodes ... yet at least. (data.Cb() = " );

            cChecks += 4; // for small & large page both basic and default

            if ( cChecks != ( cGoodSmall + cGoodLarge + cPrefixUsageIsLargerThanPrefixNode + cSuffixSizeLargerThanTagSize + cTagEndsInFreeSpace + cDataSizeLargerThanTagSize + cDataZeroOnNonTag0 ) )
            {
                wprintf (L" Checks got out of sync with bad errors tracked - successes: %d / %d\n", cGoodSmall, cGoodLarge );
                prtbuf.Print( *CPRINTFSTDOUT::PcprintfInstance() );
                CHECK( fFalse );
            }

            prtbuf.Reset();

            //  Test primitive functions

            FNegTestSet( fCorruptingPageLogically );

            TestGetPtrAll( &cpageSmall );
            TestGetPtrAll( &cpageLarge );

            TestGetKdfAll( &cpageSmall );
            TestGetKdfAll( &cpageLarge );

            FNegTestUnset( fCorruptingPageLogically );
        }
    }

    //  NOTE: These numbers are based upon random data generated in the nodes, however I ran 18k iterations 
    //  over 5 days and got:
    //    cGoodSmall(pages) min: 6172  max: 6238   cGoodLarge min: 1558 max: 1610
    //  ... so the numbers should be pretty stable but may increase by a few over time, but should not jump 
    //  largely. If they jump largely it is likely someone broke the strictness of the checks.
    wprintf( L"\n   cChecks = %u ... cGoodSmall = %u (%1.3f%%), cGoodLarge = %u (%1.3f%%) ... %1.3f secs\n", cChecks, cGoodSmall, Pct( cChecks, cGoodSmall ), cGoodLarge, Pct( cChecks, cGoodLarge ), FsecsTestTime() );
    wprintf( L"            Errors: %u ... %u ... %u ... %u ... %u --> %u\n", cPrefixUsageIsLargerThanPrefixNode, cSuffixSizeLargerThanTagSize, cTagEndsInFreeSpace, cDataSizeLargerThanTagSize, cDataZeroOnNonTag0,
                 ( cGoodSmall + cGoodLarge + cPrefixUsageIsLargerThanPrefixNode + cSuffixSizeLargerThanTagSize + cTagEndsInFreeSpace + cDataSizeLargerThanTagSize + cDataZeroOnNonTag0 ) );

    CHECK( cGoodSmall <= 6400 ); // generally fine if validation shrinks this value, should not regress / grow check.
    CHECK( cGoodLarge <= 1650 ); // generally fine if validation shrinks this value, should not regress / grow check.

    // If these are offended, someone has made us stricter (that's good), but consider re-running this test infinitely 
    // to find the new max & min values for this range of checks above and below this comment.
    CHECK( cGoodSmall > 6100 );  
    CHECK( cGoodSmall > 1500 );

    // last modification is + 0x10000, which doesn't modify a USHORT, so should have a min two successes - no matter what.
    CHECK( cGoodSmall > 2 );
    CHECK( cGoodLarge > 2 );

    // Error counts are currently roughly 2 x this big, cut in half for margin
    CHECK( cPrefixUsageIsLargerThanPrefixNode >= 1000 );
    CHECK( cSuffixSizeLargerThanTagSize >= 1100 );
    CHECK( cTagEndsInFreeSpace >= 455000 );

    // cDataSizeLargerThanTagSize is sometimes 0, sometimes 1 - 6 area ... didn't see hire, but can't assert a min b/c of 0.
    CHECK( cDataSizeLargerThanTagSize <= 10 );

    // Sometimes is 0 ... can't assert on this.
    //CHECK( cDataZeroOnNonTag0 >= 2 );

    CHECK( cChecks == ( cGoodSmall + cGoodLarge + cPrefixUsageIsLargerThanPrefixNode + cSuffixSizeLargerThanTagSize + cTagEndsInFreeSpace + cDataSizeLargerThanTagSize + cDataZeroOnNonTag0 ) );
}

JETUNITTESTEX ( Node, TestCorruptTagCbFullFuzzResilientCodePaths, JetSimpleUnitTest::dwDontRunByDefault )
{
    const bool fPreviouslySetOuter = FNegTestSet( fCorruptingPageLogically );
    Assert( fPreviouslySetOuter == fFalse ); // or need to not call FNegTestUnste(), or was leaked by prev test.

    TICK tickStart = TickOSTimeCurrent();
    CPRINTINTRINBUF prtbuf;
    CPRINTF * pcp = &prtbuf;

    ULONG cChecks = 0;
    ULONG cGoodSmall = 0;
    ULONG cGoodLarge = 0;

    ULONG cSpaceMismatch = 0;
    ULONG cTagEndsInFreeSpace = 0;
    ULONG cTagHasZeroCb = 0;
    ULONG cSuffixSizeLargerThanTagSize = 0;
    ULONG cDataSizeLargerThanTagSize = 0;
    ULONG cPrefixUsageIsLargerThanPrefixNode = 0;
    ULONG cDataZeroOnNonTag0 = 0;

    for( ULONG i = 1; i <= (ULONG)0xFFFF; i++ )
    {
        const ULONG ctags = 1 /* ctagReserved */ + g_clinesTest;
        for( ULONG itag = 0; itag < ctags; itag++ )
        {
            CreateSmallLargePage;
            //wprintf( L"\t  add offset:  %03d\n", i );

            cpageSmall.CorruptTag( itag, fTrue, (USHORT)i );
            cpageLarge.CorruptTag( itag, fTrue, (USHORT)i );

            ERR errT;

            errT = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckBasics );
            if ( errT >= JET_errSuccess )
            {
                cGoodSmall++;
            }

            errT = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckBasics );
            if ( errT >= JET_errSuccess )
            {
                cGoodLarge++;
            }

            FNegTestSet( fCorruptingPageLogically );

            errT = cpageSmall.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag /* don't even need CheckTagsNonOverlapping from CheckDefault */ );
            if ( errT >= JET_errSuccess )
            {
                cGoodSmall++;
            }

            errT = cpageLarge.ErrCheckPage( pcp, CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag /* don't even need CheckTagsNonOverlapping from CheckDefault */ );
            if ( errT >= JET_errSuccess )
            {
                cGoodLarge++;
            }

            FNegTestUnset( fCorruptingPageLogically );

            cSpaceMismatch += prtbuf.CContains( "page corruption (2): space mismatch (" );
            cTagEndsInFreeSpace += prtbuf.CContains( "ends in free space (cb =" );
            const ULONG cT = prtbuf.CContains( "page corruption (2): Non TAG-0 - TAG" ) + prtbuf.CContains( "has zero cb(cb =" );
            cTagHasZeroCb += cT ? ( cT / 2 ) : 0;
            Assert( cT % 2 == 0 );
            cSuffixSizeLargerThanTagSize += prtbuf.CContains( "suffix size is larger than the actual tag size (suffix.Cb()" );
            cDataSizeLargerThanTagSize += prtbuf.CContains( "data size is larger than actual tag size (data.Cb()" );
            cPrefixUsageIsLargerThanPrefixNode += prtbuf.CContains( "prefix usage is larger than prefix node (prefix.Cb()" );
            cDataZeroOnNonTag0 += prtbuf.CContains( "data size is zero ... we don't have non-data nodes ... yet at least. (data.Cb() = " );


            cChecks += 4; // for small & large page both basic and default

            if ( cChecks != ( cGoodSmall + cGoodLarge + cSpaceMismatch + cTagEndsInFreeSpace + cTagHasZeroCb + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanTagSize + cPrefixUsageIsLargerThanPrefixNode + cDataZeroOnNonTag0 ) )
            {
                prtbuf.Print( *CPRINTFSTDOUT::PcprintfInstance() );
                CHECK( fFalse ); // update the list of errors checked.
            }

            //  Test primitive functions

            FNegTestSet( fCorruptingPageLogically );

            TestGetPtrAll( &cpageSmall );
            TestGetPtrAll( &cpageLarge );

            TestGetKdfAll( &cpageSmall );
            TestGetKdfAll( &cpageLarge );

            FNegTestUnset( fCorruptingPageLogically );

            prtbuf.Reset();
        }
    }

    //  NOTE: These numbers are based upon random data generated in the nodes, however I ran 18k iterations 
    //  over 5 days and got:
    //    cGoodSmall(pages) min: 49 max: 49   cGoodLarge min: 7 max: 7 ... note doubled below for basic & extensive checks.
    //  ... so the numbers had absolutely no variance over all runs.  So if they move, hopefully it is down,
    //  and so the cpage check page got stricter.  If it is moved up, it is a dubious change.
    wprintf( L"\n   cChecks = %u ... cGoodSmall = %u (%1.3f%%), cGoodLarge = %u (%1.3f%%) ... %1.3f secs\n", cChecks, cGoodSmall, Pct( cChecks, cGoodSmall ), cGoodLarge, Pct( cChecks, cGoodLarge ), FsecsTestTime() );
    wprintf( L"            Errors: %d ... %d ... %d ... %d ... %d ... %d ... %d ==> %d\n", 
                                 cSpaceMismatch, cTagEndsInFreeSpace, cTagHasZeroCb, cSuffixSizeLargerThanTagSize, cDataSizeLargerThanTagSize, cPrefixUsageIsLargerThanPrefixNode, cDataZeroOnNonTag0,
                               ( cSpaceMismatch + cTagEndsInFreeSpace + cTagHasZeroCb + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanTagSize + cPrefixUsageIsLargerThanPrefixNode + cDataZeroOnNonTag0 ) );

    CHECK( cGoodSmall == 98 ); // generally fine if validation shrinks this value, should not regress / grow check.
    CHECK( cGoodLarge == 14 ); // generally fine if validation shrinks this value, should not regress / grow check.

    // last modification is + 0x10000, which doesn't modify a USHORT, so should have min two successes.
    CHECK( cGoodSmall > 2 );
    CHECK( cGoodLarge > 2 );

    // Error counts are currently roughly 2 x this big, cut in half for margin
    CHECK( cSpaceMismatch >= 3500 );
    CHECK( cTagEndsInFreeSpace >= 457000 );
    CHECK( cTagHasZeroCb >= 45 );
    CHECK( cSuffixSizeLargerThanTagSize >= 202 );
    CHECK( cDataSizeLargerThanTagSize >= 130 ); // 195
    CHECK( cPrefixUsageIsLargerThanPrefixNode >= 48 );
    CHECK( cDataZeroOnNonTag0 >= 30 );

    CHECK( cChecks == ( cGoodSmall + cGoodLarge + cSpaceMismatch + cTagEndsInFreeSpace + cTagHasZeroCb + cSuffixSizeLargerThanTagSize + cDataSizeLargerThanTagSize + cPrefixUsageIsLargerThanPrefixNode + cDataZeroOnNonTag0 ) );
}

#endif  //  DEBUG

//  The above tests all focus on fuzzing exactly one datem of the page, the following test picks two random datum from 
//  a pre-selected list of items and randomly corrupts both to try to find corruptions that happen when more than one
//  aspect of the page is corrupted - such as is the case if the page has been completely trashed with random garbage
//  or pre-decrypted data.

#define ubMax   (0xFF)
#define usMax   (0xFFFF)

class CRandomizedPageCorruptor
{
    PAGECHECKSUM    m_xchkInitial;
    USHORT          m_rgibSaved[5];
    BYTE            m_rgbSaved[5];
    ULONG           m_cibInterestingOffsetsMac;
    USHORT          m_rgibInterestingOffsets[ 200 ];

    void *          m_pvPageInitial;

    void AddOffset_( USHORT ib )
    {
        Expected( ib < 32 * 1024 );
        Assert( m_cibInterestingOffsetsMac < _countof( m_rgibInterestingOffsets ) );
        m_rgibInterestingOffsets[ m_cibInterestingOffsetsMac ] = ib;
        m_cibInterestingOffsetsMac++;
    }

    void AddOffsetRange_( USHORT ibStart, USHORT cb )
    {
        for ( USHORT ibAddl = 0; ibAddl < cb; ibAddl++ )
        {
            AddOffset_( ibStart + ibAddl );
        }
    }

    #define AddField( s, f )    AddOffsetRange_( OffsetOf( s, f ), (USHORT)sizeof( ((s*)0)->f ) );

    BYTE * PbOffset_( _In_ const CPAGE * const pcpage, _In_ const USHORT ib )
    {
        ULONG_PTR pb = (ULONG_PTR)pcpage->PvBuffer();
        return (BYTE*)( pb + ib );
    }

    const static BOOL s_fPrintOffsets = fTrue;

public:
    CRandomizedPageCorruptor( _In_ const CPAGE * const pcpage )
    {
        m_cibInterestingOffsetsMac = 0;
        memset( m_rgibInterestingOffsets, 0, sizeof( m_rgibInterestingOffsets ) );

        memset( m_rgibSaved, 0xFF, sizeof( m_rgibSaved ) );
        memset( m_rgbSaved, 0x00, sizeof( m_rgbSaved ) );

        m_xchkInitial = pcpage->LoggedDataChecksum();

        const ULONG cbPage = pcpage->CbPage();
        const ULONG ctags = pcpage->Clines();

        wprintf( L"\t Setting up %d KB page, with clines = %d\n", cbPage / 1024, ctags );

        m_pvPageInitial = malloc( cbPage );
        memcpy( m_pvPageInitial, pcpage->PvBuffer(), pcpage->CbPage() );

        wprintf( L"\t Tracking Offset target ib(s):  " );

        // Corrupt some parts of CPAGE::PGHDR
        AddField( CPAGE::PGHDR, cbFree );
        AddField( CPAGE::PGHDR, cbUncommittedFree );
        AddField( CPAGE::PGHDR, ibMicFree );
        AddField( CPAGE::PGHDR, itagState );
        AddField( CPAGE::PGHDR, fFlags );


        for( ULONG ib = cbPage - 1; ib >= ( cbPage - ctags * 4 /* sizeof( TAG ) */ ); ib-- )
        {
            AddOffset_( (USHORT)ib );
        }

        ULONG_PTR pbPage = (ULONG_PTR)pcpage->PvBuffer();
        for( ULONG itag = 0; itag < ctags; itag++ )
        {
            LINE line;
            itag == 0 ? pcpage->ErrGetPtrExternalHeader( &line ) : pcpage->ErrGetPtr( itag - 1, &line );;

            // 4 covers both potential key cb counts and a little more.
            AddOffsetRange_( (USHORT)( (ULONG_PTR)line.pv - pbPage ), 4 );

            // select one random offset from rest of line.
            ULONG ibRand = 6 + ( rand() % ( line.cb - 6 ) );
            AddOffset_( (USHORT)( (ULONG_PTR)line.pv - pbPage ) + (USHORT)ibRand );
        }

        for( ULONG iib = 0; iib < m_cibInterestingOffsetsMac; iib++ )
        {
            wprintf( L"%d, ", m_rgibInterestingOffsets[ iib ] );
        }
        wprintf( L"\n" );
    }

    void CorruptIt( _In_ const ULONG cCorruptions, _In_ DWORD grbitUnused, _Inout_ CPAGE * const pcpage )
    {
        Expected( pcpage->LoggedDataChecksum() == m_xchkInitial ); // if this is not honored, RestoreIt() _may_ not work.

        Assert( m_rgibSaved[ 0 ] == usMax );

        for ( ULONG iCorruption = 0; iCorruption < cCorruptions; iCorruption++ )
        {
            Assert( iCorruption < _countof( m_rgibSaved ) - 1 /* -1 b/c we use last entry staying as usMax / 0xFFFF as sentinel */ );
            Assert( m_rgibSaved[ iCorruption ] == usMax );

            //  Select target for demolotion
            m_rgibSaved[ iCorruption ] = m_rgibInterestingOffsets[ rand() % m_cibInterestingOffsetsMac ];
            for( ULONG j = 0; j < iCorruption; j++ )
            {
                if ( m_rgibSaved[ iCorruption ] == m_rgibSaved[ j ] )
                {
                    m_rgibSaved[ iCorruption ] = usMax;
                    break;
                }
            }
            if ( m_rgibSaved[ iCorruption ] == usMax )
            {
                iCorruption--; // try again
                continue;
            }

            //  Backup & Demolish.
            m_rgbSaved[ iCorruption ] = *PbOffset_( pcpage, m_rgibSaved[ iCorruption ] );
            *PbOffset_( pcpage, m_rgibSaved[ iCorruption ] ) = rand() % ubMax;

            //wprintf( L"\t\t Corrupt[ %d ].%d (%d --> %d)\n", iCorruption, m_rgibSaved[ iCorruption ], m_rgbSaved[ iCorruption ], *PbOffset_( pcpage, m_rgibSaved[ iCorruption ] ) );
        }
    }

    void RestoreIt( _Inout_ CPAGE * pcpage )
    {
        for( ULONG iCorruption = 0; m_rgibSaved[ iCorruption ] != usMax; iCorruption++ )
        {
            *PbOffset_( pcpage, m_rgibSaved[ iCorruption ] ) = m_rgbSaved[ iCorruption ];
            //wprintf( L"\t\t Uncorrupt[ %d ].%d (. --> %d)\n", iCorruption, m_rgibSaved[ iCorruption ], *PbOffset_( pcpage, m_rgibSaved[ iCorruption ] ) );
            m_rgibSaved[ iCorruption ] = usMax;
        }

        OnDebug( const PAGECHECKSUM chkAfterAfter = pcpage->LoggedDataChecksum() );
	Assert( chkAfterAfter == m_xchkInitial );

    }

};

//  This test runs in retail, so we know it all still works ...

JETUNITTEST ( Node, TestCorruptRandishFuzzingStress )
{
    CreateSmallLargePage;


    CRandomizedPageCorruptor  corrSmall( &cpageSmall );
    CRandomizedPageCorruptor  corrLarge( &cpageLarge );

    TICK tickStart = TickOSTimeCurrent();
    QWORD cChecks = 0, cBasicBadSmallPages = 0, cBasicBadLargePages = 0, cBoundedBadSmallPages = 0, cBoundedBadLargePages = 0, cJunk = 0;

    //const QWORD cLimit = ( 64LL * 1024 * 1024 * 1024 ); // exhaustive iteration
    const QWORD cLimit = 1024 * 1024;                     // how many we run normally
    //const QWORD cLimit = 15;                            // super quick check

    const QWORD cStatusPrint = cLimit > 100000 ? 100000 : cLimit - 1;

    for( QWORD i = 0; DtickDelta( tickStart, TickOSTimeCurrent() ) < 10 * 1000 && i < cLimit; i++ )
    {
        if ( cLimit < 20 )
        {
            wprintf( L"\t Iter[%I64d] - \n", i );
        }

        FNegTestSet( fCorruptingPageLogically );

        corrSmall.CorruptIt( 1 + i % 4, 0x0, &cpageSmall );

        ( cpageSmall.ErrCheckPage( CPRINTFNULL::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) < JET_errSuccess ) ?
            cBasicBadSmallPages++ :
            cJunk++;
        ( cpageSmall.ErrCheckPage( CPRINTFNULL::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) < JET_errSuccess ) ?
            cBoundedBadSmallPages++ :
            cJunk++;

        FNegTestUnset( fCorruptingPageLogically );

        FNegTestSet( fCorruptingPageLogically );

        corrLarge.CorruptIt( 1 + i % 4, 0x0, &cpageLarge );

        ( cpageLarge.ErrCheckPage( CPRINTFNULL::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckBasics ) < JET_errSuccess ) ?
            cBasicBadLargePages++ :
            cJunk++;
        ( cpageLarge.ErrCheckPage( CPRINTFNULL::PcprintfInstance(), CPAGE::OnErrorReturnError, CPAGE::CheckLineBoundedByTag ) < JET_errSuccess ) ?
            cBoundedBadLargePages++ :
            cJunk++;

        cChecks += 4;

        //  Test primitive functions

        TestGetPtrAll( &cpageSmall );
        TestGetPtrAll( &cpageLarge );

        TestGetKdfAll( &cpageSmall );
        TestGetKdfAll( &cpageLarge );

        FNegTestUnset( fCorruptingPageLogically );

        corrSmall.RestoreIt( &cpageSmall );
        corrLarge.RestoreIt( &cpageLarge );

        if ( i % cStatusPrint == ( cStatusPrint - 1 ) )
        {
            wprintf( L" [%I64d]   Small: %I64d - %I64d (%2.2f%%) / %I64dd (%2.2f%%)   Large: %I64d - %I64d (%2.2f%%) / %I64d (%2.2f%%)\n",
                     i / cStatusPrint,
                     cChecks / 2, 
                     cBasicBadSmallPages, (double)cBasicBadSmallPages / ( (double)cChecks / 4.0 ) * 100.0,
                     cBoundedBadSmallPages, (double)cBoundedBadSmallPages / ( (double)cChecks / 4.0 ) * 100.0,
                     cChecks / 2, 
                     cBasicBadLargePages, (double)cBasicBadLargePages / ( (double)cChecks / 4.0 ) * 100.0,
                     cBoundedBadLargePages, (double)cBoundedBadLargePages / ( (double)cChecks / 4.0 ) * 100.0
                   );
        }
    }
}

INT INDIAddReservedTag( _In_ CPAGE& cpage, _In_ NodeResvTagId resvTagId, _In_ int cb, _In_ BYTE fill );
VOID NDIReplaceReservedTag( _In_ CPAGE& cpage, _In_ NodeResvTagId resvTagId, const DATA& data );

JETUNITTEST( Node, TestAddDuplicateReservedTag )
{
    CreateSmallLargePage;

    INT itag = INDIAddReservedTag( cpageSmall, (NodeResvTagId) 1, 100, 0 );
    CHECK( itag > 0 );

    // Should enforce
    bool fEnforce = false;
    __try
    {
        itag = INDIAddReservedTag( cpageSmall, (NodeResvTagId) 1, 101, 1 );
    }
    __except ( JetTestEnforceSEHException::Filter( GetExceptionInformation() ) )
    {
        fEnforce = true;
        JetTestEnforceSEHException::Cleanup();
    }

    CHECK( fEnforce );
}

JETUNITTEST( Node, TestAddReservedTagOverflow )
{
    CreateSmallLargePage;

    for ( int i = 1; i < CPAGE::PGHDR::CTAG_RESERVED_MAX; i++ )
    {
        INT itag = INDIAddReservedTag( cpageSmall, (NodeResvTagId) i, 100, 0 );
        CHECK( itag > 0 );
    }

    // Should enforce
    bool fEnforce = false;
    FNegTestSet( fInvalidAPIUsage );

    __try
    {
        INDIAddReservedTag( cpageSmall, (NodeResvTagId) 30, 100, 0 );
    }
    __except ( JetTestEnforceSEHException::Filter( GetExceptionInformation() ) )
    {
        fEnforce = true;
        JetTestEnforceSEHException::Cleanup();
    }

    FNegTestUnset( fInvalidAPIUsage );
    CHECK( fEnforce );
}

JETUNITTEST( Node, TestReplaceMissingReservedTag )
{
    CreateSmallLargePage;

    // Should enforce
    bool fEnforce = false;
    __try
    {
        DATA data;
        data.Nullify();
        NDIReplaceReservedTag( cpageSmall, (NodeResvTagId) 1, data );
    }
    __except ( JetTestEnforceSEHException::Filter( GetExceptionInformation() ) )
    {
        fEnforce = true;
        JetTestEnforceSEHException::Cleanup();
    }

    CHECK( fEnforce );
}
