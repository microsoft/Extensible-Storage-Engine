// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
void __stdcall EsetestAssertFail( const CHAR* szMessage, const CHAR* szFilename, long lLine );


#ifdef DEBUG
//#define AssertSz( exp, sz )       ( ( exp ) ? (void) 0 : EsetestAssertFail( _T( sz ), _T( __FILE__ ), __LINE__ ) )
#define AssertSz( exp, sz )         ( ( exp ) ? (void) 0 : EsetestAssertFail( sz, __FILE__, __LINE__ ) )
#define VerifySz( exp, sz )         AssertSz( exp, sz )
#else  //  !DEBUG
#define AssertSz( exp, sz )
#define VerifySz( exp, sz )         ( ( void )( exp ) )
#endif  //  DEBUG


