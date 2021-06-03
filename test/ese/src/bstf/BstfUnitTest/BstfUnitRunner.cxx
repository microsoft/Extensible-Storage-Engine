// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

#include "testerr.h"
#include "bstf.hxx"


#ifndef BSTF_AVOID_WIN_DEPENDENCE

//  ================================================================
ULONG64 QWQueryPerformanceCounter()
// Returns the number of milliseconds, using QueryPerformanceFrequency()
// if possible.
//  ================================================================
    {
        static bool fCheckedForQpfSupport       = false;
        static bool fUseQueryPerformanceCounter = false;
        static ULONG64  ulDivisor   = 1;
        LARGE_INTEGER   liResult;

        if ( !fCheckedForQpfSupport )
        {
            LARGE_INTEGER   liTemp;
            // Not synchronized, but that shouldn't be a problem.
            if ( QueryPerformanceFrequency( &liTemp ) )
            {
                // Since we want a result in milliseconds.
                ulDivisor = liTemp.QuadPart / 1000;
                fUseQueryPerformanceCounter = true;
            }
            fCheckedForQpfSupport = true;
        }

        if ( fUseQueryPerformanceCounter )
        {
            QueryPerformanceCounter( &liResult );
            return liResult.QuadPart / ulDivisor;
        }
        else
        {
            return (ULONG64) GetTickCount();
        }
    }

#endif // !BSTF_AVOID_WIN_DEPENDENCE

//  ================================================================
DWORD DWGetTickCount()
//  ================================================================
    {
#ifndef ESE_DISABLE_NOT_YET_PORTED
    return (DWORD) QWQueryPerformanceCounter();
#else
    return 0;
#endif
    }


//  ================================================================
void BstfPrintTests()
//  ================================================================
    {
    const UNITTEST * punittest = UNITTEST::s_punittestHead;
    while( punittest )
        {
        fprintf( stderr, "==> %s\r\n%s\r\n\r\n", punittest->SzName(), punittest->SzDescription() );
        punittest = punittest->m_punittestNext;
        }
    }


QWORD g_bvlDefault = bvlPrintTests - 1;
QWORD g_bvl;

QWORD g_cSuppressedTestSuccesses;
DWORD g_tickLastSuppressStatus;

void BstfSetVerbosity( QWORD bvl )
    {
    g_bvl = bvl;
    }

#ifdef _MSC_VER
#define NOINLINE    __declspec(noinline)
#else
//  Hopefully this won't get inlined / optimized on other platforms.
#define NOINLINE
#endif

//  Pattern fill 1 KB worth of stack, with a non-repeating pattern (0xFEnnnnnn) of 32-bit values.  No
//  32-bits will be all zeros.  Try to force it to be a function to clobber the entire stack area of
//  the next function to be called (namely the test is run right after this).

NOINLINE void BstfTrashTestStackWithPattern()
    {
    volatile ULONG  rgul[1024 / sizeof( ULONG )];
    const ULONG     cul = sizeof(rgul)/sizeof(*rgul);

    for ( ULONG iul = 0; iul < cul; iul++ )
        {
        //  We are walking the count forward, but we do the array backwards b/c stacks "grow down 
        //  in memory"... and so it appears it counts backwards from high addresses to low addresses.
        rgul[cul-1-iul] = 0xFE000000 | ( ( iul + 1 ) & 0x00FFFFFF );
        }

    printf( "Trashed stack from %p to %p \n", &( rgul[0] ), &( rgul[cul-1] ) );
#ifndef DEBUG
    //  The retail compiler is too smart ... you actually have to use the variable for retail
    //  or the above code completely disappears ... not what we want, but I have to say hats
    //  off to you compiler team for being so smart!  ;-)
    const ULONG iul = rand() % cul;
    printf( "Trashed stack @ %p to 0x%X for instance\n", &( rgul[iul] ), rgul[iul] );
#endif
    }


//  ================================================================
static ERR ErrRunTest( UNITTEST * const punittest )
    {
    printf( "==> %s\r\n", punittest->SzName() );

    const unsigned int cMsecStart = DWGetTickCount();

    QWORD cTests = g_cTests;
    QWORD cTestsSucceeded = g_cTestsSucceeded;
    QWORD cTestsFailed = g_cTestsFailed;

    if ( punittest->m_btcf & btcfForceStackTrash )
        {
        BstfTrashTestStackWithPattern();
        }
    ERR err = punittest->ErrTest( );

    const unsigned int cMsecElapsed = DWGetTickCount() - cMsecStart;

    if( JET_errSuccess == err )
        {
        PrintClearSuppressedChecks();
        printf( "==> %s finishes in %d milliseconds (%lld/%lld/%lld tests)\r\n", punittest->SzName(), cMsecElapsed,
                    g_cTestsSucceeded - cTestsSucceeded, g_cTestsFailed - cTestsFailed, g_cTests - cTests );
        }
    else
        {
        printf( "==> %s completes with err %d after %d milliseconds\r\n", punittest->SzName(), err, cMsecElapsed );
        }

    BstfSetVerbosity( g_bvlDefault );

    return err;
    }

//  ================================================================
static ERR ErrRunAllTests( void )
//  ================================================================
    {
    ERR err = JET_errSuccess;

    UNITTEST * punittest = UNITTEST::s_punittestHead;
    while( punittest )
        {
        if ( punittest->FRunByDefault() )
            {
            TestCall( ErrRunTest( punittest ) );
            }
        punittest = punittest->m_punittestNext;
        }

HandleError:
    return err;
    }

static ERR ErrRunAllAllTests( void )
//  ================================================================
    {
    ERR err = JET_errSuccess;

    UNITTEST * punittest = UNITTEST::s_punittestHead;
    while( punittest )
        {
        TestCall( ErrRunTest( punittest ) );
        punittest = punittest->m_punittestNext;
        }

HandleError:
    return err;
    }

//  ================================================================
static ERR ErrRunOneTest( const char * const szTest )
//  ================================================================
    {
    UNITTEST * punittest = UNITTEST::s_punittestHead;
    while( punittest )
        {
        if( 0 == _stricmp( szTest, punittest->SzName() ) )
            {
            return ErrRunTest( punittest );
            }
        punittest = punittest->m_punittestNext;
        }

    fprintf( stderr, "test '%s' was not found!\r\n", szTest );
    fprintf( stderr, "Available tests are:\r\n\r\n" );
    BstfPrintTests();
    return JET_errInvalidParameter;
    }


//  ================================================================
static ERR ErrRunSelectedTests( const char * szTestSpec )
//  ================================================================
    {
    ERR err = JET_errSuccess;

    int cchTestSpec = strlen( szTestSpec );
    const BOOL fPrefixMatch = ( szTestSpec[cchTestSpec-1] == '*' );
    const BOOL fSuffixMatch = ( szTestSpec[0] == '*' );
    if ( fPrefixMatch && fSuffixMatch )
        {
        //  Nope!
        return JET_errInvalidParameter;
        }

    if ( fPrefixMatch )
        {
        cchTestSpec--;
        }
    if ( fSuffixMatch )
        {
        szTestSpec++;
        }

    ULONG cTestsRun = 0;
    UNITTEST * punittest = UNITTEST::s_punittestHead;
    while( punittest )
        {
        const int cchCurrTestName = strlen( punittest->SzName() );

        if( 0 == _stricmp( szTestSpec, punittest->SzName() ) ||
            ( fPrefixMatch && 
                ( 0 == _strnicmp( szTestSpec, punittest->SzName(), cchTestSpec ) ) ) ||
            ( fSuffixMatch && 
                ( cchCurrTestName >= cchTestSpec ) && 
                ( 0 == _stricmp( szTestSpec, &(punittest->SzName()[ cchCurrTestName - cchTestSpec + 1 ]) ) ) ) )
            {
            TestCall( ErrRunTest( punittest ) );
            cTestsRun++;
            }
        else
            {
            printf( "==> skipping %s (name does not match).\r\n", punittest->SzName() );
            }
        punittest = punittest->m_punittestNext;
        }

HandleError:

    if ( err >= JET_errSuccess && cTestsRun == 0 )
        {
        err = JET_errTestError;
        }

    return err;
    }


QWORD   g_cTests;
QWORD   g_cTestsSucceeded;
QWORD   g_cTestsFailed;

//  ================================================================
ERR ErrBstfRunTests( const int argc, const char * const argv[] )
//  ================================================================
    {
    ERR err = JET_errSuccess;

    const unsigned int cMsecStart = DWGetTickCount();

    g_cTests = 0;
    g_cTestsSucceeded = 0;
    g_cTestsFailed = 0;

    g_tickLastSuppressStatus = DWGetTickCount() - g_dtickSuppressStatusUpdateDelay - 1;

    if( 1 == argc )
        {
        err = ErrRunAllTests();
        if( err )
            {
            goto HandleError;
            }
        }
    else if ( 2 == argc && ( 0 == _stricmp( "-all", argv[1] ) ) )
        {
        err = ErrRunAllAllTests();
        if( err )
            {
            goto HandleError;
            }
        }
    else
        {
        int isz;
        for( isz = 1; isz < argc; ++isz )
            {
            const int cch = strlen( argv[isz] );
            if ( argv[isz][0] == '*' || argv[isz][cch-1] == '*' )
                {
                //  Wild card cases
                //wprintf( L"Wildcard selector ... \n");
                err = ErrRunSelectedTests( argv[isz] );
                }
            else 
                {
                err = ErrRunOneTest( argv[isz] );
                }
            if ( err )
                {
                goto HandleError;
                }
            }
        }

HandleError:
    const unsigned int cMsecElapsed = DWGetTickCount() - cMsecStart;

    printf( "%s ran %d tests (success = %lld, failed = %lld).\r\n", argv[0], g_cTests, g_cTestsSucceeded, g_cTestsFailed );
    if ( err >= 0 )
        {
        printf( "%s completes successfully in %d milliseconds.\r\n", argv[0], cMsecElapsed );
        }
    else
        {
        printf( "%s completes with err = %d after %d milliseconds\r\n", argv[0], err, cMsecElapsed );
        }

    return err;
    }

