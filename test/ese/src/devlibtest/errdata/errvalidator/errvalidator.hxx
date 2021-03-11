// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef BUILD_ENV_IS_NT
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include <windows.h>
#include <cstdio>
#include <stdlib.h>


DWORD DWGetTickCount();


#define TestAssert( _condition )                                        \
    if ( !( _condition ) )                                                  \
        {                                                               \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed!\n", #_condition );   \
        exit( 1 );                                                      \
    }

#define TestAssertSzFnLn( _condition, _sz, _filename, _line )                                       \
    if ( !( _condition ) )                                                                              \
        {                                                                                           \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed @ %d (in %hs)!\n", _sz, _line, _filename );       \
        exit( 1 );                                                                                  \
    }

#define TestAssertSz( _condition, _sz )                                                         \
    if ( !( _condition ) )                                                                          \
        {                                                                                       \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed @ %d (in %hs)!\n", _sz, __LINE__, __FILE__ ); \
        exit( 1 );                                                                              \
    }

