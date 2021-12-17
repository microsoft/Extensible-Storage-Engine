// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

void OSDiagTrackInit( _In_z_ const WCHAR* wszInstDisplayName, _In_ QWORD qwLogSignHash, _In_ const ERR errInit );
void OSDiagTrackEnforceFail( _In_z_ const WCHAR* wszContext, _In_z_ const CHAR* szMessage, _In_z_ const WCHAR* wszIssueSource );
void OSDiagTrackAssertFail( _In_z_ const CHAR* szMessage, _In_z_ const WCHAR* wszIssueSource );
void OSDiagTrackHungIO( _In_ BOOL fWrite, _In_ ULONG cbRun, _In_ DWORD csecHangTime );
void OSDiagTrackWriteError( _In_ ERR err, _In_ DWORD errorSystem, _In_ QWORD cmsecIOElapsed );
void OSDiagTrackFileSystemError( _In_ ERR err, _In_ DWORD errorSystem );
void OSDiagTrackJetApiError( _In_z_ const WCHAR* wszInstDisplayName, _In_ INT op, _In_ ERR err );
void OSDiagTrackRepair( _In_ ERR err );
void OSDiagTrackEventLog( _In_ const MessageId msgid );
void OSDiagTrackCorruptLog( _In_ const DWORD reason );
void OSDiagTrackDeferredAttach( _In_ const DWORD reason );
