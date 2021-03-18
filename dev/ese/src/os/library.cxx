// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  this is the wrapper layer for LoadLibraryExW() proper.  Only edbg.cxx and
//  hapublish.cxx have usage of LoadLibrary outside this function.
#undef LoadLibraryExW
#undef GetModuleHandleW

extern volatile BOOL g_fDllUp;      //  for validation in a couple places

//  Dynamically Loaded Libraries

//  loads the library from the specified file, returning fTrue and the library
//  handle on success.  this should only be used for user specified libraries,
//  not system libraries (which should use the NTOSFunc*/FunctionLoaders)

BOOL FUtilLoadLibrary( const WCHAR* wszLibrary, LIBRARY* plibrary, const BOOL fPermitDialog )
{
    //  This function is for loading only consumer / user specified libraries.  If you are
    //  loading a subset of OS functionality, please use NTOSFunc*() macros / FunctionLoader<>.
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
        Assert( LOSStrLengthW( szMessage ) < _countof( szMessage ) );

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

//  retrieves the function pointer from the specified library or NULL if that
//  function is not found in the library

PFN PfnUtilGetProcAddress( LIBRARY library, const char* szFunction )
{
    return (PFN) GetProcAddress( (HMODULE) library, szFunction );
}

//  unloads the specified library

void UtilFreeLibrary( LIBRARY library )
{
    FreeLibrary( (HMODULE) library );
}

// easier than working out how to include the header
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

//  lines in the sand

#pragma push_macro( "GetModuleHandleExW" )
#ifdef GetModuleHandleExW
#undef GetModuleHandleExW
#endif

VOID OSLibraryValidateLoaderPolicy( const WCHAR * const mwszzDlls, OSLoadFlags oslf )
{
    //  must specify an OS revision you expect this on

    Assert( oslf & oslfExpectedOnWin5x || oslf & oslfExpectedOnWin6 || oslf & oslfExpectedOnWin7 || oslf & oslfExpectedOnWin8 || oslf & oslfExpectedOnWin10 );

    //  our minimum OS requirement is Vista / Win6, so we should check for that

    Assert( oslf & oslfExpectedOnWin5x || oslf & oslfExpectedOnWin6 || !( oslf & oslfRequired ) );

    // don't validate this in the test harness
    HMODULE hmoduleThis = NULL;
    if ( !GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PCWSTR) OSLibraryValidateLoaderPolicy, &hmoduleThis ) )
    {
        DWORD gle = GetLastError();
        //  What we can't load OUR OWN MODULE?  Well, apparently there is one such case, if you call this
        //  during DLL unload, you'll get ERROR_MOD_NOT_FOUND ... luckily we'll have set the g_fDllUp to
        //  false by this time, so we can detect said condition.
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
            //  these are the approved binaries lists
            Assert(
                    //  the "broken" libraries
                    mwszzDlls == g_mwszzKernel32CoreSystemBroken ||
                    mwszzDlls == g_mwszzAdvapi32CoreSystemBroken ||
                    //  the approved binaries list
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
        
            //  there are a couple reasons to add a library to the strict free required list
            //    1. historically the DLL has been freed, we want to keep the same contract
            //       with the new functional loader
            //    2. we are trying to move off the DLL so that we can lower our code working
            //       working set and COW pages for other DLLs caused by ESE
        
            // NOTE: If this Assert fires, but the strings look just fine, then you may have
            // compiled with StringPooling (-GF) turned off.        
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

//  tracking how many loads and unloads we've had of libraries that we'd like to follow
//  a strict free model for

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
    Assert( c < 300 );  // we're leaking
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

#endif  //  DEBUG

ERR ErrMultiLoadPfn(
    const WCHAR * const mwszzDlls,
    const BOOL          fNonSystemDll,
    const CHAR * const  szFunction,
    SHORT * const       pichDll,
    void ** const       ppfn )
{
    ERR err_ = JET_errSuccess;
    LONG lLine = __LINE__;

    Expected( *ppfn == NULL );  // if we start using this function concurrently this may one day fail

#if defined( ESENT ) && defined( OS_LAYER_VIOLATIONS )
    //  We only load a non-system DLL in test code / oslite.lib ... so in oswinnt.lib, we
    //  can assert we're not trying to load a non-system DLL.
    // Exchange can also optionally load non-system dlls
    Assert( !fNonSystemDll );
#else
    //  Note: The test code is passing L"ese.dll\0\0" or L"esent.dll\0\0"
    Expected( !fNonSystemDll || 0 == _wcsicmp( mwszzDlls, L"ese.dll" ) || 0 == _wcsicmp( mwszzDlls, L"esent.dll" ) );
#endif

    SHORT ichDll = 0;
    BOOL fAtLeastOneFailedLoad = fFalse;
    
    // Disable missing dll popup, loader will return errors without showing the dialog popup.
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
                if ( err_ == JET_errSuccess )   //  protects previous errors
                {
                    switch( errorGetProc )
                    {
                        case ERROR_PROC_NOT_FOUND:
                            err_ = JET_errUnloadableOSFunctionality; lLine = __LINE__; // fake ErrERRCheck()
                            break;
                        default:
                            AssertSzRTL( fFalse, "Unknown error(%d) loading module\n", errorGetProc );
                            err_ = JET_errInternalError; lLine = __LINE__; // fake ErrERRCheck()
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
            if ( err_ == JET_errSuccess )   //  protects previous errors
            {
                switch( errorLoadDll )
                {
                    case ERROR_MOD_NOT_FOUND:
                        err_ = JET_errUnloadableOSFunctionality; lLine = __LINE__; // fake ErrERRCheck()
                        break;
                    default:
                        AssertSz( fFalse, "Unknown error(%d) loading module\n", errorLoadDll );
                        err_ = JET_errInternalError; lLine = __LINE__; // fake ErrERRCheck()
                }
            }
            PreinitTrace( L"\tLOADERROR[%d]: Failed to load DLL: %ws (for %hs), gle=%d\n", __LINE__, &(mwszzDlls[ichDll]), szFunction, GetLastError() );
            fAtLeastOneFailedLoad = fTrue;
        }

        //  increment to the next DLL in the multi-string
        ichDll = ichDll + (SHORT)LOSStrLengthW( &(mwszzDlls[ichDll]) ) + 1;
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
            // cheap form of fake err trap in lieu of FOSPreinit() having run
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
    Assert( wszDll && LOSStrLengthW( wszDll ) );
    HMODULE hmodule = GetModuleHandleW( wszDll );

    if ( hmodule )  // just in case
    {
        PreinitTrace( L"FreeLibrary( %p(%ws) )\n", hmodule, wszDll );
        FreeLibrary( hmodule );
    }
}


//  post-terminate library subsystem

void OSLibraryPostterm()
{
}

//  pre-init library subsystem

BOOL FOSLibraryPreinit()
{
    //  nop

    return fTrue;
}


//  terminate library subsystem

void OSLibraryTerm()
{
    //  nop
}

//  init library subsystem

ERR ErrOSLibraryInit()
{
    //  nop

    return JET_errSuccess;
}

