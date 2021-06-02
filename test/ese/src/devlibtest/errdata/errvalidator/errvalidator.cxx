// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "errvalidator.hxx"

#include <algorithm>

using namespace std;

#ifdef BUILD_ENV_IS_EX
#pragma warning(disable:22018)  //  Ex12 RTM:  the version of strsafe.h we use has 22018 warnings we can't control
#endif  //  BUILD_ENV_IS_EX
#include <strsafe.h>
#ifdef BUILD_ENV_IS_EX
#pragma warning(default:22018)
#endif  //  BUILD_ENV_IS_EX

//  ================================================================
ULONG64 QWQueryPerformanceCounter()
// Returns the number of milliseconds, using QueryPerformanceFrequency()
// if possible.
//  ================================================================
{
    static bool fCheckedForQpfSupport       = false;
    static bool fUseQueryPerformanceCounter = false;
    static ULONG64  ulDivisor   = 1;
    LARGE_INTEGER   liResult;

    if ( !fCheckedForQpfSupport )
    {
        LARGE_INTEGER   liTemp;
        // Not synchronized, but that shouldn't be a problem.
        if ( QueryPerformanceFrequency( &liTemp ) )
        {
            // Since we want a result in milliseconds.
            ulDivisor = liTemp.QuadPart / 1000;
            fUseQueryPerformanceCounter = true;
        }
        fCheckedForQpfSupport = true;
    }

    if ( fUseQueryPerformanceCounter )
    {
        QueryPerformanceCounter( &liResult );
        return liResult.QuadPart / ulDivisor;
    }
    else
    {
        return (ULONG64) GetTickCount();
    }
}

//  ================================================================
DWORD DWGetTickCount()
//  ================================================================
{
    return GetTickCount();
}

#include "errdata.hxx"


JET_ERR ErrCheckErrorSpaceConsistent( __in const INT cerrData, __in const INT cerrStr )
{
    wprintf( L"Testing the entire -64k to 64k error space against errdata.txt and error string tables ..." );

    const JET_ERR errCheckMin = -0x10000;
    const JET_ERR errCheckMax = 0x10000;

    INT cerrDataCheck = 0;
    INT cerrStrCheck = 0;

    DWORD tickStart = DWGetTickCount();

    for( JET_ERR errCheck = errCheckMin; errCheck <= errCheckMax; errCheck++ )
    {
        const CHAR * pszError = NULL;
        const CHAR * pszErrorDesc = NULL;
        JetErrorToString( errCheck, &pszError, &pszErrorDesc );

        const ErrData * perrdata = PerrdataLookupErrValue( errCheck );

        if ( NULL != perrdata )
        {
            cerrDataCheck++;
        }

        if ( !FNullError( pszError ) )
        {
            cerrStrCheck++;
        }

        if ( !FNullError( pszError ) && NULL == perrdata )
        {
            wprintf( L"\n\tError expressed in error string table (from jethdr.w) and not in errdata.txt. errCheck = %d\n", errCheck );
            return errCodeInconsistency;
        }
        if ( perrdata != NULL && FNullError( pszError ) )
        {
            if ( perrdata->flags & fErrExternal )
            {
                wprintf( L"\n\tFound error in errdata.txt and none in error string AND error marked external! errCheck = %d\n", errCheck );
                return errCodeInconsistency;
            }
        }
    }

    if ( cerrData != cerrDataCheck )
    {
        wprintf( L"\n\tError space count of errors and errdata.txt table enum count of errors don't agree! %d != %d\n", cerrData, cerrDataCheck );
        return errCodeInconsistency;
    }
    if ( cerrStr != cerrStrCheck )
    {
        wprintf( L"\n\tError space count of errors and string table enum count of errors don't agree! %d != %d\n", cerrStr, cerrStrCheck );
        return errCodeInconsistency;
    }

    DWORD dtickRun = DWGetTickCount() - tickStart;

    wprintf( L"Done (%d.%03d secs)\n", dtickRun / 1000, dtickRun % 1000 );
    return JET_errSuccess;
}

JET_ERR ErrCheckErrorDataSelfConsistent( __in const INT cerr )
{
    wprintf( L"Testing there no internal errdata self-inconsistencies ..." );

    for( INT ierr = 0; ierr < cerr; ierr++ )
    {
        const ErrData * perrdata = PerrdataEntryI( ierr );

        if ( perrdata->errOrdinal != perrdata->errSymbol )
        {
            wprintf( L"\n\t errOrdinal (%d) and errSymbol (%d / %hs) do not match!?  errdata.txt data is not right!",
                        perrdata->errOrdinal, perrdata->errSymbol, perrdata->szSymbol );
            return errCodeInconsistency;
        }

        if ( perrdata->errOrdinal < -4096 || perrdata->errOrdinal > 4096 )
        {
            wprintf( L"\n\terrdata.txt error value (%d) is outside of reasonable error range!", perrdata->errOrdinal );
            return errCodeInconsistency;
        }
        if ( perrdata->errOrdinal < -0xFFFF || perrdata->errOrdinal > 0xFFFF )
        {
            wprintf( L"\n\terrdata.txt error value (%d) is outside of acceptable error range!", perrdata->errOrdinal );
            return errCodeInconsistency;
        }

        if ( perrdata->flags & fErrErr )
        {
            if ( perrdata->errOrdinal != JET_errSuccess &&
                    perrdata->errOrdinal >= JET_errSuccess )
            {
                wprintf( L"\n\terrdata.txt error value (%d) is positive error value!  error values should be negative.", perrdata->errOrdinal );
                return errCodeInconsistency;
            }
        }
        else if ( perrdata->flags & fErrWrn )
        {
            if ( perrdata->errOrdinal != JET_wrnNyi &&
                perrdata->errOrdinal < JET_errSuccess )
            {
                wprintf( L"\n\terrdata.txt error value (%d) is negative warning value!  warning values should be positive.", perrdata->errOrdinal );
                return errCodeInconsistency;
            }
        }
        else
        {
            wprintf( L"\n\terrdata.txt error value (%d) is neither a warning or error!", perrdata->errOrdinal );
            return errCodeInconsistency;
        }

        const bool fHasExtPrefix = ( ( 0 == strncmp( perrdata->szSymbol, "JET_err", strlen( "JET_err" ) ) ) ||
                                        ( 0 == strncmp( perrdata->szSymbol, "JET_wrn", strlen( "JET_wrn" ) ) ) );

        if ( ( perrdata->flags & fErrInternal ) &&
                fHasExtPrefix &&
                perrdata->errOrdinal != JET_errCannotLogDuringRecoveryRedo &&
                perrdata->errOrdinal != JET_errVersionStoreEntryTooBig &&
                perrdata->errOrdinal != JET_errPreviousVersion )
        {
            wprintf( L"\n\terrdata.txt error value (%d) has external JET_err form name but is internal error!", perrdata->errOrdinal );
            return errCodeInconsistency;
        }
    }

    wprintf( L"Done\n" );
    return JET_errSuccess;
}


JET_ERR ErrCheckErrDataErrorsAreInOrder( __in const INT cerr )
{
    wprintf( L"Testing all errors are in-order on the errdata.txt ..." );

    for( INT ierr = 1; ierr < cerr; ierr++ )
    {
        const ErrData * perrdataPre = PerrdataEntryI( ierr - 1 );
        const ErrData * perrdataPost = PerrdataEntryI( ierr );


        if ( abs( perrdataPre->errOrdinal ) >= abs( perrdataPost->errOrdinal ) )
        {
            wprintf( L"\n\tSome errors are not in pre-sorted order ... %d !< %d\n", perrdataPre->errOrdinal, perrdataPost->errOrdinal );
        }
    }

    wprintf( L"Done\n" );
    return JET_errSuccess;
}

JET_ERR ErrCheckNoAbsValueDups( __in const INT cerr )
{
    wprintf( L"Testing there no absolute value duplicates ..." );

    JET_ERR * prgerr = (JET_ERR*)_alloca( sizeof(JET_ERR) * cerr );

    for( INT ierr = 0; ierr < cerr; ierr++ )
    {
        const ErrData * perrdata = PerrdataEntryI( ierr );
        TestAssert( perrdata );
        prgerr[ierr] = abs( perrdata->errSymbol );
    }

    sort( prgerr, prgerr + cerr );

    for( INT ierr = 0; ierr < cerr; ierr++ )
    {
        if ( ierr > 0 )
        {
            if ( prgerr[ierr] < prgerr[ierr-1] )
            {
                wprintf( L"\n\tTest inconsistency, the sort() above didn't work!!!?" );
                return errCodeInconsistency;
            }
            if ( prgerr[ierr] == prgerr[ierr-1] )
            {
                wprintf( L"\n\tError set inconsistent!  Two errors have an inconsistent error ... abs value = %d", prgerr[ierr] );
                return errCodeInconsistency;
            }
        }
    }

    wprintf( L"Done.\n" );
    return JET_errSuccess;
}

JET_ERR ErrCheckExtErrorsInStrTable( __in const INT cerr )
{
    wprintf( L"Testing all externally marked errors have valid string table entries ..." );

    BOOL fWarning = FALSE;

    for( INT ierr = 0; ierr < cerr; ierr++ )
    {
        const ErrData * perrdata = PerrdataEntryI( ierr );

        if ( !( perrdata->flags & fErrExternal ) )
        {
            JET_ERR errUnexpected;
            if ( FErrStringToJetError( perrdata->szSymbol, &errUnexpected ) &&
                    !( perrdata->flags & fErrObsolete ) )
            {
                wprintf( L"\n\tCould retrieve an internal error from the string table.  Not bad, but unexpected. 0x%x err = %hs", perrdata->flags, perrdata->szSymbol );
                fWarning = TRUE;
                //return errCodeInconsistency;
            }
            continue;
        }

        JET_ERR errRetrieved;
        if ( !FErrStringToJetError( perrdata->szSymbol, &errRetrieved ) )
        {
            wprintf( L"\n\tCould not retreive an external error by error string (%hs) from the string table.", perrdata->szSymbol );
            return errCodeInconsistency;
        }

        if ( errRetrieved != perrdata->errSymbol )
        {
            wprintf( L"\n\tError retrieved by error string does not match the error value!" );
            return errCodeInconsistency;
        }

        const CHAR * pszErrorSymbolRetrieved = NULL;
        const CHAR * pszErrorDescRetrieved = NULL;
        JetErrorToString( perrdata->errSymbol, &pszErrorSymbolRetrieved, &pszErrorDescRetrieved );
        if ( FNullError( pszErrorSymbolRetrieved ) || NULL == pszErrorDescRetrieved )
        {
            wprintf( L"\n\tCould not retrieve an external error by error value (%d) from the string table.", perrdata->errSymbol );
            return errCodeInconsistency;
        }

        if ( strcmp( pszErrorSymbolRetrieved, perrdata->szSymbol ) )
        {
            wprintf( L"\n\tString error retrieved by error value from the string table does not match string from errdata.txt.  %hs != %hs.", pszErrorSymbolRetrieved, perrdata->szSymbol );
            return errCodeInconsistency;
        }

    }

    if ( fWarning )
    {
        wprintf( L"\n" );   // need a line return 
    }

    wprintf( L"Done\n" );
    return JET_errSuccess;
}

JET_ERR ErrCheckAllExtErrsHaveValidCategories( __in const INT cerr )
{
    wprintf( L"Testing all external errors have valid categories ..." );

    for( INT ierr = 0; ierr < cerr; ierr++ )
    {
        const ErrData * perrdata = PerrdataEntryI( ierr );

        if ( !( perrdata->flags & fErrExternal ) )
        {
            continue;
        }

        if ( JET_errcatUnknown == perrdata->errorCategory ||
                JET_errcatError == perrdata->errorCategory )
        {
            wprintf( L"\n\tExternal error (%d) has no fixed category\n", perrdata->errOrdinal );
            return errCodeInconsistency;
        }

        if ( perrdata->errorCategory >= JET_errcatMax )
        {
            wprintf( L"\n\tExternal error (%d) has category that is too big / larger than JET_errcatMax.\n", perrdata->errOrdinal );
            return errCodeInconsistency;
        }
    }

    wprintf( L"Done\n" );
    return JET_errSuccess;
}

JET_ERR ErrCheckExtErrorsInCatTable( __in const INT cerr )
{
    wprintf( L"Testing all externally marked errors have valid category table entries ..." );

    for( INT ierr = 0; ierr < cerr; ierr++ )
    {
        const ErrData * perrdata = PerrdataEntryI( ierr );

        if ( !( perrdata->flags & fErrExternal ) )
        {
            continue;
        }

        JET_ERRCAT errcat;
        const bool fLookup = FERRLookupErrorCategory( perrdata->errSymbol, &errcat );
        if ( !fLookup )
        {
            wprintf( L"\n\tCould not retreive an external error by error value(%d) from the cat table.", perrdata->errSymbol );
            return errCodeInconsistency;
        }

        if ( JET_errcatUnknown == errcat ||
                JET_errcatError == errcat )
        {
            wprintf( L"\n\tExternal error (%d) got bad category from the cat table.", perrdata->errSymbol );
            return errCodeInconsistency;
        }

        if ( perrdata->errorCategory != errcat )
        {
            wprintf( L"\n\tExternal error (%d) had mismatching categories from cat lookup vs. errdata.txt table.", perrdata->errSymbol );
            return errCodeInconsistency;
        }
    }

    wprintf( L"Done\n" );
    return JET_errSuccess;
}



//  simple testing program, no multiple tests, tests just run directly here, exit non-zero on fail ...

//  ================================================================
INT _cdecl main( INT argc, __in_ecount( argc ) char * argv[] )
//  ================================================================
{
    if( argc == 2
        && ( 0 == _stricmp( argv[1], "-h" )
            || 0 == _stricmp( argv[1], "/h" )
            || 0 == _stricmp( argv[1], "-?" )
            || 0 == _stricmp( argv[1], "/?" ) ) )
    {
        wprintf( L"This test takes no args, just run it." );
        return -1;
    }
    if( argc != 1 )
    {
        wprintf( L"This test takes no args, just run it." );
        return -1;
    }

    JET_ERR err = JET_errSuccess;

    DWORD tickStart = DWGetTickCount();

    //  first we count how many data table (errdata.txt) entries we have, and how many str atable entries we have ...

    INT cerrData = 0;
    //  som stats ...
    INT cerrExtErr = 0;
    INT cerrExtWrn = 0;
    INT cerrIntErr = 0;
    INT cerrIntWrn = 0;
    INT cerrObsErr = 0;
    INT cerrObsWrn = 0;
    INT rgcerrCategoryErrs[JET_errcatMax] = { 0 };
    INT rgcerrCategoryWrns[JET_errcatMax] = { 0 };
    const ErrData * perrdata = NULL;
    while ( perrdata = PerrdataEntryI( cerrData ) )
    {
        if ( perrdata->flags & fErrExternal )
        {
            if ( perrdata->flags & fErrErr )
            {
                cerrExtErr++;
            }
            else
            {
                TestAssert( perrdata->flags & fErrWrn );
                cerrExtWrn++;
            }
        }
        else if ( perrdata->flags & fErrInternal )
        {
            if ( perrdata->flags & fErrErr )
            {
                cerrIntErr++;
            }
            else
            {
                TestAssert( perrdata->flags & fErrWrn );
                cerrIntWrn++;
            }
        }
        else
        {
            TestAssert( perrdata->flags & fErrObsolete );
            if ( perrdata->flags & fErrErr )
            {
                cerrObsErr++;
            }
            else
            {
                TestAssert( perrdata->flags & fErrWrn );
                cerrObsWrn++;
            }
        }

        TestAssert( perrdata->errorCategory < _countof(rgcerrCategoryErrs) );
        if ( perrdata->flags & fErrErr )
        {
            rgcerrCategoryErrs[perrdata->errorCategory]++;
        }
        else
        {
            TestAssert( perrdata->flags & fErrWrn );
            rgcerrCategoryWrns[perrdata->errorCategory]++;
        }
        
        cerrData++;
    }
    TestAssert( cerrData == ( cerrExtErr + cerrExtWrn + cerrIntErr + cerrIntWrn + cerrObsErr + cerrObsWrn ) );

    INT cerrStr = 0;
    while ( PerrorEntryI( cerrStr ) )
    {
        cerrStr++;
    }

    wprintf( L"\n");
    wprintf( L"Test detected there are %d errors in the error string table of ESE.\n", cerrStr );
    wprintf( L"Test detected there are %d errors in the error data table of ESE.\n", cerrData );
    wprintf( L"\nDetails:\n" );
    wprintf( L"\tExternal:   %3d.\n", cerrExtErr + cerrExtWrn );
    wprintf( L"\t  Errors:       %3d.\n", cerrExtErr );
    wprintf( L"\t  Warnings:     %3d.\n", cerrExtWrn );
    wprintf( L"\tInternal:   %3d.\n", cerrIntErr + cerrIntWrn );
    wprintf( L"\t  Errors:       %3d.\n", cerrIntErr );
    wprintf( L"\t  Warnings:     %3d.\n", cerrIntWrn );
    wprintf( L"\tObsolete:   %3d.\n", cerrObsErr + cerrObsWrn );
    wprintf( L"\t  Errors:       %3d.\n", cerrObsErr );
    wprintf( L"\t  Warnings:     %3d.\n", cerrObsWrn );
    wprintf( L"\tTotal:      %3d.\n", cerrData );
    wprintf( L"\t  Errors:       %3d.\n", cerrExtErr + cerrIntErr + cerrObsErr );
    wprintf( L"\t  Warnings:     %3d.\n", cerrExtWrn + cerrIntWrn + cerrObsWrn );
    wprintf( L"\n" );
    wprintf( L"Category Stats:        errs wrns\n" );
    for( JET_ERRCAT errcat = JET_errcatUnknown; errcat < (JET_ERRCAT)_countof(rgcerrCategoryErrs); errcat = JET_ERRCAT( errcat + 1 ) )
    {
    wprintf( L"\t  errcat[%2d]:   %3d  %3d\n", errcat, rgcerrCategoryErrs[errcat], rgcerrCategoryWrns[errcat] );
    }
    wprintf( L"\n" );

    #define FailTestSuite( func )                       \
        if ( err = func )                               \
            {                                           \
            wprintf( L"Test %hs failed!\n", #func );    \
            return err;                                 \
        }

    //  run various test cases against the error (data, string, and category) tables

    FailTestSuite( ErrCheckErrorSpaceConsistent( cerrData, cerrStr ) );

    FailTestSuite( ErrCheckErrorDataSelfConsistent( cerrData ) );

    FailTestSuite( ErrCheckErrDataErrorsAreInOrder( cerrData ) );

    FailTestSuite( ErrCheckNoAbsValueDups( cerrData ) );

    FailTestSuite( ErrCheckExtErrorsInStrTable( cerrData ) );

    FailTestSuite( ErrCheckAllExtErrsHaveValidCategories( cerrData ) );

    FailTestSuite( ErrCheckExtErrorsInCatTable( cerrData ) );


    DWORD dtickRun = DWGetTickCount() - tickStart;

    wprintf( L"\nTesting Finished (%d.%03d secs)\n", dtickRun / 1000, dtickRun % 1000 );

    return err;
}


