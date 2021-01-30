// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef __OS_STRING_HXX_INCLUDED
#define __OS_STRING_HXX_INCLUDED


#include <specstrings.h>
#ifndef PSTR
typedef _Null_terminated_ char *  PSTR;    
#ifndef PCSTR
#endif
typedef _Null_terminated_ const char *  PCSTR;   
#ifndef PWSTR
#endif
typedef _Null_terminated_ wchar_t * PWSTR;   
#ifndef PCWSTR
#endif
typedef _Null_terminated_ const wchar_t * PCWSTR;  
#endif

#undef STRSAFE_NO_DEPRECATE

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef _Return_type_success_(return >= 0) LONG HRESULT;
#endif

#pragma prefast(push)
#pragma prefast(disable:28196, "Do not bother us with strsafe, someone else owns that.")
#pragma prefast(disable:28205, "Do not bother us with strsafe, someone else owns that.")
#include "strsafe.h"
#pragma prefast(pop)

#include <stddef.h>

ERR ErrFromStrsafeHr ( HRESULT hr );

LONG LOSStrLengthA( __in PCSTR const sz );
LONG LOSStrLengthW( __in PCWSTR const wsz );
LONG LOSStrLengthUnalignedW( __in const UnalignedLittleEndian< WCHAR > * wsz );
LONG LOSStrLengthMW( __in PCWSTR const wsz );


#define ErrOSStrCbCopyA( szDst, cbDst, szSrc)       ErrFromStrsafeHr( StringCbCopyA( szDst, cbDst, szSrc) )
#define ErrOSStrCbCopyW( wszDst, cbDst, wszSrc)     ErrFromStrsafeHr( StringCbCopyW( wszDst, cbDst, wszSrc) )
#if DBG
#define OSStrCbCopyA( szDst, cbDst, szSrc )     { if(ErrOSStrCbCopyA( szDst, cbDst, szSrc )){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbCopyW( wszDst, cbDst, wszSrc )       { if(ErrOSStrCbCopyW( wszDst, cbDst, wszSrc )){ AssertSz( fFalse, "Success expected"); } }
#else
#define OSStrCbCopyA( szDst, cbDst, szSrc )     ErrOSStrCbCopyA( szDst, cbDst, szSrc )
#define OSStrCbCopyW( wszDst, cbDst, wszSrc )       ErrOSStrCbCopyW( wszDst, cbDst, wszSrc )
#endif


#define ErrOSStrCbAppendA( szDst, cbDst, szSrc )        ErrFromStrsafeHr( StringCbCatA( szDst, cbDst, szSrc ) )
#define ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )  ErrFromStrsafeHr( StringCbCatW( wszDst, cbDst, wszSrc ) )
#if DBG
#define OSStrCbAppendA( szDst, cbDst, szSrc )       { if(ErrOSStrCbAppendA( szDst, cbDst, szSrc )){ AssertSz( fFalse, "Success expected"); } }
#define OSStrCbAppendW( wszDst, cbDst, wszSrc )     { if(ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )){ AssertSz( fFalse, "Success expected"); } }
#else
#define OSStrCbAppendA( szDst, cbDst, szSrc )       ErrOSStrCbAppendA( szDst, cbDst, szSrc )
#define OSStrCbAppendW( wszDst, cbDst, wszSrc )     ErrOSStrCbAppendW( wszDst, cbDst, wszSrc )
#endif


LONG LOSStrCompareA( __in PCSTR const pszStr1, __in PCSTR const pszStr2, __in const ULONG cchMax = ~ULONG( 0 ) );
LONG LOSStrCompareW( __in PCWSTR const pwszStr1, __in PCWSTR const pwszStr2, __in const ULONG cchMax = ~ULONG( 0 ) );



void __cdecl OSStrCbVFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, va_list alist );
void __cdecl OSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...);
void __cdecl OSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...);
ERR __cdecl ErrOSStrCbFormatA ( __out_bcount(cbBuffer) PSTR szBuffer, size_t cbBuffer, __format_string PCSTR szFormat, ...);
ERR __cdecl ErrOSStrCbFormatW ( __out_bcount(cbBuffer) PWSTR szBuffer, size_t cbBuffer, __format_string PCWSTR szFormat, ...);



VOID OSStrCharFindA( _In_ PCSTR const szStr, const char ch, _Outptr_result_maybenull_ PSTR * const pszFound );
VOID OSStrCharFindW( _In_ PCWSTR const wszStr, const wchar_t wch, _Outptr_result_maybenull_ PWSTR * const pwszFound );



VOID OSStrCharFindReverseA( _In_ PCSTR const szStr, const char ch, _Outptr_result_maybenull_ PSTR * const pszFound );
VOID OSStrCharFindReverseW( _In_ PCWSTR const wszStr, const wchar_t wch, _Outptr_result_maybenull_ PWSTR * const pwszFound );



BOOL FOSSTRTrailingPathDelimiterA( __in PCSTR const pszPath );
BOOL FOSSTRTrailingPathDelimiterW( __in PCWSTR const pwszPath );


typedef enum
{
    OSSTR_FIXED_CONVERSION = 0,

    OSSTR_CONTEXT_DEPENDENT_CONVERSION = 1,

} OSSTR_CONVERSION;


ERR ErrOSSTRAsciiToUnicode( _In_ PCSTR const    pszIn,
                            _Out_opt_z_cap_post_count_(cwchOut, *pcwchRequired) PWSTR const     pwszOut,
                            const size_t            cwchOut,
                            size_t * const          pcwchRequired = NULL,
                            const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );

typedef enum { OSSTR_NOT_LOSSY = 0, OSSTR_ALLOW_LOSSY = 1 } OSSTR_LOSSY;

ERR ErrOSSTRUnicodeToAscii( _In_ PCWSTR const       pwszIn,
                            _Out_opt_z_cap_post_count_(cchOut, *pcchRequired) PSTR const        pwszOut,
                            const size_t                cchOut,
                            size_t * const              pcchRequired = NULL,
                            const OSSTR_LOSSY       fLossy = OSSTR_NOT_LOSSY,
                            const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );

#ifdef __TCHAR_DEFINED

ERR ErrOSSTRUnicodeToTchar( const wchar_t *const    pwszIn,
                            __out_ecount(ctchOut) _TCHAR *const         ptszOut,
                            const INT               ctchOut,
                            const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );
#endif


#ifdef UNICODE

#define LOSStrLength                        LOSStrLengthW
#define OSStrAppend                     OSSTRAppendW
#define LOSSTRCompare                   LOSSTRCompareW
#define FOSSTRTrailingPathDelimiter     FOSSTRTrailingPathDelimiterW

#else

#define LOSStrLength                        LOSStrLengthA
#define OSStrAppend                     OSSTRAppendA
#define LOSSTRCompare                   LOSSTRCompareA
#define FOSSTRTrailingPathDelimiter     FOSSTRTrailingPathDelimiterA

#endif


ERR ErrOSSTRAsciiToUnicodeM( __in PCSTR const szzMultiIn, __out_ecount_z(cchMax) WCHAR * wszNew, ULONG cchMax, size_t * const pcchActual, const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );
ERR ErrOSSTRUnicodeToAsciiM( __in PCWSTR const wszzMultiIn, __out_ecount_z(cchMax) char * szNew, ULONG cchMax, size_t * const pcchActual, const OSSTR_CONVERSION osstrConversion = OSSTR_CONTEXT_DEPENDENT_CONVERSION );

#endif

