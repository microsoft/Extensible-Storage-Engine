// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

class SimpleQueueListTest : public UNITTEST
{
    private:
        static SimpleQueueListTest s_instance;

    protected:
        SimpleQueueListTest() {}
    public:
        ~SimpleQueueListTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

SimpleQueueListTest SimpleQueueListTest::s_instance;

const char * SimpleQueueListTest::SzName() const        { return "Simple Queue List"; };
const char * SimpleQueueListTest::SzDescription() const { return 
            "Tests the collection libraries' simple queue list facility.";
        }
bool SimpleQueueListTest::FRunUnderESE98() const        { return true; }
bool SimpleQueueListTest::FRunUnderESENT() const        { return true; }
bool SimpleQueueListTest::FRunUnderESE97() const        { return true; }


typedef struct _tagRandomStruct {

    ULONG                       ulSig1;
    struct _tagRandomStruct *   pNext;
    ULONG                       ulSig2;

} RandomStruct;

#ifndef OffsetOf
#define OffsetOf(s,m)   ((SIZE_T)&(((s *)0)->m))
#endif


ERR SimpleQueueListTest::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting simple queue list ...\n");


    RandomStruct rs1 = { 0x10000, NULL, 0x10000 };
    RandomStruct rs2 = { 0x20000, NULL, 0x20000 };
    RandomStruct rs3 = { 0x30000, NULL, 0x30000 };

    CSimpleQueue< RandomStruct > Queue;
    CSimpleQueue< RandomStruct > Queue2;
    CLocklessLinkedList< RandomStruct > List;

    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( rs2.ulSig1 == rs2.ulSig2 );
    TestCheck( rs1.ulSig1 != rs2.ulSig1 );


    TestCheck( Queue.FEmpty() );
    TestCheck( 0 == Queue.CElements() );
    TestCheck( NULL == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( NULL == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( 0 == Queue.CElements() );
    

    void * pdata [3] = { (void*)0xBAADF00D, (void*)0xF00DBAAD, (void*)0xF0BAAD0D  };
    C_ASSERT( sizeof(pdata) == sizeof(CSimpleQueue< RandomStruct >) );
    CSimpleQueue< RandomStruct > * pHeadCtorTest = (CSimpleQueue< RandomStruct > *)&pdata;
    new( pHeadCtorTest) CSimpleQueue< RandomStruct >;

    TestCheck( (void*)0xBAADF00D != pdata[0] );
    TestCheck( (void*)0xF00DBAAD != pdata[1] );
    TestCheck( (void*)0xF0BAAD0D != pdata[2] );

    TestCheck( pHeadCtorTest->FEmpty() );
    TestCheck( 0 == Queue.CElements() );
    TestCheck( NULL == pHeadCtorTest->RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );


    Queue.InsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );

    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( 1 == Queue.CElements() );

    RandomStruct * prs = Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) );

    TestCheck( &rs1 == prs );
    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( 0 == Queue.CElements() );
    TestCheck( NULL == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( 0 == Queue.CElements() );


    Queue.InsertAsPrevMost( &rs2, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );

    TestCheck( !Queue.FEmpty() );
    TestCheck( 2 == Queue.CElements() );


    prs = Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) );

    TestCheck( &rs1 == prs );
    TestCheck( rs1.ulSig1 == rs1.ulSig2 );
    TestCheck( NULL == prs->pNext );
    TestCheck( !Queue.FEmpty() );
    TestCheck( 1 == Queue.CElements() );

    
    prs = Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) );

    TestCheck( &rs2 == prs );
    TestCheck( rs2.ulSig1 == rs2.ulSig2 );
    TestCheck( NULL == prs->pNext );
    TestCheck( Queue.FEmpty() );


    Queue.InsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsPrevMost( &rs2, OffsetOf( RandomStruct, pNext ) );

    TestCheck( !Queue.FEmpty() );
    TestCheck( Queue2.FEmpty() );


    List = Queue.RemoveList();
    TestCheck( Queue.FEmpty() );
    TestCheck( 0 == Queue.CElements() );

    TestCheck( &rs2 == List.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( &rs1 == List.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( List.FEmpty() );
    TestCheck( 0 == Queue.CElements() );


    TestCheck( Queue.FEmpty() );    
    Queue.InsertAsNextMost( &rs3, OffsetOf( RandomStruct, pNext ) );
    TestCheck( !Queue.FEmpty() );
    TestCheck( 1 == Queue.CElements() );
    TestCheck( &rs3 == Queue.Head() );
    TestCheck( 1 == Queue.CElements() );
    TestCheck( &rs3 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( 0 == Queue.CElements() );
    TestCheck( NULL == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );


    Queue.InsertAsNextMost( &rs1, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsNextMost( &rs2, OffsetOf( RandomStruct, pNext ) );

    TestCheck( 2 == Queue.CElements() );
    TestCheck( &rs1 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( &rs2 == Queue.Head() );
    TestCheck( 1 == Queue.CElements() );
    Queue.InsertAsNextMost( &rs3, OffsetOf( RandomStruct, pNext ) );
    TestCheck( 2 == Queue.CElements() );
    TestCheck( &rs2 == Queue.Head() );
    TestCheck( &rs2 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( &rs3 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( NULL == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( 0 == Queue.CElements() );
    

    TestCheck( Queue.FEmpty() );    
    Queue.InsertAsPrevMost( &rs2, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsNextMost( &rs3, OffsetOf( RandomStruct, pNext ) );

    TestCheck( 3 == Queue.CElements() );
    TestCheck( &rs1 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( &rs2 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( &rs3 == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
    TestCheck( Queue.FEmpty() );
    TestCheck( 0 == Queue.CElements() );
    TestCheck( NULL == Queue.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );


    TestCheck( Queue.FEmpty() );    
    Queue.InsertAsPrevMost( &rs1, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsNextMost( &rs2, OffsetOf( RandomStruct, pNext ) );
    Queue.InsertAsNextMost( &rs3, OffsetOf( RandomStruct, pNext ) );

    List = Queue.RemoveList();
    TestCheck( Queue.FEmpty() );
    TestCheck( 0 == Queue.CElements() );
    prs = List.AtomicRemoveList();
    TestCheck( List.FEmpty() );
    TestCheck( prs );
    if ( prs )
    {
        CSimpleQueue< RandomStruct > QueueLocal( prs, OffsetOf( RandomStruct, pNext ) );

        TestCheck( Queue.FEmpty() );
        TestCheck( !QueueLocal.FEmpty() );
        TestCheck( &rs1 == QueueLocal.Head() );
        TestCheck( &rs3 == QueueLocal.Tail() );
        TestCheck( 3 == QueueLocal.CElements() );
        TestCheck( &rs1 == QueueLocal.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
        TestCheck( &rs2 == QueueLocal.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
        TestCheck( &rs3 == QueueLocal.RemovePrevMost( OffsetOf( RandomStruct, pNext ) ) );
        TestCheck( QueueLocal.FEmpty() );
    }

HandleError:

    return err;
}


