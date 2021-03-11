// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <cstdio>
#include <stdlib.h>

#include "testerr.h"
#include "bstf.hxx"

#pragma warning( disable : 4718 )


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


#ifdef DEBUG
#define Assert( exp )                               \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( #exp, __FILE__, __LINE__ );     \
    }
#else
#define Assert( exp )
#endif

#ifdef DEBUG
#define AssertSz( exp, sz )                         \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( sz, __FILE__, __LINE__ );       \
    }
#else
#define AssertSz( exp, sz )
#endif

