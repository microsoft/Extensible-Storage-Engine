// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Includes the header files from the dev code to get the LoadLibrary functors working.

#pragma once

// types.hxx needs _lrotl.
#include <stdlib.h>

#pragma push_macro( "Assert" )
#pragma push_macro( "OnDebug" )

// esetest's AssertSz() doesn't do varargs (2012/10/31).
#ifndef Assert
#define Assert( exp )               AssertSz( exp, #exp )
#endif
#define Expected    Assert
#define ExpectedSz  AssertSz

/*
#ifndef AssertSz
#define AssertSz( exp, sz, ... ) \
   if ( ! (exp) ) { \
     printf( "Assertion failed! " ); \
     printf( sz, __VA_ARGS__ ); \
     printf( "\n" ); \
   }
#endif
*/

#ifdef DEBUG
#define OnDebug( code )             code
#else  //  !DEBUG
#define OnDebug( code )
#endif  //  DEBUG

// types.hxx has a nameless struct/union (QWORDX).
#pragma warning( push )
#pragma warning ( disable : 4201 )
#include "cc.hxx"
#include "types.hxx"
#pragma warning( pop )
#include "string.hxx"
#include "library.hxx"

#pragma pop_macro( "Assert" )
#pragma pop_macro( "OnDebug" )


