// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#undef LoadLibraryExW
#undef GetModuleHandleW

extern volatile BOOL g_fDllUp;



BOOL FUtilLoadLibrary( const WCHAR* wszLibrary, LIBRARY* plibrary, const BOOL fPermitDialog )
{
    Expected( NULL == WszDllSubStr( wszLibrary, L"ntdll" ) );
    Expected( NULL == WszDllSubStr( wszLibrary, L"kernel32" ) );
    Expected( NULL == WszDllSubStr( wszLibrary, L"user32" ) );
    Expected( NULL == WszDllSubStr( wszLibrary, L"mincore" ) );
    Expected( NULL == WszDllSubStr( wszLibrary, L"API-MS-" ) );

    while ( NULL == ( *plibrary = (LIBRARY)LoadLibraryExW( (LPWSTR)wszLibrary, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS ) )
        && fPermitDialog )
    {
        WCHAR szMessage[256];
        OSStrCbFormatW(
            szMessage, sizeof(szMessage),
            L"Unable to find the callback library %ws (or one of its dependencies).\r\n"
                L"Copy in the file and hit OK to retry, or hit Cancel to abort.\r\n",
            wszLibrary );
        Assert( wcslen( szMessage ) < _countof( szMessage ) );

        const INT id = UtilMessageBoxW(
                            szMessage,
                            L"Callback DLL not found",
                            MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP |
                            MB_OKCANCEL );

        if ( IDOK != id )
        {
            break;
        }
    }
    
    return ( NULL != *plibrary );
}


PFN PfnUtilGetProcAddress( LIBRARY library, const char* szFunction )
{
    return (PFN) GetProcAddress( (HMODULE) library, szFunction );
}


void UtilFreeLibrary( LIBRARY library )
{
    FreeLibrary( (HMODULE) library );
}

#define STATUS_NOT_IMPLEMENTED           ((INT)0xC0000002L)

INT NtstatusThunkNotSupported()
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD ErrorThunkNotSupported()
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

#ifdef DEBUG


#pragma push_macro( "GetModuleHandleExW" )
#ifdef GetModuleHandleExW
#undef GetModuleHandleExW
#endif

VOID OSLibraryValidateLoaderPolicy( const WCHAR * const mwszzDlls, OSLoadFlags oslf )
{

    Assert( oslf & oslfExpectedOnWin5x || oslf & oslfExpectedOnWin6 || oslf & oslfExpectedOnWin7 || oslf & oslfExpectedOnWin8 || oslf & oslfExpectedOnWin10 );


    Assert( oslf & oslfExpectedOnWin5x || oslf & oslfExpectedOnWin6 || !( oslf & oslfRequired ) );

    HMODULE hmoduleThis = NULL;
    if ( !GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PCWSTR) OSLibraryValidateLoaderPolicy, &hmoduleThis ) )
    {
        DWORD gle = GetLastError();
        AssertSz( !g_fDllUp && gle == ERROR_MOD_NOT_FOUND, "Why did GetModuleHandleExW() fail? gle = %d.", GetLastError() );
    }
    else
    {
        WCHAR wszImgName[_MAX_FNAME] = L"\0";
        WCHAR wszT[_MAX_PATH];
        if ( GetModuleFileNameW( hmoduleThis, wszT, _countof( wszT ) ) )
        {
            _wsplitpath_s( wszT, NULL, 0, NULL, 0, wszImgName, _countof( wszImgName ), NULL, 0 );
        }
        const BOOL fIsEseDll = ( ( 0 == _wcsicmp( wszImgName, L"ese" ) ) || ( 0 == _wcsicmp( wszImgName, L"esent" ) ) );
        
        
        if ( fIsEseDll )
        {
            Assert(
                    mwszzDlls == g_mwszzKernel32CoreSystemBroken ||
                    mwszzDlls == g_mwszzAdvapi32CoreSystemBroken ||
                    mwszzDlls == g_mwszzNtdllLibs ||
                    mwszzDlls == g_mwszzRtlSupportLibs ||
                    mwszzDlls == g_mwszzCpuInfoLibs ||
                    mwszzDlls == g_mwszzSysInfoLibs ||
                    mwszzDlls == g_mwszzHeapLibs ||
                    mwszzDlls == g_mwszzFileLibs ||
                    mwszzDlls == g_mwszzThreadpoolLibs ||
                    mwszzDlls == g_mwszzRegistryLibs ||
                    mwszzDlls == g_mwszzSecSddlLibs ||
                    mwszzDlls == g_mwszzLocalizationLibs ||
                    mwszzDlls == g_mwszzCoreDebugLibs ||
                    mwszzDlls == g_mwszzErrorHandlingLegacyLibs ||
                    mwszzDlls == g_mwszzUserInterfaceLibs ||
                    mwszzDlls == g_mwszzProcessTokenLibs ||
                    mwszzDlls == g_mwszzAdjPrivLibs ||
                    mwszzDlls == g_mwszzLookupPrivLibs ||
                    mwszzDlls == g_mwszzAppModelRuntimeLibs ||
                    mwszzDlls == g_mwszzAppModelStateLibs ||
                    mwszzDlls == g_mwszzWorkingSetLibs ||
                    mwszzDlls == g_mwszzProcessMemLibs ||
                    mwszzDlls == g_mwszzWow64Libs ||
                    mwszzDlls == g_mwszzEventLogLegacyLibs ||
                    mwszzDlls == g_mwszzEventingProviderLibs
                    );
        
        
            Assert( mwszzDlls != g_mwszzKernel32CoreSystemBroken || oslf & oslfStrictFree );
            Assert( mwszzDlls != g_mwszzAdvapi32CoreSystemBroken || oslf & oslfStrictFree );
        }
    }

    if ( hmoduleThis != NULL )
    {
        FreeLibrary( hmoduleThis );
        hmoduleThis = NULL;
    }
}
#pragma pop_macro( "GetModuleHandleExW" )


LONG g_cKernel32Loads = 0;
LONG g_cAdvapi32Loads = 0;

VOID OSLibraryTrackingLoad( const WCHAR * const mwszzDlls )
{
    LONG c = 1;
    if ( mwszzDlls == g_mwszzKernel32CoreSystemBroken )
    {
        c = AtomicIncrement( &g_cKernel32Loads );
    }
    else if ( mwszzDlls == g_mwszzAdvapi32CoreSystemBroken )
    {
        c = AtomicIncrement( &g_cAdvapi32Loads );
    }
    Assert( c > 0 );
    Assert( c < 300 );
}

VOID OSLibraryTrackingFree( const WCHAR * const mwszzDlls )
{
    LONG c = 1;
    if ( mwszzDlls == g_mwszzKernel32CoreSystemBroken )
    {
        c = AtomicDecrement( &g_cKernel32Loads );
    }
    else if ( mwszzDlls == g_mwszzAdvapi32CoreSystemBroken )
    {
        c = AtomicDecrement( &g_cAdvapi32Loads );
    }
    Assert( c >= 0 );
}

#endif

ERR ErrMultiLoadPfn(
    const WCHAR * const mwszzDlls,
    const BOOL          fNonSystemDll,
    const CHAR * const  szFunction,
    SHORT * const       pichDll,
    void ** const       ppfn )
{
    ERR err_ = JET_errSuccess;
    LONG lLine = __LINE__;

    Expected( *ppfn == NULL );

#if defined( ESENT ) && defined( OS_LAYER_VIOLATIONS )
    Assert( !fNonSystemDll );
#else
    Expected( !fNonSystemDll || 0 == _wcsicmp( mwszzDlls, L"ese.dll" ) || 0 == _wcsicmp( mwszzDlls, L"esent.dll" ) );
#endif

    SHORT ichDll = 0;
    BOOL fAtLeastOneFailedLoad = fFalse;
    
    DWORD dwOldThreadErrMode;
    BOOL fThreadErrModeSet = SetThreadErrorMode( SEM_FAILCRITICALERRORS, &dwOldThreadErrMode );

    while( mwszzDlls[ichDll] != L'\0' )
    {
        HMODULE hmodule = LoadLibraryExW( &(mwszzDlls[ichDll]), NULL, fNonSystemDll ? 0 : LOAD_LIBRARY_SEARCH_SYSTEM32 );
        if (  NULL != hmodule && INVALID_HANDLE_VALUE != hmodule )
        {
            OnDebug( OSLibraryTrackingLoad( mwszzDlls ) );

            void * pfn = GetProcAddress( hmodule, szFunction );
            if ( pfn )
            {
                PreinitTrace( L"\tLOADED: proc: %hs (0x%p) from DLL: %ws (0x%p)\n", szFunction, pfn, &(mwszzDlls[ichDll]), hmodule );

                *pichDll = ichDll;
                *ppfn = (void*)pfn;
                err_ = JET_errSuccess;
                break;
            }
            else
            {
                DWORD errorGetProc = GetLastError();
                if ( err_ == JET_errSuccess )
                {
                    switch( errorGetProc )
                    {
                        case ERROR_PROC_NOT_FOUND:
                            err_ = JET_errUnloadableOSFunctionality; lLine = __LINE__;
                            break;
                        default:
                            AssertSzRTL( fFalse, "Unknown error(%d) loading module\n", errorGetProc );
                            err_ = JET_errInternalError; lLine = __LINE__;
                    }
                }
                PreinitTrace( L"\tLOADERROR[%d]: Could not get proc: %hs (from DLL: %ws), gle=%d\n", __LINE__, szFunction, &(mwszzDlls[ichDll]), GetLastError() );
                fAtLeastOneFailedLoad = fTrue;
            }

            FreeLibrary( hmodule );
            OnDebug( OSLibraryTrackingFree( mwszzDlls ) );
            hmodule = NULL;
        }
        else
        {
            DWORD errorLoadDll = GetLastError();
            if ( err_ == JET_errSuccess )
            {
                switch( errorLoadDll )
                {
                    case ERROR_MOD_NOT_FOUND:
                        err_ = JET_errUnloadableOSFunctionality; lLine = __LINE__;
                        break;
                    default:
                        AssertSz( fFalse, "Unknown error(%d) loading module\n", errorLoadDll );
                        err_ = JET_errInternalError; lLine = __LINE__;
                }
            }
            PreinitTrace( L"\tLOADERROR[%d]: Failed to load DLL: %ws (for %hs), gle=%d\n", __LINE__, &(mwszzDlls[ichDll]), szFunction, GetLastError() );
            fAtLeastOneFailedLoad = fTrue;
        }

        ichDll = ichDll + (SHORT)wcslen( &(mwszzDlls[ichDll]) ) + 1;
    }

    if ( NULL == *ppfn )
    {
        PreinitTrace( L"\tLOADFAIL[%d]: Could not get proc: %hs from any DLL, giving up! returning %d from line %d\n", __LINE__, szFunction, err_, lLine );
        Assert( err_ < JET_errSuccess );
        if ( g_fDllUp )
        {
            ErrERRCheck_( err_, __FILE__, lLine );
        }
        else
        {
            extern ERR g_errTrap;
            Assert( g_errTrap != err_ );
        }
    }
    else
    {
        Assert( err_ == JET_errSuccess );

    }

    if ( fThreadErrModeSet )
    {
        SetThreadErrorMode( dwOldThreadErrMode, NULL );
    }

    return err_;
}

VOID FreeLoadedModule(
    const WCHAR * const wszDll )
{
    Assert( wszDll && wcslen( wszDll ) );
    HMODULE hmodule = GetModuleHandleW( wszDll );

    if ( hmodule )
    {
        PreinitTrace( L"FreeLibrary( %p(%ws) )\n", hmodule, wszDll );
        FreeLibrary( hmodule );
    }
}



void OSLibraryPostterm()
{
}


BOOL FOSLibraryPreinit()
{

    return fTrue;
}



void OSLibraryTerm()
{
}


ERR ErrOSLibraryInit()
{

    return JET_errSuccess;
}

