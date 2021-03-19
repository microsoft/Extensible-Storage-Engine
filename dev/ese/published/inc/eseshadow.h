// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <windows.h>
#include <vss.h>

class EseShadowInformation;

typedef EseShadowInformation* EseShadowContext;

extern "C" {

// Initialize the ESE shadow subsystem. It uses COM with the
// apartment model.
HRESULT __stdcall
EseShadowInit(
    _Out_ EseShadowContext* pcontext
)
;

// Tears down the ESE shadow subsystem.
HRESULT __stdcall
EseShadowTerm(
    _In_ EseShadowContext context
)
;

// Takes a volume snapshot for the volume holding the specified database file
// (and log/system volumes as well, if they are specified, and different from
// the database volume). If the 3-letter basename is specified, then the log
// files will be replayed and the database will be brought to a clean state, if
// possible.
//
// Parameters:
// context
// szDatabaseFile
// szLogDirectory
//  Optional. If omitted, it's assumed to be the same as szDatabaseDirectory.
// szSystemDirectory
//  Optional. Location of the system directory (checkpoint file).
// szEseBaseName
//  Optional. If omitted, recovery will not be performed.
// fIgnoreMissingDb
//  Whether to ignore missing databases. Similar to `eseutil -r -i`
// fIgnoreLostLogs
//  Allows recovery to proceed in case there are missing log files.
HRESULT __stdcall
EseShadowCreateShadow(
    _In_ EseShadowContext context,
    _In_ PCWSTR szDatabaseFile,
    __in_opt PCWSTR szLogDirectory,
    __in_opt PCWSTR szSystemDirectory,
    __in_opt PCWSTR szEseBaseName,
    _In_ BOOL fIgnoreMissingDb,
    _In_ BOOL fIgnoreLostLogs
)
;

// Similar to EseShadowCreateShadow, but does not
// recover a database file. 
//
// Parameters:
// context
// szArbitraryFile
HRESULT __stdcall
EseShadowCreateSimpleShadow(
    _In_ EseShadowContext context,
    _In_ PCWSTR szArbitraryFile
)
;

// Retrieves the paths associated with each of the volume snapshots.
// The path looks like:
// \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy3
//
// NOTE: Previous versions took persistent snapshots and exposed them
// as d:\snapshot-nnnn.
HRESULT __stdcall
EseShadowMountShadow(
    _In_ EseShadowContext context,
    __out_ecount( cchOutDatabasePath ) PWSTR szOutDatabasePath,
    DWORD cchOutDatabasePath,
    __out_ecount( cchOutLogPath ) PWSTR szOutLogPath,
    DWORD cchOutLogPath,
    __out_ecount( cchOutSystemPath ) PWSTR szOutSystemPath,
    DWORD cchOutSystemPath
)
;

// Retrieves the paths associated with each of the volume snapshots.
// The path looks like:
// \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy3
HRESULT __stdcall
EseShadowMountSimpleShadow(
    _In_ EseShadowContext context,
    _Out_cap_( cchOutArbitraryPath ) PWSTR szOutArbitraryPath,
    DWORD cchOutArbitraryPath
)
;

// Cleans up the assosicated volume shadow copies.
HRESULT __stdcall
EseShadowPurgeShadow(
    _In_ EseShadowContext context
)
;

// Converts a GUID to a string.
HRESULT VssIdToString(
    _In_ const VSS_ID& vssId,
    __out_ecount(cch) PWSTR szStr,
    DWORD cch
)
;

} // extern "C"

// If ESESHADOW_LOCAL_DEFERRED_DLL is defined, then this header
// file implements stub functions to handle deferring loading the DLL
// until it's actually required (using LoadLibrary/GetProcAddress).
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

// If you enable ESESHADOW_LOCAL_DEFERRED_DLL, you must define the variables
// somewhere. Use this macro to declare the necessary variables.
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
    
//  Need a real value ... perhaps HRESULTify JET_wrnNyi
#define hrEseSnapshotNotYetImplemented  (-2)
#define hrEseSnapshotFuncNotPresent     (-3)

inline HRESULT __stdcall DelayLoadEseShadowInit(
    _Out_ EseShadowContext* pcontext
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
    _In_ EseShadowContext context
)
{
    if ( g_pfnEseShadowTerm )
    {
        return g_pfnEseShadowTerm( context );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowCreateShadow(
    _In_ EseShadowContext context,
    _In_ PCWSTR szDatabaseFile,
    __in_opt PCWSTR szLogDirectory,
    __in_opt PCWSTR szSystemDirectory,
    __in_opt PCWSTR szEseBaseName,
    _In_ BOOL fIgnoreMissingDb,
    _In_ BOOL fIgnoreLostLogs
)
{
    if ( g_pfnEseShadowCreateShadow )
    {
        return g_pfnEseShadowCreateShadow( context, szDatabaseFile, szLogDirectory, szSystemDirectory, szEseBaseName, fIgnoreMissingDb, fIgnoreLostLogs );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT __stdcall DelayLoadEseShadowCreateSimpleShadow(
    _In_ EseShadowContext context,
    _In_ PCWSTR szArbitraryFile
)
{
    if ( g_pfnEseShadowCreateSimpleShadow )
    {
        return g_pfnEseShadowCreateSimpleShadow( context, szArbitraryFile );
    }
    return hrEseSnapshotNotYetImplemented;
}

// Mounts the specified shadow to the file system.
// This flavour replays log files to provide a clean database.
inline HRESULT __stdcall DelayLoadEseShadowMountShadow(
    _In_ EseShadowContext context,
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

// Mounts the specified shadow to the file system.
// This flavour mounts as-is (the database could be dirty).
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
    _In_ EseShadowContext context
)
{
    if ( g_pfnEseShadowPurgeShadow )
    {
        return g_pfnEseShadowPurgeShadow( context );
    }
    return hrEseSnapshotNotYetImplemented;
}

inline HRESULT DelayLoadVssIdToString(
    _In_ const VSS_ID& vssId,
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

