// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CcLayerUnit.hxx"

#include <stdio.h>
#include <stdlib.h>


CUnitTest( CcCatchesCassert, 0x0, "Checks that the C_ASSERT() catches static compile conditions that do not hold." );
ERR CcCatchesCassert::ErrTest()
{
#ifdef ESE_COMPILE_TEST_CASSERT_FAILS
    C_ASSERT( fFalse );
#endif
    C_ASSERT( fTrue );
    return JET_errSuccess;
}

CUnitTest( CcCannotCountofSingletonVar, 0x0, "Checks the _countof() can work on singleton variables." );
ERR CcCannotCountofSingletonVar::ErrTest()
{
    ERR err = JET_errSuccess;
    LONG l;
    LONG rgl[2];
#ifdef ESE_COMPILE_TEST_COUNTOF_FAILS
    TestCheck( 10 == _countof(l) );
#endif
    TestCheck( 2 == _countof(rgl) );
HandleError:
    return JET_errSuccess;
}

