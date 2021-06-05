// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <windows.h>
#include <cstdio>
#include <stdlib.h>
#include <tchar.h>

#include "testerr.h"
#include "bstf.hxx"

#pragma warning( disable : 4718 ) //  recursive call has no side effects, deleting


//  ================================================================
//  collection.hxx required support

inline void COLLUNITEnforceFail( const char* szMessage, const char* szFilename, LONG lLine )
{
    g_cTestsFailed++;
    wprintf( L"\t\t\tCOLLUNITEnforce( %hs ) ... Failed @ %d!\n", szMessage, lLine );
    RaiseFailFastException( NULL, NULL, 0 );
}

inline void COLLUNITAssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... )
{
    g_cTestsFailed++;
    wprintf( L"\t\t\tCOLLUNITAssert( %hs ) ... Failed in %hs @ %d!\n", szMessage, szFilename, lLine );
    RaiseFailFastException( NULL, NULL, 0 );
}


#ifdef DEBUG
#define COLLAssert( exp )                                   \
    if ( !(exp) )                                           \
    {                                                       \
        COLLUNITAssertFail( #exp, __FILE__, __LINE__ );     \
    }
#else   //  !DEBUG
#define COLLAssert( exp )
#endif

#ifdef DEBUG
#define COLLAssertSz( exp, sz, ...  )                                   \
    if ( !(exp) )                                                       \
    {                                                                   \
        COLLUNITAssertFail( sz, __FILE__, __LINE__, __VA_ARGS__ );      \
    }
#else   //  !DEBUG
#define COLLAssertSz( exp, sz, ... )
#endif

#define COLLEnforce( exp )                                              \
    if ( !(exp) )                                                       \
    {                                                                   \
        COLLUNITEnforceFail( #exp, __FILE__, __LINE__ );                \
    }

#define COLLAssertTrack( exp )                                          \
    if ( !(exp) )                                                       \
    {                                                                   \
        COLLUNITEnforceFail( #exp, __FILE__, __LINE__ );                \
    }

#include "os.hxx"
#include "sync.hxx"
#include "collection.hxx"





