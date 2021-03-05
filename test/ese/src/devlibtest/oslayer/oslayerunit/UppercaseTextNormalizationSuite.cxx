// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

CUnitTest( UppercaseTextNormalizationTest, 0, "Tests the OS Layer Unicode normalization by upper case the text." );


ERR UppercaseTextNormalizationTest::ErrTest()
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

    NORM_LOCALE_VER nlv;
    QWORD qwSortVersion;

    memset( &nlv, 0, sizeof(nlv) );

    DWORD dwNlsVersion;
    DWORD dwDefinedNlsVersion;
    BYTE rgbNormalized[ 16 ];
    INT cbActual;

    Call( ErrNORMGetSortVersion( L"zh-CN", &qwSortVersion, &nlv.m_sortidCustomSortVersion ) );
    QwSortVersionToNLSDefined( qwSortVersion, &dwNlsVersion, &dwDefinedNlsVersion );
    nlv.m_dwNlsVersion = dwNlsVersion;
    nlv.m_dwDefinedNlsVersion = dwDefinedNlsVersion;
    nlv.m_dwNormalizationFlags = LCMAP_UPPERCASE;

    const WCHAR wszSrcChinese[] = { L'\x4E2D', L'\x6587', L'\0' };
	Call( ErrNORMMapString(
		&nlv,
		(BYTE*) &wszSrcChinese[0],
		sizeof( wszSrcChinese ) - sizeof(WCHAR),
		&rgbNormalized[0],
		sizeof( rgbNormalized ),
		&cbActual ) );
	const BYTE rgbExpectedChinese[] = { 0x4E, 0x2D, 0x65, 0x87, 0x00, 0x00 };
    OSTestCheck( cbActual == sizeof(rgbExpectedChinese) );
    OSTestCheck( 0 == memcmp( rgbExpectedChinese, rgbNormalized, cbActual ) );

HandleError:
    OSTerm();

    OSTestCheck( JET_errSuccess == err );
    return err;
}

CUnitTest( UppercaseTextNormalizationSanityTest, 0, "Tests the OS Layer Unicode normalization by upper case the text." );

ERR UppercaseTextNormalizationSanityTest::ErrTest()
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

    WCHAR szSrc[2] = { L'\0', L'\0' };
    WCHAR szDest[2];
    for ( INT i = 1; i < 65536; i++ )
    {
        *(WCHAR *) &szSrc[0] = (WCHAR)i;
        INT result = LCMapStringEx(L"de-DE", LCMAP_UPPERCASE, (LPWSTR)szSrc, 1, (LPWSTR) szDest, 100, NULL, NULL, 0);

        OSTestCheck( 1 == result );
    }
HandleError:
    OSTerm();

    return err;
}
