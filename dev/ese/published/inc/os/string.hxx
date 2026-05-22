// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OS_STRING_HXX_INCLUDED
#define __OS_STRING_HXX_INCLUDED

// All string functions in this library are guaranteed even under failure leave a NUL terminated 
// string with the singular exception of being passed a buffer that has zero length.

#include <specstrings.h>

#undef STRSAFE_NO_DEPRECATE

#include <stddef.h>

//
//  get the length of the string in characters.
//  Note the unusual usage.  Most of our string handling uses count of bytes.
//  Historically, however, string length is returned as count of characters.
LONG LOSStrLengthA(
    _In_ PCSTR const sz,
    _In_ ULONG       cchMax = ulMax );
LONG LOSStrLengthW(
    _In_ PCWSTR const wsz,
    _In_ ULONG        cchMax = ulMax );
LONG LOSStrLengthUnalignedW(
    _In_ const UnalignedLittleEndian< WCHAR > * wsz,
    _In_ ULONG                                  cchMax = ulMax );
LONG LOSStrLengthMW(
    _In_ PCWSTR const wsz );

//
//  copy a string up to a maximum byte count.
ERR ErrOSStrCbCopyA(
    _In_ PSTR   szDst,
    _In_ SIZE_T cbDst,
    _In_ PCSTR  szSrc );
ERR ErrOSStrCbCopyW(
    _In_ PWSTR szDst,
    _In_ SIZE_T cbDst,
    _In_ PCWSTR szSrc ); 
#define OSStrCbCopyA( szDst, cbDst, szSrc )                             \
    { if(ErrOSStrCbCopyA( szDst, cbDst, szSrc )){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbCopyW( wszDst, cbDst, wszSrc )                           \
    { if(ErrOSStrCbCopyW( wszDst, cbDst, wszSrc )){ AssertSz( fFalse, "Success expected"); } }

//  append a string
ERR ErrOSStrCbAppendA(
    _In_ PSTR   szDst,
    _In_ SIZE_T cbDst,
    _In_ PCSTR  szSrc );
ERR ErrOSStrCbAppendW(
    _In_ PWSTR  wszDst,
    _In_ SIZE_T cbDst,
    _In_ PCWSTR wszSrc );
#define OSStrCbAppendA( szDst, cbDst, szSrc )                           \
    { if( ErrOSStrCbAppendA( szDst, cbDst, szSrc ) ){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbAppendW( wszDst, cbDst, wszSrc )                         \
    { if( ErrOSStrCbAppendW( wszDst, cbDst, wszSrc ) ){ AssertSz( fFalse, "Success expected"); } }

//
//  compare the strings (up to the given maximum length).  if the first string
//  is "less than" the second string, -1 is returned.  if the strings are "equal",
//  0 is returned.  if the first string is "greater than" the second string, +1 is returned.
//  Note the unusual usage.  Most of our string handling uses count of bytes.
//  Historically, however, string length is returned as count of characters.
LONG LOSStrCompareA(
    _In_ PCSTR const pszStr1,
    _In_ PCSTR const pszStr2,
    _In_ const ULONG cchMax = -1 );
LONG LOSStrCompareW(
    _In_ PCWSTR const pwszStr1,
    _In_ PCWSTR const pwszStr2,
    _In_ const ULONG cchMax = -1 );

//
//  create a formatted string in a given buffer and a va_list
ERR __cdecl ErrOSStrCbVFormatA(
    _Out_writes_bytes_(cbBuffer) PSTR szBuffer,
    SIZE_T                            cbBuffer,
    _Printf_format_string_ PCSTR      szFormat,
    va_list                           alist );
ERR __cdecl ErrOSStrCbVFormatW(
    _Out_writes_bytes_(cbBuffer) PWSTR szBuffer,
    SIZE_T                             cbBuffer,
    _Printf_format_string_ PCWSTR      szFormat,
    va_list                            alist );
#define OSStrCbVFormatA( szBuffer, cbBuffer, szFormat, alist)           \
    { if ( ErrOSStrCbVFormatA( szBuffer, cbBuffer, szFormat, alist ) ){ AssertSz( fFalse, "Success expected" ); } }
#define OSStrCbVFormatW( szBuffer, cbBuffer, szFormat, alist)           \
    { if ( ErrOSStrCbVFormatW( szBuffer, cbBuffer, szFormat, alist ) ){ AssertSz( fFalse, "Success expected" ); } }

//
//  create a formatted string in a given buffer with a variadac parameter list
ERR __cdecl ErrOSStrCbFormatA(
    _Out_writes_bytes_(cbBuffer) PSTR szBuffer,
    SIZE_T                            cbBuffer,
    _Printf_format_string_ PCSTR      szFormat,
    ...);
ERR __cdecl ErrOSStrCbFormatW (
    _Out_writes_bytes_(cbBuffer) PWSTR szBuffer,
    SIZE_T cbBuffer,
    _Printf_format_string_ PCWSTR szFormat,
    ...);
#define OSStrCbFormatA( szBuffer, cbBuffer, szFormat, ... )             \
    { if ( ErrOSStrCbFormatA( szBuffer, cbBuffer, szFormat, __VA_ARGS__ ) ){ AssertSz( fFalse, "Success expected" ); } } 
#define OSStrCbFormatW( szBuffer, cbBuffer, szFormat, ... )             \
    { if ( ErrOSStrCbFormatW( szBuffer, cbBuffer, szFormat, __VA_ARGS__ ) ){ AssertSz( fFalse, "Success expected" ); } }

//
//  returns a pointer to the next character in the string.  when no more
//  characters are left, the given ptr is returned.
VOID OSStrCharFindA(
    _In_ PCSTR const                       szStr,
    const CHAR                             ch,
    _Outptr_result_maybenull_ PSTR * const pszFound );
VOID OSStrCharFindW(
    _In_ PCWSTR const                       wszStr,
    const WCHAR                             wch,
    _Outptr_result_maybenull_ PWSTR * const pwszFound );


//  find the last occurrence of the given character in the given string and
//  return a pointer to that character.  NULL is returned when the character
//  is not found.
VOID OSStrCharFindReverseA(
    _In_ PCSTR const                       szStr,
    const CHAR                             ch,
    _Outptr_result_maybenull_ PSTR * const pszFound );
VOID OSStrCharFindReverseW(
    _In_ PCWSTR const                       wszStr,
    const WCHAR                             wch,
    _Outptr_result_maybenull_ PWSTR * const pwszFound );


//
//  check for a trailing path-delimeter
BOOL FOSSTRTrailingPathDelimiterA( _In_ PCSTR const pszPath );
BOOL FOSSTRTrailingPathDelimiterW( _In_ PCWSTR const pwszPath );

//
//  convert with a fixed conversion code page (1252 / Windows English) or use a context dependant
//  conversion (CP_ACP).
typedef enum
{
    // Should be used when the same conversion should be used
    // regardless of the OS' locale and settings (e.g.: strings that
    // should only be in ASCII).
    OSSTR_FIXED_CONVERSION = 0,


    // Should be used when the OS locale and setting should be
    // considered (e.g.: customer data).
    OSSTR_CONTEXT_DEPENDENT_CONVERSION = 1,


} OSSTR_CONVERSION;

typedef enum
{
    OSSTR_NOT_LOSSY = 0,
    OSSTR_ALLOW_LOSSY = 1
} OSSTR_LOSSY;

//
//  convert a byte string to a wide-char string
ERR ErrOSSTRAsciiToUnicode(
    _In_ PCSTR const       pszIn,
    _Out_opt_z_cap_post_count_(cwchOut, *pcwchRequired) PWSTR const pwszOut,
    const SIZE_T           cwchOut,
    SIZE_T * const         pcwchRequired = NULL,
    const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION
    );

//
//  convert a wide-char string to a byte string
ERR ErrOSSTRUnicodeToAscii(
    _In_ PCWSTR const      pwszIn,
    _Out_opt_z_cap_post_count_(cchOut, *pcchRequired) PSTR const        pwszOut,
    const SIZE_T           cchOut,
    SIZE_T * const         pcchRequired = NULL,
    const OSSTR_LOSSY      fLossy = OSSTR_NOT_LOSSY,
    const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION
    );

ERR ErrOSSTRAsciiToUnicodeM(
    _In_ PCSTR const       szzMultiIn,
    __out_ecount_z(cchMax) WCHAR * wszNew,
    ULONG                  cchMax,
    SIZE_T * const         pcchActual,
    const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION
    );

ERR ErrOSSTRUnicodeToAsciiM(
    _In_ PCWSTR const      wszzMultiIn,
    __out_ecount_z(cchMax) CHAR * szNew,
    ULONG                  cchMax,
    SIZE_T * const         pcchActual,
    const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION
    );

#endif  //  __OS_STRING_HXX_INCLUDED

