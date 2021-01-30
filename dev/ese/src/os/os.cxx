// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"


volatile BOOL g_fOsLayerUp = fFalse;

BOOL FOSLayerUp()
{
    return g_fOsLayerUp;
}


extern volatile BOOL g_fProcessAbort;

void OSIProcessAbort()
{
    
    g_fProcessAbort = fTrue;


    OSSyncProcessAbort();
}


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

void OSPostterm()
{
    PreinitTrace( L"Begin OSPostterm()\n" );


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

#ifdef RTM
#define PREINIT_FAILURE_POINT 0
#define InitFailurePointsFromRegistry()
#else


LOCAL INT g_cFailurePoints;

#define PREINIT_FAILURE_POINT (!(++g_cFailurePoints))

LOCAL VOID InitFailurePointsFromRegistry()
{
    g_cFailurePoints = 0;

    NTOSFuncError( pfnRegOpenKeyExW, g_mwszzRegistryLibs, RegOpenKeyExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncError( pfnRegQueryValueExW, g_mwszzRegistryLibs, RegQueryValueExW, oslfExpectedOnWin5x | oslfRequired );
    NTOSFuncError( pfnRegCloseKey, g_mwszzRegistryLibs, RegCloseKey, oslfExpectedOnWin5x | oslfRequired );


    HKEY hkeyPath;
    DWORD dw = pfnRegOpenKeyExW(    HKEY_LOCAL_MACHINE,
                                    L"SOFTWARE\\Microsoft\\" WSZVERSIONNAME L"\\Global",
                                    0,
                                    KEY_READ,
                                    &hkeyPath );

    if ( dw != ERROR_SUCCESS )
    {
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



void COSLayerPreInit::SetDefaults()
{

    SetAssertAction( JET_AssertFailFast );
    SetExceptionAction( JET_ExceptionFailFast );
    SetCatchExceptionsOnBackgroundThreads( fTrue );
    SetRFSAlloc( 0xffffffff );
    SetRFSIO( 0xffffffff );

    SetZeroExtend( 1024 * 1024 );
    SetIOMaxOutstanding( 1024 );
    SetIOMaxOutstandingBackground( 32 );


    SetEventLogCache( 0 );
#ifdef PERFMON_SUPPORT
    EnablePerfmon();
#endif

    SetThreadWaitBeginNotification( NULL );
    SetThreadWaitEndNotification( NULL );
}

extern volatile BOOL g_fDllUp;

COSLayerPreInit::COSLayerPreInit() :
    m_fInitedSuccessfully( fFalse )
{
    Assert( !m_fInitedSuccessfully  );

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
    
extern CFileIdentification g_fident;
extern CCacheRepository g_crep;

COSLayerPreInit::~COSLayerPreInit()
{
    if ( m_fInitedSuccessfully )
    {
        g_fident.Cleanup();
        g_crep.Cleanup();

        g_fDllUp = fFalse;
        OSPostterm();
    }
}



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

