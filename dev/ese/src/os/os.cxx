// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"

//  up-ness variables

volatile BOOL g_fOsLayerUp = fFalse;

BOOL FOSLayerUp()
{
    return g_fOsLayerUp;
}

//  signal the OS subsystem that the process is aborting

extern volatile BOOL g_fProcessAbort;

void OSIProcessAbort()
{
    //  set the global process abort state.
    
    g_fProcessAbort = fTrue;

    //  signal the OS subsystems that need to know about unexpected process termination

    OSSyncProcessAbort();
}

//  post-terminate OS subsystem

extern void OSEdbgPostterm();
extern void OSEncryptionPostterm();
extern void OSEventTracePostterm();
extern void OSPerfmonPostterm();
extern void OSNormPostterm();
extern void OSCprintfPostterm();
extern void OSFSPostterm();
extern void OSFilePostterm();
extern void OSDiskPostterm();
extern void OSTaskPostterm();
extern void OSRMPostterm();
extern void OSTimerQueuePostterm();
extern void OSThreadPostterm();
extern void OSMemoryPostterm();
extern void OSErrorPostterm();
extern void OSEventPostterm();
extern void OSTracePostterm();
extern void OSDiagPostterm();
extern void OSConfigPostterm();
extern void OSTimePostterm();
extern void OSSysinfoPostterm();
extern void OSLibraryPostterm();
extern void OSHaPublishPostterm();
extern void OSBlockCachePostterm();

void OSPostterm()
{
    PreinitTrace( L"Begin OSPostterm()\n" );

    //  terminate all OS subsystems in reverse dependency order

    OSBlockCachePostterm();
    OSEdbgPostterm();
    OSEncryptionPostterm();
    OSEventTracePostterm();
    OSPerfmonPostterm();
    OSNormPostterm();
    OSCprintfPostterm();
    OSFSPostterm();
    OSFilePostterm();
    OSDiskPostterm();
    OSTaskPostterm();
    OSRMPostterm();
    OSTimerQueuePostterm();
    OSDiagPostterm();
    OSThreadPostterm();
    OSMemoryPostterm();
    OSSyncPostterm();
    OSErrorPostterm();
    OSEventPostterm();
    OSTracePostterm();
    OSConfigPostterm();
    OSTimePostterm();
    OSSysinfoPostterm();
    OSLibraryPostterm();
    OSHaPublishPostterm();

    PreinitTrace( L"Finish OSPostterm()\n" );
}

//  pre-init OS subsystem

extern bool FOSHaPublishPreinit();
extern BOOL FOSLibraryPreinit();
extern BOOL FOSSysinfoPreinit();
extern BOOL FOSTimePreinit();
extern BOOL FOSConfigPreinit();
extern BOOL FOSDiagPreinit();
extern BOOL FOSTracePreinit();
extern BOOL FOSEventPreinit();
extern BOOL FOSErrorPreinit();
extern BOOL FOSMemoryPreinit();
extern BOOL FOSThreadPreinit();
extern BOOL FOSTimerQueuePreinit();
extern BOOL FOSRMPreinit();
extern BOOL FOSTaskPreinit();
extern BOOL FOSDiskPreinit();
extern BOOL FOSFilePreinit();
extern BOOL FOSFSPreinit();
extern BOOL FOSCprintfPreinit();
extern BOOL FOSNormPreinit();
extern BOOL FOSPerfmonPreinit();
extern BOOL FOSEventTracePreinit();
extern BOOL FOSEncryptionPreinit();
extern BOOL FOSEdbgPreinit();
extern BOOL FOSBlockCachePreinit();

#ifdef RTM
#define PREINIT_FAILURE_POINT 0
#define InitFailurePointsFromRegistry()
#else

//  Need to test error-handling for DLL load failures
//  g_cFailurePoints should be a negative number, indicating
//  the failure point that we should stop at
//
//  The failure point count will be loaded from the registry
//
//  To test this, write a test that increments the failure point
//  until the DLL loads successfully
//

LOCAL INT g_cFailurePoints;

#define PREINIT_FAILURE_POINT (!(++g_cFailurePoints))

//  this is called before anything is init
//  so we can't use any of the wrapper functions
LOCAL VOID InitFailurePointsFromRegistry()
{
    g_cFailurePoints = 0;

    NTOSFuncError( pfnRegOpenKeyExW, g_mwszzRegistryLibs, RegOpenKeyExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncError( pfnRegQueryValueExW, g_mwszzRegistryLibs, RegQueryValueExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncError( pfnRegCloseKey, g_mwszzRegistryLibs, RegCloseKey, oslfExpectedOnWin5x | oslfRequired );

    //  open registry key with this path

    HKEY hkeyPath;
    DWORD dw = pfnRegOpenKeyExW(    HKEY_LOCAL_MACHINE,
                                    L"SOFTWARE\\Microsoft\\" WSZVERSIONNAME L"\\Global",
                                    0,
                                    KEY_READ,
                                    &hkeyPath );

    if ( dw != ERROR_SUCCESS )
    {
        //  we failed to open the key. do nothing
    }
    else
    {
        DWORD dwType;
        WCHAR wszBuf[32];
        DWORD cbValue = sizeof( wszBuf );

        dw = pfnRegQueryValueExW(   hkeyPath,
                                    L"DLLInitFailurePoint",
                                    0,
                                    &dwType,
                                    (LPBYTE)wszBuf,
                                    &cbValue );

        pfnRegCloseKey( hkeyPath );

        if ( ERROR_SUCCESS == dw && REG_SZ == dwType )
        {
            g_cFailurePoints = 0 - _wtol( wszBuf );
        }
    }

    return;
}

#endif

BOOL FOSPreinit()
{
    PreinitTrace( L"Begin FOSPreinit()\n" );

    InitFailurePointsFromRegistry();

    //  initialize all OS subsystems in dependency order

    if (
            PREINIT_FAILURE_POINT ||
            !FOSHaPublishPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSLibraryPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSSysinfoPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSTimePreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSConfigPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSTracePreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSEventPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSErrorPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSSyncPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSMemoryPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSThreadPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSDiagPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSTimerQueuePreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSRMPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSTaskPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSDiskPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSFilePreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSFSPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSCprintfPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSNormPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSPerfmonPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSEventTracePreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSEncryptionPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSEdbgPreinit() ||
            PREINIT_FAILURE_POINT ||
            !FOSBlockCachePreinit() ||
            PREINIT_FAILURE_POINT
            )
    {
        goto HandleError;
    }

    PreinitTrace( L"Finish FOSPreinit() success\n" );
    return fTrue;

HandleError:
    PreinitTrace( L"Finish FOSPreinit() failed ... begin OSPostterm()\n" );
    OSPostterm();
    return fFalse;
}


//  init OS subsystem

extern ERR ErrOSLibraryInit();
extern ERR ErrOSSysinfoInit();
extern ERR ErrOSTimeInit();
extern ERR ErrOSConfigInit();
extern ERR ErrOSDiagInit();
extern ERR ErrOSTraceInit();
extern ERR ErrOSEventInit();
extern ERR ErrOSErrorInit();
extern ERR ErrOSMemoryInit();
extern ERR ErrOSThreadInit();
extern ERR ErrOSTimerQueueInit();
extern ERR ErrOSRMInit();
extern ERR ErrOSTaskInit();
extern ERR ErrOSDiskInit();
extern ERR ErrOSFileInit();
extern ERR ErrOSFSInit();
extern ERR ErrOSCprintfInit();
extern ERR ErrOSNormInit();
extern ERR ErrOSPerfmonInit();
extern ERR ErrOSEventTraceInit();
extern ERR ErrOSEncryptionInit();
extern ERR ErrOSEdbgInit();

ERR ErrOSInit()
{
    ERR err;

    PreinitTrace( L"Begin ErrOSInit()\n" );

    //  initialize all OS subsystems in dependency order

    Call( ErrOSLibraryInit() );
    Call( ErrOSSysinfoInit() );
    Call( ErrOSTimeInit() );
    Call( ErrOSConfigInit() );
    Call( ErrOSDiagInit() );
    Call( ErrOSTraceInit() );
    Call( ErrOSEventInit() );
    Call( ErrOSErrorInit() );
    Call( ErrOSMemoryInit() );
    Call( ErrOSThreadInit() );
    Call( ErrOSTimerQueueInit() );
    Call( ErrOSRMInit() );
    Call( ErrOSTaskInit() );
    Call( ErrOSDiskInit() );
    Call( ErrOSFileInit() );
    Call( ErrOSFSInit() );
    Call( ErrOSCprintfInit() );
    Call( ErrOSNormInit() );
    Call( ErrOSPerfmonInit() );
    Call( ErrOSEventTraceInit() );
    Call( ErrOSEncryptionInit() );
    Call( ErrOSEdbgInit() );

    g_fOsLayerUp = fTrue;

    PreinitTrace( L"Finish ErrOSInit() success\n" );

    return JET_errSuccess;

HandleError:
    PreinitTrace( L"Finish ErrOSInit() failed (%d) ... begin OSTerm()\n", err );
    OSTerm();
    return err;
}

//  terminate OS subsystem

extern void OSEdbgTerm();
extern void OSEncryptionTerm();
extern void OSEventTraceTerm();
extern void OSPerfmonTerm();
extern void OSNormTerm();
extern void OSCprintfTerm();
extern void OSFSTerm();
extern void OSFileTerm();
extern void OSDiskTerm();
extern void OSTaskTerm();
extern void OSRMTerm();
extern void OSTimerQueueTerm();
extern void OSThreadTerm();
extern void OSMemoryTerm();
extern void OSErrorTerm();
extern void OSEventTerm();
extern void OSTraceTerm();
extern void OSDiagTerm();
extern void OSConfigTerm();
extern void OSTimeTerm();
extern void OSSysinfoTerm();
extern void OSLibraryTerm();

void OSTerm()
{
    PreinitTrace( L"Begin OSTerm()\n" );

    g_fOsLayerUp = fFalse;

    //  terminate all OS subsystems in reverse dependency order

    OSEdbgTerm();
    OSEncryptionTerm();
    OSEventTraceTerm();
    OSPerfmonTerm();
    OSNormTerm();
    OSCprintfTerm();
    OSFSTerm();
    OSFileTerm();
    OSDiskTerm();
    OSTaskTerm();
    OSRMTerm();
    OSTimerQueueTerm();
    OSThreadTerm();
    OSMemoryTerm();
    OSErrorTerm();
    OSEventTerm();
    OSTraceTerm();
    OSDiagTerm();
    OSConfigTerm();
    OSTimeTerm();
    OSSysinfoTerm();
    OSLibraryTerm();

    PreinitTrace( L"Finish OSTerm()\n" );
}

// Simplified EXE OS Layer support.


void COSLayerPreInit::SetDefaults()
{

    //  debugging
    //
    SetAssertAction( JET_AssertFailFast );
    SetExceptionAction( JET_ExceptionFailFast );
    SetCatchExceptionsOnBackgroundThreads( fTrue );
    SetRFSAlloc( 0xffffffff );
    SetRFSIO( 0xffffffff );

    //  os file settings
    //
    SetZeroExtend( 1024 * 1024 );
    SetIOMaxOutstanding( 1024 );
    SetIOMaxOutstandingBackground( 32 );

    //  analysis sub-systems, logging, perfmon, etc...
    //

    SetEventLogCache( 0 );  // event log cache disabled by default
#ifdef PERFMON_SUPPORT
    EnablePerfmon( 100 );   // by default we use perfmon.
#endif

    //  performance data
    //
    SetThreadWaitBeginNotification( NULL );
    SetThreadWaitEndNotification( NULL );
}

//  While it's called "fDllUp() really it means CRT & OSPreinit has run, so for library-challenged binaries (such as .exes) we
//  need to set this after they run OSPreInit.
extern volatile BOOL g_fDllUp;

COSLayerPreInit::COSLayerPreInit() :
    m_fInitedSuccessfully( fFalse )
{
    Assert( !m_fInitedSuccessfully  );

    //  Pre-init the OS Layer ...
    m_fInitedSuccessfully = FOSPreinit();

    if ( m_fInitedSuccessfully )
    {
        COSLayerPreInit::SetDefaults();
        g_fDllUp = fTrue;
    }
}

BOOL COSLayerPreInit::FInitd()
{
    return m_fInitedSuccessfully;
}

COSLayerPreInit::~COSLayerPreInit()
{
    if ( m_fInitedSuccessfully )
    {
        //  Post-term the OS Layer ...
        g_fDllUp = fFalse;
        OSPostterm();
    }
}


//  support for thread wait notifications

static void OSThreadWaitBegin()
{
}

static COSLayerPreInit::PfnThreadWait g_pfnThreadWaitBegin = OSThreadWaitBegin;

static void OSSYNCAPI OSSyncThreadWaitBegin()
{
    g_pfnThreadWaitBegin();
}

COSLayerPreInit::PfnThreadWait COSLayerPreInit::SetThreadWaitBeginNotification( COSLayerPreInit::PfnThreadWait pfnThreadWait )
{
    PfnThreadWait pfnThreadWaitOriginal = g_pfnThreadWaitBegin;
    g_pfnThreadWaitBegin = pfnThreadWait ? pfnThreadWait : OSThreadWaitBegin;

    OSSyncOnThreadWaitBegin( OSSyncThreadWaitBegin );

    return pfnThreadWaitOriginal;
}

VOID OnThreadWaitBegin()
{
    g_pfnThreadWaitBegin();
}

static void OSThreadWaitEnd()
{
}

static COSLayerPreInit::PfnThreadWait g_pfnThreadWaitEnd = OSThreadWaitEnd;

static void OSSYNCAPI OSSyncThreadWaitEnd()
{
    g_pfnThreadWaitEnd();
}

COSLayerPreInit::PfnThreadWait COSLayerPreInit::SetThreadWaitEndNotification( COSLayerPreInit::PfnThreadWait pfnThreadWait )
{
    PfnThreadWait pfnThreadWaitOriginal = g_pfnThreadWaitEnd;
    g_pfnThreadWaitEnd = pfnThreadWait ? pfnThreadWait : OSThreadWaitEnd;

    OSSyncOnThreadWaitEnd( OSSyncThreadWaitEnd );

    return pfnThreadWaitOriginal;
}

VOID OnThreadWaitEnd()
{
    g_pfnThreadWaitEnd();
}

