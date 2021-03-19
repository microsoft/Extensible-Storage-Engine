// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//  Access to the ERR data table
//

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

const ErrData * PerrdataEntryI( _In_ const INT iEntry );
const ErrData * PerrdataLookupErrValue( _In_ const JET_ERR errLookup );

//  Access to the error message table
//

struct ERRORMSGDATA
{
    JET_ERR      err;
    const char * szError;
    const char * szErrorText;
};

const ERRORMSGDATA * PerrorEntryI( _In_ INT iEntry );

bool FErrStringToJetError( const char * szError, JET_ERR * perr );
#ifndef MINIMAL_FUNCTIONALITY
void JetErrorToString( JET_ERR err, const char **szError, const char **szErrorText );
#endif

const char szUnknownError[]             = "Unknown Error";
#define FNullError( szErr )     ( NULL == szErr || ( 0 == strcmp( szErr, szUnknownError ) ) )

//  Access to the hierarchy table
//

bool FERRLookupErrorHierarchy(
    _In_ JET_ERRCAT             errortype,
    __out_bcount(8) BYTE* const pbHierarchy
);

bool FERRLookupErrorCategory(
    _In_ const JET_ERR errLookup,
    _Out_ JET_ERRCAT* perrortype
);


