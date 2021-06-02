// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

ERR ErrFromStrsafeHr ( HRESULT hr)
{
    ERR err = (hr == SEC_E_OK) ?
        JET_errSuccess :
        (hr == STRSAFE_E_INSUFFICIENT_BUFFER) ?
            ErrERRCheck(JET_errBufferTooSmall) :
            (hr == STRSAFE_E_INVALID_PARAMETER) ?
                ErrERRCheck(JET_errInvalidParameter) :
                ErrERRCheck(JET_errInternalError);
    CallSx( err, JET_errBufferTooSmall );   //  this is the only really expected error
    return(err);
}


//  get the length of the string

LONG LOSStrLengthA( __in PCSTR const sz )
{
    // According to Windows OACR, strlen cannot handle NULL.
    if ( NULL == sz )
    {
        return 0;
    }

    return strlen( sz );
}
LONG LOSStrLengthW( __in PCWSTR const wsz )
{
    // According to Windows OACR, wcslen cannot handle NULL.
    if ( NULL == wsz )
    {
        return 0;
    }
    
    return wcslen( wsz );
}

LONG LOSStrLengthUnalignedW( __in const UnalignedLittleEndian< WCHAR > * wsz )
{
    LONG                                    cchCurrent  = 0;
    const UnalignedLittleEndian< WCHAR > *  wszCurrent  = wsz;

    while ( wszCurrent[ cchCurrent ] != L'\0' )
    {
        cchCurrent++;
    }

    return cchCurrent;
}

LONG LOSStrLengthMW( __in PCWSTR const wsz )
{
    LONG        cchCurrent  = 0;
    PCWSTR      wszCurrent  = wsz;


    while ( wszCurrent[ cchCurrent ] != L'\0' )
    {
        cchCurrent += LOSStrLengthW( &( wszCurrent[ cchCurrent ] ) ) + 1;
    }

    return cchCurrent;
}


//  compare the strings (up to the given maximum length).  if the first string
//  is "less than" the second string, -1 is returned.  if the strings are "equal",
//  0 is returned.  if the first string is "greater than" the second string, +1 is returned.

LONG LOSStrCompareA( __in PCSTR const szStr1, __in PCSTR const szStr2, __in const ULONG cchMax )
{
    ULONG cch1 = ( NULL == szStr1 ) ? 0 : strlen( szStr1 );
    ULONG cch2 = ( NULL == szStr2 ) ? 0 : strlen( szStr2 );
    ULONG ich;

    //  limit the lengths

    if ( cch1 > cchMax )
    {
        cch1 = cchMax;
    }
    if ( cch2 > cchMax )
    {
        cch2 = cchMax;
    }

    //  compare the lengths

    if ( cch1 < cch2 )
    {
        return -1;
    }
    else if ( cch1 > cch2 )
    {
        return +1;
    }

    //  compare the strings

    ich = 0;
    while ( ich < cch1 )
    {
        if ( szStr1[ich] == szStr2[ich] )
        {
            ich++;
        }
        else if ( szStr1[ich] < szStr2[ich] )
        {
            return -1;
        }
        else
        {
            return +1;
        }
    }

    return 0;
}


LONG LOSStrCompareW( __in PCWSTR const wszStr1, __in PCWSTR const wszStr2, __in const ULONG cchMax )
{
    ULONG cch1 = ( NULL == wszStr1 ) ? 0 : wcslen( wszStr1 );
    ULONG cch2 = ( NULL == wszStr2 ) ? 0 : wcslen( wszStr2 );
    ULONG ich;

    //  limit the lengths

    if ( cch1 > cchMax )
    {
        cch1 = cchMax;
    }
    if ( cch2 > cchMax )
    {
        cch2 = cchMax;
    }

    //  compare the lengths

    if ( cch1 < cch2 )
    {
        return -1;
    }
    else if ( cch1 > cch2 )
    {
        return +1;
    }

    //  compare the strings

    ich = 0;
    while ( ich < cch1 )
    {
        if ( wszStr1[ich] == wszStr2[ich] )
        {
            ich++;
        }
        else if ( wszStr1[ich] < wszStr2[ich] )
        {
            return -1;
        }
        else
        {
            return +1;
        }
    }

    return 0;
}



//  create a formatted string in a given buffer
void __cdecl OSStrCbVFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, va_list alist )
{
    HRESULT hr = StringCbVPrintf( szBuffer, cbBuffer, szFormat, alist );
#ifdef DEBUG
    CallS( ErrFromStrsafeHr( hr ) );
#endif
}

void __cdecl OSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...)
{
    va_list alist;
    va_start( alist, szFormat );
    HRESULT hr = StringCbVPrintf( szBuffer, cbBuffer, szFormat, alist );
#ifdef DEBUG
    CallS( ErrFromStrsafeHr( hr ) );
#endif
    va_end( alist );
}

//  create a formatted string in a given buffer

void __cdecl OSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...)
{
    va_list alist;
    va_start( alist, szFormat );
    HRESULT hr = StringCbVPrintfW( szBuffer, cbBuffer, szFormat, alist );
#ifdef DEBUG
    CallS( ErrFromStrsafeHr( hr ) );
#endif
    va_end( alist );
}

//  create a formatted string in a given buffer

ERR __cdecl ErrOSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...)
{
    va_list alist;
    va_start( alist, szFormat );
    HRESULT hr = StringCbVPrintf( szBuffer, cbBuffer, szFormat, alist );
    va_end( alist );
    return( ErrFromStrsafeHr(hr) );
}

//  create a formatted string in a given buffer

ERR __cdecl ErrOSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...)
{
    va_list alist;
    va_start( alist, szFormat );
    HRESULT hr = StringCbVPrintfW( szBuffer, cbBuffer, szFormat, alist );
    va_end( alist );
    return( ErrFromStrsafeHr(hr) );
}

//  find the first occurrence of the given character in the given string and
//  return a pointer to that character.  NULL is returned when the character
//  is not found.

VOID OSStrCharFindA( _In_ PCSTR const szStr, const char ch, _Outptr_result_maybenull_ PSTR * const pszFound )
{
    const char* const szFound = szStr;
    if ( szFound )
    {
        *pszFound = (char *)strchr( szStr, ch );
    }
    else
    {
        *pszFound = NULL;
    }
}
VOID OSStrCharFindW( _In_ PCWSTR const wszStr, const wchar_t wch, _Outptr_result_maybenull_ PWSTR * const pwszFound )
{
    const wchar_t *wszFound = wszStr;
    if ( wszFound )
    {
        while ( L'\0' != *wszFound && wch != *wszFound )
        {
            wszFound++;
        }
        *pwszFound = const_cast< wchar_t *const >( wch == *wszFound ? wszFound : NULL );
    }
    else
    {
        *pwszFound = NULL;
    }
}


//  find the last occurrence of the given character in the given string and
//  return a pointer to that character.  NULL is returned when the character
//  is not found.

VOID OSStrCharFindReverseA( _In_ PCSTR const szStr, const char ch, _Outptr_result_maybenull_ PSTR * const pszFound )
{
    Assert( '\0' != ch );
    const char* const szFound = szStr;
    if ( szFound )
    {
        *pszFound = (char *)strrchr( szStr, ch );
    }
    else
    {
        *pszFound = NULL;
    }
}
VOID OSStrCharFindReverseW( _In_ PCWSTR const wszStr, const wchar_t wch, _Outptr_result_maybenull_ PWSTR * const pwszFound )
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
            *pwszFound = const_cast< wchar_t* const >( wszStr + ich );
            return;
        }
    }
}


//  check for a trailing path-delimeter

BOOL FOSSTRTrailingPathDelimiterA( __in PCSTR const pszPath )
{
    const DWORD cchPath = ( NULL == pszPath ) ? 0 : strlen( pszPath );

    if ( cchPath > 0 )
    {
        return BOOL( '\\' == pszPath[cchPath - 1] || '/' == pszPath[cchPath - 1] );
    }
    return fFalse;
}
BOOL FOSSTRTrailingPathDelimiterW( __in PCWSTR const pwszPath )
{
    const DWORD cchPath = ( NULL == pwszPath ) ? 0 : wcslen( pwszPath );

    if ( cchPath > 0 )
    {
        return BOOL( L'\\' == pwszPath[cchPath - 1] || L'/' == pwszPath[cchPath - 1] );
    }
    return fFalse;
}

INLINE LOCAL UINT UlCodePageFromOsstrConversion( const OSSTR_CONVERSION osstrConversion )
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

ERR ErrOSSTRAsciiToUnicode( _In_ PCSTR const    pszIn,
                            _Out_opt_z_cap_post_count_(cwchOut, *pcwchRequired) PWSTR const     pwszOut,
                            const size_t            cwchOut,    //  pass in 0 to only return output buffer size in pcwchRequired, JET_errBufferTooSmall will be returned.
                            size_t * const          pcwchRequired,
                            const OSSTR_CONVERSION  osstrConversion )
{

    //  Make sure out params are consistent ...
    Assert( ( pwszOut != NULL && cwchOut != 0 ) ||
            ( pwszOut == NULL && cwchOut == 0 ) );

    if ( NULL != pcwchRequired )
        *pcwchRequired = 0;

    //  try the conversion

    const size_t cwchActual = MultiByteToWideChar(  UlCodePageFromOsstrConversion( osstrConversion ),
                                                MB_ERR_INVALID_CHARS,
                                                pszIn,
                                                -1,
                                                pwszOut,
                                                cwchOut );
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
            *pcwchRequired = MultiByteToWideChar(   UlCodePageFromOsstrConversion( osstrConversion ), MB_ERR_INVALID_CHARS,
                                                pszIn, -1, NULL, 0 );
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

ERR ErrOSSTRUnicodeToAscii( __in PCWSTR const       pwszIn,
                            _Out_opt_z_cap_post_count_(cchOut, *pcchRequired) PSTR const                pszOut,
                            const size_t                cchOut,     //  pass in 0 to only return output buffer size in pcchRequired, JET_errBufferTooSmall will be returned.
                            size_t * const              pcchRequired,
                            const OSSTR_LOSSY       fLossy,     // CAUTION: setting this will allow return JET_errSuccess if chars were translated to ?
                            const OSSTR_CONVERSION  osstrConversion )
{

    Assert( ( pszOut != NULL && cchOut != 0 ) ||
            ( pszOut == NULL && cchOut == 0 ) );

    if ( NULL != pcchRequired )
        *pcchRequired  = 0;

    //  try the conversion
    BOOL    fUsedDefaultChar = fTrue; // presume badly behaved API ...

    const size_t cchActual = WideCharToMultiByte(   UlCodePageFromOsstrConversion( osstrConversion ),
                                                0,
                                                pwszIn,
                                                -1,
                                                pszOut,
                                                cchOut,
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
            *pcchRequired = WideCharToMultiByte(    CP_ACP, 0,
                                                pwszIn, -1, NULL, 0,
                                                NULL, &fUsedDefaultChar );
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


//  convert a WCHAR string to a _TCHAR string

ERR ErrOSSTRUnicodeToTchar( const wchar_t *const    pwszIn,
                            __out_ecount(ctchOut) _TCHAR *const         ptszOut,
                            const INT               ctchOut )
{
#ifdef UNICODE

    //  check the input buffer against the output buffer

    const wchar_t cwchIn = wcslen( pwszIn ) + 1;
    if ( ctchOut < cwchIn )
    {
        return ErrERRCheck( JET_errBufferTooSmall );
    }
    Assert( ctchOut > 0 );

    //  copy the string

    wcsncpy( ptszOut, pwszIn, ctchOut );
    return JET_errSuccess;

#else  //  !UNICODE

    return ErrOSSTRUnicodeToAscii( pwszIn, ptszOut, ctchOut );

#endif  //  UNICODE
}


// this is to convert a multi string (double zero terminated)
// into an existing buffer
// if there is no buffer, we will return the needed size
// if there is a buffer but not enough space, we will return error and NOT the actual size
//
ERR ErrOSSTRAsciiToUnicodeM( __in PCSTR const szzMultiIn,
    // UNDONE: Exchange prefix continued to complain, to make this right I might need like __success on the return value?
    //                          __out_ecount_part_z(cchMax, *pcchActual) PSTR const             pszOut,
    __out_ecount_z(cchMax) WCHAR * wszNew,
    ULONG cchMax,
    size_t * const pcchActual,
    const OSSTR_CONVERSION osstrConversion )
{
    ERR             err             = JET_errSuccess;
    const char *    szCurrent       = szzMultiIn;
    size_t          cchActualCurrent = 0;
    size_t          cchMaxCurrent   = cchMax;
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
        size_t cchCurrent;

        err = ErrOSSTRAsciiToUnicode( szCurrent, wszNewCurrent, cchMaxCurrent, &cchCurrent, osstrConversion );

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
ERR ErrOSSTRUnicodeToAsciiM( __in PCWSTR const wszzMultiIn,
    // UNDONE: Exchange prefix continued to complain, to make this right I might need like __success on the return value?
    //                          __out_ecount_part_z(cchMax, *pcchActual) PSTR const             pszOut,
    __out_ecount_z(cchMax) char * szNew,
    ULONG cchMax,
    size_t * const pcchActual,
    const OSSTR_CONVERSION osstrConversion )
{
    ERR             err             = JET_errSuccess;
    const WCHAR *   wszCurrent      = wszzMultiIn;
    size_t          cchActualCurrent = 0;
    size_t          cchMaxCurrent   = cchMax;
    char *          szNewCurrent    = szNew;


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
        size_t cchCurrent;

        err = ErrOSSTRUnicodeToAscii( wszCurrent, szNewCurrent, cchMaxCurrent * sizeof(char), &cchCurrent, OSSTR_NOT_LOSSY, osstrConversion );

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


