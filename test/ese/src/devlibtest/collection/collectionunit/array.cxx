// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

class ArrayTest : public UNITTEST
{
    private:
        static ArrayTest s_instance;

    protected:
        ArrayTest() {}
    public:
        ~ArrayTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

ArrayTest ArrayTest::s_instance;

const char * ArrayTest::SzName() const          { return "Array"; };
const char * ArrayTest::SzDescription() const   { return  "Tests the collection libraries' array (CArray) facility."; }
bool ArrayTest::FRunUnderESE98() const          { return true; }
bool ArrayTest::FRunUnderESENT() const          { return true; }
bool ArrayTest::FRunUnderESE97() const          { return true; }

#ifndef OffsetOf
#define OffsetOf(s,m)   ((SIZE_T)&(((s *)0)->m))
#endif

const INT g_defaultElement  = 123456;
const INT g_elementMult     = 3;

INT __cdecl CmpFunction( const INT* pvEntry1, const INT* pvEntry2 )
{
    return *pvEntry1 - *pvEntry2;
}

bool FTestArraySortAndSearch( CArray<INT>& array, const INT* const rgiUnsorted, const INT* const rgiSorted, const size_t ci, const INT* const rgiSortedNoDupes, const size_t ciNoDupes )
{
    ERR err = JET_errSuccess;

    const INT iMustNotBeFound = g_defaultElement;

    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 0 ) );
    TestCheck( 0 == array.Size() );


    TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchBinary( iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( &iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchBinary( &iMustNotBeFound, CmpFunction ) );




    for ( size_t i = 0; i < ci; i++ )
    {
        const INT iKey = rgiUnsorted[ i ];
        TestCheck( iMustNotBeFound != iKey );
        TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetEntry( i, iKey ) );
        TestCheck( ( i + 1 ) == array.Size() );
        TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( iMustNotBeFound, CmpFunction ) );
    }


    for ( size_t i = 0; i < ci; i++ )
    {
        const INT iKey = rgiUnsorted[ i ];
        TestCheck( iKey == array.Entry( i ) );
        const size_t iKeyFound = array.SearchLinear( iKey, CmpFunction );
        TestCheck( CArray<INT>::iEntryNotFound != iKeyFound );
        TestCheck( iKey == array.Entry( iKeyFound ) );
    }
    TestCheck( ci == array.Size() );


    array.Sort( CmpFunction );
    TestCheck( ci == array.Size() );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchBinary( iMustNotBeFound, CmpFunction ) );


    for ( size_t i = 0; i < ci; i++ )
    {
        const INT iKey = rgiSorted[ i ];
        TestCheck( iKey == array.Entry( i ) );
    }
    TestCheck( ci == array.Size() );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchBinary( iMustNotBeFound, CmpFunction ) );


    array.SortAndRemoveDuplicates( CmpFunction );
    TestCheck( ciNoDupes == array.Size() );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchBinary( iMustNotBeFound, CmpFunction ) );


    for ( size_t i = 0; i < ciNoDupes; i++ )
    {
        const INT iKey = rgiSortedNoDupes[ i ];
        TestCheck( iKey == array.Entry( i ) );
    }
    TestCheck( ciNoDupes == array.Size() );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchLinear( iMustNotBeFound, CmpFunction ) );
    TestCheck( CArray<INT>::iEntryNotFound == array.SearchBinary( iMustNotBeFound, CmpFunction ) );


    for ( size_t i = 0; i < ciNoDupes; i++ )
    {
        const INT iKey = rgiUnsorted[ i ];

        size_t iLinear = array.SearchLinear( iKey, CmpFunction );
        size_t iBinary = array.SearchBinary( iKey, CmpFunction );
        TestCheck( CArray<INT>::iEntryNotFound != iLinear );
        TestCheck( iKey == array.Entry( iLinear ) );
        TestCheck( CArray<INT>::iEntryNotFound != iBinary );
        TestCheck( iKey == array.Entry( iBinary ) );

        iLinear = array.SearchLinear( &iKey, CmpFunction );
        iBinary = array.SearchBinary( &iKey, CmpFunction );
        TestCheck( iKey == array.Entry( iLinear ) );
        TestCheck( iKey == array.Entry( iBinary ) );
    }
    TestCheck( ciNoDupes == array.Size() );

HandleError:

    if ( err != JET_errSuccess )
    {
        return false;
    }

    return true;
}

ERR ArrayTest::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting array (CArray) ...\n");

    BstfSetVerbosity(bvlPrintTests-1);

    CArray<INT> array( 2 );
    CArray<INT> arrayClone;


    TestCheck( 0 == array.Size() );
    TestCheck( 0 == array.Capacity() );
    TestCheck( NULL == array.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetCapacity( 1 ) );
    TestCheck( 0 == array.Size() );
    TestCheck( 1 == array.Capacity() );
    TestCheck( NULL == array.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetCapacity( 10 ) );
    TestCheck( 0 == array.Size() );
    TestCheck( 10 == array.Capacity() );
    TestCheck( NULL == array.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errOutOfMemory == array.ErrSetCapacity( SIZE_MAX ) );
    TestCheck( 0 == array.Size() );
    TestCheck( 10 == array.Capacity() );
    TestCheck( NULL == array.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 5 ) );
    TestCheck( 5 == array.Size() );
    TestCheck( 10 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 4 ) );
    TestCheck( NULL == array.PEntry( 5 ) );


    TestCheck( CArray<INT>::ERR::errOutOfMemory == array.ErrSetSize( SIZE_MAX ) );
    TestCheck( 5 == array.Size() );
    TestCheck( 10 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 4 ) );
    TestCheck( NULL == array.PEntry( 5 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 13 ) );
    TestCheck( 13 == array.Size() );
    TestCheck( 14 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 12 ) );
    TestCheck( NULL == array.PEntry( 13 ) );
    TestCheck( NULL == array.PEntry( 14 ) );


    array.SetCapacityGrowth( 6 );
    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 17 ) );
    TestCheck( 17 == array.Size() );
    TestCheck( 20 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 16 ) );
    TestCheck( NULL == array.PEntry( 17 ) );
    TestCheck( NULL == array.PEntry( 18 ) );


    array.SetCapacityGrowth( 0 );
    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 29 ) );
    TestCheck( 29 == array.Size() );
    TestCheck( 29 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 28 ) );
    TestCheck( NULL == array.PEntry( 29 ) );
    TestCheck( NULL == array.PEntry( 30 ) );


    array.SetCapacityGrowth( 7 );
    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 40 ) );
    TestCheck( 40 == array.Size() );
    TestCheck( 43 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 39 ) );
    TestCheck( NULL == array.PEntry( 40 ) );
    TestCheck( NULL == array.PEntry( 41 ) );

    
    for ( size_t iElement = 0; iElement < array.Size(); iElement++ )
    {
        const size_t iSetType = iElement % 3;
        const INT element = (INT)( g_elementMult * iElement );

        if ( iSetType == 0 )
        {
            array.SetEntry( array.PEntry( iElement ), element );
        }
        else if ( iSetType == 1 )
        {
            TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetEntry( iElement, element ) );
        }
        else if ( iSetType == 2 )
        {
            array[ iElement ] = element;
        }
    }


    TestCheck( CArray<INT>::ERR::errOutOfMemory == array.ErrSetCapacity( SIZE_MAX ) );
    TestCheck( 40 == array.Size() );
    TestCheck( 43 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 39 ) );
    TestCheck( NULL == array.PEntry( 40 ) );
    TestCheck( NULL == array.PEntry( 41 ) );

    
    for ( size_t iElement = 0; iElement < array.Size(); iElement++ )
    {
        const bool fGetPointer = ( iElement % 2 ) != 0;
        INT element = fGetPointer ? *( array.PEntry( iElement ) ) : array.Entry( iElement );
        const INT elementT = (INT)( g_elementMult * iElement );
        TestCheck( element == elementT );
    }


    TestCheck( 40 == array.Size() );
    TestCheck( 43 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 39 ) );
    TestCheck( NULL == array.PEntry( 40 ) );
    TestCheck( NULL == array.PEntry( 41 ) );


    array.SetEntryDefault( g_defaultElement );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetEntry( 64, g_elementMult * 64 ) );
    TestCheck( 65 == array.Size() );
    TestCheck( 71 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 64 ) );
    TestCheck( NULL == array.PEntry( 65 ) );
    TestCheck( NULL == array.PEntry( 66 ) );
    TestCheck( ( g_elementMult * 64 ) == array.Entry( 64 ) );

    
    for ( size_t iElement = 40; iElement < array.Size() - 1; iElement++ )
    {
        const bool fGetPointer = ( iElement % 2 ) == 0;
        INT element = fGetPointer ? *( array.PEntry( iElement ) ) : array.Entry( iElement );
        TestCheck( element == g_defaultElement );
    }


    TestCheck( 0 == arrayClone.Size() );
    TestCheck( 0 == arrayClone.Capacity() );
    TestCheck( NULL == arrayClone.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == arrayClone.ErrSetCapacity( 1 ) );
    TestCheck( 0 == arrayClone.Size() );
    TestCheck( 1 == arrayClone.Capacity() );
    TestCheck( NULL == arrayClone.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == arrayClone.ErrSetCapacity( 10 ) );
    TestCheck( 0 == arrayClone.Size() );
    TestCheck( 10 == arrayClone.Capacity() );
    TestCheck( NULL == arrayClone.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errOutOfMemory == arrayClone.ErrSetCapacity( SIZE_MAX ) );
    TestCheck( 0 == arrayClone.Size() );
    TestCheck( 10 == arrayClone.Capacity() );
    TestCheck( NULL == arrayClone.PEntry( 0 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == arrayClone.ErrSetSize( 5 ) );
    TestCheck( 5 == arrayClone.Size() );
    TestCheck( 10 == arrayClone.Capacity() );
    TestCheck( NULL != arrayClone.PEntry( 0 ) );
    TestCheck( NULL != arrayClone.PEntry( 4 ) );
    TestCheck( NULL == arrayClone.PEntry( 5 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == arrayClone.ErrSetSize( 13 ) );
    TestCheck( 13 == arrayClone.Size() );
    TestCheck( 13 == arrayClone.Capacity() );
    TestCheck( NULL != arrayClone.PEntry( 0 ) );
    TestCheck( NULL != arrayClone.PEntry( 12 ) );
    TestCheck( NULL == arrayClone.PEntry( 13 ) );
    TestCheck( NULL == arrayClone.PEntry( 14 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == arrayClone.ErrClone( array ) );
    TestCheck( 65 == arrayClone.Size() );
    TestCheck( 71 == arrayClone.Capacity() );
    TestCheck( NULL != arrayClone.PEntry( 0 ) );
    TestCheck( NULL != arrayClone.PEntry( 64 ) );
    TestCheck( NULL == arrayClone.PEntry( 65 ) );
    for ( size_t iElement = 0; iElement < 40; iElement++ )
    {
        const INT element = (INT)( g_elementMult * iElement );
        TestCheck( arrayClone.Entry( iElement ) == element );
        TestCheck( arrayClone.Entry( iElement ) == array.Entry( iElement ) );
        TestCheck( array[ iElement ] == array.Entry( iElement ) );
        TestCheck( arrayClone[ iElement ] == arrayClone.Entry( iElement ) );
    }
    TestCheck( g_defaultElement == arrayClone.Entry( 41 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 50 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 63 ) );
    TestCheck( ( g_elementMult * 64 ) == arrayClone.Entry( 64 ) );


    TestCheck( 65 == array.Size() );
    TestCheck( 71 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 64 ) );
    TestCheck( NULL == array.PEntry( 65 ) );
    TestCheck( ( g_elementMult * 64 ) == array.Entry( 64 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == arrayClone.ErrSetEntry( 77, g_elementMult * 77 ) );
    TestCheck( 78 == arrayClone.Size() );
    TestCheck( 78 == arrayClone.Capacity() );
    TestCheck( NULL != arrayClone.PEntry( 0 ) );
    TestCheck( NULL != arrayClone.PEntry( 77 ) );
    TestCheck( NULL == arrayClone.PEntry( 78 ) );
    TestCheck( ( g_elementMult * 77 ) == arrayClone.Entry( 77 ) );

    
    for ( size_t iElement = 65; iElement < arrayClone.Size() - 1; iElement++ )
    {
        const bool fGetPointer = ( iElement % 2 ) != 0;
        INT element = fGetPointer ? *( arrayClone.PEntry( iElement ) ) : arrayClone.Entry( iElement );
        TestCheck( element == g_defaultElement );
    }


    for ( size_t iElement = 0; iElement < 40; iElement++ )
    {
        const INT element = (INT)( g_elementMult * iElement );
        TestCheck( array.Entry( iElement ) == element );
    }
    TestCheck( 65 == array.Size() );
    TestCheck( 71 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 64 ) );
    TestCheck( NULL == array.PEntry( 65 ) );
    TestCheck( ( g_elementMult * 64 ) == array.Entry( 64 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 33 ) );
    TestCheck( 33 == array.Size() );
    TestCheck( 71 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 32 ) );
    TestCheck( NULL == array.PEntry( 33 ) );
    for ( size_t iElement = 0; iElement < array.Size(); iElement++ )
    {
        const bool fGetPointer = ( iElement % 2 ) == 0;
        INT element = fGetPointer ? *( array.PEntry( iElement ) ) : array.Entry( iElement );
        const INT elementT = (INT)( g_elementMult * iElement );
        TestCheck( element == elementT );
    }


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetCapacity( 19 ) );
    TestCheck( 19 == array.Size() );
    TestCheck( 19 == array.Capacity() );
    TestCheck( NULL != array.PEntry( 0 ) );
    TestCheck( NULL != array.PEntry( 18 ) );
    TestCheck( NULL == array.PEntry( 19 ) );
    for ( size_t iElement = 0; iElement < array.Size(); iElement++ )
    {
        const bool fGetPointer = ( iElement % 2 ) != 0;
        INT element = fGetPointer ? *( array.PEntry( iElement ) ) : array.Entry( iElement );
        const INT elementT = (INT)( g_elementMult * iElement );
        TestCheck( element == elementT );
    }


    TestCheck( 78 == arrayClone.Size() );
    TestCheck( 78 == arrayClone.Capacity() );
    TestCheck( NULL != arrayClone.PEntry( 0 ) );
    TestCheck( NULL != arrayClone.PEntry( 77 ) );
    TestCheck( NULL == arrayClone.PEntry( 78 ) );
    TestCheck( 0 == arrayClone.Entry( 0 ) );
    TestCheck( ( g_elementMult * 13 ) == arrayClone.Entry( 13 ) );
    TestCheck( ( g_elementMult * 39 ) == arrayClone.Entry( 39 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 40 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 51 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 63 ) );
    TestCheck( ( g_elementMult * 64 ) == arrayClone.Entry( 64 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 65 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 70 ) );
    TestCheck( g_defaultElement == arrayClone.Entry( 76 ) );
    TestCheck( ( g_elementMult * 77 ) == arrayClone.Entry( 77 ) );


    TestCheck( CArray<INT>::ERR::errSuccess == array.ErrSetSize( 0 ) );
    TestCheck( 0 == array.Size() );


    TestCheck( FTestArraySortAndSearch( array, NULL, NULL, 0, NULL, 0 ) );


    const INT rgiUnsorted1[]    = { 10 };
    const INT rgiSorted1[]      = { 10 };
    const size_t ci1 = _countof( rgiSorted1 );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted1, rgiSorted1, ci1, rgiSorted1, ci1 ) );


    const INT rgiUnsorted2Sorted[]  = { 10, 20 };
    const INT rgiSorted2Sorted[]    = { 10, 20 };
    const size_t ci2Sorted = _countof( rgiSorted2Sorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted2Sorted, rgiSorted2Sorted, ci2Sorted, rgiSorted2Sorted, ci2Sorted ) );


    const INT rgiUnsorted2Unsorted[]    = { 20, 10 };
    const INT rgiSorted2Unsorted[]      = { 10, 20 };
    const size_t ci2Unsorted = _countof( rgiSorted2Unsorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted2Unsorted, rgiSorted2Unsorted, ci2Unsorted, rgiSorted2Unsorted, ci2Unsorted ) );


    const INT rgiUnsorted10Sorted[] = { 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const INT rgiSorted10Sorted[]   = { 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci10Sorted = _countof( rgiSorted10Sorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted10Sorted, rgiSorted10Sorted, ci10Sorted, rgiSorted10Sorted, ci10Sorted ) );


    const INT rgiUnsorted10InvSorted[]  = { 13, 12, 11, 10, 8, 6, 5, 2, 1 };
    const INT rgiSorted10InvSorted[]    = { 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci10InvSorted = _countof( rgiSorted10InvSorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted10InvSorted, rgiSorted10InvSorted, ci10InvSorted, rgiSorted10InvSorted, ci10InvSorted ) );


    const INT rgiUnsorted10Unsorted[]   = { 5, 13, 10, 11, 8, 6, 12, 1, 2 };
    const INT rgiSorted10Unsorted[]     = { 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci10Unsorted = _countof( rgiSorted10Unsorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted10Unsorted, rgiSorted10Unsorted, ci10Unsorted, rgiSorted10Unsorted, ci10Unsorted ) );


    const INT rgiUnsorted10UnsortedDup[]        = { 5, 13, 13, 8, 10, 13, 11, 8, 6, 12, 8, 1, 2, 1, 1, 12, 1 };
    const INT rgiSorted10UnsortedDup[]          = { 1, 1, 1, 1, 2, 5, 6, 8, 8, 8, 10, 11, 12, 12, 13, 13, 13 };
    const INT rgiSorted10UnsortedDupUnique[]    = { 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci10UnsortedDup        = _countof( rgiSorted10UnsortedDup );
    const size_t ci10UnsortedDupUnique  = _countof( rgiSorted10UnsortedDupUnique );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted10UnsortedDup, rgiSorted10UnsortedDup, ci10UnsortedDup, rgiSorted10UnsortedDupUnique, ci10UnsortedDupUnique ) );


    const INT rgiUnsorted11InvSorted[]  = { 13, 12, 11, 10, 8, 6, 5, 2, 1, -1 };
    const INT rgiSorted11InvSorted[]    = { -1, 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci11InvSorted = _countof( rgiSorted11InvSorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted11InvSorted, rgiSorted11InvSorted, ci11InvSorted, rgiSorted11InvSorted, ci11InvSorted ) );


    const INT rgiUnsorted11Unsorted[]   = { 5, 13, 10, 11, -1, 8, 6, 12, 1, 2 };
    const INT rgiSorted11Unsorted[]     = { -1, 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci11Unsorted = _countof( rgiSorted11Unsorted );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted11Unsorted, rgiSorted11Unsorted, ci11Unsorted, rgiSorted11Unsorted, ci11Unsorted ) );


    const INT rgiUnsorted11UnsortedDup[]        = { 5, 13, 13, 8, 10, 13, 11, -1, -1, -1, 8, 6, 12, 8, 1, 2, 1, 1, 12, 1 };
    const INT rgiSorted11UnsortedDup[]          = { -1, -1, -1, 1, 1, 1, 1, 2, 5, 6, 8, 8, 8, 10, 11, 12, 12, 13, 13, 13 };
    const INT rgiSorted11UnsortedDupUnique[]    = { -1, 1, 2, 5, 6, 8, 10, 11, 12, 13 };
    const size_t ci11UnsortedDup        = _countof( rgiSorted11UnsortedDup );
    const size_t ci11UnsortedDupUnique  = _countof( rgiSorted11UnsortedDupUnique );
    TestCheck( FTestArraySortAndSearch( array, rgiUnsorted11UnsortedDup, rgiSorted11UnsortedDup, ci11UnsortedDup, rgiSorted11UnsortedDupUnique, ci11UnsortedDupUnique ) );

HandleError:

    return err;
}


