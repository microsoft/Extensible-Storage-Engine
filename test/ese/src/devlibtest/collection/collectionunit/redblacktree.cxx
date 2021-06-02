// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <algorithm>
using namespace std;

#include "collectionunittest.hxx"

// node constructor zeroes members and sets the color to red
CUnitTest( RedBlackTreeINodeConstructor, 0, "" );
ERR RedBlackTreeINodeConstructor::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree::BaseType::InvasiveContext inode( 1 );
    TestCheck( NULL == inode.PnodeLeft() );
    TestCheck( NULL == inode.PnodeRight() );
    TestCheck( Red == inode.Color() );
    TestCheck( 1 == inode.Key() );

HandleError:
    return err;
}

// node constructor zeroes members and sets the color to red
CUnitTest( RedBlackTreeNodeConstructor, 0, "" );
ERR RedBlackTreeNodeConstructor::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree::Node node('a');
    TestCheck(NULL == node.PnodeLeft());
    TestCheck(NULL == node.PnodeRight());
    TestCheck(Red == node.Color());
    TestCheck(0 == node.Key());
    TestCheck('a' == node.Data());

HandleError:
    return err;
}

// node members can be set
CUnitTest( RedBlackTreeSetINodeMembers, 0, "" );
ERR RedBlackTreeSetINodeMembers::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree::BaseType::InvasiveContext inode1( 1 );
    Tree::BaseType::InvasiveContext inode2( 2 );

    // (Preserve key ordering when setting nodes)

    // SetRight()
    inode1.SetRight( &inode2 );
    TestCheck( &inode2 == inode1.PnodeRight() );
    inode1.SetRight( NULL );

    // SetLeft()
    inode2.SetLeft( &inode1 );
    TestCheck( &inode1 == inode2.PnodeLeft() );
    inode2.SetLeft( NULL );

    // SetKey
    inode1.SetKey( 10 );
    TestCheck( 10 == inode1.Key() );

    // SetColor
    inode1.SetColor( Black );
    TestCheck( Black == inode1.Color() );

HandleError:
    return err;
}

// node members can be set
CUnitTest( RedBlackTreeSetNodeMembers, 0, "" );
ERR RedBlackTreeSetNodeMembers::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree::Node node('a');
    node.SetData('Z');
    TestCheck('Z' == node.Data());

HandleError:
    return err;
}

// Node can flip its color
CUnitTest( RedBlackTreeFlipINodeColor, 0, "" );
ERR RedBlackTreeFlipINodeColor::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree::BaseType::InvasiveContext inode(1);
    inode.FlipColor();
    TestCheck(Black == inode.Color());
    inode.FlipColor();
    TestCheck(Red == inode.Color());

HandleError:
    return err;
}

// A new tree has no nodes
CUnitTest( RedBlackTreeNewTreeIsEmpty, 0, "" );
ERR RedBlackTreeNewTreeIsEmpty::ErrTest()
{
    ERR err = JET_errSuccess;

    CRedBlackTree<INT, char> tree;
    TestCheck(tree.FEmpty());

HandleError:
    return err;
}

// A simple insert of two nodes
CUnitTest( RedBlackTreeTwoNodeTree, 0, "" );
ERR RedBlackTreeTwoNodeTree::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree tree;
    TestCheck( Tree::ERR::errSuccess == tree.ErrInsert(2, 'b'));
    TestCheck( Tree::ERR::errSuccess == tree.ErrInsert(1, 'a'));

    char c;
    TestCheck(Tree::ERR::errSuccess == tree.ErrFind(1, &c));
    TestCheck('a' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFind(2, &c));
    TestCheck('b' == c);
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFind(3, &c));

    TestCheck(Tree::ERR::errSuccess == tree.ErrFind(1));
    TestCheck(Tree::ERR::errSuccess == tree.ErrFind(2));
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFind(3));

    TestCheck(Tree::ERR::errSuccess == tree.ErrDelete(1));
    TestCheck(Tree::ERR::errSuccess == tree.ErrDelete(2));
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFind(1, &c));
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrDelete(2));
    TestCheck(tree.FEmpty());

HandleError:
    return err;
}

// Test the FindNearest() method
CUnitTest( RedBlackTreeFindNearest, 0, "" );
ERR RedBlackTreeFindNearest::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree tree;

    char c;
    INT i;
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindNearest(1, &i, &c));
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindNearest(1, &i));
    
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(100, 'a'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(200, 'b'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(300, 'c'));

    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(0, &i, &c));
    TestCheck(100 == i);
    TestCheck('a' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(100, &i, &c));
    TestCheck(100 == i);
    TestCheck('a' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(150, &i, &c));
    TestCheck(100 == i);
    TestCheck('a' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(250, &i, &c));
    TestCheck(300 == i);
    TestCheck('c' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(350, &i, &c));
    TestCheck(300 == i);
    TestCheck('c' == c);

    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(0, &i));
    TestCheck(100 == i);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(100, &i));
    TestCheck(100 == i);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(150, &i));
    TestCheck(100 == i);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(250, &i));
    TestCheck(300 == i);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindNearest(350, &i));
    TestCheck(300 == i);

HandleError:
    tree.MakeEmpty();
    return err;
}

// Test the ErrFindGreaterThanOrEqual() method on an empty tree
CUnitTest( FindGreaterThanOrEqualEmpty, 0, "" );
ERR FindGreaterThanOrEqualEmpty::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree tree;

    INT i;
    char c;
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindGreaterThanOrEqual(1, &i, &c));
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindGreaterThanOrEqual(1, &i));

HandleError:
    return err;
}

// Test the ErrFindGreaterThanOrEqual() method when elements are inserted in ascending order
CUnitTest( FindGreaterThanOrEqualAscending, 0, "" );
ERR FindGreaterThanOrEqualAscending::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree tree;

    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(1, '1'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(3, '3'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(5, '5'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(7, '7'));

    INT i;
    char c;
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(0, &i, &c));
    TestCheck(1 == i);
    TestCheck('1' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(1, &i, &c));
    TestCheck(1 == i);
    TestCheck('1' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(2, &i, &c));
    TestCheck(3 == i);
    TestCheck('3' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(3, &i, &c));
    TestCheck(3 == i);
    TestCheck('3' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(4, &i, &c));
    TestCheck(5 == i);
    TestCheck('5' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(5, &i, &c));
    TestCheck(5 == i);
    TestCheck('5' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(6, &i, &c));
    TestCheck(7 == i);
    TestCheck('7' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(7, &i, &c));
    TestCheck(7 == i);
    TestCheck('7' == c);
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindGreaterThanOrEqual(8, &i, &c));

HandleError:
    tree.MakeEmpty();
    return err;
}

// Test the ErrFindGreaterThanOrEqual() method when elements are inserted in descending order
CUnitTest( FindGreaterThanOrEqualDescending, 0, "" );
ERR FindGreaterThanOrEqualDescending::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, char> Tree;
    Tree tree;

    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(7, '7'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(5, '5'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(3, '3'));
    TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(1, '1'));

    INT i;
    char c;
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(0, &i, &c));
    TestCheck(1 == i);
    TestCheck('1' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(1, &i, &c));
    TestCheck(1 == i);
    TestCheck('1' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(2, &i, &c));
    TestCheck(3 == i);
    TestCheck('3' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(3, &i, &c));
    TestCheck(3 == i);
    TestCheck('3' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(4, &i, &c));
    TestCheck(5 == i);
    TestCheck('5' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(5, &i, &c));
    TestCheck(5 == i);
    TestCheck('5' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(6, &i, &c));
    TestCheck(7 == i);
    TestCheck('7' == c);
    TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(7, &i, &c));
    TestCheck(7 == i);
    TestCheck('7' == c);
    TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindGreaterThanOrEqual(8, &i, &c));

HandleError:
    tree.MakeEmpty();
    return err;
}

// Test the ErrFindGreaterThanOrEqual() method when elements are inserted in random order
CUnitTest( FindGreaterThanOrEqualRandom, 0, "" );
ERR FindGreaterThanOrEqualRandom::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef CRedBlackTree<INT, INT> Tree;
    Tree tree;

    const INT cIterations = 1000;
    const INT cNodes = 127;
    for (INT iIteration = 1; iIteration <= cIterations; iIteration++)
    {
        INT rgk[cNodes];

        tree.MakeEmpty();

        // Randomize.
        for (INT i = 0; i < cNodes; i++)
        {
            const INT k = 2 * i + 1;
            rgk[i] = k;
        }
        for (INT i = 0; i < cNodes - 1; i++)
        {
            const INT iswitch = i + 1 + (rand() % (cNodes - 1 - i));
            const INT k = rgk[i];
            rgk[i] = rgk[iswitch];
            rgk[iswitch] = k;
        }

        // Insert.
        for (INT i = 0; i < cNodes; i++)
        {
            const INT kNew = rgk[i];
            const INT vNew = 10 * kNew;
            TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(kNew, vNew));
        }

        INT k, v;

        // Find.
        for (INT i = 0; i < cNodes; i++)
        {
            const INT kExpected = 2 * i + 1;
            const INT vExpected = 10 * kExpected;
            
            // Previous element.
            TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(kExpected - 1, &k, &v));
            TestCheck(kExpected == k);
            TestCheck(vExpected == v);

            // The element.
            TestCheck(Tree::ERR::errSuccess == tree.ErrFindGreaterThanOrEqual(kExpected, &k, &v));
            TestCheck(kExpected == k);
            TestCheck(vExpected == v);
        }

        TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFindGreaterThanOrEqual(2 * cNodes, &k, &v));
    }

HandleError:
    tree.MakeEmpty();
    return err;
}

// Insert all the keys from the array into the tree
ERR ErrInsertAll(CRedBlackTree<INT,INT>& tree, const INT * const rgkeys, INT ckeys)
{
    ERR err = JET_errSuccess;
    for(INT i = 0; i < ckeys; ++i)
    {
        typedef CRedBlackTree<INT,INT> Tree;
        INT data;
        TestCheck(Tree::ERR::errSuccess == tree.ErrInsert(rgkeys[i],rgkeys[i]));
        TestCheck(Tree::ERR::errSuccess == tree.ErrFind(rgkeys[i], &data));
        tree.AssertValid();
    }

HandleError:
    return err;
}

// Retrieve all the keys in the array
ERR ErrRetrieveAll(CRedBlackTree<INT,INT>& tree, const INT * const rgkeys, INT ckeys)
{
    ERR err = JET_errSuccess;
    for(INT i = 0; i < ckeys; ++i)
    {
        typedef CRedBlackTree<INT,INT> Tree;
        INT data;
        TestCheck(Tree::ERR::errSuccess == tree.ErrFind(rgkeys[i],&data));
        TestCheck(data == rgkeys[i]);
        tree.AssertValid();
    }

HandleError:
    return err;
}

// Insert all the keys from the array into the invasive tree
ERR ErrInsertAll( CRedBlackTree<INT, INT>::BaseType& itree, const INT * const rgkeys, INT ckeys, CRedBlackTree<INT, INT>::Node*** pprgNodes )
{
    ERR err = JET_errSuccess;
    *pprgNodes = new CRedBlackTree<INT, INT>::Node*[ckeys];
    for ( INT i = 0; i < ckeys; ++i )
    {
        typedef CRedBlackTree<INT, INT> Tree;
        (*pprgNodes)[i] = new CRedBlackTree<INT, INT>::Node( rgkeys[ i ] );
        TestCheck( Tree::ERR::errSuccess == itree.ErrInsert( rgkeys[ i ], (*pprgNodes)[i] ) );
        TestCheck( Tree::ERR::errSuccess == itree.ErrFind( rgkeys[ i ], &(*pprgNodes)[i] ) );
        itree.AssertValid();
    }

HandleError:
    if ( err < JET_errSuccess )
    {
        delete[] *pprgNodes;
        *pprgNodes = NULL;
    }
    return err;
}

// Retrieve all the keys in the invasive array
ERR ErrRetrieveAll( CRedBlackTree<INT, INT>::BaseType& itree, const INT * const rgkeys, INT ckeys )
{
    ERR err = JET_errSuccess;
    for ( INT i = 0; i < ckeys; ++i )
    {
        typedef CRedBlackTree<INT, INT> Tree;
        Tree::Node* pnode;
        TestCheck( Tree::ERR::errSuccess == itree.ErrFind( rgkeys[ i ], &pnode ) );
        TestCheck( pnode->Data() == rgkeys[ i ] );
        itree.AssertValid();
    }

HandleError:
    return err;
}

// Delete all the keys from the array from the tree
ERR ErrDeleteAll(CRedBlackTree<INT,INT>& tree, const INT * const rgkeys, INT ckeys)
{
    ERR err = JET_errSuccess;
    for(INT i = 0; i < ckeys; ++i)
    {
        typedef CRedBlackTree<INT,INT> Tree;
        INT data;
        TestCheck(Tree::ERR::errSuccess == tree.ErrDelete(rgkeys[i]));
        TestCheck(Tree::ERR::errEntryNotFound == tree.ErrFind(rgkeys[i], &data));
        tree.AssertValid();
    }

HandleError:
    return err;
}

// Insert and delete keys in ascending order
CUnitTest( RedBlackTreeAscendingInserts, 0, "" );
ERR RedBlackTreeAscendingInserts::ErrTest()
{
    ERR err;
    
    INT keys[4096];
    for(INT i = 0; i < _countof(keys); ++i)
    {
        keys[i] = i;
    }

    CRedBlackTree<INT,INT> tree;
    TestCall(ErrInsertAll(tree, keys, _countof(keys)));
    TestCall(ErrRetrieveAll(tree, keys, _countof(keys)));
    TestCall(ErrDeleteAll(tree, keys, _countof(keys)));

HandleError:    
    tree.MakeEmpty();
    return err;
}

// Insert and delete keys in descending order
CUnitTest( RedBlackTreeDescendingInserts, 0, "" );
ERR RedBlackTreeDescendingInserts::ErrTest()
{
    ERR err;
    
    INT keys[4096];
    for(INT i = 0; i < _countof(keys); ++i)
    {
        keys[i] = 10000-i;
    }

    CRedBlackTree<INT,INT> tree;
    TestCall(ErrInsertAll(tree, keys, _countof(keys)));
    TestCall(ErrRetrieveAll(tree, keys, _countof(keys)));
    TestCall(ErrDeleteAll(tree, keys, _countof(keys)));
    
HandleError:    
    tree.MakeEmpty();
    return err; 
}

// Insert keys in random order, delete in a different random order
CUnitTest( RedBlackTreeRandomInserts, 0, "" );
ERR RedBlackTreeRandomInserts::ErrTest()
{
    ERR err;
    
    INT keys[4096];
    for(INT i = 0; i < _countof(keys); ++i)
    {
        keys[i] = i;
    }

    CRedBlackTree<INT,INT> tree;
    
    random_shuffle(keys, keys+_countof(keys));
    TestCall(ErrInsertAll(tree, keys, _countof(keys)));

    TestCall(ErrRetrieveAll(tree, keys, _countof(keys)));

    random_shuffle(keys, keys+_countof(keys));
    TestCall(ErrDeleteAll(tree, keys, _countof(keys)));

HandleError:    
    tree.MakeEmpty();
    return err;
}

// Empty out an invasive tree
CUnitTest( RedBlackTreeInvasiveMakeEmpty, 0, "" );
ERR RedBlackTreeInvasiveMakeEmpty::ErrTest()
{
    ERR err;
    CRedBlackTree<INT, INT>::Node** prgNodes = NULL;

    INT keys[ 4096 ];
    for ( INT i = 0; i < _countof( keys ); ++i )
    {
        keys[ i ] = i;
    }

    CRedBlackTree<INT, INT>::BaseType itree;

    random_shuffle( keys, keys + _countof( keys ) );
    TestCall( ErrInsertAll( itree, keys, _countof( keys ), &prgNodes ) );
    TestCall( ErrRetrieveAll( itree, keys, _countof( keys ) ) );
    itree.MakeEmpty();

    TestCheck( itree.FEmpty() );

HandleError:
    itree.MakeEmpty();

    if ( prgNodes != NULL )
    {
        for ( INT i = 0; i < _countof( keys ); ++i )
        {
            delete prgNodes[i];
            prgNodes[i] = NULL;
        }
        delete[] prgNodes;
        prgNodes = NULL;
    }

    return err;
}

// Empty out a tree
CUnitTest( RedBlackTreeMakeEmpty, 0, "" );
ERR RedBlackTreeMakeEmpty::ErrTest()
{
    ERR err;
    
    INT keys[4096];
    for(INT i = 0; i < _countof(keys); ++i)
    {
        keys[i] = i;
    }

    CRedBlackTree<INT,INT> tree;
    
    random_shuffle(keys, keys+_countof(keys));
    TestCall(ErrInsertAll(tree, keys, _countof(keys)));
    TestCall(ErrRetrieveAll(tree, keys, _countof(keys)));
    tree.MakeEmpty();
    TestCheck(tree.FEmpty());

HandleError:    
    tree.MakeEmpty();
    return err;
}

// Insert random keys, delete 1/2 of the keys and insert more keys
CUnitTest( RedBlackTreeInsertDeleteInsert, 0, "" );
ERR RedBlackTreeInsertDeleteInsert::ErrTest()
{
    ERR err;
    
    INT keys1[4096];
    INT keys2[_countof(keys1)];
    for(INT i = 0; i < _countof(keys1); ++i)
    {
        keys1[i] = i*2;
        keys2[i] = (i*2)+1;
    }

    CRedBlackTree<INT,INT> tree;
    
    random_shuffle(keys1, keys1+_countof(keys1));
    random_shuffle(keys2, keys2+_countof(keys2));
    TestCall(ErrInsertAll(tree, keys1, _countof(keys1)));
    TestCall(ErrRetrieveAll(tree, keys1, _countof(keys1)));
    random_shuffle(keys1, keys1+_countof(keys1));
    TestCall(ErrDeleteAll(tree, keys1, _countof(keys1)/2));
    TestCall(ErrInsertAll(tree, keys2, _countof(keys2)));
    TestCall(ErrRetrieveAll(tree, keys2, _countof(keys2)));
    TestCall(ErrRetrieveAll(tree, keys1+_countof(keys1)/2, _countof(keys1)/2 ));
HandleError:
    tree.MakeEmpty();
    return err;
}


struct TESTIFILEIBOFFSET
{
    DWORD                   m_iFile;
    QWORD                   m_ibOffset;

    //  Set of operators required by [Invasive]RedBlackTree
    //  
    bool operator==( const TESTIFILEIBOFFSET& rhs ) const
    {
        return ( m_iFile == rhs.m_iFile && m_ibOffset == rhs.m_ibOffset );
    }

    bool operator<( const TESTIFILEIBOFFSET& rhs ) const
    {
        return ( m_iFile != rhs.m_iFile ?
                m_iFile < rhs.m_iFile :
                m_ibOffset < rhs.m_ibOffset );
    }
    bool operator>( const TESTIFILEIBOFFSET& rhs ) const
    {
        return ( m_iFile != rhs.m_iFile ?
                m_iFile > rhs.m_iFile :
                m_ibOffset > rhs.m_ibOffset );
    }

    // Perhaps we should make this DEBUG only.
    const CHAR * Sz( INT cb, CHAR * szT ) const
    {
        sprintf_s( szT, cb, "0x%04X:0x%016I64x", m_iFile, m_ibOffset );
        return szT;
    }
};

class TESTIOREQ
{
public:
    static SIZE_T OffsetOfMyIC()        { return OffsetOf( TESTIOREQ, m_ic ); }

    TESTIOREQ() :
        m_ic()
    {
        m_dwStuff = 0;
        m_dwOtherStuff = 0;
        m_icKeyCheck.m_iFile = 0;
        m_icKeyCheck.m_ibOffset = 0;
    }

    TESTIOREQ( INT i ) :
        m_ic()
    {
        m_dwStuff = 0x400 + i;
        m_dwOtherStuff = i;
        m_icKeyCheck.m_iFile = i + 2;
        m_icKeyCheck.m_ibOffset = i * 0x4000;
        CHAR szT[50];
        //wprintf( L"    this = %p = { %hs, %d, %d }\n", this, m_icKeyCheck.Sz( sizeof(szT), szT ), m_dwOtherStuff, m_dwStuff );
    }


    TESTIOREQ( INT iFile, INT ib ) :
        m_ic()
    {
        m_dwStuff = ib;
        m_dwOtherStuff = ( 1 + rand() ) * ib; // in this variant ... used as the identity ... 
        COLLAssert( m_dwOtherStuff != 0 );
        m_icKeyCheck.m_iFile = iFile;
        m_icKeyCheck.m_ibOffset = ib;
        //CHAR szT[50];
        //wprintf( L"    this = %p = { %hs, %d, %d }\n", this, m_icKeyCheck.Sz( sizeof(szT), szT ), m_dwOtherStuff, m_dwStuff );
    }

    DWORD                   m_dwStuff;

private: // ensuring the IC can be private, as only access should be required by the offset function / OffsetOfMyIC
    InvasiveRedBlackTree< TESTIFILEIBOFFSET, TESTIOREQ, OffsetOfMyIC >::InvasiveContext
                            m_ic;

public:
    DWORD                   m_dwOtherStuff;
    TESTIFILEIBOFFSET       m_icKeyCheck;
};

#ifdef DEBUG
#define OnDebug( code )         code
#else
#define OnDebug( code )
#endif


// Test the a set of keys for basic insert/delete.
CUnitTest( RedBlackTreeMultiKeyTestBasicInsertDelete, 0, "" );
ERR RedBlackTreeMultiKeyTestBasicInsertDelete::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef InvasiveRedBlackTree< TESTIFILEIBOFFSET, TESTIOREQ, TESTIOREQ::OffsetOfMyIC > Tree;
    Tree irbt;

    CHAR szKeyT[50];
    CHAR szKeyF[50];

    char c;
    INT i;
    TESTIFILEIBOFFSET   iib = { 1, 0x3000 };
    TESTIFILEIBOFFSET   iibNearest = { 0x42, 0x424242424242 };
    TESTIOREQ *         pioreq = NULL;

    BstfSetVerbosity( bvlPrintTests );

    TESTIOREQ ioreq1( 1 );
    TESTIOREQ ioreq2( 2 );
    TESTIOREQ ioreq3( 3 );
    TESTIOREQ ioreq6( 6 );
    TESTIOREQ ioreq8( 8 );

    TESTIOREQ ioreqBeyond( 100 );

    TESTIFILEIBOFFSET keySeparate;

    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFindNearest( iib, &iibNearest, &pioreq ) );
    TestCheck( pioreq == NULL );
    // the value is left a whatever it was when not found...
    TestCheck( iibNearest.m_iFile == 0x42 );
    TestCheck( iibNearest.m_ibOffset == 0x424242424242 );
    
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq1.m_icKeyCheck, &ioreq1 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq2.m_icKeyCheck, &ioreq2 ) );
    keySeparate = ioreq3.m_icKeyCheck;
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( keySeparate, &ioreq3 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq8.m_icKeyCheck, &ioreq8 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq6.m_icKeyCheck, &ioreq6 ) );

    OnDebug( irbt.Print() );

    TestCheck( Tree::ERR::errSuccess == irbt.ErrFind( keySeparate, &pioreq ) );
    TestCheck( pioreq == &ioreq3 );

    // reusing keySeparate
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( ioreq2.m_icKeyCheck, &keySeparate, &pioreq ) );
    TestCheck( keySeparate == ioreq2.m_icKeyCheck );
    TestCheck( keySeparate.m_iFile == ioreq2.m_icKeyCheck.m_iFile );    // Don't tell SOMEONE that I/SOMEONE don't trust C++ operators! ;-)  Haha, he already knows.
    TestCheck( keySeparate.m_ibOffset == ioreq2.m_icKeyCheck.m_ibOffset );
    TestCheck( &ioreq2 == pioreq );

    keySeparate = ioreq2.m_icKeyCheck;
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFind( keySeparate, &pioreq ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( keySeparate ) );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );

    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq6.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq1.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq8.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq3.m_icKeyCheck ) );

    irbt.Move( &irbt );
    TestCheck( irbt.FEmpty() );

HandleError:
    irbt.MakeEmpty();
    return err;
}



// Test the insert multi-key ... 
CUnitTest( RedBlackTreeMultiKeyTestInsertDup, 0, "" );
ERR RedBlackTreeMultiKeyTestInsertDup::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef InvasiveRedBlackTree< TESTIFILEIBOFFSET, TESTIOREQ, TESTIOREQ::OffsetOfMyIC > Tree;
    Tree irbt;

    TESTIFILEIBOFFSET   iib = { 1, 0x3000 };
    TESTIFILEIBOFFSET   iibNearest = { 0x42, 0x424242424242 };
    TESTIOREQ *         pioreq = NULL;

    BstfSetVerbosity( bvlPrintTests );

    TESTIOREQ ioreq1( 1 );
    TESTIOREQ ioreq2( 2 );
    TESTIOREQ ioreq3( 3 );
    TESTIOREQ ioreq6( 6 );
    TESTIOREQ ioreq8( 8 );

    TESTIOREQ ioreqBeyond( 100 );

    TESTIOREQ ioreqDup( 8 );  ioreqDup.m_dwStuff += 500;
    TESTIFILEIBOFFSET keySeparate;

    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFindNearest( iib, &iibNearest, &pioreq ) );
    TestCheck( pioreq == NULL );
    // the value is left a whatever it was when not found...
    TestCheck( iibNearest.m_iFile == 0x42 );
    TestCheck( iibNearest.m_ibOffset == 0x424242424242 );
    
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq1.m_icKeyCheck, &ioreq1 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq2.m_icKeyCheck, &ioreq2 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq3.m_icKeyCheck, &ioreq3 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq8.m_icKeyCheck, &ioreq8 ) );

    TestCheck( Tree::ERR::errDuplicateEntry == irbt.ErrInsert( ioreqDup.m_icKeyCheck, &ioreqDup ) );

    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq6.m_icKeyCheck, &ioreq6 ) );

    OnDebug( irbt.Print() );

HandleError:
    irbt.MakeEmpty();
    return err;
}

// Test the FindNearest() method for a multi-key
CUnitTest( RedBlackTreeMultiKeyFindNearest, 0, "" );
ERR RedBlackTreeMultiKeyFindNearest::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef InvasiveRedBlackTree< TESTIFILEIBOFFSET, TESTIOREQ, TESTIOREQ::OffsetOfMyIC > Tree;
    Tree irbt;

    CHAR szKeyT[50];
    CHAR szKeyF[50];

    char c;
    INT i;
    TESTIFILEIBOFFSET   iib = { 1, 0x3000 };
    TESTIFILEIBOFFSET   iibNearest = { 0x42, 0x424242424242 };
    TESTIOREQ *         pioreq = NULL;

    BstfSetVerbosity( bvlPrintTests );

    TESTIOREQ ioreq1( 1 );
    TESTIOREQ ioreq2( 2 );
    TESTIOREQ ioreq3( 3 );
    TESTIOREQ ioreq6( 6 );
    TESTIOREQ ioreq8( 8 );

    TESTIOREQ ioreqBeyond( 100 );

    TESTIFILEIBOFFSET keySeparate;
    TESTIOREQ * rgpioreqValidList [] = { &ioreq6, &ioreq2, &ioreq1, &ioreq8, &ioreq3 }; // in psuedo-random order

    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFindNearest( iib, &iibNearest, &pioreq ) );
    TestCheck( pioreq == NULL );
    // the value is left a whatever it was when not found...
    TestCheck( iibNearest.m_iFile == 0x42 );
    TestCheck( iibNearest.m_ibOffset == 0x424242424242 );
    
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq1.m_icKeyCheck, &ioreq1 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq2.m_icKeyCheck, &ioreq2 ) );
    keySeparate = ioreq3.m_icKeyCheck;
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( keySeparate, &ioreq3 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq8.m_icKeyCheck, &ioreq8 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq6.m_icKeyCheck, &ioreq6 ) );

    OnDebug( irbt.Print() );

    TestCheck( Tree::ERR::errSuccess == irbt.ErrFind( keySeparate, &pioreq ) );
    TestCheck( pioreq == &ioreq3 );

    // reusing keySeparate
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( ioreq2.m_icKeyCheck, &keySeparate, &pioreq ) );
    TestCheck( keySeparate == ioreq2.m_icKeyCheck );
    TestCheck( keySeparate.m_iFile == ioreq2.m_icKeyCheck.m_iFile );    // Don't tell SOMEONE that I/SOMEONE don't trust C++ operators! ;-)  Haha, he already knows.
    TestCheck( keySeparate.m_ibOffset == ioreq2.m_icKeyCheck.m_ibOffset );
    TestCheck( &ioreq2 == pioreq );

    //  Check that ErrFindNearest finds all entries we know we've already inserted.
    for( ULONG iioreq = 0; iioreq < _countof( rgpioreqValidList ); iioreq++ )
    {
        TESTIFILEIBOFFSET keyFound;
        keySeparate = rgpioreqValidList[iioreq]->m_icKeyCheck;
        TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( keySeparate, &keyFound, &pioreq ) );
        TestCheck( keySeparate == rgpioreqValidList[iioreq]->m_icKeyCheck ); // search key should not have been altered.
        TestCheck( keySeparate == keyFound );  // should be exact match
        TestCheck( keySeparate == pioreq->m_icKeyCheck );   // and target of found entry should match key
        TestCheck( rgpioreqValidList[iioreq]->m_icKeyCheck == pioreq->m_icKeyCheck ); // not technically necessary
    }

    keySeparate = ioreq3.m_icKeyCheck;
    keySeparate.m_iFile -= 1;
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( keySeparate, &iibNearest, &pioreq ) );
    wprintf( L"\n\t\t\t FoundNearest3.1(%hs) -> %p = %hs \n\n", szKeyT, pioreq, pioreq->m_icKeyCheck.Sz( sizeof(szKeyF), szKeyF ) );
    TestCheck( pioreq == &ioreq3 );

    keySeparate = ioreq3.m_icKeyCheck;
    keySeparate.m_ibOffset -= 0x1000;
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( keySeparate, &iibNearest, &pioreq ) );
    wprintf( L"\n\t\t\t FoundNearest3.2(%hs) -> %p = %hs \n\n", szKeyT, pioreq, pioreq->m_icKeyCheck.Sz( sizeof(szKeyF), szKeyF ) );
    TestCheck( pioreq == &ioreq3 );

    keySeparate.m_iFile = 0x0;
    keySeparate.m_ibOffset = 0x0;
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( keySeparate, &iibNearest, &pioreq ) );
    wprintf( L"\n\t\t\t FoundNearest3.4(%hs) -> %p = %hs \n\n", szKeyT, pioreq, pioreq->m_icKeyCheck.Sz( sizeof(szKeyF), szKeyF ) );
    TestCheck( pioreq == &ioreq1 );


    keySeparate = ioreq2.m_icKeyCheck;
    keySeparate.m_ibOffset += 1;
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( keySeparate, &keySeparate, &pioreq ) );
    wprintf( L"\n\t\t\t FoundNearest3.5(%hs) -> %p = %hs \n\n", szKeyT, pioreq, pioreq->m_icKeyCheck.Sz( sizeof(szKeyF), szKeyF ) );
    TestCheck( pioreq == &ioreq3 );

    keySeparate = ioreqBeyond.m_icKeyCheck;
    keySeparate.m_ibOffset += 1;
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( keySeparate, &iibNearest, &pioreq ) );
    wprintf( L"\n\t\t\t FoundNearest3.B(%hs) -> %p = %hs \n\n", szKeyT, pioreq, pioreq->m_icKeyCheck.Sz( sizeof(szKeyF), szKeyF ) );
    TestCheck( pioreq->m_icKeyCheck < keySeparate );
    TestCheck( pioreq == &ioreq8 );

    wprintf( L" Current tree / show original tree pre move\n" );
    OnDebug( irbt.Print() );

    keySeparate = ioreq2.m_icKeyCheck;
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFind( keySeparate, &pioreq ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( keySeparate ) );
    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFind( keySeparate, &pioreq ) );

    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq6.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq1.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq8.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrDelete( ioreq3.m_icKeyCheck ) );

HandleError:
    irbt.MakeEmpty();
    return err;
}

// Test the FindNearest() method for a multi-key
CUnitTest( RedBlackTreeMultiKeyTestMoveOp, 0, "" );
ERR RedBlackTreeMultiKeyTestMoveOp::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef InvasiveRedBlackTree< TESTIFILEIBOFFSET, TESTIOREQ, TESTIOREQ::OffsetOfMyIC > Tree;
    Tree irbt;
    Tree irbtNext;

    CHAR szKeyT[50];
    CHAR szKeyF[50];

    char c;
    INT i;
    TESTIFILEIBOFFSET   iib = { 1, 0x3000 };
    TESTIFILEIBOFFSET   iibNearest = { 0x42, 0x424242424242 };
    TESTIOREQ *         pioreq = NULL;

    BstfSetVerbosity( bvlPrintTests );

    TESTIOREQ ioreq1( 1 );
    TESTIOREQ ioreq2( 2 );
    TESTIOREQ ioreq3( 3 );
    TESTIOREQ ioreq6( 6 );
    TESTIOREQ ioreq8( 8 );

    TESTIOREQ ioreqBeyond( 100 );

    TESTIFILEIBOFFSET keySeparate;

    TestCheck( Tree::ERR::errEntryNotFound == irbt.ErrFindNearest( iib, &iibNearest, &pioreq ) );
    TestCheck( pioreq == NULL );
    // the value is left a whatever it was when not found...
    TestCheck( iibNearest.m_iFile == 0x42 );
    TestCheck( iibNearest.m_ibOffset == 0x424242424242 );
    
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq1.m_icKeyCheck, &ioreq1 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq2.m_icKeyCheck, &ioreq2 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq3.m_icKeyCheck, &ioreq3 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq8.m_icKeyCheck, &ioreq8 ) );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrInsert( ioreq6.m_icKeyCheck, &ioreq6 ) );

    OnDebug( irbt.Print() );

    keySeparate = ioreq3.m_icKeyCheck;
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFind( keySeparate, &pioreq ) );
    TestCheck( pioreq == &ioreq3 );

    // reusing keySeparate
    keySeparate.Sz( sizeof(szKeyT), szKeyT );
    TestCheck( Tree::ERR::errSuccess == irbt.ErrFindNearest( ioreq2.m_icKeyCheck, &keySeparate, &pioreq ) );
    TestCheck( keySeparate == ioreq2.m_icKeyCheck );
    TestCheck( keySeparate.m_iFile == ioreq2.m_icKeyCheck.m_iFile );    // Don't tell SOMEONE that I/SOMEONE don't trust C++ operators! ;-)  Haha, he already knows.
    TestCheck( keySeparate.m_ibOffset == ioreq2.m_icKeyCheck.m_ibOffset );
    TestCheck( &ioreq2 == pioreq );

    wprintf( L"\tTrees pre move\n" );
    OnDebug( irbt.Print() );
    OnDebug( irbtNext.Print() );

    irbtNext.Move( &irbt );

    TestCheck( irbt.FEmpty() );

    wprintf( L"\tTrees post move\n" );
    OnDebug( irbt.Print() );
    OnDebug( irbtNext.Print() );

    keySeparate = ioreq2.m_icKeyCheck;
    TestCheck( Tree::ERR::errSuccess == irbtNext.ErrFind( keySeparate, &pioreq ) );
    TestCheck( Tree::ERR::errSuccess == irbtNext.ErrDelete( keySeparate ) );
    TestCheck( Tree::ERR::errEntryNotFound == irbtNext.ErrFind( keySeparate, &pioreq ) );

    TestCheck( Tree::ERR::errSuccess == irbtNext.ErrDelete( ioreq6.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbtNext.ErrDelete( ioreq1.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbtNext.ErrDelete( ioreq8.m_icKeyCheck ) );
    TestCheck( Tree::ERR::errSuccess == irbtNext.ErrDelete( ioreq3.m_icKeyCheck ) );

    irbtNext.Move( &irbt );
    TestCheck( irbt.FEmpty() );

HandleError:
    irbt.MakeEmpty();
    return err;
}

//  This should really be pulled from somewhere else.
LONG PctRand_( const LONG pctLow = 0, const LONG pctHigh = 100 );
LONG PctRand_( const LONG pctLow, const LONG pctHigh )
{
    COLLAssert( pctLow <= pctHigh );
    LONG dpct = 1 + pctHigh - pctLow;
    return pctLow + ( rand() % dpct );
}

// Test Ioreq Tree
typedef InvasiveRedBlackTree< TESTIFILEIBOFFSET, TESTIOREQ, TESTIOREQ::OffsetOfMyIC > TITree;


class CTestIoreqSet
{
    const static ULONG Cioreq = 99;
    TESTIOREQ   m_rgioreq[ Cioreq ];

public:
    CTestIoreqSet()
    {
        memset( m_rgioreq, 0, sizeof( m_rgioreq ) );
    }

    void InitRandSet( ULONG ulSeed )
    {
        srand( ulSeed );
        wprintf( L"\tInit'ing Rand Set %p w/ ulSeed = %d (0x%x)\n", this, ulSeed, ulSeed );

        for( ULONG iioreq = 0; iioreq < _countof( m_rgioreq ); iioreq++ )
        {
            new ( &m_rgioreq[iioreq] )TESTIOREQ( 1 + rand() % 2, 1 + iioreq );

            //  All this expected and depended upon in CheckCmpSetTrees
            COLLAssert( m_rgioreq[iioreq].m_icKeyCheck.m_iFile == 1 || m_rgioreq[iioreq].m_icKeyCheck.m_iFile == 2 );
            COLLAssert( m_rgioreq[iioreq].m_dwOtherStuff % m_rgioreq[iioreq].m_icKeyCheck.m_ibOffset == 0 );
            COLLAssert( m_rgioreq[iioreq].m_dwOtherStuff != 0 );
        }
        
        
    }

    void DuplicateSet( CTestIoreqSet * ptisCopyTo ) const
    {
        memcpy( ptisCopyTo->m_rgioreq, m_rgioreq, sizeof( m_rgioreq ) );
        // Note: rand() COULD select exact same slot
        ULONG iioreqSwap = rand() % _countof( m_rgioreq );
        ULONG iioreqSwapToo = rand() % _countof( m_rgioreq );
        TESTIOREQ tiSwap = ptisCopyTo->m_rgioreq[ iioreqSwap ];
        ptisCopyTo->m_rgioreq[ iioreqSwap ] = ptisCopyTo->m_rgioreq[ iioreqSwapToo ];
        ptisCopyTo->m_rgioreq[ iioreqSwapToo ] = tiSwap;
    }

    void AddToTree( TITree * const ptit )
    {
        ERR err = JET_errSuccess; // TestCheck() sets it, but also asserts.

        for( ULONG iioreq = 0; iioreq < _countof( m_rgioreq ); iioreq++ )
        {
            TITree::ERR errI = ptit->ErrInsert( m_rgioreq[ iioreq ].m_icKeyCheck, &m_rgioreq[ iioreq ] );
            TestCheck( errI == TITree::ERR::errSuccess );
        }

    HandleError:
        ;
    }

    //  Note anything between 10 - 90% will guarantee at least one element in both (if there are at least 2 elements) ...
    void RandSplitToTwoTrees( TITree * const ptitHalf, TITree * const ptitOther, const LONG pctInFirstHalf = 50 )
    {
        ERR err = JET_errSuccess; // TestCheck() sets it, but also asserts.

        TITree * ptitGuaranteed = ( pctInFirstHalf >= 10 && pctInFirstHalf <= 90 ) ?
                                        ( rand() % 2 ? ptitHalf : ptitOther ) :
                                        NULL;
        for( ULONG iioreq = 0; iioreq < _countof( m_rgioreq ); iioreq++ )
        {
            COLLAssert( m_rgioreq[ iioreq ].m_icKeyCheck.m_ibOffset != 0 );

            TITree::ERR errI;
            if ( ptitGuaranteed )
            {
                errI = ptitGuaranteed->ErrInsert( m_rgioreq[ iioreq ].m_icKeyCheck, &m_rgioreq[ iioreq ] );
                if ( iioreq == 0 )
                {
                    ptitGuaranteed = ( ptitGuaranteed == ptitHalf ) ? ptitOther : ptitHalf;
                }
                else
                {
                    ptitGuaranteed = NULL;
                    COLLAssert( !ptitHalf->FEmpty() );
                    COLLAssert( !ptitOther->FEmpty() );
                }
            }
            else if ( PctRand_() < pctInFirstHalf )
            {
                errI = ptitHalf->ErrInsert( m_rgioreq[ iioreq ].m_icKeyCheck, &m_rgioreq[ iioreq ] );
            }
            else
            {
                errI = ptitOther->ErrInsert( m_rgioreq[ iioreq ].m_icKeyCheck, &m_rgioreq[ iioreq ] );
            }
            TestCheck( errI == TITree::ERR::errSuccess );
        }

    HandleError:
        ;
    }

    static void CheckCmpSetTrees( TITree * const ptitOne, TITree * const ptitTwo )
    {
        ERR err = JET_errSuccess; // TestCheck() sets it, but also asserts.

        for ( ULONG ib = 0; ib <= Cioreq; ib++ )
        {
            BOOL fFound = fFalse;
            for ( ULONG iFile = 1; iFile < 3; iFile++ )
            {
                TESTIOREQ * pioreqOne = NULL;
                TESTIOREQ * pioreqTwo = NULL;

                const TESTIFILEIBOFFSET iibNextCheck = { iFile, ib };

                TITree::ERR errFindOne = ptitOne->ErrFind( iibNextCheck, &pioreqOne );
                TITree::ERR errFindTwo = ptitTwo->ErrFind( iibNextCheck, &pioreqTwo );

                //  Agree that they found it or not.
                TestCheck( errFindOne == errFindTwo );

                if ( errFindOne == TITree::ERR::errSuccess && errFindTwo == TITree::ERR::errSuccess )
                {
                    // ... and both found the same if succeeded.
                    TestCheck( !!pioreqOne == !!pioreqTwo );
                }

                if ( !fFound )
                {
                    fFound = ( errFindOne == TITree::ERR::errSuccess );
                }

                //  and if we did find, check same same.
                if ( pioreqOne )
                {
                    //  Almost assuredly will not go off, or ErrFind() is not working.
                    TestCheck( pioreqOne->m_dwStuff == pioreqTwo->m_dwStuff );
                    TestCheck( pioreqOne->m_icKeyCheck.m_iFile == pioreqTwo->m_icKeyCheck.m_iFile );
                    TestCheck( pioreqOne->m_icKeyCheck.m_ibOffset == pioreqTwo->m_icKeyCheck.m_ibOffset );

                    //  Check it is our kind of TESTIOREQ from the 2nd .ctor with two args (see TESTIOREQ class).
                    TestCheck( pioreqOne->m_dwOtherStuff % pioreqOne->m_icKeyCheck.m_ibOffset == 0 );
                    TestCheck( pioreqTwo->m_dwOtherStuff % pioreqOne->m_icKeyCheck.m_ibOffset == 0 );

                    // Full identity check ... pulled identities should be same.
                    TestCheck( pioreqOne->m_dwOtherStuff == pioreqTwo->m_dwOtherStuff );

                    TestCheck( pioreqOne != pioreqTwo );
                }
            }

            //  Every ib above zero is in iFile = 1 or iFile = 2.
            TestCheck( fFound || ib == 0 );
            TestCheck( ib != 0 || !fFound ); // this entry is always missing, as we start at ib = 1
        }

    HandleError:
        ;
    }

    static TESTIOREQ * PioreqSelectRandItem( TITree * const ptit )
    {
        TESTIOREQ * pioreqRet = NULL;
        TITree::ERR err;
        TESTIFILEIBOFFSET iibSearching;

        //  Start full random ...
        iibSearching.m_iFile = 1 + rand() % 2;
        iibSearching.m_ibOffset = 0 + rand() % 101;

        err = ptit->ErrFind( iibSearching, &pioreqRet );
        if ( TITree::ERR::errSuccess == err )
        {
            return pioreqRet;
        }

        //  Try ensuring we searching in iFile = 1 (lower file num)
        iibSearching.m_iFile = 1;
        err = ptit->ErrFind( iibSearching, &pioreqRet );
        if ( TITree::ERR::errSuccess == err )
        {
            return pioreqRet;
        }

        //  Do below what we didn't fine
        iibSearching.m_iFile = 1 + rand() % 2;
        iibSearching.m_ibOffset = 0 + rand() % max( iibSearching.m_ibOffset, 1 );
        err = ptit->ErrFind( iibSearching, &pioreqRet );
        if ( TITree::ERR::errSuccess == err )
        {
            return pioreqRet;
        }

        iibSearching.m_iFile = 0;
        iibSearching.m_ibOffset = 0;
        TESTIFILEIBOFFSET iibT;
        err = ptit->ErrFindNearest( iibSearching, &iibT, &pioreqRet );
        if ( TITree::ERR::errSuccess == err )
        {
            return pioreqRet;
        }

        pioreqRet = NULL;

        return pioreqRet;
    }

    static INT CmpSetTrees( TITree * const ptitOne, TITree * const ptitTwo )
    {
        ERR err = JET_errSuccess; // TestCheck() sets it, but also asserts.

        for ( ULONG ib = 0; ib <= Cioreq; ib++ )
        {
            BOOL fFound = fFalse;
            for ( ULONG iFile = 1; iFile < 3; iFile++ )
            {
                TESTIOREQ * pioreqOne = NULL;
                TESTIOREQ * pioreqTwo = NULL;

                const TESTIFILEIBOFFSET iibNextCheck = { iFile, ib };

                TITree::ERR errFindOne = ptitOne->ErrFind( iibNextCheck, &pioreqOne );
                TITree::ERR errFindTwo = ptitTwo->ErrFind( iibNextCheck, &pioreqTwo );

                //  Agree that they found it or not.
                if ( errFindOne != errFindTwo )
                {
                    return -1;
                }
                //      and both found, or neither returned item
                TestCheck( !!pioreqOne == !!pioreqTwo );
                if ( !fFound )
                {
                    fFound = ( errFindOne == TITree::ERR::errSuccess );
                }

                //  and if we did find, check same same.
                if ( pioreqOne )
                {
                    //  Almost assuredly will not go off, or ErrFind() is not working.
                    TestCheck( pioreqOne->m_dwStuff == pioreqTwo->m_dwStuff );
                    TestCheck( pioreqOne->m_icKeyCheck.m_iFile == pioreqTwo->m_icKeyCheck.m_iFile );
                    TestCheck( pioreqOne->m_icKeyCheck.m_ibOffset == pioreqTwo->m_icKeyCheck.m_ibOffset );

                    //  Check it is our kind of TESTIOREQ from the 2nd .ctor with two args (see TESTIOREQ class).
                    TestCheck( pioreqOne->m_dwOtherStuff % pioreqOne->m_icKeyCheck.m_ibOffset == 0 );
                    TestCheck( pioreqTwo->m_dwOtherStuff % pioreqOne->m_icKeyCheck.m_ibOffset == 0 );

                    // Full identity check ... pulled identities should be same.
                    TestCheck( pioreqOne->m_dwOtherStuff == pioreqTwo->m_dwOtherStuff );

                    TestCheck( pioreqOne != pioreqTwo );
                }
            }

            //  Every ib above zero is in iFile = 1 or iFile = 2.
            TestCheck( ib != 0 || !fFound ); // this entry is always missing, as we start at ib = 1
        }

    HandleError:

        //  yay, same!
        return 0;
    }

};

CUnitTest( RedBlackTreeMultiKeyTestIoreqSetInfra, 0, "" );
ERR RedBlackTreeMultiKeyTestIoreqSetInfra::ErrTest()
{
    ERR err = JET_errSuccess;

    TITree titControl;
    TITree titTest;

    CTestIoreqSet   tisControl;
    CTestIoreqSet   tisTest;

    tisControl.InitRandSet( GetTickCount() );
    tisControl.DuplicateSet( &tisTest );

    tisControl.AddToTree( &titControl );
    tisTest.AddToTree( &titTest );

    OnDebug( titControl.Print() );
    OnDebug( titTest.Print() );

    CTestIoreqSet::CheckCmpSetTrees( &titControl, &titTest );

    titControl.MakeEmpty();
    titTest.MakeEmpty();

    return err;
}


CUnitTest( RedBlackTreeMultiKeyTestErrMergeAll, 0, "" );
ERR RedBlackTreeMultiKeyTestErrMergeAll::ErrTest()
{
    ERR err = JET_errSuccess;

    TITree titControl;
    TITree titTestHalf;
    TITree titOtherHalf;

    CHAR szKeyT[50];
    CHAR szKeyF[50];

    char c;
    INT i;

    BstfSetVerbosity( bvlPrintTests - 1 );

    CTestIoreqSet   tisControl;
    CTestIoreqSet   tisTest;

    tisControl.InitRandSet( GetTickCount() );

    tisControl.DuplicateSet( &tisTest );

    tisControl.AddToTree( &titControl );

    //const LONG pctInHalf = 3;
    const LONG pctInHalf = PctRand_();
    tisTest.RandSplitToTwoTrees( &titTestHalf, &titOtherHalf, pctInHalf  );

    OnDebug( titControl.Print() );
    OnDebug( titTestHalf.Print() );
    OnDebug( titOtherHalf.Print() );

    if ( pctInHalf >= 10 && pctInHalf <= 90 )
    {
        TestCheck( 0 != CTestIoreqSet::CmpSetTrees( &titControl, &titTestHalf ) );
        TestCheck( 0 != CTestIoreqSet::CmpSetTrees( &titControl, &titOtherHalf ) );
    }

    //  The actual ErrMerge
    TestCheck( TITree::ERR::errSuccess == titTestHalf.ErrMerge( &titOtherHalf ) );

    //  Results checks
    TestCheck( titOtherHalf.FEmpty() );
    TestCheck( 0 == CTestIoreqSet::CmpSetTrees( &titControl, &titTestHalf ) );
    CTestIoreqSet::CheckCmpSetTrees( &titControl, &titTestHalf );

    OnDebug( titTestHalf.Print() );

HandleError:
    titControl.MakeEmpty();
    titTestHalf.MakeEmpty();
    titOtherHalf.MakeEmpty();

    return err;
}

CUnitTest( RedBlackTreeMultiKeyTestErrMergeMost, 0, "" );
ERR RedBlackTreeMultiKeyTestErrMergeMost::ErrTest()
{
    ERR err = JET_errSuccess;

    TITree titControl;
    TITree titTestHalf;
    TITree titOtherHalf;

    CHAR szKeyT[50];
    CHAR szKeyF[50];

    char c;
    INT i;

    BstfSetVerbosity( bvlPrintTests - 1 );

    CTestIoreqSet   tisControl;
    CTestIoreqSet   tisTest;

    tisControl.InitRandSet( GetTickCount() );

    tisControl.DuplicateSet( &tisTest );

    tisControl.AddToTree( &titControl );

    const LONG pctInHalf = PctRand_( 10, 90 );
    tisTest.RandSplitToTwoTrees( &titTestHalf, &titOtherHalf, pctInHalf  );

    OnDebug( titControl.Print() );
    OnDebug( titTestHalf.Print() );
    OnDebug( titOtherHalf.Print() );

    TESTIOREQ tiToDupFromHalf = *CTestIoreqSet::PioreqSelectRandItem( &titTestHalf );
    TESTIOREQ tiToDupFromOther = *CTestIoreqSet::PioreqSelectRandItem( &titOtherHalf );
    COLLAssert( tiToDupFromHalf.m_icKeyCheck.m_ibOffset != 0 );
    COLLAssert( tiToDupFromOther.m_icKeyCheck.m_ibOffset != 0 );

    if ( pctInHalf >= 10 && pctInHalf <= 90 )
    {
        TestCheck( 0 != CTestIoreqSet::CmpSetTrees( &titControl, &titTestHalf ) );
        TestCheck( 0 != CTestIoreqSet::CmpSetTrees( &titControl, &titOtherHalf ) );
    }

    TITree::ERR errI1 = titTestHalf.ErrInsert( tiToDupFromOther.m_icKeyCheck, &tiToDupFromOther );
    TITree::ERR errI2 = titOtherHalf.ErrInsert( tiToDupFromHalf.m_icKeyCheck, &tiToDupFromHalf );
    TestCheck( errI1 == TITree::ERR::errSuccess );
    TestCheck( errI2 == TITree::ERR::errSuccess );

    //  The actual ErrMerge
    TITree::ERR errM = titTestHalf.ErrMerge( &titOtherHalf );

    //  Check the merged tree
    TestCheck( errM == TITree::ERR::errDuplicateEntry );
    TestCheck( !titOtherHalf.FEmpty() );

    TestCheck( 0 == CTestIoreqSet::CmpSetTrees( &titControl, &titTestHalf ) );
    CTestIoreqSet::CheckCmpSetTrees( &titControl, &titTestHalf );

    //  Check the left overs.
    TESTIOREQ * pioreqChineseLeftOvers;
    errI1 = titOtherHalf.ErrFind( tiToDupFromHalf.m_icKeyCheck, &pioreqChineseLeftOvers );
    TestCheck( errI1 == TITree::ERR::errSuccess );
    //      check identity ...
    TestCheck( pioreqChineseLeftOvers->m_dwOtherStuff == tiToDupFromHalf.m_dwOtherStuff || pioreqChineseLeftOvers->m_dwOtherStuff == tiToDupFromOther.m_dwOtherStuff );
    TESTIOREQ * pioreqMexicanLeftOvers;
    errI1 = titOtherHalf.ErrFind( tiToDupFromOther.m_icKeyCheck, &pioreqMexicanLeftOvers );
    TestCheck( errI1 == TITree::ERR::errSuccess );
    //      check identity ...
    TestCheck( pioreqChineseLeftOvers->m_dwOtherStuff == tiToDupFromHalf.m_dwOtherStuff || pioreqChineseLeftOvers->m_dwOtherStuff == tiToDupFromOther.m_dwOtherStuff );

    OnDebug( titTestHalf.Print() );
    OnDebug( titOtherHalf.Print() );

HandleError:
    titControl.MakeEmpty();
    titTestHalf.MakeEmpty();
    titOtherHalf.MakeEmpty();

    return err;
}


