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
    //  create / open each path segment, returning error on any failures

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

#else  //  !DISABLE_REGISTRY

//  OS Registry Functionality Auto-Loads
//

//  Persistent Configuration Management
//
//  Configuration information is organized in a way very similar to a file
//  system.  Each configuration datum is a name and value pair stored in a
//  path.  All paths, names, ans values are text strings.
//
//  For Win32, we will store this information in the registry under the
//  following two paths:
//
//    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\<Image Name>\Global\
//    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\<Image Name>\Process\<Process Name>\
//
//  Global settings are overridden by Process settings.

LOCAL WCHAR g_wszConfigGlobal[_MAX_PATH];
LOCAL WCHAR g_wszConfigProcess[_MAX_PATH];

//  sets the specified name in the specified path to the given value, returning
//  fTrue on success.  any intermediate path that does not exist will be created
//
//  NOTE:  either '/' or '\\' is a valid path separator

const BOOL FOSConfigISet( _In_ PCWSTR const wszPath, _In_ PCWSTR const wszName, _In_ PCWSTR const wszValue )
{

    //  create the path, returning fFalse on any failures

    HKEY hkeyPathSeg = NULL;
    if ( ErrorRegCreatePath( wszPath, &hkeyPathSeg ) != ERROR_SUCCESS )
    {
        return fFalse;
    }

    //  delete existing name so that in case it has the wrong type, there will
    //  be no problems setting it

    (void)g_pfnRegDeleteValueW( hkeyPathSeg,
                                wszName );

    //  set name to value

    DWORD dw = g_pfnRegSetValueExW( hkeyPathSeg,
                                wszName,
                                0,
                                REG_SZ,
                                (LPBYTE)wszValue,
                                (ULONG)(sizeof(WCHAR) * ( LOSStrLengthW( wszValue ) + 1 ) ) );

    AssertSz( wszValue == NULL || wszValue[0] == L'\0',
                "We are setting a reg value? %ws = [%d]%ws.\n", wszName, (sizeof(WCHAR) * ( LOSStrLengthW( wszValue ) + 1 ) ), wszValue );

    //  close path

    DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPathSeg );
    Assert( dwClosedKey == ERROR_SUCCESS );

    //  return result of setting the name to the value as our success

    return dw == ERROR_SUCCESS;
}

const BOOL FOSConfigSet_( const WCHAR* const wszPath, const WCHAR* const wszName, const WCHAR* const wszValue )
{
    //  validate IN args

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

    //  convert any '/' in the relative path into '\\'

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

    //  build the absolute path to our process configuration

    WCHAR wszAbsPath[_MAX_PATH];
    OSStrCbCopyW(   wszAbsPath, sizeof(wszAbsPath), g_wszConfigProcess );
    OSStrCbAppendW( wszAbsPath, sizeof(wszAbsPath), wszRelPath );

    //  set our process configuration

    return FOSConfigISet( wszAbsPath, wszName, wszValue );
}

//  gets the value of the specified name in the specified path and places it in
//  the provided buffer, returning fFalse if the buffer is too small.  if the
//  value is not set, an empty string will be returned
//
//  NOTE:  either '/' or '\\' is a valid path separator

const BOOL FOSConfigIGet( _In_ PCWSTR const wszPath, const WCHAR* const wszName, __out_bcount(cbBuf) WCHAR* const wszBuf, const LONG cbBuf )
{
    //  open registry key with this path

    HKEY hkeyPath;
    DWORD dw = g_pfnRegOpenKeyExW(  HKEY_LOCAL_MACHINE,
                                    wszPath,
                                    0,
                                    KEY_READ,
                                    &hkeyPath );

    //  we failed to open the key

    if ( dw != ERROR_SUCCESS )
    {
        //  create the key / value with a NULL value, ignoring any errors

#ifndef ESENT
        //  do not try to create keys in windows build, perf issue on non-RTM
        //  build under app-container
        if ( dw != ERROR_ACCESS_DENIED )
        {
            (void)FOSConfigISet( wszPath, wszName, L"" );
        }
#endif

        //  return an empty string if space permits

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

    //  retrieve the value into the user buffer

    DWORD dwType;
    DWORD cbValue = cbBuf;
    dw = g_pfnRegQueryValueExW( hkeyPath,
                            wszName,
                            0,
                            &dwType,
                            (LPBYTE)wszBuf,
                            &cbValue );

    //  there was some error reading the value

    if (    dw != ERROR_SUCCESS && dw != ERROR_MORE_DATA ||
            dwType != REG_SZ )
    {
        //  close the key

        DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPath );
        Assert( dwClosedKey == ERROR_SUCCESS );

        //  create the key / value with a NULL value, ignoring any errors

#ifndef ESENT
        //  do not try to create keys in windows build, perf issue on non-RTM
        //  build under app-container
        (void)FOSConfigISet( wszPath, wszName, L"" );
#endif

        //  return an empty string if space permits

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

    //  if the value is bigger than the buffer, return fFalse

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

    /*
    if ( cbValue > 2 )
    {
        AssertSz( 0 == _wcsicmp( wszName, L"DoesntExist" ) ||
                    0 == _wcsicmp( wszName, L"System PIB Failures" ) || //  Huh? This set to zero, so why is this set to 0?
                    //Done: 0 == _wcsicmp( wszName, L"Initial Log Generation" ) ||
                    //Done: 0 == _wcsicmp( wszName, L"FTL Trace File" ) ||
                    0 == _wcsicmp( wszName, L"Fault Injection" ) ||
                    0 == _wcsicmp( wszName, L"Mem Check" )
                    ,
                    "We have a unknown non-null value read from the registry: %ws = [%d]%ws.\n", wszName, cbValue, wszBuf );
    }
    //*/

    //  we succeeded in reading the value, so close the key and return

    DWORD dwClosedKey = g_pfnRegCloseKey( hkeyPath );
    Assert( dwClosedKey == ERROR_SUCCESS );
    return fTrue;
}

const BOOL FOSConfigGet_( __in_z const WCHAR * const wszPath, __in_z const WCHAR* const wszName, __out_bcount_z(cbBuf) WCHAR* const wszBuf, const LONG cbBuf )
{
    //  validate IN args

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

    //  convert any '/' in the relative path into '\\'

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

    //  build the absolute path to our process configuration

    WCHAR wszAbsPath[_MAX_PATH];
    OSStrCbCopyW(   wszAbsPath, sizeof(wszAbsPath), g_wszConfigProcess );
    OSStrCbAppendW( wszAbsPath, sizeof(wszAbsPath), wszRelPath );

    //  get our process configuration, failing on insufficient buffer

    if ( !FOSConfigIGet( wszAbsPath, wszName, wszBuf, cbBuf ) )
    {
        return fFalse;
    }

    //  if we have process specific configuration, we're done

    if ( wszBuf[0] )
    {
        return fTrue;
    }

    //  build the absolute path to our global configuration

    OSStrCbCopyW(   wszAbsPath, sizeof(wszAbsPath), g_wszConfigGlobal );
    OSStrCbAppendW( wszAbsPath, sizeof(wszAbsPath), wszRelPath );

    //  return our global configuration, whatever it is

    return FOSConfigIGet( wszAbsPath, wszName, wszBuf, cbBuf );
}

#endif  //  DISABLE_REGISTRY

//  OSConfig V2

//  In the root of the config store is these values:
//      CsReadControl       REG_DWORD   See ConfigStoreReadFlags
//      CsPopulateControl   REG_DWORD   See ConfigStorePopulateFlags

//  Note: All OSConfig* functions must be designed to run before ErrOSInit/ErrOSConfigInit().

class CConfigStore
{

public:

    enum ConfigStorePopulateFlags   //  cspf
    {
        cspfNone        = JET_bitConfigStorePopulateControlOff, //  By default (on retail) we will not populate in the registry any settings.
        cspfOnRead      = JET_bitConfigStorePopulateControlOn,  //  Populate all settings that the ESE engine actually tried to look up.  This is
                                //  the default for DEBUG.
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

    QWORD               m_qwReserved;   //  guarantee alignment
    WCHAR               m_wszStorePath[];
    //  MUST BE LAST ELEMENT

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
            // A list of possible cases I found, x are included, o are not:
            //  o HKEY_CLASSES_ROOT
            //  x HKEY_CURRENT_CONFIG               - Accepted.
            //  x HKEY_CURRENT_USER                 - Accepted.
            //  x HKEY_CURRENT_USER_LOCAL_SETTINGS  - Accepted.  (Key is supported starting with Windows 7 and Windows Server 2008 R2.)
            //  x HKEY_LOCAL_MACHINE                - Accepted.
            //  o HKEY_PERFORMANCE_DATA
            //  o HKEY_PERFORMANCE_NLSTEXT
            //  o HKEY_PERFORMANCE_TEXT
            //  o HKEY_USERS
            DefnRootHkeyPath( HKEY_CURRENT_USER_LOCAL_SETTINGS, L"" )
            DefnRootHkeyPath( HKEY_CURRENT_USER, L"HKCU" )
            DefnRootHkeyPath( HKEY_CURRENT_CONFIG, L"HKCC" )
            DefnRootHkeyPath( HKEY_LOCAL_MACHINE, L"HKLM" )
        };

        for( ULONG irootkey = 0; irootkey < _countof(_rootkeys); irootkey++ )
        {
            if ( ( _rootkeys[irootkey].wszHkeyShort[0] != L'\\' && 0 == _wcsnicmp( wszStorePath, _rootkeys[irootkey].wszHkeyShort, LOSStrLengthW(_rootkeys[irootkey].wszHkeyShort) ) ) ||
                    0 == _wcsnicmp( wszStorePath, _rootkeys[irootkey].wszHkeyLong, LOSStrLengthW(_rootkeys[irootkey].wszHkeyLong) ) )
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

        //  open registry key with this path

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
        
        //  we failed to open the key

        if ( m_errorInit != ERROR_SUCCESS )
        {
            //  if the root reg key of the config store doesn't exist, we translate ERROR_FILE_NOT_FOUND
            //  to JET_errFileNotFound, it is a contract we should maintain into the future.
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
                //  do we need to reset m_csrf, m_cspf?  Don't think so.
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
        Assert( cbValue >= sizeof(WCHAR) ); // must be 2 bytes for NUL termination

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
            AssertSz( fFalse, "How did an unknown cssp = %d get passed in!?\n", cssp ); // should never happen.
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
                //  means the key simply doesn't exist, we treat this as value null.
                //  Note: This makes the error "sticky", and if someone later adds the regkey, we won't
                //  catch on until the next init.
                Error( m_rgcsss[cssp].m_errOpen = ErrERRCheck( JET_wrnTableEmpty ) );
            }
            Call( m_rgcsss[cssp].m_errOpen = ErrOSErrFromWin32Err( error ) );
            CallS( m_rgcsss[cssp].m_errOpen );
        }

        Assert( m_rgcsss[cssp].m_nthkeySubStore );

        //  retrieve the value into the user buffer

        DWORD dwType;
        DWORD cbStored = cbValue;
        DWORD error = g_pfnRegQueryValueExW(
                            m_rgcsss[cssp].m_nthkeySubStore,
                            wszValueName,
                            0,
                            &dwType,
                            (LPBYTE)wszValue,
                            &cbStored );

        //  the value is "there", but actually L"" - up convert this to an official null

        if ( error == ERROR_SUCCESS && cbStored == 2 && wszValue[0] == L'\0' )
        {
            Error( ErrERRCheck( JET_wrnColumnNull ) );
        }

        //  ensure NUL termination
        wszValue[cbValue / sizeof(WCHAR) - 1] = L'\0';

        //  there was some error reading the value

        if ( error != ERROR_SUCCESS && error != ERROR_MORE_DATA ||
                dwType != REG_SZ )
        {

            if ( error == ERROR_FILE_NOT_FOUND )
            {
                Error( ErrERRCheck( JET_wrnColumnNull ) );
            }
            //  some unknown error, there might be a better response to map this from.
            err = ErrOSErrFromWin32Err( error );
            CallS( err );   //  should not have thrown a warning.
            Error( err );
        }

        //  was the value bigger than the buffer / truncated ...

        if ( error == ERROR_MORE_DATA )
        {
            //  too small buffer truncated read value ...
            AssertSz( fFalse, "This means we have malformed registry data as we should always have given a big enough buffer (%d, %d).", cbValue, cbStored );
            Error( ErrERRCheck( JET_errRecordTooBig ) );
        }

        if ( cbStored == 0 )
        {
            Error( ErrERRCheck( JET_wrnColumnNull ) );
        }

        //  we succeeded in reading the value

        Assert( err == JET_errSuccess );

    HandleError:

        if ( ( ( m_rgcsss[csspTop].m_cspf & cspfOnRead ) != 0 ) &&
                ( err == JET_wrnColumnNull || err == JET_wrnTableEmpty ) )
        {
            //  client wants the read values populated ...

            HKEY nthkeyWritable = m_rgcsss[csspTop].m_nthkeySubStore;
            DWORD errorT = ErrorRegCreatePath( m_rgcsss[cssp].m_wszSubValuePath, &nthkeyWritable );
            if ( errorT == ERROR_SUCCESS )
            {
                //  Set to a NUL value
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

        if ( err != JET_wrnColumnNull && err != JET_wrnTableEmpty && /* maybe check isalpha ? */wszQwordMax[0] != L'\0' )
        {
            if ( wszQwordMax[0] == L'0' && wszQwordMax[1] == L'x' )
            {
                WCHAR * pwchEnd = NULL;
                Assert( wszQwordMax[2] != L'\0' );  //  well what if it does?  kind of bad data.
                *pqwValue = _wcstoi64( &wszQwordMax[2], &pwchEnd, 16 );
                // what if pwchEnd != L'\0'?  kind of bad data.
            }
            else
            {
                *pqwValue = _wtoi64( wszQwordMax );
            }
        }

    HandleError:
        //  will it hold?  hope so.
        Assert( err != JET_errDisabledFunctionality && err != JET_errUnloadableOSFunctionality );

        return err;
    }

};  //  end class CConfigStore

ERR CConfigStore::ErrConfigStoreCreate( _In_z_ const WCHAR * const wszStorePath, _Outptr_ CConfigStore ** ppcs )
{
    ERR err = JET_errSuccess;

    Assert( ( sizeof(CConfigStore) % sizeof(WCHAR) ) == 0 );    //  or alignment problem later ...

    const ULONG cbConfigStore = sizeof(CConfigStore) + LOSStrLengthW(wszStorePath) * sizeof(WCHAR) + sizeof(WCHAR) /* NUL term */;

    CConfigStore * pcs;
    AllocR( pcs = (CConfigStore*)( PvOSMemoryHeapAlloc( cbConfigStore ) ) );
    *ppcs = pcs;

    Assert( (ULONG_PTR)pcs->m_wszStorePath % sizeof(WCHAR) == 0 );
    const ULONG cbStorePath = cbConfigStore - sizeof(*pcs);
    Assert( cbStorePath >= ( LOSStrLengthW(wszStorePath) * sizeof(WCHAR) + sizeof(WCHAR) ) );
    Assert( FBounded( pcs->m_wszStorePath, pcs->m_wszStorePath, cbStorePath ) );
    Assert( FBounded( pcs->m_wszStorePath + LOSStrLengthW(wszStorePath), pcs->m_wszStorePath, cbStorePath ) );
    Assert( FBounded( pcs->m_wszStorePath, pcs, cbConfigStore ) );
    Assert( FBounded( pcs->m_wszStorePath + LOSStrLengthW(wszStorePath), pcs, cbConfigStore ) );

    new(pcs) CConfigStore();
    OSStrCbCopyW( pcs->m_wszStorePath, cbStorePath, wszStorePath );

    //  wether we copy str or .ctor first, ensure both elements seem intact.
    Assert( LOSStrLengthW(pcs->m_wszStorePath) == LOSStrLengthW(wszStorePath) &&
            0 == memcmp( pcs->m_wszStorePath, wszStorePath, LOSStrLengthW(pcs->m_wszStorePath) * sizeof(WCHAR) + sizeof(WCHAR) ) );
    Assert( pcs->m_qwReserved == 0 );

    return JET_errSuccess;
}

ERR ErrOSConfigStoreInit( _In_z_ const WCHAR * const wszPath, _Outptr_ CConfigStore ** ppcs )
{
    JET_ERR err = JET_errSuccess;
    CConfigStore * pcs = NULL;

    //
    //  Validate Args, and Separate the Store and the Store-Specific Path from each other
    //

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
    Expected( errT == JET_errBufferTooSmall );  //  we expected to truncate.
    Enforce( wszStore[cbStore/sizeof(WCHAR)-1] == L'\0' );  // depending OSStrCbCopyW()s NUL termination behavior
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
    wszStorePath++; // increment past ":"
    
    //
    //  Initialize the store.
    //

    Assert( 0 == _wcsicmp( L"reg", wszStore ) );
    Call( CConfigStore::ErrConfigStoreCreate( wszStorePath, &pcs ) );

    //  Open the store.

    Call( pcs->ErrConfigStoreOpen() );

    //  Configure the store.

    QWORD qwT = 0;
    Call( pcs->ErrConfigStoreIReadValue( csspTop, JET_wszConfigStorePopulateControl, &qwT ) );
    Assert( ( qwT & 0xffffffff00000000 ) == 0 );
    CConfigStore::ConfigStorePopulateFlags cspf = (CConfigStore::ConfigStorePopulateFlags)qwT;
    if ( JET_wrnColumnNull == err || JET_wrnTableEmpty == err )
    {
        //  This is fine, we will assume the default
        cspf = CConfigStore::cspfNone;
        err = JET_errSuccess;
    }

    //  Spin waiting for the read control flags to show up

    BOOL fWaitedForUserConfigComplete = fFalse;
    TICK tickStart = TickOSTimeCurrent();

    while( ( JET_errSuccess == ( err = pcs->ErrConfigStoreIReadValue( csspTop, JET_wszConfigStoreReadControl, &qwT ) ) ) &&
            ( qwT & JET_bitConfigStoreReadControlInhibitRead ) )
    {
        //  user or program must still be setting up registry ... if you're stuck here,
        //  set EseRegRoot\ REG_SZ "CsReadControl" from "1" (pause read) to "0" (default
        //  = enabled) or "2" (disabled).

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
        //  materialize this error now
        Call( ErrERRCheck( JET_errDisabledFunctionality ) );
    }

    Assert( err == JET_errSuccess || err == wrnSlow );

    *ppcs = pcs;
    pcs = NULL;

HandleError:

    if ( NULL != pcs )
    {
        //  failed, cleanup
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
        //  This can happen and is expected if no one sets the JET_paramConfigStoreSpec
        AssertSz( fFalse, "You didn't allocate / init the config store" );
        return ErrERRCheck( errNotFound );
    }

    Assert( cssp != csspTop );      //  top is reserved for config store specific stuff
    Assert( 0 != _wcsicmp( JET_wszConfigStoreReadControl, wszValueName ) );     // preserving ability to define these at each scope
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
        //  This can happen and is expected if no one sets the JET_paramConfigStoreSpec
        AssertSz( fFalse, "You didn't allocate / init the config store" );
        return ErrERRCheck( errNotFound );
    }

    Assert( cssp != csspTop );      //  top is reserved for config store specific stuff
    Assert( 0 != _wcsicmp( JET_wszConfigStoreReadControl, wszValueName ) );     // preserving ability to define these at each scope
    Assert( 0 != _wcsicmp( JET_wszConfigStorePopulateControl, wszValueName ) );

    return pcs->ErrConfigStoreIReadValue( cssp, wszValueName, pqwValue );
}

ERR ErrConfigReadValue( _In_ CConfigStore * const pcs, ConfigStoreSubPath cssp, _In_z_ const WCHAR * const wszValueName, _Out_ ULONG * pulValue )
{
    QWORD qwValue;

    if ( pcs == NULL )
    {
        //  This can happen and is expected if no one sets the JET_paramConfigStoreSpec
        AssertSz( fFalse, "You didn't allocate / init the config store" );
        return ErrERRCheck( errNotFound );
    }

    Assert( cssp != csspTop );      //  top is reserved for config store specific stuff
    Assert( 0 != _wcsicmp( JET_wszConfigStoreReadControl, wszValueName ) );     // preserving ability to define these at each scope
    Assert( 0 != _wcsicmp( JET_wszConfigStorePopulateControl, wszValueName ) );

    const ERR err = pcs->ErrConfigStoreIReadValue( cssp, wszValueName, &qwValue );
    if ( err == JET_errSuccess )
    {
        if ( ( qwValue & 0xffffffff00000000 ) != 0 &&
                ( qwValue & 0xffffffff00000000 ) != 0xffffffff00000000 /* negative value */ )
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

//  post-terminates the configuration subsystem

void OSConfigPostterm()
{
    //  nop
}

//  pre-initializes the configuration subsystem

BOOL FOSConfigPreinit()
{
#ifndef DISABLE_REGISTRY
    //  build configuration paths

    OSStrCbFormatW( g_wszConfigGlobal, sizeof( g_wszConfigGlobal ),
        L"SOFTWARE\\Microsoft\\%ws\\Global\\", WszUtilImageVersionName( ) );

    OSStrCbFormatW( g_wszConfigProcess, sizeof( g_wszConfigProcess ),
        L"SOFTWARE\\Microsoft\\%ws\\Process\\%ws\\", WszUtilImageVersionName( ), WszUtilProcessName( ) );
#endif // DISABLE_REGISTRY

    //  load OS registry APIs

    NTOSFuncErrorPreinit( g_pfnRegOpenKeyExW, g_mwszzRegistryLibs, RegOpenKeyExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegQueryValueExW, g_mwszzRegistryLibs, RegQueryValueExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegCloseKey, g_mwszzRegistryLibs, RegCloseKey, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegCreateKeyExW, g_mwszzRegistryLibs, RegCreateKeyExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegDeleteValueW, g_mwszzRegistryLibs, RegDeleteValueW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncErrorPreinit( g_pfnRegSetValueExW, g_mwszzRegistryLibs, RegSetValueExW, oslfExpectedOnWin5x | oslfRequired );

    return fTrue;
}

//  terminate config subsystem

void OSConfigTerm()
{
    //  nop
}

//  init config subsystem

ERR ErrOSConfigInit()
{
    //  nop

    return JET_errSuccess;
}


