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

//  Responsible for allocating and cleaning up of the useful string buffers.
//  The destructor is called during DLL detach.
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

        //  Note that this is called during Preinit, so we don't have the usual memory
        // allocators available.

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

    //  Access to the various strings that this object controls.

    // ErrSetProcessFriendlyName() may get called after DLL Init, so we'll treat
    // this one special and NOT base it off of the input string, but instead
    // stick with the same buffer size.
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
        return ErrStrDupLocalAlloc( &m_wszProcessName, wszNewValue, LOSStrLengthW( wszNewValue ) );
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
        return ErrStrDupLocalAlloc( &m_wszProcessFileName, wszNewValue, LOSStrLengthW( wszNewValue ) );
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
        return ErrStrDupLocalAlloc( &m_wszProcessPath, wszNewValue, LOSStrLengthW( wszNewValue ) );
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
        return ErrStrDupLocalAlloc( &m_wszImageName, wszNewValue, LOSStrLengthW( wszNewValue ) );
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
        return ErrStrDupLocalAlloc( &m_wszImagePath, wszNewValue, LOSStrLengthW( wszNewValue ) );
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
        return ErrStrDupLocalAlloc( &m_wszImageBuildClass, wszNewValue, LOSStrLengthW( wszNewValue ) );
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

            Assert( LOSStrLengthW( *pwszDestination ) == LOSStrLengthW( wszNewValue ) );
        }

HandleError:
        return err;
    }

} g_sysinfobuffers;


//  Process Attributes

//  returns the current process' name

const WCHAR* WszUtilProcessName()
{
    return g_sysinfobuffers.WszProcessName();
}

//  returns the current process' file name

const WCHAR* WszUtilProcessFileName()
{
    return g_sysinfobuffers.WszProcessFileName();
}

//  returns the current process' friendly name

const WCHAR* WszUtilProcessFriendlyName()
{
    return g_sysinfobuffers.WszProcessFriendlyName();
}

//  returns the current process' path

const WCHAR* WszUtilProcessPath()
{
    return g_sysinfobuffers.WszProcessPath();
}

//  returns the current process ID

LOCAL DWORD g_dwProcessId;

const DWORD DwUtilProcessId()
{
    return g_dwProcessId;
}

//  returns the number of system processors on which the current process can execute

LOCAL DWORD cProcessProcessor;

const DWORD CUtilProcessProcessor()
{
    //  no one uses this correctly right now so set to # system processors
    return OSSyncGetProcessorCountMax();
}

//  returns fTrue if the current process is terminating abnormally

volatile BOOL g_fProcessAbort = fFalse;

const BOOL FUtilProcessAbort()
{
    return g_fProcessAbort;
}


//  System Attributes

//  retrieves system major version

LOCAL DWORD g_dwSystemVersionMajor;

DWORD DwUtilSystemVersionMajor()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemVersionMajor;
}

//  retrieves system minor version

LOCAL DWORD g_dwSystemVersionMinor;

DWORD DwUtilSystemVersionMinor()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemVersionMinor;
}

//  retrieves system major build number

LOCAL DWORD g_dwSystemBuildNumber;

DWORD DwUtilSystemBuildNumber()
{
    GetSystemVersionOverridesFromRegistry();
    return g_dwSystemBuildNumber;
}

//  retrieves system service pack number

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
        //  in debug we test various times: 0 ms (i.e. every run / 12.5%), 1 ms (12.5%), 10 - 200 ms (25%),
        //  the default .5 seconds (25%), and 1 - 90 seconds (25%) to force nasty bugs
        dtickCheck      = ( ( rand() % 2 ) ?
                                ( ( rand() % 2 ) ? /*25%*/( rand() % 2 )  : /*25%*/( ( 1 + rand() % 20 ) * 10 ) ) :
                                ( ( rand() % 2 ) ? /*25%*/dtickCheck : /*25%*/( ( 1 + rand() % 90 ) * 1000 ) )
                            );
#endif
        dtickCheck = (DWORD)UlConfigOverrideInjection( 44080, dtickCheck );

        if ( TickOSTimeCurrent() - tickLastCheck >= dtickCheck )
        {
#ifndef MINIMAL_FUNCTIONALITY
            SYSTEM_POWER_STATUS sps;

            fLastResult     = !GetSystemPowerStatus( &sps ) || sps.ACLineStatus != 1;
            tickLastCheck   = TickOSTimeCurrent();
#endif // !MINIMAL_FUNCTIONALITY
        }
    }

#ifdef DEBUG
    if ( 0 == g_eSpeedOfAcLinePowerLoss )
    {
        g_eSpeedOfAcLinePowerLoss = 1 + rand() % 3;
    }
    fLastResult = ( ( g_eSpeedOfAcLinePowerLoss == 1 ) ? /*33%*/fLastResult :
                    ( ( g_eSpeedOfAcLinePowerLoss == 2 ) ? /*33%*/( rand() % 999 ) == 0 : /*33%*/rand() % 2 )
                   );
#endif // DEBUG

    fLastResult = (BOOL)UlConfigOverrideInjection( 56368, fLastResult );

    return fLastResult;
}

//  Image Attributes

//  retrieves image base address

VOID * g_pvImageBaseAddress;

const VOID * PvUtilImageBaseAddress()
{
    return g_pvImageBaseAddress;
}

//  retrieves image name

const WCHAR* WszUtilImageName()
{
    return g_sysinfobuffers.WszImageName();
}

//  retrieves image path

const WCHAR* WszUtilImagePath()
{
    return g_sysinfobuffers.WszImagePath();
}

//  retrieves image version name

const WCHAR* WszUtilImageVersionName()
{
    return WSZVERSIONNAME;
}

//  retrieves image major version

LOCAL DWORD g_dwImageVersionMajor;

DWORD DwUtilImageVersionMajor()
{
    return g_dwImageVersionMajor;
}

//  retrieves image minor version

LOCAL DWORD g_dwImageVersionMinor;

DWORD DwUtilImageVersionMinor()
{
    return g_dwImageVersionMinor;
}

//  retrieves image major build number

LOCAL DWORD g_dwImageBuildNumberMajor;

DWORD DwUtilImageBuildNumberMajor()
{
    return g_dwImageBuildNumberMajor;
}

//  retrieves image minor build number

LOCAL DWORD g_dwImageBuildNumberMinor;

DWORD DwUtilImageBuildNumberMinor()
{
    return g_dwImageBuildNumberMinor;
}

//  retrieves image build class

const WCHAR* WszUtilImageBuildClass()
{
    return g_sysinfobuffers.WszUtilImageBuildClass();
}


//  ^C and ^Break handler for process termination

BOOL g_fConsoleCtrlHandler;

BOOL WINAPI UtilSysinfoICtrlHandler( DWORD dwCtrlType )
{
    switch ( dwCtrlType )
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            //  set process to aborting status on ^C and ^Break
            OSIProcessAbort();
            break;

        default:
            break;
    }

    return FALSE;
}


//  do all processors in the system have the PrefetchNTA capability?

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
#if ( defined _M_AMD64 || defined _M_IX86 )
        INT cpuidInfo[4];
        __cpuid(cpuidInfo, 1);
        return !!( cpuidInfo[2] & (1 << 23) );  // check bit 23 of CX

#else
        return false;
#endif
}

BOOL FDetermineAVXCapabilities()
{
#if ( defined _M_AMD64 || defined _M_IX86 )
        INT cpuidInfo[4];
        __cpuid(cpuidInfo, 1);
        bool fAvxEnabled = false;

        bool fOSXSAVE_Enabled = !!( cpuidInfo[2] & (1 << 27) ); // Is XSAVE/XRSTORE used to store register state on context switches?
        bool fAVXSupported = !!( cpuidInfo[2] & (1 << 28) );        // Does the processor support AVX?
        if ( fOSXSAVE_Enabled && fAVXSupported )
        {
            unsigned __int64 xcrFeatureMask = _xgetbv( _XCR_XFEATURE_ENABLED_MASK );    // Did the OS enable saving AVX+SSE state? (indicated by XCR0 - eXtended Control Register 0)?
            fAvxEnabled = ( ( xcrFeatureMask & 0x06 ) == 0x06 );    // XCR0 -   bit0 = FPU state (always 1)
                                                                    //          bit1 = SSE state
                                                                    //          bit2 = AVX state
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

//  post-terminate sysinfo subsystem

void OSSysinfoPostterm()
{
    //  remove our console ctrl handler if set

    if ( g_fConsoleCtrlHandler )
    {
        (void)SetConsoleCtrlHandler( UtilSysinfoICtrlHandler, FALSE );
        g_fConsoleCtrlHandler = fFalse;
    }

    //  Free the global memory.

    g_sysinfobuffers.FreeSysinfoBufferLifetimeManager();
}


#ifndef RTM

//  provide a registry override so we can fake the version of the OS we are running on

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


//  determine which version of windows we are on

BOOL FGetSystemVersion()
{

    //  cache system attributes

    OSVERSIONINFOW ovi;
    ovi.dwOSVersionInfoSize = sizeof( ovi );
    if ( !GetVersionExW( &ovi ) )
    {
        goto HandleError;
    }

    // As of Windows 8.1, this is no longer reliable -- it depends on how the
    // executable was manifested.
    g_dwSystemVersionMajor = ovi.dwMajorVersion;
    g_dwSystemVersionMinor = ovi.dwMinorVersion;
    g_dwSystemBuildNumber = ovi.dwBuildNumber;
    //  string of format "Service Pack %d"
    WCHAR* szN;
    szN = wcspbrk( ovi.szCSDVersion, L"0123456789" );
    g_dwSystemServicePackNumber = szN ? _wtol( szN ) : 0;

    return fTrue;

HandleError:
    return fFalse;
}


//  pre-init sysinfo subsystem

BOOL FOSSysinfoPreinit()
{
    //  cache process name and path
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

    //  cache process id

    g_dwProcessId = GetCurrentProcessId();

    //  cache the version of the OS we are running on

    if( !FGetSystemVersion() )
    {
        goto HandleError;
    }

    //  cache image name and path

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
#else  //  !DEBUG
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" RETAIL" );
#endif  //  DEBUG
#ifdef RTM
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" RTM" );
#endif  //  RTM
#ifdef _UNICODE
    OSStrCbAppendA( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" UNICODE" );
#else  //  !_UNICODE
#ifdef _MBCS
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" MBCS" );
#else  //  !_MBCS
    OSStrCbAppendW( wszWorkingBuffer, sizeof(wszWorkingBuffer), L" ASCII" );
#endif  //  _MBCS
#endif  //  _UNICODE

    Call( g_sysinfobuffers.ErrSetUtilImageBuildClass( wszWorkingBuffer ) );

    //  determine processor prefetch capability

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
    if ( wszProcessFriendlyNameT == NULL || LOSStrLengthW( wszProcessFriendlyNameT ) == 0 )
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

void OSSysTraceStationId( const DWORD /* TraceStationIdentificationReason */ tsidr_ )
{
    const TraceStationIdentificationReason tsidr = (TraceStationIdentificationReason)tsidr_;

    //  Called (with tsidrPulseInfo) for every DB cache read/write.

    if ( !g_traceidcheckSysGlobal.FAnnounceTime< _etguidSysStationId >( tsidr ) )
    {
        return;
    }

    DWORD dwImageVerMajor = DwUtilImageVersionMajor();
    DWORD dwImageVerMinor = DwUtilImageVersionMinor();
    DWORD dwImageBuildMajor = DwUtilImageBuildNumberMajor();
    DWORD dwImageBuildMinor = DwUtilImageBuildNumberMinor();

    //  Note: We don't log the PID, because it is automatically logged by ETW.
    ETSysStationId( tsidr, dwImageVerMajor, dwImageVerMinor, dwImageBuildMajor, dwImageBuildMinor, WszUtilProcessFileName() );

    //  maybe computer name?  surely some of this must be in trace info itself.
}

//  FOSSetupRunning is used to determine if ESE is being used during setup.
//
static const WCHAR c_wszSysSetupKey[] = L"System\\Setup";
static const WCHAR c_wszSysSetupValue[] = L"SystemSetupInProgress";

BOOL FOSSetupRunning()
{
    BOOL                    fSetupRunning       = fFalse;  //  assume it is not
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

// Defined in $(BASE_INC_PATH)
#include <appmodel.h>
#include <statemanager.h>

#else
WINBASEAPI
// 2012/03/23 Exchange does not yet have SALv2:
// _Success_(return == ERROR_SUCCESS)
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

        // If the function is not available, then the process can't be packaged.

        NTOSFuncError( pfnGetCurrentPackageFullName, g_mwszzAppModelRuntimeLibs, GetCurrentPackageFullName, oslfExpectedOnWin8 | oslfStrictFree );

        UINT32 cbBuffer = 0;
        LONG lWin32Error = pfnGetCurrentPackageFullName( &cbBuffer, nullptr );

        if ( ERROR_CALL_NOT_IMPLEMENTED == lWin32Error )
        {
            // The function isn't available (e.g. pre-Win8 machine).
            g_fIsProcessPackagedCalculated = fTrue;
            return;
        }

        // Is it possible that this Assert might break down in low-resource conditions?
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

//  returns whether the current process is Packaged (Win8 only).

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

        // If the function is not available, then the process can't be Wow64.

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

//  returns whether the current process is Wow64 (Win8 only).

const BOOL FUtilIProcessIsWow64()
{
    if ( !g_fIsProcessWow64Calculated )
    {
        CalculateCurrentProcessIsWow64();
    }

    Assert( g_fIsProcessWow64Calculated );
    return g_fIsProcessWow64;
}

//  Private Staging (for alpha, beta, and test in production staging) of features.
//

enum UtilSystemBetaSiteMode //  usbsm
{
    //  You can _NOT_ change these constants b/c some of them are extruded out the JET API, and so
    //  I have picked values that leave room in the middle for expansion - though technically the
    //  order doesn't matter (other than making the *All constants easy).  The constants that are
    //  exposed out the JET API are marked with [*].

    //  the *Explicit and *All constants are _never_ supposed to propogated to jethdr.w ...
    //  The *All aren't propagated because they are supposed to be sums of environments for use
    //  in the g_rgbetaconfigs table, not to be passed in for the param.
    //  The *Explicit are to be used to manually use a numeric value to hack the registry to turn
    //  on a feature explicitly.

    //  Test Environments - _NO_ real data at risk
    //
    usbsmTestEnvExplicit        =     0x01,     //  requires explicit enablement
    //usbsmStolenDoNotUse       =     0x02,     //  - stolen for usbsmExFeat* space
    usbsmTestEnvLocalMode       =     0x04,     //  Set for accept on dev boxes (but only consumed by few ESE tests so far).
    //usbsmStolenDoNotUse       =     0x08,     //  - stolen for usbsmExFeat* space
    usbsmTestEnvAlphaMode       =     0x10,     //  ESE test pass (but not tests running via "store.exe") in focus/dailies (but only consumed by few ESE tests so far).
    //usbsmStolenDoNotUse       =     0x20,     //  - stolen for usbsmExFeat* space
    usbsmTestEnvBetaMode        =     0x40,     //  [*]Exchange Test Pass - Even in store (extest.microsoft.com) - includes perf tests.
    usbsmTestEnvDebugMode       =     0x80,     //  Used to turn on debug code if no staging mode is set (such as for eseutil and WSTF tests currently).
    usbsmTestEnvAll             =     0xD5,     //  Encompasses all previous usbsmTestEnv* bits / Testing environments.

    //  Dogfood / Selfhost - Microsoft data at risk
    //
    usbsmSelfhostExplicit       =   0x0100,     //  requires explicit enablement
    usbsmSelfhostLocalMode      =   0x0400,     //  [*]Primary/Pioneer/DFMain selfhost (exchange.corp.microsoft.com)
    usbsmSelfhostAlphaMode      =   0x1000,     //  [*]Service Dogfood / SDF & SDFv2
    usbsmSelfhostBetaMode       =   0x4000,     //  [*]Reserved
    usbsmSelfhostAll            =   0x5500,     //  Encompasses all previous usbsmSelfhost* bits / selfhost environments.

    //  Service(s) Production Staging - Customer data at risk
    //
    usbsmProdExplicit           = 0x010000,     //
    usbsmProdLocalMode          = 0x040000,     //  [*]NYI - Reserved
    usbsmProdAlphaMode          = 0x100000,     //  [*]Datacenter SIP (NAMPRD13 forest)
    usbsmProdBetaMode           = 0x400000,     //  [*]Datacenter Main (Microsoft owned)
    usbsmProdWindows            = 0x800000,     //  [*]Windows
    usbsmProdAll                = 0xD50000,     //  Encompasses all previous usbsmProd* bits / production staging domains.

    //  Explicit or Experimental Features (those too risky to accidentally turn on, or requiring explicit validation phase)
    //
    //  NOTE: You will only be able to test _one_ of these features at a time.
    //
    //  IMPORTANT: BEFORE REMOVING FROM THIS TABLE, ENSURE YOU'VE REMOVED ANY PROD OVERRIDES FOR YOUR FEATURE FROM
    //  THE PRODUCTION ENVIRONMENT FOR AT LEAST A FEW WEEKS.  Also recommend you just comment it out for a few weeks
    //  as well to ensure it isn't reused by anyone.
    usbsmExFeatRiskyFeatTest    = 0x00000002,   //  Basic unit test feature
    usbsmExFeatNegTest          = 0x00000008,   //  Basic unit test feature
    usbsmExFeatOtherFeatTest    = 0x00000020,   //  Basic unit test feature

    usbsmExFeatNext1                                = 0x01000000,  // was ...EseOld2DoesSecondaryIndices - removed 2020/08/04
    usbsmExFeatNext2                                = 0x02000000,  // was ...EseSetDbVersion - removed 2018/08/01 - around / bit after datacenter fork build AFTER this one v15.20.1058.000
    usbsmExFeatNext3                                = 0x04000000,
    usbsmExFeatNext4                                = 0x08000000,
    usbsmExFeatNext5                                = 0x10000000,
    usbsmExFeatNext6                                = 0x20000000,
    usbsmExFeatNext7                                = 0x40000000,
    usbsmExFeatNext8                                = 0x80000000,

    //  Add explicit features for testing above here.
    usbsmExFeatLast,
    usbsmExFeatureMask                              = 0xFF00002A
};
// The previous line needs to be "enum UtilSystemBetaSiteMode : ULONG;" to be explicit for some
// compilers, but let's compile this way for now to prove that it actually IS a ULONG.
C_ASSERT( sizeof( UtilSystemBetaSiteMode ) == sizeof( ULONG ) );

DEFINE_ENUM_FLAG_OPERATORS_BASIC( UtilSystemBetaSiteMode );

UtilSystemBetaSiteMode usbsmPrimaryEnvironments    = ( usbsmTestEnvAll | usbsmSelfhostAll | usbsmProdAll );
UtilSystemBetaSiteMode usbsmExFeatures             = usbsmExFeatureMask;
#ifdef DEBUG    // used for unit tests
UtilSystemBetaSiteMode usbsmExFeatureMin           = usbsmExFeatRiskyFeatTest;
UtilSystemBetaSiteMode usbsmExFeatureMax           = usbsmExFeatLast;
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
C_ASSERT( fLoggedEventAlready ); // testing that implicit BOOL usage works, used this way

UtilSystemBetaConfig    g_rgbetaconfigs [] =
{
    //  Note the goal is to keep this list to zero (or near zero) ... after a feature is deployed
    //  everywhere and is sound - then we remove the line from this table.

    // { fSuppressInfoEvent, fStaticFeature, ulFeatureId,           usbsm },
    { fTrue,  fFeatureDynamic, EseTestFeatures,                     usbsmTestEnvAll },

    //  Add new features above here.

#ifndef OS_LAYER_VIOLATIONS
    { fTrue,  fFeatureDynamic, EseTestCase,                         usbsmTestEnvAll OnDebug( | usbsmSelfhostAlphaMode ) OnNonRTM( | usbsmSelfhostBetaMode ) | usbsmExFeatOtherFeatTest },
    { fFalse, fFeatureStatic,  EseTestCaseTwo,                      usbsmTestEnvAll  /* ! in prod */ },
    { fFalse, fFeatureDynamic, EseTestCaseThree,                    usbsmProdAll /* ! in test */ },
    { fFalse, fFeatureDynamic, EseRiskyFeatTest,                    usbsmSelfhostAlphaMode | usbsmExFeatRiskyFeatTest },
#endif
};
C_ASSERT( _countof(g_rgbetaconfigs) == EseFeatureMax );

void UtilReportBetaFeatureInUse( const INST * const pinst, const UtilSystemBetaSiteMode usbsmCurrent, const ULONG featureid, PCWSTR const wszFeatureName )
{
    //  Ensure we only log the event once, by suppressing the event here after ...
    //  Note while we will only log the event once - in the event of two threads competing ... the
    //  2nd thread will go on to actually perform the feature potentially before the first / winning
    //  thread logs the event that we're using the feature. :P  Doesn't seem worth a critsect.
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


//  retrieves licensing policy configuration

ERR ErrUtilSystemSlConfiguration(
    _In_z_ const WCHAR *    pszValueName,
    _Out_   DWORD *         pdwValue )
{
    Assert( pdwValue );
    ERR err = JET_errSuccess;

    ExpectedSz( 0 == LOSStrCompareW( pszValueName, L"ExtensibleStorageEngine-ISAM-ParamConfiguration" ),
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
    err = ErrERRCheck( JET_errDisabledFunctionality );  // may be overwritten by config overrides below...
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

// I'm going to leave this without a #ifdef ESENT so we can get some testing on Exchange side of the
// actual Windows WNF_* API we want to use here.
#define ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

#ifdef ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

#include "NTSecAPI.h" // for NTSTATUS typedef

// Exchange doesn't have ntdef.h available?

#ifndef _DEFINED__WNF_STATE_NAME
#define _DEFINED__WNF_STATE_NAME
typedef struct _WNF_STATE_NAME {
    ULONG Data[2];
} WNF_STATE_NAME;


// Exchange doesn't have mrmwnf.h, ntexapi.h, or ntosifs.h either ... ugh.
//
// The change stamp is a value associated with the update that
// uniquely identifies the version of the data for the particular
// state within its scope.
//

typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;

   typedef struct _WNF_TYPE_ID
    {
        GUID TypeId;
    } WNF_TYPE_ID;

    typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;
    typedef struct _WNF_TYPE_ID* PWNF_TYPE_ID;
    typedef const struct _WNF_TYPE_ID* PCWNF_TYPE_ID;
//    typedef struct _WNF_USER_SUBSCRIPTION *PWNF_USER_SUBSCRIPTION;


// Exchange doesn't have wnfnamesp.h ... jeez how many .h files can one API need!
//
// WNF_DEP_UNINSTALL_DISABLED
//
//   Description : This event triggers when the Uninstall of the system has been disabled
//   Sequence    : 2
//   Type        : WnfWellKnownStateName
//   Scope       : WnfDataScopeSystem
//   SDDL        : D:(A;;1;;;AU)(A;;3;;;SY)(A;;3;;;BA)
//   Data size   : 4
//   Data format : <untyped>
//

EXTERN_C __declspec(selectany) const WNF_STATE_NAME
    WNF_DEP_UNINSTALL_DISABLED = {0xa3bc1475, 0x41960b29};


typedef struct _WNF_STATE_NAME* PWNF_STATE_NAME;
typedef const struct _WNF_STATE_NAME* PCWNF_STATE_NAME;

#define STATUS_RETRY                            ((NTSTATUS)0xC000022DL) // ntstatus
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth

_Always_(_Post_satisfies_(return == STATUS_NO_MEMORY || return == STATUS_RETRY || return == STATUS_SUCCESS))
_Function_class_(WNF_USER_CALLBACK)
typedef
NTSTATUS
// NTAPI
WNF_USER_CALLBACK(
    _In_ WNF_STATE_NAME StateName,
    _In_ WNF_CHANGE_STAMP ChangeStamp,
    _In_opt_ PWNF_TYPE_ID TypeId,
    _In_opt_ PVOID CallbackContext,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_ ULONG Length
    );
typedef WNF_USER_CALLBACK *PWNF_USER_CALLBACK;

// think this is __declspec(dllimport) ... so don't need it.
//NTSYSAPI
NTSTATUS
// NTAPI
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
    // Why would it be 0?  But other windows code expected it, so we'll handle it.
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

    return 0; // STATUS_SUCCESS;
}

#endif // ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

ERR ErrUtilOsDowngradeWindowExpired( _Out_ BOOL * pfExpired )
{
    NTOSFuncError( pfnRtlQueryWnfStateData, g_mwszzNtdllLibs, RtlQueryWnfStateData, oslfExpectedOnWin10 );

    //  Valid values for UlConfigOverrideInjection - 50026
    //    0 / fFalse   - Force JET_errSuccess + pfExpired == fFalse.
    //    1 / fTrue    - Force JET_errSuccess + pfExpired == fTrue.
    //    3 / <Unset>  - [NT] Passes through to real WinAPI; [EX] Always returns JET_errSuccess + fTrue.  (also the Retail / Free Behavior)
    //    4            - Allow real WinAPI result to be returned.  May produce inconsistent test results if the flag could be both off and on depending on OS state / pathces.
    //    -106 / <Neg> - Return -106 and leave pfExpired unset.
    //
    const ERR errFaultInj = (ERR)UlConfigOverrideInjection( 50026, 3 );
    if ( errFaultInj < JET_errSuccess )
    {
        // treat as err
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
        //  Perhaps we should do proper error mapping.  Error not returned to user though, soft fail of upgrade.
        return ErrERRCheck( JET_errTaskDropped );
#endif
    }

#else // !ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK
    Assert( errFaultInj != 4 ); // impossible - remove cases of 4 from test code then.
#endif // ENABLE_WINPRIV_DOWNGRADE_WINDOW_CHECK

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

//  terminate sysinfo subsystem

void OSSysinfoTerm()
{
    OSSysTraceStationId( tsidrCloseTerm );

    //  reset static features and event log statusi

    for( ULONG featureid = 0; featureid < _countof(g_rgbetaconfigs); featureid++ )
    {
        Assert( g_rgbetaconfigs[featureid].fStaticFeature == fFeatureDynamic || g_rgbetaconfigs[featureid].fStaticFeature == fFeatureStatic );
        if ( g_rgbetaconfigs[featureid].fSuppressInfoEvent == fLoggedEventAlready )
        {
            //  reset so we'll re-log the event
            (void)AtomicExchange( &( g_rgbetaconfigs[featureid].fSuppressInfoEvent ), fFalse );
        }
    }
}

//  init sysinfo subsystem

ERR ErrOSSysinfoInit()
{
    OSSysTraceStationId( tsidrOpenInit );

    return JET_errSuccess;
}
