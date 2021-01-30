// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

BOOL FOSDiagPreinit()
{
    return fTrue;
}

ERR ErrOSDiagInit()
{
    return JET_errSuccess;
}

void OSDiagTerm()
{
}

void OSDiagPostterm()
{
}


void OSDiagTrackInit( _In_z_ const WCHAR* wszInstDisplayName, _In_ const QWORD qwLogSignHash, _In_ const ERR errTracked )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "InitErr: %I64x, %d\n", qwLogSignHash, errTracked ) );
}


void OSDiagTrackEnforceFail( _In_z_ const WCHAR* wszContext, _In_z_ const CHAR* szMessage, _In_z_ const WCHAR* wszIssueSource )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "EnforceFail: %ws, %ws, %hs\n", wszIssueSource, wszContext, szMessage ) );
}

void OSDiagTrackAssertFail( _In_z_ const CHAR* szMessage, _In_z_ const WCHAR* wszIssueSource )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "AssertFail: %ws, %hs\n", wszIssueSource, szMessage ) );
}

void OSDiagTrackHungIO( _In_ BOOL fWrite, _In_ ULONG cbRun, _In_ DWORD csecHangTime )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "HungIo: %hs, %d, %d )", fWrite ? "Write" : "Read", cbRun, csecHangTime ) );
}

void OSDiagTrackWriteError( _In_ ERR err, _In_ DWORD errorSystem, _In_ QWORD cmsecIOElapsed )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "TrackFileWriteError: %d, %d, %I64d\n", err, errorSystem, cmsecIOElapsed ) );
}

void OSDiagTrackFileSystemError( _In_ ERR err, _In_ DWORD errorSystem )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "TrackFileError: %d, %d\n", err, errorSystem ) );
}

void OSDiagTrackJetApiError( _In_z_ const WCHAR* wszInstDisplayName, _In_ INT op, _In_ ERR err )
{
}

void OSDiagTrackRepair( _In_ ERR err )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "RepairFailErr: %d\n", err ) );
}

void OSDiagTrackEventLog( _In_ const MessageId msgid )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "TrackEventLog: %d\n", msgid ) );
}

void OSDiagTrackCorruptLog( _In_ const DWORD reason )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "CorruptLog: %d\n", reason ) );
}

void OSDiagTrackDeferredAttach( _In_ const DWORD reason )
{
    OSTrace( JET_tracetagDiagnostics, OSFormat( "DeferredAttach: %d", reason ) );
}

