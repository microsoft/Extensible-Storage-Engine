// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "syncunittest.hxx"

ULONG   g_cExpectedAsserts;
ULONG   g_cAssertsFound;

void AssertExpectedAsserts()
{
    //
    //  No validation for retail bits. We could have disabled the entire test, but
    //  running in RETAIL the code that is expected to assert in DEBUG still has
    //  some value.
    //
    
#ifdef DEBUG
    if ( g_cExpectedAsserts != g_cAssertsFound )
    {
        wprintf( L"\t\t\tDid not find the expect amount of asserts!\n" );
        *((INT*)NULL) = 0;
        exit(-1);
    }
#else
    wprintf( L"\t\t\tThis is a retail build. Skipping!\n" );
#endif
}

void SetExpectedAsserts( ULONG cExpectedAsserts )
{

    ResetExpectedAsserts();
    g_cExpectedAsserts = cExpectedAsserts;
}

void ResetExpectedAsserts()
{

    g_cExpectedAsserts = 0;
    g_cAssertsFound = 0;
}

//
//  Support functions needed to link the sync library properly.
//  

void AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... )
{
    if ( g_cExpectedAsserts > g_cAssertsFound )
    {

        wprintf( L"\t\t\tIgnoring Expected Assert( %hs ) ... @ %d (in %hs)!\n", szMessage, lLine, szFilename );     \
        g_cAssertsFound++;
        return;
    }
    
    TestAssertSzFnLn( false, szMessage, szFilename, lLine );
    *((INT*)NULL) = 0;
    exit(-1);
}

void EnforceFail( const CHAR * szMessage, const CHAR * szFilename, LONG lLine )
{
    TestAssertSzFnLn( false, szMessage, szFilename, lLine );
    *((INT*)NULL) = 0;
    exit(-1);
}
