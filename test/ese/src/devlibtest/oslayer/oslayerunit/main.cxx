// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#ifdef BUILD_ENV_IS_NT
#else  //  !BUILD_ENV_IS_NT
#pragma warning(disable:22018)  //  Ex12 RTM:  the version of strsafe.h we use has 22018 warnings we can't control
#endif  //  BUILD_ENV_IS_NT
#include <strsafe.h>
#ifdef BUILD_ENV_IS_NT
#else  //  !BUILD_ENV_IS_NT
#pragma warning(default:22018)
#endif  //  BUILD_ENV_IS_NT


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

#ifndef ESENT

#ifdef DEBUG

#ifndef MEM_CHECK
#error "MEM_CHECK is suspposed to be on in debug."
#endif // MEM_CHECK

#else // !DEBUG

#ifdef MEM_CHECK
#error "MEM_CHECK is supposed to be off in retail."
#endif // MEM_CHECK

#endif // DEBUG

#endif // !ESENT

//  ================================================================
INT __cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
//  ================================================================
{
    if( FBstfHelpArg( argv[1] ) )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    return ErrBstfRunTests( argc, argv );
}
