// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

#if defined(ESENT)
#include <slc.h>
#include <slerror.h>
#elif defined(ESEEX)
#include "bldver.h"
#elif !( defined(PRODUCT_MAJOR) && defined(PRODUCT_MINOR) && defined(BUILD_MAJOR) && defined(BUILD_MINOR) )
#error "Please define PRODUCT_MAJOR, PRODUCT_MINOR, BUILD_MAJOR, and BUILD_MINOR in the build environment"
#endif

#ifndef RTM
LOCAL VOID GetSystemVersionOverridesFromRegistry();
#else
#define GetSystemVersionOverridesFromRegistry()
#endif

extern void OSIProcessAbort();


extern volatile BOOL    g_fDllUp;

class SysinfoBufferLifetimeManager
{
public:

    void FreeSysinfoBufferLifetimeManager()
    {
        AssertSz( !g_fDllUp, "This cleanup function is intended to be called only when the DLL is being unloaded." );

        if ( m_wszProcessName )
        {
            LocalFree( m_wszProcessName );
            m_wszProcessName = NULL;
        }
        if ( m_wszProcessFileName )
        {
            LocalFree( m_wszProcessFileName );
            m_wszProcessFileName = NULL;
        }
        if ( m_wszProcessFriendlyName )
        {
            LocalFree( m_wszProcessFriendlyName );
            m_wszProcessFriendlyName = NULL;
        }
        if ( m_wszProcessPath )
        {
            LocalFree( m_wszProcessPath );
            m_wszProcessPath = NULL;
        }
        if ( m_wszImageName )
        {
            LocalFree( m_wszImageName );
            m_wszImageName = NULL;
        }
        if ( m_wszImagePath )
        {
            LocalFree( m_wszImagePath );
            m_wszImagePath = NULL;
        }
        if ( m_wszImageBuildClass )
        {
            LocalFree( m_wszImageBuildClass );
            m_wszImageBuildClass = NULL;
        }
    }

    JET_ERR ErrAllocateIfNeeded()
    {
        JET_ERR err = JET_errSuccess;
        m_fAllocationAttempted = fTrue;


        if ( !m_wszProcessFriendlyName )
        {
            Alloc( m_wszProcessFriendlyName = (WCHAR*) LocalAlloc( 0, cbProcessFriendlyName ) );
        }

        m_fAllocationSucceeded = fTrue;
HandleError:
        return err;
    }

    BOOL FAllocationAttempted() const
    {
        Assert( m_fAllocationAttempted );
        return m_fAllocationAttempted;
    }

    BOOL FAllocationSucceeded() const
    {
        Assert( m_fAllocationAttempted );
        return m_fAllocationSucceeded;
    }


    const WCHAR* WszProcessFriendlyName() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszProcessFriendlyName;
    }
    ERR ErrSetProcessFriendlyName(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrOSStrCbCopyW( m_wszProcessFriendlyName, cbProcessFriendlyName, wszNewValue );
    }
    static const size_t cbProcessFriendlyName = _MAX_FNAME * sizeof( WCHAR );

    const WCHAR* WszProcessName() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszProcessName;
    }
    ERR ErrSetProcessName(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrStrDupLocalAlloc( &m_wszProcessName, wszNewValue, wcslen( wszNewValue ) );
    }


    const WCHAR* WszProcessFileName() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszProcessFileName;
    }
    ERR ErrSetProcessFileName(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrStrDupLocalAlloc( &m_wszProcessFileName, wszNewValue, wcslen( wszNewValue ) );
    }


    const WCHAR* WszProcessPath() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszProcessPath;
    }
    ERR ErrSetProcessPath(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrStrDupLocalAlloc( &m_wszProcessPath, wszNewValue, wcslen( wszNewValue ) );
    }

    const WCHAR* WszImageName() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszImageName;
    }
    ERR ErrSetImageName(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrStrDupLocalAlloc( &m_wszImageName, wszNewValue, wcslen( wszNewValue ) );
    }

    const WCHAR* WszImagePath() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszImagePath;
    }
    ERR ErrSetImagePath(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrStrDupLocalAlloc( &m_wszImagePath, wszNewValue, wcslen( wszNewValue ) );
    }

    const WCHAR* WszUtilImageBuildClass() const
    {
        Assert( m_fAllocationAttempted );
        return m_wszImageBuildClass;
    }
    ERR ErrSetUtilImageBuildClass(
        _In_ PCWSTR wszNewValue )
    {
        Assert( m_fAllocationAttempted );
        return ErrStrDupLocalAlloc( &m_wszImageBuildClass, wszNewValue, wcslen( wszNewValue ) );
    }


private:
    WCHAR* m_wszProcessName;
    WCHAR* m_wszProcessFileName;
    WCHAR* m_wszProcessFriendlyName;
    WCHAR* m_wszProcessPath;
    WCHAR* m_wszImageName;
    WCHAR* m_wszImagePath;
    WCHAR* m_wszImageBuildClass;
    BOOL m_fAllocationAttempted;
    BOOL m_fAllocationSucceeded;

    ERR ErrStrDupLocalAlloc(
        _Out_z_capcount_( cchNewValue + 1 ) PWSTR* pwszDestination,
        _In_opt_z_count_( cchNewValue ) PCWSTR wszNewValue,
        _In_ const size_t cchNewValue )
    {
        ERR err = JET_errSuccess;

        Assert( NULL != pwszDestination );
        Assert( NULL == *pwszDestination );

        *pwszDestination = NULL;

        if ( NULL != wszNewValue )
        {
            const size_t cchAlloc = cchNewValue + 1;
            const size_t cbAlloc = cchAlloc * sizeof( wszNewValue[ 0 ] );
            if ( cchAlloc >= cchNewValue && cchAlloc >= 1 )
            {
                Alloc( *pwszDestination = (WCHAR*) LocalAlloc( 0, cbAlloc ) );
            }
            else{
                AssertSz( FALSE, "cchAlloc Overflow." );
                Error( ErrERRCheck( JET_errOutOfMemory ) );
            }
            Call( ErrOSStrCbCopyW( *pwszDestination, cbAlloc, wszNewValue ) );

            Assert( wcslen( *pwszDestination ) == wcslen( wszNewValue ) );
        }

HandleError:
        return err;
    }

} g_sysinfobuffers;




const WCHAR* WszUtilProcessName()
{
    return g_sysinfobuffers.WszProcessName();
}


const WCHAR* WszUtilProcessFileName()
{
    return g_sysinfobuffers.WszProcessFileName();
}


const WCHAR* WszUtilProcessFriendlyName()
{
    return g_sysinfobuffers.WszProcessFriendlyName();
}


const WCHAR* WszUtilProcessPath()
{
    return g_sysinfobuffers.WszProcessPath();
}


LOCAL DWORD g_dwProcessId;

const DWORD DwUtilProcessId()
{
    return g_dwProcessId;
}


LOCAL DWORD cProcessProcessor;

const DWORD CUtilProcessProcessor()
{
    return OSSyncGetProcessorCountMax();
}


volatile BOOL g_fProcessAbort = fFalse;

const BOOL FUtilProcessAbort()
{
    return g_fProcessAbort;
}




LOCAL DWORD g_dwSystemVersionMajor;

DWORD DwUtilSystemVersionMajor()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemVersionMajor;
}


LOCAL DWORD g_dwSystemVersionMinor;

DWORD DwUtilSystemVersionMinor()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemVersionMinor;
}


LOCAL DWORD g_dwSystemBuildNumber;

DWORD DwUtilSystemBuildNumber()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemBuildNumber;
}


LOCAL DWORD g_dwSystemServicePackNumber;

DWORD DwUtilSystemServicePackNumber()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemServicePackNumber;
}

#ifndef OnDebugOrRetail

#ifdef DEBUG
#define OnDebugOrRetail( debugvalue, retailvalue )      debugvalue
#else
#define OnDebugOrRetail( debugvalue, retailvalue )      retailvalue
#endif

#ifdef DEBUG
#define OnDebugOrRetailOrRtm( debugvalue, retailvalue, rtmvalue )       debugvalue
#else
#ifdef RTM
#define OnDebugOrRetailOrRtm( debugvalue, retailvalue, rtmvalue )       rtmvalue
#else
#define OnDebugOrRetailOrRtm( debugvalue, retailvalue, rtmvalue )       retailvalue
#endif
#endif


#endif

#ifdef DEBUG
INT g_eSpeedOfAcLinePowerLoss = 0;
#endif

LOCAL BOOL g_fRestrictIdleActivity = fFalse;

BOOL FUtilSystemRestrictIdleActivity()
{
    DWORD               dtickCheck      = 500;
    static DWORD        tickLastCheck   = TickOSTimeCurrent() - dtickCheck;
    static BOOL         fLastResult     = fTrue;

    if ( g_fRestrictIdleActivity )
    {
        fLastResult = fTrue;
    }
    else
    {
#ifdef DEBUG
        dtickCheck      = ( ( rand() % 2 ) ?
                                ( ( rand() % 2 ) ? ( rand() % 2 )  : ( ( 1 + rand() % 20 ) * 10 ) ) :
                                ( ( rand() % 2 ) ? dtickCheck : ( ( 1 + rand() % 90 ) * 1000 ) )
                            );
#endif
        dtickCheck = (DWORD)UlConfigOverrideInjection( 44080, dtickCheck );

        if ( TickOSTimeCurrent() - tickLastCheck >= dtickCheck )
        {
#ifndef MINIMAL_FUNCTIONALITY
            SYSTEM_POWER_STATUS sps;

            fLastResult     = !GetSystemPowerStatus( &sps ) || sps.ACLineStatus != 1;
            tickLastCheck   = TickOSTimeCurrent();
#endif
        }
    }

#ifdef DEBUG
    if ( 0 == g_eSpeedOfAcLinePowerLoss )
    {
        g_eSpeedOfAcLinePowerLoss = 1 + rand() % 3;
    }
    fLastResult = ( ( g_eSpeedOfAcLinePowerLoss == 1 ) ? fLastResult :
                    ( ( g_eSpeedOfAcLinePowerLoss == 2 ) ? ( rand() % 999 ) == 0 : rand() % 2 )
                   );
#endif

    fLastResult = (BOOL)UlConfigOverrideInjection( 56368, fLastResult );

    return fLastResult;
}



VOID * g_pvImageBaseAddress;

const VOID * PvUtilImageBaseAddress()
{
    return g_pvImageBaseAddress;
}


const WCHAR* WszUtilImageName()
{
    return g_sysinfobuffers.WszImageName();
}


const WCHAR* WszUtilImagePath()
{
    return g_sysinfobuffers.WszImagePath();
}


const WCHAR* WszUtilImageVersionName()
{
    return WSZVERSIONNAME;
}


LOCAL DWORD g_dwImageVersionMajor;

DWORD DwUtilImageVersionMajor()
{
    return g_dwImageVersionMajor;
}


LOCAL DWORD g_dwImageVersionMinor;

DWORD DwUtilImageVersionMinor()
{
    return g_dwImageVersionMinor;
}


LOCAL DWORD g_dwImageBuildNumberMajor;

DWORD DwUtilImageBuildNumberMajor()
{
    return g_dwImageBuildNumberMajor;
}


LOCAL DWORD g_dwImageBuildNumberMinor;

DWORD DwUtilImageBuildNumberMinor()
{
    return g_dwImageBuildNumberMinor;
}


const WCHAR* WszUtilImageBuildClass()
{
    return g_sysinfobuffers.WszUtilImageBuildClass();
}



BOOL g_fConsoleCtrlHandler;

BOOL WINAPI UtilSysinfoICtrlHandler( DWORD dwCtrlType )
{
    switch ( dwCtrlType )
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            OSIProcessAbort();
            break;

        default:
            break;
    }

    return FALSE;
}



LOCAL BOOL fSSEInstructionsAvailable;
LOCAL BOOL fSSE2InstructionsAvailable;
LOCAL BOOL g_fPopcntAvailable;
LOCAL BOOL g_fAVXEnabled;

BOOL FSSEInstructionsAvailable()
{
    return fSSEInstructionsAvailable;
}

BOOL FSSE2InstructionsAvailable()
{
    return fSSE2InstructionsAvailable;
}

BOOL FPopcntAvailable()
{
    return g_fPopcntAvailable;
}

BOOL FAVXEnabled()
{
    return g_fAVXEnabled;
}

BOOL FDeterminePopcntCapabilities()
{
#if ( defined _AMD64_ || defined _X86_ )
        INT cpuidInfo[4];
        __cpuid(cpuidInfo, 1);
        return !!( cpuidInfo[2] & (1 << 23) );

#else
        return false;
#endif
}

BOOL FDetermineAVXCapabilities()
{
#if ( defined _AMD64_ || defined _X86_ )
        INT cpuidInfo[4];
        __cpuid(cpuidInfo, 1);
        bool fAvxEnabled = false;

        bool fOSXSAVE_Enabled = !!( cpuidInfo[2] & (1 << 27) );
        bool fAVXSupported = !!( cpuidInfo[2] & (1 << 28) );
        if ( fOSXSAVE_Enabled && fAVXSupported )
        {
            unsigned __int64 xcrFeatureMask = _xgetbv( _XCR_XFEATURE_ENABLED_MASK );
            fAvxEnabled = ( ( xcrFeatureMask & 0x06 ) == 0x06 );
        }

        return fAvxEnabled;

#else
        return false;
#endif
}

LOCAL VOID DetermineProcessorCapabilities()
{
    fSSEInstructionsAvailable   = IsProcessorFeaturePresent( PF_XMMI_INSTRUCTIONS_AVAILABLE );
    fSSE2InstructionsAvailable  = IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE );
    g_fPopcntAvailable            = FDeterminePopcntCapabilities();
    g_fAVXEnabled                 = FDetermineAVXCapabilities();
}


void OSSysinfoPostterm()
{

    if ( g_fConsoleCtrlHandler )
    {
        (void)SetConsoleCtrlHandler( UtilSysinfoICtrlHandler, FALSE );
        g_fConsoleCtrlHandler = fFalse;
    }


    g_sysinfobuffers.FreeSysinfoBufferLifetimeManager();
}


#ifndef RTM


LOCAL BOOL g_fSystemVersionOverridesTried = fFalse;

struct OSOVERRIDE
{
    const WCHAR * wszRegistry;
    DWORD * pdw;
};

LOCAL OSOVERRIDE g_rgoverrides[] =
{
        { L"MajorVersion", &g_dwSystemVersionMajor },
        { L"MinorVersion", &g_dwSystemVersionMinor },
        { L"BuildNumber", &g_dwSystemBuildNumber },
        { L"ServicePack", &g_dwSystemServicePackNumber },
};

VOID GetSystemVersionOverridesFromRegistry()
{
    if( g_fSystemVersionOverridesTried )
    {
        return;
    }

    BOOL fChangedVersion = fFalse;
    WCHAR wsz[10];

    INT i;
    for( i = 0; i < ( sizeof( g_rgoverrides ) / sizeof( g_rgoverrides[0] )); ++i )
    {
        if ( FOSConfigGet( L"OS", g_rgoverrides[i].wszRegistry, wsz, sizeof( wsz ) ) && wsz[0] )
        {
            *(g_rgoverrides[i].pdw)   = _wtoi( wsz );
            fChangedVersion         = fTrue;
        }
    }

    if( fChangedVersion )
    {
        WCHAR           wszMessage[256];
        const WCHAR *   rgszT[1]    = { wszMessage };

        OSStrCbFormatW(
                wszMessage, sizeof( wszMessage ),
                L"OS version changed through the registry to %d.%d.%d (service pack %d)",
                g_dwSystemVersionMajor,
                g_dwSystemVersionMinor,
                g_dwSystemBuildNumber,
                g_dwSystemServicePackNumber );
        UtilReportEvent(
                eventWarning,
                GENERAL_CATEGORY,
                PLAIN_TEXT_ID,
                1,
                rgszT );
    }

    g_fSystemVersionOverridesTried = fTrue;
}

#endif



BOOL FGetSystemVersion()
{


    OSVERSIONINFOW ovi;
    ovi.dwOSVersionInfoSize = sizeof( ovi );
    if ( !GetVersionExW( &ovi ) )
    {
        goto HandleError;
    }

    g_dwSystemVersionMajor = ovi.dwMajorVersion;
    g_dwSystemVersionMinor = ovi.dwMinorVersion;
    g_dwSystemBuildNumber = ovi.dwBuildNumber;
    WCHAR* szN;
    szN = wcspbrk( ovi.szCSDVersion, L"0123456789" );
    g_dwSystemServicePackNumber = szN ? _wtol( szN ) : 0;

    return fTrue;

HandleError:
    return fFalse;
}



BOOL FOSSysinfoPreinit()
{
    WCHAR wszWorkingBuffer[ 600 ];

    ERR err = g_sysinfobuffers.ErrAllocateIfNeeded();
    if ( err < JET_errSuccess )
    {
        goto HandleError;
    }

    WCHAR wszProcessFileExt[_MAX_FNAME];
    GetModuleFileNameW( NULL, wszWorkingBuffer, _countof( wszWorkingBuffer ) );
    Call( g_sysinfobuffers.ErrSetProcessPath( wszWorkingBuffer ) );

    _wsplitpath_s( WszUtilProcessPath(), NULL, 0, NULL, 0, wszWorkingBuffer, _countof( wszWorkingBuffer ), wszProcessFileExt, _countof( wszProcessFileExt ) );
    Call( g_sysinfobuffers.ErrSetProcessName( wszWorkingBuffer ) );

    OSStrCbFormatW( wszWorkingBuffer, sizeof( wszWorkingBuffer ), L"%ws%ws", WszUtilProcessName(), wszProcessFileExt );
    Call( g_sysinfobuffers.ErrSetProcessFileName( wszWorkingBuffer ) );

    Call( g_sysinfobuffers.ErrSetProcessFriendlyName( WszUtilProcessName() ) );


    g_dwProcessId = GetCurrentProcessId();


    if( !FGetSystemVersion() )
    {
        goto HandleError;
    }


    MEMORY_BASIC_INFORMATION mbi;
    if ( !VirtualQueryEx( GetCurrentProcess(), FOSSysinfoPreinit, &mbi, sizeof( mbi ) ) )
    {
        goto HandleError;
    }
    if ( !GetModuleFileNameW( HINSTANCE( mbi.AllocationBase ), wszWorkingBuffer, _countof( wszWorkingBuffer ) ) )
    {
        goto HandleError;
    }
    Call( g_sysinfobuffers.ErrSetImagePath( wszWorkingBuffer ) );

    _wsplitpath_s( WszUtilImagePath(), NULL, 0, NULL, 0, wszWorkingBuffer, _countof( wszWorkingBuffer ), NULL, 0 );
    Call( g_sysinfobuffers.ErrSetImageName( wszWorkingBuffer ) );

    g_pvImageBaseAddress = mbi.AllocationBase;

#ifdef ESENT
    g_dwImageVersionMajor = g_dwSystemVersionMajor;
    g_dwImageVersionMinor = g_dwSystemVersionMinor;
    g_dwImageBuildNumberMajor = g_dwSystemBuildNumber;
    g_dwImageBuildNumberMinor = 0;
#else
    g_dwImageVersionMajor = atoi( PRODUCT_MAJOR );
    g_dwImageVersionMinor = atoi( PRODUCT_MINOR );
    g_dwImageBuildNumberMajor = atoi( BUILD_MAJOR );
    g_dwImageBuildNumberMinor = atoi( BUILD_MINOR );
#endif

    wszWorkingBuffer[ 0 ] = L'\0';
#ifdef ESENT
    OSStrCbFormatW( wszWorkingBuffer, sizeof(wszWorkingBuffer),
                        L"%ws[%I32u.%I32u.%I32u.%I32u]",
                        WSZVERSIONNAME,
                        DwUtilImageVersionMajor(), DwUtilImageVersionMinor(), DwUtilImageBuildNumberMajor(), DwUtilImageBuildNumberMinor()
                        );
#else
    OSStrCbFormatW( wszWorkingBuffer, sizeof(wszWorkingBuffer),
                        L"%ws[%02I32u.%02I32u.%04I32u.%03I32u]",
                        WSZVERSIONNAME,
                        DwUtilImageVersionMajor(), DwUtilImageVersionMinor(), DwUtilImageBuildNumberMajor(), DwUtilImageBuildNumberMinor()
                        );
#endif
#ifdef DEBUG
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" DEBUG" );
#else
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" RETAIL" );
#endif
#ifdef RTM
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" RTM" );
#endif
#ifdef _UNICODE
    OSStrCbAppendA( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" UNICODE" );
#else
#ifdef _MBCS
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" MBCS" );
#else
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" ASCII" );
#endif
#endif

    Call( g_sysinfobuffers.ErrSetUtilImageBuildClass( wszWorkingBuffer ) );


    DetermineProcessorCapabilities();

    g_fConsoleCtrlHandler = SetConsoleCtrlHandler( UtilSysinfoICtrlHandler, TRUE );

    return fTrue;

HandleError:
    OSSysinfoPostterm();
    return fFalse;
}

VOID COSLayerPreInit::SetProcessFriendlyName( const WCHAR* const wszProcessFriendlyNameNew )
{
    const WCHAR* wszProcessFriendlyNameT = wszProcessFriendlyNameNew;
    if ( wszProcessFriendlyNameT == NULL || wcslen( wszProcessFriendlyNameT ) == 0 )
    {
        wszProcessFriendlyNameT = WszUtilProcessName();
    }
    (void) g_sysinfobuffers.ErrSetProcessFriendlyName( wszProcessFriendlyNameT );
}

VOID COSLayerPreInit::SetRestrictIdleActivity( const BOOL fRestrictIdleActivity )
{
    g_fRestrictIdleActivity = fRestrictIdleActivity;
}

COSEventTraceIdCheck g_traceidcheckSysGlobal;

void OSSysTraceStationId( const DWORD  tsidr_ )
{
    const TraceStationIdentificationReason tsidr = (TraceStationIdentificationReason)tsidr_;


    if ( !g_traceidcheckSysGlobal.FAnnounceTime< _etguidSysStationId >( tsidr ) )
    {
        return;
    }

    DWORD dwImageVerMajor = DwUtilImageVersionMajor();
    DWORD dwImageVerMinor = DwUtilImageVersionMinor();
    DWORD dwImageBuildMajor = DwUtilImageBuildNumberMajor();
    DWORD dwImageBuildMinor = DwUtilImageBuildNumberMinor();

    ETSysStationId( tsidr, dwImageVerMajor, dwImageVerMinor, dwImageBuildMajor, dwImageBuildMinor, WszUtilProcessFileName() );

}

static const WCHAR c_wszSysSetupKey[] = L"System\\Setup";
static const WCHAR c_wszSysSetupValue[] = L"SystemSetupInProgress";

BOOL FOSSetupRunning()
{
    BOOL                    fSetupRunning       = fFalse;
    HKEY                    hKey                = NULL;
    DWORD                   dwType              = 0;
    DWORD                   dwAnswer            = 0;
    DWORD                   dwSize              = sizeof( dwAnswer );

    NTOSFuncError( pfnRegOpenKeyExW, g_mwszzRegistryLibs, RegOpenKeyExW, oslfExpectedOnWin5x | oslfRequired | oslfStrictFree );
    NTOSFuncError( pfnRegQueryValueExW, g_mwszzRegistryLibs, RegQueryValueExW, oslfExpectedOnWin5x | oslfRequired | oslfStrictFree );
    NTOSFuncError( pfnRegCloseKey, g_mwszzRegistryLibs, RegCloseKey, oslfExpectedOnWin5x | oslfRequired | oslfStrictFree );

    if (    pfnRegOpenKeyExW(   HKEY_LOCAL_MACHINE,
                                c_wszSysSetupKey,
                                0,
                                KEY_READ,
                                &hKey ) != ERROR_SUCCESS ||
            pfnRegQueryValueExW(    hKey,
                                c_wszSysSetupValue,
                                NULL,
                                &dwType,
                                (LPBYTE)&dwAnswer,
                                &dwSize ) != ERROR_SUCCESS ||
            dwType != REG_DWORD ||
            dwSize != sizeof( dwAnswer ) )
    {
        goto HandleError;
    }

    if ( dwAnswer != 0 )
    {
        fSetupRunning = fTrue;
    }
    else
    {
        fSetupRunning = fFalse;
    }

HandleError:
    if ( hKey )
    {
        pfnRegCloseKey( hKey );
    }
    return fSetupRunning;
}

#ifdef ESENT

#include <appmodel.h>
#include <statemanager.h>

#else
WINBASEAPI
LONG
WINAPI
GetCurrentPackageFullName(
    _Inout_ UINT32 * packageFullNameLength,
    _Out_writes_opt_(*packageFullNameLength) PWSTR packageFullName
    );

#ifndef APPMODEL_ERROR_NO_PACKAGE
#define APPMODEL_ERROR_NO_PACKAGE        15700L
#endif

#endif

LOCAL BOOL g_fIsProcessPackaged = fFalse;
LOCAL BOOL g_fIsProcessPackagedCalculated = fFalse;

VOID CalculateCurrentProcessIsPackaged()
{
    if ( !g_fIsProcessPackagedCalculated )
    {
        Assert( !g_fIsProcessPackaged );


        NTOSFuncError( pfnGetCurrentPackageFullName, g_mwszzAppModelRuntimeLibs, GetCurrentPackageFullName, oslfExpectedOnWin8 | oslfStrictFree );

        UINT32 cbBuffer = 0;
        LONG lWin32Error = pfnGetCurrentPackageFullName( &cbBuffer, nullptr );

        if ( ERROR_CALL_NOT_IMPLEMENTED == lWin32Error )
        {
            g_fIsProcessPackagedCalculated = fTrue;
            return;
        }

        AssertSz( APPMODEL_ERROR_NO_PACKAGE == lWin32Error || ERROR_INSUFFICIENT_BUFFER == lWin32Error,
                  "GetCurrentPackageFullName returned an unexpected return code: %d (%#x).", lWin32Error, lWin32Error );

        if ( ERROR_INSUFFICIENT_BUFFER == lWin32Error )
        {
            g_fIsProcessPackaged = fTrue;
        }

        g_fIsProcessPackagedCalculated = fTrue;
    }

    return;
}


const BOOL FUtilProcessIsPackaged()
{
    if ( !g_fIsProcessPackagedCalculated )
    {
        CalculateCurrentProcessIsPackaged();
    }

    Assert( g_fIsProcessPackagedCalculated );
    return g_fIsProcessPackaged;
}

LOCAL BOOL g_fIsProcessWow64 = fFalse;
LOCAL BOOL g_fIsProcessWow64Calculated = fFalse;

LOCAL VOID CalculateCurrentProcessIsWow64()
{
    if ( !g_fIsProcessWow64Calculated )
    {
        Assert( !g_fIsProcessWow64 );


        NTOSFuncError( pfnIsWow64Process, g_mwszzWow64Libs, IsWow64Process, oslfExpectedOnWin6 | oslfStrictFree );

        BOOL fIsWow64 = fFalse;
        if ( pfnIsWow64Process( GetCurrentProcess(), &fIsWow64 ) &&
             fIsWow64 )
        {
            g_fIsProcessWow64 = fTrue;
        }

        g_fIsProcessWow64Calculated = fTrue;
    }

    return;
}


const BOOL FUtilIProcessIsWow64()
{
    if ( !g_fIsProcessWow64Calculated )
    {
        CalculateCurrentProcessIsWow64();
    }

    Assert( g_fIsProcessWow64Calculated );
    return g_fIsProcessWow64;
}


enum UtilSystemBetaSiteMode
{


    usbsmTestEnvExplicit        =     0x01,
    usbsmTestEnvLocalMode       =     0x04,
    usbsmTestEnvAlphaMode       =     0x10,
    usbsmTestEnvBetaMode        =     0x40,
    usbsmTestEnvDebugMode       =     0x80,
    usbsmTestEnvAll             =     0xD5,

    usbsmSelfhostExplicit       =   0x0100,
    usbsmSelfhostLocalMode      =   0x0400,
    usbsmSelfhostAlphaMode      =   0x1000,
    usbsmSelfhostBetaMode       =   0x4000,
    usbsmSelfhostAll            =   0x5500,

    usbsmProdExplicit           = 0x010000,
    usbsmProdLocalMode          = 0x040000,
    usbsmProdAlphaMode          = 0x100000,
    usbsmProdBetaMode           = 0x400000,
    usbsmProdWindows            = 0x800000,
    usbsmProdAll                = 0xD50000,

    usbsmExFeatRiskyFeatTest    = 0x00000002,
    usbsmExFeatNegTest          = 0x00000008,
    usbsmExFeatOtherFeatTest    = 0x00000020,

    usbsmExFeatNext1                                = 0x01000000,
    usbsmExFeatNext2                                = 0x02000000,
    usbsmExFeatNext3                                = 0x04000000,
    usbsmExFeatNext4                                = 0x08000000,
    usbsmExFeatNext5                                = 0x10000000,
    usbsmExFeatNext6                                = 0x20000000,
    usbsmExFeatNext7                                = 0x40000000,
    usbsmExFeatNext8                                = 0x80000000,

    usbsmExFeatLast,
    usbsmExFeatureMask                              = 0xFF00002A
};

DEFINE_ENUM_FLAG_OPERATORS_BASIC( UtilSystemBetaSiteMode );

INT usbsmPrimaryEnvironments    = ( usbsmTestEnvAll | usbsmSelfhostAll | usbsmProdAll );
INT usbsmExFeatures             = usbsmExFeatureMask;
#ifdef DEBUG
INT usbsmExFeatureMin           = usbsmExFeatRiskyFeatTest;
INT usbsmExFeatureMax           = usbsmExFeatLast;
#endif

C_ASSERT( JET_bitStageTestEnvLocalMode == usbsmTestEnvLocalMode );
C_ASSERT( JET_bitStageTestEnvAlphaMode == usbsmTestEnvAlphaMode );
C_ASSERT( JET_bitStageTestEnvBetaMode == usbsmTestEnvBetaMode );

C_ASSERT( JET_bitStageSelfhostLocalMode == usbsmSelfhostLocalMode );
C_ASSERT( JET_bitStageSelfhostAlphaMode == usbsmSelfhostAlphaMode );
C_ASSERT( JET_bitStageSelfhostBetaMode == usbsmSelfhostBetaMode );

C_ASSERT( JET_bitStageProdLocalMode == usbsmProdLocalMode );
C_ASSERT( JET_bitStageProdAlphaMode == usbsmProdAlphaMode );
C_ASSERT( JET_bitStageProdBetaMode == usbsmProdBetaMode );

const LONG fLoggedEventAlready = -1;
C_ASSERT( fLoggedEventAlready );

UtilSystemBetaConfig    g_rgbetaconfigs [] =
{

    { fTrue,  fFeatureDynamic, EseTestFeatures,                     usbsmTestEnvAll },


#ifndef OS_LAYER_VIOLATIONS
    { fTrue,  fFeatureDynamic, EseTestCase,                         usbsmTestEnvAll OnDebug( | usbsmSelfhostAlphaMode ) OnNonRTM( | usbsmSelfhostBetaMode ) | usbsmExFeatOtherFeatTest },
    { fFalse, fFeatureStatic,  EseTestCaseTwo,                      usbsmTestEnvAll   },
    { fFalse, fFeatureDynamic, EseTestCaseThree,                    usbsmProdAll  },
    { fFalse, fFeatureDynamic, EseRiskyFeatTest,                    usbsmSelfhostAlphaMode | usbsmExFeatRiskyFeatTest },
#endif
};
C_ASSERT( _countof(g_rgbetaconfigs) == EseFeatureMax );

void UtilReportBetaFeatureInUse( const INST * const pinst, const UtilSystemBetaSiteMode usbsmCurrent, const ULONG featureid, PCWSTR const wszFeatureName )
{
    if ( fFalse == AtomicCompareExchange( &( g_rgbetaconfigs[featureid].fSuppressInfoEvent ), (LONG)fFalse, (LONG)fLoggedEventAlready ) )
    {
#ifdef ENABLE_MICROSOFT_MANAGED_DATACENTER_LEVEL_OPTICS
        WCHAR wszBetaSiteMode[21];
        OSStrCbFormatW( wszBetaSiteMode, sizeof(wszBetaSiteMode), L"%#x", usbsmCurrent );
        const WCHAR * rgwszT[] = { wszFeatureName, WszUtilImageVersionName(), wszBetaSiteMode };
        UtilReportEvent( eventInformation, GENERAL_CATEGORY, STAGED_BETA_FEATURE_IN_USE_ID, _countof(rgwszT), rgwszT );
#endif
        OSTrace( JET_tracetagVersionAndStagingChecks, OSFormat( "Beta Feature %d staging value %d (usbsmCurrent = %d).\n", featureid, fTrue, usbsmCurrent ) );
    }
}



ERR ErrUtilSystemSlConfiguration(
    _In_z_ const WCHAR *    pszValueName,
    _Out_   DWORD *         pdwValue )
{
    Assert( pdwValue );
    ERR err = JET_errSuccess;

    ExpectedSz( 0 == wcscmp( pszValueName, L"ExtensibleStorageEngine-ISAM-ParamConfiguration" ),
                "You are passing an unknown value (%ws) name into here, the single 55564 config override below may mess things up.", pszValueName );

#if defined(ESENT) && defined(OS_LAYER_VIOLATIONS)
    const DWORD error = SLGetWindowsInformationDWORD( pszValueName, pdwValue );
    switch( error )
    {
        case S_OK:
            err = JET_errSuccess;
            break;
        case SL_E_DATATYPE_MISMATCHED:
            AssertSz( fFalse, "That's odd, it's our licensing policy, it really SHOULD match our expected data type." );
        case SL_E_RIGHT_NOT_GRANTED:
        default:
            err = ErrERRCheck( JET_errUnloadableOSFunctionality );
    }
#else
    err = ErrERRCheck( JET_errDisabledFunctionality );
#endif

#ifdef DEBUG
    err = (ERR)UlConfigOverrideInjection( 43276, (ULONG_PTR)err );
    if ( err )
    {
        ErrERRCheck( err );
    }
#endif

    if ( err >= JET_errSuccess  )
    {
        *pdwValue = (DWORD)UlConfigOverrideInjection( 55564, *pdwValue );
    }

    return err;
}

#define ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

#ifdef ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

#include "NTSecAPI.h"


#ifndef _DEFINED__WNF_STATE_NAME
#define _DEFINED__WNF_STATE_NAME
typedef struct _WNF_STATE_NAME {
    ULONG Data[2];
} WNF_STATE_NAME;



typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;

   typedef struct _WNF_TYPE_ID
    {
        GUID TypeId;
    } WNF_TYPE_ID;

    typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;
    typedef struct _WNF_TYPE_ID* PWNF_TYPE_ID;
    typedef const struct _WNF_TYPE_ID* PCWNF_TYPE_ID;



EXTERN_C __declspec(selectany) const WNF_STATE_NAME
    WNF_DEP_UNINSTALL_DISABLED = {0xa3bc1475, 0x41960b29};


typedef struct _WNF_STATE_NAME* PWNF_STATE_NAME;
typedef const struct _WNF_STATE_NAME* PCWNF_STATE_NAME;

#define STATUS_RETRY                            ((NTSTATUS)0xC000022DL)
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L)

_Always_(_Post_satisfies_(return == STATUS_NO_MEMORY || return == STATUS_RETRY || return == STATUS_SUCCESS))
_Function_class_(WNF_USER_CALLBACK)
typedef
NTSTATUS
WNF_USER_CALLBACK(
    _In_ WNF_STATE_NAME StateName,
    _In_ WNF_CHANGE_STAMP ChangeStamp,
    _In_opt_ PWNF_TYPE_ID TypeId,
    _In_opt_ PVOID CallbackContext,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_ ULONG Length
    );
typedef WNF_USER_CALLBACK *PWNF_USER_CALLBACK;

NTSTATUS
RtlQueryWnfStateData(
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _In_ WNF_STATE_NAME StateName,
    _In_ PWNF_USER_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PWNF_TYPE_ID TypeId
    );


#endif
static NTSTATUS WnfQueryDWordDataCallback(
    _In_ WNF_STATE_NAME         oswnfsn,
    _In_ WNF_CHANGE_STAMP       oswnfcs,
    _In_opt_ PWNF_TYPE_ID       oswnftid,
    _In_ PVOID                  pfExpired,
    _In_reads_bytes_opt_(cbBuffer) const VOID* pvBuffer,
    _In_ ULONG                 cbBuffer )
{
    Assert( ( cbBuffer == 0 ) || ( cbBuffer == sizeof(DWORD) ) );
    Assert( pfExpired != NULL );

    UNREFERENCED_PARAMETER( oswnfsn );
    UNREFERENCED_PARAMETER( oswnfcs );
    UNREFERENCED_PARAMETER( oswnftid );

    DWORD dwData = 0;
    if ( cbBuffer == sizeof(DWORD) )
    {
        Assert( pvBuffer != NULL );
        RtlCopyMemory( &dwData, pvBuffer, sizeof(dwData) );
    }

    *(BOOL*)pfExpired = !!dwData;

    return 0;
}

#endif

ERR ErrUtilOsDowngradeWindowExpired( _Out_ BOOL * pfExpired )
{
    NTOSFuncError( pfnRtlQueryWnfStateData, g_mwszzNtdllLibs, RtlQueryWnfStateData, oslfExpectedOnWin10 );

    const ERR errFaultInj = (ERR)UlConfigOverrideInjection( 50026, 3 );
    if ( errFaultInj < JET_errSuccess )
    {
        return ErrERRCheck( errFaultInj );
    }

#ifdef ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

    WNF_CHANGE_STAMP oswnfcs;
    if ( FAILED( pfnRtlQueryWnfStateData(
            &oswnfcs,
            WNF_DEP_UNINSTALL_DISABLED,
            WnfQueryDWordDataCallback,
            (PVOID)pfExpired,
            nullptr ) ) )
    {
#ifndef ESENT
        *pfExpired = fTrue;
        return JET_errSuccess;
#else
        return ErrERRCheck( JET_errTaskDropped );
#endif
    }

#else
    Assert( errFaultInj != 4 );
#endif

    if ( errFaultInj != 4 )
    {
        *pfExpired = (BOOL)UlConfigOverrideInjection( 50026, *pfExpired );
    }

    if ( errFaultInj == 3 )
    {
#ifndef ESENT
        *pfExpired = fTrue;
#endif
    }

    return JET_errSuccess;
}


void OSSysinfoTerm()
{
    OSSysTraceStationId( tsidrCloseTerm );


    for( ULONG featureid = 0; featureid < _countof(g_rgbetaconfigs); featureid++ )
    {
        Assert( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureDynamic || g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic );
        if ( g_rgbetaconfigs[featureid].fSuppressInfoEvent == fLoggedEventAlready )
        {
            (void)AtomicExchange( &( g_rgbetaconfigs[featureid].fSuppressInfoEvent ), fFalse );
        }
    }
}


ERR ErrOSSysinfoInit()
{
    OSSysTraceStationId( tsidrOpenInit );

    return JET_errSuccess;
}
