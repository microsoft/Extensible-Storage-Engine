// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#include < malloc.h >
#include < locale.h >

LOCAL BOOL          g_fUnicodeSupport             = fFalse;

const WORD          sortidDefault               = SORT_DEFAULT;
const WORD          sortidNone                  = SORT_DEFAULT;
const LANGID        langidDefault               = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
const LANGID        langidNone                  = MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL );
const LANGID        langidInvariant             = MAKELANGID( LANG_INVARIANT, SUBLANG_NEUTRAL );

const LCID          lcidNone                    = MAKELCID( langidNone, sortidNone );
const LCID          lcidInvariant               = MAKELCID( langidInvariant, sortidNone );

const PWSTR         wszLocaleNameDefault        = L"en-US";
const PWSTR         wszLocaleNameNone           = L"";

const DWORD         dwLCMapFlagsDefaultOBSOLETE = ( LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH );
const DWORD         dwLCMapFlagsDefault         = ( LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH );

C_ASSERT( NORM_LOCALE_NAME_MAX_LENGTH == LOCALE_NAME_MAX_LENGTH );
C_ASSERT( 127 == lcidInvariant );

VOID NORMAssertCheckLocaleName( const PCWSTR wszLocaleName )
{
#ifdef DEBUG
    const ERR err = ErrNORMCheckLocaleName( NULL, wszLocaleNameDefault );

    if ( ( err != JET_errSuccess ) && ( err != JET_errUnicodeLanguageValidationFailure ) )
    {
        AssertSz( fFalse, "Unexpected failure in validating locale %ws.", wszLocaleName );
    }
#endif
}

#ifdef DEBUG
// These variables will store the last ErrNORMCheckLocale errors for the IsValidLocale
// call for both the system error (g_errSystemCheckLocale) and the mapped JET err (g_errCheckLocale).
DWORD               g_errSystemCheckLocale            = S_OK;
ERR                 g_errCheckLocale                  = JET_errSuccess;

VOID AssertNORMConstants()
{
    //  since we now persist LCMapString() flags, we must verify
    //  that NT doesn't change them from underneath us
    C_ASSERT( LCMAP_SORTKEY == 0x00000400 );
    C_ASSERT( LCMAP_BYTEREV == 0x00000800 );
    C_ASSERT( NORM_IGNORECASE == 0x00000001 );
    C_ASSERT( NORM_IGNORENONSPACE == 0x00000002 );
    C_ASSERT( NORM_IGNORESYMBOLS == 0x00000004 );
    C_ASSERT( LINGUISTIC_IGNORECASE == 0x00000010 );
    C_ASSERT( LINGUISTIC_IGNOREDIACRITIC == 0x00000020 );
    C_ASSERT( NORM_IGNOREKANATYPE == 0x00010000 );
    C_ASSERT( NORM_IGNOREWIDTH == 0x00020000 );
    C_ASSERT( NORM_LINGUISTIC_CASING == 0x08000000 );
    C_ASSERT( SORT_STRINGSORT == 0x00001000 );
    C_ASSERT( LCMAP_UPPERCASE == 0x00000200 );

    C_ASSERT( sortidDefault == 0 );
    C_ASSERT( langidDefault == 0x0409 );
    C_ASSERT( sortidNone == 0 );
    C_ASSERT( langidNone == 0 );
    C_ASSERT( lcidNone == 0 );

    // Both of the NLS functions below can fail during shutdown as
    // the registry is pulled from under us, so we will disconsider
    // failures when we're shutting down.
    NORMAssertCheckLocaleName( wszLocaleNameDefault );
    Assert( ErrNORMCheckLCMapFlags( NULL, dwLCMapFlagsDefault, fFalse ) == JET_errSuccess );
}
#endif

BOOL FNORMLCMapFlagsHasUpperCase( DWORD dwMapFlags )
{
    return !!( dwMapFlags & LCMAP_UPPERCASE );
}

const LANGID LangidFromLcid( const LCID lcid )
{
    return LANGIDFROMLCID( lcid );
}

//  allocates memory for psz using new []
LOCAL ERR ErrNORMGetLcidInfo( const LCID lcid, const LCTYPE lctype, __deref_out PWSTR * psz )
{
    ERR         err         = JET_errSuccess;
    const INT   cbNeeded    = GetLocaleInfoW( lcid, lctype, NULL, 0 );

    if ( NULL == ( *psz = new WCHAR[cbNeeded] ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    else
    {
        const INT   cbT     = GetLocaleInfoW( lcid, lctype, *psz, cbNeeded );

        Assert( 0 == cbT || cbT == cbNeeded );
        if ( 0 == cbT )
        {
            Call( ErrERRCheck( JET_errInternalError ) );
        }
    }

HandleError:
    return err;
}

//  allocates memory for psz using new []
LOCAL ERR ErrNORMGetLocaleNameInfo( PCWSTR wszLocaleName, const LCTYPE lctype, __deref_out PWSTR * psz )
{
    ERR         err         = JET_errSuccess;
    const INT   cbNeeded    = GetLocaleInfoEx( wszLocaleName, lctype, NULL, 0 );

    if ( NULL == ( *psz = new WCHAR[cbNeeded] ) )
    {
        Call( ErrERRCheck( JET_errOutOfMemory ) );
    }
    else
    {
        const INT   cbT     = GetLocaleInfoEx( wszLocaleName, lctype, *psz, cbNeeded );

        Assert( 0 == cbT || cbT == cbNeeded );
        if ( 0 == cbT )
        {
            Call( ErrERRCheck( JET_errInternalError ) );
        }
    }

HandleError:
    return err;
}

LOCAL VOID NORMReportInvalidLcid( INST * const pinst, const LCID lcid, const ERR errReport, const DWORD errSystem )
{
    WCHAR           szLcid[16];
    WCHAR           szError[64];
    WCHAR           szSystemError[ 64 ];
    WCHAR *         szLanguage      = NULL;
    WCHAR *         szEngLanguage   = NULL;
    const ULONG     cszT            = 6;
    const WCHAR*    rgszT[cszT];
    WCHAR *         szSystemErrorDescription    = NULL;
    WCHAR *         szFallbackLanguage = L"Unknown";

    FormatMessageW( (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                    NULL,
                    errSystem,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                    LPWSTR( &szSystemErrorDescription ),
                    0,
                    NULL );

//  these routines allocate memory, remember to free it with delete[]

    if ( ErrNORMGetLcidInfo( lcid, LOCALE_SLANGUAGE, &szLanguage ) < JET_errSuccess )
    {
        szLanguage = szFallbackLanguage;
    }
    
    if ( ErrNORMGetLcidInfo( lcid, LOCALE_SENGLANGUAGE, &szEngLanguage ) < JET_errSuccess )
    {
        szEngLanguage = szFallbackLanguage;
    }

    rgszT[0] = szLcid;
    rgszT[1] = szLanguage;
    rgszT[2] = szEngLanguage;
    rgszT[3] = szSystemError;
    rgszT[4] = szSystemErrorDescription ? szSystemErrorDescription : L"";;
    rgszT[5] = szError;

    OSStrCbFormatW( szLcid, sizeof( szLcid ), L"0x%0*x", (DWORD)sizeof(LCID)*2, lcid );
    OSStrCbFormatW( szError, sizeof( szError ), L"%i (0x%08x)", errReport, errReport );
    OSStrCbFormatW( szSystemError, sizeof( szSystemError ), L"%u (0x%08x)", errSystem, errSystem );

    UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            LANGUAGE_NOT_SUPPORTED_ID,
            cszT,
            rgszT,
            0,
            NULL,
            pinst );

    if ( szEngLanguage != szFallbackLanguage )
    {
        delete [] szEngLanguage;
    }

    if ( szLanguage != szFallbackLanguage )
    {
        delete [] szLanguage;
    }

    LocalFree( szSystemErrorDescription );
}

LOCAL VOID NORMReportInvalidLocaleName( INST * const pinst, PCWSTR wszLocaleName, const ERR errReport, const DWORD errSystem )
{
    WCHAR           szError[64];
    WCHAR           szSystemError[ 64 ];
    WCHAR *         szLanguage      = NULL;
    WCHAR *         szEngLanguage   = NULL;
    const ULONG     cszT            = 6;
    const WCHAR*    rgszT[cszT];
    WCHAR *         szSystemErrorDescription    = NULL;
    WCHAR *         szFallbackLanguage = L"Unknown";

    FormatMessageW( (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                    NULL,
                    errSystem,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                    LPWSTR( &szSystemErrorDescription ),
                    0,
                    NULL );

    //these routines allocate memory, remember to free it with delete[]
    if ( ErrNORMGetLocaleNameInfo( wszLocaleName, LOCALE_SLANGUAGE, &szLanguage ) < JET_errSuccess )
    {
        szLanguage = szFallbackLanguage;
    }
    
    if ( ErrNORMGetLocaleNameInfo( wszLocaleName, LOCALE_SENGLANGUAGE, &szEngLanguage ) < JET_errSuccess )
    {
        szEngLanguage = szFallbackLanguage;
    }

    rgszT[0] = wszLocaleName;
    rgszT[1] = szLanguage;
    rgszT[2] = szEngLanguage;
    rgszT[3] = szSystemError;
    rgszT[4] = szSystemErrorDescription ? szSystemErrorDescription : L"";;
    rgszT[5] = szError;

    OSStrCbFormatW( szError, sizeof( szError ), L"%i (0x%08x)", errReport, errReport );
    OSStrCbFormatW( szSystemError, sizeof( szSystemError ), L"%u (0x%08x)", errSystem, errSystem );

    UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            LANGUAGE_NOT_SUPPORTED_ID,
            cszT,
            rgszT,
            0,
            NULL,
            pinst );

    if ( szEngLanguage != szFallbackLanguage )
    {
        delete [] szEngLanguage;
    }

    if ( szLanguage != szFallbackLanguage )
    {
        delete [] szLanguage;
    }

    LocalFree( szSystemErrorDescription );
}

NTOSFuncPD( g_pfnGetNLSVersionEx, GetNLSVersionEx );

#ifndef ESENT
// Used to detect Win10 only

WINBASEAPI
BOOL
APIENTRY
WaitForDebugEventEx(
    _Out_ LPDEBUG_EVENT lpDebugEvent,
    _In_ DWORD dwMilliseconds
    );

NTOSFuncPD( g_pfnWaitForDebugEventEx, WaitForDebugEventEx );
#endif  //  !ESENT

//  ================================================================
LOCAL ERR ErrLoadNORMFunctions()
//  ================================================================
{
    NTOSFuncStdPreinit( g_pfnGetNLSVersionEx, g_mwszzLocalizationLibs, GetNLSVersionEx, oslfExpectedOnWin6 | oslfRequired );
#ifndef ESENT
    NTOSFuncStdPreinit( g_pfnWaitForDebugEventEx, g_mwszzLocalizationLibs, WaitForDebugEventEx, oslfExpectedOnWin10 );
#endif  //  !ESENT

    return JET_errSuccess;
}


//  ================================================================
LOCAL VOID UnloadNORMFunctions()
//  ================================================================
{
}


//  ================================================================
LOCAL_BROKEN BOOL WINAPI GetNLSNotSupported(
    _In_ NLS_FUNCTION       Function,
    _In_ LCID               Locale,
    _Out_ LPNLSVERSIONINFO  lpVersionInformation )
//  ================================================================
{
    Assert( NULL != lpVersionInformation );
    if( NULL == lpVersionInformation )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return fFalse;
    }
    
    Assert( sizeof( NLSVERSIONINFO ) == lpVersionInformation->dwNLSVersionInfoSize );
    if( sizeof( NLSVERSIONINFO ) != lpVersionInformation->dwNLSVersionInfoSize )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return fFalse;
    }
    
    lpVersionInformation->dwNLSVersion = 0;
    lpVersionInformation->dwDefinedVersion = 0;

    return fFalse;
}

//  ================================================================
LOCAL_BROKEN BOOL WINAPI GetNLSExNotSupported(
    _In_ NLS_FUNCTION           Function,
    _In_ LPCWSTR                LocaleName,
    _Out_ LPNLSVERSIONINFOEX    lpVersionInformation )
//  ================================================================
{
    Assert( NULL != lpVersionInformation );
    if( NULL == lpVersionInformation )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return fFalse;
    }

    Assert( sizeof( NLSVERSIONINFOEX ) == lpVersionInformation->dwNLSVersionInfoSize );
    if( sizeof( NLSVERSIONINFOEX ) != lpVersionInformation->dwNLSVersionInfoSize )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return fFalse;
    }

    lpVersionInformation->dwNLSVersion = 0;
    lpVersionInformation->dwDefinedVersion = 0;
    lpVersionInformation->dwEffectiveId = 0;
    memset( &(lpVersionInformation->guidCustomVersion), 0, sizeof( GUID ) );

    return fFalse;
}

//  ================================================================
ERR ErrNORMCheckLocaleName( _In_ INST * const pinst, __in_z PCWSTR const wszLocaleName )
//  ================================================================
//  +
//  Calls GetNLSVersionEx() on the locale name, to verify that it is a valid locale.
//  -
{
    ERR err = JET_errSuccess;
    DWORD errSystem = S_OK;

    if ( NULL == wszLocaleName )
    {
        Error( ErrERRCheck( JET_errInvalidLanguageId ) );
    }

    NLSVERSIONINFOEX nlsversioninfo;
    nlsversioninfo.dwNLSVersionInfoSize = sizeof( nlsversioninfo );

    const BOOL fValidLocale = (*g_pfnGetNLSVersionEx)( COMPARE_STRING, wszLocaleName, &nlsversioninfo );

    if ( !fValidLocale )
    {
        errSystem = GetLastError();

        switch ( errSystem )
        {
            case ERROR_INSUFFICIENT_BUFFER:
            case ERROR_INVALID_FLAGS:
                AssertSz( fFalse, "Unexpected error %d from GetNLSVersionEx", errSystem );
                Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );

            case ERROR_INVALID_PARAMETER:
                    Error( ErrERRCheck( JET_errInvalidLanguageId ) );

            default:
                // For everything else, we assume it is is an invalid language ID.
                Error( ErrERRCheck( JET_errInvalidLanguageId ) );
        }
    }

    Call( ErrFaultInjection( 53064 ) );

HandleError:

    if ( err < JET_errSuccess )
    {
#ifdef DEBUG
        // Record these as forensic information in case we need to debug this.
        g_errSystemCheckLocale = errSystem;
        g_errCheckLocale = err;
#endif

        // If the locale is not valid or we failed to validate it, we need
        // to report it in the event log. We will not report JET_errUnicodeLanguageValidationFailure
        // since it is not actionable for administrators.
        if ( JET_errUnicodeLanguageValidationFailure != err )
        {
            NORMReportInvalidLocaleName( pinst, wszLocaleName, err, errSystem );
        }
    }

    return err;
}

//  ================================================================
ERR ErrNORMCheckLocaleVersion(
    _In_ const NORM_LOCALE_VER* pnlv )
//  ================================================================
//  +
//  Calls LCMapString() on the locale name and versions, to verify that it is a valid locale/version
// combination.
//  -
{
    ERR err = JET_errSuccess;

    const WCHAR wszTest[] = L"TurkishI";
    BYTE rgbNormalized[ 64 ];
    INT cbActual;

    Call( ErrNORMMapString(
        pnlv,
        (BYTE*) &wszTest[0],
        sizeof( wszTest ),
        &rgbNormalized[0],
        sizeof( rgbNormalized ),
        &cbActual ) );

HandleError:

    return err;
}


//  ================================================================
ERR ErrNORMGetSortVersion( __in_z PCWSTR wszLocaleName, _Out_ QWORD * const pqwVersion, __out_opt SORTID * const psortID, _In_ const BOOL fErrorOnInvalidId )
//  ================================================================
//
//-
{
    ERR err = JET_errSuccess;
    DWORD errSystem = 0;
    *pqwVersion = 0;

#ifndef RTM
    //  check registry overrides for this LCID
    //  the override should be a string of the form version.defined version (e.g. 123.789)
    //  require LCID to have compatibility with test hooks
    LCID lcid = lcidNone;
    err = ErrNORMLocaleToLcid( wszLocaleName, &lcid );
    if ( err == JET_errSuccess )
    {
        WCHAR wsz[64];
        WCHAR wszLcid[16];

        OSStrCbFormatW( wszLcid, sizeof( wszLcid ), L"%d", lcid );

        if ( FOSConfigGet( L"OS\\LCID", wszLcid, wsz, sizeof( wsz ) ) && wsz[0] )
        {
            WCHAR * wszNextToken = NULL;
            const WCHAR * const wszNLS      = wcstok_s( wsz, L".", &wszNextToken );
            const WCHAR * const wszDefined  = wcstok_s( NULL, L".", &wszNextToken );
            const DWORD dwNLS               = _wtoi( wszNLS );
            const DWORD dwDefined           = _wtoi( wszDefined );
            /*
            _TCHAR                  szMessage[256];
            const _TCHAR *          rgszT[1]            = { szMessage };

            _stprintf(
                    szMessage,
                    _T( "Sort ordering for LCID %d changed through the registry to %d.%d" ),
                    lcid,
                    dwNLS,
                    dwDefined );
            UtilReportEvent(
                    eventWarning,
                    GENERAL_CATEGORY,
                    PLAIN_TEXT_ID,
                    1,
                    rgszT );
            */

            *pqwVersion = QwSortVersionFromNLSDefined( dwNLS, dwDefined );

            if ( NULL != psortID )
            {
                *psortID = g_sortIDNull;
            }

            return JET_errSuccess;
        }
    }
#endif

    NLSVERSIONINFOEX nlsversioninfoex;
    nlsversioninfoex.dwNLSVersionInfoSize = sizeof( nlsversioninfoex );

    const BOOL fSuccess = (*g_pfnGetNLSVersionEx)( COMPARE_STRING, wszLocaleName, &nlsversioninfoex );

    if( !fSuccess )
    {
        if ( fErrorOnInvalidId )
        {
            Assert( 0 == *pqwVersion );
            errSystem = GetLastError();
            Error( ErrERRCheck( JET_errInvalidLanguageId ) );
        }
        else
        {
            *pqwVersion = 0;
            if ( NULL != psortID )
            {
                *psortID = g_sortIDNull;
            }
            return JET_errSuccess;
        }
    }
    else
    {
#ifndef RTM
        //  If the testing has specified to either "upgrade" windows NLS version, OR upgrade just
        //  a specific LCID, we'll give the NLS version a +7 boost in version.
        ULONG ulTestLcid = (ULONG)UlConfigOverrideInjection( 51415, fFalse );
        if ( ulTestLcid == fTrue ||
             ( ulTestLcid == lcid && lcid != lcidNone ) )
        {
            nlsversioninfoex.dwNLSVersion += 7;
        }

        //  If the testing has specified to either "upgrade" windows NLS sort ID, OR upgrade just
        //  a specific LCID, we'll give the NLS version a +7 boost in version.
        ulTestLcid = (ULONG)UlConfigOverrideInjection( 43220, fFalse );
        if ( ulTestLcid == fTrue ||
             ( ulTestLcid == lcid && lcid != 0 ) )
        {
            nlsversioninfoex.guidCustomVersion.Data1 += 7;
        }

        DWORD dwNLSVersion = (DWORD)UlConfigOverrideInjection( 39204, 0);
        if ( dwNLSVersion != 0 )
        {
            nlsversioninfoex.dwNLSVersion = dwNLSVersion;
        }
#endif

        *pqwVersion = QwSortVersionFromNLSDefined( nlsversioninfoex.dwNLSVersion, nlsversioninfoex.dwDefinedVersion );

        if ( NULL != psortID )
        {
            *psortID = nlsversioninfoex.guidCustomVersion;
        }

        return JET_errSuccess;
    }

HandleError:
    Assert( err < JET_errSuccess );

    NORMReportInvalidLocaleName( NULL, wszLocaleName, err, errSystem );

    return err;
}


//  ================================================================
BOOL FNORMGetNLSExIsSupported()
//  ================================================================
{
    return ( g_pfnGetNLSVersionEx.ErrIsPresent() == JET_errSuccess );
}

BOOL g_fNormMemoryLeakAcceptable = fFalse; // used only as a unit test thunk..
//  ================================================================
BOOL FNORMIGetNLSExWithVersionInfoIsSupported()
//  ================================================================
{
#ifndef ESENT
    // Due to a memory leak in LCMapStringEx on Win8 through some Win10 builds we were forced to disable this
    // feature on builds less than Win10.  We are forced to detect Win10 indirectly by testing for a new Win32
    // API because the Win32 version APIs now lie unless the app is properly manifested and we cannot know if
    // a given application using ESE/ESENT.DLL is properly manifested and this lack of a manifest itself will
    // not disable this feature.  We only need to test this on ESE as this bug and the code to activate the bug
    // will not coexist for ESENT.
    if ( g_pfnWaitForDebugEventEx.ErrIsPresent() != JET_errSuccess )
    {
        return g_fNormMemoryLeakAcceptable && FNORMGetNLSExIsSupported();
    }
#endif  //  !ESENT
    
    Expected( FNORMGetNLSExIsSupported() );  // True on Vista+ (when NT API came in) ... so next line is basically: return true; (but do the right thing just in case)
    return FNORMGetNLSExIsSupported(); // but for defense in depth we'll defer to the other function as a check ..
}

BOOL FNORMNLSVersionEquals( QWORD qwVersionCreated, QWORD qwVersionCurrent )
{
    DWORD dwNLSVersionCreated;
    DWORD dwNLSVersionCurrent;
    DWORD dwDummy;

    QwSortVersionToNLSDefined(qwVersionCreated, &dwNLSVersionCreated, & dwDummy);
    QwSortVersionToNLSDefined(qwVersionCurrent, &dwNLSVersionCurrent, & dwDummy);

    DWORD dwNLSVersionHighest;
    DWORD dwNLSVersionLowest;

    dwNLSVersionHighest = dwNLSVersionCreated >= dwNLSVersionCurrent ? dwNLSVersionCreated : dwNLSVersionCurrent;
    dwNLSVersionLowest = dwNLSVersionCreated < dwNLSVersionCurrent ? dwNLSVersionCreated : dwNLSVersionCurrent;

    // OM 3100737: Ignore lowest byte in NLS sort order key for certain OS versions (601.1 - 601.2)
    if ( dwNLSVersionHighest == 0x00060102 && dwNLSVersionLowest == 0x00060101 )
    {
        return fTrue;
    }

    // OM 3100737: Ignore lowest byte in NLS sort order key for certain NLS versions (602.d - 602.e)
    if ( dwNLSVersionHighest == 0x0006020e && dwNLSVersionLowest == 0x0006020d )
    {
        return fTrue;
    }

    // RS4 fixed a CompareString bug and updated version to 0006020F
    if ( dwNLSVersionHighest == 0x0006020f && dwNLSVersionLowest == 0x0006020e )
    {
        return fTrue;
    }

    if ( dwNLSVersionHighest == 0x0006020f && dwNLSVersionLowest == 0x0006020d )
    {
        return fTrue;
    }

    return qwVersionCreated == qwVersionCurrent;
}

LOCAL ERR ErrNORMReportInvalidLCMapFlags( INST * const pinst, const DWORD dwLCMapFlags )
{
    WCHAR           szLCMapFlags[16];
    const ULONG     cszT                = 1;
    const WCHAR*    rgszT[cszT]         = { szLCMapFlags };

    OSStrCbFormatW( szLCMapFlags, sizeof( szLCMapFlags ), L"0x%0*x", (DWORD)sizeof(DWORD)*2, dwLCMapFlags );
    UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            INVALID_LCMAPFLAGS_ID,
            cszT,
            rgszT,
            0,
            NULL,
            pinst );

    // Assert ( fFalse );
    return ErrERRCheck( JET_errInvalidLCMapStringFlags );
}

//This is a temp fix to make the following ELS flag working on fbl_find_dev
//Please remove it once the code is built with _WIN32_WINNT_WIN7
#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 0x00000008  // use digits as numbers sort method
#endif

// IMPORTANT: This function can fail during shutdown as the registry is pulled from under us.

ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _In_ const DWORD dwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization )
{
    //  CONSIDER:  Validate flags based on OS version
    const DWORD     dwValidSortKeyFlags = ( LCMAP_BYTEREV
                                        | NORM_IGNORECASE
                                        | NORM_IGNORENONSPACE
                                        | NORM_IGNORESYMBOLS
                                        | LINGUISTIC_IGNORECASE
                                        | LINGUISTIC_IGNOREDIACRITIC
                                        | SORT_DIGITSASNUMBERS
                                        | NORM_IGNOREKANATYPE
                                        | NORM_IGNOREWIDTH
                                        | NORM_LINGUISTIC_CASING
                                        | SORT_STRINGSORT );

    // Most people want (the common case) the various "improved linguistic"
    // support of the above flag, which only working with LCMAP_SORTKEY.
    // Occasionally people want LCMAP_UPPERCASE (generally to compare file paths)
    // which must be used without the LCMAP_SORTKEY flag.
    //
    if ( LCMAP_SORTKEY == ( dwLCMapFlags & ~dwValidSortKeyFlags )
        || ( LCMAP_UPPERCASE == dwLCMapFlags && fUppercaseTextNormalization ) )
    {
        return JET_errSuccess;
    }
    else
    {
        return ErrNORMReportInvalidLCMapFlags( pinst, dwLCMapFlags );
    }
}


// IMPORTANT: This function can fail during shutdown as the registry is pulled from under us.

ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _Inout_ DWORD * const pdwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization )
{
    //  LCMAP_UPPERCASE can not be combined with LCMAP_SORTKEY.
    if ( LCMAP_UPPERCASE != *pdwLCMapFlags )
    {
        *pdwLCMapFlags |= LCMAP_SORTKEY;
    }

    return ErrNORMCheckLCMapFlags( pinst, *pdwLCMapFlags, fUppercaseTextNormalization );
}

#ifdef DEBUG_NORM
VOID NORMPrint( const BYTE * const pb, const INT cb )
{
    INT     cbT     = 0;

    while ( cbT < cb )
    {
        INT i;
        INT cbTSav  = cbT;
        for ( i = 0; i < 16; i++ )
        {
            if ( cbT < cb )
            {
                printf( "%02x ", pb[cbT] );
                cbT++;
            }
            else
            {
                printf( "   " );
            }
        }

        cbT = cbTSav;
        for ( i = 0; i < 16; i++ )
        {
            if ( cbT < cb )
            {
                printf( "%c", ( isprint( pb[cbT] ) ? pb[cbT] : '.' ) );
                cbT++;
            }
            else
            {
                printf( " " );
            }
        }
        printf( "\n" );
    }
}
#endif  //  DEBUG_NORM

// The combination of dwNLSVersion, and guidCustomVersion
// identify specific sort behavior, persist those to ensure identical
// behavior in the future.

INLINE INT CbNORMMapString_(
    _In_ const NORM_LOCALE_VER* const       pnlv,
    _In_reads_( cbColumn ) BYTE *           pbColumn,
    _In_ const INT                          cbColumn,
    _Out_opt_cap_( cbKeyMost ) BYTE *       rgbKey,
    _In_ const INT                          cbKeyMost )
{
    Assert( g_fUnicodeSupport );

    if ( !FNORMSortidIsZeroes( &pnlv->m_sortidCustomSortVersion ) )
    {
        Assert( 0 != pnlv->m_dwNlsVersion );
        Assert( 0 != pnlv->m_dwDefinedNlsVersion );
    }

    // SOMEONEn says:
    // Basically fill out the NlsVersionInfoEx structure (eg: what it returned
    // from GetNlsVersionEx()). The important parts are dwNLSVersion
    // (definedVersion is ignored), and guidCustomVersion if present (otherwise
    // it uses dwEffectiveId, but that's not perfect, so use the GUID instead,
    // but there's no GUID before Windows 8).

    NLSVERSIONINFO* psortinfoUsed = NULL;
    NLSVERSIONINFOEX sortinfo;
    sortinfo.dwNLSVersionInfoSize = sizeof( sortinfo );
    sortinfo.dwNLSVersion = pnlv->m_dwNlsVersion;
    sortinfo.dwDefinedVersion = pnlv->m_dwDefinedNlsVersion; // Ignored on Win8+. Used in Win7.

    // These two fields are added in NLSVERSIONINFOEX (ignored on Win7).
    sortinfo.dwEffectiveId = 0; // SOMEONEn SOMEONE said this is now ignored.
    sortinfo.guidCustomVersion = pnlv->m_sortidCustomSortVersion;

    if ( FNORMIGetNLSExWithVersionInfoIsSupported() && ( 0 != pnlv->m_dwNlsVersion ) )
    {
        psortinfoUsed = (NLSVERSIONINFO*) &sortinfo;

        // Must be at least Vista.
        Enforce( DwUtilSystemVersionMajor() >= 6 );

        // If running Vista/Windows 7, NLSVERSIONEX is not supported.
        if ( 6 == DwUtilSystemVersionMajor() && DwUtilSystemVersionMinor() <= 1 )
        {
            // Win7 didn't know about NLSVERSIONINFOEX.
            sortinfo.dwNLSVersionInfoSize = sizeof( NLSVERSIONINFO );
        }
        else
        {
            // If the effective NLS version is windows 7 and below, uses
            // NLSVERSIONINFO as NLSVERSIONEX is not supported.
            if ( sortinfo.dwNLSVersion < 0x00060200 )
            {
                sortinfo.dwNLSVersionInfoSize = sizeof( NLSVERSIONINFO );
            }

            // Check that it's Win8 or above.
            // Note that the version will be at most 6.2 unless the executable has opted
            // in to be version-aware via the executable manifest.
            Assert( ( 6 == DwUtilSystemVersionMajor() && DwUtilSystemVersionMinor() >= 2 ) ||
                    DwUtilSystemVersionMajor() > 6 );
        }
    }

    Assert( ( pnlv->m_dwNormalizationFlags & LCMAP_SORTKEY ) || ( pnlv->m_dwNormalizationFlags == LCMAP_UPPERCASE ) );
    INT cbSize = 0;
#pragma warning(suppress: 26000)
    // From MSDN https://msdn.microsoft.com/en-us/library/windows/desktop/dd318702(v=vs.85).aspx
    // cchDest is cb in case of sort-key and cch otherwise.
    // -------------------------------------------------------------------------
    // cchDest [in]
    // Size, in characters, of the buffer indicated by lpDestStr. If the application is using
    // the function for string mapping, it supplies a character count for this parameter.
    // If space for a terminating null character is included in cchSrc, cchDest must also 
    // include space for a terminating null character.
    // If the application is using the function to generate a sort key, it supplies a byte
    // count for the size. This byte count must include space for the sort key 0x00 terminator.
    // The application can set cchDest to 0. In this case, the function does not use the lpDestStr
    // parameter and returns the required buffer size for the mapped string or sort key.
    cbSize = LCMapStringEx(
                pnlv->m_wszLocaleName,
                pnlv->m_dwNormalizationFlags,
                (LPCWSTR)pbColumn,
                cbColumn / sizeof( WCHAR ),
                (LPWSTR)rgbKey,
                ( pnlv->m_dwNormalizationFlags & LCMAP_SORTKEY ) ? cbKeyMost : cbKeyMost / sizeof( WCHAR ),
                psortinfoUsed,
                NULL,
                0 );
#pragma warning(suppress: 26000)
    if ( ( pnlv->m_dwNormalizationFlags & LCMAP_SORTKEY ) == 0 )
    {
        // For non sort key usage scenarios, the returned value is in characters
        cbSize = cbSize * sizeof(WCHAR);
    }

    return cbSize;
}

ERR ErrNORMMapString(
    _In_ const NORM_LOCALE_VER*     pnlv,
    _In_reads_(cbColumn) BYTE *     pbColumn,
    _In_ INT                        cbColumn,
    _Out_writes_to_(cbMax, *pcbSeg) BYTE * const    rgbSeg,
    _In_ const INT                  cbMax,
    _Out_ INT * const               pcbSeg )
{
    ERR             err             = JET_errSuccess;
    const size_t        cbColumnStack   = 256;
    BYTE            pbColumnStack[cbColumnStack];
    BYTE*           pbColumnAligned = NULL;
    const size_t        cbColumnAligned = cbColumn + sizeof(wchar_t);   // reserve space for a null-terminator
    const size_t        cbKeyStack      = cbColumnStack;
    BYTE            rgbKeyStack[ cbKeyStack ];
    BYTE*           rgbKeyAlloc     = NULL;
    INT             cbKeyMax        = 0;
    BYTE*           rgbKey          = NULL;
    INT             cbKey           = 0;

    Assert( NULL != pnlv );
    Assert( NULL != pnlv->m_wszLocaleName );

    if ( !g_fUnicodeSupport )
    {
        Error( ErrERRCheck( JET_errUnicodeNormalizationNotSupported ) );
    }

    //  assert non-zero length unicode string
    //
    Assert( cbColumn > 0 );

    //  assert that the unicode string length is valid
    //
    Assert( 0 == ( cbColumn % sizeof( wchar_t ) ) );

    //  NOTE:  at one time, we used to truncate our column data to 256 bytes
    //  prior to the normalization call.  we now do this at a higher level and
    //  then only to support legacy index formats.  this changed when we added
    //  support for max key lengths larger than 255 bytes.  Further, note that
    //  this old behavior was BROKEN because it is possible that you will need
    //  more than 256 bytes of Unicode data to generate 255 bytes of sort key
    //  because there are many cases where a given char will not result in a
    //  sort weight or that a given pair of chars will result in a single sort
    //  weight


    if ( cbColumnAligned <= cbColumnStack )
    {
        pbColumnAligned = pbColumnStack;
    }
    else
    {
        Alloc( pbColumnAligned = (BYTE*)PvOSMemoryHeapAlloc( cbColumnAligned ) );
    }

    UtilMemCpy( pbColumnAligned, pbColumn, cbColumn );
    *(wchar_t *)(pbColumnAligned + cbColumn) = 0;
    pbColumn = pbColumnAligned;

    //  in the common case, we will be able to fit the normalized source data
    //  into our stack buffer
    //
    rgbKey      = rgbKeyStack;
    cbKeyMax    = cbKeyStack;

    //  attempt to normalize the source data using larger buffers until we
    //  succeed or fail with some other fatal error
    //
    while ( !( cbKey = CbNORMMapString_(    pnlv,
                                            pbColumn,
                                            cbColumn,
                                            rgbKey,
                                            cbKeyMax ) ) )
    {
        //  check for failures other than insufficient buffer
        //
        switch ( GetLastError() )
        {
            case ERROR_INVALID_USER_BUFFER:
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_WORKING_SET_QUOTA:
            case ERROR_NO_SYSTEM_RESOURCES:
                Error( ErrERRCheck( JET_errOutOfMemory ) );

            case ERROR_BADDB:
                // This has been a common case we have been encountering in
                // Windows 8. Different circumstances seem to cause the registry
                // to be pulled under this check, when shutting down.
                Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );
            
            default:
                Error( ErrERRCheck( JET_errUnicodeTranslationFail ) );

            case ERROR_INSUFFICIENT_BUFFER:
                //  exit switch
                break;
        }

        //  get the amount of buffer we will need to normalize the source data
        //
        cbKeyMax = CbNORMMapString_(    pnlv,
                                        pbColumn,
                                        cbColumn,
                                        NULL,
                                        0 );
        if ( 0 == cbKeyMax )
        {
            //  check for failures other than insufficient buffer
            //
            const DWORD dwGle = GetLastError();
            switch ( dwGle )
            {
                case ERROR_INVALID_USER_BUFFER:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_WORKING_SET_QUOTA:
                case ERROR_NO_SYSTEM_RESOURCES:
                    Error( ErrERRCheck( JET_errOutOfMemory ) );
            
                case ERROR_BADDB:
                    // This has been a common case we have been encountering in
                    // Windows 8. Different circumstances seem to cause the registry
                    // to be pulled under this check, when shutting down.
                    Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );

                default:
                    AssertSz( ERROR_INSUFFICIENT_BUFFER != dwGle,
                              "Unexpected insufficient buffer when getting buffer size for normalization. Received %d.",
                              dwGle );

                    Error( ErrERRCheck( JET_errUnicodeTranslationFail ) );
            }
        }

        //  allocate enough memory to succeed on the next call
        //
        OSMemoryHeapFree( rgbKeyAlloc );
        Alloc( rgbKeyAlloc = (BYTE*)PvOSMemoryHeapAlloc( cbKeyMax ) );
        rgbKey = rgbKeyAlloc;
    }

    //  if we have made it this far, then our locale must be valid. The NLS function below
    //  can fail during shutdown as the registry is pulled from under us, so we will disconsider
    //  failures when we're shutting down.  Additionally, we will ignore JET_errUnicodeLanguageValidationFailure
    //  for when for some reason the registry is being made unavailable for the IsValidLocale call.
    //
    NORMAssertCheckLocaleName( pnlv->m_wszLocaleName );

    //  copy the normalized source data to the output buffer
    //
    if ( pnlv->m_dwNormalizationFlags == LCMAP_UPPERCASE )
    {
        if ( cbKey > (INT)( cbMax - sizeof( WCHAR ) ) )
        {
            err = ErrERRCheck( wrnFLDKeyTooBig );
            *pcbSeg = cbMax;
        }
        else
        {
            CallS( err );
            err = JET_errSuccess;
            *pcbSeg = cbKey + sizeof( WCHAR );
            C_ASSERT( sizeof(WCHAR) == sizeof( UnalignedBigEndian< WCHAR > ) );

            INT cbPos = 0;
            UnalignedBigEndian< WCHAR > * wszDestBuf = reinterpret_cast<UnalignedBigEndian< WCHAR > *>(rgbSeg);

            while ( cbPos < cbKey )
            {
                wszDestBuf[cbPos / sizeof(WCHAR)] = * ( reinterpret_cast< WCHAR *>( &rgbKey[ cbPos ] ) );
                cbPos += sizeof( WCHAR );
            }
            *( WCHAR * )&rgbSeg[ cbKey ] = L'\0';
        }
    }
    else
    {
        if ( cbKey > cbMax )
        {
            err = ErrERRCheck( wrnFLDKeyTooBig );
            *pcbSeg = cbMax;
        }
        else
        {
            CallS( err );
            err = JET_errSuccess;
            *pcbSeg = cbKey;
        }

        UtilMemCpy( rgbSeg, rgbKey, *pcbSeg );
    }

#ifdef DEBUG_NORM
    printf( "\nOriginal Text (length %d):\n", cbColumn );
    NORMPrint( pbColumn, cbColumn );
    printf( "Normalized Text (length %d):\n", *pcbSeg );
    NORMPrint( rgbSeg, *pcbSeg );
    printf( "\n" );
#endif

HandleError:
    // if our column didnt fit in the stack buff, we allocated mem from the heap
    if ( cbColumnAligned > cbColumnStack )
    {
        OSMemoryHeapFree( pbColumnAligned );
    }
    OSMemoryHeapFree( rgbKeyAlloc );
    return err;
}


//  post-terminate norm subsystem

void OSNormPostterm()
{
    UnloadNORMFunctions();
}

//  pre-init norm subsystem

BOOL FOSNormPreinit()
{
    if ( ErrLoadNORMFunctions() )
        return fFalse;

    return fTrue;
}


//  terminate norm subsystem

void OSNormTerm()
{
    g_fUnicodeSupport = fFalse;
}

//  init norm subsystem

ERR ErrOSNormInit()
{
    ERR err = JET_errSuccess;
    
    //  a unit test for the convenience functions

    g_fUnicodeSupport = ( 0 != LCMapStringW( LOCALE_NEUTRAL, LCMAP_LOWERCASE, L"\0", 1, NULL, 0 ) );

    //  This has been a common case we have been encountering in
    //  Windows 8. Different circumstances seem to cause the registry
    //  to be pulled under this check, when shutting down.
    if ( !g_fUnicodeSupport &&
        ERROR_BADDB == GetLastError() )
    {
        Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );
    }

#ifndef ESENT
    //  Preload this in a single threaded environment. it will fail if we're before Win10, which
    //  is what we use it for above. ;-)

    (void)g_pfnWaitForDebugEventEx.ErrIsPresent();
#endif  //  !ESENT

#ifdef DEBUG
    AssertNORMConstants();
#endif  //  DEBUG

HandleError:
    return err;
}

ERR ErrNORMLcidToLocale(
    _In_ const LCID lcid,
    __out_ecount( cchLocale ) PWSTR wszLocale,
    _In_ ULONG cchLocale )
{
    ERR err = JET_errSuccess;
    DWORD errSystem = S_OK;
    
    if ( cchLocale == 0 )
    {
        Error( ErrERRCheck( JET_errBufferTooSmall ) );
    }

    if ( LCIDToLocaleName( lcid, wszLocale, cchLocale, 0 ) == 0 )
    {
        errSystem = GetLastError();
        if ( errSystem == ERROR_INSUFFICIENT_BUFFER )
        {
            Error( ErrERRCheck( JET_errBufferTooSmall ) );
        }
        
        Error( ErrERRCheck ( JET_errInvalidLanguageId ) );
    }

HandleError:
    
    if ( err < JET_errSuccess )
    {
        if ( JET_errBufferTooSmall != err)
        {
            NORMReportInvalidLcid( NULL, lcid, err, errSystem );
        }
    }
    
    return err;
}

// Stolen copy of the __ascii_towlower(), _wcsicmp() code ... remarkable irony in the former name.
#define __ascii_towlower(c)      ( (((c) >= L'A') && ((c) <= L'Z')) ? ((c) - L'A' + L'a') : (c) )

INT __ese_wcsicmp( const wchar_t * wszLocale1, const wchar_t * wszLocale2 )
{
    wchar_t f,l;

    do
    {
        f = __ascii_towlower(*wszLocale1);
        l = __ascii_towlower(*wszLocale2);
        wszLocale1++;
        wszLocale2++;
    }
    while ( (f) && (f == l) );

    return (INT)(f - l);
}

BOOL FNORMIValidLocaleName( PCWSTR const wszLocale )
{
    for( ULONG ich = 0; wszLocale[ich]; ich++ )
    {
        WCHAR wch = wszLocale[ich];

        //  Only valid characters according to NLS team / RFC are A-Z a-z 0-9 '-' '_'
        if ( wch < L'-' )
        {
            return fFalse;
        }
        if ( wch > L'-' && wch < L'0' )
        {
            return fFalse;
        }
        if ( wch > L'9' && wch < L'A' )
        {
            return fFalse;
        }
        if ( wch > L'Z' && wch < L'_' )
        {
            return fFalse;
        }
        if ( wch > L'_' && wch < L'a' )
        {
            return fFalse;
        }
        if ( wch > L'z' )
        {
            return fFalse;
        }
    }
    return fTrue;
}


INT NORMCompareLocaleName( PCWSTR const wszLocale1, PCWSTR const wszLocale2 )
{
    Assert( FNORMIValidLocaleName( wszLocale1 ) );
    Assert( FNORMIValidLocaleName( wszLocale2 ) );
    return __ese_wcsicmp( wszLocale1, wszLocale2 );
}

BOOL FNORMEqualsLocaleName( PCWSTR const wszLocale1, PCWSTR const wszLocale2 )
{
    return ( 0 == NORMCompareLocaleName( wszLocale1, wszLocale2 ) );
}

ERR ErrNORMLocaleToLcid(
    __in_z PCWSTR wszLocale,
    _Out_ LCID *plcid )
{
    ERR err = JET_errSuccess;
    DWORD errSystem = S_OK;

    // Note that there are certain locales (usually newer locales) that
    // return 0x10000 (LOCALE_CUSTOM_UNSPECIFIED).
    const LCID lcid = LocaleNameToLCID( wszLocale, 0 );

    if ( lcid == 0 )
    {
        errSystem = GetLastError();
        Error( ErrERRCheck( JET_errInvalidLanguageId ) );
    }

    *plcid = lcid;

HandleError:
    
    if ( err < JET_errSuccess )
    {
        NORMReportInvalidLocaleName( NULL, wszLocale, err, errSystem );
    }
    
    return err;
}

