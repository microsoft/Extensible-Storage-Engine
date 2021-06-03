// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

//  ================================================================
class ConcurrentModSetTest : public UNITTEST
//  ================================================================
{
    private:
        static ConcurrentModSetTest s_instance;

    protected:
        ConcurrentModSetTest() {}
    public:
        ~ConcurrentModSetTest() {}
    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

ConcurrentModSetTest ConcurrentModSetTest::s_instance;

const char * ConcurrentModSetTest::SzName() const       { return "ConcurrentModSet"; };
const char * ConcurrentModSetTest::SzDescription() const    { return 
            "Tests the collection libraries' concurrent modifiable set facility.";
        }
bool ConcurrentModSetTest::FRunUnderESE98() const       { return true; }
bool ConcurrentModSetTest::FRunUnderESENT() const       { return true; }
bool ConcurrentModSetTest::FRunUnderESE97() const       { return true; }

#define C_TEST_THREADS             10
#define C_TEST_ELEMENTS           100
#define C_TEST_MULTITHREAD_MSECS 1000

typedef class _tagCArrayElement
{
    public:
        BOOL    fPlacedIn;
        BOOL    fFoundIn;
        static SIZE_T OffsetOfIAE() { return OffsetOf( _tagCArrayElement, m_iae ); }

        CInvasiveConcurrentModSet< _tagCArrayElement, _tagCArrayElement::OffsetOfIAE>::CElement m_iae;

        _tagCArrayElement() { memset(this, 0, sizeof(*this) ); }
} CArrayElement;

typedef CInvasiveConcurrentModSet< CArrayElement, CArrayElement::OffsetOfIAE > CTestArray;

typedef struct _tagSConcurrentModSetWorkerData
{
    CTestArray    *pArray;
    INT           iThreadNum;
    HANDLE        hStartEvent;
    ERR           err;
    CArrayElement rgElements[C_TEST_ELEMENTS];
} SConcurrentModSetWorkerData;

typedef SConcurrentModSetWorkerData RGConcurrentModSetTestThreadData[ C_TEST_THREADS ];


DWORD WINAPI ConcurrentModSetWorker( LPVOID lpParameter )
{
    ERR                        err      = JET_errSuccess;
    SConcurrentModSetWorkerData *pMyData = ( SConcurrentModSetWorkerData *)lpParameter;
    INT                        iRemove = 0;
    INT                        iInsert = 0;
    INT                        iModLockCount = 0;
    //wprintf( L"\t\t\tThread %d present and accounted for pMyData=%p\n", pMyData->iThreadNum, pMyData );

    WaitForSingleObject( pMyData->hStartEvent, INFINITE );
    // wprintf( L"\t\t\tThread %d is off!\n", pMyData->iThreadNum );

    for (;;)
    {
        pMyData->pArray->LockForModification();
        iModLockCount++;
        INT index = rand() % _countof( pMyData->rgElements );
        if ( pMyData->rgElements[ index ].fPlacedIn )
        {
            pMyData->pArray->Remove( &pMyData->rgElements[ index ] );
            pMyData->rgElements[ index ].fPlacedIn = fFalse;
            iRemove++;
            // wprintf( L"\t\t\tThread %d iteration %d removed item %d\n", pMyData->iThreadNum, iModLockCount, index );
        }
        else
        {
            TestCheck( CTestArray::ERR::errSuccess == pMyData->pArray->ErrInsert( &pMyData->rgElements[ index ] ) );
            pMyData->rgElements[ index ].fPlacedIn = fTrue;
            iInsert++;
            // wprintf( L"\t\t\tThread %d iteration %d added item %d\n", pMyData->iThreadNum, iModLockCount, index );
        }
        pMyData->pArray->UnlockForModification();

        DWORD dwWaitRes =  WaitForSingleObject( pMyData->hStartEvent, 0 );
        if ( ERROR_SUCCESS != dwWaitRes )
        {
            wprintf( L"\t\t\tThread %d signing off (Inserted=%d, Removed=%d)\n",
                     pMyData->iThreadNum,
                     iInsert,
                     iRemove
                );
            return ERROR_SUCCESS;
        }
    }
HandleError:
    wprintf( L"\t\t\tThread %d errored! (error:%d)\n", pMyData->iThreadNum, err );
    pMyData->err = err;
    return ERROR_SUCCESS;
}


ERR ConcurrentModSetTestMultithreadedAccess(
    CTestArray &Array,
    RGConcurrentModSetTestThreadData &rgTestThreadData,
    BOOL       fUseEnumLock
    )
{
    ERR    err         = JET_errSuccess;
    INT    iEnumLock   = 0;
    HANDLE hStartEvent = CreateEventA( nullptr, TRUE, FALSE, nullptr );
    HANDLE rghThreads[ _countof( rgTestThreadData ) ];

    wprintf( L"\t\tRunning multi-threaded modification test%s.\n", ( fUseEnumLock ? L" with interspersed enumerations" : L"" ) );

    // Reset the test data.
    for (INT index1 = 0; index1 < _countof( rgTestThreadData ); index1++ )
    {
        rgTestThreadData[ index1 ].hStartEvent   = hStartEvent;
        rgTestThreadData[ index1 ].err           = JET_errSuccess;

        for ( INT index2 = 0; index2 < _countof( rgTestThreadData[ index1 ].rgElements ); index2++ )
        {
            rgTestThreadData[ index1 ].rgElements[ index2 ].fFoundIn = fFalse;
        }
    }

    wprintf( L"\t\tCreating %d test threads\n", _countof( rghThreads ));
    for ( INT index = 0; index < _countof( rghThreads ); index++)
    {
        rghThreads[ index ] = CreateThread( nullptr, 0, ConcurrentModSetWorker, &(rgTestThreadData[ index ]), 0, nullptr );
        TestCheck( rghThreads[ index ] != nullptr );
    }

    // Give the other threads a chance to wake up.
    Sleep(10);

    // Tell the threads to start whacking on the array.
    wprintf( L"\t\tSignalling threads to start\n" );
    SetEvent( hStartEvent );

    if ( fUseEnumLock )
    {
        iEnumLock++;
        DWORD dwTickStart = GetTickCount();
        // Let them whack for a while.
        while ( ( GetTickCount() - dwTickStart) < C_TEST_MULTITHREAD_MSECS )
        {
            // But interrupt those threads occasionally by looking at the array once in a while.
            Array.LockForEnumeration();
            for (INT index1 = 0; index1 < Array.Count(); index1++ )
            {
                TestCheck( Array[ index1 ] != nullptr );
            }
            Array.UnlockForEnumeration();

            // We will overshoot the test time by up to 10% )
            Sleep( rand() % ( C_TEST_MULTITHREAD_MSECS / 10 ) );
        }
    }
    else
    {
        Sleep( C_TEST_MULTITHREAD_MSECS );
    }

    DWORD dwTickEnd = GetTickCount();

    // Tell the threads to stop whacking on the array.
    wprintf( L"\t\tSignalling threads to stop\n" );
    ResetEvent( hStartEvent );

    // Wait for the threads to stop whacking.
    DWORD dwWaitResult = WaitForMultipleObjects( _countof( rghThreads ), rghThreads, TRUE, INFINITE );

    // Spin through the Array and mark all the elements that are in it.
    Array.LockForEnumeration();
    for ( INT index = 0; index < Array.Count(); index++ )
    {
        Array[ index ]->fFoundIn = fTrue;
    }
    Array.UnlockForEnumeration();

    // Now, walk through the test data and make sure that all the bookkeeping lines up.
    for (INT index1 = 0; index1 < _countof( rgTestThreadData ); index1++ )
    {
        // Noone should have failed.
        TestCheck( rgTestThreadData[ index1 ].err == JET_errSuccess );

        for ( INT index2 = 0; index2 < _countof( rgTestThreadData[ index1 ].rgElements ); index2++ )
        {
            CArrayElement *pElement = &(rgTestThreadData[ index1 ].rgElements[ index2 ]);
            TestCheck( !!pElement->fFoundIn == !!pElement->fPlacedIn );
        }
    }

    wprintf( L"\t\tMulti-threaded access test complete (enumerated:%d).\n", iEnumLock );

HandleError:
    CloseHandle( hStartEvent );
    return err;
}

//  ================================================================
ERR ConcurrentModSetTest::ErrTest()
//  ================================================================
{
    ERR                             err = JET_errSuccess;
    CTestArray                      Array( "TestArray", 1 );
    CArrayElement                   aeTestElement;
    RGConcurrentModSetTestThreadData rgTestThreadData;

    wprintf( L"\tTesting concurrently modifiable set ...\n");

    // Turn on the extensive and time-consuming validation for the array.
    Array.EnableExtensiveValidation();
    
    // Turn off the assert in Count() that checks for the Enumeration lock.
    Array.DisableAssertOnCountAndSize();

    // The constructor should create an unlocked array with nothing in it.

    wprintf( L"\t\tValidating constructor\n");
    TestCheck( Array.Count() == 0 );
    Array.LockForEnumeration();
    TestCheck( Array.Count() == 0 );
    TestCheck( Array[0] == nullptr );
    Array.UnlockForEnumeration();

    // The methods to report on the state of the locks only exist in DEBUG builds.
#ifdef DEBUG
    wprintf( L"\t\tValidating locks\n");
    // Check that at initialization, no locks are taken.
    TestCheck( !Array.FLockedForModification() );
    TestCheck( !Array.FLockedForEnumeration() );
    TestCheck( !!Array.FNotLockedForModification() == !Array.FLockedForModification() );
    TestCheck( !!Array.FNotLockedForEnumeration() == !Array.FLockedForEnumeration() );

    // Check that the enumeration lock works.
    Array.LockForEnumeration();
    TestCheck( !Array.FLockedForModification() );
    TestCheck( Array.FLockedForEnumeration() );
    TestCheck( !!Array.FNotLockedForModification() == !Array.FLockedForModification() );
    TestCheck( !!Array.FNotLockedForEnumeration() == !Array.FLockedForEnumeration() );

    Array.UnlockForEnumeration();
    TestCheck( !Array.FLockedForModification() );
    TestCheck( !Array.FLockedForEnumeration() );
    TestCheck( !!Array.FNotLockedForModification() == !Array.FLockedForModification() );
    TestCheck( !!Array.FNotLockedForEnumeration() == !Array.FLockedForEnumeration() );

    // Check that the modificaiton lock works.
    Array.LockForModification();
    TestCheck( Array.FLockedForModification() );
    TestCheck( !Array.FLockedForEnumeration() );
    TestCheck( !!Array.FNotLockedForModification() == !Array.FLockedForModification() );
    TestCheck( !!Array.FNotLockedForEnumeration() == !Array.FLockedForEnumeration() );

    Array.UnlockForModification();
    TestCheck( !Array.FLockedForModification() );
    TestCheck( !Array.FLockedForEnumeration() );
    TestCheck( !!Array.FNotLockedForModification() == !Array.FLockedForModification() );
    TestCheck( !!Array.FNotLockedForEnumeration() == !Array.FLockedForEnumeration() );

#endif

    // Add 1 element.
    wprintf( L"\t\tValidating first insertion\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 0 );
    TestCheck( CTestArray::ERR::errSuccess == Array.ErrInsert( &aeTestElement ) );
    TestCheck( Array.Count() == 1 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();


    // Remove 1 element.
    wprintf( L"\t\tValidating first removal\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 1 );
    Array.Remove( &aeTestElement );
    TestCheck( Array.Count() == 0 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == nullptr );
    Array.UnlockForEnumeration();

    // Re-add same element.
    wprintf( L"\t\tValidating second insertion\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 0 );
    TestCheck( CTestArray::ERR::errSuccess == Array.ErrInsert( &aeTestElement ) );
    TestCheck( Array.Count() == 1 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();


    // Re-remove same element.
    wprintf( L"\t\tValidating second removal\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 1 );
    Array.Remove( &aeTestElement );
    TestCheck( Array.Count() == 0 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == nullptr );
    Array.UnlockForEnumeration();

    // Add it back in one more time.  There's a contract that says that if someone
    // takes the enumeration lock and looks at the first element in the array, that
    // pins the first element to remain the first element, regardless of any additions
    // or removals that happen after that.  Sort of like quantum mechanics; the act
    // of observing the array collapses the probabilities into a single certainty.
    wprintf( L"\t\tValidating second insertion\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 0 );
    TestCheck( CTestArray::ERR::errSuccess == Array.ErrInsert( &aeTestElement ) );
    TestCheck( Array.Count() == 1 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();
    aeTestElement.fPlacedIn = fTrue;

    //
    // Initialize the data structure used in the multi-threaded tests below.
    // Passes don't empty out the array, so we need to keep track of what is
    // supposed to be in or out of the array at this scope.
    for (INT index = 0; index < _countof( rgTestThreadData ); index++ )
    {
        rgTestThreadData[ index ].pArray        = &Array;
        rgTestThreadData[ index ].iThreadNum    = index;
    }

    //
    // First pass of multithreading solely uses the modification lock in the worker threads.
    // This pass will grow the array significantly, which takes a write-lock in the worker
    // threads as they grow the array.
    //
    TestCheck( JET_errSuccess == ConcurrentModSetTestMultithreadedAccess( Array, rgTestThreadData, fFalse ) );

    //
    // Second pass of multithreading solely uses the modification lock in the worker threads.
    // The array should be mostly grown, so this pass should mostly use a read-lock.
    //
    TestCheck( JET_errSuccess == ConcurrentModSetTestMultithreadedAccess( Array, rgTestThreadData, fFalse ) );

    //
    // Final pass of multithreading uses the modification lock in the worker threads AND
    // uses Enumeration in the main test thread.
    //
    TestCheck( JET_errSuccess == ConcurrentModSetTestMultithreadedAccess( Array, rgTestThreadData, fTrue ) );

    // Finally, verify that the first element is still the first element.  Other than that, there is no guaranteed
    // order.
    wprintf( L"\t\tValidating first element hasn't changed\n");
    Array.LockForEnumeration();
    TestCheck( Array.Count() >= 1 );
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();

    //
    // Now, drain the array
    //
    wprintf( L"\t\tDrain the array and make sure it's empty.\n" );
    Array.LockForModification();
    Array.Remove( &aeTestElement );
    aeTestElement.fPlacedIn = fFalse;
    for (INT index1 = 0; index1 < _countof( rgTestThreadData ); index1++ )
    {
        for ( INT index2 = 0; index2 < _countof( rgTestThreadData[ index1 ].rgElements ); index2++ )
        {
            CArrayElement *pElement = &(rgTestThreadData[ index1 ].rgElements[ index2 ]);
            if ( pElement->fPlacedIn )
            {
                Array.Remove( pElement );
                pElement->fPlacedIn = fFalse;
            }
        }
    }
    Array.UnlockForModification();
    //
    // Since the array should be empty, this enumeration lock should also trigger shrinking.
    //
    wprintf( L"\t\tMake sure it's empty and that it's been shrunk.\n" );
    INT iSize = Array.Size();
    Array.LockForEnumeration();
    TestCheck( iSize >= Array.Size() ); // Shrink happened.
    TestCheck( 0 == Array.Count() );
    TestCheck( nullptr == Array[ 0 ] );
    Array.UnlockForEnumeration();

    //
    // One more time to make sure the shrinking didn't break anything.
    //
    // Add 1 element.
    wprintf( L"\t\tValidating final insertion\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 0 );
    TestCheck( CTestArray::ERR::errSuccess == Array.ErrInsert( &aeTestElement ) );
    TestCheck( Array.Count() == 1 );
    aeTestElement.fPlacedIn = fTrue;
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();


    //
    //  There are a number of negative test cases that can't be tested because the
    // collection class has undefined behavior for inappropriate usage.  The class
    // expects the callers to be well-behaved.
    //
    // * Can't test accessing anything more than 1 element past the end of the array.
    //   In the current implementation, it actually works in that it returns nullptr,
    //   but Asserts and hence would fail the test.
    //
    // * Can't test removing an invalid element (random pointer, nullptr, pointer to
    //   already removed element).  It'll most likely Assert in a debug build and
    //   probably corrupt some memory somewhere in retail.
    //
    // * Can't test inserting/removing without the modification lock or reading without
    //   the enumeration lock.  The debug build will assert, the retail build will
    //   silently do the modification, probably corrupting things in the face of contention.
    //


HandleError:

    return err;
}



