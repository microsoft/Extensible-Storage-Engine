// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"

#include "sync.hxx"


CUnitTest( SyncPerformsFAtomicIncrementPointerMaxPvoid, 0x0, "" );
ERR SyncPerformsFAtomicIncrementPointerMaxPvoid::ErrTest()
{
    ERR err = JET_errSuccess;

#ifdef _WIN64
    volatile void * pvoid = (void*)0xffffffff;
    void * pvoidI = 0;

    TestCheck( fTrue == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)0x100000002 ) );
    TestCheck( (QWORD)pvoidI == 0xffffffff );
    TestCheck( fTrue == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)0x100000002 ) );
    TestCheck( (QWORD)pvoidI == 0x100000000 );
    TestCheck( fTrue == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)0x100000002 ) );
    TestCheck( (QWORD)pvoidI == 0x100000001 );
    TestCheck( fFalse == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)0x100000002 ) );
    TestCheck( (QWORD)pvoidI == 0x100000001 );
#else
    volatile void * pvoid = 0;
    void * pvoidI = 0;

    TestCheck( fTrue == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)3 ) );
    TestCheck( pvoidI == 0 );
    TestCheck( fTrue == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)3 ) );
    TestCheck( (INT)pvoidI == 1 );
    TestCheck( fTrue == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)3 ) );
    TestCheck( (INT)pvoidI == 2 );
    TestCheck( fFalse == FAtomicIncrementPointerMax( &pvoid, &pvoidI, (void*)3 ) );
    TestCheck( (INT)pvoidI == 2 );
#endif

HandleError:
    return err;
}

CUnitTest( SyncPerformsFAtomicIncrementMaxOnDword, 0x0, "" );
ERR SyncPerformsFAtomicIncrementMaxOnDword::ErrTest()
{
    ERR err = JET_errSuccess;

    DWORD dw = 0;
    DWORD dwI = 0;

    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 4 ) );
    TestCheck( dwI == 0 );

    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 4 ) );
    TestCheck( dwI == 1 );
    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 4 ) );
    TestCheck( dwI == 2 );
    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 4 ) );
    TestCheck( dwI == 3 );
    TestCheck( fFalse == FAtomicIncrementMax( &dw, &dwI, 4 ) );
    TestCheck( dwI == 3 );
    TestCheck( fFalse == FAtomicIncrementMax( &dw, &dwI, 4 ) );
    TestCheck( dwI == 3 )

    dw = 0x7ffffffe;
    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 0x80000002 ) );
    TestCheck( dwI == 0x7ffffffe );
    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 0x80000002 ) );
    TestCheck( dwI == 0x7fffffff );
    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 0x80000002 ) );
    TestCheck( dwI == 0x80000000 );
    TestCheck( fTrue == FAtomicIncrementMax( &dw, &dwI, 0x80000002 ) );
    TestCheck( dwI == 0x80000001 );
    TestCheck( fFalse == FAtomicIncrementMax( &dw, &dwI, 0x80000002 ) );
    TestCheck( dwI == 0x80000001 );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicAddOnQword, 0x0, "" );
ERR SyncPerformsAtomicAddOnQword::ErrTest()
{
    ERR err = JET_errSuccess;
    QWORD qwAdd             = 660;
    TestCheck( 666 == AtomicAdd( (QWORD * const)&qwAdd, 6 ) );
    TestCheck( 666 == qwAdd );
    TestCheck( 667 == AtomicAdd( (QWORD * const)&qwAdd, 1 ) );
    TestCheck( 667 == qwAdd );
    TestCheck( 607 == AtomicAdd( (QWORD * const)&qwAdd, (QWORD)-60 ) );
    TestCheck( 607 == qwAdd );
HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicIncrementDecrementOnLong, 0x0, "" );
ERR SyncPerformsAtomicIncrementDecrementOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG l = 1;

    TestCheck( 2 == AtomicIncrement( &l ) );
    TestCheck( 3 == AtomicIncrement( &l ) );

    TestCheck( 2 == AtomicDecrement( &l ) );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeSetResetOnUlong, 0x0, "" );
ERR SyncPerformsAtomicExchangeSetResetOnUlong::ErrTest()
{
    ERR err = JET_errSuccess;

    ULONG l = 0x5;

    TestCheck( 0x5 == AtomicExchangeSet( &l, 0x2 ) );
    TestCheck( l == 0x7 );

    TestCheck( 0x7 == AtomicExchangeReset( &l, 0x4 ) );
    TestCheck( l == 0x3 );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeSetResetOnHighBit, 0x0, "" );
ERR SyncPerformsAtomicExchangeSetResetOnHighBit::ErrTest()
{
    ERR err = JET_errSuccess;

    ULONG l = 0x5;

    C_ASSERT( -2147483643 == 0x80000005 );

    TestCheck( 0x5 == AtomicExchangeSet( &l, 0x80000000 ) );
    TestCheck( l == -2147483643 );

    TestCheck( -2147483643 == AtomicExchangeReset( &l, 0x4 ) );
    TestCheck( l == 0x80000001 );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeMinBasicOnLong, 0x0, "" );
ERR SyncPerformsAtomicExchangeMinBasicOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG lTarget = lMax;

    TestCheck( lMax == AtomicExchangeMin( &lTarget, lMax ) );
    TestCheck( lTarget == lMax );

    TestCheck( lMax == AtomicExchangeMin( &lTarget, lMax - 1 ) );
    TestCheck( lTarget == ( lMax - 1 ) );

    TestCheck( ( lMax - 1 ) == AtomicExchangeMin( &lTarget, 1 ) );
    TestCheck( lTarget == 1 );

    TestCheck( 1 == AtomicExchangeMin( &lTarget, lMax ) );
    TestCheck( lTarget == 1 );

    TestCheck( 1 == AtomicExchangeMin( &lTarget, 0 ) );
    TestCheck( lTarget == 0 );

    TestCheck( 0 == AtomicExchangeMin( &lTarget, lMax ) );
    TestCheck( lTarget == 0 );

    TestCheck( 0 == AtomicExchangeMin( &lTarget, -1 ) );
    TestCheck( lTarget == -1 );

    TestCheck( -1 == AtomicExchangeMin( &lTarget, lMax ) );
    TestCheck( lTarget == -1 );

    TestCheck( -1 == AtomicExchangeMin( &lTarget, lMin + 1 ) );
    TestCheck( lTarget == ( lMin + 1 ) );

    TestCheck( ( lMin + 1 ) == AtomicExchangeMin( &lTarget, lMax ) );
    TestCheck( lTarget == ( lMin + 1 ) );

    TestCheck( ( lMin + 1 ) == AtomicExchangeMin( &lTarget, lMin ) );
    TestCheck( lTarget == lMin );

    TestCheck( lMin == AtomicExchangeMin( &lTarget, lMax ) );
    TestCheck( lTarget == lMin );

    TestCheck( lMin == AtomicExchangeMin( &lTarget, 1 ) );
    TestCheck( lTarget == lMin );

    TestCheck( lMin == AtomicExchangeMin( &lTarget, 0 ) );
    TestCheck( lTarget == lMin );

    TestCheck( lMin == AtomicExchangeMin( &lTarget, -1 ) );
    TestCheck( lTarget == lMin );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeMinBasicOnInt64, 0x0, "" );
ERR SyncPerformsAtomicExchangeMinBasicOnInt64::ErrTest()
{
    ERR err = JET_errSuccess;

    __int64 llTarget = llMax;

    TestCheck( llMax == AtomicExchangeMin( &llTarget, llMax ) );
    TestCheck( llTarget == llMax );

    TestCheck( llMax == AtomicExchangeMin( &llTarget, llMax - 1 ) );
    TestCheck( llTarget == ( llMax - 1 ) );

    TestCheck( ( llMax - 1 ) == AtomicExchangeMin( &llTarget, 1 ) );
    TestCheck( llTarget == 1 );

    TestCheck( 1 == AtomicExchangeMin( &llTarget, llMax ) );
    TestCheck( llTarget == 1 );

    TestCheck( 1 == AtomicExchangeMin( &llTarget, 0 ) );
    TestCheck( llTarget == 0 );

    TestCheck( 0 == AtomicExchangeMin( &llTarget, llMax ) );
    TestCheck( llTarget == 0 );

    TestCheck( 0 == AtomicExchangeMin( &llTarget, -1 ) );
    TestCheck( llTarget == -1 );

    TestCheck( -1 == AtomicExchangeMin( &llTarget, llMax ) );
    TestCheck( llTarget == -1 );

    TestCheck( -1 == AtomicExchangeMin( &llTarget, llMin + 1 ) );
    TestCheck( llTarget == ( llMin + 1 ) );

    TestCheck( ( llMin + 1 ) == AtomicExchangeMin( &llTarget, llMax ) );
    TestCheck( llTarget == ( llMin + 1 ) );

    TestCheck( ( llMin + 1 ) == AtomicExchangeMin( &llTarget, llMin ) );
    TestCheck( llTarget == llMin );

    TestCheck( llMin == AtomicExchangeMin( &llTarget, llMax ) );
    TestCheck( llTarget == llMin );

    TestCheck( llMin == AtomicExchangeMin( &llTarget, 1 ) );
    TestCheck( llTarget == llMin );

    TestCheck( llMin == AtomicExchangeMin( &llTarget, 0 ) );
    TestCheck( llTarget == llMin );

    TestCheck( llMin == AtomicExchangeMin( &llTarget, -1 ) );
    TestCheck( llTarget == llMin );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeMinBasicOnUInt64, 0x0, "" );
ERR SyncPerformsAtomicExchangeMinBasicOnUInt64::ErrTest()
{
    ERR err = JET_errSuccess;

    unsigned __int64 ullTarget = ullMax;

    TestCheck( ullMax == AtomicExchangeMin( &ullTarget, ullMax ) );
    TestCheck( ullTarget == ullMax );

    TestCheck( ullMax == AtomicExchangeMin( &ullTarget, ullMax - 1 ) );
    TestCheck( ullTarget == ( ullMax - 1 ) );

    TestCheck( ( ullMax - 1 ) == AtomicExchangeMin( &ullTarget, ullMin + 1 ) );
    TestCheck( ullTarget == ( ullMin + 1 ) );

    TestCheck( ( ullMin + 1 ) == AtomicExchangeMin( &ullTarget, ullMax ) );
    TestCheck( ullTarget == ( ullMin + 1 ) );

    TestCheck( ( ullMin + 1 ) == AtomicExchangeMin( &ullTarget, ullMin ) );
    TestCheck( ullTarget == ullMin );

    TestCheck( ullMin == AtomicExchangeMin( &ullTarget, ullMax ) );
    TestCheck( ullTarget == ullMin );

    TestCheck( ullMin == AtomicExchangeMin( &ullTarget, 0 ) );
    TestCheck( ullTarget == ullMin );

    TestCheck( ullMin == AtomicExchangeMin( &ullTarget, 1 ) );
    TestCheck( ullTarget == ullMin );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeMaxBasicOnLong, 0x0, "" );
ERR SyncPerformsAtomicExchangeMaxBasicOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG lTarget = lMin;

    TestCheck( lMin == AtomicExchangeMax( &lTarget, lMin ) );
    TestCheck( lTarget == lMin );

    TestCheck( lMin == AtomicExchangeMax( &lTarget, lMin + 1 ) );
    TestCheck( lTarget == ( lMin + 1 ) );

    TestCheck( ( lMin + 1 ) == AtomicExchangeMax( &lTarget, -1 ) );
    TestCheck( lTarget == -1 );

    TestCheck( -1 == AtomicExchangeMax( &lTarget, lMin ) );
    TestCheck( lTarget == -1 );

    TestCheck( -1 == AtomicExchangeMax( &lTarget, 0 ) );
    TestCheck( lTarget == 0 );

    TestCheck( 0 == AtomicExchangeMax( &lTarget, lMin ) );
    TestCheck( lTarget == 0 );

    TestCheck( 0 == AtomicExchangeMax( &lTarget, 1 ) );
    TestCheck( lTarget == 1 );

    TestCheck( 1 == AtomicExchangeMax( &lTarget, lMin ) );
    TestCheck( lTarget == 1 );

    TestCheck( 1 == AtomicExchangeMax( &lTarget, lMax - 1 ) );
    TestCheck( lTarget == ( lMax - 1 ) );

    TestCheck( ( lMax - 1 ) == AtomicExchangeMax( &lTarget, lMin ) );
    TestCheck( lTarget == ( lMax - 1 ) );

    TestCheck( ( lMax - 1 ) == AtomicExchangeMax( &lTarget, lMax ) );
    TestCheck( lTarget == lMax );

    TestCheck( lMax == AtomicExchangeMax( &lTarget, lMin ) );
    TestCheck( lTarget == lMax );

    TestCheck( lMax == AtomicExchangeMax( &lTarget, -1 ) );
    TestCheck( lTarget == lMax );

    TestCheck( lMax == AtomicExchangeMax( &lTarget, 0 ) );
    TestCheck( lTarget == lMax );

    TestCheck( lMax == AtomicExchangeMax( &lTarget, 1 ) );
    TestCheck( lTarget == lMax );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeMaxBasicOnInt64, 0x0, "" );
ERR SyncPerformsAtomicExchangeMaxBasicOnInt64::ErrTest()
{
    ERR err = JET_errSuccess;

    __int64 llTarget = llMin;

    TestCheck( llMin == AtomicExchangeMax( &llTarget, llMin ) );
    TestCheck( llTarget == llMin );

    TestCheck( llMin == AtomicExchangeMax( &llTarget, llMin + 1 ) );
    TestCheck( llTarget == ( llMin + 1 ) );

    TestCheck( ( llMin + 1 ) == AtomicExchangeMax( &llTarget, -1 ) );
    TestCheck( llTarget == -1 );

    TestCheck( -1 == AtomicExchangeMax( &llTarget, llMin ) );
    TestCheck( llTarget == -1 );

    TestCheck( -1 == AtomicExchangeMax( &llTarget, 0 ) );
    TestCheck( llTarget == 0 );

    TestCheck( 0 == AtomicExchangeMax( &llTarget, llMin ) );
    TestCheck( llTarget == 0 );

    TestCheck( 0 == AtomicExchangeMax( &llTarget, 1 ) );
    TestCheck( llTarget == 1 );

    TestCheck( 1 == AtomicExchangeMax( &llTarget, llMin ) );
    TestCheck( llTarget == 1 );

    TestCheck( 1 == AtomicExchangeMax( &llTarget, llMax - 1 ) );
    TestCheck( llTarget == ( llMax - 1 ) );

    TestCheck( ( llMax - 1 ) == AtomicExchangeMax( &llTarget, llMin ) );
    TestCheck( llTarget == ( llMax - 1 ) );

    TestCheck( ( llMax - 1 ) == AtomicExchangeMax( &llTarget, llMax ) );
    TestCheck( llTarget == llMax );

    TestCheck( llMax == AtomicExchangeMax( &llTarget, llMin ) );
    TestCheck( llTarget == llMax );

    TestCheck( llMax == AtomicExchangeMax( &llTarget, -1 ) );
    TestCheck( llTarget == llMax );

    TestCheck( llMax == AtomicExchangeMax( &llTarget, 0 ) );
    TestCheck( llTarget == llMax );

    TestCheck( llMax == AtomicExchangeMax( &llTarget, 1 ) );
    TestCheck( llTarget == llMax );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeMaxBasicOnUInt64, 0x0, "" );
ERR SyncPerformsAtomicExchangeMaxBasicOnUInt64::ErrTest()
{
    ERR err = JET_errSuccess;

    unsigned __int64 ullTarget = ullMin;

    TestCheck( ullMin == AtomicExchangeMax( &ullTarget, ullMin ) );
    TestCheck( ullTarget == ullMin );

    TestCheck( ullMin == AtomicExchangeMax( &ullTarget, ullMin + 1 ) );
    TestCheck( ullTarget == ( ullMin + 1 ) );

    TestCheck( ( ullMin + 1 ) == AtomicExchangeMax( &ullTarget, ullMax - 1 ) );
    TestCheck( ullTarget == ( ullMax - 1 ) );

    TestCheck( ( ullMax - 1 ) == AtomicExchangeMax( &ullTarget, ullMin ) );
    TestCheck( ullTarget == ( ullMax - 1 ) );

    TestCheck( ( ullMax - 1 ) == AtomicExchangeMax( &ullTarget, ullMax ) );
    TestCheck( ullTarget == ullMax );

    TestCheck( ullMax == AtomicExchangeMax( &ullTarget, ullMin ) );
    TestCheck( ullTarget == ullMax );

    TestCheck( ullMax == AtomicExchangeMax( &ullTarget, 0 ) );
    TestCheck( ullTarget == ullMax );

    TestCheck( ullMax == AtomicExchangeMax( &ullTarget, 1 ) );
    TestCheck( ullTarget == ullMax );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeAddPointerBasicOnPvoid, 0x0, "" );
ERR SyncPerformsAtomicExchangeAddPointerBasicOnPvoid::ErrTest()
{
    ERR err = JET_errSuccess;

    void * pvoid = NULL;
    size_t cb = 0;

    TestCheck( NULL == AtomicExchangeAddPointer( &pvoid, this ) );  //  silly, as this is not the kind of things we use this for
    TestCheck( this == AtomicExchangeAddPointer( &pvoid, (void*)-((SIGNED_PTR)this) ) );
    TestCheck( pvoid == NULL );

    TestCheck( 0 == AtomicExchangeAddPointer( (void**)&cb, (void*)40 ) );               // "alloc"
    TestCheck( 40 == (size_t)AtomicExchangeAddPointer( (void**)&cb, (void*)40 ) );      // "alloc"
    TestCheck( 80 == (size_t)AtomicExchangeAddPointer( (void**)&cb, (void*)40 ) );      // "alloc"
    TestCheck( 120 == (size_t)AtomicExchangeAddPointer( (void**)&cb, (void*)-40 ) );    // "free"
    TestCheck( cb == 80 );
    TestCheck( cb == 0x50 );

#ifdef _WIN64
    TestCheck( 80 == (size_t)AtomicExchangeAddPointer( (void**)&cb, (void*)0x180000007 ) );
    TestCheck( cb == 0x180000057 );
#else
    TestCheck( 80 == (size_t)AtomicExchangeAddPointer( (void**)&cb, (void*)0x80000007 ) );
    TestCheck( cb == 0x80000057 );
#endif

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicLinklistBasicOperations, 0x0, "" );
ERR SyncPerformsAtomicLinklistBasicOperations::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef struct _AStructForLinking {
        ULONG   ulCount;
//      struct _AStructForLinking * pvNext;
        void *  pvNext; //  just easier to link to next
} AStructForLinking;

    void * pvHead = NULL;

    AStructForLinking   n1 = { 1, NULL };
    AStructForLinking   n2 = { 2, NULL };
    AStructForLinking   n3 = { 3, NULL };
    
    AtomicAddLinklistNode( &n1, &(n1.pvNext), &pvHead );
    TestCheck( pvHead == &n1 );
    TestCheck( n1.pvNext == NULL );

    AtomicAddLinklistNode( &n2, &(n2.pvNext), &pvHead );
    TestCheck( pvHead == &n2 );
    TestCheck( n2.pvNext == &n1 );

    AtomicAddLinklistNode( &n3, &(n3.pvNext), &pvHead );    //  one more for good measure

{
    AStructForLinking * pvList = (AStructForLinking*)PvAtomicRetrieveLinklist( &pvHead );
    TestCheck( pvHead == NULL );    // empties list

    TestCheck( NULL == PvAtomicRetrieveLinklist( &pvHead ) );   // 2nd call gets nothing.

{
    //  List should be inverted ...
    ULONG i = 3;
    while ( pvList )
    {
        TestCheck( i == pvList->ulCount );
        pvList = (AStructForLinking*)pvList->pvNext;
        i--;
    }
    TestCheck( i == 0 );
}
}

HandleError:
    return err;
}


CUnitTest( SyncPerformsAtomicCompareExchangeBasicOnLong64, 0x0, "" );
ERR SyncPerformsAtomicCompareExchangeBasicOnLong64::ErrTest()
{
    ERR err = JET_errSuccess;
    __int64 qwMiddleMinus   = 0x7FFFFFFFFFFFFFF1;
    __int64 qwMiddlePlus        = 0x8000000000000002;
    __int64 qwBig           = 0xFFFFFFFFFFFFFFF5;
    __int64 qwMax           = 0xffffffffffffffff;

    __int64 i64AtomicMem    = 0;

    //
    //  Test AtomicCompareExchange() ...
    //

    //  Test basic compare exchange from 0 to 47 works

    TestCheck( 0  == i64AtomicMem );
    TestCheck( 0  == AtomicCompareExchange( &i64AtomicMem, 0, 47 ) );
    TestCheck( 47 == i64AtomicMem );

    //  Test 2nd basic compare exchange with new value works

    TestCheck( 47 == AtomicCompareExchange( &i64AtomicMem, 47, 63 ) );

    //  Test bad exchange does not set value

    TestCheck( 63 == i64AtomicMem );
    TestCheck( 63 == AtomicCompareExchange( &i64AtomicMem, 47, 80 ) );
    TestCheck( 63 == i64AtomicMem );

    //  Test 64-bit values

    TestCheck( 63            == AtomicCompareExchange( &i64AtomicMem, 63, qwMiddleMinus ) );
    TestCheck( qwMiddleMinus == AtomicCompareExchange( &i64AtomicMem, qwMiddleMinus, qwMiddlePlus ) );
    TestCheck( qwMiddlePlus  == AtomicCompareExchange( &i64AtomicMem, qwMiddlePlus, qwMax ) );
    TestCheck( qwMax         == AtomicCompareExchange( &i64AtomicMem, qwMax, qwBig ) );
    TestCheck( qwBig         == AtomicCompareExchange( &i64AtomicMem, qwMiddlePlus, qwBig ) );
    TestCheck( qwBig         == AtomicCompareExchange( &i64AtomicMem, qwBig, 63 ) );


HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeBasicOnLong64, 0x0, "" );
ERR SyncPerformsAtomicExchangeBasicOnLong64::ErrTest()
{
    ERR err = JET_errSuccess;
    __int64 qwMiddleMinus   = 0x7FFFFFFFFFFFFFF1;
    //__int64 qwMiddlePlus  = 0x8000000000000002;
    __int64 qwBig           = 0xFFFFFFFFFFFFFFF5;
    //__int64 qwMax         = 0xffffffffffffffff;
    __int64 i64AtomicMem    = 63;

    TestCheck( 63 == i64AtomicMem );
    TestCheck( 63 == AtomicExchange( &i64AtomicMem, 90 ) );
    TestCheck( 90 == i64AtomicMem );

    TestCheck( 90    == AtomicExchange( &i64AtomicMem, qwMax ) );
    TestCheck( qwMax == AtomicExchange( &i64AtomicMem, qwBig ) );
    TestCheck( qwBig == AtomicExchange( &i64AtomicMem, qwMiddleMinus ) );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicCompareExchangePointerBasicOnPvoid, 0x0, "" );
ERR SyncPerformsAtomicCompareExchangePointerBasicOnPvoid::ErrTest()
{
    ERR err = JET_errSuccess;

    void * pvoid = &err;

    TestCheck( NULL != AtomicCompareExchangePointer( &pvoid, NULL, this ) );
    TestCheck( &err == AtomicCompareExchangePointer( &pvoid, NULL, this ) );

    TestCheck( &err == AtomicCompareExchangePointer( &pvoid, &err, NULL ) );

    TestCheck( NULL == AtomicCompareExchangePointer( &pvoid, NULL, this ) );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicCompareExchangeBasicOnLong, 0x0, "" );
ERR SyncPerformsAtomicCompareExchangeBasicOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG l = 3;

    TestCheck( 3 == AtomicCompareExchange( &l, 4, 7 ) );
    TestCheck( 3 == l );

    TestCheck( 3 == AtomicCompareExchange( &l, 3, 7 ) );
    TestCheck( 7 == l );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicReadOnLong, 0x0, "" );
ERR SyncPerformsAtomicReadOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG l = lMin;
    TestCheck( lMin == AtomicRead( &l ) );

    l = 0;
    TestCheck( 0 == AtomicRead( &l ) );

    l = lMax;
    TestCheck( lMax == AtomicRead( &l ) );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicReadOnInt64, 0x0, "" );
ERR SyncPerformsAtomicReadOnInt64::ErrTest()
{
    ERR err = JET_errSuccess;

    __int64 ll = llMin;
    TestCheck( llMin == AtomicRead( &ll ) );

    ll = 0;
    TestCheck( 0 == AtomicRead( &ll ) );

    ll = llMax;
    TestCheck( llMax == AtomicRead( &ll ) );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeAddBasicOnLong, 0x0, "" );
ERR SyncPerformsAtomicExchangeAddBasicOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG l = 2;

    TestCheck( 2 == AtomicExchangeAdd( &l, 3 ) );
    TestCheck( 5 == AtomicExchangeAdd( &l, 8 ) );

    TestCheck( 13 == AtomicExchangeAdd( &l, -15 ) );
    TestCheck( l == -2 );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangePointerBasicOnPvoid, 0x0, "" );
ERR SyncPerformsAtomicExchangePointerBasicOnPvoid::ErrTest()
{
    ERR err = JET_errSuccess;

    void * pvoid = NULL;

    TestCheck( NULL == AtomicExchangePointer( &pvoid, (void*)this ) );

    TestCheck( this == pvoid );

HandleError:
    return err;
}

CUnitTest( SyncPerformsAtomicExchangeBasicOnLong, 0x0, "" );
ERR SyncPerformsAtomicExchangeBasicOnLong::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG l = 0;
    DWORD dw = 0;

    TestCheck( l == 0 );
    TestCheck( dw == 0 );

    TestCheck( 0 == AtomicExchange( &l, 33 ) );
    TestCheck( 0 == AtomicExchange( (LONG*)&dw, 34 ) );

    TestCheck( l == 33 );
    TestCheck( dw == 34 );

HandleError:
    return err;
}

CUnitTest( SyncCatchesAtomicAlignments, 0x0, "" );
ERR SyncCatchesAtomicAlignments::ErrTest()
{
    ERR err = JET_errSuccess;

    LONG l;
    void * pvoid;

    TestCheck( IsAtomicallyModifiable( &l ) );
    TestCheck( IsAtomicallyModifiablePointer( &pvoid ) );

    TestCheck( !IsAtomicallyModifiable( ((LONG*)( ((BYTE*)&l) + 1 )) ) );
    TestCheck( !IsAtomicallyModifiablePointer( ((void**)( ((BYTE*)&pvoid) + sizeof(pvoid)/2 )) ) );

HandleError:
    return err;
}


