// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

CUnitTest( NORMNLSVersionEqualsTest, 0, "Tests NLS version equals." );

struct TestData
{
    DWORD dwSourceVersion;
    DWORD dwTargetVersion;
    BOOL fexpectEquals;
};

ERR NORMNLSVersionEqualsTest::ErrTest()
{
    JET_ERR         err = JET_errSuccess;
    COSLayerPreInit     oslayer;
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    Call( ErrOSInit() );

    TestData testMatrix[] =
{
        { 0x00060100, 0x00060101, fFalse },
        { 0x00060101, 0x00060100, fFalse },
        { 0x00060101, 0x00060101, fTrue },
        { 0x00060102, 0x00060102, fTrue },
        { 0x00060101, 0x00060102, fTrue },
        { 0x00060102, 0x00060101, fTrue },
        { 0x0006020c, 0x0006020d, fFalse },
        { 0x0006020d, 0x00060200, fFalse },
        { 0x0006020d, 0x0006020d, fTrue },
        { 0x0006020e, 0x0006020e, fTrue },
        { 0x0006020d, 0x0006020e, fTrue },
        { 0x0006020e, 0x0006020d, fTrue },
};

    for ( INT i = 0; i < sizeof( testMatrix ) / sizeof (testMatrix[ 0 ] ); i++ )
{
        QWORD qwSourceVersion = QwSortVersionFromNLSDefined( testMatrix[ i ].dwSourceVersion, testMatrix[ i ].dwSourceVersion );
        QWORD qwTargetVersion = QwSortVersionFromNLSDefined( testMatrix[ i ].dwTargetVersion, testMatrix[ i ].dwTargetVersion );
        
        OSTestCheck(testMatrix[ i ].fexpectEquals == FNORMNLSVersionEquals( qwSourceVersion, qwTargetVersion ) );
}

HandleError:
    OSTerm();

    OSTestCheck( JET_errSuccess == err );
    return err;
}
