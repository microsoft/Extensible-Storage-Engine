// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CcLayerUnit.hxx"

//  ================================================================
static void PrintHelp( const char * const szApplication )
//  ================================================================
{
    fprintf( stderr, "Usage: %s [tests]\r\n", szApplication );
    fprintf( stderr, "\tNo arguments runs all tests.\r\n" );
    fprintf( stderr, "\tReturns 0 for success, and 1 for failure.\r\n" );
    fprintf( stderr, "\tAvailable tests are:\r\n\r\n" );
    BstfPrintTests();
}

//  ================================================================
INT __cdecl main( INT argc, _In_count_( argc ) char * argv[] )
//  ================================================================
{
    if( argc == 2 && FBstfHelpArg( argv[1] ) )
    {
        PrintHelp( argv[0] );
        return 0;
    }
    return ErrBstfRunTests( argc, argv );
}
