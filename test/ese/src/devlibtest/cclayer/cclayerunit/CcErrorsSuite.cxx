// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CcLayerUnit.hxx"

#include <stdio.h>
#include <stdlib.h>

// ESE_MANUAL_TESTS: A set of compiler errors we expect, one by one enable on new platforms and ensure they
//      show up.
//#define ESE_COMPILE_TEST_CASSERT_FAILS    //  ..\ccerrorssuite.cxx(18) : error C2118: negative subscript
                                            //  ../CcErrorsSuite.cxx:17:2: error: size of array "__C_ASSERT__" is negative
//#define ESE_COMPILE_TEST_COUNTOF_FAILS    //  ..\ccerrorssuite.cxx(31) : error C2784: 'char (*__countof_helper(__unaligned _CountofType (&)[_SizeOfArray]))[_SizeOfArray]' : could not deduce template argument for '__unaligned _CountofType (&)[_SizeOfArray]' from 'LONG'
                                            //  ../CcErrorsSuite.cxx:32:2: error: invalid types "LONG {aka int}[int]" for array subscript

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

