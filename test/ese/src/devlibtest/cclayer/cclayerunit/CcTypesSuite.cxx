// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CcLayerUnit.hxx"

#include <stdio.h>
#include <stdlib.h>


C_ASSERT( (BYTE)fFalse != (BYTE)fTrue );

#define hex31Of32Bits       0x7FFFFFFF
#define hex32Of32Bits       0xFFFFFFFF
#define hexHiOf32Bits       0x80000000


BOOL FTestMemForByte( void * pvSearchTarget, const ULONG cbSearchTarget, const BYTE bTarget )
{
    const BYTE * const pbSearchTarget = (const BYTE * const)pvSearchTarget;
    for( ULONG ib = 0; ib < cbSearchTarget; ib++ )
    {
        if ( pbSearchTarget[ib] == bTarget )
        {
            return fTrue;
        }
    }
    return fFalse;
}

BOOL FTestMemForUlong( void * pvSearchTarget, const ULONG cbSearchTarget, const ULONG ulTarget )
{
    const BYTE * const pbSearchTarget = (const BYTE * const)pvSearchTarget;
    for( ULONG iul = 0; iul < cbSearchTarget/sizeof(ULONG); iul++ )
    {
        if ( ((ULONG*)pbSearchTarget)[iul] == ulTarget )
        {
            return fTrue;
        }
    }
    return fFalse;
}


BOOL memchk( _In_ const void * pv, _In_ char ch, _In_ const ULONG cb )
{
    const char * pb = (const char *)pv;
    ULONG ib = 0;
    for( ib = 0; ib < cb; ib++ )
    {
        if ( pb[ib] != ch )
        {
            return fFalse;
        }
    }
    return fTrue;
}

CUnitTest( CcBasicStructImplicitZeroInit, btcfForceStackTrash, "Validates that a certain coding practice I've seen generates a correctly initialized structure." );
#pragma warning(disable: 4700)
ERR CcBasicStructImplicitZeroInit::ErrTest()
{
    ERR err = JET_errSuccess;

    ULONG rgulStuff[16];
    ( (volatile ULONG*)rgulStuff )[0] += 0;

    typedef struct _AStructForImplicitInit {
        ULONG   ulOne;
        QWORD   qwTwo;
        LONG    lZero;
        QWORD   lZeroToo;
} AStructForImplicitInit;

    AStructForImplicitInit  sUninit;
    AStructForImplicitInit  sHalfInit = { 1, 2 };
    AStructForImplicitInit  sMostInit = { 1, 2, 0 };
    AStructForImplicitInit  sZero = { 0 };

    ( (volatile ULONG*)&sUninit.ulOne )[0] += 0;
    ( (volatile ULONG*)&sHalfInit.ulOne )[0] += 0;
    ( (volatile ULONG*)&sMostInit.ulOne )[0] += 0;
    ( (volatile ULONG*)&sZero.ulOne )[0] += 0;

    TestCheck( sUninit.ulOne != 0 );
    TestCheck( sUninit.qwTwo != 0 );
    TestCheck( sUninit.lZero != 0 );
    TestCheck( sUninit.lZeroToo != 0 );

    TestCheck( sHalfInit.ulOne == 1 );
    TestCheck( sHalfInit.qwTwo == 2 );
    TestCheck( sHalfInit.lZero == 0 );
    TestCheck( sHalfInit.lZeroToo == 0 );

    TestCheck( sMostInit.ulOne == 1 );
    TestCheck( sMostInit.qwTwo == 2 );
    TestCheck( sMostInit.lZero == 0 );
    TestCheck( sMostInit.lZeroToo == 0 );

    TestCheck( sZero.ulOne == 0 );
    TestCheck( sZero.qwTwo == 0 );
    TestCheck( sZero.lZero == 0 );
    TestCheck( sZero.lZeroToo == 0 );

HandleError:
    return JET_errSuccess;
}
#pragma warning(default: 4700)

CUnitTest( CcBasicArrayImplicitZeroInit, btcfForceStackTrash, "Validates that a certain coding practice I've seen generates a correctly initialized array." );
#pragma warning(disable: 4700)
ERR CcBasicArrayImplicitZeroInit::ErrTest()
{
    ERR err = JET_errSuccess;

    ULONG rgulStuff[16];
    ( (volatile ULONG*)rgulStuff )[0] += 0;

    ULONG   ul;

    ULONG   rgul[30];

    ULONG   rgulOneTwoThree[3] = { 0x1, 0x2, 0x3 };

    ULONG   rgulImplZeros[10] = { 0x1, 0x2, };

    ULONG   rgulImplZerosV2[10] = { 0x1, 0x2, 0x0, };
    ULONG   rgulImplOnes[10] = { 0x1, 0x2, 0x7F7F7F7F, };

    TestCheck( ul != 0 );

    TestCheck( sizeof(rgul) == 120 );
    TestCheck( !FTestMemForUlong( rgul, sizeof(rgul), 0 ) );
#ifdef _WIN64
    TestCheck( FTestMemForUlong( rgul, sizeof(rgul), 0xfe000012 ) );
#else
    TestCheck( FTestMemForUlong( rgul, sizeof(rgul), 0xfe000034 ) );
#endif

    TestCheck( !FTestMemForByte( rgulOneTwoThree, sizeof(rgulOneTwoThree), 0xFF ) );
    TestCheck( FTestMemForUlong( rgulOneTwoThree, sizeof(rgulOneTwoThree), 0x1 ) );
    TestCheck( FTestMemForUlong( rgulOneTwoThree, sizeof(rgulOneTwoThree), 0x2 ) );
    TestCheck( FTestMemForUlong( rgulOneTwoThree, sizeof(rgulOneTwoThree), 0x3 ) );

    TestCheck( FTestMemForUlong( rgulImplZeros, sizeof(rgulImplZeros), 0x1 ) );
    TestCheck( FTestMemForUlong( rgulImplZeros, sizeof(rgulImplZeros), 0x2 ) );
    TestCheck( FTestMemForUlong( rgulImplZeros+2, sizeof(rgulImplZeros)-2*sizeof(rgulImplZeros[0]), 0x0 ) );
    TestCheck( memchk( rgulImplZeros+2, 0x0, sizeof(rgulImplZeros)-2*sizeof(rgulImplZeros[0]) ) );

    TestCheck( FTestMemForUlong( rgulImplZerosV2, sizeof(rgulImplZerosV2), 0x1 ) );
    TestCheck( FTestMemForUlong( rgulImplZerosV2, sizeof(rgulImplZerosV2), 0x2 ) );
    TestCheck( FTestMemForUlong( rgulImplZerosV2+2, sizeof(rgulImplZerosV2)-2*sizeof(rgulImplZerosV2[0]), 0x0 ) );
    TestCheck( memchk( rgulImplZeros+2, 0x0, sizeof(rgulImplZerosV2)-2*sizeof(rgulImplZerosV2[0]) ) );
    TestCheck( FTestMemForUlong( rgulImplOnes, sizeof(rgulImplOnes), 0x1 ) );
    TestCheck( FTestMemForUlong( rgulImplOnes, sizeof(rgulImplOnes), 0x2 ) );
    TestCheck( FTestMemForUlong( rgulImplOnes+2, sizeof(rgulImplOnes)-2*sizeof(rgulImplOnes[0]), 0x7F7F7F7F ) );

HandleError:
    return err; 
}
#pragma warning(default: 4700)


CUnitTest( CcFlagBitFieldSetting, 0x0, "Tests that single-bit bit fields will operate as normal BOOL-like variables." );
ERR CcFlagBitFieldSetting::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting FLAG32 behaves as expected ...\n" );

    typedef struct TestBitFieldStruct_
    {
        INT     iFullField1:14;
        FLAG32  fTester:1;
        FLAG32  fTesty:1;
        INT     iFullField2:16;
    } TestBitFieldStruct;

    TestBitFieldStruct test;
    TestBitFieldStruct test2 = { 0x42, fTrue, fFalse, 0x43 };

    TestCheck( 4 == sizeof( TestBitFieldStruct ) );
    TestCheck( 4 == sizeof( test ) );
    TestCheck( 4 == sizeof( test2 ) );

    test.iFullField1 = 0x44;
    test.fTester = fFalse;
    test.fTesty = fTrue;
    test.iFullField2 = 0x45;

    TestCheck( !test.fTester );
    TestCheck( test.fTesty );

    TestCheck( test.fTesty == test2.fTester );
    TestCheck( test.fTester == test2.fTesty );

    test2 = test;
    TestCheck( test.fTesty == test2.fTesty );
    TestCheck( test.fTester == test2.fTester );


HandleError:
    return err; 
}


CUnitTest( CcFlagBitFieldSettingWithUnion, 0x0, "Tests that single-bit bit fields will operate as normal BOOL-like variables." );
ERR CcFlagBitFieldSettingWithUnion::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting FLAG32 behaves as expected ...\n" );

    typedef struct TestBitFieldStruct_
    {
        union
        {
            INT flags;
            struct
            {
                INT     iFullField1:14;
                FLAG32  fTester:1;
                FLAG32  fTesty:1;
                INT     iFullField2:16;
            };
        };
    } TestBitFieldStruct;

    TestBitFieldStruct test, test2;

    TestCheck( 4 == sizeof( TestBitFieldStruct ) );
    TestCheck( 4 == sizeof( test ) );

    test.iFullField1 = 0x46;
    test.fTester = fFalse;
    test.fTesty = fTrue;
    test.iFullField2 = 0x47;

    TestCheck( !test.fTester );
    TestCheck( test.fTesty );
    TestCheck( test.iFullField1 == 0x46 );
    TestCheck( test.iFullField2 == 0x47 );
    TestCheck( test.flags == 0x00478046 );

    test2 = test;
    TestCheck( test.fTesty == test2.fTesty );
    TestCheck( test.fTester == test2.fTester );
    TestCheck( test.iFullField1 == test2.iFullField1 );
    TestCheck( test.iFullField2 == test2.iFullField2 );

HandleError:
    return err; 
}


CUnitTest( CcSupportsAcceptable32BitLong, 0x0, "This tests the compiler and options we're using supports a LONG type that is 32-bits and ONLY 32-bits" );
ERR CcSupportsAcceptable32BitLong::ErrTest()
{
    ERR err = JET_errSuccess;


    wprintf( L"\tTesting LONG behaves as expected ...\n" );

    LONG lUshortMax     = 0xFFFF;
    LONG lLongMax       = hex31Of32Bits;
    LONG lUlongMax      = hex32Of32Bits;

    TestCheck( (QWORD)0x1FFFE == ( lUshortMax + lUshortMax ) );
    TestCheck( (QWORD)0x1FFFE == ( lUshortMax + lUshortMax ) );

    TestCheck( -2 == ( lLongMax + lLongMax ) );
    TestCheck( 0xFFFFFFFE == ( lLongMax + lLongMax ) );

    TestCheck( -2 == ( lUlongMax + lUlongMax ) );
    TestCheck( 0xFFFFFFFE == ( lUlongMax + lUlongMax ) );

HandleError:
    return err; 
}

CUnitTest( CcLimitConstantsAreRightForLongs, 0x0, "This tests the compiler and limits are 32-bit maxes." );
ERR CcLimitConstantsAreRightForLongs::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting LONG behaves as expected ...\n" );

    LONG l;
    ULONG ul;
    LONG64 ll;
    ULONG64 ull;
    QWORD qw;
    DWORD dw;

    LONG lLongMax       = hex31Of32Bits;
    LONG lUlongMax      = hex32Of32Bits;

    QWORD qwLongMax     = hex31Of32Bits;
    QWORD qwUlongMin    = 0;
    QWORD qwUlongMax    = hex32Of32Bits;

    TestCheck( lLongMax == lMax );
    TestCheck( lUlongMax == ulMax );


    TestCheck( qwLongMax == lMax );
    TestCheck( qwUlongMin == ulMin );
    TestCheck( qwUlongMax == ulMax );

    qw = qwMax;
    TestCheck( qw+1 == 0 );

    dw = dwMax;
    TestCheck( dw+1 == 0 );

    l = lMax;
    TestCheck( l+1 == lMin );

    ll = llMax;
    TestCheck( ll+1 == llMin );

HandleError:
    return err; 
}


CUnitTest( CcBasicTypesAreRightSizes, 0x0, "This tests that our typedef defined types all match the expected sizes." );
ERR CcBasicTypesAreRightSizes::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting that our typedef types are expected sizes ...\n" );

    TestCheck( sizeof(bool) == 1 );
    TestCheck( sizeof(BOOL) == 4 );

    TestCheck( sizeof(CHAR) == 1 );

    TestCheck( sizeof(SHORT) == 2 );
    TestCheck( sizeof(USHORT) == 2 );

    TestCheck( sizeof(INT) == 4 );
    TestCheck( sizeof(UINT) == 4 );

    TestCheck( sizeof(LONG) == 4 );
    TestCheck( sizeof(ULONG) == 4 );

    TestCheck( sizeof(LONG64) == 8 );
    TestCheck( sizeof(ULONG64) == 8 );

    TestCheck( sizeof(BYTE) == 1 );
    TestCheck( sizeof(WORD) == 2 );
    TestCheck( sizeof(DWORD) == 4 );
    TestCheck( sizeof(QWORD) == 8 );

    if ( sizeof(void*) == 8 )
    {
        const QWORD cbPointer = 8;
        printf( "\t\tDetected 64-bit platform, testing expected variant sized type against %d bytes.\n", cbPointer );
        TestCheck( sizeof(UNSIGNED_PTR) == cbPointer );
        TestCheck( sizeof(SIGNED_PTR) == cbPointer );
        TestCheck( sizeof(size_t) == cbPointer );
    }
    else
    {
        const QWORD cbPointer = 4;
        printf( "\t\tDetected 64-bit platform, testing expected variant sized type against %d bytes.\n", cbPointer );
        TestCheck( sizeof(UNSIGNED_PTR) == cbPointer );
        TestCheck( sizeof(SIGNED_PTR) == cbPointer );
        TestCheck( sizeof(size_t) == cbPointer );
    }

HandleError:
    return err; 
}

