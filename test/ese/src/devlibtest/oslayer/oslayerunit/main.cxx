// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#ifdef BUILD_ENV_IS_NT
#else
#pragma warning(disable:22018)
#endif
#include <strsafe.h>
#ifdef BUILD_ENV_IS_NT
#else
#pragma warning(default:22018)
#endif


static void PrintHelp( const char * const szApplication )
{
    fprintf( stderr, "Usage: %s [tests]\r\n", szApplication );
    fprintf( stderr, "\tNo arguments runs all tests.\r\n" );
    fprintf( stderr, "\tReturns 0 for success, and 1 for failure.\r\n" );
    fprintf( stderr, "\tAvailable tests are:\r\n\r\n" );
    BstfPrintTests();
}

#ifndef ESENT

#ifdef DEBUG

#ifndef MEM_CHECK
#error "MEM_CHECK is suspposed to be on in debug."
#endif

#else

#ifdef MEM_CHECK
#error "MEM_CHECK is supposed to be off in retail."
#endif

#endif

#endif

INT __cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
{
    if( FBstfHelpArg( argv[1] ) )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    return ErrBstfRunTests( argc, argv );
}
