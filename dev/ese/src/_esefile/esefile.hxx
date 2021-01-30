// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <cstdio>
#include <cstdlib>
#include <wchar.h>

#include "cc.hxx"
#include "math.hxx"
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)
#include "os.hxx"

#include "jet.h"
#include "tcconst.hxx"

#pragma warning ( disable : 4127 )
#pragma warning ( disable : 4706 )

const INT g_cpgDBReserved   = 2;

extern INT g_cbPage;
extern INT g_cpagesPerBlock;

const INT g_cbBufferTotal       = 512 * 1024;
const INT g_cbReadBuffer        = 256 * 1024;

inline void InitPageSize( const ULONG cbPage )
{
    g_cbPage = cbPage;
    
    g_cpagesPerBlock    = g_cbReadBuffer / g_cbPage;
}

inline __int64 OffsetOfPgno( const DWORD pgno )
{
    return __int64( pgno - 1 + g_cpgDBReserved ) * g_cbPage;
}

void PrintWindowsError( const wchar_t * const szMessage );
BOOL FChecksumFile(
    const wchar_t * const   szFile,
    const ULONG             cbpage,
    const ULONG             cPagesToChecksum,
    BOOL * const            pfBadPagesDetected );
JET_ERR ErrCopyFile(
    const wchar_t * const szFileSrc,
    const wchar_t * const szFileDest,
    BOOL fIgnoreDiskErrors );

void InitStatus( const wchar_t * const szOperation );
void UpdateStatus( const INT iPercentage );
void TermStatus();

