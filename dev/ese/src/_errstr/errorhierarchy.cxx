// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "sync.hxx"
#include "types.hxx"
#include "error.hxx"

#include "jet.h"
#include "types.hxx"

#include <algorithm>

using namespace std;








struct ErrorToCategory
{
    JET_ERR     error;
    JET_ERRCAT  errorCategory;
};

#define EXTERNAL_ERR(errNum, cat, err) { err, JET_errcat ## cat },
#define EXTERNAL_WRN EXTERNAL_ERR
#define INTERNAL_ERR EXTERNAL_ERR
#define INTERNAL_WRN EXTERNAL_ERR
#define OBSOLETE_ERR EXTERNAL_ERR
#define OBSOLETE_WRN EXTERNAL_ERR


const static ErrorToCategory rgerrorCategories[] =
{
#include "errdata.txt"
};

class FIND_ERRCAT
{
    public:
        FIND_ERRCAT( JET_ERR err ) : m_err( err ) {}
        bool operator()( const ErrorToCategory& errorCat ) { return m_err == errorCat.error; }

    private:
        const JET_ERR m_err;
};

bool FERRLookupErrorCategory(
    __in const JET_ERR errLookup,
    __out JET_ERRCAT* perrortype
)
{
    if ( !perrortype )
    {
        return false;
    }

    const ErrorToCategory * const perrorCat = find_if( rgerrorCategories, rgerrorCategories + _countof( rgerrorCategories ), FIND_ERRCAT( errLookup ) );
    if( ( rgerrorCategories + _countof( rgerrorCategories ) ) != perrorCat )
    {
        *perrortype = perrorCat->errorCategory;
    }
    else
    {
        return false;
    }

    return true;
}

struct CategoryToHierarchy
{
    JET_ERRCAT  errorCategory;
    BYTE        errorHierarchy[8];
};

#define HIERARCHY8( hier1, hier2, hier3, hier4, hier5, hier6, hier7, hier8 ) { hier1, hier2, hier3, hier4, hier5, hier6, hier7, hier8 }
#define HIERARCHY7( hier1, hier2, hier3, hier4, hier5, hier6, hier7 ) HIERARCHY8( hier1, hier2, hier3, hier4, hier5, hier6, hier7, JET_errcatUnknown )
#define HIERARCHY6( hier1, hier2, hier3, hier4, hier5, hier6 ) HIERARCHY7( hier1, hier2, hier3, hier4, hier5, hier6, JET_errcatUnknown )
#define HIERARCHY5( hier1, hier2, hier3, hier4, hier5 ) HIERARCHY6( hier1, hier2, hier3, hier4, hier5, JET_errcatUnknown )
#define HIERARCHY4( hier1, hier2, hier3, hier4 ) HIERARCHY5( hier1, hier2, hier3, hier4, JET_errcatUnknown )
#define HIERARCHY3( hier1, hier2, hier3 ) HIERARCHY4( hier1, hier2, hier3, JET_errcatUnknown )
#define HIERARCHY2( hier1, hier2 ) HIERARCHY3( hier1, hier2, JET_errcatUnknown )
#define HIERARCHY1( hier1 ) HIERARCHY2( hier1, JET_errcatUnknown )

LOCAL const CategoryToHierarchy rgerrorHierarchies[] =
{
    { JET_errcatUnknown, HIERARCHY1( JET_errcatUnknown ) },
    { JET_errcatError, HIERARCHY1( JET_errcatError ) },
        { JET_errcatOperation, HIERARCHY2( JET_errcatError, JET_errcatOperation ) },
            { JET_errcatFatal, HIERARCHY3( JET_errcatError, JET_errcatOperation, JET_errcatFatal ) },
            { JET_errcatIO, HIERARCHY3( JET_errcatError, JET_errcatOperation, JET_errcatIO ) },
            { JET_errcatResource, HIERARCHY3( JET_errcatError, JET_errcatOperation, JET_errcatResource ) },
                { JET_errcatMemory, HIERARCHY4( JET_errcatError, JET_errcatOperation, JET_errcatResource, JET_errcatMemory ) },
                { JET_errcatQuota, HIERARCHY4( JET_errcatError, JET_errcatOperation, JET_errcatResource, JET_errcatQuota ) },
                { JET_errcatDisk, HIERARCHY4( JET_errcatError, JET_errcatOperation, JET_errcatResource, JET_errcatDisk ) },
        { JET_errcatData, HIERARCHY2( JET_errcatError, JET_errcatData ) },
            { JET_errcatCorruption, HIERARCHY3( JET_errcatError, JET_errcatData, JET_errcatCorruption ) },
            { JET_errcatInconsistent, HIERARCHY3( JET_errcatError, JET_errcatData, JET_errcatInconsistent ) },
            { JET_errcatFragmentation, HIERARCHY3( JET_errcatError, JET_errcatData, JET_errcatFragmentation) },
        { JET_errcatApi, HIERARCHY2( JET_errcatError, JET_errcatApi ) },
            { JET_errcatUsage, HIERARCHY3( JET_errcatError, JET_errcatApi, JET_errcatUsage ) },
            { JET_errcatState, HIERARCHY3( JET_errcatError, JET_errcatApi, JET_errcatState ) },
            { JET_errcatObsolete, HIERARCHY3( JET_errcatError, JET_errcatApi, JET_errcatObsolete ) },
};

C_ASSERT( JET_errcatMax == _countof( rgerrorHierarchies ) );

bool FERRLookupErrorHierarchy(
    __in JET_ERRCAT             errortype,
    __out_bcount(8) BYTE* const pbHierarchy
)
{
    Assert( errortype < _countof( rgerrorHierarchies ) );

    if ( errortype >= _countof( rgerrorHierarchies ) )
    {
        return false;
    }

    const CategoryToHierarchy* perrorHierarchy = &rgerrorHierarchies[ errortype ];

    Assert( errortype == perrorHierarchy->errorCategory );

    C_ASSERT( 8 == sizeof( perrorHierarchy->errorHierarchy ) );
    memcpy( pbHierarchy, &perrorHierarchy->errorHierarchy, sizeof( perrorHierarchy->errorHierarchy ) );

    return true;
}

JET_ERRCAT ErrcatERRLeastSpecific( const JET_ERR err )
{
    JET_ERRCAT errCat = JET_errcatUnknown;
    BYTE rgCategoricalHierarchy[8] = { 0 };

    if ( !FERRLookupErrorCategory( err, &errCat ) )
    {
        AssertSz( fFalse, "Who is passing in an error that we don't understand (%d)", err );
    }
    if ( !FERRLookupErrorHierarchy( errCat, rgCategoricalHierarchy ) )
    {
        AssertSz( fFalse, "How are we getting a category we can't lookup (%d)", errCat );
    }

    Assert( rgCategoricalHierarchy[0] == JET_errcatError );
    Assert( rgCategoricalHierarchy[1] != JET_errcatError );

    errCat = (JET_ERRCAT)rgCategoricalHierarchy[1];

    Assert( errCat != JET_errcatUnknown );

    Expected( errCat != JET_errcatApi );

    return errCat;
}

