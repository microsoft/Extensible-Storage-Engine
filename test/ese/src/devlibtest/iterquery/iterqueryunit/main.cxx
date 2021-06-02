// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "IterQueryUnitTest.hxx"

// SOMEONE ... 
//#include "sync.hxx"

/* SOMEONE
void OSSYNCAPI EnforceFail( const char* szMessage, const char* szFilename, LONG lLine )
{
    wprintf( L"\t\t\tEnforceFail( %hs ) ... Failed @ %d!\n", szMessage, lLine );
// SOMEONE this isn't right ... 
    exit(1);
}
*/

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
INT __cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
//  ================================================================
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
    const ERR err = ErrBstfRunTests( argc, argv );
    return err;
}


void AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... )
{
    g_cTestsFailed++;
    wprintf( L"\t\t\tITQUAssert( %hs ) ... Failed in %hs @ %d!\n", szMessage, szFilename, lLine );
    exit(1);
}

void EnforceFail( const char * szMessage, const char * szFilename, LONG lLine )
{
    g_cTestsFailed++;
    wprintf( L"\t\t\tITQUEnforce( %hs ) ... Failed in %hs @ %d!\n", szMessage, szFilename, lLine );
    exit(1);
}

