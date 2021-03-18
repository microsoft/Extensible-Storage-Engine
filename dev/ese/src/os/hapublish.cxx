// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  we use this LoadLibrary to load the external HA message DLL for reporting failure items.
#undef LoadLibraryExW

#if defined( USE_HAPUBLISH_API )
#define HAPUBLISHLIBNAME    L"ExDbFailureItemApi"
#define HAPUBLISHLIBEXT     L".dll"
#define HAPUBLISHFUNC       "HaPublishDbFailureItem"

HMODULE g_hmodHaPublish;
typedef UINT WINAPI FNHAPUBLISHDBFAILUREITEM( __in HaDbFailureItem* fi );
FNHAPUBLISHDBFAILUREITEM* g_pfnHaPublishDbFailureItem;

static CRITICAL_SECTION g_csHaPublish;
static BOOL g_fcsHaPublishInit;


//================================================
// GUID utilities
//================================================
inline unsigned char wcstouc( WCHAR wc1, WCHAR wc2 )
{
    Assert( iswxdigit( wc1 ) && iswxdigit( wc2 ) );

    WCHAR wsz[3] = { wc1, wc2, 0, };
    return ( unsigned char )wcstoul( wsz, NULL, 16 );
}

static const INT s_cch = 36, s_i0 = 8, s_i1 = 13, s_i2 = 18, s_i3 = 23;

static bool FIsGUID( const WCHAR* wsz )
{
    bool fRet = true;

    const char* sz = "e981a9c7-4934-450d-892f-3a335645223d";

    // make sure we have computed the right constants.
    Assert( s_cch == strlen( sz ) );
    Assert( 0 <= s_i0 && s_i0 < s_i1 && s_i1 < s_i2 && s_i2 < s_i3 && s_i3 < s_cch );
    Assert( '-' == sz[ s_i0 ] && '-' == sz[ s_i1 ] && '-' == sz[ s_i2 ] && '-' == sz[ s_i3 ] );

    if ( fRet = wsz && s_cch <= LOSStrLengthW( wsz ) )
    {
        for ( INT i = 0; i < s_cch && fRet; ++i )
        {
            switch ( i )
            {
                case s_i0:
                case s_i1:
                case s_i2:
                case s_i3:
                    fRet = fRet && L'-' == wsz[ i ];
                    break;

                case s_cch:
                    fRet = fRet && !iswxdigit( wsz[ i ] );

                default:
                    fRet = fRet && iswxdigit( wsz[ i ] );
                    break;
            }
        }
    }

    return fRet;
}

static GUID ParseGuid( const WCHAR* const wsz )
{
    GUID guid = { 0, };

    if ( FIsGUID( wsz ) )
    {
        guid.Data1 = wcstoul( wsz, NULL, 16 );
        guid.Data2 = ( USHORT )wcstoul( &wsz[ s_i0 + 1 ], NULL, 16 );
        guid.Data3 = ( USHORT )wcstoul( &wsz[ s_i1 + 1 ], NULL, 16 );

        guid.Data4[ 0 ] = wcstouc( wsz[ s_i2 + 1 ], wsz[ s_i2 + 2 ] );
        guid.Data4[ 1 ] = wcstouc( wsz[ s_i2 + 3 ], wsz[ s_i2 + 4 ] );
        for ( INT i = 0; i < 6; guid.Data4[ 2 + i ] = wcstouc( wsz[ s_i3 + 1 + i * 2 ], wsz[ s_i3 + 1 + i * 2 + 1] ), i++ );
    }

    return guid;
}

//================================================
// main Ha publish function
//================================================
void OSUHAPublishEventImpl(
    HaDbFailureTag      haTag,
    const WCHAR*        wszDbGuid,
    const WCHAR*        wszDbInstanceName,
    DWORD               dwComponent,
    HaDbIoErrorCategory haCategory,
    const WCHAR*        wszFilename,
    unsigned _int64     qwOffset,
    DWORD               cbSize,
    DWORD               dwEventId,
    DWORD               cParameter,
    const WCHAR**       rgwszParameter )
{
    Assert( HADBFAILURE_EVENT_RANGE_START_ESE < dwComponent );
    Assert( HADBFAILURE_EVENT_RANGE_START_ESE <= dwEventId || 0 == dwEventId );

    if ( g_pfnHaPublishDbFailureItem )
    {
        HaDbIoErrorInfo haDbIoErrorInfo = { sizeof( haDbIoErrorInfo ), };
        haDbIoErrorInfo.m_category          = haCategory;
        haDbIoErrorInfo.m_fileName          = wszFilename ? wszFilename : L"";
        haDbIoErrorInfo.m_offset            = qwOffset;
        haDbIoErrorInfo.m_size              = cbSize;

        HaDbNotificationEventInfo haDbNotificationEventInfo = { sizeof( haDbNotificationEventInfo ), (INT) dwEventId, (INT)cParameter, rgwszParameter };

        HaDbFailureItem haDbFailureItem = { sizeof( haDbFailureItem ), HaDbFailureESE, };
        haDbFailureItem.m_tag               = haTag;
        haDbFailureItem.m_dbGuid            = ParseGuid( wszDbGuid );
        haDbFailureItem.m_dbInstanceName    = wszDbInstanceName;
        // TODO: add ESE component name based on HA_*_CATEGORY
        haDbFailureItem.m_componentName = L"";
        haDbFailureItem.m_ioError           = &haDbIoErrorInfo;
        haDbFailureItem.m_notifyEventInfo   = &haDbNotificationEventInfo;

        ( *g_pfnHaPublishDbFailureItem )( &haDbFailureItem );
    }
}
#endif


//================================================
// global init/term
//================================================
bool FOSHaPublishPreinit()
{
#if defined( USE_HAPUBLISH_API )
    Assert( !g_hmodHaPublish && !g_pfnHaPublishDbFailureItem );
    Assert( !g_fcsHaPublishInit );
    if ( !InitializeCriticalSectionAndSpinCount( &g_csHaPublish, 0 ) )
    {
        return false;
    }
    g_fcsHaPublishInit = fTrue;
#endif

    return true;
}

void OSHaPublishPostterm()
{
#if defined( USE_HAPUBLISH_API )
    Assert( !!g_hmodHaPublish == !!g_pfnHaPublishDbFailureItem );

    if ( g_hmodHaPublish )
    {
        FreeLibrary( g_hmodHaPublish );
        g_hmodHaPublish = NULL;
        g_pfnHaPublishDbFailureItem = NULL;
    }

    if( g_fcsHaPublishInit )
    {
        DeleteCriticalSection( &g_csHaPublish );
        g_fcsHaPublishInit = fFalse;
    }
    Assert( !g_fcsHaPublishInit );
#endif
}


//================================================
// instance init
//================================================
bool FUtilHaPublishInit()
{
#if defined( USE_HAPUBLISH_API )
    Assert( g_fcsHaPublishInit );
    EnterCriticalSection( &g_csHaPublish );
    if ( !g_hmodHaPublish )
    {
        WCHAR wszDrive[ _MAX_DRIVE ];
        WCHAR wszDir[ _MAX_DIR ];
        _wsplitpath_s( WszUtilImagePath(), wszDrive, _countof( wszDrive ), wszDir, _countof( wszDir ), NULL, 0, NULL, 0 );

        WCHAR wszPath[ _MAX_PATH ];
        _wmakepath_s( wszPath, _countof( wszPath ), wszDrive, wszDir, HAPUBLISHLIBNAME, HAPUBLISHLIBEXT );
        g_hmodHaPublish = LoadLibraryExW( wszPath, NULL, 0 );

        //  If that fails, let LoadLibrary() do the lookup.
        if ( g_hmodHaPublish == NULL )
        {
            _wmakepath_s( wszPath, _countof( wszPath ), NULL, NULL, HAPUBLISHLIBNAME, HAPUBLISHLIBEXT );
            g_hmodHaPublish = LoadLibraryExW( wszPath, NULL, 0 );
        }

        if ( g_hmodHaPublish != NULL )
        {
            const FARPROC fp = GetProcAddress( g_hmodHaPublish, HAPUBLISHFUNC );
            if ( !( g_pfnHaPublishDbFailureItem = ( FNHAPUBLISHDBFAILUREITEM* )fp ) )
            {
                FreeLibrary( g_hmodHaPublish );
                g_hmodHaPublish = NULL;
            }
        }
    }

    Assert( !!g_hmodHaPublish == !!g_pfnHaPublishDbFailureItem );
    LeaveCriticalSection( &g_csHaPublish );
    return !!g_pfnHaPublishDbFailureItem;
#else
    AssertSz( 0, "No Ha publish support enabled / compiled in." );
    return true;
#endif
}

