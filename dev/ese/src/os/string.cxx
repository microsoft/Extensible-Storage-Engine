// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include <strsafe.h>
#pragma prefast(pop)

ERR ErrFromStrsafeHr ( HRESULT hr)
{
    ERR err;
    
    switch ( hr )
    {
    case SEC_E_OK:
        err = JET_errSuccess;
        break;
        
    case STRSAFE_E_INSUFFICIENT_BUFFER:
        err = ErrERRCheck( JET_errBufferTooSmall );
        break;
        
    case STRSAFE_E_INVALID_PARAMETER:
        err = ErrERRCheck( JET_errInvalidParameter );
        break;
        
    default:
        err = ErrERRCheck( JET_errInternalError );
        break;
    }
    
    CallSx( err, JET_errBufferTooSmall );   //  this is the only really expected error
    return(err);
}


// For the LOSStrLength* functions:
// Get the length of the string in count of characters.
//
// * Note the unusual usage.  Most of our string handling uses count of bytes.
//   Historically, however, string length is returned as count of characters.
//
// * Because the length is returned as count of characters, the input parameter
//   cchMax is also in count of characters.
//
// * If caller does not supply a value for the parameter cchMax, a default is
//   used (see string.hxx).  The default indicates we expect the string to be
//   NULL terminated in a reasonable number of characters.  In practice, reasonable
//   means fewer characters than the max that StringCchLengthFoo can handle.
//
// * The default can not be the token STRSAFE_MAX_CCH, since that token is defined
//   in a Windows specific header file and cannot be referenced in our OS abstraction
//   headers.  The value ulMax is used.
//
// * If the value of the parameter cchMax is greater than STRSAFE_MAX_CCH, STRSAFE_MAX_CCH
//   is used.  We only expect that to happen if the value is ulMax.
//
// * Obviously, ulMax must be greater than STRSAFE_MAX_CCH.

static_assert( ulMax > STRSAFE_MAX_CCH );

// Note the unusual usage.  Most of our string handling uses count of bytes.
// Historically, however, string length is returned as count of characters.
LONG LOSStrLengthA(
    _In_ PCSTR const sz,
    _In_ ULONG cchMax )
{
    SIZE_T cchLength;
    SIZE_T cchMaxUsed;
    HRESULT hr;
    
    // StringCchLengthA returns an error on a NULL pointer.
    if ( NULL == sz )
    {
        return 0;
    }

    if ( cchMax > STRSAFE_MAX_CCH )
    {
        Expected( cchMax == ulMax );
        cchMaxUsed = STRSAFE_MAX_CCH;
    }
    else
    {
        cchMaxUsed = cchMax;
    }

    hr = StringCchLengthA( sz, cchMaxUsed, &cchLength );
    // We never expect this to fail.
    Assert( JET_errSuccess == ErrFromStrsafeHr( hr ) );

    Assert( cchLength <= lMax );
    
    return (LONG)cchLength;
}

// Note the unusual usage.  Most of our string handling uses count of bytes.
// Historically, however, string length is returned as count of characters.
LONG LOSStrLengthW(
    _In_ PCWSTR const wsz,
    _In_ ULONG cchMax )
{
    SIZE_T cchLength;
    SIZE_T cchMaxUsed;
    HRESULT hr;
    
    // StringCchLengthW returns an error on a NULL pointer.
    if ( NULL == wsz )
    {
        return 0;
    }

    if ( cchMax > STRSAFE_MAX_CCH )
    {
        Expected( cchMax == ulMax );
        cchMaxUsed = STRSAFE_MAX_CCH;
    }
    else
    {
        cchMaxUsed = cchMax;
    }
    
    hr = StringCchLengthW( wsz, cchMaxUsed, &cchLength );
    // We never expect this to fail.
    Assert( !ErrFromStrsafeHr( hr ) );

    Assert( cchLength <= cchMax );
    
    return (LONG)cchLength;
}

// Note the unusual usage.  Most of our string handling uses count of bytes.
// Historically, however, string length is returned as count of characters.
LONG LOSStrLengthUnalignedW(
    _In_ const UnalignedLittleEndian< WCHAR > * wsz,
    _In_ ULONG cchMax )
{
    SIZE_T cchLength;
    SIZE_T cchMaxUsed;
    HRESULT hr;

    if ( NULL == wsz )
    {
        return 0;
    }

    if ( cchMax > STRSAFE_MAX_CCH )
    {
        Expected( cchMax == ulMax );
        cchMaxUsed = STRSAFE_MAX_CCH;
    }
    else
    {
        cchMaxUsed = cchMax;
    }
    
    hr = UnalignedStringCchLengthW( ( PCWSTR )wsz, cchMaxUsed, &cchLength );
    // We never expect this to fail.
    Assert( !ErrFromStrsafeHr( hr ) );

    Assert( cchLength <= cchMax );
    
    return (LONG)cchLength;
}

// Note the unusual usage.  Most of our string handling uses count of bytes.
// Historically, however, string length is returned as count of characters.
LONG LOSStrLengthMW(
    _In_ PCWSTR const wsz )
{
    LONG        cchCurrent  = 0;
    PCWSTR      wszCurrent  = wsz;

    if ( NULL == wsz )
    {
        return 0;
    }

    while ( wszCurrent[ cchCurrent ] != L'\0' )
    {
        cchCurrent += LOSStrLengthW( &( wszCurrent[ cchCurrent ] ) ) + 1;
    }

    return cchCurrent;
}


//  Compare the strings (up to the given maximum length).  Does ordinal, not lexical compare.
//  That means byte for byte equality.  If the first string is "less than" the second string, -1
//  is returned.  If the strings are "equal", 0 is returned.  If the first string is "greater than"
//  the second string, +1 is returned.
//
//  Note the unusual usage.  Most of our string handling uses count of bytes.
//  Historically, however, string compare is limited by count of characters.
LONG LOSStrCompareA(
    _In_ PCSTR const szStr1,
    _In_ PCSTR const szStr2,
    _In_ const ULONG cchMax )
{
    LONG lCmp;
    PCSTR szStrUsed1;
    PCSTR szStrUsed2;
    
    if ( 0 == cchMax )
    {
        // Why are you doing this?
        return 0;
    }

    // We treat NULL pointers as 0 length strings.
    if ( NULL == szStr1 )
    {
        szStrUsed1 = "";
    }
    else
    {
        szStrUsed1 = szStr1;
    }

    if ( NULL == szStr2 )
    {
        szStrUsed2 = "";
    }
    else
    {
        szStrUsed2 = szStr2;
    }
    
#if 0
    if ( fIgnoreCase )
    {
        if ( cchMax == -1 )
        {
            lCmp = _stricmp( szStrUsed1, szStrUsed2 );
        }
        else
        {
            lCmp = _strnicmp( szStrUsed1, szStrUsed2, cchMax );
        }
    }
    else
#endif
    {
        if ( cchMax == -1 )
        {
            lCmp = strcmp( szStrUsed1, szStrUsed2 );
        }
        else
        {
            lCmp = strncmp( szStrUsed1, szStrUsed2, cchMax );
        }
    }

    return lCmp;
}


//  Compare the strings (up to the given maximum length).  Does ordinal, not lexical compare.
//  That means byte for byte equality.  If the first string is "less than" the second string, -1
//  is returned.  If the strings are "equal", 0 is returned.  If the first string is "greater than"
//  the second string, +1 is returned.
//
//  Note the unusual usage.  Most of our string handling uses count of bytes.
//  Historically, however, string compare is limited by count of characters.
LONG LOSStrCompareW(
    _In_ PCWSTR const wszStr1,
    _In_ PCWSTR const wszStr2,
    _In_ const ULONG  cchMax )
{
    LONG lCmp;
    PCWSTR wszStrUsed1;
    PCWSTR wszStrUsed2;
    
    if ( 0 == cchMax )
    {
        // Why are you doing this?
        return 0;
    }

    // We treat NULL pointers as 0 length strings.
    if ( NULL == wszStr1 )
    {
        wszStrUsed1 = L"";
    }
    else
    {
        wszStrUsed1 = wszStr1;
    }

    if ( NULL == wszStr2 )
    {
        wszStrUsed2 = L"";
    }
    else
    {
        wszStrUsed2 = wszStr2;
    }

#if 0
    if ( fIgnoreCase )
    {
        if ( cchMax == -1 )
        {
            lCmp = _wcsicmp( wszStrUsed1, wszStrUsed2 );
        }
        else
        {
            lCmp = _wcsnicmp( wszStrUsed1, wszStrUsed2, cchMax );
        }
    }
    else
#endif
    {
        if ( cchMax == -1 )
        {
            lCmp = wcscmp( wszStrUsed1, wszStrUsed2 );
        }
        else
        {
            lCmp = wcsncmp( wszStrUsed1, wszStrUsed2, cchMax );
        }
    }

    return lCmp;
}

ERR ErrOSStrCbCopyA(
    _In_ PSTR   szDst,
    _In_ SIZE_T cbDst,
    _In_ PCSTR  szSrc )
{
    return ErrFromStrsafeHr( StringCbCopyA( szDst, cbDst, szSrc ) );
}

ERR ErrOSStrCbCopyW(
    _In_ PWSTR  wszDst,
    _In_ SIZE_T cbDst,
    _In_ PCWSTR wszSrc )
{
    return ErrFromStrsafeHr( StringCbCopyW( wszDst, cbDst, wszSrc ) );
}

ERR ErrOSStrCbAppendA(
    _In_ PSTR   szDst,
    _In_ SIZE_T cbDst,
    _In_ PCSTR  szSrc )
{
    return ErrFromStrsafeHr( StringCbCatA( szDst, cbDst, szSrc ) );
}

ERR ErrOSStrCbAppendW(
    _In_ PWSTR  wszDst,
    _In_ SIZE_T cbDst,
    _In_ PCWSTR wszSrc )
{
    return ErrFromStrsafeHr( StringCbCatW( wszDst, cbDst, wszSrc ) );
}

//  create a formatted string in a given buffer
ERR __cdecl ErrOSStrCbVFormatA (
    _Out_writes_bytes_(cbBuffer) PSTR szBuffer,
    SIZE_T                            cbBuffer,
    __format_string PCSTR             szFormat,
    va_list                           alist )
{
    HRESULT hr = StringCbVPrintfA( szBuffer, cbBuffer, szFormat, alist );
    return ErrFromStrsafeHr( hr );
}

//  create a formatted string in a given buffer
ERR __cdecl ErrOSStrCbVFormatW (
    _Out_writes_bytes_(cbBuffer) PWSTR szBuffer,
    SIZE_T                             cbBuffer,
    __format_string PCWSTR             szFormat,
    va_list                            alist )
{
    HRESULT hr = StringCbVPrintfW( szBuffer, cbBuffer, szFormat, alist );
    return ErrFromStrsafeHr( hr );
}

//  create a formatted string in a given buffer
ERR __cdecl ErrOSStrCbFormatA (
    _Out_writes_bytes_(cbBuffer) PSTR szBuffer,
    SIZE_T                            cbBuffer,
    __format_string PCSTR             szFormat,
    ...)
{
    va_list alist;
    va_start( alist, szFormat );
    HRESULT hr = StringCbVPrintf( szBuffer, cbBuffer, szFormat, alist );
    va_end( alist );
    return ErrFromStrsafeHr( hr );
}

//  create a formatted string in a given buffer
ERR __cdecl ErrOSStrCbFormatW (
    _Out_writes_bytes_(cbBuffer) PWSTR szBuffer,
    SIZE_T                             cbBuffer,
    __format_string PCWSTR             szFormat,
    ...)
{
    va_list alist;
    va_start( alist, szFormat );
    HRESULT hr = StringCbVPrintfW( szBuffer, cbBuffer, szFormat, alist );
    va_end( alist );
    return  ErrFromStrsafeHr( hr );
}

//  find the first occurrence of the given character in the given string and
//  return a pointer to that character.  NULL is returned when the character
//  is not found.

VOID OSStrCharFindA(
    _In_ PCSTR const                       szStr,
    const CHAR                             ch,
    _Outptr_result_maybenull_ PSTR * const pszFound )
{
    const CHAR* const szFound = szStr;
    if ( szFound )
    {
        *pszFound = (CHAR *)strchr( szStr, ch );
    }
    else
    {
        *pszFound = NULL;
    }
}

VOID OSStrCharFindW(
    _In_ PCWSTR const                       wszStr,
    const WCHAR                             wch,
    _Outptr_result_maybenull_ PWSTR * const pwszFound )
{
    const WCHAR *wszFound = wszStr;
    if ( wszFound )
    {
        while ( L'\0' != *wszFound && wch != *wszFound )
        {
            wszFound++;
        }
        *pwszFound = const_cast< WCHAR *const >( wch == *wszFound ? wszFound : NULL );
    }
    else
    {
        *pwszFound = NULL;
    }
}


//  find the last occurrence of the given character in the given string and
//  return a pointer to that character.  NULL is returned when the character
//  is not found.

VOID OSStrCharFindReverseA(
    _In_ PCSTR const                       szStr,
    const CHAR                             ch,
    _Outptr_result_maybenull_ PSTR * const pszFound )
{
    Assert( '\0' != ch );
    const CHAR* const szFound = szStr;
    if ( szFound )
    {
        *pszFound = (CHAR *)strrchr( szStr, ch );
    }
    else
    {
        *pszFound = NULL;
    }
}

VOID OSStrCharFindReverseW(
    _In_ PCWSTR const                       wszStr,
    const WCHAR                             wch,
    _Outptr_result_maybenull_ PWSTR * const pwszFound )
{
    ULONG   ich;
    ULONG   cch;

    Assert( L'\0' != wch );

    *pwszFound = NULL;

    cch = LOSStrLengthW( wszStr );
    ich = cch;

    while ( ich-- > 0 )
    {
        if ( wch == wszStr[ich] )
        {
            *pwszFound = const_cast< WCHAR* const >( wszStr + ich );
            return;
        }
    }
}


//  check for a trailing path-delimeter

BOOL FOSSTRTrailingPathDelimiterA(
    _In_ PCSTR const pszPath )
{
    const DWORD cchPath = ( NULL == pszPath ) ? 0 : strlen( pszPath );

    if ( cchPath > 0 )
    {
        return BOOL( '\\' == pszPath[cchPath - 1] || '/' == pszPath[cchPath - 1] );
    }
    return fFalse;
}

BOOL FOSSTRTrailingPathDelimiterW(
    _In_ PCWSTR const pwszPath )
{
    const DWORD cchPath = LOSStrLengthW( pwszPath );

    if ( cchPath > 0 )
    {
        return BOOL( L'\\' == pwszPath[cchPath - 1] || L'/' == pwszPath[cchPath - 1] );
    }
    return fFalse;
}

INLINE LOCAL UINT UlCodePageFromOsstrConversion(
    const OSSTR_CONVERSION osstrConversion )
{
    switch( osstrConversion )
    {
        case OSSTR_FIXED_CONVERSION:
            return usEnglishCodePage;

        case OSSTR_CONTEXT_DEPENDENT_CONVERSION:
            return CP_ACP;

        default:
            AssertSz( fFalse, "Unexpected value for OSSTR_CONVERSION: %d", osstrConversion );
            return CP_ACP;
    }
}

//  convert a byte string to a wide-char string
ERR ErrOSSTRAsciiToUnicode(
    _In_ PCSTR const        pszIn,
    _Out_opt_z_cap_post_count_(cwchOut, *pcwchRequired) PWSTR const     pwszOut,
    const SIZE_T            cwchOut,       //  pass in 0 to only return output buffer size in
                                           // pcwchRequired, JET_errBufferTooSmall will be returned.
    SIZE_T * const          pcwchRequired,
    const OSSTR_CONVERSION  osstrConversion )
{

    //  Make sure out params are consistent ...
    Assert( ( pwszOut != NULL && cwchOut != 0 ) ||
            ( pwszOut == NULL && cwchOut == 0 ) );

    if ( NULL != pcwchRequired )
        *pcwchRequired = 0;

    //  try the conversion

    const SIZE_T cwchActual = MultiByteToWideChar(
        UlCodePageFromOsstrConversion( osstrConversion ),
        MB_ERR_INVALID_CHARS,
        pszIn,
        -1,
        pwszOut,
        (INT)cwchOut );
    if ( NULL != pcwchRequired )
        *pcwchRequired = cwchActual;

    if ( 0 != cwchActual )
    {
        if ( 0 == cwchOut )
        {
            Assert( pcwchRequired != NULL ); // there would be no point to pass cwchOut = 0, and not ask for the size ...
            return ErrERRCheck( JET_errBufferTooSmall ); // obviously the buffer is too small
        }

        Assert( cwchActual <= cwchOut );
        Assert( L'\0' == pwszOut[cwchActual - 1] );

        //
        //  Success!
        //
        return JET_errSuccess;
    }

    //  handle the error

    const DWORD dwError = GetLastError();

    //  insufficient buffer handled specially ...

    if ( ERROR_INSUFFICIENT_BUFFER == dwError )
    {
        // The assertion here is subtle, we're saying that ... MBTWC() won't
        // fail w/ an ERROR_INSUFFICIENT_BUFFER if we pass cwchOut = 0.  It
        // should've failed w/ like ERROR_NO_UNICODE_TRANSLATION, but not
        // insuff buffer.
        Assert( cwchOut != 0 );

        // ensure we're NUL terminated ...
        pwszOut[cwchOut-1] = L'\0';

        if ( pcwchRequired )
        {
            // note this makes the penalty for not hitting the guessed buffer
            // size 3n, if the caller retries w/ a bigger buffer.
            // note we pay 2n (1n more than necessary) just to fail if the 
            // caller passes pcwchRequired and didn't consume pcwchRequired.
            *pcwchRequired = MultiByteToWideChar(
                UlCodePageFromOsstrConversion( osstrConversion ),
                MB_ERR_INVALID_CHARS,
                pszIn,
                -1,
                NULL,
                0 );
        }
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    //  if we can NULL terminated it we will ...
    if ( cwchOut != 0 )
    {
        pwszOut[0] = L'\0';
    }

    if ( ERROR_INVALID_PARAMETER == dwError )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    else if ( ERROR_NO_UNICODE_TRANSLATION == dwError )
    {
        return ErrERRCheck( JET_errUnicodeTranslationFail );
    }
    else
    {
        //  unexpected error

        WCHAR           szT[128];
        const WCHAR *   rgszT[1]    = { szT };

        OSStrCbFormatW( szT, sizeof( szT ),
                    L"Unexpected Win32 error in ErrOSSTRAsciiToUnicode: %dL (0x%08X)",
                    dwError,
                    dwError );
        Assert( fFalse );
        UtilReportEvent(
                eventError,
                PERFORMANCE_CATEGORY,
                PLAIN_TEXT_ID,
                1,
                rgszT );

        AssertSz( fFalse, "Does this really happen?  Semi-smart people think no." );

        return ErrERRCheck( JET_errUnicodeTranslationFail );
    }

}

//  convert a wide-char string to a byte string

ERR ErrOSSTRUnicodeToAscii(
    _In_ PCWSTR const       pwszIn,
    _Out_opt_z_cap_post_count_(cchOut, *pcchRequired) PSTR const                pszOut,
    const SIZE_T            cchOut,          //  pass in 0 to only return output buffer
                                             // size in pcchRequired, JET_errBufferTooSmall will be returned.
    SIZE_T * const          pcchRequired,
    const OSSTR_LOSSY       fLossy,          // CAUTION: setting this will allow return JET_errSuccess
                                             // if chars were translated to '?'
    const OSSTR_CONVERSION  osstrConversion )
{

    Assert( ( pszOut != NULL && cchOut != 0 ) ||
            ( pszOut == NULL && cchOut == 0 ) );

    if ( NULL != pcchRequired )
        *pcchRequired  = 0;

    //  try the conversion
    BOOL    fUsedDefaultChar = fTrue; // presume badly behaved API ...

    const SIZE_T cchActual = WideCharToMultiByte(
        UlCodePageFromOsstrConversion( osstrConversion ),
        0,
        pwszIn,
        -1,
        pszOut,
        (INT)cchOut,
        NULL,
        &fUsedDefaultChar );
    if ( NULL != pcchRequired )
        *pcchRequired = cchActual;

    if ( 0 != cchActual )
    {
        if ( 0 == cchOut )
        {
            Assert( pcchRequired != NULL ); // there would be no point to pass cwchOut = 0, and not ask for the size ...
            return ErrERRCheck( JET_errBufferTooSmall ); // obviously the buffer is too small
        }

        Assert( cchActual <= cchOut );
        Assert( '\0' == pszOut[cchActual - 1] );

        // if there are non-ASCII chars, we should make sure
        // we are not defaulting them to something (like ?)
        //
        if ( fUsedDefaultChar && ( fLossy != OSSTR_ALLOW_LOSSY ) )
        {
            return ErrERRCheck( JET_errUnicodeTranslationFail );
        }

        //
        //  Success!
        //
        return JET_errSuccess;
    }

    //  handle the error

    const DWORD dwError = GetLastError();

    //  insufficient buffer handled specially ...

    if ( ERROR_INSUFFICIENT_BUFFER == dwError )
    {
        // The assertion here is subtle, we're saying that ... MBTWC() won't
        // fail w/ an ERROR_INSUFFICIENT_BUFFER if we pass cwchOut = 0.  It
        // should've failed w/ like ERROR_NO_UNICODE_TRANSLATION, but not
        // insuff buffer.
        Assert( cchOut != 0 );

        // ensure we're NUL terminated ...
        pszOut[cchOut-1] = '\0';

        if ( pcchRequired )
        {
            // note this makes the penalty for not hitting the guessed buffer
            // size 3n, if the caller retries w/ a bigger buffer.
            // note we pay 2n (1n more than necessary) just to fail if the 
            // caller passes pcchRequired and didn't consume pcchRequired.
            //
#pragma warning(suppress: 38021)
            *pcchRequired = WideCharToMultiByte(
                CP_ACP,
                0,
                pwszIn,
                -1,
                NULL,
                0,
                NULL,
                &fUsedDefaultChar );
        }
        return ErrERRCheck( JET_errBufferTooSmall );
    }

    //  if we can NULL terminated it we will ...
    if ( cchOut != 0 )
    {
        pszOut[0] = L'\0';
    }

    if ( ERROR_INVALID_PARAMETER == dwError )
    {
        return ErrERRCheck( JET_errInvalidParameter );
    }
    else
    {
        //  unexpected error

        WCHAR           szT[128];
        const WCHAR *   rgszT[1]    = { szT };

        OSStrCbFormatW( szT, sizeof( szT ),
                    L"Unexpected Win32 error in ErrOSSTRUnicodeToAscii: %dL (0x%08X)",
                    dwError,
                    dwError );
        Assert( fFalse );
        UtilReportEvent(
                eventError,
                PERFORMANCE_CATEGORY,
                PLAIN_TEXT_ID,
                1,
                rgszT );

        return ErrERRCheck( JET_errUnicodeTranslationFail );
    }
}


// this is to convert a multi string (double zero terminated)
// into an existing buffer
// if there is no buffer, we will return the needed size
// if there is a buffer but not enough space, we will return error and NOT the actual size
//
// UNDONE: Exchange prefix continued to complain, to make this right I might need like __success on the return value?
//                          __out_ecount_part_z(cchMax, *pcchActual) PSTR const             pszOut,
ERR ErrOSSTRAsciiToUnicodeM(
    _In_ PCSTR const               szzMultiIn,
    __out_ecount_z(cchMax) WCHAR * wszNew,
    ULONG                          cchMax,
    SIZE_T * const                 pcchActual,
    const OSSTR_CONVERSION         osstrConversion )
{
    ERR             err             = JET_errSuccess;
    const CHAR *    szCurrent       = szzMultiIn;
    SIZE_T          cchActualCurrent = 0;
    SIZE_T          cchMaxCurrent   = cchMax;
    WCHAR *         wszNewCurrent   = wszNew;


    if ( !szzMultiIn )
    {
        if ( pcchActual )
        {
            *pcchActual = 0;
        }
        return JET_errSuccess;
    }

    while( szCurrent[0] != '\0' )
    {
        SIZE_T cchCurrent;

        err = ErrOSSTRAsciiToUnicode(
            szCurrent,
            wszNewCurrent,
            cchMaxCurrent,
            &cchCurrent,
            osstrConversion );

        if ( JET_errBufferTooSmall == err )
        {
            if ( 0 == cchMax )
            {
                Assert( cchMaxCurrent == 0 );
                Assert( wszNewCurrent == NULL );
                err = JET_errSuccess;
            }
        }
        CallR( err );

        szCurrent += LOSStrLengthA( szCurrent );
        if ( cchMaxCurrent > cchCurrent )
        {
            cchMaxCurrent -= cchCurrent;
            wszNewCurrent = wszNewCurrent + cchCurrent;
        }
        else
        {
            cchMaxCurrent = 0;
            wszNewCurrent = NULL;
        }
        cchActualCurrent += cchCurrent;
    }

    // put the final '\0'
    if ( cchMaxCurrent > 0 )
    {
        *wszNewCurrent = L'\0';
        if ( cchMaxCurrent > 1 )
        {
            cchMaxCurrent -= 1;
            wszNewCurrent = wszNewCurrent + 1;
        }
        else
        {
            cchMaxCurrent = 0;
            wszNewCurrent = NULL;
        }
    }
    cchActualCurrent++;



    if ( pcchActual )
    {
        *pcchActual = cchActualCurrent;
    }

    return err;
}

// this is to convert a multi string (double zero terminated)
// into an existing buffer
// if there is no buffer, we will return the needed size
// if there is a buffer but not enough space, we will return error and NOT the actual size
//
// UNDONE: Exchange prefix continued to complain, to make this right I might need like __success on the return value?
//                          __out_ecount_part_z(cchMax, *pcchActual) PSTR const             pszOut,
ERR ErrOSSTRUnicodeToAsciiM(
    _In_ PCWSTR const             wszzMultiIn,
    __out_ecount_z(cchMax) CHAR * szNew,
    ULONG                         cchMax,
    SIZE_T * const                pcchActual,
    const OSSTR_CONVERSION        osstrConversion )
{
    ERR             err             = JET_errSuccess;
    const WCHAR *   wszCurrent      = wszzMultiIn;
    SIZE_T          cchActualCurrent = 0;
    SIZE_T          cchMaxCurrent   = cchMax;
    CHAR *          szNewCurrent    = szNew;


    if ( !wszzMultiIn )
    {
        if ( pcchActual )
        {
            *pcchActual = 0;
        }
        return JET_errSuccess;
    }

    while( wszCurrent[0] != L'\0' )
    {
        SIZE_T cchCurrent;

        err = ErrOSSTRUnicodeToAscii(
            wszCurrent,
            szNewCurrent,
            cchMaxCurrent * sizeof(CHAR),
            &cchCurrent,
            OSSTR_NOT_LOSSY,
            osstrConversion );

        if ( JET_errBufferTooSmall == err )
        {
            if ( 0 == cchMax )
            {
                Assert( cchMaxCurrent == 0 );
                Assert( szNewCurrent == NULL );
                err = JET_errSuccess;
            }
        }
        CallR( err );

        wszCurrent += ( LOSStrLengthW( wszCurrent ) + 1 );
        if ( cchMaxCurrent > cchCurrent )
        {
            cchMaxCurrent -= cchCurrent;
            szNewCurrent = szNewCurrent + cchCurrent;
        }
        else
        {
            cchMaxCurrent = 0;
            szNewCurrent = NULL;
        }
        cchActualCurrent += cchCurrent;
    }

    // put the final '\0'
    if ( cchMaxCurrent > 0 )
    {
        *szNewCurrent = '\0';
        if ( cchMaxCurrent > 1 )
        {
            cchMaxCurrent -= 1;
            szNewCurrent = szNewCurrent + 1;
        }
        else
        {
            cchMaxCurrent = 0;
            szNewCurrent = NULL;
        }
    }
    cchActualCurrent++;


    if ( pcchActual )
    {
        *pcchActual = cchActualCurrent;
    }

    return err;
}

