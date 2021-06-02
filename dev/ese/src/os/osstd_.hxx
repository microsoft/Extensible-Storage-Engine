// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

//** SYSTEM ******************************************************************

#define _CRT_RAND_S

#include <stdlib.h>
#include <string.h>
#pragma prefast(push)
#pragma prefast(disable:26006, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:26007, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28718, "Dont bother us with tchar, someone else owns that.")
#pragma prefast(disable:28726, "Dont bother us with tchar, someone else owns that.")
#include <tchar.h>
#pragma prefast(pop)
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <minmax.h>
#include <ctype.h>

#include <specstrings.h>

#include <algorithm>
#include <functional>
#if _MSC_VER >= 1100
using namespace std;
#endif

#define Unused( var ) ( var )

//** COMPILE OPTIONS *********************************************************


//** COMPILER CONTROL ********************************************************

#pragma warning ( disable : 4200 )  //  we allow zero sized arrays
#pragma warning ( disable : 4201 )  //  we allow unnamed structs/unions
#pragma warning ( disable : 4355 )  //  we allow the use of this in ctor-inits
#pragma warning ( disable : 4706 )  //  assignment within conditional expression
#pragma warning ( 3 : 4244 )        //  do not hide data truncations
#pragma inline_depth( 255 )
#pragma inline_recursion( on )


//** OSAL ********************************************************************

#include "os.hxx"               //  OS Abstraction Layer


//** JET API *****************************************************************

#include "jet.h"                    //  Public JET API definitions


//** Win32/64 API ************************************************************

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  //  WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winnt.h>
#include <sddl.h>
#include <rpc.h>


//** Win32 Source API Sets ***************************************************

//  In ESE due to the variety of build and runtime environments that we must
//  run in we keep very strict control over what OS features we depend upon
//  and from where those features come from.
//
//  You should not try to load (or link for that matter) the OS from the
//  traditional kernel32.dll/wszKernel32, user32.dll/wszUser32, etc as these
//  DLLs are not available on CoreSystem.  You should also not load directly
//  from API sets like api-ms-win-core-synch-l1-1-0.dll/wszCoreSynch as we
//  want to be able to run downlevel.
//
//  You should be loading your OS functionality from one of the g_mwszzFoo
//  variables which has both and API set AND a downlevel / legacy DLL listed
//  in a multi-string so that the functionality can be used on CoreSystem for
//  Win8 or on legacy Win7/Vista boxes.
//

//  base libraries (legacy / downlevel)

#define wszNtdll                L"ntdll.dll"
#define wszKernel32             L"kernel32.dll"
#define wszAdvapi32             L"advapi32.dll"
#define wszPsapi                L"psapi.dll"
#define wszUser32               L"user32.dll"

//  newer base libraries

#define wszKernelBase           L"kernelbase.dll"   // win7 I _think_
#define wszMinUser              L"minuser32.dll"    // is this on phone?
#define wszKernel32Legacy       L"kernel32legacy.dll"   // win-phone


//  base libraries (API sets)

/*
    Note these are the base API sets available to us on Win7

    api-ms-win-core-console-l1-1-0, api-ms-win-core-datetime-l1-1-0, api-ms-win-core-delayload-l1-1-0,
    api-ms-win-core-errorhandling-l1-1-0, api-ms-win-core-fibers-l1-1-0, api-ms-win-core-handle-l1-1-0,
    api-ms-win-core-interlocked-l1-1-0, api-ms-win-core-io-l1-1-0, api-ms-win-core-libraryloader-l1-1-0,
    api-ms-win-core-memory-l1-1-0, api-ms-win-core-misc-l1-1-0, api-ms-win-core-namedpipe-l1-1-0,
    api-ms-win-core-processenvironment-l1-1-0, api-ms-win-core-profile-l1-1-0, api-ms-win-core-string-l1-1-0,
    api-ms-win-core-util-l1-1-0, api-ms-win-core-xstate-l1-1-0

*/

#define wszRtlSupport           L"api-ms-win-core-rtlsupport-l1-1-0.dll"        // win7

#define wszCoreSysInfo          L"api-ms-win-core-sysinfo-l1-1-0.dll"           // win7 - note only win8 has GetNativeSystemInfo present, even though win7 has the DLL / api-set
#define wszCoreSynch            L"api-ms-win-core-synch-l1-1-0.dll"             // win7 - only logically needed for sync.lib/cxx/hxx
#define wszCoreHeap             L"api-ms-win-core-heap-l1-1-0.dll"              // win7
#define wszCoreFile             L"api-ms-win-core-file-l1-1-0.dll"              // win7
#define wszCoreProcessThreads   L"api-ms-win-core-processthreads-l1-1-1.dll"    // win7
#define wszCoreThreadpool       L"api-ms-win-core-threadpool-l1-1-0.dll"        // win7
#define wszCoreLoc              L"api-ms-win-core-localization-l1-1-0.dll"      // win7
#define wszCoreReg              L"api-ms-win-core-localregistry-l1-1-0.dll"     // win7
#define wszSecBase              L"api-ms-win-security-base-l1-1-0.dll"          // win7
#define wszCoreDebug            L"api-ms-win-core-debug-l1-1-0.dll"             // win7

// #define wszCoreReg               L"api-ms-win-core-registry-l1-1-0.dll"          // win8 - using win7 localregistry one above instead
#define wszSecSddl              L"api-ms-win-security-sddl-l1-1-0.dll"          // win8 - in win7 had api-ms-win-security-base-l1-1-0, but not sure it accomplishes same thing
#define wszLsaLookup            L"api-ms-win-security-lsalookup-l2-1-0.dll"     // win8 - note part of minwin_legacy.lib, not sure what NT plans for this API
#define wszWorkingSet           L"api-ms-win-core-psapi-l1-1-0.dll"
#define szAppModelRuntime       L"api-ms-win-appmodel-runtime-l1-1-0.dll"       // Win8
#define szAppModelState         L"api-ms-win-appmodel-state-l1-1-0.dll"         // Win8
#define wszEventingProvider     L"api-ms-win-eventing-provider-l1-1-0.dll"      // win8
#define wszEventLogLegacy       L"api-ms-win-eventlog-legacy-l1-1-0.dll"        // win8
#define wszCoreKernel32Legacy   L"api-ms-win-core-kernel32-legacy-l1-1-0.dll"   // win8
#define wszCoreWow64            L"api-ms-win-core-wow64-l1-1-0.dll"             // win8

#define wszCoreFile12           L"api-ms-win-core-file-l1-2-0.dll"              // winBlue

#ifndef ESENT
#define wszCoreDebug12          L"api-ms-win-core-debug-l1-1-2.dll"             // win10
#endif

//  search paths for various API sets

//  note: if you add a primary OS library here, add it to ValidateProvidedLibraries() as well

const wchar_t * const g_mwszzNtdllLibs          = wszNtdll L"\0";       // NT hasn't seemed that interested in breaking up (or at least removing) ntdll into API sets yet
const wchar_t * const g_mwszzRtlSupportLibs     = wszRtlSupport L"\0" /* downlevel */ wszNtdll L"\0";

const wchar_t * const g_mwszzCpuInfoLibs        = wszCoreProcessThreads L"\0" /* downlevel */ wszKernel32 L"\0";
const wchar_t * const g_mwszzSysInfoLibs        = wszCoreSysInfo L"\0" /* downlevel */ wszKernel32 L"\0";
const wchar_t * const g_mwszzSyncLibs           = wszCoreSynch L"\0" /* downlevel */ wszKernel32 L"\0";
const wchar_t * const g_mwszzHeapLibs           = wszCoreHeap L"\0" /* downlevel */ wszKernel32 L"\0";
const wchar_t * const g_mwszzFileLibs           = wszCoreFile12 L"\0" wszCoreFile L"\0" /* downlevel */ wszKernel32 L"\0" /* medium level */ wszKernelBase L"\0";
const wchar_t * const g_mwszzThreadpoolLibs     = wszCoreThreadpool L"\0" /* downlevel */ wszKernel32 L"\0";
#ifdef ESENT
const wchar_t * const g_mwszzLocalizationLibs   = wszCoreLoc L"\0" /* downlevel */ wszKernel32 L"\0";
#else
const wchar_t * const g_mwszzLocalizationLibs   = wszCoreLoc L"\0" wszCoreDebug12 L"\0" /* downlevel */ wszKernel32 L"\0";
#endif
const wchar_t * const g_mwszzCoreDebugLibs      = wszCoreDebug L"\0" /* downlevel */ wszKernel32 L"\0";

const wchar_t * const g_mwszzRegistryLibs       = wszCoreReg L"\0" /* downlevel */ wszAdvapi32 L"\0";
const wchar_t * const g_mwszzSecSddlLibs        = wszSecSddl L"\0" /* downlevel */ wszAdvapi32 L"\0";
const wchar_t * const g_mwszzProcessTokenLibs   = wszCoreProcessThreads L"\0" /* downlevel */ wszAdvapi32 L"\0";
const wchar_t * const g_mwszzAdjPrivLibs        = wszSecBase L"\0" /* downlevel */ wszAdvapi32 L"\0";
const wchar_t * const g_mwszzLookupPrivLibs     = wszLsaLookup L"\0" /* downlevel */ wszAdvapi32 L"\0";

const wchar_t * const g_mwszzUserInterfaceLibs  = wszMinUser L"\0" /* downlevel */ wszUser32 L"\0";
const wchar_t * const g_mwszzWorkingSetLibs     = wszWorkingSet L"\0" /* downlevel */ wszPsapi L"\0";
const wchar_t * const g_mwszzProcessMemLibs     = /* Cant find API-set, use this one */ wszWorkingSet L"\0" /* downlevel - in two places */ wszPsapi L"\0" wszKernel32 L"\0";

const wchar_t * const g_mwszzErrorHandlingLegacyLibs = wszCoreKernel32Legacy L"\0" /* downlevel */ wszKernel32 L"\0";
const wchar_t * const g_mwszzWow64Libs          = wszCoreWow64 L"\0" /* downlevel */ wszKernel32 L"\0";

const wchar_t * const g_mwszzAppModelRuntimeLibs = szAppModelRuntime L"\0"; // no fallback, because if not presesnt, not running on win8 where it is needed
const wchar_t * const g_mwszzAppModelStateLibs  = szAppModelState L"\0";        // no fallback, because if not presesnt, not running on win8 where it is needed

const wchar_t * const g_mwszzEventingProviderLibs = wszEventingProvider L"\0" /* downlevel */ wszAdvapi32 L"\0";
const wchar_t * const g_mwszzEventLogLegacyLibs = wszEventLogLegacy L"\0" /* downlevel */ wszAdvapi32 L"\0";

const wchar_t * const g_mwszzKernel32CoreSystemBroken       = wszKernel32 L"\0" /* medium level */ wszKernelBase L"\0" /* up-level legacy */ wszKernel32Legacy L"\0";
const wchar_t * const g_mwszzAdvapi32CoreSystemBroken       = wszAdvapi32 L"\0";


#undef wszNtdll
#undef wszKernel32
#undef wszAdvapi32
#undef wszPsapi
#undef wszUser32
#define wszNtdll        wszNtdll_silly_you_this_dll_is_not_for_you_see_osstd_hxx
#define wszKernel32     wszKernel32_silly_you_this_dll_is_not_for_you_see_osstd_hxx
#define wszAdvapi32     wszAdvapi32_silly_you_this_dll_is_not_for_you_see_osstd_hxx
#define wszPsapi        wszPsapi_silly_you_this_dll_is_not_for_you_see_osstd_hxx
#define wszUser32       wszUser32_silly_you_this_dll_is_not_for_you_see_osstd_hxx


//** OS LAYER LOCK RANKS *****************************************************


const INT rankCritTaskList                  = 0;
const INT rankAESProv                       = 1;
const INT rankIoStats                       = 1;
const INT rankIOREQ                         = 2;
const INT rankTimerTaskList                 = 3;    //  Held only during TimerTask Schedule
const INT rankTimerTaskEntry                = 3;    //  Held during TimerTask Create, and Delete
const INT rankIOREQPoolCrit /* global */    = 3;
const INT rankOSDiskIOQueueCrit             = 4;    //  File IO, IO Mgr Disk Enum / Async IO Dispatch
const INT rankOSDiskSXWL /* global */       = 6;    //  ErrFileOpen, ErrFileCreate, IO Mgr Disk Enum / Async IO Dispatch
const INT rankFTLFlushBuffs                 = 6;
const INT rankFTLFlush                      = 7;
const INT rankFTLBuffer                     = 8;    //  ErrFTLTrace, FTL Init / Term
const INT rankOSVolumeSXWL /* global */     = 8;    //  ErrFileOpen, ErrFileCreate, IO Mgr Disk Enum / Async IO Dispatch
const INT rankRwlPostTasks                  = 9;


//** INTERNAL OS LAYER HEADERS ***********************************************

#include "_dev.hxx"
#include "_os.hxx"
#include "_ostls.hxx"
#include "_osdisk.hxx"
#include "_osfs.hxx"
#include "_osfile.hxx"
#include "_oserror.hxx"

#include "_reftrace.hxx"

#include "blockcache\_blockcache.hxx"

//** INTERNAL OS PROTOTYPES THAT ARE NOT IN HEADERS **************************

//  returns whether the current process is WOW64.

const BOOL FUtilIProcessIsWow64();



//** LAYER VIOLATIONS ********************************************************

#ifndef OS_LAYER_VIOLATIONS

//  UtilReortEvent() and the jetmsg.h are needed here to ensure the entire OS
//  layer can access the event func and constans when compiled for the
//  litent\oslite.lib version of this component.  More discussion of this is
//  in litent\violated.cxx.

//  Copied from eventu.hxx
VOID UtilReportEvent(
    const EEventType    type,
    const CategoryId    catid,
    const MessageId     msgid,
    const DWORD         cString,
    const WCHAR *       rgpszString[],
    const DWORD         cbRawData = 0,
    void *              pvRawData = NULL,
    const INST *        pinst = NULL,
    const LONG          lEventLoggingLevel = 1 );   //  1==JET_EventLoggingLevelMin

#include "_jetmsg.h"


//** ISOLATED OS FUNCTIONALITY ***********************************************

//  This section disables any OS functionality that we do not want broadly
//  used across the OS layer (or across ESE for that matter).  We do this by
//  defining the function name to direct the user to the right abstracted OS
//  layer functionality.  The true user (such as library.cxx which is allowed
//  to use LoadLibraryExW) simply has to put in a #undef LoadLibraryExW after
//  the precompiled header to allow access to the true OS functionality.

#define LoadLibraryA        LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define LoadLibraryW        LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define LoadLibraryExA      LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define LoadLibraryExW      LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader

#define GetModuleHandleA    GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define GetModuleHandleW    GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define GetModuleHandleExA  GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define GetModuleHandleExW  GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader

#define GetTickCount        GetTickCount_is_protected_function_use_TickOSTimeCurrent

#endif  // OS_LAYER_VIOLATIONS

