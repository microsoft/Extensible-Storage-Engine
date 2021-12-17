// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef _OS_NORM_HXX_INCLUDED
#error NORM.HXX already included
#endif
#define _OS_NORM_HXX_INCLUDED

/*  SORTID constants
/**/
const SORTID        g_sortIDNull                  = { 0, 0, 0, 0 };

/*  code page constants
/**/
const USHORT        usUniCodePage               = 1200;     /* code page for Unicode strings */
const USHORT        usEnglishCodePage           = 1252;     /* code page for English */

/*  langid and country defaults
/**/
const LANGID LangidFromLcid( const LCID lcid );

const WORD          countryDefault              = 1;
extern const LCID   lcidNone;
extern const LCID   lcidInvariant;
extern const PWSTR  wszLocaleNameDefault;
extern const PWSTR  wszLocaleNameNone;
extern const DWORD  dwLCMapFlagsDefaultOBSOLETE;
extern const DWORD  dwLCMapFlagsDefault;


BOOL FNORMLCMapFlagsHasUpperCase( DWORD dwMapFlags );

ERR ErrNORMCheckLocaleName( _In_ INST * const pinst, __in_z const PCWSTR wszLocaleName );
ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _In_ const DWORD dwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization );
ERR ErrNORMCheckLCMapFlags( _In_ INST * const pinst, _Inout_ DWORD * const pdwLCMapFlags, _In_ const BOOL fUppercaseTextNormalization );

//  This function is necessary for determining if we can fetch the NLS version.
BOOL FNORMGetNLSExIsSupported();

//  get the sort-order version for a particular LCID
//  the sort-order has two components
//
//  - defined version:  changes when the set of characters that
//                      can be sorted changes
//  - NLS version:      changes when the ordering of the defined
//                      set of characters changes
//

ERR ErrNORMGetSortVersion( __in_z PCWSTR wszLocaleName, _Out_ QWORD * const pqwVersion, __out_opt SORTID * const psortID, _In_ const BOOL fErrorOnInvalidId = fTrue );

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

//  this value must match LOCALE_NAME_MAX_LENGTH. We cannot set it here
//  as it would needlessly drag in dependencies to a lot of headers. There is a
//  C_ASSERT to guarantee these match in norm.cxx.
#define NORM_LOCALE_NAME_MAX_LENGTH     85

struct NORM_LOCALE_VER
{
    // Explicit collation sequence.
    // When a SORTID is specified, the version (m_dwNlsVersion) is also required.
    SORTID m_sortidCustomSortVersion;

// 0x10 bytes

    // The flags for LCMapString (e.g. NORM_IGNOREKANATYPE, NORM_IGNORECASE)
    DWORD m_dwNormalizationFlags;

    // Which version to use for sorting. Optional, but if it's specified, you should also
    // specify the sort GUID (Win8+).
    DWORD m_dwNlsVersion;

    // Which version to use for sorting. Used in Vista/Win7, but ignored in Win8+ (according to
    // the NLS team). Interestingly it's still not zero on Win8+.
    DWORD m_dwDefinedNlsVersion;

// 0x1c bytes

    // The locale name. e.g. en-US, pt-BR, en-CA. de-DE_phonebook.
    _Field_z_ WCHAR     m_wszLocaleName[NORM_LOCALE_NAME_MAX_LENGTH];   // Locale name

// 0xaa bytes
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
    _In_ const LCID lcid,
    __out_ecount( cchLocale ) PWSTR wszLocale,
    _In_ ULONG cchLocale );

ERR ErrNORMLocaleToLcid(
    __in_z PCWSTR wszLocale,
    _Out_ LCID *plcid );

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

