// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CC_HXX
#define _CC_HXX 1



#ifdef _MSC_VER
#ifndef WINNT
#endif
#else
#endif


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

#else

#include <sal.h>

#define IN
#define OUT

#endif

#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]




#ifndef _MSC_VER
#include <stdint.h>
#endif

#pragma push_macro( "VOID" )
#undef VOID
typedef void VOID;
#pragma pop_macro( "VOID" )


#ifdef _MSC_VER
    typedef int                 BOOL;
#else
    typedef int32_t             BOOL;
#endif

typedef unsigned int            FLAG32;

#define fFalse  BOOL( 0 )
#define fTrue   BOOL( !0 )


typedef char                CHAR;


typedef short               SHORT;
typedef unsigned short      USHORT;

#ifdef _MSC_VER
    typedef int                 INT;
    typedef unsigned int        UINT;
    typedef long                LONG;
    typedef unsigned long       ULONG;
#else
    typedef int32_t             INT;
    typedef uint32_t            UINT;
    typedef int32_t             LONG;
    typedef uint32_t            ULONG;
#endif

typedef long long           LONG64;
typedef unsigned long long  ULONG64;


typedef unsigned char       BYTE;
typedef USHORT              WORD;
typedef ULONG               DWORD;
typedef ULONG64             QWORD;


#if defined(_WIN64)
    #ifdef _MSC_VER
        typedef unsigned __int64    UNSIGNED_PTR;
        typedef __int64             SIGNED_PTR;
    #else
        typedef unsigned long       UNSIGNED_PTR;
        typedef long                SIGNED_PTR;
    #endif
#else
    typedef unsigned long           UNSIGNED_PTR;
    typedef long                    SIGNED_PTR;
#endif


#ifndef _MSC_VER
    typedef long long           __int64;
#endif

#if defined(_WIN64)
    #ifdef _MSC_VER

        typedef __int64 INT_PTR, *PINT_PTR;
        typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

        typedef __int64 LONG_PTR, *PLONG_PTR;
        typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

    #else

        typedef unsigned long       ULONG_PTR;
        typedef long                LONG_PTR;

    #endif
#else

    typedef __w64 int INT_PTR, *PINT_PTR;
    typedef __w64 unsigned int UINT_PTR, *PUINT_PTR;

    typedef __w64 long LONG_PTR, *PLONG_PTR;
    typedef __w64 unsigned long ULONG_PTR, *PULONG_PTR;

#endif

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef ULONG_PTR SIZE_T, *PSIZE_T;


typedef _Return_type_success_( return >= 0 ) INT                ERR;



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
#else
const UNSIGNED_PTR  upMin   = ulMin;
const UNSIGNED_PTR  upMax   = ulMax;
#endif

const QWORD     bMax    = 0xFF;
const QWORD     wMax    = 0xFFFF;
const QWORD     dwMax   = 0xFFFFFFFF;
const QWORD     qwMax   = 0xFFFFFFFFFFFFFFFF;


#ifndef _MSC_VER

    #define __cdecl
    #define __stdcall

#endif




#ifndef _MSC_VER

    #define _stricmp strcasecmp

#endif




#define OffsetOf(s,m)   (SIZE_T)&(((s *)0)->m)
#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))

#ifdef _MSC_VER
#else
#define _countof(rg)        ( sizeof(rg) / sizeof(rg[0]) )
#endif

#define _cbrg(rg)           ( _countof(rg) * sizeof(rg[0]) )



#ifdef _MSC_VER

    #pragma warning ( disable : 4100 )
    #pragma warning ( disable : 4201 )
    #pragma warning ( disable : 4127 )
    #pragma warning ( disable : 4355 )
    #pragma warning ( disable : 4512 )
    #pragma warning ( disable : 4706 )
    #pragma warning ( disable : 4786 )

    #ifdef DEBUG
    #else
        #pragma warning ( disable : 4189 )
    #endif

    #define Unused( var ) ( var )

#endif

#if !defined(BEGIN_PRAGMA_OPTIMIZE_DISABLE)
#define BEGIN_PRAGMA_OPTIMIZE_DISABLE(flags, bug, reason) \
    __pragma(optimize(flags, off))
#define BEGIN_PRAGMA_OPTIMIZE_ENABLE(flags, bug, reason) \
    __pragma(optimize(flags, on))
#define END_PRAGMA_OPTIMIZE() \
    __pragma(optimize("", on))
#endif


#endif

