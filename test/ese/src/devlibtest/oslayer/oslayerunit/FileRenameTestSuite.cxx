// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

enum IOREASONPRIMARY : BYTE
{
    iorpInvalid = 0,

    iorpOSUnitTest
};


CUnitTest( TestErrRename, 0, "Test for renaming the file using IFileApi::ErrRename()" );
ERR TestErrRename::ErrTest()
{
    ERR err;
    IFileSystemAPI * pfsapi = NULL;
    IFileAPI * pfapi = NULL;
    WCHAR wszRenamePath[ IFileSystemAPI::cchPathMax ];

    COSLayerPreInit oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }
    OSTestCall( ErrOSInit() );

    OSTestCheckErr( ErrOSFSCreate( NULL, &pfsapi ) );

    OSTestCheckErr( pfsapi->ErrFileCreate( L"TestErrRename.tmp", IFileAPI::fmfOverwriteExisting, &pfapi ) );
    _wfullpath( wszRenamePath, L"TestErrRename.renamed.tmp", _countof( wszRenamePath ) - 1 );

    OSTestCheckExpectedErr( JET_errFileNotFound, pfsapi->ErrPathExists( wszRenamePath, NULL ) );
    OSTestCheckErr( pfapi->ErrRename( wszRenamePath, fFalse ) );
    OSTestCheckErr( pfsapi->ErrPathExists( wszRenamePath, NULL ) );
    delete pfapi;
    pfapi = NULL;

    OSTestCheckErr( pfsapi->ErrFileCreate( L"TestErrRename.tmp", IFileAPI::fmfNone, &pfapi ) );
    OSTestCheckExpectedErr( JET_errFileAlreadyExists, pfapi->ErrRename( wszRenamePath, fFalse ) );
    OSTestCheckErr( pfapi->ErrRename( wszRenamePath, fTrue ) );

    delete pfapi;
    pfapi = NULL;

#ifdef DEBUG
    FNegTestSet( fInvalidUsage );
#endif

    OSTestCheckErr( pfsapi->ErrFileCreate( L"TestErrRename.tmp", IFileAPI::fmfNone, &pfapi ) );
    OSTestCheckExpectedErr( JET_errInvalidParameter, pfapi->ErrRename( L"TestErrRename.failed.tmp", fFalse ) );
    OSTestCheckExpectedErr( JET_errFileNotFound, pfsapi->ErrPathExists( L"TestErrRename.failed.tmp", NULL ) );

#ifdef DEBUG
    FNegTestUnset( fInvalidUsage );
#endif

    delete pfapi;
    pfapi = NULL;
    err = JET_errSuccess;

HandleError:
    delete pfapi;

    pfsapi->ErrFileDelete( L"TestErrRename.tmp" );
    pfsapi->ErrFileDelete( L"TestErrRename.renamed.tmp" );

    delete pfsapi;
    OSTerm();
    return err;
}

