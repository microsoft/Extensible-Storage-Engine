// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <cstdio>
#include <stdlib.h>

#include "testerr.h"
#include "bstf.hxx"

#pragma warning( disable : 4718 ) //  recursive call has no side effects, deleting

//  According to MSDN:
//  Beginning in Visual C++ .NET 2002, the CRT's new function (in libc.lib, libcd.lib,
//  libcmt.lib, libcmtd.lib, msvcrt.lib, and msvcrtd.lib) will continue to return NULL
//  if memory allocation fails. However, the new function in the Standard C++
//  Library (in libcp.lib, libcpd.lib, libcpmt.lib, libcpmtd.lib, msvcprt.lib, and
//  msvcprtd.lib) will support the behavior specified in the C++ standard, which is to
//  throw a std::bad_alloc exception if the memory allocation fails.
//  That is why we need to override these operators here. ESE's implementation in
//  collection.hxx assumes "new" returns NULL in case of memory allocation failure
//  so we don't want the test code to throw exceptions for negative test cases, for
//  example.

inline void* __cdecl operator new( const size_t cbSize )
{
    return HeapAlloc( GetProcessHeap(), 0, cbSize );
}

inline void* __cdecl operator new[]( const size_t cbSize )
{
    return HeapAlloc( GetProcessHeap(), 0, cbSize );
}

inline void __cdecl operator delete( void* const pv )
{
    HeapFree( GetProcessHeap(), 0, pv );
}

inline void __cdecl operator delete[]( void* const pv )
{
    HeapFree( GetProcessHeap(), 0, pv );
}

//  ================================================================
//  collection.hxx required support

#ifdef DEBUG
#define Assert( exp )                               \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( #exp, __FILE__, __LINE__ );     \
    }
#else   //  !DEBUG
#define Assert( exp )
#endif

#ifdef DEBUG
#define AssertSz( exp, sz )                         \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( sz, __FILE__, __LINE__ );       \
    }
#else   //  !DEBUG
#define AssertSz( exp, sz )
#endif

