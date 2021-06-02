// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "resmgremulatorunit.hxx"

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
INT _cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
//  ================================================================
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
