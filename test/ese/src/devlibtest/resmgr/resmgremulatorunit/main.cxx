// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"

#ifdef BUILD_ENV_IS_NT
#else
#pragma warning(disable:22018)
#endif
#include <strsafe.h>
#ifdef BUILD_ENV_IS_NT
#else
#pragma warning(default:22018)
#endif


INT _cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
{
    COSLayerPreInit oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        return true;
    }
    
    if ( ErrOSInit() != JET_errSuccess )
    {
        return true;
    }

    const JET_ERR err = ErrBstfRunTests( argc, argv );

    OSTerm();

    return err;
}
