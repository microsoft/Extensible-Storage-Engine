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
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    { JET_paramDatabasePageSize,        16 * 1024, NULL,    JET_wrnNyi },
};

static const WCHAR * const      g_wszDatabaseY = L".\\DirY\\SvcDbY.edb";
CONN_DB_STATE                   g_dbStateSvcY;
static JET_SETSYSPARAM_W        g_rgsspAgreedUponGlobalParametersY[] = {
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    { JET_paramDatabasePageSize,        16 * 1024, NULL,    JET_wrnNyi },
};


static const WCHAR * const      g_wszDatabaseZ = L".\\DirZ\\SvcDbZ.edb";
CONN_DB_STATE                   g_dbStateSvcZ;
static JET_SETSYSPARAM_W        g_rgsspDisagreeableGlobalParametersZ[] = {
    { JET_paramConfiguration,           JET_configDynamicMediumMemory|JET_configMediumDiskFootprint, NULL,  JET_wrnNyi },
    { JET_paramMaxInstances,            3, NULL,            JET_wrnNyi },
    { JET_paramDatabasePageSize,        32 * 1024, NULL,    JET_wrnNyi },
    { JET_paramEnableFileCache,         FALSE, NULL,        JET_wrnNyi },
    { JET_paramEnableViewCache,         FALSE, NULL,        JET_wrnNyi },
};




JET_ERR ErrEnableMultiInstStartSample()
{
    JET_ERR         err = JET_errSuccess;
    ULONG           cSucceed;
    CONN_DB_STATE * pconn = NULL;




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

    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramSystemPath, 0, L".\\DirY" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFilePath, 0, L".\\DirY" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramTempPath, 0, L".\\DirY\\" ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramCheckpointDepthMax, 5 * 1024 * 1024, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramLogFileSize, 1024, NULL ) );
    Call( JetSetSystemParameterW( &pconn->inst, JET_sesidNil, JET_paramBaseName, 0, L"yyy" ) );

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




    err = JetEnableMultiInstanceW( g_rgsspDisagreeableGlobalParametersZ, _countof(g_rgsspDisagreeableGlobalParametersZ), &cSucceed );
    wprintf( L"\tService Z: JetEnableMultiInstance() -> %d / %d\n", err, cSucceed );
    if ( err == JET_errSystemParamsAlreadySet )
        {
        wprintf( L"\t\tJET_errSystemParamsAlreadySet returned, means %d parameters were already configured correctly for us.\n", cSucceed );
        assert( cSucceed == _countof(g_rgsspAgreedUponGlobalParametersZ) );
        err = JET_errSuccess;
        }
    assert( err != JET_errSystemParamsDisagreeable );
    Call( err  );

    

    return err;


HandleError:

    if ( pconn->inst && pconn->inst != JET_instanceNil )
    {
        JetTerm( pconn->inst );
    }

    return err;
}


