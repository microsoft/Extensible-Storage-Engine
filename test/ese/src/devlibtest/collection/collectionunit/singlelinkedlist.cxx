// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

class SingleLinkedListTest : public UNITTEST
{
    private:
        static SingleLinkedListTest s_instance;

    protected:
        SingleLinkedListTest() {}
    public:
        ~SingleLinkedListTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

SingleLinkedListTest SingleLinkedListTest::s_instance;

const char * SingleLinkedListTest::SzName() const       { return "Single Linked List"; };
const char * SingleLinkedListTest::SzDescription() const    { return 
            "Tests the collection libraries' single linked list facility.";
        }
bool SingleLinkedListTest::FRunUnderESE98() const       { return true; }
bool SingleLinkedListTest::FRunUnderESENT() const       { return true; }
bool SingleLinkedListTest::FRunUnderESE97() const       { return true; }


typedef struct _tagRandomStruct {

    ULONG                       ulSig1;
    struct _tagRandomStruct *   pNext;
    ULONG                       ulSig2;

} RandomStruct;

#ifndef OffsetOf
#define OffsetOf(s,m)   ((SIZE_T)&(((s *)0)->m))
#endif


ERR SingleLinkedListTest::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting single linked list ...\n");


    RandomStruct rs1 = { 0x10000, NULL, 0x10000 };
    RandomStruct rs2 = { 0x20000, NULL, 0x20000 };

    CLocklessLinkedList< RandomStruct > Head;
    CLocklessLinkedList< RandomStruct > Head2;

    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( rs2.ulSig1 == rs2.ulSig2 );
    TestCheck( rs1.ulSig1 != rs2.ulSig1 );


    TestCheck( Head.FEmpty() );
    TestCheck( NULL == Head.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( NULL == Head.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    

    void * pdata = (void*)0xBAADF00D;
    C_ASSERT( sizeof(pdata) == sizeof(CLocklessLinkedList< RandomStruct >) );
    CLocklessLinkedList< RandomStruct > * pHeadCtorTest = (CLocklessLinkedList< RandomStruct > *)&pdata;
    new( pHeadCtorTest) CLocklessLinkedList< RandomStruct >;

    TestCheck( (void*)0xBAADF00D != pdata );

    TestCheck( pHeadCtorTest->FEmpty() );
    TestCheck( NULL == pHeadCtorTest->RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );


    Head.AtomicInsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );

    TestCheck( rs1.ulSig1 == rs1.ulSig2 );

    RandomStruct * prs = Head.RemovePrevMost( OffsetOf( RandomStruct, pNext ) );

    TestCheck( &rs1 == prs );
    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( NULL == Head.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );


    Head.AtomicInsertAsPrevMost( &rs2, OffsetOf( RandomStruct, pNext ) );
    Head.AtomicInsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );

    TestCheck( !Head.FEmpty() );


    prs = Head.RemovePrevMost( OffsetOf( RandomStruct, pNext ) );

    TestCheck( &rs1 == prs );
    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( NULL == prs->pNext );
    TestCheck( !Head.FEmpty() );

    
    prs = Head.RemovePrevMost( OffsetOf( RandomStruct, pNext ) );

    TestCheck( &rs2 == prs );
    TestCheck( rs2.ulSig1 == rs2.ulSig2 );
    TestCheck( NULL == prs->pNext );
    TestCheck( Head.FEmpty() );


    Head.AtomicInsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );
    Head.AtomicInsertAsPrevMost( &rs2, OffsetOf( RandomStruct, pNext ) );

    TestCheck( !Head.FEmpty() );
    TestCheck( Head2.FEmpty() );

    Head2 = Head.AtomicRemoveList();

    TestCheck( !Head2.FEmpty() );
    TestCheck( Head.FEmpty() );

#ifdef DEBUG
    TestCheck( &rs2 == Head2.Head() );
#endif
    TestCheck( &rs2 == Head2.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( &rs1 == Head2.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    

HandleError:

    return err;
}


