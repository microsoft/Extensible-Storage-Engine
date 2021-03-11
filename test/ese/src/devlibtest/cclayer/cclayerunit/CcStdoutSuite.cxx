// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CcLayerUnit.hxx"

#include <stdio.h>
#include <stdlib.h>


CUnitTest( CcPrintfPrintsNumbersCorrectly, 0x0, "This is a manual test, it just prints things out and someone has to read it to verify it." );
ERR CcPrintfPrintsNumbersCorrectly::ErrTest()
{
    printf( "This test is supposed to appear to be counting each line being more than the last ...\n\n" );

    printf( "NOTE: ERROR - Right now this test is SOOO broken on other platforms right now.\n\n" );

    LONG lNegativeOne = -1;
    printf( "\tHello to %d platform(s) under ESE.\n", lNegativeOne );

    printf( "\tHello to %d platform(s) under ESE.\n", 0 );

    LONG lOne = 1;
    printf( "\tHello to %d platform(s) under ESE.\n", lOne );

    ULONG64 ullTwo  = 2;
    printf( "\tHello to %d platform(s) under ESE.\n", ullTwo );

    QWORD qwThree = 3;
    printf( "\tHello to %x platform(s) under ESE.\n", qwThree );

    LONG rgl[3] = { 0x42, 4, 0x42 };
    printf( "\tHello to %d platform(s) under ESE.\n", rgl[1] );

    LONG64 rgll[3] = { 0x42, 5, 0x42 };
    printf( "\tHello to %I64d platform(s) under ESE.\n", rgll[1] );


    printf( "\tHello to 6 platform(s) under ESE.\n" );
    printf( "\nThat finishes our number printing test.\n\n" );

    return JET_errSuccess;
}

CUnitTest( CcPrintfPrintsInsertStringsCorrectly, 0x0, "This is a manual test, it just prints things out and someone has to read it to verify it." );
ERR CcPrintfPrintsInsertStringsCorrectly::ErrTest()
{
    printf( "This test is supposed to appear to be counting each line being more than the last ...\n\n" );

    printf( "NOTE: ERROR - Right now this test is broken on other platforms right now.\n\n" );

    const char * szOne = "1";
    printf( "\tHello to %s platform(s) under ESE.\n", szOne );

    const wchar_t * wszTwo = L"2";
    printf( "\tHello to %ws platform(s) under ESE.\n", wszTwo );

    const wchar_t * wszThree = L"3";
    wprintf( L"\tHello to %ws platform(s) under ESE.\n", wszThree );

    const char * szFour = "4";
    printf( "\tHello to %hs platform(s) under ESE.\n", szFour );


    printf( "\tHello to 5 platform(s) under ESE.\n" );
    printf( "\nThat finishes our string printing test.\n\n" );

    return JET_errSuccess;
}


CUnitTest( CcPrintfBasicHandwritingIsLegible, 0x0, "This is a manual test, it just prints things out and someone has to read it to verify it." );
ERR CcPrintfBasicHandwritingIsLegible::ErrTest()
{
    printf( "This test is supposed to appear to be counting each line being more than the last ...\n\n" );

    printf( "NOTE: ERROR - AMAZINGLY ... even this stupid simpliest of tests is SOOO broken on another platforms right now.\n\n" );

    printf( "Hello to 1 platform under ESE.\n" );

    printf( "\tHello to 2 platform(s) under ESE. Should be 1 tab stop in from here on out.\n" );

    wprintf( L"\tHello to 3 platform(s) under ESE.\n" );


    printf( "\tHello to 4 platform(s) under ESE.\n" );
    printf( "\nThat finishes our grammar school handwriting test.\n\n" );

    return JET_errSuccess;
}

