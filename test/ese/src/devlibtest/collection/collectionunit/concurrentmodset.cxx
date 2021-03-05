// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

class ConcurrentModSetTest : public UNITTEST
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

    WaitForSingleObject( pMyData->hStartEvent, INFINITE );

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
        }
        else
        {
            TestCheck( CTestArray::ERR::errSuccess == pMyData->pArray->ErrInsert( &pMyData->rgElements[ index ] ) );
            pMyData->rgElements[ index ].fPlacedIn = fTrue;
            iInsert++;
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

    Sleep(10);

    wprintf( L"\t\tSignalling threads to start\n" );
    SetEvent( hStartEvent );

    if ( fUseEnumLock )
    {
        iEnumLock++;
        DWORD dwTickStart = GetTickCount();
        while ( ( GetTickCount() - dwTickStart) < C_TEST_MULTITHREAD_MSECS )
        {
            Array.LockForEnumeration();
            for (INT index1 = 0; index1 < Array.Count(); index1++ )
            {
                TestCheck( Array[ index1 ] != nullptr );
            }
            Array.UnlockForEnumeration();

            Sleep( rand() % ( C_TEST_MULTITHREAD_MSECS / 10 ) );
        }
    }
    else
    {
        Sleep( C_TEST_MULTITHREAD_MSECS );
    }

    DWORD dwTickEnd = GetTickCount();

    wprintf( L"\t\tSignalling threads to stop\n" );
    ResetEvent( hStartEvent );

    DWORD dwWaitResult = WaitForMultipleObjects( _countof( rghThreads ), rghThreads, TRUE, INFINITE );

    Array.LockForEnumeration();
    for ( INT index = 0; index < Array.Count(); index++ )
    {
        Array[ index ]->fFoundIn = fTrue;
    }
    Array.UnlockForEnumeration();

    for (INT index1 = 0; index1 < _countof( rgTestThreadData ); index1++ )
    {
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

ERR ConcurrentModSetTest::ErrTest()
{
    ERR                             err = JET_errSuccess;
    CTestArray                      Array( "TestArray", 1 );
    CArrayElement                   aeTestElement;
    RGConcurrentModSetTestThreadData rgTestThreadData;

    wprintf( L"\tTesting concurrently modifiable set ...\n");

    Array.EnableExtensiveValidation();
    
    Array.DisableAssertOnCountAndSize();


    wprintf( L"\t\tValidating constructor\n");
    TestCheck( Array.Count() == 0 );
    Array.LockForEnumeration();
    TestCheck( Array.Count() == 0 );
    TestCheck( Array[0] == nullptr );
    Array.UnlockForEnumeration();

#ifdef DEBUG
    wprintf( L"\t\tValidating locks\n");
    TestCheck( !Array.FLockedForModification() );
    TestCheck( !Array.FLockedForEnumeration() );
    TestCheck( !!Array.FNotLockedForModification() == !Array.FLockedForModification() );
    TestCheck( !!Array.FNotLockedForEnumeration() == !Array.FLockedForEnumeration() );

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

    wprintf( L"\t\tValidating first insertion\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 0 );
    TestCheck( CTestArray::ERR::errSuccess == Array.ErrInsert( &aeTestElement ) );
    TestCheck( Array.Count() == 1 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();


    wprintf( L"\t\tValidating first removal\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 1 );
    Array.Remove( &aeTestElement );
    TestCheck( Array.Count() == 0 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == nullptr );
    Array.UnlockForEnumeration();

    wprintf( L"\t\tValidating second insertion\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 0 );
    TestCheck( CTestArray::ERR::errSuccess == Array.ErrInsert( &aeTestElement ) );
    TestCheck( Array.Count() == 1 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();


    wprintf( L"\t\tValidating second removal\n");
    Array.LockForModification();
    TestCheck( Array.Count() == 1 );
    Array.Remove( &aeTestElement );
    TestCheck( Array.Count() == 0 );
    Array.UnlockForModification();
    Array.LockForEnumeration();
    TestCheck( Array[0] == nullptr );
    Array.UnlockForEnumeration();

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

    for (INT index = 0; index < _countof( rgTestThreadData ); index++ )
    {
        rgTestThreadData[ index ].pArray        = &Array;
        rgTestThreadData[ index ].iThreadNum    = index;
    }

    TestCheck( JET_errSuccess == ConcurrentModSetTestMultithreadedAccess( Array, rgTestThreadData, fFalse ) );

    TestCheck( JET_errSuccess == ConcurrentModSetTestMultithreadedAccess( Array, rgTestThreadData, fFalse ) );

    TestCheck( JET_errSuccess == ConcurrentModSetTestMultithreadedAccess( Array, rgTestThreadData, fTrue ) );

    wprintf( L"\t\tValidating first element hasn't changed\n");
    Array.LockForEnumeration();
    TestCheck( Array.Count() >= 1 );
    TestCheck( Array[0] == &aeTestElement );
    Array.UnlockForEnumeration();

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
    wprintf( L"\t\tMake sure it's empty and that it's been shrunk.\n" );
    INT iSize = Array.Size();
    Array.LockForEnumeration();
    TestCheck( iSize >= Array.Size() );
    TestCheck( 0 == Array.Count() );
    TestCheck( nullptr == Array[ 0 ] );
    Array.UnlockForEnumeration();

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




HandleError:

    return err;
}



