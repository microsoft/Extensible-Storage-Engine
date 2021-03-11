// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpOSUnitTest
};


#define OsLayerFileOpenModesTestInit                                        \
    COSLayerPreInit oslayer;                                                \
    JET_ERR err = JET_errSuccess;                                           \
                                                                            \
    IFileSystemAPI * pfsapi = NULL;                                         \
    IFileAPI * pfapiOne = NULL;                                             \
    IFileAPI * pfapiTwo = NULL;                                             \
                                                                            \
    class CFileSystemConfiguration : public CDefaultFileSystemConfiguration \
    {                                                                       \
        public:                                                             \
            CFileSystemConfiguration()                                      \
            {                                                               \
                m_dtickAccessDeniedRetryPeriod = 30;                        \
                m_fBlockCacheEnabled = fFalse;                              \
            }                                                               \
    } fsconfig;                                                             \
                                                                            \
    WCHAR * wszFileDat = L".\\SinglePartyFile.dat";                         \
    Call( ErrCreateIncrementFile( wszFileDat, fFalse ) );                   \
                                                                            \
    Call( ErrOSInit() );                                                    \
    Call( ErrOSFSCreate( &fsconfig, &pfsapi ) );                            \

#define OsLayerFileOpenModesTestTerm    \
HandleError:                            \
    delete pfapiOne;                    \
    delete pfapiTwo;                    \
    delete pfsapi;                      \
    OSTerm();                           \
    return err;                         \



CUnitTest( OSLayerFileOpenReadOnlyModeWorksWithReadOnlyMode, 0, "" );
ERR OSLayerFileOpenReadOnlyModeWorksWithReadOnlyMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiOne ) );
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiTwo ) );

    OsLayerFileOpenModesTestTerm;
}

CUnitTest( OSLayerFileOpenWriteExclusiveModeFailsWithReadOnlyMode, 0, "" );
ERR OSLayerFileOpenWriteExclusiveModeFailsWithReadOnlyMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiOne ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapiTwo ) );

    delete pfapiOne; pfapiOne = NULL;
    delete pfapiTwo; pfapiTwo = NULL;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapiOne ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiTwo ) );

    OsLayerFileOpenModesTestTerm;
}

CUnitTest( OSLayerFileOpenWriteExclusiveModeFailsWithWriteExclusiveMode, 0, "" );
ERR OSLayerFileOpenWriteExclusiveModeFailsWithWriteExclusiveMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapiOne ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapiTwo ) );

    OsLayerFileOpenModesTestTerm;
}



CUnitTest( OSLayerFileOpenReadFlushModeFailsWithReadOnlyMode, 0, "" );
ERR OSLayerFileOpenReadFlushModeFailsWithReadOnlyMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiOne ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiTwo ) );

    delete pfapiOne; pfapiOne = NULL;
    delete pfapiTwo; pfapiTwo = NULL;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiTwo ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiOne ) );

    OsLayerFileOpenModesTestTerm;
}

CUnitTest( OSLayerFileOpenReadFlushModeFailsWithWriteExclusiveMode, 0, "" );
ERR OSLayerFileOpenReadFlushModeFailsWithWriteExclusiveMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapiOne ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiTwo ) );

    delete pfapiOne; pfapiOne = NULL;
    delete pfapiTwo; pfapiTwo = NULL;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiTwo ) );
    OSTestCheckExpectedErr( JET_errFileAccessDenied, pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfNone, &pfapiOne ) );

    OsLayerFileOpenModesTestTerm;
}

CUnitTest( OSLayerFileOpenReadFlushModeWorksWithReadOnlyPermissibleMode, 0, "" );
ERR OSLayerFileOpenReadFlushModeWorksWithReadOnlyPermissibleMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiOne ) );
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyPermissive, &pfapiTwo ) );

    delete pfapiOne; pfapiOne = NULL;
    delete pfapiTwo; pfapiTwo = NULL;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyPermissive, &pfapiTwo ) );
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiOne ) );

    OsLayerFileOpenModesTestTerm;
}

JET_ERR ErrFileOpenHAReadMode( _In_z_ const WCHAR* const wszPath, _Out_ HANDLE * phFile )
{
    JET_ERR err = JET_errSuccess;

    *phFile = CreateFileW(  wszPath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        (   FILE_ATTRIBUTE_NORMAL |
                            FILE_FLAG_OVERLAPPED |
                            ( ( rand() % 2 ) ? 0 : FILE_FLAG_WRITE_THROUGH ) ),
                        NULL );

    if ( *phFile == NULL || *phFile == INVALID_HANDLE_VALUE )
    {
        err = ( GetLastError() == ERROR_ACCESS_DENIED ) ?
                JET_errFileAccessDenied :
                ( GetLastError() == ERROR_SUCCESS ) ?
                JET_errSuccess :
                errCodeInconsistency;
    }

    return err;
}


CUnitTest( OSLayerFileOpenReadFlushModeWorksWithExternalReadOnlyPermissibleMode, 0, "" );
ERR OSLayerFileOpenReadFlushModeWorksWithExternalReadOnlyPermissibleMode::ErrTest()
{
    OsLayerFileOpenModesTestInit;
    HANDLE hFileTwo = NULL;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiOne ) );
    OSTestCall( ErrFileOpenHAReadMode( wszFileDat, &hFileTwo ) );

    delete pfapiOne; pfapiOne = NULL;
    CloseHandle( hFileTwo ); hFileTwo = NULL;

    OSTestCall( ErrFileOpenHAReadMode( wszFileDat, &hFileTwo ) );
    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiOne ) );

    CloseHandle( hFileTwo ); hFileTwo = NULL;
    OsLayerFileOpenModesTestTerm;
}



CUnitTest( OSLayerFileFlushOnReadOnlyOpenedFileNotPossible, 0, "" );
ERR OSLayerFileFlushOnReadOnlyOpenedFileNotPossible::ErrTest()
{
    OsLayerFileOpenModesTestInit;
    HANDLE hFileOne = NULL;

    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnly, &pfapiOne ) );

    OSTestCall( pfapiOne->ErrFlushFileBuffers( (IOFLUSHREASON)0 ) );

    OSTestCheck( !FlushFileBuffers( HOSLayerTestGetFileHandle( pfapiOne ) ) );
    const DWORD error = GetLastError();
    OSTestCheckExpectedErr( ERROR_ACCESS_DENIED, error );

    delete pfapiOne;
    pfapiOne = NULL;

    OSTestCall( ErrFileOpenHAReadMode( wszFileDat, &hFileOne ) );


    OSTestCall( pfsapi->ErrFileOpen( wszFileDat, IFileAPI::fmfReadOnlyClient, &pfapiTwo ) );
    OSTestCall( pfapiTwo->ErrFlushFileBuffers( (IOFLUSHREASON)0 ) );
    OSTestCheck( FlushFileBuffers( HOSLayerTestGetFileHandle( pfapiTwo ) ) );

    CloseHandle( hFileOne );
    OsLayerFileOpenModesTestTerm;
}

