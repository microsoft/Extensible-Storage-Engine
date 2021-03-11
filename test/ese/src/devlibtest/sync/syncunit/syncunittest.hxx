// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if defined( BUILD_ENV_IS_NT ) || defined( BUILD_ENV_IS_WPHONE )
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#ifndef ESE_DISABLE_NOT_YET_PORTED
#include <windows.h>
#endif
#include <cstdio>
#include <stdlib.h>

#define BSTF_AVOID_WIN_DEPENDENCE 1
#include "testerr.h"
#include "bstf.hxx"

#pragma warning ( disable : 4100 )
#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4512 )
#pragma warning ( disable : 4706 )

DWORD DWGetTickCount();

void SetExpectedAsserts( ULONG cExpectedAsserts );
void ResetExpectedAsserts();
void AssertExpectedAsserts();


#ifdef DEBUG
#define OSSYNCAssert    CapturableTestAssert
#else
#endif

extern BOOL     g_fCaptureAssert;
extern CHAR*    g_szCapturedAssert;

#define CapturableTestAssert( _condition )                                      \
    g_cTests++;                                                         \
    if ( _condition )                                                   \
        {                                                               \
        g_cTestsSucceeded++;                                            \
        NULL;                                                           \
    }                                                               \
    else                                                                \
        {                                                               \
        if ( g_fCaptureAssert )                                         \
            {                                                           \
            if ( g_szCapturedAssert != NULL )                           \
                {                                                       \
                wprintf( L"\t\t\tDOUBLE-Assert( %hs ) ... Failed!\n", #_condition );    \
                *(INT*)NULL = 0x42;                                         \
            }                                                           \
            g_szCapturedAssert = #_condition;                               \
        }                                                               \
        else                                                                \
            {                                                               \
            g_cTestsFailed++;                                               \
            wprintf( L"\t\t\tAssert( %hs ) ... Failed!\n", #_condition );   \
            *(INT*)NULL = 0x42;                                             \
            exit( 1 );                                                      \
        }                                                               \
    }

#define TestAssertSzFnLn( _condition, _sz, _filename, _line )                                       \
    g_cTests++;                                                                                     \
    if ( _condition )                                                                               \
        {                                                                                           \
        g_cTestsSucceeded++;                                                                        \
        NULL;                                                                                       \
    }                                                                                           \
    else                                                                                            \
        {                                                                                           \
        g_cTestsFailed++;                                                                           \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed @ %d (in %hs)!\n", _sz, _line, _filename );       \
        *(INT*)NULL = 0x42;                                                                         \
        exit( 1 );                                                                                  \
    }

#define TestAssertSz( _condition, _sz )                                                         \
    g_cTests++;                                                                                 \
    if ( _condition )                                                                           \
        {                                                                                       \
        g_cTestsSucceeded++;                                                                    \
        NULL;                                                                                   \
    }                                                                                       \
    else                                                                                        \
        {                                                                                       \
        g_cTestsFailed++;                                                                       \
        wprintf( L"\t\t\tAssert( %hs ) ... Failed @ %d (in %hs)!\n", _sz, __LINE__, __FILE__ ); \
        *(INT*)NULL = 0x42;                                                                     \
        exit( 1 );                                                                              \
    }

