// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "osstd.hxx"
#include "trace.hxx"

#ifdef USE_WATSON_API
#include <NativeWatson.h>
#endif

#include <werapi.h>

#ifndef WIDEN
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#endif
CRITICAL_SECTION    g_csError;
BOOL                g_fCritSecErrorInit = fFalse;

DWORD               g_tidAssertFired = 0x0;
CErrFrameSimple     g_cerrDoubleAssert;

BOOL                g_fRetryException   = fFalse;

#ifdef DEBUG
WCHAR               g_wszAssertText[1024];
#endif
DWORD               g_fSkipAssert = fFalse;


const CHAR szNewLine[]          = "\r\n";
const WCHAR wszNewLine[]        = L"\r\n";

enum { cchPtrHexWidth = sizeof( void* ) * 8 / 4 };


__forceinline CErrFrameSimple * PefLastThrow()
{
    return Postls() ? ( &(Postls()->m_efLastErr) ) : NULL;
}

ULONG UlLineLastCall()
{
#ifdef DEBUG
    return Postls() ? Postls()->ulLineLastCall : 0;
#else
    return 0;
#endif
}




INT UtilMessageBoxW( IN const WCHAR * const wszText, IN const WCHAR * const wszCaption, IN const UINT uType )
{
    INT                     retval          = 0;

    NTOSFuncCount( pfnMessageBox, g_mwszzUserInterfaceLibs, MessageBoxW, oslfExpectedOnWin5x | oslfStrictFree );

    if ( pfnMessageBox.ErrIsPresent() < JET_errSuccess )
    {
        goto NoMessageBox;
    }

    retval = (*pfnMessageBox)( NULL, wszText, wszCaption, uType );

NoMessageBox:

    return retval;
}


BOOL IsDebuggerAttached()
{
    BOOL                    fDebuggerPresent        = fFalse;

    NTOSFuncStd( pfnIsDebuggerPresent, g_mwszzCoreDebugLibs, IsDebuggerPresent, oslfExpectedOnWin5x );

    if ( pfnIsDebuggerPresent.ErrIsPresent() < JET_errSuccess )
    {
        goto NoIsDebuggerPresent;
    }

    fDebuggerPresent = pfnIsDebuggerPresent();

NoIsDebuggerPresent:
    return fDebuggerPresent;
}


BOOL IsDebuggerAttachable()
{
    extern volatile DWORD g_tidDLLEntryPoint;

    return g_tidDLLEntryPoint != GetCurrentThreadId();
}


void KernelDebugBreakPoint()
{
    DebugBreak();
}

void UserDebugBreakPoint()
{
    DWORD dwSavedGLE = GetLastError();


#if defined(MINIMAL_FUNCTIONALITY) && !defined(DEBUG)
    *((INT*)NULL) = 0;
#else
    if ( !IsDebuggerAttached() )
    {

        if ( IsDebuggerAttachable() )
        {
            while ( !IsDebuggerAttached() )
            {

                WCHAR szCmdFormat[ 256 ];

                if ( !GetProfileStringW(    L"AeDebug",
                                        L"Debugger",
                                        L"",
                                        szCmdFormat,
                                        _countof( szCmdFormat ) - 1 ) )
                {
                    szCmdFormat[ 0 ] = L'\0';
                }


                WCHAR szCmdFname[ 256 ];

                _wsplitpath_s( szCmdFormat, NULL, 0, NULL, 0, szCmdFname, _countof(szCmdFname), NULL, 0 );
                if ( !_wcsicmp( szCmdFname, L"drwtsn32" ) )
                {
                    szCmdFormat[ 0 ] = L'\0';
                }


                SECURITY_ATTRIBUTES sa;
                HANDLE              hEvent;
                STARTUPINFOW        si;
                PROCESS_INFORMATION pi;
                WCHAR               szCmd[ 256 ];

                sa.nLength              = sizeof( SECURITY_ATTRIBUTES );
                sa.lpSecurityDescriptor = NULL;
                sa.bInheritHandle       = TRUE;

                hEvent = CreateEventW( &sa, TRUE, FALSE, NULL );

                memset( &si, 0, sizeof( si ) );

                OSStrCbFormatW( szCmd, sizeof(szCmd), szCmdFormat, GetCurrentProcessId(), hEvent );
                Assert( wcslen( szCmd ) < _countof(szCmd) );

                si.cb           = sizeof( si );
                si.lpDesktop    = L"Winsta0\\Default";

                if (    hEvent &&
                        CreateProcessW( NULL,
                                        szCmd,
                                        NULL,
                                        NULL,
                                        TRUE,
                                        0,
                                        NULL,
                                        NULL,
                                        &si,
                                        &pi ) )
                {
                    for ( DWORD dw = WaitForSingleObjectEx( hEvent, 3000, TRUE );
                        ( WAIT_IO_COMPLETION == dw || WAIT_TIMEOUT == dw ) && !IsDebuggerAttached();
                        dw = WaitForSingleObjectEx( hEvent, 30000, TRUE ) )
                    {
                        NULL;
                    }

                    CloseHandle( pi.hProcess );
                    CloseHandle( pi.hThread );
                }


                else
                {
                    WCHAR wszMessage[ 1024 ];

                    if ( !szCmdFormat[ 0 ] )
                    {
                        OSStrCbFormatW( wszMessage, sizeof( wszMessage ), L"Unable to start debugger automatically." );
                    }
                    else
                    {
                        DWORD   gle         = GetLastError();
                        WCHAR*  wszMsgBuf   = NULL;

                        FormatMessageW( (   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                            FORMAT_MESSAGE_FROM_SYSTEM |
                                            FORMAT_MESSAGE_MAX_WIDTH_MASK ),
                                        NULL,
                                        gle,
                                        MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                                        (LPWSTR) &wszMsgBuf,
                                        0,
                                        NULL );
                        OSStrCbFormatW( wszMessage, sizeof( wszMessage ),
                                    L"The debugger could not be attached to process %d!  Win32 error %d%s%s",
                                    GetCurrentProcessId(),
                                    gle,
                                    wszMsgBuf ? L":  " : L".",
                                    wszMsgBuf ? wszMsgBuf : L"" );

                        LocalFree( (LPVOID)wszMsgBuf );
                    }

                    if ( UtilMessageBoxW(   wszMessage,
                                            WszUtilImageVersionName(),
                                            MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP | MB_RETRYCANCEL ) == IDCANCEL )
                    {
                        break;
                    }
                }
                if ( hEvent )
                {
                    CloseHandle( hEvent );
                }
            }
        }


        if ( !IsDebuggerAttached() )
        {
            TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );
        }
    }
#endif


    SetLastError(dwSavedGLE);
    DebugBreak();
}

static void ForceProcessCrash()
{
    *( char* )0 = 0;
}

static void TryRaiseFailFastException(
    const PEXCEPTION_RECORD pExceptionRecord,
    const PCONTEXT pContextRecord,
    const DWORD dwFlags)
{
#ifdef OS_LAYER_VIOLATIONS
#ifdef USE_WATSON_API
    EXCEPTION_POINTERS exceptionPointers;
    exceptionPointers.ExceptionRecord = pExceptionRecord;
    exceptionPointers.ContextRecord = pContextRecord;
    SendWatsonReport( &exceptionPointers );
#else
    extern ERR ErrBFConfigureProcessForCrashDump( const JET_GRBIT grbit );

    (VOID)ErrBFConfigureProcessForCrashDump( JET_bitDumpCacheMaximum | JET_bitDumpCacheNoDecommit );
#endif
#endif
    NTOSFuncVoid( pfnRaiseFailFastException, g_mwszzErrorHandlingLegacyLibs, RaiseFailFastException, oslfExpectedOnWin7 | oslfStrictFree );

    if ( pfnRaiseFailFastException.ErrIsPresent() >= JET_errSuccess )
    {
        pfnRaiseFailFastException( pExceptionRecord, pContextRecord, dwFlags );
    }
}

VOID OSErrorRegisterForWer( VOID *pv, DWORD cb )
{
    (VOID)WerRegisterMemoryBlock( pv, cb );
}

static void RaiseFailFastException()
{

    TryRaiseFailFastException( NULL, NULL, FAIL_FAST_GENERATE_EXCEPTION_ADDRESS );

    ForceProcessCrash();
}

LOCAL void RaiseFailFastExceptionCheckingSkipAssert()
{
    __try
    {
        ForceProcessCrash();
    }
    __except ( g_fSkipAssert ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
    {
        g_fSkipAssert = fFalse;
    }
}

#if defined( DEBUG ) || defined( MEM_CHECK ) || defined( ENABLE_EXCEPTIONS )

const WCHAR wszAssertFile[]     = L"assert.txt";

const WCHAR wszAssertCaption[]  = L"JET Assertion Failure";
const WCHAR wszAssertPrompt[]   = L"Choose OK to continue execution or CANCEL to debug the process.";
const WCHAR wszAssertPrompt2[]  = L"Choose OK to continue execution (attaching the debugger is impossible during process initialization and termination).";

#endif

UINT g_wAssertAction = JET_AssertFailFast;
UINT g_wExceptionAction = JET_ExceptionFailFast;
BOOL g_fSkipFailFast = fFalse;

UINT COSLayerPreInit::SetAssertAction( UINT wAssertAction )
{
    UINT wOriginalValue = g_wAssertAction;
    g_wAssertAction = wAssertAction;
    return wOriginalValue;
}

UINT COSLayerPreInit::SetExceptionAction( const UINT wExceptionAction )
{
    const UINT wOriginalValue = g_wExceptionAction;
    g_wExceptionAction = wExceptionAction;
    return wOriginalValue;
}

INT g_fNoWriteAssertEvent = 0;

#define CCH_MAX_SHORT_FILENAME  (30)
#define CCH_32_BIT_MAX          (11)
static const ULONG  g_cchIssueSourceMax = 40 +
                                            CCH_MAX_SHORT_FILENAME * 2 +
                                            CCH_32_BIT_MAX * 12;
VOID ERRFormatIssueSource( __out_bcount( cbIssueSource ) WCHAR * wszIssueSource, __in const ULONG cbIssueSource, const DWORD dwSavedGLE, __in PCSTR szFilename, __in const LONG lLine )
{

    const CHAR * szFilenameSourcePre    = "";
    const CHAR * szFilenameSource       = SzSourceFileName( szFilename );

    const CHAR * szFilenameLastErrPre   = "";
    const CHAR * szFilenameLastErr      = "";
    if ( FOSLayerUp() && PefLastThrow() )
    {
        szFilenameLastErr = SzSourceFileName( PefLastThrow()->SzFile() );
    }
    const ERR errLast = PefLastThrow() ? PefLastThrow()->Err() : 0;
    const ULONG lErrLastLine = PefLastThrow() ? PefLastThrow()->UlLine() : 0;

#ifdef TEST_ONCE_FORMAT_MAX
    szFilenameSource = "Asdfjkl;weroasdfasadfanawenrsernmasdfnasernaesrnaserasndfnasernaensraseezxASDFJKL;AsdfJkl.cxx";
    szFilenameLastErr = "1234234535341451451234123423412348283481724571237040213488123408123481823482340%s3%d0%%s12%hs4.hxx";
    const ULONG xWorstDword = 0xFFFFFFFF;
    const ULONG xWorstInt = 0x80000000;
#endif

    const CHAR * const  szDotDotDot = "...";

    if ( strlen( szFilenameSource ) > CCH_MAX_SHORT_FILENAME )
    {
        const ULONG cch = strlen( szFilenameSource );
        szFilenameSource = &( szFilenameSource[cch - CCH_MAX_SHORT_FILENAME + strlen(szDotDotDot)] );
        szFilenameSourcePre = szDotDotDot;
    }

    if ( strlen( szFilenameLastErr ) > CCH_MAX_SHORT_FILENAME )
    {
        const ULONG cch = strlen( szFilenameLastErr );
        szFilenameLastErr = &( szFilenameLastErr[cch - CCH_MAX_SHORT_FILENAME + strlen(szDotDotDot)] );
        szFilenameLastErrPre = szDotDotDot;
    }

    OSStrCbFormatW( wszIssueSource, cbIssueSource,
                        L"PV: %u.%u.%u.%u SV: %u.%u.%u.%u GLE: %u ERR: %d(%hs%hs:%u): %hs%hs(%u)",
#ifndef TEST_ONCE_FORMAT_MAX
                        DwUtilImageVersionMajor(), DwUtilImageVersionMinor(), DwUtilImageBuildNumberMajor(), DwUtilImageBuildNumberMinor(),
                        DwUtilSystemVersionMajor(), DwUtilSystemVersionMinor(), DwUtilSystemBuildNumber(), DwUtilSystemServicePackNumber(),
                        dwSavedGLE,
                        errLast, szFilenameLastErrPre, szFilenameLastErr, lErrLastLine,
                        szFilenameSourcePre, szFilenameSource, lLine
#else
                        xWorstDword, xWorstDword, xWorstDword, xWorstDword,
                        xWorstDword, xWorstDword, xWorstDword, xWorstDword,
                        xWorstDword,
                        xWorstInt, szFilenameLastErrPre, szFilenameLastErr, xWorstDword,
                        szFilenameSourcePre, szFilenameSource, xWorstDword
#endif
                        );
}


static const ULONG  g_cchIssueMsgMax = 1024 + 1;

#ifdef DEBUG



LOCAL DWORD g_pidAssert = GetCurrentProcessId();
LOCAL DWORD g_tidAssert = GetCurrentThreadId();
LOCAL const CHAR * g_szFilenameAssert;
LOCAL LONG g_lLineAssert;

HRESULT EDBGPrintf( __in PCSTR szFormat, ... );

LOCAL DWORD g_fDebuggerPrint = fTrue;

void OSDebugPrint( const WCHAR * const wszOutput )
{
    if ( g_fDebuggerPrint && IsDebuggerAttached() )
    {
        OutputDebugStringW( wszOutput );
    }
}

void HandleNestedAssert( WCHAR const *szMessageFormat, char const *szFilename, LONG lLine )
{
    DWORD dwSavedGLE = GetLastError();



    OSDebugPrint( L"\n\nDOUBLE ASSERT: " );
    OSDebugPrint( szMessageFormat );
    OSDebugPrint( L"\nSee g_cerrDoubleAssert for file/line info.\n" );
    OSDebugPrint( L"Breaking into debugger. 'g' to continue like nothing is wrong.\n\n" );
    g_cerrDoubleAssert.Set( szFilename, lLine, -1 );


    SetLastError( dwSavedGLE );
    KernelDebugBreakPoint();
}

void __stdcall AssertFail( char const *szMessageFormat, char const *szFilename, LONG lLine, ... )
{
    va_list args;
    va_start( args, lLine );
    INT offset = 0;

    if ( g_wAssertAction == JET_AssertSkipAll )
    {
        va_end( args );
        return;
    }

    CHAR                szAssertText[1024];

    
    DWORD dwSavedGLE = GetLastError();

    EnterCriticalSection( &g_csError );

    if ( g_tidAssertFired == DwUtilThreadId() )
    {

        SetLastError( dwSavedGLE );
        WCHAR wszMessageFormat[ _MAX_PATH ];
        (void)ErrOSSTRAsciiToUnicode( szMessageFormat,
                                      wszMessageFormat,
                                      _countof( wszMessageFormat ) );
        HandleNestedAssert( wszMessageFormat, szFilename, lLine );

        LeaveCriticalSection( &g_csError );
        SetLastError( dwSavedGLE );
        return;
    }

    g_tidAssertFired = DwUtilThreadId();

    INT             id;

    g_szFilenameAssert = szFilename;
    g_lLineAssert = lLine;

    
    szFilename = ( NULL == strrchr( szFilename, chPathDelimiter ) ) ? szFilename : strrchr( szFilename, chPathDelimiter ) + sizeof( CHAR );

    

    
    OSStrCbFormatA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset * sizeof( szAssertText[0] ),
        "Assertion Failure: " );
    offset = strlen( szAssertText );

    OSStrCbVFormatA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset,
        szMessageFormat,
        args );
    offset = strlen( szAssertText );

    OSStrCbAppendA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset,
        "\r\n" );
    offset = strlen( szAssertText );

    
    OSStrCbFormatA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset,
        "Rel.%d.%d, File %hs, Line %d, PID: %d (0x%x), TID: 0x%x \r\n\r\n",
        DwUtilImageBuildNumberMajor(),
        DwUtilImageBuildNumberMinor(),
        szFilename,
        lLine,
        DwUtilProcessId(),
        DwUtilProcessId(),
        DwUtilThreadId() );
    offset = strlen( szAssertText );

    
    if ( FOSLayerUp() )
    {
        
        OSStrCbFormatA(
            szAssertText + offset,
            sizeof( szAssertText ) - offset,
            "Last * Info (may or may not be relevant to your debug):\r\n"
            "\tESE Err: %d (%hs:%d)\r\n"
            "\tWin32 Error: %d\r\n",
            PefLastThrow() ? PefLastThrow()->Err() : 0,
            PefLastThrow() ? SzSourceFileName( PefLastThrow()->SzFile() ) : "",
            PefLastThrow() ? PefLastThrow()->UlLine() : 0,
            dwSavedGLE );
        offset = strlen( szAssertText );
    }

    
    OSStrCbFormatA(
        szAssertText + offset,
        sizeof( szAssertText ) - offset,
        "\r\nComplete information can be found in:  %ws\r\n",
        wszAssertFile );
    offset = strlen( szAssertText );

    OSTrace( JET_tracetagAsserts, szAssertText );

    
    
    if ( !g_fNoWriteAssertEvent && FOSDllUp() )
    {
        const WCHAR *   rgszT[] = { g_wszAssertText };

        (void)ErrOSSTRAsciiToUnicode( szAssertText,
                                      g_wszAssertText,
                                      _countof( g_wszAssertText ) );

        UtilReportEvent(
                eventError,
                GENERAL_CATEGORY,
                PLAIN_TEXT_ID,
                1,
                rgszT );
    }

    WCHAR wszIssueSource[ g_cchIssueSourceMax ] = L"FORMAT STRING FAIL";
    C_ASSERT( _countof( wszIssueSource ) < 260 );
    if ( FOSDllUp() )
    {
        ERRFormatIssueSource( wszIssueSource, sizeof( wszIssueSource ), dwSavedGLE, szFilename, lLine );
    }
    OSDiagTrackAssertFail( szMessageFormat, wszIssueSource );

{
    CPRINTFFILE cprintffileAssertTxt( wszAssertFile );
    cprintffileAssertTxt( "%ws", g_wszAssertText );
}

    UINT wAssertAction = g_wAssertAction;
    BOOL fDevMachine = fFalse;
#ifndef MINIMAL_FUNCTIONALITY
    fDevMachine = IsDevMachine();

    if ( fDevMachine )
    {
        switch ( wAssertAction )
        {
        case JET_AssertExit:
        case JET_AssertStop:
        case JET_AssertCrash:
        case JET_AssertFailFast:
            wAssertAction = JET_AssertBreak;
            break;

        case JET_AssertMsgBox:
        case JET_AssertSkippableMsgBox:
            if ( IsDebuggerAttached() )
            {
                wAssertAction = JET_AssertBreak;
            }
            break;

        default:
            break;
        }
    }
#endif

    
    OSDebugPrint( g_wszAssertText );

    
    if ( wAssertAction == JET_AssertExit )
    {
        TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );
    }
    else if ( wAssertAction == JET_AssertBreak )
    {
        OSDebugPrint( L"\nTo continue, press 'g'.\n" );
        SetLastError(dwSavedGLE);
        KernelDebugBreakPoint();
    }
    else if ( wAssertAction == JET_AssertStop )
    {
        for( ; !g_fSkipAssert; )
        {
            
            Sleep( 100 );
        }
        g_fSkipAssert = fFalse;
    }
    else if ( wAssertAction == JET_AssertCrash )
    {
        OSDebugPrint( L"\nTo continue, \"ed ese!g_fSkipAssert 1; g\".\n" );
        RaiseFailFastExceptionCheckingSkipAssert();
    }
    else if ( JET_AssertFailFast == wAssertAction )
    {
        if ( fDevMachine )
        {
            OSDebugPrint( L"\n NOTE: WILL FAIL FAST UNLESS you run \"ed ese!g_fSkipFailFast 1; g\".  Don't just press 'g'.\n" );
            SetLastError( dwSavedGLE );
            KernelDebugBreakPoint();
        }
        if ( !g_fSkipFailFast )
        {
            RaiseFailFastException();
        }
    }
    else if (   wAssertAction == JET_AssertMsgBox ||
                wAssertAction == JET_AssertSkippableMsgBox )
    {
        ULONG   cchOffset;


        cchOffset = wcslen( g_wszAssertText );

        OSStrCbFormatW( g_wszAssertText+cchOffset, _countof(g_wszAssertText) - cchOffset,
                L"%hs%hs" L"%ws",
                szNewLine, szNewLine,
                IsDebuggerAttachable() || IsDebuggerAttached() ? wszAssertPrompt : wszAssertPrompt2
                 );

        cchOffset += wcslen( g_wszAssertText + cchOffset );

        id = UtilMessageBoxW(   g_wszAssertText,
                                wszAssertCaption,
                                MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP |
                                ( IsDebuggerAttachable() || IsDebuggerAttached() ? MB_OKCANCEL : MB_OK ) );

        if ( IDOK != id || wAssertAction == JET_AssertMsgBox )
        {
            SetLastError(dwSavedGLE);
            UserDebugBreakPoint();
        }
    }
    else if ( wAssertAction == JET_AssertSkipAll )
    {
    }

    g_tidAssertFired = 0x0;
    LeaveCriticalSection( &g_csError );

    
    va_end( args );

    
    SetLastError(dwSavedGLE);
    return;
}


void AssertErr( const ERR err, PCSTR szFileName, const LONG lLine )
{
    DWORD dwSavedGLE = GetLastError();

    if ( JET_errSuccess == err )
    {
        FireWallAt( "UnexpectedAssertErrOnSuccess", szFileName, lLine );
    }
    else
    {
        FireWallAt( OSFormat( "AssertErr:%d", err ), szFileName, lLine );
    }

    SetLastError( dwSavedGLE );
}

#else

extern ULONG_PTR UlParam( const INST* const pinst, const ULONG paramid );

void __stdcall AssertFail( PCSTR szMessage, PCSTR szFilename, LONG lLine, ... )
{
    
    DWORD dwSavedGLE = GetLastError();

    

    WCHAR wszIssueSource[  g_cchIssueSourceMax ] = L"FORMAT STRING FAIL";
    C_ASSERT( _countof( wszIssueSource ) < 260 );

    if ( FOSDllUp() )
    {
        ERRFormatIssueSource( wszIssueSource, sizeof( wszIssueSource ), dwSavedGLE, szFilename, lLine );
    }

    
    if ( !g_fNoWriteAssertEvent && FOSDllUp() )
    {
        WCHAR wszMessage[ g_cchIssueMsgMax ] = L"FORMAT STRING FAIL";
        OSStrCbFormatW( wszMessage, sizeof( wszMessage ), L"%hs", szMessage ? szMessage : "" );

        const WCHAR * rgszT[] = { wszIssueSource, WszUtilImageBuildClass(), wszMessage };

        UtilReportEvent(
                eventInformation,
                GENERAL_CATEGORY,
                INTERNAL_TRACE_ID,
                _countof( rgszT ),
                rgszT );
    }

    OSDiagTrackAssertFail( szMessage ? szMessage : "", wszIssueSource );

#ifdef DEBUG
    #error "We would not want to use staging in debug builds, we would always just want to break, but just to check this is ONLY regular retail AssertFail, so no debug code path ends here."
#endif
    if ( FUtilSystemBetaFeatureEnabled_( NULL, NULL, (UtilSystemBetaSiteMode)UlParam( NULL, JET_paramStageFlighting ), EseTestFeatures, L"EseFeatureTestOnly" ) )
    {
        EnterCriticalSection( &g_csError );
        g_tidAssertFired = DwUtilThreadId();

        switch ( g_wAssertAction )
        {
            case JET_AssertExit:
                TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );
                break;

            case JET_AssertFailFast:
                RaiseFailFastException();
                break;

            case JET_AssertBreak:
            case JET_AssertMsgBox:
            case JET_AssertSkippableMsgBox:
                SetLastError(dwSavedGLE);
                UserDebugBreakPoint();
                break;

            case JET_AssertStop:
                for( ; !g_fSkipAssert ; )
                {
                    
                    Sleep( 100 );
                }
                g_fSkipAssert = fFalse;
                break;

            case JET_AssertCrash:
                RaiseFailFastExceptionCheckingSkipAssert();
                break;

            case JET_AssertSkipAll:
                break;
        }

        g_tidAssertFired = 0x0;
        LeaveCriticalSection( &g_csError );
    }

    
    SetLastError(dwSavedGLE);
}

#endif




VOID DefaultReportEnforceFailure( const WCHAR* wszContext, const CHAR* szMessage, const WCHAR* wszIssueSource )
{
    WCHAR wszMessage[ g_cchIssueMsgMax ] = L"FORMAT STRING FAIL";
    OSStrCbFormatW( wszMessage, sizeof( wszMessage ), L"%hs", szMessage ? szMessage : "" );
    const WCHAR *   rgwszT[] = { wszIssueSource, WszUtilImageBuildClass(), wszMessage };

    UtilReportEvent(
            eventError,
            GENERAL_CATEGORY,
            ENFORCE_FAIL,
            _countof( rgwszT ),
            rgwszT );

    OSDiagTrackEnforceFail( wszContext, szMessage, wszIssueSource );
}


BOOL g_fOverrideEnforceFailure = fFalse;
VOID (*g_pfnReportEnforceFailure)( const WCHAR* wszContext, const CHAR* szMessage, const WCHAR* wszIssueSource ) = DefaultReportEnforceFailure;

void (__stdcall *g_pfnEnforceContextFail)( const WCHAR* wszContext, const CHAR* szMessage, const CHAR* szFilename, LONG lLine ) = EnforceContextFail;

void __stdcall EnforceFail( const CHAR* szMessage, const CHAR* szFilename, LONG lLine )
{
    if( g_pfnEnforceContextFail != NULL )
    {
        g_pfnEnforceContextFail( NULL, szMessage, szFilename, lLine );
    }
}

void __stdcall EnforceContextFail( const WCHAR* wszContext, const CHAR* szMessage, const CHAR* szFilename, LONG lLine )
{
    DWORD dwSavedGLE = GetLastError();

    EnterCriticalSection( &g_csError );

    g_fNoWriteAssertEvent = 1;

    if ( g_pfnReportEnforceFailure != NULL )
    {
        WCHAR           wszIssueSource[  g_cchIssueSourceMax ] = L"FORMAT STRING FAIL";
        C_ASSERT( _countof( wszIssueSource ) < 260 );

        ERRFormatIssueSource( wszIssueSource, sizeof(wszIssueSource), dwSavedGLE, szFilename, lLine );

        g_pfnReportEnforceFailure( wszContext, szMessage, wszIssueSource );
    }

    SetLastError( dwSavedGLE );

    AssertTrackAt( fFalse, szMessage, szFilename, lLine );

    if ( !g_fOverrideEnforceFailure )
    {


        RaiseFailFastException();


        TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );
    }


    LeaveCriticalSection( &g_csError );

    SetLastError(dwSavedGLE);
}


#ifdef ENABLE_EXCEPTIONS


typedef DWORD_PTR EXCEPTION;

EXCEPTION (*pfnExceptionInfo)();


const WCHAR wszExceptionCaption[]   = L"JET Exception";
const WCHAR wszExceptionInfo[]      = L"More detailed information can be found in:  ";
const WCHAR wszExceptionPrompt[]        = L"Choose OK to terminate the process or CANCEL to debug the process.";
const WCHAR wszExceptionPrompt2[]   = L"Choose OK to terminate the process (attaching the debugger is impossible during process initialization and termination).";

LOCAL_BROKEN BOOL ExceptionDialog( const WCHAR wszException[] )
{
    WCHAR       wszMessage[1024];
#ifdef RTM
    WCHAR * wszFmt          = L"%s%d.%d%s%s%s%s%s%d%s%x%s%s%s";
#else
    WCHAR * wszFmt          = L"%s%d.%d%s%s%s%s%s%d%s%x%s%s%s%s%s%s%s";
    WCHAR       wszExceptionFilePath[_MAX_PATH];
    _wfullpath( wszExceptionFilePath, wszAssertFile, _countof(wszExceptionFilePath) );
#endif

    const WCHAR wszReleaseHdr[]     = L"Rel. ";
    const WCHAR wszMsgHdr[]         = L": ";
    const WCHAR wszPidHdr[]         = L"PID: ";
    const WCHAR wszTidHdr[]         = L", TID: 0x";

    OSStrCbFormatW( wszMessage, sizeof(wszMessage), wszFmt,
        wszReleaseHdr,
        DwUtilImageBuildNumberMajor(),
        DwUtilImageBuildNumberMinor(),
        wszMsgHdr,
        wszNewLine,
        wszException,
        wszNewLine,
        wszPidHdr,
        DwUtilProcessId(),
        wszTidHdr,
        DwUtilThreadId(),
        wszNewLine,
        wszNewLine,

#ifdef RTM
#else
        wszExceptionInfo,
        wszExceptionFilePath,
        wszNewLine,
        wszNewLine,
#endif
        IsDebuggerAttachable() || IsDebuggerAttached() ? wszExceptionPrompt : wszExceptionPrompt2
        );

    Assert( wcslen( wszMessage ) < _countof( wszMessage ) );
    const INT id = UtilMessageBoxW(
                        wszMessage,
                        wszExceptionCaption,
                        MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP |
                        ( IsDebuggerAttachable() || IsDebuggerAttached() ? MB_OKCANCEL : MB_OK ) );
    return ( IDOK != id );
}


C_ASSERT( EXCEPTION_CONTINUE_EXECUTION == efaContinueExecution );
C_ASSERT( EXCEPTION_CONTINUE_SEARCH == efaContinueSearch );
C_ASSERT( EXCEPTION_EXECUTE_HANDLER == efaExecuteHandler );

EExceptionFilterAction _ExceptionFail( const CHAR* szMessage, EXCEPTION exception )
{
    PEXCEPTION_POINTERS pexp            = PEXCEPTION_POINTERS( exception );
    PEXCEPTION_RECORD   pexr            = pexp->ExceptionRecord;
    PCONTEXT            pcxr            = pexp->ContextRecord;

    const DWORD         dwException     = pexr->ExceptionCode;
    const VOID * const  pvAddress       = pexr->ExceptionAddress;

    const WCHAR *       wszException        = L"UNKNOWN";

    DWORD dwSavedGLE = GetLastError();

    if ( JET_ExceptionFailFast == g_wExceptionAction )
    {
        TryRaiseFailFastException( pexr, pcxr, 0 );
    }

    EnterCriticalSection( &g_csError );

    if ( g_fRetryException )
    {
        LeaveCriticalSection( &g_csError );
        g_fRetryException = fFalse;
        return efaContinueSearch;
    }

    if ( g_tidAssertFired )
    {
        LeaveCriticalSection( &g_csError );
        return efaContinueSearch;
    }

#if !defined( DEBUG ) && defined( RTM )


    EExceptionFilterAction efa = EExceptionFilterAction( UnhandledExceptionFilter( pexp ) );


    if ( efa == efaExecuteHandler )
    {
        TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );
        LeaveCriticalSection( &g_csError );
        return efaExecuteHandler;
    }


    else
    {
        LeaveCriticalSection( &g_csError );
        return efa;
    }

#else


    switch( dwException )
    {
#ifdef SZEXP
#error  SZEXP already defined
#endif

#define SZEXP( EXP )                    \
        case EXP:                       \
            wszException = WIDEN( #EXP );   \
            break;

        SZEXP( EXCEPTION_ACCESS_VIOLATION );
        SZEXP( EXCEPTION_ARRAY_BOUNDS_EXCEEDED );
        SZEXP( EXCEPTION_BREAKPOINT );
        SZEXP( EXCEPTION_DATATYPE_MISALIGNMENT );
        SZEXP( EXCEPTION_FLT_DENORMAL_OPERAND );
        SZEXP( EXCEPTION_FLT_DIVIDE_BY_ZERO );
        SZEXP( EXCEPTION_FLT_INEXACT_RESULT );
        SZEXP( EXCEPTION_FLT_INVALID_OPERATION );
        SZEXP( EXCEPTION_FLT_OVERFLOW );
        SZEXP( EXCEPTION_FLT_STACK_CHECK );
        SZEXP( EXCEPTION_FLT_UNDERFLOW );
        SZEXP( EXCEPTION_ILLEGAL_INSTRUCTION );
        SZEXP( EXCEPTION_IN_PAGE_ERROR );
        SZEXP( EXCEPTION_INT_DIVIDE_BY_ZERO );
        SZEXP( EXCEPTION_INT_OVERFLOW );
        SZEXP( EXCEPTION_INVALID_DISPOSITION );
        SZEXP( EXCEPTION_NONCONTINUABLE_EXCEPTION );
        SZEXP( EXCEPTION_PRIV_INSTRUCTION );
        SZEXP( EXCEPTION_SINGLE_STEP );
        SZEXP( EXCEPTION_STACK_OVERFLOW );

#undef SZEXP
    }



    {
        CPRINTFFILE cprintffileAssert( wszAssertFile );

        cprintffileAssert(  "JET Exception: Function \"%hs\" raised exception 0x%08X (%ws) at address 0x%0*I64X (base:0x%0*I64X, exr:0x%0*I64X, cxr:0x%0*I64X).",
                            szMessage,
                            dwException,
                            wszException,
                            sizeof( LONG_PTR ) * 2,
                            (QWORD)pvAddress,
                            sizeof( LONG_PTR ) * 2,
                            (QWORD)PvUtilImageBaseAddress(),
                            sizeof( LONG_PTR ) * 2,
                            (QWORD)pexr,
                            sizeof( LONG_PTR ) * 2,
                            (QWORD)pcxr );
    }



    WCHAR szT[512];
    OSStrCbFormatW(     szT, sizeof(szT),
                L"JET Exception: Function \"%hs\" raised exception 0x%08X (%ws) at address 0x%0*I64X (base:0x%0*I64X, exr:0x%0*I64X, cxr:0x%0*I64X).",
                szMessage,
                dwException,
                wszException,
                (DWORD)sizeof( LONG_PTR ) * 2,
                (QWORD)pvAddress,
                (DWORD)sizeof( LONG_PTR ) * 2,
                (QWORD)PvUtilImageBaseAddress(),
                (DWORD)sizeof( LONG_PTR ) * 2,
                (QWORD)pexr,
                (DWORD)sizeof( LONG_PTR ) * 2,
                (QWORD)pcxr );
    Assert( wcslen( szT ) < _countof(szT) );

    BOOL fDebug = ExceptionDialog( szT );


    if ( fDebug )
    {

        static DWORD dwExceptionCode = pexp->ExceptionRecord->ExceptionCode;


        SetLastError(dwSavedGLE);
        UserDebugBreakPoint();



        g_fRetryException = fTrue;
        LeaveCriticalSection( &g_csError );
        return efaContinueExecution;
    }


    TerminateProcess( GetCurrentProcess(), UINT( ~0 ) );

    LeaveCriticalSection( &g_csError );

    return efaExecuteHandler;

#endif
}

#endif


const DWORD ExceptionId( EXCEPTION exception )
{
    PEXCEPTION_POINTERS pexp = PEXCEPTION_POINTERS( exception );

    return pexp->ExceptionRecord->ExceptionCode;
}




ERR g_errTrap = JET_errSuccess;

ERR ErrERRSetErrTrap( const ERR errSet )
{
    const ERR errRet = g_errTrap;
    g_errTrap = errSet;
    return errRet;
}

#ifdef DEBUG

ERR ErrERRCheck_( const ERR err, const CHAR* szFile, const LONG lLine )
{
    DWORD dwSavedGLE = GetLastError();

    AssertRTL( err > -65536 && err < 65536 );

    if ( FOSRefTraceErrors() )
    {
        OSTraceWriteRefLog( ostrlSystemFixed, sysosrtlErrorThrow, (void*)(INT_PTR)err );
    }

    while ( g_tidAssertFired )
    {
        if ( g_tidAssertFired == DwUtilThreadId() )
        {
            return err;
        }
        UtilSleep( 1000 );
    }

    if ( err == g_errTrap )
    {
        #ifndef DEBUG
        #error "Was only attempting to enable this code for DEBUG"
        #endif
        FireWallAt( OSFormat( "ErrTrap:%d", g_errTrap ), szFile, lLine );
    }

    OSTrace( JET_tracetagErrors, OSFormat( "Error %d (0x%x) returned from %s@%d", err, err, szFile, lLine ) );

    
    switch( err )
    {
        case JET_errSuccess:
            AssertSz( fFalse, "Shouldn't call ErrERRCheck() with JET_errSuccess." );
            break;

        case JET_errDerivedColumnCorruption:
            AssertSz( fFalse, "Corruption detected in column space of derived columns." );
            break;

        default:
            break;
    }

    PefLastThrow()->Set( szFile, lLine, err );

    SetLastError( dwSavedGLE );

    return err;
}


void ERRSetLastCall( __in const CHAR* szFile, __in const LONG lLine, __in const ERR err )
{
    Postls()->ulLineLastCall = lLine;
    Postls()->szFileLastCall = szFile;
    Postls()->errLastCall    = err;
}

#endif


#ifdef DEBUG



BOOL FOSSetCleanupState(const BOOL fInCleanupState)
{
    const BOOL fInCleanupStateSaved = Postls()->fCleanupState;
    Postls()->fCleanupState = fInCleanupState;
    return fInCleanupStateSaved;
}

inline BOOL FOSGetCleanupState()
{
    return Postls()->fCleanupState;
}

#endif



#ifdef TEST_INJECTION

#include "_testinjection.hxx"

 TESTINJECTION    g_rgTestInjections[g_cTestInjectionsMax];
LOCAL INT                       g_cTestInjections;

ULONG                   g_ulIDTrap = 0;

LOCAL CRITICAL_SECTION          g_csTestInjections;
LOCAL BOOL                      g_fcsTestInjectionsInit;

ERR ErrEnableTestInjection( const ULONG ulID, const ULONG_PTR pv, const INT type, const ULONG ulProbability, const DWORD grbit )
{
    ERR err = JET_errSuccess;
    const JET_API_PTR pvT = pv;
    const JET_TESTINJECTIONTYPE typeT = (JET_TESTINJECTIONTYPE)type;
    TESTINJECTION injectionNew( ulID, pvT, ulProbability, grbit );

    Assert( g_fcsTestInjectionsInit );
    EnterCriticalSection( &g_csTestInjections );

    const BOOL fCleanup = ( grbit & JET_bitInjectionProbabilityCleanup ) != 0;

    if (
            ( ulID == ulIDInvalid ) ||
            ( ( typeT < JET_TestInjectMin || typeT >= JET_TestInjectMax ) && !fCleanup )
        )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    if (
            ( grbit == JET_bitNil ) ||
            ( ( grbit & JET_bitInjectionProbabilityPct ) && ( grbit & JET_bitInjectionProbabilityCount ) ) ||
            ( !( grbit & JET_bitInjectionProbabilityCount ) &&
                ( ( grbit & JET_bitInjectionProbabilityPermanent ) || ( grbit & JET_bitInjectionProbabilityFailUntil ) ) ) ||
            ( ( grbit & JET_bitInjectionProbabilityPermanent ) && ( grbit & JET_bitInjectionProbabilityFailUntil ) ) ||
            ( fCleanup && ( grbit != JET_bitInjectionProbabilityCleanup ) )
        )
    {
        Call( ErrERRCheck( JET_errInvalidParameter ) );
    }

    switch ( typeT )
    {
            default:
                Call( ErrERRCheck( JET_errTestInjectionNotSupported ) );

#ifdef  FAULT_INJECTION
            case JET_TestInjectFault:
#endif

#ifdef  CONFIGOVERRIDE_INJECTION
            case JET_TestInjectConfigOverride:
#endif

#ifdef  HANG_INJECTION
            case JET_TestInjectHang:
#endif

            break;
    }

    if( g_cTestInjections >= _countof( g_rgTestInjections ) )
    {
        Call( ErrERRCheck( JET_errTooManyTestInjections ) );
    }


    TESTINJECTION* const pinjection = find( g_rgTestInjections, g_rgTestInjections + g_cTestInjections, injectionNew );


    const BOOL fNewInjection = ( pinjection == g_rgTestInjections + g_cTestInjections );

    if( fCleanup && fNewInjection )
    {
    }
    else
    {
        if( fCleanup && !fNewInjection )
        {
            pinjection->TraceStats();
            injectionNew.Disable();
        }

        *pinjection = injectionNew;

        if( fNewInjection )
        {
            ++g_cTestInjections;
            Assert( g_cTestInjections <= _countof( g_rgTestInjections ) );
        }
    }

HandleError:
    LeaveCriticalSection( &g_csTestInjections );

    return err;
}

VOID RFSSuppressFaultInjection( const ULONG ulID )
{
    TESTINJECTION injectionTarget( ulID, NULL, 0, 0 );

    Assert( g_fcsTestInjectionsInit );
    EnterCriticalSection( &g_csTestInjections );

    Assert( ulID != ulIDInvalid );


    TESTINJECTION* const pinjection = find( g_rgTestInjections, g_rgTestInjections + g_cTestInjections, injectionTarget );

    if ( pinjection && pinjection->Id() == ulID )
    {
        pinjection->Suppress();
    }

    LeaveCriticalSection( &g_csTestInjections );
}

VOID RFSUnsuppressFaultInjection( const ULONG ulID )
{
    TESTINJECTION injectionTarget( ulID, NULL, 0, 0 );

    Assert( g_fcsTestInjectionsInit );
    EnterCriticalSection( &g_csTestInjections );

    Assert( ulID != ulIDInvalid );


    TESTINJECTION* const pinjection = find( g_rgTestInjections, g_rgTestInjections + g_cTestInjections, injectionTarget );

    if ( pinjection && pinjection->Id() == ulID )
    {
        pinjection->Unsuppress();
    }

    LeaveCriticalSection( &g_csTestInjections );
}


inline BOOL FRFSThreadEnabled();

INLINE TESTINJECTION* PinjectionFind_( const ULONG ulID )
{
    if( 0 == g_cTestInjections )
    {
        return fFalse;
    }

    TESTINJECTION injectionSearch( ulID, NULL, 0, 0x0 );


    TESTINJECTION* pinjection;
    const TESTINJECTION* const pinjectionTail = g_rgTestInjections + g_cTestInjections;
    if( ( pinjection = find( g_rgTestInjections, (TESTINJECTION*)pinjectionTail, injectionSearch ) ) != pinjectionTail )
    {
        Assert( pinjection->Id() == ulID );
        return pinjection;
    }

    return NULL;
}

INLINE BOOL FTestInjection_( const ULONG ulID, JET_API_PTR* const ppv )
{
    TESTINJECTION * const pinjection = PinjectionFind_( ulID );
    if ( pinjection )
    {
        if ( pinjection->FProbable() )
        {
            *ppv = pinjection->Pv();

            if ( g_ulIDTrap == pinjection->Id() || g_ulIDTrap == ulIDInvalid )
            {
                AssertSz( fFalse, "Test Injection Trap" );
            }

            return fTrue;
        }
    }

    return fFalse;
}

QWORD ChitsFaultInj( const ULONG ulID )
{
    TESTINJECTION * const pinjection = PinjectionFind_( ulID );
    if ( pinjection )
    {
        return pinjection->Chits();
    }

    return 0;
}


#else

ERR ErrEnableTestInjection( const ULONG ulID, const ULONG_PTR pv, const INT type, const ULONG ulProbability, const DWORD grbit )
{
    return ErrERRCheck( JET_errTestInjectionNotSupported );
}

#endif


#ifdef FAULT_INJECTION

ERR ErrFaultInjection_( const ULONG ulID, const CHAR * const szFile, const LONG lLine )
{
    JET_API_PTR pv;

    if ( FTestInjection_( ulID, &pv ) )
    {
        return ( pv && ulID != 63560 ) ? ErrERRCheck_( (ERR)pv, szFile, lLine ) : (ERR)pv;
    }

    return JET_errSuccess;
}

#endif

#ifdef CONFIGOVERRIDE_INJECTION

JET_API_PTR UlConfigOverrideInjection_( const ULONG ulID, const JET_API_PTR ulDefault )
{
    JET_API_PTR pv;

    if ( FTestInjection_( ulID, &pv ) )
    {
        return pv;
    }

    return ulDefault;
}

#endif


#ifdef HANG_INJECTION

void HangInjection_( const ULONG ulID )
{
    JET_API_PTR pv;

    if ( FTestInjection_( ulID, &pv ) )
    {
        if ( bitHangInjectSleep & pv )
        {
            UtilSleep( ~mskHangInjectOptions & pv );
        }
    }
}

#endif




    

const DWORD cRFSDisable             = (DWORD)-1;
const DWORD cRFSBreak               = (DWORD)-2;
const DWORD maskRFSThreadCountdown  = 0x80000000;



LOCAL const CHAR szDisableRFS[]                = "Disable RFS";
LOCAL const CHAR szLogJETCall[]                = "Enable JET Call Logging";
LOCAL const CHAR szLogRFS[]                    = "Enable RFS Logging";
LOCAL const CHAR szRFSAlloc[]                  = "RFS Allocations (-1 to allow all)";
LOCAL const CHAR szRFSIO[]                     = "RFS IOs (-1 to allow all)";



LOCAL const DWORD_PTR rgrgdwRFS2Defaults[][2] =
{
    (DWORD_PTR)szDisableRFS,            fTrue,          
    (DWORD_PTR)szLogJETCall,            fFalse,         
    (DWORD_PTR)szLogRFS,                fFalse,         
    (DWORD_PTR)szRFSAlloc,              cRFSDisable,    
    (DWORD_PTR)szRFSIO,                 cRFSDisable,    
    (DWORD_PTR)NULL,                    0,              
};



BOOL g_fDisableRFS      = fTrue;
BOOL g_fKnownRFSLeak    = fFalse;
BOOL g_fLogJETCall      = fFalse;
BOOL g_fLogRFS          = fFalse;
DWORD g_cRFSAlloc       = cRFSDisable;
DWORD g_cRFSIO          = cRFSBreak;



void EnableDisableRFS()
{
    if( cRFSDisable == g_cRFSIO && cRFSDisable == g_cRFSAlloc  )
    {
        g_fDisableRFS = fTrue;
    }
    else
    {
        g_fDisableRFS = fFalse;
    }
}

void COSLayerPreInit::SetRFSAlloc( ULONG cRFSAlloc )
{
    g_cRFSAlloc = cRFSAlloc;
    EnableDisableRFS();
}

void COSLayerPreInit::SetRFSIO( ULONG cRFSIO )
{
    g_cRFSIO = cRFSIO;
    EnableDisableRFS();
}

#ifdef RFS2

inline BOOL FRFSThreadEnabled()
{
    return ( ( Postls()->cRFSCountdown & maskRFSThreadCountdown ) == 0 );
}

inline LONG CRFSThreadCountdown()
{
    return ( Postls()->cRFSCountdown & ~maskRFSThreadCountdown );
}

inline void RFSThreadDecrementCountdown()
{
    Assert( !FRFSThreadEnabled() );
    Postls()->cRFSCountdown = ( ( CRFSThreadCountdown() - 1 ) | maskRFSThreadCountdown );
}

inline void RFSDecrementCount( LONG* const plTarget )
{
    volatile LONG lTarget = *plTarget;
    Assert( lTarget >= 0 );
    OSSYNC_FOREVER
    {
        if ( ( lTarget <= 0 ) || ( AtomicCompareExchange( plTarget, lTarget, lTarget - 1 ) == lTarget ) )
        {
            break;
        }
        lTarget = *plTarget;
    }
    Assert( lTarget >= 0 );
}

    

inline BOOL UtilRFSLog( const WCHAR* const wszType, const BOOL fPermitted )
{
    const WCHAR *   rgszT[1];

    if ( !fPermitted )
    {
        g_fLogJETCall = fTrue;
    }

    if ( !g_fLogRFS && fPermitted )
    {
        return fPermitted;
    }

    rgszT[0] = wszType;

    UtilReportEvent(
            fPermitted ? eventInformation : eventWarning,
            RFS2_CATEGORY,
            fPermitted ? RFS2_PERMITTED_ID : RFS2_DENIED_ID,
            1,
            rgszT );

    return fPermitted;
}

BOOL UtilRFSAlloc( const WCHAR* const wszType, const INT Type )
{

    
    if ( FOSGetCleanupState() && Type == UnknownAllocResource )
    {
        AssertSz( fFalse, "Cleanup codepaths should not allocate resources." );
    }

    

    if ( g_fDisableRFS )
    {
        return UtilRFSLog( wszType, fTrue );
    }

    

    if ( ( ( cRFSBreak == g_cRFSAlloc && Type == 0 ) ||
            ( cRFSBreak == g_cRFSIO && Type == 1 ) ) &&
            !g_fDisableRFS )
    {
        UserDebugBreakPoint();
    }

    DWORD* pcRFSGlobal = &g_cRFSAlloc;

    switch ( Type )
    {
        case 0:
            pcRFSGlobal = &g_cRFSAlloc;
            break;

        case 1:
            pcRFSGlobal = &g_cRFSIO;
            break;

        default:
            AssertSz( fFalse, "Unknown RFS type %u.", Type );
            break;
    }

    if ( ( *pcRFSGlobal == cRFSDisable ) || g_fDisableRFS )
    {
        return UtilRFSLog( wszType, fTrue );
    }
    if ( *pcRFSGlobal == 0 )
    {
        const BOOL fRFSThreadEnabled = FRFSThreadEnabled();
        const LONG cRFSThreadCountdown = CRFSThreadCountdown();
        if ( !fRFSThreadEnabled && ( cRFSThreadCountdown > 0 ) )
        {
            RFSThreadDecrementCountdown();
        }
        if ( fRFSThreadEnabled || ( cRFSThreadCountdown > 0 ) )
        {
            return UtilRFSLog( wszType, fFalse );
        }
        else
        {
            return UtilRFSLog( wszType, fTrue );
        }
    }
    else
    {
        RFSDecrementCount( (LONG*)pcRFSGlobal );
        return UtilRFSLog( wszType, fTrue );
    }
}

BOOL FRFSFailureDetected( const UINT Type )
{
    if ( g_fDisableRFS )
    {
        return fFalse;
    }

    if ( 0 == Type )
    {
        return ( 0 == g_cRFSAlloc );
    }
    else if ( 1 == Type )
    {
        return ( 0 == g_cRFSIO );
    }

    AssertSz( fFalse, "Unknown RFS type %u.", Type );
    return fFalse;
}

BOOL FRFSAnyFailureDetected()
{
    for ( UINT type = 0; type < RFSTypeMax; type++ )
    {
        if ( FRFSFailureDetected( type ) )
        {
            return fTrue;
        }
    }

    if ( FNegTestSet( fDiskIOError ) || FNegTestSet( fOutOfMemory ) )
    {
        return fTrue;
    }

    return fFalse;
}

void RFSSetKnownResourceLeak()
{
    g_fKnownRFSLeak = fTrue;
}

BOOL FRFSKnownResourceLeak()
{
    return g_fKnownRFSLeak;
}


LONG RFSThreadDisable( const LONG cRFSCountdown )
{
    const LONG cRFSCountdownOld = Postls()->cRFSCountdown;
    Postls()->cRFSCountdown = ( cRFSCountdown | maskRFSThreadCountdown );

    return cRFSCountdownOld;
}


void RFSThreadReEnable( const LONG cRFSCountdownOld )
{
    Assert( !FRFSThreadEnabled() );
    Postls()->cRFSCountdown = cRFSCountdownOld;
}

    

void UtilRFSLogJETCall( const CHAR* const szFunc, const ERR err, const CHAR* const szFile, const unsigned Line )
{
    WCHAR           rgrgchT[2][16];
    WCHAR           szFileName[ 260 ];
    WCHAR           szFuncName[ 64 ];
    const WCHAR *   rgszT[4];
    ERR             errTemp;

    if ( err >= 0 || !g_fLogJETCall )
    {
        return;
    }

    errTemp = ErrOSStrCbFormatW( szFuncName, sizeof( szFuncName ), L"%hs", szFunc );
    Assert( JET_errSuccess <= errTemp || JET_errBufferTooSmall == errTemp );

    rgszT[0] = szFuncName;

    OSStrCbFormatW( rgrgchT[0], sizeof( rgrgchT[0] ), L"%d", err );
    rgszT[1] = rgrgchT[0];

    OSStrCbFormatW( szFileName, sizeof( szFileName ), L"%hs", szFile );
    rgszT[2] = szFileName;

    OSStrCbFormatW( rgrgchT[1], sizeof( rgrgchT[1] ), L"%d", Line );
    rgszT[3] = rgrgchT[1];

    UtilReportEvent(
            eventInformation,
            RFS2_CATEGORY,
            RFS2_JET_CALL_ID,
            4,
            rgszT );
}

    

void UtilRFSLogJETErr( const ERR err, const CHAR* const szLabel, const CHAR* const szFile, const unsigned Line )
{
    WCHAR           rgrgchT[2][16];
    WCHAR           szFileName[ 260 ];
    WCHAR           szLabelName[ 64 ];
    const WCHAR *   rgszT[4];

    if ( !g_fLogJETCall )
    {
        return;
    }

    OSStrCbFormatW( rgrgchT[0], sizeof( rgrgchT[0] ), L"%d", err );
    rgszT[0] = rgrgchT[0];

    OSStrCbFormatW( szLabelName, sizeof( szLabelName ), L"%hs", szLabel );
    rgszT[1] = szLabelName;

    OSStrCbFormatW( szFileName, sizeof( szFileName ), L"%hs", szFile );
    rgszT[2] = szFileName;

    OSStrCbFormatW( rgrgchT[1], sizeof( rgrgchT[1] ), L"%d", Line );
    rgszT[3] = rgrgchT[1];

    UtilReportEvent(
        eventInformation,
        PERFORMANCE_CATEGORY,
        RFS2_JET_ERROR_ID,
        4,
        rgszT );
}

BOOL RFSError::Check( ERR err, ... ) const
{
    va_list arg_ptr;
    va_start( arg_ptr, err );

    for ( ; err != 0; err = va_arg( arg_ptr, ERR ) )
    {
        Assert( err > -9000 && err < 9000 );
        if ( m_err == err )
        {
            break;
        }
    }

    va_end( arg_ptr );
    return (err != 0);
}
#else

inline BOOL FRFSThreadEnabled()
{
    return fFalse;
}

#endif



void OSErrorPostterm()
{

#ifdef TEST_INJECTION
    if( g_fcsTestInjectionsInit )
    {
        DeleteCriticalSection( &g_csTestInjections );
        g_fcsTestInjectionsInit = fFalse;
    }
    for ( TESTINJECTION* pinjection = g_rgTestInjections; pinjection < ( g_rgTestInjections + g_cTestInjections ); pinjection++ )
    {
        pinjection->TraceStats();
    }
    g_cTestInjections = 0;
#endif

    if ( g_fCritSecErrorInit )
    {
        DeleteCriticalSection( &g_csError );
        g_fCritSecErrorInit = fFalse;
    }
}


BOOL FOSErrorPreinit()
{

    if ( !InitializeCriticalSectionAndSpinCount( &g_csError, 0 ) )
    {
        goto HandleError;
    }
    g_fCritSecErrorInit = fTrue;

#ifdef TEST_INJECTION
    if ( !InitializeCriticalSectionAndSpinCount( &g_csTestInjections, 0 ) )
    {
        goto HandleError;
    }
    g_fcsTestInjectionsInit = fTrue;
#endif

    return fTrue;

HandleError:
    OSErrorPostterm();
    return fFalse;
}



void OSErrorTerm()
{
}


ERR ErrOSErrorInit()
{

    return JET_errSuccess;
}


#ifndef RTM
DWORD   g_grbitNegativeTesting  = 0x0;
#endif


BOOL FUtilRepairIntegrityMsgBox( const WCHAR * const wszMsg )
{
    const WCHAR wszCaptionWarning[]     = L"Warning";
    const INT       id                      = UtilMessageBoxW(
                                                    wszMsg,
                                                    wszCaptionWarning,
                                                    MB_ICONSTOP | MB_OKCANCEL );
    return ( IDCANCEL != id );
}

ERR ErrOSErrFromWin32Err( __in DWORD dwWinError, __in ERR errDefault )
{
    switch ( dwWinError )
    {
        case NO_ERROR:
            return JET_errSuccess;

        case ERROR_DISK_FULL:
            return ErrERRCheck( JET_errDiskFull );

        case ERROR_HANDLE_EOF:
        case ERROR_VC_DISCONNECTED:
        case ERROR_IO_DEVICE:
        case ERROR_DEVICE_NOT_CONNECTED:
            return ErrERRCheck( JET_errDiskIO );

        case ERROR_FILE_CORRUPT:
        case ERROR_DISK_CORRUPT:
            return ErrERRCheck( JET_errFileSystemCorruption );

        case ERROR_NOT_READY:
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
            return ErrERRCheck( JET_errFileNotFound );

        case ERROR_PATH_NOT_FOUND:
        case ERROR_DIRECTORY:
        case ERROR_BAD_NET_NAME:
        case ERROR_BAD_NETPATH:
        case ERROR_BAD_PATHNAME:
        case ERROR_INVALID_NAME:
            return ErrERRCheck( JET_errInvalidPath );

        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
        case ERROR_LOCK_VIOLATION:
        case ERROR_WRITE_PROTECT:
            return ErrERRCheck( JET_errFileAccessDenied );

        case ERROR_TOO_MANY_OPEN_FILES:
            return ErrERRCheck( JET_errOutOfFileHandles );
            break;

        case ERROR_NO_SYSTEM_RESOURCES:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_WORKING_SET_QUOTA:
            return ErrERRCheck( JET_errOutOfMemory );

        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return ErrERRCheck( JET_errFileAlreadyExists );

        default:
            return ErrERRCheck( errDefault );
    }
}

ERR ErrOSErrFromWin32Err(__in DWORD dwWinError)
{
    return ErrOSErrFromWin32Err( dwWinError, JET_errInternalError );
}

const CHAR * g_szBadSourceFileName  = "#BadFileName#";



const CHAR * SzSourceFileName( const CHAR * szFilePath )
{
    if ( NULL == szFilePath || szFilePath[0] == '\0' )
    {
        return "";
    }
    if ( NULL == strrchr( szFilePath, chPathDelimiter ) )
    {
        return szFilePath;
    }
    if ( strrchr( szFilePath, chPathDelimiter ) + sizeof( CHAR ) >= szFilePath + strlen( szFilePath ) )
    {
        ExpectedSz( fFalse, "Source Code Path with delimiter at very end of string." );
        return g_szBadSourceFileName;
    }

    return strrchr( szFilePath, chPathDelimiter ) + sizeof( CHAR );
}

