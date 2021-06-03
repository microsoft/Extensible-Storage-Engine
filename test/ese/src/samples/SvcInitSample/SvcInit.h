// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//  SvcInit.h : Application Sample of ESE
//

//
//              Multi-Service Init and Parameter Configuration Sample
//

//  ---------------------------------------------------------------------------------
//      <Start Sample>
//  ---------------------------------------------------------------------------------

//
//      Global State Types
//

// SOMEONE rename sesidSystem to sesid ... 
typedef struct {
    JET_INSTANCE    inst;
    JET_DBID        dbid;
    JET_SESID       sesidSystem;
} CONN_DB_STATE;

//  ---------------------------------------------------------------------------------
//      <End Sample>
//  ---------------------------------------------------------------------------------

//
//  Helpers
//

// SOMEONE define assert

//  Gets the size of a string in bytes, including the bytes for the NUL terminating character.

#define CbOfWszIncNull( wszStr )            ( ( wcslen(wszStr) + 1 ) * sizeof(WCHAR) )

//  The constant checking of ESE errors every line can be quite cumbersome and distract from
//  the point of the sample code, so we use this relative simple macro that stores the function
//  call into the "err" variable, and jumps the a "HandleError" label if it fails.  This would
//  not be appropriate obviously for a code base that uses exceptions.

#define Call( funcCall )                                    \
    if ( ( err = funcCall ) < JET_errSuccess )              \
        {                                                   \
        wprintf( L"Failed: Call( %hs )\n",                  \
                    #funcCall );                            \
        wprintf( L"With err = %d @ line %d (in %hs)\n",     \
                    err, __LINE__, __FILE__ );              \
        goto HandleError;                                   \
        }

//  Just a simple little function that converts a strsafe API into a JET_err* type error for a 
//  consistent error space.

JET_ERR ErrFromStrsafeHr( const HRESULT hr );

//
//      Method A and Method B sample start functions
//

JET_ERR ErrEnableMultiInstStartSample();
JET_ERR ErrConfigStorePathStartSample( const WCHAR * const wszRegistryConfigPath, const INT fStartSvcWInstead );

