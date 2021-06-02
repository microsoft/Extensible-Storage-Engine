// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"
#include "collection.hxx"


//  ================================================================
class CountedInvasiveLinkedListTest : public UNITTEST
//  ================================================================
{
    private:
        static CountedInvasiveLinkedListTest s_instance;

    protected:
        CountedInvasiveLinkedListTest() {}
    public:
        ~CountedInvasiveLinkedListTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();

    private:

        class CElement
        {
            public:

                static SIZE_T OffsetOfILE() { return OffsetOf( CElement, m_ile ); }

            private:

                typename CCountedInvasiveList<CElement, OffsetOfILE>::CElement  m_ile;
        };
};

CountedInvasiveLinkedListTest CountedInvasiveLinkedListTest::s_instance;

const char * CountedInvasiveLinkedListTest::SzName() const          { return "Counted Invasive Linked List"; };
const char * CountedInvasiveLinkedListTest::SzDescription() const
{
    return "Tests the collection libraries' counted invasive linked list facility.";
}
bool CountedInvasiveLinkedListTest::FRunUnderESE98() const          { return true; }
bool CountedInvasiveLinkedListTest::FRunUnderESENT() const          { return true; }
bool CountedInvasiveLinkedListTest::FRunUnderESE97() const          { return true; }


//  ================================================================
ERR CountedInvasiveLinkedListTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    CCountedInvasiveList<CElement, CElement::OffsetOfILE> il;
    CElement e1;
    CElement e2;
    CElement e3;
    CCountedInvasiveList<CElement, CElement::OffsetOfILE> ilCopy;

    TestCheck( il.Count() == 0 );

    il.InsertAsNextMost( &e1 );
    TestCheck( il.Count() == 1 );
    il.InsertAsPrevMost( &e2 );
    TestCheck( il.Count() == 2 );
    il.Insert( &e3, &e1 );
    TestCheck( il.Count() == 3 );

    il.Remove( &e1 );
    TestCheck( il.Count() == 2 );

    ilCopy = il;
    TestCheck( il.Count() == 2 );

    ilCopy.Empty();
    TestCheck( ilCopy.Count() == 0 );

HandleError:
    return err;
}
