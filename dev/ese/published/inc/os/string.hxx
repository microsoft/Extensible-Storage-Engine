// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OS_STRING_HXX_INCLUDED
#define __OS_STRING_HXX_INCLUDED

// All string functions in this library are guaranteed even under failure leave a NUL terminated 
// string with the singular exception of being passed a buffer that has zero length.

#include <specstrings.h>
#ifndef PSTR
typedef _Null_terminated_ char *  PSTR;    /* ASCII string (char *) null terminated */
#ifndef PCSTR
#endif
typedef _Null_terminated_ const char *  PCSTR;   /* const ASCII string (char *) null terminated */
#ifndef PWSTR
#endif
typedef _Null_terminated_ wchar_t * PWSTR;   /* Unicode string (char *) null terminated */
#ifndef PCWSTR
#endif
typedef _Null_terminated_ const wchar_t * PCWSTR;  /* const Unicode string (char *) null terminated */
#endif

#undef STRSAFE_NO_DEPRECATE

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef _Return_type_success_(return >= 0) LONG HRESULT; // required b/c we define/use the StringXxxXxxx() functions inline ...
#endif // _HRESULT_DEFINED

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include "strsafe.h"
#pragma prefast(pop)

#include <stddef.h>

//  get the length of the string
ERR ErrFromStrsafeHr ( HRESULT hr );

LONG LOSStrLengthA( _In_ PCSTR const sz );
LONG LOSStrLengthW( _In_ PCWSTR const wsz );
LONG LOSStrLengthUnalignedW( _In_ const UnalignedLittleEndian< WCHAR > * wsz );
LONG LOSStrLengthMW( _In_ PCWSTR const wsz );

//  copy a string

#define ErrOSStrCbCopyA( szDst, cbDst, szSrc)       ErrFromStrsafeHr( StringCbCopyA( szDst, cbDst, szSrc) )
#define ErrOSStrCbCopyW( wszDst, cbDst, wszSrc)     ErrFromStrsafeHr( StringCbCopyW( wszDst, cbDst, wszSrc) )
#if DBG
#define OSStrCbCopyA( szDst, cbDst, szSrc )     { if(ErrOSStrCbCopyA( szDst, cbDst, szSrc )){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbCopyW( wszDst, cbDst, wszSrc )       { if(ErrOSStrCbCopyW( wszDst, cbDst, wszSrc )){ AssertSz( fFalse, "Success expected"); } }
#else
#define OSStrCbCopyA( szDst, cbDst, szSrc )     ErrOSStrCbCopyA( szDst, cbDst, szSrc )
#define OSStrCbCopyW( wszDst, cbDst, wszSrc )       ErrOSStrCbCopyW( wszDst, cbDst, wszSrc )
#endif

//  append a string

#define ErrOSStrCbAppendA( szDst, cbDst, szSrc )        ErrFromStrsafeHr( StringCbCatA( szDst, cbDst, szSrc ) )
#define ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )  ErrFromStrsafeHr( StringCbCatW( wszDst, cbDst, wszSrc ) )
#if DBG
#define OSStrCbAppendA( szDst, cbDst, szSrc )       { if(ErrOSStrCbAppendA( szDst, cbDst, szSrc )){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbAppendW( wszDst, cbDst, wszSrc )     { if(ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )){ AssertSz( fFalse, "Success expected"); } }
#else
#define OSStrCbAppendA( szDst, cbDst, szSrc )       ErrOSStrCbAppendA( szDst, cbDst, szSrc )
#define OSStrCbAppendW( wszDst, cbDst, wszSrc )     ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )
#endif

//  compare the strings (up to the given maximum length).  if the first string
//  is "less than" the second string, -1 is returned.  if the strings are "equal",
//  0 is returned.  if the first string is "greater than" the second string, +1 is returned.

LONG LOSStrCompareA( _In_ PCSTR const pszStr1, _In_ PCSTR const pszStr2, _In_ const ULONG cchMax = ~ULONG( 0 ) );
LONG LOSStrCompareW( _In_ PCWSTR const pwszStr1, _In_ PCWSTR const pwszStr2, _In_ const ULONG cchMax = ~ULONG( 0 ) );


//  create a formatted string in a given buffer

void __cdecl OSStrCbVFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, va_list alist );
void __cdecl OSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...);
void __cdecl OSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...);
ERR __cdecl ErrOSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...);
ERR __cdecl ErrOSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...);

//  Formats a GUID in a given given buffer.
//  Note: Does not exist. If it's necessary, please use/refactor WszCATFormatSortID()
// ERR ErrOSStrCbFormatGuid( _Out_writes_bytes_(37*2) PWSTR szBuffer, _In_ const GUID* pguid );

//  returns a pointer to the next character in the string.  when no more
//  characters are left, the given ptr is returned.

VOID OSStrCharFindA( _In_ PCSTR const szStr, const char ch, _Outptr_result_maybenull_ PSTR * const pszFound );
VOID OSStrCharFindW( _In_ PCWSTR const wszStr, const wchar_t wch, _Outptr_result_maybenull_ PWSTR * const pwszFound );


//  find the last occurrence of the given character in the given string and
//  return a pointer to that character.  NULL is returned when the character
//  is not found.

VOID OSStrCharFindReverseA( _In_ PCSTR const szStr, const char ch, _Outptr_result_maybenull_ PSTR * const pszFound );
VOID OSStrCharFindReverseW( _In_ PCWSTR const wszStr, const wchar_t wch, _Outptr_result_maybenull_ PWSTR * const pwszFound );


//  check for a trailing path-delimeter

BOOL FOSSTRTrailingPathDelimiterA( _In_ PCSTR const pszPath );
BOOL FOSSTRTrailingPathDelimiterW( _In_ PCWSTR const pwszPath );

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

//  convert a byte string to a wide-char string

ERR ErrOSSTRAsciiToUnicode( _In_ PCSTR const    pszIn,
                            _Out_opt_z_cap_post_count_(cwchOut, *pcwchRequired) PWSTR const     pwszOut,
                            const size_t            cwchOut,
                            size_t * const          pcwchRequired = NULL,
                            const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );

//  convert a wide-char string to a byte string
typedef enum { OSSTR_NOT_LOSSY = 0, OSSTR_ALLOW_LOSSY = 1 } OSSTR_LOSSY;

ERR ErrOSSTRUnicodeToAscii( _In_ PCWSTR const       pwszIn,
                            _Out_opt_z_cap_post_count_(cchOut, *pcchRequired) PSTR const        pwszOut,
                            const size_t                cchOut,
                            size_t * const              pcchRequired = NULL,
                            const OSSTR_LOSSY       fLossy = OSSTR_NOT_LOSSY,
                            const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );

#ifdef __TCHAR_DEFINED
//  convert a WCHAR string to a _TCHAR string

ERR ErrOSSTRUnicodeToTchar( const wchar_t *const    pwszIn,
                            __out_ecount(ctchOut) _TCHAR *const         ptszOut,
                            const INT               ctchOut,
                            const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );
#endif

//  generic names for UNICODE and non-UNICODE cases

#ifdef UNICODE

#define LOSStrLength                        LOSStrLengthW
#define OSStrAppend                     OSSTRAppendW
#define LOSSTRCompare                   LOSSTRCompareW
// #define OSStrFormat                  OSStrFormatW
//#define OSStrCharFind                 OSStrCharFindW
//#define OSStrCharFindReverse          OSStrCharFindReverseW
#define FOSSTRTrailingPathDelimiter     FOSSTRTrailingPathDelimiterW

#else   //  !UNICODE

#define LOSStrLength                        LOSStrLengthA
#define OSStrAppend                     OSSTRAppendA
#define LOSSTRCompare                   LOSSTRCompareA
//#define OSStrFormat                       OSStrFormatA
//#define OSStrCharFind                 OSStrCharFindA
//#define OSStrCharFindReverse              OSStrCharFindReverseA
#define FOSSTRTrailingPathDelimiter     FOSSTRTrailingPathDelimiterA

#endif  //  UNICODE


ERR ErrOSSTRAsciiToUnicodeM( _In_ PCSTR const szzMultiIn, __out_ecount_z(cchMax) WCHAR * wszNew, ULONG cchMax, size_t * const pcchActual, const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );
ERR ErrOSSTRUnicodeToAsciiM( _In_ PCWSTR const wszzMultiIn, __out_ecount_z(cchMax) char * szNew, ULONG cchMax, size_t * const pcchActual, const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );

#endif  //  __OS_STRING_HXX_INCLUDED

