// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _OS_NORM_HXX_INCLUDED
#error NORM.HXX already included
#endif
#define _OS_NORM_HXX_INCLUDED


const SORTID        g_sortIDNull                  = { 0, 0, 0, 0 };


const USHORT        usUniCodePage               = 1200;     
const USHORT        usEnglishCodePage           = 1252;     


const LANGID LangidFromLcid( const LCID lcid );

const WORD          countryDefault              = 1;
extern const LCID   lcidNone;
extern const LCID   lcidInvariant;
extern const PWSTR  wszLocaleNameDefault;
extern const PWSTR  wszLocaleNameNone;
extern const DWORD  dwLCMapFlagsDefaultOBSOLETE;
extern const DWORD  dwLCMapFlagsDefault;


ERR ErrNORMCheckLocaleName( __in INST * const pinst, __in_z const PCWSTR wszLocaleName );
ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _In_ const DWORD dwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization );
ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _Inout_ DWORD * const pdwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization );

BOOL FNORMGetNLSExIsSupported();


ERR ErrNORMGetSortVersion( __in_z PCWSTR wszLocaleName, __out QWORD * const pqwVersion, __out_opt SORTID * const psortID, __in const BOOL fErrorOnInvalidId = fTrue );

INLINE QWORD QwSortVersionFromNLSDefined( const DWORD dwNLSVersion, const DWORD dwDefinedVersion )
{
    const QWORD qwNLSVersion        = dwNLSVersion;
    const QWORD qwDefinedVersion    = dwDefinedVersion;
    const QWORD qwVersion           = ( qwNLSVersion << 32 ) + qwDefinedVersion;
    Assert( ( qwVersion & 0xFFFFFFFF ) == dwDefinedVersion );
    Assert( ( ( qwVersion >> 32 ) & 0xFFFFFFFF ) == dwNLSVersion );
    return qwVersion;
}

INLINE VOID QwSortVersionToNLSDefined(
    _In_ const QWORD qwVersion,
    _Out_ DWORD* pdwNLSVersion,
    _Out_ DWORD* pdwDefinedVersion )
{
    *pdwDefinedVersion = ( qwVersion & 0xFFFFFFFFul );
    *pdwNLSVersion = ( ( qwVersion >> 32 ) & 0xFFFFFFFFul );
    Assert( qwVersion == QwSortVersionFromNLSDefined( *pdwNLSVersion, *pdwDefinedVersion ) );
}

#ifdef DEBUG
VOID AssertNORMConstants();
#else
#define AssertNORMConstants()
#endif

#define NORM_LOCALE_NAME_MAX_LENGTH     85

struct NORM_LOCALE_VER
{
    SORTID m_sortidCustomSortVersion;


    DWORD m_dwNormalizationFlags;

    DWORD m_dwNlsVersion;

    DWORD m_dwDefinedNlsVersion;


    _Field_z_ WCHAR     m_wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];

};

ERR ErrNORMCheckLocaleVersion( _In_ const NORM_LOCALE_VER* pnlv );

ERR ErrNORMMapString(
    _In_ const NORM_LOCALE_VER*     pnlv,
    _In_reads_(cbColumn) BYTE *     pbColumn,
    _In_ INT                        cbColumn,
    _Out_writes_to_(cbMax, *pcbSeg) BYTE * const    rgbSeg,
    _In_ const INT                  cbMax,
    _Out_ INT * const               pcbSeg );

ERR ErrNORMLcidToLocale(
    __in const LCID lcid,
    __out_ecount( cchLocale ) PWSTR wszLocale,
    __in ULONG cchLocale );

ERR ErrNORMLocaleToLcid(
    __in_z PCWSTR wszLocale,
    __out LCID *plcid );

INT NORMCompareLocaleName( PCWSTR const wszLocale1, PCWSTR const wszLocale2 );

BOOL FNORMEqualsLocaleName( PCWSTR const wszLocale1, PCWSTR const wszLocale2 );

BOOL FNORMNLSVersionEquals( QWORD qwVersionCreated, QWORD qwVersionCurrent );

INLINE BOOL FNORMLocaleNameNone( PCWSTR const wszLocale )
{
    return FNORMEqualsLocaleName( wszLocale, wszLocaleNameNone );
}

INLINE BOOL FNORMLocaleNameDefault( PCWSTR const wszLocale )
{
    return FNORMEqualsLocaleName( wszLocale, wszLocaleNameDefault );
}

INLINE BOOL FNORMSortidIsZeroes(
    _In_ const SORTID* const psortid )
{
    static SORTID guidZeroes = { 0 };
    return ( 0 == memcmp( psortid, &guidZeroes, sizeof( guidZeroes ) ) );
}

