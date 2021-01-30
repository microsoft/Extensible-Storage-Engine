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
DWORD               g_errSystemCheckLocale            = S_OK;
ERR                 g_errCheckLocale                  = JET_errSuccess;

VOID AssertNORMConstants()
{
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

    NORMAssertCheckLocaleName( wszLocaleNameDefault );
    Assert( ErrNORMCheckLCMapFlags( NULL, dwLCMapFlagsDefault, fFalse ) == JET_errSuccess );
}
#endif

const LANGID LangidFromLcid( const LCID lcid )
{
    return LANGIDFROMLCID( lcid );
}

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

WINBASEAPI
BOOL
APIENTRY
WaitForDebugEventEx(
    _Out_ LPDEBUG_EVENT lpDebugEvent,
    _In_ DWORD dwMilliseconds
    );

NTOSFuncPD( g_pfnWaitForDebugEventEx, WaitForDebugEventEx );
#endif

LOCAL ERR ErrLoadNORMFunctions()
{
    NTOSFuncStdPreinit( g_pfnGetNLSVersionEx, g_mwszzLocalizationLibs, GetNLSVersionEx, oslfExpectedOnWin6 | oslfRequired );
#ifndef ESENT
    NTOSFuncStdPreinit( g_pfnWaitForDebugEventEx, g_mwszzLocalizationLibs, WaitForDebugEventEx, oslfExpectedOnWin10 );
#endif

    return JET_errSuccess;
}


LOCAL VOID UnloadNORMFunctions()
{
}


LOCAL_BROKEN BOOL WINAPI GetNLSNotSupported(
    IN  NLS_FUNCTION     Function,
    IN  LCID             Locale,
    OUT LPNLSVERSIONINFO lpVersionInformation )
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

LOCAL_BROKEN BOOL WINAPI GetNLSExNotSupported(
    IN  NLS_FUNCTION        Function,
    IN  LPCWSTR             LocaleName,
    OUT LPNLSVERSIONINFOEX  lpVersionInformation )
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

ERR ErrNORMCheckLocaleName( __in INST * const pinst, __in_z PCWSTR const wszLocaleName )
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
                Error( ErrERRCheck( JET_errInvalidLanguageId ) );
        }
    }

    Call( ErrFaultInjection( 53064 ) );

HandleError:

    if ( err < JET_errSuccess )
    {
#ifdef DEBUG
        g_errSystemCheckLocale = errSystem;
        g_errCheckLocale = err;
#endif

        if ( JET_errUnicodeLanguageValidationFailure != err )
        {
            NORMReportInvalidLocaleName( pinst, wszLocaleName, err, errSystem );
        }
    }

    return err;
}

ERR ErrNORMCheckLocaleVersion(
    _In_ const NORM_LOCALE_VER* pnlv )
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


ERR ErrNORMGetSortVersion( __in_z PCWSTR wszLocaleName, __out QWORD * const pqwVersion, __out_opt SORTID * const psortID, __in const BOOL fErrorOnInvalidId )
{
    ERR err = JET_errSuccess;
    DWORD errSystem = 0;
    *pqwVersion = 0;

#ifndef RTM
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
        ULONG ulTestLcid = (ULONG)UlConfigOverrideInjection( 51415, fFalse );
        if ( ulTestLcid == fTrue ||
             ( ulTestLcid == lcid && lcid != lcidNone ) )
        {
            nlsversioninfoex.dwNLSVersion += 7;
        }

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


BOOL FNORMGetNLSExIsSupported()
{
    return ( g_pfnGetNLSVersionEx.ErrIsPresent() == JET_errSuccess );
}

BOOL g_fNormMemoryLeakAcceptable = fFalse;
BOOL FNORMIGetNLSExWithVersionInfoIsSupported()
{
#ifndef ESENT
    if ( g_pfnWaitForDebugEventEx.ErrIsPresent() != JET_errSuccess )
    {
        return g_fNormMemoryLeakAcceptable && FNORMGetNLSExIsSupported();
    }
#endif
    
    Expected( FNORMGetNLSExIsSupported() );
    return FNORMGetNLSExIsSupported();
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

    if ( dwNLSVersionHighest == 0x00060102 && dwNLSVersionLowest == 0x00060101 )
    {
        return fTrue;
    }

    if ( dwNLSVersionHighest == 0x0006020e && dwNLSVersionLowest == 0x0006020d )
    {
        return fTrue;
    }

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

    return ErrERRCheck( JET_errInvalidLCMapStringFlags );
}

#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 0x00000008
#endif


ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _In_ const DWORD dwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization )
{
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



ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _Inout_ DWORD * const pdwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization )
{
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
#endif


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


    NLSVERSIONINFO* psortinfoUsed = NULL;
    NLSVERSIONINFOEX sortinfo;
    sortinfo.dwNLSVersionInfoSize = sizeof( sortinfo );
    sortinfo.dwNLSVersion = pnlv->m_dwNlsVersion;
    sortinfo.dwDefinedVersion = pnlv->m_dwDefinedNlsVersion;

    sortinfo.dwEffectiveId = 0;
    sortinfo.guidCustomVersion = pnlv->m_sortidCustomSortVersion;

    if ( FNORMIGetNLSExWithVersionInfoIsSupported() && ( 0 != pnlv->m_dwNlsVersion ) )
    {
        psortinfoUsed = (NLSVERSIONINFO*) &sortinfo;

        Enforce( DwUtilSystemVersionMajor() >= 6 );

        if ( 6 == DwUtilSystemVersionMajor() && DwUtilSystemVersionMinor() <= 1 )
        {
            sortinfo.dwNLSVersionInfoSize = sizeof( NLSVERSIONINFO );
        }
        else
        {
            if ( sortinfo.dwNLSVersion < 0x00060200 )
            {
                sortinfo.dwNLSVersionInfoSize = sizeof( NLSVERSIONINFO );
            }

            Assert( ( 6 == DwUtilSystemVersionMajor() && DwUtilSystemVersionMinor() >= 2 ) ||
                    DwUtilSystemVersionMajor() > 6 );
        }
    }

    Assert( ( pnlv->m_dwNormalizationFlags & LCMAP_SORTKEY ) || ( pnlv->m_dwNormalizationFlags == LCMAP_UPPERCASE ) );
    INT cbSize = 0;
#pragma warning(suppress: 26000)
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
    const size_t        cbColumnAligned = cbColumn + sizeof(wchar_t);
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

    Assert( cbColumn > 0 );

    Assert( 0 == ( cbColumn % sizeof( wchar_t ) ) );



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

    rgbKey      = rgbKeyStack;
    cbKeyMax    = cbKeyStack;

    while ( !( cbKey = CbNORMMapString_(    pnlv,
                                            pbColumn,
                                            cbColumn,
                                            rgbKey,
                                            cbKeyMax ) ) )
    {
        switch ( GetLastError() )
        {
            case ERROR_INVALID_USER_BUFFER:
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_WORKING_SET_QUOTA:
            case ERROR_NO_SYSTEM_RESOURCES:
                Error( ErrERRCheck( JET_errOutOfMemory ) );

            case ERROR_BADDB:
                Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );
            
            default:
                Error( ErrERRCheck( JET_errUnicodeTranslationFail ) );

            case ERROR_INSUFFICIENT_BUFFER:
                break;
        }

        cbKeyMax = CbNORMMapString_(    pnlv,
                                        pbColumn,
                                        cbColumn,
                                        NULL,
                                        0 );
        if ( 0 == cbKeyMax )
        {
            const DWORD dwGle = GetLastError();
            switch ( dwGle )
            {
                case ERROR_INVALID_USER_BUFFER:
                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_WORKING_SET_QUOTA:
                case ERROR_NO_SYSTEM_RESOURCES:
                    Error( ErrERRCheck( JET_errOutOfMemory ) );
            
                case ERROR_BADDB:
                    Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );

                default:
                    AssertSz( ERROR_INSUFFICIENT_BUFFER != dwGle,
                              "Unexpected insufficient buffer when getting buffer size for normalization. Received %d.",
                              dwGle );

                    Error( ErrERRCheck( JET_errUnicodeTranslationFail ) );
            }
        }

        OSMemoryHeapFree( rgbKeyAlloc );
        Alloc( rgbKeyAlloc = (BYTE*)PvOSMemoryHeapAlloc( cbKeyMax ) );
        rgbKey = rgbKeyAlloc;
    }

    NORMAssertCheckLocaleName( pnlv->m_wszLocaleName );

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
    if ( cbColumnAligned > cbColumnStack )
    {
        OSMemoryHeapFree( pbColumnAligned );
    }
    OSMemoryHeapFree( rgbKeyAlloc );
    return err;
}



void OSNormPostterm()
{
    UnloadNORMFunctions();
}


BOOL FOSNormPreinit()
{
    if ( ErrLoadNORMFunctions() )
        return fFalse;

    return fTrue;
}



void OSNormTerm()
{
    g_fUnicodeSupport = fFalse;
}


ERR ErrOSNormInit()
{
    ERR err = JET_errSuccess;
    

    g_fUnicodeSupport = ( 0 != LCMapStringW( LOCALE_NEUTRAL, LCMAP_LOWERCASE, L"\0", 1, NULL, 0 ) );

    if ( !g_fUnicodeSupport &&
        ERROR_BADDB == GetLastError() )
    {
        Error( ErrERRCheck( JET_errUnicodeLanguageValidationFailure ) );
    }

#ifndef ESENT

    (void)g_pfnWaitForDebugEventEx.ErrIsPresent();
#endif

#ifdef DEBUG
    AssertNORMConstants();
#endif

HandleError:
    return err;
}

ERR ErrNORMLcidToLocale(
    __in const LCID lcid,
    __out_ecount( cchLocale ) PWSTR wszLocale,
    __in ULONG cchLocale )
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
    __out LCID *plcid )
{
    ERR err = JET_errSuccess;
    DWORD errSystem = S_OK;

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

