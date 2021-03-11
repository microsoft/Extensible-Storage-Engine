// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"

#include <string.h>


static void PrintTests()
{
    const UNITTEST * punittest = UNITTEST::s_punittestHead;
    while( punittest )
    {
        fprintf( stderr, "==> %s\r\n%s\r\n\r\n", punittest->SzName(), punittest->SzDescription() );
        punittest = punittest->m_punittestNext;
    }
}


static void PrintHelp( const char * const szApplication )
{
    fprintf( stderr, "Usage: %s [tests]\r\n", szApplication );
    fprintf( stderr, "\tNo arguments runs all tests.\r\n" );
    fprintf( stderr, "\tReturns 0 for success, and 1 for failure.\r\n" );
    fprintf( stderr, "\tAvailable tests are:\r\n\r\n" );
    BstfPrintTests();
}


BOOL    g_fCaptureAssert = fFalse;
CHAR*   g_szCapturedAssert = NULL;

INT __cdecl main( INT argc, _In_count_( argc ) char * argv[] )
{
    if( argc == 2
        && ( 0 == _stricmp( argv[1], "-h" )
            || 0 == _stricmp( argv[1], "/h" )
            || 0 == _stricmp( argv[1], "-?" )
            || 0 == _stricmp( argv[1], "/?" ) ) )
    {
        PrintHelp( argv[0] );
        return 0;
    }
    return ErrBstfRunTests( argc, argv );
}
