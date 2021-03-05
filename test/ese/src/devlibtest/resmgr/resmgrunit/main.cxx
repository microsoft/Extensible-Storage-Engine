// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgrunittest.hxx"

#ifdef BUILD_ENV_IS_NT
#else
#pragma warning(disable:22018)
#endif
#include <strsafe.h>
#ifdef BUILD_ENV_IS_NT
#else
#pragma warning(default:22018)
#endif

#include "sync.hxx"
void OSSYNCAPI EnforceFail( const char* szMessage, const char* szFilename, LONG lLine )
{
    wprintf( L"\t\t\tEnforceFail( %hs ) ... Failed @ %d!\n", szMessage, lLine );
    exit(1);
}


static void PrintHelp( const char * const szApplication )
{
    fprintf( stderr, "Usage: %s [tests]\r\n", szApplication );
    fprintf( stderr, "\tNo arguments runs all tests.\r\n" );
    fprintf( stderr, "\tReturns 0 for success, and 1 for failure.\r\n" );
    fprintf( stderr, "\tAvailable tests are:\r\n\r\n" );
    BstfPrintTests();
}


INT _cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
{
    FOSSyncPreinit();
    if( argc == 2
        && ( 0 == _stricmp( argv[1], "-h" )
            || 0 == _stricmp( argv[1], "/h" )
            || 0 == _stricmp( argv[1], "-?" )
            || 0 == _stricmp( argv[1], "/?" ) ) )
    {
        PrintHelp( argv[0] );
        return 0;
    }
    const JET_ERR err = ErrBstfRunTests( argc, argv );
    OSSyncPostterm();
    return err;
}
