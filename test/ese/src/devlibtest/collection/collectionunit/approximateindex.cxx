// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

class CTestResource
{
    public:
        CTestResource()
        {
            m_dw = UINT_MAX;
        }

    public:
        DWORD m_dw;

    public:
        static SIZE_T OffsetOfAIIC() { return OffsetOf( CTestResource, m_aiic ); }
        typename CApproximateIndex< DWORD, CTestResource, OffsetOfAIIC >::CInvasiveContext m_aiic;
};

DECLARE_APPROXIMATE_INDEX( DWORD, CTestResource, CTestResource::OffsetOfAIIC, CTestApproximateIndex );

class ApproximateIndexTest : public UNITTEST
{
    private:
        static ApproximateIndexTest s_instance;

    private:
        ERR ErrInsertRetrieveDescendingLowRes();
        ERR ErrInsertRetrieveDescendingHighRes();
        ERR ErrInsertRetrieveAscendingLowRes();
        ERR ErrInsertRetrieveAscendingHighRes();
        ERR ErrInsertRetrieve( const bool fAscending, const bool fHighRes );

    protected:
        ApproximateIndexTest() {}

    public:
        ~ApproximateIndexTest() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

ApproximateIndexTest ApproximateIndexTest::s_instance;

const char * ApproximateIndexTest::SzName() const          { return "ApproximateIndexTest"; };
const char * ApproximateIndexTest::SzDescription() const   { return "Tests the collection libraries' approximate index (CApproximateIndex) facility."; }
bool ApproximateIndexTest::FRunUnderESE98() const          { return true; }
bool ApproximateIndexTest::FRunUnderESENT() const          { return true; }
bool ApproximateIndexTest::FRunUnderESE97() const          { return true; }

ERR ApproximateIndexTest::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting approximate index (CApproximateIndex) ...\n" );

    TestCheckErr( ErrInsertRetrieveDescendingLowRes() );
    TestCheckErr( ErrInsertRetrieveDescendingHighRes() );
    TestCheckErr( ErrInsertRetrieveAscendingLowRes() );
    TestCheckErr( ErrInsertRetrieveAscendingHighRes() );

HandleError:
    return err;
}

ERR ApproximateIndexTest::ErrInsertRetrieveDescendingLowRes()
{
    ERR err = JET_errSuccess;

    wprintf( L"\t\t%hs\n", __FUNCTION__ );

    TestCheckErr( ErrInsertRetrieve( false, false ) );

HandleError:
    return err;
}

ERR ApproximateIndexTest::ErrInsertRetrieveDescendingHighRes()
{
    ERR err = JET_errSuccess;

    wprintf( L"\t\t%hs\n", __FUNCTION__ );

    TestCheckErr( ErrInsertRetrieve( false, true ) );

HandleError:
    return err;
}

ERR ApproximateIndexTest::ErrInsertRetrieveAscendingLowRes()
{
    ERR err = JET_errSuccess;

    wprintf( L"\t\t%hs\n", __FUNCTION__ );

    TestCheckErr( ErrInsertRetrieve( true, false ) );

HandleError:
    return err;
}

ERR ApproximateIndexTest::ErrInsertRetrieveAscendingHighRes()
{
    ERR err = JET_errSuccess;

    wprintf( L"\t\t%hs\n", __FUNCTION__ );

    TestCheckErr( ErrInsertRetrieve( true, true ) );

HandleError:
    return err;
}

ERR ApproximateIndexTest::ErrInsertRetrieve( const bool fAscending, const bool fHighRes )
{
    ERR err = JET_errSuccess;
    CTestApproximateIndex::ERR errIndex = CTestApproximateIndex::ERR::errSuccess;
    const DWORD cElements = 100;
    const DWORD dwKeyDelta = 200;
    const DWORD cBucket = 10;
    const DWORD cBucketActual = ( fHighRes ? cElements : ( cBucket + 1 ) );
    const DWORD dwKeyPrecision = 1000000000;
    const DWORD dwKeyUncertainty = fHighRes ? ( dwKeyDelta / 2 ) : ( ( cElements / cBucket ) * dwKeyDelta );
    const double dblSpeedSizeTradeoff = 0.0;

    CTestApproximateIndex index( 1000000 );
    CTestApproximateIndex::CLock lock;

    CTestResource* pres = NULL;
    CTestResource rgres[ cElements ];
    CTestResource rgresOlder[ cElements ];
    CTestResource rgresNewer[ cElements ];
    CTestResource rgresActual[ cElements ];
    DWORD rgiBucket[ cElements ] = { 0 };
    DWORD rgdwMapping[ cElements ] = { UINT_MAX };

    for ( DWORD i = 0; i < cElements; i++ )
    {
        rgresOlder[ i ].m_dw = i * dwKeyDelta;
    }
    for ( DWORD i = 0; i < cElements; i++ )
    {
        rgres[ i ].m_dw = dwKeyUncertainty + ( i + cElements ) * dwKeyDelta;
    }
    for ( DWORD i = 0; i < cElements; i++ )
    {
        rgresNewer[ i ].m_dw = 2 * dwKeyUncertainty + ( i + 2 * cElements ) * dwKeyDelta;
    }

    TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrInit( dwKeyPrecision, dwKeyUncertainty, dblSpeedSizeTradeoff ) );

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.LockKeyPtr( rgres[ i ].m_dw, &rgres[ i ], &lock );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrInsertEntry( &lock, &rgres[ i ], fAscending ) );
        index.UnlockKeyPtr( &lock );
    }

    index.MoveBeforeFirst( &lock );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMovePrev( &lock ) );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    index.UnlockKeyPtr( &lock );

    index.MoveAfterLast( &lock );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMoveNext( &lock ) );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    index.UnlockKeyPtr( &lock );

    index.MoveBeforeFirst( &lock );
    for ( DWORD i = 0; i < cElements; i++ )
    {
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrMoveNext( &lock ) );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );

        if ( fHighRes )
        {
            TestCheck( pres->m_dw == rgres[ i ].m_dw );
        }
        else
        {
            const DWORD iBucketExpected = rgres[ ( i / cBucket ) * cBucket ].m_dw / dwKeyUncertainty;
            const DWORD iBucket = pres->m_dw / dwKeyUncertainty;
            TestCheck( ( iBucket == iBucketExpected ) ||
                       ( ( iBucket < iBucketExpected ) && ( ( iBucketExpected - iBucket ) == 1 ) ) ||
                       ( ( iBucket > iBucketExpected ) && ( ( iBucket - iBucketExpected ) == 1 ) ) );
        }

        TestCheck( rgresActual[ i ].m_dw == UINT_MAX );
        rgresActual[ i ].m_dw = pres->m_dw;

        if ( i == 0 )
        {
            rgiBucket[ i ] = 1;
        }
        else
        {
            const LONG cmp = index.CmpKey( pres->m_dw, rgresActual[ i - 1 ].m_dw );
            TestCheck( cmp >= 0 );
            if ( cmp == 0 )
            {
                rgiBucket[ i ] = rgiBucket[ i - 1 ];
            }
            else if ( cmp > 0 )
            {
                rgiBucket[ i ] = rgiBucket[ i - 1 ] + 1;
                TestCheck( rgiBucket[ i ] <= cBucketActual );
            }
        }

        for ( DWORD j = 0; j < cElements; j++ )
        {
            if ( rgres[ j ].m_dw == rgresActual[ i ].m_dw )
            {
                rgdwMapping[ i ] = j;
                break;
            }
        }
    }
    TestCheck( rgiBucket[ cElements - 1 ] == cBucketActual );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMoveNext( &lock ) );
    index.UnlockKeyPtr( &lock );

    index.MoveAfterLast( &lock );
    for ( DWORD i = 0; i < cElements; i++ )
    {
        const DWORD iT = cElements - i - 1;
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrMovePrev( &lock ) );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );

        if ( fHighRes )
        {
            TestCheck( pres->m_dw == rgres[ iT ].m_dw );
        }
        else
        {
            const DWORD iBucketExpected = rgres[ ( iT / cBucket ) * cBucket ].m_dw / dwKeyUncertainty;
            const DWORD iBucket = pres->m_dw / dwKeyUncertainty;
            TestCheck( ( iBucket == iBucketExpected ) ||
                       ( ( iBucket < iBucketExpected ) && ( ( iBucketExpected - iBucket ) == 1 ) ) ||
                       ( ( iBucket > iBucketExpected ) && ( ( iBucket - iBucketExpected ) == 1 ) ) );
        }

        TestCheck( pres->m_dw == rgresActual[ iT ].m_dw );
    }
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMovePrev( &lock ) );
    index.UnlockKeyPtr( &lock );

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.MoveBeforeKeyPtr( rgresActual[ i ].m_dw, &rgres[ rgdwMapping[ i ] ], &lock );
        TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );

        while ( ( errIndex = index.ErrMoveNext( &lock ) ) == CTestApproximateIndex::ERR::errSuccess )
        {
            TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
            TestCheck( index.CmpKey( pres->m_dw, rgresActual[ i ].m_dw ) >= 0 );
        }
        TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == errIndex );
        TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );

        index.UnlockKeyPtr( &lock );
    }

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.MoveAfterKeyPtr( rgresActual[ i ].m_dw, &rgres[ rgdwMapping[ i ] ], &lock );
        TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );

        while ( ( errIndex = index.ErrMovePrev( &lock ) ) == CTestApproximateIndex::ERR::errSuccess )
        {
            TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
            TestCheck( index.CmpKey( pres->m_dw, rgresActual[ i ].m_dw ) <= 0 );
        }
        TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == errIndex );
        TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );

        index.UnlockKeyPtr( &lock );
    }

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.MoveBeforeKeyPtr( rgresActual[ i ].m_dw, (CTestResource*)DWORD_PTR( 0 ), &lock );
        TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );

        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrMoveNext( &lock ) );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
        TestCheck( index.CmpKey( pres->m_dw, rgresActual[ i ].m_dw ) == 0 );

        if ( rgiBucket[ i ] == 1 )
        {
            TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMovePrev( &lock ) );
            TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
        }
        else
        {
            TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrMovePrev( &lock ) );
            TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
            TestCheck( index.CmpKey( pres->m_dw, rgresActual[ i ].m_dw ) < 0 );
        }

        index.UnlockKeyPtr( &lock );
    }

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.MoveAfterKeyPtr( rgresActual[ i ].m_dw, (CTestResource*)DWORD_PTR( -LONG_PTR( sizeof( CTestResource ) ) ), &lock );
        TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );

        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrMovePrev( &lock ) );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
        TestCheck( index.CmpKey( pres->m_dw, rgresActual[ i ].m_dw ) == 0 );

        if ( rgiBucket[ i ] == cBucketActual )
        {
            TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMoveNext( &lock ) );
            TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
        }
        else
        {
            TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrMoveNext( &lock ) );
            TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
            TestCheck( index.CmpKey( pres->m_dw, rgresActual[ i ].m_dw ) > 0 );
        }

        index.UnlockKeyPtr( &lock );
    }

    DWORD cCount;

    index.MoveAfterLast( &lock );
    while ( index.ErrMovePrev( &lock ) == CTestApproximateIndex::ERR::errSuccess );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMovePrev( &lock ) );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    index.UnlockKeyPtr( &lock );

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.LockKeyPtr( rgresOlder[ i ].m_dw, &rgresOlder[ i ], &lock );
        TestCheck( index.CmpKey( rgresOlder[ i ].m_dw, rgres[ 0 ].m_dw ) < 0 );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrInsertEntry( &lock, &rgresOlder[ i ], fAscending ) );
        TestCheck( index.CmpKey( rgresOlder[ i ].m_dw, rgres[ 0 ].m_dw ) < 0 );
        index.UnlockKeyPtr( &lock );
    }

    index.MoveBeforeKeyPtr( rgresActual[ 0 ].m_dw, (CTestResource*)DWORD_PTR( 0 ), &lock );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    cCount = 0;
    while ( index.ErrMovePrev( &lock ) == CTestApproximateIndex::ERR::errSuccess )
    {
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
        TestCheck( index.CmpKey( pres->m_dw, rgres[ 0 ].m_dw ) < 0 );
        cCount++;
    }
    TestCheck( cCount == cElements );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMovePrev( &lock ) );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    index.UnlockKeyPtr( &lock );

    index.MoveBeforeFirst( &lock );
    while ( index.ErrMoveNext( &lock ) == CTestApproximateIndex::ERR::errSuccess );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMoveNext( &lock ) );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    index.UnlockKeyPtr( &lock );

    for ( DWORD i = 0; i < cElements; i++ )
    {
        index.LockKeyPtr( rgresNewer[ i ].m_dw, &rgresNewer[ i ], &lock );
        TestCheck( index.CmpKey( rgresNewer[ i ].m_dw, rgres[ cElements - 1 ].m_dw ) > 0 );
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrInsertEntry( &lock, &rgresNewer[ i ], fAscending ) );
        TestCheck( index.CmpKey( rgresNewer[ i ].m_dw, rgres[ cElements - 1 ].m_dw ) > 0 );
        index.UnlockKeyPtr( &lock );
    }

    index.MoveAfterKeyPtr( rgresActual[ cElements - 1 ].m_dw, (CTestResource*)DWORD_PTR( -LONG_PTR( sizeof( CTestResource ) ) ), &lock );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    cCount = 0;
    while ( index.ErrMoveNext( &lock ) == CTestApproximateIndex::ERR::errSuccess )
    {
        TestCheck( CTestApproximateIndex::ERR::errSuccess == index.ErrRetrieveEntry( &lock, &pres ) );
        TestCheck( index.CmpKey( pres->m_dw, rgres[ cElements - 1 ].m_dw ) > 0 );
        cCount++;
    }
    TestCheck( cCount == cElements );
    TestCheck( CTestApproximateIndex::ERR::errNoCurrentEntry == index.ErrMoveNext( &lock ) );
    TestCheck( CTestApproximateIndex::ERR::errEntryNotFound == index.ErrRetrieveEntry( &lock, &pres ) );
    index.UnlockKeyPtr( &lock );
    
    index.Term();

HandleError:
    return err;
}
