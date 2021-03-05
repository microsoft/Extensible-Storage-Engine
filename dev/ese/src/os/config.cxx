// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

NTOSFuncPD( g_pfnRegOpenKeyExW, RegOpenKeyExW );
NTOSFuncPD( g_pfnRegQueryValueExW, RegQueryValueExW );
NTOSFuncPD( g_pfnRegCloseKey, RegCloseKey );
NTOSFuncPD( g_pfnRegCreateKeyExW, RegCreateKeyExW );
NTOSFuncPD( g_pfnRegDeleteValueW, RegDeleteValueW );
NTOSFuncPD( g_pfnRegSetValueExW, RegSetValueExW );

DWORD ErrorRegCreatePath( _In_z_ PCWSTR wszPath, _Inout_ HKEY * const pnthkeyPathSeg )
{

    if ( NULL == *pnthkeyPathSeg )
    {
        *pnthkeyPathSeg = HKEY_LOCAL_MACHINE;
    }
    HKEY nthkeyInitial = *pnthkeyPathSeg;

    const WCHAR wszDelim[] = L"/\\";
    WCHAR rgchPathT[_MAX_PATH];
    WCHAR *wszNextToken = NULL;

    OSStrCbCopyW( rgchPathT, sizeof(rgchPathT), wszPath );

    WCHAR* wszPathSeg = wcstok_s( rgchPathT, wszDelim, &wszNextToken );

    while ( wszPathSeg != NULL )
    {
        HKEY hkeyPathSegOld = *pnthkeyPathSeg;
        DWORD dwDisposition;
        DWORD dw = g_pfnRegCreateKeyExW(    hkeyPathSegOld,
                                            wszPathSeg,
                                            0,
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_WRITE,
                                            NULL,
                                            pnthkeyPathSeg,
                                            &dwDisposition );

        if ( hkeyPathSegOld != nthkeyInitial )
        {
            DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPathSegOld );
            Assert( dwClosedKey == ERROR_SUCCESS );
        }

        if ( dw != ERROR_SUCCESS )
        {
            return dw;
        }

        wszPathSeg = wcstok_s( NULL, wszDelim, &wszNextToken );
    }

    return ERROR_SUCCESS;
}

#ifdef DISABLE_REGISTRY

const BOOL FOSConfigGet_( __in_z const WCHAR * const wszPath, __in_z const WCHAR* const wszName, __out_bcount_z(cbBuf) WCHAR* const wszBuf, const LONG cbBuf )
{
    if ( cbBuf > 1 )
    {
        wszBuf[ 0 ] = L'\0';
    }
    return fFalse;
}

#else



LOCAL WCHAR g_wszConfigGlobal[_MAX_PATH];
LOCAL WCHAR g_wszConfigProcess[_MAX_PATH];


const BOOL FOSConfigISet( __in PCWSTR const wszPath, __in PCWSTR const wszName, __in PCWSTR const wszValue )
{


    HKEY hkeyPathSeg = NULL;
    if ( ErrorRegCreatePath( wszPath, &hkeyPathSeg ) != ERROR_SUCCESS )
    {
        return fFalse;
    }


    (void)g_pfnRegDeleteValueW( hkeyPathSeg,
                                wszName );


    DWORD dw = g_pfnRegSetValueExW( hkeyPathSeg,
                                wszName,
                                0,
                                REG_SZ,
                                (LPBYTE)wszValue,
                                (ULONG)(sizeof(WCHAR) * ( LOSStrLengthW( wszValue ) + 1 ) ) );

    AssertSz( wszValue == NULL || wszValue[0] == L'\0',
                "We are setting a reg value? %ws = [%d]%ws.\n", wszName, (sizeof(WCHAR) * ( LOSStrLengthW( wszValue ) + 1 ) ), wszValue );


    DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPathSeg );
    Assert( dwClosedKey == ERROR_SUCCESS );


    return dw == ERROR_SUCCESS;
}

const BOOL FOSConfigSet_( const WCHAR* const wszPath, const WCHAR* const wszName, const WCHAR* const wszValue )
{

    Assert( wszPath != NULL );
    Assert( LOSStrLengthW( wszPath ) > 0 );
    Assert( LOSStrLengthW( g_wszConfigProcess ) + LOSStrLengthW( wszPath ) < _MAX_PATH );
    Assert( wszPath[0] != L'/' );
    Assert( wszPath[0] != L'\\' );
    Assert( wszPath[LOSStrLengthW( wszPath ) - 1] != L'/' );
    Assert( wszPath[LOSStrLengthW( wszPath ) - 1] != L'\\' );
    Assert( wszName != NULL );
    Assert( LOSStrLengthW( wszName ) > 0 );
    Assert( wszValue != NULL );


    WCHAR wszRelPath[_MAX_PATH];
    OSStrCbCopyW( wszRelPath, sizeof(wszRelPath), wszPath );
    
    INT itch;
    for ( itch = 0; itch < _MAX_PATH && wszPath[itch]; itch++ )
    {
        if ( wszPath[itch] == L'/' )
        {
            wszRelPath[itch] = L'\\';
        }
    }
    wszRelPath[min( itch, _MAX_PATH - 1 ) ] = 0;


    WCHAR wszAbsPath[_MAX_PATH];
    OSStrCbCopyW(   wszAbsPath, sizeof(wszAbsPath), g_wszConfigProcess );
    OSStrCbAppendW( wszAbsPath, sizeof(wszAbsPath), wszRelPath );


    return FOSConfigISet( wszAbsPath, wszName, wszValue );
}


const BOOL FOSConfigIGet( __in PCWSTR const wszPath, const WCHAR* const wszName, __out_bcount(cbBuf) WCHAR* const wszBuf, const LONG cbBuf )
{

    HKEY hkeyPath;
    DWORD dw = g_pfnRegOpenKeyExW(  HKEY_LOCAL_MACHINE,
                                    wszPath,
                                    0,
                                    KEY_READ,
                                    &hkeyPath );


    if ( dw != ERROR_SUCCESS )
    {

#ifndef ESENT
        if ( dw != ERROR_ACCESS_DENIED )
        {
            (void)FOSConfigISet( wszPath, wszName, L"" );
        }
#endif


        if ( cbBuf >= sizeof( wszBuf[0] ) )
        {
            wszBuf[0] = 0;
            return fTrue;
        }
        else
        {
            return fFalse;
        }
    }


    DWORD dwType;
    DWORD cbValue = cbBuf;
    dw = g_pfnRegQueryValueExW( hkeyPath,
                            wszName,
                            0,
                            &dwType,
                            (LPBYTE)wszBuf,
                            &cbValue );


    if (    dw != ERROR_SUCCESS && dw != ERROR_MORE_DATA ||
            dwType != REG_SZ )
    {

        DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPath );
        Assert( dwClosedKey == ERROR_SUCCESS );


#ifndef ESENT
        (void)FOSConfigISet( wszPath, wszName, L"" );
#endif


        if ( cbBuf >= sizeof( wszBuf[0] ) )
        {
            wszBuf[0] = 0;
            return fTrue;
        }
        else
        {
            return fFalse;
        }
    }


    if ( cbValue > (DWORD)cbBuf )
    {
        DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPath );
        Assert( dwClosedKey == ERROR_SUCCESS );
        return fFalse;
    }

    if ( cbBuf / sizeof( WCHAR ) > sizeof( WCHAR )  )
    {
        wszBuf[ cbBuf / sizeof( WCHAR ) - 1 ] = '\0';
    }

    


    DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPath );
    Assert( dwClosedKey == ERROR_SUCCESS );
    return fTrue;
}

const BOOL FOSConfigGet_( __in_z const WCHAR * const wszPath, __in_z const WCHAR* const wszName, __out_bcount_z(cbBuf) WCHAR* const wszBuf, const LONG cbBuf )
{

    Assert( wszPath != NULL );
    Assert( LOSStrLengthW( wszPath ) > 0 );
    Assert( LOSStrLengthW( g_wszConfigGlobal ) + LOSStrLengthW( wszPath ) < _MAX_PATH );
    Assert( LOSStrLengthW( g_wszConfigProcess ) + LOSStrLengthW( wszPath ) < _MAX_PATH );
    Assert( wszPath[0] != L'/' );
    Assert( wszPath[0] != L'\\' );
    Assert( wszPath[LOSStrLengthW( wszPath ) - 1] != L'/' );
    Assert( wszPath[LOSStrLengthW( wszPath ) - 1] != L'\\' );
    Assert( wszName != NULL );
    Assert( LOSStrLengthW( wszName ) > 0 );
    Assert( wszBuf != NULL );


    WCHAR wszRelPath[_MAX_PATH];
    INT itch;
    for ( itch = 0; wszPath[itch]; itch++ )
    {
        if ( wszPath[itch] == '/' )
        {
            wszRelPath[min( itch, _MAX_PATH - 1 )] = '\\';
        }
        else
        {
            wszRelPath[min( itch, _MAX_PATH - 1 )] = wszPath[itch];
        }
    }
    wszRelPath[min( itch, _MAX_PATH - 1 )] = 0;


    WCHAR wszAbsPath[_MAX_PATH];
    OSStrCbCopyW(   wszAbsPath, sizeof(wszAbsPath), g_wszConfigProcess );
    OSStrCbAppendW( wszAbsPath, sizeof(wszAbsPath), wszRelPath );


    if ( !FOSConfigIGet( wszAbsPath, wszName, wszBuf, cbBuf ) )
    {
        return fFalse;
    }


    if ( wszBuf[0] )
    {
        return fTrue;
    }


    OSStrCbCopyW(   wszAbsPath, sizeof(wszAbsPath), g_wszConfigGlobal );
    OSStrCbAppendW( wszAbsPath, sizeof(wszAbsPath), wszRelPath );


    return FOSConfigIGet( wszAbsPath, wszName, wszBuf, cbBuf );
}

#endif




class CConfigStore
{

public:

    enum ConfigStorePopulateFlags
    {
        cspfNone        = JET_bitConfigStorePopulateControlOff,
        cspfOnRead      = JET_bitConfigStorePopulateControlOn,
    };

private:
    
    typedef struct _ConfigStoreSubStore
    {
        WCHAR *                     m_wszSubValuePath;
        HKEY                        m_nthkeySubStore;
        ERR                         m_errOpen;
        ConfigStoreReadFlags        m_csrf;
        ConfigStorePopulateFlags    m_cspf;
    } ConfigStoreSubStore;
    
    DWORD               m_errorInit;

    ConfigStoreSubStore m_rgcsss[csspMax];

    QWORD               m_qwReserved;
    WCHAR               m_wszStorePath[];

    HKEY NthkeyGetRoot( PCWSTR wszStorePath )
    {
        typedef struct _RootKeys
        {
            WCHAR *     wszHkeyLong;
            WCHAR *     wszHkeyShort;
            HKEY        hkey;
        }
        RootKeys;

        #define DefnRootHkeyPath( hkey, szHkeyShort )   { L#hkey L"\\", szHkeyShort L"\\", hkey },

        RootKeys _rootkeys [] =
        {
            DefnRootHkeyPath( HKEY_CURRENT_USER_LOCAL_SETTINGS, L"" )
            DefnRootHkeyPath( HKEY_CURRENT_USER, L"HKCU" )
            DefnRootHkeyPath( HKEY_CURRENT_CONFIG, L"HKCC" )
            DefnRootHkeyPath( HKEY_LOCAL_MACHINE, L"HKLM" )
        };

        for( ULONG irootkey = 0; irootkey < _countof(_rootkeys); irootkey++ )
        {
            if ( ( _rootkeys[irootkey].wszHkeyShort[0] != L'\\' && 0 == _wcsnicmp( wszStorePath, _rootkeys[irootkey].wszHkeyShort, wcslen(_rootkeys[irootkey].wszHkeyShort) ) ) ||
                    0 == _wcsnicmp( wszStorePath, _rootkeys[irootkey].wszHkeyLong, wcslen(_rootkeys[irootkey].wszHkeyLong) ) )
            {
                Assert( NULL != _rootkeys[irootkey].hkey );
                return _rootkeys[irootkey].hkey;
            }
        }

        AssertSz( FNegTest( fInvalidUsage ), "The specified user registry key did not begin with an acceptable well known prefix, such as HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER" );

        return NULL;
    }

public:
    CConfigStore()
    {
        memset( this, 0, sizeof(*this) );
        m_rgcsss[csspTop].m_wszSubValuePath                 = L"";
        m_rgcsss[csspSysParamDefault].m_wszSubValuePath     = JET_wszConfigStoreRelPathSysParamDefault;
        m_rgcsss[csspSysParamOverride].m_wszSubValuePath    = JET_wszConfigStoreRelPathSysParamOverride;
        m_rgcsss[csspDiag].m_wszSubValuePath                = L"Diag";
        for( ULONG icsss = 0; icsss < _countof(m_rgcsss); icsss++ )
        {
            m_rgcsss[icsss].m_errOpen = JET_errSuccess;
            m_rgcsss[icsss].m_csrf = (ConfigStoreReadFlags)JET_bitConfigStoreReadControlDefault;
            m_rgcsss[icsss].m_cspf = cspfNone;
        }
        C_ASSERT( 4 == _countof(m_rgcsss) );
        C_ASSERT( 4 == csspMax );
    }

    static ERR ErrConfigStoreCreate( _In_z_ const WCHAR * const wszStorePath, _Outptr_ CConfigStore ** ppcs );

    ERR ErrConfigStoreOpen()
    {
        ERR err = JET_errSuccess;


        const HKEY nthkeyRoot = NthkeyGetRoot( m_wszStorePath );
        if ( nthkeyRoot == NULL )
        {
            Call( ErrERRCheck( JET_errInvalidObject ) );
        }
        const WCHAR * wszStoreRelPath = wcschr( m_wszStorePath, L'\\' );
        if ( NULL == wszStoreRelPath || wszStoreRelPath[1] == L'\0' )
        {
            AssertSz( FNegTest( fInvalidUsage ), "Passed a store with no key name past the root HKLM, HKCU, etc type key." );
            Call( ErrERRCheck( JET_errInvalidParameter ) );
        }
        Assert( wszStoreRelPath[0] == L'\\' );
        wszStoreRelPath++;
        m_errorInit = g_pfnRegOpenKeyExW( nthkeyRoot, wszStoreRelPath, 0, KEY_READ, &(m_rgcsss[csspTop].m_nthkeySubStore) );
        

        if ( m_errorInit != ERROR_SUCCESS )
        {
            Assert( m_errorInit != ERROR_FILE_NOT_FOUND || ErrOSErrFromWin32Err( m_errorInit ) == JET_errFileNotFound );
            Call( m_rgcsss[csspTop].m_errOpen = ErrOSErrFromWin32Err( m_errorInit ) );
        }

HandleError:
    
        return err;
    }

    VOID ConfigStoreClose()
    {
        for( ULONG icsss = 0; icsss < _countof(m_rgcsss); icsss++ )
        {
            if ( m_rgcsss[icsss].m_nthkeySubStore )
            {
                DWORD dwClosedKey = g_pfnRegCloseKey( m_rgcsss[icsss].m_nthkeySubStore );
                Assert( dwClosedKey == ERROR_SUCCESS );
                m_rgcsss[icsss].m_nthkeySubStore = NULL;
                m_rgcsss[icsss].m_errOpen = JET_errSuccess;
            }
        }
    }

    VOID SetDefaultFlags( const ConfigStoreReadFlags csrf, const ConfigStorePopulateFlags cspf )
    {
        m_rgcsss[csspTop].m_csrf = csrf;
        m_rgcsss[csspTop].m_cspf = cspf;
    }

    ERR ErrConfigStoreIReadValue(
        _In_ const ConfigStoreSubPath                   cssp,
        _In_z_ const WCHAR * const                      wszValueName,
        _Out_writes_bytes_( cbValue ) WCHAR * const     wszValue,
        _In_range_(>=,2) ULONG                          cbValue )
    {
        ERR err = JET_errSuccess;

        Assert( wszValue );
        Assert( cbValue >= sizeof(WCHAR) );

        if ( m_rgcsss[csspTop].m_errOpen )
        {
            return ErrERRCheck( m_rgcsss[csspTop].m_errOpen );
        }

        if ( m_rgcsss[cssp].m_errOpen )
        {
            return ErrERRCheck( m_rgcsss[cssp].m_errOpen );
        }


        if ( cssp >= _countof(m_rgcsss) )
        {
            AssertSz( fFalse, "How did an unknown cssp = %d get passed in!?\n", cssp );
            return ErrERRCheck( JET_errInternalError );
        }

        Expected( cssp != csspTop || ( 0 == _wcsicmp( wszValueName, JET_wszConfigStorePopulateControl ) ) || ( 0 == _wcsicmp( wszValueName, JET_wszConfigStoreReadControl ) ) );
        if ( cssp != csspTop && ( m_rgcsss[csspTop].m_csrf & JET_bitConfigStoreReadControlDisableAll ) )
        {
            return ErrERRCheck( JET_errDisabledFunctionality );
        }

        if ( m_rgcsss[cssp].m_nthkeySubStore == NULL )
        {
            Assert( m_rgcsss[cssp].m_wszSubValuePath );

            DWORD error = g_pfnRegOpenKeyExW( m_rgcsss[csspTop].m_nthkeySubStore, m_rgcsss[cssp].m_wszSubValuePath, 0, KEY_READ, &(m_rgcsss[cssp].m_nthkeySubStore) );
            if ( error == ERROR_FILE_NOT_FOUND )
            {
                Error( m_rgcsss[cssp].m_errOpen = ErrERRCheck( JET_wrnTableEmpty ) );
            }
            Call( m_rgcsss[cssp].m_errOpen = ErrOSErrFromWin32Err( error ) );
            CallS( m_rgcsss[cssp].m_errOpen );
        }

        Assert( m_rgcsss[cssp].m_nthkeySubStore );


        DWORD dwType;
        DWORD cbStored = cbValue;
        DWORD error = g_pfnRegQueryValueExW(
                            m_rgcsss[cssp].m_nthkeySubStore,
                            wszValueName,
                            0,
                            &dwType,
                            (LPBYTE)wszValue,
                            &cbStored );


        if ( error == ERROR_SUCCESS && cbStored == 2 && wszValue[0] == L'\0' )
        {
            Error( ErrERRCheck( JET_wrnColumnNull ) );
        }

        wszValue[cbValue / sizeof(WCHAR) - 1] = L'\0';


        if ( error != ERROR_SUCCESS && error != ERROR_MORE_DATA ||
                dwType != REG_SZ )
        {

            if ( error == ERROR_FILE_NOT_FOUND )
            {
                Error( ErrERRCheck( JET_wrnColumnNull ) );
            }
            err = ErrOSErrFromWin32Err( error );
            CallS( err );
            Error( err );
        }


        if ( error == ERROR_MORE_DATA )
        {
            AssertSz( fFalse, "This means we have malformed registry data as we should always have given a big enough buffer (%d, %d).", cbValue, cbStored );
            Error( ErrERRCheck( JET_errRecordTooBig ) );
        }

        if ( cbStored == 0 )
        {
            Error( ErrERRCheck( JET_wrnColumnNull ) );
        }


        Assert( err == JET_errSuccess );

    HandleError:

        if ( ( ( m_rgcsss[csspTop].m_cspf & cspfOnRead ) != 0 ) &&
                ( err == JET_wrnColumnNull || err == JET_wrnTableEmpty ) )
        {

            HKEY nthkeyWritable = m_rgcsss[csspTop].m_nthkeySubStore;
            DWORD errorT = ErrorRegCreatePath( m_rgcsss[cssp].m_wszSubValuePath, &nthkeyWritable );
            if ( errorT == ERROR_SUCCESS )
            {
                (void)g_pfnRegSetValueExW( nthkeyWritable, wszValueName, 0, REG_SZ, (LPBYTE)NULL, 0 );
            }
        }

        return err;
    }

    ERR ErrConfigStoreIReadValue(
        _In_ const ConfigStoreSubPath   cssp,
        _In_z_ const WCHAR * const      wszValueName,
        _Out_ QWORD * const             pqwValue )
    {
        ERR err = JET_errSuccess;
        WCHAR wszQwordMax[23];

        Call( ErrConfigStoreIReadValue( cssp, wszValueName, wszQwordMax, sizeof(wszQwordMax) ) );

        if ( err != JET_wrnColumnNull && err != JET_wrnTableEmpty && wszQwordMax[0] != L'\0' )
        {
            if ( wszQwordMax[0] == L'0' && wszQwordMax[1] == L'x' )
            {
                WCHAR * pwchEnd = NULL;
                Assert( wszQwordMax[2] != L'\0' );
                *pqwValue = _wcstoi64( &wszQwordMax[2], &pwchEnd, 16 );
            }
            else
            {
                *pqwValue = _wtoi64( wszQwordMax );
            }
        }

    HandleError:
        Assert( err != JET_errDisabledFunctionality && err != JET_errUnloadableOSFunctionality );

        return err;
    }

};

ERR CConfigStore::ErrConfigStoreCreate( _In_z_ const WCHAR * const wszStorePath, _Outptr_ CConfigStore ** ppcs )
{
    ERR err = JET_errSuccess;

    Assert( ( sizeof(CConfigStore) % sizeof(WCHAR) ) == 0 );

    const ULONG cbConfigStore = sizeof(CConfigStore) + wcslen(wszStorePath) * sizeof(WCHAR) + sizeof(WCHAR) ;

    CConfigStore * pcs;
    AllocR( pcs = (CConfigStore*)( PvOSMemoryHeapAlloc( cbConfigStore ) ) );
    *ppcs = pcs;

    Assert( (ULONG_PTR)pcs->m_wszStorePath % sizeof(WCHAR) == 0 );
    const ULONG cbStorePath = cbConfigStore - sizeof(*pcs);
    Assert( cbStorePath >= ( wcslen(wszStorePath) * sizeof(WCHAR) + sizeof(WCHAR) ) );
    Assert( FBounded( pcs->m_wszStorePath, pcs->m_wszStorePath, cbStorePath ) );
    Assert( FBounded( pcs->m_wszStorePath + wcslen(wszStorePath), pcs->m_wszStorePath, cbStorePath ) );
    Assert( FBounded( pcs->m_wszStorePath, pcs, cbConfigStore ) );
    Assert( FBounded( pcs->m_wszStorePath + wcslen(wszStorePath), pcs, cbConfigStore ) );

    new(pcs) CConfigStore();
    OSStrCbCopyW( pcs->m_wszStorePath, cbStorePath, wszStorePath );

    Assert( wcslen(pcs->m_wszStorePath) == wcslen(wszStorePath) &&
            0 == memcmp( pcs->m_wszStorePath, wszStorePath, wcslen(pcs->m_wszStorePath) * sizeof(WCHAR) + sizeof(WCHAR) ) );
    Assert( pcs->m_qwReserved == 0 );

    return JET_errSuccess;
}

ERR ErrOSConfigStoreInit( _In_z_ const WCHAR * const wszPath, _Outptr_ CConfigStore ** ppcs )
{
    JET_ERR err = JET_errSuccess;
    CConfigStore * pcs = NULL;


    if ( NULL == wszPath )
    {
        AssertSz( FNegTest( fInvalidUsage ), "NULL config path is not valid." );
        return ErrERRCheck( JET_errInvalidParameter );
    }

    WCHAR wszStore[6];
    const WCHAR * wszStorePath = wcschr( wszPath, L':' );
    if ( NULL == wszStorePath )
    {
        AssertSz( FNegTest( fInvalidUsage ), "NULL config path is not valid." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    const ULONG cbStore = ( wszStorePath - wszPath + 1 ) * sizeof(WCHAR);
    if ( cbStore > sizeof(wszStore) )
    {
        AssertSz( FNegTest( fInvalidUsage ), "NULL config path is not valid." );
        return ErrERRCheck( JET_errInvalidParameter );
    }
    const ERR errT = ErrOSStrCbCopyW( wszStore, cbStore, wszPath );
    Expected( errT == JET_errBufferTooSmall );
    Enforce( wszStore[cbStore/sizeof(WCHAR)-1] == L'\0' );
    Assert( wcschr( wszStore, L':' ) == NULL );

    if ( 0 != _wcsicmp( L"reg", wszStore ) )
    {
        return ErrERRCheck( JET_errFeatureNotAvailable );
    }

    if ( wcschr( wszStorePath, L',' ) )
    {
        return ErrERRCheck( JET_errFeatureNotAvailable );
    }

    Assert( wszStorePath[0] == L':' );
    wszStorePath++;
    

    Assert( 0 == _wcsicmp( L"reg", wszStore ) );
    Call( CConfigStore::ErrConfigStoreCreate( wszStorePath, &pcs ) );


    Call( pcs->ErrConfigStoreOpen() );


    QWORD qwT = 0;
    Call( pcs->ErrConfigStoreIReadValue( csspTop, JET_wszConfigStorePopulateControl, &qwT ) );
    Assert( ( qwT & 0xffffffff00000000 ) == 0 );
    CConfigStore::ConfigStorePopulateFlags cspf = (CConfigStore::ConfigStorePopulateFlags)qwT;
    if ( JET_wrnColumnNull == err || JET_wrnTableEmpty == err )
    {
        cspf = CConfigStore::cspfNone;
        err = JET_errSuccess;
    }


    BOOL fWaitedForUserConfigComplete = fFalse;
    TICK tickStart = TickOSTimeCurrent();

    while( ( JET_errSuccess == ( err = pcs->ErrConfigStoreIReadValue( csspTop, JET_wszConfigStoreReadControl, &qwT ) ) ) &&
            ( qwT & JET_bitConfigStoreReadControlInhibitRead ) )
    {

        if ( DtickDelta( tickStart, TickOSTimeCurrent() ) > 1500 )
        {
            const WCHAR * rgwszT[] = { JET_wszConfigStoreReadControl };
            UtilReportEvent(    eventWarning,
                                GENERAL_CATEGORY,
                                CONFIG_STORE_READ_INHIBIT_PAUSE_ID,
                                _countof(rgwszT), rgwszT );
        }
        fWaitedForUserConfigComplete = fTrue;
        UtilSleep( 50 );
    }
    if ( err == JET_wrnColumnNull || err == JET_wrnTableEmpty )
    {
        qwT = JET_bitConfigStoreReadControlDefault;
        err = JET_errSuccess;
    }
    Assert( ( qwT & 0xffffffff00000000 ) == 0 );
    const ConfigStoreReadFlags csrf = (ConfigStoreReadFlags)qwT;
    if ( fWaitedForUserConfigComplete )
    {
        err = ErrERRCheck( wrnSlow );
    }

    pcs->SetDefaultFlags( csrf, cspf );

    if ( csrf & JET_bitConfigStoreReadControlDisableAll )
    {
        Call( ErrERRCheck( JET_errDisabledFunctionality ) );
    }

    Assert( err == JET_errSuccess || err == wrnSlow );

    *ppcs = pcs;
    pcs = NULL;

HandleError:

    if ( NULL != pcs )
    {
        Assert( err < JET_errSuccess );
        OSConfigStoreTerm( pcs );
        pcs = NULL;
    }

    return err;
}

void OSConfigStoreTerm( _In_ CConfigStore * pcs )
{
    if ( pcs )
    {
        pcs->ConfigStoreClose();
    }

    OSMemoryHeapFree( pcs );
}

BOOL FConfigValuePresent( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName )
{
    if ( pcs == NULL )
    {
        AssertSz( fFalse, "You didn't allocate / init the config store" );
        return ErrERRCheck( errNotFound );
    }

    Assert( cssp != csspTop );
    Assert( 0 != _wcsicmp( JET_wszConfigStoreReadControl, wszValueName ) );
    Assert( 0 != _wcsicmp( JET_wszConfigStorePopulateControl, wszValueName ) );

    WCHAR wszQwordMax[23];
    return JET_errSuccess == pcs->ErrConfigStoreIReadValue( cssp, wszValueName, wszQwordMax, sizeof(wszQwordMax) );
}

BOOL FConfigValuePresent( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const CHAR * const szValueName )
{
    WCHAR wszValueName[100];
    OSStrCbFormatW( wszValueName, sizeof(wszValueName), L"%hs", szValueName );
    return FConfigValuePresent( pcs, cssp, wszValueName );
}

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName, _Out_ QWORD * pqwValue )
{
    if ( pcs == NULL )
    {
        AssertSz( fFalse, "You didn't allocate / init the config store" );
        return ErrERRCheck( errNotFound );
    }

    Assert( cssp != csspTop );
    Assert( 0 != _wcsicmp( JET_wszConfigStoreReadControl, wszValueName ) );
    Assert( 0 != _wcsicmp( JET_wszConfigStorePopulateControl, wszValueName ) );

    return pcs->ErrConfigStoreIReadValue( cssp, wszValueName, pqwValue );
}

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName, _Out_ ULONG * pulValue )
{
    QWORD qwValue;

    if ( pcs == NULL )
    {
        AssertSz( fFalse, "You didn't allocate / init the config store" );
        return ErrERRCheck( errNotFound );
    }

    Assert( cssp != csspTop );
    Assert( 0 != _wcsicmp( JET_wszConfigStoreReadControl, wszValueName ) );
    Assert( 0 != _wcsicmp( JET_wszConfigStorePopulateControl, wszValueName ) );

    const ERR err = pcs->ErrConfigStoreIReadValue( cssp, wszValueName, &qwValue );
    if ( err == JET_errSuccess )
    {
        if ( ( qwValue & 0xffffffff00000000 ) != 0 &&
                ( qwValue & 0xffffffff00000000 ) != 0xffffffff00000000  )
        {
            AssertSz( fFalse, "Someone loaded the registry with a 64-bit value, when we only accept 32-bit values for this." );
            return ErrERRCheck( JET_errRecordTooBig );
        }
        *pulValue = (ULONG)qwValue;
    }

    return err;
}

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const CHAR * const szValueName, _Out_ QWORD * pqwValue )
{
    WCHAR wszValueName[100];
    OSStrCbFormatW( wszValueName, sizeof(wszValueName), L"%hs", szValueName );
    return ErrConfigReadValue( pcs, cssp, wszValueName, pqwValue );
}

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const CHAR * const szValueName, _Out_ ULONG * pulValue )
{
    WCHAR wszValueName[100];
    OSStrCbFormatW( wszValueName, sizeof(wszValueName), L"%hs", szValueName );
    return ErrConfigReadValue( pcs, cssp, wszValueName, pulValue );
}


void OSConfigPostterm()
{
}


BOOL FOSConfigPreinit()
{
#ifndef DISABLE_REGISTRY

    OSStrCbFormatW( g_wszConfigGlobal, sizeof( g_wszConfigGlobal ),
        L"SOFTWARE\\Microsoft\\%ws\\Global\\", WszUtilImageVersionName( ) );

    OSStrCbFormatW( g_wszConfigProcess, sizeof( g_wszConfigProcess ),
        L"SOFTWARE\\Microsoft\\%ws\\Process\\%ws\\", WszUtilImageVersionName( ), WszUtilProcessName( ) );
#endif


    NTOSFuncErrorPreinit( g_pfnRegOpenKeyExW, g_mwszzRegistryLibs, RegOpenKeyExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegQueryValueExW, g_mwszzRegistryLibs, RegQueryValueExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegCloseKey, g_mwszzRegistryLibs, RegCloseKey, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegCreateKeyExW, g_mwszzRegistryLibs, RegCreateKeyExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegDeleteValueW, g_mwszzRegistryLibs, RegDeleteValueW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegSetValueExW, g_mwszzRegistryLibs, RegSetValueExW, oslfExpectedOnWin5x | oslfRequired );

    return fTrue;
}


void OSConfigTerm()
{
}


ERR ErrOSConfigInit()
{

    return JET_errSuccess;
}


