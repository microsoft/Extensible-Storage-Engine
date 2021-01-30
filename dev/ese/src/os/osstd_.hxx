// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.



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




#pragma warning ( disable : 4200 )
#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4355 )
#pragma warning ( disable : 4706 )
#pragma warning ( 3 : 4244 )
#pragma inline_depth( 255 )
#pragma inline_recursion( on )



#include "os.hxx"



#include "jet.h"



#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winnt.h>
#include <sddl.h>
#include <rpc.h>





#define wszNtdll                L"ntdll.dll"
#define wszKernel32             L"kernel32.dll"
#define wszAdvapi32             L"advapi32.dll"
#define wszPsapi                L"psapi.dll"
#define wszUser32               L"user32.dll"


#define wszKernelBase           L"kernelbase.dll"
#define wszMinUser              L"minuser32.dll"
#define wszKernel32Legacy       L"kernel32legacy.dll"





#define wszRtlSupport           L"api-ms-win-core-rtlsupport-l1-1-0.dll"

#define wszCoreSysInfo          L"api-ms-win-core-sysinfo-l1-1-0.dll"
#define wszCoreSynch            L"api-ms-win-core-synch-l1-1-0.dll"
#define wszCoreHeap             L"api-ms-win-core-heap-l1-1-0.dll"
#define wszCoreFile             L"api-ms-win-core-file-l1-1-0.dll"
#define wszCoreProcessThreads   L"api-ms-win-core-processthreads-l1-1-1.dll"
#define wszCoreThreadpool       L"api-ms-win-core-threadpool-l1-1-0.dll"
#define wszCoreLoc              L"api-ms-win-core-localization-l1-1-0.dll"
#define wszCoreReg              L"api-ms-win-core-localregistry-l1-1-0.dll"
#define wszSecBase              L"api-ms-win-security-base-l1-1-0.dll"
#define wszCoreDebug            L"api-ms-win-core-debug-l1-1-0.dll"

#define wszSecSddl              L"api-ms-win-security-sddl-l1-1-0.dll"
#define wszLsaLookup            L"api-ms-win-security-lsalookup-l2-1-0.dll"
#define wszWorkingSet           L"api-ms-win-core-psapi-l1-1-0.dll"
#define szAppModelRuntime       L"api-ms-win-appmodel-runtime-l1-1-0.dll"
#define szAppModelState         L"api-ms-win-appmodel-state-l1-1-0.dll"
#define wszEventingProvider     L"api-ms-win-eventing-provider-l1-1-0.dll"
#define wszEventLogLegacy       L"api-ms-win-eventlog-legacy-l1-1-0.dll"
#define wszCoreKernel32Legacy   L"api-ms-win-core-kernel32-legacy-l1-1-0.dll"
#define wszCoreWow64            L"api-ms-win-core-wow64-l1-1-0.dll"

#define wszCoreFile12           L"api-ms-win-core-file-l1-2-0.dll"

#ifndef ESENT
#define wszCoreDebug12          L"api-ms-win-core-debug-l1-1-2.dll"
#endif



const wchar_t * const g_mwszzNtdllLibs          = wszNtdll L"\0";
const wchar_t * const g_mwszzRtlSupportLibs     = wszRtlSupport L"\0"  wszNtdll L"\0";

const wchar_t * const g_mwszzCpuInfoLibs        = wszCoreProcessThreads L"\0"  wszKernel32 L"\0";
const wchar_t * const g_mwszzSysInfoLibs        = wszCoreSysInfo L"\0"  wszKernel32 L"\0";
const wchar_t * const g_mwszzSyncLibs           = wszCoreSynch L"\0"  wszKernel32 L"\0";
const wchar_t * const g_mwszzHeapLibs           = wszCoreHeap L"\0"  wszKernel32 L"\0";
const wchar_t * const g_mwszzFileLibs           = wszCoreFile12 L"\0" wszCoreFile L"\0"  wszKernel32 L"\0"  wszKernelBase L"\0";
const wchar_t * const g_mwszzThreadpoolLibs     = wszCoreThreadpool L"\0"  wszKernel32 L"\0";
#ifdef ESENT
const wchar_t * const g_mwszzLocalizationLibs   = wszCoreLoc L"\0"  wszKernel32 L"\0";
#else
const wchar_t * const g_mwszzLocalizationLibs   = wszCoreLoc L"\0" wszCoreDebug12 L"\0"  wszKernel32 L"\0";
#endif
const wchar_t * const g_mwszzCoreDebugLibs      = wszCoreDebug L"\0"  wszKernel32 L"\0";

const wchar_t * const g_mwszzRegistryLibs       = wszCoreReg L"\0"  wszAdvapi32 L"\0";
const wchar_t * const g_mwszzSecSddlLibs        = wszSecSddl L"\0"  wszAdvapi32 L"\0";
const wchar_t * const g_mwszzProcessTokenLibs   = wszCoreProcessThreads L"\0"  wszAdvapi32 L"\0";
const wchar_t * const g_mwszzAdjPrivLibs        = wszSecBase L"\0"  wszAdvapi32 L"\0";
const wchar_t * const g_mwszzLookupPrivLibs     = wszLsaLookup L"\0"  wszAdvapi32 L"\0";

const wchar_t * const g_mwszzUserInterfaceLibs  = wszMinUser L"\0"  wszUser32 L"\0";
const wchar_t * const g_mwszzWorkingSetLibs     = wszWorkingSet L"\0"  wszPsapi L"\0";
const wchar_t * const g_mwszzProcessMemLibs     =  wszWorkingSet L"\0"  wszPsapi L"\0" wszKernel32 L"\0";

const wchar_t * const g_mwszzErrorHandlingLegacyLibs = wszCoreKernel32Legacy L"\0"  wszKernel32 L"\0";
const wchar_t * const g_mwszzWow64Libs          = wszCoreWow64 L"\0"  wszKernel32 L"\0";

const wchar_t * const g_mwszzAppModelRuntimeLibs = szAppModelRuntime L"\0";
const wchar_t * const g_mwszzAppModelStateLibs  = szAppModelState L"\0";

const wchar_t * const g_mwszzEventingProviderLibs = wszEventingProvider L"\0"  wszAdvapi32 L"\0";
const wchar_t * const g_mwszzEventLogLegacyLibs = wszEventLogLegacy L"\0"  wszAdvapi32 L"\0";

const wchar_t * const g_mwszzKernel32CoreSystemBroken       = wszKernel32 L"\0"  wszKernelBase L"\0"  wszKernel32Legacy L"\0";
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




const INT rankCritTaskList                  = 0;
const INT rankAESProv                       = 1;
const INT rankIoStats                       = 1;
const INT rankIOREQ                         = 2;
const INT rankTimerTaskList                 = 3;
const INT rankTimerTaskEntry                = 3;
const INT rankIOREQPoolCrit     = 3;
const INT rankOSDiskIOQueueCrit             = 4;
const INT rankOSDiskSXWL        = 6;
const INT rankFTLFlushBuffs                 = 6;
const INT rankFTLFlush                      = 7;
const INT rankFTLBuffer                     = 8;
const INT rankOSVolumeSXWL      = 8;
const INT rankRwlPostTasks                  = 9;



#include "_dev.hxx"
#include "_os.hxx"
#include "_ostls.hxx"
#include "_osdisk.hxx"
#include "_osfs.hxx"
#include "_osfile.hxx"
#include "_oserror.hxx"

#include "_reftrace.hxx"

#include "blockcache\_blockcache.hxx"



const BOOL FUtilIProcessIsWow64();




#ifndef OS_LAYER_VIOLATIONS


VOID UtilReportEvent(
    const EEventType    type,
    const CategoryId    catid,
    const MessageId     msgid,
    const DWORD         cString,
    const WCHAR *       rgpszString[],
    const DWORD         cbRawData = 0,
    void *              pvRawData = NULL,
    const INST *        pinst = NULL,
    const LONG          lEventLoggingLevel = 1 );

#ifdef ESENT
#include "jetmsg.h"
#else
#include "jetmsgex.h"
#endif




#define LoadLibraryA        LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define LoadLibraryW        LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define LoadLibraryExA      LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define LoadLibraryExW      LoadLibrary_not_allowed_use_NTOSFuncXxx_FunctionLoader

#define GetModuleHandleA    GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define GetModuleHandleW    GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define GetModuleHandleExA  GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader
#define GetModuleHandleExW  GetModuleHandle_not_allowed_use_NTOSFuncXxx_FunctionLoader

#define GetTickCount        GetTickCount_is_protected_function_use_TickOSTimeCurrent

#endif

