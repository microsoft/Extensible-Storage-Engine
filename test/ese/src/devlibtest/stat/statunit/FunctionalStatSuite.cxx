// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

CUnitTest( StatUlMinRudimentary, 0x0, "This tests UlFunctionalMin() basically works." );
ERR StatUlMinRudimentary::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting UlFunctionalMin() ...\n" );

    TestCheck( 0 == UlFunctionalMin( 0, 1 ) );
    TestCheck( 0 == UlFunctionalMin( 1, 0 ) );

    TestCheck( 3 == UlFunctionalMin( 3, 5 ) );

    TestCheck( 10 == UlFunctionalMin( 10, 10 ) );

    TestCheck( 0x7FFFFFFF == UlFunctionalMin( 0x7FFFFFFF, 0x80000000 ) );
    TestCheck( 0x7FFFFFFF == UlFunctionalMin( 0x80000000, 0x7FFFFFFF ) );

    TestCheck( 0xFFFFFFFE == UlFunctionalMin( 0xFFFFFFFF, 0xFFFFFFFE ) );
    TestCheck( 0xFFFFFFFE == UlFunctionalMin( 0xFFFFFFFE, 0xFFFFFFFF ) );

HandleError:
    return err; 
}


CUnitTest( StatUlMaxRudimentary, 0x0, "This tests UlFunctionalMax() basically works." );
ERR StatUlMaxRudimentary::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting UlFunctionalMax() ...\n" );

    TestCheck( 1 == UlFunctionalMax( 0, 1 ) );
    TestCheck( 1 == UlFunctionalMax( 1, 0 ) );

    TestCheck( 5 == UlFunctionalMax( 3, 5 ) );

    TestCheck( 10 == UlFunctionalMax( 10, 10 ) );

    TestCheck( 0x80000000 == UlFunctionalMax( 0x7FFFFFFF, 0x80000000 ) );
    TestCheck( 0x80000000 == UlFunctionalMax( 0x80000000, 0x7FFFFFFF ) );

    TestCheck( 0xFFFFFFFF == UlFunctionalMax( 0xFFFFFFFF, 0xFFFFFFFE ) );
    TestCheck( 0xFFFFFFFF == UlFunctionalMax( 0xFFFFFFFE, 0xFFFFFFFF ) );

HandleError:
    return err; 
}

CUnitTest( StatLMinRudimentary, 0x0, "This tests LFunctionalMin() basically works." );
ERR StatLMinRudimentary::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting LFunctionalMin() ...\n" );

    TestCheck( 0 == LFunctionalMin( 0, 1 ) );
    TestCheck( 0 == LFunctionalMin( 1, 0 ) );

    TestCheck( -1 == LFunctionalMin( 0, -1 ) );
    TestCheck( -1 == LFunctionalMin( -1, 0 ) );

    TestCheck( 3 == LFunctionalMin( 3, 5 ) );

    TestCheck( 10 == LFunctionalMin( 10, 10 ) );

    TestCheck( (LONG)0x80000000 == LFunctionalMin( 0x7FFFFFFF, (LONG)0x80000000 ) );
    TestCheck( (LONG)0x80000000 == LFunctionalMin( (LONG)0x80000000, 0x7FFFFFFF ) );

    TestCheck( (LONG)0xFFFFFFFE == LFunctionalMin( (LONG)0xFFFFFFFF, (LONG)0xFFFFFFFE ) );
    TestCheck( (LONG)0xFFFFFFFE == LFunctionalMin( (LONG)0xFFFFFFFE, (LONG)0xFFFFFFFF ) );

HandleError:
    return err; 
}


CUnitTest( StatLMaxRudimentary, 0x0, "This tests LFunctionalMax() basically works." );
ERR StatLMaxRudimentary::ErrTest()
{
    ERR err = JET_errSuccess;

    wprintf( L"\tTesting LFunctionalMax() ...\n" );

    TestCheck( 1 == LFunctionalMax( 0, 1 ) );
    TestCheck( 1 == LFunctionalMax( 1, 0 ) );

    TestCheck( 0 == LFunctionalMax( 0, -1 ) );
    TestCheck( 0 == LFunctionalMax( -1, 0 ) );

    TestCheck( 5 == LFunctionalMax( 3, 5 ) );

    TestCheck( 10 == LFunctionalMax( 10, 10 ) );

    TestCheck( 0x7FFFFFFF == LFunctionalMax( 0x7FFFFFFF, (LONG)0x80000000 ) );
    TestCheck( 0x7FFFFFFF == LFunctionalMax( (LONG)0x80000000, 0x7FFFFFFF ) );

    TestCheck( (LONG)0xFFFFFFFF == LFunctionalMax( (LONG)0xFFFFFFFF, (LONG)0xFFFFFFFE ) );
    TestCheck( (LONG)0xFFFFFFFF == LFunctionalMax( (LONG)0xFFFFFFFE, (LONG)0xFFFFFFFF ) );

HandleError:
    return err; 
}


