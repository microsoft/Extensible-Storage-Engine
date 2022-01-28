// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "IterQueryUnitTest.hxx"

#include "stat.hxx"
#include "iterquery.hxx"


struct TestPageData
{
    ULONG   iSeq;
    BOOL    fCached;
    QWORD   qwChksum;
};

class TestPageDataEntryDescriptor : public IEntryDescriptor
{
    ULONG CbEntry( PcvEntry pcvEntry ) const   { return sizeof( TestPageData ); }
    const CHAR * SzDefaultPrintList() const    { return "print:iSeq,qwChksum"; }

    void DumpEntry( QwEntryAddr qwAddr, PcvEntry pcvEntry ) const
    {
        printf( "Entry Dump: %p / %p \n", qwAddr, pcvEntry );
    }

    const CMemberDescriptor * PmdMatch( const char * szTarget, const CHAR chDelim = '\0' ) const
    {
        static CMemberDescriptor s_rgqt[] =
        {
            QEF(   TestPageData, iSeq,       eNoHistoSupport,   DwordExprEval, NULL, DwordReadVal, DwordPrintVal )
            QEF(   TestPageData, fCached,    eNoHistoSupport,   BoolExprEval,  NULL, BoolReadVal, BoolPrintVal )
            QEF(   TestPageData, qwChksum,   eNoHistoSupport,   QwordExprEval, NULL, QwordReadVal, QwordPrintVal )
        };

        return PmdMemberDescriptorLookupHelper( s_rgqt, _countof( s_rgqt ), szTarget, chDelim );
    }

    ULONG CMembers() const
    {
        return 1 + ULONG( PmdMatch( SzIQLastMember ) - PmdMatch( SzIQFirstMember ) );
    }

};

TestPageDataEntryDescriptor g_iedTestPageData;

TestPageData rgs[] = {
    { 1,  10 },
    { 2,  20 },
    { 3,  30 },
    { 4,  40 },
    { 5,  50 },
};


//  ================================================================
CUnitTest( TestIterQueryAbstractTypesCanManageEntryTypes, 0, "See class name." );
ERR TestIterQueryAbstractTypesCanManageEntryTypes::ErrTest()
//  ================================================================
{
    //    Testing basic type usage and various struct casts works / compiles.
    //

    PvEntry pve = &rgs[1];
    PcvEntry pcve = &rgs[1];
    QwEntryAddr a1 = rgs[1].qwChksum;
    QwEntryAddr a2 = (QwEntryAddr)&rgs[1];

    BYTE * pb = (BYTE *)pve;
    BYTE * pb2 = (BYTE *)pcve;

    wprintf( L"\nTEST_DONE\n\n" );

    return JET_errSuccess;
}

//  ================================================================
CUnitTest( TestAddressFromSzArgProcessCanProcessWeirdDebuggerPointerSyntax, 0, "See class name." );
ERR TestAddressFromSzArgProcessCanProcessWeirdDebuggerPointerSyntax::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    QWORD qw;
    CHAR * rgszArgs[] = { "0x0000017b`d118a000" };

    TestAssert( 1 == FITQUAddressFromSz( 1, (const CHAR** const)rgszArgs, 0, &qw ) );
    TestAssert( qw == 0x0000017bd118a000 );

    wprintf( L"\nTEST_DONE\n\n" );

    return err;
}


//  ================================================================
CUnitTest( TestEntryDescriptorCanMatchRequestedMembersByName, 0, "See class name." );
ERR TestEntryDescriptorCanMatchRequestedMembersByName::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    IEntryDescriptor * pied = &g_iedTestPageData;

    TestAssert( pied->CMembers() == 3 );

    TestAssert( 0 == strcmp( "iSeq", pied->PmdMatch( "iSeq", 0 )->szMember ) );
    TestAssert( 0 == strcmp( "qwChksum", pied->PmdMatch( "qwChksum", 0 )->szMember ) );

    wprintf( L"\nTEST_DONE\n\n" );

    return err;
}

//  ================================================================
CUnitTest( TestBasicIteratorQueryCanFindTwoEntriesWithSeqMembersGreaterThanThree, 0, "See class name." );
ERR TestBasicIteratorQueryCanFindTwoEntriesWithSeqMembersGreaterThanThree::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;    

    CIterQuery * piq;

    __int64 c = 0;
    CHAR * rgsz [] = { "iSeq", ">", "3", "count" };

    TestCallS( ErrIQCreateIterQueryCount( &g_iedTestPageData, _countof( rgsz ), rgsz, (void*)&c, &piq ) );

    for( ULONG i = 0; i < _countof( rgs ); i++ )
    {
        if ( piq->FEvaluatePredicate( i, &rgs[i] ) )
        {
            piq->PerformAction( i, &rgs[i] );
        }
    }

    TestAssert( c == 2 );

    wprintf( L"\nTEST_DONE\n\n" );

HandleError:

    return err;
}
