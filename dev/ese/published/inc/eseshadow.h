// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <windows.h>
#include <vss.h>

class EseShadowInformation;

typedef EseShadowInformation* EseShadowContext;

extern "C" {

HRESULT __stdcall
EseShadowInit(
    __out EseShadowContext* pcontext
)
;

HRESULT __stdcall
EseShadowTerm(
    __in EseShadowContext context
)
;

HRESULT __stdcall
EseShadowCreateShadow(
    __in EseShadowContext context,
    __in PCWSTR szDatabaseFile,
    __in_opt PCWSTR szLogDirectory,
    __in_opt PCWSTR szSystemDirectory,
    __in_opt PCWSTR szEseBaseName,
    __in BOOL fIgnoreMissingDb,
    __in BOOL fIgnoreLostLogs
)
;

HRESULT __stdcall
EseShadowCreateSimpleShadow(
    __in EseShadowContext context,
    __in PCWSTR szArbitraryFile
)
;

HRESULT __stdcall
EseShadowMountShadow(
    __in EseShadowContext context,
    __out_ecount( cchOutDatabasePath ) PWSTR szOutDatabasePath,
    DWORD cchOutDatabasePath,
    __out_ecount( cchOutLogPath ) PWSTR szOutLogPath,
    DWORD cchOutLogPath,
    __out_ecount( cchOutSystemPath ) PWSTR szOutSystemPath,
    DWORD cchOutSystemPath
)
;

HRESULT __stdcall
EseShadowMountSimpleShadow(
    _In_ EseShadowContext context,
    _Out_cap_( cchOutArbitraryPath ) PWSTR szOutArbitraryPath,
    DWORD cchOutArbitraryPath
)
;

HRESULT __stdcall
EseShadowPurgeShadow(
    __in EseShadowContext context
)
;

HRESULT VssIdToString(
    __in const VSS_ID& vssId,
    __out_ecount(cch) PWSTR szStr,
    DWORD cch
)
;

}

#ifdef ESESHADOW_LOCAL_DEFERRED_DLL

typedef HRESULT (*PfnEseShadowInit)( EseShadowContext* pcontext );
typedef HRESULT (*PfnEseShadowTerm)( EseShadowContext pcontext );
typedef HRESULT (*PfnEseShadowCreateShadow)( EseShadowContext pcontext, PCWSTR  szDatabaseFile, PCWSTR szLogDirectory, PCWSTR szSystemDirectory, PCWSTR szEseBaseName, BOOL fIgnoreMissingDb, BOOL fIgnoreLostLogs );
typedef HRESULT (*PfnEseShadowCreateSimpleShadow)( EseShadowContext pcontext, PCWSTR    szArbitraryFile );
typedef HRESULT (*PfnEseShadowMountShadow)( EseShadowContext pcontext, PWSTR szOutDatabasePath, DWORD cchOutDatabasePath, PWSTR szOutLogPath, DWORD cchOutLogPath, PWSTR szOutSystemPath, DWORD cchOutSystemPath );
typedef HRESULT (*PfnEseShadowMountSimpleShadow)( EseShadowContext pcontext, PWSTR szOutArbitraryPath, DWORD cchOutArbitraryPath );
typedef HRESULT (*PfnEseShadowPurgeShadow)( EseShadowContext pcontext );
typedef HRESULT (*PfnVssIdToString)( const VSS_ID& vssId, PWSTR szStr, DWORD cch );

extern HMODULE g_hEseShadowLib;
extern PfnEseShadowInit g_pfnEseShadowInit;
extern PfnEseShadowTerm g_pfnEseShadowTerm;
extern PfnEseShadowCreateShadow g_pfnEseShadowCreateShadow;
extern PfnEseShadowCreateSimpleShadow g_pfnEseShadowCreateSimpleShadow;
extern PfnEseShadowMountShadow g_pfnEseShadowMountShadow;
extern PfnEseShadowMountSimpleShadow g_pfnEseShadowMountSimpleShadow;
extern PfnEseShadowPurgeShadow g_pfnEseShadowPurgeShadow;
extern PfnVssIdToString g_pfnVssIdToString;

#define ESESHADOW_LOCAL_DEFERRED_DLL_STATE                              \
    HMODULE g_hEseShadowLib = NULL;                                         \
    PfnEseShadowInit g_pfnEseShadowInit = NULL;                                 \
    PfnEseShadowTerm g_pfnEseShadowTerm = NULL;                             \
    PfnEseShadowCreateShadow g_pfnEseShadowCreateShadow = NULL;             \
    PfnEseShadowCreateSimpleShadow g_pfnEseShadowCreateSimpleShadow = NULL; \
    PfnEseShadowMountShadow g_pfnEseShadowMountShadow = NULL;               \
    PfnEseShadowMountSimpleShadow g_pfnEseShadowMountSimpleShadow = NULL;       \
    PfnEseShadowPurgeShadow g_pfnEseShadowPurgeShadow = NULL;                   \
    PfnVssIdToString g_pfnVssIdToString = NULL;
    
#define hrEseSnapshotNotYetImplemented  (-2)
#define hrEseSnapshotFuncNotPresent     (-3)

inline HRESULT __stdcall DelayLoadEseShadowInit(
    __out EseShadowContext* pcontext
)
{
    g_hEseShadowLib = LoadLibraryExW(
        L"esevss.dll",
        NULL,
        LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32 );

    if ( g_hEseShadowLib != NULL )
    {
        #define LoadPfn( pfnName )                                                      \
            g_pfn##pfnName = (Pfn##pfnName)GetProcAddress( g_hEseShadowLib, #pfnName ); \
            if ( NULL == g_pfn##pfnName )                                               \
            {                                                                           \
                return hrEseSnapshotFuncNotPresent;                                     \
            }

        LoadPfn( EseShadowInit );
        LoadPfn( EseShadowTerm );
        LoadPfn( EseShadowCreateShadow );
        LoadPfn( EseShadowCreateSimpleShadow );
        LoadPfn( EseShadowMountShadow );
        LoadPfn( EseShadowMountSimpleShadow );
        LoadPfn( EseShadowPurgeShadow );
        LoadPfn( VssIdToString );

        if ( g_pfnEseShadowInit )
        {
            return g_pfnEseShadowInit( pcontext );
        }

    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowTerm(
    __in EseShadowContext context
)
{
    if ( g_pfnEseShadowTerm )
    {
        return g_pfnEseShadowTerm( context );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowCreateShadow(
    __in EseShadowContext context,
    __in PCWSTR szDatabaseFile,
    __in_opt PCWSTR szLogDirectory,
    __in_opt PCWSTR szSystemDirectory,
    __in_opt PCWSTR szEseBaseName,
    __in BOOL fIgnoreMissingDb,
    __in BOOL fIgnoreLostLogs
)
{
    if ( g_pfnEseShadowCreateShadow )
    {
        return g_pfnEseShadowCreateShadow( context, szDatabaseFile, szLogDirectory, szSystemDirectory, szEseBaseName, fIgnoreMissingDb, fIgnoreLostLogs );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowCreateSimpleShadow(
    __in EseShadowContext context,
    __in PCWSTR szArbitraryFile
)
{
    if ( g_pfnEseShadowCreateSimpleShadow )
    {
        return g_pfnEseShadowCreateSimpleShadow( context, szArbitraryFile );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowMountShadow(
    __in EseShadowContext context,
    __out_ecount( cchOutDatabasePath ) PWSTR szOutDatabasePath,
    DWORD cchOutDatabasePath,
    __out_ecount( cchOutLogPath ) PWSTR szOutLogPath,
    DWORD cchOutLogPath,
    __out_ecount( cchOutSystemPath ) PWSTR szOutSystemPath,
    DWORD cchOutSystemPath
)
{
    if ( g_pfnEseShadowMountShadow )
    {
        return g_pfnEseShadowMountShadow( context, szOutDatabasePath, cchOutDatabasePath, szOutLogPath, cchOutLogPath, szOutSystemPath, cchOutSystemPath );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowMountSimpleShadow(
    _In_ EseShadowContext context,
    _Out_cap_( cchOutArbitraryPath ) PWSTR szOutArbitraryPath,
    DWORD cchOutArbitraryPath
)
{
    if ( g_pfnEseShadowMountSimpleShadow )
    {
        return g_pfnEseShadowMountSimpleShadow( context, szOutArbitraryPath, cchOutArbitraryPath );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowPurgeShadow(
    __in EseShadowContext context
)
{
    if ( g_pfnEseShadowPurgeShadow )
    {
        return g_pfnEseShadowPurgeShadow( context );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT DelayLoadVssIdToString(
    __in const VSS_ID& vssId,
    __out_ecount(cch) PWSTR szStr,
    DWORD cch
)
{
    if ( g_pfnVssIdToString )
    {
        return g_pfnVssIdToString( vssId, szStr, cch );
    }
    return hrEseSnapshotNotYetImplemented;
}

#endif

