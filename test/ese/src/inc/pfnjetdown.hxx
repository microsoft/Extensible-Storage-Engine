// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
//  Virtual PFN Jet Layer designed to work with downlevel DLLs
//

#pragma once

//  A clever way to ensure we find our DLL ...
//    - first we try ese.dll (which will only be available in Exch test installs) 
//    - and then try esent.dll (which will be available in windows deployments).
//  Note: The SysInitUnitTestRigLoadsTheRightEseDll::ErrTest checks that we actually
//  load the right ese or esent DLL.
//
//const wchar_t * const g_mwszzEse = L"ese.dll\0esent.dll\0";
//
//  Update: Initially very clever, but ultimately fatal when re-used for downlevel testing ... when we run downlevel where
//  the downlevel test is trying access a newer API (not available), it then screws up and loads the NT variant from
//  ese_NT_.dll!JetGetSessionParameter( which is uninit of course ) instead of just failing with the error thunk because
//  the downlevel ese.dll doesn't have it.  This #ifdef will unfortunately potentially spread how many / what binaries need
//  to be compiled with this knowledge.  Sigh.  Still would work if you just want to dynamically load two DLLs for like
//  perf A vs. B testing, which I have in the past.
//
#ifdef ESENT
const wchar_t * const g_mwszzEse = L"esent.dll\0";
#else
const wchar_t * const g_mwszzEse = L"ese.dll\0";
#endif

//  Now include the whole JET API implementation

#include "_pfnjet.hxx"
