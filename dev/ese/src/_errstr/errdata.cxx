// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "sync.hxx"
#include "types.hxx"

#include "jet.h"

#include "errdata.hxx"





#define ERR_DATA_DEFN( flags, cat, errNum, errSym, errStr ) { flags, errNum, errSym, errStr, JET_errcat ## cat },

#define EXTERNAL_ERR( errNum, cat, errSym )     ERR_DATA_DEFN( fErrExternal | fErrErr,  cat, errNum, errSym, #errSym )
#define EXTERNAL_WRN( errNum, cat, errSym )     ERR_DATA_DEFN( fErrExternal | fErrWrn,  cat, errNum, errSym, #errSym )

#define INTERNAL_ERR( errNum, cat, errSym )     ERR_DATA_DEFN( fErrInternal | fErrErr,  cat, errNum, errSym, #errSym )
#define INTERNAL_WRN( errNum, cat, errSym )     ERR_DATA_DEFN( fErrInternal | fErrWrn,  cat, errNum, errSym, #errSym )

#define OBSOLETE_ERR( errNum, cat, errSym )     ERR_DATA_DEFN( fErrObsolete | fErrErr,  cat, errNum, errSym, #errSym )
#define OBSOLETE_WRN( errNum, cat, errSym )     ERR_DATA_DEFN( fErrObsolete | fErrWrn,  cat, errNum, errSym, #errSym )


static ErrData rgerrdata [] =
{
#include "errdata.txt"
};



const ErrData * PerrdataEntryI( __in const INT iEntry )
{
    if ( iEntry >= _countof(rgerrdata) )
    {
        return NULL;
    }

    return &( rgerrdata[iEntry] );
}


const ErrData * PerrdataLookupErrValue( __in const JET_ERR errLookup )
{
    for ( INT iEntry = 0; iEntry < _countof(rgerrdata); iEntry++ )
    {
        if ( rgerrdata[iEntry].errSymbol == errLookup )
        {
            return &( rgerrdata[iEntry] );
        }
    }

    return NULL;
}

