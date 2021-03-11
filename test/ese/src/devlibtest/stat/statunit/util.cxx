// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "statunittest.hxx"

void __stdcall EnforceFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine )
{
    TestAssertSzFnLn( false, szMessage, szFilename, lLine );
    exit(-1);
}
