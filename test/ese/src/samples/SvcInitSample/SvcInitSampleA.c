// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  SvcInitSampleA.c : Application Sample of ESE
//

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <strsafe.h>

#include <esent.h>

#include "SvcInit.h"

//  The below code is meant to by copied, but review, remove and address all the code 
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

//  ---------------------------------------------------------------------------------
//      <Start Sample>
//  ---------------------------------------------------------------------------------

//
//      Global Configuration
//

//
//      Global State (for Method A)
//

//  Service X's State and Config variables
//
static const WCHAR * const      g_wszDatabaseX = L".\\DirX\\SvcDbX.edb";
CONN_DB_STATE                   g_dbStateSvcX;
static JET_SETSYSPARAM_W        g_rgsspAgreedUponGlobalParametersX[] = {
    //  Warning: This list must agree with Service Y's list, or there will be a conflict for the
    //  second service to start.
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    { JET_paramDatabasePageSize,        16 * 1024, NULL,    JET_wrnNyi },
    //  ESE_CLIENT_TODO: This list of _global_ parameters should be configured to your 
    //  applications' requirements.
};

//  Service Y's State and Config variables
//
static const WCHAR * const      g_wszDatabaseY = L".\\DirY\\SvcDbY.edb";
CONN_DB_STATE                   g_dbStateSvcY;
static JET_SETSYSPARAM_W        g_rgsspAgreedUponGlobalParametersY[] = {
    //  Warning: This list must agree with Service X's list, or there will be a conflict for the
    //  second service to start.
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    { JET_paramDatabasePageSize,        16 * 1024, NULL,    JET_wrnNyi },
};

//  Service Z's State and Config variables
//

static const WCHAR * const      g_wszDatabaseZ = L".\\DirZ\\SvcDbZ.edb";
CONN_DB_STATE                   g_dbStateSvcZ;
static JET_SETSYSPARAM_W        g_rgsspDisagreeableGlobalParametersZ[] = {
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    //  This next param conflicts
    { JET_paramDatabasePageSize,        32 * 1024, NULL,    JET_wrnNyi },
    { JET_paramEnableFileCache,         FALSE, NULL,        JET_wrnNyi },
    { JET_paramEnableViewCache,         FALSE, NULL,        JET_wrnNyi },
};


//  --------------------------------------------------------------------------------
//          Begin the core sample
//

//
//      Method A) JetEnableMultiInstance() common parameter set method for global config coordination.
//

JET_ERR ErrEnableMultiInstStartSample()
{
    JET_ERR         err = JET_errSuccess;
    ULONG           cSucceed;
    CONN_DB_STATE * pconn = NULL;


    //  --------------------------------------------------------------------------------------------------
    //              Service X   
    //  --------------------------------------------------------------------------------------------------

    //
    //          Configure Global Parameters
    //
    //  Listed here for clarity, but I would isolate this into ErrStartAndConfigureEseEngine() routine.

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

    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramSystemPath, 0, L".\\DirY" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFilePath, 0, L".\\DirY" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramTempPath, 0, L".\\DirY\\" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramCheckpointDepthMax, 5 * 1024 * 1024, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFileSize, 1024, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramBaseName, 0, L"yyy" ) );

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
    assert( err != JET_errSystemParamsDisagreeable );
    //  Note: This call actually will fail ...
    Call( err /* Check any remaining error of JetEnableMultiInstanceW() */ );

    /*      stdout for SvcInitSample.exe JetEnableMultiInstance

    Begining SvcInit Sample (argc = 2, JetEnableMultiInstance)
            Service X: JetEnableMultiInstance() -> 0 / 3
            Service X: Instance created and inst specific params set.
            Service X: Initialized Inst (and Transaction Log).
            Service X: Attached (or Created) Database.
            Service Y: JetEnableMultiInstance() -> -1082 / 3
                    JET_errSystemParamsAlreadySet returned, means 3 parameters were already configured correctly for us.
            Service Y: Instance created and inst specific params set.
            Service Y: Initialized Inst (and Transaction Log).
            Service Y: Attached (or Created) Database.
            Service Z: JetEnableMultiInstance() -> -1087 / 2
    Failed: Call( err )
    With err = -1087 @ line 272 (in c:\src\e16\esereg\sources\test\ese\src\samples\svcinitsample\svcinitsamplea.c)

    Sample done.

    */

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

