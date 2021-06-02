// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "osunitstd.hxx"

//  ================================================================
class PATHTESTS : public UNITTEST
//  ================================================================
{
    private:
        static PATHTESTS s_instance;

    protected:
        PATHTESTS() {}

    public:
        ~PATHTESTS() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

PATHTESTS PATHTESTS::s_instance;

const char * PATHTESTS::SzName() const      { return "Path Test"; };
const char * PATHTESTS::SzDescription() const   { return 
            "Tests the OS Layer path parsing and validation functionality.";
        }
bool PATHTESTS::FRunUnderESE98() const      { return true; }
bool PATHTESTS::FRunUnderESENT() const      { return true; }
bool PATHTESTS::FRunUnderESE97() const      { return true; }


//  ================================================================
ERR PATHTESTS::ErrTest()
//  ================================================================
{
    JET_ERR         err                 = JET_errSuccess;
    JET_ERR         errBadPathExpected  = JET_errInvalidPath;
    IFileSystemAPI * pfsapi = NULL;
    WCHAR wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR wszT[ IFileSystemAPI::cchPathMax ];
    IFileAPI * pfapi    = NULL;

    COSLayerPreInit     oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }


    //  This is very disturbing: for non-existing network paths, Vista returns ERROR_FILE_NOT_FOUND,
    //  which gets translated into JET_errFileNotFound, while Win7 returns ERROR_PATH_NOT_FOUND,
    //  which turns into JET_errInvalidPath.

    wprintf( L"\tTesting OS version ...\n" );

    const DWORD dwVersion = GetVersion();
    const DWORD dwBuild = (DWORD)(HIWORD(dwVersion));

    if ( dwBuild < 7600 )
    {
        errBadPathExpected = JET_errFileNotFound;
    }

    wprintf( L"\terrBadPathExpected is %d ...\n", errBadPathExpected );

    wprintf( L"\tTesting path parsing and validation functionality ...\n" );

    OSTestCheckErr( ErrOSFSCreate( NULL, &pfsapi ) );

    const WCHAR * wszTest1 = L"c:";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest1, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest2 = L"c:\\";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest2, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest3 = L"c:..\\bababa";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest3, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest4 = L"egy\\ket\\harom.edb";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest4, wszFolder, wszT, wszT ) );
    BOOL fExists = fFalse;
    OSTestCheckExpectedErr( JET_errFileNotFound, pfsapi->ErrPathExists( wszTest4, &fExists ) );
    OSTestCheck( !fExists );
    OSTestCheckErr( pfsapi->ErrPathComplete( wszTest4, wszT ) );
    OSTestCheckExpectedErr( JET_errInvalidPath, pfsapi->ErrFileOpen( wszTest4, IFileAPI::fmfNone, &pfapi ) );

    const WCHAR * wszTest5 = L"file";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest5, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest6 = L"\\\\.\\mamaeba";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest6, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest7 = L"c:a";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest7, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest8 = L"\\ab";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest8, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest9 = L"\\";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest9, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest10 = L"c:\\foo\\..\\bar\\.\\..\\autoexec.bat";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest10, wszFolder, wszT, wszT ) );

    const WCHAR * wszTest11 = L"\\localhost\\shouldnotexistforreal\\hagadol\\harishon\\hachaham";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest11, wszFolder, wszT, wszT ) );
    OSTestCheckExpectedErr( JET_errInvalidPath, pfsapi->ErrFileOpen( wszTest11, IFileAPI::fmfNone, &pfapi ) );
    OSTestCheck( NULL == pfapi );

#ifndef MINIMAL_FUNCTIONALITY
    // This seems to hang for a while randomly on the phone
    const WCHAR * wszTest12 = L"\\\\localhost\\shouldnotexistforreal\\hagadol\\harishon\\hachaham";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest12, wszFolder, wszT, wszT ) );
    OSTestCheckExpectedErr( errBadPathExpected, pfsapi->ErrFileOpen( wszTest12, IFileAPI::fmfNone, &pfapi ) );
    OSTestCheck( NULL == pfapi );

    const WCHAR * wszTest13 = L"\\\\?\\localhost\\shouldnotexistforreal\\hagadol\\harishon\\hachaham";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest13, wszFolder, wszT, wszT ) );
    OSTestCheckExpectedErr( JET_errInvalidPath, pfsapi->ErrFileOpen( wszTest13, IFileAPI::fmfNone, &pfapi ) );
    OSTestCheck( NULL == pfapi );
#endif

    const WCHAR * wszTest14 = L"\\\\?\\c:\\shouldnotexistforreal\\foo\\bar";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest14, wszFolder, wszT, wszT ) );
    OSTestCheckExpectedErr( JET_errInvalidPath, pfsapi->ErrFileOpen( wszTest14, IFileAPI::fmfNone, &pfapi ) );
    OSTestCheck( NULL == pfapi );

    const WCHAR * wszTest15 = L"\\\\.\\c:\\";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest15, wszFolder, wszT, wszT ) );
    OSTestCheckExpectedErr( JET_errInvalidPath, pfsapi->ErrFileOpen( wszTest15, IFileAPI::fmfNone, &pfapi ) );
    OSTestCheck( NULL == pfapi );

    const WCHAR * wszTest16 = L"\\\\.\\c:\\shouldnotexistforreal\\hagadol\\harishon\\hachaham\\sanhedrin.rosh";
    OSTestCheckErr( pfsapi->ErrPathParse( wszTest16, wszFolder, wszT, wszT ) );
    OSTestCheckExpectedErr( JET_errInvalidPath, pfsapi->ErrFileOpen( wszTest16, IFileAPI::fmfNone, &pfapi ) );
    OSTestCheck( NULL == pfapi );

HandleError:

    delete pfsapi;

    return err;
}

//  ================================================================
class TestSimpleWszPathFileNameParseFunctionSuite : public UNITTEST
//  ================================================================
{
    private:
        static TestSimpleWszPathFileNameParseFunctionSuite s_instance;

    protected:
        TestSimpleWszPathFileNameParseFunctionSuite() {}

    public:
        ~TestSimpleWszPathFileNameParseFunctionSuite() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

TestSimpleWszPathFileNameParseFunctionSuite TestSimpleWszPathFileNameParseFunctionSuite::s_instance;

const char * TestSimpleWszPathFileNameParseFunctionSuite::SzName() const        { return "TestSimpleWszPathFileNameParseFunctionSuite"; };
const char * TestSimpleWszPathFileNameParseFunctionSuite::SzDescription() const { return "Tests the OS Layer path parsing functionality for COSFileSystem::WszPathFileName."; }
bool TestSimpleWszPathFileNameParseFunctionSuite::FRunUnderESE98() const        { return true; }
bool TestSimpleWszPathFileNameParseFunctionSuite::FRunUnderESENT() const        { return true; }
bool TestSimpleWszPathFileNameParseFunctionSuite::FRunUnderESE97() const        { return true; }


//  ================================================================
ERR TestSimpleWszPathFileNameParseFunctionSuite::ErrTest()
//  ================================================================
{
    JET_ERR         err                 = JET_errSuccess;
    IFileSystemAPI * pfsapi = NULL;
    WCHAR wszFolder[ IFileSystemAPI::cchPathMax ];
    WCHAR wszT[ IFileSystemAPI::cchPathMax ];

    COSLayerPreInit     oslayer;

    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    wprintf( L"\tTesting simple filename path parsing functionality ...\n" );

    OSTestCheckErr( ErrOSFSCreate( NULL, &pfsapi ) );

    OSTestCheck( 0 == wcscmp( L"FooBar.edb", pfsapi->WszPathFileName( L"C:\\\\users\\TheHistorian\\FooBar.edb" ) ) );
    OSTestCheck( 0 == wcscmp( L"FuBar.edb", pfsapi->WszPathFileName( L"FuBar.edb" ) ) );

    OSTestCheck( 0 == wcscmp( L"FeeBar.edb", pfsapi->WszPathFileName( L"\\\\tsclient\\c\\users\\TheHistorian\\FeeBar.edb" ) ) );
    OSTestCheck( 0 == wcscmp( L"FiBar", pfsapi->WszPathFileName( L"\\\\tsclient\\c\\users\\TheHistorian\\FiBar" ) ) );

    OnDebug( FNegTestSet( fInvalidUsage ) );
    OSTestCheck( 0 == wcscmp( L"UNKNOWN.PTH", pfsapi->WszPathFileName( L"C:\\users\\TheHistorian\\FohBar.edb\\" ) ) );
    OnDebug( FNegTestUnset( fInvalidUsage ) );

    //  Filename should be pulled from the other string and not allocated or some other temporary data.
    WCHAR * wszFumBar = L"C:\\users\\TheHistorian\\FumBar.edb";
    const WCHAR * wszFumBarFn = pfsapi->WszPathFileName( wszFumBar );
    OSTestCheck( ( (QWORD)wszFumBarFn > (QWORD)wszFumBar ) &&
                    ( (QWORD)wszFumBarFn < ( ((QWORD)wszFumBarFn) + ( wcslen(wszFumBar) * sizeof(WCHAR) ) ) ) );
    OSTestCheck( 0 == wcscmp( L"FumBar.edb", wszFumBarFn ) ); // oh and should've worked!

HandleError:

    delete pfsapi;

    return err;
}


