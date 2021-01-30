// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "std.hxx"

#ifndef ENABLE_JET_UNIT_TEST
#error This file should only be compiled with the unit tests!
#endif

#ifdef ENABLE_JET_UNIT_TEST

JETUNITTEST( OSU, InvalidDirectoryPaths )
{
    IFileSystemAPI * pfsapi;
    WCHAR wszAbsPath[ IFileSystemAPI::cchPathMax ];

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );

    const JET_ERR errExpected = FUtilProcessIsPackaged() ? JET_errFileAccessDenied : JET_errInvalidPath;

    const WCHAR * wszTest1 = L"\\\\localhost\\ComputerName\\SharedFolder\\Resource";
    CHECK( errExpected == ErrUtilCreatePathIfNotExist( pfsapi, wszTest1, wszAbsPath, sizeof( wszAbsPath ) ) );

    const WCHAR * wszTest2 = L"\\\\?\\localhost\\ComputerName\\SharedFolder\\Resource";
    CHECK( errExpected == ErrUtilCreatePathIfNotExist( pfsapi, wszTest2, wszAbsPath, sizeof( wszAbsPath ) ) );

    const WCHAR * wszTest3 = L"\\\\?\\C:";
    CHECK( errExpected == ErrUtilCreatePathIfNotExist( pfsapi, wszTest3, wszAbsPath, sizeof( wszAbsPath ) ) );

    const WCHAR * wszTest4 = L"\\\\?\\3:";
    CHECK( errExpected == ErrUtilCreatePathIfNotExist( pfsapi, wszTest4, wszAbsPath, sizeof( wszAbsPath ) ) );

    const WCHAR * wszTest5 = L"\\\\?\\baba:";
    CHECK( errExpected == ErrUtilCreatePathIfNotExist( pfsapi, wszTest5, wszAbsPath, sizeof( wszAbsPath ) ) );

    const WCHAR * wszTest6 = L"\\\\?\\C:\\ProgramData\\Microsoft\\Windows\\AppRepositoryaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\tmp.edb";
    CHECK( JET_errInvalidPath == ErrUtilCreatePathIfNotExist( pfsapi, wszTest6, wszAbsPath, sizeof( wszAbsPath ) ) );

    delete pfsapi;
}

JETUNITTEST( OSU, RelativeFilePaths )
{
    IFileSystemAPI * pfsapi;

    CHECK( JET_errSuccess == ErrOSFSCreate( &pfsapi ) );
    CHECK( pfsapi->FPathIsRelative( L"accept" ) );
    CHECK( pfsapi->FPathIsRelative( L"abc" ) );
    CHECK( pfsapi->FPathIsRelative( L"abc.def" ) );
    CHECK( pfsapi->FPathIsRelative( L"\\foo.edb" ) );
    CHECK( pfsapi->FPathIsRelative( L"foo\\bar.edb" ) );

    CHECK( !pfsapi->FPathIsRelative( L"\\\\server\\share\\foo.edb" ) );
    CHECK( !pfsapi->FPathIsRelative( L"\\\\?\\share\\foo.edb" ) );
    CHECK( !pfsapi->FPathIsRelative( L"\\\\?\\Volume{21841791-19fc-11e1-93eb-78acc0b71000}\\" ) );
    CHECK( !pfsapi->FPathIsRelative( L"d:\\path\\foo.edb" ) );
    CHECK( !pfsapi->FPathIsRelative( L"d:\\root.edb" ) );

    delete pfsapi;
}


JETUNITTEST( CLimitedEventSuppressor, CheckCanAllowFourNewEventContexts )
{
    CLimitedEventSuppressor   les;

    CHECK( les.FNeedToLog( 0x42 ) );
    CHECK( les.FNeedToLog( 0x52 ) );
    CHECK( les.FNeedToLog( 0x72 ) );
    CHECK( les.FNeedToLog( 0x62 ) );
}

JETUNITTEST( CLimitedEventSuppressor, CheckSuppressesFourDupContexts )
{
    CLimitedEventSuppressor   les;

    CHECK( les.FNeedToLog( 0x42 ) );
    CHECK( les.FNeedToLog( 0x52 ) );
    CHECK( les.FNeedToLog( 0x72 ) );
    CHECK( les.FNeedToLog( 0x62 ) );

    CHECK( !les.FNeedToLog( 0x52 ) );
    CHECK( !les.FNeedToLog( 0x62 ) );
    CHECK( !les.FNeedToLog( 0x42 ) );
    CHECK( !les.FNeedToLog( 0x72 ) );
}

JETUNITTEST( CLimitedEventSuppressor, CheckSuppressesAllContextsPastFour )
{
    CLimitedEventSuppressor   les;

    CHECK( les.FNeedToLog( 0x42 ) );
    CHECK( les.FNeedToLog( 0x52 ) );
    CHECK( les.FNeedToLog( 0x72 ) );
    CHECK( les.FNeedToLog( 0x62 ) );

    CHECK( !les.FNeedToLog( 0x1002 ) );
    CHECK( !les.FNeedToLog( 0x1 ) );
}

JETUNITTEST( CLimitedEventSuppressor, CheckResettingGetsNewEvents )
{
    CLimitedEventSuppressor   les;

    CHECK( les.FNeedToLog( 0x42 ) );
    CHECK( les.FNeedToLog( 0x52 ) );
    CHECK( les.FNeedToLog( 0x72 ) );
    CHECK( les.FNeedToLog( 0x62 ) );

    CHECK( !les.FNeedToLog( 0x42 ) );

    CHECK( !les.FNeedToLog( 0x1002 ) );
    CHECK( !les.FNeedToLog( 0x1 ) );

    les.Reset();

    CHECK( les.FNeedToLog( 0x1002 ) );
    CHECK( les.FNeedToLog( 0x1 ) );
    CHECK( les.FNeedToLog( 0x42 ) );
}

JETUNITTEST( CLimitedEventSuppressor, CheckMix )
{
    CLimitedEventSuppressor   les;

    CHECK( les.FNeedToLog( 0x32 ) );

    CHECK( les.FNeedToLog( 0x52 ) );
    CHECK( !les.FNeedToLog( 0x52 ) );
    CHECK( !les.FNeedToLog( 0x52 ) );
    CHECK( !les.FNeedToLog( 0x32 ) );

    CHECK( les.FNeedToLog( 0x8C002117 ) );
    CHECK( !les.FNeedToLog( 0x52 ) );
    CHECK( !les.FNeedToLog( 0x8C002117 ) );

    CHECK( les.FNeedToLog( 0x10002 ) );
    CHECK( !les.FNeedToLog( 0x8C002117 ) );

    CHECK( !les.FNeedToLog( 0x7C002117 ) );
    CHECK( !les.FNeedToLog( 0x20 ) );
    CHECK( !les.FNeedToLog( 1 ) );
    CHECK( !les.FNeedToLog( 2 ) );

    CHECK( !les.FNeedToLog( 0x52 ) );
    CHECK( !les.FNeedToLog( 0x32 ) );
    CHECK( !les.FNeedToLog( 0x8C002117 ) );
    CHECK( !les.FNeedToLog( 0x10002 ) );
}

#endif

