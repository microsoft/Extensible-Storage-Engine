// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


#define _DECL_DLLMAIN
#include <process.h>


//  OS Layer Init / Term functions

extern BOOL FOSPreinit();
extern void OSIProcessAbort();
extern void OSPostterm();

//  globals

BOOL            g_fProcessExit = fFalse;
volatile BOOL   g_fDllUp = fFalse;

//  DLL Entry Point

extern BOOL FINSTSomeInitialized();

volatile DWORD  g_tidDLLEntryPoint;

extern "C" {

    void __cdecl __security_init_cookie(void);


    DECLSPEC_NOINLINE BOOL WINAPI DLLEntryPoint_( void* hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
    {
        g_tidDLLEntryPoint = GetCurrentThreadId();

        BOOL fResult = fTrue;

        switch( fdwReason )
        {
            case DLL_THREAD_ATTACH:
                break;

            case DLL_THREAD_DETACH:
                break;

            case DLL_PROCESS_ATTACH:

                Assert( !g_fDllUp );

                PreinitTrace( L"Begin DLLEntryPoint( DLL_PROCESS_ATTACH )\n" );

                //  we do not want to see thread notifications

                (void)DisableThreadLibraryCalls( (HMODULE)hinstDLL );

                //  init OS Layer

                #ifdef OS_LAYER_VIOLATIONS
                OSPrepreinitSetUserTLSSize( ESE_USER_TLS_SIZE );
                #endif

                fResult = fResult && FOSPreinit();

                //  init CRT

                PreinitTrace( L"Begin _CRT_INIT()\n" );
                fResult = fResult && _CRT_INIT( hinstDLL, fdwReason, lpvReserved );
                PreinitTrace( L"Finish _CRT_INIT()\n" );

                if ( fResult )
                {
                    g_fDllUp = fTrue;
                }

                PreinitTrace( L"Finish DLLEntryPoint( DLL_PROCESS_ATTACH )\n" );
                break;

            case DLL_PROCESS_DETACH:

                PreinitTrace( L"Begin DLL'Exit'Point( DLL_PROCESS_DETACH )\n" );

                //  indicate the DLL is down

                g_fDllUp = fFalse;

                //  signals Postterm is result of process exit vs. dynamic DLL unload (i.e. FreeLibrary()).

                g_fProcessExit = (NULL != lpvReserved);
                
                //  if JET is still initialized, we are experiencing an abnormal
                //  termination

                if ( FINSTSomeInitialized() )
                {
                    //  All ESE/JET instances not terminated by caller before unloading dll.
                    EnforceSz( g_fProcessExit, "InitdEseInstancesOnDllUnload" );
                    OSIProcessAbort();
                    //  we bail out here, because we cannot guarantee that the rest of ESE will
                    //  behave correctly (with all it's various background tasks going off) if we tear
                    //  down the OS layer out from under it.
                    break;
                }

                //  terminate CRT

                PreinitTrace( L"Begin _CRT_'TERM'()\n" );
                (void)_CRT_INIT( hinstDLL, fdwReason, lpvReserved );
                PreinitTrace( L"Finish _CRT_'TERM'()\n" );

                //  terminate OS Layer

                OSPostterm();

                PreinitTrace( L"Finish DLL'Exit'Point( DLL_PROCESS_DETACH )\n" );
                break;
        }

        g_tidDLLEntryPoint = 0;

        return fResult;
    }

    BOOL WINAPI DLLEntryPoint( void* hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
    {
        if ( fdwReason == DLL_PROCESS_ATTACH )
        {
            //  init /GS cookie
            __security_init_cookie();
        }

        return DLLEntryPoint_( hinstDLL, fdwReason, lpvReserved );
    }

} // extern "C"

BOOL FOSDllUp()
{
    return g_fDllUp;
}

VOID OSEventRegister();

HRESULT DllRegisterServer()
{
    OSEventRegister();
    return S_OK;
}
