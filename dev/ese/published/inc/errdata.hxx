// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#define fErrErr     (0x01)
#define fErrWrn     (0x02)

#define fErrExternal    (0x04)
#define fErrInternal    (0x08)
#define fErrObsolete    (0x10)

struct ErrData
{
    DWORD       flags;
    JET_ERR     errOrdinal;
    JET_ERR     errSymbol;
    CHAR *      szSymbol;
    JET_ERRCAT  errorCategory;
};

const ErrData * PerrdataEntryI( __in const INT iEntry );
const ErrData * PerrdataLookupErrValue( __in const JET_ERR errLookup );


struct ERRORMSGDATA
{
    JET_ERR      err;
    const char * szError;
    const char * szErrorText;
};

const ERRORMSGDATA * PerrorEntryI( __in INT iEntry );

bool FErrStringToJetError( const char * szError, JET_ERR * perr );
#ifndef MINIMAL_FUNCTIONALITY
void JetErrorToString( JET_ERR err, const char **szError, const char **szErrorText );
#endif

const char szUnknownError[]             = "Unknown Error";
#define FNullError( szErr )     ( NULL == szErr || ( 0 == strcmp( szErr, szUnknownError ) ) )


bool FERRLookupErrorHierarchy(
    __in JET_ERRCAT             errortype,
    __out_bcount(8) BYTE* const pbHierarchy
);

bool FERRLookupErrorCategory(
    __in const JET_ERR errLookup,
    __out JET_ERRCAT* perrortype
);


