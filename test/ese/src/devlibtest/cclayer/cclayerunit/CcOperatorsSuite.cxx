// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CcLayerUnit.hxx"

#include <stdlib.h>


CUnitTest( CcCountofReturnsCorrectSize, 0x0, "Tests the _countof() operator." );
ERR CcCountofReturnsCorrectSize::ErrTest()
{
    ERR err = JET_errSuccess;

    BYTE rgb[10];
    ULONG rgul[7];
    QWORD rgqw[3];
    void * rgpv[8];

    TestCheck( 10 == sizeof(rgb) );
    TestCheck( 10 == _countof(rgb) );

    TestCheck( 28 == sizeof(rgul) );
    TestCheck( 7 == _countof(rgul) );

    TestCheck( 24 == sizeof(rgqw) );
    TestCheck( 3 == _countof(rgqw) );

#ifdef _WIN64
    TestCheck( 64 == sizeof(rgpv) );
#else
    TestCheck( 32 == sizeof(rgpv) );
#endif
    TestCheck( 8 == _countof(rgpv) );

HandleError:
    return err;
}

CUnitTest( CcOffsetOfReturnsCorrectOffsets, btcfForceStackTrash, "" );
ERR CcOffsetOfReturnsCorrectOffsets::ErrTest()
{
    ERR err = JET_errSuccess;

    typedef struct _AStructForImplicitInit {
        ULONG   ulOne;
        QWORD   qwTwo;
        LONG    lZero;
        QWORD   lZeroToo;
} AStructForImplicitInit;

    TestCheck( 0 == OffsetOf( AStructForImplicitInit, ulOne ) );
    TestCheck( 8 == OffsetOf( AStructForImplicitInit, qwTwo ) );
    TestCheck( 16 == OffsetOf( AStructForImplicitInit, lZero ) );
    TestCheck( 24 == OffsetOf( AStructForImplicitInit, lZeroToo ) );

HandleError:
    return err;
}

