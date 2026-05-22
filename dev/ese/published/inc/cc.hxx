// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#ifndef _CC_HXX
#define _CC_HXX 1


// This is the C Compiler abstraction header file / library that isolates C 
// elements for common consumption across all of ESE dev and test code.

// Some interesting defines we might try ...
#ifdef _MSC_VER

    #ifndef WINNT
    //#define WINNT 1
    #endif

#else

    //#define UNIX  1
    //#define _GCC  1

#endif

#include <string>

//
//      Source Annotation Language (SAL)
//

#ifdef _MSC_VER

    #include <sal.h>

    // These conflict with definitions in headers such as <algorithm> on non-Windows platforms.
    // _In_ and _Out_ should be used instead anyway, according to Microsoft's SAL documentation.
    //
    // Unfortunately, can't easily redefine __in and __out because these old-style annotations
    // crept back into the Windows headers ntsecapi.h and dbgeng.h, which we include
    //
    // #undef __in
    // #undef __out
    //
    // #define __in Use_In_instead_of__in
    // #define __out Use_Out_instead_of__out

#else // !_MSC_VER

    //      SAL is not defined everywhere
    //

    #define _In_
    #define _Out_
    #define _Out_opt_
    #define _Inout_
    #define _In_count_(x)
    #define _In_reads_(x)
    #define _In_reads_opt_(x)
    #define _In_reads_bytes_(x)
    #define _In_reads_bytes_opt_(x)
    #define _Inout_updates_bytes_(x)
    #define _Inout_updates_opt_(x)
    #define _Out_writes_(x)
    #define _Out_writes_to_opt_(x, y)
    #define _Out_writes_bytes_(x)
    #define _Out_writes_bytes_opt_(x)
    #define _Out_writes_bytes_to_(x, y)
    #define _Out_writes_bytes_to_opt_(x, y)
    #define _Outptr_result_buffer_(x)
    #define _Null_terminated_
    #define _Return_type_success_(x)
    #define _Field_size_(x)
    #define _Field_size_opt_(x)
    #define _Field_size_bytes_(x)
    #define _Field_size_bytes_opt_(x)

#endif 


//
//      Types
//

//  odd void indirection
#pragma push_macro( "VOID" )
#undef VOID
typedef void                            VOID;
#pragma pop_macro( "VOID" )
typedef VOID *                          PVOID;

//  String types
//  We base our strings off of char from the C++ standard and wchar_t
typedef char                            CHAR;
typedef unsigned char                   UCHAR;
typedef _Null_terminated_ CHAR *        PSTR;
typedef _Null_terminated_ const CHAR *  PCSTR;
typedef wchar_t                         WCHAR;
typedef _Null_terminated_ WCHAR *       PWSTR;
typedef _Null_terminated_ const WCHAR * PCWSTR;

// We'll base our integral types on the types from stdint.h, a standards-defined file that's guaranteed to be
// equivalent on all platforms.
//
#include <stdint.h>

//  Constant sized integer types
//
typedef int8_t                          INT8;
typedef uint8_t                         UINT8;
typedef int16_t                         INT16;
typedef uint16_t                        UINT16;
typedef int32_t                         INT32;
typedef uint32_t                        UINT32;
typedef int64_t                         INT64;
typedef uint64_t                        UINT64;

//  Variable sized integer types, based on pointer-sized integer types
//
typedef intptr_t                        INT_PTR;
typedef uintptr_t                       UINT_PTR;
typedef uintptr_t                       SIZE_T;

//  Idealized machine word types
//
typedef UINT8                           BYTE;
typedef UINT16                          WORD;
typedef unsigned long                   DWORD;  //  Note the unsafe use of long.  Currently required to interop with
                                                //  Windows headers without a bunch of casts.
typedef UINT64                          QWORD;

//  ESE's standard BOOL is 4 bytes, unlike C++'s bool which is 1 byte.  This is
//  used in a bunch of persisted structures and such, so changing it to bool is
//  non-trivial.  We will fix it at 4 bytes.  Besides if you really wanted to
//  save space, just use a bit-field.
typedef INT32                           BOOL;
#define fFalse                          BOOL( 0 )
#define fTrue                           BOOL( !0 )

//  Another complication, the signed BOOL and C++ bool are unsuitable for bit fields
//  of 1-bit size, due to the way C++ sign extends 1 to be 0xFFFFFFFF.  This type is
//  designed for usage in bit fields involving 4-byte types without thse sign extension
//  problems.
typedef UINT32                         FLAG32;

// A Note about UINT32 and LONG.
// ESE code has traditionally intermingled "int" based types and "long" based types.  This
// is because, in Windows, both are 32 bit unsigned integral values.  And it's totally safe
// since the compiler will coerce between the two AND the two have the same range, bit layout, etc.
// HOWEVER.
// Because they are different base types, the compiler thinks that (int *) and (long *) are
// too different to coerce.  That's why we have random casts between the two.  The C++ spec
// says that it is permissible for a compiler to play tricks such that it actually isn't safe
// to just cast an "int *" to a "long *".  And it's possible that systems such as GCC on Linux
// actually do play games that make that unsafe (even if they were the same number of bytes,
// which they aren't on Linux).  But here, now, with MSVC and Windows, it's safe.  Furthermore,
// we actually do it all the time.  We're working towards a reorganization of our
// base types such that we don't mix and match those base types.  Until that's done and clean,
// we need the following:

// These eventually should only be in the OS directory.  They're for places where it's mandatory
// to use "long" to interact with the OS in code that otherwise wishes to restrict itself to the
// "core" ESE data types defined in this file.  The most likely places to use them will be in
// casts and in mirrored declarations of OS provided functions (i.e. not picked up from a header).
typedef long                           OS_WIN_LONG;
typedef unsigned long                  OS_WIN_ULONG;
typedef unsigned long                  OS_WIN_DWORD;
typedef long *                         OS_WIN_PLONG;
typedef unsigned long *                OS_WIN_PULONG;
typedef unsigned long *                OS_WIN_PDWORD;

// We're going to get rid of these from this file as we reorganize to the base types above.
// This file holds the minimal type definitions that the "core" implementation of ESE may use.
// There are other "non-core" portions (like the perfmon code) that will be allowed to use
// a wider variety of types in order to interact with Windows.  Those types will be defined
// elsewhere and only included where absolutely needed.
typedef INT16                          SHORT;
typedef INT32                          INT;
typedef INT64                          LONG64;
typedef INT64                          LONGLONG;
typedef UINT8 *                        PBYTE;
typedef UINT16                         USHORT;
typedef UINT32                         DWORD32;      // Unlike DWORD, not really dereived from "long" type.
typedef UINT32                         UINT;
typedef UINT32                         ULONG32;      // Not really derived from "long" type.
typedef UINT64                         ULONG64;
typedef UINT64                         ULONGLONG;
typedef UINT64 *                       PULONGLONG;
typedef INT_PTR                        LONG_PTR;     // Not really derived from "long" type.
typedef INT_PTR                        SIGNED_PTR;
typedef UINT_PTR                       DWORD_PTR;    // Not really derived from "long" type.
typedef UINT_PTR                       ULONG_PTR;    // Not really derived from "long" type.
typedef UINT_PTR                       UNSIGNED_PTR;

typedef long                           LONG;         // Note the problematic "long" derived type.
typedef unsigned long                  ULONG;        // Note the problematic "long" derived type.
typedef unsigned long *                PULONG;       // Note the problematic "long" derived type.

typedef PSTR                           LPSTR;        // The LP stands for "Long Pointer" which has been obsolete for decades.
typedef PCSTR                          LPCSTR;       // The LP stands for "Long Pointer" which has been obsolete for decades.
typedef PWSTR                          LPWSTR;       // The LP stands for "Long Pointer" which has been obsolete for decades.
typedef PCWSTR                         LPCWSTR;      // The LP stands for "Long Pointer" which has been obsolete for decades.

//  Common project types
//

typedef _Return_type_success_( return >= 0 ) INT32    ERR;


//
//      Limits
//

constexpr UINT16    usMin   = 0x0000;
constexpr UINT16    usMax   = 0xFFFF;

constexpr INT32     lMin    = 0x80000000;
constexpr INT32     lMax    = 0x7FFFFFFF;

constexpr UINT32    ulMin   = 0x00000000;
constexpr UINT32    ulMax   = 0xFFFFFFFF;

constexpr INT64     llMin   = 0x8000000000000000;
constexpr INT64     llMax   = 0x7FFFFFFFFFFFFFFF;

constexpr UINT64    ullMin  = 0x0000000000000000;
constexpr UINT64    ullMax  = 0xFFFFFFFFFFFFFFFF;

#if defined(_WIN64)
const UNSIGNED_PTR  upMin   = 0x0000000000000000;
const UNSIGNED_PTR  upMax   = 0xFFFFFFFFFFFFFFFF;
#else // !_WIN64
const UNSIGNED_PTR  upMin   = 0x00000000;
const UNSIGNED_PTR  upMax   = 0xFFFFFFFF;
#endif // _WIN64

constexpr QWORD     bMax    = 0xFF;
constexpr QWORD     wMax    = 0xFFFF;
constexpr QWORD     dwMax   = 0xFFFFFFFF;
constexpr QWORD     qwMax   = 0xFFFFFFFFFFFFFFFF;

// Explicit numeric values were used to emphasize differences visually, but lets make sure the values used
// match the expected symbolic values from standard headers.
static_assert( usMin   == 0 );
static_assert( usMax   == UINT16_MAX );
static_assert( lMin    == INT32_MIN );
static_assert( lMax    == INT32_MAX );
static_assert( ulMin   == 0 );
static_assert( ulMax   == UINT32_MAX );
static_assert( llMin   == INT64_MIN );
static_assert( llMax   == INT64_MAX );
static_assert( ullMin  == 0 );
static_assert( ullMax  == UINT64_MAX );
static_assert( upMin   == 0 );
static_assert( upMax   == UINTPTR_MAX );
static_assert( bMax    == UINT8_MAX );
static_assert( wMax    == UINT16_MAX );
static_assert( dwMax   == UINT32_MAX );
static_assert( qwMax   == UINT64_MAX );


//
//      Declarative Defines
//

//  This will go away to be replaced with static_assert
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

//
//      Call type overrides
//

#ifdef _MSC_VER

    // None

#else // !_MSC_VER

    // Only the Microsoft VC++ in some build environment has alternate calling conventions as default at play and 
    // thus requires cdecl to be declared where we want the classic calling convention, so on we can just 
    // define this to nothing elsewhere (such as GCC on Unix, as everything is implicitly __cdecl there).
    #define __cdecl
    #define __stdcall

#endif


//
//      Map commonly used CRT like pseudo functions and basic "C" operators
//

#ifdef _MSC_VER

    #define OffsetOf(s,m)   (SIZE_T)&(((s *)0)->m)
    // No need for _stricmp, it's defined by MSVC
    // No need for _countof, it's defined by MSVC

#else // !_MSC_VER

    #define OffsetOf(s,m)    __builtin_offsetof( s, m )
    #define _stricmp         strcasecmp
    #define _countof(rg)     ( sizeof(rg) / sizeof(rg[0]) )

#endif

#define _cbrg(rg)           ( _countof(rg) * sizeof(rg[0]) )

//
//      Compiler warning control
//

//  Only disable warnings that we think are valid and good coding practices here, if you are
//  dealing with a bad coding practice, the disable should be as narrowly scoped as possible.
#ifdef _MSC_VER

    #pragma warning ( disable : 4100 )  //  unreferenced formal parameter
    #pragma warning ( disable : 4201 )  //  we allow unnamed structs/unions
    #pragma warning ( disable : 4127 )  //  conditional expression is constant
    #pragma warning ( disable : 4355 )  //  we allow the use of this in ctor-inits
    #pragma warning ( disable : 4512 )  //  assignment operator could not be generated
    #pragma warning ( disable : 4706 )  //  assignment within conditional expression
    #pragma warning ( disable : 4786 )  //  we allow huge symbol names

    #ifdef DEBUG
        // None
    #else // !DEBUG
        #pragma warning ( disable : 4189 )  //  local variable is initialized but not referenced
    #endif

    #define Unused( var ) ( var )

#else // !_MSC_VER

    // None

#endif

#if !defined(BEGIN_PRAGMA_OPTIMIZE_DISABLE)
    #define BEGIN_PRAGMA_OPTIMIZE_DISABLE(flags, bug, reason) \
        __pragma(optimize(flags, off))
    #define BEGIN_PRAGMA_OPTIMIZE_ENABLE(flags, bug, reason) \
        __pragma(optimize(flags, on))
    #define END_PRAGMA_OPTIMIZE() \
        __pragma(optimize("", on))
#endif


#endif // !_CC_HXX

