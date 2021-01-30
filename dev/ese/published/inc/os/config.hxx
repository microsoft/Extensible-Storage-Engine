// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef _OS_CONFIG_HXX_INCLUDED
#define _OS_CONFIG_HXX_INCLUDED


#if defined(RTM) || defined(MINIMAL_FUNCTIONALITY)
#ifndef DEBUG
#define DISABLE_REGISTRY
#endif
#endif

#ifdef DISABLE_REGISTRY

#define FOSConfigGet( wszPath, wszName, wszBuf, cbBuf ) ( fFalse )

#else

#define FOSConfigGet( wszPath, wszName, wszBuf, cbBuf ) FOSConfigGet_( wszPath, wszName, wszBuf, cbBuf )

#endif




const BOOL FOSConfigGet_( __in_z const WCHAR * const wszPath, __in_z const WCHAR* const wszName, __out_bcount_z(cbBuf) WCHAR* const wszBuf, const LONG cbBuf );



class CConfigStore;

ERR ErrOSConfigStoreInit( _In_z_ const WCHAR * const wszPath, _Outptr_ CConfigStore ** ppcs );
void OSConfigStoreTerm( _In_ CConfigStore * pcs );

enum ConfigStoreReadFlags
{
};

enum ConfigStoreSubPath
{
    csspTop = 0,

    csspSysParamDefault,
    csspSysParamOverride,

    csspDiag,



    csspMax,
};

BOOL FConfigValuePresent( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName );
BOOL FConfigValuePresent( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const CHAR * const szValueName );

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const CHAR * const szValueName, _Out_ ULONG * pulValue );
ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName, _Out_ ULONG * pulValue );

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const CHAR * const szValueName, _Out_ QWORD * pqwValue );
ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName, _Out_ QWORD * pqwValue );

#endif

