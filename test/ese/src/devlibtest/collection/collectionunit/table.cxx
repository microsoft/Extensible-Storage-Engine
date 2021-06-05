// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

//  ================================================================
class TableTest : public UNITTEST
//  ================================================================
{
    private:
        static TableTest s_instance;

    protected:
        TableTest() {}
    public:
        ~TableTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

TableTest TableTest::s_instance;

const char * TableTest::SzName() const          { return "Table"; };
const char * TableTest::SzDescription() const   { return  "Tests the collection libraries' table (CTable) facility."; }
bool TableTest::FRunUnderESE98() const          { return true; }
bool TableTest::FRunUnderESENT() const          { return true; }
bool TableTest::FRunUnderESE97() const          { return true; }


class TableEntry
{
    public:
        INT m_id;
        TableEntry()
        {
            m_id = 0;
        }

        TableEntry( INT id )
        {
            m_id = id;
        }
};

typedef CTable< INT, TableEntry > CTestTable;

inline INT CTestTable::CKeyEntry:: Cmp( const INT& id ) const
{
    return m_id - id;
}

inline INT CTestTable::CKeyEntry:: Cmp( const CTestTable::CKeyEntry& keyentry ) const
{
    return Cmp( keyentry.m_id );
}

//  ================================================================
ERR TableTest::ErrTest()
//  ================================================================
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting table (CTable) ...\n");

    // create table
    CTestTable table;
    CTestTable tableClone;

    // test size
    TestCheck( 0 == table.Size() );

    // test load
    TableEntry data[] = {
        TableEntry( 90 ),
        TableEntry( 40 ),
        TableEntry( 60 ),
        TableEntry( 10 ),
        TableEntry( 70 ),
        TableEntry( 80 ),
        TableEntry( 0 ),
        TableEntry( 20 ),
        TableEntry( 50 ),
        TableEntry( 30 ),
};
    table.ErrLoad( _countof( data ), data );
    TestCheck( _countof( data ) == table.Size() );

    // test clone
    tableClone.ErrClone( table );
    TestCheck( _countof( data ) == tableClone.Size() );
    for ( INT i = 0; i < _countof( data ); i++ )
    {
        TestCheck( table.PEntry( i )->m_id == tableClone.PEntry( i )->m_id );
    }

    // test seek
    TestCheck( 50 == table.SeekLT( 55 )->m_id );

    TestCheck( 50 == table.SeekLE( 55 )->m_id );
    TestCheck( 50 == table.SeekLE( 50 )->m_id );

    TestCheck( 70 == table.SeekEQ( 70 )->m_id );
    TestCheck( NULL == table.SeekEQ( 65 ) );

    TestCheck( 70 == table.SeekHI( 70 )->m_id );

    TestCheck( 60 == table.SeekGE( 55 )->m_id );
    TestCheck( 50 == table.SeekGE( 50 )->m_id );

    TestCheck( 60 == table.SeekGT( 50 )->m_id );

    // test clear
    tableClone.~CTestTable();
    TestCheck( 0 == tableClone.Size() );

    table.Clear();
    TestCheck( 0 == table.Size() );

HandleError:
    return err;
}
