// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#ifndef WIN32_LEAN_AND_MEAN 
#define WIN32_LEAN_AND_MEAN 
#endif
#include <windows.h>
#include <string>
#include <list>

#pragma pack(push)
#pragma pack(8)
#pragma warning(disable:4200)

#ifndef ESEINTERNAL
#define ESEINTERNAL protected
#endif

namespace FailureInjector
{
    // The following ifdef block is the standard way of creating macros which make exporting 
    // from a DLL simpler. All files within this DLL are compiled with the FAILUREINJECTOR_EXPORTS
    // symbol defined on the command line. this symbol should not be defined on any project
    // that uses this DLL. This way any other project whose source files include this file see 
    // FAILUREINJECTOR_API functions as being imported from a DLL, whereas this DLL sees symbols
    // defined with this macro as being exported.
    #ifdef ESETEST_FAILUREINJECTOR_EXPORTS
    #define FAILUREINJECTOR_API __declspec(dllexport) __stdcall
    #else
    #define FAILUREINJECTOR_API __declspec(dllimport) __stdcall
    #endif

    const DWORD FI_ERR_BASE = 0x20000001;
    enum FI_ErrorCodes
    {
        FI_errSuccess = 0,
        FI_errOpenProcess = FI_ERR_BASE+0,
        FI_errWriteProcessMem = FI_ERR_BASE+1,
        FI_errCreateRemoteThread = FI_ERR_BASE+2
    };

    enum IOFuncs
    {
        IOF_NONE                                = 0x00000000,
        IOF_READ                                = 0x00000001,
        IOF_READ_SCATTER                        = 0x00000002,
        IOF_READ_ALL                            = 0x000000FF,
        IOF_WRITE                               = 0x00000100,
        IOF_WRITE_GATHER                        = 0x00000200,
        IOF_WRITE_ALL                           = 0x0000FF00,
        IOF_ALL                                 = 0xFFFFFFFF,
    };

    enum IOConditions
    {
        IOC_NONE        = 0x0000,
        IOC_FUNC        = 0x0001,
        IOC_THREAD_ID   = 0x0002,
        IOC_FILENAME    = 0x0004,
        IOC_OFFSET_LE   = 0x0008,
        IOC_OFFSET_GE   = 0x0010,
        IOC_LENGTH_LE   = 0x0020,
        IOC_LENGTH_GE   = 0x0040,
        IOC_PROBABILITY = 0x0080,
        IOC_HITCOUNT    = 0x0100,
    };

    #define IOA_MASK    0x0000FFFF
    enum IOErrorActions
    {
        IOA_NONE                = 0,
        IOA_RET_ERROR           = 1,
        IOA_CORRUPT_DATA        = 2,
        IOA_PASSTHROUGH         = 3,
        IOA_LOST_FLUSH          = 4,    // only relevant to write operations, will default to IOA_PASSTHROUGH on read ops
        IOA_SIM_SECTOR_REMAP    = 5,    // the filter will fail all read ops by returing the user-specified error until a write succeeds. should hook Read+Write for it to work properly

        // These flags can be combined with each other or the ones above
        IOA_LATENCY         = 0x00010000,
        IOA_CALLBACK        = 0x00020000,
        IOA_CALLBACK_EX     = 0x00060000,   // CALLBACK bit should be set as well
    };

    typedef int IOFAILURE_FILTER_ID;
    #define     IOFAILURE_FILTER_ID_NIL -1
    
    struct IOFailureFilter
    {
        friend class ESEIOFailureInjector;

    ESEINTERNAL:
        IOFAILURE_FILTER_ID id;

    public:
        DWORD               filterMask;
        DWORD               funcFilter;
        DWORD               threadID;
        std::string         filenameFilter;
        float               probability;    // any value smaller than 1 / RAND_MAX will be treated as 0
        INT64               offset_le;
        INT64               offset_ge;
        INT64               length_le;
        INT64               length_ge;
        DWORD               errAction;
        DWORD               latencyMSec;
        FARPROC             callback;
        LPARAM              lParam;
        volatile long       hitCount;   // this variable will be updated atomically on each filter hit
        union
        {
            DWORD       error;
            DWORD       corruptPercent;
        };
    };

    struct IOCallbackArgs
    {
        HANDLE          hFile;
        DWORD           numBytes;
        LPOVERLAPPED    lpOverlapped;
        void*           pvExtendedArgs;
    };

    struct StandardArgs
    {
        LPVOID  lpBuffer;
        LPDWORD lpNumberOfBytes;
    };

    struct ScatterGatherArgs
    {
        FILE_SEGMENT_ELEMENT    aSegmentArray[];
    };

    bool FAILUREINJECTOR_API InitIOFailureInjector(
        __in HMODULE hModuleToHook,
        __in bool bEnableHooksOnRuntimeLinking,
        __inout LPDWORD pFuncsToHook = NULL);

    bool FAILUREINJECTOR_API DisposeIOFailureInjector();
    
    IOFAILURE_FILTER_ID FAILUREINJECTOR_API AddIOFailureFilter(
        __in const IOFailureFilter &filter);

    IOFAILURE_FILTER_ID FAILUREINJECTOR_API InsertIOFailureFilter(
        __in IOFAILURE_FILTER_ID idAfter,
        __in const IOFailureFilter &filter);
    
    void FAILUREINJECTOR_API ModifyIOFailureFilter(
        __in IOFAILURE_FILTER_ID id,
        __in const IOFailureFilter &filter);

    bool FAILUREINJECTOR_API DelIOFailureFilter(
        __in IOFAILURE_FILTER_ID filterID);

    typedef BOOL (CALLBACK *IO_CALLBACK_PROC)(
        __in const IOFailureFilter &filter,
        __in IOFuncs callbackAPI,
        __in const IOCallbackArgs &callbackArgs,
        __inout DWORD *pErrAction);

    bool FAILUREINJECTOR_API EnableHooksOnRuntimeLinking(
        __in HMODULE hModule);
    
    LPVOID FAILUREINJECTOR_API InstallAPIHook(
        __in HMODULE hTargetModule,
        __in_z LPCSTR lpAPIModule,
        __in_z LPCSTR lpProcName,
        __in LPVOID pHook);

    LPVOID FAILUREINJECTOR_API UninstallAPIHook(
        __in HMODULE hTargetModule,
        __in_z LPCSTR lpAPIModule,
        __in_z LPCSTR lpProcName);

    //////////////////////////////////////////////////////////////////////////
    // Code Injection
    typedef int (CALLBACK *INIT_PROC)(LPVOID pData);

    HANDLE FAILUREINJECTOR_API InjectCode(
        __in DWORD dwProcessID,
        __in_z LPCSTR szDllToInject,
        __in_z LPCSTR szInitMethod, 
        __in_bcount(cbData) LPVOID pInitData,
        __in DWORD cbData,
        __in bool bWaitForRemoteThread);
}

#pragma pack(pop)
