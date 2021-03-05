// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>

#include <esent.h>

#include "SvcInit.h"














INT ErrPrintUsageFail( const char *szAppName, const WCHAR * const szReason )
{
    wprintf( L"Usage: %hs [JetEnableMultiInstance|JET_paramConfigStoreSpec [StrDifferentiator]]\n\n", szAppName );
    wprintf( L"Failed: %ws\n\n", szReason );
    return JET_errInvalidParameter;
}

JET_ERR ErrFromStrsafeHr( const HRESULT hr )
{
    JET_ERR err = (hr == SEC_E_OK) ?
        JET_errSuccess :
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ?
            JET_errBufferTooSmall :
            (hr == STRSAFE_E_INVALID_PARAMETER) ?
                JET_errInvalidParameter :
                JET_errInternalError;
    assert( err == JET_errSuccess ||
            err == JET_errBufferTooSmall );
    return(err);
}

int __cdecl main(int argc, char ** argv)
{
    JET_ERR         err = JET_errSuccess;
    const CHAR *    szRegDiff = "Default";
    const CHAR *    szSample = NULL;


    if ( argc < 2 )
    {
        return ErrPrintUsageFail( argv[0], L"Not enough arguments, must specify the sub-sample to run (JetEnableMultiInstance|JET_paramConfigStoreSpec)." );
    }
    szSample = argv[1];

    if ( argc > 2 )
    {
        szRegDiff = argv[2];
    }


    wprintf( L"Begining SvcInit Sample (argc = %d, %hs)\n", argc, szSample );
    if ( 0 == _stricmp( szSample, "JetEnableMultiInstance" ) )
    {
        err = ErrEnableMultiInstStartSample();
    }
    else if ( 0 == _stricmp( szSample, "JET_paramConfigStoreSpec" ) )
    {
        WCHAR wszRegistryConfigPath[260];
        err = ErrFromStrsafeHr( StringCbPrintfW( wszRegistryConfigPath, sizeof(wszRegistryConfigPath), L"reg:HKLM\\SOFTWARE\\Microsoft\\Ese_SvcInitSample_%hs", szRegDiff ) );

        err = ErrConfigStorePathStartSample( wszRegistryConfigPath, FALSE );
    }
    else if ( 0 == _stricmp( szSample, "JET_paramConfigStoreSpec2" ) )
    {
        WCHAR               wszRegistryConfigPath[260];
        err = ErrFromStrsafeHr( StringCbPrintfW( wszRegistryConfigPath, sizeof(wszRegistryConfigPath), L"reg:HKLM\\SOFTWARE\\Microsoft\\Ese_SvcInitSample_%hs", szRegDiff ) );

        err = ErrConfigStorePathStartSample( wszRegistryConfigPath, TRUE );
    }
    else
    {
        return ErrPrintUsageFail( argv[0], L"Unknown sub-sample specified, must run JetEnableMultiInstance|JET_paramConfigStoreSpec|JET_paramConfigStoreSpec2." );
    }

    wprintf( L"\nSample done.\n\n" );
    
    return err;
}

