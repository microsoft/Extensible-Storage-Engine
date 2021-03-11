// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

#include <tchar.h>
#include "os.hxx"



#ifdef DEBUG

CUnitTest( LibraryDllWideSubStrCheck, 0, "This test just checks that the case insensitive wide char substr utility for some Assert()s we want to have." );
ERR LibraryDllWideSubStrCheck::ErrTest()
{
    JET_ERR err = JET_errSuccess;

    WCHAR * wszTestPath1 = L"C:\\blah\\ntdll.dll";
    WCHAR * wszTestPath1PartCaps = L"C:\\blah\\ntDll.dll";
    WCHAR * wszTestPath1AllCaps = L"C:\\blah\\NTDLL.DLL";

    OSTestCheck( &(wszTestPath1[8]) == WszDllSubStr( wszTestPath1, L"ntdll" ) );
    OSTestCheck( &(wszTestPath1PartCaps[8]) == WszDllSubStr( wszTestPath1PartCaps, L"ntdll" ) );
    OSTestCheck( &(wszTestPath1AllCaps[8]) == WszDllSubStr( wszTestPath1AllCaps, L"ntdll" ) );

    OSTestCheck( &(wszTestPath1[8]) == WszDllSubStr( wszTestPath1, L"ntDll" ) );
    OSTestCheck( &(wszTestPath1PartCaps[8]) == WszDllSubStr( wszTestPath1PartCaps, L"ntDll" ) );
    OSTestCheck( &(wszTestPath1AllCaps[8]) == WszDllSubStr( wszTestPath1AllCaps, L"ntDll" ) );

    OSTestCheck( &(wszTestPath1[8]) == WszDllSubStr( wszTestPath1, L"ntdll" ) );
    OSTestCheck( &(wszTestPath1PartCaps[8]) == WszDllSubStr( wszTestPath1PartCaps, L"ntdLl" ) );
    OSTestCheck( &(wszTestPath1AllCaps[8]) == WszDllSubStr( wszTestPath1AllCaps, L"ntdLl" ) );

    OSTestCheck( &(wszTestPath1[8]) == WszDllSubStr( wszTestPath1, L"NTDLL" ) );
    OSTestCheck( &(wszTestPath1PartCaps[8]) == WszDllSubStr( wszTestPath1PartCaps, L"NTDLL" ) );
    OSTestCheck( &(wszTestPath1AllCaps[8]) == WszDllSubStr( wszTestPath1AllCaps, L"NTDLL" ) );

    OSTestCheck( NULL == WszDllSubStr( wszTestPath1, L"ntdlll" ) );
    OSTestCheck( NULL == WszDllSubStr( wszTestPath1PartCaps, L"ntdlll" ) );
    OSTestCheck( NULL == WszDllSubStr( wszTestPath1AllCaps, L"ntdlll" ) );

    OSTestCheck( NULL == WszDllSubStr( wszTestPath1, L"ntdLll" ) );
    OSTestCheck( NULL == WszDllSubStr( wszTestPath1PartCaps, L"ntdLll" ) );
    OSTestCheck( NULL == WszDllSubStr( wszTestPath1AllCaps, L"ntdLll" ) );

    OSTestCheck( NULL == WszDllSubStr( wszTestPath1, L"NTDLLL" ) );
    OSTestCheck( NULL == WszDllSubStr( wszTestPath1PartCaps, L"NTDLLL" ) );
    OSTestCheck( NULL == WszDllSubStr( wszTestPath1AllCaps, L"NTDLLL" ) );

HandleError:

    return err;
}

#endif



ERR ErrLoadTreatableAsPointer()
{
    ERR err = JET_errSuccess;

    WCHAR * szDlls = L"NonExistantDll.dll\0kernel32.dll\0api-ms-win-core-synch-l1-1-1.dll\0";
    NTOSFuncPtr( pfnCreateSemaphoreTest, szDlls, CreateSemaphoreExW, oslfExpectedOnWin8  );

    OSTestCheck( NULL == pfnCreateSemaphoreTest.SzDllLoaded() );

    if ( pfnCreateSemaphoreTest )
    {
    }
    else
    {
        OSTestCheck( fFalse );
    }

    OSTestCheck( szDlls != pfnCreateSemaphoreTest.SzDllLoaded() );
    OSTestCheck( 0 != wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"NonExistantDll.dll" ) );
    OSTestCheck( 0 == wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"api-ms-win-core-synch-l1-1-1.dll" ) );
    OSTestCheck( pfnCreateSemaphoreTest.ErrIsPresent() == JET_errSuccess );

    OSTestCheck( NULL != pfnCreateSemaphoreTest );

    err = JET_errSuccess;

HandleError:

    return err;
}


ERR ErrLoadSkipsNonExistantDlls()
{
    ERR err = JET_errSuccess;

    WCHAR * szDlls = L"NonExistantDll.dll\0kernel32.dll\0api-ms-win-core-synch-l1-1-1.dll\0";
    NTOSFuncPtr( pfnCreateSemaphoreTest, szDlls, CreateSemaphoreExW, oslfExpectedOnWin8  );

    OSTestCheck( NULL == pfnCreateSemaphoreTest.SzDllLoaded() );

    HANDLE hValid = pfnCreateSemaphoreTest( 0, 1, 100, L"test", 0, SYNCHRONIZE | DELETE );
    OSTestCheck( hValid );

    OSTestCheck( szDlls != pfnCreateSemaphoreTest.SzDllLoaded() );
    OSTestCheck( 0 != wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"NonExistantDll.dll" ) );
    OSTestCheck( 0 == wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"api-ms-win-core-synch-l1-1-1.dll" ) );
    OSTestCheck( pfnCreateSemaphoreTest.ErrIsPresent() == JET_errSuccess );

    hValid = pfnCreateSemaphoreTest( 0, 1, 100, L"test2", 0, SYNCHRONIZE | DELETE );

    err = JET_errSuccess;

HandleError:

    return err;
}


ERR ErrLoadJetAttachDatabase()
{
    ERR err = JET_errSuccess;

    NTOSFuncStd( pfnJetAttachDatabase, L"esent.dll\0", JetAttachDatabase, oslfExpectedOnWin5x | oslfStrictFree );

    OSTestCall( pfnJetAttachDatabase.ErrIsPresent() );

    OSTestCheck( GetModuleHandle( "esent.dll" ) != NULL );


HandleError:

    return err;
}


ERR ErrLoadCheckFreeLibrary()
{
    ERR err = JET_errSuccess;

    OSTestCheck( GetModuleHandle( "esent.dll" ) == NULL );

    OSTestCall( ErrLoadJetAttachDatabase() );

    OSTestCheck( GetModuleHandle( "esent.dll" ) == NULL );

HandleError:

    return err;
}


ERR ErrLoadPresentFailsIfDllPresentButFuncIsNot()
{
    ERR err = JET_errSuccess;

    NTOSFuncCount( pfnMessageBoxA, L"kernel32.dll\0", MessageBoxA, oslfExpectedOnWin8  );

    Assert( pfnMessageBoxA.ErrIsPresent() < JET_errSuccess );
    if ( pfnMessageBoxA.ErrIsPresent() >= JET_errSuccess )
    {
        (void) (*pfnMessageBoxA)( NULL, "Junk", "AndStuff", MB_OK );
        OSTestCall( JET_errInvalidParameter );
    }

    OSTestCheck( NULL == pfnMessageBoxA.SzDllLoaded() );

HandleError:

    return err;
}


ERR ErrLoadPresentFailsIfDllIsNotPresent()
{
    ERR err = JET_errSuccess;

    NTOSFuncCount( pfnMessageBoxA, L"NonExistant.dll\0", MessageBoxA, oslfExpectedOnWin8  );

    Assert( pfnMessageBoxA.ErrIsPresent() < JET_errSuccess );
    if ( pfnMessageBoxA.ErrIsPresent() >= JET_errSuccess )
    {
        (void) (*pfnMessageBoxA)( NULL, "Junk", "AndStuff", MB_OK );
        OSTestCall( JET_errInvalidParameter );
    }

    OSTestCheck( NULL == pfnMessageBoxA.SzDllLoaded() );

HandleError:

    return err;
}


ERR ErrLoadTestSpecificDllIsLoaded()
{
    ERR err = JET_errSuccess;

    OSVERSIONINFOW osvi = { sizeof( osvi ), 0 };

    NTOSFuncStd( pfnGetVersionExWFirstControl, L"api-ms-win-core-sysinfo-l1-1-1.dll\0kernel32.dll\0", GetVersionExW, oslfExpectedOnWin5x );
    WCHAR * szDllListGetVersionExWFromLastDll = L"NonExistantDll.dll\0ntdll.dll\0kernel32.dll\0api-ms-win-core-sysinfo-l1-1-1.dll\0";
    NTOSFuncStd( pfnGetVersionExWFromLastDll, szDllListGetVersionExWFromLastDll, GetVersionExW, oslfExpectedOnWin5x );

    NTOSFuncStd( pfnGetVersionExWGood, L"NonExistantDll.dll\0kernel32.dll\0api-ms-win-core-sysinfo-l1-1-1.dll\0", GetVersionExW, oslfExpectedOnWin5x );
    NTOSFuncStd( pfnGetVersionExWBad, L"NonExistantDll.dll\0OtherNonDll.dll\0", GetVersionExW, oslfExpectedOnWin8 );


    OSTestCheck( osvi.dwMajorVersion == 0 );
    OSTestCheck( osvi.dwBuildNumber == 0 );


    OSTestCheck( fTrue == pfnGetVersionExWFirstControl( &osvi ) );
    OSTestCheck( 0 == wcscmp( pfnGetVersionExWFirstControl.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnGetVersionExWFirstControl.SzDllLoaded(), L"api-ms-win-core-sysinfo-l1-1-1.dll" ) );
    OSTestCheck( fTrue == pfnGetVersionExWFromLastDll( &osvi ) );
    OSTestCheck( 0 == wcscmp( pfnGetVersionExWFromLastDll.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnGetVersionExWFromLastDll.SzDllLoaded(), L"api-ms-win-core-sysinfo-l1-1-1.dll" ) );
    OSTestCheck( szDllListGetVersionExWFromLastDll != pfnGetVersionExWFromLastDll.SzDllLoaded() );
    OSTestCheck( 0 == wcscmp( pfnGetVersionExWFromLastDll.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnGetVersionExWFromLastDll.SzDllLoaded(), L"api-ms-win-core-sysinfo-l1-1-1.dll" ) );

    OSTestCheck( osvi.dwMajorVersion > 0 );
    OSTestCheck( osvi.dwBuildNumber > 0 );


    OSTestCheck( fTrue == pfnGetVersionExWGood( &osvi ) );
    OSTestCheck( 0 == wcscmp( pfnGetVersionExWGood.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnGetVersionExWGood.SzDllLoaded(), L"api-ms-win-core-sysinfo-l1-1-1.dll" ) );
    OSTestCheck( fFalse == pfnGetVersionExWBad( &osvi ) );
    OSTestCheck( ERROR_CALL_NOT_IMPLEMENTED == GetLastError() );
    OSTestCheck( pfnGetVersionExWBad.SzDllLoaded() == NULL );

HandleError:

    return err;
}


ERR ErrLoadCheckValidHandleReturn()
{
    ERR err = JET_errSuccess;

    NTOSFuncPtr( pfnCreateSemaphoreTest, L"NonExistantDll.dll\0kernel32.dll\0api-ms-win-core-synch-l1-1-1.dll\0", CreateSemaphoreExW, oslfExpectedOnWin8 );

    OSTestCheck( NULL == pfnCreateSemaphoreTest.SzDllLoaded() );

    HANDLE hValid = pfnCreateSemaphoreTest( 0, 1, 100, L"TestHandleReturn", 0, SYNCHRONIZE | DELETE );

    DWORD dwGle = GetLastError();
    OSTestCheck( ERROR_SUCCESS == dwGle || ERROR_ALREADY_EXISTS == dwGle );
    OSTestCheck( hValid != NULL && hValid != INVALID_HANDLE_VALUE );

    OSTestCheck( 0 != wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"NonExistantDll.dll" ) );
    OSTestCheck( 0 == wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnCreateSemaphoreTest.SzDllLoaded(), L"api-ms-win-core-synch-l1-1-1.dll" ) );
    
    HANDLE hDup = pfnCreateSemaphoreTest( 0, 1, 100, L"TestHandleReturn", 0, SYNCHRONIZE | DELETE );

    dwGle = GetLastError();
    OSTestCheck( ERROR_ALREADY_EXISTS == dwGle );
    OSTestCheck( INVALID_HANDLE_VALUE != hDup );

    hValid = pfnCreateSemaphoreTest( 0, 1, 100, L"TestHandleReturn2", 0, SYNCHRONIZE | DELETE );

    dwGle = GetLastError();
    OSTestCheck( ERROR_SUCCESS == dwGle );
    OSTestCheck( hValid != NULL && hValid != INVALID_HANDLE_VALUE );

    CloseHandle( hValid );
    CloseHandle( hDup );

    err = JET_errSuccess;

HandleError:

    return err;
}

ERR ErrLoadCheckInvalidHandleReturn()
{
    ERR err = JET_errSuccess;

    NTOSFuncPtr( pfnCreateSemaphoreTest, L"NonExistantDll.dll\0AnotherNonDll.dll\0", CreateSemaphoreExW, oslfExpectedOnWin8 );

    OSTestCheck( NULL == pfnCreateSemaphoreTest.SzDllLoaded() );

    HANDLE hCheck = pfnCreateSemaphoreTest( 0, 1, 100, L"TestInvalidHandleReturn", 0, SYNCHRONIZE | DELETE );

    const DWORD dwGle = GetLastError();
    OSTestCheck( ERROR_CALL_NOT_IMPLEMENTED == dwGle );
    OSTestCheck( NULL == hCheck );
    OSTestCheck( NULL == pfnCreateSemaphoreTest.SzDllLoaded() );

    CloseHandle( hCheck );

HandleError:

    return err;
}


ERR ErrLoadCheckValidBoolReturn()
{
    ERR err = JET_errSuccess;

    OSVERSIONINFOW osvi = { sizeof( osvi ), 0 };

    NTOSFuncStd( pfnGetVersionExWTest, L"kernel32.dll\0api-ms-win-core-sysinfo-l1-1-1.dll\0", GetVersionExW, oslfExpectedOnWin8 );

    OSTestCheck( NULL == pfnGetVersionExWTest.SzDllLoaded() );

    BOOL fCheck = pfnGetVersionExWTest( &osvi );

    OSTestCheck( fCheck );

    OSTestCheck( 0 == wcscmp( pfnGetVersionExWTest.SzDllLoaded(), L"kernel32.dll" ) ||
                 0 == wcscmp( pfnGetVersionExWTest.SzDllLoaded(), L"api-ms-win-core-sysinfo-l1-1-1.dll" ) );

    OSTestCheck( osvi.dwMajorVersion > 0 );

    fCheck = pfnGetVersionExWTest( &osvi );
    OSTestCheck( fCheck );

    err = JET_errSuccess;

HandleError:

    return err;
}

ERR ErrLoadCheckInvalidBoolReturn()
{
    ERR err = JET_errSuccess;

    OSVERSIONINFOW osvi = { sizeof( osvi ), 0 };

    NTOSFuncStd( pfnGetVersionExWTest, L"NonExistantDll.dll\0OtherNonDll.dll\0", GetVersionExW, oslfExpectedOnWin8 );

    OSTestCheck( NULL == pfnGetVersionExWTest.SzDllLoaded() );

    BOOL fCheck = pfnGetVersionExWTest( &osvi );

    OSTestCheck( !fCheck );
    const DWORD dwGle = GetLastError();
    OSTestCheck( ERROR_CALL_NOT_IMPLEMENTED == dwGle );
    OSTestCheck( NULL == pfnGetVersionExWTest.SzDllLoaded() );

HandleError:

    return err;
}

__kernel_entry NTSYSCALLAPI
HANDLE
NTAPI
NtCreateEvent(
    __in_opt LPSECURITY_ATTRIBUTES lpEventAttributes,
    __in     BOOL bManualReset,
    __in     BOOL bInitialState,
    __in_opt LPCWSTR lpName
    );

ERR ErrLoadPfnHook()
{
    ERR err = JET_errSuccess;
    HMODULE hNtDll = NULL;
    const void* pfnNtCreateEventExplicit = NULL;
    const void* const pfnReplace1 = (void*)0x12345678;
    const void* const pfnReplace2 = (void*)0x87654321;
    NTOSFuncStd( pfnNtCreateEventImplicit, L"ntdll.dll\0", NtCreateEvent, oslfExpectedOnWin5x | oslfRequired | oslfHookable );
    const void* pfnOld = NULL;

    hNtDll = GetModuleHandleW( L"ntdll.dll" );
    OSTestCheck( hNtDll != NULL );

    pfnNtCreateEventExplicit = GetProcAddress( hNtDll, "NtCreateEvent" );
    OSTestCheck( pfnNtCreateEventExplicit != NULL );

    pfnOld = pfnNtCreateEventImplicit.PfnLoadHook( pfnReplace1 );
    OSTestCheck( pfnOld == pfnNtCreateEventExplicit );
    OSTestCheck( pfnReplace1 == pfnNtCreateEventImplicit );

    pfnOld = pfnNtCreateEventImplicit.PfnLoadHook( pfnReplace2 );
    OSTestCheck( pfnOld == pfnReplace1 );
    OSTestCheck( pfnReplace2 == pfnNtCreateEventImplicit );

    pfnOld = pfnNtCreateEventImplicit.PfnLoadHook( pfnNtCreateEventExplicit );
    OSTestCheck( pfnOld == pfnReplace2 );
    OSTestCheck( pfnNtCreateEventExplicit == pfnNtCreateEventImplicit );

    err = JET_errSuccess;

HandleError:

    return err;
}



NTOSFuncPtr( g_pfnCreateSemaphoreTest, L"kernel32.dll\0api-ms-win-core-sysinfo-l1-1-1.dll\0", CreateSemaphoreExW, oslfExpectedOnWin5x | oslfRequired );


CUnitTest( LibraryLoadLibraryTesting, 0, "This tests that the various options of the PFN FunctionLaoder work as expected" );
ERR LibraryLoadLibraryTesting::ErrTest()
{
    JET_ERR err = JET_errSuccess;
    COSLayerPreInit     oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"ERROR: Out of memory error during OS Layer pre-init." );
        return JET_errOutOfMemory;
    }


    HANDLE hValid;
    OSTestCheck( hValid = g_pfnCreateSemaphoreTest( 0, 1, 100, L"TestGlobalHandleReturn", 0, SYNCHRONIZE | DELETE ) );
    OSTestCheck( hValid );

    OSTestCall( ErrLoadTreatableAsPointer() );

    OSTestCall( ErrLoadSkipsNonExistantDlls() );

    OSTestCall( ErrLoadCheckFreeLibrary() );

    OSTestCall( ErrLoadPresentFailsIfDllPresentButFuncIsNot() );
    OSTestCall( ErrLoadPresentFailsIfDllIsNotPresent() );

    OSTestCall( ErrLoadTestSpecificDllIsLoaded() );

    OSTestCall( ErrLoadCheckValidHandleReturn() );
    OSTestCall( ErrLoadCheckInvalidHandleReturn() );

    OSTestCall( ErrLoadCheckValidBoolReturn() );
    OSTestCall( ErrLoadCheckInvalidBoolReturn() );

    OSTestCall( ErrLoadPfnHook() );

HandleError:

    if ( err )
    {
        wprintf( L"\tDone(err=%d).\n", err );
    }
    else
    {
        wprintf( L"\tDone(success).\n" );
    }

    return err;
}

