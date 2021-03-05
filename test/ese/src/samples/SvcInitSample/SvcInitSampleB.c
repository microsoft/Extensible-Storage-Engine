// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>

#include <esent.h>

#include "SvcInit.h"







static const WCHAR * const      g_wszDatabaseX = L".\\DirX\\SvcDbX.edb";
CONN_DB_STATE                   g_dbStateSvcX;
static JET_SETSYSPARAM_W        g_rgsspAgreedUponGlobalParametersX[] = {
    { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL , JET_wrnNyi },
};


static const WCHAR * const      g_wszDatabaseY = L".\\DirY\\SvcDbY.edb";
CONN_DB_STATE                   g_dbStateSvcY;
static JET_SETSYSPARAM_W            g_rgsspAgreedUponGlobalParametersY[] = {
    { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL , JET_wrnNyi },
};



static const WCHAR * const      g_wszDatabaseZ = L".\\DirZ\\SvcDbZ.edb";
CONN_DB_STATE                   g_dbStateSvcZ;
static JET_SETSYSPARAM_W        g_rgsspDisagreeableGlobalParametersZ[] = {
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    { JET_paramDatabasePageSize,        32 * 1024, NULL,    JET_wrnNyi },
    { JET_paramEnableFileCache,         FALSE, NULL,            JET_wrnNyi },
    { JET_paramEnableViewCache,         FALSE, NULL,        JET_wrnNyi },
};



static const WCHAR * const      g_wszDatabaseW = L".\\DirW\\SvcDbZ.edb";
CONN_DB_STATE                   g_dbStateSvcW;
static JET_SETSYSPARAM_W        g_rgsspDisagreeableGlobalParametersW[] = {
    { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL , JET_wrnNyi },
};



DWORD ConfigureEseRegistry( const WCHAR * const wszRegRoot );
DWORD ConfigureEseRegistryForInst( const WCHAR * const wszRegRoot );




JET_ERR ErrConfigStorePathStartSample( const WCHAR * const wszRegistryConfigPath, const INT fStartSvcWInstead  )
{
    JET_ERR         err = JET_errSuccess;
    ULONG           cSucceed;
    CONN_DB_STATE * pconn = NULL;

    ConfigureEseRegistry( wszRegistryConfigPath );



    assert( g_rgsspAgreedUponGlobalParametersX[0].paramid == JET_paramConfigStoreSpec );
    g_rgsspAgreedUponGlobalParametersX[0].sz = wszRegistryConfigPath;
    wprintf( L"\tService X: Initializing off %ws\n", wszRegistryConfigPath );

    err = JetEnableMultiInstanceW( g_rgsspAgreedUponGlobalParametersX, _countof(g_rgsspAgreedUponGlobalParametersX), &cSucceed );
    wprintf( L"\tService X: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
    if ( err == JET_errSystemParamsAlreadySet )
    {
        wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
        assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersX) );
        err = JET_errSuccess;
    }
    assert( err != JET_errSystemParamsDisagreeable );
    Call( err  );

    pconn = &g_dbStateSvcX;

    Call( JetCreateInstance2W( &pconn->inst, L"ESE_CLIENT_TODO_FRIENDLY_X", L"ESE_CLIENT_TODO_FRIENDLY_X", JET_bitNil ) );

    (void)CreateDirectoryW( L".\\DirX", 0 );

    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramSystemPath, 0, L".\\DirX" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFilePath, 0, L".\\DirX" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramTempPath, 0, L".\\DirX\\" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramCheckpointDepthMax, 5 * 1024 * 1024, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFileSize, 512, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramBaseName, 0, L"xxx" ) );

    wprintf( L"\tService X: Instance created and inst specific params set.\n" );


    err = JetInit3W( &pconn->inst, NULL, JET_bitNil );
    if ( err < JET_errSuccess )
    {
        pconn->inst = JET_instanceNil;
    }
    Call( err  );

    wprintf( L"\tService X: Initialized Inst (and Transaction Log).\n" );

    Call( JetBeginSessionW( pconn->inst, &pconn->sesidSystem, NULL, NULL ) );

    err = JetAttachDatabaseW( pconn->sesidSystem, g_wszDatabaseX, 0 );
    if ( err == JET_errFileNotFound )
    {
        err = JetCreateDatabaseW( pconn->sesidSystem, g_wszDatabaseX, NULL, &pconn->dbid, 0 );
    }
    else if ( err >= JET_errSuccess )
    {
        err = JetOpenDatabaseW( pconn->sesidSystem, g_wszDatabaseX, L"", &pconn->dbid, 0 );
    }
    Call( err  );

    wprintf( L"\tService X: Attached (or Created) Database.\n" );




    assert( g_rgsspAgreedUponGlobalParametersY[0].paramid == JET_paramConfigStoreSpec );
    g_rgsspAgreedUponGlobalParametersY[0].sz = wszRegistryConfigPath;
    wprintf( L"\tService Y: Initializing off %ws\n", wszRegistryConfigPath );

    err = JetEnableMultiInstanceW( g_rgsspAgreedUponGlobalParametersY, _countof(g_rgsspAgreedUponGlobalParametersY), &cSucceed );
    wprintf( L"\tService Y: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
    if ( err == JET_errSystemParamsAlreadySet )
    {
        wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
        assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersY) );
        err = JET_errSuccess;
    }
    assert( err != JET_errSystemParamsDisagreeable );
    Call( err  );

    pconn = &g_dbStateSvcY;

    Call( JetCreateInstance2W( &pconn->inst, L"ESE_CLIENT_TODO_FRIENDLY_Y", L"ESE_CLIENT_TODO_FRIENDLY_Y", JET_bitNil ) );

    (void)CreateDirectoryW( L".\\DirY", 0 );
    
    {
    WCHAR wszRegistryConfigPathInstY[260];
    Call( ErrFromStrsafeHr( StringCbPrintfW( wszRegistryConfigPathInstY, sizeof(wszRegistryConfigPathInstY), L"%ws_SvcY", wszRegistryConfigPath ) ) );
    ConfigureEseRegistryForInst( wszRegistryConfigPathInstY );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramConfigStoreSpec, 0, wszRegistryConfigPathInstY ) );
    }

    wprintf( L"\tService Y: Instance created and inst specific params set.\n" );


    err = JetInit3W( &pconn->inst, NULL, JET_bitNil );
    if ( err < JET_errSuccess )
    {
        pconn->inst = JET_instanceNil;
    }
    Call( err  );

    wprintf( L"\tService Y: Initialized Inst (and Transaction Log).\n" );

    Call( JetBeginSessionW( pconn->inst, &pconn->sesidSystem, NULL, NULL ) );

    err = JetAttachDatabaseW( pconn->sesidSystem, g_wszDatabaseY, 0 );
    if ( err == JET_errFileNotFound )
    {
        err = JetCreateDatabaseW( pconn->sesidSystem, g_wszDatabaseY, NULL, &pconn->dbid, 0 );
    }
    else if ( err >= JET_errSuccess )
    {
        err = JetOpenDatabaseW( pconn->sesidSystem, g_wszDatabaseY, L"", &pconn->dbid, 0 );
    }
    Call( err  );

    wprintf( L"\tService Y: Attached (or Created) Database.\n" );



    if ( !fStartSvcWInstead )
    {

        err = JetEnableMultiInstanceW( g_rgsspDisagreeableGlobalParametersZ, _countof(g_rgsspDisagreeableGlobalParametersZ), &cSucceed );
        wprintf( L"\tService Z: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
        if ( err == JET_errSystemParamsAlreadySet )
            {
            wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
            assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersZ) );
            err = JET_errSuccess;
            }
        Call( err  );
        
        
    }
    else
    {


        assert( g_rgsspDisagreeableGlobalParametersW[0].paramid == JET_paramConfigStoreSpec );
        g_rgsspDisagreeableGlobalParametersW[0].sz = L"reg:HKLM\\SOFTWARE\\Microsoft\\Ese_SvcInitSample_NotGoingToHappen";
        wprintf( L"\tService W: Initializing off %ws\n", wszRegistryConfigPath );

        err = JetEnableMultiInstanceW( g_rgsspDisagreeableGlobalParametersW, _countof(g_rgsspDisagreeableGlobalParametersW), &cSucceed );
        wprintf( L"\tService W: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
        if ( err == JET_errSystemParamsAlreadySet )
        {
            wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
            assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersW) );
            err = JET_errSuccess;
        }
        Call( err  );

        
    }

    return err;

HandleError:

    if ( pconn->inst && pconn->inst != JET_instanceNil )
    {
        JetTerm( pconn->inst );
    }

    return err;
}




DWORD ConfigureEseRegistry( const WCHAR * const wszRegRoot )
    {
    HKEY    nthkeyRoot;
    HKEY    nthkeySub;
    DWORD   dwDisposition;
    DWORD   error;
    WCHAR   wszValue[10];

    const WCHAR * const wszRegRootRel = wcschr( wszRegRoot, L'\\' ) + 1;

    wprintf( L"Configuring Registry for sample Method B of concurrent svc init.\n" );
    wprintf( L"\tConfig Store Root: %ws.\n", wszRegRoot );


    assert( wszRegRootRel != (void*)2 );

    error = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                    wszRegRootRel,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &nthkeyRoot,
                    &dwDisposition );
    if ( error )
        {
        wprintf( L"Sample Failed: RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );


    error = RegCreateKeyExW( nthkeyRoot, JET_wszConfigStoreRelPathSysParamDefault, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &nthkeySub, &dwDisposition );
    if ( error )
        {
        wprintf( L"Failed RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );


    {
    ULONG ulParamId;
    WCHAR wszParamName[100];
    #define SetParamData( paramid )     ulParamId = paramid; StringCbPrintfW( wszParamName, sizeof(wszParamName), L#paramid );

    SetParamData( JET_paramConfiguration );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_configDynamicMediumMemory|JET_configMediumDiskFootprint );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    SetParamData( JET_paramMaxInstances );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 3 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    SetParamData( JET_paramDatabasePageSize );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 16 * 1024 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    }

    RegCloseKey( nthkeySub );


    

#ifdef DEBUG

    {
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_bitConfigStorePopulateControlOn );

    error = RegSetValueExW( nthkeyRoot, JET_wszConfigStorePopulateControl, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    if ( error )
        {
        wprintf( L"Failed RegSetValueExW( %ws \\ %ws ) -> %d\n", wszRegRootRel, JET_wszConfigStorePopulateControl, error );
        assert( !"Failed RegSetValueExW()" );
        return error;
        }
    }

#endif


    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_bitConfigStoreReadControlDefault );
    
    error = RegSetValueExW( nthkeyRoot, JET_wszConfigStoreReadControl, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    if ( error )
        {
        wprintf( L"Failed RegSetValueExW( %ws \\ %ws ) -> %d\n", wszRegRootRel, JET_wszConfigStoreReadControl, error );
        assert( !"Failed RegSetValueExW" );
        return error;
        }


    RegCloseKey( nthkeyRoot );

    return ERROR_SUCCESS;
    }

DWORD ConfigureEseRegistryForInst( const WCHAR * const wszRegRoot )
    {
    HKEY    nthkeyRoot;
    HKEY    nthkeySub;
    DWORD   dwDisposition;
    DWORD   error;
    WCHAR   wszValue[10];

    const WCHAR * const wszRegRootRel = wcschr( wszRegRoot, L'\\' ) + 1;    // skip past "HKEY_LOCAL_MACHINE\" ...

    wprintf( L"Configuring Registry for Svc Y Instance.\n" );
    wprintf( L"\tConfig Store Root: %ws.\n", wszRegRoot );


    assert( wszRegRootRel != (void*)2 );

    error = RegCreateKeyExW( HKEY_LOCAL_MACHINE,
                    wszRegRootRel,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &nthkeyRoot,
                    &dwDisposition );
    if ( error )
        {
        wprintf( L"Sample Failed: RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );


    error = RegCreateKeyExW( nthkeyRoot, JET_wszConfigStoreRelPathSysParamDefault, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &nthkeySub, &dwDisposition );
    if ( error )
        {
        wprintf( L"Failed RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );


    {

    ULONG ulParamId;
    WCHAR wszParamName[100];
    #define SetParamData( paramid )     ulParamId = paramid; StringCbPrintfW( wszParamName, sizeof(wszParamName), L#paramid );

    SetParamData( JET_paramSystemPath );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L".\\DirY", (ULONG)CbOfWszIncNull( L".\\DirY" ) );

    SetParamData( JET_paramLogFilePath );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L".\\DirY", (ULONG)CbOfWszIncNull( L".\\DirY" ) );

    SetParamData( JET_paramTempPath );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L".\\DirY", (ULONG)CbOfWszIncNull( L".\\DirY" ) );

    SetParamData( JET_paramCheckpointDepthMax );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 5 * 1024 * 1024 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    SetParamData( JET_paramLogFileSize );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 1024 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    SetParamData( JET_paramBaseName );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L"yyy", (ULONG)CbOfWszIncNull( L"yyy" ) );
    }

    RegCloseKey( nthkeySub );


#ifdef DEBUG

    {
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_bitConfigStorePopulateControlOn );

    error = RegSetValueExW( nthkeyRoot, JET_wszConfigStorePopulateControl, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    if ( error )
        {
        wprintf( L"Failed RegSetValueExW( %ws \\ %ws ) -> %d\n", wszRegRootRel, JET_wszConfigStorePopulateControl, error );
        assert( !"Failed RegSetValueExW()" );
        return error;
        }
    }

#endif


    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_bitConfigStoreReadControlDefault );
    
    error = RegSetValueExW( nthkeyRoot, JET_wszConfigStoreReadControl, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    if ( error )
        {
        wprintf( L"Failed RegSetValueExW( %ws \\ %ws ) -> %d\n", wszRegRootRel, JET_wszConfigStoreReadControl, error );
        assert( !"Failed RegSetValueExW" );
        return error;
        }


    RegCloseKey( nthkeyRoot );

    return ERROR_SUCCESS;
    }

