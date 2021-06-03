// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// needed for JET errors
#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include <esent_x.h>
#endif
#ifdef BUILD_ENV_IS_EX
#include <jet.h>
#endif

#include <windows.h>
#include <cstdio>
#include <stdlib.h>

#include <functional>

//  We don't want to depend upon the full OS layer, but we need library.hxx and a few supporting things
//  for library.hxx to work.

#ifndef Expected
#define Expected    Assert
#else
#error "If this is fixed everywhere pfnjet.hxx is used, then remove whole thing, otherwise just strip this #else #error clause."
#endif

#ifndef OnDebug
#ifdef DEBUG
#define OnDebug( code )             code
#else  //   !DEBUG
#define OnDebug( code )
#endif //   DEBUG
#else  //   defined(OnDebug)
#error "If this is fixed everywhere pfnjet.hxx is used, then remove whole thing, otherwise just strip this #else #error clause."
#endif //   OnDebug

#include "cc.hxx"
#include "types.hxx"
#include "library.hxx"

//
//  Helpers
//

BOOL FDllLoaded( const CHAR * const szDll );

//
//  Indirected JET API...
//

#define FEseDllLoaded()     ( FDllLoaded( "ese.dll" ) | FDllLoaded( "esent.dll" ) )

inline JET_ERR ErrThunkNotSupported()
    {
    wprintf( L"Not supported ....\n");
    fflush(stdout);
    return JET_errJetApiLoadFailThunk;
    }

template< typename Ret, typename Arg1 >
INLINE Ret ErrFailed( Arg1 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2 >
INLINE Ret ErrFailed( Arg1, Arg2 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2, typename Arg3 >
INLINE Ret ErrFailed( Arg1, Arg2, Arg3 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4 >
INLINE Ret ErrFailed( Arg1, Arg2, Arg3, Arg4 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5 >
INLINE Ret ErrFailed( Arg1, Arg2, Arg3, Arg4, Arg5 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6 >
INLINE Ret ErrFailed( Arg1, Arg2, Arg3, Arg4, Arg5, Arg6 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8 >
INLINE Ret ErrFailed( Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8 )
    {
    return Ret( ErrThunkNotSupported() );
    }

template< typename Ret, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9 >
INLINE Ret ErrFailed( Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8, Arg9 )
    {
    return Ret( ErrThunkNotSupported() );
    }

//  returns JET_ERR, LoadErr returns the standard ERR thunk value - JET_errJetApiLoadFailThunk / -68

#define JetApiFunc( g_pfn, mszzDlls, func, oslf )               \
            FunctionLoader<decltype(&func)> g_pfn( ErrFailed, mszzDlls, #func, oslf );

//  The loader system was really meant to be used for OS-functionality, not ESE functionality, so its
//  flags aren't 100% sensible for this purpose, it would be good to enhance it, but don't have time
//  right now, so anyway we need these two odd flags:
//    - oslfExpectedOnWin5x because you need at least one ExpectedOnXxx flag or it asserts.
//    - oslfNotExpectedOnCoreSystem because you need to tell it not to assert if a function is not 
//      present and that returning a failed thunk error is acceptable for this function.

const OSLoadFlags oslfJetApiBasic   = oslfNonSystem|oslfStrictFree|oslfExpectedOnWin5x;
const OSLoadFlags oslfJetApiNewApi  = oslfNonSystem|oslfStrictFree|oslfNotExpectedOnCoreSystem|oslfExpectedOnWin5x;

//  we call pfnJetSetSystemParameterA.ErrIsPresent() to ensure it is loaded in retail.

#define DECL_JET_API()          \
    JetApiFunc( pfnJetSetSystemParameterA, g_mwszzEse, JetSetSystemParameterA, oslfJetApiBasic );   \
    JetApiFunc( pfnJetGetSystemParameterA, g_mwszzEse, JetGetSystemParameterA, oslfJetApiBasic );   \
    JetApiFunc( pfnJetSetSystemParameterW, g_mwszzEse, JetSetSystemParameterW, oslfJetApiBasic );   \
    JetApiFunc( pfnJetGetSystemParameterW, g_mwszzEse, JetGetSystemParameterW, oslfJetApiBasic );   \
    JetApiFunc( pfnJetSetSessionParameter, g_mwszzEse, JetSetSessionParameter, oslfJetApiNewApi );  \
    JetApiFunc( pfnJetGetSessionParameter, g_mwszzEse, JetGetSessionParameter, oslfJetApiNewApi );  \
    JetApiFunc( pfnJetEnableMultiInstanceW, g_mwszzEse, JetEnableMultiInstanceW, oslfJetApiBasic ); \
    JetApiFunc( pfnJetCreateInstanceA, g_mwszzEse, JetCreateInstanceA, oslfJetApiBasic );       \
    JetApiFunc( pfnJetCreateInstanceW, g_mwszzEse, JetCreateInstanceW, oslfJetApiBasic );       \
    JetApiFunc( pfnJetInit, g_mwszzEse, JetInit, oslfJetApiBasic );                 \
    JetApiFunc( pfnJetTerm, g_mwszzEse, JetTerm, oslfJetApiBasic );                 \
    JetApiFunc( pfnJetCreateDatabaseA, g_mwszzEse, JetCreateDatabaseA, oslfJetApiBasic );       \
    JetApiFunc( pfnJetBeginSessionA, g_mwszzEse, JetBeginSessionA, oslfJetApiBasic );       \
    JetApiFunc( pfnJetEndSession, g_mwszzEse, JetEndSession, oslfJetApiBasic );         \
    JetApiFunc( pfnJetCreateDatabase2W, g_mwszzEse, JetCreateDatabase2W, oslfJetApiBasic );     \
    JetApiFunc( pfnJetBackupA, g_mwszzEse, JetBackupA, oslfJetApiBasic );               \
    JetApiFunc( pfnJetRestoreA, g_mwszzEse, JetRestoreA, oslfJetApiBasic );             \
    JetApiFunc( pfnJetTestHook, g_mwszzEse, JetTestHook, oslfJetApiNewApi );            \
    JetApiFunc( pfnJetGetDatabaseInfoW, g_mwszzEse, JetGetDatabaseInfoW, oslfJetApiBasic );     \
    JetApiFunc( pfnJetGetDatabaseFileInfoW, g_mwszzEse, JetGetDatabaseFileInfoW, oslfJetApiBasic ); \
    (void)pfnJetSetSystemParameterA.ErrIsPresent();


