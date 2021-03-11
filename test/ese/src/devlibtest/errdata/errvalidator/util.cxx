// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "errvalidator.hxx"

void AssertFail( const char * szMessage, const char * szFilename, LONG lLine, ... )
{   
    TestAssertSzFnLn( false, szMessage, szFilename, lLine );
    *((INT*)NULL) = 0;
    exit(-1);
}

void __stdcall EnforceFail( const CHAR * szMessage, const CHAR * szFilename, LONG lLine )
{
    TestAssertSzFnLn( false, szMessage, szFilename, lLine );
    *((INT*)NULL) = 0;
    exit(-1);
}
