// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if defined( BUILD_ENV_IS_NT ) || defined( BUILD_ENV_IS_WPHONE )
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include <windows.h>
#include <cstdio>
#include <stdlib.h>
#include <stddef.h>

#include "testerr.h"
#include "bstf.hxx"

#pragma warning ( disable : 4100 )
#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4512 )
#pragma warning ( disable : 4706 )

DWORD DWGetTickCount();


#ifdef DEBUG
#define Assert( exp )                               \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( #exp, __FILE__, __LINE__ );     \
    }
#define AssertSz( exp, sz ) Assert( exp )
#else
#define Assert( exp )
#define AssertSz( exp, sz )
#endif

#define DHTAssert       Assert
#define COLLAssert      Assert
#define RESMGRAssert    Assert

_inline void AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... )
{
    g_cTestsFailed++;
    wprintf( L"\t\t\tUnit Test Assert( %hs ) ... Failed %hs @ %d!\n", szMessage, szFilename, lLine );
    DebugBreak();
    exit(1);
}


const LONG g_pctCachePriorityMin        = 0;
const LONG g_pctCachePriorityMax        = 1000;
const LONG g_pctCachePriorityMaxMax     = (LONG)wMax;
const LONG g_pctCachePriorityDefault    = 100;
#define FIsCachePriorityValid( pctCachePriority )   ( ( (LONG)(pctCachePriority) >= g_pctCachePriorityMin ) && ( (LONG)(pctCachePriority) <= g_pctCachePriorityMax ) )


inline ULONG TickOSTimeCurrent()
{
    return GetTickCount();
}

