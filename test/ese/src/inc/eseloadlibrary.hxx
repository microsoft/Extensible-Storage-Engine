// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Copy a few things from other ESE dev headers we need for library.hxx.  Include library.hxx in order
//     to get LoadLibrary functors working.


#pragma once

#include <strsafe.h>

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


// From cc.hxx
typedef int       BOOL;
typedef INT       ERR;

#define fFalse  BOOL( 0 )


// From types.hxx
typedef ULONG_PTR LIBRARY;
typedef INT_PTR   (*PFN)();

#define INLINE inline

#define DEFINE_ENUM_FLAG_OPERATORS_BASIC(ENUMTYPE)  \
    extern "C++" {                                  \
        inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) | ((INT)b)); }               \
        inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) & ((INT)b)); }               \
        inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((INT)a)); }                                     \
    }      


// From string.hxx
#ifndef OSStrCbCopyW  // We might have already picked this one up.
#define OSStrCbCopyW( wszDst, cbDst, wszSrc )                           \
    {                                                                   \
        if( SEC_E_OK != ( StringCbCopyW( wszDst, cbDst, wszSrc ) ) )    \
        {                                                               \
            AssertSz( fFalse, "Success expected");                      \
        }                                                               \
    }
#endif

#include "library.hxx"

#pragma pop_macro( "Assert" )
#pragma pop_macro( "OnDebug" )


