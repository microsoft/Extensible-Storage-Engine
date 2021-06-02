// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// Formerly known as 'jettest.exe', this is used to execute unit tests
// that are embedded in ese.dll/esent.dll

#if defined(BUILD_ENV_IS_NT) || defined(BUILD_ENV_IS_WPHONE)
#include "esent_x.h"
#else
#include "jet.h"
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

void OSTestReportErr( LONG err, ULONG ulLine, const char *szFileName )
{
    printf( "error %ld at line %lu of %s\r\n", err, ulLine, szFileName );
}

void OSTestReportErr( ULONG ulLine, const char *szFileName )
{
    printf( "error at line %ld of %s\r\n", ulLine, szFileName );
}


//  This is OS Layer Unit test Call/CallJ macros, because sometimes we want
//  to test the Call()/CallJ()/etc macros as they're part of the OS Layer.
//

#define OSTestCallConditionJ( condition, label )            \
{                                                           \
    if ( !(condition) )                                     \
    {                                                       \
        OSTestReportErr( __LINE__, __FILE__ );              \
        err = ErrERRCheck( JET_errInvalidParameter );       \
        goto label;                                         \
    }                                                       \
}

#define OSTestCallJ( func, label )                          \
{                                                           \
    const JET_ERR errFuncT = (func);                        \
    if ( errFuncT < JET_errSuccess )                        \
    {                                                       \
        OSTestReportErr( errFuncT, __LINE__, __FILE__ );    \
    }                                                       \
    err = errFuncT;                                         \
    if ( err < JET_errSuccess )                             \
    {                                                       \
        goto label;                                         \
    }                                                       \
}

#define OSTestCallCondition( condition )        OSTestCallConditionJ( condition, HandleError )
#define OSTestCall( func )                      OSTestCallJ( func, HandleError )


//  ================================================================
void GetTimes(
    LARGE_INTEGER * const psystemTime,
    LARGE_INTEGER * const pkernelTime,
    LARGE_INTEGER * const puserTime )
//  ================================================================
{
    FILETIME creationTime;  // ignored
    FILETIME exitTime;      // ignored
    FILETIME kernelTime;
    FILETIME userTime;
    GetProcessTimes( GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime );
    pkernelTime->LowPart    = kernelTime.dwLowDateTime;
    pkernelTime->HighPart   = kernelTime.dwHighDateTime;
    puserTime->LowPart      = userTime.dwLowDateTime;
    puserTime->HighPart     = userTime.dwHighDateTime;

    FILETIME systemTime;
    GetSystemTimeAsFileTime( &systemTime );
    psystemTime->LowPart    = systemTime.dwLowDateTime;
    psystemTime->HighPart   = systemTime.dwHighDateTime;
}

//  ================================================================
double TimeDifference( const LARGE_INTEGER& start, const LARGE_INTEGER& end )
//  ================================================================
//
//  Returns the time difference as a double.
//
//-
{
    const LONGLONG diff = end.QuadPart - start.QuadPart;
    return (double)diff / 10000000.0;
}

extern BOOL g_fDisablePerfmon;

//  ================================================================
void PrintHelpEmbeddedUnitTest( _In_ const char* szExecutable )
//  ================================================================
{
    printf( "Usage: %s <args>\n", szExecutable );
    printf( "   <blank>              Executes all non-persisted tests\n"
            "   <wildcard>           Executes specified non-persisted tests\n"
            "   -database <blank>    Executes all tests requiring a database.\n"
            "   -database <wildcard> Executes specified tests requiring a database.\n"
            "   -?|/?|-h|/h          Prints out all tests (and this message).\n"
            "\n"
            );
}

//  ================================================================
JET_ERR ErrConfigureDatabase(
    _Out_ JET_INSTANCE* pinst,
    _Out_ JET_DBID* pdbid,
    _Out_ JET_SESID* psesid
    )
//  ================================================================
{
    JET_ERR err = JET_errSuccess;
    //  Global Params
    //

    OSTestCall( JetSetSystemParameterW( pinst, JET_sesidNil, JET_paramDatabasePageSize, 4 * 1024, NULL ) );

    //  Create the test instance

    OSTestCall( JetCreateInstanceW( pinst, L"KVPStoreInst" ) );

    //  Inst Params
    //

    OSTestCall( JetSetSystemParameterW( pinst, JET_sesidNil, JET_paramRecovery, 0, L"off" ) );
    OSTestCall( JetSetSystemParameterW( pinst, JET_sesidNil, JET_paramMaxTemporaryTables, 0, NULL ) );
    OSTestCall( JetSetSystemParameterW( pinst, JET_sesidNil, JET_paramBaseName, 0, L"kvp" ) );

    //  Create the database so we can run the DB based unit test
    //

    OSTestCall( JetInit( pinst ) );
    OSTestCall( JetBeginSessionW( *pinst, psesid, NULL, NULL ) );
    OSTestCall( JetCreateDatabaseW( *psesid, L"embeddedunittest.edb", NULL, pdbid, JET_bitDbRecoveryOff ) );
    OSTestCall( JetOpenDatabaseW( *psesid, L"embeddedunittest.edb", NULL, pdbid, JET_bitNil ) );

HandleError:
    return err;
}

//  ================================================================
JET_ERR ErrExecutePersistedDatabaseUnitTests(
    _In_opt_ PCSTR szTestToRun
)
//  ================================================================
{
    JET_ERR err = JET_errSuccess;
    JET_INSTANCE inst = JET_instanceNil;
    JET_DBID dbid = JET_dbidNil;
    JET_SESID sesid = JET_sesidNil;

    err = ErrConfigureDatabase( &inst, &dbid, &sesid );
    if ( err < JET_errSuccess )
    {
        printf( "ErrConfigureDatabase() failed with %d.\n", err );
        goto HandleError;
    }

    //  Create the arg to specify the KVPStore unit tests we want to run
    //

    JET_TESTHOOKUNITTEST2 jettest;
    jettest.cbStruct = sizeof(jettest);
    jettest.dbidTestOn = dbid;

    jettest.szTestName = ( szTestToRun != NULL ? szTestToRun : "*" );
    err = JetTestHook( opTestHookUnitTests2, &jettest );
    if ( err < JET_errSuccess )
    {
        printf( "Executing test hook with %s failed with %d.\n", jettest.szTestName, err );
        goto HandleError;
    }

HandleError:
    //  Cleanup ourselves cleanly (note does not delete DB)
    //

    // if test went ok, promote cleanup errors to be return instead
    JET_ERR errCleanup;

    errCleanup = JetCloseDatabase( sesid, dbid, JET_bitNil );
    if ( err >= JET_errSuccess )
    {
        err = errCleanup;
    }
    errCleanup = JetEndSession( sesid, JET_bitNil );
    if ( err >= JET_errSuccess )
    {
        err = errCleanup;
    }
    errCleanup  = JetTerm( inst );
    if ( err >= JET_errSuccess )
    {
        err = errCleanup;
    }

    return err;
}

INT ErrSetTestConfigStore()
{
    JET_INSTANCE instNil = JET_instanceNil;
    // Note: This path HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\ESE2_EmbeddedUnitTest_Temp is just something 
    // that accept has to agree upon before running EmbeddedUnitTest.
    return JetSetSystemParameterW( &instNil, JET_sesidNil, JET_paramConfigStoreSpec, 0, L"reg:HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\ESE2_EmbeddedUnitTest_Temp" );
}

//  ================================================================
INT __cdecl main( INT argc, __in_ecount(argc) char * argv[] )
//  ================================================================
{
    LARGE_INTEGER systemTimeStart;
    LARGE_INTEGER kernelTimeStart;
    LARGE_INTEGER userTimeStart;
    GetTimes( &systemTimeStart, &kernelTimeStart, &userTimeStart );
    JET_ERR err = JET_errSuccess;

    err = ErrSetTestConfigStore();
    if ( err == JET_errFileNotFound )
    {
        //  Just means the key is not present, that's not a blocker, just no overrides in that case.
        err = JET_errSuccess;
    }

    if ( argc > 1
        && ( argv[ 1 ][ 0 ] == '-' || argv[ 1 ][ 0 ] == '/' )
         && ( argv[ 1 ][ 1 ] == '?' || argv[ 1 ][ 1 ] == 'h' ) )
    {
        // Asked for help, just print all the tests.
        PrintHelpEmbeddedUnitTest( argv[ 0 ] );

        err = JetTestHook( opTestHookUnitTests, "-?" );
    }
    else if ( ( 1 == argc )   // No arguments, execute all of the tests.
              || ( 2 == argc && 0 == strcmp( "*", argv[ 1 ] ) ) ) // Explicitly want all tests.
    {
        // Execute all of the tests.
        err = JetTestHook( opTestHookUnitTests, "*" );
    }
    else if ( 0 == strcmp( "-database", argv[ 1 ] ) )
    {
        err = ErrExecutePersistedDatabaseUnitTests( ( 3 == argc ) ? argv[ 2 ] : NULL );
    }
    else
    {
        // Execute the list of user-specified tests.
        for( INT i = 1; i < argc; ++i )
        {
            const JET_ERR errT = JetTestHook( opTestHookUnitTests, argv[i] );
            if( errT < JET_errSuccess )
            {
                err = errT;
            }
        }
    }

    LARGE_INTEGER systemTimeEnd;
    LARGE_INTEGER kernelTimeEnd;
    LARGE_INTEGER userTimeEnd;
    GetTimes( &systemTimeEnd, &kernelTimeEnd, &userTimeEnd );

    wprintf( L"\n" );
    wprintf( L"\tTime of execution: %.2f\n", TimeDifference( systemTimeStart, systemTimeEnd ) );
    wprintf( L"\t        User time: %.2f\n", TimeDifference( userTimeStart, userTimeEnd ) );
    wprintf( L"\t      Kernel time: %.2f\n", TimeDifference( kernelTimeStart, kernelTimeEnd ) );

    return err;
}


