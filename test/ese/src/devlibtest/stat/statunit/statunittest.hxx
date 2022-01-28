// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cstdio>
#include <stdlib.h>

#include "testerr.h"

#define BSTF_AVOID_WIN_DEPENDENCE 1
#include "bstf.hxx"

#pragma warning ( disable : 4100 )  // unreferenced formal parameter
#pragma warning ( disable : 4127 )  // conditional expression is constant
#pragma warning ( disable : 4512 )  // assignment operator could not be generated
#pragma warning ( disable : 4706 )  // assignment within conditional expression


//  ================================================================
//  stat.hxx required support

#ifdef DEBUG
#define Assert( exp )                               \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( #exp, __FILE__, __LINE__ );     \
    }
#define AssertSz( exp, sz )                         \
    if ( !(exp) )                                   \
        {                                           \
        AssertFail( sz, __FILE__, __LINE__ );       \
    }
#else   //  !DEBUG
#define Assert( exp )
#define AssertSz( exp, sz )
#endif

#define STATAssert      Assert
#define STATAssertSz    AssertSz

#define TestAssertSzFnLn( _condition, _sz, _filename, _line )                   \
    g_cTests++;                                     \
    if ( _condition )                               \
        {                                           \
        g_cTestsSucceeded++;                        \
        NULL;                                       \
    }                                           \
    else                                            \
        {                                           \
        g_cTestsFailed++;                           \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed @ %d (in %hs)!\n", _sz, _line, _filename );   \
        exit( 1 );                                  \
    }

#define TestAssertSz( _condition, _sz )                 \
    g_cTests++;                                     \
    if ( _condition )                               \
        {                                           \
        g_cTestsSucceeded++;                        \
        NULL;                                       \
    }                                           \
    else                                            \
        {                                           \
        g_cTestsFailed++;                           \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed @ %d (in %hs)!\n", _sz, __LINE__, __FILE__ ); \
        exit( 1 );                                  \
    }


_inline void AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... )
{
    TestAssertSzFnLn( false, szMessage, szFilename, lLine );
    *((INT*)0x42) = 0x42;
    exit(-1);
}

// Must implement somewhere else, to avoid the inline, which messes up the linking.
void __stdcall EnforceFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine );

#include "stat.hxx"

#define DEFINE_ENUM_FLAG_OPERATORS_BASIC(ENUMTYPE)  \
    extern "C++" {                                  \
        inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) | ((INT)b)); }               \
        inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((INT)a) & ((INT)b)); }               \
        inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((INT)a)); }                                     \
}

//
//  From histoutil.cxx
//
//  Contains common histogram testing
//

enum TestZeroFlags { fTestZeroNormal = 0x00, fHasExplicitBuckets = 0x01, fNoPercentileSupport = 0x2 };
DEFINE_ENUM_FLAG_OPERATORS_BASIC( TestZeroFlags )
CStats::ERR ErrTestZerodHisto( CStats * pHistoClass, const enum TestZeroFlags fZeroTestFlags = fTestZeroNormal );

