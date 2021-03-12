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

//
//      SAL is not defined everywhere
//

#ifndef _MSC_VER

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

#else // _MSC_VER

#include <sal.h>

#endif // !_MSC_VER

#define IN
#define OUT

//  Like SAL this produces a compile-time assert ...
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]



//
//      Types
//

#ifndef _MSC_VER
//  the required intXX_t types are std on VC - windows as well, but we can't define
//  them commonly b/c we get redefinition of basic types conflicts on WINNT.
#include <stdint.h>
#endif

//  odd void indirection
#pragma push_macro( "VOID" )
#undef VOID
typedef void VOID;
#pragma pop_macro( "VOID" )

//  Boolean types
//

//  ESE's standard BOOL is 4 bytes, unlike bool which is 1 byte.  This is used
//  in a bunch of persisted structures and such, so changing it to bool is non-
//  trivial.  We will fix it at 4 bytes for now.  Besides if you really wanted
//  to save space, just use a bit-field.
#ifdef _MSC_VER
    typedef int                 BOOL;
#else
    typedef int32_t             BOOL;
#endif

//  Another complication, the signed BOOL and C++ bool are unsuitable for bit fields
//  of 1-bit size, due to the way C sign extends 1 to be 0xFFFFFFFF.  This type is
//  designed for usage in bit fields involving 4-byte types (INT, ULONG, etc) without 
//  these sign extension problems.
typedef unsigned int            FLAG32;

#define fFalse  BOOL( 0 )
#define fTrue   BOOL( !0 )

//  String types
//

typedef char                CHAR;

//  Basic integer types
//

typedef short               SHORT;
typedef unsigned short      USHORT;

#ifdef _MSC_VER
    typedef int                 INT;
    typedef unsigned int        UINT;
    typedef long                LONG;
    typedef unsigned long       ULONG;
#else
    // On most other platforms, int and long are 64-bit on 64-bit platforms, but the ESE format
    // is dependent upon LONG being 32-bits.
    typedef int32_t             INT;
    typedef uint32_t            UINT;
    typedef int32_t             LONG;
    typedef uint32_t            ULONG;
#endif

typedef long long           LONG64;
typedef unsigned long long  ULONG64;

//  Machine word types
//

typedef unsigned char       BYTE;
typedef USHORT              WORD;
typedef ULONG               DWORD;
typedef ULONG64             QWORD;

//  Pointer types
//

#if defined(_WIN64)
    #ifdef _MSC_VER
        typedef unsigned __int64    UNSIGNED_PTR;
        typedef __int64             SIGNED_PTR;
    #else // !_MSC_VER
        typedef unsigned long       UNSIGNED_PTR;
        typedef long                SIGNED_PTR;
    #endif // _MSC_VER
#else
    typedef unsigned long           UNSIGNED_PTR;
    typedef long                    SIGNED_PTR;
#endif


//typedef long long         INT64;
//typedef unsigned long long    UINT64;
#ifndef _MSC_VER
    typedef long long           __int64;
#endif

#if defined(_WIN64)
    #ifdef _MSC_VER

        typedef __int64 INT_PTR, *PINT_PTR;
        typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

        typedef __int64 LONG_PTR, *PLONG_PTR;
        typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

    #else // !_MSC_VER

        typedef unsigned long       ULONG_PTR;
        typedef long                LONG_PTR;

    #endif // _MSC_VER
#else

    typedef __w64 int INT_PTR, *PINT_PTR;
    typedef __w64 unsigned int UINT_PTR, *PUINT_PTR;

    typedef __w64 long LONG_PTR, *PLONG_PTR;
    typedef __w64 unsigned long ULONG_PTR, *PULONG_PTR;

#endif

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef ULONG_PTR SIZE_T, *PSIZE_T;

//  Common project types
//

typedef _Return_type_success_( return >= 0 ) INT                ERR;



//
//      Limits
//
const USHORT    usMin   = 0x0000;
const USHORT    usMax   = 0xFFFF;

const LONG      lMin    = 0x80000000;
const LONG      lMax    = 0x7FFFFFFF;

const ULONG     ulMin   = 0x00000000;
const ULONG     ulMax   = 0xFFFFFFFF;

const LONG64    llMin   = 0x8000000000000000;
const LONG64    llMax   = 0x7FFFFFFFFFFFFFFF;

const ULONG64   ullMin  = 0x0000000000000000;
const ULONG64   ullMax  = 0xFFFFFFFFFFFFFFFF;

#if defined(_WIN64)
const UNSIGNED_PTR  upMin   = ullMin;
const UNSIGNED_PTR  upMax   = ullMax;
#else // !_WIN64
const UNSIGNED_PTR  upMin   = ulMin;
const UNSIGNED_PTR  upMax   = ulMax;
#endif // _WIN64

const QWORD     bMax    = 0xFF;
const QWORD     wMax    = 0xFFFF;
const QWORD     dwMax   = 0xFFFFFFFF;
const QWORD     qwMax   = 0xFFFFFFFFFFFFFFFF;

//
//      Declarative Defines
//

#ifndef _MSC_VER

    // Only the Microsoft VC++ in some build environment has alternate calling conventions as default at play and 
    // thus requires cdecl to be declared where we want the classic calling convention, so on we can just 
    // define this to nothing on UNIX (as everything is implicitly __cdecl there).
    #define __cdecl
    #define __stdcall

#endif // !_MSC_VER



//
//      Map commonly used CRT like pseudo functions
//

#ifndef _MSC_VER

    #define _stricmp strcasecmp

#endif // !_MSC_VER



//
//      Basic "C operators"
//

#ifdef _MSC_VER
#define OffsetOf(s,m)   (SIZE_T)&(((s *)0)->m)
#else
#define OffsetOf(s,m)    __builtin_offsetof( s, m )
#endif

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))

#ifdef _MSC_VER
//  No need - this set of operators (such as _countof()) is defined for MSVC tool set.
#else
#define _countof(rg)        ( sizeof(rg) / sizeof(rg[0]) )
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
    #else // DEBUG
        #pragma warning ( disable : 4189 )  //  local variable is initialized but not referenced
    #endif // !DEBUG

    #define Unused( var ) ( var )

#endif // _MSC_VER

#if !defined(BEGIN_PRAGMA_OPTIMIZE_DISABLE)
#define BEGIN_PRAGMA_OPTIMIZE_DISABLE(flags, bug, reason) \
    __pragma(optimize(flags, off))
#define BEGIN_PRAGMA_OPTIMIZE_ENABLE(flags, bug, reason) \
    __pragma(optimize(flags, on))
#define END_PRAGMA_OPTIMIZE() \
    __pragma(optimize("", on))
#endif


#endif // !_CC_HXX

