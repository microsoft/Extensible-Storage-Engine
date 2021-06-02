// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "collectionunittest.hxx"

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
    ERR err;

    if( argc == 2
        && ( 0 == _stricmp( argv[1], "-h" )
            || 0 == _stricmp( argv[1], "/h" )
            || 0 == _stricmp( argv[1], "-?" )
            || 0 == _stricmp( argv[1], "/?" ) ) )
    {
        PrintHelp( argv[0] );
        return 0;
    }

    COSLayerPreInit oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        return err;
    }
        
    COSLayerPreInit::SetDefaults();
    err = ErrOSInit();
    if ( JET_errSuccess != err )
    {
        wprintf( L"ErrOSInit failed, error:%d", err );
        return err;
    }

    FOSSyncPreinit();
    
    err = ErrBstfRunTests( argc, argv );

    OSTerm();
    
    return err;
}

