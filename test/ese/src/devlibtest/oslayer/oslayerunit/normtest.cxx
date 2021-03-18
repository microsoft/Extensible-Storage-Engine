// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osunitstd.hxx"

//  ================================================================
class NORMTESTS : public UNITTEST
//  ================================================================
{
    private:
        static NORMTESTS s_instance;

    protected:
        NORMTESTS() {}

    public:
        ~NORMTESTS() {}

    public:
        const char * SzName() const;
        const char * SzDescription() const;

        bool FRunUnderESE98() const;
        bool FRunUnderESENT() const;
        bool FRunUnderESE97() const;

        ERR ErrTest();
};

NORMTESTS NORMTESTS::s_instance;

const char * NORMTESTS::SzName() const      { return "NormalizationTest"; };
const char * NORMTESTS::SzDescription() const   { return 
            "Tests the OS Layer Unicode normalization.";
        }
bool NORMTESTS::FRunUnderESE98() const      { return true; }
bool NORMTESTS::FRunUnderESENT() const      { return true; }
bool NORMTESTS::FRunUnderESE97() const      { return true; }


INT CbNORMMapString_(
    _In_ PCWSTR                             wszLocaleName,
    _In_ const DWORD                        dwNlsVersion,
    _In_opt_ const GUID*                    pguidCustomVersion,
    _In_ const DWORD                        dwLCMapFlags,
    _In_reads_( cbColumng ) BYTE *          pbColumn,
    _In_ const INT                          cbColumn,
    _Out_writes_opt_( cbKeyMost ) BYTE *    rgbKey,
    _In_ const INT                          cbKeyMost );

// reach into the OS layer for testing purposes ...
extern BOOL g_fNormMemoryLeakAcceptable;
BOOL FNORMIGetNLSExWithVersionInfoIsSupported();

//  ================================================================
ERR NORMTESTS::ErrTest()
//  ================================================================
{
    JET_ERR         err = JET_errSuccess;

    const BOOL fNormMemoryLeakAcceptableSaved = g_fNormMemoryLeakAcceptable;

    COSLayerPreInit     oslayer;

    // The table works fine on Win8; but should be updated to be OS-agnostic.
    struct
{
        NORM_LOCALE_VER nlv;
        JET_ERR errExpectedWinVista;
        JET_ERR errExpectedWin70;
        JET_ERR errExpectedWin80;
        JET_ERR errExpectedWin81;
        JET_ERR errExpectedWin10;
} rgnormalizations[] =
{
        // normal case. no NLS versions.
    {
        {
                SORTIDNil, // Sort GUID
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0, // NLS Version
                0, // NLS Defined Version
                { L'p', L't', L'-', L'b', L'r', L'\0' },
        },
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
    },
        // Vista version.
    {
        {
                SORTIDNil, // Sort GUID
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x500100, // NLS Version, Vista
                0x50100, // NLS Defined Version, Vista
                { L'p', L't', L'-', L'b', L'r', L'\0' },
        },
            JET_errSuccess,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
    },
        // Win7 version.
    /* Does not work on server core
    {
        {
                SORTIDNil, // Sort GUID
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x60101, // NLS Version, Win7
                0x60101, // NLS Defined Version, Not valid
                { L'p', L't', L'-', L'b', L'r', L'\0' },
        },
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
    },
    */
        // Win8.1 version, no GUID.
    {
        {
                SORTIDNil, // Sort GUID
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x6020e, // NLS Version, Win8-10
                0x6020e, // ignored on win8+
                { L'p', L't', L'-', L'b', L'r', L'\0' },
        },
            JET_errSuccess, // Vista ignores the larger sort version.
            JET_errUnicodeTranslationFail, // We asked for Win8, but are on Win7.
            JET_errSuccess, // Interesting that it succeeds on Win8.0
            JET_errSuccess,
            JET_errSuccess,
    },

        // Win8.1 version, with GUID.
    {
        {
                0x00000001, 0x57ee, 0x1e5c, { 0x00, 0xb4, 0xd0, 0x00, 0x0b, 0xb1, 0xe1, 0x1e },  // Sort GUID {00000001-57ee-1e5c-00b4-d0000bb1e11e}
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x6020e, // NLS Version, Win8-10
                0x6020e, // ignored on win8+
                { L'e', L'n', L'-', L'u', L's', L'\0' }, // en-us
        },
            JET_errSuccess,
            JET_errUnicodeTranslationFail, // Fails on Win7.
            JET_errSuccess, // Interesting that it succeeds on Win8.0
            JET_errSuccess,
            JET_errSuccess,
    },

        // Invariant Locale (LCID 127)
    {
        {
                0x00000001, 0x57ee, 0x1e5c, { 0x00, 0xb4, 0xd0, 0x00, 0x0b, 0xb1, 0xe1, 0x1e },  // Sort GUID {00000001-57ee-1e5c-00b4-d0000bb1e11e}
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x6020e, // NLS Version, Win8-10
                0x6020e, // ignored on win8+
                { L'\0' }, // ""
        },
            JET_errSuccess,
            JET_errUnicodeTranslationFail, // Fails on Win7.
            JET_errSuccess, // Interesting that it succeeds on Win8.0
            JET_errSuccess,
            JET_errSuccess,
    },

        // Some locales which have no corresponding LCIDs:
        // jv-Latn-ID is recognized
        // mg-MG is recognized
        // nqo-GN is recognized
        // pt-AO is recognized
        // sn-Latn-ZW is recognized
        // zgh-Tfng-MA is recognized
    {
        {
                SORTIDNil, // Sort GUID
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x6020e, // NLS Version, Win8.1-10
                0x6020e, // ignored on win8+
                { L's', L'n', L'-', L'L', L'a', L't', L'n', L'-', L'Z', L'W', L'\0' }, // sn-Latn-ZW
        },
            JET_errUnicodeTranslationFail, // Vista ignores the larger sort version.
            JET_errUnicodeTranslationFail, // We asked for Win8.1, but are on Win7.
            JET_errUnicodeTranslationFail, // We asked for Win8.1, but are on Win8.
            JET_errSuccess,
            JET_errSuccess,
    },

        // Win10 only:
    {
        {
                SORTIDNil, // Sort GUID
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0x6020e, // NLS Version, Win8.1-10
                0x6020e, // ignored on win8+
                { L'h', L'e', L'-', L'I', L'L', L'-', L'u', L'-', L'c', L'a', L'-', L'h', L'e', L'b', L'r', L'e', L'w', L'-', L't', L'z', L'-', L'j', L'e', L'r', L'u', L's', L'l', L'm', L'\0' }, // he-IL-u-ca-hebrew-tz-jeruslm
        },
            JET_errUnicodeTranslationFail, // Vista ignores the larger sort version.
            JET_errUnicodeTranslationFail, // We asked for Win10, but are on Win7.
            JET_errUnicodeTranslationFail, // We asked for Win10, but are on Win8.
            JET_errUnicodeTranslationFail, // We asked for Win10, but are on Win81.
            JET_errSuccess,
    },

        // he-IL-u-ca-hebrew-tz-jeruslm
        
        // Commenting out this case. It asserts if we pass in a sort GUID and no
        // NLS version.
        /*
    {
        {
                0x00000001, 0x57ee, 0x1e5c, { 0x00, 0xb4, 0xd0, 0x00, 0x0b, 0xb1, 0xe1, 0x1e },  // Sort GUID {00000001-57ee-1e5c-00b4-d0000bb1e11e}
                NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LCMAP_SORTKEY,
                0, // NLS Version, Win8-10
                0, // ignored on win8+
                { L'e', L'n', L'-', L'u', L's', L'\0' }, // en-us
        },
            JET_errSuccess,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
    },
        */

        // Invalid LCMAP string flags.
    {
        {
                SORTIDNil, // Sort GUID
                LCMAP_SORTKEY | LCMAP_UPPERCASE,
                0x6020e, // NLS Version, Win8.1-10
                0x6020e, // ignored on win8+
                { L'z', L'h', L'-', L'C', L'N', L'\0' }, // zh-CN
        },
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
    },

        // LCMAP_UPPPERCASE flag.
    {
        {
                SORTIDNil, // Sort GUID
                LCMAP_UPPERCASE,
                0x6020e, // NLS Version, Win8.1-10
                0x6020e, // ignored on win8+
                { L'z', L'h', L'-', L'C', L'N', L'\0' }, // zh-CN
        },
            JET_errUnicodeTranslationFail,
            JET_errUnicodeTranslationFail,
            JET_errSuccess,
            JET_errSuccess,
            JET_errSuccess,
    },
};

    // FOSPreinit()
    if ( !oslayer.FInitd() )
    {
        wprintf( L"Out of memory error during OS Layer pre-init." );
        err = JET_errOutOfMemory;
        goto HandleError;
    }

    Call( ErrOSInit() );

    // NOTE: fWin81/fWin10 will only be accurate if the executable is manifested correctly.
    // By default, all executables from Win8.1+ will get a WinVer of 6.2 (Win8.0).
    const BOOL fVista = ( DwUtilSystemVersionMajor() == 6 ) && ( DwUtilSystemVersionMinor() == 0 );
    const BOOL fWin70 = ( DwUtilSystemVersionMajor() == 6 ) && ( DwUtilSystemVersionMinor() == 1 );
    const BOOL fWin80 = ( DwUtilSystemVersionMajor() == 6 ) && ( DwUtilSystemVersionMinor() == 2 );
    const BOOL fWin81 = ( DwUtilSystemVersionMajor() == 6 ) && ( DwUtilSystemVersionMinor() == 3 );
    const BOOL fWin10 = ( DwUtilSystemVersionMajor() == 10 ) && ( DwUtilSystemVersionMinor() == 0 );

    wprintf( L"fVista = %d, fWin70 = %d, fWin80 = %d, fWin81 = %d, fWin10 = %d\n",
             fVista, fWin70, fWin80, fWin81, fWin10 );

    OSTestCheck( fVista || fWin70 || fWin80 || fWin81 || fWin10 );

    // Due to a memory leak in LCMapStringEx on Win8 through some Win10 Th1 builds we were forced to disable this
    // feature on builds less than Win10 Th2.  We'll test our XxxVersionInfoIsSupported() func here though.
    // Note: On the Windows side we're assuming no one is shipping our DLL downevel (so this FNORMIGetNLSExWithVersionInfoIsSupported()
    // routine always returns true) ... but this is sort of potentially not true if we test on older OSes, but whatever.
#ifndef ESENT
    if ( fVista || fWin70 || fWin80 || fWin81 )
    {
        OSTestCheck( !FNORMIGetNLSExWithVersionInfoIsSupported() );

        wprintf( L"Warning: Unicode normalization with sort info version has a mem leak on OS version < Win10!\n" );
        wprintf( L"   and SO normally this functionality is not supported on these OS.\n" );
        wprintf( L"   BUT we'll do the testing anyways, as the memory leak isn't bad enough for our testing ....\n" );
        // If this extra testing every gives any guff, we can just bail out to HandleError here.
        g_fNormMemoryLeakAcceptable = fTrue;
    }
#endif // !ESENT
    OSTestCheck( FNORMIGetNLSExWithVersionInfoIsSupported() );

    wprintf( L"\tTesting Unicode normalization functionality ...\n");

    const WCHAR* szData = L"The rain in Brazil falls mainly in GRU.";
    const size_t cbData = ( LOSStrLengthW( szData ) + 1 ) * sizeof( szData[0] );

    for ( size_t i = 0; i < _countof( rgnormalizations ); ++i )
    {
        JET_ERR errExpected = JET_errInternalError;
        if ( fVista ) { errExpected = rgnormalizations[i].errExpectedWinVista; }
        if ( fWin70 ) { errExpected = rgnormalizations[i].errExpectedWin70; }
        if ( fWin80 ) { errExpected = rgnormalizations[i].errExpectedWin80; }
        if ( fWin81 ) { errExpected = rgnormalizations[i].errExpectedWin81; }
        if ( fWin10 ) { errExpected = rgnormalizations[i].errExpectedWin10; }

        RPC_WSTR strGuid = NULL;

        RPC_STATUS rpcStatus = UuidToStringW( &rgnormalizations[i].nlv.m_sortidCustomSortVersion, &strGuid );

        printf( "Test case %d expected to return %d. Locale %ws, Nls=%#x, DefinedNls=%#x, sortguid=%ws.\n",
                i,
                errExpected,
                rgnormalizations[i].nlv.m_wszLocaleName,
                rgnormalizations[i].nlv.m_dwNlsVersion,
                rgnormalizations[i].nlv.m_dwDefinedNlsVersion,
                strGuid );
        if ( strGuid != NULL && rpcStatus == RPC_S_OK )
        {
            RpcStringFreeW(&strGuid);
            strGuid = NULL;
        }

        const size_t cbNormalizedDataMax = 1024;
        BYTE rgbNormalizedData[ cbNormalizedDataMax ];
        INT cbActualNormalized = -1;

        OSTestCheckExpectedErr(
            errExpected,
            ErrNORMMapString(
                &rgnormalizations[i].nlv,
                (BYTE*) szData,
                cbData,
                &rgbNormalizedData[ 0 ],
                cbNormalizedDataMax,
                &cbActualNormalized ) );

        printf( "Test case %d returned %d. Expected was %d.\n",
                i, err, errExpected );

        // ErrNORMCheckLocaleVersion was written later. Does it return the same value
        // as our function?
    {
        const ERR errCheckVersion = ErrNORMCheckLocaleVersion( &rgnormalizations[i].nlv );
        OSTestCheck( errExpected == errCheckVersion );
    }

        err = JET_errSuccess;

        if ( errExpected >= JET_errSuccess )
        {
            printf( "Normalizing that hard-coded string: From %Id to %d bytes.\n",
                    cbData,
                    cbActualNormalized );

            OSTestCheck( cbActualNormalized >= (INT)cbData );
        }
    }

HandleError:

    g_fNormMemoryLeakAcceptable = fNormMemoryLeakAcceptableSaved;

    OSTerm();

    return err;
}

