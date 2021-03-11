// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.




typedef struct {
    JET_INSTANCE    inst;
    JET_DBID        dbid;
    JET_SESID       sesidSystem;
} CONN_DB_STATE;





#define CbOfWszIncNull( wszStr )            ( ( wcslen(wszStr) + 1 ) * sizeof(WCHAR) )


#define Call( funcCall )                                    \
    if ( ( err = funcCall ) < JET_errSuccess )              \
        {                                                   \
        wprintf( L"Failed: Call( %hs )\n",                  \
                    #funcCall );                            \
        wprintf( L"With err = %d @ line %d (in %hs)\n",     \
                    err, __LINE__, __FILE__ );              \
        goto HandleError;                                   \
        }


JET_ERR ErrFromStrsafeHr( const HRESULT hr );


JET_ERR ErrEnableMultiInstStartSample();
JET_ERR ErrConfigStorePathStartSample( const WCHAR * const wszRegistryConfigPath, const INT fStartSvcWInstead );

