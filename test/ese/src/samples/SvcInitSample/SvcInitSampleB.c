// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  SvcInitSampleB.c : Application Sample of ESE
//

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>

#include <esent.h>

#include "SvcInit.h"

//  The below code is meant to by copied, but review, remove and address all comments 
//  starting with ESE_CLIENT_TODO.

//  Below we list three imaginary service init paths, that while all sequential, could 
//  be operated concurrently.  Also while the three service init paths are all in one
//  file it should be imagined these are in three separate unrelated code bases.
//
//  Service X and Service Y are two services that would be able to peacibly co-exist
//  together without any sort of configuration problems or service interruption.
//
//  Service Z is a bad-example case that will cause problems for X and Y if it inits
//  first, or have trouble initing itself if it is second (which is how the code is
//  laid out below).
//
//  Service W is a 2nd bad-example case that will cause problems for X and Y because
//  it tries to set the JET_paramConfigStoreSpec to a different path.


//  ---------------------------------------------------------------------------------
//      <Start Sample>
//  ---------------------------------------------------------------------------------

//
//      Global Configuration
//

//
//      Global State (for Method B)
//

//  Service X's State and Config variables
//
static const WCHAR * const      g_wszDatabaseX = L".\\DirX\\SvcDbX.edb";
CONN_DB_STATE                   g_dbStateSvcX;
static JET_SETSYSPARAM_W        g_rgsspAgreedUponGlobalParametersX[] = {
    //  The global param state now lives in the registry, for coordination with other services 
    //  and install / build / manifest decisions.
    { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL /* filled out in init */, JET_wrnNyi },
};


//  Service Y's State and Config variables
//
static const WCHAR * const      g_wszDatabaseY = L".\\DirY\\SvcDbY.edb";
CONN_DB_STATE                   g_dbStateSvcY;
static JET_SETSYSPARAM_W            g_rgsspAgreedUponGlobalParametersY[] = {
    //  The global param state now lives in the registry, for coordination with other services 
    //  and install / build / manifest decisions.
    { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL /* filled out in init */, JET_wrnNyi },
};


//  Service Z's State and Config variables
//

static const WCHAR * const      g_wszDatabaseZ = L".\\DirZ\\SvcDbZ.edb";
CONN_DB_STATE                   g_dbStateSvcZ;
static JET_SETSYSPARAM_W        g_rgsspDisagreeableGlobalParametersZ[] = {
    //  All these params conflict because we aren't setting the JET_paramConfigStoreSpec setting.
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    //  This next param conflicts
    { JET_paramDatabasePageSize,        32 * 1024, NULL,    JET_wrnNyi },
    { JET_paramEnableFileCache,         FALSE, NULL,            JET_wrnNyi },
    { JET_paramEnableViewCache,         FALSE, NULL,        JET_wrnNyi },
//  //  The global param state now lives in the registry.
//  { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL /* filled out in init */, JET_wrnNyi },
};


//  Service W's State and Config variables
//

static const WCHAR * const      g_wszDatabaseW = L".\\DirW\\SvcDbZ.edb";
CONN_DB_STATE                   g_dbStateSvcW;
static JET_SETSYSPARAM_W        g_rgsspDisagreeableGlobalParametersW[] = {
    { JET_paramConfigStoreSpec, (JET_API_PTR)NULL, NULL /* filled out in init */, JET_wrnNyi },
};


//  Configure registry helper declarations ...
//

DWORD ConfigureEseRegistry( const WCHAR * const wszRegRoot );
DWORD ConfigureEseRegistryForInst( const WCHAR * const wszRegRoot );


//  --------------------------------------------------------------------------------
//          Begin the core sample
//

//
//      Method B) JET_paramConfigStoreSpec registry method for global config coordination.
//

JET_ERR ErrConfigStorePathStartSample( const WCHAR * const wszRegistryConfigPath, const INT fStartSvcWInstead  )
{
    JET_ERR         err = JET_errSuccess;
    ULONG           cSucceed;
    CONN_DB_STATE * pconn = NULL;

    //  NOTE: This is not the expected model for apps to code this ... it is expected that thier
    //  install routines or the windows manifest files would configure this properly.
    //  
    ConfigureEseRegistry( wszRegistryConfigPath );

    //  --------------------------------------------------------------------------------------------------
    //              Service X   
    //  --------------------------------------------------------------------------------------------------

    //
    //          Configure Global Parameters
    //
    //  Listed here for clarity, but I would isolate this into ErrStartAndConfigureEseEngine() routine.

    assert( g_rgsspAgreedUponGlobalParametersX[0].paramid == JET_paramConfigStoreSpec );
    g_rgsspAgreedUponGlobalParametersX[0].sz = wszRegistryConfigPath;
    wprintf( L"\tService X: Initializing off %ws\n", wszRegistryConfigPath );

    err = JetEnableMultiInstanceW( g_rgsspAgreedUponGlobalParametersX, _countof(g_rgsspAgreedUponGlobalParametersX), &cSucceed );
    wprintf( L"\tService X: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
    if ( err == JET_errSystemParamsAlreadySet )
    {
        //  Note: Backwards compatibility note - In Win 8.1 and prior, cSucceed would not be set when this 
        //  error code is returned.
        wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
        assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersX) );
        err = JET_errSuccess;
    }
    //  to speed early detection of conflicting parameter sets / services, we'll assert we didn't hit this error.
    assert( err != JET_errSystemParamsDisagreeable );
    Call( err /* Check any remaining error of JetEnableMultiInstanceW() */ );

    //
    //          Create and Configure Inst Parameters
    //
    //  Listed here for clarity, but I would isolate this into ErrCreateAndConfigureInstance( &g_dbStateSvcX ) routine.
    pconn = &g_dbStateSvcX;

    Call( JetCreateInstance2W( &pconn->inst, L"ESE_CLIENT_TODO_FRIENDLY_X", L"ESE_CLIENT_TODO_FRIENDLY_X", JET_bitNil ) );

    (void)CreateDirectoryW( L".\\DirX", 0 );    //  this fails on 2nd run, that's fine as the logs/DB now exist

    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramSystemPath, 0, L".\\DirX" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFilePath, 0, L".\\DirX" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramTempPath, 0, L".\\DirX\\" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramCheckpointDepthMax, 5 * 1024 * 1024, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFileSize, 512, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramBaseName, 0, L"xxx" ) );
    //  ESE_CLIENT_TODO: The client should trim the above list and/or add any other instance specific
    //  system parameters that the client requires to run effectively.  It is preferrable to add more
    //  parameters here, rather than to g_rgsspAgreedUponGlobalParametersX.

    wprintf( L"\tService X: Instance created and inst specific params set.\n" );

    //
    //          Start the ESE Instance (this accesses the Log+Database files)
    //
    //  Listed here for clarity, but I would isolate this into ErrStartInstAndDatabase( &g_dbStateSvcX ) routine.

    err = JetInit3W( &pconn->inst, NULL, JET_bitNil );
    if ( err < JET_errSuccess )
    {
        //  In an unfortunate API design, if JetInit() fails ESE frees the instance, even if the
        //  instance was created via JetCreateInstance*().
        pconn->inst = JET_instanceNil;
    }
    Call( err /* Check any remaining error of JetInit3W() */ );

    wprintf( L"\tService X: Initialized Inst (and Transaction Log).\n" );

    //  Need a session to attach or create a database.
    Call( JetBeginSessionW( pconn->inst, &pconn->sesidSystem, NULL, NULL ) );

    //  look for database (and create if not found)
    //
    err = JetAttachDatabaseW( pconn->sesidSystem, g_wszDatabaseX, 0 );
    if ( err == JET_errFileNotFound )
    {
        err = JetCreateDatabaseW( pconn->sesidSystem, g_wszDatabaseX, NULL, &pconn->dbid, 0 );
        //  ESE_CLIENT_TODO: Client should decide if they want to separate the DB creation from the
        //  main attach path completely or, embedded DB create inline.
        //  ESE_CLIENT_TODO: After create of course the client should begin a transaction and create 
        //  any initial DB schema the later client code will expect.
    }
    else if ( err >= JET_errSuccess )
    {
        err = JetOpenDatabaseW( pconn->sesidSystem, g_wszDatabaseX, L"", &pconn->dbid, 0 );
    }
    Call( err /* Check any remaining error of JetAttac|Create|OpenDatabase() */ );

    wprintf( L"\tService X: Attached (or Created) Database.\n" );

    //  Now the client is free / safe to open up additional worker sessions for regular DB activity.


    //  --------------------------------------------------------------------------------------------------
    //              Service Y   
    //  --------------------------------------------------------------------------------------------------

    assert( g_rgsspAgreedUponGlobalParametersY[0].paramid == JET_paramConfigStoreSpec );
    g_rgsspAgreedUponGlobalParametersY[0].sz = wszRegistryConfigPath;
    wprintf( L"\tService Y: Initializing off %ws\n", wszRegistryConfigPath );

    err = JetEnableMultiInstanceW( g_rgsspAgreedUponGlobalParametersY, _countof(g_rgsspAgreedUponGlobalParametersY), &cSucceed );
    wprintf( L"\tService Y: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
    if ( err == JET_errSystemParamsAlreadySet )
    {
        //  Note: Backwards compatibility note - In Win 8.1 and prior, cSucceed would not be set when this 
        //  error code is returned.
        wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
        assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersY) );
        err = JET_errSuccess;
    }
    //  to speed early detection of conflicting parameter sets / services, we'll assert we didn't hit this error.
    assert( err != JET_errSystemParamsDisagreeable );
    Call( err /* Check any remaining error of JetEnableMultiInstanceW() */ );

    //
    //          Create and Configure Inst Parameters
    //
    //  Listed here for clarity, but I would isolate this into ErrCreateAndConfigureInstance( &g_dbStateSvcY ) routine.
    pconn = &g_dbStateSvcY;

    Call( JetCreateInstance2W( &pconn->inst, L"ESE_CLIENT_TODO_FRIENDLY_Y", L"ESE_CLIENT_TODO_FRIENDLY_Y", JET_bitNil ) );

    (void)CreateDirectoryW( L".\\DirY", 0 );    //  this fails on 2nd run, that's fine as the logs/DB now exist
    
    {
    //  The Service Y, also has decided to keep it's instance specific parameter defaults in the registry
    //  as well, so setup the registry for that.  See ConfigureEseRegistryForInst() for how these parameters
    //  are set in this mode.
    WCHAR wszRegistryConfigPathInstY[260];
    Call( ErrFromStrsafeHr( StringCbPrintfW( wszRegistryConfigPathInstY, sizeof(wszRegistryConfigPathInstY), L"%ws_SvcY", wszRegistryConfigPath ) ) );
    ConfigureEseRegistryForInst( wszRegistryConfigPathInstY );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramConfigStoreSpec, 0, wszRegistryConfigPathInstY ) );
    }

    wprintf( L"\tService Y: Instance created and inst specific params set.\n" );

    //
    //          Start the ESE Instance (this accesses the Log+Database files)
    //
    //  Listed here for clarity, but I would isolate this into ErrStartInstAndDatabase( &g_dbStateSvcY ) routine.

    err = JetInit3W( &pconn->inst, NULL, JET_bitNil );
    if ( err < JET_errSuccess )
    {
        //  In an unfortunate API design, if JetInit() fails ESE frees the instance, even if the
        //  instance was created via JetCreateInstance*().
        pconn->inst = JET_instanceNil;
    }
    Call( err /* Check any remaining error of JetInit3W() */ );

    wprintf( L"\tService Y: Initialized Inst (and Transaction Log).\n" );

    //  Need a session to attach or create a database.
    Call( JetBeginSessionW( pconn->inst, &pconn->sesidSystem, NULL, NULL ) );

    //  look for database (and create if not found)
    //
    err = JetAttachDatabaseW( pconn->sesidSystem, g_wszDatabaseY, 0 );
    if ( err == JET_errFileNotFound )
    {
        err = JetCreateDatabaseW( pconn->sesidSystem, g_wszDatabaseY, NULL, &pconn->dbid, 0 );
    }
    else if ( err >= JET_errSuccess )
    {
        err = JetOpenDatabaseW( pconn->sesidSystem, g_wszDatabaseY, L"", &pconn->dbid, 0 );
    }
    Call( err /* Check any remaining error of JetAttac|Create|OpenDatabase() */ );

    wprintf( L"\tService Y: Attached (or Created) Database.\n" );

    //  Now the client is free / safe to open up additional worker sessions for regular DB activity.


    if ( !fStartSvcWInstead )
    {
        //  --------------------------------------------------------------------------------------------------
        //              Service Z
        //  --------------------------------------------------------------------------------------------------

        err = JetEnableMultiInstanceW( g_rgsspDisagreeableGlobalParametersZ, _countof(g_rgsspDisagreeableGlobalParametersZ), &cSucceed );
        wprintf( L"\tService Z: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
        if ( err == JET_errSystemParamsAlreadySet )
            {
            wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
            assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersZ) );
            err = JET_errSuccess;
            }
        //  to speed early detection of conflicting parameter sets / services, we'll assert we didn't hit this error.
        //  NOTE: Of course since this is "the bad service", we have to disable this as this goes off.
        //assert( err != JET_errSystemParamsDisagreeable );
        //  Note: This call actually will fail ...
        Call( err /* Check any remaining error of JetEnableMultiInstanceW() */ );
        
        /*      stdout for SvcInitSample.exe JET_paramConfigStoreSpec
        
        Begining SvcInit Sample (argc = 2, JET_paramConfigStoreSpec)
        Configuring Registry for sample Method B of concurrent svc init.
                Config Store Root: reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default.
                Service X: Initializing off reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default
                Service X: JetEnableMultiInstance() -> 0 / 1
                Service X: Instance created and inst specific params set.
                Service X: Initialized Inst (and Transaction Log).
                Service X: Attached (or Created) Database.
                Service Y: Initializing off reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default
                Service Y: JetEnableMultiInstance() -> -1082 / 1
                        JET_errSystemParamsAlreadySet returned, means 1 parameters were already configured correctly for us.
        Configuring Registry for Svc Y Instance.
                Config Store Root: reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default_SvcY.
                Service Y: Instance created and inst specific params set.
                Service Y: Initialized Inst (and Transaction Log).
                Service Y: Attached (or Created) Database.
                Service Z: JetEnableMultiInstance() -> -1087 / 0
        Failed: Call( err )
        With err = -1087 @ line 308 (in c:\src\e16\esereg\sources\test\ese\src\samples\svcinitsample\svcinitsampleb.c)

        Sample done.
        */
    }
    else
    {
        //  --------------------------------------------------------------------------------------------------
        //              Service W
        //  --------------------------------------------------------------------------------------------------

        //
        //          Configure Global Parameters
        //
        //  Listed here for clarity, but I would isolate this into ErrStartAndConfigureEseEngine() routine.

        assert( g_rgsspDisagreeableGlobalParametersW[0].paramid == JET_paramConfigStoreSpec );
        //  Note: The path here is different than the above path!  So this will fail.
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
        //  Note: This call actually will fail ...
        Call( err /* Check any remaining error of JetEnableMultiInstanceW() */ );

        /*      stdout for SvcInitSample.exe JET_paramConfigStoreSpec2

        Begining SvcInit Sample (argc = 2, JET_paramConfigStoreSpec2)
        Configuring Registry for sample Method B of concurrent svc init.
                Config Store Root: reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default.
                Service X: Initializing off reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default
                Service X: JetEnableMultiInstance() -> 0 / 1
                Service X: Instance created and inst specific params set.
                Service X: Initialized Inst (and Transaction Log).
                Service X: Attached (or Created) Database.
                Service Y: Initializing off reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default
                Service Y: JetEnableMultiInstance() -> -1082 / 1
                        JET_errSystemParamsAlreadySet returned, means 1 parameters were already configured correctly for us.
        Configuring Registry for Svc Y Instance.
                Config Store Root: reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default_SvcY.
                Service Y: Instance created and inst specific params set.
                Service Y: Initialized Inst (and Transaction Log).
                Service Y: Attached (or Created) Database.
                Service W: Initializing off reg:HKLM\SOFTWARE\Microsoft\Ese_SvcInitSample_Default
                Service W: JetEnableMultiInstance() -> -1087 / 0
        Failed: Call( err )
        With err = -1087 @ line 360 (in c:\src\e16\esereg\sources\test\ese\src\samples\svcinitsample\svcinitsampleb.c)

        Sample done.
        */
    }

    return err;

HandleError:

    if ( pconn->inst && pconn->inst != JET_instanceNil )
    {
        JetTerm( pconn->inst );
    }

    return err;
}

//  ---------------------------------------------------------------------------------
//      <End Sample>
//  ---------------------------------------------------------------------------------


//  This is test code - it is not locked down properly for security purposes, nor does
//  it have stringent error checking or a strong failure contract.

DWORD ConfigureEseRegistry( const WCHAR * const wszRegRoot )
    {
    HKEY    nthkeyRoot;
    HKEY    nthkeySub;
    DWORD   dwDisposition;
    DWORD   error;
    WCHAR   wszValue[10];

    const WCHAR * const wszRegRootRel = wcschr( wszRegRoot, L'\\' ) + 1;    // skip past "HKEY_LOCAL_MACHINE\" ...

    wprintf( L"Configuring Registry for sample Method B of concurrent svc init.\n" );
    wprintf( L"\tConfig Store Root: %ws.\n", wszRegRoot );

    //
    //  Create the registry key that is the root of the ESE Config Store
    //

    assert( wszRegRootRel != (void*)2 );    // ((WCHAR*)NULL)++

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

    //
    //  Create the System Parameter's Default reg key so that we can set some system parameters
    //

    error = RegCreateKeyExW( nthkeyRoot, JET_wszConfigStoreRelPathSysParamDefault, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &nthkeySub, &dwDisposition );
    if ( error )
        {
        wprintf( L"Failed RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );

    //
    //  Set global system parameters into the registry.
    //

    {
    //  The first ulParamId while not used directly, effectively makes sure we don't mis-spell a 
    //  JET_param constant value, which would make it ineffective.
    ULONG ulParamId;
    WCHAR wszParamName[100];
    #define SetParamData( paramid )     ulParamId = paramid; StringCbPrintfW( wszParamName, sizeof(wszParamName), L#paramid );

    //  { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    SetParamData( JET_paramConfiguration );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_configDynamicMediumMemory|JET_configMediumDiskFootprint );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    //  { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    SetParamData( JET_paramMaxInstances );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 3 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    //  { JET_paramDatabasePageSize,        16 * 1024, NULL,    JET_wrnNyi },
    SetParamData( JET_paramDatabasePageSize );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 16 * 1024 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    }

    RegCloseKey( nthkeySub );

    //
    //  We don't have to create the System Parameter Override key
    //

    /*
    error = RegCreateKeyExW( nthkeyRoot, JET_wszConfigStoreRelPathSysParamOverride, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &nthkeySub, &dwDisposition );
    if ( error )
        {
        wprintf( L"Failed RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );
    RegCloseKey( nthkeySub );
    //*/

#ifdef DEBUG
    //
    //  This makes it much easier to see what registry settings ESE is using.
    //

    //*
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
    //*/
#endif

    //
    //  Finally, configure the Config Read Control - to make all the settings above effective.
    //

    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_bitConfigStoreReadControlDefault );
    
    error = RegSetValueExW( nthkeyRoot, JET_wszConfigStoreReadControl, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    if ( error )
        {
        wprintf( L"Failed RegSetValueExW( %ws \\ %ws ) -> %d\n", wszRegRootRel, JET_wszConfigStoreReadControl, error );
        assert( !"Failed RegSetValueExW" );
        return error;
        }

    //
    //  Cleanup
    //

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

    //
    //  Create the registry key that is the root of the ESE Config Store
    //

    assert( wszRegRootRel != (void*)2 );    // ((WCHAR*)NULL)++

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

    //
    //  Create the System Parameter's Default reg key so that we can set some system parameters
    //
    error = RegCreateKeyExW( nthkeyRoot, JET_wszConfigStoreRelPathSysParamDefault, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &nthkeySub, &dwDisposition );
    if ( error )
        {
        wprintf( L"Failed RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );

    //
    //  Set instance specifc system parameters into the registry.
    //
    {
    //  The first ulParamId while not used directly, effectively makes sure we don't mis-spell a 
    //  JET_param constant value, which would make it ineffective.
    ULONG ulParamId;
    WCHAR wszParamName[100];
    #define SetParamData( paramid )     ulParamId = paramid; StringCbPrintfW( wszParamName, sizeof(wszParamName), L#paramid );

    //err = JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramSystemPath, 0,  );
    SetParamData( JET_paramSystemPath );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L".\\DirY", (ULONG)CbOfWszIncNull( L".\\DirY" ) );

    //err = JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFilePath, 0, L".\\DirY" );
    SetParamData( JET_paramLogFilePath );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L".\\DirY", (ULONG)CbOfWszIncNull( L".\\DirY" ) );

    //err = JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramTempPath, 0, L".\\DirY\\" );
    SetParamData( JET_paramTempPath );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L".\\DirY", (ULONG)CbOfWszIncNull( L".\\DirY" ) );

    //err = JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramCheckpointDepthMax, 5 * 1024 * 1024, NULL );
    SetParamData( JET_paramCheckpointDepthMax );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 5 * 1024 * 1024 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    //err = JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFileSize, 512, NULL );
    SetParamData( JET_paramLogFileSize );
    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", 1024 );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );

    //err = JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramBaseName, 0, L"yyy" );
    SetParamData( JET_paramBaseName );
    error = RegSetValueExW( nthkeySub, wszParamName, 0, REG_SZ, (LPBYTE)L"yyy", (ULONG)CbOfWszIncNull( L"yyy" ) );
    }

    RegCloseKey( nthkeySub );

    //
    //  We don't have to create the System Parameter Override key
    //

    /*
    error = RegCreateKeyExW( nthkeyRoot, JET_wszConfigStoreRelPathSysParamOverride, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &nthkeySub, &dwDisposition );
    if ( error )
        {
        wprintf( L"Failed RegCreateKeyExW( %ws ) -> %d\n", wszRegRootRel, error );
        assert( !"Failed RegCreateKeyExW()" );
        return error;
        }
    assert( dwDisposition == REG_CREATED_NEW_KEY );
    RegCloseKey( nthkeySub );
    //*/

#ifdef DEBUG
    //
    //  This makes it much easier to see what registry settings ESE is using.
    //

    //*
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
    //*/
#endif

    //
    //  Finally, configure the Config Read Control - to make all the settings above effective.
    //

    StringCbPrintfW( wszValue, sizeof(wszValue), L"%d", JET_bitConfigStoreReadControlDefault );
    
    error = RegSetValueExW( nthkeyRoot, JET_wszConfigStoreReadControl, 0, REG_SZ, (LPBYTE)wszValue, (ULONG)CbOfWszIncNull( wszValue ) );
    if ( error )
        {
        wprintf( L"Failed RegSetValueExW( %ws \\ %ws ) -> %d\n", wszRegRootRel, JET_wszConfigStoreReadControl, error );
        assert( !"Failed RegSetValueExW" );
        return error;
        }

    //
    //  Cleanup
    //

    RegCloseKey( nthkeyRoot );

    return ERROR_SUCCESS;
    }

